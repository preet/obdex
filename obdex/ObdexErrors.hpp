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

#ifndef OBDEX_ERRORS_HPP
#define OBDEX_ERRORS_HPP

#include <exception>
#include <vector>
#include <obdex/ObdexLog.hpp>

namespace obdex
{
    // ============================================================= //

    class Exception : public std::exception
    {
    public:
        using ErrorLevel = obdex::Logger::Level;

        Exception();
        Exception(ErrorLevel err_lvl,std::string msg);
        virtual ~Exception();

        virtual const char* what() const noexcept;

    protected:
        static std::vector<std::string> const m_lkup_err_lvl;
        std::string m_msg;
    };

    // ============================================================= //

    class XMLParsingFailed : public Exception
    {
    public:
        XMLParsingFailed(std::string msg);
        ~XMLParsingFailed();
    };

    // ============================================================= //

    class JSContextSetupFailed : public Exception
    {
    public:
        JSContextSetupFailed();
        ~JSContextSetupFailed();
    };

    // ============================================================= //

    class BuildParamFrameFailed : public Exception
    {
    public:
        BuildParamFrameFailed(std::string msg);
        ~BuildParamFrameFailed();
    };

    // ============================================================= //

    class ParseParamFrameFailed : public Exception
    {
    public:
        ParseParamFrameFailed(std::string msg);
        ~ParseParamFrameFailed();
    };

    // ============================================================= //

    class InvalidHexStr : public Exception
    {
    public:
        InvalidHexStr();
        ~InvalidHexStr();
    };

    // ============================================================= //
}

#endif // OBDEX_ERRORS_HPP

