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

#شامل "config_build.h"
#شامل "verilatedos.h"

#شامل "V3Ast.h"
#شامل "V3EmitCBase.h"
#شامل "V3Error.h"

#شامل <cinttypes>

//######################################################################
// Emitter that can emit NodeCons expressions

class EmitCConstInit VL_NOT_FINAL : عمومی EmitCBaseVisitorConst {
    // METHODS
    ایستا uint32_t tabModulus(ثابت Ast_گره_DType* dtypep) {
        ثابت uint32_t elemBytes = dtypep->widthTotalBytes();
        بازگشت dtypep->isString() ? 1  // String
               : elemBytes <= 2   ? 8  // CData, SData
               : elemBytes <= 4   ? 4  // IData
               : elemBytes <= 8   ? 2  // QData
                                  : 1;
    }

    تهی emitConsArray(AstConsArray* nodep) {
        ثابت AstUnpackArrayDType* ثابت dtypep
            = VN_AS(nodep->dtypep()->skipRefp(), UnpackArrayDType);
        ثابت uint64_t size = dtypep->elementsConst();
        uint32_t tabMod = tabModulus(dtypep->subDTypep());
        اگر (nodep->defaultp() && !VN_IS(nodep->defaultp(), Const)) tabMod = 1;
        برای (ثابت AstConsArrayItem* itemp = nodep->itemsp(); itemp;
             itemp = VN_AS(itemp->nextp(), ConsArrayItem)) {
            اگر (!VN_IS(itemp->valuep(), Const)) tabMod = 1;
        }
        ofp()->putsNoTracking("{");
        puts("\n");
        برای (uint64_t n = 0; n < size; ++n) {
            اگر (n) puts((n % tabMod) ? ", " : ",\n");
            emit_مقدار(nodep->valuep(n));
        }
        puts("\n");
        ofp()->putsNoTracking("}");
    }

    // 'size, default, {{key, value}, ...}', where the size and default are optional, or nothing
    // if empty
    template <typename T_Item>
    تهی emit_Keyed_Cons_اقلام(Ast_گره_عبارت* sizep, Ast_گره_عبارت* defaultp, T_Item* itemsp) {
        اگر (!sizep && !defaultp && !itemsp) بازگشت;
        puts("\n");
        برای (Ast_گره_عبارت* ثابت valuep : {sizep, defaultp}) {
            اگر (!valuep) ادامه;
            emit_مقدار(valuep);
            puts(",\n");
        }
        puts("{");
        برای (T_Item* itemp = itemsp; itemp; itemp = Ast_گره::as<T_Item>(itemp->nextp())) {
            اگر (itemp != itemsp) puts(",");
            puts("\n{");
            emit_مقدار(itemp->keyp());
            puts(", ");
            emit_مقدار(itemp->valuep());
            puts("}");
        }
        puts("\n}");
        puts("\n");
    }

    // The items, 'tabMod' per line if all are constants, otherwise one per line
    template <typename T_Item>
    تهی emit_Unkeyed_Cons_اقلام(T_Item* itemsp, uint32_t tabMod) {
        اگر (!itemsp) بازگشت;
        برای (T_Item* itemp = itemsp; itemp; itemp = Ast_گره::as<T_Item>(itemp->nextp())) {
            اگر (!VN_IS(itemp->valuep(), Const)) tabMod = 1;
        }
        puts("\n");
        uint64_t n = 0;
        برای (T_Item* itemp = itemsp; itemp; itemp = Ast_گره::as<T_Item>(itemp->nextp())) {
            اگر (n) puts((n % tabMod) ? ", " : ",\n");
            emit_مقدار(itemp->valuep());
            ++n;
        }
        puts("\n");
    }

    // Emit a value
    تهی emit_مقدار(Ast_گره_عبارت* nodep) {
        // Either a value constructor
        اگر (Ast_گره_Cons* ثابت consp = VN_CAST(nodep, گره_Cons)) {
            emitValCons(consp);
            بازگشت;
        }
        // Or a constant
        اگر (ثابت AstConst* ثابت constp = VN_CAST(nodep, Const)) {
            ثابت V3_عدد& رقم = constp->رقم();
            UASSERT_OBJ(!رقم.isFourState(), constp, "4-state value in emitted constant");
            // A narrow constant is a literal, which converts implicitly wherever a value is used
            // (This also covers string, double and null, which don't need type prefix)
            اگر (!constp->isWide()) {
                putns(constp, رقم.emitC());
                بازگشت;
            }
            // Otherwise need the type prefix (naked braced lists cannot be deduced by templates)
            putnbs(constp, constp->dtypep()->cType("", false, false));
            emitInitializer(nodep);
            بازگشت;
        }
        // Or an arbitrary expression, emitted via the derived visitor
        iterateConst(nodep);
    }

