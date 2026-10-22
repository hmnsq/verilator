// -*- mode: C++; c-file-style: "cc-mode" -*-
//*************************************************************************
// DESCRIPTION: Verilator: Emit C++ for tree
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

#شامل "V3PchAstMT.h"

#شامل "V3EmitCFunc.h"

#شامل <map>
#شامل <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// EmitCFunc

منطقی EmitCFunc::emitSimpleOk(Ast_گرہ_اظہاریہ* nodep) {
    // Can we put out a simple (A + B) instead of VL_ADD_III(A,B)?
    اگر (nodep->emitSimpleOperator() == "") واپس جھوٹ;
    اگر (nodep->isWide()) واپس جھوٹ;
    اگر (nodep->op1p() && nodep->op1p()->isWide()) واپس جھوٹ;
    اگر (nodep->op2p() && nodep->op2p()->isWide()) واپس جھوٹ;
    اگر (nodep->op3p() && nodep->op3p()->isWide()) واپس جھوٹ;
    واپس سچ;
}

خالی EmitCFunc::emitOpName(Ast_گرہ* nodep, ثابت string& format, Ast_گرہ* lhsp, Ast_گرہ* rhsp,
                           Ast_گرہ* thsp) {
    // Look at emitOperator() format for term/uni/dual/triops,
    // and write out appropriate text.
    //  %n*     node
    //   %nq      emitIQW on the [node]
    //   %nw      width in bits
    //   %nW      width in words
    //   %ni      iterate
    //  %l*     lhsp - if appropriate, then second char as above
    //  %r*     rhsp - if appropriate, then second char as above
    //  %t*     thsp - if appropriate, then second char as above
    //  %k      Potential line break
    //  %P      Wide temporary name
    //  ,       Commas suppressed if the previous field is suppressed
    string برآمد;
    putnbs(nodep, "");

    منطقی needComma = جھوٹ;
    منطقی استعمال_کرتا_قطار = جھوٹ;
    string اگلا_Comma;
    خودکار comma_برآمد = [&برآمد, &اگلا_Comma]() {
        اگر (!اگلا_Comma.خالی_خانے()) {
            برآمد += اگلا_Comma;
            اگلا_Comma = "";
        }
    };

    خودکار put_برآمد = [یہ, &برآمد]() {
        اگر (!برآمد.خالی_خانے()) puts(برآمد);
        برآمد = "";
    };

    برائے (string::const_iterator pos = format.begin(); pos != format.انتہا(); ++pos) {
        اگر (pos[0] == ',') {
            // Remember we need to add one, but don't do yet to avoid ",)"
            اگر (needComma) {
                اگر (pos[1] == ' ') {
                    اگلا_Comma = ", ";
                } ورنہ {
                    اگلا_Comma = ",";
                }
                needComma = جھوٹ;
            }
            اگر (pos[1] == ' ') ++pos;  // Must do even if no nextComma
        } ورنہ اگر (pos[0] == '%') {
            ++pos;
            منطقی detail = جھوٹ;
            Ast_گرہ* detailp = خالی_اشاریہ;
            انتخاب (pos[0]) {
            حالت '%': برآمد += '%'; توڑو;
            حالت 'k':
                put_برآمد();
                putbs("");
                توڑو;
            حالت 'n':
                detail = سچ;
                detailp = nodep;
                توڑو;
            حالت 'l':
                detail = سچ;
                detailp = lhsp;
                توڑو;
            حالت 'r':
                detail = سچ;
                detailp = rhsp;
                توڑو;
            حالت 't':
                detail = سچ;
                detailp = thsp;
                توڑو;
            حالت 'P':
                اگر (nodep->isWide() && !استعمال_کرتا_قطار) {
                    UASSERT_OBJ(m_wide_عارضی_Refp, nodep,
                                "Wide op " << nodep->prettyTypeName()
                                           << " w/ no temp, perhaps missing op in V3EmitC?");
                    comma_برآمد();
                    put_برآمد();
                    اگر (!m_wide_عارضی_Refp->self_اشارہ_گر().is_خالی_خانے()) {
                        emitDereference(m_wide_عارضی_Refp,
                                        m_wide_عارضی_Refp->self_اشارہ_گر_Protect(m_useSelfForThis));
                    }
                    برآمد += varName(m_wide_عارضی_Refp->varp());
                    m_wide_عارضی_Refp = خالی_اشاریہ;
                    needComma = سچ;
                } ورنہ اگر (استعمال_کرتا_قطار) {
                    comma_برآمد();
                    put_برآمد();
                    iterate_And_اگلا_Const_Null(nodep->backp()->op2p());
                    needComma = سچ;
                }

                توڑو;
            طےشدہ: nodep->v3fatalSrc("Unknown emitOperator format code: %" << pos[0]); توڑو;
            }
            اگر (detail) {
                // Get next letter of %[nlrt]
                ++pos;
                انتخاب (pos[0]) {
                حالت 'q':
                    put_برآمد();
                    // If we are assigning this to a queue we need to get the return type
                    اگر (VN_IS(detailp->backp(), تفویض)
                        && VN_IS(detailp->backp()->op2p()->dtypep()->skipRefp(), قطار_DType)) {
                        puts("R");  // R for queue
                        استعمال_کرتا_قطار = سچ;
                    } ورنہ اگر (VN_IS(detailp->dtypep()->skipRefp(), قطار_DType)
                               || VN_IS(detailp->dtypep()->skipRefp(), دھارا_DType)) {
                        puts("R");  // R for queue
                    } ورنہ {
                        emitIQW(detailp);
                    }
                    توڑو;
                حالت 'w':
                    comma_برآمد();
                    برآمد += cvt_مقصد_Str(detailp->چوڑائی_Min());
                    needComma = سچ;
                    توڑو;
                حالت 'W':
                    اگر (lhsp->isWide()) {
                        comma_برآمد();
                        برآمد += cvt_مقصد_Str(lhsp->چوڑائی_الفاظ());
                        needComma = سچ;
                    } ورنہ اگر (VN_IS(lhsp, دھارا_R)) {
                        comma_برآمد();
                        برآمد += cvt_مقصد_Str(rhsp->چوڑائی_الفاظ());
                        needComma = سچ;
                    }
                    توڑو;
                حالت 'i':
                    comma_برآمد();
                    UASSERT_OBJ(detailp, nodep, "emitOperator() references undef node");
                    put_برآمد();
                    iterate_And_اگلا_Const_Null(detailp);
                    needComma = سچ;
                    توڑو;
                طےشدہ:
                    nodep->v3fatalSrc("Unknown emitOperator format code: %[nlrt]" << pos[0]);
                    توڑو;
                }
            }
        } ورنہ اگر (pos[0] == ')') {
            اگلا_Comma = "";
            برآمد += ')';
        } ورنہ اگر (pos[0] == '(') {
            comma_برآمد();
            needComma = جھوٹ;
            برآمد += '(';
        } ورنہ {
            // Normal text
            اگر (std::حرف_یا_ہندسہ(pos[0])) needComma = سچ;
            comma_برآمد();
            برآمد += pos[0];
        }
    }
    put_برآمد();
}

