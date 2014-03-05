DEFINES="-DFRAMEBUFFER_WIDTH=144 -DFRAMEBUFFER_HEIGHT=144"
gcc $DEFINES -std=c99 -I. -I../src main.c ../src/dither.c ../src/model_stl.c ../src/miniGL/draw2d.c ../src/miniGL/miniGL.c ../src/miniGL/llvm_extendsfdf2.c -o sdl_main.bin -lm -lSDL
