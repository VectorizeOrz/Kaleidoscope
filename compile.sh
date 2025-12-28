clang++ -g -O3 main.cpp -rdynamic `llvm-config --cxxflags --ldflags --system-libs --libs all` -o kale
