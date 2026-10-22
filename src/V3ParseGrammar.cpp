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

#ವ್ಯಾಖ್ಯಾನಿಸು YYDEBUG 1  // Nicer errors

#ವ್ಯಾಖ್ಯಾನಿಸು VL_MT_DISABLED_ಸಂಕೇತ_UNIT 1

#ಅನ್ತರ್ಭಾವ "V3Ast.h"  // This must be before V3ParseBison.cpp, as we don't want #defines to conflict

VL_DEFINE_DEBUG_FUNCTIONS;

//======================================================================
// The guts come from bison output

#ಅನ್ತರ್ಭಾವ "V3ParseBison.c"

//======================================================================
// V3ParseImp functions requiring bison state

ಪೂರ್ಣಾಂಕ V3ParseImp::bisonParse() {
    // Use --debugi-bison 9 to enable this
    ಆದರೆ (PARSEP->debugBison() >= 9) yydebug = 1;
    ಹಿಂತಿರುಗಿ yyparse();
}

ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* V3ParseImp::tokenName(ಪೂರ್ಣಾಂಕ token) {
#ಆದರೆ YYDEBUG || YYERROR_VERBOSE
    ಸ್ಥಿರ ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ** nameTablep = ಶೂನ್ಯಸೂಚಕ;
    ಆದರೆ (!nameTablep) {
        ಪೂರ್ಣಾಂಕ ಗಾತ್ರ_2;
        ಗಾಗಿ (ಗಾತ್ರ_2 = 0; yytname[ಗಾತ್ರ_2]; ++ಗಾತ್ರ_2) {}
        nameTablep = ಹೊಸ ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ*[ಗಾತ್ರ_2];
        // Workaround bug in bison's which have '!' in yytname but not token values
        ಪೂರ್ಣಾಂಕ iout = 0;
        ಗಾಗಿ (ಪೂರ್ಣಾಂಕ i = 0; yytname[i]; ++i) {
            ಆದರೆ (yytname[i][0] == '\'') ಮುಂದುವರಿ;
            nameTablep[iout++] = yytname[i];
        }
    }
    ಆದರೆ (token >= 255) {
        ಹಿಂತಿರುಗಿ nameTablep[token - 255];
    } ಇಲ್ಲದಿದ್ದರೆ {
        ಸ್ಥಿರ ಅಕ್ಷರ s_ch[2];
        s_ch[0] = token;
        s_ch[1] = '\0';
        ಹಿಂತಿರುಗಿ s_ch;
    }
#ಇಲ್ಲದಿದ್ದರೆ
    ಹಿಂತಿರುಗಿ "";
#ಆದರೆ_ಅಂತ್ಯ
}

ಶೂನ್ಯ V3ParseImp::candidatePli(VSpellCheck* spellerp) {
#ಆದರೆ !YYERROR_VERBOSE
#ದೋಷ "Need lex token names"
#ಆದರೆ_ಅಂತ್ಯ
    ಗಾಗಿ (ಪೂರ್ಣಾಂಕ i = 0; yytname[i]; ++i) {
        ಆದರೆ (yytname[i][0] != '\"') ಮುಂದುವರಿ;
        ಆದರೆ (yytname[i][1] != '$') ಮುಂದುವರಿ;
        spellerp->ತಳ್ಳು_Candidate(string{yytname[i]}.substr(1, ಅಕ್ಷರಸರಣಿ_ಉದ್ದ(yytname[i]) - 2));
    }
}

ಶೂನ್ಯ V3ParseImp::parserClear() {
    // Clear up any dynamic memory V3Parser required
    VARDTYPE(ಶೂನ್ಯಸೂಚಕ);
    GRAMMARP->ಗಣ_Net_Delay(ಶೂನ್ಯಸೂಚಕ);
    GRAMMARP->ಗಣ_Scoped_Sig_Attr(ಶೂನ್ಯಸೂಚಕ);
}

//======================================================================
// V3ParseGrammar functions requiring bison state

