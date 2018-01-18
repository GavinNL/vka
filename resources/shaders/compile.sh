#!/bin/bash

BASE=$1
VERT=$BASE/$1.vert
FRAG=$BASE/$1.frag
GEOM=$BASE/$1.geom

mkdir -p $BASE

if [ -f $VERT  ] ; then
	glslangValidator -V $VERT -o $BASE/${BASE}_v.spv
fi

if [ -f $FRAG  ] ; then
	glslangValidator -V $FRAG -o $BASE/${BASE}_f.spv
fi

if [ -f $GEOM ] ; then
	glslangValidator -V $GEOM -o $BASE/${BASE}_g.spv
fi
