#include<stdio.h>
#include<stdlib.h>
#include<math.h>

//initializing cache parameters 
int blocksize=0;
int l1_size=0;
int l1_assoc=0;
int l2_size=0;
int l2_assoc=0;
int replacement_policy=0;
int inclusion_property=0;

//initializing cacheline parameters
int l1index_w=0;
int l2index_w=0;
int l1offset_w=0;
int l2offset_w=0;
int l1tag_w=0;
int l2tag_w=0;

//cache performance parameters
int l1counter=0;
int l1read_hits=0;
int l1read_miss=0;
int l1write_hits=0;
int l1write_miss=0;
int l1reads=0;
int l1writes=0;
int l1write_backs=0;
int l2counter=0;
int l2read_hits=0;
int l2read_miss=0;
int l2write_hits=0;
int l2write_miss=0;
int l2reads=0;
int l2writes=0;
int l2write_backs=0;

unsigned long address;
unsigned long optimal[100001];
int line_counter=0;
int total_lines=0;

//initializing Cache
typedef struct Cache{
    int valid;
    int dirty;
    unsigned long tag;
    int counter;
}Cache;


//function for finding a replacing candidate in cache using LRU or FIFO policy
int LRUorFIFO(Cache Lcache[][64], unsigned long index, int level)
{
    int i,rep_policy=0,way=0, assoc;

    if(level==1){
        assoc=l1_assoc;
    }else{
        assoc=l2_assoc;
    }

    rep_policy=Lcache[index][0].counter;
    for(i=1;i<assoc;i++)
    {
        if(Lcache[index][i].counter<rep_policy)
        {
            rep_policy=Lcache[index][i].counter;
            way=i;
        }
    }
    return way;
}

//function to get the tag from L1 Cache
unsigned long getTagL1(unsigned long address){
    return address>>(l1index_w+l1offset_w);
}

//function to get the index from L2 Cache
unsigned long getIndexL1(unsigned long address){
    return (address<<l1tag_w)>>(l1tag_w+l1offset_w);
}

//function to get the tag from L2 Cache
unsigned long getTagL2(unsigned long address){
    return address>>(l2index_w+l2offset_w);
}

//function to get the index from L2 Cache
unsigned long getIndexL2(unsigned long address){
    return (address<<l2tag_w)>>(l2tag_w+l2offset_w);
}

//function for finding a replacing candidate in  cache using Optimal poliy
int Optimal(Cache Lcache[][64], unsigned long index, int level){
    
    int i, j, way=0, assoc, far=0, occ;
    unsigned long tag;
    unsigned long index1;

    if(level==1){
        assoc=l1_assoc;
    }else{
        assoc=l2_assoc;
    }

    for(i=0; i<assoc; i++){
        occ=0;
        j=0;
        for(j=line_counter+1; j<total_lines; j++){
            if(level==1){
                tag=getTagL1(optimal[j]);
                index1=getIndexL1(optimal[j]);
            }else{
                tag=getTagL2(optimal[j]);
                index1=getIndexL2(optimal[j]);
            }
            if(Lcache[index][i].tag==tag && index==index1){
                occ=j;
                break;
            }
        }
        if(j==total_lines){
            way=i;
            break;
        }else if(far<occ){
            far=occ;
            way=i;
        }
    }
    return way;

}

//function for invalidating L1 Cache incase of inclusive property
void invalidateL1(Cache L1Cache[][64], unsigned long address){
    unsigned long tag=getTagL1(address);
    unsigned long index=getIndexL1(address);
    int i, found=0;
    for(i=0; i<l1_assoc; i++){
        if(L1Cache[index][i].tag==tag && L1Cache[index][i].valid==1){
            L1Cache[index][i].valid=0;
            L1Cache[index][i].dirty=0;
            found=1;
        }
    }
}

