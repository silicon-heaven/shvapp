# shvapp
[Beginner's guide](beginners-guide.md)

## Build
Use `-DCMAKE_PREFIX_PATH:PATH=/path/to/qt/gcc_64` only if you do not want to use system Qt.
```sh
git clone https://github.com/silicon-heaven/shvapp.git
cd shvapp
mkdir build
cd build
cmake -DSHV_USE_QT6=OFF -DLIBSHV_USE_QT6=OFF -DCMAKE_PREFIX_PATH:PATH=/path/to/qt/gcc_64 ..
cmake --build .
cmake --install . --prefix /dir/to/install
```
