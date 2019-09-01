#ifndef __SYMTAB_REGION_H__
#define __SYMTAB_REGION_H__

#include "common/Serialization.h"
#include "common/Annotatable.h"
#include "symutil.h"


namespace Dyninst {
namespace SymtabAPI {

class Symbol;
class relocationEntry;
class Symtab;

/* This class represents a contiguous range of code or data as encoded
 * in the object file. For ELF, Region represents a section. */
class SYMTAB_EXPORT Region : public AnnotatableSparse {
	friend class Symtab;

	public:

		/* permission of the range */
		enum perm_t {
			/* Read-only data */
			RP_R,
			/* Read/write data */
			RP_RW,
			/* Read-only code */
			RP_RX,
			/* Read/write code */
			RP_RWX,
		};

		static const char *permissions2Str(perm_t);

		enum RegionType {
			/* Executable code */
			RT_TEXT,
			/* Read/write data */
			RT_DATA,
			/* Mix of code and data */
			RT_TEXTDATA,
			/* Static symbol table */
			RT_SYMTAB,
			/* String table used by symbol table */
			RT_STRTAB,
			/* 0-initialized memory */
			RT_BSS,
			/* Version information for symbols */
			RT_SYMVERSIONS,
			RT_SYMVERDEF,
			RT_SYMVERNEEDED,
			/* `Rel` relocation section */
			RT_REL,
			/* `Rela` relocation section */
			RT_RELA,
			/* `Rel` relocation section for PLT (inter-libraries references)
			 * entries */
			RT_PLTREL,
			/* `Rela` relocation section for PLT (inter-libraries
			 * references) entries */
			RT_PLTRELA,
			/* Description of library dependencies */
			RT_DYNAMIC,
			/* Fast symbol lookup section */
			RT_HASH,
			/* GNU-specific fast symbol lookup section */
			RT_GNU_HASH,
			/* Other */
			RT_OTHER,
			RT_INVALID = -1,
		};

		static const char *regionType2Str(RegionType);

		Region();
		static Region *createRegion(Offset diskOff,
						perm_t perms,
						RegionType regType,
						unsigned long diskSize = 0,
						Offset memOff = 0,
						unsigned long memSize = 0,
						std::string name = "",
						char *rawDataPtr = NULL,
						bool isLoadable = false,
						bool isTLS = false,
						unsigned long memAlign = sizeof(unsigned));
		Region(const Region &reg);
		Region& operator=(const Region &reg);
		std::ostream& operator<< (std::ostream &os);
		bool operator== (const Region &reg);

		~Region();

		unsigned getRegionNumber() const;
		bool setRegionNumber(unsigned regnumber);
		std::string getRegionName() const;

		Offset getDiskOffset() const;
		unsigned long getDiskSize() const;
		unsigned long getFileOffset();

		Offset getMemOffset() const;
		unsigned long getMemSize() const;
		unsigned long getMemAlignment() const;
		void setMemOffset(Offset);
		void setMemSize(unsigned long);
		void setDiskSize(unsigned long);
		void setFileOffset(Offset);

		void *getPtrToRawData() const;
		bool setPtrToRawData(void *, unsigned long);//also sets diskSize

		bool isBSS() const;
		bool isText() const;
		bool isData() const;
		bool isTLS() const;
		bool isOffsetInRegion(const Offset &offset) const;
		bool isLoadable() const;
		bool setLoadable(bool isLoadable);
		bool isDirty() const;
		std::vector<relocationEntry> &getRelocations();
		bool patchData(Offset off, void *buf, unsigned size);
		bool isStandardCode();

		perm_t getRegionPermissions() const;
		bool setRegionPermissions(perm_t newPerms);
		RegionType getRegionType() const;

		bool addRelocationEntry(Offset relocationAddr, Symbol *dynref,
						unsigned long relType,
						Region::RegionType rtype = Region::RT_REL);
		bool addRelocationEntry(const relocationEntry& rel);

		bool updateRelocations(Address start, Address end,
						Symbol *oldsym, Symbol *newsym);

		Symtab *symtab() const { return symtab_; }

	protected:							
		Region(unsigned regnum,
						std::string name,
						Offset diskOff,
						unsigned long diskSize,
						Offset memOff,
						unsigned long memSize,
						char *rawDataPtr,
						perm_t perms,
						RegionType regType,
						bool isLoadable = false,
						bool isTLS = false,
						unsigned long memAlign = sizeof(unsigned));
		void setSymtab(Symtab *sym) { symtab_ = sym; }
	private:
		/* region number */
		unsigned regNum_;
		/* region name */
		std::string name_;
		Offset diskOff_;
		unsigned long diskSize_;
		Offset memOff_;
		unsigned long memSize_;
		Offset fileOff_;
		void *rawDataPtr_;
		perm_t permissions_;
		RegionType rType_;
		bool isDirty_;
		std::vector<relocationEntry> rels_;
		char *buffer_;  //To hold dirty data
		bool isLoadable_;
		bool isTLS_;
		unsigned long memAlign_;
		Symtab *symtab_;
};

}	/* SymtabAPI */
}	/* Dyninst */

#endif /* __SYMTAB_REGION_H__ */
