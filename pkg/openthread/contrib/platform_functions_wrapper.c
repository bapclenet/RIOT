/*
 * Copyright (C)
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

/**
 * @{
 * @ingroup     net
 * @file
 * @brief       Implementation of OpenThread functions wrapper. They are used to call OT functions from OT thread
 *
 * @author      Jose Ignacio Alamos <jialamos@uc.cl>
 * @author      Baptiste CLENET <bapclenet@gmail.com>
 * @}
 */

#include <stdint.h>
#include <stdio.h>
#include "thread.h"
#include "openthread/ip6.h"
#include "openthread/thread.h"
#include "openthread/udp.h"
#include "ot.h"

#define ENABLE_DEBUG (1)
#include "debug.h"

typedef uint8_t OT_JOB;

OT_JOB ot_channel(otInstance* ot_instance, void* arg, void* answer);
OT_JOB ot_eui64(otInstance* ot_instance, void* arg, void* answer);
OT_JOB ot_extaddr(otInstance* ot_instance, void* arg, void* answer);
OT_JOB ot_ipaddr(otInstance* ot_instance, void* arg, void* answer);
OT_JOB ot_masterkey(otInstance* ot_instance, void* arg, void* answer);
OT_JOB ot_networkname(otInstance* ot_instance, void* arg, void* answer);
OT_JOB ot_mode(otInstance* ot_instance, void* arg, void* answer);
OT_JOB ot_panid(otInstance* ot_instance, void* arg, void* answer);
OT_JOB ot_parent(otInstance* ot_instance, void* arg, void* answer);
OT_JOB ot_state(otInstance* ot_instance, void* arg, void* answer);
OT_JOB ot_thread(otInstance* ot_instance, void* arg, void* answer);

/**
 * @brief   Struct containing an OpenThread job command
 */
typedef struct {
    const char *mName;                              /**< A pointer to the job name string. */
    OT_JOB (*mFunction)(otInstance*, void*, void*);    /**< function to be called when executing job */
} ot_job_command_t;

const ot_job_command_t otJobs[] =
{
    { "channel", &ot_channel },         /* arg NULL: get channel in answer | arg not NULL: set channel */
    { "eui64", &ot_eui64 },             /* arg NULL: get eui64 in answer | arg not NULL: set eui64 */
    { "extaddr", &ot_extaddr },         /* arg NULL: get extaddr in answer | arg not NULL: set extaddr */
    { "ipaddr", &ot_ipaddr },           /* arg NULL: get number of ipaddr in answer | arg not NULL: get ipaddr[arg] */
    { "masterkey", &ot_masterkey },     /* arg NULL: get masterkey in answer | arg not NULL: set masterkey */
    { "mode", ot_mode },                /* arg NULL: get mode in answer | arg not NULL: set mode */
    { "networkname", &ot_networkname }, /* arg NULL: get networkname in answer | arg not NULL: set networkname */
    { "panid", &ot_panid },             /* arg NULL: get panid in answer | arg not NULL: set panid */
    { "parent", &ot_parent },           /* arg NULL: get parent in answer */
    { "state", &ot_state },             /* arg NULL: get state in answer */
    { "thread", &ot_thread },           /* arg "start"/"stop": start/stop thread operation */
};

uint8_t ot_exec_job(otInstance *ot_instance, const char* name, void *arg, void* answer) {
    uint8_t res = 0xFF;
    /* Check running thread */
    if (openthread_get_pid() == thread_getpid()) {
        for (uint8_t i = 0; i < sizeof(otJobs) / sizeof(otJobs[0]); i++) {
            if (strcmp(name, otJobs[i].mName) == 0) {
                res = (*otJobs[i].mFunction)(ot_instance, arg, answer);
                break;
            }
        }
        if (res == 0xFF) {
            DEBUG("Wrong ot_job name\n");
            res = 1;
        }
    } else {
        printf("ERROR: ot_exec_job needs to run in OpenThread thread\n");
    }
    return res;
}

void output_bytes(const char* name, const uint8_t *aBytes, uint8_t aLength)
{
    DEBUG("%s: ", name);
    for (int i = 0; i < aLength; i++)
    {
        DEBUG("%02x", aBytes[i]);
    }
    DEBUG("\n");
}

OT_JOB ot_channel(otInstance* ot_instance, void* arg, void* answer) {
    if (answer != NULL) {
        *((uint8_t *) answer) = otLinkGetChannel(ot_instance);
        DEBUG("Channel: %04x\n", *((uint8_t *) answer));
    } else if (arg != NULL) {
        uint8_t channel = *((uint8_t *) arg);
        otLinkSetChannel(ot_instance, channel);
    } else {
        printf("ERROR: wrong argument\n");
    }
    return 0;
}

OT_JOB ot_eui64(otInstance* ot_instance, void* arg, void* answer) {
    if (answer != NULL) {
        otExtAddress extAddress;
        otLinkGetFactoryAssignedIeeeEui64(ot_instance, &extAddress);
        output_bytes("eui64", extAddress.m8, OT_EXT_ADDRESS_SIZE);
        *((otExtAddress *) answer) = extAddress;
    } else {
        printf("ERROR: wrong argument\n");
    }
    return 0;
}


OT_JOB ot_extaddr(otInstance* ot_instance, void* arg, void* answer) {
    if (answer != NULL) {
        answer = (void*)otLinkGetExtendedAddress(ot_instance);
        output_bytes("extaddr", (const uint8_t *)answer, OT_EXT_ADDRESS_SIZE);
    } else {
        printf("ERROR: wrong argument\n");
    }
    return 0;
}

