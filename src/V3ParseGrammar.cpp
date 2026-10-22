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

#அந்தர்பாவ "V3Ast.h"  // This must be before V3ParseBison.cpp, as we don't want #defines to conflict

VL_DEFINE_DEBUG_FUNCTIONS;

//======================================================================
// The guts come from bison output

#அந்தர்பாவ "V3ParseBison.c"

//======================================================================
// V3ParseImp functions requiring bison state

முழு V3ParseImp::bisonParse() {
    // Use --debugi-bison 9 to enable this
    எனில் (PARSEP->debugBison() >= 9) yydebug = 1;
    திரும்பு yyparse();
}

மாறிலி எழுத்து* V3ParseImp::tokenName(முழு token) {
#எனில் YYDEBUG || YYERROR_VERBOSE
    நிலையான மாறிலி எழுத்து** nameTablep = nullptr;
    எனில் (!nameTablep) {
        முழு size;
        க்கு (size = 0; yytname[size]; ++size) {}
        nameTablep = புதிய மாறிலி எழுத்து*[size];
        // Workaround bug in bison's which have '!' in yytname but not token values
        முழு iout = 0;
        க்கு (முழு i = 0; yytname[i]; ++i) {
            எனில் (yytname[i][0] == '\'') தொடர்;
            nameTablep[iout++] = yytname[i];
        }
    }
    எனில் (token >= 255) {
        திரும்பு nameTablep[token - 255];
    } இல்லையெனில் {
        நிலையான எழுத்து s_ch[2];
        s_ch[0] = token;
        s_ch[1] = '\0';
        திரும்பு s_ch;
    }
#இல்லையெனில்
    திரும்பு "";
#endif
}

வெறுமை V3ParseImp::candidatePli(VSpellCheck* spellerp) {
#எனில் !YYERROR_VERBOSE
#error "Need lex token names"
#endif
    க்கு (முழு i = 0; yytname[i]; ++i) {
        எனில் (yytname[i][0] != '\"') தொடர்;
        எனில் (yytname[i][1] != '$') தொடர்;
        spellerp->தள்ளு_Candidate(string{yytname[i]}.substr(1, strlen(yytname[i]) - 2));
    }
}

வெறுமை V3ParseImp::parserClear() {
    // Clear up any dynamic memory V3Parser required
    VARDTYPE(nullptr);
    GRAMMARP->setNetDelay(nullptr);
    GRAMMARP->setScopedSigAttr(nullptr);
}

//======================================================================
// V3ParseGrammar functions requiring bison state

AstArg* V3ParseGrammar::argWrapList(Ast_முனை_கோவை* nodep) {
    // Convert list of expressions to list of arguments
    AstArg* outp = nullptr;
    போது (nodep) {
        Ast_முனை_கோவை* மாறிலி nextp = VN_AS(nodep->nextp(), முனை_கோவை);
        எனில் (nextp) nextp->unlinkFrBackWithNext();
        outp = Ast_முனை::addNext(outp, புதிய AstArg{nodep->fileline(), "", nodep});
        nodep = nextp;
    }
    திரும்பு outp;
}

AstAssignW* V3ParseGrammar::create_Supply_கோவை(File_வரி* fileline, மாறிலி string& name, முழு மதிப்பு) {
    AstAssignW* assignp = புதிய AstAssignW{fileline, புதிய AstParseRef{fileline, name},
                                         மதிப்பு ? புதிய AstConst{fileline, AstConst::All1{}}
                                               : புதிய AstConst{fileline, AstConst::All0{}}};
    AstStrengthSpec* strengthSpecp
        = புதிய AstStrengthSpec{fileline, VStrength::SUPPLY, VStrength::SUPPLY};
    assignp->strengthSpecp(strengthSpecp);
    திரும்பு assignp;
}

AstRange* V3ParseGrammar::scrubRangeMulti(Ast_முனை_Range* nrangep) {
    // Remove any UnsizedRange's from list; preserves multi-dim chain via nextp().
    க்கு (Ast_முனை_Range *nodep = nrangep, *nextp; nodep; nodep = nextp) {
        nextp = VN_AS(nodep->nextp(), முனை_Range);
        எனில் (!VN_IS(nodep, Range)) {
            nodep->v3error(
                "Unsupported or syntax error: Unsized range in instance or other declaration");
            nodep->unlinkFrBack();
            VL_DO_DANGLING(nodep->delete_மரம்(), nodep);
        }
    }
    திரும்பு VN_CAST(nrangep, Range);
}
AstRange* V3ParseGrammar::scrubRange(Ast_முனை_Range* nrangep) {
    AstRange* மாறிலி rangep = scrubRangeMulti(nrangep);
    எனில் (rangep && rangep->nextp()) {
        // Gate primitives only support a single dimension
        rangep->nextp()->v3warn(E_UNSUPPORTED, "Unsupported: Multidimensional gate instances.");
        rangep->nextp()->unlinkFrBackWithNext()->delete_மரம்();
    }
    திரும்பு rangep;
}

