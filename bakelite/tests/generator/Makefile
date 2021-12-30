
INCLUDEPATH=../../generator/runtimes/cpptiny/

test: cpptiny
	./cpptiny

cpptiny: cpptiny.cpp bakelite.h struct.h
	gcc cpptiny.cpp -lstdc++ -std=c++14 -o cpptiny

struct.h: struct.ex
	poetry run bakelite gen -l cpptiny -i struct.ex -o struct.h

bakelite.h: ${INCLUDEPATH}/bakelite.h
	poetry run bakelite runtime -l cpptiny -o bakelite.h