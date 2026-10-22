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

#ಅನ್ತರ್ಭಾವ "config_build.h"
#ಅನ್ತರ್ಭಾವ "verilatedos.h"

#ಅನ್ತರ್ಭಾವ "V3Ast.h"
#ಅನ್ತರ್ಭಾವ "V3EmitCBase.h"
#ಅನ್ತರ್ಭಾವ "V3Error.h"

#ಅನ್ತರ್ಭಾವ <cinttypes>

//######################################################################
// Emitter that can emit NodeCons expressions

class EmitCConstInit VL_NOT_FINAL : ಸಾರ್ವಜನಿಕ EmitCBaseVisitorConst {
    // METHODS
    ಸ್ಥಿರ uint32_t tabModulus(ಧ್ರುವ Ast_ಗಂಟು_DType* dtypep) {
        ಧ್ರುವ uint32_t elemBytes = dtypep->widthTotalBytes();
        ಪ್ರತ್ಯಾಗಮ dtypep->isString() ? 1  // String
               : elemBytes <= 2   ? 8  // CData, SData
               : elemBytes <= 4   ? 4  // IData
               : elemBytes <= 8   ? 2  // QData
                                  : 1;
    }

    ಶೂನ್ಯ emitConsArray(AstConsArray* nodep) {
        ಧ್ರುವ AstUnpackArrayDType* ಧ್ರುವ dtypep
            = VN_AS(nodep->dtypep()->skipRefp(), UnpackArrayDType);
        ಧ್ರುವ uint64_t size = dtypep->elementsConst();
        uint32_t tabMod = tabModulus(dtypep->subDTypep());
        ಯದಿ (nodep->defaultp() && !VN_IS(nodep->defaultp(), Const)) tabMod = 1;
        ಪ್ರತಿ (ಧ್ರುವ AstConsArrayItem* itemp = nodep->itemsp(); itemp;
             itemp = VN_AS(itemp->nextp(), ConsArrayItem)) {
            ಯದಿ (!VN_IS(itemp->valuep(), Const)) tabMod = 1;
        }
        ofp()->putsNoTracking("{");
        puts("\n");
        ಪ್ರತಿ (uint64_t n = 0; n < size; ++n) {
            ಯದಿ (n) puts((n % tabMod) ? ", " : ",\n");
            emit_ಮೌಲ್ಯ(nodep->valuep(n));
        }
        puts("\n");
        ofp()->putsNoTracking("}");
    }

    // 'size, default, {{key, value}, ...}', where the size and default are optional, or nothing
    // if empty
    template <typename T_Item>
    ಶೂನ್ಯ emit_Keyed_Cons_ವಸ್ತುಗಳು(Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* sizep, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* defaultp, T_Item* itemsp) {
        ಯದಿ (!sizep && !defaultp && !itemsp) ಪ್ರತ್ಯಾಗಮ;
        puts("\n");
        ಪ್ರತಿ (Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ valuep : {sizep, defaultp}) {
            ಯದಿ (!valuep) ಅನುವರ್ತ;
            emit_ಮೌಲ್ಯ(valuep);
            puts(",\n");
        }
        puts("{");
        ಪ್ರತಿ (T_Item* itemp = itemsp; itemp; itemp = Ast_ಗಂಟು::as<T_Item>(itemp->nextp())) {
            ಯದಿ (itemp != itemsp) puts(",");
            puts("\n{");
            emit_ಮೌಲ್ಯ(itemp->keyp());
            puts(", ");
            emit_ಮೌಲ್ಯ(itemp->valuep());
            puts("}");
        }
        puts("\n}");
        puts("\n");
    }

    // The items, 'tabMod' per line if all are constants, otherwise one per line
    template <typename T_Item>
    ಶೂನ್ಯ emit_Unkeyed_Cons_ವಸ್ತುಗಳು(T_Item* itemsp, uint32_t tabMod) {
        ಯದಿ (!itemsp) ಪ್ರತ್ಯಾಗಮ;
        ಪ್ರತಿ (T_Item* itemp = itemsp; itemp; itemp = Ast_ಗಂಟು::as<T_Item>(itemp->nextp())) {
            ಯದಿ (!VN_IS(itemp->valuep(), Const)) tabMod = 1;
        }
        puts("\n");
        uint64_t n = 0;
        ಪ್ರತಿ (T_Item* itemp = itemsp; itemp; itemp = Ast_ಗಂಟು::as<T_Item>(itemp->nextp())) {
            ಯದಿ (n) puts((n % tabMod) ? ", " : ",\n");
            emit_ಮೌಲ್ಯ(itemp->valuep());
            ++n;
        }
        puts("\n");
    }

    // Emit a value
    ಶೂನ್ಯ emit_ಮೌಲ್ಯ(Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* nodep) {
        // Either a value constructor
        ಯದಿ (Ast_ಗಂಟು_Cons* ಧ್ರುವ consp = VN_CAST(nodep, ಗಂಟು_Cons)) {
            emitValCons(consp);
            ಪ್ರತ್ಯಾಗಮ;
        }
        // Or a constant
        ಯದಿ (ಧ್ರುವ AstConst* ಧ್ರುವ constp = VN_CAST(nodep, Const)) {
            ಧ್ರುವ V3_ಸಂಖ್ಯೆ& ಅಂಕೆ = constp->ಅಂಕೆ();
            UASSERT_OBJ(!ಅಂಕೆ.isFourState(), constp, "4-state value in emitted constant");
            // A narrow constant is a literal, which converts implicitly wherever a value is used
            // (This also covers string, double and null, which don't need type prefix)
            ಯದಿ (!constp->isWide()) {
                putns(constp, ಅಂಕೆ.emitC());
                ಪ್ರತ್ಯಾಗಮ;
            }
            // Otherwise need the type prefix (naked braced lists cannot be deduced by templates)
            putnbs(constp, constp->dtypep()->cType("", false, false));
            emitInitializer(nodep);
            ಪ್ರತ್ಯಾಗಮ;
        }
        // Or an arbitrary expression, emitted via the derived visitor
        iterateConst(nodep);
    }

