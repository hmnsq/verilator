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

#تعریف_کرو YYDEBUG 1  // Nicer errors

#تعریف_کرو VL_MT_DISABLED_کوڈ_UNIT 1

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
    أرجع yyparse();
}

ثابت حرف* V3ParseImp::tokenName(صحيح token) {
#إذا YYDEBUG || YYERROR_VERBOSE
    ساكن ثابت حرف** nameTablep = خالی_سوچک;
    إذا (!nameTablep) {
        صحيح ناپ;
        لكل (ناپ = 0; yytname[ناپ]; ++ناپ) {}
        nameTablep = جديد ثابت حرف*[ناپ];
        // Workaround bug in bison's which have '!' in yytname but not token values
        صحيح iout = 0;
        لكل (صحيح i = 0; yytname[i]; ++i) {
            إذا (yytname[i][0] == '\'') استمر;
            nameTablep[iout++] = yytname[i];
        }
    }
    إذا (token >= 255) {
        أرجع nameTablep[token - 255];
    } وإلا {
        ساكن حرف s_ch[2];
        s_ch[0] = token;
        s_ch[1] = '\0';
        أرجع s_ch;
    }
#وإلا
    أرجع "";
#جے_انت
}

فراغ V3ParseImp::candidatePli(VSpellCheck* spellerp) {
#إذا !YYERROR_VERBOSE
#غلطی "Need lex token names"
#جے_انت
    لكل (صحيح i = 0; yytname[i]; ++i) {
        إذا (yytname[i][0] != '\"') استمر;
        إذا (yytname[i][1] != '$') استمر;
        spellerp->ڈالو_Candidate(string{yytname[i]}.substr(1, لڑی_لمبائی(yytname[i]) - 2));
    }
}

فراغ V3ParseImp::parserClear() {
    // Clear up any dynamic memory V3Parser required
    VARDTYPE(خالی_سوچک);
    GRAMMARP->سیٹ_Net_Delay(خالی_سوچک);
    GRAMMARP->سیٹ_Scoped_Sig_Attr(خالی_سوچک);
}

//======================================================================
// V3ParseGrammar functions requiring bison state

AstArg* V3ParseGrammar::argWrapList(Ast_گرہ_اظہاریہ* nodep) {
    // Convert list of expressions to list of arguments
    AstArg* outp = خالی_سوچک;
    طالما (nodep) {
        Ast_گرہ_اظہاریہ* ثابت nextp = VN_AS(nodep->nextp(), گرہ_اظہاریہ);
        إذا (nextp) nextp->unlink_Fr_پٹھ_With_اگلا();
        outp = Ast_گرہ::جوڑو_اگلا(outp, جديد AstArg{nodep->fileline(), "", nodep});
        nodep = nextp;
    }
    أرجع outp;
}

Ast_سونپو_W* V3ParseGrammar::create_Supply_اظہاریہ(File_سطر* fileline, ثابت string& name, صحيح قدر) {
    Ast_سونپو_W* assignp = جديد Ast_سونپو_W{fileline, جديد AstParseRef{fileline, name},
                                         قدر ? جديد AstConst{fileline, AstConst::All1{}}
                                               : جديد AstConst{fileline, AstConst::All0{}}};
    AstStrengthSpec* strengthSpecp
        = جديد AstStrengthSpec{fileline, VStrength::SUPPLY, VStrength::SUPPLY};
    assignp->strengthSpecp(strengthSpecp);
    أرجع assignp;
}

AstRange* V3ParseGrammar::scrubRangeMulti(Ast_گرہ_Range* nrangep) {
    // Remove any UnsizedRange's from list; preserves multi-dim chain via nextp().
    لكل (Ast_گرہ_Range *nodep = nrangep, *nextp; nodep; nodep = nextp) {
        nextp = VN_AS(nodep->nextp(), گرہ_Range);
        إذا (!VN_IS(nodep, Range)) {
            nodep->v3error(
                "Unsupported or syntax error: Unsized range in instance or other declaration");
            nodep->unlink_Fr_پٹھ();
            VL_DO_DANGLING(nodep->delete_درخت(), nodep);
        }
    }
    أرجع VN_CAST(nrangep, Range);
}
AstRange* V3ParseGrammar::scrubRange(Ast_گرہ_Range* nrangep) {
    AstRange* ثابت rangep = scrubRangeMulti(nrangep);
    إذا (rangep && rangep->nextp()) {
        // Gate primitives only support a single dimension
        rangep->nextp()->v3warn(E_UNSUPPORTED, "Unsupported: Multidimensional gate instances.");
        rangep->nextp()->unlink_Fr_پٹھ_With_اگلا()->delete_درخت();
    }
    أرجع rangep;
}

