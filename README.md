# Concurrent Hashtable

## Description
This project implements a hash table with thread-safe operations using reader-writer locks. The program reads instructions from a file, creates multiple threads to handle operations concurrently, and produces output to the console.

## Team Members
- Edward Weir
- Vi Dang
- Bao Huynh
- Michael McCorvey
- Steven Huynh

## Chat GPT Usage
Citation:
AI assistance was provided for constructing the hashtable program. 

### Link to Chat GPT Discussion:
https://chatgpt.com/share/673398db-5d54-8000-9e37-1a7f959ba215

## Files 
- chash (main file)
- hash_table (controls hash table functionality)
- logger (prints logs output file)
- test_output (output file used to check correct output)
- Makefile

## Running the program
1. Ensure you have the necessary C libraries installed
2. In the terminal enter `make` to compile the code
3. After compilation run the command `make run` or `./chash_program`
4. To recompile after any changes run `make clean` and run `make` again
