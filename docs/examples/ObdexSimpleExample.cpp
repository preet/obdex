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

#include <memory>

// Include obdex headers
#include <obdex/ObdexParser.hpp>
#include <obdex/ObdexLog.hpp> // Not needed, we just use it to print to stdout

void elm327_write(std::string,std::string)
{}

std::string elm327_read()
{
    return "486B10410C2ABC";
}

void print_parsed_data(std::vector<obdex::Data> const &list_data);

void bad_input()
{
    obdex::obdexlog.Error()
            << "\nPass the obd definitions file in as an argument:"
               "\n./obdex_example /path/to/obd2.xml";
}

int main(int argc, char * argv[])
{
    if(argc != 2)
    {
        bad_input();
        return -1;
    }

    // Expect a single argument that specifies
    // the path to the test definitions file
    std::string file_path(argv[1]);
    if(file_path.empty())
    {
        bad_input();
        return -1;
    }

    // Create a Parser object with a path to the definitions file
    obdex::Parser parser(file_path);

    // Create a Parameter frame and fill out the spec,
    // protocol, address and name by referring to the
    // definitions file.
    obdex::ParameterFrame pf;
    pf.spec = "SAEJ1979";
    pf.protocol = "ISO 9141-2";
    pf.address = "Default";
    pf.name = "Engine RPM";

    // Tell obdex to fill in important information by
    // calling BuildParameterFrame -- if required, this
    // will generate the message you need to send to the
    // vehicle to request the parameter.
    parser.BuildParameterFrame(pf);

    // Send the message created by obdex to your vehicle.
    // This step will vary based on the interface you are
    // using. Here is an example for an ELM327 compatible
    // scan tool.

    // Each entry in list_message_data is typically tied
    // to a vehicle request. Here we know "Engine RPM"
    // only has a single request.
    obdex::MessageData &msg = pf.list_message_data[0];

    // ELM327 Adapters expect data in ASCII characters,
    // so we have to convert the request header and data
    // before sending a message out
    std::string elm_req_header;
    std::string elm_req_data;

    // Convert the header for ELM
    for(auto byte : msg.req_header_bytes)
    {
        elm_req_header.append(parser.ConvUByteToHexStr(byte));
    }

    // Convert the data for the ELM
    // The request data is provided in a list because
    // it may have been split into frames for certain
    // protocols. In this case, we know there is only
    // one frame in the request
    for(auto byte : msg.list_req_data_bytes[0])
    {
        elm_req_data.append(parser.ConvUByteToHexStr(byte));
    }

    // Now we have something that looks like:
    // elm_req_header: "686AF1"
    // elm_req_data: "010C"

    // Use the ELM adapter to send this request
    // to the vehicle
    elm327_write(elm_req_header,elm_req_data);

    // Get the response

    // Note:
    // obdex expects the reponse to be input as a
    // series of frames with header and data bytes:
    // [h0 h1 h2 ... d0 d1 d2...]

    // Additional fields (like the check sum, or the
    // DLC for ISO 15765) should be discarded

    // We need to convert the response from the ELM
    // adapter and pass it back to obdex
    std::string resp = elm327_read();
    obdex::ByteList resp_bytes;
    for(uint i=0; i < resp.size(); i+=2)
    {
        resp_bytes.push_back(parser.ConvHexStrToUByte(resp.substr(i,2)));
    }

    // Save the frame in its corresponding message
    msg.list_raw_frames.push_back(resp_bytes);

    // Create a data list to store the results and
    // parse the vehicle response
    std::vector<obdex::Data> list_data;

    parser.ParseParameterFrame(pf,list_data);

    // All responses are parsed into two kinds of data:
    // Numerical, and Literal. There can be any number of
    // each in each obdex::Data struct.
    print_parsed_data(list_data);

    // Done
    return 0;
}

// helper print function
void print_parsed_data(std::vector<obdex::Data> const &list_data)
{
    for(uint i=0; i < list_data.size(); i++)
    {
        if(list_data[i].list_literal_data.size() > 0)
        {   obdex::obdexlog.Trace() << "[Literal Data]";   }

        for(uint j=0; j < list_data[i].list_literal_data.size(); j++)
        {
            if(list_data[i].list_literal_data[j].value)
            {
                obdex::obdexlog.Trace() << list_data[i].list_literal_data[j].property <<
                "  " << list_data[i].list_literal_data[j].value_if_true;
            }
            else
            {
                obdex::obdexlog.Trace() << list_data[i].list_literal_data[j].property <<
                "  " << list_data[i].list_literal_data[j].value_if_false;
            }
        }

        if(list_data[i].list_numerical_data.size() > 0)
        {   obdex::obdexlog.Trace() << "[Numerical Data]";   }

        for(uint j=0; j < list_data[i].list_numerical_data.size(); j++)
        {
            if(!list_data[i].list_numerical_data[j].property.empty())
            {   obdex::obdexlog.Trace() << list_data[i].list_numerical_data[j].property;   }

            obdex::obdexlog.Trace() << list_data[i].list_numerical_data[j].value
                     << list_data[i].list_numerical_data[j].units;
        }
    }

    obdex::obdexlog.Trace() << "================================================";
}
