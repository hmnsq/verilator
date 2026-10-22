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

#अन्तर्भाव "config_build.h"
#अन्तर्भाव "verilatedos.h"

#अन्तर्भाव "V3Ast.h"
#अन्तर्भाव "V3EmitCBase.h"
#अन्तर्भाव "V3Error.h"

#अन्तर्भाव <cinttypes>

//######################################################################
// Emitter that can emit NodeCons expressions

class EmitCConstInit VL_NOT_FINAL : सार्वजनिक EmitCBaseVisitorConst {
    // METHODS
    स्थिर uint32_t tabModulus(स्थिरांक Ast_गाठ_DType* dtypep) {
        स्थिरांक uint32_t elemBytes = dtypep->widthTotalBytes();
        परत dtypep->isString() ? 1  // String
               : elemBytes <= 2   ? 8  // CData, SData
               : elemBytes <= 4   ? 4  // IData
               : elemBytes <= 8   ? 2  // QData
                                  : 1;
    }

    शून्य emitConsArray(AstConsArray* nodep) {
        स्थिरांक AstUnpackArrayDType* स्थिरांक dtypep
            = VN_AS(nodep->dtypep()->skipRefp(), UnpackArrayDType);
        स्थिरांक uint64_t size = dtypep->elementsConst();
        uint32_t tabMod = tabModulus(dtypep->subDTypep());
        जर (nodep->defaultp() && !VN_IS(nodep->defaultp(), Const)) tabMod = 1;
        साठी (स्थिरांक AstConsArrayItem* itemp = nodep->itemsp(); itemp;
             itemp = VN_AS(itemp->nextp(), ConsArrayItem)) {
            जर (!VN_IS(itemp->valuep(), Const)) tabMod = 1;
        }
        ofp()->putsNoTracking("{");
        माला_दो("\n");
        साठी (uint64_t n = 0; n < size; ++n) {
            जर (n) माला_दो((n % tabMod) ? ", " : ",\n");
            emit_मूल्य(nodep->valuep(n));
        }
        माला_दो("\n");
        ofp()->putsNoTracking("}");
    }

