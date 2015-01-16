INTERFACE[i8259]:

#include "initcalls.h"

EXTENSION class Pic
{
public:
  enum
  {
    MASTER_PIC_BASE = 0x20,
    SLAVES_PIC_BASE = 0xa0,
    OFF_ICW	    = 0x00,
    OFF_OCW	    = 0x01,

    MASTER_ICW	    = MASTER_PIC_BASE + OFF_ICW,
    MASTER_OCW	    = MASTER_PIC_BASE + OFF_OCW,
    SLAVES_ICW	    = SLAVES_PIC_BASE + OFF_ICW,
    SLAVES_OCW	    = SLAVES_PIC_BASE + OFF_OCW,


    /*
    **	ICW1				
    */

    ICW_TEMPLATE    = 0x10,
    
    LEVL_TRIGGER    = 0x08,
    EDGE_TRIGGER    = 0x00,
    ADDR_INTRVL4    = 0x04,
    ADDR_INTRVL8    = 0x00,
    SINGLE__MODE    = 0x02,
    CASCADE_MODE    = 0x00,
    ICW4__NEEDED    = 0x01,
    NO_ICW4_NEED    = 0x00,

    /*
    **	ICW3				
    */

    SLAVE_ON_IR0    = 0x01,
    SLAVE_ON_IR1    = 0x02,
    SLAVE_ON_IR2    = 0x04,
    SLAVE_ON_IR3    = 0x08,
    SLAVE_ON_IR4    = 0x10,
    SLAVE_ON_IR5    = 0x20,
    SLAVE_ON_IR6    = 0x40,
    SLAVE_ON_IR7    = 0x80,
    
    I_AM_SLAVE_0    = 0x00,
    I_AM_SLAVE_1    = 0x01,
    I_AM_SLAVE_2    = 0x02,
    I_AM_SLAVE_3    = 0x03,
    I_AM_SLAVE_4    = 0x04,
    I_AM_SLAVE_5    = 0x05,
    I_AM_SLAVE_6    = 0x06,
    I_AM_SLAVE_7    = 0x07,

    /*
    **	ICW4				
    */
    
    SNF_MODE_ENA    = 0x10,
    SNF_MODE_DIS    = 0x00,
    BUFFERD_MODE    = 0x08,
    NONBUFD_MODE    = 0x00,
    AUTO_EOI_MOD    = 0x02,
    NRML_EOI_MOD    = 0x00,
    I8086_EMM_MOD   = 0x01,
    SET_MCS_MODE    = 0x00,

    /*
    **	OCW1				
    */
    
    PICM_MASK	    = 0xFF,
    PICS_MASK	    = 0xFF,
    
    /*
    **	OCW2				
    */
    
    NON_SPEC_EOI    = 0x20,
    SPECIFIC_EOI    = 0x60,
    ROT_NON_SPEC    = 0xa0,
    SET_ROT_AEOI    = 0x80,
    RSET_ROTAEOI    = 0x00,
    ROT_SPEC_EOI    = 0xe0,
    SET_PRIORITY    = 0xc0,
    NO_OPERATION    = 0x40,
    
    SND_EOI_IR0    = 0x00,
    SND_EOI_IR1    = 0x01,
    SND_EOI_IR2    = 0x02,
    SND_EOI_IR3    = 0x03,
    SND_EOI_IR4    = 0x04,
    SND_EOI_IR5    = 0x05,
    SND_EOI_IR6    = 0x06,
    SND_EOI_IR7    = 0x07,
 
    /*
    **	OCW3				
    */
    
    OCW_TEMPLATE    = 0x08,
    SPECIAL_MASK    = 0x40,
    MASK_MDE_SET    = 0x20,
    MASK_MDE_RST    = 0x00,
    POLL_COMMAND    = 0x04,
    NO_POLL_CMND    = 0x00,
    READ_NEXT_RD    = 0x02,
    READ_IR_ONRD    = 0x00,
    READ_IS_ONRD    = 0x01,
    
    /*
    **	Standard PIC initialization values for PCs.
    */
    PICM_ICW1	    = (ICW_TEMPLATE | EDGE_TRIGGER | ADDR_INTRVL8 
		       | CASCADE_MODE | ICW4__NEEDED),
    PICM_ICW3	    = SLAVE_ON_IR2,
    PICM_ICW4	    = (SNF_MODE_DIS | NONBUFD_MODE | NRML_EOI_MOD 
		       | I8086_EMM_MOD),

    PICS_ICW1	    = (ICW_TEMPLATE | EDGE_TRIGGER | ADDR_INTRVL8 
		       | CASCADE_MODE | ICW4__NEEDED),
    PICS_ICW3	    = I_AM_SLAVE_2,
    PICS_ICW4	    = (SNF_MODE_DIS | NONBUFD_MODE | NRML_EOI_MOD
		       | I8086_EMM_MOD),
  };

  static int special_fully_nested_mode;
};

