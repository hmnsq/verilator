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

#అన్తర్భావ "V3PchAstMT.h"

#అన్తర్భావ "V3EmitCFunc.h"

#అన్తర్భావ <map>
#అన్తర్భావ <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// EmitCFunc

తార్కిక EmitCFunc::emitSimpleOk(Ast_కణుపు_సమాసం* nodep) {
    // Can we put out a simple (A + B) instead of VL_ADD_III(A,B)?
    ఒకవేళ (nodep->emitSimpleOperator() == "") తిరిగి అబద్ధం;
    ఒకవేళ (nodep->isWide()) తిరిగి అబద్ధం;
    ఒకవేళ (nodep->op1p() && nodep->op1p()->isWide()) తిరిగి అబద్ధం;
    ఒకవేళ (nodep->op2p() && nodep->op2p()->isWide()) తిరిగి అబద్ధం;
    ఒకవేళ (nodep->op3p() && nodep->op3p()->isWide()) తిరిగి అబద్ధం;
    తిరిగి నిజం;
}

శూన్య EmitCFunc::emitOpName(Ast_కణుపు* nodep, స్థిరరాశి string& format, Ast_కణుపు* lhsp, Ast_కణుపు* rhsp,
                           Ast_కణుపు* thsp) {
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
    string బహిర్గతం;
    putnbs(nodep, "");

    తార్కిక needComma = అబద్ధం;
    తార్కిక ఉపయోగిస్తుంది_వరుస = అబద్ధం;
    string తదుపరి_Comma;
    స్వయంచాలక comma_బహిర్గతం = [&బహిర్గతం, &తదుపరి_Comma]() {
        ఒకవేళ (!తదుపరి_Comma.ఖాళీలు()) {
            బహిర్గతం += తదుపరి_Comma;
            తదుపరి_Comma = "";
        }
    };

    స్వయంచాలక put_బహిర్గతం = [ఇది, &బహిర్గతం]() {
        ఒకవేళ (!బహిర్గతం.ఖాళీలు()) puts(బహిర్గతం);
        బహిర్గతం = "";
    };

    కొరకు (string::const_iterator pos = format.begin(); pos != format.చివర(); ++pos) {
        ఒకవేళ (pos[0] == ',') {
            // Remember we need to add one, but don't do yet to avoid ",)"
            ఒకవేళ (needComma) {
                ఒకవేళ (pos[1] == ' ') {
                    తదుపరి_Comma = ", ";
                } లేకపోతే {
                    తదుపరి_Comma = ",";
                }
                needComma = అబద్ధం;
            }
            ఒకవేళ (pos[1] == ' ') ++pos;  // Must do even if no nextComma
        } లేకపోతే ఒకవేళ (pos[0] == '%') {
            ++pos;
            తార్కిక detail = అబద్ధం;
            Ast_కణుపు* detailp = శూన్యసూచిక;
            ఎంపిక (pos[0]) {
            స్థితి '%': బహిర్గతం += '%'; ఆపు;
            స్థితి 'k':
                put_బహిర్గతం();
                putbs("");
                ఆపు;
            స్థితి 'n':
                detail = నిజం;
                detailp = nodep;
                ఆపు;
            స్థితి 'l':
                detail = నిజం;
                detailp = lhsp;
                ఆపు;
            స్థితి 'r':
                detail = నిజం;
                detailp = rhsp;
                ఆపు;
            స్థితి 't':
                detail = నిజం;
                detailp = thsp;
                ఆపు;
            స్థితి 'P':
                ఒకవేళ (nodep->isWide() && !ఉపయోగిస్తుంది_వరుస) {
                    UASSERT_OBJ(m_wide_తాత్కాలిక_Refp, nodep,
                                "Wide op " << nodep->prettyTypeName()
                                           << " w/ no temp, perhaps missing op in V3EmitC?");
                    comma_బహిర్గతం();
                    put_బహిర్గతం();
                    ఒకవేళ (!m_wide_తాత్కాలిక_Refp->self_సూచిక().is_ఖాళీలు()) {
                        emitDereference(m_wide_తాత్కాలిక_Refp,
                                        m_wide_తాత్కాలిక_Refp->self_సూచిక_Protect(m_useSelfForThis));
                    }
                    బహిర్గతం += varName(m_wide_తాత్కాలిక_Refp->varp());
                    m_wide_తాత్కాలిక_Refp = శూన్యసూచిక;
                    needComma = నిజం;
                } లేకపోతే ఒకవేళ (ఉపయోగిస్తుంది_వరుస) {
                    comma_బహిర్గతం();
                    put_బహిర్గతం();
                    iterate_And_తదుపరి_Const_Null(nodep->backp()->op2p());
                    needComma = నిజం;
                }

                ఆపు;
            అప్రమేయ: nodep->v3fatalSrc("Unknown emitOperator format code: %" << pos[0]); ఆపు;
            }
            ఒకవేళ (detail) {
                // Get next letter of %[nlrt]
                ++pos;
                ఎంపిక (pos[0]) {
                స్థితి 'q':
                    put_బహిర్గతం();
                    // If we are assigning this to a queue we need to get the return type
                    ఒకవేళ (VN_IS(detailp->backp(), కేటాయించు)
                        && VN_IS(detailp->backp()->op2p()->dtypep()->skipRefp(), వరుస_DType)) {
                        puts("R");  // R for queue
                        ఉపయోగిస్తుంది_వరుస = నిజం;
                    } లేకపోతే ఒకవేళ (VN_IS(detailp->dtypep()->skipRefp(), వరుస_DType)
                               || VN_IS(detailp->dtypep()->skipRefp(), ప్రవాహం_DType)) {
                        puts("R");  // R for queue
                    } లేకపోతే {
                        emitIQW(detailp);
                    }
                    ఆపు;
                స్థితి 'w':
                    comma_బహిర్గతం();
                    బహిర్గతం += cvt_గమ్యం_Str(detailp->వెడల్పు_Min());
                    needComma = నిజం;
                    ఆపు;
                స్థితి 'W':
                    ఒకవేళ (lhsp->isWide()) {
                        comma_బహిర్గతం();
                        బహిర్గతం += cvt_గమ్యం_Str(lhsp->వెడల్పు_పదాలు());
                        needComma = నిజం;
                    } లేకపోతే ఒకవేళ (VN_IS(lhsp, ప్రవాహం_R)) {
                        comma_బహిర్గతం();
                        బహిర్గతం += cvt_గమ్యం_Str(rhsp->వెడల్పు_పదాలు());
                        needComma = నిజం;
                    }
                    ఆపు;
                స్థితి 'i':
                    comma_బహిర్గతం();
                    UASSERT_OBJ(detailp, nodep, "emitOperator() references undef node");
                    put_బహిర్గతం();
                    iterate_And_తదుపరి_Const_Null(detailp);
                    needComma = నిజం;
                    ఆపు;
                అప్రమేయ:
                    nodep->v3fatalSrc("Unknown emitOperator format code: %[nlrt]" << pos[0]);
                    ఆపు;
                }
            }
        } లేకపోతే ఒకవేళ (pos[0] == ')') {
            తదుపరి_Comma = "";
            బహిర్గతం += ')';
        } లేకపోతే ఒకవేళ (pos[0] == '(') {
            comma_బహిర్గతం();
            needComma = అబద్ధం;
            బహిర్గతం += '(';
        } లేకపోతే {
            // Normal text
            ఒకవేళ (std::అక్షరమా_అంకెనా(pos[0])) needComma = నిజం;
            comma_బహిర్గతం();
            బహిర్గతం += pos[0];
        }
    }
    put_బహిర్గతం();
}

