#define FUSE_USE_VERSION 29

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include<iostream>
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
using namespace std;
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
std::map<std::string , struct node* > map_node1;

void print_child(struct node *n){
   int i;
   printf("print child() for file: %s\n",n->file_name);
   for(i=0;i<n->num_child;i++){
   cout<<"file : "<<n->child[i]->file_name<<endl;
  }
return; 
}

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
//changed to /etc/abc/cde => /abc/cde
char *getDirName(const char *path){
    printf("inside getDirName() with path as %s\n\n",path);
    char *dirc, *dname;
    dirc = strdup(path);
    dname = dirname(dirc);
    printf("dirname of %s = %s\n", path, dname);
    //return getFileName(dname);
    return dname;
}

struct node* getParNode(const char *path){
    printf("**** getParNode() witb path as %s and root path is %s***\n\n",path,root_path);
    char *fpath =(char*)malloc(1024*sizeof(char));
    strcpy(fpath,getDirName(path));
    printf("parent dir for %s is %s\n",path,fpath);
    
    if(strcmp(fpath,root_path)==0){
	printf("***********root is parent\n");
        return root;
        }
    struct node* n = map_node1[fpath];
    return n;
}


struct node* find_node(std::string ss){
    //printf("*** inside find_node() with path as %s***\n\n",path);
    cout<<"*** inside find_node() with search for path "<< ss <<endl;
    std::map<std::string, struct node*>::iterator it;
    for(it=map_node1.begin();it!=map_node1.end();it++){
        //printf("%s => %d \n",it->first,it->second);
	//cout<< it->first << " = >" <<it->second<<endl;
	//if(strcmp(path,it->first)==0){
        if(ss == it->first){
		printf("node found  inside map yyyyyyyyyyyaaaaaaaaaaaaaaaaaaahooooooooooooooooooooooo\n");
		return it->second;
	}
    }
    return NULL;
}


void print_map1(){
    printf("*** inside print_map1() with size as %d\n\n",map_node1.size());
    std::map<std::string, struct node*>::iterator it;
    for(it=map_node1.begin();it!=map_node1.end();it++){
	cout<< it->first << " = >" <<it->second<<endl;
    }
    return ;
}

void print_map(){
    printf("*** inside print_map() with size as %d\n\n",map_node.size());
    std::map<const char*, struct node*>::iterator it;
    for(it=map_node.begin();it!=map_node.end();it++){
        printf("%s => %d \n",it->first,it->second);
    }
    return ;
}

static int l_truncate(const char *path, off_t size){

  return 0;

}

static int l_write(const char *path, const char *buf, size_t size,
		     off_t offset, struct fuse_file_info *fi)
{
    std::string ss(path);
    int i=0, j =offset;
    printf("******** l_write() with size %d and offset %d and content as %s\n",size,offset,buf);
    struct node* n  = find_node(ss);
    if(n == NULL)
	return -ENOENT;

    if(n->size < offset+size)
	n->size += offset + size; 
   printf("******new n->size = %d\n",n->size);

    char *content =(char*) malloc(n->size);
    if(offset>0)
    memcpy(content,n->buffer,offset);
 
    while(i<size){
       //printf("buf %d : %c and j : %d\n",i,buf[i],j);
        content[j] = buf[i];
        if(buf[i]=='\0')
          break;
        i++;
        j++;
    }

   printf("////////////  content in file : %s and length : %d********** \n", content,i);
   n->buffer = content;
   printf("*** return from l_write()after writing %s in node *************\n",n->buffer);
   return i; 
    
}
static int l_read( const char *path, char *buffer, size_t size, off_t offset, struct fuse_file_info *fi ){
    printf("*** l_read() with path as %s***\n\n",path);
    //char *file = getFileName(path);
    struct node *n = find_node(path);
    printf("******node is not null and has file as %s and size: %d\n",n->file_name,n->size);
    if(n==NULL)
        return -ENOENT;
    if(size > n->size)
        size = n->size;
    if(offset > strlen(n->buffer))
	return -1;
    if (size+offset > n->size)
        size = strlen(n->buffer)-offset;
    printf("size is %d\n",size);
    char *buf = n->buffer;
    memcpy( buffer, buf + offset, size );
    buffer[n->size+offset+size]='\0'; 
    printf("************ read buf is %s**********\n",buffer);
    printf("**********return from l_read()************\n"); 
    return size;      
}
/*
static int my_write(const char *path, const char *buf, size_t size,
		     off_t offset, struct fuse_file_info *fi){
                 
}
*/
int getattr( const char *path, struct stat *st)
{
	printf("*** getattr() map_node size is %d path is %s***\n\n",map_node.size(), path);
	printf( "\tAttributes of %s requested\n\n", path );
    //print_map();
    print_map1();
    std::string ss(path);
    struct node* n  = find_node(ss);
    //struct node* n  = find_node(path);
    struct fuse_context* fc = fuse_get_context();
    //char *file = getFileName(path);
    memset(st, 0, sizeof(struct stat));
    //if(map_node.find(path)==map_node.end()){
     if(n==NULL){
        printf("*** node not found ..return from getatt()***\n");
        return 0;
    }
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
    return 1;
}

