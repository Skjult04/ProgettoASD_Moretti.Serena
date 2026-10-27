CXX      := g++
CXXFLAGS := -std=c++17 -O2 -Wall -Wextra -Iinclude -MMD -MP
BUILD    := build
TARGET   := asd_project
TEST     := test_example

SRC      := $(wildcard src/*.cpp)
OBJ      := $(SRC:src/%.cpp=$(BUILD)/%.o)
DEP      := $(OBJ:.o=.d)
LIB_OBJ  := $(filter-out $(BUILD)/main.o,$(OBJ))

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CXX) $(OBJ) -o $@

$(BUILD)/%.o: src/%.cpp
	@mkdir -p $(BUILD)
	$(CXX) $(CXXFLAGS) -c $< -o $@

# test: stessi moduli del programma, ma senza il suo main()
$(TEST): tests/test_example.cpp $(LIB_OBJ)
	$(CXX) $(CXXFLAGS) $^ -o $@

test: $(TEST)
	./$(TEST)

clean:
	rm -rf $(BUILD) $(TARGET) $(TARGET).exe $(TEST) $(TEST).exe *.d

-include $(DEP)

.PHONY: all test clean