తార్కిక EmitCFunc::display_Emit_శీర్షిక(Ast_కణుపు* nodep) {
    తార్కిక isStmt = అబద్ధం;
    ఒకవేళ (స్థిరరాశి AstFScanF* స్థిరరాశి dispp = VN_CAST(nodep, FScanF)) {
        isStmt = అబద్ధం;
        putns(nodep, "VL_FSCANF_INX(");
        iterateConst(dispp->filep());
        puts(",");
    } లేకపోతే ఒకవేళ (స్థిరరాశి AstSScanF* స్థిరరాశి dispp = VN_CAST(nodep, SScanF)) {
        isStmt = అబద్ధం;
        putns(nodep, "VL_SSCANF_I");
        emitIQW(dispp->fromp());
        puts("NX(");
        puts(cvt_గమ్యం_Str(dispp->fromp()->వెడల్పు_Min()));
        puts(",");
        iterateConst(dispp->fromp());
        puts(",");
    } లేకపోతే ఒకవేళ (స్థిరరాశి AstDisplay* స్థిరరాశి dispp = VN_CAST(nodep, Display)) {
        isStmt = నిజం;
        ఒకవేళ (dispp->filep()) {
            putns(nodep, "VL_FWRITEF_NX(");
            iterateConst(dispp->filep());
            puts(",");
        } లేకపోతే {
            putns(nodep, "VL_WRITEF_NX(");
        }
    } లేకపోతే ఒకవేళ (స్థిరరాశి AstSFormat* స్థిరరాశి dispp = VN_CAST(nodep, SFormat)) {
        isStmt = నిజం;
        puts("VL_SFORMAT_NX(");
        ఒకవేళ (!dispp->lhsp()->dtypep()->isString()) {
            puts(cvt_గమ్యం_Str(dispp->lhsp()->వెడల్పు_Min()));
            putbs(",");
        }
        iterateConst(dispp->lhsp());
        emitDatap(dispp->lhsp());
        putbs(",");
    } లేకపోతే ఒకవేళ (VN_IS(nodep, SFormatF)) {
        isStmt = అబద్ధం;
        putns(nodep, "VL_SFORMATF_N_NX(");
    } లేకపోతే {
        nodep->v3fatalSrc("Unknown displayEmit node type");
    }
    తిరిగి isStmt;
}

