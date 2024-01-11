//add to makefile to link to p1 main
typedef struct bg_pro{
    __pid_t pid;
    char **command;
    int command_nums;
    int element;
    struct bg_pro * next;
} bg_pro;

bg_pro *set_root(__pid_t id, char ** instructions, int instructions_nums);
bg_pro *add_element(bg_pro * root,__pid_t id, char ** instructions, int instructions_nums);
void remove_root(bg_pro ** root);
int remove_by_pid(bg_pro **root, __pid_t pid);
void print_list(bg_pro **root);
void free_node(bg_pro * cur);
