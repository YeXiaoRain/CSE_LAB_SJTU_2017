// yfs client.  implements FS operations using extent and lock server
#include "yfs_client.h"
#include "extent_client.h"
#include <sstream>
#include <iostream>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

yfs_client::yfs_client()
{
  ec = NULL;
  lc = NULL;
}

yfs_client::yfs_client(std::string extent_dst, std::string lock_dst, const char* cert_file)
{
  ec = new extent_client(extent_dst);
  lc = new lock_client(lock_dst);
  if (ec->put(1, "") != extent_protocol::OK)
      printf("error init root dir\n"); // XYB: init root dir
}

int
yfs_client::verify(const char* name, unsigned short *uid)
{
  	int ret = OK;

	return ret;
}


yfs_client::inum
yfs_client::n2i(std::string n)
{
    std::istringstream ist(n);
    unsigned long long finum;
    ist >> finum;
    return finum;
}

std::string
yfs_client::filename(inum inum)
{
    std::ostringstream ost;
    ost << inum;
    return ost.str();
}

bool
yfs_client::isfile(inum inum)
{
    lc->acquire(inum);
    extent_protocol::attr a;

    if (ec->getattr(inum, a) != extent_protocol::OK) {
        printf("error getting attr\n");
        lc->release(inum);
        return false;
    }

    if (a.type == extent_protocol::T_FILE) {
        printf("isfile: %lld is a file\n", inum);
        lc->release(inum);
        return true;
    }
    printf("isfile: %lld is not a file\n", inum);
    lc->release(inum);
    return false;
}

bool
yfs_client::issymlink(inum inum)
{
    lc->acquire(inum);
    extent_protocol::attr a;

    if (ec->getattr(inum, a) != extent_protocol::OK) {
        printf("error getting attr\n");
        lc->release(inum);
        return false;
    }

    if (a.type == extent_protocol::T_SYMLINK) {
        printf("issymlink: %lld is a symlink\n", inum);
        lc->release(inum);
        return true;
    }
    printf("issymlink: %lld is not a symlink\n", inum);
    lc->release(inum);
    return false;
}

bool
yfs_client::_isdir(inum inum)
{
    extent_protocol::attr a;

    if (ec->getattr(inum, a) != extent_protocol::OK) {
        printf("error getting attr\n");
        return false;
    }

    if (a.type == extent_protocol::T_DIR) {
        printf("isdir: %lld is a dir\n", inum);
        return true;
    }
    printf("isdir: %lld is not a dir\n", inum);
    return false;
}

bool
yfs_client::isdir(inum inum)
{
    lc->acquire(inum);
    bool r = _isdir(inum);
    lc->release(inum);
    return r;
}

int
yfs_client::getfile(inum inum, fileinfo &fin)
{
    lc->acquire(inum);
    int r = OK;

    printf("getfile %016llx\n", inum);
    extent_protocol::attr a;
    if (ec->getattr(inum, a) != extent_protocol::OK) {
        r = IOERR;
        goto release;
    }

    fin.atime = a.atime;
    fin.mtime = a.mtime;
    fin.ctime = a.ctime;
    fin.size = a.size;
    printf("getfile %016llx -> sz %llu\n", inum, fin.size);

release:
    lc->release(inum);
    return r;
}

int
yfs_client::getsymlink(inum inum, symlinkinfo &symin)
{
    lc->acquire(inum);
    int r = OK;

    printf("getsym %016llx\n", inum);
    extent_protocol::attr a;
    if (ec->getattr(inum, a) != extent_protocol::OK) {
        r = IOERR;
        goto release;
    }

    symin.atime = a.atime;
    symin.mtime = a.mtime;
    symin.ctime = a.ctime;
    symin.size = a.size;
    printf("getsym %016llx -> sz %llu\n", inum, symin.size);

release:
    lc->release(inum);
    return r;
}

int
yfs_client::getdir(inum inum, dirinfo &din)
{
    lc->acquire(inum);
    int r = OK;

    printf("getdir %016llx\n", inum);
    extent_protocol::attr a;
    if (ec->getattr(inum, a) != extent_protocol::OK) {
        r = IOERR;
        goto release;
    }
    din.atime = a.atime;
    din.mtime = a.mtime;
    din.ctime = a.ctime;

release:
    lc->release(inum);
    return r;
}


#define EXT_RPC(xx) do { \
    if ((xx) != extent_protocol::OK) { \
        printf("EXT_RPC Error: %s:%d \n", __FILE__, __LINE__); \
        r = IOERR; \
        goto release; \
    } \
} while (0)

// Only support set size of attr
int
yfs_client::setattr(inum ino, filestat st, unsigned long toset)
{
  lc->acquire(ino);
  std::string buf;
  int r;
  if((r = ec->get(ino, buf)) != extent_protocol::OK){
    lc->release(ino);
    return r;
  }
  buf.resize(size);
  lc->release(ino);
  return ec->put(ino, buf);
}

int
yfs_client::create(inum parent, const char *name, mode_t mode, inum &ino_out)
{
  lc->acquire(parent);
  std::string dir_content;
  if (ec->get(parent, dir_content) != extent_protocol::OK){
    lc->release(parent);
    return IOERR;
  }
  if (dir_content.find("/" + std::string(name) + "//") != std::string::npos){
    lc->release(parent);
    return EXIST;
  }
  if (ec->create(extent_protocol::T_FILE, ino_out) != extent_protocol::OK){
    lc->release(parent);
    return IOERR;
  }
  ec->put(parent, dir_content + "/" + std::string(name) + "//" + filename(ino_out));
  lc->release(parent);
  return OK;
}

