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
#include <openssl/ssl.h>

#include <algorithm>
#include <cstdlib>
#include <deque>
#include <filesystem>
#include <iterator>
#include <memory>
#include <string>
#include <bitcoin/network/define.hpp>
#include <bitcoin/network/tls/tls.hpp>

using namespace libbitcoin;
using namespace libbitcoin::system;
using namespace libbitcoin::network;

BC_PUSH_WARNING(NO_NEW_OR_DELETE)
BC_PUSH_WARNING(NO_REINTERPRET_CAST)
BC_PUSH_WARNING(NO_POINTER_ARITHMETIC)
BC_PUSH_WARNING(NO_THROW_IN_NOEXCEPT)

// Method identities (only their addresses are significant).
struct ssl_method_st
{
    bool server;
    bool client;
};

static const SSL_METHOD any_method{ true, true };
static const SSL_METHOD client_method{ false, true };
static const SSL_METHOD server_method{ true, false };

// A memory BIO pair shares two byte queues, one for each direction.
struct bio_pair
{
    std::deque<uint8_t> forward{};
    std::deque<uint8_t> backward{};
};

struct bio_st
{
    std::shared_ptr<bio_pair> pair;
    bool first;

    std::deque<uint8_t>& incoming() NOEXCEPT
    {
        return first ? pair->backward : pair->forward;
    }

    std::deque<uint8_t>& outgoing() NOEXCEPT
    {
        return first ? pair->forward : pair->backward;
    }
};

struct x509_store_st
{
};

struct ssl_ctx_st
{
    const SSL_METHOD* method{};
    tls::context context{};
    unsigned long options{};
    int verify_mode{};
    SSL_verify_cb verify_callback{};
    pem_password_cb* password_callback{};
    void* password_userdata{};
    void* data{};
    x509_store_st store{};
};

struct ssl_st
{
    SSL_CTX* ctx{};
    BIO* bio{};
    std::unique_ptr<tls::server> server{};
    int error{};
    int shutdown{};
    int verify_mode{};
    SSL_verify_cb verify_callback{};
    void* data{};
};

// Error queue.
// ----------------------------------------------------------------------------

static std::deque<unsigned long>& errors() NOEXCEPT
{
    static thread_local std::deque<unsigned long> queue{};
    return queue;
}

static void push_error(int library, int reason) NOEXCEPT
{
    errors().push_back(ERR_PACK(library, 0, reason));
}

static std::string alert_text(int description) NOEXCEPT
{
    switch (description)
    {
        case tls::alert::unexpected_message: return "unexpected message";
        case tls::alert::bad_record_mac: return "bad record mac";
        case tls::alert::record_overflow: return "record overflow";
        case tls::alert::handshake_failure: return "handshake failure";
        case tls::alert::bad_certificate: return "bad certificate";
        case tls::alert::unsupported_certificate:
            return "unsupported certificate";
        case tls::alert::certificate_expired: return "certificate expired";
        case tls::alert::certificate_unknown: return "certificate unknown";
        case tls::alert::illegal_parameter: return "illegal parameter";
        case tls::alert::unknown_ca: return "unknown ca";
        case tls::alert::decode_error: return "decode error";
        case tls::alert::decrypt_error: return "decrypt error";
        case tls::alert::protocol_version: return "protocol version";
        case tls::alert::internal_error: return "internal error";
        case tls::alert::missing_extension: return "missing extension";
        case tls::alert::certificate_required: return "certificate required";
        default: return "alert " + std::to_string(description);
    }
}

// Files.
// ----------------------------------------------------------------------------

static bool read_file(std::string& out, const char* file) NOEXCEPT
{
    if (file == nullptr)
        return false;

    ifstream stream{ std::filesystem::path{ to_path(file) } };
    if (!stream.good())
        return false;

    out.assign(std::istreambuf_iterator<char>(stream), {});
    return true;
}