Ast_گرہ_Pre_Sel* V3ParseGrammar::scrubSel(Ast_گرہ_اظہاریہ* fromp, Ast_گرہ_Pre_Sel* selp) VL_MT_DISABLED {
    // SEL(PARSELVALUE, ...) -> SEL(fromp, ...)
    Ast_گرہ_Pre_Sel* subSelp = selp;
    طالما (سچ) {
        إذا (VN_IS(subSelp->fromp(), ParseHolder)) اكسر;
        إذا (Ast_گرہ_Pre_Sel* ثابت ہیٹھلا_Selp = VN_CAST(subSelp->fromp(), گرہ_Pre_Sel)) {
            subSelp = ہیٹھلا_Selp;
            استمر;
        }
        subSelp->v3fatalSrc("Couldn't find where to insert expression into select");
    }
    Ast_گرہ* subSelFromp = subSelp->fromp();
    subSelFromp->replaceWith(fromp);
    VL_DO_DANGLING(subSelFromp->delete_درخت(), subSelFromp);
    أرجع selp;
}

Ast_گرہ_DType* V3ParseGrammar::createArray(Ast_گرہ_DType* basep, Ast_گرہ_Range* nrangep,
                                          منطقي isPacked) {
    // Split RANGE0-RANGE1-RANGE2
    // into ARRAYDTYPE0(ARRAYDTYPE1(ARRAYDTYPE2(BASICTYPE3), RANGE), RANGE)
    Ast_گرہ_DType* arrayp = basep;
    إذا (nrangep) {  // Maybe no range - return unmodified base type
        طالما (nrangep->nextp()) nrangep = VN_AS(nrangep->nextp(), گرہ_Range);
        طالما (nrangep) {
            Ast_گرہ_Range* ثابت prevp = VN_AS(nrangep->backp(), گرہ_Range);
            إذا (prevp) nrangep->unlink_Fr_پٹھ();
            AstRange* ثابت rangep = VN_CAST(nrangep, Range);
            إذا (rangep && isPacked) {
                arrayp
                    = جديد AstPackArrayDType{rangep->fileline(), VFlag_بچہ_DType{}, arrayp, rangep};
            } وإلا إذا (rangep
                       && (VN_IS(rangep->leftp(), Unbounded)
                           || VN_IS(rangep->rightp(), Unbounded))) {
                arrayp = جديد Ast_قطار_DType{nrangep->fileline(), VFlag_بچہ_DType{}, arrayp,
                                           rangep->rightp()->clone_درخت(سچ)};
                VL_DO_DANGLING(nrangep->delete_درخت(), nrangep);
            } وإلا إذا (rangep) {
                arrayp = جديد AstUnpackArrayDType{rangep->fileline(), VFlag_بچہ_DType{}, arrayp,
                                                 rangep};
            } وإلا إذا (VN_IS(nrangep, UnsizedRange)) {
                arrayp = جديد AstDynArrayDType{nrangep->fileline(), VFlag_بچہ_DType{}, arrayp};
                VL_DO_DANGLING(nrangep->delete_درخت(), nrangep);
            } وإلا إذا (VN_IS(nrangep, BracketRange)) {
                ثابت AstBracketRange* ثابت arangep = VN_AS(nrangep, BracketRange);
                Ast_گرہ* ثابت keyp = arangep->elementsp()->unlink_Fr_پٹھ();
                arrayp = جديد AstBracketArrayDType{nrangep->fileline(), VFlag_بچہ_DType{}, arrayp,
                                                  keyp};
                VL_DO_DANGLING(nrangep->delete_درخت(), nrangep);
            } وإلا إذا (VN_IS(nrangep, WildcardRange)) {
                arrayp = جديد AstWildcardArrayDType{nrangep->fileline(), VFlag_بچہ_DType{}, arrayp};
                VL_DO_DANGLING(nrangep->delete_درخت(), nrangep);
            } وإلا {
                UASSERT_OBJ(0, nrangep, "Expected range or unsized range");
            }
            nrangep = prevp;
        }
    }
    أرجع arrayp;
}

