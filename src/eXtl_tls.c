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
#include "eXtransport.h"

#ifdef HAVE_OPENSSL_SSL_H

#include <openssl/ssl.h>
#include <openssl/err.h>
#include <openssl/pem.h>
#include <openssl/x509.h>
#include <openssl/rand.h>

#define SSLDEBUG 1
/*#define PATH "D:/conf/"

#define PASSWORD "23-+Wert"
#define CLIENT_KEYFILE PATH"ckey.pem"
#define CLIENT_CERTFILE PATH"c.pem"
#define SERVER_KEYFILE PATH"skey.pem"
#define SERVER_CERTFILE PATH"s.pem"
#define CA_LIST PATH"cacert.pem"
#define RANDOM  PATH"random.pem"
#define DHFILE PATH"dh1024.pem"*/

#if defined(_WIN32_WCE)
#define strerror(X) "-1"
#endif

SSL_CTX *initialize_client_ctx (const char *keyfile, const char *certfile,
                                const char *password, int transport);

SSL_CTX *initialize_server_ctx (const char *keyfile, const char *certfile,
                                const char *password, int transport);

static int tls_socket;
static struct sockaddr_storage ai_addr;

static char tls_firewall_ip[64];
static char tls_firewall_port[10];

static SSL_CTX *ssl_ctx;
static SSL_CTX *client_ctx;
static eXosip_tls_ctx_t eXosip_tls_ctx_params;

/* persistent connection */
struct socket_tab
{
  int socket;
  char remote_ip[65];
  int remote_port;
  SSL *ssl_conn;
  SSL_CTX *ssl_ctx;
  int ssl_state;

};

#ifndef EXOSIP_MAX_SOCKETS
#define EXOSIP_MAX_SOCKETS 100
#endif

static struct socket_tab tls_socket_tab[EXOSIP_MAX_SOCKETS];

static int
tls_tl_init (void)
{
  tls_socket = 0;
  ssl_ctx = NULL;
  client_ctx = NULL;
  memset (&ai_addr, 0, sizeof (struct sockaddr_storage));
  memset (&tls_socket_tab, 0, sizeof (struct socket_tab) * EXOSIP_MAX_SOCKETS);
  memset (tls_firewall_ip, 0, sizeof (tls_firewall_ip));
  memset (tls_firewall_port, 0, sizeof (tls_firewall_port));
  return OSIP_SUCCESS;
}

static int
tls_tl_free (void)
{
  int pos;
  if (ssl_ctx != NULL)
    SSL_CTX_free (ssl_ctx);

  if (client_ctx != NULL)
    SSL_CTX_free (client_ctx);

  for (pos = 0; pos < EXOSIP_MAX_SOCKETS; pos++)
    {
      if (tls_socket_tab[pos].socket > 0)
        {
          if (tls_socket_tab[pos].ssl_conn != NULL)
            {
              SSL_shutdown (tls_socket_tab[pos].ssl_conn);
              SSL_shutdown (tls_socket_tab[pos].ssl_conn);
              SSL_free (tls_socket_tab[pos].ssl_conn);
            }
          if (tls_socket_tab[pos].ssl_ctx != NULL)
            SSL_CTX_free (tls_socket_tab[pos].ssl_ctx);
          close (tls_socket_tab[pos].socket);
        }
    }

  //free error strings (OpenSSL-lib)
  EVP_cleanup ();
  ERR_free_strings ();
  ERR_remove_state (0);
  //clear the encryption data (OpenSSL-lib)

  CRYPTO_cleanup_all_ex_data ();


  memset (&tls_socket_tab, 0, sizeof (struct socket_tab) * EXOSIP_MAX_SOCKETS);

  memset (tls_firewall_ip, 0, sizeof (tls_firewall_ip));
  memset (tls_firewall_port, 0, sizeof (tls_firewall_port));
  memset (&ai_addr, 0, sizeof (struct sockaddr_storage));
  if (tls_socket > 0)
    close (tls_socket);

  //free the memory from our SSL parameters
  memset (&eXosip_tls_ctx_params, 0, sizeof (eXosip_tls_ctx_t));
  return OSIP_SUCCESS;
}

static int
password_cb (char *buf, int num, int rwflag, void *userdata)
{
  char *passwd = (char *) userdata;
  if (passwd == NULL || passwd[0] == '\0')
    {
      return OSIP_SUCCESS;
    }
  strncpy (buf, passwd, num);
  buf[num - 1] = '\0';
  return strlen (buf);
}

static void
load_dh_params (SSL_CTX * ctx, char *file)
{
  DH *ret = 0;
  BIO *bio;

  if ((bio = BIO_new_file (file, "r")) == NULL)
    {
      OSIP_TRACE (osip_trace
                  (__FILE__, __LINE__, OSIP_ERROR, NULL,
                   "eXosip: Couldn't open DH file!\n"));
  } else
    {
      ret = PEM_read_bio_DHparams (bio, NULL, NULL, NULL);
      BIO_free (bio);
      if (SSL_CTX_set_tmp_dh (ctx, ret) < 0)
        OSIP_TRACE (osip_trace
                    (__FILE__, __LINE__, OSIP_ERROR, NULL,
                     "eXosip: Couldn't set DH param!\n"));
    }
}

static void
generate_eph_rsa_key (SSL_CTX * ctx)
{
  RSA *rsa;

  rsa = RSA_generate_key (512, RSA_F4, NULL, NULL);

  if (rsa != NULL)
    {
      if (!SSL_CTX_set_tmp_rsa (ctx, rsa))
        OSIP_TRACE (osip_trace
                    (__FILE__, __LINE__, OSIP_ERROR, NULL,
                     "eXosip: Couldn't set RSA key!\n"));

      RSA_free (rsa);
    }
}

