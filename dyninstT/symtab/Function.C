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

#include "Symtab.h"
#include "symutil.h"
#include "Module.h"
#include "Collections.h"
#include "Function.h"
#include "common/VariableLocation.h"
//#include "Object.h"

#include <iterator>
#include <algorithm>
#include <iostream>

using namespace std;
using namespace Dyninst;
using namespace Dyninst::SymtabAPI;

FunctionBase::FunctionBase() :
	locals(NULL),
	params(NULL),
	functionSize_(0),
	retType_(NULL),
	data(NULL)
{
}

Type *FunctionBase::getReturnType() const
{
	getModule()->exec()->parseTypesNow();	
	return retType_;
}

bool FunctionBase::setReturnType(Type *newType)
{
	retType_ = newType;
	return true;
}

bool FunctionBase::findLocalVariable(std::vector<localVar *> &vars,
				std::string name)
{
	getModule()->exec()->parseTypesNow();	

	unsigned origSize = vars.size();	

	if (locals) {
		localVar *var = locals->findLocalVar(name);
		if (var)
			vars.push_back(var);
	}

	if (params) {
		localVar *var = params->findLocalVar(name);
		if (var) 
			vars.push_back(var);
	}

	if (vars.size() > origSize)
		return true;

	return false;
}

const FuncRangeCollection &FunctionBase::getRanges()
{
	return ranges;
}

bool FunctionBase::getLocalVariables(std::vector<localVar *> &vars)
{
	getModule()->exec()->parseTypesNow();	
	if (!locals)
		return false;

	auto p = locals->getAllVars();

	std::copy(p.begin(), p.end(), back_inserter(vars));

	if (p.empty())
		return false;
	return true;
}

bool FunctionBase::getParams(std::vector<localVar *> &params_)
{
	getModule()->exec()->parseTypesNow();
	if (!params)
		return false;

	auto p = params->getAllVars();

	std::copy(p.begin(), p.end(), back_inserter(params_));

	if (p.empty())
		return false;
	return true;
}

bool FunctionBase::addLocalVar(localVar *locVar)
{
	if (!locals) {
		locals = new localVarCollection();
	}

	locals->addLocalVar(locVar);
	return true;
}

bool FunctionBase::addParam(localVar *param)
{
	if (!params) {
		params = new localVarCollection();
	}
	params->addLocalVar(param);
	return true;
}

FunctionBase::~FunctionBase()
{
	if (locals) {
		delete locals;
		locals = NULL;
	}
	if (params) {
		delete params;
		params = NULL;
	}
}

void *FunctionBase::getData()
{
	return data;
}

void FunctionBase::setData(void *d)
{
	data = d;
}

Function::Function(Symbol *sym)
	: FunctionBase(), Aggregate(sym)
{}

Function::Function()
	: FunctionBase()
{}

Function::~Function()
{
}

bool Function::removeSymbol(Symbol *sym) 
{
	removeSymbolInt(sym);
	if (symbols_.empty()) {
		getModule()->exec()->deleteFunction(this);
	}
	return true;
}

std::ostream &operator<<(std::ostream &os, const Dyninst::VariableLocation &l)
{
	const char *stc = storageClass2Str(l.stClass);
	const char *strc = storageRefClass2Str(l.refClass);
	os << "{"
			<< "storageClass=" << stc
			<< " storageRefClass=" << strc
			<< " reg=" << l.mr_reg.name() 
			<< " frameOffset=" << l.frameOffset
			<< " lowPC=" << l.lowPC
			<< " hiPC=" << l.hiPC
			<< "}";
	return os;
}

std::ostream &operator<<(std::ostream &os, const Dyninst::SymtabAPI::Function &f)
{
	Type *retType = (const_cast<Function &>(f)).getReturnType();

	std::string tname(retType ? retType->getName() : "no_type");
	const Aggregate *ag = dynamic_cast<const Aggregate *>(&f);
	assert(ag);

	os  << "Function{"
		<< " type=" << tname
		<< " FramePtrLocationList=[";
	os  << "] ";
	os  <<  *ag;
	os  <<  "}";
	return os;

}

std::string Function::getName() const
{
	return getFirstSymbol()->getMangledName();
}

bool FunctionBase::operator==(const FunctionBase &f)
{
	if (retType_ && !f.retType_)
		return false;
	if (!retType_ && f.retType_)
		return false;
	if (retType_) {
		if (retType_->getID() != f.retType_->getID())
			return false;
	}

	return ((Aggregate &)(*this)) == ((const Aggregate &)f);
}

Module* Function::getModule() const {
	return module_;
}
