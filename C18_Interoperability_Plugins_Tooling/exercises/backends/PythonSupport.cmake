include_guard(GLOBAL)

function(c18_python310_paths out_exe out_include out_libdir)
  execute_process(
    COMMAND "${Python3_EXECUTABLE}" -c "import sys,sysconfig,pathlib; print(sys.executable); print(sysconfig.get_path('include')); print(pathlib.Path(sys.executable).resolve().parent/'libs')"
    OUTPUT_VARIABLE c18_python_info
    OUTPUT_STRIP_TRAILING_WHITESPACE
    COMMAND_ERROR_IS_FATAL ANY)
  string(REPLACE "\n" ";" c18_python_info "${c18_python_info}")
  list(GET c18_python_info 0 c18_py_exe)
  list(GET c18_python_info 1 c18_py_include)
  list(GET c18_python_info 2 c18_py_libdir)
  execute_process(
    COMMAND "${c18_py_exe}" -c "import importlib.util,pathlib,sys; assert sys.version_info[:3] == (3, 10, 11), sys.version; assert importlib.util.find_spec('importlib') is not None; assert (pathlib.Path(r'${c18_py_include}')/'Python.h').is_file(); assert (pathlib.Path(r'${c18_py_libdir}')/'python310.lib').is_file() if sys.platform == 'win32' else True"
    COMMAND_ERROR_IS_FATAL ANY)
  set(${out_exe} "${c18_py_exe}" PARENT_SCOPE)
  set(${out_include} "${c18_py_include}" PARENT_SCOPE)
  set(${out_libdir} "${c18_py_libdir}" PARENT_SCOPE)
endfunction()

function(c18_python38_paths out_exe out_include out_libdir)
  if(WIN32)
    set(c18_py38_cmd py -3.8)
  else()
    set(c18_py38_cmd python3.8)
  endif()
  execute_process(
    COMMAND ${c18_py38_cmd} -c "import sys,sysconfig,pathlib; print(sys.executable); print(sysconfig.get_path('include')); print(pathlib.Path(sys.executable).resolve().parent/'libs')"
    OUTPUT_VARIABLE c18_python_info
    OUTPUT_STRIP_TRAILING_WHITESPACE
    COMMAND_ERROR_IS_FATAL ANY)
  string(REPLACE "\n" ";" c18_python_info "${c18_python_info}")
  list(GET c18_python_info 0 c18_py_exe)
  list(GET c18_python_info 1 c18_py_include)
  list(GET c18_python_info 2 c18_py_libdir)
  execute_process(
    COMMAND "${c18_py_exe}" -c "import importlib.util,pathlib,sys; assert sys.version_info[:3] == (3, 8, 10), sys.version; assert importlib.util.find_spec('importlib') is not None; assert (pathlib.Path(r'${c18_py_include}')/'Python.h').is_file(); assert (pathlib.Path(r'${c18_py_libdir}')/'python3.lib').is_file() if sys.platform == 'win32' else True"
    COMMAND_ERROR_IS_FATAL ANY)
  set(${out_exe} "${c18_py_exe}" PARENT_SCOPE)
  set(${out_include} "${c18_py_include}" PARENT_SCOPE)
  set(${out_libdir} "${c18_py_libdir}" PARENT_SCOPE)
endfunction()

function(c18_python_extension target)
  c18_python310_paths(c18_py_exe c18_py_include c18_py_libdir)
  add_library(${target} MODULE ${ARGN})
  c18_target(${target})
  target_include_directories(${target} PRIVATE "${c18_py_include}")
  if(WIN32)
    target_link_libraries(${target} PRIVATE "${c18_py_libdir}/python310.lib")
    set_target_properties(${target} PROPERTIES PREFIX "" SUFFIX ".pyd")
  else()
    set_target_properties(${target} PROPERTIES PREFIX "" SUFFIX ".so")
  endif()
endfunction()

function(c18_limited_extension target)
  c18_python310_paths(c18_py310_exe c18_py310_include c18_py310_libdir)
  c18_python38_paths(c18_py38_exe c18_py38_include c18_py38_libdir)
  add_library(${target} MODULE ${ARGN})
  c18_target(${target})
  target_compile_definitions(${target} PRIVATE Py_LIMITED_API=0x03080000)
  target_include_directories(${target} PRIVATE "${c18_py38_include}")
  if(WIN32)
    target_link_libraries(${target} PRIVATE "${c18_py38_libdir}/python3.lib")
    set_target_properties(${target} PROPERTIES PREFIX "" SUFFIX ".pyd")
  else()
    set_target_properties(${target} PROPERTIES PREFIX "" SUFFIX ".abi3.so")
  endif()
  set(${target}_PY38_EXECUTABLE "${c18_py38_exe}" PARENT_SCOPE)
endfunction()
