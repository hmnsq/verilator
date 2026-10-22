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

#ਅਨ੍ਤਰ੍ਭਾਵ "V3PchAstMT.h"

#ਅਨ੍ਤਰ੍ਭਾਵ "V3EmitCFunc.h"

#ਅਨ੍ਤਰ੍ਭਾਵ <map>
#ਅਨ੍ਤਰ੍ਭਾਵ <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// EmitCFunc

ਤਾਰਕਿਕ EmitCFunc::emitSimpleOk(Ast_ਗੰਢ_ਵਿਅੰਜਕ* nodep) {
    // Can we put out a simple (A + B) instead of VL_ADD_III(A,B)?
    ਜੇ (nodep->emitSimpleOperator() == "") ਵਾਪਸ ਝੂਠ;
    ਜੇ (nodep->isWide()) ਵਾਪਸ ਝੂਠ;
    ਜੇ (nodep->op1p() && nodep->op1p()->isWide()) ਵਾਪਸ ਝੂਠ;
    ਜੇ (nodep->op2p() && nodep->op2p()->isWide()) ਵਾਪਸ ਝੂਠ;
    ਜੇ (nodep->op3p() && nodep->op3p()->isWide()) ਵਾਪਸ ਝੂਠ;
    ਵਾਪਸ ਸੱਚ;
}

