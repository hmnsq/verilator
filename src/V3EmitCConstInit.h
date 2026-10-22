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

#ضمن "config_build.h"
#ضمن "verilatedos.h"

#ضمن "V3Ast.h"
#ضمن "V3EmitCBase.h"
#ضمن "V3Error.h"

#ضمن <cinttypes>

//######################################################################
// Emitter that can emit NodeCons expressions

class EmitCConstInit VL_NOT_FINAL : عام EmitCBaseVisitorConst {
    // METHODS
    ساكن uint32_t tabModulus(ثابت Ast_عقدة_DType* dtypep) {
        ثابت uint32_t elemBytes = dtypep->widthTotalBytes();
        أرجع dtypep->isString() ? 1  // String
               : elemBytes <= 2   ? 8  // CData, SData
               : elemBytes <= 4   ? 4  // IData
               : elemBytes <= 8   ? 2  // QData
                                  : 1;
    }

    فراغ emitConsArray(AstConsArray* nodep) {
        ثابت AstUnpackArrayDType* ثابت dtypep
            = VN_AS(nodep->dtypep()->skipRefp(), UnpackArrayDType);
        ثابت uint64_t size = dtypep->elementsConst();
        uint32_t tabMod = tabModulus(dtypep->subDTypep());
        إذا (nodep->defaultp() && !VN_IS(nodep->defaultp(), Const)) tabMod = 1;
        لكل (ثابت AstConsArrayItem* itemp = nodep->itemsp(); itemp;
             itemp = VN_AS(itemp->nextp(), ConsArrayItem)) {
            إذا (!VN_IS(itemp->valuep(), Const)) tabMod = 1;
        }
        ofp()->putsNoTracking("{");
        puts("\n");
        لكل (uint64_t n = 0; n < size; ++n) {
            إذا (n) puts((n % tabMod) ? ", " : ",\n");
            emit_قيمة(nodep->valuep(n));
        }
        puts("\n");
        ofp()->putsNoTracking("}");
    }

    // 'size, default, {{key, value}, ...}', where the size and default are optional, or nothing
    // if empty
    template <typename T_Item>
    فراغ emit_Keyed_Cons_عناصر(Ast_عقدة_تعبير* sizep, Ast_عقدة_تعبير* defaultp, T_Item* itemsp) {
        إذا (!sizep && !defaultp && !itemsp) أرجع;
        puts("\n");
        لكل (Ast_عقدة_تعبير* ثابت valuep : {sizep, defaultp}) {
            إذا (!valuep) استمر;
            emit_قيمة(valuep);
            puts(",\n");
        }
        puts("{");
        لكل (T_Item* itemp = itemsp; itemp; itemp = Ast_عقدة::as<T_Item>(itemp->nextp())) {
            إذا (itemp != itemsp) puts(",");
            puts("\n{");
            emit_قيمة(itemp->keyp());
            puts(", ");
            emit_قيمة(itemp->valuep());
            puts("}");
        }
        puts("\n}");
        puts("\n");
    }

    // The items, 'tabMod' per line if all are constants, otherwise one per line
    template <typename T_Item>
    فراغ emit_Unkeyed_Cons_عناصر(T_Item* itemsp, uint32_t tabMod) {
        إذا (!itemsp) أرجع;
        لكل (T_Item* itemp = itemsp; itemp; itemp = Ast_عقدة::as<T_Item>(itemp->nextp())) {
            إذا (!VN_IS(itemp->valuep(), Const)) tabMod = 1;
        }
        puts("\n");
        uint64_t n = 0;
        لكل (T_Item* itemp = itemsp; itemp; itemp = Ast_عقدة::as<T_Item>(itemp->nextp())) {
            إذا (n) puts((n % tabMod) ? ", " : ",\n");
            emit_قيمة(itemp->valuep());
            ++n;
        }
        puts("\n");
    }

    // Emit a value
    فراغ emit_قيمة(Ast_عقدة_تعبير* nodep) {
        // Either a value constructor
        إذا (Ast_عقدة_Cons* ثابت consp = VN_CAST(nodep, عقدة_Cons)) {
            emitValCons(consp);
            أرجع;
        }
        // Or a constant
        إذا (ثابت AstConst* ثابت constp = VN_CAST(nodep, Const)) {
            ثابت V3_رقم& رقم = constp->رقم();
            UASSERT_OBJ(!رقم.isFourState(), constp, "4-state value in emitted constant");
            // A narrow constant is a literal, which converts implicitly wherever a value is used
            // (This also covers string, double and null, which don't need type prefix)
            إذا (!constp->isWide()) {
                putns(constp, رقم.emitC());
                أرجع;
            }
            // Otherwise need the type prefix (naked braced lists cannot be deduced by templates)
            putnbs(constp, constp->dtypep()->cType("", false, false));
            emitInitializer(nodep);
            أرجع;
        }
        // Or an arbitrary expression, emitted via the derived visitor
        iterateConst(nodep);
    }

