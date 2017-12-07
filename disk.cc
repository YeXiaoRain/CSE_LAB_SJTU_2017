#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/shm.h>
#include <sys/ipc.h>
#include <string.h>
#include <error.h>
#include <pthread.h>
#include <time.h>

#include "inode_manager.h"

#define PATH "/tmp"
#define BUFFER_SIZE 16
#define ID 1

//disk layer
static void space_ray_1(unsigned char *blocks, int idx)
{
	int i;
	int j;
	unsigned char val;

	i = rand() % BLOCK_SIZE;	
	j = rand() % 8;

	for(; idx < BLOCK_NUM; ++idx)
	{
		val = *(blocks + idx * BLOCK_SIZE + i);
		val ^= (1 << j);
		*(blocks + idx * BLOCK_SIZE + i) = val;
	}
}


static void space_ray_2(unsigned char *blocks, int idx)
{
	int i;
	int j;
	unsigned char val;

	for(; idx < BLOCK_NUM; ++idx)
	{
		i = rand() % BLOCK_SIZE;	
		j = rand() % 8;
		val = *(blocks + idx * BLOCK_SIZE + i);
		val ^= (1 << j);
		*(blocks + idx * BLOCK_SIZE + i) = val;

	}
}

static void space_ray_3(unsigned char *blocks, int idx)
{
	int i;
	int j;
	unsigned char val;

	for(; idx < BLOCK_NUM; ++idx)
	{
		i = rand() % BLOCK_SIZE;	
		j = rand() % 8;
		val = *(blocks + idx * BLOCK_SIZE + i);
		val ^= (1 << j);
		*(blocks + idx * BLOCK_SIZE + i) = val;

		i = rand() % BLOCK_SIZE;	
		j = rand() % 8;
		val = *(blocks + idx * BLOCK_SIZE + i);
		val ^= (1 << j);
		*(blocks + idx * BLOCK_SIZE + i) = val;

	}
}

static void space_ray_4(unsigned char *blocks, int idx)
{
	int i;
	int j;
	unsigned char val;

	for(; idx < BLOCK_NUM; ++idx)
	{
		for(i = 0; i < BLOCK_SIZE; ++i)		
		{
		  //i = rand() % BLOCK_SIZE;	
		  j = rand() % 8;
		  val = *(blocks + idx * BLOCK_SIZE + i);
		  val ^= (1 << j);
		  *(blocks + idx * BLOCK_SIZE + i) = val;
		}
	}
}


void* test_daemon(void* arg)
{
	char *shmAddr;
	key_t key;
	int shmid;
	int start_idx;
	unsigned char *blocks;
		
	blocks = (unsigned char*)arg;
	//pthread_detach(pthread_self());
	printf("[daemon] 0x%lx\n", pthread_self());
	srand((unsigned)time(NULL));

	key = ftok(PATH, ID); 
	if(key == -1)
	{
		printf("ftok error");	
		pthread_exit((void*)1);	
	}
	shmid = shmget(key, BUFFER_SIZE, 0666|IPC_CREAT);
	if(shmid == -1)
	{
		printf("FILE %s line %d:share memory get %s\n", __FILE__, __LINE__, strerror(errno));    
		pthread_exit((void*)1);	
	}
	
	shmAddr = (char*)shmat(shmid, (void*)0, 0);
	if(shmAddr == (void*)-1)
	{
		printf("FILE %s line %d:share memory get %s\n", __FILE__, __LINE__, strerror(errno));    
		pthread_exit((void*)1);	
	}
	
	//no restore for disk
	{	
		strncpy(shmAddr, "clear\0", 6);
		//first round
		while(strncmp(shmAddr, "space-rays-1", 12) != 0){sleep(1);}
		start_idx = IBLOCK(INODE_NUM, BLOCK_NUM) + 1;
		space_ray_1(blocks, start_idx);
		strncpy(shmAddr, "space-rays-1d", 13);

		//second round
		while(strncmp(shmAddr, "space-rays-2", 12) != 0){sleep(1);}
		space_ray_2(blocks, start_idx);
		strncpy(shmAddr, "space-rays-2d", 13);

		//third round
		while(strncmp(shmAddr, "space-rays-3", 12) != 0){sleep(1);}
		space_ray_3(blocks, start_idx);
		strncpy(shmAddr, "space-rays-3d", 13);

		//last round
		while(strncmp(shmAddr, "space-rays-4", 12) != 0){sleep(1);}
		start_idx = BBLOCK(0);
		space_ray_2(blocks, start_idx);
		strncpy(shmAddr, "space-rays-4d", 13);

		//bonus
		while(strncmp(shmAddr, "space-rays-5", 12) != 0){sleep(1);}
		start_idx = IBLOCK(INODE_NUM, BLOCK_NUM) + 1;
		space_ray_4(blocks, start_idx);
		strncpy(shmAddr, "space-rays-5d", 13);
	}	

	//----------------
	shmdt(shmAddr);
	//shmctl(shmid, IPC_RMID, NULL);
	return (void*)0;
}
