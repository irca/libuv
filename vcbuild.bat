@echo off

cd %~dp0

if /i "%1"=="help" goto help
if /i "%1"=="--help" goto help
if /i "%1"=="-help" goto help
if /i "%1"=="/help" goto help
if /i "%1"=="?" goto help
if /i "%1"=="-?" goto help
if /i "%1"=="--?" goto help
if /i "%1"=="/?" goto help

@rem Bail out early if not running in VS build env.
if not defined VCINSTALLDIR goto msbuild-not-found

@rem Process arguments.
set config=Debug
set target=Build
set noprojgen=
set run=

:next-arg
if "%1"=="" goto args-done
if /i "%1"=="debug"        set config=Debug&goto arg-ok
if /i "%1"=="release"      set config=Release&goto arg-ok
if /i "%1"=="test"         set run=run-tests.exe&goto arg-ok
if /i "%1"=="bench"        set run=run-benchmarks.exe&goto arg-ok
if /i "%1"=="clean"        set target=Clean&goto arg-ok
if /i "%1"=="noprojgen"    set noprojgen=1&goto arg-ok
:arg-ok
shift
goto next-arg
:args-done


@rem Skip project generation if requested.
if defined noprojgen goto msbuild

:project-gen
@rem Generate the VS project.
call create-msvs-files.bat
if errorlevel 1 goto create-msvs-files-failed
if not exist uv.sln goto create-msvs-files-failed

:msbuild
@rem Build the sln with msbuild.
msbuild uv.sln /t:%target% /p:Configuration=%config% /clp:NoSummary;NoItemAndPropertyList;Verbosity=minimal /nologo
if errorlevel 1 goto exit

:run
@rem Run tests if requested.
if "%run%"=="" goto exit
if not exist %config%\%run% goto exit
%config%\%run%
goto exit

:create-msvs-files-failed
echo Failed to create vc project files. 
goto exit

:msbuild-not-found
echo Failed to build.  In order to build the solution this file needs
echo to run from VS command script.
goto exit

:help
echo This script must run from VS command prompt.
echo vcbuild.bat [debug/release] [test/bench] [clean] [noprojgen]
echo Examples:
echo   vcbuild.bat              : builds debug build
echo   vcbuild.bat test         : builds debug build and runs tests
echo   vcbuild.bat release bench: builds release build and runs benchmarks
goto exit

:exit
