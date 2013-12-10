//////////////////////
// filename: btree.h//
//////////////////////


#define NODE_SIZE 8
#define BLOCKSIZE 512

#include <stdlib.h>
#include <stdio.h>
#include <math.h>

/************************************************/
/*   Copyright@2002 by CIS,Universty of Guelph  */
/*   All rights reserved                        */
/*   Only for individual use in CIS*443 (W02)   */
/*   Any question, pls email:                    */
/*        ta443@cis.uoguelph.ca                     */
/************************************************/

typedef struct x_Node Node;
typedef struct x_Entry Entry;

struct x_Entry{
	char *key;//entry value;
	//char key[KEY_SIZE+1];
	int  blockID;//address of the key;
};

struct x_Node{
	//Node *parent; //pointer to parent;
	Node* ptr[NODE_SIZE+2]; //pointers to other nodes;
  Entry* entry[NODE_SIZE+1];//leave one extra space to
	                          //be used in splitting
	int  leaf;   //TRUE if this is a leaf, FALSE otherwise;
	int  root;   //TRUE if this is the root; otherwise FALSE;
	int  size;   //num of items stored in a node (=n)
	int  numEntry; //num of spaces being occupied (=<n)
	int toFile; //TRUE is this node has been written to file, FALSE otherwise
	int nodeNum; // number of the node as it gets printed to file
};

Node *pRoot = 0;

//initialize and return a node
Node *getNode() {
	Node *temp = (Node*)malloc(sizeof(Node));
	temp->leaf = 0;
	temp->root = 0;
	temp->size = NODE_SIZE;
	temp->numEntry = 0;
	temp->toFile = 0;
	temp->nodeNum = 0;
	int i;
	for (i=0; i<NODE_SIZE+1; i++) {
		temp->ptr[i] = NULL;
		temp->entry[i] = NULL;
	}
	temp->ptr[i] = NULL;
	if(pRoot == 0) pRoot = temp;
	return temp;
}

//initialize a existing pointer to a node
//NOTE: two parameters(leaf and root) won't be change!!
void resetNode(Node *temp) {
	temp->size = NODE_SIZE;
	temp->numEntry = 0;
	int i;
	for (i=0; i<NODE_SIZE; i++) {
		temp->ptr[i] = NULL;
		temp->entry[i] = NULL;
	}
	temp->ptr[i] = NULL;
}//resetNode()

//search tree for entry
Node* tree_search(Node *pNode, char *key) {
	if (pNode->leaf) return pNode;
	int i;
	for (i=0; i<pNode->numEntry; i++)
		if (strcmp(key, pNode->entry[i]->key)<0) {
			return tree_search(pNode->ptr[i], key);
		}//if
    return tree_search(pNode->ptr[i], key);
}//tree_search()

void sortNode(Node *pNode) {
	int i,j;
	Entry *ptr;
	Node  *ptrN;
	for (i=0; i<pNode->numEntry-1; i++)
		for (j=pNode->numEntry-1;i<j;j--)
			if (strcmp(pNode->entry[j]->key,pNode->entry[j-1]->key)<0) {
				//swap pointers to entry and pointers to node;
				ptr = pNode->entry[j];
				pNode->entry[j] = pNode->entry[j-1];
				pNode->entry[j-1] = ptr;
				ptrN = pNode->ptr[j];
				pNode->ptr[j] = pNode->ptr[j+1];
				pNode->ptr[j+1] = ptrN;
			}//if
}//sortNode()

// When is a node has space, this function adds an entry to this
//node in the correct order.NOTE: THE POINTER MAY PAIR WITH THIS
//ENTRY WILL NOT ADD TO THE NODE WITH THIS FUNCTION!
void addEntry(Node *pNode, Entry *entry) {
	int i, index;
	Entry *temp = (Entry*)malloc(sizeof(Entry));
	temp->key = malloc(sizeof(char)*(strlen(entry->key)+1));
	strcpy(temp->key,entry->key);
	temp->blockID = entry->blockID;

	for(index=0; index<pNode->numEntry; index++) {
		if(strcmp(entry->key, pNode->entry[index]->key) < 0) {
			break;
		}
	}
	for(i=pNode->numEntry; i>index; i--) {
		pNode->entry[i] = pNode->entry[i-1];
	}
	pNode->numEntry++;
	pNode->entry[index] = temp;
}//addEntry()

