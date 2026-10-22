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

#અન્તર્ભાવ "config_build.h"
#અન્તર્ભાવ "verilatedos.h"

#અન્તર્ભાવ "V3Ast.h"
#અન્તર્ભાવ "V3EmitCBase.h"
#અન્તર્ભાવ "V3Error.h"

#અન્તર્ભાવ <cinttypes>

//######################################################################
// Emitter that can emit NodeCons expressions

class EmitCConstInit VL_NOT_FINAL : જાહેર EmitCBaseVisitorConst {
    // METHODS
    સ્થિર uint32_t tabModulus(અચલ Ast_ગાંઠ_DType* dtypep) {
        અચલ uint32_t elemBytes = dtypep->widthTotalBytes();
        પાછા dtypep->isString() ? 1  // String
               : elemBytes <= 2   ? 8  // CData, SData
               : elemBytes <= 4   ? 4  // IData
               : elemBytes <= 8   ? 2  // QData
                                  : 1;
    }

    શૂન્ય emitConsArray(AstConsArray* nodep) {
        અચલ AstUnpackArrayDType* અચલ dtypep
            = VN_AS(nodep->dtypep()->skipRefp(), UnpackArrayDType);
        અચલ uint64_t size = dtypep->elementsConst();
        uint32_t tabMod = tabModulus(dtypep->subDTypep());
        જો (nodep->defaultp() && !VN_IS(nodep->defaultp(), Const)) tabMod = 1;
        માટે (અચલ AstConsArrayItem* itemp = nodep->itemsp(); itemp;
             itemp = VN_AS(itemp->nextp(), ConsArrayItem)) {
            જો (!VN_IS(itemp->valuep(), Const)) tabMod = 1;
        }
        ofp()->putsNoTracking("{");
        puts("\n");
        માટે (uint64_t n = 0; n < size; ++n) {
            જો (n) puts((n % tabMod) ? ", " : ",\n");
            emit_કિંમત(nodep->valuep(n));
        }
        puts("\n");
        ofp()->putsNoTracking("}");
    }

    // 'size, default, {{key, value}, ...}', where the size and default are optional, or nothing
    // if empty
    template <typename T_Item>
    શૂન્ય emit_Keyed_Cons_વસ્તુઓ(Ast_ગાંઠ_પદાવલિ* sizep, Ast_ગાંઠ_પદાવલિ* defaultp, T_Item* itemsp) {
        જો (!sizep && !defaultp && !itemsp) પાછા;
        puts("\n");
        માટે (Ast_ગાંઠ_પદાવલિ* અચલ valuep : {sizep, defaultp}) {
            જો (!valuep) ચાલુ;
            emit_કિંમત(valuep);
            puts(",\n");
        }
        puts("{");
        માટે (T_Item* itemp = itemsp; itemp; itemp = Ast_ગાંઠ::as<T_Item>(itemp->nextp())) {
            જો (itemp != itemsp) puts(",");
            puts("\n{");
            emit_કિંમત(itemp->keyp());
            puts(", ");
            emit_કિંમત(itemp->valuep());
            puts("}");
        }
        puts("\n}");
        puts("\n");
    }

    // The items, 'tabMod' per line if all are constants, otherwise one per line
    template <typename T_Item>
    શૂન્ય emit_Unkeyed_Cons_વસ્તુઓ(T_Item* itemsp, uint32_t tabMod) {
        જો (!itemsp) પાછા;
        માટે (T_Item* itemp = itemsp; itemp; itemp = Ast_ગાંઠ::as<T_Item>(itemp->nextp())) {
            જો (!VN_IS(itemp->valuep(), Const)) tabMod = 1;
        }
        puts("\n");
        uint64_t n = 0;
        માટે (T_Item* itemp = itemsp; itemp; itemp = Ast_ગાંઠ::as<T_Item>(itemp->nextp())) {
            જો (n) puts((n % tabMod) ? ", " : ",\n");
            emit_કિંમત(itemp->valuep());
            ++n;
        }
        puts("\n");
    }

    // Emit a value
    શૂન્ય emit_કિંમત(Ast_ગાંઠ_પદાવલિ* nodep) {
        // Either a value constructor
        જો (Ast_ગાંઠ_Cons* અચલ consp = VN_CAST(nodep, ગાંઠ_Cons)) {
            emitValCons(consp);
            પાછા;
        }
        // Or a constant
        જો (અચલ AstConst* અચલ constp = VN_CAST(nodep, Const)) {
            અચલ V3_સંખ્યા& અંક = constp->અંક();
            UASSERT_OBJ(!અંક.isFourState(), constp, "4-state value in emitted constant");
            // A narrow constant is a literal, which converts implicitly wherever a value is used
            // (This also covers string, double and null, which don't need type prefix)
            જો (!constp->isWide()) {
                putns(constp, અંક.emitC());
                પાછા;
            }
            // Otherwise need the type prefix (naked braced lists cannot be deduced by templates)
            putnbs(constp, constp->dtypep()->cType("", false, false));
            emitInitializer(nodep);
            પાછા;
        }
        // Or an arbitrary expression, emitted via the derived visitor
        iterateConst(nodep);
    }

