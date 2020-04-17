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

#include "Types.hpp"

#include <string>
#include <string_view>

class OpenNosException {
  public:
    OpenNosException(const Result::Value errorCode, std::string_view funcName, off_t line);
    std::string getErrorMessage();

  private:
    std::string translateErroCodeToErrorMsg();
    std::string _errorMsg;
    Result::Value _errorCode;
    std::string_view _funcName;
    off_t _line;
};

#define MAKE_OPENNOS_EXCEPTION(ERROR_CODE) \
    OpenNosException(ERROR_CODE, __FUNCTION__, __LINE__)
