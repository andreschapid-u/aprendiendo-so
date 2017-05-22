/**
 * @file
 * @ingroup kernel_code 
 * @author Erwin Meza <emezav@gmail.com>
 * @copyright GNU Public License. 
 * @brief Contiene la implementaci�n de las rutinas relacionadas
 * con la gesti�n de memoria f�sica mediante un mapa de bits.
 * La memoria se gestiona en unidades de PAGE_SIZE denominadas marcos de
 * p�gina.
 */

#include <console.h>
#include <pm.h>
#include <paging.h>
#include <physmem.h>
#include <multiboot.h>
#include <stdlib.h>

/* Variable definida en start.S que almacena la direcci�n f�sica en la cual
 * terminan las tablas de p�gina iniciales del kernel */
extern unsigned int kernel_initial_pagetables_end;

/** @brief Mapa de bits de memoria disponible
 * @details Esta variable almacena el apuntador del inicio del mapa de bits
 * que permite gestionar las unidades de memoria. */
 unsigned int * memory_bitmap;

 /** @brief Siguiente marco disponible en el mapa de bits */
 unsigned int next_free_frame;

 /** @brief Numero de marcos libres en la memoria */
 int free_frames;

 /** @brief Numero total de marcos disponibles en la memoria */
 int total_frames;

 /** @brief Marco inicial de los marcos disponibles en memoria */
 unsigned int base_frame;


 /** @brief Tamano del mapa de bits en memoria.
  * @details
  * Para un espacio fisico de maximo 4 GB, se requiere un mapa de bits
  * de 128 KB. Si cada entrada ocupa 4 bytes, se requiere 32678 entradas.
  */
unsigned int memory_bitmap_length =
		~(0x0) / (PAGE_SIZE * BITS_PER_ENTRY);

/** @brief Variable global del kernel que almacena el inicio de la regi�n
 * de memoria disponible */
unsigned int memory_start;
/** @brief Variable global del kernel que almacena el tamano en bytes de
 * la memoria disponible */
unsigned int memory_length;

/** @brief M�nima direcci�n de memoria permitida para liberar */
unsigned int allowed_free_start;

/* @brief Variable que almacena la ubicaci�n de la estructura multiboot en
 * memoria. Definida en start.S */
extern unsigned int multiboot_info_location;

/**
 * @brief Inicializa el mapa de bits de memoria, a partir de la informacion
 * proporcionada por GRUB.
 */
