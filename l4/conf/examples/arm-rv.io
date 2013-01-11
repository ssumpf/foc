hw-root
{
  NIC => new Device()
  {
    .hid = "smsc911x";
    new-res Mmio(0x4e000000 .. 0x4e000fff);
    new-res Irq(60);
  }
}

l4lx => new System_bus()
{
  NIC => wrap(hw-root.NIC);
} 
