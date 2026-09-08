CC=gcc
CFLAGS=-Wall -Wextra -O2 -pthread

TARGET=ken_socket
SRC=ken_socket.c ken_socket_funs.c
DEPLOY=\\\\192.168.0.240\\project\\Ken_Lin\\mySocket

all: $(TARGET) deploy

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) $(SRC) -o $@

deploy: $(TARGET)
	powershell.exe -Command "Copy-Item '$(TARGET)' '$(DEPLOY)' -Force"

clean:
	rm -f $(TARGET)