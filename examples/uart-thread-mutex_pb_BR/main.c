/*
 * Copyright (C) 2015 Freie Universität Berlin
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

/**
 * @ingroup     examples
 * @{
 *
 * @file
 * @brief       Example application for demonstrating the RIOT network stack
 *
 * @author      Hauke Petersen <hauke.petersen@fu-berlin.de>
 *
 * @}
 */

#include <stdio.h>

#include "shell.h"
#include "msg.h"
#include "mutex.h"
#include "thread.h"
#include "xtimer.h"

#define MAIN_QUEUE_SIZE (8)
static msg_t _main_msg_queue[MAIN_QUEUE_SIZE];

static char _stack[THREAD_STACKSIZE_MAIN];
// static mutex_t _mutex = MUTEX_INIT;

void print_str_simple(char *aStr)
{
    // mutex_lock(&_mutex);
    puts(aStr);
    // mutex_unlock(&_mutex);
}

static void *_second_thread(void *arg)
{
    (void)arg;
    int count = 0;
    while (count < 3)
    {
        print_str_simple("THREAD 2-|+2-|+2-|+2-|+2-|+2-|+2-|+2-|+2-|+2-|+2-|+2-|+2-|+2-|+2-|+2-|+2-|+2-|+2-|+2-|+2-|+2-|+2-|+2-|+2-|+2-|+2-|+2-|+2-|+2-|+");
        count++;
    }

    return NULL;
}

int main(void)
{
    /* we need a message queue for the thread running the shell in order to
     * receive potentially fast incoming networking packets */
    msg_init_queue(_main_msg_queue, MAIN_QUEUE_SIZE);
    puts("RIOT border router example application");

    thread_create(_stack,
                  sizeof(_stack),
                  THREAD_PRIORITY_MAIN + 1,
                  THREAD_CREATE_WOUT_YIELD | THREAD_CREATE_STACKTEST,
                  _second_thread,
                  NULL,
                  "second_thread");

    xtimer_usleep(11800);
    print_str_simple("THREAD 1_!*11_!*11_!*11_!*11_!*11_!*11_!*11_!*11_!*11_!*11_!*11_!*11_!*11_!*11_!*11_!*11_!*11_!*11_!*11_!*11_!*11_!*11_!*11_!*1");

    /* should be never reached */
    return 0;
}
