/*
 * Copyright (c) 2024 Wojtek Kaniewski <wojtekka@toxygen.net>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 * 3. Neither the name of the author nor the names of its contributors
 *    may be used to endorse or promote products derived from this
 *    software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include "pp.h"
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <stdint.h>
#include <stdbool.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <getopt.h>
#include <sched.h>
#include <limits.h>

#define DEFAULT_PORT "/dev/parport0"
#define DEFAULT_SIZE 65536

static pp_t pp;

#define BIT_ADDRESS_LOW_INVERTED PARPORT_CONTROL_AUTOFD
#define BIT_ADDRESS_HIGH_INVERTED PARPORT_CONTROL_INIT
#define BIT_DATA PARPORT_CONTROL_STROBE
#define BIT_PROGRAM PARPORT_CONTROL_SELECT

static void write_address_low(uint8_t address_low)
{
    pp_wdata(&pp, address_low);
    pp_wcontrol(&pp, BIT_PROGRAM | BIT_ADDRESS_HIGH_INVERTED);
    pp_wcontrol(&pp, BIT_PROGRAM | BIT_ADDRESS_LOW_INVERTED | BIT_ADDRESS_HIGH_INVERTED);
}

static void write_address_high(uint8_t address_high)
{
    pp_wdata(&pp, address_high);
    pp_wcontrol(&pp, BIT_PROGRAM | BIT_ADDRESS_LOW_INVERTED);
    pp_wcontrol(&pp, BIT_PROGRAM | BIT_ADDRESS_LOW_INVERTED | BIT_ADDRESS_HIGH_INVERTED);
}

static void write_data(uint8_t data)
{
    pp_wdata(&pp, data);
    pp_wcontrol(&pp, BIT_PROGRAM | BIT_ADDRESS_LOW_INVERTED | BIT_ADDRESS_HIGH_INVERTED | BIT_DATA);
    pp_wcontrol(&pp, BIT_PROGRAM | BIT_ADDRESS_LOW_INVERTED | BIT_ADDRESS_HIGH_INVERTED);
}

static void program_enable(void)
{
    pp_wcontrol(&pp, BIT_PROGRAM | BIT_ADDRESS_LOW_INVERTED | BIT_ADDRESS_HIGH_INVERTED);
}

static void program_disable(void)
{
    pp_wcontrol(&pp, BIT_ADDRESS_LOW_INVERTED | BIT_ADDRESS_HIGH_INVERTED);
}

void usage(const char *argv0)
{
    fprintf(stderr, "usage: %s [OPTIONS] FILENAME\n"
                    "\n"
                    "  -p, --port=PORT       select either parport (e.g. /dev/parport0) or physical\n"
                    "                        port (e.g. 0x378), default is " DEFAULT_PORT "\n"
                    "  -s, --size=BYTES      memory size in bytes or kilobytes, must be power of 2,\n"
                    "                        default is 65536.\n"
                    "  -h, --help            print this message\n"
                    "\n"
                    "File format is binary.\n"
                    "\n", argv0);
}

int main(int argc, char **argv)
{
    const char *port = DEFAULT_PORT;
    unsigned int size = DEFAULT_SIZE;

    while (true)
    {
        static const struct option long_options[] = 
        {
            { "port",           required_argument,  0, 'p' },
            { "size",           required_argument,  0, 's' },
            { "help",           no_argument,        0, 'h' },
            { 0,                0,                  0, 0 }
        };

        int option_index = 0;
        int c = getopt_long(argc, argv, "p:s:h", long_options, &option_index);

        if (c == -1)
        {
            break;
        }

        switch (c)
        {
        case 'p':
            port = optarg;
            break;

        case 's':
        {
            char *endptr = NULL;
            unsigned long tmp = strtoul(optarg, &endptr, 0);
            if (tmp > UINT32_MAX || (tmp == ULONG_MAX && errno == ERANGE) || (tmp == 0 && errno == EINVAL) || (*endptr != 0) || tmp > 65536 || ((tmp & (tmp >> 1)) != 0))
            {
                fprintf(stderr, "Invalid size '%s'\n", optarg);
                exit(1);
            }
            size = (unsigned int) tmp;
            if (size <= 64)
            {
                size *= 1024;
            }
            break;
        }

        case 'h':
            usage(argv[0]);
            exit(0);

        default:
            exit(1);
        }
    }

    if (optind >= argc)
    {
        usage(argv[0]);
        exit(1);
    }

    const char *filename = argv[optind];

    printf("%s\n", filename);

    int fd = open(filename, O_RDONLY);

    if (fd == -1)
    {
        perror(filename);
        exit(1);
    }

    char buf[size];

    memset(buf, 0xff, size);

    if (read(fd, buf, size) < 0)
    {
        perror(filename);
        close(fd);
        exit(1);
    }

    if (pp_open(&pp, port) == -1)
    {
        printf("pp_open failed\n");
        perror(port);
        exit(1);
    }

    pp_wdata(&pp, 0);

    program_enable();

    unsigned int addr;
    for (addr = 0; addr < 65536; ++addr)
    {
        if (addr % 1024 == 0)
        {
            printf("\rWriting %u kB...", addr / 1024);
            fflush(stdout);
        }

        if ((addr & 0xff) == 0)
        {
            write_address_high((addr >> 8) & 255);
        }

        write_address_low(addr & 255);

        write_data(buf[addr % size]);
    }

    program_disable();

    printf("\nWrite complete\n");
    pp_close(&pp);
    return 0;
}
