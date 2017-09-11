# A brief introduction of Docker

## Install

Please install docker by following the instructions in the: [ref](https://docs.docker.com/engine/installation/linux/docker-ce/ubuntu/)

### install in ubuntu 16.04
>sudo apt-get update
sudo apt-get install  apt-transport-https   ca-certificates  curl  software-properties-common
curl -fsSL https://download.docker.com/linux/ubuntu/gpg | sudo apt-key add -
sudo add-apt-repository   "deb [arch=amd64] https://download.docker.com/linux/ubuntu   $(lsb_release -cs)   stable"
sudo apt-get update
sudo apt-get install docker-ce

### Test docker

> sudo docker run hello-world

you will see some results like

> Hello from Docker! 
This message shows that your installation appears to be working correctly.
....



## Examples

### hello world

type the command:

> $ docker run hello-world

you will see some results like

> Hello from Docker! 
This message shows that your installation appears to be working correctly.
....


### List images

> docker images

will list all the docker image in the current machine

### Pull docker images

You can search a docker image in the DockerHub(link:https://hub.docker.com/)

When you find a image you want to use, take the `cselab_env` image(link:https://hub.docker.com/r/ddnirvana/cselab_env/) as an example, you can pull the image by

> docker pull ddnirvana/cselab_env:latest

here， `ddnirvana/sharekernel_env` is the name of the image, `:latest` is the tag of the image, which means the newest version of a image.

After the `docker pull ` success, you can see the image has been downloaded in your local machine by `docker images`  command

### Run CSELAB_ENV image

1 . create a dir in the host machine, for example
> mkdir -p /home/dd/courses/cse/lab_hostdir

2 . run a container
> docker run -it --rm -v /home/dd/courses/cse/lab_hostdir:/home/devlop ddnirvana/cselab_env:latest /bin/bash

This command means we want to run a container from the image `ddnirvana/cselab_env:latest`. `-it` means attach the container's terminal to current terminal. `--rm`  means remove the container when you exit it. ` -v /home/dd/courses/cse/lab_hostdir:/home/devlop` means mount the host dir `/home/dd/courses/cse/lab_hostdir` to the container's dir `/home/devlop`. 

**Notes**: everything in a docker container will be cleaned after you exit a container, so you need to store every usefull files in a mountted volume, in the above example, only files in `/home/devlop` in container will be persistent and every modification in these files will be persistent.