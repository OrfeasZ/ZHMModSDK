# Building TitaniumBullets with Visual Studio Build Tools

You can build this plugin using **Visual Studio Build Tools** without needing the full Visual Studio IDE.

## Prerequisites

1. **Visual Studio Build Tools 2022** (you already have this)
   - Make sure "Desktop development with C++" workload is installed
   - Run the VS Build Tools installer and check:
     - MSVC v143 - VS 2022 C++ x64/x86 build tools
     - Windows 10/11 SDK
     - C++ CMake tools for Windows

2. **CMake** (usually included with Build Tools, or install separately)
   - Download from: https://cmake.org/download/
   - Add to PATH during installation

3. **vcpkg** (package manager for C++ dependencies)
   ```cmd
   cd C:\
   git clone https://github.com/microsoft/vcpkg.git
   cd vcpkg
   bootstrap-vcpkg.bat
   ```

4. **Git** (to clone ZHMModSDK if needed)

## Build Steps

### Step 1: Open Developer Command Prompt

Open **"Developer Command Prompt for VS 2022"** or **"x64 Native Tools Command Prompt for VS 2022"**

You can find this in Start Menu under "Visual Studio 2022" folder.

Alternatively, run this in a normal cmd:
```cmd
"C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\Common7\Tools\VsDevCmd.bat" -arch=amd64
```

### Step 2: Install vcpkg Dependencies

```cmd
cd C:\vcpkg

vcpkg install 7zip:x64-windows-static
vcpkg install directxtk12:x64-windows-static
vcpkg install minhook:x64-windows-static
vcpkg install spdlog:x64-windows-static
vcpkg install directx-headers:x64-windows-static
vcpkg install libuv:x64-windows-static
vcpkg install zlib:x64-windows-static
vcpkg install simdjson:x64-windows-static
vcpkg install openssl:x64-windows-static
vcpkg install neargye-semver:x64-windows-static
vcpkg install sentry-native:x64-windows-static
vcpkg install imgui[docking-experimental]:x64-windows-static
vcpkg install imguizmo:x64-windows-static
vcpkg install imgui-node-editor:x64-windows-static
```

This will take a while (20-60 minutes depending on your system).

### Step 3: Navigate to ZHMModSDK Source

```cmd
cd /d "C:\path\to\your\ZHMModSDK(source)"
```

### Step 4: Configure with CMake

```cmd
cmake -B build -S . -G "Visual Studio 17 2022" -A x64 ^
  -DCMAKE_TOOLCHAIN_FILE=C:/vcpkg/scripts/buildsystems/vcpkg.cmake ^
  -DVCPKG_TARGET_TRIPLET=x64-windows-static ^
  -DGAME_INSTALL_PATH="C:/Program Files/Epic Games/HITMAN3"
```

**Notes:**
- Change `C:/vcpkg` to your vcpkg installation path
- Change `GAME_INSTALL_PATH` to your HITMAN 3 installation path
- Use forward slashes `/` in paths for CMake

### Step 5: Build

```cmd
cmake --build build --config Release --target TitaniumBullets
```

To build all mods:
```cmd
cmake --build build --config Release
```

### Step 6: Find the DLL

The built DLL will be at:
```
build\Mods\TitaniumBullets\Release\TitaniumBullets.dll
```

### Step 7: Install

Copy the DLL to your game:
```cmd
copy build\Mods\TitaniumBullets\Release\TitaniumBullets.dll "C:\Program Files\Epic Games\HITMAN3\Retail\mods\"
```

## Alternative: Build Only TitaniumBullets (Standalone)

If you have trouble building the full SDK, you can build just the TitaniumBullets plugin as a standalone project.

### Standalone Build Script

Create a file `build_standalone.bat`:

```batch
@echo off
setlocal

REM Set paths
set VCPKG_ROOT=C:\vcpkg
set ZHMMODSDK_DIR=C:\path\to\ZHMModSDK(source)
set OUTPUT_DIR=%~dp0build

REM Find Visual Studio
call "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\Common7\Tools\VsDevCmd.bat" -arch=amd64

REM Create build directory
if not exist "%OUTPUT_DIR%" mkdir "%OUTPUT_DIR%"

REM Compile
cl.exe /nologo /EHsc /std:c++20 /MD /O2 /W3 ^
  /I"%ZHMMODSDK_DIR%\ZHMModSDK\Include" ^
  /I"%VCPKG_ROOT%\installed\x64-windows-static\include" ^
  /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_USRDLL" ^
  /c Src\TitaniumBullets.cpp ^
  /Fo"%OUTPUT_DIR%\TitaniumBullets.obj"

link.exe /nologo /DLL ^
  /OUT:"%OUTPUT_DIR%\TitaniumBullets.dll" ^
  "%OUTPUT_DIR%\TitaniumBullets.obj" ^
  /LIBPATH:"%ZHMMODSDK_DIR%\build\ZHMModSDK\Release" ^
  ZHMModSDK.lib kernel32.lib user32.lib

echo.
echo Build complete! DLL is at: %OUTPUT_DIR%\TitaniumBullets.dll
```

**Note:** This standalone approach requires ZHMModSDK to already be built for linking.

## Troubleshooting

### "cmake not found"
- Install CMake or use the one from Build Tools
- Add CMake to PATH: `C:\Program Files\CMake\bin`

### "vcpkg not found"
- Clone vcpkg and run `bootstrap-vcpkg.bat`
- Add to PATH: `C:\vcpkg`

### "Generator not found"
- Make sure Build Tools is properly installed
- Use `cmake --help` to see available generators
- Try: `-G "Visual Studio 17 2022"`

### Linking errors
- Make sure all vcpkg packages installed successfully
- Use the same triplet everywhere (`x64-windows-static`)

### Missing dependencies
Run `vcpkg integrate install` to integrate with Visual Studio/Build Tools.

## Quick Command Summary

```cmd
REM 1. Open Developer Command Prompt for VS 2022

REM 2. Navigate to source
cd /d "C:\path\to\ZHMModSDK(source)"

REM 3. Configure
cmake -B build -S . -G "Visual Studio 17 2022" -A x64 -DCMAKE_TOOLCHAIN_FILE=C:/vcpkg/scripts/buildsystems/vcpkg.cmake -DVCPKG_TARGET_TRIPLET=x64-windows-static

REM 4. Build
cmake --build build --config Release --target TitaniumBullets

REM 5. Copy
copy build\Mods\TitaniumBullets\Release\TitaniumBullets.dll "HITMAN3\Retail\mods\"