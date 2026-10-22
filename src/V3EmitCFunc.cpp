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

منطقي EmitCFunc::emitSimpleOk(Ast_ڳنڍ_اظهار* nodep) {
    // Can we put out a simple (A + B) instead of VL_ADD_III(A,B)?
    إذا (nodep->emitSimpleOperator() == "") أرجع خطأ;
    إذا (nodep->isWide()) أرجع خطأ;
    إذا (nodep->op1p() && nodep->op1p()->isWide()) أرجع خطأ;
    إذا (nodep->op2p() && nodep->op2p()->isWide()) أرجع خطأ;
    إذا (nodep->op3p() && nodep->op3p()->isWide()) أرجع خطأ;
    أرجع سچ;
}

فراغ EmitCFunc::emitOpName(Ast_ڳنڍ* nodep, ثابت string& format, Ast_ڳنڍ* lhsp, Ast_ڳنڍ* rhsp,
                           Ast_ڳنڍ* thsp) {
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
    string ٻاهريون;
    putnbs(nodep, "");

    منطقي needComma = خطأ;
    منطقي استعمال_ڪري_قطار = خطأ;
    string ايندڙ_Comma;
    خودڪار comma_ٻاهريون = [&ٻاهريون, &ايندڙ_Comma]() {
        إذا (!ايندڙ_Comma.خالي_جايون()) {
            ٻاهريون += ايندڙ_Comma;
            ايندڙ_Comma = "";
        }
    };

    خودڪار put_ٻاهريون = [هذا, &ٻاهريون]() {
        إذا (!ٻاهريون.خالي_جايون()) puts(ٻاهريون);
        ٻاهريون = "";
    };

    لكل (string::const_iterator pos = format.begin(); pos != format.پڇاڙي(); ++pos) {
        إذا (pos[0] == ',') {
            // Remember we need to add one, but don't do yet to avoid ",)"
            إذا (needComma) {
                إذا (pos[1] == ' ') {
                    ايندڙ_Comma = ", ";
                } وإلا {
                    ايندڙ_Comma = ",";
                }
                needComma = خطأ;
            }
            إذا (pos[1] == ' ') ++pos;  // Must do even if no nextComma
        } وإلا إذا (pos[0] == '%') {
            ++pos;
            منطقي detail = خطأ;
            Ast_ڳنڍ* detailp = خالي_اشارو;
            بدل (pos[0]) {
            حالة '%': ٻاهريون += '%'; اكسر;
            حالة 'k':
                put_ٻاهريون();
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
                إذا (nodep->isWide() && !استعمال_ڪري_قطار) {
                    UASSERT_OBJ(m_wide_عارضي_Refp, nodep,
                                "Wide op " << nodep->prettyTypeName()
                                           << " w/ no temp, perhaps missing op in V3EmitC?");
                    comma_ٻاهريون();
                    put_ٻاهريون();
                    إذا (!m_wide_عارضي_Refp->self_اشارو().is_خالي_جايون()) {
                        emitDereference(m_wide_عارضي_Refp,
                                        m_wide_عارضي_Refp->self_اشارو_Protect(m_useSelfForThis));
                    }
                    ٻاهريون += varName(m_wide_عارضي_Refp->varp());
                    m_wide_عارضي_Refp = خالي_اشارو;
                    needComma = سچ;
                } وإلا إذا (استعمال_ڪري_قطار) {
                    comma_ٻاهريون();
                    put_ٻاهريون();
                    iterate_And_ايندڙ_Const_Null(nodep->backp()->op2p());
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
                    put_ٻاهريون();
                    // If we are assigning this to a queue we need to get the return type
                    إذا (VN_IS(detailp->backp(), سونپيو)
                        && VN_IS(detailp->backp()->op2p()->dtypep()->skipRefp(), قطار_DType)) {
                        puts("R");  // R for queue
                        استعمال_ڪري_قطار = سچ;
                    } وإلا إذا (VN_IS(detailp->dtypep()->skipRefp(), قطار_DType)
                               || VN_IS(detailp->dtypep()->skipRefp(), وهڪرو_DType)) {
                        puts("R");  // R for queue
                    } وإلا {
                        emitIQW(detailp);
                    }
                    اكسر;
                حالة 'w':
                    comma_ٻاهريون();
                    ٻاهريون += cvt_منزل_Str(detailp->ويڪر_Min());
                    needComma = سچ;
                    اكسر;
                حالة 'W':
                    إذا (lhsp->isWide()) {
                        comma_ٻاهريون();
                        ٻاهريون += cvt_منزل_Str(lhsp->ويڪر_لفظ());
                        needComma = سچ;
                    } وإلا إذا (VN_IS(lhsp, وهڪرو_R)) {
                        comma_ٻاهريون();
                        ٻاهريون += cvt_منزل_Str(rhsp->ويڪر_لفظ());
                        needComma = سچ;
                    }
                    اكسر;
                حالة 'i':
                    comma_ٻاهريون();
                    UASSERT_OBJ(detailp, nodep, "emitOperator() references undef node");
                    put_ٻاهريون();
                    iterate_And_ايندڙ_Const_Null(detailp);
                    needComma = سچ;
                    اكسر;
                افتراضي:
                    nodep->v3fatalSrc("Unknown emitOperator format code: %[nlrt]" << pos[0]);
                    اكسر;
                }
            }
        } وإلا إذا (pos[0] == ')') {
            ايندڙ_Comma = "";
            ٻاهريون += ')';
        } وإلا إذا (pos[0] == '(') {
            comma_ٻاهريون();
            needComma = خطأ;
            ٻاهريون += '(';
        } وإلا {
            // Normal text
            إذا (std::اکر_يا_انگ(pos[0])) needComma = سچ;
            comma_ٻاهريون();
            ٻاهريون += pos[0];
        }
    }
    put_ٻاهريون();
}

