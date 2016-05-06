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
#include <obdex/ObdexLog.hpp>
#include <obdex/test/ObdexTestHelpers.hpp>

#include <random>

namespace obdex
{
    namespace test
    {
        std::string cli_definitions_file;
        std::string cli_spec;
        std::string cli_protocol;
        std::string cli_address;

        std::string g_test_desc;
        bool g_debug_output=false;


        // ============================================================= //

        namespace
        {
            std::random_device random_dev;
            std::mt19937 random_gen(random_dev());
            std::uniform_int_distribution<uint> dist(0,255); // [0,255]
        }

        // ============================================================= //

        ubyte GenRandomByte()
        {
            return dist(random_gen);
        }

        // ============================================================= //

        std::string bytelist_to_string(ByteList const &byte_list)
        {
            std::string bts;
            for(uint i=0; i < byte_list.size(); i++)
            {
                std::string byte_str = ToHexString(byte_list[i]);
                while(byte_str.size() < 2)
                {
                    PrependString(byte_str,"0");
                }

                StringToUpper(byte_str);
                byte_str.append("  ");
                bts.append(byte_str);
            }

            return bts;
        }

        // ============================================================= //

        void sim_vehicle_message_legacy(ParameterFrame &param,
                                        uint const frames,
                                        bool const random_header)
        {
            if(param.parse_protocol < 0xA00)   {
                // legacy obd message structure
                for(uint i=0; i < param.list_message_data.size(); i++)   {
                    MessageData &msg = param.list_message_data[i];
                    uint exp_data_byte_count = (msg.exp_data_byte_count > 0) ?
                                msg.exp_data_byte_count : 4;

                    // create the header
                    ByteList header = msg.exp_header_bytes;
                    if(random_header)   {
                        for(uint j=0; j < msg.exp_header_bytes.size(); j++)   {
                            msg.exp_header_bytes[j] = 0x00;
                            msg.exp_header_mask[j] = 0x00;
                            header[j] = GenRandomByte();
                        }
                    }
                    // save frames
                    for(uint j=0; j < frames; j++)
                    {   // for each frame
                        ByteList data = msg.exp_data_prefix;
                        for(uint k=0; k < exp_data_byte_count; k++)   {
                            data.push_back(GenRandomByte());
                        }
                        ByteList frame;

                        frame.insert(frame.end(),header.begin(),header.end());
                        frame.insert(frame.end(),data.begin(),data.end());
                        msg.list_raw_frames.push_back(frame);
                    }
                }
            }
        }

        // ============================================================= //

        void sim_vehicle_message_iso14230(ParameterFrame &param,
                                          uint const frames,
                                          bool const randomizeHeader,
                                          uint const iso14230_headerLength)
        {
            if(param.parse_protocol == obdex::PROTOCOL_ISO_14230)
            {
                for(uint i=0; i < param.list_message_data.size(); i++)   {
                    obdex::MessageData &msg = param.list_message_data[i];

                    // data length
                    obdex::ubyte exp_data_byte_count =
                        (msg.exp_data_byte_count > 0) ? msg.exp_data_byte_count : 4;

                    obdex::ubyte dataLength =
                            exp_data_byte_count+msg.exp_data_prefix.size();

                    // create the header
                    // for iso14230 exp_header_bytes is always [F] [T] [S]
                    obdex::ByteList header;

                    if(iso14230_headerLength == 1)   { // [F]
                        header.push_back(0x3F & dataLength);
                    }
                    else if(iso14230_headerLength == 2)   { // [F] [L]
                        header.push_back(msg.exp_header_bytes[0] & 0x3F);
                        header.push_back(dataLength);
                    }
                    else if(iso14230_headerLength == 3)   { // [F] [T] [S]
                        header.push_back(0x3F & dataLength);
                        header.push_back(msg.exp_header_bytes[1]);
                        header.push_back(msg.exp_header_bytes[2]);

                        // encode that [T] and [S] is present in [F] byte
                        header[0] |= 0x80;  // or 0xC0 it doesnt matter for sim
                    }
                    else if(iso14230_headerLength == 4)   { // [F] [T] [S] [L]
                        header.push_back(msg.exp_header_bytes[0]);
                        header.push_back(msg.exp_header_bytes[1]);
                        header.push_back(msg.exp_header_bytes[2]);
                        header.push_back(dataLength);

                        // encode that [T] and [S] is present in [F] byte
                        if((header[0] & 0xC0) == 0)   {
                            header[0] |= 0x80;  // or 0xC0 it doesnt matter for sim
                        }
                    }

                    if(randomizeHeader)   {
                        msg.exp_header_bytes[1] = GenRandomByte();
                        msg.exp_header_mask[1] = 0x00;

                        msg.exp_header_bytes[2] = GenRandomByte();
                        msg.exp_header_mask[2] = 0x00;
                    }
                    // save frames
                    for(uint j=0; j < frames; j++)
                    {   // for each frame
                        obdex::ByteList data = msg.exp_data_prefix;
                        for(int k=0; k < exp_data_byte_count; k++)   {
                            data.push_back(GenRandomByte());
                        }
                        obdex::ByteList frame;
                        frame.insert(frame.end(),header.begin(),header.end());
                        frame.insert(frame.end(),data.begin(),data.end());
                        msg.list_raw_frames.push_back(frame);
                    }
                }
            }
        }

