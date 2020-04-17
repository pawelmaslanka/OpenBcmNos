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

#pragma once

#include "Command.hpp"
#include "LoggingFacility.hpp"
#include "Types.hpp"

extern "C" {
#   include <opennsl/error.h>
}

#define CALL_CALLBACK_AND_RETURN_IF_OPENNSL_FAIL(RESULT, CALLBACK)                                  \
    do {                                                                                            \
        if (OPENNSL_FAILURE(RESULT)) {                                                              \
            auto errorMsg = ("Failed on BCM API call: %s (%d)", opennsl_errmsg(RESULT), RESULT);    \
            ERROR_LOG(errorMsg);                                                                    \
            CALL_CALLBACK_AND_RETURN_RESULT_FAIL(CALLBACK);                                         \
        }                                                                                           \
    } while (0)
