#ifndef __SYMTAB_SYMTAB_H__
#define __SYMTAB_SYMTAB_H__

#include <set>

#include "Region.h"
#include "Symbol.h"

#include "common/Annotatable.h"
#include "common/Serialization.h"
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

namespace Dyninst {
	struct SymSegment;

namespace SymtabAPI {

class Type;
class ExceptionBlock;
//class typeCollection;

typedef Dyninst::ProcessReader MemRegReader;

class SYMTAB_EXPORT Symtab : public LookupInterface,
		public Serializable,
		public AnnotatableSparse
{
	friend class Symbol;
	friend class Region;
	friend class relocationEntry;
	friend class ExceptionBlock;

	public:

	private:
		pfq_rwlock_t symbols_rwlock;

		std::string member_name_;
		Offset member_offset_;

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

		std::vector<relocationEntry> relocation_table_;
		std::vector<ExceptionBlock *> excpBlocks;
		std::vector<std::string> deps_;

		bool isTypeInfoValid_;

		// relocation sections
		bool hasRel_;
		bool hasRela_;
		bool hasReldyn_;
		bool hasReladyn_;
		bool hasRelplt_;
		bool hasRelaplt_;

		bool isStaticBinary_;
		bool isDefensiveBinary_;

		unsigned _ref_cnt;

	public:
		typedef enum {
			NotDefensive,
			Defensive
		} def_t;

		Symtab();
		Symtab(const Symtab &obj);
		Symtab(unsigned char *mem_image, size_t image_size,
						const std::string &name,
						bool defensive_binary, bool &err);
		~Symtab();


		/********* Type Information ********/
		bool addType(Type *typ);
//		static boost::shared_ptr<typeCollection> stdTypes();

		static boost::shared_ptr<Type> type_Error();
		static boost::shared_ptr<Type> type_Untyped();

		/********* Error Handling *********/
		static SymtabError getLastSymtabError();
		static void setSymtabError(SymtabError new_err);
		static std::string printError(SymtabError serr);

	private:
//		static boost::shared_ptr<typeCollection> setupStdTypes();

		bool addUserType(Type *);
};

};
};



#endif /* __SYMTAB_SYMTAB_H__ */
