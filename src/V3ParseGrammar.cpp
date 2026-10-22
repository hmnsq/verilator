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

#અન્તર્ભાવ "V3Ast.h"  // This must be before V3ParseBison.cpp, as we don't want #defines to conflict

VL_DEFINE_DEBUG_FUNCTIONS;

//======================================================================
// The guts come from bison output

#અન્તર્ભાવ "V3ParseBison.c"

//======================================================================
// V3ParseImp functions requiring bison state

પૂર્ણાંક V3ParseImp::bisonParse() {
    // Use --debugi-bison 9 to enable this
    જો (PARSEP->debugBison() >= 9) yydebug = 1;
    પાછા yyparse();
}

અચલ અક્ષર* V3ParseImp::tokenName(પૂર્ણાંક token) {
#જો YYDEBUG || YYERROR_VERBOSE
    સ્થિર અચલ અક્ષર** nameTablep = nullptr;
    જો (!nameTablep) {
        પૂર્ણાંક size;
        માટે (size = 0; yytname[size]; ++size) {}
        nameTablep = નવું અચલ અક્ષર*[size];
        // Workaround bug in bison's which have '!' in yytname but not token values
        પૂર્ણાંક iout = 0;
        માટે (પૂર્ણાંક i = 0; yytname[i]; ++i) {
            જો (yytname[i][0] == '\'') ચાલુ;
            nameTablep[iout++] = yytname[i];
        }
    }
    જો (token >= 255) {
        પાછા nameTablep[token - 255];
    } નહીંતર {
        સ્થિર અક્ષર s_ch[2];
        s_ch[0] = token;
        s_ch[1] = '\0';
        પાછા s_ch;
    }
#નહીંતર
    પાછા "";
#endif
}

શૂન્ય V3ParseImp::candidatePli(VSpellCheck* spellerp) {
#જો !YYERROR_VERBOSE
#error "Need lex token names"
#endif
    માટે (પૂર્ણાંક i = 0; yytname[i]; ++i) {
        જો (yytname[i][0] != '\"') ચાલુ;
        જો (yytname[i][1] != '$') ચાલુ;
        spellerp->ધકેલો_Candidate(string{yytname[i]}.substr(1, strlen(yytname[i]) - 2));
    }
}

શૂન્ય V3ParseImp::parserClear() {
    // Clear up any dynamic memory V3Parser required
    VARDTYPE(nullptr);
    GRAMMARP->setNetDelay(nullptr);
    GRAMMARP->setScopedSigAttr(nullptr);
}

//======================================================================
// V3ParseGrammar functions requiring bison state

AstArg* V3ParseGrammar::argWrapList(Ast_ગાંઠ_પદાવલિ* nodep) {
    // Convert list of expressions to list of arguments
    AstArg* outp = nullptr;
    જ્યારે (nodep) {
        Ast_ગાંઠ_પદાવલિ* અચલ nextp = VN_AS(nodep->nextp(), ગાંઠ_પદાવલિ);
        જો (nextp) nextp->unlinkFrBackWithNext();
        outp = Ast_ગાંઠ::addNext(outp, નવું AstArg{nodep->fileline(), "", nodep});
        nodep = nextp;
    }
    પાછા outp;
}

AstAssignW* V3ParseGrammar::create_Supply_પદાવલિ(File_લીટી* fileline, અચલ string& name, પૂર્ણાંક કિંમત) {
    AstAssignW* assignp = નવું AstAssignW{fileline, નવું AstParseRef{fileline, name},
                                         કિંમત ? નવું AstConst{fileline, AstConst::All1{}}
                                               : નવું AstConst{fileline, AstConst::All0{}}};
    AstStrengthSpec* strengthSpecp
        = નવું AstStrengthSpec{fileline, VStrength::SUPPLY, VStrength::SUPPLY};
    assignp->strengthSpecp(strengthSpecp);
    પાછા assignp;
}

