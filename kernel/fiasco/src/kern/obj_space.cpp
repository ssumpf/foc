INTERFACE:

#include "config.h"
#include "l4_types.h"
#include "l4_msg_item.h"
#include <hlist>

class Kobject_mapdb;
class Jdb_mapdb;
class Kobject_iface;
class Kobject;
class Space;
class Ram_quota;
class Mem_space;


namespace Obj {

  class Capability
  {
  private:
    Mword _obj;

  public:
    Capability() {}
    explicit Capability(Mword v) : _obj(v) {}
    Kobject_iface *obj() const { return (Kobject_iface *)(_obj & ~3UL); }
    //void obj(Phys_addr obj) { _obj = Mword(obj); }
    void set(Kobject_iface *obj, unsigned char rights)
    { _obj = Mword(obj) | rights; }
    bool valid() const { return _obj; }
    void invalidate() { _obj = 0; }
    unsigned char rights() const { return _obj & 3; }
    void add_rights(unsigned char r) { _obj |= r & L4_fpage::CWS; }
    void del_rights(unsigned char r) { _obj &= ~(r & L4_fpage::CWS); }

    bool operator == (Capability const &c) const { return _obj == c._obj; }
  };

  /**
   * Tn the case of a flat copy model for capabilities, we have some
   * extra mapping information directly colocated within the cap tables.
   */
  class Mapping : public cxx::H_list_item
  {
    friend class ::Kobject_mapdb;
    friend class ::Jdb_mapdb;

  public:
    typedef cxx::H_list<Mapping> List;

    enum Flag
    {
      Delete  = L4_fpage::CD,
      Ref_cnt = 0x10,

      Initial_flags = Delete | Ref_cnt | L4_msg_item::C_ctl_rights
    };

  protected:
    Mword _flags : 8;
    Mword _pad   : 24;

  public:
    Mapping() : _flags(0) {}
    // fake this really badly
    Mapping *parent() { return this; }
    Mword delete_rights() const { return _flags & Delete; }
    Mword ref_cnt() const { return _flags & Ref_cnt; }

    void put_as_root() { _flags = Initial_flags; }
  };


  class Entry : public Capability, public Mapping
  {
  public:
    Entry() {}
    explicit Entry(Mword v) : Capability(v) {}

    unsigned rights() const
    { return Capability::rights() | (_flags & ~3); }

    void set(Kobject_iface *obj, unsigned char rights)
    {
      Capability::set(obj, rights & 3);
      _flags = rights & 0xf8;
    }
    void add_rights(unsigned char r)
    {
      Capability::add_rights(r & 3);
      _flags |= (r & 0xf8);
    }

    void del_rights(unsigned char r)
    {
      Capability::del_rights(r & 3);
      _flags &= ~(r & 0xf8);
    }
  };


  struct Cap_addr
  {
  public:
    Page_number _a;
    mutable Entry *_c;
    Cap_addr() {}
    Cap_addr(Page_number a, Entry *c) : _a(a), _c(c) {}

    explicit Cap_addr(Page_number a) : _a(a), _c(0) {}
    explicit Cap_addr(Address adr) : _a(adr), _c(0) {}

    Address value() const { return _a.value(); }
    void set_entry(Entry *e) const { _c = e; }

    bool operator == (Cap_addr const &o) const { return _a == o._a; }
    bool operator < (Cap_addr const &o) const { return _a < o._a; }
    bool operator > (Cap_addr const &o) const { return _a > o._a; }

    void operator += (Page_number o) { _a += o; }
    void operator += (Cap_addr const &o) { operator += (o._a); }

    Cap_addr operator + (Page_number o) const { return Cap_addr(_a + o); }

    Cap_addr offset(Page_number s) const { return Cap_addr(_a.offset(s)); }
    Cap_addr trunc(Page_number s) const
    {
      if (s == Page_number(1))
	return *this;
      return Cap_addr(_a.trunc(s));
    }

    operator Page_number::Type_conversion_error const * () const
    { return (Page_number::Type_conversion_error const *)_a; }

    operator Page_number () const { return _a; }
  };

  inline void set_entry(Cap_addr const &ca, Entry *e)
  { ca.set_entry(e); }
};



template< typename SPACE >
class Generic_obj_space
{
  friend class Jdb_obj_space;
  friend class Jdb_tcb;

public:
  enum { Caps_per_page = Config::PAGE_SIZE / sizeof(Obj::Entry) };

  static char const * const name;

