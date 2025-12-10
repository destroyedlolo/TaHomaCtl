/* Call Tahoma's API
 */

#include "TaHomaCtl.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>

CURL *curl = NULL;
static struct curl_slist *global_resolve_list = NULL;	/* forced resolver */
static struct curl_slist *global_headers = NULL;				/* Headers */

	/* Response handling */
void freeResponse(struct ResponseBuffer *buff){
	if(buff->memory){
		free(buff->memory);
		buff->memory = NULL;
		buff->size = 0;
	}
}

static size_t WriteCallback(void *contents, size_t size, size_t nmemb, void *userp){
	size_t realsize = size * nmemb;
	struct ResponseBuffer *mem = (struct ResponseBuffer *)userp;
	char *p;

	if(!mem->memory)
		p = malloc(realsize + 1);
	else
		p = realloc(mem->memory, mem->size + realsize + 1);

	if(!p){
		fputs("*E* Out of memory\n", stderr);
		return 0;	/* Signal error to libcurl */
	}

	mem->memory = p;
	memcpy(&(mem->memory[mem->size]), contents, realsize);	/* Append new data */
	mem->size += realsize;
	mem->memory[mem->size] = 0;

	return realsize;
}

	/* API handling */
void curl_cleanup(void){
		/* internally protected against NULL pointer */
	curl_easy_cleanup(curl);	
	curl_slist_free_all(global_resolve_list);
	curl_slist_free_all(global_headers);
	curl_global_cleanup();
}

void buildURL(void){
	if(!tahoma || !ip || !port || !token)	/* Some information are missing */
		return;

		/* Build target base URL */
	if(url){
		free(url);
		url = NULL;
	}

	url_len = strlen("https://:/enduser-mobile-web/1/enduserAPI/");
	url_len += strlen(ip);
	url_len += 5; /* port: 65535 */

	url = malloc(url_len + 1);
	assert(url);

	sprintf(url, "https://%s:%u/enduser-mobile-web/1/enduserAPI/", ip, port);
	url_len = strlen(url);	/* Because the port length is unknown */

	if(debug)
		printf("*D* url: '%s'\n", url);

		/* Build DNS overwriting */
	if(global_resolve_list){
		curl_slist_free_all(global_resolve_list);
		global_resolve_list = NULL;
	}

	char resolve_entry[strlen(tahoma) + strlen(ip) + 3]; /* host:port:ip */
	sprintf(resolve_entry, "%s:%u:%s", tahoma, port, ip);
	if(debug)
		printf("*D* Resolution: '%s'\n", resolve_entry);

	if(!(global_resolve_list = curl_slist_append(NULL, resolve_entry))){
		fputs("*E* Failed to force DNS resolution", stderr);
		return;
	}

	curl_easy_setopt(curl, CURLOPT_RESOLVE, global_resolve_list);

		/* Authorization header string */
	if(global_headers){
		curl_slist_free_all(global_headers);
		global_headers = NULL;
	}
	
	char auth_header[strlen("Authorization: Bearer ") + strlen(token) + 1];
	strcpy(auth_header, "Authorization: Bearer ");
	strcat(auth_header, token);

	if(debug)
		printf("*D* Auth header : '%s'\n", auth_header);

	if(!(global_headers = curl_slist_append(NULL, auth_header))){
		fputs("*E* Failed to set header", stderr);
		return;
	}
	
	char host_header[strlen("Host: ") + strlen(tahoma) + 5 + 2]; /* Host: host:port + null */
	sprintf(host_header, "Host: %s:%u", tahoma, port);

	if(debug)
		printf("*D* Host header : '%s'\n", host_header);
	
	global_headers = curl_slist_append(global_headers, host_header);
	global_headers = curl_slist_append(global_headers, "Content-Type: application/json");
	if(debug)
		for(struct curl_slist *c = global_headers; c; c = c->next)
			printf("*D* Header -> '%s'\n", c->data);

	int res = curl_easy_setopt(curl, CURLOPT_HTTPHEADER, global_headers);
	if(res != CURLE_OK)
		fprintf(stderr, "*E* curl_easy_setopt(%p) : %s\n", curl, curl_easy_strerror(res));
}

void callAPI(const char *api, struct ResponseBuffer *buff){
	if(!tahoma || !ip || !port || !token){
		fputs("*E* missing connection information to run the request.\n", stderr);
		return;
	}
	
	char full_url[url_len + strlen(api) + 1];
	strcpy(full_url, url);
	strcpy(full_url + url_len, api);

	freeResponse(buff);
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)buff);

	if(debug)
		printf("*D* calling '%s'\n", full_url);
	curl_easy_setopt(curl, CURLOPT_URL, full_url);

	if(debug)
		curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);

	spent(false);
	int res = curl_easy_perform(curl);
	spent(true);

	if(res != CURLE_OK)
		fprintf(stderr, "*E* Calling error : %s\n", curl_easy_strerror(res));
	else if(verbose || debug){
		long http_code = 0;
		curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
		printf("*I* HTTP return code : %ld\n", http_code);
	}
}
