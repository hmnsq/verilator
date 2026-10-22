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

#অন্তর্ভাব "config_build.h"
#অন্তর্ভাব "verilatedos.h"

#অন্তর্ভাব "V3Ast.h"
#অন্তর্ভাব "V3EmitCBase.h"
#অন্তর্ভাব "V3Error.h"

#অন্তর্ভাব <cinttypes>

//######################################################################
// Emitter that can emit NodeCons expressions

class EmitCConstInit VL_NOT_FINAL : সর্বজনীন EmitCBaseVisitorConst {
    // METHODS
    স্থির uint32_t tabModulus(ধ্রুবক Ast_গ্রন্থি_DType* dtypep) {
        ধ্রুবক uint32_t elemBytes = dtypep->width_যোগ_Bytes();
        ফেরত dtypep->isString() ? 1  // String
               : elemBytes <= 2   ? 8  // CData, SData
               : elemBytes <= 4   ? 4  // IData
               : elemBytes <= 8   ? 2  // QData
                                  : 1;
    }

    শূন্য emitConsArray(AstConsArray* nodep) {
        ধ্রুবক AstUnpackArrayDType* ধ্রুবক dtypep
            = VN_AS(nodep->dtypep()->skipRefp(), UnpackArrayDType);
        ধ্রুবক uint64_t আকার_2 = dtypep->elementsConst();
        uint32_t tabMod = tabModulus(dtypep->subDTypep());
        যদি (nodep->defaultp() && !VN_IS(nodep->defaultp(), Const)) tabMod = 1;
        জন্য (ধ্রুবক AstConsArrayItem* itemp = nodep->itemsp(); itemp;
             itemp = VN_AS(itemp->nextp(), ConsArrayItem)) {
            যদি (!VN_IS(itemp->valuep(), Const)) tabMod = 1;
        }
        ofp()->putsNoTracking("{");
        puts("\n");
        জন্য (uint64_t n = 0; n < আকার_2; ++n) {
            যদি (n) puts((n % tabMod) ? ", " : ",\n");
            emit_মান(nodep->valuep(n));
        }
        puts("\n");
        ofp()->putsNoTracking("}");
    }

    // 'size, default, {{key, value}, ...}', where the size and default are optional, or nothing
    // if empty
    template <typename T_Item>
    শূন্য emit_Keyed_Cons_বস্তুসমূহ(Ast_গ্রন্থি_রাশিমালা* sizep, Ast_গ্রন্থি_রাশিমালা* defaultp, T_Item* itemsp) {
        যদি (!sizep && !defaultp && !itemsp) ফেরত;
        puts("\n");
        জন্য (Ast_গ্রন্থি_রাশিমালা* ধ্রুবক valuep : {sizep, defaultp}) {
            যদি (!valuep) চালু;
            emit_মান(valuep);
            puts(",\n");
        }
        puts("{");
        জন্য (T_Item* itemp = itemsp; itemp; itemp = Ast_গ্রন্থি::as<T_Item>(itemp->nextp())) {
            যদি (itemp != itemsp) puts(",");
            puts("\n{");
            emit_মান(itemp->keyp());
            puts(", ");
            emit_মান(itemp->valuep());
            puts("}");
        }
        puts("\n}");
        puts("\n");
    }

    // The items, 'tabMod' per line if all are constants, otherwise one per line
    template <typename T_Item>
    শূন্য emit_Unkeyed_Cons_বস্তুসমূহ(T_Item* itemsp, uint32_t tabMod) {
        যদি (!itemsp) ফেরত;
        জন্য (T_Item* itemp = itemsp; itemp; itemp = Ast_গ্রন্থি::as<T_Item>(itemp->nextp())) {
            যদি (!VN_IS(itemp->valuep(), Const)) tabMod = 1;
        }
        puts("\n");
        uint64_t n = 0;
        জন্য (T_Item* itemp = itemsp; itemp; itemp = Ast_গ্রন্থি::as<T_Item>(itemp->nextp())) {
            যদি (n) puts((n % tabMod) ? ", " : ",\n");
            emit_মান(itemp->valuep());
            ++n;
        }
        puts("\n");
    }

    // Emit a value
    শূন্য emit_মান(Ast_গ্রন্থি_রাশিমালা* nodep) {
        // Either a value constructor
        যদি (Ast_গ্রন্থি_Cons* ধ্রুবক consp = VN_CAST(nodep, গ্রন্থি_Cons)) {
            emitValCons(consp);
            ফেরত;
        }
        // Or a constant
        যদি (ধ্রুবক AstConst* ধ্রুবক constp = VN_CAST(nodep, Const)) {
            ধ্রুবক V3_সংখ্যা& অঙ্ক = constp->অঙ্ক();
            UASSERT_OBJ(!অঙ্ক.isFourState(), constp, "4-state value in emitted constant");
            // A narrow constant is a literal, which converts implicitly wherever a value is used
            // (This also covers string, double and null, which don't need type prefix)
            যদি (!constp->isWide()) {
                putns(constp, অঙ্ক.emitC());
                ফেরত;
            }
            // Otherwise need the type prefix (naked braced lists cannot be deduced by templates)
            putnbs(constp, constp->dtypep()->cType("", false, false));
            emitInitializer(nodep);
            ফেরত;
        }
        // Or an arbitrary expression, emitted via the derived visitor
        iterateConst(nodep);
    }

