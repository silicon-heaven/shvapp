#!/bin/bash

QT_DIR=/home/fanda/programs/qt5/5.14.1
DEST_DIR=/opt/qt
DEST_HOST=localhost
ARCH=gcc_64

help() {
	echo "Usage: copy-qt.sh [ options ... ]"
	echo "required options: qt-dir, dest-host"
	echo -e "\n"
	echo "avaible options"
	echo "    --qt-dir <path>          QT dir, ie: /home/me/qt5/5.13.1/gcc_64"
	echo "    --dest-host <IP>       IP of mechine where Qt will be copied"
	echo "    --dest-dir <path>   QT dest dir, default value $DEST_DIR"
	echo -e "\n"
	echo "example: copy-qt.sh --qt-dir /home/me/qt5/5.13.1 --dest-host 192.168.11.12"
	exit 0
}

error() {
	echo -e "\e[31m${1}\e[0m"
}

while [[ $# -gt 0 ]]
do
key="$1"
# echo key: $key
case $key in
	--qt-dir)
	QT_DIR="$2"
	shift # past argument
	shift # past value
	;;
	--dest-dir)
	DEST_DIR="$2"
	shift # past argument
	shift # past value
	;;
	--dest-host)
	DEST_HOST="$2"
	shift # past argument
	shift # past value
	;;
	-h|--help)
	shift # past value
	help
	;;
	*)    # unknown option
	echo "ignoring argument $1"
	shift # past argument
	;;
esac
done

SRC_DIR=$QT_DIR/$ARCH
QT_VER=`basename $QT_DIR`
DEST_DIR_ARCH=$DEST_DIR/$QT_VER/$ARCH
# DEST=$DEST_HOST:$DEST_DIR_ARCH

if [ ! -d $SRC_DIR ]; then
	error "invalid QT dir, use --qt-dir <path> to specify it\n"
	help
fi

# if [ -z $APP_VER ]; then
# 	APP_VER=`grep APP_VERSION $SRC_DIR/quickevent/app/quickevent/src/appversion.h | cut -d\" -f2`
# 	echo "Distro version not specified, deduced from source code: $APP_VER" >&2
# 	#exit 1
# fi

echo SRC_DIR: $SRC_DIR
echo QT_VER: $QT_VER
echo DEST: $DEST_HOST:$DEST_DIR_ARCH

QT_LIB_DIR=$SRC_DIR/lib
QT_PLUGINS_DIR=$SRC_DIR/plugins
DEST_LIB_DIR=$DEST_DIR_ARCH/lib
DEST_FULL_LIB_DIR=$DEST_HOST:$DEST_LIB_DIR
DEST_PLUGINS_DIR=$DEST_DIR_ARCH/plugins
DEST_FULL_PLUGINS_DIR=$DEST_HOST:$DEST_PLUGINS_DIR

# RSYNC_OPTS="-avz --exclude '*.debug' --progress"

# rsync -avz --exclude '*.debug' --progress $BUILD_DIR/lib/ $DEST_FULL_LIB_DIR
# rsync -avz --exclude '*.debug' --progress $BUILD_DIR/bin/ $DIST_BIN_DIR

ssh $DEST_HOST mkdir -p $DEST_DIR_ARCH
# echo "-- ssh $DEST_HOST mkdir -p $DEST ret: $?"
# if [ $? -ne 0 ] ; then
#   error "cannot create dir $DEST_DIR_ARCH"
# fi

rsync -avz --exclude '*.debug' --progress $QT_LIB_DIR/libQt5Core.so* $DEST_FULL_LIB_DIR
# rsync -avz --exclude '*.debug' --progress $QT_LIB_DIR/libQt5Gui.so* $DEST_FULL_LIB_DIR
# rsync -avz --exclude '*.debug' --progress $QT_LIB_DIR/libQt5Widgets.so* $DEST_FULL_LIB_DIR
# rsync -avz --exclude '*.debug' --progress $QT_LIB_DIR/libQt5XmlPatterns.so* $DEST_FULL_LIB_DIR
rsync -avz --exclude '*.debug' --progress $QT_LIB_DIR/libQt5Network.so* $DEST_FULL_LIB_DIR
rsync -avz --exclude '*.debug' --progress $QT_LIB_DIR/libQt5WebSockets.so* $DEST_FULL_LIB_DIR
rsync -avz --exclude '*.debug' --progress $QT_LIB_DIR/libQt5Sql.so* $DEST_FULL_LIB_DIR
rsync -avz --exclude '*.debug' --progress $QT_LIB_DIR/libQt5Xml.so* $DEST_FULL_LIB_DIR
# rsync -avz --exclude '*.debug' --progress $QT_LIB_DIR/libQt5Qml.so* $DEST_FULL_LIB_DIR
# rsync -avz --exclude '*.debug' --progress $QT_LIB_DIR/libQt5Quick.so* $DEST_FULL_LIB_DIR
# rsync -avz --exclude '*.debug' --progress $QT_LIB_DIR/libQt5Svg.so* $DEST_FULL_LIB_DIR
# rsync -avz --exclude '*.debug' --progress $QT_LIB_DIR/libQt5Script.so* $DEST_FULL_LIB_DIR
# rsync -avz --exclude '*.debug' --progress $QT_LIB_DIR/libQt5ScriptTools.so* $DEST_FULL_LIB_DIR
# rsync -avz --exclude '*.debug' --progress $QT_LIB_DIR/libQt5PrintSupport.so* $DEST_FULL_LIB_DIR
rsync -avz --exclude '*.debug' --progress $QT_LIB_DIR/libQt5SerialPort.so* $DEST_FULL_LIB_DIR
rsync -avz --exclude '*.debug' --progress $QT_LIB_DIR/libQt5DBus.so* $DEST_FULL_LIB_DIR
# rsync -avz --exclude '*.debug' --progress $QT_LIB_DIR/libQt5Multimedia.so* $DEST_FULL_LIB_DIR
# rsync -avz --exclude '*.debug' --progress $QT_LIB_DIR/libQt5XcbQpa.so* $DEST_FULL_LIB_DIR

