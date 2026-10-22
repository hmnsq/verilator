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

#অন্তর্ভাব "V3PchAstMT.h"

#অন্তর্ভাব "V3EmitCFunc.h"

#অন্তর্ভাব <map>
#অন্তর্ভাব <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// EmitCFunc

যৌক্তিক EmitCFunc::emitSimpleOk(Ast_গ্রন্থি_রাশিমালা* nodep) {
    // Can we put out a simple (A + B) instead of VL_ADD_III(A,B)?
    যদি (nodep->emitSimpleOperator() == "") ফেরত মিথ্যা;
    যদি (nodep->isWide()) ফেরত মিথ্যা;
    যদি (nodep->op1p() && nodep->op1p()->isWide()) ফেরত মিথ্যা;
    যদি (nodep->op2p() && nodep->op2p()->isWide()) ফেরত মিথ্যা;
    যদি (nodep->op3p() && nodep->op3p()->isWide()) ফেরত মিথ্যা;
    ফেরত সত্য;
}

শূন্য EmitCFunc::emitOpName(Ast_গ্রন্থি* nodep, ধ্রুবক string& format, Ast_গ্রন্থি* lhsp, Ast_গ্রন্থি* rhsp,
                           Ast_গ্রন্থি* thsp) {
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

    যৌক্তিক needComma = মিথ্যা;
    যৌক্তিক usesQueue = মিথ্যা;
    string পরবর্তী_Comma;
    auto commaOut = [&out, &পরবর্তী_Comma]() {
        যদি (!পরবর্তী_Comma.empty()) {
            out += পরবর্তী_Comma;
            পরবর্তী_Comma = "";
        }
    };

    auto putOut = [এই, &out]() {
        যদি (!out.empty()) puts(out);
        out = "";
    };

    জন্য (string::const_iterator pos = format.begin(); pos != format.end(); ++pos) {
        যদি (pos[0] == ',') {
            // Remember we need to add one, but don't do yet to avoid ",)"
            যদি (needComma) {
                যদি (pos[1] == ' ') {
                    পরবর্তী_Comma = ", ";
                } অন্যথা {
                    পরবর্তী_Comma = ",";
                }
                needComma = মিথ্যা;
            }
            যদি (pos[1] == ' ') ++pos;  // Must do even if no nextComma
        } অন্যথা যদি (pos[0] == '%') {
            ++pos;
            যৌক্তিক detail = মিথ্যা;
            Ast_গ্রন্থি* detailp = nullptr;
            নির্বাচন (pos[0]) {
            ক্ষেত্র '%': out += '%'; বিরতি;
            ক্ষেত্র 'k':
                putOut();
                putbs("");
                বিরতি;
            ক্ষেত্র 'n':
                detail = সত্য;
                detailp = nodep;
                বিরতি;
            ক্ষেত্র 'l':
                detail = সত্য;
                detailp = lhsp;
                বিরতি;
            ক্ষেত্র 'r':
                detail = সত্য;
                detailp = rhsp;
                বিরতি;
            ক্ষেত্র 't':
                detail = সত্য;
                detailp = thsp;
                বিরতি;
            ক্ষেত্র 'P':
                যদি (nodep->isWide() && !usesQueue) {
                    UASSERT_OBJ(m_wide_অস্থায়ী_Refp, nodep,
                                "Wide op " << nodep->prettyTypeName()
                                           << " w/ no temp, perhaps missing op in V3EmitC?");
                    commaOut();
                    putOut();
                    যদি (!m_wide_অস্থায়ী_Refp->selfPointer().isEmpty()) {
                        emitDereference(m_wide_অস্থায়ী_Refp,
                                        m_wide_অস্থায়ী_Refp->selfPointerProtect(m_useSelfForThis));
                    }
                    out += varName(m_wide_অস্থায়ী_Refp->varp());
                    m_wide_অস্থায়ী_Refp = nullptr;
                    needComma = সত্য;
                } অন্যথা যদি (usesQueue) {
                    commaOut();
                    putOut();
                    iterate_And_পরবর্তী_Const_Null(nodep->backp()->op2p());
                    needComma = সত্য;
                }

                বিরতি;
            পূর্বনির্ধারিত: nodep->v3fatalSrc("Unknown emitOperator format code: %" << pos[0]); বিরতি;
            }
            যদি (detail) {
                // Get next letter of %[nlrt]
                ++pos;
                নির্বাচন (pos[0]) {
                ক্ষেত্র 'q':
                    putOut();
                    // If we are assigning this to a queue we need to get the return type
                    যদি (VN_IS(detailp->backp(), Assign)
                        && VN_IS(detailp->backp()->op2p()->dtypep()->skipRefp(), QueueDType)) {
                        puts("R");  // R for queue
                        usesQueue = সত্য;
                    } অন্যথা যদি (VN_IS(detailp->dtypep()->skipRefp(), QueueDType)
                               || VN_IS(detailp->dtypep()->skipRefp(), StreamDType)) {
                        puts("R");  // R for queue
                    } অন্যথা {
                        emitIQW(detailp);
                    }
                    বিরতি;
                ক্ষেত্র 'w':
                    commaOut();
                    out += cvtToStr(detailp->widthMin());
                    needComma = সত্য;
                    বিরতি;
                ক্ষেত্র 'W':
                    যদি (lhsp->isWide()) {
                        commaOut();
                        out += cvtToStr(lhsp->width_শব্দগুলি());
                        needComma = সত্য;
                    } অন্যথা যদি (VN_IS(lhsp, StreamR)) {
                        commaOut();
                        out += cvtToStr(rhsp->width_শব্দগুলি());
                        needComma = সত্য;
                    }
                    বিরতি;
                ক্ষেত্র 'i':
                    commaOut();
                    UASSERT_OBJ(detailp, nodep, "emitOperator() references undef node");
                    putOut();
                    iterate_And_পরবর্তী_Const_Null(detailp);
                    needComma = সত্য;
                    বিরতি;
                পূর্বনির্ধারিত:
                    nodep->v3fatalSrc("Unknown emitOperator format code: %[nlrt]" << pos[0]);
                    বিরতি;
                }
            }
        } অন্যথা যদি (pos[0] == ')') {
            পরবর্তী_Comma = "";
            out += ')';
        } অন্যথা যদি (pos[0] == '(') {
            commaOut();
            needComma = মিথ্যা;
            out += '(';
        } অন্যথা {
            // Normal text
            যদি (std::isalnum(pos[0])) needComma = সত্য;
            commaOut();
            out += pos[0];
        }
    }
    putOut();
}

