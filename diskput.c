#include <stdlib.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <dirent.h>
#include <time.h>
#include <string.h>
#include <sys/types.h>
#include <stdint.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdint.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <math.h>
#include  <ctype.h> 
#include <stdbool.h>
#include "filesys.h"
superblock_t superBlock;


//Structure for time and date creation for a directory entry
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

//Structure for time and date modification for a directory entry
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

/*Creates a directory entry for a given file*/
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

/*Obtains information about a directory, such as the number of items inside of it, creation date and size.*/
dir_entry_t* getDirInfo(FILE *file, int *itemCount, int dirStart){
    int offset=0;
    dir_entry_t newEntry;
    newEntry.size=0;
    newEntry.starting_block=0;
    newEntry.status=0;

    unsigned char line1[NUMBYTES];
    unsigned char line2[NUMBYTES];
    unsigned char line3[NUMBYTES];
    unsigned char line4[NUMBYTES];

    dir_entry_t* contentSubDir = (dir_entry_t*)malloc(superBlock.root_dir_block_count * sizeof(dir_entry_t));
    fseek(file,dirStart, SEEK_SET);  //seek to next directory block pointer :P
    for (int i=0 ;i<superBlock.root_dir_block_count;i++){ 
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
        newEntry=createEntry(line1, line2, line3, line4);
        if (newEntry.size!=0 && newEntry.starting_block!=0 && newEntry.status!=0 ){
            contentSubDir[*itemCount]= newEntry;
            (*itemCount)++;
        }
    }
    return contentSubDir;
}

/*Obtains information about the super block*/
void getBlockInfo(FILE *file, int *freed, int *reserved, int *allocated){
    unsigned char firstLine[NUMBYTES];
    unsigned char secondLine[NUMBYTES];
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
    for (int i=0; i<=fatEnd; i++){
        int read= fread(&chunk, 4,1,file); //read 1 chunck at a time 
        if (read==0){
            printf("error ocurred \n");
            exit(1);
        }
        chunk=ntohl(chunk); //covert to little endian? have same endiandness
        if (chunk==1){
            (*reserved)++;
        }else if (chunk==0){
            (*freed)++;
        }else{
            (*allocated)++;
        }
    }
}

