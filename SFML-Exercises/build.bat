@echo off
cls
IF NOT "%1"=="" (
	SET FILE=%1
) ELSE (
	SET FILE=main
)
@echo on
g++ -std=c++17 ^
	%FILE%.cpp ^
	-o %FILE%.exe ^
	-static ^
	-DSFML_STATIC  ^
	-Iinclude ^
	-Llibs ^
	-Lextlibs\libs-mingw\x64 ^
	-lsfml-network-s -lsfml-audio-s -lsfml-graphics-s -lsfml-window-s -lsfml-system-s ^
	-lopengl32  -lwinmm  -lgdi32 -lfreetype -lopenal32 -lflac -lvorbisenc -lvorbisfile -lvorbis -logg
	