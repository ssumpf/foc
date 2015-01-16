# vim:set ft=ioconfig:
# configuration file for io

gui => new System_bus()
{
  ps2dev => wrap(hw-root.match("PNP0303,PNP0F13"));
}

fbdrv => new System_bus()
{
  PCI0 => new PCI_bus_ident()
  {
    host_bridge_dummy => new PCI_dummy_device();
    pci_gfx[] => wrap(hw-root.match("PCI/CC_03"));
  }
  dev1 => wrap(hw-root.match("BIOS"));
  dev2 => wrap(hw-root.match("PNP0900"));
  dev3 => wrap(hw-root.match("PNP0100"));
}
