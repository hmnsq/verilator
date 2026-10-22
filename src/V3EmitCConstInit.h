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

class EmitCConstInit VL_NOT_FINAL : সার্বজনিক EmitCBaseVisitorConst {
    // METHODS
    স্থির uint32_t tabModulus(ধ্রুব Ast_গ্রন্থি_DType* dtypep) {
        ধ্রুব uint32_t elemBytes = dtypep->widthTotalBytes();
        প্রত্যাগম dtypep->isString() ? 1  // String
               : elemBytes <= 2   ? 8  // CData, SData
               : elemBytes <= 4   ? 4  // IData
               : elemBytes <= 8   ? 2  // QData
                                  : 1;
    }

    শূন্য emitConsArray(AstConsArray* nodep) {
        ধ্রুব AstUnpackArrayDType* ধ্রুব dtypep
            = VN_AS(nodep->dtypep()->skipRefp(), UnpackArrayDType);
        ধ্রুব uint64_t size = dtypep->elementsConst();
        uint32_t tabMod = tabModulus(dtypep->subDTypep());
        যদি (nodep->defaultp() && !VN_IS(nodep->defaultp(), Const)) tabMod = 1;
        প্রতি (ধ্রুব AstConsArrayItem* itemp = nodep->itemsp(); itemp;
             itemp = VN_AS(itemp->nextp(), ConsArrayItem)) {
            যদি (!VN_IS(itemp->valuep(), Const)) tabMod = 1;
        }
        ofp()->putsNoTracking("{");
        puts("\n");
        প্রতি (uint64_t n = 0; n < size; ++n) {
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
        যদি (!sizep && !defaultp && !itemsp) প্রত্যাগম;
        puts("\n");
        প্রতি (Ast_গ্রন্থি_রাশিমালা* ধ্রুব valuep : {sizep, defaultp}) {
            যদি (!valuep) অনুবর্ত;
            emit_মান(valuep);
            puts(",\n");
        }
        puts("{");
        প্রতি (T_Item* itemp = itemsp; itemp; itemp = Ast_গ্রন্থি::as<T_Item>(itemp->nextp())) {
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
        যদি (!itemsp) প্রত্যাগম;
        প্রতি (T_Item* itemp = itemsp; itemp; itemp = Ast_গ্রন্থি::as<T_Item>(itemp->nextp())) {
            যদি (!VN_IS(itemp->valuep(), Const)) tabMod = 1;
        }
        puts("\n");
        uint64_t n = 0;
        প্রতি (T_Item* itemp = itemsp; itemp; itemp = Ast_গ্রন্থি::as<T_Item>(itemp->nextp())) {
            যদি (n) puts((n % tabMod) ? ", " : ",\n");
            emit_মান(itemp->valuep());
            ++n;
        }
        puts("\n");
    }

    // Emit a value
    শূন্য emit_মান(Ast_গ্রন্থি_রাশিমালা* nodep) {
        // Either a value constructor
        যদি (Ast_গ্রন্থি_Cons* ধ্রুব consp = VN_CAST(nodep, গ্রন্থি_Cons)) {
            emitValCons(consp);
            প্রত্যাগম;
        }
        // Or a constant
        যদি (ধ্রুব AstConst* ধ্রুব constp = VN_CAST(nodep, Const)) {
            ধ্রুব V3_সংখ্যা& অঙ্ক = constp->অঙ্ক();
            UASSERT_OBJ(!অঙ্ক.isFourState(), constp, "4-state value in emitted constant");
            // A narrow constant is a literal, which converts implicitly wherever a value is used
            // (This also covers string, double and null, which don't need type prefix)
            যদি (!constp->isWide()) {
                putns(constp, অঙ্ক.emitC());
                প্রত্যাগম;
            }
            // Otherwise need the type prefix (naked braced lists cannot be deduced by templates)
            putnbs(constp, constp->dtypep()->cType("", false, false));
            emitInitializer(nodep);
            প্রত্যাগম;
        }
        // Or an arbitrary expression, emitted via the derived visitor
        iterateConst(nodep);
    }

    শূন্য emitInitializerBody(Ast_গ্রন্থি_রাশিমালা* nodep) {
        যদি (AstConsArray* ধ্রুব arrayp = VN_CAST(nodep, ConsArray)) {
            emitConsArray(arrayp);
            প্রত্যাগম;
        }
        যদি (AstConsAssoc* ধ্রুব assocp = VN_CAST(nodep, ConsAssoc)) {
            emit_Keyed_Cons_বস্তুসমূহ(nullptr, assocp->defaultp(), assocp->itemsp());
            প্রত্যাগম;
        }
        যদি (AstConsWildcard* ধ্রুব wildp = VN_CAST(nodep, ConsWildcard)) {
            emit_Keyed_Cons_বস্তুসমূহ(nullptr, wildp->defaultp(), wildp->itemsp());
            প্রত্যাগম;
        }
        AstConsDynArray* ধ্রুব dynp = VN_CAST(nodep, ConsDynArray);
        যদি (dynp && dynp->sizep()) {
            emit_Keyed_Cons_বস্তুসমূহ(dynp->sizep(), dynp->defaultp(), dynp->itemsp());
            প্রত্যাগম;
        }
        যদি (dynp) {
            emit_Unkeyed_Cons_বস্তুসমূহ(dynp->itemsp(),
                                 tabModulus(dynp->dtypep()->skipRefp()->subDTypep()));
            প্রত্যাগম;
        }
        যদি (AstConsQueue* ধ্রুব queuep = VN_CAST(nodep, ConsQueue)) {
            emit_Unkeyed_Cons_বস্তুসমূহ(queuep->itemsp(),
                                 tabModulus(queuep->dtypep()->skipRefp()->subDTypep()));
            প্রত্যাগম;
        }
        যদি (AstConsStruct* ধ্রুব structp = VN_CAST(nodep, ConsStruct)) {
            emit_Unkeyed_Cons_বস্তুসমূহ(structp->itemsp(), 1);  // Members of mixed types
            প্রত্যাগম;
        }
        যদি (VN_IS(nodep, Unbounded)) {
            // A public parameter's '$', which is only special during elaboration, so use zero
            puts("0");
            প্রত্যাগম;
        }
        যদি (ধ্রুব AstConst* ধ্রুব constp = VN_CAST(nodep, Const)) {
            ধ্রুব V3_সংখ্যা& অঙ্ক = constp->অঙ্ক();
            UASSERT_OBJ(!অঙ্ক.isFourState(), constp, "4-state value in emitted constant");
            যদি (!constp->isWide()) {
                puts(অঙ্ক.emitC());
                প্রত্যাগম;
            }

            ofp()->putsNoTracking("{");  // The initializer of m_storage within the VlWide
            ধ্রুব পূর্ণাঙ্ক শব্দগুলি = constp->width_শব্দগুলি();
            ধ্রুব পূর্ণাঙ্ক tabMod = 4;
            যদি (শব্দগুলি > tabMod) puts("\n");
            প্রতি (পূর্ণাঙ্ক n = 0; n < শব্দগুলি; ++n) {
                যদি (n) puts((n % tabMod) ? ", " : ",\n");
                ofp()->printf("0x%08" PRIx32, অঙ্ক.edataWord(n));
            }
            যদি (শব্দগুলি > tabMod) puts("\n");
            ofp()->putsNoTracking("}");
            প্রত্যাগম;
        }
        nodep->v3fatalSrc("Unknown node type in emitInitializerBody: " << nodep->prettyTypeName());
    }

    // Emit the braced initializer for the given value '{...}'
    শূন্য emitInitializer(Ast_গ্রন্থি_রাশিমালা* nodep) {
        putns(nodep, "{");
        emitInitializerBody(nodep);
        puts("}");
    }

রক্ষিত:
    // Emit a C++ value constructor: 'Type{...}'
    শূন্য emitValCons(Ast_গ্রন্থি_Cons* nodep) {
        putnbs(nodep, nodep->dtypep()->cType("", false, false));
        emitInitializer(nodep);
    }

    // Emit the C++ variable definition 'Type name{...}'
    শূন্য emitVarDefn(ধ্রুব AstVar* varp) {
        ধ্রুব std::string name = varp->constPoolEntry()
                                     ? EmitCUtil::constPoolName(varp)
                                     : EmitCUtil::prefixNameProtect(EmitCParentModule::get(varp))
                                           + "::" + varp->nameProtect();
        putnbs(varp, varp->dtypep()->cType(name, false, false));
        emitInitializer(VN_AS(varp->valuep(), গ্রন্থি_রাশিমালা));
    }
};
