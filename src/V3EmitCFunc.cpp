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

#ضمن "V3PchAstMT.h"

#ضمن "V3EmitCFunc.h"

#ضمن <map>
#ضمن <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// EmitCFunc

منطقي EmitCFunc::emitSimpleOk(Ast_غوټه_بیان* nodep) {
    // Can we put out a simple (A + B) instead of VL_ADD_III(A,B)?
    إذا (nodep->emitSimpleOperator() == "") أرجع خطأ;
    إذا (nodep->isWide()) أرجع خطأ;
    إذا (nodep->op1p() && nodep->op1p()->isWide()) أرجع خطأ;
    إذا (nodep->op2p() && nodep->op2p()->isWide()) أرجع خطأ;
    إذا (nodep->op3p() && nodep->op3p()->isWide()) أرجع خطأ;
    أرجع ریښتیا;
}

فراغ EmitCFunc::emitOpName(Ast_غوټه* nodep, ثابت string& format, Ast_غوټه* lhsp, Ast_غوټه* rhsp,
                           Ast_غوټه* thsp) {
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
    string بهر;
    putnbs(nodep, "");

    منطقي needComma = خطأ;
    منطقي کاروي_کتار = خطأ;
    string بل_Comma;
    خپلکاری comma_بهر = [&بهر, &بل_Comma]() {
        إذا (!بل_Comma.تشه()) {
            بهر += بل_Comma;
            بل_Comma = "";
        }
    };

    خپلکاری put_بهر = [هذا, &بهر]() {
        إذا (!بهر.تشه()) puts(بهر);
        بهر = "";
    };

    لكل (string::const_iterator pos = format.begin(); pos != format.پای_ټکی(); ++pos) {
        إذا (pos[0] == ',') {
            // Remember we need to add one, but don't do yet to avoid ",)"
            إذا (needComma) {
                إذا (pos[1] == ' ') {
                    بل_Comma = ", ";
                } وإلا {
                    بل_Comma = ",";
                }
                needComma = خطأ;
            }
            إذا (pos[1] == ' ') ++pos;  // Must do even if no nextComma
        } وإلا إذا (pos[0] == '%') {
            ++pos;
            منطقي detail = خطأ;
            Ast_غوټه* detailp = تش_اشاره;
            بدل (pos[0]) {
            حالة '%': بهر += '%'; اكسر;
            حالة 'k':
                put_بهر();
                putbs("");
                اكسر;
            حالة 'n':
                detail = ریښتیا;
                detailp = nodep;
                اكسر;
            حالة 'l':
                detail = ریښتیا;
                detailp = lhsp;
                اكسر;
            حالة 'r':
                detail = ریښتیا;
                detailp = rhsp;
                اكسر;
            حالة 't':
                detail = ریښتیا;
                detailp = thsp;
                اكسر;
            حالة 'P':
                إذا (nodep->isWide() && !کاروي_کتار) {
                    UASSERT_OBJ(m_wide_لنډمهاله_Refp, nodep,
                                "Wide op " << nodep->prettyTypeName()
                                           << " w/ no temp, perhaps missing op in V3EmitC?");
                    comma_بهر();
                    put_بهر();
                    إذا (!m_wide_لنډمهاله_Refp->self_اشاره_ګر().is_تشه()) {
                        emitDereference(m_wide_لنډمهاله_Refp,
                                        m_wide_لنډمهاله_Refp->self_اشاره_ګر_Protect(m_useSelfForThis));
                    }
                    بهر += varName(m_wide_لنډمهاله_Refp->varp());
                    m_wide_لنډمهاله_Refp = تش_اشاره;
                    needComma = ریښتیا;
                } وإلا إذا (کاروي_کتار) {
                    comma_بهر();
                    put_بهر();
                    iterate_And_بل_Const_Null(nodep->backp()->op2p());
                    needComma = ریښتیا;
                }

                اكسر;
            افتراضي: nodep->v3fatalSrc("Unknown emitOperator format code: %" << pos[0]); اكسر;
            }
            إذا (detail) {
                // Get next letter of %[nlrt]
                ++pos;
                بدل (pos[0]) {
                حالة 'q':
                    put_بهر();
                    // If we are assigning this to a queue we need to get the return type
                    إذا (VN_IS(detailp->backp(), ټاکل)
                        && VN_IS(detailp->backp()->op2p()->dtypep()->skipRefp(), کتار_DType)) {
                        puts("R");  // R for queue
                        کاروي_کتار = ریښتیا;
                    } وإلا إذا (VN_IS(detailp->dtypep()->skipRefp(), کتار_DType)
                               || VN_IS(detailp->dtypep()->skipRefp(), بهیر_DType)) {
                        puts("R");  // R for queue
                    } وإلا {
                        emitIQW(detailp);
                    }
                    اكسر;
                حالة 'w':
                    comma_بهر();
                    بهر += cvt_منزل_Str(detailp->پلنوالی_Min());
                    needComma = ریښتیا;
                    اكسر;
                حالة 'W':
                    إذا (lhsp->isWide()) {
                        comma_بهر();
                        بهر += cvt_منزل_Str(lhsp->پلنوالی_ټکي());
                        needComma = ریښتیا;
                    } وإلا إذا (VN_IS(lhsp, بهیر_R)) {
                        comma_بهر();
                        بهر += cvt_منزل_Str(rhsp->پلنوالی_ټکي());
                        needComma = ریښتیا;
                    }
                    اكسر;
                حالة 'i':
                    comma_بهر();
                    UASSERT_OBJ(detailp, nodep, "emitOperator() references undef node");
                    put_بهر();
                    iterate_And_بل_Const_Null(detailp);
                    needComma = ریښتیا;
                    اكسر;
                افتراضي:
                    nodep->v3fatalSrc("Unknown emitOperator format code: %[nlrt]" << pos[0]);
                    اكسر;
                }
            }
        } وإلا إذا (pos[0] == ')') {
            بل_Comma = "";
            بهر += ')';
        } وإلا إذا (pos[0] == '(') {
            comma_بهر();
            needComma = خطأ;
            بهر += '(';
        } وإلا {
            // Normal text
            إذا (std::که_توری_یا_عدد(pos[0])) needComma = ریښتیا;
            comma_بهر();
            بهر += pos[0];
        }
    }
    put_بهر();
}

