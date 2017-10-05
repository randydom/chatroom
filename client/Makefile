BIN_NAME := chatroom_client
CXX ?= g++
SRC_EXT = cpp
CLIENT_SRC_PATH = source
COMMON_SRC_PATH = ../common/source

COMPILE_FLAGS = -std=c++11 -Wall -Wextra -Wno-missing-field-initializers -g
RCOMPILE_FLAGS = -D NDEBUG
DCOMPILE_FLAGS = -D DEBUG
INCLUDES = -I ../common/header -I header/
LINK_FLAGS = -lpthread
RLINK_FLAGS =
DLINK_FLAGS =

release: export CXXFLAGS := $(CXXFLAGS) $(COMPILE_FLAGS) $(RCOMPILE_FLAGS)
release: export LDFLAGS := $(LDFLAGS) $(LINK_FLAGS) $(RLINK_FLAGS)
debug: export CXXFLAGS := $(CXXFLAGS) $(COMPILE_FLAGS) $(DCOMPILE_FLAGS)
debug: export LDFLAGS := $(LDFLAGS) $(LINK_FLAGS) $(DLINK_FLAGS)

release: export BUILD_PATH := build/release
release: export BIN_PATH := bin/release
debug: export BUILD_PATH := build/debug
debug: export BIN_PATH := bin/debug

COMMON_SOURCES = $(shell find $(COMMON_SRC_PATH) -name '*.$(SRC_EXT)')
COMMON_OBJECTS = $(COMMON_SOURCES:$(COMMON_SRC_PATH)/%.$(SRC_EXT)=../$(BUILD_PATH)/common/%.o)

CLIENT_SOURCES = $(shell find $(CLIENT_SRC_PATH) -name '*.$(SRC_EXT)')
CLIENT_OBJECTS = $(CLIENT_SOURCES:$(CLIENT_SRC_PATH)/%.$(SRC_EXT)=$(BUILD_PATH)/%.o)
DEPS = $(COMMON_OBJECTS:.o=.d) $(CLIENT_OBJECTS:.o=.d)

.PHONY: release
release: dirs
	@$(MAKE) all --no-print-directory

.PHONY: debug
debug: dirs
	@$(MAKE) all --no-print-directory

.PHONY: dirs
dirs:
	@mkdir -p $(dir $(COMMON_OBJECTS))
	@mkdir -p $(dir $(CLIENT_OBJECTS))
	@mkdir -p $(BIN_PATH)

.PHONY: clean
clean:
	@$(RM) $(BIN_NAME)
	@$(RM) -r build
	@$(RM) -r bin
	@$(RM) -r ../build

all: $(BIN_PATH)/$(BIN_NAME)
	@$(RM) $(BIN_NAME)
	@ln -s $(BIN_PATH)/$(BIN_NAME) $(BIN_NAME)

$(BIN_PATH)/$(BIN_NAME): $(COMMON_OBJECTS) $(CLIENT_OBJECTS)
	$(CXX) $(COMMON_OBJECTS) $(CLIENT_OBJECTS) $(LDFLAGS) -o $@

../$(BUILD_PATH)/common/%.o: $(COMMON_SRC_PATH)/%.$(SRC_EXT)
	$(CXX) $(CXXFLAGS) $(INCLUDES) -MP -MMD -c $< -o $@

-include $(DEPS)

$(BUILD_PATH)/%.o: $(CLIENT_SRC_PATH)/%.$(SRC_EXT)
	$(CXX) $(CXXFLAGS) $(INCLUDES) -MP -MMD -c $< -o $@
