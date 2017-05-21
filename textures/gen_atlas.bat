textureatlas.exe -I "images" --width 256 --height 256 -O out -P tex
xcopy /hrkysd out\tex_0.* ..\bin\64\Debug\overlay_atlas.*
xcopy /hrkysd out\tex_0.* ..\bin\64\Release\overlay_atlas.*
xcopy /hrkysd out\tex_0.* ..\bin\32\Debug\overlay_atlas.*
xcopy /hrkysd out\tex_0.* ..\bin\32\Release\overlay_atlas.*

xcopy /hrkysd out\tex_0.* ..\resource\overlay_atlas.*
