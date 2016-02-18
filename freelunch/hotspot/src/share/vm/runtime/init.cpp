/*
 * Copyright (c) 1997, 2013, Oracle and/or its affiliates. All rights reserved.
 * DO NOT ALTER OR REMOVE COPYRIGHT NOTICES OR THIS FILE HEADER.
 *
 * This code is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 only, as
 * published by the Free Software Foundation.
 *
 * This code is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * version 2 for more details (a copy is included in the LICENSE file that
 * accompanied this code).
 *
 * You should have received a copy of the GNU General Public License version
 * 2 along with this work; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 * Please contact Oracle, 500 Oracle Parkway, Redwood Shores, CA 94065 USA
 * or visit www.oracle.com if you need additional information or have any
 * questions.
 *
 */

#include "precompiled.hpp"
#include "classfile/symbolTable.hpp"
#include "code/icBuffer.hpp"
#include "gc_interface/collectedHeap.hpp"
#include "interpreter/bytecodes.hpp"
#include "memory/universe.hpp"
#include "prims/methodHandles.hpp"
#include "runtime/handles.inline.hpp"
#include "runtime/icache.hpp"
#include "runtime/init.hpp"
#include "runtime/safepoint.hpp"
#include "runtime/sharedRuntime.hpp"
#include "utilities/macros.hpp"
/* +EDIT */
#include "runtime/objectMonitor.hpp"
#include "runtime/osThread.hpp"
#include "runtime/task.hpp"

#include "runtime/synchronizer.hpp"
#include "runtime/profiling.hpp"
/* +EDIT */

// Initialization done by VM thread in vm_init_globals()
void check_ThreadShadow();
void eventlog_init();
void mutex_init();
void chunkpool_init();
void perfMemory_init();

// Initialization done by Java thread in init_globals()
void management_init();
void bytecodes_init();
void classLoader_init();
void codeCache_init();
void VM_Version_init();
void os_init_globals();        // depends on VM_Version_init, before universe_init
void stubRoutines_init1();
jint universe_init();          // depends on codeCache_init and stubRoutines_init
void interpreter_init();       // before any methods loaded
void invocationCounter_init(); // before any methods loaded
void marksweep_init();
void accessFlags_init();
void templateTable_init();
void InterfaceSupport_init();
void universe2_init();  // dependent on codeCache_init and stubRoutines_init, loads primordial classes
void referenceProcessor_init();
void jni_handles_init();
void vmStructs_init();

void vtableStubs_init();
void InlineCacheBuffer_init();
void compilerOracle_init();
void compilationPolicy_init();
void compileBroker_init();

// Initialization after compiler initialization
bool universe_post_init();  // must happen after compiler_init
void javaClasses_init();  // must happen after vtable initialization
void stubRoutines_init2(); // note: StubRoutines need 2-phase init

// Do not disable thread-local-storage, as it is important for some
// JNI/JVM/JVMTI functions and signal handlers to work properly
// during VM shutdown
void perfMemory_exit();
void ostream_exit();

void vm_init_globals() {
  check_ThreadShadow();
  basic_types_init();
  eventlog_init();
  mutex_init();
  chunkpool_init();
  perfMemory_init();
}


jint init_globals() {
  HandleMark hm;
  management_init();
  bytecodes_init();
  classLoader_init();
  codeCache_init();
  VM_Version_init();
  os_init_globals();
  stubRoutines_init1();
  jint status = universe_init();  // dependent on codeCache_init and
                                  // stubRoutines_init1 and metaspace_init.
  if (status != JNI_OK)
    return status;

  interpreter_init();  // before any methods loaded
  invocationCounter_init();  // before any methods loaded
  marksweep_init();
  accessFlags_init();
  templateTable_init();
  InterfaceSupport_init();
  SharedRuntime::generate_stubs();
  universe2_init();  // dependent on codeCache_init and stubRoutines_init1
  referenceProcessor_init();
  jni_handles_init();
#if INCLUDE_VM_STRUCTS
  vmStructs_init();
#endif // INCLUDE_VM_STRUCTS

  vtableStubs_init();
  InlineCacheBuffer_init();
  compilerOracle_init();
  compilationPolicy_init();
  compileBroker_init();
  VMRegImpl::set_regName();

  if (!universe_post_init()) {
    return JNI_ERR;
  }
  javaClasses_init();   // must happen after vtable initialization
  stubRoutines_init2(); // note: StubRoutines need 2-phase init

  // All the flags that get adjusted by VM_Version_init and os::init_2
  // have been set so dump the flags now.
  if (PrintFlagsFinal) {
    CommandLineFlags::printFlags(tty, false);
  }

  return JNI_OK;
}