منطقي EmitCFunc::display_Emit_سرلیک(Ast_غوټه* nodep) {
    منطقي isStmt = خطأ;
    إذا (ثابت AstFScanF* ثابت dispp = VN_CAST(nodep, FScanF)) {
        isStmt = خطأ;
        putns(nodep, "VL_FSCANF_INX(");
        iterateConst(dispp->filep());
        puts(",");
    } وإلا إذا (ثابت AstSScanF* ثابت dispp = VN_CAST(nodep, SScanF)) {
        isStmt = خطأ;
        putns(nodep, "VL_SSCANF_I");
        emitIQW(dispp->fromp());
        puts("NX(");
        puts(cvt_منزل_Str(dispp->fromp()->پلنوالی_Min()));
        puts(",");
        iterateConst(dispp->fromp());
        puts(",");
    } وإلا إذا (ثابت AstDisplay* ثابت dispp = VN_CAST(nodep, Display)) {
        isStmt = ریښتیا;
        إذا (dispp->filep()) {
            putns(nodep, "VL_FWRITEF_NX(");
            iterateConst(dispp->filep());
            puts(",");
        } وإلا {
            putns(nodep, "VL_WRITEF_NX(");
        }
    } وإلا إذا (ثابت AstSFormat* ثابت dispp = VN_CAST(nodep, SFormat)) {
        isStmt = ریښتیا;
        puts("VL_SFORMAT_NX(");
        إذا (!dispp->lhsp()->dtypep()->isString()) {
            puts(cvt_منزل_Str(dispp->lhsp()->پلنوالی_Min()));
            putbs(",");
        }
        iterateConst(dispp->lhsp());
        emitDatap(dispp->lhsp());
        putbs(",");
    } وإلا إذا (VN_IS(nodep, SFormatF)) {
        isStmt = خطأ;
        putns(nodep, "VL_SFORMATF_N_NX(");
    } وإلا {
        nodep->v3fatalSrc("Unknown displayEmit node type");
    }
    أرجع isStmt;
}

