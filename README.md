# Word-Count

***
## Programmazione Concorrente, Parallela e su Cloud

### Università  degli Studi di Salerno

#### *Anno Accademico 2017/2018*

**Professore:** *Vittorio Scarano,*
 **Dottor:** *Carmine Spagnuolo,*
 **Studente:** *Ferdinando D'Avino*


---

## Problem Statement  

E' stato realizzato un programma che effettua il conteggio delle occorrenze di ogni parola presente all'interno di una serie di file.

## Soluzione proposta

Al nostro programma viene fornito in input un file che chiameremo index. Questo file conterrà sulla prima riga il numero di file che andranno analizzati e nelle seguenti il nome di ognuno, uno per riga.

**Esempio di file index**
```
6
f-0.txt
f-1.txt
f-2.txt
f-3.txt
f-4.txt
f-5.txt
```
Una volta analizzati i file l'output del nostro programma sarà formato dal conteggio totale delle parole (stampato da ogni processore), una serie di coppie "parola"-"occorrenza" ed il tempo di esecuzione per ottenere questo risultato.

**Esempio di output**
```
count: 62
[0]__word:area -  #:4
[0]__word:burn -  #:4
[0]__word:sea -  #:4
[0]__word:yes -  #:4
[0]__word:order -  #:4
[0]__word:phrase -  #:4
[0]__word:complete -  #:4
[0]__word:claim -  #:4
[0]__word:back -  #:4
[0]__word:wing -  #:4
[0]__word:speak -  #:4
[0]__word:place -  #:4
[0]__word:speech -  #:8
[0]__word:element -  #:4
[0]__word:chord -  #:2
0.005439
```
La soluzione prevede una fase di inizializzazione ed una fase di computazione entrambe eseguite in parallelo sui diversi processori.
Per la comunicazione sono state utilizzate tre funzioni collettive MPI: **MPI_BCast**, **MPI_Allgather** ed **MPI_Gather** .
Le misurazioni sono state effettuate su istanze **m4.large** fornite da Amazon AWS.

### Implementazione
L'obiettivo di questo programma è riuscire nel corretto conteggio delle occorrenze delle parole dei diversi file riuscendo a distribuire il carico di lavoro tra  diversi processori nel modo più equo possibile. Analizziamo il codice nel dettaglio

```
#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
```
```
typedef struct InfoBlock{
	char key[255];
	int value;
	int offset;
} info;
```
Struttura utilizzata per la comunicazione di informazioni tra i processori. Verrà utilizzata per diversi scopi che saranno illustrati successivamente
```
void initInfoArray(info* list, int size){
	int i = 0;
	while(i < size){
		list[i].value = -1;
		sprintf(list[i].key, "%s", "*");
		list[i].offset = 0;
		i++;
	}	
}
```
Funzione utilizzata per inizializzare un array di info (la struttura precedentemente dichiarata)
```
int off(int rank, int mnf, int nf){
	if(rank == 0)
		return 0;
	else
		return (off(rank-1,mnf,nf) + ((rank-1 < mnf)? nf+1 :nf));	
}
```
Una funzione che calcola in base al rank del processore, l'indice del file da analizzare


```
char **alloc_contigous_2D_array(int r, int c){
	char *mem = (char *)malloc(r*c*sizeof(char));
	char **a  = (char **)malloc(r*sizeof(char*));
	int i;
	for(i=0; i<r ; i++)
		a[i] = &(mem[c*i]);
	
	return a;
}
```
Una funzione utilizzata per dichiarare un array di puntatori a carattere in modo che ogni singolo array di caratteri sia contiguo agli altri in memoria


```
int main (int argc, char* argv[])
{
	
	int rank, np;
	double tstart, tfinish;
```


- **rank** *identificativo del processore*
- **np** *numero processori*
- **tstart** *tempo iniziale*
- **tfinish** *tempo finale*

```	
  MPI_Status status;
  MPI_Init (&argc, &argv);
  MPI_Comm_rank (MPI_COMM_WORLD, &rank);       
  MPI_Comm_size (MPI_COMM_WORLD, &np);
```
Inizializzazione ambiente MPI
```  
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

```
creazione struttura per MPI
```	
	if(rank == 0)
		tstart = MPI_Wtime();
```
memorizzazione tempo iniziale

