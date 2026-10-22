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
    ಆದರೆ (nodep->emitSimpleOperator() == "") ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
    ಆದರೆ (nodep->isWide()) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
    ಆದರೆ (nodep->op1p() && nodep->op1p()->isWide()) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
    ಆದರೆ (nodep->op2p() && nodep->op2p()->isWide()) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
    ಆದರೆ (nodep->op3p() && nodep->op3p()->isWide()) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
    ಹಿಂತಿರುಗಿ ನಿಜ;
}

ಶೂನ್ಯ EmitCFunc::emitOpName(Ast_ಗಂಟು* nodep, ಸ್ಥಿರಾಂಕ string& format, Ast_ಗಂಟು* lhsp, Ast_ಗಂಟು* rhsp,
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
    string ಹೊರಹೋಗು;
    putnbs(nodep, "");

    ತಾರ್ಕಿಕ needComma = ಸುಳ್ಳು;
    ತಾರ್ಕಿಕ ಬಳಸುತ್ತದೆ_ಸರತಿ = ಸುಳ್ಳು;
    string ಮುಂದಿನ_Comma;
    ಸ್ವಯಂಚಾಲಿತ comma_ಹೊರಹೋಗು = [&ಹೊರಹೋಗು, &ಮುಂದಿನ_Comma]() {
        ಆದರೆ (!ಮುಂದಿನ_Comma.ಖಾಲಿಗಳು()) {
            ಹೊರಹೋಗು += ಮುಂದಿನ_Comma;
            ಮುಂದಿನ_Comma = "";
        }
    };

    ಸ್ವಯಂಚಾಲಿತ put_ಹೊರಹೋಗು = [ಇದು, &ಹೊರಹೋಗು]() {
        ಆದರೆ (!ಹೊರಹೋಗು.ಖಾಲಿಗಳು()) puts(ಹೊರಹೋಗು);
        ಹೊರಹೋಗು = "";
    };

    ಗಾಗಿ (string::const_iterator pos = format.begin(); pos != format.ಕೊನೆ(); ++pos) {
        ಆದರೆ (pos[0] == ',') {
            // Remember we need to add one, but don't do yet to avoid ",)"
            ಆದರೆ (needComma) {
                ಆದರೆ (pos[1] == ' ') {
                    ಮುಂದಿನ_Comma = ", ";
                } ಇಲ್ಲದಿದ್ದರೆ {
                    ಮುಂದಿನ_Comma = ",";
                }
                needComma = ಸುಳ್ಳು;
            }
            ಆದರೆ (pos[1] == ' ') ++pos;  // Must do even if no nextComma
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (pos[0] == '%') {
            ++pos;
            ತಾರ್ಕಿಕ detail = ಸುಳ್ಳು;
            Ast_ಗಂಟು* detailp = ಶೂನ್ಯಸೂಚಕ;
            ಆಯ್ಕೆ (pos[0]) {
            ಸ್ಥಿತಿ '%': ಹೊರಹೋಗು += '%'; ನಿಲ್ಲಿಸು;
            ಸ್ಥಿತಿ 'k':
                put_ಹೊರಹೋಗು();
                putbs("");
                ನಿಲ್ಲಿಸು;
            ಸ್ಥಿತಿ 'n':
                detail = ನಿಜ;
                detailp = nodep;
                ನಿಲ್ಲಿಸು;
            ಸ್ಥಿತಿ 'l':
                detail = ನಿಜ;
                detailp = lhsp;
                ನಿಲ್ಲಿಸು;
            ಸ್ಥಿತಿ 'r':
                detail = ನಿಜ;
                detailp = rhsp;
                ನಿಲ್ಲಿಸು;
            ಸ್ಥಿತಿ 't':
                detail = ನಿಜ;
                detailp = thsp;
                ನಿಲ್ಲಿಸು;
            ಸ್ಥಿತಿ 'P':
                ಆದರೆ (nodep->isWide() && !ಬಳಸುತ್ತದೆ_ಸರತಿ) {
                    UASSERT_OBJ(m_wide_ತಾತ್ಕಾಲಿಕ_Refp, nodep,
                                "Wide op " << nodep->prettyTypeName()
                                           << " w/ no temp, perhaps missing op in V3EmitC?");
                    comma_ಹೊರಹೋಗು();
                    put_ಹೊರಹೋಗು();
                    ಆದರೆ (!m_wide_ತಾತ್ಕಾಲಿಕ_Refp->self_ಸೂಚಕ().is_ಖಾಲಿಗಳು()) {
                        emitDereference(m_wide_ತಾತ್ಕಾಲಿಕ_Refp,
                                        m_wide_ತಾತ್ಕಾಲಿಕ_Refp->self_ಸೂಚಕ_Protect(m_useSelfForThis));
                    }
                    ಹೊರಹೋಗು += varName(m_wide_ತಾತ್ಕಾಲಿಕ_Refp->varp());
                    m_wide_ತಾತ್ಕಾಲಿಕ_Refp = ಶೂನ್ಯಸೂಚಕ;
                    needComma = ನಿಜ;
                } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (ಬಳಸುತ್ತದೆ_ಸರತಿ) {
                    comma_ಹೊರಹೋಗು();
                    put_ಹೊರಹೋಗು();
                    iterate_And_ಮುಂದಿನ_Const_Null(nodep->backp()->op2p());
                    needComma = ನಿಜ;
                }

                ನಿಲ್ಲಿಸು;
            ಪೂರ್ವನಿಯೋಜಿತ: nodep->v3fatalSrc("Unknown emitOperator format code: %" << pos[0]); ನಿಲ್ಲಿಸು;
            }
            ಆದರೆ (detail) {
                // Get next letter of %[nlrt]
                ++pos;
                ಆಯ್ಕೆ (pos[0]) {
                ಸ್ಥಿತಿ 'q':
                    put_ಹೊರಹೋಗು();
                    // If we are assigning this to a queue we need to get the return type
                    ಆದರೆ (VN_IS(detailp->backp(), ನಿಯೋಜಿಸು)
                        && VN_IS(detailp->backp()->op2p()->dtypep()->skipRefp(), ಸರತಿ_DType)) {
                        puts("R");  // R for queue
                        ಬಳಸುತ್ತದೆ_ಸರತಿ = ನಿಜ;
                    } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (VN_IS(detailp->dtypep()->skipRefp(), ಸರತಿ_DType)
                               || VN_IS(detailp->dtypep()->skipRefp(), ಹರಿವು_DType)) {
                        puts("R");  // R for queue
                    } ಇಲ್ಲದಿದ್ದರೆ {
                        emitIQW(detailp);
                    }
                    ನಿಲ್ಲಿಸು;
                ಸ್ಥಿತಿ 'w':
                    comma_ಹೊರಹೋಗು();
                    ಹೊರಹೋಗು += cvt_ಗಮ್ಯ_Str(detailp->ಅಗಲ_Min());
                    needComma = ನಿಜ;
                    ನಿಲ್ಲಿಸು;
                ಸ್ಥಿತಿ 'W':
                    ಆದರೆ (lhsp->isWide()) {
                        comma_ಹೊರಹೋಗು();
                        ಹೊರಹೋಗು += cvt_ಗಮ್ಯ_Str(lhsp->ಅಗಲ_ಪದಗಳು());
                        needComma = ನಿಜ;
                    } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (VN_IS(lhsp, ಹರಿವು_R)) {
                        comma_ಹೊರಹೋಗು();
                        ಹೊರಹೋಗು += cvt_ಗಮ್ಯ_Str(rhsp->ಅಗಲ_ಪದಗಳು());
                        needComma = ನಿಜ;
                    }
                    ನಿಲ್ಲಿಸು;
                ಸ್ಥಿತಿ 'i':
                    comma_ಹೊರಹೋಗು();
                    UASSERT_OBJ(detailp, nodep, "emitOperator() references undef node");
                    put_ಹೊರಹೋಗು();
                    iterate_And_ಮುಂದಿನ_Const_Null(detailp);
                    needComma = ನಿಜ;
                    ನಿಲ್ಲಿಸು;
                ಪೂರ್ವನಿಯೋಜಿತ:
                    nodep->v3fatalSrc("Unknown emitOperator format code: %[nlrt]" << pos[0]);
                    ನಿಲ್ಲಿಸು;
                }
            }
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (pos[0] == ')') {
            ಮುಂದಿನ_Comma = "";
            ಹೊರಹೋಗು += ')';
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (pos[0] == '(') {
            comma_ಹೊರಹೋಗು();
            needComma = ಸುಳ್ಳು;
            ಹೊರಹೋಗು += '(';
        } ಇಲ್ಲದಿದ್ದರೆ {
            // Normal text
            ಆದರೆ (std::ಅಕ್ಷರವೇ_ಅಂಕಿಯೇ(pos[0])) needComma = ನಿಜ;
            comma_ಹೊರಹೋಗು();
            ಹೊರಹೋಗು += pos[0];
        }
    }
    put_ಹೊರಹೋಗು();
}

ತಾರ್ಕಿಕ EmitCFunc::display_Emit_ಶೀರ್ಷಿಕೆ(Ast_ಗಂಟು* nodep) {
    ತಾರ್ಕಿಕ isStmt = ಸುಳ್ಳು;
    ಆದರೆ (ಸ್ಥಿರಾಂಕ AstFScanF* ಸ್ಥಿರಾಂಕ dispp = VN_CAST(nodep, FScanF)) {
        isStmt = ಸುಳ್ಳು;
        putns(nodep, "VL_FSCANF_INX(");
        iterateConst(dispp->filep());
        puts(",");
    } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (ಸ್ಥಿರಾಂಕ AstSScanF* ಸ್ಥಿರಾಂಕ dispp = VN_CAST(nodep, SScanF)) {
        isStmt = ಸುಳ್ಳು;
        putns(nodep, "VL_SSCANF_I");
        emitIQW(dispp->fromp());
        puts("NX(");
        puts(cvt_ಗಮ್ಯ_Str(dispp->fromp()->ಅಗಲ_Min()));
        puts(",");
        iterateConst(dispp->fromp());
        puts(",");
    } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (ಸ್ಥಿರಾಂಕ AstDisplay* ಸ್ಥಿರಾಂಕ dispp = VN_CAST(nodep, Display)) {
        isStmt = ನಿಜ;
        ಆದರೆ (dispp->filep()) {
            putns(nodep, "VL_FWRITEF_NX(");
            iterateConst(dispp->filep());
            puts(",");
        } ಇಲ್ಲದಿದ್ದರೆ {
            putns(nodep, "VL_WRITEF_NX(");
        }
    } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (ಸ್ಥಿರಾಂಕ AstSFormat* ಸ್ಥಿರಾಂಕ dispp = VN_CAST(nodep, SFormat)) {
        isStmt = ನಿಜ;
        puts("VL_SFORMAT_NX(");
        ಆದರೆ (!dispp->lhsp()->dtypep()->isString()) {
            puts(cvt_ಗಮ್ಯ_Str(dispp->lhsp()->ಅಗಲ_Min()));
            putbs(",");
        }
        iterateConst(dispp->lhsp());
        emitDatap(dispp->lhsp());
        putbs(",");
    } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (VN_IS(nodep, SFormatF)) {
        isStmt = ಸುಳ್ಳು;
        putns(nodep, "VL_SFORMATF_N_NX(");
    } ಇಲ್ಲದಿದ್ದರೆ {
        nodep->v3fatalSrc("Unknown displayEmit node type");
    }
    ಹಿಂತಿರುಗಿ isStmt;
}

ಶೂನ್ಯ EmitCFunc::display_ಗಂಟು(Ast_ಗಂಟು* nodep, AstSFormatF* fmtp,  // fmtp is nullptr for AstScan
                            ಸ್ಥಿರಾಂಕ string& vformat, Ast_ಗಂಟು* exprsp, ತಾರ್ಕಿಕ is_ಪರಿಶೀಲನೆ) {
    // Check format, if it exists
    ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ ಅಭಿವ್ಯಕ್ತಿ_Format_2 = fmtp && fmtp->ಅಭಿವ್ಯಕ್ತಿ_Format_2();
    ತಾರ್ಕಿಕ needsScope = ಅಭಿವ್ಯಕ್ತಿ_Format_2;
    ತಾರ್ಕಿಕ needsTimescale = ಅಭಿವ್ಯಕ್ತಿ_Format_2;
    ಪೂರ್ಣಾಂಕ format_ವಾದಗಳು = 0;
    ಆದರೆ (!ಅಭಿವ್ಯಕ್ತಿ_Format_2) {
        // Check display and argument count to head-off later runtime issues
        ತಾರ್ಕಿಕ ಒಳ_Pct = ಸುಳ್ಳು;
        ತಾರ್ಕಿಕ ignore = ಸುಳ್ಳು;
        ಗಾಗಿ (ಅಕ್ಷರ ch : vformat) {
            // UINFO(1, "Parse '" << *pos << "'  IP" << inPct << " List " << cvtToHex(elistp));
            ಆದರೆ (!ಒಳ_Pct && ch == '%') {
                ಒಳ_Pct = ನಿಜ;
                ignore = ಸುಳ್ಳು;
            } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (ಒಳ_Pct && (std::ಅಂಕಿಯೇ(ch) || ch == '.' || ch == '-')) {
            } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (!ಒಳ_Pct) {  // Normal text
            } ಇಲ್ಲದಿದ್ದರೆ {  // Format character
                ಒಳ_Pct = ಸುಳ್ಳು;
                ch = std::tolower(ch);
                ಆಯ್ಕೆ (ch) {
                ಸ್ಥಿತಿ '%': ನಿಲ್ಲಿಸು;
                ಸ್ಥಿತಿ '*':
                    ಒಳ_Pct = ನಿಜ;  // Get more digits
                    ignore = ನಿಜ;
                    ನಿಲ್ಲಿಸು;
                ಸ್ಥಿತಿ 'm': needsScope = ನಿಜ; ನಿಲ್ಲಿಸು;
                ಸ್ಥಿತಿ 't':
                    needsTimescale = ನಿಜ;
                    ++format_ವಾದಗಳು;
                    ನಿಲ್ಲಿಸು;
                ಪೂರ್ವನಿಯೋಜಿತ:
                    ಆದರೆ (!ignore && V3_ಸಂಖ್ಯೆ::displayedFmtHasArg(ch, is_ಪರಿಶೀಲನೆ)) ++format_ವಾದಗಳು;
                    ನಿಲ್ಲಿಸು;
                }
            }
        }
    }

    ಆದರೆ (vformat.ಖಾಲಿಗಳು() && VN_IS(nodep, Display))  // not fscanf etc, as they need to return value
        ಹಿಂತಿರುಗಿ;  // NOP

    ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ isStmt = display_Emit_ಶೀರ್ಷಿಕೆ(nodep);

    ಆದರೆ (ಅಭಿವ್ಯಕ್ತಿ_Format_2) {
        UASSERT_OBJ(exprsp, nodep, "Missing format expression");
        iterateConst(exprsp);
        emitDatap(exprsp);
        exprsp = exprsp->nextp();
    } ಇಲ್ಲದಿದ್ದರೆ {
        ofp()->putsQuoted(vformat);
    }

    ಪೂರ್ಣಾಂಕ ಅಭಿವ್ಯಕ್ತಿ_ವಾದಗಳು = 0;
    ಗಾಗಿ (Ast_ಗಂಟು* argp = exprsp; argp; argp = argp->nextp()) ++ಅಭಿವ್ಯಕ್ತಿ_ವಾದಗಳು;
    ಆದರೆ (VL_UNCOVERABLE(!ಅಭಿವ್ಯಕ್ತಿ_Format_2 && ಅಭಿವ್ಯಕ್ತಿ_ವಾದಗಳು != format_ವಾದಗಳು)) {  // LCOV_EXCL_START
        // expectDisplay() checks this first, so probably internal error if found here
        nodep->v3error("Internal: Missing or extra arguments for $display-like format, "
                       << " expression had " << ಅಭಿವ್ಯಕ್ತಿ_ವಾದಗಳು << ", format had " << format_ವಾದಗಳು);
    }  // LCOV_EXCL_STOP

    // argc for error check. Also MSVC++ requires va_args to not be off a reference
    ಪೂರ್ಣಾಂಕ argc = 0;
    ಆದರೆ (needsScope) ++argc;
    ಆದರೆ (needsTimescale) ++argc;
    ಗಾಗಿ (Ast_ಗಂಟು* argp = exprsp; argp; argp = argp->nextp()) {
        ++argc;
        ಸ್ಥಿರಾಂಕ AstSFormatArg* ಸ್ಥಿರಾಂಕ fargp = VN_CAST(argp, SFormatArg);
        ಆದರೆ (fargp && fargp->formatAttr().isEnum()) ++argc;  // Additional name argument
    }
    ofp()->puts("," + std::ಗಮ್ಯ_string(argc));

    ಆದರೆ (needsScope) {
        AstScopeName* ಸ್ಥಿರಾಂಕ scopenamep = fmtp ? fmtp->scopeNamep() : ಶೂನ್ಯಸೂಚಕ;
        UASSERT_OBJ(scopenamep, nodep, "Display with %m but no AstScopeName");
        ofp()->puts(", '"s + VFormatAttr{VFormatAttr::SCOPE}.ascii() + "'");
        ಆದರೆ (scopenamep->selfRelative()) {
            // vlNamep is catName(name(), instance), so starts with '.' when the model is unnamed
            ಸ್ಥಿರಾಂಕ string namep = string{m_useSelfForThis ? "vlSelf" : "this"} + "->vlNamep";
            ofp()->puts(",(vlSymsp->name()[0] ? " + namep + " : " + namep + " + 1),");
            ofp()->putsQuoted(scopenamep->scopeAttr());
        } ಇಲ್ಲದಿದ್ದರೆ {
            ofp()->puts(",vlSymsp->name(),");
            ofp()->putsQuoted(scopenamep->scopePrettySymName());
        }
    }

    ಆದರೆ (needsTimescale) {
        VTimescale timeunit = VTimescale::NONE;
        ಆದರೆ (ಸ್ಥಿರಾಂಕ AstDisplay* ಸ್ಥಿರಾಂಕ anodep = VN_CAST(nodep, Display)) {
            timeunit = anodep->fmtp()->timeunit();
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (ಸ್ಥಿರಾಂಕ AstSFormat* ಸ್ಥಿರಾಂಕ anodep = VN_CAST(nodep, SFormat)) {
            timeunit = anodep->fmtp()->timeunit();
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (ಸ್ಥಿರಾಂಕ AstSScanF* ಸ್ಥಿರಾಂಕ anodep = VN_CAST(nodep, SScanF)) {
            timeunit = anodep->timeunit();
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (ಸ್ಥಿರಾಂಕ AstSFormatF* ಸ್ಥಿರಾಂಕ anodep = VN_CAST(nodep, SFormatF)) {
            timeunit = anodep->timeunit();
        }
        UASSERT_OBJ(!timeunit.isNone(), nodep,
                    "Use of %t must be under AstDisplay, AstSFormat, or AstSFormatF, or "
                    "SScanF, and timeunit set");
        ofp()->puts(", '"s + VFormatAttr{VFormatAttr::TIMEUNIT}.ascii() + "'");
        ofp()->puts(","s + cvt_ಗಮ್ಯ_Str((ಪೂರ್ಣಾಂಕ)timeunit.power_ದ_Ten()));
    }

    // Arguments
    ಗಾಗಿ (Ast_ಗಂಟು* argp = exprsp; argp; argp = argp->nextp()) {
        ofp()->indentInc();
        ofp()->putbs("");
        AstSFormatArg* ಸ್ಥಿರಾಂಕ fargp = VN_CAST(argp, SFormatArg);
        Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ subargp = fargp ? fargp->exprp() : argp;
        ಸ್ಥಿರಾಂಕ VFormatAttr formatAttr = AstSFormatArg::formatAttrDefauled(fargp, subargp->dtypep());
        puts(", '"s + formatAttr.ascii() + '\'');
        ಆದರೆ (formatAttr.isSigned() || formatAttr.isUnsigned() || formatAttr.isEnum())
            puts("," + cvt_ಗಮ್ಯ_Str(subargp->ಅಗಲ_Min()));
        ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ addrof = is_ಪರಿಶೀಲನೆ || formatAttr.isString() || formatAttr.isComplex();
        ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ wideEnum = formatAttr.isEnum() && subargp->isWide();
        puts(",");
        ಆದರೆ (wideEnum) puts("static_cast<const EData*>(");
        ಆದರೆ (addrof) puts("&(");
        ಆದರೆ (VN_IS(subargp, ಹರಿವು_R))
            emit_ಹರಿವು_R(
                VN_CAST(subargp, ಹರಿವು_R),
                nodep);  // This has to be done here because streamR doesn't know what it returns
        ಇಲ್ಲದಿದ್ದರೆ { iterateConst(subargp); }
        ಆದರೆ (addrof) puts(")");
        ಆದರೆ (!addrof) emitDatap(argp);
        ಆದರೆ (wideEnum) puts(")");
        ಆದರೆ (formatAttr.isEnum()) {
            puts(", '"s + VFormatAttr{VFormatAttr::STRING}.ascii() + "', &(");
            iterateConst(fargp->namep());
            puts(")");
        }
        ofp()->indentDec();
    }

    // End
    ಆದರೆ (isStmt) {
        puts(");\n");
    } ಇಲ್ಲದಿದ್ದರೆ {
        puts(") ");
    }
}

ಶೂನ್ಯ EmitCFunc::emit_CCall_ವಾದಗಳು(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_CCall* nodep, ಸ್ಥಿರಾಂಕ string& self_ಸೂಚಕ,
                              ತಾರ್ಕಿಕ ಒಳ_Process) {
    putns(nodep, "(");
    ತಾರ್ಕಿಕ comma = ಸುಳ್ಳು;
    ಆದರೆ (nodep->funcp()->isLoose() && !nodep->funcp()->isStatic()) {
        UASSERT_OBJ(!self_ಸೂಚಕ.ಖಾಲಿಗಳು(), nodep, "Call to loose method without self pointer");
        puts(self_ಸೂಚಕ);
        comma = ನಿಜ;
    }
    ಆದರೆ (nodep->funcp()->needProcess()) {
        ಆದರೆ (comma) puts(", ");
        ಆದರೆ (VN_IS(nodep->backp(), CAwait) || !nodep->funcp()->isCoroutine()) {
            puts("vlProcess");
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (ಒಳ_Process) {
            puts("std::make_shared<VlProcess>(vlProcess)");
        } ಇಲ್ಲದಿದ್ದರೆ {
            puts("std::make_shared<VlProcess>()");
        }
        comma = ನಿಜ;
    }
    ಆದರೆ (!nodep->argTypes().ಖಾಲಿಗಳು()) {
        ಆದರೆ (comma) puts(", ");
        puts(nodep->argTypes());
        comma = ನಿಜ;
    }
    put_Comma_Iterate_ಮುಂದಿನ(nodep->argsp(), comma);
    puts(")");
}

std::string EmitCFunc::dereferenceString(ಸ್ಥಿರಾಂಕ std::string& ಸೂಚಕ) ಸ್ಥಿರಾಂಕ {
    ಆದರೆ (ಸೂಚಕ[0] == '(' && ಸೂಚಕ[1] == '&') {
        // remove "address of" followed by immediate dereference
        // Note: this relies on only the form '(&OBJECT)' being used by Verilator
        ಹಿಂತಿರುಗಿ ಸೂಚಕ.substr(2, ಸೂಚಕ.ಉದ್ದಳತೆ() - 3) + '.';
    } ಇಲ್ಲದಿದ್ದರೆ {
        ಆದರೆ (ಸೂಚಕ == "vlSelf" && m_usevlSelfRef) {
            ಹಿಂತಿರುಗಿ "vlSelfRef.";
        } ಇಲ್ಲದಿದ್ದರೆ {
            ಹಿಂತಿರುಗಿ ಸೂಚಕ + "->";
        }
    }
}
ಶೂನ್ಯ EmitCFunc::emitDereference(Ast_ಗಂಟು* nodep, ಸ್ಥಿರಾಂಕ string& ಸೂಚಕ) {
    putns(nodep, dereferenceString(ಸೂಚಕ));
}

ಶೂನ್ಯ EmitCFunc::emitCvtPackStr(Ast_ಗಂಟು* nodep) {
    ಆದರೆ (ಸ್ಥಿರಾಂಕ AstConst* ಸ್ಥಿರಾಂಕ constp = VN_CAST(nodep, Const)) {
        emitConstantString(constp);
    } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (VN_IS(nodep->dtypep(), ಹರಿವು_DType)) {
        putnbs(nodep, "VL_CVT_PACK_STR_ND(");
        iterate_And_ಮುಂದಿನ_Const_Null(nodep);
        puts(")");
    } ಇಲ್ಲದಿದ್ದರೆ {
        putnbs(nodep, "VL_CVT_PACK_STR_N");
        emitIQW(nodep);
        puts("(");
        ಆದರೆ (nodep->isWide()) {
            // Note argument width, not node width (which is always 32)
            puts(cvt_ಗಮ್ಯ_Str(nodep->ಅಗಲ_ಪದಗಳು()));
            puts(", ");
        }
        iterate_And_ಮುಂದಿನ_Const_Null(nodep);
        puts(")");
    }
}

ಶೂನ್ಯ EmitCFunc::emitConstant(AstConst* nodep) {
    // Put out constant set to the specified variable, or given variable in a string
    ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& ಅಂಕೆ = nodep->ಅಂಕೆ();
    ಆದರೆ (ಅಂಕೆ.isFourState()) {
        nodep->v3warn(E_UNSUPPORTED, "Unsupported: 4-state numbers in this context");
        ಹಿಂತಿರುಗಿ;
    }
    putns(nodep, ಅಂಕೆ.emitC());
}

ಶೂನ್ಯ EmitCFunc::emitConstantString(ಸ್ಥಿರಾಂಕ AstConst* nodep) {
    // Const might be a Verilog array-type string, but need to always output std::string
    putnbs(nodep, "std::string{");
    ಸ್ಥಿರಾಂಕ string str = nodep->ಅಂಕೆ().ಗಮ್ಯ_String();
    ಆದರೆ (!str.ಖಾಲಿಗಳು()) putsQuoted(str);
    puts("}");
}

ಶೂನ್ಯ EmitCFunc::emitVarReset(ಸ್ಥಿರಾಂಕ string& ಪೂರ್ವಪ್ರತ್ಯಯ, AstVar* varp, ತಾರ್ಕಿಕ constructing) {
    // 'constructing' indicates that the object was just constructed, so if it is a string or
    // something that starts off clear already, no need to clear it again
    Ast_ಗಂಟು_DType* ಸ್ಥಿರಾಂಕ dtypep = varp->dtypep()->skipRefp();
    ಸ್ಥಿರಾಂಕ string vlSelf = VSelf_ಸೂಚಕ_ಪಠ್ಯ::replaceThis(m_useSelfForThis, "this->");
    ಸ್ಥಿರಾಂಕ string varNameProtected
        = ((VN_IS(m_modp, Class) || varp->isFuncLocal()) || !ಪೂರ್ವಪ್ರತ್ಯಯ.ಖಾಲಿಗಳು())
              ? varName(varp)
              : vlSelf + varName(varp);
    ಸ್ಥಿರಾಂಕ string new_ಪೂರ್ವಪ್ರತ್ಯಯ = ಪೂರ್ವಪ್ರತ್ಯಯ + varNameProtected;
    ಆದರೆ (varp->isIO() && m_modp->is_ತುದಿ() && optSystemC()) {
        // System C top I/O doesn't need loading, as the lower level subinst code does it.}
    } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (varp->isParam()) {
        UASSERT_OBJ(varp->valuep(), varp, "No init for a param?");
        // If a simple CONST value we initialize it using an enum
        // If an ARRAYINIT we initialize it using an initial block similar to a signal
        // puts("// parameter "+varp->nameProtect()+" = "+varp->valuep()->name()+"\n");
    } ಇಲ್ಲದಿದ್ದರೆ {
        putns(varp,
              emitVarResetRecurse(varp, constructing, new_ಪೂರ್ವಪ್ರತ್ಯಯ, dtypep, 0, "", varp->valuep()));
    }
}

string EmitCFunc::emitVarResetRecurse(ಸ್ಥಿರಾಂಕ AstVar* varp, ತಾರ್ಕಿಕ constructing,
                                      ಸ್ಥಿರಾಂಕ string& varNameProtected, Ast_ಗಂಟು_DType* dtypep,
                                      ಪೂರ್ಣಾಂಕ ಆಳ, ಸ್ಥಿರಾಂಕ string& suffix, ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* valuep) {
    dtypep = dtypep->skipRefp();
    AstBasicDType* ಸ್ಥಿರಾಂಕ basicp = dtypep->basicp();
    // Returns string to do resetting, empty to do nothing (which caller should handle)
    ಆದರೆ (AstAssocArrayDType* ಸ್ಥಿರಾಂಕ adtypep = VN_CAST(dtypep, AssocArrayDType)) {
        // Access std::array as C array
        ಸ್ಥಿರಾಂಕ string pre = constructing ? "" : varNameProtected + suffix + ".clear();\n";
        ಹಿಂತಿರುಗಿ pre
               + emitVarResetRecurse(varp, constructing, varNameProtected, adtypep->subDTypep(),
                                     ಆಳ + 1, suffix + ".atDefault()", ಶೂನ್ಯಸೂಚಕ);
    } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (AstWildcardArrayDType* ಸ್ಥಿರಾಂಕ adtypep = VN_CAST(dtypep, WildcardArrayDType)) {
        // Access std::array as C array
        ಸ್ಥಿರಾಂಕ string pre = constructing ? "" : varNameProtected + suffix + ".clear();\n";
        ಹಿಂತಿರುಗಿ pre
               + emitVarResetRecurse(varp, constructing, varNameProtected, adtypep->subDTypep(),
                                     ಆಳ + 1, suffix + ".atDefault()", ಶೂನ್ಯಸೂಚಕ);
    } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (VN_IS(dtypep, CDType)) {
        ಹಿಂತಿರುಗಿ "";  // Constructor does it
    } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (VN_IS(dtypep, CoverCrossDType) || VN_IS(dtypep, CoverpointDType)) {
        ಹಿಂತಿರುಗಿ "";  // Covergroup constructor creates the runtime and assigns the pointer
    } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (ಸ್ಥಿರಾಂಕ AstClassRefDType* ಸ್ಥಿರಾಂಕ adtypep = VN_CAST(dtypep, ClassRefDType)) {
        ಹಿಂತಿರುಗಿ adtypep->raw_ಸೂಚಕ() ? varNameProtected + suffix + " = nullptr;\n" : "";
    } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (VN_IS(dtypep, IfaceRefDType)) {
        ಹಿಂತಿರುಗಿ varNameProtected + suffix + " = nullptr;\n";
    } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (ಸ್ಥಿರಾಂಕ AstDynArrayDType* ಸ್ಥಿರಾಂಕ adtypep = VN_CAST(dtypep, DynArrayDType)) {
        ಸ್ಥಿರಾಂಕ string pre = constructing ? "" : varNameProtected + suffix + ".clear();\n";
        ಹಿಂತಿರುಗಿ pre
               + emitVarResetRecurse(varp, constructing, varNameProtected, adtypep->subDTypep(),
                                     ಆಳ + 1, suffix + ".atDefault()", ಶೂನ್ಯಸೂಚಕ);
    } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (ಸ್ಥಿರಾಂಕ Ast_ಸರತಿ_DType* ಸ್ಥಿರಾಂಕ adtypep = VN_CAST(dtypep, ಸರತಿ_DType)) {
        ಸ್ಥಿರಾಂಕ string pre = constructing ? "" : varNameProtected + suffix + ".clear();\n";
        ಹಿಂತಿರುಗಿ pre
               + emitVarResetRecurse(varp, constructing, varNameProtected, adtypep->subDTypep(),
                                     ಆಳ + 1, suffix + ".atDefault()", ಶೂನ್ಯಸೂಚಕ);
    } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (VN_IS(dtypep, Sample_ಸರತಿ_DType)) {
        ಹಿಂತಿರುಗಿ "";
    } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (ಸ್ಥಿರಾಂಕ AstUnpackArrayDType* ಸ್ಥಿರಾಂಕ adtypep = VN_CAST(dtypep, UnpackArrayDType)) {
        UASSERT_OBJ(adtypep->hi() >= adtypep->lo(), varp,
                    "Should have swapped msb & lsb earlier.");
        ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_DType* ಸ್ಥಿರಾಂಕ elementp = adtypep->subDTypep()->skipRefp();
        ಆದರೆ (!varp->isFuncLocal() && elementp->isIntegralOrPacked()) {
            // Scoped reset has no array-index input or mutable RNG state. Initialize one
            // element, then fill in the optimized runtime even when this code is cold.
            ಸ್ಥಿರಾಂಕ string ಮೊದಲ
                = emitVarResetRecurse(varp, constructing, varNameProtected, adtypep->subDTypep(),
                                      ಆಳ + 1, suffix + "[0]", ಶೂನ್ಯಸೂಚಕ);
            ಸ್ಥಿರಾಂಕ string array = varNameProtected + suffix;
            ಹಿಂತಿರುಗಿ ಮೊದಲ + "VL_UNPACKED_FILL(" + array + ".data(), sizeof(" + array + "[0]), "
                   + cvt_ಗಮ್ಯ_Str(adtypep->elementsConst()) + ");\n";
        }
        ಸ್ಥಿರಾಂಕ string ivar = "__Vi"s + cvt_ಗಮ್ಯ_Str(ಆಳ);
        ಸ್ಥಿರಾಂಕ string pre = ("for (int " + ivar + " = " + cvt_ಗಮ್ಯ_Str(0) + "; " + ivar + " < "
                            + cvt_ಗಮ್ಯ_Str(adtypep->elementsConst()) + "; ++" + ivar + ") {\n");
        ಸ್ಥಿರಾಂಕ string ಕೆಳಗೆ
            = emitVarResetRecurse(varp, constructing, varNameProtected, adtypep->subDTypep(),
                                  ಆಳ + 1, suffix + "[" + ivar + "]", ಶೂನ್ಯಸೂಚಕ);
        ಸ್ಥಿರಾಂಕ string post = "}\n";
        ಹಿಂತಿರುಗಿ ಕೆಳಗೆ.ಖಾಲಿಗಳು() ? "" : pre + ಕೆಳಗೆ + post;
    } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (VN_IS(dtypep, ಗಂಟು_UOr_Struct_DType) && !VN_AS(dtypep, ಗಂಟು_UOr_Struct_DType)->packed()) {
        ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_UOr_Struct_DType* ಸ್ಥಿರಾಂಕ sdtypep = VN_AS(dtypep, ಗಂಟು_UOr_Struct_DType);
        string literal;
        ಗಾಗಿ (ಸ್ಥಿರಾಂಕ AstMemberDType* itemp = sdtypep->membersp(); itemp;
             itemp = VN_AS(itemp->nextp(), MemberDType)) {
            ಸ್ಥಿರಾಂಕ std::string ಸಾಲು = emitVarResetRecurse(
                varp, constructing, varNameProtected + suffix + "." + itemp->nameProtect(),
                itemp->dtypep(), ಆಳ + 1, "", itemp->valuep());
            ಆದರೆ (!ಸಾಲು.ಖಾಲಿಗಳು()) literal += ಸಾಲು;
        }
        ಹಿಂತಿರುಗಿ literal;
    } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (basicp && basicp->keyword() == VBasicDTypeKwd::STRING) {
        ಆದರೆ (constructing) ಹಿಂತಿರುಗಿ "";  // String's constructor deals with it
        ಹಿಂತಿರುಗಿ varNameProtected + suffix + ".clear();\n";
    } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (basicp && basicp->isForkSync()) {
        ಹಿಂತಿರುಗಿ "";
    } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (basicp && basicp->isProcessRef()) {
        ಹಿಂತಿರುಗಿ "";
    } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (basicp && basicp->isDelayScheduler()) {
        ಹಿಂತಿರುಗಿ "";
    } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (basicp && basicp->isTriggerScheduler()) {
        ಹಿಂತಿರುಗಿ "";
    } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (basicp && basicp->isDynamicTriggerScheduler()) {
        ಹಿಂತಿರುಗಿ "";
    } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (basicp && (basicp->isRandomGenerator() || basicp->isStdRandomGenerator())) {
        ಹಿಂತಿರುಗಿ "";
    } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (basicp && basicp->isCovergroupInstHandle()) {
        // The handle's own constructor deals with it.
        ಹಿಂತಿರುಗಿ "";
    } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (basicp && (basicp->isEvent())) {
        ಹಿಂತಿರುಗಿ "VlAssignableEvent{};\n";
    } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (basicp) {
        ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ zeroit
            = (varp->attrFileDescr()  // Zero so we don't do file IO if never $fopen
               || varp->isFuncLocal()  // Randomization too slow
               || (basicp && basicp->isZeroInit())
               || (v3Global.opt.underlineZero() && !varp->name().ಖಾಲಿಗಳು() && varp->name()[0] == '_')
               || (varp->varType().is_ತಾತ್ಕಾಲಿಕ() && !varp->isXTemp())
               || (varp->isXTemp()
                       ? (v3Global.opt.x_ನಿಯೋಜಿಸು() != "unique")
                       : (v3Global.opt.xInitial() == "fast" || v3Global.opt.xInitial() == "0")));
        ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ slow = !varp->isFuncLocal() && !varp->isClassMember();
        ವಿಭಜಿಸು_ಗಾತ್ರ_Inc(1);
        ಆದರೆ (dtypep->isWide()) {  // Handle unpacked; not basicp->isWide
            string ಹೊರಹೋಗು;
            ಆದರೆ (valuep) {
                ಸ್ಥಿರಾಂಕ AstConst* ಸ್ಥಿರಾಂಕ constp = VN_AS(valuep, Const);
                UASSERT_OBJ(constp, varp, "non-const initializer for variable");
                ಗಾಗಿ (ಪೂರ್ಣಾಂಕ w = 0; w < dtypep->ಅಗಲ_ಪದಗಳು(); ++w) {
                    ಹೊರಹೋಗು += varNameProtected + suffix + "[" + cvt_ಗಮ್ಯ_Str(w) + "] = ";
                    ಹೊರಹೋಗು += cvt_ಗಮ್ಯ_Str(constp->ಅಂಕೆ().edata_ಶಬ್ದ(w)) + "U;\n";
                }
            } ಇಲ್ಲದಿದ್ದರೆ {
                ಹೊರಹೋಗು += zeroit ? (slow ? "VL_ZERO_RESET_W(" : "VL_ZERO_W(")
                              : (varp->isXTemp() ? "VL_SCOPED_RAND_RESET_ASSIGN_W("
                                                 : "VL_SCOPED_RAND_RESET_W(");
                ಹೊರಹೋಗು += cvt_ಗಮ್ಯ_Str(dtypep->ಅಗಲ_Min());
                ಹೊರಹೋಗು += ", " + varNameProtected + suffix;
                ಆದರೆ (!zeroit) {
                    emitVarResetScopeHash();
                    ಸ್ಥಿರಾಂಕ uint64_t salt = VString::hashMurmur(varp->prettyName());
                    ಹೊರಹೋಗು += ", ";
                    ಹೊರಹೋಗು += m_classOrPackage ? m_classOrPackageHash : "__VscopeHash";
                    ಹೊರಹೋಗು += ", ";
                    ಹೊರಹೋಗು += std::ಗಮ್ಯ_string(salt);
                    ಹೊರಹೋಗು += "ull";
                }
                ಹೊರಹೋಗು += ");\n";
            }
            ಹಿಂತಿರುಗಿ ಹೊರಹೋಗು;
        } ಇಲ್ಲದಿದ್ದರೆ {
            string ಹೊರಹೋಗು = varNameProtected + suffix;
            ಆದರೆ (valuep) {
                ಹೊರಹೋಗು += " = ";
                // TODO cleanup code shared between here, V3EmitCConstInit.h,
                // EmitCFunc::emitVarReset, EmitCFunc::emitConstant
                ಸ್ಥಿರಾಂಕ AstConst* ಸ್ಥಿರಾಂಕ constp = VN_AS(valuep, Const);
                UASSERT_OBJ(constp, varp, "non-const initializer for variable");
                ಹೊರಹೋಗು += cvt_ಗಮ್ಯ_Str(constp->ಅಂಕೆ().edata_ಶಬ್ದ(0)) + "U;\n";
                ಹೊರಹೋಗು += ";\n";
            } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (zeroit) {
                ಹೊರಹೋಗು += " = 0;\n";
            } ಇಲ್ಲದಿದ್ದರೆ {
                emitVarResetScopeHash();
                ಸ್ಥಿರಾಂಕ uint64_t salt = VString::hashMurmur(varp->prettyName());
                ಹೊರಹೋಗು += " = VL_SCOPED_RAND_RESET_";
                ಆದರೆ (varp->isXTemp()) ಹೊರಹೋಗು += "ASSIGN_";
                ಹೊರಹೋಗು += dtypep->charIQWN();
                ಹೊರಹೋಗು += "(" + cvt_ಗಮ್ಯ_Str(dtypep->ಅಗಲ_Min()) + ", "
                       + (m_classOrPackage ? m_classOrPackageHash : "__VscopeHash") + ", "
                       + std::ಗಮ್ಯ_string(salt) + "ull);\n";
            }
            ಹಿಂತಿರುಗಿ ಹೊರಹೋಗು;
        }
    } ಇಲ್ಲದಿದ್ದರೆ {  // LCOV_EXCL_BR_LINE
        v3fatalSrc("Unknown node type in reset generator: " << varp->prettyTypeName());
    }
    ಹಿಂತಿರುಗಿ "";
}

ಶೂನ್ಯ EmitCFunc::emitVarResetScopeHash() {
    ಆದರೆ (VL_LIKELY(m_createdScopeHash)) { ಹಿಂತಿರುಗಿ; }
    ಆದರೆ (m_classOrPackage) {
        m_classOrPackageHash
            = std::ಗಮ್ಯ_string(VString::hashMurmur(m_classOrPackage->name())) + "ULL";
    } ಇಲ್ಲದಿದ್ದರೆ {
        puts(string("const uint64_t __VscopeHash = VL_MURMUR64_HASH(")
             + (m_useSelfForThis ? "vlSelf" : "this") + "->vlNamep);\n");
    }
    m_createdScopeHash = ನಿಜ;
}
