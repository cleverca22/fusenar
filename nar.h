#pragma once

struct NarHandle;

#ifdef __cplusplus
extern "C" {
#endif
    struct NarHandle* open_nar(const char* path);
    typedef void (*Filler)(const char* entry, int isDir, int isExec);
    void nar_list(const char* narpath, const char* dir, Filler filler);
#ifdef __cplusplus
}
#endif
int unpack_nar(std::string narpath, std::string outpath);
