#include <stdio.h>

#include "../include/ks_common.h"
#include "../include/ks_args.h"
#include "../include/ks_addr.h"
#include "../include/ks_transport.h"
#include "../include/ks_chat.h"

static void print_banner(void)
{
    printf("=====================================\n");
    printf(" %s\n", APP_NAME);
    printf(" Version : %s\n", VERSION);
    printf(" Author  : %s\n", AUTHOR);
    printf(" Build   : %s %s\n", BUILD_DATE, BUILD_TIME);
    printf("=====================================\n\n");
}

int main(int argc, char *argv[])
{
    ks_config_t cfg;
    ks_endpoint_t ep;
    char str[96];
    int rc;

    print_banner();

    rc = ks_args_parse(argc, argv, &cfg);
    if (rc != KS_OK)
        return (rc == 1) ? 0 : 1;

    const ks_transport_ops_t *ops = ks_get_transport(cfg.proto);
    if (ops == NULL)
    {
        fprintf(stderr, "Protocol not supported yet\n");
        return 1;
    }

    if (cfg.mode == KS_MODE_CLIENT)
    {
        if (ops->open_client(&ep, &cfg) != KS_OK)
            return 1;

        printf("Connected to %s, Input '%s' to close.\n",
               ks_addr_to_str(&ep.peer, str, sizeof(str)), KS_QUIT_CMD);

        rc = ks_chat_run(ops, &ep);
        ops->close(&ep);
        return (rc == KS_OK) ? 0 : 1;
    }

    /* server：一個 client 結束後繼續等下一個，Ctrl-C 離開 */
    if (ops->open_server(&ep, &cfg) != KS_OK)
        return 1;

    for (;;)
    {
        if (ops->wait_peer(&ep) != KS_OK)
            break;
        ks_chat_run(ops, &ep);
    }

    ops->close(&ep);
    return 1;
}