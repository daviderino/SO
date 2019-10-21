#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "fs.h"
#include "lib/hash.h"

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
	fs->hashSize = hashSize;
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

	sync_wrlock(&(fs->bstLock));
	fs->hashTable[i] = insert(fs->hashTable[i], name, inumber);
	sync_unlock(&(fs->bstLock));
}

void delete(tecnicofs* fs, char *name){
	int i = hash(name, fs->hashSize);

	sync_wrlock(&(fs->bstLock));
	fs->hashTable[i] = remove_item(fs->hashTable[i], name);
	sync_unlock(&(fs->bstLock));
}

int lookup(tecnicofs* fs, char *name){
	int i = hash(name, fs->hashSize);

	sync_rdlock(&(fs->bstLock));
	int inumber = 0;
	node* searchNode = search(fs->hashTable[i], name);

	if ( searchNode )  {
		inumber = searchNode->inumber;
	}
	sync_unlock(&(fs->bstLock));
	return inumber;
}

void print_tecnicofs_tree(FILE *fp, tecnicofs *fs) {
	int i;

	for(i = 0; i < fs->hashSize; i++) {
		print_tree(fp, fs->hashTable[i]);
	}
}
