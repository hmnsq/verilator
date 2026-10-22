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

#അന്തര്ഭാവ "V3Ast.h"  // This must be before V3ParseBison.cpp, as we don't want #defines to conflict

VL_DEFINE_DEBUG_FUNCTIONS;

//======================================================================
// The guts come from bison output

#അന്തര്ഭാവ "V3ParseBison.c"

//======================================================================
// V3ParseImp functions requiring bison state

പൂർണ്ണസംഖ്യ V3ParseImp::bisonParse() {
    // Use --debugi-bison 9 to enable this
    എങ്കിൽ (PARSEP->debugBison() >= 9) yydebug = 1;
    തിരികെ yyparse();
}

സ്ഥിരം അക്ഷരം* V3ParseImp::tokenName(പൂർണ്ണസംഖ്യ token) {
#എങ്കിൽ YYDEBUG || YYERROR_VERBOSE
    സ്ഥിതം സ്ഥിരം അക്ഷരം** nameTablep = nullptr;
    എങ്കിൽ (!nameTablep) {
        പൂർണ്ണസംഖ്യ size;
        വേണ്ടി (size = 0; yytname[size]; ++size) {}
        nameTablep = പുതിയ സ്ഥിരം അക്ഷരം*[size];
        // Workaround bug in bison's which have '!' in yytname but not token values
        പൂർണ്ണസംഖ്യ iout = 0;
        വേണ്ടി (പൂർണ്ണസംഖ്യ i = 0; yytname[i]; ++i) {
            എങ്കിൽ (yytname[i][0] == '\'') തുടരുക;
            nameTablep[iout++] = yytname[i];
        }
    }
    എങ്കിൽ (token >= 255) {
        തിരികെ nameTablep[token - 255];
    } അല്ലെങ്കിൽ {
        സ്ഥിതം അക്ഷരം s_ch[2];
        s_ch[0] = token;
        s_ch[1] = '\0';
        തിരികെ s_ch;
    }
#അല്ലെങ്കിൽ
    തിരികെ "";
#endif
}

ശൂന്യം V3ParseImp::candidatePli(VSpellCheck* spellerp) {
#എങ്കിൽ !YYERROR_VERBOSE
#error "Need lex token names"
#endif
    വേണ്ടി (പൂർണ്ണസംഖ്യ i = 0; yytname[i]; ++i) {
        എങ്കിൽ (yytname[i][0] != '\"') തുടരുക;
        എങ്കിൽ (yytname[i][1] != '$') തുടരുക;
        spellerp->തള്ളുക_Candidate(string{yytname[i]}.substr(1, strlen(yytname[i]) - 2));
    }
}

ശൂന്യം V3ParseImp::parserClear() {
    // Clear up any dynamic memory V3Parser required
    VARDTYPE(nullptr);
    GRAMMARP->setNetDelay(nullptr);
    GRAMMARP->setScopedSigAttr(nullptr);
}

//======================================================================
// V3ParseGrammar functions requiring bison state

AstArg* V3ParseGrammar::argWrapList(Ast_കണ്ണി_വ്യഞ്ജകം* nodep) {
    // Convert list of expressions to list of arguments
    AstArg* outp = nullptr;
    ആകുമ്പോൾ (nodep) {
        Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം nextp = VN_AS(nodep->nextp(), കണ്ണി_വ്യഞ്ജകം);
        എങ്കിൽ (nextp) nextp->unlinkFrBackWithNext();
        outp = Ast_കണ്ണി::addNext(outp, പുതിയ AstArg{nodep->fileline(), "", nodep});
        nodep = nextp;
    }
    തിരികെ outp;
}

AstAssignW* V3ParseGrammar::create_Supply_വ്യഞ്ജകം(File_വരി* fileline, സ്ഥിരം string& name, പൂർണ്ണസംഖ്യ മൂല്യം) {
    AstAssignW* assignp = പുതിയ AstAssignW{fileline, പുതിയ AstParseRef{fileline, name},
                                         മൂല്യം ? പുതിയ AstConst{fileline, AstConst::All1{}}
                                               : പുതിയ AstConst{fileline, AstConst::All0{}}};
    AstStrengthSpec* strengthSpecp
        = പുതിയ AstStrengthSpec{fileline, VStrength::SUPPLY, VStrength::SUPPLY};
    assignp->strengthSpecp(strengthSpecp);
    തിരികെ assignp;
}