যৌক্তিক EmitCFunc::displayEmitHeader(Ast_গ্রন্থি* nodep) {
    যৌক্তিক isStmt = মিথ্যা;
    যদি (ধ্রুবক AstFScanF* ধ্রুবক dispp = VN_CAST(nodep, FScanF)) {
        isStmt = মিথ্যা;
        putns(nodep, "VL_FSCANF_INX(");
        iterateConst(dispp->filep());
        puts(",");
    } অন্যথা যদি (ধ্রুবক AstSScanF* ধ্রুবক dispp = VN_CAST(nodep, SScanF)) {
        isStmt = মিথ্যা;
        putns(nodep, "VL_SSCANF_I");
        emitIQW(dispp->fromp());
        puts("NX(");
        puts(cvtToStr(dispp->fromp()->widthMin()));
        puts(",");
        iterateConst(dispp->fromp());
        puts(",");
    } অন্যথা যদি (ধ্রুবক AstDisplay* ধ্রুবক dispp = VN_CAST(nodep, Display)) {
        isStmt = সত্য;
        যদি (dispp->filep()) {
            putns(nodep, "VL_FWRITEF_NX(");
            iterateConst(dispp->filep());
            puts(",");
        } অন্যথা {
            putns(nodep, "VL_WRITEF_NX(");
        }
    } অন্যথা যদি (ধ্রুবক AstSFormat* ধ্রুবক dispp = VN_CAST(nodep, SFormat)) {
        isStmt = সত্য;
        puts("VL_SFORMAT_NX(");
        যদি (!dispp->lhsp()->dtypep()->isString()) {
            puts(cvtToStr(dispp->lhsp()->widthMin()));
            putbs(",");
        }
        iterateConst(dispp->lhsp());
        emitDatap(dispp->lhsp());
        putbs(",");
    } অন্যথা যদি (VN_IS(nodep, SFormatF)) {
        isStmt = মিথ্যা;
        putns(nodep, "VL_SFORMATF_N_NX(");
    } অন্যথা {
        nodep->v3fatalSrc("Unknown displayEmit node type");
    }
    ফেরত isStmt;
}

