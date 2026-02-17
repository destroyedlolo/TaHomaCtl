/* Process APIs
 */

#include "TaHomaCtl.h"

#include <assert.h>
#include <string.h>
#include <json-c/json.h>

#define OBJPATH(...) (const char*[]){ __VA_ARGS__ }

static struct json_object *getObj(struct json_object *parent, const char *path[]){
	struct json_object *obj = parent;

	for(int i=0; path[i]; ++i){
#if 0	/* Remove uneeded noise */
		if(debug)
			printf("*D* %d: '%s'\n", i, path[i]);
#endif

		if(!obj){
			if(debug)
				fprintf(stderr, "*E* Broken path at %dth\n", i);
			return NULL;
		}
		obj = json_object_object_get(obj, path[i]);
	}

	return obj;
}

static const char *getObjString(struct json_object *parent, const char *path[]){
	struct json_object *obj = getObj(parent, path);
	if(!obj)
		return NULL;

	if(json_object_is_type(obj, json_type_string))
		return json_object_get_string(obj);
	else if(debug)
		fputs("*E* Not a string\n", stderr);

	return NULL;
}

static int getObjInt(struct json_object *parent, const char *path[]){
	struct json_object *obj = getObj(parent, path);
	if(!obj)
		return 0;

	if(json_object_is_type(obj, json_type_int))
		return json_object_get_int(obj);
	else if(debug)
		fputs("*E* Not an integer\n", stderr);

	return 0;
}

static double getObjNumber(struct json_object *parent, const char *path[]){
	struct json_object *obj = getObj(parent, path);
	if(!obj)
		return 0;

	if(json_object_is_type(obj, json_type_double))
		return json_object_get_double(obj);
	else if(json_object_is_type(obj, json_type_int))	/* It seems some object are only integers */
		return json_object_get_int(obj);
	else if(debug)
		fputs("*E* Not a double or integer\n", stderr);

	return 0;
}

static bool getObjBool(struct json_object *parent, const char *path[]){
	struct json_object *obj = getObj(parent, path);
	if(!obj)
		return false;

	if(json_object_is_type(obj, json_type_boolean))
		return json_object_get_boolean(obj);
	else if(debug)
		fputs("*E* Not a boolean\n", stderr);

	return false;
}

static const char *affString(const char *v){
	if(v)
		return v;
	else
		return "Not found";
}

static void gentab(unsigned int n){
	do 
		putchar('\t');
	while(--n);
}

void printObject(struct json_object *obj, unsigned int nbre_tab){
	if(!obj)
		return;

	switch(json_object_get_type(obj)){
	case json_type_object: {
			json_object_object_foreach(obj, key, val){
				gentab(nbre_tab);
				printf("%s :\n", key);
				printObject(val, nbre_tab+1 );
			}
		}
		break;
	case json_type_array: {
			int n = json_object_array_length(obj);
			gentab(nbre_tab); puts("[");
			for(int i = 0; i < n; ++i){
				struct json_object *sub = json_object_array_get_idx(obj, i);
				printObject(sub, nbre_tab+1);
			}
			gentab(nbre_tab); puts("]");
		}
		break;
	case json_type_string:
		gentab(nbre_tab);
		printf("\"%s\"\n", json_object_get_string(obj));
		break;
	case json_type_int:
		gentab(nbre_tab);
		printf("%lld\n", (long long)json_object_get_int64(obj));
		break;
	case json_type_boolean:
		gentab(nbre_tab);
		printf("%s\n", json_object_get_boolean(obj) ? "true":"false");
		break;
	case json_type_double:
		gentab(nbre_tab);
		printf("%f\n", json_object_get_double(obj));
		break;
	case json_type_null:
		gentab(nbre_tab);
		puts("NULL");
		break;
	}
}

	/*
	 * Devices
	 */

struct Device *devices_list = NULL;

static void freeDevice(struct Device *dev){
	free((void *)dev->label);
	free((void *)dev->url);
}

