CXXFLAGS ?=-march=native -O3 -std=gnu++17 -fdata-sections -ffunction-sections
LDFLAGS ?= -Wl,--gc-sections -Wl,--print-gc-sections -Wl,-s
RM ?= rm
DEBUG ?= -DDEBUG -DDEBUG2 -DDEBUG3

all:
	g++ ${CXXFLAGS} -Isrc ${DEBUG} src/Dump.cpp src/fcplay.cpp src/FC.cpp src/LamePaula.cpp -o fcplay -lasound ${LDFLAGS}

clean: 
	${RM} -r fcplay fcplay.*

