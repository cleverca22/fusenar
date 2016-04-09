#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <fuse.h>

#include "fusenar_core.h"

struct fuse_operations fusenar_oper = {
    .getattr = fusenar_getattr,
    .readlink = fusenar_readlink,
    .open = fusenar_open,
    .read = fusenar_read,
    .readdir = fusenar_readdir,
    .release = fusenar_release
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
