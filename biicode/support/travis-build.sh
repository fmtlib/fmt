#!/bin/bash

if [ $TRAVIS_OS_NAME == linux ]; then
    sudo apt-get install libglu1-mesa-dev xorg-dev
    wget http://www.biicode.com/downloads/latest/ubuntu64
    mv ubuntu64 bii-ubuntu64.deb
    (sudo dpkg -i bii-ubuntu64.deb) && sudo apt-get -f install
    rm bii-ubuntu64.deb
    wget https://s3.amazonaws.com/biibinaries/thirdparty/cmake-3.0.2-Linux-64.tar.gz
    tar -xzf cmake-3.0.2-Linux-64.tar.gz
    sudo cp -fR cmake-3.0.2-Linux-64/* /usr
    rm -rf cmake-3.0.2-Linux-64
    rm cmake-3.0.2-Linux-64.tar.gz
elif [ $TRAVIS_OS_NAME == osx ]; then
    wget http://www.biicode.com/downloads/latest/macos
    mv macos macos.pkg
    sudo installer -pkg macos.pkg -target /
    rm macos.pkg
fi

cmake --version
bii init biicode_project
mkdir -p ./biicode_project/blocks/vitaut/cppformat
shopt -s extglob
mv !(biicode_project|cmake-3.0.2-Darwin64-universal) ./biicode_project/blocks/vitaut/cppformat
cd biicode_project
bii cpp:build
ls bin/*
./bin/vitaut_cppformat_biicode_samples_basic

