#!/usr/bin/sh

ndk_build='ndk-build'
repo_root=`pwd`'/..'
dist=`pwd`'/libs/'


function build_module() {
    module=$1
    cd $repo_root'/'$module'/build/jni/'
    $ndk_build clean
    $ndk_build
    cp ../libs/armeabi-v7a/* $dist
}


# ndk_build='D:/Android/NDK/android-ndk-r15c/ndk-build.cmd'
path=$1
if [ ${#path} -gt 0 ] 
then
    ndk_build=$path
fi

mkdir -p $dist

build_module 'dscv'
build_module 'face'
build_module 'mio'
build_module 'cvclient'
build_module 'cvdaemon'
build_module 'mediac'
build_module 'mediad'
