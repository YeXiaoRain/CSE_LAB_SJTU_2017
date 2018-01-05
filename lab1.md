# lab1

在已经配置好docker 以及 在docker 的环境中下好了 lab的代码，下面先回到 环境中

```
docker container ps -a
docker start csecontainer
docker attach csecontainer
```

# This lab will be divided into 3 parts.

Before you write any code, we suggest that you should read `inode_manager.h` first and be familiar with all the classes. We have already provide you some useful functions such as `get_inode` and `put_inode`.

 * In part 1, you should implement `disk::read_block` and `disk::write_block` `inode_manager::alloc_inode` and `inode_manager::getattr`, to support `CREATE` and `GETATTR` APIs. Your code should pass the `test_create_and_getattr()` in `lab1_tester`, which tests creating empty files, getting their attributes like type.

 * In part 2, you should implement `inode_manager::write_file`, `inode_manager::read_file`, `block_manager::alloc_block`, `block_manager::free_block`, to support `PUT` and `GET` APIs. Your code should pass the `test_put_and_get()` in `lab1_tester`, which, write and read files.

 * In part 3, you should implement `inode_manager::remove_file` and `inode_manager::free_inode`, to support `REMOVE` API. Your code should pass the `test_remove()` in `lab1_tester`.

In this lab, you should only need to make changes to `inode_manager.cc`. (Although you are allowed to change many other files, except those directly used to implement tests.) Although maybe we won't check all the corner case, you should try your best to make your code robust. It will be good for the coming labs.

# Part 1: CREATE/GETATTR

Your job in Part 1 is to implement the `read_block` and `write_block` of disk and the `alloc_inode` and `getattr` of `inode_manager`, to support the `CREATE` and `GETATTR` APIs of `extent_server`. You may modify or add any files you like, except that you should not modify the `lab1_tester.cc`. (Although our sample solution, for lab1, contains changes to `inode_manager.cc` only.)

The tips can be found on the codes of `inode_manager.[h|cc]`. Be aware that you should firstly scan through the code in `inode_manager.h`, where defines most of the variables, structures and macros you can use, as well as the functions `get_inode` and `put_inode` of `inode_manager` I leave to you to refer to.

Meanwhile, pay attention to one of the comments in `inode_manager.c`:

    // The layout of disk should be like this:
    // |<-sb->|<-free block bitmap->|<-inode table->|<-data->|

It may be helpful for you to understand most of the process of the data access. After you finish these 4 functions implementation, run:

    % make
    % ./lab1_tester

You should get following output:

    ========== begin test create and getattr ==========
    …
    …
    ========== pass test create and getattr ==========
    ========== begin test put and get ==========
    …
    …
    [TEST_ERROR] : error …
    --------------------------------------------------
    Final score is : 40

---

通过阅读代码`inode_manager.h`先知道整个结构 为`inode_manager`->`block_manager`->`disk`其中disk用的是数组模拟

其中注意的是 disk的分化要按照
` inode_manager.c:// |<-sb->|<-free block bitmap->|<-inode table->|<-data->|`

Part1 的任务 是

    disk:
    void read_block(uint32_t id, char *buf);
    void write_block(uint32_t id, const char *buf);
    inode_manager:
    int32_t alloc_inode(uint32_t type);
    oid getattr(uint32_t inum, extent_protocol::attr &a);

disk的两个函数跟着注释实现即可

```c++
void
disk::read_block(blockid_t id, char *buf)
{
    if(id < 0 || id >= BLOCK_NUM || buf == NULL)
        return ;
    memcpy(buf, blocks[id], BLOCK_SIZE);
}

void
disk::write_block(blockid_t id, const char *buf)
{
    if (id < 0 || id >= BLOCK_NUM || buf == NULL)
        return;
    memcpy(blocks[id], buf, BLOCK_SIZE);
}
```

`alloc_inode`参考下面的`get_inode`的实现,其中要注意的是 一个`BLOCK_SIZE`里面存的是IPB个inode,所以tcbbd的代码实现是半错不错的XD。 其次 虽然不算错，但是个人建议 两处使用`bm->`而不是宏里面的`INODE_` 这里稍微表现一下 层级关系

