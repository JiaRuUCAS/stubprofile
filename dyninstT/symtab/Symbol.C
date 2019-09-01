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
#include "common/headers.h"

#include "symutil.h"
#include "Symbol.h"
#include "annotations.h"
#include "Region.h"
#include "Type.h"
#include "Module.h"
#include "Object.h"
#include "Function.h"

#include <string>
#include <iostream>

using namespace Dyninst;
using namespace SymtabAPI;

std::string Symbol::emptyString("");

bool ____sym_hdr_printed = false;
void Dyninst::SymtabAPI::print_symbols(std::vector< Symbol *>& allsymbols) {
	FILE* fd = stdout;
	Symbol *sym;
	std::string modname;

	if (!____sym_hdr_printed) {
		fprintf(fd, "%-20s  %-15s  %-10s  %5s  SEC  TYP  LN  VIS  INFO\n", 
				"SYMBOL", "MODULE", "ADDR", "SIZE");
		____sym_hdr_printed = true;
	}

	for (unsigned i=0; i<allsymbols.size(); i++) {
		sym = allsymbols[i];
		modname = (sym->getModule() ? sym->getModule()->fileName() : "");

		fprintf(fd, "%-20s  %-15s  0x%08x  %5u  %3u", 
				sym->getMangledName().substr(0,20).c_str(), 
				"", (unsigned)sym->getOffset(),
				(unsigned)sym->getSize(),
				sym->getRegion() ? sym->getRegion()->getRegionNumber() : 0);

		switch (sym->getType()) {
			case Symbol::ST_FUNCTION: fprintf(fd, "  FUN"); break;
			case Symbol::ST_TLS:	  fprintf(fd, "  TLS"); break;
			case Symbol::ST_OBJECT:   fprintf(fd, "  OBJ"); break;
			case Symbol::ST_MODULE:   fprintf(fd, "  MOD"); break;
			case Symbol::ST_SECTION:  fprintf(fd, "  SEC"); break;
			case Symbol::ST_DELETED:  fprintf(fd, "  DEL"); break;
			case Symbol::ST_NOTYPE:   fprintf(fd, "   - "); break;
			default:
			case Symbol::ST_UNKNOWN:  fprintf(fd, "  ???"); break;				 
		}

		switch (sym->getLinkage()) {
			case Symbol::SL_UNKNOWN: fprintf(fd, "  ??"); break;
			case Symbol::SL_GLOBAL:  fprintf(fd, "  GL"); break;
			case Symbol::SL_LOCAL:   fprintf(fd, "  LO"); break;
			case Symbol::SL_WEAK:	 fprintf(fd, "  WK"); break;
			case Symbol::SL_UNIQUE:  fprintf(fd, "  UQ"); break;
		}

		switch (sym->getVisibility()) {
			case Symbol::SV_UNKNOWN:   fprintf(fd, "  ???"); break;
			case Symbol::SV_DEFAULT:   fprintf(fd, "   - "); break;
			case Symbol::SV_INTERNAL:  fprintf(fd, "  INT"); break;
			case Symbol::SV_HIDDEN:	fprintf(fd, "  HID"); break;
			case Symbol::SV_PROTECTED: fprintf(fd, "  PRO"); break;
		}

		fprintf(fd, " ");
		if (sym->isInSymtab())
			fprintf(fd, " STA");
		if (sym->isInDynSymtab())
			fprintf(fd, " DYN");
		if (sym->isAbsolute())
			fprintf(fd, " ABS");
		if (sym->isDebug())
			fprintf(fd, " DBG");

		std::string fileName;
		std::vector<std::string> *vers;

		if (sym->getVersionFileName(fileName))
			fprintf(fd, "  [%s]", fileName.c_str());
		if (sym->getVersions(vers)) {
			fprintf(fd, " {");
			for (unsigned j=0; j < vers->size(); j++) {
				if (j > 0)
					fprintf(fd, ", ");
				fprintf(fd, "%s", (*vers)[j].c_str());
			}
			fprintf(fd, "}");
		}
		fprintf(fd,"\n");
	}
}

void Dyninst::SymtabAPI::print_symbol_map(
				dyn_hash_map<std::string, std::vector<Symbol *>> *symbols)
{
	dyn_hash_map<std::string, std::vector<Symbol *>>::iterator siter =
			symbols->begin();
	int total_syms = 0;

	while (siter != symbols->end()) {
		print_symbols(siter->second);
		total_syms += siter->second.size();
		siter++;
	}
	printf("%d total symbol(s)\n", total_syms);
}

