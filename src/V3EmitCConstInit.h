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
    स्थिर uint32_t tabModulus(अचर Ast_गाँठो_DType* dtypep) {
        अचर uint32_t elemBytes = dtypep->widthTotalBytes();
        फर्काऊ dtypep->isString() ? 1  // String
               : elemBytes <= 2   ? 8  // CData, SData
               : elemBytes <= 4   ? 4  // IData
               : elemBytes <= 8   ? 2  // QData
                                  : 1;
    }

    शून्य emitConsArray(AstConsArray* nodep) {
        अचर AstUnpackArrayDType* अचर dtypep
            = VN_AS(nodep->dtypep()->skipRefp(), UnpackArrayDType);
        अचर uint64_t size = dtypep->elementsConst();
        uint32_t tabMod = tabModulus(dtypep->subDTypep());
        यदि (nodep->defaultp() && !VN_IS(nodep->defaultp(), Const)) tabMod = 1;
        लागि (अचर AstConsArrayItem* itemp = nodep->itemsp(); itemp;
             itemp = VN_AS(itemp->nextp(), ConsArrayItem)) {
            यदि (!VN_IS(itemp->valuep(), Const)) tabMod = 1;
        }
        ofp()->putsNoTracking("{");
        माला_दो("\n");
        लागि (uint64_t n = 0; n < size; ++n) {
            यदि (n) माला_दो((n % tabMod) ? ", " : ",\n");
            emit_मान(nodep->valuep(n));
        }
        माला_दो("\n");
        ofp()->putsNoTracking("}");
    }

    // 'size, default, {{key, value}, ...}', where the size and default are optional, or nothing
    // if empty
    ढाँचा <typename T_Item>
    शून्य emit_Keyed_Cons_वस्तुहरू(Ast_गाँठो_अभिव्यक्ति* sizep, Ast_गाँठो_अभिव्यक्ति* defaultp, T_Item* itemsp) {
        यदि (!sizep && !defaultp && !itemsp) फर्काऊ;
        माला_दो("\n");
        लागि (Ast_गाँठो_अभिव्यक्ति* अचर valuep : {sizep, defaultp}) {
            यदि (!valuep) जारी;
            emit_मान(valuep);
            माला_दो(",\n");
        }
        माला_दो("{");
        लागि (T_Item* itemp = itemsp; itemp; itemp = Ast_गाँठो::as<T_Item>(itemp->nextp())) {
            यदि (itemp != itemsp) माला_दो(",");
            माला_दो("\n{");
            emit_मान(itemp->keyp());
            माला_दो(", ");
            emit_मान(itemp->valuep());
            माला_दो("}");
        }
        माला_दो("\n}");
        माला_दो("\n");
    }

    // The items, 'tabMod' per line if all are constants, otherwise one per line
    ढाँचा <typename T_Item>
    शून्य emit_Unkeyed_Cons_वस्तुहरू(T_Item* itemsp, uint32_t tabMod) {
        यदि (!itemsp) फर्काऊ;
        लागि (T_Item* itemp = itemsp; itemp; itemp = Ast_गाँठो::as<T_Item>(itemp->nextp())) {
            यदि (!VN_IS(itemp->valuep(), Const)) tabMod = 1;
        }
        माला_दो("\n");
        uint64_t n = 0;
        लागि (T_Item* itemp = itemsp; itemp; itemp = Ast_गाँठो::as<T_Item>(itemp->nextp())) {
            यदि (n) माला_दो((n % tabMod) ? ", " : ",\n");
            emit_मान(itemp->valuep());
            ++n;
        }
        माला_दो("\n");
    }

    // Emit a value
    शून्य emit_मान(Ast_गाँठो_अभिव्यक्ति* nodep) {
        // Either a value constructor
        यदि (Ast_गाँठो_Cons* अचर consp = VN_CAST(nodep, गाँठो_Cons)) {
            emitValCons(consp);
            फर्काऊ;
        }
        // Or a constant
        यदि (अचर AstConst* अचर constp = VN_CAST(nodep, Const)) {
            अचर V3_सङ्ख्या& अङ्क = constp->अङ्क();
            UASSERT_OBJ(!अङ्क.isFourState(), constp, "4-state value in emitted constant");
            // A narrow constant is a literal, which converts implicitly wherever a value is used
            // (This also covers string, double and null, which don't need type prefix)
            यदि (!constp->isWide()) {
                putns(constp, अङ्क.emitC());
                फर्काऊ;
            }
            // Otherwise need the type prefix (naked braced lists cannot be deduced by templates)
            putnbs(constp, constp->dtypep()->cType("", false, false));
            emitInitializer(nodep);
            फर्काऊ;
        }
        // Or an arbitrary expression, emitted via the derived visitor
        iterateConst(nodep);
    }

    शून्य emitInitializerBody(Ast_गाँठो_अभिव्यक्ति* nodep) {
        यदि (AstConsArray* अचर arrayp = VN_CAST(nodep, ConsArray)) {
            emitConsArray(arrayp);
            फर्काऊ;
        }
        यदि (AstConsAssoc* अचर assocp = VN_CAST(nodep, ConsAssoc)) {
            emit_Keyed_Cons_वस्तुहरू(nullptr, assocp->defaultp(), assocp->itemsp());
            फर्काऊ;
        }
        यदि (AstConsWildcard* अचर wildp = VN_CAST(nodep, ConsWildcard)) {
            emit_Keyed_Cons_वस्तुहरू(nullptr, wildp->defaultp(), wildp->itemsp());
            फर्काऊ;
        }
        AstConsDynArray* अचर dynp = VN_CAST(nodep, ConsDynArray);
        यदि (dynp && dynp->sizep()) {
            emit_Keyed_Cons_वस्तुहरू(dynp->sizep(), dynp->defaultp(), dynp->itemsp());
            फर्काऊ;
        }
        यदि (dynp) {
            emit_Unkeyed_Cons_वस्तुहरू(dynp->itemsp(),
                                 tabModulus(dynp->dtypep()->skipRefp()->subDTypep()));
            फर्काऊ;
        }
        यदि (AstConsQueue* अचर queuep = VN_CAST(nodep, ConsQueue)) {
            emit_Unkeyed_Cons_वस्तुहरू(queuep->itemsp(),
                                 tabModulus(queuep->dtypep()->skipRefp()->subDTypep()));
            फर्काऊ;
        }
        यदि (AstConsStruct* अचर structp = VN_CAST(nodep, ConsStruct)) {
            emit_Unkeyed_Cons_वस्तुहरू(structp->itemsp(), 1);  // Members of mixed types
            फर्काऊ;
        }
        यदि (VN_IS(nodep, Unbounded)) {
            // A public parameter's '$', which is only special during elaboration, so use zero
            माला_दो("0");
            फर्काऊ;
        }
        यदि (अचर AstConst* अचर constp = VN_CAST(nodep, Const)) {
            अचर V3_सङ्ख्या& अङ्क = constp->अङ्क();
            UASSERT_OBJ(!अङ्क.isFourState(), constp, "4-state value in emitted constant");
            यदि (!constp->isWide()) {
                माला_दो(अङ्क.emitC());
                फर्काऊ;
            }

            ofp()->putsNoTracking("{");  // The initializer of m_storage within the VlWide
            अचर पूर्णाङ्क शब्दहरू = constp->width_शब्दहरू();
            अचर पूर्णाङ्क tabMod = 4;
            यदि (शब्दहरू > tabMod) माला_दो("\n");
            लागि (पूर्णाङ्क n = 0; n < शब्दहरू; ++n) {
                यदि (n) माला_दो((n % tabMod) ? ", " : ",\n");
                ofp()->म_लिखो("0x%08" PRIx32, अङ्क.edataWord(n));
            }
            यदि (शब्दहरू > tabMod) माला_दो("\n");
            ofp()->putsNoTracking("}");
            फर्काऊ;
        }
        nodep->v3fatalSrc("Unknown node type in emitInitializerBody: " << nodep->prettyTypeName());
    }

    // Emit the braced initializer for the given value '{...}'
    शून्य emitInitializer(Ast_गाँठो_अभिव्यक्ति* nodep) {
        putns(nodep, "{");
        emitInitializerBody(nodep);
        माला_दो("}");
    }

रक्षित:
    // Emit a C++ value constructor: 'Type{...}'
    शून्य emitValCons(Ast_गाँठो_Cons* nodep) {
        putnbs(nodep, nodep->dtypep()->cType("", false, false));
        emitInitializer(nodep);
    }

    // Emit the C++ variable definition 'Type name{...}'
    शून्य emitVarDefn(अचर AstVar* varp) {
        अचर std::string name = varp->constPoolEntry()
                                     ? EmitCUtil::constPoolName(varp)
                                     : EmitCUtil::prefixNameProtect(EmitCParentModule::get(varp))
                                           + "::" + varp->nameProtect();
        putnbs(varp, varp->dtypep()->cType(name, false, false));
        emitInitializer(VN_AS(varp->valuep(), गाँठो_अभिव्यक्ति));
    }
};
