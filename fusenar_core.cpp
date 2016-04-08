#include <sys/types.h>
#include <dirent.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <string>
#include <iostream>

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
        return NULL;
    }
}

int fusenar_redirect_readdir(string path, void* buf, fuse_fill_dir_t filler) {
    cout << "listing " << path << endl;
    DIR* dir = opendir(path.c_str());
    if (!dir) return -ENOENT;
    struct dirent* dp;
    while ( (dp = readdir(dir)) != NULL) {
        cout << "found " << dp->d_name << endl;
        filler(buf,dp->d_name,NULL,0);
    }
    closedir(dir);
    return 0;
}
int fusenar_readdir(const char* path_in, void* buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info* fi) {
    int retstat = 0;
    struct fusenar_state* fusenar_data = (struct fusenar_state*)fuse_get_context()->private_data;
    printf("root dir %s\n",fusenar_data->rootdir);
    string path_buffer1, path_buffer2;
    string path = path_in;

    printf("fusenar_readdir(path=\"%s\", buf=0x%p, filler=0x%p, offset=%ld, fi=0x%p)\n",path_in,buf,filler,offset,fi);

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
            printf("found %s\n",dp->d_name);
            string entry = dp->d_name;
            size_t pos = entry.find(".nar");
            if ( (entry == ".") || (entry == "..")) {
            } else if ( entry.size() - 4 == pos) {
                printf("its a nar file\n");
                filler(buf,entry.substr(0,pos).c_str(),NULL,0);
            }
        }
        closedir(dir);
    }

    size_t firstslash = path.find("/",1);
    if (firstslash == string::npos) firstslash = path.size();
    string name = path.substr(1,firstslash-1);
    if (name.size() > 32) {
        cout << red << "checking cache for " << name << reset << endl;
        string cache_path = string(fusenar_data->cachedir) + "/" + name;
        struct stat statbuf2;
        if (stat(cache_path.c_str(),&statbuf2) == 0) {
            return fusenar_redirect_readdir(cache_path + "/" + path.substr(firstslash), buf, filler);
        }
        cout << "finding nar for " << name << endl;
        path_buffer2 = find_nar(name);
        if (path_buffer2.size() > 0) {
            //filler(buf,".",NULL,0);
            //filler(buf,"..",NULL,0);

            if (unpack_nar(path_buffer2, cache_path) == 0) {
                return fusenar_redirect_readdir(cache_path + "/" + path.substr(firstslash), buf, filler);
            } else {
                printf("failure to unpack nar\n");
                return -ENOENT;
            }
            return retstat;
        }
    }
    if (path != "/") return -ENOENT;

    return retstat;
}

int fusenar_getattr(const char* path_in, struct stat* statbuf) {
    int res = 0;
    char path_buffer[PATH_MAX];
    string path = path_in;
    struct fusenar_state* fusenar_data = (struct fusenar_state*)fuse_get_context()->private_data;

    //cout << red << __func__ << " " << path << reset << endl;

    memset(statbuf, 0, sizeof(struct stat));

    if (path.size() > 32) {
        size_t firstslash = path.find("/",1);
        if (firstslash == string::npos) firstslash = path.size();
        string name = path.substr(1,firstslash-1);
        string narpath = find_nar(name);
        string cache_path = string(fusenar_data->cachedir) + "/" + name;
        string absolute_path = cache_path + "/" + path.substr(firstslash);
        //cout << green << "checking cache for " << absolute_path << reset << endl;
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
    struct fusenar_state* fusenar_data = (struct fusenar_state*)fuse_get_context()->private_data;
    int retstat = 0;
    int fd;
    string path = path_in;
    size_t firstslash = path.find("/",1);
    if (firstslash == string::npos) firstslash = path.size();
    string name = path.substr(1,firstslash-1);
    string narpath = find_nar(name);
    string cache_path = string(fusenar_data->cachedir) + "/" + name;
    string absolute_path = cache_path + "/" + path.substr(firstslash);

    cout << green << "checking cache for " << absolute_path << reset << endl;

    fd = open(absolute_path.c_str(),fi->flags);
    if (fd < 0) retstat = -errno;

    fi->fh = fd;
    
    return retstat;
}

int fusenar_read(const char* path, char* buf, size_t size, off_t offset, struct fuse_file_info *fi) {
    int retstat = 0;

    retstat = pread(fi->fh, buf, size, offset);
    if (retstat < 0) retstat = -errno;

    return retstat;
}
int fusenar_readlink(const char* path, char* link, size_t size) {
    struct fusenar_state* fusenar_data = (struct fusenar_state*)fuse_get_context()->private_data;
    printf("fusenar_readlink %s %s %d\n",path,link,(int)size);
    ssize_t result = readlink( (string(fusenar_data->cachedir) + path).c_str(), link, size);
    if (result == -1) return -errno;
    link[result] = 0;
    return 0;
}