فراغ EmitCFunc::display_غوټه(Ast_غوټه* nodep, AstSFormatF* fmtp,  // fmtp is nullptr for AstScan
                            ثابت string& vformat, Ast_غوټه* exprsp, منطقي is_لټون) {
    // Check format, if it exists
    ثابت منطقي بیان_Format_2 = fmtp && fmtp->بیان_Format_2();
    منطقي needsScope = بیان_Format_2;
    منطقي needsTimescale = بیان_Format_2;
    صحيح format_دلیلونه = 0;
    إذا (!بیان_Format_2) {
        // Check display and argument count to head-off later runtime issues
        منطقي دننه_Pct = خطأ;
        منطقي ignore = خطأ;
        لكل (حرف ch : vformat) {
            // UINFO(1, "Parse '" << *pos << "'  IP" << inPct << " List " << cvtToHex(elistp));
            إذا (!دننه_Pct && ch == '%') {
                دننه_Pct = ریښتیا;
                ignore = خطأ;
            } وإلا إذا (دننه_Pct && (std::که_عدد(ch) || ch == '.' || ch == '-')) {
            } وإلا إذا (!دننه_Pct) {  // Normal text
            } وإلا {  // Format character
                دننه_Pct = خطأ;
                ch = std::tolower(ch);
                بدل (ch) {
                حالة '%': اكسر;
                حالة '*':
                    دننه_Pct = ریښتیا;  // Get more digits
                    ignore = ریښتیا;
                    اكسر;
                حالة 'm': needsScope = ریښتیا; اكسر;
                حالة 't':
                    needsTimescale = ریښتیا;
                    ++format_دلیلونه;
                    اكسر;
                افتراضي:
                    إذا (!ignore && V3_شمېره::displayedFmtHasArg(ch, is_لټون)) ++format_دلیلونه;
                    اكسر;
                }
            }
        }
    }

    إذا (vformat.تشه() && VN_IS(nodep, Display))  // not fscanf etc, as they need to return value
        أرجع;  // NOP

    ثابت منطقي isStmt = display_Emit_سرلیک(nodep);

    إذا (بیان_Format_2) {
        UASSERT_OBJ(exprsp, nodep, "Missing format expression");
        iterateConst(exprsp);
        emitDatap(exprsp);
        exprsp = exprsp->nextp();
    } وإلا {
        ofp()->putsQuoted(vformat);
    }

    صحيح بیان_دلیلونه = 0;
    لكل (Ast_غوټه* argp = exprsp; argp; argp = argp->nextp()) ++بیان_دلیلونه;
    إذا (VL_UNCOVERABLE(!بیان_Format_2 && بیان_دلیلونه != format_دلیلونه)) {  // LCOV_EXCL_START
        // expectDisplay() checks this first, so probably internal error if found here
        nodep->v3error("Internal: Missing or extra arguments for $display-like format, "
                       << " expression had " << بیان_دلیلونه << ", format had " << format_دلیلونه);
    }  // LCOV_EXCL_STOP

    // argc for error check. Also MSVC++ requires va_args to not be off a reference
    صحيح argc = 0;
    إذا (needsScope) ++argc;
    إذا (needsTimescale) ++argc;
    لكل (Ast_غوټه* argp = exprsp; argp; argp = argp->nextp()) {
        ++argc;
        ثابت AstSFormatArg* ثابت fargp = VN_CAST(argp, SFormatArg);
        إذا (fargp && fargp->formatAttr().isEnum()) ++argc;  // Additional name argument
    }
    ofp()->puts("," + std::منزل_string(argc));

    إذا (needsScope) {
        AstScopeName* ثابت scopenamep = fmtp ? fmtp->scopeNamep() : تش_اشاره;
        UASSERT_OBJ(scopenamep, nodep, "Display with %m but no AstScopeName");
        ofp()->puts(", '"s + VFormatAttr{VFormatAttr::SCOPE}.ascii() + "'");
        إذا (scopenamep->selfRelative()) {
            // vlNamep is catName(name(), instance), so starts with '.' when the model is unnamed
            ثابت string namep = string{m_useSelfForThis ? "vlSelf" : "this"} + "->vlNamep";
            ofp()->puts(",(vlSymsp->name()[0] ? " + namep + " : " + namep + " + 1),");
            ofp()->putsQuoted(scopenamep->scopeAttr());
        } وإلا {
            ofp()->puts(",vlSymsp->name(),");
            ofp()->putsQuoted(scopenamep->scopePrettySymName());
        }
    }

    إذا (needsTimescale) {
        VTimescale timeunit = VTimescale::NONE;
        إذا (ثابت AstDisplay* ثابت anodep = VN_CAST(nodep, Display)) {
            timeunit = anodep->fmtp()->timeunit();
        } وإلا إذا (ثابت AstSFormat* ثابت anodep = VN_CAST(nodep, SFormat)) {
            timeunit = anodep->fmtp()->timeunit();
        } وإلا إذا (ثابت AstSScanF* ثابت anodep = VN_CAST(nodep, SScanF)) {
            timeunit = anodep->timeunit();
        } وإلا إذا (ثابت AstSFormatF* ثابت anodep = VN_CAST(nodep, SFormatF)) {
            timeunit = anodep->timeunit();
        }
        UASSERT_OBJ(!timeunit.isNone(), nodep,
                    "Use of %t must be under AstDisplay, AstSFormat, or AstSFormatF, or "
                    "SScanF, and timeunit set");
        ofp()->puts(", '"s + VFormatAttr{VFormatAttr::TIMEUNIT}.ascii() + "'");
        ofp()->puts(","s + cvt_منزل_Str((صحيح)timeunit.power_د_Ten()));
    }

    // Arguments
    لكل (Ast_غوټه* argp = exprsp; argp; argp = argp->nextp()) {
        ofp()->indentInc();
        ofp()->putbs("");
        AstSFormatArg* ثابت fargp = VN_CAST(argp, SFormatArg);
        Ast_غوټه* ثابت subargp = fargp ? fargp->exprp() : argp;
        ثابت VFormatAttr formatAttr = AstSFormatArg::formatAttrDefauled(fargp, subargp->dtypep());
        puts(", '"s + formatAttr.ascii() + '\'');
        إذا (formatAttr.isSigned() || formatAttr.isUnsigned() || formatAttr.isEnum())
            puts("," + cvt_منزل_Str(subargp->پلنوالی_Min()));
        ثابت منطقي addrof = is_لټون || formatAttr.isString() || formatAttr.isComplex();
        ثابت منطقي wideEnum = formatAttr.isEnum() && subargp->isWide();
        puts(",");
        إذا (wideEnum) puts("static_cast<const EData*>(");
        إذا (addrof) puts("&(");
        إذا (VN_IS(subargp, بهیر_R))
            emit_بهیر_R(
                VN_CAST(subargp, بهیر_R),
                nodep);  // This has to be done here because streamR doesn't know what it returns
        وإلا { iterateConst(subargp); }
        إذا (addrof) puts(")");
        إذا (!addrof) emitDatap(argp);
        إذا (wideEnum) puts(")");
        إذا (formatAttr.isEnum()) {
            puts(", '"s + VFormatAttr{VFormatAttr::STRING}.ascii() + "', &(");
            iterateConst(fargp->namep());
            puts(")");
        }
        ofp()->indentDec();
    }

    // End
    إذا (isStmt) {
        puts(");\n");
    } وإلا {
        puts(") ");
    }
}

