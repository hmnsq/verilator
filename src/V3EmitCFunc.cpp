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

#ଅନ୍ତର୍ଭାଵ "V3PchAstMT.h"

#ଅନ୍ତର୍ଭାଵ "V3EmitCFunc.h"

#ଅନ୍ତର୍ଭାଵ <map>
#ଅନ୍ତର୍ଭାଵ <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// EmitCFunc

ତାର୍କିକ EmitCFunc::emitSimpleOk(Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* nodep) {
    // Can we put out a simple (A + B) instead of VL_ADD_III(A,B)?
    ଯଦି (nodep->emitSimpleOperator() == "") ଫେରନ୍ତୁ ମିଥ୍ୟା;
    ଯଦି (nodep->isWide()) ଫେରନ୍ତୁ ମିଥ୍ୟା;
    ଯଦି (nodep->op1p() && nodep->op1p()->isWide()) ଫେରନ୍ତୁ ମିଥ୍ୟା;
    ଯଦି (nodep->op2p() && nodep->op2p()->isWide()) ଫେରନ୍ତୁ ମିଥ୍ୟା;
    ଯଦି (nodep->op3p() && nodep->op3p()->isWide()) ଫେରନ୍ତୁ ମିଥ୍ୟା;
    ଫେରନ୍ତୁ ସତ୍ୟ;
}

ଶୂନ୍ୟ EmitCFunc::emitOpName(Ast_ଗଣ୍ଠି* nodep, ଅଚଳ string& format, Ast_ଗଣ୍ଠି* lhsp, Ast_ଗଣ୍ଠି* rhsp,
                           Ast_ଗଣ୍ଠି* thsp) {
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

    ତାର୍କିକ needComma = ମିଥ୍ୟା;
    ତାର୍କିକ usesQueue = ମିଥ୍ୟା;
    string nextComma;
    auto commaOut = [&out, &nextComma]() {
        ଯଦି (!nextComma.empty()) {
            out += nextComma;
            nextComma = "";
        }
    };

    auto putOut = [ଏହି, &out]() {
        ଯଦି (!out.empty()) puts(out);
        out = "";
    };

    ପାଇଁ (string::const_iterator pos = format.begin(); pos != format.end(); ++pos) {
        ଯଦି (pos[0] == ',') {
            // Remember we need to add one, but don't do yet to avoid ",)"
            ଯଦି (needComma) {
                ଯଦି (pos[1] == ' ') {
                    nextComma = ", ";
                } ଅନ୍ୟଥା {
                    nextComma = ",";
                }
                needComma = ମିଥ୍ୟା;
            }
            ଯଦି (pos[1] == ' ') ++pos;  // Must do even if no nextComma
        } ଅନ୍ୟଥା ଯଦି (pos[0] == '%') {
            ++pos;
            ତାର୍କିକ detail = ମିଥ୍ୟା;
            Ast_ଗଣ୍ଠି* detailp = nullptr;
            ବାଛ (pos[0]) {
            ସ୍ଥିତି '%': out += '%'; ଭାଙ୍ଗ;
            ସ୍ଥିତି 'k':
                putOut();
                putbs("");
                ଭାଙ୍ଗ;
            ସ୍ଥିତି 'n':
                detail = ସତ୍ୟ;
                detailp = nodep;
                ଭାଙ୍ଗ;
            ସ୍ଥିତି 'l':
                detail = ସତ୍ୟ;
                detailp = lhsp;
                ଭାଙ୍ଗ;
            ସ୍ଥିତି 'r':
                detail = ସତ୍ୟ;
                detailp = rhsp;
                ଭାଙ୍ଗ;
            ସ୍ଥିତି 't':
                detail = ସତ୍ୟ;
                detailp = thsp;
                ଭାଙ୍ଗ;
            ସ୍ଥିତି 'P':
                ଯଦି (nodep->isWide() && !usesQueue) {
                    UASSERT_OBJ(m_wideTempRefp, nodep,
                                "Wide op " << nodep->prettyTypeName()
                                           << " w/ no temp, perhaps missing op in V3EmitC?");
                    commaOut();
                    putOut();
                    ଯଦି (!m_wideTempRefp->selfPointer().isEmpty()) {
                        emitDereference(m_wideTempRefp,
                                        m_wideTempRefp->selfPointerProtect(m_useSelfForThis));
                    }
                    out += varName(m_wideTempRefp->varp());
                    m_wideTempRefp = nullptr;
                    needComma = ସତ୍ୟ;
                } ଅନ୍ୟଥା ଯଦି (usesQueue) {
                    commaOut();
                    putOut();
                    iterateAndNextConstNull(nodep->backp()->op2p());
                    needComma = ସତ୍ୟ;
                }

                ଭାଙ୍ଗ;
            ପୂର୍ବନିର୍ଦ୍ଧାରିତ: nodep->v3fatalSrc("Unknown emitOperator format code: %" << pos[0]); ଭାଙ୍ଗ;
            }
            ଯଦି (detail) {
                // Get next letter of %[nlrt]
                ++pos;
                ବାଛ (pos[0]) {
                ସ୍ଥିତି 'q':
                    putOut();
                    // If we are assigning this to a queue we need to get the return type
                    ଯଦି (VN_IS(detailp->backp(), Assign)
                        && VN_IS(detailp->backp()->op2p()->dtypep()->skipRefp(), QueueDType)) {
                        puts("R");  // R for queue
                        usesQueue = ସତ୍ୟ;
                    } ଅନ୍ୟଥା ଯଦି (VN_IS(detailp->dtypep()->skipRefp(), QueueDType)
                               || VN_IS(detailp->dtypep()->skipRefp(), StreamDType)) {
                        puts("R");  // R for queue
                    } ଅନ୍ୟଥା {
                        emitIQW(detailp);
                    }
                    ଭାଙ୍ଗ;
                ସ୍ଥିତି 'w':
                    commaOut();
                    out += cvtToStr(detailp->widthMin());
                    needComma = ସତ୍ୟ;
                    ଭାଙ୍ଗ;
                ସ୍ଥିତି 'W':
                    ଯଦି (lhsp->isWide()) {
                        commaOut();
                        out += cvtToStr(lhsp->width_ଶବ୍ଦଗୁଡ଼ିକ());
                        needComma = ସତ୍ୟ;
                    } ଅନ୍ୟଥା ଯଦି (VN_IS(lhsp, StreamR)) {
                        commaOut();
                        out += cvtToStr(rhsp->width_ଶବ୍ଦଗୁଡ଼ିକ());
                        needComma = ସତ୍ୟ;
                    }
                    ଭାଙ୍ଗ;
                ସ୍ଥିତି 'i':
                    commaOut();
                    UASSERT_OBJ(detailp, nodep, "emitOperator() references undef node");
                    putOut();
                    iterateAndNextConstNull(detailp);
                    needComma = ସତ୍ୟ;
                    ଭାଙ୍ଗ;
                ପୂର୍ବନିର୍ଦ୍ଧାରିତ:
                    nodep->v3fatalSrc("Unknown emitOperator format code: %[nlrt]" << pos[0]);
                    ଭାଙ୍ଗ;
                }
            }
        } ଅନ୍ୟଥା ଯଦି (pos[0] == ')') {
            nextComma = "";
            out += ')';
        } ଅନ୍ୟଥା ଯଦି (pos[0] == '(') {
            commaOut();
            needComma = ମିଥ୍ୟା;
            out += '(';
        } ଅନ୍ୟଥା {
            // Normal text
            ଯଦି (std::isalnum(pos[0])) needComma = ସତ୍ୟ;
            commaOut();
            out += pos[0];
        }
    }
    putOut();
}

