#ifndef __SYMTAB_SYMTAB_H__
#define __SYMTAB_SYMTAB_H__

#include <set>

#include "Region.h"
#include "Symbol.h"
#include "Module.h"

#include "common/Annotatable.h"
#include "common/ProcReader.h"
#include "common/IBSTree.h"
#include "common/pfq-rwlock.h"

#include "boost/shared_ptr.hpp"
#include "boost/multi_index_container.hpp"
#include <boost/multi_index/member.hpp>
#include <boost/multi_index/mem_fun.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/hashed_index.hpp>
#include <boost/multi_index/identity.hpp>
#include <boost/multi_index/random_access_index.hpp>

using boost::multi_index_container;
using boost::multi_index::indexed_by;
using boost::multi_index::ordered_unique;
using boost::multi_index::ordered_non_unique;
using boost::multi_index::hashed_non_unique;

using boost::multi_index::identity;
using boost::multi_index::tag;
using boost::multi_index::const_mem_fun;
using boost::multi_index::member;

class MappedFile;

namespace Dyninst {
	struct SymSegment;

namespace SymtabAPI {

class Object;
class Type;
class ExceptionBlock;
class typeCollection;
class builtInTypeCollection;
class FunctionBase;
class FuncRange;
class localVar;
class relocationEntry;

typedef IBSTree<ModRange> ModRangeLookup;
typedef IBSTree<FuncRange> FuncRangeLookup;
typedef Dyninst::ProcessReader MemRegReader;

class SYMTAB_EXPORT Symtab : public LookupInterface,
		public AnnotatableSparse
{
	friend class Aggregate;
	friend class Symbol;
	friend class Function;
	friend class Variable;
	friend class Module;
	friend class Region;
	friend class relocationEntry;
	friend class ExceptionBlock;

	private:
		pfq_rwlock_t symbols_rwlock;

		std::string member_name_;
		Offset member_offset_;
		MappedFile *mf;
		MappedFile *mfForDebugInfo;

		Offset preferedBase_;
		Offset imageOffset_;
		unsigned imageLen_;
		Offset dataOffset_;
		unsigned dataLen_;

		bool is_a_out;
		// address of call to main()
		Offset main_call_addr_;

		bool nativeCompiler;

		unsigned address_width_;
		char *code_ptr_;
		char *data_ptr_;
		std::string interpreter_name_;
		Offset entry_address_;
		Offset base_address_;
		Offset load_address_;

		ObjectType object_type_;
		bool is_eel_;
		std::vector<Segment> segments_;

		static std::vector<Symtab *> allSymtabs;
		std::string defaultNamespacePrefix;

		// sections
		unsigned no_of_sections;
		std::vector<Region *> regions_;
		std::vector<Region *> codeRegions_;
		std::vector<Region *> dataRegions_;
		dyn_hash_map<Offset, Region *> regionsByEntryAddr;

		// Point where new loadable sections will be inserted
		unsigned newSectionInsertPoint;

		// symbols
		unsigned no_of_symbols;

		// indices
		struct offset {};
		struct pretty {};
		struct mangled {};
		struct typed {};
		struct id {};

		typedef boost::multi_index_container<Symbol::Ptr, indexed_by <
			ordered_unique<
				tag<id>,
				const_mem_fun<Symbol::Ptr, Symbol *, &Symbol::Ptr::get>>,
			ordered_non_unique<
				tag<offset>,
				const_mem_fun<Symbol, Offset, &Symbol::getOffset>>,
			hashed_non_unique<
				tag<mangled>,
				const_mem_fun<Symbol, std::string, &Symbol::getMangledName>>,
			hashed_non_unique<
				tag<pretty>,
				const_mem_fun<Symbol, std::string, &Symbol::getPrettyName>>,
			hashed_non_unique<
				tag<typed>,
				const_mem_fun<Symbol, std::string, &Symbol::getTypedName>>
			>> indexed_symbols;

		indexed_symbols everyDefinedSymbol;
		indexed_symbols undefDynSyms;

		// per-aggregate indices
		bool sorted_everyFunction;
		std::vector<Function *> everyFunction;
		dyn_hash_map<Offset, Function *> funcsByOffset;

		std::vector<Variable *> everyVariable;
		dyn_hash_map<Offset, Variable *> varsByOffset;

		boost::multi_index_container<Module *,
			boost::multi_index::indexed_by<
				boost::multi_index::random_access<>,
				boost::multi_index::ordered_unique<
					boost::multi_index::identity<Module *>>,
				boost::multi_index::ordered_non_unique<
					boost::multi_index::const_mem_fun<
						Module, const std::string&, &Module::fileName>>,
				boost::multi_index::ordered_non_unique<
					boost::multi_index::const_mem_fun<
						Module, const std::string&, &Module::fileName>>
			>
		> indexed_modules;

		std::vector<relocationEntry> relocation_table_;
		std::vector<ExceptionBlock *> excpBlocks;
		std::vector<std::string> deps_;

		bool isTypeInfoValid_;

		// line number information
//		int nline_;
//		unsigned long fdptr_;
//		char *lines_;
//		char *stabstr_;
//		int nstabs_;
//		void *stabs_;
//		char *stringpool_;

		// relocation sections
		bool hasRel_;
		bool hasRela_;
		bool hasReldyn_;
		bool hasReladyn_;
		bool hasRelplt_;
		bool hasRelaplt_;

		bool isStaticBinary_;
		bool isDefensiveBinary_;

		FuncRangeLookup *func_lookup;
		ModRangeLookup *mod_lookup_;

		Object *obj_private;
		// dynamic libray name substitutions
		std::map<std::string, std::string> dynLibSubs;

		unsigned _ref_cnt;

	public:
		typedef enum {
			NotDefensive,
			Defensive
		} def_t;

		Symtab();
		Symtab(MappedFile *);
		Symtab(const Symtab &obj);
//		Symtab(unsigned char *mem_image, size_t image_size,
//						const std::string &name,
//						bool defensive_binary, bool &err);
		~Symtab();

		/********** Data member access ****************/
		std::string file() const;
		std::string name() const;
		std::string memberName() const;

		char *mem_image() const;

		Offset preferedBase() const;
		Offset imageOffset() const;
		Offset dataOffset() const;
		Offset dataLength() const;
		Offset imageLength() const;
//		Offset getInitOffset();
//		Offset getFiniOffset();

		const char *getInterpreterName() const;

		unsigned getAddressWidth() const;
//		bool isBigEndianDataEncoding() const;
//		bool getABIVersion(int &major, int &minor) const;

		Offset getLoadOffset() const;
		Offset getEntryOffset() const;
		Offset getBaseOffset() const;
//		Offset getTOCoffset(Function *func = NULL) const;
//		Offset getTOCoffset(Offset off) const;
//		Address getLoadAddress();
		bool isDefensiveBinary() const { return isDefensiveBinary_; }
//		Offset fileToDiskOffset(Dyninst::Offset) const;
//		Offset fileToMemOffset(Dyninst::Offset) const;

		std::string getDefaultNamespacePrefix() const;

		unsigned getNumberOfRegions() const;
		unsigned getNumberOfSymbols() const;

		Object *getObject() { return obj_private; }
		const Object *getObject() const { return obj_private; }
		ObjectType getObjectType() const { return object_type_; }

		/********** Query Functions **********/
		bool isStaticBinary() const { return isStaticBinary_; }
		bool isExec() const { return is_a_out; }
		bool isNativeCompiler() const { return nativeCompiler; }
		bool hasRel() const { return hasRel_; }
		bool hasRela() const { return hasRela_; }
		bool hasReldyn() const { return hasReldyn_; }
		bool hasReladyn() const { return hasReladyn_; }
		bool hasRelplt() const { return hasRelplt_; }
		bool hasRelaplt() const { return hasRelaplt_; }
		bool isCode(const Offset where) const;
		bool isData(const Offset where) const;
		bool isValidOffset(const Offset where) const;

		/********* Type Information ********/
		bool addType(Type *typ);
		void parseTypesNow();

		static boost::shared_ptr<builtInTypeCollection> builtInTypes();
		static boost::shared_ptr<typeCollection> stdTypes();

		static boost::shared_ptr<Type> type_Error();
		static boost::shared_ptr<Type> type_Untyped();

		/********* Error Handling *********/
		static SymtabError getLastSymtabError();
		static void setSymtabError(SymtabError new_err);
		static std::string printError(SymtabError serr);

		/************ Symbol Lookup Function *************/
		virtual bool findSymbol(std::vector<Symbol *> &ret,
						const std::string& name,
						Symbol::SymbolType sType = Symbol::ST_UNKNOWN,
						NameType nameType = anyName,
						bool isRegex = false,
						bool checkCase = false,
						bool includeUndefined = false);
		std::vector<Symbol *> findSymbolByOffset(Offset);
		virtual bool getAllSymbols(std::vector<Symbol *> &ret);
		virtual bool getAllSymbolsByType(std::vector<Symbol *> &ret,
						Symbol::SymbolType sType);
		// Return all undefined symbols in the binary. Currently used for
		// finding the .o's in a static archive that have definitions of
		// these symbols.
		bool getAllUndefinedSymbols(std::vector<Symbol *> &ret);
		// Inversely, return all non-undefined symbols in the binary
		bool getAllDefinedSymbols(std::vector<Symbol *> &ret);

		/************ Function Lookup Function *************/
		bool findFuncByEntryOffset(Function *&ret, const Offset offset);
		bool findFunctionsByName(std::vector<Function *> &ret,
						const std::string name,
						NameType nameType = anyName, 
						bool isRegex = false,
						bool checkCase = true);
		bool getAllFunctions(std::vector<Function *>&ret);
		//Searches for functions without returning inlined instances
		bool getContainingFunction(Offset offset, Function* &func);

		/************ Variable Lookup Function *************/
		bool findVariableByOffset(Variable *&ret, const Offset offset);
		bool findVariablesByName(std::vector<Variable *> &ret,
						const std::string name,
						NameType nameType = anyName, 
						bool isRegex = false, 
						bool checkCase = true);
		bool getAllVariables(std::vector<Variable *> &ret);

		/************ Module Lookup Function *************/
		bool getAllModules(std::vector<Module *>&ret);
		bool findModuleByOffset(std::set<Module *>& ret, Offset off);
		bool findModuleByOffset(Module *& ret, Offset off);
		bool findModuleByName(Module *&ret, const std::string name);
		Module *getDefaultModule();

		/************ Region Lookup Function *************/
		bool getCodeRegions(std::vector<Region *>&ret);
		bool getDataRegions(std::vector<Region *>&ret);
		bool getAllRegions(std::vector<Region *>&ret);
		bool getAllNewRegions(std::vector<Region *>&ret);
		//  change me to use a hash
		bool findRegion(Region *&ret, std::string regname);
		bool findRegion(Region *&ret, const Offset addr,
						const unsigned long size);
		bool findRegionByEntry(Region *&ret, const Offset offset);
		Region *findEnclosingRegion(const Offset offset);

		/************ Exception Lookup Function *************/
		bool findException(ExceptionBlock &excp,Offset addr);
		bool getAllExceptions(std::vector<ExceptionBlock *> &exceptions);
		bool findCatchBlock(ExceptionBlock &excp, Offset addr,
						unsigned size = 0);

		/************* Deletion **************/
		bool deleteSymbol(Symbol *sym); 
		bool deleteFunction(Function *func);
		bool deleteVariable(Variable *var);

		/************ Add ************/
		bool addSymbol(Symbol *newsym);
		bool addSymbol(Symbol *newSym, Symbol *referringSymbol);
		Function *createFunction(std::string name, Offset offset,
						size_t size, Module *mod = NULL);
		Variable *createVariable(std::string name, Offset offset,
						size_t size, Module *mod = NULL);

		/************** Update **************/
		bool updateRelocations(Address start, Address end,
						Symbol *oldsym, Symbol *newsym);


		ModRangeLookup* mod_lookup();

	private:
		static boost::shared_ptr<builtInTypeCollection> setupBuiltinTypes();
		static boost::shared_ptr<typeCollection> setupStdTypes();

		bool addUserType(Type *);
		void parseTypes();
		// change the type of a symbol after the fact
		bool changeType(Symbol *sym, Symbol::SymbolType oldType);

		bool buildDemangledName(const std::string &mangled,
						std::string &pretty,
						std::string &typed,
						bool nativeCompiler,
						supportedLanguages lang);
		bool demangleSymbol(Symbol *&sym);
		bool doNotAggregate(const Symbol *sym);

		void createDefaultModule();

		bool deleteSymbolFromIndices(Symbol *sym);
		bool deleteAggregate(Aggregate *agg);

		bool changeSymbolOffset(Symbol *sym, Offset newOffset);
		bool changeAggregateOffset(Aggregate *agg,
						Offset oldOffset, Offset newOffset);

		bool addSymbolToIndices(Symbol *&sym, bool undefined);
		bool addSymbolToAggregates(const Symbol *sym);

		bool setDefaultNamespacePrefix(std::string &str);

		//	typedef enum {
//		NotDefensive,
//		Defensive} def_t; 
//
//	static bool openFile(Symtab *&obj, std::string filename, 
//												  def_t defensive_binary = NotDefensive);
//	static bool openFile(Symtab *&obj, void *mem_image, size_t size, 
//												  std::string name, def_t defensive_binary = NotDefensive);
//	static Symtab *findOpenSymtab(std::string filename);
//	static bool closeSymtab(Symtab *);
//
//	 bool exportXML(std::string filename);
//	bool exportBin(std::string filename);
//	static Symtab *importBin(std::string filename);
//	bool getRegValueAtFrame(Address pc, 
//												 Dyninst::MachRegister reg, 
//												 Dyninst::MachRegisterVal &reg_result,
//												 MemRegReader *reader);
//	bool hasStackwalkDebugInfo();
//
//	/**************************************
//	 *** LOOKUP FUNCTIONS *****************
//	 **************************************/
//
//	// Relocation entries
//	bool getFuncBindingTable(std::vector<relocationEntry> &fbt) const;
//	bool updateFuncBindingTable(Offset stub_addr, Offset plt_addr);
//
//	/**************************************
//	 *** SYMBOL ADDING FUNCS **************
//	 **************************************/
//
//
//	/*****Query Functions*****/
//	bool isStripped();
//	Dyninst::Architecture getArchitecture() const;
//
//	bool getMappedRegions(std::vector<Region *> &mappedRegs) const;
//
//	/***** Line Number Information *****/
//	bool getAddressRanges(std::vector<AddressRange> &ranges,
//								 std::string lineSource, unsigned int LineNo);
//	bool getSourceLines(std::vector<Statement::Ptr> &lines,
//							  Offset addressInRange);
//	bool getSourceLines(std::vector<LineNoTuple> &lines,
//												 Offset addressInRange);
//	bool addLine(std::string lineSource, unsigned int lineNo,
//			unsigned int lineOffset, Offset lowInclAddr,
//			Offset highExclAddr);
//	bool addAddressRange(Offset lowInclAddr, Offset highExclAddr, std::string lineSource,
//			unsigned int lineNo, unsigned int lineOffset = 0);
//	void setTruncateLinePaths(bool value);
//	bool getTruncateLinePaths();
//	void forceFullLineInfoParse();
//	
//	/***** Type Information *****/
//	virtual bool findType(Type *&type, std::string name);
//	virtual Type *findType(unsigned type_id);
//	virtual bool findVariableType(Type *&type, std::string name);
//
//	bool addType(Type *typ);
//
//
//
//	/***** Local Variable Information *****/
//	bool findLocalVariable(std::vector<localVar *>&vars, std::string name);
//
//	/***** Relocation Sections *****/
//	
//
//	/***** Write Back binary functions *****/
//	bool emitSymbols(Object *linkedFile, std::string filename, unsigned flag = 0);
//	bool addRegion(Offset vaddr, void *data, unsigned int dataSize, 
//			std::string name, Region::RegionType rType_, bool loadable = false,
//			unsigned long memAlign = sizeof(unsigned), bool tls = false);
//	bool addRegion(Region *newreg);
//	bool emit(std::string filename, unsigned flag = 0);
//
//	void addDynLibSubstitution(std::string oldName, std::string newName);
//	std::string getDynLibSubstitution(std::string name);
//
//	bool getSegments(std::vector<Segment> &segs) const;
//	
//	void fixup_code_and_data(Offset newImageOffset,
//														Offset newImageLength,
//														Offset newDataOffset,
//														Offset newDataLength);
//	bool fixup_RegionAddr(const char* name, Offset memOffset, long memSize);
//	bool fixup_SymbolAddr(const char* name, Offset newOffset);
//	bool updateRegion(const char* name, void *buffer, unsigned size);
//	bool updateCode(void *buffer, unsigned size);
//	bool updateData(void *buffer, unsigned size);
//	Offset getFreeOffset(unsigned size);
//
//	bool addLibraryPrereq(std::string libname);
//	bool addSysVDynamic(long name, long value);
//
//	bool addLinkingResource(Archive *library);
//	bool getLinkingResources(std::vector<Archive *> &libs);
//
//	bool addExternalSymbolReference(Symbol *externalSym, Region *localRegion, relocationEntry localRel);
//	bool addTrapHeader_win(Address ptr);
//
//
//	/***** Data Member Access *****/
//	std::vector<std::string> &getDependencies();
//	bool removeLibraryDependency(std::string lib);
//
//	Archive *getParentArchive() const;
//
//	Symbol *getSymbolByIndex(unsigned);
//
//	/***** Private Member Functions *****/
//	private:
//
//	Symtab(std::string filename, bool defensive_bin, bool &err);
//
//	bool extractInfo(Object *linkedFile);
//
//	// Parsing code
//
//	bool extractSymbolsFromFile(Object *linkedFile, std::vector<Symbol *> &raw_syms);
//
//	bool fixSymRegion(Symbol *sym);
//
//	bool fixSymModules(std::vector<Symbol *> &raw_syms);
//	bool demangleSymbols(std::vector<Symbol *> &rawsyms);
//	bool createIndices(std::vector<Symbol *> &raw_syms, bool undefined);
//	bool createAggregates();
//
//	bool fixSymModule(Symbol *&sym);
//	bool demangleSymbol(Symbol *&sym);
//	bool updateIndices(Symbol *sym, std::string newName, NameType nameType);
//
//
//	void setModuleLanguages(dyn_hash_map<std::string, supportedLanguages> *mod_langs);
//
//
//
//	bool addFunctionRange(FunctionBase *fbase, Dyninst::Offset next_start);
//
//	// Used by binaryEdit.C...
// public:
//
//
//	bool canBeShared();
//	Module *getOrCreateModule(const std::string &modName, 
//														 const Offset modAddr);
//	bool parseFunctionRanges();
//
//	//Only valid on ELF formats
//	Offset getElfDynamicOffset();
//	// SymReader interface
//	void getSegmentsSymReader(std::vector<SymSegment> &segs);
//	void rebase(Offset offset);
//
// private:
//
//	Module *newModule(const std::string &name, const Offset addr, supportedLanguages lang);
//	
//	//bool buildFunctionLists(std::vector <Symbol *> &raw_funcs);
//	//void enterFunctionInTables(Symbol *func, bool wasSymtab);
//
//
//	bool addSymtabVariables();
//
//	void checkPPC64DescriptorSymbols(Object *linkedFile);
//
//
//	void parseLineInformation();
//	
//
//	bool addUserRegion(Region *newreg);
//	bool addUserType(Type *newtypeg);
//
//	void setTOCOffset(Offset offset);
//	//Don't use obj_private, use getObject() instead.
// public:
//	void dumpModRanges();
//	void dumpFuncRanges();

};

};
};



#endif /* __SYMTAB_SYMTAB_H__ */