AstRange* V3ParseGrammar::scrubRangeMulti(Ast_കണ്ണി_Range* nrangep) {
    // Remove any UnsizedRange's from list; preserves multi-dim chain via nextp().
    വേണ്ടി (Ast_കണ്ണി_Range *nodep = nrangep, *nextp; nodep; nodep = nextp) {
        nextp = VN_AS(nodep->nextp(), കണ്ണി_Range);
        എങ്കിൽ (!VN_IS(nodep, Range)) {
            nodep->v3error(
                "Unsupported or syntax error: Unsized range in instance or other declaration");
            nodep->unlinkFrBack();
            VL_DO_DANGLING(nodep->delete_വൃക്ഷം(), nodep);
        }
    }
    തിരികെ VN_CAST(nrangep, Range);
}
AstRange* V3ParseGrammar::scrubRange(Ast_കണ്ണി_Range* nrangep) {
    AstRange* സ്ഥിരം rangep = scrubRangeMulti(nrangep);
    എങ്കിൽ (rangep && rangep->nextp()) {
        // Gate primitives only support a single dimension
        rangep->nextp()->v3warn(E_UNSUPPORTED, "Unsupported: Multidimensional gate instances.");
        rangep->nextp()->unlinkFrBackWithNext()->delete_വൃക്ഷം();
    }
    തിരികെ rangep;
}

Ast_കണ്ണി_Pre_Sel* V3ParseGrammar::scrubSel(Ast_കണ്ണി_വ്യഞ്ജകം* fromp, Ast_കണ്ണി_Pre_Sel* selp) VL_MT_DISABLED {
    // SEL(PARSELVALUE, ...) -> SEL(fromp, ...)
    Ast_കണ്ണി_Pre_Sel* subSelp = selp;
    ആകുമ്പോൾ (സത്യം) {
        എങ്കിൽ (VN_IS(subSelp->fromp(), ParseHolder)) നിർത്തുക;
        എങ്കിൽ (Ast_കണ്ണി_Pre_Sel* സ്ഥിരം lowerSelp = VN_CAST(subSelp->fromp(), കണ്ണി_Pre_Sel)) {
            subSelp = lowerSelp;
            തുടരുക;
        }
        subSelp->v3fatalSrc("Couldn't find where to insert expression into select");
    }
    Ast_കണ്ണി* subSelFromp = subSelp->fromp();
    subSelFromp->replaceWith(fromp);
    VL_DO_DANGLING(subSelFromp->delete_വൃക്ഷം(), subSelFromp);
    തിരികെ selp;
}

Ast_കണ്ണി_DType* V3ParseGrammar::createArray(Ast_കണ്ണി_DType* basep, Ast_കണ്ണി_Range* nrangep,
                                          തർക്കം isPacked) {
    // Split RANGE0-RANGE1-RANGE2
    // into ARRAYDTYPE0(ARRAYDTYPE1(ARRAYDTYPE2(BASICTYPE3), RANGE), RANGE)
    Ast_കണ്ണി_DType* arrayp = basep;
    എങ്കിൽ (nrangep) {  // Maybe no range - return unmodified base type
        ആകുമ്പോൾ (nrangep->nextp()) nrangep = VN_AS(nrangep->nextp(), കണ്ണി_Range);
        ആകുമ്പോൾ (nrangep) {
            Ast_കണ്ണി_Range* സ്ഥിരം prevp = VN_AS(nrangep->backp(), കണ്ണി_Range);
            എങ്കിൽ (prevp) nrangep->unlinkFrBack();
            AstRange* സ്ഥിരം rangep = VN_CAST(nrangep, Range);
            എങ്കിൽ (rangep && isPacked) {
                arrayp
                    = പുതിയ AstPackArrayDType{rangep->fileline(), VFlagChildDType{}, arrayp, rangep};
            } അല്ലെങ്കിൽ എങ്കിൽ (rangep
                       && (VN_IS(rangep->leftp(), Unbounded)
                           || VN_IS(rangep->rightp(), Unbounded))) {
                arrayp = പുതിയ AstQueueDType{nrangep->fileline(), VFlagChildDType{}, arrayp,
                                           rangep->rightp()->clone_വൃക്ഷം(സത്യം)};
                VL_DO_DANGLING(nrangep->delete_വൃക്ഷം(), nrangep);
            } അല്ലെങ്കിൽ എങ്കിൽ (rangep) {
                arrayp = പുതിയ AstUnpackArrayDType{rangep->fileline(), VFlagChildDType{}, arrayp,
                                                 rangep};
            } അല്ലെങ്കിൽ എങ്കിൽ (VN_IS(nrangep, UnsizedRange)) {
                arrayp = പുതിയ AstDynArrayDType{nrangep->fileline(), VFlagChildDType{}, arrayp};
                VL_DO_DANGLING(nrangep->delete_വൃക്ഷം(), nrangep);
            } അല്ലെങ്കിൽ എങ്കിൽ (VN_IS(nrangep, BracketRange)) {
                സ്ഥിരം AstBracketRange* സ്ഥിരം arangep = VN_AS(nrangep, BracketRange);
                Ast_കണ്ണി* സ്ഥിരം keyp = arangep->elementsp()->unlinkFrBack();
                arrayp = പുതിയ AstBracketArrayDType{nrangep->fileline(), VFlagChildDType{}, arrayp,
                                                  keyp};
                VL_DO_DANGLING(nrangep->delete_വൃക്ഷം(), nrangep);
            } അല്ലെങ്കിൽ എങ്കിൽ (VN_IS(nrangep, WildcardRange)) {
                arrayp = പുതിയ AstWildcardArrayDType{nrangep->fileline(), VFlagChildDType{}, arrayp};
                VL_DO_DANGLING(nrangep->delete_വൃക്ഷം(), nrangep);
            } അല്ലെങ്കിൽ {
                UASSERT_OBJ(0, nrangep, "Expected range or unsized range");
            }
            nrangep = prevp;
        }
    }
    തിരികെ arrayp;
}

