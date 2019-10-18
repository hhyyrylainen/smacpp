# Some helpful targets
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
	build/src/smacpp -I test/data/JM2018TS/strings/overflow test/data/JM2018TS/strings/overflow/test_incorrect/01_simple_if.c -o /dev/null

analyzer_plugin_run: compile
	clang --analyze -I test/data/JM2018TS/strings/overflow test/data/JM2018TS/strings/overflow/test_incorrect/01_simple_if.c -o /dev/null -Xclang -load -Xclang build/src/libsmacpp-clang-analyzer.so -Xclang -analyzer-checker=smacpp.All

clang_ast_view:
	clang -I test/data/JM2018TS/strings/overflow test/data/JM2018TS/strings/overflow/test_incorrect/01_simple_if.c -Xclang -ast-dump -fsyntax-only


.PHONY: clang_plugin_run analyzer_plugin_run cmake compile test