eXosip_tls_ctx_error
eXosip_set_tls_ctx (eXosip_tls_ctx_t * ctx)
{
  eXosip_tls_credentials_t *ownClient = &eXosip_tls_ctx_params.client;
  eXosip_tls_credentials_t *ownServer = &eXosip_tls_ctx_params.server;

  //get the credentials
  eXosip_tls_credentials_t *client = &ctx->client;
  eXosip_tls_credentials_t *server = &ctx->server;

  //check if public AND private keys are valid
  if ((client->cert[0] != '\0' && client->priv_key[0] == '\0') ||
      (client->cert[0] == '\0' && client->priv_key[0] != '\0'))
    {
      //no, one is missing
      return TLS_ERR_MISSING_AUTH_PART;
    }
  //check if a password is set, when a private key is present
  if (client->priv_key[0] != '\0' && client->priv_key_pw[0] == '\0')
    {
      return TLS_ERR_NO_PW;
    }
  //check if public AND private keys are valid
  if ((server->cert[0] != '\0' && server->priv_key[0] == '\0') ||
      (server->cert[0] == '\0' && server->priv_key[0] != '\0'))
    {
      //no, one is missing
      return TLS_ERR_MISSING_AUTH_PART;
    }
  //check if a password is set, when a private key is present
  if (server->priv_key[0] != '\0' && server->priv_key_pw[0] == '\0')
    {
      return TLS_ERR_NO_PW;
    }
  //check if the file for diffie hellman is present 
  /*if(ctx->dh_param[0] == '\0') {
     return TLS_ERR_NO_DH_PARAM;
     } */

  //check if a file with random data is present --> will be verified when random file is needed (see tls_tl_open)
  /*if(ctx->random_file[0] == '\0') {
     return TLS_ERR_NO_RAND;
     } */

  //check if a file with the list of possible rootCAs is available
  /*if(ctx->root_ca_cert[0] == '\0') {
     return TLS_ERR_NO_ROOT_CA;
     } */

  /* clean up configuration */
  memset (&eXosip_tls_ctx_params, 0, sizeof (eXosip_tls_ctx_t));

  //check if client has own certificate
  if (client->cert[0] != '\0')
    {
      snprintf (ownClient->cert, sizeof (ownClient->cert), "%s", client->cert);
      snprintf (ownClient->priv_key, sizeof (ownClient->priv_key), "%s",
                client->priv_key);
      snprintf (ownClient->priv_key_pw, sizeof (ownClient->priv_key_pw), "%s",
                client->priv_key_pw);
  } else if (server->cert[0] != '\0')
    {
      //no, has no certificates -> copy the chars of the server
      snprintf (ownClient->cert, sizeof (ownClient->cert), "%s", server->cert);
      snprintf (ownClient->priv_key, sizeof (ownClient->priv_key), "%s",
                server->priv_key);
      snprintf (ownClient->priv_key_pw, sizeof (ownClient->priv_key_pw), "%s",
                server->priv_key_pw);
    }
  //check if server has own certificate
  if (server->cert[0] != '\0')
    {
      snprintf (ownServer->cert, sizeof (ownServer->cert), "%s", server->cert);
      snprintf (ownServer->cert, sizeof (ownServer->cert), "%s", server->priv_key);
      snprintf (ownServer->cert, sizeof (ownServer->cert), "%s",
                server->priv_key_pw);
  } else if (client->cert[0] != '\0')
    {
      //no, has no certificates -> copy the chars of the client
      snprintf (ownServer->cert, sizeof (ownServer->cert), "%s", client->cert);
      snprintf (ownServer->cert, sizeof (ownServer->cert), "%s", client->priv_key);
      snprintf (ownServer->cert, sizeof (ownServer->cert), "%s",
                client->priv_key_pw);
    }

  snprintf (eXosip_tls_ctx_params.dh_param, sizeof (ctx->dh_param), "%s",
            ctx->dh_param);
  snprintf (eXosip_tls_ctx_params.random_file, sizeof (ctx->random_file), "%s",
            ctx->random_file);
  snprintf (eXosip_tls_ctx_params.root_ca_cert, sizeof (ctx->root_ca_cert), "%s",
            ctx->root_ca_cert);

  return TLS_OK;
}

SSL_CTX *
initialize_client_ctx (const char *keyfile, const char *certfile,
                       const char *password, int transport)
{
  SSL_METHOD *meth = NULL;
  SSL_CTX *ctx;

  if (transport == IPPROTO_UDP)
    {
#if !(OPENSSL_VERSION_NUMBER < 0x00908000L)
      meth = DTLSv1_client_method ();
#endif
  } else if (transport == IPPROTO_TCP)
    {
      meth = TLSv1_client_method ();
  } else
    {
      return NULL;
    }

  ctx = SSL_CTX_new (meth);

  if (ctx == NULL)
    {
      OSIP_TRACE (osip_trace
                  (__FILE__, __LINE__, OSIP_ERROR, NULL,
                   "eXosip: Couldn't create SSL_CTX!\n"));
      return NULL;
    }
  /* SSL_CTX_set_read_ahead(ctx, 1); */

  SSL_CTX_set_default_passwd_cb_userdata (ctx, (void *) password);
  SSL_CTX_set_default_passwd_cb (ctx, password_cb);


  if (certfile[0] != '\0')
    {
      // Load our keys and certificates
      if (!(SSL_CTX_use_certificate_file (ctx, certfile, SSL_FILETYPE_PEM)))
        {
          OSIP_TRACE (osip_trace
                      (__FILE__, __LINE__, OSIP_ERROR, NULL,
                       "eXosip: Couldn't read client certificate file %s!\n",
                       certfile));
        }

      if (!(SSL_CTX_use_PrivateKey_file (ctx, keyfile, SSL_FILETYPE_PEM)))
        OSIP_TRACE (osip_trace
                    (__FILE__, __LINE__, OSIP_ERROR, NULL,
                     "eXosip: Couldn't read client pkey file %s!\n", keyfile));

      if (!(SSL_CTX_use_RSAPrivateKey_file (ctx, keyfile, SSL_FILETYPE_PEM)))
        OSIP_TRACE (osip_trace
                    (__FILE__, __LINE__, OSIP_ERROR, NULL,
                     "eXosip: Couldn't read client RSA key file %s!\n", keyfile));
    }
  // Load the CAs we trust
  if (!
      (SSL_CTX_load_verify_locations (ctx, eXosip_tls_ctx_params.root_ca_cert, 0)))
    OSIP_TRACE (osip_trace
                (__FILE__, __LINE__, OSIP_ERROR, NULL,
                 "eXosip: Couldn't read CA list\n"));

  {
    int verify_mode = SSL_VERIFY_NONE;
#if 0
    verify_mode = SSL_VERIFY_PEER;
#endif

    SSL_CTX_set_verify (ctx, verify_mode, NULL);
    SSL_CTX_set_verify_depth (ctx, 3);
  }

  SSL_CTX_set_options (ctx, SSL_OP_ALL | SSL_OP_NO_SSLv2 |
                       SSL_OP_NO_SESSION_RESUMPTION_ON_RENEGOTIATION |
                       SSL_OP_CIPHER_SERVER_PREFERENCE);

  return ctx;
}

