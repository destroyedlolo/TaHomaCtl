/*
 * avahi_debug_simple_retry.c
 *
 * Simple Avahi (mDNS) browser + resolver for debugging.
 * Compatible with Avahi 0.8 (Arch Linux) + non-blocking retry mechanism.
 *
 * Compilation : 
 * gcc -std=c11 -Wall -O2 avahi_debug_simple_retry.c -o avahi_debug $(pkg-config --cflags --libs avahi-client)
 *
 * GPLv3
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/time.h> 

#include <avahi-common/defs.h>
#include <avahi-client/client.h>
#include <avahi-client/lookup.h>
#include <avahi-common/simple-watch.h>
#include <avahi-common/strlst.h>
#include <avahi-common/address.h>
#include <avahi-common/error.h>
#include <avahi-common/malloc.h> 

/* Déclarations anticipées */
static void resolve_callback(
    AvahiServiceResolver *r,
    AvahiIfIndex interface,
    AvahiProtocol protocol,
    AvahiResolverEvent event,
    const char *name,
    const char *type,
    const char *domain,
    const char *host_name,
    const AvahiAddress *address,
    uint16_t port,
    AvahiStringList *txt,
    AvahiLookupResultFlags flags,
    void* userdata);

static void resolve_retry_schedule(AvahiTimeout *t, void *userdata);

static void record_callback(
    AvahiRecordBrowser *b,
    AvahiIfIndex interface,
    AvahiProtocol protocol,
    AvahiBrowserEvent event,
    const char *name,
    uint16_t cls,
    uint16_t type,
    const void *rdata,
    size_t size,
    AvahiLookupResultFlags flags,
    void* userdata);

static void client_callback(AvahiClient *c, AvahiClientState state, void *userdata);


/* --- Global Tracker pour Éviter les Résolutions Dupliquées --- */

typedef struct ActiveResolver {
    char *name;
    struct ActiveResolver *next;
} ActiveResolver;

static ActiveResolver *active_resolvers = NULL;

static int is_resolver_active(const char *name) {
    ActiveResolver *curr = active_resolvers;
    while (curr) {
        if (strcmp(curr->name, name) == 0) {
            return 1;
        }
        curr = curr->next;
    }
    return 0;
}

static void add_active_resolver(const char *name) {
    ActiveResolver *new_entry = (ActiveResolver *)avahi_malloc(sizeof(ActiveResolver));
    if (!new_entry) return;
    new_entry->name = avahi_strdup(name);
    if (!new_entry->name) {
        avahi_free(new_entry);
        return;
    }
    new_entry->next = active_resolvers;
    active_resolvers = new_entry;
}

static void remove_active_resolver(const char *name) {
    ActiveResolver **ptr = &active_resolvers;
    while (*ptr) {
        if (strcmp((*ptr)->name, name) == 0) {
            ActiveResolver *temp = *ptr;
            *ptr = temp->next;
            avahi_free(temp->name);
            avahi_free(temp);
            return;
        }
        ptr = &(*ptr)->next;
    }
}

/* Structure pour passer le contexte de relance au timer */
typedef struct {
    char *name;   
    char *type;   
    char *domain; 

    AvahiClient *client; 
    AvahiPoll *poll;     
    AvahiIfIndex interface;
    AvahiProtocol protocol;
    struct timeval tv_target; 
} RetryContext;

/* --- Fonctions Utilitaires --- */

/* Timestamp HH:MM:SS:ms */
static void timestamp_now(char *buf, size_t len) {
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    struct tm tm;
    localtime_r(&ts.tv_sec, &tm);
    snprintf(buf, len, "%02d:%02d:%02d:%03ld",
             tm.tm_hour, tm.tm_min, tm.tm_sec,
             ts.tv_nsec / 1000000);
}

static void lookup_flags_to_string(AvahiLookupResultFlags flags, char *out, size_t outlen){
    out[0] = '\0'; 
    if(flags & AVAHI_LOOKUP_RESULT_CACHED) strncat(out, "CACHED", outlen - 1); 
    if(flags & AVAHI_LOOKUP_RESULT_MULTICAST) {
        if (out[0] != '\0') strncat(out, "|", outlen - strlen(out) - 1);
        strncat(out, "MULTICAST", outlen - strlen(out) - 1); 
    }
    if(out[0] == '\0') strncat(out, "NONE", outlen - 1);
}

