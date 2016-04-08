#ifdef __cplusplus
extern "C" {
#endif
struct fusenar_state {
    char* rootdir;
    char* cachedir;
};

int fusenar_readdir(const char* path, void* buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info* fi);
int fusenar_getattr(const char* path, struct stat* statbuf);
int fusenar_open(const char* path_in, struct fuse_file_info* fi);
int fusenar_read(const char* path, char* buf, size_t size, off_t offset, struct fuse_file_info *fi);
int fusenar_readlink(const char* path, char* link, size_t size);
#ifdef __cplusplus
}
#endif