منطقي EmitCFunc::display_Emit_سرو(Ast_ڳنڍ* nodep) {
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
        puts(cvt_منزل_Str(dispp->fromp()->ويڪر_Min()));
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
            puts(cvt_منزل_Str(dispp->lhsp()->ويڪر_Min()));
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

فراغ EmitCFunc::display_ڳنڍ(Ast_ڳنڍ* nodep, AstSFormatF* fmtp,  // fmtp is nullptr for AstScan
                            ثابت string& vformat, Ast_ڳنڍ* exprsp, منطقي is_جاچ) {
    // Check format, if it exists
    ثابت منطقي اظهار_Format_2 = fmtp && fmtp->اظهار_Format_2();
    منطقي needsScope = اظهار_Format_2;
    منطقي needsTimescale = اظهار_Format_2;
    صحيح format_دليلون = 0;
    إذا (!اظهار_Format_2) {
        // Check display and argument count to head-off later runtime issues
        منطقي اندريون_Pct = خطأ;
        منطقي ignore = خطأ;
        لكل (حرف ch : vformat) {
            // UINFO(1, "Parse '" << *pos << "'  IP" << inPct << " List " << cvtToHex(elistp));
            إذا (!اندريون_Pct && ch == '%') {
                اندريون_Pct = سچ;
                ignore = خطأ;
            } وإلا إذا (اندريون_Pct && (std::انگ_آهي(ch) || ch == '.' || ch == '-')) {
            } وإلا إذا (!اندريون_Pct) {  // Normal text
            } وإلا {  // Format character
                اندريون_Pct = خطأ;
                ch = std::tolower(ch);
                بدل (ch) {
                حالة '%': اكسر;
                حالة '*':
                    اندريون_Pct = سچ;  // Get more digits
                    ignore = سچ;
                    اكسر;
                حالة 'm': needsScope = سچ; اكسر;
                حالة 't':
                    needsTimescale = سچ;
                    ++format_دليلون;
                    اكسر;
                افتراضي:
                    إذا (!ignore && V3_عدد::displayedFmtHasArg(ch, is_جاچ)) ++format_دليلون;
                    اكسر;
                }
            }
        }
    }

    إذا (vformat.خالي_جايون() && VN_IS(nodep, Display))  // not fscanf etc, as they need to return value
        أرجع;  // NOP

    ثابت منطقي isStmt = display_Emit_سرو(nodep);

    إذا (اظهار_Format_2) {
        UASSERT_OBJ(exprsp, nodep, "Missing format expression");
        iterateConst(exprsp);
        emitDatap(exprsp);
        exprsp = exprsp->nextp();
    } وإلا {
        ofp()->putsQuoted(vformat);
    }

    صحيح اظهار_دليلون = 0;
    لكل (Ast_ڳنڍ* argp = exprsp; argp; argp = argp->nextp()) ++اظهار_دليلون;
    إذا (VL_UNCOVERABLE(!اظهار_Format_2 && اظهار_دليلون != format_دليلون)) {  // LCOV_EXCL_START
        // expectDisplay() checks this first, so probably internal error if found here
        nodep->v3error("Internal: Missing or extra arguments for $display-like format, "
                       << " expression had " << اظهار_دليلون << ", format had " << format_دليلون);
    }  // LCOV_EXCL_STOP

    // argc for error check. Also MSVC++ requires va_args to not be off a reference
    صحيح argc = 0;
    إذا (needsScope) ++argc;
    إذا (needsTimescale) ++argc;
    لكل (Ast_ڳنڍ* argp = exprsp; argp; argp = argp->nextp()) {
        ++argc;
        ثابت AstSFormatArg* ثابت fargp = VN_CAST(argp, SFormatArg);
        إذا (fargp && fargp->formatAttr().isEnum()) ++argc;  // Additional name argument
    }
    ofp()->puts("," + std::منزل_string(argc));

    إذا (needsScope) {
        AstScopeName* ثابت scopenamep = fmtp ? fmtp->scopeNamep() : خالي_اشارو;
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
        ofp()->puts(","s + cvt_منزل_Str((صحيح)timeunit.power_جو_Ten()));
    }

    // Arguments
    لكل (Ast_ڳنڍ* argp = exprsp; argp; argp = argp->nextp()) {
        ofp()->indentInc();
        ofp()->putbs("");
        AstSFormatArg* ثابت fargp = VN_CAST(argp, SFormatArg);
        Ast_ڳنڍ* ثابت subargp = fargp ? fargp->exprp() : argp;
        ثابت VFormatAttr formatAttr = AstSFormatArg::formatAttrDefauled(fargp, subargp->dtypep());
        puts(", '"s + formatAttr.ascii() + '\'');
        إذا (formatAttr.isSigned() || formatAttr.isUnsigned() || formatAttr.isEnum())
            puts("," + cvt_منزل_Str(subargp->ويڪر_Min()));
        ثابت منطقي addrof = is_جاچ || formatAttr.isString() || formatAttr.isComplex();
        ثابت منطقي wideEnum = formatAttr.isEnum() && subargp->isWide();
        puts(",");
        إذا (wideEnum) puts("static_cast<const EData*>(");
        إذا (addrof) puts("&(");
        إذا (VN_IS(subargp, وهڪرو_R))
            emit_وهڪرو_R(
                VN_CAST(subargp, وهڪرو_R),
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

فراغ EmitCFunc::emit_CCall_دليلون(ثابت Ast_ڳنڍ_CCall* nodep, ثابت string& self_اشارو,
                              منطقي اندريون_Process) {
    putns(nodep, "(");
    منطقي comma = خطأ;
    إذا (nodep->funcp()->isLoose() && !nodep->funcp()->isStatic()) {
        UASSERT_OBJ(!self_اشارو.خالي_جايون(), nodep, "Call to loose method without self pointer");
        puts(self_اشارو);
        comma = سچ;
    }
    إذا (nodep->funcp()->needProcess()) {
        إذا (comma) puts(", ");
        إذا (VN_IS(nodep->backp(), CAwait) || !nodep->funcp()->isCoroutine()) {
            puts("vlProcess");
        } وإلا إذا (اندريون_Process) {
            puts("std::make_shared<VlProcess>(vlProcess)");
        } وإلا {
            puts("std::make_shared<VlProcess>()");
        }
        comma = سچ;
    }
    إذا (!nodep->argTypes().خالي_جايون()) {
        إذا (comma) puts(", ");
        puts(nodep->argTypes());
        comma = سچ;
    }
    put_Comma_Iterate_ايندڙ(nodep->argsp(), comma);
    puts(")");
}

std::string EmitCFunc::dereferenceString(ثابت std::string& اشارو) ثابت {
    إذا (اشارو[0] == '(' && اشارو[1] == '&') {
        // remove "address of" followed by immediate dereference
        // Note: this relies on only the form '(&OBJECT)' being used by Verilator
        أرجع اشارو.substr(2, اشارو.ڊيگهائي() - 3) + '.';
    } وإلا {
        إذا (اشارو == "vlSelf" && m_usevlSelfRef) {
            أرجع "vlSelfRef.";
        } وإلا {
            أرجع اشارو + "->";
        }
    }
}
فراغ EmitCFunc::emitDereference(Ast_ڳنڍ* nodep, ثابت string& اشارو) {
    putns(nodep, dereferenceString(اشارو));
}

فراغ EmitCFunc::emitCvtPackStr(Ast_ڳنڍ* nodep) {
    إذا (ثابت AstConst* ثابت constp = VN_CAST(nodep, Const)) {
        emitConstantString(constp);
    } وإلا إذا (VN_IS(nodep->dtypep(), وهڪرو_DType)) {
        putnbs(nodep, "VL_CVT_PACK_STR_ND(");
        iterate_And_ايندڙ_Const_Null(nodep);
        puts(")");
    } وإلا {
        putnbs(nodep, "VL_CVT_PACK_STR_N");
        emitIQW(nodep);
        puts("(");
        إذا (nodep->isWide()) {
            // Note argument width, not node width (which is always 32)
            puts(cvt_منزل_Str(nodep->ويڪر_لفظ()));
            puts(", ");
        }
        iterate_And_ايندڙ_Const_Null(nodep);
        puts(")");
    }
}

فراغ EmitCFunc::emitConstant(AstConst* nodep) {
    // Put out constant set to the specified variable, or given variable in a string
    ثابت V3_عدد& انگ = nodep->انگ();
    إذا (انگ.isFourState()) {
        nodep->v3warn(E_UNSUPPORTED, "Unsupported: 4-state numbers in this context");
        أرجع;
    }
    putns(nodep, انگ.emitC());
}

فراغ EmitCFunc::emitConstantString(ثابت AstConst* nodep) {
    // Const might be a Verilog array-type string, but need to always output std::string
    putnbs(nodep, "std::string{");
    ثابت string str = nodep->انگ().منزل_String();
    إذا (!str.خالي_جايون()) putsQuoted(str);
    puts("}");
}

فراغ EmitCFunc::emitVarReset(ثابت string& اڳياڙي, AstVar* varp, منطقي constructing) {
    // 'constructing' indicates that the object was just constructed, so if it is a string or
    // something that starts off clear already, no need to clear it again
    Ast_ڳنڍ_DType* ثابت dtypep = varp->dtypep()->skipRefp();
    ثابت string vlSelf = VSelf_اشارو_متن::replaceThis(m_useSelfForThis, "this->");
    ثابت string varNameProtected
        = ((VN_IS(m_modp, Class) || varp->isFuncLocal()) || !اڳياڙي.خالي_جايون())
              ? varName(varp)
              : vlSelf + varName(varp);
    ثابت string new_اڳياڙي = اڳياڙي + varNameProtected;
    إذا (varp->isIO() && m_modp->is_چوٽي() && optSystemC()) {
        // System C top I/O doesn't need loading, as the lower level subinst code does it.}
    } وإلا إذا (varp->isParam()) {
        UASSERT_OBJ(varp->valuep(), varp, "No init for a param?");
        // If a simple CONST value we initialize it using an enum
        // If an ARRAYINIT we initialize it using an initial block similar to a signal
        // puts("// parameter "+varp->nameProtect()+" = "+varp->valuep()->name()+"\n");
    } وإلا {
        putns(varp,
              emitVarResetRecurse(varp, constructing, new_اڳياڙي, dtypep, 0, "", varp->valuep()));
    }
}

string EmitCFunc::emitVarResetRecurse(ثابت AstVar* varp, منطقي constructing,
                                      ثابت string& varNameProtected, Ast_ڳنڍ_DType* dtypep,
                                      صحيح اونهائي, ثابت string& suffix, ثابت Ast_ڳنڍ* valuep) {
    dtypep = dtypep->skipRefp();
    AstBasicDType* ثابت basicp = dtypep->basicp();
    // Returns string to do resetting, empty to do nothing (which caller should handle)
    إذا (AstAssocArrayDType* ثابت adtypep = VN_CAST(dtypep, AssocArrayDType)) {
        // Access std::array as C array
        ثابت string pre = constructing ? "" : varNameProtected + suffix + ".clear();\n";
        أرجع pre
               + emitVarResetRecurse(varp, constructing, varNameProtected, adtypep->subDTypep(),
                                     اونهائي + 1, suffix + ".atDefault()", خالي_اشارو);
    } وإلا إذا (AstWildcardArrayDType* ثابت adtypep = VN_CAST(dtypep, WildcardArrayDType)) {
        // Access std::array as C array
        ثابت string pre = constructing ? "" : varNameProtected + suffix + ".clear();\n";
        أرجع pre
               + emitVarResetRecurse(varp, constructing, varNameProtected, adtypep->subDTypep(),
                                     اونهائي + 1, suffix + ".atDefault()", خالي_اشارو);
    } وإلا إذا (VN_IS(dtypep, CDType)) {
        أرجع "";  // Constructor does it
    } وإلا إذا (VN_IS(dtypep, CoverCrossDType) || VN_IS(dtypep, CoverpointDType)) {
        أرجع "";  // Covergroup constructor creates the runtime and assigns the pointer
    } وإلا إذا (ثابت AstClassRefDType* ثابت adtypep = VN_CAST(dtypep, ClassRefDType)) {
        أرجع adtypep->raw_اشارو() ? varNameProtected + suffix + " = nullptr;\n" : "";
    } وإلا إذا (VN_IS(dtypep, IfaceRefDType)) {
        أرجع varNameProtected + suffix + " = nullptr;\n";
    } وإلا إذا (ثابت AstDynArrayDType* ثابت adtypep = VN_CAST(dtypep, DynArrayDType)) {
        ثابت string pre = constructing ? "" : varNameProtected + suffix + ".clear();\n";
        أرجع pre
               + emitVarResetRecurse(varp, constructing, varNameProtected, adtypep->subDTypep(),
                                     اونهائي + 1, suffix + ".atDefault()", خالي_اشارو);
    } وإلا إذا (ثابت Ast_قطار_DType* ثابت adtypep = VN_CAST(dtypep, قطار_DType)) {
        ثابت string pre = constructing ? "" : varNameProtected + suffix + ".clear();\n";
        أرجع pre
               + emitVarResetRecurse(varp, constructing, varNameProtected, adtypep->subDTypep(),
                                     اونهائي + 1, suffix + ".atDefault()", خالي_اشارو);
    } وإلا إذا (VN_IS(dtypep, Sample_قطار_DType)) {
        أرجع "";
    } وإلا إذا (ثابت AstUnpackArrayDType* ثابت adtypep = VN_CAST(dtypep, UnpackArrayDType)) {
        UASSERT_OBJ(adtypep->hi() >= adtypep->lo(), varp,
                    "Should have swapped msb & lsb earlier.");
        ثابت Ast_ڳنڍ_DType* ثابت elementp = adtypep->subDTypep()->skipRefp();
        إذا (!varp->isFuncLocal() && elementp->isIntegralOrPacked()) {
            // Scoped reset has no array-index input or mutable RNG state. Initialize one
            // element, then fill in the optimized runtime even when this code is cold.
            ثابت string پهريون
                = emitVarResetRecurse(varp, constructing, varNameProtected, adtypep->subDTypep(),
                                      اونهائي + 1, suffix + "[0]", خالي_اشارو);
            ثابت string array = varNameProtected + suffix;
            أرجع پهريون + "VL_UNPACKED_FILL(" + array + ".data(), sizeof(" + array + "[0]), "
                   + cvt_منزل_Str(adtypep->elementsConst()) + ");\n";
        }
        ثابت string ivar = "__Vi"s + cvt_منزل_Str(اونهائي);
        ثابت string pre = ("for (int " + ivar + " = " + cvt_منزل_Str(0) + "; " + ivar + " < "
                            + cvt_منزل_Str(adtypep->elementsConst()) + "; ++" + ivar + ") {\n");
        ثابت string هيٺ
            = emitVarResetRecurse(varp, constructing, varNameProtected, adtypep->subDTypep(),
                                  اونهائي + 1, suffix + "[" + ivar + "]", خالي_اشارو);
        ثابت string post = "}\n";
        أرجع هيٺ.خالي_جايون() ? "" : pre + هيٺ + post;
    } وإلا إذا (VN_IS(dtypep, ڳنڍ_UOr_Struct_DType) && !VN_AS(dtypep, ڳنڍ_UOr_Struct_DType)->packed()) {
        ثابت Ast_ڳنڍ_UOr_Struct_DType* ثابت sdtypep = VN_AS(dtypep, ڳنڍ_UOr_Struct_DType);
        string literal;
        لكل (ثابت AstMemberDType* itemp = sdtypep->membersp(); itemp;
             itemp = VN_AS(itemp->nextp(), MemberDType)) {
            ثابت std::string سٽ = emitVarResetRecurse(
                varp, constructing, varNameProtected + suffix + "." + itemp->nameProtect(),
                itemp->dtypep(), اونهائي + 1, "", itemp->valuep());
            إذا (!سٽ.خالي_جايون()) literal += سٽ;
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
               || (v3Global.opt.underlineZero() && !varp->name().خالي_جايون() && varp->name()[0] == '_')
               || (varp->varType().is_عارضي() && !varp->isXTemp())
               || (varp->isXTemp()
                       ? (v3Global.opt.x_سونپيو() != "unique")
                       : (v3Global.opt.xInitial() == "fast" || v3Global.opt.xInitial() == "0")));
        ثابت منطقي slow = !varp->isFuncLocal() && !varp->isClassMember();
        ورهايو_سائيز_Inc(1);
        إذا (dtypep->isWide()) {  // Handle unpacked; not basicp->isWide
            string ٻاهريون;
            إذا (valuep) {
                ثابت AstConst* ثابت constp = VN_AS(valuep, Const);
                UASSERT_OBJ(constp, varp, "non-const initializer for variable");
                لكل (صحيح w = 0; w < dtypep->ويڪر_لفظ(); ++w) {
                    ٻاهريون += varNameProtected + suffix + "[" + cvt_منزل_Str(w) + "] = ";
                    ٻاهريون += cvt_منزل_Str(constp->انگ().edata_لفظ(w)) + "U;\n";
                }
            } وإلا {
                ٻاهريون += zeroit ? (slow ? "VL_ZERO_RESET_W(" : "VL_ZERO_W(")
                              : (varp->isXTemp() ? "VL_SCOPED_RAND_RESET_ASSIGN_W("
                                                 : "VL_SCOPED_RAND_RESET_W(");
                ٻاهريون += cvt_منزل_Str(dtypep->ويڪر_Min());
                ٻاهريون += ", " + varNameProtected + suffix;
                إذا (!zeroit) {
                    emitVarResetScopeHash();
                    ثابت uint64_t salt = VString::hashMurmur(varp->prettyName());
                    ٻاهريون += ", ";
                    ٻاهريون += m_classOrPackage ? m_classOrPackageHash : "__VscopeHash";
                    ٻاهريون += ", ";
                    ٻاهريون += std::منزل_string(salt);
                    ٻاهريون += "ull";
                }
                ٻاهريون += ");\n";
            }
            أرجع ٻاهريون;
        } وإلا {
            string ٻاهريون = varNameProtected + suffix;
            إذا (valuep) {
                ٻاهريون += " = ";
                // TODO cleanup code shared between here, V3EmitCConstInit.h,
                // EmitCFunc::emitVarReset, EmitCFunc::emitConstant
                ثابت AstConst* ثابت constp = VN_AS(valuep, Const);
                UASSERT_OBJ(constp, varp, "non-const initializer for variable");
                ٻاهريون += cvt_منزل_Str(constp->انگ().edata_لفظ(0)) + "U;\n";
                ٻاهريون += ";\n";
            } وإلا إذا (zeroit) {
                ٻاهريون += " = 0;\n";
            } وإلا {
                emitVarResetScopeHash();
                ثابت uint64_t salt = VString::hashMurmur(varp->prettyName());
                ٻاهريون += " = VL_SCOPED_RAND_RESET_";
                إذا (varp->isXTemp()) ٻاهريون += "ASSIGN_";
                ٻاهريون += dtypep->charIQWN();
                ٻاهريون += "(" + cvt_منزل_Str(dtypep->ويڪر_Min()) + ", "
                       + (m_classOrPackage ? m_classOrPackageHash : "__VscopeHash") + ", "
                       + std::منزل_string(salt) + "ull);\n";
            }
            أرجع ٻاهريون;
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
