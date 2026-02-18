/* TaHomaCtl
 *
 * CLI tool to control your TaHoma
 *
 * History:
 * 	8/11/2025 - LF - First version
 */

#include "TaHomaCtl.h"

#include <unistd.h>	/* getopt() */
#include <stdio.h>
#include <stdlib.h>
#include <pwd.h>
#include <readline/readline.h>
#include <readline/history.h>

#define VERSION "0.13"

	/* **
	 * Configuration
	 * **/

char *tahoma = NULL;
char *ip = NULL;
uint16_t port = 0;
char *token = NULL;
bool unsafe = false;

char *url = NULL;
size_t url_len;
long timeout = 0;
unsigned int verbose = 0;
bool trace = false;
bool debug = false;

static const char *ascript = NULL;	/* User script to launch (from launch parameters) */
static bool nostartup = false;	/* Do not source .tahomactl */

struct Device *dev;	/* Device currently considered */

static const char *affval(const char *v){
	if(v)
		return v;
	else
		return "Not set";
}

	/* ***
	 * Commands interpreter
	 * ***/

static void execscript(const char *, bool);
static void func_qmark(const char *);

static void func_script(const char *arg){
	if(arg)
		execscript(arg, false);
	else
		fputs("*E* Expecting a filename.\n", stderr);
}

static void func_token(const char *arg){
	if(arg){
		FreeAndSet(&token, arg);
		buildURL();
	} else
		printf("*I* Token : %s\n", affval(token));
}

static void func_THost(const char *arg){
	if(arg){
		FreeAndSet(&tahoma, arg);
		buildURL();
	} else
		printf("*I* Tahoma's host : %s\n", affval(tahoma));
}

static void func_TAddr(const char *arg){
	if(arg){
		FreeAndSet(&ip, arg);
		buildURL();
	} else
		printf("*I* Tahoma's IP address : %s\n", affval(ip));
}

static void func_TPort(const char *arg){
	if(arg){
		port = (uint16_t)atoi(arg);
		buildURL();
	} else
		printf("*I* Tahoma's port : %u\n", port);
}

static void func_save(const char *arg){
	if(!arg){
		fputs("*E* file name expected\n", stderr);
		return;
	}

	FILE *f = fopen(arg, "w");
	if(!f){
		perror(arg);
		return;
	}

	if(tahoma)
		fprintf(f, "TaHoma_host %s\n", tahoma);

	if(ip)
		fprintf(f, "TaHoma_address %s\n", ip);

	if(port)
		fprintf(f, "TaHoma_port %u\n", port);

	if(token)
		fprintf(f, "token %s\n", token);

	fclose(f);
}

static void func_status(const char *){
	printf("*I* Connection :\n"
		"\tTahoma's host : %s\n"
		"\tTahoma's IP : %s\n"
		"\tTahoma's port : %u\n"
		"\tToken : %s\n"
		"\tSSL chaine : %s\n",
		affval(tahoma),
		affval(ip),
		port,
		token ? "set": "unset",
		unsafe ? "not checked (unsafe)" : "Enforced"
	);
	if(timeout)
		printf("\tTimeout : %lds\n", timeout);

	unsigned int nbre = 0;
	for(struct Device *dev = devices_list; dev; dev = dev->next)
		++nbre;

	printf("*I* %u Stored device%c\n", nbre, nbre > 1 ? 's':' ');
}

static void device_info(struct Device *dev){
	puts("\tCommands");
	for(struct Command *cmd = dev->commands; cmd; cmd = cmd->next)
		printf("\t\t%s (%d %s)\n", cmd->command, cmd->nparams, cmd->nparams > 1 ? "args": "arg");
	puts("\tStates");
	for(struct State *state = dev->states; state; state = state->next)
		printf("\t\t%s\n", state->state);
}

static void internal_Devs(const char *arg, bool url){
	if(!arg){	/* List all devices */
		for(struct Device *dev = devices_list; dev; dev = dev->next){
			printf("%s : %s\n", dev->label, dev->url);
			if(verbose > 1)
				device_info(dev);
		}
	} else {	/* Info of a specific devices */
		struct substring devname;
		const char *unused;

		extractTokenSub(&devname, arg, &unused);
		dev = url ?  findDeviceByURLSS(&devname) : findDevice(&devname);
		if(dev){
			printf("%s : %s\n", dev->label, dev->url);
			device_info(dev);
		} else
			printf("*W* Device \"%.*s\" not found\n", (int)devname.len, devname.s);
	}
}

static void func_Devs(const char *arg){
	internal_Devs(arg, true);
}

static void func_NDevs(const char *arg){
	internal_Devs(arg, false);
}

