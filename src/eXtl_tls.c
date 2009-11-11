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

#ifdef HAVE_FCNTL_H
#include <fcntl.h>
#endif

#ifdef WIN32
#include <Mstcpip.h>
#include <wincrypt.h>
#endif

#ifdef HAVE_OPENSSL_SSL_H

#define verify_depth 10
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

SSL_CTX *initialize_client_ctx(const char *keyfile, const char *certfile,
							   const char *password, int transport);

SSL_CTX *initialize_server_ctx(const char *keyfile, const char *certfile,
							   const char *password, int transport);

static int tls_socket;
static struct sockaddr_storage ai_addr;

static char tls_firewall_ip[64];
static char tls_firewall_port[10];

static SSL_CTX *server_ctx;
static SSL_CTX *client_ctx;
static eXosip_tls_ctx_t eXosip_tls_ctx_params;

/* persistent connection */
struct socket_tab {
	int socket;
	char remote_ip[65];
	int remote_port;
	SSL *ssl_conn;
	SSL_CTX *ssl_ctx;
	int ssl_state;

};

#define SOCKET_TIMEOUT 0

#ifndef EXOSIP_MAX_SOCKETS
#define EXOSIP_MAX_SOCKETS 100
#endif

static struct socket_tab tls_socket_tab[EXOSIP_MAX_SOCKETS];

static int tls_tl_init(void)
{
	tls_socket = 0;
	server_ctx = NULL;
	client_ctx = NULL;
	memset(&ai_addr, 0, sizeof(struct sockaddr_storage));
	memset(&tls_socket_tab, 0, sizeof(struct socket_tab) * EXOSIP_MAX_SOCKETS);
	memset(tls_firewall_ip, 0, sizeof(tls_firewall_ip));
	memset(tls_firewall_port, 0, sizeof(tls_firewall_port));
	return OSIP_SUCCESS;
}

static int tls_tl_free(void)
{
	int pos;
	if (server_ctx != NULL)
		SSL_CTX_free(server_ctx);
	server_ctx=NULL;

	if (client_ctx != NULL)
		SSL_CTX_free(client_ctx);
	client_ctx=NULL;

	for (pos = 0; pos < EXOSIP_MAX_SOCKETS; pos++) {
		if (tls_socket_tab[pos].socket > 0) {
			if (tls_socket_tab[pos].ssl_conn != NULL) {
				SSL_shutdown(tls_socket_tab[pos].ssl_conn);
				SSL_shutdown(tls_socket_tab[pos].ssl_conn);
				SSL_free(tls_socket_tab[pos].ssl_conn);
			}
			if (tls_socket_tab[pos].ssl_ctx != NULL)
				SSL_CTX_free(tls_socket_tab[pos].ssl_ctx);
			close(tls_socket_tab[pos].socket);
		}
	}

	EVP_cleanup();
	ERR_free_strings();
	ERR_remove_state(0);

	CRYPTO_cleanup_all_ex_data();


	memset(&tls_socket_tab, 0, sizeof(struct socket_tab) * EXOSIP_MAX_SOCKETS);

	memset(tls_firewall_ip, 0, sizeof(tls_firewall_ip));
	memset(tls_firewall_port, 0, sizeof(tls_firewall_port));
	memset(&ai_addr, 0, sizeof(struct sockaddr_storage));
	if (tls_socket > 0)
		close(tls_socket);

	memset(&eXosip_tls_ctx_params, 0, sizeof(eXosip_tls_ctx_t));
	return OSIP_SUCCESS;
}

static void tls_dump_cert_info(const char *s, X509 * cert)
{
	char *subj;
	char *issuer;

	subj = X509_NAME_oneline(X509_get_subject_name(cert), 0, 0);
	issuer = X509_NAME_oneline(X509_get_issuer_name(cert), 0, 0);

	OSIP_TRACE(osip_trace
			   (__FILE__, __LINE__, OSIP_INFO2, NULL,
				"%s subject:%s\n", s ? s : "", subj));
	OSIP_TRACE(osip_trace
			   (__FILE__, __LINE__, OSIP_INFO2, NULL,
				"%s issuer: %s\n", s ? s : "", issuer));
	OPENSSL_free(subj);
	OPENSSL_free(issuer);
}

int _tls_add_windows_certificates(SSL_CTX *ctx, const char *store)
{
	int count=0;
	PCCERT_CONTEXT pCertCtx;
	X509 *cert = NULL;
	HCERTSTORE hStore = CertOpenSystemStore(0, store);

	for ( pCertCtx = CertEnumCertificatesInStore(hStore, NULL);
		pCertCtx != NULL;
		pCertCtx = CertEnumCertificatesInStore(hStore, pCertCtx) )
	{
		cert = d2i_X509(NULL, (const unsigned char **) &pCertCtx->pbCertEncoded,
			pCertCtx->cbCertEncoded);
		if (cert == NULL) {
			SSLerr(SSL_F_SSL_CTX_USE_CERTIFICATE_FILE, ERR_R_ASN1_LIB);
			continue;
		}
		tls_dump_cert_info(store, cert);

		if (!X509_STORE_add_cert(ctx->cert_store, cert)) {
			continue;
		}
		count++;
		X509_free(cert);
	}

	CertCloseStore(hStore, 0);
	return count;
}


int verify_cb(int preverify_ok, X509_STORE_CTX *store)
{
	char    buf[256];
    X509   *err_cert;
    int     err, depth;
    SSL    *ssl;

    err_cert = X509_STORE_CTX_get_current_cert(store);
    err = X509_STORE_CTX_get_error(store);
    depth = X509_STORE_CTX_get_error_depth(store);

	ssl = X509_STORE_CTX_get_ex_data(store, SSL_get_ex_data_X509_STORE_CTX_idx());
    X509_NAME_oneline(X509_get_subject_name(err_cert), buf, 256);

	if (depth > verify_depth /* depth -1 */) {
        preverify_ok = 0;
        err = X509_V_ERR_CERT_CHAIN_TOO_LONG;
        X509_STORE_CTX_set_error(store, err);
    }
    if (!preverify_ok) {
		OSIP_TRACE(osip_trace
		   (__FILE__, __LINE__, OSIP_ERROR, NULL,
		   "verify error:num=%d:%s:depth=%d:%s\n", err,
		   X509_verify_cert_error_string(err), depth, buf));
    }
    else
    {
		OSIP_TRACE(osip_trace
		   (__FILE__, __LINE__, OSIP_ERROR, NULL,
		   "depth=%d:%s\n", depth, buf));
    }
    /*
     * At this point, err contains the last verification error. We can use
     * it for something special
     */
    if (!preverify_ok && (err == X509_V_ERR_UNABLE_TO_GET_ISSUER_CERT))
    {
      X509_NAME_oneline(X509_get_issuer_name(store->current_cert), buf, 256);
		OSIP_TRACE(osip_trace
		   (__FILE__, __LINE__, OSIP_ERROR, NULL,
		   "issuer= %s\n", buf));
    }

    if (!preverify_ok && (err == X509_V_ERR_SELF_SIGNED_CERT_IN_CHAIN))
    {
      X509_NAME_oneline(X509_get_issuer_name(store->current_cert), buf, 256);
		OSIP_TRACE(osip_trace
		   (__FILE__, __LINE__, OSIP_ERROR, NULL,
		   "issuer= %s\n", buf));
		preverify_ok=1;
		X509_STORE_CTX_set_error(store,X509_V_OK);
    }

    if (!preverify_ok && (err == X509_V_ERR_CERT_HAS_EXPIRED))
    {
      X509_NAME_oneline(X509_get_issuer_name(store->current_cert), buf, 256);
		OSIP_TRACE(osip_trace
		   (__FILE__, __LINE__, OSIP_ERROR, NULL,
		   "issuer= %s\n", buf));
		preverify_ok=1;
		X509_STORE_CTX_set_error(store,X509_V_OK);
    }
	
	return preverify_ok;
}

static int password_cb(char *buf, int num, int rwflag, void *userdata)
{
	char *passwd = (char *) userdata;
	if (passwd == NULL || passwd[0] == '\0') {
		return OSIP_SUCCESS;
	}
	strncpy(buf, passwd, num);
	buf[num - 1] = '\0';
	return strlen(buf);
}

static void load_dh_params(SSL_CTX * ctx, char *file)
{
	DH *ret = 0;
	BIO *bio;

	if ((bio = BIO_new_file(file, "r")) == NULL) {
		OSIP_TRACE(osip_trace
				   (__FILE__, __LINE__, OSIP_ERROR, NULL,
					"eXosip: Couldn't open DH file!\n"));
	} else {
		ret = PEM_read_bio_DHparams(bio, NULL, NULL, NULL);
		BIO_free(bio);
		if (SSL_CTX_set_tmp_dh(ctx, ret) < 0)
			OSIP_TRACE(osip_trace
					   (__FILE__, __LINE__, OSIP_ERROR, NULL,
						"eXosip: Couldn't set DH param!\n"));
	}
}

