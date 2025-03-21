#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <stdbool.h>
#include "dictionary.c"
#include "dictionary.h"
#include "hash_functions.h"
#include <pthread.h>

// KEEP is defined in dictionary.h
#define MAX_LEN 256
#define NUM_THREADS 4

pthread_mutex_t hash_table_lock = PTHREAD_MUTEX_INITIALIZER;

typedef struct thread_content
{
	char *password_list;
	unsigned int start;
	unsigned int end;
} thread_content;

typedef unsigned char * (*hashing)(unsigned char *, unsigned int);

int n_algs = 4;
hashing fn[4] = {calculate_md5, calculate_sha1, calculate_sha256, calculate_sha512};
char *algs[4] = {"MD5", "SHA1", "SHA256", "SHA512"};

// abstracted hash count function to optimize it with threads
// int count_hashes (char *hashed_list)
// {
// 	char hex_hash[2*KEEP+1];
// 	int n = 0;
// 	FILE *fp = fopen(hashed_list, "r");
// 	assert(fp != NULL);
// 	while(fscanf(fp, "%s", hex_hash) == 1)
// 		n++;
// 	return n;
// }

void *routine_crack_hashed_passwords(void *arg)
{
	thread_content *content = (thread_content *)arg;
	FILE *fp = fopen(content->password_list, "r");
	assert(fp != NULL);

	fseek(fp, content->start, SEEK_SET);

	char password[MAX_LEN];
	while(ftell(fp) < content->end && fgets(password, MAX_LEN, fp) != NULL)
	{
		password[strcspn(password, "\n")] = '\0';

        for (int i = 0; i < n_algs; i++) {
            unsigned char *hash_v = fn[i]((unsigned char *)password, strlen(password));
            char hex_hash[2*KEEP+1];
            for(int j=0; j < KEEP; j++)
				sprintf(&hex_hash[2*j], "%02x", hash_v[j]); // converting the binary hash into hex hash
			hex_hash[2*KEEP] = '\0';

            pthread_mutex_lock(&hash_table_lock);
            node *res = check(hex_hash);
            if (res && res->password == NULL) {
                res->password = strdup(password);
                res->alg = algs[i];
            }
            pthread_mutex_unlock(&hash_table_lock);

            free(hash_v);
        }
    }

    fclose(fp);
    return NULL;
}



// Function name: crack_hashed_passwords
// Description:   Computes different hashes for each password in the password list,
//                then compare them to the hashed passwords to decide whether if
//                any of them matches this password. When multiple passwords match
//                the same hash, only the first one in the list is printed.
void crack_hashed_passwords(char *password_list, char *hashed_list, char *output) {
	FILE *fp;
	// char password[MAX_LEN];  // passwords have at most 255 characters
	char hex_hash[2*KEEP+1]; // hashed passwords have at most 'keep' characters

	// PART 1: load hashed passwords into hash table and file ordered linked list
	bool loaded = load(hashed_list);

	if (!loaded)
	{
		printf("Error loading the hash table\n");
		return;
	}
	int n_hashed = loaded;
	

	// PART 2: load common passwords, hash them, and compare them to hashed passwords
	fp = fopen(password_list, "r");
	assert(fp != NULL);

	// getting the file size of password list to avoid multiple IO opearations with fgets, fscanf....etc
	fseek(fp, 0, SEEK_END);
	long file_size = ftell(fp);
	fseek(fp, 0, SEEK_SET);

	// USING THREADS
	pthread_t th[NUM_THREADS];
	thread_content th_content[NUM_THREADS];

	long chunk_size = file_size / NUM_THREADS;
	for (int i = 0; i < NUM_THREADS; i++)
	{
		th_content[i].password_list = password_list;
		th_content[i].start = i * chunk_size;
		th_content[i].end = (i == NUM_THREADS - 1)  ? file_size : (i + 1) * chunk_size;
		pthread_create(&th[i], NULL, routine_crack_hashed_passwords, &th_content[i]);
	}

	for (int i = 0; i < NUM_THREADS; i++)
	{
		pthread_join(th[i], NULL);
	}
	
	// USING A SINGLE THREAD
	// // allocating a buffer to save the whole file into it.
	// char *passwords_file_buffer = malloc(file_size + 1);
	// if (passwords_file_buffer == NULL)
	// {
	// 	printf("Error allocating memory for passwords file buffer\n");
	// 	fclose(fp);
	// 	unload(); 
	// 	return;
	// }

	// //reading the entire file into the buffer
	// size_t bytes_read = fread(passwords_file_buffer, 1, file_size, fp); // args: buffer, times to read, size of buffer, file stream
	// if (bytes_read != file_size)
	// {
	// 	printf("Error while reading passwords file, only ready %zu of %ld bytes", bytes_read, file_size);
	// 	free(passwords_file_buffer);
	// 	fclose(fp);
	// 	unload();
	// 	return;
	// }
	// passwords_file_buffer[file_size] = '\0';
	// fclose(fp);
	
	// // processing each line
	// char *password = strtok(passwords_file_buffer, "\n");
	// while(password != NULL) {	// changed fscanf function to fgets as it is faster
	// 	for(int i=0; i < n_algs; i++) 
	// 	{
	// 		unsigned char *hash_v = fn[i]((unsigned char *)password, strlen(password)); // storing the value of the output of the hash function in the hash pointer
			
	// 		for(int j=0; j < KEEP; j++)
	// 			sprintf(&hex_hash[2*j], "%02x", hash_v[j]); // converting the binary hash into hex hash
	// 		hex_hash[2*KEEP] = '\0';
	// 		// printf("%s : %s\n",password, hex_hash);

	// 		node *res = check(hex_hash);
	// 		if (res)
	// 		{
	// 			res->password = strdup(password);
	// 			res->alg = algs[i];
	// 		}
	// 		free(hash_v);
	// 	}
	// 	password = strtok(NULL, "\n");
	// }


	// PART 3 print results
	fp = fopen(output, "w");
	assert(fp != NULL);
	node *p;
	for(p = head; p != NULL; p = p->next_in_file) {
		if(p->password ==  NULL)
			fprintf(fp, "not found\n");
		else
			fprintf(fp, "%s:%s\n", p->password, p->alg);
	}

	fclose(fp);

	// release stuff
	unload();
	// free(passwords_file_buffer);
}

