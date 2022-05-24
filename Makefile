CC := clang++
CCVERSION := $(shell $(CC) -dumpversion)
CPPFLAGS :=-Wshadow -Wall -Wextra -Wpedantic -Wstrict-overflow -Wfatal-errors -fno-strict-aliasing -Wno-nullability-completeness -Wno-nullability-extension -rdynamic -lc++ -march=native -undefined dynamic_lookup
LLVMFLAGS := $(shell llvm-config --cxxflags --ldflags --libs all --system-libs)
BUILD_DIR := build
SRCS := src/main.cpp

all: lang

lang: |$(BUILD_DIR)
	@echo -n 'building meowlang with: '
	@$(CC) --version | sed 1q
	$(CC) $(SRCS) $(CPPFLAGS) $(LLVMFLAGS) -std=c++17 -o $(BUILD_DIR)/meowi 

$(BUILD_DIR):
	@echo "Folder $(BUILD_DIR) does not exist, creating it..."
	mkdir -p $@

