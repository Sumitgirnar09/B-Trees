/* Laura Richards  
 * CIS*6030 - Assign#1
 */

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "btree.h"


// returns the length of the given number
int numDigits(int n) {
    if (n < 10)
		return 1;
    return (1 + numDigits(n / 10));
}


// loops through the header information and looks for the record that corresponds to the given key
// returns and Int array of - record offset, record length, header offset, header length, overflow block#, buffer offset of prev in block record
int* getRecordInfo(char* key, char* bufferCopy, int treeFlag, int fileName)
{
	int* recInfo;
	recInfo = malloc(sizeof(int)*6); // for storing the found record info
	
	char* recordsAll;
	recordsAll = strchr(bufferCopy, '^'); 
	
	char* header; // stores the header info of the current block
	header = malloc(sizeof(char)*(strlen(bufferCopy) - strlen(recordsAll) +1));
	memcpy(header, bufferCopy, strlen(bufferCopy) - strlen(recordsAll));
	header[ strlen(bufferCopy) - strlen(recordsAll)] = '\0';
	
	// jump pass the ^
	recordsAll = recordsAll+1; // points to the beginning of the records in the buffer
	
	int buffer_offset = 0;
	int record_length = 0;
	int header_offset = 0;
	int header_length = 0;
	
	// loop and find the offset and length of record for reading in
	// compare keys for a match
	char* recordInfo = header;
	char* keyP = malloc(sizeof(char));
	char* currentRecord  = malloc(sizeof(char));
	
	do{		
		if(recordInfo[0] == 'O') // deal will getting overflow block information
		{
			int OFB_recOff, OFB_recLeng, OFB_num;
			record_length = 0;
			
			// grab out overflow record info from header
			sscanf(recordInfo, "O_%d_%d_%d%*s", &OFB_num, &OFB_recOff, &OFB_recLeng );
			header_length = numDigits(OFB_num)+numDigits(OFB_recOff)+numDigits(OFB_recLeng)+4;
			
			// create the buffer
			char* buffer;
			buffer = malloc(sizeof(char)*BLOCKSIZE);
			memset(buffer, 0, BLOCKSIZE);
			
			// seek to and read the overflow block from the database
			if(lseek(fileName, OFB_num*BLOCKSIZE,SEEK_SET) < 0) 
				printf("---PROBLEMS SEEKING---\n");
			read(fileName,buffer,BLOCKSIZE);
			
			currentRecord = realloc(currentRecord, sizeof(char)*(OFB_recLeng+1));
			memcpy(currentRecord, buffer+OFB_recOff, OFB_recLeng);
			currentRecord[OFB_recLeng] = '\0';
			
			// get field offsets
			char* field1_end = strchr(currentRecord, '|');
			char* field2_end = strrchr(currentRecord, '|');
			
			if(treeFlag == 0) // pull out primary key
			{
				keyP = realloc(keyP, sizeof(char)*(field2_end-field1_end));
				memcpy(keyP, currentRecord+(field1_end - currentRecord)+1, field2_end-field1_end-1);
				keyP[field2_end-field1_end-1] = '\0';
			}
			else if (treeFlag == 1) // pull out secondary key
			{
				keyP = realloc(keyP, sizeof(char)*(field1_end-currentRecord+1));
				memcpy(keyP, currentRecord, field1_end-currentRecord);
				keyP[field1_end-currentRecord] = '\0';
			}

			// look for a match and send back the corresponding info
			if(strcmp(key, keyP) == 0)
			{
				free(buffer);
				
				recInfo[0] = OFB_recOff;
				recInfo[1] = OFB_recLeng;
				recInfo[2] = header_offset;
				recInfo[3] = header_length;
				recInfo[4] = OFB_num;
				recInfo[5] = buffer_offset;
				
				return recInfo;
			}
			free(buffer);
		}
		else
		{
			sscanf(recordInfo, "%d%*s", &record_length);
			
			header_length = numDigits(record_length);
			
			// pull out record that belongs to the current header info
			currentRecord = realloc(currentRecord, sizeof(char)*(record_length+1));
			memcpy(currentRecord, recordsAll+buffer_offset, record_length);
			currentRecord[record_length] = '\0';
			
			// get field offsets
			char* field1_end = strchr(currentRecord, '|');
			char* field2_end = strrchr(currentRecord, '|');
			
			if(treeFlag == 0) // pull out primary key
			{
				keyP = realloc(keyP, sizeof(char)*(field2_end-field1_end));
				memcpy(keyP, currentRecord+(field1_end - currentRecord)+1, field2_end-field1_end-1);
				keyP[field2_end-field1_end-1] = '\0';
			}
			else if (treeFlag == 1) // pull out secondary key
			{
				keyP = realloc(keyP, sizeof(char)*(field1_end-currentRecord+1));
				memcpy(keyP, currentRecord, field1_end-currentRecord);
				keyP[field1_end-currentRecord] = '\0';
			}
			
			if(strcmp(key, keyP) == 0)
			{
				recInfo[0] = buffer_offset;
				recInfo[1] = record_length;
				recInfo[2] = header_offset;
				recInfo[3] = header_length;
				recInfo[4] = -1;
				recInfo[5] = -1;
				return recInfo;
			}
		}
		
		recordInfo = strchr(recordInfo, '|');
		if(recordInfo != NULL)
		{
			header_offset = strlen(header) - strlen(recordInfo);
			buffer_offset = buffer_offset+record_length;
			recordInfo = recordInfo+1; // jump past '|'		
		}
		
	}while(recordInfo != NULL);
	
	
	free(currentRecord);
	free(keyP);
	free(header);
	
	recInfo[0] = -1;
	recInfo[1] = -1;
	recInfo[2] = -1;
	recInfo[3] = -1;
	recInfo[4] = -1;
	recInfo[5] = -1;
	return recInfo;
}


