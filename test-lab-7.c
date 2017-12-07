/*
 * test-lab-4-a /classfs/dir1 /classfs/dir2
 *
 * Test correctness of locking and cache coherence by creating
 * and deleting files in the same underlying directory
 * via two different ccfs servers.
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <dirent.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <time.h>

#include <sys/shm.h>
#include <sys/ipc.h>

char d1[512], d2[512];
extern int errno;

char normal[1000];
char big[20001];
char huge[65536];

void
create1(const char *d, const char *f, const char *in)
{
  int fd;
  char n[512];

  /*
   * The FreeBSD NFS client only invalidates its caches
   * cache if the mtime changes by a whole second.
   */
  sleep(1);

  sprintf(n, "%s/%s", d, f);
  fd = creat(n, 0666);
  if(fd < 0){
    fprintf(stderr, "test-lab-5: create(%s): %s\n",
            n, strerror(errno));
    exit(1);
  }
  if(write(fd, in, strlen(in)) != strlen(in)){
    fprintf(stderr, "test-lab-5: write(%s): %s\n",
            n, strerror(errno));
    exit(1);
  }
  if(close(fd) != 0){
    fprintf(stderr, "test-lab-5: close(%s): %s\n",
            n, strerror(errno));
    exit(1);
  }
}

void
check1(const char *d, const char *f, const char *in)
{
  int fd, cc;
  char n[512], buf[21000];

  sprintf(n, "%s/%s", d, f);
  fd = open(n, 0);
  if(fd < 0){
    fprintf(stderr, "test-lab-5: open(%s): %s\n",
            n, strerror(errno));
    exit(1);
  }
  errno = 0;
  cc = read(fd, buf, sizeof(buf) - 1);
  if(cc != strlen(in)){
    fprintf(stderr, "test-lab-5: read(%s) returned too little %d%s%s\n",
            n,
            cc,
            errno ? ": " : "",
            errno ? strerror(errno) : "");
    exit(1);
  }
  close(fd);
  buf[cc] = '\0';
  //if(strncmp(buf, in, strlen(n)) != 0){ //[tmac] Are you kidding me??
  if(strncmp(buf, in, strlen(in)) != 0){
    fprintf(stderr, "test-lab-5: read(%s) got \"%s\", not \"%s\"\n",
            n, buf, in);
    exit(1);
  }
}

void
unlink1(const char *d, const char *f)
{
  char n[512];

  sleep(1);

  sprintf(n, "%s/%s", d, f);
  if(unlink(n) != 0){
    fprintf(stderr, "test-lab-5: unlink(%s): %s\n",
            n, strerror(errno));
    exit(1);
  }
}

void
checknot(const char *d, const char *f)
{
  int fd;
  char n[512];

  sprintf(n, "%s/%s", d, f);
  fd = open(n, 0);
  if(fd >= 0){
    fprintf(stderr, "test-lab-4-a: open(%s) succeeded for deleted file\n", n);
    exit(1);
  }
}

void
append1(const char *d, const char *f, const char *in)
{
  int fd;
  char n[512];

  sleep(1);

  sprintf(n, "%s/%s", d, f);
  fd = open(n, O_WRONLY|O_APPEND);
  if(fd < 0){
    fprintf(stderr, "test-lab-4-a: append open(%s): %s\n",
            n, strerror(errno));
    exit(1);
  }
  if(write(fd, in, strlen(in)) != strlen(in)){
    fprintf(stderr, "test-lab-4-a: append write(%s): %s\n",
            n, strerror(errno));
    exit(1);
  }
  if(close(fd) != 0){
    fprintf(stderr, "test-lab-4-a: append close(%s): %s\n",
            n, strerror(errno));
    exit(1);
  }
}

// write n characters starting at offset start,
// one at a time.
void
write1(const char *d, const char *f, int start, int n, char c)
{
  int fd;
  char name[512];

  sleep(1);

  sprintf(name, "%s/%s", d, f);
  fd = open(name, O_WRONLY|O_CREAT, 0666);
  if (fd < 0 && errno == EEXIST)
    fd = open(name, O_WRONLY, 0666);
  if(fd < 0){
    fprintf(stderr, "test-lab-4-a: open(%s): %s\n",
            name, strerror(errno));
    exit(1);
  }
  if(lseek(fd, start, 0) != (off_t) start){
    fprintf(stderr, "test-lab-4-a: lseek(%s, %d): %s\n",
            name, start, strerror(errno));
    exit(1);
  }
  for(int i = 0; i < n; i++){
    if(write(fd, &c, 1) != 1){
      fprintf(stderr, "test-lab-4-a: write(%s): %s\n",
              name, strerror(errno));
      exit(1);
    }
    if(fsync(fd) != 0){
      fprintf(stderr, "test-lab-4-a: fsync(%s): %s\n",
              name, strerror(errno));
      exit(1);
    }
  }
  if(close(fd) != 0){
    fprintf(stderr, "test-lab-4-a: close(%s): %s\n",
            name, strerror(errno));
    exit(1);
  }
}

