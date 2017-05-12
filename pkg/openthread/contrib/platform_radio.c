/*
 * Copyright (C) 2016 José Ignacio Alamos
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

/**
 * @{
 *
 * @file
 * @author  José Ignacio Alamos <jialamos@uc.cl>
 */

#include <stdio.h>
#include <assert.h>
#include <openthread/platform/radio.h>
#include "ot.h"

#define ENABLE_DEBUG (1)
#include "debug.h"

#include "errno.h"
#include "net/ethernet/hdr.h"
#include "net/ethertype.h"
#include "byteorder.h"
#include "net/netdev/ieee802154.h"
#include "net/ieee802154.h"
#include <string.h>

static bool sDisabled;

static RadioPacket sTransmitFrame;
static RadioPacket sReceiveFrame;
static int8_t Rssi;

static netdev_t *_dev;

/* set 15.4 channel */
int _set_channel(uint16_t channel)
{
    return _dev->driver->set(_dev, NETOPT_CHANNEL, &channel, sizeof(uint16_t));
}

/*get transmission power from driver */
int16_t _get_power(void)
{
    int16_t power;

    _dev->driver->get(_dev, NETOPT_TX_POWER, &power, sizeof(int16_t));
    return power;
}

/* set transmission power */
int _set_power(int16_t power)
{
    return _dev->driver->set(_dev, NETOPT_TX_POWER, &power, sizeof(int16_t));
}

/* set IEEE802.15.4 PAN ID */
int _set_panid(uint16_t panid)
{
    return _dev->driver->set(_dev, NETOPT_NID, &panid, sizeof(uint16_t));
}

/* set extended HW address */
int _set_long_addr(uint8_t *ext_addr)
{
    return _dev->driver->set(_dev, NETOPT_ADDRESS_LONG, ext_addr, IEEE802154_LONG_ADDRESS_LEN);
}

/* set short address */
int _set_addr(uint16_t addr)
{
    return _dev->driver->set(_dev, NETOPT_ADDRESS, &addr, sizeof(uint16_t));
}

/* check the state of promiscuous mode */
netopt_enable_t _is_promiscuous(void)
{
    netopt_enable_t en;

    _dev->driver->get(_dev, NETOPT_PROMISCUOUSMODE, &en, sizeof(en));
    return en == NETOPT_ENABLE ? true : false;;
}

/* set the state of promiscuous mode */
int _set_promiscuous(netopt_enable_t enable)
{
    return _dev->driver->set(_dev, NETOPT_PROMISCUOUSMODE, &enable, sizeof(enable));
}

/* wrapper for setting device state */
void _set_state(netopt_state_t state)
{
    _dev->driver->set(_dev, NETOPT_STATE, &state, sizeof(netopt_state_t));
}

/* sets device state to SLEEP */
void _set_sleep(void)
{
    _set_state(NETOPT_STATE_SLEEP);
}

/* set device state to IDLE */
void _set_idle(void)
{
    _set_state(NETOPT_STATE_IDLE);
}

/* init framebuffers and initial state */
void openthread_radio_init(netdev_t *dev, uint8_t *tb, uint8_t *rb)
{
    sTransmitFrame.mPsdu = tb;
    sTransmitFrame.mLength = 0;
    sReceiveFrame.mPsdu = rb;
    sReceiveFrame.mLength = 0;
    _dev = dev;
}

/* Called upon NETDEV_EVENT_RX_COMPLETE event */
void recv_pkt(otInstance *aInstance, netdev_t *dev)
{
    DEBUG("Openthread: Received pkt");
    netdev_ieee802154_rx_info_t rx_info;
    /* Read frame length from driver */
    int len = dev->driver->recv(dev, NULL, 0, &rx_info);
    Rssi = rx_info.rssi;

    /* very unlikely */
    if ((len > (unsigned) UINT16_MAX)) {
        otPlatRadioReceiveDone(aInstance, NULL, kThreadError_Abort);
        return;
    }

    /* Fill OpenThread receive frame */
    sReceiveFrame.mLength = len;
    sReceiveFrame.mPower = _get_power();


    /* Read received frame */
    int res = dev->driver->recv(dev, (char *) sReceiveFrame.mPsdu, len, NULL);

    /* Tell OpenThread that receive has finished */
    otPlatRadioReceiveDone(aInstance, res > 0 ? &sReceiveFrame : NULL, res > 0 ? kThreadError_None : kThreadError_Abort);
}

