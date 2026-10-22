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

#समावेश "config_build.h"
#समावेश "verilatedos.h"

#समावेश "V3Ast.h"
#समावेश "V3EmitCBase.h"
#समावेश "V3Error.h"

#समावेश <cinttypes>

//######################################################################
// Emitter that can emit NodeCons expressions

class EmitCConstInit VL_NOT_FINAL : खुला EmitCBaseVisitorConst {
    // METHODS
    अटल uint32_t tabModulus(स्थिर Ast_गाँठ_DType* dtypep) {
        स्थिर uint32_t elemBytes = dtypep->width_योग_Bytes();
        वापस dtypep->isString() ? 1  // String
               : elemBytes <= 2   ? 8  // CData, SData
               : elemBytes <= 4   ? 4  // IData
               : elemBytes <= 8   ? 2  // QData
                                  : 1;
    }

    व्योम emitConsArray(AstConsArray* nodep) {
        स्थिर AstUnpackArrayDType* स्थिर dtypep
            = VN_AS(nodep->dtypep()->skipRefp(), UnpackArrayDType);
        स्थिर uint64_t आकार = dtypep->elementsConst();
        uint32_t tabMod = tabModulus(dtypep->subDTypep());
        अगर (nodep->defaultp() && !VN_IS(nodep->defaultp(), Const)) tabMod = 1;
        क्रम (स्थिर AstConsArrayItem* itemp = nodep->itemsp(); itemp;
             itemp = VN_AS(itemp->nextp(), ConsArrayItem)) {
            अगर (!VN_IS(itemp->valuep(), Const)) tabMod = 1;
        }
        ofp()->putsNoTracking("{");
        माला_दो("\n");
        क्रम (uint64_t n = 0; n < आकार; ++n) {
            अगर (n) माला_दो((n % tabMod) ? ", " : ",\n");
            emit_मूल्य(nodep->valuep(n));
        }
        माला_दो("\n");
        ofp()->putsNoTracking("}");
    }

    // 'size, default, {{key, value}, ...}', where the size and default are optional, or nothing
    // if empty
    ढाँचा <typename T_Item>
    व्योम emit_Keyed_Cons_वस्तुएँ(Ast_गाँठ_व्यंजक* sizep, Ast_गाँठ_व्यंजक* defaultp, T_Item* itemsp) {
        अगर (!sizep && !defaultp && !itemsp) वापस;
        माला_दो("\n");
        क्रम (Ast_गाँठ_व्यंजक* स्थिर valuep : {sizep, defaultp}) {
            अगर (!valuep) जारी;
            emit_मूल्य(valuep);
            माला_दो(",\n");
        }
        माला_दो("{");
        क्रम (T_Item* itemp = itemsp; itemp; itemp = Ast_गाँठ::as<T_Item>(itemp->nextp())) {
            अगर (itemp != itemsp) माला_दो(",");
            माला_दो("\n{");
            emit_मूल्य(itemp->keyp());
            माला_दो(", ");
            emit_मूल्य(itemp->valuep());
            माला_दो("}");
        }
        माला_दो("\n}");
        माला_दो("\n");
    }

    // The items, 'tabMod' per line if all are constants, otherwise one per line
    ढाँचा <typename T_Item>
    व्योम emit_Unkeyed_Cons_वस्तुएँ(T_Item* itemsp, uint32_t tabMod) {
        अगर (!itemsp) वापस;
        क्रम (T_Item* itemp = itemsp; itemp; itemp = Ast_गाँठ::as<T_Item>(itemp->nextp())) {
            अगर (!VN_IS(itemp->valuep(), Const)) tabMod = 1;
        }
        माला_दो("\n");
        uint64_t n = 0;
        क्रम (T_Item* itemp = itemsp; itemp; itemp = Ast_गाँठ::as<T_Item>(itemp->nextp())) {
            अगर (n) माला_दो((n % tabMod) ? ", " : ",\n");
            emit_मूल्य(itemp->valuep());
            ++n;
        }
        माला_दो("\n");
    }

    // Emit a value
    व्योम emit_मूल्य(Ast_गाँठ_व्यंजक* nodep) {
        // Either a value constructor
        अगर (Ast_गाँठ_Cons* स्थिर consp = VN_CAST(nodep, गाँठ_Cons)) {
            emitValCons(consp);
            वापस;
        }
        // Or a constant
        अगर (स्थिर AstConst* स्थिर constp = VN_CAST(nodep, Const)) {
            स्थिर V3_संख्या& अंक = constp->अंक();
            UASSERT_OBJ(!अंक.isFourState(), constp, "4-state value in emitted constant");
            // A narrow constant is a literal, which converts implicitly wherever a value is used
            // (This also covers string, double and null, which don't need type prefix)
            अगर (!constp->isWide()) {
                putns(constp, अंक.emitC());
                वापस;
            }
            // Otherwise need the type prefix (naked braced lists cannot be deduced by templates)
            putnbs(constp, constp->dtypep()->cType("", false, false));
            emitInitializer(nodep);
            वापस;
        }
        // Or an arbitrary expression, emitted via the derived visitor
        iterateConst(nodep);
    }

