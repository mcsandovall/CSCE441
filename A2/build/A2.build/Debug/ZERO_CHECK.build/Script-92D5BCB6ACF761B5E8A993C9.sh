#!/bin/sh
set -e
if test "$CONFIGURATION" = "Debug"; then :
  cd "/Users/mariopinto/Desktop/CSCE 441/CSCE441/A2/build"
  make -f /Users/mariopinto/Desktop/CSCE\ 441/CSCE441/A2/build/CMakeScripts/ReRunCMake.make
fi
if test "$CONFIGURATION" = "Release"; then :
  cd "/Users/mariopinto/Desktop/CSCE 441/CSCE441/A2/build"
  make -f /Users/mariopinto/Desktop/CSCE\ 441/CSCE441/A2/build/CMakeScripts/ReRunCMake.make
fi
if test "$CONFIGURATION" = "MinSizeRel"; then :
  cd "/Users/mariopinto/Desktop/CSCE 441/CSCE441/A2/build"
  make -f /Users/mariopinto/Desktop/CSCE\ 441/CSCE441/A2/build/CMakeScripts/ReRunCMake.make
fi
if test "$CONFIGURATION" = "RelWithDebInfo"; then :
  cd "/Users/mariopinto/Desktop/CSCE 441/CSCE441/A2/build"
  make -f /Users/mariopinto/Desktop/CSCE\ 441/CSCE441/A2/build/CMakeScripts/ReRunCMake.make
fi

