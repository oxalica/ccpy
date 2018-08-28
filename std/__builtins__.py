######## Basic functions ########

def __builtin__global_get(name):
  _def = __intrinsic__obj_new3(None, None, {})
  g = __intrinsic__get_global0()
  value =  __intrinsic__dict_get3(g, name, _def)
  if __intrinsic__not1(value is _def):
    return value
  raise NameError(name)

def __builtin__bool(x):
  b = __intrinsic__obj_get_type1(x).__bool__(x)
  if __intrinsic__not1(b is True):
    if __intrinsic__not1(b is False):
      raise TypeError('__bool__ should return bool')
  return b

def __builtin__not(x):
  b = __intrinsic__v_call2(__builtin__bool, (x,))
  return __intrinsic__not1(b)

def __builtin__call(f, args):
  ty = __intrinsic__obj_get_type1(f)
  if __intrinsic__not1(__intrinsic__not1(ty is None)): # `type`
    ty = type
  g = __intrinsic__v_call2(__builtin__getattr, (ty, '__call__', None))
  if __intrinsic__not1(g is None):
    x = __intrinsic__tuple_concat2((f,), args)
    return __intrinsic__v_call2(g, x)
  raise TypeError('Call to uncallable object')

def __builtin__getattr_from_base(obj, name, default):
  _def = __intrinsic__obj_new3(None, None, {})
  v = __intrinsic__getattr3(obj, name, _def)
  if __intrinsic__not1(v is _def): # Direct attr
    return v
  base = __intrinsic__obj_get_base1(obj)
  if __intrinsic__not1(base is None): # Not `object`
    return __intrinsic__v_call2(__builtin__getattr_from_base, (base, name, default))
  return default

def __builtin__getattr(obj, name, default=None):
  _def = __intrinsic__obj_new3(None, None, {})
  v = __intrinsic__v_call2(__builtin__getattr_from_base, (obj, name, _def))
  if __intrinsic__not1(v is _def): # Direct attr
    return v

  ty = __intrinsic__obj_get_type1(obj)
  if __intrinsic__not1(__intrinsic__not1(ty is None)):
    ty = type
  v = __intrinsic__v_call2(__builtin__getattr_from_base, (ty, name, _def))
  if __intrinsic__not1(v is _def):
    if __intrinsic__not1(__intrinsic__obj_get_type1(v) is function):
      return v
    # Bounded function
    def bounded(*args):
      args = __intrinsic__tuple_concat2((obj,), args)
      return __intrinsic__v_call2(v, args)
    return bounded
  return default

def __builtin__index(obj, idx):
  return type(obj).__getitem__(obj, idx)

def __builtin__setattr(obj, name, value):
  __intrinsic__setattr3(obj, name, value)

def __builtin__iter(iterable):
  ty = __intrinsic__obj_get_type1(iterable)
  f = __builtin__getattr(ty, '__iter__', None)
  if f is None:
    raise TypeError('Call `iter` on non-iterable type')
  return f(iterable)

def __builtin__next(it):
  ty = __intrinsic__obj_get_type1(it)
  f = __builtin__getattr(ty, '__next__', None)
  if f is None:
    raise TypeError('Call `next` on non-iterator type')
  return f(it)

def __builtin__isinstance(obj, cls):
  if cls is object:
    return True
  ty = __intrinsic__obj_get_type1(obj)
  while ty is not None:
    if ty is cls:
      return True
    ty = __intrinsic__obj_get_base1(ty)
  return False


######## Super basic object ########

def _type_ctor(cls, *args):
  x = __intrinsic__tuple_concat2((cls,), args)
  obj = __intrinsic__v_call2(cls.__new__, x)
  x = __intrinsic__tuple_concat2((obj,), args)
  __intrinsic__v_call2(cls.__init__, x)
  return obj

def _type_new(cls, obj):
  v = __intrinsic__obj_get_type1(obj)
  if v is None: # Type of `type`
    return type
  return v

def _type_repr(self):
  return '<type ...>'

def _type_init(self):
  pass

type = __intrinsic__obj_new3(None, None, {})
type.__call__ = _type_ctor
type.__new__ = _type_new
type.__init__ = _type_init
type.__repr__ = _type_repr
type.__str__ = _type_repr

def _obj_new(cls):
  return __intrinsic__obj_new3(None, cls, {})

