INTERFACE:

class Fpu_state
{
public:
#if 0
  void save_state();
  void restore_state();
#endif
  void *state_buffer();
  void state_buffer(void *b);

  Fpu_state();
  ~Fpu_state();

private:
  friend class Fpu_alloc;

  void *_state_buffer;
};

IMPLEMENTATION:

IMPLEMENT inline
Fpu_state::Fpu_state() : _state_buffer(0)
{}

IMPLEMENT inline
Fpu_state::~Fpu_state()
{
  //free_state();
}

IMPLEMENT inline
void *Fpu_state::state_buffer()
{
  return _state_buffer;
}

IMPLEMENT inline
void Fpu_state::state_buffer(void *b)
{
  _state_buffer = b;
}


#if 0
IMPLEMENT inline
void Fpu_state::save_state()
{
  Fpu::save_state(this);
}

IMPLEMENT inline
void Fpu_state::restore_state()
{
  Fpu::restore_state(this);
}
#endif
