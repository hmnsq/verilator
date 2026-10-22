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

#ضمن "V3Ast.h"  // This must be before V3ParseBison.cpp, as we don't want #defines to conflict

VL_DEFINE_DEBUG_FUNCTIONS;

//======================================================================
// The guts come from bison output

#ضمن "V3ParseBison.c"

//======================================================================
// V3ParseImp functions requiring bison state

صحيح V3ParseImp::bisonParse() {
    // Use --debugi-bison 9 to enable this
    إذا (PARSEP->debugBison() >= 9) yydebug = 1;
    إرجاع yyparse();
}

ثابت حرف* V3ParseImp::tokenName(صحيح token) {
#إذا YYDEBUG || YYERROR_VERBOSE
    ساكن ثابت حرف** nameTablep = nullptr;
    إذا (!nameTablep) {
        صحيح size;
        لكل (size = 0; yytname[size]; ++size) {}
        nameTablep = جديد ثابت حرف*[size];
        // Workaround bug in bison's which have '!' in yytname but not token values
        صحيح iout = 0;
        لكل (صحيح i = 0; yytname[i]; ++i) {
            إذا (yytname[i][0] == '\'') استمر;
            nameTablep[iout++] = yytname[i];
        }
    }
    إذا (token >= 255) {
        إرجاع nameTablep[token - 255];
    } وإلا {
        ساكن حرف s_ch[2];
        s_ch[0] = token;
        s_ch[1] = '\0';
        إرجاع s_ch;
    }
#وإلا
    إرجاع "";
#endif
}

فراغ V3ParseImp::candidatePli(VSpellCheck* spellerp) {
#إذا !YYERROR_VERBOSE
#error "Need lex token names"
#endif
    لكل (صحيح i = 0; yytname[i]; ++i) {
        إذا (yytname[i][0] != '\"') استمر;
        إذا (yytname[i][1] != '$') استمر;
        spellerp->ادفع_Candidate(string{yytname[i]}.substr(1, strlen(yytname[i]) - 2));
    }
}

فراغ V3ParseImp::parserClear() {
    // Clear up any dynamic memory V3Parser required
    VARDTYPE(nullptr);
    GRAMMARP->setNetDelay(nullptr);
    GRAMMARP->setScopedSigAttr(nullptr);
}

//======================================================================
// V3ParseGrammar functions requiring bison state

AstArg* V3ParseGrammar::argWrapList(Ast_عقدة_تعبير* nodep) {
    // Convert list of expressions to list of arguments
    AstArg* outp = nullptr;
    بينما (nodep) {
        Ast_عقدة_تعبير* ثابت nextp = VN_AS(nodep->nextp(), عقدة_تعبير);
        إذا (nextp) nextp->unlinkFrBackWithNext();
        outp = Ast_عقدة::addNext(outp, جديد AstArg{nodep->fileline(), "", nodep});
        nodep = nextp;
    }
    إرجاع outp;
}

AstAssignW* V3ParseGrammar::create_Supply_تعبير(File_سطر* fileline, ثابت string& name, صحيح قيمة) {
    AstAssignW* assignp = جديد AstAssignW{fileline, جديد AstParseRef{fileline, name},
                                         قيمة ? جديد AstConst{fileline, AstConst::All1{}}
                                               : جديد AstConst{fileline, AstConst::All0{}}};
    AstStrengthSpec* strengthSpecp
        = جديد AstStrengthSpec{fileline, VStrength::SUPPLY, VStrength::SUPPLY};
    assignp->strengthSpecp(strengthSpecp);
    إرجاع assignp;
}

AstRange* V3ParseGrammar::scrubRangeMulti(Ast_عقدة_Range* nrangep) {
    // Remove any UnsizedRange's from list; preserves multi-dim chain via nextp().
    لكل (Ast_عقدة_Range *nodep = nrangep, *nextp; nodep; nodep = nextp) {
        nextp = VN_AS(nodep->nextp(), عقدة_Range);
        إذا (!VN_IS(nodep, Range)) {
            nodep->v3error(
                "Unsupported or syntax error: Unsized range in instance or other declaration");
            nodep->unlinkFrBack();
            VL_DO_DANGLING(nodep->delete_شجرة(), nodep);
        }
    }
    إرجاع VN_CAST(nrangep, Range);
}
AstRange* V3ParseGrammar::scrubRange(Ast_عقدة_Range* nrangep) {
    AstRange* ثابت rangep = scrubRangeMulti(nrangep);
    إذا (rangep && rangep->nextp()) {
        // Gate primitives only support a single dimension
        rangep->nextp()->v3warn(E_UNSUPPORTED, "Unsupported: Multidimensional gate instances.");
        rangep->nextp()->unlinkFrBackWithNext()->delete_شجرة();
    }
    إرجاع rangep;
}

