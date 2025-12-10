/* Process APIs
 */

#include "TaHomaCtl.h"

#include <json-c/json.h>

void func_Tgw(const char *){
	struct ResponseBuffer buff = {NULL};

	callAPI("setup/gateways", &buff);

	if(debug)
		printf("*D* Resp: '%s'\n", buff.memory ? buff.memory : "NULL data");

	freeResponse(&buff);
}


