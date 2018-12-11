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

#define TRACE_GROUP "CELL"

using namespace mbed;

STModCellular::STModCellular(FileHandle *fh) : QUECTEL_UG96(fh)
{
    CellularPower *power = open_power();
    // make sure the modem is off first
    power->off();

    DigitalOut reset(MBED_CONF_STMOD_CELLULAR_RESET);
    DigitalOut powerkey(MBED_CONF_STMOD_CELLULAR_POWER);
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

    powerkey.write(1);
    wait_ms(250);
    powerkey.write(0);
}

STModCellular::~STModCellular()
{
    CellularPower *power = open_power();
    // make sure the modem is off first
    power->off();
}
