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

पूर्णांक V3ParseImp::bisonParse() {
    // Use --debugi-bison 9 to enable this
    जर (PARSEP->debugBison() >= 9) yydebug = 1;
    परत yyparse();
}

स्थिरांक अक्षर* V3ParseImp::tokenName(पूर्णांक token) {
#जर YYDEBUG || YYERROR_VERBOSE
    स्थिर स्थिरांक अक्षर** nameTablep = nullptr;
    जर (!nameTablep) {
        पूर्णांक size;
        साठी (size = 0; yytname[size]; ++size) {}
        nameTablep = नवीन स्थिरांक अक्षर*[size];
        // Workaround bug in bison's which have '!' in yytname but not token values
        पूर्णांक iout = 0;
        साठी (पूर्णांक i = 0; yytname[i]; ++i) {
            जर (yytname[i][0] == '\'') सुरूठेवा;
            nameTablep[iout++] = yytname[i];
        }
    }
    जर (token >= 255) {
        परत nameTablep[token - 255];
    } अन्यथा {
        स्थिर अक्षर s_ch[2];
        s_ch[0] = token;
        s_ch[1] = '\0';
        परत s_ch;
    }
#अन्यथा
    परत "";
#endif
}

शून्य V3ParseImp::candidatePli(VSpellCheck* spellerp) {
#जर !YYERROR_VERBOSE
#error "Need lex token names"
#endif
    साठी (पूर्णांक i = 0; yytname[i]; ++i) {
        जर (yytname[i][0] != '\"') सुरूठेवा;
        जर (yytname[i][1] != '$') सुरूठेवा;
        spellerp->ढकला_Candidate(string{yytname[i]}.substr(1, म_माप(yytname[i]) - 2));
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

AstArg* V3ParseGrammar::argWrapList(Ast_गाठ_अभिव्यक्ती* nodep) {
    // Convert list of expressions to list of arguments
    AstArg* outp = nullptr;
    पर्यंत (nodep) {
        Ast_गाठ_अभिव्यक्ती* स्थिरांक nextp = VN_AS(nodep->nextp(), गाठ_अभिव्यक्ती);
        जर (nextp) nextp->unlinkFrBackWithNext();
        outp = Ast_गाठ::addNext(outp, नवीन AstArg{nodep->fileline(), "", nodep});
        nodep = nextp;
    }
    परत outp;
}

AstAssignW* V3ParseGrammar::create_Supply_अभिव्यक्ती(File_ओळ* fileline, स्थिरांक string& name, पूर्णांक मूल्य) {
    AstAssignW* assignp = नवीन AstAssignW{fileline, नवीन AstParseRef{fileline, name},
                                         मूल्य ? नवीन AstConst{fileline, AstConst::All1{}}
                                               : नवीन AstConst{fileline, AstConst::All0{}}};
    AstStrengthSpec* strengthSpecp
        = नवीन AstStrengthSpec{fileline, VStrength::SUPPLY, VStrength::SUPPLY};
    assignp->strengthSpecp(strengthSpecp);
    परत assignp;
}

AstRange* V3ParseGrammar::scrubRangeMulti(Ast_गाठ_Range* nrangep) {
    // Remove any UnsizedRange's from list; preserves multi-dim chain via nextp().
    साठी (Ast_गाठ_Range *nodep = nrangep, *nextp; nodep; nodep = nextp) {
        nextp = VN_AS(nodep->nextp(), गाठ_Range);
        जर (!VN_IS(nodep, Range)) {
            nodep->v3error(
                "Unsupported or syntax error: Unsized range in instance or other declaration");
            nodep->unlinkFrBack();
            VL_DO_DANGLING(nodep->delete_वृक्ष(), nodep);
        }
    }
    परत VN_CAST(nrangep, Range);
}
AstRange* V3ParseGrammar::scrubRange(Ast_गाठ_Range* nrangep) {
    AstRange* स्थिरांक rangep = scrubRangeMulti(nrangep);
    जर (rangep && rangep->nextp()) {
        // Gate primitives only support a single dimension
        rangep->nextp()->v3warn(E_UNSUPPORTED, "Unsupported: Multidimensional gate instances.");
        rangep->nextp()->unlinkFrBackWithNext()->delete_वृक्ष();
    }
    परत rangep;
}

Ast_गाठ_Pre_Sel* V3ParseGrammar::scrubSel(Ast_गाठ_अभिव्यक्ती* fromp, Ast_गाठ_Pre_Sel* selp) VL_MT_DISABLED {
    // SEL(PARSELVALUE, ...) -> SEL(fromp, ...)
    Ast_गाठ_Pre_Sel* subSelp = selp;
    पर्यंत (सत्य) {
        जर (VN_IS(subSelp->fromp(), ParseHolder)) थांबा;
        जर (Ast_गाठ_Pre_Sel* स्थिरांक lowerSelp = VN_CAST(subSelp->fromp(), गाठ_Pre_Sel)) {
            subSelp = lowerSelp;
            सुरूठेवा;
        }
        subSelp->v3fatalSrc("Couldn't find where to insert expression into select");
    }
    Ast_गाठ* subSelFromp = subSelp->fromp();
    subSelFromp->replaceWith(fromp);
    VL_DO_DANGLING(subSelFromp->delete_वृक्ष(), subSelFromp);
    परत selp;
}

Ast_गाठ_DType* V3ParseGrammar::createArray(Ast_गाठ_DType* basep, Ast_गाठ_Range* nrangep,
                                          तार्किक isPacked) {
    // Split RANGE0-RANGE1-RANGE2
    // into ARRAYDTYPE0(ARRAYDTYPE1(ARRAYDTYPE2(BASICTYPE3), RANGE), RANGE)
    Ast_गाठ_DType* arrayp = basep;
    जर (nrangep) {  // Maybe no range - return unmodified base type
        पर्यंत (nrangep->nextp()) nrangep = VN_AS(nrangep->nextp(), गाठ_Range);
        पर्यंत (nrangep) {
            Ast_गाठ_Range* स्थिरांक prevp = VN_AS(nrangep->backp(), गाठ_Range);
            जर (prevp) nrangep->unlinkFrBack();
            AstRange* स्थिरांक rangep = VN_CAST(nrangep, Range);
            जर (rangep && isPacked) {
                arrayp
                    = नवीन AstPackArrayDType{rangep->fileline(), VFlagChildDType{}, arrayp, rangep};
            } अन्यथा जर (rangep
                       && (VN_IS(rangep->leftp(), Unbounded)
                           || VN_IS(rangep->rightp(), Unbounded))) {
                arrayp = नवीन AstQueueDType{nrangep->fileline(), VFlagChildDType{}, arrayp,
                                           rangep->rightp()->clone_वृक्ष(सत्य)};
                VL_DO_DANGLING(nrangep->delete_वृक्ष(), nrangep);
            } अन्यथा जर (rangep) {
                arrayp = नवीन AstUnpackArrayDType{rangep->fileline(), VFlagChildDType{}, arrayp,
                                                 rangep};
            } अन्यथा जर (VN_IS(nrangep, UnsizedRange)) {
                arrayp = नवीन AstDynArrayDType{nrangep->fileline(), VFlagChildDType{}, arrayp};
                VL_DO_DANGLING(nrangep->delete_वृक्ष(), nrangep);
            } अन्यथा जर (VN_IS(nrangep, BracketRange)) {
                स्थिरांक AstBracketRange* स्थिरांक arangep = VN_AS(nrangep, BracketRange);
                Ast_गाठ* स्थिरांक keyp = arangep->elementsp()->unlinkFrBack();
                arrayp = नवीन AstBracketArrayDType{nrangep->fileline(), VFlagChildDType{}, arrayp,
                                                  keyp};
                VL_DO_DANGLING(nrangep->delete_वृक्ष(), nrangep);
            } अन्यथा जर (VN_IS(nrangep, WildcardRange)) {
                arrayp = नवीन AstWildcardArrayDType{nrangep->fileline(), VFlagChildDType{}, arrayp};
                VL_DO_DANGLING(nrangep->delete_वृक्ष(), nrangep);
            } अन्यथा {
                UASSERT_OBJ(0, nrangep, "Expected range or unsized range");
            }
            nrangep = prevp;
        }
    }
    परत arrayp;
}

AstVar* V3ParseGrammar::createVariable(File_ओळ* fileline, स्थिरांक string& name,
                                       Ast_गाठ_Range* arrayp, Ast_गाठ* attrsp) {
    UINFO(5, "  creVar " << name << "  decl=" << GRAMMARP->m_varDecl << "  io="
                         << GRAMMARP->m_varIO << "  dt=" << (GRAMMARP->m_varDTypep ? "set" : ""));
    जर (GRAMMARP->m_varIO == VDirection::NONE  // In non-ANSI port list
        && GRAMMARP->m_varDecl == VVarType::PORT) {
        // Just a port list with variable name (not v2k format); AstPort already created
        जर (arrayp) VL_DO_DANGLING(arrayp->delete_वृक्ष(), arrayp);
        जर (attrsp) {
            // TODO: Merge attributes across list? Or warn attribute is ignored
            VL_DO_DANGLING(attrsp->delete_वृक्ष(), attrsp);
        }
        परत nullptr;
    }

    Ast_गाठ_DType* स्थिरांक dtypep = [&]() -> Ast_गाठ_DType* {
        जर (GRAMMARP->m_varDecl == VVarType::WREAL) {
            // dtypep might not be null, might be implicit LOGIC before we knew better
            परत नवीन AstBasicDType{fileline, VBasicDTypeKwd::DOUBLE};
        }
        जर (GRAMMARP->m_varDTypep) {
            // May make new variables with same type, so clone
            परत GRAMMARP->m_varDTypep->clone_वृक्ष(असत्य);
        }
        // Created implicitly
        जर (m_insideProperty) {
            जर (m_typedPropertyPort) {
                fileline->v3warn(E_UNSUPPORTED, "Untyped property port following a typed port");
            }
            परत नवीन AstBasicDType{fileline, VBasicDTypeKwd::UNTYPED};
        }
        परत नवीन AstBasicDType{fileline, LOGIC_IMPLICIT};
    }();

    // UINFO(0,"CREVAR "<<fileline->ascii()<<" decl="<<GRAMMARP->m_varDecl.ascii()<<"
    // io="<<GRAMMARP->m_varIO.ascii()<<endl);
    VVarType type = GRAMMARP->m_varDecl;
    जर (type == VVarType::UNKNOWN) {  // e.g. "output" non-ANSI standalone direction (vs "reg")
        जर (GRAMMARP->m_varIO.isAny()) {
            type = VVarType::PORT;
        } अन्यथा {
            fileline->v3fatalSrc("Unknown signal type declared: " << type.ascii());
        }
    }
    जर (type == VVarType::GENVAR) {
        // Should be impossible as the grammar blocks this, but...
        जर (arrayp) fileline->v3error("Genvars may not be arrayed: " << name);  // LCOV_EXCL_LINE
    }

    // Split RANGE0-RANGE1-RANGE2 into
    // ARRAYDTYPE0(ARRAYDTYPE1(ARRAYDTYPE2(BASICTYPE3), RANGE), RANGE)
    Ast_गाठ_DType* स्थिरांक arrayDTypep = createArray(dtypep, arrayp, असत्य);

    AstVar* स्थिरांक nodep = नवीन AstVar{fileline, type, name, VFlagChildDType(), arrayDTypep};
    nodep->addAttrsp(attrsp);
    nodep->ansi(m_pinAnsi);
    nodep->declTyped(m_varDeclTyped);
    nodep->lifetime(m_varLifetime);
    जर (m_netDelayp) nodep->delayp(m_netDelayp->clone_वृक्ष(असत्य));
    जर (GRAMMARP->m_varDecl != VVarType::UNKNOWN) nodep->combineType(GRAMMARP->m_varDecl);
    जर (GRAMMARP->m_varIO != VDirection::NONE) {
        nodep->declDirection(GRAMMARP->m_varIO);
        nodep->direction(GRAMMARP->m_varIO);
    }

    जर (GRAMMARP->m_varDecl == VVarType::SUPPLY0) {
        AstAssignW* स्थिरांक ap = V3ParseGrammar::create_Supply_अभिव्यक्ती(fileline, nodep->name(), 0);
        Ast_गाठ::addNext<Ast_गाठ, Ast_गाठ>(nodep, नवीन AstAlways{ap});
    }
    जर (GRAMMARP->m_varDecl == VVarType::SUPPLY1) {
        AstAssignW* स्थिरांक ap = V3ParseGrammar::create_Supply_अभिव्यक्ती(fileline, nodep->name(), 1);
        Ast_गाठ::addNext<Ast_गाठ, Ast_गाठ>(nodep, नवीन AstAlways{ap});
    }
    जर (VN_IS(dtypep, ParseTypeDType)) {
        // Parser needs to know what is a type
        Ast_गाठ* स्थिरांक newp = नवीन AstTypedefFwd{fileline, name, VFwdType::NONE};
        Ast_गाठ::addNext<Ast_गाठ, Ast_गाठ>(nodep, newp);
    }
    // Don't set dtypep in the ranging;
    // We need to autosize parameters and integers separately
    //
    // Propagate from current module tracing state
    जर (nodep->isGenVar()) {
        nodep->trace(असत्य);
    } अन्यथा जर (nodep->isParam() && !v3Global.opt.traceParams()) {
        nodep->trace(असत्य);
    } अन्यथा {
        nodep->trace(allTracingOn(nodep->fileline()));
    }
    जर (nodep->varType().isVPIAccessible()) nodep->addAttrsp(GRAMMARP->cloneScopedSigAttr());

    // Remember the last variable created, so we can attach attributes to it in later parsing
    GRAMMARP->m_varAttrp = nodep;
    PARSEP->tagNodep(GRAMMARP->m_varAttrp);
    परत nodep;
}

string V3ParseGrammar::unquoteString(File_ओळ* fileline, स्थिरांक std::string& text) {
    string err_संदेश;
    string res = VString::unquoteSVString(text, err_संदेश);
    जर (!err_संदेश.empty()) fileline->v3error(err_संदेश.c_str());
    परत res;
}

पूर्णांक V3ParseGrammar::s_type_Imp_अंक = 0;