SSL_CTX *
initialize_server_ctx (const char *keyfile, const char *certfile,
                       const char *password, int transport)
{
  SSL_METHOD *meth = NULL;
  SSL_CTX *ctx;

  int s_server_session_id_context = 1;

  if (transport == IPPROTO_UDP)
    {
      OSIP_TRACE (osip_trace (__FILE__, __LINE__, OSIP_INFO3, NULL,
                              "DTLSv1 server method\n"));
#if !(OPENSSL_VERSION_NUMBER < 0x00908000L)
      meth = DTLSv1_server_method ();
#endif
  } else if (transport == IPPROTO_TCP)
    {
      OSIP_TRACE (osip_trace (__FILE__, __LINE__, OSIP_INFO3, NULL,
                              "TLS server method\n"));
      meth = TLSv1_server_method ();
  } else
    {
      return NULL;
    }

  ctx = SSL_CTX_new (meth);

  if (ctx == NULL)
    {
      OSIP_TRACE (osip_trace
                  (__FILE__, __LINE__, OSIP_ERROR, NULL,
                   "eXosip: Couldn't create SSL_CTX!\n"));
      return NULL;
    }

  SSL_CTX_set_default_passwd_cb_userdata (ctx, (void *) password);
  SSL_CTX_set_default_passwd_cb (ctx, password_cb);

  if (transport == IPPROTO_UDP)
    {
      OSIP_TRACE (osip_trace (__FILE__, __LINE__, OSIP_INFO3, NULL,
                              "DTLS read ahead\n"));
      SSL_CTX_set_read_ahead (ctx, 1);
    }

  /* Load our keys and certificates */
  if (!(SSL_CTX_use_certificate_file (ctx, certfile, SSL_FILETYPE_PEM)))
    {
      OSIP_TRACE (osip_trace
                  (__FILE__, __LINE__, OSIP_ERROR, NULL,
                   "eXosip: Couldn't read certificate file!\n"));
    }

  /* Load the CAs we trust */
  if (!
      (SSL_CTX_load_verify_locations (ctx, eXosip_tls_ctx_params.root_ca_cert, 0)))
    {
      OSIP_TRACE (osip_trace
                  (__FILE__, __LINE__, OSIP_ERROR, NULL,
                   "eXosip: Couldn't read CA list\n"));
    }
  SSL_CTX_set_verify_depth (ctx, 5);

  SSL_CTX_set_options (ctx, SSL_OP_ALL | SSL_OP_NO_SSLv2 |
                       SSL_OP_NO_SESSION_RESUMPTION_ON_RENEGOTIATION |
                       SSL_OP_CIPHER_SERVER_PREFERENCE);

  if (!(SSL_CTX_use_PrivateKey_file (ctx, keyfile, SSL_FILETYPE_PEM)))
    {
      OSIP_TRACE (osip_trace
                  (__FILE__, __LINE__, OSIP_ERROR, NULL,
                   "eXosip: Couldn't read key file: %s\n", keyfile));
      return OSIP_SUCCESS;
    }

  if (!SSL_CTX_check_private_key (ctx))
    {
      OSIP_TRACE (osip_trace
                  (__FILE__, __LINE__, OSIP_ERROR, NULL,
                   "check_private_key: Key '%s' does not match the public key of the certificate\n",
                   SSL_FILETYPE_PEM));
      return NULL;
    }

  load_dh_params (ctx, eXosip_tls_ctx_params.dh_param);

  generate_eph_rsa_key (ctx);

  SSL_CTX_set_session_id_context (ctx, (void *) &s_server_session_id_context,
                                  sizeof s_server_session_id_context);

  return ctx;
}

/**
* @brief Initializes the OpenSSL lib and the client/server contexts.
* Depending on the previously initialized eXosip TLS context (see eXosip_set_tls_ctx() ), only the necessary contexts will be initialized.
* The client context will be ALWAYS initialized, the server context only if certificates are available. The following chart should illustrate
* the behaviour.
*
* possible certificates  | Client initialized			  | Server initialized
* -------------------------------------------------------------------------------------
* no certificate		 | yes, no cert used			  | not initialized
* only client cert		 | yes, own cert (client) used    | yes, client cert used
* only server cert		 | yes, server cert used		  | yes, own cert (server) used
* server and client cert | yes, own cert (client) used    | yes, own cert (server) used
*
* The file for seeding the PRNG is only needed on Windows machines. If you compile under a Windows environment, please set W32 oder _WINDOWS as
* Preprocessor directives.
*@return < 0 if an error occured
**/
static int
tls_tl_open (void)
{
  int res;
  struct addrinfo *addrinfo = NULL;
  struct addrinfo *curinfo;
  int sock = -1;

  if (eXtl_tls.proto_port < 0)
    eXtl_tls.proto_port = 5061;

  // initialization (outside initialize_server_ctx)
  SSL_library_init ();
  SSL_load_error_strings ();

  if (eXosip_tls_ctx_params.server.cert[0] != '\0')
    {
      ssl_ctx = initialize_server_ctx (eXosip_tls_ctx_params.server.priv_key,
                                       eXosip_tls_ctx_params.server.cert,
                                       eXosip_tls_ctx_params.server.priv_key_pw,
                                       IPPROTO_TCP);
    }
  //always initialize the client
  client_ctx = initialize_client_ctx (eXosip_tls_ctx_params.client.priv_key,
                                      eXosip_tls_ctx_params.client.cert,
                                      eXosip_tls_ctx_params.server.priv_key_pw,
                                      IPPROTO_TCP);

//only necessary under Windows-based OS, unix-like systems use /dev/random or /dev/urandom
#if defined(WIN32) || defined(_WINDOWS)

#if 0
  //check if a file with random data is present --> will be verified when random file is needed
  if (eXosip_tls_ctx_params.random_file[0] == '\0')
    {
      return TLS_ERR_NO_RAND;
    }
#endif

  /* Load randomness */
  if (!(RAND_load_file (eXosip_tls_ctx_params.random_file, 1024 * 1024)))
    OSIP_TRACE (osip_trace
                (__FILE__, __LINE__, OSIP_ERROR, NULL,
                 "eXosip: Couldn't load randomness\n"));
#endif

  res = eXosip_get_addrinfo (&addrinfo,
                             eXtl_tls.proto_ifs,
                             eXtl_tls.proto_port, eXtl_tls.proto_num);
  if (res)
    return -1;

  for (curinfo = addrinfo; curinfo; curinfo = curinfo->ai_next)
    {
      socklen_t len;

      if (curinfo->ai_protocol && curinfo->ai_protocol != eXtl_tls.proto_num)
        {
          OSIP_TRACE (osip_trace
                      (__FILE__, __LINE__, OSIP_INFO3, NULL,
                       "eXosip: Skipping protocol %d\n", curinfo->ai_protocol));
          continue;
        }

      sock = (int) socket (curinfo->ai_family, curinfo->ai_socktype,
                           curinfo->ai_protocol);
      if (sock < 0)
        {
          OSIP_TRACE (osip_trace
                      (__FILE__, __LINE__, OSIP_ERROR, NULL,
                       "eXosip: Cannot create socket %s!\n", strerror (errno)));
          continue;
        }

      if (curinfo->ai_family == AF_INET6)
        {
#ifdef IPV6_V6ONLY
          if (setsockopt_ipv6only (sock))
            {
              close (sock);
              sock = -1;
              OSIP_TRACE (osip_trace
                          (__FILE__, __LINE__, OSIP_ERROR, NULL,
                           "eXosip: Cannot set socket option %s!\n",
                           strerror (errno)));
              continue;
            }
#endif /* IPV6_V6ONLY */
        }

      res = bind (sock, curinfo->ai_addr, curinfo->ai_addrlen);
      if (res < 0)
        {
          OSIP_TRACE (osip_trace
                      (__FILE__, __LINE__, OSIP_ERROR, NULL,
                       "eXosip: Cannot bind socket node:%s family:%d %s\n",
                       eXtl_tls.proto_ifs, curinfo->ai_family, strerror (errno)));
          close (sock);
          sock = -1;
          continue;
        }
      len = sizeof (ai_addr);
      res = getsockname (sock, (struct sockaddr *) &ai_addr, &len);
      if (res != 0)
        {
          OSIP_TRACE (osip_trace
                      (__FILE__, __LINE__, OSIP_ERROR, NULL,
                       "eXosip: Cannot get socket name (%s)\n", strerror (errno)));
          memcpy (&ai_addr, curinfo->ai_addr, curinfo->ai_addrlen);
        }

      if (eXtl_tls.proto_num == IPPROTO_TCP)
        {
          res = listen (sock, SOMAXCONN);
          if (res < 0)
            {
              OSIP_TRACE (osip_trace
                          (__FILE__, __LINE__, OSIP_ERROR, NULL,
                           "eXosip: Cannot bind socket node:%s family:%d %s\n",
                           eXtl_tls.proto_ifs, curinfo->ai_family,
                           strerror (errno)));
              close (sock);
              sock = -1;
              continue;
            }
        }

      break;
    }

  eXosip_freeaddrinfo (addrinfo);

  if (sock < 0)
    {
      OSIP_TRACE (osip_trace
                  (__FILE__, __LINE__, OSIP_ERROR, NULL,
                   "eXosip: Cannot bind on port: %i\n", eXtl_tls.proto_port));
      return -1;
    }

  tls_socket = sock;

  if (eXtl_tls.proto_port == 0)
    {
      /* get port number from socket */
      if (eXtl_tls.proto_family == AF_INET)
        eXtl_tls.proto_port = ntohs (((struct sockaddr_in *) &ai_addr)->sin_port);
      else
        eXtl_tls.proto_port =
          ntohs (((struct sockaddr_in6 *) &ai_addr)->sin6_port);
      OSIP_TRACE (osip_trace
                  (__FILE__, __LINE__, OSIP_INFO1, NULL,
                   "eXosip: Binding on port %i!\n", eXtl_tls.proto_port));
    }

  snprintf (tls_firewall_port, sizeof (tls_firewall_port), "%i",
            eXtl_tls.proto_port);
  return OSIP_SUCCESS;
}