IMPLEMENTATION[i8259]:

#include <cstring>
#include <cstdio>
#include <cassert>

#include "io.h"
#include "config.h"
#include "initcalls.h"
#include "koptions.h"
#include "mem_layout.h"

int Pic::special_fully_nested_mode = 1; // be compatible with Jochen's L4

IMPLEMENT FIASCO_INIT
void
Pic::init()
{
  pic_init(0x20,0x28);
}

PUBLIC static inline
unsigned
Pic::nr_irqs()
{ return 16; }


//
// this version of pic_init() overrides the OSKIT's.
//
// differences to standard AT initialization: 
// . we use the "special fully nested mode" to let irq sources on the
//   slave irq raise nested
// . we give irq 2 (= slave pic) the highest prio on the master pic; this
//   results in the following sequence of prios: 8-15,3-7,0,1
//   this way, the timer interrupt on irq 8 always gets thru (even if 
//   some user irq handler doesn't acknowledge its irq!)
//
static FIASCO_INIT
void
Pic::pic_init(unsigned char master_base, unsigned char slave_base)
{
  // disallow all interrupts before we selectively enable them 
  Pic::disable_all_save();
  /*
   * Set the LINTEN bit in the HyperTransport Transaction
   * Control Register.
   *
   * This will cause EXTINT and NMI interrupts routed over the
   * hypertransport bus to be fed into the LAPIC LINT0/LINT1.  If
   * the bit isn't set, the interrupts will go to the general cpu
   * INTR/NMI pins.  On a dual-core cpu the interrupt winds up
   * going to BOTH cpus.  The first cpu that does the interrupt ack
   * cycle will get the correct interrupt.  The second cpu that does
   * it will get a spurious interrupt vector (typically IRQ 7).
   */
#if 0
  if ((cpu_id & 0xff0) == 0xf30) 
#endif
    {
#if 0
      Unsigned32 tcr;
      Io::out32(0x0cf8,
	  (1 << 31) | /* enable */
	  (0 << 16) | /* bus */
	  (24 << 11) |        /* dev (cpu + 24) */
	  (0 << 8) |  /* func */
	  0x68                /* reg */
	  );
      tcr = Io::in32(0xcfc);
      if ((tcr & 0x00010000) == 0) {
	  Io::out32(0xcfc, tcr|0x00010000);
	  printf("AMD: Rerouting HyperTransport "
	      "EXTINT/NMI to APIC\n");
      }
      Io::out32(0x0cf8, 0);
#endif
  }
  // VMware isn't able to deal with the special fully nested mode
  // correctly so we simply don't use it while running under
  // VMware. Otherwise VMware will barf with 
  // *** VMware Workstation internal monitor error ***
  // BUG F(152):393 bugNr=4388

  if (Koptions::o()->opt(Koptions::F_nosfn))
    special_fully_nested_mode = 0;

  if (special_fully_nested_mode)
    {
      puts ("Enabling special fully nested mode for PIC");
      /* Initialize the master. */

      Io::out8_p(PICM_ICW1, MASTER_ICW);
      Io::out8_p(master_base, MASTER_OCW);
      Io::out8_p(PICM_ICW3, MASTER_OCW);
      Io::out8_p(SNF_MODE_ENA | PICM_ICW4, MASTER_OCW);

      /* Initialize the slave. */
      Io::out8_p(PICS_ICW1, SLAVES_ICW);
      Io::out8_p(slave_base, SLAVES_OCW);
      Io::out8_p(PICS_ICW3, SLAVES_OCW);
      Io::out8_p(SNF_MODE_ENA | PICS_ICW4, SLAVES_OCW);

      // the timer interrupt should have the highest priority so that it
      // always gets through
      if (Config::Pic_prio_modify
	  && (int)Config::Scheduler_mode == Config::SCHED_RTC)
	{
	  // setting specific rotation (specific priority) 
	  // -- see Intel 8259A reference manual
	  // irq 1 on master hast lowest prio
	  // => irq 2 (cascade) = irqs 8..15 have highest prio
	  Io::out8_p(SET_PRIORITY | 1, MASTER_ICW);
	  // irq 7 on slave has lowest prio
	  // => irq 0 on slave (= irq 8) has highest prio
	  Io::out8_p(SET_PRIORITY | 7, SLAVES_ICW);
	}
    }
  else
    {
      printf("%sUsing (normal) fully nested PIC mode\n",
	  Config::found_vmware ? "Found VMware: " : "");

      /* Initialize the master. */
      Io::out8_p(PICM_ICW1, MASTER_ICW);
      Io::out8_p(master_base, MASTER_OCW);
      Io::out8_p(PICM_ICW3, MASTER_OCW);
      Io::out8_p(PICM_ICW4, MASTER_OCW);

      /* Initialize the slave. */
      Io::out8_p(PICS_ICW1, SLAVES_ICW);
      Io::out8_p(slave_base, SLAVES_OCW);
      Io::out8_p(PICS_ICW3, SLAVES_OCW);
      Io::out8_p(PICS_ICW4, SLAVES_OCW);
    }

  // set initial masks
  Io::out8_p(0xfb, MASTER_OCW);	// unmask irq2
  Io::out8_p(0xff, SLAVES_OCW);	// mask everything

  /* Ack any bogus intrs by setting the End Of Interrupt bit. */
  Io::out8_p(NON_SPEC_EOI, MASTER_ICW);
  Io::out8_p(NON_SPEC_EOI, SLAVES_ICW);

  // disallow all interrupts before we selectively enable them 
  Pic::disable_all_save();

}

