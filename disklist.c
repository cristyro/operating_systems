#include <stdlib.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include  <ctype.h> 
#include <stdbool.h>
#include "filesys.h"

superblock_t superBlock;
/* Function declarations */
dir_entry_t createEntry(unsigned char line1[NUMBYTES], unsigned char line2[NUMBYTES], unsigned char line3[NUMBYTES], unsigned char line4[NUMBYTES]);
dir_entry_timedate_t createTimeEntry(unsigned char line1[NUMBYTES], unsigned char line2[NUMBYTES]);
dir_entry_timedate_t modifyTimeEntry(unsigned char line2[NUMBYTES]);

/* Main helpers */


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

/*
*   Creates a dir_entry_t struct with all the attributes for the root directory or any other directory
*   Returns: the dir_entry_t struct for the directory 
*/
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

/* Reads from the hexdump file and calls a helper that assigns a struct for the each of the entries of the directory we are looking for  
*   Returns the array containing a dir_entry_t per item found on the root or given directory
*/
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
/* Prints the information about the directory found.
*   Returns: none
*/
void printDirInfo(dir_entry_t *dirEntry, int itemCount){
    for (int i=0; i<itemCount; i++){
        dir_entry_t entry= dirEntry[i];
        dir_entry_timedate_t entryTime= entry.create_time;
        char letter= entry.status==3? 'F':'D'; //asign F is status is 3, directory if its 5
        printf("%c %10u %30s ", letter, entry.size, entry.filename);
        printf(" %4d/%02d/%02d %02d:%02d:%02d",
            entryTime.year,
            entryTime.month,
            entryTime.day,
            entryTime.hour,
            entryTime.minute,
            entryTime.second);
        printf("\n");
    } 
}

/*Secondary helpers */

/*
*    Assigns create time and returns dir_entry_timedate_t struct
*/
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


/* Asssigns modify time */
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


/* Removes the slash '/' to deal with the text only. Returns none*/
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

/* Returns 1 if the content of 2 strings are the same */
int compareStrings (const char* str1, const char* str2) {
    while (*str1 && *str2) {
        if (!isspace(*str1) && !isspace(*str2)) {
            if (tolower(*str1) != tolower(*str2)) {
                return false;  // Different characters found
            }
        } else if (!isspace(*str1) || !isspace(*str2)) {
            return false;  
        }
        str1++;
        str2++;
    }
    // Check if both strings finished
    return *str1 == '\0' && *str2 == '\0';
}


/* Counts and returns number of words in a string */
int countWords(const char *str) {
    int wordCount = 0;
    bool inWord = false;
    while (*str) {
        if (*str == ' ' || *str == '\t' || *str == '\n') {
            inWord = false; 
        } else if (!inWord) {
            inWord = true;  // Start of a new word
            wordCount++;
        }
        str++;
    }
    return wordCount;
}

char*  pathToSubdir(const char* str1, const char* str2) {
    char* result = (char*)malloc(strlen(str1) + strlen(str2) + 2);  // +2 for the space and null terminator
    strcpy(result, str1);
    strcat(result, " ");
    strcat(result, str2);
    return result;
}

/*
 * This function searches for a subdirectory specified by the given path within the directory. It takes account case-insensitive matching and handles nested directories.
 * If match is found at location, the dir_entry_t and pass by reference of found with 1 is returned
 * Otherwise, we exit the program
 * */
dir_entry_t testSubdir(FILE* file, char *subdirName, dir_entry_t *dirEntry, int itemCount, int *found){
    removeSlash(subdirName);
    int nestedDirCount= countWords(subdirName);
      for (int i=0; i<itemCount; i++){
        dir_entry_t entry= dirEntry[i];
        if (entry.status==5){ //directory 
            for(int i=0; i<nestedDirCount; i++){
                int nestedDirStart = entry.starting_block*superBlock.block_size;
                dir_entry_t* nested= getDirInfo(file, &itemCount,nestedDirStart);

                for (int j=0; j<itemCount; j++){    
                    dir_entry_t subdirEntry= nested[j];
                    if (subdirEntry.status==5){ 
                        char *subdirString= pathToSubdir((const char *)entry.filename, (const char *)subdirEntry.filename);
                        int diff= compareStrings(subdirString, subdirName);
                        if (diff){
                        (*found)++;
                        (*found)++;
                        return subdirEntry;
                        }
                    }
                }
                if(strcasecmp((const char *)entry.filename,subdirName)==0){ //match case insensitive
                    (*found)++;
                    (*found)++;
                    return entry;
                    }
            }
        }
    }
    dir_entry_t notFound;
    printf("File not found. \n");
    exit(1);
    return notFound;
}



/* Error handling for argc */
int main(int argc, char *argv[] ){
    char *fileName= argv[1];
    char *directorySearch=argv[2];

    if(fileName==NULL){ //if provided an argument to terminal 
        printf("Could not open it \n");
        exit(1);
    }

    FILE *file= fopen(fileName, "rb");
    if (file==NULL){
        printf("Could not open file.\n");
        exit(1);
    }
    int freed=0, reserved=0, allocated=0, itemCount=0;
    getBlockInfo(fileName, &freed, &reserved, &allocated);

    int dirStart= superBlock.root_dir_start_block*superBlock.block_size;
    dir_entry_t *dir =getDirInfo(file, &itemCount, dirStart);

    if (directorySearch!=NULL && strcmp(directorySearch,"/")){ //if it is not / 
        int found=-1;
        dir_entry_t subdir= testSubdir(file,directorySearch, dir, itemCount, &found);
        if (found){
            int subStart= subdir.starting_block*superBlock.block_size;
            int count=0;
            dir_entry_t *sub= getDirInfo(file, &count, subStart);
            printDirInfo(sub, count);
        }
    }else{
        //If no subdirectory was given, print contents in root directory
        printDirInfo(dir, itemCount);
    }
    free(dir);
    fclose(file);
}
