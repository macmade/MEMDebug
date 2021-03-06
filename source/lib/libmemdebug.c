/*******************************************************************************
 * Copyright (c) 2009, Jean-David Gadina - www.xs-labs.com
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 * 
 *  -   Redistributions of source code must retain the above copyright notice,
 *      this list of conditions and the following disclaimer.
 *  -   Redistributions in binary form must reproduce the above copyright
 *      notice, this list of conditions and the following disclaimer in the
 *      documentation and/or other materials provided with the distribution.
 *  -   Neither the name of 'Jean-David Gadina' nor the names of its
 *      contributors may be used to endorse or promote products derived from
 *      this software without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 ******************************************************************************/

/* $Id$ */

/* System includes */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>
#include <signal.h>
#include <stdarg.h>

/* Checks if we are compiling under Mac OS X */
#if defined( __APPLE__ )

/* The backtrace functions are available */
#include <execinfo.h>
#define MEMDEBUG_HAVE_EXECINFO_H

/*  */
#include <malloc/malloc.h>

#elif defined( __GLIBC__ ) && defined( HAVE_EXECINFO_H )

/* The backtrace functions are available */
#include <execinfo.h>
#define MEMDEBUG_HAVE_EXECINFO_H

#endif

/* Local includes */
#include "libmemdebug.h"

/* Undefines the standard memory macros, so we can use the real memory functions */
#undef malloc
#undef calloc
#undef realloc
#undef free

/* Undefines the alloca macro if it has been defined (GCC 3 and greater) */
#if defined( _ALLOCA_H_ ) && defined( __GNUC__ ) && __GNUC__ >= 3
#undef alloca
#endif

/* Undefines the Objective-C garbage collector memory macros if they have been defined */
#if defined( OBJC_WITH_GC ) && OBJC_WITH_GC
#undef GC_malloc
#undef GC_malloc_atomic
#undef GC_calloc
#undef GC_realloc
#endif

#ifdef _MALLOC_MALLOC_H_
#undef malloc_zone_malloc
#undef malloc_zone_calloc
#undef malloc_zone_valloc
#undef malloc_zone_free
#undef malloc_zone_realloc
#endif

/* Macro to check if MEMDebug was inited (if not, it will init it) */
#define MEMDEBUG_INIT_CHECK if( memdebug_inited == MEMDEBUG_FALSE ) { memdebug_init(); }

/* Horizontal ruler */
#define MEMDEBUG_HR "#-----------------------------------------------------------------------------------------------------------------\n"

#define MEMDEBUG_REV "# \n# $Revision$\n# $Date$\n"

/* The supported allocation types */
#define MEMDEBUG_ALLOC_TYPE_MALLOC                  0x0001L
#define MEMDEBUG_ALLOC_TYPE_VALLOC                  0x0002L
#define MEMDEBUG_ALLOC_TYPE_CALLOC                  0x0004L
#define MEMDEBUG_ALLOC_TYPE_REALLOC                 0x0008L
#define MEMDEBUG_ALLOC_TYPE_STD                     0x000FL
#define MEMDEBUG_ALLOC_TYPE_ALLOCA_FUNC             0x0010L
#define MEMDEBUG_ALLOC_TYPE_ALLOCA_BUILTIN          0x0020L
#define MEMDEBUG_ALLOC_TYPE_ALLOCA                  0x00F0L
#define MEMDEBUG_ALLOC_TYPE_OBJC_GC_MALLOC          0x0100L
#define MEMDEBUG_ALLOC_TYPE_OBJC_GC_MALLOC_ATOMIC   0x0200L
#define MEMDEBUG_ALLOC_TYPE_OBJC_GC_CALLOC          0x0400L
#define MEMDEBUG_ALLOC_TYPE_OBJC_GC_REALLOC         0x0800L
#define MEMDEBUG_ALLOC_TYPE_OBJC_GC                 0x0F00L
#define MEMDEBUG_ALLOC_TYPE_ZONE_MALLOC             0x1000L
#define MEMDEBUG_ALLOC_TYPE_ZONE_CALLOC             0x2000L
#define MEMDEBUG_ALLOC_TYPE_ZONE_VALLOC             0x4000L
#define MEMDEBUG_ALLOC_TYPE_ZONE_REALLOC            0x8000L
#define MEMDEBUG_ALLOC_TYPE_ZONE                    0xF000L

/* The number of bytes for each line of the memory data dump */
#define MEMDEBUG_DUMP_BYTES 24

/* The value of the fence, used to check for buffer overflows */
#define MEMDEBUG_FENCE_VAL 0x3CC3L

/* The size of the fence value  */
#define MEMDEBUG_FENCE_SIZE sizeof( memdebug_fence )

/* The size of an allocation, plus the two fences values */
#define MEMDEBUG_ALLOC_SIZE( size ) ( size ) + ( 2 * MEMDEBUG_FENCE_SIZE )

/* Definition of the type for the allocation type */
typedef unsigned long int memdebug_alloc_type;

/* The type used for the fence */
typedef unsigned long int memdebug_fence;

/* Definition of a boolean type, as usual */
typedef enum { MEMDEBUG_FALSE = 0, MEMDEBUG_TRUE = 1 } memdebug_bool;

/* Structure for a memory record */
struct memdebug_object
{
    /* The pointer to the allocated memory area */
    void * ptr;
    
    /* The size of the record in the memory */
    size_t size;
    
    /* Whether the object was freed or not */
    memdebug_bool free;
    
    /* The name of the file in which the object was allocated */
    const char * alloc_file;
    
    /* The line of the file in which the object was allocated */
    int alloc_line;
    
    /* The name of the function in which the object was allocated */
    const char * alloc_func;
    
    /* The allocation type (MEMDEBUG_ALLOC_TYPE_XXX) */
    memdebug_alloc_type alloc_type;
    
    /* The name of the file in which the object was freed */
    const char * free_file;
    
    /* The line of the file in which the object was freed */
    int free_line;
    
    /* The name of the function in which the object was freed */
    const char * free_func;
    
    /* Checks if we are using GCC */
    #ifdef __GNUC__
    
    /* The address of the function in which the object was allocated */
    void * alloc_func_addr;
    
    /* The address of the function in which the object was freed */
    void * free_func_addr;
    
    #endif
};

/* Structure for the memory trace pool */
struct memdebug_pool
{
    /* The memory record pool itself */
    struct memdebug_object * objects;
    
    /* The current size of the memory record pool */
    unsigned long int pool_size;
    
    /* The total number of memory records in the pool */
    unsigned long int num_objects;
    
    /* The number of active (non-freed) memory records in the pool */
    unsigned long int num_active;
    
    /* The number of freed memory records in the pool */
    unsigned long int num_free;
    
    /* The number of automatically freed memory records in the pool */
    unsigned long int num_auto;
    
    /* The total memory usage */
    size_t memory_total;
    
    /* The memory usage of active (non-freed) memory records */
    size_t memory_active;
};

