@echo off
IF NOT EXIST build mkdir build
pushd build

set IncludePath="..\include"

call cl -nologo -Zi -FC /FC /Zi -FC /I%IncludePath% ..\src\smint.cpp

popd