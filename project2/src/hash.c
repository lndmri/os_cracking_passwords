#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <pthread.h>
#include <ctype.h>
#include <semaphore.h>

#include "hash_functions.h"

#define KEEP 16 // only the first 16 bytes of a hash are kept
#define TABLE_SIZE (1 << 16) // build a hash table of size 65536 by default; indexing on (digest[0]<<8) ^ digest[1]

struct cracked_hash {
    unsigned char target[KEEP];  // binary target hash
    char *password, *alg;
    int dict_index;              // dictionary index of the password
};

// single target entry
struct target_node {
    unsigned char target[KEEP];
    int index;
    struct target_node *next;
};

// hex to numerical
static int hex_digit(char c) {
    if ('0' <= c && c <= '9') return c - '0';
    if ('a' <= c && c <= 'f') return c - 'a' + 10;
    if ('A' <= c && c <= 'F') return c - 'A' + 10;
    return 0;
}

// hex string to binary array
static void hex_to_bytes(const char *hex, unsigned char *bytes) {
    for (int i = 0; i < KEEP; i++) {
        bytes[i] = (hex_digit(hex[2*i]) << 4) | hex_digit(hex[2*i+1]);
    }
}

// index the first two bytes of a 16-byte digest
static inline unsigned int hash_digest(const unsigned char *digest) {
    return ((unsigned int)digest[0] << 8) ^ digest[1];
}

// arrays for hashing
typedef unsigned char * (*hashing)(unsigned char *, unsigned int);
int n_algs = 4;
hashing fn[4] = { calculate_md5, calculate_sha1, calculate_sha256, calculate_sha512 };
char *algs[4] = { "MD5", "SHA1", "SHA256", "SHA512" };

static pthread_mutex_t *entry_locks;
static struct target_node *hash_table[TABLE_SIZE];

// dictionary segments for each thread
struct thread_args {
    int start_idx;
    int end_idx;
    char **dictionary;
    unsigned int *pw_lengths;
    struct cracked_hash *cracked_hashes;
    int n_hashed;
};

////////////////////////////////////////////////////////////////////////
//   1. compute its 4 possible digests                                //
//   2. do a single hash table lookup for each digest                 //
//   3. walk the linked list of possible matches, update as needed    //
////////////////////////////////////////////////////////////////////////
static void *crack_segment(void *arg) {
    struct thread_args *args = (struct thread_args*) arg;
    int start = args->start_idx;
    int end   = args->end_idx;
    char **dictionary = args->dictionary;
    struct cracked_hash *cracked_hashes = args->cracked_hashes;
    int n_hashed = args->n_hashed;

    for (int idx = start; idx < end; idx++) {
        unsigned char computed[KEEP];
        char *password = dictionary[idx];
        unsigned int pw_len = (unsigned int)strlen(password);

        for (int ai = 0; ai < n_algs; ai++) {
            unsigned char *digest = fn[ai]((unsigned char *)password, pw_len);
            memcpy(computed, digest, KEEP);
            free(digest);

            // table lookup
            unsigned int h = hash_digest(computed);
            struct target_node *node = hash_table[h];

            while (node) {
                if (memcmp(node->target, computed, KEEP) == 0) {
                    int j = node->index; // cracked_hashes

                    if (pthread_mutex_trylock(&entry_locks[j]) == 0) {
                        //////////////////////////////////////////////////////////////////////////////////////
                        //   1. the hash is not yet cracked => fill in                                      //
                        //   2. the hash is cracked => possibly override if this password's index is lower  // 
                        //////////////////////////////////////////////////////////////////////////////////////
                        if (cracked_hashes[j].password == NULL || idx < cracked_hashes[j].dict_index) {
                            if (cracked_hashes[j].password) {
                                free(cracked_hashes[j].password);
                            }
                            cracked_hashes[j].password   = strdup(password);
                            cracked_hashes[j].alg        = algs[ai];
                            cracked_hashes[j].dict_index = idx;
                        }
                        pthread_mutex_unlock(&entry_locks[j]);
                    }
                }
                node = node->next;
            } // end (node)

            
        } // end (ai)
    } // end (idx)

    return NULL;
}