// deletes the corresonding record and header info from the block/overflow block
char* deleteRecord(int blockID, char* keyDEL)
{
	char* secondaryKey;
	
	size_t fileName = open("txtFiles/database.txt", O_RDWR);
	if(fileName < 0)
		printf("---PROBLEMS---\n");
	//--------//
	
	// create the buffer
	char* buffer;
	buffer = malloc(sizeof(char)*BLOCKSIZE);
	memset(buffer, 0, BLOCKSIZE);
	
	// seek to and read the block from the database
	if(lseek(fileName,blockID*BLOCKSIZE,SEEK_SET) < 0) 
		printf("---PROBLEMS---\n");
	read(fileName,buffer,BLOCKSIZE);
	
	//----------//
	
	// make copy of buffer for changing
	char* bufferCopy;
	bufferCopy = malloc(sizeof(char)*BLOCKSIZE);
	memcpy(bufferCopy, buffer, BLOCKSIZE);
	
	char* recordsAll;
	recordsAll = strchr(buffer, '^'); 
	
	char* header; // holds all the header info
	header = malloc(sizeof(char)*(strlen(buffer) - strlen(recordsAll) +1));
	memcpy(header, buffer, strlen(buffer) - strlen(recordsAll));
	header[ strlen(buffer) - strlen(recordsAll)] = '\0';
	
	// jump past ^
	recordsAll = recordsAll+1; // points to the start of the records
	
	int* recInfo;
	char* cR;
	recInfo = getRecordInfo(keyDEL, buffer, 0, fileName); // assuming its primary key
	
	
	if(recInfo[4] >= 0)// dealing with record in overflow block
	{
		if(lseek(fileName,recInfo[4]*BLOCKSIZE,SEEK_SET) < 0) 
			printf("---PROBLEMS---\n");
		read(fileName,buffer,BLOCKSIZE);
		
		cR = malloc(sizeof(char)*(recInfo[1]+1));
		memcpy(cR, buffer+recInfo[0], recInfo[1]);
		cR[recInfo[1]] = '\0';
	}
	else
	{
		cR = malloc(sizeof(char)*(recInfo[1]+1));
		memcpy(cR, recordsAll+recInfo[0], recInfo[1]);
		cR[recInfo[1]] = '\0';
	}
	
	printf("\nDeleting Record : %s\n", cR);
	
	// get out secondary key for return
	char* secondaryKeyEnd = strchr(cR, '|');
	secondaryKey = malloc(sizeof(char)*((secondaryKeyEnd - cR) +1));
	memcpy(secondaryKey, cR, (secondaryKeyEnd - cR));
	secondaryKey[(secondaryKeyEnd - cR)] = '\0';
		
	char* moveString;
	if(recInfo[4] >= 0)// delete from overflow block
	{
		// seek to overflow block
		if(lseek(fileName,recInfo[4]*BLOCKSIZE,SEEK_SET) < 0) 
			printf("---PROBLEMS SEEKING---\n");
		read(fileName,buffer,BLOCKSIZE);
				
		moveString = buffer+recInfo[0];
		memset(moveString, ' ', recInfo[1]); // just clear memory, don't move things down
		
		// write the updated block back
		if(lseek(fileName,recInfo[4]*BLOCKSIZE,SEEK_SET) < 0) 
			printf("---PROBLEMS---\n");
		write(fileName,buffer,BLOCKSIZE);
	}
	else
	{
		// shift next record up by deleted record length, clear tail of the buffer
		moveString = bufferCopy+strlen(header)+1+recInfo[0];
		memmove(moveString, moveString+recInfo[1], strlen(moveString+recInfo[1]));
		memset(bufferCopy+strlen(bufferCopy)-recInfo[1], 0, recInfo[1]);
	}
	
	// remove the corresponding header info
	moveString = bufferCopy+recInfo[2];
	memmove(moveString, moveString+recInfo[3]+1, strlen(moveString+recInfo[3])-1);
	memset(bufferCopy+ strlen(bufferCopy) - (recInfo[3]+1), 0 , recInfo[3]+1);	
	
	// seek to and write the block back into the database
	if(lseek(fileName,blockID*BLOCKSIZE,SEEK_SET) < 0) 
		printf("---PROBLEMS---\n");
	write(fileName,bufferCopy,BLOCKSIZE);
	
	free(buffer);
	free(cR);
	free(header);
	free(recInfo);
	close(fileName);
	
	return secondaryKey;
}



