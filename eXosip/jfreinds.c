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

#ifndef EXOSIP_ETC_DIR
#define EXOSIP_ETC_DIR ".eXosip"
#endif

#ifndef EXOSIP_ADDFREINDS_SH
#define EXOSIP_ADDFREINDS_SH "eXosip_addfreind.sh"
#endif

void freinds_add(char *nickname, char *home,
		 char *work, char *email, char *e164)
{
  char command[256];
  char *tmp = command;
  int length = 0;
  if (nickname!=NULL)
    length = strlen(nickname);

  if (home!=NULL)
    length = length + strlen(home);
  else
    return; /* MUST be set */
  if (work!=NULL)
    length = length + strlen(work);
  if (email!=NULL)
    length = length + strlen(email);
  if (e164!=NULL)
    length = length + strlen(e164);
  length = length + strlen(EXOSIP_ETC_DIR);

  length = length + strlen("/jm_contact");
  if (length>235) /* leave some room for SPACEs and \r\n */
    return ;

  sprintf(tmp , "%s %s/jm_contact", EXOSIP_ADDFREINDS_SH, EXOSIP_ETC_DIR);

  tmp = tmp + strlen(tmp);
  if (nickname!=NULL)
    sprintf(tmp , " %s", nickname);
  else
    sprintf(tmp , " \"\"");

  tmp = tmp + strlen(tmp);
  if (home!=NULL)
    sprintf(tmp , " %s", home);
  else
    sprintf(tmp , " \"\"");

  tmp = tmp + strlen(tmp);
  if (work!=NULL)
    sprintf(tmp , " %s", work);
  else
    sprintf(tmp , " \"\"");

  tmp = tmp + strlen(tmp);
  if (email!=NULL)
    sprintf(tmp , " %s", email);
  else
    sprintf(tmp , " \"\"");

  tmp = tmp + strlen(tmp);
  if (e164!=NULL)
    sprintf(tmp , " %s", e164);
  else
    sprintf(tmp , " \"\"");

  system(command);  
}



int
jfreind_get_and_set_next_token (char **dest, char *buf, char **next)
{
  char *end;
  char *start;

  *next = NULL;

  /* find first non space and tab element */
  start = buf;
  while (((*start == ' ') || (*start == '\t')) && (*start != '\0')
         && (*start != '\r') && (*start != '\n') )
    start++;
  end = start+1;
  while ((*end != '\0') && (*end != '\r') && (*end != '\n')
         && (*end != '\t') && (*end != '|'))
    end++;
  
  if ((*end == '\r') || (*end == '\n'))
    /* we should continue normally only if this is the separator asked! */
    return -1;
  if (end == start)
    return -1;                  /* empty value (or several space!) */

  *dest = smalloc (end - (start) + 1);
  sstrncpy (*dest, start, end - start);

  *next = end + 1;   /* return the position right after the separator
 */
  return 0;
}


int jfreind_init(jfreind_t **fr, char *ch)
{
  char *next;
  int i;

  *fr = (jfreind_t *)smalloc(sizeof(jfreind_t));
  if (*fr==NULL) return -1;

  i = jfreind_get_and_set_next_token(&((*fr)->f_nick), ch, &next);
  if (i != 0)
    goto jf_error1;
  sclrspace ((*fr)->f_nick);
  ch = next;

  i = jfreind_get_and_set_next_token(&((*fr)->f_home), next, &next);
  if (i != 0)
    goto jf_error2;
  sclrspace ((*fr)->f_home);
  ch = next;

  i = jfreind_get_and_set_next_token(&((*fr)->f_work), ch, &next);
  if (i != 0)
    goto jf_error3;
  sclrspace ((*fr)->f_work);
  ch = next;

  i = jfreind_get_and_set_next_token(&((*fr)->f_email), ch, &next);
  if (i != 0)
    goto jf_error4;
  sclrspace ((*fr)->f_email);

  (*fr)->f_e164 = sgetcopy(next);
  sclrspace ((*fr)->f_e164);

  return 0;

 jf_error4:
  sfree((*fr)->f_work);
 jf_error3:
  sfree((*fr)->f_home);
 jf_error2:
  sfree((*fr)->f_nick);
 jf_error1:
  sfree(*fr);
  *fr = NULL;
  return -1;
}


void
jfreind_unload()
{
  jfreind_t *fr;
  if (eXosip.j_freinds==NULL) return;
  for (fr=eXosip.j_freinds; fr!=NULL; fr=fr->next)
    {
    REMOVE_ELEMENT(eXosip.j_freinds,fr);
    sfree(fr->f_nick);
    sfree(fr->f_home);
    sfree(fr->f_work);
    sfree(fr->f_email);
    sfree(fr->f_e164);
    sfree(fr);
    }

  sfree(eXosip.j_freinds);
  eXosip.j_freinds=NULL;
  return;
}

int
jfreind_load()
{
  FILE *file;
  char *s; 
  jfreind_t *fr;
  int pos;
  char *home;
  char filename[255];

  jfreind_unload();
  home = getenv("HOME");
  sprintf(filename, "%s/%s/%s", home, EXOSIP_ETC_DIR, "jm_contact");
  

  file = fopen(filename, "r");
  if (file==NULL) return -1;
  s = (char *)smalloc(255*sizeof(char));
  pos = 0;
  while (NULL!=fgets(s, 254, file))
    {
      char *tmp = s;
      while (*tmp!='\0' && *tmp!=' ') tmp++;
      while (*tmp!='\0' && *tmp==' ') tmp++;
      while (*tmp!='\0' && *tmp!=' ') tmp++;
      tmp++; /* first usefull characters */
      pos++;

      jfreind_init(&fr, tmp);
      if (fr!=NULL)
	{ ADD_ELEMENT(eXosip.j_freinds, fr); }
    }
  sfree(s);
  fclose(file);

  return 0; /* ok */
}

char *
jfreind_get_home(int fid)
{
  jfreind_t *fr;
  for (fr = eXosip.j_freinds; fr!=NULL ; fr=fr->next)
    {
      if (fid==0)
	return sgetcopy(fr->f_home);
      fid--;
    }
  return NULL;
}