//addEntryPtr() inserts an entry to proper position together with
//a relative pointer to child node
void addEntryPtr(Node *pNode, Entry *entry, Node *newChildNode) {
	int i, index;
	Entry *temp = (Entry*)malloc(sizeof(Entry));
	temp->key = malloc(sizeof(char)*(strlen(entry->key)+1));
	strcpy(temp->key,entry->key);
	temp->blockID = entry->blockID;

	for(index=0; index<pNode->numEntry; index++) {
		if(strcmp(entry->key, pNode->entry[index]->key) < 0) {
			break;
		}
	}
	for(i=pNode->numEntry; i>index; i--) {
		pNode->entry[i] = pNode->entry[i-1];
		pNode->ptr[i+1] = pNode->ptr[i];
	}
	pNode->numEntry++;
	pNode->entry[index] = temp;
	pNode->ptr[index+1] = newChildNode;
}// addEntryPtr()

//newChildEntry stores the entry to be inserted in parent node
//newChildNode points to a brand new node after split
void insert(Node **pNode, Entry *entry, Node **newChildNode, Entry *newChildEntry) {

	int i;
	if ((*pNode)->leaf) {//current node is a leaf
		if ((*pNode)->numEntry<(*pNode)->size) {//current node still has space
			//put entry on it, set newChildEntry to NULL and return;
			addEntry(*pNode,entry);
			sortNode(*pNode);
			*newChildNode = NULL;
			return;
		}
		else {//the leaf is full
			Node *temp = getNode();
			Node *newNode = getNode();//temporary use to sort all nodes to be split;
			//let *newNode points to the same items as pNode does
			for (i=0; i<(*pNode)->numEntry; i++) {
				newNode->entry[i] = (*pNode)->entry[i];
				newNode->ptr[i] = (*pNode)->ptr[i];
				newNode->numEntry++;
			}
			newNode->ptr[i] = (*pNode)->ptr[i];
			//addEntry(newNode, newChildEntry);
			Entry *tempE = (Entry*)malloc(sizeof(Entry));
			tempE->key = malloc(sizeof(char)*(strlen(entry->key)+1));
			strcpy(tempE->key,entry->key);
			tempE->blockID = entry->blockID;
			newNode->entry[newNode->numEntry] = tempE;
			newNode->numEntry++;
			//set sibling point that pairs with the entry
			newNode->ptr[newNode->numEntry] = *newChildNode;
			int numStay = (int)ceil((double)(NODE_SIZE+1)/(double)2);
			resetNode(*pNode);//reset pointers to NULL,and relative parameters
			sortNode(newNode);
			for (i=0;i<numStay;i++)
				addEntry(*pNode, newNode->entry[i]);
			//copy the entry to be inserted in parent node into newChildEntry
			newChildEntry->key = malloc(sizeof(char)*(strlen(newNode->entry[i]->key)+1));
			strcpy(newChildEntry->key,newNode->entry[i]->key);
			newChildEntry->blockID = newNode->entry[i]->blockID;
			//now save the rest to a brand new node temp
			for (i=numStay; i<newNode->numEntry; i++)
			  	addEntry(temp, newNode->entry[i]);
			temp->leaf = 1;
			newNode = NULL;
			*newChildNode = temp;
			//special case: if this leaf is also the root
			if ((*pNode)->root) {
				Node *ptrR;
				ptrR = getNode();
				addEntry(ptrR,newChildEntry);
				ptrR->ptr[0] = *pNode;
				ptrR->ptr[1] = *newChildNode;
				ptrR->root = 1;
				(*pNode)->root = 0;
				//set the new root pointer by copying the value of ptrR;
				*pNode = ptrR;
				pRoot = ptrR;
				*newChildNode = NULL;
				newChildEntry = NULL;
			}
			return;
		}
	}
	else {// *pNode is a non-leaf node
		//locate a position
		for (i=0; i<(*pNode)->numEntry; i++)
			if (strcmp(entry->key, (*pNode)->entry[i]->key)<0) break;
		

		insert(&((*pNode)->ptr[i]), entry, newChildNode, newChildEntry);
		if (*newChildNode==NULL) return; //usual case;didn't split child;
		
		else {//child split,must insert newChildEntry in parent node

			if ((*pNode)->numEntry<(*pNode)->size) 
			{//have space
				addEntryPtr(*pNode,newChildEntry,*newChildNode);//copy newChildEntry
				*newChildNode = NULL;
				return;
			}//if
			else 
			{//current node has no space, must be split

				Node *temp = getNode();
				Node *newNode = getNode();//temporary use to sort all nodes to be split;
				//let *newNode points to the same items as pNode does
				for (i=0; i<(*(pNode))->numEntry; i++) 
				{
					newNode->entry[i] = (*pNode)->entry[i];
					newNode->ptr[i] = (*pNode)->ptr[i];
					newNode->numEntry++;
				}
				newNode->ptr[i] = (*pNode)->ptr[i];
				addEntryPtr(newNode, newChildEntry, *newChildNode);

				int numStay = (int)ceil((double)NODE_SIZE/(double)2);
				resetNode(*pNode);//reset pointers to NULL, and relative parameters
				for (i=0;i<numStay;i++) 
				{
					(*pNode)->entry[i] = newNode->entry[i];
					(*pNode)->ptr[i] = newNode->ptr[i];
				}
				(*pNode)->ptr[i] = newNode->ptr[i];
				(*pNode)->leaf = 0;
				(*pNode)->numEntry = numStay;

				//copy the entry to be inserted in parent node into newChildEntry
				newChildEntry->key = malloc(sizeof(char)*(strlen(newNode->entry[i]->key)+1));
				strcpy(newChildEntry->key,newNode->entry[i]->key);
				newChildEntry->blockID = newNode->entry[i]->blockID;

				//now save the rest to a brand new node temp
				numStay++;
				for (i=numStay; i<newNode->numEntry; i++) 
				{
					temp->entry[i-numStay] = newNode->entry[i];
					temp->ptr[i-numStay] = newNode->ptr[i];
					temp->numEntry++;
				}//for
				temp->ptr[i-numStay] = newNode->ptr[i];
				newNode = NULL;
				*newChildNode = temp;

				if ((*pNode)->root) 
				{ //root node was just split
					Node *newRoot;
					newRoot = getNode();
					addEntry(newRoot,newChildEntry);
					newRoot->root = 1;
					(*pNode)->root = 0;
					newRoot->ptr[0] = *pNode;
					newRoot->ptr[1] = *newChildNode;
					*pNode = newRoot;
					pRoot = newRoot;
				}//if
				return;
			}//else
		}//else
	}//else
}//insert()

