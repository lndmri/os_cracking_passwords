// Implements a hash table
#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <stdint.h>
#include "dictionary.h"



// Compare function. It checks two hashes and returns 1 (true) if they are the same, otherwise it returns 0.
int compare_hashes(char *a, char *b) {
	return memcmp(a, b, 2 * KEEP) == 0;
}

// Choosing number of buckets ( prime number chosen to avoid collisions. Number chosen through trial and error)
#define N 523

// global variable to keep track of all the words loaded
unsigned int loaded = 0;

// Hash table
node *table[N];

// Linked List for keeping order of appearance in the source file 
node *head = NULL;
node *tail = NULL;

// Returns the pointer of the if the word is found is in hash table, else NULL 
node* check(char *word)
{
    // index of where the word would be in the hash table
    unsigned int index = hash(word);
    node *p;
    for (p = table[index]; p != NULL; p = p->next_in_table)
    {
        if(compare_hashes(word, p->hash))
        {
            return p;
        }
    }
    return NULL;
}


// // OLD HASH FUNCTION USED
// // Hashes word to an index of out hash table
// unsigned int hash(const char* word)
// {
//     // word is a hex word
//     uint64_t hash_value = 0;

//     // Convert hex string to an integer safely
//     while (*word) {
//         char c = *word++;
//         hash_value = (hash_value << 4) | 
//                         ((c >= '0' && c <= '9') ? (c - '0') :
//                         (c >= 'a' && c <= 'f') ? (c - 'a' + 10) :
//                         (c >= 'A' && c <= 'F') ? (c - 'A' + 10) : 0);
//     }

//     // Modulo operation to fit into available buckets
//     return (unsigned int)(hash_value % N);
// }

// New Hash Function 

// new hash function
unsigned int hash(const char *word) {
    uint32_t hash = 2166136261u; // FNV-1a hash initial value
    while (*word) {
        hash ^= (unsigned char)*word++;
        hash *= 16777619;
    }
    return hash % N;
}

// Loads dictionary into memory, returning true if successful, else false
bool load(const char *file)
{
    FILE *fp = fopen(file, "r");
    if (fp == NULL)
    {
        printf("Error opening source file to populate the hash table\n");
        return false;
    }

    // make each bucket of the table point to NULL
    point_null(table, N);

    // declaring a static buffer to hold the word
    char word[2*KEEP+1];

    // scanning from the file each word
    while (fscanf(fp, "%s", word) != EOF)
    {
        node *new = malloc(sizeof(node));
        if (new == NULL)
        {
            printf("Error mallocking on load function\n");
            return false;
        }

        strcpy(new->hash, word);
        new->password = NULL;
        new->alg = NULL;
        new->next_in_table = NULL;
        new->next_in_file = NULL;


        // now we use the hash function to find the index of the link list in which we are going to isert the node
        int index = hash(word);
        // adding to hash table 
        new->next_in_table = table[index];
        table[index] = new;

        // adding to linked list 
        if (head == NULL)
        {
            head = new;
            tail = new;
        }
        else
        {
            tail->next_in_file = new;
            tail = new;
        }
        // counting elements
        loaded++;
    }

    fclose(fp);
    return true;
}

// Returns number of words in dictionary if loaded, else 0 if not yet loaded
unsigned int size(void)
{
    return loaded;
}

// Unloads dictionary from memory, returning true if successful, else false
bool unload(void)
{
    for (int i = 0; i < N; i++)
    {
        while(table[i] != NULL)
        {
            node *temp = table[i];
            table[i] = table[i]->next_in_table;
            free(temp);
        }

        if (table[i] != NULL)
        {
            printf("Error unloading\n");
            return false;
        }
    }
    head = NULL;
    tail = NULL;
    return true;
}

// prints the hash table
void print_table(node *hash_table[], int n)
{   node *p;
    for (int i = 0; i < n; i++)
        for(p = hash_table[i]; p != NULL; p = p->next_in_table)
            printf("Bucket : %d\tWord: %s\n", i, p->hash);

}

// prints the order of how words appeared in the file (uses a linked list)
void print_file_order()
{
    node *p = NULL;
    printf("File order: \n");
    for(p = head; p != NULL; p = p->next_in_file)
    {
        printf("%s\n", p->hash);
    }

}

// point all buckets to NULL in the table
void point_null(node *hash_table[], int n)
{
    for(int i = 0; i < n; i++)
    {
        hash_table[i] = NULL;
    }
}

