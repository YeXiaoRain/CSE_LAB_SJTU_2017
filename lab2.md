G# Lab 2: Basic File Server

## Introduction

In this lab, you will start your file system implementation by getting the following FUSE operations to work:

    CREATE/MKNOD, LOOKUP, and READDIR
    SETATTR, WRITE and READ
    MKDIR and UNLINK
    SIMBOLIC LINK

(For your own convenience, you may want to implement rmdir to facilitate your debugging/testing.)

At first, let's review the YFS architecture:

`yfs_client.cc --- put()/get() ---> extent_client.cc`

`fuse.cc --- write()/read() ---> yfs_client.cc`

In lab2, what you should concern about are the parts framed by the box above: FUSE and YFS client.

The FUSE interface, in `fuse.cc`. It translates FUSE operations from the FUSE kernel modules into YFS client calls. We provide you with much of the code needed to register with FUSE and receive FUSE operations. We have implemented all of those methods for you except for Symbol Link. **So don't modify fuse.cc unless you want to implement Symbol Link.**

The YFS client, in `yfs_client.{cc,h}`. The YFS client implements the file system logic. For example, when creating a new file, your `yfs_client` will add directory entries to a directory block.

## Getting started

先提交你的lab1的代码到本地

    make clean
    git add .
    git commit -m "finish lab1"

切换分支并且获取

    git checkout -b lab2 origin/lab2

可以采取两种方式获得 lab1完成的`inode_manager.cc`

方法1

    git checkout lab1 inode_manager.cc

方法2 [可能还要解决一下冲突问题]

    git merge lab1

我建议使用方法2

这里因为stu还不属于sudo组,所以先添加stu到sudo组

    % su -
    输入密码000
    % adduser stu sudo
    % exit

We provide you with the script start.sh to automatically start `yfs_client`, and stop.sh to kill previously started processes. Actually, start.sh starts one `yfs_client` with ./yfs1 mountpoint. Thus you can type:

    % cd lab2
    % make
    % sudo ./start.sh
    % sudo ./test-lab-2-a.pl ./yfs1
    % sudo ./test-lab-2-b.pl ./yfs1
    % sudo ./stop.sh

Note 1: Since you need to mount fuse file system, so you should add sudo to above commands;

Note 2: If stop.sh reports "Device or resource busy", please keep executing stop.sh until it says "not found in /etc/mtab", such as:

    fusermount: entry for /home/your_name/yfs-class/yfs1 not found in /etc/mtab
    fusermount: entry for /home/your_name/yfs-class/yfs2 not found in /etc/mtab
    …

This lab will be divided into 4 parts:

At the beginning, it will be helpful to scan the interfaces and structs in `yfs_client.h` and some other files. The functions you have implemented in lab1 are the fundament of this lab.

## Part 1: CREATE/MKNOD, LOOKUP, and READDIR

Your job

In Part 1 your job is to implement the CREATE/MKNOD, LOOKUP and READDIR of yfs_client.cc in YFS. You may modify or add any files you like, except that you should not modify the test scripts. Your code should pass the test-lab-2-a.pl script, which tests creating empty files, looking up names in a directory, and listing directory contents.

On some systems, FUSE uses the MKNOD operation to create files, and on others, it uses CREATE. The two interfaces have slight differences, but in order to spare you the details, we have given you wrappers for both that call the common routine createhelper(). You can see it in fuse.cc.

As before, if your YFS passes our tester, you are done. If you have questions about whether you have to implement specific pieces of file system functionality, then you should be guided by the tester: if you can pass the tests without implementing something, then you do not have to implement it. For example, you don't need to implement the exclusive create semantics of the CREATE/MKNOD operation.

### Detailed Guidance

1. When creating a new file (`fuseserver_createhelper`) or directory (`fuseserver_mkdir`), you must assign a unique inum (which you’ve done in lab1).

inum要互不相同，root的inum是1，在`yfs_client`启动时需要root的目录已经建好