/* Convert DNS Type to string (minimal implementation) */
static const char* dns_type_to_string(uint16_t type) {
    switch(type) {
        case AVAHI_DNS_TYPE_A: return "A (IPv4 Address)";
        case AVAHI_DNS_TYPE_AAAA: return "AAAA (IPv6 Address)";
        case AVAHI_DNS_TYPE_PTR: return "PTR (Pointer)";
        case AVAHI_DNS_TYPE_TXT: return "TXT (Text)";
        case AVAHI_DNS_TYPE_SRV: return "SRV (Service)";
        case AVAHI_DNS_TYPE_CNAME: return "CNAME (Alias)";
        default: return "(Unknown Type)";
    }
}

/* Convert DNS Class to string (minimal implementation) */
static const char* dns_class_to_string(uint16_t cls) {
    switch(cls) {
        case AVAHI_DNS_CLASS_IN: return "IN (Internet)";
        default: return "(Unknown Class)";
    }
}


/* Décodage et affichage du RDATA (minimal implementation) */
static void display_rdata(uint16_t type, const void *rdata, size_t size) {
    char address_str[AVAHI_ADDRESS_STR_MAX];
    
    printf("\t\tRDATA (%zu bytes):\n", size);

    if (rdata == NULL || size == 0) return;

    switch(type) {
        case AVAHI_DNS_TYPE_A:
            if (size == 4) {
                inet_ntop(AF_INET, rdata, address_str, sizeof(address_str));
                printf("\t\t\tAddress: %s\n", address_str);
            }
            break;
        case AVAHI_DNS_TYPE_AAAA:
            if (size == 16) {
                inet_ntop(AF_INET6, rdata, address_str, sizeof(address_str));
                printf("\t\t\tAddress: %s\n", address_str);
            }
            break;
        default:
            printf("\t\t\t(Data not parsed - type %u)\n", type);
            break;
    }
}

/* Record Browser callback */
static void record_callback(
    AvahiRecordBrowser *b,
    AvahiIfIndex interface,
    AvahiProtocol protocol,
    AvahiBrowserEvent event,
    const char *name,
    uint16_t cls,
    uint16_t type,
    const void *rdata,
    size_t size,
    AvahiLookupResultFlags flags,
    void* userdata)
{
    (void)b; (void)interface; (void)protocol; (void)userdata;
    char ts[16];
    timestamp_now(ts,sizeof(ts));
    char flagstr[128];
    lookup_flags_to_string(flags, flagstr, sizeof(flagstr));

    const char *type_str = dns_type_to_string(type);
    const char *class_str = dns_class_to_string(cls);
    
    switch (event) {
        case AVAHI_BROWSER_NEW:
            printf("%s RECORD_FOUND: name='%s' class=%s (0x%x) type=%s (0x%x) flags=0x%x (%s)\n", 
                ts, name, class_str, cls, type_str, type, flags, flagstr);
            display_rdata(type, rdata, size);
            break;

        case AVAHI_BROWSER_REMOVE:
            printf("%s RECORD_REMOVED: name='%s' class=%s (0x%x) type=%s (0x%x) flags=0x%x (%s)\n", 
                ts, name, class_str, cls, type_str, type, flags, flagstr);
            break;

        case AVAHI_BROWSER_FAILURE:
            printf("%s RECORD_BROWSER_FAILURE: name='%s'\n", ts, name);
            break;
        default:
            break;
    }
}

/* --- Logique de Relance Non Bloquante --- */

/* Fonction de rappel pour la relance */
static void resolve_retry_schedule(AvahiTimeout *t, void *userdata) {
    RetryContext *ctx = (RetryContext*) userdata;
    AvahiClient *client = ctx->client;
    AvahiPoll *poll = ctx->poll; 
    char ts[16];
    timestamp_now(ts,sizeof(ts));
    
    printf("%s RESOLVE_RETRY: Timeout expired, restarting resolution for '%s' )...\n", 
           ts, ctx->name);

    // Lancer la nouvelle résolution en utilisant les paramètres stockés (qui seront UNPEC)
    AvahiServiceResolver *r = avahi_service_resolver_new(
        client, 
        ctx->interface, 
        ctx->protocol, 
        ctx->name, 
        ctx->type, 
        ctx->domain, 
        AVAHI_PROTO_UNSPEC, 
        0, 
        resolve_callback,
        poll 
    );
    
    if (!r) {
        printf("%s RESOLVE_RETRY: FAILED to restart resolution for '%s': %s\n", 
               ts, ctx->name, avahi_strerror(avahi_client_errno(client)));
        // Si la relance échoue, on retire le nom du tracker
        remove_active_resolver(ctx->name);
    }

    // Libération des chaînes allouées dynamiquement
    avahi_free(ctx->name);
    avahi_free(ctx->type);
    avahi_free(ctx->domain);

    free(ctx);
    poll->timeout_free(t); 
}