    ಶೂನ್ಯ emitInitializerBody(Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* nodep) {
        ಯದಿ (AstConsArray* ಧ್ರುವ arrayp = VN_CAST(nodep, ConsArray)) {
            emitConsArray(arrayp);
            ಪ್ರತ್ಯಾಗಮ;
        }
        ಯದಿ (AstConsAssoc* ಧ್ರುವ assocp = VN_CAST(nodep, ConsAssoc)) {
            emit_Keyed_Cons_ವಸ್ತುಗಳು(nullptr, assocp->defaultp(), assocp->itemsp());
            ಪ್ರತ್ಯಾಗಮ;
        }
        ಯದಿ (AstConsWildcard* ಧ್ರುವ wildp = VN_CAST(nodep, ConsWildcard)) {
            emit_Keyed_Cons_ವಸ್ತುಗಳು(nullptr, wildp->defaultp(), wildp->itemsp());
            ಪ್ರತ್ಯಾಗಮ;
        }
        AstConsDynArray* ಧ್ರುವ dynp = VN_CAST(nodep, ConsDynArray);
        ಯದಿ (dynp && dynp->sizep()) {
            emit_Keyed_Cons_ವಸ್ತುಗಳು(dynp->sizep(), dynp->defaultp(), dynp->itemsp());
            ಪ್ರತ್ಯಾಗಮ;
        }
        ಯದಿ (dynp) {
            emit_Unkeyed_Cons_ವಸ್ತುಗಳು(dynp->itemsp(),
                                 tabModulus(dynp->dtypep()->skipRefp()->subDTypep()));
            ಪ್ರತ್ಯಾಗಮ;
        }
        ಯದಿ (AstConsQueue* ಧ್ರುವ queuep = VN_CAST(nodep, ConsQueue)) {
            emit_Unkeyed_Cons_ವಸ್ತುಗಳು(queuep->itemsp(),
                                 tabModulus(queuep->dtypep()->skipRefp()->subDTypep()));
            ಪ್ರತ್ಯಾಗಮ;
        }
        ಯದಿ (AstConsStruct* ಧ್ರುವ structp = VN_CAST(nodep, ConsStruct)) {
            emit_Unkeyed_Cons_ವಸ್ತುಗಳು(structp->itemsp(), 1);  // Members of mixed types
            ಪ್ರತ್ಯಾಗಮ;
        }
        ಯದಿ (VN_IS(nodep, Unbounded)) {
            // A public parameter's '$', which is only special during elaboration, so use zero
            puts("0");
            ಪ್ರತ್ಯಾಗಮ;
        }
        ಯದಿ (ಧ್ರುವ AstConst* ಧ್ರುವ constp = VN_CAST(nodep, Const)) {
            ಧ್ರುವ V3_ಸಂಖ್ಯೆ& ಅಂಕೆ = constp->ಅಂಕೆ();
            UASSERT_OBJ(!ಅಂಕೆ.isFourState(), constp, "4-state value in emitted constant");
            ಯದಿ (!constp->isWide()) {
                puts(ಅಂಕೆ.emitC());
                ಪ್ರತ್ಯಾಗಮ;
            }

            ofp()->putsNoTracking("{");  // The initializer of m_storage within the VlWide
            ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ ಪದಗಳು = constp->width_ಪದಗಳು();
            ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ tabMod = 4;
            ಯದಿ (ಪದಗಳು > tabMod) puts("\n");
            ಪ್ರತಿ (ಪೂರ್ಣಾಙ್ಕ n = 0; n < ಪದಗಳು; ++n) {
                ಯದಿ (n) puts((n % tabMod) ? ", " : ",\n");
                ofp()->printf("0x%08" PRIx32, ಅಂಕೆ.edataWord(n));
            }
            ಯದಿ (ಪದಗಳು > tabMod) puts("\n");
            ofp()->putsNoTracking("}");
            ಪ್ರತ್ಯಾಗಮ;
        }
        nodep->v3fatalSrc("Unknown node type in emitInitializerBody: " << nodep->prettyTypeName());
    }

    // Emit the braced initializer for the given value '{...}'
    ಶೂನ್ಯ emitInitializer(Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* nodep) {
        putns(nodep, "{");
        emitInitializerBody(nodep);
        puts("}");
    }

ರಕ್ಷಿತ:
    // Emit a C++ value constructor: 'Type{...}'
    ಶೂನ್ಯ emitValCons(Ast_ಗಂಟು_Cons* nodep) {
        putnbs(nodep, nodep->dtypep()->cType("", false, false));
        emitInitializer(nodep);
    }

    // Emit the C++ variable definition 'Type name{...}'
    ಶೂನ್ಯ emitVarDefn(ಧ್ರುವ AstVar* varp) {
        ಧ್ರುವ std::string name = varp->constPoolEntry()
                                     ? EmitCUtil::constPoolName(varp)
                                     : EmitCUtil::prefixNameProtect(EmitCParentModule::get(varp))
                                           + "::" + varp->nameProtect();
        putnbs(varp, varp->dtypep()->cType(name, false, false));
        emitInitializer(VN_AS(varp->valuep(), ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ));
    }
};
