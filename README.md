# CSA
CSA (Captain Sonar Assist) is a tool to assist in winning games of Captain Sonar.

**Features:**
- Keeps track of your position and history
- Tracks down your enemy, including on maps such as Kilo where this is normally very difficult.
- Keeps track of the engineering and first mate boards.

**Dependencies:**
- gtk4 (https://www.gtk.org/)
- luajit (https://luajit.org/)
- VCL (https://github.com/vectorclass/version2 - this is a submodule which is automatically initialised by the makefile when the relevant build variable is set to enable use of SSE vectorisation)

**To build:**
- Clone https://github.com/hiornso/csa
- `cd csa`
- `make -j$(nproc)`
- `./csa`

**Benchmarking:**
- `make testbench`
- `./testbench`

This will run 50 back-to-back map renders of a test map at a 1000x1000 resolution and print the time taken. This is a standardised way of testing the performance of the map rendering, which is by far the most performance-needing part of CSA. It allows easy comparison of different settings, for example to see whether LTO or using VCL make a difference (positive or negative) on your machine. If the testbench is compiled with `OUTPUT_PNG=1` (this can be achieved with `CFLAGS="-DOUTPUT_PNG=1" make`) then it will also produce an image called `testbench.png` which is the result of rendering the map - this allows you to verify that the map was rendered correctly, and that you aren't testing how fast it is to render a blank screen.

**To install:**
- `make install`

**To uninstall:**
- `make uninstall`

**To clean the directory:**
- `make clean`

This removes the `csa` and `testbench` executables, object files, the extracted VCL source if it was extracted, `testbench.png` if it was created by the testbench, `resources.c` (this is autogenerated) and `resources/builder/menu.ui` (this is copied into this location from `linuxdeps` or `darwindeps` since it is OS-dependent).

**Further build customisation:**

Most of these will not need to be modified - the Makefile is designed to make sensible decisions and be aware of the system it is being run on.
- `OPT_LEVEL=3 make` sets the level of optimisation (internally used with `-O`). Result: passed as a compiler flag to control optimisation type. Default: `3` (fastest sensible setting)
- `USE_LTO=1 make` enables LTO (link-time optimisation). Result: slower compiles, marginally smaller and faster executable. Default: `0` (off)
- `USE_VCL=1 make` enables use of VCL (Agner Fog's Vectorclass library). Result: use of hardware vectors. This may slightly improve map rendering performance on some machines. Default: `0` (off)
- `CC=clang CXX=clang++ make` specifies the C/C++ compilers to use. Result: uses the specified compilers. Default: `clang`, `clang++`
- `MACOS_APP_NAME=Captain\ Sonar\ Assist.app make` specifies the name of the app bundle to be created when compiling on MacOS (this is created in addition to the executable itself). Result: sets the name of the app bundle on MacOS. Default: `Captain Sonar Assist.app`
There are some other settings available in the Makefile, but messing with them will likely break things unless you know what you're doing - if you want to learn how to use them, I'd recommend looking at the source.

**How to use CSA:**

Run the executable, load a map using the dropdown, enter all actions performed by yourself or by your opponent... (A proper manual is on the to-do list).
