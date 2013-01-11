/*
 * Fiasco-ia32
 * Specific code for I/O port protection
 */
INTERFACE[(ia32|amd64) & io & debug]:

#include "mapdb.h"
#include "types.h"

extern Static_object<Mapdb> mapdb_io;

IMPLEMENTATION[(ia32|amd64) & io]:

#include "l4_types.h"
#include "assert.h"
#include "space.h"
#include "io_space.h"

Static_object<Mapdb> mapdb_io;

void init_mapdb_io(Space *sigma0)
{
  static size_t const io_page_sizes[] =
    {Io_space::Map_superpage_shift, 9, Io_space::Page_shift};

  mapdb_io.construct(sigma0, Page_number::create(0x10000 >> io_page_sizes[0]), io_page_sizes, 3);
}

/** Map the IO port region described by "fp_from" of address space "from" 
    into address space "to". IO ports can only be mapped idempotently, 
    therefore there is no offset for fp_from and only those ports are mapped 
    that lay in the intersection of fp_from and fp_to
    @param from source address space
    @param fp_from... IO flexpage descripton for IO space range
	in source IO space	
    @param to destination address space
    @param fp_to... IO flexpage description for IO space range
	in destination IO space
    @return IPC error code that describes the status of the operation
 */
L4_error __attribute__((nonnull(1, 3)))
io_map(Space *from, L4_fpage const &fp_from,
       Space *to, L4_fpage const &fp_to, L4_msg_item control)
{
/*   printf("io_map %u -> %u "
 * 	    "snd %08x base %x size %x rcv %08x base %x size %x\n",
 * 	    (unsigned)from->space(), (unsigned)to->space(),
 * 	    fp_from.fpage, 
 * 	    fp_from.iofp.iopage, fp_from.iofp.iosize,
 * 	    fp_to.fpage, 
 * 	    fp_to.iofp.iopage, fp_to.iofp.iosize);
 *   kdb_ke("io_fpage_map 1");
 */

  typedef Map_traits<Io_space> Mt;
  Mt::Addr rcv_pos = Mt::get_addr(fp_to);
  Mt::Addr snd_pos = Mt::get_addr(fp_from);

  Mt::Size rcv_size = Mt::Size::from_shift(fp_to.order());
  Mt::Size snd_size = Mt::Size::from_shift(fp_from.order());

  snd_pos = snd_pos.trunc(snd_size);
  rcv_pos = rcv_pos.trunc(rcv_size);
  Mt::constraint(snd_pos, snd_size, rcv_pos, rcv_size, Mt::Addr(0));

  if (snd_size == 0)
    return L4_error::None;

  //assert(snd_pos < L4_fpage::Io_port_max);

  unsigned long del_attribs, add_attribs;
  Mt::attribs(control, fp_from, &del_attribs, &add_attribs);

  return map<Io_space>(mapdb_io.get(), from, from, snd_pos,
             snd_size,
             to, to, snd_pos,
             control.is_grant(), add_attribs, del_attribs,
             (Io_space::Reap_list**)0);
}

/** Unmap IO mappings.
    Unmap the region described by "fp" from the IO
    space "space" and/or the IO spaces the mappings have been
    mapped into. 
    XXX not implemented yet
    @param space address space that should be flushed
    @param fp    IO flexpage descriptor of IO-space range that should
                 be flushed
    @param me_too If false, only flush recursive mappings.  If true, 
                 additionally flush the region in the given address space.
    @return true if successful
*/
unsigned __attribute__((nonnull(1)))
io_fpage_unmap(Space *space, L4_fpage fp, L4_map_mask mask)
{
  typedef Map_traits<Io_space> Mt;
  Mt::Size size = Mt::Size::from_shift(fp.order());
  Mt::Addr port = Mt::get_addr(fp);
  port = port.trunc(size);

  // Here we _would_ reset IOPL to 0 but this doesn't make much sense
  // for only one thread since this thread may have forwarded the right
  // to other threads too. Therefore we had to walk through any thread
  // of this space.
  //
  // current()->regs()->eflags &= ~EFLAGS_IOPL;

  return unmap<Io_space>(mapdb_io.get(), space, space,
               port, size,
               fp.rights(), mask, (Io_space::Reap_list**)0);
}

static inline
void
save_access_attribs(Mapdb* /*mapdb*/, const Mapdb::Frame& /*mapdb_frame*/,
		    Mapping* /*mapping*/, Io_space* /*space*/, 
		    unsigned /*page_rights*/, 
		    Io_space::Addr /*virt*/, Io_space::Phys_addr /*phys*/,
		    Io_space::Size /*size*/,
		    bool /*me_too*/)
{}