فراغ EmitCFunc::emit_CCall_دلیلونه(ثابت Ast_غوټه_CCall* nodep, ثابت string& self_اشاره_ګر,
                              منطقي دننه_Process) {
    putns(nodep, "(");
    منطقي comma = خطأ;
    إذا (nodep->funcp()->isLoose() && !nodep->funcp()->isStatic()) {
        UASSERT_OBJ(!self_اشاره_ګر.تشه(), nodep, "Call to loose method without self pointer");
        puts(self_اشاره_ګر);
        comma = ریښتیا;
    }
    إذا (nodep->funcp()->needProcess()) {
        إذا (comma) puts(", ");
        إذا (VN_IS(nodep->backp(), CAwait) || !nodep->funcp()->isCoroutine()) {
            puts("vlProcess");
        } وإلا إذا (دننه_Process) {
            puts("std::make_shared<VlProcess>(vlProcess)");
        } وإلا {
            puts("std::make_shared<VlProcess>()");
        }
        comma = ریښتیا;
    }
    إذا (!nodep->argTypes().تشه()) {
        إذا (comma) puts(", ");
        puts(nodep->argTypes());
        comma = ریښتیا;
    }
    put_Comma_Iterate_بل(nodep->argsp(), comma);
    puts(")");
}

std::string EmitCFunc::dereferenceString(ثابت std::string& اشاره_ګر) ثابت {
    إذا (اشاره_ګر[0] == '(' && اشاره_ګر[1] == '&') {
        // remove "address of" followed by immediate dereference
        // Note: this relies on only the form '(&OBJECT)' being used by Verilator
        أرجع اشاره_ګر.substr(2, اشاره_ګر.اوږدتیا() - 3) + '.';
    } وإلا {
        إذا (اشاره_ګر == "vlSelf" && m_usevlSelfRef) {
            أرجع "vlSelfRef.";
        } وإلا {
            أرجع اشاره_ګر + "->";
        }
    }
}
فراغ EmitCFunc::emitDereference(Ast_غوټه* nodep, ثابت string& اشاره_ګر) {
    putns(nodep, dereferenceString(اشاره_ګر));
}

