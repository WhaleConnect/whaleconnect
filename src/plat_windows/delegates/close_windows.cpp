// Copyright 2021-2023 Aidan Sun and the Network Socket Terminal contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#if OS_WINDOWS
#include "sockets/delegates/closeable.hpp"
#include "sockets/enums.hpp"

template <auto Tag>
void Delegates::Closeable<Tag>::_closeImpl() const {
    shutdown(_handle, SD_BOTH);
    closesocket(_handle);
}

template void Delegates::Closeable<SocketTag::IP>::_closeImpl() const;
template void Delegates::Closeable<SocketTag::BT>::_closeImpl() const;
#endif
