// -*- mode: C++; c-file-style: "cc-mode" -*-
//*************************************************************************
// DESCRIPTION: Verilator: Parse syntax tree
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

#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ YYDEBUG 1  // Nicer errors

#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ VL_MT_DISABLED_ਕੋਡ_UNIT 1

#ਅਨ੍ਤਰ੍ਭਾਵ "V3Ast.h"  // This must be before V3ParseBison.cpp, as we don't want #defines to conflict

VL_DEFINE_DEBUG_FUNCTIONS;

//======================================================================
// The guts come from bison output

#ਅਨ੍ਤਰ੍ਭਾਵ "V3ParseBison.c"

//======================================================================
// V3ParseImp functions requiring bison state

ਪੂਰਨ V3ParseImp::bisonParse() {
    // Use --debugi-bison 9 to enable this
    ਜੇ (PARSEP->debugBison() >= 9) yydebug = 1;
    ਵਾਪਸ yyparse();
}

ਅਚਲ ਅੱਖਰ* V3ParseImp::tokenName(ਪੂਰਨ token) {
#ਜੇ YYDEBUG || YYERROR_VERBOSE
    ਸਥਿਰ ਅਚਲ ਅੱਖਰ** nameTablep = ਖਾਲੀ_ਸੂਚਕ;
    ਜੇ (!nameTablep) {
        ਪੂਰਨ ਮਾਪ;
        ਲਈ (ਮਾਪ = 0; yytname[ਮਾਪ]; ++ਮਾਪ) {}
        nameTablep = ਨਵਾਂ ਅਚਲ ਅੱਖਰ*[ਮਾਪ];
        // Workaround bug in bison's which have '!' in yytname but not token values
        ਪੂਰਨ iout = 0;
        ਲਈ (ਪੂਰਨ i = 0; yytname[i]; ++i) {
            ਜੇ (yytname[i][0] == '\'') ਜਾਰੀ;
            nameTablep[iout++] = yytname[i];
        }
    }
    ਜੇ (token >= 255) {
        ਵਾਪਸ nameTablep[token - 255];
    } ਨਹੀਂਤਾਂ {
        ਸਥਿਰ ਅੱਖਰ s_ch[2];
        s_ch[0] = token;
        s_ch[1] = '\0';
        ਵਾਪਸ s_ch;
    }
#ਨਹੀਂਤਾਂ
    ਵਾਪਸ "";
#ਜੇ_ਅੰਤ
}

ਖਾਲੀ V3ParseImp::candidatePli(VSpellCheck* spellerp) {
#ਜੇ !YYERROR_VERBOSE
#ਗਲਤੀ "Need lex token names"
#ਜੇ_ਅੰਤ
    ਲਈ (ਪੂਰਨ i = 0; yytname[i]; ++i) {
        ਜੇ (yytname[i][0] != '\"') ਜਾਰੀ;
        ਜੇ (yytname[i][1] != '$') ਜਾਰੀ;
        spellerp->ਧੱਕੋ_Candidate(string{yytname[i]}.substr(1, ਲੜੀ_ਲੰਬਾਈ(yytname[i]) - 2));
    }
}

ਖਾਲੀ V3ParseImp::parserClear() {
    // Clear up any dynamic memory V3Parser required
    VARDTYPE(ਖਾਲੀ_ਸੂਚਕ);
    GRAMMARP->ਸੈੱਟ_Net_Delay(ਖਾਲੀ_ਸੂਚਕ);
    GRAMMARP->ਸੈੱਟ_Scoped_Sig_Attr(ਖਾਲੀ_ਸੂਚਕ);
}

//======================================================================
// V3ParseGrammar functions requiring bison state