static int l_getattr(const char *path, struct stat *stbuf){
    printf("*** l_getattr() with path as %s***\n\n",path);
    //char *file = getFileName(path);
    int k=1;    
    memset(stbuf, 0, sizeof(struct stat));
 /*   if(cnt==0){
    stbuf->st_mode = S_IFDIR | 0755;
	stbuf->st_nlink = 2;
    stbuf-> st_atime = stbuf-> st_ctime = stbuf-> st_mtime = time(NULL);
    //cnt++;
    //}
*/	if (strcmp(path, "/") == 0) {
		stbuf->st_mode = S_IFDIR | 0755;
		stbuf->st_nlink = 2;
	}
    else
         k = getattr(path,stbuf);
    printf("return value from getattr() : %d\n",k);
    if(!k)
    	return -ENOENT;
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
    struct node *n ,*n_child;
    struct stat *st = (struct stat*) malloc(sizeof(struct stat));
    //char *file = getFileName(path);

    printf("before .\n");
    filler(buf, ".", NULL, 0);//FUSE_FILL_DIR_PLUS);
    printf("before ..\n");
    filler(buf, "..", NULL, 0);
    //print_map();
    print_map1();
    std::string ss(path);
    n = find_node(ss);
    //n = find_node(path);
    //if(map_node.find(path)==map_node.end() && k==0){
    if(n==NULL){ 
       printf("node not found\n");
        return -ENOENT;
    }    
    else{
        //n = map_node[path];
      printf("node found in l_readdir()\n\n");
     }

    printf("num child : %d\n",n->num_child);
   for(i=0;i<n->num_child;i++){
       printf("child : %s\n",n->child[i]->file_name);
        printf("checkkkkkkkkkkkkkkkkkkkkkkkkkkk\n");
        memset(st, 0, sizeof(struct stat));
        printf("checkkkkkkkkkkkkkkkkkkkkkkkkkkk\n");
        //if(getattr(n->child[i]->file_name,st));
        int k = getattr(n->child[i]->file_name,st);
        printf("came bacl from getattr(), now need to fill buffer \n\n");
        filler(buf, n->child[i]->file_name, st, 0);
    }
    printf("return from l_readdir()\n"); 
    return 0;
}

void freeNode(struct node* n){
    n->file_name = NULL;
    n->buffer = NULL;
    n->parent = NULL;
    n->size = n->num_child = 0;
    free(n);
    return;
}

static int l_unlink(const char *path){
    printf("*** l_unlink() with path as %s***\n\n",path);
    //char *file = getFileName(path);
    int i,n_c;
    static node *n ,*n_par;
   /*
    if(map_node.find(path)==map_node.end())
        return ENOENT;
    else
        n = map_node[path];  
    */
    n = find_node(path);
    if(n==NULL)
	return -ENOENT;
    //n_par = n->parent;
    n_par = getParNode(path);
    n_c = n_par->num_child;

   printf("original child of parent node\n");
   for(i=0;i<n_par->num_child;i++){
   printf("child is %s \n",n_par->child[i]->file_name);
  }
  int kk = 0; 
    for(i=0;i<n_par->num_child;i++){
        if(n==n_par->child[i]){
            printf("**********************child with path as %s and node pointer as %d which is same as n_par->child[i] found at i : %d and num_child : %d match at i : %d\n",path,n,n_par->child[i],n_par->num_child,i);
          if(n->file_type=='d')
		root_size+=128;
	  else 
		root_size+=256;
            freeNode(n);
            kk = 1;
           n_par->num_child--;
           // if(i == n_par->num_child-1)
	   //	break; 
       }
       if(i == n_par->num_child)
         break;
       printf("*********child is %s \n",n_par->child[i]->file_name);
      if(kk){      
      n_par->child[i]=n_par->child[i+1];
      cout <<" now file at "<< i+1 <<" is copied at "<< i<<endl;
       } 
    }
    //free(n);
    //n_par->num_child--;
    printf("***********reduced num of child is %d\n",n_par->num_child);

   print_child(n_par);
   printf("erase entry from map_node1\n");
   map_node1.erase(path);
    return 0;
}

