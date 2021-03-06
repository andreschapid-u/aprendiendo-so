/**
 * @file
 * @ingroup kernel_code 
 * @author Erwin Meza <emezav@gmail.com>
 * @copyright GNU Public License. 
 *
 * @brief Código de inicialización del kernel en C
 *
 * Este codigo recibe el control de start.S y continúa con la ejecución.
*/
#include <asm.h>

#include <console.h>

#include <irq.h>
#include <pci.h>
#include <stdlib.h>
#include <string.h>
#include <kmem.h>
#include <kcache.h>

void cmain(){

    /* Inicializar y limpiar la consola console.c*/
    console_clear();

     /* Inicializar la estructura para gestionar la memoria física. physmem.c*/
    setup_physical_memory();

    /* Configura la IDT y el PIC.interrupt.c */
    setup_interrupts();
    
    /* Completa la configuración de la memoria virtual. paging.c*/
    setup_paging();
    
    console_printf("Kernel started!\n");

    /* Detecta los dispositivos PCI conectados. pci.c */
    pci_detect();

    /* Inicializar la memoria virtual para el kernel */
    setup_kmem();

    kcache * cache;

    cache = kcache_create(32);

    if (cache == 0) {
        console_printf("Could not create cache!\n");
        for (;;);
    }

    void * ptr = kcache_alloc(cache);

    if (ptr == 0) {
        console_printf("Could not allocate object!\n");
        for (;;);
    }

    kcache_free(ptr);


}
