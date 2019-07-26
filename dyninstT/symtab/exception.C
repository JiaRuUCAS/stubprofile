#include <iostream>
#include "exception.h"

using namespace Dyninst;
using namespace Dyninst::SymtabAPI;
using namespace std;

SYMTAB_EXPORT ExceptionBlock::ExceptionBlock(Offset tStart,
				unsigned tSize, Offset cStart) :
		tryStart_(tStart),
		trySize_(tSize),
		catchStart_(cStart),
		hasTry_(true),
		tryStart_ptr(0),
		tryEnd_ptr(0),
		catchStart_ptr(0),
		fdeStart_ptr(0),
		fdeEnd_ptr(0)
{
}

SYMTAB_EXPORT ExceptionBlock::ExceptionBlock(Offset cStart) :
		tryStart_(0),
		trySize_(0),
		catchStart_(cStart),
		hasTry_(false),
		tryStart_ptr(0),
		tryEnd_ptr(0),
		catchStart_ptr(0),
		fdeStart_ptr(0),
		fdeEnd_ptr(0)
{
}

SYMTAB_EXPORT ExceptionBlock::ExceptionBlock(const ExceptionBlock &eb) :
		Serializable(),
		tryStart_(eb.tryStart_),
		trySize_(eb.trySize_), 
		catchStart_(eb.catchStart_),
		hasTry_(eb.hasTry_),
		tryStart_ptr(eb.tryStart_ptr),
		tryEnd_ptr(eb.tryEnd_ptr),
		catchStart_ptr(eb.catchStart_ptr),
		fdeStart_ptr(eb.fdeStart_ptr),
		fdeEnd_ptr(eb.fdeEnd_ptr)
{
}

SYMTAB_EXPORT ExceptionBlock::~ExceptionBlock() 
{
}

SYMTAB_EXPORT ExceptionBlock::ExceptionBlock() :
		tryStart_(0),
		trySize_(0),
		catchStart_(0),
		hasTry_(false),
		tryStart_ptr(0),
		tryEnd_ptr(0),
		catchStart_ptr(0),
		fdeStart_ptr(0),
		fdeEnd_ptr(0)
{
}

SYMTAB_EXPORT Offset ExceptionBlock::catchStart() const 
{
	return catchStart_;
}

SYMTAB_EXPORT bool ExceptionBlock::hasTry() const
{
	return hasTry_; 
}

SYMTAB_EXPORT Offset ExceptionBlock::tryStart() const
{
	return tryStart_; 
}

SYMTAB_EXPORT Offset ExceptionBlock::tryEnd() const
{
	return tryStart_ + trySize_; 
}

SYMTAB_EXPORT Offset ExceptionBlock::trySize() const
{
	return trySize_; 
}

SYMTAB_EXPORT bool ExceptionBlock::contains(Offset a) const
{ 
	return (a >= tryStart_ && a < tryStart_ + trySize_); 
}

#if !defined(SERIALIZATION_DISABLED)
Serializable * ExceptionBlock::serialize_impl(
				SerializerBase *sb, const char *tag) THROW_SPEC (SerializerError)
{
	ifxml_start_element(sb, tag);
	gtranslate(sb, tryStart_, "tryStart");
	gtranslate(sb, trySize_, "trySize");
	gtranslate(sb, catchStart_, "catchStart");
	gtranslate(sb, hasTry_, "hasTry");
	ifxml_end_element(sb, tag);
	return NULL;
}
#else
Serializable * ExceptionBlock::serialize_impl(
				SerializerBase *, const char *) THROW_SPEC (SerializerError)
{
   return NULL;
}
#endif

std::ostream& Dyninst::SymtabAPI::operator<<(ostream &s, const ExceptionBlock &eb) 
{
	s << "tryStart=" << eb.tryStart_
	  << ", trySize=" << eb.trySize_
	  << ", catchStart=" << eb.catchStart_
	  << ", hasTry=" << eb.trySize_
	  << ", tryStart_ptr=" << eb.tryStart_ptr
	  << ", tryEnd_ptr=" << eb.tryEnd_ptr
	  << ", catchStart_ptr=" << eb.catchStart_ptr;

	return s; 
}