static void func_history(const char *arg){
	if(arg)
		fputs("*E* Argument is ignored\n", stderr);

	HISTORY_STATE *my_history_state = history_get_history_state();

	if(!my_history_state || !my_history_state->entries){
		puts("*I* The history is empty");
		return;
	}

	for(int i = 0; i < my_history_state->length; ++i)
		puts(my_history_state->entries[i]->line);

	history_set_history_state(my_history_state);
}

static void func_verbose(const char *arg){
	if(arg){
		if(!strcmp(arg, "on"))
			verbose = 1;
		else if(!strcmp(arg, "more"))
			verbose = 2;
		else if(!strcmp(arg, "off"))
			verbose = 0;
		else
			fputs("*E* verbose accepts only 'on', 'off' and 'more'\n", stderr);
	} else
		puts(verbose ? "I'm verbose" : "I'm quiet");
}

static void func_trace(const char *arg){
	if(arg){
		if(!strcmp(arg, "on"))
			trace = true;
		else if(!strcmp(arg, "off"))
			trace = false;
		else
			fputs("*E* trace accepts only 'on' and 'off'\n", stderr);
	} else
		puts(trace ? "Traces enabled" : "Traces disabled");
}

static void func_debug(const char *arg){
	if(arg){
		if(!strcmp(arg, "on"))
			debug = true;
		else if(!strcmp(arg, "off"))
			debug = false;
		else
			fputs("*E* debug accepts only 'on' and 'off'\n", stderr);
	} else
		puts(debug ? "Debug enabled" : "Debug disabled");
}

static void func_timeout(const char *arg){
	if(arg){
		timeout = atol(arg);

		if(debug || verbose)
			printf("*I* Timeout : %lds\n", timeout);
	} else
		fputs("timeout is execting the number of seconds to wait.\n", stderr);
}

static void func_quit(const char *){
	exit(EXIT_SUCCESS);
}

static char *state_generator(const char *, int);
static char *action_generator(const char *, int);

struct _commands {
	const char *name;		/* Command's name */
	void(*func)(const char *);	/* executor */
	const char *help;		/* Help message */
		/* Argument managing
		 * Notez-bien : 1st and 2nd argument autocompletion are enabled only for
		 *	commands expecting a device as first element.
		 *	The second one is linked with the device's content.
		 */
	enum _arg_t {
		ARG_NO = 0,	/* No argument */
		ARG_NAME,	/* Device name */
		ARG_URL		/* URL */
	} devarg;		/* 1st argument is a device (enable autocompletion) */
	char *(*autofunc)(const char *, int);	/* Function to be used as 2nd argument completion */
} Commands[] = {
	{ NULL, NULL, "TaHoma's Configuration", ARG_NO, NULL},
	{ "TaHoma_host", func_THost, "[name] set or display TaHoma's host", ARG_NO, NULL},
	{ "TaHoma_address", func_TAddr, "[ip] set or display TaHoma's ip address", ARG_NO, NULL},
	{ "TaHoma_port", func_TPort, "[num] set or display TaHoma's port number", ARG_NO, NULL},
	{ "TaHoma_token", func_token, "[value] indicate application token", ARG_NO, NULL},
	{ "timeout", func_timeout, "[value] specify API call timeout (seconds)", ARG_NO, NULL},
	{ "status", func_status, "Display current connection informations", ARG_NO, NULL},

	{ NULL, NULL, "Scripting", ARG_NO, NULL},
	{ "save_config", func_save, "<file> save current configuration to the given file", ARG_NO, NULL},
	{ "script", func_script, "<file> execute the file", ARG_NO, NULL},

	{ NULL, NULL, "Interacting with the TaHoma", ARG_NO, NULL},
	{ "scan_TaHoma", func_scan, "Look for Tahoma's ZeroConf advertising", ARG_NO, NULL},
	{ "scan_Devices", func_scandevs, "Query and store attached devices", ARG_NO, NULL},
	{ "Gateway", func_Tgw, "Query your gateway own configuration", ARG_NO, NULL},
	{ "Current", func_Current, "Get action group executions currently running and launched from the local API", ARG_NO, NULL },

	{ NULL, NULL, "Interacting by device's URL", ARG_NO, NULL},
	{ "Device", func_Devs, "[URL] display device \"URL\" information or the devices list", ARG_URL, NULL },
	{ "States", func_States, "<device URL> [state name] query the states of a device", ARG_URL, state_generator },
	{ "Command", func_Command, "<device URL> <command name> [argument] send a command to a device", ARG_NAME, action_generator },

	{ NULL, NULL, "Interacting by device's name", ARG_NO, NULL},
	{ "NDevice", func_NDevs, "[name] display device \"name\" information or the devices list", ARG_NAME, NULL },
	{ "NStates", func_NStates, "<device name> [state name] query the states of a device", ARG_NAME, state_generator },
	{ "NCommand", func_NCommand, "<device name> <command name> [argument] send a command to a device", ARG_NAME, action_generator },

	{ NULL, NULL, "Verbosity", ARG_NO, NULL},
	{ "verbose", func_verbose, "[on|off|more] Be verbose", ARG_NO, NULL},
	{ "trace", func_trace, "[on|off|] Trace every commands", ARG_NO, NULL},
	{ "debug", func_debug, "[on|off] enable debug messages", ARG_NO, NULL},

	{ NULL, NULL, "Miscs", ARG_NO, NULL},
	{ "#", NULL, "Comment, ignored line", ARG_NO, NULL},
	{ "?", func_qmark, "List available commands", ARG_NO, NULL},
	{ "history", func_history, "List command line history", ARG_NO, NULL},
	{ "Quit", func_quit, "See you", ARG_NO, NULL},
	{ NULL, NULL, NULL, ARG_NO, NULL}
};

