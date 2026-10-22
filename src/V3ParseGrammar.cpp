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

#অন্তর্ভাব "V3Ast.h"  // This must be before V3ParseBison.cpp, as we don't want #defines to conflict

VL_DEFINE_DEBUG_FUNCTIONS;

//======================================================================
// The guts come from bison output

#অন্তর্ভাব "V3ParseBison.c"

//======================================================================
// V3ParseImp functions requiring bison state

পূর্ণসংখ্যা V3ParseImp::bisonParse() {
    // Use --debugi-bison 9 to enable this
    যদি (PARSEP->debugBison() >= 9) yydebug = 1;
    ফেরত yyparse();
}

ধ্রুবক অক্ষর* V3ParseImp::tokenName(পূর্ণসংখ্যা token) {
#যদি YYDEBUG || YYERROR_VERBOSE
    স্থির ধ্রুবক অক্ষর** nameTablep = nullptr;
    যদি (!nameTablep) {
        পূর্ণসংখ্যা আকার_2;
        জন্য (আকার_2 = 0; yytname[আকার_2]; ++আকার_2) {}
        nameTablep = নতুন ধ্রুবক অক্ষর*[আকার_2];
        // Workaround bug in bison's which have '!' in yytname but not token values
        পূর্ণসংখ্যা iout = 0;
        জন্য (পূর্ণসংখ্যা i = 0; yytname[i]; ++i) {
            যদি (yytname[i][0] == '\'') চালু;
            nameTablep[iout++] = yytname[i];
        }
    }
    যদি (token >= 255) {
        ফেরত nameTablep[token - 255];
    } অন্যথা {
        স্থির অক্ষর s_ch[2];
        s_ch[0] = token;
        s_ch[1] = '\0';
        ফেরত s_ch;
    }
#অন্যথা
    ফেরত "";
#endif
}

শূন্য V3ParseImp::candidatePli(VSpellCheck* spellerp) {
#যদি !YYERROR_VERBOSE
#error "Need lex token names"
#endif
    জন্য (পূর্ণসংখ্যা i = 0; yytname[i]; ++i) {
        যদি (yytname[i][0] != '\"') চালু;
        যদি (yytname[i][1] != '$') চালু;
        spellerp->ঠেলো_Candidate(string{yytname[i]}.substr(1, strlen(yytname[i]) - 2));
    }
}

শূন্য V3ParseImp::parserClear() {
    // Clear up any dynamic memory V3Parser required
    VARDTYPE(nullptr);
    GRAMMARP->setNetDelay(nullptr);
    GRAMMARP->setScopedSigAttr(nullptr);
}

//======================================================================
// V3ParseGrammar functions requiring bison state

AstArg* V3ParseGrammar::argWrapList(Ast_গ্রন্থি_রাশিমালা* nodep) {
    // Convert list of expressions to list of arguments
    AstArg* outp = nullptr;
    যখন (nodep) {
        Ast_গ্রন্থি_রাশিমালা* ধ্রুবক nextp = VN_AS(nodep->nextp(), গ্রন্থি_রাশিমালা);
        যদি (nextp) nextp->unlink_Fr_Back_With_পরবর্তী();
        outp = Ast_গ্রন্থি::add_পরবর্তী(outp, নতুন AstArg{nodep->fileline(), "", nodep});
        nodep = nextp;
    }
    ফেরত outp;
}

AstAssignW* V3ParseGrammar::create_Supply_রাশিমালা(File_সারি* fileline, ধ্রুবক string& name, পূর্ণসংখ্যা মান) {
    AstAssignW* assignp = নতুন AstAssignW{fileline, নতুন AstParseRef{fileline, name},
                                         মান ? নতুন AstConst{fileline, AstConst::All1{}}
                                               : নতুন AstConst{fileline, AstConst::All0{}}};
    AstStrengthSpec* strengthSpecp
        = নতুন AstStrengthSpec{fileline, VStrength::SUPPLY, VStrength::SUPPLY};
    assignp->strengthSpecp(strengthSpecp);
    ফেরত assignp;
}