ਖਾਲੀ EmitCFunc::emitOpName(Ast_ਗੰਢ* nodep, ਅਚਲ string& format, Ast_ਗੰਢ* lhsp, Ast_ਗੰਢ* rhsp,
                           Ast_ਗੰਢ* thsp) {
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
    string ਬਾਹਰਲਾ;
    putnbs(nodep, "");

    ਤਾਰਕਿਕ needComma = ਝੂਠ;
    ਤਾਰਕਿਕ ਵਰਤਦਾ_ਕਤਾਰ = ਝੂਠ;
    string ਅਗਲਾ_Comma;
    ਸਵੈਚਾਲਿਤ comma_ਬਾਹਰਲਾ = [&ਬਾਹਰਲਾ, &ਅਗਲਾ_Comma]() {
        ਜੇ (!ਅਗਲਾ_Comma.ਖਾਲੀ_ਥਾਵਾਂ()) {
            ਬਾਹਰਲਾ += ਅਗਲਾ_Comma;
            ਅਗਲਾ_Comma = "";
        }
    };

    ਸਵੈਚਾਲਿਤ put_ਬਾਹਰਲਾ = [ਇਹ, &ਬਾਹਰਲਾ]() {
        ਜੇ (!ਬਾਹਰਲਾ.ਖਾਲੀ_ਥਾਵਾਂ()) puts(ਬਾਹਰਲਾ);
        ਬਾਹਰਲਾ = "";
    };

    ਲਈ (string::const_iterator pos = format.begin(); pos != format.ਸਿਰਾ(); ++pos) {
        ਜੇ (pos[0] == ',') {
            // Remember we need to add one, but don't do yet to avoid ",)"
            ਜੇ (needComma) {
                ਜੇ (pos[1] == ' ') {
                    ਅਗਲਾ_Comma = ", ";
                } ਨਹੀਂਤਾਂ {
                    ਅਗਲਾ_Comma = ",";
                }
                needComma = ਝੂਠ;
            }
            ਜੇ (pos[1] == ' ') ++pos;  // Must do even if no nextComma
        } ਨਹੀਂਤਾਂ ਜੇ (pos[0] == '%') {
            ++pos;
            ਤਾਰਕਿਕ detail = ਝੂਠ;
            Ast_ਗੰਢ* detailp = ਖਾਲੀ_ਸੂਚਕ;
            ਚੋਣ (pos[0]) {
            ਹਾਲਤ '%': ਬਾਹਰਲਾ += '%'; ਰੋਕੋ;
            ਹਾਲਤ 'k':
                put_ਬਾਹਰਲਾ();
                putbs("");
                ਰੋਕੋ;
            ਹਾਲਤ 'n':
                detail = ਸੱਚ;
                detailp = nodep;
                ਰੋਕੋ;
            ਹਾਲਤ 'l':
                detail = ਸੱਚ;
                detailp = lhsp;
                ਰੋਕੋ;
            ਹਾਲਤ 'r':
                detail = ਸੱਚ;
                detailp = rhsp;
                ਰੋਕੋ;
            ਹਾਲਤ 't':
                detail = ਸੱਚ;
                detailp = thsp;
                ਰੋਕੋ;
            ਹਾਲਤ 'P':
                ਜੇ (nodep->isWide() && !ਵਰਤਦਾ_ਕਤਾਰ) {
                    UASSERT_OBJ(m_wide_ਆਰਜ਼ੀ_Refp, nodep,
                                "Wide op " << nodep->prettyTypeName()
                                           << " w/ no temp, perhaps missing op in V3EmitC?");
                    comma_ਬਾਹਰਲਾ();
                    put_ਬਾਹਰਲਾ();
                    ਜੇ (!m_wide_ਆਰਜ਼ੀ_Refp->self_ਸੂਚਕ().is_ਖਾਲੀ_ਥਾਵਾਂ()) {
                        emitDereference(m_wide_ਆਰਜ਼ੀ_Refp,
                                        m_wide_ਆਰਜ਼ੀ_Refp->self_ਸੂਚਕ_Protect(m_useSelfForThis));
                    }
                    ਬਾਹਰਲਾ += varName(m_wide_ਆਰਜ਼ੀ_Refp->varp());
                    m_wide_ਆਰਜ਼ੀ_Refp = ਖਾਲੀ_ਸੂਚਕ;
                    needComma = ਸੱਚ;
                } ਨਹੀਂਤਾਂ ਜੇ (ਵਰਤਦਾ_ਕਤਾਰ) {
                    comma_ਬਾਹਰਲਾ();
                    put_ਬਾਹਰਲਾ();
                    iterate_And_ਅਗਲਾ_Const_Null(nodep->backp()->op2p());
                    needComma = ਸੱਚ;
                }

                ਰੋਕੋ;
            ਮੂਲ: nodep->v3fatalSrc("Unknown emitOperator format code: %" << pos[0]); ਰੋਕੋ;
            }
            ਜੇ (detail) {
                // Get next letter of %[nlrt]
                ++pos;
                ਚੋਣ (pos[0]) {
                ਹਾਲਤ 'q':
                    put_ਬਾਹਰਲਾ();
                    // If we are assigning this to a queue we need to get the return type
                    ਜੇ (VN_IS(detailp->backp(), ਸੌਂਪੋ)
                        && VN_IS(detailp->backp()->op2p()->dtypep()->skipRefp(), ਕਤਾਰ_DType)) {
                        puts("R");  // R for queue
                        ਵਰਤਦਾ_ਕਤਾਰ = ਸੱਚ;
                    } ਨਹੀਂਤਾਂ ਜੇ (VN_IS(detailp->dtypep()->skipRefp(), ਕਤਾਰ_DType)
                               || VN_IS(detailp->dtypep()->skipRefp(), ਧਾਰਾ_DType)) {
                        puts("R");  // R for queue
                    } ਨਹੀਂਤਾਂ {
                        emitIQW(detailp);
                    }
                    ਰੋਕੋ;
                ਹਾਲਤ 'w':
                    comma_ਬਾਹਰਲਾ();
                    ਬਾਹਰਲਾ += cvt_ਮੰਜ਼ਿਲ_Str(detailp->ਚੌੜਾਈ_Min());
                    needComma = ਸੱਚ;
                    ਰੋਕੋ;
                ਹਾਲਤ 'W':
                    ਜੇ (lhsp->isWide()) {
                        comma_ਬਾਹਰਲਾ();
                        ਬਾਹਰਲਾ += cvt_ਮੰਜ਼ਿਲ_Str(lhsp->ਚੌੜਾਈ_ਸ਼ਬਦ());
                        needComma = ਸੱਚ;
                    } ਨਹੀਂਤਾਂ ਜੇ (VN_IS(lhsp, ਧਾਰਾ_R)) {
                        comma_ਬਾਹਰਲਾ();
                        ਬਾਹਰਲਾ += cvt_ਮੰਜ਼ਿਲ_Str(rhsp->ਚੌੜਾਈ_ਸ਼ਬਦ());
                        needComma = ਸੱਚ;
                    }
                    ਰੋਕੋ;
                ਹਾਲਤ 'i':
                    comma_ਬਾਹਰਲਾ();
                    UASSERT_OBJ(detailp, nodep, "emitOperator() references undef node");
                    put_ਬਾਹਰਲਾ();
                    iterate_And_ਅਗਲਾ_Const_Null(detailp);
                    needComma = ਸੱਚ;
                    ਰੋਕੋ;
                ਮੂਲ:
                    nodep->v3fatalSrc("Unknown emitOperator format code: %[nlrt]" << pos[0]);
                    ਰੋਕੋ;
                }
            }
        } ਨਹੀਂਤਾਂ ਜੇ (pos[0] == ')') {
            ਅਗਲਾ_Comma = "";
            ਬਾਹਰਲਾ += ')';
        } ਨਹੀਂਤਾਂ ਜੇ (pos[0] == '(') {
            comma_ਬਾਹਰਲਾ();
            needComma = ਝੂਠ;
            ਬਾਹਰਲਾ += '(';
        } ਨਹੀਂਤਾਂ {
            // Normal text
            ਜੇ (std::ਅੱਖਰ_ਜਾਂ_ਅੰਕ(pos[0])) needComma = ਸੱਚ;
            comma_ਬਾਹਰਲਾ();
            ਬਾਹਰਲਾ += pos[0];
        }
    }
    put_ਬਾਹਰਲਾ();
}

