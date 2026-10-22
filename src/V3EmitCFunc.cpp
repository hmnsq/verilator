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

منطقي EmitCFunc::emitSimpleOk(AstNodeExpr* nodep) {
    // Can we put out a simple (A + B) instead of VL_ADD_III(A,B)?
    𐤀𐤌 (nodep->emitSimpleOperator() == "") أرجع خطأ;
    𐤀𐤌 (nodep->isWide()) أرجع خطأ;
    𐤀𐤌 (nodep->op1p() && nodep->op1p()->isWide()) أرجع خطأ;
    𐤀𐤌 (nodep->op2p() && nodep->op2p()->isWide()) أرجع خطأ;
    𐤀𐤌 (nodep->op3p() && nodep->op3p()->isWide()) أرجع خطأ;
    أرجع true;
}

فراغ EmitCFunc::emitOpName(AstNode* nodep, ثابت string& format, AstNode* lhsp, AstNode* rhsp,
                           AstNode* thsp) {
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

    منطقي needComma = خطأ;
    منطقي usesQueue = خطأ;
    string nextComma;
    auto commaOut = [&out, &nextComma]() {
        𐤀𐤌 (!nextComma.empty()) {
            out += nextComma;
            nextComma = "";
        }
    };

    auto putOut = [هذا, &out]() {
        𐤀𐤌 (!out.empty()) puts(out);
        out = "";
    };

    لكل (string::const_iterator pos = format.begin(); pos != format.end(); ++pos) {
        𐤀𐤌 (pos[0] == ',') {
            // Remember we need to add one, but don't do yet to avoid ",)"
            𐤀𐤌 (needComma) {
                𐤀𐤌 (pos[1] == ' ') {
                    nextComma = ", ";
                } وإلا {
                    nextComma = ",";
                }
                needComma = خطأ;
            }
            𐤀𐤌 (pos[1] == ' ') ++pos;  // Must do even if no nextComma
        } وإلا 𐤀𐤌 (pos[0] == '%') {
            ++pos;
            منطقي detail = خطأ;
            AstNode* detailp = nullptr;
            بدل (pos[0]) {
            حالة '%': out += '%'; اكسر;
            حالة 'k':
                putOut();
                putbs("");
                اكسر;
            حالة 'n':
                detail = true;
                detailp = nodep;
                اكسر;
            حالة 'l':
                detail = true;
                detailp = lhsp;
                اكسر;
            حالة 'r':
                detail = true;
                detailp = rhsp;
                اكسر;
            حالة 't':
                detail = true;
                detailp = thsp;
                اكسر;
            حالة 'P':
                𐤀𐤌 (nodep->isWide() && !usesQueue) {
                    UASSERT_OBJ(m_wideTempRefp, nodep,
                                "Wide op " << nodep->prettyTypeName()
                                           << " w/ no temp, perhaps missing op in V3EmitC?");
                    commaOut();
                    putOut();
                    𐤀𐤌 (!m_wideTempRefp->selfPointer().isEmpty()) {
                        emitDereference(m_wideTempRefp,
                                        m_wideTempRefp->selfPointerProtect(m_useSelfForThis));
                    }
                    out += varName(m_wideTempRefp->varp());
                    m_wideTempRefp = nullptr;
                    needComma = true;
                } وإلا 𐤀𐤌 (usesQueue) {
                    commaOut();
                    putOut();
                    iterateAndNextConstNull(nodep->backp()->op2p());
                    needComma = true;
                }

                اكسر;
            افتراضي: nodep->v3fatalSrc("Unknown emitOperator format code: %" << pos[0]); اكسر;
            }
            𐤀𐤌 (detail) {
                // Get next letter of %[nlrt]
                ++pos;
                بدل (pos[0]) {
                حالة 'q':
                    putOut();
                    // If we are assigning this to a queue we need to get the return type
                    𐤀𐤌 (VN_IS(detailp->backp(), Assign)
                        && VN_IS(detailp->backp()->op2p()->dtypep()->skipRefp(), QueueDType)) {
                        puts("R");  // R for queue
                        usesQueue = true;
                    } وإلا 𐤀𐤌 (VN_IS(detailp->dtypep()->skipRefp(), QueueDType)
                               || VN_IS(detailp->dtypep()->skipRefp(), StreamDType)) {
                        puts("R");  // R for queue
                    } وإلا {
                        emitIQW(detailp);
                    }
                    اكسر;
                حالة 'w':
                    commaOut();
                    out += cvtToStr(detailp->widthMin());
                    needComma = true;
                    اكسر;
                حالة 'W':
                    𐤀𐤌 (lhsp->isWide()) {
                        commaOut();
                        out += cvtToStr(lhsp->widthWords());
                        needComma = true;
                    } وإلا 𐤀𐤌 (VN_IS(lhsp, StreamR)) {
                        commaOut();
                        out += cvtToStr(rhsp->widthWords());
                        needComma = true;
                    }
                    اكسر;
                حالة 'i':
                    commaOut();
                    UASSERT_OBJ(detailp, nodep, "emitOperator() references undef node");
                    putOut();
                    iterateAndNextConstNull(detailp);
                    needComma = true;
                    اكسر;
                افتراضي:
                    nodep->v3fatalSrc("Unknown emitOperator format code: %[nlrt]" << pos[0]);
                    اكسر;
                }
            }
        } وإلا 𐤀𐤌 (pos[0] == ')') {
            nextComma = "";
            out += ')';
        } وإلا 𐤀𐤌 (pos[0] == '(') {
            commaOut();
            needComma = خطأ;
            out += '(';
        } وإلا {
            // Normal text
            𐤀𐤌 (std::isalnum(pos[0])) needComma = true;
            commaOut();
            out += pos[0];
        }
    }
    putOut();
}

