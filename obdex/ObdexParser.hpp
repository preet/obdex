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

#ifndef OBDEX_PARSER_HPP
#define OBDEX_PARSER_HPP

// pugixml
#include <obdex/pugixml/pugixml.hpp>

// duktape
#include <obdex/duktape/duktape.h>

// obdex
#include <obdex/ObdexDataTypes.hpp>

#include <unordered_map>

namespace obdex
{
    class Parser
    {
    public:
        Parser(std::string const &file_path);

        ~Parser();

        // BuildParameterFrame
        // * uses the definitions file to build
        //   up request message data for the spec,
        //   protocol and param defined in msgFrame
        void BuildParameterFrame(ParameterFrame& param_frame) const;


        // ParseParameterFrame
        // * parses vehicle response data defined
        //   in msgFrame[i].listRawDataFrames and
        //   saves it in listDataResults
        void ParseParameterFrame(
                ParameterFrame &msgFrame,
                std::vector<Data> &listDataResults);


        // GetParameterNames
        // * returns a list of parameter names from
        //   the definitions file based on input args
        std::vector<std::string>
        GetParameterNames(std::string const &specName,
                          std::string const &protocolName,
                          std::string const &addressName) const;


        // helpers to convert bytes into strings and vice versa
        std::string ConvUByteToHexStr(ubyte byte) const;
        ubyte ConvHexStrToUByte(std::string const &str) const;

    private:
        void jsInit();

        void buildHeader_Legacy(
                ParameterFrame &param_frame,
                pugi::xml_node xn_address) const;

        void buildHeader_ISO_14230(
                ParameterFrame &param_frame,
                pugi::xml_node xn_address) const;

        void buildHeader_ISO_15765(
                ParameterFrame &param_frame,
                pugi::xml_node xn_address) const;

        void buildData(
                ParameterFrame & param_frame,
                pugi::xml_node xn_parameter) const;

        // parseResponse
        // * passes data processed by cleanRawData[] to
        //   the javascript engine and uses the script
        //   defined in the definitions file to convert
        //   response data into meaningful values, which
        //   is saved in listData
        // * parseMode == PER_RESP_DATA
        //   the script is run once for each cleaned
        //   response in MessageData
        // * parseMode == PER_MESSAGE
        //   the script is run one for the entire
        //   MessageData, however many responses
        //   there are [not implemented yet]
        void parseResponse(ParameterFrame const &msg_frame,
                           std::vector<Data> &list_data);

        // saveNumAndLitData
        // * helper function that saves the numerical
        //   and literal data interpreted with the
        //   vehicle response from the js context
        void saveNumAndLitData(Data &myData);

        // cleanFrames_[...]
        // * cleans up rawDataFrames by checking for
        //   expected message bytes and groups/merges
        //   the frames as appropriate to save the
        //   data in listHeaders and listCleanData

        // cleanFrames_Legacy
        // * includes: SAEJ1850 VPW/PWM,ISO 9141-2,ISO 14230-4
        //   (not suitable for any other ISO 14230)
        void cleanFrames_Legacy(MessageData &msg);

        // cleanFrames_ISO14230
        void cleanFrames_ISO_14230(MessageData &msg);

        // cleanFrames_ISO_15765
        // * includes: ISO 15765, 11-bit and 29-bit headers,
        //   and ISO 15765-2/ISO-TP (multiframe messages)
        void cleanFrames_ISO_15765(MessageData &msg,
                                   int const headerLength);

        // checkHeaderBytes
        // * checks bytes against expected bytes with a mask
        // * returns false if the masked values do not match
        bool checkBytesAgainstMask(ByteList const &expBytes,
                                   ByteList const &expMask,
                                   ByteList const &bytes);

        // checkAndRemoveDataPrefix
        // * checks data bytes against the expected prefix
        // * removes prefix from data bytes
        // * returns false if prefix doesn't match
        bool checkAndRemoveDataPrefix(ByteList const &expDataPrefix,
                                      ByteList &dataBytes);




        std::vector<std::string> m_lkup_ubyte_hex_str;
        std::unordered_map<std::string,u8> m_lkup_hex_str_ubyte;


        // pugixml vars
        std::string const m_xml_file_path;
        pugi::xml_document m_xml_doc;

        // duktape
        duk_context * m_js_ctx;
        u32 m_js_idx_global_object;
        u32 m_js_idx_f_add_databytes;
        u32 m_js_idx_f_add_msg_data;
        u32 m_js_idx_f_clear_data;
        u32 m_js_idx_f_get_lit_data;
        u32 m_js_idx_f_get_num_data;

        // duktape javascript parse function registry
        std::vector<std::string> m_js_list_function_key;
        std::vector<u32> m_js_list_function_idx;
    };
}

#endif // OBDEX_PARSER_HPP
