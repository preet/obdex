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

#include <obdex/ObdexParser.hpp>
#include <obdex/ObdexErrors.hpp>
#include <obdex/ObdexUtil.hpp>
#include <obdex/ObdexJSGlobals.hpp>

namespace obdex
{
    // ============================================================= //
    // ============================================================= //

    Parser::Parser(std::string const &file_path) :
        m_xml_file_path(file_path)
    {
        // fill in lookup maps for hex str conversion
        for(uint i=0; i < 256; i++)
        {
            auto hex_byte_str_uppercase = StringToUpper(ToHexString(i));

            // zero pad to two digits
            if(hex_byte_str_uppercase.size() < 2)
            {
                PrependString(hex_byte_str_uppercase,"0");
            }

            m_lkup_ubyte_hex_str.push_back(hex_byte_str_uppercase);
            m_lkup_hex_str_ubyte.emplace(hex_byte_str_uppercase,u8(i));
            m_lkup_hex_str_ubyte.emplace(StringToLower(hex_byte_str_uppercase),u8(i));
        }

        // read in xml file
        auto xml_parse_result = m_xml_doc.load_file(file_path.c_str());

        if(!xml_parse_result)
        {
            std::string err_desc;
            err_desc += "Could not parse XML file: " + file_path;
            err_desc += ": \n";
            err_desc += xml_parse_result.description();
            err_desc += ": \n offset char: ";
            err_desc += ToString(s64(xml_parse_result.offset));

            throw XMLParsingFailed(err_desc);
        }

        // setup js context
        jsInit();
    }

    Parser::~Parser()
    {}

    // ============================================================= //
    // ============================================================= //

    void Parser::BuildParameterFrame(ParameterFrame &param_frame) const
    {
        bool xn_spec_found       = false;
        bool xn_protocol_found   = false;
        bool xn_address_found    = false;
        bool xn_params_found     = false;
        bool xn_parameter_found  = false;

        pugi::xml_node xn_spec = m_xml_doc.child("spec");
        for(; xn_spec!=NULL; xn_spec=xn_spec.next_sibling("spec"))
        {
            std::string const spec(xn_spec.attribute("name").value());
            if(spec == param_frame.spec)
            {   // found spec
                xn_spec_found = true;
                pugi::xml_node xn_protocol = xn_spec.child("protocol");
                for(; xn_protocol!=NULL; xn_protocol=xn_protocol.next_sibling("protocol"))
                {
                    std::string const protocol(xn_protocol.attribute("name").value());
                    if(protocol == param_frame.protocol)
                    {   // found protocol
                        xn_protocol_found = true;

                        std::vector<std::string> list_opt_names;
                        std::vector<u8> list_opt_values;
                        pugi::xml_node xn_option = xn_protocol.child("option");
                        for(; xn_option!=NULL; xn_option=xn_option.next_sibling("option"))
                        {
                            std::string const opt_name(xn_option.attribute("name").value());
                            std::string const opt_value(xn_option.attribute("value").value());

                            if(!opt_name.empty())
                            {
                                list_opt_names.push_back(opt_name);
                                list_opt_values.push_back(0);

                                if(opt_value == "true")
                                {
                                    list_opt_values.back() = 1;
                                }
                            }
                        }

                        // set actual protocol used to clean up raw message data
                        sint opt_idx;
                        if(StringContains(protocol,"SAE J1850"))
                        {
                            param_frame.parse_protocol = PROTOCOL_SAE_J1850;
                        }
                        else if(protocol == "ISO 9141-2")   {
                            param_frame.parse_protocol = PROTOCOL_ISO_9141_2;
                        }
                        else if(protocol == "ISO 14230")   {
                            param_frame.parse_protocol = PROTOCOL_ISO_14230;

                            // check for options
                            opt_idx = StringListIndexOf(list_opt_names,"Length Byte");
                            if(opt_idx > -1) {
                                param_frame.iso14230_add_length_byte = bool(list_opt_values[opt_idx]);
                            }
                        }
                        else if(StringContains(protocol,"ISO 15765"))   {
                            param_frame.parse_protocol = PROTOCOL_ISO_15765;

                            if(StringContains(protocol,"Extended Id"))   {
                                param_frame.iso15765_extended_id = true;
                            }

                            // check for options
                            opt_idx = StringListIndexOf(list_opt_names,"Extended Address");
                            if(opt_idx > -1)   {
                                param_frame.iso15765_extended_addr = bool(list_opt_values[opt_idx]);
                            }
                        }
                        else   {
                            throw BuildParamFrameFailed("Unsupported Protocol:"+protocol);
                        }

                        // use address information to build the request header
                        pugi::xml_node xn_address = xn_protocol.child("address");
                        for(; xn_address!=NULL; xn_address=xn_address.next_sibling("address"))
                        {
                            std::string const address(xn_address.attribute("name").value());
                            if(address == param_frame.address)
                            {   // found address
                                xn_address_found = true;

                                // [build headers]
                                if(param_frame.parse_protocol < 0xA00)   {
                                    buildHeader_Legacy(param_frame,xn_address);
                                }
                                else if(param_frame.parse_protocol == PROTOCOL_ISO_14230)   {
                                    buildHeader_ISO_14230(param_frame,xn_address);
                                }
                                else if(param_frame.parse_protocol == PROTOCOL_ISO_15765)   {
                                    buildHeader_ISO_15765(param_frame,xn_address);
                                }
                            }
                        }
                    }
                }
                if(!xn_address_found)   {
                    break;
                }

                pugi::xml_node xn_params = xn_spec.child("parameters");
                for(; xn_params!=NULL; xn_params=xn_params.next_sibling("parameters"))
                {
                    std::string const params_addr(xn_params.attribute("address").value());
                    if(params_addr == param_frame.address)
                    {   // found parameters group
                        xn_params_found = true;

                        // use parameter information to build request data
                        pugi::xml_node xn_parameter = xn_params.child("parameter");
                        for(; xn_parameter!=NULL; xn_parameter=xn_parameter.next_sibling("parameter"))
                        {
                            std::string const name(xn_parameter.attribute("name").value());
                            if(name == param_frame.name)
                            {   // found parameter
                                xn_parameter_found = true;

                                // [build request data]
                                buildData(param_frame,xn_parameter);

                                // [save parse script]
                                // set parse mode
                                std::string parse_mode(xn_parameter.attribute("parse").value());
                                if(parse_mode == "combined")   {
                                    param_frame.parse_mode = PARSE_COMBINED;
                                }
                                else   {
                                    param_frame.parse_mode = PARSE_SEPARATELY;
                                }

                                // save reference to parse function
                                pugi::xml_node xn_script = xn_parameter.child("script");
                                std::string protocols(xn_script.attribute("protocols").value());
                                if(!protocols.empty())   {
                                    bool found_protocol=false;
                                    for(; xn_script!=NULL; xn_script = xn_script.next_sibling("script"))   {
                                        // get the script for the specified protocol
                                        protocols = std::string(xn_script.attribute("protocols").value());
                                        if(StringContains(protocols,param_frame.protocol))   {
                                            found_protocol = true;
                                            break;
                                        }
                                    }
                                    if(!found_protocol)   {
                                        throw BuildParamFrameFailed(
                                                    "Protocol specified not found in parse script");
                                    }
                                }

                                std::string js_function_key =
                                        param_frame.spec+":"+
                                        param_frame.address+":"+
                                        param_frame.name+":"+
                                        protocols;

                                param_frame.function_key_idx =
                                        StringListIndexOf(m_js_list_function_key,js_function_key);

                                if(param_frame.function_key_idx == -1)   {
                                    throw BuildParamFrameFailed(
                                                "No parse function found for message: "+
                                                param_frame.name);
                                }
                            }
                        }
                    }
                }
            }
        }

        if(!xn_spec_found)   {
            throw BuildParamFrameFailed("could not find spec " + param_frame.spec);
        }
        else if(!xn_protocol_found)   {
            throw BuildParamFrameFailed("Error: could not find protocol " + param_frame.protocol);
        }
        else if(!xn_address_found)    {
            throw BuildParamFrameFailed("Error: could not find address " + param_frame.address);
        }
        else if(!xn_params_found)   {
            throw BuildParamFrameFailed("Error: could not find param group");
        }
        else if(!xn_parameter_found)   {
            throw BuildParamFrameFailed("Error: could not find parameter " + param_frame.name);
        }
    }