శూన్య EmitCFunc::display_కణుపు(Ast_కణుపు* nodep, AstSFormatF* fmtp,  // fmtp is nullptr for AstScan
                            స్థిరరాశి string& vformat, Ast_కణుపు* exprsp, తార్కిక is_పరిశీలన) {
    // Check format, if it exists
    స్థిరరాశి తార్కిక సమాసం_Format_2 = fmtp && fmtp->సమాసం_Format_2();
    తార్కిక needsScope = సమాసం_Format_2;
    తార్కిక needsTimescale = సమాసం_Format_2;
    పూర్ణాంకం format_వాదనలు = 0;
    ఒకవేళ (!సమాసం_Format_2) {
        // Check display and argument count to head-off later runtime issues
        తార్కిక లోపల_Pct = అబద్ధం;
        తార్కిక ignore = అబద్ధం;
        కొరకు (అక్షరం ch : vformat) {
            // UINFO(1, "Parse '" << *pos << "'  IP" << inPct << " List " << cvtToHex(elistp));
            ఒకవేళ (!లోపల_Pct && ch == '%') {
                లోపల_Pct = నిజం;
                ignore = అబద్ధం;
            } లేకపోతే ఒకవేళ (లోపల_Pct && (std::అంకెనా(ch) || ch == '.' || ch == '-')) {
            } లేకపోతే ఒకవేళ (!లోపల_Pct) {  // Normal text
            } లేకపోతే {  // Format character
                లోపల_Pct = అబద్ధం;
                ch = std::tolower(ch);
                ఎంపిక (ch) {
                స్థితి '%': ఆపు;
                స్థితి '*':
                    లోపల_Pct = నిజం;  // Get more digits
                    ignore = నిజం;
                    ఆపు;
                స్థితి 'm': needsScope = నిజం; ఆపు;
                స్థితి 't':
                    needsTimescale = నిజం;
                    ++format_వాదనలు;
                    ఆపు;
                అప్రమేయ:
                    ఒకవేళ (!ignore && V3_సంఖ్య::displayedFmtHasArg(ch, is_పరిశీలన)) ++format_వాదనలు;
                    ఆపు;
                }
            }
        }
    }

    ఒకవేళ (vformat.ఖాళీలు() && VN_IS(nodep, Display))  // not fscanf etc, as they need to return value
        తిరిగి;  // NOP

    స్థిరరాశి తార్కిక isStmt = display_Emit_శీర్షిక(nodep);

    ఒకవేళ (సమాసం_Format_2) {
        UASSERT_OBJ(exprsp, nodep, "Missing format expression");
        iterateConst(exprsp);
        emitDatap(exprsp);
        exprsp = exprsp->nextp();
    } లేకపోతే {
        ofp()->putsQuoted(vformat);
    }

    పూర్ణాంకం సమాసం_వాదనలు = 0;
    కొరకు (Ast_కణుపు* argp = exprsp; argp; argp = argp->nextp()) ++సమాసం_వాదనలు;
    ఒకవేళ (VL_UNCOVERABLE(!సమాసం_Format_2 && సమాసం_వాదనలు != format_వాదనలు)) {  // LCOV_EXCL_START
        // expectDisplay() checks this first, so probably internal error if found here
        nodep->v3error("Internal: Missing or extra arguments for $display-like format, "
                       << " expression had " << సమాసం_వాదనలు << ", format had " << format_వాదనలు);
    }  // LCOV_EXCL_STOP

    // argc for error check. Also MSVC++ requires va_args to not be off a reference
    పూర్ణాంకం argc = 0;
    ఒకవేళ (needsScope) ++argc;
    ఒకవేళ (needsTimescale) ++argc;
    కొరకు (Ast_కణుపు* argp = exprsp; argp; argp = argp->nextp()) {
        ++argc;
        స్థిరరాశి AstSFormatArg* స్థిరరాశి fargp = VN_CAST(argp, SFormatArg);
        ఒకవేళ (fargp && fargp->formatAttr().isEnum()) ++argc;  // Additional name argument
    }
    ofp()->puts("," + std::గమ్యం_string(argc));

    ఒకవేళ (needsScope) {
        AstScopeName* స్థిరరాశి scopenamep = fmtp ? fmtp->scopeNamep() : శూన్యసూచిక;
        UASSERT_OBJ(scopenamep, nodep, "Display with %m but no AstScopeName");
        ofp()->puts(", '"s + VFormatAttr{VFormatAttr::SCOPE}.ascii() + "'");
        ఒకవేళ (scopenamep->selfRelative()) {
            // vlNamep is catName(name(), instance), so starts with '.' when the model is unnamed
            స్థిరరాశి string namep = string{m_useSelfForThis ? "vlSelf" : "this"} + "->vlNamep";
            ofp()->puts(",(vlSymsp->name()[0] ? " + namep + " : " + namep + " + 1),");
            ofp()->putsQuoted(scopenamep->scopeAttr());
        } లేకపోతే {
            ofp()->puts(",vlSymsp->name(),");
            ofp()->putsQuoted(scopenamep->scopePrettySymName());
        }
    }

    ఒకవేళ (needsTimescale) {
        VTimescale timeunit = VTimescale::NONE;
        ఒకవేళ (స్థిరరాశి AstDisplay* స్థిరరాశి anodep = VN_CAST(nodep, Display)) {
            timeunit = anodep->fmtp()->timeunit();
        } లేకపోతే ఒకవేళ (స్థిరరాశి AstSFormat* స్థిరరాశి anodep = VN_CAST(nodep, SFormat)) {
            timeunit = anodep->fmtp()->timeunit();
        } లేకపోతే ఒకవేళ (స్థిరరాశి AstSScanF* స్థిరరాశి anodep = VN_CAST(nodep, SScanF)) {
            timeunit = anodep->timeunit();
        } లేకపోతే ఒకవేళ (స్థిరరాశి AstSFormatF* స్థిరరాశి anodep = VN_CAST(nodep, SFormatF)) {
            timeunit = anodep->timeunit();
        }
        UASSERT_OBJ(!timeunit.isNone(), nodep,
                    "Use of %t must be under AstDisplay, AstSFormat, or AstSFormatF, or "
                    "SScanF, and timeunit set");
        ofp()->puts(", '"s + VFormatAttr{VFormatAttr::TIMEUNIT}.ascii() + "'");
        ofp()->puts(","s + cvt_గమ్యం_Str((పూర్ణాంకం)timeunit.power_యొక్క_Ten()));
    }

    // Arguments
    కొరకు (Ast_కణుపు* argp = exprsp; argp; argp = argp->nextp()) {
        ofp()->indentInc();
        ofp()->putbs("");
        AstSFormatArg* స్థిరరాశి fargp = VN_CAST(argp, SFormatArg);
        Ast_కణుపు* స్థిరరాశి subargp = fargp ? fargp->exprp() : argp;
        స్థిరరాశి VFormatAttr formatAttr = AstSFormatArg::formatAttrDefauled(fargp, subargp->dtypep());
        puts(", '"s + formatAttr.ascii() + '\'');
        ఒకవేళ (formatAttr.isSigned() || formatAttr.isUnsigned() || formatAttr.isEnum())
            puts("," + cvt_గమ్యం_Str(subargp->వెడల్పు_Min()));
        స్థిరరాశి తార్కిక addrof = is_పరిశీలన || formatAttr.isString() || formatAttr.isComplex();
        స్థిరరాశి తార్కిక wideEnum = formatAttr.isEnum() && subargp->isWide();
        puts(",");
        ఒకవేళ (wideEnum) puts("static_cast<const EData*>(");
        ఒకవేళ (addrof) puts("&(");
        ఒకవేళ (VN_IS(subargp, ప్రవాహం_R))
            emit_ప్రవాహం_R(
                VN_CAST(subargp, ప్రవాహం_R),
                nodep);  // This has to be done here because streamR doesn't know what it returns
        లేకపోతే { iterateConst(subargp); }
        ఒకవేళ (addrof) puts(")");
        ఒకవేళ (!addrof) emitDatap(argp);
        ఒకవేళ (wideEnum) puts(")");
        ఒకవేళ (formatAttr.isEnum()) {
            puts(", '"s + VFormatAttr{VFormatAttr::STRING}.ascii() + "', &(");
            iterateConst(fargp->namep());
            puts(")");
        }
        ofp()->indentDec();
    }

    // End
    ఒకవేళ (isStmt) {
        puts(");\n");
    } లేకపోతే {
        puts(") ");
    }
}

