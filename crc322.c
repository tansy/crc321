/*
 * CRC32(1) calculator
 *
 * Program reads file and calculates Cyclic Redundancy Check sum (32-bit)
 * crc321/2 v0.2 (with slicing by 4/8 bytes at a time)
 * There is no difference in functionality; this version utilizes
 * slice-by-4/8 algorithm which is Intel's modification to Sarwate's
 * algorythm used in version 0.1. It processes 4/8 bytes at a time using
 * specially prepared look up table (LUT) that is 4/8 times bigger than
 * Sarwate's LUT but it makes whole operation significantly faster.
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


//typedef uint32_t CRC32_table[256]; /* Table of CRCs of all 8-bit messages. */
//CRC32_table crc32; /* look up table (LUT) as described in Sarwate algorithm http://web.archive.org/web/20160313131147if_/http://www.ifp.illinois.edu/~sarwate/pubs/Sarwate88Computing.pdf */
uint32_t Crc32Lookup[8][256]; /* slice-by-8 look up table (LUT) as described in http://web.archive.org/web/20061001000000/http://www.intel.com/technology/comms/perfnet/download/CRC_generators.pdf */
uint32_t CRC32_POLYNOMIAL = 0xEDB88320U; /* zlib polynomial */


// precomputation of slicing-by-4 and slicing-by-8 (after Stephan Brumme https://create.stephan-brumme.com/crc32/)
void CRC32_LUT8_init()
	{
	// same as Sarwate LUT, it becomes first slice and reference for computing the rest
	uint32_t i, j;
	uint32_t crc;
	uint32_t Polynomial = CRC32_POLYNOMIAL;
	//uint32_t Crc32Lookup[8][256] = CRC_LUT_8;

	for (i = 0; i <= 0xFF; i++)
		{
		crc = i;
		for (j = 0; j < 8; j++) {
			crc = (crc >> 1) ^ ((crc & 1) * Polynomial); // although it seems slightly more comlicated than previous /@CRC32_init()/ version it's ~120% faster due to lack of branching
			}
		Crc32Lookup[0][i] = crc;
		}

	for (i = 0; i <= 0xFF; i++)
		{
		// for Slicing-by-4 and Slicing-by-8
		Crc32Lookup[1][i] = (Crc32Lookup[0][i] >> 8) ^ Crc32Lookup[0][Crc32Lookup[0][i] & 0xFF];
		Crc32Lookup[2][i] = (Crc32Lookup[1][i] >> 8) ^ Crc32Lookup[0][Crc32Lookup[1][i] & 0xFF];
		Crc32Lookup[3][i] = (Crc32Lookup[2][i] >> 8) ^ Crc32Lookup[0][Crc32Lookup[2][i] & 0xFF];
		// only Slicing-by-8
		Crc32Lookup[4][i] = (Crc32Lookup[3][i] >> 8) ^ Crc32Lookup[0][Crc32Lookup[3][i] & 0xFF];
		Crc32Lookup[5][i] = (Crc32Lookup[4][i] >> 8) ^ Crc32Lookup[0][Crc32Lookup[4][i] & 0xFF];
		Crc32Lookup[6][i] = (Crc32Lookup[5][i] >> 8) ^ Crc32Lookup[0][Crc32Lookup[5][i] & 0xFF];
		Crc32Lookup[7][i] = (Crc32Lookup[6][i] >> 8) ^ Crc32Lookup[0][Crc32Lookup[6][i] & 0xFF];
		}
	}
//void CRC_LUT8_init()


///void CRC32_update_buf( uint32_t * const crc, const uint8_t * const buffer, const int size )
static inline void crc32_4bytes_v(uint32_t * const previousCrc32, const void* data, int length)
	{
	///uint32_t  crc = ~(*previousCrc32); // same as previousCrc32 ^ 0xFFFFFFFF
	uint32_t  crc = *previousCrc32;
	const uint32_t* current = (const uint32_t*) data;

	// process four bytes at once (Slicing-by-4)
	while (length >= 4)
		{
			uint32_t one = *current++ ^ crc;
			crc = Crc32Lookup[0][(one>>24) & 0xFF] ^
			Crc32Lookup[1][(one>>16) & 0xFF] ^
			Crc32Lookup[2][(one>> 8) & 0xFF] ^
			Crc32Lookup[3][ one      & 0xFF];

		length -= 4;
		}

	const uint8_t* currentChar = (const uint8_t*) current;
	// remaining 1 to 3 bytes (standard algorithm)
	while (length-- != 0)
	crc = (crc >> 8) ^ Crc32Lookup[0][(crc & 0xFF) ^ *currentChar++];

	///*previousCrc32 = ~crc; // same as crc ^ 0xFFFFFFFF
	*previousCrc32 = crc;
}
//void crc32_4bytes_v(uint32_t * const previousCrc32, const void* data, int length)


