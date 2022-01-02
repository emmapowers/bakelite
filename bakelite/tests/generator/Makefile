
INCLUDEPATH=../../generator/runtimes/cpptiny/

test: cpptiny
	./cpptiny

cpptiny: cpptiny.cpp bakelite.h struct.h
	gcc cpptiny.cpp -lstdc++ -std=c++14 -lm -o cpptiny

.PHONY: struct.h
struct.h: struct.ex
	poetry run bakelite gen -l cpptiny -i struct.ex -o struct.h

.PHONY: bakelite.h
bakelite.h: ${INCLUDEPATH}/bakelite.h
	poetry run bakelite runtime -l cpptiny -o bakelite.h
