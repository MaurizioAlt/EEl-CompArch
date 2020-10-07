//Maurizio Altamura

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>


/*

./SIM <CACHE_SIZE> <ASSOC> <REPLACEMENT> <WB> <TRACE_FILE>
<CACHE_SIZE> is the size of the simulated cache in bytes
<ASSOC> is the associativity
<REPLACEMENT> replacement policy: 0 means LRU, 1 means FIFO
<WB> Write-back policy:0 means write-through, 1 meanswrite-back
<TRACE_FILE> trace file name with full path


./SIM 32768 8 0 1 /home/TRACES/XSBENCH.t
0.112008 - miss ratio
44.000000 - writes hit
2371758.000000 - reads miss

./SIM 32768 8 0 0 /home/TRACES/XSBENCH.t
0.112008
5013495.000000
2371758.000000

TA test case~~
./SIM 32768 8 1 1 XSBENCH.t
0.177705
251583.000000
3762906.000000


gcc sim.c
a 32768 8 0 1 "C:\Users\mauri\OneDrive\Documents\EEL CompArch\Cache\XSBENCH.t"

*/


float cacheMiss = 0;
float cacheHit = 0;
float read = 0;
float write = 0;
char mode;
int WB;



void h2b(char *in, char *out);


int main(int argc, char* argv[])
{    
    //char mode;
    char line[50];
    char in[64];
    char out[64];
    char add[64];

    int cachesize = atoi(argv[1]);
    int assoc = atoi(argv[2]);
    int replacement = atoi(argv[3]);
	int wb = atoi(argv[4]);
	WB = wb;

	//open file
	FILE *inp = fopen(argv[5], "r");
    if(inp == NULL)
    {
        printf("Could not open file.\n");
        return -1;
    }


    int i = 0, j = 0, k = 0;
    unsigned long long int setadd = 0;
    unsigned long long int totalset = 0;
    unsigned long long int totaltag = 0;
    unsigned long long int numAccess = 0;
    int len = 0;
    int blank = 0;
    unsigned long long int blocksize = 64;

	unsigned long long int setnumber = (cachesize/(assoc*blocksize));
    unsigned long long int setbits = log(setnumber)/log(2);
    unsigned long long int blockbits = log(blocksize)/log(2);
    unsigned long long int tagsize = 64 - (blockbits + setbits);
    //printf("%d %d %d %d\n", setnumber, setbits, blockbits, tagsize);

    //lru and dirty 2d array
    char* lru[setnumber][assoc];
    int dirty[setnumber][assoc];
    for(i = 0; i < setnumber; i++)
    {
    	for(j = 0; j < assoc; j++)
	    {
	        lru[i][j] = "0";
	        dirty[i][j] = 0;
	    }
    }

    unsigned long long int indx, indy, indi, indj;
    char blockBits[64];
    char setBits[64];
    char *tagBits = malloc(64);
    int flag = 0; 

    while(fgets(line, 25, inp) != NULL)
    {
    	//intialize
    	numAccess++;
    	//printf("LOOP %d\n", numAccess);

    	setadd = 0;
        totalset = 0;

        //scan
        sscanf(line, "%c %s", &mode, &add);
        //printf("%c %s\n", mode, add);

        //shift add
        memmove(add, add + 2, strlen(add));
        //printf("%s\n", add);

        len = strlen(add);
       	blank = (16 - len);
       	//printf("%d %d\n\n", len, blank);

       	//fill in add
        for(i = 0; i < blank; i++)
            {in[i] = '0';}

        for(i = blank, j = 0; i < 16; j++, i++)
            {in[i]=add[j];}

        in[16] = '\0';
        //printf("%s\n", in);

        //hex to binary
        h2b(in, out);
        //printf("%s\n", out);

        //add into tag, set and block bits
        for(indx = 0, indy = (64-blockbits); indy < 64; indy++, indx++)
           {blockBits[indx] = out[indy];}
    	blockBits[indx]='\0';

    	for(indx = 0, indi = (64-(blockbits+setbits)); indi < (64-(blockbits)); indi++, indx++)
        	{setBits[indx] = out[indi];}
    	setBits[indx]='\0';

    	for(indx = 0, indj = 0; indj < (64-(blockbits+setbits)); indj++, indx++)
        	{tagBits[indx] = out[indj];}
    	tagBits[indx]='\0';

    	//printf("%s %s %s\n", tagBits, setBits, blockBits);

	    //string into int
	    for(indx = 0, indi = (setbits - 1); indx < setbits; indx++, indi--)
    	{
            if(setBits[indx] == '1')
                setadd = 1;
            if(setBits[indx] == '0')
                setadd = 0;
            
            setadd = setadd * pow(2, indi);
            totalset += setadd;
        }

        for(indx = 0, indi = (tagsize - 1); indx < tagsize; indx++, indi--)
    	{
            if(tagBits[indx] == '1')
                setadd = 1;
            if(tagBits[indx] == '0')
                setadd = 0;

            setadd = setadd * pow(2, indi);
            totaltag += setadd;
        }

        totalset = totalset % blocksize;
        totaltag =  totaltag;

        //printf("accessing array[%d][] = %s\n", totalset, tagBits);
    	char *tagBits2 = malloc(100);
        strcpy(tagBits2, tagBits);

        //Calculate
        //LRU
        if(replacement == 0)
        {
        	flag = 0;

	        for(i = 0; i < assoc; i++)
			{
				if(strcmp(lru[totalset][i], tagBits) == 0)
				{
					//hit
					flag = 1;
					cacheHit++;
			    	if(WB == 0 && mode == 'W')
			    		write++;
			    	
			    	if(i != 0)
			    	{
			    		//keep track of prev dirty
			    		int prev = dirty[totalset][i];

			    		//move over the slots
			    		for(j = i-1; j >= 0; j--)
			    		{
					    	lru[totalset][j+1] = lru[totalset][j];
					    	dirty[totalset][j+1] = dirty[totalset][j];
					    }
					 
					 	//move to front of slots
					    if(WB == 1 && mode == 'W')
					    	dirty[totalset][0] = 1;
					    else if(prev == 1 && WB == 1 && mode == 'R')
					    	dirty[totalset][0] = 1;
					    else if(prev == 0 && WB == 1 && mode == 'R')
					    	dirty[totalset][0] = 0;
					    lru[totalset][0] = tagBits2;

					    break;
			    	}
			    	else if(i == 0 && WB == 1 && mode == 'W')
				    	dirty[totalset][0] = 1;

				}

			}

		

			if(flag == 0)
			{
				//miss
				cacheMiss++;
		    	if(mode == 'R')
		    		read++;
		    	if(WB == 0 && mode == 'W')
		    	{
		    		read++;
		    		write++;
		    	}
		    	if(WB == 1 && mode == 'W')
		    		read++;

		    	//evict dirty?
		    	if(dirty[totalset][assoc-1] == 1 && WB == 1)
			    	write++;

		    	for(j = assoc-2; j >= 0; j--)
		    	{
				    dirty[totalset][j+1] = dirty[totalset][j];
				    lru[totalset][j+1] = lru[totalset][j];
		    	}
			 
			    if(WB == 1 && mode == 'W')
			    	dirty[totalset][0] = 1;
			    else if(WB == 1 && mode == 'R')
			    	dirty[totalset][0] = 0;

			    //create new slot at front
			    lru[totalset][0] = tagBits2;
			}

		}


		//FIFO
        else if(replacement == 1)
        {
        	flag = 0; 

	        for(i = 0; i < assoc; i++)
			{
				if(strcmp(lru[totalset][i], tagBits) == 0)
				{
					flag = 1;
					cacheHit++;
			    	if(WB == 0 && mode == 'W')
			    		write++;

					//WriteBack
				    if(WB == 1 && mode == 'W')
				    	dirty[totalset][i] = 1;
				    else if(dirty[totalset][i] == 1 && WB == 1 && mode == 'R')
				    	dirty[totalset][i] = 1;
				    else if(WB == 1 && mode == 'R')
				    	dirty[totalset][i] = 0;

				    break;

				}

			}
		
			if(flag == 0)
			{
				//miss
				cacheMiss++;
		    	if(mode == 'R')
		    		read++;
		    	if(WB == 0 && mode == 'W')
		    	{
		    		read++;
		    		write++;
		    	}
		    	if(WB == 1 && mode == 'W')
		    		read++;

		    	//evict dirty?
		    	if(WB == 1 && dirty[totalset][assoc-1] == 1)
			    	write++;

			    //shift slots down
		    	for(j = assoc-2; j >= 0; j--)
			    	lru[totalset][j+1] = lru[totalset][j];

		    	if(WB == 1 && mode == 'W')
			    	dirty[totalset][0] = 1;
			    else if(WB == 1 && mode == 'R')
			    	dirty[totalset][0] = 0;
			 
			 	//create slot at front
			    lru[totalset][0] = tagBits2;

			}

		}
		//free(tagBits2);
		//printf("%d: lru[46][0] = %s\n\n", numAccess, lru[46][0]);
    }
        	
    //evict cache
	// if(WB == 1)
	// {
	// 	for(i = 0; i < setnumber; i++)
	// 	{
	// 		for(j = 0; j < assoc; j++)
	// 	    {
	// 	        if(dirty[i][j] == 1)
	// 	        	write++;
	// 	    }
	// 	}
	// }

    free(tagBits);

    float hitratio = cacheHit/numAccess;
  	float missratio = 1 - hitratio;
  	missratio = cacheMiss/numAccess;
    
	// printf("\nMiss ratio %f\n", missratio);
	// printf("Writes: %f ", write);
	// printf("Hits: %f\n", cacheHit);
	// printf("Reads: %f ", read);
	// printf("Misses: %f\n", cacheMiss);

    printf("%f\n", missratio);
    printf("%f\n", write);
	printf("%f\n", read);

	return 1;
}