// check that the n bytes at offset start are all c.
void
checkread(const char *d, const char *f, int start, int n, char c)
{
  int fd;
  char name[512];

  sleep(1);

  sprintf(name, "%s/%s", d, f);
  fd = open(name, 0);
  if(fd < 0){
    fprintf(stderr, "test-lab-4-a: open(%s): %s\n",
            name, strerror(errno));
    exit(1);
  }
  if(lseek(fd, start, 0) != (off_t) start){
    fprintf(stderr, "test-lab-4-a: lseek(%s, %d): %s\n",
            name, start, strerror(errno));
    exit(1);
  }
  for(int i = 0; i < n; i++){
    char xc;
    if(read(fd, &xc, 1) != 1){
      fprintf(stderr, "test-lab-4-a: read(%s): %s\n",
              name, strerror(errno));
      exit(1);
    }
    if(xc != c){
      fprintf(stderr, "test-lab-4-a: checkread off %d %02x != %02x\n",
              start + i, xc, c);
      exit(1);
    }
  }
  close(fd);
}


void
createn(const char *d, const char *prefix, int nf, bool possible_dup)
{
  int fd, i;
  char n[512];

  /*
   * The FreeBSD NFS client only invalidates its caches
   * cache if the mtime changes by a whole second.
   */
  sleep(1);

  for(i = 0; i < nf; i++){
    sprintf(n, "%s/%s-%d", d, prefix, i);
    fd = creat(n, 0666);
    if (fd < 0 && possible_dup && errno == EEXIST)
      continue;
    if(fd < 0){
      fprintf(stderr, "test-lab-4-a: create(%s): %s\n",
              n, strerror(errno));
      exit(1);
    }
    if(write(fd, &i, sizeof(i)) != sizeof(i)){
      fprintf(stderr, "test-lab-4-a: write(%s): %s\n",
              n, strerror(errno));
      exit(1);
    }
    if(close(fd) != 0){
      fprintf(stderr, "test-lab-4-a: close(%s): %s\n",
              n, strerror(errno));
      exit(1);
    }
  }
}

void
checkn(const char *d, const char *prefix, int nf)
{
  int fd, i, cc, j;
  char n[512];

  for(i = 0; i < nf; i++){
    sprintf(n, "%s/%s-%d", d, prefix, i);
    fd = open(n, 0);
    if(fd < 0){
      fprintf(stderr, "test-lab-4-a: open(%s): %s\n",
              n, strerror(errno));
      exit(1);
    }
    j = -1;
    cc = read(fd, &j, sizeof(j));
    if(cc != sizeof(j)){
      fprintf(stderr, "test-lab-4-a: read(%s) returned too little %d%s%s\n",
              n,
              cc,
              errno ? ": " : "",
              errno ? strerror(errno) : "");
      exit(1);
    }
    if(j != i){
      fprintf(stderr, "test-lab-4-a: checkn %s contained %d not %d\n",
              n, j, i);
      exit(1);
    }
    close(fd);
  }
}

void
unlinkn(const char *d, const char *prefix, int nf)
{
  char n[512];
  int i;

  sleep(1);

  for(i = 0; i < nf; i++){
    sprintf(n, "%s/%s-%d", d, prefix, i);
    if(unlink(n) != 0){
      fprintf(stderr, "test-lab-4-a: unlink(%s): %s\n",
              n, strerror(errno));
      exit(1);
    }
  }
}

int
compar(const void *xa, const void *xb)
{
  char *a = *(char**)xa;
  char *b = *(char**)xb;
  return strcmp(a, b);
}