منطقی EmitCFunc::display_Emit_سرخی(Ast_گرہ* nodep) {
    منطقی isStmt = جھوٹ;
    اگر (ثابت AstFScanF* ثابت dispp = VN_CAST(nodep, FScanF)) {
        isStmt = جھوٹ;
        putns(nodep, "VL_FSCANF_INX(");
        iterateConst(dispp->filep());
        puts(",");
    } ورنہ اگر (ثابت AstSScanF* ثابت dispp = VN_CAST(nodep, SScanF)) {
        isStmt = جھوٹ;
        putns(nodep, "VL_SSCANF_I");
        emitIQW(dispp->fromp());
        puts("NX(");
        puts(cvt_مقصد_Str(dispp->fromp()->چوڑائی_Min()));
        puts(",");
        iterateConst(dispp->fromp());
        puts(",");
    } ورنہ اگر (ثابت AstDisplay* ثابت dispp = VN_CAST(nodep, Display)) {
        isStmt = سچ;
        اگر (dispp->filep()) {
            putns(nodep, "VL_FWRITEF_NX(");
            iterateConst(dispp->filep());
            puts(",");
        } ورنہ {
            putns(nodep, "VL_WRITEF_NX(");
        }
    } ورنہ اگر (ثابت AstSFormat* ثابت dispp = VN_CAST(nodep, SFormat)) {
        isStmt = سچ;
        puts("VL_SFORMAT_NX(");
        اگر (!dispp->lhsp()->dtypep()->isString()) {
            puts(cvt_مقصد_Str(dispp->lhsp()->چوڑائی_Min()));
            putbs(",");
        }
        iterateConst(dispp->lhsp());
        emitDatap(dispp->lhsp());
        putbs(",");
    } ورنہ اگر (VN_IS(nodep, SFormatF)) {
        isStmt = جھوٹ;
        putns(nodep, "VL_SFORMATF_N_NX(");
    } ورنہ {
        nodep->v3fatalSrc("Unknown displayEmit node type");
    }
    واپس isStmt;
}

