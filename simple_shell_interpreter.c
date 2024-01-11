#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>
#include <readline/readline.h>
#include <sys/wait.h>
#include <readline/history.h>
#include "linkedlist.h"
#define BUFF_LEN 256
#define PATH_MAX 4096
#define LENGTH 100

struct userInfo{
char name[BUFF_LEN];
char hostname[BUFF_LEN];
char cur_dir[PATH_MAX];
};

/* Makes a struct with all the information to be displayed when prompting user */
struct userInfo get_user_info(){
  struct userInfo usr;
   int login= getlogin_r(usr.name, BUFF_LEN);
   if (login!=0)
   {
    printf("Could not assign name \n");
    exit(1);
    }
   int host_result= gethostname(usr.hostname,BUFF_LEN);
   getcwd(usr.cur_dir, PATH_MAX);
   if (host_result!=0 || usr.cur_dir==NULL)
   {
    printf("host or cwd could not be resolved");
    exit(1);
    }  
   return usr;
}

/*Concatenates the user information obtained with the corresponding characters to emulate a Linux terminal prompt*/
char* shell_info(char *user, char* host,char *dir){
char* at= "@";
int at_len=1, null_terminator=1;
char dots[]=": ";
char arrow[]=" > ";
int length=strlen(user)+strlen(host)+strlen(dir)+at_len+strlen(dots)+strlen(arrow)+null_terminator; //for null terminated
char * info_str= calloc(length+3, sizeof(char)); //change to static memory

if (info_str==NULL)
{
  printf("Memory could not be allocated.\n");
  exit(1);
}

strncat(user,"@",at_len) ;
strncat(user,host,strlen(host));
strncat(user,dots,strlen(dots));
strncat(user,dir,strlen(dir));
strncat(user,arrow, strlen(arrow));
strncat(user,"\0",null_terminator);
strcpy(info_str,user);

return info_str;
}

/* Cuts the string entered into individual words and stores it in a 2D array */
char** process_command(char* reply, char **str_array, int cols){
  char *token = strtok(reply," ");
  for (int i=0;i<cols;i++){
    if (token!=NULL){
      strcpy(str_array[i],token);
      token= strtok(NULL," ");
    } 
  }
  str_array[cols-1]=NULL; 
  return str_array; 
}

/*Counts the amount of words in the string entered by the user. 
Note: this function returns the amount of words and the length of the longest
word found to main through pass by reference in the pointers*/
void count_tokens(char* input, int* cols, int* rows){ 
   int count_tokens=0, max_string_len=0;
   char copy[PATH_MAX];
   strcpy(copy,input);
   char *token = strtok(copy," "); 
   max_string_len= strlen(token);

  while (token!=NULL) 
  {
    count_tokens++;
    int token_len=strlen(token);
    if (token_len>max_string_len)
    {
      max_string_len= token_len;
    }
    token=strtok(NULL," ");
  }

  count_tokens++;
  max_string_len+=2; //include null terminator character
  *cols=count_tokens; //derefernce to point to pass by reference back to main 
  *rows=max_string_len;
}

/* Creates child function and invokes on excecvp to execute commands */
void basic_excec(char **instructions,int count){ 
  int pid= fork();
    if (pid==0)
      {
      execvp(instructions[0],instructions);
      } 
    if (pid<0)
      {
      perror("EXCECVP could not excecute");
      }
    
    int result= waitpid(pid,NULL,0);   
} 

/*Used to change the directory to the desired location*/
void change_directories(char **instructions, int count){ 
char *change_to;
 char new_dir[PATH_MAX]; 

 if (count==2){ 
    change_to=getenv("HOME");
  }else{ 
    change_to= instructions[1]; 
    if (strcmp(change_to, "~")==0) 
    {
      change_to=getenv("HOME"); 
    }
  }
  int change_status= chdir(change_to);

  if (change_status==-1){
    printf("change dir failed");
  } else{
    getcwd(new_dir, sizeof(new_dir));
  }
}

/*Function used to check upon the termination of background process 
Note: this function calls methods from linked list.c 
 */
