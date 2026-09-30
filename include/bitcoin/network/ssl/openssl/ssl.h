/**
 * Copyright (c) 2011-2026 libbitcoin developers
 *
 * This file is part of libbitcoin.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#ifndef LIBBITCOIN_NETWORK_SSL_OPENSSL_SSL_H
#define LIBBITCOIN_NETWORK_SSL_OPENSSL_SSL_H

/* The subset of the OpenSSL 1.1.1 interface referenced by boost::asio::ssl,
 * implemented over the libbitcoin TLS 1.3 server. Functions boost references
 * but does not call for a TLS 1.3 server fail. */

#include <stddef.h>
#include <bitcoin/network/preprocessor.hpp>

/* The include guards of the OpenSSL headers that boost::asio includes, so
 * that an OpenSSL elsewhere on the include path is excluded. */
#define OPENSSL_CONF_H
#define HEADER_CONF_H
#define OPENSSL_SSL_H
#define HEADER_SSL_H
#define OPENSSL_DH_H
#define HEADER_DH_H
#define OPENSSL_ERR_H
#define HEADER_ERR_H
#define OPENSSL_RSA_H
#define HEADER_RSA_H
#define OPENSSL_X509_H
#define HEADER_X509_H
#define OPENSSL_X509V3_H
#define HEADER_X509V3_H
#define OPENSSL_ENGINE_H
#define HEADER_ENGINE_H

#define OPENSSL_VERSION_NUMBER 0x1010100fL
#define OPENSSL_VERSION 0
#define OPENSSL_NO_SSL2
#define OPENSSL_NO_SSL3
#define OPENSSL_NO_ENGINE
#define OPENSSL_NO_COMP

/* Opaque types. */
typedef struct ssl_st SSL;
typedef struct ssl_ctx_st SSL_CTX;
typedef struct ssl_method_st SSL_METHOD;
typedef struct ssl_comp_st SSL_COMP;
typedef struct bio_st BIO;
typedef struct x509_st X509;
typedef struct x509_store_st X509_STORE;
typedef struct x509_store_ctx_st X509_STORE_CTX;
typedef struct evp_pkey_st EVP_PKEY;
typedef struct rsa_st RSA;
typedef struct dh_st DH;
typedef struct stack_st_X509 STACK_OF_X509;
#define STACK_OF(type) struct stack_st_##type

typedef int pem_password_cb(char* buffer, int size, int rwflag,
    void* userdata);
typedef int (*SSL_verify_cb)(int preverified, X509_STORE_CTX* context);

/* Protocol versions. */
#define SSL3_VERSION 0x0300
#define TLS1_VERSION 0x0301
#define TLS1_1_VERSION 0x0302
#define TLS1_2_VERSION 0x0303
#define TLS1_3_VERSION 0x0304

/* SSL_get_error results. */
#define SSL_ERROR_NONE 0
#define SSL_ERROR_SSL 1
#define SSL_ERROR_WANT_READ 2
#define SSL_ERROR_WANT_WRITE 3
#define SSL_ERROR_WANT_X509_LOOKUP 4
#define SSL_ERROR_SYSCALL 5
#define SSL_ERROR_ZERO_RETURN 6

/* Modes. */
#define SSL_MODE_ENABLE_PARTIAL_WRITE 0x00000001L
#define SSL_MODE_ACCEPT_MOVING_WRITE_BUFFER 0x00000002L
#define SSL_MODE_RELEASE_BUFFERS 0x00000010L

/* Options (recorded, protocol versions are fixed at TLS 1.3). */
#define SSL_OP_ALL 0x80000854L
#define SSL_OP_NO_COMPRESSION 0x00020000L
#define SSL_OP_SINGLE_DH_USE 0x00000000L
#define SSL_OP_NO_SSLv2 0x00000000L
#define SSL_OP_NO_SSLv3 0x02000000L
#define SSL_OP_NO_TLSv1 0x04000000L
#define SSL_OP_NO_TLSv1_2 0x08000000L
#define SSL_OP_NO_TLSv1_1 0x10000000L
#define SSL_OP_NO_TLSv1_3 0x20000000L

/* File types. */
#define SSL_FILETYPE_PEM 1
#define SSL_FILETYPE_ASN1 2

/* Verification. */
#define SSL_VERIFY_NONE 0x00
#define SSL_VERIFY_PEER 0x01
#define SSL_VERIFY_FAIL_IF_NO_PEER_CERT 0x02
#define SSL_VERIFY_CLIENT_ONCE 0x04
#define X509_V_OK 0

/* Shutdown state. */
#define SSL_SENT_SHUTDOWN 1
#define SSL_RECEIVED_SHUTDOWN 2

