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

#అన్తర్భావ "config_build.h"
#అన్తర్భావ "verilatedos.h"

#అన్తర్భావ "V3Ast.h"
#అన్తర్భావ "V3EmitCBase.h"
#అన్తర్భావ "V3Error.h"

#అన్తర్భావ <cinttypes>

//######################################################################
// Emitter that can emit NodeCons expressions

class EmitCConstInit VL_NOT_FINAL : బహిరంగ EmitCBaseVisitorConst {
    // METHODS
    స్థిర uint32_t tabModulus(స్థిరరాశి Ast_కణుపు_DType* dtypep) {
        స్థిరరాశి uint32_t elem_బైట్లు = dtypep->వెడల్పు_మొత్తం_బైట్లు();
        తిరిగి dtypep->isString() ? 1  // String
               : elem_బైట్లు <= 2   ? 8  // CData, SData
               : elem_బైట్లు <= 4   ? 4  // IData
               : elem_బైట్లు <= 8   ? 2  // QData
                                  : 1;
    }

    శూన్య emitConsArray(AstConsArray* nodep) {
        స్థిరరాశి AstUnpackArrayDType* స్థిరరాశి dtypep
            = VN_AS(nodep->dtypep()->skipRefp(), UnpackArrayDType);
        స్థిరరాశి uint64_t కొలత = dtypep->elementsConst();
        uint32_t tabMod = tabModulus(dtypep->subDTypep());
        ఒకవేళ (nodep->defaultp() && !VN_IS(nodep->defaultp(), Const)) tabMod = 1;
        కొరకు (స్థిరరాశి Ast_Cons_Array_అంశం* itemp = nodep->itemsp(); itemp;
             itemp = VN_AS(itemp->nextp(), Cons_Array_అంశం)) {
            ఒకవేళ (!VN_IS(itemp->valuep(), Const)) tabMod = 1;
        }
        ofp()->putsNoTracking("{");
        puts("\n");
        కొరకు (uint64_t n = 0; n < కొలత; ++n) {
            ఒకవేళ (n) puts((n % tabMod) ? ", " : ",\n");
            emit_విలువ(nodep->valuep(n));
        }
        puts("\n");
        ofp()->putsNoTracking("}");
    }

    // 'size, default, {{key, value}, ...}', where the size and default are optional, or nothing
    // if empty
    మూస <typename T_అంశం>
    శూన్య emit_Keyed_Cons_వస్తువులు(Ast_కణుపు_సమాసం* sizep, Ast_కణుపు_సమాసం* defaultp, T_అంశం* itemsp) {
        ఒకవేళ (!sizep && !defaultp && !itemsp) తిరిగి;
        puts("\n");
        కొరకు (Ast_కణుపు_సమాసం* స్థిరరాశి valuep : {sizep, defaultp}) {
            ఒకవేళ (!valuep) కొనసాగు;
            emit_విలువ(valuep);
            puts(",\n");
        }
        puts("{");
        కొరకు (T_అంశం* itemp = itemsp; itemp; itemp = Ast_కణుపు::as<T_అంశం>(itemp->nextp())) {
            ఒకవేళ (itemp != itemsp) puts(",");
            puts("\n{");
            emit_విలువ(itemp->keyp());
            puts(", ");
            emit_విలువ(itemp->valuep());
            puts("}");
        }
        puts("\n}");
        puts("\n");
    }

    // The items, 'tabMod' per line if all are constants, otherwise one per line
    మూస <typename T_అంశం>
    శూన్య emit_Unkeyed_Cons_వస్తువులు(T_అంశం* itemsp, uint32_t tabMod) {
        ఒకవేళ (!itemsp) తిరిగి;
        కొరకు (T_అంశం* itemp = itemsp; itemp; itemp = Ast_కణుపు::as<T_అంశం>(itemp->nextp())) {
            ఒకవేళ (!VN_IS(itemp->valuep(), Const)) tabMod = 1;
        }
        puts("\n");
        uint64_t n = 0;
        కొరకు (T_అంశం* itemp = itemsp; itemp; itemp = Ast_కణుపు::as<T_అంశం>(itemp->nextp())) {
            ఒకవేళ (n) puts((n % tabMod) ? ", " : ",\n");
            emit_విలువ(itemp->valuep());
            ++n;
        }
        puts("\n");
    }

    // Emit a value
    శూన్య emit_విలువ(Ast_కణుపు_సమాసం* nodep) {
        // Either a value constructor
        ఒకవేళ (Ast_కణుపు_Cons* స్థిరరాశి consp = VN_CAST(nodep, కణుపు_Cons)) {
            emitValCons(consp);
            తిరిగి;
        }
        // Or a constant
        ఒకవేళ (స్థిరరాశి AstConst* స్థిరరాశి constp = VN_CAST(nodep, Const)) {
            స్థిరరాశి V3_సంఖ్య& అంకె = constp->అంకె();
            UASSERT_OBJ(!అంకె.isFourState(), constp, "4-state value in emitted constant");
            // A narrow constant is a literal, which converts implicitly wherever a value is used
            // (This also covers string, double and null, which don't need type prefix)
            ఒకవేళ (!constp->isWide()) {
                putns(constp, అంకె.emitC());
                తిరిగి;
            }
            // Otherwise need the type prefix (naked braced lists cannot be deduced by templates)
            putnbs(constp, constp->dtypep()->cType("", false, false));
            emitInitializer(nodep);
            తిరిగి;
        }
        // Or an arbitrary expression, emitted via the derived visitor
        iterateConst(nodep);
    }