خالی EmitCFunc::display_گرہ(Ast_گرہ* nodep, AstSFormatF* fmtp,  // fmtp is nullptr for AstScan
                            ثابت string& vformat, Ast_گرہ* exprsp, منطقی is_جائزہ) {
    // Check format, if it exists
    ثابت منطقی اظہاریہ_Format_2 = fmtp && fmtp->اظہاریہ_Format_2();
    منطقی needsScope = اظہاریہ_Format_2;
    منطقی needsTimescale = اظہاریہ_Format_2;
    صحیح format_دلائل = 0;
    اگر (!اظہاریہ_Format_2) {
        // Check display and argument count to head-off later runtime issues
        منطقی اندر_Pct = جھوٹ;
        منطقی ignore = جھوٹ;
        برائے (حرف ch : vformat) {
            // UINFO(1, "Parse '" << *pos << "'  IP" << inPct << " List " << cvtToHex(elistp));
            اگر (!اندر_Pct && ch == '%') {
                اندر_Pct = سچ;
                ignore = جھوٹ;
            } ورنہ اگر (اندر_Pct && (std::ہندسہ_ہے(ch) || ch == '.' || ch == '-')) {
            } ورنہ اگر (!اندر_Pct) {  // Normal text
            } ورنہ {  // Format character
                اندر_Pct = جھوٹ;
                ch = std::tolower(ch);
                انتخاب (ch) {
                حالت '%': توڑو;
                حالت '*':
                    اندر_Pct = سچ;  // Get more digits
                    ignore = سچ;
                    توڑو;
                حالت 'm': needsScope = سچ; توڑو;
                حالت 't':
                    needsTimescale = سچ;
                    ++format_دلائل;
                    توڑو;
                طےشدہ:
                    اگر (!ignore && V3_نمبر::displayedFmtHasArg(ch, is_جائزہ)) ++format_دلائل;
                    توڑو;
                }
            }
        }
    }

    اگر (vformat.خالی_خانے() && VN_IS(nodep, Display))  // not fscanf etc, as they need to return value
        واپس;  // NOP

    ثابت منطقی isStmt = display_Emit_سرخی(nodep);

    اگر (اظہاریہ_Format_2) {
        UASSERT_OBJ(exprsp, nodep, "Missing format expression");
        iterateConst(exprsp);
        emitDatap(exprsp);
        exprsp = exprsp->nextp();
    } ورنہ {
        ofp()->putsQuoted(vformat);
    }

    صحیح اظہاریہ_دلائل = 0;
    برائے (Ast_گرہ* argp = exprsp; argp; argp = argp->nextp()) ++اظہاریہ_دلائل;
    اگر (VL_UNCOVERABLE(!اظہاریہ_Format_2 && اظہاریہ_دلائل != format_دلائل)) {  // LCOV_EXCL_START
        // expectDisplay() checks this first, so probably internal error if found here
        nodep->v3error("Internal: Missing or extra arguments for $display-like format, "
                       << " expression had " << اظہاریہ_دلائل << ", format had " << format_دلائل);
    }  // LCOV_EXCL_STOP

    // argc for error check. Also MSVC++ requires va_args to not be off a reference
    صحیح argc = 0;
    اگر (needsScope) ++argc;
    اگر (needsTimescale) ++argc;
    برائے (Ast_گرہ* argp = exprsp; argp; argp = argp->nextp()) {
        ++argc;
        ثابت AstSFormatArg* ثابت fargp = VN_CAST(argp, SFormatArg);
        اگر (fargp && fargp->formatAttr().isEnum()) ++argc;  // Additional name argument
    }
    ofp()->puts("," + std::مقصد_string(argc));

    اگر (needsScope) {
        AstScopeName* ثابت scopenamep = fmtp ? fmtp->scopeNamep() : خالی_اشاریہ;
        UASSERT_OBJ(scopenamep, nodep, "Display with %m but no AstScopeName");
        ofp()->puts(", '"s + VFormatAttr{VFormatAttr::SCOPE}.ascii() + "'");
        اگر (scopenamep->selfRelative()) {
            // vlNamep is catName(name(), instance), so starts with '.' when the model is unnamed
            ثابت string namep = string{m_useSelfForThis ? "vlSelf" : "this"} + "->vlNamep";
            ofp()->puts(",(vlSymsp->name()[0] ? " + namep + " : " + namep + " + 1),");
            ofp()->putsQuoted(scopenamep->scopeAttr());
        } ورنہ {
            ofp()->puts(",vlSymsp->name(),");
            ofp()->putsQuoted(scopenamep->scopePrettySymName());
        }
    }

    اگر (needsTimescale) {
        VTimescale timeunit = VTimescale::NONE;
        اگر (ثابت AstDisplay* ثابت anodep = VN_CAST(nodep, Display)) {
            timeunit = anodep->fmtp()->timeunit();
        } ورنہ اگر (ثابت AstSFormat* ثابت anodep = VN_CAST(nodep, SFormat)) {
            timeunit = anodep->fmtp()->timeunit();
        } ورنہ اگر (ثابت AstSScanF* ثابت anodep = VN_CAST(nodep, SScanF)) {
            timeunit = anodep->timeunit();
        } ورنہ اگر (ثابت AstSFormatF* ثابت anodep = VN_CAST(nodep, SFormatF)) {
            timeunit = anodep->timeunit();
        }
        UASSERT_OBJ(!timeunit.isNone(), nodep,
                    "Use of %t must be under AstDisplay, AstSFormat, or AstSFormatF, or "
                    "SScanF, and timeunit set");
        ofp()->puts(", '"s + VFormatAttr{VFormatAttr::TIMEUNIT}.ascii() + "'");
        ofp()->puts(","s + cvt_مقصد_Str((صحیح)timeunit.power_کا_Ten()));
    }

    // Arguments
    برائے (Ast_گرہ* argp = exprsp; argp; argp = argp->nextp()) {
        ofp()->indentInc();
        ofp()->putbs("");
        AstSFormatArg* ثابت fargp = VN_CAST(argp, SFormatArg);
        Ast_گرہ* ثابت subargp = fargp ? fargp->exprp() : argp;
        ثابت VFormatAttr formatAttr = AstSFormatArg::formatAttrDefauled(fargp, subargp->dtypep());
        puts(", '"s + formatAttr.ascii() + '\'');
        اگر (formatAttr.isSigned() || formatAttr.isUnsigned() || formatAttr.isEnum())
            puts("," + cvt_مقصد_Str(subargp->چوڑائی_Min()));
        ثابت منطقی addrof = is_جائزہ || formatAttr.isString() || formatAttr.isComplex();
        ثابت منطقی wideEnum = formatAttr.isEnum() && subargp->isWide();
        puts(",");
        اگر (wideEnum) puts("static_cast<const EData*>(");
        اگر (addrof) puts("&(");
        اگر (VN_IS(subargp, دھارا_R))
            emit_دھارا_R(
                VN_CAST(subargp, دھارا_R),
                nodep);  // This has to be done here because streamR doesn't know what it returns
        ورنہ { iterateConst(subargp); }
        اگر (addrof) puts(")");
        اگر (!addrof) emitDatap(argp);
        اگر (wideEnum) puts(")");
        اگر (formatAttr.isEnum()) {
            puts(", '"s + VFormatAttr{VFormatAttr::STRING}.ascii() + "', &(");
            iterateConst(fargp->namep());
            puts(")");
        }
        ofp()->indentDec();
    }

    // End
    اگر (isStmt) {
        puts(");\n");
    } ورنہ {
        puts(") ");
    }
}

