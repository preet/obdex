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

#define CATCH_CONFIG_RUNNER
#include <obdex/test/catch/catch.hpp>

#include <obdex/test/ObdexTestHelpers.hpp>

namespace obdex
{
    namespace test
    {
        void CLISetDefinitionsFile(Catch::ConfigData&,std::string const &s)
        {
            obdex::test::cli_definitions_file = s;
        }

        void CLISetSpec(Catch::ConfigData&,std::string const &s)
        {
            obdex::test::cli_spec = s;
        }

        void CLISetProtocol(Catch::ConfigData&,std::string const &s)
        {
            obdex::test::cli_protocol = s;
        }

        void CLISetAddress(Catch::ConfigData&,std::string const &s)
        {
            obdex::test::cli_address = s;
        }
    }
}

int main(int argc, char * const argv[])
{
    // ref:
    // https://github.com/philsquared/Catch/issues/622
    Catch::Session session;

    // NOTE: This requires a modified version of catch, where
    // session.cli() returns a non-const reference
    auto& cli = session.cli();

    cli["--obdex-definitions-file"]
            .describe("Path to definitions file")
            .bind(&obdex::test::CLISetDefinitionsFile,"definitions file");

    cli["--obdex-spec"]
            .describe("obd spec")
            .bind(&obdex::test::CLISetSpec,"spec");

    cli["--obdex-protocol"]
            .describe("obd protocol")
            .bind(&obdex::test::CLISetProtocol,"protocol");

    cli["--obdex-address"]
            .describe("obd address")
            .bind(&obdex::test::CLISetAddress,"address");

    int result = session.applyCommandLine(argc,argv);

    if(result==0)
    {
        result = session.run();
    }

    return result;
}
