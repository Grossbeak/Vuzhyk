# Скрипт CMake для копирования установленного Python в папку рантайма приложения
# Требуемые переменные:
#   PYTHON_EXECUTABLE — путь к системному python.exe
#   VUZHYK_DEST_DIR   — куда копировать (например, <build>/runtime/python)

if(NOT DEFINED PYTHON_EXECUTABLE)
  message(FATAL_ERROR "PYTHON_EXECUTABLE is not defined")
endif()

if(NOT DEFINED VUZHYK_DEST_DIR)
  message(FATAL_ERROR "VUZHYK_DEST_DIR is not defined")
endif()

message(STATUS "Bundling Python from: ${PYTHON_EXECUTABLE}")

execute_process(
  COMMAND "${PYTHON_EXECUTABLE}" -c "import sys; print(sys.base_prefix)"
  OUTPUT_VARIABLE PY_BASE_PREFIX
  OUTPUT_STRIP_TRAILING_WHITESPACE
  ERROR_QUIET
)

if(NOT PY_BASE_PREFIX)
  message(FATAL_ERROR "Failed to query sys.base_prefix from Python")
endif()

set(PY_ROOT "${PY_BASE_PREFIX}")
set(PY_DLLS_DIR "${PY_ROOT}/DLLs")
set(PY_LIB_DIR  "${PY_ROOT}/Lib")

file(MAKE_DIRECTORY "${VUZHYK_DEST_DIR}")

# Копируем исполняемые файлы интерпретатора, если существуют
foreach(pyexe IN ITEMS "python.exe" "pythonw.exe")
  if(EXISTS "${PY_ROOT}/${pyexe}")
    file(COPY "${PY_ROOT}/${pyexe}" DESTINATION "${VUZHYK_DEST_DIR}")
  endif()
endforeach()

# Копируем основные DLL (имя зависит от версии Python)
set(PY_DLL_CANDIDATES
  "python38.dll"
  "python39.dll"
  "python310.dll"
  "python311.dll"
  "python312.dll"
  "python3.dll"
)

foreach(dllname IN LISTS PY_DLL_CANDIDATES)
  if(EXISTS "${PY_ROOT}/${dllname}")
    file(COPY "${PY_ROOT}/${dllname}" DESTINATION "${VUZHYK_DEST_DIR}")
  endif()
endforeach()

# Библиотеки и DLLs каталоги (для стандартной библиотеки и расширений)
if(EXISTS "${PY_LIB_DIR}")
  file(COPY "${PY_LIB_DIR}" DESTINATION "${VUZHYK_DEST_DIR}")
else()
  message(WARNING "Python Lib directory not found: ${PY_LIB_DIR}")
endif()

if(EXISTS "${PY_DLLS_DIR}")
  file(COPY "${PY_DLLS_DIR}" DESTINATION "${VUZHYK_DEST_DIR}")
else()
  message(WARNING "Python DLLs directory not found: ${PY_DLLS_DIR}")
endif()

message(STATUS "Python bundled into: ${VUZHYK_DEST_DIR}")



