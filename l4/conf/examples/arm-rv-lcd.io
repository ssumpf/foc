-- vi:ft=lua
-- configuration file for io

local hw = Io.system_bus()

Io.add_vbus("gui", Io.Vi.System_bus
{
  KBD = wrap(hw:match("arm,pl050","AMBA KMI Kbd"));
  MOUSE = wrap(hw:match("arm,pl050","AMBA KMI mou"));
})

Io.add_vbus("fbdrv", Io.Vi.System_bus
{
  CTRL = wrap(hw:match("arm,sysctl"));
  LCD = wrap(hw:match("arm,pl111"));
})