/*Removes a slash in a string and returns the same string without the / separating it */
void removeSlash(char *str) {
    int len = strlen(str);
    int j = 0;
    int countSlash=0;
    for (int i = 0; i < len; i++) {
        if(str[i]=='/'){
            countSlash++;
        }
        if (str[i] != '/') {
            str[j++] = str[i];
        }if (countSlash>=2 && str[i]=='/'){
                str[j++] = ' ';  // Add a space after the third slash
                countSlash++;
            }
    }
    str[j] = '\0'; // Add the null terminator to mark the end of the new string
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

/*Returns if a file was found in a subdirectory or inside the given directory provided the path. 
If the file we are looking for is found, we return the struct dir_entry_t with all the information 
about it.
*/
dir_entry_t findFile(FILE *fp, char *searchName, dir_entry_t *sub, int *fileFound, int count){
    dir_entry_t fileEntry;
    char firstChar= searchName[0];
     for (int i=0; i<count; i++){
        dir_entry_t entry= sub[i];
        int equal= compareStrings((const char *)entry.filename,searchName);
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
                equal= compareStrings((const char *)subNested.filename,fileInPath);
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
                        equal= compareStrings((const char *)newNested.filename,fileInPath);
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
      
/* 
    Reads the user's current LINUX directory and looks for a match of stringToFound with a file in the current directory.
    It also uses fileStat to gather more information about the file we are processing.
    If the file was found, it copies the content into a char* which is returned.
    Otherwise we exit and print "File not found."
*/
char* readDirContents(char *stringToFound, struct stat *fileStat) {
    FILE *fp;
    DIR *folderDir;
    folderDir = opendir(".");
    struct dirent *entry;
    int filesCount = 0;

    if (folderDir == NULL) {
        printf("Could not read the directory");
        exit(1);
    }

    char *fileContent = NULL;
    size_t totalFileSize = 0;  // To keep track of the total size

    while ((entry = readdir(folderDir))) {
        filesCount++;
        if (compareStrings(stringToFound, entry->d_name)) {
            fp = fopen(entry->d_name, "r");
            if (fp == NULL) {
                fprintf(stderr, "Could not open file %s for reading.\n", entry->d_name);
                exit(1);
            }
            if (stat(entry->d_name, fileStat) != 0) {
                fprintf(stderr, "Could not get file information for %s.\n", entry->d_name);
                exit(1);
            }
            totalFileSize += fileStat->st_size; //update for dynamic memory
            fileContent = (char*)realloc(fileContent, totalFileSize + 1); // +1 for null terminator

            if (fileContent == NULL) {
                fprintf(stderr, "Memory allocation error.\n");
                exit(1);
            }
            // Read the file contents
            fread(fileContent + totalFileSize - fileStat->st_size, 1, fileStat->st_size, fp);
            fclose(fp);
            closedir(folderDir);
            return fileContent;
        }
    }
    printf("File not found.\n");
    exit(1);
}

/* 
    Finds the offset of the blocks in the FAT which are available to write to. It returns
    them into an array to be used. 
*/
uint32_t * freeBlocksAddress(FILE *fp, uint32_t freed,long sizeRequested) {
   int fatStart= superBlock.fat_start_block*superBlock.block_size;
    int entry=0;
    uint32_t chunck;
    long availableSize= freed*superBlock.block_size;
    if(sizeRequested>availableSize){
        printf("No space to allocate this on the FAT. \n");
        exit(1);
    }

    fseek(fp,fatStart, SEEK_SET); // seek to the start of FAT placed in memory
    uint32_t *arrayFree= calloc(freed, sizeof(uint32_t));
    for (int i=0; i<=freed; i++){
        int read= fread(&chunck, 4,1,fp); //read 1 chunck at a time 
        if (read==0){
            printf("error ocurred \n");
            exit(1);
        }
        chunck=ntohl(chunck); 
        if (chunck==0x0000000){
            arrayFree[entry]= i; //store the location number where th entries are available
            entry++;
        }
    }
    return arrayFree;
}

/* 
    Helper to obtain the current time of creation.
*/
dir_entry_timedate_t obtainTime(){
    dir_entry_timedate_t timeInfo;
    time_t timeNow;
    time(&timeNow);
    struct tm* localTime= localtime(&timeNow);
    timeInfo.year = (uint16_t)localTime->tm_year + 1900; 
    timeInfo.month = (uint8_t)localTime->tm_mon + 1;    
    timeInfo.day = (uint8_t)localTime->tm_mday;
    timeInfo.hour = (uint8_t)localTime->tm_hour;
    timeInfo.minute =(uint8_t)localTime->tm_min;
    timeInfo.second = (uint8_t)localTime->tm_sec;

    return timeInfo;
}

//Copies the struct dir_entry_t filename attribute from the string provided, representing the name of the file 
void fillFilename(dir_entry_t *fileEntry,char *fileContents) {
    int length = strlen(fileContents);
    int copyLength = length < sizeof(fileEntry->filename) ? length : sizeof(fileEntry->filename) - 1; //check if it fits
    strncpy((char *)fileEntry->filename, fileContents, copyLength);    // Copy the content to the filename array
    fileEntry->filename[copyLength] = '\0';
}

/*
    takes the struct of the file we want to copy, the name we want to title the file as and 
    the array with available locations to fill. 
*/
dir_entry_t generateEntry (struct stat fileStat, char *filename, uint32_t *locationArray, int* numBlocks){
    dir_entry_t new;
    //status define
    if (S_ISREG(fileStat.st_mode)){
        new.status= 3;
    }else if (S_ISDIR(fileStat.st_mode)){
        new.status=5;
    }
    int remainder= fileStat.st_size%superBlock.block_size;
    * numBlocks=fileStat.st_size/superBlock.block_size;
    if (remainder!=0){
       (* numBlocks)++;
    }

    new.starting_block=(uint32_t)locationArray[0]; //starting block is next block available in the FAT
    new.block_count = (uint32_t)(*numBlocks);
    new.size=(uint32_t)fileStat.st_size;
    new.create_time= obtainTime();
    new.modify_time= obtainTime();
    fillFilename(&new,filename);
    return new;
}

/* 
    Writes into the FAT both the contents and the "links" between allocated blocks
*/
void writeToFAT(FILE *fp,char *content, dir_entry_t toAdd, uint32_t *addressOfBlocksToUse){
    int startAddress= superBlock.fat_start_block*superBlock.block_size;
    //int firstAddress= toAdd.starting_block*4;
    char *cont= content;

    //Write content into each block
    for (int i=0; i<toAdd.block_count;i++){ 
        int nextLocation= startAddress+ (addressOfBlocksToUse[i]*4);
        fseek(fp, nextLocation, SEEK_SET);
        fwrite(&cont,1, superBlock.block_size,fp);
        cont+=superBlock.block_size; 
    }

    //link fats by pointing them to next address
    for (int j=0; j<toAdd.block_count-1; j++){ //account for last entry
        int curLocation= addressOfBlocksToUse[j];
        int nextLocation= addressOfBlocksToUse[j+1];
        int offset= startAddress+curLocation*4;
        fseek(fp, offset, SEEK_SET); 
        nextLocation= htonl(nextLocation);
        fwrite(&nextLocation, sizeof(uint32_t),1, fp); //point to the next location adress
    }   

    //add FFFF as the end of the blocks
    int lastEntry= addressOfBlocksToUse[toAdd.block_count-1]; 
    int offsetLast= startAddress+lastEntry*4;
    fseek(fp, offsetLast, SEEK_SET);
    uint32_t lastFAT= 0xFFFFFFFF;
    fwrite(&lastFAT, sizeof(uint32_t), 1, fp);
}

/* 
    Writes the hex data into the rootDirectory section to add the file to it.
    Only works for the root directory.
*/
void writeToDir(int dirAvailable, dir_entry_t toAdd, char *name, FILE *fp){
    //File each individual line and then manually copy it into the rootDirectory
    unsigned char line1[NUMBYTES];
    unsigned char line2[NUMBYTES];
    unsigned char line3[NUMBYTES];
    unsigned char line4[NUMBYTES];

    //status
    memmove(&line1[0], &toAdd.status, sizeof(uint8_t));

    //starting block
    uint32_t start= htonl(toAdd.starting_block);
    memmove(&line1[1], &start, sizeof(uint32_t));
    memmove(&line1[2], ((unsigned char*) &start) + 1, sizeof(uint32_t) - 1); //where we move by 1 byte offset
    memmove(&line1[3], ((unsigned char*)&start) + 2, sizeof(uint32_t) - 2);
    memmove(&line1[4], ((unsigned char*)&start) + 3, sizeof(uint32_t) - 3);


    //block count
    uint32_t count = htonl(toAdd.block_count);
    memmove(&line1[5], &count, sizeof(uint32_t));
    memmove(&line1[6], ((unsigned char*) &count) + 1, sizeof(uint32_t) - 1); //where we move by 1 byte offset
    memmove(&line1[7], ((unsigned char*)&count) + 2, sizeof(uint32_t) - 2);
    memmove(&line1[8], ((unsigned char*)&count) + 3, sizeof(uint32_t) - 3);


    //size
    uint32_t sz = htonl(toAdd.size);
    memmove(&line1[9], &sz, sizeof(uint32_t));
    memmove(&line1[10], ((unsigned char*) &sz) + 1, sizeof(uint32_t) - 1); //where we move by 1 byte offset
    memmove(&line1[11], ((unsigned char*)&sz) + 2, sizeof(uint32_t) - 2);
    memmove(&line1[12], ((unsigned char*)&sz) + 3, sizeof(uint32_t) - 3);


    //create time
    dir_entry_timedate_t create= toAdd.create_time;
    uint16_t year= htons (create.year);
    memmove(&line1[13], &year, sizeof(uint16_t));
    memmove(&line1[14], ((uint8_t*) &year) + 1, sizeof(uint16_t)-1 );
    memmove(&line1[15], &create.month, sizeof(uint8_t));
    memmove(&line2[0], &create.day, sizeof(uint8_t));
    memmove(&line2[1], &create.hour, sizeof(uint8_t));
    memmove(&line2[2], &create.minute, sizeof(uint8_t));
    memmove(&line2[3], &create.second, sizeof(uint8_t));


    //modify time 
    dir_entry_timedate_t modify= toAdd.modify_time;
    year= htonl(modify.year);
    memmove(&line2[4], &year, sizeof(uint16_t));
    memmove(&line2[5], ((uint8_t*) &year) + 1, sizeof(uint8_t)); //where we move by 1 byte offset
    memmove(&line2[6], &modify.month, sizeof(uint8_t));
    memmove(&line2[7], &modify.day, sizeof(uint8_t));
    memmove(&line2[8], &modify.hour, sizeof(uint8_t));
    memmove(&line2[9], &modify.minute, sizeof(uint8_t));
    memmove(&line2[10], &modify.second, sizeof(uint8_t));

    //adding to line2
    int stringIndex=11;
    for (int i=0;i<=5;i++){ //end of line2 is 11+4= 15 as last index
        memmove(&line2[stringIndex], &toAdd.filename[i], sizeof(uint8_t));
        stringIndex++;
    }

    //write thorugh in line 3
    int fillIndex=5; //16+5=21
    for (int i=0; i<16; i++){ //traverse through all entries in line3
        memmove(&line3[i], &toAdd.filename[fillIndex], sizeof(uint8_t));
        fillIndex++;
    }
   
    //write through line 4 with fillIndex-> already at desired spot
    for (int i=0; i<11; i++){
        memmove(&line4[i], &toAdd.filename[fillIndex], sizeof(uint8_t));
        fillIndex++;
    }

    int offset= dirAvailable;
    fseek(fp, offset, SEEK_SET);
    fwrite(line1, sizeof(unsigned char), NUMBYTES, fp); //line 1 into fat
    offset+=16;
    fseek(fp, offset, SEEK_SET); //offset to line 2 start
    fwrite(line2, sizeof(unsigned char), NUMBYTES, fp); 
    offset+=16;
    fseek(fp, offset, SEEK_SET); //offset to line 3 start
    fwrite(line3, sizeof(unsigned char), NUMBYTES, fp);
    offset+=16;
    fseek(fp, offset, SEEK_SET); //offset to line 4 strt
    fwrite(line4, sizeof(unsigned char), NUMBYTES, fp);
}

/* Finds available space in the root directory where we can add next entry to. */
int findAvailableSpace(FILE *fp , int start){
    int offset=0;
    int sz= 64; //each dir is 16*4
    fseek(fp, start, SEEK_SET);
    for(int i=0; i< superBlock.root_dir_block_count;i++){
        fread(&offset, sizeof(int),1, fp);
        if (offset==0){ //found available space
            return i*sz+start;
        }
        fseek(fp, sz- sizeof(int), SEEK_CUR);
    }
    return -1; //not found :(
}


int main(int argc, char *argv[] ){
    char *fileName= argv[1];
    char *fileToCopyName=argv[2];
    char *fileNameWrite= argv[3];
    FILE *file= fopen(fileName,"r+b");
     if(fileName==NULL){ //if provided an argument to terminal 
        printf("No image to read from \n");
        exit(1);
    }
    int freeBlockCount=0, reserved=0, allocated=0;
    getBlockInfo(file, &freeBlockCount, &reserved, &allocated);  
    int rootDirStart= superBlock.root_dir_start_block*superBlock.block_size;
    struct stat fileStat;
    int numBlocksUsed;
    char *fileRetrievedContent = readDirContents(fileToCopyName, &fileStat); //gather content of the file we want to add
    long fileSize= fileStat.st_size;
    uint32_t *locationArray=freeBlocksAddress(file, freeBlockCount, fileSize); //gather an array with all the available locations we can write to 
    dir_entry_t newEntry=  generateEntry(fileStat, fileNameWrite, locationArray, &numBlocksUsed); //make a dir_entry_t struct for the file we want to add
    writeToFAT(file, fileRetrievedContent, newEntry, locationArray);  //write data into FAT 
    int available= findAvailableSpace(file, rootDirStart); //available space to add to   
    writeToDir(available, newEntry,fileNameWrite, file); //writes the entry to the root directory
    fclose(file);
}