static int
tls_tl_set_fdset (fd_set * osip_fdset, int *fd_max)
{
  int pos;
  if (tls_socket <= 0)
    return -1;

  eXFD_SET (tls_socket, osip_fdset);

  if (tls_socket > *fd_max)
    *fd_max = tls_socket;

  for (pos = 0; pos < EXOSIP_MAX_SOCKETS; pos++)
    {
      if (tls_socket_tab[pos].socket > 0)
        {
          eXFD_SET (tls_socket_tab[pos].socket, osip_fdset);
          if (tls_socket_tab[pos].socket > *fd_max)
            *fd_max = tls_socket_tab[pos].socket;
        }
    }

  return OSIP_SUCCESS;
}

int static
print_ssl_error (int err)
{
  switch (err)
    {
      case SSL_ERROR_NONE:
        OSIP_TRACE (osip_trace
                    (__FILE__, __LINE__, OSIP_ERROR, NULL,
                     "SSL ERROR NONE - OK\n"));
        break;
      case SSL_ERROR_ZERO_RETURN:
        OSIP_TRACE (osip_trace
                    (__FILE__, __LINE__, OSIP_ERROR, NULL,
                     "SSL ERROR ZERO RETURN - SHUTDOWN\n"));
        break;
      case SSL_ERROR_WANT_READ:
        OSIP_TRACE (osip_trace
                    (__FILE__, __LINE__, OSIP_ERROR, NULL, "SSL want read\n"));
        break;
      case SSL_ERROR_WANT_WRITE:
        OSIP_TRACE (osip_trace
                    (__FILE__, __LINE__, OSIP_ERROR, NULL, "SSL want write\n"));
        break;
      case SSL_ERROR_SSL:
        OSIP_TRACE (osip_trace
                    (__FILE__, __LINE__, OSIP_ERROR, NULL, "SSL ERROR\n"));
        break;
      case SSL_ERROR_SYSCALL:
        OSIP_TRACE (osip_trace
                    (__FILE__, __LINE__, OSIP_ERROR, NULL, "SSL ERROR SYSCALL\n"));
        break;
      default:
        OSIP_TRACE (osip_trace
                    (__FILE__, __LINE__, OSIP_ERROR, NULL, "SSL problem\n"));
    }
  return OSIP_SUCCESS;
}