def _obj_init(self):
  pass

def _obj_repr(self):
  return __intrinsic__repr1(self)

def _obj_str(self):
  return __intrinsic__obj_get_type1(self).__repr__(self)

def _obj_eq(self, other):
  return True if self is other else NotImplemented

def _obj_ne(self, other):
  return False if self is other else NotImplemented

object = __intrinsic__obj_new3(None, type, {})
object.__new__ = _obj_new
object.__init__ = _obj_init
object.__repr__ = _obj_repr
object.__str__ = _obj_str
object.__eq__ = _obj_eq
object.__ne__ = _obj_ne

class function(object):
  def __new__(self, code):
    raise NotImplementedError()

  def __call__(self, *args):
    return __intrinsic__v_call2(self, args)

_default = object()

######## Basic types ########

class NoneType(object):
  def __new__(cls):
    return None

class NotImplementedType(object):
  def __new__(cls):
    return NotImplemented

NotImplemented = __intrinsic__obj_new3(None, NotImplementedType, {})

class int(object):
  def __new__(cls, x):
    return __intrinsic__obj_get_type1(x).__int__(x)

  def __int__(self):
    return self

  def __bool__(self):
    return __intrinsic__int_eq2(self, 0)

  def __str__(self):
    return __intrinsic__int_to_str1(self)

  def __eq__(self, other):
    if isinstance(other, int):
      return __intrinsic__int_eq2(self, other)
    return NotImplemented

  def __lt__(self, other):
    if isinstance(other, int):
      return __intrinsic__int_lt2(self, other)
    return NotImplemented

  def __le__(self, other):
    if isinstance(other, int):
      return not __intrinsic__int_lt2(other, self)
    return NotImplemented

  def __add__(self, other):
    if isinstance(other, int):
      return __intrinsic__int_add2(self, other)
    return NotImplemented

  def __sub__(self, other):
    if isinstance(other, int):
      return __intrinsic__int_sub2(self, other)
    return NotImplemented

  def __mul__(self, other):
    if isinstance(other, int):
      return __intrinsic__int_mul2(self, other)
    return NotImplemented

  def __floordiv__(self, other):
    if isinstance(other, int):
      return __intrinsic__int_div2(self, other)
    return NotImplemented

  def __mod__(self, other):
    if isinstance(other, int):
      return __intrinsic__int_mod2(self, other)
    return NotImplemented

  def __neg__(self):
    return __intrinsic__int_sub2(0, self)

class bool(object):
  def __new__(cls, x):
    return __intrinsic__v_call2(__builtin__bool, (x,))

  def __bool__(self):
    return self

  def __int__(self):
    return 1 if self else 0

  def __str__(self):
    return 'True' if self else 'False'

class generator(object):
  def __new__(cls):
    raise NotImplementedError

  def __iter__(self):
    return self

  def __next__(self):
    if __intrinsic__gen_stopped1(self):
      raise StopIteration()
    v = __intrinsic__v_gen_next1(self)
    if __intrinsic__gen_stopped1(self):
      raise StopIteration(v)
    return v


######## Operators ########

def _op(l, r, lop, rop):
  def orz(*args): return NotImplemented
  t = __builtin__getattr(type(l), lop, orz)(l, r)
  if t is not NotImplemented:
    return t
  t = __builtin__getattr(type(r), rop, orz)(r, l)
  if t is not NotImplemented:
    return t
  return NotImplemented

def __builtin__eq(l, r):
  t = _op(l, r, '__eq__', '__eq__')
  if t is not NotImplemented:
    return t
  return l is r

def __builtin__ne(l, r):
  t = _op(l, r, '__ne__', '__ne__')
  if t is not NotImplemented:
    return t
  return not __builtin__eq(l, r)

def _def_op(op, lop, rop=_default):
  if rop is _default:
    rop = __intrinsic__str_concat2('r', lop)
  lname = __intrinsic__str_concat2(__intrinsic__str_concat2('__', lop), '__')
  rname = __intrinsic__str_concat2(__intrinsic__str_concat2('__', rop), '__')
  def f(l, r):
    t = _op(l, r, lname, rname)
    if t is not NotImplemented:
      return t
    s = __intrinsic__str_concat2(__intrinsic__str_concat2('`', op), '` is not implemented')
    raise TypeError(s)
  return f