//function for write access in L2 Cache
void writeL2(Cache L2Cache[][64], Cache L1Cache[][64], unsigned long address){
    unsigned long tag=getTagL2(address);
    unsigned long index=getIndexL2(address);
    unsigned long evict_tag;
    unsigned long evict_offset;
    unsigned long evict_address;
    int i, found=0, way;
    l2writes+=1;
    l2counter+=1;
    for(i=0; i<l2_assoc; i++){
        if(L2Cache[index][i].tag==tag && L2Cache[index][i].valid==1){
            l2write_hits+=1;
            found=1;
            L2Cache[index][i].dirty=1;
            if(replacement_policy==0){
                L2Cache[index][i].counter=l2counter;
            }
            break;
        }
    }
    if(found==0){
        l2write_miss+=1;
        for(i=0; i<l2_assoc; i++){
            if(L2Cache[index][i].valid==0){
                way=i;
                break;
            }
        }
        if(i==l2_assoc){
            if(replacement_policy==0 || replacement_policy==1){
                way=LRUorFIFO(L2Cache, index, 2);
            }else if(replacement_policy==2){
                way=Optimal(L2Cache, index, 2); 
            }
        }
        if(inclusion_property==1){
            evict_tag=L2Cache[index][way].tag;
            evict_offset=(address<<(l2tag_w+l2index_w))>>(l2tag_w+l2index_w);
            evict_address=((((evict_tag<<l2index_w) | (index)) << (l2offset_w)) | (evict_offset));
            invalidateL1(L1Cache, evict_address);
        }
        if(L2Cache[index][way].dirty==1 && L2Cache[index][way].valid==1){
            l2write_backs+=1;
        }

        L2Cache[index][way].tag=tag;
        L2Cache[index][way].valid=1;
        L2Cache[index][way].dirty=1;
        L2Cache[index][way].counter=l2counter;
    }

}

//function for read access in L2 Cache
void readL2(Cache L2Cache[][64], Cache L1Cache[][64], unsigned long address){
    unsigned long tag=getTagL2(address);
    unsigned long index=getIndexL2(address);
    unsigned long evict_tag;
    unsigned long evict_offset;
    unsigned long evict_address;
    int i, found=0, way;
    l2reads+=1;
    l2counter+=1;
    for(i=0; i<l2_assoc; i++){
        if(L2Cache[index][i].tag==tag && L2Cache[index][i].valid==1){
            l2read_hits+=1;
            found=1;
            if(replacement_policy==0){
                L2Cache[index][i].counter=l2counter;
            }
            break;
        }
    }
    if(found==0){
        l2read_miss+=1;
        for(i=0; i<l2_assoc; i++){
            if(L2Cache[index][i].valid==0){
                way=i;
                break;
            }
        }
        if(i==l2_assoc){
            if(replacement_policy==0 || replacement_policy==1){
                way=LRUorFIFO(L2Cache, index, 2);
            }else if(replacement_policy==2){
                way=Optimal(L2Cache, index, 2); 
            }
        }
        if(inclusion_property==1){
            evict_tag=L2Cache[index][way].tag;
            evict_offset=(address<<(l2tag_w+l2index_w))>>(l2tag_w+l2index_w);
            evict_address=((((evict_tag<<l2index_w) | (index)) << (l2offset_w)) | (evict_offset));
            invalidateL1(L1Cache, evict_address);
        }
        if(L2Cache[index][way].dirty==1 && L2Cache[index][way].valid==1){
            l2write_backs+=1;
        }

        L2Cache[index][way].tag=tag;
        L2Cache[index][way].valid=1;
        L2Cache[index][way].dirty=0;
        L2Cache[index][way].counter=l2counter;
    }

}

//function for read access in L1 Cache
void readL1(Cache L1Cache[][64], Cache L2Cache[][64], unsigned long address){
    unsigned long tag=getTagL1(address);
    unsigned long index=getIndexL1(address);
    unsigned long replace_tag;
    unsigned long replace_offset;
    unsigned long replace_address;
    int i, found=0, way;
    l1reads+=1;
    l1counter+=1;
    for(i=0; i<l1_assoc; i++){
        if(L1Cache[index][i].tag==tag && L1Cache[index][i].valid==1){
            l1read_hits+=1;
            found=1;
            if(replacement_policy==0){
                L1Cache[index][i].counter=l1counter;
            }
            break;
        }
    }
    if(found==0){
        l1read_miss+=1;
        for(i=0; i<l1_assoc; i++){
            if(L1Cache[index][i].valid==0){
                way=i;
                break;
            }
        }
        if(i==l1_assoc){
            if(replacement_policy==0 || replacement_policy==1){
                way=LRUorFIFO(L1Cache, index, 1);
            }else if(replacement_policy==2){
                way=Optimal(L1Cache, index, 1); 
            }
        }
        if(L1Cache[index][way].dirty==1 && L1Cache[index][way].valid==1){
            l1write_backs+=1;
            if(l2_size!=0){
                replace_tag=L1Cache[index][way].tag;
                replace_offset=(address<<(l1tag_w+l1index_w))>>(l1tag_w+l1index_w);
                replace_address=((((replace_tag<<l1index_w) | (index)) << (l1offset_w)) | (replace_offset));
                writeL2(L2Cache, L1Cache, replace_address);
            }
        }
        if(l2_size!=0){
            readL2(L2Cache, L1Cache, address);
        }

        L1Cache[index][way].tag=tag;
        L1Cache[index][way].valid=1;
        L1Cache[index][way].dirty=0;
        L1Cache[index][way].counter=l1counter;
    }

}

