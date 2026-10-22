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
    स्थिर uint32_t tabModulus(ध्रुव Ast_ग्रन्थिः_DType* dtypep) {
        ध्रुव uint32_t elem_अष्टकानि = dtypep->विस्तारः_योग_अष्टकानि();
        प्रत्यागम dtypep->isString() ? 1  // String
               : elem_अष्टकानि <= 2   ? 8  // CData, SData
               : elem_अष्टकानि <= 4   ? 4  // IData
               : elem_अष्टकानि <= 8   ? 2  // QData
                                  : 1;
    }

    शून्य emitConsArray(AstConsArray* nodep) {
        ध्रुव AstUnpackArrayDType* ध्रुव dtypep
            = VN_AS(nodep->dtypep()->skipRefp(), UnpackArrayDType);
        ध्रुव uint64_t आकार = dtypep->elementsConst();
        uint32_t tabMod = tabModulus(dtypep->subDTypep());
        यदि (nodep->defaultp() && !VN_IS(nodep->defaultp(), Const)) tabMod = 1;
        प्रति (ध्रुव AstConsArrayItem* itemp = nodep->itemsp(); itemp;
             itemp = VN_AS(itemp->nextp(), ConsArrayItem)) {
            यदि (!VN_IS(itemp->valuep(), Const)) tabMod = 1;
        }
        ofp()->putsNoTracking("{");
        पङ्क्तिं_लिख("\n");
        प्रति (uint64_t n = 0; n < आकार; ++n) {
            यदि (n) पङ्क्तिं_लिख((n % tabMod) ? ", " : ",\n");
            emit_मानम्(nodep->valuep(n));
        }
        पङ्क्तिं_लिख("\n");
        ofp()->putsNoTracking("}");
    }

    // 'size, default, {{key, value}, ...}', where the size and default are optional, or nothing
    // if empty
    प्रतिरूपम् <typename T_Item>
    शून्य emit_Keyed_Cons_वस्तूनि(Ast_ग्रन्थिः_व्यञ्जकम्* sizep, Ast_ग्रन्थिः_व्यञ्जकम्* defaultp, T_Item* itemsp) {
        यदि (!sizep && !defaultp && !itemsp) प्रत्यागम;
        पङ्क्तिं_लिख("\n");
        प्रति (Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव valuep : {sizep, defaultp}) {
            यदि (!valuep) अनुवर्त;
            emit_मानम्(valuep);
            पङ्क्तिं_लिख(",\n");
        }
        पङ्क्तिं_लिख("{");
        प्रति (T_Item* itemp = itemsp; itemp; itemp = Ast_ग्रन्थिः::as<T_Item>(itemp->nextp())) {
            यदि (itemp != itemsp) पङ्क्तिं_लिख(",");
            पङ्क्तिं_लिख("\n{");
            emit_मानम्(itemp->keyp());
            पङ्क्तिं_लिख(", ");
            emit_मानम्(itemp->valuep());
            पङ्क्तिं_लिख("}");
        }
        पङ्क्तिं_लिख("\n}");
        पङ्क्तिं_लिख("\n");
    }

    // The items, 'tabMod' per line if all are constants, otherwise one per line
    प्रतिरूपम् <typename T_Item>
    शून्य emit_Unkeyed_Cons_वस्तूनि(T_Item* itemsp, uint32_t tabMod) {
        यदि (!itemsp) प्रत्यागम;
        प्रति (T_Item* itemp = itemsp; itemp; itemp = Ast_ग्रन्थिः::as<T_Item>(itemp->nextp())) {
            यदि (!VN_IS(itemp->valuep(), Const)) tabMod = 1;
        }
        पङ्क्तिं_लिख("\n");
        uint64_t n = 0;
        प्रति (T_Item* itemp = itemsp; itemp; itemp = Ast_ग्रन्थिः::as<T_Item>(itemp->nextp())) {
            यदि (n) पङ्क्तिं_लिख((n % tabMod) ? ", " : ",\n");
            emit_मानम्(itemp->valuep());
            ++n;
        }
        पङ्क्तिं_लिख("\n");
    }

    // Emit a value
    शून्य emit_मानम्(Ast_ग्रन्थिः_व्यञ्जकम्* nodep) {
        // Either a value constructor
        यदि (Ast_ग्रन्थिः_Cons* ध्रुव consp = VN_CAST(nodep, ग्रन्थिः_Cons)) {
            emitValCons(consp);
            प्रत्यागम;
        }
        // Or a constant
        यदि (ध्रुव AstConst* ध्रुव constp = VN_CAST(nodep, Const)) {
            ध्रुव V3_सङ्ख्या& अङ्कः = constp->अङ्कः();
            UASSERT_OBJ(!अङ्कः.isFourState(), constp, "4-state value in emitted constant");
            // A narrow constant is a literal, which converts implicitly wherever a value is used
            // (This also covers string, double and null, which don't need type prefix)
            यदि (!constp->isWide()) {
                putns(constp, अङ्कः.emitC());
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

    शून्य emitInitializerBody(Ast_ग्रन्थिः_व्यञ्जकम्* nodep) {
        यदि (AstConsArray* ध्रुव arrayp = VN_CAST(nodep, ConsArray)) {
            emitConsArray(arrayp);
            प्रत्यागम;
        }
        यदि (AstConsAssoc* ध्रुव assocp = VN_CAST(nodep, ConsAssoc)) {
            emit_Keyed_Cons_वस्तूनि(nullptr, assocp->defaultp(), assocp->itemsp());
            प्रत्यागम;
        }
        यदि (AstConsWildcard* ध्रुव wildp = VN_CAST(nodep, ConsWildcard)) {
            emit_Keyed_Cons_वस्तूनि(nullptr, wildp->defaultp(), wildp->itemsp());
            प्रत्यागम;
        }
        AstConsDynArray* ध्रुव dynp = VN_CAST(nodep, ConsDynArray);
        यदि (dynp && dynp->sizep()) {
            emit_Keyed_Cons_वस्तूनि(dynp->sizep(), dynp->defaultp(), dynp->itemsp());
            प्रत्यागम;
        }
        यदि (dynp) {
            emit_Unkeyed_Cons_वस्तूनि(dynp->itemsp(),
                                 tabModulus(dynp->dtypep()->skipRefp()->subDTypep()));
            प्रत्यागम;
        }
        यदि (AstConsQueue* ध्रुव queuep = VN_CAST(nodep, ConsQueue)) {
            emit_Unkeyed_Cons_वस्तूनि(queuep->itemsp(),
                                 tabModulus(queuep->dtypep()->skipRefp()->subDTypep()));
            प्रत्यागम;
        }
        यदि (AstConsStruct* ध्रुव structp = VN_CAST(nodep, ConsStruct)) {
            emit_Unkeyed_Cons_वस्तूनि(structp->itemsp(), 1);  // Members of mixed types
            प्रत्यागम;
        }
        यदि (VN_IS(nodep, Unbounded)) {
            // A public parameter's '$', which is only special during elaboration, so use zero
            पङ्क्तिं_लिख("0");
            प्रत्यागम;
        }
        यदि (ध्रुव AstConst* ध्रुव constp = VN_CAST(nodep, Const)) {
            ध्रुव V3_सङ्ख्या& अङ्कः = constp->अङ्कः();
            UASSERT_OBJ(!अङ्कः.isFourState(), constp, "4-state value in emitted constant");
            यदि (!constp->isWide()) {
                पङ्क्तिं_लिख(अङ्कः.emitC());
                प्रत्यागम;
            }

            ofp()->putsNoTracking("{");  // The initializer of m_storage within the VlWide
            ध्रुव पूर्णाङ्क शब्दाः = constp->विस्तारः_शब्दाः();
            ध्रुव पूर्णाङ्क tabMod = 4;
            यदि (शब्दाः > tabMod) पङ्क्तिं_लिख("\n");
            प्रति (पूर्णाङ्क n = 0; n < शब्दाः; ++n) {
                यदि (n) पङ्क्तिं_लिख((n % tabMod) ? ", " : ",\n");
                ofp()->प्रारूप_लिख("0x%08" PRIx32, अङ्कः.edataWord(n));
            }
            यदि (शब्दाः > tabMod) पङ्क्तिं_लिख("\n");
            ofp()->putsNoTracking("}");
            प्रत्यागम;
        }
        nodep->v3fatalSrc("Unknown node type in emitInitializerBody: " << nodep->prettyTypeName());
    }

    // Emit the braced initializer for the given value '{...}'
    शून्य emitInitializer(Ast_ग्रन्थिः_व्यञ्जकम्* nodep) {
        putns(nodep, "{");
        emitInitializerBody(nodep);
        पङ्क्तिं_लिख("}");
    }

रक्षित:
    // Emit a C++ value constructor: 'Type{...}'
    शून्य emitValCons(Ast_ग्रन्थिः_Cons* nodep) {
        putnbs(nodep, nodep->dtypep()->cType("", false, false));
        emitInitializer(nodep);
    }

    // Emit the C++ variable definition 'Type name{...}'
    शून्य emitVarDefn(ध्रुव AstVar* varp) {
        ध्रुव std::string name = varp->const_Pool_प्रविष्टिः()
                                     ? EmitCUtil::constPoolName(varp)
                                     : EmitCUtil::उपसर्गः_Name_Protect(EmitCParentModule::get(varp))
                                           + "::" + varp->nameProtect();
        putnbs(varp, varp->dtypep()->cType(name, false, false));
        emitInitializer(VN_AS(varp->valuep(), ग्रन्थिः_व्यञ्जकम्));
    }
};
