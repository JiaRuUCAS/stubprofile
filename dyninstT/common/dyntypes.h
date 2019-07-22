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

#if !defined(DYNTYPES_H)
#define DYNTYPES_H

#ifndef FILE__
#if defined(__GNUC__)
#define FILE__ ((strrchr(__FILE__, '/') ? : __FILE__ - 1) + 1)
#else
#define FILE__ (strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__)
#endif
#endif


#if defined(__GNUC__)
  #include <functional>
  #define DECLTHROW(x) throw(x)
  //*****************libcxx**********************
  #if defined(_LIBCPP_VERSION)
      #include <unordered_set>
      #include <unordered_map>
      #define dyn_hash_set std::unordered_set
      #define dyn_hash_map std::unordered_map
  //***************** GCC ***********************
   #elif defined (__GLIBCXX__) && (__GLIBCXX__ >= 20080306)
      //**************** GCC >= 4.3.0 ***********
      #include <tr1/unordered_set>
      #include <tr1/unordered_map>
      #define cap_tr1
      #define dyn_hash_set std::tr1::unordered_set
      #define dyn_hash_map std::tr1::unordered_map
   #else
      //**************** GCC < 4.3.0 ************
      #include <ext/hash_map>
      #include <ext/hash_set>
      #include <string>
      #define dyn_hash_set __gnu_cxx::hash_set
      #define dyn_hash_map __gnu_cxx::hash_map    
      namespace Dyninst {
	     unsigned ptrHash(void * addr);
      }
      using namespace __gnu_cxx;
      namespace __gnu_cxx {

		  template<> struct hash<std::string> {
			  hash<char*> h;
			  unsigned operator()(const std::string &s) const 
			  {
				  const char *cstr = s.c_str();
				  return h(cstr);
			  };
		  };

		  template<> struct hash<void *> {
			  unsigned operator()(void *v) const 
			  {
				  return Dyninst::ptrHash(v);
			  };
		  };

	  }
   #endif
#else
   #error Unknown compiler
#endif

// TODO: when should we use thread_local?
#define dyn_tls __thread

namespace Dyninst
{
	typedef unsigned long Address;
    typedef unsigned long Offset;

   	typedef int PID;
   	typedef int PROC_HANDLE;
   	typedef int LWP;
	typedef long THR_ID;

#define NULL_PID     -1
#define NULL_LWP     -1
#define NULL_THR_ID     -1
#ifndef INVALID_HANDLE_VALUE
#define INVALID_HANDLE_VALUE -1
#endif

   int ThrIDToTid(Dyninst::THR_ID id);
}

namespace Dyninst
{
   typedef enum {
      OSNone,
      Linux,
      FreeBSD,
      Windows,
      VxWorks,
      BlueGeneL,
      BlueGeneP,
      BlueGeneQ
   } OSType;
}

#include "dyn_regs.h"
#endif
