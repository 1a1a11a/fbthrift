#
# Autogenerated by Thrift
#
# DO NOT EDIT UNLESS YOU ARE SURE THAT YOU KNOW WHAT YOU ARE DOING
#  @generated
#
cimport cython as __cython
from cython.operator cimport dereference as deref
from libcpp.memory cimport make_unique, unique_ptr, shared_ptr

cimport thrift.py3.types
from thrift.py3.types cimport (
    reset_field as __reset_field,
    StructFieldsSetter as __StructFieldsSetter
)

from thrift.py3.types cimport const_pointer_cast


@__cython.auto_pickle(False)
cdef class __MyStruct_FieldsSetter(__StructFieldsSetter):

    @staticmethod
    cdef __MyStruct_FieldsSetter create(_module_types.cMyStruct* struct_cpp_obj):
        cdef __MyStruct_FieldsSetter __fbthrift_inst = __MyStruct_FieldsSetter.__new__(__MyStruct_FieldsSetter)
        __fbthrift_inst._struct_cpp_obj = struct_cpp_obj
        __fbthrift_inst._setters[__cstring_view(<const char*>"MyIntField")] = __MyStruct_FieldsSetter._set_field_0
        __fbthrift_inst._setters[__cstring_view(<const char*>"MyStringField")] = __MyStruct_FieldsSetter._set_field_1
        __fbthrift_inst._setters[__cstring_view(<const char*>"MyDataField")] = __MyStruct_FieldsSetter._set_field_2
        __fbthrift_inst._setters[__cstring_view(<const char*>"myEnum")] = __MyStruct_FieldsSetter._set_field_3
        __fbthrift_inst._setters[__cstring_view(<const char*>"oneway")] = __MyStruct_FieldsSetter._set_field_4
        __fbthrift_inst._setters[__cstring_view(<const char*>"readonly")] = __MyStruct_FieldsSetter._set_field_5
        __fbthrift_inst._setters[__cstring_view(<const char*>"idempotent")] = __MyStruct_FieldsSetter._set_field_6
        return __fbthrift_inst

    cdef void set_field(__MyStruct_FieldsSetter self, const char* name, object value) except *:
        cdef __cstring_view cname = __cstring_view(name)
        cdef cumap[__cstring_view, __MyStruct_FieldsSetterFunc].iterator found = self._setters.find(cname)
        if found == self._setters.end():
            raise TypeError(f"invalid field name {name.decode('utf-8')}")
        deref(found).second(self, value)

    cdef void _set_field_0(self, __fbthrift_value) except *:
        # for field MyIntField
        if __fbthrift_value is None:
            __reset_field[_module_types.cMyStruct](deref(self._struct_cpp_obj), 0)
            return
        if not isinstance(__fbthrift_value, int):
            raise TypeError(f'MyIntField is not a { int !r}.')
        __fbthrift_value = <cint64_t> __fbthrift_value
        deref(self._struct_cpp_obj).MyIntField_ref().assign(__fbthrift_value)
        deref(self._struct_cpp_obj).__isset.MyIntField = True

    cdef void _set_field_1(self, __fbthrift_value) except *:
        # for field MyStringField
        if __fbthrift_value is None:
            __reset_field[_module_types.cMyStruct](deref(self._struct_cpp_obj), 1)
            return
        if not isinstance(__fbthrift_value, str):
            raise TypeError(f'MyStringField is not a { str !r}.')
        deref(self._struct_cpp_obj).MyStringField_ref().assign(cmove(bytes_to_string(__fbthrift_value.encode('utf-8'))))
        deref(self._struct_cpp_obj).__isset.MyStringField = True

    cdef void _set_field_2(self, __fbthrift_value) except *:
        # for field MyDataField
        if __fbthrift_value is None:
            __reset_field[_module_types.cMyStruct](deref(self._struct_cpp_obj), 2)
            return
        if not isinstance(__fbthrift_value, _module_types.MyDataItem):
            raise TypeError(f'MyDataField is not a { _module_types.MyDataItem !r}.')
        deref(self._struct_cpp_obj).MyDataField_ref().assign(deref((<_module_types.MyDataItem?> __fbthrift_value)._cpp_obj))
        deref(self._struct_cpp_obj).__isset.MyDataField = True

    cdef void _set_field_3(self, __fbthrift_value) except *:
        # for field myEnum
        if __fbthrift_value is None:
            __reset_field[_module_types.cMyStruct](deref(self._struct_cpp_obj), 3)
            return
        if not isinstance(__fbthrift_value, _module_types.MyEnum):
            raise TypeError(f'field myEnum value: {repr(__fbthrift_value)} is not of the enum type { _module_types.MyEnum }.')
        deref(self._struct_cpp_obj).myEnum_ref().assign(<_module_types.cMyEnum><int>__fbthrift_value)
        deref(self._struct_cpp_obj).__isset.myEnum = True

    cdef void _set_field_4(self, __fbthrift_value) except *:
        # for field oneway
        if __fbthrift_value is None:
            __reset_field[_module_types.cMyStruct](deref(self._struct_cpp_obj), 4)
            return
        if not isinstance(__fbthrift_value, bool):
            raise TypeError(f'oneway is not a { bool !r}.')
        deref(self._struct_cpp_obj).oneway_ref().assign(__fbthrift_value)
        deref(self._struct_cpp_obj).__isset.oneway = True

    cdef void _set_field_5(self, __fbthrift_value) except *:
        # for field readonly
        if __fbthrift_value is None:
            __reset_field[_module_types.cMyStruct](deref(self._struct_cpp_obj), 5)
            return
        if not isinstance(__fbthrift_value, bool):
            raise TypeError(f'readonly is not a { bool !r}.')
        deref(self._struct_cpp_obj).readonly_ref().assign(__fbthrift_value)
        deref(self._struct_cpp_obj).__isset.readonly = True

    cdef void _set_field_6(self, __fbthrift_value) except *:
        # for field idempotent
        if __fbthrift_value is None:
            __reset_field[_module_types.cMyStruct](deref(self._struct_cpp_obj), 6)
            return
        if not isinstance(__fbthrift_value, bool):
            raise TypeError(f'idempotent is not a { bool !r}.')
        deref(self._struct_cpp_obj).idempotent_ref().assign(__fbthrift_value)
        deref(self._struct_cpp_obj).__isset.idempotent = True


@__cython.auto_pickle(False)
cdef class __MyDataItem_FieldsSetter(__StructFieldsSetter):

    @staticmethod
    cdef __MyDataItem_FieldsSetter create(_module_types.cMyDataItem* struct_cpp_obj):
        cdef __MyDataItem_FieldsSetter __fbthrift_inst = __MyDataItem_FieldsSetter.__new__(__MyDataItem_FieldsSetter)
        __fbthrift_inst._struct_cpp_obj = struct_cpp_obj
        return __fbthrift_inst

    cdef void set_field(__MyDataItem_FieldsSetter self, const char* name, object value) except *:
        cdef __cstring_view cname = __cstring_view(name)
        cdef cumap[__cstring_view, __MyDataItem_FieldsSetterFunc].iterator found = self._setters.find(cname)
        if found == self._setters.end():
            raise TypeError(f"invalid field name {name.decode('utf-8')}")
        deref(found).second(self, value)