/* Resolver callback */
static void resolve_callback(
    AvahiServiceResolver *r,
    AvahiIfIndex interface,
    AvahiProtocol protocol,
    AvahiResolverEvent event,
    const char *name,
    const char *type,
    const char *domain,
    const char *host_name,
    const AvahiAddress *address,
    uint16_t port,
    AvahiStringList *txt,
    AvahiLookupResultFlags flags,
    void* userdata)
{
    (void)txt;

    AvahiPoll *poll = (AvahiPoll*)userdata; 

    char ts[16];
    timestamp_now(ts,sizeof(ts));

    AvahiClient *client = avahi_service_resolver_get_client(r);

    switch(event) {
        case AVAHI_RESOLVER_FOUND: {
            char address_str[AVAHI_ADDRESS_STR_MAX]; 

            if (address && avahi_address_snprint(address_str, sizeof(address_str), address) == NULL) {
                 strcpy(address_str, "N/A");
            }
            
            printf("%s RESOLVED: Host='%s' Address='%s' Port=%u\n", ts, host_name, address_str, port); 
            
            // Démarrer un Record Browser pour le nom d'hôte résolu
            AvahiRecordBrowser *rb = avahi_record_browser_new(
                client, 
                interface, 
                protocol, 
                host_name, 
                AVAHI_DNS_CLASS_IN, 
                0xFFFF, // Type ANY (tous les enregistrements)
                0, // flags
                record_callback, 
                NULL 
            );

            if (!rb) {
                 printf("%s RESOLVED: WARNING: Failed to start RecordBrowser for '%s': %s\n", 
                        ts, host_name, avahi_strerror(avahi_client_errno(client)));
            } else {
                 printf("%s RESOLVED: Started RecordBrowser for '%s' (watching records).\n", ts, host_name);
            }
            
            // Retirer le nom du tracker après résolution réussie
            remove_active_resolver(name);
            avahi_service_resolver_free(r);
            return; 
        }
        case AVAHI_RESOLVER_FAILURE: {
            int err = avahi_client_errno(client);
            const char *errstr = avahi_strerror(err);
            printf("%s RESOLVE_FAILED: name='%s' type='%s' domain='%s' -> %s\n",
                   ts, name, type, domain, errstr?errstr:"(unknown error)");
            
            printf("%s RESOLVE_RETRY: Scheduling retry in 5 seconds...\n", ts);
            
            // 1. Allouer et remplir le contexte de relance
            RetryContext *ctx = (RetryContext*) calloc(1, sizeof(RetryContext)); // calloc pour initialisation à zéro
            if (!ctx) { 
                fprintf(stderr, "%s: Memory allocation failed for retry context. Aborting retry.\n", ts);
                remove_active_resolver(name); // Échec définitif
                goto fail; 
            }

            gettimeofday(&ctx->tv_target, NULL); 
            ctx->tv_target.tv_sec += 5; // 5 secondes dans le futur
            
            ctx->client = client;
            ctx->poll = poll; 
            // Stocke les paramètres interface/protocol reçus. 
            // S'ils étaient UNSPEC (depuis browse_callback), ils le resteront pour la relance.
            ctx->interface = interface;
            ctx->protocol = protocol;
            
            // Allocation dynamique des chaînes avec avahi_strdup()
            ctx->name = avahi_strdup(name);
            ctx->type = avahi_strdup(type);
            ctx->domain = avahi_strdup(domain);

            if (!ctx->name || !ctx->type || !ctx->domain) {
                fprintf(stderr, "%s: Dynamic string allocation failed. Aborting retry.\n", ts);
                avahi_free(ctx->name);
                avahi_free(ctx->type);
                avahi_free(ctx->domain);
                free(ctx);
                remove_active_resolver(name); // Échec définitif
                goto fail;
            }
            
            // 2. Créer le timer de relance (passe &ctx->tv_target)
            AvahiTimeout *t = poll->timeout_new(poll, &ctx->tv_target, resolve_retry_schedule, ctx);
            
            if (!t) {
                fprintf(stderr, "%s: Failed to create retry timer. Aborting retry.\n", ts);
                avahi_free(ctx->name);
                avahi_free(ctx->type);
                avahi_free(ctx->domain);
                free(ctx);
                remove_active_resolver(name); // Échec définitif
                goto fail;
            }
            
            // 3. Libérer l'ancien résolveur
            avahi_service_resolver_free(r);
            return; 
        }
    }

fail:
    avahi_service_resolver_free(r);
}

