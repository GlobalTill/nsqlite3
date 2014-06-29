#!/usr/bin/env bash

set -u -e

if [[ ! -d ../.nvm ]]; then
    git clone https://github.com/creationix/nvm.git ../.nvm
fi
source ../.nvm/nvm.sh
nvm install $NODE_VERSION
nvm use $NODE_VERSION
node --version
npm --version

# test installing from source
npm install --build-from-source
node-pre-gyp package testpackage
npm test

PUBLISH_BINARY=false
if test "${COMMIT_MESSAGE#*'[publish binary]'}" != "$COMMIT_MESSAGE"; then
    node-pre-gyp publish
    node-pre-gyp info
    node-pre-gyp clean
    make clean
    # now install from binary
    INSTALL_RESULT=$(npm install --fallback-to-build=false > /dev/null)$? || true
    # if install returned non zero (errored) then we first unpublish and then call false so travis will bail at this line
    if [[ $INSTALL_RESULT != 0 ]]; then echo "returned $INSTALL_RESULT";node-pre-gyp unpublish;false; fi
    # If success then we arrive here so lets clean up
    node-pre-gyp clean
fi

# now test building against shared sqlite
if [[ $(uname -s) == 'Darwin' ]]; then
    brew install sqlite
    npm install --build-from-source --sqlite=$(brew --prefix)
else
    sudo apt-get -qq update
    sudo apt-get -qq install libsqlite3-dev
    npm install --build-from-source --sqlite=/usr
fi
npm test

if [[ $(uname -s) == 'Linux' ]]; then
    sudo apt-get -y install gcc-multilib g++-multilib; fi
    # node v0.8 and above provide pre-built 32 bit and 64 bit binaries
    # so here we use the 32 bit ones to also test 32 bit builds
    NVER=`node -v`
    wget http://nodejs.org/dist/${NVER}/node-${NVER}-${platform}-x86.tar.gz
    tar xf node-${NVER}-${platform}-x86.tar.gz
    # enable 32 bit node
    export PATH=$(pwd)/node-${NVER}-${platform}-x86/bin:$PATH
    # install 32 bit compiler toolchain and X11
    # test source compile in 32 bit mode with internal libsqlite3
    CC=gcc-4.6 CXX=g++-4.6 npm install --build-from-source
    npm install --build-from-source
    npm test
    if test "${COMMIT_MESSAGE#*'[publish binary]'}" != "$COMMIT_MESSAGE"; then
        node-pre-gyp package publish
    fi
    make clean
    # test source compile in 32 bit mode against external libsqlite3
    sudo apt-get -y install libsqlite3-dev:i386
    CC=gcc-4.6 CXX=g++-4.6 npm install --build-from-source --sqlite=/usr
    npm test
fi
