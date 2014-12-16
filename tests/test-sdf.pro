TARGET = test-sdf

SOURCES += test-sdf.cpp \
    ../src/sdf.cpp
HEADERS += ../src/sdf.h


QMAKE_CXXFLAGS += -fopenmp -std=c++0x
QMAKE_LINK += -fopenmp
