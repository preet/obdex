### What is obdex
obdex is a small library that is meant to act as a layer between 
on-board diagnostics ("OBD") interface software and higher level 
applications. It helps a developer build and parse messages 
required to communicate with a vehicle. 

An XML+JavaScript definitions file is used to define vehicle parameters, 
and can be easily extended to add support for additional parameters 
unique to different vehicles.

obdex can parse many kinds of OBD messages but it is not necessarily fully compliant with any OBD standards.

It also hasn't been rigorously tested in the real world and there's always the chance you might send/receive erroneous data to/from your vehicle as a result.

**Use this library at your own risk!**
***
### History
obdex is a mostly-identical rewrite of an older library called obdref that gets rid of the dependency on Qt
***
### License
obdex is licensed under the Apache License, version 2.0. For more information see the LICENSE file.
***
### Dependencies
obdex requires a compiler with c++11 support. Additional dependencies are included in the source:

* [**duktape**](http://duktape.org) (MIT License): JavaScript engine
* [**pugixml**](http://pugixml.org) (MIT License): For XML parsing
* [**catch**](https://github.com/philsquared/Catch) (Boost License): Testing framework

***
### Building
QMake is used for development. The included project file builds tests:

    cd obdex
    qmake obdex.pro && make
    
However, qmake isn't required and it should be straightforward to compile the small number of source files with any build tool:

    headers:
    pugixml/pugiconfig.hpp
    pugixml/pugixml.hpp
    duktape/duktape.h
    ObdexLog.hpp
    ObdexUtil.hpp
    ObdexTemp.hpp
    ObdexErrors.hpp
    ObdexParser.hpp
    
    sources:
    pugixml/pugixml.cpp
    duktape/duktape.c
    ObdexLog.cpp
    ObdexUtil.cpp
    ObdexErrors.cpp
    ObdexParser.cpp  
***
### Tests
First build obdex.pro using qmake. To run all tests:

	./obdex_test TestUtils && \
	./obdex_test TestBasic --obdex-definitions-file /path/to/definitions/test.xml && \
	./obdex_test TestSpec \
	--obdex-definitions-file /path/to/definitions/obd2.xml \ // set file to test
	--obdex-spec "SAEJ1979" \ // set spec to test
	--obdex-protocol "ISO 15765 Standard Id" \ // set protocol to test
	--obdex-address "Default" // set address to test

***
### Help
Check the docs folder for documentation and examples. 