```c++
uint32_t
inode_manager::alloc_inode(uint32_t type)
{
    char buf[BLOCK_SIZE];
    for(uint32_t inum = 1; inum <= bm->sb.ninodes; inum++) {
        bm->read_block(IBLOCK(inum, bm->sb.nblocks), buf);
        struct inode * ino_disk = (struct inode*)buf + inum%IPB;
        if (ino_disk->type == 0) {
            ino_disk->type = type;
            ino_disk->size  = 0;
            ino_disk->ctime = time(NULL);
            bm->write_block(IBLOCK(inum, bm->sb.nblocks), buf);
            return inum;
        }
    }
    return 0;
}
```

然后是GETATTR 很直接,要注意的是 阅读一下`get_inode`的代码 其中返回的是malloc 出来的 所以请free掉

```c++
inode_manager::getattr(uint32_t inum, extent_protocol::attr &a)
{
    struct inode * ino = get_inode(inum);
    if(ino){
        a.type  = ino->type ;
        a.size  = ino->size ;
        a.atime = ino->atime;
        a.mtime = ino->mtime;
        a.ctime = ino->ctime;
        free(ino);
    }
}
```

至此part 1 完成40分到手

# Part 2 PUT/GET

implement

`void inode_manager::read_file(uint32_t inum, char **buf_out, int *size)`

`void inode_manager::write_file(uint32_t inum, const char *buf, int size)`

`blockid_t block_manager::alloc_block()`

`void block_manager::free_block(uint32_t id)`

You should pay attention to the indirect block test. In our inode manager, each file has only one additional level of indirect block, which means one file has 32 direct block and 1 indirect block which point to a block filled with other blocks id.

After you finish these 4 functions implementation, run:

    % make
    % ./lab1_tester

You should get following output:

    ========== pass test create and getattr ==========
    …
    ========== pass test put and get ==========
    …
    Final score is : 80

根据注释中 说的 判断一下是否有indirect的块 即可

```c++
/* Get all the data of a file by inum.�
 * Return alloced data, should be freed by caller. */
void
inode_manager::read_file(uint32_t inum, char **buf_out, int *size)
{
    char buf[BLOCK_SIZE];
    inode_t * ino = get_inode(inum);
    if( !ino ){
        *size = 0;
        *buf_out = (char *)malloc(0);
        return ;
    }
    unsigned int offset = 0;
    char * ret = (char *)malloc(ino->size);

    for (int i = 0; i < NDIRECT && offset < ino->size; i++) {
        int len = MIN(ino->size - offset , BLOCK_SIZE);
        bm->read_block(ino->blocks[i], buf);
        memcpy(ret + offset, buf, len);
        offset += len;
    }

    if (offset < ino->size) {
        char indirect[BLOCK_SIZE];
        bm->read_block(ino->blocks[NDIRECT], indirect);
        for (unsigned int i = 0; i < NINDIRECT && offset < ino->size; i++) {
            blockid_t id = *((blockid_t *)indirect + i);
            int len = MIN(ino->size - offset , BLOCK_SIZE);
            bm->read_block(id, buf);
            memcpy(ret + offset, buf, len);
            offset += len;
        }
    }
    *size = offset;
    *buf_out = ret;
    ino->atime = ino->ctime = time(NULL);
    put_inode(inum, ino);
    free(ino);
}
```

写块这里我做得暴力一点，直接把原来的全删除了 再重新建立。XD,需要注意的是 如果有indirect不只要删除其中指向的数据block，同时也要删除用来存间接指向的indirect block,建立时同理

