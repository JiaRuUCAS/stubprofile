/*
 * See the dyninst/COPYRIGHT file for copyright information.
 * 
 * We provide the Paradyn Tools (below described as "Paradyn")
 * on an AS IS basis, and do not warrant its validity or performance.
 * We reserve the right to update, modify, or discontinue this
 * software at any time.  We shall have no obligation to supply such
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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

#if !defined(__SYMTAB_VARIABLE_H__)
#define __SYMTAB_VARIABLE_H__

#include "common/Annotatable.h"
#include "common/dyn_regs.h"
#include "common/VariableLocation.h"

#include "Symtab.h"
#include "Aggregate.h"

SYMTAB_EXPORT std::ostream &operator<<(std::ostream &os, const Dyninst::SymtabAPI::Variable &);

namespace Dyninst {
namespace SymtabAPI {


class Symbol;
class Symtab;
class Aggregate;
class Function;
class FunctionBase;

/* The Variable represents a collection of symbols that have the same address and represent data */
class SYMTAB_EXPORT Variable : public Aggregate, public AnnotatableSparse {
	friend class Symtab;
	friend std::ostream &::operator<<(std::ostream &os, const Dyninst::SymtabAPI::Variable &);

 private:
	Variable(Symbol *sym);
	static Variable *createVariable(Symbol *sym);
	
 public:
	Variable();

	/* Symbol management */
	bool removeSymbol(Symbol *sym);		

	void setType(Type *type);
	Type *getType();

	bool operator==(const Variable &v);

 private:

	Type *type_;
};

/* localVar represents a local variable or parameter of a function */
class SYMTAB_EXPORT localVar : public AnnotatableSparse
{
	friend class typeCommon;
	friend class localVarCollection;

	std::string name_;
	Type *type_;
	/* File where the variable was declared, if known */
	std::string fileName_;
	/* Line number where the variable was declared, if known */
	int lineNum_;
	FunctionBase *func_;
	/* A local variable can be inscope at different positions and based
	 * on its accessible on different ways. Location list provides a way
	 * to encode this information. */
	std::vector<VariableLocation> locs_;


 public:
	localVar() : type_(NULL), lineNum_(-1), func_(NULL) {}

	//  Internal use only
	localVar(std::string name,  Type *typ, std::string fileName, 
				int lineNum, FunctionBase *f, 
				std::vector<VariableLocation> *locs = NULL);
				
	// Copy constructor
	localVar(localVar &lvar);

	bool addLocation(const VariableLocation &location);
	~localVar();
	void fixupUnknown(Module *);

 public:
	//  end of functions for internal use only
	std::string &getName();
	Type *getType();
	bool setType(Type *newType);
	int  getLineNum();
	std::string &getFileName();
	std::vector<VariableLocation> &getLocationLists();
	bool operator==(const localVar &l);
};

}
}

#endif
