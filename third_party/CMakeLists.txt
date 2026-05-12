# Macro needed by libpg_query
macro(disable_target_warnings TARGET_NAME)
  if(MSVC)
    target_compile_options(${TARGET_NAME} PRIVATE /W0)
  else()
    target_compile_options(${TARGET_NAME} PRIVATE -w)
  endif()
endmacro()

add_subdirectory(libpg_query)