خالی EmitCFunc::emit_CCall_دلائل(ثابت Ast_گرہ_CCall* nodep, ثابت string& self_اشارہ_گر,
                              منطقی اندر_Process) {
    putns(nodep, "(");
    منطقی comma = جھوٹ;
    اگر (nodep->funcp()->isLoose() && !nodep->funcp()->isStatic()) {
        UASSERT_OBJ(!self_اشارہ_گر.خالی_خانے(), nodep, "Call to loose method without self pointer");
        puts(self_اشارہ_گر);
        comma = سچ;
    }
    اگر (nodep->funcp()->needProcess()) {
        اگر (comma) puts(", ");
        اگر (VN_IS(nodep->backp(), CAwait) || !nodep->funcp()->isCoroutine()) {
            puts("vlProcess");
        } ورنہ اگر (اندر_Process) {
            puts("std::make_shared<VlProcess>(vlProcess)");
        } ورنہ {
            puts("std::make_shared<VlProcess>()");
        }
        comma = سچ;
    }
    اگر (!nodep->argTypes().خالی_خانے()) {
        اگر (comma) puts(", ");
        puts(nodep->argTypes());
        comma = سچ;
    }
    put_Comma_Iterate_اگلا(nodep->argsp(), comma);
    puts(")");
}

std::string EmitCFunc::dereferenceString(ثابت std::string& اشارہ_گر) ثابت {
    اگر (اشارہ_گر[0] == '(' && اشارہ_گر[1] == '&') {
        // remove "address of" followed by immediate dereference
        // Note: this relies on only the form '(&OBJECT)' being used by Verilator
        واپس اشارہ_گر.substr(2, اشارہ_گر.لمبائی_2() - 3) + '.';
    } ورنہ {
        اگر (اشارہ_گر == "vlSelf" && m_usevlSelfRef) {
            واپس "vlSelfRef.";
        } ورنہ {
            واپس اشارہ_گر + "->";
        }
    }
}
خالی EmitCFunc::emitDereference(Ast_گرہ* nodep, ثابت string& اشارہ_گر) {
    putns(nodep, dereferenceString(اشارہ_گر));
}

