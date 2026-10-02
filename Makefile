CC=gcc
CFLAGS=-Wall -Wextra -O2 -Iinclude -MMD -MP

TARGET=ken_socket_v2
SRC=$(wildcard src/*.c)
OBJ=$(SRC:src/%.c=build/%.o)
DEPLOY=\\\\192.168.0.240\\project\\Ken_Lin\\mySocket

all: $(TARGET) deploy

$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) $(OBJ) -o $@

build/%.o: src/%.c | build
	$(CC) $(CFLAGS) -c $< -o $@

build:
	mkdir -p build

deploy: $(TARGET)
	powershell.exe -Command "Copy-Item '$(TARGET)' '$(DEPLOY)' -Force"

clean:
	rm -rf build $(TARGET)

-include $(OBJ:.o=.d)

.PHONY: all deploy clean