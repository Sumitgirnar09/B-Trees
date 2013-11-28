Laura Richards
Assignment # 1 - CIS 6030
*********************************

General Problem
----------------
1) Create a program that reads in a text file and creates a database file, and two index files using B+-trees.
2) Create a program that uses the index and database files to do searching, insertion and deletion on the database.

Assumptions & Limitations
--------------------------
- no '^' in a record
- records for insertion can not be longer than 200 characters
- deletion takes place with the primary key
- if there is no room in a block for a header update during insertion the program will not allow the insertion


Building Program
-----------------
In the assignment folder there is a makefile, to build all the programs at once type "make"

To run the first program type "./prog1"

To run the second program type "./prog2"

To clear all programs and .txt files type "make clean" 

Please run the first program before trying to run the second program.


Changes to btree.h
-------------------
- all integer division was changed to double division
- in insert(), if root node was split pRoot wasn't getting updated
- in redistributeRight(), updating of left sibling was off by 1 (was using index instead of index-1)
- in deletion(), if statements dealing with minLeafSpace/minInteriorSpace were changed from > to >=, since a key is allowed to have the min number
- in deletion(), pRoot was being set to a leaf when it shouldn't have been


Tested On (C = cleared)
----------

Search:
- primary and secondary keys - C
- records in overflow blocks found - C
- records that have been deleted not found - C
- records that have been inserted found - C

Insert:
- into normal block - C
- into very first position in tree - C
- after an overflow header - C
- into new overflow block - C
- into existing overflow block - C
- into new overflow block when existing one is full - C


Delete:
- normal primary record - C
- overflow primary record - C
- overflow block empty - C
- normal block empty - C

Display:
- normal and overflow blocks in sequence - C

