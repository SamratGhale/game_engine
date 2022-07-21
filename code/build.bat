@echo off
set CommonCompilerFlags=-nologo -Gm- -MTd -GR- -EHa- -Od -Oi -wd4189 -W3 -wd4201 -wd4100 -DHANDMADE_INTERNAL=1 -DHANDMADE_WIN32=1 -DHANDMADE_SLOW=1 -Z7 
set CommonLinkerFlags=-opt:ref  User32.lib Gdi32.lib  winmm.lib

REM TODO - can we just build both with one exe?
IF NOT EXIST ..\build mkdir ..\build
pushd  ..\build
del *.pdb > NULL 2> NULL
REM 32-bit build
REM cl %CommonCompilerFlags% ..\code\win32_handmade.cpp /link  -subsystem:windows,5.1 %CommonLinkerFlags%

:: DLL Linker switches
set dll_link =           -EXPORT:GameUpdateAndRender
set dll_link =%dll_link% -EXPORT:GameGetSoundSamples

REM 64-bit build
cl %CommonCompilerFlags% -Fmwin32_handmade.map ..\code\win32_handmade.cpp /link %CommonLinkerFlags%

cl %CommonCompilerFlags% ..\code\handmade.cpp -Fmhandmade.map -LD /link -incremental:no -opt:ref -PDB:handmade_%random%.pdb -EXPORT:GameUpdateAndRender -EXPORT:GameGetSoundSamples 


popd
