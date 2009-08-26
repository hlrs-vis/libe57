/*
 * stdint.h - stub version of general integer type decls.
 *
 * Copyright (C) 2009 Kevin Ackley (kackley@gwi.net)
 *
 * This file is part of the E57 Reference Implementation (E57RI).
 * 
 * E57RI is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation, either version 3 of
 * the License, or at your option) any later version.
 * 
 * E57RI is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public
 * License along with E57RI.  If not, see <http://www.gnu.org/licenses/>.
 */
#ifndef STDINT_H_INCLUDED
#define STDINT_H_INCLUDED

/// Shorthands for integers of known size
typedef char                int8_t;
typedef short               int16_t;
typedef int                 int32_t;
//typedef long long         int64_t;
typedef __int64             int64_t; //??? not portable
typedef unsigned char       uint8_t;
typedef unsigned short      uint16_t;
typedef unsigned int        uint32_t;
//typedef unsigned long long    uint64_t;
typedef unsigned __int64 uint64_t; //???

/// Minimum and maximum values for integers
#define INT8_MIN        (-128)
#define INT8_MAX        (127)
#define INT16_MIN       (-32767-1)
#define INT16_MAX       (32767)
#define INT32_MIN       (-2147483647-1)
#define INT32_MAX       (2147483647)
#define INT64_MIN       (-9223372036854775808LL)
#define INT64_MAX       (9223372036854775807LL)
#define UINT8_MIN       (0)
#define UINT8_MAX       (255)
#define UINT16_MIN      (0)
#define UINT16_MAX      (65535)
#define UINT32_MIN      (0)
#define UINT32_MAX      (4294967295U)
#define UINT64_MIN      (0)
#define UINT64_MAX      (18446744073709551615LL)

#endif
