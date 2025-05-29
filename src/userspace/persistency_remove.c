#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/utsname.h>
#include "config.h"

#define SUCCESS 0

int main(int argc, char **argv)
{
    int ret = SUCCESS;
    struct utsname buffer;
    if (uname(&buffer) != 0)
        return ret;
    char module_path[512];
    snprintf(module_path, sizeof(module_path), MODULE_PATH, buffer.release);

    // Stop and disable the systemd service
    ret = system("systemctl stop " SERVICE_NAME);
    // if (ret == -1 || WEXITSTATUS(ret) != 0) {
    //     PERROR("Failed to stop service");
    //     return ret;
    // }
    ret = system("systemctl disable " SERVICE_NAME);
    // if (ret == -1 || WEXITSTATUS(ret) != 0) {
    //     PERROR("Failed to disable service");
    //     return ret;
    // }
    // Remove the service file
    unlink(SERVICE_FILE);
    // Optional: reload systemd daemon
    ret = system("systemctl daemon-reload");
    // if (ret == -1 || WEXITSTATUS(ret) != 0) {
    //     PERROR("Failed to reload systemd daemon");
    //     return ret;
    // }

    return SUCCESS;
}