/* Prototypes for the internal (private) functions */
static void memdebug_fatal( const char * format, ... );
static void memdebug_init( void );
static struct memdebug_object * memdebug_new_object( void * ptr, size_t size, const char * file, const int line, const char * func, memdebug_alloc_type alloc_type );
static void memdebug_update_object( void * ptr, void * ptr_new, size_t size, const char * file, const int line, const char * func, memdebug_alloc_type alloc_type );
static void memdebug_free_object( void * ptr, const char * file, const int line, const char * func );
static struct memdebug_object * memdebug_get_object( void * ptr );
static void memdebug_write_fence( struct memdebug_object * object );
static memdebug_bool memdebug_check_fence( struct memdebug_object * object );
static void memdebug_dump( struct memdebug_object * object );
static void memdebug_ask_debug_cmd( void );
static void memdebug_sig_handler( int id );
static void memdebug_warning( const char * str, const char * file, const int line, const char * func, ... );
static void memdebug_print_object( struct memdebug_object * object );

/* Checks if we can have a backtrace */
#ifdef MEMDEBUG_HAVE_EXECINFO_H
static void memdebug_backtrace( unsigned int skip_levels );
#endif

/* Whether MEMDebug has been inited or not */
static memdebug_bool memdebug_inited;

/* The MEMDebug memory record pool */
static struct memdebug_pool * memdebug_trace;

/**
 * Internal fatal error
 * 
 * @param   const char *    The error message
 * @param   ...             The format parameters, if any
 */
static void memdebug_fatal( const char * format, ... )
{
    va_list ap;
    
    /* Gets the variable arguments */
    va_start( ap, format );
    
    /* Prints the error message */
    vprintf( format, ap );
    
    /* Aborts the program execution */
    va_end( ap );
    exit( EXIT_FAILURE );
}

/**
 * Initializes MEMDebug
 * 
 * @return  void
 */
static void memdebug_init( void )
{
    struct sigaction sa1;
    struct sigaction sa2;
    
    /* Nothing to do if MEMDebug is already initialized */
    if( memdebug_inited == MEMDEBUG_TRUE )
    {
        return;
    }
    
    /* Signal handling */
    sa1.sa_handler = memdebug_sig_handler;
    sa1.sa_flags   = 0;
    sigemptyset( &sa1.sa_mask );
    
    /* Handles segmentation faults ( SIGSEGV ) */
    if( sigaction( SIGSEGV, &sa1, &sa2 ) != 0 )
    {
        memdebug_fatal
        (
            "MEMDebug error: cannot set a handler for SIGSEGV\n"
        );
    }
    
    /* Handles bus errors ( SIGBUS ) */
    if( sigaction( SIGBUS, &sa1, &sa2 ) != 0 )
    {
        memdebug_fatal
        (
            "MEMDebug error: cannot set a handler for SIGBUS\n"
        );
    }
    
    /* Allocates the memory record pool structure */
    if( NULL == ( memdebug_trace = ( struct memdebug_pool * )calloc( 1, sizeof( struct memdebug_pool ) ) ) )
    {
        memdebug_fatal
        (
            "MEMDebug error: cannot initialize the trace pool. Reason: %s\n",
            strerror( errno )
        );
    }
    
    /* Allocates room for memory record objects */
    if( NULL == ( memdebug_trace->objects = ( struct memdebug_object * )calloc( MEMDEBUG_POOL_SIZE, sizeof( struct memdebug_object ) ) ) )
    {
        memdebug_fatal
        (
            "MEMDebug error: cannot initialize the trace pool. Reason: %s\n",
            strerror( errno )
        );
    }
    
    /* Pool initialization */
    memdebug_trace->num_objects   = 0;
    memdebug_trace->num_active    = 0;
    memdebug_trace->num_free      = 0;
    memdebug_trace->memory_total  = 0;
    memdebug_trace->memory_active = 0;
    memdebug_trace->pool_size     = MEMDEBUG_POOL_SIZE;
    memdebug_inited               = MEMDEBUG_TRUE;
}

/**
 * Creates a new memory record object in the pool
 * 
 * @param   void *                      The pointer to the allocated memory area
 * @param   size_t                      The size of the allocated memory area
 * @param   const char *                The file in which the allocation was made
 * @param   const int                   The line of the file in which the allocation was made
 * @param   const char *                The name of the function in which the allocation was made
 * @param   const char *                The allocation type (MEMDEBUG_ALLOC_TYPE_XXX)
 * @return  struct memdebug_object *    The new memory record object
 */
static struct memdebug_object * memdebug_new_object( void * ptr, size_t size, const char * file, const int line, const char * func, memdebug_alloc_type alloc_type )
{
    struct memdebug_object * object;
    
    /* Initialization check */
    MEMDEBUG_INIT_CHECK;
    
    /* Checks there's enough room in the current pool for a new object */
    if( memdebug_trace->pool_size == memdebug_trace->num_objects )
    {
        /* No, let's reallocate some memory */
        if( NULL == ( memdebug_trace->objects = ( struct memdebug_object * )realloc( memdebug_trace->objects, memdebug_trace->pool_size + MEMDEBUG_POOL_SIZE ) ) )
        {
            memdebug_fatal
            (
                "MEMDebug error: cannot reallocate the MEMDebug pool. Reason: %s\n",
                strerror( errno )
            );
        }
        
        /* Sets the new pool size */
        memdebug_trace->pool_size += MEMDEBUG_POOL_SIZE;
    }
    
    /* Increments the counter as we have a new object */
    memdebug_trace->num_objects++;
    
    object = &memdebug_trace->objects[ memdebug_trace->num_objects - 1 ];
    
    object->ptr        = ptr;
    object->size       = size;
    object->alloc_file = file;
    object->alloc_line = line;
    object->alloc_func = func;
    object->alloc_type = alloc_type;
    
    /* Checks if the object is autamatically freed (alloca or GC functions) */
    if( alloc_type & MEMDEBUG_ALLOC_TYPE_ALLOCA || alloc_type & MEMDEBUG_ALLOC_TYPE_OBJC_GC )
    {
        /* The object will be automatically freed */
        object->free = MEMDEBUG_TRUE;
        memdebug_trace->num_auto++;
        
    }
    else
    {
        /* The object will need to be manually freed */
        object->free = MEMDEBUG_FALSE;
        memdebug_trace->num_active++;
    }
    
    /* Checks if we are using GCC */
    #ifdef __GNUC__
    
    /* Stores the address of the caller function */
    object->alloc_func_addr = __builtin_return_address( 2 );
    
    #endif
    
    /* Checks if the allocation was made using dynamic memory */
    if( !( alloc_type & MEMDEBUG_ALLOC_TYPE_ALLOCA ) )
    {
        /* Updates the memory usage */
        memdebug_trace->memory_total  += size;
        memdebug_trace->memory_active += size;
    }
    
    memdebug_write_fence( object );
    
    /* Returns the new object */
    return object;    
}

