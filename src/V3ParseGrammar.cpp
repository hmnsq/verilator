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
    بازگشت yyparse();
}

ثابت نویسه* V3ParseImp::tokenName(صحیح token) {
#اگر YYDEBUG || YYERROR_VERBOSE
    ایستا ثابت نویسه** nameTablep = nullptr;
    اگر (!nameTablep) {
        صحیح size;
        برای (size = 0; yytname[size]; ++size) {}
        nameTablep = جدید ثابت نویسه*[size];
        // Workaround bug in bison's which have '!' in yytname but not token values
        صحیح iout = 0;
        برای (صحیح i = 0; yytname[i]; ++i) {
            اگر (yytname[i][0] == '\'') ادامه;
            nameTablep[iout++] = yytname[i];
        }
    }
    اگر (token >= 255) {
        بازگشت nameTablep[token - 255];
    } وگرنه {
        ایستا نویسه s_ch[2];
        s_ch[0] = token;
        s_ch[1] = '\0';
        بازگشت s_ch;
    }
#وگرنه
    بازگشت "";
#endif
}

تهی V3ParseImp::candidatePli(VSpellCheck* spellerp) {
#اگر !YYERROR_VERBOSE
#error "Need lex token names"
#endif
    برای (صحیح i = 0; yytname[i]; ++i) {
        اگر (yytname[i][0] != '\"') ادامه;
        اگر (yytname[i][1] != '$') ادامه;
        spellerp->افزودن_Candidate(string{yytname[i]}.substr(1, strlen(yytname[i]) - 2));
    }
}

تهی V3ParseImp::parserClear() {
    // Clear up any dynamic memory V3Parser required
    VARDTYPE(nullptr);
    GRAMMARP->setNetDelay(nullptr);
    GRAMMARP->setScopedSigAttr(nullptr);
}

//======================================================================
// V3ParseGrammar functions requiring bison state

AstArg* V3ParseGrammar::argWrapList(Ast_گره_عبارت* nodep) {
    // Convert list of expressions to list of arguments
    AstArg* outp = nullptr;
    وقتی (nodep) {
        Ast_گره_عبارت* ثابت nextp = VN_AS(nodep->nextp(), گره_عبارت);
        اگر (nextp) nextp->unlinkFrBackWithNext();
        outp = Ast_گره::addNext(outp, جدید AstArg{nodep->fileline(), "", nodep});
        nodep = nextp;
    }
    بازگشت outp;
}

AstAssignW* V3ParseGrammar::create_Supply_عبارت(File_سطر* fileline, ثابت string& name, صحیح مقدار) {
    AstAssignW* assignp = جدید AstAssignW{fileline, جدید AstParseRef{fileline, name},
                                         مقدار ? جدید AstConst{fileline, AstConst::All1{}}
                                               : جدید AstConst{fileline, AstConst::All0{}}};
    AstStrengthSpec* strengthSpecp
        = جدید AstStrengthSpec{fileline, VStrength::SUPPLY, VStrength::SUPPLY};
    assignp->strengthSpecp(strengthSpecp);
    بازگشت assignp;
}

AstRange* V3ParseGrammar::scrubRangeMulti(Ast_گره_Range* nrangep) {
    // Remove any UnsizedRange's from list; preserves multi-dim chain via nextp().
    برای (Ast_گره_Range *nodep = nrangep, *nextp; nodep; nodep = nextp) {
        nextp = VN_AS(nodep->nextp(), گره_Range);
        اگر (!VN_IS(nodep, Range)) {
            nodep->v3error(
                "Unsupported or syntax error: Unsized range in instance or other declaration");
            nodep->unlinkFrBack();
            VL_DO_DANGLING(nodep->delete_درخت(), nodep);
        }
    }
    بازگشت VN_CAST(nrangep, Range);
}
AstRange* V3ParseGrammar::scrubRange(Ast_گره_Range* nrangep) {
    AstRange* ثابت rangep = scrubRangeMulti(nrangep);
    اگر (rangep && rangep->nextp()) {
        // Gate primitives only support a single dimension
        rangep->nextp()->v3warn(E_UNSUPPORTED, "Unsupported: Multidimensional gate instances.");
        rangep->nextp()->unlinkFrBackWithNext()->delete_درخت();
    }
    بازگشت rangep;
}

