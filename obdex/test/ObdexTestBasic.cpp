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

        void TestLegacy(Parser& parser, bool const randomize_header)
        {
            auto list_param_names =
                    parser.GetParameterNames("TEST","ISO 9141-2","Default");

            for(auto const &param_name : list_param_names)
            {
                ParameterFrame param;
                param.spec = "TEST";
                param.protocol = "ISO 9141-2";
                param.address = "Default";
                param.name = param_name;

                // Build
                parser.BuildParameterFrame(param);

                // Simulate response
                if(     param.name == "T_REQ_NONE_RESP_SF_PARSE_SEP")   {
                    sim_vehicle_message_legacy(param,1,randomize_header);
                }
                else if(param.name == "T_REQ_NON_RESP_MF_PARSE_COMBINED")   {
                    sim_vehicle_message_legacy(param,1,randomize_header);
                    sim_vehicle_message_legacy(param,1,randomize_header);
                    sim_vehicle_message_legacy(param,1,randomize_header);
                }
                else if(param.name == "T_REQ_SINGLE_RESP_SF_PARSE_SEP")   {
                    sim_vehicle_message_legacy(param,1,randomize_header);
                }
                else if(param.name == "T_REQ_SINGLE_RESP_MF_PARSE_SEP")   {
                    sim_vehicle_message_legacy(param,2,randomize_header);
                    sim_vehicle_message_legacy(param,2,randomize_header);
                }
                else if(param.name == "T_REQ_MULTI_RESP_SF_PARSE_SEP")   {
                    sim_vehicle_message_legacy(param,1,randomize_header);
                }
                else if(param.name == "T_REQ_MULTI_RESP_SF_PARSE_COMBINED")   {
                    sim_vehicle_message_legacy(param,1,randomize_header);
                }
                else if(param.name == "T_REQ_MULTI_RESP_MF_PARSE_COMBINED")   {
                    sim_vehicle_message_legacy(param,2,randomize_header);
                    sim_vehicle_message_legacy(param,1,randomize_header);
                }
                else   {
                    continue;
                }

                // Parse
                std::vector<obdex::Data> list_data;
                parser.ParseParameterFrame(param,list_data);

                // Debug output
                if(g_debug_output)
                {
                    print_message_frame(param);
                    print_parsed_data(list_data);
                }
            }
        }

        // ============================================================= //

        void TestISO14230(Parser& parser,
                          bool const randomize_header,
                          uint const header_length,
                          bool const check_header_format_byte)
        {
            auto list_param_names =
                    parser.GetParameterNames("TEST","ISO 14230","Default");

            for(auto const &param_name : list_param_names)
            {
                ParameterFrame param;
                param.spec = "TEST";
                param.protocol = "ISO 14230";
                param.address = "Default";
                param.name = param_name;

                // Build
                parser.BuildParameterFrame(param);

                // Simulate response
                if(check_header_format_byte==false)
                {
                    // just makes testing multiple header lengths
                    // at once a bit easier
                    for(uint j=0; j < param.list_message_data.size(); j++)
                    {
                        param.list_message_data[j].exp_header_mask[0] = 0x00;
                    }
                }

                if(     param.name == "T_REQ_NONE_RESP_SF_PARSE_SEP")   {
                    sim_vehicle_message_iso14230(param,1,randomize_header,header_length);
                }
                else if(param.name == "T_REQ_NON_RESP_MF_PARSE_COMBINED")   {
                    sim_vehicle_message_iso14230(param,1,randomize_header,header_length);
                    sim_vehicle_message_iso14230(param,1,randomize_header,header_length);
                    sim_vehicle_message_iso14230(param,1,randomize_header,header_length);
                }
                else if(param.name == "T_REQ_SINGLE_RESP_SF_PARSE_SEP")   {
                    sim_vehicle_message_iso14230(param,1,randomize_header,header_length);
                }
                else if(param.name == "T_REQ_SINGLE_RESP_MF_PARSE_SEP")   {
                    sim_vehicle_message_iso14230(param,2,randomize_header,header_length);
                    sim_vehicle_message_iso14230(param,2,randomize_header,header_length);
                }
                else if(param.name == "T_REQ_MULTI_RESP_SF_PARSE_SEP")   {
                    sim_vehicle_message_iso14230(param,1,randomize_header,header_length);
                }
                else if(param.name == "T_REQ_MULTI_RESP_SF_PARSE_COMBINED")   {
                    sim_vehicle_message_iso14230(param,1,randomize_header,header_length);
                }
                else if(param.name == "T_REQ_MULTI_RESP_MF_PARSE_COMBINED")   {
                    sim_vehicle_message_iso14230(param,2,randomize_header,header_length);
                    sim_vehicle_message_iso14230(param,1,randomize_header,header_length);
                }
                else   {
                    continue;
                }

                // Parse
                std::vector<obdex::Data> list_data;
                parser.ParseParameterFrame(param,list_data);

                // Debug output
                if(g_debug_output)
                {
                    print_message_frame(param);
                    print_parsed_data(list_data);
                }
            }
        }

        // ============================================================= //

        void TestISO15765(Parser & parser,
                          bool const randomize_header,
                          bool const extended_id)
        {
            std::string protocol = (extended_id) ?
                        "ISO 15765 Extended Id" :
                        "ISO 15765 Standard Id";

            std::vector<std::string> list_param_names =
                    parser.GetParameterNames(
                        "TEST",protocol,"Default");

            for(auto const &param_name : list_param_names)
            {
                ParameterFrame param;
                param.spec = "TEST";
                param.protocol = protocol;
                param.address = "Default";
                param.name = param_name;

                // Build
                parser.BuildParameterFrame(param);

                // Simulate response
                if(     param.name == "T_REQ_NONE_RESP_SF_PARSE_SEP")   {
                    sim_vehicle_message_iso15765(param,1,randomize_header);
                }
                else if(param.name == "T_REQ_NON_RESP_MF_PARSE_COMBINED")   {
                    sim_vehicle_message_iso15765(param,1,randomize_header);
                    sim_vehicle_message_iso15765(param,1,randomize_header);
                    sim_vehicle_message_iso15765(param,1,randomize_header);
                }
                else if(param.name == "T_REQ_SINGLE_RESP_SF_PARSE_SEP")   {
                    sim_vehicle_message_iso15765(param,1,randomize_header);
                }
                else if(param.name == "T_REQ_SINGLE_RESP_MF_PARSE_SEP")   {
                    sim_vehicle_message_iso15765(param,3,randomize_header);
                    sim_vehicle_message_iso15765(param,4,randomize_header);
                }
                else if(param.name == "T_REQ_MULTI_RESP_SF_PARSE_SEP")   {
                    sim_vehicle_message_iso15765(param,1,randomize_header);
                }
                else if(param.name == "T_REQ_MULTI_RESP_SF_PARSE_COMBINED")   {
                    sim_vehicle_message_iso15765(param,1,randomize_header);
                }
                else if(param.name == "T_REQ_MULTI_RESP_MF_PARSE_COMBINED")   {
                    sim_vehicle_message_iso15765(param,2,randomize_header);
                    sim_vehicle_message_iso15765(param,1,randomize_header);
                }
                else   {
                    continue;
                }

                // Parse
                std::vector<obdex::Data> list_data;
                parser.ParseParameterFrame(param,list_data);

                // Debug output
                if(g_debug_output)
                {
                    print_message_frame(param);
                    print_parsed_data(list_data);
                }
            }
        }

        // ============================================================= //
    }
}

