#!/bin/sh

DIST_DIR=_dist

mkdir -p $DIST_DIR
rm -rf $DIST_DIR/*

RSYNC='rsync -av --exclude *.debug'
# $RSYNC expands as: rsync -av '--exclude=*.debug'

echo "$RSYNC lib bin $DIST_DIR"
$RSYNC lib bin $DIST_DIR
pwd
