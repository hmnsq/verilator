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

#घोषणा YYDEBUG 1  // Nicer errors

#घोषणा VL_MT_DISABLED_CODE_UNIT 1

#समावेश "V3Ast.h"  // This must be before V3ParseBison.cpp, as we don't want #defines to conflict

VL_DEFINE_DEBUG_FUNCTIONS;

//======================================================================
// The guts come from bison output

#समावेश "V3ParseBison.c"

//======================================================================
// V3ParseImp functions requiring bison state

पूर्णांक V3ParseImp::bisonParse() {
    // Use --debugi-bison 9 to enable this
    अगर (PARSEP->debugBison() >= 9) yydebug = 1;
    वापस yyparse();
}

स्थिर अक्षर* V3ParseImp::tokenName(पूर्णांक token) {
#अगर YYDEBUG || YYERROR_VERBOSE
    अटल स्थिर अक्षर** nameTablep = nullptr;
    अगर (!nameTablep) {
        पूर्णांक आकार;
        क्रम (आकार = 0; yytname[आकार]; ++आकार) {}
        nameTablep = नया स्थिर अक्षर*[आकार];
        // Workaround bug in bison's which have '!' in yytname but not token values
        पूर्णांक iout = 0;
        क्रम (पूर्णांक i = 0; yytname[i]; ++i) {
            अगर (yytname[i][0] == '\'') जारी;
            nameTablep[iout++] = yytname[i];
        }
    }
    अगर (token >= 255) {
        वापस nameTablep[token - 255];
    } अन्यथा {
        अटल अक्षर s_ch[2];
        s_ch[0] = token;
        s_ch[1] = '\0';
        वापस s_ch;
    }
#अन्यथा
    वापस "";
#पूर्ण_अगर
}

व्योम V3ParseImp::candidatePli(VSpellCheck* spellerp) {
#अगर !YYERROR_VERBOSE
#त्रुटि "Need lex token names"
#पूर्ण_अगर
    क्रम (पूर्णांक i = 0; yytname[i]; ++i) {
        अगर (yytname[i][0] != '\"') जारी;
        अगर (yytname[i][1] != '$') जारी;
        spellerp->धकेलो_Candidate(string{yytname[i]}.substr(1, म_माप(yytname[i]) - 2));
    }
}

व्योम V3ParseImp::parserClear() {
    // Clear up any dynamic memory V3Parser required
    VARDTYPE(nullptr);
    GRAMMARP->setNetDelay(nullptr);
    GRAMMARP->setScopedSigAttr(nullptr);
}

//======================================================================
// V3ParseGrammar functions requiring bison state

AstArg* V3ParseGrammar::argWrapList(Ast_गाँठ_व्यंजक* nodep) {
    // Convert list of expressions to list of arguments
    AstArg* outp = nullptr;
    जबतक (nodep) {
        Ast_गाँठ_व्यंजक* स्थिर nextp = VN_AS(nodep->nextp(), गाँठ_व्यंजक);
        अगर (nextp) nextp->unlink_Fr_Back_With_अग्र();
        outp = Ast_गाँठ::add_अग्र(outp, नया AstArg{nodep->fileline(), "", nodep});
        nodep = nextp;
    }
    वापस outp;
}

AstAssignW* V3ParseGrammar::create_Supply_व्यंजक(File_पंक्ति* fileline, स्थिर string& name, पूर्णांक मूल्य) {
    AstAssignW* assignp = नया AstAssignW{fileline, नया AstParseRef{fileline, name},
                                         मूल्य ? नया AstConst{fileline, AstConst::All1{}}
                                               : नया AstConst{fileline, AstConst::All0{}}};
    AstStrengthSpec* strengthSpecp
        = नया AstStrengthSpec{fileline, VStrength::SUPPLY, VStrength::SUPPLY};
    assignp->strengthSpecp(strengthSpecp);
    वापस assignp;
}

AstRange* V3ParseGrammar::scrubRangeMulti(Ast_गाँठ_Range* nrangep) {
    // Remove any UnsizedRange's from list; preserves multi-dim chain via nextp().
    क्रम (Ast_गाँठ_Range *nodep = nrangep, *nextp; nodep; nodep = nextp) {
        nextp = VN_AS(nodep->nextp(), गाँठ_Range);
        अगर (!VN_IS(nodep, Range)) {
            nodep->v3error(
                "Unsupported or syntax error: Unsized range in instance or other declaration");
            nodep->unlinkFrBack();
            VL_DO_DANGLING(nodep->delete_वृक्ष(), nodep);
        }
    }
    वापस VN_CAST(nrangep, Range);
}
AstRange* V3ParseGrammar::scrubRange(Ast_गाँठ_Range* nrangep) {
    AstRange* स्थिर rangep = scrubRangeMulti(nrangep);
    अगर (rangep && rangep->nextp()) {
        // Gate primitives only support a single dimension
        rangep->nextp()->v3warn(E_UNSUPPORTED, "Unsupported: Multidimensional gate instances.");
        rangep->nextp()->unlink_Fr_Back_With_अग्र()->delete_वृक्ष();
    }
    वापस rangep;
}

