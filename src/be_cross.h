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

#ifndef BE_CROSS_H
#define BE_CROSS_H

#include <stdbool.h>
#include <stdio.h>

#define __STDC_FORMAT_MACROS
#include <inttypes.h>

// WARNING: It's actually safer to include this earlier (e.g., for endianness
// related macros), but this header may be the one included first
#include "refkeen_config.h"

typedef enum BE_Log_Message_Class_T
{
	BE_LOG_MSG_NORMAL, BE_LOG_MSG_WARNING, BE_LOG_MSG_ERROR
} BE_Log_Message_Class_T;

#define BE_Cross_TypedMax(T, x, y) ({T _x = (x), _y = (y); (_x > _y) ? _x : _y;})
#define BE_Cross_TypedMin(T, x, y) ({T _x = (x), _y = (y); (_x < _y) ? _x : _y;})

#define BE_Cross_Swap16(x) ((uint16_t)(((uint16_t)(x)<<8)|((uint16_t)(x)>>8)))

#define BE_Cross_Swap32(x) ((uint32_t)(((uint32_t)(x)<<24)|(((uint32_t)(x)<<8)&0x00FF0000)|(((uint32_t)(x)>>8)&0x0000FF00)|((uint32_t)(x)>>24)))

#define BE_Cross_Swap64(x) ((uint64_t)(((uint64_t)(x)<<56)|(((uint64_t)(x)<<40)&0x00FF000000000000UL)|(((uint64_t)(x)<<24)&0x0000FF0000000000UL)|(((uint64_t)(x)<<8)&0x000000FF00000000UL)| \
                           (((uint64_t)(x)>>8)&0x00000000FF000000UL)|(((uint64_t)(x)>>24)&0x0000000000FF0000UL)|(((uint64_t)(x)>>40)&0x000000000000FF00UL)|((uint64_t)(x)>>56)))

#ifdef REFKEEN_ARCH_LITTLE_ENDIAN
#define BE_Cross_Swap16LE(x) (x)
#define BE_Cross_Swap16BE(x) BE_Cross_Swap16(x)
#define BE_Cross_Swap32LE(x) (x)
#define BE_Cross_Swap32BE(x) BE_Cross_Swap32(x)
#define BE_Cross_Swap64LE(x) (x)
#define BE_Cross_Swap64BE(x) BE_Cross_Swap64(x)
#else
#define BE_Cross_Swap16LE(x) BE_Cross_Swap16(x)
#define BE_Cross_Swap16BE(x) (x)
#define BE_Cross_Swap32LE(x) BE_Cross_Swap32(x)
#define BE_Cross_Swap32BE(x) (x)
#define BE_Cross_Swap64LE(x) BE_Cross_Swap64(x)
#define BE_Cross_Swap64BE(x) (x)
#endif

// Used for some resource definitions internally
#ifdef REFKEEN_ARCH_LITTLE_ENDIAN
#define BE_Cross_SwapGroup16LE(a, b)       a, b,
#define BE_Cross_SwapGroup24LE(a, b, c)    a, b, c,
#define BE_Cross_SwapGroup32LE(a, b, c, d) a, b, c, d,
#else
#define BE_Cross_SwapGroup16LE(a, b)       b, a,
#define BE_Cross_SwapGroup24LE(a, b, c)    c, b, a,
#define BE_Cross_SwapGroup32LE(a, b, c, d) d, c, b, a,
#endif

inline char *BE_Cross_ultoa_dec(uint32_t n, char *buffer)
{
	sprintf(buffer, "%" PRIu32, n);
	return buffer;
}

inline char *BE_Cross_ltoa_dec(int32_t n, char *buffer)
{
	sprintf(buffer, "%" PRId32, n);
	return buffer;
}

inline char *BE_Cross_itoa_dec(int16_t n, char *buffer)
{
	sprintf(buffer, "%" PRId16, n);
	return buffer;
}

