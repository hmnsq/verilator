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

#నిర్వచించు YYDEBUG 1  // Nicer errors

#నిర్వచించు VL_MT_DISABLED_సంకేతం_UNIT 1

#అన్తర్భావ "V3Ast.h"  // This must be before V3ParseBison.cpp, as we don't want #defines to conflict

VL_DEFINE_DEBUG_FUNCTIONS;

//======================================================================
// The guts come from bison output

#అన్తర్భావ "V3ParseBison.c"

//======================================================================
// V3ParseImp functions requiring bison state

పూర్ణాంకం V3ParseImp::bisonParse() {
    // Use --debugi-bison 9 to enable this
    ఒకవేళ (PARSEP->debugBison() >= 9) yydebug = 1;
    తిరిగి yyparse();
}

స్థిరరాశి అక్షరం* V3ParseImp::tokenName(పూర్ణాంకం token) {
#ఒకవేళ YYDEBUG || YYERROR_VERBOSE
    స్థిర స్థిరరాశి అక్షరం** nameTablep = శూన్యసూచిక;
    ఒకవేళ (!nameTablep) {
        పూర్ణాంకం కొలత;
        కొరకు (కొలత = 0; yytname[కొలత]; ++కొలత) {}
        nameTablep = కొత్త స్థిరరాశి అక్షరం*[కొలత];
        // Workaround bug in bison's which have '!' in yytname but not token values
        పూర్ణాంకం iout = 0;
        కొరకు (పూర్ణాంకం i = 0; yytname[i]; ++i) {
            ఒకవేళ (yytname[i][0] == '\'') కొనసాగు;
            nameTablep[iout++] = yytname[i];
        }
    }
    ఒకవేళ (token >= 255) {
        తిరిగి nameTablep[token - 255];
    } లేకపోతే {
        స్థిర అక్షరం s_ch[2];
        s_ch[0] = token;
        s_ch[1] = '\0';
        తిరిగి s_ch;
    }
#లేకపోతే
    తిరిగి "";
#అయితే_ముగింపు
}

శూన్య V3ParseImp::candidatePli(VSpellCheck* spellerp) {
#ఒకవేళ !YYERROR_VERBOSE
#దోషం "Need lex token names"
#అయితే_ముగింపు
    కొరకు (పూర్ణాంకం i = 0; yytname[i]; ++i) {
        ఒకవేళ (yytname[i][0] != '\"') కొనసాగు;
        ఒకవేళ (yytname[i][1] != '$') కొనసాగు;
        spellerp->నెట్టు_Candidate(string{yytname[i]}.substr(1, అక్షరశ్రేణి_పొడవు(yytname[i]) - 2));
    }
}

శూన్య V3ParseImp::parserClear() {
    // Clear up any dynamic memory V3Parser required
    VARDTYPE(శూన్యసూచిక);
    GRAMMARP->సమితి_Net_Delay(శూన్యసూచిక);
    GRAMMARP->సమితి_Scoped_Sig_Attr(శూన్యసూచిక);
}

//======================================================================
// V3ParseGrammar functions requiring bison state

AstArg* V3ParseGrammar::argWrapList(Ast_కణుపు_సమాసం* nodep) {
    // Convert list of expressions to list of arguments
    AstArg* outp = శూన్యసూచిక;
    అయితే (nodep) {
        Ast_కణుపు_సమాసం* స్థిరరాశి nextp = VN_AS(nodep->nextp(), కణుపు_సమాసం);
        ఒకవేళ (nextp) nextp->unlink_Fr_వెనుక_With_తదుపరి();
        outp = Ast_కణుపు::కూడు_తదుపరి(outp, కొత్త AstArg{nodep->fileline(), "", nodep});
        nodep = nextp;
    }
    తిరిగి outp;
}

Ast_కేటాయించు_W* V3ParseGrammar::create_Supply_సమాసం(File_పంక్తి* fileline, స్థిరరాశి string& name, పూర్ణాంకం విలువ) {
    Ast_కేటాయించు_W* assignp = కొత్త Ast_కేటాయించు_W{fileline, కొత్త AstParseRef{fileline, name},
                                         విలువ ? కొత్త AstConst{fileline, AstConst::All1{}}
                                               : కొత్త AstConst{fileline, AstConst::All0{}}};
    AstStrengthSpec* strengthSpecp
        = కొత్త AstStrengthSpec{fileline, VStrength::SUPPLY, VStrength::SUPPLY};
    assignp->strengthSpecp(strengthSpecp);
    తిరిగి assignp;
}