    // 'size, default, {{key, value}, ...}', where the size and default are optional, or nothing
    // if empty
    ढाँचा <typename T_Item>
    शून्य emit_Keyed_Cons_वस्तुसंच(Ast_गाठ_अभिव्यक्ती* sizep, Ast_गाठ_अभिव्यक्ती* defaultp, T_Item* itemsp) {
        जर (!sizep && !defaultp && !itemsp) परत;
        माला_दो("\n");
        साठी (Ast_गाठ_अभिव्यक्ती* स्थिरांक valuep : {sizep, defaultp}) {
            जर (!valuep) सुरूठेवा;
            emit_मूल्य(valuep);
            माला_दो(",\n");
        }
        माला_दो("{");
        साठी (T_Item* itemp = itemsp; itemp; itemp = Ast_गाठ::as<T_Item>(itemp->nextp())) {
            जर (itemp != itemsp) माला_दो(",");
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
    शून्य emit_Unkeyed_Cons_वस्तुसंच(T_Item* itemsp, uint32_t tabMod) {
        जर (!itemsp) परत;
        साठी (T_Item* itemp = itemsp; itemp; itemp = Ast_गाठ::as<T_Item>(itemp->nextp())) {
            जर (!VN_IS(itemp->valuep(), Const)) tabMod = 1;
        }
        माला_दो("\n");
        uint64_t n = 0;
        साठी (T_Item* itemp = itemsp; itemp; itemp = Ast_गाठ::as<T_Item>(itemp->nextp())) {
            जर (n) माला_दो((n % tabMod) ? ", " : ",\n");
            emit_मूल्य(itemp->valuep());
            ++n;
        }
        माला_दो("\n");
    }

    // Emit a value
    शून्य emit_मूल्य(Ast_गाठ_अभिव्यक्ती* nodep) {
        // Either a value constructor
        जर (Ast_गाठ_Cons* स्थिरांक consp = VN_CAST(nodep, गाठ_Cons)) {
            emitValCons(consp);
            परत;
        }
        // Or a constant
        जर (स्थिरांक AstConst* स्थिरांक constp = VN_CAST(nodep, Const)) {
            स्थिरांक V3_संख्या& अंक = constp->अंक();
            UASSERT_OBJ(!अंक.isFourState(), constp, "4-state value in emitted constant");
            // A narrow constant is a literal, which converts implicitly wherever a value is used
            // (This also covers string, double and null, which don't need type prefix)
            जर (!constp->isWide()) {
                putns(constp, अंक.emitC());
                परत;
            }
            // Otherwise need the type prefix (naked braced lists cannot be deduced by templates)
            putnbs(constp, constp->dtypep()->cType("", false, false));
            emitInitializer(nodep);
            परत;
        }
        // Or an arbitrary expression, emitted via the derived visitor
        iterateConst(nodep);
    }

    शून्य emitInitializerBody(Ast_गाठ_अभिव्यक्ती* nodep) {
        जर (AstConsArray* स्थिरांक arrayp = VN_CAST(nodep, ConsArray)) {
            emitConsArray(arrayp);
            परत;
        }
        जर (AstConsAssoc* स्थिरांक assocp = VN_CAST(nodep, ConsAssoc)) {
            emit_Keyed_Cons_वस्तुसंच(nullptr, assocp->defaultp(), assocp->itemsp());
            परत;
        }
        जर (AstConsWildcard* स्थिरांक wildp = VN_CAST(nodep, ConsWildcard)) {
            emit_Keyed_Cons_वस्तुसंच(nullptr, wildp->defaultp(), wildp->itemsp());
            परत;
        }
        AstConsDynArray* स्थिरांक dynp = VN_CAST(nodep, ConsDynArray);
        जर (dynp && dynp->sizep()) {
            emit_Keyed_Cons_वस्तुसंच(dynp->sizep(), dynp->defaultp(), dynp->itemsp());
            परत;
        }
        जर (dynp) {
            emit_Unkeyed_Cons_वस्तुसंच(dynp->itemsp(),
                                 tabModulus(dynp->dtypep()->skipRefp()->subDTypep()));
            परत;
        }
        जर (AstConsQueue* स्थिरांक queuep = VN_CAST(nodep, ConsQueue)) {
            emit_Unkeyed_Cons_वस्तुसंच(queuep->itemsp(),
                                 tabModulus(queuep->dtypep()->skipRefp()->subDTypep()));
            परत;
        }
        जर (AstConsStruct* स्थिरांक structp = VN_CAST(nodep, ConsStruct)) {
            emit_Unkeyed_Cons_वस्तुसंच(structp->itemsp(), 1);  // Members of mixed types
            परत;
        }
        जर (VN_IS(nodep, Unbounded)) {
            // A public parameter's '$', which is only special during elaboration, so use zero
            माला_दो("0");
            परत;
        }
        जर (स्थिरांक AstConst* स्थिरांक constp = VN_CAST(nodep, Const)) {
            स्थिरांक V3_संख्या& अंक = constp->अंक();
            UASSERT_OBJ(!अंक.isFourState(), constp, "4-state value in emitted constant");
            जर (!constp->isWide()) {
                माला_दो(अंक.emitC());
                परत;
            }

            ofp()->putsNoTracking("{");  // The initializer of m_storage within the VlWide
            स्थिरांक पूर्णांक शब्द = constp->width_शब्द();
            स्थिरांक पूर्णांक tabMod = 4;
            जर (शब्द > tabMod) माला_दो("\n");
            साठी (पूर्णांक n = 0; n < शब्द; ++n) {
                जर (n) माला_दो((n % tabMod) ? ", " : ",\n");
                ofp()->म_लिखो("0x%08" PRIx32, अंक.edataWord(n));
            }
            जर (शब्द > tabMod) माला_दो("\n");
            ofp()->putsNoTracking("}");
            परत;
        }
        nodep->v3fatalSrc("Unknown node type in emitInitializerBody: " << nodep->prettyTypeName());
    }

    // Emit the braced initializer for the given value '{...}'
    शून्य emitInitializer(Ast_गाठ_अभिव्यक्ती* nodep) {
        putns(nodep, "{");
        emitInitializerBody(nodep);
        माला_दो("}");
    }

संरक्षित:
    // Emit a C++ value constructor: 'Type{...}'
    शून्य emitValCons(Ast_गाठ_Cons* nodep) {
        putnbs(nodep, nodep->dtypep()->cType("", false, false));
        emitInitializer(nodep);
    }

    // Emit the C++ variable definition 'Type name{...}'
    शून्य emitVarDefn(स्थिरांक AstVar* varp) {
        स्थिरांक std::string name = varp->constPoolEntry()
                                     ? EmitCUtil::constPoolName(varp)
                                     : EmitCUtil::prefixNameProtect(EmitCParentModule::get(varp))
                                           + "::" + varp->nameProtect();
        putnbs(varp, varp->dtypep()->cType(name, false, false));
        emitInitializer(VN_AS(varp->valuep(), गाठ_अभिव्यक्ती));
    }
};
