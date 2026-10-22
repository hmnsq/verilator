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

#ଅନ୍ତର୍ଭାଵ "config_build.h"
#ଅନ୍ତର୍ଭାଵ "verilatedos.h"

#ଅନ୍ତର୍ଭାଵ "V3Ast.h"
#ଅନ୍ତର୍ଭାଵ "V3EmitCBase.h"
#ଅନ୍ତର୍ଭାଵ "V3Error.h"

#ଅନ୍ତର୍ଭାଵ <cinttypes>

//######################################################################
// Emitter that can emit NodeCons expressions

class EmitCConstInit VL_NOT_FINAL : ସର୍ବସାଧାରଣ EmitCBaseVisitorConst {
    // METHODS
    ସ୍ଥିର uint32_t tabModulus(ଅଚଳ Ast_ଗଣ୍ଠି_DType* dtypep) {
        ଅଚଳ uint32_t elemBytes = dtypep->widthTotalBytes();
        ଫେରନ୍ତୁ dtypep->isString() ? 1  // String
               : elemBytes <= 2   ? 8  // CData, SData
               : elemBytes <= 4   ? 4  // IData
               : elemBytes <= 8   ? 2  // QData
                                  : 1;
    }

    ଶୂନ୍ୟ emitConsArray(AstConsArray* nodep) {
        ଅଚଳ AstUnpackArrayDType* ଅଚଳ dtypep
            = VN_AS(nodep->dtypep()->skipRefp(), UnpackArrayDType);
        ଅଚଳ uint64_t size = dtypep->elementsConst();
        uint32_t tabMod = tabModulus(dtypep->subDTypep());
        ଯଦି (nodep->defaultp() && !VN_IS(nodep->defaultp(), Const)) tabMod = 1;
        ପାଇଁ (ଅଚଳ AstConsArrayItem* itemp = nodep->itemsp(); itemp;
             itemp = VN_AS(itemp->nextp(), ConsArrayItem)) {
            ଯଦି (!VN_IS(itemp->valuep(), Const)) tabMod = 1;
        }
        ofp()->putsNoTracking("{");
        puts("\n");
        ପାଇଁ (uint64_t n = 0; n < size; ++n) {
            ଯଦି (n) puts((n % tabMod) ? ", " : ",\n");
            emit_ମୂଲ୍ୟ(nodep->valuep(n));
        }
        puts("\n");
        ofp()->putsNoTracking("}");
    }

    // 'size, default, {{key, value}, ...}', where the size and default are optional, or nothing
    // if empty
    template <typename T_Item>
    ଶୂନ୍ୟ emit_Keyed_Cons_ବସ୍ତୁଗୁଡ଼ିକ(Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* sizep, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* defaultp, T_Item* itemsp) {
        ଯଦି (!sizep && !defaultp && !itemsp) ଫେରନ୍ତୁ;
        puts("\n");
        ପାଇଁ (Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ valuep : {sizep, defaultp}) {
            ଯଦି (!valuep) ଜାରି;
            emit_ମୂଲ୍ୟ(valuep);
            puts(",\n");
        }
        puts("{");
        ପାଇଁ (T_Item* itemp = itemsp; itemp; itemp = Ast_ଗଣ୍ଠି::as<T_Item>(itemp->nextp())) {
            ଯଦି (itemp != itemsp) puts(",");
            puts("\n{");
            emit_ମୂଲ୍ୟ(itemp->keyp());
            puts(", ");
            emit_ମୂଲ୍ୟ(itemp->valuep());
            puts("}");
        }
        puts("\n}");
        puts("\n");
    }

    // The items, 'tabMod' per line if all are constants, otherwise one per line
    template <typename T_Item>
    ଶୂନ୍ୟ emit_Unkeyed_Cons_ବସ୍ତୁଗୁଡ଼ିକ(T_Item* itemsp, uint32_t tabMod) {
        ଯଦି (!itemsp) ଫେରନ୍ତୁ;
        ପାଇଁ (T_Item* itemp = itemsp; itemp; itemp = Ast_ଗଣ୍ଠି::as<T_Item>(itemp->nextp())) {
            ଯଦି (!VN_IS(itemp->valuep(), Const)) tabMod = 1;
        }
        puts("\n");
        uint64_t n = 0;
        ପାଇଁ (T_Item* itemp = itemsp; itemp; itemp = Ast_ଗଣ୍ଠି::as<T_Item>(itemp->nextp())) {
            ଯଦି (n) puts((n % tabMod) ? ", " : ",\n");
            emit_ମୂଲ୍ୟ(itemp->valuep());
            ++n;
        }
        puts("\n");
    }

    // Emit a value
    ଶୂନ୍ୟ emit_ମୂଲ୍ୟ(Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* nodep) {
        // Either a value constructor
        ଯଦି (Ast_ଗଣ୍ଠି_Cons* ଅଚଳ consp = VN_CAST(nodep, ଗଣ୍ଠି_Cons)) {
            emitValCons(consp);
            ଫେରନ୍ତୁ;
        }
        // Or a constant
        ଯଦି (ଅଚଳ AstConst* ଅଚଳ constp = VN_CAST(nodep, Const)) {
            ଅଚଳ V3_ସଂଖ୍ୟା& ଅଙ୍କ = constp->ଅଙ୍କ();
            UASSERT_OBJ(!ଅଙ୍କ.isFourState(), constp, "4-state value in emitted constant");
            // A narrow constant is a literal, which converts implicitly wherever a value is used
            // (This also covers string, double and null, which don't need type prefix)
            ଯଦି (!constp->isWide()) {
                putns(constp, ଅଙ୍କ.emitC());
                ଫେରନ୍ତୁ;
            }
            // Otherwise need the type prefix (naked braced lists cannot be deduced by templates)
            putnbs(constp, constp->dtypep()->cType("", false, false));
            emitInitializer(nodep);
            ଫେରନ୍ତୁ;
        }
        // Or an arbitrary expression, emitted via the derived visitor
        iterateConst(nodep);
    }

