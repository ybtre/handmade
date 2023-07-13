@echo off

set common_compiler_flags= -MT -nologo -Gm- -GR- -EHa- -Od -Oi -WX -W4 -wd4201 -wd4100 -wd4189 -DHANDMADE_INTERNAL=1 -DHANDMADE_SLOW=1 -DHANDMADE_WIN32=1 -FC -Z7 -Fmwin32_handmade.map
set common_linker_flags= -opt:ref -subsystem:windows,5.2 user32.lib gdi32.lib


REM TODO - can we just build both x86 and x64 with one exe?

IF NOT EXIST ..\build mkdir ..\build
pushd ..\build

cl %common_compiler_flags%  ..\code\win32_handmade.cpp /link %common_linker_flags%
popd

REM -Zi/-Z7 produce debug files
REM -Oi always do intrinsic versions
