#include <stdio.h>
#include <string.h>
#include "../include/ks_args.h"
#include "../include/ks_transport.h"

int main(int argc, char *argv[])
{
    ks_config_t cfg;
    ks_endpoint_t ep;
    char buf[KS_BUF_SIZE];
    ssize_t n;

    int rc = ks_args_parse(argc, argv, &cfg);
    if (rc != KS_OK)
        return (rc == 1) ? 0 : 1;

    const ks_transport_ops_t *ops = ks_get_transport(cfg.proto);
    if (ops == NULL) {
        fprintf(stderr, "Protocol not supported yet\n");
        return 1;
    }

    if (cfg.mode == KS_MODE_SERVER) {
        if (ops->open_server(&ep, &cfg) != KS_OK || ops->wait_peer(&ep) != KS_OK)
            return 1;

        n = ops->recv(&ep, buf, sizeof(buf) - 1);
        if (n > 0) {
            buf[n] = '\0';
            printf("%s: %s\n", ep.name, buf);
            const char *reply = "echo ok";
            ops->send(&ep, reply, strlen(reply));
        }
    } else {
        if (ops->open_client(&ep, &cfg) != KS_OK)
            return 1;

        const char *msg = "hello from client";
        ops->send(&ep, msg, strlen(msg));

        n = ops->recv(&ep, buf, sizeof(buf) - 1);
        if (n > 0) {
            buf[n] = '\0';
            printf("%s: %s\n", ep.name, buf);
        }
    }

    ops->close(&ep);
    return 0;
}