    // ============================================================= //
    // ============================================================= //

    void Parser::ParseParameterFrame(ParameterFrame &msg_frame,
                                     std::vector<obdex::Data> &list_data)
    {
        if(msg_frame.function_key_idx == -1)   {
            throw ParseParamFrameFailed(
                        "Invalid parse function "
                        "index in message frame");
        }

        bool format_ok=true;

        // clean message data based on protocol type
        Protocol parse_protocol = msg_frame.parse_protocol;

        // clear any data left over from prior use
        for(uint i=0; i < msg_frame.list_message_data.size(); i++)   {
            msg_frame.list_message_data[i].list_headers.clear();
            msg_frame.list_message_data[i].list_data.clear();
        }

        if(parse_protocol < 0xA00)
        {   // SAE_J1850, ISO_9141-2, ISO_14230-4
            for(uint i=0; i < msg_frame.list_message_data.size(); i++)   {
                cleanFrames_Legacy(msg_frame.list_message_data[i]);
            }
        }
        else if(parse_protocol == PROTOCOL_ISO_15765)   {
            int headerLength = (msg_frame.iso15765_extended_id) ? 4 : 2;
            for(uint i=0; i < msg_frame.list_message_data.size(); i++)   {
                cleanFrames_ISO_15765(msg_frame.list_message_data[i],
                                      headerLength);
            }
        }
        else if(parse_protocol == PROTOCOL_ISO_14230)   {
            for(uint i=0; i < msg_frame.list_message_data.size(); i++)   {
                cleanFrames_ISO_14230(msg_frame.list_message_data[i]);
            }
        }
        else   {
            throw ParseParamFrameFailed(
                        "Protocol not yet supported");
        }


        if(!format_ok)   {
            throw ParseParamFrameFailed(
                        "Could not clean raw data "
                        "using spec'd format");
        }

        // parse
        parseResponse(msg_frame,list_data);
    }

    // ============================================================= //
    // ============================================================= //

    std::vector<std::string>
    Parser::GetParameterNames(std::string const &spec_name,
                              std::string const &protocol_name,
                              std::string const &address_name) const
    {
        bool found_address = false;
        std::vector<std::string> param_list;
        pugi::xml_node node_spec = m_xml_doc.child("spec");

        for(; node_spec!=NULL; node_spec = node_spec.next_sibling("spec"))
        {
            std::string filespec_name(node_spec.attribute("name").value());
            if(filespec_name == spec_name)
            {
                pugi::xml_node node_protocol = node_spec.child("protocol");
                for(; node_protocol!=NULL; node_protocol = node_protocol.next_sibling("protocol"))
                {
                    std::string fileprotocol_name(node_protocol.attribute("name").value());
                    if(fileprotocol_name == protocol_name)
                    {
                        pugi::xml_node node_address = node_protocol.child("address");
                        for(; node_address!=NULL; node_address = node_address.next_sibling("address"))
                        {
                            std::string fileaddress_name(node_address.attribute("name").value());
                            if(fileaddress_name == address_name)
                            {
                                found_address = true;
                            }
                        }
                    }
                }

                if(!found_address)
                {   break;   }

                pugi::xml_node node_params = node_spec.child("parameters");
                for(; node_params!=NULL; node_params = node_protocol.next_sibling("parameters"))
                {
                    std::string file_params_name(node_params.attribute("address").value());
                    if(file_params_name == address_name)
                    {
                        pugi::xml_node node_parameter = node_params.child("parameter");
                        for(; node_parameter!=NULL; node_parameter = node_parameter.next_sibling("parameter"))
                        {
                            std::string file_parameter_name(node_parameter.attribute("name").value());
                            param_list.push_back(file_parameter_name);
                        }
                    }
                }
            }
        }

        return param_list;
    }

    // ============================================================= //
    // ============================================================= //

    void Parser::jsInit()
    {
        // create js heap and default context
        m_js_ctx = duk_create_heap_default();
        if(!m_js_ctx)   {
            throw JSContextSetupFailed();
        }

        // push the global object onto the context's stack
        duk_push_global_object(m_js_ctx);
        m_js_idx_global_object = duk_normalize_index(m_js_ctx,-1);

        // register properties to the global object
        duk_eval_string(m_js_ctx,globals_js);
        duk_pop(m_js_ctx);

        // add important properties to the stack and
        // and save their location
        duk_get_prop_string(m_js_ctx,m_js_idx_global_object,
                            "__private_get_lit_data");

        duk_get_prop_string(m_js_ctx,m_js_idx_global_object,
                            "__private_get_num_data");

        duk_get_prop_string(m_js_ctx,m_js_idx_global_object,
                            "__private__clear_all_data");

        duk_get_prop_string(m_js_ctx,m_js_idx_global_object,
                            "__private__add_list_databytes");

        duk_get_prop_string(m_js_ctx,m_js_idx_global_object,
                            "__private__add_msg_data");

        m_js_idx_f_add_msg_data     = duk_normalize_index(m_js_ctx,-1);
        m_js_idx_f_add_databytes    = duk_normalize_index(m_js_ctx,-2);
        m_js_idx_f_clear_data       = duk_normalize_index(m_js_ctx,-3);
        m_js_idx_f_get_num_data     = duk_normalize_index(m_js_ctx,-4);
        m_js_idx_f_get_lit_data     = duk_normalize_index(m_js_ctx,-5);

        // evaluate and save all parse functions
        pugi::xml_node xn_spec = m_xml_doc.child("spec");
        for(; xn_spec!=NULL; xn_spec=xn_spec.next_sibling("spec"))
        {   // for each spec
            std::string spec(xn_spec.attribute("name").value());
            pugi::xml_node xn_params = xn_spec.child("parameters");
            for(; xn_params!=NULL; xn_params=xn_params.next_sibling("parameters"))
            {   // for each set of parameters
                std::string address(xn_params.attribute("address").value());
                pugi::xml_node xn_param = xn_params.child("parameter");
                for(; xn_param!=NULL; xn_param=xn_param.next_sibling("parameter"))
                {   // for each parameter
                    std::string param(xn_param.attribute("name").value());
                    pugi::xml_node xn_script = xn_param.child("script");
                    for(; xn_script!=NULL; xn_script=xn_script.next_sibling("script"))
                    {   // for each script
                        std::string protocols(xn_script.attribute("protocols").value());
                        std::string script(xn_script.child_value());

                        // add parse function name and scope
                        std::string idx = ToString(m_js_list_function_key.size());
                        std::string fname = "f"+idx;
                        PrependString(script,"function "+fname+"() {");
                        script.append("}");

                        // register parse function to global js object
                        duk_eval_string(m_js_ctx,script.c_str());
                        duk_pop(m_js_ctx);

                        // add parse function to top of stack
                        duk_get_prop_string(m_js_ctx,m_js_idx_global_object,
                                            fname.c_str());

                        // save unique key string and stack index for function
                        std::string js_function_key = spec+":"+address+":"+param+":"+protocols;
                        m_js_list_function_key.push_back(js_function_key);
                        m_js_list_function_idx.push_back(duk_normalize_index(m_js_ctx,-1));
                    }
                }
            }
        }
    }

