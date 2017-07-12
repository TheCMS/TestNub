TEMPLATE = app
CONFIG += console c++11
CONFIG -= app_bundle
CONFIG -= qt

#DEFINES += _WINDOWS

DESTDIR = $$_PRO_FILE_PWD_/bin
OBJECTS_DIR = $$_PRO_FILE_PWD_/build

SOURCES += main.cpp \
    	src/PluginsManager.cpp \
		src/TestCompiler.cpp \
		src/TestManager.cpp \
		src/TestLoader.cpp \ 
    src/TestRunner.cpp \
    src/AutoTestHelperFuncs.cpp \
    src/InternalPlugin.cpp

HEADERS += \
    inc/IActionResolver.h \
    inc/IAutoCLIInfo.h \
    inc/IAutoTestPlugin.h \
    inc/IAutoEventsListener.h \
    inc/IAutoGrammar.h \
    src/AutoTestTypes.h \
    src/PluginsManager.h \
    src/TestCompiler.h \
    src/TestManager.h \
    src/TestLoader.h \
    inc/AutoTestLogging.h \
    src/TestRunner.h \
    src/AutoTestHelperFuncs.h \
    src/InternalPlugin.h


LIBS += -ldl -lpthread

INCLUDEPATH += ./inc
INCLUDEPATH += ./src

DISTFILES += \
    res/samplePluginsFile.txt