static void freeDeviceList(void){
	for(struct Device *dev = devices_list; dev; ){
		freeDevice(dev);

		struct Device *ans = dev;
		dev = dev->next;
		free(ans);
	}

	devices_list = NULL;
}

static void addDevice(struct json_object *obj){
	const char *t; 

#if 0
	t = getObjString(obj, OBJPATH( "definition", "type", NULL ));
	if(!t || !strcmp(t, "PROTOCOL_GATEWAY"))
		return;
#endif

	struct Device *dev = malloc(sizeof(struct Device));
	assert(dev);

		/* feed with the label */
	t = getObjString(obj, OBJPATH( "label", NULL ));
	assert(t);

	assert( (dev->label = strdup(t)) );
	for(char *c = (char *)dev->label; *c; ++c)
		if(*c == ' ')
			*c = '_';

		/* and the URL */
	assert( (t = getObjString(obj, OBJPATH( "deviceURL", NULL ) )) );
	assert( (dev->url = strdup(t)) );

		/* store known commands */
	dev->commands = NULL;

	struct json_object *lstc = getObj(obj, OBJPATH( "definition", "commands", NULL ));
	if(!lstc){
		fprintf(stderr, "*E* [%s] commands field not found.\n", dev->label);
		freeDevice(dev);
		free(dev);
		return;
	}

	if(!json_object_is_type(lstc, json_type_array)){
		fprintf(stderr, "*E* [%s] commands field not an array.\n", dev->label);
		freeDevice(dev);
		free(dev);
		return;
	}

	size_t nbr = json_object_array_length(lstc);
	if(debug)
		printf("*I* %ld command(s)\n", nbr);

	for(size_t idx=0; idx < nbr; ++idx){
		struct json_object *cmd = json_object_array_get_idx(lstc, idx);
		if(!cmd){
			fprintf(stderr, "*E* [%s / %ld] Command not found.\n", dev->label, idx);
			freeDevice(dev);
			free(dev);
			return;
		}

		assert( (t = getObjString(cmd, OBJPATH( "commandName", NULL ) )) );

		struct Command *ncmd = malloc(sizeof(struct Command));
		assert(ncmd);

		assert( (ncmd->command = strdup(t)) );
		ncmd->nparams = getObjInt(cmd, OBJPATH( "nparams", NULL ) );

		ncmd->next = dev->commands;
		dev->commands = ncmd;
	}

		/* store known states */
	dev->states = NULL;
	lstc = getObj(obj, OBJPATH( "definition", "states", NULL ));
	if(!lstc){
		fprintf(stderr, "*E* [%s] states field not found.\n", dev->label);
		for(struct Command *cmd = dev->commands; cmd; ){
			free((void *)cmd->command);
			struct Command *old = cmd;
			cmd = cmd->next;
			free(old);
		}
		freeDevice(dev);
		free(dev);
		return;
	}

	if(!json_object_is_type(lstc, json_type_array)){
		fprintf(stderr, "*E* [%s] states field not an array.\n", dev->label);
		for(struct Command *cmd = dev->commands; cmd; ){
			free((void *)cmd->command);
			struct Command *old = cmd;
			cmd = cmd->next;
			free(old);
		}
		freeDevice(dev);
		free(dev);
		return;
	}

	nbr = json_object_array_length(lstc);
	if(debug)
		printf("*I* %ld state(s)\n", nbr);

	for(size_t idx=0; idx < nbr; ++idx){
		struct json_object *state = json_object_array_get_idx(lstc, idx);
		if(!state){
			fprintf(stderr, "*E* [%s / %ld] State not found.\n", dev->label, idx);
			for(struct Command *cmd = dev->commands; cmd; ){
				free((void *)cmd->command);
				struct Command *old = cmd;
				cmd = cmd->next;
				free(old);
			}
			freeDevice(dev);
			free(dev);
			return;
		}

		assert( (t = getObjString(state, OBJPATH( "name", NULL ) )) );

		struct State *nstate = malloc(sizeof(struct State));
		assert(nstate);

		assert( (nstate->state = strdup(t)) );

		nstate->next = dev->states;
		dev->states = nstate;
	}

		/* Add the new device in the list */
	dev->next = devices_list;
	devices_list = dev;
}

