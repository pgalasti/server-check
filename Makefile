CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -I./include -I/usr/local/include
LDFLAGS = -lssh -lncurses -pthread -L/usr/local/lib -lg-lib -Wl,-rpath,/usr/local/lib

SRC_DIR = src
INC_DIR = include
OBJ_DIR = obj
BIN_DIR = build

TARGET = $(BIN_DIR)/server-check

SOURCES = $(wildcard $(SRC_DIR)/*.cpp)
OBJECTS = $(patsubst $(SRC_DIR)/%.cpp,$(OBJ_DIR)/%.o,$(SOURCES))

all: directories $(TARGET)

directories:
	@mkdir -p $(OBJ_DIR) $(BIN_DIR)

$(TARGET): $(OBJECTS)
	$(CXX) $(OBJECTS) -o $@ $(LDFLAGS)

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -rf $(OBJ_DIR) $(BIN_DIR)

install: $(TARGET)
	install -d /usr/local/bin
	install -m 755 $(TARGET) /usr/local/bin/server-check

.PHONY: all directories clean install