/**
 * Updates an existing memory record object in the pool
 * 
 * @param   void *          The address of the memory area corresponding to the memory record object
 * @param   void *          The new address of the memory area for the memory record object
 * @param   size_t          The size of the allocated memory area
 * @param   const char *    The file in which the allocation was made
 * @param   const int       The line of the file in which the allocation was made
 * @param   const char *    The name of the function in which the allocation was made
 * @param   const char *    The allocation type (MEMDEBUG_ALLOC_TYPE_XXX)
 * @return  void
 */
static void memdebug_update_object( void * ptr, void * ptr_new, size_t size, const char * file, const int line, const char * func, memdebug_alloc_type alloc_type )
{
    struct memdebug_object * object;
    
    /* Checks if the address we are trying to reallocate exists in the pool */
    if( NULL == ( object = memdebug_get_object( ptr ) ) )
    {
        memdebug_warning
        (
            "Trying to reallocate a non-existing object (pointer address: %p)",
            file,
            line,
            func,
            ptr
        );
    }
    
    /* Checks if the memory area was already freed */
    if( object->free == MEMDEBUG_TRUE )
    {
        memdebug_warning
        (
            "Trying to reallocate a freed object (pointer address: %p)",
            file,
            line,
            func,
            ptr
        );
    }
    
    if( memdebug_check_fence( object ) == MEMDEBUG_FALSE )
    {
        memdebug_warning
        (
            "A buffer overflow was detected (pointer address: %p)",
            file,
            line,
            func,
            ptr
        );
    }
    
    /* Updates the memory record informations */
    object->ptr        = ptr_new;
    object->alloc_file = file;
    object->alloc_line = line;
    object->alloc_func = func;
    object->alloc_type = alloc_type;
    
    /* Updates the pool memory size and usage */
    memdebug_trace->memory_total  -= object->size;
    memdebug_trace->memory_active -= object->size;
    memdebug_trace->memory_total  += size;
    memdebug_trace->memory_active += size;
    
    /* Updates the size of the memory record */
    object->size = size;
    
    /* Checks if we are using GCC */
    #ifdef __GNUC__
    
    /* Stores the address of the caller function */
    object->alloc_func_addr = __builtin_return_address( 2 );
    
    #endif
}

/**
 * Marks an existing memory record object as free
 * 
 * @param   void *          The address of the memory area corresponding to the memory record object
 * @param   const char *    The file in which the deallocation was made
 * @param   const int       The line of the file in which the deallocation was made
 * @param   const char *    The name of the function in which the deallocation was made
 * @return  void
 */
static void memdebug_free_object( void * ptr, const char * file, const int line, const char * func )
{
    struct memdebug_object * object;
    
    /* Checks if the address we are trying to free exists in the pool */
    if( NULL == ( object = memdebug_get_object( ptr ) ) )
    {
        memdebug_warning
        (
            "Trying to free a non-existing object (pointer address: %p)",
            file,
            line,
            func,
            ptr
        );
    }
    
    /* Checks if the memory area was already freed */
    if( object->free == MEMDEBUG_TRUE )
    {
        memdebug_warning
        (
            "Trying to free a freed object (pointer address: %p)",
            file,
            line,
            func,
            ptr
        );
    }
    
    if( memdebug_check_fence( object ) == MEMDEBUG_FALSE )
    {
        memdebug_warning
        (
            "A buffer overflow was detected (pointer address: %p)",
            file,
            line,
            func,
            ptr
        );
    }
    
    /* Udpates the memory record object */
    object->free      = MEMDEBUG_TRUE;
    object->free_file = file;
    object->free_line = line;
    object->free_func = func;
    
    /* Checks if we are using GCC */
    #ifdef __GNUC__
    
    /* Stores the address of the caller function */
    object->free_func_addr = __builtin_return_address( 1 );
    
    #endif
    
    /* Updates the memory usage */
    memdebug_trace->num_active--;
    memdebug_trace->num_free++;
    memdebug_trace->memory_active -= object->size;
}

/**
 * Gets a memory record object from the pool
 * 
 * @param   void *                      The address of the memory area corresponding to the memory record object
 * @return  struct memdebug_object *    The memory record object, or NULL if it was not found in the pool
 */
static struct memdebug_object * memdebug_get_object( void * ptr )
{
    unsigned int i;
    
    /* Initialization check */
    MEMDEBUG_INIT_CHECK;
    
    /* Tries to find the object in the pool */
    for( i = 0; i < memdebug_trace->num_objects; i++ )
    {
        /* Checks if the given pointer is the same as the one in the memory record object */
        if( memdebug_trace->objects[ i ].ptr == ptr )
        {
            /* Yes, returns the memory record object corresponding to the given pointer */
            return &memdebug_trace->objects[ i ];
        }
    }
    
    /* No memory record object corresponding to the given pointer */
    return NULL;
}

/**
 * 
 */
static void memdebug_write_fence( struct memdebug_object * object )
{
    memdebug_fence * ptr;
    
    ptr      = ( ( memdebug_fence * )object->ptr - 1 );
    *( ptr ) = MEMDEBUG_FENCE_VAL;
    ptr      = ( memdebug_fence * )( ( char * )ptr + object->size ) + 1;
    *( ptr ) = MEMDEBUG_FENCE_VAL;
}

/**
 * 
 */
static memdebug_bool memdebug_check_fence( struct memdebug_object * object )
{
    memdebug_fence * ptr;
    
    ptr  = ( ( memdebug_fence * )object->ptr - 1 );
    
    if( *( ptr ) != MEMDEBUG_FENCE_VAL )
    {
        return MEMDEBUG_FALSE;
    }
    
    ptr = ( memdebug_fence * )( ( char * )ptr + object->size ) + 1;
    
    if( *( ptr ) != MEMDEBUG_FENCE_VAL )
    {
        return MEMDEBUG_FALSE;
    }
    
    return MEMDEBUG_TRUE;
}

/**
 * Allocates some memory
 * 
 * @param   size_t          The memory size to allocate
 * @param   const char *    The file in which the call was made
 * @param   const int       The line of the file in which the call was made
 * @param   const char *    The name of the function in which the call was made
 * @return  void *          A pointer to the allocated memory area
 */
void * memdebug_malloc( size_t size, const char * file, const int line, const char * func )
{
    void * ptr;
    
    /* Allocates memory */
    if( NULL == ( ptr = ( void * )malloc( MEMDEBUG_ALLOC_SIZE( size ) ) ) )
    {
        memdebug_warning
        (
            "The call to malloc() failed. Reason: %s",
            file,
            line,
            func,
            strerror( errno )
        );
    }
    else
    {
        ptr = ( void * )( ( memdebug_fence * )ptr + 1 );
        
        /* Creates a new memory record object for the allocated area */
        memdebug_new_object( ptr, size, file, line, func, MEMDEBUG_ALLOC_TYPE_MALLOC );
    }
    
    /* Returns the address of the allocated area */
    return ptr;
}

/**
 * Allocates some memory
 * 
 * @param   size_t          The memory size to allocate
 * @param   const char *    The file in which the call was made
 * @param   const int       The line of the file in which the call was made
 * @param   const char *    The name of the function in which the call was made
 * @return  void *          A pointer to the allocated memory area
 */