    ଶୂନ୍ୟ emitInitializerBody(Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* nodep) {
        ଯଦି (AstConsArray* ଅଚଳ arrayp = VN_CAST(nodep, ConsArray)) {
            emitConsArray(arrayp);
            ଫେରନ୍ତୁ;
        }
        ଯଦି (AstConsAssoc* ଅଚଳ assocp = VN_CAST(nodep, ConsAssoc)) {
            emit_Keyed_Cons_ବସ୍ତୁଗୁଡ଼ିକ(nullptr, assocp->defaultp(), assocp->itemsp());
            ଫେରନ୍ତୁ;
        }
        ଯଦି (AstConsWildcard* ଅଚଳ wildp = VN_CAST(nodep, ConsWildcard)) {
            emit_Keyed_Cons_ବସ୍ତୁଗୁଡ଼ିକ(nullptr, wildp->defaultp(), wildp->itemsp());
            ଫେରନ୍ତୁ;
        }
        AstConsDynArray* ଅଚଳ dynp = VN_CAST(nodep, ConsDynArray);
        ଯଦି (dynp && dynp->sizep()) {
            emit_Keyed_Cons_ବସ୍ତୁଗୁଡ଼ିକ(dynp->sizep(), dynp->defaultp(), dynp->itemsp());
            ଫେରନ୍ତୁ;
        }
        ଯଦି (dynp) {
            emit_Unkeyed_Cons_ବସ୍ତୁଗୁଡ଼ିକ(dynp->itemsp(),
                                 tabModulus(dynp->dtypep()->skipRefp()->subDTypep()));
            ଫେରନ୍ତୁ;
        }
        ଯଦି (AstConsQueue* ଅଚଳ queuep = VN_CAST(nodep, ConsQueue)) {
            emit_Unkeyed_Cons_ବସ୍ତୁଗୁଡ଼ିକ(queuep->itemsp(),
                                 tabModulus(queuep->dtypep()->skipRefp()->subDTypep()));
            ଫେରନ୍ତୁ;
        }
        ଯଦି (AstConsStruct* ଅଚଳ structp = VN_CAST(nodep, ConsStruct)) {
            emit_Unkeyed_Cons_ବସ୍ତୁଗୁଡ଼ିକ(structp->itemsp(), 1);  // Members of mixed types
            ଫେରନ୍ତୁ;
        }
        ଯଦି (VN_IS(nodep, Unbounded)) {
            // A public parameter's '$', which is only special during elaboration, so use zero
            puts("0");
            ଫେରନ୍ତୁ;
        }
        ଯଦି (ଅଚଳ AstConst* ଅଚଳ constp = VN_CAST(nodep, Const)) {
            ଅଚଳ V3_ସଂଖ୍ୟା& ଅଙ୍କ = constp->ଅଙ୍କ();
            UASSERT_OBJ(!ଅଙ୍କ.isFourState(), constp, "4-state value in emitted constant");
            ଯଦି (!constp->isWide()) {
                puts(ଅଙ୍କ.emitC());
                ଫେରନ୍ତୁ;
            }

            ofp()->putsNoTracking("{");  // The initializer of m_storage within the VlWide
            ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା ଶବ୍ଦଗୁଡ଼ିକ = constp->width_ଶବ୍ଦଗୁଡ଼ିକ();
            ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା tabMod = 4;
            ଯଦି (ଶବ୍ଦଗୁଡ଼ିକ > tabMod) puts("\n");
            ପାଇଁ (ପୂର୍ଣ୍ଣସଂଖ୍ୟା n = 0; n < ଶବ୍ଦଗୁଡ଼ିକ; ++n) {
                ଯଦି (n) puts((n % tabMod) ? ", " : ",\n");
                ofp()->printf("0x%08" PRIx32, ଅଙ୍କ.edataWord(n));
            }
            ଯଦି (ଶବ୍ଦଗୁଡ଼ିକ > tabMod) puts("\n");
            ofp()->putsNoTracking("}");
            ଫେରନ୍ତୁ;
        }
        nodep->v3fatalSrc("Unknown node type in emitInitializerBody: " << nodep->prettyTypeName());
    }

    // Emit the braced initializer for the given value '{...}'
    ଶୂନ୍ୟ emitInitializer(Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* nodep) {
        putns(nodep, "{");
        emitInitializerBody(nodep);
        puts("}");
    }

ସୁରକ୍ଷିତ:
    // Emit a C++ value constructor: 'Type{...}'
    ଶୂନ୍ୟ emitValCons(Ast_ଗଣ୍ଠି_Cons* nodep) {
        putnbs(nodep, nodep->dtypep()->cType("", false, false));
        emitInitializer(nodep);
    }

    // Emit the C++ variable definition 'Type name{...}'
    ଶୂନ୍ୟ emitVarDefn(ଅଚଳ AstVar* varp) {
        ଅଚଳ std::string name = varp->constPoolEntry()
                                     ? EmitCUtil::constPoolName(varp)
                                     : EmitCUtil::prefixNameProtect(EmitCParentModule::get(varp))
                                           + "::" + varp->nameProtect();
        putnbs(varp, varp->dtypep()->cType(name, false, false));
        emitInitializer(VN_AS(varp->valuep(), ଗଣ୍ଠି_ପଦସମଷ୍ଟି));
    }
};
