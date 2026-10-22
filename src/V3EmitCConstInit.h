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

#ਅਨ੍ਤਰ੍ਭਾਵ "config_build.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "verilatedos.h"

#ਅਨ੍ਤਰ੍ਭਾਵ "V3Ast.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3EmitCBase.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3Error.h"

#ਅਨ੍ਤਰ੍ਭਾਵ <cinttypes>

//######################################################################
// Emitter that can emit NodeCons expressions

class EmitCConstInit VL_NOT_FINAL : ਜਨਤਕ EmitCBaseVisitorConst {
    // METHODS
    ਸਥਿਰ uint32_t tabModulus(ਅਚਲ Ast_ਗੰਢ_DType* dtypep) {
        ਅਚਲ uint32_t elem_ਬਾਈਟਾਂ = dtypep->ਚੌੜਾਈ_ਕੁੱਲ_ਬਾਈਟਾਂ();
        ਵਾਪਸ dtypep->isString() ? 1  // String
               : elem_ਬਾਈਟਾਂ <= 2   ? 8  // CData, SData
               : elem_ਬਾਈਟਾਂ <= 4   ? 4  // IData
               : elem_ਬਾਈਟਾਂ <= 8   ? 2  // QData
                                  : 1;
    }

    ਖਾਲੀ emitConsArray(AstConsArray* nodep) {
        ਅਚਲ AstUnpackArrayDType* ਅਚਲ dtypep
            = VN_AS(nodep->dtypep()->skipRefp(), UnpackArrayDType);
        ਅਚਲ uint64_t ਮਾਪ = dtypep->elementsConst();
        uint32_t tabMod = tabModulus(dtypep->subDTypep());
        ਜੇ (nodep->defaultp() && !VN_IS(nodep->defaultp(), Const)) tabMod = 1;
        ਲਈ (ਅਚਲ Ast_Cons_Array_ਵਸਤੂ* itemp = nodep->itemsp(); itemp;
             itemp = VN_AS(itemp->nextp(), Cons_Array_ਵਸਤੂ)) {
            ਜੇ (!VN_IS(itemp->valuep(), Const)) tabMod = 1;
        }
        ofp()->putsNoTracking("{");
        puts("\n");
        ਲਈ (uint64_t n = 0; n < ਮਾਪ; ++n) {
            ਜੇ (n) puts((n % tabMod) ? ", " : ",\n");
            emit_ਮੁੱਲ(nodep->valuep(n));
        }
        puts("\n");
        ofp()->putsNoTracking("}");
    }

    // 'size, default, {{key, value}, ...}', where the size and default are optional, or nothing
    // if empty
    ਸਾਂਚਾ <typename T_ਵਸਤੂ>
    ਖਾਲੀ emit_Keyed_Cons_ਚੀਜ਼ਾਂ(Ast_ਗੰਢ_ਵਿਅੰਜਕ* sizep, Ast_ਗੰਢ_ਵਿਅੰਜਕ* defaultp, T_ਵਸਤੂ* itemsp) {
        ਜੇ (!sizep && !defaultp && !itemsp) ਵਾਪਸ;
        puts("\n");
        ਲਈ (Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ valuep : {sizep, defaultp}) {
            ਜੇ (!valuep) ਜਾਰੀ;
            emit_ਮੁੱਲ(valuep);
            puts(",\n");
        }
        puts("{");
        ਲਈ (T_ਵਸਤੂ* itemp = itemsp; itemp; itemp = Ast_ਗੰਢ::as<T_ਵਸਤੂ>(itemp->nextp())) {
            ਜੇ (itemp != itemsp) puts(",");
            puts("\n{");
            emit_ਮੁੱਲ(itemp->keyp());
            puts(", ");
            emit_ਮੁੱਲ(itemp->valuep());
            puts("}");
        }
        puts("\n}");
        puts("\n");
    }

