/*****************************************************************************/
/**
 * \file
 * \brief   L4 kernel event tracing, event memory layout
 * \ingroup api_calls
 */
/*
 * (c) 2008-2009 Adam Lackorzynski <adam@os.inf.tu-dresden.de>
 *     economic rights: Technische Universität Dresden (Germany)
 *
 * This file is part of TUD:OS and distributed under the terms of the
 * GNU General Public License 2.
 * Please see the COPYING-GPL-2 file for details.
 *
 * As a special exception, you may use this file as part of a free software
 * library without restriction.  Specifically, if other files instantiate
 * templates or use macros or inline functions from this file, or you compile
 * this file and link it with other files to produce an executable, this
 * file does not by itself cause the resulting executable to be covered by
 * the GNU General Public License.  This exception does not however
 * invalidate any other reasons why the executable file might be covered by
 * the GNU General Public License.
 */
/*****************************************************************************/
#ifndef __L4_KTRACE_EVENTS_H__
#define __L4_KTRACE_EVENTS_H__

#include <l4/sys/types.h>

enum
{
    l4_ktrace_tbuf_unused             = 0,
    l4_ktrace_tbuf_pf                 = 1,
    l4_ktrace_tbuf_ipc                = 2,
    l4_ktrace_tbuf_ipc_res            = 3,
    l4_ktrace_tbuf_ipc_trace          = 4,
    l4_ktrace_tbuf_ke                 = 5,
    l4_ktrace_tbuf_ke_reg             = 6,
    l4_ktrace_tbuf_unmap              = 7,
    l4_ktrace_tbuf_shortcut_failed    = 8,
    l4_ktrace_tbuf_shortcut_succeeded = 9,
    l4_ktrace_tbuf_context_switch     = 10,
    l4_ktrace_tbuf_exregs             = 11,
    l4_ktrace_tbuf_breakpoint         = 12,
    l4_ktrace_tbuf_trap               = 13,
    l4_ktrace_tbuf_pf_res             = 14,
    l4_ktrace_tbuf_sched              = 15,
    l4_ktrace_tbuf_preemption         = 16,
    l4_ktrace_tbuf_ke_bin             = 17,
    l4_ktrace_tbuf_dynentries         = 18,
    l4_ktrace_tbuf_max                = 0x80,
    l4_ktrace_tbuf_hidden             = 0x80,
};

// IA-32 typedefs...
typedef void               Context; // We don't have a class Context
                                    // here...  But that's fine, we
                                    // can calculate the taskid and
                                    // the threadid from this void
                                    // pointer...
typedef void               Space;
typedef void               Sched_context;
typedef l4_fpage_t         L4_fpage;
typedef unsigned long      Address;
typedef unsigned long      Mword;
typedef Mword              L4_error;
typedef unsigned int       L4_snd_desc;
typedef unsigned int       L4_rcv_desc;
typedef unsigned long      L4_msg_tag;
typedef unsigned long      L4_obj_ref;
typedef l4_timeout_t       L4_timeout_pair;
typedef unsigned char      Unsigned8;
typedef unsigned short     Unsigned16;
typedef unsigned int       Unsigned32;
typedef unsigned long long Unsigned64;

typedef struct __attribute__((packed))
{
  Mword      number;                    // event number
  Mword      ip;                        // instruction pointer
  Context  * context;                   // current Context
  Unsigned64 tsc;                       // time stamp counter
  Unsigned32 pmc1;                      // performance counter 1
  Unsigned32 pmc2;                      // performance counter 2
  Unsigned32 kclock;                    // lower 32 bit of kernel clock
  Unsigned8  type;                      // type of entry
  Unsigned8  cpu;                       // cpu
  union __attribute__((__packed__))     // event specific data
    {
      struct __attribute__((__packed__))
        {
          Address     _pfa;           ///< pagefault address
          Mword       _error;         ///< pagefault error code
          Space       *_space;

        } pf;  // logged page fault
      struct __attribute__((__packed__))
        {
          L4_msg_tag  _tag;           ///< message tag
          Mword       _dword[2];      ///< first two message words
          L4_obj_ref  _dst;           ///< destination id
          Mword       _dbg_id;
          Mword       _label;
          L4_timeout_pair _timeout;   ///< timeout
        } ipc;  // logged ipc
      struct __attribute__((__packed__))
        {
          Unsigned64  _snd_tsc;       ///< entry tsc
          L4_msg_tag  _result;        ///< result
          L4_obj_ref  _snd_dst;       ///< send destination
          Mword       _rcv_dst;       ///< rcv partner
          Unsigned8   _snd_desc;
          Unsigned8   _rcv_desc;

        } ipc_res;  // logged ipc result
      struct __attribute__((__packed__))
        {
        } ipc_trace;  // traced ipc
      struct __attribute__((__packed__))
        {
          char msg[30];
        } ke;   // logged kernel event
      struct __attribute__((__packed__))
        {
          char msg[18];
          Mword vals[3];
        } ke_reg;  // logged kernel event plus register content
      struct __attribute__((__packed__))
        {
          char     _pad[3];
          L4_fpage fpage;
          Mword    mask;
          int      result;  // was bool
        } unmap;  // logged unmap operation
      struct __attribute__((__packed__))
        {
          // fixme: this seems unused in fiasco
        } shortcut_succeeded;  // logged short-cut ipc succeeded
      struct __attribute__((__packed__))
        {
          Context const *_dst;                ///< switcher target
          Context const *_dst_orig;
          Address     _kernel_ip;
          Mword       _lock_cnt;
          Space       *_from_space;
          Sched_context *_from_sched;
          Mword       _from_prio;
        } context_switch;  // logged context switch
      struct __attribute__((__packed__))
        {
          Address     _address;       ///< breakpoint address
          int         _len;           ///< breakpoint length
          Mword       _value;         ///< value at address
          int         _mode;          ///< breakpoint mode

        } breakpoint;  // logged breakpoint
      struct __attribute__((__packed__))
        {
          char       trapno;
          Unsigned16 _errno;
          Mword      ebp;
          Mword      cr2;
          Mword      eax;
          Mword      eflags;
          Mword      esp;
          Unsigned16 cs;
          Unsigned16 ds;
        } trap;  // logged trap
      struct __attribute__((__packed__))
        {
          Address     _pfa;
          L4_error    _err;
          L4_error    _ret;
        } pf_res;  // logged page fault result
      struct __attribute__((__packed__))
        {
          unsigned short _mode;
          Context const *_owner;
          unsigned short _id;
          unsigned short _prio;
          signed long  _left;
          unsigned long  _quantum;
        } sched;
      struct __attribute__((__packed__))
        {
          Mword        _preempter;
        } preemption;
      struct __attribute__((__packed__))
        {
          Mword irq_obj;
          int irq_number;
        } irq;
      struct __attribute__((__packed__))
        {
            char _pad[30];
        } fit;
    } m;
} l4_tracebuffer_entry_t;

#endif
