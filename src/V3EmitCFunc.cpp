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

منطقي EmitCFunc::emitSimpleOk(Ast_گرہ_اظہاریہ* nodep) {
    // Can we put out a simple (A + B) instead of VL_ADD_III(A,B)?
    إذا (nodep->emitSimpleOperator() == "") أرجع خطأ;
    إذا (nodep->isWide()) أرجع خطأ;
    إذا (nodep->op1p() && nodep->op1p()->isWide()) أرجع خطأ;
    إذا (nodep->op2p() && nodep->op2p()->isWide()) أرجع خطأ;
    إذا (nodep->op3p() && nodep->op3p()->isWide()) أرجع خطأ;
    أرجع سچ;
}

فراغ EmitCFunc::emitOpName(Ast_گرہ* nodep, ثابت string& format, Ast_گرہ* lhsp, Ast_گرہ* rhsp,
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
    string باہرلا;
    putnbs(nodep, "");

    منطقي needComma = خطأ;
    منطقي ورتدا_قطار = خطأ;
    string اگلا_Comma;
    خودکار comma_باہرلا = [&باہرلا, &اگلا_Comma]() {
        إذا (!اگلا_Comma.خالی_تھاواں()) {
            باہرلا += اگلا_Comma;
            اگلا_Comma = "";
        }
    };

    خودکار put_باہرلا = [هذا, &باہرلا]() {
        إذا (!باہرلا.خالی_تھاواں()) puts(باہرلا);
        باہرلا = "";
    };

    لكل (string::const_iterator pos = format.begin(); pos != format.سرا(); ++pos) {
        إذا (pos[0] == ',') {
            // Remember we need to add one, but don't do yet to avoid ",)"
            إذا (needComma) {
                إذا (pos[1] == ' ') {
                    اگلا_Comma = ", ";
                } وإلا {
                    اگلا_Comma = ",";
                }
                needComma = خطأ;
            }
            إذا (pos[1] == ' ') ++pos;  // Must do even if no nextComma
        } وإلا إذا (pos[0] == '%') {
            ++pos;
            منطقي detail = خطأ;
            Ast_گرہ* detailp = خالی_سوچک;
            بدل (pos[0]) {
            حالة '%': باہرلا += '%'; اكسر;
            حالة 'k':
                put_باہرلا();
                putbs("");
                اكسر;
            حالة 'n':
                detail = سچ;
                detailp = nodep;
                اكسر;
            حالة 'l':
                detail = سچ;
                detailp = lhsp;
                اكسر;
            حالة 'r':
                detail = سچ;
                detailp = rhsp;
                اكسر;
            حالة 't':
                detail = سچ;
                detailp = thsp;
                اكسر;
            حالة 'P':
                إذا (nodep->isWide() && !ورتدا_قطار) {
                    UASSERT_OBJ(m_wide_عارضی_Refp, nodep,
                                "Wide op " << nodep->prettyTypeName()
                                           << " w/ no temp, perhaps missing op in V3EmitC?");
                    comma_باہرلا();
                    put_باہرلا();
                    إذا (!m_wide_عارضی_Refp->self_اشارہ_گر().is_خالی_تھاواں()) {
                        emitDereference(m_wide_عارضی_Refp,
                                        m_wide_عارضی_Refp->self_اشارہ_گر_Protect(m_useSelfForThis));
                    }
                    باہرلا += varName(m_wide_عارضی_Refp->varp());
                    m_wide_عارضی_Refp = خالی_سوچک;
                    needComma = سچ;
                } وإلا إذا (ورتدا_قطار) {
                    comma_باہرلا();
                    put_باہرلا();
                    iterate_And_اگلا_Const_Null(nodep->backp()->op2p());
                    needComma = سچ;
                }

                اكسر;
            افتراضي: nodep->v3fatalSrc("Unknown emitOperator format code: %" << pos[0]); اكسر;
            }
            إذا (detail) {
                // Get next letter of %[nlrt]
                ++pos;
                بدل (pos[0]) {
                حالة 'q':
                    put_باہرلا();
                    // If we are assigning this to a queue we need to get the return type
                    إذا (VN_IS(detailp->backp(), سونپو)
                        && VN_IS(detailp->backp()->op2p()->dtypep()->skipRefp(), قطار_DType)) {
                        puts("R");  // R for queue
                        ورتدا_قطار = سچ;
                    } وإلا إذا (VN_IS(detailp->dtypep()->skipRefp(), قطار_DType)
                               || VN_IS(detailp->dtypep()->skipRefp(), دھارا_DType)) {
                        puts("R");  // R for queue
                    } وإلا {
                        emitIQW(detailp);
                    }
                    اكسر;
                حالة 'w':
                    comma_باہرلا();
                    باہرلا += cvt_منزل_Str(detailp->چوڑائی_Min());
                    needComma = سچ;
                    اكسر;
                حالة 'W':
                    إذا (lhsp->isWide()) {
                        comma_باہرلا();
                        باہرلا += cvt_منزل_Str(lhsp->چوڑائی_الفاظ());
                        needComma = سچ;
                    } وإلا إذا (VN_IS(lhsp, دھارا_R)) {
                        comma_باہرلا();
                        باہرلا += cvt_منزل_Str(rhsp->چوڑائی_الفاظ());
                        needComma = سچ;
                    }
                    اكسر;
                حالة 'i':
                    comma_باہرلا();
                    UASSERT_OBJ(detailp, nodep, "emitOperator() references undef node");
                    put_باہرلا();
                    iterate_And_اگلا_Const_Null(detailp);
                    needComma = سچ;
                    اكسر;
                افتراضي:
                    nodep->v3fatalSrc("Unknown emitOperator format code: %[nlrt]" << pos[0]);
                    اكسر;
                }
            }
        } وإلا إذا (pos[0] == ')') {
            اگلا_Comma = "";
            باہرلا += ')';
        } وإلا إذا (pos[0] == '(') {
            comma_باہرلا();
            needComma = خطأ;
            باہرلا += '(';
        } وإلا {
            // Normal text
            إذا (std::حرف_یا_ہندسہ(pos[0])) needComma = سچ;
            comma_باہرلا();
            باہرلا += pos[0];
        }
    }
    put_باہرلا();
}

