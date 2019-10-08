
add_library(smacppcommon
  parse/CodeBlock.h
  parse/CodeBlock.cpp
  parse/ClangFrontendAction.h
  parse/ClangASTAction.h  
  parse/CodeBlockBuildingVisitor.h
  parse/MainASTConsumer.h
  integration/SMACPPFinder.h
  integration/SMACPPFinder.cpp  
  )

target_link_libraries(smacppcommon PUBLIC
  clangFrontend
  clangParse
  clangSema
  clangAST
  clangBasic
  clangLex
  clangTooling
  clangSerialization
  ${Boost_LIBRARIES}
  )

target_include_directories(smacppcommon PUBLIC ${CLANG_INCLUDE_DIRS})
target_include_directories(smacppcommon PUBLIC ${CMAKE_CURRENT_LIST_DIR})

add_library(smacpp-clang-plugin SHARED
  integration/ClangPlugin.cpp
  )

target_link_libraries(smacpp-clang-plugin PRIVATE smacppcommon)

install(TARGETS smacpp-clang-plugin)


add_library(smacpp-clang-analyzer SHARED
  integration/AnalyzerPlugin.cpp
  integration/SMACPPChecker.cpp  
  )

target_link_libraries(smacpp-clang-analyzer PRIVATE smacppcommon)

install(TARGETS smacpp-clang-analyzer)

if(UNIX)
  add_executable(smacpp main.cpp)

  target_link_libraries(smacpp PRIVATE smacppcommon)

  install(TARGETS smacpp)

  # Same code a second time, just to easily have two programs with different names
  add_executable(smacpp++ main.cpp)

  target_link_libraries(smacpp++ PRIVATE smacppcommon)

  install(TARGETS smacpp++)
endif()

