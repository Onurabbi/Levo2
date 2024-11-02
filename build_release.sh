#!/bin/bash
start=$(date +%s.%3N)
clang -Wall -Wfatal-errors -Wno-gnu -Wno-microsoft -O3 -DVULKAN_BACKEND -std=c11 -fms-extensions \
-I"./libs/" ./src/main.c -lSDL2 -lvulkan -ldl -lpthread -lX11 -lXxf86vm -lXrandr -lXi -lm -lktx -o gameengine
stop=$(date +%s.%3N)
echo "the build took $(bc <<< $stop-$start) seconds"