منطقي EmitCFunc::displayEmitHeader(AstNode* nodep) {
    منطقي isStmt = خطأ;
    𐤀𐤌 (ثابت AstFScanF* ثابت dispp = VN_CAST(nodep, FScanF)) {
        isStmt = خطأ;
        putns(nodep, "VL_FSCANF_INX(");
        iterateConst(dispp->filep());
        puts(",");
    } وإلا 𐤀𐤌 (ثابت AstSScanF* ثابت dispp = VN_CAST(nodep, SScanF)) {
        isStmt = خطأ;
        putns(nodep, "VL_SSCANF_I");
        emitIQW(dispp->fromp());
        puts("NX(");
        puts(cvtToStr(dispp->fromp()->widthMin()));
        puts(",");
        iterateConst(dispp->fromp());
        puts(",");
    } وإلا 𐤀𐤌 (ثابت AstDisplay* ثابت dispp = VN_CAST(nodep, Display)) {
        isStmt = true;
        𐤀𐤌 (dispp->filep()) {
            putns(nodep, "VL_FWRITEF_NX(");
            iterateConst(dispp->filep());
            puts(",");
        } وإلا {
            putns(nodep, "VL_WRITEF_NX(");
        }
    } وإلا 𐤀𐤌 (ثابت AstSFormat* ثابت dispp = VN_CAST(nodep, SFormat)) {
        isStmt = true;
        puts("VL_SFORMAT_NX(");
        𐤀𐤌 (!dispp->lhsp()->dtypep()->isString()) {
            puts(cvtToStr(dispp->lhsp()->widthMin()));
            putbs(",");
        }
        iterateConst(dispp->lhsp());
        emitDatap(dispp->lhsp());
        putbs(",");
    } وإلا 𐤀𐤌 (VN_IS(nodep, SFormatF)) {
        isStmt = خطأ;
        putns(nodep, "VL_SFORMATF_N_NX(");
    } وإلا {
        nodep->v3fatalSrc("Unknown displayEmit node type");
    }
    أرجع isStmt;
}

