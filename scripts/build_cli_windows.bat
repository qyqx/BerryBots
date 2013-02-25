REM Note: This is for building BerryBots to run single battles from the command
REM       line, like the Raspberry Pi version.
cd luajit
mingw32-make
cd ..
g++ bbsfmlmain.cpp bbutil.cpp gfxmanager.cpp filemanager.cpp circle2d.cpp ^
line2d.cpp point2d.cpp sensorhandler.cpp zone.cpp bbengine.cpp bblua.cpp ^
gfxeventhandler.cpp rectangle.cpp stage.cpp cliprinthandler.cpp ^
clipackagereporter.cpp dockitem.cpp dockshape.cpp docktext.cpp  dockfader.cpp ^
zipper.cpp guizipper.cpp ^
.\luajit\src\lua51.dll .\libarchive\libarchive_static.a .\zlib1.dll ^
-I.\luajit\src -I.\libarchive\include -I.\stlsoft-1.9.116\include ^
-I.\sfml\include -L.\sfml-lib -lpthread ^
-lsfml-graphics -lsfml-window -lsfml-system -o bbmain
mkdir luajit-lib
copy .\luajit\src\lua51.dll .\luajit-lib
copy .\scripts\bb_windows.bat .\berrybots.bat