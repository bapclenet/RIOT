/*
 * Copyright (C) 2014 Freie Universität Berlin
 *
 * This file is subject to the terms and conditions of the GNU Lesser General
 * Public License v2.1. See the file LICENSE in the top level directory for more
 * details.
 */

/**
 * @ingroup     cpu_samd21
 * @{
 *
 * @file
 * @brief       Low-level random number generator driver implementation
 *
 * @author      Baptiste Clenet <baptiste.clenet@xsoen.com>
 *
 * @}
 */

#include "cpu.h"
#include "periph/random.h"
#include "periph_conf.h"
#include "at86rf231/at86rf231_settings.h"

/* ignore file in case no RNG device is defined */
#if RANDOM_NUMOF

void random_init(void)
{
	uint8_t tmp = 0;
#ifndef MODULE_AT86RF231
	DEBUG("MODULE_AT86RF231 not used");
#endif
#ifndef MODULE_TRANSCEIVER
	DEBUG("MODULE_TRANSCEIVER not used");
#endif
	tmp = at86rf231_reg_read(AT86RF231_REG__RX_SYN);
	if (tmp>>7 == 0){
		at86rf231_reg_write(AT86RF231_REG__RX_SYN,tmp|128);
	}
}

int random_read(char *buf, unsigned int num)
{
    uint32_t tmp = 0;
    uint32_t tmp_reg = 0;
    int count = 0;

    while (count < num) {
        for (int i = 0; i < 4; i++) {
        	/* wait for random data to be ready to read */

        	/* read next 2 bytes */
        	tmp_reg = at86rf231_reg_read(AT86RF231_REG__PHY_RSSI );
        	tmp |= ((tmp_reg >> 5) & 3) << i;
		}
        /* copy data into result vector */
        if(tmp != 0){
        	buf[count++] = (char)tmp;
            tmp = 0;
        }
    }
    return count;
}

void random_poweron(void)
{
	/*Nothing to do*/
}

void random_poweroff(void)
{
	/*Nothing to do*/
}

#endif /* RANDOM_NUMOF */