__builtin__lt = _def_op('<', 'lt', 'gt')
__builtin__gt = _def_op('>', 'gt', 'lt')
__builtin__le = _def_op('<=', 'le', 'gt')
__builtin__ge = _def_op('>=', 'ge', 'le')

__builtin__add = _def_op('+', 'add')
__builtin__sub = _def_op('-', 'sub')
__builtin__mul = _def_op('*', 'mul')
__bulltin__pow = _def_op('**', 'pow')

def __builtin__neg(x):
  def orz(x): return NotImplemented
  x = __builtin__getattr(type(x), '__neg__', orz)(x)
  if x is NotImplemented:
    raise 'Negate is not implemented'
  return x

def __builtin__floordiv(l, r):
  t = _op(l, r, '__floordiv__', '__rfloordiv__')
  if t is not NotImplemented:
    return t
  t = _op(l, r, '__divmod__', '__rdivmod__')
  if t is not NotImplemented:
    return t[0]
  return '`/` is not implemented'

def __builtin__mod(l, r):
  t = _op(l, r, '__mod__', '__rmod__')
  if t is not NotImplemented:
    return t
  t = _op(l, r, '__divmod__', '__rdivmod__')
  if t is not NotImplemented:
    return t[1]
  return '`%` is not implemented'

######## Exceptions ########

class Exception(object):
  __name__ = 'Exception'

  def __init__(self, *args):
    self.args = args

  def __repr__(self):
    name = __intrinsic__obj_get_type1(self).__name__
    return __intrinsic__str_concat2(name, str(self.args))

  def __str__(self):
    return str(self.args)

class AttributeError(Exception):
  __name__ = 'AttributeError'

class KeyError(Exception):
  __name__ = 'KeyError'

class NameError(Exception):
  __name__ = 'NameError'

class NotImplementedError(Exception):
  __name__ = 'NotImplementedError'

class StopIteration(Exception):
  __name__ = 'StopIteration'

class TypeError(Exception):
  __name__ = 'TypeError'

class ValueError(Exception):
  __name__ = 'ValueError'


######## Builtin classes ########

class dict(object):
  def __init__(self, iterable):
    for k, v in iterable:
      self[k] = v

  def get(self, k, default=None):
    if not isinstance(k, str):
      raise TypeError('`dict.get` requires `k` to be str')
    v = __intrinsic__dict_get3(self, k, _default)
    if v is not _default:
      return v
    raise KeyError(k)

  def __getitem__(self, k):
    if not isinstance(k, str):
      raise TypeError('`dict.__getitem__` requires `k` to be str')
    return self.get(k)

  def __setitem__(self, k, v):
    __intrinsic__dict_set3(self, k, v)

  def __iter__(self):
    for k, v in __intrinsic__dict_to_tuple1(self):
      yield k, v

class list(object): # Implemented by tuple
  def __init__(self, iterable=_default):
    self.xs = tuple(iterable)

  def append(self, x):
    n = len(self.xs)
    __intrinsic__tuple_splice4(self.xs, n, n, (x,))

  def clear(self):
    __intrinsic__tuple_splice4(self.xs, 0, len(self.xs), ())

  def copy(self):
    return list(self)

  def pop(self):
    if not self:
      raise IndexError('`list.pop` on empty list')
    n = len(self.xs)
    return __intrinsic__tuple_splice4(self.xs, n - 1, n, ())[0]

  def __bool__(self):
    return len(self.xs) != 0

  def __len__(self):
    return __intrinsic__tuple_len1(self.xs)

  def __repr__(self):
    return '[' + ', '.join(map(repr, self.xs)) + ']'

  def __getitem__(self, idx):
    if __intrinsic__int_lt2(idx, len(self.xs)):
      return __intrinsic__tuple_idx2(self.xs, idx)
    raise IndexError(idx)

  def __iter__(self):
    return tuple.__iter__(self.xs)

class range(object):
  def __init__(self, a, b=_default, step=_default):
    if b is _default:
      self.start, self.end = 0, a
    else:
      self.start, self.end = a, b
    self.step = 1 if step is _default else step
    if not isinstance(self.start, int):
      raise typeerror('`range` start should be int')
    if not isinstance(self.end, int):
      raise typeerror('`range` end should be int')
    if not isinstance(self.step, int):
      raise typeerror('`range` step should be int')
    if __intrinsic__int_eq2(self.step, 0):
      raise ValueError('`range` step cannot be zero')

  def __iter__(self):
    x = self.start
    while __intrinsic__int_lt2(x, self.end):
      yield x
      x = __intrinsic__int_add2(x, self.step)