//function for write access in L1 Cache
void writeL1(Cache L1Cache[][64], Cache L2Cache[][64], unsigned long address){
    unsigned long tag=getTagL1(address);
    unsigned long index=getIndexL1(address);
    unsigned long replace_tag;
    unsigned long replace_offset;
    unsigned long replace_address;
    int i, found=0, way;
    l1writes+=1;
    l1counter+=1;
    for(i=0; i<l1_assoc; i++){
        if(L1Cache[index][i].tag==tag && L1Cache[index][i].valid==1){
            l1write_hits+=1;
            found=1;
            L1Cache[index][i].dirty=1;
            if(replacement_policy==0){
                L1Cache[index][i].counter=l1counter;
            }
            break;
        }
    }
    if(found==0){
        l1write_miss+=1;
        for(i=0; i<l1_assoc; i++){
            if(L1Cache[index][i].valid==0){
                way=i;
                break;
            }
        }
        if(i==l1_assoc){
            if(replacement_policy==0 || replacement_policy==1){
                way=LRUorFIFO(L1Cache, index, 1);
            }else if(replacement_policy==2){
                way=Optimal(L1Cache, index, 1); 
            }
        }
        if(L1Cache[index][way].dirty==1 && L1Cache[index][way].valid==1){
            l1write_backs+=1;
            if(l2_size!=0){
                replace_tag=L1Cache[index][way].tag;
                replace_offset=(address<<(l1tag_w+l1index_w))>>(l1tag_w+l1index_w);
                replace_address=((((replace_tag<<l1index_w) | (index)) << (l1offset_w)) | (replace_offset));
                writeL2(L2Cache, L1Cache, replace_address);
            }
        }
        if(l2_size!=0){
            readL2(L2Cache, L1Cache, address);
        }

        L1Cache[index][way].tag=tag;
        L1Cache[index][way].valid=1;
        L1Cache[index][way].dirty=1;
        L1Cache[index][way].counter=l1counter;
    }

}