rsync -avz --exclude '*.debug' --progress $QT_LIB_DIR/libQt5OpcUa.so* $DEST_FULL_LIB_DIR

rsync -avz --exclude '*.debug' --progress $QT_LIB_DIR/libicu*.so* $DEST_FULL_LIB_DIR

# rsync -avz --exclude '*.debug' --progress $QT_DIR/plugins/platforms/ $DIST_BIN_DIR/platforms
# rsync -avz --exclude '*.debug' --progress $QT_DIR/plugins/printsupport/ $DIST_BIN_DIR/printsupport

# mkdir -p $DIST_BIN_DIR/imageformats
# rsync -avz --exclude '*.debug' --progress $QT_DIR/plugins/imageformats/libqjpeg.so $DIST_BIN_DIR/imageformats/
# rsync -avz --exclude '*.debug' --progress $QT_DIR/plugins/imageformats/libqsvg.so $DIST_BIN_DIR/imageformats/

# echo "-- ssh $DEST_HOST mkdir -p $DEST/plugins/sqldrivers"
ssh $DEST_HOST mkdir -p $DEST_PLUGINS_DIR/sqldrivers
# echo "-- rsync -avz --exclude '*.debug' --progress $QT_PLUGINS_DIR/sqldrivers/libqsqlite.so $DEST/plugins/sqldrivers/"
rsync -avz --exclude '*.debug' --progress $QT_PLUGINS_DIR/sqldrivers/libqsqlite.so $DEST_FULL_PLUGINS_DIR/sqldrivers/
rsync -avz --exclude '*.debug' --progress $QT_PLUGINS_DIR/sqldrivers/libqsqlpsql.so $DEST_FULL_PLUGINS_DIR/sqldrivers/

rsync -avz --exclude '*.debug' --progress $QT_PLUGINS_DIR/opcua $DEST_FULL_PLUGINS_DIR/

# mkdir -p $DIST_BIN_DIR/audio
# rsync -avz --exclude '*.debug' --progress $QT_DIR/plugins/audio/ $DIST_BIN_DIR/audio/

# mkdir -p $DIST_BIN_DIR/QtQuick/Window.2
# rsync -avz --exclude '*.debug' --progress $QT_DIR/qml/QtQuick/Window.2/ $DIST_BIN_DIR/QtQuick/Window.2
# rsync -avz --exclude '*.debug' --progress $QT_DIR/qml/QtQuick.2/ $DIST_BIN_DIR/QtQuick.2

SHV_BIN_DIR=/home/fanda/proj/_build/shv-release
SHV_SRC_DIR=/home/fanda/proj/shv
rsync -avz --progress $SHV_BIN_DIR/lib $SHV_BIN_DIR/bin $DEST_HOST:/opt/shv/

SHVGATE_SRC_DIR=/home/fanda/proj/shvgate
SHVGATE_BIN_DIR=/home/fanda/proj/_build/shvgate-release
rsync -avz --progress $SHVGATE_BIN_DIR/bin $DEST_HOST:/opt/shv/

exit 0

rsync -avz --exclude '*.db' --progress $SHV_SRC_DIR/shvbroker/etc/shv/shvbroker/ $DEST_HOST:/etc/shv/shvbroker/

SHVGATE_PROFILE=vystavka
rsync -avz --progress $SHVGATE_SRC_DIR/etc/shv/shvgate/pki $DEST_HOST:/etc/shv/shvgate/
rsync -avz --progress $SHVGATE_SRC_DIR/etc/shv/shvgate/nodes-$SHVGATE_PROFILE.cpon $DEST_HOST:/etc/shv/shvgate/
rsync -avz --progress $SHVGATE_SRC_DIR/etc/shv/shvgate/shvgate-$SHVGATE_PROFILE.conf $DEST_HOST:/etc/shv/shvgate/
rsync -avz --progress $SHVGATE_SRC_DIR/etc/shv/shvgate/plc-opcua-g2.cpon $DEST_HOST:/etc/shv/shvgate/

