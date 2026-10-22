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

#অন্তর্ভাৱ "config_build.h"
#অন্তর্ভাৱ "verilatedos.h"

#অন্তর্ভাৱ "V3Ast.h"
#অন্তর্ভাৱ "V3EmitCBase.h"
#অন্তর্ভাৱ "V3Error.h"

#অন্তর্ভাৱ <cinttypes>

//######################################################################
// Emitter that can emit NodeCons expressions

class EmitCConstInit VL_NOT_FINAL : সার্ৱজনিক EmitCBaseVisitorConst {
    // METHODS
    স্থির uint32_t tabModulus(ধ্রুৱ Ast_গ্ৰন্থি_DType* dtypep) {
        ধ্রুৱ uint32_t elemBytes = dtypep->widthTotalBytes();
        ঘূৰাই dtypep->isString() ? 1  // String
               : elemBytes <= 2   ? 8  // CData, SData
               : elemBytes <= 4   ? 4  // IData
               : elemBytes <= 8   ? 2  // QData
                                  : 1;
    }

    শূন্য emitConsArray(AstConsArray* nodep) {
        ধ্রুৱ AstUnpackArrayDType* ধ্রুৱ dtypep
            = VN_AS(nodep->dtypep()->skipRefp(), UnpackArrayDType);
        ধ্রুৱ uint64_t size = dtypep->elementsConst();
        uint32_t tabMod = tabModulus(dtypep->subDTypep());
        যদি (nodep->defaultp() && !VN_IS(nodep->defaultp(), Const)) tabMod = 1;
        বাবে (ধ্রুৱ AstConsArrayItem* itemp = nodep->itemsp(); itemp;
             itemp = VN_AS(itemp->nextp(), ConsArrayItem)) {
            যদি (!VN_IS(itemp->valuep(), Const)) tabMod = 1;
        }
        ofp()->putsNoTracking("{");
        puts("\n");
        বাবে (uint64_t n = 0; n < size; ++n) {
            যদি (n) puts((n % tabMod) ? ", " : ",\n");
            emit_মান(nodep->valuep(n));
        }
        puts("\n");
        ofp()->putsNoTracking("}");
    }

