# Some helpful targets
SMACPP=build/src/smacpp
DEBUG_ARGS=-Xclang -plugin-arg-smacpp -Xclang -smacpp-debug -o /dev/null
OVERFLOW_FOLDER=test/data/JM2018TS/strings/overflow
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

clang_ast_view:
	clang $(AST) -I $(OVERFLOW_FOLDER) $(OVERFLOW_FOLDER)//test_incorrect/01_simple_if.c

ast_overflow_2:
	clang $(AST) -I $(OVERFLOW_FOLDER) $(OVERFLOW_FOLDER)//test_incorrect/02_simple_if_int1.c

ast_overflow_4:
	clang $(AST) -I $(OVERFLOW_FOLDER) $(OVERFLOW_FOLDER)//test_incorrect/04_simple_switch.c


.PHONY: clang_plugin_run analyzer_plugin_run cmake compile test
