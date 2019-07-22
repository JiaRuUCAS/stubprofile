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

#define DYN_DEFINE_REGS
#include "common/dyn_regs.h"

#include "common/rose-compat.h"

#include <iostream>

using namespace Dyninst;

boost::shared_ptr<MachRegister::NameMap> MachRegister::names()
{
    static boost::shared_ptr<MachRegister::NameMap> store =
       boost::shared_ptr<MachRegister::NameMap>(new MachRegister::NameMap);
    return store;
}

MachRegister::MachRegister() :
   reg(0)
{
}

MachRegister::MachRegister(signed int r) :
   reg(r)
{
}

MachRegister::MachRegister(signed int r, const char *n) :
   reg(r)
{
	(*names())[r] = std::string(n);
}

MachRegister::MachRegister(signed int r, std::string n) :
reg(r)
{
	(*names())[r] = n;
}

unsigned int MachRegister::regClass() const
{
    return reg & 0x00ff0000;
}

MachRegister MachRegister::getBaseRegister() const {
   signed int category = (reg & 0x00ff0000);
   switch (getArchitecture()) {
      case Arch_x86:
         if (category == x86::GPR) return MachRegister(reg & 0xfffff0ff);
         else if (category == x86::FLAG) return x86::flags;
         else return *this;
      case Arch_x86_64:
         if (category == x86_64::GPR) return MachRegister(reg & 0xfffff0ff);
         else if (category == x86_64::FLAG) return x86_64::flags;
         else return *this;
	  default:
		 return InvalidReg;
   }
   return InvalidReg;
}

Architecture MachRegister::getArchitecture() const {
   return (Architecture) (reg & 0xff000000);
}

bool MachRegister::isValid() const {
   return (reg != InvalidReg.reg);
}

MachRegisterVal MachRegister::getSubRegValue(const MachRegister& subreg,
                                             MachRegisterVal &orig) const
{
   if (subreg.reg == reg)
      return orig;

   assert(subreg.getBaseRegister() == getBaseRegister());
   switch ((subreg.reg & 0x00000f00) >> 8) {
      case 0x0: return orig;
      case 0x1: return (orig & 0xff);
      case 0x2: return (orig & 0xff00) >> 8;
      case 0x3: return (orig & 0xffff);
      case 0xf: return (orig & 0xffffffff);
      default: assert(0); return orig;
   }
}

std::string MachRegister::name() const {
	assert(names() != NULL);
	NameMap::const_iterator iter = names()->find(reg);
	if (iter != names()->end()) {
		return iter->second;
	}
	return std::string("<INVALID_REG>");
}

unsigned int MachRegister::size() const {
   switch (getArchitecture())
   {
      case Arch_x86:
         switch (reg & 0x0000ff00) {
            case x86::L_REG: //L_REG
            case x86::H_REG: //H_REG
               return 1;
            case x86::W_REG: //W_REG
               return 2;
            case x86::FULL: //FULL
               return 4;
            // Commented out because no register
            // is defined with this size type
            //case x86::QUAD:
            //   return 8;
            case x86::OCT:
               return 16;
            case x86::FPDBL:
               return 10;
            case x86::BIT:
               return 0;
            case x86::YMMS:
               return 32;
            case x86::ZMMS:
               return 64;
            default:
               return 0;//KEVINTODO: removed sanity-check assert because of asprotect fuzz testing, could use this as a sign that the parse has gone into junk
               assert(0);
         }
      case Arch_x86_64:
         switch (reg & 0x0000ff00) {
            case x86_64::L_REG: //L_REG
            case x86_64::H_REG: //H_REG
                return 1;
            case x86_64::W_REG: //W_REG
                return 2;
            case x86_64::FULL: //FULL
                return 8;
            case x86_64::D_REG:
               return 4;
            case x86_64::OCT:
               return 16;
            case x86_64::FPDBL:
               return 10;
            case x86_64::BIT:
               return 0;
            case x86_64::YMMS:
               return 32;
            case x86_64::ZMMS:
               return 64;

            default:
	       return 0; // Xiaozhu: do not assert, but return 0 as an indication of parsing junk.
               assert(0);
         }
      case Arch_none:
         return 0;
   }
   return 0; //Unreachable, but disable warnings
}

bool MachRegister::operator<(const MachRegister &a) const {
   return (reg < a.reg);
}

bool MachRegister::operator==(const MachRegister &a) const {
   return (reg == a.reg);
}

MachRegister::operator signed int() const {
   return reg;
}

