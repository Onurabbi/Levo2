#!/bin/bash
BASEDIR=$(dirname $0)
glslc $BASEDIR/shader.vert -o $BASEDIR/vert.spv
glslc $BASEDIR/shader.frag -o $BASEDIR/frag.spv
