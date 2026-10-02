#include <errno.h>
#include <poll.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "../include/ks_chat.h"

static void prompt(void)
{
    printf("> ");
    fflush(stdout);
}

int ks_chat_run(const ks_transport_ops_t *ops, ks_endpoint_t *ep)
{
    char buf[KS_BUF_SIZE];
    struct pollfd fds[2];

    printf("Type '%s' to close.\n", KS_QUIT_CMD);
    prompt();

    for (;;)
    {
        fds[0].fd      = STDIN_FILENO;
        fds[0].events  = POLLIN;
        fds[0].revents = 0;
        fds[1].fd      = ep->fd;
        fds[1].events  = POLLIN;
        fds[1].revents = 0;

        if (poll(fds, 2, -1) < 0)
        {
            if (errno == EINTR)
                continue;
            perror("poll");
            return KS_ERR;
        }

        /* ---- 收到對方資料 ---- */
        if (fds[1].revents & (POLLIN | POLLHUP | POLLERR))
        {
            ssize_t n = ops->recv(ep, buf, sizeof(buf) - 1);

            if (n < 0 && errno == EAGAIN)
                continue;               /* UDP 空封包：沒資料，回去 poll */

            if (n <= 0)
            {
                if (n < 0)
                    fprintf(stderr, "\nrecv: %s", strerror(errno));
                printf("\n%s disconnected.\n", ep->name);
                return KS_OK;
            }
            buf[n] = '\0';

            /* 應用層結束指令：UDP 沒有斷線通知，靠它收尾 */
            if (strcmp(buf, KS_QUIT_CMD) == 0)
            {
                printf("\n%s closed the session.\n", ep->name);
                return KS_OK;
            }

            printf("\n%s: %s\n", ep->name, buf);
            prompt();
        }

        /* ---- 使用者鍵盤輸入 ---- */
        if (fds[0].revents & (POLLIN | POLLHUP))
        {
            if (fgets(buf, sizeof(buf), stdin) == NULL)
            {
                /* EOF (Ctrl-D)：當作 quit */
                ops->send(ep, KS_QUIT_CMD, strlen(KS_QUIT_CMD));
                return KS_OK;
            }

            buf[strcspn(buf, "\n")] = '\0';

            if (buf[0] == '\0')     /* 空行不送（UDP 的空 datagram 也沒意義） */
            {
                prompt();
                continue;
            }

            if (ops->send(ep, buf, strlen(buf)) < 0)
            {
                perror("send");
                return KS_ERR;
            }

            if (strcmp(buf, KS_QUIT_CMD) == 0)
                return KS_OK;

            prompt();
        }
    }
}