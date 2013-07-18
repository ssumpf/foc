// --------------------------------------------------------------------------
INTERFACE [arm && mptimer && omap4_pandaboard]:

EXTENSION class Timer
{
private:
  static Mword interval()
  {
    /*
     * This is only valid for Panda ES2, as we run it at our offices. For ES2
     * U-Boot will clock the board at 700 MHz leading to a 350 MHz private timer
     * tick.
     */
    return 349999;
  }
};
