#include <stdlib.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdint.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include  <ctype.h> 
#include <stdbool.h>
#include "filesys.h"
superblock_t superBlock;

dir_entry_timedate_t createTimeEntry(unsigned char line1[NUMBYTES], unsigned char line2[NUMBYTES]){
    dir_entry_timedate_t createTime;
    createTime.year= (line1[13]<<8| line1[14]);
    createTime.month=line1[15];
     createTime.day=line2[0];
     createTime.hour=line2[1];
     createTime.minute= line2[2];
     createTime.second=line2[3];
     return createTime;
}

dir_entry_timedate_t modifyTimeEntry(unsigned char line2[NUMBYTES]){
    dir_entry_timedate_t modifyTime;
    modifyTime.year= (line2[4]<<8|line2[5]);
    modifyTime.month= line2[6];
    modifyTime.day= line2[7];
    modifyTime.hour=line2[8];
    modifyTime.minute= line2[9];
    modifyTime.second= line2[10];
    return modifyTime;
}

dir_entry_t createEntry(unsigned char line1[NUMBYTES], unsigned char line2[NUMBYTES], unsigned char line3[NUMBYTES], unsigned char line4[NUMBYTES]){
    dir_entry_t dirEntry;
    dirEntry.status=(line1[0]); 
    dirEntry.starting_block=(line1[1]<<24| line1[2]<<16| line1[3]<<8|line1[4]);
    dirEntry.block_count= (line1[5]<<24| line1[6]<<16| line1[7]<<8|line1[8]);
    dirEntry.size= (line1[9]<<24| line1[10]<<16| line1[11]<<8|line1[12]);
    dirEntry.create_time= createTimeEntry(line1, line2);
     dirEntry.modify_time= modifyTimeEntry(line2);
    int index=0;
    int iString= 11;
     for (int i=0; i<5 ; i++){
        if (line2[iString]>= 32 && line2[iString]<=126){
            dirEntry.filename[i]= (uint8_t)line2[iString];
        }else{
            dirEntry.filename[i]='.';
        }
        iString++;
        index++;   
    }

     for (int i=0; i<=16 ; i++){
        if (line3[i]==0){
            break;
        }
        if (line3[i]>= 32 && line3[i]<=126){
            dirEntry.filename[index]= (uint8_t)line3[i];
        }else{
            dirEntry.filename[index]='.';
        }
        index++;
    }

    for (int i=0; i<9 ; i++){
        if(line4[i]==0){
            break;
        }else if (line4[i]>= 32 && line4[i]<=126){
            dirEntry.filename[index]= (uint8_t)line4[i];
        }else{
            dirEntry.filename[index]='.';
        }
        index++;
    }
    dirEntry.filename[index]= '\0';

    return dirEntry;
}

//Obtains information about a directory, such as the number of items inside of it, creation date and size.
dir_entry_t* getDirInfo(FILE *file, int *itemCount, int dirStart){
    int offset=0;
    unsigned char line1[NUMBYTES];
    unsigned char line2[NUMBYTES];
    unsigned char line3[NUMBYTES];
    unsigned char line4[NUMBYTES];
    dir_entry_t* contentSubDir = (dir_entry_t*)malloc(superBlock.root_dir_block_count * sizeof(dir_entry_t));
    fseek(file,dirStart, SEEK_SET);  //seek to next directory block pointer :P
    for (int i=0 ;i<=superBlock.root_dir_block_count;i++){ 
        if (i>0){
            offset+=64; //4 lines of 16 bytes each
            fseek(file,dirStart+offset, SEEK_SET);
        }
        fread(line1, 1, sizeof(line1), file);
        if (line1[0]==0){ //if the status says the file is empty-> dont add to array 
            continue;
        } 
        fread(line2, 1, sizeof(line2), file);
        fread(line3, 1, sizeof(line3), file);
        fread(line4, 1, sizeof(line4), file);
        dir_entry_t newEntry=createEntry(line1, line2, line3, line4);
        if (newEntry.size!=0 && newEntry.starting_block!=0 && newEntry.status!=0 ){
            contentSubDir[*itemCount]= newEntry;
            (*itemCount)++;
        }
    }
    return contentSubDir;
}

/*Obtains information about the super block*/
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

/*Compares the content of 2 strings and returns 1 if they have the same characters*/
int compareStrings (const char* str1, const char* str2) {
    while (*str1 && *str2) {
        if (!isspace(*str1) && !isspace(*str2)) {
            if (tolower(*str1) != tolower(*str2)) {
                return false;  // Different characters found
            }
        } else if (!isspace(*str1) || !isspace(*str2)) {
            return false;  // One string has extra space(s)
        }
        str1++;
        str2++;
    }
    // Check if both strings finished
    return *str1 == '\0' && *str2 == '\0';
}

/*given a path, it returns the file name we are looking for in the given path 
    for instance, given : /sub1/sub2/file.txt returns file.txt
*/
char* findFileName(char* path) {
    const char* lastSlash = strrchr(path, '/');
    if (lastSlash != NULL) {
        return strdup(lastSlash + 1);
    } else {
        return strdup(path);
    }
}

