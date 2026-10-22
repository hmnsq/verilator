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

#ಅನ್ತರ್ಭಾವ "V3PchAstMT.h"

#ಅನ್ತರ್ಭಾವ "V3EmitCFunc.h"

#ಅನ್ತರ್ಭಾವ <map>
#ಅನ್ತರ್ಭಾವ <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// EmitCFunc

ತಾರ್ಕಿಕ EmitCFunc::emitSimpleOk(Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* nodep) {
    // Can we put out a simple (A + B) instead of VL_ADD_III(A,B)?
    ಯದಿ (nodep->emitSimpleOperator() == "") ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
    ಯದಿ (nodep->isWide()) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
    ಯದಿ (nodep->op1p() && nodep->op1p()->isWide()) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
    ಯದಿ (nodep->op2p() && nodep->op2p()->isWide()) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
    ಯದಿ (nodep->op3p() && nodep->op3p()->isWide()) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
    ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;
}

ಶೂನ್ಯ EmitCFunc::emitOpName(Ast_ಗಂಟು* nodep, ಧ್ರುವ string& format, Ast_ಗಂಟು* lhsp, Ast_ಗಂಟು* rhsp,
                           Ast_ಗಂಟು* thsp) {
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

    ತಾರ್ಕಿಕ needComma = ಅಸತ್ಯ;
    ತಾರ್ಕಿಕ usesQueue = ಅಸತ್ಯ;
    string nextComma;
    auto commaOut = [&out, &nextComma]() {
        ಯದಿ (!nextComma.empty()) {
            out += nextComma;
            nextComma = "";
        }
    };

    auto putOut = [ಏತತ್, &out]() {
        ಯದಿ (!out.empty()) puts(out);
        out = "";
    };

    ಪ್ರತಿ (string::const_iterator pos = format.begin(); pos != format.end(); ++pos) {
        ಯದಿ (pos[0] == ',') {
            // Remember we need to add one, but don't do yet to avoid ",)"
            ಯದಿ (needComma) {
                ಯದಿ (pos[1] == ' ') {
                    nextComma = ", ";
                } ಅನ್ಯಥಾ {
                    nextComma = ",";
                }
                needComma = ಅಸತ್ಯ;
            }
            ಯದಿ (pos[1] == ' ') ++pos;  // Must do even if no nextComma
        } ಅನ್ಯಥಾ ಯದಿ (pos[0] == '%') {
            ++pos;
            ತಾರ್ಕಿಕ detail = ಅಸತ್ಯ;
            Ast_ಗಂಟು* detailp = nullptr;
            ವಿಕಲ್ಪ (pos[0]) {
            ಪಕ್ಷ '%': out += '%'; ಭಙ್ಗ;
            ಪಕ್ಷ 'k':
                putOut();
                putbs("");
                ಭಙ್ಗ;
            ಪಕ್ಷ 'n':
                detail = ಸತ್ಯ;
                detailp = nodep;
                ಭಙ್ಗ;
            ಪಕ್ಷ 'l':
                detail = ಸತ್ಯ;
                detailp = lhsp;
                ಭಙ್ಗ;
            ಪಕ್ಷ 'r':
                detail = ಸತ್ಯ;
                detailp = rhsp;
                ಭಙ್ಗ;
            ಪಕ್ಷ 't':
                detail = ಸತ್ಯ;
                detailp = thsp;
                ಭಙ್ಗ;
            ಪಕ್ಷ 'P':
                ಯದಿ (nodep->isWide() && !usesQueue) {
                    UASSERT_OBJ(m_wideTempRefp, nodep,
                                "Wide op " << nodep->prettyTypeName()
                                           << " w/ no temp, perhaps missing op in V3EmitC?");
                    commaOut();
                    putOut();
                    ಯದಿ (!m_wideTempRefp->selfPointer().isEmpty()) {
                        emitDereference(m_wideTempRefp,
                                        m_wideTempRefp->selfPointerProtect(m_useSelfForThis));
                    }
                    out += varName(m_wideTempRefp->varp());
                    m_wideTempRefp = nullptr;
                    needComma = ಸತ್ಯ;
                } ಅನ್ಯಥಾ ಯದಿ (usesQueue) {
                    commaOut();
                    putOut();
                    iterateAndNextConstNull(nodep->backp()->op2p());
                    needComma = ಸತ್ಯ;
                }

                ಭಙ್ಗ;
            ಸ್ವಭಾವ: nodep->v3fatalSrc("Unknown emitOperator format code: %" << pos[0]); ಭಙ್ಗ;
            }
            ಯದಿ (detail) {
                // Get next letter of %[nlrt]
                ++pos;
                ವಿಕಲ್ಪ (pos[0]) {
                ಪಕ್ಷ 'q':
                    putOut();
                    // If we are assigning this to a queue we need to get the return type
                    ಯದಿ (VN_IS(detailp->backp(), Assign)
                        && VN_IS(detailp->backp()->op2p()->dtypep()->skipRefp(), QueueDType)) {
                        puts("R");  // R for queue
                        usesQueue = ಸತ್ಯ;
                    } ಅನ್ಯಥಾ ಯದಿ (VN_IS(detailp->dtypep()->skipRefp(), QueueDType)
                               || VN_IS(detailp->dtypep()->skipRefp(), StreamDType)) {
                        puts("R");  // R for queue
                    } ಅನ್ಯಥಾ {
                        emitIQW(detailp);
                    }
                    ಭಙ್ಗ;
                ಪಕ್ಷ 'w':
                    commaOut();
                    out += cvtToStr(detailp->widthMin());
                    needComma = ಸತ್ಯ;
                    ಭಙ್ಗ;
                ಪಕ್ಷ 'W':
                    ಯದಿ (lhsp->isWide()) {
                        commaOut();
                        out += cvtToStr(lhsp->width_ಪದಗಳು());
                        needComma = ಸತ್ಯ;
                    } ಅನ್ಯಥಾ ಯದಿ (VN_IS(lhsp, StreamR)) {
                        commaOut();
                        out += cvtToStr(rhsp->width_ಪದಗಳು());
                        needComma = ಸತ್ಯ;
                    }
                    ಭಙ್ಗ;
                ಪಕ್ಷ 'i':
                    commaOut();
                    UASSERT_OBJ(detailp, nodep, "emitOperator() references undef node");
                    putOut();
                    iterateAndNextConstNull(detailp);
                    needComma = ಸತ್ಯ;
                    ಭಙ್ಗ;
                ಸ್ವಭಾವ:
                    nodep->v3fatalSrc("Unknown emitOperator format code: %[nlrt]" << pos[0]);
                    ಭಙ್ಗ;
                }
            }
        } ಅನ್ಯಥಾ ಯದಿ (pos[0] == ')') {
            nextComma = "";
            out += ')';
        } ಅನ್ಯಥಾ ಯದಿ (pos[0] == '(') {
            commaOut();
            needComma = ಅಸತ್ಯ;
            out += '(';
        } ಅನ್ಯಥಾ {
            // Normal text
            ಯದಿ (std::isalnum(pos[0])) needComma = ಸತ್ಯ;
            commaOut();
            out += pos[0];
        }
    }
    putOut();
}

