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

#include "eXosip2.h"
#include <eXosip/eXosip.h>
#include <eXosip/eXosip_cfg.h>

#include <osip2/osip_mt.h>
#include <osip2/osip_condv.h>

/* #include <osip2/global.h> */
#include <osipparser2/osip_md5.h>

/* TAKEN from rcf2617.txt */

#define HASHLEN 16
typedef char HASH[HASHLEN];
#define HASHHEXLEN 32
typedef char HASHHEX[HASHHEXLEN+1];
#define IN
#define OUT

extern eXosip_t eXosip;
extern char    *localip;

void CvtHex(IN HASH Bin,
	    OUT HASHHEX Hex)
{
  unsigned short i;
  unsigned char j;
  
  for (i = 0; i < HASHLEN; i++) {
    j = (Bin[i] >> 4) & 0xf;
    if (j <= 9)
      Hex[i*2] = (j + '0');
    else
      Hex[i*2] = (j + 'a' - 10);
    j = Bin[i] & 0xf;
    if (j <= 9)
      Hex[i*2+1] = (j + '0');
    else
      Hex[i*2+1] = (j + 'a' - 10);
  };
  Hex[HASHHEXLEN] = '\0';
}

/* calculate H(A1) as per spec */
void DigestCalcHA1(IN char * pszAlg,
		   IN char * pszUserName,
		   IN char * pszRealm,
		   IN char * pszPassword,
		   IN char * pszNonce,
		   IN char * pszCNonce,
		   OUT HASHHEX SessionKey)
{
  MD5_CTX Md5Ctx;
  HASH HA1;
  
  MD5Init(&Md5Ctx);
  MD5Update(&Md5Ctx, (unsigned char *)pszUserName, strlen(pszUserName));
  MD5Update(&Md5Ctx, (unsigned char *)":", 1);
  MD5Update(&Md5Ctx, (unsigned char *)pszRealm, strlen(pszRealm));
  MD5Update(&Md5Ctx, (unsigned char *)":", 1);
  MD5Update(&Md5Ctx, (unsigned char *)pszPassword, strlen(pszPassword));
  MD5Final((unsigned char *)HA1, &Md5Ctx);
  if ((pszAlg!=NULL)&&strcasecmp(pszAlg, "md5-sess") == 0)
    {
      MD5Init(&Md5Ctx);
      MD5Update(&Md5Ctx, (unsigned char *)HA1, HASHLEN);
      MD5Update(&Md5Ctx, (unsigned char *)":", 1);
      MD5Update(&Md5Ctx, (unsigned char *)pszNonce, strlen(pszNonce));
      MD5Update(&Md5Ctx, (unsigned char *)":", 1);
      MD5Update(&Md5Ctx, (unsigned char *)pszCNonce, strlen(pszCNonce));
      MD5Final((unsigned char *)HA1, &Md5Ctx);
    }
  CvtHex(HA1, SessionKey);
}

