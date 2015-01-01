TARGET = sdf-gen

SOURCES += sdf-gen.cpp \
    ../src/sdf.cpp \
    main.cpp
HEADERS += ../src/sdf.h \
    sdf-gen.h

CONFIG += console
TEMPLATE = app

RESOURCES = sdf-gen.qrc
TRANSLATIONS = ru.ts

.SUFFIXES.depends = .ts .qm
.ts.qm.commands = lrelease -qm $@ $<
QMAKE_EXTRA_TARGETS += .SUFFIXES .ts.qm

"$(TARGET)".depends = $$replace(TRANSLATIONS, .ts, .qm)
QMAKE_EXTRA_TARGETS += $(TARGET)

QMAKE_CXXFLAGS += -fopenmp -std=c++0x
QMAKE_LINK += -fopenmp
