# DAM-2160i Makefile demo 
# by nemo @ 2008/12/19
# NOTE:
# 1.use make to compile all the object source file;
# 2.add new source file to SRC macro;
# 3.add your head file dir to INCS macro, and libs to LIBS macro;
# 4.when modify a head file, do make clean and then make;
# 5.when debug in x86 machine, define CROSS_COMPILER to nothing;
#
CROSS_COMPILER =##arm-himix200-linux-
TARGET=client

CC = $(CROSS_COMPILER)gcc
LD = $(CROSS_COMPILER)gcc 
CPP = $(CROSS_COMPILER)g++

DIR_PRJ:=$(shell pwd)
DIR_HAL:=$(DIR_PRJ)/hal

INCLUDE+=-I$(DIR_PRJ)/

CFLAGS+=-O2  -D_GNU_SOURCE -D_LINUX 
CPPFLAGS+=-O2  -D_GNU_SOURCE -D_LINUX -std=c++11 -Wall -fno-builtin -fno-strict-aliasing -Wno-unused-but-set-variable

LDFLAGS+=-I$(INCLUDE) 
LIBS=-lpthread -lm -lrt -lstdc++ -ldl
LIBS+=
STATIC_OLIB+= 


SRC:=$(wildcard $(DIR_PRJ)/*.c)


#all : $(TARGET) 

#$(TARGET) : $(SRC)
#	$(CC) $(INCLUDE) $(CFLAGS) -o $@ $^ $(STATIC_OLIB) $(LIBS) 
	
OBJS := $(SRC:.c=.o)

OBJS_CPP := $(SRC_CPP:.cpp=.o)

DEP := $(OBJS:.o=.d)

all: $(TARGET)

$(TARGET):$(SRC) $(OBJS) $(SRC_CPP) $(OBJS_CPP)
	$(LD) $(LDFLAGS) $(OBJS) $(OBJS_CPP) $(STATIC_OLIB) $(LIBS) -o $@

-include $(DEP)
%.o:%.c
	$(CC) $(INCLUDE) $(CFLAGS) -MM -MT $@ -MF $(patsubst %.o, %.d, $@) $<
	$(CC) $(INCLUDE) $(CFLAGS) -c $< -o $@
	
%.o:%.cpp
	$(CPP) $(INCLUDE) $(CPPFLAGS) -c $< -o $@
    
clean :
	rm -f $(TARGET) $(OBJS) $(OBJS_CPP) $(DEP) *.o *.tgz
	
