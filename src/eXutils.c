/*
  eXosip - This is the eXtended osip library.
  Copyright (C) 2002,2003,2004,2005,2006,2007  Aymeric MOIZARD  - jack@atosc.org
  
  eXosip is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
  
  eXosip is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
  
  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/


#ifdef ENABLE_MPATROL
#include <mpatrol.h>
#endif

#include <osipparser2/osip_port.h>
#include "eXosip2.h"

#if defined(_WIN32_WCE)
#elif defined(WIN32)
#include <windns.h>
#include <malloc.h>
#else
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#ifdef HAVE_ARPA_NAMESER_H
#include <arpa/nameser.h>
#endif

#ifdef HAVE_RESOLV8_COMPAT_H
#include <nameser8_compat.h>
#include <resolv8_compat.h>
#elif defined(HAVE_RESOLV_H) || defined(OpenBSD) || defined(FreeBSD) || defined(NetBSD)
#include <resolv.h>
#endif

#ifdef HAVE_NETDB_H
#include <netdb.h>
#endif
#endif


extern eXosip_t eXosip;

extern int ipv6_enable;

#if defined(__arc__)
#define USE_GETHOSTBYNAME
#endif

#if defined(USE_GETHOSTBYNAME)

void
eXosip_freeaddrinfo (struct addrinfo *ai)
{
  struct addrinfo *next;

  while (ai)
    {
      next = ai->ai_next;
      free (ai);
      ai = next;
    }
}

struct namebuf
{
  struct hostent hostentry;
  char *h_addr_list[2];
  struct in_addr addrentry;
  char h_name[16];              /* 123.123.123.123 = 15 letters is maximum */
};

static struct addrinfo *
osip_he2ai (struct hostent *he, int port, int protocol)
{
  struct addrinfo *ai;

  struct addrinfo *prevai = NULL;

  struct addrinfo *firstai = NULL;

  struct sockaddr_in *addr;

  int i;

  struct in_addr *curr;

  if (!he)
    /* no input == no output! */
    return NULL;

  for (i = 0; (curr = (struct in_addr *) he->h_addr_list[i]); i++)
    {

      ai = calloc (1, sizeof (struct addrinfo) + sizeof (struct sockaddr_in));

      if (!ai)
        break;

      if (!firstai)
        /* store the pointer we want to return from this function */
        firstai = ai;

      if (prevai)
        /* make the previous entry point to this */
        prevai->ai_next = ai;

      ai->ai_family = AF_INET;  /* we only support this */
      if (protocol == IPPROTO_UDP)
        ai->ai_socktype = SOCK_DGRAM;
      else
        ai->ai_socktype = SOCK_STREAM;
      ai->ai_addrlen = sizeof (struct sockaddr_in);
      /* make the ai_addr point to the address immediately following this struct
         and use that area to store the address */
      ai->ai_addr = (struct sockaddr *) ((char *) ai + sizeof (struct addrinfo));

      /* leave the rest of the struct filled with zero */

      addr = (struct sockaddr_in *) ai->ai_addr;        /* storage area for this info */

      memcpy ((char *) &(addr->sin_addr), curr, sizeof (struct in_addr));
      addr->sin_family = he->h_addrtype;
      addr->sin_port = htons ((unsigned short) port);

      prevai = ai;
    }
  return firstai;
}

/*
 * osip_ip2addr() takes a 32bit ipv4 internet address as input parameter
 * together with a pointer to the string version of the address, and it
 * returns a struct addrinfo chain filled in correctly with information for this
 * address/host.
 *
 * The input parameters ARE NOT checked for validity but they are expected
 * to have been checked already when this is called.
 */
static struct addrinfo *
osip_ip2addr (in_addr_t num, const char *hostname, int port, int protocol)
{
  struct addrinfo *ai;

  struct hostent *h;

  struct in_addr *addrentry;

  struct namebuf buffer;

  struct namebuf *buf = &buffer;

  h = &buf->hostentry;
  h->h_addr_list = &buf->h_addr_list[0];
  addrentry = &buf->addrentry;
  addrentry->s_addr = num;
  h->h_addr_list[0] = (char *) addrentry;
  h->h_addr_list[1] = NULL;
  h->h_addrtype = AF_INET;
  h->h_length = sizeof (*addrentry);
  h->h_name = &buf->h_name[0];
  h->h_aliases = NULL;

  /* Now store the dotted version of the address */
  snprintf ((char *) h->h_name, 16, "%s", hostname);

  ai = osip_he2ai (h, port, protocol);
  return ai;
}

static int
eXosip_inet_pton (int family, const char *src, void *dst)
{
  if (strchr (src, ':'))        /* possible IPv6 address */
    return OSIP_UNDEFINED_ERROR;        /* (inet_pton(AF_INET6, src, dst)); */
  else if (strchr (src, '.'))   /* possible IPv4 address */
    {
      struct in_addr *tmp = dst;

      tmp->s_addr = inet_addr (src);    /* already in N. byte order */
      if (tmp->s_addr == INADDR_NONE)
        return 0;

      return 1;                 /* (inet_pton(AF_INET, src, dst)); */
  } else                        /* Impossibly a valid ip address */
    return INADDR_NONE;
}

/*
 * osip_getaddrinfo() - the ipv4 synchronous version.
 *
 * The original code to this function was from the Dancer source code, written
 * by Bjorn Reese, it has since been patched and modified considerably.
 *
 * gethostbyname_r() is the thread-safe version of the gethostbyname()
 * function. When we build for plain IPv4, we attempt to use this
 * function. There are _three_ different gethostbyname_r() versions, and we
 * detect which one this platform supports in the configure script and set up
 * the HAVE_GETHOSTBYNAME_R_3, HAVE_GETHOSTBYNAME_R_5 or
 * HAVE_GETHOSTBYNAME_R_6 defines accordingly. Note that HAVE_GETADDRBYNAME
 * has the corresponding rules. This is primarily on *nix. Note that some unix
 * flavours have thread-safe versions of the plain gethostbyname() etc.
 *
 */
