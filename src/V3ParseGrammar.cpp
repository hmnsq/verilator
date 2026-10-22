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

#تعریف_کرو VL_MT_DISABLED_رمز_UNIT 1

#شامل "V3Ast.h"  // This must be before V3ParseBison.cpp, as we don't want #defines to conflict

VL_DEFINE_DEBUG_FUNCTIONS;

//======================================================================
// The guts come from bison output

#شامل "V3ParseBison.c"

//======================================================================
// V3ParseImp functions requiring bison state

صحیح V3ParseImp::bisonParse() {
    // Use --debugi-bison 9 to enable this
    اگر (PARSEP->debugBison() >= 9) yydebug = 1;
    واپس yyparse();
}

ثابت حرف* V3ParseImp::tokenName(صحیح token) {
#اگر YYDEBUG || YYERROR_VERBOSE
    ساکن ثابت حرف** nameTablep = خالی_اشاریہ;
    اگر (!nameTablep) {
        صحیح سائز;
        برائے (سائز = 0; yytname[سائز]; ++سائز) {}
        nameTablep = نیا ثابت حرف*[سائز];
        // Workaround bug in bison's which have '!' in yytname but not token values
        صحیح iout = 0;
        برائے (صحیح i = 0; yytname[i]; ++i) {
            اگر (yytname[i][0] == '\'') جاری;
            nameTablep[iout++] = yytname[i];
        }
    }
    اگر (token >= 255) {
        واپس nameTablep[token - 255];
    } ورنہ {
        ساکن حرف s_ch[2];
        s_ch[0] = token;
        s_ch[1] = '\0';
        واپس s_ch;
    }
#ورنہ
    واپس "";
#اگر_ختم
}

خالی V3ParseImp::candidatePli(VSpellCheck* spellerp) {
#اگر !YYERROR_VERBOSE
#غلطی "Need lex token names"
#اگر_ختم
    برائے (صحیح i = 0; yytname[i]; ++i) {
        اگر (yytname[i][0] != '\"') جاری;
        اگر (yytname[i][1] != '$') جاری;
        spellerp->ڈالو_Candidate(string{yytname[i]}.substr(1, لڑی_لمبائی(yytname[i]) - 2));
    }
}

خالی V3ParseImp::parserClear() {
    // Clear up any dynamic memory V3Parser required
    VARDTYPE(خالی_اشاریہ);
    GRAMMARP->سیٹ_Net_Delay(خالی_اشاریہ);
    GRAMMARP->سیٹ_Scoped_Sig_Attr(خالی_اشاریہ);
}

//======================================================================
// V3ParseGrammar functions requiring bison state

AstArg* V3ParseGrammar::argWrapList(Ast_گرہ_اظہاریہ* nodep) {
    // Convert list of expressions to list of arguments
    AstArg* outp = خالی_اشاریہ;
    جب (nodep) {
        Ast_گرہ_اظہاریہ* ثابت nextp = VN_AS(nodep->nextp(), گرہ_اظہاریہ);
        اگر (nextp) nextp->unlink_Fr_عقب_With_اگلا();
        outp = Ast_گرہ::جوڑو_اگلا(outp, نیا AstArg{nodep->fileline(), "", nodep});
        nodep = nextp;
    }
    واپس outp;
}

Ast_تفویض_W* V3ParseGrammar::create_Supply_اظہاریہ(File_سطر* fileline, ثابت string& name, صحیح قدر) {
    Ast_تفویض_W* assignp = نیا Ast_تفویض_W{fileline, نیا AstParseRef{fileline, name},
                                         قدر ? نیا AstConst{fileline, AstConst::All1{}}
                                               : نیا AstConst{fileline, AstConst::All0{}}};
    AstStrengthSpec* strengthSpecp
        = نیا AstStrengthSpec{fileline, VStrength::SUPPLY, VStrength::SUPPLY};
    assignp->strengthSpecp(strengthSpecp);
    واپس assignp;
}

