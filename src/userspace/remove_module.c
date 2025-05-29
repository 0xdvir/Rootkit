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

    // Check if the persistent
    if (access(SERVICE_FILE, F_OK) == 0)
    {
        // Stop and disable the systemd service
        ret = system("systemctl stop " SERVICE_NAME);
        // if (ret == -1 || WEXITSTATUS(ret) != 0) {
        //     perror("Failed to stop service");
        //     return ret;
        // }
        ret = system("systemctl disable " SERVICE_NAME);
        // if (ret == -1 || WEXITSTATUS(ret) != 0) {
        //     perror("Failed to disable service");
        //     return ret;
        // }
        // Remove the service file
        unlink(SERVICE_FILE);
        // Optional: reload systemd daemon
        ret = system("systemctl daemon-reload");
        // if (ret == -1 || WEXITSTATUS(ret) != 0) {
        //     perror("Failed to reload systemd daemon");
        //     return ret;
        // }
    }

    unlink(REC_FILE);
    unlink(REV_SHELL_FILE);

    // Remove the kernel module
    unlink(module_path);

    // Unload the module
    // Comment out if moudule is hidden (won't work on a hidden module)
    ret = system("rmmod " MODULE_NAME);
    // if (ret != SUCCESS) {
    //     perror("Failed to remove kernel module");
    //     return ret;
    // }

    return SUCCESS;
}
