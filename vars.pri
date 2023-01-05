TARGET = tp_obj
TEMPLATE = lib

DEFINES += TP_OBJ_LIBRARY

SOURCES += src/Globals.cpp
HEADERS += inc/tp_obj/Globals.h

SOURCES += src/WriteOBJ.cpp
HEADERS += inc/tp_obj/WriteOBJ.h

SOURCES += src/ReadOBJ.cpp
HEADERS += inc/tp_obj/ReadOBJ.h

SOURCES += src/OBJParser.cpp
HEADERS += inc/tp_obj/OBJParser.h
