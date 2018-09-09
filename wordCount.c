#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>

typedef struct InfoBlock{
	char key[255];
	int value;
	int offset;
} info;

void initInfoArray(info* list, int size){
	int i = 0;
	while(i < size){
		list[i].value = -1;
		sprintf(list[i].key, "%s", "*");
		list[i].offset = 0;
		i++;
	}	
}

int off(int rank, int mnf, int nf){
	if(rank == 0)
		return 0;
	else
		return (off(rank-1,mnf,nf) + ((rank-1 < mnf)? nf+1 :nf));	
}

char **alloc_contigous_2D_array(int r, int c){
	char *mem = (char *)malloc(r*c*sizeof(char));
	char **a  = (char **)malloc(r*sizeof(char*));
	int i;
	for(i=0; i<r ; i++)
		a[i] = &(mem[c*i]);
	
	return a;
}

int main (int argc, char* argv[])
{
	
	int rank, np;
	double tstart, tfinish;
	
	MPI_Status status;
  MPI_Init (&argc, &argv);      /* starts MPI */
  MPI_Comm_rank (MPI_COMM_WORLD, &rank);        /* get current process id */
  MPI_Comm_size (MPI_COMM_WORLD, &np);        /* get number of processes */
	
	/** create mpi struct **/	
	const int nitems=3;
  
  MPI_Datatype infoCalc;
	MPI_Datatype types[3] = {MPI_CHAR, MPI_INT, MPI_INT};
	int blocklengths[3] = {255,1,1};
  MPI_Aint offsets[3];

  offsets[0] = offsetof(info, key);
  offsets[1] = offsetof(info, value);
	offsets[2] = offsetof(info, offset);

  MPI_Type_create_struct(nitems, blocklengths, offsets, types, &infoCalc);
  MPI_Type_commit(&infoCalc);

	/**************************/	
	if(rank == 0)
		tstart = MPI_Wtime();
		
	
	char *mex;
	mex = malloc(sizeof(char)*255);
	
	int numFiles, scan, i, count;
	char **files;
	FILE *index;
	
	if(rank == 0){
		index = fopen(argv[1], "r");
		scan = fscanf(index, "%d",&numFiles);
	}
	
	MPI_Bcast(&numFiles, sizeof(MPI_INT), MPI_INT, 0, MPI_COMM_WORLD);
	
	int mnf = numFiles % np;
	int nf = numFiles / np;
	
	files = alloc_contigous_2D_array(numFiles, 255);
	//files = malloc(sizeof(char*)*numFiles);
	//char files[numFiles][255];
	
	if(rank == 0){
	
	for(i = 0; i < numFiles; i++)
			scan = fscanf(index, "%s\n", files[i]);	
	fclose(index);
	
	}
	
	MPI_Bcast(&(files[0][0]), numFiles*255, MPI_CHAR, 0, MPI_COMM_WORLD);
	
	int mynf = (rank<mnf)?nf+1:nf;
	int uppBSize = (mnf > 0)?nf+1:nf;
	
	//info list[uppBSize];
	info *list = (info*)malloc(sizeof(info)*uppBSize);
																
	//info totalList[uppBSize * np];
	
	info *totalList = (info*)malloc(sizeof(info)*(uppBSize * np));
																	
	initInfoArray(list, uppBSize);
	//initInfoArray(totalList, uppBSize*np);
	
	int offFile = off(rank, mnf, nf);

	FILE *fileInExam;

	for(i=0; i<mynf; i++){
		fileInExam = fopen(files[i+offFile], "r");
		
		sprintf(list[i].key,"%s",files[i+offFile]);
		list[i].offset = 0;
		list[i].value = 0;
		
		while(!feof(fileInExam))
			list[i].value += fscanf(fileInExam,"%s\n",mex);
		
		fclose(fileInExam);
	}

	MPI_Allgather(list, uppBSize, infoCalc, totalList, uppBSize, infoCalc, MPI_COMM_WORLD);
	
	count = 0;
	for(i = 0; i < (uppBSize * np); i++){
		
		if(totalList[i].value != -1)
			count += totalList[i].value;
		
		//printf("[%d] k:%s v:%d o:%d\n",rank,totalList[i].key,totalList[i].value,totalList[i].offset);
	}
	
	printf("count: %d\n", count);
	
	int offset;
	int mwfp = count % np;
	int wfp = count / np;
	int mywfp = (rank < mwfp)?wfp+1:wfp;
	int oldUppBSize = uppBSize;
	uppBSize = (mwfp > 0)?wfp+1:wfp;
	info myPart;

	if(rank == 0){
		
		int fi = 0;
		int offset = 0;
		
		for(i=np-1; i>=0; i--){
			info toSend;
			sprintf(toSend.key,"%s",totalList[fi].key);
			toSend.value = (i<mwfp)?wfp+1:wfp; 
			toSend.offset = offset;
			
			printf("part of %d : key: %s value: %d offset : %d\n", i, toSend.key, toSend.value, toSend.offset);
			
			
			int c = toSend.value;
		
			
			while(fi < (oldUppBSize*np)){
				
				
				
				if(totalList[fi].value != -1){
					if(c >= (totalList[fi].value - offset)){
						c -= (totalList[fi].value - offset);
						offset = 0;
						fi++;
					}
					else{
						offset = c + offset;
						break;
					}
					
				}
				else{
					fi++;
				}
				
				
			}
		
			if(i!=0)
				MPI_Send(&toSend, 1, infoCalc, i, 1, MPI_COMM_WORLD);	
			else{
				myPart = toSend;
			}
			
		}
		
	}
	else{
		MPI_Recv(&myPart, 1,  infoCalc, 0, 1, MPI_COMM_WORLD, &status);	
		
	}
	
	free(totalList);
	
	int initF=0;
	while(initF < numFiles && (strcmp(myPart.key, files[initF])!=0) )
		initF++;
	
	//info tab[uppBSize];
	info *tab = (info*)malloc(sizeof(info)*uppBSize);
	
	initInfoArray(tab, uppBSize);
	offset = myPart.offset;
	
	int tot = 0;
	
	while(tot < mywfp){
		fileInExam = fopen(files[initF], "r");
		int y = 0;
		for(y=0; y < offset; y++)
			scan = fscanf(fileInExam,"%*s\n");
		y=0;
		offset = 0;
		info el;
		
		while(y < (mywfp - tot) && !feof(fileInExam)){
			scan = fscanf(fileInExam,"%s\n",mex);
			sprintf(el.key,"%s",mex);
			
			int j=0;
			while(tab[j].value!=-1 && strcmp(mex,tab[j].key)!=0 && j<uppBSize)
				j++;
			if(tab[j].value==-1){
				el.value = 1;
				el.offset = 0;
				tab[j] = el;
			}
			else if(strcmp(mex,tab[j].key)==0)
				tab[j].value +=1;
				else if(j<myPart.value)
					printf("Inaxpeted Overflow");
				
				y++;
		}
		
		tot+=y;
		if(tot < mywfp && feof(fileInExam))
				initF++;
			
		fclose(fileInExam);
		
	}
	
	free(files);
	
	//info result[uppBSize * np];
	info* result = (info*)malloc(sizeof(info)*uppBSize*np);
	
	MPI_Gather(tab, uppBSize, infoCalc, result, uppBSize, infoCalc, 0, MPI_COMM_WORLD);
	free(tab);
	
	
	if(rank == 0){
		tfinish = MPI_Wtime();
	
		for(i=0 ;i<(uppBSize * np) ;i++){
			if(result[i].value < 0)
					continue;
				
				int y;
				for(y=i+1; y<(uppBSize * np); y++){
					if(result[y].value < 0)
						continue;
					if((strcmp(result[y].key, result[i].key))==0){
						result[i].value += result[y].value;
						result[y].value = -1;
					}
				}
				
				printf("[%d]__word:%s -  #:%d\n", rank,result[i].key,result[i].value);
				
			}
			
		printf("%f\n", tfinish - tstart);
	}
	
	free(mex);
	free(result);
	
	MPI_Finalize();
  return 0;
}