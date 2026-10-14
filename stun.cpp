/*
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 2026 Yeatol <vim@vimer.org>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "stun.h"

#include <bit>
#include <span>
#include <cstring>
#include <concepts>
#include <stdint.h>

#pragma pack(push, 1)

struct message_header_t
{
    uint16_t type;
    uint16_t length;
    uint32_t magic_cookie;
    uint8_t  transaction_id[12];
};

struct attribute_header_t
{
    uint16_t type;
    uint16_t length;
};

struct xor_address_ipv4_t
{
    uint8_t  zero;
    uint8_t  family;
    uint16_t x_port;
    uint8_t  x_address[4];
};

struct xor_address_ipv6_t
{
    uint8_t  zero;
    uint8_t  family;
    uint16_t x_port;
    uint8_t  x_address[16];
};

#pragma pack(pop)

static constexpr uint8_t ipv4_mapped[12] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF};

namespace { template <std::integral T> constexpr T hton(T v) { if constexpr (std::endian::native == std::endian::little) return std::byteswap(v); else return v; } }

void network_stun_handler(std::span<uint8_t, 16> ip, uint16_t& port, std::span<uint8_t, 1232> buffer, uint32_t& size, bool& drop)
{
    message_header_t* message_header = (message_header_t*)buffer.data();

    if (size < sizeof(message_header_t) || message_header->magic_cookie != hton<uint32_t>(0x2112A442)) return;

    uint8_t* magic_cookie = (uint8_t*)&message_header->magic_cookie;

    if (message_header->type == hton<uint16_t>(0x0001))
    {
        if (std::memcmp(ip.data(), ipv4_mapped, sizeof(ipv4_mapped)) == 0)
        {
            auto attribute_header = (attribute_header_t*)(buffer.data() + sizeof(message_header_t));
            auto xor_address_ipv4 = (xor_address_ipv4_t*)(buffer.data() + sizeof(message_header_t) + sizeof(attribute_header_t));

            message_header->type     = hton<uint16_t>(0x0101);
            message_header->length   = hton<uint16_t>(sizeof(attribute_header_t) + sizeof(xor_address_ipv4_t));
            attribute_header->type   = hton<uint16_t>(0x0020);
            attribute_header->length = hton<uint16_t>(sizeof(xor_address_ipv4_t));
            xor_address_ipv4->zero   = 0x00;
            xor_address_ipv4->family = 0x01;
            xor_address_ipv4->x_port = port ^ hton<uint16_t>(0x2112);

            for (int i = 0; i < 4; ++i) xor_address_ipv4->x_address[i] = ip[i + 12] ^ magic_cookie[i];

            size = sizeof(message_header_t) + sizeof(attribute_header_t) + sizeof(xor_address_ipv4_t);
        }
        else
        {
            auto attribute_header = (attribute_header_t*)(buffer.data() + sizeof(message_header_t));
            auto xor_address_ipv6 = (xor_address_ipv6_t*)(buffer.data() + sizeof(message_header_t) + sizeof(attribute_header_t));

            message_header->type     = hton<uint16_t>(0x0101);
            message_header->length   = hton<uint16_t>(sizeof(attribute_header_t) + sizeof(xor_address_ipv6_t));
            attribute_header->type   = hton<uint16_t>(0x0020);
            attribute_header->length = hton<uint16_t>(sizeof(xor_address_ipv6_t));
            xor_address_ipv6->zero   = 0x00;
            xor_address_ipv6->family = 0x02;
            xor_address_ipv6->x_port = port ^ hton<uint16_t>(0x2112);

            for (int i = 0; i < 4;  ++i) xor_address_ipv6->x_address[i] = ip[i] ^ magic_cookie[i];
            for (int i = 4; i < 16; ++i) xor_address_ipv6->x_address[i] = ip[i] ^ message_header->transaction_id[i - 4];

            size = sizeof(message_header_t) + sizeof(attribute_header_t) + sizeof(xor_address_ipv6_t);
        }

        drop = false;
    }
}
