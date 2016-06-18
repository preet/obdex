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

#include <obdex/ObdexUtil.hpp>
#include <cstdlib>
#include <algorithm>

namespace obdex
{
    std::string ToHexString(uint val)
    {
        std::stringstream oss;
        oss << std::hex << val;
        return oss.str();
    }

    void PrependString(std::string& target,
                       std::string const &before)
    {
        target.insert(0,before);
    }

    sint StringListIndexOf(
            std::vector<std::string> const &list_strings,
            std::string const &value)
    {
        sint result = -1;
        for(uint i=0; i < list_strings.size(); i++)
        {
            if(list_strings[i]==value)
            {
                result = i;
                break;
            }
        }

        return result;
    }

    bool StringContains(std::string const &str,
                        std::string const &value)
    {
        return (str.find(value) != std::string::npos);
    }

    uint StringToUInt(std::string const &str,bool& ok)
    {
        int base=10;
        std::string num_str(str);
        if(str[0] == '0')   {
            if(str[1] == 'b')   {
                num_str = str.substr(2);
                base = 2;
            }
            else if(str[1] == 'x')   {
                num_str = str.substr(2);
                base = 16;
            }
        }

        char* endptr;
        auto val = strtoul(num_str.c_str(),&endptr,base);

        // endptr should point to the end of @num_str.c_str()
        // if parsing succeeded
        ok = (*endptr == '\0');
        return val;
    }

    std::string StringToUpper(std::string const &str)
    {
        std::string upper_str = str;
        for(auto& c : upper_str)
        {
            if(c > 96 && c < 123)
            {
                c -= 32;
            }
        }

        return upper_str;
    }

    std::string StringToLower(std::string const &str)
    {
        std::string lower_str = str;
        for(auto& c : lower_str)
        {
            if(c > 64 && c < 91)
            {
                c += 32;
            }
        }

        return lower_str;
    }

    std::vector<std::string>
    SplitString(std::string const &str,
                std::string const &dlm,
                bool keep_empty)
    {
        std::vector<std::string> list_strings;

        // ref: http://stackoverflow.com/questions/14265581/...
        // ...parse-split-a-string-in-c-using-string-delimiter-standard-c

        auto end = str.find(dlm);
        auto start = end;
        start = 0;

        while(end != std::string::npos)
        {
            list_strings.push_back(str.substr(start,end-start));
            start = end + dlm.length();
            end = str.find(dlm,start);
        }

        list_strings.push_back(str.substr(start,end));

        if(!keep_empty)
        {
            list_strings.erase(
                        std::remove_if(
                            list_strings.begin(),
                            list_strings.end(),
                            [](std::string const &s){
                                return s.empty();
                            }),
                        list_strings.end()
                    );
        }

        return list_strings;
    }
}
