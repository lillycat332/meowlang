CC := clang++
CCVERSION :=$(shell $(CC) -dumpversion)
CFLAGS :=-Wshadow -Wall -Wextra -Wpedantic -Wstrict-overflow -Wfatal-errors -Wno-unused-function -fno-strict-aliasing -Wno-nullability-completeness -Wno-nullability-extension
BUILD_DIR := build
SRCS := src/main.cpp

all: lang

lang: |$(BUILD_DIR)
	@echo -n 'building meowlang with: '
	@$(CC) --version | sed 1q
	$(CC) $(SRCS) $(CFLAGS) `llvm-config --cxxflags --ldflags --libs` -o $(BUILD_DIR)/meowi 

$(BUILD_DIR):
	@echo "Folder $(BUILD_DIR) does not exist, creating it..."
	mkdir -p $@