فراغ EmitCFunc::emitCvtPackStr(Ast_غوټه* nodep) {
    إذا (ثابت AstConst* ثابت constp = VN_CAST(nodep, Const)) {
        emitConstantString(constp);
    } وإلا إذا (VN_IS(nodep->dtypep(), بهیر_DType)) {
        putnbs(nodep, "VL_CVT_PACK_STR_ND(");
        iterate_And_بل_Const_Null(nodep);
        puts(")");
    } وإلا {
        putnbs(nodep, "VL_CVT_PACK_STR_N");
        emitIQW(nodep);
        puts("(");
        إذا (nodep->isWide()) {
            // Note argument width, not node width (which is always 32)
            puts(cvt_منزل_Str(nodep->پلنوالی_ټکي()));
            puts(", ");
        }
        iterate_And_بل_Const_Null(nodep);
        puts(")");
    }
}

فراغ EmitCFunc::emitConstant(AstConst* nodep) {
    // Put out constant set to the specified variable, or given variable in a string
    ثابت V3_شمېره& عدد = nodep->عدد();
    إذا (عدد.isFourState()) {
        nodep->v3warn(E_UNSUPPORTED, "Unsupported: 4-state numbers in this context");
        أرجع;
    }
    putns(nodep, عدد.emitC());
}

فراغ EmitCFunc::emitConstantString(ثابت AstConst* nodep) {
    // Const might be a Verilog array-type string, but need to always output std::string
    putnbs(nodep, "std::string{");
    ثابت string str = nodep->عدد().منزل_String();
    إذا (!str.تشه()) putsQuoted(str);
    puts("}");
}

