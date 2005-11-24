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
 *         -leXosip2 -losip2 -losipparser2 -lpthread
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
#include <eXosip2/eXosip.h>

#define _GNU_SOURCE
#ifdef _GNU_SOURCE
#include <getopt.h>

#endif /* _GNU_SOURCE */

#define PROG_NAME "sipreg"
#define PROG_VER  "1.0"
#define UA_STRING "SipReg v" PROG_VER
#define SYSLOG_FACILITY LOG_DAEMON

#ifdef LOG_PERROR
/* If we can, we use syslog() to emit the debugging messages to stderr. */
#define syslog_wrapper    syslog
#else
#define syslog_wrapper(a,b...) syslog(a,b),fprintf(stderr,b),fprintf(stderr,"\n")
#endif

static void usage (void);
static void *register_proc (void *arg);

static void
usage (void)
{
  printf ("Usage: " PROG_NAME " [required_options] [optional_options]\n"
          "\n\t[required_options]\n"
          "\t-r --proxy\tsip:proxyhost[:port]\n"
          "\t-u --from\tsip:user@host[:port]\n"
          "\t-c --contact\tsip:user@host[:port]\n"
          "\n\t[optional_options]\n"
          "\t-d --debug (log to stderr and do not fork)\n"
          "\t-e --expiry\tnumber (default 3600)\n"
          "\t-f --firewallip\tN.N.N.N\n"
          "\t-h --help\n"
          "\t-l --localip\tN.N.N.N (force local IP address)\n"
          "\t-p --port\tnumber (default 5060)\n"
          "\t-U --username\tauthentication username\n"
          "\t-P --password\tauthentication password\n");
}

typedef struct regparam_t
{
  int regid;
  int expiry;
  int auth;
} regparam_t;

static void *
register_proc (void *arg)
{
  struct regparam_t *regparam = arg;
  int reg;

  for (;;)
    {
      sleep (regparam->expiry / 2);
      eXosip_lock ();
      reg = eXosip_register_send_register (regparam->regid, NULL);
      if (0 > reg)
        {
          perror ("eXosip_register");
          exit (1);
        }
      regparam->auth = 0;
      eXosip_unlock ();
    }
  return NULL;
}

