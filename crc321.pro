# ------------------------------------------------------------------------ #
# Project created manually, based on one by QtCreator 2017-07-08 03:31:56  #
# ------------------------------------------------------------------------ #
QT -= core
QT -= gui
TARGET = crc321
CONFIG += console
CONFIG -= app_bundle
TEMPLATE = app

QMAKE_CXXFLAGS += 
QMAKE_LFLAGS += 
#QMAKE_LFLAGS_RELEASE += -static-libgcc

# it works too
WARNINGS += -Wno-format -Wno-unused
*-g++ {
  QMAKE_LFLAGS += -static-libgcc
  QMAKE_CFLAGS += $$WARNINGS
  QMAKE_CXXFLAGS += $$WARNINGS
}

#INCLUDEPATH += 

win32:LIBS += -L. -lkernel32 -luser32 -lgdi32 -lcomdlg32 -lcomctl32 -ladvapi32 -lshell32
unix:LIBS += -L. 


SOURCES += \
	crc321.c

HEADERS += \
	crc321.h