// finds and prints out the total record that belongs to the key
void readData(int blockID, char* key, int treeFlag)
{
	size_t fileName = open("txtFiles/database.txt", O_RDONLY);
	if(fileName < 0)
		printf("---PROBLEMS OPENING FILE---\n");
	//--------//
	
	// create the buffer
	char* buffer;
	buffer = malloc(sizeof(char)*BLOCKSIZE);
	memset(buffer, 0, BLOCKSIZE);
	
	// seek to and read the block from the database
	if(lseek(fileName,blockID*BLOCKSIZE,SEEK_SET) < 0) 
		printf("---PROBLEMS---\n");
	read(fileName,buffer,BLOCKSIZE);
	
	//--------//
	
	char* recordsAll; 
	recordsAll = strchr(buffer, '^');
	recordsAll = recordsAll+1; // points to the start of the records in the current buffer
	
	int* recInfo;
	recInfo = getRecordInfo(key, buffer, treeFlag, fileName); // gets the offset info of record so it can be read from the buffer
	
	char* cR;
	if(recInfo[4] >= 0) // printing out from overflow block
	{
		if(lseek(fileName,recInfo[4]*BLOCKSIZE,SEEK_SET) < 0) 
			printf("---PROBLEMS SEEKING---\n");
		read(fileName,buffer,BLOCKSIZE);
		
		cR = malloc(sizeof(char)*(recInfo[1]+1));
		memcpy(cR, buffer+recInfo[0], recInfo[1]);
		cR[recInfo[1]] = '\0';
	}
	else
	{
		cR = malloc(sizeof(char)*(recInfo[1]+1));
		memcpy(cR, recordsAll+recInfo[0], recInfo[1]);
		cR[recInfo[1]] = '\0';
	}
	
	printf("\nRecord FOUND: %s\n", cR);
	
	free(buffer);
	free(cR);
	free(recInfo);
	close(fileName);	
}


// searches through the corresponding index to find a given key
void searchIndex(int treeFlag, char* key)
{
	// open file
	size_t fileName;
	if(treeFlag == 0) // primary
		fileName = open("txtFiles/PrimaryIndex.txt", O_RDONLY);
	if(treeFlag == 1) // secondary
		fileName = open("txtFiles/SecondaryIndex.txt", O_RDONLY);
	if(fileName < 0)
		printf("---PROBLEMS OPENING FILE---\n");
	
	//------//
	
	int readCounter = 0; // to keep track of the number of disk reads
	
	// create the buffer
	char* buffer;
	buffer = malloc(sizeof(char)*BLOCKSIZE);
	memset(buffer, 0, BLOCKSIZE);
	
	// seek to and read the head of the tree
	if(lseek(fileName,0-BLOCKSIZE,SEEK_END) < 0) 
		printf("---PROBLEMS SEEKING---\n");
	read(fileName,buffer,BLOCKSIZE);
	readCounter++;
	
	//------//
	
	printf("\n");
	
	// step through the corresponding index file
	int offset = 0; // offset for reading in the next blocks
	while(buffer[0] != 'L') // while not a leaf - FORM::  I|key block#_key block#|ptr_ptr_ptr|
	{		
		// break up the node
		char* keyPtr;
		char* indexPtr;
		keyPtr = strtok(buffer, "|"); // eat the header
		keyPtr = strtok(NULL, "|"); // will now point to the string of keys
		printf("Node-I Keys: %s\n", keyPtr); // need to be displayed
		indexPtr = strtok(NULL, "|"); // will now point to the string of ptrs
		
		// loop through keys first
		char* keys = strtok(keyPtr, "_");
		
		// grab out the two strings that make up the key
		int keyL_max = strlen(keys);
		char* keyInfo;
		keyInfo = malloc(sizeof(char)*(strlen(keys)+1));
		strcpy(keyInfo, keys);
		keyInfo[strlen(keys)] = '\0';
		
		int key_count = 0;
		while(keys != NULL) // loop until key is found
		{
			char grabKey1[keyL_max], grabKey2[keyL_max];
			int i = 0;
			for( i = 0; i < keyL_max; i++)
			{
				grabKey1[i] = '\0';
				grabKey2[i] = '\0';
			}
			
			sscanf(keys, "%s %s %*d ", grabKey1, grabKey2);
			grabKey1[strlen(grabKey1)] = ' ';
			strcat(grabKey1, grabKey2);

			if (strcmp(key,grabKey1)<0)  // next ptr found
				break;
			
			key_count++;
			keys = strtok(NULL, "_");
		}
		
		// Find the corresponding ptr
		char* indexes = strtok(indexPtr, "_");
		int i = 0;
		for(i = 0; i < key_count; i++)
			indexes = strtok(NULL, "_");
		
		offset = atoi(indexes); // turn corresponding ptr to a number so it can be used in lseek

		
		// clear buffer - seek to and read the next block
		memset(buffer, 0, BLOCKSIZE);
		if(lseek(fileName,(offset-1)*BLOCKSIZE,SEEK_SET) < 0) 
			printf("---PROBLEMS---\n");
		read(fileName,buffer,BLOCKSIZE);
		readCounter++;
	}
	
	// now have a leaf in the buffer - find the key the user is searching for
	int blockID;
	int foundFlag = 0;
	
	printf("Node-L Keys: %s\n", buffer); // need to be displayed FORM:: L|key_block#|key_block#|key_block#|
	
	char* record;
	record = strtok(buffer, "|"); // eat the header
	record = strtok(NULL, "|");
	
	char* record_key;
	record_key = malloc(sizeof(char));
	while(record != NULL) // loop through the records and pull out the current key
	{
		char* underscore;
		underscore = strchr(record, '_');
		
		record_key = realloc( record_key, sizeof(char)*(underscore-record+1));
		strncpy(record_key, record,underscore-record );
		record_key[underscore-record] = '\0';
		
		if (strcmp(key,record_key) == 0) // do the keys match?
		{
			foundFlag = 1;
			sscanf(underscore, "_%d", &blockID);

			readData(blockID, key, treeFlag); // read the record from the database.txt file
			readCounter++;
			break;
		}
		
		record = strtok(NULL, "|");
	}
	
	
	if(foundFlag == 0)
		printf("\nERROR: Key does not exist!\n");
	
	printf("\nNumber of Disk Reads: %d\n", readCounter);

	free(record_key);
	free(buffer);
}


