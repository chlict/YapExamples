BOOST=${HOME}/install/boost-1.68
export CC=c++
export OPT=-std=c++17

all: test

hello: hello_world.cpp
	$(CC) $(OPT) -I$(BOOST) $< -o $@

test: test.cpp
	c++ -std=c++17 -I$(BOOST) $<

calc2b: calc2b.cpp
	c++ -std=c++17 -I$(BOOST) $<

map_assign: map_assign.cpp
	c++ -std=c++17 -I$(BOOST) $<

clean:
	rm -f a.out

