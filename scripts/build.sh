#!/bin/bash

./tools/premake5 gmake
make -C generated/ $@