/* +EDIT */
#ifdef WITH_PHASES
extern long _end_last_phase_ms;
extern long long _prev_phase_time;

extern long long minimum_time_between_phases;
#endif
/* -EDIT */
void exit_globals() {
  static bool destructorsCalled = false;
  if (!destructorsCalled) {
    destructorsCalled = true;
    perfMemory_exit();
    if (PrintSafepointStatistics) {
      // Print the collected safepoint statistics.
      SafepointSynchronize::print_stat_on_exit();
    }
    if (PrintStringTableStatistics) {
      SymbolTable::dump(tty);
      StringTable::dump(tty);
    }
    ostream_exit();
  }

/* +EDIT */
  // Reset minimum_time_between_phases to force the last CSP computation.
  minimum_time_between_phases = 0;
  SafepointSynchronize::gather_data();
  SafepointSynchronize::compute_phase_statistics();

  // Stop after the last CSP computation
  FreeLunchStats::application_stop();

#ifdef WITH_MONITOR_ASSOCIATION // TODO: is it still necessary ?
  if (PrintLockCSPSummary)
    FreeLunchStats::printLockCSPSummary();

  if (PrintLockStackTraceSummary)
    FreeLunchStats::printLockStackTraceSummary();

  if (PrintLockingFrequencyStat)
    FreeLunchStats::printLockingFrequencyStat();
#endif

  // for (int i = 0; i < MAX_THREADS; i++)
  //   if (OSThread::osthread_array[i] != NULL) OSThread::th_stop[i] = _stop_application_time;

  if (PrintThreadStats) {
    monitor_stream->print_cr("------- th_start th_stop th_cs_and_wait_time th_wait_time th_name th_type th_cs_start th_cs_recursions");

    for (int i = 0; i < MAX_THREADS; i++) {
      if (OSThread::th_start[i] == 0 && OSThread::th_stop[i] == 0)
	break;

      monitor_stream->print("%d|%lld|%lld|%lld|%lld|%s", i, OSThread::th_start[i], OSThread::th_stop[i], 
			    OSThread::th_cs_time[i], OSThread::th_wait_time[i], OSThread::th_name[i]);
      /* DEBUG: */ monitor_stream->print_cr("|%lld|%lld", OSThread::th_cs_start[i], OSThread::th_cs_recursions[i]);
    }
  }
/* -EDIT */
}


static bool _init_completed = false;

bool is_init_completed() {
  return _init_completed;
}


/* +EDIT */
// Periodic task in order to trigger safepoint regularly
class PeriodicSafepointProfilerTask : public PeriodicTask {
public:
  PeriodicSafepointProfilerTask(int interval_time) : PeriodicTask(interval_time) {}
  void task();
};

static PeriodicSafepointProfilerTask* task                = NULL;

void PeriodicSafepointProfilerTask::task() {
  VM_ForceSafepoint op;
  VMThread::execute(&op);
}
/* -EDIT */
void set_init_completed() {
/* +EDIT */
#ifdef WITH_PHASES
  FreeLunchStats::application_start();
  _prev_phase_time = FreeLunchStats::_start_application_time;
  _end_last_phase_ms = FreeLunchStats::_start_application_time_ms;

  minimum_time_between_phases = MinimumTimeBetweenTwoPhases;

  if (EnableRegularSafepoint) {
    if (task == NULL) {
      //TODO: Verify the semantic of MinimumTimeBetweenTwoPhases and EnableRegularSafepoint when used together
      //minimum_time_between_phases = EnableRegularSafepoint;
      task = new PeriodicSafepointProfilerTask(EnableRegularSafepoint);
      task->enroll();
    }
  }
#endif
/* -EDIT */
  assert(Universe::is_fully_initialized(), "Should have completed initialization");
  _init_completed = true;
}