శూన్య EmitCFunc::emit_CCall_వాదనలు(స్థిరరాశి Ast_కణుపు_CCall* nodep, స్థిరరాశి string& self_సూచిక,
                              తార్కిక లోపల_Process) {
    putns(nodep, "(");
    తార్కిక comma = అబద్ధం;
    ఒకవేళ (nodep->funcp()->isLoose() && !nodep->funcp()->isStatic()) {
        UASSERT_OBJ(!self_సూచిక.ఖాళీలు(), nodep, "Call to loose method without self pointer");
        puts(self_సూచిక);
        comma = నిజం;
    }
    ఒకవేళ (nodep->funcp()->needProcess()) {
        ఒకవేళ (comma) puts(", ");
        ఒకవేళ (VN_IS(nodep->backp(), CAwait) || !nodep->funcp()->isCoroutine()) {
            puts("vlProcess");
        } లేకపోతే ఒకవేళ (లోపల_Process) {
            puts("std::make_shared<VlProcess>(vlProcess)");
        } లేకపోతే {
            puts("std::make_shared<VlProcess>()");
        }
        comma = నిజం;
    }
    ఒకవేళ (!nodep->argTypes().ఖాళీలు()) {
        ఒకవేళ (comma) puts(", ");
        puts(nodep->argTypes());
        comma = నిజం;
    }
    put_Comma_Iterate_తదుపరి(nodep->argsp(), comma);
    puts(")");
}