    // ============================================================= //
    // ============================================================= //

    void Parser::buildHeader_Legacy(ParameterFrame &param_frame,
                                    pugi::xml_node xn_address) const
    {
        // store the request header data in
        // a MessageData struct
        MessageData msg;

        // request header bytes
        pugi::xml_node xn_req = xn_address.child("request");
        if(xn_req)   {
            std::string prio(xn_req.attribute("prio").value());
            std::string target(xn_req.attribute("target").value());
            std::string source(xn_req.attribute("source").value());

            // all three bytes must be defined
            if(prio.empty() || target.empty() || source.empty())   {
                throw BuildParamFrameFailed(
                            "ISO 9141-2/SAE J1850, "
                            "Incomplete Request Header");
            }

            bool ok_prio,ok_target,ok_source;
            msg.req_header_bytes.push_back(StringToUInt(prio,ok_prio));
            msg.req_header_bytes.push_back(StringToUInt(target,ok_target));
            msg.req_header_bytes.push_back(StringToUInt(source,ok_source));

            if(!(ok_prio && ok_target && ok_source))
            {
                throw BuildParamFrameFailed(
                            "ISO 9141-2/SAE J1850,"
                            "bad request header");
            }
        }
        else   {
            // for messages without requests,
            // we use an empty header
            obdexlog.Warn() << "ISO 9141-2/SAE J1850,"
                            << "No Request Header";
        }

        // preemptively fill out response header
        // bytes (unspec'd bytes will be ignored
        // by exp_header_mask)
        msg.exp_header_bytes.push_back(0);
        msg.exp_header_bytes.push_back(0);
        msg.exp_header_bytes.push_back(0);

        msg.exp_header_mask.push_back(0);
        msg.exp_header_mask.push_back(0);
        msg.exp_header_mask.push_back(0);

        // response header bytes
        pugi::xml_node xn_resp = xn_address.child("response");
        if(xn_resp)   {
            std::string prio(xn_resp.attribute("prio").value());
            std::string target(xn_resp.attribute("target").value());
            std::string source(xn_resp.attribute("source").value());

            bool ok_prio = true;
            bool ok_target = true;
            bool ok_source = true;

            if(!prio.empty())   {
                msg.exp_header_bytes[0] = StringToUInt(prio,ok_prio);
                msg.exp_header_mask[0] = 0xFF;
            }
            if(!target.empty())   {
                msg.exp_header_bytes[1] = StringToUInt(target,ok_target);
                msg.exp_header_mask[1] = 0xFF;
            }
            if(!source.empty())   {
                msg.exp_header_bytes[2] = StringToUInt(source,ok_source);
                msg.exp_header_mask[2] = 0xFF;
            }

            if(!(ok_prio && ok_target && ok_source))   {
                throw BuildParamFrameFailed(
                            "ISO 9141-2/SAE J1850,"
                            "bad response header");
            }
        }

        // save the MessageData
        param_frame.list_message_data.push_back(msg);
    }

    // ============================================================= //
    // ============================================================= //

    void Parser::buildHeader_ISO_14230(ParameterFrame &param_frame,
                                       pugi::xml_node xn_address) const
    {
        // ISO 14230
        // This protocol has a variable header:
        // A [format]
        // B [format] [target] [source] // ISO 14230-4, OBDII spec
        // C [format] [length]
        // D [format] [target] [source] [length]

        // ref: http://www.dgtech.com/product/dpa/manual/dpa_family_1_261.pdf

        // The format byte determines which header is used.
        // Assume the 8 bits of the format byte look like:
        // A1 A0 L5 L4 L3 L2 L1 L0

        // * If [A1 A0] == [0 0], [target] and [source]
        //   addresses are not included

        // * If [A1 A0] == [1 0], physical addressing is used

        // * If [A1 A0] == [0 1], functional addressing is used

        // * If [L0 to L5] are all 0, data length is specified
        //   in a separate length byte. Otherwise, the length
        //   is specified by [L0 to L5]

        // store the request header data in
        // a MessageData struct
        MessageData msg;

        // request header bytes
        pugi::xml_node xn_req = xn_address.child("request");
        if(xn_req)   {
            std::string format(xn_req.attribute("format").value());
            if(format.empty())   {
                throw BuildParamFrameFailed(
                            "ISO 14230, request header "
                            "is missing format byte");
            }

            bool conv_ok=false;
            ubyte format_byte = StringToUInt(format,conv_ok);
            msg.req_header_bytes.push_back(format_byte);

            if(!conv_ok)   {
                throw BuildParamFrameFailed(
                            "ISO 14230, invalid "
                            "request header format byte");
            }

            // check which type of header
            if((format_byte >> 6) != 0)   {
                // [source] and [target] must be present
                std::string target(xn_req.attribute("target").value());
                std::string source(xn_req.attribute("source").value());

                bool ok_target=false;
                bool ok_source=false;
                msg.req_header_bytes.push_back(StringToUInt(target,ok_target));
                msg.req_header_bytes.push_back(StringToUInt(source,ok_source));

                if(!(ok_target&&ok_source))   {
                    throw BuildParamFrameFailed(
                                "ISO 14230, invalid "
                                "request source/target bytes");
                }
            }
        }
        else   {
            obdexlog.Warn() << "ISO 14230, no request header";
        }

        // preemptively fill out response header
        // bytes (unspec'd bytes will be ignored
        // by exp_header_mask)
        msg.exp_header_bytes.push_back(0);
        msg.exp_header_bytes.push_back(0);
        msg.exp_header_bytes.push_back(0);

        msg.exp_header_mask.push_back(0);
        msg.exp_header_mask.push_back(0);
        msg.exp_header_mask.push_back(0);

        // response header bytes
        pugi::xml_node xn_resp = xn_address.child("response");
        if(xn_resp)   {
            std::string format(xn_resp.attribute("format").value());
            std::string target(xn_resp.attribute("target").value());
            std::string source(xn_resp.attribute("source").value());

            bool ok_format = true;
            bool ok_target = true;
            bool ok_source = true;

            if(!format.empty())   {
                msg.exp_header_bytes[0] = StringToUInt(format,ok_format);
                msg.exp_header_mask[0] = 0xC0;
                // note: the mask for the format_byte is set to
                // 0b11000000 (0xC0) to ignore the length bits
            }
            if(!target.empty())   {
                msg.exp_header_bytes[1] = StringToUInt(target,ok_target);
                msg.exp_header_mask[1] = 0xFF;
            }
            if(!source.empty())   {
                msg.exp_header_bytes[2] = StringToUInt(source,ok_source);
                msg.exp_header_mask[2] = 0xFF;
            }

            if(!(ok_format&&ok_target&&ok_source))   {
                throw BuildParamFrameFailed(
                            "ISO 14230, invalid response header");
            }
        }

        // save the MessageData
        param_frame.list_message_data.push_back(msg);
    }

