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

int eXosip_call_find(int cid, eXosip_call_t **jc)
{
  for (*jc=eXosip.j_calls; *jc!=NULL; *jc=(*jc)->next)
    {
      if ((*jc)->c_id==cid)
	{
	  return 0;
	}
    }
  *jc = NULL;
  return -1;
}

osip_transaction_t *
eXosip_find_last_invite(eXosip_call_t *jc, eXosip_dialog_t *jd )
{
  osip_transaction_t *inc_tr;
  osip_transaction_t *out_tr;
  inc_tr = eXosip_find_last_inc_invite(jc, jd);
  out_tr = eXosip_find_last_inc_invite(jc, jd);
  if (inc_tr==NULL)
    return out_tr;
  if (out_tr==NULL)
    return inc_tr;

  if (inc_tr->birth_time>out_tr->birth_time)
    return inc_tr;
  return out_tr;
}

osip_transaction_t *
eXosip_find_last_inc_invite(eXosip_call_t *jc, eXosip_dialog_t *jd )
{
  osip_transaction_t *inc_tr;
  int pos;
  inc_tr = NULL;
  pos=0;
  if (jd!=NULL)
    {
      while (!osip_list_eol(jd->d_inc_trs, pos))
	{
	  inc_tr = osip_list_get(jd->d_inc_trs, pos);
	  if (0==strcmp(inc_tr->cseq->method, "INVITE"))
	    break;
	  else inc_tr = NULL;
	  pos++;
	}
    }
  else
    inc_tr = NULL;

  if (inc_tr==NULL)
    return jc->c_inc_tr; /* can be NULL */

  return inc_tr;
}

osip_transaction_t *
eXosip_find_last_out_invite(eXosip_call_t *jc, eXosip_dialog_t *jd )
{
  osip_transaction_t *out_tr;
  int pos;
  out_tr = NULL;
  pos=0;
  if (jd==NULL && jc==NULL) return NULL;

  if (jd!=NULL)
    {
      while (!osip_list_eol(jd->d_out_trs, pos))
	{
	  out_tr = osip_list_get(jd->d_out_trs, pos);
	  if (0==strcmp(out_tr->cseq->method, "INVITE"))
	    break;
	  else out_tr = NULL;
	  pos++;
	}
    }

  if (out_tr==NULL)
    return jc->c_out_tr; /* can be NULL */

  return out_tr;
}

osip_transaction_t *
eXosip_find_last_inc_bye(eXosip_call_t *jc, eXosip_dialog_t *jd )
{
  osip_transaction_t *inc_tr;
  int pos;
  inc_tr = NULL;
  pos=0;
  if (jd!=NULL)
    {
      while (!osip_list_eol(jd->d_inc_trs, pos))
	{
	  inc_tr = osip_list_get(jd->d_inc_trs, pos);
	  if (0==strcmp(inc_tr->cseq->method, "BYE"))
	    return inc_tr;
	  pos++;
	}
    }

  return NULL;
}

osip_transaction_t *
eXosip_find_last_out_bye(eXosip_call_t *jc, eXosip_dialog_t *jd )
{
  osip_transaction_t *out_tr;
  int pos;
  out_tr = NULL;
  pos=0;
  if (jd!=NULL)
    {
      while (!osip_list_eol(jd->d_out_trs, pos))
	{
	  out_tr = osip_list_get(jd->d_out_trs, pos);
	  if (0==strcmp(out_tr->cseq->method, "BYE"))
	    return out_tr;
	  pos++;
	}
    }

  return NULL;
}

osip_transaction_t *
eXosip_find_last_inc_refer(eXosip_call_t *jc, eXosip_dialog_t *jd )
{
  osip_transaction_t *inc_tr;
  int pos;
  inc_tr = NULL;
  pos=0;
  if (jd!=NULL)
    {
      while (!osip_list_eol(jd->d_inc_trs, pos))
	{
	  inc_tr = osip_list_get(jd->d_inc_trs, pos);
	  if (0==strcmp(inc_tr->cseq->method, "REFER"))
	    return inc_tr;
	  pos++;
	}
    }

  return NULL;
}

osip_transaction_t *
eXosip_find_last_out_refer(eXosip_call_t *jc, eXosip_dialog_t *jd )
{
  osip_transaction_t *out_tr;
  int pos;
  out_tr = NULL;
  pos=0;
  if (jd!=NULL)
    {
      while (!osip_list_eol(jd->d_out_trs, pos))
	{
	  out_tr = osip_list_get(jd->d_out_trs, pos);
	  if (0==strcmp(out_tr->cseq->method, "REFER"))
	    return out_tr;
	  pos++;
	}
    }
  
  return NULL;
}

int
eXosip_call_init(eXosip_call_t **jc)
{
  *jc = (eXosip_call_t *)osip_malloc(sizeof(eXosip_call_t));
  if (*jc == NULL) return -1;
  memset(*jc, '\0', sizeof(eXosip_call_t));
  sdp_negotiation_ctx_init(&(*jc)->c_ctx);
  return 0;
}

void
eXosip_call_free(eXosip_call_t *jc)
{
  /* ... */

  eXosip_dialog_t *jd;

  for (jd = jc->c_dialogs; jd!=NULL; jd=jc->c_dialogs)
    {
      REMOVE_ELEMENT(jc->c_dialogs, jd);
      eXosip_dialog_free(jd);
    }

  __eXosip_delete_jinfo(jc->c_inc_tr);
  __eXosip_delete_jinfo(jc->c_out_tr);
  osip_transaction_free(jc->c_inc_tr);
  osip_transaction_free(jc->c_out_tr);

  sdp_negotiation_ctx_free(jc->c_ctx);
  osip_free(jc);

}

void
eXosip_call_set_subject(eXosip_call_t *jc, char *subject)
{
  if (jc==NULL||subject==NULL) return;
  snprintf(jc->c_subject, 99, "%s", subject);
}

