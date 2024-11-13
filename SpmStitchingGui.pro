QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++17

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

include($$PWD/opencv470/opencv470.pri)

INCLUDEPATH += \
    mainwindow/ \
    multi_select_file_dialog/include/ \
    spm_process/include/

SOURCES += \
    mainwindow/mainwindow.cpp \
    multi_select_file_dialog/src/multi_select_file_dialog.cpp \
    spm_process/src/spm_reader.cpp \
    main.cpp

HEADERS += \
    mainwindow/mainwindow.h \
    multi_select_file_dialog/include/multi_select_file_dialog.h \
    spm_process/include/spm_stitching.hpp \

FORMS += \
    mainwindow/mainwindow.ui

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

RESOURCES += \
    resource.qrc

RC_FILE += app.rc
