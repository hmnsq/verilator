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

#அந்தர்பாவ "config_build.h"
#அந்தர்பாவ "verilatedos.h"

#அந்தர்பாவ "V3Ast.h"
#அந்தர்பாவ "V3EmitCBase.h"
#அந்தர்பாவ "V3Error.h"

#அந்தர்பாவ <cinttypes>

//######################################################################
// Emitter that can emit NodeCons expressions

class EmitCConstInit VL_NOT_FINAL : பொது EmitCBaseVisitorConst {
    // METHODS
    நிலையான uint32_t tabModulus(மாறிலி Ast_முனை_DType* dtypep) {
        மாறிலி uint32_t elemBytes = dtypep->widthTotalBytes();
        திரும்பு dtypep->isString() ? 1  // String
               : elemBytes <= 2   ? 8  // CData, SData
               : elemBytes <= 4   ? 4  // IData
               : elemBytes <= 8   ? 2  // QData
                                  : 1;
    }

    வெறுமை emitConsArray(AstConsArray* nodep) {
        மாறிலி AstUnpackArrayDType* மாறிலி dtypep
            = VN_AS(nodep->dtypep()->skipRefp(), UnpackArrayDType);
        மாறிலி uint64_t size = dtypep->elementsConst();
        uint32_t tabMod = tabModulus(dtypep->subDTypep());
        எனில் (nodep->defaultp() && !VN_IS(nodep->defaultp(), Const)) tabMod = 1;
        க்கு (மாறிலி AstConsArrayItem* itemp = nodep->itemsp(); itemp;
             itemp = VN_AS(itemp->nextp(), ConsArrayItem)) {
            எனில் (!VN_IS(itemp->valuep(), Const)) tabMod = 1;
        }
        ofp()->putsNoTracking("{");
        puts("\n");
        க்கு (uint64_t n = 0; n < size; ++n) {
            எனில் (n) puts((n % tabMod) ? ", " : ",\n");
            emit_மதிப்பு(nodep->valuep(n));
        }
        puts("\n");
        ofp()->putsNoTracking("}");
    }

    // 'size, default, {{key, value}, ...}', where the size and default are optional, or nothing
    // if empty
    template <typename T_Item>
    வெறுமை emit_Keyed_Cons_பொருட்கள்(Ast_முனை_கோவை* sizep, Ast_முனை_கோவை* defaultp, T_Item* itemsp) {
        எனில் (!sizep && !defaultp && !itemsp) திரும்பு;
        puts("\n");
        க்கு (Ast_முனை_கோவை* மாறிலி valuep : {sizep, defaultp}) {
            எனில் (!valuep) தொடர்;
            emit_மதிப்பு(valuep);
            puts(",\n");
        }
        puts("{");
        க்கு (T_Item* itemp = itemsp; itemp; itemp = Ast_முனை::as<T_Item>(itemp->nextp())) {
            எனில் (itemp != itemsp) puts(",");
            puts("\n{");
            emit_மதிப்பு(itemp->keyp());
            puts(", ");
            emit_மதிப்பு(itemp->valuep());
            puts("}");
        }
        puts("\n}");
        puts("\n");
    }

    // The items, 'tabMod' per line if all are constants, otherwise one per line
    template <typename T_Item>
    வெறுமை emit_Unkeyed_Cons_பொருட்கள்(T_Item* itemsp, uint32_t tabMod) {
        எனில் (!itemsp) திரும்பு;
        க்கு (T_Item* itemp = itemsp; itemp; itemp = Ast_முனை::as<T_Item>(itemp->nextp())) {
            எனில் (!VN_IS(itemp->valuep(), Const)) tabMod = 1;
        }
        puts("\n");
        uint64_t n = 0;
        க்கு (T_Item* itemp = itemsp; itemp; itemp = Ast_முனை::as<T_Item>(itemp->nextp())) {
            எனில் (n) puts((n % tabMod) ? ", " : ",\n");
            emit_மதிப்பு(itemp->valuep());
            ++n;
        }
        puts("\n");
    }

    // Emit a value
    வெறுமை emit_மதிப்பு(Ast_முனை_கோவை* nodep) {
        // Either a value constructor
        எனில் (Ast_முனை_Cons* மாறிலி consp = VN_CAST(nodep, முனை_Cons)) {
            emitValCons(consp);
            திரும்பு;
        }
        // Or a constant
        எனில் (மாறிலி AstConst* மாறிலி constp = VN_CAST(nodep, Const)) {
            மாறிலி V3_எண்& இலக்கம் = constp->இலக்கம்();
            UASSERT_OBJ(!இலக்கம்.isFourState(), constp, "4-state value in emitted constant");
            // A narrow constant is a literal, which converts implicitly wherever a value is used
            // (This also covers string, double and null, which don't need type prefix)
            எனில் (!constp->isWide()) {
                putns(constp, இலக்கம்.emitC());
                திரும்பு;
            }
            // Otherwise need the type prefix (naked braced lists cannot be deduced by templates)
            putnbs(constp, constp->dtypep()->cType("", false, false));
            emitInitializer(nodep);
            திரும்பு;
        }
        // Or an arbitrary expression, emitted via the derived visitor
        iterateConst(nodep);
    }