    // The items, 'tabMod' per line if all are constants, otherwise one per line
    ਸਾਂਚਾ <typename T_ਵਸਤੂ>
    ਖਾਲੀ emit_Unkeyed_Cons_ਚੀਜ਼ਾਂ(T_ਵਸਤੂ* itemsp, uint32_t tabMod) {
        ਜੇ (!itemsp) ਵਾਪਸ;
        ਲਈ (T_ਵਸਤੂ* itemp = itemsp; itemp; itemp = Ast_ਗੰਢ::as<T_ਵਸਤੂ>(itemp->nextp())) {
            ਜੇ (!VN_IS(itemp->valuep(), Const)) tabMod = 1;
        }
        puts("\n");
        uint64_t n = 0;
        ਲਈ (T_ਵਸਤੂ* itemp = itemsp; itemp; itemp = Ast_ਗੰਢ::as<T_ਵਸਤੂ>(itemp->nextp())) {
            ਜੇ (n) puts((n % tabMod) ? ", " : ",\n");
            emit_ਮੁੱਲ(itemp->valuep());
            ++n;
        }
        puts("\n");
    }

    // Emit a value
    ਖਾਲੀ emit_ਮੁੱਲ(Ast_ਗੰਢ_ਵਿਅੰਜਕ* nodep) {
        // Either a value constructor
        ਜੇ (Ast_ਗੰਢ_Cons* ਅਚਲ consp = VN_CAST(nodep, ਗੰਢ_Cons)) {
            emitValCons(consp);
            ਵਾਪਸ;
        }
        // Or a constant
        ਜੇ (ਅਚਲ AstConst* ਅਚਲ constp = VN_CAST(nodep, Const)) {
            ਅਚਲ V3_ਸੰਖਿਆ& ਅੰਕ = constp->ਅੰਕ();
            UASSERT_OBJ(!ਅੰਕ.isFourState(), constp, "4-state value in emitted constant");
            // A narrow constant is a literal, which converts implicitly wherever a value is used
            // (This also covers string, double and null, which don't need type prefix)
            ਜੇ (!constp->isWide()) {
                putns(constp, ਅੰਕ.emitC());
                ਵਾਪਸ;
            }
            // Otherwise need the type prefix (naked braced lists cannot be deduced by templates)
            putnbs(constp, constp->dtypep()->cType("", false, false));
            emitInitializer(nodep);
            ਵਾਪਸ;
        }
        // Or an arbitrary expression, emitted via the derived visitor
        iterateConst(nodep);
    }

