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

// $Id: Object.C,v 1.31 2008/11/03 15:19:25 jaw Exp $

#include "symutil.h"
#include "common/Annotatable.h"

#include "Symtab.h"
#include "Module.h"
#include "Region.h"
#include "Collections.h"
#include "annotations.h"
#include "Symbol.h"

#include "Aggregate.h"
#include "Function.h"
#include "Variable.h"

#include "exception.h"
#include "Object.h"

#include <iostream>

using namespace std;
using namespace Dyninst;
using namespace Dyninst::SymtabAPI;

const char *Dyninst::SymtabAPI::supportedLanguages2Str(supportedLanguages s)
{
	switch(s) {
		CASE_RETURN_STR(lang_Unknown);
		CASE_RETURN_STR(lang_Assembly);
		CASE_RETURN_STR(lang_C);
		CASE_RETURN_STR(lang_CPlusPlus);
		CASE_RETURN_STR(lang_GnuCPlusPlus);
		CASE_RETURN_STR(lang_Fortran);
		CASE_RETURN_STR(lang_Fortran_with_pretty_debug);
		CASE_RETURN_STR(lang_CMFortran);
	};
	return "bad_language";
}

bool AObject::needs_function_binding() const 
{
	return false;
}

bool AObject::get_func_binding_table(std::vector<relocationEntry> &) const 
{
	return false;
}

bool AObject::get_func_binding_table_ptr(const std::vector<relocationEntry> *&) const 
{
	return false;
}

bool AObject::addRelocationEntry(relocationEntry &)
{
	return true;
}

char *AObject::mem_image() const
{
	return NULL;
}

/**************************************************
 *
 *  Stream based debuggering output - for regreesion testing.
 *  Dump info on state of object *this....
 *
 **************************************************/

SYMTAB_EXPORT unsigned AObject::nsymbols () const 
{ 
	unsigned n = 0;
	for (dyn_hash_map<std::string, std::vector<Symbol *> >::const_iterator i = symbols_.begin();
					i != symbols_.end(); i++) {
		n += i->second.size();
	}
	return n;
}

SYMTAB_EXPORT bool AObject::get_symbols(string & name, 
	  std::vector<Symbol *> &symbols ) 
{
	if ( symbols_.find(name) == symbols_.end()) {
		return false;
	}

	symbols = symbols_[name];
	return true;
}

SYMTAB_EXPORT char* AObject::code_ptr () const 
{ 
	return code_ptr_; 
}

SYMTAB_EXPORT Offset AObject::code_off () const 
{ 
	return code_off_; 
}

SYMTAB_EXPORT Offset AObject::code_len () const 
{ 
	return code_len_; 
}

SYMTAB_EXPORT char* AObject::data_ptr () const 
{ 
	return data_ptr_; 
}

SYMTAB_EXPORT Offset AObject::data_off () const 
{ 
	return data_off_; 
}

SYMTAB_EXPORT Offset AObject::data_len () const 
{ 
	return data_len_; 
}

SYMTAB_EXPORT bool AObject::is_aout() const 
{
	return is_aout_;  
}

SYMTAB_EXPORT bool AObject::isDynamic() const 
{
	return is_dynamic_;  
}

SYMTAB_EXPORT unsigned AObject::no_of_sections() const 
{ 
	return no_of_sections_; 
}

SYMTAB_EXPORT unsigned AObject::no_of_symbols() const 
{ 
	return no_of_symbols_;  
}

SYMTAB_EXPORT bool AObject::getAllExceptions(
				std::vector<ExceptionBlock *>&excpBlocks) const
{
	for (unsigned i=0;i<catch_addrs_.size();i++)
		excpBlocks.push_back(new ExceptionBlock(catch_addrs_[i]));

	return true;
}

SYMTAB_EXPORT std::vector<Region *> AObject::getAllRegions() const
{
	return regions_;	
}

SYMTAB_EXPORT int AObject::getAddressWidth() const 
{ 
	return addressWidth_nbytes; 
}

SYMTAB_EXPORT bool AObject::have_deferred_parsing(void) const
{ 
	return deferredParse;
}

SYMTAB_EXPORT void * AObject::getErrFunc() const 
{
	return (void *) err_func_; 
}

SYMTAB_EXPORT dyn_hash_map<string, std::vector<Symbol *>> *AObject::getAllSymbols() 
{ 
	return &(symbols_);
}

