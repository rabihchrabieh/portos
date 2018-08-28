/* Select between dynamic memory allocation of Portos or DSP BIOS.
 * Non-zero for DSP BIOS.
 */
#define po_memory_USE_TARGET  1

#if po_memory_USE_TARGET
#define po_memory_ADD 0
#define po_memory_error()
#define po_memory_size2index(size) (size)
#define po_memory_create(region, heap, length, maxBlockSize)
#define po_free(block) MEM_free((block))

#define po_rmalloc(size, region)         MEM_alloc((region), (size), 8)
#define po_rmalloc_const(size, region)   po_rmalloc((size), (region))
#define po_rmalloc_constP(size, region)  po_rmalloc((size), (region))
#define po_rmalloc_forever(size, region) po_rmalloc((size), (region))
#define po_rmalloc_index(index, region)  po_rmalloc((index), (region))

#define po_memory_DEFAULT_REGION 0
#define po_malloc(size)          po_rmalloc((size), po_memory_DEFAULT_REGION)
#define po_malloc_const(size)    po_rmalloc((size), po_memory_DEFAULT_REGION)
#define po_malloc_forever(size)  po_rmalloc((size), po_memory_DEFAULT_REGION)
#define po_malloc_index(index)   po_rmalloc((index), po_memory_DEFAULT_REGION)

#define po_smalloc(size)         po_rmalloc((size), po_sys_MEM_REGION)
#define po_smalloc_const(size)   po_rmalloc((size), po_sys_MEM_REGION)
#define po_smalloc_constP(size)  po_rmalloc((size), po_sys_MEM_REGION)
#define po_smalloc_forever(size) po_rmalloc((size), po_sys_MEM_REGION)
#define po_smalloc_index(index)  po_rmalloc((index), po_sys_MEM_REGION)
#endif //po_memory_USE_TARGET

