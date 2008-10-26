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

#ifndef MINISIZE

#include "eXosip2.h"

extern eXosip_t eXosip;

int
_eXosip_pub_update (eXosip_pub_t ** pub, osip_transaction_t * tr,
                    osip_message_t * answer)
{
  eXosip_pub_t *jpub;

  *pub = NULL;

  for (jpub = eXosip.j_pub; jpub != NULL; jpub = jpub->next)
    {
      if (jpub->p_last_tr == NULL)
        {                       /*bug? */
      } else if (tr == jpub->p_last_tr)
        {
          /* update the sip_etag parameter */
          if (answer == NULL)
            {                   /* bug? */
          } else if (MSG_IS_STATUS_2XX (answer))
            {
              osip_header_t *sip_etag = NULL;

              osip_message_header_get_byname (answer, "SIP-ETag", 0, &sip_etag);
              if (sip_etag != NULL && sip_etag->hvalue != NULL)
                snprintf (jpub->p_sip_etag, 64, "%s", sip_etag->hvalue);
            }
          *pub = jpub;
          return OSIP_SUCCESS;
        }
    }
  return OSIP_NOTFOUND;
}

int
_eXosip_pub_find_by_aor (eXosip_pub_t ** pub, const char *aor)
{
  eXosip_pub_t *jpub;
  *pub = NULL;

  for (jpub = eXosip.j_pub; jpub != NULL; jpub = jpub->next)
    {
      if (osip_strcasecmp (aor, jpub->p_aor) == 0)
        {
          *pub = jpub;
          return OSIP_SUCCESS;
        }
    }
  return OSIP_NOTFOUND;
}

int
_eXosip_pub_find_by_tid (eXosip_pub_t ** pjp, int tid)
{
  eXosip_pub_t *pub = eXosip.j_pub;

  *pjp = NULL;
  while (pub)
    {
      if (pub->p_last_tr && pub->p_last_tr->transactionid == tid)
        {
          *pjp = pub;
          return OSIP_SUCCESS;
        }

      pub = pub->next;
    }

  return OSIP_NOTFOUND;
}

int
_eXosip_pub_init (eXosip_pub_t ** pub, const char *aor, const char *exp)
{
  static int p_id;
  eXosip_pub_t *jpub;

  if (p_id > 1000000)           /* keep it non-negative */
    p_id = 0;

  *pub = NULL;

  jpub = (eXosip_pub_t *) osip_malloc (sizeof (eXosip_pub_t));
  if (jpub == 0)
    return OSIP_NOMEM;
  memset (jpub, 0, sizeof (eXosip_pub_t));
  snprintf (jpub->p_aor, 256, "%s", aor);

  jpub->p_period = atoi (exp);
  jpub->p_id = ++p_id;

  *pub = jpub;
  return OSIP_SUCCESS;
}

void
_eXosip_pub_free (eXosip_pub_t * pub)
{
  if (pub->p_last_tr != NULL)
    {
      if (pub->p_last_tr != NULL && pub->p_last_tr->orig_request != NULL
          && pub->p_last_tr->orig_request->call_id != NULL
          && pub->p_last_tr->orig_request->call_id->number != NULL)
        _eXosip_delete_nonce (pub->p_last_tr->orig_request->call_id->number);

      osip_list_add (&eXosip.j_transactions, pub->p_last_tr, 0);
    }
  osip_free (pub);
}

#endif
