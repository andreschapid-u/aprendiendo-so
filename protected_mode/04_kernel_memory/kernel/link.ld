/** 
   @file
   @author Erwin Meza <emezav@gmail.com> 
   @brief Archivo  para realizar el enlace (linking) del kernel.
   Este archivo toma los archivos de objeto y organiza el archivo
   final. 
*/
/** @verbatim */
/* 
   Se supone que el kernel se carga en la direcci�n de memoria 0x100000.
*/

phys = 0x100000;
virt_offset = 0xC0000000;

virt = phys + virt_offset;
start_offset = phys + (start - virt);

ENTRY(start_offset);
OUTPUT_FORMAT(elf32-i386);
SECTIONS {
   . = virt;
	/* direcci�n f�sica en la cual se carga el kernel */
	kernel_phys_start = . - virt_offset;	
	/* Segmento de texto */
   .text ALIGN (0x1000): AT(code_start - virt_offset) {
    /* Inicio de la secci�n de c�digo */
     code_start = .;	
    /* Incluir primero la secci�n de c�digo de start.S llamada .startcode */
     *(.startcode) 
     *(.text)
     *(.rodata)
   } = 0x90909090
   kernel_code_phys_end = . - virt_offset;
   .data ALIGN(0x1000) : AT (data_start - virt_offset) {
       data_start = .;
    /* Incluir primero la secci�n de c�digo de start.S llamada .startcode */
	   *(.startdata) 
       *(.data)
   } = 0x00000000
   data_phys_end = . - virt_offset;
   .bss : AT (bss_start - virt_offset) {
       bss_start = .;
       *(.bss)
       *(COMMON)
   } = 0x00000000

   . = ALIGN(4096);

   bss_phys_end = . - virt_offset;

   kernel_phys_end = bss_phys_end;

   /* Tama�o del kernel en bytes */
   kernel_size_bytes = kernel_phys_end - kernel_phys_start;

   /* N�mero de p�ginas de 4096 bytes que ocupa el kernel */
   num_kernel_pages = (kernel_size_bytes / 0x1000) + 1;

   /* N�mero de tablas de p�gina necesarias para mapear el kernel */
   num_kernel_page_tables = (num_kernel_pages / 1024) + 1;

   /* N�mero de p�ginas que ocupa el primer MB de memoria y el kernel */
   tmp_total_kernel_pages = (0x100000 / 0x1000) + num_kernel_pages;
}

/** @endverbatim */
