/**
 * \file
 * \brief   L4 kernel event tracing
 * \ingroup api_calls
 */
/*
 * (c) 2008-2009 Adam Lackorzynski <adam@os.inf.tu-dresden.de>,
 *               Björn Döbel <doebel@os.inf.tu-dresden.de>,
 *               Torsten Frenzel <frenzel@os.inf.tu-dresden.de>
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
#ifndef __L4_KTRACE_H__
#define __L4_KTRACE_H__

#include <l4/sys/types.h>
#include <l4/sys/kdebug.h>

#define LOG_EVENT_CONTEXT_SWITCH   0  /**< Event: context switch
				       **  \ingroup api_calls_fiasco
				       **/
#define LOG_EVENT_IPC_SHORTCUT     1  /**< Event: IPC shortcut
				       **  \ingroup api_calls_fiasco
				       **/
#define LOG_EVENT_IRQ_RAISED       2  /**< Event: IRQ occurred
				       **  \ingroup api_calls_fiasco
				       **/
#define LOG_EVENT_TIMER_IRQ        3  /**< Event: Timer IRQ occurred
				       **  \ingroup api_calls_fiasco
				       **/
#define LOG_EVENT_THREAD_EX_REGS   4  /**< Event: thread_ex_regs
				       **  \ingroup api_calls_fiasco
				       **/

#define LOG_EVENT_MAX_EVENTS      16  /**< Maximum number of events
				       **  \ingroup api_calls_fiasco
				       **/

/**
 * Trace buffer status.
 * \ingroup api_calls_fiasco
 */
typedef struct
{
  /// Address of trace buffer 0
  l4_umword_t tracebuffer0;
  /// Size of trace buffer 0
  l4_umword_t size0;
  /// Version number of trace buffer 0 (incremented if tb0 overruns)
  l4_umword_t version0;
  /// Address of trace buffer 1 (there is no gap between tb0 and tb1)
  l4_umword_t tracebuffer1;
  /// Size of trace buffer 1 (same as tb0)
  l4_umword_t size1;
  /// Version number of trace buffer 1 (incremented if tb1 overruns)
  l4_umword_t version1;
  /// Available LOG events
  l4_umword_t logevents[LOG_EVENT_MAX_EVENTS];

  /// Scaler used for translation of CPU cycles to nano seconds
  l4_umword_t scaler_tsc_to_ns;
  /// Scaler used for translation of CPU cycles to micro seconds
  l4_umword_t scaler_tsc_to_us;
  /// Scaler used for translation of nano seconds to CPU cycles
  l4_umword_t scaler_ns_to_tsc;

  /// Number of context switches (intra AS or inter AS)
  l4_umword_t cnt_context_switch;
  /// Number of inter AS context switches
  l4_umword_t cnt_addr_space_switch;
  /// How often was the IPC shortcut not taken
  l4_umword_t cnt_shortcut_failed;
  /// How often was the IPC shortcut taken
  l4_umword_t cnt_shortcut_success;
  /// Number of hardware interrupts (without kernel scheduling interrupt)
  l4_umword_t cnt_irq;
  /// Number of long IPCs
  l4_umword_t cnt_ipc_long;

} l4_tracebuffer_status_t;

/**
 * Return trace buffer status.
 * \ingroup api_calls_fiasco
 *
 * \return Pointer to trace buffer status struct.
 */
L4_INLINE l4_tracebuffer_status_t *
fiasco_tbuf_get_status(void);

/**
 * Return the physical address of the trace buffer status struct.
 * \ingroup api_calls_fiasco
 *
 * \return physical address of status struct.
 */
L4_INLINE l4_addr_t
fiasco_tbuf_get_status_phys(void);

/**
 * Create new trace buffer entry with describing \<text\>.
 * \ingroup api_calls_fiasco
 *
 * \param  text   Logging text
 * \return Pointer to trace buffer entry
 */
L4_INLINE l4_umword_t
fiasco_tbuf_log(const char *text);

/**
 * Create new trace buffer entry with describing \<text\> and three additional
 * values.
 * \ingroup api_calls_fiasco
 *
 * \param  text   Logging text
 * \param  v1     first value
 * \param  v2     second value
 * \param  v3     third value
 * \return Pointer to trace buffer entry
 */
L4_INLINE l4_umword_t
fiasco_tbuf_log_3val(const char *text, l4_umword_t v1, l4_umword_t v2, l4_umword_t v3);

/**
 * Create new trace buffer entry with binary data.
 * \ingroup api_calls_fiasco
 *
 * \param  data       binary data
 * \return Pointer to trace buffer entry
 */
L4_INLINE l4_umword_t
fiasco_tbuf_log_binary(const unsigned char *data);

/**
 * Clear trace buffer.
 * \ingroup api_calls_fiasco
 */
L4_INLINE void
fiasco_tbuf_clear(void);

/**
 * Dump trace buffer to kernel console.
 * \ingroup api_calls_fiasco
 */
L4_INLINE void
fiasco_tbuf_dump(void);

L4_INLINE void
fiasco_timer_disable(void);

L4_INLINE void
fiasco_timer_enable(void);

/*****************************************************************************
 *** Implementation
 *****************************************************************************/

L4_INLINE l4_tracebuffer_status_t *
fiasco_tbuf_get_status(void)
{
  /* Not implemented */
  return (l4_tracebuffer_status_t *)__KDEBUG_ARM_PARAM_1(29, 0);
}

L4_INLINE l4_addr_t
fiasco_tbuf_get_status_phys(void)
{
  enter_kdebug("fiasco_tbuf_get_status_phys not there");
  return ~0UL;
}

L4_INLINE l4_umword_t
fiasco_tbuf_log(const char *text)
{
  return __KDEBUG_ARM_PARAM_2(29, 1, text);
}

L4_INLINE l4_umword_t
fiasco_tbuf_log_3val(const char *text, l4_umword_t v1, l4_umword_t v2, l4_umword_t v3)
{
  return __KDEBUG_ARM_PARAM_5(29, 4, text, v1, v2, v3);
}

L4_INLINE void
fiasco_tbuf_clear(void)
{
  __KDEBUG_ARM_PARAM_1(29, 2);
}

L4_INLINE void
fiasco_tbuf_dump(void)
{
  __KDEBUG_ARM_PARAM_1(29, 3);
}

L4_INLINE void
fiasco_timer_disable(void)
{
  __KDEBUG_ARM_PARAM_1(29, 6);
}

L4_INLINE void
fiasco_timer_enable(void)
{
  __KDEBUG_ARM_PARAM_1(29, 6);
}

L4_INLINE l4_umword_t
fiasco_tbuf_log_binary(const unsigned char *data)
{
  return __KDEBUG_ARM_PARAM_2(29, 8, data);
}

#endif

