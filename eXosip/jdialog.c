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

#include <eXosip.h>

extern eXosip_t eXosip;

void eXosip_dialog_set_state(eXosip_dialog_t *jd, int state)
{
  jd->d_STATE = state;
}

int eXosip_dialog_find(int jid, eXosip_call_t **jc, eXosip_dialog_t **jd)
{
  for (*jc=eXosip.j_calls; *jc!=NULL; *jc=(*jc)->next)
    {
      for (*jd=(*jc)->c_dialogs; *jd!=NULL; *jd=(*jd)->next)
	{
	  if ((*jd)->d_id==jid)
	    {
	      return 0;
	    }
	}
    }
  *jd = NULL;
  *jc = NULL;
  return -1;
}

int eXosip_dialog_set_200ok(eXosip_dialog_t *jd, sip_t *_200Ok)
{
  int i;
  if (jd==NULL) return -1;
  i = msg_clone(_200Ok, &(jd->d_200Ok));
  if (i!=0) {
    eXosip_dialog_free(jd);
    return -1;
  }
  return 0;
}


int eXosip_dialog_init_as_uac(eXosip_dialog_t **_jd, sip_t *_200Ok)
{
  int i;
  eXosip_dialog_t *jd;
  *_jd = NULL;
  jd = (eXosip_dialog_t *) smalloc(sizeof(eXosip_dialog_t));
  jd->d_id  = -1; /* not yet available to user */
  jd->d_STATE = JD_EMPTY;

  i = dialog_init_as_uac(&(jd->d_dialog), _200Ok);
  if (i!=0)
    {
      sfree(jd);
      return -1;
    }

  jd->media_lines = (list_t*) smalloc(sizeof(list_t));
  list_init(jd->media_lines);

  jd->d_timer = time(NULL);
  jd->d_200Ok = NULL;
  jd->d_ack   = NULL;
  jd->next    = NULL;
  jd->parent  = NULL;
  jd->d_out_trs = (list_t*) smalloc(sizeof(list_t));
  list_init(jd->d_out_trs);
  jd->d_inc_trs = (list_t*) smalloc(sizeof(list_t));
  list_init(jd->d_inc_trs);

  *_jd = jd;
  return 0;
}

int eXosip_dialog_init_as_uas(eXosip_dialog_t **_jd, sip_t *_invite, sip_t *_200Ok)
{
  int i;
  eXosip_dialog_t *jd;
  *_jd = NULL;
  jd = (eXosip_dialog_t *) smalloc(sizeof(eXosip_dialog_t));
  jd->d_id  = -1; /* not yet available to user */
  jd->d_STATE = JD_EMPTY;
  i = dialog_init_as_uas(&(jd->d_dialog), _invite, _200Ok);
  if (i!=0)
    {
      sfree(jd);
      return -1;
    }

  jd->media_lines = (list_t*) smalloc(sizeof(list_t));
  list_init(jd->media_lines);

  jd->d_timer = time(NULL);
  jd->d_200Ok = NULL;
  jd->d_ack   = NULL;
  jd->next    = NULL;
  jd->parent  = NULL;
  jd->d_out_trs = (list_t*) smalloc(sizeof(list_t));
  list_init(jd->d_out_trs);
  jd->d_inc_trs = (list_t*) smalloc(sizeof(list_t));
  list_init(jd->d_inc_trs);

  *_jd = jd;
  return 0;
}

void eXosip_dialog_free(eXosip_dialog_t *jd)
{
  int i;
  msg_free(jd->d_200Ok);
  msg_free(jd->d_ack);

  dialog_free(jd->d_dialog);

  while (!list_eol(jd->media_lines, 0))
    {
      char *tmp = list_get(jd->media_lines, 0);
      list_remove(jd->media_lines, 0);
      sfree(tmp);
    }

  while (!list_eol(jd->d_inc_trs, 0))
    {
      transaction_t *tr = list_get(jd->d_inc_trs, 0);
      list_remove(jd->d_inc_trs, 0);
      i = osip_remove_nist(eXosip.j_osip, tr);
      if (i!=0)
	osip_remove_ist(eXosip.j_osip, tr);
      transaction_free2(tr);
    }

  while (!list_eol(jd->d_out_trs, 0))
    {
      transaction_t *tr = list_get(jd->d_out_trs, 0);
      list_remove(jd->d_out_trs, 0);
      i = osip_remove_nict(eXosip.j_osip, tr);
      if (i!=0)
	osip_remove_ict(eXosip.j_osip, tr);
      transaction_free2(tr);
    }
  sfree(jd);
}
