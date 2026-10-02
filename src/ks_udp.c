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
    ep->listen_fd = -1;     /* UDP 不使用 */
    ep->proto     = KS_PROTO_UDP;
}

static int udp_open_client(ks_endpoint_t *ep, const ks_config_t *cfg)
{
    ks_addr_t addr;

    ep_init(ep);

    if (ks_addr_resolve(cfg->ip, cfg->port, KS_PROTO_UDP, 0, &addr) != KS_OK)
        return KS_ERR;

    ep->fd = socket(addr.family, SOCK_DGRAM, 0);
    if (ep->fd < 0) {
        perror("socket");
        return KS_ERR;
    }

    /* UDP connect：只是記住預設對象，並讓核心過濾其他來源的封包 */
    if (connect(ep->fd, (struct sockaddr *)&addr.ss, addr.len) < 0) {
        perror("connect");
        close(ep->fd);
        ep->fd = -1;
        return KS_ERR;
    }

    /* 先送空封包當 hello，讓 server 的 wait_peer 知道有 client 來了 */
    if (send(ep->fd, "", 0, 0) < 0) {
        perror("send(hello)");
        close(ep->fd);
        ep->fd = -1;
        return KS_ERR;
    }

    ep->peer = addr;
    snprintf(ep->name, sizeof(ep->name), "From Server");
    return KS_OK;
}

static int udp_open_server(ks_endpoint_t *ep, const ks_config_t *cfg)
{
    ks_addr_t addr;
    char str[96];
    int fd, opt = 1;

    ep_init(ep);

    if (ks_addr_resolve(cfg->ip, cfg->port, KS_PROTO_UDP, 1, &addr) != KS_OK)
        return KS_ERR;

    fd = socket(addr.family, SOCK_DGRAM, 0);
    if (fd < 0) {
        perror("socket");
        return KS_ERR;
    }

    if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        perror("setsockopt(SO_REUSEADDR)");
        goto fail;
    }

    /* 沒指定 bind IP：dual-stack，IPv4 / IPv6 client 都能進來 */
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

    ep->fd = fd;
    printf("Listening on %s (UDP)...\n",
           ks_addr_to_str(&addr, str, sizeof(str)));
    return KS_OK;

fail:
    close(fd);
    return KS_ERR;
}

/* 可重複呼叫：先解除上一個 peer 的綁定，再等下一個 client 的第一個封包 */
static int udp_wait_peer(ks_endpoint_t *ep)
{
    struct sockaddr unspec;
    ks_addr_t peer;
    char tmp[KS_BUF_SIZE];
    char str[96];
    ssize_t n;

    memset(&unspec, 0, sizeof(unspec));
    unspec.sa_family = AF_UNSPEC;
    connect(ep->fd, &unspec, sizeof(unspec));   /* 解除綁定；失敗可忽略 */

    memset(&peer, 0, sizeof(peer));
    do {
        peer.len = sizeof(peer.ss);
        n = recvfrom(ep->fd, tmp, sizeof(tmp), 0,
                     (struct sockaddr *)&peer.ss, &peer.len);
    } while (n < 0 && errno == EINTR);

    if (n < 0) {
        perror("recvfrom");
        return KS_ERR;
    }
    peer.family = peer.ss.ss_family;

    /* 之後只收這個 peer 的封包，send/recv 也不必再指定位址 */
    if (connect(ep->fd, (struct sockaddr *)&peer.ss, peer.len) < 0) {
        perror("connect(peer)");
        return KS_ERR;
    }

    ep->peer = peer;
    snprintf(ep->name, sizeof(ep->name), "From Client");
    printf("Client connected: %s\n", ks_addr_to_str(&peer, str, sizeof(str)));
    return KS_OK;
}

static ssize_t udp_send(ks_endpoint_t *ep, const void *buf, size_t len)
{
    ssize_t n;

    do {
        n = send(ep->fd, buf, len, MSG_NOSIGNAL);
    } while (n < 0 && errno == EINTR);

    return n;
}

static ssize_t udp_recv(ks_endpoint_t *ep, void *buf, size_t len)
{
    ssize_t n;

    do {
        n = recv(ep->fd, buf, len, 0);
    } while (n < 0 && errno == EINTR);

    /* UDP 的 0 代表「空封包」而不是斷線，轉成 EAGAIN 讓 chat 層略過 */
    if (n == 0) {
        errno = EAGAIN;
        return -1;
    }
    return n;
}

static void udp_close(ks_endpoint_t *ep)
{
    if (ep->fd >= 0) {
        close(ep->fd);
        ep->fd = -1;
    }
}

const ks_transport_ops_t ks_udp_ops = {
    .open_client = udp_open_client,
    .open_server = udp_open_server,
    .wait_peer   = udp_wait_peer,
    .send        = udp_send,
    .recv        = udp_recv,
    .close       = udp_close,
};