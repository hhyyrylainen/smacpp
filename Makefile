# Some helpful targets
SMACPP=build/src/smacpp
DEBUG_ARGS=-Xclang -plugin-arg-smacpp -Xclang -smacpp-debug -o /dev/null
OVERFLOW_FOLDER=test/data/JM2018TS/strings/overflow
JULIET_COMMON=test/data/Juliet_Test_Suite_v1.3_for_C_Cpp/C/testcasesupport
CWE126=test/data/Juliet_Test_Suite_v1.3_for_C_Cpp/C/testcases/CWE126_Buffer_Overread
JULIET_ARGS=-fsyntax-only -DINCLUDEMAIN -I $(JULIET_COMMON) $(DEBUG_ARGS)
AST=-Xclang -ast-dump -fsyntax-only

all: build cmake compile test

build:
	mkdir build

cmake:
	cd build && cmake ..

compile: cmake
	$(MAKE) -C build

test: cmake
	$(MAKE) -C build test

clang_plugin_run: compile
	$(SMACPP) $(DEBUG_ARGS) -I $(OVERFLOW_FOLDER) $(OVERFLOW_FOLDER)/test_incorrect/01_simple_if.c

analyzer_plugin_run: compile
	clang --analyze -I test/data/JM2018TS/strings/overflow test/data/JM2018TS/strings/overflow/test_incorrect/01_simple_if.c -o /dev/null -Xclang -load -Xclang build/src/libsmacpp-clang-analyzer.so -Xclang -analyzer-checker=smacpp.All

run_overflow_2: compile
	$(SMACPP) $(DEBUG_ARGS) -I $(OVERFLOW_FOLDER) $(OVERFLOW_FOLDER)/test_incorrect/02_simple_if_int1.c

run_overflow_2_2: compile
	$(SMACPP) $(DEBUG_ARGS) -I $(OVERFLOW_FOLDER) $(OVERFLOW_FOLDER)/test_incorrect/02_simple_if_int2.c

run_overflow_3: compile
	$(SMACPP) $(DEBUG_ARGS) -I $(OVERFLOW_FOLDER) $(OVERFLOW_FOLDER)/test_incorrect/03_simple_if_multi_func.c

run_overflow_4: compile
	$(SMACPP) $(DEBUG_ARGS) -I $(OVERFLOW_FOLDER) $(OVERFLOW_FOLDER)/test_incorrect/04_simple_switch.c

# Used for development (adding constant switch value detection)
run_CWE126_1: compile
	$(SMACPP) $(JULIET_ARGS) $(CWE126)/s01/CWE126_Buffer_Overread__CWE170_char_memcpy_15.c

clang_ast_view:
	clang $(AST) -I $(OVERFLOW_FOLDER) $(OVERFLOW_FOLDER)//test_incorrect/01_simple_if.c

ast_overflow_2:
	clang $(AST) -I $(OVERFLOW_FOLDER) $(OVERFLOW_FOLDER)//test_incorrect/02_simple_if_int1.c

ast_overflow_2_2:
	clang $(AST) -I $(OVERFLOW_FOLDER) $(OVERFLOW_FOLDER)//test_incorrect/02_simple_if_int2.c

ast_overflow_4:
	clang $(AST) -I $(OVERFLOW_FOLDER) $(OVERFLOW_FOLDER)//test_incorrect/04_simple_switch.c


.PHONY: clang_plugin_run analyzer_plugin_run cmake compile test