// Used for debugging
void BE_Cross_LogMessage(BE_Log_Message_Class_T msgClass, const char *format, ...);
// More (possibly semi) standard C functions emulated,
// taking English locale into account (and more, but NOT all)
inline int BE_Cross_toupper(int c)
{
	return ((c >= 'a') && (c <= 'z')) ? (c - 'a' + 'A') : c;
}
inline int BE_Cross_tolower(int c)
{
	return ((c >= 'A') && (c <= 'Z')) ? (c - 'A' + 'a') : c;
}
inline int BE_Cross_isupper(int c)
{
	return ((c >= 'A') && (c <= 'Z'));
}

int BE_Cross_strcasecmp(const char *s1, const char *s2);
int BE_Cross_strncasecmp(const char *s1, const char *s2, size_t count);

/* A safe(r) string copying function that:
 * - Gets a pointer to the destination buffer's end (little performance bit).
 * - Returns the END of the written string. (This differs from strcpy!)
 * - Is ALLOWED to write past the written string's end, if there's room
 * (e.g., any additional null terminator).
 *
 * ASSUMPTIONS: The pointers do point to valid buffers, there's enough room for
 * a null terminator, and the source and destination buffers do NOT overlap.
 */
inline char *BE_Cross_safeandfastcstringcopy(char *dest, char *destEnd, const char *src)
{
	char ch;
	// We assume that initially, destEnd - dest > 0.
	do
	{
		ch = *src++;
		*dest++ = ch; // This includes the null terminator if there's the room
	} while ((dest < destEnd) && ch);
	// These work in case dest == destEnd, and also if not
	--dest;
	*dest = '\0';
	return dest; // WARNING: This differs from strcpy!!
}

// A few convenience functions for concatenating multiple strings together
inline char *BE_Cross_safeandfastcstringcopy_2strs(char *dest, char *destEnd, const char *src0, const char *src1)
{
	return BE_Cross_safeandfastcstringcopy(BE_Cross_safeandfastcstringcopy(dest, destEnd, src0), destEnd, src1);
}

inline char *BE_Cross_safeandfastcstringcopy_3strs(char *dest, char *destEnd, const char *src0, const char *src1, const char *src2)
{
	return BE_Cross_safeandfastcstringcopy(BE_Cross_safeandfastcstringcopy(BE_Cross_safeandfastcstringcopy(dest, destEnd, src0), destEnd, src1), destEnd, src2);
}

inline char *BE_Cross_safeandfastcstringcopy_4strs(char *dest, char *destEnd, const char *src0, const char *src1, const char *src2, const char *src3)
{
	return BE_Cross_safeandfastcstringcopy(BE_Cross_safeandfastcstringcopy(BE_Cross_safeandfastcstringcopy(BE_Cross_safeandfastcstringcopy(dest, destEnd, src0), destEnd, src1), destEnd, src2), destEnd, src3);
}


// This one should be called early
void BE_Cross_PrepareAppPaths(void);

// Game installations stuff

#define BE_CROSS_MAX_ROOT_PATHS 32

// Describes a required file from a specific game version
typedef struct {
	const char *filename;
	int filesize;
	uint32_t crc32;
} BE_GameFileDetails_T;

// Maps each game version to an array of game file details, ending with a NULL entry (filename == NULL)
extern const BE_GameFileDetails_T *g_be_gamefiledetails_ptrs[];

const char *BE_Cross_GetGameInstallationDescription(int num);
int BE_Cross_GetGameVerFromInstallation(int num);
extern int g_be_gameinstallations_num;

// gameVer should be BE_GAMEVER_LAST if no specific version is desired
void BE_Cross_StartGame(int gameVerVal, int argc, char **argv, void (*mainFuncPtr)(void));

/*** Use for game versions selection ***/