    تهی emitInitializerBody(Ast_گره_عبارت* nodep) {
        اگر (AstConsArray* ثابت arrayp = VN_CAST(nodep, ConsArray)) {
            emitConsArray(arrayp);
            بازگشت;
        }
        اگر (AstConsAssoc* ثابت assocp = VN_CAST(nodep, ConsAssoc)) {
            emit_Keyed_Cons_اقلام(nullptr, assocp->defaultp(), assocp->itemsp());
            بازگشت;
        }
        اگر (AstConsWildcard* ثابت wildp = VN_CAST(nodep, ConsWildcard)) {
            emit_Keyed_Cons_اقلام(nullptr, wildp->defaultp(), wildp->itemsp());
            بازگشت;
        }
        AstConsDynArray* ثابت dynp = VN_CAST(nodep, ConsDynArray);
        اگر (dynp && dynp->sizep()) {
            emit_Keyed_Cons_اقلام(dynp->sizep(), dynp->defaultp(), dynp->itemsp());
            بازگشت;
        }
        اگر (dynp) {
            emit_Unkeyed_Cons_اقلام(dynp->itemsp(),
                                 tabModulus(dynp->dtypep()->skipRefp()->subDTypep()));
            بازگشت;
        }
        اگر (AstConsQueue* ثابت queuep = VN_CAST(nodep, ConsQueue)) {
            emit_Unkeyed_Cons_اقلام(queuep->itemsp(),
                                 tabModulus(queuep->dtypep()->skipRefp()->subDTypep()));
            بازگشت;
        }
        اگر (AstConsStruct* ثابت structp = VN_CAST(nodep, ConsStruct)) {
            emit_Unkeyed_Cons_اقلام(structp->itemsp(), 1);  // Members of mixed types
            بازگشت;
        }
        اگر (VN_IS(nodep, Unbounded)) {
            // A public parameter's '$', which is only special during elaboration, so use zero
            puts("0");
            بازگشت;
        }
        اگر (ثابت AstConst* ثابت constp = VN_CAST(nodep, Const)) {
            ثابت V3_عدد& رقم = constp->رقم();
            UASSERT_OBJ(!رقم.isFourState(), constp, "4-state value in emitted constant");
            اگر (!constp->isWide()) {
                puts(رقم.emitC());
                بازگشت;
            }

            ofp()->putsNoTracking("{");  // The initializer of m_storage within the VlWide
            ثابت صحیح واژگان = constp->width_واژگان();
            ثابت صحیح tabMod = 4;
            اگر (واژگان > tabMod) puts("\n");
            برای (صحیح n = 0; n < واژگان; ++n) {
                اگر (n) puts((n % tabMod) ? ", " : ",\n");
                ofp()->printf("0x%08" PRIx32, رقم.edataWord(n));
            }
            اگر (واژگان > tabMod) puts("\n");
            ofp()->putsNoTracking("}");
            بازگشت;
        }
        nodep->v3fatalSrc("Unknown node type in emitInitializerBody: " << nodep->prettyTypeName());
    }

    // Emit the braced initializer for the given value '{...}'
    تهی emitInitializer(Ast_گره_عبارت* nodep) {
        putns(nodep, "{");
        emitInitializerBody(nodep);
        puts("}");
    }

محافظتشده:
    // Emit a C++ value constructor: 'Type{...}'
    تهی emitValCons(Ast_گره_Cons* nodep) {
        putnbs(nodep, nodep->dtypep()->cType("", false, false));
        emitInitializer(nodep);
    }

    // Emit the C++ variable definition 'Type name{...}'
    تهی emitVarDefn(ثابت AstVar* varp) {
        ثابت std::string name = varp->constPoolEntry()
                                     ? EmitCUtil::constPoolName(varp)
                                     : EmitCUtil::prefixNameProtect(EmitCParentModule::get(varp))
                                           + "::" + varp->nameProtect();
        putnbs(varp, varp->dtypep()->cType(name, false, false));
        emitInitializer(VN_AS(varp->valuep(), گره_عبارت));
    }
};
