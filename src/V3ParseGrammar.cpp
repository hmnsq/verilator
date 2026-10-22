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

#define YYDEBUG 1  // Nicer errors

#define VL_MT_DISABLED_CODE_UNIT 1

#ಅನ್ತರ್ಭಾವ "V3Ast.h"  // This must be before V3ParseBison.cpp, as we don't want #defines to conflict

VL_DEFINE_DEBUG_FUNCTIONS;

//======================================================================
// The guts come from bison output

#ಅನ್ತರ್ಭಾವ "V3ParseBison.c"

//======================================================================
// V3ParseImp functions requiring bison state

ಪೂರ್ಣಾಙ್ಕ V3ParseImp::bisonParse() {
    // Use --debugi-bison 9 to enable this
    ಯದಿ (PARSEP->debugBison() >= 9) yydebug = 1;
    ಪ್ರತ್ಯಾಗಮ yyparse();
}

ಧ್ರುವ ವರ್ಣ* V3ParseImp::tokenName(ಪೂರ್ಣಾಙ್ಕ token) {
#ಯದಿ YYDEBUG || YYERROR_VERBOSE
    ಸ್ಥಿರ ಧ್ರುವ ವರ್ಣ** nameTablep = nullptr;
    ಯದಿ (!nameTablep) {
        ಪೂರ್ಣಾಙ್ಕ size;
        ಪ್ರತಿ (size = 0; yytname[size]; ++size) {}
        nameTablep = ನೂತನ ಧ್ರುವ ವರ್ಣ*[size];
        // Workaround bug in bison's which have '!' in yytname but not token values
        ಪೂರ್ಣಾಙ್ಕ iout = 0;
        ಪ್ರತಿ (ಪೂರ್ಣಾಙ್ಕ i = 0; yytname[i]; ++i) {
            ಯದಿ (yytname[i][0] == '\'') ಅನುವರ್ತ;
            nameTablep[iout++] = yytname[i];
        }
    }
    ಯದಿ (token >= 255) {
        ಪ್ರತ್ಯಾಗಮ nameTablep[token - 255];
    } ಅನ್ಯಥಾ {
        ಸ್ಥಿರ ವರ್ಣ s_ch[2];
        s_ch[0] = token;
        s_ch[1] = '\0';
        ಪ್ರತ್ಯಾಗಮ s_ch;
    }
#ಅನ್ಯಥಾ
    ಪ್ರತ್ಯಾಗಮ "";
#endif
}

ಶೂನ್ಯ V3ParseImp::candidatePli(VSpellCheck* spellerp) {
#ಯದಿ !YYERROR_VERBOSE
#error "Need lex token names"
#endif
    ಪ್ರತಿ (ಪೂರ್ಣಾಙ್ಕ i = 0; yytname[i]; ++i) {
        ಯದಿ (yytname[i][0] != '\"') ಅನುವರ್ತ;
        ಯದಿ (yytname[i][1] != '$') ಅನುವರ್ತ;
        spellerp->ತಳ್ಳು_Candidate(string{yytname[i]}.substr(1, strlen(yytname[i]) - 2));
    }
}

ಶೂನ್ಯ V3ParseImp::parserClear() {
    // Clear up any dynamic memory V3Parser required
    VARDTYPE(nullptr);
    GRAMMARP->setNetDelay(nullptr);
    GRAMMARP->setScopedSigAttr(nullptr);
}

//======================================================================
// V3ParseGrammar functions requiring bison state

AstArg* V3ParseGrammar::argWrapList(Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* nodep) {
    // Convert list of expressions to list of arguments
    AstArg* outp = nullptr;
    ಯಾವತ್ (nodep) {
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ nextp = VN_AS(nodep->nextp(), ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ);
        ಯದಿ (nextp) nextp->unlinkFrBackWithNext();
        outp = Ast_ಗಂಟು::addNext(outp, ನೂತನ AstArg{nodep->fileline(), "", nodep});
        nodep = nextp;
    }
    ಪ್ರತ್ಯಾಗಮ outp;
}

AstAssignW* V3ParseGrammar::create_Supply_ಅಭಿವ್ಯಕ್ತಿ(File_ಸಾಲು* fileline, ಧ್ರುವ string& name, ಪೂರ್ಣಾಙ್ಕ ಮೌಲ್ಯ) {
    AstAssignW* assignp = ನೂತನ AstAssignW{fileline, ನೂತನ AstParseRef{fileline, name},
                                         ಮೌಲ್ಯ ? ನೂತನ AstConst{fileline, AstConst::All1{}}
                                               : ನೂತನ AstConst{fileline, AstConst::All0{}}};
    AstStrengthSpec* strengthSpecp
        = ನೂತನ AstStrengthSpec{fileline, VStrength::SUPPLY, VStrength::SUPPLY};
    assignp->strengthSpecp(strengthSpecp);
    ಪ್ರತ್ಯಾಗಮ assignp;
}