/* Error queue codes. */
#define ERR_LIB_SYS 2
#define ERR_LIB_EVP 6
#define ERR_LIB_PEM 9
#define ERR_LIB_SSL 20
#define ERR_R_PEM_LIB 9
#define EVP_R_EXPECTING_AN_RSA_KEY 127
#define SSL_R_SHORT_READ 219
#define PEM_R_NO_START_LINE 108
#define SSL_AD_REASON_OFFSET 1000
#define ERR_PACK(lib, func, reason) \
    ((((unsigned long)(lib) & 0xffUL) << 24) | ((unsigned long)(reason) & 0xfffUL))
#define ERR_GET_LIB(code) ((int)(((unsigned long)(code) >> 24) & 0xffUL))
#define ERR_GET_REASON(code) ((int)((unsigned long)(code) & 0xfffUL))

/* Application data (ex_data index zero). */
#define SSL_get_app_data(ssl) SSL_get_ex_data(ssl, 0)
#define SSL_set_app_data(ssl, data) SSL_set_ex_data(ssl, 0, (void*)(data))
#define SSL_CTX_get_app_data(ctx) SSL_CTX_get_ex_data(ctx, 0)
#define SSL_CTX_set_app_data(ctx, data) SSL_CTX_set_ex_data(ctx, 0, (void*)(data))

