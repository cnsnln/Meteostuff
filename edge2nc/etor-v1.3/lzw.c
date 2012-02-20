/*----------------------------------------------------------------------**
**
** lzw.c -- Compress data using LZW technique
**
**----------------------------------------------------------------------**
**
** DESCRIPTION
**
** Compress data using LZW technique.
**
**
**
** USAGE:
**
** void set_lzw_verbose(setting)            
** int setting;                          -* Value to set verbose level    *-
**
** int lzw_compress( inputaddr, insize, outputaddr, outsize)
** unsigned char *inputaddr;             -* Data to compress              *-
** int insize;                           -* Size of input data            *-
** unsigned char *outputaddr;            -* Place for compressed data     *-
** int outsize;                          -* Maximum compressed size       *-
**
** int lzw_uncompress( inputaddr, insize, outputaddr, outsize )
** unsigned char *inputaddr;             -* Point to compressed data      *-
** int insize;                           -* Size of compressed data       *-
** unsigned char *outputaddr;            -* Place for uncompressed data   *-
** int outsize;                          -* Maximum uncompressed size     *-
**
**
** PROCESSING:
**
** This is the LZW module which implements a more powerful version
** of the algorithm.  This version of the program has three major
** improvements over LZW12.C.  First, it expands the maximum code size
** to 15 bits.  Second, it starts encoding with 9 bit codes, working
** its way up in bit size only as necessary.  Finally, it flushes the
** dictionary when done.
**
** Note that under MS-DOS this program needs to be built using the
** Compact or Large memory model.
**
**
** COPYRIGHT NOTICE
**
**	Copyright (c) 1997 by Enterprise Electronics Corporation
**	All Rights Reserved
** 
** This program is copyright by Enterprise Electronics  Corporation,
** Enterprise,  Alabama,  USA  36330 (334) 347-3478.  It is licensed
** for use on a specific CPU and is not to be transferred or  other-
** wise divulged. Copies or modifications of this program must carry
** this copyright notice.
** 
** HEADER INFOMATION
**
**	Software Suite		- EDGE
**	Package			- dap
**	Reference number	- SP1/PGM/
**	Revision number		- $Revision: 1.1.1.1 $
**	Release state		- $State: Exp $
**	Author, designer	- Terry Walsh, Unisys
**				  From "The Data Compression Book", Mark Nelson
**	Source file		- $Source: /nfs/trmm/src/CVS/etor/lzw.c,v $
**	Modification date	- $Date: 1999/03/15 16:26:02 $
**	Modified by		- $Author: merritt $
** 
**
** MODIFICATION RECORD
**
** $Log: lzw.c,v $
** Revision 1.1.1.1  1999/03/15 16:26:02  merritt
** Original import.
**
 * Revision 1.3  1997/06/03  13:04:24  bobstaff
 * Changed copyright message
 *
 * Revision 1.2  1997/02/14  15:51:12  bobstaff
 * Post Pakistan Version
 *
 * Revision 1.1  1995/12/04  16:56:21  edge
 * Initial revision
 *
 * Revision 1.1  1994/09/09  01:43:59  stafford
 * Initial revision
 *
**
**----------------------------------------------------------------------*/

/*----------------------------------------------------------------------*/
/* Feature Test Switches                                                */
/*----------------------------------------------------------------------*/
#define _POSIX_SOURCE	1

/*----------------------------------------------------------------------*/
/* System Headers                                                       */
/*----------------------------------------------------------------------*/
#include	<stdio.h>	/* stdio library			*/
#include	<stdlib.h>	/* Some standard funct.			*/
#include	<string.h>	/* String functions			*/

/*----------------------------------------------------------------------*/
/* Application Headers                                                  */
/*----------------------------------------------------------------------*/
#include "bitx.h"

/*----------------------------------------------------------------------*/
/* Macros                                                               */
/*----------------------------------------------------------------------*/