منطقي EmitCFunc::display_Emit_سرخی(Ast_گرہ* nodep) {
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
        puts(cvt_منزل_Str(dispp->fromp()->چوڑائی_Min()));
        puts(",");
        iterateConst(dispp->fromp());
        puts(",");
    } وإلا إذا (ثابت AstDisplay* ثابت dispp = VN_CAST(nodep, Display)) {
        isStmt = سچ;
        إذا (dispp->filep()) {
            putns(nodep, "VL_FWRITEF_NX(");
            iterateConst(dispp->filep());
            puts(",");
        } وإلا {
            putns(nodep, "VL_WRITEF_NX(");
        }
    } وإلا إذا (ثابت AstSFormat* ثابت dispp = VN_CAST(nodep, SFormat)) {
        isStmt = سچ;
        puts("VL_SFORMAT_NX(");
        إذا (!dispp->lhsp()->dtypep()->isString()) {
            puts(cvt_منزل_Str(dispp->lhsp()->چوڑائی_Min()));
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

فراغ EmitCFunc::display_گرہ(Ast_گرہ* nodep, AstSFormatF* fmtp,  // fmtp is nullptr for AstScan
                            ثابت string& vformat, Ast_گرہ* exprsp, منطقي is_جانچ) {
    // Check format, if it exists
    ثابت منطقي اظہاریہ_Format_2 = fmtp && fmtp->اظہاریہ_Format_2();
    منطقي needsScope = اظہاریہ_Format_2;
    منطقي needsTimescale = اظہاریہ_Format_2;
    صحيح format_دلیلاں = 0;
    إذا (!اظہاریہ_Format_2) {
        // Check display and argument count to head-off later runtime issues
        منطقي وچ_Pct = خطأ;
        منطقي ignore = خطأ;
        لكل (حرف ch : vformat) {
            // UINFO(1, "Parse '" << *pos << "'  IP" << inPct << " List " << cvtToHex(elistp));
            إذا (!وچ_Pct && ch == '%') {
                وچ_Pct = سچ;
                ignore = خطأ;
            } وإلا إذا (وچ_Pct && (std::ہندسہ_اے(ch) || ch == '.' || ch == '-')) {
            } وإلا إذا (!وچ_Pct) {  // Normal text
            } وإلا {  // Format character
                وچ_Pct = خطأ;
                ch = std::tolower(ch);
                بدل (ch) {
                حالة '%': اكسر;
                حالة '*':
                    وچ_Pct = سچ;  // Get more digits
                    ignore = سچ;
                    اكسر;
                حالة 'm': needsScope = سچ; اكسر;
                حالة 't':
                    needsTimescale = سچ;
                    ++format_دلیلاں;
                    اكسر;
                افتراضي:
                    إذا (!ignore && V3_نمبر::displayedFmtHasArg(ch, is_جانچ)) ++format_دلیلاں;
                    اكسر;
                }
            }
        }
    }

    إذا (vformat.خالی_تھاواں() && VN_IS(nodep, Display))  // not fscanf etc, as they need to return value
        أرجع;  // NOP

    ثابت منطقي isStmt = display_Emit_سرخی(nodep);

    إذا (اظہاریہ_Format_2) {
        UASSERT_OBJ(exprsp, nodep, "Missing format expression");
        iterateConst(exprsp);
        emitDatap(exprsp);
        exprsp = exprsp->nextp();
    } وإلا {
        ofp()->putsQuoted(vformat);
    }

    صحيح اظہاریہ_دلیلاں = 0;
    لكل (Ast_گرہ* argp = exprsp; argp; argp = argp->nextp()) ++اظہاریہ_دلیلاں;
    إذا (VL_UNCOVERABLE(!اظہاریہ_Format_2 && اظہاریہ_دلیلاں != format_دلیلاں)) {  // LCOV_EXCL_START
        // expectDisplay() checks this first, so probably internal error if found here
        nodep->v3error("Internal: Missing or extra arguments for $display-like format, "
                       << " expression had " << اظہاریہ_دلیلاں << ", format had " << format_دلیلاں);
    }  // LCOV_EXCL_STOP

    // argc for error check. Also MSVC++ requires va_args to not be off a reference
    صحيح argc = 0;
    إذا (needsScope) ++argc;
    إذا (needsTimescale) ++argc;
    لكل (Ast_گرہ* argp = exprsp; argp; argp = argp->nextp()) {
        ++argc;
        ثابت AstSFormatArg* ثابت fargp = VN_CAST(argp, SFormatArg);
        إذا (fargp && fargp->formatAttr().isEnum()) ++argc;  // Additional name argument
    }
    ofp()->puts("," + std::منزل_string(argc));

    إذا (needsScope) {
        AstScopeName* ثابت scopenamep = fmtp ? fmtp->scopeNamep() : خالی_سوچک;
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
        ofp()->puts(","s + cvt_منزل_Str((صحيح)timeunit.power_دا_Ten()));
    }

    // Arguments
    لكل (Ast_گرہ* argp = exprsp; argp; argp = argp->nextp()) {
        ofp()->indentInc();
        ofp()->putbs("");
        AstSFormatArg* ثابت fargp = VN_CAST(argp, SFormatArg);
        Ast_گرہ* ثابت subargp = fargp ? fargp->exprp() : argp;
        ثابت VFormatAttr formatAttr = AstSFormatArg::formatAttrDefauled(fargp, subargp->dtypep());
        puts(", '"s + formatAttr.ascii() + '\'');
        إذا (formatAttr.isSigned() || formatAttr.isUnsigned() || formatAttr.isEnum())
            puts("," + cvt_منزل_Str(subargp->چوڑائی_Min()));
        ثابت منطقي addrof = is_جانچ || formatAttr.isString() || formatAttr.isComplex();
        ثابت منطقي wideEnum = formatAttr.isEnum() && subargp->isWide();
        puts(",");
        إذا (wideEnum) puts("static_cast<const EData*>(");
        إذا (addrof) puts("&(");
        إذا (VN_IS(subargp, دھارا_R))
            emit_دھارا_R(
                VN_CAST(subargp, دھارا_R),
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

فراغ EmitCFunc::emit_CCall_دلیلاں(ثابت Ast_گرہ_CCall* nodep, ثابت string& self_اشارہ_گر,
                              منطقي وچ_Process) {
    putns(nodep, "(");
    منطقي comma = خطأ;
    إذا (nodep->funcp()->isLoose() && !nodep->funcp()->isStatic()) {
        UASSERT_OBJ(!self_اشارہ_گر.خالی_تھاواں(), nodep, "Call to loose method without self pointer");
        puts(self_اشارہ_گر);
        comma = سچ;
    }
    إذا (nodep->funcp()->needProcess()) {
        إذا (comma) puts(", ");
        إذا (VN_IS(nodep->backp(), CAwait) || !nodep->funcp()->isCoroutine()) {
            puts("vlProcess");
        } وإلا إذا (وچ_Process) {
            puts("std::make_shared<VlProcess>(vlProcess)");
        } وإلا {
            puts("std::make_shared<VlProcess>()");
        }
        comma = سچ;
    }
    إذا (!nodep->argTypes().خالی_تھاواں()) {
        إذا (comma) puts(", ");
        puts(nodep->argTypes());
        comma = سچ;
    }
    put_Comma_Iterate_اگلا(nodep->argsp(), comma);
    puts(")");
}

std::string EmitCFunc::dereferenceString(ثابت std::string& اشارہ_گر) ثابت {
    إذا (اشارہ_گر[0] == '(' && اشارہ_گر[1] == '&') {
        // remove "address of" followed by immediate dereference
        // Note: this relies on only the form '(&OBJECT)' being used by Verilator
        أرجع اشارہ_گر.substr(2, اشارہ_گر.لمائی() - 3) + '.';
    } وإلا {
        إذا (اشارہ_گر == "vlSelf" && m_usevlSelfRef) {
            أرجع "vlSelfRef.";
        } وإلا {
            أرجع اشارہ_گر + "->";
        }
    }
}
فراغ EmitCFunc::emitDereference(Ast_گرہ* nodep, ثابت string& اشارہ_گر) {
    putns(nodep, dereferenceString(اشارہ_گر));
}

فراغ EmitCFunc::emitCvtPackStr(Ast_گرہ* nodep) {
    إذا (ثابت AstConst* ثابت constp = VN_CAST(nodep, Const)) {
        emitConstantString(constp);
    } وإلا إذا (VN_IS(nodep->dtypep(), دھارا_DType)) {
        putnbs(nodep, "VL_CVT_PACK_STR_ND(");
        iterate_And_اگلا_Const_Null(nodep);
        puts(")");
    } وإلا {
        putnbs(nodep, "VL_CVT_PACK_STR_N");
        emitIQW(nodep);
        puts("(");
        إذا (nodep->isWide()) {
            // Note argument width, not node width (which is always 32)
            puts(cvt_منزل_Str(nodep->چوڑائی_الفاظ()));
            puts(", ");
        }
        iterate_And_اگلا_Const_Null(nodep);
        puts(")");
    }
}

فراغ EmitCFunc::emitConstant(AstConst* nodep) {
    // Put out constant set to the specified variable, or given variable in a string
    ثابت V3_نمبر& عدد = nodep->عدد();
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
    إذا (!str.خالی_تھاواں()) putsQuoted(str);
    puts("}");
}

فراغ EmitCFunc::emitVarReset(ثابت string& اگیتر, AstVar* varp, منطقي constructing) {
    // 'constructing' indicates that the object was just constructed, so if it is a string or
    // something that starts off clear already, no need to clear it again
    Ast_گرہ_DType* ثابت dtypep = varp->dtypep()->skipRefp();
    ثابت string vlSelf = VSelf_اشارہ_گر_متن::replaceThis(m_useSelfForThis, "this->");
    ثابت string varNameProtected
        = ((VN_IS(m_modp, Class) || varp->isFuncLocal()) || !اگیتر.خالی_تھاواں())
              ? varName(varp)
              : vlSelf + varName(varp);
    ثابت string new_اگیتر = اگیتر + varNameProtected;
    إذا (varp->isIO() && m_modp->is_چوٹی() && optSystemC()) {
        // System C top I/O doesn't need loading, as the lower level subinst code does it.}
    } وإلا إذا (varp->isParam()) {
        UASSERT_OBJ(varp->valuep(), varp, "No init for a param?");
        // If a simple CONST value we initialize it using an enum
        // If an ARRAYINIT we initialize it using an initial block similar to a signal
        // puts("// parameter "+varp->nameProtect()+" = "+varp->valuep()->name()+"\n");
    } وإلا {
        putns(varp,
              emitVarResetRecurse(varp, constructing, new_اگیتر, dtypep, 0, "", varp->valuep()));
    }
}

string EmitCFunc::emitVarResetRecurse(ثابت AstVar* varp, منطقي constructing,
                                      ثابت string& varNameProtected, Ast_گرہ_DType* dtypep,
                                      صحيح ڈونگھائی, ثابت string& suffix, ثابت Ast_گرہ* valuep) {
    dtypep = dtypep->skipRefp();
    AstBasicDType* ثابت basicp = dtypep->basicp();
    // Returns string to do resetting, empty to do nothing (which caller should handle)
    إذا (AstAssocArrayDType* ثابت adtypep = VN_CAST(dtypep, AssocArrayDType)) {
        // Access std::array as C array
        ثابت string pre = constructing ? "" : varNameProtected + suffix + ".clear();\n";
        أرجع pre
               + emitVarResetRecurse(varp, constructing, varNameProtected, adtypep->subDTypep(),
                                     ڈونگھائی + 1, suffix + ".atDefault()", خالی_سوچک);
    } وإلا إذا (AstWildcardArrayDType* ثابت adtypep = VN_CAST(dtypep, WildcardArrayDType)) {
        // Access std::array as C array
        ثابت string pre = constructing ? "" : varNameProtected + suffix + ".clear();\n";
        أرجع pre
               + emitVarResetRecurse(varp, constructing, varNameProtected, adtypep->subDTypep(),
                                     ڈونگھائی + 1, suffix + ".atDefault()", خالی_سوچک);
    } وإلا إذا (VN_IS(dtypep, CDType)) {
        أرجع "";  // Constructor does it
    } وإلا إذا (VN_IS(dtypep, CoverCrossDType) || VN_IS(dtypep, CoverpointDType)) {
        أرجع "";  // Covergroup constructor creates the runtime and assigns the pointer
    } وإلا إذا (ثابت AstClassRefDType* ثابت adtypep = VN_CAST(dtypep, ClassRefDType)) {
        أرجع adtypep->raw_اشارہ_گر() ? varNameProtected + suffix + " = nullptr;\n" : "";
    } وإلا إذا (VN_IS(dtypep, IfaceRefDType)) {
        أرجع varNameProtected + suffix + " = nullptr;\n";
    } وإلا إذا (ثابت AstDynArrayDType* ثابت adtypep = VN_CAST(dtypep, DynArrayDType)) {
        ثابت string pre = constructing ? "" : varNameProtected + suffix + ".clear();\n";
        أرجع pre
               + emitVarResetRecurse(varp, constructing, varNameProtected, adtypep->subDTypep(),
                                     ڈونگھائی + 1, suffix + ".atDefault()", خالی_سوچک);
    } وإلا إذا (ثابت Ast_قطار_DType* ثابت adtypep = VN_CAST(dtypep, قطار_DType)) {
        ثابت string pre = constructing ? "" : varNameProtected + suffix + ".clear();\n";
        أرجع pre
               + emitVarResetRecurse(varp, constructing, varNameProtected, adtypep->subDTypep(),
                                     ڈونگھائی + 1, suffix + ".atDefault()", خالی_سوچک);
    } وإلا إذا (VN_IS(dtypep, Sample_قطار_DType)) {
        أرجع "";
    } وإلا إذا (ثابت AstUnpackArrayDType* ثابت adtypep = VN_CAST(dtypep, UnpackArrayDType)) {
        UASSERT_OBJ(adtypep->hi() >= adtypep->lo(), varp,
                    "Should have swapped msb & lsb earlier.");
        ثابت Ast_گرہ_DType* ثابت elementp = adtypep->subDTypep()->skipRefp();
        إذا (!varp->isFuncLocal() && elementp->isIntegralOrPacked()) {
            // Scoped reset has no array-index input or mutable RNG state. Initialize one
            // element, then fill in the optimized runtime even when this code is cold.
            ثابت string پہلا
                = emitVarResetRecurse(varp, constructing, varNameProtected, adtypep->subDTypep(),
                                      ڈونگھائی + 1, suffix + "[0]", خالی_سوچک);
            ثابت string array = varNameProtected + suffix;
            أرجع پہلا + "VL_UNPACKED_FILL(" + array + ".data(), sizeof(" + array + "[0]), "
                   + cvt_منزل_Str(adtypep->elementsConst()) + ");\n";
        }
        ثابت string ivar = "__Vi"s + cvt_منزل_Str(ڈونگھائی);
        ثابت string pre = ("for (int " + ivar + " = " + cvt_منزل_Str(0) + "; " + ivar + " < "
                            + cvt_منزل_Str(adtypep->elementsConst()) + "; ++" + ivar + ") {\n");
        ثابت string ہیٹھاں
            = emitVarResetRecurse(varp, constructing, varNameProtected, adtypep->subDTypep(),
                                  ڈونگھائی + 1, suffix + "[" + ivar + "]", خالی_سوچک);
        ثابت string post = "}\n";
        أرجع ہیٹھاں.خالی_تھاواں() ? "" : pre + ہیٹھاں + post;
    } وإلا إذا (VN_IS(dtypep, گرہ_UOr_Struct_DType) && !VN_AS(dtypep, گرہ_UOr_Struct_DType)->packed()) {
        ثابت Ast_گرہ_UOr_Struct_DType* ثابت sdtypep = VN_AS(dtypep, گرہ_UOr_Struct_DType);
        string literal;
        لكل (ثابت AstMemberDType* itemp = sdtypep->membersp(); itemp;
             itemp = VN_AS(itemp->nextp(), MemberDType)) {
            ثابت std::string سطر = emitVarResetRecurse(
                varp, constructing, varNameProtected + suffix + "." + itemp->nameProtect(),
                itemp->dtypep(), ڈونگھائی + 1, "", itemp->valuep());
            إذا (!سطر.خالی_تھاواں()) literal += سطر;
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
               || (v3Global.opt.underlineZero() && !varp->name().خالی_تھاواں() && varp->name()[0] == '_')
               || (varp->varType().is_عارضی() && !varp->isXTemp())
               || (varp->isXTemp()
                       ? (v3Global.opt.x_سونپو() != "unique")
                       : (v3Global.opt.xInitial() == "fast" || v3Global.opt.xInitial() == "0")));
        ثابت منطقي slow = !varp->isFuncLocal() && !varp->isClassMember();
        ونڈو_ناپ_Inc(1);
        إذا (dtypep->isWide()) {  // Handle unpacked; not basicp->isWide
            string باہرلا;
            إذا (valuep) {
                ثابت AstConst* ثابت constp = VN_AS(valuep, Const);
                UASSERT_OBJ(constp, varp, "non-const initializer for variable");
                لكل (صحيح w = 0; w < dtypep->چوڑائی_الفاظ(); ++w) {
                    باہرلا += varNameProtected + suffix + "[" + cvt_منزل_Str(w) + "] = ";
                    باہرلا += cvt_منزل_Str(constp->عدد().edata_لفظ(w)) + "U;\n";
                }
            } وإلا {
                باہرلا += zeroit ? (slow ? "VL_ZERO_RESET_W(" : "VL_ZERO_W(")
                              : (varp->isXTemp() ? "VL_SCOPED_RAND_RESET_ASSIGN_W("
                                                 : "VL_SCOPED_RAND_RESET_W(");
                باہرلا += cvt_منزل_Str(dtypep->چوڑائی_Min());
                باہرلا += ", " + varNameProtected + suffix;
                إذا (!zeroit) {
                    emitVarResetScopeHash();
                    ثابت uint64_t salt = VString::hashMurmur(varp->prettyName());
                    باہرلا += ", ";
                    باہرلا += m_classOrPackage ? m_classOrPackageHash : "__VscopeHash";
                    باہرلا += ", ";
                    باہرلا += std::منزل_string(salt);
                    باہرلا += "ull";
                }
                باہرلا += ");\n";
            }
            أرجع باہرلا;
        } وإلا {
            string باہرلا = varNameProtected + suffix;
            إذا (valuep) {
                باہرلا += " = ";
                // TODO cleanup code shared between here, V3EmitCConstInit.h,
                // EmitCFunc::emitVarReset, EmitCFunc::emitConstant
                ثابت AstConst* ثابت constp = VN_AS(valuep, Const);
                UASSERT_OBJ(constp, varp, "non-const initializer for variable");
                باہرلا += cvt_منزل_Str(constp->عدد().edata_لفظ(0)) + "U;\n";
                باہرلا += ";\n";
            } وإلا إذا (zeroit) {
                باہرلا += " = 0;\n";
            } وإلا {
                emitVarResetScopeHash();
                ثابت uint64_t salt = VString::hashMurmur(varp->prettyName());
                باہرلا += " = VL_SCOPED_RAND_RESET_";
                إذا (varp->isXTemp()) باہرلا += "ASSIGN_";
                باہرلا += dtypep->charIQWN();
                باہرلا += "(" + cvt_منزل_Str(dtypep->چوڑائی_Min()) + ", "
                       + (m_classOrPackage ? m_classOrPackageHash : "__VscopeHash") + ", "
                       + std::منزل_string(salt) + "ull);\n";
            }
            أرجع باہرلا;
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
    m_createdScopeHash = سچ;
}