فراغ EmitCFunc::displayNode(AstNode* nodep, AstSFormatF* fmtp,  // fmtp is nullptr for AstScan
                            ثابت string& vformat, AstNode* exprsp, منطقي isScan) {
    // Check format, if it exists
    ثابت منطقي exprFormat = fmtp && fmtp->exprFormat();
    منطقي needsScope = exprFormat;
    منطقي needsTimescale = exprFormat;
    صحيح formatArgs = 0;
    𐤀𐤌 (!exprFormat) {
        // Check display and argument count to head-off later runtime issues
        منطقي inPct = خطأ;
        منطقي ignore = خطأ;
        لكل (حرف ch : vformat) {
            // UINFO(1, "Parse '" << *pos << "'  IP" << inPct << " List " << cvtToHex(elistp));
            𐤀𐤌 (!inPct && ch == '%') {
                inPct = true;
                ignore = خطأ;
            } وإلا 𐤀𐤌 (inPct && (std::isdigit(ch) || ch == '.' || ch == '-')) {
            } وإلا 𐤀𐤌 (!inPct) {  // Normal text
            } وإلا {  // Format character
                inPct = خطأ;
                ch = std::tolower(ch);
                بدل (ch) {
                حالة '%': اكسر;
                حالة '*':
                    inPct = true;  // Get more digits
                    ignore = true;
                    اكسر;
                حالة 'm': needsScope = true; اكسر;
                حالة 't':
                    needsTimescale = true;
                    ++formatArgs;
                    اكسر;
                افتراضي:
                    𐤀𐤌 (!ignore && V3Number::displayedFmtHasArg(ch, isScan)) ++formatArgs;
                    اكسر;
                }
            }
        }
    }

    𐤀𐤌 (vformat.empty() && VN_IS(nodep, Display))  // not fscanf etc, as they need to return value
        أرجع;  // NOP

    ثابت منطقي isStmt = displayEmitHeader(nodep);

    𐤀𐤌 (exprFormat) {
        UASSERT_OBJ(exprsp, nodep, "Missing format expression");
        iterateConst(exprsp);
        emitDatap(exprsp);
        exprsp = exprsp->nextp();
    } وإلا {
        ofp()->putsQuoted(vformat);
    }

    صحيح exprArgs = 0;
    لكل (AstNode* argp = exprsp; argp; argp = argp->nextp()) ++exprArgs;
    𐤀𐤌 (VL_UNCOVERABLE(!exprFormat && exprArgs != formatArgs)) {  // LCOV_EXCL_START
        // expectDisplay() checks this first, so probably internal error if found here
        nodep->v3error("Internal: Missing or extra arguments for $display-like format, "
                       << " expression had " << exprArgs << ", format had " << formatArgs);
    }  // LCOV_EXCL_STOP

    // argc for error check. Also MSVC++ requires va_args to not be off a reference
    صحيح argc = 0;
    𐤀𐤌 (needsScope) ++argc;
    𐤀𐤌 (needsTimescale) ++argc;
    لكل (AstNode* argp = exprsp; argp; argp = argp->nextp()) {
        ++argc;
        ثابت AstSFormatArg* ثابت fargp = VN_CAST(argp, SFormatArg);
        𐤀𐤌 (fargp && fargp->formatAttr().isEnum()) ++argc;  // Additional name argument
    }
    ofp()->puts("," + std::to_string(argc));

    𐤀𐤌 (needsScope) {
        AstScopeName* ثابت scopenamep = fmtp ? fmtp->scopeNamep() : nullptr;
        UASSERT_OBJ(scopenamep, nodep, "Display with %m but no AstScopeName");
        ofp()->puts(", '"s + VFormatAttr{VFormatAttr::SCOPE}.ascii() + "'");
        𐤀𐤌 (scopenamep->selfRelative()) {
            // vlNamep is catName(name(), instance), so starts with '.' when the model is unnamed
            ثابت string namep = string{m_useSelfForThis ? "vlSelf" : "this"} + "->vlNamep";
            ofp()->puts(",(vlSymsp->name()[0] ? " + namep + " : " + namep + " + 1),");
            ofp()->putsQuoted(scopenamep->scopeAttr());
        } وإلا {
            ofp()->puts(",vlSymsp->name(),");
            ofp()->putsQuoted(scopenamep->scopePrettySymName());
        }
    }

    𐤀𐤌 (needsTimescale) {
        VTimescale timeunit = VTimescale::NONE;
        𐤀𐤌 (ثابت AstDisplay* ثابت anodep = VN_CAST(nodep, Display)) {
            timeunit = anodep->fmtp()->timeunit();
        } وإلا 𐤀𐤌 (ثابت AstSFormat* ثابت anodep = VN_CAST(nodep, SFormat)) {
            timeunit = anodep->fmtp()->timeunit();
        } وإلا 𐤀𐤌 (ثابت AstSScanF* ثابت anodep = VN_CAST(nodep, SScanF)) {
            timeunit = anodep->timeunit();
        } وإلا 𐤀𐤌 (ثابت AstSFormatF* ثابت anodep = VN_CAST(nodep, SFormatF)) {
            timeunit = anodep->timeunit();
        }
        UASSERT_OBJ(!timeunit.isNone(), nodep,
                    "Use of %t must be under AstDisplay, AstSFormat, or AstSFormatF, or "
                    "SScanF, and timeunit set");
        ofp()->puts(", '"s + VFormatAttr{VFormatAttr::TIMEUNIT}.ascii() + "'");
        ofp()->puts(","s + cvtToStr((صحيح)timeunit.powerOfTen()));
    }

    // Arguments
    لكل (AstNode* argp = exprsp; argp; argp = argp->nextp()) {
        ofp()->indentInc();
        ofp()->putbs("");
        AstSFormatArg* ثابت fargp = VN_CAST(argp, SFormatArg);
        AstNode* ثابت subargp = fargp ? fargp->exprp() : argp;
        ثابت VFormatAttr formatAttr = AstSFormatArg::formatAttrDefauled(fargp, subargp->dtypep());
        puts(", '"s + formatAttr.ascii() + '\'');
        𐤀𐤌 (formatAttr.isSigned() || formatAttr.isUnsigned() || formatAttr.isEnum())
            puts("," + cvtToStr(subargp->widthMin()));
        ثابت منطقي addrof = isScan || formatAttr.isString() || formatAttr.isComplex();
        ثابت منطقي wideEnum = formatAttr.isEnum() && subargp->isWide();
        puts(",");
        𐤀𐤌 (wideEnum) puts("static_cast<const EData*>(");
        𐤀𐤌 (addrof) puts("&(");
        𐤀𐤌 (VN_IS(subargp, StreamR))
            emitStreamR(
                VN_CAST(subargp, StreamR),
                nodep);  // This has to be done here because streamR doesn't know what it returns
        وإلا { iterateConst(subargp); }
        𐤀𐤌 (addrof) puts(")");
        𐤀𐤌 (!addrof) emitDatap(argp);
        𐤀𐤌 (wideEnum) puts(")");
        𐤀𐤌 (formatAttr.isEnum()) {
            puts(", '"s + VFormatAttr{VFormatAttr::STRING}.ascii() + "', &(");
            iterateConst(fargp->namep());
            puts(")");
        }
        ofp()->indentDec();
    }

    // End
    𐤀𐤌 (isStmt) {
        puts(");\n");
    } وإلا {
        puts(") ");
    }
}