/*
 * Constants used throughout the program.  BITS defines the maximum
 * number of bits that can be used in the output code.  TABLE_SIZE defines
 * the size of the dictionary table.  TABLE_BANKS are the number of
 * 256 element dictionary pages needed.  The code defines should be
 * self-explanatory.
 */

#define BITS		15
#define MAX_CODE	( ( 1 << BITS ) - 1 )
#define TABLE_SIZE	35023L
#define TABLE_BANKS	( ( TABLE_SIZE >> 8 ) + 1 )
#define END_OF_STREAM	 256
#define BUMP_CODE	 257
#define FLUSH_CODE	 258
#define FIRST_CODE	 259
#define UNUSED		 -1
#define DICT( i ) dict[ i >> 8 ][ i & 0xff ]


/*----------------------------------------------------------------------*/
/* External (Import) Variables                                          */
/*----------------------------------------------------------------------*/

extern void dump_bit_file();
extern BIT_STRM *OpenOutputBitStream();
extern BIT_STRM *OpenInputBitStream();
extern int nextc();
extern int outc();
extern int CloseOutputBitStream();
extern int CloseInputBitStream();
extern void StreamOutputBit();
extern void StreamOutputBits();
extern int StreamInputBit();
extern unsigned long StreamInputBits();

/*----------------------------------------------------------------------*/
/* Global (Export) Variables                                            */
/*----------------------------------------------------------------------*/
int lzw_verbose = 0;

/*----------------------------------------------------------------------*/
/* Local (Static) Variables                                             */
/*----------------------------------------------------------------------*/

/*
 * This data structure defines the dictionary.  Each entry in the dictionary
 * has a code value.  This is the code emitted by the compressor.  Each
 * code is actually made up of two pieces:  a parent_code, and a
 * character.  Code values of less than 256 are actually plain
 * text codes.
 *
 * Note that in order to handle 16 bit segmented compilers, such as most
 * of the MS-DOS compilers, it was necessary to break up the dictionary
 * into a table of smaller dictionary pointers.  Every reference to the
 * dictionary was replaced by a macro that did a pointer dereference first.
 * By breaking up the index along byte boundaries we should be as efficient
 * as possible.
 */

static struct dictionary 
{
	int code_value;
	int parent_code;
	char character;
} *dict[ TABLE_BANKS ];

static int inited = 0;

/*
 * Other global data structures.  The decode_stack is used to reverse
 * strings that come out of the tree during decoding.  next_code is the
 * next code to be added to the dictionary, both during compression and
 * decompression.  current_code_bits defines how many bits are currently
 * being used for output, and next_bump_code defines the code that will
 * trigger the next jump in word size.
 */

static char decode_stack[ TABLE_SIZE ];
static unsigned int next_code;
static int current_code_bits;
static unsigned int next_bump_code;

/*----------------------------------------------------------------------*/
/* Local Function                                                       */
/*----------------------------------------------------------------------*/

/*----------------------------------------------------------------------*/
/*
 * This routine is used to initialize the dictionary, both when the
 * compressor or decompressor first starts up, and also when a flush
 * code comes in.  Note that even thought the decompressor sets all
 * the code_value elements to UNUSED, it doesn't really need to.
 */
/*----------------------------------------------------------------------*/

static void InitializeDictionary()
{
	unsigned int i;

	for ( i = 0 ; i < TABLE_SIZE ; i++ )
		DICT( i ).code_value = UNUSED;
	next_code = FIRST_CODE;
	if(lzw_verbose) putc( 'F', stderr );
	current_code_bits = 9;
	next_bump_code = 511;
}

/*----------------------------------------------------------------------*/
/*
 * This routine allocates the dictionary.  Since the total size of the
 * dictionary is much larger than 64K, it can't be allocated as a single
 * object.  Instead, it is allocated as a set of pointers to smaller
 * dictionary objects.  The special DICT() macro is used to translate
 * indices into pairs of references.
 */