ਤਾਰਕਿਕ EmitCFunc::display_Emit_ਸਿਰਲੇਖ(Ast_ਗੰਢ* nodep) {
    ਤਾਰਕਿਕ isStmt = ਝੂਠ;
    ਜੇ (ਅਚਲ AstFScanF* ਅਚਲ dispp = VN_CAST(nodep, FScanF)) {
        isStmt = ਝੂਠ;
        putns(nodep, "VL_FSCANF_INX(");
        iterateConst(dispp->filep());
        puts(",");
    } ਨਹੀਂਤਾਂ ਜੇ (ਅਚਲ AstSScanF* ਅਚਲ dispp = VN_CAST(nodep, SScanF)) {
        isStmt = ਝੂਠ;
        putns(nodep, "VL_SSCANF_I");
        emitIQW(dispp->fromp());
        puts("NX(");
        puts(cvt_ਮੰਜ਼ਿਲ_Str(dispp->fromp()->ਚੌੜਾਈ_Min()));
        puts(",");
        iterateConst(dispp->fromp());
        puts(",");
    } ਨਹੀਂਤਾਂ ਜੇ (ਅਚਲ AstDisplay* ਅਚਲ dispp = VN_CAST(nodep, Display)) {
        isStmt = ਸੱਚ;
        ਜੇ (dispp->filep()) {
            putns(nodep, "VL_FWRITEF_NX(");
            iterateConst(dispp->filep());
            puts(",");
        } ਨਹੀਂਤਾਂ {
            putns(nodep, "VL_WRITEF_NX(");
        }
    } ਨਹੀਂਤਾਂ ਜੇ (ਅਚਲ AstSFormat* ਅਚਲ dispp = VN_CAST(nodep, SFormat)) {
        isStmt = ਸੱਚ;
        puts("VL_SFORMAT_NX(");
        ਜੇ (!dispp->lhsp()->dtypep()->isString()) {
            puts(cvt_ਮੰਜ਼ਿਲ_Str(dispp->lhsp()->ਚੌੜਾਈ_Min()));
            putbs(",");
        }
        iterateConst(dispp->lhsp());
        emitDatap(dispp->lhsp());
        putbs(",");
    } ਨਹੀਂਤਾਂ ਜੇ (VN_IS(nodep, SFormatF)) {
        isStmt = ਝੂਠ;
        putns(nodep, "VL_SFORMATF_N_NX(");
    } ਨਹੀਂਤਾਂ {
        nodep->v3fatalSrc("Unknown displayEmit node type");
    }
    ਵਾਪਸ isStmt;
}