int BE_Cross_DirSelection_GetNumOfRootPaths(void);
const char **BE_Cross_DirSelection_GetRootPathsNames(void);
const char **BE_Cross_DirSelection_Start(int rootPathIndex, int *outNumOfSubDirs); // Start dir selection
void BE_Cross_DirSelection_Finish(void); // Finish dir selection
const char **BE_Cross_DirSelection_GetNext(int dirIndex, int *outNumOfSubDirs); // Enter dir by index into last array
const char **BE_Cross_DirSelection_GetPrev(int *outNumOfSubDirs); // Go up in the filesystem hierarchy


/*** Use for game EXEs (main functions) selection ***/

// Gets matching description string (AS A C STRING LITERAL), if found, otherwise returns NULL.
const char *BE_Cross_GetEXEFileDescriptionStrForGameVer(const char *exeFileName, int verId);
// Gets matching main function pointer, if accessible by the user, otherwise returns default function pointer for game version
void (*BE_Cross_GetAccessibleMainFuncPtrForGameVer(const char *exeFileName, int verId))(void);
// Returns the amount of main functions accessible by the user, for the given game version
int BE_Cross_GetAccessibleEXEsCountForGameVer(int verId);
// Fills array with C STRING LITERALS consisting of descriptions of the accessible main functions.
//
// *** WARNING ***
// The outStrs array *must* have at least BE_Cross_GetAccessibleEXEsCountForGameVer(verId) pointers.
void BE_Cross_FillAccessibleEXEFileNamesForGameVer(int verId, const char **outStrs);
// Returns main function pointer for the given version id, indexed by "index", in the same
// order in which BE_Cross_FillAccessibleEXEFileNamesForGameVer fills the descriptive strings.
void (*BE_Cross_GetAccessibleEXEFuncPtrForGameVerByIndex(int index, int verId))(void);

// If select game version has digitized sounds with a common sample rate,
// then this rate is returned, otherwise 0 is returned.
int BE_Cross_GetSelectedGameVerSampleRate(void);

typedef char BE_TryAddGameInstallation_ErrorMsg_T[40];

// Attempt to add a game installation from currently selected dir;
// Returns BE_GAMEVER_LAST if no new supported game version is found; Otherwise game version id is returned.
// The given array is used in order to report an error for each checked version, in case of failure.
//
// Array MUST have at least BE_GAMEVER_LAST elements.
int BE_Cross_DirSelection_TryAddGameInstallation(BE_TryAddGameInstallation_ErrorMsg_T errorMsgsArray[]);

// Often used as a replacement for file handles of type "int",
// this one is given a different name so it's easy to swap in case of a need
typedef FILE * BE_FILE_T;

inline BE_FILE_T BE_Cross_IsFileValid(BE_FILE_T fp) { return fp; }
inline BE_FILE_T BE_Cross_GetNilFile(void) { return NULL; }
inline int BE_Cross_seek(BE_FILE_T fp, long int offset, int origin) { return fseek(fp, offset, origin); }
inline int BE_Cross_putc(int character, BE_FILE_T fp) { return putc(character, fp); }
inline int BE_Cross_getc(BE_FILE_T fp) { return getc(fp); }

int32_t BE_Cross_FileLengthFromHandle(BE_FILE_T fp);

// Semi cross-platform file opening wrappers, hiding search paths
BE_FILE_T BE_Cross_open_readonly_for_reading(const char *filename); // For e.g., read-only gamedata files like Catacomb 3-D's EGAGRAPH.C3D
BE_FILE_T BE_Cross_open_rewritable_for_reading(const char *filename); // For e.g., rewritable files like saved games
BE_FILE_T BE_Cross_open_rewritable_for_overwriting(const char *filename); // For the same rewritable files
// Used for NEW files not originating from the originals (like RefKeen cfg)
BE_FILE_T BE_Cross_open_additionalfile_for_reading(const char *filename);
BE_FILE_T BE_Cross_open_additionalfile_for_overwriting(const char *filename);
// Should be shared
inline void BE_Cross_close(BE_FILE_T fp)
{
	if (!fp) // This may happen e.g., if Keen Dreams doesn't find the GAMEMAPS file (from ID_CA.C)
		BE_Cross_LogMessage(BE_LOG_MSG_WARNING, "WARNING: Attempted to close a file from a NULL pointer.\n");
	else
		fclose(fp);
}

