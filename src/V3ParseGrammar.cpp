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
    𐡄𐡍 (PARSEP->debugBison() >= 9) yydebug = 1;
    أرجع yyparse();
}

ثابت حرف* V3ParseImp::tokenName(صحيح token) {
#𐡄𐡍 YYDEBUG || YYERROR_VERBOSE
    ساكن ثابت حرف** nameTablep = nullptr;
    𐡄𐡍 (!nameTablep) {
        صحيح size;
        لكل (size = 0; yytname[size]; ++size) {}
        nameTablep = جديد ثابت حرف*[size];
        // Workaround bug in bison's which have '!' in yytname but not token values
        صحيح iout = 0;
        لكل (صحيح i = 0; yytname[i]; ++i) {
            𐡄𐡍 (yytname[i][0] == '\'') استمر;
            nameTablep[iout++] = yytname[i];
        }
    }
    𐡄𐡍 (token >= 255) {
        أرجع nameTablep[token - 255];
    } وإلا {
        ساكن حرف s_ch[2];
        s_ch[0] = token;
        s_ch[1] = '\0';
        أرجع s_ch;
    }
#وإلا
    أرجع "";
#endif
}

فراغ V3ParseImp::candidatePli(VSpellCheck* spellerp) {
#𐡄𐡍 !YYERROR_VERBOSE
#error "Need lex token names"
#endif
    لكل (صحيح i = 0; yytname[i]; ++i) {
        𐡄𐡍 (yytname[i][0] != '\"') استمر;
        𐡄𐡍 (yytname[i][1] != '$') استمر;
        spellerp->pushCandidate(string{yytname[i]}.substr(1, strlen(yytname[i]) - 2));
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

AstArg* V3ParseGrammar::argWrapList(AstNodeExpr* nodep) {
    // Convert list of expressions to list of arguments
    AstArg* outp = nullptr;
    طالما (nodep) {
        AstNodeExpr* ثابت nextp = VN_AS(nodep->nextp(), NodeExpr);
        𐡄𐡍 (nextp) nextp->unlinkFrBackWithNext();
        outp = AstNode::addNext(outp, جديد AstArg{nodep->fileline(), "", nodep});
        nodep = nextp;
    }
    أرجع outp;
}

AstAssignW* V3ParseGrammar::createSupplyExpr(FileLine* fileline, ثابت string& name, صحيح value) {
    AstAssignW* assignp = جديد AstAssignW{fileline, جديد AstParseRef{fileline, name},
                                         value ? جديد AstConst{fileline, AstConst::All1{}}
                                               : جديد AstConst{fileline, AstConst::All0{}}};
    AstStrengthSpec* strengthSpecp
        = جديد AstStrengthSpec{fileline, VStrength::SUPPLY, VStrength::SUPPLY};
    assignp->strengthSpecp(strengthSpecp);
    أرجع assignp;
}

AstRange* V3ParseGrammar::scrubRangeMulti(AstNodeRange* nrangep) {
    // Remove any UnsizedRange's from list; preserves multi-dim chain via nextp().
    لكل (AstNodeRange *nodep = nrangep, *nextp; nodep; nodep = nextp) {
        nextp = VN_AS(nodep->nextp(), NodeRange);
        𐡄𐡍 (!VN_IS(nodep, Range)) {
            nodep->v3error(
                "Unsupported or syntax error: Unsized range in instance or other declaration");
            nodep->unlinkFrBack();
            VL_DO_DANGLING(nodep->deleteTree(), nodep);
        }
    }
    أرجع VN_CAST(nrangep, Range);
}
AstRange* V3ParseGrammar::scrubRange(AstNodeRange* nrangep) {
    AstRange* ثابت rangep = scrubRangeMulti(nrangep);
    𐡄𐡍 (rangep && rangep->nextp()) {
        // Gate primitives only support a single dimension
        rangep->nextp()->v3warn(E_UNSUPPORTED, "Unsupported: Multidimensional gate instances.");
        rangep->nextp()->unlinkFrBackWithNext()->deleteTree();
    }
    أرجع rangep;
}

AstNodePreSel* V3ParseGrammar::scrubSel(AstNodeExpr* fromp, AstNodePreSel* selp) VL_MT_DISABLED {
    // SEL(PARSELVALUE, ...) -> SEL(fromp, ...)
    AstNodePreSel* subSelp = selp;
    طالما (𐡒𐡔𐡈) {
        𐡄𐡍 (VN_IS(subSelp->fromp(), ParseHolder)) اكسر;
        𐡄𐡍 (AstNodePreSel* ثابت lowerSelp = VN_CAST(subSelp->fromp(), NodePreSel)) {
            subSelp = lowerSelp;
            استمر;
        }
        subSelp->v3fatalSrc("Couldn't find where to insert expression into select");
    }
    AstNode* subSelFromp = subSelp->fromp();
    subSelFromp->replaceWith(fromp);
    VL_DO_DANGLING(subSelFromp->deleteTree(), subSelFromp);
    أرجع selp;
}

AstNodeDType* V3ParseGrammar::createArray(AstNodeDType* basep, AstNodeRange* nrangep,
                                          منطقي isPacked) {
    // Split RANGE0-RANGE1-RANGE2
    // into ARRAYDTYPE0(ARRAYDTYPE1(ARRAYDTYPE2(BASICTYPE3), RANGE), RANGE)
    AstNodeDType* arrayp = basep;
    𐡄𐡍 (nrangep) {  // Maybe no range - return unmodified base type
        طالما (nrangep->nextp()) nrangep = VN_AS(nrangep->nextp(), NodeRange);
        طالما (nrangep) {
            AstNodeRange* ثابت prevp = VN_AS(nrangep->backp(), NodeRange);
            𐡄𐡍 (prevp) nrangep->unlinkFrBack();
            AstRange* ثابت rangep = VN_CAST(nrangep, Range);
            𐡄𐡍 (rangep && isPacked) {
                arrayp
                    = جديد AstPackArrayDType{rangep->fileline(), VFlagChildDType{}, arrayp, rangep};
            } وإلا 𐡄𐡍 (rangep
                       && (VN_IS(rangep->leftp(), Unbounded)
                           || VN_IS(rangep->rightp(), Unbounded))) {
                arrayp = جديد AstQueueDType{nrangep->fileline(), VFlagChildDType{}, arrayp,
                                           rangep->rightp()->cloneTree(𐡒𐡔𐡈)};
                VL_DO_DANGLING(nrangep->deleteTree(), nrangep);
            } وإلا 𐡄𐡍 (rangep) {
                arrayp = جديد AstUnpackArrayDType{rangep->fileline(), VFlagChildDType{}, arrayp,
                                                 rangep};
            } وإلا 𐡄𐡍 (VN_IS(nrangep, UnsizedRange)) {
                arrayp = جديد AstDynArrayDType{nrangep->fileline(), VFlagChildDType{}, arrayp};
                VL_DO_DANGLING(nrangep->deleteTree(), nrangep);
            } وإلا 𐡄𐡍 (VN_IS(nrangep, BracketRange)) {
                ثابت AstBracketRange* ثابت arangep = VN_AS(nrangep, BracketRange);
                AstNode* ثابت keyp = arangep->elementsp()->unlinkFrBack();
                arrayp = جديد AstBracketArrayDType{nrangep->fileline(), VFlagChildDType{}, arrayp,
                                                  keyp};
                VL_DO_DANGLING(nrangep->deleteTree(), nrangep);
            } وإلا 𐡄𐡍 (VN_IS(nrangep, WildcardRange)) {
                arrayp = جديد AstWildcardArrayDType{nrangep->fileline(), VFlagChildDType{}, arrayp};
                VL_DO_DANGLING(nrangep->deleteTree(), nrangep);
            } وإلا {
                UASSERT_OBJ(0, nrangep, "Expected range or unsized range");
            }
            nrangep = prevp;
        }
    }
    أرجع arrayp;
}

AstVar* V3ParseGrammar::createVariable(FileLine* fileline, ثابت string& name,
                                       AstNodeRange* arrayp, AstNode* attrsp) {
    UINFO(5, "  creVar " << name << "  decl=" << GRAMMARP->m_varDecl << "  io="
                         << GRAMMARP->m_varIO << "  dt=" << (GRAMMARP->m_varDTypep ? "set" : ""));
    𐡄𐡍 (GRAMMARP->m_varIO == VDirection::NONE  // In non-ANSI port list
        && GRAMMARP->m_varDecl == VVarType::PORT) {
        // Just a port list with variable name (not v2k format); AstPort already created
        𐡄𐡍 (arrayp) VL_DO_DANGLING(arrayp->deleteTree(), arrayp);
        𐡄𐡍 (attrsp) {
            // TODO: Merge attributes across list? Or warn attribute is ignored
            VL_DO_DANGLING(attrsp->deleteTree(), attrsp);
        }
        أرجع nullptr;
    }

    AstNodeDType* ثابت dtypep = [&]() -> AstNodeDType* {
        𐡄𐡍 (GRAMMARP->m_varDecl == VVarType::WREAL) {
            // dtypep might not be null, might be implicit LOGIC before we knew better
            أرجع جديد AstBasicDType{fileline, VBasicDTypeKwd::DOUBLE};
        }
        𐡄𐡍 (GRAMMARP->m_varDTypep) {
            // May make new variables with same type, so clone
            أرجع GRAMMARP->m_varDTypep->cloneTree(خطأ);
        }
        // Created implicitly
        𐡄𐡍 (m_insideProperty) {
            𐡄𐡍 (m_typedPropertyPort) {
                fileline->v3warn(E_UNSUPPORTED, "Untyped property port following a typed port");
            }
            أرجع جديد AstBasicDType{fileline, VBasicDTypeKwd::UNTYPED};
        }
        أرجع جديد AstBasicDType{fileline, LOGIC_IMPLICIT};
    }();

    // UINFO(0,"CREVAR "<<fileline->ascii()<<" decl="<<GRAMMARP->m_varDecl.ascii()<<"
    // io="<<GRAMMARP->m_varIO.ascii()<<endl);
    VVarType type = GRAMMARP->m_varDecl;
    𐡄𐡍 (type == VVarType::UNKNOWN) {  // e.g. "output" non-ANSI standalone direction (vs "reg")
        𐡄𐡍 (GRAMMARP->m_varIO.isAny()) {
            type = VVarType::PORT;
        } وإلا {
            fileline->v3fatalSrc("Unknown signal type declared: " << type.ascii());
        }
    }
    𐡄𐡍 (type == VVarType::GENVAR) {
        // Should be impossible as the grammar blocks this, but...
        𐡄𐡍 (arrayp) fileline->v3error("Genvars may not be arrayed: " << name);  // LCOV_EXCL_LINE
    }

    // Split RANGE0-RANGE1-RANGE2 into
    // ARRAYDTYPE0(ARRAYDTYPE1(ARRAYDTYPE2(BASICTYPE3), RANGE), RANGE)
    AstNodeDType* ثابت arrayDTypep = createArray(dtypep, arrayp, خطأ);

    AstVar* ثابت nodep = جديد AstVar{fileline, type, name, VFlagChildDType(), arrayDTypep};
    nodep->addAttrsp(attrsp);
    nodep->ansi(m_pinAnsi);
    nodep->declTyped(m_varDeclTyped);
    nodep->lifetime(m_varLifetime);
    𐡄𐡍 (m_netDelayp) nodep->delayp(m_netDelayp->cloneTree(خطأ));
    𐡄𐡍 (GRAMMARP->m_varDecl != VVarType::UNKNOWN) nodep->combineType(GRAMMARP->m_varDecl);
    𐡄𐡍 (GRAMMARP->m_varIO != VDirection::NONE) {
        nodep->declDirection(GRAMMARP->m_varIO);
        nodep->direction(GRAMMARP->m_varIO);
    }

    𐡄𐡍 (GRAMMARP->m_varDecl == VVarType::SUPPLY0) {
        AstAssignW* ثابت ap = V3ParseGrammar::createSupplyExpr(fileline, nodep->name(), 0);
        AstNode::addNext<AstNode, AstNode>(nodep, جديد AstAlways{ap});
    }
    𐡄𐡍 (GRAMMARP->m_varDecl == VVarType::SUPPLY1) {
        AstAssignW* ثابت ap = V3ParseGrammar::createSupplyExpr(fileline, nodep->name(), 1);
        AstNode::addNext<AstNode, AstNode>(nodep, جديد AstAlways{ap});
    }
    𐡄𐡍 (VN_IS(dtypep, ParseTypeDType)) {
        // Parser needs to know what is a type
        AstNode* ثابت newp = جديد AstTypedefFwd{fileline, name, VFwdType::NONE};
        AstNode::addNext<AstNode, AstNode>(nodep, newp);
    }
    // Don't set dtypep in the ranging;
    // We need to autosize parameters and integers separately
    //
    // Propagate from current module tracing state
    𐡄𐡍 (nodep->isGenVar()) {
        nodep->trace(خطأ);
    } وإلا 𐡄𐡍 (nodep->isParam() && !v3Global.opt.traceParams()) {
        nodep->trace(خطأ);
    } وإلا {
        nodep->trace(allTracingOn(nodep->fileline()));
    }
    𐡄𐡍 (nodep->varType().isVPIAccessible()) nodep->addAttrsp(GRAMMARP->cloneScopedSigAttr());

    // Remember the last variable created, so we can attach attributes to it in later parsing
    GRAMMARP->m_varAttrp = nodep;
    PARSEP->tagNodep(GRAMMARP->m_varAttrp);
    أرجع nodep;
}

string V3ParseGrammar::unquoteString(FileLine* fileline, ثابت std::string& text) {
    string errMsg;
    string res = VString::unquoteSVString(text, errMsg);
    𐡄𐡍 (!errMsg.empty()) fileline->v3error(errMsg.c_str());
    أرجع res;
}

صحيح V3ParseGrammar::s_typeImpNum = 0;
