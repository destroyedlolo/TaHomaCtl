/* Process APIs
 */

#include "TaHomaCtl.h"

#include <json-c/json.h>

void func_Tgw(const char *){
	struct ResponseBuffer buff = {NULL};

	callAPI("setup/gateways", &buff);

	if(debug)
		printf("*D* Resp: '%s'\n", buff.memory ? buff.memory : "NULL data");

		/* Display result */
	if(buff.memory){
		struct json_object *parsed_json = json_tokener_parse(buff.memory);
		struct json_object *obj;

		if(json_object_is_type(parsed_json, json_type_array)){	/* 1st object is an array */
			struct json_object *first_object = json_object_array_get_idx(parsed_json, 0);
			if(first_object){ 
				{
					const char *path_id[] = {"gatewayId", NULL};
					if((obj = json_object_get_path(first_object, path_id)) && json_object_is_type(obj, json_type_string)){
						printf("gatewayId : %s\n", json_object_get_string(obj));
					} else
						fputs("*E* gatewayId not found ", stderr);

				}
			} else
				fputs("*E* Empty or unexpected object returned", stderr);
		} else
			fputs("*E* Returned object is not an array", stderr);

		json_object_put(parsed_json);
	}
	
	freeResponse(&buff);
}

#if 0

struct json_object* get_json_path(struct json_object* jparent, const char *path[]) {
    struct json_object *current_obj = jparent;
    
    // Itérer sur les éléments du chemin jusqu'à rencontrer NULL
    for (int i = 0; path[i] != NULL; ++i) {
        if (!current_obj) {
            // Un objet précédent était NULL, chemin brisé
            return NULL;
        }

        // Tenter d'accéder au sous-objet
        current_obj = json_object_object_get(current_obj, path[i]);
    }

    return current_obj; // Retourne l'objet final ou NULL
}

void display_json_fields(void) {
    // ... (Vérifications initiales et parsing) ...

    struct json_object *parsed_json = json_tokener_parse(response_data.memory);

    // ... (Vérification que parsed_json est un tableau et non vide) ...
    struct json_object *first_object = json_object_array_get_idx(parsed_json, 0);

    // ----------------------------------------------------
    // NOUVEAU : Extraction de gatewayId via le chemin
    // ----------------------------------------------------
    
    struct json_object *gateway_id_obj = NULL;
    const char *path_id[] = {"gatewayId", NULL}; // Le chemin est juste la clé

    gateway_id_obj = json_object_object_get_path(first_object, path_id);

    if (gateway_id_obj != NULL) {
        if (json_object_is_type(gateway_id_obj, json_type_string)) {
            const char *gateway_id_value = json_object_get_string(gateway_id_obj);
            
            printf("Extracted gatewayId (via path): %s\n", gateway_id_value);
        } else {
            fprintf(stderr, "*E* 'gatewayId' found but is not a string.\n");
        }
    } else {
        fprintf(stderr, "*E* 'gatewayId' field not found.\n");
    }

    // ----------------------------------------------------
    // Extraction de connectivity/status (pour rappel)
    // ----------------------------------------------------
    
    struct json_object *status_obj = NULL;
    const char *path_status[] = {"connectivity", "status", NULL};

    status_obj = json_object_object_get_path(first_object, path_status);

    if (status_obj != NULL && json_object_is_type(status_obj, json_type_string)) {
        printf("Extracted status (via path): %s\n", json_object_get_string(status_obj));
    }
    
    // ... (Libération de l'objet racine) ...
    json_object_put(parsed_json);
}
#endif
