#include <stdlib.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include "filesys.h"

superblock_t superBlock;

void printInfo(int *freed, int *reserved, int *allocated){
    printf("Super block information: \n");
    printf("Block size: %2d \n", superBlock.block_size); 
    printf("Block count: %2d \n",superBlock.file_system_block_count);
    printf("FAT starts: %2d \n",superBlock.fat_start_block);
    printf("FAT blocks: %2d \n",superBlock.fat_block_count);
    printf("Root directory start: %2d \n",superBlock.root_dir_start_block);
    printf("Root directory blocks: %2d \n",superBlock.root_dir_block_count);
    printf("\n");
    printf("FAT information \n");
    printf("Free Blocks: %u \n", *freed);
    printf("Reserved Blocks: %u \n", *reserved);
    printf("Allocated Blocks: %u \n", *allocated);

}


/*Gathers all the data of the superblock from the provided hexdump, by assigning it to a superBlock struct 
    Returns the number of blocks freed, reserved and allocated in a pointer
*/
void getBlockInfo(char *name, int *freed, int *reserved, int *allocated){
    unsigned char firstLine[NUMBYTES];
    unsigned char secondLine[NUMBYTES];
    FILE *file= fopen(name, "rb");
    if (file==NULL){
        printf("Could not open file %s\n", name);
        exit(1);
    }
    fread(firstLine, 1, sizeof(firstLine), file);
    fread(secondLine, 1, sizeof(secondLine), file);

    //Assign values to super block info
    unsigned short bSize = (firstLine[B_SIZE] << 8) | firstLine[B_SIZE+1]; //bit shift and OR expressions to combine 
    memmove(&superBlock.block_size, &bSize,4);
    superBlock.file_system_block_count=(firstLine[FILE_SYS_COUNT]<< 24 |firstLine[FILE_SYS_COUNT+1]<< 16 | firstLine[FILE_SYS_COUNT+2]<< 8 |firstLine[FILE_SYS_COUNT+3]); 
    superBlock.fat_start_block=(firstLine[FAT_START]<< 24 |firstLine[FAT_START+1]<< 16 | secondLine[0]<< 8 |secondLine[1]); 
    superBlock.fat_block_count=(secondLine[FAT_BLOCKS]<< 24 |secondLine[FAT_BLOCKS+1]<< 16 | secondLine[FAT_BLOCKS+2]<< 8 |secondLine[FAT_BLOCKS+3]) ;
    superBlock.root_dir_start_block=(secondLine[ROOT_DIR_START]<< 24 |secondLine[ROOT_DIR_START+1]<< 16 | secondLine[ROOT_DIR_START+2]<< 8 |secondLine[ROOT_DIR_START+3]) ;
    superBlock.root_dir_block_count=(secondLine[ROOT_DIR_BLOCKS]<< 24 |secondLine[ROOT_DIR_BLOCKS+1]<< 16 | secondLine[ROOT_DIR_BLOCKS+2]<< 8 |secondLine[ROOT_DIR_BLOCKS+3]) ;

    int fatStart= superBlock.fat_start_block*superBlock.block_size;
    int fatEnd= ((superBlock.block_size/4)*superBlock.fat_block_count);  
    uint32_t chunk;
    fseek(file,fatStart, SEEK_SET); // seek to the start of FAT placed in memory
    for (int i=0; i<fatEnd; i++){
        int read= fread(&chunk, 4,1,file); //read 1 chunck of 4 bits at a time 
        if (read==0){
            printf("error ocurred \n");
            exit(1);
        }
        chunk=ntohl(chunk); //covert endiandness 
        if (chunk==1){
            (*reserved)++;
        }else if (chunk==0){
            (*freed)++;
        }else{
            (*allocated)++;
        }
    }
    fclose(file);
}
    
    
int main(int argc, char *argv[] ){
    char *fileName= argv[1];
    if(fileName==NULL){ //if provided an argument to terminal 
        printf("Could not open it \n");
        exit(1);
    }
    int freed=0, reserved=0, allocated=0;
    getBlockInfo(fileName, &freed, &reserved, &allocated);
    printInfo(&freed, &reserved, &allocated);

}
