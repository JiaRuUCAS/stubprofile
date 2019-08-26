#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <algorithm>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <dlfcn.h>
#include <iomanip>
#include <stdarg.h>

#include "common/race-detector-annotations.h"

#include "common/Timer.h"
#include "common/serialize.h"
#include "common/pathName.h"
#include "common/Serialization.h"
#include "common/MappedFile.h"

#include "Symtab.h"
#include "relocation.h"
#include "exception.h"
#include "Type.h"
//#include "Module.h"
//#include "Collections.h"
//#include "Function.h"
//#include "Variable.h"

#include "annotations.h"

using namespace Dyninst;
using namespace Dyninst::SymtabAPI;
using namespace std;

static std::string errMsg;

std::vector<Symtab *> Symtab::allSymtabs;

static thread_local SymtabError serr;

SYMTAB_EXPORT LookupInterface::LookupInterface() 
{
}

SYMTAB_EXPORT LookupInterface::~LookupInterface()
{
}

#define fake_symtab_error_lock race_detector_fake_lock(Symtab::getLastSymtabError)
 
SymtabError Symtab::getLastSymtabError()
{
	race_detector_fake_lock_acquire(fake_symtab_error_lock);
	SymtabError last = serr;
	race_detector_fake_lock_release(fake_symtab_error_lock);
	return last;
}

void Symtab::setSymtabError(SymtabError new_err)
{
	race_detector_fake_lock_acquire(fake_symtab_error_lock);
	serr = new_err;
	race_detector_fake_lock_release(fake_symtab_error_lock);
}

std::string Symtab::printError(SymtabError serr)
{
	switch (serr) {
		case Obj_Parsing:
			return "Failed to parse the Object"+errMsg;
		case Syms_To_Functions:
			return "Failed to convert Symbols to Functions";
		case No_Such_Function:
			return "Function does not exist";
		case No_Such_Variable:
			return "Variable does not exist";
		case No_Such_Module:
		  return "Module does not exist";
		case No_Such_Region:
			return "Region does not exist";
		case No_Such_Symbol:
			return "Symbol does not exist";
		case Not_A_File:
			return "Not a File. Call openArchive()";
		case Not_An_Archive:
			return "Not an Archive. Call openFile()";
		case Export_Error:
			return "Error Constructing XML"+errMsg;
		case Emit_Error:
			return "Error rewriting binary: " + errMsg;
		case Invalid_Flags:
			return "Flags passed are invalid.";
		case No_Error:
			return "No previous Error.";
		default:
			return "Unknown Error";
	}
}

boost::shared_ptr<Type> Symtab::type_Error()
{
	static boost::shared_ptr<Type> store =
			boost::shared_ptr<Type>(new Type(std::string("<error>"),
									0, dataUnknownType));
	return store;
}

boost::shared_ptr<Type> Symtab::type_Untyped()
{
	static boost::shared_ptr<Type> store =
			boost::shared_ptr<Type>(new Type(std::string("<no type>"),
									0, dataUnknownType));
	return store;
}

//boost::shared_ptr<typeCollection> Symtab::stdTypes()
//{
//	static boost::shared_ptr<typeCollection> store =
//			setupStdTypes();
//	return store;
//}
//
//boost::shared_ptr<typeCollection> Symtab::setupStdTypes() 
//{
//	boost::shared_ptr<typeCollection> stdTypes =
//		boost::shared_ptr<typeCollection>(new typeCollection);
//
//	typeScalar *newType;
//
//	stdTypes->addType(newType = new typeScalar(-1, sizeof(int), "int"));
//	newType->decrRefCount();
//
//	Type *charType = new typeScalar(-2, sizeof(char), "char");
//	stdTypes->addType(charType);
//
//	std::string tName = "char *";
//	typePointer *newPtrType;
//	stdTypes->addType(newPtrType = new typePointer(-3, charType, tName));
//	charType->decrRefCount();
//	newPtrType->decrRefCount();
//
//	Type *voidType = new typeScalar(-11, 0, "void", false);
//	stdTypes->addType(voidType);
//
//	tName = "void *";
//	stdTypes->addType(newPtrType = new typePointer(-4, voidType, tName));
//	voidType->decrRefCount();
//	newPtrType->decrRefCount();
//
//	stdTypes->addType(newType = new typeScalar(-12, sizeof(float), "float"));
//	newType->decrRefCount();
//
//	stdTypes->addType(newType = new typeScalar(-31, sizeof(long long), "long long"));
//	newType->decrRefCount();
//
//	return stdTypes;
//}

