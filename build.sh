cmake -B build -DCMAKE_EXPORT_COMPILE_COMMANDS=1
if [ -z "$1" ]; then
  cmake --build build --config Debug -j4
else
  cmake --build build -j4
fi
