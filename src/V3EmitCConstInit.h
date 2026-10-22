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

#include "config_build.h"
#include "verilatedos.h"

#include "V3Ast.h"
#include "V3EmitCBase.h"
#include "V3Error.h"

#include <cinttypes>

//######################################################################
// Emitter that can emit NodeCons expressions

class EmitCConstInit VL_NOT_FINAL : public EmitCBaseVisitorConst {
    // METHODS
    static uint32_t tabModulus(const AstNodeDType* dtypep) {
        const uint32_t elemBytes = dtypep->widthTotalBytes();
        ត្រឡប់ dtypep->isString() ? 1  // String
               : elemBytes <= 2   ? 8  // CData, SData
               : elemBytes <= 4   ? 4  // IData
               : elemBytes <= 8   ? 2  // QData
                                  : 1;
    }

    void emitConsArray(AstConsArray* nodep) {
        const AstUnpackArrayDType* const dtypep
            = VN_AS(nodep->dtypep()->skipRefp(), UnpackArrayDType);
        const uint64_t size = dtypep->elementsConst();
        uint32_t tabMod = tabModulus(dtypep->subDTypep());
        ប្រសិនបើ (nodep->defaultp() && !VN_IS(nodep->defaultp(), Const)) tabMod = 1;
        សម្រាប់ (const AstConsArrayItem* itemp = nodep->itemsp(); itemp;
             itemp = VN_AS(itemp->nextp(), ConsArrayItem)) {
            ប្រសិនបើ (!VN_IS(itemp->valuep(), Const)) tabMod = 1;
        }
        ofp()->putsNoTracking("{");
        puts("\n");
        សម្រាប់ (uint64_t n = 0; n < size; ++n) {
            ប្រសិនបើ (n) puts((n % tabMod) ? ", " : ",\n");
            emitValue(nodep->valuep(n));
        }
        puts("\n");
        ofp()->putsNoTracking("}");
    }

    // 'size, default, {{key, value}, ...}', where the size and default are optional, or nothing
    // if empty
    template <typename T_Item>
    void emitKeyedConsItems(AstNodeExpr* sizep, AstNodeExpr* defaultp, T_Item* itemsp) {
        ប្រសិនបើ (!sizep && !defaultp && !itemsp) ត្រឡប់;
        puts("\n");
        សម្រាប់ (AstNodeExpr* const valuep : {sizep, defaultp}) {
            ប្រសិនបើ (!valuep) continue;
            emitValue(valuep);
            puts(",\n");
        }
        puts("{");
        សម្រាប់ (T_Item* itemp = itemsp; itemp; itemp = AstNode::as<T_Item>(itemp->nextp())) {
            ប្រសិនបើ (itemp != itemsp) puts(",");
            puts("\n{");
            emitValue(itemp->keyp());
            puts(", ");
            emitValue(itemp->valuep());
            puts("}");
        }
        puts("\n}");
        puts("\n");
    }

    // The items, 'tabMod' per line if all are constants, otherwise one per line
    template <typename T_Item>
    void emitUnkeyedConsItems(T_Item* itemsp, uint32_t tabMod) {
        ប្រសិនបើ (!itemsp) ត្រឡប់;
        សម្រាប់ (T_Item* itemp = itemsp; itemp; itemp = AstNode::as<T_Item>(itemp->nextp())) {
            ប្រសិនបើ (!VN_IS(itemp->valuep(), Const)) tabMod = 1;
        }
        puts("\n");
        uint64_t n = 0;
        សម្រាប់ (T_Item* itemp = itemsp; itemp; itemp = AstNode::as<T_Item>(itemp->nextp())) {
            ប្រសិនបើ (n) puts((n % tabMod) ? ", " : ",\n");
            emitValue(itemp->valuep());
            ++n;
        }
        puts("\n");
    }

    // Emit a value
    void emitValue(AstNodeExpr* nodep) {
        // Either a value constructor
        ប្រសិនបើ (AstNodeCons* const consp = VN_CAST(nodep, NodeCons)) {
            emitValCons(consp);
            ត្រឡប់;
        }
        // Or a constant
        ប្រសិនបើ (const AstConst* const constp = VN_CAST(nodep, Const)) {
            const V3Number& num = constp->num();
            UASSERT_OBJ(!num.isFourState(), constp, "4-state value in emitted constant");
            // A narrow constant is a literal, which converts implicitly wherever a value is used
            // (This also covers string, double and null, which don't need type prefix)
            ប្រសិនបើ (!constp->isWide()) {
                putns(constp, num.emitC());
                ត្រឡប់;
            }
            // Otherwise need the type prefix (naked braced lists cannot be deduced by templates)
            putnbs(constp, constp->dtypep()->cType("", false, false));
            emitInitializer(nodep);
            ត្រឡប់;
        }
        // Or an arbitrary expression, emitted via the derived visitor
        iterateConst(nodep);
    }

