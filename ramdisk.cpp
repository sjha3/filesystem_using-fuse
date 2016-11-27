#define FUSE_USE_VERSION 29

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

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
#include <libgen.h>
#include <unistd.h>
#include <sys/types.h>
#include <string>

struct node{
    char *file_name;
    char file_type;
    int num_child;
    char *buffer;
    int size;
    struct node* child[1024];
    struct node* parent;
};

struct node *root;
char *root_path = "/";
int root_size;
static int cnt = 0;
std::map<const char*, struct node* > map_node;

//for /etc/abc/cde , this function returns "cde"
char *getFileName(const char* path){
    printf("inside getFileName() with path as %s\n\n",path);
    char *fpath = (char*)malloc(1024*sizeof(char));
    char s[2]="/";
    strcpy(fpath,path);
    char *token=strtok(fpath,s);
    char *file = (char*)malloc(256);
    
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
//changed to /etc/abc/cde => /abc/cde
char *getDirName(const char *path){
    printf("inside getDirName() with path as %s\n\n",path);
    char *dirc, *dname;
    dirc = strdup(path);
    dname = dirname(dirc);
    printf("dirname of %s = %s\n", path, dname);
    //return getFileName(dname);
    return dname;
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
    printf("**** getParNode() witb path as %s***\n\n",path);
    char *fpath =(char*)malloc(1024*sizeof(char));
    strcpy(fpath,getDirName(path));
    printf("parent dir for %s is %s\n",path,fpath);
    
    if(strcmp(fpath,root_path)==0)
        return root;
        
    struct node* n = map_node[fpath];
    return n;
}

void print_map(){
    printf("*** inside print_map() ***\n\n");
    std::map<const char*, struct node*>::iterator it;
    for(it=map_node.begin();it!=map_node.end();it++){
        printf("%s => %d \n",it->first,it->second);
    }
    return;
}

static int l_read( const char *path, char *buffer, size_t size, off_t offset, struct fuse_file_info *fi ){
    printf("*** l_read() with path as %s***\n\n",path);
    //char *file = getFileName(path);
    struct node *n;
    if(map_node.find(path)==map_node.end())
        return ENOENT;
    else
        n = map_node[path];
    if(size > n->size)
        size = n->size;
    if (size+offset > n->size)
        size = n->size-offset;
    char *buf = n->buffer;
    memcpy( buffer, buf + offset, size );  
    return size;      
}
/*
static int my_write(const char *path, const char *buf, size_t size,
		     off_t offset, struct fuse_file_info *fi){
                 
}
*/
void getattr( const char *path, struct stat *st)
{
	printf("*** getattr() map_node size is %d path is %s***\n\n",map_node.size(), path);
	printf( "\tAttributes of %s requested\n\n", path );
    print_map();
    struct fuse_context* fc = fuse_get_context();
    char *file = getFileName(path);
    memset(st, 0, sizeof(struct stat));
    struct node* n;
    if(map_node.find(path)==map_node.end()){
        printf("*** node not found ***\n");
        return ;
    }
    else
        n = map_node[path];
    printf("found node in map_node\n");
	st-> st_uid = fc->uid;
	st-> st_gid = fc->gid;
	st-> st_blksize = 512;
	st-> st_blocks = 1;
	st-> st_atime = st-> st_ctime = st-> st_mtime = time(NULL);
	
	if (n->file_type =='d')
	{
        printf("type => directory\n");
		st->st_mode = S_IFDIR | 0755;
		st->st_nlink = 2; // Why "two" hardlinks instead of "one"? The answer is here: http://unix.stackexchange.com/a/101536
	}
	else
	{
        printf("type => file\n");
		st->st_mode = S_IFREG | 0644;
		st->st_nlink = 1;
		st->st_size = 1024;
	}
	printf("*** return from getattr()***\n");
    return;
}

static int l_getattr(const char *path, struct stat *stbuf){
    printf("*** l_getattr() with path as %s***\n\n",path);
    char *file = getFileName(path);
    
    memset(stbuf, 0, sizeof(struct stat));
 /*   if(cnt==0){
    stbuf->st_mode = S_IFDIR | 0755;
	stbuf->st_nlink = 2;
    stbuf-> st_atime = stbuf-> st_ctime = stbuf-> st_mtime = time(NULL);
    cnt++;
    }
    else
 */   getattr(file,stbuf);
    printf("*** return from l_getattr()***\n");    
    return 0;
}

static int l_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
            off_t offset, struct fuse_file_info *fi)
{
    printf("*** l_readdir() with path as %s***\n\n",path); 
    (void) offset;
    (void) fi;
    int i;
    static node *n ,*n_child;
    struct stat *st;
    //char *file = getFileName(path);

    printf("before .\n");
    filler(buf, ".", NULL, 0);//FUSE_FILL_DIR_PLUS);
    printf("before ..\n");
    filler(buf, "..", NULL, 0);

    if(map_node.find(path)==map_node.end()){
        printf("node not found\n");
        return ENOENT;
    }    
    else
        n = map_node[path];
        
    printf("node found in l_readdir()\n\n");
    //filler(buf, "file54", NULL, 0, (fuse_fill_dir_flags)0 );
    //filler(buf, "file349", NULL, 0, (fuse_fill_dir_flags)0 );
    printf("num child : %d\n",n->num_child);
   for(i=0;i<n->num_child;i++){
       printf("child : %s\n",n->child[i]->file_name);
        n_child = n->child[i];
        memset(st, 0, sizeof(struct stat));
        getattr(n_child->file_name,st);
        filler(buf, n_child->file_name, st, 0);
    }
    printf("return from l_readdir()\n"); 
    return 0;
}
/*

static int my_readdir(const char *path, void *buf, fuse_fill_dir_t filler,

			 off_t offset, struct fuse_file_info *fi)

{

	

	(void) offset;

	(void) fi;	

	

	printf("Inside readdir path=%s \n",path);	

	filler(buf, ".", NULL, 0);

	filler(buf, "..", NULL, 0);
	struct stat stbuf;
	printf("Exiting readdir \n");

	return 0;

}

*/
static int l_unlink(const char *path){
    printf("*** l_unlink() with path as %s***\n\n",path);
    //char *file = getFileName(path);
    int i,n_c;
    static node *n ,*n_par;
    if(map_node.find(path)==map_node.end())
        return ENOENT;
    else
        n = map_node[path];  
    n_par = n->parent;
    n_c = n_par->num_child;
    
    for(i=0;i<n_c;i++){
        if(n==n_par->child[i]){
            n_par->child[i]=n_par->child[i+1];
        }
    }
    free(n);
    n_par->num_child--;
    return 1;
}