// remakes the tree by traversing through the Nodes -> going leftmost first || pulls out the Node information at the given block number and stores it accordingly and returns it
Node* readNode(int blockNum, int fileName)
{
	// create the buffer
	char* buffer;
	buffer = malloc(sizeof(char)*BLOCKSIZE);
	memset(buffer, 0, BLOCKSIZE);
	
	//-------//
	
	// seek to and read the head of the tree
	if(lseek(fileName,(blockNum-1)*BLOCKSIZE,SEEK_SET) < 0) 
		printf("---PROBLEMS!!!! readNode---\n");
	read(fileName,buffer,BLOCKSIZE);
	
	//-------//
	
	Node* temp;
	temp = getNode();
	
	if(buffer[0] == 'I') // set up internal node
	{	
		// break up the node
		char* keyPtr;
		char* indexPtr;
		keyPtr = strtok(buffer, "|"); // eat the header
		keyPtr = strtok(NULL, "|"); // will now point to the string of keys
		indexPtr = strtok(NULL, "|"); // will now point to the string of ptrs
		
		char* indexPtrs; // copy pointers over to new string
		indexPtrs = malloc(sizeof(char)*(strlen(indexPtr)+1));
		strcpy(indexPtrs, indexPtr);
		indexPtrs[strlen(indexPtr)] = '\0';
		
		// loop through keys first
		char* keys = strtok(keyPtr, "_");
		char* keyInfo;
		int keyL_max;
		
		int key_count = 0;
		temp->numEntry = 0;
		while(keys != NULL) // break down the keys in the block and store them in Entries
		{
			keyL_max = strlen(keys);
			keyInfo = malloc(sizeof(char)*(strlen(keys)+1));
			strcpy(keyInfo, keys);
			keyInfo[strlen(keys)] = '\0';

			// grab the key and block number out
			int bNum;
			char grabKey1[keyL_max];
			char grabKey2[keyL_max];
			int i = 0;
			for( i = 0; i < keyL_max; i++)
			{
				grabKey1[i] = '\0';
				grabKey2[i] = '\0';
			}
			
			sscanf(keyInfo, "%s %s %d", grabKey1, grabKey2, &bNum);
			grabKey1[strlen(grabKey1)] = ' ';
			strcat(grabKey1, grabKey2);
			
			// make new entry
			Entry *newEntry = (Entry*)malloc(sizeof(Entry));
			newEntry->key = malloc(sizeof(char)*(strlen(grabKey1)+1));
			strcpy(newEntry->key,grabKey1);
			newEntry->key[strlen(grabKey1)] = '\0';
			newEntry->blockID = bNum;
			
			// set it to the corresponding spot in the Node
			temp->entry[key_count] = newEntry;

			key_count++;
			keys = strtok(NULL, "_");
			
			free(keyInfo);
		}
		temp->numEntry = key_count; // total number of keys in the Node
		
		
		int j = 0;
		int i = 0;
		int prevJ = 0;
		for(i = 0; i <= key_count; i++) // set up the pointers by traversing to the left most first
		{
			prevJ = j;
		    while(indexPtr[j] != '_') // once out of the while loop prevJ will point to the beginning of the ptr and j with point to the end
			{
				j ++;
				
				if(j >= strlen(indexPtrs))
					break;
			}
			
			// get out next block number to next Node
			char* num;
			num = malloc(sizeof(char)*((j - prevJ) +1));
			strncpy(num, indexPtrs+ prevJ, (j - prevJ));
			num[(j - prevJ)] = '\0';

			j++;
			
			temp->ptr[i] = readNode(atoi(num), fileName); // get next Node down
		
			free(num);
		}
		
		temp->leaf = 0;
		return temp;
	}
	else if( buffer[0] == 'L') // set up leaf Node
	{		
		char* record;
		record = strtok(buffer, "|"); // eat the header
		record = strtok(NULL, "|"); // now points to first entry
		
		int keyCounter = 0;
		temp->numEntry = 0;
		while(record != NULL) // loop through the records
		{
			int blockID;
			char* underscore;
			underscore = strchr(record, '_');
			
			// make new entry
			Entry *newEntry = (Entry*)malloc(sizeof(Entry));
			newEntry->key = malloc(sizeof(char)*(underscore-record+1));
			strncpy(newEntry->key, record,underscore-record);
			newEntry->key[underscore-record] = '\0';
			
			// get and set blockID
			sscanf(underscore, "_%d", &blockID);
			newEntry->blockID = blockID;

			temp->entry[keyCounter] = newEntry;

			keyCounter++;
			record = strtok(NULL, "|");
		}
		
		temp->leaf = 1;
		temp->numEntry = keyCounter;
		
		return temp;
	}
	
	free(buffer);
	
	return temp;
}


// Starts off the remaking of the tree by calling on the root node first
Node* remakeTree(int treeFlag)
{
	// open file
	size_t fileName;
	
	if(treeFlag == 0) // primary
		fileName = open("txtFiles/PrimaryIndex.txt", O_RDONLY);
	if(treeFlag == 1)
		fileName = open("txtFiles/SecondaryIndex.txt", O_RDONLY);
	
	if(fileName < 0)
		printf("---PROBLEMS---\n");
	
	//------//
		
	// gets the stats of the file
	struct stat fileStat;
    if(fstat(fileName,&fileStat) < 0)    
        return NULL;
	Node *root;
	pRoot = 0;
	root = readNode(((fileStat.st_size - 512)/512)+1, fileName);
	pRoot = root;
	root->leaf = 0;
	root->root = 1;
	

	close(fileName);
	
	return root;
}


