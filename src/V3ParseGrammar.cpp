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

#अन्तर्भाव "V3Ast.h"  // This must be before V3ParseBison.cpp, as we don't want #defines to conflict

VL_DEFINE_DEBUG_FUNCTIONS;

//======================================================================
// The guts come from bison output

#अन्तर्भाव "V3ParseBison.c"

//======================================================================
// V3ParseImp functions requiring bison state

पूर्णाङ्क V3ParseImp::bisonParse() {
    // Use --debugi-bison 9 to enable this
    सचे (PARSEP->debugBison() >= 9) yydebug = 1;
    पटिनिवत्त yyparse();
}

ध्रुव वर्ण* V3ParseImp::tokenName(पूर्णाङ्क token) {
#सचे YYDEBUG || YYERROR_VERBOSE
    स्थिर ध्रुव वर्ण** nameTablep = nullptr;
    सचे (!nameTablep) {
        पूर्णाङ्क size;
        अत्थाय (size = 0; yytname[size]; ++size) {}
        nameTablep = नूतन ध्रुव वर्ण*[size];
        // Workaround bug in bison's which have '!' in yytname but not token values
        पूर्णाङ्क iout = 0;
        अत्थाय (पूर्णाङ्क i = 0; yytname[i]; ++i) {
            सचे (yytname[i][0] == '\'') अनुवर्त;
            nameTablep[iout++] = yytname[i];
        }
    }
    सचे (token >= 255) {
        पटिनिवत्त nameTablep[token - 255];
    } अञ्ञथा {
        स्थिर वर्ण s_ch[2];
        s_ch[0] = token;
        s_ch[1] = '\0';
        पटिनिवत्त s_ch;
    }
#अञ्ञथा
    पटिनिवत्त "";
#endif
}

शून्य V3ParseImp::candidatePli(VSpellCheck* spellerp) {
#सचे !YYERROR_VERBOSE
#error "Need lex token names"
#endif
    अत्थाय (पूर्णाङ्क i = 0; yytname[i]; ++i) {
        सचे (yytname[i][0] != '\"') अनुवर्त;
        सचे (yytname[i][1] != '$') अनुवर्त;
        spellerp->प्रक्षिप_Candidate(string{yytname[i]}.substr(1, strlen(yytname[i]) - 2));
    }
}

शून्य V3ParseImp::parserClear() {
    // Clear up any dynamic memory V3Parser required
    VARDTYPE(nullptr);
    GRAMMARP->setNetDelay(nullptr);
    GRAMMARP->setScopedSigAttr(nullptr);
}

//======================================================================
// V3ParseGrammar functions requiring bison state

AstArg* V3ParseGrammar::argWrapList(Ast_ग्रन्थिः_व्यञ्जकम्* nodep) {
    // Convert list of expressions to list of arguments
    AstArg* outp = nullptr;
    याव (nodep) {
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव nextp = VN_AS(nodep->nextp(), ग्रन्थिः_व्यञ्जकम्);
        सचे (nextp) nextp->unlinkFrBackWithNext();
        outp = Ast_ग्रन्थिः::addNext(outp, नूतन AstArg{nodep->fileline(), "", nodep});
        nodep = nextp;
    }
    पटिनिवत्त outp;
}

AstAssignW* V3ParseGrammar::create_Supply_व्यञ्जकम्(File_पङ्क्तिः* fileline, ध्रुव string& name, पूर्णाङ्क मानम्) {
    AstAssignW* assignp = नूतन AstAssignW{fileline, नूतन AstParseRef{fileline, name},
                                         मानम् ? नूतन AstConst{fileline, AstConst::All1{}}
                                               : नूतन AstConst{fileline, AstConst::All0{}}};
    AstStrengthSpec* strengthSpecp
        = नूतन AstStrengthSpec{fileline, VStrength::SUPPLY, VStrength::SUPPLY};
    assignp->strengthSpecp(strengthSpecp);
    पटिनिवत्त assignp;
}

