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
	fs->bstLock = malloc(sizeof(syncMech) * hashSize);

	for(i = 0; i < hashSize; i++) {
		fs->hashTable[i] = NULL;
		sync_init(&(fs->bstLock[i]));
	}

	fs->nextINumber= 0;
	fs->hashSize = hashSize;
	return fs;
}

void free_tecnicofs(tecnicofs* fs){
	int i;
	for(i = 0; i < fs->hashSize; i++) {
		free_tree(fs->hashTable[i]);
		sync_destroy(&(fs->bstLock[i]));
	}
	free(fs->hashTable);
	free(fs);
}

void create(tecnicofs* fs, char *name, int inumber){
	int i = hash(name, fs->hashSize);

	sync_wrlock(&(fs->bstLock[i]));
	fs->hashTable[i] = insert(fs->hashTable[i], name, inumber);
	sync_unlock(&(fs->bstLock[i]));
}

void delete(tecnicofs* fs, char *name){
	int i = hash(name, fs->hashSize);

	sync_wrlock(&(fs->bstLock[i]));
	fs->hashTable[i] = remove_item(fs->hashTable[i], name);
	sync_unlock(&(fs->bstLock[i]));
}

int lookup(tecnicofs* fs, char *name){
	int i = hash(name, fs->hashSize);

	sync_rdlock(&(fs->bstLock[i]));
	int inumber = 0;
	node* searchNode = search(fs->hashTable[i], name);

	if (searchNode)  {
		inumber = searchNode->inumber;
	}
	sync_unlock(&(fs->bstLock[i]));
	return inumber;
}

void fs_rename(tecnicofs* fs, char *oldNodeName, char *newNodeName) {
	int oldINumber = lookup(fs, oldNodeName);
	const int CONST = 10;
	int attempts = 0;

	int oldIndex = hash(oldNodeName, fs->hashSize);
	int newIndex = hash(newNodeName, fs->hashSize);

	while(1) {
		// Apagar trylocks
		if(!sync_trylock(&(fs->bstLock[oldIndex])) && !sync_trylock(&(fs->bstLock[newIndex]))) {
			if(oldINumber && !lookup(fs, newNodeName)) {
				fs->hashTable[oldIndex] = remove_item(fs->hashTable[oldIndex], oldNodeName);
				fs->hashTable[newIndex] = insert(fs->hashTable[newIndex], newNodeName, oldINumber);
				sync_unlock(&(fs->bstLock[oldIndex]));
				sync_unlock(&(fs->bstLock[newIndex]));
				break;
			}
			else {
				sync_unlock(&(fs->bstLock[oldIndex]));
				sync_unlock(&(fs->bstLock[newIndex]));
				return;
			}
		}
		else {
			if(attempts == 5) 
				break;
			attempts++;
			usleep(CONST * attempts);
		}
	}
}

void print_tecnicofs_tree(FILE *fp, tecnicofs *fs) {
	int i;

	for(i = 0; i < fs->hashSize; i++) {
		print_tree(fp, fs->hashTable[i]);
	}
}
