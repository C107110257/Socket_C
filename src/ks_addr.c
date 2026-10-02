#include <stdio.h>
#include <string.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#include "../include/ks_addr.h"

int ks_addr_detect_family(const char *ip)
{
    struct addrinfo hints, *res = NULL;
    int family;

    if (ip == NULL || ip[0] == '\0')
        return KS_ERR;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family   = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;   /* 固定一種，避免回傳重複結果 */
    hints.ai_flags    = AI_NUMERICHOST; /* 只接受數字 IP，不做 DNS 查詢 */

    if (getaddrinfo(ip, NULL, &hints, &res) != 0)
        return KS_ERR;

    family = res->ai_family;
    freeaddrinfo(res);
    return family;
}

int ks_addr_resolve(const char *ip, const char *port,
                    ks_proto_t proto, int passive, ks_addr_t *out)
{
    struct addrinfo hints, *res = NULL;
    const char *node = (ip != NULL && ip[0] != '\0') ? ip : NULL;
    int rc;

    if (port == NULL || out == NULL)
        return KS_ERR;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family   = AF_UNSPEC;
    hints.ai_socktype = (proto == KS_PROTO_TCP) ? SOCK_STREAM : SOCK_DGRAM;
    hints.ai_flags    = AI_NUMERICSERV;
    if (node != NULL)
        hints.ai_flags |= AI_NUMERICHOST;
    if (passive)
        hints.ai_flags |= AI_PASSIVE;

    /* server 沒指定 IP：優先用 IPv6 的 "::"，搭配 V6ONLY=0 就是 dual-stack */
    if (passive && node == NULL)
        hints.ai_family = AF_INET6;

    rc = getaddrinfo(node, port, &hints, &res);

    /* 系統不支援 IPv6 時退回 IPv4 的 0.0.0.0 */
    if (rc != 0 && passive && node == NULL) {
        hints.ai_family = AF_INET;
        rc = getaddrinfo(node, port, &hints, &res);
    }

    if (rc != 0) {
        fprintf(stderr, "getaddrinfo(%s, %s): %s\n",
                node ? node : "(any)", port, gai_strerror(rc));
        return KS_ERR;
    }

    memset(out, 0, sizeof(*out));
    memcpy(&out->ss, res->ai_addr, res->ai_addrlen);
    out->len    = res->ai_addrlen;
    out->family = res->ai_family;

    freeaddrinfo(res);
    return KS_OK;
}

const char *ks_addr_to_str(const ks_addr_t *addr, char *buf, size_t buflen)
{
    char host[64], serv[16];
    const struct sockaddr *sa;
    socklen_t salen;
    struct sockaddr_in v4;
    int is_v6;

    if (buf == NULL || buflen == 0)
        return "";
    if (addr == NULL) {
        snprintf(buf, buflen, "?");
        return buf;
    }

    sa    = (const struct sockaddr *)&addr->ss;
    salen = addr->len;
    is_v6 = (addr->family == AF_INET6);

    /* dual-stack server 收到的 IPv4 client 會是 ::ffff:a.b.c.d，轉回 IPv4 顯示 */
    if (is_v6) {
        const struct sockaddr_in6 *s6 = (const struct sockaddr_in6 *)&addr->ss;
        if (IN6_IS_ADDR_V4MAPPED(&s6->sin6_addr)) {
            memset(&v4, 0, sizeof(v4));
            v4.sin_family = AF_INET;
            v4.sin_port   = s6->sin6_port;
            memcpy(&v4.sin_addr, &s6->sin6_addr.s6_addr[12], 4);
            sa    = (const struct sockaddr *)&v4;
            salen = sizeof(v4);
            is_v6 = 0;
        }
    }

    if (getnameinfo(sa, salen, host, sizeof(host), serv, sizeof(serv),
                    NI_NUMERICHOST | NI_NUMERICSERV) != 0) {
        snprintf(buf, buflen, "?");
        return buf;
    }

    if (is_v6)
        snprintf(buf, buflen, "[%s]:%s", host, serv);
    else
        snprintf(buf, buflen, "%s:%s", host, serv);

    return buf;
}