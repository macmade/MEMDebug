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

#ifndef MEMDEBUG_H
#define MEMDEBUG_H
#pragma once

/* Checks if the function name constant is defined */
#ifndef __func__

/* Checks the C standard version */
#if __STDC_VERSION__ < 199901L

/* Checks if we are using GNU C */
#ifdef __GNUC__

/* Checks the GNU C version */
#if __GNUC__ >= 2

/* Uses the old constant name */
#define __func__ __FUNCTION__

#else

/* Function name cannot be used */
#define __func__ "<unknown>"

#endif
#else

/* Function name cannot be used */
#define __func__ "<unknown>"

#endif
#endif
#endif

/* Checks if MEMDebug must be activated */
#if defined( MEMDEBUG ) && MEMDEBUG
    
/* Redefines the memory functions */
#define malloc( size )          memdebug_malloc( size, __FILE__, __LINE__, __func__ )
#define calloc( size1, size2 )  memdebug_calloc( size1, size2, __FILE__, __LINE__, __func__ )
#define realloc( ptr, size )    memdebug_realloc( ptr, size, __FILE__, __LINE__, __func__ )
#define free( ptr )             memdebug_free( ptr, __FILE__, __LINE__, __func__ )

/* Checks if the alloca function is available */
#ifdef _ALLOCA_H_

/* Checks if we are using GCC 3 or greater */
#if defined( __GNUC__ ) && __GNUC__ >= 3

/* Redefines the built-in alloca function  */
#undef alloca
#define alloca( size )  memdebug_builtin_alloca( size, __FILE__, __LINE__, __func__ )

#else 

/* Redefine the alloca function */
#define alloca( size )  memdebug_alloca( size, __FILE__, __LINE__, __func__ )

#endif
#endif

/* Checks if the Objective-C garbage collector is present */
#if defined( OBJC_WITH_GC ) && OBJC_WITH_GC

/* Redefines the Objective-C garbage collector memory functions */
#define GC_malloc( size )           memdebug_gc_malloc( size, __FILE__, __LINE__, __func__ )
#define GC_malloc_atomic( size )    memdebug_gc_malloc_atomic( size, __FILE__, __LINE__, __func__ )
#define GC_calloc( size1, size2 )   memdebug_gc_calloc( size1, size2, __FILE__, __LINE__, __func__ )
#define GC_realloc( ptr, size )     memdebug_gc_realloc( ptr, size, __FILE__, __LINE__, __func__ )

#endif

#ifdef _MALLOC_MALLOC_H_

#define malloc_zone_malloc( zone, size )            memdebug_malloc_zone_malloc( zone, size, __FILE__, __LINE__, __func__ )
#define malloc_zone_calloc( zone, size1, size2 )    memdebug_malloc_zone_calloc( zone, size1, size2, __FILE__, __LINE__, __func__ )
#define malloc_zone_valloc( zone, size )            memdebug_malloc_zone_valloc( zone, size, __FILE__, __LINE__, __func__ )
#define malloc_zone_free( zone, ptr )               memdebug_malloc_zone_free( zone, ptr, __FILE__, __LINE__, __func__ )
#define malloc_zone_realloc( zone, ptr, size )      memdebug_malloc_zone_realloc( zone, ptr, size, __FILE__, __LINE__, __func__ )

#endif
#endif

/* Defines the original pool size if it's not already defined */
#ifndef MEMDEBUG_POOL_SIZE
#define MEMDEBUG_POOL_SIZE 100    
#endif

/* Defines the bactrace size if it's not already defined */
#ifndef MEMDEBUG_BACKTRACE_SIZE
#define MEMDEBUG_BACKTRACE_SIZE 100    
#endif

/* Prototypes for the standard memory functions */
void * memdebug_malloc( size_t size, const char * file, const int line, const char * func );
void * memdebug_calloc( size_t size1, size_t size2, const char * file, const int line, const char * func );
void * memdebug_realloc( void * ptr, size_t size, const char * file, const int line, const char * func );
void   memdebug_free( void * ptr, const char * file, int line, const char * func );

/* Checks if the alloca function is available */
#ifdef _ALLOCA_H_

/* Checks if we are using GCC 3 or greater */
#if defined( __GNUC__ ) && __GNUC__ >= 3

/* Prototype for the alloca function */
void * memdebug_builtin_alloca( size_t size, const char * file, const int line, const char * func );

#else 

/* Prototype for the alloca function */
void * memdebug_alloca( size_t size, const char * file, const int line, const char * func );

#endif
#endif

/* Checks if the Objective-C garbage collector is present */
#if defined( OBJC_WITH_GC ) && OBJC_WITH_GC

/* Prototypes for the Objective-C garbage collector memory functions */
void * memdebug_gc_malloc( size_t size, const char * file, const int line, const char * func );
void * memdebug_gc_malloc_atomic( size_t size, const char * file, const int line, const char * func );
void * memdebug_gc_calloc( size_t size1, size_t size2, const char * file, const int line, const char * func );
void * memdebug_gc_realloc( void * ptr, size_t size, const char * file, const int line, const char * func );

#endif

#ifdef _MALLOC_MALLOC_H_

void * memdebug_malloc_zone_malloc( malloc_zone_t * zone, size_t size, const char * file, const int line, const char * func );
void * memdebug_malloc_zone_calloc( malloc_zone_t * zone, size_t size1, size_t size2, const char * file, const int line, const char * func );
void * memdebug_malloc_zone_valloc( malloc_zone_t * zone, size_t size, const char * file, const int line, const char * func );
void   memdebug_malloc_zone_free( malloc_zone_t * zone, void * ptr, const char * file, const int line, const char * func );
void * memdebug_malloc_zone_realloc( malloc_zone_t * zone, void * ptr, size_t size, const char * file, const int line, const char * func );

#endif

/* Debug output functions */
void memdebug_print_status( void );
void memdebug_print_objects( void );
void memdebug_print_free( void );
void memdebug_print_active( void );

/* Informational functions */
unsigned long int memdebug_num_objects( void );
unsigned long int memdebug_num_free( void );
unsigned long int memdebug_num_active( void );

#endif /* MEMDEBUG_H */
