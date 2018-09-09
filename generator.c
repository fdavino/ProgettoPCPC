#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main (int argc, char**argv){
	
	FILE *input, *index, *writed;
	char word[30];
	char namePrefix[30];
	char actual[30];
	unsigned long limit;
	int numFiles;
	int numWords;
	int i,j,randVar;
	int seed = atoi(argv[5]);
	
	srand(seed);
	
	if(argc < 6)
		printf("usage: generator.o <wordInputFile.txt> <prefix> <numFiles> <numWords> <seed>\n");
		//nome eseguibile, dizionario, prefisso nome file, numero di file da generare, numero massimo di parole per ognuno, seed.
	else{
		
		numFiles = atoi(argv[3]);
		
		input = fopen(argv[1], "r");
		sprintf(namePrefix,"%s",argv[2]);
		index = fopen("index.txt", "w");
	
		fseek(input, 0, SEEK_END);
		limit = ftell(input);
		fseek(input, 0, SEEK_SET);
		fprintf(index,"%d\n",numFiles);
		
		for(i=0; i<numFiles; i++){
			sprintf(actual,"%s-%d.txt",namePrefix,i);
			writed = fopen(actual, "w");
			fprintf(index,"%s\n",actual);

			numWords = atoi(argv[4]);
			randVar = rand() % 16;
			randVar = (numWords * randVar)/100;
			if(rand()%2 == 0)
				numWords += randVar;
			else
				numWords -= randVar;

			printf("randVar: %d numWords: %d\n", randVar,numWords);
			
			for(j=0; j<numWords; j++){
				int off = rand() % limit;
				fseek(input, off, SEEK_SET);
				fscanf(input, "%*s %s ",word);
				fprintf(writed, "%s\n", word);
			}
			
			fflush(writed);
			fclose(writed);
		}
		
		fclose(input);
		fflush(index);
		fclose(index);
	
	}
		
	return 0;
}