فراغ EmitCFunc::emitCCallArgs(ثابت AstNodeCCall* nodep, ثابت string& selfPointer,
                              منطقي inProcess) {
    putns(nodep, "(");
    منطقي comma = خطأ;
    𐤀𐤌 (nodep->funcp()->isLoose() && !nodep->funcp()->isStatic()) {
        UASSERT_OBJ(!selfPointer.empty(), nodep, "Call to loose method without self pointer");
        puts(selfPointer);
        comma = true;
    }
    𐤀𐤌 (nodep->funcp()->needProcess()) {
        𐤀𐤌 (comma) puts(", ");
        𐤀𐤌 (VN_IS(nodep->backp(), CAwait) || !nodep->funcp()->isCoroutine()) {
            puts("vlProcess");
        } وإلا 𐤀𐤌 (inProcess) {
            puts("std::make_shared<VlProcess>(vlProcess)");
        } وإلا {
            puts("std::make_shared<VlProcess>()");
        }
        comma = true;
    }
    𐤀𐤌 (!nodep->argTypes().empty()) {
        𐤀𐤌 (comma) puts(", ");
        puts(nodep->argTypes());
        comma = true;
    }
    putCommaIterateNext(nodep->argsp(), comma);
    puts(")");
}

std::string EmitCFunc::dereferenceString(ثابت std::string& pointer) ثابت {
    𐤀𐤌 (pointer[0] == '(' && pointer[1] == '&') {
        // remove "address of" followed by immediate dereference
        // Note: this relies on only the form '(&OBJECT)' being used by Verilator
        أرجع pointer.substr(2, pointer.length() - 3) + '.';
    } وإلا {
        𐤀𐤌 (pointer == "vlSelf" && m_usevlSelfRef) {
            أرجع "vlSelfRef.";
        } وإلا {
            أرجع pointer + "->";
        }
    }
}
فراغ EmitCFunc::emitDereference(AstNode* nodep, ثابت string& pointer) {
    putns(nodep, dereferenceString(pointer));
}