ತಾರ್ಕಿಕ EmitCFunc::displayEmitHeader(Ast_ಗಂಟು* nodep) {
    ತಾರ್ಕಿಕ isStmt = ಅಸತ್ಯ;
    ಯದಿ (ಧ್ರುವ AstFScanF* ಧ್ರುವ dispp = VN_CAST(nodep, FScanF)) {
        isStmt = ಅಸತ್ಯ;
        putns(nodep, "VL_FSCANF_INX(");
        iterateConst(dispp->filep());
        puts(",");
    } ಅನ್ಯಥಾ ಯದಿ (ಧ್ರುವ AstSScanF* ಧ್ರುವ dispp = VN_CAST(nodep, SScanF)) {
        isStmt = ಅಸತ್ಯ;
        putns(nodep, "VL_SSCANF_I");
        emitIQW(dispp->fromp());
        puts("NX(");
        puts(cvtToStr(dispp->fromp()->widthMin()));
        puts(",");
        iterateConst(dispp->fromp());
        puts(",");
    } ಅನ್ಯಥಾ ಯದಿ (ಧ್ರುವ AstDisplay* ಧ್ರುವ dispp = VN_CAST(nodep, Display)) {
        isStmt = ಸತ್ಯ;
        ಯದಿ (dispp->filep()) {
            putns(nodep, "VL_FWRITEF_NX(");
            iterateConst(dispp->filep());
            puts(",");
        } ಅನ್ಯಥಾ {
            putns(nodep, "VL_WRITEF_NX(");
        }
    } ಅನ್ಯಥಾ ಯದಿ (ಧ್ರುವ AstSFormat* ಧ್ರುವ dispp = VN_CAST(nodep, SFormat)) {
        isStmt = ಸತ್ಯ;
        puts("VL_SFORMAT_NX(");
        ಯದಿ (!dispp->lhsp()->dtypep()->isString()) {
            puts(cvtToStr(dispp->lhsp()->widthMin()));
            putbs(",");
        }
        iterateConst(dispp->lhsp());
        emitDatap(dispp->lhsp());
        putbs(",");
    } ಅನ್ಯಥಾ ಯದಿ (VN_IS(nodep, SFormatF)) {
        isStmt = ಅಸತ್ಯ;
        putns(nodep, "VL_SFORMATF_N_NX(");
    } ಅನ್ಯಥಾ {
        nodep->v3fatalSrc("Unknown displayEmit node type");
    }
    ಪ್ರತ್ಯಾಗಮ isStmt;
}

