@echo off
set CommonCompilerFlags=-nologo -Fmwin32_handmade.map -Gm- -MT -GR- -EHa- -Od -Oi -wd4189 -W3 -wd4201 -wd4100 -DHANDMADE_INTERNAL=1 -DHANDMADE_WIN32=1 -DHANDMADE_SLOW=1 -Z7 
set CommonLinkerFlags=-opt:ref  User32.lib Gdi32.lib  winmm.lib

REM TODO - can we just build both with one exe?
pushd  ..\Debug
REM 32-bit build
REM cl %CommonCompilerFlags% ..\code\win32_handmade.cpp /link  -subsystem:windows,5.1 %CommonLinkerFlags%

REM 64-bit build
cl %CommonCompilerFlags% ..\code\test_time.cpp /link %CommonLinkerFlags%

popd