Ast_गाँठ_Pre_Sel* V3ParseGrammar::scrubSel(Ast_गाँठ_व्यंजक* fromp, Ast_गाँठ_Pre_Sel* selp) VL_MT_DISABLED {
    // SEL(PARSELVALUE, ...) -> SEL(fromp, ...)
    Ast_गाँठ_Pre_Sel* subSelp = selp;
    जबतक (true) {
        अगर (VN_IS(subSelp->fromp(), ParseHolder)) अवरोध;
        अगर (Ast_गाँठ_Pre_Sel* स्थिर lowerSelp = VN_CAST(subSelp->fromp(), गाँठ_Pre_Sel)) {
            subSelp = lowerSelp;
            जारी;
        }
        subSelp->v3fatalSrc("Couldn't find where to insert expression into select");
    }
    Ast_गाँठ* subSelFromp = subSelp->fromp();
    subSelFromp->replaceWith(fromp);
    VL_DO_DANGLING(subSelFromp->delete_वृक्ष(), subSelFromp);
    वापस selp;
}

Ast_गाँठ_DType* V3ParseGrammar::createArray(Ast_गाँठ_DType* basep, Ast_गाँठ_Range* nrangep,
                                          bool isPacked) {
    // Split RANGE0-RANGE1-RANGE2
    // into ARRAYDTYPE0(ARRAYDTYPE1(ARRAYDTYPE2(BASICTYPE3), RANGE), RANGE)
    Ast_गाँठ_DType* arrayp = basep;
    अगर (nrangep) {  // Maybe no range - return unmodified base type
        जबतक (nrangep->nextp()) nrangep = VN_AS(nrangep->nextp(), गाँठ_Range);
        जबतक (nrangep) {
            Ast_गाँठ_Range* स्थिर prevp = VN_AS(nrangep->backp(), गाँठ_Range);
            अगर (prevp) nrangep->unlinkFrBack();
            AstRange* स्थिर rangep = VN_CAST(nrangep, Range);
            अगर (rangep && isPacked) {
                arrayp
                    = नया AstPackArrayDType{rangep->fileline(), VFlagChildDType{}, arrayp, rangep};
            } अन्यथा अगर (rangep
                       && (VN_IS(rangep->leftp(), Unbounded)
                           || VN_IS(rangep->rightp(), Unbounded))) {
                arrayp = नया AstQueueDType{nrangep->fileline(), VFlagChildDType{}, arrayp,
                                           rangep->rightp()->clone_वृक्ष(true)};
                VL_DO_DANGLING(nrangep->delete_वृक्ष(), nrangep);
            } अन्यथा अगर (rangep) {
                arrayp = नया AstUnpackArrayDType{rangep->fileline(), VFlagChildDType{}, arrayp,
                                                 rangep};
            } अन्यथा अगर (VN_IS(nrangep, UnsizedRange)) {
                arrayp = नया AstDynArrayDType{nrangep->fileline(), VFlagChildDType{}, arrayp};
                VL_DO_DANGLING(nrangep->delete_वृक्ष(), nrangep);
            } अन्यथा अगर (VN_IS(nrangep, BracketRange)) {
                स्थिर AstBracketRange* स्थिर arangep = VN_AS(nrangep, BracketRange);
                Ast_गाँठ* स्थिर keyp = arangep->elementsp()->unlinkFrBack();
                arrayp = नया AstBracketArrayDType{nrangep->fileline(), VFlagChildDType{}, arrayp,
                                                  keyp};
                VL_DO_DANGLING(nrangep->delete_वृक्ष(), nrangep);
            } अन्यथा अगर (VN_IS(nrangep, WildcardRange)) {
                arrayp = नया AstWildcardArrayDType{nrangep->fileline(), VFlagChildDType{}, arrayp};
                VL_DO_DANGLING(nrangep->delete_वृक्ष(), nrangep);
            } अन्यथा {
                UASSERT_OBJ(0, nrangep, "Expected range or unsized range");
            }
            nrangep = prevp;
        }
    }
    वापस arrayp;
}

