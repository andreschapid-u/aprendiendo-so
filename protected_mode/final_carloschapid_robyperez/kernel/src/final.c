#include <ata.h>
#include <console.h>
#include <final.h>
#include <kmem.h>
#include <stdlib.h>
#include <string.h>

tabla_particiones *p, *p_linux;
unsigned char H;
unsigned char S;
unsigned short C;
unsigned int LBA = 0;

void leer_sector_mbr(ata_device *dev, char *buffer)
{
    mbr *sector_valido = &buffer[DIR_FIRMA_MBR];

    if (sector_valido->sect != FIRMA_MBR)
    {
        console_printf("Sector de arranque invalido!\n");
        return;
    }

    imprimir_tabla_particiones(buffer);
    imprimir_particion_linux(dev, buffer);
}

void imprimir_tabla_particiones(char *buffer)
{
    for (int particion = INICIO_PARTICIONES, i = 1; i < NUM_PARTICIONES + 1; i++, particion += TAM_PARTICION)
    {
        p = &buffer[particion];
        if (p->boot_indicator == PARTICION_ACTIVA)
        {
            console_printf("Particion %d, Tipo de particion 0x%x, Tamanio de la particion  0x%x\n", i, p->type, (p->partition_size * BYTES_X_SECTOR));
        }
        else
        {
            console_printf("Particion %d no presente!\n", i);
        }
    }
    console_printf("\n");
}

void imprimir_particion_linux(ata_device *dev, char *buffer)
{
    char linuxOk = 0;
    for (int particion = INICIO_PARTICIONES, i = 1; i < NUM_PARTICIONES + 1; i++, particion += TAM_PARTICION)
    {
        p_linux = &buffer[particion];

        if (p_linux->type == TIPO_LINUX)
        {
            // sacando valores
            // H = h
            // S = s AND 0x3F
            // S = s AND ~0xC0
            // C = c | (s AND 0xC0) << 2

            // HEADS = 16
            // SPT = 63
            // LBA = C * HEADS * SPT + H * SPT + S -1
            // LBA = ((C * HEADS) + H) * SPT + S -1
            // SUPER_BLOQUE = LBA + 2 SECTORES
            console_printf("Particion LINUX\n");
            console_printf("Particion %d, Boot Indicator 0x%x\n", i, p_linux->boot_indicator);
            H = p_linux->starting_chs[0];
            S = p_linux->starting_chs[1] & 0x3F;
            C = p_linux->starting_chs[2] | ((p_linux->starting_chs[1] & ~0x3F) << 2);

            if (H == 0xFE && S == 0xFF && C == 0xFF)
            {
                LBA = p_linux->starting_sector;
            }
            else
            {
                LBA = ((C * CABEZAS_X_CILINDRO) + H) * SECTORES_X_PISTA + S - 1;
            }

            console_printf("\tTipo de particion 0x%x\n", p_linux->type);
            console_printf("\tInicion LBA  0x%x\n", LBA);
            console_printf("\tTamaño de la particion  0x%x\n", (p_linux->partition_size * BYTES_X_SECTOR));
            linuxOk = 1;

            imprimir_super_bloque(dev, LBA);
            break;
        }
    }
    if (!linuxOk)
    {
        console_printf("No hay particiones LINUX!\n");
    }
}

void imprimir_super_bloque(ata_device *dev, unsigned int inicio_LBA)
{
    if (LBA == 0)
    {
        console_printf("No se pudo leer el superbloque!\n");
        return;
    }
    char *buf;

    //Calculo de la posición del superbloque
    inicio_LBA += OFFSET_SUPERBLOQUE;

    //Se asignan 2 páginas para guardar la información del superbloque (1024 bytes)
    buf = (char *)kmem_allocate_pages(2, 1);

    int result;
    /* Leer a buf del primer dispositivo el sector LBA+2 (2 sectores). */
    result = ata_read(dev, buf, inicio_LBA, 2);

    if (result == -1)
    {
        console_printf("No se pudo leer el superbloque!\n");
        return;
    }

    super_bloque *sd = (super_bloque *)buf;

    console_printf("\n\tSuper Bloque\n");
    console_printf("Numero Inodos: %u\n", sd->inodes_count);
    console_printf("Numero Bloques: %u\n", sd->blocks_count);
    console_printf("Bloques Libres: %u\n", sd->free_blocks_count);
    console_printf("Tamanio del Bloque: %u\n", (1024 << sd->log_block_size));
    console_printf("Nombre de la partición: %s\n", sd->volume_name);
}