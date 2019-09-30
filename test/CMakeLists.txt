add_executable(smacpptest ../thirdparty/catch.hpp
  main.cpp
  test_plugin_loading.cpp
  )

target_include_directories(smacpptest PRIVATE .)
target_link_libraries(smacpptest PRIVATE smacppcommon ${CMAKE_THREAD_LIBS_INIT})
add_dependencies(smacpptest smacpp-clang-plugin smacpp)


# add_test(NAME catch_tests COMMAND smacpptest
#   CONFIGURATIONS ALL
#   WORKING_DIRECTORY ${CMAKE_BINARY_DIR})

add_custom_target(check COMMAND smacpptest
  WORKING_DIRECTORY "${PROJECT_BINARY_DIR}")

# This might only work with quite new cmake
add_custom_target(test COMMAND smacpptest
  WORKING_DIRECTORY "${PROJECT_BINARY_DIR}")
