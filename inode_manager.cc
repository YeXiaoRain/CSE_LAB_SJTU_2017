#include "inode_manager.h"

// disk layer -----------------------------------------

disk::disk()
{
  bzero(blocks, sizeof(blocks));
}

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

void
disk::commit()
{
  unsigned char * new_log = new unsigned char [DISK_SIZE];
  memcpy(new_log,blocks,DISK_SIZE);
  blocks_log.push_back(new_log);
  log_id++;
}

void
disk::undo()
{
  if(log_id > 0){
    log_id -- ;
    memcpy(blocks,blocks_log[log_id],DISK_SIZE);
  }
}

void
disk::redo()
{
  if(log_id + 1 < blocks_log.size()){
    log_id ++ ;
    memcpy(blocks,blocks_log[log_id],DISK_SIZE);
  }
}

// block layer -----------------------------------------

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

// The layout of disk should be like this:
// |<-sb->|<-free block bitmap->|<-inode table->|<-data->|
block_manager::block_manager()
{
  d = new disk();

  // format the disk
  sb.size = BLOCK_SIZE * BLOCK_NUM;
  sb.nblocks = BLOCK_NUM;
  sb.ninodes = INODE_NUM;

}

void
block_manager::read_block(uint32_t id, char *buf)
{
  d->read_block(id, buf);
}

void
block_manager::write_block(uint32_t id, const char *buf)
{
  d->write_block(id, buf);
}

void
block_manager::commit()
{
  d->commit();
}

void
block_manager::undo()
{
  d->undo();
}

void
block_manager::redo()
{
  d->redo();
}

// inode layer -----------------------------------------

inode_manager::inode_manager()
{
  bm = new block_manager();
  uint32_t root_dir = alloc_inode(extent_protocol::T_DIR);
  if (root_dir != 1) {
    printf("\tim: error! alloc first inode %d, should be 1\n", root_dir);
    exit(0);
  }
}

/* Create a new file.
 * Return its inum. */
uint32_t
inode_manager::alloc_inode(uint32_t type)
{
    char buf[BLOCK_SIZE];
    for(uint32_t inum = 1; inum <= bm->sb.ninodes; inum++) {
        bm->read_block(IBLOCK(inum, bm->sb.nblocks), buf);
        struct inode * ino_disk = (struct inode*)buf + inum%IPB;
        if (ino_disk->type == 0) {
            ino_disk->type  = type;
            ino_disk->size  = 0;
            ino_disk->ctime = time(NULL);
            bm->write_block(IBLOCK(inum, bm->sb.nblocks), buf);
            return inum;
        }
    }
    return 0;
}

void
inode_manager::free_inode(uint32_t inum)
{
    struct inode * ino = get_inode(inum);
    if(!ino)
        return ;
    ino->type = 0;
    put_inode(inum,ino);
}


/* Return an inode structure by inum, NULL otherwise.
 * Caller should release the memory. */
struct inode* 
inode_manager::get_inode(uint32_t inum)
{
  struct inode *ino, *ino_disk;
  char buf[BLOCK_SIZE];

  printf("\tim: get_inode %d\n", inum);

  if (inum < 0 || inum >= INODE_NUM) {
    printf("\tim: inum out of range\n");
    return NULL;
  }

  bm->read_block(IBLOCK(inum, bm->sb.nblocks), buf);
  // printf("%s:%d\n", __FILE__, __LINE__);

  ino_disk = (struct inode*)buf + inum%IPB;
  if (ino_disk->type == 0) {
    printf("\tim: inode not exist\n");
    return NULL;
  }

  ino = (struct inode*)malloc(sizeof(struct inode));
  *ino = *ino_disk;

  return ino;
}

void
inode_manager::put_inode(uint32_t inum, struct inode *ino)
{
  char buf[BLOCK_SIZE];
  struct inode *ino_disk;

  printf("\tim: put_inode %d\n", inum);
  if (ino == NULL)
    return;

  bm->read_block(IBLOCK(inum, bm->sb.nblocks), buf);
  ino_disk = (struct inode*)buf + inum%IPB;
  *ino_disk = *ino;
  bm->write_block(IBLOCK(inum, bm->sb.nblocks), buf);
}

#define MIN(a,b) ((a)<(b) ? (a) : (b))

/* Get all the data of a file by inum. 
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

void
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

void
inode_manager::remove_file(uint32_t inum)
{
    write_file(inum, NULL, 0);
    free_inode(inum);
}

void
inode_manager::commit()
{
  bm->commit();
}

void
inode_manager::undo()
{
  bm->undo();
}

void
inode_manager::redo()
{
  bm->redo();
}
