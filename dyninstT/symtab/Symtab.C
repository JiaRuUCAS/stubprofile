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
#include "common/pathName.h"
#include "common/MappedFile.h"

#include "debug.h"
#include "Symtab.h"
#include "relocation.h"
#include "exception.h"
#include "Type.h"
#include "Module.h"
#include "Collections.h"
#include "Function.h"
#include "Variable.h"
#include "annotations.h"
#include "Object.h"

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

void symtab_log_perror(const char *msg)
{
	errMsg = std::string(msg);
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

boost::shared_ptr<typeCollection> Symtab::stdTypes()
{
	static boost::shared_ptr<typeCollection> store =
			setupStdTypes();
	return store;
}

boost::shared_ptr<builtInTypeCollection> Symtab::builtInTypes()
{
	static boost::shared_ptr<builtInTypeCollection> store =
			setupBuiltinTypes();
	return store;
}

boost::shared_ptr<typeCollection> Symtab::setupStdTypes() 
{
	boost::shared_ptr<typeCollection> stdTypes =
		boost::shared_ptr<typeCollection>(new typeCollection);

	typeScalar *newType;

	stdTypes->addType(newType = new typeScalar(-1, sizeof(int), "int"));
	newType->decrRefCount();

	Type *charType = new typeScalar(-2, sizeof(char), "char");
	stdTypes->addType(charType);

	std::string tName = "char *";
	typePointer *newPtrType;
	stdTypes->addType(newPtrType = new typePointer(-3, charType, tName));
	charType->decrRefCount();
	newPtrType->decrRefCount();

	Type *voidType = new typeScalar(-11, 0, "void", false);
	stdTypes->addType(voidType);

	tName = "void *";
	stdTypes->addType(newPtrType = new typePointer(-4, voidType, tName));
	voidType->decrRefCount();
	newPtrType->decrRefCount();

	stdTypes->addType(newType = new typeScalar(-12, sizeof(float), "float"));
	newType->decrRefCount();

	stdTypes->addType(newType = new typeScalar(-31, sizeof(long long), "long long"));
	newType->decrRefCount();

	return stdTypes;
}

boost::shared_ptr<builtInTypeCollection> Symtab::setupBuiltinTypes()
{
	boost::shared_ptr<builtInTypeCollection> builtInTypes =
			boost::shared_ptr<builtInTypeCollection>(new builtInTypeCollection);

	typeScalar *newType;

	// NOTE: integral type  mean twos-complement
	// -1  int, 32 bit signed integral type
	// in stab document, size specified in bits, system size is in bytes
	builtInTypes->addBuiltInType(newType = new typeScalar(-1, 4, "int", true));
	newType->decrRefCount();
	// -2  char, 8 bit type holding a character. GDB treats as signed
	builtInTypes->addBuiltInType(newType = new typeScalar(-2, 1, "char", true));
	newType->decrRefCount();
	// -3  short, 16 bit signed integral type
	builtInTypes->addBuiltInType(newType = new typeScalar(-3, 2, "short", true));
	newType->decrRefCount();
	// -4  long, 32/64 bit signed integral type
	builtInTypes->addBuiltInType(newType = new typeScalar(-4, sizeof(long), "long", true));
	newType->decrRefCount();
	// -5  unsigned char, 8 bit unsigned integral type
	builtInTypes->addBuiltInType(newType = new typeScalar(-5, 1, "unsigned char"));
	newType->decrRefCount();
	// -6  signed char, 8 bit signed integral type
	builtInTypes->addBuiltInType(newType = new typeScalar(-6, 1, "signed char", true));
	newType->decrRefCount();
	// -7  unsigned short, 16 bit unsigned integral type
	builtInTypes->addBuiltInType(newType = new typeScalar(-7, 2, "unsigned short"));
	newType->decrRefCount();
	// -8  unsigned int, 32 bit unsigned integral type
	builtInTypes->addBuiltInType(newType = new typeScalar(-8, 4, "unsigned int"));
	newType->decrRefCount();
	// -9  unsigned, 32 bit unsigned integral type
	builtInTypes->addBuiltInType(newType = new typeScalar(-9, 4, "unsigned"));
	newType->decrRefCount();
	// -10 unsigned long, 32 bit unsigned integral type
	builtInTypes->addBuiltInType(newType = new typeScalar(-10, sizeof(unsigned long), "unsigned long"));
	newType->decrRefCount();
	// -11 void, type indicating the lack of a value
	//  XXX-size may not be correct jdd 4/22/99
	builtInTypes->addBuiltInType(newType = new typeScalar(-11, 0, "void", false));
	newType->decrRefCount();
	// -12 float, IEEE single precision
	builtInTypes->addBuiltInType(newType = new typeScalar(-12, sizeof(float), "float", true));
	newType->decrRefCount();
	// -13 double, IEEE double precision
	builtInTypes->addBuiltInType(newType = new typeScalar(-13, sizeof(double), "double", true));
	newType->decrRefCount();
	// -14 long double, IEEE double precision, size may increase in future
	builtInTypes->addBuiltInType(newType = new typeScalar(-14, sizeof(long double), "long double", true));
	newType->decrRefCount();
	// -15 integer, 32 bit signed integral type
	builtInTypes->addBuiltInType(newType = new typeScalar(-15, 4, "integer", true));
	newType->decrRefCount();
	// -16 boolean, 32 bit type. GDB/GCC 0=False, 1=True, all other values
	//  have unspecified meaning
	builtInTypes->addBuiltInType(newType = new typeScalar(-16, sizeof(bool), "boolean"));
	newType->decrRefCount();
	// -17 short real, IEEE single precision
	//  XXX-size may not be correct jdd 4/22/99
	builtInTypes->addBuiltInType(newType = new typeScalar(-17, sizeof(float), "short real", true));
	newType->decrRefCount();
	// -18 real, IEEE double precision XXX-size may not be correct jdd 4/22/99
	builtInTypes->addBuiltInType(newType = new typeScalar(-18, sizeof(double), "real", true));
	newType->decrRefCount();
	// -19 stringptr XXX- size of void * -- jdd 4/22/99
	builtInTypes->addBuiltInType(newType = new typeScalar(-19, sizeof(void *), "stringptr"));
	newType->decrRefCount();
	// -20 character, 8 bit unsigned character type
	builtInTypes->addBuiltInType(newType = new typeScalar(-20, 1, "character"));
	newType->decrRefCount();
	// -21 logical*1, 8 bit type (Fortran, used for boolean or unsigned int)
	builtInTypes->addBuiltInType(newType = new typeScalar(-21, 1, "logical*1"));
	newType->decrRefCount();
	// -22 logical*2, 16 bit type (Fortran, some for boolean or unsigned int)
	builtInTypes->addBuiltInType(newType = new typeScalar(-22, 2, "logical*2"));
	newType->decrRefCount();
	// -23 logical*4, 32 bit type (Fortran, some for boolean or unsigned int)
	builtInTypes->addBuiltInType(newType = new typeScalar(-23, 4, "logical*4"));
	newType->decrRefCount();
	// -24 logical, 32 bit type (Fortran, some for boolean or unsigned int)
	builtInTypes->addBuiltInType(newType = new typeScalar(-24, 4, "logical"));
	newType->decrRefCount();
	// -25 complex, consists of 2 IEEE single-precision floating point values
	builtInTypes->addBuiltInType(newType = new typeScalar(-25, sizeof(float)*2, "complex", true));
	newType->decrRefCount();
	// -26 complex, consists of 2 IEEE double-precision floating point values
	builtInTypes->addBuiltInType(newType = new typeScalar(-26, sizeof(double)*2, "complex*16", true));
	newType->decrRefCount();
	// -27 integer*1, 8 bit signed integral type
	builtInTypes->addBuiltInType(newType = new typeScalar(-27, 1, "integer*1", true));
	newType->decrRefCount();
	// -28 integer*2, 16 bit signed integral type
	builtInTypes->addBuiltInType(newType = new typeScalar(-28, 2, "integer*2", true));
	newType->decrRefCount();

	/* Quick hack to make integer*4 compatible with int for Fortran
		jnb 6/20/01 */
	// This seems questionable - let's try removing that hack - jmo 05/21/04
	/*
	  builtInTypes->addBuiltInType(newType = new type("int",-29,
	  built_inType, 4));
	  newType->decrRefCount();
	*/
	// -29 integer*4, 32 bit signed integral type
	builtInTypes->addBuiltInType(newType = new typeScalar(-29, 4, "integer*4", true));
	newType->decrRefCount();
	// -30 wchar, Wide character, 16 bits wide, unsigned (unknown format)
	builtInTypes->addBuiltInType(newType = new typeScalar(-30, 2, "wchar"));
	newType->decrRefCount();
	// -31 long long, 64 bit signed integral type
	builtInTypes->addBuiltInType(newType = new typeScalar(-31, sizeof(long long), "long long", true));
	newType->decrRefCount();
	// -32 unsigned long long, 64 bit unsigned integral type
	builtInTypes->addBuiltInType(newType = new typeScalar(-32, sizeof(unsigned long long), "unsigned long long"));
	newType->decrRefCount();
	// -33 logical*8, 64 bit unsigned integral type
	builtInTypes->addBuiltInType(newType = new typeScalar(-33, 8, "logical*8"));
	newType->decrRefCount();
	// -34 integer*8, 64 bit signed integral type
	builtInTypes->addBuiltInType(newType = new typeScalar(-34, 8, "integer*8", true));
	newType->decrRefCount();

	return builtInTypes;
}

SYMTAB_EXPORT Symtab::Symtab() :
	LookupInterface(),
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
	object_type_(obj_Unknown), is_eel_(false),
	no_of_sections(0),
	newSectionInsertPoint(0),
	no_of_symbols(0),
	sorted_everyFunction(false),
	isTypeInfoValid_(false),
//	nlines_(0), fdptr_(0), lines_(NULL),
//	stabstr_(NULL), nstabs_(0), stabs_(NULL),
//	stringpool_(NULL),
	hasRel_(false), hasRela_(false), hasReldyn_(false),
	hasReladyn_(false), hasRelplt_(false), hasRelaplt_(false),
	isStaticBinary_(false), isDefensiveBinary_(false),
	func_lookup(NULL),
	mod_lookup_(NULL),
	obj_private(NULL),
	_ref_cnt(1)
{  
	pfq_rwlock_init(symbols_rwlock);
	init_debug_symtabAPI();
	create_printf("%s[%d]: Created symtab via default constructor\n",
					__FILE__, __LINE__);
}

//Symtab::Symtab(unsigned char *mem_image, size_t image_size,
//				const std::string &name, bool defensive_bin,
//				bool &err) :
//	LookupInterface(),
//	AnnotatableSparse(),
//	member_offset_(0),
////	parentArchive_(NULL),
//	mf(NULL), mfForDebugInfo(NULL),
//	imageOffset_(0), imageLen_(0),
//	dataOffset_(0), dataLen_(0),
//	is_a_out(false),
//	main_call_addr_(0),
//	nativeCompiler(false),
//	address_width_(sizeof(int)),
//	code_ptr_(NULL), data_ptr_(NULL),
//	entry_address_(0), base_address_(0), load_address_(0),
//	object_type_(obj_Unknown), is_eel_(false),
//	no_of_sections(0),
//	newSectionInsertPoint(0),
//	no_of_symbols(0),
//	sorted_everyFunction(false),
//	isTypeInfoValid_(false),
////	nlines_(0), fdptr_(0), lines_(NULL),
////	stabstr_(NULL), nstabs_(0), stabs_(NULL),
////	stringpool_(NULL),
//	hasRel_(false), hasRela_(false), hasReldyn_(false),
//	hasReladyn_(false), hasRelplt_(false), hasRelaplt_(false),
//	isStaticBinary_(false),
//	isDefensiveBinary_(defensive_bin),
//	func_lookup(NULL),
//	mod_lookup_(NULL),
//	obj_private(NULL),
//	_ref_cnt(1)
//{
//	pfq_rwlock_init(symbols_rwlock);
//
//	// Initialize error parameter
//	err = false;
//  
//	create_printf("%s[%d]: created symtab for memory image at addr %u\n", 
//				FILE__, __LINE__, mem_image);
//
//	//  createMappedFile handles reference counting
//	mf = MappedFile::createMappedFile(mem_image, image_size, name);
//	if (!mf) {
//		create_printf("%s[%d]: WARNING: creating symtab for memory image at " 
//					"addr %u, createMappedFile() failed\n", FILE__, __LINE__, 
//					mem_image);
//		err = true;
//		return;
//	}
//
//	obj_private = new Object(mf, defensive_bin, 
//							symtab_log_perror, true, this);
//	if (obj_private->hasError()) {
//		err = true;
//		return;
//	}
//
////  FIXME	
////	if (!extractInfo(obj_private)) {
////		create_printf("%s[%d]: WARNING: creating symtab for memory image at addr" 
////					"%u, extractInfo() failed\n", FILE__, __LINE__, mem_image);
////		err = true;
////	}
//
//	member_name_ = mf->filename();
//	defaultNamespacePrefix = "";
//}

Symtab::Symtab(const Symtab& obj) :
	LookupInterface(),
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
	object_type_(obj_Unknown), is_eel_(false),
	defaultNamespacePrefix(obj.defaultNamespacePrefix),
	no_of_sections(0),
	newSectionInsertPoint(0),
	no_of_symbols(obj.no_of_symbols),
	sorted_everyFunction(false),
	isTypeInfoValid_(obj.isTypeInfoValid_),
//	nlines_(0), fdptr_(0), lines_(NULL),
//	stabstr_(NULL), nstabs_(0), stabs_(NULL),
//	stringpool_(NULL),
	hasRel_(false), hasRela_(false), hasReldyn_(false),
	hasReladyn_(false), hasRelplt_(false), hasRelaplt_(false),
	isStaticBinary_(false), isDefensiveBinary_(obj.isDefensiveBinary_),
	func_lookup(NULL),
	mod_lookup_(NULL),
	obj_private(NULL),
	_ref_cnt(1)
{
	pfq_rwlock_init(symbols_rwlock);
	create_printf("%s[%d]: Creating symtab 0x%p from symtab 0x%p\n",
					__FILE__, __LINE__, this, &obj);

	unsigned i;

	for (i=0; i < obj.regions_.size(); i++) {
		regions_.push_back(new Region(*(obj.regions_[i])));
		regions_.back()->setSymtab(this);
	}

	for (i=0;i<regions_.size();i++)
		regionsByEntryAddr[regions_[i]->getMemOffset()] = regions_[i];

	for (i=0;i<obj.indexed_modules.size();i++) {
		Module *m = new Module(*(obj.indexed_modules[i]));
		indexed_modules.push_back(m);
	}

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
	object_type_(obj_Unknown), is_eel_(false),
	no_of_sections(0),
	newSectionInsertPoint(0),
	no_of_symbols(0),
	sorted_everyFunction(false),
	isTypeInfoValid_(false),
//	nlines_(0), fdptr_(0), lines_(NULL),
//	stabstr_(NULL), nstabs_(0), stabs_(NULL),
//	stringpool_(NULL),
	hasRel_(false), hasRela_(false), hasReldyn_(false),
	hasReladyn_(false), hasRelplt_(false), hasRelaplt_(false),
	isStaticBinary_(false), isDefensiveBinary_(false),
	func_lookup(NULL),
	mod_lookup_(NULL),
	obj_private(NULL),
	_ref_cnt(1)
{
	pfq_rwlock_init(symbols_rwlock);
	init_debug_symtabAPI();
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

	for (unsigned i = 0; i < everyFunction.size(); i++)
		delete everyFunction[i];

	everyFunction.clear();
	funcsByOffset.clear();

	for (unsigned i = 0; i < everyVariable.size(); i++)
		delete everyVariable[i];

	everyVariable.clear();
	varsByOffset.clear();

	for (auto i = indexed_modules.begin(); i != indexed_modules.end(); ++i)
		delete (*i);
	indexed_modules.clear();

	for (unsigned i=0;i<excpBlocks.size();i++)
		delete excpBlocks[i];

	create_printf("%s[%d]: Symtab::~Symtab removing %p from allSymtabs\n", 
		__FILE__, __LINE__, this);

	deps_.clear();

	for (unsigned i = 0; i < allSymtabs.size(); i++) {
		if (allSymtabs[i] == this)
			allSymtabs.erase(allSymtabs.begin()+i);
	}

	delete func_lookup;
	delete mod_lookup_;

	// Make sure to free the underlying Object as it doesn't have a factory
	// open method
	delete obj_private;

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
			create_printf("%s[%d]:  failed to addAnnotation here\n",
							__FILE__, __LINE__);
			return false;
		}
	}

	if (!user_types) {
		create_printf("%s[%d]:  failed to addAnnotation here\n", FILE__, __LINE__);
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

bool Symtab::buildDemangledName(const std::string &mangled, 
		std::string &pretty,
		std::string &typed,
		bool nativeCompiler, 
		supportedLanguages lang )
{
	/* The C++ demangling function demangles MPI__Allgather (and other MPI__
	* functions with start with A) into the MPI constructor.  In order to
	* prevent this a hack needed to be made, and this seemed the cleanest
	* approach.
	*/

	if ((mangled.length()>5) &&
					(mangled.substr(0,5)==std::string("MPI__")))
		return false;	 

	/* If it's Fortran, eliminate the trailing underscores. */
	if (lang == lang_Fortran
			|| lang == lang_CMFortran
			|| lang == lang_Fortran_with_pretty_debug) {
		if (mangled[mangled.length() - 1] == '_') {
			char * demangled = P_strdup( mangled.c_str() );

			demangled[ mangled.length() - 1 ] = '\0';
			pretty = std::string( demangled );

			free ( demangled );
			return true;
		} else {
			/* No trailing underscores, do nothing */
			return false;
		}
	} /* end if it's Fortran. */

	// Check to see if we have a gnu versioned symbol on our hands.
	// These are of the form <symbol>@<version> or <symbol>@@<version>
	// If we do, we want to create a "demangled" name for the one that
	// is of the form <symbol>@@<version> since this is, by definition,
	// the default.  The "demangled" name will just be <symbol>

	// NOTE:  this is just a 0th order approach to dealing with versioned
	//		symbols.  We may need to do something more sophisticated
	//		in the future.  JAW 10/03

	const char *atat;

	if (NULL != (atat = strstr(mangled.c_str(), "@@"))) {
		pretty = mangled.substr(0 /*start pos*/,
						(int)(atat - mangled.c_str())/*len*/);
		return true;
	}

	bool retval = false;
	/* Try demangling it. */
	char * demangled = P_cplus_demangle(
					mangled.c_str(), nativeCompiler, false);

	if (demangled) {
		pretty = std::string(demangled);
		retval = true;
	}

	char *t_demangled = P_cplus_demangle(
					mangled.c_str(), nativeCompiler, true);

	if (t_demangled && (strcmp(t_demangled, demangled) != 0)) {
		typed = std::string(t_demangled);
		retval = true;
	}

	if (demangled)
		free(demangled);
	if (t_demangled)
		free(t_demangled);

	return retval;
} /* end buildDemangledName() */

bool Symtab::demangleSymbol(Symbol *&sym) {
	bool typed_demangle = false;

	if (sym->getType() == Symbol::ST_FUNCTION)
		typed_demangle = true;

	// This is a bit of a hack; we're trying to demangle undefined symbols which don't necessarily
	// have a ST_FUNCTION type. 
	if (sym->getRegion() == NULL && !sym->isAbsolute() &&
					!sym->isCommonStorage())
		typed_demangle = true;

	if (typed_demangle) {
		Module *rawmod = sym->getModule();

		// At this point we need to generate the following information:
		// A symtab name.
		// A pretty (demangled) name.
		// The symtab name goes in the global list as well as the module list.
		// Same for the pretty name.
		// Finally, check addresses to find aliases.
		std::string mangled_name = sym->getMangledName();
		std::string working_name = mangled_name;

		//Remove extra stabs information
		size_t colon = working_name.find(":");
		if(colon != std::string::npos) {
			working_name = working_name.substr(0, colon);
		}

		std::string pretty_name = working_name;
		std::string typed_name = working_name;

		if (!buildDemangledName(working_name, pretty_name,
				typed_name, nativeCompiler,
				(rawmod ? rawmod->language() : lang_Unknown))) {
			pretty_name = working_name;
		}
	}
	else {
		// All cases where there really shouldn't be a mangled
		// name, since mangling is for functions.
		char *prettyName = P_cplus_demangle(
						sym->getMangledName().c_str(),
						nativeCompiler, false);

		if (prettyName) {
			free(prettyName); 
		}
	}

	return true;
}

void Symtab::parseTypes()
{
	Object *linkedFile = getObject();

	if (!linkedFile)
		return;

	linkedFile->parseTypeInfo();

	for (auto i = indexed_modules.begin();
					i != indexed_modules.end(); i++) {
		(*i)->setModuleTypes(typeCollection::getModTypeCollection((*i)));
		(*i)->finalizeRanges();
	}

	typeCollection::fileToTypesMap.clear();
}

void Symtab::parseTypesNow()
{
	if (isTypeInfoValid_)
		return;

	isTypeInfoValid_ = true;
	parseTypes();
}

void Symtab::createDefaultModule() {
	assert(indexed_modules.empty());

	Module *mod = new Module(lang_Unknown, imageOffset_, name(), this);
	mod->addRange(imageOffset_, imageLen_ + imageOffset_);
	indexed_modules.push_back(mod);
	mod->finalizeRanges();
}

bool Symtab::setDefaultNamespacePrefix(string &str)
{
	defaultNamespacePrefix = str;
	return true;
}

ModRangeLookup *Symtab::mod_lookup() {
	if(!mod_lookup_)
		mod_lookup_ = new ModRangeLookup;

	return mod_lookup_;
}

/* A hacky override for specially treating symbols that appear
 * to be functions or variables but aren't.
 *
 * Example: IA-32/AMD-64 libc (and others compiled with libc headers)
 * uses outlined locking primitives. These are named _L_lock_<num>
 * and _L_unlock_<num> and labelled as functions. We explicitly do
 * not include them in function scope.
 *
 * Also, exclude symbols that begin with _imp_ in defensive mode.
 * These symbols are entries in the IAT and shouldn't be treated
 * as functions.
 */
bool Symtab::doNotAggregate(const Symbol* sym) {
	const std::string& mangled = sym->getMangledName();

	if (isDefensiveBinary() && mangled.compare(0, 5, "_imp_", 5) == 0) {
		return true;
	}

	if (mangled.compare(0, strlen("_L_lock_"), "_L_lock_") == 0) {
		return true;
	}

	if (mangled.compare(0, strlen("_L_unlock_"), "_L_unlock_") == 0) {
		return true;
	}

	return false;
}

// Address must be in code or data range since some code may end up
// in the data segment
bool Symtab::isValidOffset(const Offset where) const
{
	return isCode(where) || isData(where);
}

/* Performs a binary search on the codeRegions_ vector, which must
 * be kept in sorted order
 */
bool Symtab::isCode(const Offset where)  const
{
	if (!codeRegions_.size()) {
		create_printf("%s[%d] No code regions in %s \n",
						  __FILE__, __LINE__, mf->filename().c_str());
		return false;
	}

	// search for "where" in codeRegions_ (code regions must not overlap)
	int first = 0; 
	int last = codeRegions_.size() - 1;

	while (last >= first) {
		Region *curreg = codeRegions_[(first + last) / 2];

		if (where >= curreg->getMemOffset()
				&& where < (curreg->getMemOffset()
					+ curreg->getMemSize())) {
			if (curreg->getRegionType() == Region::RT_BSS)
				return false;
			return true;
		}
		else if (where < curreg->getMemOffset()) {
			last = ((first + last) / 2) - 1;
		}
		else if (where >= (curreg->getMemOffset() + curreg->getMemSize())) {
			first = ((first + last) / 2) + 1;
		}
		else {
			// "where" is in the range: 
			// [memOffset + diskSize , memOffset + memSize)
			// meaning that it's in an uninitialized data region 
			return false;
		}
	}

	return false;
}

/* Performs a binary search on the dataRegions_ vector, which must
 * be kept in sorted order */
bool Symtab::isData(const Offset where)  const
{
	if (!dataRegions_.size()) {
		create_printf("%s[%d] No data regions in %s \n",
						  __FILE__,__LINE__,mf->filename().c_str());
		return false;
	}

	int first = 0; 
	int last = dataRegions_.size() - 1;

	while (last >= first) {
		Region *curreg = dataRegions_[(first + last) / 2];

		if ((where >= curreg->getMemOffset())
						&& (where < (curreg->getMemOffset() + curreg->getMemSize()))) {
			return true;
		}
		else if (where < curreg->getMemOffset()) {
			last = ((first + last) / 2) - 1;
		}
		else {
			first = ((first + last) / 2) + 1;
		}
	}

	return false;
}