AstRange* V3ParseGrammar::scrubRangeMulti(Ast_కణుపు_Range* nrangep) {
    // Remove any UnsizedRange's from list; preserves multi-dim chain via nextp().
    కొరకు (Ast_కణుపు_Range *nodep = nrangep, *nextp; nodep; nodep = nextp) {
        nextp = VN_AS(nodep->nextp(), కణుపు_Range);
        ఒకవేళ (!VN_IS(nodep, Range)) {
            nodep->v3error(
                "Unsupported or syntax error: Unsized range in instance or other declaration");
            nodep->unlink_Fr_వెనుక();
            VL_DO_DANGLING(nodep->delete_వృక్షం(), nodep);
        }
    }
    తిరిగి VN_CAST(nrangep, Range);
}
AstRange* V3ParseGrammar::scrubRange(Ast_కణుపు_Range* nrangep) {
    AstRange* స్థిరరాశి rangep = scrubRangeMulti(nrangep);
    ఒకవేళ (rangep && rangep->nextp()) {
        // Gate primitives only support a single dimension
        rangep->nextp()->v3warn(E_UNSUPPORTED, "Unsupported: Multidimensional gate instances.");
        rangep->nextp()->unlink_Fr_వెనుక_With_తదుపరి()->delete_వృక్షం();
    }
    తిరిగి rangep;
}

Ast_కణుపు_Pre_Sel* V3ParseGrammar::scrubSel(Ast_కణుపు_సమాసం* fromp, Ast_కణుపు_Pre_Sel* selp) VL_MT_DISABLED {
    // SEL(PARSELVALUE, ...) -> SEL(fromp, ...)
    Ast_కణుపు_Pre_Sel* subSelp = selp;
    అయితే (నిజం) {
        ఒకవేళ (VN_IS(subSelp->fromp(), ParseHolder)) ఆపు;
        ఒకవేళ (Ast_కణుపు_Pre_Sel* స్థిరరాశి క్రింది_Selp = VN_CAST(subSelp->fromp(), కణుపు_Pre_Sel)) {
            subSelp = క్రింది_Selp;
            కొనసాగు;
        }
        subSelp->v3fatalSrc("Couldn't find where to insert expression into select");
    }
    Ast_కణుపు* subSelFromp = subSelp->fromp();
    subSelFromp->replaceWith(fromp);
    VL_DO_DANGLING(subSelFromp->delete_వృక్షం(), subSelFromp);
    తిరిగి selp;
}

