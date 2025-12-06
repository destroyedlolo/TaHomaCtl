#include <stdio.h>
#include <curl/curl.h>
#include <string.h>
#include <stdlib.h> 

// Variables globales modifiables pour la résolution forcée
char HOST_NAME[] = "toto.local";
int PORT = 8442;
char IP_ADDRESS[] = "192.168.0.25";

// NOUVELLE VARIABLE GLOBALE MODIFIABLE POUR L'AUTORISATION
char AUTH_TOKEN[] = "initial_secret_bearer_token_12345"; 

// Pointeur global pour la liste de résolution, nécessaire pour la libérer proprement
struct curl_slist *global_resolve_list = NULL;
// NOUVEAU POINTEUR GLOBAL POUR LES EN-TETES HTTP
struct curl_slist *global_headers = NULL; 

// Define a structure to hold captured data (used for both body and headers)
struct MemoryData {
    char *memory;
    size_t size;
};

// Déclarations des fonctions
int setup_forced_resolution(CURL *curl_handle);
int setup_bearer_authorization(CURL *curl_handle); // NOUVELLE FONCTION
int perform_request(CURL *curl_handle, const char *url);
static size_t WriteMemoryCallback(void *contents, size_t size, size_t nmemb, void *userp);

// ----------------------------------------------------------------------------
// General Callback function (unchanged)
// ----------------------------------------------------------------------------
static size_t WriteMemoryCallback(void *contents, size_t size, size_t nmemb, void *userp) {
    size_t realsize = size * nmemb;
    struct MemoryData *mem = (struct MemoryData *)userp;

    char *ptr = realloc(mem->memory, mem->size + realsize + 1);

    if (!ptr) {
        fprintf(stderr, "Error: not enough memory (realloc returned NULL)\n");
        return 0;
    }

    mem->memory = ptr;
    memcpy(&(mem->memory[mem->size]), contents, realsize);
    mem->size += realsize;
    mem->memory[mem->size] = 0; 

    return realsize;
}

// ----------------------------------------------------------------------------
// Fonction pour configurer la résolution forcée (inchangée)
// ----------------------------------------------------------------------------
int setup_forced_resolution(CURL *curl_handle) {
    char resolve_entry[256]; 

    if (global_resolve_list != NULL) {
        curl_slist_free_all(global_resolve_list); 
        global_resolve_list = NULL;
    }
    
    if (snprintf(resolve_entry, sizeof(resolve_entry), "%s:%d:%s", 
                 HOST_NAME, PORT, IP_ADDRESS) >= sizeof(resolve_entry)) {
        fprintf(stderr, "Error: Resolve string is too long.\n");
        return 1;
    }
    
    global_resolve_list = curl_slist_append(NULL, resolve_entry);
    if (global_resolve_list == NULL) {
        fprintf(stderr, "Error: curl_slist_append failed.\n");
        return 1;
    }

    curl_easy_setopt(curl_handle, CURLOPT_RESOLVE, global_resolve_list);
    printf("\n[CONFIG] Nouvelle résolution forcée configurée pour %s: %s\n", HOST_NAME, IP_ADDRESS);

    return 0;
}

// ----------------------------------------------------------------------------
// NOUVELLE FONCTION : Configuration de l'entête Authorization: Bearer
// ----------------------------------------------------------------------------
int setup_bearer_authorization(CURL *curl_handle) {
    char auth_header[512]; 
    
    // 1. Libérer l'ancienne liste d'entêtes si elle existe
    if (global_headers != NULL) {
        curl_slist_free_all(global_headers); 
        global_headers = NULL;
    }

    // 2. Créer l'entête au format "Authorization: Bearer <token>"
    if (snprintf(auth_header, sizeof(auth_header), "Authorization: Bearer %s", 
                 AUTH_TOKEN) >= sizeof(auth_header)) {
        fprintf(stderr, "Error: Authorization string is too long.\n");
        return 1;
    }
    
    // 3. Créer la nouvelle liste d'entêtes
    global_headers = curl_slist_append(NULL, auth_header);
    if (global_headers == NULL) {
        fprintf(stderr, "Error: curl_slist_append failed for headers.\n");
        return 1;
    }
    
    // 4. Appliquer les entêtes au handle CURL
    curl_easy_setopt(curl_handle, CURLOPT_HTTPHEADER, global_headers);
    printf("[CONFIG] Nouvelle entête Authorization: Bearer appliquée (Token: %s)\n", AUTH_TOKEN);

    return 0;
}

