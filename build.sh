#!/bin/bash
set -e
# copy common lib stuff
rsync -a -v --delete ../Components/src/utils/ src/utils/.
rsync -a -v --delete ../Components/res/components/ res/components/.
#rm src/utils/CVMidi.h
rm src/utils/ExpanderMidi.h
rm src/utils/LEDMatrix.h
#rm src/utils/Midi*
rm src/utils/ParamMapper.*
rm src/utils/ThemeChooser.*
# compile
make -j8 install
