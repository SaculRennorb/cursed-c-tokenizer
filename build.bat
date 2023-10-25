@echo off
call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvarsall.bat" x64

SET BASIC_OPTIONS=-nologo -Oi -GR- -EHa- -Zi -FC -diagnostics:column /std:c++20

IF NOT EXIST build mkdir build
pushd build
cl %BASIC_OPTIONS% ../main.cpp /link /INCREMENTAL:NO /out:../test.exe
popd

test.exe

pushd build
cl %BASIC_OPTIONS% ../test_serializer.cpp /link /INCREMENTAL:NO /out:../test_serializer.exe
popd

test_serializer.exe