ତାର୍କିକ EmitCFunc::displayEmitHeader(Ast_ଗଣ୍ଠି* nodep) {
    ତାର୍କିକ isStmt = ମିଥ୍ୟା;
    ଯଦି (ଅଚଳ AstFScanF* ଅଚଳ dispp = VN_CAST(nodep, FScanF)) {
        isStmt = ମିଥ୍ୟା;
        putns(nodep, "VL_FSCANF_INX(");
        iterateConst(dispp->filep());
        puts(",");
    } ଅନ୍ୟଥା ଯଦି (ଅଚଳ AstSScanF* ଅଚଳ dispp = VN_CAST(nodep, SScanF)) {
        isStmt = ମିଥ୍ୟା;
        putns(nodep, "VL_SSCANF_I");
        emitIQW(dispp->fromp());
        puts("NX(");
        puts(cvtToStr(dispp->fromp()->widthMin()));
        puts(",");
        iterateConst(dispp->fromp());
        puts(",");
    } ଅନ୍ୟଥା ଯଦି (ଅଚଳ AstDisplay* ଅଚଳ dispp = VN_CAST(nodep, Display)) {
        isStmt = ସତ୍ୟ;
        ଯଦି (dispp->filep()) {
            putns(nodep, "VL_FWRITEF_NX(");
            iterateConst(dispp->filep());
            puts(",");
        } ଅନ୍ୟଥା {
            putns(nodep, "VL_WRITEF_NX(");
        }
    } ଅନ୍ୟଥା ଯଦି (ଅଚଳ AstSFormat* ଅଚଳ dispp = VN_CAST(nodep, SFormat)) {
        isStmt = ସତ୍ୟ;
        puts("VL_SFORMAT_NX(");
        ଯଦି (!dispp->lhsp()->dtypep()->isString()) {
            puts(cvtToStr(dispp->lhsp()->widthMin()));
            putbs(",");
        }
        iterateConst(dispp->lhsp());
        emitDatap(dispp->lhsp());
        putbs(",");
    } ଅନ୍ୟଥା ଯଦି (VN_IS(nodep, SFormatF)) {
        isStmt = ମିଥ୍ୟା;
        putns(nodep, "VL_SFORMATF_N_NX(");
    } ଅନ୍ୟଥା {
        nodep->v3fatalSrc("Unknown displayEmit node type");
    }
    ଫେରନ୍ତୁ isStmt;
}