Ast_కణుపు_DType* V3ParseGrammar::createArray(Ast_కణుపు_DType* basep, Ast_కణుపు_Range* nrangep,
                                          తార్కిక isPacked) {
    // Split RANGE0-RANGE1-RANGE2
    // into ARRAYDTYPE0(ARRAYDTYPE1(ARRAYDTYPE2(BASICTYPE3), RANGE), RANGE)
    Ast_కణుపు_DType* arrayp = basep;
    ఒకవేళ (nrangep) {  // Maybe no range - return unmodified base type
        అయితే (nrangep->nextp()) nrangep = VN_AS(nrangep->nextp(), కణుపు_Range);
        అయితే (nrangep) {
            Ast_కణుపు_Range* స్థిరరాశి prevp = VN_AS(nrangep->backp(), కణుపు_Range);
            ఒకవేళ (prevp) nrangep->unlink_Fr_వెనుక();
            AstRange* స్థిరరాశి rangep = VN_CAST(nrangep, Range);
            ఒకవేళ (rangep && isPacked) {
                arrayp
                    = కొత్త AstPackArrayDType{rangep->fileline(), VFlag_బిడ్డ_DType{}, arrayp, rangep};
            } లేకపోతే ఒకవేళ (rangep
                       && (VN_IS(rangep->leftp(), Unbounded)
                           || VN_IS(rangep->rightp(), Unbounded))) {
                arrayp = కొత్త Ast_వరుస_DType{nrangep->fileline(), VFlag_బిడ్డ_DType{}, arrayp,
                                           rangep->rightp()->clone_వృక్షం(నిజం)};
                VL_DO_DANGLING(nrangep->delete_వృక్షం(), nrangep);
            } లేకపోతే ఒకవేళ (rangep) {
                arrayp = కొత్త AstUnpackArrayDType{rangep->fileline(), VFlag_బిడ్డ_DType{}, arrayp,
                                                 rangep};
            } లేకపోతే ఒకవేళ (VN_IS(nrangep, UnsizedRange)) {
                arrayp = కొత్త AstDynArrayDType{nrangep->fileline(), VFlag_బిడ్డ_DType{}, arrayp};
                VL_DO_DANGLING(nrangep->delete_వృక్షం(), nrangep);
            } లేకపోతే ఒకవేళ (VN_IS(nrangep, BracketRange)) {
                స్థిరరాశి AstBracketRange* స్థిరరాశి arangep = VN_AS(nrangep, BracketRange);
                Ast_కణుపు* స్థిరరాశి keyp = arangep->elementsp()->unlink_Fr_వెనుక();
                arrayp = కొత్త AstBracketArrayDType{nrangep->fileline(), VFlag_బిడ్డ_DType{}, arrayp,
                                                  keyp};
                VL_DO_DANGLING(nrangep->delete_వృక్షం(), nrangep);
            } లేకపోతే ఒకవేళ (VN_IS(nrangep, WildcardRange)) {
                arrayp = కొత్త AstWildcardArrayDType{nrangep->fileline(), VFlag_బిడ్డ_DType{}, arrayp};
                VL_DO_DANGLING(nrangep->delete_వృక్షం(), nrangep);
            } లేకపోతే {
                UASSERT_OBJ(0, nrangep, "Expected range or unsized range");
            }
            nrangep = prevp;
        }
    }
    తిరిగి arrayp;
}