```c++
/* alloc/free blocks if needed */
void
inode_manager::write_file(uint32_t inum, const char *buf, int size)
{
    inode_t * ino = get_inode(inum);
    if(!ino)
        return ;
    unsigned int offset = 0;
    // free
    for (int i = 0; i < NDIRECT && offset < ino->size; i++) {
        bm->free_block(ino->blocks[i]);
        offset += BLOCK_SIZE;
    }
    if (offset < ino->size) {
        char indirect[BLOCK_SIZE];
        bm->read_block(ino->blocks[NDIRECT], indirect);
        for (unsigned int i = 0; i < NINDIRECT && offset < ino->size; i++) {
            blockid_t id = *((blockid_t *)indirect + i);
            bm->free_block(id);
            offset += BLOCK_SIZE;
        }
        bm->free_block(ino->blocks[NDIRECT]);
    }
    // new
    char writebuf[BLOCK_SIZE];
    offset = 0;
    ino->size = size;
    for (int i = 0; i < NDIRECT && offset < ino->size; i++) {
        int len = MIN(ino->size - offset , BLOCK_SIZE);
        ino->blocks[i] = bm->alloc_block();
        memcpy(writebuf,buf + offset,len);
        bm->write_block(ino->blocks[i],writebuf);
        offset += len;
    }
    if (offset < ino->size) {
        ino->blocks[NDIRECT] = bm->alloc_block();
        char indirect[BLOCK_SIZE];
        for (unsigned int i = 0; i < NINDIRECT && offset < ino->size; i++) {
            blockid_t * id = (blockid_t *)indirect + i;
            *id = bm->alloc_block();
            int len = MIN(ino->size - offset , BLOCK_SIZE);
            memcpy(writebuf,buf + offset,len);
            bm->write_block(*id,writebuf);
            offset += len;
        }
        bm->write_block(ino->blocks[NDIRECT], indirect);
    }
    ino->atime = ino->ctime = ino->mtime = time(NULL);
    put_inode(inum,ino);
    free(ino);
}
```

回头看一下 磁盘划分，也就是从 inode部分以后才是可以使用的,而inode及前面的部分 不需要bitmap标记,是通过相对偏移和宏来进行使用，

```c++
// Allocate a free disk block.
blockid_t
block_manager::alloc_block()
{
    char buf[BLOCK_SIZE];
    for (blockid_t id = IBLOCK(sb.ninodes,sb.nblocks) + 1; id < sb.nblocks; id++) {
        unsigned int offset = id % BPB;
        d->read_block(BBLOCK(id), buf);
        if (!(buf[offset/8] & (1 << (offset % 8)))) {
            buf[offset/8] |= 1 << (offset % 8);
            write_block(BBLOCK(id), buf);
            return id;
        }
    }
    return 0;
}

void
block_manager::free_block(uint32_t id)
{
    char buf[BLOCK_SIZE];
    if (id <= IBLOCK(sb.ninodes,sb.nblocks) || id >= sb.nblocks)
        return;
    uint32_t offset = id % BPB;
    read_block(BBLOCK(id), buf);
    buf[offset/8] &= ~(1 << (offset % 8));
    write_block(BBLOCK(id), buf);
}
```

至此80分到手

# Part 3 REMOVE

implement `inode_manager:` `remove_file` and `free_inode` to support the REMOVE API of `extent_server`.

After you finish these 2 functions implementation, run:

    % make
    % ./lab1_tester

You should get following output:

    ========== begin test create and getattr ==========
    …
    …
    ========== pass test create and getattr ==========
    ========== begin test put and get ==========
    …
    …
    ========== pass test put and get ==========
    ========== begin test remove ==========
    …
    ...
    ========== pass test remove ==========
    --------------------------------------------------
    Final score is : 100

Handin Procedure
After all above done:

    % cd /path_to_cselab/lab1
    % make handin

正好前面的是 简洁 暴力的实现 在这里反而让代码变简单了

```c++
void
inode_manager::free_inode(uint32_t inum)
{
    struct inode * ino = get_inode(inum);
    if(!ino)
        return ;
    ino->type = 0;
    put_inode(inum,ino);
}
```

```c++
void
inode_manager::remove_file(uint32_t inum)
{
    write_file(inum, NULL, 0);
    free_inode(inum);
}
```



# 结束

整个lab的测试过程和之前靠 grep输出评分的lab不同，

在代码中 也可以根据自己喜好添加一些printf,也可以多加一些assert 方便调试

