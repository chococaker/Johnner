cmake -S . -B build -G "MinGW Makefiles" -DCMAKE_CXX_COMPILER=g++ DCMAKE_BUILD_TYPE=Release
cmake --build build
cd build/src
perft 3 "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1"
cd ../../
