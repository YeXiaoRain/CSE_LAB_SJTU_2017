# Lab 3: RPC

## Introduction

In this lab, you will use RPC to implement a single client file server.

前面lab实现了单个机器上的文件系统，lab3-4要实现分布式文件系统也就是RPC部分，和LAB1的相对重叠，属于底层一点的[TODO really?]

## Getting started

提交lab2的代码到本地

    % make clean
    % git add .
    % git commit -m "finish lab2"

获取切换到lab3

    % git checkout -b lab3 origin/lab3

然后 下面说的是把之前改动过的文件都拷贝过来，除了fuse.cc拷贝完以后还要注意一些改动【具体见tutorial】

我个人还是建议直接merge【我的lab1-2的总共改动文件有`extent_protocol.h, fuse.cc, inode_manager.cc, yfs_client.cc, yfs_client.h`

    % git merge lab2
    % 处理冲突
    % git add .
    % git commit -m "manual merge lab2 to lab3"

如果没有错的话，make会生成3个可执行文件: yfs_client, extent_server, test-lab-3-g

Both 32-bit and 64-bit librpc are provided, so the lab should be architecture independent.

不需要担心 server和client的崩溃，需要关心的是带有恶意或者bug的程序

### Distributed FileSystem (Strawman's Approach)

In lab2, we have implemented a file system on a single machine. In this lab, we just extend the single machine fils system to a distributed file system.

Separating extent service from yfs logic brings us a lot of advantages, such as no fate sharing with yfs client, high availability.

Luckily, most of your job has been done in the previous lab. You now can use extent service provided by extent_server through RPC in extent_client. Then a strawman distributed file system has been finished.

You had better test your code with the previous test suit before any progress.

### Detailed Guidance

In principle, you can implement whatever design you like as long as it satisfies the requirements in the "Your Job" section and passes the testers. In practice, you should follow the detailed guidance below.

Using the RPC system:

 * The RPC library. In this lab, you don't need to care about the implementation of RPC mechanisms, rather you'll use the RPC system to make your local filesystem become a distributed filesystem.

 * A server uses the RPC library by creating an RPC server object (rpcs) listening on a port and registering various RPC handlers (see main() function in demo_server.cc).

 * A client creates a RPC client object (rpcc), asks for it to be connected to the demo_server's address and port, and invokes RPC calls (see demo_client.cc).

 * You can learn how to use the RPC system by studying the stat call implementation. please note it's for illustration purpose only, you won't need to follow the implementation:use make rpcdemo to build the RPC demo

 * RPC handlers have a standard interface with one to six request arguments and a reply value implemented as a last reference argument. The handler also returns an integer status code; the convention is to return zero for success and to return positive numbers for various errors. If the RPC fails in the RPC library (e.g.timeouts), the RPC client gets a negative return value instead. The various reasons for RPC failures in the RPC library are defined in rpc.h under rpc_const.

 * The RPC system marshalls objects into a stream of bytes to transmit over the network and unmarshalls them at the other end. Beware: the RPC library does not check that the data in an arriving message have the expected type(s). If a client sends one type and the server is expecting a different type, something bad will happen. You should check that the client's RPC call function sends types that are the same as those expected by the corresponding server handler function.

 * The RPC library provides marshall/unmarshall methods for standard C++ objects such as std::string, int, and char. You should be able to complete this lab with existing marshall/unmarshall methods.

 * To simulate the distributed environment, you need to run your file system in two containers. One is for extent_server and another for yfs_client.

---

总结上面两段 :代码做了extent和yfs的分化 我们分得很好很牛逼

RPC不需要你实现，直接用提供的RPC库，client和server 可以参考`demo_*.cc` 以及GNUmakefile里面make rpcdemo

然后参数的左后一个是返回的引用，本身也会返回状态(0 成功，正错误，负超时，具体看rpc.h 的rpc_const),传输过程是流，所以需要你自己保证发送和接收端的类型一致，RPC提供相关的转换函数(string int char),但不会帮你检查收发类型一致性

这个lab要用两个container来模拟

```
rpcdemo的玩法：

container2:
    $makerpcdemo
    $ifconfig |grep inet
        inet addr:172.17.0.3  Bcast:172.17.255.255  Mask:255.255.0.0
        inet addr:127.0.0.1  Mask:255.0.0.0
    $ ./demo_server 12345

container1:
    $makerpcdemo
    $./demo_client 172.17.0.3:12345

```

---

**Configure the Network**

