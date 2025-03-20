// Declares a hash table's functionality
#ifndef DICTIONARY_H
#define DICTIONARY_H
#include <stdbool.h>

#define KEEP 16 // only the first 16 bytes of a hash are kept

// Represents a node in a hash table
typedef struct node
{
    char hash[2*KEEP+1];
	char *password, *alg;
    struct node *next_in_table; // represents the next node in the hash table 
    struct node *next_in_file; // represents the next node in sequential order from the file
}
node;


// Prototypes
int compare_hashes(char *a, char *b);
node *check(char *word);
unsigned int hash(const char *word);
bool load(const char *dictionary);
unsigned int size(void);
bool unload(void);
void print_table(node *hash_table[], int n);
void print_file_order();
void point_null(node *hash_table[], int n);

#endif // DICTIONARY_H
