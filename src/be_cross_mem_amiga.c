/* Copyright (C) 2014-2017 NY00123
 *
 * This file is part of Reflection Keen.
 *
 * Reflection Keen is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include <string.h>
#include <stdlib.h>
#include <proto/exec.h>

#include "refkeen_config.h" // MUST precede other contents due to e.g., endianness-based ifdefs

#include "be_cross.h" // For some inline functions (C99)
#include "be_st.h" // For BE_ST_ExitWithErrorMsg

#define USE_NODE_SIZE
#define EMULATED_FAR_PARAGRAPHS 28222

/*** Memory blocks definitions ***/

typedef struct {
	struct MinNode node;
	uint8_t *ptr;
#ifdef USE_NODE_SIZE
	uint32_t len;
#endif
} BE_MemoryBlock_T;

/*static*/ uint32_t g_farBytesLeft = 16*EMULATED_FAR_PARAGRAPHS;

static struct MinList memlist;

static BE_MemoryBlock_T *MML_GetNewNode(uint32_t size)
{
	BE_MemoryBlock_T *node = calloc(1, sizeof(BE_MemoryBlock_T) + size);
	if (node)
	{
#ifdef USE_NODE_SIZE
		node->len = size;
#endif
		AddTail((struct List *)&memlist, (struct Node *)node);
	}

	return node;
}

static void BEL_FreeNode(BE_MemoryBlock_T *node)
{
	Remove((struct Node *)node);
	free(node);
}

void *BE_Cross_Bmalloc(uint16_t size)
{
	BE_MemoryBlock_T *node = MML_GetNewNode(size);
	if (node)
	{
		// success
		return (void *)(node + 1);
	}

	// REFKEEN NOTE - Plain malloc should return NULL,
	// but we rather do the following for debugging
//outofmemory:
	BE_ST_ExitWithErrorMsg("BE_Cross_Bmalloc: Out of memory!");
	return NULL; // Mute compiler warning
}

void *BE_Cross_Bfarmalloc(uint32_t size)
{
	BE_MemoryBlock_T *node = MML_GetNewNode(size);
	if (node)
	{
		// success
		return (void *)(node + 1);
	}

	// REFKEEN NOTE - Plain malloc should return NULL,
	// but we rather do the following for debugging
//outofmemory:
	BE_ST_ExitWithErrorMsg("BE_Cross_Bfarmalloc: Out of memory!");
	return NULL; // Mute compiler warning
}

void BE_Cross_Bfree(void *ptr)
{
	if (ptr == NULL)
		return;

	BE_MemoryBlock_T *node = ((BE_MemoryBlock_T *)ptr - 1);
	BEL_FreeNode(node);

	//BE_ST_ExitWithErrorMsg("BE_Cross_Bfree: Got an invalid pointer!");
}

void BE_Cross_Bfarfree(void *ptr)
{
	if (ptr == NULL)
		return;

	BE_MemoryBlock_T *node = ((BE_MemoryBlock_T *)ptr - 1);
	BEL_FreeNode(node);

	//BE_ST_ExitWithErrorMsg("BE_Cross_Bfarfree: Got an invalid pointer!");
}

void BEL_Cross_ClearMemory(void)
{
	BE_MemoryBlock_T *node, *node2;

	for (node = (BE_MemoryBlock_T *)memlist.mlh_Head; node2 = (BE_MemoryBlock_T *)node->node.mln_Succ; node = node2)
	{
		BEL_FreeNode(node);
	}
}