خالی EmitCFunc::emitCvtPackStr(Ast_گرہ* nodep) {
    اگر (ثابت AstConst* ثابت constp = VN_CAST(nodep, Const)) {
        emitConstantString(constp);
    } ورنہ اگر (VN_IS(nodep->dtypep(), دھارا_DType)) {
        putnbs(nodep, "VL_CVT_PACK_STR_ND(");
        iterate_And_اگلا_Const_Null(nodep);
        puts(")");
    } ورنہ {
        putnbs(nodep, "VL_CVT_PACK_STR_N");
        emitIQW(nodep);
        puts("(");
        اگر (nodep->isWide()) {
            // Note argument width, not node width (which is always 32)
            puts(cvt_مقصد_Str(nodep->چوڑائی_الفاظ()));
            puts(", ");
        }
        iterate_And_اگلا_Const_Null(nodep);
        puts(")");
    }
}

خالی EmitCFunc::emitConstant(AstConst* nodep) {
    // Put out constant set to the specified variable, or given variable in a string
    ثابت V3_نمبر& عدد = nodep->عدد();
    اگر (عدد.isFourState()) {
        nodep->v3warn(E_UNSUPPORTED, "Unsupported: 4-state numbers in this context");
        واپس;
    }
    putns(nodep, عدد.emitC());
}

خالی EmitCFunc::emitConstantString(ثابت AstConst* nodep) {
    // Const might be a Verilog array-type string, but need to always output std::string
    putnbs(nodep, "std::string{");
    ثابت string str = nodep->عدد().مقصد_String();
    اگر (!str.خالی_خانے()) putsQuoted(str);
    puts("}");
}

