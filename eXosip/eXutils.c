/*
  eXosip - This is the eXtended osip library.
  Copyright (C) 2002, 2003  Aymeric MOIZARD  - jack@atosc.org
  
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

#ifdef WIN32

/* You need the Platform SDK to compile this. */
#include <Windows.h>
#include <Iphlpapi.h>

int
ppl_dns_get_local_fqdn (char **servername, char **serverip,
			char **netmask, unsigned int WIN32_interface)
{
	unsigned int pos;

	*servername = NULL; /* no name on win32? */
	*serverip   = NULL;
	*netmask    = NULL;

	/* First, try to get the interface where we should listen */
	{
		DWORD size_of_iptable = 0;
		PMIB_IPADDRTABLE ipt;
		PMIB_IFROW ifrow;

		if (GetIpAddrTable(NULL, &size_of_iptable, TRUE) == ERROR_INSUFFICIENT_BUFFER)
		{
			ifrow = (PMIB_IFROW) _alloca (sizeof(MIB_IFROW));
			ipt = (PMIB_IPADDRTABLE) _alloca (size_of_iptable);
			if (ifrow==NULL || ipt==NULL)
			{
				/* not very usefull to continue */
				OSIP_TRACE (osip_trace (__FILE__, __LINE__, OSIP_INFO4, NULL,
					"ERROR alloca failed\r\n"));
				return -1;
			}

			if (!GetIpAddrTable(ipt, &size_of_iptable, TRUE))
			{
				/* look for the best public interface */

				for (pos=0; pos < ipt->dwNumEntries && *netmask==NULL ; ++pos)
				{
					/* index is */
					struct in_addr addr;
					struct in_addr mask;
					ifrow->dwIndex = ipt->table[pos].dwIndex;
					if (GetIfEntry(ifrow) == NO_ERROR)
					{
						switch(ifrow->dwType)
						{
						case MIB_IF_TYPE_LOOPBACK:
						  /*	break; */
						case MIB_IF_TYPE_ETHERNET:
						default:
							addr.s_addr = ipt->table[pos].dwAddr;
							mask.s_addr = ipt->table[pos].dwMask;
							if (ipt->table[pos].dwIndex == WIN32_interface)
							{
								*servername = NULL; /* no name on win32? */
								*serverip   = osip_strdup(inet_ntoa(addr));
								*netmask    = osip_strdup(inet_ntoa(mask));
								OSIP_TRACE (osip_trace (__FILE__, __LINE__, OSIP_INFO4, NULL,
									"Interface ethernet: %s/%s\r\n", *serverip, *netmask));
								break;
							}
						}
					}
				}
			}
		}
	}

	if (*serverip==NULL || *netmask==NULL)
	{
		OSIP_TRACE (osip_trace (__FILE__, __LINE__, OSIP_WARNING, NULL,
			"ERROR No network interface found\r\n"));
		return -1;
	}

	return 0;
}