void * memdebug_valloc( size_t size, const char * file, const int line, const char * func )
{
    void * ptr;
    
    /* Allocates memory */
    if( NULL == ( ptr = ( void * )valloc( MEMDEBUG_ALLOC_SIZE( size ) ) ) )
    {
        memdebug_warning
        (
            "The call to valloc() failed. Reason: %s",
            file,
            line,
            func,
            strerror( errno )
        );
    }
    else
    {
        ptr = ( void * )( ( memdebug_fence * )ptr + 1 );
        
        /* Creates a new memory record object for the allocated area */
        memdebug_new_object( ptr, size, file, line, func, MEMDEBUG_ALLOC_TYPE_VALLOC );
    }
    
    /* Returns the address of the allocated area */
    return ptr;
}

/**
 * Allocates some memory
 * 
 * @param   size_t          The number of time to allocate the memory size
 * @param   size_t          The memory size to allocate
 * @param   const char *    The file in which the call was made
 * @param   const int       The line of the file in which the call was made
 * @param   const char *    The name of the function in which the call was made
 * @return  void *          A pointer to the allocated memory area
 */
void * memdebug_calloc( size_t size1, size_t size2, const char * file, const int line, const char * func )
{
    void * ptr;
    
    /* Allocates memory */
    /* We don't use calloc as we want to add room for the fence */
    if( NULL == ( ptr = ( void * )malloc( MEMDEBUG_ALLOC_SIZE( size1 * size2 ) ) ) )
    {
        memdebug_warning
        (
            "The call to calloc() failed. Reason: %s",
            file,
            line,
            func,
            strerror( errno )
        );
    }
    else
    {
        ptr = ( void * )( ( memdebug_fence * )ptr + 1 );
        
        /* Sets all bits to 0 */
        memset( ptr, 0, size1 * size2 );
        
        /* Creates a new memory record object for the allocated area */
        memdebug_new_object( ptr, size1 * size2, file, line, func, MEMDEBUG_ALLOC_TYPE_CALLOC );
    }
    
    /* Returns the address of the allocated area */
    return ptr;
}

/**
 * Reallocates a memory area
 * 
 * @param   void *          The address of the original memory area
 * @param   size_t          The new memory size to allocate
 * @param   const char *    The file in which the call was made
 * @param   const int       The line of the file in which the call was made
 * @param   const char *    The name of the function in which the call was made
 * @return  void *          A pointer to the reallocated memory area
 */
void * memdebug_realloc( void * ptr, size_t size, const char * file, const int line, const char * func )
{
    void * ptr_new;
    
    /* Rellocates memory */
    if( NULL == ( ptr_new = ( void * )realloc( ptr, MEMDEBUG_ALLOC_SIZE( size ) ) ) )
    {
        memdebug_warning
        (
            "The call to realloc() failed. Reason: %s",
            file,
            line,
            func,
            strerror( errno )
        );
        return ptr_new;
    }
    else
    {
        ptr_new = ( void * )( ( memdebug_fence * )ptr_new + 1 );
        
        /* Updates the memory record object */
        memdebug_update_object( ptr, ptr_new, size, file, line, func, MEMDEBUG_ALLOC_TYPE_REALLOC );
    }
    
    /* Returns the address of the reallocated area */
    return ptr_new;
}

/**
 * Frees a memory area
 * 
 * @param   void *          The address of the memory area to free
 * @param   const char *    The file in which the call was made
 * @param   const int       The line of the file in which the call was made
 * @param   const char *    The name of the function in which the call was made
 * @return  void
 */
void memdebug_free( void * ptr, const char * file, const int line, const char * func )
{
    /* Marks the object as freed */
    memdebug_free_object( ptr, file, line, func );
    
    /* Frees the memory area */
    free( ( memdebug_fence * )ptr - 1 );
}

/* Checks if the alloca function is available */
#ifdef _ALLOCA_H_

/* Checks if we are using GCC 3 or greater */
#if defined( __GNUC__ ) && __GNUC__ >= 3

/**
 * Allocates some memory in the stack
 * 
 * @param   size_t          The memory size to allocate
 * @param   const char *    The file in which the call was made
 * @param   const int       The line of the file in which the call was made
 * @param   const char *    The name of the function in which the call was made
 * @return  void *          A pointer to the allocated memory area
 */
void * memdebug_builtin_alloca( size_t size, const char * file, const int line, const char * func )
{
    void * ptr;
    
    /* Allocates memory */
    if( NULL == ( ptr = ( void * )__builtin_alloca( MEMDEBUG_ALLOC_SIZE( size ) ) ) )
    {
        memdebug_warning
        (
            "The call to __builtin_alloca() failed. Reason: %s",
            file,
            line,
            func,
            strerror( errno )
        );
    }
    else
    {
        ptr = ( void * )( ( memdebug_fence * )ptr + 1 );
        
        /* Creates a new memory record object for the allocated area */
        memdebug_new_object( ptr, size, file, line, func, MEMDEBUG_ALLOC_TYPE_ALLOCA_BUILTIN );
    }
    
    /* Returns the address of the allocated area */
    return ptr;
}

#else 

/**
 * Allocates some memory in the stack
 * 
 * @param   size_t          The memory size to allocate
 * @param   const char *    The file in which the call was made
 * @param   const int       The line of the file in which the call was made
 * @param   const char *    The name of the function in which the call was made
 * @return  void *          A pointer to the allocated memory area
 */
void * memdebug_alloca( size_t size, const char * file, const int line, const char * func )
{
    void * ptr;
    
    /* Allocates memory */
    if( NULL == ( ptr = ( void * )alloca( MEMDEBUG_ALLOC_SIZE( size ) ) ) )
    {
        memdebug_warning
        (
            "The call to alloca() failed. Reason: %s",
            file,
            line,
            func,
            strerror( errno )
        );
    }
    else
    {
        ptr = ( void * )( ( memdebug_fence * )ptr + 1 );
        
        /* Creates a new memory record object for the allocated area */
        memdebug_new_object( ptr, size, file, line, func, MEMDEBUG_ALLOC_TYPE_ALLOCA );
    }
    
    /* Returns the address of the allocated area */
    return ptr;
}

#endif
#endif

/* Checks if the Objective-C garbage collector is present */
#if defined( OBJC_WITH_GC ) && OBJC_WITH_GC

/**
 * Allocates some memory
 * 
 * @param   size_t          The memory size to allocate
 * @param   const char *    The file in which the call was made
 * @param   const int       The line of the file in which the call was made
 * @param   const char *    The name of the function in which the call was made
 * @return  void *          A pointer to the allocated memory area
 */
void * memdebug_gc_malloc( size_t size, const char * file, const int line, const char * func )
{
    void * ptr;
    
    /* Allocates memory */
    if( NULL == ( ptr = ( void * )GC_malloc( MEMDEBUG_ALLOC_SIZE( size ) ) ) )
    {
        memdebug_warning
        (
            "The call to GC_malloc() failed. Reason: %s",
            file,
            line,
            func,
            strerror( errno )
        );
    }
    else
    {
        ptr = ( void * )( ( memdebug_fence * )ptr + 1 );
        
        /* Creates a new memory record object for the allocated area */
        memdebug_new_object( ptr, size, file, line, func, MEMDEBUG_ALLOC_TYPE_OBJC_GC_MALLOC );
    }
    
    /* Returns the address of the allocated area */
    return ptr;
}

