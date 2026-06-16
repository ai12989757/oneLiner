@echo off
setlocal EnableExtensions EnableDelayedExpansion

set "SCRIPT_DIR=%~dp0"
set "PROJECT_DIR=%SCRIPT_DIR:~0,-1%"
set "SOURCE_DIR=%PROJECT_DIR%\script"
set "ONEQT_DIR=%PROJECT_DIR%\OneQtC++"
set "BIN_ROOT=%PROJECT_DIR%\bin"
set "BUILD_ROOT=%PROJECT_DIR%\build"

if "%VS_VCVARS%"=="" set "VS_VCVARS=C:\Program Files\Microsoft Visual Studio\18\Insiders\VC\Auxiliary\Build\vcvars64.bat"
if "%MAYA_INSTALL_ROOT%"=="" set "MAYA_INSTALL_ROOT=C:\Program Files\Autodesk"
if "%MAYA_VERSION_LIST%"=="" set "MAYA_VERSION_LIST=2027 2026 2025 2024 2023 2022 2021 2020 2019 2018 2017 2016 2015 2014 2013 2012"

if not defined VisualStudioVersion (
    call "%VS_VCVARS%"
    if errorlevel 1 exit /b 1
)

if not exist "%SOURCE_DIR%" (
    echo [ERROR] SOURCE_DIR not found: "%SOURCE_DIR%"
    exit /b 1
)

if not exist "%ONEQT_DIR%" (
    echo [ERROR] ONEQT_DIR not found: "%ONEQT_DIR%"
    exit /b 1
)

set "FOUND_COUNT=0"
set "SUCCESS_COUNT=0"
set "FAIL_COUNT=0"
set "SKIP_COUNT=0"
set "FAILED_VERSIONS="
set "SKIPPED_VERSIONS="

echo [INFO] Searching installed Maya versions under "%MAYA_INSTALL_ROOT%"...

for %%V in (%MAYA_VERSION_LIST%) do (
    if exist "%MAYA_INSTALL_ROOT%\Maya%%V" (
        set /a FOUND_COUNT+=1
        call :BuildMayaVersion %%V "%MAYA_INSTALL_ROOT%\Maya%%V"
        if errorlevel 2 (
            set /a SKIP_COUNT+=1
            set "SKIPPED_VERSIONS=!SKIPPED_VERSIONS! %%V"
        ) else if errorlevel 1 (
            set /a FAIL_COUNT+=1
            set "FAILED_VERSIONS=!FAILED_VERSIONS! %%V"
        ) else (
            set /a SUCCESS_COUNT+=1
        )
    )
)

echo.
echo [SUMMARY] Found: %FOUND_COUNT%, Succeeded: %SUCCESS_COUNT%, Failed: %FAIL_COUNT%, Skipped: %SKIP_COUNT%
if not "%FAILED_VERSIONS%"=="" echo [SUMMARY] Failed versions:%FAILED_VERSIONS%
if not "%SKIPPED_VERSIONS%"=="" echo [SUMMARY] Skipped versions:%SKIPPED_VERSIONS%

if "%FOUND_COUNT%"=="0" (
    echo [ERROR] No Maya installation found under "%MAYA_INSTALL_ROOT%".
    exit /b 1
)

if exist "%BUILD_ROOT%" rd /s /q "%BUILD_ROOT%" 2>nul

if not "%FAIL_COUNT%"=="0" exit /b 1
exit /b 0

:BuildMayaVersion
set "MAYA_VERSION=%~1"
set "MAYA_ROOT=%~2"
set "MAYA_INCLUDE=%MAYA_ROOT%\include"
set "MAYA_LIB=%MAYA_ROOT%\lib"
set "QT_INCLUDE="
set "QT_LIB=%MAYA_LIB%"
set "BIN_DIR=%BIN_ROOT%\%MAYA_VERSION%"
set "BUILD_DIR=%BUILD_ROOT%\%MAYA_VERSION%"
set "TEMP_MLL=%BUILD_DIR%\oneLiner.mll"

echo.
echo ============================================================
echo [INFO] Building Maya %MAYA_VERSION%
echo [INFO] Maya root: "%MAYA_ROOT%"

if not exist "%MAYA_INCLUDE%" (
    echo [SKIP] Maya %MAYA_VERSION% include folder not found: "%MAYA_INCLUDE%"
    exit /b 2
)

if not exist "%MAYA_LIB%" (
    echo [SKIP] Maya %MAYA_VERSION% lib folder not found: "%MAYA_LIB%"
    exit /b 2
)

for /d %%Q in ("%MAYA_INCLUDE%\qt*_vc*-include") do (
    if not defined QT_INCLUDE set "QT_INCLUDE=%%~fQ"
)

if not defined QT_INCLUDE (
    if exist "%MAYA_INCLUDE%\QtCore" set "QT_INCLUDE=%MAYA_INCLUDE%"
)

if not defined QT_INCLUDE (
    echo [SKIP] Maya %MAYA_VERSION% Qt include folder not found under "%MAYA_INCLUDE%".
    exit /b 2
)

if not exist "%QT_LIB%\Qt5Core.lib" (
    echo [SKIP] Maya %MAYA_VERSION% Qt5Core.lib not found: "%QT_LIB%\Qt5Core.lib"
    exit /b 2
)

if not exist "%BIN_DIR%" mkdir "%BIN_DIR%"
if not exist "%BUILD_DIR%" mkdir "%BUILD_DIR%"

