@set PATH=%PATH%;c:\msys64\mingw64\bin
gcc -O2 main.c -o usm.exe
strip usm.exe