ಶೂನ್ಯ EmitCFunc::display_ಗಂಟು(Ast_ಗಂಟು* nodep, AstSFormatF* fmtp,  // fmtp is nullptr for AstScan
                            ಧ್ರುವ string& vformat, Ast_ಗಂಟು* exprsp, ತಾರ್ಕಿಕ isScan) {
    // Check format, if it exists
    ಧ್ರುವ ತಾರ್ಕಿಕ ಅಭಿವ್ಯಕ್ತಿ_Format_2 = fmtp && fmtp->ಅಭಿವ್ಯಕ್ತಿ_Format_2();
    ತಾರ್ಕಿಕ needsScope = ಅಭಿವ್ಯಕ್ತಿ_Format_2;
    ತಾರ್ಕಿಕ needsTimescale = ಅಭಿವ್ಯಕ್ತಿ_Format_2;
    ಪೂರ್ಣಾಙ್ಕ formatArgs = 0;
    ಯದಿ (!ಅಭಿವ್ಯಕ್ತಿ_Format_2) {
        // Check display and argument count to head-off later runtime issues
        ತಾರ್ಕಿಕ inPct = ಅಸತ್ಯ;
        ತಾರ್ಕಿಕ ignore = ಅಸತ್ಯ;
        ಪ್ರತಿ (ವರ್ಣ ch : vformat) {
            // UINFO(1, "Parse '" << *pos << "'  IP" << inPct << " List " << cvtToHex(elistp));
            ಯದಿ (!inPct && ch == '%') {
                inPct = ಸತ್ಯ;
                ignore = ಅಸತ್ಯ;
            } ಅನ್ಯಥಾ ಯದಿ (inPct && (std::isdigit(ch) || ch == '.' || ch == '-')) {
            } ಅನ್ಯಥಾ ಯದಿ (!inPct) {  // Normal text
            } ಅನ್ಯಥಾ {  // Format character
                inPct = ಅಸತ್ಯ;
                ch = std::tolower(ch);
                ವಿಕಲ್ಪ (ch) {
                ಪಕ್ಷ '%': ಭಙ್ಗ;
                ಪಕ್ಷ '*':
                    inPct = ಸತ್ಯ;  // Get more digits
                    ignore = ಸತ್ಯ;
                    ಭಙ್ಗ;
                ಪಕ್ಷ 'm': needsScope = ಸತ್ಯ; ಭಙ್ಗ;
                ಪಕ್ಷ 't':
                    needsTimescale = ಸತ್ಯ;
                    ++formatArgs;
                    ಭಙ್ಗ;
                ಸ್ವಭಾವ:
                    ಯದಿ (!ignore && V3_ಸಂಖ್ಯೆ::displayedFmtHasArg(ch, isScan)) ++formatArgs;
                    ಭಙ್ಗ;
                }
            }
        }
    }

    ಯದಿ (vformat.empty() && VN_IS(nodep, Display))  // not fscanf etc, as they need to return value
        ಪ್ರತ್ಯಾಗಮ;  // NOP

    ಧ್ರುವ ತಾರ್ಕಿಕ isStmt = displayEmitHeader(nodep);

    ಯದಿ (ಅಭಿವ್ಯಕ್ತಿ_Format_2) {
        UASSERT_OBJ(exprsp, nodep, "Missing format expression");
        iterateConst(exprsp);
        emitDatap(exprsp);
        exprsp = exprsp->nextp();
    } ಅನ್ಯಥಾ {
        ofp()->putsQuoted(vformat);
    }

    ಪೂರ್ಣಾಙ್ಕ ಅಭಿವ್ಯಕ್ತಿ_Args = 0;
    ಪ್ರತಿ (Ast_ಗಂಟು* argp = exprsp; argp; argp = argp->nextp()) ++ಅಭಿವ್ಯಕ್ತಿ_Args;
    ಯದಿ (VL_UNCOVERABLE(!ಅಭಿವ್ಯಕ್ತಿ_Format_2 && ಅಭಿವ್ಯಕ್ತಿ_Args != formatArgs)) {  // LCOV_EXCL_START
        // expectDisplay() checks this first, so probably internal error if found here
        nodep->v3error("Internal: Missing or extra arguments for $display-like format, "
                       << " expression had " << ಅಭಿವ್ಯಕ್ತಿ_Args << ", format had " << formatArgs);
    }  // LCOV_EXCL_STOP

    // argc for error check. Also MSVC++ requires va_args to not be off a reference
    ಪೂರ್ಣಾಙ್ಕ argc = 0;
    ಯದಿ (needsScope) ++argc;
    ಯದಿ (needsTimescale) ++argc;
    ಪ್ರತಿ (Ast_ಗಂಟು* argp = exprsp; argp; argp = argp->nextp()) {
        ++argc;
        ಧ್ರುವ AstSFormatArg* ಧ್ರುವ fargp = VN_CAST(argp, SFormatArg);
        ಯದಿ (fargp && fargp->formatAttr().isEnum()) ++argc;  // Additional name argument
    }
    ofp()->puts("," + std::to_string(argc));

    ಯದಿ (needsScope) {
        AstScopeName* ಧ್ರುವ scopenamep = fmtp ? fmtp->scopeNamep() : nullptr;
        UASSERT_OBJ(scopenamep, nodep, "Display with %m but no AstScopeName");
        ofp()->puts(", '"s + VFormatAttr{VFormatAttr::SCOPE}.ascii() + "'");
        ಯದಿ (scopenamep->selfRelative()) {
            // vlNamep is catName(name(), instance), so starts with '.' when the model is unnamed
            ಧ್ರುವ string namep = string{m_useSelfForThis ? "vlSelf" : "this"} + "->vlNamep";
            ofp()->puts(",(vlSymsp->name()[0] ? " + namep + " : " + namep + " + 1),");
            ofp()->putsQuoted(scopenamep->scopeAttr());
        } ಅನ್ಯಥಾ {
            ofp()->puts(",vlSymsp->name(),");
            ofp()->putsQuoted(scopenamep->scopePrettySymName());
        }
    }

    ಯದಿ (needsTimescale) {
        VTimescale timeunit = VTimescale::NONE;
        ಯದಿ (ಧ್ರುವ AstDisplay* ಧ್ರುವ anodep = VN_CAST(nodep, Display)) {
            timeunit = anodep->fmtp()->timeunit();
        } ಅನ್ಯಥಾ ಯದಿ (ಧ್ರುವ AstSFormat* ಧ್ರುವ anodep = VN_CAST(nodep, SFormat)) {
            timeunit = anodep->fmtp()->timeunit();
        } ಅನ್ಯಥಾ ಯದಿ (ಧ್ರುವ AstSScanF* ಧ್ರುವ anodep = VN_CAST(nodep, SScanF)) {
            timeunit = anodep->timeunit();
        } ಅನ್ಯಥಾ ಯದಿ (ಧ್ರುವ AstSFormatF* ಧ್ರುವ anodep = VN_CAST(nodep, SFormatF)) {
            timeunit = anodep->timeunit();
        }
        UASSERT_OBJ(!timeunit.isNone(), nodep,
                    "Use of %t must be under AstDisplay, AstSFormat, or AstSFormatF, or "
                    "SScanF, and timeunit set");
        ofp()->puts(", '"s + VFormatAttr{VFormatAttr::TIMEUNIT}.ascii() + "'");
        ofp()->puts(","s + cvtToStr((ಪೂರ್ಣಾಙ್ಕ)timeunit.powerOfTen()));
    }

    // Arguments
    ಪ್ರತಿ (Ast_ಗಂಟು* argp = exprsp; argp; argp = argp->nextp()) {
        ofp()->indentInc();
        ofp()->putbs("");
        AstSFormatArg* ಧ್ರುವ fargp = VN_CAST(argp, SFormatArg);
        Ast_ಗಂಟು* ಧ್ರುವ subargp = fargp ? fargp->exprp() : argp;
        ಧ್ರುವ VFormatAttr formatAttr = AstSFormatArg::formatAttrDefauled(fargp, subargp->dtypep());
        puts(", '"s + formatAttr.ascii() + '\'');
        ಯದಿ (formatAttr.isSigned() || formatAttr.isUnsigned() || formatAttr.isEnum())
            puts("," + cvtToStr(subargp->widthMin()));
        ಧ್ರುವ ತಾರ್ಕಿಕ addrof = isScan || formatAttr.isString() || formatAttr.isComplex();
        ಧ್ರುವ ತಾರ್ಕಿಕ wideEnum = formatAttr.isEnum() && subargp->isWide();
        puts(",");
        ಯದಿ (wideEnum) puts("static_cast<const EData*>(");
        ಯದಿ (addrof) puts("&(");
        ಯದಿ (VN_IS(subargp, StreamR))
            emitStreamR(
                VN_CAST(subargp, StreamR),
                nodep);  // This has to be done here because streamR doesn't know what it returns
        ಅನ್ಯಥಾ { iterateConst(subargp); }
        ಯದಿ (addrof) puts(")");
        ಯದಿ (!addrof) emitDatap(argp);
        ಯದಿ (wideEnum) puts(")");
        ಯದಿ (formatAttr.isEnum()) {
            puts(", '"s + VFormatAttr{VFormatAttr::STRING}.ascii() + "', &(");
            iterateConst(fargp->namep());
            puts(")");
        }
        ofp()->indentDec();
    }

    // End
    ಯದಿ (isStmt) {
        puts(");\n");
    } ಅನ್ಯಥಾ {
        puts(") ");
    }
}