static int l_opendir(const char *path, struct fuse_file_info *fi){
    return 0;
}

static int l_rmdir(const char *path){
    printf("*** l_rmdir() with path as %s***\n\n",path);
    //char *file = getFileName(path);
    static node *n = find_node(path);
    //if(map_node1.find(path)==map_node.end())
    if(n==NULL)  
        return -ENOENT;
    cout<<"node is "<<n<<" num child : "<<n->num_child<<endl; 
    if(n->num_child != 0){
	printf("num_child !=0...so don't delete directory \n");
        return -ENOTEMPTY;
    }
    printf("num_child ==0..go to unlink \n");
    return l_unlink(path);    
}

static int l_mknod(const char *path, mode_t mode, dev_t rdev){
    printf("*** l_mknod() with path as %s***\n\n",path);
    if(root->size<1024)
        return -1;
    
    struct node *n1 = (struct node*)malloc(sizeof(struct node));
    char *file = getFileName(path); //gets filename from path
    struct node* n1_par = getParNode(path);
    char *path1=(char*)malloc(1024);
    strcpy(path1,path);
    n1->file_name = path1;//file;
    n1->file_type = 'f';
    n1->num_child = 0;
    n1->parent = n1_par;
    n1->size = 256;//put a check here to see if 1024 is still available with root
    n1->buffer = NULL;
    std::string ss(path);
    map_node1[path] = n1;
    map_node[path] = n1;  //mapping of complete path name to it's node;
    n1_par->child[n1_par->num_child++] = n1;
    root_size -= 256;
    print_child(n1_par); 
    return 0;    
}

static int l_mkdir(const char *path, mode_t mode){
    printf("*** l_mkdir() with path as %s***\n\n",path);
    if(root->size<128){
        printf("*********root size < 128 ..return ENOMEM*********\n");
        return -ENOMEM;
     }
    printf("****** create a directory node ****** \n");   
    struct node *n1 = (struct node*)malloc(sizeof(struct node));
    if(n1==NULL)
	printf("node of path %s is NULL\n",path);
    printf("node of path %s is %d \n",path,n1);
    cout<<"node as cpp command is "<<n1<<endl;
    char *dir = getFileName(path); //gets directory name from path
    struct node* n1_par = getParNode(path);
    if(n1_par==NULL)
	printf("parent of node is NULL\n");
    printf("parent of path %s is %s\n",path,n1_par->file_name);
    char *path1=(char*)malloc(1024);
    strcpy(path1,path);
    n1->file_name = path1;//dir; //may have to use strcpy()
    n1->file_type = 'd';
    n1->num_child = 0;
    n1->parent = n1_par;
    n1->size = 128;   //put a check here to see if 128 is still available with root
    n1->buffer = NULL;
    printf("before inserting %s , map node1 size is %d\n",path,map_node1.size());
    std::string ss(path);
    cout<<"string converted path is " <<ss<<endl;
    map_node[path] = n1;
    map_node1[ss] = n1;
    n1_par->num_child++;
    printf("increased child of parent and new num_child : %d  new map1 size is %d\n",n1_par->num_child,map_node1.size());
    n1_par->child[n1_par->num_child-1] = n1;
    root_size -= 128;
    printf("return from mkdir() after creating node for file %s\n",n1->file_name);
    printf("return from mkdir() after creating child node %d for node %d\n",n1_par->child[n1_par->num_child-1],n1_par);
    print_child(n1_par);
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
    std::string buf(root->file_name);
    map_node1[buf] = root;
    //map_node[root_path] = root;
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
    //root_path = argv[1];    
    root_size = atoi(argv[argc-1]);//*1024*1024; //MB to B conversion
    root_size = root_size*1024*1024;
    printf("********** root size : %d ************* \n",root_size);
    make_root_node(root_path,root_size);
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
    l_rmdk.write = l_write;
    l_rmdk.truncate = l_truncate;
    //strcpy(root_path ,"/");
    printf( "file is %s size %d  argc : %d\n", root_path, root_size, argc);//, map_node[root_path]->file_name,argc);
    printf("print root:\n");
    printNode(root);
    printf("******check********\n");
    return fuse_main(argc-1, argv, &l_rmdk, NULL);
}