AstVar* V3ParseGrammar::createVariable(File_पंक्ति* fileline, स्थिर string& name,
                                       Ast_गाँठ_Range* arrayp, Ast_गाँठ* attrsp) {
    UINFO(5, "  creVar " << name << "  decl=" << GRAMMARP->m_varDecl << "  io="
                         << GRAMMARP->m_varIO << "  dt=" << (GRAMMARP->m_varDTypep ? "set" : ""));
    अगर (GRAMMARP->m_varIO == VDirection::NONE  // In non-ANSI port list
        && GRAMMARP->m_varDecl == VVarType::PORT) {
        // Just a port list with variable name (not v2k format); AstPort already created
        अगर (arrayp) VL_DO_DANGLING(arrayp->delete_वृक्ष(), arrayp);
        अगर (attrsp) {
            // TODO: Merge attributes across list? Or warn attribute is ignored
            VL_DO_DANGLING(attrsp->delete_वृक्ष(), attrsp);
        }
        वापस nullptr;
    }

    Ast_गाँठ_DType* स्थिर dtypep = [&]() -> Ast_गाँठ_DType* {
        अगर (GRAMMARP->m_varDecl == VVarType::WREAL) {
            // dtypep might not be null, might be implicit LOGIC before we knew better
            वापस नया AstBasicDType{fileline, VBasicDTypeKwd::DOUBLE};
        }
        अगर (GRAMMARP->m_varDTypep) {
            // May make new variables with same type, so clone
            वापस GRAMMARP->m_varDTypep->clone_वृक्ष(false);
        }
        // Created implicitly
        अगर (m_insideProperty) {
            अगर (m_typedPropertyPort) {
                fileline->v3warn(E_UNSUPPORTED, "Untyped property port following a typed port");
            }
            वापस नया AstBasicDType{fileline, VBasicDTypeKwd::UNTYPED};
        }
        वापस नया AstBasicDType{fileline, LOGIC_IMPLICIT};
    }();

    // UINFO(0,"CREVAR "<<fileline->ascii()<<" decl="<<GRAMMARP->m_varDecl.ascii()<<"
    // io="<<GRAMMARP->m_varIO.ascii()<<endl);
    VVarType type = GRAMMARP->m_varDecl;
    अगर (type == VVarType::UNKNOWN) {  // e.g. "output" non-ANSI standalone direction (vs "reg")
        अगर (GRAMMARP->m_varIO.isAny()) {
            type = VVarType::PORT;
        } अन्यथा {
            fileline->v3fatalSrc("Unknown signal type declared: " << type.ascii());
        }
    }
    अगर (type == VVarType::GENVAR) {
        // Should be impossible as the grammar blocks this, but...
        अगर (arrayp) fileline->v3error("Genvars may not be arrayed: " << name);  // LCOV_EXCL_LINE
    }

    // Split RANGE0-RANGE1-RANGE2 into
    // ARRAYDTYPE0(ARRAYDTYPE1(ARRAYDTYPE2(BASICTYPE3), RANGE), RANGE)
    Ast_गाँठ_DType* स्थिर arrayDTypep = createArray(dtypep, arrayp, false);

    AstVar* स्थिर nodep = नया AstVar{fileline, type, name, VFlagChildDType(), arrayDTypep};
    nodep->addAttrsp(attrsp);
    nodep->ansi(m_pinAnsi);
    nodep->declTyped(m_varDeclTyped);
    nodep->lifetime(m_varLifetime);
    अगर (m_netDelayp) nodep->delayp(m_netDelayp->clone_वृक्ष(false));
    अगर (GRAMMARP->m_varDecl != VVarType::UNKNOWN) nodep->combineType(GRAMMARP->m_varDecl);
    अगर (GRAMMARP->m_varIO != VDirection::NONE) {
        nodep->declDirection(GRAMMARP->m_varIO);
        nodep->direction(GRAMMARP->m_varIO);
    }

    अगर (GRAMMARP->m_varDecl == VVarType::SUPPLY0) {
        AstAssignW* स्थिर ap = V3ParseGrammar::create_Supply_व्यंजक(fileline, nodep->name(), 0);
        Ast_गाँठ::add_अग्र<Ast_गाँठ, Ast_गाँठ>(nodep, नया AstAlways{ap});
    }
    अगर (GRAMMARP->m_varDecl == VVarType::SUPPLY1) {
        AstAssignW* स्थिर ap = V3ParseGrammar::create_Supply_व्यंजक(fileline, nodep->name(), 1);
        Ast_गाँठ::add_अग्र<Ast_गाँठ, Ast_गाँठ>(nodep, नया AstAlways{ap});
    }
    अगर (VN_IS(dtypep, ParseTypeDType)) {
        // Parser needs to know what is a type
        Ast_गाँठ* स्थिर newp = नया AstTypedefFwd{fileline, name, VFwdType::NONE};
        Ast_गाँठ::add_अग्र<Ast_गाँठ, Ast_गाँठ>(nodep, newp);
    }
    // Don't set dtypep in the ranging;
    // We need to autosize parameters and integers separately
    //
    // Propagate from current module tracing state
    अगर (nodep->isGenVar()) {
        nodep->trace(false);
    } अन्यथा अगर (nodep->isParam() && !v3Global.opt.traceParams()) {
        nodep->trace(false);
    } अन्यथा {
        nodep->trace(allTracingOn(nodep->fileline()));
    }
    अगर (nodep->varType().isVPIAccessible()) nodep->addAttrsp(GRAMMARP->cloneScopedSigAttr());

    // Remember the last variable created, so we can attach attributes to it in later parsing
    GRAMMARP->m_varAttrp = nodep;
    PARSEP->tagNodep(GRAMMARP->m_varAttrp);
    वापस nodep;
}

string V3ParseGrammar::unquoteString(File_पंक्ति* fileline, स्थिर std::string& text) {
    string err_संदेश;
    string res = VString::unquoteSVString(text, err_संदेश);
    अगर (!err_संदेश.empty()) fileline->v3error(err_संदेश.c_str());
    वापस res;
}

पूर्णांक V3ParseGrammar::s_type_Imp_अंक = 0;
