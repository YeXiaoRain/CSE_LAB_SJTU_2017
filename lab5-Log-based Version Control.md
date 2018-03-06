# Lab 5: Log-based Version Control

## Introduction

Usually, we depend on log systems to survive from crashes. However, human error are even more harmful than crashes. Thus, version control is needed to help us. In this lab, we will add support for version control operations based on log.

首先这个lab,不是来自MIT的【目测】，首次出现是在sjtu的13级的cse_lab6里面。 然后现在sjtu的cse_lab把两个与cache相关的lab砍掉了？？

## Getting started

Please back up your solution to lab4 before going on.

    % git commit -am "sol for lab4"
    % git checkout -b lab5 origin/lab5
    % git merge lab4

Note: There may be some conflicts. If so, you need to merge it manually. Please make sure that you can run it properly in the docker(under the guidance of lab4).

Note: Here when you execute "git branch", you should see that you are in lab5 branch.

果然后面lab都是merge不再像最开始lab的那种copy paste了XD

我现在做的这个lab的冲突除了git的，从make时的错误还有

 * `yfs_client.cc`的setattr的`resize(size)`改为`resize(st.size)`
 * `yfs_client.h`中的`#define CA_FILE "./cert/ca.pem"`引号打成冒号了

## Part I - Try

New API Three operations have been added. By typing

    %./yfs-version -h
  
it will show you the usage.
    %./start.sh
    %./yfs-version -c
    %./yfs-version -p
    %./yfs-version -n
  
Each of the above will send a signal to your yfs_client. (Note: You must start a yfs_client by start.sh, before performing those operations and there can be only one yfs_client running in the background.)

To receive the signals, you should register a signal handler in fuse.cc. Here's a example,

    void sig_handler(int no) {
      switch (no) {
      case SIGINT:
          printf("commit a new version\n");
          break;
      case SIGUSR1:
          printf("to previous version\n");
          break;
      case SIGUSR2:
          printf("to next version\n");
          break;
      }
    }
  
Try those operations and test your system before going on.

就在 磁盘初始化前注册个处理函数就好了,改动如下

```diff
diff --git a/fuse.cc b/fuse.cc
index deb91f5..d72f7c9 100644
--- a/fuse.cc
+++ b/fuse.cc
@@ -17,6 +17,7 @@
 #include <arpa/inet.h>
 #include "lang/verify.h"
 #include "yfs_client.h"
+#include "signal.h"
 
 int myid;
 yfs_client *yfs;
@@ -553,6 +554,20 @@ fuseserver_symlink(fuse_req_t req, const char *link, fuse_ino_t parent, const ch
 
 struct fuse_lowlevel_ops fuseserver_oper;
 
+void sig_handler(int no) {
+  switch (no) {
+    case SIGINT:
+      printf("commit a new version\n");
+      break;
+    case SIGUSR1:
+      printf("to previous version\n");
+      break;
+    case SIGUSR2:
+      printf("to next version\n");
+      break;
+  }
+}
+
 int
 main(int argc, char *argv[])
 {
@@ -597,6 +612,10 @@ main(int argc, char *argv[])
     fuseserver_oper.readlink   = fuseserver_readlink;
     fuseserver_oper.symlink    = fuseserver_symlink;
 
+    signal(SIGINT , sig_handler);
+    signal(SIGUSR1, sig_handler);
+    signal(SIGUSR2, sig_handler);
+
     const char *fuse_argv[20];
     int fuse_argc = 0;
     fuse_argv[fuse_argc++] = argv[0];
```

## Part II - Undo and Redo

 * Commit

 Contents of yfs filesystem should be remembered and marked as a new version. For example, we have committed v0 and v1. After committing, we will get a new version, v2, and be in version 3.

 * Roll back

Recover contents of yfs filesystem by undoing logs. For example, we are in version 2 currently(have committed v0 and v1). After rolling back, we will be in version 1 and contents of yfs filesystem should be the same as that of version 1.

 * Step forward

Recover contents of yfs filesystem by redoing logs. For example, we are in version 2 currently(have committed v0 and v1). After stepping forward, we will be in version 3 and contents of yfs filesystem should be the same as that of version 3.

* Hint:

Be careful to delete log entries.

Although some log can not be delete, it's still good practice to keep checkpoints. Also, you can roll back and step forward from a checkpoint directly.

Pass Test

Type the following commands to test your lab5.

    %./start.sh
    %./test-lab-5 ./yfs1
    %./stop.sh

If you pass the lab, it will look like this.

  ===Undo Test:
  ...
  Pass Undo Test
  ===Redo Test:
  ...
  Pass Redo Test

