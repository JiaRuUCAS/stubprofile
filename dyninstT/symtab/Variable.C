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

// $Id: Object.C,v 1.31 2008/11/03 15:19:25 jaw Exp $

#include "common/Annotatable.h"

#include "Symtab.h"
#include "symutil.h"
#include "Module.h"
#include "Collections.h"
#include "Variable.h"
#include "Aggregate.h"
#include "Function.h"
#include <iterator>

//#include "symtabAPI/src/Object.h"

#include <iostream>

using namespace std;
using namespace Dyninst;
using namespace Dyninst::SymtabAPI;

Variable::Variable(Symbol *sym) :
	Aggregate(sym),
	type_(NULL)
{
}

Variable::Variable() :
	Aggregate(),
	type_(NULL)
{
}
void Variable::setType(Type *type)
{
	type_ = type;
}

Type* Variable::getType()
{
	module_->exec()->parseTypesNow();
	return type_;
}

std::ostream &operator<<(std::ostream &os, const Dyninst::SymtabAPI::Variable &v)
{
	Type *var_t = (const_cast<Variable &>(v)).getType();
	std::string tname(var_t ? var_t->getName() : "no_type");
	const Aggregate *ag = dynamic_cast<const Aggregate *>(&v);
	assert(ag);

	os  << "Variable{"		
		<< " type=" 
		<< tname
		<< " ";
	os  << 	*ag;						
	os  << 	"}";

	return os;
}
bool Variable::operator==(const Variable &v)
{
	if (type_ && !v.type_)
		return false;
	if (!type_ && v.type_)
		return false;
	if (type_) {
		if (type_->getID() != v.type_->getID())
			return false;
	}

	return ((Aggregate &)(*this)) == ((const Aggregate &)v);
}

bool Variable::removeSymbol(Symbol *sym) 
{
	removeSymbolInt(sym);
	if (symbols_.empty()) {
		module_->exec()->deleteVariable(this);
	}
	return true;
}

localVar::localVar(std::string name,  Type *typ, std::string fileName, 
		int lineNum, FunctionBase *f, std::vector<VariableLocation> *locs) :
	name_(name), 
	type_(typ), 
	fileName_(fileName), 
	lineNum_(lineNum),
	func_(f)
{
	type_->incrRefCount();

	if (locs)
		std::copy(locs->begin(), locs->end(), std::back_inserter(locs_));
}

localVar::localVar(localVar &lvar)
{
	name_ = lvar.name_;
	type_ = lvar.type_;
	fileName_ = lvar.fileName_;
	lineNum_ = lvar.lineNum_;
	func_ = lvar.func_;

	std::copy(lvar.locs_.begin(), lvar.locs_.end(),
			std::back_inserter(locs_));

	if (type_ != NULL)
		type_->incrRefCount();
}

bool localVar::addLocation(const VariableLocation &location)
{
	locs_.push_back(location);
	return true;
}

localVar::~localVar()
{
	//XXX jdd 5/25/99 More to do later
	type_->decrRefCount();
}

void localVar::fixupUnknown(Module *module) 
{
	if (type_->getDataClass() == dataUnknownType) 
	{
		Type *otype = type_;
		typeCollection *tc = typeCollection::getModTypeCollection(module);

		assert(tc);
		type_ = tc->findType(type_->getID());

		if (type_) {
			type_->incrRefCount();
			otype->decrRefCount();
		}
		else
			type_ = otype;
	}
}

std::string &localVar::getName() 
{
	return name_;
}

Type *localVar::getType()
{
	return type_;
}

bool localVar::setType(Type *newType) 
{
	type_ = newType;
	return true;
}

int localVar::getLineNum() 
{
	return lineNum_;
}

std::string &localVar::getFileName() 
{
	return fileName_;
}

std::vector<Dyninst::VariableLocation> &localVar::getLocationLists() 
{
	return locs_;
}

bool localVar::operator==(const localVar &l)
{
	if (type_ && !l.type_) return false;
	if (!type_ && l.type_) return false;

	if (type_) {
		if (type_->getID() != l.type_->getID())
			return false;
	}

	if (name_ != l.name_) return false;
	if (fileName_ != l.fileName_) return false;
	if (lineNum_ != l.lineNum_) return false;
	if (locs_ != l.locs_) return false;

	return true;
}