  typedef Obj::Capability Capability;
  typedef Obj::Entry Entry;
  typedef Kobject *Reap_list;
  typedef Kobject_iface *Phys_addr;
  typedef Obj::Cap_addr Addr;
  typedef Page_count Size;

  enum
  {
    Need_insert_tlb_flush = 0,
    Need_xcpu_tlb_flush = 0,
    Map_page_size = 1,
    Page_shift = 0,
    Map_superpage_size = 1,
    Map_max_address = 1UL << 20, /* 20bit obj index */
    Whole_space = 20,
    Identity_map = 0,
  };

  enum Status
  {
    Insert_ok = 0,		///< Mapping was added successfully.
    Insert_warn_exists,		///< Mapping already existed
    Insert_warn_attrib_upgrade,	///< Mapping already existed, attribs upgrade
    Insert_err_nomem,		///< Couldn't alloc new page table
    Insert_err_exists		///< A mapping already exists at the target addr
  };

  enum Page_attrib 
  {
    /// A mask which contains all mask bits
    //Page_user_accessible = 1,
    //Page_writable = 2,
    Page_references = 0,
    Page_all_attribs = 3
  };

  static Address superpage_size() { return Map_superpage_size; }
  static bool has_superpages() { return false; }
  static Phys_addr page_address(Phys_addr o, Size) { return o; }
  static Phys_addr subpage_address(Phys_addr addr, Size) { return addr; }
};

template< typename SPACE >
char const * const Generic_obj_space<SPACE>::name = "Obj_space";


// -------------------------------------------------------------------------
IMPLEMENTATION:

PUBLIC template< typename SPACE >
static inline
Mword
Generic_obj_space<SPACE>::xlate_flush(unsigned char rights)
{ return rights; }

PUBLIC template< typename SPACE >
static inline
Mword
Generic_obj_space<SPACE>::is_full_flush(unsigned char rights)
{ return rights & L4_fpage::R; }

PUBLIC template< typename SPACE >
static inline
Mword
Generic_obj_space<SPACE>::xlate_flush_result(Mword /*attribs*/)
{ return 0; }

PUBLIC template< typename SPACE >
inline NEEDS[Generic_obj_space::caps_free]
Generic_obj_space<SPACE>::~Generic_obj_space()
{
  caps_free();
}

PUBLIC template< typename SPACE >
inline
bool
Generic_obj_space<SPACE>::v_fabricate(Addr const &address,
                                      Phys_addr *phys, Size *size,
                                      unsigned* attribs = 0)
{
  return Generic_obj_space::v_lookup(address, phys, size, attribs);
}


PUBLIC template< typename SPACE >
inline static
void
Generic_obj_space<SPACE>::tlb_flush ()
{}

PUBLIC template< typename SPACE >
inline static
typename Generic_obj_space<SPACE>::Addr
Generic_obj_space<SPACE>::canonize(Addr v)
{ return v; }

// ------------------------------------------------------------------------
INTERFACE [debug]:

EXTENSION class Generic_obj_space
{
public:
  struct Dbg_info
  {
    Address offset;
    Generic_obj_space<SPACE> *s;
  };

};


// ------------------------------------------------------------------------
IMPLEMENTATION [debug]:

#include "dbg_page_info.h"
#include "warn.h"

PUBLIC  template< typename SPACE >
static
void
Generic_obj_space<SPACE>::add_dbg_info(void *p, Generic_obj_space *s,
                                         Address cap)
{
  Dbg_page_info *info = new Dbg_page_info(Virt_addr((Address)p));

  if (EXPECT_FALSE(!info))
    {
      WARN("oom: could not allocate debug info fo page %p\n", p);
      return;
    }

  info->info<Dbg_info>()->s = s;
  info->info<Dbg_info>()->offset = (cap / Caps_per_page) * Caps_per_page;
  Dbg_page_info::table().insert(info);
}

PUBLIC  template< typename SPACE >
static
void
Generic_obj_space<SPACE>::remove_dbg_info(void *p)
{
  Dbg_page_info *info = Dbg_page_info::table().remove(Virt_addr((Address)p));
  if (info)
    delete info;
  else
    WARN("could not find debug info for page %p\n", p);
}


// ------------------------------------------------------------------------
IMPLEMENTATION [!debug]:

PUBLIC  template< typename SPACE > inline
static
void
Generic_obj_space<SPACE>::add_dbg_info(void *, Generic_obj_space *, Address)
{}
  
PUBLIC  template< typename SPACE > inline
static
void
Generic_obj_space<SPACE>::remove_dbg_info(void *)
{}
