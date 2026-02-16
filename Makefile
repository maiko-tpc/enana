CXX = g++
TARGET = ./enana
OBJDIR=./obj
SOURCES = $(wildcard *.cpp)
ROOTFLAGS = $(shell root-config --cflags)
ROOTLIBS  = $(shell root-config --libs) -lRHTTP
OBJECTS = $(addprefix $(OBJDIR)/, $(SOURCES:.cpp=.o)) $(OBJDIR)/config.o

CFLAGS = -O2 -Wall -Wno-write-strings -Wno-stringop-truncation -mtune=native -march=native -lncurses ${ROOTFLAGS} -Iinclude
DEBAG = -g
LDFLAGS = -L/home/furuno/opt/babirl/babirl250503/lib
LIBS = ${ROOTLIBS}  -lbabirl -lm

all: ${TARGET}
${TARGET}: $(OBJECTS)
	${CXX} -o $@ $^ ${CFLAGS} ${LDFLAGS} ${LIBS}

$(OBJDIR)/config.o: config.c
	gcc $< -c -o $@ -lm -DLinux

$(OBJDIR)/%.o: %.cpp
	@[ -d $(OBJDIR) ]
	$(CXX) ${CFLAGS} ${LDFLAGS} ${LIBS} -o $@ -c $<


clean:
	${RM} ${OBJDIR}/*.o ${TARGET}