AstRange* V3ParseGrammar::scrubRangeMulti(Ast_ಗಂಟು_Range* nrangep) {
    // Remove any UnsizedRange's from list; preserves multi-dim chain via nextp().
    ಪ್ರತಿ (Ast_ಗಂಟು_Range *nodep = nrangep, *nextp; nodep; nodep = nextp) {
        nextp = VN_AS(nodep->nextp(), ಗಂಟು_Range);
        ಯದಿ (!VN_IS(nodep, Range)) {
            nodep->v3error(
                "Unsupported or syntax error: Unsized range in instance or other declaration");
            nodep->unlinkFrBack();
            VL_DO_DANGLING(nodep->delete_ವೃಕ್ಷ(), nodep);
        }
    }
    ಪ್ರತ್ಯಾಗಮ VN_CAST(nrangep, Range);
}
AstRange* V3ParseGrammar::scrubRange(Ast_ಗಂಟು_Range* nrangep) {
    AstRange* ಧ್ರುವ rangep = scrubRangeMulti(nrangep);
    ಯದಿ (rangep && rangep->nextp()) {
        // Gate primitives only support a single dimension
        rangep->nextp()->v3warn(E_UNSUPPORTED, "Unsupported: Multidimensional gate instances.");
        rangep->nextp()->unlinkFrBackWithNext()->delete_ವೃಕ್ಷ();
    }
    ಪ್ರತ್ಯಾಗಮ rangep;
}

Ast_ಗಂಟು_Pre_Sel* V3ParseGrammar::scrubSel(Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* fromp, Ast_ಗಂಟು_Pre_Sel* selp) VL_MT_DISABLED {
    // SEL(PARSELVALUE, ...) -> SEL(fromp, ...)
    Ast_ಗಂಟು_Pre_Sel* subSelp = selp;
    ಯಾವತ್ (ಸತ್ಯ) {
        ಯದಿ (VN_IS(subSelp->fromp(), ParseHolder)) ಭಙ್ಗ;
        ಯದಿ (Ast_ಗಂಟು_Pre_Sel* ಧ್ರುವ lowerSelp = VN_CAST(subSelp->fromp(), ಗಂಟು_Pre_Sel)) {
            subSelp = lowerSelp;
            ಅನುವರ್ತ;
        }
        subSelp->v3fatalSrc("Couldn't find where to insert expression into select");
    }
    Ast_ಗಂಟು* subSelFromp = subSelp->fromp();
    subSelFromp->replaceWith(fromp);
    VL_DO_DANGLING(subSelFromp->delete_ವೃಕ್ಷ(), subSelFromp);
    ಪ್ರತ್ಯಾಗಮ selp;
}