ਖਾਲੀ EmitCFunc::display_ਗੰਢ(Ast_ਗੰਢ* nodep, AstSFormatF* fmtp,  // fmtp is nullptr for AstScan
                            ਅਚਲ string& vformat, Ast_ਗੰਢ* exprsp, ਤਾਰਕਿਕ is_ਜਾਂਚ) {
    // Check format, if it exists
    ਅਚਲ ਤਾਰਕਿਕ ਵਿਅੰਜਕ_Format_2 = fmtp && fmtp->ਵਿਅੰਜਕ_Format_2();
    ਤਾਰਕਿਕ needsScope = ਵਿਅੰਜਕ_Format_2;
    ਤਾਰਕਿਕ needsTimescale = ਵਿਅੰਜਕ_Format_2;
    ਪੂਰਨ format_ਦਲੀਲਾਂ = 0;
    ਜੇ (!ਵਿਅੰਜਕ_Format_2) {
        // Check display and argument count to head-off later runtime issues
        ਤਾਰਕਿਕ ਵਿੱਚ_Pct = ਝੂਠ;
        ਤਾਰਕਿਕ ignore = ਝੂਠ;
        ਲਈ (ਅੱਖਰ ch : vformat) {
            // UINFO(1, "Parse '" << *pos << "'  IP" << inPct << " List " << cvtToHex(elistp));
            ਜੇ (!ਵਿੱਚ_Pct && ch == '%') {
                ਵਿੱਚ_Pct = ਸੱਚ;
                ignore = ਝੂਠ;
            } ਨਹੀਂਤਾਂ ਜੇ (ਵਿੱਚ_Pct && (std::ਅੰਕ_ਹੈ(ch) || ch == '.' || ch == '-')) {
            } ਨਹੀਂਤਾਂ ਜੇ (!ਵਿੱਚ_Pct) {  // Normal text
            } ਨਹੀਂਤਾਂ {  // Format character
                ਵਿੱਚ_Pct = ਝੂਠ;
                ch = std::tolower(ch);
                ਚੋਣ (ch) {
                ਹਾਲਤ '%': ਰੋਕੋ;
                ਹਾਲਤ '*':
                    ਵਿੱਚ_Pct = ਸੱਚ;  // Get more digits
                    ignore = ਸੱਚ;
                    ਰੋਕੋ;
                ਹਾਲਤ 'm': needsScope = ਸੱਚ; ਰੋਕੋ;
                ਹਾਲਤ 't':
                    needsTimescale = ਸੱਚ;
                    ++format_ਦਲੀਲਾਂ;
                    ਰੋਕੋ;
                ਮੂਲ:
                    ਜੇ (!ignore && V3_ਸੰਖਿਆ::displayedFmtHasArg(ch, is_ਜਾਂਚ)) ++format_ਦਲੀਲਾਂ;
                    ਰੋਕੋ;
                }
            }
        }
    }

    ਜੇ (vformat.ਖਾਲੀ_ਥਾਵਾਂ() && VN_IS(nodep, Display))  // not fscanf etc, as they need to return value
        ਵਾਪਸ;  // NOP

    ਅਚਲ ਤਾਰਕਿਕ isStmt = display_Emit_ਸਿਰਲੇਖ(nodep);

    ਜੇ (ਵਿਅੰਜਕ_Format_2) {
        UASSERT_OBJ(exprsp, nodep, "Missing format expression");
        iterateConst(exprsp);
        emitDatap(exprsp);
        exprsp = exprsp->nextp();
    } ਨਹੀਂਤਾਂ {
        ofp()->putsQuoted(vformat);
    }

    ਪੂਰਨ ਵਿਅੰਜਕ_ਦਲੀਲਾਂ = 0;
    ਲਈ (Ast_ਗੰਢ* argp = exprsp; argp; argp = argp->nextp()) ++ਵਿਅੰਜਕ_ਦਲੀਲਾਂ;
    ਜੇ (VL_UNCOVERABLE(!ਵਿਅੰਜਕ_Format_2 && ਵਿਅੰਜਕ_ਦਲੀਲਾਂ != format_ਦਲੀਲਾਂ)) {  // LCOV_EXCL_START
        // expectDisplay() checks this first, so probably internal error if found here
        nodep->v3error("Internal: Missing or extra arguments for $display-like format, "
                       << " expression had " << ਵਿਅੰਜਕ_ਦਲੀਲਾਂ << ", format had " << format_ਦਲੀਲਾਂ);
    }  // LCOV_EXCL_STOP

    // argc for error check. Also MSVC++ requires va_args to not be off a reference
    ਪੂਰਨ argc = 0;
    ਜੇ (needsScope) ++argc;
    ਜੇ (needsTimescale) ++argc;
    ਲਈ (Ast_ਗੰਢ* argp = exprsp; argp; argp = argp->nextp()) {
        ++argc;
        ਅਚਲ AstSFormatArg* ਅਚਲ fargp = VN_CAST(argp, SFormatArg);
        ਜੇ (fargp && fargp->formatAttr().isEnum()) ++argc;  // Additional name argument
    }
    ofp()->puts("," + std::ਮੰਜ਼ਿਲ_string(argc));

    ਜੇ (needsScope) {
        AstScopeName* ਅਚਲ scopenamep = fmtp ? fmtp->scopeNamep() : ਖਾਲੀ_ਸੂਚਕ;
        UASSERT_OBJ(scopenamep, nodep, "Display with %m but no AstScopeName");
        ofp()->puts(", '"s + VFormatAttr{VFormatAttr::SCOPE}.ascii() + "'");
        ਜੇ (scopenamep->selfRelative()) {
            // vlNamep is catName(name(), instance), so starts with '.' when the model is unnamed
            ਅਚਲ string namep = string{m_useSelfForThis ? "vlSelf" : "this"} + "->vlNamep";
            ofp()->puts(",(vlSymsp->name()[0] ? " + namep + " : " + namep + " + 1),");
            ofp()->putsQuoted(scopenamep->scopeAttr());
        } ਨਹੀਂਤਾਂ {
            ofp()->puts(",vlSymsp->name(),");
            ofp()->putsQuoted(scopenamep->scopePrettySymName());
        }
    }

    ਜੇ (needsTimescale) {
        VTimescale timeunit = VTimescale::NONE;
        ਜੇ (ਅਚਲ AstDisplay* ਅਚਲ anodep = VN_CAST(nodep, Display)) {
            timeunit = anodep->fmtp()->timeunit();
        } ਨਹੀਂਤਾਂ ਜੇ (ਅਚਲ AstSFormat* ਅਚਲ anodep = VN_CAST(nodep, SFormat)) {
            timeunit = anodep->fmtp()->timeunit();
        } ਨਹੀਂਤਾਂ ਜੇ (ਅਚਲ AstSScanF* ਅਚਲ anodep = VN_CAST(nodep, SScanF)) {
            timeunit = anodep->timeunit();
        } ਨਹੀਂਤਾਂ ਜੇ (ਅਚਲ AstSFormatF* ਅਚਲ anodep = VN_CAST(nodep, SFormatF)) {
            timeunit = anodep->timeunit();
        }
        UASSERT_OBJ(!timeunit.isNone(), nodep,
                    "Use of %t must be under AstDisplay, AstSFormat, or AstSFormatF, or "
                    "SScanF, and timeunit set");
        ofp()->puts(", '"s + VFormatAttr{VFormatAttr::TIMEUNIT}.ascii() + "'");
        ofp()->puts(","s + cvt_ਮੰਜ਼ਿਲ_Str((ਪੂਰਨ)timeunit.power_ਦਾ_Ten()));
    }

    // Arguments
    ਲਈ (Ast_ਗੰਢ* argp = exprsp; argp; argp = argp->nextp()) {
        ofp()->indentInc();
        ofp()->putbs("");
        AstSFormatArg* ਅਚਲ fargp = VN_CAST(argp, SFormatArg);
        Ast_ਗੰਢ* ਅਚਲ subargp = fargp ? fargp->exprp() : argp;
        ਅਚਲ VFormatAttr formatAttr = AstSFormatArg::formatAttrDefauled(fargp, subargp->dtypep());
        puts(", '"s + formatAttr.ascii() + '\'');
        ਜੇ (formatAttr.isSigned() || formatAttr.isUnsigned() || formatAttr.isEnum())
            puts("," + cvt_ਮੰਜ਼ਿਲ_Str(subargp->ਚੌੜਾਈ_Min()));
        ਅਚਲ ਤਾਰਕਿਕ addrof = is_ਜਾਂਚ || formatAttr.isString() || formatAttr.isComplex();
        ਅਚਲ ਤਾਰਕਿਕ wideEnum = formatAttr.isEnum() && subargp->isWide();
        puts(",");
        ਜੇ (wideEnum) puts("static_cast<const EData*>(");
        ਜੇ (addrof) puts("&(");
        ਜੇ (VN_IS(subargp, ਧਾਰਾ_R))
            emit_ਧਾਰਾ_R(
                VN_CAST(subargp, ਧਾਰਾ_R),
                nodep);  // This has to be done here because streamR doesn't know what it returns
        ਨਹੀਂਤਾਂ { iterateConst(subargp); }
        ਜੇ (addrof) puts(")");
        ਜੇ (!addrof) emitDatap(argp);
        ਜੇ (wideEnum) puts(")");
        ਜੇ (formatAttr.isEnum()) {
            puts(", '"s + VFormatAttr{VFormatAttr::STRING}.ascii() + "', &(");
            iterateConst(fargp->namep());
            puts(")");
        }
        ofp()->indentDec();
    }

    // End
    ਜੇ (isStmt) {
        puts(");\n");
    } ਨਹੀਂਤਾਂ {
        puts(") ");
    }
}

