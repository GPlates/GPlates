#! /bin/bash

set -x
aclocal && \
autoconf && \
./configure --enable-dev && \
make dep && \
make