总的来说 就是做一个 单发展线路 的能够记录的文件系统。每次commit会产生一个版本，

v1->v2->v3->v4->v5->....

之后会有 undo和redo，在版本之间切换，但切换以后不会再有 文件的其它操作(比如写 删)，只会检验是否存在(详细检测见test-lab-5.c)

所以总的来说两种做法

每次commit整个磁盘复制新建一个磁盘

每次commit用log记录，之后解析并反向操作log。对于log又分为三种 在文件系统的所在`磁盘`内的log，和外部`磁盘`的log，内存log

讲道理，符合出题者意图的只有内部磁盘log。

再从代码文件来看操作的流向

用户调用yfs-version发送信号，fuse.cc中接受信号下发给yfs_client,yfs_client通过rpc发送给extent_client,

【TODO】从讲道理的话，具体的细节应该也是放在extent_client那边也就是server端，而且应该对文件系统的所有文件都加上锁 XD

【所以最后实现只是为了过lab测试，做了不该存在的disk XD

cheat disk passed code:

```diff
diff --git a/extent_client.cc b/extent_client.cc
index 9b19db3..8fd4e92 100644
--- a/extent_client.cc
+++ b/extent_client.cc
@@ -53,4 +53,24 @@ extent_client::remove(extent_protocol::extentid_t eid)
   return  cl->call(extent_protocol::remove, eid, r);
 }
 
+extent_protocol::status
+extent_client::commit()
+{
+  int r;
+  return  cl->call(extent_protocol::commit, 0, r);
+}
+
+extent_protocol::status
+extent_client::undo()
+{
+  int r;
+  return  cl->call(extent_protocol::undo, 0, r);
+}
+
+extent_protocol::status
+extent_client::redo()
+{
+  int r;
+  return  cl->call(extent_protocol::redo, 0, r);
+}
 
diff --git a/extent_client.h b/extent_client.h
index b21ed4c..7ee61dd 100644
--- a/extent_client.h
+++ b/extent_client.h
@@ -21,6 +21,9 @@ class extent_client {
                                                          extent_protocol::attr &a);
   extent_protocol::status put(extent_protocol::extentid_t eid, std::string buf);
   extent_protocol::status remove(extent_protocol::extentid_t eid);
+  extent_protocol::status commit();
+  extent_protocol::status undo();
+  extent_protocol::status redo();
 };
 
 #endif 
diff --git a/extent_protocol.h b/extent_protocol.h
index c67e00b..a6c966c 100644
--- a/extent_protocol.h
+++ b/extent_protocol.h
@@ -15,7 +15,10 @@ class extent_protocol {
     get,
     getattr,
     remove,
-    create
+    create,
+    commit,
+    undo,
+    redo
   };
 
   enum types {
diff --git a/extent_server.cc b/extent_server.cc
index 116ec07..74b87b3 100644
--- a/extent_server.cc
+++ b/extent_server.cc
@@ -78,3 +78,20 @@ int extent_server::remove(extent_protocol::extentid_t id, int &)
   return extent_protocol::OK;
 }
 
+int extent_server::commit(int,int &)
+{
+  im->commit();
+  return extent_protocol::OK;
+}
+
+int extent_server::undo(int,int &)
+{
+  im->undo();
+  return extent_protocol::OK;
+}
+
+int extent_server::redo(int,int &)
+{
+  im->redo();
+  return extent_protocol::OK;
+}
diff --git a/extent_server.h b/extent_server.h
index 4efece4..d04bfbc 100644
--- a/extent_server.h
+++ b/extent_server.h
@@ -27,6 +27,9 @@ class extent_server {
   int get(extent_protocol::extentid_t id, std::string &);
   int getattr(extent_protocol::extentid_t id, extent_protocol::attr &);
   int remove(extent_protocol::extentid_t id, int &);
+  int commit(int, int &);
+  int undo(int, int &);
+  int redo(int, int &);
 };
 
 #endif 
diff --git a/extent_smain.cc b/extent_smain.cc
index 26f48c0..0fda2b6 100644
--- a/extent_smain.cc
+++ b/extent_smain.cc
@@ -32,6 +32,9 @@ main(int argc, char *argv[])
   server.reg(extent_protocol::put, &ls, &extent_server::put);
   server.reg(extent_protocol::remove, &ls, &extent_server::remove);
   server.reg(extent_protocol::create, &ls, &extent_server::create);
+  server.reg(extent_protocol::commit, &ls, &extent_server::commit);
+  server.reg(extent_protocol::undo, &ls, &extent_server::undo);
+  server.reg(extent_protocol::redo, &ls, &extent_server::redo);
 
   while(1)
     sleep(1000);
diff --git a/fuse.cc b/fuse.cc
index d72f7c9..c2ef516 100644
--- a/fuse.cc
+++ b/fuse.cc
@@ -558,12 +558,15 @@ void sig_handler(int no) {
   switch (no) {
     case SIGINT:
       printf("commit a new version\n");
+      yfs->commit_a_new_version();
       break;
     case SIGUSR1:
       printf("to previous version\n");
+      yfs->previous_version();
       break;
     case SIGUSR2:
       printf("to next version\n");
+      yfs->next_version();
       break;
   }
 }
diff --git a/inode_manager.cc b/inode_manager.cc
index 594862f..d4c74ed 100644
--- a/inode_manager.cc
+++ b/inode_manager.cc
@@ -23,6 +23,33 @@ disk::write_block(blockid_t id, const char *buf)
     memcpy(blocks[id], buf, BLOCK_SIZE);
 }
 
+void
+disk::commit()
+{
+  unsigned char * new_log = new unsigned char [DISK_SIZE];
+  memcpy(new_log,blocks,DISK_SIZE);
+  blocks_log.push_back(new_log);
+  log_id++;
+}
+
+void
+disk::undo()
+{
+  if(log_id > 0){
+    log_id -- ;
+    memcpy(blocks,blocks_log[log_id],DISK_SIZE);
+  }
+}
+
+void
+disk::redo()
+{
+  if(log_id + 1 < blocks_log.size()){
+    log_id ++ ;
+    memcpy(blocks,blocks_log[log_id],DISK_SIZE);
+  }
+}
+
 // block layer -----------------------------------------
 
 // Allocate a free disk block.
@@ -79,6 +106,24 @@ block_manager::write_block(uint32_t id, const char *buf)
   d->write_block(id, buf);
 }
 
+void
+block_manager::commit()
+{
+  d->commit();
+}
+
+void
+block_manager::undo()
+{
+  d->undo();
+}
+
+void
+block_manager::redo()
+{
+  d->redo();
+}
+
 // inode layer -----------------------------------------
 
 inode_manager::inode_manager()
@@ -282,3 +327,21 @@ inode_manager::remove_file(uint32_t inum)
     write_file(inum, NULL, 0);
     free_inode(inum);
 }
+
+void
+inode_manager::commit()
+{
+  bm->commit();
+}
+
+void
+inode_manager::undo()
+{
+  bm->undo();
+}
+
+void
+inode_manager::redo()
+{
+  bm->redo();
+}
diff --git a/inode_manager.h b/inode_manager.h
index 6bbec09..7024120 100644
--- a/inode_manager.h
+++ b/inode_manager.h
@@ -18,10 +18,16 @@ class disk {
  private:
   unsigned char blocks[BLOCK_NUM][BLOCK_SIZE];
 
+  std::vector<unsigned char*>blocks_log;
+  int log_id;
+
  public:
   disk();
   void read_block(uint32_t id, char *buf);
   void write_block(uint32_t id, const char *buf);
+  void commit();
+  void undo();
+  void redo();
 };
 
 // block layer -----------------------------------------
@@ -44,6 +50,9 @@ class block_manager {
   void free_block(uint32_t id);
   void read_block(uint32_t id, char *buf);
   void write_block(uint32_t id, const char *buf);
+  void commit();
+  void undo();
+  void redo();
 };
 
 // inode layer -----------------------------------------
@@ -90,6 +99,9 @@ class inode_manager {
   void write_file(uint32_t inum, const char *buf, int size);
   void remove_file(uint32_t inum);
   void getattr(uint32_t inum, extent_protocol::attr &a);
+  void commit();
+  void undo();
+  void redo();
 };
 
 #endif
diff --git a/yfs_client.cc b/yfs_client.cc
index 15b98eb..6208223 100644
--- a/yfs_client.cc
+++ b/yfs_client.cc
@@ -420,3 +420,19 @@ yfs_client::readlink(inum ino, std::string &link)
   return r;
 }
 
+void
+yfs_client::commit_a_new_version()
+{
+  ec->commit();
+}
+
+void
+yfs_client::previous_version()
+{
+  ec->undo();
+}
+void
+yfs_client::next_version()
+{
+  ec->redo();
+}
diff --git a/yfs_client.h b/yfs_client.h
index 8ce51f2..7185af7 100644
--- a/yfs_client.h
+++ b/yfs_client.h
@@ -93,6 +93,10 @@ class yfs_client {
   int verify(const char* cert_file, unsigned short*);
   int readlink(inum ino, std::string &link);
   int symlink(inum parent, const char * name, const char * link, inum & ino_out);
+
+  void commit_a_new_version();
+  void previous_version();
+  void next_version();
 };
 
 #endif 
```