struct Device *findDevice(struct substring *name){
	for(struct Device *r = devices_list; r; r = r->next){
		if(!substringcmp(name, r->label))
			return r;
	}

	return NULL;
}

struct Device *findDeviceByURL(const char *url){
	for(struct Device *r = devices_list; r; r = r->next){
		if(!strcmp(url, r->url))
			return r;
	}

	return NULL;
}

	/*
	 * User commands
	 */

void func_Tgw(const char *arg){
	if(arg){
		fputs("*E* Gateway doesn't expect an argument.\n", stderr);
		return;
	}

	struct ResponseBuffer buff = {NULL};

	callAPI("setup/gateways", NULL, &buff);
	if(debug)
		printf("*D* Resp: '%s'\n", buff.memory ? buff.memory : "NULL data");

		/* Display result */
	if(buff.memory){
		struct json_object *parsed_json = json_tokener_parse(buff.memory);

		if(json_object_is_type(parsed_json, json_type_array)){	/* 1st object is an array */
			struct json_object *first_object = json_object_array_get_idx(parsed_json, 0);
			if(first_object){
				printf("gatewayId : %s\n", affString(getObjString(first_object, OBJPATH( "gatewayId", NULL ) )));
				printf("Connected : %s\n", affString(getObjString(first_object, OBJPATH( "connectivity", "status", NULL ) )));
				printf("protocolVersion : %s\n", affString(getObjString(first_object, OBJPATH( "connectivity", "protocolVersion", NULL ) )));

			} else
				fputs("*E* Empty or unexpected object returned", stderr);
		} else
			fputs("*E* Returned object is not an array as expected", stderr);

		json_object_put(parsed_json);
	} else if(verbose)
		fputs("*W* Empty response", stderr);
	
	freeResponse(&buff);
}

static void printDeviceInfo(struct json_object *obj){
	printf("*I* %s [%s]\n", 
		affString(getObjString(obj, OBJPATH( "label", NULL ) )),
		affString(getObjString(obj, OBJPATH( "controllableName", NULL ) ))
	);

	printf("\tURL : %s\n", 
		affString(getObjString(obj, OBJPATH( "deviceURL", NULL ) ))
	);

	printf("\tType : %d, subsystemId : %d\n", 
		getObjInt(obj, OBJPATH( "type", NULL ) ),
		getObjInt(obj, OBJPATH( "subsystemId", NULL ) )
	);

	printf("\t%ssynced, %senabled, %savailable\n",
		getObjBool(obj, OBJPATH( "synced", NULL ) ) ? "":"Not ",
		getObjBool(obj, OBJPATH( "enabled", NULL ) ) ? "":"Not ",
		getObjBool(obj, OBJPATH( "available", NULL ) ) ? "":"Not "
	);

	printf("\t\tType: %s\n",
		affString(getObjString(obj, OBJPATH( "definition", "type", NULL ) ))
	);
}

void func_scandevs(const char *arg){
	if(arg){
		fputs("*E* Devices doesn't expect an argument.\n", stderr);
		return;
	}

	struct ResponseBuffer buff = {NULL};
	callAPI("setup/devices", NULL, &buff);
	if(debug)
		printf("*D* Resp: '%s'\n", buff.memory ? buff.memory : "NULL data");

		/* Process result */
	if(buff.memory){
		struct json_object *res= json_tokener_parse(buff.memory);

		if(json_object_is_type(res, json_type_array)){	/* 1st object is an array */
			freeDeviceList();	/* Remove old list */

			size_t nbr = json_object_array_length(res);
			if(debug || verbose)
				printf("*I* %ld devices\n", nbr);

			for(size_t idx=0; idx < nbr; ++idx){
				struct json_object *obj = json_object_array_get_idx(res, idx);

				if(obj){
					if(debug || verbose > 1)
						printDeviceInfo(obj);
					addDevice(obj);
				} else
					fprintf(stderr, "*E* Can't get %ld\n", idx);
			}
		} else
			fputs("*E* Returned object is not an array", stderr);

		json_object_put(res);
	} else if(verbose)
		fputs("*W* Empty response", stderr);

	freeResponse(&buff);
}

