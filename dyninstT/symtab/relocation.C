#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <elf.h>

#include "common/Serialization.h"

#include "annotations.h"
#include "relocation.h"
#include "Symbol.h"
#include "Region.h"

using namespace std;
using namespace Dyninst;
using namespace Dyninst::SymtabAPI;

static const unsigned X86_64_WIDTH = 8;

SYMTAB_EXPORT relocationEntry::relocationEntry() :
	target_addr_(0), 
	rel_addr_(0), 
	addend_(0), 
	rtype_(Region::RT_REL), 
	name_(""), 
	dynref_(NULL), 
	relType_(0),
	rel_struct_addr_(0)
{
}	

SYMTAB_EXPORT relocationEntry::relocationEntry(
				Offset ta, Offset ra, std::string n,
				Symbol *dynref, unsigned long relType) :
	target_addr_(ta),
	rel_addr_(ra),
	addend_(0),
	rtype_(Region::RT_REL),
	name_(n),
	dynref_(dynref),
	relType_(relType),
	rel_struct_addr_(0)
{
}

SYMTAB_EXPORT relocationEntry::relocationEntry(
				Offset ta, Offset ra, Offset add,
				std::string n, Symbol *dynref, unsigned long relType) :
	target_addr_(ta),
	rel_addr_(ra),
	addend_(add),
	rtype_(Region::RT_REL),
	name_(n),
	dynref_(dynref),
	relType_(relType),
	rel_struct_addr_(0)
{
}

SYMTAB_EXPORT relocationEntry::relocationEntry(
				Offset ra, std::string n,
				Symbol *dynref, unsigned long relType,
				Region::RegionType rtype) :
	target_addr_(0),
	rel_addr_(ra),
	addend_(0),
	rtype_(rtype),
	name_(n),
	dynref_(dynref),
	relType_(relType),
	rel_struct_addr_(0)
{
}

SYMTAB_EXPORT relocationEntry::relocationEntry(
				Offset ta, Offset ra, Offset add,
				std::string n, Symbol *dynref, unsigned long relType,
				Region::RegionType rtype) :
	target_addr_(ta),
	rel_addr_(ra),
	addend_(add),
	rtype_(rtype),
	name_(n),
	dynref_(dynref),
	relType_(relType),
	rel_struct_addr_(0)
{
}

SYMTAB_EXPORT Offset relocationEntry::target_addr() const 
{
	return target_addr_;
}

SYMTAB_EXPORT void relocationEntry::setTargetAddr(const Offset off)
{
	target_addr_ = off;
}

SYMTAB_EXPORT Offset relocationEntry::rel_addr() const 
{
	return rel_addr_;
}

SYMTAB_EXPORT void relocationEntry::setRelAddr(const Offset value)
{
	rel_addr_ = value;
}

SYMTAB_EXPORT const string &relocationEntry::name() const 
{
	return name_;
}

SYMTAB_EXPORT Symbol *relocationEntry::getDynSym() const 
{
	return dynref_;
}

SYMTAB_EXPORT bool relocationEntry::addDynSym(Symbol *dynref) 
{
	dynref_ = dynref;
	return true;
}

SYMTAB_EXPORT Region::RegionType relocationEntry::regionType() const
{
	return rtype_;
}

SYMTAB_EXPORT unsigned long relocationEntry::getRelType() const 
{
	return relType_;
}

SYMTAB_EXPORT Offset relocationEntry::addend() const
{
	return addend_;
}

SYMTAB_EXPORT void relocationEntry::setAddend(const Offset value)
{
	addend_ = value;
}

SYMTAB_EXPORT void relocationEntry::setRegionType(const Region::RegionType value)
{
	rtype_ = value;
}

SYMTAB_EXPORT void relocationEntry::setName(const std::string &newName) {
	name_ = newName;
}

bool relocationEntry::operator==(const relocationEntry &r) const
{
	if (target_addr_ != r.target_addr_) return false;
	if (rel_addr_ != r.rel_addr_) return false;
	if (addend_ != r.addend_) return false;
	if (rtype_ != r.rtype_) return false;
	if (name_ != r.name_) return false;
	if (relType_ != r.relType_) return false;
	if (dynref_ && !r.dynref_) return false;
	if (!dynref_ && r.dynref_) return false;
	if (dynref_) {
		if (dynref_->getMangledName() != r.dynref_->getMangledName()) return false;
		if (dynref_->getOffset() != r.dynref_->getOffset()) return false;
	}

	return true;
}

