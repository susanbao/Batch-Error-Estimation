#definitions
DIR_INC = inc
DIR_SRC = src
DIR_OBJ = obj
DIR_LIB = lib
SOURCE  := $(wildcard ${DIR_SRC}/*.c) $(wildcard ${DIR_SRC}/*.cpp)
OBJS    := $(patsubst ${DIR_SRC}/%.c,${DIR_OBJ}/%.o,$(patsubst ${DIR_SRC}/%.cpp,${DIR_OBJ}/%.o,$(SOURCE)))
TARGET  := bee.out

#compiling parameters
CC      := g++
LIBS    := -labc -lm -ldl -rdynamic -lreadline -ltermcap -lpthread -lstdc++ -lrt
LDFLAGS := -L ${DIR_LIB}
DEFINES := $(FLAG) -DLIN64
INCLUDE := -I ${DIR_INC}/abc -I ${DIR_INC}/usr
CFLAGS  := -g -Wall -O3 -std=c++11 $(DEFINES) $(INCLUDE)
CXXFLAGS:= $(CFLAGS)

#commands
.PHONY : all objs rebuild clean init
all : init $(TARGET)

objs : init $(OBJS)

rebuild: clean all

clean :
	rm -rf $(DIR_OBJ)
	rm -f $(TARGET)

init :
	if [ ! -d obj ]; then mkdir obj; fi

$(TARGET) : $(OBJS)
	$(CC) $(CXXFLAGS) -o $@ $(OBJS) $(LDFLAGS) $(LIBS)

${DIR_OBJ}/%.o:${DIR_SRC}/%.cpp
	$(CC) $(CXXFLAGS) -c $< -o $@