    வெறுமை emitInitializerBody(Ast_முனை_கோவை* nodep) {
        எனில் (AstConsArray* மாறிலி arrayp = VN_CAST(nodep, ConsArray)) {
            emitConsArray(arrayp);
            திரும்பு;
        }
        எனில் (AstConsAssoc* மாறிலி assocp = VN_CAST(nodep, ConsAssoc)) {
            emit_Keyed_Cons_பொருட்கள்(nullptr, assocp->defaultp(), assocp->itemsp());
            திரும்பு;
        }
        எனில் (AstConsWildcard* மாறிலி wildp = VN_CAST(nodep, ConsWildcard)) {
            emit_Keyed_Cons_பொருட்கள்(nullptr, wildp->defaultp(), wildp->itemsp());
            திரும்பு;
        }
        AstConsDynArray* மாறிலி dynp = VN_CAST(nodep, ConsDynArray);
        எனில் (dynp && dynp->sizep()) {
            emit_Keyed_Cons_பொருட்கள்(dynp->sizep(), dynp->defaultp(), dynp->itemsp());
            திரும்பு;
        }
        எனில் (dynp) {
            emit_Unkeyed_Cons_பொருட்கள்(dynp->itemsp(),
                                 tabModulus(dynp->dtypep()->skipRefp()->subDTypep()));
            திரும்பு;
        }
        எனில் (AstConsQueue* மாறிலி queuep = VN_CAST(nodep, ConsQueue)) {
            emit_Unkeyed_Cons_பொருட்கள்(queuep->itemsp(),
                                 tabModulus(queuep->dtypep()->skipRefp()->subDTypep()));
            திரும்பு;
        }
        எனில் (AstConsStruct* மாறிலி structp = VN_CAST(nodep, ConsStruct)) {
            emit_Unkeyed_Cons_பொருட்கள்(structp->itemsp(), 1);  // Members of mixed types
            திரும்பு;
        }
        எனில் (VN_IS(nodep, Unbounded)) {
            // A public parameter's '$', which is only special during elaboration, so use zero
            puts("0");
            திரும்பு;
        }
        எனில் (மாறிலி AstConst* மாறிலி constp = VN_CAST(nodep, Const)) {
            மாறிலி V3_எண்& இலக்கம் = constp->இலக்கம்();
            UASSERT_OBJ(!இலக்கம்.isFourState(), constp, "4-state value in emitted constant");
            எனில் (!constp->isWide()) {
                puts(இலக்கம்.emitC());
                திரும்பு;
            }

            ofp()->putsNoTracking("{");  // The initializer of m_storage within the VlWide
            மாறிலி முழு சொற்கள் = constp->width_சொற்கள்();
            மாறிலி முழு tabMod = 4;
            எனில் (சொற்கள் > tabMod) puts("\n");
            க்கு (முழு n = 0; n < சொற்கள்; ++n) {
                எனில் (n) puts((n % tabMod) ? ", " : ",\n");
                ofp()->printf("0x%08" PRIx32, இலக்கம்.edataWord(n));
            }
            எனில் (சொற்கள் > tabMod) puts("\n");
            ofp()->putsNoTracking("}");
            திரும்பு;
        }
        nodep->v3fatalSrc("Unknown node type in emitInitializerBody: " << nodep->prettyTypeName());
    }

    // Emit the braced initializer for the given value '{...}'
    வெறுமை emitInitializer(Ast_முனை_கோவை* nodep) {
        putns(nodep, "{");
        emitInitializerBody(nodep);
        puts("}");
    }

பாதுகாக்கப்பட்ட:
    // Emit a C++ value constructor: 'Type{...}'
    வெறுமை emitValCons(Ast_முனை_Cons* nodep) {
        putnbs(nodep, nodep->dtypep()->cType("", false, false));
        emitInitializer(nodep);
    }

    // Emit the C++ variable definition 'Type name{...}'
    வெறுமை emitVarDefn(மாறிலி AstVar* varp) {
        மாறிலி std::string name = varp->constPoolEntry()
                                     ? EmitCUtil::constPoolName(varp)
                                     : EmitCUtil::prefixNameProtect(EmitCParentModule::get(varp))
                                           + "::" + varp->nameProtect();
        putnbs(varp, varp->dtypep()->cType(name, false, false));
        emitInitializer(VN_AS(varp->valuep(), முனை_கோவை));
    }
};
