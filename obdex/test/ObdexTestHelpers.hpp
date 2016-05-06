/*
   Copyright (C) 2012-2016 Preet Desai (preet.desai@gmail.com)

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
*/

#include <obdex/ObdexDataTypes.hpp>

#ifndef OBDEX_TEST_HELPERS_HPP
#define OBDEX_TEST_HELPERS_HPP

namespace obdex
{
    namespace test
    {
        extern std::string cli_definitions_file;
        extern std::string cli_spec;
        extern std::string cli_protocol;
        extern std::string cli_address;

        extern std::string g_test_desc;
        extern bool g_debug_output;

        ubyte GenRandomByte();

        std::string bytelist_to_string(ByteList const &byte_list);

        void sim_vehicle_message_legacy(ParameterFrame &param,
                                        uint const frames,
                                        bool const randomHeader);

        void sim_vehicle_message_iso14230(ParameterFrame &param,
                                          uint const frames,
                                          bool const randomizeHeader,
                                          uint const iso14230_headerLength=3);

        void sim_vehicle_message_iso15765(ParameterFrame &param,
                                          uint const frames,
                                          bool const randomizeHeader);

        void print_message_data(MessageData const &msgData);

        void print_message_frame(ParameterFrame const &msgFrame);

        void print_parsed_data(std::vector<obdex::Data> &listData);
    }
}

#endif // OBDEX_TEST_HELPERS_HPP