static int
tls_tl_read_message (fd_set * osip_fdset)
{
  int pos = 0;
  char *buf;

  if (FD_ISSET (tls_socket, osip_fdset))
    {
      /* accept incoming connection */
      char src6host[NI_MAXHOST];
      int recvport = 0;
      struct sockaddr_storage sa;
      int sock;
      int i;

#ifdef __linux
      socklen_t slen;
#else
      int slen;
#endif

      SSL *ssl = NULL;
      BIO *sbio;


      if (eXtl_tls.proto_family == AF_INET)
        slen = sizeof (struct sockaddr_in);
      else
        slen = sizeof (struct sockaddr_in6);

      for (pos = 0; pos < EXOSIP_MAX_SOCKETS; pos++)
        {
          if (tls_socket_tab[pos].socket <= 0)
            break;
        }
      if (pos < 0)
        {
          /* delete an old one! */
          pos = 0;
          if (tls_socket_tab[pos].socket > 0)
            {
              if (tls_socket_tab[pos].ssl_conn != NULL)
                {
                  SSL_shutdown (tls_socket_tab[pos].ssl_conn);
                  SSL_shutdown (tls_socket_tab[pos].ssl_conn);
                  SSL_free (tls_socket_tab[pos].ssl_conn);
                  SSL_CTX_free (tls_socket_tab[pos].ssl_ctx);
                }
              close (tls_socket_tab[pos].socket);
            }
          memset (&tls_socket_tab[pos], 0, sizeof (struct socket_tab));
        }
      OSIP_TRACE (osip_trace (__FILE__, __LINE__, OSIP_INFO3, NULL,
                              "creating TLS socket at index: %i\n", pos));

      sock = accept (tls_socket, (struct sockaddr *) &sa, &slen);
      if (sock < 0)
        {
          OSIP_TRACE (osip_trace (__FILE__, __LINE__, OSIP_ERROR, NULL,
                                  "Error accepting TLS socket\n"));
      } else
        {
          if (ssl_ctx != NULL)
            {
              OSIP_TRACE (osip_trace (__FILE__, __LINE__, OSIP_INFO1, NULL,
                                      "TLS connection rejected\n"));
              close (sock);
              return -1;
            }

          if (!SSL_CTX_check_private_key (ssl_ctx))
            {
              OSIP_TRACE (osip_trace (__FILE__, __LINE__, OSIP_ERROR, NULL,
                                      "SSL CTX private key check error\n"));
            }

          ssl = SSL_new (ssl_ctx);
          if (ssl == NULL)
            {
              OSIP_TRACE (osip_trace (__FILE__, __LINE__, OSIP_ERROR, NULL,
                                      "Cannot create ssl connection context\n"));
              return -1;
            }

          if (!SSL_check_private_key (ssl))
            {
              OSIP_TRACE (osip_trace (__FILE__, __LINE__, OSIP_ERROR, NULL,
                                      "SSL private key check error\n"));
            }

          sbio = BIO_new_socket (sock, BIO_NOCLOSE);
          if (sbio == NULL)
            {
              OSIP_TRACE (osip_trace (__FILE__, __LINE__, OSIP_ERROR, NULL,
                                      "BIO_new_socket error\n"));
            }

          SSL_set_bio (ssl, sbio, sbio);        /* cannot fail */

          i = SSL_accept (ssl);
          if (i <= 0)
            {
              i = SSL_get_error (ssl, i);
              print_ssl_error (i);


              SSL_shutdown (ssl);
              close (sock);
              SSL_free (ssl);
              if (tls_socket_tab[pos].ssl_ctx != NULL)
                SSL_CTX_free (tls_socket_tab[pos].ssl_ctx);

              tls_socket_tab[pos].ssl_conn = NULL;
              tls_socket_tab[pos].ssl_ctx = NULL;
              tls_socket_tab[pos].socket = 0;

              return -1;
            }

          OSIP_TRACE (osip_trace (__FILE__, __LINE__, OSIP_INFO1, NULL,
                                  "New TLS connection accepted\n"));

          tls_socket_tab[pos].socket = sock;
          tls_socket_tab[pos].ssl_conn = ssl;
          tls_socket_tab[pos].ssl_state = 2;


          memset (src6host, 0, sizeof (src6host));

          if (eXtl_tls.proto_family == AF_INET)
            recvport = ntohs (((struct sockaddr_in *) &sa)->sin_port);
          else
            recvport = ntohs (((struct sockaddr_in6 *) &sa)->sin6_port);

#if defined(__arc__)
          {
            struct sockaddr_in *fromsa = (struct sockaddr_in *) &sa;
            char *tmp;
            tmp = inet_ntoa (fromsa->sin_addr);
            if (tmp == NULL)
              {
                OSIP_TRACE (osip_trace
                            (__FILE__, __LINE__, OSIP_ERROR, NULL,
                             "Message received from: NULL:%i inet_ntoa failure\n",
                             recvport));
            } else
              {
                snprintf (src6host, sizeof (src6host), "%s", tmp);
                OSIP_TRACE (osip_trace
                            (__FILE__, __LINE__, OSIP_INFO1, NULL,
                             "Message received from: %s:%i\n", src6host,
                             recvport));
                osip_strncpy (tls_socket_tab[pos].remote_ip, src6host,
                              sizeof (tls_socket_tab[pos].remote_ip) - 1);
                tls_socket_tab[pos].remote_port = recvport;
              }
          }
#else
          i = getnameinfo ((struct sockaddr *) &sa, slen,
                           src6host, NI_MAXHOST, NULL, 0, NI_NUMERICHOST);

          if (i != 0)
            {
              OSIP_TRACE (osip_trace
                          (__FILE__, __LINE__, OSIP_ERROR, NULL,
                           "Message received from: NULL:%i getnameinfo failure\n",
                           recvport));
              snprintf (src6host, sizeof (src6host), "127.0.0.1");
          } else
            {
              OSIP_TRACE (osip_trace
                          (__FILE__, __LINE__, OSIP_INFO1, NULL,
                           "Message received from: %s:%i\n", src6host, recvport));
              osip_strncpy (tls_socket_tab[pos].remote_ip, src6host,
                            sizeof (tls_socket_tab[pos].remote_ip) - 1);
              tls_socket_tab[pos].remote_port = recvport;
            }
#endif
        }
    }



  buf = NULL;

  for (pos = 0; pos < EXOSIP_MAX_SOCKETS; pos++)
    {
      if (tls_socket_tab[pos].socket > 0
          && FD_ISSET (tls_socket_tab[pos].socket, osip_fdset))
        {
          int i;
          int rlen, err;

          if (buf == NULL)
            buf =
              (char *) osip_malloc (SIP_MESSAGE_MAX_LENGTH * sizeof (char) + 1);
          if (buf == NULL)
            return OSIP_NOMEM;

          /* do TLS handshake? */
          if (tls_socket_tab[pos].ssl_state == 2)
            {
              i = SSL_do_handshake (tls_socket_tab[pos].ssl_conn);
              if (i <= 0)
                {
                  i = SSL_get_error (tls_socket_tab[pos].ssl_conn, i);
                  print_ssl_error (i);

                  SSL_shutdown (tls_socket_tab[pos].ssl_conn);
                  close (tls_socket_tab[pos].socket);
                  SSL_free (tls_socket_tab[pos].ssl_conn);
                  if (tls_socket_tab[pos].ssl_ctx != NULL)
                    SSL_CTX_free (tls_socket_tab[pos].ssl_ctx);

                  memset (&(tls_socket_tab[pos]), 0, sizeof (tls_socket_tab[pos]));
                  continue;
                }
              tls_socket_tab[pos].ssl_state = 3;

            }

          if (tls_socket_tab[pos].ssl_state != 3)
            continue;

          i = 0;
          rlen = 0;

          do
            {
              i = SSL_read (tls_socket_tab[pos].ssl_conn, buf + rlen,
                            SIP_MESSAGE_MAX_LENGTH - rlen);
              err = SSL_get_error (tls_socket_tab[pos].ssl_conn, i);
              print_ssl_error (err);
              switch (err)
                {
                  case SSL_ERROR_NONE:
                    rlen += i;
                    break;
                }
              if (err == SSL_ERROR_SSL
                  || err == SSL_ERROR_SYSCALL || err == SSL_ERROR_ZERO_RETURN)
                {
                  /*
                     The TLS/SSL connection has been closed.  If the protocol version
                     is SSL 3.0 or TLS 1.0, this result code is returned only if a
                     closure alert has occurred in the protocol, i.e. if the
                     connection has been closed cleanly. Note that in this case
                     SSL_ERROR_ZERO_RETURN does not necessarily indicate that the
                     underlying transport has been closed. */
                  OSIP_TRACE (osip_trace
                              (__FILE__, __LINE__, OSIP_WARNING,
                               NULL, "TLS closed\n"));

                  SSL_shutdown (tls_socket_tab[pos].ssl_conn);
                  close (tls_socket_tab[pos].socket);
                  SSL_free (tls_socket_tab[pos].ssl_conn);
                  if (tls_socket_tab[pos].ssl_ctx != NULL)
                    SSL_CTX_free (tls_socket_tab[pos].ssl_ctx);

                  memset (&(tls_socket_tab[pos]), 0, sizeof (tls_socket_tab[pos]));

                  rlen = 0;     /* discard any remaining data ? */
                  break;
                }
            }
          while (SSL_pending (tls_socket_tab[pos].ssl_conn));

          if (rlen > 5)
            {
              osip_strncpy (buf + rlen, "\0", 1);
              OSIP_TRACE (osip_trace
                          (__FILE__, __LINE__, OSIP_INFO1, NULL,
                           "Received TLS message: \n%s\n", buf));
              _eXosip_handle_incoming_message (buf, i,
                                               tls_socket_tab[pos].socket,
                                               tls_socket_tab[pos].remote_ip,
                                               tls_socket_tab[pos].remote_port);
            }
        }
    }

  if (buf != NULL)
    osip_free (buf);

  return OSIP_SUCCESS;
}