//change name of sub to dir or somethign like that
//return file found by reference to test 1 if file was found 0 otherwise
/*Returns if a file was found in a subdirectory or inside the given directory provided the path. 
If the file we are looking for is found, we return the struct dir_entry_t with all the information 
about it.
*/
dir_entry_t findFile(FILE *fp, char *searchName, dir_entry_t *sub, int *fileFound, int count){
    dir_entry_t fileEntry;
    char firstChar= searchName[0];
     for (int i=0; i<count; i++){
        dir_entry_t entry= sub[i];
        int equal= compareStrings((const char*)entry.filename,searchName);
        if (equal){
            fileEntry= entry;
            *fileFound=1;
            return fileEntry;

        } else if (firstChar== '/' && entry.status==5){
            int itemCount=0;
             char *fileInPath= findFileName(searchName);
            int nestedStart= entry.starting_block*superBlock.block_size;
            dir_entry_t * nested= getDirInfo(fp, &itemCount, nestedStart);
            for(int j=0; j<itemCount; j++){
                dir_entry_t subNested= nested[j];
                equal= compareStrings((const char*)subNested.filename,fileInPath);
                if (equal){
                    fileEntry= subNested;
                    *fileFound=1;
                    return fileEntry;
                }
                if (subNested.status==5){
                    nestedStart= subNested.starting_block*superBlock.block_size;
                    dir_entry_t * reNested= getDirInfo(fp, &itemCount, nestedStart);
                    for (int k=0; k<itemCount; k++){
                        dir_entry_t newNested= reNested[k];
                        equal= compareStrings((const char*)newNested.filename,fileInPath);
                        if (equal){
                            fileEntry= newNested;
                            *fileFound=1;
                            return fileEntry;
                        }
                    }
                }
            }
            }

            }
        exit(1);
        printf("File not found.\n");
        }
      


/* Given a dir_entry_t we find the places in the File Allocation Table where its contents are located.
We return the array of addresses containing the contents of the given file.
*/
uint32_t *gatherFATblocks(FILE *file,dir_entry_t info, int *elem){
    int startAddress= superBlock.fat_start_block*superBlock.block_size;
    int firstAddress= info.starting_block*4;
    int offsetInitial= startAddress+firstAddress;
    uint32_t *addressLog= (uint32_t *)calloc(40, sizeof(uint32_t)); //change the 40 to the max number of entries in a cluster o realloc array
    uint32_t cluster;
    int clusterCount=0 , totalSize=0; //read first cluster
    addressLog[clusterCount] = info.starting_block;
    clusterCount++;
    (*elem)++;
    fseek(file,offsetInitial, SEEK_SET);
    fread(&cluster,sizeof(uint32_t),1,file);
    cluster=ntohl(cluster);
    addressLog[clusterCount] = cluster;
    clusterCount++;
    (*elem)++;


    while (cluster!= 0xffffffff){ //change end to a define in .h file
        totalSize += superBlock.block_size;
        int nextCluster= startAddress+cluster*sizeof(uint32_t); //to get next block
        fseek(file, nextCluster, SEEK_SET);
        fread(&cluster, sizeof(uint32_t), 1, file);
        cluster = ntohl(cluster);
        if (cluster!=0xffffffff){
            addressLog[clusterCount] = cluster;
            clusterCount++;
            (*elem)++;
        }
    }
    return addressLog;
}

/* Using the blocks found where the file contents are located, we copy them to the new destination 
*/
void copyFileContents(FILE *file, char *fileName, dir_entry_t fileToCopy, char *fileNameCopy){
    int count=0;
    uint32_t *address = gatherFATblocks(file,fileToCopy, &count);
    FILE *fp= fopen(fileNameCopy, "w");//maybe change name to output file
    for (int i=0; i<count; i++){
        int nextCluster = address[i] * superBlock.block_size;
        fseek(file, nextCluster, SEEK_SET);

        char buffer[superBlock.block_size];
        fread(buffer, 1, superBlock.block_size, file);
        fwrite(buffer,1, superBlock.block_size, fp);
    }
    fclose(fp);
}



int main(int argc, char *argv[] ){
    char *fileName= argv[1];
    char *SearchName=argv[2];
    char *fileNameWrite= argv[3];
    
    if(fileName==NULL){ //if provided an argument to terminal 
        printf("Could not open it \n");
        exit(1);
    }
    FILE *file= fopen(fileName, "rb");
    int freed=0, reserved=0, allocated=0, itemCount=0;
    getBlockInfo(fileName, &freed, &reserved, &allocated);

    int dirStart= superBlock.root_dir_start_block*superBlock.block_size;
    dir_entry_t *dir =getDirInfo(file, &itemCount, dirStart);  
    int fileFound=-1;
    dir_entry_t fileToCopy=  findFile(file,SearchName,dir, &fileFound, itemCount); //esta AQUI EL BUG
    if (fileFound){
        copyFileContents(file, fileName, fileToCopy,fileNameWrite);
    }
    fclose(file);
}
