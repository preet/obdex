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

#include <obdex/test/catch/catch.hpp>
#include <obdex/test/ObdexTestHelpers.hpp>
#include <obdex/ObdexUtil.hpp>
#include <obdex/ObdexLog.hpp>
#include <obdex/ObdexParser.hpp>

namespace obdex
{
    namespace test
    {
        // ============================================================= //
    }
}

using namespace obdex;

TEST_CASE("TestSpec","[spec]")
{
    if(test::cli_definitions_file.empty() ||
       test::cli_spec.empty() ||
       test::cli_protocol.empty() ||
       test::cli_address.empty())
    {
        obdexlog.Trace() << "\n**********************************************************************\n"
                         << "Invalid args for TestSpec\n"
                            "To run TestSpec, pass in: definitions file, spec, protocol, address:\n"
                            "./test_obdex TestSpec \\\n"
                            "--obdex-definitions-file /path/to/obd2.xml \\\n"
                            "--obdex-spec \"SAEJ1979\" \\\n"
                            "--obdex-protocol \"ISO 14230\" \\\n"
                            "--obdex-address \"Default\" \n"
                            "**********************************************************************\n";

        bool cli_spec_params_avail=false;
        REQUIRE(cli_spec_params_avail);
    }

    std::string test_spec_args_summary =
            test::cli_definitions_file + ", " +
            test::cli_spec + ", " +
            test::cli_protocol + ", " +
            test::cli_address;

    obdex::Parser parser(test::cli_definitions_file);

    auto list_param_names =
            parser.GetParameterNames(
                test::cli_spec,
                test::cli_protocol,
                test::cli_address);

    if(list_param_names.empty())
    {
        obdexlog.Trace() << "TestSpec: No Parameters found with given arguments:";
        obdexlog.Trace() << test_spec_args_summary;
        REQUIRE(false);
    }

    bool randomize_headers = true;
    test::g_debug_output = false;


    for(auto const &param_name : list_param_names)
    {
        ParameterFrame param;
        param.spec = test::cli_spec;
        param.protocol = test::cli_protocol;
        param.address = test::cli_address;
        param.name = param_name;

        // Build
        parser.BuildParameterFrame(param);

        // Simulate response
        if(param.parse_protocol < 0xA00)   {
            test::sim_vehicle_message_legacy(param,1,randomize_headers);
            test::sim_vehicle_message_legacy(param,3,randomize_headers);
        }
        else if(param.parse_protocol == obdex::PROTOCOL_ISO_14230)   {
            test::sim_vehicle_message_iso14230(param,1,randomize_headers);
            test::sim_vehicle_message_iso14230(param,3,randomize_headers);
        }
        else if(param.parse_protocol == obdex::PROTOCOL_ISO_15765)   {
            test::sim_vehicle_message_iso15765(param,1,randomize_headers);
            test::sim_vehicle_message_iso15765(param,3,randomize_headers);
        }
        else   {
            obdexlog.Trace() << "TestSpec: Error: unknown protocol " << test::cli_protocol;
            REQUIRE(false);
        }

        // Parse
        std::vector<obdex::Data> list_data;
        parser.ParseParameterFrame(param,list_data);

        // Debug output
        if(test::g_debug_output)
        {
            test::print_message_frame(param);
            test::print_parsed_data(list_data);
        }
    }
}
