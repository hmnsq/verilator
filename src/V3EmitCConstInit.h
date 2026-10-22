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

#അന്തര്ഭാവ "config_build.h"
#അന്തര്ഭാവ "verilatedos.h"

#അന്തര്ഭാവ "V3Ast.h"
#അന്തര്ഭാവ "V3EmitCBase.h"
#അന്തര്ഭാവ "V3Error.h"

#അന്തര്ഭാവ <cinttypes>

//######################################################################
// Emitter that can emit NodeCons expressions

class EmitCConstInit VL_NOT_FINAL : പൊതു EmitCBaseVisitorConst {
    // METHODS
    സ്ഥിതം uint32_t tabModulus(സ്ഥിരം Ast_കണ്ണി_DType* dtypep) {
        സ്ഥിരം uint32_t elemBytes = dtypep->widthTotalBytes();
        തിരികെ dtypep->isString() ? 1  // String
               : elemBytes <= 2   ? 8  // CData, SData
               : elemBytes <= 4   ? 4  // IData
               : elemBytes <= 8   ? 2  // QData
                                  : 1;
    }

    ശൂന്യം emitConsArray(AstConsArray* nodep) {
        സ്ഥിരം AstUnpackArrayDType* സ്ഥിരം dtypep
            = VN_AS(nodep->dtypep()->skipRefp(), UnpackArrayDType);
        സ്ഥിരം uint64_t size = dtypep->elementsConst();
        uint32_t tabMod = tabModulus(dtypep->subDTypep());
        എങ്കിൽ (nodep->defaultp() && !VN_IS(nodep->defaultp(), Const)) tabMod = 1;
        വേണ്ടി (സ്ഥിരം AstConsArrayItem* itemp = nodep->itemsp(); itemp;
             itemp = VN_AS(itemp->nextp(), ConsArrayItem)) {
            എങ്കിൽ (!VN_IS(itemp->valuep(), Const)) tabMod = 1;
        }
        ofp()->putsNoTracking("{");
        puts("\n");
        വേണ്ടി (uint64_t n = 0; n < size; ++n) {
            എങ്കിൽ (n) puts((n % tabMod) ? ", " : ",\n");
            emit_മൂല്യം(nodep->valuep(n));
        }
        puts("\n");
        ofp()->putsNoTracking("}");
    }

    // 'size, default, {{key, value}, ...}', where the size and default are optional, or nothing
    // if empty
    template <typename T_Item>
    ശൂന്യം emit_Keyed_Cons_ഇനങ്ങൾ(Ast_കണ്ണി_വ്യഞ്ജകം* sizep, Ast_കണ്ണി_വ്യഞ്ജകം* defaultp, T_Item* itemsp) {
        എങ്കിൽ (!sizep && !defaultp && !itemsp) തിരികെ;
        puts("\n");
        വേണ്ടി (Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം valuep : {sizep, defaultp}) {
            എങ്കിൽ (!valuep) തുടരുക;
            emit_മൂല്യം(valuep);
            puts(",\n");
        }
        puts("{");
        വേണ്ടി (T_Item* itemp = itemsp; itemp; itemp = Ast_കണ്ണി::as<T_Item>(itemp->nextp())) {
            എങ്കിൽ (itemp != itemsp) puts(",");
            puts("\n{");
            emit_മൂല്യം(itemp->keyp());
            puts(", ");
            emit_മൂല്യം(itemp->valuep());
            puts("}");
        }
        puts("\n}");
        puts("\n");
    }

    // The items, 'tabMod' per line if all are constants, otherwise one per line
    template <typename T_Item>
    ശൂന്യം emit_Unkeyed_Cons_ഇനങ്ങൾ(T_Item* itemsp, uint32_t tabMod) {
        എങ്കിൽ (!itemsp) തിരികെ;
        വേണ്ടി (T_Item* itemp = itemsp; itemp; itemp = Ast_കണ്ണി::as<T_Item>(itemp->nextp())) {
            എങ്കിൽ (!VN_IS(itemp->valuep(), Const)) tabMod = 1;
        }
        puts("\n");
        uint64_t n = 0;
        വേണ്ടി (T_Item* itemp = itemsp; itemp; itemp = Ast_കണ്ണി::as<T_Item>(itemp->nextp())) {
            എങ്കിൽ (n) puts((n % tabMod) ? ", " : ",\n");
            emit_മൂല്യം(itemp->valuep());
            ++n;
        }
        puts("\n");
    }

    // Emit a value
    ശൂന്യം emit_മൂല്യം(Ast_കണ്ണി_വ്യഞ്ജകം* nodep) {
        // Either a value constructor
        എങ്കിൽ (Ast_കണ്ണി_Cons* സ്ഥിരം consp = VN_CAST(nodep, കണ്ണി_Cons)) {
            emitValCons(consp);
            തിരികെ;
        }
        // Or a constant
        എങ്കിൽ (സ്ഥിരം AstConst* സ്ഥിരം constp = VN_CAST(nodep, Const)) {
            സ്ഥിരം V3_സംഖ്യ& അക്കം = constp->അക്കം();
            UASSERT_OBJ(!അക്കം.isFourState(), constp, "4-state value in emitted constant");
            // A narrow constant is a literal, which converts implicitly wherever a value is used
            // (This also covers string, double and null, which don't need type prefix)
            എങ്കിൽ (!constp->isWide()) {
                putns(constp, അക്കം.emitC());
                തിരികെ;
            }
            // Otherwise need the type prefix (naked braced lists cannot be deduced by templates)
            putnbs(constp, constp->dtypep()->cType("", false, false));
            emitInitializer(nodep);
            തിരികെ;
        }
        // Or an arbitrary expression, emitted via the derived visitor
        iterateConst(nodep);
    }

