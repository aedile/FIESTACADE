CC ?= cc
CXX ?= c++
CFLAGS ?= -O1 -g -Wall -I. -Ishim -I../components/gfx/include -I../components/mqart/include -I../main
preview: preview.c ../components/gfx/src/gfx.c ../components/mqart/src/mqart.c ../main/menu.cpp
	$(CXX) $(CFLAGS) -x c++ -o $@ $^
clean:
	rm -f preview