SYMTAB_EXPORT AObject::~AObject() 
{
	using std::string;
	using std::vector;

	dyn_hash_map<string,vector<Symbol *>>::iterator it = symbols_.begin();
	for( ; it != symbols_.end(); ++it) {
		vector<Symbol *> & v = (*it).second;
		for(unsigned i=0;i<v.size();++i)
			delete v[i];
		v.clear();
	}
}

// explicitly protected
SYMTAB_EXPORT AObject::AObject(
				MappedFile *mf_, void (*err_func)(const char *), Symtab* st) :
		mf(mf_), code_ptr_(0), code_off_(0), code_len_(0),
		data_ptr_(0), data_off_(0), data_len_(0),
		code_vldS_(0), code_vldE_(0),
		data_vldS_(0), data_vldE_(0),
		is_aout_(false), is_dynamic_(false),
		has_error(false), is_static_binary_(false),
		no_of_sections_(0), no_of_symbols_(0),
		deferredParse(false), parsedAllLineInfo(false),
		err_func_(err_func), addressWidth_nbytes(4),
		associated_symtab(st)
{
}

//  a helper routine that selects a language based on information from the symtab
supportedLanguages AObject::pickLanguage(
				string &working_module, char *working_options,
				supportedLanguages working_lang)
{
	supportedLanguages lang = lang_Unknown;
	static int sticky_fortran_modifier_flag = 0;
	// (2) -- check suffixes -- try to keep most common suffixes near the top of the checklist
	string::size_type len = working_module.length();
	if((len>2) && (working_module.substr(len-2,2) == string(".c"))) lang = lang_C;
	else if ((len>2) && (working_module.substr(len-2,2) == string(".C"))) lang = lang_CPlusPlus;
	else if ((len>4) && (working_module.substr(len-4,4) == string(".cpp"))) lang = lang_CPlusPlus;
	else if ((len>2) && (working_module.substr(len-2,2) == string(".F"))) lang = lang_Fortran;
	else if ((len>2) && (working_module.substr(len-2,2) == string(".f"))) lang = lang_Fortran;
	else if ((len>3) && (working_module.substr(len-3,3) == string(".cc"))) lang = lang_C;
	else if ((len>2) && (working_module.substr(len-2,2) == string(".a"))) lang = lang_Assembly; // is this right?
	else if ((len>2) && (working_module.substr(len-2,2) == string(".S"))) lang = lang_Assembly;
	else if ((len>2) && (working_module.substr(len-2,2) == string(".s"))) lang = lang_Assembly;
	else {
		//(3) -- try to use options string -- if we have 'em
		if (working_options) {
			//  NOTE:  a binary is labeled "gcc2_compiled" even if compiled w/g77 -- thus this is
			//  quite inaccurate to make such assumptions
			if (strstr(working_options, "gcc"))
				lang = lang_C;
			else if (strstr(working_options, "g++"))
				lang = lang_CPlusPlus;
		}
	}
	//  This next section tries to determine the version of the debug info generator for a
	//  Sun fortran compiler.  Some leave the underscores on names in the debug info, and some
	//  have the "pretty" names, we need to detect this in order to properly read the debug.
	if (working_lang == lang_Fortran) {
		if (sticky_fortran_modifier_flag) {
			working_lang = lang_Fortran_with_pretty_debug;
		} else if (working_options) {
			char *dbg_gen = NULL;

			if (NULL != (dbg_gen = strstr(working_options, "DBG_GEN="))) {
				// Sun fortran compiler (probably), need to examine version
				char *dbg_gen_ver_maj = dbg_gen + strlen("DBG_GEN=");
				char *next_dot = strchr(dbg_gen_ver_maj, '.');

				if (NULL != next_dot) {
					*next_dot = '\0';  //terminate major version number string

					int ver_maj = atoi(dbg_gen_ver_maj);

					if (ver_maj < 3) {
						working_lang = lang_Fortran_with_pretty_debug;
						sticky_fortran_modifier_flag = 1;
					}
				}
			}
		}
	}
	return lang;
}

const std::string AObject::findModuleForSym(Symbol *sym) {
	return symsToModules_[sym];
}

void AObject::clearSymsToMods() {
	symsToModules_.clear();
}

bool AObject::hasError() const
{
  return has_error;
}

void AObject::setTruncateLinePaths(bool)
{
}

bool AObject::getTruncateLinePaths()
{
	return false;
}

void AObject::setModuleForOffset(Offset sym_off, std::string module) {
	auto found_syms = symsByOffset_.find(sym_off);
	if(found_syms == symsByOffset_.end()) return;

	for(auto s = found_syms->second.begin();
			s != found_syms->second.end();
			++s)
	{
		symsToModules_[*s] = module;
	}
}
