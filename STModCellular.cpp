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

#include "AT_CellularPower.h"
#include "ATHandler.h"
#include "STModCellular.h"
#include "mbed_wait_api.h"
#include "gpio_api.h"
#include "mbed_trace.h"
#include "DigitalOut.h"
#include "QUECTEL_UG96_CellularPower.h"

#define TRACE_GROUP "CELL"

using namespace mbed;

namespace mbed {
class STModCellular_CellularPower: public QUECTEL_UG96_CellularPower {
    public:
        STModCellular_CellularPower(ATHandler &atHandler): QUECTEL_UG96_CellularPower(atHandler) {}
        ~STModCellular_CellularPower() {}

        nsapi_error_t on() {
            nsapi_error_t err = QUECTEL_UG96_CellularPower::on();
            if (err == 0) {
                DigitalOut powerkey(MBED_CONF_STMOD_CELLULAR_POWER);
                powerkey.write(1);
                wait_ms(250);
                powerkey.write(0);

                // wait for RDY
                _at.lock();
                _at.set_at_timeout(5000);
                _at.set_stop_tag("RDY");
                bool rdy = _at.consume_to_stop_tag();
                tr_debug("Modem %sready to receive AT commands", rdy?"":"NOT ");
                _at.unlock();

                _at.lock();
                // enable CTS/RTS flowcontrol
                _at.set_stop_tag(mbed::OK);
                _at.set_at_timeout(400);
                _at.cmd_start("AT+IFC=");
                _at.write_int(2);
                _at.write_int(2);
                _at.cmd_stop_read_resp();

                err = _at.get_last_error();
                if (err == NSAPI_ERROR_OK) {
                    tr_debug("Flow control turned ON");
                } else {
                    tr_error("Failed to enable hw flow control");
                }

                _at.restore_at_timeout();
                _at.unlock();
            }
            return err;
        }

        nsapi_error_t off() {
            _at.cmd_start("AT+QPOWD");
            _at.cmd_stop();
            wait_ms(1000);
            // should wait for POWERED DOWN with a time out up to 65 second according to the manual.
            // we cannot afford such a long wait though.
            return QUECTEL_UG96_CellularPower::off();
        }
};
};

STModCellular::STModCellular(FileHandle *fh) : QUECTEL_UG96(fh)
{
    DigitalOut reset(MBED_CONF_STMOD_CELLULAR_RESET);
    DigitalOut simsel0(MBED_CONF_STMOD_CELLULAR_SIMSEL0);
    DigitalOut simsel1(MBED_CONF_STMOD_CELLULAR_SIMSEL1);
    DigitalOut mdmdtr(MBED_CONF_STMOD_CELLULAR_MDMDTR);

    // start with modem disabled
    mdmdtr.write(0);
    simsel0.write(MBED_CONF_STMOD_CELLULAR_SIM_SELECTION);
    simsel1.write(0);

    reset.write(1);
    wait_ms(250);
    reset.write(0);
}

STModCellular::~STModCellular()
{
}

AT_CellularPower *STModCellular::open_power_impl(ATHandler &at)
{
    return new STModCellular_CellularPower(at);
}
