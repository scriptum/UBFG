TARGET = tests

SOURCES += test-sdf.cpp \
    sdf.cpp
HEADERS += sdf.h


QMAKE_CXXFLAGS += -fopenmp -std=c++0x
QMAKE_LINK += -fopenmp
