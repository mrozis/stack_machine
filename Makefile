CC = gcc
CFLAGS = -Wall
TARGET = vm
OBJECTS = main.o stack.o
TARGET : $(OBJECTS)
	$(CC) $(CFLAGS) $(OBJECTS) -o $(TARGET)

main.o :  main.c vm.h
	$(CC) $(CFLAGS) -c main.c -o main.o
stack.o : stack.c vm.h
	$(CC) $(CFLAGS) -c stack.c -o stack.o

clean:
	rm $(TARGET) *.o        