void h2b(char *in, char *out)
{
    int i;
    int len = strlen(in);

    for(i = 0; i < len; i++)
    {
    	switch(in[i])
    	{
    		case '0': 
	            out[i*4] = '0';
	            out[i*4 + 1] = '0';
	            out[i*4 + 2] = '0';
	            out[i*4 + 3] = '0';
	            break; 
	        case '1': 
	            out[i*4] = '0';
	            out[i*4 + 1] = '0';
	            out[i*4 + 2] = '0';
	            out[i*4 + 3] = '1';
	            break; 
	        case '2': 
	            out[i*4] = '0';
	            out[i*4 + 1] = '0';
	            out[i*4 + 2] = '1';
	            out[i*4 + 3] = '0'; 
	            break; 
	        case '3': 
	            out[i*4] = '0';
	            out[i*4 + 1] = '0';
	            out[i*4 + 2] = '1';
	            out[i*4 + 3] = '1';
	            break; 
	        case '4': 
	            out[i*4] = '0';
	            out[i*4 + 1] = '1';
	            out[i*4 + 2] = '0';
	            out[i*4 + 3] = '0';
	            break; 
	        case '5': 
	            out[i*4] = '0';
	            out[i*4 + 1] = '1';
	            out[i*4 + 2] = '0';
	            out[i*4 + 3] = '1'; 
	            break; 
	        case '6': 
	            out[i*4] = '0';
	            out[i*4 + 1] = '1';
	            out[i*4 + 2] = '1';
	            out[i*4 + 3] = '0'; 
	            break; 
	        case '7': 
	            out[i*4] = '0';
	            out[i*4 + 1] = '1';
	            out[i*4 + 2] = '1';
	            out[i*4 + 3] = '1';
	            break; 
	        case '8': 
	            out[i*4] = '1';
	            out[i*4 + 1] = '0';
	            out[i*4 + 2] = '0';
	            out[i*4 + 3] = '0';
	            break; 
	        case '9': 
	            out[i*4] = '1';
	            out[i*4 + 1] = '0';
	            out[i*4 + 2] = '0';
	            out[i*4 + 3] = '1';
	            break; 
	        case 'A': 
	        case 'a': 
	            out[i*4] = '1';
	            out[i*4 + 1] = '0';
	            out[i*4 + 2] = '1';
	            out[i*4 + 3] = '0';
	            break; 
	        case 'B': 
	        case 'b': 
	            out[i*4] = '1';
	            out[i*4 + 1] = '0';
	            out[i*4 + 2] = '1';
	            out[i*4 + 3] = '1'; 
	            break; 
	        case 'C': 
	        case 'c': 
	            out[i*4] = '1';
	            out[i*4 + 1] = '1';
	            out[i*4 + 2] = '0';
	            out[i*4 + 3] = '0';
	            break; 
	        case 'D': 
	        case 'd': 
	           out[i*4] = '1';
	            out[i*4 + 1] = '1';
	            out[i*4 + 2] = '0';
	            out[i*4 + 3] = '1';
	            break; 
	        case 'E': 
	        case 'e': 
	            out[i*4] = '1';
	            out[i*4 + 1] = '1';
	            out[i*4 + 2] = '1';
	            out[i*4 + 3] = '0';
	            break; 
	        case 'F': 
	        case 'f': 
	            out[i*4] = '1';
	            out[i*4 + 1] = '1';
	            out[i*4 + 2] = '1';
	            out[i*4 + 3] = '1';
	            break; 
	        default: 
        		printf("\nInvalid hexadecimal digit %c", in[i]); 
    	}
    }
    out[64] = '\0';
}