/* calculate request-digest/response-digest as per HTTP Digest spec */
void DigestCalcResponse(IN HASHHEX HA1,         /* H(A1) */
			IN char * pszNonce,     /* nonce from server */
			IN char * pszNonceCount,/* 8 hex digits */
			IN char * pszCNonce,    /* client nonce */
			IN char * pszQop,       /* qop-value: "", "auth", "auth-int" */
			IN char * pszMethod,    /* method from the request */
			IN char * pszDigestUri, /* requested URL */
			IN HASHHEX HEntity,     /* H(entity body) if qop="auth-int" */
			OUT HASHHEX Response    /* request-digest or response-digest */)
{
  MD5_CTX Md5Ctx;
  HASH HA2;
  HASH RespHash;
  HASHHEX HA2Hex;
  
  /* calculate H(A2) */
  MD5Init(&Md5Ctx);
  MD5Update(&Md5Ctx, (unsigned char *)pszMethod, strlen(pszMethod));
  MD5Update(&Md5Ctx, (unsigned char *)":", 1);
  MD5Update(&Md5Ctx, (unsigned char *)pszDigestUri, strlen(pszDigestUri));
  
  if (pszQop!=NULL)
    {

/*#define AUTH_INT_SUPPORT*/            /* experimental  */
#ifdef AUTH_INT_SUPPORT                   /* experimental  */
      char *index = strchr(pszQop,'i');
      while (index!=NULL&&index-pszQop>=5&&strlen(index)>=3)
	{
	  if (strncasecmp(index-5, "auth-int",8) == 0)
	    {
	      goto auth_withqop;
	    }
	  index = strchr(index+1,'i');
	}

      strchr(pszQop,'a');
      while (index!=NULL&&strlen(index)>=4)
	{
	  if (strncasecmp(index-5, "auth",4) == 0)
	    {
	      /* and in the case of a unknown token
		 like auth1. It is not auth, but this
		 implementation will think it is!??
		 This is may not happen but it's a bug!
	      */
	      goto auth_withqop;
	    }
	  index = strchr(index+1,'a');
	}
#endif
      goto auth_withoutqop;

  };
  
 auth_withoutqop:
  MD5Final((unsigned char*)HA2, &Md5Ctx);
  CvtHex(HA2, HA2Hex);

  /* calculate response */
  MD5Init(&Md5Ctx);
  MD5Update(&Md5Ctx, (unsigned char*)HA1, HASHHEXLEN);
  MD5Update(&Md5Ctx, (unsigned char*)":", 1);
  MD5Update(&Md5Ctx, (unsigned char*)pszNonce, strlen(pszNonce));
  MD5Update(&Md5Ctx, (unsigned char*)":", 1);

  goto end;

 auth_withqop:

  MD5Update(&Md5Ctx, (unsigned char*)":", 1);
  MD5Update(&Md5Ctx, (unsigned char*)HEntity, HASHHEXLEN);
  MD5Final((unsigned char*)HA2, &Md5Ctx);
  CvtHex(HA2, HA2Hex);

  /* calculate response */
  MD5Init(&Md5Ctx);
  MD5Update(&Md5Ctx, (unsigned char*)HA1, HASHHEXLEN);
  MD5Update(&Md5Ctx, (unsigned char*)":", 1);
  MD5Update(&Md5Ctx, (unsigned char*)pszNonce, strlen(pszNonce));
  MD5Update(&Md5Ctx, (unsigned char*)":", 1);
  MD5Update(&Md5Ctx, (unsigned char*)pszNonceCount, strlen(pszNonceCount));
  MD5Update(&Md5Ctx, (unsigned char*)":", 1);
  MD5Update(&Md5Ctx, (unsigned char*)pszCNonce, strlen(pszCNonce));
  MD5Update(&Md5Ctx, (unsigned char*)":", 1);
  MD5Update(&Md5Ctx, (unsigned char*)pszQop, strlen(pszQop));
  MD5Update(&Md5Ctx, (unsigned char*)":", 1);

 end:
  MD5Update(&Md5Ctx, (unsigned char*)HA2Hex, HASHHEXLEN);
  MD5Final((unsigned char*)RespHash, &Md5Ctx);
  CvtHex(RespHash, Response);
}