    // ============================================================= //
    // ============================================================= //

    void Parser::buildHeader_ISO_15765(ParameterFrame &param_frame,
                                       pugi::xml_node xn_address) const
    {
        // store the request header data in
        // a MessageData struct
        MessageData msg;

        // ISO 15765-4 (11-bit standard id)
        // store 11-bit header in two bytes
        if(!param_frame.iso15765_extended_id)   {
            // request header bytes
            pugi::xml_node xn_req = xn_address.child("request");
            if(xn_req)   {
                std::string identifier(xn_req.attribute("identifier").value());
                if(identifier.empty())   {
                    throw BuildParamFrameFailed(
                                "ISO 15765-4 std, "
                                "Incomplete Request Header");
                }
                bool conv_ok = false;
                u32 header_val = StringToUInt(identifier,conv_ok);
                if(!conv_ok)   {
                    throw BuildParamFrameFailed(
                                "ISO 15765 std, "
                                "bad response header");
                }
                ubyte upper_byte = ((header_val & 0xF00) >> 8);
                ubyte lower_byte = (header_val & 0xFF);

                msg.req_header_bytes.push_back(upper_byte);
                msg.req_header_bytes.push_back(lower_byte);
            }
            else   {
                obdexlog.Warn() << "ISO 15765 std, no request header";
            }

            // preemptively fill out response header
            // bytes (unspec'd bytes will be ignored
            // by exp_header_mask)
            msg.exp_header_bytes.push_back(0);
            msg.exp_header_bytes.push_back(0);

            msg.exp_header_mask.push_back(0);
            msg.exp_header_mask.push_back(0);

            // response header bytes
            pugi::xml_node xn_resp = xn_address.child("response");
            if(xn_resp)   {
                std::string identifier(xn_resp.attribute("identifier").value());
                if(identifier.empty())   {
                    throw BuildParamFrameFailed(
                                "ISO 15765-4 std, "
                                "incomplete response header");

                }
                bool conv_ok = false;
                u32 header_val = StringToUInt(identifier,conv_ok);
                if(!conv_ok)   {
                    throw BuildParamFrameFailed(
                                "Error: ISO 15765 std, "
                                "bad request header");
                }
                ubyte upper_byte = ((header_val & 0xF00) >> 8);
                ubyte lower_byte = (header_val & 0xFF);
                msg.exp_header_bytes[0] = upper_byte;
                msg.exp_header_bytes[1] = lower_byte;
            }
        }
        // ISO 15765-4 (29-bit extended id)
        // store 29-bit header in two bytes
        else   {
            // request header bytes
            pugi::xml_node xn_req = xn_address.child("request");
            if(xn_req)   {
                std::string prio(xn_req.attribute("prio").value());
                std::string format(xn_req.attribute("format").value());
                std::string target(xn_req.attribute("target").value());
                std::string source(xn_req.attribute("source").value());

                // all four bytes must be defined
                if(prio.empty() || format.empty() ||
                   target.empty() || source.empty())   {
                    throw BuildParamFrameFailed(
                                "ISO 15765-4 Ext,"
                                "incomplete request header");
                }

                bool ok_prio,ok_format,ok_target,ok_source;
                msg.req_header_bytes.push_back(StringToUInt(prio,ok_prio));
                msg.req_header_bytes.push_back(StringToUInt(format,ok_format));
                msg.req_header_bytes.push_back(StringToUInt(target,ok_target));
                msg.req_header_bytes.push_back(StringToUInt(source,ok_source));

                if(!(ok_prio&&ok_format&&ok_target&&ok_source))   {
                    throw BuildParamFrameFailed(
                                "ISO 15765-4 Ext,"
                                "invalid request header");
                }

            }
            else   {
                obdexlog.Warn() << "ISO 15765 ext, no request header";
            }

            // preemptively fill out response header
            // bytes (unspec'd bytes will be ignored
            // by exp_header_mask)
            msg.exp_header_bytes.push_back(0);
            msg.exp_header_bytes.push_back(0);
            msg.exp_header_bytes.push_back(0);
            msg.exp_header_bytes.push_back(0);

            msg.exp_header_mask.push_back(0);
            msg.exp_header_mask.push_back(0);
            msg.exp_header_mask.push_back(0);
            msg.exp_header_mask.push_back(0);

            // response header bytes
            pugi::xml_node xn_resp = xn_address.child("response");
            if(xn_resp)   {
                std::string prio(xn_req.attribute("prio").value());
                std::string format(xn_req.attribute("format").value());
                std::string target(xn_req.attribute("target").value());
                std::string source(xn_req.attribute("source").value());

                bool ok_prio   = true;
                bool ok_format = true;
                bool ok_target = true;
                bool ok_source = true;

                if(!prio.empty())   {
                    msg.exp_header_bytes[0] = StringToUInt(prio,ok_prio);
                    msg.exp_header_mask[0] = 0xFF;
                }
                if(!format.empty())   {
                    msg.exp_header_bytes[1] = StringToUInt(format,ok_format);
                    msg.exp_header_mask[1] = 0xFF;
                }
                if(!target.empty())   {
                    msg.exp_header_bytes[2] = StringToUInt(target,ok_target);
                    msg.exp_header_mask[2] = 0xFF;
                }
                if(!target.empty())   {
                    msg.exp_header_bytes[3] = StringToUInt(source,ok_source);
                    msg.exp_header_mask[3] = 0xFF;
                }

                if(!(ok_prio&&ok_format&&ok_target&&ok_source))   {
                    throw BuildParamFrameFailed(
                                "Error: ISO 15765-4 ext, "
                                "invalid response header");
                }
            }
        }

        // save the MessageData
        param_frame.list_message_data.push_back(msg);
    }

    // ============================================================= //
    // ============================================================= //