#if !defined(SERIALIZATION_DISABLED)
Serializable *relocationEntry::serialize_impl(
				SerializerBase *sb, const char *tag) THROW_SPEC(SerializerError)
{
	/* on deserialize need to rebuild symtab::undefDynSyms before
	 * deserializing relocations
	 */

	std::string symname = dynref_ ? dynref_->getName() : std::string("");
	Offset symoff = dynref_ ? dynref_->getOffset() : (Offset) -1;

	ifxml_start_element(sb, tag);
	gtranslate(sb, target_addr_, "targetAddress");
	gtranslate(sb, rel_addr_, "relocationAddress");
	gtranslate(sb, addend_, "Addend");
	gtranslate(sb, name_, "relocationName");
	gtranslate(sb,  rtype_, Region::regionType2Str, "regionType");
	gtranslate(sb, relType_, "relocationType");
	gtranslate(sb, symname, "SymbolName");
	gtranslate(sb, symoff, "SymbolOffset");
	ifxml_end_element(sb, tag);

	if (sb->isInput()) {
		dynref_ = NULL;
//		if (symname != std::string("")) {
//			SerContextBase *scb = sb->getContext();
//			if (!scb)
//				SER_ERR("FIXME");
//
//			SerContext<Symtab> *scs = dynamic_cast<SerContext<Symtab> *>(scb);
//
//			if (!scs)
//				SER_ERR("FIXME");
//
//			Symtab *st = scs->getScope();
//
//			if (!st)
//				SER_ERR("FIXME");
//
//			std::vector<Symbol *> *syms = st->findSymbolByOffset(symoff);
//
//			if (!syms || !syms->size()) {
//				serialize_printf("%s[%d]:  cannot find symbol by offset %p\n",
//								FILE__, __LINE__, (void *)symoff);
//				return NULL;
//			}
//
//			//  Might want to try to select the "best" symbol here if there is
//			//  more than one.  Or Maybe just returning the first is sufficient.
//
//			dynref_ = (*syms)[0];
//		}
	}
	return NULL;
}
#else
Serializable *relocationEntry::serialize_impl(
				SerializerBase *, const char *) THROW_SPEC (SerializerError)
{
	return NULL;
}
#endif

ostream & Dyninst::SymtabAPI::operator<<(ostream &os, const relocationEntry &r) 
{
	if( r.getDynSym() != NULL )
		os << "Name: " << setw(20) << ( "'" + r.getDynSym()->getMangledName() + "'" );
	else
		os << "Name: " << setw(20) << r.name();

	os << " Offset: " << std::hex << std::setfill('0') << setw(8) << r.rel_addr() 
		<< std::dec << std::setfill(' ')
		<< " Offset: " << std::hex << std::setfill('0') << setw(8) << r.target_addr() 
		<< std::dec << std::setfill(' ')
		<< " Addend: " << r.addend()
		<< " Region: " << Region::regionType2Str(r.regionType())
		<< " Type: " << setw(15) << relocationEntry::relType2Str(r.getRelType())
		<< "(" << r.getRelType() << ")";

	if( r.getDynSym() != NULL ) {
		os << " Symbol Offset: " << std::hex << std::setfill('0')
				<< setw(8) << r.getDynSym()->getOffset();
		os << std::dec << std::setfill(' ');
		if( r.getDynSym()->isCommonStorage() ) {
			os << " COM";
		}else if( r.getDynSym()->getRegion() == NULL ) {
			os << " UND";
		}
	}
	return os;
}

