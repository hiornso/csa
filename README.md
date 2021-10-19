# CSA
CSA (Captain Sonar Assist) is a tool to assist in winning games of Captain Sonar.

It makes it easy to:
- Keep track of your position and history
- Track down your enemy, including on maps such as Kilo where this is normally very difficult.
- Keep track of the engineering and first mate boards.

Dependencies:
- gtk4 (https://www.gtk.org/)
- luajit (https://luajit.org/)
- vcl (https://github.com/vectorclass/version2 - this is currently included in a zip file and is extracted by the makefile when the relevant build variable is set to enable use of SSE vectorisation)
- lrzip (https://github.com/ckolivas/lrzip - only needed if you intend to create distributions compressed using `lrzip`)

To build:
- Clone https://github.com/hiornso/csa
- `cd csa`
- `make -j$(nproc)`
- `./csa`

To install:
- `make install`
To uninstall:
- `make uninstall`
To make a distribution:
- `make distrib`
To clean the directory (removes `csa` executable, object files, the extracted VCL source if it was extracted, 

Further build customisation:
- `OPT_LEVEL=3 make` sets the level of optimisation (internally used with `-O`). Result: passed as a compiler flag to control optimisation type. Default: `3` (fastest sensible setting)
- `USE_LTO=1 make` enables LTO (link-time optimisation). Result: slower compiles, marginally smaller and faster executable. Default: `0` (off)
- `USE_VCL=1 make` enables use of VCL (Agner Fog's Vectorclass library). Result: use of hardware vectors. This may slightly improve map rendering performance on some machines. Default: `0` (off)
- `CC=clang CXX=clang++ make` specifies the C/C++ compilers to use. Result: uses the specified compilers. Default: `clang`, `clang++`
- `DO_GZIP_DISTRIB=1 DO_LRZIP_DISTRIB=1 make` specifies whether or not to make differently-compressed versions of distributions, when creating distributions with `make distrib`. Result: whether or not distributions compressed with `gzip`/`lrzip` are created. Default: both `1` (on)
- `DISTRIB_DIR=distrib make distrib` specifies the directory in which to put the created distributions when creating them with `make distrib`. Result: sets the directory to put created distributions in. Default: `distrib`
- `DISTRIB_FILENAME=$(shell date +"csa_%F_%H-%M-%S") make distrib` specifies the name of the created distribution archive (to which an extension will be added corresponding to the type of compression). Result: sets the name of the distribution file. Default: `$(shell date +"csa_%F_%H-%M-%S")` (generates a name like `csa_2021-05-13_23-30-45` for example, if the date and time was 23:30:45 on 2021-05-13, to which a suffix of `.tar.gz`/`.tar.lrz` will be appended when creating the relevant distribution)
- `MACOS_APP_NAME=Captain\ Sonar\ Assist.app make` specifies the name of the app bundle to be created when compiling on MacOS (this is created in addition to the executable itself). Result: sets the name of the app bundle on MacOS. Default: `Captain Sonar Assist.app`
- `UNZIP=unzip TAR=tar LRZIP=lrzip make` specifies the programs to be used for unzipping (used when extracting VCL), tarring (done when creating any distribution archives) and lrzipping (done when creating `.tar.lrz` distribution archives specifically). On MacOS/BSD, `gtar` must be used rather than `tar` so that the `--transform` switch is available (used to format paths in the distribution archives correctly during creation).
There are some other settings available in the makefile, but messing with them will likely break things unless you know what you're doing - if you want to learn how to use them, I'd recommend looking at the Makefile.

How to use CSA:

Run the executable, load a map using the dropdown, enter all actions by yourself or your opponent... (A proper manual is on the to-do list).
