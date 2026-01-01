/*
 * A Divinity 2/Ego Draconis archive handler.
 *
 * December 2019 Vometia a.k.a. Christine Munro
 *
 * io.c - reads/writes data in what seems in hindsight to be
 * an unnecessarily convoluted manner.  Could arguably have the
 * thing with "buffering already buffered input" chucked but as
 * we still need a means of reading null-terminated strings using
 * stdio and checking endianness I'm not sure how a rewrite would
 * be an improvement.  The main problem is that its scope changed
 * as I was figuring out how to deal with DV2s.
 */

#include "io.h"
#include "DV2Importer/io.h"

#include <stdio.h>
#include <string.h>

#include "DV2Importer/debug.h"

#define ENDIAN_LITTLE	0
#define ENDIAN_BIG	1

typedef union
{
	unsigned char b[4];
	unsigned short s[2];
	unsigned int l;
} end_t;

static int endian(int);
static void byteorder(end_t*, int);

/*
 * Read nbytes' worth of data from the current file position, or a
 * zero-terminated string if no size is specified.
 *
 * Also resist the temptation for it to remember and restore its
 * current position if necessary, just as a note to myself if I
 * have that "good idea" again.  If something else changes it and
 * we need to come back (which seems unlikely at the time of writing)
 * it's the something else's responsibility to restore it.
 */

int getBytesRaw(const TUniquePtr<IFileHandle>& fileHandle, int nbytes, void* ptr)
{
	return fileHandle->Read((uint8*)ptr, nbytes) ? nbytes : 0;
}

int getBytesString(const TUniquePtr<IFileHandle>& fileHandle, FString& str)
{
	str = "";
	TCHAR ch = 0;
	size_t numRead = 0;
	while (fileHandle->Read((uint8*)&ch, 1))
	{
		numRead++;
		if (ch == 0)
			return numRead;
		str += ch;
	}

	error(TEXT("getbytesString: unexpected end of file"));
}


int getsizeddata(const TUniquePtr<IFileHandle>& FileHandle, sizeddata_t* dp)
{
	int r = 0;

	while (dp->s_data)
	{
		bool isString = dp->s_size == 0;

		if ((isString
			     ? getBytesString(FileHandle, *(FString*)dp->s_data)
			     : getBytesRaw(FileHandle, dp->s_size, dp->s_data)) == EOF)
			error(TEXT("getsizedata: insufficient data left for %d byte(s)"), dp->s_size)
		if (dp->s_size)
			byteorder((end_t*)dp->s_data, dp->s_size);
		++dp;
		++r;
	}
	return r;
}

/*
 * isn't there a more convenient way of figuring this out?  *shrug*
 */

static int endian(int endianness)
{
	static end_t e = {.b = {1, 0, 0, 0}};
	int r = e.l & 0xff;

	return r ^ endianness;
}


/*
 * Swap bytes if necessary.  Untested but hopefully works!  Except
 * on a Vax, which is the only other thing I have handy, as a Vax's
 * idea of little-endian is bitwise rather than bytes, so instead
 * of byte order being 1, 2, 3, 4 vs. 4, 3, 2, 1, the Vax likes the
 * idea of bits 0-31 being LSB-MSB rather than MSB-LSB.
 *
 * So instead of the number 0x12345678 as written by a big-endian CPU
 * such as a Motorola 68K or IBM System/370 coming out as 0x78563412
 * on the little-endian x86, as we might expect, a Vax would see it as
 * 0x1e6a2c48, i.e. complete bit-reversal.  At least I think so.  If
 * that number seems to not make sense you need to look at it in binary
 * which I may as well do here.
 *
 * Our hypothetical number 0x12345678 as written in binary format by
 * e.g. a 68030 and read by various architectures:
 *
 *   Big-endian:    0001,0010,0011,0100,0101,0110,0111,1000
 *   Little-endian: 0111,1000,0101,0110,0011,0100,0001,0010
 *   Vax-endian:    0001,1110,0110,1010,0010,1100,0100,1000
 *
 * Of course there are other architectures too, such as those which
 * swap octets based on a 16-bit word, and then there's the likes of
 * the PDP-10 which has no real interest in bytes of any size, just
 * 36-bits or GTFO.
 */

static void byteorder(end_t* s, int l)
{
	int t, i;

	if (endian(ENDIAN_BIG) && l > 1)
	{
		for (i = 0; i < l / 2; i++)
		{
			t = s->b[i];
			s->b[i] = s->b[l - 1 - i];
			s->b[l - 1 - i] = t;
		}
	}
}

/* vim:set syntax=c nospell tabstop=8: */