فراغ EmitCFunc::emitVarReset(ثابت string& مختاړی, AstVar* varp, منطقي constructing) {
    // 'constructing' indicates that the object was just constructed, so if it is a string or
    // something that starts off clear already, no need to clear it again
    Ast_غوټه_DType* ثابت dtypep = varp->dtypep()->skipRefp();
    ثابت string vlSelf = VSelf_اشاره_ګر_متن::replaceThis(m_useSelfForThis, "this->");
    ثابت string varNameProtected
        = ((VN_IS(m_modp, Class) || varp->isFuncLocal()) || !مختاړی.تشه())
              ? varName(varp)
              : vlSelf + varName(varp);
    ثابت string new_مختاړی = مختاړی + varNameProtected;
    إذا (varp->isIO() && m_modp->is_سر() && optSystemC()) {
        // System C top I/O doesn't need loading, as the lower level subinst code does it.}
    } وإلا إذا (varp->isParam()) {
        UASSERT_OBJ(varp->valuep(), varp, "No init for a param?");
        // If a simple CONST value we initialize it using an enum
        // If an ARRAYINIT we initialize it using an initial block similar to a signal
        // puts("// parameter "+varp->nameProtect()+" = "+varp->valuep()->name()+"\n");
    } وإلا {
        putns(varp,
              emitVarResetRecurse(varp, constructing, new_مختاړی, dtypep, 0, "", varp->valuep()));
    }
}

