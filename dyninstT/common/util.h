/*
 * See the dyninst/COPYRIGHT file for copyright information.
 * 
 * We provide the Paradyn Tools (below described as "Paradyn")
 * on an AS IS basis, and do not warrant its validity or performance.
 * We reserve the right to update, modify, or discontinue this
 * software at any time.  We shall have no obligation to supply such
 * updates or modifications or any other form of support to you.
 * 
 * By your use of Paradyn, you understand and agree that we (or any
 * other person or entity with proprietary rights in Paradyn) are
 * under no obligation to provide either maintenance services,
 * update services, notices of latent defects, or correction of
 * defects for Paradyn.
 * 
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 * 
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */


#if !defined(SYMTAB_EXPORT)
    #define SYMTAB_EXPORT __attribute__((visibility ("default")))
#endif

#if !defined(DYNELF_EXPORT)
    #define DYNELF_EXPORT __attribute__((visibility ("default")))
#endif

#if !defined(COMMON_EXPORT)
    #define COMMON_EXPORT __attribute__((visibility ("default")))
#endif

#if !defined(COMMON_TEMPLATE_EXPORT)
    #define COMMON_TEMPLATE_EXPORT  __attribute__((visibility ("default")))
#endif

#if !defined(INSTRUCTION_EXPORT)
    #define INSTRUCTION_EXPORT __attribute__((visibility ("default")))
#endif

#if !defined(PARSER_EXPORT)
    #define PARSER_EXPORT __attribute__((visibility ("default")))
#endif

#if !defined(PATCHAPI_EXPORT)
    #define PATCHAPI_EXPORT __attribute__((visibility ("default")))
#endif

#if !defined(DATAFLOW_EXPORT)
    #define DATAFLOW_EXPORT __attribute__((visibility ("default")))
#endif

#if !defined(PC_EXPORT)
    #define PC_EXPORT __attribute__((visibility ("default")))
#endif

#if !defined(SW_EXPORT)
    #define SW_EXPORT __attribute__((visibility ("default")))
#endif

#if !defined(INJECTOR_EXPORT)
    #define INJECTOR_EXPORT __attribute__((visibility ("default")))
#endif

#if !defined(SYMEVAL_EXPORT)
    #define SYMEVAL_EXPORT __attribute__((visibility ("default")))
#endif

#if !defined(THROW) && !defined(THROW_SPEC)
#define THROW_SPEC(x) throw (x)
#define THROW throw ()
#endif

#ifndef TLS_VAR
#define TLS_VAR __thread
#endif

#ifndef __UTIL_H__
#define __UTIL_H__

#include <string>
#include "dyntypes.h"

/* GCC 4.7.0 and 4.7.1 broke ABI compatibility between C++11 and C++98
 * code in a MAJOR way. Disallow that combination; other versions of
 * the compiler are fine. 
 */
#if !((__cplusplus >= 201103L) || defined(__GXX_EXPERIMENTAL_CXX0X__))
#if defined(__GLIBCXX__) && (__GLIBCXX__ >= 20120322) && (__GLIBCXX__ < 20120920)
#error "Using GCC 4.7.0 or 4.7.1 with Dyninst requires the -std:c++0x or -std:c++11 flag. Other versions do not."
#endif
#endif


namespace Dyninst {

COMMON_EXPORT unsigned addrHashCommon(const Address &addr);
COMMON_EXPORT unsigned ptrHash(const void * addr);
COMMON_EXPORT unsigned ptrHash(void * addr);

COMMON_EXPORT unsigned addrHash(const Address &addr);
COMMON_EXPORT unsigned addrHash4(const Address &addr);
COMMON_EXPORT unsigned addrHash16(const Address &addr);

COMMON_EXPORT unsigned stringhash(const std::string &s);
COMMON_EXPORT std::string itos(int);
COMMON_EXPORT std::string utos(unsigned);

#define WILDCARD_CHAR '?'
#define MULTIPLE_WILDCARD_CHAR '*'

COMMON_EXPORT bool wildcardEquiv(const std::string &us, const std::string &them, bool checkCase = false );

const char *platform_string();
}

#endif
