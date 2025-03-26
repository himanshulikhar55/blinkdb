# Compiler
CXX = g++

# Compiler flags
CXXFLAGS = -Wall -Wextra -std=c++17 -Ilib -MMD -MP -Wno-reorder -Wno-unused-parameter

# Directories
OBJ_DIR = obj

# Source and object files
SRC_FILES = $(wildcard *.cpp)          # Look for .cpp files in the current directory
OBJ_FILES = $(patsubst %.cpp, $(OBJ_DIR)/%.o, $(SRC_FILES))
DEP_FILES = $(OBJ_FILES:.o=.d)

# Output binary
TARGET = client  # Change this if you want a different executable name

# Ensure obj directory exists
$(shell mkdir -p $(OBJ_DIR))

# Debugging output to check if source files are detected
$(info Found source files: $(SRC_FILES))

# Default rule
all: $(TARGET)

# Linking step
$(TARGET): $(OBJ_FILES)
	$(CXX) $(OBJ_FILES) -o $(TARGET)

# Compiling source files
$(OBJ_DIR)/%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Include dependency files if they exist
-include $(DEP_FILES)

# Clean rule
clean:
	rm -rf $(OBJ_DIR) $(OBJ_DIR) $(TARGET) debug.log