// ----------------------------------------------------------------------------
// Fonction pour exécuter la requête (mise à jour pour inclure les headers)
// ----------------------------------------------------------------------------
int perform_request(CURL *curl_handle, const char *url) {
    CURLcode res;
    long http_code = 0;
    int result = 0;
    
    // --- Étape Clé 1 : Reconfigurer l'autorisation si nécessaire ---
    result |= setup_bearer_authorization(curl_handle);

    if (result != 0) {
        fprintf(stderr, "Setup failed, aborting request.\n");
        return 1;
    }

    // ... (Reste de la fonction inchangé pour la gestion de la mémoire et l'exécution)

    struct MemoryData body_chunk;
    struct MemoryData header_chunk;
    
    body_chunk.memory = malloc(1); body_chunk.size = 0;
    header_chunk.memory = malloc(1); header_chunk.size = 0;
    
    if (!body_chunk.memory || !header_chunk.memory) {
        fprintf(stderr, "Error: Initial memory allocation failed.\n");
        free(body_chunk.memory);
        free(header_chunk.memory);
        return 1;
    }
    
    curl_easy_setopt(curl_handle, CURLOPT_URL, url);
    curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);
    curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, (void *)&body_chunk);
    curl_easy_setopt(curl_handle, CURLOPT_HEADERFUNCTION, WriteMemoryCallback);
    curl_easy_setopt(curl_handle, CURLOPT_HEADERDATA, (void *)&header_chunk);

    printf("  Requesting URL: %s\n", url);

    res = curl_easy_perform(curl_handle);

    // Vérifier les erreurs et afficher les résultats
    if (res != CURLE_OK) {
        fprintf(stderr, "  curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
        result = 1; 
    } else {
        curl_easy_getinfo(curl_handle, CURLINFO_RESPONSE_CODE, &http_code);
        
        printf("  **HTTP Status Code:** %ld\n", http_code);
        
        printf("\n  **Response Headers (Size %zu bytes):**\n", header_chunk.size);
        if (header_chunk.size > 0) {
            printf(">>>>>>>>>>>>>>> HEADERS START >>>>>>>>>>>>>>>\n");
            printf("%s", header_chunk.memory); 
            printf("<<<<<<<<<<<<<<<< HEADERS END <<<<<<<<<<<<<<<<\n");
        }
        
        printf("\n  **Response Body (Size %zu bytes):**\n", body_chunk.size);
        if (body_chunk.size > 0) {
            printf(">>>>>>>>>>>>>>>>>>> BODY START >>>>>>>>>>>>>>>>>\n");
            printf("%s\n", body_chunk.memory);
            printf("<<<<<<<<<<<<<<<<<<<< BODY END <<<<<<<<<<<<<<<<<\n");
        } else {
            printf("  (No response body received)\n");
        }
        result = 0; 
    }

    free(body_chunk.memory); 
    free(header_chunk.memory);

    return result;
}

// ----------------------------------------------------------------------------
// Fonction Main (Démonstration de la modification du Token)
// ----------------------------------------------------------------------------
int main(void) {
    CURL *curl;
    int global_result = 0;

    printf("--- 1. Initialisation globale de libcurl ---\n");
    curl_global_init(CURL_GLOBAL_DEFAULT);
    
    curl = curl_easy_init();

    if (!curl) {
        fprintf(stderr, "Error: curl_easy_init failed.\n");
        curl_global_cleanup();
        return 1;
    }

    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L); 
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);

    // --- Scénario 1 : Configuration et Appel Initial ---
    printf("\n--- SCÉNARIO 1 : Configuration initiale ---\n");
    global_result |= setup_forced_resolution(curl); 
    // Le token Bearer initial est utilisé ici
    
    printf("\n--- Execution 1: Call to /endpoint ---\n");
    global_result |= perform_request(curl, "https://toto.local:8442/endpoint");
    printf("----------------------------------------\n");

    // --- Scénario 2 : Modification des valeurs (IP et Token) et nouvel appel ---
    printf("\n--- SCÉNARIO 2 : Modification de l'IP et du TOKEN ---\n");
    
    // Modification des variables globales
    strcpy(IP_ADDRESS, "172.16.0.100"); // Nouvelle IP
    PORT = 9443;                       // Nouveau port
    // MISE À JOUR DU TOKEN D'AUTORISATION
    strcpy(AUTH_TOKEN, "new_secure_token_for_request_2_xyz"); 
    
    // Réinitialisation de la résolution forcée avec les nouvelles valeurs
    global_result |= setup_forced_resolution(curl); 

    printf("\n--- Execution 2: Call to /another_endpoint ---\n");
    // L'appel à perform_request mettra automatiquement à jour l'en-tête Authorization
    global_result |= perform_request(curl, "https://toto.local:8442/another_endpoint");
    printf("--------------------------------------------\n");
    
    // 4. Nettoyage
    
    printf("\n--- 3. Nettoyage ---\n");
    curl_slist_free_all(global_resolve_list); 
    curl_slist_free_all(global_headers);       // NOUVEAU NETTOYAGE
    curl_easy_cleanup(curl);           
    curl_global_cleanup();             
    
    return global_result;
}
