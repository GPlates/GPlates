#! /bin/bash

autoconf && \
	./configure --enable-dev && \
	make dep && \
	make