    ਖਾਲੀ emitInitializerBody(Ast_ਗੰਢ_ਵਿਅੰਜਕ* nodep) {
        ਜੇ (AstConsArray* ਅਚਲ arrayp = VN_CAST(nodep, ConsArray)) {
            emitConsArray(arrayp);
            ਵਾਪਸ;
        }
        ਜੇ (AstConsAssoc* ਅਚਲ assocp = VN_CAST(nodep, ConsAssoc)) {
            emit_Keyed_Cons_ਚੀਜ਼ਾਂ(nullptr, assocp->defaultp(), assocp->itemsp());
            ਵਾਪਸ;
        }
        ਜੇ (AstConsWildcard* ਅਚਲ wildp = VN_CAST(nodep, ConsWildcard)) {
            emit_Keyed_Cons_ਚੀਜ਼ਾਂ(nullptr, wildp->defaultp(), wildp->itemsp());
            ਵਾਪਸ;
        }
        AstConsDynArray* ਅਚਲ dynp = VN_CAST(nodep, ConsDynArray);
        ਜੇ (dynp && dynp->sizep()) {
            emit_Keyed_Cons_ਚੀਜ਼ਾਂ(dynp->sizep(), dynp->defaultp(), dynp->itemsp());
            ਵਾਪਸ;
        }
        ਜੇ (dynp) {
            emit_Unkeyed_Cons_ਚੀਜ਼ਾਂ(dynp->itemsp(),
                                 tabModulus(dynp->dtypep()->skipRefp()->subDTypep()));
            ਵਾਪਸ;
        }
        ਜੇ (Ast_Cons_ਕਤਾਰ* ਅਚਲ queuep = VN_CAST(nodep, Cons_ਕਤਾਰ)) {
            emit_Unkeyed_Cons_ਚੀਜ਼ਾਂ(queuep->itemsp(),
                                 tabModulus(queuep->dtypep()->skipRefp()->subDTypep()));
            ਵਾਪਸ;
        }
        ਜੇ (AstConsStruct* ਅਚਲ structp = VN_CAST(nodep, ConsStruct)) {
            emit_Unkeyed_Cons_ਚੀਜ਼ਾਂ(structp->itemsp(), 1);  // Members of mixed types
            ਵਾਪਸ;
        }
        ਜੇ (VN_IS(nodep, Unbounded)) {
            // A public parameter's '$', which is only special during elaboration, so use zero
            puts("0");
            ਵਾਪਸ;
        }
        ਜੇ (ਅਚਲ AstConst* ਅਚਲ constp = VN_CAST(nodep, Const)) {
            ਅਚਲ V3_ਸੰਖਿਆ& ਅੰਕ = constp->ਅੰਕ();
            UASSERT_OBJ(!ਅੰਕ.isFourState(), constp, "4-state value in emitted constant");
            ਜੇ (!constp->isWide()) {
                puts(ਅੰਕ.emitC());
                ਵਾਪਸ;
            }

            ofp()->putsNoTracking("{");  // The initializer of m_storage within the VlWide
            ਅਚਲ ਪੂਰਨ ਸ਼ਬਦ_2 = constp->ਚੌੜਾਈ_ਸ਼ਬਦ();
            ਅਚਲ ਪੂਰਨ tabMod = 4;
            ਜੇ (ਸ਼ਬਦ_2 > tabMod) puts("\n");
            ਲਈ (ਪੂਰਨ n = 0; n < ਸ਼ਬਦ_2; ++n) {
                ਜੇ (n) puts((n % tabMod) ? ", " : ",\n");
                ofp()->ਲਿਖੋ("0x%08" PRIx32, ਅੰਕ.edata_ਸ਼ਬਦ(n));
            }
            ਜੇ (ਸ਼ਬਦ_2 > tabMod) puts("\n");
            ofp()->putsNoTracking("}");
            ਵਾਪਸ;
        }
        nodep->v3fatalSrc("Unknown node type in emitInitializerBody: " << nodep->prettyTypeName());
    }

    // Emit the braced initializer for the given value '{...}'
    ਖਾਲੀ emitInitializer(Ast_ਗੰਢ_ਵਿਅੰਜਕ* nodep) {
        putns(nodep, "{");
        emitInitializerBody(nodep);
        puts("}");
    }

ਸੁਰੱਖਿਅਤ:
    // Emit a C++ value constructor: 'Type{...}'
    ਖਾਲੀ emitValCons(Ast_ਗੰਢ_Cons* nodep) {
        putnbs(nodep, nodep->dtypep()->cType("", false, false));
        emitInitializer(nodep);
    }

    // Emit the C++ variable definition 'Type name{...}'
    ਖਾਲੀ emitVarDefn(ਅਚਲ AstVar* varp) {
        ਅਚਲ std::string name = varp->const_Pool_ਇੰਦਰਾਜ()
                                     ? EmitCUtil::constPoolName(varp)
                                     : EmitCUtil::ਅਗੇਤਰ_Name_Protect(EmitCParentModule::get(varp))
                                           + "::" + varp->nameProtect();
        putnbs(varp, varp->dtypep()->cType(name, false, false));
        emitInitializer(VN_AS(varp->valuep(), ਗੰਢ_ਵਿਅੰਜਕ));
    }
};
