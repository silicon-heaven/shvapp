#!/bin/sh

APP_NAME=shvspy
APP_VER=1.0.0

# QT_DIR=/home/fanda/programs/qt5/5.14.1/gcc_64
QT_DIR=/opt/qt/5.14.1/gcc_64
QT_LIB_DIR=$QT_DIR/lib

# BUILD_DIR=.

DIST_DIR=_dist
DIST_LIB_DIR=$DIST_DIR/lib
DIST_BIN_DIR=$DIST_DIR/bin

mkdir -p $DIST_DIR
rm -rf $DIST_DIR/*

RSYNC='rsync -av --exclude *.debug'
# $RSYNC expands as: rsync -av '--exclude=*.debug'

echo "$RSYNC lib bin $DIST_DIR"
$RSYNC lib bin $DIST_DIR
# exit 0
$RSYNC $QT_LIB_DIR/libQt5Core.so* $DIST_LIB_DIR
$RSYNC $QT_LIB_DIR/libQt5Gui.so* $DIST_LIB_DIR
$RSYNC $QT_LIB_DIR/libQt5Widgets.so* $DIST_LIB_DIR
#$RSYNC $QT_LIB_DIR/libQt5XmlPatterns.so* $DIST_LIB_DIR
$RSYNC $QT_LIB_DIR/libQt5Network.so* $DIST_LIB_DIR
$RSYNC $QT_LIB_DIR/libQt5Sql.so* $DIST_LIB_DIR
$RSYNC $QT_LIB_DIR/libQt5Xml.so* $DIST_LIB_DIR
#$RSYNC $QT_LIB_DIR/libQt5Qml.so* $DIST_LIB_DIR
#$RSYNC $QT_LIB_DIR/libQt5Quick.so* $DIST_LIB_DIR
$RSYNC $QT_LIB_DIR/libQt5Svg.so* $DIST_LIB_DIR
#$RSYNC $QT_LIB_DIR/libQt5Script.so* $DIST_LIB_DIR
#$RSYNC $QT_LIB_DIR/libQt5ScriptTools.so* $DIST_LIB_DIR
#$RSYNC $QT_LIB_DIR/libQt5PrintSupport.so* $DIST_LIB_DIR
#$RSYNC $QT_LIB_DIR/libQt5SerialPort.so* $DIST_LIB_DIR
#$RSYNC $QT_LIB_DIR/libQt5DBus.so* $DIST_LIB_DIR
#$RSYNC $QT_LIB_DIR/libQt5Multimedia.so* $DIST_LIB_DIR
$RSYNC $QT_LIB_DIR/libQt5XcbQpa.so* $DIST_LIB_DIR

$RSYNC $QT_LIB_DIR/libicu*.so* $DIST_LIB_DIR

$RSYNC $QT_DIR/plugins/platforms/ $DIST_BIN_DIR/platforms
#$RSYNC $QT_DIR/plugins/printsupport/ $DIST_BIN_DIR/printsupport

#mkdir -p $DIST_BIN_DIR/imageformats
$RSYNC -R  $QT_DIR/plugins/./imageformats/libqjpeg.so $DIST_BIN_DIR/
$RSYNC -R  $QT_DIR/plugins/./imageformats/libqsvg.so $DIST_BIN_DIR/

# mkdir -p $DIST_BIN_DIR/sqldrivers
# $RSYNC $QT_DIR/plugins/sqldrivers/libqsqlite.so $DIST_BIN_DIR/sqldrivers/
# $RSYNC $QT_DIR/plugins/sqldrivers/libqsqlpsql.so $DIST_BIN_DIR/sqldrivers/

# mkdir -p $DIST_BIN_DIR/QtQuick/Window.2
# $RSYNC $QT_DIR/qml/QtQuick/Window.2/ $DIST_BIN_DIR/QtQuick/Window.2
# $RSYNC $QT_DIR/qml/QtQuick.2/ $DIST_BIN_DIR/QtQuick.2

cd $DIST_DIR
mkdir -p install
tar -cvzf install/$APP_NAME-$APP_VER-gcc_64.tgz  bin lib