IMPLEMENT inline NEEDS["io.h"]
void
Pic::disable_locked(unsigned irq)
{
  if (irq < 8)
    Io::out8(Io::in8(MASTER_OCW) | (1 << irq), MASTER_OCW);
  else
    Io::out8(Io::in8(SLAVES_OCW) | (1 << (irq-8)), SLAVES_OCW);
}

IMPLEMENT inline NEEDS["io.h"]
void
Pic::enable_locked(unsigned irq, unsigned /*prio*/)
{
  if (irq < 8)
    Io::out8(Io::in8(MASTER_OCW) & ~(1 << irq), MASTER_OCW);
  else
    {
      Io::out8(Io::in8(MASTER_OCW) & ~(1 << 2), MASTER_OCW);
      Io::out8(Io::in8(SLAVES_OCW) & ~(1 << (irq-8)), SLAVES_OCW);
    }
}

IMPLEMENT inline NEEDS["io.h"]
void
Pic::acknowledge_locked(unsigned irq)
{
  if (irq >= 8)
    {
      Io::out8(NON_SPEC_EOI, SLAVES_ICW); // EOI slave
      if (special_fully_nested_mode)
	{
	  Io::out8(OCW_TEMPLATE | READ_NEXT_RD | READ_IS_ONRD, SLAVES_ICW);
	  if (Io::in8(SLAVES_ICW))      // slave still active?
	    return;                 // -- don't EOI master
	}
    }
  Io::out8(NON_SPEC_EOI, MASTER_ICW); // EOI master
}

IMPLEMENT inline
void
Pic::block_locked(unsigned)
{}

IMPLEMENT inline NEEDS["io.h"]
Pic::Status
Pic::disable_all_save()
{
  Status s;
  s  = Io::in8(MASTER_OCW);
  s |= Io::in8(SLAVES_OCW) << 8;
  Io::out8( 0xff, MASTER_OCW );
  Io::out8( 0xff, SLAVES_OCW );
  
  return s;
}

IMPLEMENT inline NEEDS["io.h"]
void
Pic::restore_all( Status s )
{
  Io::out8( s & 0x0ff, MASTER_OCW );
  Io::out8( (s >> 8) & 0x0ff, SLAVES_OCW );
}


IMPLEMENT inline NEEDS[<cassert>]
void
Pic::set_cpu(unsigned, Cpu_number cpu)
{
  (void)cpu;
  assert(cpu == Cpu_number::boot_cpu());
}

