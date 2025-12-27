CXX := clang++
CXXFLAGS := -Wall -Wextra -std=c++23 -ggdb
CC := clang
CCFLAGS := -Wall -Wextra -ggdb
LDFLAGS := -fsanitize=address,undefined

SRC := src/main.cpp src/parser.cpp src/traverser.cpp src/ir.cpp src/utf.cpp src/make_c.cpp src/builder.cpp src/checks.cpp
OBJ := $(SRC:.cpp=.o)

CORE_SRC := core/core.c
CORE_H := core/core.h
CORE_OBJ := $(CORE_SRC:.c=.o)

all: core charta mangler

charta: $(OBJ)
	$(CXX) -o charta $^ $(LDFLAGS)

core: $(CORE_OBJ) $(CORE_H)
	ar rcs libcore.a $^

mangler: src/mangler.cpp src/utf.cpp
	$(CXX) $(CXXFLAGS) -o mangler $^ $(LDFLAGS)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<

core/%.o: core/%.c core/%.h
	$(CC) $(CCFLAGS) -DPRE=1 -c -o $@ $<

core/%.c: core/%.pre.c mangler
	python ./process.py $< $@

core/%.h: core/%.pre.h mangler
	python ./process.py $< $@

.PRECIOUS: core/%.c core/%.h

clean:
	rm -f $(OBJ) charta $(CORE_OBJ) libcore.a core/core.h core/core.c mangler