/**
 * Allocates some memory
 * 
 * @param   size_t          The memory size to allocate
 * @param   const char *    The file in which the call was made
 * @param   const int       The line of the file in which the call was made
 * @param   const char *    The name of the function in which the call was made
 * @return  void *          A pointer to the allocated memory area
 */
void * memdebug_gc_malloc_atomic( size_t size, const char * file, const int line, const char * func )
{
    void * ptr;
    
    /* Allocates memory */
    if( NULL == ( ptr = ( void * )GC_malloc_atomic( MEMDEBUG_ALLOC_SIZE( size ) ) ) )
    {
        memdebug_warning
        (
            "The call to GC_malloc_atomic() failed. Reason: %s",
            file,
            line,
            func,
            strerror( errno )
        );
    }
    else
    {
        ptr = ( void * )( ( memdebug_fence * )ptr + 1 );
        
        /* Creates a new memory record object for the allocated area */
        memdebug_new_object( ptr, size, file, line, func, MEMDEBUG_ALLOC_TYPE_OBJC_GC_MALLOC_ATOMIC );
    }
    
    /* Returns the address of the allocated area */
    return ptr;
}

/**
 * Allocates some memory
 * 
 * @param   size_t          The number of time to allocate the memory size
 * @param   size_t          The memory size to allocate
 * @param   const char *    The file in which the call was made
 * @param   const int       The line of the file in which the call was made
 * @param   const char *    The name of the function in which the call was made
 * @return  void *          A pointer to the allocated memory area
 */
void * memdebug_gc_calloc( size_t size1, size_t size2, const char * file, const int line, const char * func )
{
    void * ptr;
    
    /* Allocates memory */
    /* We don't use GC_calloc as we want to add room for the fence */
    if( NULL == ( ptr = ( void * )GC_malloc( MEMDEBUG_ALLOC_SIZE( size1 * size2 ) ) ) )
    {
        memdebug_warning
        (
            "The call to GC_calloc() failed. Reason: %s",
            file,
            line,
            func,
            strerror( errno )
        );
    }
    else
    {
        ptr = ( void * )( ( memdebug_fence * )ptr + 1 );
        
        /* Sets all bits to 0 */
        memset( ptr, 0, size1 * size2 );
        
        /* Creates a new memory record object for the allocated area */
        memdebug_new_object( ptr, size1 * size2, file, line, func, MEMDEBUG_ALLOC_TYPE_OBJC_GC_CALLOC );
    }
    
    /* Returns the address of the allocated area */
    return ptr;
}

/**
 * Reallocates a memory area
 * 
 * @param   void *          The address of the original memory area
 * @param   size_t          The new memory size to allocate
 * @param   const char *    The file in which the call was made
 * @param   const int       The line of the file in which the call was made
 * @param   const char *    The name of the function in which the call was made
 * @return  void *          A pointer to the reallocated memory area
 */
void * memdebug_gc_realloc( void * ptr, size_t size, const char * file, const int line, const char * func )
{
    void * ptr_new;
    
    /* Rellocates memory */
    if( NULL == ( ptr_new = ( void * )GC_realloc( ptr, MEMDEBUG_ALLOC_SIZE( size ) ) ) )
    {
        memdebug_warning
        (
            "The call to GC_realloc() failed. Reason: %s",
            file,
            line,
            func,
            strerror( errno )
        );
        return ptr_new;
    }
    else
    {
        ptr_new = ( void * )( ( memdebug_fence * )ptr_new + 1 );
        
        /* Updates the memory record object */
        memdebug_update_object( ptr, ptr_new, size, file, line, func, MEMDEBUG_ALLOC_TYPE_OBJC_GC_REALLOC );
    }
    
    /* Returns the address of the reallocated area */
    return ptr_new;
}

#endif

#ifdef _MALLOC_MALLOC_H_

void * memdebug_malloc_zone_malloc( malloc_zone_t * zone, size_t size, const char * file, const int line, const char * func )
{
    void * ptr;
    
    /* Allocates memory */
    if( NULL == ( ptr = ( void * )malloc_zone_malloc( zone, MEMDEBUG_ALLOC_SIZE( size ) ) ) )
    {
        memdebug_warning
        (
            "The call to malloc_zone_malloc() failed. Reason: %s",
            file,
            line,
            func,
            strerror( errno )
        );
    }
    else
    {
        ptr = ( void * )( ( memdebug_fence * )ptr + 1 );
        
        /* Creates a new memory record object for the allocated area */
        memdebug_new_object( ptr, size, file, line, func, MEMDEBUG_ALLOC_TYPE_ZONE_MALLOC );
    }
    
    /* Returns the address of the allocated area */
    return ptr;
}

void * memdebug_malloc_zone_calloc( malloc_zone_t * zone, size_t size1, size_t size2, const char * file, const int line, const char * func )
{
    void * ptr;
    
    /* Allocates memory */
    /* We don't use malloc_zone_calloc as we want to add room for the fence */
    if( NULL == ( ptr = ( void * )malloc_zone_malloc( zone, MEMDEBUG_ALLOC_SIZE( size1 * size2 ) ) ) )
    {
        memdebug_warning
        (
            "The call to malloc_zone_calloc() failed. Reason: %s",
            file,
            line,
            func,
            strerror( errno )
        );
    }
    else
    {
        ptr = ( void * )( ( memdebug_fence * )ptr + 1 );
        
        /* Sets all bits to 0 */
        memset( ptr, 0, size1 * size2 );
        
        /* Creates a new memory record object for the allocated area */
        memdebug_new_object( ptr, size1 * size2, file, line, func, MEMDEBUG_ALLOC_TYPE_ZONE_CALLOC );
    }
    
    /* Returns the address of the allocated area */
    return ptr;
}

void * memdebug_malloc_zone_valloc( malloc_zone_t * zone, size_t size, const char * file, const int line, const char * func )
{
    void * ptr;
    
    /* Allocates memory */
    if( NULL == ( ptr = ( void * )malloc_zone_valloc( zone, MEMDEBUG_ALLOC_SIZE( size ) ) ) )
    {
        memdebug_warning
        (
            "The call to malloc_zone_valloc() failed. Reason: %s",
            file,
            line,
            func,
            strerror( errno )
        );
    }
    else
    {
        ptr = ( void * )( ( memdebug_fence * )ptr + 1 );
        
        /* Creates a new memory record object for the allocated area */
        memdebug_new_object( ptr, size, file, line, func, MEMDEBUG_ALLOC_TYPE_ZONE_VALLOC );
    }
    
    /* Returns the address of the allocated area */
    return ptr;
}

void memdebug_malloc_zone_free( malloc_zone_t * zone, void * ptr, const char * file, const int line, const char * func )
{
    /* Marks the object as freed */
    memdebug_free_object( ptr, file, line, func );
    
    /* Frees the memory area */
    malloc_zone_free( zone, ( memdebug_fence * )ptr - 1 );
}