SYMTAB_EXPORT Symtab::Symtab() :
	LookupInterface(),
	Serializable(),
	AnnotatableSparse(),
	member_offset_(0),
//	parentArchive_(NULL),
	mf(NULL), mfForDebugInfo(NULL),
	imageOffset_(0), imageLen_(0),
	dataOffset_(0), dataLen_(0),
	is_a_out(false),
	main_call_addr_(0),
	nativeCompiler(false),
	address_width_(sizeof(int)),
	code_ptr_(NULL), data_ptr_(NULL),
	entry_address_(0), base_address_(0), load_address_(0),
//	object_type_(obj_Unknown), is_eel_(false),
	no_of_sections(0),
	newSectionInsertPoint(0),
	no_of_symbols(0),
//	sorted_everyFunction(false),
	isTypeInfoValid_(false),
//	nlines_(0), fdptr_(0), lines_(NULL),
//	stabstr_(NULL), nstabs_(0), stabs_(NULL),
//	stringpool_(NULL),
	hasRel_(false), hasRela_(false), hasReldyn_(false),
	hasReladyn_(false), hasRelplt_(false), hasRelaplt_(false),
	isStaticBinary_(false), isDefensiveBinary_(false),
//	func_lookup(NULL),
//	mod_lookup_(NULL),
//	obj_private(NULL),
	_ref_cnt(1)
{  
	 pfq_rwlock_init(symbols_rwlock);
//	 init_debug_symtabAPI();
//	 create_printf("%s[%d]: Created symtab via default constructor\n",
//					 __FILE__, __LINE__);
}

Symtab::Symtab(unsigned char *mem_image, size_t image_size,
				const std::string &name, bool defensive_bin,
				bool &err) :
	LookupInterface(),
	Serializable(),
	AnnotatableSparse(),
	member_offset_(0),
//	parentArchive_(NULL),
	mf(NULL), mfForDebugInfo(NULL),
	imageOffset_(0), imageLen_(0),
	dataOffset_(0), dataLen_(0),
	is_a_out(false),
	main_call_addr_(0),
	nativeCompiler(false),
	address_width_(sizeof(int)),
	code_ptr_(NULL), data_ptr_(NULL),
	entry_address_(0), base_address_(0), load_address_(0),
//	object_type_(obj_Unknown), is_eel_(false),
	no_of_sections(0),
	newSectionInsertPoint(0),
	no_of_symbols(0),
//	sorted_everyFunction(false),
	isTypeInfoValid_(false),
//	nlines_(0), fdptr_(0), lines_(NULL),
//	stabstr_(NULL), nstabs_(0), stabs_(NULL),
//	stringpool_(NULL),
	hasRel_(false), hasRela_(false), hasReldyn_(false),
	hasReladyn_(false), hasRelplt_(false), hasRelaplt_(false),
	isStaticBinary_(false),
	isDefensiveBinary_(defensive_bin),
//	func_lookup(NULL),
//	mod_lookup_(NULL),
//	obj_private(NULL),
	_ref_cnt(1)
{
	pfq_rwlock_init(symbols_rwlock);

	// Initialize error parameter
	err = false;
  
//	create_printf("%s[%d]: created symtab for memory image at addr %u\n", 
//				 FILE__, __LINE__, mem_image);

	//  createMappedFile handles reference counting
	mf = MappedFile::createMappedFile(mem_image, image_size, name);
	if (!mf) {
//	  create_printf("%s[%d]: WARNING: creating symtab for memory image at " 
//					"addr %u, createMappedFile() failed\n", FILE__, __LINE__, 
//					mem_image);
	  err = true;
	  return;
	}
//
//	obj_private = new Object(mf, defensive_bin, 
//							symtab_log_perror, true, this);
//	if (obj_private->hasError()) {
//	 err = true;
//	 return;
//	}
//
//	if (!extractInfo(obj_private))
//	{
//	  create_printf("%s[%d]: WARNING: creating symtab for memory image at addr" 
//					"%u, extractInfo() failed\n", FILE__, __LINE__, mem_image);
//	  err = true;
//	}
//
	member_name_ = mf->filename();
	defaultNamespacePrefix = "";
}

Symtab::Symtab(const Symtab& obj) :
	LookupInterface(),
	Serializable(),
	AnnotatableSparse(),
	member_name_(obj.member_name_),
	member_offset_(obj.member_offset_),