AstVar* V3ParseGrammar::createVariable(File_വരി* fileline, സ്ഥിരം string& name,
                                       Ast_കണ്ണി_Range* arrayp, Ast_കണ്ണി* attrsp) {
    UINFO(5, "  creVar " << name << "  decl=" << GRAMMARP->m_varDecl << "  io="
                         << GRAMMARP->m_varIO << "  dt=" << (GRAMMARP->m_varDTypep ? "set" : ""));
    എങ്കിൽ (GRAMMARP->m_varIO == VDirection::NONE  // In non-ANSI port list
        && GRAMMARP->m_varDecl == VVarType::PORT) {
        // Just a port list with variable name (not v2k format); AstPort already created
        എങ്കിൽ (arrayp) VL_DO_DANGLING(arrayp->delete_വൃക്ഷം(), arrayp);
        എങ്കിൽ (attrsp) {
            // TODO: Merge attributes across list? Or warn attribute is ignored
            VL_DO_DANGLING(attrsp->delete_വൃക്ഷം(), attrsp);
        }
        തിരികെ nullptr;
    }

    Ast_കണ്ണി_DType* സ്ഥിരം dtypep = [&]() -> Ast_കണ്ണി_DType* {
        എങ്കിൽ (GRAMMARP->m_varDecl == VVarType::WREAL) {
            // dtypep might not be null, might be implicit LOGIC before we knew better
            തിരികെ പുതിയ AstBasicDType{fileline, VBasicDTypeKwd::DOUBLE};
        }
        എങ്കിൽ (GRAMMARP->m_varDTypep) {
            // May make new variables with same type, so clone
            തിരികെ GRAMMARP->m_varDTypep->clone_വൃക്ഷം(അസത്യം);
        }
        // Created implicitly
        എങ്കിൽ (m_insideProperty) {
            എങ്കിൽ (m_typedPropertyPort) {
                fileline->v3warn(E_UNSUPPORTED, "Untyped property port following a typed port");
            }
            തിരികെ പുതിയ AstBasicDType{fileline, VBasicDTypeKwd::UNTYPED};
        }
        തിരികെ പുതിയ AstBasicDType{fileline, LOGIC_IMPLICIT};
    }();

    // UINFO(0,"CREVAR "<<fileline->ascii()<<" decl="<<GRAMMARP->m_varDecl.ascii()<<"
    // io="<<GRAMMARP->m_varIO.ascii()<<endl);
    VVarType type = GRAMMARP->m_varDecl;
    എങ്കിൽ (type == VVarType::UNKNOWN) {  // e.g. "output" non-ANSI standalone direction (vs "reg")
        എങ്കിൽ (GRAMMARP->m_varIO.isAny()) {
            type = VVarType::PORT;
        } അല്ലെങ്കിൽ {
            fileline->v3fatalSrc("Unknown signal type declared: " << type.ascii());
        }
    }
    എങ്കിൽ (type == VVarType::GENVAR) {
        // Should be impossible as the grammar blocks this, but...
        എങ്കിൽ (arrayp) fileline->v3error("Genvars may not be arrayed: " << name);  // LCOV_EXCL_LINE
    }

    // Split RANGE0-RANGE1-RANGE2 into
    // ARRAYDTYPE0(ARRAYDTYPE1(ARRAYDTYPE2(BASICTYPE3), RANGE), RANGE)
    Ast_കണ്ണി_DType* സ്ഥിരം arrayDTypep = createArray(dtypep, arrayp, അസത്യം);

    AstVar* സ്ഥിരം nodep = പുതിയ AstVar{fileline, type, name, VFlagChildDType(), arrayDTypep};
    nodep->addAttrsp(attrsp);
    nodep->ansi(m_pinAnsi);
    nodep->declTyped(m_varDeclTyped);
    nodep->lifetime(m_varLifetime);
    എങ്കിൽ (m_netDelayp) nodep->delayp(m_netDelayp->clone_വൃക്ഷം(അസത്യം));
    എങ്കിൽ (GRAMMARP->m_varDecl != VVarType::UNKNOWN) nodep->combineType(GRAMMARP->m_varDecl);
    എങ്കിൽ (GRAMMARP->m_varIO != VDirection::NONE) {
        nodep->declDirection(GRAMMARP->m_varIO);
        nodep->direction(GRAMMARP->m_varIO);
    }

    എങ്കിൽ (GRAMMARP->m_varDecl == VVarType::SUPPLY0) {
        AstAssignW* സ്ഥിരം ap = V3ParseGrammar::create_Supply_വ്യഞ്ജകം(fileline, nodep->name(), 0);
        Ast_കണ്ണി::addNext<Ast_കണ്ണി, Ast_കണ്ണി>(nodep, പുതിയ AstAlways{ap});
    }
    എങ്കിൽ (GRAMMARP->m_varDecl == VVarType::SUPPLY1) {
        AstAssignW* സ്ഥിരം ap = V3ParseGrammar::create_Supply_വ്യഞ്ജകം(fileline, nodep->name(), 1);
        Ast_കണ്ണി::addNext<Ast_കണ്ണി, Ast_കണ്ണി>(nodep, പുതിയ AstAlways{ap});
    }
    എങ്കിൽ (VN_IS(dtypep, ParseTypeDType)) {
        // Parser needs to know what is a type
        Ast_കണ്ണി* സ്ഥിരം newp = പുതിയ AstTypedefFwd{fileline, name, VFwdType::NONE};
        Ast_കണ്ണി::addNext<Ast_കണ്ണി, Ast_കണ്ണി>(nodep, newp);
    }
    // Don't set dtypep in the ranging;
    // We need to autosize parameters and integers separately
    //
    // Propagate from current module tracing state
    എങ്കിൽ (nodep->isGenVar()) {
        nodep->trace(അസത്യം);
    } അല്ലെങ്കിൽ എങ്കിൽ (nodep->isParam() && !v3Global.opt.traceParams()) {
        nodep->trace(അസത്യം);
    } അല്ലെങ്കിൽ {
        nodep->trace(allTracingOn(nodep->fileline()));
    }
    എങ്കിൽ (nodep->varType().isVPIAccessible()) nodep->addAttrsp(GRAMMARP->cloneScopedSigAttr());

    // Remember the last variable created, so we can attach attributes to it in later parsing
    GRAMMARP->m_varAttrp = nodep;
    PARSEP->tagNodep(GRAMMARP->m_varAttrp);
    തിരികെ nodep;
}

string V3ParseGrammar::unquoteString(File_വരി* fileline, സ്ഥിരം std::string& text) {
    string err_സന്ദേശം;
    string res = VString::unquoteSVString(text, err_സന്ദേശം);
    എങ്കിൽ (!err_സന്ദേശം.empty()) fileline->v3error(err_സന്ദേശം.c_str());
    തിരികെ res;
}

പൂർണ്ണസംഖ്യ V3ParseGrammar::s_type_Imp_അക്കം = 0;