void * memdebug_malloc_zone_realloc( malloc_zone_t * zone, void * ptr, size_t size, const char * file, const int line, const char * func )
{
    void * ptr_new;
    
    /* Rellocates memory */
    if( NULL == ( ptr_new = ( void * )malloc_zone_realloc( zone, ptr, MEMDEBUG_ALLOC_SIZE( size ) ) ) )
    {
        memdebug_warning
        (
            "The call to malloc_zone_realloc() failed. Reason: %s",
            file,
            line,
            func,
            strerror( errno )
        );
        return ptr_new;
    }
    else
    {
        ptr_new = ( void * )( ( memdebug_fence * )ptr_new + 1 );
        
        /* Updates the memory record object */
        memdebug_update_object( ptr, ptr_new, size, file, line, func, MEMDEBUG_ALLOC_TYPE_ZONE_REALLOC );
    }
    
    /* Returns the address of the reallocated area */
    return ptr_new;
}

#endif

/**
 * Displays an hexadecimal and ASCII representation of a memory record object
 * 
 * @param   struct memdebug_object *    The memory record object
 * @return  void
 */
static void memdebug_dump( struct memdebug_object * object )
{
    long unsigned int i;
    long unsigned int j;
    unsigned char * read_ptr;
    unsigned char c;
    
    /* Gets a pointer to read the memory area of the object */
    read_ptr = ( unsigned char * )object->ptr;
    
    /* Reads the memory area */
    for( i = 0; i < object->size; i += MEMDEBUG_DUMP_BYTES )
    {
        /* Byte offset */
        printf( "#   %010lu: ", i );
        
        /* Prints the hexadecimal representation */
        for( j = i; j < i + MEMDEBUG_DUMP_BYTES; j++ )
        {
            /* Checks if we have data to display */
            if( j < object->size )
            {
                /* Prints the hexadecimal data */
                printf( "%02X ", ( unsigned char )read_ptr[ j ] );
            }
            else
            {
                /* No data to display - Pad */
                printf( "   " );
            }
        }
        
        /* Separator */
        printf( "| " );
        
        /* Prints the ASCII representation */
        for( j = i; j < i + MEMDEBUG_DUMP_BYTES; j++ )
        {
            /* Reset */
            c = 0;
            
            /* Checks if we have data to display */
            if( j < object->size )
            {
                /* Gets the current character */
                c = ( unsigned char )read_ptr[ j ];
                
                /* Checks if we can print the character */
                if( ( ( ( c & 0x80 ) == 0 ) && isprint( ( int )c ) ) )
                {
                    /* Prints the current character */
                    printf( "%c", c );
                    
                }
                else
                {
                    /* Unprintable character */
                    printf( "." );
                }
            }
        }
        
        /* New line for the next offset */
        printf( "\n" );
    }
}

/* Checks if we can have a backtrace */
#ifdef MEMDEBUG_HAVE_EXECINFO_H

/**
 * Displays the backtrace (function call stack)
 * 
 * @return  void
 */
void memdebug_backtrace( unsigned int skip_levels )
{
    size_t size;
    size_t i;
    size_t j;
    void ** trace;
    char ** symbols;
    char * symbol;
    unsigned long int frame_num;
    
    /* Allocates enough memory for the backtrace informations */
    if( NULL == ( trace = calloc( MEMDEBUG_BACKTRACE_SIZE + skip_levels, sizeof( void * ) ) ) )
    {
        memdebug_fatal
        (
            "MEMDebug error: cannot allocate memory for the backtrace. Reason: %s\n",
            strerror( errno )
        );
    }
    
    /* Gets the backtrace */
    size    = backtrace( trace, MEMDEBUG_BACKTRACE_SIZE + skip_levels );
    
    /* Gets the symbols for the backtrace */
    symbols = backtrace_symbols( trace, size );
    
    /* Header */
    printf
    (
        MEMDEBUG_HR
        "# MEMDebug - Backtrace\n"
        MEMDEBUG_REV
        MEMDEBUG_HR
        "# \n"
    );
    
    /* Number of stack frames */
    printf
    (
        "# Displaying %lu stack frames:\n"
        "# \n",
        ( unsigned long int )size - skip_levels
    );
    
    /* Process each frame */
    for( i = skip_levels; i < size; i++ )
    {
        /* Current symbol */
        symbol = symbols[ i ];
        
        /* Process each character of the symbol string */
        for( j = 0; j < strlen( symbol ); j++ )
        {
            /* Checks if the frame number was passed, as we are going to rewrite it */
            if( symbol[ j ] != ' ' && ( symbol[ j ] < 48 || symbol[ j ] > 57 ) )
            {
                break;
            }
            
            /* Still on the frame number - Increments the pointer so we don't print it */
            symbol++;
        }
        
        /* Frame number of the symbol */
        frame_num = ( unsigned long int )i - skip_levels;
        
        /* Checks the frame number, in order to get a clean outpout */
        if( frame_num < 10 )
        {
            printf
            (
                "#     %lu:    %s\n",
                frame_num,
                ++symbol
            );
        }
        else if( frame_num < 100 )
        {
            printf
            (
                "#     %lu:   %s\n",
                frame_num,
                ++symbol
            );
        }
        else if( frame_num < 1000 )
        {
            printf
            (
                "#     %lu:  %s\n",
                frame_num,
                ++symbol
            );
        }
        else if( frame_num < 10000 )
        {
            printf
            (
                "#     %lu: %s\n",
                frame_num,
                ++symbol
            );
        }
        else
        {
            printf
            (
                "#     %lu:%s\n",
                frame_num,
                ++symbol
            );
        }
    }
    
    /* Horizontal ruler */
    printf
    (
        "# \n"
        MEMDEBUG_HR
    );
    
    /* Frees the allocated memory for the backtrace and the backtrace symbols */
    free( trace );
    free( symbols );
}

#endif

/**
 * Asks for a debug command
 * 
 * @return void
 */