Ast_عقدة_Pre_Sel* V3ParseGrammar::scrubSel(Ast_عقدة_تعبير* fromp, Ast_عقدة_Pre_Sel* selp) VL_MT_DISABLED {
    // SEL(PARSELVALUE, ...) -> SEL(fromp, ...)
    Ast_عقدة_Pre_Sel* subSelp = selp;
    بينما (true) {
        إذا (VN_IS(subSelp->fromp(), ParseHolder)) اكسر;
        إذا (Ast_عقدة_Pre_Sel* ثابت lowerSelp = VN_CAST(subSelp->fromp(), عقدة_Pre_Sel)) {
            subSelp = lowerSelp;
            استمر;
        }
        subSelp->v3fatalSrc("Couldn't find where to insert expression into select");
    }
    Ast_عقدة* subSelFromp = subSelp->fromp();
    subSelFromp->replaceWith(fromp);
    VL_DO_DANGLING(subSelFromp->delete_شجرة(), subSelFromp);
    إرجاع selp;
}

Ast_عقدة_DType* V3ParseGrammar::createArray(Ast_عقدة_DType* basep, Ast_عقدة_Range* nrangep,
                                          منطقي isPacked) {
    // Split RANGE0-RANGE1-RANGE2
    // into ARRAYDTYPE0(ARRAYDTYPE1(ARRAYDTYPE2(BASICTYPE3), RANGE), RANGE)
    Ast_عقدة_DType* arrayp = basep;
    إذا (nrangep) {  // Maybe no range - return unmodified base type
        بينما (nrangep->nextp()) nrangep = VN_AS(nrangep->nextp(), عقدة_Range);
        بينما (nrangep) {
            Ast_عقدة_Range* ثابت prevp = VN_AS(nrangep->backp(), عقدة_Range);
            إذا (prevp) nrangep->unlinkFrBack();
            AstRange* ثابت rangep = VN_CAST(nrangep, Range);
            إذا (rangep && isPacked) {
                arrayp
                    = جديد AstPackArrayDType{rangep->fileline(), VFlagChildDType{}, arrayp, rangep};
            } وإلا إذا (rangep
                       && (VN_IS(rangep->leftp(), Unbounded)
                           || VN_IS(rangep->rightp(), Unbounded))) {
                arrayp = جديد AstQueueDType{nrangep->fileline(), VFlagChildDType{}, arrayp,
                                           rangep->rightp()->clone_شجرة(true)};
                VL_DO_DANGLING(nrangep->delete_شجرة(), nrangep);
            } وإلا إذا (rangep) {
                arrayp = جديد AstUnpackArrayDType{rangep->fileline(), VFlagChildDType{}, arrayp,
                                                 rangep};
            } وإلا إذا (VN_IS(nrangep, UnsizedRange)) {
                arrayp = جديد AstDynArrayDType{nrangep->fileline(), VFlagChildDType{}, arrayp};
                VL_DO_DANGLING(nrangep->delete_شجرة(), nrangep);
            } وإلا إذا (VN_IS(nrangep, BracketRange)) {
                ثابت AstBracketRange* ثابت arangep = VN_AS(nrangep, BracketRange);
                Ast_عقدة* ثابت keyp = arangep->elementsp()->unlinkFrBack();
                arrayp = جديد AstBracketArrayDType{nrangep->fileline(), VFlagChildDType{}, arrayp,
                                                  keyp};
                VL_DO_DANGLING(nrangep->delete_شجرة(), nrangep);
            } وإلا إذا (VN_IS(nrangep, WildcardRange)) {
                arrayp = جديد AstWildcardArrayDType{nrangep->fileline(), VFlagChildDType{}, arrayp};
                VL_DO_DANGLING(nrangep->delete_شجرة(), nrangep);
            } وإلا {
                UASSERT_OBJ(0, nrangep, "Expected range or unsized range");
            }
            nrangep = prevp;
        }
    }
    إرجاع arrayp;
}

