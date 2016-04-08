#include <stdint.h>
#include <sstream>
#include <nix/archive.hh>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <map>
#include <string>
#include <iostream>

using namespace std;

#include "nar.h"

struct NarMember {
    enum Type { tMissing, tRegular, tSymlink, tDirectory };

    Type type;

    bool isExecutable;

    size_t start, size;

    std::string target;
};
class NarIndexer : public nix::ParseSink {
public:
    typedef std::map<nix::Path, NarMember> Members;

    Members members;
    nix::Path currentPath;
    std::string currentStart;
    bool isExec;

    void createDirectory(const nix::Path & path) override {
        members.emplace(path, NarMember{NarMember::Type::tDirectory, false, 0, 0});
        printf("directory %s\n",path.c_str());
    }
    void createRegularFile(const nix::Path & path) override {
        currentPath = path;
        printf("create file %s\n",path.c_str());
    }
    void isExecutable() override {
        isExec = true;
        puts("it is executable");
    }
    void receiveContents(unsigned char * data, unsigned int len) override {
        printf("%p %d\n",data,len);
        if (!currentStart.empty()) {
            assert(len < 16 || currentStart == string((char *) data, 16));
            currentStart.clear();
        }
    }
};
struct NarHandle {
    NarIndexer indexer;
public:
    NarHandle(const char* nar) {
        int fd = open(nar ,O_RDONLY);
        nix::FdSource handle(fd);
        parseDump(indexer, handle);
        close(fd);
    }
};
class OpenFail : public exception {
public:
    const char* what() const noexcept { return "unable to open target file"; };
};
class NarExtractor : public nix::ParseSink {
    string root;
    FILE *currentFile;
    unsigned long long bytes_remaining;
    bool isExec;
public:
    NarExtractor(string nar, string out) {
        root = out;
        int fd = open(nar.c_str() ,O_RDONLY);
        nix::FdSource handle(fd);
        parseDump(*this,handle);
        close(fd);
    }
    void createDirectory(const nix::Path & path) override {
        string target = root + path;
        mkdir(target.c_str(),0700);
    }
    void createRegularFile(const nix::Path & path) override {
        string target = root + path;
        currentFile = fopen(target.c_str(), "w");
        if (!currentFile) throw OpenFail();
        cout << "making " << target << " " << currentFile << endl;
        isExec = false;
    }
    void isExecutable() override {
        isExec = true;
        puts("it is executable");
    }
    void preallocateContents(unsigned long long size) override {
        ftruncate(fileno(currentFile), size);
        cout << "size set to" << size << endl;
        bytes_remaining = size;
    }
    void receiveContents(unsigned char * data, unsigned int len) override {
        cout << len << "bytes added" << endl;
        fwrite(data,len,1,currentFile);
        bytes_remaining = bytes_remaining - len;
        if (bytes_remaining == 0) {
            if (isExec) {
                fchmod(fileno(currentFile), 0555);
            } else {
                fchmod(fileno(currentFile), 0444);
            }
            fclose(currentFile);
        }
    }
    void createSymlink(const nix::Path & path, const string & target) override {
        printf("symlink %s -> %s\n",path.c_str(), target.c_str());
        symlink(target.c_str(), (root + path).c_str());
    }
};

extern "C" {
    struct NarHandle* open_nar(const char* path) {
        NarHandle* handle = new NarHandle(path);
        return handle;
    }
}
int unpack_nar(string narpath, string outpath) {
    try {
        printf("unpacking %s to %s\n",narpath.c_str(),outpath.c_str());
        NarExtractor handle(narpath,outpath);
        return 0;
    } catch (OpenFail of) {
        return -1;
    }
}
