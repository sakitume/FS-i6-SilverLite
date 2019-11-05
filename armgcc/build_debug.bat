cmake -DCMAKE_TOOLCHAIN_FILE="./armgcc.cmake" -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Debug  .
mingw32-make -j4 2> build_log.txt 
