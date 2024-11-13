CC = gcc
CFLAGS = -pthread -Wall -Wextra
OBJ = logger.o hash_table.o chash.o
EXEC = chash_program

# Target to build the final executable
$(EXEC): $(OBJ)
	$(CC) $(OBJ) -o $(EXEC)

# Compile logger.c
logger.o: logger.c hash_table.h
	$(CC) $(CFLAGS) -c logger.c

# Compile hash_table.c
hash_table.o: hash_table.c hash_table.h
	$(CC) $(CFLAGS) -c hash_table.c

# Compile chash.c (main file)
chash.o: chash.c hash_table.h
	$(CC) $(CFLAGS) -c chash.c

# Clean object files and executable
clean:
	rm -f $(OBJ) $(EXEC)

# Run the program
run: $(EXEC)
	./$(EXEC)

# Create the directory for output files if it doesn't exist
output:
	mkdir -p output

# Default target
all: $(EXEC)