ಶೂನ್ಯ EmitCFunc::emitCCallArgs(ಧ್ರುವ Ast_ಗಂಟು_CCall* nodep, ಧ್ರುವ string& selfPointer,
                              ತಾರ್ಕಿಕ inProcess) {
    putns(nodep, "(");
    ತಾರ್ಕಿಕ comma = ಅಸತ್ಯ;
    ಯದಿ (nodep->funcp()->isLoose() && !nodep->funcp()->isStatic()) {
        UASSERT_OBJ(!selfPointer.empty(), nodep, "Call to loose method without self pointer");
        puts(selfPointer);
        comma = ಸತ್ಯ;
    }
    ಯದಿ (nodep->funcp()->needProcess()) {
        ಯದಿ (comma) puts(", ");
        ಯದಿ (VN_IS(nodep->backp(), CAwait) || !nodep->funcp()->isCoroutine()) {
            puts("vlProcess");
        } ಅನ್ಯಥಾ ಯದಿ (inProcess) {
            puts("std::make_shared<VlProcess>(vlProcess)");
        } ಅನ್ಯಥಾ {
            puts("std::make_shared<VlProcess>()");
        }
        comma = ಸತ್ಯ;
    }
    ಯದಿ (!nodep->argTypes().empty()) {
        ಯದಿ (comma) puts(", ");
        puts(nodep->argTypes());
        comma = ಸತ್ಯ;
    }
    putCommaIterateNext(nodep->argsp(), comma);
    puts(")");
}

