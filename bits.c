// bits.c ... functions on bit-strings
// part of signature indexed files
// Bit-strings are arbitrarily long byte arrays
// Least significant bits (LSB) are in array[0]
// Most significant bits (MSB) are in array[nbytes-1]

// Written by John Shepherd, March 2019

#include <assert.h>
#include "defs.h"
#include "bits.h"
#include "page.h"

typedef struct _BitsRep
{
	Count nbits;  // how many bits
	Count nbytes; // how many bytes in array
	// why define as 1 here? what if buffer overflow?
	Byte bitstring[1]; // array of bytes to hold bits
					   // actual array size is nbytes
} BitsRep;

// create a new Bits object

Bits newBits(int nbits)
{
	Count nbytes = iceil(nbits, 8);
	Bits new = malloc(2 * sizeof(Count) + nbytes);
	new->nbits = nbits;
	new->nbytes = nbytes;
	// where is the address? in stack or in heap?
	memset(&(new->bitstring[0]), 0, nbytes);
	return new;
}

// release memory associated with a Bits object

void freeBits(Bits b)
{
	//TODO
	free(b);
}

// check if the bit at position is 1

Bool bitIsSet(Bits b, int position)
{
	assert(b != NULL);
	assert(0 <= position && position < b->nbits);
	//TODO
	// since each byte is only 8 bits
	int nth_byte = position / 8;
	int pth_bit = position % 8;
	if (b->bitstring[nth_byte] & (1 << pth_bit))
	{
		return TRUE;
	}
	return FALSE; // remove this
}

// check whether one Bits b1 is a subset of Bits b2

Bool isSubset(Bits b1, Bits b2)
{
	assert(b1 != NULL && b2 != NULL);
	assert(b1->nbytes == b2->nbytes);
	//TODO
	for (int i = 0; i < b2->nbytes; i++)
	{
		for (int j = 0; j < 8; j++)
		{
			if ((b1->bitstring[i] & (1 << j)) && !(b2->bitstring[i] & (1 << j)))
			{
				return FALSE;
			}
		}
	}
	return TRUE;
}

// set the bit at position to 1

void setBit(Bits b, int position)
{
	assert(b != NULL);
	assert(0 <= position && position < b->nbits);
	//TODO
	int nth_byte = position / 8;
	int pth_bit = position % 8;
	b->bitstring[nth_byte] |= (1 << pth_bit);
}

// set all bits to 1

void setAllBits(Bits b)
{
	assert(b != NULL);
	//TODO
	for (int i = 0; i < b->nbytes; i++)
	{
		b->bitstring[i] |= 0xFF;
	}
}

// set the bit at position to 0

void unsetBit(Bits b, int position)
{
	assert(b != NULL);
	assert(0 <= position && position < b->nbits);
	//TODO
	int nth_byte = position / 8;
	int pth_bit = position % 8;
	b->bitstring[nth_byte] &= ~(1 << pth_bit);
}

// set all bits to 0

void unsetAllBits(Bits b)
{
	assert(b != NULL);
	//TODO
	for (int i = 0; i < b->nbytes; i++)
	{
		b->bitstring[i] |= 0;
	}
}

// bitwise AND ... b1 = b1 & b2

void andBits(Bits b1, Bits b2)
{
	assert(b1 != NULL && b2 != NULL);
	assert(b1->nbytes == b2->nbytes);
	//TODO
	for (int i = 0; i < b1->nbytes; i++)
	{
		b1->bitstring[i] &= b2->bitstring[i];
	}
}

// bitwise OR ... b1 = b1 | b2

void orBits(Bits b1, Bits b2)
{
	assert(b1 != NULL && b2 != NULL);
	assert(b1->nbytes == b2->nbytes);
	//TODO
	for (int i = 0; i < b1->nbytes; i++)
	{
		b1->bitstring[i] |= b2->bitstring[i];
	}
}

// left-shift ... b1 = b1 << n
// negative n gives right shift

void shiftBits(Bits b, int n)
{
	// TODO
	Byte store = 0;
	
	if (n >= 0) {
		for (int i = 0; i < b->nbytes; i++) {
			Byte tmp;
			tmp = store;
			store = b->bitstring[i] >> (8-n);
			b->bitstring[i] = b->bitstring[i] << n;
			b->bitstring[i] |= tmp;
		}
	} else {
		n = -n;
		for (int i = b->nbytes - 1; i >= 0 ; i--) {
			Byte tmp;
			tmp = store;		
			store = b->bitstring[i] << (8-n);
			b->bitstring[i] = b->bitstring[i] >> n;
			b->bitstring[i] |= tmp;
		}
	} 
}

// get a bit-string (of length b->nbytes)
// from specified position in Page buffer
// and place it in a BitsRep structure

void getBits(Page p, Offset pos, Bits b)
{
	//TODO
	Count nbytes = b->nbytes;
	// we need nbytes to decide each tuple's length in order to find nth tuple. pos == nth
	memcpy(b->bitstring, addrInPage(p, pos, nbytes), nbytes);
}

// copy the bit-string array in a BitsRep
// structure to specified position in Page buffer

void putBits(Page p, Offset pos, Bits b)
{
	//TODO
	Byte * address = addrInPage(p, pos, b->nbytes);
	memcpy(address, b->bitstring, b->nbytes);
}

// show Bits on stdout
// display in order MSB to LSB
// do not append '\n'

void showBits(Bits b)
{
	assert(b != NULL);
	//printf("(%d,%d)",b->nbits,b->nbytes);
	for (int i = b->nbytes - 1; i >= 0; i--)
	{
		for (int j = 7; j >= 0; j--)
		{
			Byte mask = (1 << j);
			if (b->bitstring[i] & mask)
				putchar('1');
			else
				putchar('0');
		}
	}
}