AstRange* V3ParseGrammar::scrubRangeMulti(Ast_ग्रन्थिः_Range* nrangep) {
    // Remove any UnsizedRange's from list; preserves multi-dim chain via nextp().
    अत्थाय (Ast_ग्रन्थिः_Range *nodep = nrangep, *nextp; nodep; nodep = nextp) {
        nextp = VN_AS(nodep->nextp(), ग्रन्थिः_Range);
        सचे (!VN_IS(nodep, Range)) {
            nodep->v3error(
                "Unsupported or syntax error: Unsized range in instance or other declaration");
            nodep->unlinkFrBack();
            VL_DO_DANGLING(nodep->delete_वृक्षः(), nodep);
        }
    }
    पटिनिवत्त VN_CAST(nrangep, Range);
}
AstRange* V3ParseGrammar::scrubRange(Ast_ग्रन्थिः_Range* nrangep) {
    AstRange* ध्रुव rangep = scrubRangeMulti(nrangep);
    सचे (rangep && rangep->nextp()) {
        // Gate primitives only support a single dimension
        rangep->nextp()->v3warn(E_UNSUPPORTED, "Unsupported: Multidimensional gate instances.");
        rangep->nextp()->unlinkFrBackWithNext()->delete_वृक्षः();
    }
    पटिनिवत्त rangep;
}

Ast_ग्रन्थिः_Pre_Sel* V3ParseGrammar::scrubSel(Ast_ग्रन्थिः_व्यञ्जकम्* fromp, Ast_ग्रन्थिः_Pre_Sel* selp) VL_MT_DISABLED {
    // SEL(PARSELVALUE, ...) -> SEL(fromp, ...)
    Ast_ग्रन्थिः_Pre_Sel* subSelp = selp;
    याव (सच्च) {
        सचे (VN_IS(subSelp->fromp(), ParseHolder)) भङ्ग;
        सचे (Ast_ग्रन्थिः_Pre_Sel* ध्रुव lowerSelp = VN_CAST(subSelp->fromp(), ग्रन्थिः_Pre_Sel)) {
            subSelp = lowerSelp;
            अनुवर्त;
        }
        subSelp->v3fatalSrc("Couldn't find where to insert expression into select");
    }
    Ast_ग्रन्थिः* subSelFromp = subSelp->fromp();
    subSelFromp->replaceWith(fromp);
    VL_DO_DANGLING(subSelFromp->delete_वृक्षः(), subSelFromp);
    पटिनिवत्त selp;
}

Ast_ग्रन्थिः_DType* V3ParseGrammar::createArray(Ast_ग्रन्थिः_DType* basep, Ast_ग्रन्थिः_Range* nrangep,
                                          तार्किक isPacked) {
    // Split RANGE0-RANGE1-RANGE2
    // into ARRAYDTYPE0(ARRAYDTYPE1(ARRAYDTYPE2(BASICTYPE3), RANGE), RANGE)
    Ast_ग्रन्थिः_DType* arrayp = basep;
    सचे (nrangep) {  // Maybe no range - return unmodified base type
        याव (nrangep->nextp()) nrangep = VN_AS(nrangep->nextp(), ग्रन्थिः_Range);
        याव (nrangep) {
            Ast_ग्रन्थिः_Range* ध्रुव prevp = VN_AS(nrangep->backp(), ग्रन्थिः_Range);
            सचे (prevp) nrangep->unlinkFrBack();
            AstRange* ध्रुव rangep = VN_CAST(nrangep, Range);
            सचे (rangep && isPacked) {
                arrayp
                    = नूतन AstPackArrayDType{rangep->fileline(), VFlagChildDType{}, arrayp, rangep};
            } अञ्ञथा सचे (rangep
                       && (VN_IS(rangep->leftp(), Unbounded)
                           || VN_IS(rangep->rightp(), Unbounded))) {
                arrayp = नूतन AstQueueDType{nrangep->fileline(), VFlagChildDType{}, arrayp,
                                           rangep->rightp()->clone_वृक्षः(सच्च)};
                VL_DO_DANGLING(nrangep->delete_वृक्षः(), nrangep);
            } अञ्ञथा सचे (rangep) {
                arrayp = नूतन AstUnpackArrayDType{rangep->fileline(), VFlagChildDType{}, arrayp,
                                                 rangep};
            } अञ्ञथा सचे (VN_IS(nrangep, UnsizedRange)) {
                arrayp = नूतन AstDynArrayDType{nrangep->fileline(), VFlagChildDType{}, arrayp};
                VL_DO_DANGLING(nrangep->delete_वृक्षः(), nrangep);
            } अञ्ञथा सचे (VN_IS(nrangep, BracketRange)) {
                ध्रुव AstBracketRange* ध्रुव arangep = VN_AS(nrangep, BracketRange);
                Ast_ग्रन्थिः* ध्रुव keyp = arangep->elementsp()->unlinkFrBack();
                arrayp = नूतन AstBracketArrayDType{nrangep->fileline(), VFlagChildDType{}, arrayp,
                                                  keyp};
                VL_DO_DANGLING(nrangep->delete_वृक्षः(), nrangep);
            } अञ्ञथा सचे (VN_IS(nrangep, WildcardRange)) {
                arrayp = नूतन AstWildcardArrayDType{nrangep->fileline(), VFlagChildDType{}, arrayp};
                VL_DO_DANGLING(nrangep->delete_वृक्षः(), nrangep);
            } अञ्ञथा {
                UASSERT_OBJ(0, nrangep, "Expected range or unsized range");
            }
            nrangep = prevp;
        }
    }
    पटिनिवत्त arrayp;
}