AstRange* V3ParseGrammar::scrubRangeMulti(Ast_ગાંઠ_Range* nrangep) {
    // Remove any UnsizedRange's from list; preserves multi-dim chain via nextp().
    માટે (Ast_ગાંઠ_Range *nodep = nrangep, *nextp; nodep; nodep = nextp) {
        nextp = VN_AS(nodep->nextp(), ગાંઠ_Range);
        જો (!VN_IS(nodep, Range)) {
            nodep->v3error(
                "Unsupported or syntax error: Unsized range in instance or other declaration");
            nodep->unlinkFrBack();
            VL_DO_DANGLING(nodep->delete_વૃક્ષ(), nodep);
        }
    }
    પાછા VN_CAST(nrangep, Range);
}
AstRange* V3ParseGrammar::scrubRange(Ast_ગાંઠ_Range* nrangep) {
    AstRange* અચલ rangep = scrubRangeMulti(nrangep);
    જો (rangep && rangep->nextp()) {
        // Gate primitives only support a single dimension
        rangep->nextp()->v3warn(E_UNSUPPORTED, "Unsupported: Multidimensional gate instances.");
        rangep->nextp()->unlinkFrBackWithNext()->delete_વૃક્ષ();
    }
    પાછા rangep;
}

Ast_ગાંઠ_Pre_Sel* V3ParseGrammar::scrubSel(Ast_ગાંઠ_પદાવલિ* fromp, Ast_ગાંઠ_Pre_Sel* selp) VL_MT_DISABLED {
    // SEL(PARSELVALUE, ...) -> SEL(fromp, ...)
    Ast_ગાંઠ_Pre_Sel* subSelp = selp;
    જ્યારે (સાચું) {
        જો (VN_IS(subSelp->fromp(), ParseHolder)) તોડો;
        જો (Ast_ગાંઠ_Pre_Sel* અચલ lowerSelp = VN_CAST(subSelp->fromp(), ગાંઠ_Pre_Sel)) {
            subSelp = lowerSelp;
            ચાલુ;
        }
        subSelp->v3fatalSrc("Couldn't find where to insert expression into select");
    }
    Ast_ગાંઠ* subSelFromp = subSelp->fromp();
    subSelFromp->replaceWith(fromp);
    VL_DO_DANGLING(subSelFromp->delete_વૃક્ષ(), subSelFromp);
    પાછા selp;
}

Ast_ગાંઠ_DType* V3ParseGrammar::createArray(Ast_ગાંઠ_DType* basep, Ast_ગાંઠ_Range* nrangep,
                                          તાર્કિક isPacked) {
    // Split RANGE0-RANGE1-RANGE2
    // into ARRAYDTYPE0(ARRAYDTYPE1(ARRAYDTYPE2(BASICTYPE3), RANGE), RANGE)
    Ast_ગાંઠ_DType* arrayp = basep;
    જો (nrangep) {  // Maybe no range - return unmodified base type
        જ્યારે (nrangep->nextp()) nrangep = VN_AS(nrangep->nextp(), ગાંઠ_Range);
        જ્યારે (nrangep) {
            Ast_ગાંઠ_Range* અચલ prevp = VN_AS(nrangep->backp(), ગાંઠ_Range);
            જો (prevp) nrangep->unlinkFrBack();
            AstRange* અચલ rangep = VN_CAST(nrangep, Range);
            જો (rangep && isPacked) {
                arrayp
                    = નવું AstPackArrayDType{rangep->fileline(), VFlagChildDType{}, arrayp, rangep};
            } નહીંતર જો (rangep
                       && (VN_IS(rangep->leftp(), Unbounded)
                           || VN_IS(rangep->rightp(), Unbounded))) {
                arrayp = નવું AstQueueDType{nrangep->fileline(), VFlagChildDType{}, arrayp,
                                           rangep->rightp()->clone_વૃક્ષ(સાચું)};
                VL_DO_DANGLING(nrangep->delete_વૃક્ષ(), nrangep);
            } નહીંતર જો (rangep) {
                arrayp = નવું AstUnpackArrayDType{rangep->fileline(), VFlagChildDType{}, arrayp,
                                                 rangep};
            } નહીંતર જો (VN_IS(nrangep, UnsizedRange)) {
                arrayp = નવું AstDynArrayDType{nrangep->fileline(), VFlagChildDType{}, arrayp};
                VL_DO_DANGLING(nrangep->delete_વૃક્ષ(), nrangep);
            } નહીંતર જો (VN_IS(nrangep, BracketRange)) {
                અચલ AstBracketRange* અચલ arangep = VN_AS(nrangep, BracketRange);
                Ast_ગાંઠ* અચલ keyp = arangep->elementsp()->unlinkFrBack();
                arrayp = નવું AstBracketArrayDType{nrangep->fileline(), VFlagChildDType{}, arrayp,
                                                  keyp};
                VL_DO_DANGLING(nrangep->delete_વૃક્ષ(), nrangep);
            } નહીંતર જો (VN_IS(nrangep, WildcardRange)) {
                arrayp = નવું AstWildcardArrayDType{nrangep->fileline(), VFlagChildDType{}, arrayp};
                VL_DO_DANGLING(nrangep->delete_વૃક્ષ(), nrangep);
            } નહીંતર {
                UASSERT_OBJ(0, nrangep, "Expected range or unsized range");
            }
            nrangep = prevp;
        }
    }
    પાછા arrayp;
}

