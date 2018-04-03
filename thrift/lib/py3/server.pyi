#!/usr/bin/env python3

import ipaddress
from enum import Enum
import pathlib
import os
from typing import Callable, NamedTuple, Union, Optional, TypeVar, Mapping, ClassVar

mT = TypeVar('mT', bound=Callable)
IPAddress = Union[ipaddress.IPv4Address, ipaddress.IPv6Address]
Path = Union[str, bytes, os.PathLike]


class SocketAddress(NamedTuple):
    ip: Optional[IPAddress]
    port: Optional[int]
    path: Optional[pathlib.Path]


def pass_context(func: mT) -> mT: ...


class SSLPolicy(Enum):
    DISABLED: int
    PERMITTED: int
    REQUIRED: int
    value: int


class ServiceInterface:
    annotations: ClassVar[Mapping[str, str]] = ...


hT = TypeVar('hT', bound=ServiceInterface)


class ThriftServer:
    def __init__(
        self,
        handler: hT,
        port: int=0,
        ip: Optional[Union[IPAddress, str]]=None,
        path: Optional[Path]=None,
    ) -> None: ...
    async def serve(self) -> None: ...
    def set_ssl_policy(self, policy: SSLPolicy) -> None: ...
    def stop(self) -> None: ...
    async def get_address(self) -> SocketAddress: ...
    def get_active_requests(self) -> int: ...
    def get_max_requests(self) -> int: ...
    def set_max_requests(self, max_requests: int) -> None: ...
    def get_max_connections(self) -> int: ...
    def set_max_connections(self, max_connections: int) -> None: ...
    def get_listen_backlog(self) -> int: ...
    def set_listen_backlog(self, listen_backlog: int) -> None: ...


class ConnectionContext:
    peer_address: SocketAddress
    is_tls: bool
    peer_common_name: str
    peer_certificate: bytes


class RequestContext:
    connection_context: ConnectionContext