static void func_qmark(const char *){
	puts("List of known commands\n"
		 "======================");

	for(struct _commands *c = Commands; c->help; ++c){
		if(c->name)
			printf("'%s' : %s\n", c->name, c->help);
		else {
			printf("\n%s\n", c->help);
			for(const char *p = c->help; *p; ++p)
				putchar('-');
			putchar('\n');
		}
	}
}

static struct _commands *findCommand(struct substring *cmd){
	for(struct _commands *c = Commands; c->help; ++c){
		if(c->name && !substringcmp(cmd, c->name))
			return c;
	}

	return NULL;
}

static void exec(struct substring *cmd, const char *arg){
	if(trace && *cmd->s != '#')
		printf("> %.*s\n", (int)cmd->len, cmd->s);

	struct _commands *c = findCommand(cmd);
	if(c){
		if(c->func)
			c->func(arg);
	} else
		printf("*E* Unknown command \"%.*s\" : type '?' for list of known directives\n", (int)cmd->len, cmd->s);
}

static void execline(char *l){
	struct substring cmd;
	const char *arg;

	if(extractTokenSub(&cmd, l, &arg))
		exec(&cmd, *arg ? arg:NULL );
	else	/* No argument */
		exec(&cmd, NULL);
}

static void execscript(const char *name, bool dontfail){
	FILE *f = fopen(name, "r");
	if(!f){
		if(dontfail)
			return;
		else {
			perror(name);
			exit(EXIT_FAILURE);
		}
	}

	char *l = NULL;
	size_t len = 0;
	while(getline(&l, &len, f) != -1){
		char *c = strchr(l, '\n');	// Remove leading CR
		if(c)
			*c = 0;

		if(*l)	// Ignore empty line
			execline(l);
	}

	free(l);
	fclose(f);
}

static char *command_generator(const char *text, int state){
    static int list_index, len;
    const char *name;

    if(!state){
        list_index = 0;
        len = strlen(text);
    }

    // Iterate through the command_table for names
	while(Commands[list_index].help){
        ++list_index;
    	if((name = Commands[list_index].name)){
	        if(!strncmp(name, text, len))
    	        return(strdup(name));
	    }
	}

    return((char *)NULL);
}

static char *dev_generator(const char *text, int state){
	static struct Device *dev;
	static int len;

	if(!state){
		dev = devices_list;
		len = strlen(text);
	}

	while(dev){
		struct Device *cur = dev;
		dev = dev->next;

		if(!strncmp(cur->label, text, len))
			return(strdup(cur->label));
	}

    return((char *)NULL);
}

static char *URL_generator(const char *text, int state){
	static struct Device *dev;
	static int len;

	if(!state){
		dev = devices_list;
		len = strlen(text);
	}

	while(dev){
		struct Device *cur = dev;
		dev = dev->next;

		if(!strncmp(cur->url, text, len))
			return(strdup(cur->url));
	}

    return((char *)NULL);
}

static char *state_generator(const char *text, int state){
	static struct State *st;
	static int len;

	if(!dev)
    	return((char *)NULL);

	if(!state){
		st = dev->states;
		len = strlen(text);
	}

	while(st){
		struct State *cur = st;
		st = st->next;

		if(!strncmp(cur->state, text, len))
			return(strdup(cur->state));
	}
	
    return((char *)NULL);
}

static char *action_generator(const char *text, int state){
	static struct Command *com;
	static int len;

	if(!dev)
    	return((char *)NULL);

	if(!state){
		com = dev->commands;
		len = strlen(text);
	}

	while(com){
		struct Command *cur = com;
		com = com->next;

		if(!strncmp(cur->command, text, len))
			return(strdup(cur->command));
	}
	
    return((char *)NULL);
}

