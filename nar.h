#pragma once

#include <nix/serialise.hh>
#include <nix/archive.hh>
#include <map>

#include <fuse.h>

class MMapSource : public nix::Source {
public:
    MMapSource(std::string file);
    ~MMapSource();
    virtual size_t read(unsigned char* data, size_t len);
    bool open();
    uint8_t* addr() { return address; }

    size_t position;
private:
    uint8_t* address;
    size_t size;
    std::string file;
};
struct NarMember {
    enum Type { tMissing, tRegular, tSymlink, tDirectory };

    Type type;

    bool isExecutable;

    size_t start, size;

    std::string target;
};
class FileHandle {
public:
    FileHandle(MMapSource* source, NarMember file);
    int pread(char* buf, size_t size, off_t offset);
private:
    MMapSource* source;
    NarMember file;
};
class NarIndexer : public nix::ParseSink {
public:
    void createDirectory(const nix::Path & path) override;
    void createRegularFile(const nix::Path & path) override;
    void isExecutable() override;
    void preallocateContents(unsigned long long size) override;
    void receiveContents(unsigned char * data, unsigned int len) override;
    void createSymlink(const nix::Path & path, const std::string & target) override;

    typedef std::map<nix::Path, NarMember> Members;

    Members members;
    nix::Path currentPath;
    std::string currentStart;
    bool isExec;
    MMapSource* source;
};
class NarHandle {
    NarIndexer indexer;
    MMapSource source;
public:
    NarHandle(std::string narpath);
    void readdir(std::string path, void* buf, fuse_fill_dir_t filler);
    int getattr(std::string path, struct stat *statbuf);
    int readlink(std::string path, char* link, size_t size);
    int open(std::string path, FileHandle** fh);
};

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
struct NarHandle* open_nar(const std::string path);
struct NarHandle* open_nar_simple(const std::string path);
std::string find_nar(const std::string prefix);

class OpenFail : public std::exception {
public:
    const char* what() const noexcept { return "unable to open target file"; };
};
class OpenNarFail : public std::exception {
public:
    const char* what() const noexcept { return "unable to open target file"; };
};
