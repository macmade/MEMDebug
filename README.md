MEMDebug
========

[![Build Status](https://img.shields.io/travis/macmade/MEMDebug.svg?branch=master&style=flat)](https://travis-ci.org/macmade/MEMDebug)
[![Issues](http://img.shields.io/github/issues/macmade/MEMDebug.svg?style=flat)](https://github.com/macmade/MEMDebug/issues)
![Status](https://img.shields.io/badge/status-inactive-lightgray.svg?style=flat)
![License](https://img.shields.io/badge/license-bsd-brightgreen.svg?style=flat)
[![Contact](https://img.shields.io/badge/contact-@macmade-blue.svg?style=flat)](https://twitter.com/macmade)  
[![Donate-Patreon](https://img.shields.io/badge/donate-patreon-yellow.svg?style=flat)](https://patreon.com/macmade)
[![Donate-Gratipay](https://img.shields.io/badge/donate-gratipay-yellow.svg?style=flat)](https://www.gratipay.com/macmade)
[![Donate-Paypal](https://img.shields.io/badge/donate-paypal-yellow.svg?style=flat)](https://paypal.me/xslabs)

About
-----

MEMDebug is a C library allowing to trace, inspect and debug the dynamic memory allocations inside a C program.

### Overview

Amongst the main reasons why a C program may crash is memory management.  
That kind of errors can be hard to track, depending on the application's logic. It can come from a pointer deallocated twice, a buffer overflow, a segmentation fault, a bus error, etc.

MEMDebug is C library that can be linked to a C program to provide assistance with memory debugging.  
It can detect buffer overflows, double frees, segmentation faults, bus errors, and will give detailed informations about the error that occured and the current memory layout.

MEMDebug is currently compatible with the following memory allocation functions:

 * malloc
 * valloc
 * calloc
 * realloc
 * free
 * alloca
 * GC_malloc
 * GC_malloc_atomic
 * GC_calloc
 * GC_realloc
 * malloc_zone_malloc
 * malloc_zone_valloc
 * malloc_zone_calloc
 * malloc_zone_realloc
 * malloc_zone_free

Documentation
-------------

### 1. Technical details

MEMDebug uses C macros to replace the C memory allocation functions with its own functions.  
This allows MEMDebug to track the memory allocations made from a C program.

In other words, MEMDebug intercepts calls like malloc() and re-route them to a specific function, that will effectively call the malloc() function, so you program will run as usual.

It will allocate a little more memory than asked, so it can create a specific structure, with informations about the memory block.  
It will also put some specific markers just before and after the memory block, so it can detect buffer overflows.

### 2. Usage

#### 2.1 Example

An example program is provided with MEMDebug.

That very simple C program will allocate some memory, using various memory allocation functions (depending on the platform).  
It will then creates a buffer overflow and a segmentation fault, so you can see how MEMDebug handles that kind of error.

The buffer overflow is created the following way:

    unsigned char * m;
    
    m        = ( unsigned char * )malloc( 256 * sizeof( unsigned char ) );
    m[ 256 ] = 0;
    
    free( m );

As you can see, we are allocating enough contiguous memory to hold 256 chars, and we are writing a value past that memory area, as if we had allocated memory for 257 chars.

And here's how the segmentation fault is generated:

    unsigned char * m;
    
    m = 0;
    
    printf( "%i", m[ 0 ] );

Once built, you can run the example with the following command (always from the MEMDebug directory):

    ./build/bin/memdebug

Here's what you will see:

    #-------------------------------------------------------------------------------------------------------
    # MEMDebug: WARNING
    # 
    # $Revision: 69 $
    # $Date: 2010-04-05 18:51:01 +0200 (Mon, 05 Apr 2010) $
    #-------------------------------------------------------------------------------------------------------
    # 
    # A buffer overflow was detected (pointer address: 0x1001001f8)
    # 
    # Function:    main()
    # File:        ./source/memdebug.c
    # Line:        102
    # 
    # Your choices are:
    # 
    #     - c : Default: continue the program execution
    #     - q : Abort the program execution
    #     - s : Display the status of the memory allocations
    #     - t : Display the backtrace (function call stack)
    #     - p : Display all memory records (active and free)
    #     - a : Display only the active memory records
    #     - f : Display only the freed memory records
    # 
    #-------------------------------------------------------------------------------------------------------

The buffer overflow is detected, and the program's execution is paused.

You can now decide what action to take:

 * Ignore the error, and continue the program's execution
 * Quit the program
 * Display a status of the memory allocations till that point
 * Display a backtrace (aka the functions call stack)
 * Display details about all allocations till that point
 * Display details about the allocations still active till that point
 * Display details about the allocations that were freed till that point

#### 2.2 Memory allocation status

If you choose to display the status of the memory allocations, to following kind of output will be produced:

    #-------------------------------------------------------------------------------------------------------
    # 
    # - Total allocated objects:               5
    # - Number of non-freed objects:           2
    # - Number of freed objects:               2
    # - Number of automatically-freed objects: 1
    # 
    # - Total memory:                          2570
    # - Active memory:                         512
    # 
    #-------------------------------------------------------------------------------------------------------

It will give you the number of allocated objects, the number of allocated objects that has been freed, and the number of allocated objects that are still active.

It will also give you the total memory usage of your application, as well as the current usage.

#### 2.3 Function call backtrace

The functions call stack is only available when using GCC as a compiler.  
It will produce the following output:

    #-------------------------------------------------------------------------------------------------------
    # 
    # Displaying 3 stack frames:
    # 
    #     0:     memdebug                            0x00000001000008ba main + 310
    #     1:     memdebug                            0x000000010000077c start + 52
    #     2:     ???                                 0x0000000000000001 0x0 + 1
    # 
    #-------------------------------------------------------------------------------------------------------

#### 2.4 Memory allocation details

You can also display detailed informations about the allocated memory areas.  
The produced output will be the following:

    #-------------------------------------------------------------------------------------------------------
    # 
    # - Memory record:           #3
    # 
    # - Address:                 0x100802608
    # - Size:                    2048
    # - Allocation type:         malloc
    # 
    # - Allocated in function:   main() - 0x10000077c
    # - Allocated in file:       ./source/memdebug.c
    # - Allocated at line:       86
    # 
    # - Freed:                   yes
    # - Freed in function:       main() - 0x100000874
    # - Freed in file:           ./source/memdebug.c
    # - Freed at line:           91
    # 
    #-------------------------------------------------------------------------------------------------------
    # 
    # - Memory record:           #4
    # 
    # - Address:                 0x1001000e8
    # - Size:                    256
    # - Allocation type:         malloc
    # 
    # - Allocated in function:   main() - 0x10000077c
    # - Allocated in file:       ./source/memdebug.c
    # - Allocated at line:       87
    # 
    # - Freed:                   no
    # 
    # - Memory dump:
    # 
    #   0000000000: 00 01 02 03 04 05 06 07 08 09 0A 0B 0C 0D 0E 0F 10 11 12 13 14 15 16 17 | ........................
    #   0000000024: 18 19 1A 1B 1C 1D 1E 1F 20 21 22 23 24 25 26 27 28 29 2A 2B 2C 2D 2E 2F | ........ !"#$%&'()*+,-./
    #   0000000048: 30 31 32 33 34 35 36 37 38 39 3A 3B 3C 3D 3E 3F 40 41 42 43 44 45 46 47 | 0123456789:;<=>?@ABCDEFG
    #   0000000072: 48 49 4A 4B 4C 4D 4E 4F 50 51 52 53 54 55 56 57 58 59 5A 5B 5C 5D 5E 5F | HIJKLMNOPQRSTUVWXYZ[\]^_
    #   0000000096: 60 61 62 63 64 65 66 67 68 69 6A 6B 6C 6D 6E 6F 70 71 72 73 74 75 76 77 | `abcdefghijklmnopqrstuvw
    #   0000000120: 78 79 7A 7B 7C 7D 7E 7F 80 81 82 83 84 85 86 87 88 89 8A 8B 8C 8D 8E 8F | xyz{|}~.................
    #   0000000144: 90 91 92 93 94 95 96 97 98 99 9A 9B 9C 9D 9E 9F A0 A1 A2 A3 A4 A5 A6 A7 | ........................
    #   0000000168: A8 A9 AA AB AC AD AE AF B0 B1 B2 B3 B4 B5 B6 B7 B8 B9 BA BB BC BD BE BF | ........................
    #   0000000192: C0 C1 C2 C3 C4 C5 C6 C7 C8 C9 CA CB CC CD CE CF D0 D1 D2 D3 D4 D5 D6 D7 | ........................
    #   0000000216: D8 D9 DA DB DC DD DE DF E0 E1 E2 E3 E4 E5 E6 E7 E8 E9 EA EB EC ED EE EF | ........................
    #   0000000240: F0 F1 F2 F3 F4 F5 F6 F7 F8 F9 FA FB FC FD FE FF                         | ................
    # 
    #-------------------------------------------------------------------------------------------------------

For each allocated memory block, you'll be able to see its address, size, and which was the function used to allocate it.  
You will also be able to see in which function the block was allocated, in which file, and at which line.

If the block has been freed, you'll also see where it has been freed (function, file and line).  
Otherwise, you'll get a dump of the memory content.

#### 2.5 Fatal memory errors

Back to the example program, if you choose to continue its execution, the next error will occur.  
This time, it's a segmentation fault.

That kind of error, as well as bus-erros, are normally fatal, meaning the Operating System will kill the process.  
With MEMDebug, you have a last chance to see what happened:

    #-------------------------------------------------------------------------------------------------------
    # MEMDebug: SIGSEGV
    # 
    # $Revision: 69 $
    # $Date: 2010-04-05 18:51:01 +0200 (Mon, 05 Apr 2010) $
    #-------------------------------------------------------------------------------------------------------
    # 
    # A segmentation fault was detected.
    # 
    # Your choices are:
    # 
    #     - c : Default: continue the program execution
    #     - q : Abort the program execution
    #     - s : Display the status of the memory allocations
    #     - t : Display the backtrace (function call stack)
    #     - p : Display all memory records (active and free)
    #     - a : Display only the active memory records
    #     - f : Display only the freed memory records
    # 
    #-------------------------------------------------------------------------------------------------------

As you can see, MEMDebug caught here the segmentation fault, and gives you the same possibilities seen before.

Note that if you decide to continue the program's execution at that point, the OS will probably kill the process.

### 3. Linking with your project

The first step to do in order to use MEMDebug with your project is to include its header file.

    #include "libmemdebug.h"

This is absolutely required, as this header file will declare the macros that will replace the standard memory allocation functions.

You will the need to include the MEMDebug library in your program, either by:

 * compiling the C code with your program
 * linking the C code with the program

The first option will generate an executable by compiling your software's sources with the MEMDebug sources.  
This can be done with the following kind of command:

    gcc -o foo foo.c libmemdebug.c

This will generate an executable named `foo`, by compiling both `foo.c` and `memdebug.c` source files.

You can also decide to create a static library with MEMDebug, and then link that library with your program:

    glibtool --quiet --mode=compile gcc -o libmemdebug.lo -c libmemdebug.c
    glibtool --quiet --mode=link gcc -o libmemdebug.la -c libmemdebug.lo
    gcc -o foo.o -c foo.c
    glibtool --quiet --mode=link gcc -o foo foo.o libmemdebug.la

The first line create an object file from the MEMDebug source file, while the second one creates a library archive file, from the object file.

The program's file is then compiled as object code, and finally linked with the MEMDebug library. This last step creates the final executable.

### 4. Helper functions

MEMDebug includes some extras functions for you to use while developing your C program.  
Please note that those functions are not intended to be used on production.

Here's a list of those functions:

    // Prints the current status of the memory allocations.
    void memdebug_print_status( void );
    
    // Prints all the memory records (active and freed).
    void memdebug_print_objects( void );
    
    // Prints the freed memory records.
    void memdebug_print_free( void );
    
    // Prints the active memory records.
    void memdebug_print_active( void );
    
    // Returns the total number of memory records (active and freed).
    unsigned long int memdebug_num_objects( void );
    
    // Returns the number of freed memory records.
    unsigned long int memdebug_num_free( void );
    
    // Returns the number of active memory records.
    unsigned long int memdebug_num_active( void );

License
-------

MEMDebug is released under the terms of the BSD license.

Repository Infos
----------------

    Owner:			Jean-David Gadina - XS-Labs
    Web:			www.xs-labs.com
    Blog:			www.noxeos.com
    Twitter:		@macmade
    GitHub:			github.com/macmade
    LinkedIn:		ch.linkedin.com/in/macmade/
    StackOverflow:	stackoverflow.com/users/182676/macmade