std::string EmitCFunc::dereferenceString(ಧ್ರುವ std::string& pointer) ಧ್ರುವ {
    ಯದಿ (pointer[0] == '(' && pointer[1] == '&') {
        // remove "address of" followed by immediate dereference
        // Note: this relies on only the form '(&OBJECT)' being used by Verilator
        ಪ್ರತ್ಯಾಗಮ pointer.substr(2, pointer.length() - 3) + '.';
    } ಅನ್ಯಥಾ {
        ಯದಿ (pointer == "vlSelf" && m_usevlSelfRef) {
            ಪ್ರತ್ಯಾಗಮ "vlSelfRef.";
        } ಅನ್ಯಥಾ {
            ಪ್ರತ್ಯಾಗಮ pointer + "->";
        }
    }
}
ಶೂನ್ಯ EmitCFunc::emitDereference(Ast_ಗಂಟು* nodep, ಧ್ರುವ string& pointer) {
    putns(nodep, dereferenceString(pointer));
}

ಶೂನ್ಯ EmitCFunc::emitCvtPackStr(Ast_ಗಂಟು* nodep) {
    ಯದಿ (ಧ್ರುವ AstConst* ಧ್ರುವ constp = VN_CAST(nodep, Const)) {
        emitConstantString(constp);
    } ಅನ್ಯಥಾ ಯದಿ (VN_IS(nodep->dtypep(), StreamDType)) {
        putnbs(nodep, "VL_CVT_PACK_STR_ND(");
        iterateAndNextConstNull(nodep);
        puts(")");
    } ಅನ್ಯಥಾ {
        putnbs(nodep, "VL_CVT_PACK_STR_N");
        emitIQW(nodep);
        puts("(");
        ಯದಿ (nodep->isWide()) {
            // Note argument width, not node width (which is always 32)
            puts(cvtToStr(nodep->width_ಪದಗಳು()));
            puts(", ");
        }
        iterateAndNextConstNull(nodep);
        puts(")");
    }
}

ಶೂನ್ಯ EmitCFunc::emitConstant(AstConst* nodep) {
    // Put out constant set to the specified variable, or given variable in a string
    ಧ್ರುವ V3_ಸಂಖ್ಯೆ& ಅಂಕೆ = nodep->ಅಂಕೆ();
    ಯದಿ (ಅಂಕೆ.isFourState()) {
        nodep->v3warn(E_UNSUPPORTED, "Unsupported: 4-state numbers in this context");
        ಪ್ರತ್ಯಾಗಮ;
    }
    putns(nodep, ಅಂಕೆ.emitC());
}

ಶೂನ್ಯ EmitCFunc::emitConstantString(ಧ್ರುವ AstConst* nodep) {
    // Const might be a Verilog array-type string, but need to always output std::string
    putnbs(nodep, "std::string{");
    ಧ್ರುವ string str = nodep->ಅಂಕೆ().toString();
    ಯದಿ (!str.empty()) putsQuoted(str);
    puts("}");
}