AstArg* V3ParseGrammar::argWrapList(Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* nodep) {
    // Convert list of expressions to list of arguments
    AstArg* outp = ಶೂನ್ಯಸೂಚಕ;
    ಆಗ (nodep) {
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ nextp = VN_AS(nodep->nextp(), ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ);
        ಆದರೆ (nextp) nextp->unlink_Fr_ಹಿಂಭಾಗ_With_ಮುಂದಿನ();
        outp = Ast_ಗಂಟು::ಕೂಡಿಸು_ಮುಂದಿನ(outp, ಹೊಸ AstArg{nodep->fileline(), "", nodep});
        nodep = nextp;
    }
    ಹಿಂತಿರುಗಿ outp;
}

Ast_ನಿಯೋಜಿಸು_W* V3ParseGrammar::create_Supply_ಅಭಿವ್ಯಕ್ತಿ(File_ಸಾಲು* fileline, ಸ್ಥಿರಾಂಕ string& name, ಪೂರ್ಣಾಂಕ ಮೌಲ್ಯ) {
    Ast_ನಿಯೋಜಿಸು_W* assignp = ಹೊಸ Ast_ನಿಯೋಜಿಸು_W{fileline, ಹೊಸ AstParseRef{fileline, name},
                                         ಮೌಲ್ಯ ? ಹೊಸ AstConst{fileline, AstConst::All1{}}
                                               : ಹೊಸ AstConst{fileline, AstConst::All0{}}};
    AstStrengthSpec* strengthSpecp
        = ಹೊಸ AstStrengthSpec{fileline, VStrength::SUPPLY, VStrength::SUPPLY};
    assignp->strengthSpecp(strengthSpecp);
    ಹಿಂತಿರುಗಿ assignp;
}

AstRange* V3ParseGrammar::scrubRangeMulti(Ast_ಗಂಟು_Range* nrangep) {
    // Remove any UnsizedRange's from list; preserves multi-dim chain via nextp().
    ಗಾಗಿ (Ast_ಗಂಟು_Range *nodep = nrangep, *nextp; nodep; nodep = nextp) {
        nextp = VN_AS(nodep->nextp(), ಗಂಟು_Range);
        ಆದರೆ (!VN_IS(nodep, Range)) {
            nodep->v3error(
                "Unsupported or syntax error: Unsized range in instance or other declaration");
            nodep->unlink_Fr_ಹಿಂಭಾಗ();
            VL_DO_DANGLING(nodep->delete_ವೃಕ್ಷ(), nodep);
        }
    }
    ಹಿಂತಿರುಗಿ VN_CAST(nrangep, Range);
}
AstRange* V3ParseGrammar::scrubRange(Ast_ಗಂಟು_Range* nrangep) {
    AstRange* ಸ್ಥಿರಾಂಕ rangep = scrubRangeMulti(nrangep);
    ಆದರೆ (rangep && rangep->nextp()) {
        // Gate primitives only support a single dimension
        rangep->nextp()->v3warn(E_UNSUPPORTED, "Unsupported: Multidimensional gate instances.");
        rangep->nextp()->unlink_Fr_ಹಿಂಭಾಗ_With_ಮುಂದಿನ()->delete_ವೃಕ್ಷ();
    }
    ಹಿಂತಿರುಗಿ rangep;
}

Ast_ಗಂಟು_Pre_Sel* V3ParseGrammar::scrubSel(Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* fromp, Ast_ಗಂಟು_Pre_Sel* selp) VL_MT_DISABLED {
    // SEL(PARSELVALUE, ...) -> SEL(fromp, ...)
    Ast_ಗಂಟು_Pre_Sel* subSelp = selp;
    ಆಗ (ನಿಜ) {
        ಆದರೆ (VN_IS(subSelp->fromp(), ParseHolder)) ನಿಲ್ಲಿಸು;
        ಆದರೆ (Ast_ಗಂಟು_Pre_Sel* ಸ್ಥಿರಾಂಕ ಅಧೋ_Selp = VN_CAST(subSelp->fromp(), ಗಂಟು_Pre_Sel)) {
            subSelp = ಅಧೋ_Selp;
            ಮುಂದುವರಿ;
        }
        subSelp->v3fatalSrc("Couldn't find where to insert expression into select");
    }
    Ast_ಗಂಟು* subSelFromp = subSelp->fromp();
    subSelFromp->replaceWith(fromp);
    VL_DO_DANGLING(subSelFromp->delete_ವೃಕ್ಷ(), subSelFromp);
    ಹಿಂತಿರುಗಿ selp;
}

