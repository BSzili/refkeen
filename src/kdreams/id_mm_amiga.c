/* Keen Dreams Source Code
 * Copyright (C) 2014 Javier M. Chavez
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

// Lightweight memory manager using the C library allocation functions

#include "id_heads.h"
#define Point AmigaPoint
#include <proto/exec.h>

#define USE_NODE_SIZE

typedef struct mmnodestruct
{
	struct MinNode node;
	id0_int_t purge;
	id0_boolean_t locked;
#ifdef USE_NODE_SIZE
	id0_unsigned_long_t size;
#endif
	id0_unsigned_t attributes;
} mmnodetype;

mminfotype	mminfo;
memptr		bufferseg;
id0_boolean_t		mmerror;

void		(* beforesort) (void);
void		(* aftersort) (void);

id0_boolean_t		mmstarted;
id0_boolean_t		bombonerror;
struct MinList memlist;

void MM_Startup (void)
{
	if (mmstarted)
		MM_Shutdown ();

	mmstarted = true;
	bombonerror = true;

	mminfo.nearheap = AvailMem(MEMF_FAST);
	mminfo.farheap = AvailMem(MEMF_CHIP);
	mminfo.mainmem = mminfo.nearheap + mminfo.farheap;
	mminfo.EMSmem = 0;
	mminfo.XMSmem = 0;

	MM_GetPtr (&bufferseg,BUFFERSIZE);
}

void MM_Shutdown (void)
{
	if (!mmstarted)
		return;

	MM_FreePtr (&bufferseg);
}

static mmnodetype *MML_GetNewNode(id0_unsigned_long_t size)
{
	mmnodetype *node = calloc(1, sizeof(mmnodetype) + size);
	if (node)
	{
#ifdef USE_NODE_SIZE
		node->size = size;
#endif
		AddTail((struct List *)&memlist, (struct Node *)node);
	}

	return node;
}

static void MML_PurgeNodes(void)
{
	mmnodetype *node, *node2;

	VW_ColorBorder(15);
	// not very sophisticated, but the game only uses purge level 0 and 3
	for (node = (mmnodetype *)memlist.mlh_Head; node2 = (mmnodetype *)node->node.mln_Succ; node = node2)
	{
		if (!node->locked && node->purge)
		{
			//bug("%s purging node %p\node", __FUNCTION__, node);
			Remove((struct Node *)node);
			free(node);
		}
	}
	VW_ColorBorder(0);
}

void MM_GetPtr (memptr *baseptr,id0_unsigned_long_t size)
{
	mmnodetype *node;

	//bug("%s(%p,%u)\n", __FUNCTION__, baseptr, size);

	*baseptr = NULL;
	node = MML_GetNewNode(size);
	if (!node)
	{
		// try to free some purgable nodes
		MML_PurgeNodes();
		node = MML_GetNewNode(size);
	}

	if (node)
	{
		// success
		*baseptr = (memptr)(node + 1);
		//bug("%s %p -> %p %u\n", __FUNCTION__, baseptr, *baseptr, size);
	}

	if (*baseptr)
		return;

	Quit ("MM_GetPtr: Out of memory!");
}

void MM_FreePtr (memptr *baseptr)
{
	mmnodetype *node;

	//bug("%s(%p)\n", __FUNCTION__, baseptr);

	if (!baseptr || !*baseptr)
		return;

	node = ((mmnodetype *)*baseptr - 1);
	Remove((struct Node *)node);
	free(node);

	//bug("%s %p -> %p\n", __FUNCTION__, baseptr, *baseptr);
	*baseptr = NULL;
}

void MM_SetPurge (memptr *baseptr, id0_int_t purge)
{
	mmnodetype *node;

	//bug("%s(%p,%d)\n", __FUNCTION__, baseptr, purge);

	if (!baseptr || !*baseptr)
		return;

	node = ((mmnodetype *)*baseptr - 1);
	node->purge = purge;
}

void MM_SetLock (memptr *baseptr, id0_boolean_t locked)
{
	mmnodetype *node;

	//bug("%s(%p,%d)\n", __FUNCTION__, baseptr, locked);

	if (!baseptr || !*baseptr)
		return;

	node = ((mmnodetype *)*baseptr - 1);
	node->locked = locked;
}

void MM_SortMem (void)
{
}

void MM_ShowMemory (void)
{
}

id0_long_t MM_UnusedMemory (void)
{
	return AvailMem(MEMF_ANY);
}

id0_long_t MM_TotalFree (void)
{
	id0_unsigned_long_t free;
	mmnodetype *node;

	free = AvailMem(MEMF_ANY);
#ifdef USE_NODE_SIZE
	for (node = (mmnodetype *)memlist.mlh_Head; node->node.mln_Succ; node = (mmnodetype *)node->node.mln_Succ)
	{
		if (!node->locked && node->purge)
		{
			//bug("free %lu size %lu\n", free, node->size);
			free += node->size;
		}
	}
#endif
	return free;
}

void MM_BombOnError (id0_boolean_t bomb)
{
	bombonerror = bomb;
}

id0_unsigned_t MM_GetAttributes (memptr *baseptr)
{
	mmnodetype *node;

	//bug("%s(%p)\n", __FUNCTION__, baseptr);

	if (!baseptr || !*baseptr)
		return;

	node = ((mmnodetype *)*baseptr - 1);

	return node->attributes;
}

void MM_SetAttributes (memptr *baseptr, id0_unsigned_t attributes)
{
	mmnodetype *node;

	//bug("%s(%p,%d)\n", __FUNCTION__, baseptr, attributes);

	if (!baseptr || !*baseptr)
		return;

	node = ((mmnodetype *)*baseptr - 1);
	node->attributes = attributes;
}