// check current overflow block for space; insert record if space avaliable
int* checkE_OFB(int blockNum, char* record, int fileName)
{
	int* info = malloc(sizeof(int)*2);
	
	// create the buffer
	char* buffer;
	buffer = malloc(sizeof(char)*BLOCKSIZE);
	memset(buffer, 0, BLOCKSIZE);

	// seek to and read the block from the database
	if(lseek(fileName,blockNum*BLOCKSIZE,SEEK_SET) < 0) 
		printf("---PROBLEMS---\n");
	read(fileName,buffer,BLOCKSIZE);
	
	if(BLOCKSIZE - strlen(buffer) > strlen(record)+1) //  have room in overflow block
	{
		info[0] = blockNum;
		info[1] = strlen(buffer);
		
		memcpy(buffer+strlen(buffer), record, strlen(record)); // add record in
		
		if(lseek(fileName,blockNum*BLOCKSIZE,SEEK_SET) < 0) 
			printf("---PROBLEMS---\n");
		write(fileName,buffer,BLOCKSIZE);
		
		return info;
	}
	
	
	info[0] = -1;
	info[1] = -1;
	free(buffer);
	return info;
}


// make a new overflow block and insert record
int* makeOFB(char* record, int fileName)
{
	int* info = malloc(sizeof(int)*2);
	
	// create the buffer
	char* buffer;
	buffer = malloc(sizeof(char)*BLOCKSIZE);
	memset(buffer, 0, BLOCKSIZE);
	
	memcpy(buffer, record, strlen(record));
	
	// gets the stats of the file
	struct stat fileStat;
    fstat(fileName,&fileStat);

	info[0] = ((int)fileStat.st_size / BLOCKSIZE); // calc current block number
	info[1] = 0; // offset to start of number info
	
	
	if(lseek(fileName,0,SEEK_END) < 0) 
		printf("---PROBLEMS---\n");
	// add to end
	write(fileName,buffer,BLOCKSIZE);
	
	free(buffer);
	return info;
}