    void Parser::buildData(ParameterFrame &param_frame,
                           pugi::xml_node xn_parameter) const
    {
        // If the parameter is only being used to parse
        // passively, it should not have any request or
        // response attributes
        std::string request,request0;
        bool has_multiple_reqs=false;
        {
            request = std::string(xn_parameter.attribute("request").value());
            request0 = std::string(xn_parameter.attribute("request0").value());
            if(request.empty() && request0.empty())   {
                // assume that no requests will be made
                // for this parameter so we are done
                return;
            }
            else if((!request.empty()) && (!request0.empty()))   {
                BuildParamFrameFailed("mixed single and multiple requests");
            }
            else if(request.empty())   {
                has_multiple_reqs=true;
            }
        }

        // We expect singular request data to be indicated by:
        // request="" [response.prefix=""] [response.mask=""] ...

        // And multiple request data to be indicated by:
        // request0="" [response0.prefix=""] [response0.mask=""] ...
        // request1="" [response1.prefix=""] [response1.mask=""] ...
        // requestN="" [responseN.prefix=""] [responseN.mask=""] ...

        // The request attribute is mandatory, the response
        // attributes are optional

        // For multiple requests, numbering starts at 0

        bool conv_ok=false;
        int n=0;

        while(true)
        {
            if(n > 0)   {
                // each separate request is stored in
                // individual MessageData objects
                MessageData msg;
                param_frame.list_message_data.push_back(msg);
            }
            MessageData &msg = param_frame.list_message_data.back();

            std::string request_delay,response_prefix,response_bytes;

            if(!has_multiple_reqs)  {
                request         = xn_parameter.attribute("request").value();
                request_delay   = xn_parameter.attribute("request.delay").value();
                response_prefix = xn_parameter.attribute("response.prefix").value();
                response_bytes  = xn_parameter.attribute("response.bytes").value();
            }
            else   {
                std::string request_n           = "request"+ToString(n);
                std::string request_delay_n     = request_n+".delay";
                std::string response_n          = "response"+ToString(n);
                std::string response_prefix_n   = response_n+".prefix";
                std::string response_bytes_n    = response_n+".bytes";

                request = xn_parameter.attribute(request_n.c_str()).value();
                request_delay = xn_parameter.attribute(request_delay_n.c_str()).value();
                response_prefix = xn_parameter.attribute(response_prefix_n.c_str()).value();
                response_bytes = xn_parameter.attribute(response_bytes_n.c_str()).value();

                if(request.empty())   {
                    param_frame.list_message_data.pop_back();
                    break;
                }
                n++;
            }

            // request

            // add an initial set of data bytes
            ByteList byte_list;
            msg.list_req_data_bytes.push_back(byte_list);

            std::vector<std::string> sl_request = SplitString(request," ",false);

            for(uint i=0; i < sl_request.size(); i++)   {
                msg.list_req_data_bytes[0].push_back(StringToUInt(sl_request[i],conv_ok));
            }
            if(msg.list_req_data_bytes[0].empty())   {
                throw BuildParamFrameFailed("Invalid request data bytes");
            }

            // request delay
            if(!request_delay.empty())   {
                msg.req_data_delay_ms = StringToUInt(request_delay,conv_ok);
            }

            // response prefix
            if(!response_prefix.empty())   {
                std::vector<std::string> sl_response_prefix = SplitString(response_prefix," ",false);
                for(uint i=0; i < sl_response_prefix.size(); i++)   {
                    msg.exp_data_prefix.push_back(StringToUInt(sl_response_prefix[i],conv_ok));
                }
            }

            // response bytes
            if(!response_bytes.empty())   {
                msg.exp_data_byte_count = StringToUInt(response_bytes,conv_ok);
            }

            if(!has_multiple_reqs)   {
                break;
            }
        }

        // copy over header data from first message in list
        // to other entries; only relevant for multi-response
        MessageData const &first_msg = param_frame.list_message_data[0];
        for(uint i=1; i < param_frame.list_message_data.size(); i++)   {
            MessageData &next_msg = param_frame.list_message_data[i];
            next_msg.req_header_bytes = first_msg.req_header_bytes;
            next_msg.exp_header_bytes = first_msg.exp_header_bytes;
            next_msg.exp_header_mask  = first_msg.exp_header_mask;
        }

        // ISO 15765 may need additional formatting
        if(param_frame.parse_protocol == PROTOCOL_ISO_15765)
        {
            for(uint i=0; i < param_frame.list_message_data.size(); i++)   {
                MessageData &msg = param_frame.list_message_data[i];
                std::vector<ByteList> &list_req_data_bytes = msg.list_req_data_bytes;
                int data_length = list_req_data_bytes[0].size();

                if(param_frame.iso15765_split_req_into_frames && data_length > 7)   {
                    // split the request data into frames
                    ubyte idx_frame = 0;
                    ByteList emptybyte_list;

                    // (first frame)
                    // truncate the current frame after six
                    // bytes and copy to the next frame
                    list_req_data_bytes.push_back(emptybyte_list);
                    while(list_req_data_bytes[idx_frame].size() > 6)   {
                        auto it = std::next(list_req_data_bytes[idx_frame].begin(),6);
                        list_req_data_bytes[idx_frame+1].push_back(*it);
                        list_req_data_bytes[idx_frame].erase(it);
                    }
                    idx_frame++;

                    // (consecutive frames)
                    // truncate the current frame after seven
                    // bytes and copy to the next frame
                    while(list_req_data_bytes[idx_frame].size() > 7)   {
                        list_req_data_bytes.push_back(emptybyte_list);
                        while(list_req_data_bytes[idx_frame].size() > 7)   {
                            auto it = std::next(list_req_data_bytes[idx_frame].begin(),7);
                            list_req_data_bytes[idx_frame+1].push_back(*it);
                            list_req_data_bytes[idx_frame].erase(it);
                        }
                        idx_frame++;
                    }
                }
                if(param_frame.iso15765_add_pci_byte)   {
                    // (single frame)
                    // * higher 4 bits set to 0 for a single frame
                    // * lower 4 bits gives the number of data bytes
                    if(list_req_data_bytes.size() == 1)   {
                        ubyte pci_byte = list_req_data_bytes[0].size();
                        PushFront(list_req_data_bytes[0],pci_byte);
                    }
                    // (multi frame)
                    else   {
                        // (first frame)
                        // higher 4 bits set to 0001
                        // lower 12 bits give number of data bytes
                        ubyte lower_byte = (data_length & 0x0FF);
                        ubyte upper_byte = (data_length & 0xF00) >> 8;
                        upper_byte += 16;    // += 0b1000

                        PushFront(list_req_data_bytes[0],lower_byte);
                        PushFront(list_req_data_bytes[0],upper_byte);

                        // (consecutive frames)
                        // * cycle 0x20-0x2F for each CF starting with 0x21
                        for(uint j=1; j < list_req_data_bytes.size(); j++)   {
                            ubyte pci_byte = 0x20 + (j % 0x10);
                            PushFront(list_req_data_bytes[j],pci_byte);
                        }
                    }
                }
            }
        }

        // ISO 14230 needs additional formatting to
        // add data length info to the header
        if(param_frame.parse_protocol == PROTOCOL_ISO_14230)
        {
            for(uint i=0; i < param_frame.list_message_data.size(); i++)   {
                MessageData &msg = param_frame.list_message_data[i];
                std::vector<ByteList> &list_req_data_bytes = msg.list_req_data_bytes;
                int data_length = list_req_data_bytes[0].size();

                if(data_length > 255)   {
                    throw BuildParamFrameFailed(
                                "ISO 14230, invalid"
                                "data length ( > 255)");
                }

                if(param_frame.iso14230_add_length_byte)   {
                    // add separate length byte
                    msg.req_header_bytes.push_back(ubyte(data_length));
                }
                else   {
                    // encode length in format byte
                    if(data_length > 63)   {
                        throw BuildParamFrameFailed(
                                    "ISO 14230, invalid"
                                    "data length ( > 63)");
                    }
                    msg.req_header_bytes[0] |= ubyte(data_length);
                }
            }
        }
    }