ಶೂನ್ಯ EmitCFunc::emitVarReset(ಧ್ರುವ string& prefix, AstVar* varp, ತಾರ್ಕಿಕ constructing) {
    // 'constructing' indicates that the object was just constructed, so if it is a string or
    // something that starts off clear already, no need to clear it again
    Ast_ಗಂಟು_DType* ಧ್ರುವ dtypep = varp->dtypep()->skipRefp();
    ಧ್ರುವ string vlSelf = VSelfPointerText::replaceThis(m_useSelfForThis, "this->");
    ಧ್ರುವ string varNameProtected
        = ((VN_IS(m_modp, Class) || varp->isFuncLocal()) || !prefix.empty())
              ? varName(varp)
              : vlSelf + varName(varp);
    ಧ್ರುವ string newPrefix = prefix + varNameProtected;
    ಯದಿ (varp->isIO() && m_modp->is_ತುದಿ() && optSystemC()) {
        // System C top I/O doesn't need loading, as the lower level subinst code does it.}
    } ಅನ್ಯಥಾ ಯದಿ (varp->isParam()) {
        UASSERT_OBJ(varp->valuep(), varp, "No init for a param?");
        // If a simple CONST value we initialize it using an enum
        // If an ARRAYINIT we initialize it using an initial block similar to a signal
        // puts("// parameter "+varp->nameProtect()+" = "+varp->valuep()->name()+"\n");
    } ಅನ್ಯಥಾ {
        putns(varp,
              emitVarResetRecurse(varp, constructing, newPrefix, dtypep, 0, "", varp->valuep()));
    }
}