// checks to see if new record can go in block -> makes/fills overflow blocks accordingly
int insertRecord(int blockID, char* entryKey, char* prevKey)
{
	int fileName = open("txtFiles/database.txt", O_RDWR);
	if(fileName < 0)
		printf("---PROBLEMS---\n");
	
	//--------//
	
	// create the buffer
	char* buffer;
	buffer = malloc(sizeof(char)*BLOCKSIZE);
	memset(buffer, 0, BLOCKSIZE);
	
	// seek to and read the block from the database
	if(lseek(fileName,blockID*BLOCKSIZE,SEEK_SET) < 0) 
		printf("---PROBLEMS---\n");
	read(fileName,buffer,BLOCKSIZE);
	
	//---------//
	
	// make copy of buffer for changing
	char* bufferCopy;
	bufferCopy = malloc(sizeof(char)*BLOCKSIZE);
	memcpy(bufferCopy, buffer, BLOCKSIZE);

	char* recordsAll;
	recordsAll = strchr(buffer, '^');
	
	char* header;
	header = malloc(sizeof(char)*(strlen(buffer) - strlen(recordsAll) +1));
	memcpy(header, buffer, strlen(buffer) - strlen(recordsAll));
	header[ strlen(buffer) - strlen(recordsAll)] = '\0';
	
	// jump pass the ^
	recordsAll = recordsAll+1; // points to beginning of records in buffercopy
	
	// break down key into fields
	char* field1_end;
	char* field2_end;
	char* space = strchr(entryKey, ' ');
	int i = 0;
	for( i = 0; i < 3; i++) 
	{
		if(space != NULL)
		{
			if(space[1] == ' ') // advance pointer one char to get rid of double spaces
				space = strchr(space+1, ' ');
			space = strchr(space+1, ' '); 
		}
		
		if(i == 0)
			field1_end = space;
		if(i == 2)
			field2_end = space;
	}
	
	// put | inbetween fields
	field1_end[0] = '|';
	int newRecord_Length = strlen(entryKey);
	int newHeader_Length = numDigits(newRecord_Length);
	if(field2_end  == NULL)
	{
		int oldLen = strlen(entryKey);
		entryKey[oldLen] = '|';
		newRecord_Length = newRecord_Length +1;
	}
	else if(field2_end[0] == ' ')
		field2_end[0] = '|';
	
	
	// find previous record info
	int* recInfo;
	if(prevKey[0] == '\0') // new record goes at the beginning of the block
	{
		recInfo = malloc(sizeof(int)*6);
		recInfo[0] = 0;
		recInfo[1] = 0;
		recInfo[2] = 0;
		recInfo[3] = 0;
		recInfo[4] = 0;
		recInfo[5] = 0;
	}
	else // get info of record that comes before the new record
		recInfo = getRecordInfo(prevKey, buffer, 0, fileName);
	
	char* moveString;
	if(BLOCKSIZE - strlen(bufferCopy) > strlen(entryKey)+10) // inserting into existing block -> +10 to deal with header update
	{		
		if(recInfo[4]>= 0) // overflow record comes before
			moveString = bufferCopy+strlen(header)+1+recInfo[5];
		else
			moveString = bufferCopy+strlen(header)+1+recInfo[0]+recInfo[1];
		
		// shift next record down by new record length, add new record in
		memmove(moveString+newRecord_Length, moveString, strlen(moveString)); //shifts
		memcpy(moveString, entryKey, newRecord_Length); // adds
	
		moveString = bufferCopy+recInfo[2]+recInfo[3]; // now points to where new header info should go
				
		if(recInfo[2] != 0)
			moveString = moveString+1; // jump past '|'
		
		// make new header, shift down next header by header length
		char* recLeng_String;
		recLeng_String = malloc(sizeof(char)*newHeader_Length+2);
		
		// deals with positioning
		if(recInfo[3] == 0) // goes at front
			sprintf(recLeng_String, "%d|", newRecord_Length);
		else if(recInfo[3] > 0)
			sprintf(recLeng_String, "|%d", newRecord_Length);
		
		// shift down header, insert new header info
		memmove(moveString+newHeader_Length+1, moveString, strlen(moveString));
		memcpy(moveString, recLeng_String , newHeader_Length+1);
		
		// seek to and write the block from the database
		if(lseek(fileName,blockID*BLOCKSIZE,SEEK_SET) < 0) 
			printf("---PROBLEMS---\n");
		write(fileName,bufferCopy,BLOCKSIZE);
		
		free(recLeng_String);
	}
	else if(BLOCKSIZE - strlen(bufferCopy) > 12) // room for header update - but not enought room for record
	{
		int* checkOFB_info = NULL;
		
		if(header[0] == 'O') // check for beginning OFB
		{
			// current OFB - parse info
			checkOFB_info = checkE_OFB(0, entryKey, fileName);
			
			if(checkOFB_info[0] != -1) // there was space for record
			{
				moveString = bufferCopy+recInfo[2]+recInfo[3]; // update header info
				
				if(recInfo[2] != 0)
					moveString = moveString+1; // jump past '|'
				
				// make new header, shift down next header by header length
				int newHeaderLeng = numDigits(checkOFB_info[0])+numDigits(checkOFB_info[1])+numDigits(strlen(entryKey))+1+3;
				char* recLeng_String;
				recLeng_String = malloc(sizeof(char)*(newHeaderLeng+1));
				
				// deals with positioning
				if(recInfo[3] == 0) // goes at front
					sprintf(recLeng_String, "O_%d_%d_%d|", checkOFB_info[0], checkOFB_info[1], (int)strlen(entryKey));
				else if(recInfo[3] > 0)
					sprintf(recLeng_String, "|O_%d_%d_%d", checkOFB_info[0], checkOFB_info[1], (int)strlen(entryKey));
				
				// shift down header, insert new header info
				memmove(moveString+newHeaderLeng+1, moveString, strlen(moveString));
				memcpy(moveString, recLeng_String , newHeaderLeng+1);
				
				// write to memory
				if(lseek(fileName,blockID*BLOCKSIZE,SEEK_SET) < 0) 
					printf("---PROBLEMS---\n");
				write(fileName,bufferCopy,BLOCKSIZE);
				
				free(recLeng_String);
			}
		}
		else // check the rest of the header info for exsiting OFB
		{
			char* recLoop;
			recLoop = strchr(header, '|');
			while(recLoop != NULL)
			{
				if(recLoop[1] == 'O')
				{
					// check current OFB for space
					int block_num = 0;
					sscanf(recLoop, "|O_%d_%*s", &block_num);
					checkOFB_info = checkE_OFB(block_num, entryKey, fileName);
					
					if(checkOFB_info[0] != -1) // there was space for record
					{
						moveString = bufferCopy+recInfo[2]+recInfo[3]; // up date header info
						
						if(recInfo[2] != 0)
							moveString = moveString+1; // jump past '|'
						
						// make new header, shift down next header by header length
						int newHeaderLeng = numDigits(checkOFB_info[0])+numDigits(checkOFB_info[1])+numDigits(strlen(entryKey))+1+3;
						char* recLeng_String;
						recLeng_String = malloc(sizeof(char)*(newHeaderLeng+1));
						
						// deals with positioning
						if(recInfo[3] == 0) // goes at front
							sprintf(recLeng_String, "O_%d_%d_%d|", checkOFB_info[0], checkOFB_info[1], (int)strlen(entryKey));
						else if(recInfo[3] > 0)
							sprintf(recLeng_String, "|O_%d_%d_%d", checkOFB_info[0], checkOFB_info[1], (int)strlen(entryKey));
												
						// shift down header, insert new header info
						memmove(moveString+newHeaderLeng+1, moveString, strlen(moveString));
						memcpy(moveString, recLeng_String , newHeaderLeng+1);
												
						// write to memory
						if(lseek(fileName,blockID*BLOCKSIZE,SEEK_SET) < 0) 
							printf("---PROBLEMS---\n");
						write(fileName,bufferCopy,BLOCKSIZE);
						
						free(recLeng_String);
						break;
					}
				}
				recLoop = strchr(recLoop+1, '|');
			}
		}
		
		// no prevous block found/had space - add a new one
		if(checkOFB_info == NULL || checkOFB_info[0] == -1)
		{
			checkOFB_info = makeOFB(entryKey, fileName); // make a new block

			moveString = bufferCopy+recInfo[2]+recInfo[3]; // up date header info
			
			if(recInfo[2] != 0)
				moveString = moveString+1; // jump past '|'
			
			// make new header, shift down next header by header length
			int newHeaderLeng = numDigits(checkOFB_info[0])+numDigits(checkOFB_info[1])+numDigits(strlen(entryKey))+1+3;
			char* recLeng_String;
			recLeng_String = malloc(sizeof(char)*(newHeaderLeng+1));
			
			// deals with positioning
			if(recInfo[3] == 0) // goes at front
				sprintf(recLeng_String, "O_%d_%d_%d|", checkOFB_info[0], checkOFB_info[1], (int)strlen(entryKey));
			else if(recInfo[3] > 0)
				sprintf(recLeng_String, "|O_%d_%d_%d", checkOFB_info[0], checkOFB_info[1], (int)strlen(entryKey));
						
			// shift down header, insert new header info
			memmove(moveString+newHeaderLeng+1, moveString, strlen(moveString));
			memcpy(moveString, recLeng_String , newHeaderLeng+1);
						
			// write to memory
			if(lseek(fileName,blockID*BLOCKSIZE,SEEK_SET) < 0) 
				printf("---PROBLEMS---\n");
			write(fileName,bufferCopy,BLOCKSIZE);
			
			free(recLeng_String);
		}
	}
	else
	{
		printf("\nERROR: Can't insert - not enough room to update header\n");
		close(fileName);
		return 0;
	}
	
	free(buffer);
	free(bufferCopy);
	free(header);
	free(recInfo);
	close(fileName);
	return 1;
}


