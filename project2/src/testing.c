// #include <stdio.h>
// #include <string.h>
// #include <stdlib.h>
// #include <assert.h>
// #include <stdbool.h>
// // #include "dictionary.c"
// #include "hash_functions.h"

#define KEEP 16

// #define HASH_LEN 256

// typedef struct node
// {
//     char hash[2*KEEP+1];
// 	char *password, *alg;
// } node;


// int main(void)
// {
//     FILE *fp = fopen("data/hashes.txt", "r");
//     if (fp == NULL)
//     {
//         printf("Error opening file");
//         return 1;
//     }

    
//     char hex_hash[HASH_LEN];
//     while(fgets(hex_hash, HASH_LEN, fp) != NULL)
//     {
//         hex_hash[strcspn(hex_hash, "\n")] = '\0';
//         printf("%s\n", hex_hash);
//     }

//     // VS

//     while(fscanf(fp, "%s", hex_hash) != NULL)
//     {
//         printf("%s\n", hex_hash);
//     }

//     fclose(fp);

//     return 0;
// }

// --------------------------------------testing fgets vs fscanf---------------------------------

// #include <stdio.h>
// #include <string.h>
// #include <stdlib.h>
// #include <time.h>

// #define HASH_LEN 256

// void benchmark_fgets(FILE *fp) {
//     char hex_hash[HASH_LEN];
//     rewind(fp);  // Reset file pointer

//     clock_t start = clock();
//     while (fgets(hex_hash, HASH_LEN, fp) != NULL) {
//         hex_hash[strcspn(hex_hash, "\n")] = '\0';  // Remove newline
//     }
//     clock_t end = clock();

//     printf("fgets time: %lf seconds\n", (double)(end - start) / CLOCKS_PER_SEC);
// }

// void benchmark_fscanf(FILE *fp) {
//     char hex_hash[2*KEEP+1];
//     rewind(fp);  // Reset file pointer

//     clock_t start = clock();
//     while (fscanf(fp, "%s", hex_hash) != EOF) {
//         // No need to remove '\n' as fscanf ignores whitespace
//     }
//     clock_t end = clock();

//     printf("fscanf time: %lf seconds\n", (double)(end - start) / CLOCKS_PER_SEC);
// }

// int main(void) {
//     FILE *fp = fopen("data/hashes_more.txt", "r");
//     if (fp == NULL) {
//         printf("Error opening file\n");
//         return 1;
//     }

//     benchmark_fscanf(fp);
//     benchmark_fgets(fp);

//     fclose(fp);
//     return 0;
// }

// --------------------------------------testing hash table functionality ---------------------------------
#include "dictionary.h"
#include "dictionary.c"

int main(void)
{
    char *file_path = "data/hashes.txt";
    bool loaded = load(file_path);
    printf("Loaded: %d\n", loaded);
    print_table(table, N);
    print_file_order();
    unload();
    return 0;
}