OT_JOB ot_ipaddr(otInstance* ot_instance, void* arg, void* answer) {
    uint8_t cnt = 0;
    for (const otNetifAddress *addr = otIp6GetUnicastAddresses(ot_instance); addr; addr = addr->mNext) {
        if (arg != NULL && answer != NULL && cnt == *((uint8_t *) arg)) {
            *((otNetifAddress *) answer) = *addr;
            return 0;
        }
        cnt++;
    }
    if (answer != NULL) {
        *((uint8_t *) answer) = cnt;
    } else {
        printf("ERROR: wrong argument\n");
    }
    return 0;
}

OT_JOB ot_masterkey(otInstance* ot_instance, void* arg, void* answer) {
    if (answer != NULL) {
        const otMasterKey* masterkey = otThreadGetMasterKey(ot_instance);
        *((otMasterKey *) answer) = *masterkey;
        output_bytes("masterkey", (const uint8_t *)answer, OT_MASTER_KEY_SIZE);
    } else if (arg != NULL) {
        otThreadSetMasterKey(ot_instance, (otMasterKey*)arg);
    } else {
        printf("ERROR: wrong argument\n");
    }
    return 0;
}

OT_JOB ot_mode(otInstance* ot_instance, void* arg, void* answer) {
    if (arg != NULL) {
        otLinkModeConfig linkMode;
        memset(&linkMode, 0, sizeof(otLinkModeConfig));
        char mode[6] = "";
        memcpy(mode, (char*)arg, 5);
        mode[5] = '\0';
        for (char *arg = &mode[0]; *arg != '\0'; arg++) {
            switch (*arg) {
                case 'r':
                    linkMode.mRxOnWhenIdle = 1;
                    break;
                case 's':
                    linkMode.mSecureDataRequests = 1;
                    break;
                case 'd':
                    linkMode.mDeviceType = 1;
                    break;
                case 'n':
                    linkMode.mNetworkData = 1;
                    break;
            }
        }
        otThreadSetLinkMode(ot_instance, linkMode);
        DEBUG("OT mode changed to %s\n", (char*)arg);
    } else {
        printf("ERROR: wrong argument\n");
    }
    return 0;
}

OT_JOB ot_networkname(otInstance* ot_instance, void* arg, void* answer) {
    if (answer != NULL) {
        const char* networkName = otThreadGetNetworkName(ot_instance);
        strcpy((char*) answer, networkName);
        DEBUG("networkname: %.*s\n", OT_NETWORK_NAME_MAX_SIZE, networkName);
    } else if (arg != NULL) {
        otThreadSetNetworkName(ot_instance, (char*) arg);
    } else {
        printf("ERROR: wrong argument\n");
    }
    return 0;
}


OT_JOB ot_panid(otInstance* ot_instance, void* arg, void* answer) {
    if (answer != NULL) {
        *((uint16_t *) answer) = otLinkGetPanId(ot_instance);
        DEBUG("PanID: %04x\n", *((uint16_t *) answer));
    } else if (arg != NULL) {
        /* Thread operation needs to be stopped before setting panid */
        otThreadSetEnabled(ot_instance, false);
        uint16_t panid = *((uint16_t *) arg);
        otLinkSetPanId(ot_instance, panid);
        otThreadSetEnabled(ot_instance, true);
    } else {
        printf("ERROR: wrong argument\n");
    }
    return 0;
}

OT_JOB ot_parent(otInstance* ot_instance, void* arg, void* answer) {
    if (answer != NULL) {
        otRouterInfo parentInfo;
        otThreadGetParentInfo(ot_instance, &parentInfo);
        output_bytes("parent", (const uint8_t *)parentInfo.mExtAddress.m8, sizeof(parentInfo.mExtAddress));
        DEBUG("Rloc: %x\n", parentInfo.mRloc16);
        *((otRouterInfo *) answer) = parentInfo;
    } else {
        printf("ERROR: wrong argument\n");
    }
    return 0;
}

OT_JOB ot_state(otInstance* ot_instance, void* arg, void* answer) {
    if (answer != NULL) {
        uint8_t state = otThreadGetDeviceRole(ot_instance);
        *((uint8_t *) answer) = state;
        DEBUG("state: ");
        switch (state) {
            case kDeviceRoleOffline:
                puts("offline");
                break;
            case kDeviceRoleDisabled:
                puts("disabled");
                break;
            case kDeviceRoleDetached:
                puts("detached");
                break;
            case kDeviceRoleChild:
                puts("child");
                break;
            case kDeviceRoleRouter:
                puts("router");
                break;
            case kDeviceRoleLeader:
                puts("leader");
                break;
            default:
                puts("invalid state");
                break;
        }
    } else {
        printf("ERROR: wrong argument\n");
    }
    return 0;
}

OT_JOB ot_thread(otInstance* ot_instance, void* arg, void* answer) {
    if (arg != NULL) {
        if (strcmp((char*)arg, "start") == 0) {
            otThreadSetEnabled(ot_instance, true);
            DEBUG("Thread start\n");
        } else if (strcmp((char*)arg, "stop") == 0) {
            otThreadSetEnabled(ot_instance, false);
            DEBUG("Thread stop\n");
        } else {
            printf("ERROR: thread available args: start/stop\n");
        }
    } else {
        printf("ERROR: wrong argument\n");
    }
    return 0;
}
