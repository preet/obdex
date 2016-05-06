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

#include <obdex/ObdexErrors.hpp>
#include <obdex/ObdexDataTypes.hpp>

namespace obdex
{
    // ============================================================= //

    const std::vector<std::string> Exception::m_lkup_err_lvl {
        "TRACE: ",
        "DEBUG: ",
        "INFO:  ",
        "WARN:  ",
        "ERROR: ",
        "FATAL: "
    };

    Exception::Exception()
    {}

    Exception::Exception(ErrorLevel err_lvl, std::string msg)
    {
        m_msg = m_lkup_err_lvl[static_cast<u8>(err_lvl)] + msg;
    }

    Exception::~Exception()
    {}

    const char* Exception::what() const noexcept
    {
        return m_msg.c_str();
    }

    // ============================================================= //

    XMLParsingFailed::XMLParsingFailed(std::string msg) :
        Exception(ErrorLevel::ERROR,std::move(msg))
    {}

    XMLParsingFailed::~XMLParsingFailed()
    {}

    // ============================================================= //

    JSContextSetupFailed::JSContextSetupFailed() :
        Exception(ErrorLevel::ERROR,"")
    {}

    JSContextSetupFailed::~JSContextSetupFailed()
    {}

    // ============================================================= //

    BuildParamFrameFailed::BuildParamFrameFailed(std::string msg) :
        Exception(ErrorLevel::ERROR,std::move(msg))
    {}

    BuildParamFrameFailed::~BuildParamFrameFailed()
    {}

    // ============================================================= //

    ParseParamFrameFailed::ParseParamFrameFailed(std::string msg) :
        Exception(ErrorLevel::ERROR,std::move(msg))
    {}

    ParseParamFrameFailed::~ParseParamFrameFailed()
    {}

    // ============================================================= //

    InvalidHexStr::InvalidHexStr() :
        Exception(ErrorLevel::ERROR,"")
    {}

    InvalidHexStr::~InvalidHexStr()
    {}

    // ============================================================= //
}
