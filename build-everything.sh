#! /bin/bash

set -x
aclocal -I config && \
autoheader && \
autoconf && \
./configure --enable-dev && \
make dep && \
make