/* Browser callback */
static void browse_callback(
    AvahiServiceBrowser *b,
    AvahiIfIndex interface,
    AvahiProtocol protocol,
    AvahiBrowserEvent event,
    const char *name,
    const char *type,
    const char *domain,
    AvahiLookupResultFlags flags,
    void *userdata)
{
    (void)b;
    AvahiPoll *poll = (AvahiPoll*)userdata;
    char ts[16];
    timestamp_now(ts,sizeof(ts));
    AvahiClient *client=avahi_service_browser_get_client(b);

    switch(event) {
        case AVAHI_BROWSER_NEW:
            // Vérifie si une résolution est déjà active pour ce nom
            if (is_resolver_active(name)) {
                printf("%s NEW: '%s' type='%s' domain='%s' ... Resolution already active, skipping.\n", ts, name, type, domain);
                break;
            }
            
            printf("%s NEW: '%s' type='%s' domain='%s' ... Starting resolution.\n", ts,name,type,domain);
            add_active_resolver(name); // Ajoute le nom au tracker
            
            // CORRIGÉ: Utiliser AVAHI_IF_UNSPEC et AVAHI_PROTO_UNSPEC pour le resolver
            // Cela augmente les chances de succès en demandant une recherche globale, comme avahi-browse -r.
            avahi_service_resolver_new(
                client, 
                AVAHI_IF_UNSPEC, // Interface: UNKNOWN/Global
                AVAHI_PROTO_UNSPEC, // Protocol: UNKNOWN/Global
                name, 
                type, 
                domain, 
                AVAHI_PROTO_UNSPEC, 
                0, // flags
                resolve_callback, 
                poll
            );
            break;
        case AVAHI_BROWSER_REMOVE:
            printf("%s REMOVE: '%s' type='%s' domain='%s' ...\n", ts,name,type,domain);
            break;
        case AVAHI_BROWSER_ALL_FOR_NOW:
            printf("%s ALL_FOR_NOW\n", ts);
            break;
        case AVAHI_BROWSER_CACHE_EXHAUSTED:
            printf("%s CACHE_EXHAUSTED\n", ts);
            break;
        case AVAHI_BROWSER_FAILURE:
            printf("%s BROWSER_FAILURE\n", ts);
            break;
        default: break;
    }
}

/* Client callback */
static void client_callback(AvahiClient *c, AvahiClientState state, void *userdata) {
    (void)c; (void)userdata;
    char ts[16];
    timestamp_now(ts,sizeof(ts));

    switch(state) {
        case AVAHI_CLIENT_CONNECTING:
            printf("%s CLIENT: connecting\n", ts); break;
        case AVAHI_CLIENT_FAILURE:
            printf("%s CLIENT-ERROR: %s\n", ts, avahi_strerror(avahi_client_errno(c)));
            break;
        case AVAHI_CLIENT_S_RUNNING:
            printf("%s CLIENT: running\n", ts); break;
        default: break;
    }
}

/* Main */
int main(int argc,char*argv[]) {
    if(argc<3){ fprintf(stderr,"Usage: %s <service_type> <domain>\n",argv[0]); return 1; }
    
    const char *stype=argv[1]; 
    const char *domain=argv[2];

    AvahiSimplePoll *poll_obj = avahi_simple_poll_new();
    if(!poll_obj){ fprintf(stderr,"Failed to create simple poll\n"); return 1; }

    AvahiPoll *poll = (AvahiPoll*) avahi_simple_poll_get(poll_obj);
    
    AvahiClient *client = avahi_client_new(poll, 0, client_callback, NULL, NULL);
    if(!client){ 
        fprintf(stderr,"Failed to create Avahi client\n"); 
        avahi_simple_poll_free(poll_obj);
        return 1; 
    }

    AvahiServiceBrowser *browser = avahi_service_browser_new(client, AVAHI_IF_UNSPEC, AVAHI_PROTO_UNSPEC, stype, domain, 0, browse_callback, poll);
    if(!browser){ 
        fprintf(stderr,"Failed to create Avahi browser\n"); 
        avahi_client_free(client); avahi_simple_poll_free(poll_obj);
        return 1; 
    }
    
    printf("Starting Avahi event loop...\n");
    avahi_simple_poll_loop(poll_obj);

    avahi_service_browser_free(browser);
    avahi_client_free(client);
    avahi_simple_poll_free(poll_obj);
    
    return 0;
}