//	parentArchive_(NULL),
	mf(NULL), mfForDebugInfo(NULL),
	imageOffset_(obj.imageOffset_), imageLen_(obj.imageLen_),
	dataOffset_(obj.dataOffset_), dataLen_(obj.dataLen_),
	is_a_out(obj.is_a_out),
	main_call_addr_(obj.main_call_addr_),
	nativeCompiler(obj.nativeCompiler),
	address_width_(sizeof(int)),
	code_ptr_(NULL), data_ptr_(NULL),
	entry_address_(0), base_address_(0), load_address_(0),
//	object_type_(obj_Unknown), is_eel_(false),
	defaultNamespacePrefix(obj.defaultNamespacePrefix),
	no_of_sections(0),
	newSectionInsertPoint(0),
	no_of_symbols(obj.no_of_symbols),
//	sorted_everyFunction(false),
	isTypeInfoValid_(obj.isTypeInfoValid_),
//	nlines_(0), fdptr_(0), lines_(NULL),
//	stabstr_(NULL), nstabs_(0), stabs_(NULL),
//	stringpool_(NULL),
	hasRel_(false), hasRela_(false), hasReldyn_(false),
	hasReladyn_(false), hasRelplt_(false), hasRelaplt_(false),
	isStaticBinary_(false), isDefensiveBinary_(obj.isDefensiveBinary_),
//	func_lookup(NULL),
//	mod_lookup_(NULL),
//	obj_private(NULL),
	_ref_cnt(1)
{
	pfq_rwlock_init(symbols_rwlock);
//	create_printf("%s[%d]: Creating symtab 0x%p from symtab 0x%p\n", FILE__, __LINE__, this, &obj);

	unsigned i;

	for (i=0; i < obj.regions_.size(); i++) {
		regions_.push_back(new Region(*(obj.regions_[i])));
		regions_.back()->setSymtab(this);
	}

	for (i=0;i<regions_.size();i++)
		regionsByEntryAddr[regions_[i]->getMemOffset()] = regions_[i];

//	for (i=0;i<obj.indexed_modules.size();i++) {
//		Module *m = new Module(*(obj.indexed_modules[i]));
//		indexed_modules.push_back(m);
//	}

	for (i=0; i < obj.relocation_table_.size(); i++) {
		relocation_table_.push_back(
						relocationEntry(obj.relocation_table_[i]));
	}

	for (i=0; i < obj.excpBlocks.size(); i++) {
		excpBlocks.push_back(new ExceptionBlock(*(obj.excpBlocks[i])));
	}

	deps_ = obj.deps_;
}

SYMTAB_EXPORT Symtab::Symtab(MappedFile *mf_) :
	AnnotatableSparse(),
	member_offset_(0),
//	parentArchive_(NULL),
	mf(mf_), mfForDebugInfo(NULL),
	imageOffset_(0), imageLen_(0),
	dataOffset_(0), dataLen_(0),
	is_a_out(false),
	main_call_addr_(0),
	nativeCompiler(false),
	address_width_(sizeof(int)),
	code_ptr_(NULL), data_ptr_(NULL),
	entry_address_(0), base_address_(0), load_address_(0),
//	object_type_(obj_Unknown), is_eel_(false),
	no_of_sections(0),
	newSectionInsertPoint(0),
	no_of_symbols(0),
//	sorted_everyFunction(false),
	isTypeInfoValid_(false),
//	nlines_(0), fdptr_(0), lines_(NULL),
//	stabstr_(NULL), nstabs_(0), stabs_(NULL),
//	stringpool_(NULL),
	hasRel_(false), hasRela_(false), hasReldyn_(false),
	hasReladyn_(false), hasRelplt_(false), hasRelaplt_(false),
	isStaticBinary_(false), isDefensiveBinary_(false),
//	func_lookup(NULL),
//	mod_lookup_(NULL),
//	obj_private(NULL),
	_ref_cnt(1)
{
	 pfq_rwlock_init(symbols_rwlock);
//	 init_debug_symtabAPI();
}

