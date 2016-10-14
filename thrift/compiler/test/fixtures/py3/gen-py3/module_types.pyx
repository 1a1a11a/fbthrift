from libcpp.memory cimport shared_ptr, make_shared, unique_ptr, make_unique
from libcpp.string cimport string
from libcpp cimport bool as cbool
from cpython cimport bool as pbool
from libc.stdint cimport int8_t, int16_t, int32_t, int64_t
from cython.operator cimport dereference as deref

from module_types cimport (
    cSimpleStruct
)

cdef class SimpleStruct:

    def __init__(
        self,
         is_on,
         tiny_int,
        int small_int,
        int nice_sized_int,
        int big_int,
         real
    ):
        deref(self.c_SimpleStruct).is_on = is_on
        deref(self.c_SimpleStruct).tiny_int = tiny_int
        deref(self.c_SimpleStruct).small_int = small_int
        deref(self.c_SimpleStruct).nice_sized_int = nice_sized_int
        deref(self.c_SimpleStruct).big_int = big_int
        
        
    @staticmethod
    cdef create(shared_ptr[cSimpleStruct] c_SimpleStruct):
        inst = <SimpleStruct>SimpleStruct.__new__(SimpleStruct)
        inst.c_SimpleStruct = c_SimpleStruct
        return inst

    @property
    def is_on(self):
        return <pbool> self.c_SimpleStruct.get().is_on

    @property
    def tiny_int(self):
        return self.c_SimpleStruct.get().tiny_int

    @property
    def small_int(self):
        return self.c_SimpleStruct.get().small_int

    @property
    def nice_sized_int(self):
        return self.c_SimpleStruct.get().nice_sized_int

    @property
    def big_int(self):
        return self.c_SimpleStruct.get().big_int

    @property
    def real(self):
        return self.c_SimpleStruct.get().real