int
eXosip_get_addrinfo (struct addrinfo **addrinfo,
                     const char *hostname, int port, int protocol)
{
  struct hostent *h = NULL;

  in_addr_t in;

  struct hostent *buf = NULL;

  char portbuf[10];

  *addrinfo = NULL;             /* default return */

  if (port < 0)                 /* -1 for SRV record */
    return OSIP_BADPARAMETER;

  snprintf (portbuf, sizeof (portbuf), "%i", port);

  if (1 == eXosip_inet_pton (AF_INET, hostname, &in))
    /* This is a dotted IP address 123.123.123.123-style */
    {
      *addrinfo = osip_ip2addr (in, hostname, port, protocol);
      return OSIP_SUCCESS;
    }
#if defined(HAVE_GETHOSTBYNAME_R)
  /*
   * gethostbyname_r() is the preferred resolve function for many platforms.
   * Since there are three different versions of it, the following code is
   * somewhat #ifdef-ridden.
   */
  else
    {
      int h_errnop;

      int res = ERANGE;

      buf = (struct hostent *) calloc (CURL_HOSTENT_SIZE, 1);
      if (!buf)
        return NULL;            /* major failure */
      /*
       * The clearing of the buffer is a workaround for a gethostbyname_r bug in
       * qnx nto and it is also _required_ for some of these functions on some
       * platforms.
       */

#ifdef HAVE_GETHOSTBYNAME_R_5
      /* Solaris, IRIX and more */
      (void) res;               /* prevent compiler warning */
      h = gethostbyname_r (hostname,
                           (struct hostent *) buf,
                           (char *) buf + sizeof (struct hostent),
                           CURL_HOSTENT_SIZE - sizeof (struct hostent), &h_errnop);

      /* If the buffer is too small, it returns NULL and sets errno to
       * ERANGE. The errno is thread safe if this is compiled with
       * -D_REENTRANT as then the 'errno' variable is a macro defined to get
       * used properly for threads.
       */

      if (h)
        {
          ;
      } else
#endif /* HAVE_GETHOSTBYNAME_R_5 */
#ifdef HAVE_GETHOSTBYNAME_R_6
        /* Linux */

        res = gethostbyname_r (hostname, (struct hostent *) buf, (char *) buf + sizeof (struct hostent), CURL_HOSTENT_SIZE - sizeof (struct hostent), &h,       /* DIFFERENCE */
                               &h_errnop);
      /* Redhat 8, using glibc 2.2.93 changed the behavior. Now all of a
       * sudden this function returns EAGAIN if the given buffer size is too
       * small. Previous versions are known to return ERANGE for the same
       * problem.
       *
       * This wouldn't be such a big problem if older versions wouldn't
       * sometimes return EAGAIN on a common failure case. Alas, we can't
       * assume that EAGAIN *or* ERANGE means ERANGE for any given version of
       * glibc.
       *
       * For now, we do that and thus we may call the function repeatedly and
       * fail for older glibc versions that return EAGAIN, until we run out of
       * buffer size (step_size grows beyond CURL_HOSTENT_SIZE).
       *
       * If anyone has a better fix, please tell us!
       *
       * -------------------------------------------------------------------
       *
       * On October 23rd 2003, Dan C dug up more details on the mysteries of
       * gethostbyname_r() in glibc:
       *
       * In glibc 2.2.5 the interface is different (this has also been
       * discovered in glibc 2.1.1-6 as shipped by Redhat 6). What I can't
       * explain, is that tests performed on glibc 2.2.4-34 and 2.2.4-32
       * (shipped/upgraded by Redhat 7.2) don't show this behavior!
       *
       * In this "buggy" version, the return code is -1 on error and 'errno'
       * is set to the ERANGE or EAGAIN code. Note that 'errno' is not a
       * thread-safe variable.
       */

      if (!h)                   /* failure */
#endif /* HAVE_GETHOSTBYNAME_R_6 */
#ifdef HAVE_GETHOSTBYNAME_R_3
        /* AIX, Digital Unix/Tru64, HPUX 10, more? */

        /* For AIX 4.3 or later, we don't use gethostbyname_r() at all, because of
         * the plain fact that it does not return unique full buffers on each
         * call, but instead several of the pointers in the hostent structs will
         * point to the same actual data! This have the unfortunate down-side that
         * our caching system breaks down horribly. Luckily for us though, AIX 4.3
         * and more recent versions have a "completely thread-safe"[*] libc where
         * all the data is stored in thread-specific memory areas making calls to
         * the plain old gethostbyname() work fine even for multi-threaded
         * programs.
         *
         * This AIX 4.3 or later detection is all made in the configure script.
         *
         * Troels Walsted Hansen helped us work this out on March 3rd, 2003.
         *
         * [*] = much later we've found out that it isn't at all "completely
         * thread-safe", but at least the gethostbyname() function is.
         */

        if (CURL_HOSTENT_SIZE >=
            (sizeof (struct hostent) + sizeof (struct hostent_data)))
          {

            /* August 22nd, 2000: Albert Chin-A-Young brought an updated version
             * that should work! September 20: Richard Prescott worked on the buffer
             * size dilemma.
             */

            res = gethostbyname_r (hostname,
                                   (struct hostent *) buf,
                                   (struct hostent_data *) ((char *) buf +
                                                            sizeof (struct
                                                                    hostent)));
            h_errnop = errno;   /* we don't deal with this, but set it anyway */
        } else
          res = -1;             /* failure, too smallish buffer size */

      if (!res)
        {                       /* success */

          h = buf;              /* result expected in h */

          /* This is the worst kind of the different gethostbyname_r() interfaces.
           * Since we don't know how big buffer this particular lookup required,
           * we can't realloc down the huge alloc without doing closer analysis of
           * the returned data. Thus, we always use CURL_HOSTENT_SIZE for every
           * name lookup. Fixing this would require an extra malloc() and then
           * calling struct addrinfo_copy() that subsequent realloc()s down the new
           * memory area to the actually used amount.
           */
      } else
#endif /* HAVE_GETHOSTBYNAME_R_3 */
        {
          OSIP_TRACE (osip_trace
                      (__FILE__, __LINE__, OSIP_INFO2, NULL,
                       "gethostbyname failure. %s:%s (%s)\n", hostname, port));
          h = NULL;             /* set return code to NULL */
          free (buf);
        }
#else /* HAVE_GETHOSTBYNAME_R */
  /*
   * Here is code for platforms that don't have gethostbyname_r() or for
   * which the gethostbyname() is the preferred() function.
   */
  else
    {
      h = NULL;
#if !defined(__arc__)
      h = gethostbyname (hostname);
#endif
      if (!h)
        {
          OSIP_TRACE (osip_trace
                      (__FILE__, __LINE__, OSIP_INFO2, NULL,
                       "gethostbyname failure. %s:%s (%s)\n", hostname, port));
        }
#endif /*HAVE_GETHOSTBYNAME_R */
    }

  if (h)
    {
      *addrinfo = osip_he2ai (h, port, protocol);

      if (buf)                  /* used a *_r() function */
        free (buf);
    }

  return OSIP_SUCCESS;
}

#endif

#if defined(WIN32) || defined(_WIN32_WCE)

/* You need the Platform SDK to compile this. */
#include <windows.h>
#include <iphlpapi.h>

int
eXosip_dns_get_local_fqdn (char **servername, char **serverip,
                           char **netmask, unsigned int WIN32_interface)
{
  unsigned int pos;

  *servername = NULL;           /* no name on win32? */
  *serverip = NULL;
  *netmask = NULL;

  /* First, try to get the interface where we should listen */
  {
    DWORD size_of_iptable = 0;

    PMIB_IPADDRTABLE ipt;

    PMIB_IFROW ifrow;

    if (GetIpAddrTable (NULL, &size_of_iptable, TRUE) == ERROR_INSUFFICIENT_BUFFER)
      {
        ifrow = (PMIB_IFROW) _alloca (sizeof (MIB_IFROW));
        ipt = (PMIB_IPADDRTABLE) _alloca (size_of_iptable);
        if (ifrow == NULL || ipt == NULL)
          {
            /* not very usefull to continue */
            OSIP_TRACE (osip_trace (__FILE__, __LINE__, OSIP_INFO4, NULL,
                                    "ERROR alloca failed\r\n"));
            return OSIP_NOMEM;
          }

        if (!GetIpAddrTable (ipt, &size_of_iptable, TRUE))
          {
            /* look for the best public interface */

            for (pos = 0; pos < ipt->dwNumEntries && *netmask == NULL; ++pos)
              {
                /* index is */
                struct in_addr addr;

                struct in_addr mask;

                ifrow->dwIndex = ipt->table[pos].dwIndex;
                if (GetIfEntry (ifrow) == NO_ERROR)
                  {
                    switch (ifrow->dwType)
                      {
                        case MIB_IF_TYPE_LOOPBACK:
                          /*    break; */
                        case MIB_IF_TYPE_ETHERNET:
                        default:
                          addr.s_addr = ipt->table[pos].dwAddr;
                          mask.s_addr = ipt->table[pos].dwMask;
                          if (ipt->table[pos].dwIndex == WIN32_interface)
                            {
                              *servername = NULL;       /* no name on win32? */
                              *serverip = osip_strdup (inet_ntoa (addr));
                              *netmask = osip_strdup (inet_ntoa (mask));
                              OSIP_TRACE (osip_trace
                                          (__FILE__, __LINE__, OSIP_INFO4, NULL,
                                           "Interface ethernet: %s/%s\r\n",
                                           *serverip, *netmask));
                              break;
                            }
                      }
                  }
              }
          }
      }
  }

  if (*serverip == NULL || *netmask == NULL)
    {
      OSIP_TRACE (osip_trace (__FILE__, __LINE__, OSIP_WARNING, NULL,
                              "ERROR No network interface found\r\n"));
      return OSIP_NO_NETWORK;
    }

  return OSIP_SUCCESS;
}

int
eXosip_guess_ip_for_via (int family, char *address, int size)
{
  SOCKET sock;

  SOCKADDR_STORAGE local_addr;

  int local_addr_len;

  struct addrinfo *addrf;

  address[0] = '\0';
  sock = socket (family, SOCK_DGRAM, 0);

  if (family == AF_INET)
    {
      getaddrinfo (eXosip.ipv4_for_gateway, NULL, NULL, &addrf);
  } else if (family == AF_INET6)
    {
      getaddrinfo (eXosip.ipv6_for_gateway, NULL, NULL, &addrf);
    }

  if (addrf == NULL)
    {
      closesocket (sock);
      snprintf (address, size, (family == AF_INET) ? "127.0.0.1" : "::1");
      return OSIP_NO_NETWORK;
    }

  if (WSAIoctl
      (sock, SIO_ROUTING_INTERFACE_QUERY, addrf->ai_addr, addrf->ai_addrlen,
       &local_addr, sizeof (local_addr), &local_addr_len, NULL, NULL) != 0)
    {
      closesocket (sock);
      freeaddrinfo (addrf);
      snprintf (address, size, (family == AF_INET) ? "127.0.0.1" : "::1");
      return OSIP_NO_NETWORK;
    }

  closesocket (sock);
  freeaddrinfo (addrf);

  if (getnameinfo ((const struct sockaddr *) &local_addr,
                   local_addr_len, address, size, NULL, 0, NI_NUMERICHOST))
    {
      snprintf (address, size, (family == AF_INET) ? "127.0.0.1" : "::1");
      return OSIP_NO_NETWORK;
    }

  return OSIP_SUCCESS;
}

#else /* sun, *BSD, linux, and other? */


#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#include <sys/ioctl.h>
#include <net/if.h>
#include <unistd.h>
#include <sys/param.h>

#include <stdio.h>

static int _eXosip_default_gateway_ipv4 (char *address, int size);

static int _eXosip_default_gateway_ipv6 (char *address, int size);

#ifdef HAVE_GETIFADDRS

#include <ifaddrs.h>
static int
_eXosip_default_gateway_with_getifaddrs (int type, char *address, int size)
{
  struct ifaddrs *ifp;

  struct ifaddrs *ifpstart;

  int ret = -1;

  if (getifaddrs (&ifpstart) < 0)
    {
      return OSIP_NO_NETWORK;
    }

  for (ifp = ifpstart; ifp != NULL; ifp = ifp->ifa_next)
    {
      if (ifp->ifa_addr && ifp->ifa_addr->sa_family == type
          && (ifp->ifa_flags & IFF_RUNNING) && !(ifp->ifa_flags & IFF_LOOPBACK))
        {
          getnameinfo (ifp->ifa_addr,
                       (type == AF_INET6) ?
                       sizeof (struct sockaddr_in6) : sizeof (struct sockaddr_in),
                       address, size, NULL, 0, NI_NUMERICHOST);
          if (strchr (address, '%') == NULL)
            {                   /*avoid ipv6 link-local addresses */
              OSIP_TRACE (osip_trace
                          (__FILE__, __LINE__, OSIP_INFO2, NULL,
                           "_eXosip_default_gateway_with_getifaddrs(): found %s\n",
                           address));
              ret = 0;
              break;
            }
        }
    }
  freeifaddrs (ifpstart);
  return ret;
}
#endif

int
eXosip_guess_ip_for_via (int family, char *address, int size)
{
  int err;

  if (family == AF_INET6)
    {
      err = _eXosip_default_gateway_ipv6 (address, size);
  } else
    {
      err = _eXosip_default_gateway_ipv4 (address, size);
    }
#ifdef HAVE_GETIFADDRS
  if (err < 0)
    err = _eXosip_default_gateway_with_getifaddrs (family, address, size);
#endif
  return err;
}

/* This is a portable way to find the default gateway.
 * The ip of the default interface is returned.
 */
static int
_eXosip_default_gateway_ipv4 (char *address, int size)
{
#ifdef __APPLE_CC__
  int len;
#else
  unsigned int len;
#endif
  int sock_rt, on = 1;

  struct sockaddr_in iface_out;

  struct sockaddr_in remote;

  memset (&remote, 0, sizeof (struct sockaddr_in));

  remote.sin_family = AF_INET;
  remote.sin_addr.s_addr = inet_addr (eXosip.ipv4_for_gateway);
  remote.sin_port = htons (11111);

  memset (&iface_out, 0, sizeof (iface_out));
  sock_rt = socket (AF_INET, SOCK_DGRAM, 0);

  if (setsockopt (sock_rt, SOL_SOCKET, SO_BROADCAST, &on, sizeof (on)) == -1)
    {
      perror ("DEBUG: [get_output_if] setsockopt(SOL_SOCKET, SO_BROADCAST");
      close (sock_rt);
      snprintf (address, size, "127.0.0.1");
      return OSIP_NO_NETWORK;
    }

  if (connect
      (sock_rt, (struct sockaddr *) &remote, sizeof (struct sockaddr_in)) == -1)
    {
      perror ("DEBUG: [get_output_if] connect");
      close (sock_rt);
      snprintf (address, size, "127.0.0.1");
      return OSIP_NO_NETWORK;
    }

  len = sizeof (iface_out);
  if (getsockname (sock_rt, (struct sockaddr *) &iface_out, &len) == -1)
    {
      perror ("DEBUG: [get_output_if] getsockname");
      close (sock_rt);
      snprintf (address, size, "127.0.0.1");
      return OSIP_NO_NETWORK;
    }

  close (sock_rt);
  if (iface_out.sin_addr.s_addr == 0)
    {                           /* what is this case?? */
      snprintf (address, size, "127.0.0.1");
      return OSIP_NO_NETWORK;
    }
  osip_strncpy (address, inet_ntoa (iface_out.sin_addr), size - 1);
  return OSIP_SUCCESS;
}


/* This is a portable way to find the default gateway.
 * The ip of the default interface is returned.
 */
static int
_eXosip_default_gateway_ipv6 (char *address, int size)
{
#ifdef __APPLE_CC__
  int len;
#else
  unsigned int len;
#endif
  int sock_rt, on = 1;

  struct sockaddr_in6 iface_out;

  struct sockaddr_in6 remote;

  memset (&remote, 0, sizeof (struct sockaddr_in6));

  remote.sin6_family = AF_INET6;
  inet_pton (AF_INET6, eXosip.ipv6_for_gateway, &remote.sin6_addr);
  remote.sin6_port = htons (11111);

  memset (&iface_out, 0, sizeof (iface_out));
  sock_rt = socket (AF_INET6, SOCK_DGRAM, 0);
  /*default to ipv6 local loopback in case something goes wrong: */
  strncpy (address, "::1", size);
  if (setsockopt (sock_rt, SOL_SOCKET, SO_BROADCAST, &on, sizeof (on)) == -1)
    {
      perror ("DEBUG: [get_output_if] setsockopt(SOL_SOCKET, SO_BROADCAST");
      close (sock_rt);
      return OSIP_NO_NETWORK;
    }

  if (connect
      (sock_rt, (struct sockaddr *) &remote, sizeof (struct sockaddr_in6)) == -1)
    {
      perror ("DEBUG: [get_output_if] connect");
      close (sock_rt);
      return OSIP_NO_NETWORK;
    }

  len = sizeof (iface_out);
  if (getsockname (sock_rt, (struct sockaddr *) &iface_out, &len) == -1)
    {
      perror ("DEBUG: [get_output_if] getsockname");
      close (sock_rt);
      return OSIP_NO_NETWORK;
    }
  close (sock_rt);

  if (iface_out.sin6_addr.s6_addr == 0)
    {                           /* what is this case?? */
      return OSIP_NO_NETWORK;
    }
  inet_ntop (AF_INET6, (const void *) &iface_out.sin6_addr, address, size - 1);
  return OSIP_SUCCESS;
}

#endif

char *
strdup_printf (const char *fmt, ...)
{
  /* Guess we need no more than 100 bytes. */
  int n, size = 100;

  char *p;

  va_list ap;

  if ((p = osip_malloc (size)) == NULL)
    return NULL;
  while (1)
    {
      /* Try to print in the allocated space. */
      va_start (ap, fmt);
#ifdef WIN32
      n = _vsnprintf (p, size, fmt, ap);
#else
      n = vsnprintf (p, size, fmt, ap);
#endif
      va_end (ap);
      /* If that worked, return the string. */
      if (n > -1 && n < size)
        return p;
      /* Else try again with more space. */
      if (n > -1)               /* glibc 2.1 */
        size = n + 1;           /* precisely what is needed */
      else                      /* glibc 2.0 */
        size *= 2;              /* twice the old size */
      if ((p = realloc (p, size)) == NULL)
        return NULL;
    }
}

#if !defined(USE_GETHOSTBYNAME)

int
eXosip_get_addrinfo (struct addrinfo **addrinfo, const char *hostname,
                     int service, int protocol)
{
  struct addrinfo hints;

  int error;

  char portbuf[10];

  if (hostname == NULL)
    return OSIP_BADPARAMETER;

  if (service != -1)            /* -1 for SRV record */
    {
      int i;

      for (i = 0; i < MAX_EXOSIP_DNS_ENTRY; i++)
        {
          if (eXosip.dns_entries[i].host[0] != '\0'
              && 0 == osip_strcasecmp (eXosip.dns_entries[i].host, hostname))
            {
              /* update entry */
              if (eXosip.dns_entries[i].ip[0] != '\0')
                {
                  hostname = eXosip.dns_entries[i].ip;
                  OSIP_TRACE (osip_trace
                              (__FILE__, __LINE__, OSIP_INFO1, NULL,
                               "eXosip option set: dns cache used:%s -> %s\n",
                               eXosip.dns_entries[i].host,
                               eXosip.dns_entries[i].ip));
                }
            }
        }
    }

  if (service != -1)            /* -1 for SRV record */
    snprintf (portbuf, sizeof (portbuf), "%i", service);

  memset (&hints, 0, sizeof (hints));

  hints.ai_flags = 0;

  if (ipv6_enable)
    hints.ai_family = PF_INET6;
  else
    hints.ai_family = PF_INET;  /* ipv4 only support */

  if (protocol == IPPROTO_UDP)
    hints.ai_socktype = SOCK_DGRAM;
  else
    hints.ai_socktype = SOCK_STREAM;

  hints.ai_protocol = protocol; /* IPPROTO_UDP or IPPROTO_TCP */
  if (service == -1)            /* -1 for SRV record */
    {
      error = getaddrinfo (hostname, "sip", &hints, addrinfo);
      OSIP_TRACE (osip_trace
                  (__FILE__, __LINE__, OSIP_INFO2, NULL,
                   "SRV resolution with udp-sip-%s\n", hostname));
  } else
    {
      error = getaddrinfo (hostname, portbuf, &hints, addrinfo);
      OSIP_TRACE (osip_trace
                  (__FILE__, __LINE__, OSIP_INFO2, NULL,
                   "DNS resolution with %s:%i\n", hostname, service));
    }
  if (error || *addrinfo == NULL)
    {
      OSIP_TRACE (osip_trace
                  (__FILE__, __LINE__, OSIP_INFO2, NULL,
                   "getaddrinfo failure. %s:%s (%d)\n", hostname, portbuf, error));
      return OSIP_UNKNOWN_HOST;
  } else
    {
      struct addrinfo *elem;

      char tmp[INET6_ADDRSTRLEN];

      char porttmp[10];

      OSIP_TRACE (osip_trace
                  (__FILE__, __LINE__, OSIP_INFO2, NULL,
                   "getaddrinfo returned the following addresses:\n"));
      for (elem = *addrinfo; elem != NULL; elem = elem->ai_next)
        {
          getnameinfo (elem->ai_addr, elem->ai_addrlen, tmp, sizeof (tmp), porttmp,
                       sizeof (porttmp), NI_NUMERICHOST | NI_NUMERICSERV);
          OSIP_TRACE (osip_trace
                      (__FILE__, __LINE__, OSIP_INFO2, NULL, "%s port %s\n", tmp,
                       porttmp));
        }
    }

  return OSIP_SUCCESS;
}
#endif

#ifdef SRV_RECORD

static void osip_srv_record_sort(struct osip_srv_record *rec, int n){
  int i;
  int permuts;
  struct osip_srv_entry swap;
  do{
    struct osip_srv_entry *s1,*s2;
    permuts=0;
    for(i=0;i<n-1;++i){
      s1=&rec->srventry[i];
      s2=&rec->srventry[i+1];
      if (s1->priority>s2->priority){
        memcpy(&swap,s1,sizeof(swap));
        memcpy(s1,s2,sizeof(swap));
        memcpy(s2,&swap,sizeof(swap));
        permuts++;
      }
    }
  }while(permuts!=0);
}


int
_eXosip_srv_lookup (osip_transaction_t * tr, osip_message_t * sip,
                    struct osip_srv_record *record)
{
  int use_srv = 1;

  int port;

  char *host;

  osip_via_t *via;

  via = (osip_via_t *) osip_list_get (&sip->vias, 0);
  if (via == NULL || via->protocol == NULL)
    return OSIP_BADPARAMETER;

  if (MSG_IS_REQUEST (sip))
    {
      osip_route_t *route;

      osip_message_get_route (sip, 0, &route);
      if (route != NULL)
        {
          osip_uri_param_t *lr_param = NULL;

          osip_uri_uparam_get_byname (route->url, "lr", &lr_param);
          if (lr_param == NULL)
            route = NULL;
        }

      if (route != NULL)
        {
          port = 5060;
          if (route->url->port != NULL)
            {
              port = osip_atoi (route->url->port);
              use_srv = 0;
            }
          host = route->url->host;
      } else
        {
          /* search for maddr parameter */
          osip_uri_param_t *maddr_param = NULL;

          osip_uri_uparam_get_byname (sip->req_uri, "maddr", &maddr_param);
          host = NULL;
          if (maddr_param != NULL && maddr_param->gvalue != NULL)
            host = maddr_param->gvalue;

          port = 5060;
          if (sip->req_uri->port != NULL)
            {
              use_srv = 0;
              port = osip_atoi (sip->req_uri->port);
            }

          if (host == NULL)
            host = sip->req_uri->host;
        }
  } else
    {
      osip_generic_param_t *maddr;

      osip_generic_param_t *received;

      osip_generic_param_t *rport;

      osip_via_param_get_byname (via, "maddr", &maddr);
      osip_via_param_get_byname (via, "received", &received);
      osip_via_param_get_byname (via, "rport", &rport);
      if (maddr != NULL)
        host = maddr->gvalue;
      else if (received != NULL)
        host = received->gvalue;
      else
        host = via->host;

      if (via->port == NULL)
        use_srv = 0;
      if (rport == NULL || rport->gvalue == NULL)
        {
          if (via->port != NULL)
            port = osip_atoi (via->port);
          else
            port = 5060;
      } else
        port = osip_atoi (rport->gvalue);
    }

  /* check if we have an IPv4 or IPv6 address */
  if (strchr (host, ':') || (INADDR_NONE != inet_addr (host)))
    {
      return OSIP_UNKNOWN_HOST;
    }

  if (use_srv == 1)
    {
      int i;

      i = eXosip_get_srv_record (record, host, via->protocol);
      return i;
    }
  return OSIP_SUCCESS;
}

#if defined(WIN32) && !defined(_WIN32_WCE)

int
_eX_dn_expand (unsigned char *msg, unsigned char *eomorig, unsigned char *comp_dn,
               unsigned char *exp_dn, int length)
{
  unsigned char *cp;
  unsigned char *dest;
  unsigned char *eom;
  int len = -1;
  int len_copied = 0;

  dest = exp_dn;
  cp = comp_dn;
  eom = exp_dn + length;

  while (*cp != '\0')
    {
      int w = *cp;
      int comp = w & 0xc0;

      cp++;
      if (comp == 0)
        {
          if (dest != exp_dn)
            {
              if (dest >= eom)
                return -1;
              *dest = '.';
              dest++;
            }
          if (dest + w >= eom)
            return -1;
          len_copied++;
          len_copied = len_copied + w;
          w--;
          for (; w >= 0; w--, cp++)
            {
              if ((*cp == '.') || (*cp == '\\'))
                {
                  if (dest + w + 2 >= eom)
                    return -1;
                  *dest = '\\';
                  dest++;
                }
              *dest = *cp;
              dest++;
              if (cp >= eomorig)
                return -1;
            }
      } else if (comp == 0xc0)
        {
          if (len < 0)
            len = cp - comp_dn + 1;
          cp = msg + (((w & 0x3f) << 8) | (*cp & 0xff));
          if (cp < msg || cp >= eomorig)
            return -1;
          len_copied = len_copied + 2;
          if (len_copied >= eomorig - msg)
            return -1;
      } else
        return -1;
    }

  *dest = '\0';

  if (len < 0)
    len = cp - comp_dn;
  return len;
}

int
eXosip_get_naptr (char *domain, char *protocol, char *srv_record, int max_length)
{
  char zone[1024];

  PDNS_RECORD answer, tmp;      /* answer buffer from nameserver */

  int n;

  char tr[100];

  memset (srv_record, 0, max_length);
  if (domain == NULL || protocol == NULL)
    return OSIP_BADPARAMETER;

  if (strlen (domain) + strlen (protocol) > 1000)
    return OSIP_BADPARAMETER;

  if (strlen (protocol) >= 100)
    return OSIP_BADPARAMETER;
  snprintf (tr, 100, protocol);
  osip_tolower (tr);

  snprintf (zone, 1024, "%s", domain);

  OSIP_TRACE (osip_trace
              (__FILE__, __LINE__, OSIP_INFO2, NULL,
               "About to ask for '%s NAPTR'\n", zone));

  if (DnsQuery (zone, DNS_TYPE_NAPTR, DNS_QUERY_STANDARD, NULL, &answer, NULL) !=
      0)
    {
      return OSIP_UNKNOWN_HOST;
    }

  n = 0;
  for (tmp = answer; tmp != NULL; tmp = tmp->pNext)
    {
      char *buf = (char *) &tmp->Data;

      int len;

      typedef struct
      {
        unsigned short order;
        unsigned short pref;
        char flag[256];
        char service[1024];
        char regexp[1024];
        char replacement[1024];
      } osip_naptr_t;

      osip_naptr_t anaptr;

      if (tmp->wType != DNS_TYPE_NAPTR)
        continue;

      memset (&anaptr, 0, sizeof (osip_naptr_t));

      memcpy ((void *) &anaptr.order, buf, 2);
      anaptr.order = ntohs (anaptr.order);      /* ((unsigned short)buf[0] << 8) | ((unsigned short)buf[1]); */
      buf += sizeof (unsigned short);
      memcpy ((void *) &anaptr.pref, buf, 2);
      anaptr.pref = ntohs (anaptr.pref);        /* ((unsigned short)buf[0] << 8) | ((unsigned short)buf[1]); */
      buf += sizeof (unsigned short);

      len = *buf;
      if (len < 0 || len > 255)
        break;
      buf++;
      strncpy (anaptr.flag, buf, len);
      anaptr.flag[len] = '\0';
      buf += len;

      len = *buf;
      if (len < 0 || len > 1023)
        break;
      buf++;
      strncpy (anaptr.service, buf, len);
      anaptr.service[len] = '\0';
      buf += len;

      len = *buf;
      if (len < 0 || len > 1023)
        break;
      buf++;
      strncpy (anaptr.regexp, buf, len);
      anaptr.regexp[len] = '\0';
      buf += len;

      len =
        _eX_dn_expand ((char *) &tmp->Data,
                       ((char *) &tmp->Data) + tmp->wDataLength, buf,
                       anaptr.replacement, 1024 - 1);

      if (len < 0)
        break;
      buf += len;

      OSIP_TRACE (osip_trace
                  (__FILE__, __LINE__, OSIP_INFO2, NULL,
                   "NAPTR %s ->%i/%i/%s/%s/%s/%s\n",
                   zone, anaptr.order, anaptr.pref, anaptr.flag,
                   anaptr.service, anaptr.regexp, anaptr.replacement));

      if (osip_strncasecmp (tr, "udp", 4) == 0
          && osip_strncasecmp (anaptr.service, "SIP+D2U", 8) == 0)
        {
          snprintf (srv_record, max_length, "%s", anaptr.replacement);
          DnsRecordListFree (answer, DnsFreeRecordList);
          return OSIP_SUCCESS;
      } else if (osip_strncasecmp (tr, "tcp", 4) == 0
                 && osip_strncasecmp (anaptr.service, "SIP+D2T", 8) == 0)
        {
          snprintf (srv_record, max_length, "%s", anaptr.replacement);
          DnsRecordListFree (answer, DnsFreeRecordList);
          return OSIP_SUCCESS;
      } else if (osip_strncasecmp (tr, "udp-tls", 8) == 0
                 && osip_strncasecmp (anaptr.service, "SIPS+D2U", 9) == 0)
        {
          snprintf (srv_record, max_length, "%s", anaptr.replacement);
          DnsRecordListFree (answer, DnsFreeRecordList);
          return OSIP_SUCCESS;
      } else if (osip_strncasecmp (tr, "tls", 4) == 0
                 && osip_strncasecmp (anaptr.service, "SIPS+D2T", 9) == 0)
        {
          snprintf (srv_record, max_length, "%s", anaptr.replacement);
          DnsRecordListFree (answer, DnsFreeRecordList);
          return OSIP_SUCCESS;
      } else if (osip_strncasecmp (tr, "sctp", 5) == 0
                 && osip_strncasecmp (anaptr.service, "SIP+D2S", 8) == 0)
        {
          snprintf (srv_record, max_length, "%s", anaptr.replacement);
          DnsRecordListFree (answer, DnsFreeRecordList);
          return OSIP_SUCCESS;
        }

      n++;
    }

  DnsRecordListFree (answer, DnsFreeRecordList);

  if (n == 0)
    return OSIP_UNKNOWN_HOST;

  OSIP_TRACE (osip_trace
              (__FILE__, __LINE__, OSIP_INFO2, NULL,
              "protocol: %s is not supported by domain %s\n", protocol, domain));
  return OSIP_SUCCESS;
}


int
eXosip_get_srv_record (struct osip_srv_record *record, char *domain,
                       char *protocol)
{
  char zone[1024];
  PDNS_RECORD answer, tmp;      /* answer buffer from nameserver */
  int n;
  char tr[100];

  if (domain == NULL || protocol == NULL)
    return OSIP_BADPARAMETER;

  memset (record, 0, sizeof (struct osip_srv_record));
  if (strlen (domain) + strlen (protocol) > 1000)
    return OSIP_BADPARAMETER;

  if (strlen (protocol) >= 100)
    return OSIP_BADPARAMETER;
  snprintf (tr, 100, protocol);
  osip_tolower (tr);
  if (eXosip.use_naptr)
     n = eXosip_get_naptr (domain, protocol, zone, sizeof (zone) - 1);
  else n = -1;
  if (n==OSIP_SUCCESS && zone=='\0')
    {
      /* protocol is not listed in NAPTR answer: not supported */
      return OSIP_UNKNOWN_HOST;
    }

  if (n != OSIP_SUCCESS)
    {
      snprintf (zone, sizeof (zone) - 1, "_sip._%s.%s", tr, domain);
      OSIP_TRACE (osip_trace
                  (__FILE__, __LINE__, OSIP_INFO2, NULL,
                   "Using locally generated SRV record %s\n", zone));
    }

  OSIP_TRACE (osip_trace
              (__FILE__, __LINE__, OSIP_INFO2, NULL,
               "About to ask for '%s IN SRV'\n", zone));

  if (DnsQuery (zone, DNS_TYPE_SRV, DNS_QUERY_STANDARD, NULL, &answer, NULL) != 0)
    {
      return OSIP_UNKNOWN_HOST;
    }

  n = 0;
  for (tmp = answer; tmp != NULL; tmp = tmp->pNext)
    {
      struct osip_srv_entry *srventry;

      DNS_SRV_DATA *data;

      if (tmp->wType != DNS_TYPE_SRV)
        continue;
      srventry = &record->srventry[n];
      data = &tmp->Data.SRV;
      snprintf (srventry->srv, sizeof (srventry->srv), "%s", data->pNameTarget);

      srventry->priority = data->wPriority;
      srventry->weight = data->wWeight;
      if (srventry->weight)
        srventry->rweight = 1 + rand () % (10000 * srventry->weight);
      else
        srventry->rweight = 0;
      srventry->port = data->wPort;

      OSIP_TRACE (osip_trace
                  (__FILE__, __LINE__, OSIP_INFO2, NULL,
                   "SRV record %s IN SRV -> %s:%i/%i/%i/%i\n",
                   zone, srventry->srv, srventry->port, srventry->priority,
                   srventry->weight, srventry->rweight));

      n++;
      if (n == 10)
        break;
    }

  DnsRecordListFree (answer, DnsFreeRecordList);

  if (n == 0)
    return OSIP_UNKNOWN_HOST;
  osip_srv_record_sort(record,n);
  snprintf (record->name, sizeof (record->name), "%s", domain);
  return OSIP_SUCCESS;
}

#elif defined(__linux)

/* the biggest packet we'll send and receive */
#if PACKETSZ > 1024
#define	MAXPACKET PACKETSZ
#else
#define	MAXPACKET 1024
#endif

/* and what we send and receive */
typedef union
{
  HEADER hdr;
  u_char buf[MAXPACKET];
}
querybuf;

#ifndef T_SRV
#define T_SRV		33
#endif

#ifndef T_NAPTR
#define T_NAPTR	35
#endif

int
eXosip_get_naptr (char *domain, char *protocol, char *srv_record, int max_length)
{
  querybuf answer;              /* answer buffer from nameserver */
  int n;
  char zone[1024];
  int ancount, qdcount;         /* answer count and query count */
  HEADER *hp;                   /* answer buffer header */
  char hostbuf[256];
  unsigned char *msg, *eom, *cp;        /* answer buffer positions */
  int dlen, type, aclass;
  long ttl;
  int answerno;
  char tr[100];

  memset (srv_record, 0, max_length);
  if (domain == NULL || protocol == NULL)
    return OSIP_BADPARAMETER;

  if (strlen (domain) + strlen (protocol) > 1000)
    return OSIP_BADPARAMETER;

  if (strlen (protocol) >= 100)
    return OSIP_BADPARAMETER;
  snprintf (tr, 100, protocol);
  osip_tolower (tr);

  snprintf (zone, 1024, "%s", domain);

  OSIP_TRACE (osip_trace
              (__FILE__, __LINE__, OSIP_INFO2, NULL,
               "About to ask for '%s IN NAPTR\n", zone));

  n = res_query (zone, C_IN, T_NAPTR, (unsigned char *) &answer, sizeof (answer));

  if (n < (int) sizeof (HEADER))
    {
      return OSIP_UNKNOWN_HOST;
    }

  /* browse message and search for DNS answers part */
  hp = (HEADER *) & answer;
  qdcount = ntohs (hp->qdcount);
  ancount = ntohs (hp->ancount);

  msg = (unsigned char *) (&answer);
  eom = (unsigned char *) (&answer) + n;
  cp = (unsigned char *) (&answer) + sizeof (HEADER);

  while (qdcount-- > 0 && cp < eom)
    {
      n = dn_expand (msg, eom, cp, (char *) hostbuf, 256);
      if (n < 0)
        {
          OSIP_TRACE (osip_trace
                      (__FILE__, __LINE__, OSIP_ERROR, NULL,
                       "Invalid SRV record answer for '%s': bad format\n", zone));
          return OSIP_UNDEFINED_ERROR;
        }
      cp += n + QFIXEDSZ;
    }


  /* browse DNS answers */
  answerno = 0;

  /* loop through the answer buffer and extract SRV records */
  while (ancount-- > 0 && cp < eom)
    {
      int len;

      typedef struct
      {
        unsigned short order;
        unsigned short pref;
        char flag[256];
        char service[1024];
        char regexp[1024];
        char replacement[1024];
      } osip_naptr_t;

      osip_naptr_t anaptr;

      n = dn_expand (msg, eom, cp, (char *) hostbuf, 256);
      if (n < 0)
        {
          OSIP_TRACE (osip_trace
                      (__FILE__, __LINE__, OSIP_ERROR, NULL,
                       "Invalid NAPTR answer for '%s': bad format\n", zone));
          return OSIP_UNDEFINED_ERROR;
        }

      cp += n;

#if defined(__NetBSD__) || defined(__OpenBSD__) ||\
defined(OLD_NAMESER) || defined(__FreeBSD__)
      type = _get_short (cp);
      cp += sizeof (u_short);
#elif defined(__APPLE_CC__)
      GETSHORT (type, cp);
#else
      NS_GET16 (type, cp);
#endif

#if defined(__NetBSD__) || defined(__OpenBSD__) ||\
defined(OLD_NAMESER) || defined(__FreeBSD__)
      aclass = _get_short (cp);
      cp += sizeof (u_short);
#elif defined(__APPLE_CC__)
      GETSHORT (aclass, cp);
#else
      NS_GET16 (aclass, cp);
#endif

#if defined(__NetBSD__) || defined(__OpenBSD__) ||\
defined(OLD_NAMESER) || defined(__FreeBSD__)
      ttl = _get_long (cp);
      cp += sizeof (u_long);
#elif defined(__APPLE_CC__)
      GETLONG (ttl, cp);
#else
      NS_GET32 (ttl, cp);
#endif

#if defined(__NetBSD__) || defined(__OpenBSD__) ||\
defined(OLD_NAMESER) || defined(__FreeBSD__)
      dlen = _get_short (cp);
      cp += sizeof (u_short);
#elif defined(__APPLE_CC__)
      GETSHORT (dlen, cp);
#else
      NS_GET16 (dlen, cp);
#endif

      if (type != T_NAPTR)
        {
          cp += dlen;
          continue;
        }

      memset (&anaptr, 0, sizeof (osip_naptr_t));

      memcpy ((void *) &anaptr.order, cp, 2);
      anaptr.order = ntohs (anaptr.order);      /*((unsigned short)cp[0] << 8) | ((unsigned short)cp[1]); */
      cp += sizeof (unsigned short);
      memcpy ((void *) &anaptr.pref, cp, 2);
      anaptr.pref = ntohs (anaptr.pref);        /* ((unsigned short)cp[0] << 8) | ((unsigned short)cp[1]); */
      cp += sizeof (unsigned short);

      len = *cp;
      cp++;
      strncpy (anaptr.flag, (char *) cp, len);
      anaptr.flag[len] = '\0';
      cp += len;

      len = *cp;
      cp++;
      strncpy (anaptr.service, (char *) cp, len);
      anaptr.service[len] = '\0';
      cp += len;

      len = *cp;
      cp++;
      strncpy (anaptr.regexp, (char *) cp, len);
      anaptr.regexp[len] = '\0';
      cp += len;

      n = dn_expand (msg, eom, cp, anaptr.replacement, 1024 - 1);

      if (n < 0)
        break;
      cp += n;

      OSIP_TRACE (osip_trace
                  (__FILE__, __LINE__, OSIP_INFO2, NULL,
                   "NAPTR %s ->%i/%i/%s/%s/%s/%s\n",
                   zone, anaptr.order, anaptr.pref, anaptr.flag,
                   anaptr.service, anaptr.regexp, anaptr.replacement));

      if (osip_strncasecmp (tr, "udp", 4) == 0
          && osip_strncasecmp (anaptr.service, "SIP+D2U", 8) == 0)
        {
          snprintf (srv_record, max_length, "%s", anaptr.replacement);
          return OSIP_SUCCESS;
      } else if (osip_strncasecmp (tr, "tcp", 4) == 0
                 && osip_strncasecmp (anaptr.service, "SIP+D2T", 8) == 0)
        {
          snprintf (srv_record, max_length, "%s", anaptr.replacement);
          return OSIP_SUCCESS;
      } else if (osip_strncasecmp (tr, "udp-tls", 8) == 0
                 && osip_strncasecmp (anaptr.service, "SIPS+D2U", 9) == 0)
        {
          snprintf (srv_record, max_length, "%s", anaptr.replacement);
          return OSIP_SUCCESS;
      } else if (osip_strncasecmp (tr, "tls", 4) == 0
                 && osip_strncasecmp (anaptr.service, "SIPS+D2T", 9) == 0)
        {
          snprintf (srv_record, max_length, "%s", anaptr.replacement);
          return OSIP_SUCCESS;
      } else if (osip_strncasecmp (tr, "sctp", 5) == 0
                 && osip_strncasecmp (anaptr.service, "SIP+D2S", 8) == 0)
        {
          snprintf (srv_record, max_length, "%s", anaptr.replacement);
          return OSIP_SUCCESS;
        }

      answerno++;
    }

  if (answerno == 0)
    return OSIP_UNKNOWN_HOST;

  OSIP_TRACE (osip_trace
              (__FILE__, __LINE__, OSIP_INFO2, NULL,
              "protocol: %s is not supported by domain %s\n", protocol, domain));
  return OSIP_SUCCESS;
}

int
eXosip_get_srv_record (struct osip_srv_record *record, char *domain,
                       char *protocol)
{
  querybuf answer;              /* answer buffer from nameserver */
  int n;
  char zone[1024];
  int ancount, qdcount;         /* answer count and query count */
  HEADER *hp;                   /* answer buffer header */
  char hostbuf[256];
  unsigned char *msg, *eom, *cp;        /* answer buffer positions */
  int dlen, type, aclass, pref, weight, port;
  long ttl;
  int answerno;
  char tr[100];

  if (domain == NULL || protocol == NULL)
    return OSIP_BADPARAMETER;

  memset (record, 0, sizeof (struct osip_srv_record));
  if (strlen (domain) + strlen (protocol) > 1000)
    return OSIP_BADPARAMETER;

  if (strlen (protocol) >= 100)
    return OSIP_BADPARAMETER;
  snprintf (tr, 100, protocol);
  osip_tolower (tr);
  if (eXosip.use_naptr)
    n = eXosip_get_naptr (domain, protocol, zone, sizeof (zone) - 1);
  else n=-1;
  if (n==OSIP_SUCCESS && zone=='\0')
    {
      /* protocol is not listed in NAPTR answer: not supported */
      return OSIP_UNKNOWN_HOST;
    }
  if (n != OSIP_SUCCESS)
    {
      snprintf (zone, sizeof (zone) - 1, "_sip._%s.%s", tr, domain);
      OSIP_TRACE (osip_trace
                  (__FILE__, __LINE__, OSIP_INFO2, NULL,
                   "Using locally generated SRV record %s\n", zone));
    }

  OSIP_TRACE (osip_trace
              (__FILE__, __LINE__, OSIP_INFO2, NULL,
               "About to ask for '%s IN SRV'\n", zone));

  n = res_query (zone, C_IN, T_SRV, (unsigned char *) &answer, sizeof (answer));

  if (n < (int) sizeof (HEADER))
    {
      return OSIP_UNKNOWN_HOST;
    }

  /* browse message and search for DNS answers part */
  hp = (HEADER *) & answer;
  qdcount = ntohs (hp->qdcount);
  ancount = ntohs (hp->ancount);

  msg = (unsigned char *) (&answer);
  eom = (unsigned char *) (&answer) + n;
  cp = (unsigned char *) (&answer) + sizeof (HEADER);

  while (qdcount-- > 0 && cp < eom)
    {
      n = dn_expand (msg, eom, cp, (char *) hostbuf, 256);
      if (n < 0)
        {
          OSIP_TRACE (osip_trace
                      (__FILE__, __LINE__, OSIP_ERROR, NULL,
                       "Invalid SRV record answer for '%s': bad format\n", zone));
          return OSIP_UNDEFINED_ERROR;
        }
      cp += n + QFIXEDSZ;
    }


  /* browse DNS answers */
  answerno = 0;

  /* loop through the answer buffer and extract SRV records */
  while (ancount-- > 0 && cp < eom)
    {
      struct osip_srv_entry *srventry;

      n = dn_expand (msg, eom, cp, (char *) hostbuf, 256);
      if (n < 0)
        {
          OSIP_TRACE (osip_trace
                      (__FILE__, __LINE__, OSIP_ERROR, NULL,
                       "Invalid SRV record answer for '%s': bad format\n", zone));
          return OSIP_UNDEFINED_ERROR;
        }

      cp += n;


#if defined(__NetBSD__) || defined(__OpenBSD__) ||\
defined(OLD_NAMESER) || defined(__FreeBSD__)
      type = _get_short (cp);
      cp += sizeof (u_short);
#elif defined(__APPLE_CC__)
      GETSHORT (type, cp);
#else
      NS_GET16 (type, cp);
#endif

#if defined(__NetBSD__) || defined(__OpenBSD__) ||\
defined(OLD_NAMESER) || defined(__FreeBSD__)
      aclass = _get_short (cp);
      cp += sizeof (u_short);
#elif defined(__APPLE_CC__)
      GETSHORT (aclass, cp);
#else
      NS_GET16 (aclass, cp);
#endif

#if defined(__NetBSD__) || defined(__OpenBSD__) ||\
defined(OLD_NAMESER) || defined(__FreeBSD__)
      ttl = _get_long (cp);
      cp += sizeof (u_long);
#elif defined(__APPLE_CC__)
      GETLONG (ttl, cp);
#else
      NS_GET32 (ttl, cp);
#endif

#if defined(__NetBSD__) || defined(__OpenBSD__) ||\
defined(OLD_NAMESER) || defined(__FreeBSD__)
      dlen = _get_short (cp);
      cp += sizeof (u_short);
#elif defined(__APPLE_CC__)
      GETSHORT (dlen, cp);
#else
      NS_GET16 (dlen, cp);
#endif

      if (type != T_SRV)
        {
          cp += dlen;
          continue;
        }
#if defined(__NetBSD__) || defined(__OpenBSD__) ||\
defined(OLD_NAMESER) || defined(__FreeBSD__)
      pref = _get_short (cp);
      cp += sizeof (u_short);
#elif defined(__APPLE_CC__)
      GETSHORT (pref, cp);
#else
      NS_GET16 (pref, cp);
#endif

#if defined(__NetBSD__) || defined(__OpenBSD__) ||\
defined(OLD_NAMESER) || defined(__FreeBSD__)
      weight = _get_short (cp);
      cp += sizeof (u_short);
#elif defined(__APPLE_CC__)
      GETSHORT (weight, cp);
#else
      NS_GET16 (weight, cp);
#endif

#if defined(__NetBSD__) || defined(__OpenBSD__) ||\
defined(OLD_NAMESER) || defined(__FreeBSD__)
      port = _get_short (cp);
      cp += sizeof (u_short);
#elif defined(__APPLE_CC__)
      GETSHORT (port, cp);
#else
      NS_GET16 (port, cp);
#endif

      n = dn_expand (msg, eom, cp, (char *) hostbuf, 256);
      if (n < 0)
        break;
      cp += n;

      srventry = &record->srventry[answerno];
      snprintf (srventry->srv, sizeof (srventry->srv), "%s", hostbuf);

      srventry->priority = pref;
      srventry->weight = weight;
      if (weight)
        srventry->rweight = 1 + random () % (10000 * srventry->weight);
      else
        srventry->rweight = 0;
      srventry->port = port;

      OSIP_TRACE (osip_trace
                  (__FILE__, __LINE__, OSIP_INFO2, NULL,
                   "SRV record %s IN SRV -> %s:%i/%i/%i/%i\n",
                   zone, srventry->srv, srventry->port, srventry->priority,
                   srventry->weight, srventry->rweight));

      answerno++;
      if (answerno == 10)
        break;
    }

  if (answerno == 0)
    return OSIP_UNKNOWN_HOST;
  osip_srv_record_sort(record,answerno);
  snprintf (record->name, sizeof (record->name), "%s", domain);
  return OSIP_SUCCESS;
}


#else

int
eXosip_get_srv_record (struct osip_srv_record *record, char *domain,
                       char *protocol)
{
  return OSIP_UNDEFINED_ERROR;
}

#endif

#endif