AstVar* V3ParseGrammar::createVariable(File_पङ्क्तिः* fileline, ध्रुव string& name,
                                       Ast_ग्रन्थिः_Range* arrayp, Ast_ग्रन्थिः* attrsp) {
    UINFO(5, "  creVar " << name << "  decl=" << GRAMMARP->m_varDecl << "  io="
                         << GRAMMARP->m_varIO << "  dt=" << (GRAMMARP->m_varDTypep ? "set" : ""));
    सचे (GRAMMARP->m_varIO == VDirection::NONE  // In non-ANSI port list
        && GRAMMARP->m_varDecl == VVarType::PORT) {
        // Just a port list with variable name (not v2k format); AstPort already created
        सचे (arrayp) VL_DO_DANGLING(arrayp->delete_वृक्षः(), arrayp);
        सचे (attrsp) {
            // TODO: Merge attributes across list? Or warn attribute is ignored
            VL_DO_DANGLING(attrsp->delete_वृक्षः(), attrsp);
        }
        पटिनिवत्त nullptr;
    }

    Ast_ग्रन्थिः_DType* ध्रुव dtypep = [&]() -> Ast_ग्रन्थिः_DType* {
        सचे (GRAMMARP->m_varDecl == VVarType::WREAL) {
            // dtypep might not be null, might be implicit LOGIC before we knew better
            पटिनिवत्त नूतन AstBasicDType{fileline, VBasicDTypeKwd::DOUBLE};
        }
        सचे (GRAMMARP->m_varDTypep) {
            // May make new variables with same type, so clone
            पटिनिवत्त GRAMMARP->m_varDTypep->clone_वृक्षः(मिच्छा);
        }
        // Created implicitly
        सचे (m_insideProperty) {
            सचे (m_typedPropertyPort) {
                fileline->v3warn(E_UNSUPPORTED, "Untyped property port following a typed port");
            }
            पटिनिवत्त नूतन AstBasicDType{fileline, VBasicDTypeKwd::UNTYPED};
        }
        पटिनिवत्त नूतन AstBasicDType{fileline, LOGIC_IMPLICIT};
    }();

    // UINFO(0,"CREVAR "<<fileline->ascii()<<" decl="<<GRAMMARP->m_varDecl.ascii()<<"
    // io="<<GRAMMARP->m_varIO.ascii()<<endl);
    VVarType type = GRAMMARP->m_varDecl;
    सचे (type == VVarType::UNKNOWN) {  // e.g. "output" non-ANSI standalone direction (vs "reg")
        सचे (GRAMMARP->m_varIO.isAny()) {
            type = VVarType::PORT;
        } अञ्ञथा {
            fileline->v3fatalSrc("Unknown signal type declared: " << type.ascii());
        }
    }
    सचे (type == VVarType::GENVAR) {
        // Should be impossible as the grammar blocks this, but...
        सचे (arrayp) fileline->v3error("Genvars may not be arrayed: " << name);  // LCOV_EXCL_LINE
    }

    // Split RANGE0-RANGE1-RANGE2 into
    // ARRAYDTYPE0(ARRAYDTYPE1(ARRAYDTYPE2(BASICTYPE3), RANGE), RANGE)
    Ast_ग्रन्थिः_DType* ध्रुव arrayDTypep = createArray(dtypep, arrayp, मिच्छा);

    AstVar* ध्रुव nodep = नूतन AstVar{fileline, type, name, VFlagChildDType(), arrayDTypep};
    nodep->addAttrsp(attrsp);
    nodep->ansi(m_pinAnsi);
    nodep->declTyped(m_varDeclTyped);
    nodep->lifetime(m_varLifetime);
    सचे (m_netDelayp) nodep->delayp(m_netDelayp->clone_वृक्षः(मिच्छा));
    सचे (GRAMMARP->m_varDecl != VVarType::UNKNOWN) nodep->combineType(GRAMMARP->m_varDecl);
    सचे (GRAMMARP->m_varIO != VDirection::NONE) {
        nodep->declDirection(GRAMMARP->m_varIO);
        nodep->direction(GRAMMARP->m_varIO);
    }

    सचे (GRAMMARP->m_varDecl == VVarType::SUPPLY0) {
        AstAssignW* ध्रुव ap = V3ParseGrammar::create_Supply_व्यञ्जकम्(fileline, nodep->name(), 0);
        Ast_ग्रन्थिः::addNext<Ast_ग्रन्थिः, Ast_ग्रन्थिः>(nodep, नूतन AstAlways{ap});
    }
    सचे (GRAMMARP->m_varDecl == VVarType::SUPPLY1) {
        AstAssignW* ध्रुव ap = V3ParseGrammar::create_Supply_व्यञ्जकम्(fileline, nodep->name(), 1);
        Ast_ग्रन्थिः::addNext<Ast_ग्रन्थिः, Ast_ग्रन्थिः>(nodep, नूतन AstAlways{ap});
    }
    सचे (VN_IS(dtypep, ParseTypeDType)) {
        // Parser needs to know what is a type
        Ast_ग्रन्थिः* ध्रुव newp = नूतन AstTypedefFwd{fileline, name, VFwdType::NONE};
        Ast_ग्रन्थिः::addNext<Ast_ग्रन्थिः, Ast_ग्रन्थिः>(nodep, newp);
    }
    // Don't set dtypep in the ranging;
    // We need to autosize parameters and integers separately
    //
    // Propagate from current module tracing state
    सचे (nodep->isGenVar()) {
        nodep->trace(मिच्छा);
    } अञ्ञथा सचे (nodep->isParam() && !v3Global.opt.traceParams()) {
        nodep->trace(मिच्छा);
    } अञ्ञथा {
        nodep->trace(allTracingOn(nodep->fileline()));
    }
    सचे (nodep->varType().isVPIAccessible()) nodep->addAttrsp(GRAMMARP->cloneScopedSigAttr());

    // Remember the last variable created, so we can attach attributes to it in later parsing
    GRAMMARP->m_varAttrp = nodep;
    PARSEP->tagNodep(GRAMMARP->m_varAttrp);
    पटिनिवत्त nodep;
}

string V3ParseGrammar::unquoteString(File_पङ्क्तिः* fileline, ध्रुव std::string& text) {
    string err_सन्देशः;
    string res = VString::unquoteSVString(text, err_सन्देशः);
    सचे (!err_सन्देशः.empty()) fileline->v3error(err_सन्देशः.c_str());
    पटिनिवत्त res;
}

पूर्णाङ्क V3ParseGrammar::s_type_Imp_अङ्कः = 0;