শূন্য EmitCFunc::display_গ্রন্থি(Ast_গ্রন্থি* nodep, AstSFormatF* fmtp,  // fmtp is nullptr for AstScan
                            ধ্রুবক string& vformat, Ast_গ্রন্থি* exprsp, যৌক্তিক isScan) {
    // Check format, if it exists
    ধ্রুবক যৌক্তিক রাশিমালা_Format_2 = fmtp && fmtp->রাশিমালা_Format_2();
    যৌক্তিক needsScope = রাশিমালা_Format_2;
    যৌক্তিক needsTimescale = রাশিমালা_Format_2;
    পূর্ণসংখ্যা formatArgs = 0;
    যদি (!রাশিমালা_Format_2) {
        // Check display and argument count to head-off later runtime issues
        যৌক্তিক inPct = মিথ্যা;
        যৌক্তিক ignore = মিথ্যা;
        জন্য (অক্ষর ch : vformat) {
            // UINFO(1, "Parse '" << *pos << "'  IP" << inPct << " List " << cvtToHex(elistp));
            যদি (!inPct && ch == '%') {
                inPct = সত্য;
                ignore = মিথ্যা;
            } অন্যথা যদি (inPct && (std::isdigit(ch) || ch == '.' || ch == '-')) {
            } অন্যথা যদি (!inPct) {  // Normal text
            } অন্যথা {  // Format character
                inPct = মিথ্যা;
                ch = std::tolower(ch);
                নির্বাচন (ch) {
                ক্ষেত্র '%': বিরতি;
                ক্ষেত্র '*':
                    inPct = সত্য;  // Get more digits
                    ignore = সত্য;
                    বিরতি;
                ক্ষেত্র 'm': needsScope = সত্য; বিরতি;
                ক্ষেত্র 't':
                    needsTimescale = সত্য;
                    ++formatArgs;
                    বিরতি;
                পূর্বনির্ধারিত:
                    যদি (!ignore && V3_সংখ্যা::displayedFmtHasArg(ch, isScan)) ++formatArgs;
                    বিরতি;
                }
            }
        }
    }

    যদি (vformat.empty() && VN_IS(nodep, Display))  // not fscanf etc, as they need to return value
        ফেরত;  // NOP

    ধ্রুবক যৌক্তিক isStmt = displayEmitHeader(nodep);

    যদি (রাশিমালা_Format_2) {
        UASSERT_OBJ(exprsp, nodep, "Missing format expression");
        iterateConst(exprsp);
        emitDatap(exprsp);
        exprsp = exprsp->nextp();
    } অন্যথা {
        ofp()->putsQuoted(vformat);
    }

    পূর্ণসংখ্যা রাশিমালা_Args = 0;
    জন্য (Ast_গ্রন্থি* argp = exprsp; argp; argp = argp->nextp()) ++রাশিমালা_Args;
    যদি (VL_UNCOVERABLE(!রাশিমালা_Format_2 && রাশিমালা_Args != formatArgs)) {  // LCOV_EXCL_START
        // expectDisplay() checks this first, so probably internal error if found here
        nodep->v3error("Internal: Missing or extra arguments for $display-like format, "
                       << " expression had " << রাশিমালা_Args << ", format had " << formatArgs);
    }  // LCOV_EXCL_STOP

    // argc for error check. Also MSVC++ requires va_args to not be off a reference
    পূর্ণসংখ্যা argc = 0;
    যদি (needsScope) ++argc;
    যদি (needsTimescale) ++argc;
    জন্য (Ast_গ্রন্থি* argp = exprsp; argp; argp = argp->nextp()) {
        ++argc;
        ধ্রুবক AstSFormatArg* ধ্রুবক fargp = VN_CAST(argp, SFormatArg);
        যদি (fargp && fargp->formatAttr().isEnum()) ++argc;  // Additional name argument
    }
    ofp()->puts("," + std::to_string(argc));

    যদি (needsScope) {
        AstScopeName* ধ্রুবক scopenamep = fmtp ? fmtp->scopeNamep() : nullptr;
        UASSERT_OBJ(scopenamep, nodep, "Display with %m but no AstScopeName");
        ofp()->puts(", '"s + VFormatAttr{VFormatAttr::SCOPE}.ascii() + "'");
        যদি (scopenamep->selfRelative()) {
            // vlNamep is catName(name(), instance), so starts with '.' when the model is unnamed
            ধ্রুবক string namep = string{m_useSelfForThis ? "vlSelf" : "this"} + "->vlNamep";
            ofp()->puts(",(vlSymsp->name()[0] ? " + namep + " : " + namep + " + 1),");
            ofp()->putsQuoted(scopenamep->scopeAttr());
        } অন্যথা {
            ofp()->puts(",vlSymsp->name(),");
            ofp()->putsQuoted(scopenamep->scopePrettySymName());
        }
    }

    যদি (needsTimescale) {
        VTimescale timeunit = VTimescale::NONE;
        যদি (ধ্রুবক AstDisplay* ধ্রুবক anodep = VN_CAST(nodep, Display)) {
            timeunit = anodep->fmtp()->timeunit();
        } অন্যথা যদি (ধ্রুবক AstSFormat* ধ্রুবক anodep = VN_CAST(nodep, SFormat)) {
            timeunit = anodep->fmtp()->timeunit();
        } অন্যথা যদি (ধ্রুবক AstSScanF* ধ্রুবক anodep = VN_CAST(nodep, SScanF)) {
            timeunit = anodep->timeunit();
        } অন্যথা যদি (ধ্রুবক AstSFormatF* ধ্রুবক anodep = VN_CAST(nodep, SFormatF)) {
            timeunit = anodep->timeunit();
        }
        UASSERT_OBJ(!timeunit.isNone(), nodep,
                    "Use of %t must be under AstDisplay, AstSFormat, or AstSFormatF, or "
                    "SScanF, and timeunit set");
        ofp()->puts(", '"s + VFormatAttr{VFormatAttr::TIMEUNIT}.ascii() + "'");
        ofp()->puts(","s + cvtToStr((পূর্ণসংখ্যা)timeunit.powerOfTen()));
    }

    // Arguments
    জন্য (Ast_গ্রন্থি* argp = exprsp; argp; argp = argp->nextp()) {
        ofp()->indentInc();
        ofp()->putbs("");
        AstSFormatArg* ধ্রুবক fargp = VN_CAST(argp, SFormatArg);
        Ast_গ্রন্থি* ধ্রুবক subargp = fargp ? fargp->exprp() : argp;
        ধ্রুবক VFormatAttr formatAttr = AstSFormatArg::formatAttrDefauled(fargp, subargp->dtypep());
        puts(", '"s + formatAttr.ascii() + '\'');
        যদি (formatAttr.isSigned() || formatAttr.isUnsigned() || formatAttr.isEnum())
            puts("," + cvtToStr(subargp->widthMin()));
        ধ্রুবক যৌক্তিক addrof = isScan || formatAttr.isString() || formatAttr.isComplex();
        ধ্রুবক যৌক্তিক wideEnum = formatAttr.isEnum() && subargp->isWide();
        puts(",");
        যদি (wideEnum) puts("static_cast<const EData*>(");
        যদি (addrof) puts("&(");
        যদি (VN_IS(subargp, StreamR))
            emitStreamR(
                VN_CAST(subargp, StreamR),
                nodep);  // This has to be done here because streamR doesn't know what it returns
        অন্যথা { iterateConst(subargp); }
        যদি (addrof) puts(")");
        যদি (!addrof) emitDatap(argp);
        যদি (wideEnum) puts(")");
        যদি (formatAttr.isEnum()) {
            puts(", '"s + VFormatAttr{VFormatAttr::STRING}.ascii() + "', &(");
            iterateConst(fargp->namep());
            puts(")");
        }
        ofp()->indentDec();
    }

    // End
    যদি (isStmt) {
        puts(");\n");
    } অন্যথা {
        puts(") ");
    }
}