Symtab::~Symtab()
{
	// Doesn't do anything yet, moved here so we don't mess with symtab.h
	// Only called if we fail to create a process.
	// Or delete the a.out...

	for (unsigned i = 0; i < regions_.size(); i++)
	  delete regions_[i];

	regions_.clear();
	codeRegions_.clear();
	dataRegions_.clear();
	regionsByEntryAddr.clear();

	std::vector<Region *> *user_regions = NULL;

	getAnnotation(user_regions, UserRegionsAnno);

	if (user_regions) {
		for (unsigned i = 0; i < user_regions->size(); ++i) 
			delete (*user_regions)[i];
		user_regions->clear();
	}

	// Symbols are copied from linkedFile, and NOT deleted
	everyDefinedSymbol.clear();
	undefDynSyms.clear();

//	for (unsigned i = 0; i < everyFunction.size(); i++)
//	  delete everyFunction[i];
//
//	everyFunction.clear();
//	funcsByOffset.clear();
//
//	for (unsigned i = 0; i < everyVariable.size(); i++) 
//	{
//	  delete everyVariable[i];
//	}
//
//	everyVariable.clear();
//	varsByOffset.clear();
//
//	for (auto i = indexed_modules.begin(); i != indexed_modules.end(); ++i)
//	{
//	  delete (*i);
//	}
//	indexed_modules.clear();

	for (unsigned i=0;i<excpBlocks.size();i++)
		delete excpBlocks[i];

//	create_printf("%s[%d]: Symtab::~Symtab removing %p from allSymtabs\n", 
//		 FILE__, __LINE__, this);

	deps_.clear();

	for (unsigned i = 0; i < allSymtabs.size(); i++) {
		if (allSymtabs[i] == this)
			allSymtabs.erase(allSymtabs.begin()+i);
	}

//	delete func_lookup;
//	delete mod_lookup_;
//
//	// Make sure to free the underlying Object as it doesn't have a factory
//	// open method
//	delete obj_private;
//
	if (mf) MappedFile::closeMappedFile(mf);

}

bool Symtab::addType(Type *type)
{
	bool result = false;

	result = addUserType(type);
	if (!result)
		return false;
	return true;
}

bool Symtab::addUserType(Type *t)
{
	std::vector<Type *> *user_types = NULL;

	//  need to change this to something based on AnnotationContainer
	//  for it to work with serialization
	if (!getAnnotation(user_types, UserTypesAnno)) {
		user_types = new std::vector<Type *>();

		if (!addAnnotation(user_types, UserTypesAnno)) {
//			create_printf("%s[%d]:  failed to addAnnotation here\n",
//							__FILE__, __LINE__);
			return false;
		}
	}

	if (!user_types) {
//		create_printf("%s[%d]:  failed to addAnnotation here\n", FILE__, __LINE__);
		return false;
	}

	user_types->push_back(t);

	return true;
}

SYMTAB_EXPORT unsigned Symtab::getAddressWidth() const 
{
	return address_width_;
}

SYMTAB_EXPORT Offset Symtab::preferedBase() const 
{
	return preferedBase_;
}

SYMTAB_EXPORT Offset Symtab::imageOffset() const 
{
	return imageOffset_;
}

SYMTAB_EXPORT Offset Symtab::dataOffset() const 
{ 
	return dataOffset_;
}

SYMTAB_EXPORT Offset Symtab::dataLength() const 
{
	return dataLen_;
} 

SYMTAB_EXPORT Offset Symtab::imageLength() const 
{
	return imageLen_;
}

SYMTAB_EXPORT const char *Symtab::getInterpreterName() const 
{
	if (interpreter_name_.length())
		return interpreter_name_.c_str();
	return NULL;
}
 
SYMTAB_EXPORT Offset Symtab::getEntryOffset() const 
{ 
	return entry_address_;
}

SYMTAB_EXPORT Offset Symtab::getBaseOffset() const 
{
	return base_address_;
}

SYMTAB_EXPORT Offset Symtab::getLoadOffset() const 
{ 
	return load_address_;
}

SYMTAB_EXPORT string Symtab::getDefaultNamespacePrefix() const
{
	return defaultNamespacePrefix;
}

SYMTAB_EXPORT char *Symtab::mem_image() const 
{
	return (char *)mf->base_addr();
}

SYMTAB_EXPORT std::string Symtab::file() const 
{
	assert(mf);
	return mf->pathname();
}

SYMTAB_EXPORT std::string Symtab::name() const 
{
	return mf->filename();
}

SYMTAB_EXPORT std::string Symtab::memberName() const 
{
	return member_name_;
}

SYMTAB_EXPORT unsigned Symtab::getNumberOfRegions() const 
{
	return no_of_sections; 
}

SYMTAB_EXPORT unsigned Symtab::getNumberOfSymbols() const 
{
	return no_of_symbols; 
}