int main(int argc, char *argv[]){
    //printf("Entered Parameters are :-")
    setbuf(stdout,NULL);
    int i,j;
    blocksize=atoi(argv[1]);
    l1_size=atoi(argv[2]);
    l1_assoc=atoi(argv[3]);
    l2_size=atoi(argv[4]);
    l2_assoc=atoi(argv[5]);
    replacement_policy=atoi(argv[6]);
    inclusion_property=atoi(argv[7]);
    //creating L1 Cache
    //printf("%d\n", l1_assoc);
    Cache L1Cache[(l1_size/(blocksize*l1_assoc))][64];
    Cache L2Cache[2000][64];
    
    for(i=0; i<(l1_size/(blocksize*l1_assoc)); i++){
        for(j=0; j<l1_assoc; j++){
            L1Cache[i][j].valid=0;
            L1Cache[i][j].dirty=0;
            L1Cache[i][j].tag=0;
            L1Cache[i][j].counter=0;
        }
    }

    //creating L2 Cache if exists
    if(l2_size!=0){
        Cache L2Cache[(l2_size/(blocksize*l2_assoc))][64];
        for(i=0; i<(l2_size/(blocksize*l2_assoc)); i++){
            for(j=0; j<l2_assoc; j++){
                L2Cache[i][j].valid=0;
                L2Cache[i][j].dirty=0;
                L2Cache[i][j].tag=0;
                L2Cache[i][j].counter=0;
            }
        }
    }

    l1index_w=(int)(log(l1_size/(l1_assoc*blocksize))/log(2));
    l1offset_w=(int)(log(blocksize)/log(2));
    l1tag_w=64-l1index_w-l1offset_w;

    if(l2_size!=0){
        l2index_w=(int)(log(l2_size/(l2_assoc*blocksize))/log(2));
        l2offset_w=(int)(log(blocksize)/log(2));
        l2tag_w=64-l2index_w-l2offset_w;    
    }

    char op;

    //storing the entire trace in case of Optimal replacement policy
    if(replacement_policy==2){
        FILE *fp1;
        fp1=fopen(argv[8],"r");
        fseek(fp1,0,SEEK_SET);
        if(fp1==NULL)
            {
                printf("\nNo Trace Content in File!\n");
                exit(1);
            }

        line_counter=0;
        while(1){
            if(feof(fp1))
            {
                break;
            }
            fscanf(fp1,"%c %lx\n",&op,&address);
            optimal[line_counter]=address;
            line_counter+=1;
        }
        total_lines=line_counter;
    }
    line_counter=0;

    
    FILE *fp2;
    fp2=fopen(argv[8],"r");
    fseek(fp2,0,SEEK_SET);
    while(1)
    {
        if(feof(fp2))
        {
            break;
        }
        fscanf(fp2,"%c %lx\n",&op,&address);

        if(op=='r'){
            readL1(L1Cache, L2Cache, address);
        }
        else if(op=='w'){
            writeL1(L1Cache, L2Cache, address);         
        }
        line_counter+=1;
    }

    printf("===== Simulator configuration =====\n");
    printf("BLOCKSIZE:             %d\n",blocksize);
    printf("L1_SIZE:               %d\n",l1_size);
    printf("L1_ASSOC:              %d\n",l1_assoc);
    printf("L2_SIZE:               %d\n",l2_size);
    printf("L2_ASSOC:              %d\n",l2_assoc);
    if(replacement_policy==0){
        printf("REPLACEMENT POLICY:    LRU\n");
    }else if(replacement_policy==1){
        printf("REPLACEMENT POLICY:    FIFO\n");
    }else if(replacement_policy==2){
        printf("REPLACEMENT POLICY:    Optimal\n");
    }

    if(inclusion_property==0){
        printf("INCLUSION PROPERTY:    non-inclusive\n");
    }else if(inclusion_property==1){
        printf("INCLUSION PROPERTY:    inclusive\n");
    }

    printf("trace_file:            %s\n",argv[8]);

    printf("===== L1 contents =====\n");
    for(i=0; i<(l1_size/(blocksize*l1_assoc)); i++){
        printf("Set    %d:    ", i);
        for(j=0;j<l1_assoc; j++){
            printf("%lx ", L1Cache[i][j].tag);
            if(L1Cache[i][j].dirty==1){
                printf("D");
            }else{
                printf(" ");
            }
            printf("    ");
        }
        printf("\n");
    }
    if(l2_size!=0){
        printf("===== L2 contents =====\n");
        for(i=0; i<(l2_size/(blocksize*l2_assoc)); i++){
            printf("Set    %d:    ", i);
            for(j=0;j<l2_assoc; j++){
                printf("%lx ", L2Cache[i][j].tag);
                if(L2Cache[i][j].dirty==1){
                    printf("D");
                }else{
                    printf(" ");
                }
                printf("    ");
            }
            printf("\n");
        }   
    }

    float l1miss_rate=0, l2miss_rate=0;
    int memory_traffic=0;

    l1miss_rate=(float)(((float)l1read_miss+(float)l1write_miss)/((float)l1reads+(float)l1writes));

    if(l2_size==0){
        l2miss_rate=0;
    }else{
        l2miss_rate=(float)((float)l2read_miss/((float)l2reads));
    }

    if(l2_size==0){
        memory_traffic=l1read_miss+l1write_miss+l1write_backs;
    }else{
        memory_traffic=l2read_miss+l2write_miss+l2write_backs;
    }

    printf("===== Simulation results (raw) =====\n");
    printf("a. number of L1 reads:        %d\n",l1reads);
    printf("b. number of L1 read misses:  %d\n",l1read_miss);
    printf("c. number of L1 writes:       %d\n",l1writes);
    printf("d. number of L1 write misses: %d\n",l1write_miss);
    printf("e. L1 miss rate:              %f\n",l1miss_rate);
    printf("f. number of L1 writebacks:   %d\n",l1write_backs);
    printf("g. number of L2 reads:        %d\n",l2reads);
    printf("h. number of L2 read misses:  %d\n",l2read_miss);
    printf("i. number of L2 writes:       %d\n",l2writes);
    printf("j. number of L2 write misses: %d\n",l2write_miss);
    printf("k. L2 miss rate:              %f\n",l2miss_rate);
    printf("l. number of L2 writebacks:   %d\n",l2write_backs);
    printf("m. total memory traffic:      %d\n",memory_traffic);
}