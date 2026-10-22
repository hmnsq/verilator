// -*- mode: C++; c-file-style: "cc-mode" -*-
//*************************************************************************
// DESCRIPTION: Verilator: Emit C++ for tree
//
// Code available from: https://verilator.org
//
//*************************************************************************
//
// This program is free software; you can redistribute it and/or modify it
// under the terms of either the GNU Lesser General Public License Version 3
// or the Perl Artistic License Version 2.0.
// SPDX-FileCopyrightText: 2003-2026 Wilson Snyder
// SPDX-License-Identifier: LGPL-3.0-only OR Artistic-2.0
//
//*************************************************************************

#ضمن "config_build.h"
#ضمن "verilatedos.h"

#ضمن "V3Ast.h"
#ضمن "V3EmitCBase.h"
#ضمن "V3Error.h"

#ضمن <cinttypes>

//######################################################################
// Emitter that can emit NodeCons expressions

class EmitCConstInit VL_NOT_FINAL : عام EmitCBaseVisitorConst {
    // METHODS
    ساكن uint32_t tabModulus(ثابت Ast_گرہ_DType* dtypep) {
        ثابت uint32_t elem_بائٹاں = dtypep->چوڑائی_کل_بائٹاں();
        أرجع dtypep->isString() ? 1  // String
               : elem_بائٹاں <= 2   ? 8  // CData, SData
               : elem_بائٹاں <= 4   ? 4  // IData
               : elem_بائٹاں <= 8   ? 2  // QData
                                  : 1;
    }

    فراغ emitConsArray(AstConsArray* nodep) {
        ثابت AstUnpackArrayDType* ثابت dtypep
            = VN_AS(nodep->dtypep()->skipRefp(), UnpackArrayDType);
        ثابت uint64_t ناپ = dtypep->elementsConst();
        uint32_t tabMod = tabModulus(dtypep->subDTypep());
        إذا (nodep->defaultp() && !VN_IS(nodep->defaultp(), Const)) tabMod = 1;
        لكل (ثابت Ast_Cons_Array_شے* itemp = nodep->itemsp(); itemp;
             itemp = VN_AS(itemp->nextp(), Cons_Array_شے)) {
            إذا (!VN_IS(itemp->valuep(), Const)) tabMod = 1;
        }
        ofp()->putsNoTracking("{");
        puts("\n");
        لكل (uint64_t n = 0; n < ناپ; ++n) {
            إذا (n) puts((n % tabMod) ? ", " : ",\n");
            emit_قدر(nodep->valuep(n));
        }
        puts("\n");
        ofp()->putsNoTracking("}");
    }

    // 'size, default, {{key, value}, ...}', where the size and default are optional, or nothing
    // if empty
    سانچا <typename T_شے>
    فراغ emit_Keyed_Cons_اشیاء(Ast_گرہ_اظہاریہ* sizep, Ast_گرہ_اظہاریہ* defaultp, T_شے* itemsp) {
        إذا (!sizep && !defaultp && !itemsp) أرجع;
        puts("\n");
        لكل (Ast_گرہ_اظہاریہ* ثابت valuep : {sizep, defaultp}) {
            إذا (!valuep) استمر;
            emit_قدر(valuep);
            puts(",\n");
        }
        puts("{");
        لكل (T_شے* itemp = itemsp; itemp; itemp = Ast_گرہ::as<T_شے>(itemp->nextp())) {
            إذا (itemp != itemsp) puts(",");
            puts("\n{");
            emit_قدر(itemp->keyp());
            puts(", ");
            emit_قدر(itemp->valuep());
            puts("}");
        }
        puts("\n}");
        puts("\n");
    }

    // The items, 'tabMod' per line if all are constants, otherwise one per line
    سانچا <typename T_شے>
    فراغ emit_Unkeyed_Cons_اشیاء(T_شے* itemsp, uint32_t tabMod) {
        إذا (!itemsp) أرجع;
        لكل (T_شے* itemp = itemsp; itemp; itemp = Ast_گرہ::as<T_شے>(itemp->nextp())) {
            إذا (!VN_IS(itemp->valuep(), Const)) tabMod = 1;
        }
        puts("\n");
        uint64_t n = 0;
        لكل (T_شے* itemp = itemsp; itemp; itemp = Ast_گرہ::as<T_شے>(itemp->nextp())) {
            إذا (n) puts((n % tabMod) ? ", " : ",\n");
            emit_قدر(itemp->valuep());
            ++n;
        }
        puts("\n");
    }

