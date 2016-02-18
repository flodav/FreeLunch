/*
 * Copyright (c) 1997, 2010, Oracle and/or its affiliates. All rights reserved.
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
#include "oops/oop.inline.hpp"
#include "runtime/osThread.hpp"

/* +EDIT */
intptr_t  OSThread::th_id_counter = -1;
long long OSThread::th_start          [MAX_THREADS];
long long OSThread::th_stop           [MAX_THREADS];

long long OSThread::th_cs_time        [MAX_THREADS];
long long OSThread::th_wait_time      [MAX_THREADS];

// Name and type of threads.
char*     OSThread::th_name           [MAX_THREADS];
int       OSThread::th_type           [MAX_THREADS];

// Array of all OSThread created
OSThread* OSThread::osthread_array    [MAX_THREADS];

// For debug purposes only.
long long OSThread::th_cs_start       [MAX_THREADS];
long long OSThread::th_cs_recursions  [MAX_THREADS];
/* -EDIT */

OSThread::OSThread(OSThreadStartFunc start_proc, void* start_parm) {
  pd_initialize();
  set_start_proc(start_proc);
  set_start_parm(start_parm);
  set_interrupted(false);
/* +EDIT */
  intptr_t id = Atomic::add_ptr(1, &th_id_counter);
  th_id = id;
  guarantee(th_id < MAX_THREADS, "Too much thread for thread arrays");
  guarantee(th_id >= 0, "ID error");

  cs_start = cs_recursions = cs_time = wait_time = cs_wait_time_tmp = 0;

  unsigned long start_1, start_2;
  RDTSC(start_1, start_2);
  OSThread::th_start[th_id]       = start_1 | (start_2 << 32);
  OSThread::osthread_array[th_id] = this;
/* -EDIT */
}

OSThread::~OSThread() {
  pd_destroy();
/* +EDIT */
  unsigned long stop_1, stop_2;
  RDTSC(stop_1, stop_2);
  OSThread::th_stop[th_id]          = stop_1 | (stop_2 << 32);

  OSThread::th_cs_time[th_id]        = cs_time;
  OSThread::th_wait_time[th_id]      = wait_time;
  OSThread::th_cs_start[th_id]       = cs_start;
  OSThread::th_cs_recursions[th_id]  = cs_recursions;

  OSThread::osthread_array[th_id]    = NULL;
/* -EDIT */
}

// Printing
void OSThread::print_on(outputStream *st) const {
  st->print("nid=0x%lx ", thread_id());
  switch (_state) {
    case ALLOCATED:               st->print("allocated ");                 break;
    case INITIALIZED:             st->print("initialized ");               break;
    case RUNNABLE:                st->print("runnable ");                  break;
    case MONITOR_WAIT:            st->print("waiting for monitor entry "); break;
    case CONDVAR_WAIT:            st->print("waiting on condition ");      break;
    case OBJECT_WAIT:             st->print("in Object.wait() ");          break;
    case BREAKPOINTED:            st->print("at breakpoint");               break;
    case SLEEPING:                st->print("sleeping");                    break;
    case ZOMBIE:                  st->print("zombie");                      break;
    default:                      st->print("unknown state %d", _state); break;
  }
}
