/*
 * SIP Registration Agent -- by ww@styx.org
 * 
 * This program is Free Software, released under the GNU General
 * Public License v2.0 http://www.gnu.org/licenses/gpl
 *
 * This program will register to a SIP proxy using the contact
 * supplied on the command line. This is useful if, for some 
 * reason your SIP client cannot register to the proxy itself.
 * For example, if your SIP client registers to Proxy A, but
 * you want to be able to recieve calls that arrive at Proxy B,
 * you can use this program to register the client's contact
 * information to Proxy B.
 *
 * This program requires the eXosip library. To compile,
 * assuming your eXosip installation is in /usr/local,
 * use something like:
 *
 * gcc -O2 -I/usr/local/include -L/usr/local/lib sipreg.c \
 *         -o sipreg \
 *         -leXosip -losip2 -losipparser2 -lpthread
 *
 * It should compile and run on any POSIX compliant system
 * that supports pthreads.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <netdb.h>
#include <syslog.h>
#include <pthread.h>
#include <eXosip/eXosip.h>

#define _GNU_SOURCE
#ifdef _GNU_SOURCE
#include <getopt.h>

#endif /* _GNU_SOURCE */

#define PROG_NAME "sipreg"
#define PROG_VER  "1.0"
#define UA_STRING "SipReg v" PROG_VER
#define SYSLOG_FACILITY LOG_DAEMON

void usage(void) {
    printf("Usage: " PROG_NAME " [required_options] [optional_options]\n"
	   "\n\t[required_options]\n"
	   "\t-r --proxy\tsip:proxyhost[:port]\n"
	   "\t-u --from\tsip:user@host[:port]\n"
	   "\t-c --contact\tsip:user@host[:port]\n"
	   "\n\t[optional_options]\n"
	   "\t-d --debug (log to stderr and do not fork\n"
	   "\t-e --expiry\tnumber (default 3600)\n"
	   "\t-f --firewallip\tN.N.N.N\n"
	   "\t-h --help\n"
	   "\t-l --localip\tN.N.N.N (force local IP address)\n"
	   "\t-p --port\tnumber (default 5060)\n"
	   "\t-U --username\tauthentication username\n"
	   "\t-P --password\tauthentication password\n");
}

typedef struct regparam_t {
    int regid;
    int expiry;
    int auth;
} regparam_t;

void *register_proc(void *arg) {
    struct regparam_t *regparam = arg;
    int reg;
    for(;;) {
	eXosip_lock();
	if(0 > (reg = eXosip_register(regparam->regid, regparam->expiry))) {
	    perror("eXosip_register");
	    exit(1);
	}
	regparam->auth = 0;
	eXosip_unlock();
	sleep(regparam->expiry/2);
    }
    return NULL;
}

