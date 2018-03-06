# Prepare

## [install docker-ce](https://docs.docker.com/engine/installation/linux/docker-ce/ubuntu/#set-up-the-repository)

```
sudo apt-get update
sudo apt-get install apt-transport-https ca-certificates curl software-properties-common
curl -fsSL https://download.docker.com/linux/ubuntu/gpg | sudo apt-key add -
sudo apt-key fingerprint 0EBFCD88

sudo add-apt-repository "deb [arch=amd64] https://download.docker.com/linux/ubuntu zesty stable"

sudo apt-get update
sudo apt-get install docker-ce
```

[TODO] docker config , such as usergroup

[TODO] replace git clone link

[TODO better dir design]

[这里有说 为什么 docker需要root 以及如何让docker 不需要root](https://askubuntu.com/questions/477551/how-can-i-use-docker-without-sudo)

不需要root 的使用docker 的方法，其中 `newgrp docker`也可以用登出再登入

```
sudo groupadd docker
sudo gpasswd -a $USER docker
newgrp docker
docker run hello-world
```

查看docker运行状态 `service docker status`

---

```
mkdir cselab
cd cselab
docker pull ddnirvana/cselab_env:latest
docker run --name csecontainer -it --privileged --cap-add=ALL ddnirvana/cselab_env:latest /bin/bash
# now you will enter the csecontainer environment
git clone http://ipads.se.sjtu.edu.cn:1312/lab/cse-2017.git lab
cd lab
git checkout -b lab1 origin/lab1
make
./lab1_tester
```

**令人窒息的是 我以为只要不用外面的卷 就可以不要privileged 和 capadd 然而lab2以后要在container内使用fuse都需要**

看了一下ddnirvana大佬的这个images 是ubuntu的，也就是说 你想在container搭一个更舒服的开发环境也行，不过如果你本身就用的是 linux真机还是在外面编码 里面运行比较好=。= 

[TODO] 搭一个有vim配置的images 目前个人比较推荐的是`spf13-vim`:`curl https://j.mp/spf13-vim3 -L -o - | sh`

如果你使用`spf13-vim`它有一个自动去多余末尾空格的在`~/.vimrc.before`里把`git config --global alias.vimdiff difftool`的注释"去掉，否则 你的每次会出现很多删除空格的改动XD

再次访问

首先 用`docker container ps -a` 查看所有的`container`

然后 用形如`docker start csecontainer`和`docker attach csecontainer`就不会有ddnirvana所说的 在里面的文件不见了的事情了=。= 如果你每次都是用上面的run 命令 实际是每次基于images新建立了container，所以才会有所谓的不见了。

至此已经配置好了 但是这个是prepare 所以 先exit 出来


# 关于

本目录下的`lab*.md`既是对应每一个lab的过程记录，专门把原始的分支和完成的分支分开。

[TODO] 整理/清理一下原本分支中的杂项

    lab3 grade_backup

    lab1 lab2 lab3 前面分支的评测程序清理，像lab4一样

在代码实现上 以 最小对原有代码的改动第一，易读性第二，简洁第三，效率第四 为原则进行编码。

# TODO

[MIT 6.824](https://pdos.csail.mit.edu/6.824/index.html)

目测从13年开始 lab就不再是c++ 也变了很多，开始用go语言，然后毕竟6.824的标题也是DS不是CSE，它在MIT的架构上6.824(DS)是 要先修6.004(数据结构)+ 6.033(CSE)/6.828(OS)

然后 我看6.033至少2018年是没有lab的XD,他们有一个是[DESIGN PROJECT](http://web.mit.edu/6.033/www/dp.shtml) 是要设计一个系统？？

重做每一个lab最开始的初始文件

 * 清除 多余的readme 等垃圾文件
 * 修改GNUMAKE 的clean内容
 * 修改inode_manager.cc 与lab1的 修改后的对应
 * fix 后面lab的缩进
 * 完成所有lab [6/7]

