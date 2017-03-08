// --------------------------------------------------------------------------
INTERFACE [arm && mptimer && omap4_pandaboard]:

EXTENSION class Timer
{
private:
  static Mword interval()
  {
    /*
     * This is only valid for Panda A6, as we run it at our offices. For A6
     * U-Boot will clock the board at 800 MHz leading to a 400 MHz private timer
     * tick.
     */
    return 399999;
  }
};