int deleteEntry(Node *pNode, Entry *entry) {
	Entry *tempE ;
	int i;
	//locate the key
    for (i=0; i<pNode->numEntry; i++)
		if (strcmp(entry->key,pNode->entry[i]->key)==0) break;
	if (i==pNode->numEntry) {
		return 1; // no deletion
/*
		tempE = pNode->entry[i];
		pNode->entry[i] = NULL;
		pNode->ptr[i+1] = NULL;
        if (pNode->leaf&&!(pNode->root)) free(tempE);
		pNode->numEntry--;
*/
	}//if
	else {//delete current entry;
		tempE = pNode->entry[i];
		pNode->entry[i] = NULL;
        if (pNode->leaf&&!(pNode->root)) free(tempE);
		while (i<pNode->numEntry-1) {
			pNode->entry[i] = pNode->entry[i+1];
			i++;
			pNode->ptr[i] = pNode->ptr[i+1];
		}
		pNode->entry[i] = NULL;
		pNode->ptr[i+1] = NULL;
		pNode->numEntry--;
		return 0;
	}//else
}//deleteEntry()

//isCanSibling() return 1 if the # of keys of the sibling
//is greater than required minimum; 0 otherwise;
int isCandidateSibling(Node *pNode, int index, int min) {
	if (pNode->numEntry>min) return 1;
    else return 0;
}

