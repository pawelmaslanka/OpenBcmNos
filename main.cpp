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

#include <iostream>

#include "Asic.hpp"
#include "PortManager.hpp"
#include "Switching.hpp"
#include "Utils/LoggingFacility.hpp"

using namespace std;

int main()
{
    Asic::Handle asic = std::make_shared<Asic>();
    PortManager::Handle portManager = std::make_shared<PortManager>();
    Switching::Handle switching = std::make_shared<Switching>(asic, portManager);
    if (Failed(switching->init())) {
        cout << "Failed initialize switch" << endl;
    }

    cout << "Hello World!" << endl;
    return 0;
}
