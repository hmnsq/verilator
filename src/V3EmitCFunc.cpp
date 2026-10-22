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

তার্কিক EmitCFunc::emitSimpleOk(Ast_গ্রন্থি_রাশিমালা* nodep) {
    // Can we put out a simple (A + B) instead of VL_ADD_III(A,B)?
    যদি (nodep->emitSimpleOperator() == "") প্রত্যাগম অসত্য;
    যদি (nodep->isWide()) প্রত্যাগম অসত্য;
    যদি (nodep->op1p() && nodep->op1p()->isWide()) প্রত্যাগম অসত্য;
    যদি (nodep->op2p() && nodep->op2p()->isWide()) প্রত্যাগম অসত্য;
    যদি (nodep->op3p() && nodep->op3p()->isWide()) প্রত্যাগম অসত্য;
    প্রত্যাগম সত্য;
}

শূন্য EmitCFunc::emitOpName(Ast_গ্রন্থি* nodep, ধ্রুব string& format, Ast_গ্রন্থি* lhsp, Ast_গ্রন্থি* rhsp,
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

    তার্কিক needComma = অসত্য;
    তার্কিক usesQueue = অসত্য;
    string nextComma;
    auto commaOut = [&out, &nextComma]() {
        যদি (!nextComma.empty()) {
            out += nextComma;
            nextComma = "";
        }
    };

    auto putOut = [এতত্, &out]() {
        যদি (!out.empty()) puts(out);
        out = "";
    };

    প্রতি (string::const_iterator pos = format.begin(); pos != format.end(); ++pos) {
        যদি (pos[0] == ',') {
            // Remember we need to add one, but don't do yet to avoid ",)"
            যদি (needComma) {
                যদি (pos[1] == ' ') {
                    nextComma = ", ";
                } অন্যথা {
                    nextComma = ",";
                }
                needComma = অসত্য;
            }
            যদি (pos[1] == ' ') ++pos;  // Must do even if no nextComma
        } অন্যথা যদি (pos[0] == '%') {
            ++pos;
            তার্কিক detail = অসত্য;
            Ast_গ্রন্থি* detailp = nullptr;
            বিকল্প (pos[0]) {
            পক্ষ '%': out += '%'; ভঙ্গ;
            পক্ষ 'k':
                putOut();
                putbs("");
                ভঙ্গ;
            পক্ষ 'n':
                detail = সত্য;
                detailp = nodep;
                ভঙ্গ;
            পক্ষ 'l':
                detail = সত্য;
                detailp = lhsp;
                ভঙ্গ;
            পক্ষ 'r':
                detail = সত্য;
                detailp = rhsp;
                ভঙ্গ;
            পক্ষ 't':
                detail = সত্য;
                detailp = thsp;
                ভঙ্গ;
            পক্ষ 'P':
                যদি (nodep->isWide() && !usesQueue) {
                    UASSERT_OBJ(m_wideTempRefp, nodep,
                                "Wide op " << nodep->prettyTypeName()
                                           << " w/ no temp, perhaps missing op in V3EmitC?");
                    commaOut();
                    putOut();
                    যদি (!m_wideTempRefp->selfPointer().isEmpty()) {
                        emitDereference(m_wideTempRefp,
                                        m_wideTempRefp->selfPointerProtect(m_useSelfForThis));
                    }
                    out += varName(m_wideTempRefp->varp());
                    m_wideTempRefp = nullptr;
                    needComma = সত্য;
                } অন্যথা যদি (usesQueue) {
                    commaOut();
                    putOut();
                    iterateAndNextConstNull(nodep->backp()->op2p());
                    needComma = সত্য;
                }

                ভঙ্গ;
            স্বভাব: nodep->v3fatalSrc("Unknown emitOperator format code: %" << pos[0]); ভঙ্গ;
            }
            যদি (detail) {
                // Get next letter of %[nlrt]
                ++pos;
                বিকল্প (pos[0]) {
                পক্ষ 'q':
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
                    ভঙ্গ;
                পক্ষ 'w':
                    commaOut();
                    out += cvtToStr(detailp->widthMin());
                    needComma = সত্য;
                    ভঙ্গ;
                পক্ষ 'W':
                    যদি (lhsp->isWide()) {
                        commaOut();
                        out += cvtToStr(lhsp->width_শব্দগুলি());
                        needComma = সত্য;
                    } অন্যথা যদি (VN_IS(lhsp, StreamR)) {
                        commaOut();
                        out += cvtToStr(rhsp->width_শব্দগুলি());
                        needComma = সত্য;
                    }
                    ভঙ্গ;
                পক্ষ 'i':
                    commaOut();
                    UASSERT_OBJ(detailp, nodep, "emitOperator() references undef node");
                    putOut();
                    iterateAndNextConstNull(detailp);
                    needComma = সত্য;
                    ভঙ্গ;
                স্বভাব:
                    nodep->v3fatalSrc("Unknown emitOperator format code: %[nlrt]" << pos[0]);
                    ভঙ্গ;
                }
            }
        } অন্যথা যদি (pos[0] == ')') {
            nextComma = "";
            out += ')';
        } অন্যথা যদি (pos[0] == '(') {
            commaOut();
            needComma = অসত্য;
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

তার্কিক EmitCFunc::displayEmitHeader(Ast_গ্রন্থি* nodep) {
    তার্কিক isStmt = অসত্য;
    যদি (ধ্রুব AstFScanF* ধ্রুব dispp = VN_CAST(nodep, FScanF)) {
        isStmt = অসত্য;
        putns(nodep, "VL_FSCANF_INX(");
        iterateConst(dispp->filep());
        puts(",");
    } অন্যথা যদি (ধ্রুব AstSScanF* ধ্রুব dispp = VN_CAST(nodep, SScanF)) {
        isStmt = অসত্য;
        putns(nodep, "VL_SSCANF_I");
        emitIQW(dispp->fromp());
        puts("NX(");
        puts(cvtToStr(dispp->fromp()->widthMin()));
        puts(",");
        iterateConst(dispp->fromp());
        puts(",");
    } অন্যথা যদি (ধ্রুব AstDisplay* ধ্রুব dispp = VN_CAST(nodep, Display)) {
        isStmt = সত্য;
        যদি (dispp->filep()) {
            putns(nodep, "VL_FWRITEF_NX(");
            iterateConst(dispp->filep());
            puts(",");
        } অন্যথা {
            putns(nodep, "VL_WRITEF_NX(");
        }
    } অন্যথা যদি (ধ্রুব AstSFormat* ধ্রুব dispp = VN_CAST(nodep, SFormat)) {
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
        isStmt = অসত্য;
        putns(nodep, "VL_SFORMATF_N_NX(");
    } অন্যথা {
        nodep->v3fatalSrc("Unknown displayEmit node type");
    }
    প্রত্যাগম isStmt;
}

শূন্য EmitCFunc::display_গ্রন্থি(Ast_গ্রন্থি* nodep, AstSFormatF* fmtp,  // fmtp is nullptr for AstScan
                            ধ্রুব string& vformat, Ast_গ্রন্থি* exprsp, তার্কিক isScan) {
    // Check format, if it exists
    ধ্রুব তার্কিক রাশিমালা_Format_2 = fmtp && fmtp->রাশিমালা_Format_2();
    তার্কিক needsScope = রাশিমালা_Format_2;
    তার্কিক needsTimescale = রাশিমালা_Format_2;
    পূর্ণাঙ্ক formatArgs = 0;
    যদি (!রাশিমালা_Format_2) {
        // Check display and argument count to head-off later runtime issues
        তার্কিক inPct = অসত্য;
        তার্কিক ignore = অসত্য;
        প্রতি (বর্ণ ch : vformat) {
            // UINFO(1, "Parse '" << *pos << "'  IP" << inPct << " List " << cvtToHex(elistp));
            যদি (!inPct && ch == '%') {
                inPct = সত্য;
                ignore = অসত্য;
            } অন্যথা যদি (inPct && (std::isdigit(ch) || ch == '.' || ch == '-')) {
            } অন্যথা যদি (!inPct) {  // Normal text
            } অন্যথা {  // Format character
                inPct = অসত্য;
                ch = std::tolower(ch);
                বিকল্প (ch) {
                পক্ষ '%': ভঙ্গ;
                পক্ষ '*':
                    inPct = সত্য;  // Get more digits
                    ignore = সত্য;
                    ভঙ্গ;
                পক্ষ 'm': needsScope = সত্য; ভঙ্গ;
                পক্ষ 't':
                    needsTimescale = সত্য;
                    ++formatArgs;
                    ভঙ্গ;
                স্বভাব:
                    যদি (!ignore && V3_সংখ্যা::displayedFmtHasArg(ch, isScan)) ++formatArgs;
                    ভঙ্গ;
                }
            }
        }
    }

    যদি (vformat.empty() && VN_IS(nodep, Display))  // not fscanf etc, as they need to return value
        প্রত্যাগম;  // NOP

    ধ্রুব তার্কিক isStmt = displayEmitHeader(nodep);

    যদি (রাশিমালা_Format_2) {
        UASSERT_OBJ(exprsp, nodep, "Missing format expression");
        iterateConst(exprsp);
        emitDatap(exprsp);
        exprsp = exprsp->nextp();
    } অন্যথা {
        ofp()->putsQuoted(vformat);
    }

    পূর্ণাঙ্ক রাশিমালা_Args = 0;
    প্রতি (Ast_গ্রন্থি* argp = exprsp; argp; argp = argp->nextp()) ++রাশিমালা_Args;
    যদি (VL_UNCOVERABLE(!রাশিমালা_Format_2 && রাশিমালা_Args != formatArgs)) {  // LCOV_EXCL_START
        // expectDisplay() checks this first, so probably internal error if found here
        nodep->v3error("Internal: Missing or extra arguments for $display-like format, "
                       << " expression had " << রাশিমালা_Args << ", format had " << formatArgs);
    }  // LCOV_EXCL_STOP

    // argc for error check. Also MSVC++ requires va_args to not be off a reference
    পূর্ণাঙ্ক argc = 0;
    যদি (needsScope) ++argc;
    যদি (needsTimescale) ++argc;
    প্রতি (Ast_গ্রন্থি* argp = exprsp; argp; argp = argp->nextp()) {
        ++argc;
        ধ্রুব AstSFormatArg* ধ্রুব fargp = VN_CAST(argp, SFormatArg);
        যদি (fargp && fargp->formatAttr().isEnum()) ++argc;  // Additional name argument
    }
    ofp()->puts("," + std::to_string(argc));

    যদি (needsScope) {
        AstScopeName* ধ্রুব scopenamep = fmtp ? fmtp->scopeNamep() : nullptr;
        UASSERT_OBJ(scopenamep, nodep, "Display with %m but no AstScopeName");
        ofp()->puts(", '"s + VFormatAttr{VFormatAttr::SCOPE}.ascii() + "'");
        যদি (scopenamep->selfRelative()) {
            // vlNamep is catName(name(), instance), so starts with '.' when the model is unnamed
            ধ্রুব string namep = string{m_useSelfForThis ? "vlSelf" : "this"} + "->vlNamep";
            ofp()->puts(",(vlSymsp->name()[0] ? " + namep + " : " + namep + " + 1),");
            ofp()->putsQuoted(scopenamep->scopeAttr());
        } অন্যথা {
            ofp()->puts(",vlSymsp->name(),");
            ofp()->putsQuoted(scopenamep->scopePrettySymName());
        }
    }

    যদি (needsTimescale) {
        VTimescale timeunit = VTimescale::NONE;
        যদি (ধ্রুব AstDisplay* ধ্রুব anodep = VN_CAST(nodep, Display)) {
            timeunit = anodep->fmtp()->timeunit();
        } অন্যথা যদি (ধ্রুব AstSFormat* ধ্রুব anodep = VN_CAST(nodep, SFormat)) {
            timeunit = anodep->fmtp()->timeunit();
        } অন্যথা যদি (ধ্রুব AstSScanF* ধ্রুব anodep = VN_CAST(nodep, SScanF)) {
            timeunit = anodep->timeunit();
        } অন্যথা যদি (ধ্রুব AstSFormatF* ধ্রুব anodep = VN_CAST(nodep, SFormatF)) {
            timeunit = anodep->timeunit();
        }
        UASSERT_OBJ(!timeunit.isNone(), nodep,
                    "Use of %t must be under AstDisplay, AstSFormat, or AstSFormatF, or "
                    "SScanF, and timeunit set");
        ofp()->puts(", '"s + VFormatAttr{VFormatAttr::TIMEUNIT}.ascii() + "'");
        ofp()->puts(","s + cvtToStr((পূর্ণাঙ্ক)timeunit.powerOfTen()));
    }

    // Arguments
    প্রতি (Ast_গ্রন্থি* argp = exprsp; argp; argp = argp->nextp()) {
        ofp()->indentInc();
        ofp()->putbs("");
        AstSFormatArg* ধ্রুব fargp = VN_CAST(argp, SFormatArg);
        Ast_গ্রন্থি* ধ্রুব subargp = fargp ? fargp->exprp() : argp;
        ধ্রুব VFormatAttr formatAttr = AstSFormatArg::formatAttrDefauled(fargp, subargp->dtypep());
        puts(", '"s + formatAttr.ascii() + '\'');
        যদি (formatAttr.isSigned() || formatAttr.isUnsigned() || formatAttr.isEnum())
            puts("," + cvtToStr(subargp->widthMin()));
        ধ্রুব তার্কিক addrof = isScan || formatAttr.isString() || formatAttr.isComplex();
        ধ্রুব তার্কিক wideEnum = formatAttr.isEnum() && subargp->isWide();
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

শূন্য EmitCFunc::emitCCallArgs(ধ্রুব Ast_গ্রন্থি_CCall* nodep, ধ্রুব string& selfPointer,
                              তার্কিক inProcess) {
    putns(nodep, "(");
    তার্কিক comma = অসত্য;
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
    putCommaIterateNext(nodep->argsp(), comma);
    puts(")");
}

std::string EmitCFunc::dereferenceString(ধ্রুব std::string& pointer) ধ্রুব {
    যদি (pointer[0] == '(' && pointer[1] == '&') {
        // remove "address of" followed by immediate dereference
        // Note: this relies on only the form '(&OBJECT)' being used by Verilator
        প্রত্যাগম pointer.substr(2, pointer.length() - 3) + '.';
    } অন্যথা {
        যদি (pointer == "vlSelf" && m_usevlSelfRef) {
            প্রত্যাগম "vlSelfRef.";
        } অন্যথা {
            প্রত্যাগম pointer + "->";
        }
    }
}
শূন্য EmitCFunc::emitDereference(Ast_গ্রন্থি* nodep, ধ্রুব string& pointer) {
    putns(nodep, dereferenceString(pointer));
}

শূন্য EmitCFunc::emitCvtPackStr(Ast_গ্রন্থি* nodep) {
    যদি (ধ্রুব AstConst* ধ্রুব constp = VN_CAST(nodep, Const)) {
        emitConstantString(constp);
    } অন্যথা যদি (VN_IS(nodep->dtypep(), StreamDType)) {
        putnbs(nodep, "VL_CVT_PACK_STR_ND(");
        iterateAndNextConstNull(nodep);
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
        iterateAndNextConstNull(nodep);
        puts(")");
    }
}

শূন্য EmitCFunc::emitConstant(AstConst* nodep) {
    // Put out constant set to the specified variable, or given variable in a string
    ধ্রুব V3_সংখ্যা& অঙ্ক = nodep->অঙ্ক();
    যদি (অঙ্ক.isFourState()) {
        nodep->v3warn(E_UNSUPPORTED, "Unsupported: 4-state numbers in this context");
        প্রত্যাগম;
    }
    putns(nodep, অঙ্ক.emitC());
}

শূন্য EmitCFunc::emitConstantString(ধ্রুব AstConst* nodep) {
    // Const might be a Verilog array-type string, but need to always output std::string
    putnbs(nodep, "std::string{");
    ধ্রুব string str = nodep->অঙ্ক().toString();
    যদি (!str.empty()) putsQuoted(str);
    puts("}");
}

শূন্য EmitCFunc::emitVarReset(ধ্রুব string& prefix, AstVar* varp, তার্কিক constructing) {
    // 'constructing' indicates that the object was just constructed, so if it is a string or
    // something that starts off clear already, no need to clear it again
    Ast_গ্রন্থি_DType* ধ্রুব dtypep = varp->dtypep()->skipRefp();
    ধ্রুব string vlSelf = VSelfPointerText::replaceThis(m_useSelfForThis, "this->");
    ধ্রুব string varNameProtected
        = ((VN_IS(m_modp, Class) || varp->isFuncLocal()) || !prefix.empty())
              ? varName(varp)
              : vlSelf + varName(varp);
    ধ্রুব string newPrefix = prefix + varNameProtected;
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

string EmitCFunc::emitVarResetRecurse(ধ্রুব AstVar* varp, তার্কিক constructing,
                                      ধ্রুব string& varNameProtected, Ast_গ্রন্থি_DType* dtypep,
                                      পূর্ণাঙ্ক depth, ধ্রুব string& suffix, ধ্রুব Ast_গ্রন্থি* valuep) {
    dtypep = dtypep->skipRefp();
    AstBasicDType* ধ্রুব basicp = dtypep->basicp();
    // Returns string to do resetting, empty to do nothing (which caller should handle)
    যদি (AstAssocArrayDType* ধ্রুব adtypep = VN_CAST(dtypep, AssocArrayDType)) {
        // Access std::array as C array
        ধ্রুব string pre = constructing ? "" : varNameProtected + suffix + ".clear();\n";
        প্রত্যাগম pre
               + emitVarResetRecurse(varp, constructing, varNameProtected, adtypep->subDTypep(),
                                     depth + 1, suffix + ".atDefault()", nullptr);
    } অন্যথা যদি (AstWildcardArrayDType* ধ্রুব adtypep = VN_CAST(dtypep, WildcardArrayDType)) {
        // Access std::array as C array
        ধ্রুব string pre = constructing ? "" : varNameProtected + suffix + ".clear();\n";
        প্রত্যাগম pre
               + emitVarResetRecurse(varp, constructing, varNameProtected, adtypep->subDTypep(),
                                     depth + 1, suffix + ".atDefault()", nullptr);
    } অন্যথা যদি (VN_IS(dtypep, CDType)) {
        প্রত্যাগম "";  // Constructor does it
    } অন্যথা যদি (VN_IS(dtypep, CoverCrossDType) || VN_IS(dtypep, CoverpointDType)) {
        প্রত্যাগম "";  // Covergroup constructor creates the runtime and assigns the pointer
    } অন্যথা যদি (ধ্রুব AstClassRefDType* ধ্রুব adtypep = VN_CAST(dtypep, ClassRefDType)) {
        প্রত্যাগম adtypep->rawPointer() ? varNameProtected + suffix + " = nullptr;\n" : "";
    } অন্যথা যদি (VN_IS(dtypep, IfaceRefDType)) {
        প্রত্যাগম varNameProtected + suffix + " = nullptr;\n";
    } অন্যথা যদি (ধ্রুব AstDynArrayDType* ধ্রুব adtypep = VN_CAST(dtypep, DynArrayDType)) {
        ধ্রুব string pre = constructing ? "" : varNameProtected + suffix + ".clear();\n";
        প্রত্যাগম pre
               + emitVarResetRecurse(varp, constructing, varNameProtected, adtypep->subDTypep(),
                                     depth + 1, suffix + ".atDefault()", nullptr);
    } অন্যথা যদি (ধ্রুব AstQueueDType* ধ্রুব adtypep = VN_CAST(dtypep, QueueDType)) {
        ধ্রুব string pre = constructing ? "" : varNameProtected + suffix + ".clear();\n";
        প্রত্যাগম pre
               + emitVarResetRecurse(varp, constructing, varNameProtected, adtypep->subDTypep(),
                                     depth + 1, suffix + ".atDefault()", nullptr);
    } অন্যথা যদি (VN_IS(dtypep, SampleQueueDType)) {
        প্রত্যাগম "";
    } অন্যথা যদি (ধ্রুব AstUnpackArrayDType* ধ্রুব adtypep = VN_CAST(dtypep, UnpackArrayDType)) {
        UASSERT_OBJ(adtypep->hi() >= adtypep->lo(), varp,
                    "Should have swapped msb & lsb earlier.");
        ধ্রুব Ast_গ্রন্থি_DType* ধ্রুব elementp = adtypep->subDTypep()->skipRefp();
        যদি (!varp->isFuncLocal() && elementp->isIntegralOrPacked()) {
            // Scoped reset has no array-index input or mutable RNG state. Initialize one
            // element, then fill in the optimized runtime even when this code is cold.
            ধ্রুব string first
                = emitVarResetRecurse(varp, constructing, varNameProtected, adtypep->subDTypep(),
                                      depth + 1, suffix + "[0]", nullptr);
            ধ্রুব string array = varNameProtected + suffix;
            প্রত্যাগম first + "VL_UNPACKED_FILL(" + array + ".data(), sizeof(" + array + "[0]), "
                   + cvtToStr(adtypep->elementsConst()) + ");\n";
        }
        ধ্রুব string ivar = "__Vi"s + cvtToStr(depth);
        ধ্রুব string pre = ("for (int " + ivar + " = " + cvtToStr(0) + "; " + ivar + " < "
                            + cvtToStr(adtypep->elementsConst()) + "; ++" + ivar + ") {\n");
        ধ্রুব string below
            = emitVarResetRecurse(varp, constructing, varNameProtected, adtypep->subDTypep(),
                                  depth + 1, suffix + "[" + ivar + "]", nullptr);
        ধ্রুব string post = "}\n";
        প্রত্যাগম below.empty() ? "" : pre + below + post;
    } অন্যথা যদি (VN_IS(dtypep, গ্রন্থি_UOr_Struct_DType) && !VN_AS(dtypep, গ্রন্থি_UOr_Struct_DType)->packed()) {
        ধ্রুব Ast_গ্রন্থি_UOr_Struct_DType* ধ্রুব sdtypep = VN_AS(dtypep, গ্রন্থি_UOr_Struct_DType);
        string literal;
        প্রতি (ধ্রুব AstMemberDType* itemp = sdtypep->membersp(); itemp;
             itemp = VN_AS(itemp->nextp(), MemberDType)) {
            ধ্রুব std::string সারি = emitVarResetRecurse(
                varp, constructing, varNameProtected + suffix + "." + itemp->nameProtect(),
                itemp->dtypep(), depth + 1, "", itemp->valuep());
            যদি (!সারি.empty()) literal += সারি;
        }
        প্রত্যাগম literal;
    } অন্যথা যদি (basicp && basicp->keyword() == VBasicDTypeKwd::STRING) {
        যদি (constructing) প্রত্যাগম "";  // String's constructor deals with it
        প্রত্যাগম varNameProtected + suffix + ".clear();\n";
    } অন্যথা যদি (basicp && basicp->isForkSync()) {
        প্রত্যাগম "";
    } অন্যথা যদি (basicp && basicp->isProcessRef()) {
        প্রত্যাগম "";
    } অন্যথা যদি (basicp && basicp->isDelayScheduler()) {
        প্রত্যাগম "";
    } অন্যথা যদি (basicp && basicp->isTriggerScheduler()) {
        প্রত্যাগম "";
    } অন্যথা যদি (basicp && basicp->isDynamicTriggerScheduler()) {
        প্রত্যাগম "";
    } অন্যথা যদি (basicp && (basicp->isRandomGenerator() || basicp->isStdRandomGenerator())) {
        প্রত্যাগম "";
    } অন্যথা যদি (basicp && basicp->isCovergroupInstHandle()) {
        // The handle's own constructor deals with it.
        প্রত্যাগম "";
    } অন্যথা যদি (basicp && (basicp->isEvent())) {
        প্রত্যাগম "VlAssignableEvent{};\n";
    } অন্যথা যদি (basicp) {
        ধ্রুব তার্কিক zeroit
            = (varp->attrFileDescr()  // Zero so we don't do file IO if never $fopen
               || varp->isFuncLocal()  // Randomization too slow
               || (basicp && basicp->isZeroInit())
               || (v3Global.opt.underlineZero() && !varp->name().empty() && varp->name()[0] == '_')
               || (varp->varType().isTemp() && !varp->isXTemp())
               || (varp->isXTemp()
                       ? (v3Global.opt.xAssign() != "unique")
                       : (v3Global.opt.xInitial() == "fast" || v3Global.opt.xInitial() == "0")));
        ধ্রুব তার্কিক slow = !varp->isFuncLocal() && !varp->isClassMember();
        splitSizeInc(1);
        যদি (dtypep->isWide()) {  // Handle unpacked; not basicp->isWide
            string out;
            যদি (valuep) {
                ধ্রুব AstConst* ধ্রুব constp = VN_AS(valuep, Const);
                UASSERT_OBJ(constp, varp, "non-const initializer for variable");
                প্রতি (পূর্ণাঙ্ক w = 0; w < dtypep->width_শব্দগুলি(); ++w) {
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
                    ধ্রুব uint64_t salt = VString::hashMurmur(varp->prettyName());
                    out += ", ";
                    out += m_classOrPackage ? m_classOrPackageHash : "__VscopeHash";
                    out += ", ";
                    out += std::to_string(salt);
                    out += "ull";
                }
                out += ");\n";
            }
            প্রত্যাগম out;
        } অন্যথা {
            string out = varNameProtected + suffix;
            যদি (valuep) {
                out += " = ";
                // TODO cleanup code shared between here, V3EmitCConstInit.h,
                // EmitCFunc::emitVarReset, EmitCFunc::emitConstant
                ধ্রুব AstConst* ধ্রুব constp = VN_AS(valuep, Const);
                UASSERT_OBJ(constp, varp, "non-const initializer for variable");
                out += cvtToStr(constp->অঙ্ক().edataWord(0)) + "U;\n";
                out += ";\n";
            } অন্যথা যদি (zeroit) {
                out += " = 0;\n";
            } অন্যথা {
                emitVarResetScopeHash();
                ধ্রুব uint64_t salt = VString::hashMurmur(varp->prettyName());
                out += " = VL_SCOPED_RAND_RESET_";
                যদি (varp->isXTemp()) out += "ASSIGN_";
                out += dtypep->charIQWN();
                out += "(" + cvtToStr(dtypep->widthMin()) + ", "
                       + (m_classOrPackage ? m_classOrPackageHash : "__VscopeHash") + ", "
                       + std::to_string(salt) + "ull);\n";
            }
            প্রত্যাগম out;
        }
    } অন্যথা {  // LCOV_EXCL_BR_LINE
        v3fatalSrc("Unknown node type in reset generator: " << varp->prettyTypeName());
    }
    প্রত্যাগম "";
}

শূন্য EmitCFunc::emitVarResetScopeHash() {
    যদি (VL_LIKELY(m_createdScopeHash)) { প্রত্যাগম; }
    যদি (m_classOrPackage) {
        m_classOrPackageHash
            = std::to_string(VString::hashMurmur(m_classOrPackage->name())) + "ULL";
    } অন্যথা {
        puts(string("const uint64_t __VscopeHash = VL_MURMUR64_HASH(")
             + (m_useSelfForThis ? "vlSelf" : "this") + "->vlNamep);\n");
    }
    m_createdScopeHash = সত্য;
}