Ast_گره_Pre_Sel* V3ParseGrammar::scrubSel(Ast_گره_عبارت* fromp, Ast_گره_Pre_Sel* selp) VL_MT_DISABLED {
    // SEL(PARSELVALUE, ...) -> SEL(fromp, ...)
    Ast_گره_Pre_Sel* subSelp = selp;
    وقتی (درست) {
        اگر (VN_IS(subSelp->fromp(), ParseHolder)) بشکن;
        اگر (Ast_گره_Pre_Sel* ثابت lowerSelp = VN_CAST(subSelp->fromp(), گره_Pre_Sel)) {
            subSelp = lowerSelp;
            ادامه;
        }
        subSelp->v3fatalSrc("Couldn't find where to insert expression into select");
    }
    Ast_گره* subSelFromp = subSelp->fromp();
    subSelFromp->replaceWith(fromp);
    VL_DO_DANGLING(subSelFromp->delete_درخت(), subSelFromp);
    بازگشت selp;
}

Ast_گره_DType* V3ParseGrammar::createArray(Ast_گره_DType* basep, Ast_گره_Range* nrangep,
                                          منطقی isPacked) {
    // Split RANGE0-RANGE1-RANGE2
    // into ARRAYDTYPE0(ARRAYDTYPE1(ARRAYDTYPE2(BASICTYPE3), RANGE), RANGE)
    Ast_گره_DType* arrayp = basep;
    اگر (nrangep) {  // Maybe no range - return unmodified base type
        وقتی (nrangep->nextp()) nrangep = VN_AS(nrangep->nextp(), گره_Range);
        وقتی (nrangep) {
            Ast_گره_Range* ثابت prevp = VN_AS(nrangep->backp(), گره_Range);
            اگر (prevp) nrangep->unlinkFrBack();
            AstRange* ثابت rangep = VN_CAST(nrangep, Range);
            اگر (rangep && isPacked) {
                arrayp
                    = جدید AstPackArrayDType{rangep->fileline(), VFlagChildDType{}, arrayp, rangep};
            } وگرنه اگر (rangep
                       && (VN_IS(rangep->leftp(), Unbounded)
                           || VN_IS(rangep->rightp(), Unbounded))) {
                arrayp = جدید AstQueueDType{nrangep->fileline(), VFlagChildDType{}, arrayp,
                                           rangep->rightp()->clone_درخت(درست)};
                VL_DO_DANGLING(nrangep->delete_درخت(), nrangep);
            } وگرنه اگر (rangep) {
                arrayp = جدید AstUnpackArrayDType{rangep->fileline(), VFlagChildDType{}, arrayp,
                                                 rangep};
            } وگرنه اگر (VN_IS(nrangep, UnsizedRange)) {
                arrayp = جدید AstDynArrayDType{nrangep->fileline(), VFlagChildDType{}, arrayp};
                VL_DO_DANGLING(nrangep->delete_درخت(), nrangep);
            } وگرنه اگر (VN_IS(nrangep, BracketRange)) {
                ثابت AstBracketRange* ثابت arangep = VN_AS(nrangep, BracketRange);
                Ast_گره* ثابت keyp = arangep->elementsp()->unlinkFrBack();
                arrayp = جدید AstBracketArrayDType{nrangep->fileline(), VFlagChildDType{}, arrayp,
                                                  keyp};
                VL_DO_DANGLING(nrangep->delete_درخت(), nrangep);
            } وگرنه اگر (VN_IS(nrangep, WildcardRange)) {
                arrayp = جدید AstWildcardArrayDType{nrangep->fileline(), VFlagChildDType{}, arrayp};
                VL_DO_DANGLING(nrangep->delete_درخت(), nrangep);
            } وگرنه {
                UASSERT_OBJ(0, nrangep, "Expected range or unsized range");
            }
            nrangep = prevp;
        }
    }
    بازگشت arrayp;
}

