/*
 * Copyright (c) 2018, Arm Limited and affiliates.
 * SPDX-License-Identifier: Apache-2.0
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "STModCellular.h"
#include "mbed_wait_api.h"
#include "mbed_trace.h"

#define TRACE_GROUP "CELL"

using namespace mbed;

STModCellular::STModCellular(FileHandle *fh) : QUECTEL_UG96(fh),
    m_powerkey(MBED_CONF_STMOD_CELLULAR_POWER),
    m_reset(MBED_CONF_STMOD_CELLULAR_RESET),
    m_simsel0(MBED_CONF_STMOD_CELLULAR_SIMSEL0),
    m_simsel1(MBED_CONF_STMOD_CELLULAR_SIMSEL1),
    m_mdmdtr(MBED_CONF_STMOD_CELLULAR_MDMDTR)
{
    // start with modem disabled
    m_mdmdtr.write(0);
    m_simsel0.write(MBED_CONF_STMOD_CELLULAR_SIM_SELECTION);
    m_simsel1.write(0);

    m_reset.write(1);
    wait_ms(250);
    m_reset.write(0);
}

STModCellular::~STModCellular()
{
}

nsapi_error_t STModCellular::soft_power_on() {
    nsapi_error_t err = QUECTEL_UG96::soft_power_on();
    if (err != 0) {
        return err;
    }
    m_powerkey.write(1);
    wait_ms(250);
    m_powerkey.write(0);

    // wait for RDY
    _at->lock();
    _at->set_at_timeout(5000);
    _at->set_stop_tag("RDY");
    bool rdy = _at->consume_to_stop_tag();
    tr_debug("Modem %sready to receive AT commands", rdy?"":"NOT ");
    (void)rdy;
    _at->restore_at_timeout();
    _at->unlock();

#ifdef DEVICE_SERIAL_FC
    if ((MBED_CONF_STMOD_CELLULAR_CTS != NC) && (MBED_CONF_STMOD_CELLULAR_RTS != NC)) {
        _at->lock();
        // enable CTS/RTS flowcontrol
        _at->set_stop_tag(mbed::OK);
        _at->set_at_timeout(400);
        _at->cmd_start("AT+IFC=");
        _at->write_int(2);
        _at->write_int(2);
        _at->cmd_stop_read_resp();

        err = _at->get_last_error();
        if (err == NSAPI_ERROR_OK) {
            tr_debug("Flow control turned ON");
        } else {
            tr_error("Failed to enable hw flow control");
        }

        _at->restore_at_timeout();
        _at->unlock();
    }
#endif
    return err;
}
nsapi_error_t STModCellular::soft_power_off() {
    _at->cmd_start("AT+QPOWD");
    _at->cmd_stop();
    wait_ms(1000);
    // should wait for POWERED DOWN with a time out up to 65 second according to the manual.
    // we cannot afford such a long wait though.
    return QUECTEL_UG96::soft_power_off();
}

#if MBED_CONF_STMOD_CELLULAR_PROVIDE_DEFAULT
#include "UARTSerial.h"
CellularDevice *CellularDevice::get_default_instance()
{
    static UARTSerial serial(MBED_CONF_STMOD_CELLULAR_TX, MBED_CONF_STMOD_CELLULAR_RX, MBED_CONF_STMOD_CELLULAR_BAUDRATE);
    static STModCellular device(&serial);
    return &device;
}
#endif