        // ============================================================= //

        void sim_vehicle_message_iso15765(ParameterFrame &param,
                                          uint const frames,
                                          bool const randomizeHeader)
        {
            if(param.parse_protocol == obdex::PROTOCOL_ISO_15765)   {
                for(uint i=0; i < param.list_message_data.size(); i++)   {
                    obdex::MessageData &msg = param.list_message_data[i];
                    obdex::ByteList header = msg.exp_header_bytes;
                    if(randomizeHeader)   {
                        for(uint j=0; j < header.size(); j++)   {
                            header[j] = GenRandomByte();
                            msg.exp_header_mask[j] = 0x00;
                        }
                    }

                    if(frames == 1)   {   // single frame
                        obdex::ByteList data;
                        data.push_back(0x07);   // pci byte
                        data.insert(data.end(),msg.exp_data_prefix.begin(),msg.exp_data_prefix.end());
                        for(int j=0; j < int(8-(msg.exp_data_prefix.size()+1)); j++)   {
                            data.push_back(GenRandomByte());
                        }

                        obdex::ByteList frame;
                        frame.insert(frame.end(),header.begin(),header.end());
                        frame.insert(frame.end(),data.begin(),data.end());
                        msg.list_raw_frames.push_back(frame);
                    }
                    else if(frames > 1)   {
                        // multi frame
                        int dataLength=(frames*7)-1;
                        obdex::ubyte dataLengthLow = dataLength & 0xFF;
                        obdex::ubyte dataLengthHigh = (dataLength >> 8) && 0xFF;

                        if((dataLengthHigh & 0xF0) != 1)   {
                            dataLengthHigh = dataLengthHigh & 0x0F;
                            dataLengthHigh = dataLengthHigh | 0x10;
                        }
                        obdex::ByteList catData; // concatenated data
                        catData.insert(catData.end(),msg.exp_data_prefix.begin(),msg.exp_data_prefix.end());
                        for(int j=0; j < int(dataLength-msg.exp_data_prefix.size()); j++)   {
                            catData.push_back(GenRandomByte());
                        }

                        for(uint f=0; f < frames; f++)   {
                            if(f==0)   {
                                // first frame
                                obdex::ByteList data;
                                data.push_back(dataLengthHigh);
                                data.push_back(dataLengthLow);
                                for(obdex::ubyte i=0; i < 6; i++)   {
                                    data.push_back(catData[0]);
                                    catData.erase(catData.begin());
                                }
                                obdex::ByteList frame;
                                frame.insert(frame.end(),header.begin(),header.end());
                                frame.insert(frame.end(),data.begin(),data.end());
                                msg.list_raw_frames.push_back(frame);
                            }
                            else   {
                                // consecutive frame
                                obdex::ubyte cfNumber = f%0x10;
                                obdex::ubyte cfPciByte = 0x20 | cfNumber;
                                obdex::ByteList data;
                                data.push_back(cfPciByte);
                                for(obdex::ubyte i=0; i < 7; i++)   {
                                    data.push_back(catData[0]);
                                    catData.erase(catData.begin());
                                }
                                obdex::ByteList frame;
                                frame.insert(frame.end(),header.begin(),header.end());
                                frame.insert(frame.end(),data.begin(),data.end());
                                msg.list_raw_frames.push_back(frame);
                            }
                        }
                    }
                }
            }
        }