শূন্য EmitCFunc::emitCCallArgs(ধ্রুবক Ast_গ্রন্থি_CCall* nodep, ধ্রুবক string& selfPointer,
                              যৌক্তিক inProcess) {
    putns(nodep, "(");
    যৌক্তিক comma = মিথ্যা;
    যদি (nodep->funcp()->isLoose() && !nodep->funcp()->isStatic()) {
        UASSERT_OBJ(!selfPointer.empty(), nodep, "Call to loose method without self pointer");
        puts(selfPointer);
        comma = সত্য;
    }
    যদি (nodep->funcp()->needProcess()) {
        যদি (comma) puts(", ");
        যদি (VN_IS(nodep->backp(), CAwait) || !nodep->funcp()->isCoroutine()) {
            puts("vlProcess");
        } অন্যথা যদি (inProcess) {
            puts("std::make_shared<VlProcess>(vlProcess)");
        } অন্যথা {
            puts("std::make_shared<VlProcess>()");
        }
        comma = সত্য;
    }
    যদি (!nodep->argTypes().empty()) {
        যদি (comma) puts(", ");
        puts(nodep->argTypes());
        comma = সত্য;
    }
    put_Comma_Iterate_পরবর্তী(nodep->argsp(), comma);
    puts(")");
}

std::string EmitCFunc::dereferenceString(ধ্রুবক std::string& pointer) ধ্রুবক {
    যদি (pointer[0] == '(' && pointer[1] == '&') {
        // remove "address of" followed by immediate dereference
        // Note: this relies on only the form '(&OBJECT)' being used by Verilator
        ফেরত pointer.substr(2, pointer.দৈর্ঘ্য() - 3) + '.';
    } অন্যথা {
        যদি (pointer == "vlSelf" && m_usevlSelfRef) {
            ফেরত "vlSelfRef.";
        } অন্যথা {
            ফেরত pointer + "->";
        }
    }
}
শূন্য EmitCFunc::emitDereference(Ast_গ্রন্থি* nodep, ধ্রুবক string& pointer) {
    putns(nodep, dereferenceString(pointer));
}

