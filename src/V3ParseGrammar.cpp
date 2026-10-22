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

পূর্ণাঙ্ক V3ParseImp::bisonParse() {
    // Use --debugi-bison 9 to enable this
    যদি (PARSEP->debugBison() >= 9) yydebug = 1;
    প্রত্যাগম yyparse();
}

ধ্রুব বর্ণ* V3ParseImp::tokenName(পূর্ণাঙ্ক token) {
#যদি YYDEBUG || YYERROR_VERBOSE
    স্থির ধ্রুব বর্ণ** nameTablep = nullptr;
    যদি (!nameTablep) {
        পূর্ণাঙ্ক size;
        প্রতি (size = 0; yytname[size]; ++size) {}
        nameTablep = নূতন ধ্রুব বর্ণ*[size];
        // Workaround bug in bison's which have '!' in yytname but not token values
        পূর্ণাঙ্ক iout = 0;
        প্রতি (পূর্ণাঙ্ক i = 0; yytname[i]; ++i) {
            যদি (yytname[i][0] == '\'') অনুবর্ত;
            nameTablep[iout++] = yytname[i];
        }
    }
    যদি (token >= 255) {
        প্রত্যাগম nameTablep[token - 255];
    } অন্যথা {
        স্থির বর্ণ s_ch[2];
        s_ch[0] = token;
        s_ch[1] = '\0';
        প্রত্যাগম s_ch;
    }
#অন্যথা
    প্রত্যাগম "";
#endif
}