Ast_ಗಂಟು_DType* V3ParseGrammar::createArray(Ast_ಗಂಟು_DType* basep, Ast_ಗಂಟು_Range* nrangep,
                                          ತಾರ್ಕಿಕ isPacked) {
    // Split RANGE0-RANGE1-RANGE2
    // into ARRAYDTYPE0(ARRAYDTYPE1(ARRAYDTYPE2(BASICTYPE3), RANGE), RANGE)
    Ast_ಗಂಟು_DType* arrayp = basep;
    ಯದಿ (nrangep) {  // Maybe no range - return unmodified base type
        ಯಾವತ್ (nrangep->nextp()) nrangep = VN_AS(nrangep->nextp(), ಗಂಟು_Range);
        ಯಾವತ್ (nrangep) {
            Ast_ಗಂಟು_Range* ಧ್ರುವ prevp = VN_AS(nrangep->backp(), ಗಂಟು_Range);
            ಯದಿ (prevp) nrangep->unlinkFrBack();
            AstRange* ಧ್ರುವ rangep = VN_CAST(nrangep, Range);
            ಯದಿ (rangep && isPacked) {
                arrayp
                    = ನೂತನ AstPackArrayDType{rangep->fileline(), VFlagChildDType{}, arrayp, rangep};
            } ಅನ್ಯಥಾ ಯದಿ (rangep
                       && (VN_IS(rangep->leftp(), Unbounded)
                           || VN_IS(rangep->rightp(), Unbounded))) {
                arrayp = ನೂತನ AstQueueDType{nrangep->fileline(), VFlagChildDType{}, arrayp,
                                           rangep->rightp()->clone_ವೃಕ್ಷ(ಸತ್ಯ)};
                VL_DO_DANGLING(nrangep->delete_ವೃಕ್ಷ(), nrangep);
            } ಅನ್ಯಥಾ ಯದಿ (rangep) {
                arrayp = ನೂತನ AstUnpackArrayDType{rangep->fileline(), VFlagChildDType{}, arrayp,
                                                 rangep};
            } ಅನ್ಯಥಾ ಯದಿ (VN_IS(nrangep, UnsizedRange)) {
                arrayp = ನೂತನ AstDynArrayDType{nrangep->fileline(), VFlagChildDType{}, arrayp};
                VL_DO_DANGLING(nrangep->delete_ವೃಕ್ಷ(), nrangep);
            } ಅನ್ಯಥಾ ಯದಿ (VN_IS(nrangep, BracketRange)) {
                ಧ್ರುವ AstBracketRange* ಧ್ರುವ arangep = VN_AS(nrangep, BracketRange);
                Ast_ಗಂಟು* ಧ್ರುವ keyp = arangep->elementsp()->unlinkFrBack();
                arrayp = ನೂತನ AstBracketArrayDType{nrangep->fileline(), VFlagChildDType{}, arrayp,
                                                  keyp};
                VL_DO_DANGLING(nrangep->delete_ವೃಕ್ಷ(), nrangep);
            } ಅನ್ಯಥಾ ಯದಿ (VN_IS(nrangep, WildcardRange)) {
                arrayp = ನೂತನ AstWildcardArrayDType{nrangep->fileline(), VFlagChildDType{}, arrayp};
                VL_DO_DANGLING(nrangep->delete_ವೃಕ್ಷ(), nrangep);
            } ಅನ್ಯಥಾ {
                UASSERT_OBJ(0, nrangep, "Expected range or unsized range");
            }
            nrangep = prevp;
        }
    }
    ಪ್ರತ್ಯಾಗಮ arrayp;
}

