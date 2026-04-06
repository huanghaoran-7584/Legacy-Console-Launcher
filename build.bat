@echo off
chcp 65001>nul
echo Building embedded launcher...

REM Set MinGW path
set MINGW_PATH=C:\msys32\mingw64
set PATH=%MINGW_PATH%\bin;%PATH%

REM Step 1: Check if minecraft.zip exists
echo.
echo [1/5] Checking minecraft.zip...
if not exist ".minecraft.zip" (
    echo minecraft.zip not found!
    echo Please compress .minecraft folder manually and name it .minecraft.zip
    pause
    exit /b 1
)

REM Step 2: Copy 7-Zip files
echo.
echo [2/5] Copying 7-Zip files...
if not exist "C:\Program Files\7-Zip\7z.exe" (
    echo 7-Zip not found at C:\Program Files\7-Zip\7z.exe
    pause
    exit /b 1
)
copy "C:\Program Files\7-Zip\7z.exe" . /Y >nul
copy "C:\Program Files\7-Zip\7z.dll" . /Y >nul

REM Step 3: Create resource file
echo.
echo [3/5] Creating resource file...
echo #include ^<windows.h^> > minecraft.rc
echo 1 RCDATA ".minecraft.zip" >> minecraft.rc
echo 2 RCDATA "7z.exe" >> minecraft.rc
echo 3 RCDATA "7z.dll" >> minecraft.rc

REM Step 4: Compile resource
echo.
echo [4/5] Compiling resource files...
windres -i minecraft.rc -o minecraft_res.o
if %ERRORLEVEL% NEQ 0 (
    echo Failed to compile resources
    pause
    exit /b 1
)

REM Step 5: Compile and link
echo.
echo [5/5] Compiling and linking...
if not exist bin mkdir bin
g++ -std=c++17 -static -static-libgcc -static-libstdc++ -O2 -s -o bin\mc_launcher.exe mc_launcher_embedded.cpp minecraft_res.o -lshell32 -luser32
if %ERRORLEVEL% NEQ 0 (
    echo Compilation failed
    pause
    exit /b 1
)

REM Clean up
echo.
echo Cleaning up...
del minecraft.rc minecraft_res.o 7z.exe 7z.dll 2>nul

echo.
echo ====================================
echo Build complete!
echo Launcher: bin\mc_launcher.exe
echo Minecraft and 7-Zip embedded in exe
echo ====================================
echo.
echo Copy bin\mc_launcher.exe to run anywhere
echo First run will extract to %%LOCALAPPDATA%%\.minecraft
echo.

pause