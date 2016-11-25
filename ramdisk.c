#define FUSE_USE_VERSION 30

#include <config.h>

#include <fuse.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <stddef.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <map>
static struct options {
	const char *filename;
	const char *contents;
	int show_help;
} options;

struct node{
    char *file_name;
    char file_type;
    int num_child;
    char *buffer;
    int size;
    struct node* child[1024];
    struct node* parent;
}

struct node *root;
std::map<char *file_name, struct node* file_node> map_node;

//for /etc/abc/cde , this function returns "cde"
char *getFileName(const char* path){
    char *fpath=malloc(1024*sizeof(char));
    char s[2]="/";
    strcpy(fpath,path);
    char *token=strtok(fpath,s);
    char *file = malloc(256);
    
    while( token != NULL ) 
    {
      //printf( " %s\n", token );
      strcpy(file,token);
      token = strtok(NULL, s);
    }
   printf( "file is %s\n", file);
    
   return file;        
}
//for /etc/abc/cde , this function returns "abc", which is parent directory of cde.
char *getDirName(const char *path){
    char *dirc, *dname;
    dirc = strdup(path);
    dname = dirname(dirc);
    printf("dirname=%s\n", dname);
    return getFileName(dname);
    /*
    const char s[2] = "/";
    char* token = strtok(dname, s);
    char* last_dir = malloc(256);
    
    while( token != NULL ){ 
       strcpy(last_dir,token);
       token = strtok(NULL, s);
    }  
    printf("last directory is %s\n",last_dir);
    return last_dir;
   */
}

struct node* getParNode(const char *path){
    char *fpath=malloc(1024*sizeof(char));
    strcpy(fpath,getDirName(path));
    printf("parent dir for %s is %s\n",path,fpath);
    return map_node[fpath];
}

void print_map(){
    std::map<char *file_name, struct node* file_node>::iterator it;
    for(it=map_node.begin();it!=map_node.end();it++){
        printf("%s => %s \n",it->first,it->second.file_name);
    }
    return;
}

static int l_mknod(const char *path, mode_t mode, dev_t rdev){
    
    if(root->size<1024)
        return -1;
    
    struct node *n1 = (struct node*)malloc(sizeof(struct node));
    char *file = getFileName(path); //gets filename from path
    struct node* n1_par = getParNode(path);
    n1->file_name = file;
    n1->file_type = 'f';
    n1->num_child = 0;
    n1->parent = n1_par;
    n1->size = 1024;//put a check here to see if 1024 is still available with root
    n1->buffer = NULL;
    map_node[file] = n1;  //mapping of file name to it's node;
    n1_par->child[n1_par.num_child++] = n1;
    root->size -= 1024;
    
    return 0;    
}

static int l_mkdir(const char *path, mode_t mode){
    
    if(root->size<128)
        return -1;
        
    struct node *n1 = (struct node*)malloc(sizeof(struct node));
    char *dir = getName(path); //gets directory name from path
    struct node* n1_par = getParNode(path);
    n1->file_name = dir; //may have to use strcpy()
    n1->file_type = 'd';
    n1->num_child = 0;
    n1->parent = n1_par;
    n1->size = 128;   //put a check here to see if 128 is still available with root
    n1->buffer = NULL;
    map_node[dir] = n1;
    n1_par->child[n1_par.num_child++] = n1;
    root->size -= 128;
    
    return 0;
}

void make_root_node(char *root_path, int root_size){
    root = (struct node*)malloc(sizeof(struct node));
    root->file_name = root_path;
    root->file_type = 'd';
    root->num_child = 0;
    root->parent = NULL;
    root->size = root_size;
    root->buffer = NULL;    
}

static struct fuse_operations my_rmdk = {
    .mkdir    = l_mkdir,
    .opendir  = l_opendir,
    .rmdir    = l_rmdir,    
    .getattr  = l_getattr,
    .readdir  = l_readdir,
    .open     = l_open,
    .read     = l_read,
    .unlink   = l_unlink,
    .mknod    = l_mknod,
    .write    = l_write,
    .truncate = l_truncate,
};


int main(int argc, char *argv[])
{
    char *root_path = argv[1];
    int root_size = atoi(argv[2]);
    make_root_node(root_path,root_size*1024*1024);
    return fuse_main(args.argc, args.argv, &hello_oper, NULL);
}