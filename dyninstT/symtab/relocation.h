#ifndef __SYMTAB_RELOCATION_H__
#define __SYMTAB_RELOCATION_H__

#include "symutil.h"
#include "Region.h"

/* Relocation Section
 *
 * Relocation is the process of connecting symbolic references with
 * symbolic definitions. For example, when a program calls a function,
 * the associated call instruction must transfer control to the proper
 * destination address at execution. Relocatable files must have
 * information that describes how to modify their section contents.
 *
 * Relocation entry can have the following structure.
 * typedef struct {
 *	  Elf32_Addr		r_offset;
 *	  Elf32_Word		r_info;
 * } Elf32_Rel;
 *
 * typedef struct {
 *	  Elf32_Addr		r_offset;
 *	  Elf32_Word		r_info;
 *	  Elf32_Sword	  r_addend;
 * } Elf32_Rela;
 *
 * typedef struct {
 *	  Elf64_Addr		r_offset;
 *	  Elf64_Xword	  r_info;
 * } Elf64_Rel;
 *
 * typedef struct {
 *	  Elf64_Addr		r_offset;
 *	  Elf64_Xword	  r_info;
 *	  Elf64_Sxword	 r_addend;
 * } Elf64_Rela;
 *
 * - `r_offset` gives the location at which to apply the relocation action.
 * 		- For a relocatable file, the value indicates a section offset. It's
 *		  a storage unit within the section that need to be modified.
 *		- For an execuable ot shared object, the value indicates the virtual
 *		  address of the storage unit that need to be relocated.
 * - `r_info` contains two informations
 *		- The symbol table index, that points to the symbol the relocation is
 *		  made to. For example, a call instruction's relocation entry holds
 *		  the symbol table index of the function being called.
 *		- Relocation type, which is processor-specific.
 * - `r_addend` specifies a constant addend used to compute the value to be
 *	stored into the relocatable field.
 *		- 64-bit x86 use only Rela relocation entries, while x86 uses only Rel
 *		  relocation entries.
 *
 */

namespace Dyninst {
namespace SymtabAPI {

class Symbol;
class Region;

/* Represents a relocation entry */
class SYMTAB_EXPORT relocationEntry :
		public Serializable, public AnnotatableSparse {
	
	public:

		relocationEntry();
		relocationEntry(Offset ta, Offset ra, Offset add,
						std::string n, Symbol *dynref = NULL,
						unsigned long relType = 0);
		relocationEntry(Offset ta, Offset ra, std::string n,
						Symbol *dynref = NULL, unsigned long relType = 0);
		relocationEntry(Offset ra, std::string n, Symbol *dynref = NULL,
						unsigned long relType = 0,
						Region::RegionType rtype = Region::RT_REL);
		relocationEntry(Offset ta, Offset ra, Offset add, std::string n,
						Symbol *dynref = NULL, unsigned long relType = 0,
						Region::RegionType rtype = Region::RT_REL);

		Serializable * serialize_impl(SerializerBase *sb,
						const char *tag = "relocationEntry")
				THROW_SPEC (SerializerError);

		Offset target_addr() const;
		Offset rel_addr() const;
		Offset addend() const;
		Region::RegionType regionType() const;
		const std::string &name() const;
		Symbol *getDynSym() const;
		bool addDynSym(Symbol *dynref);
		unsigned long getRelType() const;

		void setTargetAddr(const Offset);
		void setRelAddr(const Offset);
		void setAddend(const Offset);
		void setRegionType(const Region::RegionType);
		void setName(const std::string &newName);
		void setRelType(unsigned long relType) { relType_ = relType; }

		friend SYMTAB_EXPORT std::ostream & operator<<(std::ostream &os, const relocationEntry &q);

		enum {pltrel = 1, dynrel = 2};
		bool operator==(const relocationEntry &) const;

		enum category { relative, jump_slot, absolute };

		// Architecture-specific functions
		static unsigned long getGlobalRelType(unsigned addressWidth, Symbol *sym = NULL);
		static const char *relType2Str(unsigned long r, unsigned addressWidth = sizeof(Address));
		category getCategory( unsigned addressWidth );

	private:
		Offset target_addr_;	// target address of call instruction 
		Offset rel_addr_;		// address of corresponding relocation entry 
		Offset addend_;		 // addend (from RELA entries)
		Region::RegionType rtype_;		  // RT_REL vs. RT_RELA
		std::string  name_;
		Symbol *dynref_;
		unsigned long relType_;
		Offset rel_struct_addr_;
};	/* class relocationEntry */

SYMTAB_EXPORT std::ostream &operator<<(std::ostream &os,
				const relocationEntry &q);

}	/* symtabAPI */
}	/* Dyninst */

#endif /* __SYMTAB_RELOCATION_H__ */
