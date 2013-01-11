# vim:set ft=ioconfig:
# configuration file for io

gui => new System_bus()
{
  KBD => wrap(hw-root.KBD);
  MOUSE => wrap(hw-root.MOUSE);
}

fbdrv => new System_bus()
{
  CTRL => wrap(hw-root.CTRL);
  LCD => wrap(hw-root.LCD);
}