/// compute CRC32 (Slicing-by-8 algorithm)
///uint32_t crc32_8bytes_v(const void* data, size_t length, uint32_t previousCrc32)
static inline void crc32_8bytes_v(uint32_t * const previousCrc32, const void* data, int length)
	{
	///uint32_t crc = ~previousCrc32; // same as previousCrc32 ^ 0xFFFFFFFF
	uint32_t crc = *previousCrc32;
	const uint32_t* current = (const uint32_t*) data;

	// process eight bytes at once (Slicing-by-8)
	while (length >= 8)
	{
	uint32_t one = *current++ ^ crc;
	uint32_t two = *current++;
	crc = Crc32Lookup[0][(two>>24) & 0xFF] ^
		  Crc32Lookup[1][(two>>16) & 0xFF] ^
		  Crc32Lookup[2][(two>> 8) & 0xFF] ^
		  Crc32Lookup[3][ two      & 0xFF] ^
		  Crc32Lookup[4][(one>>24) & 0xFF] ^
		  Crc32Lookup[5][(one>>16) & 0xFF] ^
		  Crc32Lookup[6][(one>> 8) & 0xFF] ^
		  Crc32Lookup[7][ one      & 0xFF];

	length -= 8;
	}

	const uint8_t* currentChar = (const uint8_t*) current;
	// remaining 1 to 7 bytes (standard algorithm)
	while (length-- != 0)
	crc = (crc >> 8) ^ Crc32Lookup[0][(crc & 0xFF) ^ *currentChar++];

	*previousCrc32 = crc;
	///return ~crc; // same as crc ^ 0xFFFFFFFF
}
//void crc32_8bytes_v(uint32_t * const previousCrc32, const void* data, int length)

/*// I leave them as reference but exclude from build
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
				c = CRC32_POLYNOMIAL ^ ( c >> 1 );
			else
				c >>= 1;
			}
		crc32[n] = c;
		}
	}

static inline void CRC32_update_byte( uint32_t * const crc, const uint8_t byte )
	{
	*crc = crc32[(*crc^byte)&0xFF] ^ ( *crc >> 8 );
	}

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
*/

uint32_t calc_crc32_stream( FILE *instream )
	{
	uint8_t ibuf[16*1024];
	uint32_t size_buf;
	uint32_t crc = 0xffffffffU;

	//CRC32_init(); // 14.6 [us]
	CRC32_LUT8_init(); // init table; could be static but it's fast enough, on my ancient machine it takes 9.8 [us]

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
		///CRC32_update_buf( &crc, ibuf, size_buf );
		crc32_8bytes_v( &crc, ibuf, size_buf );
		}
	;
	return crc;
	ccs_errhandler_io:
		{
		fprintf(stderr, "instream error!\n");
		return 0xffffffff;
		}
	}

int main(int argc, char** argv)
	{
	FILE *is;
	uint32_t crc;
	char* progname = "crc322";

	is = stdin;
	#if defined(__MSVCRT__) || defined(__OS2__) || defined(_MSC_VER)
	setmode( fileno( stdin ), O_BINARY );
    setmode( fileno( stdout ), O_BINARY );
	#endif //_MSC_VER//

	if( argc > 1 )
		{
		if ((strcmp(argv[1], "-h") == 0) || (strcmp(argv[1], "--help") == 0))
			{
			progname = argv[0]; // I wouldn't recommend to use it under windows as system uses full path which is irritating
			printf( "%s v%s - crc32 calculator.\n", progname, "0.2" );
			printf( "\nUsage:\n");
			printf( " %s < file \n", progname);
			printf( " cat file | %s \n", progname);
			printf( " %s  file.name \n", progname);
			return 0;
			}
		is = fopen(argv[1], "rb");
		if (is == NULL) {
			fprintf(stderr, "input file `%s' failed to open.\n", argv[1]);
			return 1;
			}
		}
	
	crc = calc_crc32_stream(is);
	printf("%08x\n", (crc ^ 0xffffffffU));
	
	return 0;
	}
//int main(int argc, char** argv)
