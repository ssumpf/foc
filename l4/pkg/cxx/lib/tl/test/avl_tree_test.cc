/*
 * (c) 2008-2009 Alexander Warg <warg@os.inf.tu-dresden.de>
 *     economic rights: Technische Universität Dresden (Germany)
 *
 * This file is part of TUD:OS and distributed under the terms of the
 * GNU General Public License 2.
 * Please see the COPYING-GPL-2 file for details.
 *
 * As a special exception, you may use this file as part of a free software
 * library without restriction.  Specifically, if other files instantiate
 * templates or use macros or inline functions from this file, or you compile
 * this file and link it with other files to produce an executable, this
 * file does not by itself cause the resulting executable to be covered by
 * the GNU General Public License.  This exception does not however
 * invalidate any other reasons why the executable file might be covered by
 * the GNU General Public License.
 */
#define DEBUG_AVL_TREE 1

#include "avl_tree.h"

#include <iostream>
#include <string>

class Region
{
private:
  unsigned long _start, _end;

public:
  Region()
  : _start(0), _end(0) 
  {}
  
  Region(unsigned long start)
  : _start(start), _end(start +1) 
  {}
  
  Region(unsigned long start, unsigned long end)
  : _start(start), _end(end) 
  {}

  bool operator < (Region const &o) const
  { return _end < o._start; }

  bool operator == (Region const &o) const
  { return _start <= o._start && _end >= o._end; }

  friend std::ostream &operator << (std::ostream &s, Region const &r);
};
  
std::ostream &operator << (std::ostream &s, Region const &r)
{
  s << '[' << r._start << "; " << (void*)r._end << ']';
  return s;
}


typedef cxx::Avl_tree<int, int> Tree;

Tree tree;


#define check_x(a) \
  std::cerr << " => " #a " =====================\n"; \
  check_z(a)

void check_z(int err)
{
  if (err)
    {
      std::cerr << "Error: " << err << "\n";
      abort();
    }

  tree.dump();
}

inline int ins(int x)
{ return tree.insert(x,x); }

inline void find(int x)
{
  Tree::Node_type const *n;
  std::cerr << "find(" << x << ") = "; 
  n = tree.find(x);
  if (!n)
    std::cerr << "[nil]\n";
  else
    std::cerr << "(" << n->key << "; " << n->data << ")\n";
}



int main()
{
  std::cerr << "Test AVL Tree\n";
  check_x(ins(50));
  check_x(ins(60));
  check_x(ins(70));
  check_x(ins(80));
  check_x(ins(81));
  check_x(ins(45));
  check_x(ins(40));
  check_x(ins(35));
  check_x(ins(33));
  check_x(ins(32));
  check_x(ins(41));
  check_x(ins(42));
  check_x(ins(43));
  check_x(ins(31));
  check_x(ins(30));
  check_x(ins(29));
  check_x(ins(28));
  check_x(ins(27));
  check_x(ins(26));

  std::cerr << "Find some entries\n";
  find(10);
  find(50);
  find(26);
  find(30);
  find(34);
  find(22);
  find(81);
  
  std::cerr << "Remove some entries\n";
  check_x(tree.remove(29));
  check_x(tree.remove(70));
  check_x(tree.remove(50));
  check_x(tree.remove(60));
  check_x(tree.remove(26));
  check_x(tree.remove(32));
  check_x(tree.remove(35));
  check_x(tree.remove(30));
  check_x(tree.remove(45));
  check_x(tree.remove(43));
  check_x(tree.remove(80));
  check_x(tree.remove(41));
  check_x(tree.remove(42));

  return 0;
}
