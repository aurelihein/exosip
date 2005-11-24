#ifndef _JMAINCONFIG_H_
#define _JMAINCONFIG_H_ 1
/*
 * josua - Jack's open sip User Agent
 *
 * Copyright (C) 2002,2003   Aymeric Moizard <jack@atosc.org>
 *
 * This is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2,
 * or (at your option) any later version.
 *
 * This is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with dpkg; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */


typedef struct _main_config_t
{
  char config_file[256];        /* -f <config file>   */
  char identity_file[256];      /* -I <identity file> */
  char nat_address[256];        /* -c <nat address>  */
  char log_file[256];           /* -L <log file>      */
  int debug_level;              /* -d <verbose level>   */
  int port;                     /* -p <SIP port>  default is 5060 */
  char identity[256];           /* -i <from url>  local identity */

  /* the application can command to make a new call */
  char to[255];                 /* -t <sipurl to call>!  */
  char route[256];              /* -r <sipurl for route> */
  char subject[256];            /* -s <subject>          */
  int proto;                    /* -t 0 (udp) 1 (tcp) 2 (tls) */

  char service_route[2048];
} main_config_t;

extern main_config_t cfg;


#endif
