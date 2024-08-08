CC = gcc
CFLAGS = -g -Wall

SRCMODULES = shell.c fslib.c strlib.c memlib.c path.c task.c
OBJMODULES = $(SRCMODULES:.c=.o)

%.o: %.c %.h
	$(CC) $(CFLAGS) -c $< -o $@

ifneq (clean, $(MAKECMDGOALS))
-include deps.mk
endif

run: task
	./task

task: main.c $(OBJMODULES)
	$(CC) $(CFLAGS) $^ -o $@

deps.mk: $(SRCMODULES)
	$(CC) -MM $^ > $@

clean:
	rm -f *.o task

