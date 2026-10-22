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
    यदि (PARSEP->debugBison() >= 9) yydebug = 1;
    फर्काऊ yyparse();
}

अचर वर्ण* V3ParseImp::tokenName(पूर्णाङ्क token) {
#यदि YYDEBUG || YYERROR_VERBOSE
    स्थिर अचर वर्ण** nameTablep = nullptr;
    यदि (!nameTablep) {
        पूर्णाङ्क size;
        लागि (size = 0; yytname[size]; ++size) {}
        nameTablep = नूतन अचर वर्ण*[size];
        // Workaround bug in bison's which have '!' in yytname but not token values
        पूर्णाङ्क iout = 0;
        लागि (पूर्णाङ्क i = 0; yytname[i]; ++i) {
            यदि (yytname[i][0] == '\'') जारी;
            nameTablep[iout++] = yytname[i];
        }
    }
    यदि (token >= 255) {
        फर्काऊ nameTablep[token - 255];
    } अन्यथा {
        स्थिर वर्ण s_ch[2];
        s_ch[0] = token;
        s_ch[1] = '\0';
        फर्काऊ s_ch;
    }
#अन्यथा
    फर्काऊ "";
#endif
}

शून्य V3ParseImp::candidatePli(VSpellCheck* spellerp) {
#यदि !YYERROR_VERBOSE
#error "Need lex token names"
#endif
    लागि (पूर्णाङ्क i = 0; yytname[i]; ++i) {
        यदि (yytname[i][0] != '\"') जारी;
        यदि (yytname[i][1] != '$') जारी;
        spellerp->थप_Candidate(string{yytname[i]}.substr(1, म_माप(yytname[i]) - 2));
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

AstArg* V3ParseGrammar::argWrapList(Ast_गाँठो_अभिव्यक्ति* nodep) {
    // Convert list of expressions to list of arguments
    AstArg* outp = nullptr;
    जबसम्म (nodep) {
        Ast_गाँठो_अभिव्यक्ति* अचर nextp = VN_AS(nodep->nextp(), गाँठो_अभिव्यक्ति);
        यदि (nextp) nextp->unlinkFrBackWithNext();
        outp = Ast_गाँठो::addNext(outp, नूतन AstArg{nodep->fileline(), "", nodep});
        nodep = nextp;
    }
    फर्काऊ outp;
}

AstAssignW* V3ParseGrammar::create_Supply_अभिव्यक्ति(File_पङ्क्ति* fileline, अचर string& name, पूर्णाङ्क मान) {
    AstAssignW* assignp = नूतन AstAssignW{fileline, नूतन AstParseRef{fileline, name},
                                         मान ? नूतन AstConst{fileline, AstConst::All1{}}
                                               : नूतन AstConst{fileline, AstConst::All0{}}};
    AstStrengthSpec* strengthSpecp
        = नूतन AstStrengthSpec{fileline, VStrength::SUPPLY, VStrength::SUPPLY};
    assignp->strengthSpecp(strengthSpecp);
    फर्काऊ assignp;
}

AstRange* V3ParseGrammar::scrubRangeMulti(Ast_गाँठो_Range* nrangep) {
    // Remove any UnsizedRange's from list; preserves multi-dim chain via nextp().
    लागि (Ast_गाँठो_Range *nodep = nrangep, *nextp; nodep; nodep = nextp) {
        nextp = VN_AS(nodep->nextp(), गाँठो_Range);
        यदि (!VN_IS(nodep, Range)) {
            nodep->v3error(
                "Unsupported or syntax error: Unsized range in instance or other declaration");
            nodep->unlinkFrBack();
            VL_DO_DANGLING(nodep->delete_रूख(), nodep);
        }
    }
    फर्काऊ VN_CAST(nrangep, Range);
}
AstRange* V3ParseGrammar::scrubRange(Ast_गाँठो_Range* nrangep) {
    AstRange* अचर rangep = scrubRangeMulti(nrangep);
    यदि (rangep && rangep->nextp()) {
        // Gate primitives only support a single dimension
        rangep->nextp()->v3warn(E_UNSUPPORTED, "Unsupported: Multidimensional gate instances.");
        rangep->nextp()->unlinkFrBackWithNext()->delete_रूख();
    }
    फर्काऊ rangep;
}

Ast_गाँठो_Pre_Sel* V3ParseGrammar::scrubSel(Ast_गाँठो_अभिव्यक्ति* fromp, Ast_गाँठो_Pre_Sel* selp) VL_MT_DISABLED {
    // SEL(PARSELVALUE, ...) -> SEL(fromp, ...)
    Ast_गाँठो_Pre_Sel* subSelp = selp;
    जबसम्म (सत्य) {
        यदि (VN_IS(subSelp->fromp(), ParseHolder)) रोक;
        यदि (Ast_गाँठो_Pre_Sel* अचर lowerSelp = VN_CAST(subSelp->fromp(), गाँठो_Pre_Sel)) {
            subSelp = lowerSelp;
            जारी;
        }
        subSelp->v3fatalSrc("Couldn't find where to insert expression into select");
    }
    Ast_गाँठो* subSelFromp = subSelp->fromp();
    subSelFromp->replaceWith(fromp);
    VL_DO_DANGLING(subSelFromp->delete_रूख(), subSelFromp);
    फर्काऊ selp;
}

Ast_गाँठो_DType* V3ParseGrammar::createArray(Ast_गाँठो_DType* basep, Ast_गाँठो_Range* nrangep,
                                          तार्किक isPacked) {
    // Split RANGE0-RANGE1-RANGE2
    // into ARRAYDTYPE0(ARRAYDTYPE1(ARRAYDTYPE2(BASICTYPE3), RANGE), RANGE)
    Ast_गाँठो_DType* arrayp = basep;
    यदि (nrangep) {  // Maybe no range - return unmodified base type
        जबसम्म (nrangep->nextp()) nrangep = VN_AS(nrangep->nextp(), गाँठो_Range);
        जबसम्म (nrangep) {
            Ast_गाँठो_Range* अचर prevp = VN_AS(nrangep->backp(), गाँठो_Range);
            यदि (prevp) nrangep->unlinkFrBack();
            AstRange* अचर rangep = VN_CAST(nrangep, Range);
            यदि (rangep && isPacked) {
                arrayp
                    = नूतन AstPackArrayDType{rangep->fileline(), VFlagChildDType{}, arrayp, rangep};
            } अन्यथा यदि (rangep
                       && (VN_IS(rangep->leftp(), Unbounded)
                           || VN_IS(rangep->rightp(), Unbounded))) {
                arrayp = नूतन AstQueueDType{nrangep->fileline(), VFlagChildDType{}, arrayp,
                                           rangep->rightp()->clone_रूख(सत्य)};
                VL_DO_DANGLING(nrangep->delete_रूख(), nrangep);
            } अन्यथा यदि (rangep) {
                arrayp = नूतन AstUnpackArrayDType{rangep->fileline(), VFlagChildDType{}, arrayp,
                                                 rangep};
            } अन्यथा यदि (VN_IS(nrangep, UnsizedRange)) {
                arrayp = नूतन AstDynArrayDType{nrangep->fileline(), VFlagChildDType{}, arrayp};
                VL_DO_DANGLING(nrangep->delete_रूख(), nrangep);
            } अन्यथा यदि (VN_IS(nrangep, BracketRange)) {
                अचर AstBracketRange* अचर arangep = VN_AS(nrangep, BracketRange);
                Ast_गाँठो* अचर keyp = arangep->elementsp()->unlinkFrBack();
                arrayp = नूतन AstBracketArrayDType{nrangep->fileline(), VFlagChildDType{}, arrayp,
                                                  keyp};
                VL_DO_DANGLING(nrangep->delete_रूख(), nrangep);
            } अन्यथा यदि (VN_IS(nrangep, WildcardRange)) {
                arrayp = नूतन AstWildcardArrayDType{nrangep->fileline(), VFlagChildDType{}, arrayp};
                VL_DO_DANGLING(nrangep->delete_रूख(), nrangep);
            } अन्यथा {
                UASSERT_OBJ(0, nrangep, "Expected range or unsized range");
            }
            nrangep = prevp;
        }
    }
    फर्काऊ arrayp;
}

AstVar* V3ParseGrammar::createVariable(File_पङ्क्ति* fileline, अचर string& name,
                                       Ast_गाँठो_Range* arrayp, Ast_गाँठो* attrsp) {
    UINFO(5, "  creVar " << name << "  decl=" << GRAMMARP->m_varDecl << "  io="
                         << GRAMMARP->m_varIO << "  dt=" << (GRAMMARP->m_varDTypep ? "set" : ""));
    यदि (GRAMMARP->m_varIO == VDirection::NONE  // In non-ANSI port list
        && GRAMMARP->m_varDecl == VVarType::PORT) {
        // Just a port list with variable name (not v2k format); AstPort already created
        यदि (arrayp) VL_DO_DANGLING(arrayp->delete_रूख(), arrayp);
        यदि (attrsp) {
            // TODO: Merge attributes across list? Or warn attribute is ignored
            VL_DO_DANGLING(attrsp->delete_रूख(), attrsp);
        }
        फर्काऊ nullptr;
    }

    Ast_गाँठो_DType* अचर dtypep = [&]() -> Ast_गाँठो_DType* {
        यदि (GRAMMARP->m_varDecl == VVarType::WREAL) {
            // dtypep might not be null, might be implicit LOGIC before we knew better
            फर्काऊ नूतन AstBasicDType{fileline, VBasicDTypeKwd::DOUBLE};
        }
        यदि (GRAMMARP->m_varDTypep) {
            // May make new variables with same type, so clone
            फर्काऊ GRAMMARP->m_varDTypep->clone_रूख(असत्य);
        }
        // Created implicitly
        यदि (m_insideProperty) {
            यदि (m_typedPropertyPort) {
                fileline->v3warn(E_UNSUPPORTED, "Untyped property port following a typed port");
            }
            फर्काऊ नूतन AstBasicDType{fileline, VBasicDTypeKwd::UNTYPED};
        }
        फर्काऊ नूतन AstBasicDType{fileline, LOGIC_IMPLICIT};
    }();

    // UINFO(0,"CREVAR "<<fileline->ascii()<<" decl="<<GRAMMARP->m_varDecl.ascii()<<"
    // io="<<GRAMMARP->m_varIO.ascii()<<endl);
    VVarType type = GRAMMARP->m_varDecl;
    यदि (type == VVarType::UNKNOWN) {  // e.g. "output" non-ANSI standalone direction (vs "reg")
        यदि (GRAMMARP->m_varIO.isAny()) {
            type = VVarType::PORT;
        } अन्यथा {
            fileline->v3fatalSrc("Unknown signal type declared: " << type.ascii());
        }
    }
    यदि (type == VVarType::GENVAR) {
        // Should be impossible as the grammar blocks this, but...
        यदि (arrayp) fileline->v3error("Genvars may not be arrayed: " << name);  // LCOV_EXCL_LINE
    }

    // Split RANGE0-RANGE1-RANGE2 into
    // ARRAYDTYPE0(ARRAYDTYPE1(ARRAYDTYPE2(BASICTYPE3), RANGE), RANGE)
    Ast_गाँठो_DType* अचर arrayDTypep = createArray(dtypep, arrayp, असत्य);

    AstVar* अचर nodep = नूतन AstVar{fileline, type, name, VFlagChildDType(), arrayDTypep};
    nodep->addAttrsp(attrsp);
    nodep->ansi(m_pinAnsi);
    nodep->declTyped(m_varDeclTyped);
    nodep->lifetime(m_varLifetime);
    यदि (m_netDelayp) nodep->delayp(m_netDelayp->clone_रूख(असत्य));
    यदि (GRAMMARP->m_varDecl != VVarType::UNKNOWN) nodep->combineType(GRAMMARP->m_varDecl);
    यदि (GRAMMARP->m_varIO != VDirection::NONE) {
        nodep->declDirection(GRAMMARP->m_varIO);
        nodep->direction(GRAMMARP->m_varIO);
    }

    यदि (GRAMMARP->m_varDecl == VVarType::SUPPLY0) {
        AstAssignW* अचर ap = V3ParseGrammar::create_Supply_अभिव्यक्ति(fileline, nodep->name(), 0);
        Ast_गाँठो::addNext<Ast_गाँठो, Ast_गाँठो>(nodep, नूतन AstAlways{ap});
    }
    यदि (GRAMMARP->m_varDecl == VVarType::SUPPLY1) {
        AstAssignW* अचर ap = V3ParseGrammar::create_Supply_अभिव्यक्ति(fileline, nodep->name(), 1);
        Ast_गाँठो::addNext<Ast_गाँठो, Ast_गाँठो>(nodep, नूतन AstAlways{ap});
    }
    यदि (VN_IS(dtypep, ParseTypeDType)) {
        // Parser needs to know what is a type
        Ast_गाँठो* अचर newp = नूतन AstTypedefFwd{fileline, name, VFwdType::NONE};
        Ast_गाँठो::addNext<Ast_गाँठो, Ast_गाँठो>(nodep, newp);
    }
    // Don't set dtypep in the ranging;
    // We need to autosize parameters and integers separately
    //
    // Propagate from current module tracing state
    यदि (nodep->isGenVar()) {
        nodep->trace(असत्य);
    } अन्यथा यदि (nodep->isParam() && !v3Global.opt.traceParams()) {
        nodep->trace(असत्य);
    } अन्यथा {
        nodep->trace(allTracingOn(nodep->fileline()));
    }
    यदि (nodep->varType().isVPIAccessible()) nodep->addAttrsp(GRAMMARP->cloneScopedSigAttr());

    // Remember the last variable created, so we can attach attributes to it in later parsing
    GRAMMARP->m_varAttrp = nodep;
    PARSEP->tagNodep(GRAMMARP->m_varAttrp);
    फर्काऊ nodep;
}

string V3ParseGrammar::unquoteString(File_पङ्क्ति* fileline, अचर std::string& text) {
    string err_सन्देश;
    string res = VString::unquoteSVString(text, err_सन्देश);
    यदि (!err_सन्देश.empty()) fileline->v3error(err_सन्देश.c_str());
    फर्काऊ res;
}

पूर्णाङ्क V3ParseGrammar::s_type_Imp_अङ्क = 0;
