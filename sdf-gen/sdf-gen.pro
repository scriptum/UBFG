TARGET = sdf-gen

SOURCES += sdf-gen.cpp \
    ../src/sdf.cpp \
    main.cpp
HEADERS += ../src/sdf.h \
    sdf-gen.h

CONFIG += console
TEMPLATE = app

QMAKE_CXXFLAGS += -fopenmp -std=c++0x
QMAKE_LINK += -fopenmp