set "CORE_DIR=%ONEQT_DIR%\Core"
set "WIDGETS_DIR=%ONEQT_DIR%\Widgets"
set "CPPFLAGS=/nologo /std:c++17 /utf-8 /EHsc /MD /W4 /Zc:__cplusplus /DWIN32 /D_WINDOWS /DUNICODE /D_UNICODE /DNT_PLUGIN /DQT_NO_KEYWORDS /DQT_DEPRECATED_WARNINGS /DONETWIDGETS_STATIC /I"%SOURCE_DIR%" /I"%CORE_DIR%" /I"%CORE_DIR%\OneAnimIcon" /I"%CORE_DIR%\OneBackground" /I"%CORE_DIR%\OneBrush" /I"%CORE_DIR%\OneFont" /I"%CORE_DIR%\OneIcon" /I"%CORE_DIR%\Internal" /I"%WIDGETS_DIR%" /I"%WIDGETS_DIR%\OneAction" /I"%WIDGETS_DIR%\OneList" /I"%WIDGETS_DIR%\OneMenu" /I"%WIDGETS_DIR%\OneScrollBar" /I"%WIDGETS_DIR%\OneSeparator" /I"%WIDGETS_DIR%\OneTree" /I"%MAYA_INCLUDE%" /I"%QT_INCLUDE%" /I"%QT_INCLUDE%\QtCore" /I"%QT_INCLUDE%\QtGui" /I"%QT_INCLUDE%\QtWidgets" /I"%QT_INCLUDE%\QtSvg""
set "LDFLAGS=/NOLOGO /DLL /INCREMENTAL:NO /MACHINE:X64 /LIBPATH:"%MAYA_LIB%" /LIBPATH:"%QT_LIB%" Foundation.lib OpenMaya.lib OpenMayaUI.lib Qt5Core.lib Qt5Gui.lib Qt5Widgets.lib Qt5Svg.lib user32.lib gdi32.lib shell32.lib advapi32.lib imm32.lib"

pushd "%BUILD_DIR%" >nul
del /q *.obj *.exp *.lib *.pdb *.ilk *.mll 2>nul

cl %CPPFLAGS% /c ^
    "%CORE_DIR%\OneBrush\oneqt_brush.cpp" ^
    "%CORE_DIR%\OneAnimIcon\one_anim_icon.cpp" ^
    "%CORE_DIR%\Internal\oneqt_geometry.cpp" ^
    "%CORE_DIR%\Internal\oneqt_source.cpp" ^
    "%CORE_DIR%\Internal\oneqt_renderer.cpp" ^
    "%CORE_DIR%\Internal\oneqt_system.cpp" ^
    "%CORE_DIR%\OneBackground\one_background.cpp" ^
    "%CORE_DIR%\OneFont\one_font.cpp" ^
    "%CORE_DIR%\OneIcon\one_icon.cpp" ^
    "%WIDGETS_DIR%\OneAction\one_action.cpp" ^
    "%WIDGETS_DIR%\OneList\one_list.cpp" ^
    "%WIDGETS_DIR%\OneMenu\one_menu.cpp" ^
    "%WIDGETS_DIR%\OneScrollBar\one_scroll_bar.cpp" ^
    "%WIDGETS_DIR%\OneTree\one_tree.cpp" ^
    "%WIDGETS_DIR%\OneTree\one_tree_header_view.cpp" ^
    "%WIDGETS_DIR%\OneTree\internal\one_tree_maya_style.cpp" ^
    "%SOURCE_DIR%\one_liner_engine.cpp" ^
    "%SOURCE_DIR%\one_liner_help.cpp" ^
    "%SOURCE_DIR%\one_liner_preview.cpp" ^
    "%SOURCE_DIR%\one_liner_tools_menu.cpp" ^
    "%SOURCE_DIR%\one_liner_ui.cpp" ^
    "%SOURCE_DIR%\one_liner_plugin.cpp"
if errorlevel 1 (
    echo [ERROR] Maya %MAYA_VERSION% compile failed.
    popd >nul
    exit /b 1
)

link %LDFLAGS% /OUT:"%TEMP_MLL%" ^
    oneqt_brush.obj ^
    one_anim_icon.obj ^
    oneqt_geometry.obj ^
    oneqt_source.obj ^
    oneqt_renderer.obj ^
    oneqt_system.obj ^
    one_background.obj ^
    one_font.obj ^
    one_icon.obj ^
    one_action.obj ^
    one_list.obj ^
    one_menu.obj ^
    one_scroll_bar.obj ^
    one_tree.obj ^
    one_tree_header_view.obj ^
    one_tree_maya_style.obj ^
    one_liner_engine.obj ^
    one_liner_help.obj ^
    one_liner_preview.obj ^
    one_liner_tools_menu.obj ^
    one_liner_ui.obj ^
    one_liner_plugin.obj
if errorlevel 1 (
    echo [ERROR] Maya %MAYA_VERSION% link failed.
    popd >nul
    exit /b 1
)

del /q "%BIN_DIR%\oneLiner.exp" "%BIN_DIR%\oneLiner.lib" "%BIN_DIR%\oneLiner.pdb" "%BIN_DIR%\oneLiner.ilk" 2>nul
copy /y "%TEMP_MLL%" "%BIN_DIR%\oneLiner.mll" >nul
if errorlevel 1 (
    echo [WARN] Built Maya %MAYA_VERSION% temporary mll but failed to overwrite "%BIN_DIR%\oneLiner.mll". The file is likely locked by Maya.
    echo [WARN] New build is available at "%TEMP_MLL%"
    popd >nul
    exit /b 1
)

del /q *.exp *.lib *.pdb *.ilk *.obj *.mll 2>nul
popd >nul
rmdir /s /q "%BUILD_DIR%" 2>nul
echo [OK] Built Maya %MAYA_VERSION%: "%BIN_DIR%\oneLiner.mll"
exit /b 0