static int l_opendir(const char *path, struct fuse_file_info *fi){
    return 0;
}

static int l_rmdir(const char *path){
    printf("*** l_rmdir() with path as %s***\n\n",path);
    //char *file = getFileName(path);
    static node *n;
    if(map_node.find(path)==map_node.end())
        return ENOENT;
    else
        n = map_node[path];
     
    if(n->num_child==0)
        return -1;

    return l_unlink(path);    
}

static int l_mknod(const char *path, mode_t mode, dev_t rdev){
    printf("*** l_mknod() with path as %s***\n\n",path);
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
    map_node[path] = n1;  //mapping of complete path name to it's node;
    n1_par->child[n1_par->num_child++] = n1;
    root_size -= 1024;
    
    return 0;    
}

static int l_mkdir(const char *path, mode_t mode){
    printf("*** l_mkdir() with path as %s***\n\n",path);
    if(root->size<128)
        return -1;
        
    struct node *n1 = (struct node*)malloc(sizeof(struct node));
    char *dir = getFileName(path); //gets directory name from path
    struct node* n1_par = getParNode(path);
    n1->file_name = dir; //may have to use strcpy()
    n1->file_type = 'd';
    n1->num_child = 0;
    n1->parent = n1_par;
    n1->size = 128;   //put a check here to see if 128 is still available with root
    n1->buffer = NULL;
    map_node[path] = n1;
    n1_par->child[n1_par->num_child++] = n1;
    root_size -= 128;
    
    return 0;
}

void make_root_node(char *root_path, int rt_size){
    printf("*** make_root_node() with size as %d and path as %s***\n\n",rt_size,root_path);
    root = (struct node*)malloc(sizeof(struct node));
    root->file_name = "/";
    root->file_type = 'd';
    root->num_child = 0;
    root->parent = NULL;
    root->size = rt_size;
    root->buffer = NULL; 
    map_node[root->file_name] = root;
    return;   
}

fuse_operations l_rmdk;

void printNode(struct node* node){
    printf("file name : %s\n",node->file_name);
    printf("file type : %c\n",node->file_type);
    printf("file size : %d\n",node->size);
    printf("num child : %d\n",node->num_child);
    //printf("value in map node: %d\n",map_node[node->file_name]->file_name);
    return;
}

int main(int argc, char *argv[])
{
    root_path = argv[1];    
    root_size = atoi(argv[2]);//*1024*1024; //MB to B conversion
    make_root_node(root_path,root_size*1024*1024);
    if (root==NULL)
        printf("root is NULL");
    else    
        printf("root node file : %s and size %d\n",root->file_name, root_size);
    l_rmdk.mkdir = l_mkdir;
    l_rmdk.rmdir = l_rmdir;
    l_rmdk.readdir = l_readdir;
    l_rmdk.read = l_read;
    l_rmdk.unlink = l_unlink;
    l_rmdk.mknod = l_mknod;
    l_rmdk.getattr = l_getattr;
    l_rmdk.opendir = l_opendir;
    //strcpy(root_path ,"/");
    printf( "file is %s size %d  argc : %d\n", root_path, root_size, argc);//, map_node[root_path]->file_name,argc);
    printf("print root:\n");
    printNode(root);
    printf("******check********\n");
    return fuse_main(argc-1, argv, &l_rmdk, NULL);
}