خالی EmitCFunc::emitVarReset(ثابت string& سابقہ, AstVar* varp, منطقی constructing) {
    // 'constructing' indicates that the object was just constructed, so if it is a string or
    // something that starts off clear already, no need to clear it again
    Ast_گرہ_DType* ثابت dtypep = varp->dtypep()->skipRefp();
    ثابت string vlSelf = VSelf_اشارہ_گر_متن::replaceThis(m_useSelfForThis, "this->");
    ثابت string varNameProtected
        = ((VN_IS(m_modp, Class) || varp->isFuncLocal()) || !سابقہ.خالی_خانے())
              ? varName(varp)
              : vlSelf + varName(varp);
    ثابت string new_سابقہ = سابقہ + varNameProtected;
    اگر (varp->isIO() && m_modp->is_چوٹی() && optSystemC()) {
        // System C top I/O doesn't need loading, as the lower level subinst code does it.}
    } ورنہ اگر (varp->isParam()) {
        UASSERT_OBJ(varp->valuep(), varp, "No init for a param?");
        // If a simple CONST value we initialize it using an enum
        // If an ARRAYINIT we initialize it using an initial block similar to a signal
        // puts("// parameter "+varp->nameProtect()+" = "+varp->valuep()->name()+"\n");
    } ورنہ {
        putns(varp,
              emitVarResetRecurse(varp, constructing, new_سابقہ, dtypep, 0, "", varp->valuep()));
    }
}