    શૂન્ય emitInitializerBody(Ast_ગાંઠ_પદાવલિ* nodep) {
        જો (AstConsArray* અચલ arrayp = VN_CAST(nodep, ConsArray)) {
            emitConsArray(arrayp);
            પાછા;
        }
        જો (AstConsAssoc* અચલ assocp = VN_CAST(nodep, ConsAssoc)) {
            emit_Keyed_Cons_વસ્તુઓ(nullptr, assocp->defaultp(), assocp->itemsp());
            પાછા;
        }
        જો (AstConsWildcard* અચલ wildp = VN_CAST(nodep, ConsWildcard)) {
            emit_Keyed_Cons_વસ્તુઓ(nullptr, wildp->defaultp(), wildp->itemsp());
            પાછા;
        }
        AstConsDynArray* અચલ dynp = VN_CAST(nodep, ConsDynArray);
        જો (dynp && dynp->sizep()) {
            emit_Keyed_Cons_વસ્તુઓ(dynp->sizep(), dynp->defaultp(), dynp->itemsp());
            પાછા;
        }
        જો (dynp) {
            emit_Unkeyed_Cons_વસ્તુઓ(dynp->itemsp(),
                                 tabModulus(dynp->dtypep()->skipRefp()->subDTypep()));
            પાછા;
        }
        જો (AstConsQueue* અચલ queuep = VN_CAST(nodep, ConsQueue)) {
            emit_Unkeyed_Cons_વસ્તુઓ(queuep->itemsp(),
                                 tabModulus(queuep->dtypep()->skipRefp()->subDTypep()));
            પાછા;
        }
        જો (AstConsStruct* અચલ structp = VN_CAST(nodep, ConsStruct)) {
            emit_Unkeyed_Cons_વસ્તુઓ(structp->itemsp(), 1);  // Members of mixed types
            પાછા;
        }
        જો (VN_IS(nodep, Unbounded)) {
            // A public parameter's '$', which is only special during elaboration, so use zero
            puts("0");
            પાછા;
        }
        જો (અચલ AstConst* અચલ constp = VN_CAST(nodep, Const)) {
            અચલ V3_સંખ્યા& અંક = constp->અંક();
            UASSERT_OBJ(!અંક.isFourState(), constp, "4-state value in emitted constant");
            જો (!constp->isWide()) {
                puts(અંક.emitC());
                પાછા;
            }

            ofp()->putsNoTracking("{");  // The initializer of m_storage within the VlWide
            અચલ પૂર્ણાંક શબ્દો = constp->width_શબ્દો();
            અચલ પૂર્ણાંક tabMod = 4;
            જો (શબ્દો > tabMod) puts("\n");
            માટે (પૂર્ણાંક n = 0; n < શબ્દો; ++n) {
                જો (n) puts((n % tabMod) ? ", " : ",\n");
                ofp()->printf("0x%08" PRIx32, અંક.edataWord(n));
            }
            જો (શબ્દો > tabMod) puts("\n");
            ofp()->putsNoTracking("}");
            પાછા;
        }
        nodep->v3fatalSrc("Unknown node type in emitInitializerBody: " << nodep->prettyTypeName());
    }

    // Emit the braced initializer for the given value '{...}'
    શૂન્ય emitInitializer(Ast_ગાંઠ_પદાવલિ* nodep) {
        putns(nodep, "{");
        emitInitializerBody(nodep);
        puts("}");
    }

સુરક્ષિત:
    // Emit a C++ value constructor: 'Type{...}'
    શૂન્ય emitValCons(Ast_ગાંઠ_Cons* nodep) {
        putnbs(nodep, nodep->dtypep()->cType("", false, false));
        emitInitializer(nodep);
    }

    // Emit the C++ variable definition 'Type name{...}'
    શૂન્ય emitVarDefn(અચલ AstVar* varp) {
        અચલ std::string name = varp->constPoolEntry()
                                     ? EmitCUtil::constPoolName(varp)
                                     : EmitCUtil::prefixNameProtect(EmitCParentModule::get(varp))
                                           + "::" + varp->nameProtect();
        putnbs(varp, varp->dtypep()->cType(name, false, false));
        emitInitializer(VN_AS(varp->valuep(), ગાંઠ_પદાવલિ));
    }
};
