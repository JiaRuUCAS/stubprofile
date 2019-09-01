/*
 * See the dyninst/COPYRIGHT file for copyright information.
 * 
 * We provide the Paradyn Tools (below described as "Paradyn")
 * on an AS IS basis, and do not warrant its validity or performance.
 * We reserve the right to update, modify, or discontinue this
 * software at any time.	We shall have no obligation to supply such
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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	See the GNU
 * Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

#ifndef _Collections_h_
#define _Collections_h_

#include "common/race-detector-annotations.h"
#include <tbb/concurrent_hash_map.h>
#include "Type.h"
#include "Variable.h"
#include <boost/thread/mutex.hpp>
#include <boost/thread/lock_guard.hpp>

namespace Dyninst {

namespace SymtabAPI {

	extern tbb::concurrent_hash_map<void *, size_t> type_memory;

class Module;
class Symtab;
class localVar;

/*
 * This class contains a collection of local variables.
 * Each function will have one of these objects associated with it.
 * This object will store all the local variables within this function.
 * Note: This class is unaware of scope.
 */


class SYMTAB_EXPORT localVarCollection {

	tbb::concurrent_vector<localVar* > localVars;

	bool addItem_impl(localVar *);
public:
	localVarCollection(){}
	~localVarCollection();

	void addLocalVar(localVar * var);
	localVar * findLocalVar(std::string &name);
	const tbb::concurrent_vector<localVar *> &getAllVars() const;

	bool addItem(localVar *var) { return addItem_impl(var); }
};


/*
 * Due to DWARF weirdness, this can be shared between multiple BPatch_modules.
 * So we reference-count to make life easier.
 */
class SYMTAB_EXPORT typeCollection //: public AnnotatableSparse 
{
	friend class Symtab;
	friend class Object;
	friend class Module;
	friend class Type;

	tbb::concurrent_hash_map<std::string, Type *> typesByName;
	tbb::concurrent_hash_map<std::string, Type *> globalVarsByName;
	tbb::concurrent_hash_map<int, Type *> typesByID;

	static dyn_hash_map<void *, typeCollection *> fileToTypesMap;

	public:
	typeCollection();
	~typeCollection();
	static boost::mutex create_lock;

	static typeCollection *getModTypeCollection(Module *mod);

	Type *findType(std::string name);
	Type *findType(const int ID);
	Type *findTypeLocal(std::string name);
	Type *findTypeLocal(const int ID);
	void addType(Type *type);
	void addType(Type *type, boost::lock_guard<boost::mutex>&);
	void addGlobalVariable(std::string &name, Type *type);

	/* Some debug formats allow forward references.	Rather than
	 * fill in forward in a second pass, generate placeholder
	 * types, and fill them in as we go.	Because we require
	 * One True Pointer for each type (in parseStab.C), when
	 * updating a type, return that One True Pointer.
	 */
	Type * findOrCreateType( const int ID );

	template<class T>
	T *upgradePlaceholder(Type *placeholder, T *new_type)
	{
		void *mem = (void *) placeholder;
		tbb::concurrent_hash_map<void*, size_t>::accessor a;
	
		assert(type_memory.find(a, placeholder));
	
		size_t size = a->second;
	
		assert(sizeof(T) < size);
		memset(mem, 0, size);
	
		T *ret = new(mem) T();
	
		assert(mem == (void *) ret);
		*ret = *new_type;
		race_detector_forget_access_history(ret, sizeof(T));
		return ret;
	}

	template<class T>
	T* addOrUpdateType(T* type)
	{
		//Instanciating this function for 'Type' would be a mistake, which
		//the following assert tries to guard against.  If you trigger this,
		//then a caller to this function is likely using 'Type'.  Change
		//this to a more specific call, e.g. typeFunction instead of Type
		BOOST_STATIC_ASSERT(sizeof(T) != sizeof(Type));
		boost::lock_guard<boost::mutex> g(placeholder_mutex);
	
		Type *existingType = findTypeLocal(type->getID());
		tbb::concurrent_hash_map<int, Type*>::accessor id_accessor;
		tbb::concurrent_hash_map<std::string, Type*>::accessor name_accessor;
	
		if (!existingType) {
			if (type->getName() != "")
				typesByName.insert(name_accessor, std::make_pair(type->getName(), type));
			typesByID.insert(id_accessor, std::make_pair(type->getID(), type));
			type->incrRefCount();
			return type;
		}
	
		/* Multiple inclusions of the same object file can result
		   in us parsing the same module types repeatedly. GCC does this
		   with some of its internal routines */
	
		T *existingT = dynamic_cast<T*>(existingType);
	
		if (existingT && (*existingT == *type)) {
			return (T*) existingType;
		}
	
		if (existingType->getDataClass() == dataUnknownType) {
			upgradePlaceholder(existingType, type);
		} else {
			/* Merge the type information. */
			existingType->merge(type);
		}
	
		/* The type may have gained a name. */
		if ( existingType->getName() != "") {
			tbb::concurrent_hash_map<std::string, Type*>::accessor a;
			bool found = typesByName.find(a, existingType->getName());
	
			if (found) {
				if (a->second != existingType) {
					a->second->decrRefCount();
					a->second = existingType;
					existingType->incrRefCount();
				}
			} else {
				typesByName.insert(a, std::make_pair(existingType->getName(), existingType));
				existingType->incrRefCount();
			}
		}
	
		/* Tell the parser to update its type pointer. */
		return (T*) existingType;
	} /* end addOrUpdateType() */

	Type *findVariableType(std::string &name);

	std::vector<Type *> *getAllTypes();
	std::vector<std::pair<std::string, Type *> > *getAllGlobalVariables();
	void clearNumberedTypes();

 private:
	/* The only intermodule contention should be around typedefs/other
	 * placeholders, but we'll go ahead and lock around type add operations
	 * to be safe.
	 */
	boost::mutex placeholder_mutex;
};

/*
 * This class defines the collection for the built-in Types
 * gnu use negative numbers to define other types
 * in terms of these built-in types.
 * This collection is global and built in the BPatch_image constructor.
 * This means that only one collection of built-in types is made
 * per image.	jdd 4/21/99
 *
 */

class SYMTAB_EXPORT builtInTypeCollection {
	 
	dyn_hash_map<std::string, Type *> builtInTypesByName;
	dyn_hash_map<int, Type *> builtInTypesByID;
public:

	builtInTypeCollection();
	~builtInTypeCollection();

	Type	*findBuiltInType(std::string &name);
	Type	*findBuiltInType(const int ID);
	void	addBuiltInType(Type *type);
	std::vector<Type *> *getAllBuiltInTypes();

};

}// namespace SymtabAPI
}// namespace Dyninst

#endif /* _Collections_h_ */