void
eXosip_guess_ip_for_via (char *alocalip)
{
	/* w2000 and W95/98 */
	unsigned long  best_interface_index;
	DWORD hr;

	/* NT4 (sp4 only?) */
	PMIB_IPFORWARDTABLE ipfwdt;
	DWORD siz_ipfwd_table = 0;
	unsigned int ipf_cnt;

	alocalip[0] = '\0';
	best_interface_index = -1;
	/* w2000 and W95/98 only */
	hr = GetBestInterface(inet_addr("217.12.3.11"),&best_interface_index);
	if (hr)
	{
		LPVOID lpMsgBuf;
		FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER |
			FORMAT_MESSAGE_FROM_SYSTEM |
			FORMAT_MESSAGE_IGNORE_INSERTS,
			NULL,
			hr,
			MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
			(LPSTR) &lpMsgBuf, 0, NULL);

		OSIP_TRACE (osip_trace (__FILE__, __LINE__, OSIP_INFO4, NULL,
					 "GetBestInterface: %s\r\n", lpMsgBuf));
		best_interface_index = -1;
	}

	if (best_interface_index != -1)
	{ /* probably W2000 or W95/W98 */
		char *servername;
		char *serverip;
		char *netmask;
		OSIP_TRACE (osip_trace (__FILE__, __LINE__, OSIP_INFO4, NULL,
					 "Default Interface found %i\r\n", best_interface_index));

		if (0 == ppl_dns_get_local_fqdn(&servername, &serverip, &netmask,
						best_interface_index))
		{
			osip_strncpy(alocalip, serverip, strlen(serverip));
			osip_free(servername);
			osip_free(serverip);
			osip_free(netmask);
			return;
		}
		return;
	}


	if (!GetIpForwardTable(NULL, &siz_ipfwd_table, FALSE) == ERROR_INSUFFICIENT_BUFFER
		|| !(ipfwdt = (PMIB_IPFORWARDTABLE) alloca (siz_ipfwd_table)))
	{
		OSIP_TRACE (osip_trace (__FILE__, __LINE__, OSIP_INFO4, NULL,
			"Allocation error\r\n"));
		return ;
	}


	/* NT4 (sp4 support only?) */
	if (!GetIpForwardTable(ipfwdt, &siz_ipfwd_table, FALSE))
	{
		for (ipf_cnt = 0; ipf_cnt < ipfwdt->dwNumEntries; ++ipf_cnt) 
		{
			if (ipfwdt->table[ipf_cnt].dwForwardDest == 0)
			{ /* default gateway found */
				char *servername;
				char *serverip;
				char *netmask;
				OSIP_TRACE (osip_trace (__FILE__, __LINE__, OSIP_INFO4, NULL,
					"Default Interface found %i\r\n", ipfwdt->table[ipf_cnt].dwForwardIfIndex));

				if (0 ==  ppl_dns_get_local_fqdn(&servername,
								 &serverip,
								 &netmask,
								 ipfwdt->table[ipf_cnt].dwForwardIfIndex))
				{
					osip_strncpy(alocalip, serverip, strlen(serverip));
					osip_free(servername);
					osip_free(serverip);
					osip_free(netmask);
					return ;
				}
				return ;
			}
		}

	}
	/* no default gateway interface found */
	return ;
}


#else /* sun, *BSD, linux, and other? */


#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <sys/ioctl.h>
#include <net/route.h>
#include <net/if.h>
#include <unistd.h>

#include <stdio.h>

#ifndef SIOCGIFCOUNT
#define SIOCGIFCOUNT 0x8935
#endif

char all_interface[16];

/* This is a portable way to find the default gateway.
 * The ip of the default interface is returned.
 */
void
eXosip_guess_ip_for_via (char *alocalip)
{
  unsigned int len;
  int sock_rt, on=1;
  struct sockaddr_in iface_out;
  struct sockaddr_in remote;
  
  alocalip[0] = '\0';
  memset(&remote, 0, sizeof(struct sockaddr_in));
  remote.sin_family = AF_INET;
  /*  remote.sin_addr.s_addr = inet_addr("217.12.3.11"); */
  remote.sin_addr.s_addr = inet_addr("217.12.3.11");
  remote.sin_port = htons(11111);
  
  memset(&iface_out, 0, sizeof(iface_out));
  sock_rt = socket(AF_INET, SOCK_DGRAM, 0 );
  
  if (setsockopt(sock_rt, SOL_SOCKET, SO_BROADCAST, &on, sizeof(on))
      == -1) {
    perror("DEBUG: [get_output_if] setsockopt(SOL_SOCKET, SO_BROADCAST");
    close(sock_rt);
    return ;
  }
  
  if (connect(sock_rt, (struct sockaddr*)&remote, sizeof(struct sockaddr_in))
      == -1 ) {
    perror("DEBUG: [get_output_if] connect");
    close(sock_rt);
    return ;
  }
  
  len = sizeof(iface_out);
  if (getsockname(sock_rt, (struct sockaddr *)&iface_out, &len) == -1 ) {
    perror("DEBUG: [get_output_if] getsockname");
    close(sock_rt);
    return ;
  }
  close(sock_rt);
  if (iface_out.sin_addr.s_addr == 0)
    { /* what is this case?? */
      return ;
    }

  strcpy(alocalip, inet_ntoa(iface_out.sin_addr));
  return;
}

#endif
