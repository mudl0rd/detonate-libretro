import glob
import pathlib
import sys
import os
desktop = pathlib.Path(sys.argv[1])
sources = []
for ext in ('*.cpp', '*.c'):
   sources.extend(desktop.rglob(ext))
for i in sources:
    print(i)