/* Called upon TX event */
void send_pkt(otInstance *aInstance, netdev_t *dev, netdev_event_t event)
{
    /* Tell OpenThread transmission is done depending on the NETDEV event */
    switch (event) {
        case NETDEV_EVENT_TX_COMPLETE:
            DEBUG("openthread: NETDEV_EVENT_TX_COMPLETE\n");
            otPlatRadioTransmitDone(aInstance, &sTransmitFrame, false, kThreadError_None);
            break;
        case NETDEV_EVENT_TX_COMPLETE_DATA_PENDING:
            DEBUG("openthread: NETDEV_EVENT_TX_COMPLETE_DATA_PENDING\n");
            otPlatRadioTransmitDone(aInstance, &sTransmitFrame, true, kThreadError_None);
            break;
        case NETDEV_EVENT_TX_NOACK:
            DEBUG("openthread: NETDEV_EVENT_TX_NOACK\n");
            otPlatRadioTransmitDone(aInstance, &sTransmitFrame, false, kThreadError_NoAck);
            break;
        case NETDEV_EVENT_TX_MEDIUM_BUSY:
            DEBUG("openthread: NETDEV_EVENT_TX_MEDIUM_BUSY\n");
            otPlatRadioTransmitDone(aInstance, &sTransmitFrame, false, kThreadError_ChannelAccessFailure);
            break;
        default:
            break;
    }
}

/* OpenThread will call this for setting PAN ID */
void otPlatRadioSetPanId(otInstance *aInstance, uint16_t panid)
{
    DEBUG("openthread: otPlatRadioSetPanId: setting PAN ID to %04x\n", panid);
    _set_panid(panid);
}

/* OpenThread will call this for setting extended address */
void otPlatRadioSetExtendedAddress(otInstance *aInstance, uint8_t *aExtendedAddress)
{
    DEBUG("openthread: otPlatRadioSetExtendedAddress\n");
    uint8_t reversed_addr[IEEE802154_LONG_ADDRESS_LEN];
    for (int i = 0; i < IEEE802154_LONG_ADDRESS_LEN; i++) {
        reversed_addr[i] = aExtendedAddress[IEEE802154_LONG_ADDRESS_LEN - 1 - i];
    }
    _set_long_addr(reversed_addr);
}

/* OpenThread will call this for setting short address */
void otPlatRadioSetShortAddress(otInstance *aInstance, uint16_t aShortAddress)
{
    DEBUG("openthread: otPlatRadioSetShortAddress: setting address to %04x\n", aShortAddress);
    _set_addr(((aShortAddress & 0xff) << 8) | ((aShortAddress >> 8) & 0xff));
}

/* OpenThread will call this for enabling the radio */
ThreadError otPlatRadioEnable(otInstance *aInstance)
{
    DEBUG("openthread: otPlatRadioEnable");
    (void) aInstance;

    ThreadError error;

    if (sDisabled)
    {
        sDisabled = false;
        error = kThreadError_None;
    }
    else
    {
        error = kThreadError_InvalidState;
    }

return error;
}

/* OpenThread will call this for disabling the radio */
ThreadError otPlatRadioDisable(otInstance *aInstance)
{
    DEBUG("openthread: otPlatRadioDisable");
    (void) aInstance;

    ThreadError error;

    if (!sDisabled)
    {
        sDisabled = true;
        error = kThreadError_None;
    }
    else
    {
        error = kThreadError_InvalidState;
    }

    return error;
}

bool otPlatRadioIsEnabled(otInstance *aInstance)
{
    DEBUG("otPlatRadioIsEnabled\n");
    (void) aInstance;

    return !sDisabled;
}

/* OpenThread will call this for setting device state to SLEEP */
ThreadError otPlatRadioSleep(otInstance *aInstance)
{
    DEBUG("otPlatRadioSleep\n");
   (void) aInstance;

    _set_sleep();
    return kThreadError_None;
}

/*OpenThread will call this for waiting the reception of a packet */

ThreadError otPlatRadioReceive(otInstance *aInstance, uint8_t aChannel)
{
    DEBUG("openthread: otPlatRadioReceive. Channel: %i\n", aChannel);
    (void) aInstance;

    _set_idle();
    _set_channel(aChannel);
    return kThreadError_None;
}


/* OpenThread will call this function to get the transmit buffer */
RadioPacket *otPlatRadioGetTransmitBuffer(otInstance *aInstance)
{
    DEBUG("openthread: otPlatRadioGetTransmitBuffer\n");
    return &sTransmitFrame;
}

