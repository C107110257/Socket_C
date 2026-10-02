#include "../include/ks_transport.h"

const ks_transport_ops_t *ks_get_transport(ks_proto_t proto)
{
    switch (proto) {
    case KS_PROTO_TCP:
        return &ks_tcp_ops;
    case KS_PROTO_UDP:
        return &ks_udp_ops;   /* TODO: 寫完 ks_udp.c 後改成 &ks_udp_ops */
    default:
        return NULL;
    }
}