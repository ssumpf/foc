INTERFACE:

#include <types.h>
#include <pm.h>

/**
 * Encapsulation of the platforms interrupt controller
 */
class Pic : public Pm_object
{
public:
  /**
   * The type holding the saved Pic state.
   */
  typedef unsigned Status;

  /**
   * Static initalization of the interrupt controller
   */
  static void init();

  /**
   * Disable the given irq (without lock protection).
   * @pre The Cpu_lock must be locked (cli'ed)
   *
   * This function must be implemented in the
   * architecture specific part (e.g. pic-i8259.cpp).
   */
  static void disable_locked( unsigned irqnum );

  /**
   * Enable the given irq (without lock protection).
   * @pre The Cpu_lock must be locked (cli'ed)
   *
   * This function must be implemented in the
   * architecture specific part (e.g. pic-i8259.cpp).
   */
  static void enable_locked(unsigned irqnum, unsigned prio = 0);

  /**
   * Temporarily block the IRQ til the next ACK.
   * @pre The Cpu_lock must be locked (cli'ed)
   *
   * This function must be implemented in the
   * architecture specific part (e.g. pic-i8259.cpp).
   */
  static void block_locked(unsigned irqnum);

  /**
   * Disable all IRQ's and and return the old Pic state.
   * @pre Must be done with disabled interrupts.
   */
  static Status disable_all_save();

  /**
   * Restore the IRQ's to the saved state s.
   * @pre Must be done with disabled interrupts.
   * @param s, the saved state.
   */
  static void restore_all( Status s );

  /**
   * Acknowledge the given IRQ.
   * @pre The Cpu_lock must be locked (cli'ed).
   * @param irq, the irq to acknowledge.
   *
   * This function must be implemented in the
   * architecture specific part (e.g. pic-i8259.cpp).
   */
  static void acknowledge_locked( unsigned irq );

  /**
   * Set CPU affinity of IRQ.
   * @param irq IRQ.
   * @param cpu Logical CPU.
   */
  static void set_cpu(unsigned irq, Cpu_number cpu);

  /**
   * Make Pic power management aware.
   */
  void pm_on_suspend(Cpu_number)
    {
      saved_state = disable_all_save();
    }

  void pm_on_resume(Cpu_number)
    {
      restore_all(saved_state);
    }

private:
  Status saved_state;
};