    // ============================================================= //
    // ============================================================= //

    void Parser::parseResponse(ParameterFrame const &msg_frame,
                               std::vector<Data> &list_data)
    {
        if(msg_frame.function_key_idx < 0)   {
            throw ParseParamFrameFailed("Invalid function idx");
        }
        int js_f_idx = msg_frame.function_key_idx;

        if(msg_frame.parse_mode == PARSE_SEPARATELY)
        {
            // * default parse mode -- the parse script is run
            //   once for each entry in MessageData.list_data

            // * the data is accessed using "BYTE(N)" in js where N
            //   is the Nth data byte in a list_data entry

            for(uint i=0; i < msg_frame.list_message_data.size(); i++)
            {
                MessageData const &msg = msg_frame.list_message_data[i];
                for(uint j=0; j < msg.list_headers.size(); j++)
                {
                    ByteList const &header_bytes = msg.list_headers[j];
                    ByteList const &data_bytes = msg.list_data[j];

                    obdex::Data parsed_data;

                    // fill out parameter data
                    parsed_data.param_name    = msg_frame.name;
                    parsed_data.src_name      = msg_frame.address;

                    // clear existing data in js context
                    duk_dup(m_js_ctx,m_js_idx_f_clear_data);
                    duk_call(m_js_ctx,0);
                    duk_pop(m_js_ctx);

                    // copy over data_bytes to js context
                    duk_dup(m_js_ctx,m_js_idx_f_add_databytes);
                    int list_arr_idx = duk_push_array(m_js_ctx);    // list_data_bytes
                    int data_arr_idx = duk_push_array(m_js_ctx);    // data_bytes
                    for(uint k=0; k < data_bytes.size(); k++)   {
                        duk_push_number(m_js_ctx,data_bytes[k]);
                        duk_put_prop_index(m_js_ctx,data_arr_idx,k);
                    }
                    duk_put_prop_index(m_js_ctx,list_arr_idx,0);
                    duk_call(m_js_ctx,1);
                    duk_pop(m_js_ctx);

                    // parse the data
                    duk_dup(m_js_ctx,m_js_list_function_idx[js_f_idx]);
                    duk_call(m_js_ctx,0);
                    duk_pop(m_js_ctx);

                    // save results
                    this->saveNumAndLitData(parsed_data);

                    // save data source address info in LiteralData
                    LiteralData src_address;
                    src_address.property = "Source Address";
                    for(uint k=0; k < header_bytes.size(); k++)   {
                        std::string b_str = ConvUByteToHexStr(header_bytes[k]) + " ";
                        src_address.value_if_true.append(b_str);
                    }
                    src_address.value_if_true = StringToUpper(src_address.value_if_true);
                    src_address.value = true;
                    parsed_data.list_literal_data.push_back(src_address);

                    list_data.push_back(parsed_data);
                }
            }
            return;
        }
        else if(msg_frame.parse_mode == PARSE_COMBINED)
        {
            // * the parse script is run once for every
            //   ParameterFrame.list_message_data

            // * all entries in an individual MessageData
            //   are passed to the parse function together

            // * both the header bytes and data bytes are
            //   passed to the js context

            // * data within a MessageData can be accessed
            //   using "REQ(N).DATA(N).BYTE(N)", where:
            //   - REQ(N) represents the MessageData to access
            //   - DATA(N) represents a single entry in the list
            //     of data bytes available for that MessageData
            //   - BYTE(N) is a single byte in that list of
            //     data bytes

            // clear existing data in js context
            duk_dup(m_js_ctx,m_js_idx_f_clear_data);
            duk_call(m_js_ctx,0);
            duk_pop(m_js_ctx);

            obdex::Data parsed_data;
            parsed_data.param_name    = msg_frame.name;
            parsed_data.src_name      = msg_frame.address;

            for(uint i=0; i < msg_frame.list_message_data.size(); i++)
            {
                MessageData const &msg = msg_frame.list_message_data[i];
                duk_dup(m_js_ctx,m_js_idx_f_add_msg_data);
                int list_arr_idx;

                // build header bytes js array
                list_arr_idx = duk_push_array(m_js_ctx);    // listheader_bytes

                for(uint j=0; j < msg.list_headers.size(); j++)   {
                    ByteList const &header_bytes = msg.list_headers[j];
                    int header_arr_idx = duk_push_array(m_js_ctx);    // header_bytes
                    for(uint k=0; k < header_bytes.size(); k++)   {
                        duk_push_number(m_js_ctx,header_bytes[k]);
                        duk_put_prop_index(m_js_ctx,header_arr_idx,k);
                    }
                    duk_put_prop_index(m_js_ctx,list_arr_idx,j);
                }

                // build data bytes js array
                list_arr_idx = duk_push_array(m_js_ctx);    // list_data_bytes

                for(uint j=0; j < msg.list_data.size(); j++)   {
                    ByteList const &data_bytes = msg.list_data[j];
                    int data_arr_idx = duk_push_array(m_js_ctx);    // data_bytes
                    for(uint k=0; k < data_bytes.size(); k++)   {
                        duk_push_number(m_js_ctx,data_bytes[k]);
                        duk_put_prop_index(m_js_ctx,data_arr_idx,k);
                    }
                    duk_put_prop_index(m_js_ctx,list_arr_idx,j);
                }
                duk_call(m_js_ctx,2);
                duk_pop(m_js_ctx);
            }
            // parse the data
            duk_dup(m_js_ctx,m_js_list_function_idx[js_f_idx]);
            duk_call(m_js_ctx,0);
            duk_pop(m_js_ctx);

            // save results
            this->saveNumAndLitData(parsed_data);
            list_data.push_back(parsed_data);
        }
    }

    // ============================================================= //
    // ============================================================= //

