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

#ifndef OBDEX_DATA_TYPES_HPP
#define OBDEX_DATA_TYPES_HPP

#include <cstdint>
#include <chrono>
#include <vector>
#include <string>

namespace obdex
{
    // Convenience typedefs
    using uint = unsigned int;
    using u8 = std::uint8_t;
    using u16 = std::uint16_t;
    using u32 = std::uint32_t;
    using u64 = std::uint64_t;

    using sint = int;
    using s8 = std::int8_t;
    using s16 = std::int16_t;
    using s32 = std::int32_t;
    using s64 = std::int64_t;

    // Note: each of the predefined duration types
    // covers a range of at least ±292 years
    using Microseconds = std::chrono::microseconds;
    using Milliseconds = std::chrono::milliseconds;
    using Seconds = std::chrono::seconds;
    using Minutes = std::chrono::minutes;
    using Hours = std::chrono::hours;

    using TimePoint =
        std::chrono::time_point<
            std::chrono::high_resolution_clock>;


    using ubyte = u8;

    using ByteList = std::vector<ubyte>;


    struct LiteralData
    {
        LiteralData() : value(false) {}

        bool value;
        std::string value_if_false;
        std::string value_if_true;
        std::string property;
    };

    struct NumericalData
    {
        NumericalData() : value(0),min(0),max(0) {}

        double value;
        double min;
        double max;
        std::string units;
        std::string property;
    };

    struct Data
    {
        std::string param_name;
        std::string src_name;
        std::vector<LiteralData> list_literal_data;
        std::vector<NumericalData> list_numerical_data;
    };

    // MessageData
    // * generic container for vehicle message data
    // * the message data may represent data tied to
    //   a (single) request or just be any data retreived
    //   from the vehicle some other way (by monitoring
    //   the bus, etc)
    struct MessageData
    {
        // Request Data
        // * used to put together a vehicle request
        //   message (sending a request is optional)

        // request header bytes
        ByteList req_header_bytes;

        // request data bytes (may be split into frames)
        std::vector<ByteList> list_req_data_bytes;

        // delay before sending this request
        u32 req_data_delay_ms;

        // Expected Data
        // * we expect the following to be seen in any
        //   message data received from the vehicle

        // expected header bytes in the response
        ByteList exp_header_bytes;

        // expected header bytes mask
        ByteList exp_header_mask;

        // expected data prefix
        ByteList exp_data_prefix;

        // expected data byte count (after prefix); a value
        // less than 0 means the expected length is unknown
        sint exp_data_byte_count;

        // Raw Data
        // * each entry in the list contains bytes received for
        //   a single data frame in the format [header] [data]
        // * the frames may have originated from different source
        //   addresses and do not need to be organized as such
        std::vector<ByteList> list_raw_frames;

        // Cleaned Data
        // * unlike raw data, cleaned data has no 'frames', but
        //   is organized as one set of data bytes corresponding
        //   to its header:
        //   [header1] [d0 d1 d2 ...]
        //   [header1] [d0 d1 d2 ...]
        //   [header2] [d0 d1 d2 ...]
        std::vector<ByteList> list_headers;
        std::vector<ByteList> list_data;


        MessageData() :
            req_data_delay_ms(0),
            exp_data_byte_count(-1)
        {}
    };

    enum ParseMode
    {
        // PARSE_SEPARATELY (default)
        // * The parse function is called once separately
        //   for each entry in MessageData.listCleanData
        PARSE_SEPARATELY,

        // PARSE_COMBINED
        // * The parse function is called only once for
        //   all of the data in all of the MessageFrames
        // * Individual MessageFrames are accessed with REQ(N)
        // * Individual entries in MessageData.listCleanData
        //   are accessed with DATA(N)
        PARSE_COMBINED
    };

    enum Protocol
    {
        // Protocols that are less than 0xA00 are legacy
        // strict OBDII format, so they can be cleaned by
        // the cleanRawData_Legacy method

        PROTOCOL_SAE_J1850      = 0x001,
        PROTOCOL_ISO_9141_2     = 0x002,
        PROTOCOL_ISO_14230      = 0xA01,
        PROTOCOL_ISO_15765      = 0xA02,    // doesn't currently support
                                            // extended or mixed addressing
    };

    // ParameterFrame
    // * struct that stores all the data for a single
    //   parameter from the definitions file
    class ParameterFrame
    {
    public:
        // Lookup Data
        // * info required to lookup a parameter
        //   from the definitions file
        std::string             spec;
        std::string             protocol;
        std::string             address;
        std::string             name;

        // ISO 15765 Settings
        // * flag to calculate and add the PCI byte
        //   when generating MessageData.req_data_bytes
        bool iso15765_add_pci_byte;

        // * flag to split request data for this message
        //   into single frames (true by default)
        bool iso15765_split_req_into_frames;

        // Options
        // * options that are set indirectly
        //   through the xml definitions file
        ParseMode parse_mode;
        Protocol parse_protocol;
        bool iso14230_add_length_byte;
        bool iso15765_extended_id;
        bool iso15765_extended_addr;

        // Message Data
        // * list of message data for this parameter
        // * each MessageData struct is tied to at most
        //   one request -- multiple MessageData structs
        //   indicate that multiple requests are used to
        //   construct the parameter
        std::vector<MessageData> list_message_data;

        // Parse
        // * contains the javascript function name that will
        //   be called in the definitions file to parse
        //   the MessageData into useful values
        sint function_key_idx;


        ParameterFrame() :
            iso15765_add_pci_byte(true),
            iso15765_split_req_into_frames(true),
            iso14230_add_length_byte(false),
            iso15765_extended_id(false),
            iso15765_extended_addr(false),
            function_key_idx(-1)
        {}
    };

} // obdex

#endif // OBDEX_DATA_TYPES_HPP
