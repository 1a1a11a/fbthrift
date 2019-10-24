# Copyright (c) Facebook, Inc. and its affiliates.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

import enum
from typing import (
    TypeVar,
    Type,
    SupportsInt,
    Iterator,
    Any,
    Tuple,
    Union as tUnion,
    Mapping,
    NamedTuple,
    Sequence,
    Optional,
)

_T = TypeVar("_T")
eT = TypeVar("eT", bound=Enum)

class NOTSETTYPE(enum.Enum):
    token: NOTSETTYPE = ...

NOTSET = NOTSETTYPE.token

class Struct:
    def __copy__(self: _T) -> _T: ...

class Union(Struct): ...

class EnumMeta(type):
    def __iter__(self: Type[_T]) -> Iterator[_T]: ...
    def __revered__(self: Type[_T]) -> Iterator[_T]: ...
    def __contains__(self: Type[_T], member: Any) -> bool: ...
    def __getitem__(self: Type[_T], name: str) -> _T: ...
    def __len__(self) -> int: ...
    @property
    def __members__(self: Type[_T]) -> Mapping[str, _T]: ...

class Enum(metaclass=EnumMeta):
    name: str
    value: int
    def __init__(self: eT, value: tUnion[eT, int]) -> None: ...  # __call__ for meta
    def __repr__(self) -> str: ...
    def __str__(self) -> str: ...
    def __hash__(self) -> int: ...
    def __reduce__(self: eT) -> Tuple[Type[eT], Tuple[int]]: ...
    def __int__(self) -> int: ...

class Flag(Enum):
    def __contains__(self: eT, other: eT) -> bool: ...
    def __bool__(self) -> bool: ...
    def __or__(self: eT, other: eT) -> eT: ...
    def __and__(self: eT, other: eT) -> eT: ...
    def __xor__(self: eT, other: eT) -> eT: ...
    def __invert__(self: eT, other: eT) -> eT: ...

class BadEnum(SupportsInt):
    name: str
    value: int
    enum: Enum
    def __init__(self, the_enum: Type[eT], value: int) -> None: ...
    def __repr__(self) -> str: ...
    def __int__(self) -> int: ...

class StructType(enum.Enum):
    STRUCT: StructType = ...
    UNION: StructType = ...
    EXCEPTION: StructType = ...

class Qualifier(enum.Enum):
    NONE: Qualifier = ...
    REQUIRED: Qualifier = ...
    OPTIONAL: Qualifier = ...

class StructSpec(NamedTuple):
    name: str
    fields: Sequence[FieldSpec]
    kind: StructType
    annotations: Mapping[str, str] = {}

class FieldSpec(NamedTuple):
    name: str
    type: Type[Any]
    qualifier: Qualifier
    default: Optional[Any]
    annotations: Mapping[str, str] = {}

class ListSpec(NamedTuple):
    value: Type[Any]

class SetSpec(NamedTuple):
    value: Type[Any]

class MapSpec(NamedTuple):
    key: Type[Any]
    value: Type[Any]