static int
_tls_tl_find_socket (char *host, int port)
{
  int pos;

  for (pos = 0; pos < EXOSIP_MAX_SOCKETS; pos++)
    {
      if (tls_socket_tab[pos].socket != 0)
        {
          if (0 == osip_strcasecmp (tls_socket_tab[pos].remote_ip, host)
              && port == tls_socket_tab[pos].remote_port)
            return pos;
        }
    }
  return -1;
}

static void
tls_dump_cert_info (char *s, X509 * cert)
{
  char *subj;
  char *issuer;

  subj = X509_NAME_oneline (X509_get_subject_name (cert), 0, 0);
  issuer = X509_NAME_oneline (X509_get_issuer_name (cert), 0, 0);

  OSIP_TRACE (osip_trace
              (__FILE__, __LINE__, OSIP_INFO2, NULL,
               "%s subject:%s\n", s ? s : "", subj));
  OSIP_TRACE (osip_trace
              (__FILE__, __LINE__, OSIP_INFO2, NULL,
               "%s issuer: %s\n", s ? s : "", issuer));
  OPENSSL_free (subj);
  OPENSSL_free (issuer);
}

static void
tls_dump_verification_failure (long verification_result)
{
  char tmp[64];

  snprintf (tmp, sizeof (tmp), "unknown errror");
  switch (verification_result)
    {
      case X509_V_ERR_UNABLE_TO_GET_ISSUER_CERT:
        snprintf (tmp, sizeof (tmp), "unable to get issuer certificate");
        break;
      case X509_V_ERR_UNABLE_TO_GET_CRL:
        snprintf (tmp, sizeof (tmp), "unable to get certificate CRL");
        break;
      case X509_V_ERR_UNABLE_TO_DECRYPT_CERT_SIGNATURE:
        snprintf (tmp, sizeof (tmp), "unable to decrypt certificate's signature");
        break;
      case X509_V_ERR_UNABLE_TO_DECRYPT_CRL_SIGNATURE:
        snprintf (tmp, sizeof (tmp), "unable to decrypt CRL's signature");
        break;
      case X509_V_ERR_UNABLE_TO_DECODE_ISSUER_PUBLIC_KEY:
        snprintf (tmp, sizeof (tmp), "unable to decode issuer public key");
        break;
      case X509_V_ERR_CERT_SIGNATURE_FAILURE:
        snprintf (tmp, sizeof (tmp), "certificate signature failure");
        break;
      case X509_V_ERR_CRL_SIGNATURE_FAILURE:
        snprintf (tmp, sizeof (tmp), "CRL signature failure");
        break;
      case X509_V_ERR_CERT_NOT_YET_VALID:
        snprintf (tmp, sizeof (tmp), "certificate is not yet valid");
        break;
      case X509_V_ERR_CERT_HAS_EXPIRED:
        snprintf (tmp, sizeof (tmp), "certificate has expired");
        break;
      case X509_V_ERR_CRL_NOT_YET_VALID:
        snprintf (tmp, sizeof (tmp), "CRL is not yet valid");
        break;
      case X509_V_ERR_CRL_HAS_EXPIRED:
        snprintf (tmp, sizeof (tmp), "CRL has expired");
        break;
      case X509_V_ERR_ERROR_IN_CERT_NOT_BEFORE_FIELD:
        snprintf (tmp, sizeof (tmp),
                  "format error in certificate's notBefore field");
        break;
      case X509_V_ERR_ERROR_IN_CERT_NOT_AFTER_FIELD:
        snprintf (tmp, sizeof (tmp),
                  "format error in certificate's notAfter field");
        break;
      case X509_V_ERR_ERROR_IN_CRL_LAST_UPDATE_FIELD:
        snprintf (tmp, sizeof (tmp), "format error in CRL's lastUpdate field");
        break;
      case X509_V_ERR_ERROR_IN_CRL_NEXT_UPDATE_FIELD:
        snprintf (tmp, sizeof (tmp), "format error in CRL's nextUpdate field");
        break;
      case X509_V_ERR_OUT_OF_MEM:
        snprintf (tmp, sizeof (tmp), "out of memory");
        break;
      case X509_V_ERR_DEPTH_ZERO_SELF_SIGNED_CERT:
        snprintf (tmp, sizeof (tmp), "self signed certificate");
        break;
      case X509_V_ERR_SELF_SIGNED_CERT_IN_CHAIN:
        snprintf (tmp, sizeof (tmp),
                  "self signed certificate in certificate chain");
        break;
      case X509_V_ERR_UNABLE_TO_GET_ISSUER_CERT_LOCALLY:
        snprintf (tmp, sizeof (tmp), "unable to get local issuer certificate");
        break;
      case X509_V_ERR_UNABLE_TO_VERIFY_LEAF_SIGNATURE:
        snprintf (tmp, sizeof (tmp), "unable to verify the first certificate");
        break;
      case X509_V_ERR_CERT_CHAIN_TOO_LONG:
        snprintf (tmp, sizeof (tmp), "certificate chain too long");
        break;
      case X509_V_ERR_CERT_REVOKED:
        snprintf (tmp, sizeof (tmp), "certificate revoked");
        break;
      case X509_V_ERR_INVALID_CA:
        snprintf (tmp, sizeof (tmp), "invalid CA certificate");
        break;
      case X509_V_ERR_PATH_LENGTH_EXCEEDED:
        snprintf (tmp, sizeof (tmp), "path length constraint exceeded");
        break;
      case X509_V_ERR_INVALID_PURPOSE:
        snprintf (tmp, sizeof (tmp), "unsupported certificate purpose");
        break;
      case X509_V_ERR_CERT_UNTRUSTED:
        snprintf (tmp, sizeof (tmp), "certificate not trusted");
        break;
      case X509_V_ERR_CERT_REJECTED:
        snprintf (tmp, sizeof (tmp), "certificate rejected");
        break;
      case X509_V_ERR_SUBJECT_ISSUER_MISMATCH:
        snprintf (tmp, sizeof (tmp), "subject issuer mismatch");
        break;
      case X509_V_ERR_AKID_SKID_MISMATCH:
        snprintf (tmp, sizeof (tmp),
                  "authority and subject key identifier mismatch");
        break;
      case X509_V_ERR_AKID_ISSUER_SERIAL_MISMATCH:
        snprintf (tmp, sizeof (tmp),
                  "authority and issuer serial number mismatch");
        break;
      case X509_V_ERR_KEYUSAGE_NO_CERTSIGN:
        snprintf (tmp, sizeof (tmp),
                  "key usage does not include certificate signing");
        break;
      case X509_V_ERR_APPLICATION_VERIFICATION:
        snprintf (tmp, sizeof (tmp), "application verification failure");
        break;
    }

  OSIP_TRACE (osip_trace
              (__FILE__, __LINE__, OSIP_INFO2, NULL,
               "verification failure: %s\n", tmp));
}