AstVar* V3ParseGrammar::createVariable(File_લીટી* fileline, અચલ string& name,
                                       Ast_ગાંઠ_Range* arrayp, Ast_ગાંઠ* attrsp) {
    UINFO(5, "  creVar " << name << "  decl=" << GRAMMARP->m_varDecl << "  io="
                         << GRAMMARP->m_varIO << "  dt=" << (GRAMMARP->m_varDTypep ? "set" : ""));
    જો (GRAMMARP->m_varIO == VDirection::NONE  // In non-ANSI port list
        && GRAMMARP->m_varDecl == VVarType::PORT) {
        // Just a port list with variable name (not v2k format); AstPort already created
        જો (arrayp) VL_DO_DANGLING(arrayp->delete_વૃક્ષ(), arrayp);
        જો (attrsp) {
            // TODO: Merge attributes across list? Or warn attribute is ignored
            VL_DO_DANGLING(attrsp->delete_વૃક્ષ(), attrsp);
        }
        પાછા nullptr;
    }

    Ast_ગાંઠ_DType* અચલ dtypep = [&]() -> Ast_ગાંઠ_DType* {
        જો (GRAMMARP->m_varDecl == VVarType::WREAL) {
            // dtypep might not be null, might be implicit LOGIC before we knew better
            પાછા નવું AstBasicDType{fileline, VBasicDTypeKwd::DOUBLE};
        }
        જો (GRAMMARP->m_varDTypep) {
            // May make new variables with same type, so clone
            પાછા GRAMMARP->m_varDTypep->clone_વૃક્ષ(ખોટું);
        }
        // Created implicitly
        જો (m_insideProperty) {
            જો (m_typedPropertyPort) {
                fileline->v3warn(E_UNSUPPORTED, "Untyped property port following a typed port");
            }
            પાછા નવું AstBasicDType{fileline, VBasicDTypeKwd::UNTYPED};
        }
        પાછા નવું AstBasicDType{fileline, LOGIC_IMPLICIT};
    }();

    // UINFO(0,"CREVAR "<<fileline->ascii()<<" decl="<<GRAMMARP->m_varDecl.ascii()<<"
    // io="<<GRAMMARP->m_varIO.ascii()<<endl);
    VVarType type = GRAMMARP->m_varDecl;
    જો (type == VVarType::UNKNOWN) {  // e.g. "output" non-ANSI standalone direction (vs "reg")
        જો (GRAMMARP->m_varIO.isAny()) {
            type = VVarType::PORT;
        } નહીંતર {
            fileline->v3fatalSrc("Unknown signal type declared: " << type.ascii());
        }
    }
    જો (type == VVarType::GENVAR) {
        // Should be impossible as the grammar blocks this, but...
        જો (arrayp) fileline->v3error("Genvars may not be arrayed: " << name);  // LCOV_EXCL_LINE
    }

    // Split RANGE0-RANGE1-RANGE2 into
    // ARRAYDTYPE0(ARRAYDTYPE1(ARRAYDTYPE2(BASICTYPE3), RANGE), RANGE)
    Ast_ગાંઠ_DType* અચલ arrayDTypep = createArray(dtypep, arrayp, ખોટું);

    AstVar* અચલ nodep = નવું AstVar{fileline, type, name, VFlagChildDType(), arrayDTypep};
    nodep->addAttrsp(attrsp);
    nodep->ansi(m_pinAnsi);
    nodep->declTyped(m_varDeclTyped);
    nodep->lifetime(m_varLifetime);
    જો (m_netDelayp) nodep->delayp(m_netDelayp->clone_વૃક્ષ(ખોટું));
    જો (GRAMMARP->m_varDecl != VVarType::UNKNOWN) nodep->combineType(GRAMMARP->m_varDecl);
    જો (GRAMMARP->m_varIO != VDirection::NONE) {
        nodep->declDirection(GRAMMARP->m_varIO);
        nodep->direction(GRAMMARP->m_varIO);
    }

    જો (GRAMMARP->m_varDecl == VVarType::SUPPLY0) {
        AstAssignW* અચલ ap = V3ParseGrammar::create_Supply_પદાવલિ(fileline, nodep->name(), 0);
        Ast_ગાંઠ::addNext<Ast_ગાંઠ, Ast_ગાંઠ>(nodep, નવું AstAlways{ap});
    }
    જો (GRAMMARP->m_varDecl == VVarType::SUPPLY1) {
        AstAssignW* અચલ ap = V3ParseGrammar::create_Supply_પદાવલિ(fileline, nodep->name(), 1);
        Ast_ગાંઠ::addNext<Ast_ગાંઠ, Ast_ગાંઠ>(nodep, નવું AstAlways{ap});
    }
    જો (VN_IS(dtypep, ParseTypeDType)) {
        // Parser needs to know what is a type
        Ast_ગાંઠ* અચલ newp = નવું AstTypedefFwd{fileline, name, VFwdType::NONE};
        Ast_ગાંઠ::addNext<Ast_ગાંઠ, Ast_ગાંઠ>(nodep, newp);
    }
    // Don't set dtypep in the ranging;
    // We need to autosize parameters and integers separately
    //
    // Propagate from current module tracing state
    જો (nodep->isGenVar()) {
        nodep->trace(ખોટું);
    } નહીંતર જો (nodep->isParam() && !v3Global.opt.traceParams()) {
        nodep->trace(ખોટું);
    } નહીંતર {
        nodep->trace(allTracingOn(nodep->fileline()));
    }
    જો (nodep->varType().isVPIAccessible()) nodep->addAttrsp(GRAMMARP->cloneScopedSigAttr());

    // Remember the last variable created, so we can attach attributes to it in later parsing
    GRAMMARP->m_varAttrp = nodep;
    PARSEP->tagNodep(GRAMMARP->m_varAttrp);
    પાછા nodep;
}

string V3ParseGrammar::unquoteString(File_લીટી* fileline, અચલ std::string& text) {
    string err_સંદેશ;
    string res = VString::unquoteSVString(text, err_સંદેશ);
    જો (!err_સંદેશ.empty()) fileline->v3error(err_સંદેશ.c_str());
    પાછા res;
}

પૂર્ણાંક V3ParseGrammar::s_type_Imp_અંક = 0;
