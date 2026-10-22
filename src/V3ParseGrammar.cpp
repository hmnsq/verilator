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
    जइ (PARSEP->debugBison() >= 9) yydebug = 1;
    प्रत्यागम yyparse();
}

ध्रुव वर्ण* V3ParseImp::tokenName(पूर्णाङ्क token) {
#जइ YYDEBUG || YYERROR_VERBOSE
    स्थिर ध्रुव वर्ण** nameTablep = nullptr;
    जइ (!nameTablep) {
        पूर्णाङ्क size;
        प्रति (size = 0; yytname[size]; ++size) {}
        nameTablep = नूतन ध्रुव वर्ण*[size];
        // Workaround bug in bison's which have '!' in yytname but not token values
        पूर्णाङ्क iout = 0;
        प्रति (पूर्णाङ्क i = 0; yytname[i]; ++i) {
            जइ (yytname[i][0] == '\'') अनुवर्त;
            nameTablep[iout++] = yytname[i];
        }
    }
    जइ (token >= 255) {
        प्रत्यागम nameTablep[token - 255];
    } अण्णहा {
        स्थिर वर्ण s_ch[2];
        s_ch[0] = token;
        s_ch[1] = '\0';
        प्रत्यागम s_ch;
    }
#अण्णहा
    प्रत्यागम "";
#endif
}

