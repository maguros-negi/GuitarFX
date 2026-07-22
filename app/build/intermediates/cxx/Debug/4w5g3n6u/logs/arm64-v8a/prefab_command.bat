@echo off
"C:\\Program Files\\Android\\Android Studio\\jbr\\bin\\java" ^
  --class-path ^
  "C:\\Users\\souta\\.gradle\\caches\\modules-2\\files-2.1\\com.google.prefab\\cli\\2.1.0\\aa32fec809c44fa531f01dcfb739b5b3304d3050\\cli-2.1.0-all.jar" ^
  com.google.prefab.cli.AppKt ^
  --build-system ^
  cmake ^
  --platform ^
  android ^
  --abi ^
  arm64-v8a ^
  --os-version ^
  27 ^
  --stl ^
  c++_shared ^
  --ndk-version ^
  27 ^
  --output ^
  "C:\\Users\\souta\\AppData\\Local\\Temp\\agp-prefab-staging2518111787530844729\\staged-cli-output" ^
  "C:\\Users\\souta\\.gradle\\caches\\8.9\\transforms\\1114272a07611eb40b6da6a2370796ea\\transformed\\oboe-1.10.0\\prefab"
