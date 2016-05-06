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
#include <obdex/ObdexUtil.hpp>
#include <obdex/ObdexLog.hpp>
#include <obdex/ObdexParser.hpp>

using namespace obdex;

TEST_CASE("TestUtils","[utils]")
{
    SECTION("Convert int to hex string")
    {
        bool ok=false;
        std::string hex_str;

        hex_str = ToHexString(0);
        REQUIRE(hex_str=="0");

        hex_str = ToHexString(10);
        ok = hex_str=="A" || hex_str=="a";
        REQUIRE(ok);

        hex_str = ToHexString(255);
        ok = hex_str=="FF" || hex_str=="ff";
        REQUIRE(ok);
    }

    SECTION("Prepend string")
    {
        std::string a = "World";
        std::string b = "Hello ";
        PrependString(a,b);
        REQUIRE(a == "Hello World");
    }

    SECTION("Find in string list")
    {
        std::vector<std::string> list_fruits{
            "apple",
            "banana",
            "cantaloupe",
            "mango",
            "guava",
            "lime"
        };

        REQUIRE(StringListIndexOf(list_fruits,"apple")==0);
        REQUIRE(StringListIndexOf(list_fruits,"cantaloupe")==2);
        REQUIRE(StringListIndexOf(list_fruits,"lime")==5);
        REQUIRE(StringListIndexOf(list_fruits,"orange")==-1);
        REQUIRE(StringListIndexOf(list_fruits,"grape")==-1);
    }

    SECTION("Find in string")
    {
        std::string text = "This is a message";
        REQUIRE(StringContains(text,"This"));
        REQUIRE(StringContains(text,"a m"));
        REQUIRE(StringContains(text,"sage"));
        REQUIRE(StringContains(text,"Tzhisz")==false);
    }

    SECTION("Convert int to string")
    {
        std::string s0 = "00";
        std::string s1 = "123";
        std::string s2 = "z532";
        std::string s3 = "436z";

        std::string s4 = "0x0";
        std::string s5 = "0xa";
        std::string s6 = "0xFF";
        std::string s7 = "0xFFZ";
        std::string s8 = "0xxF";

        std::string s9 = "0b0";
        std::string s10 = "0b010";
        std::string s11 = "0b11";
        std::string s12 = "0b100";
        std::string s13 = "0bb0";
        std::string s14 = "0b010z";

        bool ok = false;
        uint i = 0;

        i = StringToUInt(s0,ok);
        REQUIRE(i==0);
        REQUIRE(ok);

        i = StringToUInt(s1,ok);
        REQUIRE(i==123);
        REQUIRE(ok);

        i = StringToUInt(s2,ok);
        REQUIRE(!ok);

        i = StringToUInt(s3,ok);
        REQUIRE(!ok);

        i = StringToUInt(s4,ok);
        REQUIRE(i==0);
        REQUIRE(ok);

        i = StringToUInt(s5,ok);
        REQUIRE(i==10);
        REQUIRE(ok);

        i = StringToUInt(s6,ok);
        REQUIRE(i==255);
        REQUIRE(ok);

        i = StringToUInt(s7,ok);
        REQUIRE(!ok);

        i = StringToUInt(s8,ok);
        REQUIRE(!ok);

        i = StringToUInt(s9,ok);
        REQUIRE(i==0);
        REQUIRE(ok);

        i = StringToUInt(s10,ok);
        REQUIRE(i==2);
        REQUIRE(ok);

        i = StringToUInt(s11,ok);
        REQUIRE(i==3);
        REQUIRE(ok);

        i = StringToUInt(s12,ok);
        REQUIRE(i==4);
        REQUIRE(ok);

        i = StringToUInt(s13,ok);
        REQUIRE(!ok);

        i = StringToUInt(s14,ok);
        REQUIRE(!ok);
    }

    SECTION("ASCII string to upper/lower case")
    {
        auto a = StringToUpper("az hello world az");
        REQUIRE(a=="AZ HELLO WORLD AZ");

        auto b = StringToUpper("aZ HeLlO WoRlD Az");
        REQUIRE(b=="AZ HELLO WORLD AZ");

        auto c = StringToLower("AZ HELLO WORLD AZ");
        REQUIRE(c=="az hello world az");

        auto d = StringToLower("aZ hElLo wOrLd Az");
        REQUIRE(d=="az hello world az");
    }

    SECTION("Split string with delimiter")
    {
        std::vector<std::string> list_strings;

        std::string text = "This is a message";
        list_strings = SplitString(text," ",true);
        REQUIRE(list_strings[0]=="This");
        REQUIRE(list_strings[1]=="is");
        REQUIRE(list_strings[2]=="a");
        REQUIRE(list_strings[3]=="message");

        text = " This is a message ";
        list_strings = SplitString(text," ",true);
        REQUIRE(list_strings[0]=="");
        REQUIRE(list_strings[1]=="This");
        REQUIRE(list_strings[2]=="is");
        REQUIRE(list_strings[3]=="a");
        REQUIRE(list_strings[4]=="message");
        REQUIRE(list_strings[5]=="");

        text = " This is a message ";
        list_strings = SplitString(text," ",false);
        REQUIRE(list_strings[0]=="This");
        REQUIRE(list_strings[1]=="is");
        REQUIRE(list_strings[2]=="a");
        REQUIRE(list_strings[3]=="message");

        text = "..mess..age..";
        list_strings = SplitString(text,".",true);
        REQUIRE(list_strings[0]=="");
        REQUIRE(list_strings[1]=="");
        REQUIRE(list_strings[2]=="mess");
        REQUIRE(list_strings[3]=="");
        REQUIRE(list_strings[4]=="age");
        REQUIRE(list_strings[5]=="");
        REQUIRE(list_strings[6]=="");

        text = "..mess..age..";
        list_strings = SplitString(text,".",false);
        REQUIRE(list_strings[0]=="mess");
        REQUIRE(list_strings[1]=="age");


        // try with a weird delimiter
        std::string dlm = "!@#";
        text = "This!@#is!@#a!@#message";
        list_strings = SplitString(text,dlm,true);
        REQUIRE(list_strings[0]=="This");
        REQUIRE(list_strings[1]=="is");
        REQUIRE(list_strings[2]=="a");
        REQUIRE(list_strings[3]=="message");

        text = "!@#This!@#is!@#a!@#message!@#";
        list_strings = SplitString(text,dlm,true);
        REQUIRE(list_strings[0]=="");
        REQUIRE(list_strings[1]=="This");
        REQUIRE(list_strings[2]=="is");
        REQUIRE(list_strings[3]=="a");
        REQUIRE(list_strings[4]=="message");
        REQUIRE(list_strings[5]=="");

        text = "!@#This!@#is!@#a!@#message!@#";
        list_strings = SplitString(text,dlm,false);
        REQUIRE(list_strings[0]=="This");
        REQUIRE(list_strings[1]=="is");
        REQUIRE(list_strings[2]=="a");
        REQUIRE(list_strings[3]=="message");

        text = "!@#!@#mess!@#!@#age!@#!@#";
        list_strings = SplitString(text,dlm,true);
        REQUIRE(list_strings[0]=="");
        REQUIRE(list_strings[1]=="");
        REQUIRE(list_strings[2]=="mess");
        REQUIRE(list_strings[3]=="");
        REQUIRE(list_strings[4]=="age");
        REQUIRE(list_strings[5]=="");
        REQUIRE(list_strings[6]=="");

        text = "!@#!@#mess!@#!@#age!@#!@#";
        list_strings = SplitString(text,dlm,false);
        REQUIRE(list_strings[0]=="mess");
        REQUIRE(list_strings[1]=="age");
    }
}
