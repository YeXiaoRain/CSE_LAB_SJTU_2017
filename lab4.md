# Lab 4: Lock Server

## Introduction

In this lab, you will:

 * Implememnt a lock server

 * Add locking to ensure that concurrent operations to the same file/directory from different yfs_clients occur one at a time.

前面说lab3-4是实现分布式文件系统，lab只是实现了client和server分离，依然是只有一个client，一个server，这个lab就是 锁 了

原tutorial上的图不错！

Reference: [Distributed Systems (G22.3033-001, Fall 2009, NYU)](http://www.news.cs.nyu.edu/~jinyang/fa09/notes/ds-lec1.ppt)

## Getting Started

【[tutorial终于不是copy而是merge了

备份lab3

    git add .
    git commit -m "finish lab3"

切换分支并merge

    git checkout lab4 origin/lab4
    git merge lab3

然后处理冲突

    git add .
    git commit -m "manual merge with lab3"

冲突需要注意的是

* `yfs_client.cc,yfs_client.h`中的构造函数变了，多了lock相关的代码
* fuse的main变了

如果合并没有错的话，执行make会生成6个可执行文件`yfs_client, lock_server, lock_demo, lock_tester, test-lab-4-a, test-lab-4-b`

[TODO git比较下 lab3有librpc.a lab4没有]

Note: For this lab, you will not have to worry about server failures or client failures. You also need not be concerned about malicious or buggy applications.

### Part 1: Lock Server

这一段以及demo是讲RPC如何用的 然而我们在lab3里面已经写过了，所以难道是sjtu把别人的一个lab拆分成了两个？？不懂

**Your Job**

你的任务是，假设网络保持没有问题，实现一个正确的锁server，也就是一个时间至多只有一个client拿锁。

We will use the program lock_tester to check the correctness invariant, i.e. whether the server grants each lock just once at any given time, under a variety of conditions. You run lock_tester with the same arguments as lock_demo. A successful run of lock_tester (with a correct lock server) will look like this:

    % make
    % ./lock_server 3772 &
    % ./lock_tester 3772
    simple lock client
    acquire a release a acquire a release a
    acquire a acquire b release b release a
    test2: client 0 acquire a release a
    test2: client 2 acquire a release a
    . . .
    ./lock_tester: passed all tests successfully

If your lock server isn't correct, lock_tester will print an error message. For example, if lock_tester complains "error: server granted XXX twice", the problem is probably that lock_tester sent two simultaneous requests for the same lock, and the server granted both requests. A correct server would have granted the lock to just one client, waited for a release, and only then sent granted the lock to the second client.

Note that you only need one container for this part.

上面总结一下就是说，在一个container内要`./lock_server port &`再`./lock_tester port`输出正确就行了

**Detailed Guidance**

In principle, you can implement whatever design you like as long as it satisfies the requirements in the "Your Job" section and passes the testers. In practice, you should follow the detailed guidance below:

第一段[见原tutorial]就是lab3就本来应该看到的，结果在这里才看到【难道真的是把原来一个lab拆分成两个了？,,讲的就是rpc怎么用 做过lab3的都知道了吧

第二段：

Implementing the lock server:

 * The lock server can manage many distinct locks. Each lock is identified by an integer of type `lock_protocol::lockid_t`. The set of locks is open-ended: if a client asks for a lock that the server has never seen before, the server should create the lock and grant it to the client. When multiple clients request the same lock, the lock server must grant the lock to one client at a time.

lock以`int(lock_protocol::lockid_t)`为主键，无则创建，多client请求同一个锁 则最多一个client拥有

 * You will need to modify the lock server skeleton implementation in files `lock_server.{cc,h}` to accept acquire/release RPCs from the lock client, and to keep track of the state of the locks. Here is our suggested implementation plan.

>  server上lock两种状态，free/locked, 如果在请求时已经锁住 处理函数应当block，如果请求时是free 则获得锁(标记为锁住) 返回给client，返回值不用在意，， 在release时 就直接改变为free并且通知其它等待锁的线程，考虑用STL的`map`来记录lock的持有状态

> client :`lock_client.{cc,h}`. 提供acquire() and release() 函数接口，实现就是用RPC向server发请求. `lock_client::acquire`需要成功acquire才返回.

> 处理多线程concurrency:

> server一侧用线程池，池中空闲的线程调用RPC处理函数，来接受不同client发来的请求，client一端 不同的线程可能会调用client的`acquire() 和 release()`并行.

> 你应当用pthread mutexes 来保护threads间的数据共享. 你应当用pthread condition variables 这样lock server 的acquire 处理能等待, 说是有一个关于`pthreads,mutexes,condition variables`的link但并没看到【跟着mit 2012年的lab资料 找到的[链接](https://computing.llnl.gov/tutorials/pthreads/) ,线程应该是等待condition variable 而不是不断循环，这样线程 不会被 pthread_cond_wait() and pthread_cond_timedwait() 函数虚假的唤醒

然后为了简化代码 只需要粗粒度的mutexes即可

---

总结一下 要求，整理一下思路

前面说了 client和server都会有多线程参与，但是对于client来说，是会有多个线程调用client，也就是client端正常写首发即可

server 是需要自己开多个线程，来处理多个请求的,其中用map来记录锁的状态，用mutex来保持原子操作，[相关参考文档](https://computing.llnl.gov/tutorials/pthreads),不需要细粒度锁，一个大的锁就行

---

client: 直接copy paste `stat`的代码再小改一下就好了

```diff
diff --git a/lock_client.cc b/lock_client.cc
index 84ac08f..a45def1 100644
--- a/lock_client.cc
+++ b/lock_client.cc
@@ -30,12 +30,18 @@ lock_client::stat(lock_protocol::lockid_t lid)
 lock_protocol::status
 lock_client::acquire(lock_protocol::lockid_t lid)
 {
-       // Your lab4 code goes here
+  int r;
+  lock_protocol::status ret = cl->call(lock_protocol::acquire, cl->id(), lid, r);
+  VERIFY (ret == lock_protocol::OK);
+  return r;
 }
 
 lock_protocol::status
 lock_client::release(lock_protocol::lockid_t lid)
 {
-       // Your lab4 code goes here
+  int r;
+  lock_protocol::status ret = cl->call(lock_protocol::release, cl->id(), lid, r);
+  VERIFY (ret == lock_protocol::OK);
+  return r;
 }
```

Server:需要 首先在rpc中 注册 两个函数，让rpc能够调用

`lock_smain.cc`:

```
server.reg(lock_protocol::acquire, &ls, &lock_server::acquire);
server.reg(lock_protocol::release, &ls, &lock_server::release);`
```

然后是`lock_server.h`的 锁相关 声明


```diff
diff --git a/lock_server.h b/lock_server.h
index 82f778a..af2ce35 100644
--- a/lock_server.h
+++ b/lock_server.h
@@ -13,10 +13,13 @@ class lock_server {
 
  protected:
   int nacquire;
+  pthread_mutex_t lm;
+  pthread_cond_t  notbusy;
+  std::map<lock_protocol::lockid_t, bool> islocked;
 
  public:
   lock_server();
-  ~lock_server() {};
+  ~lock_server();
   lock_protocol::status stat(int clt, lock_protocol::lockid_t lid, int &);
   lock_protocol::status acquire(int clt, lock_protocol::lockid_t lid, int &);
   lock_protocol::status release(int clt, lock_protocol::lockid_t lid, int &);
```

最后是`lock_server.cc` 实现，虽然感觉以前写thread都没写destroy【但我胆子小 看文档里说要用，这里就加上XD】，然后就是看似没有什么卵用的nacquire，不过好像测试也真的没有检查，但是从逻辑上它还是要++--，以及map的用法，默认的值[非类的会初始化为零](http://en.cppreference.com/w/cpp/container/map/operator_at)所以 这里也就写得很简洁

```diff
diff --git a/lock_server.cc b/lock_server.cc
index 91a6112..3045206 100644
--- a/lock_server.cc
+++ b/lock_server.cc
@@ -9,6 +9,14 @@
 lock_server::lock_server():
   nacquire (0)
 {
+  pthread_mutex_init(&lm, NULL);
+  pthread_cond_init(&notbusy, NULL);
+}
+
+lock_server::~lock_server()
+{
+  pthread_cond_destroy(&notbusy);
+  pthread_mutex_destroy(&lm);
 }
 
 lock_protocol::status
@@ -23,15 +31,25 @@ lock_server::stat(int clt, lock_protocol::lockid_t lid, int &r)
 lock_protocol::status
 lock_server::acquire(int clt, lock_protocol::lockid_t lid, int &r)
 {
-  lock_protocol::status ret = lock_protocol::OK;
-       // Your lab4 code goes here
-  return ret;
+
+  pthread_mutex_lock(&lm);
+  while (islocked[lid])
+    pthread_cond_wait(&notbusy, &lm);
+  islocked[lid] = true;
+  nacquire++;
+  pthread_mutex_unlock(&lm);
+  return lock_protocol::OK;
 }
 
 lock_protocol::status
 lock_server::release(int clt, lock_protocol::lockid_t lid, int &r)
 {
-  lock_protocol::status ret = lock_protocol::OK;
-       // Your lab4 code goes here
-  return ret;
+  pthread_mutex_lock(&lm);
+  if(islocked[lid]){
+    islocked[lid] = false;
+    nacquire--;
+    pthread_cond_signal(&notbusy);
+  }
+  pthread_mutex_unlock(&lm);
+  return lock_protocol::OK;
 }
```

有一个疑问是[这里说](https://computing.llnl.gov/tutorials/pthreads/#ConVarSignal)有多个线程时应该用broadcast代替signal，【但我感觉他的意思是，比如多个线程每次减1，但增线程一次会增加很多，所以用boardcast同时通知，至少用signal/broadcast这里都是能通过的，而且这里增减都是1

```
./lock_server 3772 &
./lock_tester 3772
能看到 passed all tests successfully 即可【如果调试之前使用lock_server,记得先kill一下
```

### Part 2: Locking

Next, you are going to ensure the atomicity of file system operations when there are multiple yfs_client processes sharing a file system. Your current implementation does not handle concurrent operations correctly. For example, your yfs_client's create method probably reads the directory's contents from the extent server, makes some changes, and stores the new contents back to the extent server. Suppose two clients issue simultaneous CREATEs for different file names in the same directory via different yfs_client processes. Both yfs_client processes might fetch the old directory contents at the same time and each might insert the newly created file for its client and write back the new directory contents. Only one of the files would be present in the directory in the end. The correct answer, however, is for both files to exist. This is one of many potential races. Others exist: concurrent CREATE and UNLINK, concurrent MKDIR and UNLINK, concurrent WRITEs, etc.

You should eliminate YFS races by having yfs_client use your lock server's locks. For example, a yfs_client should acquire a lock on the directory before starting a CREATE, and only release the lock after finishing the write of the new information back to the extent server. If there are concurrent operations, the locks force one of the two operations to delay until the other one has completed. All yfs_clients must acquire locks from the same lock server.

We will use the same used in previous lab to simulate a distributed system, which means we will use three containers in Lab4 to build the whole system. The first container is used for Extent Server and the second for Lock Server, the last one is used for yfs_client(yfs server is the architecture graph).

---

上面也就是说要在原来的文件系统上 加上 我们刚刚实现的锁服务，来保证文件操作的原子性。

以及 在模拟的时候 用3个container，一个file server 一个lock server 一个client

**Your Job**

一个`client`的container

一个`extent_server`的container2

一个`lock_server`的container3

和lab3一样获取后两个server的ip，并修改client的container中`start_client.sh`的`EXTENT_SERVER_HOST`和`LOCK_SERVER_HOST`为对应的ip

然后在`extent_server`中执行 `./start_extent_server.sh`

然后在`lock_server`中执行 `./start_lock_server.sh`

最后`client`中 执行

    % ./start_client.sh
    % ./test-lab-4-a ./yfs1 ./yfs2
    % ./test-lab-4-b ./yfs1 ./yfs2

如果都OK 就完成lab了

---

**Detailed Guidance**

What to lock?

* At one extreme you could have a single lock for the whole file system, so that operations never proceed in parallel. At the other extreme you could lock each entry in a directory, or each field in the attributes structure. Neither of these is a good idea! A single global lock prevents concurrency that would have been okay, for example CREATEs in different directories. Fine-grained locks have high overhead and make deadlock likely, since you often need to hold more than one fine-grained lock.

大锁就行，细粒度也行，但细粒度锁可能会有死锁问题如果你处理不当

* You should associate a lock with each inumber. Use the file or directory's inum as the name of the lock (i.e. pass the inum to acquire and release). The convention should be that any yfs_client operation should acquire the lock on the file or directory it uses, perform the operation, finish updating the extent server (if the operation has side-effects), and then release the lock on the inum. Be careful to release locks even for error returns from yfs_client operations.

锁号直接用inum【那上面说大锁？？？】

* You'll use your lock server from part 1. yfs_client should create and use a lock_client in the same way that it creates and uses its extent_client.

* (Be warned! Do not use a block/offset based locking protocol! Many adopters of a block-id-as-lock ended up refactoring their code in labs later on).

不要基于block或者偏移量做锁，不然你后面的lab会凉凉

Things to watch out for:

This is the first lab that creates files using two different YFS-mounted directories. If you were not careful in earlier labs, you may find that the components that assign inum for newly-created files and directories choose the same identifiers.

If your inode manager relies on pseudo-randomness to generate unique inode number, one possible way to fix this may be to seed the random number generator differently depending on the process's pid. The provided code has already done such seeding for you in the main function of fuse.cc.

Tips和上一个lab一样 这里省略了，大概就是string和`\0`的问题

---

那么开始实现，首先理清流程

多个`yfs_client`调用文件系统，首先操作的过程 要申请锁，然后操作，结束后放回锁，锁的id直接使用文件的inum

也就是`yfs_client`和lockserver以及extentserver交流，而extentserver与lockserver之间并没有直接交流。

**其中要注意的是，如果yfs_client中有调用自己的函数，那么 可能发生重复锁而产生死锁的问题，比如A(),B()都是yfs_client的函数，但A()的实现中还调用了B。**

一个实现办法是 修改yfs_client的函数，让它变成public和private的分化，类似c/s分化，在public的位置加锁，调用private，private实现具体内容。

另一个实现办法是 修改lock_server,记录每一个锁的拥有者，当拥有者重复提出要锁的时候，假装分配成功。

我采用的是第一种方法。

修改过程打开`yfs_client.h`把所有public的都加上lock就好了

【这里就看到了，我在lab3更应该仿照它的函数写，因为可以说它是有预谋的，它的lab3提供的函数都是r来记录返回，用goto来跳跃到return，这样的写法 acquire和release 就每个函数只需要写一个 XD，而我每个地方return就每个地方都需要release，感觉凉凉XD】

实现的变更 因为太多了，这里就不贴出，直接看我的commit变化就好

---

测试：

在container里先make

    csecontainer> make

新建一个container

    host> docker run --name csecontainer3 -it --privileged --cap-add=ALL ddnirvana/cselab_env:latest /bin/bash
    stu> su -
    root> adduser stu sudo
    root> exit

打开lab3建立过的csecontainer2,在2中删除原来的

    host> docker start -i csecontainer2
    csecontainer2> sudo rm -rf /home/stu/CSE

通过docker和主机复制文件到两个container中，【这样看的话，用mit的 卷映射就没有复制文件的麻烦 XD

    host> docker cp csecontainer:/home/stu/CSE /tmp/CSE
    host> docker cp /tmp/CSE/ csecontainer2:/home/stu/CSE
    host> docker cp /tmp/CSE/ csecontainer3:/home/stu/CSE

在container2和3中查看ip

    csecontainer2/3 > ifconfig | grep inet

在csecontainer修改配置文件`start_client`为对应的ip 例如

    EXTENT_SERVER_HOST=172.17.0.4
    LOCK_SERVER_HOST=172.17.0.3

这里我的extentserver用container2，而lockserver用container3

    csecontainer2> sudo chown -R stu:root /home/stu/CSE
    csecontainer2> cd /home/stu/CSE
    csecontainer2> ./start_extent_server.sh

    csecontainer2> sudo chown -R stu:root /home/stu/CSE
    csecontainer2> cd /home/stu/CSE
    csecontainer2> ./start_lock_server.sh

再回到最初的container

    container>./start_client.sh
    container>./test-lab-4-a ./yfs1 ./yfs2
    Create then read: OK
    Unlink: OK
    Append: OK
    Readdir: OK
    Many sequential creates: OK
    Write 20000 bytes: OK
    Concurrent creates: OK
    Concurrent creates of the same file: OK
    Concurrent create/delete: OK
    Concurrent creates, same file, same server: OK
    Concurrent writes to different parts of same file: OK
    test-lab-4-a: Passed all tests.
    container>./test-lab-4-b ./yfs1 ./yfs2
    Create/delete in separate directories: tests completed OK
    container>./stop_client.sh

通过了测试

# 其它

如果你玩多个container分不清那个是那个，可以去改一下~/.bashrc的输出

part2说着很简单，结果很愚蠢的错误,我调了很久，XD 如果有bug的话，建议通过输出和看log定位XD，这样能调试得快一些。