AstVar* V3ParseGrammar::createVariable(File_سطر* fileline, ثابت string& name,
                                       Ast_گره_Range* arrayp, Ast_گره* attrsp) {
    UINFO(5, "  creVar " << name << "  decl=" << GRAMMARP->m_varDecl << "  io="
                         << GRAMMARP->m_varIO << "  dt=" << (GRAMMARP->m_varDTypep ? "set" : ""));
    اگر (GRAMMARP->m_varIO == VDirection::NONE  // In non-ANSI port list
        && GRAMMARP->m_varDecl == VVarType::PORT) {
        // Just a port list with variable name (not v2k format); AstPort already created
        اگر (arrayp) VL_DO_DANGLING(arrayp->delete_درخت(), arrayp);
        اگر (attrsp) {
            // TODO: Merge attributes across list? Or warn attribute is ignored
            VL_DO_DANGLING(attrsp->delete_درخت(), attrsp);
        }
        بازگشت nullptr;
    }

    Ast_گره_DType* ثابت dtypep = [&]() -> Ast_گره_DType* {
        اگر (GRAMMARP->m_varDecl == VVarType::WREAL) {
            // dtypep might not be null, might be implicit LOGIC before we knew better
            بازگشت جدید AstBasicDType{fileline, VBasicDTypeKwd::DOUBLE};
        }
        اگر (GRAMMARP->m_varDTypep) {
            // May make new variables with same type, so clone
            بازگشت GRAMMARP->m_varDTypep->clone_درخت(نادرست);
        }
        // Created implicitly
        اگر (m_insideProperty) {
            اگر (m_typedPropertyPort) {
                fileline->v3warn(E_UNSUPPORTED, "Untyped property port following a typed port");
            }
            بازگشت جدید AstBasicDType{fileline, VBasicDTypeKwd::UNTYPED};
        }
        بازگشت جدید AstBasicDType{fileline, LOGIC_IMPLICIT};
    }();

    // UINFO(0,"CREVAR "<<fileline->ascii()<<" decl="<<GRAMMARP->m_varDecl.ascii()<<"
    // io="<<GRAMMARP->m_varIO.ascii()<<endl);
    VVarType type = GRAMMARP->m_varDecl;
    اگر (type == VVarType::UNKNOWN) {  // e.g. "output" non-ANSI standalone direction (vs "reg")
        اگر (GRAMMARP->m_varIO.isAny()) {
            type = VVarType::PORT;
        } وگرنه {
            fileline->v3fatalSrc("Unknown signal type declared: " << type.ascii());
        }
    }
    اگر (type == VVarType::GENVAR) {
        // Should be impossible as the grammar blocks this, but...
        اگر (arrayp) fileline->v3error("Genvars may not be arrayed: " << name);  // LCOV_EXCL_LINE
    }

    // Split RANGE0-RANGE1-RANGE2 into
    // ARRAYDTYPE0(ARRAYDTYPE1(ARRAYDTYPE2(BASICTYPE3), RANGE), RANGE)
    Ast_گره_DType* ثابت arrayDTypep = createArray(dtypep, arrayp, نادرست);

    AstVar* ثابت nodep = جدید AstVar{fileline, type, name, VFlagChildDType(), arrayDTypep};
    nodep->addAttrsp(attrsp);
    nodep->ansi(m_pinAnsi);
    nodep->declTyped(m_varDeclTyped);
    nodep->lifetime(m_varLifetime);
    اگر (m_netDelayp) nodep->delayp(m_netDelayp->clone_درخت(نادرست));
    اگر (GRAMMARP->m_varDecl != VVarType::UNKNOWN) nodep->combineType(GRAMMARP->m_varDecl);
    اگر (GRAMMARP->m_varIO != VDirection::NONE) {
        nodep->declDirection(GRAMMARP->m_varIO);
        nodep->direction(GRAMMARP->m_varIO);
    }

    اگر (GRAMMARP->m_varDecl == VVarType::SUPPLY0) {
        AstAssignW* ثابت ap = V3ParseGrammar::create_Supply_عبارت(fileline, nodep->name(), 0);
        Ast_گره::addNext<Ast_گره, Ast_گره>(nodep, جدید AstAlways{ap});
    }
    اگر (GRAMMARP->m_varDecl == VVarType::SUPPLY1) {
        AstAssignW* ثابت ap = V3ParseGrammar::create_Supply_عبارت(fileline, nodep->name(), 1);
        Ast_گره::addNext<Ast_گره, Ast_گره>(nodep, جدید AstAlways{ap});
    }
    اگر (VN_IS(dtypep, ParseTypeDType)) {
        // Parser needs to know what is a type
        Ast_گره* ثابت newp = جدید AstTypedefFwd{fileline, name, VFwdType::NONE};
        Ast_گره::addNext<Ast_گره, Ast_گره>(nodep, newp);
    }
    // Don't set dtypep in the ranging;
    // We need to autosize parameters and integers separately
    //
    // Propagate from current module tracing state
    اگر (nodep->isGenVar()) {
        nodep->trace(نادرست);
    } وگرنه اگر (nodep->isParam() && !v3Global.opt.traceParams()) {
        nodep->trace(نادرست);
    } وگرنه {
        nodep->trace(allTracingOn(nodep->fileline()));
    }
    اگر (nodep->varType().isVPIAccessible()) nodep->addAttrsp(GRAMMARP->cloneScopedSigAttr());

    // Remember the last variable created, so we can attach attributes to it in later parsing
    GRAMMARP->m_varAttrp = nodep;
    PARSEP->tagNodep(GRAMMARP->m_varAttrp);
    بازگشت nodep;
}

string V3ParseGrammar::unquoteString(File_سطر* fileline, ثابت std::string& text) {
    string err_پیام;
    string res = VString::unquoteSVString(text, err_پیام);
    اگر (!err_پیام.empty()) fileline->v3error(err_پیام.c_str());
    بازگشت res;
}

صحیح V3ParseGrammar::s_type_Imp_رقم = 0;
