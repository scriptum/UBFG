# -------------------------------------------------
# Project created by QtCreator 2011-05-28T21:40:19
# -------------------------------------------------
TARGET = UBFG

QT_VERSION=$$[QT_VERSION]

contains(QT_VERSION, "^5.*") {
  QT += widgets
} else {
}

TEMPLATE = app
SOURCES += main.cpp \
    mainwindow.cpp \
    fontrender.cpp \
    fontview.cpp \
    imagepacker.cpp \
    imagecrop.cpp \
    guillotine.cpp \
    imagesort.cpp \
    maxrects.cpp
HEADERS += mainwindow.h \
    fontrender.h \
    fontview.h \
    imagepacker.h \
    guillotine.h \
    maxrects.h
FORMS += mainwindow.ui
QMAKE_CXXFLAGS_RELEASE -= -O2
QMAKE_CXXFLAGS_RELEASE += -O3 -ffast-math -fomit-frame-pointer
