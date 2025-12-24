CXX := clang++
CXXFLAGS := -Wall -Wextra -std=c++23 -ggdb
LDFLAGS := -fsanitize=address,undefined

SRC := src/main.cpp src/parser.cpp src/traverser.cpp src/ir.cpp src/utf.cpp
OBJ := $(SRC:.cpp=.o)

charta: $(OBJ)
	$(CXX) $(LDFLAGS) -o charta $^

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<

clean:
	rm -f $(OBJ) charta