AstVar* V3ParseGrammar::createVariable(File_سطر* fileline, ثابت string& name,
                                       Ast_گرہ_Range* arrayp, Ast_گرہ* attrsp) {
    UINFO(5, "  creVar " << name << "  decl=" << GRAMMARP->m_varDecl << "  io="
                         << GRAMMARP->m_varIO << "  dt=" << (GRAMMARP->m_varDTypep ? "set" : ""));
    إذا (GRAMMARP->m_varIO == VDirection::NONE  // In non-ANSI port list
        && GRAMMARP->m_varDecl == VVarType::پورٹ) {
        // Just a port list with variable name (not v2k format); AstPort already created
        إذا (arrayp) VL_DO_DANGLING(arrayp->delete_درخت(), arrayp);
        إذا (attrsp) {
            // TODO: Merge attributes across list? Or warn attribute is ignored
            VL_DO_DANGLING(attrsp->delete_درخت(), attrsp);
        }
        أرجع خالی_سوچک;
    }

    Ast_گرہ_DType* ثابت dtypep = [&]() -> Ast_گرہ_DType* {
        إذا (GRAMMARP->m_varDecl == VVarType::WREAL) {
            // dtypep might not be null, might be implicit LOGIC before we knew better
            أرجع جديد AstBasicDType{fileline, VBasicDTypeKwd::DOUBLE};
        }
        إذا (GRAMMARP->m_varDTypep) {
            // May make new variables with same type, so clone
            أرجع GRAMMARP->m_varDTypep->clone_درخت(خطأ);
        }
        // Created implicitly
        إذا (m_insideProperty) {
            إذا (m_typed_Property_پورٹ) {
                fileline->v3warn(E_UNSUPPORTED, "Untyped property port following a typed port");
            }
            أرجع جديد AstBasicDType{fileline, VBasicDTypeKwd::UNTYPED};
        }
        أرجع جديد AstBasicDType{fileline, LOGIC_IMPLICIT};
    }();

    // UINFO(0,"CREVAR "<<fileline->ascii()<<" decl="<<GRAMMARP->m_varDecl.ascii()<<"
    // io="<<GRAMMARP->m_varIO.ascii()<<endl);
    VVarType type = GRAMMARP->m_varDecl;
    إذا (type == VVarType::UNKNOWN) {  // e.g. "output" non-ANSI standalone direction (vs "reg")
        إذا (GRAMMARP->m_varIO.isAny()) {
            type = VVarType::پورٹ;
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
    Ast_گرہ_DType* ثابت arrayDTypep = createArray(dtypep, arrayp, خطأ);

    AstVar* ثابت nodep = جديد AstVar{fileline, type, name, VFlag_بچہ_DType(), arrayDTypep};
    nodep->جوڑو_Attrsp(attrsp);
    nodep->ansi(m_pinAnsi);
    nodep->declTyped(m_varDeclTyped);
    nodep->lifetime(m_varLifetime);
    إذا (m_netDelayp) nodep->delayp(m_netDelayp->clone_درخت(خطأ));
    إذا (GRAMMARP->m_varDecl != VVarType::UNKNOWN) nodep->combineType(GRAMMARP->m_varDecl);
    إذا (GRAMMARP->m_varIO != VDirection::NONE) {
        nodep->declDirection(GRAMMARP->m_varIO);
        nodep->direction(GRAMMARP->m_varIO);
    }

    إذا (GRAMMARP->m_varDecl == VVarType::SUPPLY0) {
        Ast_سونپو_W* ثابت ap = V3ParseGrammar::create_Supply_اظہاریہ(fileline, nodep->name(), 0);
        Ast_گرہ::جوڑو_اگلا<Ast_گرہ, Ast_گرہ>(nodep, جديد AstAlways{ap});
    }
    إذا (GRAMMARP->m_varDecl == VVarType::SUPPLY1) {
        Ast_سونپو_W* ثابت ap = V3ParseGrammar::create_Supply_اظہاریہ(fileline, nodep->name(), 1);
        Ast_گرہ::جوڑو_اگلا<Ast_گرہ, Ast_گرہ>(nodep, جديد AstAlways{ap});
    }
    إذا (VN_IS(dtypep, ParseTypeDType)) {
        // Parser needs to know what is a type
        Ast_گرہ* ثابت newp = جديد AstTypedefFwd{fileline, name, VFwdType::NONE};
        Ast_گرہ::جوڑو_اگلا<Ast_گرہ, Ast_گرہ>(nodep, newp);
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
    إذا (nodep->varType().isVPIAccessible()) nodep->جوڑو_Attrsp(GRAMMARP->cloneScopedSigAttr());

    // Remember the last variable created, so we can attach attributes to it in later parsing
    GRAMMARP->m_varAttrp = nodep;
    PARSEP->ٹیگ_Nodep(GRAMMARP->m_varAttrp);
    أرجع nodep;
}

string V3ParseGrammar::unquoteString(File_سطر* fileline, ثابت std::string& متن) {
    string err_پیغام;
    string res = VString::unquoteSVString(متن, err_پیغام);
    إذا (!err_پیغام.خالی_تھاواں()) fileline->v3error(err_پیغام.c_str());
    أرجع res;
}

صحيح V3ParseGrammar::s_type_Imp_عدد = 0;
