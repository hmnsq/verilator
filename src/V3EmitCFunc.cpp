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

#অন্তর্ভাৱ "V3PchAstMT.h"

#অন্তর্ভাৱ "V3EmitCFunc.h"

#অন্তর্ভাৱ <map>
#অন্তর্ভাৱ <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// EmitCFunc

তার্কিক EmitCFunc::emitSimpleOk(Ast_গ্ৰন্থি_ৰাশিমালা* nodep) {
    // Can we put out a simple (A + B) instead of VL_ADD_III(A,B)?
    যদি (nodep->emitSimpleOperator() == "") ঘূৰাই মিছা;
    যদি (nodep->isWide()) ঘূৰাই মিছা;
    যদি (nodep->op1p() && nodep->op1p()->isWide()) ঘূৰাই মিছা;
    যদি (nodep->op2p() && nodep->op2p()->isWide()) ঘূৰাই মিছা;
    যদি (nodep->op3p() && nodep->op3p()->isWide()) ঘূৰাই মিছা;
    ঘূৰাই সঁচা;
}

শূন্য EmitCFunc::emitOpName(Ast_গ্ৰন্থি* nodep, ধ্রুৱ string& format, Ast_গ্ৰন্থি* lhsp, Ast_গ্ৰন্থি* rhsp,
                           Ast_গ্ৰন্থি* thsp) {
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

    তার্কিক needComma = মিছা;
    তার্কিক usesQueue = মিছা;
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

    বাবে (string::const_iterator pos = format.begin(); pos != format.end(); ++pos) {
        যদি (pos[0] == ',') {
            // Remember we need to add one, but don't do yet to avoid ",)"
            যদি (needComma) {
                যদি (pos[1] == ' ') {
                    nextComma = ", ";
                } অন্যথা {
                    nextComma = ",";
                }
                needComma = মিছা;
            }
            যদি (pos[1] == ' ') ++pos;  // Must do even if no nextComma
        } অন্যথা যদি (pos[0] == '%') {
            ++pos;
            তার্কিক detail = মিছা;
            Ast_গ্ৰন্থি* detailp = nullptr;
            ৱিকল্প (pos[0]) {
            পক্ষ '%': out += '%'; ভঙ্গ;
            পক্ষ 'k':
                putOut();
                putbs("");
                ভঙ্গ;
            পক্ষ 'n':
                detail = সঁচা;
                detailp = nodep;
                ভঙ্গ;
            পক্ষ 'l':
                detail = সঁচা;
                detailp = lhsp;
                ভঙ্গ;
            পক্ষ 'r':
                detail = সঁচা;
                detailp = rhsp;
                ভঙ্গ;
            পক্ষ 't':
                detail = সঁচা;
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
                    needComma = সঁচা;
                } অন্যথা যদি (usesQueue) {
                    commaOut();
                    putOut();
                    iterateAndNextConstNull(nodep->backp()->op2p());
                    needComma = সঁচা;
                }

                ভঙ্গ;
            স্ৱভাৱ: nodep->v3fatalSrc("Unknown emitOperator format code: %" << pos[0]); ভঙ্গ;
            }
            যদি (detail) {
                // Get next letter of %[nlrt]
                ++pos;
                ৱিকল্প (pos[0]) {
                পক্ষ 'q':
                    putOut();
                    // If we are assigning this to a queue we need to get the return type
                    যদি (VN_IS(detailp->backp(), Assign)
                        && VN_IS(detailp->backp()->op2p()->dtypep()->skipRefp(), QueueDType)) {
                        puts("R");  // R for queue
                        usesQueue = সঁচা;
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
                    needComma = সঁচা;
                    ভঙ্গ;
                পক্ষ 'W':
                    যদি (lhsp->isWide()) {
                        commaOut();
                        out += cvtToStr(lhsp->width_শব্দগুলি());
                        needComma = সঁচা;
                    } অন্যথা যদি (VN_IS(lhsp, StreamR)) {
                        commaOut();
                        out += cvtToStr(rhsp->width_শব্দগুলি());
                        needComma = সঁচা;
                    }
                    ভঙ্গ;
                পক্ষ 'i':
                    commaOut();
                    UASSERT_OBJ(detailp, nodep, "emitOperator() references undef node");
                    putOut();
                    iterateAndNextConstNull(detailp);
                    needComma = সঁচা;
                    ভঙ্গ;
                স্ৱভাৱ:
                    nodep->v3fatalSrc("Unknown emitOperator format code: %[nlrt]" << pos[0]);
                    ভঙ্গ;
                }
            }
        } অন্যথা যদি (pos[0] == ')') {
            nextComma = "";
            out += ')';
        } অন্যথা যদি (pos[0] == '(') {
            commaOut();
            needComma = মিছা;
            out += '(';
        } অন্যথা {
            // Normal text
            যদি (std::isalnum(pos[0])) needComma = সঁচা;
            commaOut();
            out += pos[0];
        }
    }
    putOut();
}