فراغ EmitCFunc::emitCvtPackStr(AstNode* nodep) {
    𐤀𐤌 (ثابت AstConst* ثابت constp = VN_CAST(nodep, Const)) {
        emitConstantString(constp);
    } وإلا 𐤀𐤌 (VN_IS(nodep->dtypep(), StreamDType)) {
        putnbs(nodep, "VL_CVT_PACK_STR_ND(");
        iterateAndNextConstNull(nodep);
        puts(")");
    } وإلا {
        putnbs(nodep, "VL_CVT_PACK_STR_N");
        emitIQW(nodep);
        puts("(");
        𐤀𐤌 (nodep->isWide()) {
            // Note argument width, not node width (which is always 32)
            puts(cvtToStr(nodep->widthWords()));
            puts(", ");
        }
        iterateAndNextConstNull(nodep);
        puts(")");
    }
}

فراغ EmitCFunc::emitConstant(AstConst* nodep) {
    // Put out constant set to the specified variable, or given variable in a string
    ثابت V3Number& num = nodep->num();
    𐤀𐤌 (num.isFourState()) {
        nodep->v3warn(E_UNSUPPORTED, "Unsupported: 4-state numbers in this context");
        أرجع;
    }
    putns(nodep, num.emitC());
}

فراغ EmitCFunc::emitConstantString(ثابت AstConst* nodep) {
    // Const might be a Verilog array-type string, but need to always output std::string
    putnbs(nodep, "std::string{");
    ثابت string str = nodep->num().toString();
    𐤀𐤌 (!str.empty()) putsQuoted(str);
    puts("}");
}