// SPECIAL - Attempts to open config/config.vdf file from Steam
// installation, used for importing controller mappings.
//
// Do *NOT* assume any specific format for the newline chars!
// (May be LF or CR-LF)
#ifdef REFKEEN_CONFIG_CHECK_FOR_STEAM_INSTALLATION
BE_FILE_T BE_Cross_open_steamcfg_for_reading(void);
#endif

// Loads a file originally embedded into the EXE (for DOS) to a newly allocated
// chunk of memory. Should be freed with BE_Cross_free_mem_loaded_embedded_rsrc.
// Returns chunk size if successful, or a negative number in case of failure.
int BE_Cross_load_embedded_rsrc_to_mem(const char *filename, void **ptr);

// Frees file loaded using BE_Cross_load_embedded_rsrc_to_mem. Accepts a NULL pointer.
void BE_Cross_free_mem_loaded_embedded_rsrc(void *ptr);

// Outputs a list of *rewritable* file names (e.g., saved games),
// matching given name suffix from a corresponding "search path"
// (used by an implementation of gelib.c:ReadGameList from Catacomb Abyss)
// and sorted alphabetically in a case-insensitive manner (English locale).
//
// Output is upper-case and has the suffix removed. If a filename length is
// at least strLenBound characters (excluding the suffix), only the first
// strLenBound-1 characters are stored. In addition, if there are more than
// maxNum matching files, the last maxNum entries are written.
//
// Input is assumed to point to a buffer (bidimensional array) of size
// maxNum*strLenBound;
//
// Returns: Number of filled entries.
int BE_Cross_GetSortedRewritableFilenames_AsUpperCase(char *outFilenames, int maxNum, int strLenBound, const char *suffix);

// Used for preparing game installations and search paths
void BE_Cross_PrepareGameInstallations(void);

// Semi cross-platform binary (non-textual) file I/O, where it can be used directly (config file)
size_t BE_Cross_readInt8LE(BE_FILE_T fp, void *ptr);
size_t BE_Cross_readInt16LE(BE_FILE_T fp, void *ptr);
size_t BE_Cross_readInt32LE(BE_FILE_T fp, void *ptr);
size_t BE_Cross_readInt8LEBuffer(BE_FILE_T fp, void *ptr, size_t nbyte);
size_t BE_Cross_readInt16LEBuffer(BE_FILE_T fp, void *ptr, size_t nbyte);
size_t BE_Cross_readInt32LEBuffer(BE_FILE_T fp, void *ptr, size_t nbyte);
// This exists for the EGAHEADs from the Catacombs
size_t BE_Cross_readInt24LEBuffer(BE_FILE_T fp, void *ptr, size_t nbyte);
// A template for enum reading (from 16-bit little-endian int).
// A declaration and implementation must exist for each used type separately
// (implementation should be found in be_cross.c).
#if 0
size_t BE_Cross_read_EnumType_From16LE(BE_FILE_T fp, EnumType *ptr);
#endif
// boolean implementation may be separated from enums, otherwise it's the same
size_t BE_Cross_read_boolean_From16LE(BE_FILE_T fp, bool *ptr);
size_t BE_Cross_read_boolean_From32LE(BE_FILE_T fp, bool *ptr);
// booleans buffer
size_t BE_Cross_read_booleans_From16LEBuffer(BE_FILE_T fp, bool *ptr, size_t nbyte);
size_t BE_Cross_read_booleans_From32LEBuffer(BE_FILE_T fp, bool *ptr, size_t nbyte);

