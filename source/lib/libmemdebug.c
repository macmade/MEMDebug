/*******************************************************************************
 * Copyright (c) 2009, Jean-David Gadina <macmade@eosgarden.com>
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

/* Checks if we can have a backtrace, using execinfo.h */
#if defined( __APPLE__ )
    
    /* Yes - Mac OS X */
    #include <execinfo.h>
    #define MEMDEBUG_HAVE_EXECINFO_H
    
#elif defined( __GLIBC__ ) && defined( HAVE_EXECINFO_H )
    
    /* GNU LibC */
    #include <execinfo.h>
    #define MEMDEBUG_HAVE_EXECINFO_H
    
#endif

/* Local includes */
#include "libmemdebug.h"

/* Undefines the macros, so we can use the real memory functions */
#undef malloc
#undef calloc
#undef realloc
#undef free

/* Macro for the internal fatal errors (just in case) */
#define MEMDEBUG_FATAL( ... )                                                   \
    fprintf( stderr, __VA_ARGS__ );                                             \
    exit( EXIT_FAILURE );

/* Macro to check if MEMDebug was inited (if not, it will init it) */
#define MEMDEBUG_INIT_CHECK                                                     \
    if( memdebug_inited == FALSE ) {                                            \
        memdebug_init();                                                        \
    }

/* Horizontal rules */
#define MEMDEBUG_HR "#-----------------------------------------------------------------------------------------------------------------\n"

/* The number of bytes for each line of the memory data dump */
#define MEMDEBUG_DUMP_BYTES 24

/* Definition of a boolean type, as usual */
typedef enum{ FALSE, TRUE } memdebug_bool;

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
    char * alloc_file;
    
    /* The line of the file in which the object was allocated */
    int alloc_line;
    
    /* The name of the function in which the object was allocated */
    char * alloc_func;
    
    /* The name of the file in which the object was freed */
    char * free_file;
    
    /* The line of the file in which the object was freed */
    int free_line;
    
    /* The name of the function in which the object was freed */
    char * free_func;
    
    /* Checks if we are using GCC */
    #ifdef __GNUC__
        
        /* The address of the function in which the object was allocated */
        void * alloc_func_addr;
        
        /* The address of the function in which the object was allocated */
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
    
    /* The total memory usage */
    size_t memory_total;
    
    /* The memory usage of active (non-freed) memory records */
    size_t memory_active;
};

/* Prototypes for the internal (private) functions */
static void memdebug_init( void );
static struct memdebug_object * memdebug_new_object( void );
static struct memdebug_object * memdebug_get_object( void * ptr );
static void memdebug_dump( struct memdebug_object * object );
static void memdebug_sig_handler( int id );
static void memdebug_warning( const char * str, const char * file, const int line, const char * func );
static void memdebug_print_object( struct memdebug_object * object );

/* Whether MEMDebug has been inited or not */
static memdebug_bool memdebug_inited;

