TARGET      := hdr

BUILD_DIR   := ./build
SRC_DIR     := ./src
INC_DIR     := ./inc
OBJ_DIR     := $(BUILD_DIR)/obj
BIN_DIR     := $(BUILD_DIR)/bin

SRCS        :=                               \
   $(wildcard $(SRC_DIR)/data/*.cpp)         \
   $(wildcard $(SRC_DIR)/alignment/*.cpp)    \
   $(wildcard $(SRC_DIR)/hdr/*.cpp)          \
   $(wildcard $(SRC_DIR)/tone_mapping/*.cpp) \
   $(wildcard $(SRC_DIR)/*.cpp)
OBJS        := $(patsubst $(SRC_DIR)/%.cpp,$(OBJ_DIR)/%.o,$(SRCS))

CXX         := g++
CXXFLAGS    := -Wall -Wextra -std=c++17
INCLUDE     := -I$(INC_DIR) $(shell pkg-config --cflags opencv4)
LDFLAGS     := -L/usr/lib $(shell pkg-config --libs opencv4)

.PHONY: all build clean debug release install run

all: release

$(BIN_DIR)/$(TARGET): $(OBJS)
	$(CXX) -o $@ $^ $(LDFLAGS)

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp
	@mkdir -p $(@D)
	$(CXX) $(CXXFLAGS) $(INCLUDE) -o $@ -c $<

build: $(BIN_DIR)/$(TARGET)
	@mkdir -p $(BIN_DIR)
	@mkdir -p $(OBJ_DIR)

debug: CXXFLAGS += -DDEBUG -g
debug: all

release: CXXFLAGS += -O2
release: clean build install

clean:
	-@rm -rvf $(OBJ_DIR)/*
	-@rm -rvf $(BIN_DIR)/*
	-@rm -vf ./$(TARGET)

install:
	@mv $(BIN_DIR)/$(TARGET) .

run:
	./$(TARGET)