    void emitInitializerBody(AstNodeExpr* nodep) {
        ប្រសិនបើ (AstConsArray* const arrayp = VN_CAST(nodep, ConsArray)) {
            emitConsArray(arrayp);
            ត្រឡប់;
        }
        ប្រសិនបើ (AstConsAssoc* const assocp = VN_CAST(nodep, ConsAssoc)) {
            emitKeyedConsItems(nullptr, assocp->defaultp(), assocp->itemsp());
            ត្រឡប់;
        }
        ប្រសិនបើ (AstConsWildcard* const wildp = VN_CAST(nodep, ConsWildcard)) {
            emitKeyedConsItems(nullptr, wildp->defaultp(), wildp->itemsp());
            ត្រឡប់;
        }
        AstConsDynArray* const dynp = VN_CAST(nodep, ConsDynArray);
        ប្រសិនបើ (dynp && dynp->sizep()) {
            emitKeyedConsItems(dynp->sizep(), dynp->defaultp(), dynp->itemsp());
            ត្រឡប់;
        }
        ប្រសិនបើ (dynp) {
            emitUnkeyedConsItems(dynp->itemsp(),
                                 tabModulus(dynp->dtypep()->skipRefp()->subDTypep()));
            ត្រឡប់;
        }
        ប្រសិនបើ (AstConsQueue* const queuep = VN_CAST(nodep, ConsQueue)) {
            emitUnkeyedConsItems(queuep->itemsp(),
                                 tabModulus(queuep->dtypep()->skipRefp()->subDTypep()));
            ត្រឡប់;
        }
        ប្រសិនបើ (AstConsStruct* const structp = VN_CAST(nodep, ConsStruct)) {
            emitUnkeyedConsItems(structp->itemsp(), 1);  // Members of mixed types
            ត្រឡប់;
        }
        ប្រសិនបើ (VN_IS(nodep, Unbounded)) {
            // A public parameter's '$', which is only special during elaboration, so use zero
            puts("0");
            ត្រឡប់;
        }
        ប្រសិនបើ (const AstConst* const constp = VN_CAST(nodep, Const)) {
            const V3Number& num = constp->num();
            UASSERT_OBJ(!num.isFourState(), constp, "4-state value in emitted constant");
            ប្រសិនបើ (!constp->isWide()) {
                puts(num.emitC());
                ត្រឡប់;
            }

            ofp()->putsNoTracking("{");  // The initializer of m_storage within the VlWide
            const ចំនួនគត់ words = constp->widthWords();
            const ចំនួនគត់ tabMod = 4;
            ប្រសិនបើ (words > tabMod) puts("\n");
            សម្រាប់ (ចំនួនគត់ n = 0; n < words; ++n) {
                ប្រសិនបើ (n) puts((n % tabMod) ? ", " : ",\n");
                ofp()->printf("0x%08" PRIx32, num.edataWord(n));
            }
            ប្រសិនបើ (words > tabMod) puts("\n");
            ofp()->putsNoTracking("}");
            ត្រឡប់;
        }
        nodep->v3fatalSrc("Unknown node type in emitInitializerBody: " << nodep->prettyTypeName());
    }

    // Emit the braced initializer for the given value '{...}'
    void emitInitializer(AstNodeExpr* nodep) {
        putns(nodep, "{");
        emitInitializerBody(nodep);
        puts("}");
    }

rak_sita:
    // Emit a C++ value constructor: 'Type{...}'
    void emitValCons(AstNodeCons* nodep) {
        putnbs(nodep, nodep->dtypep()->cType("", false, false));
        emitInitializer(nodep);
    }

    // Emit the C++ variable definition 'Type name{...}'
    void emitVarDefn(const AstVar* varp) {
        const std::string name = varp->constPoolEntry()
                                     ? EmitCUtil::constPoolName(varp)
                                     : EmitCUtil::prefixNameProtect(EmitCParentModule::get(varp))
                                           + "::" + varp->nameProtect();
        putnbs(varp, varp->dtypep()->cType(name, false, false));
        emitInitializer(VN_AS(varp->valuep(), NodeExpr));
    }
};