    ശൂന്യം emitInitializerBody(Ast_കണ്ണി_വ്യഞ്ജകം* nodep) {
        എങ്കിൽ (AstConsArray* സ്ഥിരം arrayp = VN_CAST(nodep, ConsArray)) {
            emitConsArray(arrayp);
            തിരികെ;
        }
        എങ്കിൽ (AstConsAssoc* സ്ഥിരം assocp = VN_CAST(nodep, ConsAssoc)) {
            emit_Keyed_Cons_ഇനങ്ങൾ(nullptr, assocp->defaultp(), assocp->itemsp());
            തിരികെ;
        }
        എങ്കിൽ (AstConsWildcard* സ്ഥിരം wildp = VN_CAST(nodep, ConsWildcard)) {
            emit_Keyed_Cons_ഇനങ്ങൾ(nullptr, wildp->defaultp(), wildp->itemsp());
            തിരികെ;
        }
        AstConsDynArray* സ്ഥിരം dynp = VN_CAST(nodep, ConsDynArray);
        എങ്കിൽ (dynp && dynp->sizep()) {
            emit_Keyed_Cons_ഇനങ്ങൾ(dynp->sizep(), dynp->defaultp(), dynp->itemsp());
            തിരികെ;
        }
        എങ്കിൽ (dynp) {
            emit_Unkeyed_Cons_ഇനങ്ങൾ(dynp->itemsp(),
                                 tabModulus(dynp->dtypep()->skipRefp()->subDTypep()));
            തിരികെ;
        }
        എങ്കിൽ (AstConsQueue* സ്ഥിരം queuep = VN_CAST(nodep, ConsQueue)) {
            emit_Unkeyed_Cons_ഇനങ്ങൾ(queuep->itemsp(),
                                 tabModulus(queuep->dtypep()->skipRefp()->subDTypep()));
            തിരികെ;
        }
        എങ്കിൽ (AstConsStruct* സ്ഥിരം structp = VN_CAST(nodep, ConsStruct)) {
            emit_Unkeyed_Cons_ഇനങ്ങൾ(structp->itemsp(), 1);  // Members of mixed types
            തിരികെ;
        }
        എങ്കിൽ (VN_IS(nodep, Unbounded)) {
            // A public parameter's '$', which is only special during elaboration, so use zero
            puts("0");
            തിരികെ;
        }
        എങ്കിൽ (സ്ഥിരം AstConst* സ്ഥിരം constp = VN_CAST(nodep, Const)) {
            സ്ഥിരം V3_സംഖ്യ& അക്കം = constp->അക്കം();
            UASSERT_OBJ(!അക്കം.isFourState(), constp, "4-state value in emitted constant");
            എങ്കിൽ (!constp->isWide()) {
                puts(അക്കം.emitC());
                തിരികെ;
            }

            ofp()->putsNoTracking("{");  // The initializer of m_storage within the VlWide
            സ്ഥിരം പൂർണ്ണസംഖ്യ വാക്കുകൾ = constp->width_വാക്കുകൾ();
            സ്ഥിരം പൂർണ്ണസംഖ്യ tabMod = 4;
            എങ്കിൽ (വാക്കുകൾ > tabMod) puts("\n");
            വേണ്ടി (പൂർണ്ണസംഖ്യ n = 0; n < വാക്കുകൾ; ++n) {
                എങ്കിൽ (n) puts((n % tabMod) ? ", " : ",\n");
                ofp()->printf("0x%08" PRIx32, അക്കം.edataWord(n));
            }
            എങ്കിൽ (വാക്കുകൾ > tabMod) puts("\n");
            ofp()->putsNoTracking("}");
            തിരികെ;
        }
        nodep->v3fatalSrc("Unknown node type in emitInitializerBody: " << nodep->prettyTypeName());
    }

    // Emit the braced initializer for the given value '{...}'
    ശൂന്യം emitInitializer(Ast_കണ്ണി_വ്യഞ്ജകം* nodep) {
        putns(nodep, "{");
        emitInitializerBody(nodep);
        puts("}");
    }

സംരക്ഷിത:
    // Emit a C++ value constructor: 'Type{...}'
    ശൂന്യം emitValCons(Ast_കണ്ണി_Cons* nodep) {
        putnbs(nodep, nodep->dtypep()->cType("", false, false));
        emitInitializer(nodep);
    }

    // Emit the C++ variable definition 'Type name{...}'
    ശൂന്യം emitVarDefn(സ്ഥിരം AstVar* varp) {
        സ്ഥിരം std::string name = varp->constPoolEntry()
                                     ? EmitCUtil::constPoolName(varp)
                                     : EmitCUtil::prefixNameProtect(EmitCParentModule::get(varp))
                                           + "::" + varp->nameProtect();
        putnbs(varp, varp->dtypep()->cType(name, false, false));
        emitInitializer(VN_AS(varp->valuep(), കണ്ണി_വ്യഞ്ജകം));
    }
};
