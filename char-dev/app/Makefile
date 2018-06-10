CC = gcc
CFLAGS = -I.
TARGET = testapp
OBJ = main.o

%.o: %.c
	$(CC) -c -o $@ $< $(CFLAGS)

$(TARGET): $(OBJ)
	$(CC) -o $@ $^ $(CFLAGS)

clean:
	rm -f *.o $(TARGET)

.PHONY: clean
