#ifndef __SYMTAB_EXCEPTION_H__
#define __SYMTAB_EXCEPTION_H__

#include "symutil.h"
#include "common/Serialization.h"
#include "common/Annotatable.h"


namespace Dyninst {
namespace SymtabAPI {
/**
 * Used to represent something like a C++ try/catch block.  
 * Currently only used on Linux
 **/
class SYMTAB_EXPORT ExceptionBlock : public Serializable,
		public AnnotatableSparse
{
	// Accessors provide consistent access to the *original* offsets.
	// We allow this to be updated (e.g. to account for relocated code
	public:
		Serializable * serialize_impl(SerializerBase *sb, 
						const char *tag = "exceptionBlock") THROW_SPEC (SerializerError);
		ExceptionBlock(Offset tStart, unsigned tSize, Offset cStart);
		ExceptionBlock(Offset cStart);
		ExceptionBlock(const ExceptionBlock &eb);
		ExceptionBlock();
		~ExceptionBlock();

		bool hasTry() const;
		Offset tryStart() const;
		Offset tryEnd() const;
		Offset trySize() const;
		Offset catchStart() const;
		bool contains(Offset a) const;

		void setTryStart(Offset ts) {
			tryStart_ptr = ts;
		}

		void setTryEnd(Offset te) {
			tryEnd_ptr = te;
		}

		void setCatchStart(Offset cs) {
			catchStart_ptr = cs;
		}

		void setFdeStart(Offset fs) {
			fdeStart_ptr = fs;
		}

		void setFdeEnd(Offset fe) {
			fdeEnd_ptr = fe;
		}

		friend SYMTAB_EXPORT std::ostream &operator<<(
						std::ostream &os, const ExceptionBlock &q);

	private:
		Offset tryStart_;
		unsigned trySize_;
		Offset catchStart_;
		bool hasTry_;
		Offset tryStart_ptr;
		Offset tryEnd_ptr;
		Offset catchStart_ptr;
		Offset fdeStart_ptr;
		Offset fdeEnd_ptr;
};

SYMTAB_EXPORT std::ostream &operator<<(std::ostream &os,
				const ExceptionBlock &q);

} // namespace SymtabAPI
} // namespace Dyninst

#endif
