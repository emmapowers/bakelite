
INCLUDEPATH=../../generator/runtimes/cpptiny/

test: cpptiny
	./cpptiny

cpptiny: cpptiny-serialization.cpp cpptiny-framing.cpp cpptiny-protocol.cpp bakelite.h struct.h proto.h
	gcc cpptiny-serialization.cpp cpptiny-framing.cpp cpptiny-protocol.cpp -ggdb -fsanitize=address -fno-omit-frame-pointer -static-libsan -lstdc++ -std=c++14 -lm -o cpptiny

.PHONY: struct.h
struct.h: struct.bakelite
	poetry run bakelite gen -l cpptiny -i struct.bakelite -o struct.h

.PHONY: proto.h
struct.h: proto.bakelite
	poetry run bakelite gen -l cpptiny -i proto.bakelite -o proto.h

.PHONY: bakelite.h
bakelite.h: ${INCLUDEPATH}/bakelite.h
	poetry run bakelite runtime -l cpptiny -o bakelite.h
