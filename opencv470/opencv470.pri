DEFINES += OPENCV4_DLL
 
INCLUDEPATH += $$PWD/include
 
win32:CONFIG(release, debug|release): LIBS += -L$$PWD/lib/ -lopencv_world470
else:win32:CONFIG(debug, debug|release): LIBS += -L$$PWD/lib/ -lopencv_world470d