/*----------------------------------------------------------------------*/

static struct dictionary dict_store[ TABLE_BANKS ][256];
static void InitializeStorage()
{
	int i;

	for ( i = 0 ; i < TABLE_BANKS ; i++ )
		dict[ i ] = dict_store[ i ];

}

/*----------------------------------------------------------------------*/
/*
 * This hashing routine is responsible for finding the table location
 * for a string/character combination.  The table index is created
 * by using an exclusive OR combination of the prefix and character.
 * This code also has to check for collisions, and handles them by
 * jumping around in the table.
 */
/*----------------------------------------------------------------------*/

static unsigned int find_child_node( parent_code, child_character )
int parent_code;
int child_character;
{
	unsigned int index;
	int offset;

	index = ( child_character << ( BITS - 8 ) ) ^ parent_code;
	if ( index == 0 )
		offset = 1;
	else
		offset = TABLE_SIZE - index;
	for ( ; ; )
	{
		if ( DICT( index ).code_value == UNUSED )
			return( (unsigned int) index );
		if ( DICT( index ).parent_code == parent_code &&
		    DICT( index ).character == (char) child_character )
			return( index );
		if ( (int) index >= offset )
			index -= offset;
		else
			index += TABLE_SIZE - offset;
	}
}

/*----------------------------------------------------------------------*/
/*
 * This routine decodes a string from the dictionary, and stores it
 * in the decode_stack data structure.  It returns a count to the
 * calling program of how many characters were placed in the stack.
 */
/*----------------------------------------------------------------------*/

static unsigned int decode_string( unsigned int count, unsigned int code )
{
	while ( code > 255 )
	{
		decode_stack[ count++ ] = DICT( code ).character;
		code = DICT( code ).parent_code;
	}
	decode_stack[ count++ ] = (char) code;
	return( count );
}
/*----------------------------------------------------------------------*/
/* Main Function                                                        */
/*----------------------------------------------------------------------*/

/*-END OF MODULE--------------------------------------------------------*/





void set_lzw_verbose(setting)
int setting;
{
	lzw_verbose = setting;
}



#ifdef NEVER
/*----------------------------------------------------------------------*/
/* Local Functions:							*/
/*----------------------------------------------------------------------*/
/*
 * Local prototypes.
 */

#ifdef __STDC__
unsigned int find_child_node( int parent_code, int child_character );
unsigned int decode_string( unsigned int offset, unsigned int code );
#else
unsigned int find_child_node();
unsigned int decode_string();
#endif
#endif





/*----------------------------------------------------------------------*/
/*
 * The compressor is short and simple.  It reads in new symbols one
 * at a time from the input file.  It then  checks to see if the
 * combination of the current symbol and the current code are already
 * defined in the dictionary.  If they are not, they are added to the
 * dictionary, and we start over with a new one symbol code.  If they
 * are, the code for the combination of the code and character becomes
 * our new code.  Note that in this enhanced version of LZW, the
 * encoder needs to check the codes for boundary conditions.
 */
/*----------------------------------------------------------------------*/

