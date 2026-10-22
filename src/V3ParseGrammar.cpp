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

#ଅନ୍ତର୍ଭାଵ "V3Ast.h"  // This must be before V3ParseBison.cpp, as we don't want #defines to conflict

VL_DEFINE_DEBUG_FUNCTIONS;

//======================================================================
// The guts come from bison output

#ଅନ୍ତର୍ଭାଵ "V3ParseBison.c"

//======================================================================
// V3ParseImp functions requiring bison state

ପୂର୍ଣ୍ଣସଂଖ୍ୟା V3ParseImp::bisonParse() {
    // Use --debugi-bison 9 to enable this
    ଯଦି (PARSEP->debugBison() >= 9) yydebug = 1;
    ଫେରନ୍ତୁ yyparse();
}

ଅଚଳ ଅକ୍ଷର* V3ParseImp::tokenName(ପୂର୍ଣ୍ଣସଂଖ୍ୟା token) {
#ଯଦି YYDEBUG || YYERROR_VERBOSE
    ସ୍ଥିର ଅଚଳ ଅକ୍ଷର** nameTablep = nullptr;
    ଯଦି (!nameTablep) {
        ପୂର୍ଣ୍ଣସଂଖ୍ୟା size;
        ପାଇଁ (size = 0; yytname[size]; ++size) {}
        nameTablep = ନୂତନ ଅଚଳ ଅକ୍ଷର*[size];
        // Workaround bug in bison's which have '!' in yytname but not token values
        ପୂର୍ଣ୍ଣସଂଖ୍ୟା iout = 0;
        ପାଇଁ (ପୂର୍ଣ୍ଣସଂଖ୍ୟା i = 0; yytname[i]; ++i) {
            ଯଦି (yytname[i][0] == '\'') ଜାରି;
            nameTablep[iout++] = yytname[i];
        }
    }
    ଯଦି (token >= 255) {
        ଫେରନ୍ତୁ nameTablep[token - 255];
    } ଅନ୍ୟଥା {
        ସ୍ଥିର ଅକ୍ଷର s_ch[2];
        s_ch[0] = token;
        s_ch[1] = '\0';
        ଫେରନ୍ତୁ s_ch;
    }
#ଅନ୍ୟଥା
    ଫେରନ୍ତୁ "";
#endif
}

ଶୂନ୍ୟ V3ParseImp::candidatePli(VSpellCheck* spellerp) {
#ଯଦି !YYERROR_VERBOSE
#error "Need lex token names"
#endif
    ପାଇଁ (ପୂର୍ଣ୍ଣସଂଖ୍ୟା i = 0; yytname[i]; ++i) {
        ଯଦି (yytname[i][0] != '\"') ଜାରି;
        ଯଦି (yytname[i][1] != '$') ଜାରି;
        spellerp->ଠେଲ_Candidate(string{yytname[i]}.substr(1, strlen(yytname[i]) - 2));
    }
}

ଶୂନ୍ୟ V3ParseImp::parserClear() {
    // Clear up any dynamic memory V3Parser required
    VARDTYPE(nullptr);
    GRAMMARP->setNetDelay(nullptr);
    GRAMMARP->setScopedSigAttr(nullptr);
}

//======================================================================
// V3ParseGrammar functions requiring bison state

AstArg* V3ParseGrammar::argWrapList(Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* nodep) {
    // Convert list of expressions to list of arguments
    AstArg* outp = nullptr;
    ଯେତେବେଳେ (nodep) {
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ nextp = VN_AS(nodep->nextp(), ଗଣ୍ଠି_ପଦସମଷ୍ଟି);
        ଯଦି (nextp) nextp->unlinkFrBackWithNext();
        outp = Ast_ଗଣ୍ଠି::addNext(outp, ନୂତନ AstArg{nodep->fileline(), "", nodep});
        nodep = nextp;
    }
    ଫେରନ୍ତୁ outp;
}

AstAssignW* V3ParseGrammar::create_Supply_ପଦସମଷ୍ଟି(File_ଧାଡ଼ି* fileline, ଅଚଳ string& name, ପୂର୍ଣ୍ଣସଂଖ୍ୟା ମୂଲ୍ୟ) {
    AstAssignW* assignp = ନୂତନ AstAssignW{fileline, ନୂତନ AstParseRef{fileline, name},
                                         ମୂଲ୍ୟ ? ନୂତନ AstConst{fileline, AstConst::All1{}}
                                               : ନୂତନ AstConst{fileline, AstConst::All0{}}};
    AstStrengthSpec* strengthSpecp
        = ନୂତନ AstStrengthSpec{fileline, VStrength::SUPPLY, VStrength::SUPPLY};
    assignp->strengthSpecp(strengthSpecp);
    ଫେରନ୍ତୁ assignp;
}

