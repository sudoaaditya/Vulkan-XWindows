
gcc -c -o XWindow.o XWindow.c
gcc -o XWindow XWindow.o -lX11 -lm
./XWindow