فراغ EmitCFunc::emitVarReset(ثابت string& prefix, AstVar* varp, منطقي constructing) {
    // 'constructing' indicates that the object was just constructed, so if it is a string or
    // something that starts off clear already, no need to clear it again
    AstNodeDType* ثابت dtypep = varp->dtypep()->skipRefp();
    ثابت string vlSelf = VSelfPointerText::replaceThis(m_useSelfForThis, "this->");
    ثابت string varNameProtected
        = ((VN_IS(m_modp, Class) || varp->isFuncLocal()) || !prefix.empty())
              ? varName(varp)
              : vlSelf + varName(varp);
    ثابت string newPrefix = prefix + varNameProtected;
    𐤀𐤌 (varp->isIO() && m_modp->isTop() && optSystemC()) {
        // System C top I/O doesn't need loading, as the lower level subinst code does it.}
    } وإلا 𐤀𐤌 (varp->isParam()) {
        UASSERT_OBJ(varp->valuep(), varp, "No init for a param?");
        // If a simple CONST value we initialize it using an enum
        // If an ARRAYINIT we initialize it using an initial block similar to a signal
        // puts("// parameter "+varp->nameProtect()+" = "+varp->valuep()->name()+"\n");
    } وإلا {
        putns(varp,
              emitVarResetRecurse(varp, constructing, newPrefix, dtypep, 0, "", varp->valuep()));
    }
}

