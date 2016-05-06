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

#ifndef OBDEX_UTIL_HPP
#define OBDEX_UTIL_HPP

#include <obdex/ObdexDataTypes.hpp>
#include <vector>
#include <sstream>
#include <iomanip>

namespace obdex
{
    /// * Converts common types to std::string
    /// * Included instead of using std::to_string because the latter
    ///   is missing on Android
    template<typename T>
    std::string ToString(T const &val)
    {
        std::ostringstream oss;
        oss << val;
        return oss.str();
    }

    template<typename T>
    std::string ToStringFormat(T const &val,
                               uint precision,
                               uint width,
                               char fill)
    {
        std::ostringstream oss;
        oss << std::fixed
            << std::setw(width)
            << std::setfill(fill)
            << std::setprecision(precision) << val;
        return oss.str();
    }

    template<typename T>
    void PushFront(std::vector<T>& target,
                   T const &value)
    {
        target.insert(target.begin(),value);
    }

    std::string ToHexString(uint val);

    void PrependString(std::string& target,
                       std::string const &before);

    // Finds the first occurance of @value in @list_strings
    // and returns its index. Returns -1 if no match was found
    sint StringListIndexOf(
            std::vector<std::string> const &list_strings,
            std::string const &value);

    // Returns true of @str contains @value
    bool StringContains(std::string const &str,
                        std::string const &value);

    // Converts a string representing a binary,
    // hex, or decimal number into a uint

    // expect ONE token representing a number
    // if str contains '0bXXXXX' -> parse as binary
    // if str contains '0xXXXXX' -> parse as hex
    // else parse as dec
    uint StringToUInt(std::string const &str,bool& ok);

    // Converts an ASCII string to all upper/lower case
    std::string StringToUpper(std::string const &str);
    std::string StringToLower(std::string const &str);

    // Split @str into a list of strings wherever the
    // delimiter @dlm occurs

    // If @dlm does not occur in the string, a list
    // containing only @str is returned

    // Empty strings are discarded unless @keep_empty
    // is set to true
    std::vector<std::string>
    SplitString(std::string const &str,
                std::string const &dlm,
                bool keep_empty=false);
}

#endif // OBDEX_UTIL_HPP