AstRange* V3ParseGrammar::scrubRangeMulti(Ast_গ্রন্থি_Range* nrangep) {
    // Remove any UnsizedRange's from list; preserves multi-dim chain via nextp().
    জন্য (Ast_গ্রন্থি_Range *nodep = nrangep, *nextp; nodep; nodep = nextp) {
        nextp = VN_AS(nodep->nextp(), গ্রন্থি_Range);
        যদি (!VN_IS(nodep, Range)) {
            nodep->v3error(
                "Unsupported or syntax error: Unsized range in instance or other declaration");
            nodep->unlinkFrBack();
            VL_DO_DANGLING(nodep->delete_বৃক্ষ(), nodep);
        }
    }
    ফেরত VN_CAST(nrangep, Range);
}
AstRange* V3ParseGrammar::scrubRange(Ast_গ্রন্থি_Range* nrangep) {
    AstRange* ধ্রুবক rangep = scrubRangeMulti(nrangep);
    যদি (rangep && rangep->nextp()) {
        // Gate primitives only support a single dimension
        rangep->nextp()->v3warn(E_UNSUPPORTED, "Unsupported: Multidimensional gate instances.");
        rangep->nextp()->unlink_Fr_Back_With_পরবর্তী()->delete_বৃক্ষ();
    }
    ফেরত rangep;
}

Ast_গ্রন্থি_Pre_Sel* V3ParseGrammar::scrubSel(Ast_গ্রন্থি_রাশিমালা* fromp, Ast_গ্রন্থি_Pre_Sel* selp) VL_MT_DISABLED {
    // SEL(PARSELVALUE, ...) -> SEL(fromp, ...)
    Ast_গ্রন্থি_Pre_Sel* subSelp = selp;
    যখন (সত্য) {
        যদি (VN_IS(subSelp->fromp(), ParseHolder)) বিরতি;
        যদি (Ast_গ্রন্থি_Pre_Sel* ধ্রুবক lowerSelp = VN_CAST(subSelp->fromp(), গ্রন্থি_Pre_Sel)) {
            subSelp = lowerSelp;
            চালু;
        }
        subSelp->v3fatalSrc("Couldn't find where to insert expression into select");
    }
    Ast_গ্রন্থি* subSelFromp = subSelp->fromp();
    subSelFromp->replaceWith(fromp);
    VL_DO_DANGLING(subSelFromp->delete_বৃক্ষ(), subSelFromp);
    ফেরত selp;
}

Ast_গ্রন্থি_DType* V3ParseGrammar::createArray(Ast_গ্রন্থি_DType* basep, Ast_গ্রন্থি_Range* nrangep,
                                          যৌক্তিক isPacked) {
    // Split RANGE0-RANGE1-RANGE2
    // into ARRAYDTYPE0(ARRAYDTYPE1(ARRAYDTYPE2(BASICTYPE3), RANGE), RANGE)
    Ast_গ্রন্থি_DType* arrayp = basep;
    যদি (nrangep) {  // Maybe no range - return unmodified base type
        যখন (nrangep->nextp()) nrangep = VN_AS(nrangep->nextp(), গ্রন্থি_Range);
        যখন (nrangep) {
            Ast_গ্রন্থি_Range* ধ্রুবক prevp = VN_AS(nrangep->backp(), গ্রন্থি_Range);
            যদি (prevp) nrangep->unlinkFrBack();
            AstRange* ধ্রুবক rangep = VN_CAST(nrangep, Range);
            যদি (rangep && isPacked) {
                arrayp
                    = নতুন AstPackArrayDType{rangep->fileline(), VFlagChildDType{}, arrayp, rangep};
            } অন্যথা যদি (rangep
                       && (VN_IS(rangep->leftp(), Unbounded)
                           || VN_IS(rangep->rightp(), Unbounded))) {
                arrayp = নতুন AstQueueDType{nrangep->fileline(), VFlagChildDType{}, arrayp,
                                           rangep->rightp()->clone_বৃক্ষ(সত্য)};
                VL_DO_DANGLING(nrangep->delete_বৃক্ষ(), nrangep);
            } অন্যথা যদি (rangep) {
                arrayp = নতুন AstUnpackArrayDType{rangep->fileline(), VFlagChildDType{}, arrayp,
                                                 rangep};
            } অন্যথা যদি (VN_IS(nrangep, UnsizedRange)) {
                arrayp = নতুন AstDynArrayDType{nrangep->fileline(), VFlagChildDType{}, arrayp};
                VL_DO_DANGLING(nrangep->delete_বৃক্ষ(), nrangep);
            } অন্যথা যদি (VN_IS(nrangep, BracketRange)) {
                ধ্রুবক AstBracketRange* ধ্রুবক arangep = VN_AS(nrangep, BracketRange);
                Ast_গ্রন্থি* ধ্রুবক keyp = arangep->elementsp()->unlinkFrBack();
                arrayp = নতুন AstBracketArrayDType{nrangep->fileline(), VFlagChildDType{}, arrayp,
                                                  keyp};
                VL_DO_DANGLING(nrangep->delete_বৃক্ষ(), nrangep);
            } অন্যথা যদি (VN_IS(nrangep, WildcardRange)) {
                arrayp = নতুন AstWildcardArrayDType{nrangep->fileline(), VFlagChildDType{}, arrayp};
                VL_DO_DANGLING(nrangep->delete_বৃক্ষ(), nrangep);
            } অন্যথা {
                UASSERT_OBJ(0, nrangep, "Expected range or unsized range");
            }
            nrangep = prevp;
        }
    }
    ফেরত arrayp;
}

