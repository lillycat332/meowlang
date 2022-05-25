CC := clang++
CCVERSION := $(shell $(CC) -dumpversion)
CPPFLAGS :=-Wshadow -Wall -Wextra -Wpedantic -Wstrict-overflow -Wfatal-errors -fno-strict-aliasing -Wno-nullability-completeness -Wno-nullability-extension -march=native -lstdc++ -lc++ -undefined dynamic_lookup -std=c++20
LLVMFLAGS := $(shell llvm-config --cxxflags --ldflags --libs all --system-libs)
BUILD_DIR := build
SRC := src/

all: lang lib

lang: |$(BUILD_DIR)
	@echo -n 'building meowlang compiler with: '
	@$(CC) --version | sed 1q
	$(CC) $(SRC)/meowlang/main.cpp $(CPPFLAGS) $(LLVMFLAGS) -o $(BUILD_DIR)/meowc

lib: |$(BUILD_DIR)
	@echo -n 'building meowlang standard library with: '
	@$(CC) --version | sed 1q
	$(CC) $(SRC)/libmeow/libmeow.cpp $(CPPFLAGS) -c -o $(BUILD_DIR)/libmeow.o

clean: |$(BUILD_DIR)
	@rm -rf $(BUILD_DIR)

$(BUILD_DIR):
	@echo "Folder $(BUILD_DIR) does not exist, creating it..."
	mkdir -p $@

