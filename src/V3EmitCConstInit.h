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
        जइ (nodep->defaultp() && !VN_IS(nodep->defaultp(), Const)) tabMod = 1;
        प्रति (ध्रुव AstConsArrayItem* itemp = nodep->itemsp(); itemp;
             itemp = VN_AS(itemp->nextp(), ConsArrayItem)) {
            जइ (!VN_IS(itemp->valuep(), Const)) tabMod = 1;
        }
        ofp()->putsNoTracking("{");
        puts("\n");
        प्रति (uint64_t n = 0; n < size; ++n) {
            जइ (n) puts((n % tabMod) ? ", " : ",\n");
            emit_मानम्(nodep->valuep(n));
        }
        puts("\n");
        ofp()->putsNoTracking("}");
    }

    // 'size, default, {{key, value}, ...}', where the size and default are optional, or nothing
    // if empty
    template <typename T_Item>
    शून्य emit_Keyed_Cons_वस्तूनि(Ast_ग्रन्थिः_व्यञ्जकम्* sizep, Ast_ग्रन्थिः_व्यञ्जकम्* defaultp, T_Item* itemsp) {
        जइ (!sizep && !defaultp && !itemsp) प्रत्यागम;
        puts("\n");
        प्रति (Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव valuep : {sizep, defaultp}) {
            जइ (!valuep) अनुवर्त;
            emit_मानम्(valuep);
            puts(",\n");
        }
        puts("{");
        प्रति (T_Item* itemp = itemsp; itemp; itemp = Ast_ग्रन्थिः::as<T_Item>(itemp->nextp())) {
            जइ (itemp != itemsp) puts(",");
            puts("\n{");
            emit_मानम्(itemp->keyp());
            puts(", ");
            emit_मानम्(itemp->valuep());
            puts("}");
        }
        puts("\n}");
        puts("\n");
    }

    // The items, 'tabMod' per line if all are constants, otherwise one per line
    template <typename T_Item>
    शून्य emit_Unkeyed_Cons_वस्तूनि(T_Item* itemsp, uint32_t tabMod) {
        जइ (!itemsp) प्रत्यागम;
        प्रति (T_Item* itemp = itemsp; itemp; itemp = Ast_ग्रन्थिः::as<T_Item>(itemp->nextp())) {
            जइ (!VN_IS(itemp->valuep(), Const)) tabMod = 1;
        }
        puts("\n");
        uint64_t n = 0;
        प्रति (T_Item* itemp = itemsp; itemp; itemp = Ast_ग्रन्थिः::as<T_Item>(itemp->nextp())) {
            जइ (n) puts((n % tabMod) ? ", " : ",\n");
            emit_मानम्(itemp->valuep());
            ++n;
        }
        puts("\n");
    }

    // Emit a value
    शून्य emit_मानम्(Ast_ग्रन्थिः_व्यञ्जकम्* nodep) {
        // Either a value constructor
        जइ (Ast_ग्रन्थिः_Cons* ध्रुव consp = VN_CAST(nodep, ग्रन्थिः_Cons)) {
            emitValCons(consp);
            प्रत्यागम;
        }
        // Or a constant
        जइ (ध्रुव AstConst* ध्रुव constp = VN_CAST(nodep, Const)) {
            ध्रुव V3_सङ्ख्या& अङ्कः = constp->अङ्कः();
            UASSERT_OBJ(!अङ्कः.isFourState(), constp, "4-state value in emitted constant");
            // A narrow constant is a literal, which converts implicitly wherever a value is used
            // (This also covers string, double and null, which don't need type prefix)
            जइ (!constp->isWide()) {
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
        जइ (AstConsArray* ध्रुव arrayp = VN_CAST(nodep, ConsArray)) {
            emitConsArray(arrayp);
            प्रत्यागम;
        }
        जइ (AstConsAssoc* ध्रुव assocp = VN_CAST(nodep, ConsAssoc)) {
            emit_Keyed_Cons_वस्तूनि(nullptr, assocp->defaultp(), assocp->itemsp());
            प्रत्यागम;
        }
        जइ (AstConsWildcard* ध्रुव wildp = VN_CAST(nodep, ConsWildcard)) {
            emit_Keyed_Cons_वस्तूनि(nullptr, wildp->defaultp(), wildp->itemsp());
            प्रत्यागम;
        }
        AstConsDynArray* ध्रुव dynp = VN_CAST(nodep, ConsDynArray);
        जइ (dynp && dynp->sizep()) {
            emit_Keyed_Cons_वस्तूनि(dynp->sizep(), dynp->defaultp(), dynp->itemsp());
            प्रत्यागम;
        }
        जइ (dynp) {
            emit_Unkeyed_Cons_वस्तूनि(dynp->itemsp(),
                                 tabModulus(dynp->dtypep()->skipRefp()->subDTypep()));
            प्रत्यागम;
        }
        जइ (AstConsQueue* ध्रुव queuep = VN_CAST(nodep, ConsQueue)) {
            emit_Unkeyed_Cons_वस्तूनि(queuep->itemsp(),
                                 tabModulus(queuep->dtypep()->skipRefp()->subDTypep()));
            प्रत्यागम;
        }
        जइ (AstConsStruct* ध्रुव structp = VN_CAST(nodep, ConsStruct)) {
            emit_Unkeyed_Cons_वस्तूनि(structp->itemsp(), 1);  // Members of mixed types
            प्रत्यागम;
        }
        जइ (VN_IS(nodep, Unbounded)) {
            // A public parameter's '$', which is only special during elaboration, so use zero
            puts("0");
            प्रत्यागम;
        }
        जइ (ध्रुव AstConst* ध्रुव constp = VN_CAST(nodep, Const)) {
            ध्रुव V3_सङ्ख्या& अङ्कः = constp->अङ्कः();
            UASSERT_OBJ(!अङ्कः.isFourState(), constp, "4-state value in emitted constant");
            जइ (!constp->isWide()) {
                puts(अङ्कः.emitC());
                प्रत्यागम;
            }

            ofp()->putsNoTracking("{");  // The initializer of m_storage within the VlWide
            ध्रुव पूर्णाङ्क शब्दाः = constp->width_शब्दाः();
            ध्रुव पूर्णाङ्क tabMod = 4;
            जइ (शब्दाः > tabMod) puts("\n");
            प्रति (पूर्णाङ्क n = 0; n < शब्दाः; ++n) {
                जइ (n) puts((n % tabMod) ? ", " : ",\n");
                ofp()->printf("0x%08" PRIx32, अङ्कः.edataWord(n));
            }
            जइ (शब्दाः > tabMod) puts("\n");
            ofp()->putsNoTracking("}");
            प्रत्यागम;
        }
        nodep->v3fatalSrc("Unknown node type in emitInitializerBody: " << nodep->prettyTypeName());
    }

    // Emit the braced initializer for the given value '{...}'
    शून्य emitInitializer(Ast_ग्रन्थिः_व्यञ्जकम्* nodep) {
        putns(nodep, "{");
        emitInitializerBody(nodep);
        puts("}");
    }

रक्षित:
    // Emit a C++ value constructor: 'Type{...}'
    शून्य emitValCons(Ast_ग्रन्थिः_Cons* nodep) {
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
        emitInitializer(VN_AS(varp->valuep(), ग्रन्थिः_व्यञ्जकम्));
    }
};