void displayDatabase()
{
	size_t fileName = open("txtFiles/database.txt", O_RDONLY);
	if(fileName < 0)
		printf("---PROBLEMS---\n");
	//--------//
	
	// create the buffer
	char* buffer;
	buffer = malloc(sizeof(char)*BLOCKSIZE);
	memset(buffer, 0, BLOCKSIZE);
	
	// create the buffer for an Overflow block
	char* bufferOF;
	bufferOF = malloc(sizeof(char)*BLOCKSIZE);
	memset(bufferOF, 0, BLOCKSIZE);

	int currentBlock = 0;
	while(read(fileName,buffer,BLOCKSIZE) > 0) // read block by block
	{
		if(buffer[0] != 0)
		{
			
			char* recordsAll;
			recordsAll = strchr(buffer, '^');
			
			if(recordsAll == NULL) // in overflow block (reached end of headers)
				break;
			
			char* header;
			header = malloc(sizeof(char)*(strlen(buffer) - strlen(recordsAll) +1));
			memcpy(header, buffer, strlen(buffer) - strlen(recordsAll));
			header[ strlen(buffer) - strlen(recordsAll)] = '\0';
			
			// jump pass the ^
			recordsAll = recordsAll+1;
			
			int recInfo[5];
			char* recordInfo = header;
			char* currentRecord = malloc(sizeof(char));
			int bufferOffset = 0;
			int headerOffset = 0;
			int record_length = 0;
			
			
			
			// loop through the header and print out the corresponding record
			do{
				if(recordInfo[0] == 'O')// deal will getting overflow block information
				{
					sscanf(recordInfo, "O_%d_%d_%d%*s", &recInfo[4], &recInfo[0], &recInfo[1] );
					headerOffset = numDigits(recInfo[4])+numDigits(recInfo[0])+numDigits(recInfo[1])+4;
					
					// seek to and read the block from the database
					if(lseek(fileName,recInfo[4]*BLOCKSIZE,SEEK_SET) < 0) 
						printf("---PROBLEMS---\n");
					read(fileName,bufferOF,BLOCKSIZE);
					
					currentRecord = realloc(currentRecord, sizeof(char)*(recInfo[1]+1));
					memcpy(currentRecord, bufferOF+recInfo[0], recInfo[1]);
					currentRecord[recInfo[1]] = '\0';
					
					// resets pointer
					if(lseek(fileName, (currentBlock+1)*BLOCKSIZE,SEEK_SET) < 0) 
						printf("---PROBLEMS---\n");
				}
				else
				{
					sscanf(recordInfo, "%d%*s", &record_length);
					headerOffset = numDigits(record_length);
					
					currentRecord = realloc(currentRecord, sizeof(char)*(record_length+1));
					memcpy(currentRecord, recordsAll+bufferOffset, record_length);
					currentRecord[record_length] = '\0';
					
					bufferOffset = bufferOffset + record_length;
				}
				
				printf("Record: %s\n", currentRecord);
				
				recordInfo = strchr(recordInfo, '|');
				
				if(recordInfo != NULL)
				{
					headerOffset = strlen(header) - strlen(recordInfo);
					recordInfo = recordInfo+1; // jump past '|'
				}
				
			}while(recordInfo != NULL);
			
			free(header);
			
		}
		
		currentBlock ++;
	}
	
	free(buffer);
	free(bufferOF);
	close(fileName);
}