#ifdef __cplusplus
extern "C" {
#endif

/* Library. */
BCT_API const char* OpenSSL_version(int type);
BCT_API void CONF_modules_unload(int all);
BCT_API void OPENSSL_free(void* pointer);

/* Methods and contexts. */
BCT_API const SSL_METHOD* TLS_method(void);
BCT_API const SSL_METHOD* TLS_client_method(void);
BCT_API const SSL_METHOD* TLS_server_method(void);
BCT_API const SSL_METHOD* SSLv23_method(void);
BCT_API const SSL_METHOD* SSLv23_client_method(void);
BCT_API const SSL_METHOD* SSLv23_server_method(void);
BCT_API SSL_CTX* SSL_CTX_new(const SSL_METHOD* method);
BCT_API void SSL_CTX_free(SSL_CTX* ctx);
BCT_API unsigned long SSL_CTX_set_options(SSL_CTX* ctx, unsigned long options);
BCT_API unsigned long SSL_CTX_clear_options(SSL_CTX* ctx,
    unsigned long options);
BCT_API unsigned long SSL_CTX_get_options(const SSL_CTX* ctx);
BCT_API int SSL_CTX_set_min_proto_version(SSL_CTX* ctx, int version);
BCT_API int SSL_CTX_set_max_proto_version(SSL_CTX* ctx, int version);
BCT_API void* SSL_CTX_get_ex_data(const SSL_CTX* ctx, int index);
BCT_API int SSL_CTX_set_ex_data(SSL_CTX* ctx, int index, void* data);

/* Context verification. */
BCT_API void SSL_CTX_set_verify(SSL_CTX* ctx, int mode, SSL_verify_cb callback);
BCT_API int SSL_CTX_get_verify_mode(const SSL_CTX* ctx);
BCT_API SSL_verify_cb SSL_CTX_get_verify_callback(const SSL_CTX* ctx);
BCT_API void SSL_CTX_set_verify_depth(SSL_CTX* ctx, int depth);
BCT_API int SSL_CTX_load_verify_locations(SSL_CTX* ctx, const char* file,
    const char* path);
BCT_API int SSL_CTX_set_default_verify_paths(SSL_CTX* ctx);
BCT_API X509_STORE* SSL_CTX_get_cert_store(const SSL_CTX* ctx);
BCT_API int X509_STORE_add_cert(X509_STORE* store, X509* certificate);

/* Context credentials. */
BCT_API int SSL_CTX_use_certificate(SSL_CTX* ctx, X509* certificate);
BCT_API int SSL_CTX_use_certificate_ASN1(SSL_CTX* ctx, int size,
    const unsigned char* data);
BCT_API int SSL_CTX_use_certificate_file(SSL_CTX* ctx, const char* file,
    int type);
BCT_API int SSL_CTX_use_certificate_chain_file(SSL_CTX* ctx,
    const char* file);
BCT_API long SSL_CTX_add_extra_chain_cert(SSL_CTX* ctx, X509* certificate);
BCT_API int SSL_CTX_clear_chain_certs(SSL_CTX* ctx);
BCT_API int SSL_CTX_use_PrivateKey(SSL_CTX* ctx, EVP_PKEY* key);
BCT_API int SSL_CTX_use_PrivateKey_file(SSL_CTX* ctx, const char* file,
    int type);
BCT_API int SSL_CTX_use_RSAPrivateKey(SSL_CTX* ctx, RSA* key);
BCT_API int SSL_CTX_use_RSAPrivateKey_file(SSL_CTX* ctx, const char* file,
    int type);
BCT_API long SSL_CTX_set_tmp_dh(SSL_CTX* ctx, DH* parameters);
BCT_API void SSL_CTX_set_default_passwd_cb(SSL_CTX* ctx,
    pem_password_cb* callback);
BCT_API void SSL_CTX_set_default_passwd_cb_userdata(SSL_CTX* ctx,
    void* userdata);
BCT_API pem_password_cb* SSL_CTX_get_default_passwd_cb(SSL_CTX* ctx);
BCT_API void* SSL_CTX_get_default_passwd_cb_userdata(SSL_CTX* ctx);

/* Connections. */
BCT_API SSL* SSL_new(SSL_CTX* ctx);
BCT_API void SSL_free(SSL* ssl);
BCT_API SSL_CTX* SSL_get_SSL_CTX(const SSL* ssl);
BCT_API long SSL_set_mode(SSL* ssl, long mode);
BCT_API void SSL_set_bio(SSL* ssl, BIO* read, BIO* write);
BCT_API int SSL_accept(SSL* ssl);
BCT_API int SSL_connect(SSL* ssl);
BCT_API int SSL_read(SSL* ssl, void* buffer, int size);
BCT_API int SSL_write(SSL* ssl, const void* buffer, int size);
BCT_API int SSL_shutdown(SSL* ssl);
BCT_API int SSL_get_error(const SSL* ssl, int result);
BCT_API int SSL_get_shutdown(const SSL* ssl);
BCT_API int SSL_version(const SSL* ssl);
BCT_API void* SSL_get_ex_data(const SSL* ssl, int index);
BCT_API int SSL_set_ex_data(SSL* ssl, int index, void* data);
BCT_API int SSL_get_ex_data_X509_STORE_CTX_idx(void);
BCT_API void SSL_set_verify(SSL* ssl, int mode, SSL_verify_cb callback);
BCT_API int SSL_get_verify_mode(const SSL* ssl);
BCT_API SSL_verify_cb SSL_get_verify_callback(const SSL* ssl);
BCT_API void SSL_set_verify_depth(SSL* ssl, int depth);
BCT_API long SSL_get_verify_result(const SSL* ssl);
BCT_API X509* SSL_get_peer_certificate(const SSL* ssl);

/* Memory BIO pairs and files. */
BCT_API int BIO_new_bio_pair(BIO** first, size_t first_size, BIO** second,
    size_t second_size);
BCT_API BIO* BIO_new_file(const char* file, const char* mode);
BCT_API BIO* BIO_new_mem_buf(const void* data, int size);
BCT_API int BIO_free(BIO* bio);
BCT_API int BIO_read(BIO* bio, void* buffer, int size);
BCT_API int BIO_write(BIO* bio, const void* buffer, int size);
BCT_API size_t BIO_ctrl_pending(BIO* bio);
BCT_API size_t BIO_wpending(BIO* bio);

/* Error queue (per thread). */
BCT_API void ERR_clear_error(void);
BCT_API unsigned long ERR_get_error(void);
BCT_API unsigned long ERR_peek_error(void);
BCT_API unsigned long ERR_peek_last_error(void);
BCT_API const char* ERR_lib_error_string(unsigned long code);
BCT_API const char* ERR_func_error_string(unsigned long code);
BCT_API const char* ERR_reason_error_string(unsigned long code);
BCT_API void ERR_error_string_n(unsigned long code, char* buffer, size_t size);

/* Keys, certificates and parameters (not supported as objects). */
BCT_API X509* PEM_read_bio_X509(BIO* bio, X509** out, pem_password_cb* cb,
    void* userdata);
BCT_API X509* PEM_read_bio_X509_AUX(BIO* bio, X509** out,
    pem_password_cb* cb, void* userdata);
BCT_API EVP_PKEY* PEM_read_bio_PrivateKey(BIO* bio, EVP_PKEY** out,
    pem_password_cb* cb, void* userdata);
BCT_API RSA* PEM_read_bio_RSAPrivateKey(BIO* bio, RSA** out,
    pem_password_cb* cb, void* userdata);
BCT_API DH* PEM_read_bio_DHparams(BIO* bio, DH** out, pem_password_cb* cb,
    void* userdata);
BCT_API EVP_PKEY* d2i_PrivateKey_bio(BIO* bio, EVP_PKEY** out);
BCT_API RSA* d2i_RSAPrivateKey_bio(BIO* bio, RSA** out);
BCT_API void X509_free(X509* certificate);
BCT_API void EVP_PKEY_free(EVP_PKEY* key);
BCT_API void RSA_free(RSA* key);
BCT_API void DH_free(DH* parameters);
BCT_API void sk_X509_pop_free(STACK_OF_X509* stack, void (*free)(X509*));

/* Verification contexts and host checks (verify callbacks are not called). */
BCT_API void* X509_STORE_CTX_get_ex_data(const X509_STORE_CTX* context,
    int index);
BCT_API X509* X509_STORE_CTX_get_current_cert(const X509_STORE_CTX* context);
BCT_API int X509_STORE_CTX_get_error_depth(const X509_STORE_CTX* context);
BCT_API int X509_check_host(X509* certificate, const char* name, size_t size,
    unsigned int flags, char** peer);
BCT_API int X509_check_ip_asc(X509* certificate, const char* address,
    unsigned int flags);

#ifdef __cplusplus
}
#endif

#endif