Ast_ಗಂಟು_DType* V3ParseGrammar::createArray(Ast_ಗಂಟು_DType* basep, Ast_ಗಂಟು_Range* nrangep,
                                          ತಾರ್ಕಿಕ isPacked) {
    // Split RANGE0-RANGE1-RANGE2
    // into ARRAYDTYPE0(ARRAYDTYPE1(ARRAYDTYPE2(BASICTYPE3), RANGE), RANGE)
    Ast_ಗಂಟು_DType* arrayp = basep;
    ಆದರೆ (nrangep) {  // Maybe no range - return unmodified base type
        ಆಗ (nrangep->nextp()) nrangep = VN_AS(nrangep->nextp(), ಗಂಟು_Range);
        ಆಗ (nrangep) {
            Ast_ಗಂಟು_Range* ಸ್ಥಿರಾಂಕ prevp = VN_AS(nrangep->backp(), ಗಂಟು_Range);
            ಆದರೆ (prevp) nrangep->unlink_Fr_ಹಿಂಭಾಗ();
            AstRange* ಸ್ಥಿರಾಂಕ rangep = VN_CAST(nrangep, Range);
            ಆದರೆ (rangep && isPacked) {
                arrayp
                    = ಹೊಸ AstPackArrayDType{rangep->fileline(), VFlag_ಮಗು_DType{}, arrayp, rangep};
            } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (rangep
                       && (VN_IS(rangep->leftp(), Unbounded)
                           || VN_IS(rangep->rightp(), Unbounded))) {
                arrayp = ಹೊಸ Ast_ಸರತಿ_DType{nrangep->fileline(), VFlag_ಮಗು_DType{}, arrayp,
                                           rangep->rightp()->clone_ವೃಕ್ಷ(ನಿಜ)};
                VL_DO_DANGLING(nrangep->delete_ವೃಕ್ಷ(), nrangep);
            } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (rangep) {
                arrayp = ಹೊಸ AstUnpackArrayDType{rangep->fileline(), VFlag_ಮಗು_DType{}, arrayp,
                                                 rangep};
            } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (VN_IS(nrangep, UnsizedRange)) {
                arrayp = ಹೊಸ AstDynArrayDType{nrangep->fileline(), VFlag_ಮಗು_DType{}, arrayp};
                VL_DO_DANGLING(nrangep->delete_ವೃಕ್ಷ(), nrangep);
            } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (VN_IS(nrangep, BracketRange)) {
                ಸ್ಥಿರಾಂಕ AstBracketRange* ಸ್ಥಿರಾಂಕ arangep = VN_AS(nrangep, BracketRange);
                Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ keyp = arangep->elementsp()->unlink_Fr_ಹಿಂಭಾಗ();
                arrayp = ಹೊಸ AstBracketArrayDType{nrangep->fileline(), VFlag_ಮಗು_DType{}, arrayp,
                                                  keyp};
                VL_DO_DANGLING(nrangep->delete_ವೃಕ್ಷ(), nrangep);
            } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (VN_IS(nrangep, WildcardRange)) {
                arrayp = ಹೊಸ AstWildcardArrayDType{nrangep->fileline(), VFlag_ಮಗು_DType{}, arrayp};
                VL_DO_DANGLING(nrangep->delete_ವೃಕ್ಷ(), nrangep);
            } ಇಲ್ಲದಿದ್ದರೆ {
                UASSERT_OBJ(0, nrangep, "Expected range or unsized range");
            }
            nrangep = prevp;
        }
    }
    ಹಿಂತಿರುಗಿ arrayp;
}