std::string EmitCFunc::dereferenceString(స్థిరరాశి std::string& సూచిక) స్థిరరాశి {
    ఒకవేళ (సూచిక[0] == '(' && సూచిక[1] == '&') {
        // remove "address of" followed by immediate dereference
        // Note: this relies on only the form '(&OBJECT)' being used by Verilator
        తిరిగి సూచిక.substr(2, సూచిక.నిడివి() - 3) + '.';
    } లేకపోతే {
        ఒకవేళ (సూచిక == "vlSelf" && m_usevlSelfRef) {
            తిరిగి "vlSelfRef.";
        } లేకపోతే {
            తిరిగి సూచిక + "->";
        }
    }
}
శూన్య EmitCFunc::emitDereference(Ast_కణుపు* nodep, స్థిరరాశి string& సూచిక) {
    putns(nodep, dereferenceString(సూచిక));
}

శూన్య EmitCFunc::emitCvtPackStr(Ast_కణుపు* nodep) {
    ఒకవేళ (స్థిరరాశి AstConst* స్థిరరాశి constp = VN_CAST(nodep, Const)) {
        emitConstantString(constp);
    } లేకపోతే ఒకవేళ (VN_IS(nodep->dtypep(), ప్రవాహం_DType)) {
        putnbs(nodep, "VL_CVT_PACK_STR_ND(");
        iterate_And_తదుపరి_Const_Null(nodep);
        puts(")");
    } లేకపోతే {
        putnbs(nodep, "VL_CVT_PACK_STR_N");
        emitIQW(nodep);
        puts("(");
        ఒకవేళ (nodep->isWide()) {
            // Note argument width, not node width (which is always 32)
            puts(cvt_గమ్యం_Str(nodep->వెడల్పు_పదాలు()));
            puts(", ");
        }
        iterate_And_తదుపరి_Const_Null(nodep);
        puts(")");
    }
}

శూన్య EmitCFunc::emitConstant(AstConst* nodep) {
    // Put out constant set to the specified variable, or given variable in a string
    స్థిరరాశి V3_సంఖ్య& అంకె = nodep->అంకె();
    ఒకవేళ (అంకె.isFourState()) {
        nodep->v3warn(E_UNSUPPORTED, "Unsupported: 4-state numbers in this context");
        తిరిగి;
    }
    putns(nodep, అంకె.emitC());
}

శూన్య EmitCFunc::emitConstantString(స్థిరరాశి AstConst* nodep) {
    // Const might be a Verilog array-type string, but need to always output std::string
    putnbs(nodep, "std::string{");
    స్థిరరాశి string str = nodep->అంకె().గమ్యం_String();
    ఒకవేళ (!str.ఖాళీలు()) putsQuoted(str);
    puts("}");
}