signed int MachRegister::val() const {
   return reg;
}


MachRegister MachRegister::getPC(Dyninst::Architecture arch)
{
   switch (arch)
   {
      case Arch_x86:
         return x86::eip;
      case Arch_x86_64:
         return x86_64::rip;
      case Arch_none:
         return InvalidReg;
   }
   return InvalidReg;
}


MachRegister MachRegister::getReturnAddress(Dyninst::Architecture arch)
{
   switch (arch)
   {
      case Arch_x86:
          assert(0); //not implemented
      case Arch_x86_64:
          assert(0); //not implemented
      case Arch_none:
         return InvalidReg;
   }
   return InvalidReg;
}

MachRegister MachRegister::getFramePointer(Dyninst::Architecture arch)
{
   switch (arch)
   {
      case Arch_x86:
         return x86::ebp;
      case Arch_x86_64:
         return x86_64::rbp;
      case Arch_none:
         return InvalidReg;
      default:
         assert(0);
         return InvalidReg;
   }
   return InvalidReg;
}

MachRegister MachRegister::getStackPointer(Dyninst::Architecture arch)
{
   switch (arch)
   {
      case Arch_x86:
         return x86::esp;
      case Arch_x86_64:
         return x86_64::rsp;
      case Arch_none:
         return InvalidReg;
      default:
         assert(0);
         return InvalidReg;
   }
   return InvalidReg;
}

MachRegister MachRegister::getSyscallNumberReg(Dyninst::Architecture arch)
{
    switch (arch)
    {
        case Arch_x86:
            return x86::eax;
        case Arch_x86_64:
            return x86_64::rax;
        case Arch_none:
            return InvalidReg;
      default:
         assert(0);
         return InvalidReg;
    }
    return InvalidReg;
}

MachRegister MachRegister::getSyscallNumberOReg(Dyninst::Architecture arch)
{
    switch (arch)
    {
        case Arch_x86:
            return x86::oeax;
        case Arch_x86_64:
            return x86_64::orax;
        case Arch_none:
            return InvalidReg;
      default:
         assert(0);
         return InvalidReg;
    }
    return InvalidReg;
}

MachRegister MachRegister::getSyscallReturnValueReg(Dyninst::Architecture arch)
{
    switch (arch)
    {
        case Arch_x86:
            return x86::eax;
        case Arch_x86_64:
            return x86_64::rax;
        case Arch_none:
            return InvalidReg;
      default:
         assert(0);
         return InvalidReg;
    }
    return InvalidReg;
}

MachRegister MachRegister::getZeroFlag(Dyninst::Architecture arch)
{
   switch (arch)
   {
      case Arch_x86:
         return x86::zf;
      case Arch_x86_64:
         return x86_64::zf;
      case Arch_none:
         return InvalidReg;
      default:
         return InvalidReg;
   }

   return InvalidReg;
}


bool MachRegister::isPC() const
{
   return (*this == x86_64::rip || *this == x86::eip);
}

bool MachRegister::isFramePointer() const
{
   return (*this == x86_64::rbp || *this == x86::ebp);
}

bool MachRegister::isStackPointer() const
{
   return (*this == x86_64::rsp || *this == x86::esp);
}

bool MachRegister::isSyscallNumberReg() const
{
   return ( *this == x86_64::orax || *this == x86::oeax);
}

bool MachRegister::isSyscallReturnValueReg() const
{
    return (*this == x86_64::rax || *this == x86::eax);
}

bool MachRegister::isFlag() const
{
    int regC = regClass();
    switch (getArchitecture())
    {
      case Arch_x86:
         return regC == x86::FLAG;
      case Arch_x86_64:
         return regC == x86_64::FLAG;
      default:
         assert(!"Not implemented!");
   }
   return false;
}

bool MachRegister::isZeroFlag() const
{
    switch (getArchitecture())
    {
      case Arch_x86:
         return *this == x86::zf;
      case Arch_x86_64:
         return *this == x86_64::zf;
      default:
         assert(!"Not implemented!");
   }
   return false;
}

COMMON_EXPORT bool Dyninst::isSegmentRegister(int regClass)
{
   return 0 != (regClass & x86::SEG);
}


/* This function should has a boolean return value
 * to indicate whether there is a corresponding
 * ROSE register.
 *
 * Since historically, this function does not 
 * have a return value. We set c to -1 to represent
 * error cases
 */