শূন্য EmitCFunc::emitCvtPackStr(Ast_গ্রন্থি* nodep) {
    যদি (ধ্রুবক AstConst* ধ্রুবক constp = VN_CAST(nodep, Const)) {
        emitConstantString(constp);
    } অন্যথা যদি (VN_IS(nodep->dtypep(), StreamDType)) {
        putnbs(nodep, "VL_CVT_PACK_STR_ND(");
        iterate_And_পরবর্তী_Const_Null(nodep);
        puts(")");
    } অন্যথা {
        putnbs(nodep, "VL_CVT_PACK_STR_N");
        emitIQW(nodep);
        puts("(");
        যদি (nodep->isWide()) {
            // Note argument width, not node width (which is always 32)
            puts(cvtToStr(nodep->width_শব্দগুলি()));
            puts(", ");
        }
        iterate_And_পরবর্তী_Const_Null(nodep);
        puts(")");
    }
}

শূন্য EmitCFunc::emitConstant(AstConst* nodep) {
    // Put out constant set to the specified variable, or given variable in a string
    ধ্রুবক V3_সংখ্যা& অঙ্ক = nodep->অঙ্ক();
    যদি (অঙ্ক.isFourState()) {
        nodep->v3warn(E_UNSUPPORTED, "Unsupported: 4-state numbers in this context");
        ফেরত;
    }
    putns(nodep, অঙ্ক.emitC());
}

শূন্য EmitCFunc::emitConstantString(ধ্রুবক AstConst* nodep) {
    // Const might be a Verilog array-type string, but need to always output std::string
    putnbs(nodep, "std::string{");
    ধ্রুবক string str = nodep->অঙ্ক().toString();
    যদি (!str.empty()) putsQuoted(str);
    puts("}");
}

