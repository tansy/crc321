/*
 * CRC32(1) calculator
 *
 * Program reads file and calculates Cyclic Redundancy Check (32-bit) sum..
 *
 * It is based on Lzd - Educational decompressor for the lzip format
 * @ https://www.nongnu.org/lzip/lzd.html
 * and is redistributed under General Public License version 2.0
 * Disclaimer is added to source in COPYING-GPL-2.0-1 file, full text under address:
 * https://www.gnu.org/licenses/old-licenses/gpl-2.0.txt
 *
    This program is free software. Redistribution and use in source and
    binary forms, with or without modification, are permitted provided
    that the following conditions are met:

    1. Redistributions of source code must retain the above copyright
    notice, this list of conditions and the following disclaimer.

    2. Redistributions in binary form must reproduce the above copyright
    notice, this list of conditions and the following disclaimer in the
    documentation and/or other materials provided with the distribution.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

 #    Exit status: 0 for a normal exit, 1 for environmental problems
 #    (file not found, invalid flags, I/O errors, etc), 2 to indicate a
 #    corrupt or invalid input file.
 */

#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#if defined(__MSVCRT__) || defined(__OS2__) || defined(_MSC_VER)
 #include <fcntl.h>
 #include <io.h>
#endif //_MSC_VER//


typedef uint32_t CRC32_table[256]; /* Table of CRCs of all 8-bit messages. */
//extern CRC32_table crc32;
CRC32_table crc32;

/* declarations; definitions are below main() */
static inline void CRC32_init( void );
static inline void CRC32_update_byte( uint32_t * const crc, const uint8_t byte );
static inline void CRC32_update_buf( uint32_t * const crc,
                                     const uint8_t * const buffer,
                                     const int size );
uint32_t calc_crc32_stream( FILE *instream );


/* int main(int argc, char** argv) */
int main(int argc, char** argv)
	{
	FILE *is;
	uint32_t crc;

	is = stdin;
	#if defined(__MSVCRT__) || defined(__OS2__) || defined(_MSC_VER)
	setmode( fileno( stdin ), O_BINARY );
	setmode( fileno( stdout ), O_BINARY );
	#endif //_MSC_VER//

	if( argc > 1 )
		{
		if ((strcmp(argv[1], "-h") == 0) || (strcmp(argv[1], "--help") == 0))
			{
			printf( "crc321 v%s - crc32 calculator.\n", "0.1" );
			printf( "\nUsage:\n");
			printf( " %s < file \n", argv[0] );
			printf( " cat file | %s \n", argv[0] );
			printf( " %s  file.name \n", argv[0] );
			return 0;
			}
		is = fopen(argv[1], "rb");
		if (is == NULL) {
			fprintf(stderr, "input file `%s' failed to open.\n", argv[1]);
			return 1;
			}
		}
	//is = stdin;
	CRC32_init();
	crc = calc_crc32_stream(is);
	printf("%08x\n", (crc ^ 0xffffffffU));
	
	return 0;
	}
/*// int main(int argc, char** argv) //*/

/* initialise crc table */
static inline void CRC32_init( void )
	{
	unsigned n;
	for( n = 0; n < 256; ++n )
		{
		unsigned c = n;
		int k;
		for( k = 0; k < 8; ++k )
			{
			if( c & 1 ) 
				c = 0xEDB88320U ^ ( c >> 1 ); 
			else 
				c >>= 1;
			}
		crc32[n] = c;
		}
	}

/* calculate crc of 1 byte */
static inline void CRC32_update_byte( uint32_t * const crc, const uint8_t byte )
	{
	*crc = crc32[(*crc^byte)&0xFF] ^ ( *crc >> 8 );
	}

/* process buffer */
static inline void CRC32_update_buf( uint32_t * const crc,
                                     const uint8_t * const buffer,
                                     const int size )
	{
	int i;
	uint32_t c = *crc;
	for( i = 0; i < size; ++i )
		{
		c = crc32[(c^buffer[i])&0xFF] ^ ( c >> 8 );
		}
	*crc = c;
	}

/* process stream */
uint32_t calc_crc32_stream( FILE *instream )
	{
	uint8_t ibuf[16*1024];
	uint32_t size_buf;
	uint32_t crc = 0xffffffffU;

	//CRC32_init();

	if (ferror(instream))
		goto ccs_errhandler_io;
	//SET_BINARY_MODE(stream);
	//while (true)
	while (!feof(instream))
		{
		//if (feof(instream)) break;
		size_buf = fread ( ibuf, sizeof(uint8_t), 16*1024, instream );
		if (ferror(instream)) 
			goto ccs_errhandler_io;
		CRC32_update_buf( &crc, ibuf, size_buf );
		}
	;
	return crc;
	ccs_errhandler_io:
		{
		fprintf(stderr, "instream error!\n");
		return 0xffffffff;
		}
	}