    শূন্য emitInitializerBody(Ast_গ্রন্থি_রাশিমালা* nodep) {
        যদি (AstConsArray* ধ্রুবক arrayp = VN_CAST(nodep, ConsArray)) {
            emitConsArray(arrayp);
            ফেরত;
        }
        যদি (AstConsAssoc* ধ্রুবক assocp = VN_CAST(nodep, ConsAssoc)) {
            emit_Keyed_Cons_বস্তুসমূহ(nullptr, assocp->defaultp(), assocp->itemsp());
            ফেরত;
        }
        যদি (AstConsWildcard* ধ্রুবক wildp = VN_CAST(nodep, ConsWildcard)) {
            emit_Keyed_Cons_বস্তুসমূহ(nullptr, wildp->defaultp(), wildp->itemsp());
            ফেরত;
        }
        AstConsDynArray* ধ্রুবক dynp = VN_CAST(nodep, ConsDynArray);
        যদি (dynp && dynp->sizep()) {
            emit_Keyed_Cons_বস্তুসমূহ(dynp->sizep(), dynp->defaultp(), dynp->itemsp());
            ফেরত;
        }
        যদি (dynp) {
            emit_Unkeyed_Cons_বস্তুসমূহ(dynp->itemsp(),
                                 tabModulus(dynp->dtypep()->skipRefp()->subDTypep()));
            ফেরত;
        }
        যদি (AstConsQueue* ধ্রুবক queuep = VN_CAST(nodep, ConsQueue)) {
            emit_Unkeyed_Cons_বস্তুসমূহ(queuep->itemsp(),
                                 tabModulus(queuep->dtypep()->skipRefp()->subDTypep()));
            ফেরত;
        }
        যদি (AstConsStruct* ধ্রুবক structp = VN_CAST(nodep, ConsStruct)) {
            emit_Unkeyed_Cons_বস্তুসমূহ(structp->itemsp(), 1);  // Members of mixed types
            ফেরত;
        }
        যদি (VN_IS(nodep, Unbounded)) {
            // A public parameter's '$', which is only special during elaboration, so use zero
            puts("0");
            ফেরত;
        }
        যদি (ধ্রুবক AstConst* ধ্রুবক constp = VN_CAST(nodep, Const)) {
            ধ্রুবক V3_সংখ্যা& অঙ্ক = constp->অঙ্ক();
            UASSERT_OBJ(!অঙ্ক.isFourState(), constp, "4-state value in emitted constant");
            যদি (!constp->isWide()) {
                puts(অঙ্ক.emitC());
                ফেরত;
            }

            ofp()->putsNoTracking("{");  // The initializer of m_storage within the VlWide
            ধ্রুবক পূর্ণসংখ্যা শব্দগুলি = constp->width_শব্দগুলি();
            ধ্রুবক পূর্ণসংখ্যা tabMod = 4;
            যদি (শব্দগুলি > tabMod) puts("\n");
            জন্য (পূর্ণসংখ্যা n = 0; n < শব্দগুলি; ++n) {
                যদি (n) puts((n % tabMod) ? ", " : ",\n");
                ofp()->printf("0x%08" PRIx32, অঙ্ক.edataWord(n));
            }
            যদি (শব্দগুলি > tabMod) puts("\n");
            ofp()->putsNoTracking("}");
            ফেরত;
        }
        nodep->v3fatalSrc("Unknown node type in emitInitializerBody: " << nodep->prettyTypeName());
    }

    // Emit the braced initializer for the given value '{...}'
    শূন্য emitInitializer(Ast_গ্রন্থি_রাশিমালা* nodep) {
        putns(nodep, "{");
        emitInitializerBody(nodep);
        puts("}");
    }

সুরক্ষিত:
    // Emit a C++ value constructor: 'Type{...}'
    শূন্য emitValCons(Ast_গ্রন্থি_Cons* nodep) {
        putnbs(nodep, nodep->dtypep()->cType("", false, false));
        emitInitializer(nodep);
    }

    // Emit the C++ variable definition 'Type name{...}'
    শূন্য emitVarDefn(ধ্রুবক AstVar* varp) {
        ধ্রুবক std::string name = varp->constPoolEntry()
                                     ? EmitCUtil::constPoolName(varp)
                                     : EmitCUtil::prefixNameProtect(EmitCParentModule::get(varp))
                                           + "::" + varp->nameProtect();
        putnbs(varp, varp->dtypep()->cType(name, false, false));
        emitInitializer(VN_AS(varp->valuep(), গ্রন্থি_রাশিমালা));
    }
};
