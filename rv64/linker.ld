/* https://wiki.osdev.org/Bare_Bones#Linking_the_Kernel 
   The bootloader will look at this image and start execution at the symbol
   designated as the entry point. */
/* https://github.com/s-rah/pseudos */
OUTPUT_ARCH( "riscv" )

ENTRY(_start)

MEMORY
{
  ram  (wxa) : ORIGIN = 0x80000000, LENGTH = 128M
}

PHDRS
{
  text PT_LOAD;
  data PT_LOAD;
  bss PT_LOAD;
}


SECTIONS
{
  . = 0x80000000;
  .text : {
    PROVIDE(_text_start = .);
    *(.text.init) *(.text .text.*)
    PROVIDE(_text_end = .);
  } >ram AT>ram :text

   PROVIDE(_global_pointer = .);

  .rodata : {
    PROVIDE(_rodata_start = .);
    *(.rodata .rodata.*)
    PROVIDE(_rodata_end = .);
  } >ram AT>ram :text

  .data : {
    . = ALIGN(4096);
    PROVIDE(_data_start = .);
    *(.sdata .sdata.*) *(.data .data.*)
    PROVIDE(_data_end = .);
  } >ram AT>ram :data

  .bss : {
    PROVIDE(_bss_start = .);
    *(.sbss .sbss.*) *(.bss .bss.*)
    PROVIDE(_bss_end = .);
  } >ram AT>ram :bss


  PROVIDE(_memory_start = ORIGIN(ram));
  PROVIDE(_memory_end = ORIGIN(ram) + LENGTH(ram));
  PROVIDE(_heap_end = _memory_end - 4096);
  PROVIDE(_heap_counter = _heap_end + 8);
  PROVIDE(_uart_base_addr = 0x10000000);

}
