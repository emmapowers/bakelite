
INCLUDEPATH=../../generator/runtimes/cpptiny/

UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S),Linux)
	FLAGS = -static-libasan
endif
ifeq ($(UNAME_S),Darwin)
	FLAGS = -static-libsan
endif

ifdef CI
CI_FLAGS = 
else
CI_FLAGS = -ggdb -fsanitize=address -fno-omit-frame-pointer ${FLAGS}
endif

test: cpptiny
	./cpptiny

cpptiny: cpptiny-serialization.cpp cpptiny-framing.cpp cpptiny-protocol.cpp bakelite.h struct.h proto.h
	gcc cpptiny-serialization.cpp cpptiny-framing.cpp cpptiny-protocol.cpp ${CI_FLAGS} -lstdc++ -std=c++14 -lm -o cpptiny

.PHONY: struct.h
struct.h: struct.bakelite
	poetry run bakelite gen -l cpptiny -i struct.bakelite -o struct.h

.PHONY: proto.h
proto.h: proto.bakelite
	poetry run bakelite gen -l cpptiny -i proto.bakelite -o proto.h

.PHONY: bakelite.h
bakelite.h: ${INCLUDEPATH}/serializer.h ${INCLUDEPATH}/cobs.h ${INCLUDEPATH}/crc.h ${INCLUDEPATH}/declarations.h
	poetry run bakelite runtime -l cpptiny -o bakelite.h
