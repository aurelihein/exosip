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

#include <eXosip/eXosip.h>

extern eXosip_t eXosip;

int eXosip_reg_init(eXosip_reg_t **jr, char *from, char *proxy, char *contact)
{
  *jr = (eXosip_reg_t*) osip_malloc(sizeof(eXosip_reg_t));
  if (*jr==NULL) return -1;

  (*jr)->r_id         = -1;
  (*jr)->r_reg_period = 3600;      /* delay between registration */
  (*jr)->r_aor        = osip_strdup(from);      /* sip identity */
  (*jr)->r_contact    = osip_strdup(contact);   /* sip identity */
  (*jr)->r_registrar  = osip_strdup(proxy);     /* registrar */
  (*jr)->r_realms     = NULL;      /* list of realms */
  (*jr)->r_last_tr    = NULL;

  (*jr)->next   = NULL;
  (*jr)->parent = NULL;
  return 0;
}

void eXosip_reg_free(eXosip_reg_t *jreg)
{

  osip_free(jreg->r_aor);
  osip_free(jreg->r_contact);
  osip_free(jreg->r_registrar);
  osip_free(jreg->r_realms);

  if (jreg->r_last_tr->state==IST_TERMINATED ||
      jreg->r_last_tr->state==ICT_TERMINATED ||
      jreg->r_last_tr->state== NICT_TERMINATED ||
      jreg->r_last_tr->state==NIST_TERMINATED)
    {
      OSIP_TRACE(osip_trace(__FILE__,__LINE__,OSIP_INFO1,NULL,
			    "Release a terminated transaction\n"));
      __eXosip_delete_jinfo(jreg->r_last_tr);
      if (jreg->r_last_tr!=NULL)
	osip_list_add(eXosip.j_transactions, jreg->r_last_tr, 0);
    }
  else
    {
      OSIP_TRACE(osip_trace(__FILE__,__LINE__,OSIP_INFO1,NULL,
			    "Release a non-terminated transaction\n"));
      __eXosip_delete_jinfo(jreg->r_last_tr);
      if (jreg->r_last_tr!=NULL)
	osip_list_add(eXosip.j_transactions, jreg->r_last_tr, 0);
    }
  
  osip_free(jreg);
}