AstRange* V3ParseGrammar::scrubRangeMulti(Ast_ଗଣ୍ଠି_Range* nrangep) {
    // Remove any UnsizedRange's from list; preserves multi-dim chain via nextp().
    ପାଇଁ (Ast_ଗଣ୍ଠି_Range *nodep = nrangep, *nextp; nodep; nodep = nextp) {
        nextp = VN_AS(nodep->nextp(), ଗଣ୍ଠି_Range);
        ଯଦି (!VN_IS(nodep, Range)) {
            nodep->v3error(
                "Unsupported or syntax error: Unsized range in instance or other declaration");
            nodep->unlinkFrBack();
            VL_DO_DANGLING(nodep->delete_ବୃକ୍ଷ(), nodep);
        }
    }
    ଫେରନ୍ତୁ VN_CAST(nrangep, Range);
}
AstRange* V3ParseGrammar::scrubRange(Ast_ଗଣ୍ଠି_Range* nrangep) {
    AstRange* ଅଚଳ rangep = scrubRangeMulti(nrangep);
    ଯଦି (rangep && rangep->nextp()) {
        // Gate primitives only support a single dimension
        rangep->nextp()->v3warn(E_UNSUPPORTED, "Unsupported: Multidimensional gate instances.");
        rangep->nextp()->unlinkFrBackWithNext()->delete_ବୃକ୍ଷ();
    }
    ଫେରନ୍ତୁ rangep;
}

Ast_ଗଣ୍ଠି_Pre_Sel* V3ParseGrammar::scrubSel(Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* fromp, Ast_ଗଣ୍ଠି_Pre_Sel* selp) VL_MT_DISABLED {
    // SEL(PARSELVALUE, ...) -> SEL(fromp, ...)
    Ast_ଗଣ୍ଠି_Pre_Sel* subSelp = selp;
    ଯେତେବେଳେ (ସତ୍ୟ) {
        ଯଦି (VN_IS(subSelp->fromp(), ParseHolder)) ଭାଙ୍ଗ;
        ଯଦି (Ast_ଗଣ୍ଠି_Pre_Sel* ଅଚଳ lowerSelp = VN_CAST(subSelp->fromp(), ଗଣ୍ଠି_Pre_Sel)) {
            subSelp = lowerSelp;
            ଜାରି;
        }
        subSelp->v3fatalSrc("Couldn't find where to insert expression into select");
    }
    Ast_ଗଣ୍ଠି* subSelFromp = subSelp->fromp();
    subSelFromp->replaceWith(fromp);
    VL_DO_DANGLING(subSelFromp->delete_ବୃକ୍ଷ(), subSelFromp);
    ଫେରନ୍ତୁ selp;
}

