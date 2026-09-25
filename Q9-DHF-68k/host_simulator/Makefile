CC = gcc
CFLAGS = -Wall -Wextra -O2

all: host_simulator

host_simulator: main.o memory_backend.o fs_ops.o ../descriptor/dhf_descriptor.o
	$(CC) $(CFLAGS) -o $@ $^

main.o: main.c
	$(CC) $(CFLAGS) -c -o $@ main.c

memory_backend.o: memory_backend.c
	$(CC) $(CFLAGS) -c -o $@ memory_backend.c

fs_ops.o: fs_ops.c
	$(CC) $(CFLAGS) -c -o $@ fs_ops.c

../descriptor/dhf_descriptor.o: ../descriptor/dhf_descriptor.c
	$(CC) $(CFLAGS) -c -o $@ ../descriptor/dhf_descriptor.c

clean:
	rm -f *.o host_simulator ../descriptor/dhf_descriptor.o