AstVar* V3ParseGrammar::createVariable(File_పంక్తి* fileline, స్థిరరాశి string& name,
                                       Ast_కణుపు_Range* arrayp, Ast_కణుపు* attrsp) {
    UINFO(5, "  creVar " << name << "  decl=" << GRAMMARP->m_varDecl << "  io="
                         << GRAMMARP->m_varIO << "  dt=" << (GRAMMARP->m_varDTypep ? "set" : ""));
    ఒకవేళ (GRAMMARP->m_varIO == VDirection::NONE  // In non-ANSI port list
        && GRAMMARP->m_varDecl == VVarType::పోర్టు) {
        // Just a port list with variable name (not v2k format); AstPort already created
        ఒకవేళ (arrayp) VL_DO_DANGLING(arrayp->delete_వృక్షం(), arrayp);
        ఒకవేళ (attrsp) {
            // TODO: Merge attributes across list? Or warn attribute is ignored
            VL_DO_DANGLING(attrsp->delete_వృక్షం(), attrsp);
        }
        తిరిగి శూన్యసూచిక;
    }

    Ast_కణుపు_DType* స్థిరరాశి dtypep = [&]() -> Ast_కణుపు_DType* {
        ఒకవేళ (GRAMMARP->m_varDecl == VVarType::WREAL) {
            // dtypep might not be null, might be implicit LOGIC before we knew better
            తిరిగి కొత్త AstBasicDType{fileline, VBasicDTypeKwd::DOUBLE};
        }
        ఒకవేళ (GRAMMARP->m_varDTypep) {
            // May make new variables with same type, so clone
            తిరిగి GRAMMARP->m_varDTypep->clone_వృక్షం(అబద్ధం);
        }
        // Created implicitly
        ఒకవేళ (m_insideProperty) {
            ఒకవేళ (m_typed_Property_పోర్టు) {
                fileline->v3warn(E_UNSUPPORTED, "Untyped property port following a typed port");
            }
            తిరిగి కొత్త AstBasicDType{fileline, VBasicDTypeKwd::UNTYPED};
        }
        తిరిగి కొత్త AstBasicDType{fileline, LOGIC_IMPLICIT};
    }();

    // UINFO(0,"CREVAR "<<fileline->ascii()<<" decl="<<GRAMMARP->m_varDecl.ascii()<<"
    // io="<<GRAMMARP->m_varIO.ascii()<<endl);
    VVarType type = GRAMMARP->m_varDecl;
    ఒకవేళ (type == VVarType::UNKNOWN) {  // e.g. "output" non-ANSI standalone direction (vs "reg")
        ఒకవేళ (GRAMMARP->m_varIO.isAny()) {
            type = VVarType::పోర్టు;
        } లేకపోతే {
            fileline->v3fatalSrc("Unknown signal type declared: " << type.ascii());
        }
    }
    ఒకవేళ (type == VVarType::GENVAR) {
        // Should be impossible as the grammar blocks this, but...
        ఒకవేళ (arrayp) fileline->v3error("Genvars may not be arrayed: " << name);  // LCOV_EXCL_LINE
    }

    // Split RANGE0-RANGE1-RANGE2 into
    // ARRAYDTYPE0(ARRAYDTYPE1(ARRAYDTYPE2(BASICTYPE3), RANGE), RANGE)
    Ast_కణుపు_DType* స్థిరరాశి arrayDTypep = createArray(dtypep, arrayp, అబద్ధం);

    AstVar* స్థిరరాశి nodep = కొత్త AstVar{fileline, type, name, VFlag_బిడ్డ_DType(), arrayDTypep};
    nodep->కూడు_Attrsp(attrsp);
    nodep->ansi(m_pinAnsi);
    nodep->declTyped(m_varDeclTyped);
    nodep->lifetime(m_varLifetime);
    ఒకవేళ (m_netDelayp) nodep->delayp(m_netDelayp->clone_వృక్షం(అబద్ధం));
    ఒకవేళ (GRAMMARP->m_varDecl != VVarType::UNKNOWN) nodep->combineType(GRAMMARP->m_varDecl);
    ఒకవేళ (GRAMMARP->m_varIO != VDirection::NONE) {
        nodep->declDirection(GRAMMARP->m_varIO);
        nodep->direction(GRAMMARP->m_varIO);
    }

    ఒకవేళ (GRAMMARP->m_varDecl == VVarType::SUPPLY0) {
        Ast_కేటాయించు_W* స్థిరరాశి ap = V3ParseGrammar::create_Supply_సమాసం(fileline, nodep->name(), 0);
        Ast_కణుపు::కూడు_తదుపరి<Ast_కణుపు, Ast_కణుపు>(nodep, కొత్త AstAlways{ap});
    }
    ఒకవేళ (GRAMMARP->m_varDecl == VVarType::SUPPLY1) {
        Ast_కేటాయించు_W* స్థిరరాశి ap = V3ParseGrammar::create_Supply_సమాసం(fileline, nodep->name(), 1);
        Ast_కణుపు::కూడు_తదుపరి<Ast_కణుపు, Ast_కణుపు>(nodep, కొత్త AstAlways{ap});
    }
    ఒకవేళ (VN_IS(dtypep, ParseTypeDType)) {
        // Parser needs to know what is a type
        Ast_కణుపు* స్థిరరాశి newp = కొత్త AstTypedefFwd{fileline, name, VFwdType::NONE};
        Ast_కణుపు::కూడు_తదుపరి<Ast_కణుపు, Ast_కణుపు>(nodep, newp);
    }
    // Don't set dtypep in the ranging;
    // We need to autosize parameters and integers separately
    //
    // Propagate from current module tracing state
    ఒకవేళ (nodep->isGenVar()) {
        nodep->trace(అబద్ధం);
    } లేకపోతే ఒకవేళ (nodep->isParam() && !v3Global.opt.traceParams()) {
        nodep->trace(అబద్ధం);
    } లేకపోతే {
        nodep->trace(allTracingOn(nodep->fileline()));
    }
    ఒకవేళ (nodep->varType().isVPIAccessible()) nodep->కూడు_Attrsp(GRAMMARP->cloneScopedSigAttr());

    // Remember the last variable created, so we can attach attributes to it in later parsing
    GRAMMARP->m_varAttrp = nodep;
    PARSEP->ట్యాగ్_Nodep(GRAMMARP->m_varAttrp);
    తిరిగి nodep;
}

string V3ParseGrammar::unquoteString(File_పంక్తి* fileline, స్థిరరాశి std::string& పాఠ్యం) {
    string err_సందేశం;
    string res = VString::unquoteSVString(పాఠ్యం, err_సందేశం);
    ఒకవేళ (!err_సందేశం.ఖాళీలు()) fileline->v3error(err_సందేశం.c_str());
    తిరిగి res;
}

పూర్ణాంకం V3ParseGrammar::s_type_Imp_అంకె = 0;
