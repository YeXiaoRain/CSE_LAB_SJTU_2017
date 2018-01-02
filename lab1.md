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
      






















Part 2: PUT/GET
Your job in Part 2 is to implement the write_file and read_file of inode_manager, and alloc_block and free_block of block_manager, to support the PUT and GET APIs of extent_server.

You should pay attention to the indirect block test. In our inode manager, each file has only one additional level of indirect block, which means one file has 32 direct block and 1 indirect block which point to a block filled with other blocks id.

After you finish these 4 functions implementation, run:

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
    [TEST_ERROR] : error …
    --------------------------------------------------
    Final score is : 80
  
Part 3: REMOVE
Our job in Part 3 is to implement the remove_file and free_inode of inode_manager, to support the REMOVE API of extent_server.

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
  
That should produce a file called lab1.tgz in your lab1/ directory. Change the file name to your student id:
    % mv lab.tgz [your student id]-lab1.tgz
  
Then upload [your student id]-lab1.tgz file to ftp://Dd_nirvana:public@public.sjtu.edu.cn/upload/cse/lab1/ before the deadline. You are only given the permission to list and create new file, but no overwrite and read. So make sure your implementation has passed all the tests before final submit. (If you must re-submit a new version, add explicit version number such as "V2" to indicate).
You will receive full credit if your software passes the same tests we gave you when we run your software on our machines.

Please take your time examining this lab and the overall architecture of yfs. There are more interesting challenges ahead waiting for you.
