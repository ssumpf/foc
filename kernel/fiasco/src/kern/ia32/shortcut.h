#ifndef SHORTCUT_H
#define SHORTCUT_H

#include "globalconfig.h"

// thread_state consts
#define Thread_ready			0x1
#define Thread_utcb_ip_sp		0x2
#define Thread_receiving		0x4
#define Thread_polling			0x8
#define	Thread_ipc_in_progress		0x10
#define Thread_send_in_progress		0x20
#define Thread_busy			0x40
#define Thread_cancel			0x100
#define Thread_dead			0x200
#define Thread_delayed_deadline		0x2000
#define Thread_delayed_ipc		0x4000
#define Thread_fpu_owner		0x8000
#define Thread_alien_or_vcpu_user	0x810000
#define Thread_dis_alien		0x20000
#define Thread_transfer_in_progress     0x80000

#define Thread_ipc_sending_mask        (Thread_send_in_progress		| \
					Thread_polling)
#define Thread_ipc_receiving_mask      (Thread_receiving		| \
					Thread_busy                     | \
                                        Thread_transfer_in_progress)
#define Thread_ipc_mask                (Thread_ipc_in_progress		| \
					Thread_ipc_sending_mask		| \
					Thread_ipc_receiving_mask)

// stackframe structure
#ifdef CONFIG_BIT32
#define REG_ECX
#define REG_EDX	(1*4)
#define REG_ESI	(2*4)
#define REG_EDI	(3*4)
#define REG_EBX	(4*4)
#define REG_EBP	(5*4)
#define REG_EAX	(6*4)
#define REG_EIP (7*4)
#define REG_CS	(8*4)
#define REG_EFL	(9*4)
#define REG_ESP	(10*4)
#define REG_SS	(11*4)
#else



/*
#define REG_RAX		(THREAD_BLOCK_SIZE - 6*8)
#define REG_RBP		(THREAD_BLOCK_SIZE - 7*8)
#define REG_RBX		(THREAD_BLOCK_SIZE - 8*8)
#define REG_RDI		(THREAD_BLOCK_SIZE - 9*8)
#define REG_RSI		(THREAD_BLOCK_SIZE - 10*8)

#define REG_RDX		(THREAD_BLOCK_SIZE - 11*8)	
#define REG_RCX		(THREAD_BLOCK_SIZE - 12*8)

#define REG_R8		(THREAD_BLOCK_SIZE - 13*8)
#define REG_R9		(THREAD_BLOCK_SIZE - 14*8)
#define REG_R10		(THREAD_BLOCK_SIZE - 15*8)
#define REG_R11		(THREAD_BLOCK_SIZE - 16*8)
#define REG_R12		(THREAD_BLOCK_SIZE - 17*8)
#define REG_R13		(THREAD_BLOCK_SIZE - 18*8)
#define REG_R14		(THREAD_BLOCK_SIZE - 19*8)
#define REG_R15		(THREAD_BLOCK_SIZE - 20*8)
*/

#define REG_R15
#define REG_R14 (1*8)
#define REG_R13 (2*8)
#define REG_R12 (3*8)
#define REG_R11 (4*8)
#define REG_R10 (5*8)
#define REG_R9  (6*8)
#define REG_R8  (7*8)
#define REG_RCX (8*8)
#define REG_RDX	(9*8)
#define REG_RSI	(10*8)
#define REG_RDI	(11*8)
#define REG_RBX	(12*8)
#define REG_RBP	(13*8)
#define REG_RAX	(14*8)
#define REG_RIP (15*8)
#define REG_CS	(16*8)
#define REG_RFL	(17*8)
#define REG_RSP	(18*8)
#define REG_SS	(19*8)

#endif

#ifdef CONFIG_ABI_X0
#  define RETURN_DOPE 0x6000 // three dwords
#  define TCB_ADDRESS_MASK 0x01fff800
#else
#  define RETURN_DOPE 0x4000 // two dwords
#  define TCB_ADDRESS_MASK 0x1ffff800
#endif


#if defined(CONFIG_JDB) && defined(CONFIG_JDB_ACCOUNTING)

#define CNT_CONTEXT_SWITCH	incl (VAL__MEM_LAYOUT__TBUF_STATUS_PAGE+ \
				    OFS__TBUF_STATUS__KERNCNTS)
#define CNT_ADDR_SPACE_SWITCH	incl (VAL__MEM_LAYOUT__TBUF_STATUS_PAGE+ \
				    OFS__TBUF_STATUS__KERNCNTS + 4)
#define CNT_SHORTCUT_FAILED	incl (VAL__MEM_LAYOUT__TBUF_STATUS_PAGE+ \
				    OFS__TBUF_STATUS__KERNCNTS + 8)
#define CNT_SHORTCUT_SUCCESS	incl (VAL__MEM_LAYOUT__TBUF_STATUS_PAGE+ \
				    OFS__TBUF_STATUS__KERNCNTS + 12)
#define CNT_IOBMAP_TLB_FLUSH	incl (VAL__MEM_LAYOUT__TBUF_STATUS_PAGE+ \
				    OFS__TBUF_STATUS__KERNCNTS + 40)

#else

#define CNT_CONTEXT_SWITCH
#define CNT_ADDR_SPACE_SWITCH
#define CNT_SHORTCUT_FAILED
#define CNT_SHORTCUT_SUCCESS
#define CNT_IOBMAP_TLB_FLUSH

#endif

#endif