int
osip_create_authorization_header(osip_message_t *previous_answer,
				 char *rquri, char *username, char *passwd,
				 osip_authorization_t **auth)
{
  osip_authorization_t *aut;
  osip_www_authenticate_t *wa=NULL;

  osip_message_get_www_authenticate(previous_answer,0,&wa);

  /* make some test */
  if (passwd==NULL)
    return -1;
  if (wa==NULL||wa->auth_type==NULL
      ||(wa->realm==NULL)||(wa->nonce==NULL)) {
    OSIP_TRACE (osip_trace
		(__FILE__, __LINE__, OSIP_ERROR, NULL,
		 "www_authenticate header is not acceptable.\n"));
    return -1;
    }
  if (0!=osip_strcasecmp("Digest",wa->auth_type))
    {
      OSIP_TRACE (osip_trace
		  (__FILE__, __LINE__, OSIP_ERROR, NULL,
		   "Authentication method not supported. (Digest only).\n"));
      return -1;
    }
  if (wa->algorithm!=NULL&&0!=osip_strcasecmp("MD5",wa->algorithm))
    {
      OSIP_TRACE (osip_trace
		  (__FILE__, __LINE__, OSIP_ERROR, NULL,
		   "Authentication method not supported. (Digest only).\n"));
      return -1;
    }
  if (0!=osip_authorization_init(&aut))
    {
      OSIP_TRACE (osip_trace
		  (__FILE__, __LINE__, OSIP_ERROR, NULL,
		   "allocation with authorization_init failed.\n"));
      return -1;
    }

  /* just copy some feilds from response to new request */
  osip_authorization_set_auth_type(aut,osip_strdup("Digest"));
  osip_authorization_set_realm(aut,osip_strdup(osip_www_authenticate_get_realm(wa)));
  osip_authorization_set_nonce(aut,osip_strdup(osip_www_authenticate_get_nonce(wa)));
  if (osip_www_authenticate_get_opaque(wa)!=NULL)
    osip_authorization_set_opaque(aut,osip_strdup(osip_www_authenticate_get_opaque(wa)));
  /* copy the username field in new request */
  aut->username = osip_malloc(strlen(username)+3);
  sprintf(aut->username,"\"%s\"",username);

  {
    char *tmp = osip_malloc(strlen(rquri)+3);
    sprintf(tmp,"\"%s\"",rquri);
    osip_authorization_set_uri(aut,tmp);
  }

  osip_authorization_set_algorithm(aut,osip_strdup("MD5"));
  
  {   
    char * pszNonce = osip_strdup_without_quote(osip_www_authenticate_get_nonce(wa));
    char * pszCNonce= NULL;
    char * pszUser = username;
    char * pszRealm = osip_strdup_without_quote(osip_authorization_get_realm(aut));
    char * pszPass=NULL;
    char * pszAlg = osip_strdup("MD5");
    char *szNonceCount = NULL;
    char * pszMethod = previous_answer->cseq->method;
    char * pszQop = NULL;
    char * pszURI = rquri;

    HASHHEX HA1;
    HASHHEX HA2 = "";
    HASHHEX Response;

    pszPass=passwd;
    if (osip_authorization_get_nonce_count(aut)!=NULL)
      szNonceCount = osip_strdup(osip_authorization_get_nonce_count(aut));
    if (osip_authorization_get_message_qop(aut)!=NULL)
      pszQop = osip_strdup(osip_authorization_get_message_qop(aut));

    DigestCalcHA1(pszAlg, pszUser, pszRealm, pszPass, pszNonce,
	 	  pszCNonce, HA1);
    DigestCalcResponse(HA1, pszNonce, szNonceCount, pszCNonce, pszQop,
		       pszMethod, pszURI, HA2, Response);
    OSIP_TRACE (osip_trace
		(__FILE__, __LINE__, OSIP_INFO4, NULL,
		 "Response in authorization |%s|\n", Response));
    {
      char *resp = osip_malloc(35);
      sprintf(resp,"\"%s\"",Response);
      osip_authorization_set_response(aut,resp);
    }
  }

  *auth = aut;
  return 0;
}