// Same but for writing
size_t BE_Cross_writeInt8LE(BE_FILE_T fp, const void *ptr);
size_t BE_Cross_writeInt16LE(BE_FILE_T fp, const void *ptr);
size_t BE_Cross_writeInt32LE(BE_FILE_T fp, const void *ptr);
size_t BE_Cross_writeInt8LEBuffer(BE_FILE_T fp, const void *ptr, size_t nbyte);
size_t BE_Cross_writeInt16LEBuffer(BE_FILE_T fp, const void *ptr, size_t nbyte);

#if 0
size_t BE_Cross_write_EnumType_To16LE(BE_FILE_T fp, const EnumType *ptr);
#endif

size_t BE_Cross_write_boolean_To16LE(BE_FILE_T fp, const bool *ptr);
size_t BE_Cross_write_boolean_To32LE(BE_FILE_T fp, const bool *ptr);
size_t BE_Cross_write_booleans_To16LEBuffer(BE_FILE_T fp, const bool *ptr, size_t nbyte);
size_t BE_Cross_write_booleans_To32LEBuffer(BE_FILE_T fp, const bool *ptr, size_t nbyte);

// Assuming segPtr is replacement for a 16-bit segment pointer, and offInSegPtr
// is a replacement for an offset in this segment (pointing to a place in the
// emulated segment), increases offInSegPtr by count with wrapping
// (if offInSegPtr-segPtr >= 0x10000 after wrapping, offInSegPtr is reduced by 0x10000)
void BE_Cross_Wrapped_Add(uint8_t *segPtr, uint8_t **offInSegPtrPtr, uint16_t count);
// Same as above but with count == 1 forced
void BE_Cross_Wrapped_Inc(uint8_t *segPtr, uint8_t **offInSegPtrPtr);
// Similar; Used for copying from linear buffer to cyclic 0x10000 bytes long "segment"
void BE_Cross_LinearToWrapped_MemCopy(uint8_t *segDstPtr, uint8_t *offDstPtr, const uint8_t *linearSrc, uint16_t num);
// Vice-versa
void BE_Cross_WrappedToLinear_MemCopy(uint8_t *linearDst, const uint8_t *segSrcPtr, const uint8_t *offSrcPtr, uint16_t num);
// Similar, but under a common segment.
// ASSUMPTION: Buffers do not overlap!!
void BE_Cross_WrappedToWrapped_MemCopy(uint8_t *segCommonPtr, uint8_t *offDstPtr, const uint8_t *offSrcPtr, uint16_t num);
// Wrapped memset
void BE_Cross_Wrapped_MemSet(uint8_t *segPtr, uint8_t *offInSegPtr, int value, uint16_t num);

// Returns an offset that should be added to the 16-bit segments of 32-bit
// far pointers present in The Catacomb Armageddon/Apocalypse saved games
// (in the case of the original DOS exes, it depends on the locations of
// modified copies of them in memory)
uint16_t BE_Cross_Compat_GetFarPtrRelocationSegOffset(void);

// Alternatives for Borland's randomize and random macros used in Catacomb Abyss
void BE_Cross_Brandomize(void);
int16_t BE_Cross_Brandom(int16_t num);

// UNSAFE alternative for Borland's getdate function used in Keen Dreams v1.00;
//
// Do NOT call the function from multiple threads!
// (Internally it uses localtime, which isn't thread-safe on Linux.)
void BE_Cross_GetLocalDate_UNSAFE(int *y, int *m, int *d);

/*** Memory management functions - used as alternatives to     ***/
/*** Borland C++ functions/macros in 16-bit real-mode codebase ***/

void *BE_Cross_Bmalloc(uint16_t size);
void *BE_Cross_Bfarmalloc(uint32_t size);
void BE_Cross_Bfree(void *ptr);
void BE_Cross_Bfarfree(void *ptr);

inline uint16_t BE_Cross_Bcoreleft(void)
{
	extern uint16_t g_nearBytesLeft;
	return g_nearBytesLeft;
}

