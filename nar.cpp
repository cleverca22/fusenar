#include <stdint.h>
#include <sstream>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <map>
#include <string>
#include <iostream>
#include <unistd.h>
#include <string.h>

using namespace std;

#include "nar.h"


void NarIndexer::createDirectory(const nix::Path & path) {
    members.emplace(path, NarMember{NarMember::Type::tDirectory, false, 0, 0});
}
void NarIndexer::createRegularFile(const nix::Path & path) {
    currentPath = path;
    isExec = false;
}
void NarIndexer::isExecutable() {
    isExec = true;
}
void NarIndexer::preallocateContents(unsigned long long size) {
    members.emplace(currentPath, NarMember{NarMember::Type::tRegular, isExec, source->position, size});
}
void NarIndexer::receiveContents(unsigned char * data, unsigned int len) {
    if (!currentStart.empty()) {
        assert(len < 16 || currentStart == string((char *) data, 16));
        currentStart.clear();
    }
}
void NarIndexer::createSymlink(const nix::Path & path, const string & target) {
    members.emplace(path, NarMember{NarMember::Type::tSymlink, false, 0, target.size(), target});
}
NarHandle::NarHandle(const string nar) : source(nar) {
    cout << "indexing nar file:" << nar << endl;
    if (source.open()) throw OpenNarFail();
    indexer.source = &source;
    parseDump(indexer, source);
}
MMapSource::MMapSource(string file_in) : file(file_in) {
    address = 0;
    size = 0;
}
MMapSource::~MMapSource() {
    munmap(address, size);
}
bool MMapSource::open() {
    struct stat statbuf;
    int fd = ::open(file.c_str(), O_RDONLY);
    int err;
    if (fd < 0) {
        cerr << "unable to open " << file << endl;
        return true;
    }

    if (fstat(fd, &statbuf)) {
        err = errno;
        cerr << "unable to stat " << fd << " " << err << " " << strerror(err) << " " << file << endl;
        close(fd);
        return true;
    }
    size = statbuf.st_size;
    //cout << "file size is " << size << endl;
    position = 0;
    address = (uint8_t*) mmap(0, size, PROT_READ, MAP_SHARED, fd, 0);
    if (address == MAP_FAILED) {
        close(fd);
        return true;
    }
    close(fd);
    return false;
}
size_t MMapSource::read(unsigned char* data, size_t len) {
    int available = std::min(size - position,len);
    memcpy(data, address + position, available);
    position += available;
    return available;
}
class NarExtractor : public nix::ParseSink {
    string root;
    FILE *currentFile;
    unsigned long long bytes_remaining;
    bool isExec;
public:
    NarExtractor(string nar, string out) {
        root = out;
        //int fd = open(nar.c_str() ,O_RDONLY);
        //nix::FdSource handle(fd);
        MMapSource handle(nar);
        if (handle.open()) throw OpenNarFail();
        parseDump(*this,handle);
        //close(fd);
    }
    void createDirectory(const nix::Path & path) override {
        string target = root + path;
        mkdir(target.c_str(),0700);
    }
    void createRegularFile(const nix::Path & path) override {
        string target = root + path;
        currentFile = fopen(target.c_str(), "w");
        if (!currentFile) throw OpenFail();
        //cout << "making " << target << endl;
        isExec = false;
    }
    void isExecutable() override {
        isExec = true;
        puts("it is executable");
    }
    void preallocateContents(unsigned long long size) override {
        ftruncate(fileno(currentFile), size);
        //cout << "size set to " << size << endl;
        bytes_remaining = size;
    }
    void receiveContents(unsigned char * data, unsigned int len) override {
        //cout << len << " bytes added" << endl;
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
        //printf("symlink %s -> %s\n",path.c_str(), target.c_str());
        symlink(target.c_str(), (root + path).c_str());
    }
};

typedef map<string,struct NarHandle*> NarCache;
typedef map<string,struct NarHandle*>::iterator NarCacheIt;
NarCache open_nars;
struct NarHandle* open_nar(const string path) {
    NarCacheIt it = open_nars.find(path);
    if (it != open_nars.end()) {
        return it->second;
    }
    NarHandle* handle = new NarHandle(path);
    open_nars[path] = handle;
    return handle;
}
struct NarHandle* open_nar_simple(const string path) {
    string narpath = find_nar(path);
    if (narpath.size() > 0) return open_nar(narpath);
    else return NULL;
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
void NarHandle::readdir(string path, void* buf, fuse_fill_dir_t filler) {
    NarIndexer::Members::iterator it;
    set<string> results;
    cout << "readdir '" << path << "'(" << path.size() << ")" << endl;

    size_t offset = max((int)path.size()+1,1);

    for (it = indexer.members.begin(); it != indexer.members.end(); ++it) {
        if (it->first.size() == 0) continue;

        if (it->first.substr(0,path.size()) != path) {
            cout << "skipping:" << it->first << endl;
            continue;
        }

        size_t pos = it->first.find("/",offset);
        if (pos == string::npos) pos = it->first.size();
        else pos -= 1;

        if (offset > it->first.size()) continue;

        string fragment = it->first.substr(offset, (pos - offset)+1);

        results.emplace(fragment);

        cout << "pos:" << pos << " full:" << it->first << " fragment:" << fragment << endl;
    }
    set<string>::iterator it2;
    for (it2 = results.begin(); it2 != results.end(); ++it2) {
        filler(buf, it2->c_str(), NULL, 0);
    }
}
int NarHandle::getattr(std::string path, struct stat *statbuf) {
    NarIndexer::Members::iterator it;
    it = indexer.members.find(path);
    if (it == indexer.members.end()) return -ENOENT;
    else {
        switch (it->second.type) {
        case NarMember::tRegular:
            if (it->second.isExecutable) {
                statbuf->st_mode = S_IFREG | 0555;
            } else {
                statbuf->st_mode = S_IFREG | 0444;
            }
            statbuf->st_size = it->second.size;
            break;
        case NarMember::tSymlink:
            statbuf->st_mode = S_IFLNK | 0777;
            statbuf->st_size = it->second.size;
            break;
        case NarMember::tDirectory:
            statbuf->st_mode = S_IFDIR | 0555;
            break;
        default:
            cout << "fault " << it->second.type << endl;
            return -ENOENT;
        }
        return 0;
    }
    for (it = indexer.members.begin(); it != indexer.members.end(); ++it) {
        cout << "'" << it->first << "' " << it->second.type << endl;
    }
}
int NarHandle::readlink(std::string path, char* link, size_t size) {
    NarIndexer::Members::iterator it;
    it = indexer.members.find(path);
    if (it == indexer.members.end()) return -ENOENT;
    else if (it->second.type != NarMember::tSymlink) return -EINVAL;
    else {
        int size2 = it->second.target.copy(link,size);
        link[size2] = 0;
        return 0;
    }
}
int NarHandle::open(std::string path, FileHandle** fh) {
    NarIndexer::Members::iterator it;
    it = indexer.members.find(path);
    if (it == indexer.members.end()) return -ENOENT;
    else if (it->second.type != NarMember::tRegular) return -EINVAL;
    else {
        *fh = new FileHandle(&source, it->second);
        return 0;
    }
}
FileHandle::FileHandle(MMapSource* source, NarMember file): source(source), file(file) {
}
int FileHandle::pread(char* buf, size_t size, off_t offset) {
    size_t available = min(size, file.size - offset);
    memcpy(buf, source->addr() + file.start + offset, available);
    return available;
}
