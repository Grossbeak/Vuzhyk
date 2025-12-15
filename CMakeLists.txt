cmake_minimum_required(VERSION 3.15)

project(Vuzhyk LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# Qt5 is required for Windows 7 support (Qt6 не поддерживает Win7)
find_package(Qt5 5.12 REQUIRED COMPONENTS Widgets Svg Network)

# Найти установленный Python (будет использован для бандлинга рантайма)
find_package(Python3 REQUIRED COMPONENTS Interpreter)

set(SRC_DIR ${CMAKE_CURRENT_LIST_DIR}/src)

add_executable(Vuzhyk WIN32
  ${SRC_DIR}/main.cpp
  ${SRC_DIR}/MainWindow.cpp
  ${SRC_DIR}/MainWindow.h
  ${SRC_DIR}/PythonHighlighter.cpp
  ${SRC_DIR}/PythonHighlighter.h
  ${SRC_DIR}/CodeEditor.cpp
  ${SRC_DIR}/CodeEditor.h
  ${SRC_DIR}/TitleBar.cpp
  ${SRC_DIR}/TitleBar.h
  ${SRC_DIR}/AnimatedMenu.cpp
  ${SRC_DIR}/AnimatedMenu.h
  ${SRC_DIR}/ConsoleWidget.cpp
  ${SRC_DIR}/ConsoleWidget.h
  ${SRC_DIR}/WindowFrameOverlay.cpp
  ${SRC_DIR}/WindowFrameOverlay.h
  ${SRC_DIR}/SettingsWidget.cpp
  ${SRC_DIR}/SettingsWidget.h
  ${SRC_DIR}/HelpWidget.cpp
  ${SRC_DIR}/HelpWidget.h
  ${SRC_DIR}/pyrobeditor/PyrobEditorWidget.cpp
  ${SRC_DIR}/pyrobeditor/PyrobEditorWidget.h
  ${SRC_DIR}/pyrobeditor/grideditor.cpp
  ${SRC_DIR}/pyrobeditor/grideditor.h
  ${SRC_DIR}/pyrobeditor/projectmodel.cpp
  ${SRC_DIR}/pyrobeditor/projectmodel.h
  ${SRC_DIR}/sea/SnakeGame.cpp
  ${SRC_DIR}/sea/SnakeGame.h
  ${CMAKE_CURRENT_LIST_DIR}/resources.qrc
)

# Встраиваем иконку EXE через ресурс .rc (Windows)
if (WIN32)
  target_sources(Vuzhyk PRIVATE ${CMAKE_CURRENT_LIST_DIR}/app.rc)
  set_target_properties(Vuzhyk PROPERTIES WIN32_EXECUTABLE TRUE)
endif()

target_link_libraries(Vuzhyk PRIVATE Qt5::Widgets Qt5::Svg Qt5::Network)

# FilePicker - отдельный exe для выбора файлов (использует нативный Windows API)
add_executable(FilePicker WIN32
  ${SRC_DIR}/filepicker.cpp
)

# Для Windows используем COM библиотеки
if (WIN32)
  target_link_libraries(FilePicker PRIVATE ole32 shell32 comdlg32)
endif()

# Куда собирать встроенный Python
set(VUZHYK_PY_RUNTIME_DIR "${CMAKE_BINARY_DIR}/runtime/python")

# Цель для копирования Python из системы внутрь билда
add_custom_target(bundle_python ALL
  COMMAND ${CMAKE_COMMAND}
          -DPYTHON_EXECUTABLE=${Python3_EXECUTABLE}
          -DVUZHYK_DEST_DIR=${VUZHYK_PY_RUNTIME_DIR}
          -P ${CMAKE_CURRENT_LIST_DIR}/cmake/BundlePython.cmake
  COMMENT "Bundling Python runtime into ${VUZHYK_PY_RUNTIME_DIR}"
  VERBATIM
)

add_dependencies(Vuzhyk bundle_python)

# Установка: бинарь + папка с Python
install(TARGETS Vuzhyk FilePicker RUNTIME DESTINATION .)
install(DIRECTORY "${VUZHYK_PY_RUNTIME_DIR}/" DESTINATION "python")


