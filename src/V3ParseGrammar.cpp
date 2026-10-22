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

#include "V3Ast.h"  // This must be before V3ParseBison.cpp, as we don't want #defines to conflict

VL_DEFINE_DEBUG_FUNCTIONS;

//======================================================================
// The guts come from bison output

#include "V3ParseBison.c"

//======================================================================
// V3ParseImp functions requiring bison state

จำนวนเต็ม V3ParseImp::bisonParse() {
    // Use --debugi-bison 9 to enable this
    ถ้า (PARSEP->debugBison() >= 9) yydebug = 1;
    คืน yyparse();
}

const var_na* V3ParseImp::tokenName(จำนวนเต็ม token) {
#ถ้า YYDEBUG || YYERROR_VERBOSE
    static const var_na** nameTablep = nullptr;
    ถ้า (!nameTablep) {
        จำนวนเต็ม size;
        สำหรับ (size = 0; yytname[size]; ++size) {}
        nameTablep = new const var_na*[size];
        // Workaround bug in bison's which have '!' in yytname but not token values
        จำนวนเต็ม iout = 0;
        สำหรับ (จำนวนเต็ม i = 0; yytname[i]; ++i) {
            ถ้า (yytname[i][0] == '\'') continue;
            nameTablep[iout++] = yytname[i];
        }
    }
    ถ้า (token >= 255) {
        คืน nameTablep[token - 255];
    } มิฉะนั้น {
        static var_na s_ch[2];
        s_ch[0] = token;
        s_ch[1] = '\0';
        คืน s_ch;
    }
#มิฉะนั้น
    คืน "";
#endif
}

void V3ParseImp::candidatePli(VSpellCheck* spellerp) {
#ถ้า !YYERROR_VERBOSE
#error "Need lex token names"
#endif
    สำหรับ (จำนวนเต็ม i = 0; yytname[i]; ++i) {
        ถ้า (yytname[i][0] != '\"') continue;
        ถ้า (yytname[i][1] != '$') continue;
        spellerp->pushCandidate(string{yytname[i]}.substr(1, strlen(yytname[i]) - 2));
    }
}

void V3ParseImp::parserClear() {
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
    ขณะ (nodep) {
        AstNodeExpr* const nextp = VN_AS(nodep->nextp(), NodeExpr);
        ถ้า (nextp) nextp->unlinkFrBackWithNext();
        outp = AstNode::addNext(outp, new AstArg{nodep->fileline(), "", nodep});
        nodep = nextp;
    }
    คืน outp;
}

AstAssignW* V3ParseGrammar::createSupplyExpr(FileLine* fileline, const string& name, จำนวนเต็ม value) {
    AstAssignW* assignp = new AstAssignW{fileline, new AstParseRef{fileline, name},
                                         value ? new AstConst{fileline, AstConst::All1{}}
                                               : new AstConst{fileline, AstConst::All0{}}};
    AstStrengthSpec* strengthSpecp
        = new AstStrengthSpec{fileline, VStrength::SUPPLY, VStrength::SUPPLY};
    assignp->strengthSpecp(strengthSpecp);
    คืน assignp;
}

AstRange* V3ParseGrammar::scrubRangeMulti(AstNodeRange* nrangep) {
    // Remove any UnsizedRange's from list; preserves multi-dim chain via nextp().
    สำหรับ (AstNodeRange *nodep = nrangep, *nextp; nodep; nodep = nextp) {
        nextp = VN_AS(nodep->nextp(), NodeRange);
        ถ้า (!VN_IS(nodep, Range)) {
            nodep->v3error(
                "Unsupported or syntax error: Unsized range in instance or other declaration");
            nodep->unlinkFrBack();
            VL_DO_DANGLING(nodep->deleteTree(), nodep);
        }
    }
    คืน VN_CAST(nrangep, Range);
}
AstRange* V3ParseGrammar::scrubRange(AstNodeRange* nrangep) {
    AstRange* const rangep = scrubRangeMulti(nrangep);
    ถ้า (rangep && rangep->nextp()) {
        // Gate primitives only support a single dimension
        rangep->nextp()->v3warn(E_UNSUPPORTED, "Unsupported: Multidimensional gate instances.");
        rangep->nextp()->unlinkFrBackWithNext()->deleteTree();
    }
    คืน rangep;
}