ਖਾਲੀ EmitCFunc::emit_CCall_ਦਲੀਲਾਂ(ਅਚਲ Ast_ਗੰਢ_CCall* nodep, ਅਚਲ string& self_ਸੂਚਕ,
                              ਤਾਰਕਿਕ ਵਿੱਚ_Process) {
    putns(nodep, "(");
    ਤਾਰਕਿਕ comma = ਝੂਠ;
    ਜੇ (nodep->funcp()->isLoose() && !nodep->funcp()->isStatic()) {
        UASSERT_OBJ(!self_ਸੂਚਕ.ਖਾਲੀ_ਥਾਵਾਂ(), nodep, "Call to loose method without self pointer");
        puts(self_ਸੂਚਕ);
        comma = ਸੱਚ;
    }
    ਜੇ (nodep->funcp()->needProcess()) {
        ਜੇ (comma) puts(", ");
        ਜੇ (VN_IS(nodep->backp(), CAwait) || !nodep->funcp()->isCoroutine()) {
            puts("vlProcess");
        } ਨਹੀਂਤਾਂ ਜੇ (ਵਿੱਚ_Process) {
            puts("std::make_shared<VlProcess>(vlProcess)");
        } ਨਹੀਂਤਾਂ {
            puts("std::make_shared<VlProcess>()");
        }
        comma = ਸੱਚ;
    }
    ਜੇ (!nodep->argTypes().ਖਾਲੀ_ਥਾਵਾਂ()) {
        ਜੇ (comma) puts(", ");
        puts(nodep->argTypes());
        comma = ਸੱਚ;
    }
    put_Comma_Iterate_ਅਗਲਾ(nodep->argsp(), comma);
    puts(")");
}

std::string EmitCFunc::dereferenceString(ਅਚਲ std::string& ਸੂਚਕ) ਅਚਲ {
    ਜੇ (ਸੂਚਕ[0] == '(' && ਸੂਚਕ[1] == '&') {
        // remove "address of" followed by immediate dereference
        // Note: this relies on only the form '(&OBJECT)' being used by Verilator
        ਵਾਪਸ ਸੂਚਕ.substr(2, ਸੂਚਕ.ਲੰਮਾਈ() - 3) + '.';
    } ਨਹੀਂਤਾਂ {
        ਜੇ (ਸੂਚਕ == "vlSelf" && m_usevlSelfRef) {
            ਵਾਪਸ "vlSelfRef.";
        } ਨਹੀਂਤਾਂ {
            ਵਾਪਸ ਸੂਚਕ + "->";
        }
    }
}
ਖਾਲੀ EmitCFunc::emitDereference(Ast_ਗੰਢ* nodep, ਅਚਲ string& ਸੂਚਕ) {
    putns(nodep, dereferenceString(ਸੂਚਕ));
}

ਖਾਲੀ EmitCFunc::emitCvtPackStr(Ast_ਗੰਢ* nodep) {
    ਜੇ (ਅਚਲ AstConst* ਅਚਲ constp = VN_CAST(nodep, Const)) {
        emitConstantString(constp);
    } ਨਹੀਂਤਾਂ ਜੇ (VN_IS(nodep->dtypep(), ਧਾਰਾ_DType)) {
        putnbs(nodep, "VL_CVT_PACK_STR_ND(");
        iterate_And_ਅਗਲਾ_Const_Null(nodep);
        puts(")");
    } ਨਹੀਂਤਾਂ {
        putnbs(nodep, "VL_CVT_PACK_STR_N");
        emitIQW(nodep);
        puts("(");
        ਜੇ (nodep->isWide()) {
            // Note argument width, not node width (which is always 32)
            puts(cvt_ਮੰਜ਼ਿਲ_Str(nodep->ਚੌੜਾਈ_ਸ਼ਬਦ()));
            puts(", ");
        }
        iterate_And_ਅਗਲਾ_Const_Null(nodep);
        puts(")");
    }
}

ਖਾਲੀ EmitCFunc::emitConstant(AstConst* nodep) {
    // Put out constant set to the specified variable, or given variable in a string
    ਅਚਲ V3_ਸੰਖਿਆ& ਅੰਕ = nodep->ਅੰਕ();
    ਜੇ (ਅੰਕ.isFourState()) {
        nodep->v3warn(E_UNSUPPORTED, "Unsupported: 4-state numbers in this context");
        ਵਾਪਸ;
    }
    putns(nodep, ਅੰਕ.emitC());
}

ਖਾਲੀ EmitCFunc::emitConstantString(ਅਚਲ AstConst* nodep) {
    // Const might be a Verilog array-type string, but need to always output std::string
    putnbs(nodep, "std::string{");
    ਅਚਲ string str = nodep->ਅੰਕ().ਮੰਜ਼ਿਲ_String();
    ਜੇ (!str.ਖਾਲੀ_ਥਾਵਾਂ()) putsQuoted(str);
    puts("}");
}

