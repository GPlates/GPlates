#! /bin/bash

set -x
autoconf && \
./configure --enable-dev && \
make doc
