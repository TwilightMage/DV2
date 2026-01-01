#include "DV2Importer/utils.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include "DV2Importer/debug.h"
#include "HAL/PlatformFileManager.h"

#pragma warning(push)
#pragma warning(disable : 4706)

/*
 * Simple cache of directory names for finddir() so we don't have to
 * check every time.
 */

#define DIRTABLE_SIZE 257 // does it help if this is prime?  Dunno really.

typedef struct dirs_s
{
	char* d_name;
	unsigned long d_hash;
	struct dirs_s* d_next;
} dirs_t;

static int checkdir(char*);

void* allocate(int nbytes)
{
	void* ptr = 0;

	if (nbytes && (ptr = malloc(nbytes)) == NULL)
		fatal_v(nullptr, TEXT("malloc of %d byte(s) failed"), nbytes);

	return ptr;
}

char* copy(const char* str)
{
	int l = strlen(str);
	char* result = 0;

	if ((result = (char*)allocate(l + 1)))
		strcpy(result, str);
	return result;
}

/*
 * djb2 hash function from http://www.cse.yorku.ca/~oz/hash.html
 */

unsigned long hash(char* str)
{
	unsigned long hash = 5381;
	int c;

	while ((c = *str++))
		hash += (hash << 5) + c;

	return hash;
}

char* finddir(char* dirname, int makedirs)
{
	static dirs_t** dirs = 0;

	unsigned long hash(char*), h;
	dirs_t* dp;
	char* p;

	if (*dirname == '\\')
		++dirname;
	h = hash(dirname);
	if (dirs == NULL)
	{
		dirs = allocate<dirs_t*>(sizeof(dirs_t*) * DIRTABLE_SIZE);
		memset(dirs, '\0', sizeof(dirs_t*) * DIRTABLE_SIZE);
	}
	dp = dirs[h % DIRTABLE_SIZE];
	while (dp && dp->d_hash != h && strcmp(dp->d_name, dirname))
		dp = dp->d_next;

	/*
 	 * Not found in history, so check, store and create as
 	 * necessary
 	 */

	if (dp == NULL)
	{
		for (p = dirname; *p; p++)
			if (*p == '\\')
				*p = '/';
		checkpath(p);
		if (dirs[h % DIRTABLE_SIZE])
			debug(TEXT("dirhash %3lu * %hs"), h % DIRTABLE_SIZE, dirname)
		else
			debug(TEXT("dirhash %3lu   %hs"), h % DIRTABLE_SIZE, dirname);
		dp = allocate<dirs_t>(sizeof(dirs_t));
		dp->d_hash = h;
		dp->d_name = copy(dirname);
		dp->d_next = dirs[h % DIRTABLE_SIZE];
		dirs[h % DIRTABLE_SIZE] = dp;
		if (makedirs && checkdir(dirname) == 0)
			do_mkdir(dirname);
	}
	return dp->d_name;
}

/*
 * Check for poisoned file/directory names.  Although DV2s don't *seem* to
 * be passed around, no point being careless.
 */

void checkpath(char* path)
{
	if (strstr(path, "../"))
		error_v(, TEXT("archive contains invalid directory name: \"%hs\""), path);
}

/*
 * See if a directory already exists; and if *something* exists, that it's
 * actually a directory!
 */

static int checkdir(char* path)
{
	struct stat s;
	int r = 0;

	if (stat(path, &s) != -1)
	{
		if (s.st_mode & S_IFDIR)
			r = 1;
		else
			fatal(TEXT("%hs exists but is not a directory"), path);
	}
	return r;
}

/*
 * Equivalent of "mkdir -p"
 */

void do_mkdir(char* path)
{
	FPlatformFileManager::Get().GetPlatformFile().CreateDirectoryTree(*FString(path));
}

/* vim:set syntax=c nospell tabstop=8: */

#pragma warning(pop)
