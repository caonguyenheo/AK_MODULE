#!/bin/sh

export CROSS_PATH=/opt/arm-anykav200-crosstool/usr
export PATH=$PATH:$CROSS_PATH/bin/

make BOARD=cc3200bd clean all

echo "Build OK!"

cp image/sky39ev200.bin ../image/

echo "Copy image to host window!"

