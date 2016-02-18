#include "runtime/thread.inline.hpp"
#include "runtime/profiling.hpp"
#include "runtime/objectMonitor.inline.hpp"
#include <stdlib.h>

/*** Time accounting ***/
unsigned long long FreeLunchStats::_start_application_time    = 0;
         long      FreeLunchStats::_start_application_time_ms = 0;
unsigned long long FreeLunchStats::application_time           = 0;
unsigned long long FreeLunchStats::application_time_ms        = 0;

void FreeLunchStats::application_start() {
  unsigned long start_1, start_2;
  RDTSC(start_1, start_2);
  _start_application_time  = start_1 | (start_2 << 32);
  _start_application_time_ms = os::javaTimeMillis();
}

extern long long _current_phase_time;
extern long      _current_phase_time_ms;

void FreeLunchStats::application_stop() {
  application_time    = _current_phase_time    - _start_application_time;
  application_time_ms = _current_phase_time_ms - FreeLunchStats::_start_application_time_ms;
}


#ifndef NO_OBJECTMONITOR_LIST
/*** N-highest locks data structure ***/

ObjectMonitor_list *ObjectMonitor_list::phase_contended_locks_list;

// insert
void ObjectMonitor_list::insert(const value_type& val) {
  if (capacity <= 0)
    return;

  // Insert the monitor if the list is not full or 
  // if it has a CSP superior to the monitor with the lowest CSP in the list.
  if (size() < capacity || min_cs_time < val->_current_phase_cs_time) {
    // The list is sorted in ascending order of CSP.
    // Insert the monitor at the right place
    std::list<ObjectMonitor*>::iterator it = begin();    
    while (it != end() && val->_current_phase_cs_time > (*it)->_current_phase_cs_time)
      it++;

    std::list<ObjectMonitor*>::insert(it, val);

    // Keep the size <= capacity
    if (size() > capacity)
      pop_front();

    min_cs_time = front()->_current_phase_cs_time;
  }
}

void ObjectMonitor_list::print() {
  std::list<ObjectMonitor*>::iterator it;
  for (it = begin(); it != end(); it++) {
    ObjectMonitor *m = *it;
    monitor_stream->print("%p %lld|", m, m->_current_phase_cs_time);
  }
  monitor_stream->print_cr("");

  for (it = begin(); it != end(); it++) {
    ObjectMonitor *m = *it;
    monitor_stream->print("          :");
    m->myprint(monitor_stream);
  }
}

void ObjectMonitor_list::clear() {
  min_cs_time = 0;
  std::list<ObjectMonitor*>::clear();
}
#endif


/*** Locking statistics functions  ***/
#define CHAINMARKER (cast_to_oop<intptr_t>(-1)) // from synchronizer.cpp

// PrintLockCSPSummary
struct monitorSort {
  ObjectMonitor *mid;
  float avg_csp;
};

static int monitorSort_comparison(const void *ptr1, const void *ptr2) {
  struct monitorSort *m1, *m2;
  m1 = (struct monitorSort *) ptr1;
  m2 = (struct monitorSort *) ptr2;

  if (m1->avg_csp < m2->avg_csp)
    return 1;
  else if (m1->avg_csp > m2->avg_csp)
    return -1;
  else
    return 0;
}

