@echo off
REM Скрипт для генерации MOC файлов для Qt проекта
REM Использование: generate_moc.bat [путь_к_Qt]

setlocal

if "%1"=="" (
    if defined QTDIR (
        set QT_PATH=%QTDIR%
    ) else (
        echo ОШИБКА: Укажите путь к Qt или установите переменную QTDIR
        echo Использование: generate_moc.bat "C:\Qt\Qt5.12.12\5.12.12\msvc2017_64"
        exit /b 1
    )
) else (
    set QT_PATH=%1
)

set MOC_EXE=%QT_PATH%\bin\moc.exe
set SRC_DIR=src
set OUT_DIR=moc_generated

if not exist "%MOC_EXE%" (
    echo ОШИБКА: MOC не найден по пути %MOC_EXE%
    exit /b 1
)

echo Генерация MOC файлов...
echo Qt путь: %QT_PATH%
echo.

if not exist "%OUT_DIR%" mkdir "%OUT_DIR%"

REM Список заголовочных файлов с Q_OBJECT
"%MOC_EXE%" -o "%OUT_DIR%\moc_MainWindow.cpp" "%SRC_DIR%\MainWindow.h"
"%MOC_EXE%" -o "%OUT_DIR%\moc_CodeEditor.cpp" "%SRC_DIR%\CodeEditor.h"
"%MOC_EXE%" -o "%OUT_DIR%\moc_TitleBar.cpp" "%SRC_DIR%\TitleBar.h"
"%MOC_EXE%" -o "%OUT_DIR%\moc_AnimatedMenu.cpp" "%SRC_DIR%\AnimatedMenu.h"
"%MOC_EXE%" -o "%OUT_DIR%\moc_ConsoleWidget.cpp" "%SRC_DIR%\ConsoleWidget.h"
"%MOC_EXE%" -o "%OUT_DIR%\moc_WindowFrameOverlay.cpp" "%SRC_DIR%\WindowFrameOverlay.h"
"%MOC_EXE%" -o "%OUT_DIR%\moc_SettingsWidget.cpp" "%SRC_DIR%\SettingsWidget.h"
"%MOC_EXE%" -o "%OUT_DIR%\moc_HelpWidget.cpp" "%SRC_DIR%\HelpWidget.h"
"%MOC_EXE%" -o "%OUT_DIR%\moc_PythonHighlighter.cpp" "%SRC_DIR%\PythonHighlighter.h"
"%MOC_EXE%" -o "%OUT_DIR%\moc_PyrobEditorWidget.cpp" "%SRC_DIR%\pyrobeditor\PyrobEditorWidget.h"
"%MOC_EXE%" -o "%OUT_DIR%\moc_grideditor.cpp" "%SRC_DIR%\pyrobeditor\grideditor.h"
"%MOC_EXE%" -o "%OUT_DIR%\moc_projectmodel.cpp" "%SRC_DIR%\pyrobeditor\projectmodel.h"
"%MOC_EXE%" -o "%OUT_DIR%\moc_SnakeGame.cpp" "%SRC_DIR%\sea\SnakeGame.h"

echo.
echo MOC файлы сгенерированы в папке: %OUT_DIR%
echo Теперь добавьте эти файлы в проект Visual Studio.

endlocal



