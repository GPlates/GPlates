#! /bin/bash

set -x
aclocal -I config && \
autoconf && \
./configure --enable-dev && \
make dep && \
make