int main(int argc, char *argv[])
{
    int c;
    int port = 5060;
    char *contact = NULL;
    char *fromuser = NULL;
    const char *localip = NULL;
    const char *firewallip = NULL;
    char *proxy = NULL;
    struct servent *service;
    char *username = NULL;
    char *password = NULL;
    struct regparam_t regparam = { 0, 3600, 0 };
    pthread_t register_thread;
    int debug = 0;

    for(;;) {
#define short_options "c:de:f:hl:p:r:u:U:P:"
#ifdef _GNU_SOURCE
	int option_index = 0;
	static struct option long_options[] = {
	    { "contact", required_argument, NULL, 'c' },
	    { "debug", no_argument, NULL, 'd' },
	    { "expiry", required_argument, NULL, 'e' },
	    { "firewallip", required_argument, NULL, 'f' },
	    { "from", required_argument, NULL, 'u' },
	    { "help", no_argument, NULL, 'h' },
	    { "localip", required_argument, NULL, 'l' },
	    { "port", required_argument, NULL, 'p' },
	    { "proxy", required_argument, NULL, 'r' },
	    { "username", required_argument, NULL, 'U' },
	    { "password", required_argument, NULL, 'P' },
	    { NULL, 0, NULL, 0 }
	};
	
	c = getopt_long(argc, argv, short_options, long_options, &option_index);
#else
	c = getopt(argc, argv, short_options);
#endif
	if(c == -1)
	    break;

	switch(c) {
	case 'c':
	    contact = optarg;
	    break;
	case 'd':
	    debug = LOG_PERROR;
	    break;
	case 'e':
	    regparam.expiry = atoi(optarg);
	    break;
	case 'f':
	    firewallip = optarg;
	    break;
	case 'h':
	    usage();
	    exit(0);
	case 'l':
	    localip = optarg;
	    break;
	case 'p':
	    service = getservbyname(optarg, "udp");
	    if(service)
		port = service->s_port;
	    else
		port = atoi(optarg);
	    break;
	case 'r':
	    proxy = optarg;
	    break;
	case 'u':
	    fromuser = optarg;
	    break;
	case 'U':
	    username = malloc(strlen(optarg));
	    strcpy(username, optarg);
	    memset(optarg, 0, strlen(optarg));
	    break;
	case 'P':
	    password = malloc(strlen(optarg));
	    strcpy(password, optarg);
	    memset(optarg, 0, strlen(optarg));
	    break;
	default:
	    break;
	}
    }

    if(!proxy || !contact || !fromuser) {
	usage();
	exit(1);
    }

    if(!debug) {
	int cpid = fork();
	if(cpid) /* parent */
	    exit(0);
	/* child */
	close(0); close(1); close(2);
    }

    openlog(PROG_NAME, LOG_PID|debug, SYSLOG_FACILITY);

    syslog(LOG_INFO, UA_STRING " up and running");
    syslog(LOG_INFO, "proxy: %s", proxy);
    syslog(LOG_INFO, "fromuser: %s", fromuser);
    syslog(LOG_INFO, "contact: %s", contact);
    syslog(LOG_INFO, "expiry: %d", regparam.expiry);
    syslog(LOG_INFO, "local port: %d", port);

    if(eXosip_init(NULL, NULL, port)) {
	syslog(LOG_ERR, "eXosip_init failed");
	exit(1);
    }

    eXosip_sdp_negotiation_remove_audio_payloads();

    if(localip) {
	syslog(LOG_INFO, "local address: %s", localip);
	eXosip_force_localip(localip);
    }

    if(firewallip) {
	syslog(LOG_INFO, "firewall address: %s", firewallip);
	eXosip_set_firewallip(firewallip);
    }

    eXosip_set_user_agent(UA_STRING);

    if(username && password) {
	syslog(LOG_INFO, "username: %s", username);
	syslog(LOG_INFO, "password: [removed]");
	if(eXosip_add_authentication_info(username, username, password, NULL, NULL)) {
	    syslog(LOG_ERR, "eXosip_add_authentication_info failed");
	    exit(1);
	}
    }

    regparam.regid = eXosip_register_init(fromuser, proxy, contact);
    if(regparam.regid < 1) {
	syslog(LOG_ERR, "eXosip_register_init failed");
	exit(1);
    }

    if(pthread_create(&register_thread, NULL, register_proc, &regparam)) {
	syslog(LOG_ERR, "pthread_create failed");
	exit(1);
    }

    for(;;) {
	eXosip_event_t *event;

	if(!(event = eXosip_event_wait(0, 0))) {
	    usleep(10000);
	    continue;
	}

	switch(event->type) {
	case EXOSIP_REGISTRATION_NEW:
	    syslog(LOG_INFO, "received new registration");
	    break;
	case EXOSIP_REGISTRATION_SUCCESS:
	    syslog(LOG_INFO, "registrered successfully");
	    break;
	case EXOSIP_REGISTRATION_FAILURE:
	    eXosip_lock();
	    if(regparam.auth) {
		syslog(LOG_WARNING, "registration failed");
		regparam.auth = 0;
		eXosip_unlock();
		break;
	    }
	    eXosip_register(regparam.regid, regparam.expiry);
	    eXosip_unlock();
	    regparam.auth = 1;
	    break;
	case EXOSIP_REGISTRATION_TERMINATED:
	    printf("Registration terminated\n");
	    break;
	default:
	    syslog(LOG_DEBUG, "recieved unknown eXosip event (type, did, cid) = (%d, %d, %d)",
		   event->type, event->did, event->cid);

	}
	eXosip_event_free(event);
    }
}