static void memdebug_ask_debug_cmd( void )
{
    char c;
    static unsigned int skip_levels = 3;
    
    printf
    (
       "# \n"
       "# Your choices are:\n"
       "# \n"
       "#     - c : Default: continue the program execution\n"
       "#     - q : Abort the program execution\n"
       "#     - s : Display the status of the memory allocations\n"
   );
    
    /* Checks if we can have a backtrace */
    #ifdef MEMDEBUG_HAVE_EXECINFO_H
    printf
    (
        "#     - t : Display the backtrace (function call stack)\n"
    );
    #endif
    
    printf
    (
        "#     - p : Display all memory records (active and free)\n"
        "#     - a : Display only the active memory records\n"
        "#     - f : Display only the freed memory records\n"
        "# \n"
        MEMDEBUG_HR
        "\n"
        "Choice: "
    );
    
    /* Reads a character from STDIN to know what to do */
    fflush( stdin );
    c = getchar();
    
    /* Checks if the user choosed a command */
    if( c == '\n' )
    {
        /* Default - continues the program execution */
        return;
    }
    
    /* Don't not about the next characters */
    while( getchar() != '\n' ) {}
    
    /* Checks if the user answer */
    if( c == 's' || c == 'S' )
    {
        /* Prints the memory allocation status */
        printf( "\n" );
        memdebug_print_status();
        
    }
    
    /* Checks if we can have a backtrace */
    #ifdef MEMDEBUG_HAVE_EXECINFO_H
    
    else if( c == 't' || c == 'T' )
    {
        /* Prints the function call stack */
        printf( "\n" );
        memdebug_backtrace( skip_levels );
    }
    
    #endif
    
    else if( c == 'p' || c == 'P' )
    {
        /* Prints all allocated objects */
        printf( "\n" );
        memdebug_print_objects();
    }
    else if( c == 'a' || c == 'A' )
    {
        /* Prints all active objects */
        printf( "\n" );
        memdebug_print_active();
    }
    else if( c == 'f' || c == 'F' )
    {
        /* Prints all freed objects */
        printf( "\n" );
        memdebug_print_free();
    }
    else if( c == 'q' || c == 'Q' )
    {
        /* Aborts the program execution */
        printf
        (
            "\n"
            "Program execution stopped\n"
        );
        exit( EXIT_FAILURE );
    }
    else
    {
        /* Default - continues the program execution */
        return;
    }
    
    skip_levels++;
    
    /* Asks for a command again */
    memdebug_ask_debug_cmd();
}

/**
 * Handles signals (SIGSEGV and SIGBUS)
 * 
 * @param   int     The ID of the signal
 * @return  void
 */
static void memdebug_sig_handler( int id )
{
    /* Checks the signal ID */
    if( id == SIGSEGV || id == SIGBUS )
    {
        if( id == SIGSEGV )
        {
            /* Segmentation fault */
            printf
            (
                MEMDEBUG_HR
                "# MEMDebug: SIGSEGV\n"
                MEMDEBUG_REV
                MEMDEBUG_HR
                "# \n"
                "# A segmentation fault was detected.\n"
            );
        }
        else
        {
            /* Bus error */
            printf
            (
                MEMDEBUG_HR
                "# MEMDebug: SIGBUS\n"
                MEMDEBUG_REV
                MEMDEBUG_HR
                "# \n"
                "# A bus error was detected.\n"
                "Choice: "
            );
        }
        
        /* Asks for a debug command */
        memdebug_ask_debug_cmd();
        
        /* Checks the signal ID */
        if( id == SIGSEGV )
        {
            /* Segmentation fault */
            printf
            (
                "\n"
                "SIGSEGV - Segmentation fault\n"
                "Program execution stopped\n"
            );
        }
        else
        {
            /* Bus error */
            printf
            (
                "\n"
                "SIGBUS - Bus error\n"
                "Program execution stopped\n"
            );
        }
        
        /* Aborts the program execution */
        exit( EXIT_FAILURE );
    }
}

/**
 * Issues a warning
 * 
 * @param   const char *    The warning message
 * @param   const char *    The file concerned by the warning
 * @param   const int       The line number concerned by the warning
 * @param   const char *    The function concerned by the warning
 * @param   ...             The parameters for the warning message, if any
 * @return  void
 */
static void memdebug_warning( const char * str, const char * file, const int line, const char * func, ... )
{
    va_list ap;
    
    /* Gets the variable arguments */
    va_start( ap, func );
    
    /* Issues the warning message */
    printf
    (
        MEMDEBUG_HR
        "# MEMDebug: WARNING\n"
        MEMDEBUG_REV
        MEMDEBUG_HR
        "# \n"
        "# "
    );
    vprintf( str, ap );
    printf
    (
        "\n"
        "# \n"
        "# Function:    %s()\n"
        "# File:        %s\n"
        "# Line:        %i\n",
        func,
        file,
        line
    );
    
    /* Cleanup */
    va_end( ap );
    
    /* Asks for a debug command */
    memdebug_ask_debug_cmd();
}

/**
 * Prints informations about an allocated object
 * 
 * @param   struct memdebug_object *    The allocated object
 * @return  void
 */
static void memdebug_print_object( struct memdebug_object * object )
{
    /* Pointer address and size */
    printf
    (
        "# \n"
        "# - Address:                 %p\n"
        "# - Size:                    %lu\n",
        object->ptr,
        ( unsigned long int )object->size
    );
    
    /* Checks the allocation type */
    switch( object->alloc_type )
    {
        /* malloc */
        case MEMDEBUG_ALLOC_TYPE_MALLOC:
            
            printf( "# - Allocation type:         malloc\n" );
            break;
            
        /* calloc */
        case MEMDEBUG_ALLOC_TYPE_CALLOC:
            
            printf( "# - Allocation type:         calloc\n" );
            break;
            
        /* realloc */
        case MEMDEBUG_ALLOC_TYPE_REALLOC:
            
            printf( "# - Allocation type:         realloc\n" );
            break;
            
        /* alloca */
        case MEMDEBUG_ALLOC_TYPE_ALLOCA_FUNC:
            
            printf( "# - Allocation type:         alloca\n" );
            break;
            
        /* __builtin_alloca */
        case MEMDEBUG_ALLOC_TYPE_ALLOCA_BUILTIN:
            
            printf( "# - Allocation type:         alloca (built-in)\n" );
            break;
            
        /* gc_malloc */
        case MEMDEBUG_ALLOC_TYPE_OBJC_GC_MALLOC:
            
            printf( "# - Allocation type:         malloc (GC)\n" );
            break;
            
        /* gc_malloc_atomic */
        case MEMDEBUG_ALLOC_TYPE_OBJC_GC_MALLOC_ATOMIC:
            
            printf( "# - Allocation type:         atomic malloc (GC)\n" );
            break;
            
        /* gc_calloc */
        case MEMDEBUG_ALLOC_TYPE_OBJC_GC_CALLOC:
            
            printf( "# - Allocation type:         calloc (GC)\n" );
            break;
            
        /* gc_realloc */
        case MEMDEBUG_ALLOC_TYPE_OBJC_GC_REALLOC:
            
            printf( "# - Allocation type:         realloc (GC)\n" );
            break;
            
        /* malloc_zone_malloc */
        case MEMDEBUG_ALLOC_TYPE_ZONE_MALLOC:
            
            printf( "# - Allocation type:         zone malloc\n" );
            break;
            
        /* malloc_zone_calloc */
        case MEMDEBUG_ALLOC_TYPE_ZONE_CALLOC:
            
            printf( "# - Allocation type:         zone calloc\n" );
            break;
            
        /* malloc_zone_valloc */
        case MEMDEBUG_ALLOC_TYPE_ZONE_VALLOC:
            
            printf( "# - Allocation type:         zone valloc\n" );
            break;
            
        /* malloc_zone_realloc */
        case MEMDEBUG_ALLOC_TYPE_ZONE_REALLOC:
            
            printf( "# - Allocation type:         zone realloc\n" );
            break;
            
        /* Unknown type */
        default:
            
            printf( "# - Allocation type:         <unknown>\n" );
            break;
    }
    
    /* Allocation nformations */
    printf
    (
        "# \n"
        "# - Allocated in function:   %s()"
        #ifdef __GNUC__
        " - %p"
        #endif
        "\n"
        "# - Allocated in file:       %s\n"
        "# - Allocated at line:       %i\n"
        "# \n",
        object->alloc_func,
        #ifdef __GNUC__
        object->alloc_func_addr,
        #endif
        object->alloc_file,
        object->alloc_line
    );
    
    /* Checks if the object was freed */
    if( object->free == MEMDEBUG_TRUE )
    {
        /* Checks if the object is automatically freed */
        if( object->alloc_type & MEMDEBUG_ALLOC_TYPE_ALLOCA || object->alloc_type & MEMDEBUG_ALLOC_TYPE_OBJC_GC )
        {
            /* Free informations not available */
            printf
            (
                "# - Freed:                   auto\n"
                "# \n"
            );
        }
        else
        {
            /* Free informations */
            printf
            (
                "# - Freed:                   yes\n"
                "# - Freed in function:       %s()"
                #ifdef __GNUC__
                " - %p"
                #endif
                "\n"
                "# - Freed in file:           %s\n"
                "# - Freed at line:           %i\n"
                "# \n",
                object->free_func,
                #ifdef __GNUC__
                object->free_func_addr,
                #endif
                object->free_file,
                object->free_line
            );
        }
    }
    else
    {
        /* No free information */
        printf
        (
            "# - Freed:                   no\n"
            "# \n"
            "# - Memory dump:\n"
            "# \n"
        );
        
        /* Dumps the actual data */
        memdebug_dump( object );
        printf( "# \n" );
    }
    
    /* Prints a separator */
    printf
    (
        MEMDEBUG_HR
    );
}