static int
_tls_tl_connect_socket (char *host, int port)
{
  int pos;

  int res;
  struct addrinfo *addrinfo = NULL;
  struct addrinfo *curinfo;
  int sock = -1;

  BIO *sbio;
  SSL *ssl;
  SSL_CTX *ctx;
  X509 *cert;

  char src6host[NI_MAXHOST];
  memset (src6host, 0, sizeof (src6host));

  for (pos = 0; pos < EXOSIP_MAX_SOCKETS; pos++)
    {
      if (tls_socket_tab[pos].socket == 0)
        {
          break;
        }
    }

  if (pos == EXOSIP_MAX_SOCKETS)
    return -1;

  res = eXosip_get_addrinfo (&addrinfo, host, port, IPPROTO_TCP);
  if (res)
    return -1;


  for (curinfo = addrinfo; curinfo; curinfo = curinfo->ai_next)
    {
      if (curinfo->ai_protocol && curinfo->ai_protocol != IPPROTO_TCP)
        {
          OSIP_TRACE (osip_trace
                      (__FILE__, __LINE__, OSIP_INFO2, NULL,
                       "eXosip: Skipping protocol %d\n", curinfo->ai_protocol));
          continue;
        }

      res = getnameinfo ((struct sockaddr *) curinfo->ai_addr, curinfo->ai_addrlen,
                         src6host, NI_MAXHOST, NULL, 0, NI_NUMERICHOST);

      if (res == 0)
        {
          int i = _tls_tl_find_socket (src6host, port);
          if (i >= 0)
            {
              eXosip_freeaddrinfo (addrinfo);
              return i;
            }
          OSIP_TRACE (osip_trace
                      (__FILE__, __LINE__, OSIP_ERROR, NULL,
                       "New binding with %s\n", src6host));
        }

      sock = (int) socket (curinfo->ai_family, curinfo->ai_socktype,
                           curinfo->ai_protocol);
      if (sock < 0)
        {
#if !defined(OSIP_MT) || defined(_WIN32_WCE)
          OSIP_TRACE (osip_trace
                      (__FILE__, __LINE__, OSIP_INFO2, NULL,
                       "eXosip: Cannot create socket!\n"));
#else
          OSIP_TRACE (osip_trace
                      (__FILE__, __LINE__, OSIP_INFO2, NULL,
                       "eXosip: Cannot create socket %s!\n", strerror (errno)));
#endif
          continue;
        }

      if (curinfo->ai_family == AF_INET6)
        {
#ifdef IPV6_V6ONLY
          if (setsockopt_ipv6only (sock))
            {
              close (sock);
              sock = -1;
#if !defined(OSIP_MT) || defined(_WIN32_WCE)
              OSIP_TRACE (osip_trace
                          (__FILE__, __LINE__, OSIP_INFO2, NULL,
                           "eXosip: Cannot set socket option!\n"));
#else
              OSIP_TRACE (osip_trace
                          (__FILE__, __LINE__, OSIP_INFO2, NULL,
                           "eXosip: Cannot set socket option %s!\n",
                           strerror (errno)));
#endif
              continue;
            }
#endif /* IPV6_V6ONLY */
        }

      res = connect (sock, curinfo->ai_addr, curinfo->ai_addrlen);
      if (res < 0)
        {
#if !defined(OSIP_MT) || defined(_WIN32_WCE)
          OSIP_TRACE (osip_trace
                      (__FILE__, __LINE__, OSIP_INFO2, NULL,
                       "eXosip: Cannot bind socket node:%s family:%d\n",
                       host, curinfo->ai_family));
#else
          OSIP_TRACE (osip_trace
                      (__FILE__, __LINE__, OSIP_INFO2, NULL,
                       "eXosip: Cannot bind socket node:%s family:%d %s\n",
                       host, curinfo->ai_family, strerror (errno)));
#endif
          close (sock);
          sock = -1;
          continue;
        }

      break;
    }

  eXosip_freeaddrinfo (addrinfo);

  if (sock > 0)
    {
      ctx = initialize_client_ctx (eXosip_tls_ctx_params.client.priv_key,
                                   eXosip_tls_ctx_params.client.cert,
                                   eXosip_tls_ctx_params.server.priv_key_pw,
                                   IPPROTO_TCP);

      //FIXME: changed parameter from ctx to client_ctx -> works now
      ssl = SSL_new (ctx);
      if (ssl == NULL)
        {
          OSIP_TRACE (osip_trace (__FILE__, __LINE__, OSIP_ERROR, NULL,
                                  "SSL_new error\n"));
          close (sock);
          if (ctx != NULL)
            SSL_CTX_free (ctx);
          return -1;
        }
      sbio = BIO_new_socket (sock, BIO_NOCLOSE);

      if (sbio == NULL)
        {
          OSIP_TRACE (osip_trace (__FILE__, __LINE__, OSIP_ERROR, NULL,
                                  "BIO_new_socket error\n"));
          SSL_shutdown (ssl);
          close (sock);
          SSL_free (ssl);
          if (ctx != NULL)
            SSL_CTX_free (ctx);
          return -1;
        }
      SSL_set_bio (ssl, sbio, sbio);
      res = SSL_connect (ssl);
      if (res < 1)
        {
          res = SSL_get_error (ssl, res);
          print_ssl_error (res);

          OSIP_TRACE (osip_trace (__FILE__, __LINE__, OSIP_ERROR, NULL,
                                  "SSL_connect error\n"));
          SSL_shutdown (ssl);
          close (sock);
          SSL_free (ssl);
          if (ctx != NULL)
            SSL_CTX_free (ctx);
          return -1;
        }

      cert = SSL_get_peer_certificate (ssl);
      if (cert != 0)
        {
          int cert_err;
          tls_dump_cert_info ("tls_connect: remote certificate: ", cert);

          cert_err = SSL_get_verify_result (ssl);
          if (cert_err != X509_V_OK)
            {
              OSIP_TRACE (osip_trace (__FILE__, __LINE__, OSIP_ERROR, NULL,
                                      "Failed to verify remote certificate\n"));
              tls_dump_verification_failure (cert_err);

              if (eXosip_tls_ctx_params.server.cert[0] != '\0')
                {
                  X509_free(cert);
                  return -1;
                }
              else if (cert_err != X509_V_ERR_DEPTH_ZERO_SELF_SIGNED_CERT
                      && cert_err != X509_V_ERR_SELF_SIGNED_CERT_IN_CHAIN
                      && cert_err != X509_V_ERR_CRL_HAS_EXPIRED
                      && cert_err != X509_V_ERR_CERT_HAS_EXPIRED
                      && cert_err != X509_V_ERR_CERT_REVOKED
                      && cert_err != X509_V_ERR_CERT_UNTRUSTED
                      && cert_err != X509_V_ERR_CERT_REJECTED)
                {
                  X509_free (cert);
                  return -1;
                }
              /*else -> I want to keep going ONLY when API didn't specified
              any SSL server certificate */
            }
          X509_free (cert);
      } else
        {
          OSIP_TRACE (osip_trace (__FILE__, __LINE__, OSIP_ERROR, NULL,
                                  "No certificate received\n"));
          //X509_free is not necessary because no cert-object was created -> cert == NULL
          return -1;
        }

      tls_socket_tab[pos].socket = sock;

      if (src6host[0] == '\0')
        osip_strncpy (tls_socket_tab[pos].remote_ip, host,
                      sizeof (tls_socket_tab[pos].remote_ip) - 1);
      else
        osip_strncpy (tls_socket_tab[pos].remote_ip, src6host,
                      sizeof (tls_socket_tab[pos].remote_ip) - 1);

      tls_socket_tab[pos].remote_port = port;
      tls_socket_tab[pos].ssl_conn = ssl;
      tls_socket_tab[pos].ssl_state = 3;

      return pos;
    }

  return -1;
}

