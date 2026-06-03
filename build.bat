@echo off
setlocal EnableExtensions

set "SCRIPT_DIR=%~dp0"
set "PROJECT_DIR=%SCRIPT_DIR:~0,-1%"
set "SOURCE_DIR=%PROJECT_DIR%\script"
set "ONEQT_DIR=%PROJECT_DIR%\OneQtC++"
set "BIN_ROOT=%PROJECT_DIR%\bin"
set "BUILD_ROOT=%PROJECT_DIR%\build"

if "%MAYA_VERSION%"=="" set "MAYA_VERSION=2024"
if "%VS_VCVARS%"=="" set "VS_VCVARS=C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvars64.bat"
if "%MAYA_INCLUDE%"=="" set "MAYA_INCLUDE=C:\Program Files\Autodesk\Maya2024\include"
if "%MAYA_LIB%"=="" set "MAYA_LIB=C:\Program Files\Autodesk\Maya2024\lib"
if "%QT_INCLUDE%"=="" set "QT_INCLUDE=C:\Program Files\Autodesk\Maya2024\include\qt_5.15.2_vc14-include"
if "%QT_SVG_INCLUDE%"=="" set "QT_SVG_INCLUDE=%QT_INCLUDE%"
if "%QT_LIB%"=="" set "QT_LIB=C:\Program Files\Autodesk\Maya2024\lib"

for %%V in (2027 2026 2025 2024 2023 2022 2021 2020 2019 2018 2017 2016 2015 2014 2013 2012) do (
    echo %MAYA_INCLUDE% | findstr /i "Maya%%V" >nul && set "MAYA_VERSION=%%V"
)

set "BIN_DIR=%BIN_ROOT%\%MAYA_VERSION%"
set "BUILD_DIR=%BUILD_ROOT%\%MAYA_VERSION%"
set "TEMP_MLL=%BUILD_DIR%\oneLiner.mll"

if not defined VisualStudioVersion (
    call "%VS_VCVARS%"
    if errorlevel 1 exit /b 1
)

if not exist "%MAYA_INCLUDE%" (
    echo [ERROR] MAYA_INCLUDE not found: "%MAYA_INCLUDE%"
    exit /b 1
)

if not exist "%SOURCE_DIR%" (
    echo [ERROR] SOURCE_DIR not found: "%SOURCE_DIR%"
    exit /b 1
)

if not exist "%BIN_DIR%" mkdir "%BIN_DIR%"
if not exist "%BUILD_DIR%" mkdir "%BUILD_DIR%"

set "CORE_DIR=%ONEQT_DIR%\Core"
set "WIDGETS_DIR=%ONEQT_DIR%\Widgets"
set "CPPFLAGS=/nologo /std:c++17 /utf-8 /EHsc /MD /W4 /Zc:__cplusplus /DWIN32 /D_WINDOWS /DUNICODE /D_UNICODE /DNT_PLUGIN /DQT_NO_KEYWORDS /DQT_DEPRECATED_WARNINGS /DONETWIDGETS_STATIC /I"%SOURCE_DIR%" /I"%CORE_DIR%" /I"%CORE_DIR%\OneAnimIcon" /I"%CORE_DIR%\OneBackground" /I"%CORE_DIR%\OneBrush" /I"%CORE_DIR%\OneFont" /I"%CORE_DIR%\OneIcon" /I"%CORE_DIR%\Internal" /I"%WIDGETS_DIR%" /I"%WIDGETS_DIR%\OneAction" /I"%WIDGETS_DIR%\OneList" /I"%WIDGETS_DIR%\OneMenu" /I"%WIDGETS_DIR%\OneScrollBar" /I"%WIDGETS_DIR%\OneSeparator" /I"%MAYA_INCLUDE%" /I"%QT_INCLUDE%" /I"%QT_INCLUDE%\QtCore" /I"%QT_INCLUDE%\QtGui" /I"%QT_INCLUDE%\QtWidgets" /I"%QT_SVG_INCLUDE%\QtSvg""
set "LDFLAGS=/NOLOGO /DLL /INCREMENTAL:NO /MACHINE:X64 /LIBPATH:"%MAYA_LIB%" /LIBPATH:"%QT_LIB%" Foundation.lib OpenMaya.lib OpenMayaUI.lib Qt5Core.lib Qt5Gui.lib Qt5Widgets.lib Qt5Svg.lib user32.lib gdi32.lib shell32.lib advapi32.lib imm32.lib"

pushd "%BUILD_DIR%"
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
    "%SOURCE_DIR%\one_liner_engine.cpp" ^
    "%SOURCE_DIR%\one_liner_help.cpp" ^
    "%SOURCE_DIR%\one_liner_ui.cpp" ^
    "%SOURCE_DIR%\one_liner_plugin.cpp"
if errorlevel 1 (
    popd
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
    one_liner_engine.obj ^
    one_liner_help.obj ^
    one_liner_ui.obj ^
    one_liner_plugin.obj
if errorlevel 1 (
    popd
    exit /b 1
)

del /q "%BIN_DIR%\oneLiner.exp" "%BIN_DIR%\oneLiner.lib" "%BIN_DIR%\oneLiner.pdb" "%BIN_DIR%\oneLiner.ilk" 2>nul
del /q "%BIN_ROOT%\oneLiner.exp" "%BIN_ROOT%\oneLiner.lib" "%BIN_ROOT%\oneLiner.pdb" "%BIN_ROOT%\oneLiner.ilk" 2>nul

copy /y "%TEMP_MLL%" "%BIN_DIR%\oneLiner.mll" >nul
if errorlevel 1 (
    echo [WARN] Built temporary mll but failed to overwrite "%BIN_DIR%\oneLiner.mll". The file is likely locked by Maya.
    echo [WARN] New build is available at "%TEMP_MLL%"
    popd
    exit /b 0
)

del /q *.exp *.lib *.pdb *.ilk *.obj *.mll 2>nul
popd
rmdir /s /q "%BUILD_DIR%" 2>nul
rd /q "%BUILD_ROOT%" 2>nul
echo [OK] Built oneLiner.mll: "%BIN_DIR%\oneLiner.mll"
exit /b 0