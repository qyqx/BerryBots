#!/bin/bash

SFML_LIBNAMES="libsfml-graphics libsfml-system libsfml-window"
SFML_VERSION="2.0"

rm -rf $2
if [ ! -e $2 ]
  then
  echo "Copying SFML libs..."
  mkdir $2
  for TARGET in ${SFML_LIBNAMES} ; do
    cp $1/${TARGET}.${SFML_VERSION}.dylib $2
    cd $2
    ln -s ${TARGET}.${SFML_VERSION}.dylib ${TARGET}.2.dylib
    ln -s ${TARGET}.${SFML_VERSION}.dylib ${TARGET}.dylib
    cd -
  done

  for TARGET in ${SFML_LIBNAMES} ; do
    LIBFILE=$2/${TARGET}.${SFML_VERSION}.dylib
    TARGETID=`otool -DX ${LIBFILE}`
    install_name_tool -id ${LIBFILE} ${LIBFILE}
    for TARGET2 in ${SFML_LIBNAMES} ; do
      LIBFILE2=$2/${TARGET2}.${SFML_VERSION}.dylib
      install_name_tool -change ${TARGETID} ${LIBFILE} ${LIBFILE2}
    done
  done
  echo "  done!"
fi
