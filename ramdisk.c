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
char *root_path;
int root_size;
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

static int l_read( const char *path, char *buffer, size_t size, off_t offset, struct fuse_file_info *fi ){
    char *file = getFileName(path);
    struct node *n;
    if(map_node.find(file)==map_node.end())
        return -1;
    else
        n = map_node[file];
    if(size > n->size)
        size = n->size;
    if (size+offset > n->size)
        size = n->size-offset;
    char *buf = n->buffer;
    memcpy( buffer, buf + offset, size );  
    return size;      
}

static int getattr( const char *path, struct stat *st )
{
	printf( "[getattr] Called\n" );
	printf( "\tAttributes of %s requested\n", path );
    char *file = getFileName(path);
    struct node* n;
    if(map_node.find(file)==map_node.end())
        return -1;
    else
        n = map_node[file];

    st->st_uid = getuid(); // The owner of the file/directory is the user who mounted the filesystem
	st->st_gid = getgid(); // The group of the file/directory is the same as the group of the user who mounted the filesystem
	st->st_atime = time( NULL ); // The last "a"ccess of the file/directory is right now
	st->st_mtime = time( NULL ); // The last "m"odification of the file/directory is right now
	
	if (n->type =='d')
	{
		st->st_mode = S_IFDIR | 0755;
		st->st_nlink = 2; // Why "two" hardlinks instead of "one"? The answer is here: http://unix.stackexchange.com/a/101536
	}
	else
	{
		st->st_mode = S_IFREG | 0644;
		st->st_nlink = 1;
		st->st_size = 1024;
	}
	
	return 0;
}

static int l_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
			 off_t offset, struct fuse_file_info *fi,
			 enum fuse_readdir_flags flags)
{
    (void) offset;
    (void) fi;
    (void) flags;
    int i,n_c;
    static node *n ,*n_par,*n_child;
    struct stat *st;
    char *file = getFileName(path);
    
    if(map_node.find(file)==map_node.end())
        return -1;
    else
        n = map_node[file];
    
    filler(buf, ".", NULL, 0, 0);
    filler(buf, "..", NULL, 0, 0);
    for(int i=0;i<n->num_child;i++){
        n_child = n->child[i];
        memset(st, 0, sizeof(struct stat));
        getattr(n_child,st);
        filler(buf,n_child->file_name,st,0);
    }
    
    return 0;
}

static int l_unlink(const char *path){
    char *file = getFileName(path);
    int i,n_c;
    static node *n ,*n_par;
    if(map_node.find(file)==map_node.end())
        return -1;
    else
        n = map_node[file];  
    n_par = n->parent;
    n_c = n_par->num_child;
    
    for(i=0;i<n_c;i++){
        if(n1==n_par->child[i]){
            n_par->child[i]=n_par->chile[i+1];
        }
    }
    free(n1);
    n_par->num_child--;
    return 1;
}

static int l_rmdir(const char *path){
    char *file = getFileName(path);
    static node *n1;
    if(map_node.find(file)==map_node.end())
        return -1;
    else
        n = map_node[file];
     
    if(n1->num_child==0)
        return -1;

    return l_unlink(path);    
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
    root_path = argv[1];
    root_size = atoi(argv[2])*1024*1024; //MB to B conversion
    make_root_node(root_path,root_size*1024*1024);
    return fuse_main(args.argc, args.argv, &hello_oper, NULL);
}