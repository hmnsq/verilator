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

#شامل "config_build.h"
#شامل "verilatedos.h"

#شامل "V3Ast.h"
#شامل "V3EmitCBase.h"
#شامل "V3Error.h"

#شامل <cinttypes>

//######################################################################
// Emitter that can emit NodeCons expressions

class EmitCConstInit VL_NOT_FINAL : عوامی EmitCBaseVisitorConst {
    // METHODS
    ساکن uint32_t tabModulus(ثابت Ast_گرہ_DType* dtypep) {
        ثابت uint32_t elem_بائٹیں = dtypep->چوڑائی_کل_بائٹیں();
        واپس dtypep->isString() ? 1  // String
               : elem_بائٹیں <= 2   ? 8  // CData, SData
               : elem_بائٹیں <= 4   ? 4  // IData
               : elem_بائٹیں <= 8   ? 2  // QData
                                  : 1;
    }

    خالی emitConsArray(AstConsArray* nodep) {
        ثابت AstUnpackArrayDType* ثابت dtypep
            = VN_AS(nodep->dtypep()->skipRefp(), UnpackArrayDType);
        ثابت uint64_t سائز = dtypep->elementsConst();
        uint32_t tabMod = tabModulus(dtypep->subDTypep());
        اگر (nodep->defaultp() && !VN_IS(nodep->defaultp(), Const)) tabMod = 1;
        برائے (ثابت Ast_Cons_Array_شے* itemp = nodep->itemsp(); itemp;
             itemp = VN_AS(itemp->nextp(), Cons_Array_شے)) {
            اگر (!VN_IS(itemp->valuep(), Const)) tabMod = 1;
        }
        ofp()->putsNoTracking("{");
        puts("\n");
        برائے (uint64_t n = 0; n < سائز; ++n) {
            اگر (n) puts((n % tabMod) ? ", " : ",\n");
            emit_قدر(nodep->valuep(n));
        }
        puts("\n");
        ofp()->putsNoTracking("}");
    }

