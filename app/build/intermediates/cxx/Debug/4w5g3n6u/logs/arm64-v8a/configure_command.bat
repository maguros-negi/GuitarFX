@echo off
"C:\\Users\\souta\\AppData\\Local\\Android\\Sdk\\cmake\\3.22.1\\bin\\cmake.exe" ^
  "-HC:\\Users\\souta\\Downloads\\GuitarFX_v0.1\\GuitarFX_v0.1\\app\\src\\main\\cpp" ^
  "-DCMAKE_SYSTEM_NAME=Android" ^
  "-DCMAKE_EXPORT_COMPILE_COMMANDS=ON" ^
  "-DCMAKE_SYSTEM_VERSION=27" ^
  "-DANDROID_PLATFORM=android-27" ^
  "-DANDROID_ABI=arm64-v8a" ^
  "-DCMAKE_ANDROID_ARCH_ABI=arm64-v8a" ^
  "-DANDROID_NDK=C:\\Users\\souta\\AppData\\Local\\Android\\Sdk\\ndk\\27.0.12077973" ^
  "-DCMAKE_ANDROID_NDK=C:\\Users\\souta\\AppData\\Local\\Android\\Sdk\\ndk\\27.0.12077973" ^
  "-DCMAKE_TOOLCHAIN_FILE=C:\\Users\\souta\\AppData\\Local\\Android\\Sdk\\ndk\\27.0.12077973\\build\\cmake\\android.toolchain.cmake" ^
  "-DCMAKE_MAKE_PROGRAM=C:\\Users\\souta\\AppData\\Local\\Android\\Sdk\\cmake\\3.22.1\\bin\\ninja.exe" ^
  "-DCMAKE_CXX_FLAGS=-std=c++20 -O3 -ffast-math" ^
  "-DCMAKE_LIBRARY_OUTPUT_DIRECTORY=C:\\Users\\souta\\Downloads\\GuitarFX_v0.1\\GuitarFX_v0.1\\app\\build\\intermediates\\cxx\\Debug\\4w5g3n6u\\obj\\arm64-v8a" ^
  "-DCMAKE_RUNTIME_OUTPUT_DIRECTORY=C:\\Users\\souta\\Downloads\\GuitarFX_v0.1\\GuitarFX_v0.1\\app\\build\\intermediates\\cxx\\Debug\\4w5g3n6u\\obj\\arm64-v8a" ^
  "-DCMAKE_BUILD_TYPE=Debug" ^
  "-DCMAKE_FIND_ROOT_PATH=C:\\Users\\souta\\Downloads\\GuitarFX_v0.1\\GuitarFX_v0.1\\app\\.cxx\\Debug\\4w5g3n6u\\prefab\\arm64-v8a\\prefab" ^
  "-BC:\\Users\\souta\\Downloads\\GuitarFX_v0.1\\GuitarFX_v0.1\\app\\.cxx\\Debug\\4w5g3n6u\\arm64-v8a" ^
  -GNinja ^
  "-DANDROID_STL=c++_shared"
