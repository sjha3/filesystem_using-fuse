#define FUSE_USE_VERSION 30

#include <config.h>

#include <fuse.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <stddef.h>
#include <assert.h>

static struct options {
	const char *filename;
	const char *contents;
	int show_help;
} options;

struct node{
    char *file_name;
    char file_type;
    int num_child;
    struct node* child[1024];
    struct node* parent;
}

char *getFilename(const char* path){
    char *fpath=malloc(1024*sizeof(char));
    char s[]="/";
    strcpy(fpath,path);
    char *token=strtok(path,s);
    
    
}

static int l_mknod(const char *path, mode_t mode, dev_t rdev){
    struct node *n1 = (struct node*)malloc(sizeof(struct node));
    char *file = getFilename(path);
    struct node* n1_par = getParent(path);
    n1->filename = file;
    n1->file_type = 'f';
    n1->num_child = 0;
    n1->parent = n1_par;

    n1_par->child[n1_par.num_child++] = n1;
    
}

int main(int argc, char *argv[])
{
	struct fuse_args args = FUSE_ARGS_INIT(argc, argv);

	/* Set defaults -- we have to use strdup so that
	   fuse_opt_parse can free the defaults if other
	   values are specified */
	options.filename = strdup("hello");
	options.contents = strdup("Hello World!\n");

	/* Parse options */
	if (fuse_opt_parse(&args, &options, option_spec, NULL) == -1)
		return 1;

	/* When --help is specified, first print our own file-system
	   specific help text, then signal fuse_main to show
	   additional help (by adding `--help` to the options again)
	   without usage: line (by setting argv[0] to the empty
	   string) */
	if (options.show_help) {
		show_help(argv[0]);
		assert(fuse_opt_add_arg(&args, "--help") == 0);
		args.argv[0] = (char*) "";
	}

	return fuse_main(args.argc, args.argv, &hello_oper, NULL);
}