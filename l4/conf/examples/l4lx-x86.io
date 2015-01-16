input => new System_bus()
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
    
  x1 => wrap(hw-root.match("BIOS"));
  x2 => wrap(hw-root.match("PNP0900"));
  x3 => wrap(hw-root.match("PNP0100"));
} 

l4linux => new System_bus()
{
  # Add a new virtual PCI root bridge
  PCI0 => new PCI_bus()
  {
    pci_l4x[] => wrap(hw-root.match("PCI/CC_02,PCI/CC_01,PCI/CC_04"));
  }
}