ଶୂନ୍ୟ EmitCFunc::display_ଗଣ୍ଠି(Ast_ଗଣ୍ଠି* nodep, AstSFormatF* fmtp,  // fmtp is nullptr for AstScan
                            ଅଚଳ string& vformat, Ast_ଗଣ୍ଠି* exprsp, ତାର୍କିକ isScan) {
    // Check format, if it exists
    ଅଚଳ ତାର୍କିକ ପଦସମଷ୍ଟି_Format_2 = fmtp && fmtp->ପଦସମଷ୍ଟି_Format_2();
    ତାର୍କିକ needsScope = ପଦସମଷ୍ଟି_Format_2;
    ତାର୍କିକ needsTimescale = ପଦସମଷ୍ଟି_Format_2;
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା formatArgs = 0;
    ଯଦି (!ପଦସମଷ୍ଟି_Format_2) {
        // Check display and argument count to head-off later runtime issues
        ତାର୍କିକ inPct = ମିଥ୍ୟା;
        ତାର୍କିକ ignore = ମିଥ୍ୟା;
        ପାଇଁ (ଅକ୍ଷର ch : vformat) {
            // UINFO(1, "Parse '" << *pos << "'  IP" << inPct << " List " << cvtToHex(elistp));
            ଯଦି (!inPct && ch == '%') {
                inPct = ସତ୍ୟ;
                ignore = ମିଥ୍ୟା;
            } ଅନ୍ୟଥା ଯଦି (inPct && (std::isdigit(ch) || ch == '.' || ch == '-')) {
            } ଅନ୍ୟଥା ଯଦି (!inPct) {  // Normal text
            } ଅନ୍ୟଥା {  // Format character
                inPct = ମିଥ୍ୟା;
                ch = std::tolower(ch);
                ବାଛ (ch) {
                ସ୍ଥିତି '%': ଭାଙ୍ଗ;
                ସ୍ଥିତି '*':
                    inPct = ସତ୍ୟ;  // Get more digits
                    ignore = ସତ୍ୟ;
                    ଭାଙ୍ଗ;
                ସ୍ଥିତି 'm': needsScope = ସତ୍ୟ; ଭାଙ୍ଗ;
                ସ୍ଥିତି 't':
                    needsTimescale = ସତ୍ୟ;
                    ++formatArgs;
                    ଭାଙ୍ଗ;
                ପୂର୍ବନିର୍ଦ୍ଧାରିତ:
                    ଯଦି (!ignore && V3_ସଂଖ୍ୟା::displayedFmtHasArg(ch, isScan)) ++formatArgs;
                    ଭାଙ୍ଗ;
                }
            }
        }
    }

    ଯଦି (vformat.empty() && VN_IS(nodep, Display))  // not fscanf etc, as they need to return value
        ଫେରନ୍ତୁ;  // NOP

    ଅଚଳ ତାର୍କିକ isStmt = displayEmitHeader(nodep);

    ଯଦି (ପଦସମଷ୍ଟି_Format_2) {
        UASSERT_OBJ(exprsp, nodep, "Missing format expression");
        iterateConst(exprsp);
        emitDatap(exprsp);
        exprsp = exprsp->nextp();
    } ଅନ୍ୟଥା {
        ofp()->putsQuoted(vformat);
    }

    ପୂର୍ଣ୍ଣସଂଖ୍ୟା ପଦସମଷ୍ଟି_Args = 0;
    ପାଇଁ (Ast_ଗଣ୍ଠି* argp = exprsp; argp; argp = argp->nextp()) ++ପଦସମଷ୍ଟି_Args;
    ଯଦି (VL_UNCOVERABLE(!ପଦସମଷ୍ଟି_Format_2 && ପଦସମଷ୍ଟି_Args != formatArgs)) {  // LCOV_EXCL_START
        // expectDisplay() checks this first, so probably internal error if found here
        nodep->v3error("Internal: Missing or extra arguments for $display-like format, "
                       << " expression had " << ପଦସମଷ୍ଟି_Args << ", format had " << formatArgs);
    }  // LCOV_EXCL_STOP

    // argc for error check. Also MSVC++ requires va_args to not be off a reference
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା argc = 0;
    ଯଦି (needsScope) ++argc;
    ଯଦି (needsTimescale) ++argc;
    ପାଇଁ (Ast_ଗଣ୍ଠି* argp = exprsp; argp; argp = argp->nextp()) {
        ++argc;
        ଅଚଳ AstSFormatArg* ଅଚଳ fargp = VN_CAST(argp, SFormatArg);
        ଯଦି (fargp && fargp->formatAttr().isEnum()) ++argc;  // Additional name argument
    }
    ofp()->puts("," + std::to_string(argc));

    ଯଦି (needsScope) {
        AstScopeName* ଅଚଳ scopenamep = fmtp ? fmtp->scopeNamep() : nullptr;
        UASSERT_OBJ(scopenamep, nodep, "Display with %m but no AstScopeName");
        ofp()->puts(", '"s + VFormatAttr{VFormatAttr::SCOPE}.ascii() + "'");
        ଯଦି (scopenamep->selfRelative()) {
            // vlNamep is catName(name(), instance), so starts with '.' when the model is unnamed
            ଅଚଳ string namep = string{m_useSelfForThis ? "vlSelf" : "this"} + "->vlNamep";
            ofp()->puts(",(vlSymsp->name()[0] ? " + namep + " : " + namep + " + 1),");
            ofp()->putsQuoted(scopenamep->scopeAttr());
        } ଅନ୍ୟଥା {
            ofp()->puts(",vlSymsp->name(),");
            ofp()->putsQuoted(scopenamep->scopePrettySymName());
        }
    }

    ଯଦି (needsTimescale) {
        VTimescale timeunit = VTimescale::NONE;
        ଯଦି (ଅଚଳ AstDisplay* ଅଚଳ anodep = VN_CAST(nodep, Display)) {
            timeunit = anodep->fmtp()->timeunit();
        } ଅନ୍ୟଥା ଯଦି (ଅଚଳ AstSFormat* ଅଚଳ anodep = VN_CAST(nodep, SFormat)) {
            timeunit = anodep->fmtp()->timeunit();
        } ଅନ୍ୟଥା ଯଦି (ଅଚଳ AstSScanF* ଅଚଳ anodep = VN_CAST(nodep, SScanF)) {
            timeunit = anodep->timeunit();
        } ଅନ୍ୟଥା ଯଦି (ଅଚଳ AstSFormatF* ଅଚଳ anodep = VN_CAST(nodep, SFormatF)) {
            timeunit = anodep->timeunit();
        }
        UASSERT_OBJ(!timeunit.isNone(), nodep,
                    "Use of %t must be under AstDisplay, AstSFormat, or AstSFormatF, or "
                    "SScanF, and timeunit set");
        ofp()->puts(", '"s + VFormatAttr{VFormatAttr::TIMEUNIT}.ascii() + "'");
        ofp()->puts(","s + cvtToStr((ପୂର୍ଣ୍ଣସଂଖ୍ୟା)timeunit.powerOfTen()));
    }

    // Arguments
    ପାଇଁ (Ast_ଗଣ୍ଠି* argp = exprsp; argp; argp = argp->nextp()) {
        ofp()->indentInc();
        ofp()->putbs("");
        AstSFormatArg* ଅଚଳ fargp = VN_CAST(argp, SFormatArg);
        Ast_ଗଣ୍ଠି* ଅଚଳ subargp = fargp ? fargp->exprp() : argp;
        ଅଚଳ VFormatAttr formatAttr = AstSFormatArg::formatAttrDefauled(fargp, subargp->dtypep());
        puts(", '"s + formatAttr.ascii() + '\'');
        ଯଦି (formatAttr.isSigned() || formatAttr.isUnsigned() || formatAttr.isEnum())
            puts("," + cvtToStr(subargp->widthMin()));
        ଅଚଳ ତାର୍କିକ addrof = isScan || formatAttr.isString() || formatAttr.isComplex();
        ଅଚଳ ତାର୍କିକ wideEnum = formatAttr.isEnum() && subargp->isWide();
        puts(",");
        ଯଦି (wideEnum) puts("static_cast<const EData*>(");
        ଯଦି (addrof) puts("&(");
        ଯଦି (VN_IS(subargp, StreamR))
            emitStreamR(
                VN_CAST(subargp, StreamR),
                nodep);  // This has to be done here because streamR doesn't know what it returns
        ଅନ୍ୟଥା { iterateConst(subargp); }
        ଯଦି (addrof) puts(")");
        ଯଦି (!addrof) emitDatap(argp);
        ଯଦି (wideEnum) puts(")");
        ଯଦି (formatAttr.isEnum()) {
            puts(", '"s + VFormatAttr{VFormatAttr::STRING}.ascii() + "', &(");
            iterateConst(fargp->namep());
            puts(")");
        }
        ofp()->indentDec();
    }

    // End
    ଯଦି (isStmt) {
        puts(");\n");
    } ଅନ୍ୟଥା {
        puts(") ");
    }
}

ଶୂନ୍ୟ EmitCFunc::emitCCallArgs(ଅଚଳ Ast_ଗଣ୍ଠି_CCall* nodep, ଅଚଳ string& selfPointer,
                              ତାର୍କିକ inProcess) {
    putns(nodep, "(");
    ତାର୍କିକ comma = ମିଥ୍ୟା;
    ଯଦି (nodep->funcp()->isLoose() && !nodep->funcp()->isStatic()) {
        UASSERT_OBJ(!selfPointer.empty(), nodep, "Call to loose method without self pointer");
        puts(selfPointer);
        comma = ସତ୍ୟ;
    }
    ଯଦି (nodep->funcp()->needProcess()) {
        ଯଦି (comma) puts(", ");
        ଯଦି (VN_IS(nodep->backp(), CAwait) || !nodep->funcp()->isCoroutine()) {
            puts("vlProcess");
        } ଅନ୍ୟଥା ଯଦି (inProcess) {
            puts("std::make_shared<VlProcess>(vlProcess)");
        } ଅନ୍ୟଥା {
            puts("std::make_shared<VlProcess>()");
        }
        comma = ସତ୍ୟ;
    }
    ଯଦି (!nodep->argTypes().empty()) {
        ଯଦି (comma) puts(", ");
        puts(nodep->argTypes());
        comma = ସତ୍ୟ;
    }
    putCommaIterateNext(nodep->argsp(), comma);
    puts(")");
}

std::string EmitCFunc::dereferenceString(ଅଚଳ std::string& pointer) ଅଚଳ {
    ଯଦି (pointer[0] == '(' && pointer[1] == '&') {
        // remove "address of" followed by immediate dereference
        // Note: this relies on only the form '(&OBJECT)' being used by Verilator
        ଫେରନ୍ତୁ pointer.substr(2, pointer.length() - 3) + '.';
    } ଅନ୍ୟଥା {
        ଯଦି (pointer == "vlSelf" && m_usevlSelfRef) {
            ଫେରନ୍ତୁ "vlSelfRef.";
        } ଅନ୍ୟଥା {
            ଫେରନ୍ତୁ pointer + "->";
        }
    }
}
ଶୂନ୍ୟ EmitCFunc::emitDereference(Ast_ଗଣ୍ଠି* nodep, ଅଚଳ string& pointer) {
    putns(nodep, dereferenceString(pointer));
}

ଶୂନ୍ୟ EmitCFunc::emitCvtPackStr(Ast_ଗଣ୍ଠି* nodep) {
    ଯଦି (ଅଚଳ AstConst* ଅଚଳ constp = VN_CAST(nodep, Const)) {
        emitConstantString(constp);
    } ଅନ୍ୟଥା ଯଦି (VN_IS(nodep->dtypep(), StreamDType)) {
        putnbs(nodep, "VL_CVT_PACK_STR_ND(");
        iterateAndNextConstNull(nodep);
        puts(")");
    } ଅନ୍ୟଥା {
        putnbs(nodep, "VL_CVT_PACK_STR_N");
        emitIQW(nodep);
        puts("(");
        ଯଦି (nodep->isWide()) {
            // Note argument width, not node width (which is always 32)
            puts(cvtToStr(nodep->width_ଶବ୍ଦଗୁଡ଼ିକ()));
            puts(", ");
        }
        iterateAndNextConstNull(nodep);
        puts(")");
    }
}

ଶୂନ୍ୟ EmitCFunc::emitConstant(AstConst* nodep) {
    // Put out constant set to the specified variable, or given variable in a string
    ଅଚଳ V3_ସଂଖ୍ୟା& ଅଙ୍କ = nodep->ଅଙ୍କ();
    ଯଦି (ଅଙ୍କ.isFourState()) {
        nodep->v3warn(E_UNSUPPORTED, "Unsupported: 4-state numbers in this context");
        ଫେରନ୍ତୁ;
    }
    putns(nodep, ଅଙ୍କ.emitC());
}

ଶୂନ୍ୟ EmitCFunc::emitConstantString(ଅଚଳ AstConst* nodep) {
    // Const might be a Verilog array-type string, but need to always output std::string
    putnbs(nodep, "std::string{");
    ଅଚଳ string str = nodep->ଅଙ୍କ().toString();
    ଯଦି (!str.empty()) putsQuoted(str);
    puts("}");
}

