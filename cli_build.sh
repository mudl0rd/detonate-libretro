#!/bin/bash
currentDir=$(
  cd $(dirname "$0")
  pwd
)
install_direct="$currentDir/compile_dir"
if [ -d "$currentDir/builddir" ] 
then
    rm -r "$currentDir/builddir"
    mkdir "$currentDir/builddir"
else
    mkdir "$currentDir/builddir"
fi;

if [ -d "$install_direct" ] 
then
    mkdir "$install_direct"
fi;

CC=gcc CXX=g++ CC_LD=g++ meson setup builddir
meson builddir
ninja -C builddir -f build.ninja
cd builddir && meson install