void setup_physical_memory(void){

	extern multiboot_header_t multiboot_header;
  extern unsigned int kernel_memory_bitmap;

    /*Ubicar el mapa de bits justo despu�s de las tablas de p�gina iniciales en
     * memoria. */
  memory_bitmap = (unsigned int*)&kernel_memory_bitmap;

	/* Variables temporales para hallar la region de memoria disponible */
	unsigned int tmp_start;
	unsigned int tmp_length;
	unsigned int tmp_end;
	int mod_count;
    unsigned int mmap_address;
    unsigned int mods_address;

    /* Dado que ya se habilit� la memoria virtual, se debe usar la
     * direcci�n virtual en la cual se encuentra mapeada la estructura de
     * informaci�n multiboot. */
	multiboot_info_t * info = (multiboot_info_t *)(multiboot_info_location + KERNEL_VIRT_OFFSET);

	int i;

	unsigned int mods_end; /* Almacena la direcci�n de memoria final
	del ultimo modulo cargado, o 0 si no se cargaron modulos. */

	for(i=0; i<memory_bitmap_length; i++){
		memory_bitmap[i] = 0;
	}

	/* si flags[3] = 1, se especificaron m�dulos que deben ser cargados junto
	 * con el kernel y justo despu�s del mismo. */

	mods_end = 0;

    /* Se debe sumar KERNEL_VIRT_OFFSET a la direcci�n, dado que ya se
     * activ� la memoria virtual. */
	if (test_bit(info->flags, 3)) {
		mod_info_t * mod_info;
        mods_address = info->mods_addr + KERNEL_VIRT_OFFSET;
		for (mod_info = (mod_info_t*)(mods_address), mod_count=0;
				mod_count < info->mods_count;
				mod_count++, mod_info++) {
			if (mod_info->mod_end > mods_end) {
				/* Los modulos se redondean a limites de 4 KB, redondear
				 * la direcci�n final del modulo a un limite de 4096 */
				mods_end = mod_info->mod_end + (mod_info->mod_end % 4096);
			}
		}
	}


	/* si flags[6] = 1, los campos mmap_length y mmap_addr son validos */

	/* Revisar las regiones de memoria, y extraer la region de memoria
	 * de mayor tamano, marcada como disponible, cuya direcci�n base sea
	* mayor o igual a la posicion del kernel en memoria.
	*/

	memory_start = 0;
	memory_length = 0;

	free_frames = 0;

	/* Suponer que el inicio de la memoria disponible se encuentra
	 * al finalizar el kernel, los m�dulos, el directorio de tablas de p�gina y
     * las tablas de p�gina del kernel. */
	allowed_free_start = kernel_initial_pagetables_end;


	/** Existe un mapa de memoria v�lido creado por GRUB? */
	if (test_bit(info->flags, 6)) {
        
		memory_map_t *mmap;

        /* Calcular la direcci�n virtual del mapa de memoria*/
        mmap_address = info->mmap_addr + KERNEL_VIRT_OFFSET;

		for (mmap = (memory_map_t *) (mmap_address);
			(unsigned int) mmap < mmap_address +  info->mmap_length;
			mmap = (memory_map_t *) ((unsigned int) mmap
									 + mmap->entry_size
									 + sizeof (mmap->entry_size))) {

	  /** Verificar si la regi�n de memoria cumple con las condiciones
	   * para ser considerada "memoria disponible".
	   *
	   * Importante: Si se supone un procesador de 32 bits, los valores
	   * de la parte alta de base y length (base_addr_high y
	   * length_high) son cero. Por esta razon se pueden ignorar y solo
	   * se usan los 32 bits menos significativos de base y length.
	   *
	   * Para que una region de memoria sea considerada "memoria
	   * disponible", debe cumplir con las siguientes condiciones:
	   *
	   * - Estar ubicada en una posicion de memoria mayor o igual que
	   * 	1 MB.
	   * - Tener su atributo 'type' en 1 = memoria disponible.
	   * */
		 /* La region esta marcada como disponible y su direcci�n base
		  * es igual o superior a 1 MB? */
		 if (mmap->type == 1 && mmap->base_addr_low >= 0x100000)  {
			 tmp_start = mmap->base_addr_low;
			 tmp_length = mmap->length_low;

			 /* Verificar si el kernel se encuentra en esta region */
			 if (kernel_initial_pagetables_end >= tmp_start &&
					kernel_initial_pagetables_end <= tmp_start + tmp_length) {
				 /* El kernel se encuentra en esta region. Tomar el inicio
				  * de la memoria disponible en la posicion en la cual
				  * finaliza el kernel
				 */
				 tmp_start =kernel_initial_pagetables_end;

				 /* Ahora verificar si ser cargaron m�dulos junto con el
				  * kernel. Estos modulos se cargan en regiones continuas
				  * al kernel.
				  * Si es as�, la nueva posici�n inicial de la memoria
				  * disponible es la posici�n en la cual terminan los m�dulos
				  * */
				 if (mods_end > 0 &&
						 mods_end >= tmp_start &&
						 mods_end <= tmp_start + tmp_length) {
						tmp_start = mods_end;
				 }
				 /* Restar al espacio disponible.*/
				 tmp_length -= tmp_start - mmap->base_addr_low;
				 if (tmp_length > memory_length) {
					 memory_start = tmp_start;
					 memory_length = tmp_length; /* Tomar el espacio */
				 }
			 }else {
				 /* El kernel no se encuentra en esta region, verificar si
				  * su tamano es mayor que la region mas grande encontrada
				  * hasta ahora
				  */
				 if (tmp_length > memory_length) {
					 memory_start = tmp_start;
					 memory_length = tmp_length; /* Tomar el espacio */
				 }
			 }
		 }
		} //endfor
	}

	/* Existe una regi�n de memoria disponible? */
	if (memory_start > 0 && memory_length > 0) {
		/* Antes de retornar, establecer la minima direcci�n de memoria
		 * permitida para liberar*/

		tmp_start = memory_start;
		/* Calcular la direcci�n en la cual finaliza la memoria disponible */
		tmp_end = tmp_start + tmp_length;

		/* Redondear el inicio y el fin de la regi�n de memoria disponible a
         * p�ginas */
		tmp_end = ROUND_DOWN_TO_PAGE(tmp_end);
		tmp_start = ROUND_UP_TO_PAGE(tmp_start);

		/* Calcular el tamano de la regi�n de memoria disponible, redondeada
		 * a l�mites de p�gina */
		tmp_length = tmp_end - tmp_start;

		/* Actualizar las variables globales del kernel */
		memory_start = tmp_start;
		memory_length = tmp_length;

		/* Marcar la regi�n de memoria como disponible */
		mark_available_memory(memory_start, memory_length);

        /* Establecer la direcci�n de memoria a partir de la cual se puede
         * liberar memoria */
		allowed_free_start = memory_start;
		next_free_frame = allowed_free_start / PAGE_SIZE;

		total_frames = free_frames;
		base_frame = next_free_frame;

	}
 }

/** @brief Permite verificar si el marco de p�gina se encuentra disponible. */
int test_frame(unsigned int frame) {
	 volatile int entry = frame / BITS_PER_ENTRY;
	 volatile int offset = frame % BITS_PER_ENTRY;
	 return (memory_bitmap[entry] & 0x1 << offset);
}


/** @brief  Permite marcar el marco de p�gina como ocupado */
void clear_frame(unsigned int frame) {
	 volatile int entry = frame / BITS_PER_ENTRY;
	 volatile int offset = frame % BITS_PER_ENTRY;
	 memory_bitmap[entry] &= ~(0x1 << offset);
}

