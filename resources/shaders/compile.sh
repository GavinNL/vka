#!/bin/bash

BASE=$1
VERT=$BASE/$1.vert
FRAG=$BASE/$1.frag
GEOM=$BASE/$1.geom
TESC=$BASE/$1.tesc
TESE=$BASE/$1.tese

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

if [ -f $TESC ] ; then
	glslangValidator -V $TESC -o $BASE/${BASE}_tc.spv
fi

if [ -f $TESE ] ; then
	glslangValidator -V $TESE -o $BASE/${BASE}_te.spv
fi