int
osip_create_proxy_authorization_header(osip_message_t *previous_answer,
				       char *rquri,char *username,char *passwd,
				       osip_proxy_authorization_t **auth)
{
  osip_proxy_authorization_t *aut;
  osip_proxy_authenticate_t *wa;

  osip_message_get_proxy_authenticate(previous_answer,0,&wa);
  
  /* make some test */
  if (passwd==NULL)
    return -1;
  if (wa==NULL||wa->auth_type==NULL
      ||(wa->realm==NULL)||(wa->nonce==NULL))
    {
      OSIP_TRACE (osip_trace
		  (__FILE__, __LINE__, OSIP_ERROR, NULL,
		   "www_authenticate header is not acceptable.\n"));
      return -1;
    }
  if (0!=strcasecmp("Digest",wa->auth_type))
    {
      OSIP_TRACE (osip_trace
		  (__FILE__, __LINE__, OSIP_ERROR, NULL,
		   "Authentication method not supported. (Digest only).\n"));
      return -1;
    }
  if (wa->algorithm!=NULL&&0!=strcasecmp("MD5",wa->algorithm))
    {
      OSIP_TRACE (osip_trace
		  (__FILE__, __LINE__, OSIP_ERROR, NULL,
		   "Authentication method not supported. (MD5 Digest only).\n"));
      return -1;
    }
  if (0!=osip_proxy_authorization_init(&aut))
    {
      OSIP_TRACE (osip_trace
		  (__FILE__, __LINE__, OSIP_ERROR, NULL,
		   "allocation with authorization_init failed.\n"));
      return -1;
    }

  /* just copy some feilds from response to new request */
  osip_proxy_authorization_set_auth_type(aut,osip_strdup("Digest"));
  osip_proxy_authorization_set_realm(aut,osip_strdup(osip_proxy_authenticate_get_realm(wa)));
  osip_proxy_authorization_set_nonce(aut,osip_strdup(osip_proxy_authenticate_get_nonce(wa)));
  if (osip_proxy_authenticate_get_opaque(wa)!=NULL)
    osip_proxy_authorization_set_opaque(aut,osip_strdup(osip_proxy_authenticate_get_opaque(wa)));
  /* copy the username field in new request */
  aut->username = osip_malloc(strlen(username)+3);
  sprintf(aut->username,"\"%s\"",username);

  {
    char *tmp = osip_malloc(strlen(rquri)+3);
    sprintf(tmp,"\"%s\"",rquri);
    osip_proxy_authorization_set_uri(aut,tmp);
  }
  osip_proxy_authorization_set_algorithm(aut,osip_strdup("MD5"));
  
  {
    char * pszNonce = NULL;
    char * pszCNonce= NULL ; 
    char * pszUser = username;
    char * pszRealm = osip_strdup_without_quote(osip_proxy_authorization_get_realm(aut));
    char * pszPass = NULL;
    char * pszAlg = osip_strdup("MD5");
    char *szNonceCount = NULL;
    char * pszMethod = previous_answer->cseq->method;
    char * pszQop = NULL;
    char * pszURI = rquri; 
   
    HASHHEX HA1;
    HASHHEX HA2 = "";
    HASHHEX Response;
    
    pszPass=passwd;
	
    if (osip_www_authenticate_get_nonce(wa)==NULL)
      return -1;
    pszNonce = osip_strdup_without_quote(osip_www_authenticate_get_nonce(wa));

    /* should upgrade szNonceCount */
    /* should add szNonceCount in aut*/
    /* should upgrade pszCNonce */
    /* should add pszCNonce in aut */
    
    if (osip_proxy_authenticate_get_qop_options(wa)!=NULL)
      {
	szNonceCount = osip_strdup("00000001");
	/* MUST be incremented on each */
	pszQop = osip_strdup(osip_proxy_authenticate_get_qop_options(wa));
	pszCNonce = osip_strdup("234abcc436e2667097e7fe6eia53e8dd");
      }
    DigestCalcHA1(pszAlg, pszUser, pszRealm, pszPass, pszNonce,
	 	  pszCNonce, HA1);
    DigestCalcResponse(HA1, pszNonce, szNonceCount, pszCNonce, pszQop,
		       pszMethod, pszURI, HA2, Response);
    OSIP_TRACE (osip_trace
		(__FILE__, __LINE__, OSIP_INFO4, NULL,
		 "Response in proxy_authorization |%s|\n", Response));
    {
      char *resp = osip_malloc(35);
      sprintf(resp,"\"%s\"",Response);
      osip_proxy_authorization_set_response(aut,resp);
    }
  }

  *auth = aut;
  return 0;
}