//getSibling() returns 0 if there is no candidate found;
// 1-- the left sibling is a candidate or
// 2-- indicates the right sibling is a candidate
//PRE:min -- the minimum required space
//    index -- the index of current pointer in its parent's pointers
int getSibling(Node *parent, int index, int min) {

	if (index==parent->numEntry)//the case index if the most right position
	{
		if (parent->ptr[index-1]->numEntry>min)
			return 1;
		else 
			return 0;
	}
	if (index==0)//the case index if the most left position
	{
		if (parent->ptr[index+1]->numEntry>min) 
			return 2;
		else 
			return 0;
	}
	if (parent->ptr[index-1]->numEntry>min) 
		return 1;
	if (parent->ptr[index+1]->numEntry>min) 
		return 2;
	
	return 0;
}

//borrow an entry from the right sibling
void redistributeLeft(Node *parent, Node *me) {
	int i,index;
	for (index=0;index<parent->numEntry;index++)
	{
		if (parent->ptr[index]==me)
			break;
	}
	index++;
	me->entry[me->numEntry] = parent->ptr[index]->entry[0];
	me->numEntry++;
	for (i=0;i<parent->ptr[index]->numEntry-1;i++)
		parent->ptr[index]->entry[i] = parent->ptr[index]->entry[i+1];
	parent->ptr[index]->entry[i] = NULL;
	parent->ptr[index]->numEntry--;
	//now copy sibling's most left entry to the appropriate
	//position in its parent
	parent->entry[index-1]->key = malloc(sizeof(char)*(strlen(parent->ptr[index]->entry[0]->key)+1));
	strcpy(parent->entry[index-1]->key,parent->ptr[index]->entry[0]->key);
	parent->entry[index-1]->blockID = parent->ptr[index]->entry[0]->blockID;

}

//Non-leaf version: borrow an entry from the right sibling
void redistributeLeft2(Node *parent, Node *me) {
	int i,index;
	for (index=0;index<parent->numEntry;index++)
		if (parent->ptr[index]==me) break;
	index++;
	me->entry[me->numEntry] = parent->entry[index-1];
	me->ptr[me->numEntry+1] = parent->ptr[index]->ptr[0];
	me->numEntry++;
	parent->entry[index-1] = parent->ptr[index]->entry[0];
	for (i=0;i<parent->ptr[index]->numEntry-1;i++) {
		parent->ptr[index]->entry[i]= parent->ptr[index]->entry[i+1];
		parent->ptr[index]->ptr[i]= parent->ptr[index]->ptr[i+1];
	}
	parent->ptr[index]->ptr[i] = parent->ptr[index]->ptr[i+1];
	parent->ptr[index]->entry[i] = NULL;
	parent->ptr[index]->ptr[i+1] = NULL;
	parent->ptr[index]->numEntry--;
}

//borrow an entry from the left sibling
void redistributeRight(Node *parent, Node *me) {
	int i,index;
	
//	printf("rR - %s :: %s\n", parent->entry[0]->key, me->entry[0]->key);
//printf("total %d :: \n", me->numEntry);

	for (index=0;index<parent->numEntry;index++)
	{
		if (parent->ptr[index]==me)
			break;
	}
	for (i=me->numEntry;i>0;i--)
	{
		me->entry[i] = me->entry[i-1];
//		printf("Swap forward %s :: \n", me->entry[i]->key);
	}
	me->entry[0]= parent->ptr[index-1]->entry[parent->ptr[index-1]->numEntry-1];
//	printf("Last %s :: \n", me->entry[0]->key);
	me->numEntry++;
//	printf("total %d :: \n", me->numEntry);
	parent->ptr[index-1]->numEntry--;
	parent->ptr[index-1]->entry[parent->ptr[index-1]->numEntry] = NULL;
	parent->entry[index-1]->key = malloc(sizeof(char)*(strlen(me->entry[0]->key)+1));
	strcpy(parent->entry[index-1]->key,me->entry[0]->key);
	parent->entry[index-1]->blockID = me->entry[0]->blockID;

}

