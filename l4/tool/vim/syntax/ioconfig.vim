" Vim syntax file for io configuration language
" Language:    IO configuration language
" Maintainer:  Adam Lackorzynski <adam@os.inf.tu-dresden.de>
" Last Change: 2010 June

if exists("b:current_syntax")
  finish
endif

syn clear
syn case match

setlocal iskeyword+=.
setlocal iskeyword+=-
syn keyword ioconfigStatement       hw-root hw-root.match Device new System_bus wrap new-res .hid Irq Io Mmio Mmio_ram PCI_bus PCI_bus_ident
syn match   ioconfigComment         /#.*/

hi def link ioconfigStatement      Type
hi def link ioconfigComment        Comment

let b:current_syntax = "ioconfig"