void MachRegister::getROSERegister(int &c, int &n, int &p)
{
   // Rose: class, number, position
   // Dyninst: category, base id, subrange

   signed int category = (reg & 0x00ff0000);
   signed int subrange = (reg & 0x0000ff00);
   signed int baseID =   (reg & 0x000000ff);

   switch (getArchitecture()) {
      case Arch_x86:
         switch (category) {
            case x86::GPR:
               c = x86_regclass_gpr;
               switch (baseID) {
                  case x86::BASEA:
                     n = x86_gpr_ax;
                     break;
                  case x86::BASEC:
                     n = x86_gpr_cx;
                     break;
                  case x86::BASED:
                     n = x86_gpr_dx;
                     break;
                  case x86::BASEB:
                     n = x86_gpr_bx;
                     break;
                  case x86::BASESP:
                     n = x86_gpr_sp;
                     break;
                  case x86::BASEBP:
                     n = x86_gpr_bp;
                     break;
                  case x86::BASESI:
                     n = x86_gpr_si;
                     break;
                  case x86::BASEDI:
                     n = x86_gpr_di;
                     break;
                  default:
                     n = 0;
                     break;
               }
               break;
            case x86::SEG:
               c = x86_regclass_segment;
               switch (baseID) {
                  case 0x0:
                     n = x86_segreg_ds;
                     break;
                  case 0x1:
                     n = x86_segreg_es;
                     break;
                  case 0x2:
                     n = x86_segreg_fs;
                     break;
                  case 0x3:
                     n = x86_segreg_gs;
                     break;
                  case 0x4:
                     n = x86_segreg_cs;
                     break;
                  case 0x5:
                     n = x86_segreg_ss;
                     break;
                  default:
                     n = 0;
                     break;
               }
               break;
            case x86::FLAG:
               c = x86_regclass_flags;
	       switch(baseID) {
	         case x86::CF:
		         n = x86_flag_cf;
		         break;
	         case x86::PF:
		         n = x86_flag_pf;
		         break;
	         case x86::AF:
		         n = x86_flag_af;
		         break;
	         case x86::ZF:
		         n = x86_flag_zf;
		         break;
	         case x86::SF:
		         n = x86_flag_sf;
		         break;
	         case x86::TF:
		         n = x86_flag_tf;
		         break;
	         case x86::IF:
		         n = x86_flag_if;
		         break;
	         case x86::DF:
		         n = x86_flag_df;
		         break;
	         case x86::OF:
		         n = x86_flag_of;
		         break;
	         default:
		         assert(0);
		         break;
	         }
         break;
         case x86::MISC:
               c = x86_regclass_unknown;
               break;
         case x86::XMM:
            c = x86_regclass_xmm;
            n = baseID;
            break;
         case x86::MMX:
            c = x86_regclass_mm;
            n = baseID;
            break;
         case x86::CTL:
            c = x86_regclass_cr;
            n = baseID;
            break;
         case x86::DBG:
            c = x86_regclass_dr;
            n = baseID;
            break;
         case x86::TST:
            c = x86_regclass_unknown;
            break;
         case 0:
           switch (baseID) {
              case 0x10:
                 c = x86_regclass_ip;
                 n = 0;
                 break;
              default:
                 c = x86_regclass_unknown;
                 break;
           }
         break;
         }
      break;
    case Arch_x86_64:
         switch (category) {
            case x86_64::GPR:
               c = x86_regclass_gpr;
               switch (baseID) {
                  case x86_64::BASEA:
                     n = x86_gpr_ax;
                     break;
                  case x86_64::BASEC:
                     n = x86_gpr_cx;
                     break;
                  case x86_64::BASED:
                     n = x86_gpr_dx;
                     break;
                  case x86_64::BASEB:
                     n = x86_gpr_bx;
                     break;
                  case x86_64::BASESP:
                     n = x86_gpr_sp;
                     break;
                  case x86_64::BASEBP:
                     n = x86_gpr_bp;
                     break;
                  case x86_64::BASESI:
                     n = x86_gpr_si;
                     break;
                  case x86_64::BASEDI:
                     n = x86_gpr_di;
                     break;
		  case x86_64::BASE8:
		     n = x86_gpr_r8;
		     break;
		  case x86_64::BASE9:
		     n = x86_gpr_r9;
		     break;
		  case x86_64::BASE10:
		     n = x86_gpr_r10;
		     break;
		  case x86_64::BASE11:
		     n = x86_gpr_r11;
		     break;
		  case x86_64::BASE12:
		     n = x86_gpr_r12;
		     break;
		  case x86_64::BASE13:
		     n = x86_gpr_r13;
		     break;
		  case x86_64::BASE14:
		     n = x86_gpr_r14;
		     break;
		  case x86_64::BASE15:
		     n = x86_gpr_r15;
		     break;
                  default:
                     n = 0;
                     break;
               }
               break;
            case x86_64::SEG:
               c = x86_regclass_segment;
               switch (baseID) {
                  case 0x0:
                     n = x86_segreg_ds;
                     break;
                  case 0x1:
                     n = x86_segreg_es;
                     break;
                  case 0x2:
                     n = x86_segreg_fs;
                     break;
                  case 0x3:
                     n = x86_segreg_gs;
                     break;
                  case 0x4:
                     n = x86_segreg_cs;
                     break;
                  case 0x5:
                     n = x86_segreg_ss;
                     break;
                  default:
                     n = 0;
                     break;
               }
               break;
            case x86_64::FLAG:
               c = x86_regclass_flags;
	       switch(baseID) {
	       case x86_64::CF:
		 n = x86_flag_cf;
		 break;
	       case x86_64::PF:
		 n = x86_flag_pf;
		 break;
	       case x86_64::AF:
		 n = x86_flag_af;
		 break;
	       case x86_64::ZF:
		 n = x86_flag_zf;
		 break;
	       case x86_64::SF:
		 n = x86_flag_sf;
		 break;
	       case x86_64::TF:
		 n = x86_flag_tf;
		 break;
	       case x86_64::IF:
		 n = x86_flag_if;
		 break;
	       case x86_64::DF:
		 n = x86_flag_df;
		 break;
	       case x86_64::OF:
		 n = x86_flag_of;
		 break;
	       default:
                 c = -1;
                 return; 
		 break;
      }
               break;
            case x86_64::MISC:
               c = x86_regclass_unknown;
               break;
            case x86_64::KMASK:
               c = x86_regclass_kmask;
               n = baseID;
               break;
            case x86_64::ZMM:
               c = x86_regclass_zmm;
               n = baseID;
               break;
            case x86_64::YMM:
               c = x86_regclass_ymm;
               n = baseID;
               break;
            case x86_64::XMM:
               c = x86_regclass_xmm;
               n = baseID;
               break;
            case x86_64::MMX:
               c = x86_regclass_mm;
               n = baseID;
               break;
            case x86_64::CTL:
               c = x86_regclass_cr;
               n = baseID;
               break;
            case x86_64::DBG:
               c = x86_regclass_dr;
               n = baseID;
               break;
            case x86_64::TST:
               c = x86_regclass_unknown;
               break;
            case 0:
               switch (baseID) {
                  case 0x10:
                     c = x86_regclass_ip;
                     n = 0;
                     break;
                  default:
                     c = x86_regclass_unknown;
                     break;
               }
               break;
         }
         break;
      default:
         c = x86_regclass_unknown;
         n = 0;
         break;
   }

   switch (getArchitecture()) {
      case Arch_x86:
         switch (subrange) {
            case x86::OCT:
            case x86::FPDBL:
               p = x86_regpos_qword;
               break;
            case x86::H_REG:
               p = x86_regpos_high_byte;
               break;
            case x86::L_REG:
               p = x86_regpos_low_byte;
               break;
            case x86::W_REG:
               p = x86_regpos_word;
               break;
            case x86::FULL:
            case x86_64::D_REG:
               p = x86_regpos_dword;
               break;
	    case x86::BIT:
     	       p = x86_regpos_all;
	       break;
         }
         break;

      case Arch_x86_64:
         switch (subrange) {
            case x86::FULL:
            case x86::OCT:
            case x86::FPDBL:
               p = x86_regpos_qword;
               break;
            case x86::H_REG:
               p = x86_regpos_high_byte;
               break;
            case x86::L_REG:
               p = x86_regpos_low_byte;
               break;
            case x86::W_REG:
               p = x86_regpos_word;
               break;
            case x86_64::D_REG:
               p = x86_regpos_dword;
               break;
	         case x86::BIT:
     	         p = x86_regpos_all;
	         break;
         }
      break;
      default:
        p = x86_regpos_unknown;
   }
}

unsigned Dyninst::getArchAddressWidth(Dyninst::Architecture arch)
{
   switch (arch) {
      case Arch_none:
         return 0;
      case Arch_x86:
         return 4;
      case Arch_x86_64:
         return 8;
      default:
         assert(0);
         return InvalidReg;
   }
   return 0;
}