AstVar* V3ParseGrammar::createVariable(File_ಸಾಲು* fileline, ಧ್ರುವ string& name,
                                       Ast_ಗಂಟು_Range* arrayp, Ast_ಗಂಟು* attrsp) {
    UINFO(5, "  creVar " << name << "  decl=" << GRAMMARP->m_varDecl << "  io="
                         << GRAMMARP->m_varIO << "  dt=" << (GRAMMARP->m_varDTypep ? "set" : ""));
    ಯದಿ (GRAMMARP->m_varIO == VDirection::NONE  // In non-ANSI port list
        && GRAMMARP->m_varDecl == VVarType::PORT) {
        // Just a port list with variable name (not v2k format); AstPort already created
        ಯದಿ (arrayp) VL_DO_DANGLING(arrayp->delete_ವೃಕ್ಷ(), arrayp);
        ಯದಿ (attrsp) {
            // TODO: Merge attributes across list? Or warn attribute is ignored
            VL_DO_DANGLING(attrsp->delete_ವೃಕ್ಷ(), attrsp);
        }
        ಪ್ರತ್ಯಾಗಮ nullptr;
    }

    Ast_ಗಂಟು_DType* ಧ್ರುವ dtypep = [&]() -> Ast_ಗಂಟು_DType* {
        ಯದಿ (GRAMMARP->m_varDecl == VVarType::WREAL) {
            // dtypep might not be null, might be implicit LOGIC before we knew better
            ಪ್ರತ್ಯಾಗಮ ನೂತನ AstBasicDType{fileline, VBasicDTypeKwd::DOUBLE};
        }
        ಯದಿ (GRAMMARP->m_varDTypep) {
            // May make new variables with same type, so clone
            ಪ್ರತ್ಯಾಗಮ GRAMMARP->m_varDTypep->clone_ವೃಕ್ಷ(ಅಸತ್ಯ);
        }
        // Created implicitly
        ಯದಿ (m_insideProperty) {
            ಯದಿ (m_typedPropertyPort) {
                fileline->v3warn(E_UNSUPPORTED, "Untyped property port following a typed port");
            }
            ಪ್ರತ್ಯಾಗಮ ನೂತನ AstBasicDType{fileline, VBasicDTypeKwd::UNTYPED};
        }
        ಪ್ರತ್ಯಾಗಮ ನೂತನ AstBasicDType{fileline, LOGIC_IMPLICIT};
    }();

    // UINFO(0,"CREVAR "<<fileline->ascii()<<" decl="<<GRAMMARP->m_varDecl.ascii()<<"
    // io="<<GRAMMARP->m_varIO.ascii()<<endl);
    VVarType type = GRAMMARP->m_varDecl;
    ಯದಿ (type == VVarType::UNKNOWN) {  // e.g. "output" non-ANSI standalone direction (vs "reg")
        ಯದಿ (GRAMMARP->m_varIO.isAny()) {
            type = VVarType::PORT;
        } ಅನ್ಯಥಾ {
            fileline->v3fatalSrc("Unknown signal type declared: " << type.ascii());
        }
    }
    ಯದಿ (type == VVarType::GENVAR) {
        // Should be impossible as the grammar blocks this, but...
        ಯದಿ (arrayp) fileline->v3error("Genvars may not be arrayed: " << name);  // LCOV_EXCL_LINE
    }

    // Split RANGE0-RANGE1-RANGE2 into
    // ARRAYDTYPE0(ARRAYDTYPE1(ARRAYDTYPE2(BASICTYPE3), RANGE), RANGE)
    Ast_ಗಂಟು_DType* ಧ್ರುವ arrayDTypep = createArray(dtypep, arrayp, ಅಸತ್ಯ);

    AstVar* ಧ್ರುವ nodep = ನೂತನ AstVar{fileline, type, name, VFlagChildDType(), arrayDTypep};
    nodep->addAttrsp(attrsp);
    nodep->ansi(m_pinAnsi);
    nodep->declTyped(m_varDeclTyped);
    nodep->lifetime(m_varLifetime);
    ಯದಿ (m_netDelayp) nodep->delayp(m_netDelayp->clone_ವೃಕ್ಷ(ಅಸತ್ಯ));
    ಯದಿ (GRAMMARP->m_varDecl != VVarType::UNKNOWN) nodep->combineType(GRAMMARP->m_varDecl);
    ಯದಿ (GRAMMARP->m_varIO != VDirection::NONE) {
        nodep->declDirection(GRAMMARP->m_varIO);
        nodep->direction(GRAMMARP->m_varIO);
    }

    ಯದಿ (GRAMMARP->m_varDecl == VVarType::SUPPLY0) {
        AstAssignW* ಧ್ರುವ ap = V3ParseGrammar::create_Supply_ಅಭಿವ್ಯಕ್ತಿ(fileline, nodep->name(), 0);
        Ast_ಗಂಟು::addNext<Ast_ಗಂಟು, Ast_ಗಂಟು>(nodep, ನೂತನ AstAlways{ap});
    }
    ಯದಿ (GRAMMARP->m_varDecl == VVarType::SUPPLY1) {
        AstAssignW* ಧ್ರುವ ap = V3ParseGrammar::create_Supply_ಅಭಿವ್ಯಕ್ತಿ(fileline, nodep->name(), 1);
        Ast_ಗಂಟು::addNext<Ast_ಗಂಟು, Ast_ಗಂಟು>(nodep, ನೂತನ AstAlways{ap});
    }
    ಯದಿ (VN_IS(dtypep, ParseTypeDType)) {
        // Parser needs to know what is a type
        Ast_ಗಂಟು* ಧ್ರುವ newp = ನೂತನ AstTypedefFwd{fileline, name, VFwdType::NONE};
        Ast_ಗಂಟು::addNext<Ast_ಗಂಟು, Ast_ಗಂಟು>(nodep, newp);
    }
    // Don't set dtypep in the ranging;
    // We need to autosize parameters and integers separately
    //
    // Propagate from current module tracing state
    ಯದಿ (nodep->isGenVar()) {
        nodep->trace(ಅಸತ್ಯ);
    } ಅನ್ಯಥಾ ಯದಿ (nodep->isParam() && !v3Global.opt.traceParams()) {
        nodep->trace(ಅಸತ್ಯ);
    } ಅನ್ಯಥಾ {
        nodep->trace(allTracingOn(nodep->fileline()));
    }
    ಯದಿ (nodep->varType().isVPIAccessible()) nodep->addAttrsp(GRAMMARP->cloneScopedSigAttr());

    // Remember the last variable created, so we can attach attributes to it in later parsing
    GRAMMARP->m_varAttrp = nodep;
    PARSEP->tagNodep(GRAMMARP->m_varAttrp);
    ಪ್ರತ್ಯಾಗಮ nodep;
}

string V3ParseGrammar::unquoteString(File_ಸಾಲು* fileline, ಧ್ರುವ std::string& text) {
    string err_ಸಂದೇಶ;
    string res = VString::unquoteSVString(text, err_ಸಂದೇಶ);
    ಯದಿ (!err_ಸಂದೇಶ.empty()) fileline->v3error(err_ಸಂದೇಶ.c_str());
    ಪ್ರತ್ಯಾಗಮ res;
}

ಪೂರ್ಣಾಙ್ಕ V3ParseGrammar::s_type_Imp_ಅಂಕೆ = 0;