/**
 * Prints the status of the memory allocations
 * 
 * @return  void
 */
void memdebug_print_status( void )
{
    /* Initialization check */
    MEMDEBUG_INIT_CHECK;
    
    /* Prints the allocation status */
    printf
    (
        MEMDEBUG_HR
        "# MEMDebug - Status\n"
        MEMDEBUG_REV
        MEMDEBUG_HR
    );
    printf
    (
        "# \n"
        "# - Total allocated objects:               %lu\n"
        "# - Number of non-freed objects:           %lu\n"
        "# - Number of freed objects:               %lu\n"
        "# - Number of automatically-freed objects: %lu\n",
        memdebug_trace->num_objects,
        memdebug_trace->num_active,
        memdebug_trace->num_free,
        memdebug_trace->num_auto
    );
    printf
    (
        "# \n"
        "# - Total memory:                          %lu\n"
        "# - Active memory:                         %lu\n"
        "# \n"
        MEMDEBUG_HR,
        memdebug_trace->memory_total,
        memdebug_trace->memory_active
    );
}

/**
 * Prints informations about the allocated objects (active and free)
 * 
 * @return  void
 */
void memdebug_print_objects( void )
{
    unsigned long int i;
    
    /* Initialization check */
    MEMDEBUG_INIT_CHECK;
    
    /* Header */
    printf
    (
        MEMDEBUG_HR
        "# MEMDebug - Allocated objects\n"
        MEMDEBUG_REV
        MEMDEBUG_HR
    );
    
    /* Checks if objects were allocated */
    if( memdebug_trace->num_objects == 0 )
    {
        /* No allocated objects */
        printf
        (
            "# No objects were allocated\n"
            MEMDEBUG_HR
        );
    }
    else
    {
        /* Process each allocated object */
        for( i = 0; i < memdebug_trace->num_objects; i++ )
        {
            /* Prints information about the current object */
            printf
            (
                "# \n"
                "# - Memory record:           #%lu\n",
                i + 1
            );
            memdebug_print_object( &memdebug_trace->objects[ i ] );
        }
    }
}

/**
 * Prints informations about the freed objects
 * 
 * @return  void
 */
void memdebug_print_free( void )
{
    unsigned long int i;
    
    /* Initialization check */
    MEMDEBUG_INIT_CHECK;
    
    /* Header */
    printf
    (
        MEMDEBUG_HR
        "# MEMDebug - Freed objects\n"
        MEMDEBUG_REV
        MEMDEBUG_HR
    );
    
    /* Checks if objects were freed */
    if( memdebug_trace->num_free == 0 )
    {
        /* No freed object */
        printf
        (
            "# No objects were freed\n"
            MEMDEBUG_HR
        );
    }
    else
    {
        /* Process each allocated object */
        for( i = 0; i < memdebug_trace->num_objects; i++ )
        {
            /* Checks if the current object was freed */
            if( memdebug_trace->objects[ i ].free == MEMDEBUG_TRUE )
            {
                /* Prints information about the current object */
                printf
                (
                    "# \n"
                    "# - Memory record:           #%lu\n",
                    i + 1
                );
                memdebug_print_object( &memdebug_trace->objects[ i ] );
            }
        }
    }
}

/**
 * Prints informations about the active objects
 * 
 * @return  void
 */
void memdebug_print_active( void )
{
    unsigned long int i;
    
    /* Initialization check */
    MEMDEBUG_INIT_CHECK;
    
    /* Header */
    printf
    (
        MEMDEBUG_HR
        "# MEMDebug - Active objects\n"
        MEMDEBUG_REV
        MEMDEBUG_HR
    );
    
    /* Checks if objects are active */
    if( memdebug_trace->num_active == 0 )
    {
        /* No active object */
        printf
        (
            "# No objects are currently active\n"
            MEMDEBUG_HR
        );
    }
    else
    {
        /* Process each allocated object */
        for( i = 0; i < memdebug_trace->num_objects; i++ )
        { 
            /* Checks if the current object is active */
            if( memdebug_trace->objects[ i ].free == MEMDEBUG_FALSE )
            {
                /* Prints information about the current object */
                printf
                (
                    "# \n"
                    "# - Memory record:           #%lu\n",
                    i + 1
                );
                memdebug_print_object( &memdebug_trace->objects[ i ] );
            }
        }
    }
}

/**
 * Gets the number of allocated objects (active and freed)
 * 
 * @return  unsigned long int   The number of allocated objects
 */
unsigned long int memdebug_num_objects( void )
{
    /* Initialization check */
    MEMDEBUG_INIT_CHECK;
    
    /* Returns the number of allocated objects */
    return memdebug_trace->num_objects;
}

/**
 * Gets the number of freed objects
 * 
 * @return  unsigned long int   The number of freed objects
 */
unsigned long int memdebug_num_free( void )
{
    /* Initialization check */
    MEMDEBUG_INIT_CHECK;
    
    /* Returns the number of freed objects */
    return memdebug_trace->num_free;
}

/**
 * Gets the number of active objects
 * 
 * @return  unsigned long int   The number of active objects
 */
unsigned long int memdebug_num_active( void )
{
    /* Initialization check */
    MEMDEBUG_INIT_CHECK;
    
    /* Returns the number of active objects */
    return memdebug_trace->num_active;
}