class str(object):
  def __new__(cls, x=''):
    return type(x).__str__(x)

  def join(self, iterable):
    s = ''
    fst = True
    for c in iterable:
      if fst:
        fst = False
      else:
        s = s + self
      s = s + c
    return s

  def find(self, sub, start=_default, end=_default):
    if start is _default:
      start = 0
    if end is _default:
      end = len(self)
    if not isinstance(sub, str):
      raise TypeError('`str.find` requires `sub` to be str')
    if not isinstance(start, int):
      raise TypeError('`str.find` requires `start` to be int')
    if not isinstance(end, int):
      raise TypeError('`str.find` requires `end` to be int')
    if start < 0:
      start = start + len(self)
    return __intrinsic__str_find4(self, sub, start, end)

  def __str__(self):
    return self

  def __bool__(self):
    return __intrinsic__str_len1(self) != 0

  def __int__(self):
    def is_dig(c):
      return '0123456789'.find(c) >= 0
    if not self or not all(map(is_dig, self)):
      raise ValueError('`str.__int__` requires self to be /[0-9]+/')
    return __intrinsic__str_to_int1(self)

  def __len__(self):
    return __intrinsic__str_len1(self)

  def __getitem__(self, idx):
    if idx < 0:
      idx = idx + len(self)
    return __intrinsic__str_slice4(self, idx, idx + 1, 1)

  def __iter__(self):
    for i in range(len(self)):
      yield self[i]

  def __add__(self, other):
    if not isinstance(other, str):
      return NotImplemented
    return __intrinsic__str_concat2(self, other)

class super(object):
  pass

class tuple(object):
  def __new__(cls, iterable=_default):
    ret = ()
    if iterable is not _default:
      for c in iterable:
        ret = __intrinsic__tuple_concat2(ret, (c,))
    return ret

  def __len__(self):
    return __intrinsic__tuple_len1(self)

  def __getitem__(self, idx):
    if __intrinsic__int_lt2(idx, len(self)):
      return __intrinsic__tuple_idx2(self, idx)
    raise IndexError(idx)

  def __eq__(self, other):
    n, m = len(self), len(other)
    if n != m:
      return False
    for i in range(n):
      if self[i] != other[i]:
        return False
    return True

  def __iter__(self):
    for i in range(len(self)):
      yield self[i]


######## Basic builtin functions & helpers ########

def all(iterable):
  for c in iter(iterable):
    if not c:
      return False
  return True

def any(iterable):
  for c in iter(iterable):
    if c:
      return True
  return False

def divmod(x, y):
  t = _op(x, y, '__divmod__', '__rdivmod__')
  if t is not NotImplemented:
    return t
  return x // y, x % y

def flush():
  __intrinsic__flush0()

def getattr(obj, name, default=_default):
  _def = object()
  v = __builtin__getattr(obj, name, _def)
  if v is not _def:
    return v
  if default is not _default:
    return default
  raise AttributeError(name)

def id(obj):
  return __intrinsic__id1(obj)

def input(prompt=None):
  if prompt is not None:
    raw_print(prompt)
    flush()
  return __intrinsic__input0()

isinstance = __builtin__isinstance

def iter(x, sentinel=_default):
  if sentinel is _default:
    return __builtin__iter(x)
  else:
    def f():
      while True:
        v = x()
        if v != sentinel:
          return
        yield v
    return f()

def len(x):
  return type(x).__len__(x)

def map(fn, iterable):
  for arg in iterable:
    yield fn(arg)

def next(it, default=_default):
  try:
    return __builtin__next(it)
  except Exception as e:
    if not isinstance(e, StopIteration) or default is _default:
      raise e
    return default

def pow(x, y, z=None):
  return type(x).__pow__(x, y, z)

def print(*args):
  __intrinsic__print1(' '.join(map(str, args)) + '\n')

def raw_print(s):
  __intrinsic__print1(str(s))

def repr(x):
  return type(x).__repr__(x)

def setattr(obj, name, value):
  __builtin__setattr(obj, name, value)