শূন্য EmitCFunc::emitVarReset(ধ্রুবক string& prefix, AstVar* varp, যৌক্তিক constructing) {
    // 'constructing' indicates that the object was just constructed, so if it is a string or
    // something that starts off clear already, no need to clear it again
    Ast_গ্রন্থি_DType* ধ্রুবক dtypep = varp->dtypep()->skipRefp();
    ধ্রুবক string vlSelf = VSelfPointerText::replaceThis(m_useSelfForThis, "this->");
    ধ্রুবক string varNameProtected
        = ((VN_IS(m_modp, Class) || varp->isFuncLocal()) || !prefix.empty())
              ? varName(varp)
              : vlSelf + varName(varp);
    ধ্রুবক string newPrefix = prefix + varNameProtected;
    যদি (varp->isIO() && m_modp->is_শীর্ষ() && optSystemC()) {
        // System C top I/O doesn't need loading, as the lower level subinst code does it.}
    } অন্যথা যদি (varp->isParam()) {
        UASSERT_OBJ(varp->valuep(), varp, "No init for a param?");
        // If a simple CONST value we initialize it using an enum
        // If an ARRAYINIT we initialize it using an initial block similar to a signal
        // puts("// parameter "+varp->nameProtect()+" = "+varp->valuep()->name()+"\n");
    } অন্যথা {
        putns(varp,
              emitVarResetRecurse(varp, constructing, newPrefix, dtypep, 0, "", varp->valuep()));
    }
}

