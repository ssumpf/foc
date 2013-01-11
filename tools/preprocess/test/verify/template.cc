// AUTOMATICALLY GENERATED -- DO NOT EDIT!         -*- c++ -*-

#include "template.h"
#include "template_i.h"


#line 185 "template.cpp"


template <> stack_t<int>*
create_stack<int>()
{
  return new stack<int>();
}

#line 192 "template.cpp"



template <> stack_t<bool>*
create_stack<bool>()
{
  return new stack<bool>();
}