static int
tls_tl_send_message (osip_transaction_t * tr, osip_message_t * sip, char *host,
                     int port, int out_socket)
{
  size_t length = 0;
  char *message;
  int i;

  SSL *ssl = NULL;

  if (host == NULL)
    {
      host = sip->req_uri->host;
      if (sip->req_uri->port != NULL)
        port = osip_atoi (sip->req_uri->port);
      else
        port = 5061;
    }

  if (port == 5060)
    port = 5061;

  /* remove preloaded route if there is no tag in the To header
   */
  {
    osip_route_t *route = NULL;
    osip_generic_param_t *tag = NULL;
    osip_message_get_route (sip, 0, &route);

    osip_to_get_tag (sip->to, &tag);
    if (tag == NULL && route != NULL && route->url != NULL)
      {
        osip_list_remove (&sip->routes, 0);
      }
    i = osip_message_to_str (sip, &message, &length);
    if (tag == NULL && route != NULL && route->url != NULL)
      {
        osip_list_add (&sip->routes, route, 0);
      }
  }

  if (i != 0 || length <= 0)
    {
      return -1;
    }

  if (out_socket > 0)
    {
      int pos;
      for (pos = 0; pos < EXOSIP_MAX_SOCKETS; pos++)
        {
          if (tls_socket_tab[pos].socket != 0)
            {
              if (tls_socket_tab[pos].socket == out_socket)
                {
                  out_socket = tls_socket_tab[pos].socket;
                  ssl = tls_socket_tab[pos].ssl_conn;
                  OSIP_TRACE (osip_trace (__FILE__, __LINE__, OSIP_INFO1, NULL,
                                          "Message sent: \n%s (reusing REQUEST connection)\n",
                                          message));
                  break;
                }
            }
        }
      if (pos == EXOSIP_MAX_SOCKETS)
        out_socket = 0;
    }

  /* Step 1: find existing socket to send message */
  if (out_socket <= 0)
    {
      int pos = _tls_tl_find_socket (host, port);

      /* Step 2: create new socket with host:port */
      if (pos < 0)
        {
          pos = _tls_tl_connect_socket (host, port);
        }
      if (pos >= 0)
        {
          out_socket = tls_socket_tab[pos].socket;
          ssl = tls_socket_tab[pos].ssl_conn;
        }

      OSIP_TRACE (osip_trace (__FILE__, __LINE__, OSIP_INFO1, NULL,
                              "Message sent: \n%s (to dest=%s:%i)\n",
                              message, host, port));
    }

  if (out_socket <= 0 || ssl == NULL)
    {
      return -1;
    }

  SSL_set_mode (ssl, SSL_MODE_AUTO_RETRY);

  while (1)
    {
      i = SSL_write (ssl, (const void *) message, length);

      if (i <= 0)
        {
          i = SSL_get_error (ssl, i);
          print_ssl_error (i);
          if (i == SSL_ERROR_WANT_READ)
            continue;

          osip_free (message);
          return -1;
        }
      break;
    }

  osip_free (message);
  return OSIP_SUCCESS;
}

static int
tls_tl_keepalive (void)
{
  return OSIP_SUCCESS;
}

static int
tls_tl_set_socket (int socket)
{
  tls_socket = socket;

  return OSIP_SUCCESS;
}

static int
tls_tl_masquerade_contact (const char *public_address, int port)
{
  if (public_address == NULL || public_address[0] == '\0')
    {
      memset (tls_firewall_ip, '\0', sizeof (tls_firewall_ip));
      return OSIP_SUCCESS;
    }
  snprintf (tls_firewall_ip, sizeof (tls_firewall_ip), "%s", public_address);
  if (port > 0)
    {
      snprintf (tls_firewall_port, sizeof (tls_firewall_port), "%i", port);
    }
  return OSIP_SUCCESS;
}

static int
tls_tl_get_masquerade_contact (char *ip, int ip_size, char *port, int port_size)
{
  memset (ip, 0, ip_size);
  memset (port, 0, port_size);

  if (tls_firewall_ip[0] != '\0')
    snprintf (ip, ip_size, "%s", tls_firewall_ip);

  if (tls_firewall_port[0] != '\0')
    snprintf (port, port_size, "%s", tls_firewall_port);
  return OSIP_SUCCESS;
}

struct eXtl_protocol eXtl_tls = {
  1,
  5061,
  "TLS",
  "0.0.0.0",
  IPPROTO_TCP,
  AF_INET,
  0,
  0,

  &tls_tl_init,
  &tls_tl_free,
  &tls_tl_open,
  &tls_tl_set_fdset,
  &tls_tl_read_message,
  &tls_tl_send_message,
  &tls_tl_keepalive,
  &tls_tl_set_socket,
  &tls_tl_masquerade_contact,
  &tls_tl_get_masquerade_contact
};

#endif