string EmitCFunc::emitVarResetRecurse(ثابت AstVar* varp, منطقي constructing,
                                      ثابت string& varNameProtected, Ast_غوټه_DType* dtypep,
                                      صحيح ژوروالی, ثابت string& suffix, ثابت Ast_غوټه* valuep) {
    dtypep = dtypep->skipRefp();
    AstBasicDType* ثابت basicp = dtypep->basicp();
    // Returns string to do resetting, empty to do nothing (which caller should handle)
    إذا (AstAssocArrayDType* ثابت adtypep = VN_CAST(dtypep, AssocArrayDType)) {
        // Access std::array as C array
        ثابت string pre = constructing ? "" : varNameProtected + suffix + ".clear();\n";
        أرجع pre
               + emitVarResetRecurse(varp, constructing, varNameProtected, adtypep->subDTypep(),
                                     ژوروالی + 1, suffix + ".atDefault()", تش_اشاره);
    } وإلا إذا (AstWildcardArrayDType* ثابت adtypep = VN_CAST(dtypep, WildcardArrayDType)) {
        // Access std::array as C array
        ثابت string pre = constructing ? "" : varNameProtected + suffix + ".clear();\n";
        أرجع pre
               + emitVarResetRecurse(varp, constructing, varNameProtected, adtypep->subDTypep(),
                                     ژوروالی + 1, suffix + ".atDefault()", تش_اشاره);
    } وإلا إذا (VN_IS(dtypep, CDType)) {
        أرجع "";  // Constructor does it
    } وإلا إذا (VN_IS(dtypep, CoverCrossDType) || VN_IS(dtypep, CoverpointDType)) {
        أرجع "";  // Covergroup constructor creates the runtime and assigns the pointer
    } وإلا إذا (ثابت AstClassRefDType* ثابت adtypep = VN_CAST(dtypep, ClassRefDType)) {
        أرجع adtypep->raw_اشاره_ګر() ? varNameProtected + suffix + " = nullptr;\n" : "";
    } وإلا إذا (VN_IS(dtypep, IfaceRefDType)) {
        أرجع varNameProtected + suffix + " = nullptr;\n";
    } وإلا إذا (ثابت AstDynArrayDType* ثابت adtypep = VN_CAST(dtypep, DynArrayDType)) {
        ثابت string pre = constructing ? "" : varNameProtected + suffix + ".clear();\n";
        أرجع pre
               + emitVarResetRecurse(varp, constructing, varNameProtected, adtypep->subDTypep(),
                                     ژوروالی + 1, suffix + ".atDefault()", تش_اشاره);
    } وإلا إذا (ثابت Ast_کتار_DType* ثابت adtypep = VN_CAST(dtypep, کتار_DType)) {
        ثابت string pre = constructing ? "" : varNameProtected + suffix + ".clear();\n";
        أرجع pre
               + emitVarResetRecurse(varp, constructing, varNameProtected, adtypep->subDTypep(),
                                     ژوروالی + 1, suffix + ".atDefault()", تش_اشاره);
    } وإلا إذا (VN_IS(dtypep, Sample_کتار_DType)) {
        أرجع "";
    } وإلا إذا (ثابت AstUnpackArrayDType* ثابت adtypep = VN_CAST(dtypep, UnpackArrayDType)) {
        UASSERT_OBJ(adtypep->hi() >= adtypep->lo(), varp,
                    "Should have swapped msb & lsb earlier.");
        ثابت Ast_غوټه_DType* ثابت elementp = adtypep->subDTypep()->skipRefp();
        إذا (!varp->isFuncLocal() && elementp->isIntegralOrPacked()) {
            // Scoped reset has no array-index input or mutable RNG state. Initialize one
            // element, then fill in the optimized runtime even when this code is cold.
            ثابت string لومړی
                = emitVarResetRecurse(varp, constructing, varNameProtected, adtypep->subDTypep(),
                                      ژوروالی + 1, suffix + "[0]", تش_اشاره);
            ثابت string array = varNameProtected + suffix;
            أرجع لومړی + "VL_UNPACKED_FILL(" + array + ".data(), sizeof(" + array + "[0]), "
                   + cvt_منزل_Str(adtypep->elementsConst()) + ");\n";
        }
        ثابت string ivar = "__Vi"s + cvt_منزل_Str(ژوروالی);
        ثابت string pre = ("for (int " + ivar + " = " + cvt_منزل_Str(0) + "; " + ivar + " < "
                            + cvt_منزل_Str(adtypep->elementsConst()) + "; ++" + ivar + ") {\n");
        ثابت string لاندې
            = emitVarResetRecurse(varp, constructing, varNameProtected, adtypep->subDTypep(),
                                  ژوروالی + 1, suffix + "[" + ivar + "]", تش_اشاره);
        ثابت string post = "}\n";
        أرجع لاندې.تشه() ? "" : pre + لاندې + post;
    } وإلا إذا (VN_IS(dtypep, غوټه_UOr_Struct_DType) && !VN_AS(dtypep, غوټه_UOr_Struct_DType)->packed()) {
        ثابت Ast_غوټه_UOr_Struct_DType* ثابت sdtypep = VN_AS(dtypep, غوټه_UOr_Struct_DType);
        string literal;
        لكل (ثابت AstMemberDType* itemp = sdtypep->membersp(); itemp;
             itemp = VN_AS(itemp->nextp(), MemberDType)) {
            ثابت std::string کرښه = emitVarResetRecurse(
                varp, constructing, varNameProtected + suffix + "." + itemp->nameProtect(),
                itemp->dtypep(), ژوروالی + 1, "", itemp->valuep());
            إذا (!کرښه.تشه()) literal += کرښه;
        }
        أرجع literal;
    } وإلا إذا (basicp && basicp->keyword() == VBasicDTypeKwd::STRING) {
        إذا (constructing) أرجع "";  // String's constructor deals with it
        أرجع varNameProtected + suffix + ".clear();\n";
    } وإلا إذا (basicp && basicp->isForkSync()) {
        أرجع "";
    } وإلا إذا (basicp && basicp->isProcessRef()) {
        أرجع "";
    } وإلا إذا (basicp && basicp->isDelayScheduler()) {
        أرجع "";
    } وإلا إذا (basicp && basicp->isTriggerScheduler()) {
        أرجع "";
    } وإلا إذا (basicp && basicp->isDynamicTriggerScheduler()) {
        أرجع "";
    } وإلا إذا (basicp && (basicp->isRandomGenerator() || basicp->isStdRandomGenerator())) {
        أرجع "";
    } وإلا إذا (basicp && basicp->isCovergroupInstHandle()) {
        // The handle's own constructor deals with it.
        أرجع "";
    } وإلا إذا (basicp && (basicp->isEvent())) {
        أرجع "VlAssignableEvent{};\n";
    } وإلا إذا (basicp) {
        ثابت منطقي zeroit
            = (varp->attrFileDescr()  // Zero so we don't do file IO if never $fopen
               || varp->isFuncLocal()  // Randomization too slow
               || (basicp && basicp->isZeroInit())
               || (v3Global.opt.underlineZero() && !varp->name().تشه() && varp->name()[0] == '_')
               || (varp->varType().is_لنډمهاله() && !varp->isXTemp())
               || (varp->isXTemp()
                       ? (v3Global.opt.x_ټاکل() != "unique")
                       : (v3Global.opt.xInitial() == "fast" || v3Global.opt.xInitial() == "0")));
        ثابت منطقي slow = !varp->isFuncLocal() && !varp->isClassMember();
        وېشل_اندازه_Inc(1);
        إذا (dtypep->isWide()) {  // Handle unpacked; not basicp->isWide
            string بهر;
            إذا (valuep) {
                ثابت AstConst* ثابت constp = VN_AS(valuep, Const);
                UASSERT_OBJ(constp, varp, "non-const initializer for variable");
                لكل (صحيح w = 0; w < dtypep->پلنوالی_ټکي(); ++w) {
                    بهر += varNameProtected + suffix + "[" + cvt_منزل_Str(w) + "] = ";
                    بهر += cvt_منزل_Str(constp->عدد().edata_کلمه(w)) + "U;\n";
                }
            } وإلا {
                بهر += zeroit ? (slow ? "VL_ZERO_RESET_W(" : "VL_ZERO_W(")
                              : (varp->isXTemp() ? "VL_SCOPED_RAND_RESET_ASSIGN_W("
                                                 : "VL_SCOPED_RAND_RESET_W(");
                بهر += cvt_منزل_Str(dtypep->پلنوالی_Min());
                بهر += ", " + varNameProtected + suffix;
                إذا (!zeroit) {
                    emitVarResetScopeHash();
                    ثابت uint64_t salt = VString::hashMurmur(varp->prettyName());
                    بهر += ", ";
                    بهر += m_classOrPackage ? m_classOrPackageHash : "__VscopeHash";
                    بهر += ", ";
                    بهر += std::منزل_string(salt);
                    بهر += "ull";
                }
                بهر += ");\n";
            }
            أرجع بهر;
        } وإلا {
            string بهر = varNameProtected + suffix;
            إذا (valuep) {
                بهر += " = ";
                // TODO cleanup code shared between here, V3EmitCConstInit.h,
                // EmitCFunc::emitVarReset, EmitCFunc::emitConstant
                ثابت AstConst* ثابت constp = VN_AS(valuep, Const);
                UASSERT_OBJ(constp, varp, "non-const initializer for variable");
                بهر += cvt_منزل_Str(constp->عدد().edata_کلمه(0)) + "U;\n";
                بهر += ";\n";
            } وإلا إذا (zeroit) {
                بهر += " = 0;\n";
            } وإلا {
                emitVarResetScopeHash();
                ثابت uint64_t salt = VString::hashMurmur(varp->prettyName());
                بهر += " = VL_SCOPED_RAND_RESET_";
                إذا (varp->isXTemp()) بهر += "ASSIGN_";
                بهر += dtypep->charIQWN();
                بهر += "(" + cvt_منزل_Str(dtypep->پلنوالی_Min()) + ", "
                       + (m_classOrPackage ? m_classOrPackageHash : "__VscopeHash") + ", "
                       + std::منزل_string(salt) + "ull);\n";
            }
            أرجع بهر;
        }
    } وإلا {  // LCOV_EXCL_BR_LINE
        v3fatalSrc("Unknown node type in reset generator: " << varp->prettyTypeName());
    }
    أرجع "";
}

فراغ EmitCFunc::emitVarResetScopeHash() {
    إذا (VL_LIKELY(m_createdScopeHash)) { أرجع; }
    إذا (m_classOrPackage) {
        m_classOrPackageHash
            = std::منزل_string(VString::hashMurmur(m_classOrPackage->name())) + "ULL";
    } وإلا {
        puts(string("const uint64_t __VscopeHash = VL_MURMUR64_HASH(")
             + (m_useSelfForThis ? "vlSelf" : "this") + "->vlNamep);\n");
    }
    m_createdScopeHash = ریښتیا;
}
