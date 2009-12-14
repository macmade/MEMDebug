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

/* Activates MEMDebug */
#define MEMDEBUG 1

/* Includes the MEMDebug header */
#include "libmemdebug.h"

/**
 * C main function
 * 
 * @return  int     The program exit status
 */
int main( void )
{
    int i;
    unsigned char * x;
    unsigned char * y;
    
    /* Allocates some memory */
    x = ( unsigned char * )malloc( 2048 * sizeof( unsigned char ) );
    y = ( unsigned char * )malloc( 256 * sizeof( unsigned char ) );
    
    /* Free some memory */
    free( x );
    
    /* Fills an allocated memory area */
    for( i = 0; i < 256; i++ ) {
        
        y[ i ] = i;
    }
    
    /* This will create a segmentation fault (SIGSEGV) */
    x = 0;
    printf( "%i", x[ 0 ] );
    
    return 0;
}