/** @brief Permite marcar el marco de p�gina como libre. */
void set_frame(unsigned int frame) {
	 volatile int entry = frame / BITS_PER_ENTRY;
	 volatile int offset = frame % BITS_PER_ENTRY;
	 memory_bitmap[entry] |= (0x1 << offset);
}


/**
 @brief Reserva un marco libre dentro del mapa de bits de memoria.
 * @return Direcci�n de inicio del marco de p�gina, 0 si no existen marcos
 * disponibles
 */
unsigned int allocate_frame(void) {
	 unsigned int frame;
	 unsigned int entry;
	 unsigned int offset;


	 /* Si no existen marcos libres, retornar*/
	 if (free_frame == 0) {
		 return 0;
	 }

	/* Iterar por el mapa de bits*/
 	frame = next_free_frame;
 	 do {
 		 if (test_frame(frame)) {
 			 entry = frame / BITS_PER_ENTRY;
 			 offset = frame % BITS_PER_ENTRY;
 			 memory_bitmap[entry] &= ~(0x1 << offset);

        /* Avanzar en la posicion de busqueda de la proxima unidad disponible */
 			 next_free_frame++;
			 if (next_free_frame > base_frame + total_frames) {
				 next_free_frame = base_frame;
			 }
 			/* Descontar la unidad tomada */

 			free_frames--;
 			 return frame * PAGE_SIZE;
 		 }
 		frame++;
 		if (frame > base_frame + total_frames) {
 			frame = base_frame;
 		}
 	 }while (frame != next_free_frame);


 	 return 0;
  }

/** @brief Reserva una regi�n de memoria contigua libre dentro del mapa de bits
* de memoria.
* @param length Tamano de la regi�n de memoria a asignar.
* @return Direcci�n de inicio de la regi�n en memoria, 0 si no es posible
* reservar.
*/
unsigned int allocate_frame_region(unsigned int length) {
	unsigned int frame;
	unsigned int frame_count;
	unsigned int i;
	int result;

	frame_count = (length / PAGE_SIZE);

	if (length % PAGE_SIZE > 0) {
		frame_count++;
	}

    /* Verificar si existe suficiente espacio */
	if (free_frames < frame_count) {
		 return 0;
	}

	/* Iterar por el mapa de bits*/
	frame = next_free_frame;
	 do {
		 if (test_frame(frame) &&
				 (frame + frame_count) < (base_frame + total_frames)) {

			 result = 1;
			 for (i=frame; i<frame + frame_count; i++){
				 result = (result && test_frame(i));
			 }
			 /* Marcar las p�ginas como ocupadas */
			 if (result) {
				 for (i=frame; i<frame + frame_count; i++){
					 /* Descontar la p�gina tomada */
					 free_frames--;
					 clear_frame(i);
				 }

                 /* Avanzar en la posicion de busqueda del pr�ximo marco
                  * disponible */
				 next_free_frame = frame + frame_count;
				 if (next_free_frame > base_frame + total_frames) {
					 next_free_frame = base_frame;
				 }

				return frame * PAGE_SIZE;
			 }
		 }
		frame++;
		if (frame > base_frame + total_frames) {
			frame = base_frame;
		}
	 }while (frame != next_free_frame);

	  return 0;
  }

/**
 * @brief Permite liberar un marco de p�gina
 * @param addr Direcci�n de inicio del marco. Se redondea hacia abajo si no es
 * m�ltiplo de PAGE_SIZE
 */
void free_frame(unsigned int addr) {
	 unsigned int start;
	 unsigned int entry;
	 int offset;
	 unsigned int frame;

	 start = ROUND_DOWN_TO_PAGE(addr);

	 if (start < allowed_free_start) {return;}

	 frame = start / PAGE_SIZE;

	 set_frame(frame);

	 /* Marcar la unidad recien liberada como la proxima unidad
	  * para asignar */
	 next_free_frame = frame;

	 /* Aumentar en 1 el numero de unidades libres */
	 free_frames ++;
 }

/**
 * @brief Marca una regi�n de memoria como disponible
 * @param start_addr Direcci�n de memoria del inicio de la regi�n a marcar como
 * disponible. Se redondea por debajo a una p�gina
 * @param length Tamano de la regi�n en bytes, m�ltiplo de PAGE_SIZE
 */
void mark_available_memory(unsigned int start_addr, unsigned int length) {
	 unsigned int start;
	 unsigned int end;

	 start = ROUND_DOWN_TO_PAGE((unsigned int)start_addr);

	 if (start < allowed_free_start) {return;}

	 end = start + length;

	 for (; start < end; start += PAGE_SIZE) {
		 free_frame(start);
	 }

	 /* Almacenar el inicio de la regi�n liberada para una pr�xima asignaci�n */
	 next_free_frame = (unsigned int)start_addr / PAGE_SIZE;
}