// deals with the user menu, and updating the index files for insertion and deletion
int main()
{
	Node *root,*childNode;	
	Entry *entry, *childEntry;
		
	char inkey[200];

	int stop=0;
	Node *ptrParent;
	Entry *oldChildEntry;


	do
	{
		printf("\nSelect an operation:\n1-Search on primary key\n2-Search on secondary key\n3-Insert new record\n4-Delete record\n5-Display records sequentially\nq-Quit\n\n");

		// print up enter char
		char throw[5];
		scanf("%s", inkey);
		fgets(throw,5, stdin);

		switch(inkey[0])
		{
		   case '1': // search on primary
			{
				fputs("Enter the Primary Key to retrieve: ", stdout);
				fflush(stdout);
				
				fgets(inkey, 200, stdin);
				char * newline = strchr(inkey, '\n');
				if(newline)
					newline[0] = '\0';
				
				searchIndex(0, inkey);
				
				break;	
			}
			case '2': // search on secondary
			{
				fputs("Enter the Secondary Key to retrieve: ", stdout);
				fflush(stdout);
				
				fgets(inkey, 200, stdin);
				char * newline = strchr(inkey, '\n');
				if(newline)
					newline[0] = '\0';
				
				searchIndex(1, inkey);
				
				break;	
			}
			case '3': // insert new record
			{
				root = remakeTree(0); // makes primary tree
				
				fputs("Enter the full record to insert: ", stdout);
				fflush(stdout);
				
				fgets(inkey, 200, stdin);
				char * newline = strchr(inkey, '\n');
				if(newline)
					newline[0] = '\0';
				
				
				// Check for format problems
				if(inkey[0] == ' ')
				{
					printf("ERROR: Record can not start with a space\n");
					break;
				}
				else if(inkey[strlen(inkey)-1] == ' ')
				{
					printf("ERROR: Record can not end with a space\n");
					break;
				}
				
				// Check for 3 word minimum and for double spaces
				char* wordCount = strchr(inkey, ' ');
				if(wordCount == NULL)
				{
					printf("ERROR: Record must have at least 3 words\n");
					break;
				}
				else if(wordCount[1] == ' ')
				{
					printf("ERROR: Record has double spaces\n");
					break;
				}
				
				wordCount = strchr(wordCount+1, ' ');
				if(wordCount == NULL)
				{
					printf("ERROR: Record must have at least 3 words\n");
					break;
				}
				else if(wordCount[1] == ' ')
				{
					printf("ERROR: Record has double spaces\n");
					break;
				}
				
				
				// grabs the primary key out of the record to insert
				int keyL_max = strlen(inkey);
				char grabKey1[keyL_max];
				char grabKey2[keyL_max];
				int i = 0;
				for( i = 0; i < keyL_max; i++)
				{
					grabKey1[i] = '\0';
					grabKey2[i] = '\0';
				}
				
				sscanf(inkey, "%*s %*s %s %s %*s", grabKey1, grabKey2);

				// add in a space
				if(strlen(grabKey2) != 0)
				{
					grabKey1[strlen(grabKey1)] = ' ';
					strcat(grabKey1, grabKey2);
				}
				

				entry = (Entry*)malloc(sizeof(Entry)); 	// entry will hold the info of the record that comes right before the new record
				retrieve(root, grabKey1, entry); // will return key as '\0' if it goes at the beginning of the file
								
				if(insertRecord(entry->blockID, inkey, entry->key)) // was inserting succesful? YES- update the index files accordingly
				{
					// for primary tree insertion
					entry->key = malloc(sizeof(char)*(strlen(grabKey1)+1));
					strcpy(entry->key,grabKey1);
					entry->key[strlen(grabKey1)] = '\0';
					
					childNode = (Node*)malloc(sizeof(Node));
					childEntry = (Entry*)malloc(sizeof(Entry));
					
					insert(&root,entry,&childNode,childEntry);
					
					// write to file
					remove("txtFiles/PrimaryIndex.txt");
					treeToFile(root, 1, 0);
					free(root);
					
					// for secondary tree insertion
					// grab the secondary key
					for( i = 0; i < keyL_max; i++)
					{
						grabKey1[i] = '\0';
						grabKey2[i] = '\0';
					}
					char* div = strchr(inkey, '|');
					div[0] = ' ';
					sscanf(inkey, "%s %s %*s", grabKey1, grabKey2);
					grabKey1[strlen(grabKey1)] = ' ';
					strcat(grabKey1, grabKey2);
					
					root = remakeTree(1);
					
					free(entry->key);
					entry->key = malloc(sizeof(char)*(strlen(grabKey1)+1));
					strcpy(entry->key,grabKey1);
					entry->key[strlen(grabKey1)] = '\0';
					
					childNode = (Node*)malloc(sizeof(Node));
					childEntry = (Entry*)malloc(sizeof(Entry));
					
					insert(&root,entry,&childNode,childEntry);	
					
					// write to file
					remove("txtFiles/SecondaryIndex.txt");
					treeToFile(root, 1, 1);
				}
				
				free(root);
				break;	
			}
			case '4': //Delete record
			{
				entry = (Entry*)malloc(sizeof(Entry));
				root = remakeTree(0); // remake the primary tree

				fputs("Enter the key to delete: ", stdout);
				fflush(stdout);
				
				fgets(inkey, 200, stdin);
				char * newline = strchr(inkey, '\n');
				if(newline)
					newline[0] = '\0';
				
				printf("\n");
				
				Entry* entry2 = (Entry*)malloc(sizeof(Entry));
				if (retrieve(root,inkey,entry2)) // check to see if key is valid
				{
					char* secondaryKey = deleteRecord(entry2->blockID, inkey); // returns secondary key to update the second index tree
					
					// for primary tree deletion
					entry->key = malloc(sizeof(char)*(strlen(inkey)+1));
					strcpy(entry->key,inkey);
					entry->key[strlen(inkey)] = '\0';
					ptrParent = NULL;
					oldChildEntry = (Entry*)malloc(sizeof(Entry));
					
					deletion(ptrParent,&root,entry,&oldChildEntry);
					
					// write to file
					remove("txtFiles/PrimaryIndex.txt");
					treeToFile(root, 1, 0);
					free(root);
					
					// for secondary tree deletion
					root = remakeTree(1);
					
					free(entry->key);
					entry->key = malloc(sizeof(char)*(strlen(secondaryKey)+1));
					strcpy(entry->key,secondaryKey);
					entry->key[strlen(secondaryKey)] = '\0';
					ptrParent = NULL;
					oldChildEntry = (Entry*)malloc(sizeof(Entry));
					
					deletion(ptrParent,&root,entry,&oldChildEntry);

					// write to file
					remove("txtFiles/SecondaryIndex.txt");
					treeToFile(root, 1, 1);
					free(root);
				}
				else
					printf("ERROR: key '%s' does not exist\n", inkey);
				
				break;	
			}
			case '5': // Dispaly records sequentially
			{
				displayDatabase();
				break;	
			}
			case 'q': {
				stop=1;
				break;
			}
			default: break;
		}

	} while (stop==0);


	return 1;
}