//Non-leaf version: borrow an entry from the left sibling
void redistributeRight2(Node *parent, Node *me) {
	int i,index;
	for (index=0;index<parent->numEntry;index++)
		if (parent->ptr[index]==me) break;
	index--;
	for (i=me->numEntry;i>0;i--) {
		me->entry[i] = me->entry[i-1];
		me->ptr[i+1] = me->ptr[i];
	}
	me->entry[0] = parent->entry[index];
	me->ptr[0]
		= parent->ptr[index]->ptr[parent->ptr[index]->numEntry+1];
	me->numEntry++;
	parent->entry[index]
		= parent->ptr[index]->entry[parent->ptr[index]->numEntry];
    parent->ptr[index]->entry[parent->ptr[index]->numEntry] = NULL;
    parent->ptr[index]->ptr[parent->ptr[index]->numEntry+1] = NULL;
	parent->ptr[index]->numEntry--;
}

//mergeRight() merges *pNode with its right sibling
//i -- the index of *pNode in its parent
void mergeRight(Node *ptrParent,Node *pNode,int i) {
	int j;
	for (j=0;j<ptrParent->ptr[i+1]->numEntry;j++)
		pNode->entry[pNode->numEntry++] = ptrParent->ptr[i+1]->entry[j];
	Node *temp = ptrParent->ptr[i+1];
	ptrParent->ptr[i+1] = NULL;
	free(temp);
}

//mergeRight2() merges *pNode with its right sibling
//i -- the index of *pNode in its parent
void mergeRight2(Node *ptrParent,Node *pNode,int i) {
	int j;
    //pull down the appropriate key from the parent
	pNode->entry[pNode->numEntry] = ptrParent->entry[i];
	pNode->numEntry++;
	pNode->ptr[pNode->numEntry] = ptrParent->ptr[i+1]->ptr[0];
	//merge the two nodes
	for (j=0;j<ptrParent->ptr[i+1]->numEntry;j++) {
		pNode->entry[pNode->numEntry++]
			= ptrParent->ptr[i+1]->entry[j];
		pNode->ptr[pNode->numEntry]
			= ptrParent->ptr[i+1]->ptr[j+1];
	}
    //adjust parent pointers between this splitting key, pointing to
	//the same node after merging; discard the space pNode points to
	Node *temp = ptrParent->ptr[i+1];
	ptrParent->ptr[i+1] = pNode;
	free(temp);
}


//mergeLeft() merges *pNode with its left sibling
//i -- the index of *pNode in its parent
void mergeLeft(Node *ptrParent,Node *pNode,int i) {
	int j;
	for (j=0;j<pNode->numEntry;j++)
		ptrParent->ptr[i-1]->entry[ptrParent->ptr[i-1]->numEntry++]
			= pNode->entry[j];
	Node *temp = pNode;
	ptrParent->ptr[i] = NULL;//NOTE: change pNode to NULL!!!
	free(temp);
}

//Non-leaf version mergeLeft2() merges *pNode with its left sibling
//i -- the index of *pNode in its parent
void mergeLeft2(Node *ptrParent,Node *pNode,int i) {
	int j;
    //pull down the appropriate key from the parent
	ptrParent->ptr[i-1]->entry[ptrParent->ptr[i-1]->numEntry]
		    = ptrParent->entry[i-1];
	ptrParent->ptr[i-1]->numEntry++;
	ptrParent->ptr[i-1]->ptr[ptrParent->ptr[i-1]->numEntry]
		= pNode->ptr[0];
	//move all entries with pointers to the left sibling
	for (j=0;j<pNode->numEntry;j++) {
		ptrParent->ptr[i-1]->entry[ptrParent->ptr[i-1]->numEntry++]
			= pNode->entry[j];
		ptrParent->ptr[i-1]->ptr[ptrParent->ptr[i-1]->numEntry]
			= pNode->ptr[j+1];
	}
    //adjust parent pointers between this splitting key,
	//pointing to the same node after merging
	ptrParent->ptr[i] = ptrParent->ptr[i-1];
	//delete the empty node
	Node *temp = pNode;
	free(temp);
}