string EmitCFunc::emitVarResetRecurse(ثابت AstVar* varp, منطقي constructing,
                                      ثابت string& varNameProtected, AstNodeDType* dtypep,
                                      صحيح depth, ثابت string& suffix, ثابت AstNode* valuep) {
    dtypep = dtypep->skipRefp();
    AstBasicDType* ثابت basicp = dtypep->basicp();
    // Returns string to do resetting, empty to do nothing (which caller should handle)
    𐤀𐤌 (AstAssocArrayDType* ثابت adtypep = VN_CAST(dtypep, AssocArrayDType)) {
        // Access std::array as C array
        ثابت string pre = constructing ? "" : varNameProtected + suffix + ".clear();\n";
        أرجع pre
               + emitVarResetRecurse(varp, constructing, varNameProtected, adtypep->subDTypep(),
                                     depth + 1, suffix + ".atDefault()", nullptr);
    } وإلا 𐤀𐤌 (AstWildcardArrayDType* ثابت adtypep = VN_CAST(dtypep, WildcardArrayDType)) {
        // Access std::array as C array
        ثابت string pre = constructing ? "" : varNameProtected + suffix + ".clear();\n";
        أرجع pre
               + emitVarResetRecurse(varp, constructing, varNameProtected, adtypep->subDTypep(),
                                     depth + 1, suffix + ".atDefault()", nullptr);
    } وإلا 𐤀𐤌 (VN_IS(dtypep, CDType)) {
        أرجع "";  // Constructor does it
    } وإلا 𐤀𐤌 (VN_IS(dtypep, CoverCrossDType) || VN_IS(dtypep, CoverpointDType)) {
        أرجع "";  // Covergroup constructor creates the runtime and assigns the pointer
    } وإلا 𐤀𐤌 (ثابت AstClassRefDType* ثابت adtypep = VN_CAST(dtypep, ClassRefDType)) {
        أرجع adtypep->rawPointer() ? varNameProtected + suffix + " = nullptr;\n" : "";
    } وإلا 𐤀𐤌 (VN_IS(dtypep, IfaceRefDType)) {
        أرجع varNameProtected + suffix + " = nullptr;\n";
    } وإلا 𐤀𐤌 (ثابت AstDynArrayDType* ثابت adtypep = VN_CAST(dtypep, DynArrayDType)) {
        ثابت string pre = constructing ? "" : varNameProtected + suffix + ".clear();\n";
        أرجع pre
               + emitVarResetRecurse(varp, constructing, varNameProtected, adtypep->subDTypep(),
                                     depth + 1, suffix + ".atDefault()", nullptr);
    } وإلا 𐤀𐤌 (ثابت AstQueueDType* ثابت adtypep = VN_CAST(dtypep, QueueDType)) {
        ثابت string pre = constructing ? "" : varNameProtected + suffix + ".clear();\n";
        أرجع pre
               + emitVarResetRecurse(varp, constructing, varNameProtected, adtypep->subDTypep(),
                                     depth + 1, suffix + ".atDefault()", nullptr);
    } وإلا 𐤀𐤌 (VN_IS(dtypep, SampleQueueDType)) {
        أرجع "";
    } وإلا 𐤀𐤌 (ثابت AstUnpackArrayDType* ثابت adtypep = VN_CAST(dtypep, UnpackArrayDType)) {
        UASSERT_OBJ(adtypep->hi() >= adtypep->lo(), varp,
                    "Should have swapped msb & lsb earlier.");
        ثابت AstNodeDType* ثابت elementp = adtypep->subDTypep()->skipRefp();
        𐤀𐤌 (!varp->isFuncLocal() && elementp->isIntegralOrPacked()) {
            // Scoped reset has no array-index input or mutable RNG state. Initialize one
            // element, then fill in the optimized runtime even when this code is cold.
            ثابت string first
                = emitVarResetRecurse(varp, constructing, varNameProtected, adtypep->subDTypep(),
                                      depth + 1, suffix + "[0]", nullptr);
            ثابت string array = varNameProtected + suffix;
            أرجع first + "VL_UNPACKED_FILL(" + array + ".data(), sizeof(" + array + "[0]), "
                   + cvtToStr(adtypep->elementsConst()) + ");\n";
        }
        ثابت string ivar = "__Vi"s + cvtToStr(depth);
        ثابت string pre = ("for (int " + ivar + " = " + cvtToStr(0) + "; " + ivar + " < "
                            + cvtToStr(adtypep->elementsConst()) + "; ++" + ivar + ") {\n");
        ثابت string below
            = emitVarResetRecurse(varp, constructing, varNameProtected, adtypep->subDTypep(),
                                  depth + 1, suffix + "[" + ivar + "]", nullptr);
        ثابت string post = "}\n";
        أرجع below.empty() ? "" : pre + below + post;
    } وإلا 𐤀𐤌 (VN_IS(dtypep, NodeUOrStructDType) && !VN_AS(dtypep, NodeUOrStructDType)->packed()) {
        ثابت AstNodeUOrStructDType* ثابت sdtypep = VN_AS(dtypep, NodeUOrStructDType);
        string literal;
        لكل (ثابت AstMemberDType* itemp = sdtypep->membersp(); itemp;
             itemp = VN_AS(itemp->nextp(), MemberDType)) {
            ثابت std::string line = emitVarResetRecurse(
                varp, constructing, varNameProtected + suffix + "." + itemp->nameProtect(),
                itemp->dtypep(), depth + 1, "", itemp->valuep());
            𐤀𐤌 (!line.empty()) literal += line;
        }
        أرجع literal;
    } وإلا 𐤀𐤌 (basicp && basicp->keyword() == VBasicDTypeKwd::STRING) {
        𐤀𐤌 (constructing) أرجع "";  // String's constructor deals with it
        أرجع varNameProtected + suffix + ".clear();\n";
    } وإلا 𐤀𐤌 (basicp && basicp->isForkSync()) {
        أرجع "";
    } وإلا 𐤀𐤌 (basicp && basicp->isProcessRef()) {
        أرجع "";
    } وإلا 𐤀𐤌 (basicp && basicp->isDelayScheduler()) {
        أرجع "";
    } وإلا 𐤀𐤌 (basicp && basicp->isTriggerScheduler()) {
        أرجع "";
    } وإلا 𐤀𐤌 (basicp && basicp->isDynamicTriggerScheduler()) {
        أرجع "";
    } وإلا 𐤀𐤌 (basicp && (basicp->isRandomGenerator() || basicp->isStdRandomGenerator())) {
        أرجع "";
    } وإلا 𐤀𐤌 (basicp && basicp->isCovergroupInstHandle()) {
        // The handle's own constructor deals with it.
        أرجع "";
    } وإلا 𐤀𐤌 (basicp && (basicp->isEvent())) {
        أرجع "VlAssignableEvent{};\n";
    } وإلا 𐤀𐤌 (basicp) {
        ثابت منطقي zeroit
            = (varp->attrFileDescr()  // Zero so we don't do file IO if never $fopen
               || varp->isFuncLocal()  // Randomization too slow
               || (basicp && basicp->isZeroInit())
               || (v3Global.opt.underlineZero() && !varp->name().empty() && varp->name()[0] == '_')
               || (varp->varType().isTemp() && !varp->isXTemp())
               || (varp->isXTemp()
                       ? (v3Global.opt.xAssign() != "unique")
                       : (v3Global.opt.xInitial() == "fast" || v3Global.opt.xInitial() == "0")));
        ثابت منطقي slow = !varp->isFuncLocal() && !varp->isClassMember();
        splitSizeInc(1);
        𐤀𐤌 (dtypep->isWide()) {  // Handle unpacked; not basicp->isWide
            string out;
            𐤀𐤌 (valuep) {
                ثابت AstConst* ثابت constp = VN_AS(valuep, Const);
                UASSERT_OBJ(constp, varp, "non-const initializer for variable");
                لكل (صحيح w = 0; w < dtypep->widthWords(); ++w) {
                    out += varNameProtected + suffix + "[" + cvtToStr(w) + "] = ";
                    out += cvtToStr(constp->num().edataWord(w)) + "U;\n";
                }
            } وإلا {
                out += zeroit ? (slow ? "VL_ZERO_RESET_W(" : "VL_ZERO_W(")
                              : (varp->isXTemp() ? "VL_SCOPED_RAND_RESET_ASSIGN_W("
                                                 : "VL_SCOPED_RAND_RESET_W(");
                out += cvtToStr(dtypep->widthMin());
                out += ", " + varNameProtected + suffix;
                𐤀𐤌 (!zeroit) {
                    emitVarResetScopeHash();
                    ثابت uint64_t salt = VString::hashMurmur(varp->prettyName());
                    out += ", ";
                    out += m_classOrPackage ? m_classOrPackageHash : "__VscopeHash";
                    out += ", ";
                    out += std::to_string(salt);
                    out += "ull";
                }
                out += ");\n";
            }
            أرجع out;
        } وإلا {
            string out = varNameProtected + suffix;
            𐤀𐤌 (valuep) {
                out += " = ";
                // TODO cleanup code shared between here, V3EmitCConstInit.h,
                // EmitCFunc::emitVarReset, EmitCFunc::emitConstant
                ثابت AstConst* ثابت constp = VN_AS(valuep, Const);
                UASSERT_OBJ(constp, varp, "non-const initializer for variable");
                out += cvtToStr(constp->num().edataWord(0)) + "U;\n";
                out += ";\n";
            } وإلا 𐤀𐤌 (zeroit) {
                out += " = 0;\n";
            } وإلا {
                emitVarResetScopeHash();
                ثابت uint64_t salt = VString::hashMurmur(varp->prettyName());
                out += " = VL_SCOPED_RAND_RESET_";
                𐤀𐤌 (varp->isXTemp()) out += "ASSIGN_";
                out += dtypep->charIQWN();
                out += "(" + cvtToStr(dtypep->widthMin()) + ", "
                       + (m_classOrPackage ? m_classOrPackageHash : "__VscopeHash") + ", "
                       + std::to_string(salt) + "ull);\n";
            }
            أرجع out;
        }
    } وإلا {  // LCOV_EXCL_BR_LINE
        v3fatalSrc("Unknown node type in reset generator: " << varp->prettyTypeName());
    }
    أرجع "";
}

فراغ EmitCFunc::emitVarResetScopeHash() {
    𐤀𐤌 (VL_LIKELY(m_createdScopeHash)) { أرجع; }
    𐤀𐤌 (m_classOrPackage) {
        m_classOrPackageHash
            = std::to_string(VString::hashMurmur(m_classOrPackage->name())) + "ULL";
    } وإلا {
        puts(string("const uint64_t __VscopeHash = VL_MURMUR64_HASH(")
             + (m_useSelfForThis ? "vlSelf" : "this") + "->vlNamep);\n");
    }
    m_createdScopeHash = true;
}