AstVar* V3ParseGrammar::createVariable(File_ಸಾಲು* fileline, ಸ್ಥಿರಾಂಕ string& name,
                                       Ast_ಗಂಟು_Range* arrayp, Ast_ಗಂಟು* attrsp) {
    UINFO(5, "  creVar " << name << "  decl=" << GRAMMARP->m_varDecl << "  io="
                         << GRAMMARP->m_varIO << "  dt=" << (GRAMMARP->m_varDTypep ? "set" : ""));
    ಆದರೆ (GRAMMARP->m_varIO == VDirection::NONE  // In non-ANSI port list
        && GRAMMARP->m_varDecl == VVarType::ಪೋರ್ಟ್) {
        // Just a port list with variable name (not v2k format); AstPort already created
        ಆದರೆ (arrayp) VL_DO_DANGLING(arrayp->delete_ವೃಕ್ಷ(), arrayp);
        ಆದರೆ (attrsp) {
            // TODO: Merge attributes across list? Or warn attribute is ignored
            VL_DO_DANGLING(attrsp->delete_ವೃಕ್ಷ(), attrsp);
        }
        ಹಿಂತಿರುಗಿ ಶೂನ್ಯಸೂಚಕ;
    }

    Ast_ಗಂಟು_DType* ಸ್ಥಿರಾಂಕ dtypep = [&]() -> Ast_ಗಂಟು_DType* {
        ಆದರೆ (GRAMMARP->m_varDecl == VVarType::WREAL) {
            // dtypep might not be null, might be implicit LOGIC before we knew better
            ಹಿಂತಿರುಗಿ ಹೊಸ AstBasicDType{fileline, VBasicDTypeKwd::DOUBLE};
        }
        ಆದರೆ (GRAMMARP->m_varDTypep) {
            // May make new variables with same type, so clone
            ಹಿಂತಿರುಗಿ GRAMMARP->m_varDTypep->clone_ವೃಕ್ಷ(ಸುಳ್ಳು);
        }
        // Created implicitly
        ಆದರೆ (m_insideProperty) {
            ಆದರೆ (m_typed_Property_ಪೋರ್ಟ್) {
                fileline->v3warn(E_UNSUPPORTED, "Untyped property port following a typed port");
            }
            ಹಿಂತಿರುಗಿ ಹೊಸ AstBasicDType{fileline, VBasicDTypeKwd::UNTYPED};
        }
        ಹಿಂತಿರುಗಿ ಹೊಸ AstBasicDType{fileline, LOGIC_IMPLICIT};
    }();

    // UINFO(0,"CREVAR "<<fileline->ascii()<<" decl="<<GRAMMARP->m_varDecl.ascii()<<"
    // io="<<GRAMMARP->m_varIO.ascii()<<endl);
    VVarType type = GRAMMARP->m_varDecl;
    ಆದರೆ (type == VVarType::UNKNOWN) {  // e.g. "output" non-ANSI standalone direction (vs "reg")
        ಆದರೆ (GRAMMARP->m_varIO.isAny()) {
            type = VVarType::ಪೋರ್ಟ್;
        } ಇಲ್ಲದಿದ್ದರೆ {
            fileline->v3fatalSrc("Unknown signal type declared: " << type.ascii());
        }
    }
    ಆದರೆ (type == VVarType::GENVAR) {
        // Should be impossible as the grammar blocks this, but...
        ಆದರೆ (arrayp) fileline->v3error("Genvars may not be arrayed: " << name);  // LCOV_EXCL_LINE
    }

    // Split RANGE0-RANGE1-RANGE2 into
    // ARRAYDTYPE0(ARRAYDTYPE1(ARRAYDTYPE2(BASICTYPE3), RANGE), RANGE)
    Ast_ಗಂಟು_DType* ಸ್ಥಿರಾಂಕ arrayDTypep = createArray(dtypep, arrayp, ಸುಳ್ಳು);

    AstVar* ಸ್ಥಿರಾಂಕ nodep = ಹೊಸ AstVar{fileline, type, name, VFlag_ಮಗು_DType(), arrayDTypep};
    nodep->ಕೂಡಿಸು_Attrsp(attrsp);
    nodep->ansi(m_pinAnsi);
    nodep->declTyped(m_varDeclTyped);
    nodep->lifetime(m_varLifetime);
    ಆದರೆ (m_netDelayp) nodep->delayp(m_netDelayp->clone_ವೃಕ್ಷ(ಸುಳ್ಳು));
    ಆದರೆ (GRAMMARP->m_varDecl != VVarType::UNKNOWN) nodep->combineType(GRAMMARP->m_varDecl);
    ಆದರೆ (GRAMMARP->m_varIO != VDirection::NONE) {
        nodep->declDirection(GRAMMARP->m_varIO);
        nodep->direction(GRAMMARP->m_varIO);
    }

    ಆದರೆ (GRAMMARP->m_varDecl == VVarType::SUPPLY0) {
        Ast_ನಿಯೋಜಿಸು_W* ಸ್ಥಿರಾಂಕ ap = V3ParseGrammar::create_Supply_ಅಭಿವ್ಯಕ್ತಿ(fileline, nodep->name(), 0);
        Ast_ಗಂಟು::ಕೂಡಿಸು_ಮುಂದಿನ<Ast_ಗಂಟು, Ast_ಗಂಟು>(nodep, ಹೊಸ AstAlways{ap});
    }
    ಆದರೆ (GRAMMARP->m_varDecl == VVarType::SUPPLY1) {
        Ast_ನಿಯೋಜಿಸು_W* ಸ್ಥಿರಾಂಕ ap = V3ParseGrammar::create_Supply_ಅಭಿವ್ಯಕ್ತಿ(fileline, nodep->name(), 1);
        Ast_ಗಂಟು::ಕೂಡಿಸು_ಮುಂದಿನ<Ast_ಗಂಟು, Ast_ಗಂಟು>(nodep, ಹೊಸ AstAlways{ap});
    }
    ಆದರೆ (VN_IS(dtypep, ParseTypeDType)) {
        // Parser needs to know what is a type
        Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ newp = ಹೊಸ AstTypedefFwd{fileline, name, VFwdType::NONE};
        Ast_ಗಂಟು::ಕೂಡಿಸು_ಮುಂದಿನ<Ast_ಗಂಟು, Ast_ಗಂಟು>(nodep, newp);
    }
    // Don't set dtypep in the ranging;
    // We need to autosize parameters and integers separately
    //
    // Propagate from current module tracing state
    ಆದರೆ (nodep->isGenVar()) {
        nodep->trace(ಸುಳ್ಳು);
    } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (nodep->isParam() && !v3Global.opt.traceParams()) {
        nodep->trace(ಸುಳ್ಳು);
    } ಇಲ್ಲದಿದ್ದರೆ {
        nodep->trace(allTracingOn(nodep->fileline()));
    }
    ಆದರೆ (nodep->varType().isVPIAccessible()) nodep->ಕೂಡಿಸು_Attrsp(GRAMMARP->cloneScopedSigAttr());

    // Remember the last variable created, so we can attach attributes to it in later parsing
    GRAMMARP->m_varAttrp = nodep;
    PARSEP->ಟ್ಯಾಗ್_Nodep(GRAMMARP->m_varAttrp);
    ಹಿಂತಿರುಗಿ nodep;
}

string V3ParseGrammar::unquoteString(File_ಸಾಲು* fileline, ಸ್ಥಿರಾಂಕ std::string& ಪಠ್ಯ) {
    string err_ಸಂದೇಶ;
    string res = VString::unquoteSVString(ಪಠ್ಯ, err_ಸಂದೇಶ);
    ಆದರೆ (!err_ಸಂದೇಶ.ಖಾಲಿಗಳು()) fileline->v3error(err_ಸಂದೇಶ.c_str());
    ಹಿಂತಿರುಗಿ res;
}

ಪೂರ್ಣಾಂಕ V3ParseGrammar::s_type_Imp_ಅಂಕೆ = 0;