2. Directory format: Next, you must choose the format for directories. A directory's content contains a set of name to inode number mappings. You should store a directory's entire content in a directory (recall what you learned). A simple design will make your code simple. You may refer to the [FAT32 specification](http://staff.washington.edu/dittrich/misc/fatgen103.pdf) or the [EXT inode design](http://en.wikipedia.org/wiki/Inode_pointer_structure) for an example to follow.The [EXT3 filesystem](http://en.wikipedia.org/wiki/Ext3) which we go after supports any characters but '\0' and '/' in the filename. Make sure your code passes when there's `'$', '_', ' '`, etc, in the filename.

选择一种文件系统格式来实现，注意特殊字符

3. FUSE:When a program (such as ls or a test script) manipulates a file or directory (such as yfs1) served by your yfs_client, the FUSE code in the kernel sends corresponding operations to yfs_client via FUSE. The code we provide you in fuse.cc responds to each such operation by calling one of a number of procedures, for create, read, write, etc. operations. You should modify the relevant routines in fuse.cc to call methods in yfs_client.cc. fuse.cc should just contain glue code, and the core of your file system logic should be in yfs_client.cc. For example, to handle file creation, fuseserver_createhelper to call yfs->create(...), and you should complete the create(...) method to yfs_client.cc. Look at getattr() in fuse.cc for an example of how a fuse operation handler works, how it calls methods in yfs_client, and how it sends results and errors back to the kernel. YFS uses FUSE's "lowlevel" API.

`用户操作->fuse->yfs`看fuse中的代码是怎么和yfs中的交互的，例如fuse中的getattr，也就要实现yfs中的create()

4. YFS code:The bulk of your file system logic should be in yfs_client, for the most part in routines that correspond to fuse operations (create, read, write, mkdir, etc.). Your fuse.cc code should pass inums, file names, etc. to your yfs_client methods. Your yfs_client code should retrieve file and directory contents from the extent client with get(), using the inum as the extent ID. In the case of directories, your yfs_client code should parse the directory content into a sequence of name/inum pairs (i.e. yfs_client::dirents), for lookups, and be able to add new name/inum pairs.

yfs主要的操作对应fuse的(create read write mkdir...),你的yfs获取文件、目录的数据是用`extent_client` 的get()和相应的extent ID,因此你的yfs需要 能解析目录中的(文件名,inum)组并有增加键值对的能力

5. A reasonable way to get going on fuse.cc is to run test-lab-2-a.pl, find the function in fuse.cc whose missing implementation is causing the tester to fail, and start fixing that function. Look at the end of yfs_client1.log and/or add your own print statements to fuse.cc. If a file already exists, the CREATE operator (fuseserver_create and fuseserver_mknod) should return EEXIST to FUSE.

一点debug方法,通`test-lab-2-a.pl`来看 哪里错了，CREATE时如果文件存在应当返回EEXIST给FUSE

6. start.sh redirects the STDOUT and STDERR of the servers to files in the current directory. For example, any output you make from fuse.cc will be written to yfs_client1.log. Thus, you should look at these files for any debug information you print out in your code.

又一些debug相关 ，STDOUT和STDERR都输出到`yfs_client1.log`

7. If you wish to test your code with only some of the FUSE hooks implemented, be advised that FUSE may implicitly try to call other hooks. For example, FUSE calls LOOKUP when mounting the file system, so you may want to implement that first. FUSE prints out to the yfs_client1.log file the requests and results of operations it passes to your file system. You can study this file to see exactly what hooks are called at every step.

FUSE最先调用LOOKUP，如果你想指导FUSE的具体的某一个hook 实现 还是看 那个log文件

### About Test

The Lab 1 tester for Part 1 is test-lab-2-a.pl. Run it with your YFS mountpoint as the argument. Here's what a successful run of test-lab-2-a.pl looks like:

    % make
    % sudo ./start.sh
    starting ./yfs_client /home/lab/Courses/CSE-g/lab1-sol/yfs1  > yfs_client1.log 2>&1 &
    % sudo ./test-lab-2-a.pl ./yfs1
    create file-yyuvjztagkprvmxjnzrbczmvmfhtyxhwloulhggy-18674-0
    create file-hcmaxnljdgbpirprwtuxobeforippbndpjtcxywf-18674-1
    …
    Passed all tests!

The tester creates lots of files with names like file-XXX-YYY-Z and checks that they appear in directory listings.

If test-lab-2-a.pl exits without printing "Passed all tests!", then it thinks something is wrong with your file server. For example, if you run test-lab-2-a.pl on the skeleton code we give you, you'll probably see some error message like this:

    test-lab-2-a: cannot create /tmp/b/file-ddscdywqxzozdoabhztxexkvpaazvtmrmmvcoayp-21501-0 : No such file or directory

This error message appears because you have not yet provided code to handle the CREATE/MKNOD operation with FUSE. That code belongs in fuseserver_createhelper in fuse.cc.

Note: testing Part 1 on the command line using commands like touch will not work until you implement the SETATTR operation in Part 2. For now, you should do your testing via the creat/open, lookup, and readdir system calls in a language like Perl, or simply use the provided test script.

Note: if you are sure that there is not any mistake in your implementation for part1 and still cannot pass this test, maybe there are some bugs in your lab1, especially read_file and write_file. Remeber that passing the test do not guarantee completely correct.

---

总结一下 Part1：

用户调用fuse,fuse调用yfsclient，yfsclient调用extentclient，extentclient调用lab1的代码

要实现 `yfs_client.cc`中的`CREATE/MKNOD, LOOKUP and READDIR`

debug 可以看log，可以看fuse.cc中的代码调用方法

如果代码正确，但测试通不过可能是lab1 中有错XD

---

然而通过grep发现mknod 是在fuse.cc中的并不是yfs的，所以这里的任务理解为要支持 fuse的 create/mknod lookup,readdir,通过读fuse.cc的代码，知道需要实现的有

```c++
int yfs_client::create(inum parent, const char *name, mode_t mode, inum &ino_out)
int yfs_client::lookup(inum parent, const char *name, bool &found, inum &ino_out)
int yfs_client::readdir(inum dir, std::list<dirent> &list)
```

我们可以调用的有下层有

```c++
extent_protocol::status create(uint32_t type, extent_protocol::extentid_t &eid);
extent_protocol::status get(extent_protocol::extentid_t eid,std::string &buf);
extent_protocol::status getattr(extent_protocol::extentid_t eid,extent_protocol::attr &a);
extent_protocol::status put(extent_protocol::extentid_t eid, std::string buf);
extent_protocol::status remove(extent_protocol::extentid_t eid);
```

返回值类型有

```
yfs_client.h:  enum xxstatus { OK, RPCERR, NOENT, IOERR, EXIST  };
```

create实现如下，其中要注意的是 文件名也可能为数字，所以在find的时候要用想办法确定找到重复的是重复的文件名而不是文件的inum

bug 样例: 创建了文件a它的inum是123，然后创建123，结果返回文件已存在，这里用单双斜杠进行分离

然后 在tutorial上是说可以按照fat或者ext来设计，有兴趣的话 你可以去根据它们的文档实现，我先鸽了XD

```c++
int
yfs_client::create(inum parent, const char *name, mode_t mode, inum &ino_out)
{
  std::string dir_content;
  if (ec->get(parent, dir_content) != extent_protocol::OK)
    return IOERR;
  if (dir_content.find("/" + std::string(name) + "//") != std::string::npos)
    return EXIST;
  if (ec->create(extent_protocol::T_FILE, ino_out) != extent_protocol::OK)
    return IOERR;
  ec->put(parent, dir_content + "/" + std::string(name) + "//" + filename(ino_out));
  return OK;
}
```

然后是lookup找文件，这里用一点小技巧，临时在目录数据的末尾添加一个斜杠

```c++
int
yfs_client::lookup(inum parent, const char *name, bool &found, inum &ino_out)
{
  found = false;
  std::string dir_content;
  if (ec->get(parent, dir_content) != extent_protocol::OK)
    return IOERR;
  dir_content+="/";
  std::string file_name = "/" + std::string(name) + "//";
  size_t st, end;
  if((st = dir_content.find(file_name)) != std::string::npos){
    found = true;
    st += file_name.size();
    end = dir_content.find_first_of("/", st);
    ino_out = n2i(dir_content.substr(st, end-st));
  }
  return OK;
}
```

最后是readdir

```c++
int
yfs_client::readdir(inum dir, std::list<dirent> &list)
{
  std::string dir_content;
  if(ec->get(dir, dir_content) != extent_protocol::OK)
    return IOERR;
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
  return OK;
}
```

这里有两个的bug，一个是文件名中如果有斜杠，我应该返回一个IOERR，另一个就是空文件名XD，不过因为不会测试这两种情况，就把这两个判断都鸽掉了XD

然后就是实现函数的过程中 个人建议是单独写一个程序出来测函数，至少我是这么干的

**如果希望代码能简洁一些，可以把实现顺序反过来 readdir，再 lookup，再create**不过我这样实现也不算复杂XD

然后测试

```bash
make
sudo ./start.sh
sudo ./test-lab-2-a.pl ./yfs1
得到输出 Passed all tests!
sudo ./stop.sh
```

至此part1 完成

### Part 2: SETATTR, READ, WRITE

In Part 2 your job is to implement SETATTR, WRITE, and READ FUSE operations in fuse.cc and yfs_client.cc. Once your server passes test-lab-2-b.pl, you are done. Please don't modify the test program or the RPC library. We will use our own versions of these files during grading.

Detailed Guidance

1. Implementing SETATTR. The operating system can tell your file system to set one or more attributes via the FUSE SETATTR operation. See fuse_lowlevel.h for the relevant definitions. The to_set argument to your SETATTR handler is a mask that indicates which attributes should be set. There is really only one attribute (the file size attribute) you need to pay attention to (but feel free to implement the others if you like), indicated by bit FUSE_SET_ATTR_SIZE. Just AND (i.e., &) the to_set mask value with an attribute's bitmask to see if the attribute is to be set. The new value for the attribute to be set is in the attar parameter passed to your SETATTR handler. The operating system may implement overwriting an existing file with a call to SETATTR (to truncate the file) rather than CREATE. Setting the size attribute of a file can correspond to truncating it completely to zero bytes, truncating it to a subset of its current length, or even padding bytes on to the file to make it bigger. Your system should handle all these cases correctly.

  SETATTR:看[fuse_lowlevel.h](https://sourceforge.net/u/noon/fuse/ci/ecd073bd7054c9e13516041e3ef930e39270c8df/tree/include/fuse_lowlevel.h)中相关的定义

`to_set`参数 是一个那些需要被设置的指示 (mask),然而你真正只需要注意文件大小的设置

```c++
/* 'to_set' flags in setattr */
#define FUSE_SET_ATTR_MODE	(1 << 0)
#define FUSE_SET_ATTR_UID	(1 << 1)
#define FUSE_SET_ATTR_GID	(1 << 2)
#define FUSE_SET_ATTR_SIZE	(1 << 3)
#define FUSE_SET_ATTR_ATIME	(1 << 4)
#define FUSE_SET_ATTR_MTIME	(1 << 5)
#define FUSE_SET_ATTR_ATIME_NOW	(1 << 7)
#define FUSE_SET_ATTR_MTIME_NOW	(1 << 8)
```

然后setattr的操作会 改变 它的长度为0/比原来小/比原来大

2. Implementing READ/WRITE:A read (fuseserver_read) wants up to size bytes from a file, starting from a certain offset. When less than size bytes are available, you should return to fuse only the available number of bytes. See the manpage for read(2) for details. For writes (fuseserver_write), a non-obvious situation may arise if the client tries to write at a file offset that's past the current end of the file. Linux expects the file system to return '\0's for any reads of unwritten bytes in these "holes" (see the manpage for lseek(2) for details). Your write should handle this case correctly.

read 和write ，具体细节参照`man 2 read`和`man 2 lseek`

关于测试

    % sudo ./start.sh
    % sudo ./test-lab-2-b.pl ./yfs1
    Write and read one file: OK
    Write and read a second file: OK
    Overwrite an existing file: OK
    Append to an existing file: OK
    Write into the middle of an existing file: OK
    Check that one cannot open non-existant file: OK
    Check directory listing: OK
    Passed all tests
    % sudo ./stop.sh

---

实现 SETATTR WRITE READ

通过`test-lab-2-b.pl`的测试【同时part1的测试也不能挂掉，这不是废话吗 外国人的脑回路真奇怪

SETATTR : 根据`to_set`来进行修改， 需要注意各种长度修改的情况

---

fuse.cc里对setattr就只处理了`to_set & FUSE_SET_ATTR_SIZE`的情况,既然tutorial也说不要改fuse.cc,那就这样吧【所以上面说那么多干嘛XD】

setattr代码如下 这里有一个不算问题的问题，需要写转换的代码吗，虽然`extent_protocol`中有`enum xxstatus { OK, RPCERR, NOENT, IOERR };`，然后`yfs_client`中也有`enum xxstatus { OK, RPCERR, NOENT, IOERR, EXIST }` 而且它们是对应的，从数值上来说不写也没错，但从意义上来说又好像需要写，当然简洁期间还是直接`return r`了，但在if判断是还是要写的【虽然不写也不会有运行错误】
``

```c++
// Only support set size of attr
int
yfs_client::setattr(inum ino, size_t size)
{
  std::string buf;
  int r;
  if((r = ec->get(ino, buf)) != extent_protocol::OK)
    return r;
  buf.resize(size);
  return ec->put(ino, buf);
}
```

read/write: (这里write最开始我用string去转化data再用replace，然而在后面的点崩了，这里还是要用for)

```c++
int
yfs_client::read(inum ino, size_t size, off_t off, std::string &data)
{
  std::string buf;
  int r;
  if((r = ec->get(ino, buf))!= extent_protocol::OK)
    return r;
  if (off < (off_t)buf.size())
    data = buf.substr(off, size);
  else
    data = "";
  return OK;
}

int
yfs_client::write(inum ino, size_t size, off_t off, const char *data,
        size_t &bytes_written)
{
  std::string buf;
  int r;
  if((r = ec->get(ino, buf)) != extent_protocol::OK)
    return r;
  if (buf.size() < off + size)
    buf.resize(off + size);
  for(unsigned int i=0;i<size;i++)
    buf[off+i]=data[i];
  if((r = ec->put(ino, buf)) != extent_protocol::OK)
    return r;
  bytes_written = size;
  return OK;
}
```

测试

```
$ sudo ./start.sh
$ sudo ./test-lab-2-b.pl ./yfs1
看到 Passed all tests
$ sudo ./stop.sh
```

至此 part2 完成

### Part 3: MKDIR and UNLINK

In Part 3 your job is to handle the MKDIR and UNLINK FUSE operations. For MKDIR, you do not have to create "." or ".." entries in the new directory since the Linux kernel handles them transparently to YFS. UNLINK should always free the file's extent; you do not need to implement UNIX-style link counts.

关于测试

If your implementation passes the test-lab-2-c.pl script, you are done with part 3. The test script creates a directory, creates and deletes lots of files in the directory, and checks file and directory mtimes and ctimes. Note that this is the first test that explicitly checks the correctness of these time attributes. A create or delete should change both the parent directory's mtime and ctime (here you should decide which level you can modify the 3 time attributes, and think about why?). Here is a successful run of the tester:

    % sudo ./start.sh
    % sudo ./test-lab-2-c .pl ./yfs1

    mkdir ./yfs1/d3319
    create x-0
    delete x-0
    create x-1
    checkmtime x-1
    ...
    delete x-33
    dircheck
    Passed all tests!

    % sudo ./stop.sh

Note: Now run the command sudo ./grade and you should pass A, B, C and E.

---

要实现 MKDIR和UNLINK,yfs和fuse对应的名称，看描述 这里的unlink实际就是删除的功能吧，然后说这里会检查文件的特种time值 a/c/m

测试要能通过A、B、C、E

代码如下

```c++
int
yfs_client::mkdir(inum parent, const char *name, mode_t mode, inum &ino_out)
{
  std::string dir_content;
  if (ec->get(parent, dir_content) != extent_protocol::OK)
    return IOERR;
  if (dir_content.find("/" + std::string(name) + "//") != std::string::npos)
    return EXIST;
  if (ec->create(extent_protocol::T_DIR, ino_out) != extent_protocol::OK)
    return IOERR;
  return ec->put(parent, dir_content + "/" + std::string(name) + "//" + filename(inum));
}
```

```c++
int
yfs_client::unlink(inum parent,const char *name)
{
  std::string dir_content;
  std::string file_name = "/" + std::string(name) + "//";
  if (ec->get(parent, dir_content) != extent_protocol::OK)
    return IOERR;
  dir_content+="/";
  size_t st0, st1 , end;
  if ((st0 = dir_content.find(file_name)) == std::string::npos)
    return NOENT;
  st1 = st0 + file_name.size();
  if((end = dir_content.find_first_of("/", st1)) == std::string::npos)
    return NOENT;
  inum ino = n2i(dir_content.substr(st1, end - st1));
  if (isdir(ino))
    return IOERR;
  dir_content.erase(st0, end - st0);
  dir_content.erase(dir_content.size()-1);
  if (ec->put(parent, dir_content) != extent_protocol::OK)
    return IOERR;
  return ec->remove(ino);
}
```

这里判断如果是 目录 则直接爆错 如果是文件才删除 XD，感觉还是可以做一个递归的[哦 我错了 我看了一下fuse的 官方的文档，删目录有专门的rmdir]，这部分代码倒没什么问题，如果出现错误估计都是lab1的代码中对不同操作的a/c/m的三个time的标志的修改的部分的代码有问题XD。

**如果你通过了ABC没有通过E，那么你可能遇到和我一样的问题，当然按照上面的过程不会发生，在我最开始实现write的时候，用了string去转化`char *`然而 这就崩了XD 要用for进行写**

### Part 4: SYMLINK, READLINK

Please implement symbolic link. To implement this feature, you should refer to the FUSE documentation available online and figure out the methods you need to implement. It's all on yourself. Also, look out for comments and hints in the hand-out code. Note: remember to add related method to fuse.cc. You may want to refer to [how to make symbolic links in fuse](http://stackoverflow.com/questions/6096193/how-to-make-symbolic-links-in-fuse) and [structfuse operations](https://fossies.org/dox/fuse-2.9.7/structfuse__operations.html).

后面这个链接没啥好看的，也就看看函数名和它默认的lib函数 就是 说fuse的上层是怎么回事，而且tutorial上给的2.9.7的链接已经过期了，我现在看到的是3.2.1也不知道改了多少XD，docker里面的版本是2.9.4,[github](https://github.com/libfuse/libfuse/blob/fuse_2_9_bugfix/lib/fuse.c)上也能找到fuse的源码

第一个stackoverflow的链接有提到 如果要做 symlink

给fuse上层的操作提供readlink函数指针`int my_readlink(const char *path, char *buf, size_t size)`

在 `my_getattr(, struct stat * stbuf)`中要

 * `stbuf->st_mod` to `S_IFLNK | 0777`
 * `stbuf->st_nlink` to `1`
 * `stbuf->st_size` to the length of the target path (don't include the string's zero terminator in the length)

[readlink](https://fossies.org/dox/fuse-3.2.1/structfuse__lowlevel__ops.html#adc02a9a897f917f69295c011bebc6fd1)

[symlink](https://fossies.org/dox/fuse-3.2.1/structfuse__lowlevel__ops.html#a3f37006d0cd3fb33dd96cb1b11087e17)

 - [fuse_reply_entry](https://fossies.org/dox/fuse-3.2.1/fuse__lowlevel_8c.html#a672c45e126cd240f4bcd59bf9b7e3708)
 - [struct fuse_entry_param](https://fossies.org/dox/fuse-3.2.1/structfuse__entry__param.html)

 也可以参照fuse其他部分的代码设置`fuse_entry_param`

```diff
diff --git a/extent_protocol.h b/extent_protocol.h
index 5ece0c2..1325393 100644
--- a/extent_protocol.h
+++ b/extent_protocol.h
@@ -19,7 +19,8 @@ class extent_protocol {

   enum types {
     T_DIR = 1,
-    T_FILE
+    T_FILE ,
+    T_SYMLINK
   };

   struct attr {
```

fuse.cc:

```diff
diff --git a/fuse.cc b/fuse.cc
index ba9dd6e..779803a 100644
--- a/fuse.cc
+++ b/fuse.cc
@@ -58,7 +58,7 @@ getattr(yfs_client::inum inum, struct stat &st)
         st.st_ctime = info.ctime;
         st.st_size = info.size;
         printf("   getattr -> %llu\n", info.size);
-    } else {
+    } else if(yfs->isdir(inum)){
         yfs_client::dirinfo info;
         ret = yfs->getdir(inum, info);
         if(ret != yfs_client::OK)
@@ -69,6 +69,18 @@ getattr(yfs_client::inum inum, struct stat &st)
         st.st_mtime = info.mtime;
         st.st_ctime = info.ctime;
         printf("   getattr -> %lu %lu %lu\n", info.atime, info.mtime, info.ctime);
+    } else {
+        yfs_client::symlinkinfo info;
+        ret = yfs->getsymlink(inum, info);
+        if(ret != yfs_client::OK)
+            return ret;
+        st.st_mode = S_IFLNK | 0777;
+        st.st_nlink = 1;
+        st.st_atime = info.atime;
+        st.st_mtime = info.mtime;
+        st.st_ctime = info.ctime;
+        st.st_size = info.size;
+        printf("   getattr -> %llu\n", info.size);
     }
     return yfs_client::OK;
 }
@@ -457,6 +469,41 @@ fuseserver_statfs(fuse_req_t req)
     fuse_reply_statfs(req, &buf);
 }

+void
+fuseserver_readlink(fuse_req_t req, fuse_ino_t ino)
+{
+    int r;
+    std::string buf;
+    if((r = yfs->readlink(ino, buf)) != yfs_client::OK){
+        fuse_reply_err(req, ENOENT);
+        return;
+    }
+    fuse_reply_readlink(req, buf.c_str());
+}
+
+void
+fuseserver_symlink(fuse_req_t req, const char *link, fuse_ino_t parent, const char *name)
+{
+  struct fuse_entry_param e;
+  e.attr_timeout  = 0.0;
+  e.entry_timeout = 0.0;
+  e.generation    = 0;
+
+  yfs_client::inum ino;
+  int r;
+  if ((r = yfs->symlink(parent, name, link, ino)) != yfs_client::OK) {
+    if (r == yfs_client::EXIST) {
+      fuse_reply_err(req, EEXIST);
+    } else {
+      fuse_reply_err(req, ENOENT);
+    }
+    return;
+  }
+  e.ino = ino;
+  getattr(ino, e.attr);
+  fuse_reply_entry(req, &e);
+}
+
 struct fuse_lowlevel_ops fuseserver_oper;

 int
@@ -499,11 +546,8 @@ main(int argc, char *argv[])
     fuseserver_oper.setattr    = fuseserver_setattr;
     fuseserver_oper.unlink     = fuseserver_unlink;
     fuseserver_oper.mkdir      = fuseserver_mkdir;
-    /** Your code here for Lab.
-     * you may want to add
-     * routines here to implement symbolic link,
-     * rmdir, etc.
-     * */
+    fuseserver_oper.readlink   = fuseserver_readlink;
+    fuseserver_oper.symlink    = fuseserver_symlink;

     const char *fuse_argv[20];
     int fuse_argc = 0;
```

`yfs_client.cc`

```diff
diff --git a/yfs_client.cc b/yfs_client.cc
index 76f503a..c3bd37a 100644
--- a/yfs_client.cc
+++ b/yfs_client.cc
@@ -52,21 +52,45 @@ yfs_client::isfile(inum inum)
     if (a.type == extent_protocol::T_FILE) {
         printf("isfile: %lld is a file\n", inum);
         return true;
-    }
-    printf("isfile: %lld is a dir\n", inum);
+    }
+    printf("isfile: %lld is not a file\n", inum);
+    return false;
+}
+
+bool
+yfs_client::issymlink(inum inum)
+{
+    extent_protocol::attr a;
+
+    if (ec->getattr(inum, a) != extent_protocol::OK) {
+        printf("error getting attr\n");
+        return false;
+    }
+
+    if (a.type == extent_protocol::T_SYMLINK) {
+        printf("issymlink: %lld is a symlink\n", inum);
+        return true;
+    }
+    printf("issymlink: %lld is not a symlink\n", inum);
     return false;
 }
-/** Your code here for Lab...
- * You may need to add routines such as
- * readlink, issymlink here to implement symbolic link.
- *
- * */

 bool
 yfs_client::isdir(inum inum)
 {
-    // Oops! is this still correct when you implement symlink?
-    return ! isfile(inum);
+    extent_protocol::attr a;
+
+    if (ec->getattr(inum, a) != extent_protocol::OK) {
+        printf("error getting attr\n");
+        return false;
+    }
+
+    if (a.type == extent_protocol::T_DIR) {
+        printf("isdir: %lld is a dir\n", inum);
+        return true;
+    }
+    printf("isdir: %lld is not a dir\n", inum);
+    return false;
 }

 int
@@ -92,6 +116,28 @@ release:
 }

 int
+yfs_client::getsymlink(inum inum, symlinkinfo &symin)
+{
+    int r = OK;
+
+    printf("getsym %016llx\n", inum);
+    extent_protocol::attr a;
+    if (ec->getattr(inum, a) != extent_protocol::OK) {
+        r = IOERR;
+        goto release;
+    }
+
+    symin.atime = a.atime;
+    symin.mtime = a.mtime;
+    symin.ctime = a.ctime;
+    symin.size = a.size;
+    printf("getsym %016llx -> sz %llu\n", inum, symin.size);
+
+release:
+    return r;
+}
+
+int
 yfs_client::getdir(inum inum, dirinfo &din)
 {
     int r = OK;
@@ -159,6 +205,21 @@ yfs_client::mkdir(inum parent, const char *name, mode_t mode, inum &ino_out)
 }

 int
+yfs_client::symlink(inum parent, const char * name, const char * link, inum & ino_out)
+{
+  std::string dir_content;
+  if (ec->get(parent, dir_content) != extent_protocol::OK)
+    return IOERR;
+  if (dir_content.find("/" + std::string(name) + "//") != std::string::npos)
+    return EXIST;
+  if (ec->create(extent_protocol::T_SYMLINK, ino_out) != extent_protocol::OK)
+    return IOERR;
+  if (ec->put(ino_out, std::string(link))!= extent_protocol::OK)
+    return IOERR;
+  return ec->put(parent, dir_content + "/" + std::string(name) + "//" + filename(ino_out));
+}
+
+int
 yfs_client::lookup(inum parent, const char *name, bool &found, inum &ino_out)
 {
   found = false;
@@ -254,3 +315,9 @@ int yfs_client::unlink(inum parent,const char *name)
   return ec->remove(ino);
 }

+int
+yfs_client::readlink(inum ino, std::string &link)
+{
+  return ec->get(ino, link);
+}
+
```

`yfs_client.h`

```diff
diff --git a/yfs_client.h b/yfs_client.h
index 78df27f..adba716 100644
--- a/yfs_client.h
+++ b/yfs_client.h
@@ -26,6 +26,12 @@ class yfs_client {
     unsigned long mtime;
     unsigned long ctime;
   };
+  struct symlinkinfo {
+    unsigned long long size;
+    unsigned long atime;
+    unsigned long mtime;
+    unsigned long ctime;
+  };
   struct dirent {
     std::string name;
     yfs_client::inum inum;
@@ -40,9 +46,11 @@ class yfs_client {
   yfs_client(std::string, std::string);

   bool isfile(inum);
+  bool issymlink(inum);
   bool isdir(inum);

   int getfile(inum, fileinfo &);
+  int getsymlink(inum, symlinkinfo &);
   int getdir(inum, dirinfo &);

   int setattr(inum, size_t);
@@ -53,8 +61,9 @@ class yfs_client {
   int read(inum, size_t, off_t, std::string &);
   int unlink(inum,const char *);
   int mkdir(inum , const char *, mode_t , inum &);
-
-  /** you may need to add symbolic link related methods here.*/
+
+  int readlink(inum ino, std::string &link);
+  int symlink(inum parent, const char * name, const char * link, inum & ino_out);
 };

 #endif
```

至此通过测试D

### 最后测试

    % ./grade.sh
    Passed A
    Passed B
    Passed C
    touch: setting times of 'yfs1/hosts_copy': Function not implemented
    Passed D
    Passed E
    Passed all tests!

    Total score: 100

Note that if you encounter a "yfs_client DIED", your filesystem is not working. In such cases the requests are served by the system's file system (usually EXT3 or btrfs or tmpfs). You would not be awarded credits if your yfs_client crashes, but could get partial credit if it produces incorrect result for some test cases. So do look out for such mistakes. We've seen dozens of students every year thinking that they've passed lots of tests before realizing this.

# TODO

add '.yfs_client.log' to GNUmakefile's `clean:
