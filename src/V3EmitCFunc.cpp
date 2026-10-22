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

#അന്തര്ഭാവ "V3PchAstMT.h"

#അന്തര്ഭാവ "V3EmitCFunc.h"

#അന്തര്ഭാവ <map>
#അന്തര്ഭാവ <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// EmitCFunc

തർക്കം EmitCFunc::emitSimpleOk(Ast_കണ്ണി_വ്യഞ്ജകം* nodep) {
    // Can we put out a simple (A + B) instead of VL_ADD_III(A,B)?
    എങ്കിൽ (nodep->emitSimpleOperator() == "") തിരികെ അസത്യം;
    എങ്കിൽ (nodep->isWide()) തിരികെ അസത്യം;
    എങ്കിൽ (nodep->op1p() && nodep->op1p()->isWide()) തിരികെ അസത്യം;
    എങ്കിൽ (nodep->op2p() && nodep->op2p()->isWide()) തിരികെ അസത്യം;
    എങ്കിൽ (nodep->op3p() && nodep->op3p()->isWide()) തിരികെ അസത്യം;
    തിരികെ സത്യം;
}

ശൂന്യം EmitCFunc::emitOpName(Ast_കണ്ണി* nodep, സ്ഥിരം string& format, Ast_കണ്ണി* lhsp, Ast_കണ്ണി* rhsp,
                           Ast_കണ്ണി* thsp) {
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
    string out;
    putnbs(nodep, "");

    തർക്കം needComma = അസത്യം;
    തർക്കം usesQueue = അസത്യം;
    string nextComma;
    auto commaOut = [&out, &nextComma]() {
        എങ്കിൽ (!nextComma.empty()) {
            out += nextComma;
            nextComma = "";
        }
    };

    auto putOut = [ഇത്, &out]() {
        എങ്കിൽ (!out.empty()) puts(out);
        out = "";
    };

    വേണ്ടി (string::const_iterator pos = format.begin(); pos != format.end(); ++pos) {
        എങ്കിൽ (pos[0] == ',') {
            // Remember we need to add one, but don't do yet to avoid ",)"
            എങ്കിൽ (needComma) {
                എങ്കിൽ (pos[1] == ' ') {
                    nextComma = ", ";
                } അല്ലെങ്കിൽ {
                    nextComma = ",";
                }
                needComma = അസത്യം;
            }
            എങ്കിൽ (pos[1] == ' ') ++pos;  // Must do even if no nextComma
        } അല്ലെങ്കിൽ എങ്കിൽ (pos[0] == '%') {
            ++pos;
            തർക്കം detail = അസത്യം;
            Ast_കണ്ണി* detailp = nullptr;
            തിരഞ്ഞെടുപ്പ് (pos[0]) {
            അവസ്ഥ '%': out += '%'; നിർത്തുക;
            അവസ്ഥ 'k':
                putOut();
                putbs("");
                നിർത്തുക;
            അവസ്ഥ 'n':
                detail = സത്യം;
                detailp = nodep;
                നിർത്തുക;
            അവസ്ഥ 'l':
                detail = സത്യം;
                detailp = lhsp;
                നിർത്തുക;
            അവസ്ഥ 'r':
                detail = സത്യം;
                detailp = rhsp;
                നിർത്തുക;
            അവസ്ഥ 't':
                detail = സത്യം;
                detailp = thsp;
                നിർത്തുക;
            അവസ്ഥ 'P':
                എങ്കിൽ (nodep->isWide() && !usesQueue) {
                    UASSERT_OBJ(m_wideTempRefp, nodep,
                                "Wide op " << nodep->prettyTypeName()
                                           << " w/ no temp, perhaps missing op in V3EmitC?");
                    commaOut();
                    putOut();
                    എങ്കിൽ (!m_wideTempRefp->selfPointer().isEmpty()) {
                        emitDereference(m_wideTempRefp,
                                        m_wideTempRefp->selfPointerProtect(m_useSelfForThis));
                    }
                    out += varName(m_wideTempRefp->varp());
                    m_wideTempRefp = nullptr;
                    needComma = സത്യം;
                } അല്ലെങ്കിൽ എങ്കിൽ (usesQueue) {
                    commaOut();
                    putOut();
                    iterateAndNextConstNull(nodep->backp()->op2p());
                    needComma = സത്യം;
                }

                നിർത്തുക;
            സ്ഥിരസ്ഥിതി: nodep->v3fatalSrc("Unknown emitOperator format code: %" << pos[0]); നിർത്തുക;
            }
            എങ്കിൽ (detail) {
                // Get next letter of %[nlrt]
                ++pos;
                തിരഞ്ഞെടുപ്പ് (pos[0]) {
                അവസ്ഥ 'q':
                    putOut();
                    // If we are assigning this to a queue we need to get the return type
                    എങ്കിൽ (VN_IS(detailp->backp(), Assign)
                        && VN_IS(detailp->backp()->op2p()->dtypep()->skipRefp(), QueueDType)) {
                        puts("R");  // R for queue
                        usesQueue = സത്യം;
                    } അല്ലെങ്കിൽ എങ്കിൽ (VN_IS(detailp->dtypep()->skipRefp(), QueueDType)
                               || VN_IS(detailp->dtypep()->skipRefp(), StreamDType)) {
                        puts("R");  // R for queue
                    } അല്ലെങ്കിൽ {
                        emitIQW(detailp);
                    }
                    നിർത്തുക;
                അവസ്ഥ 'w':
                    commaOut();
                    out += cvtToStr(detailp->widthMin());
                    needComma = സത്യം;
                    നിർത്തുക;
                അവസ്ഥ 'W':
                    എങ്കിൽ (lhsp->isWide()) {
                        commaOut();
                        out += cvtToStr(lhsp->width_വാക്കുകൾ());
                        needComma = സത്യം;
                    } അല്ലെങ്കിൽ എങ്കിൽ (VN_IS(lhsp, StreamR)) {
                        commaOut();
                        out += cvtToStr(rhsp->width_വാക്കുകൾ());
                        needComma = സത്യം;
                    }
                    നിർത്തുക;
                അവസ്ഥ 'i':
                    commaOut();
                    UASSERT_OBJ(detailp, nodep, "emitOperator() references undef node");
                    putOut();
                    iterateAndNextConstNull(detailp);
                    needComma = സത്യം;
                    നിർത്തുക;
                സ്ഥിരസ്ഥിതി:
                    nodep->v3fatalSrc("Unknown emitOperator format code: %[nlrt]" << pos[0]);
                    നിർത്തുക;
                }
            }
        } അല്ലെങ്കിൽ എങ്കിൽ (pos[0] == ')') {
            nextComma = "";
            out += ')';
        } അല്ലെങ്കിൽ എങ്കിൽ (pos[0] == '(') {
            commaOut();
            needComma = അസത്യം;
            out += '(';
        } അല്ലെങ്കിൽ {
            // Normal text
            എങ്കിൽ (std::isalnum(pos[0])) needComma = സത്യം;
            commaOut();
            out += pos[0];
        }
    }
    putOut();
}

തർക്കം EmitCFunc::displayEmitHeader(Ast_കണ്ണി* nodep) {
    തർക്കം isStmt = അസത്യം;
    എങ്കിൽ (സ്ഥിരം AstFScanF* സ്ഥിരം dispp = VN_CAST(nodep, FScanF)) {
        isStmt = അസത്യം;
        putns(nodep, "VL_FSCANF_INX(");
        iterateConst(dispp->filep());
        puts(",");
    } അല്ലെങ്കിൽ എങ്കിൽ (സ്ഥിരം AstSScanF* സ്ഥിരം dispp = VN_CAST(nodep, SScanF)) {
        isStmt = അസത്യം;
        putns(nodep, "VL_SSCANF_I");
        emitIQW(dispp->fromp());
        puts("NX(");
        puts(cvtToStr(dispp->fromp()->widthMin()));
        puts(",");
        iterateConst(dispp->fromp());
        puts(",");
    } അല്ലെങ്കിൽ എങ്കിൽ (സ്ഥിരം AstDisplay* സ്ഥിരം dispp = VN_CAST(nodep, Display)) {
        isStmt = സത്യം;
        എങ്കിൽ (dispp->filep()) {
            putns(nodep, "VL_FWRITEF_NX(");
            iterateConst(dispp->filep());
            puts(",");
        } അല്ലെങ്കിൽ {
            putns(nodep, "VL_WRITEF_NX(");
        }
    } അല്ലെങ്കിൽ എങ്കിൽ (സ്ഥിരം AstSFormat* സ്ഥിരം dispp = VN_CAST(nodep, SFormat)) {
        isStmt = സത്യം;
        puts("VL_SFORMAT_NX(");
        എങ്കിൽ (!dispp->lhsp()->dtypep()->isString()) {
            puts(cvtToStr(dispp->lhsp()->widthMin()));
            putbs(",");
        }
        iterateConst(dispp->lhsp());
        emitDatap(dispp->lhsp());
        putbs(",");
    } അല്ലെങ്കിൽ എങ്കിൽ (VN_IS(nodep, SFormatF)) {
        isStmt = അസത്യം;
        putns(nodep, "VL_SFORMATF_N_NX(");
    } അല്ലെങ്കിൽ {
        nodep->v3fatalSrc("Unknown displayEmit node type");
    }
    തിരികെ isStmt;
}

ശൂന്യം EmitCFunc::display_കണ്ണി(Ast_കണ്ണി* nodep, AstSFormatF* fmtp,  // fmtp is nullptr for AstScan
                            സ്ഥിരം string& vformat, Ast_കണ്ണി* exprsp, തർക്കം isScan) {
    // Check format, if it exists
    സ്ഥിരം തർക്കം വ്യഞ്ജകം_Format_2 = fmtp && fmtp->വ്യഞ്ജകം_Format_2();
    തർക്കം needsScope = വ്യഞ്ജകം_Format_2;
    തർക്കം needsTimescale = വ്യഞ്ജകം_Format_2;
    പൂർണ്ണസംഖ്യ formatArgs = 0;
    എങ്കിൽ (!വ്യഞ്ജകം_Format_2) {
        // Check display and argument count to head-off later runtime issues
        തർക്കം inPct = അസത്യം;
        തർക്കം ignore = അസത്യം;
        വേണ്ടി (അക്ഷരം ch : vformat) {
            // UINFO(1, "Parse '" << *pos << "'  IP" << inPct << " List " << cvtToHex(elistp));
            എങ്കിൽ (!inPct && ch == '%') {
                inPct = സത്യം;
                ignore = അസത്യം;
            } അല്ലെങ്കിൽ എങ്കിൽ (inPct && (std::isdigit(ch) || ch == '.' || ch == '-')) {
            } അല്ലെങ്കിൽ എങ്കിൽ (!inPct) {  // Normal text
            } അല്ലെങ്കിൽ {  // Format character
                inPct = അസത്യം;
                ch = std::tolower(ch);
                തിരഞ്ഞെടുപ്പ് (ch) {
                അവസ്ഥ '%': നിർത്തുക;
                അവസ്ഥ '*':
                    inPct = സത്യം;  // Get more digits
                    ignore = സത്യം;
                    നിർത്തുക;
                അവസ്ഥ 'm': needsScope = സത്യം; നിർത്തുക;
                അവസ്ഥ 't':
                    needsTimescale = സത്യം;
                    ++formatArgs;
                    നിർത്തുക;
                സ്ഥിരസ്ഥിതി:
                    എങ്കിൽ (!ignore && V3_സംഖ്യ::displayedFmtHasArg(ch, isScan)) ++formatArgs;
                    നിർത്തുക;
                }
            }
        }
    }

    എങ്കിൽ (vformat.empty() && VN_IS(nodep, Display))  // not fscanf etc, as they need to return value
        തിരികെ;  // NOP

    സ്ഥിരം തർക്കം isStmt = displayEmitHeader(nodep);

    എങ്കിൽ (വ്യഞ്ജകം_Format_2) {
        UASSERT_OBJ(exprsp, nodep, "Missing format expression");
        iterateConst(exprsp);
        emitDatap(exprsp);
        exprsp = exprsp->nextp();
    } അല്ലെങ്കിൽ {
        ofp()->putsQuoted(vformat);
    }

    പൂർണ്ണസംഖ്യ വ്യഞ്ജകം_Args = 0;
    വേണ്ടി (Ast_കണ്ണി* argp = exprsp; argp; argp = argp->nextp()) ++വ്യഞ്ജകം_Args;
    എങ്കിൽ (VL_UNCOVERABLE(!വ്യഞ്ജകം_Format_2 && വ്യഞ്ജകം_Args != formatArgs)) {  // LCOV_EXCL_START
        // expectDisplay() checks this first, so probably internal error if found here
        nodep->v3error("Internal: Missing or extra arguments for $display-like format, "
                       << " expression had " << വ്യഞ്ജകം_Args << ", format had " << formatArgs);
    }  // LCOV_EXCL_STOP

    // argc for error check. Also MSVC++ requires va_args to not be off a reference
    പൂർണ്ണസംഖ്യ argc = 0;
    എങ്കിൽ (needsScope) ++argc;
    എങ്കിൽ (needsTimescale) ++argc;
    വേണ്ടി (Ast_കണ്ണി* argp = exprsp; argp; argp = argp->nextp()) {
        ++argc;
        സ്ഥിരം AstSFormatArg* സ്ഥിരം fargp = VN_CAST(argp, SFormatArg);
        എങ്കിൽ (fargp && fargp->formatAttr().isEnum()) ++argc;  // Additional name argument
    }
    ofp()->puts("," + std::to_string(argc));

    എങ്കിൽ (needsScope) {
        AstScopeName* സ്ഥിരം scopenamep = fmtp ? fmtp->scopeNamep() : nullptr;
        UASSERT_OBJ(scopenamep, nodep, "Display with %m but no AstScopeName");
        ofp()->puts(", '"s + VFormatAttr{VFormatAttr::SCOPE}.ascii() + "'");
        എങ്കിൽ (scopenamep->selfRelative()) {
            // vlNamep is catName(name(), instance), so starts with '.' when the model is unnamed
            സ്ഥിരം string namep = string{m_useSelfForThis ? "vlSelf" : "this"} + "->vlNamep";
            ofp()->puts(",(vlSymsp->name()[0] ? " + namep + " : " + namep + " + 1),");
            ofp()->putsQuoted(scopenamep->scopeAttr());
        } അല്ലെങ്കിൽ {
            ofp()->puts(",vlSymsp->name(),");
            ofp()->putsQuoted(scopenamep->scopePrettySymName());
        }
    }

    എങ്കിൽ (needsTimescale) {
        VTimescale timeunit = VTimescale::NONE;
        എങ്കിൽ (സ്ഥിരം AstDisplay* സ്ഥിരം anodep = VN_CAST(nodep, Display)) {
            timeunit = anodep->fmtp()->timeunit();
        } അല്ലെങ്കിൽ എങ്കിൽ (സ്ഥിരം AstSFormat* സ്ഥിരം anodep = VN_CAST(nodep, SFormat)) {
            timeunit = anodep->fmtp()->timeunit();
        } അല്ലെങ്കിൽ എങ്കിൽ (സ്ഥിരം AstSScanF* സ്ഥിരം anodep = VN_CAST(nodep, SScanF)) {
            timeunit = anodep->timeunit();
        } അല്ലെങ്കിൽ എങ്കിൽ (സ്ഥിരം AstSFormatF* സ്ഥിരം anodep = VN_CAST(nodep, SFormatF)) {
            timeunit = anodep->timeunit();
        }
        UASSERT_OBJ(!timeunit.isNone(), nodep,
                    "Use of %t must be under AstDisplay, AstSFormat, or AstSFormatF, or "
                    "SScanF, and timeunit set");
        ofp()->puts(", '"s + VFormatAttr{VFormatAttr::TIMEUNIT}.ascii() + "'");
        ofp()->puts(","s + cvtToStr((പൂർണ്ണസംഖ്യ)timeunit.powerOfTen()));
    }

    // Arguments
    വേണ്ടി (Ast_കണ്ണി* argp = exprsp; argp; argp = argp->nextp()) {
        ofp()->indentInc();
        ofp()->putbs("");
        AstSFormatArg* സ്ഥിരം fargp = VN_CAST(argp, SFormatArg);
        Ast_കണ്ണി* സ്ഥിരം subargp = fargp ? fargp->exprp() : argp;
        സ്ഥിരം VFormatAttr formatAttr = AstSFormatArg::formatAttrDefauled(fargp, subargp->dtypep());
        puts(", '"s + formatAttr.ascii() + '\'');
        എങ്കിൽ (formatAttr.isSigned() || formatAttr.isUnsigned() || formatAttr.isEnum())
            puts("," + cvtToStr(subargp->widthMin()));
        സ്ഥിരം തർക്കം addrof = isScan || formatAttr.isString() || formatAttr.isComplex();
        സ്ഥിരം തർക്കം wideEnum = formatAttr.isEnum() && subargp->isWide();
        puts(",");
        എങ്കിൽ (wideEnum) puts("static_cast<const EData*>(");
        എങ്കിൽ (addrof) puts("&(");
        എങ്കിൽ (VN_IS(subargp, StreamR))
            emitStreamR(
                VN_CAST(subargp, StreamR),
                nodep);  // This has to be done here because streamR doesn't know what it returns
        അല്ലെങ്കിൽ { iterateConst(subargp); }
        എങ്കിൽ (addrof) puts(")");
        എങ്കിൽ (!addrof) emitDatap(argp);
        എങ്കിൽ (wideEnum) puts(")");
        എങ്കിൽ (formatAttr.isEnum()) {
            puts(", '"s + VFormatAttr{VFormatAttr::STRING}.ascii() + "', &(");
            iterateConst(fargp->namep());
            puts(")");
        }
        ofp()->indentDec();
    }

    // End
    എങ്കിൽ (isStmt) {
        puts(");\n");
    } അല്ലെങ്കിൽ {
        puts(") ");
    }
}

ശൂന്യം EmitCFunc::emitCCallArgs(സ്ഥിരം Ast_കണ്ണി_CCall* nodep, സ്ഥിരം string& selfPointer,
                              തർക്കം inProcess) {
    putns(nodep, "(");
    തർക്കം comma = അസത്യം;
    എങ്കിൽ (nodep->funcp()->isLoose() && !nodep->funcp()->isStatic()) {
        UASSERT_OBJ(!selfPointer.empty(), nodep, "Call to loose method without self pointer");
        puts(selfPointer);
        comma = സത്യം;
    }
    എങ്കിൽ (nodep->funcp()->needProcess()) {
        എങ്കിൽ (comma) puts(", ");
        എങ്കിൽ (VN_IS(nodep->backp(), CAwait) || !nodep->funcp()->isCoroutine()) {
            puts("vlProcess");
        } അല്ലെങ്കിൽ എങ്കിൽ (inProcess) {
            puts("std::make_shared<VlProcess>(vlProcess)");
        } അല്ലെങ്കിൽ {
            puts("std::make_shared<VlProcess>()");
        }
        comma = സത്യം;
    }
    എങ്കിൽ (!nodep->argTypes().empty()) {
        എങ്കിൽ (comma) puts(", ");
        puts(nodep->argTypes());
        comma = സത്യം;
    }
    putCommaIterateNext(nodep->argsp(), comma);
    puts(")");
}

std::string EmitCFunc::dereferenceString(സ്ഥിരം std::string& pointer) സ്ഥിരം {
    എങ്കിൽ (pointer[0] == '(' && pointer[1] == '&') {
        // remove "address of" followed by immediate dereference
        // Note: this relies on only the form '(&OBJECT)' being used by Verilator
        തിരികെ pointer.substr(2, pointer.length() - 3) + '.';
    } അല്ലെങ്കിൽ {
        എങ്കിൽ (pointer == "vlSelf" && m_usevlSelfRef) {
            തിരികെ "vlSelfRef.";
        } അല്ലെങ്കിൽ {
            തിരികെ pointer + "->";
        }
    }
}
ശൂന്യം EmitCFunc::emitDereference(Ast_കണ്ണി* nodep, സ്ഥിരം string& pointer) {
    putns(nodep, dereferenceString(pointer));
}

ശൂന്യം EmitCFunc::emitCvtPackStr(Ast_കണ്ണി* nodep) {
    എങ്കിൽ (സ്ഥിരം AstConst* സ്ഥിരം constp = VN_CAST(nodep, Const)) {
        emitConstantString(constp);
    } അല്ലെങ്കിൽ എങ്കിൽ (VN_IS(nodep->dtypep(), StreamDType)) {
        putnbs(nodep, "VL_CVT_PACK_STR_ND(");
        iterateAndNextConstNull(nodep);
        puts(")");
    } അല്ലെങ്കിൽ {
        putnbs(nodep, "VL_CVT_PACK_STR_N");
        emitIQW(nodep);
        puts("(");
        എങ്കിൽ (nodep->isWide()) {
            // Note argument width, not node width (which is always 32)
            puts(cvtToStr(nodep->width_വാക്കുകൾ()));
            puts(", ");
        }
        iterateAndNextConstNull(nodep);
        puts(")");
    }
}

ശൂന്യം EmitCFunc::emitConstant(AstConst* nodep) {
    // Put out constant set to the specified variable, or given variable in a string
    സ്ഥിരം V3_സംഖ്യ& അക്കം = nodep->അക്കം();
    എങ്കിൽ (അക്കം.isFourState()) {
        nodep->v3warn(E_UNSUPPORTED, "Unsupported: 4-state numbers in this context");
        തിരികെ;
    }
    putns(nodep, അക്കം.emitC());
}

ശൂന്യം EmitCFunc::emitConstantString(സ്ഥിരം AstConst* nodep) {
    // Const might be a Verilog array-type string, but need to always output std::string
    putnbs(nodep, "std::string{");
    സ്ഥിരം string str = nodep->അക്കം().toString();
    എങ്കിൽ (!str.empty()) putsQuoted(str);
    puts("}");
}

ശൂന്യം EmitCFunc::emitVarReset(സ്ഥിരം string& prefix, AstVar* varp, തർക്കം constructing) {
    // 'constructing' indicates that the object was just constructed, so if it is a string or
    // something that starts off clear already, no need to clear it again
    Ast_കണ്ണി_DType* സ്ഥിരം dtypep = varp->dtypep()->skipRefp();
    സ്ഥിരം string vlSelf = VSelfPointerText::replaceThis(m_useSelfForThis, "this->");
    സ്ഥിരം string varNameProtected
        = ((VN_IS(m_modp, Class) || varp->isFuncLocal()) || !prefix.empty())
              ? varName(varp)
              : vlSelf + varName(varp);
    സ്ഥിരം string newPrefix = prefix + varNameProtected;
    എങ്കിൽ (varp->isIO() && m_modp->is_മുകൾ() && optSystemC()) {
        // System C top I/O doesn't need loading, as the lower level subinst code does it.}
    } അല്ലെങ്കിൽ എങ്കിൽ (varp->isParam()) {
        UASSERT_OBJ(varp->valuep(), varp, "No init for a param?");
        // If a simple CONST value we initialize it using an enum
        // If an ARRAYINIT we initialize it using an initial block similar to a signal
        // puts("// parameter "+varp->nameProtect()+" = "+varp->valuep()->name()+"\n");
    } അല്ലെങ്കിൽ {
        putns(varp,
              emitVarResetRecurse(varp, constructing, newPrefix, dtypep, 0, "", varp->valuep()));
    }
}

string EmitCFunc::emitVarResetRecurse(സ്ഥിരം AstVar* varp, തർക്കം constructing,
                                      സ്ഥിരം string& varNameProtected, Ast_കണ്ണി_DType* dtypep,
                                      പൂർണ്ണസംഖ്യ depth, സ്ഥിരം string& suffix, സ്ഥിരം Ast_കണ്ണി* valuep) {
    dtypep = dtypep->skipRefp();
    AstBasicDType* സ്ഥിരം basicp = dtypep->basicp();
    // Returns string to do resetting, empty to do nothing (which caller should handle)
    എങ്കിൽ (AstAssocArrayDType* സ്ഥിരം adtypep = VN_CAST(dtypep, AssocArrayDType)) {
        // Access std::array as C array
        സ്ഥിരം string pre = constructing ? "" : varNameProtected + suffix + ".clear();\n";
        തിരികെ pre
               + emitVarResetRecurse(varp, constructing, varNameProtected, adtypep->subDTypep(),
                                     depth + 1, suffix + ".atDefault()", nullptr);
    } അല്ലെങ്കിൽ എങ്കിൽ (AstWildcardArrayDType* സ്ഥിരം adtypep = VN_CAST(dtypep, WildcardArrayDType)) {
        // Access std::array as C array
        സ്ഥിരം string pre = constructing ? "" : varNameProtected + suffix + ".clear();\n";
        തിരികെ pre
               + emitVarResetRecurse(varp, constructing, varNameProtected, adtypep->subDTypep(),
                                     depth + 1, suffix + ".atDefault()", nullptr);
    } അല്ലെങ്കിൽ എങ്കിൽ (VN_IS(dtypep, CDType)) {
        തിരികെ "";  // Constructor does it
    } അല്ലെങ്കിൽ എങ്കിൽ (VN_IS(dtypep, CoverCrossDType) || VN_IS(dtypep, CoverpointDType)) {
        തിരികെ "";  // Covergroup constructor creates the runtime and assigns the pointer
    } അല്ലെങ്കിൽ എങ്കിൽ (സ്ഥിരം AstClassRefDType* സ്ഥിരം adtypep = VN_CAST(dtypep, ClassRefDType)) {
        തിരികെ adtypep->rawPointer() ? varNameProtected + suffix + " = nullptr;\n" : "";
    } അല്ലെങ്കിൽ എങ്കിൽ (VN_IS(dtypep, IfaceRefDType)) {
        തിരികെ varNameProtected + suffix + " = nullptr;\n";
    } അല്ലെങ്കിൽ എങ്കിൽ (സ്ഥിരം AstDynArrayDType* സ്ഥിരം adtypep = VN_CAST(dtypep, DynArrayDType)) {
        സ്ഥിരം string pre = constructing ? "" : varNameProtected + suffix + ".clear();\n";
        തിരികെ pre
               + emitVarResetRecurse(varp, constructing, varNameProtected, adtypep->subDTypep(),
                                     depth + 1, suffix + ".atDefault()", nullptr);
    } അല്ലെങ്കിൽ എങ്കിൽ (സ്ഥിരം AstQueueDType* സ്ഥിരം adtypep = VN_CAST(dtypep, QueueDType)) {
        സ്ഥിരം string pre = constructing ? "" : varNameProtected + suffix + ".clear();\n";
        തിരികെ pre
               + emitVarResetRecurse(varp, constructing, varNameProtected, adtypep->subDTypep(),
                                     depth + 1, suffix + ".atDefault()", nullptr);
    } അല്ലെങ്കിൽ എങ്കിൽ (VN_IS(dtypep, SampleQueueDType)) {
        തിരികെ "";
    } അല്ലെങ്കിൽ എങ്കിൽ (സ്ഥിരം AstUnpackArrayDType* സ്ഥിരം adtypep = VN_CAST(dtypep, UnpackArrayDType)) {
        UASSERT_OBJ(adtypep->hi() >= adtypep->lo(), varp,
                    "Should have swapped msb & lsb earlier.");
        സ്ഥിരം Ast_കണ്ണി_DType* സ്ഥിരം elementp = adtypep->subDTypep()->skipRefp();
        എങ്കിൽ (!varp->isFuncLocal() && elementp->isIntegralOrPacked()) {
            // Scoped reset has no array-index input or mutable RNG state. Initialize one
            // element, then fill in the optimized runtime even when this code is cold.
            സ്ഥിരം string first
                = emitVarResetRecurse(varp, constructing, varNameProtected, adtypep->subDTypep(),
                                      depth + 1, suffix + "[0]", nullptr);
            സ്ഥിരം string array = varNameProtected + suffix;
            തിരികെ first + "VL_UNPACKED_FILL(" + array + ".data(), sizeof(" + array + "[0]), "
                   + cvtToStr(adtypep->elementsConst()) + ");\n";
        }
        സ്ഥിരം string ivar = "__Vi"s + cvtToStr(depth);
        സ്ഥിരം string pre = ("for (int " + ivar + " = " + cvtToStr(0) + "; " + ivar + " < "
                            + cvtToStr(adtypep->elementsConst()) + "; ++" + ivar + ") {\n");
        സ്ഥിരം string below
            = emitVarResetRecurse(varp, constructing, varNameProtected, adtypep->subDTypep(),
                                  depth + 1, suffix + "[" + ivar + "]", nullptr);
        സ്ഥിരം string post = "}\n";
        തിരികെ below.empty() ? "" : pre + below + post;
    } അല്ലെങ്കിൽ എങ്കിൽ (VN_IS(dtypep, കണ്ണി_UOr_Struct_DType) && !VN_AS(dtypep, കണ്ണി_UOr_Struct_DType)->packed()) {
        സ്ഥിരം Ast_കണ്ണി_UOr_Struct_DType* സ്ഥിരം sdtypep = VN_AS(dtypep, കണ്ണി_UOr_Struct_DType);
        string literal;
        വേണ്ടി (സ്ഥിരം AstMemberDType* itemp = sdtypep->membersp(); itemp;
             itemp = VN_AS(itemp->nextp(), MemberDType)) {
            സ്ഥിരം std::string വരി = emitVarResetRecurse(
                varp, constructing, varNameProtected + suffix + "." + itemp->nameProtect(),
                itemp->dtypep(), depth + 1, "", itemp->valuep());
            എങ്കിൽ (!വരി.empty()) literal += വരി;
        }
        തിരികെ literal;
    } അല്ലെങ്കിൽ എങ്കിൽ (basicp && basicp->keyword() == VBasicDTypeKwd::STRING) {
        എങ്കിൽ (constructing) തിരികെ "";  // String's constructor deals with it
        തിരികെ varNameProtected + suffix + ".clear();\n";
    } അല്ലെങ്കിൽ എങ്കിൽ (basicp && basicp->isForkSync()) {
        തിരികെ "";
    } അല്ലെങ്കിൽ എങ്കിൽ (basicp && basicp->isProcessRef()) {
        തിരികെ "";
    } അല്ലെങ്കിൽ എങ്കിൽ (basicp && basicp->isDelayScheduler()) {
        തിരികെ "";
    } അല്ലെങ്കിൽ എങ്കിൽ (basicp && basicp->isTriggerScheduler()) {
        തിരികെ "";
    } അല്ലെങ്കിൽ എങ്കിൽ (basicp && basicp->isDynamicTriggerScheduler()) {
        തിരികെ "";
    } അല്ലെങ്കിൽ എങ്കിൽ (basicp && (basicp->isRandomGenerator() || basicp->isStdRandomGenerator())) {
        തിരികെ "";
    } അല്ലെങ്കിൽ എങ്കിൽ (basicp && basicp->isCovergroupInstHandle()) {
        // The handle's own constructor deals with it.
        തിരികെ "";
    } അല്ലെങ്കിൽ എങ്കിൽ (basicp && (basicp->isEvent())) {
        തിരികെ "VlAssignableEvent{};\n";
    } അല്ലെങ്കിൽ എങ്കിൽ (basicp) {
        സ്ഥിരം തർക്കം zeroit
            = (varp->attrFileDescr()  // Zero so we don't do file IO if never $fopen
               || varp->isFuncLocal()  // Randomization too slow
               || (basicp && basicp->isZeroInit())
               || (v3Global.opt.underlineZero() && !varp->name().empty() && varp->name()[0] == '_')
               || (varp->varType().isTemp() && !varp->isXTemp())
               || (varp->isXTemp()
                       ? (v3Global.opt.xAssign() != "unique")
                       : (v3Global.opt.xInitial() == "fast" || v3Global.opt.xInitial() == "0")));
        സ്ഥിരം തർക്കം slow = !varp->isFuncLocal() && !varp->isClassMember();
        splitSizeInc(1);
        എങ്കിൽ (dtypep->isWide()) {  // Handle unpacked; not basicp->isWide
            string out;
            എങ്കിൽ (valuep) {
                സ്ഥിരം AstConst* സ്ഥിരം constp = VN_AS(valuep, Const);
                UASSERT_OBJ(constp, varp, "non-const initializer for variable");
                വേണ്ടി (പൂർണ്ണസംഖ്യ w = 0; w < dtypep->width_വാക്കുകൾ(); ++w) {
                    out += varNameProtected + suffix + "[" + cvtToStr(w) + "] = ";
                    out += cvtToStr(constp->അക്കം().edataWord(w)) + "U;\n";
                }
            } അല്ലെങ്കിൽ {
                out += zeroit ? (slow ? "VL_ZERO_RESET_W(" : "VL_ZERO_W(")
                              : (varp->isXTemp() ? "VL_SCOPED_RAND_RESET_ASSIGN_W("
                                                 : "VL_SCOPED_RAND_RESET_W(");
                out += cvtToStr(dtypep->widthMin());
                out += ", " + varNameProtected + suffix;
                എങ്കിൽ (!zeroit) {
                    emitVarResetScopeHash();
                    സ്ഥിരം uint64_t salt = VString::hashMurmur(varp->prettyName());
                    out += ", ";
                    out += m_classOrPackage ? m_classOrPackageHash : "__VscopeHash";
                    out += ", ";
                    out += std::to_string(salt);
                    out += "ull";
                }
                out += ");\n";
            }
            തിരികെ out;
        } അല്ലെങ്കിൽ {
            string out = varNameProtected + suffix;
            എങ്കിൽ (valuep) {
                out += " = ";
                // TODO cleanup code shared between here, V3EmitCConstInit.h,
                // EmitCFunc::emitVarReset, EmitCFunc::emitConstant
                സ്ഥിരം AstConst* സ്ഥിരം constp = VN_AS(valuep, Const);
                UASSERT_OBJ(constp, varp, "non-const initializer for variable");
                out += cvtToStr(constp->അക്കം().edataWord(0)) + "U;\n";
                out += ";\n";
            } അല്ലെങ്കിൽ എങ്കിൽ (zeroit) {
                out += " = 0;\n";
            } അല്ലെങ്കിൽ {
                emitVarResetScopeHash();
                സ്ഥിരം uint64_t salt = VString::hashMurmur(varp->prettyName());
                out += " = VL_SCOPED_RAND_RESET_";
                എങ്കിൽ (varp->isXTemp()) out += "ASSIGN_";
                out += dtypep->charIQWN();
                out += "(" + cvtToStr(dtypep->widthMin()) + ", "
                       + (m_classOrPackage ? m_classOrPackageHash : "__VscopeHash") + ", "
                       + std::to_string(salt) + "ull);\n";
            }
            തിരികെ out;
        }
    } അല്ലെങ്കിൽ {  // LCOV_EXCL_BR_LINE
        v3fatalSrc("Unknown node type in reset generator: " << varp->prettyTypeName());
    }
    തിരികെ "";
}

ശൂന്യം EmitCFunc::emitVarResetScopeHash() {
    എങ്കിൽ (VL_LIKELY(m_createdScopeHash)) { തിരികെ; }
    എങ്കിൽ (m_classOrPackage) {
        m_classOrPackageHash
            = std::to_string(VString::hashMurmur(m_classOrPackage->name())) + "ULL";
    } അല്ലെങ്കിൽ {
        puts(string("const uint64_t __VscopeHash = VL_MURMUR64_HASH(")
             + (m_useSelfForThis ? "vlSelf" : "this") + "->vlNamep);\n");
    }
    m_createdScopeHash = സത്യം;
}
