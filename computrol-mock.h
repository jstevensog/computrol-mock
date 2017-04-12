/* tests/unit-test.h.  Generated from unit-test.h.in by configure.  */
/*
 * Copyright © 2008-2014 Stéphane Raimbault <stephane.raimbault@gmail.com>
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef _UNIT_TEST_H_
#define _UNIT_TEST_H_

/* Constants defined by configure.ac */
#define HAVE_INTTYPES_H 1
#define HAVE_STDINT_H 1

#ifdef HAVE_INTTYPES_H
#include <inttypes.h>
#endif
#ifdef HAVE_STDINT_H
# ifndef _MSC_VER
# include <stdint.h>
# else
# include "stdint.h"
# endif
#endif

#define SERVER_ID         17
#define INVALID_SERVER_ID 18

const uint16_t BITS_ADDRESS = 0x130;
const uint16_t BITS_NB = 0x25;
const uint8_t BITS_TAB[] = { 0xCD, 0x6B, 0xB2, 0x0E, 0x1B };

const uint16_t INPUT_BITS_ADDRESS = 0x1C4;
const uint16_t INPUT_BITS_NB = 0x16;
const uint8_t INPUT_BITS_TAB[] = { 0xAC, 0xDB, 0x35 };

//const uint16_t REGISTERS_ADDRESS = 0x160;
const uint16_t REGISTERS_ADDRESS = 0x0;
const uint16_t REGISTERS_NB = 0x80;
const uint16_t REGISTERS_NB_MAX = 0x80;
const uint16_t REGISTERS_TAB[] = { 0x0000, 0x0000, 0x0005, 0x0000, 0x0000,
								   0x0000, 0x0000, 0xffff, 0x0000, 0x0000, 
								   0x0000, 0x0000, 0x00ff, 0x0000, 0x0000};

/* Raise a manual exception when this address is used for the first byte */
const uint16_t REGISTERS_ADDRESS_SPECIAL = 0x170;
/* The response of the server will contains an invalid TID or slave */
const uint16_t REGISTERS_ADDRESS_INVALID_TID_OR_SLAVE = 0x171;
/* The server will wait for 1 second before replying to test timeout */
const uint16_t REGISTERS_ADDRESS_SLEEP_500_MS = 0x172;
/* The server will wait for 5 ms before sending each byte */
const uint16_t REGISTERS_ADDRESS_BYTE_SLEEP_5_MS = 0x173;

/* If the following value is used, a bad response is sent.
   It's better to test with a lower value than
   REGISTERS_NB_POINTS to try to raise a segfault. */
const uint16_t REGISTERS_NB_SPECIAL = 0x2;

const uint16_t INPUT_REGISTERS_ADDRESS = 0x0;
const uint16_t INPUT_REGISTERS_NB = 0x500;
const uint16_t INPUT_REGISTERS_TAB[] = { 0x000A , 0x000B, 0x0A0A, 0xB0B0};

const float REAL = 123456.00;

const uint32_t IREAL_ABCD = 0x0020F147;
const uint32_t IREAL_DCBA = 0x47F12000;
const uint32_t IREAL_BADC = 0x200047F1;
const uint32_t IREAL_CDAB = 0xF1470020;

/* const uint32_t IREAL_ABCD = 0x47F12000);
const uint32_t IREAL_DCBA = 0x0020F147;
const uint32_t IREAL_BADC = 0xF1470020;
const uint32_t IREAL_CDAB = 0x200047F1;*/

#endif /* _UNIT_TEST_H_ */