ଶୂନ୍ୟ EmitCFunc::emitVarReset(ଅଚଳ string& prefix, AstVar* varp, ତାର୍କିକ constructing) {
    // 'constructing' indicates that the object was just constructed, so if it is a string or
    // something that starts off clear already, no need to clear it again
    Ast_ଗଣ୍ଠି_DType* ଅଚଳ dtypep = varp->dtypep()->skipRefp();
    ଅଚଳ string vlSelf = VSelfPointerText::replaceThis(m_useSelfForThis, "this->");
    ଅଚଳ string varNameProtected
        = ((VN_IS(m_modp, Class) || varp->isFuncLocal()) || !prefix.empty())
              ? varName(varp)
              : vlSelf + varName(varp);
    ଅଚଳ string newPrefix = prefix + varNameProtected;
    ଯଦି (varp->isIO() && m_modp->is_ଶୀର୍ଷ() && optSystemC()) {
        // System C top I/O doesn't need loading, as the lower level subinst code does it.}
    } ଅନ୍ୟଥା ଯଦି (varp->isParam()) {
        UASSERT_OBJ(varp->valuep(), varp, "No init for a param?");
        // If a simple CONST value we initialize it using an enum
        // If an ARRAYINIT we initialize it using an initial block similar to a signal
        // puts("// parameter "+varp->nameProtect()+" = "+varp->valuep()->name()+"\n");
    } ଅନ୍ୟଥା {
        putns(varp,
              emitVarResetRecurse(varp, constructing, newPrefix, dtypep, 0, "", varp->valuep()));
    }
}