void
dircheck(const char *d, int nf)
{
  DIR *dp;
  struct dirent *e;
  char *names[1000];
  int nnames = 0, i;

  dp = opendir(d);
  if(dp == 0){
    fprintf(stderr, "test-lab-4-a: opendir(%s): %s\n", d, strerror(errno));
    exit(1);
  }
  while((e = readdir(dp))){
    if(e->d_name[0] != '.'){
      if(nnames >= sizeof(names)/sizeof(names[0])){
        fprintf(stderr, "warning: too many files in %s\n", d);
      }
      names[nnames] = (char *) malloc(strlen(e->d_name) + 1);
      strcpy(names[nnames], e->d_name);
      nnames++;
    }
  }
  closedir(dp);

  if(nf != nnames){
    fprintf(stderr, "test-lab-4-a: wanted %d dir entries, got %d\n", nf, nnames);
    exit(1);
  }

  /* check for duplicate entries */
  qsort(names, nnames, sizeof(names[0]), compar);
  for(i = 0; i < nnames-1; i++){
    if(strcmp(names[i], names[i+1]) == 0){
      fprintf(stderr, "test-lab-4-a: duplicate directory entry for %s\n", names[i]);
      exit(1);
    }
  }

  for(i = 0; i < nnames; i++)
    free(names[i]);
}

void
reap (int pid)
{
  int wpid, status;
  wpid = waitpid (pid, &status, 0);
  if (wpid < 0) {
    perror("waitpid");
    exit(1);
  }
  if (wpid != pid) {
    fprintf(stderr, "unexpected pid reaped: %d\n", wpid);
    exit(1);
  }
  if(!WIFEXITED(status) || WEXITSTATUS(status) != 0) {
    fprintf(stderr, "child exited unhappily\n");
    exit(1);
  }
}

#define PATH "/tmp"
#define BUFFER_SIZE 16
#define ID 1

