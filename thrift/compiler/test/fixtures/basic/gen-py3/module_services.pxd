from cy_thrift_server cimport cServerInterface

cdef extern from "src/gen-cpp2/MyService.h" namespace "cpp2":
    cdef cppclass cMyServiceSvAsyncIf "cpp2::MyServiceSvAsyncIf":
      pass

    cdef cppclass cMyServiceSvIf "cpp2::MyServiceSvIf"(cMyServiceSvAsyncIf, cServerInterface):
      pass

cdef extern from "src/gen-cpp2/MyServiceFast.h" namespace "cpp2":
    cdef cppclass cMyServiceFastSvAsyncIf "cpp2::MyServiceFastSvAsyncIf":
      pass

    cdef cppclass cMyServiceFastSvIf "cpp2::MyServiceFastSvIf"(cMyServiceFastSvAsyncIf, cServerInterface):
      pass