    فراغ emitInitializerBody(Ast_عقدة_تعبير* nodep) {
        إذا (AstConsArray* ثابت arrayp = VN_CAST(nodep, ConsArray)) {
            emitConsArray(arrayp);
            أرجع;
        }
        إذا (AstConsAssoc* ثابت assocp = VN_CAST(nodep, ConsAssoc)) {
            emit_Keyed_Cons_عناصر(nullptr, assocp->defaultp(), assocp->itemsp());
            أرجع;
        }
        إذا (AstConsWildcard* ثابت wildp = VN_CAST(nodep, ConsWildcard)) {
            emit_Keyed_Cons_عناصر(nullptr, wildp->defaultp(), wildp->itemsp());
            أرجع;
        }
        AstConsDynArray* ثابت dynp = VN_CAST(nodep, ConsDynArray);
        إذا (dynp && dynp->sizep()) {
            emit_Keyed_Cons_عناصر(dynp->sizep(), dynp->defaultp(), dynp->itemsp());
            أرجع;
        }
        إذا (dynp) {
            emit_Unkeyed_Cons_عناصر(dynp->itemsp(),
                                 tabModulus(dynp->dtypep()->skipRefp()->subDTypep()));
            أرجع;
        }
        إذا (AstConsQueue* ثابت queuep = VN_CAST(nodep, ConsQueue)) {
            emit_Unkeyed_Cons_عناصر(queuep->itemsp(),
                                 tabModulus(queuep->dtypep()->skipRefp()->subDTypep()));
            أرجع;
        }
        إذا (AstConsStruct* ثابت structp = VN_CAST(nodep, ConsStruct)) {
            emit_Unkeyed_Cons_عناصر(structp->itemsp(), 1);  // Members of mixed types
            أرجع;
        }
        إذا (VN_IS(nodep, Unbounded)) {
            // A public parameter's '$', which is only special during elaboration, so use zero
            puts("0");
            أرجع;
        }
        إذا (ثابت AstConst* ثابت constp = VN_CAST(nodep, Const)) {
            ثابت V3_رقم& رقم = constp->رقم();
            UASSERT_OBJ(!رقم.isFourState(), constp, "4-state value in emitted constant");
            إذا (!constp->isWide()) {
                puts(رقم.emitC());
                أرجع;
            }

            ofp()->putsNoTracking("{");  // The initializer of m_storage within the VlWide
            ثابت صحيح كلمات = constp->width_كلمات();
            ثابت صحيح tabMod = 4;
            إذا (كلمات > tabMod) puts("\n");
            لكل (صحيح n = 0; n < كلمات; ++n) {
                إذا (n) puts((n % tabMod) ? ", " : ",\n");
                ofp()->printf("0x%08" PRIx32, رقم.edataWord(n));
            }
            إذا (كلمات > tabMod) puts("\n");
            ofp()->putsNoTracking("}");
            أرجع;
        }
        nodep->v3fatalSrc("Unknown node type in emitInitializerBody: " << nodep->prettyTypeName());
    }

    // Emit the braced initializer for the given value '{...}'
    فراغ emitInitializer(Ast_عقدة_تعبير* nodep) {
        putns(nodep, "{");
        emitInitializerBody(nodep);
        puts("}");
    }

محمي:
    // Emit a C++ value constructor: 'Type{...}'
    فراغ emitValCons(Ast_عقدة_Cons* nodep) {
        putnbs(nodep, nodep->dtypep()->cType("", false, false));
        emitInitializer(nodep);
    }

    // Emit the C++ variable definition 'Type name{...}'
    فراغ emitVarDefn(ثابت AstVar* varp) {
        ثابت std::string name = varp->constPoolEntry()
                                     ? EmitCUtil::constPoolName(varp)
                                     : EmitCUtil::prefixNameProtect(EmitCParentModule::get(varp))
                                           + "::" + varp->nameProtect();
        putnbs(varp, varp->dtypep()->cType(name, false, false));
        emitInitializer(VN_AS(varp->valuep(), عقدة_تعبير));
    }
};
