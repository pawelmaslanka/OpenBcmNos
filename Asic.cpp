// Copyright 2020 - Present | Pawel Maslanka (pawmas.pawelmaslanka@gmail.com)
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "Asic.hpp"
#include "LoggingFacility.hpp"

extern "C" {
#   include <opennsl/error.h>
#   include <sal/driver.h>
}

Asic::Asic()
{

}

Result::Value Asic::init() {
    /* Initialize the system. */
    int rv = opennsl_driver_init(nullptr);

    if (rv != OPENNSL_E_NONE) {
        VLOG_ERR("Failed to initialize the system.  rc=%s",
                 opennsl_errmsg(rv));
        return Result::Value::Fail;
    }

    return Result::Value::Success;
}

int Asic::getMaxPorts(const int hwUnit) {
    switch (hwUnit) {
      default:
            return 129; // For DX010 that is 32x4 + 1
    }
}