Go through [Network Basics in Docker](https://github.com/docker/labs/blob/master/networking/A1-network-basics.md) first.

You should be able to get the ip address of your container by docker network command now. Compare it with the result you get by running the command ifconfig directly in your container.

在你的主机上另开一个终端,基于images建立一个新的container并命名为csecontainer2

    host> docker run --name csecontainer2 -it --privileged --cap-add=ALL ddnirvana/cselab_env:latest /bin/bash

进入以后输入

    container2> ifconfig | grep inet

得到

    container2> inet addr:172.17.0.3  Bcast:172.17.255.255  Mask:255.255.0.0
    container2> inet addr:127.0.0.1  Mask:255.0.0.0

在原来的那个container里ping grep得到的地址

    container1> ping 172.17.0.3

如果输出表示连通的话就OK了

---

[我整个做的时候 container都是没有访问外部的文件的，所以 要复制代码在主机上用类似如下的命令

    host> docker cp csecontainer:/home/stu/CSE /tmp/CSE
    host> docker cp /tmp/CSE csecontainer2:/home/stu/CSE

然后 还要配置container和改文件权限

    stu> su -
    root> adduser stu sudo
    root> exit

    stu> sudo chown -R stu:root /home/stu/CSE

---

**Testers & Grading**

Your will need two containers for your testing and grading. Open two terminals, and start two containers following the above guidence. We assume the container 1 is used for yfs_client and the container 2 is used for extent server.

The test for this lab is test-lab-3-g. The test take two directories as arguments, issue operations in the two directories, and check that the results are consistent with the operations. Here's a successful execution of the tester. In the container 2(extent server container).

   % make
   % ./start_server.sh

In the container 1(yfs client container). Modify the server IP address in the start_client.sh file. change the

      EXTENT_SERVER_HOST=127.0.0.1

to , here the 172.17.0.4 is the ip address of the container2(extent server container)

      EXTENT_SERVER_HOST=172.17.0.4

    % ./start_client.sh
    starting ./extent_server 29409 > extent_server.log 2>&1 &
    starting ./yfs_client /home/cse/cse-2014/yfs1 29409 > yfs_client1.log 2>&1 &
    starting ./yfs_client /home/cse/cse-2014/yfs2 29409 > yfs_client2.log 2>&1 &
    % ./test-lab-3-g ./yfs1 ./yfs2
    Create then read: OK
    Unlink: OK
    Append: OK
    Readdir: OK
    Many sequential creates: OK
    Write 20000 bytes: OK
    test-lab-2-g: Passed all tests.
    % ./stop_client.sh

To grade this part of lab, a test script grade.sh is provided. It contains are all tests from lab2 (tested on both clients), and test-lab-3-g. Here's a successful grading. Start the server in the container 2 first

   % make
   % ./start_server.sh

In the container 1(yfs client container).
    % ./grade.sh
    Passed A
    Passed B
    Passed C
    Passed D
    Passed E
    Passed test-lab-3-g (consistency)
    Passed all tests!
    >> Lab 3 OK

---

总结就是，一个container运行`./start_server.sh` 另一个运行`./start_client.sh`和`./grade.sh`要通过最终测试

其中`./start_client.sh`中要把`EXTENT_SERVER_HOST`的值设为server的ip

虽然也有`stop_server.sh` 脚本，不过我推荐`sudo apt install htop`用htop来杀进程【个人习惯而已 可忽略本行

---

**Tips**

This is also the first lab that writes null ('\0') characters to files. The `std::string(char*)`constructor treats '\0' as the end of the string, so if you use that constructor to hold file content or the written data, you will have trouble with this lab. Use the std::string(buf, size) constructor instead. Also, if you use C-style char[] carelessly you may run into trouble!

Do notice that a non RPC version may pass the tests, but RPC is checked against in actual grading. So please refrain yourself from doing so ;)

You can use stop_server.sh in extent_server container side to stop the server, use stop_client in yfs_client container side to stop the client.

就是说string会直接把`\0`当做字符串结束了，所以为了避免这个问题请用`std::string(buf,size)`构造器，以及没有RPC的版本也许也能通过测试，但助教会测RPC的，所以确保你的使用RPC的代码也是正确的

### 惊了

精彩啊，整个lab除了给你说有些什么新工具，和环境怎么搭建，和之前的lab的tutorial一样的就不存在了。所以我们要凭空去实现这样一个带RPC的文件系统

那么先grep一下

```
grep -r "[Ll]ab3" *
extent_client.cc:  // Your lab3 code goes here
extent_client.cc:  // Your lab3 code goes here
extent_client.cc:  // Your lab3 code goes here
extent_client.cc:  // Your lab3 code goes here
...
```

所以说lab3只有三个要写代码的地方？【你以为我要说4个，不不不0,1,2,3没毛病啊

再看一下具体是什么

```
grep -r "lab3 code" * -A3 -B3
extent_client.cc-extent_client::create(uint32_t type, extent_protocol::extentid_t &id)
extent_client.cc-{
extent_client.cc-  extent_protocol::status ret = extent_protocol::OK;
extent_client.cc:  // Your lab3 code goes here
extent_client.cc-  return ret;
extent_client.cc-}
extent_client.cc-
--
extent_client.cc-extent_client::get(extent_protocol::extentid_t eid, std::string &buf)
extent_client.cc-{
extent_client.cc-  extent_protocol::status ret = extent_protocol::OK;
extent_client.cc:  // Your lab3 code goes here
extent_client.cc-  return ret;
extent_client.cc-}
extent_client.cc-
--
extent_client.cc-extent_client::put(extent_protocol::extentid_t eid, std::string buf)
extent_client.cc-{
extent_client.cc-  extent_protocol::status ret = extent_protocol::OK;
extent_client.cc:  // Your lab3 code goes here
extent_client.cc-  return ret;
extent_client.cc-}
extent_client.cc-
--
extent_client.cc-extent_client::remove(extent_protocol::extentid_t eid)
extent_client.cc-{
extent_client.cc-  extent_protocol::status ret = extent_protocol::OK;
extent_client.cc:  // Your lab3 code goes here
extent_client.cc-  return ret;
extent_client.cc-}
extent_client.cc-
```

这个函数我们前面lab是lab的作者提供的，我们没有实现过，先看一下原来的lab写的是些什么

通过```git diff lab2 extent_client.cc```可以看到，前面的lab是按照c/s模型写的，也就是调用server的过程

原来的是`es = new extent_server();`，现在的是`cl = new rpcc(dstsock);`

那么接下来问题就是怎么调用了

下面看一下`demo_server.cc`和`demo_client.cc`里的用法，client的和tutorial上说的类似，就是一个call，然后server在有函数的情况下再用`server.reg(rpc序号,服务类实例,类内函数)`注册一下即可

grep一下发现server部分的已经对应写好了

```
grep -r "server.reg" *
demo_server.cc:    server.reg(demo_protocol::stat, &ds, &demo_server::stat);
extent_smain.cc:  server.reg(extent_protocol::get, &ls, &extent_server::get);
extent_smain.cc:  server.reg(extent_protocol::getattr, &ls, &extent_server::getattr);
extent_smain.cc:  server.reg(extent_protocol::put, &ls, &extent_server::put);
extent_smain.cc:  server.reg(extent_protocol::remove, &ls, &extent_server::remove);
extent_smain.cc:  server.reg(extent_protocol::create, &ls, &extent_server::create);
```

那么client部分只需要call即可

上面tutorial说 调用的最后一个参数是引用作为数据返回，直接的return返回值是状态值，然后我去查看了一下put remove 这种它最后都是int &连参数名都省略了，学习了66666

另一个是 为什么server有4个函数注册了，然而我们只需要实现3个=。= 卧槽，原来getattr已经写好了，哇本来直接可以仿写的我好蠢XD

```diff
diff --git a/extent_client.cc b/extent_client.cc
index a2a094f..9b19db3 100644
--- a/extent_client.cc
+++ b/extent_client.cc
@@ -30,33 +30,27 @@ extent_client::getattr(extent_protocol::extentid_t eid,
 extent_protocol::status
 extent_client::create(uint32_t type, extent_protocol::extentid_t &id)
 {
-  extent_protocol::status ret = extent_protocol::OK;
-  // Your lab3 code goes here
-  return ret;
+  return cl->call(extent_protocol::create, type, id);
 }

 extent_protocol::status
 extent_client::get(extent_protocol::extentid_t eid, std::string &buf)
 {
-  extent_protocol::status ret = extent_protocol::OK;
-  // Your lab3 code goes here
-  return ret;
+  return cl->call(extent_protocol::get, eid, buf);
 }

 extent_protocol::status
 extent_client::put(extent_protocol::extentid_t eid, std::string buf)
 {
-  extent_protocol::status ret = extent_protocol::OK;
-  // Your lab3 code goes here
-  return ret;
+  int r;
+  return cl->call(extent_protocol::put, eid, buf, r);
 }

 extent_protocol::status
 extent_client::remove(extent_protocol::extentid_t eid)
 {
-  extent_protocol::status ret = extent_protocol::OK;
-  // Your lab3 code goes here
-  return ret;
+  int r;
+  return  cl->call(extent_protocol::remove, eid, r);
 }
```

最后测试【如果你没有按照前面的方法改ip配置，返回前面去改】

container2:

```
./start_server.sh
```

container1:

```
./grade.sh
```

然后通过测试，这么看来，没有遇到string的问题，【其实因为前面lab的测试已经遇到了 并修复了XD

