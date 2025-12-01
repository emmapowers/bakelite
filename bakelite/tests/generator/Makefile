
CPPTINY_INCLUDEPATH=../../generator/runtimes/cpptiny/
CTINY_INCLUDEPATH=../../generator/runtimes/ctiny/

UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S),Linux)
	FLAGS = -static-libasan
endif
ifeq ($(UNAME_S),Darwin)
	FLAGS =
endif

ifdef CI
CI_FLAGS =
else
CI_FLAGS = -ggdb -fsanitize=address -fno-omit-frame-pointer ${FLAGS}
endif

test: cpptiny ctiny
	./cpptiny
	./ctiny

# C++ (cpptiny) tests
cpptiny: cpptiny-serialization.cpp cpptiny-framing.cpp cpptiny-protocol.cpp bakelite.h struct.h proto.h
	gcc cpptiny-serialization.cpp cpptiny-framing.cpp cpptiny-protocol.cpp ${CI_FLAGS} -lstdc++ -std=c++14 -lm -o cpptiny

.PHONY: struct.h
struct.h: struct.bakelite
	pixi run -- bakelite gen -l cpptiny -i struct.bakelite -o struct.h

.PHONY: proto.h
proto.h: proto.bakelite
	pixi run -- bakelite gen -l cpptiny -i proto.bakelite -o proto.h

.PHONY: bakelite.h
bakelite.h: ${CPPTINY_INCLUDEPATH}/serializer.h ${CPPTINY_INCLUDEPATH}/cobs.h ${CPPTINY_INCLUDEPATH}/crc.h ${CPPTINY_INCLUDEPATH}/declarations.h
	pixi run -- bakelite runtime -l cpptiny -o bakelite.h

# C99 (ctiny) tests
ctiny: ctiny-serialization.c ctiny-framing.c ctiny-protocol.c bakelite-ctiny.h struct-ctiny.h proto-ctiny.h
	gcc ctiny-serialization.c ctiny-framing.c ctiny-protocol.c ${CI_FLAGS} -std=c99 -lm -o ctiny

.PHONY: struct-ctiny.h
struct-ctiny.h: struct-ctiny.bakelite bakelite-ctiny.h
	pixi run -- bakelite gen -l ctiny -i struct-ctiny.bakelite -o struct-ctiny.h
	sed -i.bak 's|#include "bakelite.h"|#include "bakelite-ctiny.h"|g' struct-ctiny.h && rm -f struct-ctiny.h.bak

.PHONY: proto-ctiny.h
proto-ctiny.h: proto.bakelite bakelite-ctiny.h
	pixi run -- bakelite gen -l ctiny -i proto.bakelite -o proto-ctiny.h
	sed -i.bak 's|#include "bakelite.h"|#include "bakelite-ctiny.h"|g' proto-ctiny.h && rm -f proto-ctiny.h.bak

.PHONY: bakelite-ctiny.h
bakelite-ctiny.h: ${CTINY_INCLUDEPATH}/serializer.h ${CTINY_INCLUDEPATH}/cobs.h ${CTINY_INCLUDEPATH}/crc.h ${CTINY_INCLUDEPATH}/types.h ${CTINY_INCLUDEPATH}/stream.h
	pixi run -- bakelite runtime -l ctiny -o bakelite-ctiny.h

clean:
	rm -f cpptiny ctiny *.h

clean-ctiny:
	rm -f ctiny struct-ctiny.h proto-ctiny.h bakelite-ctiny.h

rebuild-ctiny: clean-ctiny ctiny
