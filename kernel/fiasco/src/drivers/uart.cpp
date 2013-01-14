INTERFACE:

#include "console.h"

/**
 * Platform independent UART stub.
 */
class Uart : public Console
{
public:
  /**
   * Type UART transfer mode (Bits, Stopbits etc.).
   */
  typedef unsigned TransferMode;

  /**
   * Type for baud rate.
   */
  typedef unsigned BaudRate;

  /* These constants must be defined in the 
     arch part of the uart. To define them there
     has the advantage of most efficent definition
     for the hardware.

  static unsigned const PAR_NONE = xxx;
  static unsigned const PAR_EVEN = xxx;
  static unsigned const PAR_ODD  = xxx;
  static unsigned const DAT_5    = xxx;
  static unsigned const DAT_6    = xxx;
  static unsigned const DAT_7    = xxx;
  static unsigned const DAT_8    = xxx;
  static unsigned const STOP_1   = xxx;
  static unsigned const STOP_2   = xxx;

  static unsigned const MODE_8N1 = PAR_NONE | DAT_8 | STOP_1;
  static unsigned const MODE_7E1 = PAR_EVEN | DAT_7 | STOP_1;

  // these two values are to leave either mode
  // or baud rate unchanged on a call to change_mode
  static unsigned const MODE_NC  = xxx;
  static unsigned const BAUD_NC  = xxx;

  */


public:
  /* Interface definition - implemented in the arch part */
  /// ctor
  Uart();

  /// dtor
  ~Uart();

  /**
   * (abstract) Shutdown the serial port.
   */
  void shutdown();

  /**
   * (abstract) Get the IRQ assigned to the port.
   */
  int irq() const;

  Address base() const;

  /**
   * (abstract) Enable rcv IRQ in UART.
   */
  void enable_rcv_irq();

  /**
   * (abstract) Disable rcv IRQ in UART.
   */
  void disable_rcv_irq();

  /**
   * (abstract) Change transfer mode or speed.
   * @param m the new mode for the transfer, or MODE_NC for no mode change.
   * @param r the new baud rate, or BAUD_NC, for no speed change.
   */
  bool change_mode(TransferMode m, BaudRate r);

  /**
   * (abstract) Get the current transfer mode.
   */
  TransferMode get_mode();

  /**
   * (abstract) Write str.
   */
  int write( char const *str, size_t len );

  /**
   * (abstract) Read a character.
   */
  int getchar( bool blocking = true );

  /**
   * (abstract) Is there anything to read?
   */
  int char_avail() const;
  
  Mword get_attributes() const;
};

IMPLEMENTATION:

IMPLEMENT
Mword
Uart::get_attributes() const
{
  return UART | IN | OUT;
}

//---------------------------------------------------------------------------
INTERFACE [libuart]:

#include "uart_base.h"

EXTENSION class Uart
{
public:
  enum
  {
    MODE_8N1 = 1,
  };

  static L4::Uart *uart();
};


//---------------------------------------------------------------------------
IMPLEMENTATION [libuart]:


IMPLEMENT inline Uart::~Uart()
{ }

IMPLEMENT inline void Uart::shutdown()
{
  uart()->shutdown();
}

IMPLEMENT inline bool Uart::change_mode(TransferMode m, BaudRate baud)
{
  return uart()->change_mode(m, baud);
}

IMPLEMENT inline
int Uart::write(const char *s, __SIZE_TYPE__ count)
{
  return uart()->write(s, count);
}

IMPLEMENT inline
int Uart::getchar(bool blocking)
{
  return uart()->get_char(blocking);
}


IMPLEMENT inline
int Uart::char_avail() const
{
  return uart()->char_avail();
}

IMPLEMENT
void Uart::enable_rcv_irq()
{
  uart()->enable_rx_irq(true);
}

IMPLEMENT
void Uart::disable_rcv_irq()
{
  uart()->enable_rx_irq(false);
}