static void generate_eph_rsa_key(SSL_CTX * ctx)
{
	RSA *rsa;

	rsa = RSA_generate_key(512, RSA_F4, NULL, NULL);

	if (rsa != NULL) {
		if (!SSL_CTX_set_tmp_rsa(ctx, rsa))
			OSIP_TRACE(osip_trace
					   (__FILE__, __LINE__, OSIP_ERROR, NULL,
						"eXosip: Couldn't set RSA key!\n"));

		RSA_free(rsa);
	}
}

eXosip_tls_ctx_error eXosip_set_tls_ctx(eXosip_tls_ctx_t * ctx)
{
	eXosip_tls_credentials_t *ownClient = &eXosip_tls_ctx_params.client;
	eXosip_tls_credentials_t *ownServer = &eXosip_tls_ctx_params.server;

	eXosip_tls_credentials_t *client = &ctx->client;
	eXosip_tls_credentials_t *server = &ctx->server;

	/* check if public AND private keys are valid */
	if ((client->cert[0] != '\0' && client->priv_key[0] == '\0') ||
		(client->cert[0] == '\0' && client->priv_key[0] != '\0')) {
		/* no, one is missing */
		return TLS_ERR_MISSING_AUTH_PART;
	}
	/* check if a password is set, when a private key is present */
	if (client->priv_key[0] != '\0' && client->priv_key_pw[0] == '\0') {
		return TLS_ERR_NO_PW;
	}
	/* check if public AND private keys are valid */
	if ((server->cert[0] != '\0' && server->priv_key[0] == '\0') ||
		(server->cert[0] == '\0' && server->priv_key[0] != '\0')) {
		/* no, one is missing */
		return TLS_ERR_MISSING_AUTH_PART;
	}
	/* check if a password is set, when a private key is present */
	if (server->priv_key[0] != '\0' && server->priv_key_pw[0] == '\0') {
		return TLS_ERR_NO_PW;
	}
	/* check if the file for diffie hellman is present */
	/*if(ctx->dh_param[0] == '\0') {
	   return TLS_ERR_NO_DH_PARAM;
	   } */

	/* check if a file with random data is present --> will be verified when random file is needed (see tls_tl_open) */
	/*if(ctx->random_file[0] == '\0') {
	   return TLS_ERR_NO_RAND;
	   } */

	/* check if a file with the list of possible rootCAs is available */
	/*if(ctx->root_ca_cert[0] == '\0') {
	   return TLS_ERR_NO_ROOT_CA;
	   } */

	/* clean up configuration */
	memset(&eXosip_tls_ctx_params, 0, sizeof(eXosip_tls_ctx_t));

	/* check if client has own certificate */
	if (client->cert[0] != '\0') {
		snprintf(ownClient->cert, sizeof(ownClient->cert), "%s", client->cert);
		snprintf(ownClient->priv_key, sizeof(ownClient->priv_key), "%s",
				 client->priv_key);
		snprintf(ownClient->priv_key_pw, sizeof(ownClient->priv_key_pw), "%s",
				 client->priv_key_pw);
	} else if (server->cert[0] != '\0') {
		/* no, has no certificates -> copy the chars of the server */
		snprintf(ownClient->cert, sizeof(ownClient->cert), "%s", server->cert);
		snprintf(ownClient->priv_key, sizeof(ownClient->priv_key), "%s",
				 server->priv_key);
		snprintf(ownClient->priv_key_pw, sizeof(ownClient->priv_key_pw), "%s",
				 server->priv_key_pw);
	}
	/* check if server has own certificate */
	if (server->cert[0] != '\0') {
		snprintf(ownServer->cert, sizeof(ownServer->cert), "%s", server->cert);
		snprintf(ownServer->priv_key, sizeof(ownServer->priv_key), "%s",
				 server->priv_key);
		snprintf(ownServer->priv_key_pw, sizeof(ownServer->priv_key_pw), "%s",
				 server->priv_key_pw);
	} else if (client->cert[0] != '\0') {
		/* no, has no certificates -> copy the chars of the client */
		snprintf(ownServer->cert, sizeof(ownServer->cert), "%s", client->cert);
		snprintf(ownServer->priv_key, sizeof(ownServer->priv_key), "%s",
				 client->priv_key);
		snprintf(ownServer->priv_key_pw, sizeof(ownServer->priv_key_pw), "%s",
				 client->priv_key_pw);
	}

	snprintf(eXosip_tls_ctx_params.dh_param, sizeof(ctx->dh_param), "%s",
			 ctx->dh_param);
	snprintf(eXosip_tls_ctx_params.random_file, sizeof(ctx->random_file), "%s",
			 ctx->random_file);
	snprintf(eXosip_tls_ctx_params.root_ca_cert, sizeof(ctx->root_ca_cert), "%s",
			 ctx->root_ca_cert);

	return TLS_OK;
}

SSL_CTX *initialize_client_ctx(const char *keyfile, const char *certfile,
							   const char *password, int transport)
{
	SSL_METHOD *meth = NULL;
	SSL_CTX *ctx;

	if (transport == IPPROTO_UDP) {
#if !(OPENSSL_VERSION_NUMBER < 0x00908000L)
		meth = DTLSv1_client_method();
#endif
	} else if (transport == IPPROTO_TCP) {
		meth = TLSv1_client_method();
	} else {
		return NULL;
	}

	ctx = SSL_CTX_new(meth);

	if (ctx == NULL) {
		OSIP_TRACE(osip_trace
				   (__FILE__, __LINE__, OSIP_ERROR, NULL,
					"eXosip: Couldn't create SSL_CTX!\n"));
		return NULL;
	}

	SSL_CTX_set_default_passwd_cb_userdata(ctx, (void *) password);
	SSL_CTX_set_default_passwd_cb(ctx, password_cb);


	if (certfile[0] != '\0') {
		/* Load our keys and certificates */
		if (!(SSL_CTX_use_certificate_file(ctx, certfile, SSL_FILETYPE_PEM))) {
			OSIP_TRACE(osip_trace
					   (__FILE__, __LINE__, OSIP_ERROR, NULL,
						"eXosip: Couldn't read client certificate file %s!\n",
						certfile));
		}

		if (!(SSL_CTX_use_PrivateKey_file(ctx, keyfile, SSL_FILETYPE_PEM)))
			OSIP_TRACE(osip_trace
					   (__FILE__, __LINE__, OSIP_ERROR, NULL,
						"eXosip: Couldn't read client pkey file %s!\n", keyfile));

		if (!(SSL_CTX_use_RSAPrivateKey_file(ctx, keyfile, SSL_FILETYPE_PEM)))
			OSIP_TRACE(osip_trace
					   (__FILE__, __LINE__, OSIP_ERROR, NULL,
						"eXosip: Couldn't read client RSA key file %s!\n",
						keyfile));
	}
	/* Load the CAs we trust */
	if (!
		(SSL_CTX_load_verify_locations
		 (ctx, eXosip_tls_ctx_params.root_ca_cert, 0)))
		OSIP_TRACE(osip_trace
				   (__FILE__, __LINE__, OSIP_ERROR, NULL,
					"eXosip: Couldn't read CA list\n"));

	{
		int verify_mode = SSL_VERIFY_NONE;
		verify_mode = SSL_VERIFY_PEER;

		SSL_CTX_set_verify(ctx, verify_mode, &verify_cb);
		SSL_CTX_set_verify_depth(ctx, verify_depth+1);
	}

	SSL_CTX_set_options(ctx, SSL_OP_ALL | SSL_OP_NO_SSLv2 |
						SSL_OP_NO_SESSION_RESUMPTION_ON_RENEGOTIATION |
						SSL_OP_CIPHER_SERVER_PREFERENCE);

	if (_tls_add_windows_certificates(ctx, "CA")<=0) {
		OSIP_TRACE(osip_trace
				   (__FILE__, __LINE__, OSIP_ERROR, NULL,
				   "Cannot load CA certificates from Microsoft Certificate Store"));
	}
	if (_tls_add_windows_certificates(ctx, "ROOT")<=0) {
		OSIP_TRACE(osip_trace
				   (__FILE__, __LINE__, OSIP_ERROR, NULL,
				   "Cannot load Root certificates from Microsoft Certificate Store"));
	}
	if (_tls_add_windows_certificates(ctx, "MY")<=0) {
		OSIP_TRACE(osip_trace
				   (__FILE__, __LINE__, OSIP_ERROR, NULL,
				   "Cannot load Root certificates from Microsoft Certificate Store"));
	}

	return ctx;
}