Ast_ଗଣ୍ଠି_DType* V3ParseGrammar::createArray(Ast_ଗଣ୍ଠି_DType* basep, Ast_ଗଣ୍ଠି_Range* nrangep,
                                          ତାର୍କିକ isPacked) {
    // Split RANGE0-RANGE1-RANGE2
    // into ARRAYDTYPE0(ARRAYDTYPE1(ARRAYDTYPE2(BASICTYPE3), RANGE), RANGE)
    Ast_ଗଣ୍ଠି_DType* arrayp = basep;
    ଯଦି (nrangep) {  // Maybe no range - return unmodified base type
        ଯେତେବେଳେ (nrangep->nextp()) nrangep = VN_AS(nrangep->nextp(), ଗଣ୍ଠି_Range);
        ଯେତେବେଳେ (nrangep) {
            Ast_ଗଣ୍ଠି_Range* ଅଚଳ prevp = VN_AS(nrangep->backp(), ଗଣ୍ଠି_Range);
            ଯଦି (prevp) nrangep->unlinkFrBack();
            AstRange* ଅଚଳ rangep = VN_CAST(nrangep, Range);
            ଯଦି (rangep && isPacked) {
                arrayp
                    = ନୂତନ AstPackArrayDType{rangep->fileline(), VFlagChildDType{}, arrayp, rangep};
            } ଅନ୍ୟଥା ଯଦି (rangep
                       && (VN_IS(rangep->leftp(), Unbounded)
                           || VN_IS(rangep->rightp(), Unbounded))) {
                arrayp = ନୂତନ AstQueueDType{nrangep->fileline(), VFlagChildDType{}, arrayp,
                                           rangep->rightp()->clone_ବୃକ୍ଷ(ସତ୍ୟ)};
                VL_DO_DANGLING(nrangep->delete_ବୃକ୍ଷ(), nrangep);
            } ଅନ୍ୟଥା ଯଦି (rangep) {
                arrayp = ନୂତନ AstUnpackArrayDType{rangep->fileline(), VFlagChildDType{}, arrayp,
                                                 rangep};
            } ଅନ୍ୟଥା ଯଦି (VN_IS(nrangep, UnsizedRange)) {
                arrayp = ନୂତନ AstDynArrayDType{nrangep->fileline(), VFlagChildDType{}, arrayp};
                VL_DO_DANGLING(nrangep->delete_ବୃକ୍ଷ(), nrangep);
            } ଅନ୍ୟଥା ଯଦି (VN_IS(nrangep, BracketRange)) {
                ଅଚଳ AstBracketRange* ଅଚଳ arangep = VN_AS(nrangep, BracketRange);
                Ast_ଗଣ୍ଠି* ଅଚଳ keyp = arangep->elementsp()->unlinkFrBack();
                arrayp = ନୂତନ AstBracketArrayDType{nrangep->fileline(), VFlagChildDType{}, arrayp,
                                                  keyp};
                VL_DO_DANGLING(nrangep->delete_ବୃକ୍ଷ(), nrangep);
            } ଅନ୍ୟଥା ଯଦି (VN_IS(nrangep, WildcardRange)) {
                arrayp = ନୂତନ AstWildcardArrayDType{nrangep->fileline(), VFlagChildDType{}, arrayp};
                VL_DO_DANGLING(nrangep->delete_ବୃକ୍ଷ(), nrangep);
            } ଅନ୍ୟଥା {
                UASSERT_OBJ(0, nrangep, "Expected range or unsized range");
            }
            nrangep = prevp;
        }
    }
    ଫେରନ୍ତୁ arrayp;
}

