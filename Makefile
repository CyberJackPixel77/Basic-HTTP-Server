CC = cl.exe
CFLAGS = /nologo /W3 /O2 /D "_CRT_SECURE_NO_WARNINGS" /I "src"
LIBS = ws2_32.lib

SRC_DIR = src
OBJ_DIR = .

TARGET = server.exe
OBJS = main.obj server.obj

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) /Fe$(TARGET) /link $(LIBS)
main.obj: $(SRC_DIR)\main.c
	$(CC) $(CFLAGS) /c $(SRC_DIR)\main.c

server.obj: $(SRC_DIR)\server.c
	$(CC) $(CFLAGS) /c $(SRC_DIR)\server.c
clean:
	del *.obj $(TARGET)

