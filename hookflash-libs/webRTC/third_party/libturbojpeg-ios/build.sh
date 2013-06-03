#!/bin/bash
rm -rf "lib"
mkdir -p "lib"
xcodebuild -project libjpeg-turbo-ios.xcodeproj -configuration Release -target turbojpeg -arch "armv7 armv7s" -sdk iphoneos6.0 build || exit $?
xcodebuild -project libjpeg-turbo-simulator.xcodeproj -configuration Release -target turbojpeg -arch i386 -sdk iphonesimulator6.0 build || exit $?

lipo -output "lib/libturbojpeg-universal.a" -create "lib/libturbojpeg-arm.a" "lib/libturbojpeg-simulator.a"