/* OpenThread will call this for transmitting a packet*/
ThreadError otPlatRadioTransmit(otInstance *aInstance, RadioPacket *aPacket)
{
    (void) aInstance;
    struct iovec pkt;

    /* Populate iovec with transmit data */
    pkt.iov_base = aPacket->mPsdu;
    pkt.iov_len = aPacket->mLength;

    /*Set channel and power based on transmit frame */
    DEBUG("otPlatRadioTransmit->channel: %i\n", (int) aPacket->mChannel);
    _set_channel(aPacket->mChannel);
    _set_power(aPacket->mPower);

    /* send packet though netdev */
    _dev->driver->send(_dev, &pkt, 1);

    return kThreadError_None;
}

/* OpenThread will call this for getting the radio caps */
otRadioCaps otPlatRadioGetCaps(otInstance *aInstance)
{
    DEBUG("openthread: otPlatRadioGetCaps\n");
    /* all drivers should handle ACK, including call of NETDEV_EVENT_TX_NOACK */
    return kRadioCapsNone;
}

/* OpenThread will call this for getting the state of promiscuous mode */
bool otPlatRadioGetPromiscuous(otInstance *aInstance)
{
    DEBUG("openthread: otPlatRadioGetPromiscuous\n");
    return _is_promiscuous();
}

/* OpenThread will call this for setting the state of promiscuous mode */
void otPlatRadioSetPromiscuous(otInstance *aInstance, bool aEnable)
{
    DEBUG("openthread: otPlatRadioSetPromiscuous\n");
    _set_promiscuous((aEnable) ? NETOPT_ENABLE : NETOPT_DISABLE);
}

int8_t otPlatRadioGetRssi(otInstance *aInstance)
{
    DEBUG("otPlatRadioGetRssi\n");
    (void) aInstance;
    return Rssi;
}

void otPlatRadioEnableSrcMatch(otInstance *aInstance, bool aEnable)
{
    DEBUG("otPlatRadioEnableSrcMatch\n");
    (void)aInstance;
    (void)aEnable;
}

ThreadError otPlatRadioAddSrcMatchShortEntry(otInstance *aInstance, const uint16_t aShortAddress)
{
    DEBUG("otPlatRadioAddSrcMatchShortEntry\n");
    (void)aInstance;
    (void)aShortAddress;
    return kThreadError_None;
}

ThreadError otPlatRadioAddSrcMatchExtEntry(otInstance *aInstance, const uint8_t *aExtAddress)
{
    DEBUG("otPlatRadioAddSrcMatchExtEntry\n");
    (void)aInstance;
    (void)aExtAddress;
    return kThreadError_None;
}

ThreadError otPlatRadioClearSrcMatchShortEntry(otInstance *aInstance, const uint16_t aShortAddress)
{
    DEBUG("otPlatRadioClearSrcMatchShortEntry\n");
    (void)aInstance;
    (void)aShortAddress;
    return kThreadError_None;
}

ThreadError otPlatRadioClearSrcMatchExtEntry(otInstance *aInstance, const uint8_t *aExtAddress)
{
    DEBUG("otPlatRadioClearSrcMatchExtEntry\n");
    (void)aInstance;
    (void)aExtAddress;
    return kThreadError_None;
}

void otPlatRadioClearSrcMatchShortEntries(otInstance *aInstance)
{
    DEBUG("otPlatRadioClearSrcMatchShortEntries\n");
    (void)aInstance;
}

void otPlatRadioClearSrcMatchExtEntries(otInstance *aInstance)
{
    DEBUG("otPlatRadioClearSrcMatchExtEntries\n");
    (void)aInstance;
}

ThreadError otPlatRadioEnergyScan(otInstance *aInstance, uint8_t aScanChannel, uint16_t aScanDuration)
{
    DEBUG("otPlatRadioEnergyScan\n");
    (void)aInstance;
    (void)aScanChannel;
    (void)aScanDuration;
    return kThreadError_NotImplemented;
}

void otPlatRadioGetIeeeEui64(otInstance *aInstance, uint8_t *aIeee64Eui64)
{
    _dev->driver->get(_dev, NETOPT_IPV6_IID, aIeee64Eui64, sizeof(eui64_t));
}

void otPlatRadioSetDefaultTxPower(otInstance *aInstance, int8_t aPower)
{
    // TODO: Create a proper implementation for this driver.
    (void)aInstance;
    (void)aPower;
}

/** @} */
