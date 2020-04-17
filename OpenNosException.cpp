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

#include "OpenNosException.hpp"
#include <sstream>

OpenNosException::OpenNosException(const Result::Value errorCode, std::string_view funcName, off_t line)
    : _errorCode { errorCode }, _funcName { funcName }, _line { line } {
    // Nothing more to do
}

std::string OpenNosException::translateErroCodeToErrorMsg() {
    return "";
}

std::string OpenNosException::getErrorMessage() {
    if (_errorMsg.size() == 0) {
        std::stringstream buffer {};
        buffer << "[";
        buffer << _funcName;
        buffer << "():";
        buffer << _line;
        buffer << "] ";
        buffer << translateErroCodeToErrorMsg();
        buffer << " (";
        buffer << std::to_string(static_cast<uint16_t>(_errorCode));
        buffer << ")";
        _errorMsg = buffer.str();
    }

    return _errorMsg;
}