AstVar* V3ParseGrammar::createVariable(File_ଧାଡ଼ି* fileline, ଅଚଳ string& name,
                                       Ast_ଗଣ୍ଠି_Range* arrayp, Ast_ଗଣ୍ଠି* attrsp) {
    UINFO(5, "  creVar " << name << "  decl=" << GRAMMARP->m_varDecl << "  io="
                         << GRAMMARP->m_varIO << "  dt=" << (GRAMMARP->m_varDTypep ? "set" : ""));
    ଯଦି (GRAMMARP->m_varIO == VDirection::NONE  // In non-ANSI port list
        && GRAMMARP->m_varDecl == VVarType::PORT) {
        // Just a port list with variable name (not v2k format); AstPort already created
        ଯଦି (arrayp) VL_DO_DANGLING(arrayp->delete_ବୃକ୍ଷ(), arrayp);
        ଯଦି (attrsp) {
            // TODO: Merge attributes across list? Or warn attribute is ignored
            VL_DO_DANGLING(attrsp->delete_ବୃକ୍ଷ(), attrsp);
        }
        ଫେରନ୍ତୁ nullptr;
    }

    Ast_ଗଣ୍ଠି_DType* ଅଚଳ dtypep = [&]() -> Ast_ଗଣ୍ଠି_DType* {
        ଯଦି (GRAMMARP->m_varDecl == VVarType::WREAL) {
            // dtypep might not be null, might be implicit LOGIC before we knew better
            ଫେରନ୍ତୁ ନୂତନ AstBasicDType{fileline, VBasicDTypeKwd::DOUBLE};
        }
        ଯଦି (GRAMMARP->m_varDTypep) {
            // May make new variables with same type, so clone
            ଫେରନ୍ତୁ GRAMMARP->m_varDTypep->clone_ବୃକ୍ଷ(ମିଥ୍ୟା);
        }
        // Created implicitly
        ଯଦି (m_insideProperty) {
            ଯଦି (m_typedPropertyPort) {
                fileline->v3warn(E_UNSUPPORTED, "Untyped property port following a typed port");
            }
            ଫେରନ୍ତୁ ନୂତନ AstBasicDType{fileline, VBasicDTypeKwd::UNTYPED};
        }
        ଫେରନ୍ତୁ ନୂତନ AstBasicDType{fileline, LOGIC_IMPLICIT};
    }();

    // UINFO(0,"CREVAR "<<fileline->ascii()<<" decl="<<GRAMMARP->m_varDecl.ascii()<<"
    // io="<<GRAMMARP->m_varIO.ascii()<<endl);
    VVarType type = GRAMMARP->m_varDecl;
    ଯଦି (type == VVarType::UNKNOWN) {  // e.g. "output" non-ANSI standalone direction (vs "reg")
        ଯଦି (GRAMMARP->m_varIO.isAny()) {
            type = VVarType::PORT;
        } ଅନ୍ୟଥା {
            fileline->v3fatalSrc("Unknown signal type declared: " << type.ascii());
        }
    }
    ଯଦି (type == VVarType::GENVAR) {
        // Should be impossible as the grammar blocks this, but...
        ଯଦି (arrayp) fileline->v3error("Genvars may not be arrayed: " << name);  // LCOV_EXCL_LINE
    }

    // Split RANGE0-RANGE1-RANGE2 into
    // ARRAYDTYPE0(ARRAYDTYPE1(ARRAYDTYPE2(BASICTYPE3), RANGE), RANGE)
    Ast_ଗଣ୍ଠି_DType* ଅଚଳ arrayDTypep = createArray(dtypep, arrayp, ମିଥ୍ୟା);

    AstVar* ଅଚଳ nodep = ନୂତନ AstVar{fileline, type, name, VFlagChildDType(), arrayDTypep};
    nodep->addAttrsp(attrsp);
    nodep->ansi(m_pinAnsi);
    nodep->declTyped(m_varDeclTyped);
    nodep->lifetime(m_varLifetime);
    ଯଦି (m_netDelayp) nodep->delayp(m_netDelayp->clone_ବୃକ୍ଷ(ମିଥ୍ୟା));
    ଯଦି (GRAMMARP->m_varDecl != VVarType::UNKNOWN) nodep->combineType(GRAMMARP->m_varDecl);
    ଯଦି (GRAMMARP->m_varIO != VDirection::NONE) {
        nodep->declDirection(GRAMMARP->m_varIO);
        nodep->direction(GRAMMARP->m_varIO);
    }

    ଯଦି (GRAMMARP->m_varDecl == VVarType::SUPPLY0) {
        AstAssignW* ଅଚଳ ap = V3ParseGrammar::create_Supply_ପଦସମଷ୍ଟି(fileline, nodep->name(), 0);
        Ast_ଗଣ୍ଠି::addNext<Ast_ଗଣ୍ଠି, Ast_ଗଣ୍ଠି>(nodep, ନୂତନ AstAlways{ap});
    }
    ଯଦି (GRAMMARP->m_varDecl == VVarType::SUPPLY1) {
        AstAssignW* ଅଚଳ ap = V3ParseGrammar::create_Supply_ପଦସମଷ୍ଟି(fileline, nodep->name(), 1);
        Ast_ଗଣ୍ଠି::addNext<Ast_ଗଣ୍ଠି, Ast_ଗଣ୍ଠି>(nodep, ନୂତନ AstAlways{ap});
    }
    ଯଦି (VN_IS(dtypep, ParseTypeDType)) {
        // Parser needs to know what is a type
        Ast_ଗଣ୍ଠି* ଅଚଳ newp = ନୂତନ AstTypedefFwd{fileline, name, VFwdType::NONE};
        Ast_ଗଣ୍ଠି::addNext<Ast_ଗଣ୍ଠି, Ast_ଗଣ୍ଠି>(nodep, newp);
    }
    // Don't set dtypep in the ranging;
    // We need to autosize parameters and integers separately
    //
    // Propagate from current module tracing state
    ଯଦି (nodep->isGenVar()) {
        nodep->trace(ମିଥ୍ୟା);
    } ଅନ୍ୟଥା ଯଦି (nodep->isParam() && !v3Global.opt.traceParams()) {
        nodep->trace(ମିଥ୍ୟା);
    } ଅନ୍ୟଥା {
        nodep->trace(allTracingOn(nodep->fileline()));
    }
    ଯଦି (nodep->varType().isVPIAccessible()) nodep->addAttrsp(GRAMMARP->cloneScopedSigAttr());

    // Remember the last variable created, so we can attach attributes to it in later parsing
    GRAMMARP->m_varAttrp = nodep;
    PARSEP->tagNodep(GRAMMARP->m_varAttrp);
    ଫେରନ୍ତୁ nodep;
}

string V3ParseGrammar::unquoteString(File_ଧାଡ଼ି* fileline, ଅଚଳ std::string& text) {
    string err_ସନ୍ଦେଶ;
    string res = VString::unquoteSVString(text, err_ସନ୍ଦେଶ);
    ଯଦି (!err_ସନ୍ଦେଶ.empty()) fileline->v3error(err_ସନ୍ଦେଶ.c_str());
    ଫେରନ୍ତୁ res;
}

ପୂର୍ଣ୍ଣସଂଖ୍ୟା V3ParseGrammar::s_type_Imp_ଅଙ୍କ = 0;
