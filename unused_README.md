写在前面
---

[TODO] 
 * `2017年02月14日16:52:30` 当前进度`10%` 刚把整个初始文件整理收集好，写好虚拟环境配置tutorial(见下),还没开始做文档移植lab的整理，由于个人情况,可能要先做OS的整理了...搁置。


# 这是啥？
 * Computer System Engineering [mit 6.033](http://web.mit.edu/6.033/www/) + [SJTU-CSE-LAB](https://github.com/SJTU-SE/awesome-se#cse-yfs-lab-mit-6033)

# 课程结构

||MIT-2015|SJTU-2015|
|---|---|---|
|hands-out|11|9|
|papers|26|7|
|LEC|26|26|
|labs|???|7(2016)|

# 课程资料(不包括lab)
 * 书:[Principles of Computer System Design: An Introduction](https://ocw.mit.edu/resources/res-6-004-principles-of-computer-system-design-an-introduction-spring-2009/) 或 清华大学出版社的`计算机系统设计原理`
 * [公开课/笔记/pdf等](https://ocw.mit.edu/courses/electrical-engineering-and-computer-science/6-033-computer-system-engineering-spring-2009/)
 * [TODO]印象中之前夏学姐有整理相关paper的项目

# lab原始mit资料

搜lab 硬是没搜到，最后发现与lab相关的(据说该lab有抄有改)
 * `Zheng Yang`大腿做的课程网页 [CSE 2015](http://plusun.github.io/)
 * 学校的官方[CSE](http://ipads.se.sjtu.edu.cn/courses/cse/)
 * `mit` 的`6.824` [mit 6.824 2013](https://pdos.csail.mit.edu/archive/6.824-2013/) [TODO] 虽然我现在还没把我们的lab 和它完全对上号
 * SJTU 的CSP的[讨论](http://ipads.se.sjtu.edu.cn/courses/qa/?/question/1640?fromuid=354&item_id=5941#!answer_5941)
 * 田生学长的文字讲解[计算机系统工程（原理）编程作业总结](http://blog.renren.com/blog/435494914/921087193#nogo)
 * [ldaochen](https://github.com/ldaochen/yfs2012)
 * [gaocegege](https://github.com/gaocegege/CSE-Labs)
 * [tcbbd](https://github.com/tcbbd/cselabs)
 * [codeworm96](https://github.com/codeworm96/cse-labs)
 * [myspring](https://github.com/mycspring/cse-lab)
 * [wizardforcel](https://github.com/wizardforcel/cselabs)

## 注意！(关于lab)
 * 请**务必** 使用虚拟机,如若在真机上出现了任何问题,概不负责。

[TODO] 内容一览 结构图

## lab 上手
 * 首先你需要一个vmware
 * 下载提供的[vm image](http://ipads.se.sjtu.edu.cn/courses/cse/lab_vm.zip)并用vmware 打开它即可 [或者你自己选个linux 自己搭fuse :-) 据说也有可行性]
 * 打开后输入用户名`cse`和密码`cselab`即可登录

### 上手问题

安装的时候源崩了! 开始以为是nameserver什么的崩了,最后发现,无论sjtu 163 还是什么源,里面的debian的squeeze都没了...有尝试升级到新版,但出了依赖问题未成功...可行解决办法[修改到老版源](https://wiki.debian.org/DebianSqueeze),具体如下[`请自行学会用vim :-)`]

  `> sudo apt edit-sources`

 注释掉163的,再添加上`deb http://archive.debian.org/debian squeeze main` 图书馆测速`700k/s~1.5M/s`

`> sudo apt-get update`

如果出现形如这样的提示`There is no public key available for the following key IDs: 8B48AD6246925553` 则运行命令

`> sudo apt-key adv --keyserver keyserver.ubuntu.com --recv-keys 8B48AD6246925553`

**其中那一串数字字母用你看到的提示里的** 完成后再`> sudo apt-get update`

接下来如文档所说 需要图形界面的小伙伴请用[讲道理 kde 越做越好看了]

```
sudo apt-get install x-window-system-core
sudo apt-get install kde-core
sudo reboot  
```

重启后输入`用户名`和`密码`登录，为了方便和主机交互，安装VMware tools
 * `Virtual Machine` 
 * -> `Install VMware Tools` 
 * -> 进入虚拟机里 点击左下角蓝色按钮搜索terminal 
 * -> 打开搜索到的`Konsole`
 * -> 输入命令`mount /media/cdrom`
 * 把`/media/cdrom`里的`VM****.tar.gz`解压 并运行`sudo ./vmware-install.pl -d`
 * 重启`sudo reboot -t now`

然后 你可以愉快的 改变虚拟机窗口大小 以及 在真机和虚拟机之间复制粘贴了

之后`build-essential` 之类,以及个人喜好的编辑器之类的就自己装去吧

### lab代码

我已经把代码迁移到[https://github.com/CroMarmot/CSE](https://github.com/CroMarmot/CSE) 下了 [TODO 完成tutorial后 更改路径]

`git clone https://github.com/CroMarmot/CSE cselab -b lab1` 即可 

`git fetch --all` 把所有branch获取到本地

`git branch -a` 查看所有branch

### 关于虚拟机里中文显示支持

以下这么多+ 一次重启 反正是可以显示了 具体是哪几条起作用并不知道 [相关文档](http://wiki.debian.org.hk/w/Make_Debian_support_Chinese_)
```
> sudo apt-get install kde-i18n-zh*
> sudo apt-get install ttf-arphic-ukai ttf-arphic-uming ttf-arphic-gbsn00lp ttf-arphic-bkai00mp ttf-arphic-bsmi00lp
> sudo dpkg-reconfigure locales
用空格选中 所有zh_CN开头的 然后回车确定 最后重启
```
# 其它建议

如果你的电脑内存大,可以在虚拟机设置里把内存也调大一些

---

如果 你的真机是Linux或者Mac,我更建议直接用ssh 不必安装图形库

登入虚拟机后,在虚拟机里输入`> ip addr show` 得到一个形如`192.*.*.*`的ip

在你的真机上用`ssh cse@192.*.*.*` 连接 密码还是`cselab`

---

可以改一下vim,git,bash的配置 让写代码更舒适

# TODO
 * 把文档也完全整理下来？
 * 做一个初始化系统的脚本？
 * 有神牛知道如何把git升级到2版本么,源里最高貌似就1.7, 难道要我去下源码make? 求告知`_(:з」∠)_`
