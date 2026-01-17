

INCLUDE = ./include
SRC   = ./src
BUILD = ./build
BIN   = $(BUILD)/bin
TARGET = ptxd

TESTS = ./tests
TESTS_BUILD = $(BUILD)/tests

CC    = g++
CFLAGS = -std=c++20 -O3 -g -I$(INCLUDE) -MMD -MP
LDFLAGS = -pthread

# Color Codes
Black        = \033[0;30m
DarkGray     = \033[1;30m
Red          = \033[0;31m
LightRed     = \033[1;31m
Green        = \033[0;32m
LightGreen   = \033[1;32m
Brown        = \033[0;33m
Yellow       = \033[1;33m
Blue         = \033[0;34m
LightBlue    = \033[1;34m
Purple       = \033[0;35m
LightPurple  = \033[1;35m
Cyan         = \033[0;36m
LightCyan    = \033[1;36m
LightGray    = \033[0;37m
White        = \033[1;37m
NC           = \033[0m

define logi
	@echo "$(Green)[*]$(1) $(NC)"
endef
define logs
	@echo "$(Green)[+]$(1) $(NC)"
endef
define loge
	@echo "$(Red)[-]$(1) $(NC)"
endef
define logw
	@echo "$(Yellow)[!]$(1) $(NC)"
endef

SRC_FILES = $(shell find $(SRC) -name "*.cpp" 2>/dev/null)
SRC_FILES := $(filter-out %/main.cpp, $(SRC_FILES))
MAIN_CPP = $(SRC)/main.cpp

INCLUDE_FILES = $(wildcard $(INCLUDE)/*.h) $(wildcard $(INCLUDE)/**/*.h) $(wildcard $(INCLUDE)/**/**/*.h)
DEPS_FILES = $(patsubst $(SRC)/%.cpp,$(BUILD)/%.d,$(SRC_FILES))
DEPS_FILES += $(patsubst $(SRC)/%.cpp,$(BUILD)/%.d,$(MAIN_CPP))

OBJ_FILES = $(patsubst $(SRC)/%.cpp,$(BUILD)/%.o,$(SRC_FILES))
MAIN_OBJ = $(patsubst $(SRC)/%.cpp,$(BUILD)/%.o,$(MAIN_CPP))

TEST_SRC_FILES = $(wildcard $(TESTS)/*.cpp)
TEST_OBJ_FILES = $(patsubst $(TESTS)/%.cpp,$(TESTS_BUILD)/%.o,$(TEST_SRC_FILES))
TEST_OUT_FILES = $(patsubst $(TESTS)/%.cpp,$(BIN)/%.out,$(TEST_SRC_FILES))
DEPS_FILES += $(patsubst $(TESTS)/%.cpp,$(TESTS_BUILD)/%.d,$(TEST_SRC_FILES))

SAMPLE_SRC_FILES = $(wildcard $(SAMPLES)/*.cpp)
SAMPLE_OUT_FILES = $(patsubst $(SAMPLES)/%.cpp,$(SAMPLES_BIN)/%,$(SAMPLE_SRC_FILES))

all: precheck $(BIN)/$(TARGET)

precheck:
	@mkdir -p $(BUILD)
	@mkdir -p $(BIN)
	@mkdir -p $(TESTS_BUILD)

run: $(BIN)/$(TARGET)
	@$(call logs, "Running Target...")
	@$(BIN)/$(TARGET)

test: $(TEST_OUT_FILES)
	@$(call logs, "Running Engine Tests...")
	@for testfile in $(TEST_OUT_FILES); do \
		echo "$(Green)[+] Running $$testfile $(NC)"; \
		timeout -k 2s 10s $$testfile; [ "$$?" -eq 124 ] && echo "$(Yellow)[-] Test timmedout! $(NC)"; \
		echo "$(Green)[+] Finished $$testfile $(NC)"; \
	done
	@echo "\n$(Green)[+] Done!!"

samples: $(SAMPLE_OUT_FILES)
	@true

%.test: $(BIN)/%.test.out
	@$(call logs, "Running Test $*...")
	@$(BIN)/$*.test.out
	@$(call logs, "Finished Test $*")

clean:
	@$(call logw, "Removing Build Folder")
	@rm -rf $(BUILD)

$(BIN)/$(TARGET): $(OBJ_FILES) $(MAIN_OBJ)
	@mkdir -p $(BIN)
	@$(call logi, "Building Target")
	$(CC) $(CFLAGS) $(OBJ_FILES) $(MAIN_OBJ) -o $@ $(LDFLAGS)

$(BIN)/%.out: $(TESTS_BUILD)/%.o $(OBJ_FILES)
	@$(call logi, "Building test $*")
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $< $(OBJ_FILES) -o $@ $(LDFLAGS)

$(TESTS_BUILD)/%.o: $(TESTS)/%.cpp
	@$(call logi, "Compiling TEST $*.cpp")
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $(TESTS)/$*.cpp -o $@

$(BUILD)/%.o: $(SRC)/%.cpp
	@$(call logi, "Compiling $*.cpp")
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $(SRC)/$*.cpp -o $@


-include ${DEPS_FILES}