char **command_completion(const char *text, int start, int end){
	rl_attempted_completion_over = 1;
	if(!start)	/* At command level */
        return rl_completion_matches(text, command_generator);

		/* Find out the command we're working on */
	struct substring cmd;
	const char *arg;
	extractTokenSub(&cmd, rl_line_buffer, &arg);
	
	struct _commands *c = findCommand(&cmd);
	if(c && c->devarg){
			/* Determine the argument number */
		int i=0;
		while(isblank(rl_line_buffer[i])){	/* Leading space */
			++i;
			if(i >= start){	/* Should never happen */
				puts("*B* Leading space bug !!!");
				return((char **)NULL);
			}
		}

		bool inspace = false;
		unsigned int argnum = 0;
		for(; i<start; ++i){
			if(isblank(rl_line_buffer[i])){
				if(!inspace)
					++argnum;
				inspace = true;
			} else
				inspace = false;
		}

		if(argnum == 1){		/* Looking for a device */
			if(c->devarg == ARG_NAME)
				return rl_completion_matches(text, dev_generator);
			else	/* ARG_URL */
				return rl_completion_matches(text, URL_generator);
		} else if(c->autofunc){	/* second argument */
			struct substring devname;
			const char *unused;

			extractTokenSub(&devname, arg, &unused);
			dev = (c->devarg == ARG_URL) ?  findDeviceByURLSS(&devname) : findDevice(&devname);
			return rl_completion_matches(text, c->autofunc);
		}
	}

    return((char **)NULL);
}

	/* ***
	 * Here we go
	 * ***/

int main(int ac, char **av){
	int opt;

	while( (opt = getopt(ac, av, ":+NhH:p:Uk:f:dvVt46")) != -1){
		switch(opt){
		case 'f':
			ascript = optarg;
			break;
		case 'N':
			nostartup = true;
			break;
		case '4':
			avahiIP = AVAHI_PROTO_INET;
			break;
		case '6':
			avahiIP = AVAHI_PROTO_INET6;
			break;
		case 'H':
			FreeAndSet(&tahoma, optarg);
			break;
		case 'p':
			port = (uint16_t)atoi(optarg);	// Quick and dirty but harmless
			break;
		case 'U':
			unsafe = true;
			break;
		case 'd':
			debug = true;
			break;
		case 't':
			trace = true;
			break;
		case 'v':
			verbose = 1;
			break;
		case 'V':
			verbose = 2;
			break;
		case '?':	/* Unknown option */
			fprintf(stderr, "unknown option: -%c\n", optopt);
		case 'h':
		case ':':	/* no argument provided (or missing argument) */
			puts(
				"TaHomaCrl v" VERSION "\n"
				"\tControl your TaHoma box from a command line.\n"
				"(c) L.Faillie (destroyedlolo) 2025-26\n"
				"\nScripting :\n"
				"\t-f : source provided script\n"
				"\t-N : don't execute ~/.tahomactl at startup\n"
				"\nTaHoma's :\n"
				"\t-H : set TaHoma's hostname\n"
				"\t-p : set TaHoma's port\n"
				"\t-k : set bearer token\n"
				"\t-U : don't verify SSL chaine (unsafe mode)\n"
				"\nLimiting scanning :\n"
				"\t-4 : resolve Avahi advertisement in IPv4 only\n"
				"\t-6 : resolve Avahi advertisement in IPv6 only\n"
				"\nMisc :\n"
				"\t-v : add verbosity\n"
				"\t-V : add even more verbosity\n"
				"\t-t : add tracing\n"
				"\t-d : add some debugging messages\n"
				"\t-h ; display this help"
			);
			exit(EXIT_FAILURE);
		}
	}

		/* libCURL's */
	curl_global_init(CURL_GLOBAL_DEFAULT);
	atexit(curl_cleanup);
	if(!(curl = curl_easy_init())){
		fputs("*F* curl_easy_init() failed.\n", stderr);
		exit(EXIT_FAILURE);
	}

	if(unsafe){
		if(debug || verbose)
			puts("*W* SSL chaine not enforced (unsafe mode)");

		curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);	/* Don't verify SSL */
		curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
	}

	if(!nostartup){
			/* Read startup (configuration ?) file */
		struct passwd *pw = getpwuid(getuid());	/* Find user's info */
		if(!pw)
			fputs("*E* Can't read user's info\n", stderr);
		else {
			char t[strlen(pw->pw_dir) + 12];	/* "/.tahomactl" */
			sprintf(t, "%s/.tahomactl", pw->pw_dir);
			execscript(t, true);
		}
	}
	
		/* Command line handling */
	rl_attempted_completion_function = command_completion;
	for(;;){
		char *l = readline(isatty(STDIN_FILENO) ? "TaHomaCtl > ":NULL);
		
		if(!l)			// End requested
			break;

		char *line;
		for(line = l; *line && !isgraph(*line); ++line);	// Strip spaces

		if(*line){	// Ignore empty line
			if(isatty(fileno(stdin)))
				add_history(line);

			execline(line);
		}

		free(l);
	}
}