তার্কিক EmitCFunc::displayEmitHeader(Ast_গ্ৰন্থি* nodep) {
    তার্কিক isStmt = মিছা;
    যদি (ধ্রুৱ AstFScanF* ধ্রুৱ dispp = VN_CAST(nodep, FScanF)) {
        isStmt = মিছা;
        putns(nodep, "VL_FSCANF_INX(");
        iterateConst(dispp->filep());
        puts(",");
    } অন্যথা যদি (ধ্রুৱ AstSScanF* ধ্রুৱ dispp = VN_CAST(nodep, SScanF)) {
        isStmt = মিছা;
        putns(nodep, "VL_SSCANF_I");
        emitIQW(dispp->fromp());
        puts("NX(");
        puts(cvtToStr(dispp->fromp()->widthMin()));
        puts(",");
        iterateConst(dispp->fromp());
        puts(",");
    } অন্যথা যদি (ধ্রুৱ AstDisplay* ধ্রুৱ dispp = VN_CAST(nodep, Display)) {
        isStmt = সঁচা;
        যদি (dispp->filep()) {
            putns(nodep, "VL_FWRITEF_NX(");
            iterateConst(dispp->filep());
            puts(",");
        } অন্যথা {
            putns(nodep, "VL_WRITEF_NX(");
        }
    } অন্যথা যদি (ধ্রুৱ AstSFormat* ধ্রুৱ dispp = VN_CAST(nodep, SFormat)) {
        isStmt = সঁচা;
        puts("VL_SFORMAT_NX(");
        যদি (!dispp->lhsp()->dtypep()->isString()) {
            puts(cvtToStr(dispp->lhsp()->widthMin()));
            putbs(",");
        }
        iterateConst(dispp->lhsp());
        emitDatap(dispp->lhsp());
        putbs(",");
    } অন্যথা যদি (VN_IS(nodep, SFormatF)) {
        isStmt = মিছা;
        putns(nodep, "VL_SFORMATF_N_NX(");
    } অন্যথা {
        nodep->v3fatalSrc("Unknown displayEmit node type");
    }
    ঘূৰাই isStmt;
}