শূন্য V3ParseImp::candidatePli(VSpellCheck* spellerp) {
#যদি !YYERROR_VERBOSE
#error "Need lex token names"
#endif
    প্রতি (পূর্ণাঙ্ক i = 0; yytname[i]; ++i) {
        যদি (yytname[i][0] != '\"') অনুবর্ত;
        যদি (yytname[i][1] != '$') অনুবর্ত;
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
    যাবত্ (nodep) {
        Ast_গ্রন্থি_রাশিমালা* ধ্রুব nextp = VN_AS(nodep->nextp(), গ্রন্থি_রাশিমালা);
        যদি (nextp) nextp->unlinkFrBackWithNext();
        outp = Ast_গ্রন্থি::addNext(outp, নূতন AstArg{nodep->fileline(), "", nodep});
        nodep = nextp;
    }
    প্রত্যাগম outp;
}

AstAssignW* V3ParseGrammar::create_Supply_রাশিমালা(File_সারি* fileline, ধ্রুব string& name, পূর্ণাঙ্ক মান) {
    AstAssignW* assignp = নূতন AstAssignW{fileline, নূতন AstParseRef{fileline, name},
                                         মান ? নূতন AstConst{fileline, AstConst::All1{}}
                                               : নূতন AstConst{fileline, AstConst::All0{}}};
    AstStrengthSpec* strengthSpecp
        = নূতন AstStrengthSpec{fileline, VStrength::SUPPLY, VStrength::SUPPLY};
    assignp->strengthSpecp(strengthSpecp);
    প্রত্যাগম assignp;
}

AstRange* V3ParseGrammar::scrubRangeMulti(Ast_গ্রন্থি_Range* nrangep) {
    // Remove any UnsizedRange's from list; preserves multi-dim chain via nextp().
    প্রতি (Ast_গ্রন্থি_Range *nodep = nrangep, *nextp; nodep; nodep = nextp) {
        nextp = VN_AS(nodep->nextp(), গ্রন্থি_Range);
        যদি (!VN_IS(nodep, Range)) {
            nodep->v3error(
                "Unsupported or syntax error: Unsized range in instance or other declaration");
            nodep->unlinkFrBack();
            VL_DO_DANGLING(nodep->delete_বৃক্ষ(), nodep);
        }
    }
    প্রত্যাগম VN_CAST(nrangep, Range);
}
AstRange* V3ParseGrammar::scrubRange(Ast_গ্রন্থি_Range* nrangep) {
    AstRange* ধ্রুব rangep = scrubRangeMulti(nrangep);
    যদি (rangep && rangep->nextp()) {
        // Gate primitives only support a single dimension
        rangep->nextp()->v3warn(E_UNSUPPORTED, "Unsupported: Multidimensional gate instances.");
        rangep->nextp()->unlinkFrBackWithNext()->delete_বৃক্ষ();
    }
    প্রত্যাগম rangep;
}

Ast_গ্রন্থি_Pre_Sel* V3ParseGrammar::scrubSel(Ast_গ্রন্থি_রাশিমালা* fromp, Ast_গ্রন্থি_Pre_Sel* selp) VL_MT_DISABLED {
    // SEL(PARSELVALUE, ...) -> SEL(fromp, ...)
    Ast_গ্রন্থি_Pre_Sel* subSelp = selp;
    যাবত্ (সত্য) {
        যদি (VN_IS(subSelp->fromp(), ParseHolder)) ভঙ্গ;
        যদি (Ast_গ্রন্থি_Pre_Sel* ধ্রুব lowerSelp = VN_CAST(subSelp->fromp(), গ্রন্থি_Pre_Sel)) {
            subSelp = lowerSelp;
            অনুবর্ত;
        }
        subSelp->v3fatalSrc("Couldn't find where to insert expression into select");
    }
    Ast_গ্রন্থি* subSelFromp = subSelp->fromp();
    subSelFromp->replaceWith(fromp);
    VL_DO_DANGLING(subSelFromp->delete_বৃক্ষ(), subSelFromp);
    প্রত্যাগম selp;
}

Ast_গ্রন্থি_DType* V3ParseGrammar::createArray(Ast_গ্রন্থি_DType* basep, Ast_গ্রন্থি_Range* nrangep,
                                          তার্কিক isPacked) {
    // Split RANGE0-RANGE1-RANGE2
    // into ARRAYDTYPE0(ARRAYDTYPE1(ARRAYDTYPE2(BASICTYPE3), RANGE), RANGE)
    Ast_গ্রন্থি_DType* arrayp = basep;
    যদি (nrangep) {  // Maybe no range - return unmodified base type
        যাবত্ (nrangep->nextp()) nrangep = VN_AS(nrangep->nextp(), গ্রন্থি_Range);
        যাবত্ (nrangep) {
            Ast_গ্রন্থি_Range* ধ্রুব prevp = VN_AS(nrangep->backp(), গ্রন্থি_Range);
            যদি (prevp) nrangep->unlinkFrBack();
            AstRange* ধ্রুব rangep = VN_CAST(nrangep, Range);
            যদি (rangep && isPacked) {
                arrayp
                    = নূতন AstPackArrayDType{rangep->fileline(), VFlagChildDType{}, arrayp, rangep};
            } অন্যথা যদি (rangep
                       && (VN_IS(rangep->leftp(), Unbounded)
                           || VN_IS(rangep->rightp(), Unbounded))) {
                arrayp = নূতন AstQueueDType{nrangep->fileline(), VFlagChildDType{}, arrayp,
                                           rangep->rightp()->clone_বৃক্ষ(সত্য)};
                VL_DO_DANGLING(nrangep->delete_বৃক্ষ(), nrangep);
            } অন্যথা যদি (rangep) {
                arrayp = নূতন AstUnpackArrayDType{rangep->fileline(), VFlagChildDType{}, arrayp,
                                                 rangep};
            } অন্যথা যদি (VN_IS(nrangep, UnsizedRange)) {
                arrayp = নূতন AstDynArrayDType{nrangep->fileline(), VFlagChildDType{}, arrayp};
                VL_DO_DANGLING(nrangep->delete_বৃক্ষ(), nrangep);
            } অন্যথা যদি (VN_IS(nrangep, BracketRange)) {
                ধ্রুব AstBracketRange* ধ্রুব arangep = VN_AS(nrangep, BracketRange);
                Ast_গ্রন্থি* ধ্রুব keyp = arangep->elementsp()->unlinkFrBack();
                arrayp = নূতন AstBracketArrayDType{nrangep->fileline(), VFlagChildDType{}, arrayp,
                                                  keyp};
                VL_DO_DANGLING(nrangep->delete_বৃক্ষ(), nrangep);
            } অন্যথা যদি (VN_IS(nrangep, WildcardRange)) {
                arrayp = নূতন AstWildcardArrayDType{nrangep->fileline(), VFlagChildDType{}, arrayp};
                VL_DO_DANGLING(nrangep->delete_বৃক্ষ(), nrangep);
            } অন্যথা {
                UASSERT_OBJ(0, nrangep, "Expected range or unsized range");
            }
            nrangep = prevp;
        }
    }
    প্রত্যাগম arrayp;
}

AstVar* V3ParseGrammar::createVariable(File_সারি* fileline, ধ্রুব string& name,
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
        প্রত্যাগম nullptr;
    }

    Ast_গ্রন্থি_DType* ধ্রুব dtypep = [&]() -> Ast_গ্রন্থি_DType* {
        যদি (GRAMMARP->m_varDecl == VVarType::WREAL) {
            // dtypep might not be null, might be implicit LOGIC before we knew better
            প্রত্যাগম নূতন AstBasicDType{fileline, VBasicDTypeKwd::DOUBLE};
        }
        যদি (GRAMMARP->m_varDTypep) {
            // May make new variables with same type, so clone
            প্রত্যাগম GRAMMARP->m_varDTypep->clone_বৃক্ষ(অসত্য);
        }
        // Created implicitly
        যদি (m_insideProperty) {
            যদি (m_typedPropertyPort) {
                fileline->v3warn(E_UNSUPPORTED, "Untyped property port following a typed port");
            }
            প্রত্যাগম নূতন AstBasicDType{fileline, VBasicDTypeKwd::UNTYPED};
        }
        প্রত্যাগম নূতন AstBasicDType{fileline, LOGIC_IMPLICIT};
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
    Ast_গ্রন্থি_DType* ধ্রুব arrayDTypep = createArray(dtypep, arrayp, অসত্য);

    AstVar* ধ্রুব nodep = নূতন AstVar{fileline, type, name, VFlagChildDType(), arrayDTypep};
    nodep->addAttrsp(attrsp);
    nodep->ansi(m_pinAnsi);
    nodep->declTyped(m_varDeclTyped);
    nodep->lifetime(m_varLifetime);
    যদি (m_netDelayp) nodep->delayp(m_netDelayp->clone_বৃক্ষ(অসত্য));
    যদি (GRAMMARP->m_varDecl != VVarType::UNKNOWN) nodep->combineType(GRAMMARP->m_varDecl);
    যদি (GRAMMARP->m_varIO != VDirection::NONE) {
        nodep->declDirection(GRAMMARP->m_varIO);
        nodep->direction(GRAMMARP->m_varIO);
    }

    যদি (GRAMMARP->m_varDecl == VVarType::SUPPLY0) {
        AstAssignW* ধ্রুব ap = V3ParseGrammar::create_Supply_রাশিমালা(fileline, nodep->name(), 0);
        Ast_গ্রন্থি::addNext<Ast_গ্রন্থি, Ast_গ্রন্থি>(nodep, নূতন AstAlways{ap});
    }
    যদি (GRAMMARP->m_varDecl == VVarType::SUPPLY1) {
        AstAssignW* ধ্রুব ap = V3ParseGrammar::create_Supply_রাশিমালা(fileline, nodep->name(), 1);
        Ast_গ্রন্থি::addNext<Ast_গ্রন্থি, Ast_গ্রন্থি>(nodep, নূতন AstAlways{ap});
    }
    যদি (VN_IS(dtypep, ParseTypeDType)) {
        // Parser needs to know what is a type
        Ast_গ্রন্থি* ধ্রুব newp = নূতন AstTypedefFwd{fileline, name, VFwdType::NONE};
        Ast_গ্রন্থি::addNext<Ast_গ্রন্থি, Ast_গ্রন্থি>(nodep, newp);
    }
    // Don't set dtypep in the ranging;
    // We need to autosize parameters and integers separately
    //
    // Propagate from current module tracing state
    যদি (nodep->isGenVar()) {
        nodep->trace(অসত্য);
    } অন্যথা যদি (nodep->isParam() && !v3Global.opt.traceParams()) {
        nodep->trace(অসত্য);
    } অন্যথা {
        nodep->trace(allTracingOn(nodep->fileline()));
    }
    যদি (nodep->varType().isVPIAccessible()) nodep->addAttrsp(GRAMMARP->cloneScopedSigAttr());

    // Remember the last variable created, so we can attach attributes to it in later parsing
    GRAMMARP->m_varAttrp = nodep;
    PARSEP->tagNodep(GRAMMARP->m_varAttrp);
    প্রত্যাগম nodep;
}

string V3ParseGrammar::unquoteString(File_সারি* fileline, ধ্রুব std::string& text) {
    string err_বার্তা;
    string res = VString::unquoteSVString(text, err_বার্তা);
    যদি (!err_বার্তা.empty()) fileline->v3error(err_বার্তা.c_str());
    প্রত্যাগম res;
}

পূর্ণাঙ্ক V3ParseGrammar::s_type_Imp_অঙ্ক = 0;
