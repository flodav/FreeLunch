#ifndef SHARE_VM_RUNTIME_PROFILING_HPP
#define SHARE_VM_RUNTIME_PROFILING_HPP

#include <list>
#include "memory/resourceArea.hpp"
#include "runtime/objectMonitor.hpp"
#include "runtime/synchronizer.hpp"

#ifndef NO_OBJECTMONITOR_LIST
/*** N-highest locks data strucure ***/
// This class keeps the 'capacity' monitors with the most important CSP
// during the last phase.
// The list os always sorted in ascendig order of CSP, i.e. the monitor with the 
// lowest CSP is the head.
class ObjectMonitor_list: public std::list<ObjectMonitor*> {
 private:
  size_type capacity;
  long long min_cs_time;

 public:
  ObjectMonitor_list(size_type cap): capacity(cap), min_cs_time(0), std::list<ObjectMonitor*>() {};
  ~ObjectMonitor_list() {};

  void insert(const value_type& val);
  void print();
  void clear();

  static ObjectMonitor_list *phase_contended_locks_list;

  // Redefine allocation scheme like in the ObjectMonitor class
 public:
  void* operator new (size_t size) throw() {
    return AllocateHeap(size, mtInternal);
  }
  void* operator new[] (size_t size) throw() {
    return operator new (size);
  }
  void operator delete(void* p) {
    FreeHeap(p, mtInternal);
  }
  void operator delete[] (void *p) {
    operator delete(p);
  }
};
#endif

class CurrentPhaseStatistics {

};

class FreeLunchRecordData {
 public:
  static void recordThreadName(uintptr_t thread_id, const char *name) {
    ResourceMark rm;
    u_char *p = NEW_C_HEAP_ARRAY(u_char, strlen(name) + 1, mtInternal);
    strcpy((char*) p, name);
    OSThread::th_name[thread_id] = (char*) p;
  }
};

class FreeLunchStats {
  /*** Time accounting ***/
 public:
  static unsigned long long _start_application_time;
  static          long      _start_application_time_ms;
  static unsigned long long application_time;
  static unsigned long long application_time_ms;

  static void     application_start();
  static void     application_stop();

  /*** Locking statistics functions  ***/
 public:
  // PrintLockingFrequencyStat
  static void printLockingFrequencyStat();
  // PrintLockStackTraceSummary
  static void printLockStackTraceSummary();
  // PrintLockCSPSummary
  static void printLockCSPSummary();


  // http://openjdk.java.net/groups/hotspot/docs/RuntimeOverview.html#VM%20Lifecycle|outline
  // Use JVM_OnExit() to print all statistics on exit
  // Use VM_OnExit to print all statistics on exit ?

  // synchronizer.cpp
  // Replace assert in deflate_idle_monitors() (first line)

  // safepoint.cpp
  //static void SafepointSynchronize::gather_data();
  //void compute_wait_park_time(long long timestamp) {
  //void SafepointSynchronize::compute_phase_statistics() {

  // init.cpp
  // l.178 -> l.190 PrintThreadStats()

  // thread.cpp
  //ObjectSynchronizer::initialize_MonitorHashTable();

  // Print the value of all options in the monitor_contention file at startup

  // Everywhere: replace by a unique function
  // copy_thread_name
  //{
  //  ResourceMark rm;
  //  const char* str = thread->get_thread_name();
  //  u_char *p = NEW_C_HEAP_ARRAY(u_char, strlen(str) + 1, mtInternal);
  //  strcpy((char*) p, str);
  //  OSThread::th_name[thread->osthread()->th_id] = (char*) p;
  //}
};

#endif