void func_NStates(const char *arg){
	if(!arg){
		fputs("*E* States is expecting a device's name.\n", stderr);
		return;
	}

	struct substring devname, name;
	const char *next;

		/* Extract the device name */
	extractTokenSub(&devname, arg, &next);

		/* Extract the state name ... if any */
	name.s = NULL;	/* Yet empty */
	if(next && *next){	/* We got an name */
		const char *unused;
		extractTokenSub(&name, next, &unused);
	}

	struct Device *dev = findDevice(&devname);
	if(!dev){
		fputs("*E* Device not found.\n", stderr);
		return;
	}

	char *enc = curl_easy_escape(curl, dev->url, 0);
	assert(enc);
	char url[ strlen("setup/devices//states") + strlen(enc) +1];
	sprintf(url, "setup/devices/%s/states", enc);
	curl_free(enc);

	if(debug)
		printf("*D* Url: '%s'\n", url);

	struct ResponseBuffer buff = {NULL};
	callAPI(url, NULL, &buff);
	if(debug)
		printf("*D* Resp: '%s'\n", buff.memory ? buff.memory : "NULL data");

		/* Process result */
	if(buff.memory){
		struct json_object *res= json_tokener_parse(buff.memory);

		if(json_object_is_type(res, json_type_array)){	/* 1st object is an array */
			size_t nbr = json_object_array_length(res);
			if(debug)
				printf("*I* %ld states\n", nbr);
			for(size_t idx=0; idx < nbr; ++idx){
				struct json_object *obj = json_object_array_get_idx(res, idx);

				const char *n = getObjString(obj, OBJPATH( "name", NULL ));
				if(!n || (name.s && substringcmp(&name, n)))	/* Looking for a specific state */
					continue;

				if(!name.s)
					printf("\t%s : ", affString(n));
				int type = getObjInt(obj, OBJPATH( "type", NULL ));

				switch(type){
				case 1:	/* Number */
					printf("%lf\n", getObjNumber(obj, OBJPATH( "value", NULL ) ));
					break;
				case 3:	/* String */
					printf("\"%s\"\n", affString(getObjString(obj, OBJPATH( "value", NULL ) )));
					break;
				case 6:	/* Boolean */
					printf("%s\n", getObjBool(obj, OBJPATH( "value", NULL ) ) ? "true":"false");
				case 10:
					puts("[Array]");
					break;
				case 11:
					puts("{Object}");
					break;
				default:
					printf("Unknown type (%d)\n", type);
				}
			}
		} else
			fputs("*E* Returned object is not an array", stderr);

		json_object_put(res);
	} else if(verbose)
		fputs("*W* Empty response", stderr);

	freeResponse(&buff);
}

