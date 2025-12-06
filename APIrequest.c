/* Call Tahoma's API
 */

#include "TaHomaCtl.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>

CURL *curl = NULL;

void curl_cleanup(void){
	curl_easy_cleanup(curl);	/* internally protected against NULL pointer */
	curl_global_cleanup();
}

void buildURL(void){
	if(!tahoma || !ip || !port || !token)	/* Some information are missing */
		return;

	if(url){
		free(url);
		url = NULL;
	}

	url_len = strlen("https://:/enduser-mobile-web/1/enduserAPI/");
	url_len += strlen(tahoma);
	url_len += 5; /* port: 65535 */

	url = malloc(url_len + 1);
	assert(url);

	sprintf(url, "https://%s:%u/enduser-mobile-web/1/enduserAPI/", tahoma, port);
	if(debug)
		printf("*D* url: '%s'\n", url);
}


