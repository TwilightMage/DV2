#pragma once

#define DV2_BUFSIZ	32768
#define DV2_COMPRESSION	1

/*
 * dv2.h December 2019 vometia a.k.a. Christine Munro
 *
 * DV2 headers: though one struct would cover both eventualities, both
 * have been written out separately for reference.
 *
 * Also beware: either the reported documentation about V4 headers
 * and/or the extrapolation about V5 headers is incorrect or the
 * information is unused (or used inconsistently) but e.g. stuff like
 * aligned and packed always seems to be set to 0 & 1 respectively
 * whether or not the values are ignored.
 *
 * It would appear that DKS determines whether or not a file is
 * compressed by whether or not the uncompressed size in filedata_t has
 * a value, in which case perhaps the nomenclature should be changed to
 * not explicitly state compressed and uncompressed sizes.
 *
 * The point being that when (cough) this is updated to repack files,
 * it should just use whichever values are always found in the header
 * instead of trying to be clever about it.
 */

/*
 * Commands and subcommands: use #defines rather than enums so we can OR
 * stuff like TESTING.
 *
 * DON'T FORGET to add to cmdargs_t array in dv2.c if adding new
 * commands here!  And that it's indexed by DV2_CMD_whatevs so must be
 * in the correct order with no gaps.
 */

#define DV2_CMD_DUNNO		000 // okay, the difference is semantic but
#define DV2_CMD_UNSET		000 // for the sake of clarity!
#define DV2_CMD_PACK		001
#define DV2_CMD_UNPACK		002
#define DV2_CMD_LIST		003
#define DV2_CMD_TEST		010
#define DV2_CMD_COMPRESS	020

#define DV2_CMD_MASK		007
#define DV2_SUBCMD_MASK		070

#define DV2_CMD(cmd)		((cmd) & DV2_CMD_MASK)
#define DV2_SUBCMD(cmd)		((cmd) & DV2_SUBCMD_MASK)

#define DV2_ISLIST(cmd)		(DV2_CMD(cmd) == DV2_CMD_LIST)
#define DV2_ISPACK(cmd)		(DV2_CMD(cmd) == DV2_CMD_PACK)
#define DV2_ISUNPACK(cmd)	(DV2_CMD(cmd) == DV2_CMD_UNPACK)
#define DV2_ISTEST(cmd)		(DV2_SUBCMD(cmd) == DV2_CMD_TEST)
#define DV2_ISCOMPR(cmd)	(DV2_SUBCMD(cmd) == DV2_CMD_COMPRESS)

#define DV2_ARGS_NONE		{ 0, 0 }
#define DV2_ARGS_DUNNO		DV2_ARGS_NONE
#define DV2_ARGS_PACK		{ 2, 2 }
#define DV2_ARGS_UNPACK		{ 1, 2 }
#define DV2_ARGS_LIST		{ 1, 1 }

typedef unsigned int cmd_t;

#ifdef DV2_C // put this stuff here to keep it in one place.

#define NEXTARG(a)		{ if(c < 1)\
				    usage(p, 1, "nextarg: not enough args");\
				  (a) = v++[0];\
				  --c;\
				}

typedef struct {
	int		a_min;
	int		a_max;
} cmdargs_t;

typedef struct {
	char		*c_string;
	cmd_t		c_cmd;
} cmdlist_t;

static cmdargs_t cmdargs[] = {
	DV2_ARGS_DUNNO,
	DV2_ARGS_PACK,
	DV2_ARGS_UNPACK,
	DV2_ARGS_LIST
};

static cmdlist_t cmdlist[] = {
	{ "list",	DV2_CMD_LIST },
	{ "dv2list",	DV2_CMD_LIST },
	{ "pack",	DV2_CMD_PACK },
	{ "dv2pack",	DV2_CMD_PACK },
	{ "unpack",	DV2_CMD_UNPACK },
	{ "dv2unpack",	DV2_CMD_UNPACK },
	{ 0,		DV2_CMD_UNSET }
};

static char *cmds[] = {
	"dunno",
	"pack",
	"unpack",
	"list"
};

#endif // DV2_C

#define	DV2_HEADER_VERSION	5
#define DV2_HEADER_U1		1
#define DV2_HEADER_U2		4
#define DV2_HEADER_ALIGNED	0
#define DV2_HEADER_PACKED	1

typedef struct header_v4 {
	unsigned int	h_id;
	unsigned char	h_isaligned;	// on 32K boundaries, apparently
	unsigned char	h_ispacked;	// zlib compression or not
	unsigned int	h_start;	// this is *added* to filedata.fd_start
	unsigned int	h_filenames;	// size of section containing null-
} header_v4_t;				// terminated filenames.

typedef struct header_v5 {
	unsigned int	h_id;
	unsigned int	h_unknown1;	// usually 1
	unsigned int	h_unknown2;	// usually 4
	unsigned char	h_isaligned;	// usually 0
	unsigned char	h_ispacked;	// usually 1, regardless of content
	unsigned int	h_start;
	unsigned int	h_filenames;
} header_v5_t;

/*
 * XXX need to actually code this properly!  Program should "taste"
 * the first four bytes and take it from there instead of blindly
 * trying to read in the entire struct in one go.
 *
 * Update: DONE, unpack now reads h_id before the rest of the header;
 * though it still uses the V5 header struct either way, there are
 * separate sizeddata_t definitions for packing it from either V4 or
 * V5 data.
 */

typedef header_v5_t header_t;

/*
 * "filedata" struct as it appears in the DV2 archive.  There should
 * in theory be h_filenames worth of these.
 */

typedef struct {
	unsigned int	fd_start;
	unsigned int	fd_lz;	// compressed data size
	unsigned int	fd_l;	// uncompressed data size
} filedata_t;

/* vim:set syntax=c nospell tabstop=8: */
