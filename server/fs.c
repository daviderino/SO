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

	tecnicofs *fs = malloc(sizeof(tecnicofs));
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
	free(fs->bstLock);
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
	int inumber = -1;
	node* searchNode = search(fs->hashTable[i], name);

	if (searchNode) {
		inumber = searchNode->inumber;
	}
	sync_unlock(&(fs->bstLock[i]));
	return inumber;
}

void swap_name(tecnicofs* fs, char *oldNodeName, char *newNodeName) {
	int oldIndex = hash(oldNodeName, fs->hashSize);
	int newIndex = hash(newNodeName, fs->hashSize);

	node *oldNode = NULL;
	node *newNode = NULL;

	if(oldIndex == newIndex) {
		sync_wrlock(&(fs->bstLock[oldIndex]));
	}
	else if(oldIndex < newIndex) {
		sync_wrlock(&(fs->bstLock[oldIndex]));
		sync_wrlock(&(fs->bstLock[newIndex]));
	}
	else {
		sync_wrlock(&(fs->bstLock[newIndex]));
		sync_wrlock(&(fs->bstLock[oldIndex]));
	}
	
	oldNode = search(fs->hashTable[oldIndex], oldNodeName);
	newNode = search(fs->hashTable[newIndex], newNodeName);

	if(oldNode != NULL && newNode == NULL) {
		int inumber = oldNode->inumber;
		fs->hashTable[oldIndex] = remove_item(fs->hashTable[oldIndex], oldNodeName);
		fs->hashTable[newIndex] = insert(fs->hashTable[newIndex], newNodeName, inumber);
	}

	if(oldIndex == newIndex) {
		sync_unlock(&(fs->bstLock[newIndex]));
	}
	else {
		sync_unlock(&(fs->bstLock[newIndex]));
		sync_unlock(&(fs->bstLock[oldIndex]));
	}

	return;
}

void print_tecnicofs_tree(FILE *fp, tecnicofs *fs) {
	int i;

	for(i = 0; i < fs->hashSize; i++) {
		print_tree(fp, fs->hashTable[i]);
	}
}