void FreeLunchStats::printLockCSPSummary() {// TODO: voir safepoint.cpp:570 ->     // application_time += (_current_phase_time - _prev_phase_time) * number_of_threads - current_phase_wait_time - accumulated_phase_park_time;
  monitor_stream->print_cr("-----------------------------------------------------");
  monitor_stream->print_cr("Monitors ranked by average CSP (Threshold: %.2f %%)", CSPThresholdSummary);
  monitor_stream->print_cr("Rank   Average CSP         Object class");

  struct monitorSort *monitors_array = NEW_C_HEAP_ARRAY(struct monitorSort, ObjectSynchronizer::MonitorPopulation * sizeof(struct monitorSort), mtInternal);
  int count = 0;
  for (ObjectMonitor* block = ObjectSynchronizer::gBlockList; block != NULL; block = block->FreeNext) {
    // Iterate over all extant monitors
    assert(block->object() == CHAINMARKER, "must be a block header");
    for (int i = 1 ; i <  ObjectSynchronizer::_BLOCKSIZE; i++) {
      ObjectMonitor* mid = &block[i];

      if (mid->object() != NULL) {
	float avg_csp = mid->_accumulated_CSP * 100.0 / (float) application_time;
	assert(avg_csp >= 0 && avg_csp <= 100, "Error avg csp");
	
	monitors_array[count].mid        = mid;
        monitors_array[count++].avg_csp  = avg_csp;
      }
    }
  }

  // Sort the monitors
  qsort(monitors_array, count, sizeof(struct monitorSort), monitorSort_comparison);

  // Print every stack trace
  for (int i = 0; i < count; i++) {
    struct monitorSort m = monitors_array[i];
    //monitor_stream->print_cr("%4d - Average CSP: %.2f %%, Class: %s  (%d total frames)",
    monitor_stream->print_cr("%4d    %6.2f %%     %s",
			     i + 1,
			     m.avg_csp,
			     m.mid->_object_klass);

    if (m.avg_csp < CSPThresholdSummary)
      break;
  }

  // Free the array
  FREE_C_HEAP_ARRAY(struct monitorSort, monitors_array, mtInternal);
}


// PrintLockStackTraceSummary
void FreeLunchStats::printLockStackTraceSummary() {
  monitor_stream->print_cr("-----------------------------------------------------");
  monitor_stream->print_cr("Monitors ranked by average CSP (Threshold: %.2f %%)", CSPThresholdSummary);
  monitor_stream->print_cr("Displaying the associated stack trace (%d frame(s) displayed maximum)", StackFramesDisplayedCount);
  monitor_stream->print_cr("Rank   Average CSP         Object class");

  struct monitorSort *monitors_array = NEW_C_HEAP_ARRAY(struct monitorSort, ObjectSynchronizer::MonitorPopulation * sizeof(struct monitorSort), mtInternal);
  int count = 0;
  for (ObjectMonitor* block = ObjectSynchronizer::gBlockList; block != NULL; block = block->FreeNext) {
    // Iterate over all extant monitors
    assert(block->object() == CHAINMARKER, "must be a block header");
    for (int i = 1 ; i <  ObjectSynchronizer::_BLOCKSIZE; i++) {
      ObjectMonitor* mid = &block[i];

      if (mid->object() != NULL) {
	float avg_csp = mid->_accumulated_CSP * 100.0 / (float) application_time;
	assert(avg_csp >= 0 && avg_csp <= 100, "Error avg csp");
	
	monitors_array[count].mid        = mid;
        monitors_array[count++].avg_csp  = avg_csp;
      }
    }
  }

  // Sort the monitors
  qsort(monitors_array, count, sizeof(struct monitorSort), monitorSort_comparison);

  // Print every stack trace
  for (int i = 0; i < count; i++) {
    struct monitorSort m = monitors_array[i];
    monitor_stream->print_cr("%4d    %6.2f %%     %s     (%d frame(s) available)",
			     i + 1,
			     m.avg_csp,
			     m.mid->_object_klass,
			     m.mid->current_stack_depth);
    m.mid->print_stack_trace(monitor_stream);

    if (m.avg_csp < CSPThresholdSummary)
      break;
  }

  // Free the array
  FREE_C_HEAP_ARRAY(struct monitorSort, monitors_array, mtInternal);
}

// PrintLockingFrequencyStat
void FreeLunchStats::printLockingFrequencyStat() {
#ifdef COUNT_LOCKED
  monitor_stream->print_cr("Total locked:%lld |Rate: %.2f locking /ms.", 
			   SafepointSynchronize::total_locked,
			   (float) SafepointSynchronize::total_locked * 1.0 / (float) application_time_ms);
#endif
#ifdef COUNT_WAITED
  monitor_stream->print_cr("Total waited:%lld |Rate: %.2f waiting /ms.", 
			   SafepointSynchronize::total_waited,
			   (float) SafepointSynchronize::total_waited / (float) application_time_ms));
#endif
}

//#undef CHAINMARKER
