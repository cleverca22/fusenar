#include <sys/types.h>
#include <dirent.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <string>
#include <iostream>
#include <sstream>

#include <fuse.h>

#include <termcolor/termcolor.hpp>

#include "fusenar_core.h"
#include "nar.h"

using namespace std;
using namespace termcolor;

string find_nar(string prefix) {
    string output;
    struct stat statbuf2;
    struct fusenar_state* fusenar_data = (struct fusenar_state*)fuse_get_context()->private_data;
    memset(&statbuf2, 0, sizeof(struct stat));

    //printf("root dir %s, prefix %s\n",fusenar_data->rootdir,prefix.c_str());
    output = string(fusenar_data->rootdir) + "/" + prefix + ".nar";
    //printf("final path %s\n",output.c_str());
    if (stat(output.c_str(),&statbuf2) == 0) {
        return output;
    } else {
        return "";
    }
}

int fusenar_readdir(const char* path_in, void* buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info* fi) {
    int retstat = 0;
    struct fusenar_state* fusenar_data = (struct fusenar_state*)fuse_get_context()->private_data;
    string path_buffer1, path_buffer2;
    string path = path_in;

    printf("fusenar_readdir(path=\"%s\", fi=0x%p)\n",path_in,fi);

    if (path[0] != '/') {
        printf("error, path must begin with /\n");
        return -ENOENT;
    }

    // generates a listing for the root folder
    if (path == "/") {
        DIR* dir = opendir(fusenar_data->rootdir);
        if (!dir) {
            printf("unable to open %s, %s\n",fusenar_data->rootdir,strerror(errno));
            return -ENOENT;
        }
        filler(buf,".",NULL,0);
        filler(buf,"..",NULL,0);
        struct dirent* dp;
        while ( (dp = readdir(dir)) != NULL) {
            //printf("found %s\n",dp->d_name);
            string entry = dp->d_name;
            size_t pos = entry.find(".nar");
            if ( (entry == ".") || (entry == "..")) {
            } else if ( entry.size() - 4 == pos) {
                //printf("its a nar file\n");
                filler(buf,entry.substr(0,pos).c_str(),NULL,0);
            }
        }
        closedir(dir);
    }

    size_t firstslash = path.find("/",1);
    if (firstslash == string::npos) firstslash = path.size();
    string name = path.substr(1,firstslash-1);
    if (name.size() > 32) {
        NarHandle* handle = open_nar_simple(name);
        assert(handle);

        string remainder = path.substr(firstslash);
        filler(buf,".",NULL,0);
        filler(buf,"..",NULL,0);
        handle->readdir(remainder, buf, filler);
        return 0;
    }
    if (path != "/") return -ENOENT;

    return retstat;
}

int fusenar_getattr(const char* path_in, struct stat* statbuf) {
    int res = 0;
    char path_buffer[PATH_MAX];
    string path = path_in;
    struct fusenar_state* fusenar_data = (struct fusenar_state*)fuse_get_context()->private_data;

    memset(statbuf, 0, sizeof(struct stat));

    if (path.size() > 32) {
        size_t firstslash = path.find("/",1);
        if (firstslash == string::npos) firstslash = path.size();
        string name = path.substr(1,firstslash-1);
        NarHandle* handle = open_nar_simple(name);
        if (!handle) return -ENOENT;

        string remainder = path.substr(firstslash);
        return handle->getattr(remainder, statbuf);

        string narpath = find_nar(name);
        string cache_path = string(fusenar_data->cachedir) + "/" + name;
        string absolute_path = cache_path;
        if (path.size() > firstslash) {
            absolute_path = absolute_path + "/" + path.substr(firstslash);
        }
        //cout << green << "3checking cache for " << absolute_path << reset << endl;
        struct stat statbuf2;
        if (lstat(absolute_path.c_str(),&statbuf2) == 0) {
            statbuf->st_mode = statbuf2.st_mode;
            statbuf->st_size = statbuf2.st_size;
            return 0;
        }

        if (stat(cache_path.c_str(),&statbuf2) == 0) {
            // the nar has been unpacked, the file just doesnt exist
            return -ENOENT;
        } else {
            // the nar needs to be unpacked
            if (unpack_nar(narpath, cache_path) == 0) {
                // check again
                if (lstat(absolute_path.c_str(),&statbuf2) == 0) {
                    statbuf->st_mode = statbuf2.st_mode;
                    statbuf->st_size = statbuf2.st_size;
                    return 0;
                } else return -ENOENT;
            } else return -EIO;
        }
        
        if (narpath.size() > 0) {
            statbuf->st_mode = S_IFDIR | 0755;
            statbuf->st_nlink = 2;
            return res;
        } else {
            printf("unable to stat %s, %s\n",path_buffer, strerror(errno));
        }
    }

    if (path == "/") {
        statbuf->st_mode = S_IFDIR | 0755;
        statbuf->st_nlink = 2;
    } else {
        res = -ENOENT;
    }
    return res;
}

int fusenar_open(const char* path_in, struct fuse_file_info* fi) {
    int retstat = 0;
    string path = path_in;
    size_t firstslash = path.find("/",1);
    if (firstslash == string::npos) firstslash = path.size();
    string name = path.substr(1,firstslash-1);
    NarHandle* handle = open_nar_simple(name);
    assert(handle);

    string remainder = path.substr(firstslash);
    FileHandle* fh;
    retstat = handle->open(remainder, &fh);
    if (retstat == 0) {
        fi->fh = (uint64_t)fh;
        return 0;
    } else return retstat;
}

int fusenar_read(const char* path, char* buf, size_t size, off_t offset, struct fuse_file_info *fi) {
    FileHandle* fh = (FileHandle*)fi->fh;

    return fh->pread(buf, size, offset);
}

int fusenar_release(const char* path, struct fuse_file_info *fi) {
    FileHandle* fh = (FileHandle*)fi->fh;

    delete fh;
    return 0;
}

int fusenar_readlink(const char* path_in, char* link, size_t size) {
    string path = path_in;
    size_t firstslash = path.find("/",1);
    if (firstslash == string::npos) firstslash = path.size();
    string name = path.substr(1,firstslash-1);
    NarHandle* handle = open_nar_simple(name);
    assert(handle);

    string remainder = path.substr(firstslash);
    return handle->readlink(remainder, link, size);
}
