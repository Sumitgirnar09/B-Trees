prog1: prog1.c prog2.c
	gcc prog1.c -o prog1 -lm -Wall
	gcc prog2.c -o prog2 -lm -Wall

clean:
	rm -f prog1 prog2 txtFiles/database.txt txtFiles/PrimaryIndex.txt txtFiles/SecondaryIndex.txt