const char* relocationEntry::relType2Str(
				unsigned long r, unsigned addressWidth) {
	if(X86_64_WIDTH == addressWidth) {
		switch(r) {
			CASE_RETURN_STR(R_X86_64_NONE);
			CASE_RETURN_STR(R_X86_64_64);
			CASE_RETURN_STR(R_X86_64_PC32);
			CASE_RETURN_STR(R_X86_64_GOT32);
			CASE_RETURN_STR(R_X86_64_PLT32);
			CASE_RETURN_STR(R_X86_64_COPY);
			CASE_RETURN_STR(R_X86_64_GLOB_DAT);
			CASE_RETURN_STR(R_X86_64_RELATIVE);
#if defined(R_X86_64_IRELATIVE)
			CASE_RETURN_STR(R_X86_64_IRELATIVE);
#endif
			CASE_RETURN_STR(R_X86_64_GOTPCREL);
			CASE_RETURN_STR(R_X86_64_32);
			CASE_RETURN_STR(R_X86_64_32S);
			CASE_RETURN_STR(R_X86_64_16);
			CASE_RETURN_STR(R_X86_64_PC16);
			CASE_RETURN_STR(R_X86_64_8);
			CASE_RETURN_STR(R_X86_64_PC8);
			CASE_RETURN_STR(R_X86_64_DTPMOD64);
			CASE_RETURN_STR(R_X86_64_DTPOFF64);
			CASE_RETURN_STR(R_X86_64_TPOFF64);
			CASE_RETURN_STR(R_X86_64_TLSGD);
			CASE_RETURN_STR(R_X86_64_TLSLD);
			CASE_RETURN_STR(R_X86_64_DTPOFF32);
			CASE_RETURN_STR(R_X86_64_GOTTPOFF);
			CASE_RETURN_STR(R_X86_64_TPOFF32);
			CASE_RETURN_STR(R_X86_64_JUMP_SLOT);
			default:
				return "?";
		}
	} else {
		switch(r) {
			CASE_RETURN_STR(R_386_NONE);
			CASE_RETURN_STR(R_386_32);
			CASE_RETURN_STR(R_386_PC32);
			CASE_RETURN_STR(R_386_GOT32);
			CASE_RETURN_STR(R_386_PLT32);
			CASE_RETURN_STR(R_386_COPY);
			CASE_RETURN_STR(R_386_GLOB_DAT);
			CASE_RETURN_STR(R_386_JMP_SLOT);
			CASE_RETURN_STR(R_386_RELATIVE);
			CASE_RETURN_STR(R_386_GOTOFF);
			CASE_RETURN_STR(R_386_GOTPC);
			CASE_RETURN_STR(R_386_TLS_TPOFF);
			CASE_RETURN_STR(R_386_TLS_IE);
			CASE_RETURN_STR(R_386_TLS_GOTIE);
			CASE_RETURN_STR(R_386_TLS_LE);
			CASE_RETURN_STR(R_386_TLS_GD);
			CASE_RETURN_STR(R_386_TLS_LDM);
			CASE_RETURN_STR(R_386_TLS_GD_32);
			CASE_RETURN_STR(R_386_TLS_GD_PUSH);
			CASE_RETURN_STR(R_386_TLS_GD_CALL);
			CASE_RETURN_STR(R_386_TLS_GD_POP);
			CASE_RETURN_STR(R_386_TLS_LDM_32);
			CASE_RETURN_STR(R_386_TLS_LDM_PUSH);
			CASE_RETURN_STR(R_386_TLS_LDM_CALL);
			CASE_RETURN_STR(R_386_TLS_LDM_POP);
			CASE_RETURN_STR(R_386_TLS_LDO_32);
			CASE_RETURN_STR(R_386_TLS_IE_32);
			CASE_RETURN_STR(R_386_TLS_LE_32);
			CASE_RETURN_STR(R_386_TLS_DTPMOD32);
			CASE_RETURN_STR(R_386_TLS_DTPOFF32);
			CASE_RETURN_STR(R_386_TLS_TPOFF32);
			CASE_RETURN_STR(R_386_16);
			CASE_RETURN_STR(R_386_PC16);
			CASE_RETURN_STR(R_386_8);
			CASE_RETURN_STR(R_386_PC8);
			CASE_RETURN_STR(R_386_32PLT);
			default:
				return "?";
		}
	}
}

SYMTAB_EXPORT unsigned long relocationEntry::getGlobalRelType(
				unsigned addressWidth, Symbol *)
{
	if( X86_64_WIDTH == addressWidth ) {
		return R_X86_64_GLOB_DAT;
	} else {
		return R_386_GLOB_DAT;
	}
}

relocationEntry::category relocationEntry::getCategory(
				unsigned addressWidth)
{
	if(addressWidth == 8) {
		switch(getRelType()) {
			case R_X86_64_RELATIVE:
			case R_X86_64_IRELATIVE:
				return category::relative; 
			case R_X86_64_JUMP_SLOT:
				return category::jump_slot; 
			default:
				return category::absolute;
		}
	} else {
		switch(getRelType()) {
			case R_386_RELATIVE:
			case R_386_IRELATIVE:
				return category::relative; 
			case R_386_JMP_SLOT:
				return category::jump_slot; 
			default:
				return category::absolute;
		}
	}
}