bool Dyninst::SymtabAPI::symbol_compare(const Symbol *s1, const Symbol *s2) 
{
	// select the symbol with the lowest address
	Offset s1_addr = s1->getOffset();
	Offset s2_addr = s2->getOffset();

	if (s1_addr > s2_addr)
		return false;
	if (s1_addr < s2_addr)
		return true;

	// symbols are co-located at the same address
	// select the symbol which is not a function
	if ((s1->getType() != Symbol::ST_FUNCTION)
					&& (s2->getType() == Symbol::ST_FUNCTION))
		return true;
	if ((s2->getType() != Symbol::ST_FUNCTION)
					&& (s1->getType() == Symbol::ST_FUNCTION))
		return false;
	
	// symbols are both functions
	// select the symbol which has GLOBAL linkage
	if ((s1->getLinkage() == Symbol::SL_GLOBAL)
					&& (s2->getLinkage() != Symbol::SL_GLOBAL))
		return true;
	if ((s2->getLinkage() == Symbol::SL_GLOBAL)
					&& (s1->getLinkage() != Symbol::SL_GLOBAL))
		return false;
	
	// neither function is GLOBAL
	// select the symbol which has LOCAL linkage
	if ((s1->getLinkage() == Symbol::SL_LOCAL)
					&& (s2->getLinkage() != Symbol::SL_LOCAL))
		return true;
	if ((s2->getLinkage() == Symbol::SL_LOCAL)
					&& (s1->getLinkage() != Symbol::SL_LOCAL))
		return false;
	
	// both functions are WEAK
	
	// Apparently sort requires a strict weak ordering
	// and fails for equality. our compare
	// function behaviour should be as follows
	// f(x,y) => !f(y,x)
	// f(x,y),f(y,z) => f(x,z)
	// f(x,x) = false. 
	// So return which ever is first in the array. May be that would help.
	return (s1 < s2);
}

const char *Symbol::symbolType2Str(SymbolType t)
{
	switch(t) {
		CASE_RETURN_STR(ST_UNKNOWN);
		CASE_RETURN_STR(ST_FUNCTION);
		CASE_RETURN_STR(ST_OBJECT);
		CASE_RETURN_STR(ST_MODULE);
		CASE_RETURN_STR(ST_SECTION);
		CASE_RETURN_STR(ST_TLS);
		CASE_RETURN_STR(ST_DELETED);
		CASE_RETURN_STR(ST_NOTYPE);
		CASE_RETURN_STR(ST_INDIRECT);
	}
	return "Invalid symbol type";
}

const char *Symbol::symbolLinkage2Str(SymbolLinkage t) 
{
	switch(t) {
		CASE_RETURN_STR(SL_UNKNOWN);
		CASE_RETURN_STR(SL_GLOBAL);
		CASE_RETURN_STR(SL_LOCAL);
		CASE_RETURN_STR(SL_WEAK);
		CASE_RETURN_STR(SL_UNIQUE);
	};

	return "Invalid symbol linkage";
}

const char *Symbol::symbolVisibility2Str(SymbolVisibility t) 
{
	switch(t) {
		CASE_RETURN_STR(SV_UNKNOWN);
		CASE_RETURN_STR(SV_DEFAULT);
		CASE_RETURN_STR(SV_INTERNAL);
		CASE_RETURN_STR(SV_HIDDEN);
		CASE_RETURN_STR(SV_PROTECTED);
	}
	return "invalid symbol visibility";
}

Symtab *Symbol::getSymtab() const {
	return module_ ? module_->exec() : NULL;
}

Symbol::Symbol () :
	module_(NULL),
	type_(ST_NOTYPE),
	internal_type_(0),
	linkage_(SL_UNKNOWN),
	visibility_(SV_UNKNOWN),
	offset_(0),
	ptr_offset_(0),
	localTOC_(0),
	region_(NULL),
	referring_(NULL),
	size_(0),
	isDynamic_(false),
	isAbsolute_(false),
	isDebug_(false),
	aggregate_(NULL),
	mangledName_(emptyString),
	index_(-1),
	strindex_(-1),
	isCommonStorage_(false),
	versionHidden_(false)
{
}

Symbol::Symbol(const std::string& name,
				SymbolType t,
				SymbolLinkage l,
				SymbolVisibility v,
				Offset o,
				Module *module,
				Region *r,
				unsigned s,
				bool d,
				bool a,
				int index,
				int strindex,
				bool cs):
	module_(module),
	type_(t),
	internal_type_(0),
	linkage_(l),
	visibility_(v),
	offset_(o),
	ptr_offset_(0),
	localTOC_(0),
	region_(r),
	referring_(NULL),
	size_(s),
	isDynamic_(d),
	isAbsolute_(a),
	isDebug_(false),
	aggregate_(NULL),
	mangledName_(name),
	index_(index),
	strindex_(strindex),
	isCommonStorage_(cs),
	versionHidden_(false)
{
}

