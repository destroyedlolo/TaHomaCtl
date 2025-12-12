/* Process APIs
 */

#include "TaHomaCtl.h"

#include <json-c/json.h>

const char *getObjString(struct json_object *parent, const char *path[]){
	struct json_object *obj = parent;

	for(int i=0; path[i]; ++i){
		if(debug)
			printf("*D* %d: '%s'\n", i, path[i]);

		if(!obj){
			if(debug)
				fprintf(stderr, "*E* Broken path at %dth\n", i);
			return NULL;
		}
		obj = json_object_object_get(obj, path[i]);
	}

	if(json_object_is_type(obj, json_type_string))
		return json_object_get_string(obj);
	else if(debug)
		fputs("*E* Not a string\n", stderr);

	return NULL;
}

static const char *affString(const char *v){
	if(v)
		return v;
	else
		return "Not found";
}

void func_Tgw(const char *){
	struct ResponseBuffer buff = {NULL};

	callAPI("setup/gateways", &buff);

	if(debug)
		printf("*D* Resp: '%s'\n", buff.memory ? buff.memory : "NULL data");

		/* Display result */
	if(buff.memory){
		struct json_object *parsed_json = json_tokener_parse(buff.memory);

		if(json_object_is_type(parsed_json, json_type_array)){	/* 1st object is an array */
			struct json_object *first_object = json_object_array_get_idx(parsed_json, 0);
			if(first_object)
				printf("gatewayId : %s\n", affString(getObjString(first_object, (const char *[]){"gatewayId", NULL})) );
			else
				fputs("*E* Empty or unexpected object returned", stderr);
		} else
			fputs("*E* Returned object is not an array", stderr);

		json_object_put(parsed_json);
	}
	
	freeResponse(&buff);
}