AstArg* V3ParseGrammar::argWrapList(Ast_ਗੰਢ_ਵਿਅੰਜਕ* nodep) {
    // Convert list of expressions to list of arguments
    AstArg* outp = ਖਾਲੀ_ਸੂਚਕ;
    ਜਦੋਂ (nodep) {
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ nextp = VN_AS(nodep->nextp(), ਗੰਢ_ਵਿਅੰਜਕ);
        ਜੇ (nextp) nextp->unlink_Fr_ਪਿੱਠ_With_ਅਗਲਾ();
        outp = Ast_ਗੰਢ::ਜੋੜੋ_ਅਗਲਾ(outp, ਨਵਾਂ AstArg{nodep->fileline(), "", nodep});
        nodep = nextp;
    }
    ਵਾਪਸ outp;
}

Ast_ਸੌਂਪੋ_W* V3ParseGrammar::create_Supply_ਵਿਅੰਜਕ(File_ਸਤਰ* fileline, ਅਚਲ string& name, ਪੂਰਨ ਮੁੱਲ) {
    Ast_ਸੌਂਪੋ_W* assignp = ਨਵਾਂ Ast_ਸੌਂਪੋ_W{fileline, ਨਵਾਂ AstParseRef{fileline, name},
                                         ਮੁੱਲ ? ਨਵਾਂ AstConst{fileline, AstConst::All1{}}
                                               : ਨਵਾਂ AstConst{fileline, AstConst::All0{}}};
    AstStrengthSpec* strengthSpecp
        = ਨਵਾਂ AstStrengthSpec{fileline, VStrength::SUPPLY, VStrength::SUPPLY};
    assignp->strengthSpecp(strengthSpecp);
    ਵਾਪਸ assignp;
}

AstRange* V3ParseGrammar::scrubRangeMulti(Ast_ਗੰਢ_Range* nrangep) {
    // Remove any UnsizedRange's from list; preserves multi-dim chain via nextp().
    ਲਈ (Ast_ਗੰਢ_Range *nodep = nrangep, *nextp; nodep; nodep = nextp) {
        nextp = VN_AS(nodep->nextp(), ਗੰਢ_Range);
        ਜੇ (!VN_IS(nodep, Range)) {
            nodep->v3error(
                "Unsupported or syntax error: Unsized range in instance or other declaration");
            nodep->unlink_Fr_ਪਿੱਠ();
            VL_DO_DANGLING(nodep->delete_ਰੁੱਖ(), nodep);
        }
    }
    ਵਾਪਸ VN_CAST(nrangep, Range);
}
AstRange* V3ParseGrammar::scrubRange(Ast_ਗੰਢ_Range* nrangep) {
    AstRange* ਅਚਲ rangep = scrubRangeMulti(nrangep);
    ਜੇ (rangep && rangep->nextp()) {
        // Gate primitives only support a single dimension
        rangep->nextp()->v3warn(E_UNSUPPORTED, "Unsupported: Multidimensional gate instances.");
        rangep->nextp()->unlink_Fr_ਪਿੱਠ_With_ਅਗਲਾ()->delete_ਰੁੱਖ();
    }
    ਵਾਪਸ rangep;
}

Ast_ਗੰਢ_Pre_Sel* V3ParseGrammar::scrubSel(Ast_ਗੰਢ_ਵਿਅੰਜਕ* fromp, Ast_ਗੰਢ_Pre_Sel* selp) VL_MT_DISABLED {
    // SEL(PARSELVALUE, ...) -> SEL(fromp, ...)
    Ast_ਗੰਢ_Pre_Sel* subSelp = selp;
    ਜਦੋਂ (ਸੱਚ) {
        ਜੇ (VN_IS(subSelp->fromp(), ParseHolder)) ਰੋਕੋ;
        ਜੇ (Ast_ਗੰਢ_Pre_Sel* ਅਚਲ ਹੇਠਲਾ_Selp = VN_CAST(subSelp->fromp(), ਗੰਢ_Pre_Sel)) {
            subSelp = ਹੇਠਲਾ_Selp;
            ਜਾਰੀ;
        }
        subSelp->v3fatalSrc("Couldn't find where to insert expression into select");
    }
    Ast_ਗੰਢ* subSelFromp = subSelp->fromp();
    subSelFromp->replaceWith(fromp);
    VL_DO_DANGLING(subSelFromp->delete_ਰੁੱਖ(), subSelFromp);
    ਵਾਪਸ selp;
}

