#ifndef SHARED_DATA_H_
#define SHARED_DATA_H_

#include <memory>
#include <vector>
#include <iostream>
#include "boost/thread.hpp"
#include "singleton.h"

class SharedData {
 private:
  typedef boost::mutex Mutex;
  typedef boost::shared_mutex ReadWriteMutex;
  typedef boost::shared_lock<boost::shared_mutex> ReadLock;
  typedef boost::unique_lock<boost::shared_mutex> WriteLock;
 public:
  ~SharedData(){}
  
  void SetData(int data)
  {
    WriteLock lock(lock_data_);
    singleton=data;
  }
  void GetData(int *data)
  {
    ReadLock lock(lock_data_);
    *data=singleton;
  }
 private:
  int singleton=0;
  ReadWriteMutex lock_data_;
 private:
  DECLARE_SINGLETON(SharedData); 
};
#endif
