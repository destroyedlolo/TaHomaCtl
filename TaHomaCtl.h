/* TaHomaCtl.h
 *
 *	Shared definition
 *
 */

#ifndef TAHOMACTL_H
#define TAHOMACTL_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <curl/curl.h>

#include <avahi-common/address.h>	/* for AvahiProtocol */

	/* Shared configuration */
extern char *tahoma;	/* Tahoma's hostname */
extern char *ip;		/* Tahoma's IP address */
extern uint16_t port;	/* TaHoma's port */
extern char *token;		/* Bearer Token */
extern bool unsafe;		/* Don't verify SSL chaine */

extern char *url;	/* base API url */
extern size_t url_len;	/* URL's length */
extern long timeout;	/* API calling timeout */

extern AvahiProtocol avahiIP;

extern bool debug;
extern bool trace;
extern unsigned int verbose;

	/* Tokenisation and sub strings' */
struct substring {
	const char *s;	/* NULL if the string is empty */
	size_t len;
};

extern bool extractTokenSub(struct substring *, const char *, const char**);
extern int substringcmp(struct substring *, const char *);

	/* Utilities */
extern const char *FreeAndSet(char **storage, const char *val);	/* Update a storage with a new value */
extern void spent(bool);	/* Time spent. Caution, not reentrant */
extern char *dynstringAdd(char *s, const char *add);	/* Add 'add' string to s */
extern char *dynstringAddSub(char *s, struct substring *add);

	/* Configuration related */
extern void clean(char **);		/* Safe free() an object */
extern void func_scan(const char *);

	/* Response handling */
struct ResponseBuffer {
    char *memory;
    size_t size;
};

void freeResponse(struct ResponseBuffer *);

	/* API calling */
extern CURL *curl;
extern void curl_cleanup(void);
extern void buildURL(void);
extern void callAPI(const char *, const char *, struct ResponseBuffer *);

	/* Response processing */
void func_Tgw(const char *);
void func_scandevs(const char *);
void func_States(const char *);
void func_NStates(const char *);
void func_NCommand(const char *);
void func_Current(const char *);

	/* Devices' */
struct Command {
	struct Command *next;

	const char *command;
	unsigned int nparams;
};

struct State {
	struct State *next;

	const char *state;
};

extern struct Device {
	struct Device *next;

	const char *label;
	const char *url;

	struct Command *commands;
	struct State *states;
} *devices_list;

extern struct Device *findDevice(struct substring *);
extern struct Device *findDeviceByURL(const char *);
extern struct Device *findDeviceByURLSS(struct substring *);

	/* Printing helper */
struct json_object;
extern void printObject(struct json_object *, unsigned int nbre_tab);
#endif
