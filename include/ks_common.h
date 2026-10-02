#ifndef KS_COMMON_H
#define KS_COMMON_H

#include <stddef.h>
#include <sys/types.h>   /* ssize_t */
#include <sys/socket.h>  /* sockaddr_storage, socklen_t */

/* ---------- 程式資訊 ---------- */
#define APP_NAME    "Socket Client/Server Demo"
#define VERSION     "26.10.02"
#define AUTHOR      "Ken Lin"
#define BUILD_DATE  __DATE__
#define BUILD_TIME  __TIME__

/* ---------- 預設值 / 長度 ---------- */
#define KS_DEFAULT_PORT  "8787"   /* 字串型態，getaddrinfo 直接可用 */
#define KS_NAME_LEN      32
#define KS_IP_LEN        64       /* 足夠放 IPv6 + %scope_id */
#define KS_PORT_LEN      8
#define KS_BUF_SIZE      1024

/* ---------- 回傳碼 ---------- */
#define KS_OK     0
#define KS_ERR   (-1)

/* ---------- 列舉 ---------- */
typedef enum {
    KS_MODE_SERVER = 0,
    KS_MODE_CLIENT
} ks_mode_t;

typedef enum {
    KS_PROTO_TCP = 0,
    KS_PROTO_UDP
} ks_proto_t;

/* ---------- 位址：不管 IPv4/IPv6 都用這個裝 ---------- */
typedef struct {
    struct sockaddr_storage ss;   /* 足以容納 sockaddr_in / sockaddr_in6 */
    socklen_t               len;
    int                     family; /* AF_INET / AF_INET6 */
} ks_addr_t;

/* ---------- 使用者設定（由 ks_args 填入） ---------- */
typedef struct {
    ks_mode_t  mode;
    ks_proto_t proto;
    char       ip[KS_IP_LEN];     /* client: 對方 IP；server: 可選的綁定 IP，空字串=不指定 */
    char       port[KS_PORT_LEN];
} ks_config_t;

/* ---------- 通訊端點：TCP/UDP 共用 ---------- */
typedef struct {
    int        fd;
    int        listen_fd;            /* TCP server 的監聽 socket，其餘情況為 -1 */
    ks_proto_t proto;
    ks_addr_t  peer;                 /* 對方位址（UDP 一定要記） */
    char       name[KS_NAME_LEN];    /* 顯示用，例如 "From Server" */
} ks_endpoint_t;

#endif /* KS_COMMON_H */