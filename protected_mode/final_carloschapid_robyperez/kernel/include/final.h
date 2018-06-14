/**
 * @file
 * @ingroup kernel_code 
 * @author carloschapid robyperez
 * @copyright GNU Public License. 
 *
 * @brief Manejo del superbloque
 */

#ifndef FINAL_H
#define FINAL_H

#include <ata.h>

#define FIRMA_MBR 0xAA55
#define DIR_FIRMA_MBR 510

#define PARTICION_ACTIVA 0x80
#define TIPO_LINUX 0x83

#define INICIO_PARTICIONES 0x1BE
#define NUM_PARTICIONES 4
#define TAM_PARTICION sizeof(tabla_particiones)

#define BYTES_X_SECTOR 512

#define OFFSET_SUPERBLOQUE 2

#define SECTORES_X_PISTA 63
#define CABEZAS_X_CILINDRO 16

typedef struct{
    unsigned short sect;
}mbr;

typedef struct{
    unsigned char boot_indicator;
    unsigned char starting_chs[3];
    unsigned char type;
    unsigned char ending_chs[3];
    unsigned int starting_sector;
    unsigned int partition_size;
}tabla_particiones;


typedef struct
{
    unsigned int inodes_count;
    unsigned int blocks_count;
    unsigned int r_blocks_count;
    unsigned int free_blocks_count;
    unsigned int first_data_block;
    unsigned int log_block_size;
    unsigned int log_frag_size;
    unsigned int blocks_per_group;
    unsigned int frags_per_group;
    unsigned int inodes_per_group;
    unsigned int m_time;
    unsigned int w_time;
    unsigned short mnt_count;
    unsigned short max_mnt_count;
    unsigned short magic;
    unsigned short state;
    unsigned short errors;
    unsigned short minor_rev_level;
    unsigned int lastcheck;
    unsigned int checkinterval;
    unsigned int creator_os;
    unsigned int rev_level;
    unsigned short def_resuid;
    unsigned short def_resgid;
    
    unsigned int first_ino;
    unsigned short inode_size;
    unsigned short block_group_nr;
    unsigned int feature_compat;
    unsigned int feature_incompat;
    unsigned int feature_ro_compat;
    char uuid[16];
    char volume_name[16];
    char last_mounted[64];
    unsigned int algo_bitmap;
    
    char prealloc_blocks;
    char prealloc_dir_blocks;
    unsigned short alignment;
    
    char journal_uuid[16];
    unsigned int journal_inum;
    unsigned int journal_dev;
    unsigned int last_orphan;
    
    char hash_seed[4][4];
    char def_hash_version;
    char padding[3]; //Reservado para futura expansion
    
    unsigned int default_mount_options;
    unsigned int first_meta_bg;
    char unused[760]; //Reservado para futura expansion
}super_bloque;


void leer_sector_mbr(ata_device *dev, char* buffer);
void imprimir_tabla_particiones(char *buffer);
void imprimir_particion_linux(ata_device *dev,char *buffer);
void imprimir_super_bloque(ata_device *dev ,unsigned int inicio_LBA);

#endif /* FINAL_H */

// particiones  boot        C   H   S
//446 - 461     80 activa
//462 - 477
//478 - 493
//494 - 509
// es chs pero el retorna hsc
// H 8bits S 6bits C 10bits
// H 255  S 63  C 1023  lo que se puede guardar
// sector va desde 1 - 63

// FEFFFF indica que se usa LBA entonces toca decodificar 'start_sect' para obtener el lba

// Convertir chs -> LBA
// 16 cabezas
// 63 sectores por pista.

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