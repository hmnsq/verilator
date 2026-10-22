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
        ལོག dtypep->isString() ? 1  // String
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
        གལ་སྲིད (nodep->defaultp() && !VN_IS(nodep->defaultp(), Const)) tabMod = 1;
        དོན་ལུ (const AstConsArrayItem* itemp = nodep->itemsp(); itemp;
             itemp = VN_AS(itemp->nextp(), ConsArrayItem)) {
            གལ་སྲིད (!VN_IS(itemp->valuep(), Const)) tabMod = 1;
        }
        ofp()->putsNoTracking("{");
        puts("\n");
        དོན་ལུ (uint64_t n = 0; n < size; ++n) {
            གལ་སྲིད (n) puts((n % tabMod) ? ", " : ",\n");
            emitValue(nodep->valuep(n));
        }
        puts("\n");
        ofp()->putsNoTracking("}");
    }

    // 'size, default, {{key, value}, ...}', where the size and default are optional, or nothing
    // if empty
    template <typename T_Item>
    void emitKeyedConsItems(AstNodeExpr* sizep, AstNodeExpr* defaultp, T_Item* itemsp) {
        གལ་སྲིད (!sizep && !defaultp && !itemsp) ལོག;
        puts("\n");
        དོན་ལུ (AstNodeExpr* const valuep : {sizep, defaultp}) {
            གལ་སྲིད (!valuep) continue;
            emitValue(valuep);
            puts(",\n");
        }
        puts("{");
        དོན་ལུ (T_Item* itemp = itemsp; itemp; itemp = AstNode::as<T_Item>(itemp->nextp())) {
            གལ་སྲིད (itemp != itemsp) puts(",");
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
        གལ་སྲིད (!itemsp) ལོག;
        དོན་ལུ (T_Item* itemp = itemsp; itemp; itemp = AstNode::as<T_Item>(itemp->nextp())) {
            གལ་སྲིད (!VN_IS(itemp->valuep(), Const)) tabMod = 1;
        }
        puts("\n");
        uint64_t n = 0;
        དོན་ལུ (T_Item* itemp = itemsp; itemp; itemp = AstNode::as<T_Item>(itemp->nextp())) {
            གལ་སྲིད (n) puts((n % tabMod) ? ", " : ",\n");
            emitValue(itemp->valuep());
            ++n;
        }
        puts("\n");
    }

    // Emit a value
    void emitValue(AstNodeExpr* nodep) {
        // Either a value constructor
        གལ་སྲིད (AstNodeCons* const consp = VN_CAST(nodep, NodeCons)) {
            emitValCons(consp);
            ལོག;
        }
        // Or a constant
        གལ་སྲིད (const AstConst* const constp = VN_CAST(nodep, Const)) {
            const V3Number& num = constp->num();
            UASSERT_OBJ(!num.isFourState(), constp, "4-state value in emitted constant");
            // A narrow constant is a literal, which converts implicitly wherever a value is used
            // (This also covers string, double and null, which don't need type prefix)
            གལ་སྲིད (!constp->isWide()) {
                putns(constp, num.emitC());
                ལོག;
            }
            // Otherwise need the type prefix (naked braced lists cannot be deduced by templates)
            putnbs(constp, constp->dtypep()->cType("", false, false));
            emitInitializer(nodep);
            ལོག;
        }
        // Or an arbitrary expression, emitted via the derived visitor
        iterateConst(nodep);
    }

    void emitInitializerBody(AstNodeExpr* nodep) {
        གལ་སྲིད (AstConsArray* const arrayp = VN_CAST(nodep, ConsArray)) {
            emitConsArray(arrayp);
            ལོག;
        }
        གལ་སྲིད (AstConsAssoc* const assocp = VN_CAST(nodep, ConsAssoc)) {
            emitKeyedConsItems(nullptr, assocp->defaultp(), assocp->itemsp());
            ལོག;
        }
        གལ་སྲིད (AstConsWildcard* const wildp = VN_CAST(nodep, ConsWildcard)) {
            emitKeyedConsItems(nullptr, wildp->defaultp(), wildp->itemsp());
            ལོག;
        }
        AstConsDynArray* const dynp = VN_CAST(nodep, ConsDynArray);
        གལ་སྲིད (dynp && dynp->sizep()) {
            emitKeyedConsItems(dynp->sizep(), dynp->defaultp(), dynp->itemsp());
            ལོག;
        }
        གལ་སྲིད (dynp) {
            emitUnkeyedConsItems(dynp->itemsp(),
                                 tabModulus(dynp->dtypep()->skipRefp()->subDTypep()));
            ལོག;
        }
        གལ་སྲིད (AstConsQueue* const queuep = VN_CAST(nodep, ConsQueue)) {
            emitUnkeyedConsItems(queuep->itemsp(),
                                 tabModulus(queuep->dtypep()->skipRefp()->subDTypep()));
            ལོག;
        }
        གལ་སྲིད (AstConsStruct* const structp = VN_CAST(nodep, ConsStruct)) {
            emitUnkeyedConsItems(structp->itemsp(), 1);  // Members of mixed types
            ལོག;
        }
        གལ་སྲིད (VN_IS(nodep, Unbounded)) {
            // A public parameter's '$', which is only special during elaboration, so use zero
            puts("0");
            ལོག;
        }
        གལ་སྲིད (const AstConst* const constp = VN_CAST(nodep, Const)) {
            const V3Number& num = constp->num();
            UASSERT_OBJ(!num.isFourState(), constp, "4-state value in emitted constant");
            གལ་སྲིད (!constp->isWide()) {
                puts(num.emitC());
                ལོག;
            }

            ofp()->putsNoTracking("{");  // The initializer of m_storage within the VlWide
            const ཧྲིལ words = constp->widthWords();
            const ཧྲིལ tabMod = 4;
            གལ་སྲིད (words > tabMod) puts("\n");
            དོན་ལུ (ཧྲིལ n = 0; n < words; ++n) {
                གལ་སྲིད (n) puts((n % tabMod) ? ", " : ",\n");
                ofp()->printf("0x%08" PRIx32, num.edataWord(n));
            }
            གལ་སྲིད (words > tabMod) puts("\n");
            ofp()->putsNoTracking("}");
            ལོག;
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
