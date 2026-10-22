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
    स्थिर uint32_t tabModulus(ध्रुव Ast_गाँठ_DType* dtypep) {
        ध्रुव uint32_t elemBytes = dtypep->widthTotalBytes();
        प्रत्यागम dtypep->isString() ? 1  // String
               : elemBytes <= 2   ? 8  // CData, SData
               : elemBytes <= 4   ? 4  // IData
               : elemBytes <= 8   ? 2  // QData
                                  : 1;
    }

    शून्य emitConsArray(AstConsArray* nodep) {
        ध्रुव AstUnpackArrayDType* ध्रुव dtypep
            = VN_AS(nodep->dtypep()->skipRefp(), UnpackArrayDType);
        ध्रुव uint64_t size = dtypep->elementsConst();
        uint32_t tabMod = tabModulus(dtypep->subDTypep());
        यदि (nodep->defaultp() && !VN_IS(nodep->defaultp(), Const)) tabMod = 1;
        प्रति (ध्रुव AstConsArrayItem* itemp = nodep->itemsp(); itemp;
             itemp = VN_AS(itemp->nextp(), ConsArrayItem)) {
            यदि (!VN_IS(itemp->valuep(), Const)) tabMod = 1;
        }
        ofp()->putsNoTracking("{");
        माला_दो("\n");
        प्रति (uint64_t n = 0; n < size; ++n) {
            यदि (n) माला_दो((n % tabMod) ? ", " : ",\n");
            emit_मूल्य(nodep->valuep(n));
        }
        माला_दो("\n");
        ofp()->putsNoTracking("}");
    }

    // 'size, default, {{key, value}, ...}', where the size and default are optional, or nothing
    // if empty
    ढाँचा <typename T_Item>
    शून्य emit_Keyed_Cons_वस्तुएँ(Ast_गाँठ_व्यंजक* sizep, Ast_गाँठ_व्यंजक* defaultp, T_Item* itemsp) {
        यदि (!sizep && !defaultp && !itemsp) प्रत्यागम;
        माला_दो("\n");
        प्रति (Ast_गाँठ_व्यंजक* ध्रुव valuep : {sizep, defaultp}) {
            यदि (!valuep) अनुवर्त;
            emit_मूल्य(valuep);
            माला_दो(",\n");
        }
        माला_दो("{");
        प्रति (T_Item* itemp = itemsp; itemp; itemp = Ast_गाँठ::as<T_Item>(itemp->nextp())) {
            यदि (itemp != itemsp) माला_दो(",");
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
    शून्य emit_Unkeyed_Cons_वस्तुएँ(T_Item* itemsp, uint32_t tabMod) {
        यदि (!itemsp) प्रत्यागम;
        प्रति (T_Item* itemp = itemsp; itemp; itemp = Ast_गाँठ::as<T_Item>(itemp->nextp())) {
            यदि (!VN_IS(itemp->valuep(), Const)) tabMod = 1;
        }
        माला_दो("\n");
        uint64_t n = 0;
        प्रति (T_Item* itemp = itemsp; itemp; itemp = Ast_गाँठ::as<T_Item>(itemp->nextp())) {
            यदि (n) माला_दो((n % tabMod) ? ", " : ",\n");
            emit_मूल्य(itemp->valuep());
            ++n;
        }
        माला_दो("\n");
    }

    // Emit a value
    शून्य emit_मूल्य(Ast_गाँठ_व्यंजक* nodep) {
        // Either a value constructor
        यदि (Ast_गाँठ_Cons* ध्रुव consp = VN_CAST(nodep, गाँठ_Cons)) {
            emitValCons(consp);
            प्रत्यागम;
        }
        // Or a constant
        यदि (ध्रुव AstConst* ध्रुव constp = VN_CAST(nodep, Const)) {
            ध्रुव V3_संख्या& अंक = constp->अंक();
            UASSERT_OBJ(!अंक.isFourState(), constp, "4-state value in emitted constant");
            // A narrow constant is a literal, which converts implicitly wherever a value is used
            // (This also covers string, double and null, which don't need type prefix)
            यदि (!constp->isWide()) {
                putns(constp, अंक.emitC());
                प्रत्यागम;
            }
            // Otherwise need the type prefix (naked braced lists cannot be deduced by templates)
            putnbs(constp, constp->dtypep()->cType("", false, false));
            emitInitializer(nodep);
            प्रत्यागम;
        }
        // Or an arbitrary expression, emitted via the derived visitor
        iterateConst(nodep);
    }

    शून्य emitInitializerBody(Ast_गाँठ_व्यंजक* nodep) {
        यदि (AstConsArray* ध्रुव arrayp = VN_CAST(nodep, ConsArray)) {
            emitConsArray(arrayp);
            प्रत्यागम;
        }
        यदि (AstConsAssoc* ध्रुव assocp = VN_CAST(nodep, ConsAssoc)) {
            emit_Keyed_Cons_वस्तुएँ(nullptr, assocp->defaultp(), assocp->itemsp());
            प्रत्यागम;
        }
        यदि (AstConsWildcard* ध्रुव wildp = VN_CAST(nodep, ConsWildcard)) {
            emit_Keyed_Cons_वस्तुएँ(nullptr, wildp->defaultp(), wildp->itemsp());
            प्रत्यागम;
        }
        AstConsDynArray* ध्रुव dynp = VN_CAST(nodep, ConsDynArray);
        यदि (dynp && dynp->sizep()) {
            emit_Keyed_Cons_वस्तुएँ(dynp->sizep(), dynp->defaultp(), dynp->itemsp());
            प्रत्यागम;
        }
        यदि (dynp) {
            emit_Unkeyed_Cons_वस्तुएँ(dynp->itemsp(),
                                 tabModulus(dynp->dtypep()->skipRefp()->subDTypep()));
            प्रत्यागम;
        }
        यदि (AstConsQueue* ध्रुव queuep = VN_CAST(nodep, ConsQueue)) {
            emit_Unkeyed_Cons_वस्तुएँ(queuep->itemsp(),
                                 tabModulus(queuep->dtypep()->skipRefp()->subDTypep()));
            प्रत्यागम;
        }
        यदि (AstConsStruct* ध्रुव structp = VN_CAST(nodep, ConsStruct)) {
            emit_Unkeyed_Cons_वस्तुएँ(structp->itemsp(), 1);  // Members of mixed types
            प्रत्यागम;
        }
        यदि (VN_IS(nodep, Unbounded)) {
            // A public parameter's '$', which is only special during elaboration, so use zero
            माला_दो("0");
            प्रत्यागम;
        }
        यदि (ध्रुव AstConst* ध्रुव constp = VN_CAST(nodep, Const)) {
            ध्रुव V3_संख्या& अंक = constp->अंक();
            UASSERT_OBJ(!अंक.isFourState(), constp, "4-state value in emitted constant");
            यदि (!constp->isWide()) {
                माला_दो(अंक.emitC());
                प्रत्यागम;
            }

            ofp()->putsNoTracking("{");  // The initializer of m_storage within the VlWide
            ध्रुव पूर्णाङ्क शब्दगण = constp->width_शब्दगण();
            ध्रुव पूर्णाङ्क tabMod = 4;
            यदि (शब्दगण > tabMod) माला_दो("\n");
            प्रति (पूर्णाङ्क n = 0; n < शब्दगण; ++n) {
                यदि (n) माला_दो((n % tabMod) ? ", " : ",\n");
                ofp()->म_लिखो("0x%08" PRIx32, अंक.edataWord(n));
            }
            यदि (शब्दगण > tabMod) माला_दो("\n");
            ofp()->putsNoTracking("}");
            प्रत्यागम;
        }
        nodep->v3fatalSrc("Unknown node type in emitInitializerBody: " << nodep->prettyTypeName());
    }

    // Emit the braced initializer for the given value '{...}'
    शून्य emitInitializer(Ast_गाँठ_व्यंजक* nodep) {
        putns(nodep, "{");
        emitInitializerBody(nodep);
        माला_दो("}");
    }

रक्षित:
    // Emit a C++ value constructor: 'Type{...}'
    शून्य emitValCons(Ast_गाँठ_Cons* nodep) {
        putnbs(nodep, nodep->dtypep()->cType("", false, false));
        emitInitializer(nodep);
    }

    // Emit the C++ variable definition 'Type name{...}'
    शून्य emitVarDefn(ध्रुव AstVar* varp) {
        ध्रुव std::string name = varp->constPoolEntry()
                                     ? EmitCUtil::constPoolName(varp)
                                     : EmitCUtil::prefixNameProtect(EmitCParentModule::get(varp))
                                           + "::" + varp->nameProtect();
        putnbs(varp, varp->dtypep()->cType(name, false, false));
        emitInitializer(VN_AS(varp->valuep(), गाँठ_व्यंजक));
    }
};