Al fine di utilizzare le funzioni collettive (richiesto dalla traccia) e per avere una distribuzione del lavoro più equa possibile, le operazioni che seguono avranno il fine di contare il numero di parole di ogni file ed il numero totale di parole di tutti  i file.
```		
	char *mex;
	mex = malloc(sizeof(char)*255);	
	int numFiles, scan, i, count;
	char **files;
	FILE *index;
```
-**mex** *array di caratteri d'appoggio per le operazioni successive*
-**numFiles** *numero di file da analizzare*
-**scan** *variabile per memorizzare il valore di ritorno delle scanf*
-**count** *variabile per memorizzare il numero totale di parole di tutti i file*
-**files** *array per memorizzare i nomi dei diversi file*
-**index** *file index*
```
	if(rank == 0){
		index = fopen(argv[1], "r");
		scan = fscanf(index, "%d",&numFiles);
	}
	
	MPI_Bcast(&numFiles, sizeof(MPI_INT), MPI_INT, 0, MPI_COMM_WORLD);
	
	int mnf = numFiles % np;
	int nf = numFiles / np;
	
	char **files;
	files = alloc_contigous_2D_array(numFiles, 255);
																		
	if(rank == 0){
	
	for(i = 0; i < numFiles; i++)
			scan = fscanf(index, "%s\n", files[i]);	
	fclose(index);
	
	}
	MPI_Bcast(&(files[0][0]), numFiles*255, MPI_CHAR, 0, MPI_COMM_WORLD);
```
Il processore 0 apre il file index e informa tramite la successiva operazione di broadcast tutti i processori sul numero totale di file da analizzare.  Ogni processore calcola il proprio ammontare di lavoro, in più il processore 0 conclude la lettura del file index memorizzando nell'array **files** il nome di ogni file di input. Con la successiva operazione di broadcast l'array **files** viene fornito a tutti i processori. 
```
	int mynf = (rank<mnf)?nf+1:nf;
	int uppBSize = (mnf > 0)?nf+1:nf;
```
Proprio per tenere in considerazione la possibilità che il numero di files non sia perfettamente divisibile per il numero di processori calcoliamo *mynf*, ovvero il numero di files che il singolo processore dovrà analizzare.
*uppBSize* indicherà invece il numero di files che il processore con carico di lavoro maggiore andrà ad analizzare, per come abbiamo gestito la divisione del lavoro questo potrà essere al massimo superiore di 1 rispetto al lavoro degli altri processori ma è necessario memorizzarlo per uniformare la dimensione delle informazioni che saranno inviate. Infatti per utilizzare chiamate collettive MPI tutti gli elementi inviati e ricevuti dovranno avere la stessa taglia.
```
	info *list = (info*)malloc(sizeof(info)*uppBSize);
	info *totalList = (info*)malloc(sizeof(info)*(uppBSize * np));
																	
	initInfoArray(list, uppBSize);	
	int offFile = off(rank, mnf, nf);
```
Definiamo due array di *info*.
*list* sarà di taglia *uppBSize* in modo da essere certo che ogni processore opererà con un array della stessa taglia.
In questo caso la struttura *info* utilizzerà il valore *key* per memorizzare il nome del file, il valore *value* per memorizzare il numero di parole del dato file e non utilizzerà il campo *offset* che resterà a 0.
*totalList* sarà invece di taglia *np * uppBSize* in quanto sarà composto da tutti gli array *list* che riceverà dagli altri processori.
Viene inizializzato *list* in modo che ogni elemento abbia il campo *value* uguale a -1 che sta a rappresentare che il dato elemento è ancora non inizializzato.
Viene utilizzata la funzione *off* che in base a *rank*, *mnf* ed *nf* calcola un intero che rappresenterà un indice nell'array *files* e di conseguenza il file dal quale iniziare a contare. 
```
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
```
Ogni processore analizza i file che gli sono stati assegnati e per ognuno di essi memorizza il nome ed il numero di parole in esso contenuto.
```
	MPI_Allgather(list, uppBSize, infoCalc, totalList, uppBSize, infoCalc, MPI_COMM_WORLD);
	
	count = 0;
	for(i = 0; i < (uppBSize * np); i++){
		
		if(totalList[i].value != -1)
			count += totalList[i].value;
	}
	
	printf("count: %d\n", count);
```
Tramite la **MPI_Allgather** ogni processore condivide con gli altri il proprio array *list* in modo tale che tutti disporranno dopo questo punto di una lista di coppie **nome file**-**numero parole** che sarà contenuta in *totalList*.
Una volta composto basterà sommare il numero di parole di ogni file per ottenere il numero di parole totali da analizzare che verrà memorizzato in *count*.
```
	int offset;
	int mwfp = count % np;
	int wfp = count / np;
	int mywfp = (rank < mwfp)?wfp+1:wfp;
	int oldUppBSize = uppBSize;
	uppBSize = (mwfp > 0)?wfp+1:wfp;
	info myPart;
```
Con la dichiarazione di queste variabili inizia la seconda parte del codice che andrà ad effettuare l'effettivo conteggio delle occorrenze delle parole. Le suddette variabili sono analoghe a quelle utilizzate per effettuare un equa ripartizione del carico di lavoro per il calcolo del numero totale di parole.
- **offset** il numero di parole dal quale il processore dovrà partire, in un dato file, per non interfogliare il proprio lavoro con quello di altri
- **wfp** il numero di parole che ogni processore dovrà analizzare nel caso di divisione perfetta tra parole totali e numero di processori.
- **mwfp** il numero di parole di resto (nel caso in cui la divisione tra numero di parole totali e numero processori non sia perfetta) che andranno equamente distribuite tra i vari processori.
- **mywfp** il numero di parole che il singolo processore dovrà analizzare, questo valore è uguale ad *wfp* nel caso di divisione perfetta tra numero di parole totali e numero di processori, in caso contrario invece sarà integrato per considerare anche le parole di resto.
- **oldUppBSize** memorizza il vecchio valore di uppBSize.
- **uppBSize** questa variabile è stata illustrata precedentemente, mantiene lo stesso utilizzo ma il limite superiore non riguarda il numero di file per processore bensì il numero massimo di parole per processore.
- **myPart** una variabile del tipo della struttura *info*. In questo caso la struttura utilizzerà *key* per indicare il nome di un file (dal quale un dato processore dovrà iniziare l'analisi), *value* per indicare quante parole leggere da quel file ed *offset* per indicare da quale parola di quel file iniziare il conteggio. 

```
	if(rank == 0){
		
		int fi = 0;
		int offset = 0;
		
		for(i=np-1; i>=0; i--){
			info toSend;
			sprintf(toSend.key,"%s",totalList[fi].key);
			toSend.value = (i<mwfp)?wfp+1:wfp; 
			toSend.offset = offset;

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
```

A questo punto come indicato da **if(rank==0)** il master organizzerà il lavoro di tutti i processori. L'organizzazione consisterà nell' invio ad ogni processore di una diversa struttura *info* che come illustrato in precedenza comunicherà al processore ricevente da che file iniziare, da che parola di quest'ultimo e quante parole leggere.
Iniziando con un offset di 0 e dal primo file, analizziamo ogni file e se il numero di parole che dovrà leggere è inferiore alle parole restanti del file (parole restanti perchè un altro processore potrebbe aver già letto parte di esso) l'offset viene aggiornato per l'organizzazione del lavoro dei successivi processori. In caso contrario invece significa che il processore dovrà completare il file in esame ed iniziare a leggerne un altro, l'offset verrà quindi aggiornato di conseguenza.

Una volta inizializzata la struttura questa verrà inviata al processore slave in esame. Nel caso in cui il processore in esame sia proprio il master non ci sarà nessuno invio, la struttura verrà soltanto memorizzata da esso.
```
	else{
		MPI_Recv(&myPart, 1,  infoCalc, 0, 1, MPI_COMM_WORLD, &status);		
	}
	
	free(totalList);
```
I processori slave ricevono la propria variabile *info* per effettuare la propria computazione.
```	
	int initF=0;
	while(initF < numFiles && (strcmp(myPart.key, files[initF])!=0) )
		initF++;

	info *tab = (info*)malloc(sizeof(info)*uppBSize);
	
	initInfoArray(tab, uppBSize);
	offset = myPart.offset;
	
	int tot = 0;
```
Ogni processore inizializza il proprio contatore di parole e l'array di strutture *info* *tab*.
In questa parte la struttura *info* assumerà un significato diverso in quanto il campo *key* sarà la parola in esame, il campo *value* sarà l'occorrenza della stessa. In questo caso il campo offset non sarà utilizzato.
```	
	while(tot < mywfp){
		fileInExam = fopen(files[initF], "r");
		int y = 0;
		for(y=0; y < offset; y++)
			scan = fscanf(fileInExam,"%*s\n");

```
Posizioniamo il cursore di lettura prima della parola dalla quale iniziare a leggere, ignorando le parole precedenti. In un primo momento avevo pensato ad un posizionamento diretto tramite la funzione **seek** ma siccome sono a conoscenza solo del numero di parole già lette e non del numero di byte già letti sono costretto a spostarmi nel file parola per parola.
```
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

```
Le parole vengono lette una per volta. Per ognuna di esse si controlla se sono già state individuate occorrenze precedentemente, in questo caso semplicemente si incrementa il campo *value* della struttura relativa alla parola stessa, altrimenti si inizializza ad 1 una cella non ancora utilizzata (con campo *value* a -1). Ricordiamo che l'array di *info* che conterrà le coppie **parola**-**occorrenza** è della dimensione *uppBSize*, ovvero anche nel caso peggiore in cui tutte le parole del files siano diverse tra loro non si rischierebbe di sforare la dimensione massima.
```
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
```
Tramite un operazione di **MPI_Gather** vengono raccolti tutti gli array **parola**-**occorrenza** calcolati dai processori. L'insieme di questi ultimi sarà l'array *result*.
Essendo *result* la composizione di diversi array ricevuti da diversi processori è necessario compattare il risultato ovvero se una parola è presente più volte nell'array incrementiamo il campo *value* della struttura trovata per prima del campo *value* della struttura trovata per seconda. Quest ' ultima verrà eliminata logicamente settando il suo campo *value* a -1.

Successivamente viene memorizzato il tempo di fine in *tfinish* e vengono accorpate 			
```
				printf("[%d]__word:%s -  #:%d\n", rank,result[i].key,result[i].value);
				
			}
			
		printf("%f\n", tfinish - tstart);
	}
	
	free(mex);
	free(result);
	
	MPI_Finalize();
  return 0;
}
```
Stampa dei risultati, stampa del tempo di esecuzione e finalizzazione dell' ambiente MPI. 

## Testing
I test sono stati effettuati su delle istanze **m4.large** di Amazon AWS.
Si sono effettuati test relativamente allo strong scaling e al weak scaling.

Al fine di agevolare il testing ho creato un generatore di file che li crea partendo da un file dizionario, la radice del nome dei file che verranno creati, numero di file, numero di parole per file ed un seed. Vista la richiesta di un programma in grado di funzionare per input anche di diversa lunghezza ho creato il generatore di file in modo che il numero di parole inserito dall'utente vari per ognuno di essi venendo incrementato o decrementato di una quantità scelta casualmente tra lo 0 ed il 15%

## Strong Scaling
Per testare lo strong scaling ho utilizzato 6 files da circa 10000 parole l'uno. Questa tipologia di test infatti aumenta il numero di processori di iterazione in iterazione lasciando invariata la taglia dell'input.
Nella figura che segue sono indicati in un grafico i risultati dei test relativamente al tempo impiegato ed al numero di processori utilizzato.

![Strong Scaling](https://github.com/fdavino/ProgettoPCPC/blob/master/img/Strong.png?raw=true)


## Weak Scaling
Per testare la weak scaling invece è necessario aumentare il numero di parole da analizzare proporzionalmente al numero di processori. Ho deciso di iniziare con un singolo file da 10000 parole per un singolo processore, fino ad arrivare a 16 file da 10000 parole per 16 processori. I risultati sono mostrati nel grafico che segue relativamente al tempo impiegato ed al numero di processori utilizzato.

![Weak Scaling](https://github.com/fdavino/ProgettoPCPC/blob/master/img/Weak.png?raw=true)

*Se le immagini non risultano visibili, possono essere visionate nella cartella img*

## Compilare
Per compilare wordCount.c va utilizzata l'istruzione seguente
```
mpicc wordCount.c -o wordCount.o
```
Per compilare il generatore di file va utilizzata l'istruzione seguente
```
gcc generator.c -o generator.o
```
Eseguire il generatore per creare i file passando come argomento
* il file dizionario
* il prefisso del nome dei file che verranno creati
* il numero di file da generare
* il numero di parole per file (può subire cambiamenti in positivo ed in negativo dallo 0 al 15% per garantire la casualità del numero di parole per file)
* seme per il generatore pseudo-casuale che verrà utilizzato

```
./generator.o bigDictionary.txt f 16 10000 404
```

Lanciare wordCount sul numero di processori desiderato
```
mpirun -np 4 ./wordCount.o
```