SSL_CTX *initialize_server_ctx(const char *keyfile, const char *certfile,
							   const char *password, int transport)
{
	SSL_METHOD *meth = NULL;
	SSL_CTX *ctx;

	int s_server_session_id_context = 1;

	if (transport == IPPROTO_UDP) {
		OSIP_TRACE(osip_trace(__FILE__, __LINE__, OSIP_INFO3, NULL,
							  "DTLSv1 server method\n"));
#if !(OPENSSL_VERSION_NUMBER < 0x00908000L)
		meth = DTLSv1_server_method();
#endif
	} else if (transport == IPPROTO_TCP) {
		OSIP_TRACE(osip_trace(__FILE__, __LINE__, OSIP_INFO3, NULL,
							  "TLS server method\n"));
		meth = TLSv1_server_method();
	} else {
		return NULL;
	}

	ctx = SSL_CTX_new(meth);

	if (ctx == NULL) {
		OSIP_TRACE(osip_trace
				   (__FILE__, __LINE__, OSIP_ERROR, NULL,
					"eXosip: Couldn't create SSL_CTX!\n"));
		return NULL;
	}

	if (transport == IPPROTO_UDP) {
		OSIP_TRACE(osip_trace(__FILE__, __LINE__, OSIP_INFO3, NULL,
							  "DTLS read ahead\n"));
		SSL_CTX_set_read_ahead(ctx, 1);
	}

	if (certfile[0] != '\0') {
		SSL_CTX_set_default_passwd_cb_userdata(ctx, (void *) password);
		SSL_CTX_set_default_passwd_cb(ctx, password_cb);

		/* Load our keys and certificates */
		if (!(SSL_CTX_use_certificate_file(ctx, certfile, SSL_FILETYPE_PEM))) {
			OSIP_TRACE(osip_trace
					   (__FILE__, __LINE__, OSIP_ERROR, NULL,
						"eXosip: Couldn't read certificate file!\n"));
		}

		/* Load the CAs we trust */
		if (!
			(SSL_CTX_load_verify_locations
			 (ctx, eXosip_tls_ctx_params.root_ca_cert, 0))) {
			OSIP_TRACE(osip_trace
					   (__FILE__, __LINE__, OSIP_ERROR, NULL,
						"eXosip: Couldn't read CA list\n"));
		}
		SSL_CTX_set_verify_depth(ctx, verify_depth+1);

		SSL_CTX_set_options(ctx, SSL_OP_ALL | SSL_OP_NO_SSLv2 |
							SSL_OP_NO_SESSION_RESUMPTION_ON_RENEGOTIATION |
							SSL_OP_CIPHER_SERVER_PREFERENCE);

		if (!(SSL_CTX_use_PrivateKey_file(ctx, keyfile, SSL_FILETYPE_PEM))) {
			OSIP_TRACE(osip_trace
					   (__FILE__, __LINE__, OSIP_ERROR, NULL,
						"eXosip: Couldn't read key file: %s\n", keyfile));
			return OSIP_SUCCESS;
		}

		if (!SSL_CTX_check_private_key(ctx)) {
			OSIP_TRACE(osip_trace
					   (__FILE__, __LINE__, OSIP_ERROR, NULL,
						"check_private_key: Key '%s' does not match the public key of the certificate\n",
						SSL_FILETYPE_PEM));
			return NULL;
		}

		load_dh_params(ctx, eXosip_tls_ctx_params.dh_param);

		generate_eph_rsa_key(ctx);

		SSL_CTX_set_session_id_context(ctx, (void *) &s_server_session_id_context,
									   sizeof s_server_session_id_context);
	}
	else
	{
		if (_tls_add_windows_certificates(ctx, "CA")<=0) {
			OSIP_TRACE(osip_trace
					   (__FILE__, __LINE__, OSIP_ERROR, NULL,
					   "Cannot load CA certificates from Microsoft Certificate Store"));
		}
		if (_tls_add_windows_certificates(ctx, "ROOT")<=0) {
			OSIP_TRACE(osip_trace
					   (__FILE__, __LINE__, OSIP_ERROR, NULL,
					   "Cannot load Root certificates from Microsoft Certificate Store"));
		}

		generate_eph_rsa_key(ctx);

		SSL_CTX_set_session_id_context(ctx, (void *) &s_server_session_id_context,
									   sizeof s_server_session_id_context);
	}

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
static int tls_tl_open(void)
{
	int res;
	struct addrinfo *addrinfo = NULL;
	struct addrinfo *curinfo;
	int sock = -1;

	if (eXtl_tls.proto_port < 0)
		eXtl_tls.proto_port = 5061;

	/* initialization (outside initialize_server_ctx) */
	SSL_library_init();
	SSL_load_error_strings();

	if (eXosip_tls_ctx_params.server.cert[0] != '\0') {
		server_ctx = initialize_server_ctx(eXosip_tls_ctx_params.server.priv_key,
										eXosip_tls_ctx_params.server.cert,
										eXosip_tls_ctx_params.server.priv_key_pw,
										IPPROTO_TCP);
	}
	/* always initialize the client */
	client_ctx = initialize_client_ctx(eXosip_tls_ctx_params.client.priv_key,
									   eXosip_tls_ctx_params.client.cert,
									   eXosip_tls_ctx_params.server.priv_key_pw,
									   IPPROTO_TCP);

/*only necessary under Windows-based OS, unix-like systems use /dev/random or /dev/urandom */
#if defined(WIN32) || defined(_WINDOWS)

#if 0
	/* check if a file with random data is present --> will be verified when random file is needed */
	if (eXosip_tls_ctx_params.random_file[0] == '\0') {
		return TLS_ERR_NO_RAND;
	}
#endif

	/* Load randomness */
	if (!(RAND_load_file(eXosip_tls_ctx_params.random_file, 1024 * 1024)))
		OSIP_TRACE(osip_trace
				   (__FILE__, __LINE__, OSIP_ERROR, NULL,
					"eXosip: Couldn't load randomness\n"));
#endif

	res = eXosip_get_addrinfo(&addrinfo,
							  eXtl_tls.proto_ifs,
							  eXtl_tls.proto_port, eXtl_tls.proto_num);
	if (res)
		return -1;

	for (curinfo = addrinfo; curinfo; curinfo = curinfo->ai_next) {
		socklen_t len;

		if (curinfo->ai_protocol && curinfo->ai_protocol != eXtl_tls.proto_num) {
			OSIP_TRACE(osip_trace
					   (__FILE__, __LINE__, OSIP_INFO3, NULL,
						"eXosip: Skipping protocol %d\n", curinfo->ai_protocol));
			continue;
		}

		sock = (int) socket(curinfo->ai_family, curinfo->ai_socktype,
							curinfo->ai_protocol);
		if (sock < 0) {
			OSIP_TRACE(osip_trace
					   (__FILE__, __LINE__, OSIP_ERROR, NULL,
						"eXosip: Cannot create socket %s!\n", strerror(errno)));
			continue;
		}

		if (curinfo->ai_family == AF_INET6) {
#ifdef IPV6_V6ONLY
			if (setsockopt_ipv6only(sock)) {
				close(sock);
				sock = -1;
				OSIP_TRACE(osip_trace
						   (__FILE__, __LINE__, OSIP_ERROR, NULL,
							"eXosip: Cannot set socket option %s!\n",
							strerror(errno)));
				continue;
			}
#endif							/* IPV6_V6ONLY */
		}

		res = bind(sock, curinfo->ai_addr, curinfo->ai_addrlen);
		if (res < 0) {
			OSIP_TRACE(osip_trace
					   (__FILE__, __LINE__, OSIP_ERROR, NULL,
						"eXosip: Cannot bind socket node:%s family:%d %s\n",
						eXtl_tls.proto_ifs, curinfo->ai_family, strerror(errno)));
			close(sock);
			sock = -1;
			continue;
		}
		len = sizeof(ai_addr);
		res = getsockname(sock, (struct sockaddr *) &ai_addr, &len);
		if (res != 0) {
			OSIP_TRACE(osip_trace
					   (__FILE__, __LINE__, OSIP_ERROR, NULL,
						"eXosip: Cannot get socket name (%s)\n", strerror(errno)));
			memcpy(&ai_addr, curinfo->ai_addr, curinfo->ai_addrlen);
		}

		if (eXtl_tls.proto_num == IPPROTO_TCP) {
			res = listen(sock, SOMAXCONN);
			if (res < 0) {
				OSIP_TRACE(osip_trace
						   (__FILE__, __LINE__, OSIP_ERROR, NULL,
							"eXosip: Cannot bind socket node:%s family:%d %s\n",
							eXtl_tls.proto_ifs, curinfo->ai_family,
							strerror(errno)));
				close(sock);
				sock = -1;
				continue;
			}
		}

		break;
	}

	eXosip_freeaddrinfo(addrinfo);

	if (sock < 0) {
		OSIP_TRACE(osip_trace
				   (__FILE__, __LINE__, OSIP_ERROR, NULL,
					"eXosip: Cannot bind on port: %i\n", eXtl_tls.proto_port));
		return -1;
	}

	tls_socket = sock;

	if (eXtl_tls.proto_port == 0) {
		/* get port number from socket */
		if (eXtl_tls.proto_family == AF_INET)
			eXtl_tls.proto_port =
				ntohs(((struct sockaddr_in *) &ai_addr)->sin_port);
		else
			eXtl_tls.proto_port =
				ntohs(((struct sockaddr_in6 *) &ai_addr)->sin6_port);
		OSIP_TRACE(osip_trace
				   (__FILE__, __LINE__, OSIP_INFO1, NULL,
					"eXosip: Binding on port %i!\n", eXtl_tls.proto_port));
	}

	snprintf(tls_firewall_port, sizeof(tls_firewall_port), "%i",
			 eXtl_tls.proto_port);
	return OSIP_SUCCESS;
}

static int tls_tl_set_fdset(fd_set * osip_fdset, int *fd_max)
{
	int pos;
	if (tls_socket <= 0)
		return -1;

	eXFD_SET(tls_socket, osip_fdset);

	if (tls_socket > *fd_max)
		*fd_max = tls_socket;

	for (pos = 0; pos < EXOSIP_MAX_SOCKETS; pos++) {
		if (tls_socket_tab[pos].socket > 0) {
			eXFD_SET(tls_socket_tab[pos].socket, osip_fdset);
			if (tls_socket_tab[pos].socket > *fd_max)
				*fd_max = tls_socket_tab[pos].socket;
		}
	}

	return OSIP_SUCCESS;
}

int static print_ssl_error(int err)
{
	switch (err) {
	case SSL_ERROR_NONE:
		OSIP_TRACE(osip_trace
				   (__FILE__, __LINE__, OSIP_ERROR, NULL,
					"SSL ERROR NONE - OK\n"));
		break;
	case SSL_ERROR_ZERO_RETURN:
		OSIP_TRACE(osip_trace
				   (__FILE__, __LINE__, OSIP_ERROR, NULL,
					"SSL ERROR ZERO RETURN - SHUTDOWN\n"));
		break;
	case SSL_ERROR_WANT_READ:
		OSIP_TRACE(osip_trace
				   (__FILE__, __LINE__, OSIP_ERROR, NULL, "SSL want read\n"));
		break;
	case SSL_ERROR_WANT_WRITE:
		OSIP_TRACE(osip_trace
				   (__FILE__, __LINE__, OSIP_ERROR, NULL, "SSL want write\n"));
		break;
	case SSL_ERROR_SSL:
		OSIP_TRACE(osip_trace
				   (__FILE__, __LINE__, OSIP_ERROR, NULL, "SSL ERROR\n"));
		break;
	case SSL_ERROR_SYSCALL:
		OSIP_TRACE(osip_trace
				   (__FILE__, __LINE__, OSIP_ERROR, NULL, "SSL ERROR SYSCALL\n"));
		break;
	default:
		OSIP_TRACE(osip_trace
				   (__FILE__, __LINE__, OSIP_ERROR, NULL, "SSL problem\n"));
	}
	return OSIP_SUCCESS;
}


static void tls_dump_verification_failure(long verification_result)
{
	char tmp[64];

	snprintf(tmp, sizeof(tmp), "unknown errror");
	switch (verification_result) {
	case X509_V_ERR_UNABLE_TO_GET_ISSUER_CERT:
		snprintf(tmp, sizeof(tmp), "unable to get issuer certificate");
		break;
	case X509_V_ERR_UNABLE_TO_GET_CRL:
		snprintf(tmp, sizeof(tmp), "unable to get certificate CRL");
		break;
	case X509_V_ERR_UNABLE_TO_DECRYPT_CERT_SIGNATURE:
		snprintf(tmp, sizeof(tmp), "unable to decrypt certificate's signature");
		break;
	case X509_V_ERR_UNABLE_TO_DECRYPT_CRL_SIGNATURE:
		snprintf(tmp, sizeof(tmp), "unable to decrypt CRL's signature");
		break;
	case X509_V_ERR_UNABLE_TO_DECODE_ISSUER_PUBLIC_KEY:
		snprintf(tmp, sizeof(tmp), "unable to decode issuer public key");
		break;
	case X509_V_ERR_CERT_SIGNATURE_FAILURE:
		snprintf(tmp, sizeof(tmp), "certificate signature failure");
		break;
	case X509_V_ERR_CRL_SIGNATURE_FAILURE:
		snprintf(tmp, sizeof(tmp), "CRL signature failure");
		break;
	case X509_V_ERR_CERT_NOT_YET_VALID:
		snprintf(tmp, sizeof(tmp), "certificate is not yet valid");
		break;
	case X509_V_ERR_CERT_HAS_EXPIRED:
		snprintf(tmp, sizeof(tmp), "certificate has expired");
		break;
	case X509_V_ERR_CRL_NOT_YET_VALID:
		snprintf(tmp, sizeof(tmp), "CRL is not yet valid");
		break;
	case X509_V_ERR_CRL_HAS_EXPIRED:
		snprintf(tmp, sizeof(tmp), "CRL has expired");
		break;
	case X509_V_ERR_ERROR_IN_CERT_NOT_BEFORE_FIELD:
		snprintf(tmp, sizeof(tmp),
				 "format error in certificate's notBefore field");
		break;
	case X509_V_ERR_ERROR_IN_CERT_NOT_AFTER_FIELD:
		snprintf(tmp, sizeof(tmp), "format error in certificate's notAfter field");
		break;
	case X509_V_ERR_ERROR_IN_CRL_LAST_UPDATE_FIELD:
		snprintf(tmp, sizeof(tmp), "format error in CRL's lastUpdate field");
		break;
	case X509_V_ERR_ERROR_IN_CRL_NEXT_UPDATE_FIELD:
		snprintf(tmp, sizeof(tmp), "format error in CRL's nextUpdate field");
		break;
	case X509_V_ERR_OUT_OF_MEM:
		snprintf(tmp, sizeof(tmp), "out of memory");
		break;
	case X509_V_ERR_DEPTH_ZERO_SELF_SIGNED_CERT:
		snprintf(tmp, sizeof(tmp), "self signed certificate");
		break;
	case X509_V_ERR_SELF_SIGNED_CERT_IN_CHAIN:
		snprintf(tmp, sizeof(tmp), "self signed certificate in certificate chain");
		break;
	case X509_V_ERR_UNABLE_TO_GET_ISSUER_CERT_LOCALLY:
		snprintf(tmp, sizeof(tmp), "unable to get local issuer certificate");
		break;
	case X509_V_ERR_UNABLE_TO_VERIFY_LEAF_SIGNATURE:
		snprintf(tmp, sizeof(tmp), "unable to verify the first certificate");
		break;
	case X509_V_ERR_CERT_CHAIN_TOO_LONG:
		snprintf(tmp, sizeof(tmp), "certificate chain too long");
		break;
	case X509_V_ERR_CERT_REVOKED:
		snprintf(tmp, sizeof(tmp), "certificate revoked");
		break;
	case X509_V_ERR_INVALID_CA:
		snprintf(tmp, sizeof(tmp), "invalid CA certificate");
		break;
	case X509_V_ERR_PATH_LENGTH_EXCEEDED:
		snprintf(tmp, sizeof(tmp), "path length constraint exceeded");
		break;
	case X509_V_ERR_INVALID_PURPOSE:
		snprintf(tmp, sizeof(tmp), "unsupported certificate purpose");
		break;
	case X509_V_ERR_CERT_UNTRUSTED:
		snprintf(tmp, sizeof(tmp), "certificate not trusted");
		break;
	case X509_V_ERR_CERT_REJECTED:
		snprintf(tmp, sizeof(tmp), "certificate rejected");
		break;
	case X509_V_ERR_SUBJECT_ISSUER_MISMATCH:
		snprintf(tmp, sizeof(tmp), "subject issuer mismatch");
		break;
	case X509_V_ERR_AKID_SKID_MISMATCH:
		snprintf(tmp, sizeof(tmp),
				 "authority and subject key identifier mismatch");
		break;
	case X509_V_ERR_AKID_ISSUER_SERIAL_MISMATCH:
		snprintf(tmp, sizeof(tmp), "authority and issuer serial number mismatch");
		break;
	case X509_V_ERR_KEYUSAGE_NO_CERTSIGN:
		snprintf(tmp, sizeof(tmp),
				 "key usage does not include certificate signing");
		break;
	case X509_V_ERR_APPLICATION_VERIFICATION:
		snprintf(tmp, sizeof(tmp), "application verification failure");
		break;
	}

	OSIP_TRACE(osip_trace
			   (__FILE__, __LINE__, OSIP_INFO2, NULL,
				"verification failure: %s\n", tmp));
}

static int _tls_tl_is_connected(int sock)
{
	int res;
	struct timeval tv;
	fd_set wrset;
	int valopt;
	socklen_t sock_len;
	tv.tv_sec = SOCKET_TIMEOUT / 1000;
	tv.tv_usec = (SOCKET_TIMEOUT % 1000) * 1000;

	FD_ZERO(&wrset);
	FD_SET(sock, &wrset);

	res = select(sock + 1, NULL, &wrset, NULL, &tv);
	if (res > 0) {
		sock_len = sizeof(int);
		if (getsockopt(sock, SOL_SOCKET, SO_ERROR, (void *) (&valopt), &sock_len)
			== 0) {
			if (valopt) {
				OSIP_TRACE(osip_trace
						   (__FILE__, __LINE__, OSIP_INFO2, NULL,
							"Cannot connect socket node / %s[%d]\n",
							strerror(errno), valopt));
				return -1;
			} else {
				return 0;
			}
		} else {
			OSIP_TRACE(osip_trace
					   (__FILE__, __LINE__, OSIP_INFO2, NULL,
						"Cannot connect socket node / error in getsockopt %s[%d]\n",
						strerror(errno), errno));
			return -1;
		}
	} else if (res < 0) {
		OSIP_TRACE(osip_trace
				   (__FILE__, __LINE__, OSIP_INFO2, NULL,
					"Cannot connect socket node / error in select %s[%d]\n",
					strerror(errno), errno));
		return -1;
	} else {
		OSIP_TRACE(osip_trace
				   (__FILE__, __LINE__, OSIP_INFO2, NULL,
					"Cannot connect socket node / select timeout (%d ms)\n",
					SOCKET_TIMEOUT));
		return 1;
	}
}

static int _tls_tl_ssl_connect_socket(int pos)
{
	X509 *cert;
	BIO *sbio;
	int res;

	if (tls_socket_tab[pos].ssl_ctx == NULL) {
		tls_socket_tab[pos].ssl_ctx =
			initialize_client_ctx(eXosip_tls_ctx_params.client.priv_key,
								  eXosip_tls_ctx_params.client.cert,
								  eXosip_tls_ctx_params.server.priv_key_pw,
								  IPPROTO_TCP);

		/* FIXME: changed parameter from ctx to client_ctx -> works now */
		tls_socket_tab[pos].ssl_conn = SSL_new(tls_socket_tab[pos].ssl_ctx);
		if (tls_socket_tab[pos].ssl_conn == NULL) {
			OSIP_TRACE(osip_trace(__FILE__, __LINE__, OSIP_ERROR, NULL,
								  "SSL_new error\n"));
			return -1;
		}
		sbio = BIO_new_socket(tls_socket_tab[pos].socket, BIO_NOCLOSE);

		if (sbio == NULL) {
			OSIP_TRACE(osip_trace(__FILE__, __LINE__, OSIP_ERROR, NULL,
								  "BIO_new_socket error\n"));
			return -1;
		}
		SSL_set_bio(tls_socket_tab[pos].ssl_conn, sbio, sbio);

	}

	if (SSL_is_init_finished(tls_socket_tab[pos].ssl_conn)) {
		OSIP_TRACE(osip_trace(__FILE__, __LINE__, OSIP_INFO2, NULL,
							  "SSL_is_init_finished already done\n"));
	} else {
		OSIP_TRACE(osip_trace(__FILE__, __LINE__, OSIP_INFO2, NULL,
							  "SSL_is_init_finished not already done\n"));
	}

	do {
		struct timeval tv;
		int fd;
		fd_set readfds;

		res = SSL_connect(tls_socket_tab[pos].ssl_conn);
		res = SSL_get_error(tls_socket_tab[pos].ssl_conn, res);
		if (res == SSL_ERROR_NONE) {
			OSIP_TRACE(osip_trace(__FILE__, __LINE__, OSIP_INFO2, NULL,
								  "SSL_connect succeeded\n"));
			break;
		}

		if (res != SSL_ERROR_WANT_READ && res != SSL_ERROR_WANT_WRITE) {
			print_ssl_error(res);
			OSIP_TRACE(osip_trace(__FILE__, __LINE__, OSIP_ERROR, NULL,
								  "SSL_connect error\n"));
			return -1;
		}

		tv.tv_sec = SOCKET_TIMEOUT / 1000;
		tv.tv_usec = (SOCKET_TIMEOUT % 1000) * 1000;
		OSIP_TRACE(osip_trace(__FILE__, __LINE__, OSIP_INFO2, NULL,
							  "SSL_connect retry\n"));

		fd = SSL_get_fd(tls_socket_tab[pos].ssl_conn);
		FD_ZERO(&readfds);
		FD_SET(fd, &readfds);
		res = select(fd + 1, &readfds, NULL, NULL, &tv);
		if (res < 0) {
			OSIP_TRACE(osip_trace(__FILE__, __LINE__, OSIP_INFO2, NULL,
								  "SSL_connect select(read) error (%s)\n",
								  strerror(errno)));
			return -1;
		} else if (res > 0) {
			OSIP_TRACE(osip_trace(__FILE__, __LINE__, OSIP_INFO2, NULL,
								  "SSL_connect (read done)\n"));
		} else {
			OSIP_TRACE(osip_trace(__FILE__, __LINE__, OSIP_INFO2, NULL,
								  "SSL_connect (timeout not data to read) (%d ms)\n",
								  SOCKET_TIMEOUT));
			return 1;
		}
	} while (!SSL_is_init_finished(tls_socket_tab[pos].ssl_conn));

	if (SSL_is_init_finished(tls_socket_tab[pos].ssl_conn)) {
		OSIP_TRACE(osip_trace(__FILE__, __LINE__, OSIP_INFO2, NULL,
							  "SSL_is_init_finished done\n"));
	} else {
		OSIP_TRACE(osip_trace(__FILE__, __LINE__, OSIP_INFO2, NULL,
							  "SSL_is_init_finished failed\n"));
	}

	cert = SSL_get_peer_certificate(tls_socket_tab[pos].ssl_conn);
	if (cert != 0) {
		int cert_err;
		tls_dump_cert_info("tls_connect: remote certificate: ", cert);

		cert_err = SSL_get_verify_result(tls_socket_tab[pos].ssl_conn);
		if (cert_err != X509_V_OK) {
			OSIP_TRACE(osip_trace(__FILE__, __LINE__, OSIP_ERROR, NULL,
								  "Failed to verify remote certificate\n"));
			tls_dump_verification_failure(cert_err);

			if (eXosip_tls_ctx_params.server.cert[0] != '\0') {
				X509_free(cert);
				return -1;
			} else if (cert_err != X509_V_ERR_DEPTH_ZERO_SELF_SIGNED_CERT
					   && cert_err != X509_V_ERR_SELF_SIGNED_CERT_IN_CHAIN
					   && cert_err != X509_V_ERR_CRL_HAS_EXPIRED
					   && cert_err != X509_V_ERR_CERT_HAS_EXPIRED
					   && cert_err != X509_V_ERR_CERT_REVOKED
					   && cert_err != X509_V_ERR_CERT_UNTRUSTED
					   && cert_err != X509_V_ERR_CERT_REJECTED) {
				X509_free(cert);
				return -1;
			}
			/*else -> I want to keep going ONLY when API didn't specified
			   any SSL server certificate */
		}
#if 0
		{
			char peer_CN[65];
			memset(peer_CN, 0, sizeof(peer_CN));
			X509_NAME_get_text_by_NID(X509_get_subject_name(cert), NID_commonName, peer_CN, sizeof(peer_CN));
			if(osip_strcasecmp(tls_socket_tab[pos].remote_ip, peer_CN) != 0)
			{
				SSL_set_verify_result(m_pSSL, X509_V_ERR_APPLICATION_VERIFICATION+1);
			}
		}
#endif

		X509_free(cert);
	} else {
		OSIP_TRACE(osip_trace(__FILE__, __LINE__, OSIP_ERROR, NULL,
							  "No certificate received\n"));
		/* X509_free is not necessary because no cert-object was created -> cert == NULL */
		return -1;
	}

	tls_socket_tab[pos].ssl_state = 3;
	return 0;
}

static int tls_tl_read_message(fd_set * osip_fdset)
{
	int pos = 0;
	char *buf;

	if (FD_ISSET(tls_socket, osip_fdset)) {
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
			slen = sizeof(struct sockaddr_in);
		else
			slen = sizeof(struct sockaddr_in6);

		for (pos = 0; pos < EXOSIP_MAX_SOCKETS; pos++) {
			if (tls_socket_tab[pos].socket <= 0)
				break;
		}
		if (pos < 0) {
			/* delete an old one! */
			pos = 0;
			if (tls_socket_tab[pos].socket > 0) {
				if (tls_socket_tab[pos].ssl_conn != NULL) {
					SSL_shutdown(tls_socket_tab[pos].ssl_conn);
					SSL_shutdown(tls_socket_tab[pos].ssl_conn);
					SSL_free(tls_socket_tab[pos].ssl_conn);
					SSL_CTX_free(tls_socket_tab[pos].ssl_ctx);
				}
				close(tls_socket_tab[pos].socket);
			}
			memset(&tls_socket_tab[pos], 0, sizeof(struct socket_tab));
		}
		OSIP_TRACE(osip_trace(__FILE__, __LINE__, OSIP_INFO3, NULL,
							  "creating TLS socket at index: %i\n", pos));

		sock = accept(tls_socket, (struct sockaddr *) &sa, &slen);
		if (sock < 0) {
			OSIP_TRACE(osip_trace(__FILE__, __LINE__, OSIP_ERROR, NULL,
								  "Error accepting TLS socket\n"));
		} else {
			if (server_ctx == NULL) {
				OSIP_TRACE(osip_trace(__FILE__, __LINE__, OSIP_INFO1, NULL,
									  "TLS connection rejected\n"));
				close(sock);
				return -1;
			}

			if (!SSL_CTX_check_private_key(server_ctx)) {
				OSIP_TRACE(osip_trace(__FILE__, __LINE__, OSIP_ERROR, NULL,
									  "SSL CTX private key check error\n"));
			}

			ssl = SSL_new(server_ctx);
			if (ssl == NULL) {
				OSIP_TRACE(osip_trace(__FILE__, __LINE__, OSIP_ERROR, NULL,
									  "Cannot create ssl connection context\n"));
				return -1;
			}

			if (!SSL_check_private_key(ssl)) {
				OSIP_TRACE(osip_trace(__FILE__, __LINE__, OSIP_ERROR, NULL,
									  "SSL private key check error\n"));
			}

			sbio = BIO_new_socket(sock, BIO_NOCLOSE);
			if (sbio == NULL) {
				OSIP_TRACE(osip_trace(__FILE__, __LINE__, OSIP_ERROR, NULL,
									  "BIO_new_socket error\n"));
			}

			SSL_set_bio(ssl, sbio, sbio);	/* cannot fail */

			i = SSL_accept(ssl);
			if (i <= 0) {
				i = SSL_get_error(ssl, i);
				print_ssl_error(i);


				SSL_shutdown(ssl);
				close(sock);
				SSL_free(ssl);
				if (tls_socket_tab[pos].ssl_ctx != NULL)
					SSL_CTX_free(tls_socket_tab[pos].ssl_ctx);

				tls_socket_tab[pos].ssl_conn = NULL;
				tls_socket_tab[pos].ssl_ctx = NULL;
				tls_socket_tab[pos].socket = 0;

				return -1;
			}

			OSIP_TRACE(osip_trace(__FILE__, __LINE__, OSIP_INFO1, NULL,
								  "New TLS connection accepted\n"));

			tls_socket_tab[pos].socket = sock;
			tls_socket_tab[pos].ssl_conn = ssl;
			tls_socket_tab[pos].ssl_state = 2;


			memset(src6host, 0, sizeof(src6host));

			if (eXtl_tls.proto_family == AF_INET)
				recvport = ntohs(((struct sockaddr_in *) &sa)->sin_port);
			else
				recvport = ntohs(((struct sockaddr_in6 *) &sa)->sin6_port);

#if defined(__arc__)
			{
				struct sockaddr_in *fromsa = (struct sockaddr_in *) &sa;
				char *tmp;
				tmp = inet_ntoa(fromsa->sin_addr);
				if (tmp == NULL) {
					OSIP_TRACE(osip_trace
							   (__FILE__, __LINE__, OSIP_ERROR, NULL,
								"Message received from: NULL:%i inet_ntoa failure\n",
								recvport));
				} else {
					snprintf(src6host, sizeof(src6host), "%s", tmp);
					OSIP_TRACE(osip_trace
							   (__FILE__, __LINE__, OSIP_INFO1, NULL,
								"Message received from: %s:%i\n", src6host,
								recvport));
					osip_strncpy(tls_socket_tab[pos].remote_ip, src6host,
								 sizeof(tls_socket_tab[pos].remote_ip) - 1);
					tls_socket_tab[pos].remote_port = recvport;
				}
			}
#else
			i = getnameinfo((struct sockaddr *) &sa, slen,
							src6host, NI_MAXHOST, NULL, 0, NI_NUMERICHOST);

			if (i != 0) {
				OSIP_TRACE(osip_trace
						   (__FILE__, __LINE__, OSIP_ERROR, NULL,
							"Message received from: NULL:%i getnameinfo failure\n",
							recvport));
				snprintf(src6host, sizeof(src6host), "127.0.0.1");
			} else {
				OSIP_TRACE(osip_trace
						   (__FILE__, __LINE__, OSIP_INFO1, NULL,
							"Message received from: %s:%i\n", src6host, recvport));
				osip_strncpy(tls_socket_tab[pos].remote_ip, src6host,
							 sizeof(tls_socket_tab[pos].remote_ip) - 1);
				tls_socket_tab[pos].remote_port = recvport;
			}
#endif
		}
	}



	buf = NULL;

	for (pos = 0; pos < EXOSIP_MAX_SOCKETS; pos++) {
		if (tls_socket_tab[pos].socket > 0
			&& FD_ISSET(tls_socket_tab[pos].socket, osip_fdset)) {
			int i;
			int rlen, err;

			if (buf == NULL)
				buf =
					(char *) osip_malloc(SIP_MESSAGE_MAX_LENGTH * sizeof(char) +
										 1);
			if (buf == NULL)
				return OSIP_NOMEM;

			/* do TLS handshake? */

			if (tls_socket_tab[pos].ssl_state == 0) {
				i = _tls_tl_is_connected(tls_socket_tab[pos].socket);
				if (i > 0) {
					continue;
				} else if (i == 0) {
					OSIP_TRACE(osip_trace
							   (__FILE__, __LINE__, OSIP_INFO2, NULL,
								"socket node:%s , socket %d [pos=%d], connected\n",
								tls_socket_tab[pos].remote_ip,
								tls_socket_tab[pos].socket, pos));
					tls_socket_tab[pos].ssl_state = 1;
				} else {
					OSIP_TRACE(osip_trace
							   (__FILE__, __LINE__, OSIP_ERROR, NULL,
								"socket node:%s, socket %d [pos=%d], socket error\n",
								tls_socket_tab[pos].remote_ip,
								tls_socket_tab[pos].socket, pos));
					SSL_shutdown(tls_socket_tab[pos].ssl_conn);
					close(tls_socket_tab[pos].socket);
					SSL_free(tls_socket_tab[pos].ssl_conn);
					if (tls_socket_tab[pos].ssl_ctx != NULL)
						SSL_CTX_free(tls_socket_tab[pos].ssl_ctx);

					memset(&(tls_socket_tab[pos]), 0, sizeof(tls_socket_tab[pos]));
					continue;
				}
			}

			if (tls_socket_tab[pos].ssl_state == 1) {
				i = _tls_tl_ssl_connect_socket(pos);
				if (i < 0) {
					SSL_shutdown(tls_socket_tab[pos].ssl_conn);
					close(tls_socket_tab[pos].socket);
					SSL_free(tls_socket_tab[pos].ssl_conn);
					if (tls_socket_tab[pos].ssl_ctx != NULL)
						SSL_CTX_free(tls_socket_tab[pos].ssl_ctx);

					memset(&(tls_socket_tab[pos]), 0, sizeof(tls_socket_tab[pos]));
					continue;
				}
			}

			if (tls_socket_tab[pos].ssl_state == 2) {
				i = SSL_do_handshake(tls_socket_tab[pos].ssl_conn);
				if (i <= 0) {
					i = SSL_get_error(tls_socket_tab[pos].ssl_conn, i);
					print_ssl_error(i);

					SSL_shutdown(tls_socket_tab[pos].ssl_conn);
					close(tls_socket_tab[pos].socket);
					SSL_free(tls_socket_tab[pos].ssl_conn);
					if (tls_socket_tab[pos].ssl_ctx != NULL)
						SSL_CTX_free(tls_socket_tab[pos].ssl_ctx);

					memset(&(tls_socket_tab[pos]), 0, sizeof(tls_socket_tab[pos]));
					continue;
				}
				tls_socket_tab[pos].ssl_state = 3;

			}

			if (tls_socket_tab[pos].ssl_state != 3)
				continue;

			i = 0;
			rlen = 0;

			do {
				i = SSL_read(tls_socket_tab[pos].ssl_conn, buf + rlen,
							 SIP_MESSAGE_MAX_LENGTH - rlen);
				if (i <= 0) {
					err = SSL_get_error(tls_socket_tab[pos].ssl_conn, i);
					if (err == SSL_ERROR_WANT_READ || err == SSL_ERROR_WANT_WRITE) {
						break;
					} else if (err == SSL_ERROR_SYSCALL && errno != EAGAIN) {
						break;
					} else {
						print_ssl_error(err);
						/*
						   The TLS/SSL connection has been closed.  If the protocol version
						   is SSL 3.0 or TLS 1.0, this result code is returned only if a
						   closure alert has occurred in the protocol, i.e. if the
						   connection has been closed cleanly. Note that in this case
						   SSL_ERROR_ZERO_RETURN does not necessarily indicate that the
						   underlying transport has been closed. */
						OSIP_TRACE(osip_trace
								   (__FILE__, __LINE__, OSIP_WARNING,
									NULL, "TLS closed\n"));

						SSL_shutdown(tls_socket_tab[pos].ssl_conn);
						close(tls_socket_tab[pos].socket);
						SSL_free(tls_socket_tab[pos].ssl_conn);
						if (tls_socket_tab[pos].ssl_ctx != NULL)
							SSL_CTX_free(tls_socket_tab[pos].ssl_ctx);

						memset(&(tls_socket_tab[pos]), 0,
							   sizeof(tls_socket_tab[pos]));

						rlen = 0;	/* discard any remaining data ? */
						break;
					}
				} else {
					rlen += i;
				}
			}
			while (SSL_pending(tls_socket_tab[pos].ssl_conn));

			if (rlen > 5) {
				osip_strncpy(buf + rlen, "\0", 1);
				OSIP_TRACE(osip_trace
						   (__FILE__, __LINE__, OSIP_INFO1, NULL,
							"Received TLS message: \n%s\n", buf));
				_eXosip_handle_incoming_message(buf, i,
												tls_socket_tab[pos].socket,
												tls_socket_tab[pos].remote_ip,
												tls_socket_tab[pos].remote_port);
			}
		}
	}

	if (buf != NULL)
		osip_free(buf);

	return OSIP_SUCCESS;
}


static int _tls_tl_find_socket(char *host, int port)
{
	int pos;

	for (pos = 0; pos < EXOSIP_MAX_SOCKETS; pos++) {
		if (tls_socket_tab[pos].socket != 0) {
			if (0 == osip_strcasecmp(tls_socket_tab[pos].remote_ip, host)
				&& port == tls_socket_tab[pos].remote_port)
				return pos;
		}
	}
	return -1;
}


static int _tls_tl_connect_socket(char *host, int port)
{
	int pos;
	int res;
	struct addrinfo *addrinfo = NULL;
	struct addrinfo *curinfo;
	int sock = -1;
	int ssl_state = 0;

	char src6host[NI_MAXHOST];
	memset(src6host, 0, sizeof(src6host));

	for (pos = 0; pos < EXOSIP_MAX_SOCKETS; pos++) {
		if (tls_socket_tab[pos].socket == 0) {
			break;
		}
	}

	if (pos == EXOSIP_MAX_SOCKETS)
		return -1;

	res = eXosip_get_addrinfo(&addrinfo, host, port, IPPROTO_TCP);
	if (res)
		return -1;


	for (curinfo = addrinfo; curinfo; curinfo = curinfo->ai_next) {
		if (curinfo->ai_protocol && curinfo->ai_protocol != IPPROTO_TCP) {
			OSIP_TRACE(osip_trace
					   (__FILE__, __LINE__, OSIP_INFO2, NULL,
						"eXosip: Skipping protocol %d\n", curinfo->ai_protocol));
			continue;
		}

		res =
			getnameinfo((struct sockaddr *) curinfo->ai_addr, curinfo->ai_addrlen,
						src6host, NI_MAXHOST, NULL, 0, NI_NUMERICHOST);

		if (res == 0) {
			int i = _tls_tl_find_socket(src6host, port);
			if (i >= 0) {
				eXosip_freeaddrinfo(addrinfo);
				return i;
			}
			OSIP_TRACE(osip_trace
					   (__FILE__, __LINE__, OSIP_ERROR, NULL,
						"New binding with %s\n", src6host));
		}

		sock = (int) socket(curinfo->ai_family, curinfo->ai_socktype,
							curinfo->ai_protocol);
		if (sock < 0) {
#if !defined(OSIP_MT) || defined(_WIN32_WCE)
			OSIP_TRACE(osip_trace
					   (__FILE__, __LINE__, OSIP_INFO2, NULL,
						"eXosip: Cannot create socket!\n"));
#else
			OSIP_TRACE(osip_trace
					   (__FILE__, __LINE__, OSIP_INFO2, NULL,
						"eXosip: Cannot create socket %s!\n", strerror(errno)));
#endif
			continue;
		}

		if (curinfo->ai_family == AF_INET6) {
#ifdef IPV6_V6ONLY
			if (setsockopt_ipv6only(sock)) {
				close(sock);
				sock = -1;
#if !defined(OSIP_MT) || defined(_WIN32_WCE)
				OSIP_TRACE(osip_trace
						   (__FILE__, __LINE__, OSIP_INFO2, NULL,
							"eXosip: Cannot set socket option!\n"));
#else
				OSIP_TRACE(osip_trace
						   (__FILE__, __LINE__, OSIP_INFO2, NULL,
							"eXosip: Cannot set socket option %s!\n",
							strerror(errno)));
#endif
				continue;
			}
#endif							/* IPV6_V6ONLY */
		}
#if defined(_WIN32_WCE) || defined(WIN32)
		{
			unsigned long nonBlock = 1;
			int val;

			ioctlsocket(sock, FIONBIO, &nonBlock);

			val = 1;
			if (setsockopt
				(sock, SOL_SOCKET, SO_KEEPALIVE, (char *) &val,
				 sizeof(val)) == -1) {
				close(sock);
				sock = -1;
				OSIP_TRACE(osip_trace
						   (__FILE__, __LINE__, OSIP_INFO2, NULL,
							"Cannot get socket flag!\n"));
				continue;
			}
		}
#if !defined(_WIN32_WCE)
		{
			DWORD err = 0L;
			DWORD dwBytes = 0L;
			struct tcp_keepalive kalive = { 0 };
			struct tcp_keepalive kaliveOut = { 0 };
			kalive.onoff = 1;
			kalive.keepalivetime = 30000;	/* Keep Alive in 5.5 sec. */
			kalive.keepaliveinterval = 3000;	/* Resend if No-Reply */
			err = WSAIoctl(sock, SIO_KEEPALIVE_VALS, &kalive,
						   sizeof(kalive), &kaliveOut, sizeof(kaliveOut), &dwBytes,
						   NULL, NULL);
			if (err != 0) {
				OSIP_TRACE(osip_trace
						   (__FILE__, __LINE__, OSIP_WARNING, NULL,
							"Cannot set keepalive interval!\n"));
			}
		}
#endif
#else
		{
			int val;

			val = fcntl(sock, F_GETFL);
			if (val < 0) {
				close(sock);
				sock = -1;
				OSIP_TRACE(osip_trace
						   (__FILE__, __LINE__, OSIP_INFO2, NULL,
							"Cannot get socket flag!\n"));
				continue;
			}
			val |= O_NONBLOCK;
			if (fcntl(sock, F_SETFL, val) < 0) {
				close(sock);
				sock = -1;
				OSIP_TRACE(osip_trace
						   (__FILE__, __LINE__, OSIP_INFO2, NULL,
							"Cannot set socket flag!\n"));
				continue;
			}
#if 0
			val = 1;
			if (setsockopt(sock, SOL_SOCKET, SO_KEEPALIVE, &val, sizeof(val)) ==
				-1)
				val = 30;		/* 30 sec before starting probes */
			setsockopt(sock, SOL_TCP, TCP_KEEPIDLE, &val, sizeof(val));
			val = 2;			/* 2 probes max */
			setsockopt(sock, SOL_TCP, TCP_KEEPCNT, &val, sizeof(val));
			val = 10;			/* 10 seconds between each probe */
			setsockopt(sock, SOL_TCP, TCP_KEEPINTVL, &val, sizeof(val));
#endif
		}
#endif

		OSIP_TRACE(osip_trace
				   (__FILE__, __LINE__, OSIP_INFO2, NULL,
					"eXosip: socket node:%s , socket %d, family:%d set to non blocking mode\n",
					host, sock, curinfo->ai_family));
		res = connect(sock, curinfo->ai_addr, curinfo->ai_addrlen);
		if (res < 0) {
#ifdef WIN32
			int status = WSAGetLastError();
			if (status != WSAEWOULDBLOCK) {
#else
			if (errno != EINPROGRESS) {
#endif
				OSIP_TRACE(osip_trace
						   (__FILE__, __LINE__, OSIP_INFO2, NULL,
							"Cannot connect socket node:%s family:%d %s[%d]\n",
							host, curinfo->ai_family, strerror(errno), errno));

				close(sock);
				sock = -1;
				continue;
			} else {
				res = _tls_tl_is_connected(sock);
				if (res > 0) {
					OSIP_TRACE(osip_trace
							   (__FILE__, __LINE__, OSIP_INFO2, NULL,
								"socket node:%s, socket %d [pos=%d], family:%d, in progress\n",
								host, sock, pos, curinfo->ai_family));
					break;
				} else if (res == 0) {
					OSIP_TRACE(osip_trace
							   (__FILE__, __LINE__, OSIP_INFO2, NULL,
								"socket node:%s , socket %d [pos=%d], family:%d, connected\n",
								host, sock, pos, curinfo->ai_family));
					ssl_state = 1;
					break;
				} else {
					close(sock);
					sock = -1;
					continue;
				}
			}
		}

		break;
	}

	eXosip_freeaddrinfo(addrinfo);

	if (sock > 0) {
		tls_socket_tab[pos].socket = sock;

		if (src6host[0] == '\0')
			osip_strncpy(tls_socket_tab[pos].remote_ip, host,
						 sizeof(tls_socket_tab[pos].remote_ip) - 1);
		else
			osip_strncpy(tls_socket_tab[pos].remote_ip, src6host,
						 sizeof(tls_socket_tab[pos].remote_ip) - 1);

		tls_socket_tab[pos].remote_port = port;
		tls_socket_tab[pos].ssl_conn = NULL;
		tls_socket_tab[pos].ssl_state = ssl_state;
		tls_socket_tab[pos].ssl_ctx = NULL;

		if (tls_socket_tab[pos].ssl_state == 1) {	/* TCP connected but not TLS connected */
			res = _tls_tl_ssl_connect_socket(pos);
			if (res < 0) {
				SSL_shutdown(tls_socket_tab[pos].ssl_conn);
				close(tls_socket_tab[pos].socket);
				SSL_free(tls_socket_tab[pos].ssl_conn);
				if (tls_socket_tab[pos].ssl_ctx != NULL)
					SSL_CTX_free(tls_socket_tab[pos].ssl_ctx);

				memset(&(tls_socket_tab[pos]), 0, sizeof(tls_socket_tab[pos]));
				return -1;
			}
		}
		return pos;
	}

	return -1;
}

static int
tls_tl_send_message(osip_transaction_t * tr, osip_message_t * sip, char *host,
					int port, int out_socket)
{
	size_t length = 0;
	char *message;
	int i;

	int pos;

	SSL *ssl = NULL;

	if (host == NULL) {
		host = sip->req_uri->host;
		if (sip->req_uri->port != NULL)
			port = osip_atoi(sip->req_uri->port);
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
		osip_message_get_route(sip, 0, &route);

		osip_to_get_tag(sip->to, &tag);
		if (tag == NULL && route != NULL && route->url != NULL) {
			osip_list_remove(&sip->routes, 0);
		}
		i = osip_message_to_str(sip, &message, &length);
		if (tag == NULL && route != NULL && route->url != NULL) {
			osip_list_add(&sip->routes, route, 0);
		}
	}

	if (i != 0 || length <= 0) {
		return -1;
	}

	if (out_socket > 0) {
		for (pos = 0; pos < EXOSIP_MAX_SOCKETS; pos++) {
			if (tls_socket_tab[pos].socket != 0) {
				if (tls_socket_tab[pos].socket == out_socket) {
					out_socket = tls_socket_tab[pos].socket;
					ssl = tls_socket_tab[pos].ssl_conn;
					OSIP_TRACE(osip_trace(__FILE__, __LINE__, OSIP_INFO1, NULL,
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
	if (out_socket <= 0) {
		pos = _tls_tl_find_socket(host, port);

		/* Step 2: create new socket with host:port */
		if (pos < 0) {
			pos = _tls_tl_connect_socket(host, port);
		}
		if (pos >= 0) {
			out_socket = tls_socket_tab[pos].socket;
			ssl = tls_socket_tab[pos].ssl_conn;
		}
	}

	if (out_socket <= 0) {
		osip_free(message);
		return -1;
	}

	if (tls_socket_tab[pos].ssl_state == 0) {
		i = _tls_tl_is_connected(out_socket);
		if (i > 0) {
			time_t now;
			now = time(NULL);
			OSIP_TRACE(osip_trace
					   (__FILE__, __LINE__, OSIP_INFO2, NULL,
						"socket node:%s, socket %d [pos=%d], in progress\n",
						host, out_socket, -1));
			osip_free(message);
			if (tr != NULL && now - tr->birth_time > 10)
				return -1;
			return 1;
		} else if (i == 0) {
			OSIP_TRACE(osip_trace
					   (__FILE__, __LINE__, OSIP_INFO2, NULL,
						"socket node:%s , socket %d [pos=%d], connected\n",
						host, out_socket, -1));
			tls_socket_tab[pos].ssl_state = 1;
		} else {
			OSIP_TRACE(osip_trace
					   (__FILE__, __LINE__, OSIP_ERROR, NULL,
						"socket node:%s, socket %d [pos=%d], socket error\n",
						host, out_socket, -1));
			osip_free(message);
			return -1;
		}
	}

	if (tls_socket_tab[pos].ssl_state == 1) {	/* TCP connected but not TLS connected */
		i = _tls_tl_ssl_connect_socket(pos);
		if (i < 0) {
			SSL_shutdown(tls_socket_tab[pos].ssl_conn);
			close(tls_socket_tab[pos].socket);
			SSL_free(tls_socket_tab[pos].ssl_conn);
			if (tls_socket_tab[pos].ssl_ctx != NULL)
				SSL_CTX_free(tls_socket_tab[pos].ssl_ctx);

			memset(&(tls_socket_tab[pos]), 0, sizeof(tls_socket_tab[pos]));
			return -1;
		} else if (i > 0) {
			OSIP_TRACE(osip_trace
					   (__FILE__, __LINE__, OSIP_INFO2, NULL,
						"socket node:%s, socket %d [pos=%d], connected (ssl in progress)\n",
						host, out_socket, pos));
			return 1;
		}
		ssl = tls_socket_tab[pos].ssl_conn;
	}

	if (ssl == NULL) {
		osip_free(message);
		return -1;
	}

	OSIP_TRACE(osip_trace(__FILE__, __LINE__, OSIP_INFO1, NULL,
						  "Message sent: (to dest=%s:%i) \n%s\n",
						  host, port, message));

	SSL_set_mode(ssl, SSL_MODE_AUTO_RETRY);

	while (1) {
		i = SSL_write(ssl, (const void *) message, length);

		if (i <= 0) {
			i = SSL_get_error(ssl, i);
			if (i == SSL_ERROR_WANT_READ || i == SSL_ERROR_WANT_WRITE)
				continue;
			print_ssl_error(i);

			osip_free(message);
			return -1;
		}
		break;
	}

	osip_free(message);
	return OSIP_SUCCESS;
}

static int tls_tl_keepalive(void)
{
	char buf[4] = "\r\n\r\n";
	int pos;
	int i;

	if (tls_socket<=0)
		return OSIP_UNDEFINED_ERROR;

	for (pos = 0; pos < EXOSIP_MAX_SOCKETS; pos++) {
		if (tls_socket_tab[pos].socket > 0 && tls_socket_tab[pos].ssl_state > 2) {
			SSL_set_mode(tls_socket_tab[pos].ssl_conn, SSL_MODE_AUTO_RETRY);

			while (1) {
				i = SSL_write(tls_socket_tab[pos].ssl_conn, (const void *) buf, 4);

				if (i <= 0) {
					i = SSL_get_error(tls_socket_tab[pos].ssl_conn, i);
					if (i == SSL_ERROR_WANT_READ || i == SSL_ERROR_WANT_WRITE)
						continue;
					print_ssl_error(i);

					return -1;
				}
				break;
			}
		}
	}
	return OSIP_SUCCESS;
}

static int tls_tl_set_socket(int socket)
{
	tls_socket = socket;

	return OSIP_SUCCESS;
}

static int tls_tl_masquerade_contact(const char *public_address, int port)
{
	if (public_address == NULL || public_address[0] == '\0') {
		memset(tls_firewall_ip, '\0', sizeof(tls_firewall_ip));
		memset(tls_firewall_port, '\0', sizeof(tls_firewall_port));
		if (eXtl_tls.proto_port > 0)
			snprintf(tls_firewall_port, sizeof(tls_firewall_port), "%i",
					 eXtl_tls.proto_port);
		return OSIP_SUCCESS;
	}
	snprintf(tls_firewall_ip, sizeof(tls_firewall_ip), "%s", public_address);
	if (port > 0) {
		snprintf(tls_firewall_port, sizeof(tls_firewall_port), "%i", port);
	}
	return OSIP_SUCCESS;
}

static int
tls_tl_get_masquerade_contact(char *ip, int ip_size, char *port, int port_size)
{
	memset(ip, 0, ip_size);
	memset(port, 0, port_size);

	if (tls_firewall_ip[0] != '\0')
		snprintf(ip, ip_size, "%s", tls_firewall_ip);

	if (tls_firewall_port[0] != '\0')
		snprintf(port, port_size, "%s", tls_firewall_port);
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