Ast_முனை_Pre_Sel* V3ParseGrammar::scrubSel(Ast_முனை_கோவை* fromp, Ast_முனை_Pre_Sel* selp) VL_MT_DISABLED {
    // SEL(PARSELVALUE, ...) -> SEL(fromp, ...)
    Ast_முனை_Pre_Sel* subSelp = selp;
    போது (உண்மை) {
        எனில் (VN_IS(subSelp->fromp(), ParseHolder)) நிறுத்து;
        எனில் (Ast_முனை_Pre_Sel* மாறிலி lowerSelp = VN_CAST(subSelp->fromp(), முனை_Pre_Sel)) {
            subSelp = lowerSelp;
            தொடர்;
        }
        subSelp->v3fatalSrc("Couldn't find where to insert expression into select");
    }
    Ast_முனை* subSelFromp = subSelp->fromp();
    subSelFromp->replaceWith(fromp);
    VL_DO_DANGLING(subSelFromp->delete_மரம்(), subSelFromp);
    திரும்பு selp;
}

Ast_முனை_DType* V3ParseGrammar::createArray(Ast_முனை_DType* basep, Ast_முனை_Range* nrangep,
                                          தருக்க isPacked) {
    // Split RANGE0-RANGE1-RANGE2
    // into ARRAYDTYPE0(ARRAYDTYPE1(ARRAYDTYPE2(BASICTYPE3), RANGE), RANGE)
    Ast_முனை_DType* arrayp = basep;
    எனில் (nrangep) {  // Maybe no range - return unmodified base type
        போது (nrangep->nextp()) nrangep = VN_AS(nrangep->nextp(), முனை_Range);
        போது (nrangep) {
            Ast_முனை_Range* மாறிலி prevp = VN_AS(nrangep->backp(), முனை_Range);
            எனில் (prevp) nrangep->unlinkFrBack();
            AstRange* மாறிலி rangep = VN_CAST(nrangep, Range);
            எனில் (rangep && isPacked) {
                arrayp
                    = புதிய AstPackArrayDType{rangep->fileline(), VFlagChildDType{}, arrayp, rangep};
            } இல்லையெனில் எனில் (rangep
                       && (VN_IS(rangep->leftp(), Unbounded)
                           || VN_IS(rangep->rightp(), Unbounded))) {
                arrayp = புதிய AstQueueDType{nrangep->fileline(), VFlagChildDType{}, arrayp,
                                           rangep->rightp()->clone_மரம்(உண்மை)};
                VL_DO_DANGLING(nrangep->delete_மரம்(), nrangep);
            } இல்லையெனில் எனில் (rangep) {
                arrayp = புதிய AstUnpackArrayDType{rangep->fileline(), VFlagChildDType{}, arrayp,
                                                 rangep};
            } இல்லையெனில் எனில் (VN_IS(nrangep, UnsizedRange)) {
                arrayp = புதிய AstDynArrayDType{nrangep->fileline(), VFlagChildDType{}, arrayp};
                VL_DO_DANGLING(nrangep->delete_மரம்(), nrangep);
            } இல்லையெனில் எனில் (VN_IS(nrangep, BracketRange)) {
                மாறிலி AstBracketRange* மாறிலி arangep = VN_AS(nrangep, BracketRange);
                Ast_முனை* மாறிலி keyp = arangep->elementsp()->unlinkFrBack();
                arrayp = புதிய AstBracketArrayDType{nrangep->fileline(), VFlagChildDType{}, arrayp,
                                                  keyp};
                VL_DO_DANGLING(nrangep->delete_மரம்(), nrangep);
            } இல்லையெனில் எனில் (VN_IS(nrangep, WildcardRange)) {
                arrayp = புதிய AstWildcardArrayDType{nrangep->fileline(), VFlagChildDType{}, arrayp};
                VL_DO_DANGLING(nrangep->delete_மரம்(), nrangep);
            } இல்லையெனில் {
                UASSERT_OBJ(0, nrangep, "Expected range or unsized range");
            }
            nrangep = prevp;
        }
    }
    திரும்பு arrayp;
}