    // 'size, default, {{key, value}, ...}', where the size and default are optional, or nothing
    // if empty
    سانچہ <typename T_شے>
    خالی emit_Keyed_Cons_اشیاء(Ast_گرہ_اظہاریہ* sizep, Ast_گرہ_اظہاریہ* defaultp, T_شے* itemsp) {
        اگر (!sizep && !defaultp && !itemsp) واپس;
        puts("\n");
        برائے (Ast_گرہ_اظہاریہ* ثابت valuep : {sizep, defaultp}) {
            اگر (!valuep) جاری;
            emit_قدر(valuep);
            puts(",\n");
        }
        puts("{");
        برائے (T_شے* itemp = itemsp; itemp; itemp = Ast_گرہ::as<T_شے>(itemp->nextp())) {
            اگر (itemp != itemsp) puts(",");
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
    سانچہ <typename T_شے>
    خالی emit_Unkeyed_Cons_اشیاء(T_شے* itemsp, uint32_t tabMod) {
        اگر (!itemsp) واپس;
        برائے (T_شے* itemp = itemsp; itemp; itemp = Ast_گرہ::as<T_شے>(itemp->nextp())) {
            اگر (!VN_IS(itemp->valuep(), Const)) tabMod = 1;
        }
        puts("\n");
        uint64_t n = 0;
        برائے (T_شے* itemp = itemsp; itemp; itemp = Ast_گرہ::as<T_شے>(itemp->nextp())) {
            اگر (n) puts((n % tabMod) ? ", " : ",\n");
            emit_قدر(itemp->valuep());
            ++n;
        }
        puts("\n");
    }

    // Emit a value
    خالی emit_قدر(Ast_گرہ_اظہاریہ* nodep) {
        // Either a value constructor
        اگر (Ast_گرہ_Cons* ثابت consp = VN_CAST(nodep, گرہ_Cons)) {
            emitValCons(consp);
            واپس;
        }
        // Or a constant
        اگر (ثابت AstConst* ثابت constp = VN_CAST(nodep, Const)) {
            ثابت V3_نمبر& عدد = constp->عدد();
            UASSERT_OBJ(!عدد.isFourState(), constp, "4-state value in emitted constant");
            // A narrow constant is a literal, which converts implicitly wherever a value is used
            // (This also covers string, double and null, which don't need type prefix)
            اگر (!constp->isWide()) {
                putns(constp, عدد.emitC());
                واپس;
            }
            // Otherwise need the type prefix (naked braced lists cannot be deduced by templates)
            putnbs(constp, constp->dtypep()->cType("", false, false));
            emitInitializer(nodep);
            واپس;
        }
        // Or an arbitrary expression, emitted via the derived visitor
        iterateConst(nodep);
    }

    خالی emitInitializerBody(Ast_گرہ_اظہاریہ* nodep) {
        اگر (AstConsArray* ثابت arrayp = VN_CAST(nodep, ConsArray)) {
            emitConsArray(arrayp);
            واپس;
        }
        اگر (AstConsAssoc* ثابت assocp = VN_CAST(nodep, ConsAssoc)) {
            emit_Keyed_Cons_اشیاء(nullptr, assocp->defaultp(), assocp->itemsp());
            واپس;
        }
        اگر (AstConsWildcard* ثابت wildp = VN_CAST(nodep, ConsWildcard)) {
            emit_Keyed_Cons_اشیاء(nullptr, wildp->defaultp(), wildp->itemsp());
            واپس;
        }
        AstConsDynArray* ثابت dynp = VN_CAST(nodep, ConsDynArray);
        اگر (dynp && dynp->sizep()) {
            emit_Keyed_Cons_اشیاء(dynp->sizep(), dynp->defaultp(), dynp->itemsp());
            واپس;
        }
        اگر (dynp) {
            emit_Unkeyed_Cons_اشیاء(dynp->itemsp(),
                                 tabModulus(dynp->dtypep()->skipRefp()->subDTypep()));
            واپس;
        }
        اگر (Ast_Cons_قطار* ثابت queuep = VN_CAST(nodep, Cons_قطار)) {
            emit_Unkeyed_Cons_اشیاء(queuep->itemsp(),
                                 tabModulus(queuep->dtypep()->skipRefp()->subDTypep()));
            واپس;
        }
        اگر (AstConsStruct* ثابت structp = VN_CAST(nodep, ConsStruct)) {
            emit_Unkeyed_Cons_اشیاء(structp->itemsp(), 1);  // Members of mixed types
            واپس;
        }
        اگر (VN_IS(nodep, Unbounded)) {
            // A public parameter's '$', which is only special during elaboration, so use zero
            puts("0");
            واپس;
        }
        اگر (ثابت AstConst* ثابت constp = VN_CAST(nodep, Const)) {
            ثابت V3_نمبر& عدد = constp->عدد();
            UASSERT_OBJ(!عدد.isFourState(), constp, "4-state value in emitted constant");
            اگر (!constp->isWide()) {
                puts(عدد.emitC());
                واپس;
            }

            ofp()->putsNoTracking("{");  // The initializer of m_storage within the VlWide
            ثابت صحیح الفاظ = constp->چوڑائی_الفاظ();
            ثابت صحیح tabMod = 4;
            اگر (الفاظ > tabMod) puts("\n");
            برائے (صحیح n = 0; n < الفاظ; ++n) {
                اگر (n) puts((n % tabMod) ? ", " : ",\n");
                ofp()->لکھو("0x%08" PRIx32, عدد.edata_لفظ(n));
            }
            اگر (الفاظ > tabMod) puts("\n");
            ofp()->putsNoTracking("}");
            واپس;
        }
        nodep->v3fatalSrc("Unknown node type in emitInitializerBody: " << nodep->prettyTypeName());
    }

    // Emit the braced initializer for the given value '{...}'
    خالی emitInitializer(Ast_گرہ_اظہاریہ* nodep) {
        putns(nodep, "{");
        emitInitializerBody(nodep);
        puts("}");
    }

محفوظ:
    // Emit a C++ value constructor: 'Type{...}'
    خالی emitValCons(Ast_گرہ_Cons* nodep) {
        putnbs(nodep, nodep->dtypep()->cType("", false, false));
        emitInitializer(nodep);
    }

    // Emit the C++ variable definition 'Type name{...}'
    خالی emitVarDefn(ثابت AstVar* varp) {
        ثابت std::string name = varp->const_Pool_اندراج()
                                     ? EmitCUtil::constPoolName(varp)
                                     : EmitCUtil::سابقہ_Name_Protect(EmitCParentModule::get(varp))
                                           + "::" + varp->nameProtect();
        putnbs(varp, varp->dtypep()->cType(name, false, false));
        emitInitializer(VN_AS(varp->valuep(), گرہ_اظہاریہ));
    }
};
