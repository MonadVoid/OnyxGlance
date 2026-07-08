@echo off 

mkdir ..\..\build 
pushd ..\..\build 

cl -FC -Zi  w:\onyx_glance\code\win32_og.cpp user32.lib gdi32.lib onecore.lib 
REM && win32_og.exe

popd 