static std::string password(SSL_CTX* ctx) NOEXCEPT
{
    if (ctx->password_callback == nullptr)
        return {};

    std::string buffer(1024, '\0');
    const auto size = ctx->password_callback(buffer.data(),
        possible_narrow_sign_cast<int>(buffer.size()), 0,
        ctx->password_userdata);

    const auto count = (size <= 0) ? zero :
        possible_narrow_sign_cast<size_t>(size);
    const std::string value{ buffer.data(), count };
    wipe(buffer.data(), buffer.size());
    return value;
}

// Connection I/O.
// ----------------------------------------------------------------------------

// Feed pending input to the server and publish its output.
static void pump(SSL* ssl) NOEXCEPT
{
    auto& incoming = ssl->bio->incoming();
    if (!incoming.empty())
    {
        const data_chunk data(incoming.begin(), incoming.end());
        incoming.clear();
        ssl->server->receive(data);
    }

    auto& output = ssl->server->output();
    auto& outgoing = ssl->bio->outgoing();
    outgoing.insert(outgoing.end(), output.begin(), output.end());
    output.clear();
}

static int failed(SSL* ssl) NOEXCEPT
{
    const auto reason = SSL_AD_REASON_OFFSET + ssl->server->failure();
    push_error(ERR_LIB_SSL, reason);
    ssl->error = SSL_ERROR_SSL;
    return -1;
}

static int want_read(SSL* ssl) NOEXCEPT
{
    ssl->error = SSL_ERROR_WANT_READ;
    return -1;
}

static bool ready(SSL* ssl) NOEXCEPT
{
    return ssl->server && (ssl->bio != nullptr);
}