AstRange* V3ParseGrammar::scrubRangeMulti(Ast_گرہ_Range* nrangep) {
    // Remove any UnsizedRange's from list; preserves multi-dim chain via nextp().
    برائے (Ast_گرہ_Range *nodep = nrangep, *nextp; nodep; nodep = nextp) {
        nextp = VN_AS(nodep->nextp(), گرہ_Range);
        اگر (!VN_IS(nodep, Range)) {
            nodep->v3error(
                "Unsupported or syntax error: Unsized range in instance or other declaration");
            nodep->unlink_Fr_عقب();
            VL_DO_DANGLING(nodep->delete_درخت(), nodep);
        }
    }
    واپس VN_CAST(nrangep, Range);
}
AstRange* V3ParseGrammar::scrubRange(Ast_گرہ_Range* nrangep) {
    AstRange* ثابت rangep = scrubRangeMulti(nrangep);
    اگر (rangep && rangep->nextp()) {
        // Gate primitives only support a single dimension
        rangep->nextp()->v3warn(E_UNSUPPORTED, "Unsupported: Multidimensional gate instances.");
        rangep->nextp()->unlink_Fr_عقب_With_اگلا()->delete_درخت();
    }
    واپس rangep;
}

Ast_گرہ_Pre_Sel* V3ParseGrammar::scrubSel(Ast_گرہ_اظہاریہ* fromp, Ast_گرہ_Pre_Sel* selp) VL_MT_DISABLED {
    // SEL(PARSELVALUE, ...) -> SEL(fromp, ...)
    Ast_گرہ_Pre_Sel* subSelp = selp;
    جب (سچ) {
        اگر (VN_IS(subSelp->fromp(), ParseHolder)) توڑو;
        اگر (Ast_گرہ_Pre_Sel* ثابت زیریں_Selp = VN_CAST(subSelp->fromp(), گرہ_Pre_Sel)) {
            subSelp = زیریں_Selp;
            جاری;
        }
        subSelp->v3fatalSrc("Couldn't find where to insert expression into select");
    }
    Ast_گرہ* subSelFromp = subSelp->fromp();
    subSelFromp->replaceWith(fromp);
    VL_DO_DANGLING(subSelFromp->delete_درخت(), subSelFromp);
    واپس selp;
}

Ast_گرہ_DType* V3ParseGrammar::createArray(Ast_گرہ_DType* basep, Ast_گرہ_Range* nrangep,
                                          منطقی isPacked) {
    // Split RANGE0-RANGE1-RANGE2
    // into ARRAYDTYPE0(ARRAYDTYPE1(ARRAYDTYPE2(BASICTYPE3), RANGE), RANGE)
    Ast_گرہ_DType* arrayp = basep;
    اگر (nrangep) {  // Maybe no range - return unmodified base type
        جب (nrangep->nextp()) nrangep = VN_AS(nrangep->nextp(), گرہ_Range);
        جب (nrangep) {
            Ast_گرہ_Range* ثابت prevp = VN_AS(nrangep->backp(), گرہ_Range);
            اگر (prevp) nrangep->unlink_Fr_عقب();
            AstRange* ثابت rangep = VN_CAST(nrangep, Range);
            اگر (rangep && isPacked) {
                arrayp
                    = نیا AstPackArrayDType{rangep->fileline(), VFlag_بچہ_DType{}, arrayp, rangep};
            } ورنہ اگر (rangep
                       && (VN_IS(rangep->leftp(), Unbounded)
                           || VN_IS(rangep->rightp(), Unbounded))) {
                arrayp = نیا Ast_قطار_DType{nrangep->fileline(), VFlag_بچہ_DType{}, arrayp,
                                           rangep->rightp()->clone_درخت(سچ)};
                VL_DO_DANGLING(nrangep->delete_درخت(), nrangep);
            } ورنہ اگر (rangep) {
                arrayp = نیا AstUnpackArrayDType{rangep->fileline(), VFlag_بچہ_DType{}, arrayp,
                                                 rangep};
            } ورنہ اگر (VN_IS(nrangep, UnsizedRange)) {
                arrayp = نیا AstDynArrayDType{nrangep->fileline(), VFlag_بچہ_DType{}, arrayp};
                VL_DO_DANGLING(nrangep->delete_درخت(), nrangep);
            } ورنہ اگر (VN_IS(nrangep, BracketRange)) {
                ثابت AstBracketRange* ثابت arangep = VN_AS(nrangep, BracketRange);
                Ast_گرہ* ثابت keyp = arangep->elementsp()->unlink_Fr_عقب();
                arrayp = نیا AstBracketArrayDType{nrangep->fileline(), VFlag_بچہ_DType{}, arrayp,
                                                  keyp};
                VL_DO_DANGLING(nrangep->delete_درخت(), nrangep);
            } ورنہ اگر (VN_IS(nrangep, WildcardRange)) {
                arrayp = نیا AstWildcardArrayDType{nrangep->fileline(), VFlag_بچہ_DType{}, arrayp};
                VL_DO_DANGLING(nrangep->delete_درخت(), nrangep);
            } ورنہ {
                UASSERT_OBJ(0, nrangep, "Expected range or unsized range");
            }
            nrangep = prevp;
        }
    }
    واپس arrayp;
}