inline uint32_t BE_Cross_Bfarcoreleft(void)
{
	extern uint32_t g_farBytesLeft;
	return g_farBytesLeft;
}

// Use **ONLY* with memory allocated by BE_Cross_Bmalloc/BE_Cross_Bfarmalloc:
//
// Somewhat similar to FP_SEG, *but* returns the segment of
// the *normalized* pointer's form (where the offset is < 16)
inline uint16_t BE_Cross_GetPtrNormalizedSeg(void *ptr)
{
	extern uint8_t g_be_emulatedMemSpace[];
	return ((uint8_t *)ptr-g_be_emulatedMemSpace)/16;
}

// Use **ONLY* with memory allocated by BE_Cross_Bmalloc/BE_Cross_Bfarmalloc:
//
// Somewhat similar to FP_OFF, *but* returns the offset of
// the *normalized* pointer's form (which is always < 16)
inline uint16_t BE_Cross_GetPtrNormalizedOff(void *ptr)
{
	extern uint8_t g_be_emulatedMemSpace[];
	return ((uint8_t *)ptr-g_be_emulatedMemSpace)%16;
}

// Use **ONLY* with memory allocated by BE_Cross_Bmalloc/BE_Cross_Bfarmalloc:
// Converts segment to given pointer (like MK_FP(seg, 0))
inline void *BE_Cross_BGetPtrFromSeg(uint16_t seg)
{
	extern uint8_t g_be_emulatedMemSpace[];
	return g_be_emulatedMemSpace + seg*16;
}

// Use **ONLY* with memory allocated by BE_Cross_Bmalloc/BE_Cross_Bfarmalloc:
// A kind of a MK_FP replacement.
inline void *BE_Cross_BMK_FP(uint16_t seg, uint16_t off)
{
	extern uint8_t g_be_emulatedMemSpace[];
	return g_be_emulatedMemSpace + seg*16 + off;
}

// Used ONLY before calling a main function - this loads a piece of
// an original (DOS) EXE (a chunk of data) to near memory.
//
// This can be done AS LONG AS a copy of the UNCOMPRESSED EXE IMAGE
// is internally loaded to memory.
void *BE_Cross_BmallocFromEmbeddedData(const char *name, uint16_t *pSize);

// Same as above, but loads data to far memory
void *BE_Cross_BfarmallocFromEmbeddedData(const char *name, uint32_t *pSize);

// Use this in cases an original DOS program attempts to access contents of
// segment no. 0 for some reason
extern uint8_t g_be_cross_dosZeroSeg[];

// Variables set before calling a main function
extern const char **g_be_argv;
extern int g_be_argc;

extern void (*be_lastSetMainFuncPtr)(void);

// Roughly a replacement for execv, with a few exceptions/notes:
// - There's NO GUARANTEE the calling "program" (e.g., a game)
// will be ready in its EXACT original case, once its "main" function
// is called again.
// - If, for any reason, memory management was manually done with
// functions like malloc (rather than, say, BE_Cross_Bmalloc),
// such memory is NOT cleaned up automatically.
// - Being optional, a finalizer function pointer can be passed, used
// to RESTORE the calling "program" to its original state, in terms
// of e.g., values of global and static variables. Note that while
// managed memory (e.g., memory allocated by BE_Cross_Bmalloc) may be
// automatically freed by BE_Cross_Bexecv, the pointer VARIABLES still
// have to be restored. Furthermore, again, non-managed memory allocated
// by malloc or another function may remain as a memory leak.
// - The input main function pointer is assumed to point to a function
// that does NOT accept any argument. These can be separatedly accessed.
// - Finally, the last parameter tells if mainFunc expects the argc and argv
// arguments, or not (they're still stored in global variables).
void BE_Cross_Bexecv(void (*mainFunc)(void), const char **argv, void (*finalizer)(void), bool passArgsToMainFunc);

#endif // BE_CROSS_H
