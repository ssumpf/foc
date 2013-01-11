// --------------------------------------------------------------------------
INTERFACE [arm && tegra2 && mptimer]:

EXTENSION class Timer
{
private:
  static Mword interval() { return 249999; }
};