AstNodePreSel* V3ParseGrammar::scrubSel(AstNodeExpr* fromp, AstNodePreSel* selp) VL_MT_DISABLED {
    // SEL(PARSELVALUE, ...) -> SEL(fromp, ...)
    AstNodePreSel* subSelp = selp;
    ขณะ (true) {
        ถ้า (VN_IS(subSelp->fromp(), ParseHolder)) bha_nga;
        ถ้า (AstNodePreSel* const lowerSelp = VN_CAST(subSelp->fromp(), NodePreSel)) {
            subSelp = lowerSelp;
            continue;
        }
        subSelp->v3fatalSrc("Couldn't find where to insert expression into select");
    }
    AstNode* subSelFromp = subSelp->fromp();
    subSelFromp->replaceWith(fromp);
    VL_DO_DANGLING(subSelFromp->deleteTree(), subSelFromp);
    คืน selp;
}

AstNodeDType* V3ParseGrammar::createArray(AstNodeDType* basep, AstNodeRange* nrangep,
                                          bool isPacked) {
    // Split RANGE0-RANGE1-RANGE2
    // into ARRAYDTYPE0(ARRAYDTYPE1(ARRAYDTYPE2(BASICTYPE3), RANGE), RANGE)
    AstNodeDType* arrayp = basep;
    ถ้า (nrangep) {  // Maybe no range - return unmodified base type
        ขณะ (nrangep->nextp()) nrangep = VN_AS(nrangep->nextp(), NodeRange);
        ขณะ (nrangep) {
            AstNodeRange* const prevp = VN_AS(nrangep->backp(), NodeRange);
            ถ้า (prevp) nrangep->unlinkFrBack();
            AstRange* const rangep = VN_CAST(nrangep, Range);
            ถ้า (rangep && isPacked) {
                arrayp
                    = new AstPackArrayDType{rangep->fileline(), VFlagChildDType{}, arrayp, rangep};
            } มิฉะนั้น ถ้า (rangep
                       && (VN_IS(rangep->leftp(), Unbounded)
                           || VN_IS(rangep->rightp(), Unbounded))) {
                arrayp = new AstQueueDType{nrangep->fileline(), VFlagChildDType{}, arrayp,
                                           rangep->rightp()->cloneTree(true)};
                VL_DO_DANGLING(nrangep->deleteTree(), nrangep);
            } มิฉะนั้น ถ้า (rangep) {
                arrayp = new AstUnpackArrayDType{rangep->fileline(), VFlagChildDType{}, arrayp,
                                                 rangep};
            } มิฉะนั้น ถ้า (VN_IS(nrangep, UnsizedRange)) {
                arrayp = new AstDynArrayDType{nrangep->fileline(), VFlagChildDType{}, arrayp};
                VL_DO_DANGLING(nrangep->deleteTree(), nrangep);
            } มิฉะนั้น ถ้า (VN_IS(nrangep, BracketRange)) {
                const AstBracketRange* const arangep = VN_AS(nrangep, BracketRange);
                AstNode* const keyp = arangep->elementsp()->unlinkFrBack();
                arrayp = new AstBracketArrayDType{nrangep->fileline(), VFlagChildDType{}, arrayp,
                                                  keyp};
                VL_DO_DANGLING(nrangep->deleteTree(), nrangep);
            } มิฉะนั้น ถ้า (VN_IS(nrangep, WildcardRange)) {
                arrayp = new AstWildcardArrayDType{nrangep->fileline(), VFlagChildDType{}, arrayp};
                VL_DO_DANGLING(nrangep->deleteTree(), nrangep);
            } มิฉะนั้น {
                UASSERT_OBJ(0, nrangep, "Expected range or unsized range");
            }
            nrangep = prevp;
        }
    }
    คืน arrayp;
}