void deletion(Node *ptrParent, Node **pNode, Entry *entry, Entry **oldChildEntry) {
	int i, minInteriorSpace = (int)ceil((double)(NODE_SIZE+1)/(double)2) - 1;
	int minLeafSpace = (int)floor((double)(NODE_SIZE+1)/(double)2);
	Node *temp;

    if ((*pNode)->root)
    	ptrParent = *pNode;
	if ((*pNode)->leaf) { //*pNode is a leaf node
//		printf("founf leaf %s - %d :: MinL - %d\n", (*pNode)->entry[0]->key, (*pNode)->numEntry, minLeafSpace);

		// recover parent key if it's 1st leaf of the subtree
		if (strcmp(entry->key,(*pNode)->entry[0]->key)==0
			&& ptrParent!=NULL && !(*pNode)->root)
		{
//			printf("1st leaf of subtree\n");

			int find=0;
			for(i=0; i<ptrParent->numEntry+1; i++)
			{
				if(ptrParent->entry[i]==NULL) break;
				if(strcmp(ptrParent->entry[i]->key, (*pNode)->entry[0]->key)==0)
				{
					find = 1;
					break;
				}
			}
			if(find)
			{
				ptrParent->entry[i]->key = malloc(sizeof(char)*(strlen((*pNode)->entry[1]->key)+1));
				strcpy(ptrParent->entry[i]->key, (*pNode)->entry[1]->key);
				ptrParent->entry[i]->blockID = (*pNode)->entry[1]->blockID;
			}
		}

		int del;
//		printf("before:: %d\n", (*pNode)->numEntry);
		del = deleteEntry(*pNode,entry);
//		printf("Del:: %d - %d\n", del, (*pNode)->numEntry);

		//*pNode has entries to spare, remove entry, set oldChildEntry to NULL;
		if ((*pNode)->numEntry+del>= minLeafSpace || (*pNode)->root) {
//			printf("removing\n");
			*oldChildEntry = NULL;
			return;
		}//if
		else {//the # of keys less than the required minimum
            //locate the index of *pNode in its parent
			for (i=0;i<ptrParent->numEntry+1;i++)
			{
				if (ptrParent->ptr[i]==*pNode) break;
			}
			if(i==ptrParent->numEntry+1)return;
			//get a sibling
			int result;
			result = getSibling(ptrParent,i,minLeafSpace);

			if (result==1) {//borrow an entry from left
				redistributeRight(ptrParent,*pNode);
				*oldChildEntry = NULL;
				return;
			}//if
			if (result==2) {//borrow an entry from right sibling
				redistributeLeft(ptrParent,*pNode);
				*oldChildEntry = NULL;
				return;
			}//if
			if (result==0) {//no candidate siblings then merge
//				printf("NO siblings to steal - Need to merge\n");

				if (i==ptrParent->numEntry) {//merge toward left
//					printf("Merge LEFT\n");

					*oldChildEntry = ptrParent->entry[i-1];
					long add1, add2;
					add1 = (long)pNode;
					add2 = (long)ptrParent;
					if(add1==add2+sizeof(int))
					{
						pRoot->root = 1;
//						pRoot->leaf = 1;
						pRoot->leaf = 0;
					}
					mergeLeft(ptrParent,*pNode,i);
					return;
				}
				else {//merge toward right
//					printf("Merge RIGHT\n");

					*oldChildEntry = ptrParent->entry[i];
					long add1, add2;
					add1 = (long)pNode;
					add2 = (long)ptrParent;
					if(add1==add2)
					{
						pRoot->root = 1;
//						pRoot->leaf = 1;
						pRoot->leaf = 0;
					}
					mergeRight(ptrParent,*pNode,i);
					return;
				}
			}
		}//else
	}//if node is a leaf
	else {//if this is not a non-leaf node
		//locate a position i
		for (i=0; i<(*pNode)->numEntry; i++)
		{
			if (strcmp(entry->key, (*pNode)->entry[i]->key)<0)
				break;
		}
//		printf("Found non=leaf first, calling deletion again\n");

		deletion(*pNode, &((*pNode)->ptr[i]), entry, oldChildEntry);
		if (*oldChildEntry==NULL)
		{
//			printf("Returning Null\n");

			return;
		}
        deleteEntry(*pNode,*oldChildEntry);
		//if the root become empty after deletion, set root
		//pointer to its only child, if it has one
		if ((*pNode)->root) {
//			printf("root emtpy\n");

			if ((*pNode)->numEntry==0) {
				temp = *pNode;
				*pNode = (*pNode)->ptr[0];
				free(temp);
			}
			*oldChildEntry = NULL;
			return;
		}
		//*pNode has entries to spare, remove entry, set oldChildEntry to NULL;
		if ((*pNode)->numEntry>=minInteriorSpace) {
//			printf("Entries to spare\n");

			*oldChildEntry = NULL;
			return;
		}//if
		else {//the # of keys less than the required minimum
//			printf("--NON-leaf NODE--\n");

            //locate the index of *pNode in its parent
			for (i=0;i<ptrParent->numEntry+1;i++)
				if (ptrParent->ptr[i]==*pNode) break;
			if(i==ptrParent->numEntry+1) return;
			//get a sibling
			int result = getSibling(ptrParent,i,minInteriorSpace);
			if (result==1) {//borrow an entry from left
//				printf("Get from LEFT\n");

				redistributeRight2(ptrParent,*pNode);
				*oldChildEntry = NULL;
				return;
			}//if
			if (result==2) {//borrow an entry from right sibling
//				printf("Get from RIGHT\n");

				redistributeLeft2(ptrParent,*pNode);
				*oldChildEntry = NULL;
				return;
			}//if
			if (result==0) {//no candidate siblings then merge
//				printf("MERGE\n");

				if (i==ptrParent->numEntry) {//merge toward left
//					printf("MERGE = left\n");

					*oldChildEntry = ptrParent->entry[i-1];
					mergeLeft2(ptrParent,*pNode,i);
					return;
				}
				else {//merge toward right
//					printf("MERGE = rigth\n");

					*oldChildEntry = ptrParent->entry[i];
					mergeRight2(ptrParent,*pNode,i);
					return;
				}
			}
		}
	}//else--non-leaf node
}//deletion()