string EmitCFunc::emitVarResetRecurse(ಧ್ರುವ AstVar* varp, ತಾರ್ಕಿಕ constructing,
                                      ಧ್ರುವ string& varNameProtected, Ast_ಗಂಟು_DType* dtypep,
                                      ಪೂರ್ಣಾಙ್ಕ depth, ಧ್ರುವ string& suffix, ಧ್ರುವ Ast_ಗಂಟು* valuep) {
    dtypep = dtypep->skipRefp();
    AstBasicDType* ಧ್ರುವ basicp = dtypep->basicp();
    // Returns string to do resetting, empty to do nothing (which caller should handle)
    ಯದಿ (AstAssocArrayDType* ಧ್ರುವ adtypep = VN_CAST(dtypep, AssocArrayDType)) {
        // Access std::array as C array
        ಧ್ರುವ string pre = constructing ? "" : varNameProtected + suffix + ".clear();\n";
        ಪ್ರತ್ಯಾಗಮ pre
               + emitVarResetRecurse(varp, constructing, varNameProtected, adtypep->subDTypep(),
                                     depth + 1, suffix + ".atDefault()", nullptr);
    } ಅನ್ಯಥಾ ಯದಿ (AstWildcardArrayDType* ಧ್ರುವ adtypep = VN_CAST(dtypep, WildcardArrayDType)) {
        // Access std::array as C array
        ಧ್ರುವ string pre = constructing ? "" : varNameProtected + suffix + ".clear();\n";
        ಪ್ರತ್ಯಾಗಮ pre
               + emitVarResetRecurse(varp, constructing, varNameProtected, adtypep->subDTypep(),
                                     depth + 1, suffix + ".atDefault()", nullptr);
    } ಅನ್ಯಥಾ ಯದಿ (VN_IS(dtypep, CDType)) {
        ಪ್ರತ್ಯಾಗಮ "";  // Constructor does it
    } ಅನ್ಯಥಾ ಯದಿ (VN_IS(dtypep, CoverCrossDType) || VN_IS(dtypep, CoverpointDType)) {
        ಪ್ರತ್ಯಾಗಮ "";  // Covergroup constructor creates the runtime and assigns the pointer
    } ಅನ್ಯಥಾ ಯದಿ (ಧ್ರುವ AstClassRefDType* ಧ್ರುವ adtypep = VN_CAST(dtypep, ClassRefDType)) {
        ಪ್ರತ್ಯಾಗಮ adtypep->rawPointer() ? varNameProtected + suffix + " = nullptr;\n" : "";
    } ಅನ್ಯಥಾ ಯದಿ (VN_IS(dtypep, IfaceRefDType)) {
        ಪ್ರತ್ಯಾಗಮ varNameProtected + suffix + " = nullptr;\n";
    } ಅನ್ಯಥಾ ಯದಿ (ಧ್ರುವ AstDynArrayDType* ಧ್ರುವ adtypep = VN_CAST(dtypep, DynArrayDType)) {
        ಧ್ರುವ string pre = constructing ? "" : varNameProtected + suffix + ".clear();\n";
        ಪ್ರತ್ಯಾಗಮ pre
               + emitVarResetRecurse(varp, constructing, varNameProtected, adtypep->subDTypep(),
                                     depth + 1, suffix + ".atDefault()", nullptr);
    } ಅನ್ಯಥಾ ಯದಿ (ಧ್ರುವ AstQueueDType* ಧ್ರುವ adtypep = VN_CAST(dtypep, QueueDType)) {
        ಧ್ರುವ string pre = constructing ? "" : varNameProtected + suffix + ".clear();\n";
        ಪ್ರತ್ಯಾಗಮ pre
               + emitVarResetRecurse(varp, constructing, varNameProtected, adtypep->subDTypep(),
                                     depth + 1, suffix + ".atDefault()", nullptr);
    } ಅನ್ಯಥಾ ಯದಿ (VN_IS(dtypep, SampleQueueDType)) {
        ಪ್ರತ್ಯಾಗಮ "";
    } ಅನ್ಯಥಾ ಯದಿ (ಧ್ರುವ AstUnpackArrayDType* ಧ್ರುವ adtypep = VN_CAST(dtypep, UnpackArrayDType)) {
        UASSERT_OBJ(adtypep->hi() >= adtypep->lo(), varp,
                    "Should have swapped msb & lsb earlier.");
        ಧ್ರುವ Ast_ಗಂಟು_DType* ಧ್ರುವ elementp = adtypep->subDTypep()->skipRefp();
        ಯದಿ (!varp->isFuncLocal() && elementp->isIntegralOrPacked()) {
            // Scoped reset has no array-index input or mutable RNG state. Initialize one
            // element, then fill in the optimized runtime even when this code is cold.
            ಧ್ರುವ string first
                = emitVarResetRecurse(varp, constructing, varNameProtected, adtypep->subDTypep(),
                                      depth + 1, suffix + "[0]", nullptr);
            ಧ್ರುವ string array = varNameProtected + suffix;
            ಪ್ರತ್ಯಾಗಮ first + "VL_UNPACKED_FILL(" + array + ".data(), sizeof(" + array + "[0]), "
                   + cvtToStr(adtypep->elementsConst()) + ");\n";
        }
        ಧ್ರುವ string ivar = "__Vi"s + cvtToStr(depth);
        ಧ್ರುವ string pre = ("for (int " + ivar + " = " + cvtToStr(0) + "; " + ivar + " < "
                            + cvtToStr(adtypep->elementsConst()) + "; ++" + ivar + ") {\n");
        ಧ್ರುವ string below
            = emitVarResetRecurse(varp, constructing, varNameProtected, adtypep->subDTypep(),
                                  depth + 1, suffix + "[" + ivar + "]", nullptr);
        ಧ್ರುವ string post = "}\n";
        ಪ್ರತ್ಯಾಗಮ below.empty() ? "" : pre + below + post;
    } ಅನ್ಯಥಾ ಯದಿ (VN_IS(dtypep, ಗಂಟು_UOr_Struct_DType) && !VN_AS(dtypep, ಗಂಟು_UOr_Struct_DType)->packed()) {
        ಧ್ರುವ Ast_ಗಂಟು_UOr_Struct_DType* ಧ್ರುವ sdtypep = VN_AS(dtypep, ಗಂಟು_UOr_Struct_DType);
        string literal;
        ಪ್ರತಿ (ಧ್ರುವ AstMemberDType* itemp = sdtypep->membersp(); itemp;
             itemp = VN_AS(itemp->nextp(), MemberDType)) {
            ಧ್ರುವ std::string ಸಾಲು = emitVarResetRecurse(
                varp, constructing, varNameProtected + suffix + "." + itemp->nameProtect(),
                itemp->dtypep(), depth + 1, "", itemp->valuep());
            ಯದಿ (!ಸಾಲು.empty()) literal += ಸಾಲು;
        }
        ಪ್ರತ್ಯಾಗಮ literal;
    } ಅನ್ಯಥಾ ಯದಿ (basicp && basicp->keyword() == VBasicDTypeKwd::STRING) {
        ಯದಿ (constructing) ಪ್ರತ್ಯಾಗಮ "";  // String's constructor deals with it
        ಪ್ರತ್ಯಾಗಮ varNameProtected + suffix + ".clear();\n";
    } ಅನ್ಯಥಾ ಯದಿ (basicp && basicp->isForkSync()) {
        ಪ್ರತ್ಯಾಗಮ "";
    } ಅನ್ಯಥಾ ಯದಿ (basicp && basicp->isProcessRef()) {
        ಪ್ರತ್ಯಾಗಮ "";
    } ಅನ್ಯಥಾ ಯದಿ (basicp && basicp->isDelayScheduler()) {
        ಪ್ರತ್ಯಾಗಮ "";
    } ಅನ್ಯಥಾ ಯದಿ (basicp && basicp->isTriggerScheduler()) {
        ಪ್ರತ್ಯಾಗಮ "";
    } ಅನ್ಯಥಾ ಯದಿ (basicp && basicp->isDynamicTriggerScheduler()) {
        ಪ್ರತ್ಯಾಗಮ "";
    } ಅನ್ಯಥಾ ಯದಿ (basicp && (basicp->isRandomGenerator() || basicp->isStdRandomGenerator())) {
        ಪ್ರತ್ಯಾಗಮ "";
    } ಅನ್ಯಥಾ ಯದಿ (basicp && basicp->isCovergroupInstHandle()) {
        // The handle's own constructor deals with it.
        ಪ್ರತ್ಯಾಗಮ "";
    } ಅನ್ಯಥಾ ಯದಿ (basicp && (basicp->isEvent())) {
        ಪ್ರತ್ಯಾಗಮ "VlAssignableEvent{};\n";
    } ಅನ್ಯಥಾ ಯದಿ (basicp) {
        ಧ್ರುವ ತಾರ್ಕಿಕ zeroit
            = (varp->attrFileDescr()  // Zero so we don't do file IO if never $fopen
               || varp->isFuncLocal()  // Randomization too slow
               || (basicp && basicp->isZeroInit())
               || (v3Global.opt.underlineZero() && !varp->name().empty() && varp->name()[0] == '_')
               || (varp->varType().isTemp() && !varp->isXTemp())
               || (varp->isXTemp()
                       ? (v3Global.opt.xAssign() != "unique")
                       : (v3Global.opt.xInitial() == "fast" || v3Global.opt.xInitial() == "0")));
        ಧ್ರುವ ತಾರ್ಕಿಕ slow = !varp->isFuncLocal() && !varp->isClassMember();
        splitSizeInc(1);
        ಯದಿ (dtypep->isWide()) {  // Handle unpacked; not basicp->isWide
            string out;
            ಯದಿ (valuep) {
                ಧ್ರುವ AstConst* ಧ್ರುವ constp = VN_AS(valuep, Const);
                UASSERT_OBJ(constp, varp, "non-const initializer for variable");
                ಪ್ರತಿ (ಪೂರ್ಣಾಙ್ಕ w = 0; w < dtypep->width_ಪದಗಳು(); ++w) {
                    out += varNameProtected + suffix + "[" + cvtToStr(w) + "] = ";
                    out += cvtToStr(constp->ಅಂಕೆ().edataWord(w)) + "U;\n";
                }
            } ಅನ್ಯಥಾ {
                out += zeroit ? (slow ? "VL_ZERO_RESET_W(" : "VL_ZERO_W(")
                              : (varp->isXTemp() ? "VL_SCOPED_RAND_RESET_ASSIGN_W("
                                                 : "VL_SCOPED_RAND_RESET_W(");
                out += cvtToStr(dtypep->widthMin());
                out += ", " + varNameProtected + suffix;
                ಯದಿ (!zeroit) {
                    emitVarResetScopeHash();
                    ಧ್ರುವ uint64_t salt = VString::hashMurmur(varp->prettyName());
                    out += ", ";
                    out += m_classOrPackage ? m_classOrPackageHash : "__VscopeHash";
                    out += ", ";
                    out += std::to_string(salt);
                    out += "ull";
                }
                out += ");\n";
            }
            ಪ್ರತ್ಯಾಗಮ out;
        } ಅನ್ಯಥಾ {
            string out = varNameProtected + suffix;
            ಯದಿ (valuep) {
                out += " = ";
                // TODO cleanup code shared between here, V3EmitCConstInit.h,
                // EmitCFunc::emitVarReset, EmitCFunc::emitConstant
                ಧ್ರುವ AstConst* ಧ್ರುವ constp = VN_AS(valuep, Const);
                UASSERT_OBJ(constp, varp, "non-const initializer for variable");
                out += cvtToStr(constp->ಅಂಕೆ().edataWord(0)) + "U;\n";
                out += ";\n";
            } ಅನ್ಯಥಾ ಯದಿ (zeroit) {
                out += " = 0;\n";
            } ಅನ್ಯಥಾ {
                emitVarResetScopeHash();
                ಧ್ರುವ uint64_t salt = VString::hashMurmur(varp->prettyName());
                out += " = VL_SCOPED_RAND_RESET_";
                ಯದಿ (varp->isXTemp()) out += "ASSIGN_";
                out += dtypep->charIQWN();
                out += "(" + cvtToStr(dtypep->widthMin()) + ", "
                       + (m_classOrPackage ? m_classOrPackageHash : "__VscopeHash") + ", "
                       + std::to_string(salt) + "ull);\n";
            }
            ಪ್ರತ್ಯಾಗಮ out;
        }
    } ಅನ್ಯಥಾ {  // LCOV_EXCL_BR_LINE
        v3fatalSrc("Unknown node type in reset generator: " << varp->prettyTypeName());
    }
    ಪ್ರತ್ಯಾಗಮ "";
}

ಶೂನ್ಯ EmitCFunc::emitVarResetScopeHash() {
    ಯದಿ (VL_LIKELY(m_createdScopeHash)) { ಪ್ರತ್ಯಾಗಮ; }
    ಯದಿ (m_classOrPackage) {
        m_classOrPackageHash
            = std::to_string(VString::hashMurmur(m_classOrPackage->name())) + "ULL";
    } ಅನ್ಯಥಾ {
        puts(string("const uint64_t __VscopeHash = VL_MURMUR64_HASH(")
             + (m_useSelfForThis ? "vlSelf" : "this") + "->vlNamep);\n");
    }
    m_createdScopeHash = ಸತ್ಯ;
}
