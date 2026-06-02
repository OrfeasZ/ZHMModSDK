set(CMAKE_SYSTEM_NAME Windows)
set(CMAKE_SYSTEM_PROCESSOR AMD64)

set(xwin "$ENV{XWIN_SPLAT_DIR}")
if (NOT xwin)
  get_filename_component(xwin "../../.xwin/splat" ABSOLUTE)
endif ()
if (NOT EXISTS "${xwin}/crt/include")
  message(FATAL_ERROR "no Windows SDK at ${xwin} -- run cmake/scripts/fetch-xwin.sh")
endif ()

set(CMAKE_C_COMPILER clang-cl)
set(CMAKE_CXX_COMPILER clang-cl)
set(CMAKE_C_COMPILER_TARGET x86_64-pc-windows-msvc)
set(CMAKE_CXX_COMPILER_TARGET x86_64-pc-windows-msvc)
set(CMAKE_LINKER lld-link)
set(CMAKE_AR llvm-lib)
set(CMAKE_RC_COMPILER llvm-rc)
set(CMAKE_MT llvm-mt)

# MASM assembler for ports that have .asm sources (e.g. crashpad inside
# sentry-native). Upstream expects ml64.exe; llvm-ml is the LLVM-shipped,
# ml64-compatible assembler. Unlike ml64 it defaults to 32-bit, so force x64.
set(CMAKE_ASM_MASM_COMPILER llvm-ml)
set(CMAKE_ASM_MASM_FLAGS_INIT "-m64")

set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

# Emit both CodeView (for PDBs) and DWARF (so GDB on Linux can read symbols
# straight out of the DLL when debugging the game under Proton).
set(CMAKE_POLICY_DEFAULT_CMP0141 NEW)
set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "Embedded")
set(_zhm_dwarf_flags "/clang:-gdwarf-4 /clang:-fstandalone-debug")
set(CMAKE_C_FLAGS_DEBUG_INIT "${_zhm_dwarf_flags}")
set(CMAKE_CXX_FLAGS_DEBUG_INIT "${_zhm_dwarf_flags}")
set(CMAKE_C_FLAGS_RELWITHDEBINFO_INIT "${_zhm_dwarf_flags} -O2")
set(CMAKE_CXX_FLAGS_RELWITHDEBINFO_INIT "${_zhm_dwarf_flags} -O2")

foreach (d crt/include sdk/include/ucrt sdk/include/um sdk/include/shared sdk/include/winrt)
  string(APPEND inc " /imsvc${xwin}/${d}")
endforeach ()

set(CMAKE_C_FLAGS_INIT "${inc} -Wno-unused-command-line-argument -Wno-c++-keyword")
set(CMAKE_CXX_FLAGS_INIT "${inc} -Wno-unused-command-line-argument -Wno-c++-keyword /EHsc")

foreach (d crt/lib/x86_64 sdk/lib/um/x86_64 sdk/lib/ucrt/x86_64)
  string(APPEND lib " /libpath:${xwin}/${d}")
endforeach ()

set(CMAKE_EXE_LINKER_FLAGS_INIT "${lib}")
set(CMAKE_SHARED_LINKER_FLAGS_INIT "${lib}")
set(CMAKE_MODULE_LINKER_FLAGS_INIT "${lib}")

set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)