AstVar* V3ParseGrammar::createVariable(File_সারি* fileline, ধ্রুবক string& name,
                                       Ast_গ্রন্থি_Range* arrayp, Ast_গ্রন্থি* attrsp) {
    UINFO(5, "  creVar " << name << "  decl=" << GRAMMARP->m_varDecl << "  io="
                         << GRAMMARP->m_varIO << "  dt=" << (GRAMMARP->m_varDTypep ? "set" : ""));
    যদি (GRAMMARP->m_varIO == VDirection::NONE  // In non-ANSI port list
        && GRAMMARP->m_varDecl == VVarType::PORT) {
        // Just a port list with variable name (not v2k format); AstPort already created
        যদি (arrayp) VL_DO_DANGLING(arrayp->delete_বৃক্ষ(), arrayp);
        যদি (attrsp) {
            // TODO: Merge attributes across list? Or warn attribute is ignored
            VL_DO_DANGLING(attrsp->delete_বৃক্ষ(), attrsp);
        }
        ফেরত nullptr;
    }

    Ast_গ্রন্থি_DType* ধ্রুবক dtypep = [&]() -> Ast_গ্রন্থি_DType* {
        যদি (GRAMMARP->m_varDecl == VVarType::WREAL) {
            // dtypep might not be null, might be implicit LOGIC before we knew better
            ফেরত নতুন AstBasicDType{fileline, VBasicDTypeKwd::DOUBLE};
        }
        যদি (GRAMMARP->m_varDTypep) {
            // May make new variables with same type, so clone
            ফেরত GRAMMARP->m_varDTypep->clone_বৃক্ষ(মিথ্যা);
        }
        // Created implicitly
        যদি (m_insideProperty) {
            যদি (m_typedPropertyPort) {
                fileline->v3warn(E_UNSUPPORTED, "Untyped property port following a typed port");
            }
            ফেরত নতুন AstBasicDType{fileline, VBasicDTypeKwd::UNTYPED};
        }
        ফেরত নতুন AstBasicDType{fileline, LOGIC_IMPLICIT};
    }();

    // UINFO(0,"CREVAR "<<fileline->ascii()<<" decl="<<GRAMMARP->m_varDecl.ascii()<<"
    // io="<<GRAMMARP->m_varIO.ascii()<<endl);
    VVarType type = GRAMMARP->m_varDecl;
    যদি (type == VVarType::UNKNOWN) {  // e.g. "output" non-ANSI standalone direction (vs "reg")
        যদি (GRAMMARP->m_varIO.isAny()) {
            type = VVarType::PORT;
        } অন্যথা {
            fileline->v3fatalSrc("Unknown signal type declared: " << type.ascii());
        }
    }
    যদি (type == VVarType::GENVAR) {
        // Should be impossible as the grammar blocks this, but...
        যদি (arrayp) fileline->v3error("Genvars may not be arrayed: " << name);  // LCOV_EXCL_LINE
    }

    // Split RANGE0-RANGE1-RANGE2 into
    // ARRAYDTYPE0(ARRAYDTYPE1(ARRAYDTYPE2(BASICTYPE3), RANGE), RANGE)
    Ast_গ্রন্থি_DType* ধ্রুবক arrayDTypep = createArray(dtypep, arrayp, মিথ্যা);

    AstVar* ধ্রুবক nodep = নতুন AstVar{fileline, type, name, VFlagChildDType(), arrayDTypep};
    nodep->addAttrsp(attrsp);
    nodep->ansi(m_pinAnsi);
    nodep->declTyped(m_varDeclTyped);
    nodep->lifetime(m_varLifetime);
    যদি (m_netDelayp) nodep->delayp(m_netDelayp->clone_বৃক্ষ(মিথ্যা));
    যদি (GRAMMARP->m_varDecl != VVarType::UNKNOWN) nodep->combineType(GRAMMARP->m_varDecl);
    যদি (GRAMMARP->m_varIO != VDirection::NONE) {
        nodep->declDirection(GRAMMARP->m_varIO);
        nodep->direction(GRAMMARP->m_varIO);
    }

    যদি (GRAMMARP->m_varDecl == VVarType::SUPPLY0) {
        AstAssignW* ধ্রুবক ap = V3ParseGrammar::create_Supply_রাশিমালা(fileline, nodep->name(), 0);
        Ast_গ্রন্থি::add_পরবর্তী<Ast_গ্রন্থি, Ast_গ্রন্থি>(nodep, নতুন AstAlways{ap});
    }
    যদি (GRAMMARP->m_varDecl == VVarType::SUPPLY1) {
        AstAssignW* ধ্রুবক ap = V3ParseGrammar::create_Supply_রাশিমালা(fileline, nodep->name(), 1);
        Ast_গ্রন্থি::add_পরবর্তী<Ast_গ্রন্থি, Ast_গ্রন্থি>(nodep, নতুন AstAlways{ap});
    }
    যদি (VN_IS(dtypep, ParseTypeDType)) {
        // Parser needs to know what is a type
        Ast_গ্রন্থি* ধ্রুবক newp = নতুন AstTypedefFwd{fileline, name, VFwdType::NONE};
        Ast_গ্রন্থি::add_পরবর্তী<Ast_গ্রন্থি, Ast_গ্রন্থি>(nodep, newp);
    }
    // Don't set dtypep in the ranging;
    // We need to autosize parameters and integers separately
    //
    // Propagate from current module tracing state
    যদি (nodep->isGenVar()) {
        nodep->trace(মিথ্যা);
    } অন্যথা যদি (nodep->isParam() && !v3Global.opt.traceParams()) {
        nodep->trace(মিথ্যা);
    } অন্যথা {
        nodep->trace(allTracingOn(nodep->fileline()));
    }
    যদি (nodep->varType().isVPIAccessible()) nodep->addAttrsp(GRAMMARP->cloneScopedSigAttr());

    // Remember the last variable created, so we can attach attributes to it in later parsing
    GRAMMARP->m_varAttrp = nodep;
    PARSEP->tagNodep(GRAMMARP->m_varAttrp);
    ফেরত nodep;
}

string V3ParseGrammar::unquoteString(File_সারি* fileline, ধ্রুবক std::string& text) {
    string err_বার্তা;
    string res = VString::unquoteSVString(text, err_বার্তা);
    যদি (!err_বার্তা.empty()) fileline->v3error(err_বার্তা.c_str());
    ফেরত res;
}

পূর্ণসংখ্যা V3ParseGrammar::s_type_Imp_অঙ্ক = 0;