void func_NCommand(const char *arg){
	if(!arg){
		fputs("*E* Command is expecting at last a device's name.\n", stderr);
		return;
	}

	struct substring devname, command;
	const char *next;

		/* Extract the device name */
	extractTokenSub(&devname, arg, &next);

	struct Device *dev = findDevice(&devname);
	if(!dev){
		fputs("*E* Device not found.\n", stderr);
		return;
	}

		/* Extract the command name */
	if(!next || !*next){
		fputs("*E* Missing command's name.\n", stderr);
		return;
	}
	extractTokenSub(&command, next, &next);

		/* build command line */
	char *cmd = dynstringAdd(NULL, 
"{\"label\":\"TaHomaCtl\","
"\"actions\":["
"{\"commands\":["
"{\"name\":\""
	);

	cmd = dynstringAddSub(cmd, &command);

	cmd = dynstringAdd(cmd,
"\","
"\"parameters\":["
	);
	
	/* For the moment :
	 * - only string argument are considered
	 * - there is no checking about the number of arguments vs
	 * command definition
	 */
	bool first = true;
	while(next && *next){
		struct substring arg;
		extractTokenSub(&arg, next, &next);
		
		if(!first)
			cmd = dynstringAdd(cmd, ",");

		cmd = dynstringAdd(cmd, "\"");
		cmd = dynstringAddSub(cmd, &arg);
		cmd = dynstringAdd(cmd, "\"");

		first = false;
	}
	cmd = dynstringAdd(cmd, "]}],\"deviceURL\":\"");
	cmd = dynstringAdd(cmd, dev->url);
	cmd = dynstringAdd(cmd, "\"}]}");

	struct ResponseBuffer buff = {NULL};
	callAPI("exec/apply", cmd, &buff);

	long http_code = 0;
	curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
	if(http_code == 200 && verbose)
		puts("*I* Successful");
	if(debug || verbose > 1)
		printf("*I* Resp: '%s'\n", buff.memory ? buff.memory : "NULL data");

	freeResponse(&buff);
	free(cmd);
}

void func_NCurrent(const char *arg){
	if(arg){
		fputs("*E* Devices doesn't expect an argument.\n", stderr);
		return;
	}

	struct ResponseBuffer buff = {NULL};
	callAPI("/exec/current", NULL, &buff);
	if(debug)
		printf("*D* Resp: '%s'\n", buff.memory ? buff.memory : "NULL data");

			/* Display result */
	if(buff.memory){
		struct json_object *parsed_json = json_tokener_parse(buff.memory);

		if(json_object_is_type(parsed_json, json_type_array)){	/* 1st object is an array */
			int nbre = json_object_array_length(parsed_json);
			if(verbose)
				printf("*I* %d Execution(s)\n", nbre);

			for(int i=0; i<nbre; ++i){
				struct json_object *item = json_object_array_get_idx(parsed_json, i);
				printf("*I* id: %s\n", affString(getObjString(item, OBJPATH( "id", NULL ) )));
				printf("\tdescription : \"%s\"\n", affString(getObjString(item, OBJPATH( "description", NULL) )));
				printf("\towner: %s\tstate: %s\ttype: %s\n", 
					affString(getObjString(item, OBJPATH( "owner", NULL) )),
					affString(getObjString(item, OBJPATH( "state", NULL) )),
					affString(getObjString(item, OBJPATH( "executionSubType", NULL) ))
				);
				
				struct json_object *startobj = getObj(item, OBJPATH( "startTime", NULL));
				time_t sec = startobj ? json_object_get_int64(startobj)/1000 : 0;
				printf("\tStarted on %s", ctime(&sec));

				if(( startobj = getObj(item, OBJPATH( "actionGroup", "actions",NULL)) )){
					if(json_object_is_type(startobj, json_type_array)){
						int n = json_object_array_length(startobj);
						for(int j=0; j<n; ++j){
							struct json_object *action = json_object_array_get_idx(startobj, j);
							const char *url = getObjString(action, OBJPATH( "deviceURL", NULL));
							printf("\t\tDevice : ");
							struct Device *dev = findDeviceByURL(url);
							if(dev)
								printf("%s ", dev->label);
							printf("'%s'\n", affString(url));

							struct json_object *commands = getObj(action, OBJPATH( "commands", NULL));
							printObject(commands, 2);
						}
					}
				}
			}
		} else
			fputs("*E* Returned object is not an array as expected", stderr);

		json_object_put(parsed_json);
	}

	freeResponse(&buff);
}