string EmitCFunc::emitVarResetRecurse(ধ্রুবক AstVar* varp, যৌক্তিক constructing,
                                      ধ্রুবক string& varNameProtected, Ast_গ্রন্থি_DType* dtypep,
                                      পূর্ণসংখ্যা depth, ধ্রুবক string& suffix, ধ্রুবক Ast_গ্রন্থি* valuep) {
    dtypep = dtypep->skipRefp();
    AstBasicDType* ধ্রুবক basicp = dtypep->basicp();
    // Returns string to do resetting, empty to do nothing (which caller should handle)
    যদি (AstAssocArrayDType* ধ্রুবক adtypep = VN_CAST(dtypep, AssocArrayDType)) {
        // Access std::array as C array
        ধ্রুবক string pre = constructing ? "" : varNameProtected + suffix + ".clear();\n";
        ফেরত pre
               + emitVarResetRecurse(varp, constructing, varNameProtected, adtypep->subDTypep(),
                                     depth + 1, suffix + ".atDefault()", nullptr);
    } অন্যথা যদি (AstWildcardArrayDType* ধ্রুবক adtypep = VN_CAST(dtypep, WildcardArrayDType)) {
        // Access std::array as C array
        ধ্রুবক string pre = constructing ? "" : varNameProtected + suffix + ".clear();\n";
        ফেরত pre
               + emitVarResetRecurse(varp, constructing, varNameProtected, adtypep->subDTypep(),
                                     depth + 1, suffix + ".atDefault()", nullptr);
    } অন্যথা যদি (VN_IS(dtypep, CDType)) {
        ফেরত "";  // Constructor does it
    } অন্যথা যদি (VN_IS(dtypep, CoverCrossDType) || VN_IS(dtypep, CoverpointDType)) {
        ফেরত "";  // Covergroup constructor creates the runtime and assigns the pointer
    } অন্যথা যদি (ধ্রুবক AstClassRefDType* ধ্রুবক adtypep = VN_CAST(dtypep, ClassRefDType)) {
        ফেরত adtypep->rawPointer() ? varNameProtected + suffix + " = nullptr;\n" : "";
    } অন্যথা যদি (VN_IS(dtypep, IfaceRefDType)) {
        ফেরত varNameProtected + suffix + " = nullptr;\n";
    } অন্যথা যদি (ধ্রুবক AstDynArrayDType* ধ্রুবক adtypep = VN_CAST(dtypep, DynArrayDType)) {
        ধ্রুবক string pre = constructing ? "" : varNameProtected + suffix + ".clear();\n";
        ফেরত pre
               + emitVarResetRecurse(varp, constructing, varNameProtected, adtypep->subDTypep(),
                                     depth + 1, suffix + ".atDefault()", nullptr);
    } অন্যথা যদি (ধ্রুবক AstQueueDType* ধ্রুবক adtypep = VN_CAST(dtypep, QueueDType)) {
        ধ্রুবক string pre = constructing ? "" : varNameProtected + suffix + ".clear();\n";
        ফেরত pre
               + emitVarResetRecurse(varp, constructing, varNameProtected, adtypep->subDTypep(),
                                     depth + 1, suffix + ".atDefault()", nullptr);
    } অন্যথা যদি (VN_IS(dtypep, SampleQueueDType)) {
        ফেরত "";
    } অন্যথা যদি (ধ্রুবক AstUnpackArrayDType* ধ্রুবক adtypep = VN_CAST(dtypep, UnpackArrayDType)) {
        UASSERT_OBJ(adtypep->hi() >= adtypep->lo(), varp,
                    "Should have swapped msb & lsb earlier.");
        ধ্রুবক Ast_গ্রন্থি_DType* ধ্রুবক elementp = adtypep->subDTypep()->skipRefp();
        যদি (!varp->isFuncLocal() && elementp->isIntegralOrPacked()) {
            // Scoped reset has no array-index input or mutable RNG state. Initialize one
            // element, then fill in the optimized runtime even when this code is cold.
            ধ্রুবক string first
                = emitVarResetRecurse(varp, constructing, varNameProtected, adtypep->subDTypep(),
                                      depth + 1, suffix + "[0]", nullptr);
            ধ্রুবক string array = varNameProtected + suffix;
            ফেরত first + "VL_UNPACKED_FILL(" + array + ".data(), sizeof(" + array + "[0]), "
                   + cvtToStr(adtypep->elementsConst()) + ");\n";
        }
        ধ্রুবক string ivar = "__Vi"s + cvtToStr(depth);
        ধ্রুবক string pre = ("for (int " + ivar + " = " + cvtToStr(0) + "; " + ivar + " < "
                            + cvtToStr(adtypep->elementsConst()) + "; ++" + ivar + ") {\n");
        ধ্রুবক string below
            = emitVarResetRecurse(varp, constructing, varNameProtected, adtypep->subDTypep(),
                                  depth + 1, suffix + "[" + ivar + "]", nullptr);
        ধ্রুবক string post = "}\n";
        ফেরত below.empty() ? "" : pre + below + post;
    } অন্যথা যদি (VN_IS(dtypep, গ্রন্থি_UOr_Struct_DType) && !VN_AS(dtypep, গ্রন্থি_UOr_Struct_DType)->packed()) {
        ধ্রুবক Ast_গ্রন্থি_UOr_Struct_DType* ধ্রুবক sdtypep = VN_AS(dtypep, গ্রন্থি_UOr_Struct_DType);
        string literal;
        জন্য (ধ্রুবক AstMemberDType* itemp = sdtypep->membersp(); itemp;
             itemp = VN_AS(itemp->nextp(), MemberDType)) {
            ধ্রুবক std::string সারি = emitVarResetRecurse(
                varp, constructing, varNameProtected + suffix + "." + itemp->nameProtect(),
                itemp->dtypep(), depth + 1, "", itemp->valuep());
            যদি (!সারি.empty()) literal += সারি;
        }
        ফেরত literal;
    } অন্যথা যদি (basicp && basicp->keyword() == VBasicDTypeKwd::STRING) {
        যদি (constructing) ফেরত "";  // String's constructor deals with it
        ফেরত varNameProtected + suffix + ".clear();\n";
    } অন্যথা যদি (basicp && basicp->isForkSync()) {
        ফেরত "";
    } অন্যথা যদি (basicp && basicp->isProcessRef()) {
        ফেরত "";
    } অন্যথা যদি (basicp && basicp->isDelayScheduler()) {
        ফেরত "";
    } অন্যথা যদি (basicp && basicp->isTriggerScheduler()) {
        ফেরত "";
    } অন্যথা যদি (basicp && basicp->isDynamicTriggerScheduler()) {
        ফেরত "";
    } অন্যথা যদি (basicp && (basicp->isRandomGenerator() || basicp->isStdRandomGenerator())) {
        ফেরত "";
    } অন্যথা যদি (basicp && basicp->isCovergroupInstHandle()) {
        // The handle's own constructor deals with it.
        ফেরত "";
    } অন্যথা যদি (basicp && (basicp->isEvent())) {
        ফেরত "VlAssignableEvent{};\n";
    } অন্যথা যদি (basicp) {
        ধ্রুবক যৌক্তিক zeroit
            = (varp->attrFileDescr()  // Zero so we don't do file IO if never $fopen
               || varp->isFuncLocal()  // Randomization too slow
               || (basicp && basicp->isZeroInit())
               || (v3Global.opt.underlineZero() && !varp->name().empty() && varp->name()[0] == '_')
               || (varp->varType().is_অস্থায়ী() && !varp->isXTemp())
               || (varp->isXTemp()
                       ? (v3Global.opt.xAssign() != "unique")
                       : (v3Global.opt.xInitial() == "fast" || v3Global.opt.xInitial() == "0")));
        ধ্রুবক যৌক্তিক slow = !varp->isFuncLocal() && !varp->isClassMember();
        split_আকার_Inc(1);
        যদি (dtypep->isWide()) {  // Handle unpacked; not basicp->isWide
            string out;
            যদি (valuep) {
                ধ্রুবক AstConst* ধ্রুবক constp = VN_AS(valuep, Const);
                UASSERT_OBJ(constp, varp, "non-const initializer for variable");
                জন্য (পূর্ণসংখ্যা w = 0; w < dtypep->width_শব্দগুলি(); ++w) {
                    out += varNameProtected + suffix + "[" + cvtToStr(w) + "] = ";
                    out += cvtToStr(constp->অঙ্ক().edataWord(w)) + "U;\n";
                }
            } অন্যথা {
                out += zeroit ? (slow ? "VL_ZERO_RESET_W(" : "VL_ZERO_W(")
                              : (varp->isXTemp() ? "VL_SCOPED_RAND_RESET_ASSIGN_W("
                                                 : "VL_SCOPED_RAND_RESET_W(");
                out += cvtToStr(dtypep->widthMin());
                out += ", " + varNameProtected + suffix;
                যদি (!zeroit) {
                    emitVarResetScopeHash();
                    ধ্রুবক uint64_t salt = VString::hashMurmur(varp->prettyName());
                    out += ", ";
                    out += m_classOrPackage ? m_classOrPackageHash : "__VscopeHash";
                    out += ", ";
                    out += std::to_string(salt);
                    out += "ull";
                }
                out += ");\n";
            }
            ফেরত out;
        } অন্যথা {
            string out = varNameProtected + suffix;
            যদি (valuep) {
                out += " = ";
                // TODO cleanup code shared between here, V3EmitCConstInit.h,
                // EmitCFunc::emitVarReset, EmitCFunc::emitConstant
                ধ্রুবক AstConst* ধ্রুবক constp = VN_AS(valuep, Const);
                UASSERT_OBJ(constp, varp, "non-const initializer for variable");
                out += cvtToStr(constp->অঙ্ক().edataWord(0)) + "U;\n";
                out += ";\n";
            } অন্যথা যদি (zeroit) {
                out += " = 0;\n";
            } অন্যথা {
                emitVarResetScopeHash();
                ধ্রুবক uint64_t salt = VString::hashMurmur(varp->prettyName());
                out += " = VL_SCOPED_RAND_RESET_";
                যদি (varp->isXTemp()) out += "ASSIGN_";
                out += dtypep->charIQWN();
                out += "(" + cvtToStr(dtypep->widthMin()) + ", "
                       + (m_classOrPackage ? m_classOrPackageHash : "__VscopeHash") + ", "
                       + std::to_string(salt) + "ull);\n";
            }
            ফেরত out;
        }
    } অন্যথা {  // LCOV_EXCL_BR_LINE
        v3fatalSrc("Unknown node type in reset generator: " << varp->prettyTypeName());
    }
    ফেরত "";
}

শূন্য EmitCFunc::emitVarResetScopeHash() {
    যদি (VL_LIKELY(m_createdScopeHash)) { ফেরত; }
    যদি (m_classOrPackage) {
        m_classOrPackageHash
            = std::to_string(VString::hashMurmur(m_classOrPackage->name())) + "ULL";
    } অন্যথা {
        puts(string("const uint64_t __VscopeHash = VL_MURMUR64_HASH(")
             + (m_useSelfForThis ? "vlSelf" : "this") + "->vlNamep);\n");
    }
    m_createdScopeHash = সত্য;
}
