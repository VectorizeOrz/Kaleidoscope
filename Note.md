## Install LLVM
### Clone llvm
git clone https://github.com/llvm/llvm-project.git
### Build
cd llvm-project
cmake -S llvm -B build -G Ninja -DLLVM_ENABLE_PROJECTS=clang -DCMAKE_BUILD_TYPE=Release
cmake --build build
### Test
cmake --build build --target check-all
### Install
cmake --install build