static void build_target_hash_table(struct cracked_hash *cracked_hashes, int n_hashed) {
    // init the global hash table to NULL
    memset(hash_table, 0, sizeof(hash_table));

    for (int j = 0; j < n_hashed; j++) {
        unsigned int h = hash_digest(cracked_hashes[j].target);

        // allocate a new node for this target
        struct target_node *new_node = (struct target_node *)malloc(sizeof(struct target_node));
        memcpy(new_node->target, cracked_hashes[j].target, KEEP);
        new_node->index = j;

        // insert at head of the list
        new_node->next = hash_table[h];
        hash_table[h] = new_node;
    }
}
///////////////////////////////////////////////////////////////////////////////////////
// Function name: crack_hashed_passwords                                             //
// Description:   Computes different hashes for each password in the password list,  //
//                then compare them to the hashed passwords to decide whether if     //
//                any of them matches this password. When multiple passwords match   //
//                the same hash, only the first one in the list is printed.          //
///////////////////////////////////////////////////////////////////////////////////////
void crack_hashed_passwords(char *password_list, char *hashed_list, char *output) {
    FILE *fp;
    char buffer[256];
    int n_hashed = 0, n_passwords = 0;
    struct cracked_hash *cracked_hashes;
    char **dictionary;
    unsigned int *pw_lengths;

    // 1. read the target hashes into cracked_hashes
    fp = fopen(hashed_list, "r");
    assert(fp != NULL);
    while (fscanf(fp, "%255s", buffer) == 1) {
        n_hashed++;
    }
    rewind(fp);

    cracked_hashes = (struct cracked_hash*)malloc(n_hashed * sizeof(struct cracked_hash));
    assert(cracked_hashes != NULL);

    for (int i = 0; i < n_hashed; i++) {
        fscanf(fp, "%255s", buffer);
        // convert hex string to binary
        hex_to_bytes(buffer, cracked_hashes[i].target);
        cracked_hashes[i].password   = NULL;
        cracked_hashes[i].alg        = NULL;
        cracked_hashes[i].dict_index = -1;
    }
    fclose(fp);

    // 2. read the dictionary
    fp = fopen(password_list, "r");
    assert(fp != NULL);
    while (fscanf(fp, "%255s", buffer) == 1) {
        n_passwords++;
    }
    rewind(fp);

    dictionary = (char**)malloc(n_passwords * sizeof(char*));
    pw_lengths = (unsigned int*)malloc(n_passwords * sizeof(unsigned int));
    assert(dictionary != NULL && pw_lengths != NULL);
    for (int i = 0; i < n_passwords; i++) {
        fscanf(fp, "%255s", buffer);
        dictionary[i] = strdup(buffer);
    }
    fclose(fp);

    // 3. build the global hash table of target digests
    build_target_hash_table(cracked_hashes, n_hashed);

    // 4. initialize one mutex per target hash
    entry_locks = (pthread_mutex_t*)malloc(n_hashed * sizeof(pthread_mutex_t));
    assert(entry_locks != NULL);
    for (int j = 0; j < n_hashed; j++) {
        pthread_mutex_init(&entry_locks[j], NULL);
    }

    // 5. create up to 11 worker threads + main = 12 total
    int max_threads = 12;
    int num_threads = (n_passwords < max_threads) ? n_passwords : max_threads;
    if (num_threads < 1) num_threads = 1;

    pthread_t threads[11];         // up to 11 workers
    struct thread_args targs[12];  // each thread + main

    // partition the dictionary among the threads
    int base_count = n_passwords / num_threads;
    int extra = n_passwords % num_threads;
    int start_index = 0;

    for (int t = 0; t < num_threads; t++) {
        int count = base_count + ((t < extra) ? 1 : 0);
        targs[t].start_idx      = start_index;
        targs[t].end_idx        = start_index + count;
        targs[t].dictionary     = dictionary;
        targs[t].cracked_hashes = cracked_hashes;
        targs[t].n_hashed       = n_hashed;

        start_index += count;

        if (t != 0) {
            // worker threads
            pthread_create(&threads[t - 1], NULL, crack_segment, &targs[t]);
        }
    }

    // main thread handles segment 0
    crack_segment(&targs[0]);

    // wait for the worker threads
    for (int t = 1; t < num_threads; t++) {
        pthread_join(threads[t - 1], NULL);
    }

    // 6. write results to output
    fp = fopen(output, "w");
    assert(fp != NULL);
    for (int j = 0; j < n_hashed; j++) {
        if (cracked_hashes[j].password == NULL) {
            fprintf(fp, "not found\n");
        } else {
            fprintf(fp, "%s:%s\n", cracked_hashes[j].password, cracked_hashes[j].alg);
        }
    }
    fclose(fp);

    // 7. cleanup
    for (int j = 0; j < n_hashed; j++) {
        if (cracked_hashes[j].password)
            free(cracked_hashes[j].password);
        pthread_mutex_destroy(&entry_locks[j]);
    }
    free(entry_locks);

    // free dictionary
    for (int i = 0; i < n_passwords; i++) {
        free(dictionary[i]);
    }
    free(dictionary);

    // free cracked_hashes
    free(cracked_hashes);

    // free the hash table chains
    for (int i = 0; i < TABLE_SIZE; i++) {
        struct target_node *cur = hash_table[i];
        while (cur) {
            struct target_node *tmp = cur;
            cur = cur->next;
            free(tmp);
        }
        hash_table[i] = NULL;
    }
}