Ast_ਗੰਢ_DType* V3ParseGrammar::createArray(Ast_ਗੰਢ_DType* basep, Ast_ਗੰਢ_Range* nrangep,
                                          ਤਾਰਕਿਕ isPacked) {
    // Split RANGE0-RANGE1-RANGE2
    // into ARRAYDTYPE0(ARRAYDTYPE1(ARRAYDTYPE2(BASICTYPE3), RANGE), RANGE)
    Ast_ਗੰਢ_DType* arrayp = basep;
    ਜੇ (nrangep) {  // Maybe no range - return unmodified base type
        ਜਦੋਂ (nrangep->nextp()) nrangep = VN_AS(nrangep->nextp(), ਗੰਢ_Range);
        ਜਦੋਂ (nrangep) {
            Ast_ਗੰਢ_Range* ਅਚਲ prevp = VN_AS(nrangep->backp(), ਗੰਢ_Range);
            ਜੇ (prevp) nrangep->unlink_Fr_ਪਿੱਠ();
            AstRange* ਅਚਲ rangep = VN_CAST(nrangep, Range);
            ਜੇ (rangep && isPacked) {
                arrayp
                    = ਨਵਾਂ AstPackArrayDType{rangep->fileline(), VFlag_ਬੱਚਾ_DType{}, arrayp, rangep};
            } ਨਹੀਂਤਾਂ ਜੇ (rangep
                       && (VN_IS(rangep->leftp(), Unbounded)
                           || VN_IS(rangep->rightp(), Unbounded))) {
                arrayp = ਨਵਾਂ Ast_ਕਤਾਰ_DType{nrangep->fileline(), VFlag_ਬੱਚਾ_DType{}, arrayp,
                                           rangep->rightp()->clone_ਰੁੱਖ(ਸੱਚ)};
                VL_DO_DANGLING(nrangep->delete_ਰੁੱਖ(), nrangep);
            } ਨਹੀਂਤਾਂ ਜੇ (rangep) {
                arrayp = ਨਵਾਂ AstUnpackArrayDType{rangep->fileline(), VFlag_ਬੱਚਾ_DType{}, arrayp,
                                                 rangep};
            } ਨਹੀਂਤਾਂ ਜੇ (VN_IS(nrangep, UnsizedRange)) {
                arrayp = ਨਵਾਂ AstDynArrayDType{nrangep->fileline(), VFlag_ਬੱਚਾ_DType{}, arrayp};
                VL_DO_DANGLING(nrangep->delete_ਰੁੱਖ(), nrangep);
            } ਨਹੀਂਤਾਂ ਜੇ (VN_IS(nrangep, BracketRange)) {
                ਅਚਲ AstBracketRange* ਅਚਲ arangep = VN_AS(nrangep, BracketRange);
                Ast_ਗੰਢ* ਅਚਲ keyp = arangep->elementsp()->unlink_Fr_ਪਿੱਠ();
                arrayp = ਨਵਾਂ AstBracketArrayDType{nrangep->fileline(), VFlag_ਬੱਚਾ_DType{}, arrayp,
                                                  keyp};
                VL_DO_DANGLING(nrangep->delete_ਰੁੱਖ(), nrangep);
            } ਨਹੀਂਤਾਂ ਜੇ (VN_IS(nrangep, WildcardRange)) {
                arrayp = ਨਵਾਂ AstWildcardArrayDType{nrangep->fileline(), VFlag_ਬੱਚਾ_DType{}, arrayp};
                VL_DO_DANGLING(nrangep->delete_ਰੁੱਖ(), nrangep);
            } ਨਹੀਂਤਾਂ {
                UASSERT_OBJ(0, nrangep, "Expected range or unsized range");
            }
            nrangep = prevp;
        }
    }
    ਵਾਪਸ arrayp;
}