string EmitCFunc::emitVarResetRecurse(ଅଚଳ AstVar* varp, ତାର୍କିକ constructing,
                                      ଅଚଳ string& varNameProtected, Ast_ଗଣ୍ଠି_DType* dtypep,
                                      ପୂର୍ଣ୍ଣସଂଖ୍ୟା depth, ଅଚଳ string& suffix, ଅଚଳ Ast_ଗଣ୍ଠି* valuep) {
    dtypep = dtypep->skipRefp();
    AstBasicDType* ଅଚଳ basicp = dtypep->basicp();
    // Returns string to do resetting, empty to do nothing (which caller should handle)
    ଯଦି (AstAssocArrayDType* ଅଚଳ adtypep = VN_CAST(dtypep, AssocArrayDType)) {
        // Access std::array as C array
        ଅଚଳ string pre = constructing ? "" : varNameProtected + suffix + ".clear();\n";
        ଫେରନ୍ତୁ pre
               + emitVarResetRecurse(varp, constructing, varNameProtected, adtypep->subDTypep(),
                                     depth + 1, suffix + ".atDefault()", nullptr);
    } ଅନ୍ୟଥା ଯଦି (AstWildcardArrayDType* ଅଚଳ adtypep = VN_CAST(dtypep, WildcardArrayDType)) {
        // Access std::array as C array
        ଅଚଳ string pre = constructing ? "" : varNameProtected + suffix + ".clear();\n";
        ଫେରନ୍ତୁ pre
               + emitVarResetRecurse(varp, constructing, varNameProtected, adtypep->subDTypep(),
                                     depth + 1, suffix + ".atDefault()", nullptr);
    } ଅନ୍ୟଥା ଯଦି (VN_IS(dtypep, CDType)) {
        ଫେରନ୍ତୁ "";  // Constructor does it
    } ଅନ୍ୟଥା ଯଦି (VN_IS(dtypep, CoverCrossDType) || VN_IS(dtypep, CoverpointDType)) {
        ଫେରନ୍ତୁ "";  // Covergroup constructor creates the runtime and assigns the pointer
    } ଅନ୍ୟଥା ଯଦି (ଅଚଳ AstClassRefDType* ଅଚଳ adtypep = VN_CAST(dtypep, ClassRefDType)) {
        ଫେରନ୍ତୁ adtypep->rawPointer() ? varNameProtected + suffix + " = nullptr;\n" : "";
    } ଅନ୍ୟଥା ଯଦି (VN_IS(dtypep, IfaceRefDType)) {
        ଫେରନ୍ତୁ varNameProtected + suffix + " = nullptr;\n";
    } ଅନ୍ୟଥା ଯଦି (ଅଚଳ AstDynArrayDType* ଅଚଳ adtypep = VN_CAST(dtypep, DynArrayDType)) {
        ଅଚଳ string pre = constructing ? "" : varNameProtected + suffix + ".clear();\n";
        ଫେରନ୍ତୁ pre
               + emitVarResetRecurse(varp, constructing, varNameProtected, adtypep->subDTypep(),
                                     depth + 1, suffix + ".atDefault()", nullptr);
    } ଅନ୍ୟଥା ଯଦି (ଅଚଳ AstQueueDType* ଅଚଳ adtypep = VN_CAST(dtypep, QueueDType)) {
        ଅଚଳ string pre = constructing ? "" : varNameProtected + suffix + ".clear();\n";
        ଫେରନ୍ତୁ pre
               + emitVarResetRecurse(varp, constructing, varNameProtected, adtypep->subDTypep(),
                                     depth + 1, suffix + ".atDefault()", nullptr);
    } ଅନ୍ୟଥା ଯଦି (VN_IS(dtypep, SampleQueueDType)) {
        ଫେରନ୍ତୁ "";
    } ଅନ୍ୟଥା ଯଦି (ଅଚଳ AstUnpackArrayDType* ଅଚଳ adtypep = VN_CAST(dtypep, UnpackArrayDType)) {
        UASSERT_OBJ(adtypep->hi() >= adtypep->lo(), varp,
                    "Should have swapped msb & lsb earlier.");
        ଅଚଳ Ast_ଗଣ୍ଠି_DType* ଅଚଳ elementp = adtypep->subDTypep()->skipRefp();
        ଯଦି (!varp->isFuncLocal() && elementp->isIntegralOrPacked()) {
            // Scoped reset has no array-index input or mutable RNG state. Initialize one
            // element, then fill in the optimized runtime even when this code is cold.
            ଅଚଳ string first
                = emitVarResetRecurse(varp, constructing, varNameProtected, adtypep->subDTypep(),
                                      depth + 1, suffix + "[0]", nullptr);
            ଅଚଳ string array = varNameProtected + suffix;
            ଫେରନ୍ତୁ first + "VL_UNPACKED_FILL(" + array + ".data(), sizeof(" + array + "[0]), "
                   + cvtToStr(adtypep->elementsConst()) + ");\n";
        }
        ଅଚଳ string ivar = "__Vi"s + cvtToStr(depth);
        ଅଚଳ string pre = ("for (int " + ivar + " = " + cvtToStr(0) + "; " + ivar + " < "
                            + cvtToStr(adtypep->elementsConst()) + "; ++" + ivar + ") {\n");
        ଅଚଳ string below
            = emitVarResetRecurse(varp, constructing, varNameProtected, adtypep->subDTypep(),
                                  depth + 1, suffix + "[" + ivar + "]", nullptr);
        ଅଚଳ string post = "}\n";
        ଫେରନ୍ତୁ below.empty() ? "" : pre + below + post;
    } ଅନ୍ୟଥା ଯଦି (VN_IS(dtypep, ଗଣ୍ଠି_UOr_Struct_DType) && !VN_AS(dtypep, ଗଣ୍ଠି_UOr_Struct_DType)->packed()) {
        ଅଚଳ Ast_ଗଣ୍ଠି_UOr_Struct_DType* ଅଚଳ sdtypep = VN_AS(dtypep, ଗଣ୍ଠି_UOr_Struct_DType);
        string literal;
        ପାଇଁ (ଅଚଳ AstMemberDType* itemp = sdtypep->membersp(); itemp;
             itemp = VN_AS(itemp->nextp(), MemberDType)) {
            ଅଚଳ std::string ଧାଡ଼ି = emitVarResetRecurse(
                varp, constructing, varNameProtected + suffix + "." + itemp->nameProtect(),
                itemp->dtypep(), depth + 1, "", itemp->valuep());
            ଯଦି (!ଧାଡ଼ି.empty()) literal += ଧାଡ଼ି;
        }
        ଫେରନ୍ତୁ literal;
    } ଅନ୍ୟଥା ଯଦି (basicp && basicp->keyword() == VBasicDTypeKwd::STRING) {
        ଯଦି (constructing) ଫେରନ୍ତୁ "";  // String's constructor deals with it
        ଫେରନ୍ତୁ varNameProtected + suffix + ".clear();\n";
    } ଅନ୍ୟଥା ଯଦି (basicp && basicp->isForkSync()) {
        ଫେରନ୍ତୁ "";
    } ଅନ୍ୟଥା ଯଦି (basicp && basicp->isProcessRef()) {
        ଫେରନ୍ତୁ "";
    } ଅନ୍ୟଥା ଯଦି (basicp && basicp->isDelayScheduler()) {
        ଫେରନ୍ତୁ "";
    } ଅନ୍ୟଥା ଯଦି (basicp && basicp->isTriggerScheduler()) {
        ଫେରନ୍ତୁ "";
    } ଅନ୍ୟଥା ଯଦି (basicp && basicp->isDynamicTriggerScheduler()) {
        ଫେରନ୍ତୁ "";
    } ଅନ୍ୟଥା ଯଦି (basicp && (basicp->isRandomGenerator() || basicp->isStdRandomGenerator())) {
        ଫେରନ୍ତୁ "";
    } ଅନ୍ୟଥା ଯଦି (basicp && basicp->isCovergroupInstHandle()) {
        // The handle's own constructor deals with it.
        ଫେରନ୍ତୁ "";
    } ଅନ୍ୟଥା ଯଦି (basicp && (basicp->isEvent())) {
        ଫେରନ୍ତୁ "VlAssignableEvent{};\n";
    } ଅନ୍ୟଥା ଯଦି (basicp) {
        ଅଚଳ ତାର୍କିକ zeroit
            = (varp->attrFileDescr()  // Zero so we don't do file IO if never $fopen
               || varp->isFuncLocal()  // Randomization too slow
               || (basicp && basicp->isZeroInit())
               || (v3Global.opt.underlineZero() && !varp->name().empty() && varp->name()[0] == '_')
               || (varp->varType().isTemp() && !varp->isXTemp())
               || (varp->isXTemp()
                       ? (v3Global.opt.xAssign() != "unique")
                       : (v3Global.opt.xInitial() == "fast" || v3Global.opt.xInitial() == "0")));
        ଅଚଳ ତାର୍କିକ slow = !varp->isFuncLocal() && !varp->isClassMember();
        splitSizeInc(1);
        ଯଦି (dtypep->isWide()) {  // Handle unpacked; not basicp->isWide
            string out;
            ଯଦି (valuep) {
                ଅଚଳ AstConst* ଅଚଳ constp = VN_AS(valuep, Const);
                UASSERT_OBJ(constp, varp, "non-const initializer for variable");
                ପାଇଁ (ପୂର୍ଣ୍ଣସଂଖ୍ୟା w = 0; w < dtypep->width_ଶବ୍ଦଗୁଡ଼ିକ(); ++w) {
                    out += varNameProtected + suffix + "[" + cvtToStr(w) + "] = ";
                    out += cvtToStr(constp->ଅଙ୍କ().edataWord(w)) + "U;\n";
                }
            } ଅନ୍ୟଥା {
                out += zeroit ? (slow ? "VL_ZERO_RESET_W(" : "VL_ZERO_W(")
                              : (varp->isXTemp() ? "VL_SCOPED_RAND_RESET_ASSIGN_W("
                                                 : "VL_SCOPED_RAND_RESET_W(");
                out += cvtToStr(dtypep->widthMin());
                out += ", " + varNameProtected + suffix;
                ଯଦି (!zeroit) {
                    emitVarResetScopeHash();
                    ଅଚଳ uint64_t salt = VString::hashMurmur(varp->prettyName());
                    out += ", ";
                    out += m_classOrPackage ? m_classOrPackageHash : "__VscopeHash";
                    out += ", ";
                    out += std::to_string(salt);
                    out += "ull";
                }
                out += ");\n";
            }
            ଫେରନ୍ତୁ out;
        } ଅନ୍ୟଥା {
            string out = varNameProtected + suffix;
            ଯଦି (valuep) {
                out += " = ";
                // TODO cleanup code shared between here, V3EmitCConstInit.h,
                // EmitCFunc::emitVarReset, EmitCFunc::emitConstant
                ଅଚଳ AstConst* ଅଚଳ constp = VN_AS(valuep, Const);
                UASSERT_OBJ(constp, varp, "non-const initializer for variable");
                out += cvtToStr(constp->ଅଙ୍କ().edataWord(0)) + "U;\n";
                out += ";\n";
            } ଅନ୍ୟଥା ଯଦି (zeroit) {
                out += " = 0;\n";
            } ଅନ୍ୟଥା {
                emitVarResetScopeHash();
                ଅଚଳ uint64_t salt = VString::hashMurmur(varp->prettyName());
                out += " = VL_SCOPED_RAND_RESET_";
                ଯଦି (varp->isXTemp()) out += "ASSIGN_";
                out += dtypep->charIQWN();
                out += "(" + cvtToStr(dtypep->widthMin()) + ", "
                       + (m_classOrPackage ? m_classOrPackageHash : "__VscopeHash") + ", "
                       + std::to_string(salt) + "ull);\n";
            }
            ଫେରନ୍ତୁ out;
        }
    } ଅନ୍ୟଥା {  // LCOV_EXCL_BR_LINE
        v3fatalSrc("Unknown node type in reset generator: " << varp->prettyTypeName());
    }
    ଫେରନ୍ତୁ "";
}

ଶୂନ୍ୟ EmitCFunc::emitVarResetScopeHash() {
    ଯଦି (VL_LIKELY(m_createdScopeHash)) { ଫେରନ୍ତୁ; }
    ଯଦି (m_classOrPackage) {
        m_classOrPackageHash
            = std::to_string(VString::hashMurmur(m_classOrPackage->name())) + "ULL";
    } ଅନ୍ୟଥା {
        puts(string("const uint64_t __VscopeHash = VL_MURMUR64_HASH(")
             + (m_useSelfForThis ? "vlSelf" : "this") + "->vlNamep);\n");
    }
    m_createdScopeHash = ସତ୍ୟ;
}
