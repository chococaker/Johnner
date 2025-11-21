cmake -S . -B build -G "MinGW Makefiles" -DCMAKE_CXX_COMPILER=g++
cmake --build build
cd build/src
johnner_perft 6 "8/8/1k6/2b5/2pP4/8/5K2/8 b - d3 0 1"
cd ../..
