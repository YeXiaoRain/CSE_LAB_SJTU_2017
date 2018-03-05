// RPC stubs for clients to talk to extent_server

#include "extent_client.h"
#include <sstream>
#include <iostream>
#include <stdio.h>
#include <unistd.h>
#include <time.h>

// The calls assume that the caller holds a lock on the extent

extent_client::extent_client(std::string dst)
{
  sockaddr_in dstsock;
  make_sockaddr(dst.c_str(), &dstsock);
  cl = new rpcc(dstsock);
  if (cl->bind() != 0) {
    printf("extent_client: bind failed\n");
  }
}

// a demo to show how to use RPC
extent_protocol::status
extent_client::getattr(extent_protocol::extentid_t eid, 
		       extent_protocol::attr &attr)
{
  extent_protocol::status ret = extent_protocol::OK;
  ret = cl->call(extent_protocol::getattr, eid, attr);
  return ret;
}

extent_protocol::status
extent_client::create(uint32_t type, extent_protocol::extentid_t &id)
{
  return cl->call(extent_protocol::create, type, id);
}

extent_protocol::status
extent_client::get(extent_protocol::extentid_t eid, std::string &buf)
{
  return cl->call(extent_protocol::get, eid, buf);
}

extent_protocol::status
extent_client::put(extent_protocol::extentid_t eid, std::string buf)
{
  int r;
  return cl->call(extent_protocol::put, eid, buf, r);
}

extent_protocol::status
extent_client::remove(extent_protocol::extentid_t eid)
{
  int r;
  return  cl->call(extent_protocol::remove, eid, r);
}

extent_protocol::status
extent_client::commit()
{
  int r;
  return  cl->call(extent_protocol::commit, 0, r);
}

extent_protocol::status
extent_client::undo()
{
  int r;
  return  cl->call(extent_protocol::undo, 0, r);
}

extent_protocol::status
extent_client::redo()
{
  int r;
  return  cl->call(extent_protocol::redo, 0, r);
}