ਖਾਲੀ EmitCFunc::emitVarReset(ਅਚਲ string& ਅਗੇਤਰ, AstVar* varp, ਤਾਰਕਿਕ constructing) {
    // 'constructing' indicates that the object was just constructed, so if it is a string or
    // something that starts off clear already, no need to clear it again
    Ast_ਗੰਢ_DType* ਅਚਲ dtypep = varp->dtypep()->skipRefp();
    ਅਚਲ string vlSelf = VSelf_ਸੂਚਕ_ਪਾਠ::replaceThis(m_useSelfForThis, "this->");
    ਅਚਲ string varNameProtected
        = ((VN_IS(m_modp, Class) || varp->isFuncLocal()) || !ਅਗੇਤਰ.ਖਾਲੀ_ਥਾਵਾਂ())
              ? varName(varp)
              : vlSelf + varName(varp);
    ਅਚਲ string new_ਅਗੇਤਰ = ਅਗੇਤਰ + varNameProtected;
    ਜੇ (varp->isIO() && m_modp->is_ਸਿਖਰ() && optSystemC()) {
        // System C top I/O doesn't need loading, as the lower level subinst code does it.}
    } ਨਹੀਂਤਾਂ ਜੇ (varp->isParam()) {
        UASSERT_OBJ(varp->valuep(), varp, "No init for a param?");
        // If a simple CONST value we initialize it using an enum
        // If an ARRAYINIT we initialize it using an initial block similar to a signal
        // puts("// parameter "+varp->nameProtect()+" = "+varp->valuep()->name()+"\n");
    } ਨਹੀਂਤਾਂ {
        putns(varp,
              emitVarResetRecurse(varp, constructing, new_ਅਗੇਤਰ, dtypep, 0, "", varp->valuep()));
    }
}