AstVar* V3ParseGrammar::createVariable(File_ਸਤਰ* fileline, ਅਚਲ string& name,
                                       Ast_ਗੰਢ_Range* arrayp, Ast_ਗੰਢ* attrsp) {
    UINFO(5, "  creVar " << name << "  decl=" << GRAMMARP->m_varDecl << "  io="
                         << GRAMMARP->m_varIO << "  dt=" << (GRAMMARP->m_varDTypep ? "set" : ""));
    ਜੇ (GRAMMARP->m_varIO == VDirection::NONE  // In non-ANSI port list
        && GRAMMARP->m_varDecl == VVarType::ਪੋਰਟ) {
        // Just a port list with variable name (not v2k format); AstPort already created
        ਜੇ (arrayp) VL_DO_DANGLING(arrayp->delete_ਰੁੱਖ(), arrayp);
        ਜੇ (attrsp) {
            // TODO: Merge attributes across list? Or warn attribute is ignored
            VL_DO_DANGLING(attrsp->delete_ਰੁੱਖ(), attrsp);
        }
        ਵਾਪਸ ਖਾਲੀ_ਸੂਚਕ;
    }

    Ast_ਗੰਢ_DType* ਅਚਲ dtypep = [&]() -> Ast_ਗੰਢ_DType* {
        ਜੇ (GRAMMARP->m_varDecl == VVarType::WREAL) {
            // dtypep might not be null, might be implicit LOGIC before we knew better
            ਵਾਪਸ ਨਵਾਂ AstBasicDType{fileline, VBasicDTypeKwd::DOUBLE};
        }
        ਜੇ (GRAMMARP->m_varDTypep) {
            // May make new variables with same type, so clone
            ਵਾਪਸ GRAMMARP->m_varDTypep->clone_ਰੁੱਖ(ਝੂਠ);
        }
        // Created implicitly
        ਜੇ (m_insideProperty) {
            ਜੇ (m_typed_Property_ਪੋਰਟ) {
                fileline->v3warn(E_UNSUPPORTED, "Untyped property port following a typed port");
            }
            ਵਾਪਸ ਨਵਾਂ AstBasicDType{fileline, VBasicDTypeKwd::UNTYPED};
        }
        ਵਾਪਸ ਨਵਾਂ AstBasicDType{fileline, LOGIC_IMPLICIT};
    }();

    // UINFO(0,"CREVAR "<<fileline->ascii()<<" decl="<<GRAMMARP->m_varDecl.ascii()<<"
    // io="<<GRAMMARP->m_varIO.ascii()<<endl);
    VVarType type = GRAMMARP->m_varDecl;
    ਜੇ (type == VVarType::UNKNOWN) {  // e.g. "output" non-ANSI standalone direction (vs "reg")
        ਜੇ (GRAMMARP->m_varIO.isAny()) {
            type = VVarType::ਪੋਰਟ;
        } ਨਹੀਂਤਾਂ {
            fileline->v3fatalSrc("Unknown signal type declared: " << type.ascii());
        }
    }
    ਜੇ (type == VVarType::GENVAR) {
        // Should be impossible as the grammar blocks this, but...
        ਜੇ (arrayp) fileline->v3error("Genvars may not be arrayed: " << name);  // LCOV_EXCL_LINE
    }

    // Split RANGE0-RANGE1-RANGE2 into
    // ARRAYDTYPE0(ARRAYDTYPE1(ARRAYDTYPE2(BASICTYPE3), RANGE), RANGE)
    Ast_ਗੰਢ_DType* ਅਚਲ arrayDTypep = createArray(dtypep, arrayp, ਝੂਠ);

    AstVar* ਅਚਲ nodep = ਨਵਾਂ AstVar{fileline, type, name, VFlag_ਬੱਚਾ_DType(), arrayDTypep};
    nodep->ਜੋੜੋ_Attrsp(attrsp);
    nodep->ansi(m_pinAnsi);
    nodep->declTyped(m_varDeclTyped);
    nodep->lifetime(m_varLifetime);
    ਜੇ (m_netDelayp) nodep->delayp(m_netDelayp->clone_ਰੁੱਖ(ਝੂਠ));
    ਜੇ (GRAMMARP->m_varDecl != VVarType::UNKNOWN) nodep->combineType(GRAMMARP->m_varDecl);
    ਜੇ (GRAMMARP->m_varIO != VDirection::NONE) {
        nodep->declDirection(GRAMMARP->m_varIO);
        nodep->direction(GRAMMARP->m_varIO);
    }

    ਜੇ (GRAMMARP->m_varDecl == VVarType::SUPPLY0) {
        Ast_ਸੌਂਪੋ_W* ਅਚਲ ap = V3ParseGrammar::create_Supply_ਵਿਅੰਜਕ(fileline, nodep->name(), 0);
        Ast_ਗੰਢ::ਜੋੜੋ_ਅਗਲਾ<Ast_ਗੰਢ, Ast_ਗੰਢ>(nodep, ਨਵਾਂ AstAlways{ap});
    }
    ਜੇ (GRAMMARP->m_varDecl == VVarType::SUPPLY1) {
        Ast_ਸੌਂਪੋ_W* ਅਚਲ ap = V3ParseGrammar::create_Supply_ਵਿਅੰਜਕ(fileline, nodep->name(), 1);
        Ast_ਗੰਢ::ਜੋੜੋ_ਅਗਲਾ<Ast_ਗੰਢ, Ast_ਗੰਢ>(nodep, ਨਵਾਂ AstAlways{ap});
    }
    ਜੇ (VN_IS(dtypep, ParseTypeDType)) {
        // Parser needs to know what is a type
        Ast_ਗੰਢ* ਅਚਲ newp = ਨਵਾਂ AstTypedefFwd{fileline, name, VFwdType::NONE};
        Ast_ਗੰਢ::ਜੋੜੋ_ਅਗਲਾ<Ast_ਗੰਢ, Ast_ਗੰਢ>(nodep, newp);
    }
    // Don't set dtypep in the ranging;
    // We need to autosize parameters and integers separately
    //
    // Propagate from current module tracing state
    ਜੇ (nodep->isGenVar()) {
        nodep->trace(ਝੂਠ);
    } ਨਹੀਂਤਾਂ ਜੇ (nodep->isParam() && !v3Global.opt.traceParams()) {
        nodep->trace(ਝੂਠ);
    } ਨਹੀਂਤਾਂ {
        nodep->trace(allTracingOn(nodep->fileline()));
    }
    ਜੇ (nodep->varType().isVPIAccessible()) nodep->ਜੋੜੋ_Attrsp(GRAMMARP->cloneScopedSigAttr());

    // Remember the last variable created, so we can attach attributes to it in later parsing
    GRAMMARP->m_varAttrp = nodep;
    PARSEP->ਟੈਗ_Nodep(GRAMMARP->m_varAttrp);
    ਵਾਪਸ nodep;
}

string V3ParseGrammar::unquoteString(File_ਸਤਰ* fileline, ਅਚਲ std::string& ਪਾਠ) {
    string err_ਸੁਨੇਹਾ;
    string res = VString::unquoteSVString(ਪਾਠ, err_ਸੁਨੇਹਾ);
    ਜੇ (!err_ਸੁਨੇਹਾ.ਖਾਲੀ_ਥਾਵਾਂ()) fileline->v3error(err_ਸੁਨੇਹਾ.c_str());
    ਵਾਪਸ res;
}

ਪੂਰਨ V3ParseGrammar::s_type_Imp_ਅੰਕ = 0;