Symbol::~Symbol ()
{
	std::string *sfa_p = NULL;

	if (getAnnotation(sfa_p, SymbolFileNameAnno)) {
		removeAnnotation(SymbolFileNameAnno);
		delete (sfa_p);
	}
}

Symbol *Symbol::magicEmitElfSymbol() {
	// I have no idea why this is the way it is,
	// but emitElf needs it...
	return new Symbol("",		// name
					ST_NOTYPE,	// symbol type
					SL_LOCAL,	// symbol linkage
					SV_DEFAULT,	// symbol visibility
					0,			// offset_
					NULL,		// module
					NULL,		// region
					0,			// size_
					false,		// isDynamic_
					false);		// isAbsolute_
}

bool Symbol::operator==(const Symbol& s) const
{
	//  compare sections by offset, not pointer
	if (!region_ && s.region_) return false;
	if (region_ && !s.region_) return false;
	if (region_) {
		if (region_->getDiskOffset() != s.region_->getDiskOffset())
			return false;
	}

	// compare modules by name, not pointer
	if (!module_ && s.module_) return false;
	if (module_ && !s.module_) return false;
	if (module_) {
		if (module_->fullName() != s.module_->fullName())
			return false;
	}

	return ((type_	== s.type_)
			&& (linkage_ == s.linkage_)
			&& (offset_	== s.offset_)
			&& (size_	== s.size_)
			&& (isDynamic_ == s.isDynamic_)
			&& (isAbsolute_ == s.isAbsolute_)
			&& (isDebug_ == s.isDebug_)
			&& (isCommonStorage_ == s.isCommonStorage_)
			&& (versionHidden_ == s.versionHidden_)
			&& (mangledName_ == s.mangledName_));
}

std::ostream& Dyninst::SymtabAPI::operator<<(ostream &os, const Symbol &s) 
{
	return os << "{"
			<< " mangled=" << s.getMangledName()
			<< " pretty="  << s.getPrettyName()
			<< " module="  << s.module_
			<< " type=" << s.symbolType2Str(s.type_)
			<< " linkage=" << s.symbolLinkage2Str(s.linkage_)
			<< " offset=0x" << hex << s.offset_ << dec
			<< " size=0x" << hex << s.size_ << dec
			<< " ptr_offset=0x" << hex << s.ptr_offset_ << dec
			<< " localTOC=0x" << hex << s.localTOC_ << dec
			<< " isAbs=" << s.isAbsolute_
			<< " isDbg=" << s.isDebug_
			<< " isCommon=" << s.isCommonStorage_
			<< (s.isFunction() ? " [FUNC]" : "")
			<< (s.isVariable() ? " [VAR]" : "")
			<< (s.isInSymtab() ? "[STA]" : "")
			<< (s.isInDynSymtab() ? "[DYN]" : "")
			<< " }";
}

SYMTAB_EXPORT string Symbol::getMangledName() const 
{
	return mangledName_;
}

SYMTAB_EXPORT string Symbol::getPrettyName() const 
{
	std::string working_name = mangledName_;

	// Accoring to Itanium C++ ABI, all mangled names start with _Z
	if (mangledName_.size() < 2 || mangledName_[0] != '_'
					|| mangledName_[1] != 'Z')
		return working_name;

	//Remove extra stabs information
	size_t colon, atat;

	colon = working_name.find(":");
	if(colon != string::npos) {
		working_name = working_name.substr(0, colon);
	}

	atat = working_name.find("@@");
	if(atat != string::npos) {
		working_name = working_name.substr(0, atat);
	}
  
	char *prettyName = NULL;
	
	prettyName = P_cplus_demangle(working_name.c_str(),
					true /* native_comp */, false);
	if (prettyName) {
		working_name = std::string(prettyName);
		// XXX caller-freed
		free(prettyName); 
	}

	return working_name;
}

SYMTAB_EXPORT string Symbol::getTypedName() const 
{
	std::string working_name = mangledName_;

	// Accoring to Itanium C++ ABI, all mangled names start with _Z
	if (mangledName_.size() < 2 || mangledName_[0] != '_'
					|| mangledName_[1] != 'Z')
		return working_name;

	//Remove extra stabs information
	size_t colon;

	colon = working_name.find(":");
	if(colon != string::npos) 
		working_name = working_name.substr(0, colon);

	char *prettyName = NULL;
	
	prettyName = P_cplus_demangle(working_name.c_str(),
					true /* native_comp */, true);
	if (prettyName) {
		working_name = std::string(prettyName);
		// XXX caller-freed
		free(prettyName); 
	}

	return working_name;
}

bool Symbol::setOffset(Offset newOffset)
{
	offset_ = newOffset;
	return true;
}

bool Symbol::setPtrOffset(Offset newOffset)
{
	ptr_offset_ = newOffset;
	return true;
}

bool Symbol::setLocalTOC(Offset toc)
{
	localTOC_ = toc;
	return true;
}

