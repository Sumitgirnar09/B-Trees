/* Laura Richards  
 * CIS*6030 - Assign#1
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include "btree.h"

// struct to hold the record
typedef struct {  
    char* field1;
	char* field2;
	char* field3;
	int blockNumber;

} Record;


// comparison for qsort
int string_Cmp(const void *a, const void *b)
{
	 Record *ai = (Record *)a;
	 Record *bi = (Record *)b;
	
	return strcmp(ai->field2, bi->field2);
}


// reads in file and stores it into an array of structs, which is then sorted by the 2nd field
int readFile(Record **recordArray)
{
	// open file
	int fileName = open("txtFiles/data.txt", O_RDONLY);
	
	if(fileName < 0)
		printf("PROBLEMS\n");
	
	// get the stat's of the file
	struct stat fileStat;
    if(fstat(fileName,&fileStat) < 0)    
        return 1;

	// read in entire file at once
	char fullFile[fileStat.st_size];
	
	if( read(fileName, fullFile, fileStat.st_size) < 0)
		write(2, "Error in reading file\n", 31);
	
	close(fileName);
	
	//----//
	
	(*recordArray) = malloc(sizeof(Record)*100);
	
	int total_Mem = 100;
	int lineNum = 0;
	
	char* prevFileLines;
	char* fileLines;
	fileLines = strtok(fullFile, "\n");
	
	while(fileLines != NULL) // loops through all the lines in the char array by tokenizing at "\n"
	{
		if(strcmp(fileLines,"ZZZ") == 0  ) // ending string
			break;
		
		lineNum++;
		prevFileLines = fileLines; // keep track of the previous line
		
		fileLines = strtok(NULL, "\n");
		
		if(lineNum > total_Mem) // need more space
		{
			(*recordArray) = realloc((*recordArray),sizeof(Record)*(total_Mem*2));
			total_Mem = total_Mem*2;
		}
		
		// pointers
		char* record_1_ptr;
		char* record_2_ptr;
		char* space;
		char* tab;
		int extraSpace = 0;
		
		// if there is a tab change it to a space
		tab = strchr(prevFileLines, '	');
		if(tab != NULL)
			tab[0] = ' ';
		
		
		space = strchr(prevFileLines, ' '); // pointer to the first space in the line
		
		int i = 0;
		for( i = 0; i < 3; i++) // split the line up into pointers of where the fields start and stop
		{
			if(space != NULL)
			{
				if(space[1] == ' ') // advance pointer one char to get rid of double spaces
				{
					extraSpace = 1; // added on to get rid of double spaces
					space = strchr(space+1, ' ');
				}
				
				space = strchr(space+1, ' '); // finds next space
			}
			
			if(i == 0)
				record_1_ptr = space;
			
			if(i == 2)
				record_2_ptr = space;
		}
		
		// copies words to respective fields and adds null terminator
		
		// secondary field
		(*recordArray)[lineNum-1].field1 = malloc(sizeof(char)*((record_1_ptr-prevFileLines)+1+1));
		strncpy((*recordArray)[lineNum-1].field1, prevFileLines, record_1_ptr-prevFileLines);
		(*recordArray)[lineNum-1].field1[record_1_ptr-prevFileLines] = '|';
		(*recordArray)[lineNum-1].field1[record_1_ptr-prevFileLines+1] = '\0';
		
		// primary field
		(*recordArray)[lineNum-1].field2 = malloc(sizeof(char)*((record_2_ptr-(record_1_ptr+extraSpace))+1+1));
		strncpy((*recordArray)[lineNum-1].field2, (record_1_ptr+extraSpace+1), record_2_ptr-(record_1_ptr+extraSpace));
		(*recordArray)[lineNum-1].field2[record_2_ptr-(record_1_ptr+extraSpace+1)] = '|';
		(*recordArray)[lineNum-1].field2[record_2_ptr-(record_1_ptr+extraSpace+1)+1] = '\0';
		
		// everything else
		if(space == NULL) 
		{
			(*recordArray)[lineNum-1].field3 = malloc(sizeof(char));
			(*recordArray)[lineNum-1].field3[0] = '\0';
		}
		else
		{
			(*recordArray)[lineNum-1].field3 = malloc(sizeof(char)*( fileStat.st_size - (space+1 - fullFile)));
			strncpy((*recordArray)[lineNum-1].field3, space+1, fileStat.st_size - (space+1 - fullFile));
			(*recordArray)[lineNum-1].field3[fileStat.st_size - (space+1 - fullFile)] = '\0';
		}
	}
	
	return lineNum;
}


// adds the record info onto the current header of each block - header just stores the length of the record in the block
void addRecord( int totalLength, char** headerString)
{
	int ret;
	char header_buff[100];
	
	ret = sprintf(header_buff, "%d|", totalLength);
	(*headerString) = realloc((*headerString), strlen((*headerString))+(sizeof(char)*(ret+1)));
	header_buff[ret+1] = '\0';
	strcat((*headerString), header_buff);
}

// builds the b+-trees by looping through the sorted records and creating a new node on the given field
void buildBtree( int flag,  int strings_len, Record *recordArray)
{
	Node *root,*childNode;
	Entry *entry, *childEntry;
	pRoot = 0;
	root = getNode();
	root->leaf = 1;
	root->root = 1;
	
	int i = 0;
	for( i = 0; i < strings_len; i++)
	{
		entry = (Entry*)malloc(sizeof(Entry));
		childNode = (Node*)malloc(sizeof(Node));
		childEntry = (Entry*)malloc(sizeof(Entry));
		
		int f1_length = strlen(recordArray[i].field1);
		int f2_length = strlen(recordArray[i].field2);
		
		if(flag == 0)// primary
		{
			entry->key = malloc(sizeof(char)*(f2_length));
			strcpy(entry->key, recordArray[i].field2);
			entry->key[f2_length-1] = '\0';
		}
		else if(flag == 1)// secondary
		{
			entry->key = malloc(sizeof(char)*(f1_length));
			strcpy(entry->key, recordArray[i].field1);
			entry->key[f1_length-1] = '\0';
		}
		
		entry->blockID = recordArray[i].blockNumber;
		
		insert(&root,entry,&childNode,childEntry);
	}
	
	treeToFile(root, 1, flag); // in btree.h, writes the tree to an index file
}


// packs records into buffer and then writes blocks to memory. Keeps track of records for b+-trees, and makes a header for each block.
void packBuffer(Record *recordArray, int strings_len)
{	
	// open file to write buffer to
	int fileName = open("txtFiles/database.txt", O_CREAT|O_WRONLY, S_IRWXU);
	if(fileName < 0)
		printf("------------------------PROBLEMS-------------------------------\n");

	int blockSize = 512;
	
	// starting header for each block
	char* headerString;
	headerString = malloc(sizeof(char));
	headerString[0] = '\0';
	
	// create the buffer
	char* buffer;
	buffer = malloc(sizeof(char)*blockSize);
	memset(buffer, 0, blockSize);
	buffer[blockSize] = '\0';
	
	// counters
	int blockNum = 0;
	int buff_tail = 0;
	int recordNum = 0;
	
	int i = 0;
	for( i = 0; i < strings_len; i++) // loops through each record in the array
	{
		int f1_length = strlen(recordArray[i].field1);
		int f2_length = strlen(recordArray[i].field2);
		int f3_length = strlen(recordArray[i].field3);
		recordArray[i].blockNumber = blockNum;
		
		// copy record into buffer
		memcpy(buffer+buff_tail, recordArray[i].field1, f1_length); 
		buff_tail += f1_length;
		
		memcpy(buffer+buff_tail, recordArray[i].field2, f2_length);
		buff_tail += f2_length;
		
		memcpy(buffer+buff_tail, recordArray[i].field3, f3_length);
		buff_tail += f3_length;
	
		// add record info to the header
		addRecord(f1_length+f2_length+f3_length, &headerString); // update header with record information
		
		recordNum++;
		
		// check to see how full the buffer is
		// if over 80% with header, write to memory, clear and start a new block
		double percentage = (double)(strlen(headerString)+buff_tail) / (double)blockSize;
		if(percentage >= 0.8 )
		{
			if(percentage >= 100) // went over the buffer
			{
				printf("\n\nBlock packing ERROR\n\n");
				break;
			}
			
			headerString[strlen(headerString)-1] = '^';
			
			//put header and buffer together
			memmove(buffer + strlen(headerString), buffer, buff_tail);
			memcpy(buffer, headerString, strlen(headerString));
			
			// write to memory
			write(fileName, buffer, blockSize);
			
			// reset buffer and counters
			memset(buffer, 0, blockSize); // clear the buffer
			buffer[blockSize] = '\0';
			buff_tail = 0;
			
			blockNum ++;
			free(headerString);
			recordNum = 0;
			headerString = malloc(sizeof(char));
			headerString[0] = '\0';
		}
		else if(i+1 == strings_len) // put together the very last block
		{
			headerString[strlen(headerString)-1] = '^';
			
			//put header and buffer together
			memmove(buffer + strlen(headerString), buffer, buff_tail);
			memcpy(buffer, headerString, strlen(headerString));
			
			// write to memory
			write(fileName, buffer, blockSize);
		}
	}
	
	close(fileName);
}


int main (int argc, const char * argv[]) {
	
	remove("txtFiles/database.txt");
	remove("txtFiles/PrimaryIndex.txt");
	remove("txtFiles/SecondaryIndex.txt");
	
	Record *recordArray;
	
	size_t strings_len = readFile(&recordArray);

	qsort(recordArray, strings_len, sizeof(Record), string_Cmp);
	
	int i = 0;
	for( i = 0; i < strings_len; i++)
		printf("%s %s %s\n", recordArray[i].field2, recordArray[i].field1, recordArray[i].field3);
	
	packBuffer(recordArray, strings_len); // pack buffer
	
	buildBtree(0, strings_len, recordArray); // makes b-trees, writes everything to memory
	buildBtree(1, strings_len, recordArray);
	
	printf("\nDatabase, Primary Index, and Secondary Index have been written to file\n");

	return 0;
}