శూన్య EmitCFunc::emitVarReset(స్థిరరాశి string& ఉపసర్గ, AstVar* varp, తార్కిక constructing) {
    // 'constructing' indicates that the object was just constructed, so if it is a string or
    // something that starts off clear already, no need to clear it again
    Ast_కణుపు_DType* స్థిరరాశి dtypep = varp->dtypep()->skipRefp();
    స్థిరరాశి string vlSelf = VSelf_సూచిక_పాఠ్యం::replaceThis(m_useSelfForThis, "this->");
    స్థిరరాశి string varNameProtected
        = ((VN_IS(m_modp, Class) || varp->isFuncLocal()) || !ఉపసర్గ.ఖాళీలు())
              ? varName(varp)
              : vlSelf + varName(varp);
    స్థిరరాశి string new_ఉపసర్గ = ఉపసర్గ + varNameProtected;
    ఒకవేళ (varp->isIO() && m_modp->is_శిఖరం() && optSystemC()) {
        // System C top I/O doesn't need loading, as the lower level subinst code does it.}
    } లేకపోతే ఒకవేళ (varp->isParam()) {
        UASSERT_OBJ(varp->valuep(), varp, "No init for a param?");
        // If a simple CONST value we initialize it using an enum
        // If an ARRAYINIT we initialize it using an initial block similar to a signal
        // puts("// parameter "+varp->nameProtect()+" = "+varp->valuep()->name()+"\n");
    } లేకపోతే {
        putns(varp,
              emitVarResetRecurse(varp, constructing, new_ఉపసర్గ, dtypep, 0, "", varp->valuep()));
    }
}

