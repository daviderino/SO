#include "fs.h"
#include "hash.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

int obtainNewInumber(tecnicofs* fs) {
	int newInumber = ++(fs->nextINumber);
	return newInumber;
}

tecnicofs* new_tecnicofs(int hashSize){
	int i;
	tecnicofs*fs = malloc(sizeof(tecnicofs));
	if (!fs) {
		perror("failed to allocate tecnicofs");
		exit(EXIT_FAILURE);
	}
	fs->hashTable = malloc(sizeof(node*) * hashSize);
	for(i = 0; i < hashSize; i++) {
		fs->hashTable[i] = NULL;
	}
	fs->nextINumber= 0;
	return fs;
}

void free_tecnicofs(tecnicofs* fs){
	int i;
	for(i = 0; i < fs->hashSize; i++) {
		free_tree(fs->hashTable[i]);
	}
	free(fs->hashTable);
	free(fs);
}

void create(tecnicofs* fs, char *name, int inumber){
	int i = hash(name, fs->hashSize);
	fs->hashTable[i] = insert(fs->hashTable[i], name, inumber);
}

void delete(tecnicofs* fs, char *name){
	int i = hash(name, fs->hashSize);
	fs->hashTable[i] = remove_item(fs->hashTable[i], name);
}

int lookup(tecnicofs* fs, char *name){
	int i = hash(name, fs->hashSize);
	node* searchNode = search(fs->hashTable[i], name);
	if ( searchNode ) return searchNode->inumber;
	return 0;
}

void print_tecnicofs_tree(char * title, tecnicofs *fs){
	int i;
	FILE *file= fopen(title, "w");
	for(i = 0; i < fs->hashSize; i++) {
		print_tree(file, fs->hashTable[i]);
	}

	fclose(file);

}
