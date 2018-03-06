# Lab 6: Paxos Protocol

[TODO clean old trash file]

根据天神学长的文档貌似在很久很久以前 是有这个lab的，然后也不知道从哪年开始 至少12级是没有的，一直到14级的lab里都没有这个了。不过作为课上的presentation的paper阅读，每一级还是都学了paxos的paper，然而不写代码一直没法通透的理解paxos,2015以后的lab中又有了paxos 可喜可贺 可喜可贺

## Introduction

实现[Paxos](https://ipads.se.sjtu.edu.cn/courses/cse/2017/labs/lab6.html) 并用它 同意 关系改变序列(i.e., view changes). 作者改动了`lock_smain.cc` 来运行RSM 取代 lock server;实际上这个lab, `lock_server processes` 为configuration servers提供服务

已经做了一个手动翻译 见下方 附加文档部分

当你完成这个lab，你会有一个复制状态机 ，能管理一组 lock servers. Nodes 能够被加入到这些 副本组中, 它会联系master并 请求加入group. Nodes 也能在它们发生错误时从副本组中移除. 组中nodes的集合在一个时刻是一个view, 在每一次view改变时，你会运行Paxos来 同意 新的view.

设计上我们给你三层模块，RSM 和 config 层 发出下层调用 告诉下层的 需要做什么. config 和 Paxos 模块 同时也向上 调用，通知上层 重要的事件 (e.g., Paxos 同意了一个值, 或者一个node 不可到达).

RSM顶->config中->paxos底

**RSM module**

RSM模块负责 复制，当一个node加入，RSM 模块 指导 config 模块添加node.

RSM 模块也在view中每一个node上运行恢复线程保证他们的一致性.这个lab中 唯一需要recover的状态是 the sequence of Paxos operations that have been executed.(保持跟上更新？解决stuck？)

**config module**

config模块管理view. 当RSM 模块让它增加node到当前的view时，它调用Paxos来同意一个新的view.

config模块也发送 周期性的heartbeats 来检测是否有挂掉的nodes 并移除挂掉的node，移除过程也是调用Paxos 来同意一个新的view.

**Paxos module**

paxos负责运行paxos来同意值，在principle中 值可以是任何东西. 在我们的这个system(lab)中，这个值是 由nodes组中的 组成view的集合. The focus of this lab is on the Paxos module. What you need to modify are all in paxos.cc.

## Getting Started

开工

    % git commit -am 'my solution to lab5'
    % git checkout -b lab6 origin/lab6
    % git merge lab5

处理冲突，除了 GNUmakefile and lock_smain.cc，其它的都按照lab5 之前的实现为准.

如果你已经记不得哪里是你写的 哪里是ta写的，我想说这个冲突是真的多，那么请使用`git blame 文件名`可以看到最后的改动人

然后个人建议vimdiff，[目前学长 调了很多space XD，所以 在diff的时候可以`set diffopt+=iwhite`

[TODO 之后的想法当然是fix掉这些版本间的自带的冲突 2333]

然后 现在有些新文件

* `paxos_protocol.h`
* `paxos.{cc,h}`
* `log.{cc,h}`
* `rsm_tester.pl`
* `config.{cc,h}`
* `rsm.{cc,h}`
* `rsm_protocol.h`
* `handle.{cc,h}`

`lock_smain.cc`被修改 用于在lock server启动时 初始化RSM模块.在这个lab, we disable lock server you have implemented in lab4 and run rsm instead. 现在的lock server 接受两个命令行参数the port that the master and the port that the lock server you are starting should bind to. see rsm_tester.pl for detail.

`rsm.{cc,h}`中提供了 (用于设置适当的RPC 处理函数 以及 管理恢复) 的代码.

`paxos.{cc,h}`中 你会找到acceptor and proposer类的框架(也就是用于Paxos protocol to agree on view changes). `paxos_protocol.h`定义了在Paxos不同副本实例之间 建议的RPC protocol, 包括有 参数的结构体 和 返回类型, and marshall code for those structures.

`log.{cc,h}`提供了完整的log类的实现，你的acceptor and proposer应当调用这个类来进行 重要的log. 它可以用来记录log 对recovery过程游泳. **不要修改这两个文件 因为这两个文件同时用于测试**
`config.cc` 使用paxos来管理 views，你需要裂解它如何和 paxos以及rsm交互的，但不需要改动。

`handle.cc` 管理RPC 链接 的 cache. 如果第一个 call 成功的绑定了rpc server.接下来所有的call 将会返回同样的 rpcc object.`handle.h`有一个如何使用的example

`rsm_tester.pl`测试工具 为自动运行 一些lock servers, 杀死 再 重启 其中一些，然后来检测你是否正确的实现并使用paxos

## Your Job

最后就是要通过`./rsm_tester.pl 0 1 2 3 4 ` 的测试

测试运行3个lock servers，杀死再重启其中的一些，测试log

Important: 如果在测试运行中fail了，记得清理 中间产生的文件 以及杀掉所有`lock_server`: `If rsm_tester.pl fails during the middle of a test, the remaining lock_server processes are not killed and the log files are not cleaned up (so you can debug the causes.). Make sure you do 'killall lock_server; rm -f *.log' to clean up the lingering processes before running rsm_tester.pl again.`

## Detailed Guidance

### Step One: Implement Paxos

对着[伪代码](https://ipads.se.sjtu.edu.cn/courses/cse/2017/labs/lab6-ref.html)把`paxos.cc`实现.Do not worry about failures yet.

使用`paxos_protocol.h`中提供的RPC协议.

**为了通过测试，当proposer发送一个RPC, 你应该设置RPC的超时为1000毫秒.**

尽管伪代码 展示了对每一个RPC的 不同类型的返回，我们的protocol把这些返回 整合为返回结构体，例如，`prepareres`结构体 可以用于`prepare_ok`,`oldinstance`,or `reject`.

debug看`paxos-[port].log` 它是由log.cc写出的。

`configuration`的stdout和stderr被重定向到`lock_server-[arg1]-[arg2].log`中.

如果你实现了paxos，你应该能通过测试`rsm_tester.pl 0`,这个测试只检测 打开三个server是否到了同一个view中

---

然后看paxos.cc 从run看起，然后发现run是基本写好了。

读了一下它的majority 实际就是 看选中的是否 大于 总量的一半，然后在accept过程只发送给 prepare 响应成功的

然后在我写过程比较有参考性的，当然前提是前面merge没有merge出问题

config.cc(让你知道run是怎么被调的,这里 上层要发扬binary is everything/everything is a file的精神 以 everything is a string来搞 所以不用在意v的细节就好了)

paxos.h + paxos_protocol.h已经提供的类 结构体 函数

handle.h教你怎么用handle,然后用call的时候自己的函数不论结果是什么在return部分都是OK，ERR是由宕机之类的return的不用管

**然后着重说一下，伪代码和代码之间 的函数分化是不一样的，比如伪代码中run的一些步骤，在实际的代码中是被分配到prepare当中,而伪代码的prepare实际对应的是preparereq**，所以还需要 自己把伪代码 的划分调一下

### Step Two: Simple failures

用`rsm_tester.pl 0 1 2` 测试你的paxos是否 处理了简单的failures,如果你的代码是正确的，你不需要添加任何代码

### Step Three: Logging Paxos state

修改你的paxos实现，加上log的调用，记录`n_h`, `n_a` and `v_a`的改变when they are updated.理解 为什么这三个值需要被记录遭磁盘上. log.cc 中提供相关的读/写函数(log::logprop(), and log::logaccept()), 所以你要做的只是在正确的时候调用.

然后 就可以测试 能否有node先挂掉再重启`rsm_tester.pl 3 4`.原文档描述了test 4的细节[1 2 3]--杀3-->[1 2]--杀2-->因为没有满足majority所以还是[1 2]--启动3--依然不满足--杀3--启动2--恢复正常--启动3--变为[1 2 3]

所有改动实现如下

```diff
diff --git a/paxos.cc b/paxos.cc
index 697babe..12a8e00 100644
--- a/paxos.cc
+++ b/paxos.cc
@@ -150,10 +150,27 @@ proposer::prepare(unsigned instance, std::vector<std::string> &accepts,
          std::vector<std::string> nodes,
          std::string &v)
 {
-  // You fill this in for Lab 6
-  // Note: if got an "oldinstance" reply, commit the instance using
-  // acc->commit(...), and return false.
-  return false;
+  prop_t highest_n_a={0,""};
+  for(auto item : nodes){
+    handle h(item);
+    if(h.safebind()){
+      paxos_protocol::preparearg a={instance,my_n};
+      paxos_protocol::prepareres r;
+      if(h.safebind()->call(paxos_protocol::preparereq, me, a, r, rpcc::to(1000)) == paxos_protocol::OK) {
+        if (r.oldinstance) {
+          acc->commit(instance, r.v_a);
+          return false;
+        }else if (r.accept){
+          accepts.push_back(item);
+          if (r.n_a > highest_n_a) {
+            highest_n_a = r.n_a;
+            v = r.v_a;
+          }
+        }
+      }
+    }
+  }
+  return true;
 }
 
 // run() calls this to send out accept RPCs to accepts.
@@ -162,14 +179,28 @@ void
 proposer::accept(unsigned instance, std::vector<std::string> &accepts,
         std::vector<std::string> nodes, std::string v)
 {
-  // You fill this in for Lab 6
+  for (auto item : nodes) {
+    handle h(item);
+    paxos_protocol::acceptarg a={instance, my_n, v};
+    bool r;
+    if(h.safebind())
+      if(h.safebind()->call(paxos_protocol::acceptreq, me, a, r, rpcc::to(1000)) == paxos_protocol::OK)
+        if(r)
+          accepts.push_back(item);
+  }
 }
 
 void
 proposer::decide(unsigned instance, std::vector<std::string> accepts, 
              std::string v)
 {
-  // You fill this in for Lab 6
+  paxos_protocol::decidearg a={instance, v};
+  for (auto item : accepts) {
+    handle h(item);
+    int r;
+    if(h.safebind())
+      h.safebind()->call(paxos_protocol::decidereq, me, a, r, rpcc::to(1000));
+  }
 }
 
 acceptor::acceptor(class paxos_change *_cfg, bool _first, std::string _me, 
@@ -202,20 +233,32 @@ paxos_protocol::status
 acceptor::preparereq(std::string src, paxos_protocol::preparearg a,
     paxos_protocol::prepareres &r)
 {
-  // You fill this in for Lab 6
-  // Remember to initialize *BOTH* r.accept and r.oldinstance appropriately.
-  // Remember to *log* the proposal if the proposal is accepted.
+  r.oldinstance = false;
+  r.accept = false;
+  if (a.instance <= instance_h) {
+    r.oldinstance = true;
+    r.v_a = values[a.instance];
+  } else if (a.n > n_h) {
+    n_h = a.n;
+    r.n_a = n_a;
+    r.v_a = v_a;
+    r.accept = true;
+    l->logprop(n_h);
+  }
   return paxos_protocol::OK;
-
 }
 
 // the src argument is only for debug purpose
 paxos_protocol::status
 acceptor::acceptreq(std::string src, paxos_protocol::acceptarg a, bool &r)
 {
-  // You fill this in for Lab 6
-  // Remember to *log* the accept if the proposal is accepted.
-
+  r = false;
+  if (a.n >= n_h) {
+    n_a = a.n;
+    v_a = a.v;
+    r = true;
+    l->logaccept(n_a, v_a);
+  }
   return paxos_protocol::OK;
 }
```

### TODO

原来lab里的paxos.cc的缩进真的蛋疼 ，等有时间整理一下这个

---

# 附加文档(上面的那些链接)

# 理解paxo如何用来 view change

有两个同时实现的 paxos协议 acceptor和proposer，每个副本都会运行这两个类

proposer 提出新的 value并发送给所有 副本

acceptor 处理所有从proposer发送来的请求并回应

proposer::run(nodes,v)用来让当前view(nodes)同意的一个值(v)

当一个instance完成agreement完成了，acceptor会调用config's `paxos_commit(instance,v)`，v为同意的值

如下面描述的，不同的nodes/view 可能提出不同的值，也许和实际选中的值不同。实际上 paxos 如果不能让majority来接收 它的prepare或accept消息时，paxos会中止

---

config 模块 用来在所有参与的nodes中展示 view change。 最初的 view是手工配置的,之后就是靠paxos同意的一个具体的view来取代之前的view

最初，第一个node 创建view 1只包含它自己：`view_1={1}`,当第二个点加入，当第二个`RSM`模块加入node 1,并从node 1获得view 1，node 2就会询问config 模块来添加它自己到view 1中.config 模块会使用paxos 来保证所有在view_1的nodes变成`view_2={1,2}`党paxos成功，view_2就形成了。当node 3加入,它的RSM 模块会先下载最后的view 从第一个node(view 2)并且他会尝试让view 2中的node的view变成`view_3 = {1,2,3}`

如果config模块发现当前的view中有没有响应的模块，它也会发出view 改变。特别的 最小的id的node给其它所有node发送 heartbeat(其它的node 都发heartbeat给这个最小id的)如果一个rpc超时，config模块调用`proposer`的run(nodes,v) 来开始新的一轮paxos protocol，因为每个节点独立决定是否运行paxos是否运行，因此可能同时有多个paxos并行运行，paxos sorts that out correctly

proposer 保持跟踪当前的view 是否是稳定的 使用`proposer::stable`,如果不稳定 会就开始paxos protocol 改变 view(view change).

acceptor 在磁盘上记录重要的paxos事件——完整的历史的同意过的值。在任何时刻，一个node 能重启 ，当它重新加入时，它可能落后views很多，它需要先完成更新到当前view，之后才能加入 paxos的工作。通过记录views 其它nodes能够带重启的node 更到最新

# The Paxos Protocol

The [Paxos Made Simple paper](https://ipads.se.sjtu.edu.cn/courses/cse/2017/labs/lab6/paxos-simple.pdf) 介绍了一个协议 同意一个值 然后 终止. 因为我们想要在每次view 改变时运行另一个 paxos实例, 我们需要保证 不同的实例的messages不会冲突. 我们通过给所有messages加上instance numbers来保证(which are not the same as proposal numbers).因为我们使用paxos 来同意view 改变, 实例号和我们Paxos中使用的view号一样.

paxos不能保证 每一个node学习到的选择的value是正确的，其中有部分的可能是 不对的(partitioned or crashed). 所以有的nodes 会落后，卡在一个旧的paxos实例中. 如果一个 node's acceptor 得到了一个旧的实例的RPC请求,它应当返回给proposer with a special RPC response (set oldinstance to true). 这个response 通知这个proposer它落后了，并告诉它正确的值.

下面是paxos的伪代码. acceptor and proposer skeleton classes contain member variables, RPCs, and RPC handlers corresponding to this code. 除了上面说的 增加实例的处理, it mirrors the protocol described in the paper.

```
proposer run(instance, v):
 选一个n ＞ 目前所见过的所有n
 发送prepare(instance, n)给包括自己的所有servers
 if 任何一个节点 返回 oldinstance(instance, instance_value):
   把instance_value提交到本地 (跟上更新)
 else if 多数派prepare_ok(n_a, v_a) :
   v' = v_a with highest n_a; choose own v otherwise
   send accept(instance, n, v') to all
   if accept_ok(n) from majority:
     send decided(instance, v') to all

acceptor state:
 必须要是就算重启了也持久的，也就是要写在disk里
 n_h 见过的最大的请求号
 instance_h, 最高的accepted的instance // 不用写在disk里 因为这种时后 恢复是通过其它nodes传过来的 instance
 n_a, v_a (highest accept seen)

acceptor prepare(instance, n) handler:
 if instance <= instance_h // proposer的 版本落后 <= ? 提供最新版本的instance_value
   reply oldinstance(instance, instance_value)
 else if n > n_h // 新的提案 同意
   n_h = n
   reply prepare_ok(n_a, v_a)
 else
   reply prepare_reject // 拒绝

acceptor accept(instance, n, v) handler:
 if n >= n_h
   n_a = n
   v_a = v
   reply accept_ok(n)
 else
   reply accept_reject

acceptor decide(instance, v) handler:
 paxos_commit(instance, v)
```

对一个给的paxos的实例，潜在的很多nodes都能提出proposals,每一个proposals都有一个不同的proposal number. 当比较proposals时采用 number大的. 为了保证不同proposals number唯一,每一个proposal 都包含一个node的标识符. 我们提供了一个`struct prop_t` in `paxos_protocol.h` 用于 proposal numbers; 同时提供了类的`> and >=`比较运算.

每一个副本 必须记录到它Paxos的状态的明确改变(in particular the n_a, v_a, and n_h fields),以及每一个 同意ed的值. 已经提供log类 能帮你做这些事; 请不要修改的使用它，因为测试系统是基于它的输出的.

给你的RPC calls 增加一个额外的参数 `rpcc::to(1000)`来保护RPC library 画长时间连接一个崩溃的nodes.