int
main(int argc, char *argv[])
{
  int pid, i;

	char *shmAddr;
	key_t key;
	int shmid;
	
	char tmacname[16];
	char basename[5];
	int tmacnum;

  if(argc != 3){
    fprintf(stderr, "Usage: test-lab-4-a dir1 dir2\n");
    exit(1);
  }

	key = ftok(PATH, ID);
	shmid = shmget(key, BUFFER_SIZE, 0666|IPC_CREAT);
	if(shmid == -1)
	{
		printf("FILE %s line %d:share memory get %s\n", __FILE__, __LINE__, strerror(errno));
		exit(1);
	}
	
  sprintf(d1, "%s/d%d", argv[1], getpid());
  if(mkdir(d1, 0777) != 0){
    fprintf(stderr, "test-lab-4-a: failed: mkdir(%s): %s\n",
            d1, strerror(errno));
    exit(1);
  }
  sprintf(d2, "%s/d%d", argv[2], getpid());
  if(access(d2, 0) != 0){
    fprintf(stderr, "test-lab-4-a: failed: access(%s) after mkdir %s: %s\n",
            d2, d1, strerror(errno));
    exit(1);
  }

  setbuf(stdout, 0);

	srand((unsigned)time(NULL));

	for(i = 0; i < sizeof(normal)-1; ++i)
		normal[i] = '0' + rand() % 10;
	normal[sizeof(normal)-1] = '\0';
  for(i = 0; i < sizeof(big)-1; i++)
    big[i] = 'x';
	big[sizeof(big)-1] = '\0';
  for(i = 0; i < sizeof(huge)-1; i++)
    huge[i] = '0';
	huge[sizeof(huge)-1] = '\0';

	
  printf("Create then read: ");
  create1(d1, "f1", "aaa");
	check1(d2, "f1", "aaa");
  check1(d1, "f1", "aaa");
  printf("OK\n");

  printf("Unlink: ");
  unlink1(d2, "f1");
  create1(d1, "fx1", "fxx"); 
  unlink1(d1, "fx1");
  checknot(d1, "f1");
  checknot(d2, "f1");
  create1(d1, "f2", "222");
  unlink1(d2, "f2");
  checknot(d1, "f2");
  checknot(d2, "f2");
  create1(d1, "f3", "333");
  check1(d2, "f3", "333");
  check1(d1, "f3", "333");
  unlink1(d1, "f3");
  create1(d2, "fx2", "22"); 
  unlink1(d2, "fx2");
  checknot(d2, "f3");
  checknot(d1, "f3");
  printf("OK\n");

  printf("Append: ");
  create1(d2, "f1", "aaa");
  append1(d1, "f1", "bbb");
  append1(d2, "f1", "ccc");
  check1(d1, "f1", "aaabbbccc");
  check1(d2, "f1", "aaabbbccc");
  printf("OK\n");

  printf("Readdir: ");
  dircheck(d1, 1);
  dircheck(d2, 1);
  unlink1(d1, "f1");
  dircheck(d1, 0);
  dircheck(d2, 0);
  create1(d2, "f2", "aaa");
  create1(d1, "f3", "aaa");
  dircheck(d1, 2);
  dircheck(d2, 2);
  unlink1(d2, "f2");
  dircheck(d2, 1);
  dircheck(d1, 1);
  unlink1(d2, "f3");
  dircheck(d1, 0);
  dircheck(d2, 0);
  printf("OK\n");

  printf("Many sequential creates: ");
  createn(d1, "aa", 10, false);
  createn(d2, "bb", 10, false);
  dircheck(d2, 20);
  checkn(d2, "bb", 10);
  checkn(d2, "aa", 10);
  checkn(d1, "aa", 10);
  checkn(d1, "bb", 10);
  unlinkn(d1, "aa", 10);
  unlinkn(d2, "bb", 10);
  printf("OK\n");

  printf("Write 20000 bytes: ");
  create1(d1, "bf", big);
  check1(d1, "bf", big);
  check1(d2, "bf", big);
  unlink1(d1, "bf");
  printf("OK\n");

  printf("Concurrent creates: ");
  pid = fork();
  if(pid < 0){
    perror("test-lab-4-a: fork");
    exit(1);
  }
  if(pid == 0){
    createn(d2, "xx", 10, false);
    exit(0);
  }
  createn(d1, "yy", 10, false);
  sleep(4);
  reap(pid);
  dircheck(d1, 20);
  checkn(d1, "xx", 10);
  checkn(d2, "yy", 10);
  unlinkn(d1, "xx", 10);
  unlinkn(d1, "yy", 10);
  printf("OK\n");

  printf("Concurrent creates of the same file: ");
  pid = fork();
  if(pid < 0){
    perror("test-lab-4-a: fork");
    exit(1);
  }
  if(pid == 0){
    createn(d2, "zz", 10, true);
    exit(0);
  }
  createn(d1, "zz", 10, true);
  sleep(4);
  dircheck(d1, 10);
  reap(pid);
  checkn(d1, "zz", 10);
  checkn(d2, "zz", 10);
  unlinkn(d1, "zz", 10);
  printf("OK\n");

  printf("Concurrent create/delete: ");
  createn(d1, "x1", 5, false);
  createn(d2, "x2", 5, false);
  pid = fork();
  if(pid < 0){
    perror("test-lab-4-a: fork");
    exit(1);
  }
  if(pid == 0){
    unlinkn(d2, "x1", 5);
    createn(d1, "x3", 5, false);
    exit(0);
  }
  createn(d1, "x4", 5, false);
  reap(pid);
  unlinkn(d2, "x2", 5);
  unlinkn(d2, "x4", 5);
  unlinkn(d2, "x3", 5);
  dircheck(d1, 0);
  printf("OK\n");

  printf("Concurrent creates, same file, same server: ");
  pid = fork();
  if(pid < 0){
    perror("test-lab-4-a: fork");
    exit(1);
  }
  if(pid == 0){
    createn(d1, "zz", 10, true);
    exit(0);
  }
  createn(d1, "zz", 10, true);
  sleep(2);
  dircheck(d1, 10);
  reap(pid);
  checkn(d1, "zz", 10);
  unlinkn(d1, "zz", 10);
  printf("OK\n");

  printf("Concurrent writes to different parts of same file: ");
  create1(d1, "www", huge);
  pid = fork();
  if(pid < 0){
    perror("test-lab-4-a: fork");
    exit(1);
  }
  if(pid == 0){
    write1(d2, "www", 10000, 64, '2');
    exit(0);
  }
  write1(d1, "www", 0, 64, '1');
  reap(pid);
  checkread(d1, "www", 0, 64, '1');
  checkread(d2, "www", 0, 64, '1');
  checkread(d1, "www", 10000, 64, '2');
  checkread(d2, "www", 10000, 64, '2');
	unlink1(d1, "www");
  printf("OK\n");
	

	//lab5
	tmacnum = rand() % 10 + 1;
	for(i = 0; i < 4; ++i)
		basename[i] = 'a' + rand() % 24;
	basename[4] = '\0';

	memset(tmacname, '\0', 16);

	for(i = 0; i < tmacnum; ++i)
	{
		sprintf(tmacname, "%s%d", basename, i);
		create1(d1, tmacname, normal);
	}
	
	
	//first round

	shmAddr = (char*)shmat(shmid, (void*)0, 0);
	if(shmAddr == (char*)-1)
	{
		printf("FILE %s line %d:share memory get %s\n", __FILE__, __LINE__, strerror(errno));
		exit(1);
	}

	printf("round 1:\n");
	strcpy(shmAddr, "space-rays-1");
	while(strncmp(shmAddr, "space-rays-1d", 13) != 0){}

	for(i = 0; i < tmacnum; ++i)
	{
		sprintf(tmacname, "%s%d", basename, i);
  	check1(d1, tmacname, normal);
  	check1(d2, tmacname, normal);
	}	
	printf("OK: ");
	printf("You got 20 points now!\n");


	//second round

	shmAddr = (char*)shmat(shmid, (void*)0, 0);
	if(shmAddr == (char*)-1)
	{
		printf("FILE %s line %d:share memory get %s\n", __FILE__, __LINE__, strerror(errno));
		exit(1);
	}

	printf("round 2:\n");
	strcpy(shmAddr, "space-rays-2");
	while(strncmp(shmAddr, "space-rays-2d", 13) != 0){}

	for(i = 0; i < tmacnum; ++i)
	{
		sprintf(tmacname, "%s%d", basename, i);
  	check1(d1, tmacname, normal);
  	check1(d2, tmacname, normal);
	}
	printf("OK: ");
	printf("You got 50 points now!\n");


	//third round

	shmAddr = (char*)shmat(shmid, (void*)0, 0);
	if(shmAddr == (char*)-1)
	{
		printf("FILE %s line %d:share memory get %s\n", __FILE__, __LINE__, strerror(errno));
		exit(1);
	}

	printf("round 3:\n");
	strcpy(shmAddr, "space-rays-3");
	while(strncmp(shmAddr, "space-rays-3d", 13) != 0){}

	for(i = 0; i < tmacnum; ++i)
	{
		sprintf(tmacname, "%s%d", basename, i);
  	check1(d1, tmacname, normal);
  	check1(d2, tmacname, normal);
	}
	printf("OK: ");
	printf("You got 80 points now!\n");


	//last round

	shmAddr = (char*)shmat(shmid, (void*)0, 0);
	if(shmAddr == (char*)-1)
	{
		printf("FILE %s line %d:share memory get %s\n", __FILE__, __LINE__, strerror(errno));
		exit(1);
	}

	printf("round 4:\n");
	strcpy(shmAddr, "space-rays-4");
	while(strncmp(shmAddr, "space-rays-4d", 13) != 0){}

	for(i = 0; i < tmacnum; ++i)
	{
		sprintf(tmacname, "%s%d", basename, i);
  	check1(d1, tmacname, normal);
  	check1(d2, tmacname, normal);
	}
	printf("OK: ");
	printf("You got 95 points now!\n");

	printf("cleanup:\n");
	for(i = 0; i < tmacnum; ++i)
	{
		sprintf(tmacname, "%s%d", basename, i);
  	check1(d1, tmacname, normal);
  	unlink1(d1, tmacname);
	}
	printf("OK: ");
	printf("You got 100 points now!\n");


	//bonus
	for(i = 0; i < tmacnum; ++i)
	{
		sprintf(tmacname, "%s%d", basename, i);
		create1(d1, tmacname, normal);
	}
	
	shmAddr = (char*)shmat(shmid, (void*)0, 0);
	if(shmAddr == (char*)-1)
	{
		printf("FILE %s line %d:share memory get %s\n", __FILE__, __LINE__, strerror(errno));
		exit(1);
	}

	printf("bonus:\n");
	strcpy(shmAddr, "space-rays-5");
	while(strncmp(shmAddr, "space-rays-5d", 13) != 0){}

	for(i = 0; i < tmacnum; ++i)
	{
		sprintf(tmacname, "%s%d", basename, i);
  	check1(d1, tmacname, normal);
  	check1(d2, tmacname, normal);
  	unlink1(d1, tmacname);
	}
	printf("OK: ");
	printf("You got 105 points now!\n");

  printf("test-lab-5: Passed all tests.\n");

	shmdt(shmAddr);
	shmctl(shmid, IPC_RMID, NULL);

  return(0);
}
