
# if(UNIX)
#   set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -Wl,--no-undefined")
# endif()

add_library(smacppcommon
  parse/CodeBlock.h
  parse/CodeBlock.cpp
  parse/Variable.h
  parse/Variable.cpp  
  parse/Condition.h
  parse/Condition.cpp
  parse/ProcessedAction.h
  parse/ProcessedAction.cpp
  parse/ClangFrontendAction.h
  parse/ClangFrontendAction.cpp  
  parse/ClangASTAction.h
  parse/CodeBlockBuildingVisitor.h
  parse/CodeBlockBuildingVisitor.cpp
  parse/MainASTConsumer.h
  parse/MainASTConsumer.cpp
  parse/LiteralStateVisitor.h
  parse/ComplexExpressionParser.h
  integration/SMACPPFinder.h
  integration/SMACPPFinder.cpp
  analysis/BlockRegistry.h
  analysis/BlockRegistry.cpp
  analysis/Analyzer.h
  analysis/Analyzer.cpp
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

set_target_properties(smacppcommon PROPERTIES
  CXX_STANDARD 17
  CXX_EXTENSIONS OFF
  POSITION_INDEPENDENT_CODE ON  
  )

target_include_directories(smacppcommon PUBLIC ${CLANG_INCLUDE_DIRS})
target_include_directories(smacppcommon PUBLIC ${CMAKE_CURRENT_LIST_DIR})

add_library(smacpp-clang-plugin SHARED
  integration/ClangPlugin.cpp
  )

target_link_libraries(smacpp-clang-plugin PRIVATE smacppcommon)

set_target_properties(smacpp-clang-plugin PROPERTIES
  CXX_STANDARD 17
  CXX_EXTENSIONS OFF
  )

install(TARGETS smacpp-clang-plugin)


add_library(smacpp-clang-analyzer SHARED
  integration/AnalyzerPlugin.cpp
  integration/SMACPPChecker.cpp
  )

target_link_libraries(smacpp-clang-analyzer PRIVATE smacppcommon)

set_target_properties(smacpp-clang-analyzer PROPERTIES
  CXX_STANDARD 17
  CXX_EXTENSIONS OFF
  )

install(TARGETS smacpp-clang-analyzer)

if(UNIX)
  add_executable(smacpp main.cpp)

  target_link_libraries(smacpp PRIVATE smacppcommon)

  set_target_properties(smacpp PROPERTIES
    CXX_STANDARD 17
    CXX_EXTENSIONS OFF
    )

  install(TARGETS smacpp)

  # Same code a second time, just to easily have two programs with different names
  add_executable(smacpp++ main.cpp)

  target_link_libraries(smacpp++ PRIVATE smacppcommon)

  set_target_properties(smacpp++ PROPERTIES
    CXX_STANDARD 17
    CXX_EXTENSIONS OFF
    )

  install(TARGETS smacpp++)
endif()