string EmitCFunc::emitVarResetRecurse(ثابت AstVar* varp, منطقی constructing,
                                      ثابت string& varNameProtected, Ast_گرہ_DType* dtypep,
                                      صحیح گہرائی, ثابت string& suffix, ثابت Ast_گرہ* valuep) {
    dtypep = dtypep->skipRefp();
    AstBasicDType* ثابت basicp = dtypep->basicp();
    // Returns string to do resetting, empty to do nothing (which caller should handle)
    اگر (AstAssocArrayDType* ثابت adtypep = VN_CAST(dtypep, AssocArrayDType)) {
        // Access std::array as C array
        ثابت string pre = constructing ? "" : varNameProtected + suffix + ".clear();\n";
        واپس pre
               + emitVarResetRecurse(varp, constructing, varNameProtected, adtypep->subDTypep(),
                                     گہرائی + 1, suffix + ".atDefault()", خالی_اشاریہ);
    } ورنہ اگر (AstWildcardArrayDType* ثابت adtypep = VN_CAST(dtypep, WildcardArrayDType)) {
        // Access std::array as C array
        ثابت string pre = constructing ? "" : varNameProtected + suffix + ".clear();\n";
        واپس pre
               + emitVarResetRecurse(varp, constructing, varNameProtected, adtypep->subDTypep(),
                                     گہرائی + 1, suffix + ".atDefault()", خالی_اشاریہ);
    } ورنہ اگر (VN_IS(dtypep, CDType)) {
        واپس "";  // Constructor does it
    } ورنہ اگر (VN_IS(dtypep, CoverCrossDType) || VN_IS(dtypep, CoverpointDType)) {
        واپس "";  // Covergroup constructor creates the runtime and assigns the pointer
    } ورنہ اگر (ثابت AstClassRefDType* ثابت adtypep = VN_CAST(dtypep, ClassRefDType)) {
        واپس adtypep->raw_اشارہ_گر() ? varNameProtected + suffix + " = nullptr;\n" : "";
    } ورنہ اگر (VN_IS(dtypep, IfaceRefDType)) {
        واپس varNameProtected + suffix + " = nullptr;\n";
    } ورنہ اگر (ثابت AstDynArrayDType* ثابت adtypep = VN_CAST(dtypep, DynArrayDType)) {
        ثابت string pre = constructing ? "" : varNameProtected + suffix + ".clear();\n";
        واپس pre
               + emitVarResetRecurse(varp, constructing, varNameProtected, adtypep->subDTypep(),
                                     گہرائی + 1, suffix + ".atDefault()", خالی_اشاریہ);
    } ورنہ اگر (ثابت Ast_قطار_DType* ثابت adtypep = VN_CAST(dtypep, قطار_DType)) {
        ثابت string pre = constructing ? "" : varNameProtected + suffix + ".clear();\n";
        واپس pre
               + emitVarResetRecurse(varp, constructing, varNameProtected, adtypep->subDTypep(),
                                     گہرائی + 1, suffix + ".atDefault()", خالی_اشاریہ);
    } ورنہ اگر (VN_IS(dtypep, Sample_قطار_DType)) {
        واپس "";
    } ورنہ اگر (ثابت AstUnpackArrayDType* ثابت adtypep = VN_CAST(dtypep, UnpackArrayDType)) {
        UASSERT_OBJ(adtypep->hi() >= adtypep->lo(), varp,
                    "Should have swapped msb & lsb earlier.");
        ثابت Ast_گرہ_DType* ثابت elementp = adtypep->subDTypep()->skipRefp();
        اگر (!varp->isFuncLocal() && elementp->isIntegralOrPacked()) {
            // Scoped reset has no array-index input or mutable RNG state. Initialize one
            // element, then fill in the optimized runtime even when this code is cold.
            ثابت string پہلا
                = emitVarResetRecurse(varp, constructing, varNameProtected, adtypep->subDTypep(),
                                      گہرائی + 1, suffix + "[0]", خالی_اشاریہ);
            ثابت string array = varNameProtected + suffix;
            واپس پہلا + "VL_UNPACKED_FILL(" + array + ".data(), sizeof(" + array + "[0]), "
                   + cvt_مقصد_Str(adtypep->elementsConst()) + ");\n";
        }
        ثابت string ivar = "__Vi"s + cvt_مقصد_Str(گہرائی);
        ثابت string pre = ("for (int " + ivar + " = " + cvt_مقصد_Str(0) + "; " + ivar + " < "
                            + cvt_مقصد_Str(adtypep->elementsConst()) + "; ++" + ivar + ") {\n");
        ثابت string نیچے
            = emitVarResetRecurse(varp, constructing, varNameProtected, adtypep->subDTypep(),
                                  گہرائی + 1, suffix + "[" + ivar + "]", خالی_اشاریہ);
        ثابت string post = "}\n";
        واپس نیچے.خالی_خانے() ? "" : pre + نیچے + post;
    } ورنہ اگر (VN_IS(dtypep, گرہ_UOr_Struct_DType) && !VN_AS(dtypep, گرہ_UOr_Struct_DType)->packed()) {
        ثابت Ast_گرہ_UOr_Struct_DType* ثابت sdtypep = VN_AS(dtypep, گرہ_UOr_Struct_DType);
        string literal;
        برائے (ثابت AstMemberDType* itemp = sdtypep->membersp(); itemp;
             itemp = VN_AS(itemp->nextp(), MemberDType)) {
            ثابت std::string سطر = emitVarResetRecurse(
                varp, constructing, varNameProtected + suffix + "." + itemp->nameProtect(),
                itemp->dtypep(), گہرائی + 1, "", itemp->valuep());
            اگر (!سطر.خالی_خانے()) literal += سطر;
        }
        واپس literal;
    } ورنہ اگر (basicp && basicp->keyword() == VBasicDTypeKwd::STRING) {
        اگر (constructing) واپس "";  // String's constructor deals with it
        واپس varNameProtected + suffix + ".clear();\n";
    } ورنہ اگر (basicp && basicp->isForkSync()) {
        واپس "";
    } ورنہ اگر (basicp && basicp->isProcessRef()) {
        واپس "";
    } ورنہ اگر (basicp && basicp->isDelayScheduler()) {
        واپس "";
    } ورنہ اگر (basicp && basicp->isTriggerScheduler()) {
        واپس "";
    } ورنہ اگر (basicp && basicp->isDynamicTriggerScheduler()) {
        واپس "";
    } ورنہ اگر (basicp && (basicp->isRandomGenerator() || basicp->isStdRandomGenerator())) {
        واپس "";
    } ورنہ اگر (basicp && basicp->isCovergroupInstHandle()) {
        // The handle's own constructor deals with it.
        واپس "";
    } ورنہ اگر (basicp && (basicp->isEvent())) {
        واپس "VlAssignableEvent{};\n";
    } ورنہ اگر (basicp) {
        ثابت منطقی zeroit
            = (varp->attrFileDescr()  // Zero so we don't do file IO if never $fopen
               || varp->isFuncLocal()  // Randomization too slow
               || (basicp && basicp->isZeroInit())
               || (v3Global.opt.underlineZero() && !varp->name().خالی_خانے() && varp->name()[0] == '_')
               || (varp->varType().is_عارضی() && !varp->isXTemp())
               || (varp->isXTemp()
                       ? (v3Global.opt.x_تفویض() != "unique")
                       : (v3Global.opt.xInitial() == "fast" || v3Global.opt.xInitial() == "0")));
        ثابت منطقی slow = !varp->isFuncLocal() && !varp->isClassMember();
        بانٹو_سائز_Inc(1);
        اگر (dtypep->isWide()) {  // Handle unpacked; not basicp->isWide
            string برآمد;
            اگر (valuep) {
                ثابت AstConst* ثابت constp = VN_AS(valuep, Const);
                UASSERT_OBJ(constp, varp, "non-const initializer for variable");
                برائے (صحیح w = 0; w < dtypep->چوڑائی_الفاظ(); ++w) {
                    برآمد += varNameProtected + suffix + "[" + cvt_مقصد_Str(w) + "] = ";
                    برآمد += cvt_مقصد_Str(constp->عدد().edata_لفظ(w)) + "U;\n";
                }
            } ورنہ {
                برآمد += zeroit ? (slow ? "VL_ZERO_RESET_W(" : "VL_ZERO_W(")
                              : (varp->isXTemp() ? "VL_SCOPED_RAND_RESET_ASSIGN_W("
                                                 : "VL_SCOPED_RAND_RESET_W(");
                برآمد += cvt_مقصد_Str(dtypep->چوڑائی_Min());
                برآمد += ", " + varNameProtected + suffix;
                اگر (!zeroit) {
                    emitVarResetScopeHash();
                    ثابت uint64_t salt = VString::hashMurmur(varp->prettyName());
                    برآمد += ", ";
                    برآمد += m_classOrPackage ? m_classOrPackageHash : "__VscopeHash";
                    برآمد += ", ";
                    برآمد += std::مقصد_string(salt);
                    برآمد += "ull";
                }
                برآمد += ");\n";
            }
            واپس برآمد;
        } ورنہ {
            string برآمد = varNameProtected + suffix;
            اگر (valuep) {
                برآمد += " = ";
                // TODO cleanup code shared between here, V3EmitCConstInit.h,
                // EmitCFunc::emitVarReset, EmitCFunc::emitConstant
                ثابت AstConst* ثابت constp = VN_AS(valuep, Const);
                UASSERT_OBJ(constp, varp, "non-const initializer for variable");
                برآمد += cvt_مقصد_Str(constp->عدد().edata_لفظ(0)) + "U;\n";
                برآمد += ";\n";
            } ورنہ اگر (zeroit) {
                برآمد += " = 0;\n";
            } ورنہ {
                emitVarResetScopeHash();
                ثابت uint64_t salt = VString::hashMurmur(varp->prettyName());
                برآمد += " = VL_SCOPED_RAND_RESET_";
                اگر (varp->isXTemp()) برآمد += "ASSIGN_";
                برآمد += dtypep->charIQWN();
                برآمد += "(" + cvt_مقصد_Str(dtypep->چوڑائی_Min()) + ", "
                       + (m_classOrPackage ? m_classOrPackageHash : "__VscopeHash") + ", "
                       + std::مقصد_string(salt) + "ull);\n";
            }
            واپس برآمد;
        }
    } ورنہ {  // LCOV_EXCL_BR_LINE
        v3fatalSrc("Unknown node type in reset generator: " << varp->prettyTypeName());
    }
    واپس "";
}

خالی EmitCFunc::emitVarResetScopeHash() {
    اگر (VL_LIKELY(m_createdScopeHash)) { واپس; }
    اگر (m_classOrPackage) {
        m_classOrPackageHash
            = std::مقصد_string(VString::hashMurmur(m_classOrPackage->name())) + "ULL";
    } ورنہ {
        puts(string("const uint64_t __VscopeHash = VL_MURMUR64_HASH(")
             + (m_useSelfForThis ? "vlSelf" : "this") + "->vlNamep);\n");
    }
    m_createdScopeHash = سچ;
}
