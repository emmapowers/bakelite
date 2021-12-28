
INCLUDEPATH=../../generator/runtimes/cpptiny/

test: cpptiny
	./cpptiny

cpptiny: cpptiny.cpp ${INCLUDEPATH}bakelite.h struct.h
	gcc cpptiny.cpp -lstdc++ -std=c++14 -I${INCLUDEPATH} -o cpptiny

struct.h: struct.ex
	poetry run bakelite -l cpptiny -i struct.ex -o struct.h