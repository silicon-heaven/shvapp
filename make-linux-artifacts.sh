#!/bin/sh

DISTRO_NAME=shv
DISTRO_VER=master

SRC_DIR=`pwd`
BUILD_DIR=$SRC_DIR/build
DIST_DIR=$BUILD_DIR/dist
DIST_LIB_DIR=$DIST_DIR/lib
DIST_BIN_DIR=$DIST_DIR/bin

QT_DIR=/opt/qt/5.12.0/gcc_64


rm -r $DIST_DIR
mkdir -p $DIST_DIR

rsync -av --exclude '*.debug' $BUILD_DIR/lib/ $DIST_LIB_DIR
rsync -av --exclude '*.debug' $BUILD_DIR/bin/ $DIST_BIN_DIR

#rsync -a --exclude '*.debug'v $QT_DIR/lib/libicu* $DIST_LIB_DIR

rsync -av --exclude '*.debug' $QT_LIB_DIR/libQt5Core.so* $DIST_LIB_DIR
rsync -av --exclude '*.debug' $QT_LIB_DIR/libQt5Gui.so* $DIST_LIB_DIR
rsync -av --exclude '*.debug' $QT_LIB_DIR/libQt5Widgets.so* $DIST_LIB_DIR
#rsync -av --exclude '*.debug' $QT_LIB_DIR/libQt5XmlPatterns.so* $DIST_LIB_DIR
rsync -av --exclude '*.debug' $QT_LIB_DIR/libQt5Network.so* $DIST_LIB_DIR
rsync -av --exclude '*.debug' $QT_LIB_DIR/libQt5Sql.so* $DIST_LIB_DIR
rsync -av --exclude '*.debug' $QT_LIB_DIR/libQt5Xml.so* $DIST_LIB_DIR
#rsync -av --exclude '*.debug' $QT_LIB_DIR/libQt5Qml.so* $DIST_LIB_DIR
#rsync -av --exclude '*.debug' $QT_LIB_DIR/libQt5Quick.so* $DIST_LIB_DIR
rsync -av --exclude '*.debug' $QT_LIB_DIR/libQt5Svg.so* $DIST_LIB_DIR
#rsync -av --exclude '*.debug' $QT_LIB_DIR/libQt5Script.so* $DIST_LIB_DIR
#rsync -av --exclude '*.debug' $QT_LIB_DIR/libQt5ScriptTools.so* $DIST_LIB_DIR
#rsync -av --exclude '*.debug' $QT_LIB_DIR/libQt5PrintSupport.so* $DIST_LIB_DIR
#rsync -av --exclude '*.debug' $QT_LIB_DIR/libQt5SerialPort.so* $DIST_LIB_DIR
#rsync -av --exclude '*.debug' $QT_LIB_DIR/libQt5DBus.so* $DIST_LIB_DIR
#rsync -av --exclude '*.debug' $QT_LIB_DIR/libQt5Multimedia.so* $DIST_LIB_DIR
rsync -av --exclude '*.debug' $QT_LIB_DIR/libQt5XcbQpa.so* $DIST_LIB_DIR

rsync -av --exclude '*.debug' $QT_LIB_DIR/libicu*.so* $DIST_LIB_DIR

rsync -av --exclude '*.debug' $QT_DIR/plugins/platforms/ $DIST_BIN_DIR/platforms
#rsync -av --exclude '*.debug' $QT_DIR/plugins/printsupport/ $DIST_BIN_DIR/printsupport
mkdir -p $DIST_BIN_DIR/imageformats
rsync -av --exclude '*.debug' $QT_DIR/plugins/imageformats/libqjpeg.so $DIST_BIN_DIR/imageformats/
rsync -av --exclude '*.debug' $QT_DIR/plugins/imageformats/libqsvg.so $DIST_BIN_DIR/imageformats/

# mkdir -p $DIST_BIN_DIR/sqldrivers
# rsync -av --exclude '*.debug' $QT_DIR/plugins/sqldrivers/libqsqlite.so $DIST_BIN_DIR/sqldrivers/
# rsync -av --exclude '*.debug' $QT_DIR/plugins/sqldrivers/libqsqlpsql.so $DIST_BIN_DIR/sqldrivers/

# mkdir -p $DIST_BIN_DIR/QtQuick/Window.2
# rsync -av --exclude '*.debug' $QT_DIR/qml/QtQuick/Window.2/ $DIST_BIN_DIR/QtQuick/Window.2
# rsync -av --exclude '*.debug' $QT_DIR/qml/QtQuick.2/ $DIST_BIN_DIR/QtQuick.2

#tar -cvzf $BUILD_DIR/$DISTRO_NAME-$DISTRO_VER.tgz  -C $DIST_DIR ./$DIST_DIR
