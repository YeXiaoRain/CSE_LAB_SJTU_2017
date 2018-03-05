# Lab 6: Paxos Protocol

[TODO clean old trash file]

## Introduction

实现[Paxos](https://ipads.se.sjtu.edu.cn/courses/cse/2017/labs/lab6.html) 并用它 同意 关系改变序列(i.e., view changes). 作者改动了`lock_smain.cc` 来运行RSM 取代 lock server;实际上这个lab, `lock_server processes` 为configuration servers提供服务

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

Important: 如果在测试运行中fail了，记得清理 中间产生的文件 以及杀掉所有`lock_server`: If rsm_tester.pl fails during the middle of a test, the remaining lock_server processes are not killed and the log files are not cleaned up (so you can debug the causes.). Make sure you do 'killall lock_server; rm -f *.log' to clean up the lingering processes before running rsm_tester.pl again.

## Detailed Guidance

### Step One: Implement Paxos

对着[伪代码](https://ipads.se.sjtu.edu.cn/courses/cse/2017/labs/lab6-ref.html)把`paxos.cc`实现.Do not worry about failures yet.

使用`paxos_protocol.h`中提供的RPC协议.

**为了通过测试，当proposer发送一个RPC, 你应该设置RPC的超时为1000毫秒.**

** Note that though the pseudocode shows different types of responses to each kind of RPC, our protocol combines these responses into one type of return structure. For example, the prepareres struct can act as a prepare_ok, an oldinstance, or a reject message, depending on the situation.

You may find it helpful for debugging to look in the paxos-[port].log files, which are written to by log.cc. rsm_tester.pl does not remove these logs when a test fails so that you can use the logs for debugging. rsm_tester.pl also re-directs the stdout and stderr of your configuration server to lock_server-[arg1]-[arg2].log.

Upon completing this step, you should be able to pass 'rsm_tester.pl 0'. This test starts three configuration servers one after another and checks that all servers go through the same three views.

Step Two: Simple failures
Test whether your Paxos handles simple failures by running 'rsm_tester.pl 0 1 2'. You will not have to write any new code for this step if your code is already correct.

Step Three: Logging Paxos state
Modify your Paxos implementation to use the log class to log changes to n_h, and n_a and v_a when they are updated. Convince yourself why these three values must be logged to disk if we want to re-start a previously crashed node correctly. We have provided the code to write and read logs in log.cc (see log::logprop(), and log::logaccept()), so you just have to make sure to call the appropriate methods at the right times.

Now you can run tests that involve restarting a node after it fails. In particular, you should be able to pass 'rsm_tester.pl 3 4 '. In test 4, rsm_tester.pl starts three servers, kills the third server (the remaining two nodes should be able to agree on a new view), kills the second server (the remaining one node tries to run Paxos, but cannot succeed since no majority of nodes are present in the current view), restarts the third server (it will not help with the agreement since the third server is not in the current view), kills the third server, restarts second server (now agreement can be reached), and finally restarts third server.