string EmitCFunc::emitVarResetRecurse(ਅਚਲ AstVar* varp, ਤਾਰਕਿਕ constructing,
                                      ਅਚਲ string& varNameProtected, Ast_ਗੰਢ_DType* dtypep,
                                      ਪੂਰਨ ਡੂੰਘਾਈ, ਅਚਲ string& suffix, ਅਚਲ Ast_ਗੰਢ* valuep) {
    dtypep = dtypep->skipRefp();
    AstBasicDType* ਅਚਲ basicp = dtypep->basicp();
    // Returns string to do resetting, empty to do nothing (which caller should handle)
    ਜੇ (AstAssocArrayDType* ਅਚਲ adtypep = VN_CAST(dtypep, AssocArrayDType)) {
        // Access std::array as C array
        ਅਚਲ string pre = constructing ? "" : varNameProtected + suffix + ".clear();\n";
        ਵਾਪਸ pre
               + emitVarResetRecurse(varp, constructing, varNameProtected, adtypep->subDTypep(),
                                     ਡੂੰਘਾਈ + 1, suffix + ".atDefault()", ਖਾਲੀ_ਸੂਚਕ);
    } ਨਹੀਂਤਾਂ ਜੇ (AstWildcardArrayDType* ਅਚਲ adtypep = VN_CAST(dtypep, WildcardArrayDType)) {
        // Access std::array as C array
        ਅਚਲ string pre = constructing ? "" : varNameProtected + suffix + ".clear();\n";
        ਵਾਪਸ pre
               + emitVarResetRecurse(varp, constructing, varNameProtected, adtypep->subDTypep(),
                                     ਡੂੰਘਾਈ + 1, suffix + ".atDefault()", ਖਾਲੀ_ਸੂਚਕ);
    } ਨਹੀਂਤਾਂ ਜੇ (VN_IS(dtypep, CDType)) {
        ਵਾਪਸ "";  // Constructor does it
    } ਨਹੀਂਤਾਂ ਜੇ (VN_IS(dtypep, CoverCrossDType) || VN_IS(dtypep, CoverpointDType)) {
        ਵਾਪਸ "";  // Covergroup constructor creates the runtime and assigns the pointer
    } ਨਹੀਂਤਾਂ ਜੇ (ਅਚਲ AstClassRefDType* ਅਚਲ adtypep = VN_CAST(dtypep, ClassRefDType)) {
        ਵਾਪਸ adtypep->raw_ਸੂਚਕ() ? varNameProtected + suffix + " = nullptr;\n" : "";
    } ਨਹੀਂਤਾਂ ਜੇ (VN_IS(dtypep, IfaceRefDType)) {
        ਵਾਪਸ varNameProtected + suffix + " = nullptr;\n";
    } ਨਹੀਂਤਾਂ ਜੇ (ਅਚਲ AstDynArrayDType* ਅਚਲ adtypep = VN_CAST(dtypep, DynArrayDType)) {
        ਅਚਲ string pre = constructing ? "" : varNameProtected + suffix + ".clear();\n";
        ਵਾਪਸ pre
               + emitVarResetRecurse(varp, constructing, varNameProtected, adtypep->subDTypep(),
                                     ਡੂੰਘਾਈ + 1, suffix + ".atDefault()", ਖਾਲੀ_ਸੂਚਕ);
    } ਨਹੀਂਤਾਂ ਜੇ (ਅਚਲ Ast_ਕਤਾਰ_DType* ਅਚਲ adtypep = VN_CAST(dtypep, ਕਤਾਰ_DType)) {
        ਅਚਲ string pre = constructing ? "" : varNameProtected + suffix + ".clear();\n";
        ਵਾਪਸ pre
               + emitVarResetRecurse(varp, constructing, varNameProtected, adtypep->subDTypep(),
                                     ਡੂੰਘਾਈ + 1, suffix + ".atDefault()", ਖਾਲੀ_ਸੂਚਕ);
    } ਨਹੀਂਤਾਂ ਜੇ (VN_IS(dtypep, Sample_ਕਤਾਰ_DType)) {
        ਵਾਪਸ "";
    } ਨਹੀਂਤਾਂ ਜੇ (ਅਚਲ AstUnpackArrayDType* ਅਚਲ adtypep = VN_CAST(dtypep, UnpackArrayDType)) {
        UASSERT_OBJ(adtypep->hi() >= adtypep->lo(), varp,
                    "Should have swapped msb & lsb earlier.");
        ਅਚਲ Ast_ਗੰਢ_DType* ਅਚਲ elementp = adtypep->subDTypep()->skipRefp();
        ਜੇ (!varp->isFuncLocal() && elementp->isIntegralOrPacked()) {
            // Scoped reset has no array-index input or mutable RNG state. Initialize one
            // element, then fill in the optimized runtime even when this code is cold.
            ਅਚਲ string ਪਹਿਲਾ
                = emitVarResetRecurse(varp, constructing, varNameProtected, adtypep->subDTypep(),
                                      ਡੂੰਘਾਈ + 1, suffix + "[0]", ਖਾਲੀ_ਸੂਚਕ);
            ਅਚਲ string array = varNameProtected + suffix;
            ਵਾਪਸ ਪਹਿਲਾ + "VL_UNPACKED_FILL(" + array + ".data(), sizeof(" + array + "[0]), "
                   + cvt_ਮੰਜ਼ਿਲ_Str(adtypep->elementsConst()) + ");\n";
        }
        ਅਚਲ string ivar = "__Vi"s + cvt_ਮੰਜ਼ਿਲ_Str(ਡੂੰਘਾਈ);
        ਅਚਲ string pre = ("for (int " + ivar + " = " + cvt_ਮੰਜ਼ਿਲ_Str(0) + "; " + ivar + " < "
                            + cvt_ਮੰਜ਼ਿਲ_Str(adtypep->elementsConst()) + "; ++" + ivar + ") {\n");
        ਅਚਲ string ਹੇਠਾਂ
            = emitVarResetRecurse(varp, constructing, varNameProtected, adtypep->subDTypep(),
                                  ਡੂੰਘਾਈ + 1, suffix + "[" + ivar + "]", ਖਾਲੀ_ਸੂਚਕ);
        ਅਚਲ string post = "}\n";
        ਵਾਪਸ ਹੇਠਾਂ.ਖਾਲੀ_ਥਾਵਾਂ() ? "" : pre + ਹੇਠਾਂ + post;
    } ਨਹੀਂਤਾਂ ਜੇ (VN_IS(dtypep, ਗੰਢ_UOr_Struct_DType) && !VN_AS(dtypep, ਗੰਢ_UOr_Struct_DType)->packed()) {
        ਅਚਲ Ast_ਗੰਢ_UOr_Struct_DType* ਅਚਲ sdtypep = VN_AS(dtypep, ਗੰਢ_UOr_Struct_DType);
        string literal;
        ਲਈ (ਅਚਲ AstMemberDType* itemp = sdtypep->membersp(); itemp;
             itemp = VN_AS(itemp->nextp(), MemberDType)) {
            ਅਚਲ std::string ਸਤਰ = emitVarResetRecurse(
                varp, constructing, varNameProtected + suffix + "." + itemp->nameProtect(),
                itemp->dtypep(), ਡੂੰਘਾਈ + 1, "", itemp->valuep());
            ਜੇ (!ਸਤਰ.ਖਾਲੀ_ਥਾਵਾਂ()) literal += ਸਤਰ;
        }
        ਵਾਪਸ literal;
    } ਨਹੀਂਤਾਂ ਜੇ (basicp && basicp->keyword() == VBasicDTypeKwd::STRING) {
        ਜੇ (constructing) ਵਾਪਸ "";  // String's constructor deals with it
        ਵਾਪਸ varNameProtected + suffix + ".clear();\n";
    } ਨਹੀਂਤਾਂ ਜੇ (basicp && basicp->isForkSync()) {
        ਵਾਪਸ "";
    } ਨਹੀਂਤਾਂ ਜੇ (basicp && basicp->isProcessRef()) {
        ਵਾਪਸ "";
    } ਨਹੀਂਤਾਂ ਜੇ (basicp && basicp->isDelayScheduler()) {
        ਵਾਪਸ "";
    } ਨਹੀਂਤਾਂ ਜੇ (basicp && basicp->isTriggerScheduler()) {
        ਵਾਪਸ "";
    } ਨਹੀਂਤਾਂ ਜੇ (basicp && basicp->isDynamicTriggerScheduler()) {
        ਵਾਪਸ "";
    } ਨਹੀਂਤਾਂ ਜੇ (basicp && (basicp->isRandomGenerator() || basicp->isStdRandomGenerator())) {
        ਵਾਪਸ "";
    } ਨਹੀਂਤਾਂ ਜੇ (basicp && basicp->isCovergroupInstHandle()) {
        // The handle's own constructor deals with it.
        ਵਾਪਸ "";
    } ਨਹੀਂਤਾਂ ਜੇ (basicp && (basicp->isEvent())) {
        ਵਾਪਸ "VlAssignableEvent{};\n";
    } ਨਹੀਂਤਾਂ ਜੇ (basicp) {
        ਅਚਲ ਤਾਰਕਿਕ zeroit
            = (varp->attrFileDescr()  // Zero so we don't do file IO if never $fopen
               || varp->isFuncLocal()  // Randomization too slow
               || (basicp && basicp->isZeroInit())
               || (v3Global.opt.underlineZero() && !varp->name().ਖਾਲੀ_ਥਾਵਾਂ() && varp->name()[0] == '_')
               || (varp->varType().is_ਆਰਜ਼ੀ() && !varp->isXTemp())
               || (varp->isXTemp()
                       ? (v3Global.opt.x_ਸੌਂਪੋ() != "unique")
                       : (v3Global.opt.xInitial() == "fast" || v3Global.opt.xInitial() == "0")));
        ਅਚਲ ਤਾਰਕਿਕ slow = !varp->isFuncLocal() && !varp->isClassMember();
        ਵੰਡੋ_ਮਾਪ_Inc(1);
        ਜੇ (dtypep->isWide()) {  // Handle unpacked; not basicp->isWide
            string ਬਾਹਰਲਾ;
            ਜੇ (valuep) {
                ਅਚਲ AstConst* ਅਚਲ constp = VN_AS(valuep, Const);
                UASSERT_OBJ(constp, varp, "non-const initializer for variable");
                ਲਈ (ਪੂਰਨ w = 0; w < dtypep->ਚੌੜਾਈ_ਸ਼ਬਦ(); ++w) {
                    ਬਾਹਰਲਾ += varNameProtected + suffix + "[" + cvt_ਮੰਜ਼ਿਲ_Str(w) + "] = ";
                    ਬਾਹਰਲਾ += cvt_ਮੰਜ਼ਿਲ_Str(constp->ਅੰਕ().edata_ਸ਼ਬਦ(w)) + "U;\n";
                }
            } ਨਹੀਂਤਾਂ {
                ਬਾਹਰਲਾ += zeroit ? (slow ? "VL_ZERO_RESET_W(" : "VL_ZERO_W(")
                              : (varp->isXTemp() ? "VL_SCOPED_RAND_RESET_ASSIGN_W("
                                                 : "VL_SCOPED_RAND_RESET_W(");
                ਬਾਹਰਲਾ += cvt_ਮੰਜ਼ਿਲ_Str(dtypep->ਚੌੜਾਈ_Min());
                ਬਾਹਰਲਾ += ", " + varNameProtected + suffix;
                ਜੇ (!zeroit) {
                    emitVarResetScopeHash();
                    ਅਚਲ uint64_t salt = VString::hashMurmur(varp->prettyName());
                    ਬਾਹਰਲਾ += ", ";
                    ਬਾਹਰਲਾ += m_classOrPackage ? m_classOrPackageHash : "__VscopeHash";
                    ਬਾਹਰਲਾ += ", ";
                    ਬਾਹਰਲਾ += std::ਮੰਜ਼ਿਲ_string(salt);
                    ਬਾਹਰਲਾ += "ull";
                }
                ਬਾਹਰਲਾ += ");\n";
            }
            ਵਾਪਸ ਬਾਹਰਲਾ;
        } ਨਹੀਂਤਾਂ {
            string ਬਾਹਰਲਾ = varNameProtected + suffix;
            ਜੇ (valuep) {
                ਬਾਹਰਲਾ += " = ";
                // TODO cleanup code shared between here, V3EmitCConstInit.h,
                // EmitCFunc::emitVarReset, EmitCFunc::emitConstant
                ਅਚਲ AstConst* ਅਚਲ constp = VN_AS(valuep, Const);
                UASSERT_OBJ(constp, varp, "non-const initializer for variable");
                ਬਾਹਰਲਾ += cvt_ਮੰਜ਼ਿਲ_Str(constp->ਅੰਕ().edata_ਸ਼ਬਦ(0)) + "U;\n";
                ਬਾਹਰਲਾ += ";\n";
            } ਨਹੀਂਤਾਂ ਜੇ (zeroit) {
                ਬਾਹਰਲਾ += " = 0;\n";
            } ਨਹੀਂਤਾਂ {
                emitVarResetScopeHash();
                ਅਚਲ uint64_t salt = VString::hashMurmur(varp->prettyName());
                ਬਾਹਰਲਾ += " = VL_SCOPED_RAND_RESET_";
                ਜੇ (varp->isXTemp()) ਬਾਹਰਲਾ += "ASSIGN_";
                ਬਾਹਰਲਾ += dtypep->charIQWN();
                ਬਾਹਰਲਾ += "(" + cvt_ਮੰਜ਼ਿਲ_Str(dtypep->ਚੌੜਾਈ_Min()) + ", "
                       + (m_classOrPackage ? m_classOrPackageHash : "__VscopeHash") + ", "
                       + std::ਮੰਜ਼ਿਲ_string(salt) + "ull);\n";
            }
            ਵਾਪਸ ਬਾਹਰਲਾ;
        }
    } ਨਹੀਂਤਾਂ {  // LCOV_EXCL_BR_LINE
        v3fatalSrc("Unknown node type in reset generator: " << varp->prettyTypeName());
    }
    ਵਾਪਸ "";
}

ਖਾਲੀ EmitCFunc::emitVarResetScopeHash() {
    ਜੇ (VL_LIKELY(m_createdScopeHash)) { ਵਾਪਸ; }
    ਜੇ (m_classOrPackage) {
        m_classOrPackageHash
            = std::ਮੰਜ਼ਿਲ_string(VString::hashMurmur(m_classOrPackage->name())) + "ULL";
    } ਨਹੀਂਤਾਂ {
        puts(string("const uint64_t __VscopeHash = VL_MURMUR64_HASH(")
             + (m_useSelfForThis ? "vlSelf" : "this") + "->vlNamep);\n");
    }
    m_createdScopeHash = ਸੱਚ;
}
