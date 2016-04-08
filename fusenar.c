#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <errno.h>
#include <linux/limits.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <unistd.h>

#include <fuse.h>

#include "fusenar_core.h"

const char* example = "hello world";

int fusenar_getattr(const char* path, struct stat* statbuf);
int fusenar_readlink(const char* path, char* link, size_t size);
int fusenar_open(const char* path, struct fuse_file_info* fi);
int fusenar_read(const char* path, char* buf, size_t size, off_t offset, struct fuse_file_info *fi);
int fusenar_readdir(const char* path, void* buf, fuse_fill_dir_t fillter, off_t offset, struct fuse_file_info* fi);

struct fuse_operations fusenar_oper = {
    .getattr = fusenar_getattr,
    .readlink = fusenar_readlink,
    .open = fusenar_open,
    .read = fusenar_read,
    .readdir = fusenar_readdir
};

int main(int argc, char** argv) {
    if ((argc < 4) || (argv[argc-2][0] == '-') || (argv[argc-1][0] == '-')) {
        printf("%s <cachedir> <nardir> <mountpoint>\n",argv[0]);
        return -1;
    }

    struct fusenar_state fusenar_data;

    fusenar_data.rootdir = realpath(argv[argc-2], NULL);
    fusenar_data.cachedir = realpath(argv[argc-3], NULL);
    argv[argc-3] = argv[argc-1];
    argv[argc-2] = NULL;
    argc--;
    argc--;
    
    int i;
    for (i=0; i<argc; i++) {
        printf("%d = %s\n",i,argv[i]);
    }
    printf("data %s\ncache %s\n",fusenar_data.rootdir,fusenar_data.cachedir);
    return fuse_main(argc,argv,&fusenar_oper,&fusenar_data);
}


