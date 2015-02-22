@echo on
rem This scripts configures build environment and runs CMake.
rem Use it instead of running CMake directly when building with
rem the Microsoft SDK toolchain rather than Visual Studio.
rem It is used in the same way as cmake, for example:
rem
rem   run-cmake -G "Visual Studio 10 Win64" .

for /F "delims=" %%i IN ('cmake "-DPRINT_PATH=1" -P %~dp0/FindSetEnv.cmake') DO set setenv=%%i
if NOT "%setenv%" == "" call "%setenv%"
cmake %*
