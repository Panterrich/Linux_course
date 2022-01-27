#!/bin/bash

test_number=0

function unittest() {
    echo -e "=========================================================="
    echo -e "Test #$test_number:"
    echo -e "Input"
    echo -e "$@ \n"
    echo -e "Output:"

    ./bizzbuzz $@

    echo -e "==========================================================\n\n"

    test_number=$(($test_number + 1))
}

unittest 120 
unittest -
unittest -0
unittest 112333333333333333333092340948232389423482348948932423789423823483478234723487432782347862347682347823478623476823478
unittest 12313089123093108931290812098123908123089123089treiu3289428234892348989342
unittest 1.23
unittest 1231 123 032 235 2113 sad a 76886