শূন্য EmitCFunc::display_গ্ৰন্থি(Ast_গ্ৰন্থি* nodep, AstSFormatF* fmtp,  // fmtp is nullptr for AstScan
                            ধ্রুৱ string& vformat, Ast_গ্ৰন্থি* exprsp, তার্কিক isScan) {
    // Check format, if it exists
    ধ্রুৱ তার্কিক ৰাশিমালা_Format_2 = fmtp && fmtp->ৰাশিমালা_Format_2();
    তার্কিক needsScope = ৰাশিমালা_Format_2;
    তার্কিক needsTimescale = ৰাশিমালা_Format_2;
    পূর্ণাঙ্ক formatArgs = 0;
    যদি (!ৰাশিমালা_Format_2) {
        // Check display and argument count to head-off later runtime issues
        তার্কিক inPct = মিছা;
        তার্কিক ignore = মিছা;
        বাবে (ৱর্ণ ch : vformat) {
            // UINFO(1, "Parse '" << *pos << "'  IP" << inPct << " List " << cvtToHex(elistp));
            যদি (!inPct && ch == '%') {
                inPct = সঁচা;
                ignore = মিছা;
            } অন্যথা যদি (inPct && (std::isdigit(ch) || ch == '.' || ch == '-')) {
            } অন্যথা যদি (!inPct) {  // Normal text
            } অন্যথা {  // Format character
                inPct = মিছা;
                ch = std::tolower(ch);
                ৱিকল্প (ch) {
                পক্ষ '%': ভঙ্গ;
                পক্ষ '*':
                    inPct = সঁচা;  // Get more digits
                    ignore = সঁচা;
                    ভঙ্গ;
                পক্ষ 'm': needsScope = সঁচা; ভঙ্গ;
                পক্ষ 't':
                    needsTimescale = সঁচা;
                    ++formatArgs;
                    ভঙ্গ;
                স্ৱভাৱ:
                    যদি (!ignore && V3_সংখ্যা::displayedFmtHasArg(ch, isScan)) ++formatArgs;
                    ভঙ্গ;
                }
            }
        }
    }

    যদি (vformat.empty() && VN_IS(nodep, Display))  // not fscanf etc, as they need to return value
        ঘূৰাই;  // NOP

    ধ্রুৱ তার্কিক isStmt = displayEmitHeader(nodep);

    যদি (ৰাশিমালা_Format_2) {
        UASSERT_OBJ(exprsp, nodep, "Missing format expression");
        iterateConst(exprsp);
        emitDatap(exprsp);
        exprsp = exprsp->nextp();
    } অন্যথা {
        ofp()->putsQuoted(vformat);
    }

    পূর্ণাঙ্ক ৰাশিমালা_Args = 0;
    বাবে (Ast_গ্ৰন্থি* argp = exprsp; argp; argp = argp->nextp()) ++ৰাশিমালা_Args;
    যদি (VL_UNCOVERABLE(!ৰাশিমালা_Format_2 && ৰাশিমালা_Args != formatArgs)) {  // LCOV_EXCL_START
        // expectDisplay() checks this first, so probably internal error if found here
        nodep->v3error("Internal: Missing or extra arguments for $display-like format, "
                       << " expression had " << ৰাশিমালা_Args << ", format had " << formatArgs);
    }  // LCOV_EXCL_STOP

    // argc for error check. Also MSVC++ requires va_args to not be off a reference
    পূর্ণাঙ্ক argc = 0;
    যদি (needsScope) ++argc;
    যদি (needsTimescale) ++argc;
    বাবে (Ast_গ্ৰন্থি* argp = exprsp; argp; argp = argp->nextp()) {
        ++argc;
        ধ্রুৱ AstSFormatArg* ধ্রুৱ fargp = VN_CAST(argp, SFormatArg);
        যদি (fargp && fargp->formatAttr().isEnum()) ++argc;  // Additional name argument
    }
    ofp()->puts("," + std::to_string(argc));

    যদি (needsScope) {
        AstScopeName* ধ্রুৱ scopenamep = fmtp ? fmtp->scopeNamep() : nullptr;
        UASSERT_OBJ(scopenamep, nodep, "Display with %m but no AstScopeName");
        ofp()->puts(", '"s + VFormatAttr{VFormatAttr::SCOPE}.ascii() + "'");
        যদি (scopenamep->selfRelative()) {
            // vlNamep is catName(name(), instance), so starts with '.' when the model is unnamed
            ধ্রুৱ string namep = string{m_useSelfForThis ? "vlSelf" : "this"} + "->vlNamep";
            ofp()->puts(",(vlSymsp->name()[0] ? " + namep + " : " + namep + " + 1),");
            ofp()->putsQuoted(scopenamep->scopeAttr());
        } অন্যথা {
            ofp()->puts(",vlSymsp->name(),");
            ofp()->putsQuoted(scopenamep->scopePrettySymName());
        }
    }

    যদি (needsTimescale) {
        VTimescale timeunit = VTimescale::NONE;
        যদি (ধ্রুৱ AstDisplay* ধ্রুৱ anodep = VN_CAST(nodep, Display)) {
            timeunit = anodep->fmtp()->timeunit();
        } অন্যথা যদি (ধ্রুৱ AstSFormat* ধ্রুৱ anodep = VN_CAST(nodep, SFormat)) {
            timeunit = anodep->fmtp()->timeunit();
        } অন্যথা যদি (ধ্রুৱ AstSScanF* ধ্রুৱ anodep = VN_CAST(nodep, SScanF)) {
            timeunit = anodep->timeunit();
        } অন্যথা যদি (ধ্রুৱ AstSFormatF* ধ্রুৱ anodep = VN_CAST(nodep, SFormatF)) {
            timeunit = anodep->timeunit();
        }
        UASSERT_OBJ(!timeunit.isNone(), nodep,
                    "Use of %t must be under AstDisplay, AstSFormat, or AstSFormatF, or "
                    "SScanF, and timeunit set");
        ofp()->puts(", '"s + VFormatAttr{VFormatAttr::TIMEUNIT}.ascii() + "'");
        ofp()->puts(","s + cvtToStr((পূর্ণাঙ্ক)timeunit.powerOfTen()));
    }

    // Arguments
    বাবে (Ast_গ্ৰন্থি* argp = exprsp; argp; argp = argp->nextp()) {
        ofp()->indentInc();
        ofp()->putbs("");
        AstSFormatArg* ধ্রুৱ fargp = VN_CAST(argp, SFormatArg);
        Ast_গ্ৰন্থি* ধ্রুৱ subargp = fargp ? fargp->exprp() : argp;
        ধ্রুৱ VFormatAttr formatAttr = AstSFormatArg::formatAttrDefauled(fargp, subargp->dtypep());
        puts(", '"s + formatAttr.ascii() + '\'');
        যদি (formatAttr.isSigned() || formatAttr.isUnsigned() || formatAttr.isEnum())
            puts("," + cvtToStr(subargp->widthMin()));
        ধ্রুৱ তার্কিক addrof = isScan || formatAttr.isString() || formatAttr.isComplex();
        ধ্রুৱ তার্কিক wideEnum = formatAttr.isEnum() && subargp->isWide();
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

শূন্য EmitCFunc::emitCCallArgs(ধ্রুৱ Ast_গ্ৰন্থি_CCall* nodep, ধ্রুৱ string& selfPointer,
                              তার্কিক inProcess) {
    putns(nodep, "(");
    তার্কিক comma = মিছা;
    যদি (nodep->funcp()->isLoose() && !nodep->funcp()->isStatic()) {
        UASSERT_OBJ(!selfPointer.empty(), nodep, "Call to loose method without self pointer");
        puts(selfPointer);
        comma = সঁচা;
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
        comma = সঁচা;
    }
    যদি (!nodep->argTypes().empty()) {
        যদি (comma) puts(", ");
        puts(nodep->argTypes());
        comma = সঁচা;
    }
    putCommaIterateNext(nodep->argsp(), comma);
    puts(")");
}

std::string EmitCFunc::dereferenceString(ধ্রুৱ std::string& pointer) ধ্রুৱ {
    যদি (pointer[0] == '(' && pointer[1] == '&') {
        // remove "address of" followed by immediate dereference
        // Note: this relies on only the form '(&OBJECT)' being used by Verilator
        ঘূৰাই pointer.substr(2, pointer.length() - 3) + '.';
    } অন্যথা {
        যদি (pointer == "vlSelf" && m_usevlSelfRef) {
            ঘূৰাই "vlSelfRef.";
        } অন্যথা {
            ঘূৰাই pointer + "->";
        }
    }
}
শূন্য EmitCFunc::emitDereference(Ast_গ্ৰন্থি* nodep, ধ্রুৱ string& pointer) {
    putns(nodep, dereferenceString(pointer));
}

শূন্য EmitCFunc::emitCvtPackStr(Ast_গ্ৰন্থি* nodep) {
    যদি (ধ্রুৱ AstConst* ধ্রুৱ constp = VN_CAST(nodep, Const)) {
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
    ধ্রুৱ V3_সংখ্যা& অঙ্ক = nodep->অঙ্ক();
    যদি (অঙ্ক.isFourState()) {
        nodep->v3warn(E_UNSUPPORTED, "Unsupported: 4-state numbers in this context");
        ঘূৰাই;
    }
    putns(nodep, অঙ্ক.emitC());
}

শূন্য EmitCFunc::emitConstantString(ধ্রুৱ AstConst* nodep) {
    // Const might be a Verilog array-type string, but need to always output std::string
    putnbs(nodep, "std::string{");
    ধ্রুৱ string str = nodep->অঙ্ক().toString();
    যদি (!str.empty()) putsQuoted(str);
    puts("}");
}

শূন্য EmitCFunc::emitVarReset(ধ্রুৱ string& prefix, AstVar* varp, তার্কিক constructing) {
    // 'constructing' indicates that the object was just constructed, so if it is a string or
    // something that starts off clear already, no need to clear it again
    Ast_গ্ৰন্থি_DType* ধ্রুৱ dtypep = varp->dtypep()->skipRefp();
    ধ্রুৱ string vlSelf = VSelfPointerText::replaceThis(m_useSelfForThis, "this->");
    ধ্রুৱ string varNameProtected
        = ((VN_IS(m_modp, Class) || varp->isFuncLocal()) || !prefix.empty())
              ? varName(varp)
              : vlSelf + varName(varp);
    ধ্রুৱ string newPrefix = prefix + varNameProtected;
    যদি (varp->isIO() && m_modp->is_শীৰ্ষ() && optSystemC()) {
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

string EmitCFunc::emitVarResetRecurse(ধ্রুৱ AstVar* varp, তার্কিক constructing,
                                      ধ্রুৱ string& varNameProtected, Ast_গ্ৰন্থি_DType* dtypep,
                                      পূর্ণাঙ্ক depth, ধ্রুৱ string& suffix, ধ্রুৱ Ast_গ্ৰন্থি* valuep) {
    dtypep = dtypep->skipRefp();
    AstBasicDType* ধ্রুৱ basicp = dtypep->basicp();
    // Returns string to do resetting, empty to do nothing (which caller should handle)
    যদি (AstAssocArrayDType* ধ্রুৱ adtypep = VN_CAST(dtypep, AssocArrayDType)) {
        // Access std::array as C array
        ধ্রুৱ string pre = constructing ? "" : varNameProtected + suffix + ".clear();\n";
        ঘূৰাই pre
               + emitVarResetRecurse(varp, constructing, varNameProtected, adtypep->subDTypep(),
                                     depth + 1, suffix + ".atDefault()", nullptr);
    } অন্যথা যদি (AstWildcardArrayDType* ধ্রুৱ adtypep = VN_CAST(dtypep, WildcardArrayDType)) {
        // Access std::array as C array
        ধ্রুৱ string pre = constructing ? "" : varNameProtected + suffix + ".clear();\n";
        ঘূৰাই pre
               + emitVarResetRecurse(varp, constructing, varNameProtected, adtypep->subDTypep(),
                                     depth + 1, suffix + ".atDefault()", nullptr);
    } অন্যথা যদি (VN_IS(dtypep, CDType)) {
        ঘূৰাই "";  // Constructor does it
    } অন্যথা যদি (VN_IS(dtypep, CoverCrossDType) || VN_IS(dtypep, CoverpointDType)) {
        ঘূৰাই "";  // Covergroup constructor creates the runtime and assigns the pointer
    } অন্যথা যদি (ধ্রুৱ AstClassRefDType* ধ্রুৱ adtypep = VN_CAST(dtypep, ClassRefDType)) {
        ঘূৰাই adtypep->rawPointer() ? varNameProtected + suffix + " = nullptr;\n" : "";
    } অন্যথা যদি (VN_IS(dtypep, IfaceRefDType)) {
        ঘূৰাই varNameProtected + suffix + " = nullptr;\n";
    } অন্যথা যদি (ধ্রুৱ AstDynArrayDType* ধ্রুৱ adtypep = VN_CAST(dtypep, DynArrayDType)) {
        ধ্রুৱ string pre = constructing ? "" : varNameProtected + suffix + ".clear();\n";
        ঘূৰাই pre
               + emitVarResetRecurse(varp, constructing, varNameProtected, adtypep->subDTypep(),
                                     depth + 1, suffix + ".atDefault()", nullptr);
    } অন্যথা যদি (ধ্রুৱ AstQueueDType* ধ্রুৱ adtypep = VN_CAST(dtypep, QueueDType)) {
        ধ্রুৱ string pre = constructing ? "" : varNameProtected + suffix + ".clear();\n";
        ঘূৰাই pre
               + emitVarResetRecurse(varp, constructing, varNameProtected, adtypep->subDTypep(),
                                     depth + 1, suffix + ".atDefault()", nullptr);
    } অন্যথা যদি (VN_IS(dtypep, SampleQueueDType)) {
        ঘূৰাই "";
    } অন্যথা যদি (ধ্রুৱ AstUnpackArrayDType* ধ্রুৱ adtypep = VN_CAST(dtypep, UnpackArrayDType)) {
        UASSERT_OBJ(adtypep->hi() >= adtypep->lo(), varp,
                    "Should have swapped msb & lsb earlier.");
        ধ্রুৱ Ast_গ্ৰন্থি_DType* ধ্রুৱ elementp = adtypep->subDTypep()->skipRefp();
        যদি (!varp->isFuncLocal() && elementp->isIntegralOrPacked()) {
            // Scoped reset has no array-index input or mutable RNG state. Initialize one
            // element, then fill in the optimized runtime even when this code is cold.
            ধ্রুৱ string first
                = emitVarResetRecurse(varp, constructing, varNameProtected, adtypep->subDTypep(),
                                      depth + 1, suffix + "[0]", nullptr);
            ধ্রুৱ string array = varNameProtected + suffix;
            ঘূৰাই first + "VL_UNPACKED_FILL(" + array + ".data(), sizeof(" + array + "[0]), "
                   + cvtToStr(adtypep->elementsConst()) + ");\n";
        }
        ধ্রুৱ string ivar = "__Vi"s + cvtToStr(depth);
        ধ্রুৱ string pre = ("for (int " + ivar + " = " + cvtToStr(0) + "; " + ivar + " < "
                            + cvtToStr(adtypep->elementsConst()) + "; ++" + ivar + ") {\n");
        ধ্রুৱ string below
            = emitVarResetRecurse(varp, constructing, varNameProtected, adtypep->subDTypep(),
                                  depth + 1, suffix + "[" + ivar + "]", nullptr);
        ধ্রুৱ string post = "}\n";
        ঘূৰাই below.empty() ? "" : pre + below + post;
    } অন্যথা যদি (VN_IS(dtypep, গ্ৰন্থি_UOr_Struct_DType) && !VN_AS(dtypep, গ্ৰন্থি_UOr_Struct_DType)->packed()) {
        ধ্রুৱ Ast_গ্ৰন্থি_UOr_Struct_DType* ধ্রুৱ sdtypep = VN_AS(dtypep, গ্ৰন্থি_UOr_Struct_DType);
        string literal;
        বাবে (ধ্রুৱ AstMemberDType* itemp = sdtypep->membersp(); itemp;
             itemp = VN_AS(itemp->nextp(), MemberDType)) {
            ধ্রুৱ std::string সাৰি = emitVarResetRecurse(
                varp, constructing, varNameProtected + suffix + "." + itemp->nameProtect(),
                itemp->dtypep(), depth + 1, "", itemp->valuep());
            যদি (!সাৰি.empty()) literal += সাৰি;
        }
        ঘূৰাই literal;
    } অন্যথা যদি (basicp && basicp->keyword() == VBasicDTypeKwd::STRING) {
        যদি (constructing) ঘূৰাই "";  // String's constructor deals with it
        ঘূৰাই varNameProtected + suffix + ".clear();\n";
    } অন্যথা যদি (basicp && basicp->isForkSync()) {
        ঘূৰাই "";
    } অন্যথা যদি (basicp && basicp->isProcessRef()) {
        ঘূৰাই "";
    } অন্যথা যদি (basicp && basicp->isDelayScheduler()) {
        ঘূৰাই "";
    } অন্যথা যদি (basicp && basicp->isTriggerScheduler()) {
        ঘূৰাই "";
    } অন্যথা যদি (basicp && basicp->isDynamicTriggerScheduler()) {
        ঘূৰাই "";
    } অন্যথা যদি (basicp && (basicp->isRandomGenerator() || basicp->isStdRandomGenerator())) {
        ঘূৰাই "";
    } অন্যথা যদি (basicp && basicp->isCovergroupInstHandle()) {
        // The handle's own constructor deals with it.
        ঘূৰাই "";
    } অন্যথা যদি (basicp && (basicp->isEvent())) {
        ঘূৰাই "VlAssignableEvent{};\n";
    } অন্যথা যদি (basicp) {
        ধ্রুৱ তার্কিক zeroit
            = (varp->attrFileDescr()  // Zero so we don't do file IO if never $fopen
               || varp->isFuncLocal()  // Randomization too slow
               || (basicp && basicp->isZeroInit())
               || (v3Global.opt.underlineZero() && !varp->name().empty() && varp->name()[0] == '_')
               || (varp->varType().isTemp() && !varp->isXTemp())
               || (varp->isXTemp()
                       ? (v3Global.opt.xAssign() != "unique")
                       : (v3Global.opt.xInitial() == "fast" || v3Global.opt.xInitial() == "0")));
        ধ্রুৱ তার্কিক slow = !varp->isFuncLocal() && !varp->isClassMember();
        splitSizeInc(1);
        যদি (dtypep->isWide()) {  // Handle unpacked; not basicp->isWide
            string out;
            যদি (valuep) {
                ধ্রুৱ AstConst* ধ্রুৱ constp = VN_AS(valuep, Const);
                UASSERT_OBJ(constp, varp, "non-const initializer for variable");
                বাবে (পূর্ণাঙ্ক w = 0; w < dtypep->width_শব্দগুলি(); ++w) {
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
                    ধ্রুৱ uint64_t salt = VString::hashMurmur(varp->prettyName());
                    out += ", ";
                    out += m_classOrPackage ? m_classOrPackageHash : "__VscopeHash";
                    out += ", ";
                    out += std::to_string(salt);
                    out += "ull";
                }
                out += ");\n";
            }
            ঘূৰাই out;
        } অন্যথা {
            string out = varNameProtected + suffix;
            যদি (valuep) {
                out += " = ";
                // TODO cleanup code shared between here, V3EmitCConstInit.h,
                // EmitCFunc::emitVarReset, EmitCFunc::emitConstant
                ধ্রুৱ AstConst* ধ্রুৱ constp = VN_AS(valuep, Const);
                UASSERT_OBJ(constp, varp, "non-const initializer for variable");
                out += cvtToStr(constp->অঙ্ক().edataWord(0)) + "U;\n";
                out += ";\n";
            } অন্যথা যদি (zeroit) {
                out += " = 0;\n";
            } অন্যথা {
                emitVarResetScopeHash();
                ধ্রুৱ uint64_t salt = VString::hashMurmur(varp->prettyName());
                out += " = VL_SCOPED_RAND_RESET_";
                যদি (varp->isXTemp()) out += "ASSIGN_";
                out += dtypep->charIQWN();
                out += "(" + cvtToStr(dtypep->widthMin()) + ", "
                       + (m_classOrPackage ? m_classOrPackageHash : "__VscopeHash") + ", "
                       + std::to_string(salt) + "ull);\n";
            }
            ঘূৰাই out;
        }
    } অন্যথা {  // LCOV_EXCL_BR_LINE
        v3fatalSrc("Unknown node type in reset generator: " << varp->prettyTypeName());
    }
    ঘূৰাই "";
}

শূন্য EmitCFunc::emitVarResetScopeHash() {
    যদি (VL_LIKELY(m_createdScopeHash)) { ঘূৰাই; }
    যদি (m_classOrPackage) {
        m_classOrPackageHash
            = std::to_string(VString::hashMurmur(m_classOrPackage->name())) + "ULL";
    } অন্যথা {
        puts(string("const uint64_t __VscopeHash = VL_MURMUR64_HASH(")
             + (m_useSelfForThis ? "vlSelf" : "this") + "->vlNamep);\n");
    }
    m_createdScopeHash = সঁচা;
}
