// the lock server implementation

#include "lock_server.h"
#include <sstream>
#include <stdio.h>
#include <unistd.h>
#include <arpa/inet.h>

lock_server::lock_server():
  nacquire (0)
{
  pthread_mutex_init(&lm, NULL);
  pthread_cond_init(&notbusy, NULL);
}

lock_server::~lock_server()
{
  pthread_cond_destroy(&notbusy);
  pthread_mutex_destroy(&lm);
}

lock_protocol::status
lock_server::stat(int clt, lock_protocol::lockid_t lid, int &r)
{
  lock_protocol::status ret = lock_protocol::OK;
  printf("stat request from clt %d\n", clt);
  r = nacquire;
  return ret;
}

lock_protocol::status
lock_server::acquire(int clt, lock_protocol::lockid_t lid, int &r)
{

  pthread_mutex_lock(&lm);
  while (islocked[lid])
    pthread_cond_wait(&notbusy, &lm);
  islocked[lid] = true;
  nacquire++;
  pthread_mutex_unlock(&lm);
  return lock_protocol::OK;
}

lock_protocol::status
lock_server::release(int clt, lock_protocol::lockid_t lid, int &r)
{
  pthread_mutex_lock(&lm);
  if(islocked[lid]){
    islocked[lid] = false;
    nacquire--;
    pthread_cond_broadcast(&notbusy);
  }
  pthread_mutex_unlock(&lm);
  return lock_protocol::OK;
}
