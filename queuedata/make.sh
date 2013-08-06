#!/bin/bash
gcc -I /usr/src/linux-2.6.39.2/include -DMODULE -D__KERNEL__ -c seqfile_queuedata_rbfgrad.c