    void Parser::saveNumAndLitData(Data &data)
    {
        // save numerical data
        duk_dup(m_js_ctx,m_js_idx_f_get_num_data);
        duk_call(m_js_ctx,0);                       // <..., listNumData>
        duk_get_prop_string(m_js_ctx,-1,"length");  // <..., listNumData, listNumData.length>
        u32 listNumLength = u32(duk_get_number(m_js_ctx,-1));
        duk_pop(m_js_ctx);                          // <..., listNumData>

        for(u32 i=0; i < listNumLength; i++)   {
            NumericalData numData;
            duk_get_prop_index(m_js_ctx,-1,i);      // <..., listNumData, listNumData[i]>

            duk_get_prop_string(m_js_ctx,-1,"units");
            numData.units = std::string(duk_get_string(m_js_ctx,-1));
            duk_pop(m_js_ctx);

            duk_get_prop_string(m_js_ctx,-1,"min");
            numData.min = duk_get_number(m_js_ctx,-1);
            duk_pop(m_js_ctx);

            duk_get_prop_string(m_js_ctx,-1,"max");
            numData.max = duk_get_number(m_js_ctx,-1);
            duk_pop(m_js_ctx);

            duk_get_prop_string(m_js_ctx,-1,"value");
            numData.value = duk_get_number(m_js_ctx,-1);
            duk_pop(m_js_ctx);

            duk_get_prop_string(m_js_ctx,-1,"property");
            numData.property = std::string(duk_get_string(m_js_ctx,-1));
            duk_pop(m_js_ctx);

            duk_pop(m_js_ctx);                      // <..., listNumData>
            data.list_numerical_data.push_back(numData);
        }
        duk_pop(m_js_ctx);

        // save literal data
        duk_dup(m_js_ctx,m_js_idx_f_get_lit_data);
        duk_call(m_js_ctx,0);                       // <..., listLitData>
        duk_get_prop_string(m_js_ctx,-1,"length");  // <..., listLitData.length>
        u32 listLitLength = u32(duk_get_number(m_js_ctx,-1));
        duk_pop(m_js_ctx);                          // <..., listLitData>

        for(u32 i=0; i < listLitLength; i++)   {
            LiteralData litData;
            duk_get_prop_index(m_js_ctx,-1,i);      // <..., listLitData, listLitData[i]>

            duk_get_prop_string(m_js_ctx,-1,"value");
            int litDataVal = duk_get_boolean(m_js_ctx,-1);
            litData.value = (litDataVal == 1) ? true : false;
            duk_pop(m_js_ctx);

            duk_get_prop_string(m_js_ctx,-1,"valueIfFalse");
            litData.value_if_false = std::string(duk_get_string(m_js_ctx,-1));
            duk_pop(m_js_ctx);

            duk_get_prop_string(m_js_ctx,-1,"valueIfTrue");
            litData.value_if_true = std::string(duk_get_string(m_js_ctx,-1));
            duk_pop(m_js_ctx);

            duk_get_prop_string(m_js_ctx,-1,"property");
            litData.property = std::string(duk_get_string(m_js_ctx,-1));
            duk_pop(m_js_ctx);

            duk_pop(m_js_ctx);                      // <..., listLitData>
            data.list_literal_data.push_back(litData);
        }
        duk_pop(m_js_ctx);
    }

    // ============================================================= //
    // ============================================================= //

    void Parser::cleanFrames_Legacy(MessageData &msg)
    {
        int const header_length=3;
        for(uint j=0; j < msg.list_raw_frames.size(); j++)
        {
            ByteList const &raw_frame = msg.list_raw_frames[j];

            // Split each raw frame into a header and its
            // corresponding data bytes
            // [h0 h1 h2] [d0 d1 d2 d3 d4 d5 d6 ...]
            ByteList header_bytes;
            for(int k=0; k < header_length; k++)   {
                header_bytes.push_back(raw_frame[k]);
            }
            ByteList data_bytes;
            for(uint k=header_length; k < raw_frame.size(); k++)   {
                data_bytes.push_back(raw_frame[k]);
            }

            // check header
            if(!checkBytesAgainstMask(msg.exp_header_bytes,
                                      msg.exp_header_mask,
                                      header_bytes))   {
                throw ParseParamFrameFailed(
                            "SAE J1850/ISO 9141-2/ISO 14230-4, "
                            "header bytes mismatch");
            }

            // check data prefix
            bool data_prefix_ok = true;
            for(uint k=0; k < msg.exp_data_prefix.size(); k++)   {
                ubyte data_byte = data_bytes[0];
                data_bytes.erase(data_bytes.begin());
                if(data_byte != msg.exp_data_prefix[k])   {
                    data_prefix_ok = false;
                    break;
                }
            }
            if(!data_prefix_ok)   {
                throw ParseParamFrameFailed(
                            "SAE J1850/ISO 9141-2/ISO 14230-4, "
                            "data prefix mismatch");
            }

            // save
            msg.list_headers.push_back(header_bytes);
            msg.list_data.push_back(data_bytes);
        }

        if(msg.list_headers.empty())   {
            throw ParseParamFrameFailed(
                        "SAE J1850/ISO 9141-2/ISO 14230-4, "
                        "empty message data");
        }
    }

    // ============================================================= //
    // ============================================================= //

    void Parser::cleanFrames_ISO_14230(MessageData &msg)
    {
        for(uint j=0; j < msg.list_raw_frames.size(); j++)
        {
            ByteList const &raw_frame = msg.list_raw_frames[j];

            // determine header type:
            // A [format]
            // B [format] [target] [source]
            // C [format] [length]
            // D [format] [target] [source] [length]

            // let the format byte be:
            // A1 A0 L5 L4 L3 L2 L1 L0

            // if [A1 A0] == [0 0], target and source
            // bytes are not present (A and C)
            bool no_addressing = ((raw_frame[0] >> 6) == 0);

            // if [L0 to L5] are all 0, data length is specified
            // in a separate length byte (C and D)
            // 0x3F = 0b00111111
            bool has_length_bytes = ((raw_frame[0] & 0x3F) == 0);

            ubyte header_length=4;
            if(no_addressing)   { header_length -= 2; }
            if(!has_length_bytes) { header_length -= 1; }

            ubyte data_length = (has_length_bytes) ?
                raw_frame[header_length-1] : (raw_frame[0] & 0x3F);

            // split each raw frame into a header and its
            // corresponding data bytes
            ByteList header_bytes;
            for(int k=0; k < header_length; k++)   {
                header_bytes.push_back(raw_frame[k]);
            }

            ByteList data_bytes;
            for(int k=header_length; k < (header_length+data_length); k++)   {
                data_bytes.push_back(raw_frame[k]);
            }

            // filter with exp_header_bytes
            // for ISO 14230, by default exp_header_bytes
            // looks like: [format] [target] [source]

            // we add and remove bytes to match up with
            // the actual header before comparing
            ByteList exp_header_bytes,exp_header_mask;

            // all headers have a format byte
            exp_header_bytes.push_back(msg.exp_header_bytes[0]);
            exp_header_mask.push_back(msg.exp_header_mask[0]);

            switch(header_length)   {
                case 2:   { // [format] [length]
                    exp_header_bytes.push_back(0x00);
                    exp_header_mask.push_back(0x00);
                    break;
                }
                case 3:   { // [format] [target] [source]
                    exp_header_bytes.push_back(msg.exp_header_bytes[1]);
                    exp_header_bytes.push_back(msg.exp_header_bytes[2]);

                    exp_header_mask.push_back(msg.exp_header_mask[1]);
                    exp_header_mask.push_back(msg.exp_header_mask[2]);

                    break;
                }
                case 4:   { // [format] [target] [source] [length]
                    exp_header_bytes.push_back(msg.exp_header_bytes[1]);
                    exp_header_bytes.push_back(msg.exp_header_bytes[2]);
                    exp_header_bytes.push_back(0x00);

                    exp_header_mask.push_back(msg.exp_header_mask[1]);
                    exp_header_mask.push_back(msg.exp_header_mask[2]);
                    exp_header_mask.push_back(0x00);
                    break;
                }
                default:   {
                    break;
                }
            }

            // check for expected header bytes
            if(!checkBytesAgainstMask(exp_header_bytes,exp_header_mask,header_bytes))   {
                obdexlog.Warn() << "ISO 14230, header bytes mismatch";
                continue;
            }

            // check/remove data prefix
            if(!checkAndRemoveDataPrefix(msg.exp_data_prefix,data_bytes))   {
                obdexlog.Warn() << "ISO 14230, data prefix mismatch";
                continue;
            }

            // save
            msg.list_headers.push_back(header_bytes);
            msg.list_data.push_back(data_bytes);
        }

        if(msg.list_headers.empty())   {
            throw ParseParamFrameFailed("ISO 14230, empty message data");
        }
    }