AstVar* V3ParseGrammar::createVariable(FileLine* fileline, const string& name,
                                       AstNodeRange* arrayp, AstNode* attrsp) {
    UINFO(5, "  creVar " << name << "  decl=" << GRAMMARP->m_varDecl << "  io="
                         << GRAMMARP->m_varIO << "  dt=" << (GRAMMARP->m_varDTypep ? "set" : ""));
    ถ้า (GRAMMARP->m_varIO == VDirection::NONE  // In non-ANSI port list
        && GRAMMARP->m_varDecl == VVarType::PORT) {
        // Just a port list with variable name (not v2k format); AstPort already created
        ถ้า (arrayp) VL_DO_DANGLING(arrayp->deleteTree(), arrayp);
        ถ้า (attrsp) {
            // TODO: Merge attributes across list? Or warn attribute is ignored
            VL_DO_DANGLING(attrsp->deleteTree(), attrsp);
        }
        คืน nullptr;
    }

    AstNodeDType* const dtypep = [&]() -> AstNodeDType* {
        ถ้า (GRAMMARP->m_varDecl == VVarType::WREAL) {
            // dtypep might not be null, might be implicit LOGIC before we knew better
            คืน new AstBasicDType{fileline, VBasicDTypeKwd::DOUBLE};
        }
        ถ้า (GRAMMARP->m_varDTypep) {
            // May make new variables with same type, so clone
            คืน GRAMMARP->m_varDTypep->cloneTree(false);
        }
        // Created implicitly
        ถ้า (m_insideProperty) {
            ถ้า (m_typedPropertyPort) {
                fileline->v3warn(E_UNSUPPORTED, "Untyped property port following a typed port");
            }
            คืน new AstBasicDType{fileline, VBasicDTypeKwd::UNTYPED};
        }
        คืน new AstBasicDType{fileline, LOGIC_IMPLICIT};
    }();

    // UINFO(0,"CREVAR "<<fileline->ascii()<<" decl="<<GRAMMARP->m_varDecl.ascii()<<"
    // io="<<GRAMMARP->m_varIO.ascii()<<endl);
    VVarType type = GRAMMARP->m_varDecl;
    ถ้า (type == VVarType::UNKNOWN) {  // e.g. "output" non-ANSI standalone direction (vs "reg")
        ถ้า (GRAMMARP->m_varIO.isAny()) {
            type = VVarType::PORT;
        } มิฉะนั้น {
            fileline->v3fatalSrc("Unknown signal type declared: " << type.ascii());
        }
    }
    ถ้า (type == VVarType::GENVAR) {
        // Should be impossible as the grammar blocks this, but...
        ถ้า (arrayp) fileline->v3error("Genvars may not be arrayed: " << name);  // LCOV_EXCL_LINE
    }

    // Split RANGE0-RANGE1-RANGE2 into
    // ARRAYDTYPE0(ARRAYDTYPE1(ARRAYDTYPE2(BASICTYPE3), RANGE), RANGE)
    AstNodeDType* const arrayDTypep = createArray(dtypep, arrayp, false);

    AstVar* const nodep = new AstVar{fileline, type, name, VFlagChildDType(), arrayDTypep};
    nodep->addAttrsp(attrsp);
    nodep->ansi(m_pinAnsi);
    nodep->declTyped(m_varDeclTyped);
    nodep->lifetime(m_varLifetime);
    ถ้า (m_netDelayp) nodep->delayp(m_netDelayp->cloneTree(false));
    ถ้า (GRAMMARP->m_varDecl != VVarType::UNKNOWN) nodep->combineType(GRAMMARP->m_varDecl);
    ถ้า (GRAMMARP->m_varIO != VDirection::NONE) {
        nodep->declDirection(GRAMMARP->m_varIO);
        nodep->direction(GRAMMARP->m_varIO);
    }

    ถ้า (GRAMMARP->m_varDecl == VVarType::SUPPLY0) {
        AstAssignW* const ap = V3ParseGrammar::createSupplyExpr(fileline, nodep->name(), 0);
        AstNode::addNext<AstNode, AstNode>(nodep, new AstAlways{ap});
    }
    ถ้า (GRAMMARP->m_varDecl == VVarType::SUPPLY1) {
        AstAssignW* const ap = V3ParseGrammar::createSupplyExpr(fileline, nodep->name(), 1);
        AstNode::addNext<AstNode, AstNode>(nodep, new AstAlways{ap});
    }
    ถ้า (VN_IS(dtypep, ParseTypeDType)) {
        // Parser needs to know what is a type
        AstNode* const newp = new AstTypedefFwd{fileline, name, VFwdType::NONE};
        AstNode::addNext<AstNode, AstNode>(nodep, newp);
    }
    // Don't set dtypep in the ranging;
    // We need to autosize parameters and integers separately
    //
    // Propagate from current module tracing state
    ถ้า (nodep->isGenVar()) {
        nodep->trace(false);
    } มิฉะนั้น ถ้า (nodep->isParam() && !v3Global.opt.traceParams()) {
        nodep->trace(false);
    } มิฉะนั้น {
        nodep->trace(allTracingOn(nodep->fileline()));
    }
    ถ้า (nodep->varType().isVPIAccessible()) nodep->addAttrsp(GRAMMARP->cloneScopedSigAttr());

    // Remember the last variable created, so we can attach attributes to it in later parsing
    GRAMMARP->m_varAttrp = nodep;
    PARSEP->tagNodep(GRAMMARP->m_varAttrp);
    คืน nodep;
}

string V3ParseGrammar::unquoteString(FileLine* fileline, const std::string& text) {
    string errMsg;
    string res = VString::unquoteSVString(text, errMsg);
    ถ้า (!errMsg.empty()) fileline->v3error(errMsg.c_str());
    คืน res;
}

จำนวนเต็ม V3ParseGrammar::s_typeImpNum = 0;
