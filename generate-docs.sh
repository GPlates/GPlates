#! /bin/bash

set -x
./configure --enable-dev && \
( cd doc && make doc )
