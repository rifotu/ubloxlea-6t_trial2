CC=gcc
CFLAGS= -g -O0 -Wall -std=gnu99
LFLAGS= -pthread -lrt -lm
INCLUDE = -I./inc
CPP_FILES = $(wildcard src/*.c)
OBJ_FILES = $(addprefix obj/, $(notdir $(CPP_FILES:.c=.o)))
TARGET = rawGpsDataJsonizer

all: $(TARGET)

$(TARGET): $(OBJ_FILES)
	$(CC) -o $(TARGET) $^ $(LFLAGS)

obj/%.o: src/%.c $(DEPS)
	$(CC) $(CFLAGS) $(INCLUDE) -c -o $@ $<


clean:
	rm ./${TARGET} obj/*.o
