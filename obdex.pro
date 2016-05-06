 TEMPLATE = app
 TARGET = obdex_test
 CONFIG -= qt

 HEADERS += \
    obdex/duktape/duktape.h \
    obdex/pugixml/pugiconfig.hpp \
    obdex/pugixml/pugixml.hpp \
    obdex/ObdexDataTypes.hpp \
    obdex/ObdexLog.hpp \
    obdex/ObdexUtil.hpp \
    obdex/ObdexTemp.hpp \
    obdex/ObdexErrors.hpp \
    obdex/ObdexParser.hpp

SOURCES += \
    obdex/duktape/duktape.c \
    obdex/pugixml/pugixml.cpp \
    obdex/ObdexLog.cpp \
    obdex/ObdexUtil.cpp \
    obdex/ObdexErrors.cpp \
    obdex/ObdexParser.cpp

# test
HEADERS += \
    obdex/test/ObdexTestHelpers.hpp

SOURCES += \
    obdex/test/ObdexTest.cpp \
    obdex/test/ObdexTestHelpers.cpp \
    obdex/test/ObdexTestUtil.cpp \
    obdex/test/ObdexTestBasic.cpp \
    obdex/test/ObdexTestSpec.cpp

# examples
#SOURCES += \
#    docs/examples/ObdexSimpleExample.cpp


QMAKE_CXXFLAGS += -std=c++11