    व्योम emitInitializerBody(Ast_गाँठ_व्यंजक* nodep) {
        अगर (AstConsArray* स्थिर arrayp = VN_CAST(nodep, ConsArray)) {
            emitConsArray(arrayp);
            वापस;
        }
        अगर (AstConsAssoc* स्थिर assocp = VN_CAST(nodep, ConsAssoc)) {
            emit_Keyed_Cons_वस्तुएँ(nullptr, assocp->defaultp(), assocp->itemsp());
            वापस;
        }
        अगर (AstConsWildcard* स्थिर wildp = VN_CAST(nodep, ConsWildcard)) {
            emit_Keyed_Cons_वस्तुएँ(nullptr, wildp->defaultp(), wildp->itemsp());
            वापस;
        }
        AstConsDynArray* स्थिर dynp = VN_CAST(nodep, ConsDynArray);
        अगर (dynp && dynp->sizep()) {
            emit_Keyed_Cons_वस्तुएँ(dynp->sizep(), dynp->defaultp(), dynp->itemsp());
            वापस;
        }
        अगर (dynp) {
            emit_Unkeyed_Cons_वस्तुएँ(dynp->itemsp(),
                                 tabModulus(dynp->dtypep()->skipRefp()->subDTypep()));
            वापस;
        }
        अगर (AstConsQueue* स्थिर queuep = VN_CAST(nodep, ConsQueue)) {
            emit_Unkeyed_Cons_वस्तुएँ(queuep->itemsp(),
                                 tabModulus(queuep->dtypep()->skipRefp()->subDTypep()));
            वापस;
        }
        अगर (AstConsStruct* स्थिर structp = VN_CAST(nodep, ConsStruct)) {
            emit_Unkeyed_Cons_वस्तुएँ(structp->itemsp(), 1);  // Members of mixed types
            वापस;
        }
        अगर (VN_IS(nodep, Unbounded)) {
            // A public parameter's '$', which is only special during elaboration, so use zero
            माला_दो("0");
            वापस;
        }
        अगर (स्थिर AstConst* स्थिर constp = VN_CAST(nodep, Const)) {
            स्थिर V3_संख्या& अंक = constp->अंक();
            UASSERT_OBJ(!अंक.isFourState(), constp, "4-state value in emitted constant");
            अगर (!constp->isWide()) {
                माला_दो(अंक.emitC());
                वापस;
            }

            ofp()->putsNoTracking("{");  // The initializer of m_storage within the VlWide
            स्थिर पूर्णांक शब्दगण = constp->width_शब्दगण();
            स्थिर पूर्णांक tabMod = 4;
            अगर (शब्दगण > tabMod) माला_दो("\n");
            क्रम (पूर्णांक n = 0; n < शब्दगण; ++n) {
                अगर (n) माला_दो((n % tabMod) ? ", " : ",\n");
                ofp()->म_लिखो("0x%08" PRIx32, अंक.edataWord(n));
            }
            अगर (शब्दगण > tabMod) माला_दो("\n");
            ofp()->putsNoTracking("}");
            वापस;
        }
        nodep->v3fatalSrc("Unknown node type in emitInitializerBody: " << nodep->prettyTypeName());
    }

    // Emit the braced initializer for the given value '{...}'
    व्योम emitInitializer(Ast_गाँठ_व्यंजक* nodep) {
        putns(nodep, "{");
        emitInitializerBody(nodep);
        माला_दो("}");
    }

रक्षित:
    // Emit a C++ value constructor: 'Type{...}'
    व्योम emitValCons(Ast_गाँठ_Cons* nodep) {
        putnbs(nodep, nodep->dtypep()->cType("", false, false));
        emitInitializer(nodep);
    }

    // Emit the C++ variable definition 'Type name{...}'
    व्योम emitVarDefn(स्थिर AstVar* varp) {
        स्थिर std::string name = varp->constPoolEntry()
                                     ? EmitCUtil::constPoolName(varp)
                                     : EmitCUtil::prefixNameProtect(EmitCParentModule::get(varp))
                                           + "::" + varp->nameProtect();
        putnbs(varp, varp->dtypep()->cType(name, false, false));
        emitInitializer(VN_AS(varp->valuep(), गाँठ_व्यंजक));
    }
};
