#pragma once

/*
 * io.h
 *
 * Very basic stuff for the very basic getbytes(), i.e. read in n bytes'
 * worth of data (or a zero-terminated string if unspecified) from the
 * current position.
 */

#define SD_ENTRY(x)	{ (void*)(&(x)), sizeof(x) }
#define SD_ENTRYS(x)	{ (void*)(&(x)), 0 }
#define SD_PAD(x)	{ 0, sizeof(x) }
#define SD_END		{ 0, 0 }

typedef struct
{
	void* s_data;
	unsigned int s_size;
} sizeddata_t;

int getBytesRaw(const TUniquePtr<IFileHandle>&, int, void*);
int getBytesString(const TUniquePtr<IFileHandle>&, FString&);
int getsizeddata(const TUniquePtr<IFileHandle>&, sizeddata_t*);

/* vim:set syntax=c nospell tabstop=8: */
