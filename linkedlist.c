#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include "linkedlist.h"
#define COMMAND_LEN 1024
#define NUM_COMMANDS 80

/*Sets the root of the link list*/
bg_pro *set_root(__pid_t id, char ** instructions, int instructions_nums){
    struct bg_pro *root = (bg_pro*) malloc(sizeof(bg_pro));
    if (root==NULL){
        printf("memory allocation failed");
        exit (1);
    }
    root->pid= id;
    root->element=1;
    root->command_nums= instructions_nums;
    root->command= (char **)calloc(instructions_nums+4,sizeof(char *));
     if (root->command==NULL){
        printf("memory allocation failed");
        exit (1);
    }

    for (int i=0; i<instructions_nums-1; i++){
        root-> command[i]= (char *)calloc(strlen(instructions[i])+1, sizeof(char));
        if (root-> command[i]==NULL){
            printf("memory allocation failed");
            exit (1);
        }
        root->command[i]= strdup(instructions[i]);        
    }
    root->next=NULL;
    return root;
}

/*Adds element in the back of the linked list. If there are no elements found,
we add a the new element as a root. */
bg_pro *add_element(bg_pro * root, __pid_t id, char ** instructions, int instructions_nums){
    struct bg_pro *node = (bg_pro*) malloc(sizeof(bg_pro));
    if (node==NULL){
        printf("memory was not allocated \n");
        exit(1);
    }
    node->pid= id;
    node->command_nums= instructions_nums;
    node->command= (char **)calloc(instructions_nums,sizeof(char *));
    node->next= NULL;
    for (int i=0; i< instructions_nums-1; i++){
        node->command[i]= strdup(instructions[i]);
        if (node-> command[i]==NULL){
                printf("memory allocation failed");
                exit (1);
        }          
    }
    if (root == NULL) {
       root = node;
       node->element = 1;
       return root; // Return the new root
    }
 
     bg_pro * cur= root;
        while (cur->next!=NULL){
            cur= cur->next;
        }

    cur->next=node; 
    cur-> next-> next=NULL; 
    node->element= cur->element+1;

    return root;
}

/* Frees dynamically allocated memory of a given node */
void free_node(bg_pro * cur){
    int malloc_strings= cur->command_nums;
    for (int i=0; i<malloc_strings-1; i++){
        free(cur->command[i]);
    }
    free(cur->command);
    free(cur);
}


void remove_root(bg_pro ** root){
    bg_pro * second =NULL;
    if (*root == NULL){
        printf("nothing to be removed");
        exit(1);
    }
    bg_pro * cur= *root;
    *root= (*root)->next; 
    printf("%d: ", cur->pid);
    for (int i=0; i<cur->command_nums; i++){
        if (cur->command[i]!=NULL){
             printf("%s ",cur->command[i]);
        }
    } printf("has terminated. \n");   
    free_node(cur);
}

/*Searches through the link list untill we find the node with pid to_find and removes it.*/
int  remove_by_pid(bg_pro **root, __pid_t to_find){
    if (root!=NULL){ 
        if ((*root)->pid==to_find){
            remove_root(root);
            return 1;
        } else {
        bg_pro *cur= * root;
        bg_pro *prev= NULL;
        while(cur!=NULL && cur->pid!=to_find){ //iterate untill we find the node 
            prev= cur;
            cur= cur->next;
        }
        if (cur==NULL){
            return -1;
        }

        printf("%d: ", cur->pid);
        for (int i=0; i<cur->command_nums; i++){ //had 1 here
             if (cur->command[i]!=NULL){
                printf("%s ",cur->command[i]);
             }
        } printf("has terminated. \n");

        cur->element= cur->element-1;
        prev->next= cur->next; //update pointers between prev and next
        int element_num= cur->element;
        free_node(cur);

        return element_num;
        }
    }
}

void print_list(bg_pro **root){
    bg_pro *cur = *root;
    int count=0;
    while (cur != NULL) {
        printf("%d: ", cur->pid);
        for (int i=0;i<cur->command_nums; i++) { 
            if (cur->command[i]!=NULL){
                printf("%s ", cur->command[i]);      
            }
        }
        printf("\n");
        cur = cur->next;
        count++;
    }
    printf("Total background jobs: %d\n",count);


}