/* The MEMDebug memory record pool */
static struct memdebug_pool * memdebug_trace;

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
    if( memdebug_inited == TRUE ) {
        
        return;
    }
    
    /* Signal handling */
    sa1.sa_handler = memdebug_sig_handler;
    sa1.sa_flags   = 0;
    sigemptyset( &sa1.sa_mask );
    
    /* Handles segmentation faults ( SIGSEGV ) */
    if( sigaction( SIGSEGV, &sa1, &sa2 ) != 0 ) {
        
        MEMDEBUG_FATAL(
            "MEMDebug error: cannot set a handler for SIGSEGV\n"
        );
    }
    
    /* Handles bus errors ( SIGBUS ) */
    if( sigaction( SIGBUS, &sa1, &sa2 ) != 0 ) {
        
        MEMDEBUG_FATAL(
            "MEMDebug error: cannot set a handler for SIGBUS\n"
        );
    }
    
    /* Allocates the memory record pool structure */
    if( NULL == ( memdebug_trace = ( struct memdebug_pool * )calloc( 1, sizeof( struct memdebug_pool ) ) ) ) {
        
        MEMDEBUG_FATAL(
            "MEMDebug error: cannot initialize the trace pool. Reason: %s\n",
            strerror( errno )
        );
    }
    
    /* Allocates room for memory record objects */
    if( NULL == ( memdebug_trace->objects = ( struct memdebug_object * )calloc( MEMDEBUG_POOLSIZE, sizeof( struct memdebug_object ) ) ) ) {
        
        MEMDEBUG_FATAL(
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
    memdebug_trace->pool_size     = MEMDEBUG_POOLSIZE;
    memdebug_inited               = TRUE;
}

/**
 * Creates a new memory record object in the pool
 * 
 * @return  struct memdebug_object *    The new memory record object
 */
static struct memdebug_object * memdebug_new_object( void )
{
    /* Initialization check */
    MEMDEBUG_INIT_CHECK;
    
    /* Checks there's enough room in the current pool for a new object */
    if( memdebug_trace->pool_size == memdebug_trace->num_objects ) {
        
        /* No, let's reallocate some memory */
        if( NULL == ( memdebug_trace->objects = ( struct memdebug_object * )realloc( memdebug_trace->objects, memdebug_trace->pool_size + MEMDEBUG_POOLSIZE ) ) ) {
            
            MEMDEBUG_FATAL(
                "MEMDebug error: cannot reallocate the MEMDebug pool. Reason: %s\n",
                strerror( errno )
            );
        }
        
        /* Sets the new pool size */
        memdebug_trace->pool_size += MEMDEBUG_POOLSIZE;
    }
    
    /* Increments the counter as we have a new object */
    memdebug_trace->num_objects++;
    memdebug_trace->num_active++;
    
    /* Returns the address of the new object */
    return &memdebug_trace->objects[ memdebug_trace->num_objects - 1 ];    
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
    for( i = 0; i < memdebug_trace->num_objects; i++ ) {
        
        /* Checks if the given pointer is the same as the one in the memory record object */
        if( memdebug_trace->objects[ i ].ptr == ptr ) {
            
            /* Yes, returns the memory record object corresponding to the given pointer */
            return &memdebug_trace->objects[ i ];
        }
    }
    
    /* No memory record object corresponding to the given pointer */
    return NULL;
}

/**
 * Allocates some memory
 * 
 * @param   size_t          The memory size to allocate
 * @param   const char *    The file in which the call was made
 * @param   const int       The line of the file in which the call was made
 * @return  void *          A pointer to the allocated memory area
 */
void * memdebug_malloc( size_t size, const char * file, const int line, const char * func )
{
    void * ptr;
    struct memdebug_object * object;
    
    // Allocates memory
    if( NULL == ( ptr = ( void * )malloc( size ) ) ) {
        
        memdebug_warning(
            "The call to malloc() failed",
            file,
            line,
            func
        );
        return ptr;
    }
    
    /* Creates a new memory record object for the allocated area */
    object             = memdebug_new_object();
    object->ptr        = ptr;
    object->size       = size;
    object->alloc_file = file;
    object->alloc_line = line;
    object->alloc_func = func;
    object->free       = FALSE;
    
    /* Checks if we are using GCC */
    #ifdef __GNUC__
        
        /* Stores the address of the caller function */
        object->alloc_func_addr = __builtin_return_address( 1 );
        
    #endif
    
    /* Updates the memory usage */
    memdebug_trace->memory_total  += size;
    memdebug_trace->memory_active += size;
    
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
 * @return  void *          A pointer to the allocated memory area
 */
void * memdebug_calloc( size_t size1, size_t size2, const char * file, const int line, const char * func )
{
    void * ptr;
    struct memdebug_object * object;
    
    // Allocates memory
    if( NULL == ( ptr = ( void * )calloc( size1, size2 ) ) ) {
        
        return ptr;
    }
    
    /* Creates a new memory record object for the allocated area */
    object             = memdebug_new_object();
    object->ptr        = ptr;
    object->size       = size1 * size2;
    object->alloc_file = file;
    object->alloc_line = line;
    object->alloc_func = func;
    object->free       = FALSE;
    
    /* Checks if we are using GCC */
    #ifdef __GNUC__
        
        /* Stores the address of the caller function */
        object->alloc_func_addr = __builtin_return_address( 1 );
        
    #endif
    
    /* Updates the memory usage */
    memdebug_trace->memory_total  += size1 * size2;
    memdebug_trace->memory_active += size1 * size2;
    
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
 * @return  void *          A pointer to the reallocated memory area
 */
void * memdebug_realloc( void * ptr, size_t size, const char * file, const int line, const char * func )
{
    struct memdebug_object * object;
    
    /* Checks if the address we are trying to reallocate exists in the pool */
    if( NULL == ( object = memdebug_get_object( ptr ) ) ) {
        
        memdebug_warning(
            "Trying to reallocate a non-existing object",
            file,
            line,
            func
        );
    }
    
    /* Checks if the memory area was already freed */
    if( object->free == TRUE ) {
        
        memdebug_warning(
            "Trying to reallocate a freed object",
            file,
            line,
            func
        );
    }
    
    // Rellocates memory
    if( NULL == ( ptr = ( void * )realloc( ptr, size ) ) ) {
        
        return ptr;
    }
    
    /* Udpates the memory record object */
    object->ptr        = ptr;
    object->size       = size;
    object->alloc_file = file;
    object->alloc_line = line;
    object->alloc_func = func;
    object->free       = FALSE;
    
    /* Checks if we are using GCC */
    #ifdef __GNUC__
        
        /* Stores the address of the caller function */
        object->alloc_func_addr = __builtin_return_address( 1 );
        
    #endif
    
    /* Updates the memory usage */
    memdebug_trace->memory_total  += size;
    memdebug_trace->memory_active += size;
    
    /* Returns the address of the reallocated area */
    return ptr;
}

/**
 * Frees a memory area
 * 
 * @param   void *          The address of the memory area to free
 * @param   const char *    The file in which the call was made
 * @param   const int       The line of the file in which the call was made
 * @return  void
 */
void memdebug_free( void * ptr, const char * file, const int line, const char * func )
{
    struct memdebug_object * object;
    
    /* Checks if the address we are trying to free exists in the pool */
    if( NULL == ( object = memdebug_get_object( ptr ) ) ) {
        
        memdebug_warning(
            "Trying to free a non-existing object",
            file,
            line,
            func
        );
    }
    
    /* Checks if the memory area was already freed */
    if( object->free == TRUE ) {
        
        memdebug_warning(
            "Trying to free a freed object",
            file,
            line,
            func
        );
    }
    
    /* Udpates the memory record object */
    object->free      = TRUE;
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
    
    /* Frees the memory area */
    free( ptr );
}

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
    for( i = 0; i < object->size; i += MEMDEBUG_DUMP_BYTES ) {
        
        /* Byte offset */
        printf( "#   %010lu: ", i );
        
        /* Prints the hexadecimal representation */
        for( j = i; j < i + MEMDEBUG_DUMP_BYTES; j++ ) {
            
            /* Checks if we have data to display */
            if( j < object->size ) {
                
                /* Prints the hexadecimal data */
                printf( "%02X ", ( unsigned char )read_ptr[ j ] );
                
            } else {
                
                /* No data to display - Pad */
                printf( "   " );
            }
        }
        
        /* Separator */
        printf( "| " );
        
        /* Prints the ASCII representation */
        for( j = i; j < i + MEMDEBUG_DUMP_BYTES; j++ ) {
            
            /* Reset */
            c = 0;
            
            /* Checks if we have data to display */
            if( j < object->size ) {
                
                /* Gets the current character */
                c = ( unsigned char )read_ptr[ j ];
                
                /* Checks if we can print the character */
                if( ( ( ( c & 0x80 ) == 0 ) && isprint( ( int )c ) ) ) {
                    
                    /* Prints the current character */
                    printf( "%c", c );
                    
                } else {
                    
                    /* Unprintable character */
                    printf( "." );
                }
            }
        }
        
        /* New line for the next offset */
        printf( "\n" );
    }
}

/**
 * Handles signals (SIGSEGV and SIGBUS)
 * 
 * @param   int     The ID of the signal
 * @return  void
 */
static void memdebug_sig_handler( int id )
{
    char c;
    
    /* Checks the signal ID */
    if( id == SIGSEGV || id == SIGBUS ) {
        
        if( id == SIGSEGV ) {
            
            /* Segmentation fault */
            printf(
                MEMDEBUG_HR
                "# MEMDebug: SIGSEGV\n"
                MEMDEBUG_HR
                "# A segmentation fault was detected.\n"
                "# Do you want to display a trace of the memory allocations before exiting? [Y/n]\n"
                MEMDEBUG_HR
                "\n"
                "Choice: "
            );
            
        } else {
            
            /* Bus error */
            printf( 
                MEMDEBUG_HR
                "# MEMDebug: SIGBUS\n"
                MEMDEBUG_HR
                "# A bus error was detected.\n"
                "# Do you want to display a trace of the memory allocations before exiting? [Y/n]\n"
                MEMDEBUG_HR
                "\n"
                "Choice: "
            );
        }
        
        /* Reads a character from STDIN to know what to do */
        fflush( stdin );
        c = getchar();
        
        /* Checks if the user entered 'Y' */
        if( c == 78 || c == 110 ) {
            
            /* Aborts the program execution */
            exit( EXIT_FAILURE );
        }
        
        /* Prints the memory debug */
        printf( "\n" );
        memdebug_print_objects();
        printf( "\n" );
        memdebug_print_status();
        
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
 * @return  void
 */
static void memdebug_warning( const char * str, const char * file, const int line, const char * func )
{
    char c;
    
    /* Issues the warning message */
    printf(
        MEMDEBUG_HR
        "# MEMDebug: WARNING\n"
        MEMDEBUG_HR
        "# %s\n"
        "# \n"
        "# Function:    %s()\n"
        "# File:        %s\n"
        "# Line:        %i\n"
        "# \n"
        "# Do you want to continue [c], abort [a] the program, or display [p] a trace\n"
        "# of the memory allocations? [C/a/p]\n"
        MEMDEBUG_HR
        "\n"
        "Choice: ",
        str,
        func,
        file,
        line
    );
    
    /* Reads a character from STDIN to know what to do */
    fflush( stdin );
    c = getchar();
    
    /* Checks the user answer */
    if( c == 65 || c == 97 ) {
        
        /* Status */
        printf(
            "\n"
            "Aborting the program execution...\n"
            "\n"
        );
        
        /* Aborts the program execution */
        exit( EXIT_FAILURE );
        
    } else if( c == 80 || c == 112 ) {
        
        /* Prints the memory allocations */
        printf( "\n" );
        memdebug_print_objects();
        printf( "\n" );
        memdebug_print_status();
        
        /* Aborts the program execution */
        exit( EXIT_FAILURE );
        
    } else {
        
        /* Nothing - Continues the normal execution */
        printf(
            "\n"
            "Continuing the program execution...\n"
            "\n"
        );
    }
}

/**
 * Prints informations about an allocated object
 * 
 * @param   struct memdebug_object *    The allocated object
 * @return  void
 */
static void memdebug_print_object( struct memdebug_object * object )
{
    /* Common information */
    printf(
        "# - Address:                 %p\n"
        "# - Size:                    %lu\n"
        "# \n"
        "# - Allocated in function:   %s()"
        #ifdef __GNUC__
        " - %lu"
        #endif
        "\n"
        "# - Allocated in file:       %s\n"
        "# - Allocated at line:       %i\n"
        "# \n",
        ( unsigned long int )object->ptr,
        ( unsigned long int )object->size,
        object->alloc_func,
        #ifdef __GNUC__
        ( unsigned long int )object->alloc_func_addr,
        #endif
        object->alloc_file,
        object->alloc_line
    );
    
    /* Checks if the object was freed */
    if( object->free == TRUE ) {
        
        /* Free informations */
        printf(
            "# - Freed:                   yes\n"
            "# - Freed in function:       %s()"
            #ifdef __GNUC__
            " - %lu"
            #endif
            "\n"
            "# - Freed in file:           %s\n"
            "# - Freed at line:           %i\n"
            "# \n",
            object->free_func,
            #ifdef __GNUC__
            ( unsigned long int )object->free_func_addr,
            #endif
            object->free_file,
            object->free_line
        );
        
    } else {
        
        /* No free information */
        printf(
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
    printf(
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
    printf(
        MEMDEBUG_HR
        "# MEMDebug - Status\n"
        MEMDEBUG_HR
        "# \n"
        "# - Total allocated objects:       %lu\n"
        "# - Number of freed objects:       %lu\n"
        "# - Number of non-freed objects:   %lu\n"
        "# \n"
        "# - Total memory:                  %lu\n"
        "# - Active memory:                 %lu\n"
        "# \n"
        MEMDEBUG_HR,
        memdebug_trace->num_objects,
        memdebug_trace->num_free,
        memdebug_trace->num_active,
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
    printf(
        MEMDEBUG_HR
        "# MEMDebug - Allocated objects\n"
        MEMDEBUG_HR
    );
    
    /* Checks if objects were allocated */
    if( memdebug_trace->num_objects == 0 ) {
        
        /* No allocated objects */
        printf(
            "# No objects were allocated\n"
            MEMDEBUG_HR
        );
        
    } else {
        
        /* Process each allocated object */
        for( i = 0; i < memdebug_trace->num_objects; i++ ) {
            
            /* Prints information about the current object */
            printf(
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
    printf(
        MEMDEBUG_HR
        "# MEMDebug - Freed objects\n"
        MEMDEBUG_HR
    );
    
    /* Checks if objects were freed */
    if( memdebug_trace->num_free == 0 ) {
        
        /* No freed object */
        printf(
            "# No objects were freed\n"
            MEMDEBUG_HR
        );
        
    } else {
        
        /* Process each allocated object */
        for( i = 0; i < memdebug_trace->num_objects; i++ ) {
            
            /* Checks if the current object was freed */
            if( memdebug_trace->objects[ i ].free == TRUE ) {
                
                /* Prints information about the current object */
                printf(
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
    printf(
        MEMDEBUG_HR
        "# MEMDebug - Active objects\n"
        MEMDEBUG_HR
    );
    
    /* Checks if objects are active */
    if( memdebug_trace->num_active == 0 ) {
        
        /* No active object */
        printf(
            "# No objects are currently active\n"
            MEMDEBUG_HR
        );
        
    } else {
        
        /* Process each allocated object */
        for( i = 0; i < memdebug_trace->num_objects; i++ ) {
            
            /* Checks if the current object is active */
            if( memdebug_trace->objects[ i ].free == FALSE ) {
                
                /* Prints information about the current object */
                printf(
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
