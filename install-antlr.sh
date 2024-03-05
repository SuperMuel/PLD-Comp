#!/bin/sh

set -e # Abort the script at the first error

######################################################################
# Preparation step

# sanity check
if [ $(basename $PWD) != "pld-comp" ]
then
    echo "error: this script must be executed from within your 'pld-comp' directory"
    exit 1
fi

if test -d antlr.tmp
then
    echo "error: please remove the antlr.tmp directory (probably from a previous attempt)"
    exit 1
fi

if test -d antlr
then
    echo "error: antlr seems already installed"
    exit 1
fi

# cf https://moodle.insa-lyon.fr/mod/forum/discuss.php?d=11522
if ! grep -qs 'insteadof=git://github.com' ~/.gitconfig
then
    echo 'patching .gitconfig'
    echo '[url "https://github.com/"]' >> ~/.gitconfig
    echo '    insteadof=git://github.com' >> ~/.gitconfig
fi

# create a fresh temp directory
mkdir antlr.tmp
cd antlr.tmp

#download both the tool jar and the cpp runtime
curl -O https://www.antlr.org/download/antlr-4.9.2-complete.jar
curl -O https://www.antlr.org/download/antlr4-cpp-runtime-4.9.2-source.zip

######################################################################
# Build step

unzip *.zip
mkdir build
cd build
cmake ..
make -j8

cd ../..

######################################################################
# install step

# tool jar 
mkdir -p antlr/jar
mv antlr.tmp/*.jar antlr/jar

# runtime header files
mkdir antlr/include

cd antlr.tmp/runtime/src
find * -type d -print -exec mkdir ../../../antlr/include/'{}' ';'
find . -name '*.h' -print -exec cp '{}' ../../../antlr/include/'{}' ';'
cd ../../..

# runtime library files
mkdir antlr/lib
mv antlr.tmp/dist/libantlr4-runtime.a antlr/lib

######################################################################
# cleanup step

rm -rf antlr.tmp

echo PLD COMP: ANTLR OK