    // ============================================================= //
    // ============================================================= //

    void Parser::cleanFrames_ISO_15765(MessageData &msg, int const header_length)
    {
        for(uint j=0; j < msg.list_raw_frames.size(); j++)
        {
            ByteList const &raw_frame = msg.list_raw_frames[j];

            // split raw frame into a header and its
            // corresponding data bytes
            ByteList header_bytes;
            for(int k=0; k < header_length; k++)   {
                header_bytes.push_back(raw_frame[k]);
            }
            ByteList data_bytes;
            for(uint k=header_length; k < raw_frame.size(); k++)   {
                data_bytes.push_back(raw_frame[k]);
            }

            // check header
            if(!checkBytesAgainstMask(msg.exp_header_bytes,
                                      msg.exp_header_mask,
                                      header_bytes))   {
                obdexlog.Warn() << "ISO 15765-4, header bytes mismatch";
                continue;
            }

            // save
            msg.list_headers.push_back(header_bytes);
            msg.list_data.push_back(data_bytes);
        }

        // go through the frames and merge multi-frame messages

        // keep track of CFs that have already been merged
        std::vector<u8> list_merged_frames;
        for(uint j=0; j < msg.list_headers.size(); j++)   {
            list_merged_frames.push_back(false);
        }

        for(uint j=0; j < msg.list_headers.size(); j++)   {
            // ignore already merged frames
            if(list_merged_frames[j])   {
                continue;
            }

            ubyte jPciByte = msg.list_data[j][0];

            // [first frame] pci byte: 1N
            if((jPciByte >> 4) == 1)   {
                ByteList & headerFF = msg.list_headers[j];
                ubyte nextPciByte = 0x21;

                // keep track of the total number of data
                // bytes we expect to see
                int dataLength = ((jPciByte & 0x0F) << 8) +
                                 msg.list_data[j][1];

                int data_bytesSeen = msg.list_data[j].size()-2;

                for(uint k=0; k < msg.list_headers.size(); k++)   {
                    // ignore already merged frames
                    if(list_merged_frames[k])   {
                        continue;
                    }

                    // if the header and target pci byte match
                    if((msg.list_data[k][0] == nextPciByte) &&
                        msg.list_headers[k] == headerFF)   {
                        // this is the next CF frame

                        // remove the pci byte and merge this frame
                        // to the first frame's data bytes
                        msg.list_data[k].erase(msg.list_data[k].begin());

                        msg.list_data[j].insert(
                                    msg.list_data[j].end(),
                                    msg.list_data[k].begin(),
                                    msg.list_data[k].end());

                        // mark as merged
                        list_merged_frames[k] = true;

                        // we can stop once we've merged the
                        // expected number of data bytes
                        data_bytesSeen += msg.list_data[k].size();
                        if(data_bytesSeen >= dataLength)   {
                            break;
                        }

                        // set next target pci byte
                        nextPciByte+=0x01;
                        if(nextPciByte == 0x30)   {
                            nextPciByte = 0x20;
                        }

                        // reset k
                        k=0;
                    }
                }
                // once we get here, all the CF for the FF
                // at msg.list_data[j] should be merged
            }
        }

        // clean up CFs and pci bytes
        for(uint j=msg.list_headers.size(); j-- > 0;)   {
            if(list_merged_frames[j])   {
                msg.list_headers.erase(std::next(msg.list_headers.begin(),j));
                msg.list_data.erase(std::next(msg.list_data.begin(),j));
                list_merged_frames.erase(std::next(list_merged_frames.begin(),j));
                continue;
            }
            ubyte pciByte = msg.list_data[j][0];
            if((pciByte >> 4) == 0)   {         // SF
                msg.list_data[j].erase(msg.list_data[j].begin());
            }
            else if((pciByte >> 4) == 1)   {    // FF
                msg.list_data[j].erase(msg.list_data[j].begin());
                msg.list_data[j].erase(msg.list_data[j].begin());
            }

            // check data prefix
            bool dataPrefixOk = true;
            for(uint k=0; k < msg.exp_data_prefix.size(); k++)   {

                ubyte dataByte = msg.list_data[j][0];
                msg.list_data[j].erase(msg.list_data[j].begin());
                if(dataByte != msg.exp_data_prefix[k])   {
                    dataPrefixOk = false;
                    break;
                }
            }
            if(!dataPrefixOk)   {
                obdexlog.Warn() << "ISO 15765-4, data prefix mismatch";
                msg.list_headers.erase(std::next(msg.list_headers.begin(),j));
                msg.list_data.erase(std::next(msg.list_data.begin(),j));
                list_merged_frames.erase(std::next(list_merged_frames.begin(),j));
                continue;
            }
        }

        if(msg.list_headers.empty())   {
            throw ParseParamFrameFailed("ISO 15765-4, empty message data");
        }
    }

    // ============================================================= //
    // ============================================================= //

    bool Parser::checkBytesAgainstMask(ByteList const &expBytes,
                                       ByteList const &expMask,
                                       ByteList const &bytes)
    {
        if(bytes.size() < expBytes.size())   {
            return false;   // should never get here
        }

        bool bytesOk = true;
        for(uint i=0; i < bytes.size(); i++)   {
            ubyte maskByte = expMask[i];
            if((maskByte & bytes[i]) !=
               (maskByte & expBytes[i]))
            {
                bytesOk = false;
                break;
            }
        }
        return bytesOk;
    }

    // ============================================================= //
    // ============================================================= //

    bool Parser::checkAndRemoveDataPrefix(ByteList const &expDataPrefix,
                                          ByteList &dataBytes)
    {
        bool dataBytesOk = true;
        for(uint i=0; i < expDataPrefix.size(); i++)   {
            auto b = dataBytes[0];
            dataBytes.erase(dataBytes.begin());
            if(expDataPrefix[i] != b)  {
                dataBytesOk=false;
                break;
            }
        }
        return dataBytesOk;
    }

    // ============================================================= //
    // ============================================================= //

    std::string Parser::ConvUByteToHexStr(ubyte byte) const
    {
        return m_lkup_ubyte_hex_str[byte];
    }

    ubyte Parser::ConvHexStrToUByte(std::string const &str) const
    {
        auto it = m_lkup_hex_str_ubyte.find(str);
        if(it == m_lkup_hex_str_ubyte.end())
        {
            throw InvalidHexStr();
        }

        return it->second;
    }

    // ============================================================= //
    // ============================================================= //
}
