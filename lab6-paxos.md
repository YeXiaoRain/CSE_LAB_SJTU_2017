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
 choose n, unique and higher than any n seen so far
 send prepare(instance, n) to all servers including self
 if oldinstance(instance, instance_value) from any node:
   commit to the instance_value locally
 else if prepare_ok(n_a, v_a) from majority:
   v' = v_a with highest n_a; choose own v otherwise
   send accept(instance, n, v') to all
   if accept_ok(n) from majority:
     send decided(instance, v') to all

acceptor state:
 must persist across reboots
 n_h (highest prepare seen)
 instance_h, (highest instance accepted)
 n_a, v_a (highest accept seen)

acceptor prepare(instance, n) handler:
 if instance <= instance_h
   reply oldinstance(instance, instance_value)
 else if n > n_h
   n_h = n
   reply prepare_ok(n_a, v_a)
 else
   reply prepare_reject

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