int
yfs_client::mkdir(inum parent, const char *name, mode_t mode, inum &ino_out)
{
  lc->acquire(parent);
  std::string dir_content;
  if (ec->get(parent, dir_content) != extent_protocol::OK){
    lc->release(parent);
    return IOERR;
  }
  if (dir_content.find("/" + std::string(name) + "//") != std::string::npos){
    lc->release(parent);
    return EXIST;
  }
  if (ec->create(extent_protocol::T_DIR, ino_out) != extent_protocol::OK){
    lc->release(parent);
    return IOERR;
  }
  int r = ec->put(parent, dir_content + "/" + std::string(name) + "//" + filename(ino_out));
  lc->release(parent);
  return r;
}

int
yfs_client::symlink(inum parent, const char * name, const char * link, inum & ino_out)
{
  lc->acquire(parent);
  std::string dir_content;
  if (ec->get(parent, dir_content) != extent_protocol::OK){
    lc->release(parent);
    return IOERR;
  }
  if (dir_content.find("/" + std::string(name) + "//") != std::string::npos){
    lc->release(parent);
    return EXIST;
  }
  if (ec->create(extent_protocol::T_SYMLINK, ino_out) != extent_protocol::OK){
    lc->release(parent);
    return IOERR;
  }
  if (ec->put(ino_out, std::string(link))!= extent_protocol::OK){
    lc->release(parent);
    return IOERR;
  }
  int r = ec->put(parent, dir_content + "/" + std::string(name) + "//" + filename(ino_out));
  lc->release(parent);
  return r;
}

int
yfs_client::lookup(inum parent, const char *name, bool &found, inum &ino_out)
{
  lc->acquire(parent);
  found = false;
  std::string dir_content;
  if (ec->get(parent, dir_content) != extent_protocol::OK){
    lc->release(parent);
    return IOERR;
  }
  dir_content+="/";
  std::string file_name = "/" + std::string(name) + "//";
  size_t st, end;
  if((st = dir_content.find(file_name)) != std::string::npos){
    found = true;
    st += file_name.size();
    end = dir_content.find_first_of("/", st);
    ino_out = n2i(dir_content.substr(st, end-st));
  }
  lc->release(parent);
  return OK;
}

int
yfs_client::readdir(inum dir, std::list<dirent> &list)
{
  lc->acquire(dir);
  std::string dir_content;
  if(ec->get(dir, dir_content) != extent_protocol::OK){
    lc->release(dir);
    return IOERR;
  }
  dir_content+="/";
  size_t st = 1, end;
  while(st != dir_content.size()) {
    dirent dirent_pair;
    end = dir_content.find_first_of("//", st);
    dirent_pair.name = dir_content.substr(st, end - st);

    st = end + 2;
    end = dir_content.find_first_of("/", st);
    dirent_pair.inum = n2i(dir_content.substr(st, end - st));
    list.push_back(dirent_pair);
    st = end + 1;
  }
  lc->release(dir);
  return OK;
}

int
yfs_client::read(inum ino, size_t size, off_t off, std::string &data)
{
  lc->acquire(ino);
  std::string buf;
  int r;
  if((r = ec->get(ino, buf))!= extent_protocol::OK){
    lc->release(ino);
    return r;
  }
  if (off < (off_t)buf.size())
    data = buf.substr(off, size);
  else
    data = "";
  lc->release(ino);
  return OK;
}

int
yfs_client::write(inum ino, size_t size, off_t off, const char *data,
        size_t &bytes_written)
{
  lc->acquire(ino);
  std::string buf;
  int r;
  if((r = ec->get(ino, buf)) != extent_protocol::OK){
    lc->release(ino);
    return r;
  }
  if (buf.size() < off + size)
    buf.resize(off + size);
  for(unsigned int i=0;i<size;i++)
    buf[off+i]=data[i];
  if((r = ec->put(ino, buf)) != extent_protocol::OK){
    lc->release(ino);
    return r;
  }
  bytes_written = size;
  lc->release(ino);
  return OK;
}

int yfs_client::unlink(inum parent,const char *name)
{
  lc->acquire(parent);
  std::string dir_content;
  std::string file_name = "/" + std::string(name) + "//";
  if (ec->get(parent, dir_content) != extent_protocol::OK){
    lc->release(parent);
    return IOERR;
  }
  dir_content+="/";
  size_t st0, st1 , end;
  if ((st0 = dir_content.find(file_name)) == std::string::npos){
    lc->release(parent);
    return NOENT;
  }
  st1 = st0 + file_name.size();
  if((end = dir_content.find_first_of("/", st1)) == std::string::npos){
    lc->release(parent);
    return NOENT;
  }
  inum ino = n2i(dir_content.substr(st1, end - st1));
  if (_isdir(ino)){
    lc->release(parent);
    return IOERR;
  }
  dir_content.erase(st0, end - st0);
  dir_content.erase(dir_content.size()-1);
  if (ec->put(parent, dir_content) != extent_protocol::OK){
    lc->release(parent);
    return IOERR;
  }
  int r = ec->remove(ino);
  lc->release(parent);
  return r;
}

int
yfs_client::readlink(inum ino, std::string &link)
{
  lc->acquire(ino);
  int r = ec->get(ino, link);
  lc->release(ino);
  return r;
}