    // 'size, default, {{key, value}, ...}', where the size and default are optional, or nothing
    // if empty
    template <typename T_Item>
    শূন্য emit_Keyed_Cons_বস্তুসমূহ(Ast_গ্ৰন্থি_ৰাশিমালা* sizep, Ast_গ্ৰন্থি_ৰাশিমালা* defaultp, T_Item* itemsp) {
        যদি (!sizep && !defaultp && !itemsp) ঘূৰাই;
        puts("\n");
        বাবে (Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ valuep : {sizep, defaultp}) {
            যদি (!valuep) অনুৱর্ত;
            emit_মান(valuep);
            puts(",\n");
        }
        puts("{");
        বাবে (T_Item* itemp = itemsp; itemp; itemp = Ast_গ্ৰন্থি::as<T_Item>(itemp->nextp())) {
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
        যদি (!itemsp) ঘূৰাই;
        বাবে (T_Item* itemp = itemsp; itemp; itemp = Ast_গ্ৰন্থি::as<T_Item>(itemp->nextp())) {
            যদি (!VN_IS(itemp->valuep(), Const)) tabMod = 1;
        }
        puts("\n");
        uint64_t n = 0;
        বাবে (T_Item* itemp = itemsp; itemp; itemp = Ast_গ্ৰন্থি::as<T_Item>(itemp->nextp())) {
            যদি (n) puts((n % tabMod) ? ", " : ",\n");
            emit_মান(itemp->valuep());
            ++n;
        }
        puts("\n");
    }

    // Emit a value
    শূন্য emit_মান(Ast_গ্ৰন্থি_ৰাশিমালা* nodep) {
        // Either a value constructor
        যদি (Ast_গ্ৰন্থি_Cons* ধ্রুৱ consp = VN_CAST(nodep, গ্ৰন্থি_Cons)) {
            emitValCons(consp);
            ঘূৰাই;
        }
        // Or a constant
        যদি (ধ্রুৱ AstConst* ধ্রুৱ constp = VN_CAST(nodep, Const)) {
            ধ্রুৱ V3_সংখ্যা& অঙ্ক = constp->অঙ্ক();
            UASSERT_OBJ(!অঙ্ক.isFourState(), constp, "4-state value in emitted constant");
            // A narrow constant is a literal, which converts implicitly wherever a value is used
            // (This also covers string, double and null, which don't need type prefix)
            যদি (!constp->isWide()) {
                putns(constp, অঙ্ক.emitC());
                ঘূৰাই;
            }
            // Otherwise need the type prefix (naked braced lists cannot be deduced by templates)
            putnbs(constp, constp->dtypep()->cType("", false, false));
            emitInitializer(nodep);
            ঘূৰাই;
        }
        // Or an arbitrary expression, emitted via the derived visitor
        iterateConst(nodep);
    }

    শূন্য emitInitializerBody(Ast_গ্ৰন্থি_ৰাশিমালা* nodep) {
        যদি (AstConsArray* ধ্রুৱ arrayp = VN_CAST(nodep, ConsArray)) {
            emitConsArray(arrayp);
            ঘূৰাই;
        }
        যদি (AstConsAssoc* ধ্রুৱ assocp = VN_CAST(nodep, ConsAssoc)) {
            emit_Keyed_Cons_বস্তুসমূহ(nullptr, assocp->defaultp(), assocp->itemsp());
            ঘূৰাই;
        }
        যদি (AstConsWildcard* ধ্রুৱ wildp = VN_CAST(nodep, ConsWildcard)) {
            emit_Keyed_Cons_বস্তুসমূহ(nullptr, wildp->defaultp(), wildp->itemsp());
            ঘূৰাই;
        }
        AstConsDynArray* ধ্রুৱ dynp = VN_CAST(nodep, ConsDynArray);
        যদি (dynp && dynp->sizep()) {
            emit_Keyed_Cons_বস্তুসমূহ(dynp->sizep(), dynp->defaultp(), dynp->itemsp());
            ঘূৰাই;
        }
        যদি (dynp) {
            emit_Unkeyed_Cons_বস্তুসমূহ(dynp->itemsp(),
                                 tabModulus(dynp->dtypep()->skipRefp()->subDTypep()));
            ঘূৰাই;
        }
        যদি (AstConsQueue* ধ্রুৱ queuep = VN_CAST(nodep, ConsQueue)) {
            emit_Unkeyed_Cons_বস্তুসমূহ(queuep->itemsp(),
                                 tabModulus(queuep->dtypep()->skipRefp()->subDTypep()));
            ঘূৰাই;
        }
        যদি (AstConsStruct* ধ্রুৱ structp = VN_CAST(nodep, ConsStruct)) {
            emit_Unkeyed_Cons_বস্তুসমূহ(structp->itemsp(), 1);  // Members of mixed types
            ঘূৰাই;
        }
        যদি (VN_IS(nodep, Unbounded)) {
            // A public parameter's '$', which is only special during elaboration, so use zero
            puts("0");
            ঘূৰাই;
        }
        যদি (ধ্রুৱ AstConst* ধ্রুৱ constp = VN_CAST(nodep, Const)) {
            ধ্রুৱ V3_সংখ্যা& অঙ্ক = constp->অঙ্ক();
            UASSERT_OBJ(!অঙ্ক.isFourState(), constp, "4-state value in emitted constant");
            যদি (!constp->isWide()) {
                puts(অঙ্ক.emitC());
                ঘূৰাই;
            }

            ofp()->putsNoTracking("{");  // The initializer of m_storage within the VlWide
            ধ্রুৱ পূর্ণাঙ্ক শব্দগুলি = constp->width_শব্দগুলি();
            ধ্রুৱ পূর্ণাঙ্ক tabMod = 4;
            যদি (শব্দগুলি > tabMod) puts("\n");
            বাবে (পূর্ণাঙ্ক n = 0; n < শব্দগুলি; ++n) {
                যদি (n) puts((n % tabMod) ? ", " : ",\n");
                ofp()->printf("0x%08" PRIx32, অঙ্ক.edataWord(n));
            }
            যদি (শব্দগুলি > tabMod) puts("\n");
            ofp()->putsNoTracking("}");
            ঘূৰাই;
        }
        nodep->v3fatalSrc("Unknown node type in emitInitializerBody: " << nodep->prettyTypeName());
    }

    // Emit the braced initializer for the given value '{...}'
    শূন্য emitInitializer(Ast_গ্ৰন্থি_ৰাশিমালা* nodep) {
        putns(nodep, "{");
        emitInitializerBody(nodep);
        puts("}");
    }

রক্ষিত:
    // Emit a C++ value constructor: 'Type{...}'
    শূন্য emitValCons(Ast_গ্ৰন্থি_Cons* nodep) {
        putnbs(nodep, nodep->dtypep()->cType("", false, false));
        emitInitializer(nodep);
    }

    // Emit the C++ variable definition 'Type name{...}'
    শূন্য emitVarDefn(ধ্রুৱ AstVar* varp) {
        ধ্রুৱ std::string name = varp->constPoolEntry()
                                     ? EmitCUtil::constPoolName(varp)
                                     : EmitCUtil::prefixNameProtect(EmitCParentModule::get(varp))
                                           + "::" + varp->nameProtect();
        putnbs(varp, varp->dtypep()->cType(name, false, false));
        emitInitializer(VN_AS(varp->valuep(), গ্ৰন্থি_ৰাশিমালা));
    }
};