extern "C" {

// Library.
// ----------------------------------------------------------------------------

const char* OpenSSL_version(int) { return "libbitcoin tls 1.3"; }
void CONF_modules_unload(int) {}
void OPENSSL_free(void* pointer) { std::free(pointer); }

// Methods and contexts.
// ----------------------------------------------------------------------------

const SSL_METHOD* TLS_method(void) { return &any_method; }
const SSL_METHOD* TLS_client_method(void) { return &client_method; }
const SSL_METHOD* TLS_server_method(void) { return &server_method; }
const SSL_METHOD* SSLv23_method(void) { return &any_method; }
const SSL_METHOD* SSLv23_client_method(void) { return &client_method; }
const SSL_METHOD* SSLv23_server_method(void) { return &server_method; }

SSL_CTX* SSL_CTX_new(const SSL_METHOD* method)
{
    if (method == nullptr)
        return nullptr;

    const auto ctx = new SSL_CTX{};
    ctx->method = method;
    return ctx;
}

void SSL_CTX_free(SSL_CTX* ctx)
{
    delete ctx;
}

unsigned long SSL_CTX_set_options(SSL_CTX* ctx, unsigned long options)
{
    return ctx->options |= options;
}

unsigned long SSL_CTX_clear_options(SSL_CTX* ctx, unsigned long options)
{
    return ctx->options &= ~options;
}

unsigned long SSL_CTX_get_options(const SSL_CTX* ctx)
{
    return ctx->options;
}

// Only TLS 1.3 is implemented, so a range must include it.
int SSL_CTX_set_min_proto_version(SSL_CTX*, int version)
{
    return (version == 0) || (version <= TLS1_3_VERSION) ? 1 : 0;
}

int SSL_CTX_set_max_proto_version(SSL_CTX*, int version)
{
    return (version == 0) || (version >= TLS1_3_VERSION) ? 1 : 0;
}

void* SSL_CTX_get_ex_data(const SSL_CTX* ctx, int index)
{
    return is_zero(index) ? ctx->data : nullptr;
}

int SSL_CTX_set_ex_data(SSL_CTX* ctx, int index, void* data)
{
    if (!is_zero(index))
        return 0;

    ctx->data = data;
    return 1;
}

// Context verification.
// ----------------------------------------------------------------------------

void SSL_CTX_set_verify(SSL_CTX* ctx, int mode, SSL_verify_cb callback)
{
    ctx->verify_mode = mode;
    ctx->verify_callback = callback;
    const auto request = !is_zero(mode & SSL_VERIFY_PEER);
    const auto require = !is_zero(mode & SSL_VERIFY_FAIL_IF_NO_PEER_CERT);
    ctx->context.set_verify(request, require);
}

int SSL_CTX_get_verify_mode(const SSL_CTX* ctx)
{
    return ctx->verify_mode;
}

SSL_verify_cb SSL_CTX_get_verify_callback(const SSL_CTX* ctx)
{
    return ctx->verify_callback;
}

void SSL_CTX_set_verify_depth(SSL_CTX*, int)
{
}

// A path is a directory of certificate files, each of which is loaded (no
// hashed names are required). Files that are not certificates are skipped.
int SSL_CTX_load_verify_locations(SSL_CTX* ctx, const char* file,
    const char* path)
{
    if ((file == nullptr) && (path == nullptr))
        return 0;

    if (file != nullptr)
    {
        std::string text{};
        if (!read_file(text, file) || !ctx->context.add_anchors(text))
        {
            push_error(ERR_LIB_SSL, 0);
            return 0;
        }
    }

    if (path != nullptr)
    {
        std::error_code ec{};
        std::filesystem::directory_iterator it{ to_path(path), ec };
        if (ec)
        {
            push_error(ERR_LIB_SSL, 0);
            return 0;
        }

        for (const auto& entry: it)
        {
            std::string text{};
            const auto name = from_path(entry.path());
            if (entry.is_regular_file(ec) && read_file(text, name.c_str()))
                ctx->context.add_anchors(text);
        }
    }

    return 1;
}

// There are no system certificate stores.
int SSL_CTX_set_default_verify_paths(SSL_CTX*)
{
    push_error(ERR_LIB_SSL, 0);
    return 0;
}

X509_STORE* SSL_CTX_get_cert_store(const SSL_CTX* ctx)
{
    return const_cast<X509_STORE*>(&ctx->store);
}

int X509_STORE_add_cert(X509_STORE*, X509*) { return 0; }

// Context credentials.
// ----------------------------------------------------------------------------

int SSL_CTX_use_certificate_chain_file(SSL_CTX* ctx, const char* file)
{
    std::string text{};
    if (!read_file(text, file) || !ctx->context.set_chain(text))
    {
        push_error(ERR_LIB_SSL, 0);
        return 0;
    }

    return 1;
}

int SSL_CTX_use_certificate_file(SSL_CTX* ctx, const char* file, int type)
{
    if (type != SSL_FILETYPE_PEM)
        return 0;

    return SSL_CTX_use_certificate_chain_file(ctx, file);
}

int SSL_CTX_use_PrivateKey_file(SSL_CTX* ctx, const char* file, int type)
{
    std::string text{};
    if ((type != SSL_FILETYPE_PEM) || !read_file(text, file))
    {
        push_error(ERR_LIB_SSL, 0);
        return 0;
    }

    auto secret = password(ctx);
    const auto set = ctx->context.set_key(text, secret);
    wipe(text.data(), text.size());
    wipe(secret.data(), secret.size());
    if (!set)
    {
        push_error(ERR_LIB_SSL, 0);
        return 0;
    }

    return 1;
}

int SSL_CTX_use_certificate(SSL_CTX*, X509*) { return 0; }
int SSL_CTX_use_certificate_ASN1(SSL_CTX*, int, const unsigned char*)
{
    return 0;
}
long SSL_CTX_add_extra_chain_cert(SSL_CTX*, X509*) { return 0; }
int SSL_CTX_clear_chain_certs(SSL_CTX*) { return 1; }
int SSL_CTX_use_PrivateKey(SSL_CTX*, EVP_PKEY*) { return 0; }
int SSL_CTX_use_RSAPrivateKey(SSL_CTX*, RSA*) { return 0; }
int SSL_CTX_use_RSAPrivateKey_file(SSL_CTX*, const char*, int) { return 0; }
long SSL_CTX_set_tmp_dh(SSL_CTX*, DH*) { return 0; }

void SSL_CTX_set_default_passwd_cb(SSL_CTX* ctx, pem_password_cb* callback)
{
    ctx->password_callback = callback;
}

void SSL_CTX_set_default_passwd_cb_userdata(SSL_CTX* ctx, void* userdata)
{
    ctx->password_userdata = userdata;
}

pem_password_cb* SSL_CTX_get_default_passwd_cb(SSL_CTX* ctx)
{
    return ctx->password_callback;
}

void* SSL_CTX_get_default_passwd_cb_userdata(SSL_CTX* ctx)
{
    return ctx->password_userdata;
}

// Connections.
// ----------------------------------------------------------------------------

SSL* SSL_new(SSL_CTX* ctx)
{
    if (ctx == nullptr)
        return nullptr;

    const auto ssl = new SSL{};
    ssl->ctx = ctx;
    ssl->verify_mode = ctx->verify_mode;
    ssl->verify_callback = ctx->verify_callback;
    return ssl;
}

void SSL_free(SSL* ssl)
{
    if (ssl == nullptr)
        return;

    BIO_free(ssl->bio);
    delete ssl;
}

SSL_CTX* SSL_get_SSL_CTX(const SSL* ssl)
{
    return ssl->ctx;
}

long SSL_set_mode(SSL*, long mode)
{
    return mode;
}

void SSL_set_bio(SSL* ssl, BIO* read, BIO* write)
{
    // The engine always sets one end of a pair as both.
    BC_ASSERT(read == write);
    if (ssl->bio != read)
        BIO_free(ssl->bio);

    ssl->bio = read;
    static_cast<void>(write);
}

int SSL_accept(SSL* ssl)
{
    if (!ssl->server)
    {
        if (!ssl->ctx->method->server || !ssl->ctx->context.is_ready())
        {
            push_error(ERR_LIB_SSL, SSL_AD_REASON_OFFSET +
                tls::alert::internal_error);
            ssl->error = SSL_ERROR_SSL;
            return -1;
        }

        ssl->server = std::make_unique<tls::server>(ssl->ctx->context);
    }

    if (ssl->bio == nullptr)
        return want_read(ssl);

    pump(ssl);
    if (ssl->server->is_failed())
        return failed(ssl);

    if (ssl->server->is_established())
    {
        ssl->error = SSL_ERROR_NONE;
        return 1;
    }

    return want_read(ssl);
}

// The client role is not implemented.
int SSL_connect(SSL* ssl)
{
    push_error(ERR_LIB_SSL, SSL_AD_REASON_OFFSET +
        tls::alert::handshake_failure);
    ssl->error = SSL_ERROR_SSL;
    return -1;
}

int SSL_read(SSL* ssl, void* buffer, int size)
{
    if (!ready(ssl))
        return want_read(ssl);

    pump(ssl);
    if (!is_zero(ssl->server->readable()))
    {
        const auto count = ssl->server->read({ static_cast<uint8_t*>(buffer),
            possible_narrow_sign_cast<size_t>(size) });
        ssl->error = SSL_ERROR_NONE;
        return possible_narrow_sign_cast<int>(count);
    }

    if (ssl->server->is_closed())
    {
        ssl->shutdown |= SSL_RECEIVED_SHUTDOWN;
        ssl->error = SSL_ERROR_ZERO_RETURN;
        return 0;
    }

    if (ssl->server->is_failed())
        return failed(ssl);

    return want_read(ssl);
}

int SSL_write(SSL* ssl, const void* buffer, int size)
{
    if (!ready(ssl) || ssl->server->is_failed())
    {
        push_error(ERR_LIB_SSL, 0);
        ssl->error = SSL_ERROR_SSL;
        return -1;
    }

    const auto data = static_cast<const uint8_t*>(buffer);
    const auto count = possible_narrow_sign_cast<size_t>(size);
    if (!ssl->server->write({ data, count }))
    {
        push_error(ERR_LIB_SSL, 0);
        ssl->error = SSL_ERROR_SSL;
        return -1;
    }

    pump(ssl);
    ssl->error = SSL_ERROR_NONE;
    return size;
}

// Returns zero once close_notify is sent and one once the peer's is received.
int SSL_shutdown(SSL* ssl)
{
    if (!ready(ssl) || ssl->server->is_failed())
    {
        ssl->error = SSL_ERROR_NONE;
        return 1;
    }

    const auto sent = !is_zero(ssl->shutdown & SSL_SENT_SHUTDOWN);
    if (!sent)
    {
        ssl->server->close();
        ssl->shutdown |= SSL_SENT_SHUTDOWN;
    }

    pump(ssl);
    if (ssl->server->is_closed())
    {
        ssl->shutdown |= SSL_RECEIVED_SHUTDOWN;
        ssl->error = SSL_ERROR_NONE;
        return 1;
    }

    if (!sent)
    {
        ssl->error = SSL_ERROR_NONE;
        return 0;
    }

    return want_read(ssl);
}

int SSL_get_error(const SSL* ssl, int result)
{
    return (result > 0) ? SSL_ERROR_NONE : ssl->error;
}

int SSL_get_shutdown(const SSL* ssl)
{
    return ssl->shutdown;
}

int SSL_version(const SSL*)
{
    return TLS1_3_VERSION;
}

void* SSL_get_ex_data(const SSL* ssl, int index)
{
    return is_zero(index) ? ssl->data : nullptr;
}

int SSL_set_ex_data(SSL* ssl, int index, void* data)
{
    if (!is_zero(index))
        return 0;

    ssl->data = data;
    return 1;
}

int SSL_get_ex_data_X509_STORE_CTX_idx(void)
{
    return 0;
}

void SSL_set_verify(SSL* ssl, int mode, SSL_verify_cb callback)
{
    ssl->verify_mode = mode;
    ssl->verify_callback = callback;
}

int SSL_get_verify_mode(const SSL* ssl)
{
    return ssl->verify_mode;
}

SSL_verify_cb SSL_get_verify_callback(const SSL* ssl)
{
    return ssl->verify_callback;
}

void SSL_set_verify_depth(SSL*, int)
{
}

long SSL_get_verify_result(const SSL*)
{
    return X509_V_OK;
}

X509* SSL_get_peer_certificate(const SSL*)
{
    return nullptr;
}

// Memory BIO pairs and files.
// ----------------------------------------------------------------------------

int BIO_new_bio_pair(BIO** first, size_t, BIO** second, size_t)
{
    const auto pair = std::make_shared<bio_pair>();
    *first = new BIO{ pair, true };
    *second = new BIO{ pair, false };
    return 1;
}

BIO* BIO_new_file(const char*, const char*) { return nullptr; }
BIO* BIO_new_mem_buf(const void*, int) { return nullptr; }

int BIO_free(BIO* bio)
{
    delete bio;
    return 1;
}

int BIO_read(BIO* bio, void* buffer, int size)
{
    auto& incoming = bio->incoming();
    if (incoming.empty())
        return -1;

    const auto count = std::min(incoming.size(),
        possible_narrow_sign_cast<size_t>(size));
    const auto end = std::next(incoming.begin(), count);
    std::copy(incoming.begin(), end, static_cast<uint8_t*>(buffer));
    incoming.erase(incoming.begin(), end);
    return possible_narrow_sign_cast<int>(count);
}

int BIO_write(BIO* bio, const void* buffer, int size)
{
    const auto data = static_cast<const uint8_t*>(buffer);
    auto& outgoing = bio->outgoing();
    outgoing.insert(outgoing.end(), data, std::next(data, size));
    return size;
}

// Bytes readable from this end.
size_t BIO_ctrl_pending(BIO* bio)
{
    return bio->incoming().size();
}

// Bytes written to this end and not yet read by the other.
size_t BIO_wpending(BIO* bio)
{
    return bio->outgoing().size();
}

// Error queue.
// ----------------------------------------------------------------------------

void ERR_clear_error(void)
{
    errors().clear();
}

unsigned long ERR_get_error(void)
{
    auto& queue = errors();
    if (queue.empty())
        return 0;

    const auto code = queue.front();
    queue.pop_front();
    return code;
}

unsigned long ERR_peek_error(void)
{
    const auto& queue = errors();
    return queue.empty() ? 0 : queue.front();
}

unsigned long ERR_peek_last_error(void)
{
    const auto& queue = errors();
    return queue.empty() ? 0 : queue.back();
}

const char* ERR_lib_error_string(unsigned long code)
{
    return (ERR_GET_LIB(code) == ERR_LIB_SSL) ? "SSL routines" : nullptr;
}

const char* ERR_func_error_string(unsigned long)
{
    return nullptr;
}

// Strings of alert reasons are per thread, valid until the next call.
const char* ERR_reason_error_string(unsigned long code)
{
    static thread_local std::string text{};
    const auto reason = ERR_GET_REASON(code);
    if (reason < SSL_AD_REASON_OFFSET)
        return is_zero(code) ? nullptr : "tls failure";

    text = "tls alert " + alert_text(reason - SSL_AD_REASON_OFFSET);
    return text.c_str();
}

void ERR_error_string_n(unsigned long code, char* buffer, size_t size)
{
    if (is_zero(size))
        return;

    const auto reason = ERR_reason_error_string(code);
    const std::string text{ reason == nullptr ? "" : reason };
    const auto count = std::min(text.size(), sub1(size));
    std::copy_n(text.begin(), count, buffer);
    buffer[count] = '\0';
}

// Keys, certificates and parameters (not supported as objects).
// ----------------------------------------------------------------------------

X509* PEM_read_bio_X509(BIO*, X509**, pem_password_cb*, void*)
{
    return nullptr;
}

X509* PEM_read_bio_X509_AUX(BIO*, X509**, pem_password_cb*, void*)
{
    return nullptr;
}

EVP_PKEY* PEM_read_bio_PrivateKey(BIO*, EVP_PKEY**, pem_password_cb*, void*)
{
    return nullptr;
}

RSA* PEM_read_bio_RSAPrivateKey(BIO*, RSA**, pem_password_cb*, void*)
{
    return nullptr;
}

DH* PEM_read_bio_DHparams(BIO*, DH**, pem_password_cb*, void*)
{
    return nullptr;
}

EVP_PKEY* d2i_PrivateKey_bio(BIO*, EVP_PKEY**) { return nullptr; }
RSA* d2i_RSAPrivateKey_bio(BIO*, RSA**) { return nullptr; }
void X509_free(X509*) {}
void EVP_PKEY_free(EVP_PKEY*) {}
void RSA_free(RSA*) {}
void DH_free(DH*) {}
void sk_X509_pop_free(STACK_OF_X509*, void (*)(X509*)) {}

// Verification contexts and host checks.
// ----------------------------------------------------------------------------

void* X509_STORE_CTX_get_ex_data(const X509_STORE_CTX*, int)
{
    return nullptr;
}

X509* X509_STORE_CTX_get_current_cert(const X509_STORE_CTX*)
{
    return nullptr;
}

int X509_STORE_CTX_get_error_depth(const X509_STORE_CTX*)
{
    return 0;
}

int X509_check_host(X509*, const char*, size_t, unsigned int, char**)
{
    return 0;
}

int X509_check_ip_asc(X509*, const char*, unsigned int)
{
    return 0;
}

} // extern "C"

BC_POP_WARNING()
BC_POP_WARNING()
BC_POP_WARNING()
BC_POP_WARNING()