शून्य V3ParseImp::candidatePli(VSpellCheck* spellerp) {
#जइ !YYERROR_VERBOSE
#error "Need lex token names"
#endif
    प्रति (पूर्णाङ्क i = 0; yytname[i]; ++i) {
        जइ (yytname[i][0] != '\"') अनुवर्त;
        जइ (yytname[i][1] != '$') अनुवर्त;
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
    जाव (nodep) {
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव nextp = VN_AS(nodep->nextp(), ग्रन्थिः_व्यञ्जकम्);
        जइ (nextp) nextp->unlinkFrBackWithNext();
        outp = Ast_ग्रन्थिः::addNext(outp, नूतन AstArg{nodep->fileline(), "", nodep});
        nodep = nextp;
    }
    प्रत्यागम outp;
}

AstAssignW* V3ParseGrammar::create_Supply_व्यञ्जकम्(File_पङ्क्तिः* fileline, ध्रुव string& name, पूर्णाङ्क मानम्) {
    AstAssignW* assignp = नूतन AstAssignW{fileline, नूतन AstParseRef{fileline, name},
                                         मानम् ? नूतन AstConst{fileline, AstConst::All1{}}
                                               : नूतन AstConst{fileline, AstConst::All0{}}};
    AstStrengthSpec* strengthSpecp
        = नूतन AstStrengthSpec{fileline, VStrength::SUPPLY, VStrength::SUPPLY};
    assignp->strengthSpecp(strengthSpecp);
    प्रत्यागम assignp;
}

AstRange* V3ParseGrammar::scrubRangeMulti(Ast_ग्रन्थिः_Range* nrangep) {
    // Remove any UnsizedRange's from list; preserves multi-dim chain via nextp().
    प्रति (Ast_ग्रन्थिः_Range *nodep = nrangep, *nextp; nodep; nodep = nextp) {
        nextp = VN_AS(nodep->nextp(), ग्रन्थिः_Range);
        जइ (!VN_IS(nodep, Range)) {
            nodep->v3error(
                "Unsupported or syntax error: Unsized range in instance or other declaration");
            nodep->unlinkFrBack();
            VL_DO_DANGLING(nodep->delete_वृक्षः(), nodep);
        }
    }
    प्रत्यागम VN_CAST(nrangep, Range);
}
AstRange* V3ParseGrammar::scrubRange(Ast_ग्रन्थिः_Range* nrangep) {
    AstRange* ध्रुव rangep = scrubRangeMulti(nrangep);
    जइ (rangep && rangep->nextp()) {
        // Gate primitives only support a single dimension
        rangep->nextp()->v3warn(E_UNSUPPORTED, "Unsupported: Multidimensional gate instances.");
        rangep->nextp()->unlinkFrBackWithNext()->delete_वृक्षः();
    }
    प्रत्यागम rangep;
}

Ast_ग्रन्थिः_Pre_Sel* V3ParseGrammar::scrubSel(Ast_ग्रन्थिः_व्यञ्जकम्* fromp, Ast_ग्रन्थिः_Pre_Sel* selp) VL_MT_DISABLED {
    // SEL(PARSELVALUE, ...) -> SEL(fromp, ...)
    Ast_ग्रन्थिः_Pre_Sel* subSelp = selp;
    जाव (सच्च) {
        जइ (VN_IS(subSelp->fromp(), ParseHolder)) भङ्ग;
        जइ (Ast_ग्रन्थिः_Pre_Sel* ध्रुव lowerSelp = VN_CAST(subSelp->fromp(), ग्रन्थिः_Pre_Sel)) {
            subSelp = lowerSelp;
            अनुवर्त;
        }
        subSelp->v3fatalSrc("Couldn't find where to insert expression into select");
    }
    Ast_ग्रन्थिः* subSelFromp = subSelp->fromp();
    subSelFromp->replaceWith(fromp);
    VL_DO_DANGLING(subSelFromp->delete_वृक्षः(), subSelFromp);
    प्रत्यागम selp;
}

Ast_ग्रन्थिः_DType* V3ParseGrammar::createArray(Ast_ग्रन्थिः_DType* basep, Ast_ग्रन्थिः_Range* nrangep,
                                          तार्किक isPacked) {
    // Split RANGE0-RANGE1-RANGE2
    // into ARRAYDTYPE0(ARRAYDTYPE1(ARRAYDTYPE2(BASICTYPE3), RANGE), RANGE)
    Ast_ग्रन्थिः_DType* arrayp = basep;
    जइ (nrangep) {  // Maybe no range - return unmodified base type
        जाव (nrangep->nextp()) nrangep = VN_AS(nrangep->nextp(), ग्रन्थिः_Range);
        जाव (nrangep) {
            Ast_ग्रन्थिः_Range* ध्रुव prevp = VN_AS(nrangep->backp(), ग्रन्थिः_Range);
            जइ (prevp) nrangep->unlinkFrBack();
            AstRange* ध्रुव rangep = VN_CAST(nrangep, Range);
            जइ (rangep && isPacked) {
                arrayp
                    = नूतन AstPackArrayDType{rangep->fileline(), VFlagChildDType{}, arrayp, rangep};
            } अण्णहा जइ (rangep
                       && (VN_IS(rangep->leftp(), Unbounded)
                           || VN_IS(rangep->rightp(), Unbounded))) {
                arrayp = नूतन AstQueueDType{nrangep->fileline(), VFlagChildDType{}, arrayp,
                                           rangep->rightp()->clone_वृक्षः(सच्च)};
                VL_DO_DANGLING(nrangep->delete_वृक्षः(), nrangep);
            } अण्णहा जइ (rangep) {
                arrayp = नूतन AstUnpackArrayDType{rangep->fileline(), VFlagChildDType{}, arrayp,
                                                 rangep};
            } अण्णहा जइ (VN_IS(nrangep, UnsizedRange)) {
                arrayp = नूतन AstDynArrayDType{nrangep->fileline(), VFlagChildDType{}, arrayp};
                VL_DO_DANGLING(nrangep->delete_वृक्षः(), nrangep);
            } अण्णहा जइ (VN_IS(nrangep, BracketRange)) {
                ध्रुव AstBracketRange* ध्रुव arangep = VN_AS(nrangep, BracketRange);
                Ast_ग्रन्थिः* ध्रुव keyp = arangep->elementsp()->unlinkFrBack();
                arrayp = नूतन AstBracketArrayDType{nrangep->fileline(), VFlagChildDType{}, arrayp,
                                                  keyp};
                VL_DO_DANGLING(nrangep->delete_वृक्षः(), nrangep);
            } अण्णहा जइ (VN_IS(nrangep, WildcardRange)) {
                arrayp = नूतन AstWildcardArrayDType{nrangep->fileline(), VFlagChildDType{}, arrayp};
                VL_DO_DANGLING(nrangep->delete_वृक्षः(), nrangep);
            } अण्णहा {
                UASSERT_OBJ(0, nrangep, "Expected range or unsized range");
            }
            nrangep = prevp;
        }
    }
    प्रत्यागम arrayp;
}

AstVar* V3ParseGrammar::createVariable(File_पङ्क्तिः* fileline, ध्रुव string& name,
                                       Ast_ग्रन्थिः_Range* arrayp, Ast_ग्रन्थिः* attrsp) {
    UINFO(5, "  creVar " << name << "  decl=" << GRAMMARP->m_varDecl << "  io="
                         << GRAMMARP->m_varIO << "  dt=" << (GRAMMARP->m_varDTypep ? "set" : ""));
    जइ (GRAMMARP->m_varIO == VDirection::NONE  // In non-ANSI port list
        && GRAMMARP->m_varDecl == VVarType::PORT) {
        // Just a port list with variable name (not v2k format); AstPort already created
        जइ (arrayp) VL_DO_DANGLING(arrayp->delete_वृक्षः(), arrayp);
        जइ (attrsp) {
            // TODO: Merge attributes across list? Or warn attribute is ignored
            VL_DO_DANGLING(attrsp->delete_वृक्षः(), attrsp);
        }
        प्रत्यागम nullptr;
    }

    Ast_ग्रन्थिः_DType* ध्रुव dtypep = [&]() -> Ast_ग्रन्थिः_DType* {
        जइ (GRAMMARP->m_varDecl == VVarType::WREAL) {
            // dtypep might not be null, might be implicit LOGIC before we knew better
            प्रत्यागम नूतन AstBasicDType{fileline, VBasicDTypeKwd::DOUBLE};
        }
        जइ (GRAMMARP->m_varDTypep) {
            // May make new variables with same type, so clone
            प्रत्यागम GRAMMARP->m_varDTypep->clone_वृक्षः(मिच्छ);
        }
        // Created implicitly
        जइ (m_insideProperty) {
            जइ (m_typedPropertyPort) {
                fileline->v3warn(E_UNSUPPORTED, "Untyped property port following a typed port");
            }
            प्रत्यागम नूतन AstBasicDType{fileline, VBasicDTypeKwd::UNTYPED};
        }
        प्रत्यागम नूतन AstBasicDType{fileline, LOGIC_IMPLICIT};
    }();

    // UINFO(0,"CREVAR "<<fileline->ascii()<<" decl="<<GRAMMARP->m_varDecl.ascii()<<"
    // io="<<GRAMMARP->m_varIO.ascii()<<endl);
    VVarType type = GRAMMARP->m_varDecl;
    जइ (type == VVarType::UNKNOWN) {  // e.g. "output" non-ANSI standalone direction (vs "reg")
        जइ (GRAMMARP->m_varIO.isAny()) {
            type = VVarType::PORT;
        } अण्णहा {
            fileline->v3fatalSrc("Unknown signal type declared: " << type.ascii());
        }
    }
    जइ (type == VVarType::GENVAR) {
        // Should be impossible as the grammar blocks this, but...
        जइ (arrayp) fileline->v3error("Genvars may not be arrayed: " << name);  // LCOV_EXCL_LINE
    }

    // Split RANGE0-RANGE1-RANGE2 into
    // ARRAYDTYPE0(ARRAYDTYPE1(ARRAYDTYPE2(BASICTYPE3), RANGE), RANGE)
    Ast_ग्रन्थिः_DType* ध्रुव arrayDTypep = createArray(dtypep, arrayp, मिच्छ);

    AstVar* ध्रुव nodep = नूतन AstVar{fileline, type, name, VFlagChildDType(), arrayDTypep};
    nodep->addAttrsp(attrsp);
    nodep->ansi(m_pinAnsi);
    nodep->declTyped(m_varDeclTyped);
    nodep->lifetime(m_varLifetime);
    जइ (m_netDelayp) nodep->delayp(m_netDelayp->clone_वृक्षः(मिच्छ));
    जइ (GRAMMARP->m_varDecl != VVarType::UNKNOWN) nodep->combineType(GRAMMARP->m_varDecl);
    जइ (GRAMMARP->m_varIO != VDirection::NONE) {
        nodep->declDirection(GRAMMARP->m_varIO);
        nodep->direction(GRAMMARP->m_varIO);
    }

    जइ (GRAMMARP->m_varDecl == VVarType::SUPPLY0) {
        AstAssignW* ध्रुव ap = V3ParseGrammar::create_Supply_व्यञ्जकम्(fileline, nodep->name(), 0);
        Ast_ग्रन्थिः::addNext<Ast_ग्रन्थिः, Ast_ग्रन्थिः>(nodep, नूतन AstAlways{ap});
    }
    जइ (GRAMMARP->m_varDecl == VVarType::SUPPLY1) {
        AstAssignW* ध्रुव ap = V3ParseGrammar::create_Supply_व्यञ्जकम्(fileline, nodep->name(), 1);
        Ast_ग्रन्थिः::addNext<Ast_ग्रन्थिः, Ast_ग्रन्थिः>(nodep, नूतन AstAlways{ap});
    }
    जइ (VN_IS(dtypep, ParseTypeDType)) {
        // Parser needs to know what is a type
        Ast_ग्रन्थिः* ध्रुव newp = नूतन AstTypedefFwd{fileline, name, VFwdType::NONE};
        Ast_ग्रन्थिः::addNext<Ast_ग्रन्थिः, Ast_ग्रन्थिः>(nodep, newp);
    }
    // Don't set dtypep in the ranging;
    // We need to autosize parameters and integers separately
    //
    // Propagate from current module tracing state
    जइ (nodep->isGenVar()) {
        nodep->trace(मिच्छ);
    } अण्णहा जइ (nodep->isParam() && !v3Global.opt.traceParams()) {
        nodep->trace(मिच्छ);
    } अण्णहा {
        nodep->trace(allTracingOn(nodep->fileline()));
    }
    जइ (nodep->varType().isVPIAccessible()) nodep->addAttrsp(GRAMMARP->cloneScopedSigAttr());

    // Remember the last variable created, so we can attach attributes to it in later parsing
    GRAMMARP->m_varAttrp = nodep;
    PARSEP->tagNodep(GRAMMARP->m_varAttrp);
    प्रत्यागम nodep;
}

string V3ParseGrammar::unquoteString(File_पङ्क्तिः* fileline, ध्रुव std::string& text) {
    string err_सन्देशः;
    string res = VString::unquoteSVString(text, err_सन्देशः);
    जइ (!err_सन्देशः.empty()) fileline->v3error(err_सन्देशः.c_str());
    प्रत्यागम res;
}

पूर्णाङ्क V3ParseGrammar::s_type_Imp_अङ्कः = 0;
