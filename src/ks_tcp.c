#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include "../include/ks_transport.h"
#include "../include/ks_addr.h"

static void ep_init(ks_endpoint_t *ep)
{
    memset(ep, 0, sizeof(*ep));
    ep->fd        = -1;
    ep->listen_fd = -1;
    ep->proto     = KS_PROTO_TCP;
}

static int tcp_open_client(ks_endpoint_t *ep, const ks_config_t *cfg)
{
    ks_addr_t addr;
    char str[96];

    ep_init(ep);

    if (ks_addr_resolve(cfg->ip, cfg->port, KS_PROTO_TCP, 0, &addr) != KS_OK)
        return KS_ERR;

    ep->fd = socket(addr.family, SOCK_STREAM, 0);
    if (ep->fd < 0) {
        perror("socket");
        return KS_ERR;
    }

    if (connect(ep->fd, (struct sockaddr *)&addr.ss, addr.len) < 0) {
        printf("Failed to connect to %s\n",
               ks_addr_to_str(&addr, str, sizeof(str)));
        perror("connect");
        close(ep->fd);
        ep->fd = -1;
        return KS_ERR;
    }

    ep->peer = addr;
    snprintf(ep->name, sizeof(ep->name), "From Server");
    return KS_OK;
}

static int tcp_open_server(ks_endpoint_t *ep, const ks_config_t *cfg)
{
    ks_addr_t addr;
    char str[96];
    int fd, opt = 1;

    ep_init(ep);

    if (ks_addr_resolve(cfg->ip, cfg->port, KS_PROTO_TCP, 1, &addr) != KS_OK)
        return KS_ERR;

    fd = socket(addr.family, SOCK_STREAM, 0);
    if (fd < 0) {
        perror("socket");
        return KS_ERR;
    }

    /* 避免重啟 server 時出現 "Address already in use" */
    if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        perror("setsockopt(SO_REUSEADDR)");
        goto fail;
    }

    /* 沒指定 bind IP 時綁 "::"，關閉 V6ONLY 讓 IPv4 client 也能連入（dual-stack） */
    if (addr.family == AF_INET6 && cfg->ip[0] == '\0') {
        int off = 0;
        if (setsockopt(fd, IPPROTO_IPV6, IPV6_V6ONLY, &off, sizeof(off)) < 0) {
            perror("setsockopt(IPV6_V6ONLY)");
            goto fail;
        }
    }

    if (bind(fd, (struct sockaddr *)&addr.ss, addr.len) < 0) {
        perror("bind");
        goto fail;
    }

    if (listen(fd, 5) < 0) {
        perror("listen");
        goto fail;
    }

    ep->listen_fd = fd;
    printf("Listening on %s (TCP)...\n",
           ks_addr_to_str(&addr, str, sizeof(str)));
    return KS_OK;

fail:
    close(fd);
    return KS_ERR;
}

/* 可重複呼叫：若上一個 client 還在，先關掉再等下一個 */
static int tcp_wait_peer(ks_endpoint_t *ep)
{
    ks_addr_t peer;
    char str[96];
    int fd;

    if (ep->fd >= 0) {
        close(ep->fd);
        ep->fd = -1;
    }

    memset(&peer, 0, sizeof(peer));
    do {
        peer.len = sizeof(peer.ss);
        fd = accept(ep->listen_fd, (struct sockaddr *)&peer.ss, &peer.len);
    } while (fd < 0 && errno == EINTR);

    if (fd < 0) {
        perror("accept");
        return KS_ERR;
    }

    peer.family = peer.ss.ss_family;
    ep->fd   = fd;
    ep->peer = peer;
    snprintf(ep->name, sizeof(ep->name), "From Client");

    printf("Client connected: %s\n", ks_addr_to_str(&peer, str, sizeof(str)));
    return KS_OK;
}

/* TCP 可能只送出一部分，所以要送到全部送完；MSG_NOSIGNAL 避免對方斷線時被 SIGPIPE 殺掉 */
static ssize_t tcp_send(ks_endpoint_t *ep, const void *buf, size_t len)
{
    const char *p = buf;
    size_t left = len;

    while (left > 0) {
        ssize_t n = send(ep->fd, p, left, MSG_NOSIGNAL);
        if (n < 0) {
            if (errno == EINTR)
                continue;
            return -1;
        }
        p    += n;
        left -= (size_t)n;
    }
    return (ssize_t)len;
}

static ssize_t tcp_recv(ks_endpoint_t *ep, void *buf, size_t len)
{
    ssize_t n;

    do {
        n = recv(ep->fd, buf, len, 0);
    } while (n < 0 && errno == EINTR);

    return n;   /* >0 資料；0 對方關閉；<0 錯誤 */
}

static void tcp_shutdown(ks_endpoint_t *ep)
{
    if (ep->fd >= 0)
        shutdown(ep->fd, SHUT_RDWR);
}

static void tcp_close(ks_endpoint_t *ep)
{
    if (ep->fd >= 0) {
        close(ep->fd);
        ep->fd = -1;
    }
    if (ep->listen_fd >= 0) {
        close(ep->listen_fd);
        ep->listen_fd = -1;
    }
}

const ks_transport_ops_t ks_tcp_ops = {
    .open_client = tcp_open_client,
    .open_server = tcp_open_server,
    .wait_peer   = tcp_wait_peer,
    .send        = tcp_send,
    .recv        = tcp_recv,
    .shutdown    = tcp_shutdown,
    .close       = tcp_close,
};