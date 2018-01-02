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
sudo docker run --name csecontainer -it ddnirvana/cselab_env:latest /bin/bash
# now you will enter the csecontainer environment
git clone http://ipads.se.sjtu.edu.cn:1312/lab/cse-2017.git lab -b lab1
cd lab
make
./lab1_tester
```

看了一下ddnirvana大佬的这个images 是ubuntu的，也就是说 你想在container搭一个更舒服的开发环境也行，不过如果你本身就用的是 linux真机还是在外面编码 里面运行比较好=。= 

[TODO] 搭一个有vim配置的images 目前个人比较推荐的是`spf13-vim`:`curl https://j.mp/spf13-vim3 -L -o - | sh`

[TODO] 搜能够不需要privileged

再次访问

首先 用`docker container ps -a` 查看所有的`container`

然后 用形如`docker start csecontainer`和`docker attach csecontainer`就不会有ddnirvana所说的 在里面的文件不见了的事情了=。= 如果你每次都是用上面的run 命令 实际是每次基于images新建立了container，所以才会有所谓的不见了。

至此已经配置好了 但是这个是prepare 所以 先exit 出来

