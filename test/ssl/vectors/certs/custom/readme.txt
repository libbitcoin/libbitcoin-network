# server
# =============================================================================

[clear]
key_path =
cert_path =

[private]
key_path = m:/certs/server/server-ecc384-key.pem
cert_path = m:/certs/server/server-ecc384-cert.pem

[private.encrypted]
key_pass = libbitcoin
key_path = m:/certs/server/server-ecc384-key-enc.pem
cert_path = m:/certs/server/server-ecc384-cert.pem

[private.authenticated]
key_path = m:/certs/server/server-ecc384-key.pem
cert_path = m:/certs/server/server-ecc384-cert.pem
cert_auth = m:/certs/server

[private.encrypted.authenticated]
key_pass = libbitcoin
key_path = m:/certs/server/server-ecc384-key-enc.pem
cert_path = m:/certs/server/server-ecc384-cert.pem
cert_auth = m:/certs/server

# client
# =============================================================================
    
# place in cert_auth configured directory.
ca-ecc384-cert.pem
ca-ecc384-key.pem

# Use the pfx encoding for Windows certificate store import (e.g. browser auth).
client-ecc384.pfx

# --insecure disables SSL/TLS certificate verification.
# This allows bypass of cert validation again host name for testing.
curl <URL>
    --verbose
    --insecure
    --cert   client-ecc384-cert.pem
    --key    client-ecc384-key.pem
    --cacert ca-ecc384-cert.pem

# client example
# =============================================================================

m:/certs/server> curl --verbose --insecure --cert client-ecc384-cert.pem --key client-ecc384-key.pem --cacert ca-ecc384-cert.pem https://localhost:443/v1/top?format=json
Note: Using embedded CA bundle, for proxies (225076 bytes)
* Host localhost:443 was resolved.
* IPv6: ::1
* IPv4: 127.0.0.1
*   Trying [::1]:443...
* ALPN: curl offers h2,http/1.1
* TLSv1.3 (OUT), TLS handshake, Client hello (1):
* SSL Trust: peer verification disabled
* TLSv1.3 (IN), TLS handshake, Server hello (2):
* TLSv1.3 (IN), TLS handshake, Unknown (8):
* TLSv1.3 (IN), TLS handshake, Request CERT (13):
* TLSv1.3 (IN), TLS handshake, Certificate (11):
* TLSv1.3 (IN), TLS handshake, CERT verify (15):
* TLSv1.3 (IN), TLS handshake, Finished (20):
* TLSv1.3 (OUT), TLS handshake, Certificate (11):
* TLSv1.3 (OUT), TLS handshake, CERT verify (15):
* TLSv1.3 (OUT), TLS handshake, Finished (20):
* SSL connection using TLSv1.3 / TLS_AES_256_GCM_SHA384 / [blank] / UNDEF
* ALPN: server did not agree on a protocol. Uses default.
* Server certificate:
*   subject: C=US; ST=Washington; L=Seattle; O=Elliptic; OU=ECC384Srv; CN=www.wolfssl.com; emailAddress=info@wolfssl.com
*   start date: Nov 13 20:41:06 2025 GMT
*   expire date: Nov  6 20:41:06 2055 GMT
*   issuer: C=US; ST=Washington; L=Seattle; O=wolfSSL; OU=Development; CN=www.wolfssl.com; emailAddress=info@wolfssl.com
*   Certificate level 0: Public key type ? (384/192 Bits/secBits), signed using ecdsa-with-SHA384
*  SSL certificate verification failed, continuing anyway!
* Established connection to localhost (::1 port 443) from ::1 port 57364
* using HTTP/1.x
> GET /v1/top?format=json HTTP/1.1
> Host: localhost
> User-Agent: curl/8.18.0
> Accept: */*
>
* Request completely sent off
< HTTP/1.1 200 OK
< Date: Sun, 25 Jan 2026 07:12:21 GMT
< Server: libbitcoin/4
< Keep-Alive: timeout=599
< Content-Type: application/json
< Transfer-Encoding: chunked
<
900000