int lzw_compress( char *inputaddr, int insize, char *outputaddr, int outsize)
{
	int character;
	int string_code;
	unsigned int index;
	BIT_STRM *in_bs;
	BIT_STRM *out_bs;
#ifdef DUMP_ARGS
	printf("lzw_compress(inputaddr:%lx,insize:%d,outputaddr:%lx,outsize:%d\n",
		inputaddr,insize,outputaddr,outsize);
#endif /* DUMP_ARGS */

	in_bs = OpenInputBitStream( (unsigned char *)inputaddr, insize );
	out_bs = OpenOutputBitStream( (unsigned char *)outputaddr, outsize );

	InitializeStorage();
	InitializeDictionary();
	if ( ( string_code = nextc( in_bs ) ) == EOF )
		string_code = END_OF_STREAM;
	while ( ( character = nextc( in_bs ) ) != EOF )
	{
		index = find_child_node( string_code, character );
		if ( DICT( index ).code_value != - 1 )
			string_code = DICT( index ).code_value;
		else 
		{
			DICT( index ).code_value = next_code++;
			DICT( index ).parent_code = string_code;
			DICT( index ).character = (char) character;
			StreamOutputBits( out_bs,
			    (unsigned long) string_code, current_code_bits );
			string_code = character;
			if ( next_code > MAX_CODE )
			{
				StreamOutputBits( out_bs,
				   (unsigned long)FLUSH_CODE,current_code_bits);
				InitializeDictionary();
			}
			else if ( next_code > next_bump_code )
			{
				StreamOutputBits( out_bs,
				    (unsigned long)BUMP_CODE,current_code_bits);
				current_code_bits++;
				next_bump_code <<= 1;
				next_bump_code |= 1;
				if(lzw_verbose) putc( 'B', stderr );
			}
		}
	}
	StreamOutputBits( out_bs, (unsigned long)string_code, 
	    current_code_bits );
	StreamOutputBits( out_bs, (unsigned long) END_OF_STREAM, 
	    current_code_bits);
	CloseInputBitStream( in_bs );
	return CloseOutputBitStream( out_bs );
}

/*----------------------------------------------------------------------*/
/*
 * The file expander operates much like the encoder.  It has to
 * read in codes, the convert the codes to a string of characters.
 * The only catch in the whole operation occurs when the encoder
 * encounters a CHAR+STRING+CHAR+STRING+CHAR sequence.  When this
 * occurs, the encoder outputs a code that is not presently defined
 * in the table.  This is handled as an exception.  All of the special
 * input codes are handled in various ways.
 */
/*----------------------------------------------------------------------*/

int lzw_uncompress( unsigned char *inputaddr, int insize, 
	unsigned char *outputaddr, int outsize )
{
	unsigned int new_code;
	unsigned int old_code;
	int character;
	unsigned int count;
	BIT_STRM *in_bs;
	BIT_STRM *out_bs;
#ifdef DUMP_ARGS
	printf("lzw_uncompress(%lx %d %lx %d)\n",inputaddr,insize,
		outputaddr, outsize);
#endif /* DUMP_ARGS */

	in_bs = OpenInputBitStream( (unsigned char *)inputaddr, insize );
	out_bs = OpenOutputBitStream( (unsigned char *)outputaddr, outsize );

	InitializeStorage();
	for ( ; ; )
	{
		InitializeDictionary();
		old_code = (unsigned int) StreamInputBits( in_bs, 
		    current_code_bits );
		if ( old_code == END_OF_STREAM )
		{
			CloseInputBitStream( in_bs );
			return CloseOutputBitStream( out_bs );
		}
		character = old_code;
		outc( old_code, out_bs );
		for ( ; ; )
		{
			new_code = (unsigned int) StreamInputBits( in_bs, 
			    current_code_bits );
			if ( new_code == END_OF_STREAM )
			{
				CloseInputBitStream( in_bs );
				return CloseOutputBitStream( out_bs );
			}
			if ( new_code == FLUSH_CODE )
				break;
			if ( new_code == BUMP_CODE )
			{
				current_code_bits++;
				if(lzw_verbose) putc( 'B', stderr );
				continue;
			}
			if ( new_code >= next_code )
			{
				decode_stack[ 0 ] = (char) character;
				count = decode_string( 1, old_code );
			}
			else
				count = decode_string( 0, new_code );
			character = decode_stack[ count - 1 ];
			while ( count > 0 )
				outc( decode_stack[ --count ], out_bs );
			DICT( next_code ).parent_code = old_code;
			DICT( next_code ).character = (char) character;
			next_code++;
			old_code = new_code;
		}
	}
}

/*-END OF MODULE--------------------------------------------------------*/