#!/bin/bash
# Removes Qt plugins the app never uses. Some of them (PDF, virtual keyboard, 3D, the other Controls styles,
# glib networkinformation) depend on Qt frameworks that macdeployqt does not bundle, which made the app load
# a SECOND copy of QtCore / QtGui from Homebrew. Usage: prune_bundle.sh "<path to .app>"
set -euo pipefail
APP="$1"; P="$APP/Contents/PlugIns"
rm -f  "$P/imageformats/libqpdf.dylib" \
       "$P/platforminputcontexts/libqtvirtualkeyboardplugin.dylib" \
       "$P/networkinformation/libqglib.dylib" \
       "$P/quick/libpdfquickplugin.dylib" \
       "$P/quick/libqmldesignsupportplugin.dylib" \
       "$P/quick/libqtquickscene2dplugin.dylib" \
       "$P/quick/libqtquickscene3dplugin.dylib" \
       "$P/quick/libparticlesplugin.dylib" \
       "$P/quick/libqtquicktimelineblendtreesplugin.dylib" \
       "$P/quick/libqtqmlstatemachineplugin.dylib" \
       "$P/quick/libqmllocalstorageplugin.dylib" \
       "$P/quick/libqmlxmllistmodelplugin.dylib" \
       "$P/quick/libqmlfolderlistmodelplugin.dylib" \
       "$P/quick/libqtquickshapesdesignhelpersplugin.dylib"
# Controls styles other than Basic (and the native style plugins that pull in the macOS style)
rm -f "$P"/quick/libqtquickcontrols2{fluentwinui3,fusion,imagine,ios,macos,material,universal}style*plugin.dylib \
      "$P"/quick/libqtquickcontrols2nativestyleplugin.dylib "$P"/quick/libqtquicknativestyleplugin.dylib
echo "pruned: $(find "$P" -name '*.dylib' | wc -l | tr -d ' ') plugins left"