AstVar* V3ParseGrammar::createVariable(File_سطر* fileline, ثابت string& name,
                                       Ast_گرہ_Range* arrayp, Ast_گرہ* attrsp) {
    UINFO(5, "  creVar " << name << "  decl=" << GRAMMARP->m_varDecl << "  io="
                         << GRAMMARP->m_varIO << "  dt=" << (GRAMMARP->m_varDTypep ? "set" : ""));
    اگر (GRAMMARP->m_varIO == VDirection::NONE  // In non-ANSI port list
        && GRAMMARP->m_varDecl == VVarType::پورٹ) {
        // Just a port list with variable name (not v2k format); AstPort already created
        اگر (arrayp) VL_DO_DANGLING(arrayp->delete_درخت(), arrayp);
        اگر (attrsp) {
            // TODO: Merge attributes across list? Or warn attribute is ignored
            VL_DO_DANGLING(attrsp->delete_درخت(), attrsp);
        }
        واپس خالی_اشاریہ;
    }

    Ast_گرہ_DType* ثابت dtypep = [&]() -> Ast_گرہ_DType* {
        اگر (GRAMMARP->m_varDecl == VVarType::WREAL) {
            // dtypep might not be null, might be implicit LOGIC before we knew better
            واپس نیا AstBasicDType{fileline, VBasicDTypeKwd::DOUBLE};
        }
        اگر (GRAMMARP->m_varDTypep) {
            // May make new variables with same type, so clone
            واپس GRAMMARP->m_varDTypep->clone_درخت(جھوٹ);
        }
        // Created implicitly
        اگر (m_insideProperty) {
            اگر (m_typed_Property_پورٹ) {
                fileline->v3warn(E_UNSUPPORTED, "Untyped property port following a typed port");
            }
            واپس نیا AstBasicDType{fileline, VBasicDTypeKwd::UNTYPED};
        }
        واپس نیا AstBasicDType{fileline, LOGIC_IMPLICIT};
    }();

    // UINFO(0,"CREVAR "<<fileline->ascii()<<" decl="<<GRAMMARP->m_varDecl.ascii()<<"
    // io="<<GRAMMARP->m_varIO.ascii()<<endl);
    VVarType type = GRAMMARP->m_varDecl;
    اگر (type == VVarType::UNKNOWN) {  // e.g. "output" non-ANSI standalone direction (vs "reg")
        اگر (GRAMMARP->m_varIO.isAny()) {
            type = VVarType::پورٹ;
        } ورنہ {
            fileline->v3fatalSrc("Unknown signal type declared: " << type.ascii());
        }
    }
    اگر (type == VVarType::GENVAR) {
        // Should be impossible as the grammar blocks this, but...
        اگر (arrayp) fileline->v3error("Genvars may not be arrayed: " << name);  // LCOV_EXCL_LINE
    }

    // Split RANGE0-RANGE1-RANGE2 into
    // ARRAYDTYPE0(ARRAYDTYPE1(ARRAYDTYPE2(BASICTYPE3), RANGE), RANGE)
    Ast_گرہ_DType* ثابت arrayDTypep = createArray(dtypep, arrayp, جھوٹ);

    AstVar* ثابت nodep = نیا AstVar{fileline, type, name, VFlag_بچہ_DType(), arrayDTypep};
    nodep->جوڑو_Attrsp(attrsp);
    nodep->ansi(m_pinAnsi);
    nodep->declTyped(m_varDeclTyped);
    nodep->lifetime(m_varLifetime);
    اگر (m_netDelayp) nodep->delayp(m_netDelayp->clone_درخت(جھوٹ));
    اگر (GRAMMARP->m_varDecl != VVarType::UNKNOWN) nodep->combineType(GRAMMARP->m_varDecl);
    اگر (GRAMMARP->m_varIO != VDirection::NONE) {
        nodep->declDirection(GRAMMARP->m_varIO);
        nodep->direction(GRAMMARP->m_varIO);
    }

    اگر (GRAMMARP->m_varDecl == VVarType::SUPPLY0) {
        Ast_تفویض_W* ثابت ap = V3ParseGrammar::create_Supply_اظہاریہ(fileline, nodep->name(), 0);
        Ast_گرہ::جوڑو_اگلا<Ast_گرہ, Ast_گرہ>(nodep, نیا AstAlways{ap});
    }
    اگر (GRAMMARP->m_varDecl == VVarType::SUPPLY1) {
        Ast_تفویض_W* ثابت ap = V3ParseGrammar::create_Supply_اظہاریہ(fileline, nodep->name(), 1);
        Ast_گرہ::جوڑو_اگلا<Ast_گرہ, Ast_گرہ>(nodep, نیا AstAlways{ap});
    }
    اگر (VN_IS(dtypep, ParseTypeDType)) {
        // Parser needs to know what is a type
        Ast_گرہ* ثابت newp = نیا AstTypedefFwd{fileline, name, VFwdType::NONE};
        Ast_گرہ::جوڑو_اگلا<Ast_گرہ, Ast_گرہ>(nodep, newp);
    }
    // Don't set dtypep in the ranging;
    // We need to autosize parameters and integers separately
    //
    // Propagate from current module tracing state
    اگر (nodep->isGenVar()) {
        nodep->trace(جھوٹ);
    } ورنہ اگر (nodep->isParam() && !v3Global.opt.traceParams()) {
        nodep->trace(جھوٹ);
    } ورنہ {
        nodep->trace(allTracingOn(nodep->fileline()));
    }
    اگر (nodep->varType().isVPIAccessible()) nodep->جوڑو_Attrsp(GRAMMARP->cloneScopedSigAttr());

    // Remember the last variable created, so we can attach attributes to it in later parsing
    GRAMMARP->m_varAttrp = nodep;
    PARSEP->لیبل_Nodep(GRAMMARP->m_varAttrp);
    واپس nodep;
}

string V3ParseGrammar::unquoteString(File_سطر* fileline, ثابت std::string& متن) {
    string err_پیغام;
    string res = VString::unquoteSVString(متن, err_پیغام);
    اگر (!err_پیغام.خالی_خانے()) fileline->v3error(err_پیغام.c_str());
    واپس res;
}

صحیح V3ParseGrammar::s_type_Imp_عدد = 0;
