BIN_DIR = bin
OBJ_DIR = obj
SRC_DIR = src

SRCS := $(notdir $(wildcard $(SRC_DIR)/*.cpp))
OBJS := $(SRCS:%.cpp=$(OBJ_DIR)/%.o)
DEPS := $(OBJS:%.o=%.d)

_SRCS := $(SRC_DIR)/%.cpp
_OBJS := $(OBJ_DIR)/%.o

CXX = g++
CXXFLAGS = -std=c++17 -g3 -O0 -Wall -I$(SRC_DIR)
LDFLAGS = -lstdc++ -lconfig++

TARGET = $(BIN_DIR)/conf_test

.PHONY: all

all: $(TARGET)
	
$(TARGET): $(OBJS)
	@mkdir -p $(BIN_DIR)
	$(CXX) -o $(TARGET) $^ $(LDFLAGS)

-include $(DEPS)

$(_OBJS): $(_SRCS)
	@mkdir -p $(OBJ_DIR)
	$(CXX) -c $(CXXFLAGS) -MMD $< -o $@

.PHONY: clean
clean:
	@echo "cleaning up ..."
	@rm -rf $(TARGET) $(OBJ_DIR) $(BIN_DIR)

install:
	@echo "install"

unittest:
	@echo "unit test"
