#!/bin/bash

for f in *; do
    if [[ -d $f ]]; then
        ./compile.sh $f
    fi
done