    // Emit a value
    فراغ emit_قدر(Ast_گرہ_اظہاریہ* nodep) {
        // Either a value constructor
        إذا (Ast_گرہ_Cons* ثابت consp = VN_CAST(nodep, گرہ_Cons)) {
            emitValCons(consp);
            أرجع;
        }
        // Or a constant
        إذا (ثابت AstConst* ثابت constp = VN_CAST(nodep, Const)) {
            ثابت V3_نمبر& عدد = constp->عدد();
            UASSERT_OBJ(!عدد.isFourState(), constp, "4-state value in emitted constant");
            // A narrow constant is a literal, which converts implicitly wherever a value is used
            // (This also covers string, double and null, which don't need type prefix)
            إذا (!constp->isWide()) {
                putns(constp, عدد.emitC());
                أرجع;
            }
            // Otherwise need the type prefix (naked braced lists cannot be deduced by templates)
            putnbs(constp, constp->dtypep()->cType("", false, false));
            emitInitializer(nodep);
            أرجع;
        }
        // Or an arbitrary expression, emitted via the derived visitor
        iterateConst(nodep);
    }

    فراغ emitInitializerBody(Ast_گرہ_اظہاریہ* nodep) {
        إذا (AstConsArray* ثابت arrayp = VN_CAST(nodep, ConsArray)) {
            emitConsArray(arrayp);
            أرجع;
        }
        إذا (AstConsAssoc* ثابت assocp = VN_CAST(nodep, ConsAssoc)) {
            emit_Keyed_Cons_اشیاء(nullptr, assocp->defaultp(), assocp->itemsp());
            أرجع;
        }
        إذا (AstConsWildcard* ثابت wildp = VN_CAST(nodep, ConsWildcard)) {
            emit_Keyed_Cons_اشیاء(nullptr, wildp->defaultp(), wildp->itemsp());
            أرجع;
        }
        AstConsDynArray* ثابت dynp = VN_CAST(nodep, ConsDynArray);
        إذا (dynp && dynp->sizep()) {
            emit_Keyed_Cons_اشیاء(dynp->sizep(), dynp->defaultp(), dynp->itemsp());
            أرجع;
        }
        إذا (dynp) {
            emit_Unkeyed_Cons_اشیاء(dynp->itemsp(),
                                 tabModulus(dynp->dtypep()->skipRefp()->subDTypep()));
            أرجع;
        }
        إذا (Ast_Cons_قطار* ثابت queuep = VN_CAST(nodep, Cons_قطار)) {
            emit_Unkeyed_Cons_اشیاء(queuep->itemsp(),
                                 tabModulus(queuep->dtypep()->skipRefp()->subDTypep()));
            أرجع;
        }
        إذا (AstConsStruct* ثابت structp = VN_CAST(nodep, ConsStruct)) {
            emit_Unkeyed_Cons_اشیاء(structp->itemsp(), 1);  // Members of mixed types
            أرجع;
        }
        إذا (VN_IS(nodep, Unbounded)) {
            // A public parameter's '$', which is only special during elaboration, so use zero
            puts("0");
            أرجع;
        }
        إذا (ثابت AstConst* ثابت constp = VN_CAST(nodep, Const)) {
            ثابت V3_نمبر& عدد = constp->عدد();
            UASSERT_OBJ(!عدد.isFourState(), constp, "4-state value in emitted constant");
            إذا (!constp->isWide()) {
                puts(عدد.emitC());
                أرجع;
            }

            ofp()->putsNoTracking("{");  // The initializer of m_storage within the VlWide
            ثابت صحيح الفاظ = constp->چوڑائی_الفاظ();
            ثابت صحيح tabMod = 4;
            إذا (الفاظ > tabMod) puts("\n");
            لكل (صحيح n = 0; n < الفاظ; ++n) {
                إذا (n) puts((n % tabMod) ? ", " : ",\n");
                ofp()->لکھو("0x%08" PRIx32, عدد.edata_لفظ(n));
            }
            إذا (الفاظ > tabMod) puts("\n");
            ofp()->putsNoTracking("}");
            أرجع;
        }
        nodep->v3fatalSrc("Unknown node type in emitInitializerBody: " << nodep->prettyTypeName());
    }

    // Emit the braced initializer for the given value '{...}'
    فراغ emitInitializer(Ast_گرہ_اظہاریہ* nodep) {
        putns(nodep, "{");
        emitInitializerBody(nodep);
        puts("}");
    }

محمي:
    // Emit a C++ value constructor: 'Type{...}'
    فراغ emitValCons(Ast_گرہ_Cons* nodep) {
        putnbs(nodep, nodep->dtypep()->cType("", false, false));
        emitInitializer(nodep);
    }

    // Emit the C++ variable definition 'Type name{...}'
    فراغ emitVarDefn(ثابت AstVar* varp) {
        ثابت std::string name = varp->const_Pool_اندراج()
                                     ? EmitCUtil::constPoolName(varp)
                                     : EmitCUtil::اگیتر_Name_Protect(EmitCParentModule::get(varp))
                                           + "::" + varp->nameProtect();
        putnbs(varp, varp->dtypep()->cType(name, false, false));
        emitInitializer(VN_AS(varp->valuep(), گرہ_اظہاریہ));
    }
};
