#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <stdbool.h>
#include "dictionary.c"
#include "dictionary.h"
#include "hash_functions.h"

// KEEP is defined in dictionary.h
#define MAX_LEN 256

typedef struct cracked_hash {
	char hash[2*KEEP+1];
	char *password, *alg;
}cracked_hash;

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

// Function name: crack_hashed_passwords
// Description:   Computes different hashes for each password in the password list,
//                then compare them to the hashed passwords to decide whether if
//                any of them matches this password. When multiple passwords match
//                the same hash, only the first one in the list is printed.
void crack_hashed_passwords(char *password_list, char *hashed_list, char *output) {
	FILE *fp;
	char password[MAX_LEN];  // passwords have at most 255 characters
	char hex_hash[2*KEEP+1]; // hashed passwords have at most 'keep' characters

	// load hashed passwords into hash table and file ordered linked list
	bool loaded = load(hashed_list);

	if (!loaded)
	{
		printf("Error loading the hash table\n");
		return;
	}
	int n_hashed = loaded;
	

	// load common passwords, hash them, and compare them to hashed passwords
	fp = fopen(password_list, "r");
	assert(fp != NULL);
	while(fgets(password, MAX_LEN, fp) != NULL) {	// changed fscanf function to fgets as it is faster
		// removing the newline charcter
		password[strcspn(password, "\n")] = '\0';
		
		for(int i=0; i < n_algs; i++) {
			unsigned char *hash_v = fn[i]((unsigned char *)password, strlen(password)); // storing the value of the output of the hash function in the hash pointer
			for(int j=0; j < KEEP; j++)
				sprintf(&hex_hash[2*j], "%02x", hash_v[j]); // converting the binary hash into hex hash
			hex_hash[2*KEEP] = '\0';
			// printf("%s : %s\n",password, hex_hash);

			
			node *res = check(hex_hash);
			if (res)
			{
				res->password = strdup(password);
				res->alg = algs[i];
			}
			free(hash_v);
		}
	}
	fclose(fp);

	// print results
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
}

