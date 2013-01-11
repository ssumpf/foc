INTERFACE:

class Vm;
class Ram_quota;

class Vm_factory
{
public:
  static Vm *create(Ram_quota *quota, int *err);
};