int check_process(bg_pro **root){  
int stat;
pid_t ter= waitpid(0,&stat, WNOHANG); 
if (ter>0){
  if(WIFEXITED(stat)){ //if child correctly terminated 
    int num_elements= remove_by_pid(root,ter);
    return num_elements;
  } else if(WIFSIGNALED(stat)){ //if child termiated by a signal and forcefully terminated
    printf("child forcefully terminated \n");
    exit(1);
  }
  }
}
/* frees the array used to copy over and left shift bg*/
int free_ins(char **strings){
if (strings==NULL){
  return 1;
}
for (int i=0; strings[i]!=NULL; i++){
  free(strings[i]);
  }
  free(strings);
}

/* Gets rid of "bg" argument in order to excecute the commands after it */
char ** new_instructions(char **instructions, int count){
char **ins= calloc(LENGTH,sizeof(char *));
if (ins == NULL) {
  printf("malloc failed");
  exit(1);
}
for (int i = 1; instructions[i]!=NULL; i++) {
  ins[i-1]= strdup(instructions[i]); //left shift the args into a new array
  if (ins[i-1]==NULL){
    printf("cant allocate memory!!");
    exit(1);
  }    
  ins[count] = NULL;
  }
  return ins;
}

//free the new_instructions
/*Creates new child process to allow background execution.
Note: this function uses methods from linked list */
bg_pro * manage_background(char **commands,int count, bg_pro ** root, int root_removed){
char **instructions= new_instructions(commands,count);
count= count-1;
pid_t pid= fork();
if (pid==0){
  int exec_status= execvp(instructions[0], instructions);
  if (exec_status==-1){
    exit(1);
  }
} else if (pid>0){
  if (root_removed==1){ //if currently do not have a root, root_removed will be 1 
    *root = set_root(pid, instructions, count); 
    free_ins(instructions);
    return * root; 
    } else{
      *root =add_element(*root, pid, instructions, count);
      free_ins(instructions);
      return * root;
  }
}
}

/*In charge of dynamically allocate an array with given number of rows and columns*/
char ** allocate_2d_array(int cols,int rows)
{
char ** string_array;
string_array=calloc(cols,sizeof(char *));
  if(string_array==NULL){
  exit(1);   //if string_array is null exit with memory not allocated
  }
  for (int i=0;i<cols;i++){
    string_array[i]=(char *)calloc(rows,sizeof(char));
    if(string_array[i]==NULL){
      exit(1);
    }
  }
  return string_array;
}

/* Frees the array to avoid memory leaks */
void ** free_2d_array(char ** two_array, int cols)
{
  for(int i=0; i<cols;i++)
  {
    if (two_array[i]!=NULL){
     free(two_array[i]);
    }
  }
  if (two_array!=NULL){
    free(two_array);
  }
}

/* Main has all important function declarations and calls the most important helper functions*/
int main(){
    int control=0;
    int count_bg_process= 0;
    int root_removed=0;
     bg_pro * root=NULL;
     while (!control){
      char ** array_strings;
      char reply_copy[PATH_MAX];
      int  word_count, max_word_len, command_num;
      char * info_str=NULL;
      struct userInfo usr= get_user_info(); 
      if (count_bg_process>0){
        root_removed=check_process(&root);
      } 
      info_str=shell_info(usr.name, usr.hostname,usr.cur_dir);
      char *reply= readline(info_str); 
      if (strlen(reply)==0){
        free(reply);
        free(info_str);
        continue; //go to next loop iteration if user does not enter anything.
      }
      if (!strcmp(reply,"q")){
       control=1; 
      }
      strcpy(reply_copy,reply);
      count_tokens(reply, &word_count, &max_word_len);
      array_strings=allocate_2d_array(word_count,max_word_len);
      array_strings=process_command(reply_copy,array_strings,word_count);
     
      if (strcmp(array_strings[0],"cd")==0){
        change_directories(array_strings,word_count);
      } else if (strcmp(array_strings[0],"ls")==0){
        basic_excec(array_strings,word_count);
      } else if(strcmp(array_strings[0],"bg")==0) { 
        if(count_bg_process==0){
            root_removed=1;
            count_bg_process++;
          }
          root= manage_background(array_strings, word_count,&root, root_removed);    
      } else if (strcmp(array_strings[0],"bglist")==0){
        print_list(&root);
      }
      free(reply);
      free(info_str);
      free_2d_array(array_strings, word_count);    
     }
}