        // ============================================================= //

        void print_message_data(obdex::MessageData const &msg_data)
        {
            obdexlog.Trace() << "------------------------------------------------";
            obdexlog.Trace() << "[Request Header Bytes]: " << bytelist_to_string(msg_data.req_header_bytes);
            obdexlog.Trace() << "[List Request Data Bytes]";
            for(uint j=0; j < msg_data.list_req_data_bytes.size(); j++)   {
                obdexlog.Trace() << "  req:" << j << ": " << bytelist_to_string(msg_data.list_req_data_bytes[j]);
            }
            obdexlog.Trace() << "[Request Delay]: " << msg_data.req_data_delay_ms;
            obdexlog.Trace() << "[Expected Header Bytes]: " << bytelist_to_string(msg_data.exp_header_bytes);
            obdexlog.Trace() << "[Expected Header Mask]: " << bytelist_to_string(msg_data.exp_header_mask);
            obdexlog.Trace() << "[Expected Data Prefix]: " << bytelist_to_string(msg_data.exp_data_prefix);
            obdexlog.Trace() << "[Expected Number of Data Bytes]: " << msg_data.exp_data_byte_count;
            obdexlog.Trace() << "------------------------------------------------";
            obdexlog.Trace() << "list_raw_frames:";
            for(uint i=0; i < msg_data.list_raw_frames.size(); i++)   {
                obdexlog.Trace() << i << ": " << bytelist_to_string(msg_data.list_raw_frames[i]);
            }
            obdexlog.Trace() << "------------------------------------------------";
            obdexlog.Trace() << "listCleaned:";
            for(uint i=0; i < msg_data.list_headers.size(); i++)   {
                obdexlog.Trace() << i << ":"
                         << bytelist_to_string(msg_data.list_headers[i]) << ", "
                         << bytelist_to_string(msg_data.list_data[i]);
            }
            obdexlog.Trace() << "------------------------------------------------";
        }

        // ============================================================= //

        void print_message_frame(obdex::ParameterFrame const &msg_frame)
        {
            obdexlog.Trace() << "================================================";
            obdexlog.Trace() << "[Specification]: " << msg_frame.spec;
            obdexlog.Trace() << "[Protocol]: " << msg_frame.protocol;
            obdexlog.Trace() << "[Address]: " << msg_frame.address;
            obdexlog.Trace() << "[Name]: " << msg_frame.name;

            if(StringContains(msg_frame.protocol,"15765"))   {
                obdexlog.Trace() << "ISO 15765: Add PCI Bytes?"
                         << msg_frame.iso15765_add_pci_byte;

                obdexlog.Trace() << "ISO 15765: Split into frames?"
                         << msg_frame.iso15765_split_req_into_frames;
            }

            for(uint i=0; i < msg_frame.list_message_data.size(); i++)   {
                obdexlog.Trace() << "-" << i << "-";
                print_message_data(msg_frame.list_message_data[i]);
            }
        }

        // ============================================================= //

        void print_parsed_data(std::vector<obdex::Data> &list_data)
        {
            for(uint i=0; i < list_data.size(); i++)
            {
                if(list_data[i].list_literal_data.size() > 0)
                {   obdexlog.Trace() << "[Literal Data]";   }

                for(uint j=0; j < list_data[i].list_literal_data.size(); j++)
                {
                    if(list_data[i].list_literal_data[j].value)
                    {
                        obdexlog.Trace() << list_data[i].list_literal_data[j].property <<
                        "  " << list_data[i].list_literal_data[j].value_if_true;
                    }
                    else
                    {
                        obdexlog.Trace() << list_data[i].list_literal_data[j].property <<
                        "  " << list_data[i].list_literal_data[j].value_if_false;
                    }
                }

                if(list_data[i].list_numerical_data.size() > 0)
                {   obdexlog.Trace() << "[Numerical Data]";   }

                for(uint j=0; j < list_data[i].list_numerical_data.size(); j++)
                {
                    if(!list_data[i].list_numerical_data[j].property.empty())
                    {   obdexlog.Trace() << list_data[i].list_numerical_data[j].property;   }

                    obdexlog.Trace() << list_data[i].list_numerical_data[j].value
                             << list_data[i].list_numerical_data[j].units;
                }
            }

            obdexlog.Trace() << "================================================";
        }


        // ============================================================= //
    }
}
