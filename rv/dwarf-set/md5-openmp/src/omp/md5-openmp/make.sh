#!/bin/bash

#gcc -MM *.c*

make all 2>make.2.txt
#make m_th 2>make.2.txt
echo ""
wc -l make.2.txt