SYMTAB_EXPORT bool Symbol::setModule(Module *mod) 
{
	assert(mod);
	module_ = mod; 
	return true;
}

SYMTAB_EXPORT bool Symbol::isFunction() const
{
	return (getFunction() != NULL);
}

SYMTAB_EXPORT bool Symbol::setFunction(Function *func)
{
	aggregate_ = func;
	return true;
}

SYMTAB_EXPORT Function * Symbol::getFunction() const
{
	if (aggregate_ == NULL) 
		return NULL;
	return dynamic_cast<Function *>(aggregate_);
}

SYMTAB_EXPORT bool Symbol::isVariable() const 
{
	return (getVariable() != NULL);
}

SYMTAB_EXPORT bool Symbol::setVariable(Variable *var) 
{
	aggregate_ = var;
	return true;
}

SYMTAB_EXPORT Variable * Symbol::getVariable() const
{
	return dynamic_cast<Variable *>(aggregate_);
}

SYMTAB_EXPORT bool Symbol::setSize(unsigned ns)
{
	size_ = ns;
	return true;
}

SYMTAB_EXPORT bool Symbol::setSymbolType(SymbolType sType)
{
	if ((sType != ST_UNKNOWN) &&
			(sType != ST_FUNCTION) &&
			(sType != ST_OBJECT) &&
			(sType != ST_MODULE) &&
			(sType != ST_NOTYPE) &&
			(sType != ST_INDIRECT))
		return false;

	SymbolType oldType = type_;	
	type_ = sType;

//	if (module_ && module_->exec())
//		module_->exec()->changeType(this, oldType);

	return true;
}

SYMTAB_EXPORT bool Symbol::setVersionFileName(std::string &fileName)
{
	std::string *fn_p = NULL;

	if (getAnnotation(fn_p, SymbolFileNameAnno)) {
		return false;
	} else {
		//  not sure if we need to copy here or not, so let's do it...
		std::string *fn = new std::string(fileName);

		if (!addAnnotation(fn, SymbolFileNameAnno)) {
			return false;
		}
		return true;
	}

	return false;
}

SYMTAB_EXPORT bool Symbol::setVersions(std::vector<std::string> &vers)
{
	std::vector<std::string> *vn_p = NULL;

	if (getAnnotation(vn_p, SymbolVersionNamesAnno)) {
		return false;
	} else {
		addAnnotation(&vers, SymbolVersionNamesAnno);
	}

	return true;
}

SYMTAB_EXPORT bool Symbol::getVersionFileName(std::string &fileName)
{
	std::string *fn_p = NULL;

	if (getAnnotation(fn_p, SymbolFileNameAnno)) {
		if (fn_p) 
			fileName = *fn_p;
		return true;
	}

	return false;
}

SYMTAB_EXPORT bool Symbol::getVersions(std::vector<std::string> *&vers)
{
	std::vector<std::string> *vn_p = NULL;

	if (getAnnotation(vn_p, SymbolVersionNamesAnno)) {
		if (vn_p) {
			vers = vn_p;
			return true;
		}
	}

	return false;
}

SYMTAB_EXPORT bool Symbol::setMangledName(std::string name)
{
	mangledName_ = name;
	setStrIndex(-1);
	return true;
}

void Symbol::setReferringSymbol(Symbol* referringSymbol) 
{
	referring_= referringSymbol;
}

Symbol* Symbol::getReferringSymbol() {
	return referring_;
}

SymbolIter::SymbolIter(Object & obj)
		: symbols(obj.getAllSymbols()), currentPositionInVector(0) 
{
	symbolIterator = obj.getAllSymbols()->begin();
}

SymbolIter::SymbolIter(const SymbolIter & src) :
		symbols(src.symbols),
		currentPositionInVector(0),
		symbolIterator(src.symbolIterator) 
{
}

SymbolIter::~SymbolIter()
{
}

void SymbolIter::reset() 
{
	currentPositionInVector = 0;
	symbolIterator = symbols->begin();
}

SymbolIter::operator bool() const
{
	return (symbolIterator!=symbols->end());
}

void SymbolIter::operator++ ( int ) 
{
	if ( currentPositionInVector + 1 < (symbolIterator->second).size()) {
		currentPositionInVector++;
		return;
	}

	/* Otherwise, we need a new std::vector. */
	currentPositionInVector = 0;			
	symbolIterator++;
}

const string & SymbolIter::currkey() const 
{
	return symbolIterator->first;
}

/* If it's important that this be const, we could try to initialize
	currentVector to '& symbolIterator.currval()' in the constructor. */

Symbol *SymbolIter::currval()
{
	if (currentPositionInVector >= symbolIterator->second.size())
	{
		return NULL;
	}
	return ((symbolIterator->second)[ currentPositionInVector ]);
}