AstVar* V3ParseGrammar::createVariable(File_வரி* fileline, மாறிலி string& name,
                                       Ast_முனை_Range* arrayp, Ast_முனை* attrsp) {
    UINFO(5, "  creVar " << name << "  decl=" << GRAMMARP->m_varDecl << "  io="
                         << GRAMMARP->m_varIO << "  dt=" << (GRAMMARP->m_varDTypep ? "set" : ""));
    எனில் (GRAMMARP->m_varIO == VDirection::NONE  // In non-ANSI port list
        && GRAMMARP->m_varDecl == VVarType::PORT) {
        // Just a port list with variable name (not v2k format); AstPort already created
        எனில் (arrayp) VL_DO_DANGLING(arrayp->delete_மரம்(), arrayp);
        எனில் (attrsp) {
            // TODO: Merge attributes across list? Or warn attribute is ignored
            VL_DO_DANGLING(attrsp->delete_மரம்(), attrsp);
        }
        திரும்பு nullptr;
    }

    Ast_முனை_DType* மாறிலி dtypep = [&]() -> Ast_முனை_DType* {
        எனில் (GRAMMARP->m_varDecl == VVarType::WREAL) {
            // dtypep might not be null, might be implicit LOGIC before we knew better
            திரும்பு புதிய AstBasicDType{fileline, VBasicDTypeKwd::DOUBLE};
        }
        எனில் (GRAMMARP->m_varDTypep) {
            // May make new variables with same type, so clone
            திரும்பு GRAMMARP->m_varDTypep->clone_மரம்(பொய்);
        }
        // Created implicitly
        எனில் (m_insideProperty) {
            எனில் (m_typedPropertyPort) {
                fileline->v3warn(E_UNSUPPORTED, "Untyped property port following a typed port");
            }
            திரும்பு புதிய AstBasicDType{fileline, VBasicDTypeKwd::UNTYPED};
        }
        திரும்பு புதிய AstBasicDType{fileline, LOGIC_IMPLICIT};
    }();

    // UINFO(0,"CREVAR "<<fileline->ascii()<<" decl="<<GRAMMARP->m_varDecl.ascii()<<"
    // io="<<GRAMMARP->m_varIO.ascii()<<endl);
    VVarType type = GRAMMARP->m_varDecl;
    எனில் (type == VVarType::UNKNOWN) {  // e.g. "output" non-ANSI standalone direction (vs "reg")
        எனில் (GRAMMARP->m_varIO.isAny()) {
            type = VVarType::PORT;
        } இல்லையெனில் {
            fileline->v3fatalSrc("Unknown signal type declared: " << type.ascii());
        }
    }
    எனில் (type == VVarType::GENVAR) {
        // Should be impossible as the grammar blocks this, but...
        எனில் (arrayp) fileline->v3error("Genvars may not be arrayed: " << name);  // LCOV_EXCL_LINE
    }

    // Split RANGE0-RANGE1-RANGE2 into
    // ARRAYDTYPE0(ARRAYDTYPE1(ARRAYDTYPE2(BASICTYPE3), RANGE), RANGE)
    Ast_முனை_DType* மாறிலி arrayDTypep = createArray(dtypep, arrayp, பொய்);

    AstVar* மாறிலி nodep = புதிய AstVar{fileline, type, name, VFlagChildDType(), arrayDTypep};
    nodep->addAttrsp(attrsp);
    nodep->ansi(m_pinAnsi);
    nodep->declTyped(m_varDeclTyped);
    nodep->lifetime(m_varLifetime);
    எனில் (m_netDelayp) nodep->delayp(m_netDelayp->clone_மரம்(பொய்));
    எனில் (GRAMMARP->m_varDecl != VVarType::UNKNOWN) nodep->combineType(GRAMMARP->m_varDecl);
    எனில் (GRAMMARP->m_varIO != VDirection::NONE) {
        nodep->declDirection(GRAMMARP->m_varIO);
        nodep->direction(GRAMMARP->m_varIO);
    }

    எனில் (GRAMMARP->m_varDecl == VVarType::SUPPLY0) {
        AstAssignW* மாறிலி ap = V3ParseGrammar::create_Supply_கோவை(fileline, nodep->name(), 0);
        Ast_முனை::addNext<Ast_முனை, Ast_முனை>(nodep, புதிய AstAlways{ap});
    }
    எனில் (GRAMMARP->m_varDecl == VVarType::SUPPLY1) {
        AstAssignW* மாறிலி ap = V3ParseGrammar::create_Supply_கோவை(fileline, nodep->name(), 1);
        Ast_முனை::addNext<Ast_முனை, Ast_முனை>(nodep, புதிய AstAlways{ap});
    }
    எனில் (VN_IS(dtypep, ParseTypeDType)) {
        // Parser needs to know what is a type
        Ast_முனை* மாறிலி newp = புதிய AstTypedefFwd{fileline, name, VFwdType::NONE};
        Ast_முனை::addNext<Ast_முனை, Ast_முனை>(nodep, newp);
    }
    // Don't set dtypep in the ranging;
    // We need to autosize parameters and integers separately
    //
    // Propagate from current module tracing state
    எனில் (nodep->isGenVar()) {
        nodep->trace(பொய்);
    } இல்லையெனில் எனில் (nodep->isParam() && !v3Global.opt.traceParams()) {
        nodep->trace(பொய்);
    } இல்லையெனில் {
        nodep->trace(allTracingOn(nodep->fileline()));
    }
    எனில் (nodep->varType().isVPIAccessible()) nodep->addAttrsp(GRAMMARP->cloneScopedSigAttr());

    // Remember the last variable created, so we can attach attributes to it in later parsing
    GRAMMARP->m_varAttrp = nodep;
    PARSEP->tagNodep(GRAMMARP->m_varAttrp);
    திரும்பு nodep;
}

string V3ParseGrammar::unquoteString(File_வரி* fileline, மாறிலி std::string& text) {
    string err_செய்தி;
    string res = VString::unquoteSVString(text, err_செய்தி);
    எனில் (!err_செய்தி.empty()) fileline->v3error(err_செய்தி.c_str());
    திரும்பு res;
}

முழு V3ParseGrammar::s_type_Imp_இலக்கம் = 0;