int
main (int argc, char *argv[])
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
  int nofork = 0;

  for (;;)
    {
#define short_options "c:de:f:hl:p:r:u:U:P:"
#ifdef _GNU_SOURCE
      int option_index = 0;
      static struct option long_options[] = {
        {"contact", required_argument, NULL, 'c'},
        {"debug", no_argument, NULL, 'd'},
        {"expiry", required_argument, NULL, 'e'},
        {"firewallip", required_argument, NULL, 'f'},
        {"from", required_argument, NULL, 'u'},
        {"help", no_argument, NULL, 'h'},
        {"localip", required_argument, NULL, 'l'},
        {"port", required_argument, NULL, 'p'},
        {"proxy", required_argument, NULL, 'r'},
        {"username", required_argument, NULL, 'U'},
        {"password", required_argument, NULL, 'P'},
        {NULL, 0, NULL, 0}
      };

      c = getopt_long (argc, argv, short_options, long_options, &option_index);
#else
      c = getopt (argc, argv, short_options);
#endif
      if (c == -1)
        break;

      switch (c)
        {
          case 'c':
            contact = optarg;
            break;
          case 'd':
            nofork = 1;
#ifdef LOG_PERROR
            debug = LOG_PERROR;
#endif
            break;
          case 'e':
            regparam.expiry = atoi (optarg);
            break;
          case 'f':
            firewallip = optarg;
            break;
          case 'h':
            usage ();
            exit (0);
          case 'l':
            localip = optarg;
            break;
          case 'p':
            service = getservbyname (optarg, "udp");
            if (service)
              port = service->s_port;
            else
              port = atoi (optarg);
            break;
          case 'r':
            proxy = optarg;
            break;
          case 'u':
            fromuser = optarg;
            break;
          case 'U':
            username = malloc (strlen (optarg));
            strcpy (username, optarg);
            memset (optarg, 0, strlen (optarg));
            break;
          case 'P':
            password = malloc (strlen (optarg));
            strcpy (password, optarg);
            memset (optarg, 0, strlen (optarg));
            break;
          default:
            break;
        }
    }

  if (!proxy || !contact || !fromuser)
    {
      usage ();
      exit (1);
    }

  if (!nofork)
    {
      int cpid = fork ();

      if (cpid)                 /* parent */
        exit (0);
      /* child */
      close (0);
      close (1);
      close (2);
    }

  openlog (PROG_NAME, LOG_PID | debug, SYSLOG_FACILITY);

  syslog_wrapper (LOG_INFO, UA_STRING " up and running");
  syslog_wrapper (LOG_INFO, "proxy: %s", proxy);
  syslog_wrapper (LOG_INFO, "fromuser: %s", fromuser);
  syslog_wrapper (LOG_INFO, "contact: %s", contact);
  syslog_wrapper (LOG_INFO, "expiry: %d", regparam.expiry);
  syslog_wrapper (LOG_INFO, "local port: %d", port);

  if (eXosip_init ())
    {
      syslog_wrapper (LOG_ERR, "eXosip_init failed");
      exit (1);
    }
  if (eXosip_listen_addr (IPPROTO_UDP, NULL, port, AF_INET, 0))
    {
      syslog_wrapper (LOG_ERR, "eXosip_listen_to failed");
      exit (1);
    }

  if (localip)
    {
      syslog_wrapper (LOG_INFO, "local address: %s", localip);
      eXosip_force_masquerade_contact (localip);
    }

  if (firewallip)
    {
      syslog_wrapper (LOG_INFO, "firewall address: %s:%i", firewallip, port);
      eXosip_masquerade_contact (firewallip, port);
    }

  eXosip_set_user_agent (UA_STRING);

  if (username && password)
    {
      syslog_wrapper (LOG_INFO, "username: %s", username);
      syslog_wrapper (LOG_INFO, "password: [removed]");
      if (eXosip_add_authentication_info
          (username, username, password, NULL, NULL))
        {
          syslog_wrapper (LOG_ERR, "eXosip_add_authentication_info failed");
          exit (1);
        }
    }

  {
    osip_message_t *reg = NULL;
    int i;

    regparam.regid =
      eXosip_register_build_initial_register (fromuser, proxy, contact, 3600,
                                              &reg);
    if (regparam.regid < 1)
      {
        syslog_wrapper (LOG_ERR, "eXosip_register_build_initial_register failed");
        exit (1);
      }
    i = eXosip_register_send_register (regparam.regid, reg);
    if (i != 0)
      {
        syslog_wrapper (LOG_ERR, "eXosip_register_send_register failed");
        exit (1);
      }
  }

  if (pthread_create (&register_thread, NULL, register_proc, &regparam))
    {
      syslog_wrapper (LOG_ERR, "pthread_create failed");
      exit (1);
    }

  for (;;)
    {
      eXosip_event_t *event;

      if (!(event = eXosip_event_wait (0, 0)))
        {
          usleep (10000);
          continue;
        }

      switch (event->type)
        {
          case EXOSIP_REGISTRATION_NEW:
            syslog_wrapper (LOG_INFO, "received new registration");
            break;
          case EXOSIP_REGISTRATION_SUCCESS:
            syslog_wrapper (LOG_INFO, "registrered successfully");
            break;
          case EXOSIP_REGISTRATION_FAILURE:
            eXosip_lock ();
            if (regparam.auth)
              {
                syslog_wrapper (LOG_WARNING, "registration failed");
                regparam.auth = 0;
                eXosip_unlock ();
                break;
              }
            eXosip_register_send_register (regparam.regid, NULL);
            eXosip_unlock ();
            regparam.auth = 1;
            break;
          case EXOSIP_REGISTRATION_TERMINATED:
            printf ("Registration terminated\n");
            break;
          default:
            syslog_wrapper (LOG_DEBUG,
                            "recieved unknown eXosip event (type, did, cid) = (%d, %d, %d)",
                            event->type, event->did, event->cid);

        }
      eXosip_event_free (event);
    }
}