using namespace obdex;

TEST_CASE("TestBasic","[basic]")
{
    if(test::cli_definitions_file.empty())
    {
        obdexlog.Trace() << "\n**********************************************************************\n"
                         << "Invalid args for TestBasic\n"
                            "To run TestBasic, pass in the test definitions\n"
                            "file as an argument: \n"
                            "./test_obdex TestBasic --obdex-definitions-file /path/to/test.xml\n"
                            "**********************************************************************\n";

        bool cli_defn_file_avail=false;
        REQUIRE(cli_defn_file_avail);
    }

    obdex::Parser parser(test::cli_definitions_file);
    bool randomize_headers = true;

    test::g_debug_output = false;

    SECTION("test legacy (sae j1850, iso 9141)")
    {
        test::TestLegacy(parser,randomize_headers);
    }

    SECTION("test iso 14230 (1 byte header)")
    {
        test::TestISO14230(parser,randomize_headers,1,false);
    }

    SECTION("test iso 14230 (2 byte header)")
    {
        test::TestISO14230(parser,randomize_headers,2,false);
    }

    SECTION("test iso 14230 (3 byte header)")
    {
        test::TestISO14230(parser,randomize_headers,3,false);
    }

    SECTION("test iso 14230 (4 byte header)")
    {
        test::TestISO14230(parser,randomize_headers,4,false);
    }

    SECTION("test iso 15765 (standard ids)")
    {
        test::TestISO15765(parser,randomize_headers,false);
    }

    SECTION("test iso 15765 (extended ids)")
    {
        test::TestISO15765(parser,randomize_headers,true);
    }
}