//retrieve() will return 1 and return key&ID in *entry,
//if the key found. 0 otherwise.
int retrieve(Node *ptr,char *key,Entry *entry) {
	int i,flag;
	//search to a leaf
	while (!ptr->leaf) {
		flag = 1;
		for (i=0; i<ptr->numEntry; i++)
			if (strcmp(key,ptr->entry[i]->key)<0) {
				ptr = ptr->ptr[i];
				flag = 0;
				break;
			}
        if (flag) ptr = ptr->ptr[i];
	}
	
	int difference_Prev = 0;
	int difference_Current = 0;
    for (i=0; i<ptr->numEntry; i++)
	{
		difference_Prev = difference_Current;
		difference_Current = strcmp(key,ptr->entry[i]->key);
		
		if (strcmp(key,ptr->entry[i]->key)==0) {
			entry->key = malloc(sizeof(char)*(strlen(ptr->entry[i]->key)+1));
			strcpy(entry->key,ptr->entry[i]->key);
			entry->blockID = ptr->entry[i]->blockID;
			return 1;
		}
		
		// for inserting - send back entry that should come before the new one
		if(difference_Prev == 0 && difference_Current< 0)
		{
			entry->key = malloc(sizeof(char)*(1)); // special case of going at the beginning of the first block in the database
			entry->key[0] = '\0';
			entry->blockID = 0;
			return 0;
			
		}
		else if(difference_Prev > 0 && difference_Current < 0)
		{
			entry->key = malloc(sizeof(char)*(strlen(ptr->entry[i-1]->key)+1));
			strcpy(entry->key,ptr->entry[i-1]->key);
			entry->blockID = ptr->entry[i-1]->blockID;
			return 0;
		}
	
	}
	
	// return the last record
	entry->key = malloc(sizeof(char)*(strlen(ptr->entry[i-1]->key)+1));
	strcpy(entry->key,ptr->entry[i-1]->key);
	entry->blockID = ptr->entry[i-1]->blockID;
	
	return 0;
}//retrieve()