AstVar* V3ParseGrammar::createVariable(File_سطر* fileline, ثابت string& name,
                                       Ast_عقدة_Range* arrayp, Ast_عقدة* attrsp) {
    UINFO(5, "  creVar " << name << "  decl=" << GRAMMARP->m_varDecl << "  io="
                         << GRAMMARP->m_varIO << "  dt=" << (GRAMMARP->m_varDTypep ? "set" : ""));
    إذا (GRAMMARP->m_varIO == VDirection::NONE  // In non-ANSI port list
        && GRAMMARP->m_varDecl == VVarType::PORT) {
        // Just a port list with variable name (not v2k format); AstPort already created
        إذا (arrayp) VL_DO_DANGLING(arrayp->delete_شجرة(), arrayp);
        إذا (attrsp) {
            // TODO: Merge attributes across list? Or warn attribute is ignored
            VL_DO_DANGLING(attrsp->delete_شجرة(), attrsp);
        }
        إرجاع nullptr;
    }

    Ast_عقدة_DType* ثابت dtypep = [&]() -> Ast_عقدة_DType* {
        إذا (GRAMMARP->m_varDecl == VVarType::WREAL) {
            // dtypep might not be null, might be implicit LOGIC before we knew better
            إرجاع جديد AstBasicDType{fileline, VBasicDTypeKwd::DOUBLE};
        }
        إذا (GRAMMARP->m_varDTypep) {
            // May make new variables with same type, so clone
            إرجاع GRAMMARP->m_varDTypep->clone_شجرة(خطأ);
        }
        // Created implicitly
        إذا (m_insideProperty) {
            إذا (m_typedPropertyPort) {
                fileline->v3warn(E_UNSUPPORTED, "Untyped property port following a typed port");
            }
            إرجاع جديد AstBasicDType{fileline, VBasicDTypeKwd::UNTYPED};
        }
        إرجاع جديد AstBasicDType{fileline, LOGIC_IMPLICIT};
    }();

    // UINFO(0,"CREVAR "<<fileline->ascii()<<" decl="<<GRAMMARP->m_varDecl.ascii()<<"
    // io="<<GRAMMARP->m_varIO.ascii()<<endl);
    VVarType type = GRAMMARP->m_varDecl;
    إذا (type == VVarType::UNKNOWN) {  // e.g. "output" non-ANSI standalone direction (vs "reg")
        إذا (GRAMMARP->m_varIO.isAny()) {
            type = VVarType::PORT;
        } وإلا {
            fileline->v3fatalSrc("Unknown signal type declared: " << type.ascii());
        }
    }
    إذا (type == VVarType::GENVAR) {
        // Should be impossible as the grammar blocks this, but...
        إذا (arrayp) fileline->v3error("Genvars may not be arrayed: " << name);  // LCOV_EXCL_LINE
    }

    // Split RANGE0-RANGE1-RANGE2 into
    // ARRAYDTYPE0(ARRAYDTYPE1(ARRAYDTYPE2(BASICTYPE3), RANGE), RANGE)
    Ast_عقدة_DType* ثابت arrayDTypep = createArray(dtypep, arrayp, خطأ);

    AstVar* ثابت nodep = جديد AstVar{fileline, type, name, VFlagChildDType(), arrayDTypep};
    nodep->addAttrsp(attrsp);
    nodep->ansi(m_pinAnsi);
    nodep->declTyped(m_varDeclTyped);
    nodep->lifetime(m_varLifetime);
    إذا (m_netDelayp) nodep->delayp(m_netDelayp->clone_شجرة(خطأ));
    إذا (GRAMMARP->m_varDecl != VVarType::UNKNOWN) nodep->combineType(GRAMMARP->m_varDecl);
    إذا (GRAMMARP->m_varIO != VDirection::NONE) {
        nodep->declDirection(GRAMMARP->m_varIO);
        nodep->direction(GRAMMARP->m_varIO);
    }

    إذا (GRAMMARP->m_varDecl == VVarType::SUPPLY0) {
        AstAssignW* ثابت ap = V3ParseGrammar::create_Supply_تعبير(fileline, nodep->name(), 0);
        Ast_عقدة::addNext<Ast_عقدة, Ast_عقدة>(nodep, جديد AstAlways{ap});
    }
    إذا (GRAMMARP->m_varDecl == VVarType::SUPPLY1) {
        AstAssignW* ثابت ap = V3ParseGrammar::create_Supply_تعبير(fileline, nodep->name(), 1);
        Ast_عقدة::addNext<Ast_عقدة, Ast_عقدة>(nodep, جديد AstAlways{ap});
    }
    إذا (VN_IS(dtypep, ParseTypeDType)) {
        // Parser needs to know what is a type
        Ast_عقدة* ثابت newp = جديد AstTypedefFwd{fileline, name, VFwdType::NONE};
        Ast_عقدة::addNext<Ast_عقدة, Ast_عقدة>(nodep, newp);
    }
    // Don't set dtypep in the ranging;
    // We need to autosize parameters and integers separately
    //
    // Propagate from current module tracing state
    إذا (nodep->isGenVar()) {
        nodep->trace(خطأ);
    } وإلا إذا (nodep->isParam() && !v3Global.opt.traceParams()) {
        nodep->trace(خطأ);
    } وإلا {
        nodep->trace(allTracingOn(nodep->fileline()));
    }
    إذا (nodep->varType().isVPIAccessible()) nodep->addAttrsp(GRAMMARP->cloneScopedSigAttr());

    // Remember the last variable created, so we can attach attributes to it in later parsing
    GRAMMARP->m_varAttrp = nodep;
    PARSEP->tagNodep(GRAMMARP->m_varAttrp);
    إرجاع nodep;
}

string V3ParseGrammar::unquoteString(File_سطر* fileline, ثابت std::string& text) {
    string err_رسالة;
    string res = VString::unquoteSVString(text, err_رسالة);
    إذا (!err_رسالة.empty()) fileline->v3error(err_رسالة.c_str());
    إرجاع res;
}

صحيح V3ParseGrammar::s_type_Imp_رقم = 0;
