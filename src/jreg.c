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

#include "eXosip2.h"

#include <osipparser2/osip_md5.h>

/* TAKEN from rcf2617.txt */
#define HASHLEN 16
typedef char HASH[HASHLEN];
#define HASHHEXLEN 32
typedef char HASHHEX[HASHHEXLEN + 1];

void CvtHex (HASH Bin, HASHHEX Hex);

extern eXosip_t eXosip;

int
eXosip_reg_init (eXosip_reg_t ** jr, const char *from, const char *proxy,
                 const char *contact)
{
  static int r_id;

  *jr = (eXosip_reg_t *) osip_malloc (sizeof (eXosip_reg_t));
  if (*jr == NULL)
    return OSIP_NOMEM;

  if (r_id > 1000000)           /* keep it non-negative */
    r_id = 0;

  memset (*jr, '\0', sizeof (eXosip_reg_t));

  (*jr)->r_id = ++r_id;
  (*jr)->r_reg_period = 3600;   /* delay between registration */
  (*jr)->r_aor = osip_strdup (from);    /* sip identity */
  if ((*jr)->r_aor == NULL)
    {
      osip_free (*jr);
      *jr = NULL;
      return OSIP_NOMEM;
    }
  (*jr)->r_contact = osip_strdup (contact);     /* sip identity */
  (*jr)->r_registrar = osip_strdup (proxy);     /* registrar */
  if ((*jr)->r_registrar == NULL)
    {
      osip_free ((*jr)->r_contact);
      osip_free ((*jr)->r_aor);
      osip_free (*jr);
      *jr = NULL;
      return OSIP_NOMEM;
    }

  {
    osip_MD5_CTX Md5Ctx;
    HASH hval;
    HASHHEX key_line;
    char localip[128];
    char firewall_ip[65];
    char firewall_port[10];

    memset (localip, '\0', sizeof (localip));
    memset (firewall_ip, '\0', sizeof (firewall_ip));
    memset (firewall_port, '\0', sizeof (firewall_port));

    eXosip_guess_localip (AF_INET, localip, 128);
    if (eXosip.eXtl != NULL && eXosip.eXtl->tl_get_masquerade_contact != NULL)
      {
        eXosip.eXtl->tl_get_masquerade_contact (firewall_ip, sizeof (firewall_ip),
                                                firewall_port,
                                                sizeof (firewall_port));
      }

    osip_MD5Init (&Md5Ctx);
    osip_MD5Update (&Md5Ctx, (unsigned char *) from, strlen (from));
    osip_MD5Update (&Md5Ctx, (unsigned char *) ":", 1);
    osip_MD5Update (&Md5Ctx, (unsigned char *) proxy, strlen (proxy));
    osip_MD5Update (&Md5Ctx, (unsigned char *) ":", 1);
    osip_MD5Update (&Md5Ctx, (unsigned char *) localip, strlen (localip));
    osip_MD5Update (&Md5Ctx, (unsigned char *) ":", 1);
    osip_MD5Update (&Md5Ctx, (unsigned char *) firewall_ip, strlen (firewall_ip));
    osip_MD5Update (&Md5Ctx, (unsigned char *) ":", 1);
    osip_MD5Update (&Md5Ctx, (unsigned char *) firewall_port,
                    strlen (firewall_port));
    osip_MD5Final ((unsigned char *) hval, &Md5Ctx);
    CvtHex (hval, key_line);

    osip_strncpy ((*jr)->r_line, key_line, sizeof ((*jr)->r_line) - 1);
  }

  return OSIP_SUCCESS;
}

void
eXosip_reg_free (eXosip_reg_t * jreg)
{

  osip_free (jreg->r_aor);
  osip_free (jreg->r_contact);
  osip_free (jreg->r_registrar);

  if (jreg->r_last_tr != NULL)
    {
      if (jreg->r_last_tr != NULL && jreg->r_last_tr->orig_request != NULL
          && jreg->r_last_tr->orig_request->call_id != NULL
          && jreg->r_last_tr->orig_request->call_id->number != NULL)
        _eXosip_delete_nonce (jreg->r_last_tr->orig_request->call_id->number);

      if (jreg->r_last_tr->state == IST_TERMINATED ||
          jreg->r_last_tr->state == ICT_TERMINATED ||
          jreg->r_last_tr->state == NICT_TERMINATED ||
          jreg->r_last_tr->state == NIST_TERMINATED)
        {
          OSIP_TRACE (osip_trace (__FILE__, __LINE__, OSIP_INFO1, NULL,
                                  "Release a terminated transaction\n"));
          __eXosip_delete_jinfo (jreg->r_last_tr);
          if (jreg->r_last_tr != NULL)
            osip_list_add (&eXosip.j_transactions, jreg->r_last_tr, 0);
      } else
        {
          OSIP_TRACE (osip_trace (__FILE__, __LINE__, OSIP_INFO1, NULL,
                                  "Release a non-terminated transaction\n"));
          __eXosip_delete_jinfo (jreg->r_last_tr);
          if (jreg->r_last_tr != NULL)
            osip_list_add (&eXosip.j_transactions, jreg->r_last_tr, 0);
        }
    }

  osip_free (jreg);
}

int
_eXosip_reg_find (eXosip_reg_t ** reg, osip_transaction_t * tr)
{
  eXosip_reg_t *jreg;

  *reg = NULL;
  if (tr == NULL)
    return OSIP_BADPARAMETER;

  for (jreg = eXosip.j_reg; jreg != NULL; jreg = jreg->next)
    {
      if (jreg->r_last_tr == tr)
        {
          *reg = jreg;
          return OSIP_SUCCESS;
        }
    }
  return OSIP_NOTFOUND;
}


int
eXosip_reg_find_id (eXosip_reg_t ** reg, int rid)
{
  eXosip_reg_t *jreg;

  *reg = NULL;
  if (rid <= 0)
    return OSIP_BADPARAMETER;

  for (jreg = eXosip.j_reg; jreg != NULL; jreg = jreg->next)
    {
      if (jreg->r_id == rid)
        {
          *reg = jreg;
          return OSIP_SUCCESS;
        }
    }
  return OSIP_NOTFOUND;
}