//-----------------------------------//
/* Laura Richards - 0546192 
 * CIS*6030 - Assign#1
 */


// writes the nodes to file
void writeNode(Node* root, int leaf_Flag, int treeFlag)
{
	// open file to write buffer to
	size_t fileName = 0;
	if(treeFlag == 0) // primary
		fileName = open("txtFiles/PrimaryIndex.txt", O_CREAT|O_WRONLY|O_APPEND, S_IRWXU);
	if(treeFlag == 1)
		fileName = open("txtFiles/SecondaryIndex.txt", O_CREAT|O_WRONLY|O_APPEND, S_IRWXU);
	
	if(fileName < 0)
		printf("------------------------PROBLEMS-------------------------------\n");
	
	
	int blockSize = 512;
	
	//create the buffer
	char* buffer;
	buffer = malloc(sizeof(char)*blockSize);
	memset(buffer, 0, blockSize);
	buffer[blockSize] = '\0';
	
	
	int root_i;
	if(leaf_Flag == 0)
	{
		//I#keys|key %d_key %d|ptr_ptr_ptr|
		sprintf(buffer, "I%d|%s %d", root->numEntry, root->entry[0]->key,root->entry[0]->blockID);
		for(root_i=1; root_i < root->numEntry; root_i++) // look at the far right child frist
			sprintf(buffer,"%s_%s %d", buffer, root->entry[root_i]->key, root->entry[root_i]->blockID);

		
		sprintf(buffer, "%s|%d", buffer, root->ptr[0]->nodeNum);
		for(root_i=1; root_i < root->numEntry; root_i++) // look at the far right child frist
			sprintf(buffer,"%s_%d", buffer, root->ptr[root_i]->nodeNum);

		sprintf(buffer,"%s_%d|", buffer, root->ptr[root_i]->nodeNum);
		
		// write to memory
		write(fileName, buffer, blockSize);
		
	}
	else if (leaf_Flag == 1)
	{
		//L#keys|key_block|
		sprintf(buffer, "L%d|", root->numEntry);
		for(root_i=0; root_i < root->numEntry; root_i++) 
			sprintf(buffer,"%s%s_%d|", buffer, root->entry[root_i]->key, root->entry[root_i]->blockID);
		
		// write to memory
		write(fileName, buffer, blockSize);		
	}	
	
	close(fileName);
}


// traverses through the nodes and sets their node number and prints them to file, starting with the right most child.
int treeToFile(Node* root, int nodeCounter, int treeFlag)
{
	int root_i;
	
	if(root->toFile == 1) // has already been printed to file
		return nodeCounter;
	else // hasn't been printed; go down to the next rightmost node
	{
		if(root->leaf == 0) // loop through ptrs
		{
			for(root_i=root->numEntry; root_i>=0; root_i--) // look at the far right child frist
			{
				if(root->ptr[root_i] != 0)
					nodeCounter = treeToFile(root->ptr[root_i], nodeCounter++, treeFlag);
			}
			
			// now to print the interior node
			root->toFile = 1;
			root->nodeNum = nodeCounter;
			nodeCounter++;
			
			writeNode(root, 0, treeFlag);
		}
		else // special case for ptr, it is a leaf (print to file right away)
		{
			root->toFile = 1;
			root->nodeNum = nodeCounter;
			nodeCounter++;
			
			writeNode(root, 1, treeFlag);
			
			return nodeCounter;
		}
	}	
	
	return nodeCounter;
}