string EmitCFunc::emitVarResetRecurse(స్థిరరాశి AstVar* varp, తార్కిక constructing,
                                      స్థిరరాశి string& varNameProtected, Ast_కణుపు_DType* dtypep,
                                      పూర్ణాంకం లోతు, స్థిరరాశి string& suffix, స్థిరరాశి Ast_కణుపు* valuep) {
    dtypep = dtypep->skipRefp();
    AstBasicDType* స్థిరరాశి basicp = dtypep->basicp();
    // Returns string to do resetting, empty to do nothing (which caller should handle)
    ఒకవేళ (AstAssocArrayDType* స్థిరరాశి adtypep = VN_CAST(dtypep, AssocArrayDType)) {
        // Access std::array as C array
        స్థిరరాశి string pre = constructing ? "" : varNameProtected + suffix + ".clear();\n";
        తిరిగి pre
               + emitVarResetRecurse(varp, constructing, varNameProtected, adtypep->subDTypep(),
                                     లోతు + 1, suffix + ".atDefault()", శూన్యసూచిక);
    } లేకపోతే ఒకవేళ (AstWildcardArrayDType* స్థిరరాశి adtypep = VN_CAST(dtypep, WildcardArrayDType)) {
        // Access std::array as C array
        స్థిరరాశి string pre = constructing ? "" : varNameProtected + suffix + ".clear();\n";
        తిరిగి pre
               + emitVarResetRecurse(varp, constructing, varNameProtected, adtypep->subDTypep(),
                                     లోతు + 1, suffix + ".atDefault()", శూన్యసూచిక);
    } లేకపోతే ఒకవేళ (VN_IS(dtypep, CDType)) {
        తిరిగి "";  // Constructor does it
    } లేకపోతే ఒకవేళ (VN_IS(dtypep, CoverCrossDType) || VN_IS(dtypep, CoverpointDType)) {
        తిరిగి "";  // Covergroup constructor creates the runtime and assigns the pointer
    } లేకపోతే ఒకవేళ (స్థిరరాశి AstClassRefDType* స్థిరరాశి adtypep = VN_CAST(dtypep, ClassRefDType)) {
        తిరిగి adtypep->raw_సూచిక() ? varNameProtected + suffix + " = nullptr;\n" : "";
    } లేకపోతే ఒకవేళ (VN_IS(dtypep, IfaceRefDType)) {
        తిరిగి varNameProtected + suffix + " = nullptr;\n";
    } లేకపోతే ఒకవేళ (స్థిరరాశి AstDynArrayDType* స్థిరరాశి adtypep = VN_CAST(dtypep, DynArrayDType)) {
        స్థిరరాశి string pre = constructing ? "" : varNameProtected + suffix + ".clear();\n";
        తిరిగి pre
               + emitVarResetRecurse(varp, constructing, varNameProtected, adtypep->subDTypep(),
                                     లోతు + 1, suffix + ".atDefault()", శూన్యసూచిక);
    } లేకపోతే ఒకవేళ (స్థిరరాశి Ast_వరుస_DType* స్థిరరాశి adtypep = VN_CAST(dtypep, వరుస_DType)) {
        స్థిరరాశి string pre = constructing ? "" : varNameProtected + suffix + ".clear();\n";
        తిరిగి pre
               + emitVarResetRecurse(varp, constructing, varNameProtected, adtypep->subDTypep(),
                                     లోతు + 1, suffix + ".atDefault()", శూన్యసూచిక);
    } లేకపోతే ఒకవేళ (VN_IS(dtypep, Sample_వరుస_DType)) {
        తిరిగి "";
    } లేకపోతే ఒకవేళ (స్థిరరాశి AstUnpackArrayDType* స్థిరరాశి adtypep = VN_CAST(dtypep, UnpackArrayDType)) {
        UASSERT_OBJ(adtypep->hi() >= adtypep->lo(), varp,
                    "Should have swapped msb & lsb earlier.");
        స్థిరరాశి Ast_కణుపు_DType* స్థిరరాశి elementp = adtypep->subDTypep()->skipRefp();
        ఒకవేళ (!varp->isFuncLocal() && elementp->isIntegralOrPacked()) {
            // Scoped reset has no array-index input or mutable RNG state. Initialize one
            // element, then fill in the optimized runtime even when this code is cold.
            స్థిరరాశి string మొదటి
                = emitVarResetRecurse(varp, constructing, varNameProtected, adtypep->subDTypep(),
                                      లోతు + 1, suffix + "[0]", శూన్యసూచిక);
            స్థిరరాశి string array = varNameProtected + suffix;
            తిరిగి మొదటి + "VL_UNPACKED_FILL(" + array + ".data(), sizeof(" + array + "[0]), "
                   + cvt_గమ్యం_Str(adtypep->elementsConst()) + ");\n";
        }
        స్థిరరాశి string ivar = "__Vi"s + cvt_గమ్యం_Str(లోతు);
        స్థిరరాశి string pre = ("for (int " + ivar + " = " + cvt_గమ్యం_Str(0) + "; " + ivar + " < "
                            + cvt_గమ్యం_Str(adtypep->elementsConst()) + "; ++" + ivar + ") {\n");
        స్థిరరాశి string క్రింద
            = emitVarResetRecurse(varp, constructing, varNameProtected, adtypep->subDTypep(),
                                  లోతు + 1, suffix + "[" + ivar + "]", శూన్యసూచిక);
        స్థిరరాశి string post = "}\n";
        తిరిగి క్రింద.ఖాళీలు() ? "" : pre + క్రింద + post;
    } లేకపోతే ఒకవేళ (VN_IS(dtypep, కణుపు_UOr_Struct_DType) && !VN_AS(dtypep, కణుపు_UOr_Struct_DType)->packed()) {
        స్థిరరాశి Ast_కణుపు_UOr_Struct_DType* స్థిరరాశి sdtypep = VN_AS(dtypep, కణుపు_UOr_Struct_DType);
        string literal;
        కొరకు (స్థిరరాశి AstMemberDType* itemp = sdtypep->membersp(); itemp;
             itemp = VN_AS(itemp->nextp(), MemberDType)) {
            స్థిరరాశి std::string పంక్తి = emitVarResetRecurse(
                varp, constructing, varNameProtected + suffix + "." + itemp->nameProtect(),
                itemp->dtypep(), లోతు + 1, "", itemp->valuep());
            ఒకవేళ (!పంక్తి.ఖాళీలు()) literal += పంక్తి;
        }
        తిరిగి literal;
    } లేకపోతే ఒకవేళ (basicp && basicp->keyword() == VBasicDTypeKwd::STRING) {
        ఒకవేళ (constructing) తిరిగి "";  // String's constructor deals with it
        తిరిగి varNameProtected + suffix + ".clear();\n";
    } లేకపోతే ఒకవేళ (basicp && basicp->isForkSync()) {
        తిరిగి "";
    } లేకపోతే ఒకవేళ (basicp && basicp->isProcessRef()) {
        తిరిగి "";
    } లేకపోతే ఒకవేళ (basicp && basicp->isDelayScheduler()) {
        తిరిగి "";
    } లేకపోతే ఒకవేళ (basicp && basicp->isTriggerScheduler()) {
        తిరిగి "";
    } లేకపోతే ఒకవేళ (basicp && basicp->isDynamicTriggerScheduler()) {
        తిరిగి "";
    } లేకపోతే ఒకవేళ (basicp && (basicp->isRandomGenerator() || basicp->isStdRandomGenerator())) {
        తిరిగి "";
    } లేకపోతే ఒకవేళ (basicp && basicp->isCovergroupInstHandle()) {
        // The handle's own constructor deals with it.
        తిరిగి "";
    } లేకపోతే ఒకవేళ (basicp && (basicp->isEvent())) {
        తిరిగి "VlAssignableEvent{};\n";
    } లేకపోతే ఒకవేళ (basicp) {
        స్థిరరాశి తార్కిక zeroit
            = (varp->attrFileDescr()  // Zero so we don't do file IO if never $fopen
               || varp->isFuncLocal()  // Randomization too slow
               || (basicp && basicp->isZeroInit())
               || (v3Global.opt.underlineZero() && !varp->name().ఖాళీలు() && varp->name()[0] == '_')
               || (varp->varType().is_తాత్కాలిక() && !varp->isXTemp())
               || (varp->isXTemp()
                       ? (v3Global.opt.x_కేటాయించు() != "unique")
                       : (v3Global.opt.xInitial() == "fast" || v3Global.opt.xInitial() == "0")));
        స్థిరరాశి తార్కిక slow = !varp->isFuncLocal() && !varp->isClassMember();
        విభజించు_కొలత_Inc(1);
        ఒకవేళ (dtypep->isWide()) {  // Handle unpacked; not basicp->isWide
            string బహిర్గతం;
            ఒకవేళ (valuep) {
                స్థిరరాశి AstConst* స్థిరరాశి constp = VN_AS(valuep, Const);
                UASSERT_OBJ(constp, varp, "non-const initializer for variable");
                కొరకు (పూర్ణాంకం w = 0; w < dtypep->వెడల్పు_పదాలు(); ++w) {
                    బహిర్గతం += varNameProtected + suffix + "[" + cvt_గమ్యం_Str(w) + "] = ";
                    బహిర్గతం += cvt_గమ్యం_Str(constp->అంకె().edata_మాట(w)) + "U;\n";
                }
            } లేకపోతే {
                బహిర్గతం += zeroit ? (slow ? "VL_ZERO_RESET_W(" : "VL_ZERO_W(")
                              : (varp->isXTemp() ? "VL_SCOPED_RAND_RESET_ASSIGN_W("
                                                 : "VL_SCOPED_RAND_RESET_W(");
                బహిర్గతం += cvt_గమ్యం_Str(dtypep->వెడల్పు_Min());
                బహిర్గతం += ", " + varNameProtected + suffix;
                ఒకవేళ (!zeroit) {
                    emitVarResetScopeHash();
                    స్థిరరాశి uint64_t salt = VString::hashMurmur(varp->prettyName());
                    బహిర్గతం += ", ";
                    బహిర్గతం += m_classOrPackage ? m_classOrPackageHash : "__VscopeHash";
                    బహిర్గతం += ", ";
                    బహిర్గతం += std::గమ్యం_string(salt);
                    బహిర్గతం += "ull";
                }
                బహిర్గతం += ");\n";
            }
            తిరిగి బహిర్గతం;
        } లేకపోతే {
            string బహిర్గతం = varNameProtected + suffix;
            ఒకవేళ (valuep) {
                బహిర్గతం += " = ";
                // TODO cleanup code shared between here, V3EmitCConstInit.h,
                // EmitCFunc::emitVarReset, EmitCFunc::emitConstant
                స్థిరరాశి AstConst* స్థిరరాశి constp = VN_AS(valuep, Const);
                UASSERT_OBJ(constp, varp, "non-const initializer for variable");
                బహిర్గతం += cvt_గమ్యం_Str(constp->అంకె().edata_మాట(0)) + "U;\n";
                బహిర్గతం += ";\n";
            } లేకపోతే ఒకవేళ (zeroit) {
                బహిర్గతం += " = 0;\n";
            } లేకపోతే {
                emitVarResetScopeHash();
                స్థిరరాశి uint64_t salt = VString::hashMurmur(varp->prettyName());
                బహిర్గతం += " = VL_SCOPED_RAND_RESET_";
                ఒకవేళ (varp->isXTemp()) బహిర్గతం += "ASSIGN_";
                బహిర్గతం += dtypep->charIQWN();
                బహిర్గతం += "(" + cvt_గమ్యం_Str(dtypep->వెడల్పు_Min()) + ", "
                       + (m_classOrPackage ? m_classOrPackageHash : "__VscopeHash") + ", "
                       + std::గమ్యం_string(salt) + "ull);\n";
            }
            తిరిగి బహిర్గతం;
        }
    } లేకపోతే {  // LCOV_EXCL_BR_LINE
        v3fatalSrc("Unknown node type in reset generator: " << varp->prettyTypeName());
    }
    తిరిగి "";
}

శూన్య EmitCFunc::emitVarResetScopeHash() {
    ఒకవేళ (VL_LIKELY(m_createdScopeHash)) { తిరిగి; }
    ఒకవేళ (m_classOrPackage) {
        m_classOrPackageHash
            = std::గమ్యం_string(VString::hashMurmur(m_classOrPackage->name())) + "ULL";
    } లేకపోతే {
        puts(string("const uint64_t __VscopeHash = VL_MURMUR64_HASH(")
             + (m_useSelfForThis ? "vlSelf" : "this") + "->vlNamep);\n");
    }
    m_createdScopeHash = నిజం;
}