    శూన్య emitInitializerBody(Ast_కణుపు_సమాసం* nodep) {
        ఒకవేళ (AstConsArray* స్థిరరాశి arrayp = VN_CAST(nodep, ConsArray)) {
            emitConsArray(arrayp);
            తిరిగి;
        }
        ఒకవేళ (AstConsAssoc* స్థిరరాశి assocp = VN_CAST(nodep, ConsAssoc)) {
            emit_Keyed_Cons_వస్తువులు(nullptr, assocp->defaultp(), assocp->itemsp());
            తిరిగి;
        }
        ఒకవేళ (AstConsWildcard* స్థిరరాశి wildp = VN_CAST(nodep, ConsWildcard)) {
            emit_Keyed_Cons_వస్తువులు(nullptr, wildp->defaultp(), wildp->itemsp());
            తిరిగి;
        }
        AstConsDynArray* స్థిరరాశి dynp = VN_CAST(nodep, ConsDynArray);
        ఒకవేళ (dynp && dynp->sizep()) {
            emit_Keyed_Cons_వస్తువులు(dynp->sizep(), dynp->defaultp(), dynp->itemsp());
            తిరిగి;
        }
        ఒకవేళ (dynp) {
            emit_Unkeyed_Cons_వస్తువులు(dynp->itemsp(),
                                 tabModulus(dynp->dtypep()->skipRefp()->subDTypep()));
            తిరిగి;
        }
        ఒకవేళ (Ast_Cons_వరుస* స్థిరరాశి queuep = VN_CAST(nodep, Cons_వరుస)) {
            emit_Unkeyed_Cons_వస్తువులు(queuep->itemsp(),
                                 tabModulus(queuep->dtypep()->skipRefp()->subDTypep()));
            తిరిగి;
        }
        ఒకవేళ (AstConsStruct* స్థిరరాశి structp = VN_CAST(nodep, ConsStruct)) {
            emit_Unkeyed_Cons_వస్తువులు(structp->itemsp(), 1);  // Members of mixed types
            తిరిగి;
        }
        ఒకవేళ (VN_IS(nodep, Unbounded)) {
            // A public parameter's '$', which is only special during elaboration, so use zero
            puts("0");
            తిరిగి;
        }
        ఒకవేళ (స్థిరరాశి AstConst* స్థిరరాశి constp = VN_CAST(nodep, Const)) {
            స్థిరరాశి V3_సంఖ్య& అంకె = constp->అంకె();
            UASSERT_OBJ(!అంకె.isFourState(), constp, "4-state value in emitted constant");
            ఒకవేళ (!constp->isWide()) {
                puts(అంకె.emitC());
                తిరిగి;
            }

            ofp()->putsNoTracking("{");  // The initializer of m_storage within the VlWide
            స్థిరరాశి పూర్ణాంకం పదాలు = constp->వెడల్పు_పదాలు();
            స్థిరరాశి పూర్ణాంకం tabMod = 4;
            ఒకవేళ (పదాలు > tabMod) puts("\n");
            కొరకు (పూర్ణాంకం n = 0; n < పదాలు; ++n) {
                ఒకవేళ (n) puts((n % tabMod) ? ", " : ",\n");
                ofp()->రాయి("0x%08" PRIx32, అంకె.edata_మాట(n));
            }
            ఒకవేళ (పదాలు > tabMod) puts("\n");
            ofp()->putsNoTracking("}");
            తిరిగి;
        }
        nodep->v3fatalSrc("Unknown node type in emitInitializerBody: " << nodep->prettyTypeName());
    }

    // Emit the braced initializer for the given value '{...}'
    శూన్య emitInitializer(Ast_కణుపు_సమాసం* nodep) {
        putns(nodep, "{");
        emitInitializerBody(nodep);
        puts("}");
    }

రక్షిత:
    // Emit a C++ value constructor: 'Type{...}'
    శూన్య emitValCons(Ast_కణుపు_Cons* nodep) {
        putnbs(nodep, nodep->dtypep()->cType("", false, false));
        emitInitializer(nodep);
    }

    // Emit the C++ variable definition 'Type name{...}'
    శూన్య emitVarDefn(స్థిరరాశి AstVar* varp) {
        స్థిరరాశి std::string name = varp->const_Pool_నమోదు()
                                     ? EmitCUtil::constPoolName(varp)
                                     : EmitCUtil::ఉపసర్గ_Name_Protect(EmitCParentModule::get(varp))
                                           + "::" + varp->nameProtect();
        putnbs(varp, varp->dtypep()->cType(name, false, false));
        emitInitializer(VN_AS(varp->valuep(), కణుపు_సమాసం));
    }
};
