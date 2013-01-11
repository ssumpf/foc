ENTRY(_start)
SECTIONS
{
  . = 0x01000000;
  _image_start = .;
  .text :
  {
    *(.text .text.*)
    *(.rodata .rodata.*)
  } =0x9090

  .data :
  {
    *(.data .data.*)
    *(.bss .bss.*)
    *(COMMON)
  }
  _image_end = .;

  /DISCARD/ : {
    *(.interp)
    *(.comment)
    *(.note)
    *(.eh_frame)
  }
}
