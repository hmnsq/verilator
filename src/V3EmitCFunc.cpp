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

منطقي EmitCFunc::emitSimpleOk(Ast_عقدة_تعبير* nodep) {
    // Can we put out a simple (A + B) instead of VL_ADD_III(A,B)?
    إذا (nodep->emitSimpleOperator() == "") إرجاع خطأ;
    إذا (nodep->isWide()) إرجاع خطأ;
    إذا (nodep->op1p() && nodep->op1p()->isWide()) إرجاع خطأ;
    إذا (nodep->op2p() && nodep->op2p()->isWide()) إرجاع خطأ;
    إذا (nodep->op3p() && nodep->op3p()->isWide()) إرجاع خطأ;
    إرجاع true;
}

فراغ EmitCFunc::emitOpName(Ast_عقدة* nodep, ثابت string& format, Ast_عقدة* lhsp, Ast_عقدة* rhsp,
                           Ast_عقدة* thsp) {
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
        إذا (!nextComma.empty()) {
            out += nextComma;
            nextComma = "";
        }
    };

    auto putOut = [هذا, &out]() {
        إذا (!out.empty()) puts(out);
        out = "";
    };

    لكل (string::const_iterator pos = format.begin(); pos != format.end(); ++pos) {
        إذا (pos[0] == ',') {
            // Remember we need to add one, but don't do yet to avoid ",)"
            إذا (needComma) {
                إذا (pos[1] == ' ') {
                    nextComma = ", ";
                } وإلا {
                    nextComma = ",";
                }
                needComma = خطأ;
            }
            إذا (pos[1] == ' ') ++pos;  // Must do even if no nextComma
        } وإلا إذا (pos[0] == '%') {
            ++pos;
            منطقي detail = خطأ;
            Ast_عقدة* detailp = nullptr;
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
                إذا (nodep->isWide() && !usesQueue) {
                    UASSERT_OBJ(m_wideTempRefp, nodep,
                                "Wide op " << nodep->prettyTypeName()
                                           << " w/ no temp, perhaps missing op in V3EmitC?");
                    commaOut();
                    putOut();
                    إذا (!m_wideTempRefp->selfPointer().isEmpty()) {
                        emitDereference(m_wideTempRefp,
                                        m_wideTempRefp->selfPointerProtect(m_useSelfForThis));
                    }
                    out += varName(m_wideTempRefp->varp());
                    m_wideTempRefp = nullptr;
                    needComma = true;
                } وإلا إذا (usesQueue) {
                    commaOut();
                    putOut();
                    iterateAndNextConstNull(nodep->backp()->op2p());
                    needComma = true;
                }

                اكسر;
            افتراضي: nodep->v3fatalSrc("Unknown emitOperator format code: %" << pos[0]); اكسر;
            }
            إذا (detail) {
                // Get next letter of %[nlrt]
                ++pos;
                بدل (pos[0]) {
                حالة 'q':
                    putOut();
                    // If we are assigning this to a queue we need to get the return type
                    إذا (VN_IS(detailp->backp(), Assign)
                        && VN_IS(detailp->backp()->op2p()->dtypep()->skipRefp(), QueueDType)) {
                        puts("R");  // R for queue
                        usesQueue = true;
                    } وإلا إذا (VN_IS(detailp->dtypep()->skipRefp(), QueueDType)
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
                    إذا (lhsp->isWide()) {
                        commaOut();
                        out += cvtToStr(lhsp->width_كلمات());
                        needComma = true;
                    } وإلا إذا (VN_IS(lhsp, StreamR)) {
                        commaOut();
                        out += cvtToStr(rhsp->width_كلمات());
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
        } وإلا إذا (pos[0] == ')') {
            nextComma = "";
            out += ')';
        } وإلا إذا (pos[0] == '(') {
            commaOut();
            needComma = خطأ;
            out += '(';
        } وإلا {
            // Normal text
            إذا (std::isalnum(pos[0])) needComma = true;
            commaOut();
            out += pos[0];
        }
    }
    putOut();
}

منطقي EmitCFunc::displayEmitHeader(Ast_عقدة* nodep) {
    منطقي isStmt = خطأ;
    إذا (ثابت AstFScanF* ثابت dispp = VN_CAST(nodep, FScanF)) {
        isStmt = خطأ;
        putns(nodep, "VL_FSCANF_INX(");
        iterateConst(dispp->filep());
        puts(",");
    } وإلا إذا (ثابت AstSScanF* ثابت dispp = VN_CAST(nodep, SScanF)) {
        isStmt = خطأ;
        putns(nodep, "VL_SSCANF_I");
        emitIQW(dispp->fromp());
        puts("NX(");
        puts(cvtToStr(dispp->fromp()->widthMin()));
        puts(",");
        iterateConst(dispp->fromp());
        puts(",");
    } وإلا إذا (ثابت AstDisplay* ثابت dispp = VN_CAST(nodep, Display)) {
        isStmt = true;
        إذا (dispp->filep()) {
            putns(nodep, "VL_FWRITEF_NX(");
            iterateConst(dispp->filep());
            puts(",");
        } وإلا {
            putns(nodep, "VL_WRITEF_NX(");
        }
    } وإلا إذا (ثابت AstSFormat* ثابت dispp = VN_CAST(nodep, SFormat)) {
        isStmt = true;
        puts("VL_SFORMAT_NX(");
        إذا (!dispp->lhsp()->dtypep()->isString()) {
            puts(cvtToStr(dispp->lhsp()->widthMin()));
            putbs(",");
        }
        iterateConst(dispp->lhsp());
        emitDatap(dispp->lhsp());
        putbs(",");
    } وإلا إذا (VN_IS(nodep, SFormatF)) {
        isStmt = خطأ;
        putns(nodep, "VL_SFORMATF_N_NX(");
    } وإلا {
        nodep->v3fatalSrc("Unknown displayEmit node type");
    }
    إرجاع isStmt;
}

فراغ EmitCFunc::display_عقدة(Ast_عقدة* nodep, AstSFormatF* fmtp,  // fmtp is nullptr for AstScan
                            ثابت string& vformat, Ast_عقدة* exprsp, منطقي isScan) {
    // Check format, if it exists
    ثابت منطقي تعبير_Format_2 = fmtp && fmtp->تعبير_Format_2();
    منطقي needsScope = تعبير_Format_2;
    منطقي needsTimescale = تعبير_Format_2;
    صحيح formatArgs = 0;
    إذا (!تعبير_Format_2) {
        // Check display and argument count to head-off later runtime issues
        منطقي inPct = خطأ;
        منطقي ignore = خطأ;
        لكل (حرف ch : vformat) {
            // UINFO(1, "Parse '" << *pos << "'  IP" << inPct << " List " << cvtToHex(elistp));
            إذا (!inPct && ch == '%') {
                inPct = true;
                ignore = خطأ;
            } وإلا إذا (inPct && (std::isdigit(ch) || ch == '.' || ch == '-')) {
            } وإلا إذا (!inPct) {  // Normal text
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
                    إذا (!ignore && V3_العدد::displayedFmtHasArg(ch, isScan)) ++formatArgs;
                    اكسر;
                }
            }
        }
    }

    إذا (vformat.empty() && VN_IS(nodep, Display))  // not fscanf etc, as they need to return value
        إرجاع;  // NOP

    ثابت منطقي isStmt = displayEmitHeader(nodep);

    إذا (تعبير_Format_2) {
        UASSERT_OBJ(exprsp, nodep, "Missing format expression");
        iterateConst(exprsp);
        emitDatap(exprsp);
        exprsp = exprsp->nextp();
    } وإلا {
        ofp()->putsQuoted(vformat);
    }

    صحيح تعبير_Args = 0;
    لكل (Ast_عقدة* argp = exprsp; argp; argp = argp->nextp()) ++تعبير_Args;
    إذا (VL_UNCOVERABLE(!تعبير_Format_2 && تعبير_Args != formatArgs)) {  // LCOV_EXCL_START
        // expectDisplay() checks this first, so probably internal error if found here
        nodep->v3error("Internal: Missing or extra arguments for $display-like format, "
                       << " expression had " << تعبير_Args << ", format had " << formatArgs);
    }  // LCOV_EXCL_STOP

    // argc for error check. Also MSVC++ requires va_args to not be off a reference
    صحيح argc = 0;
    إذا (needsScope) ++argc;
    إذا (needsTimescale) ++argc;
    لكل (Ast_عقدة* argp = exprsp; argp; argp = argp->nextp()) {
        ++argc;
        ثابت AstSFormatArg* ثابت fargp = VN_CAST(argp, SFormatArg);
        إذا (fargp && fargp->formatAttr().isEnum()) ++argc;  // Additional name argument
    }
    ofp()->puts("," + std::to_string(argc));

    إذا (needsScope) {
        AstScopeName* ثابت scopenamep = fmtp ? fmtp->scopeNamep() : nullptr;
        UASSERT_OBJ(scopenamep, nodep, "Display with %m but no AstScopeName");
        ofp()->puts(", '"s + VFormatAttr{VFormatAttr::SCOPE}.ascii() + "'");
        إذا (scopenamep->selfRelative()) {
            // vlNamep is catName(name(), instance), so starts with '.' when the model is unnamed
            ثابت string namep = string{m_useSelfForThis ? "vlSelf" : "this"} + "->vlNamep";
            ofp()->puts(",(vlSymsp->name()[0] ? " + namep + " : " + namep + " + 1),");
            ofp()->putsQuoted(scopenamep->scopeAttr());
        } وإلا {
            ofp()->puts(",vlSymsp->name(),");
            ofp()->putsQuoted(scopenamep->scopePrettySymName());
        }
    }

    إذا (needsTimescale) {
        VTimescale timeunit = VTimescale::NONE;
        إذا (ثابت AstDisplay* ثابت anodep = VN_CAST(nodep, Display)) {
            timeunit = anodep->fmtp()->timeunit();
        } وإلا إذا (ثابت AstSFormat* ثابت anodep = VN_CAST(nodep, SFormat)) {
            timeunit = anodep->fmtp()->timeunit();
        } وإلا إذا (ثابت AstSScanF* ثابت anodep = VN_CAST(nodep, SScanF)) {
            timeunit = anodep->timeunit();
        } وإلا إذا (ثابت AstSFormatF* ثابت anodep = VN_CAST(nodep, SFormatF)) {
            timeunit = anodep->timeunit();
        }
        UASSERT_OBJ(!timeunit.isNone(), nodep,
                    "Use of %t must be under AstDisplay, AstSFormat, or AstSFormatF, or "
                    "SScanF, and timeunit set");
        ofp()->puts(", '"s + VFormatAttr{VFormatAttr::TIMEUNIT}.ascii() + "'");
        ofp()->puts(","s + cvtToStr((صحيح)timeunit.powerOfTen()));
    }

    // Arguments
    لكل (Ast_عقدة* argp = exprsp; argp; argp = argp->nextp()) {
        ofp()->indentInc();
        ofp()->putbs("");
        AstSFormatArg* ثابت fargp = VN_CAST(argp, SFormatArg);
        Ast_عقدة* ثابت subargp = fargp ? fargp->exprp() : argp;
        ثابت VFormatAttr formatAttr = AstSFormatArg::formatAttrDefauled(fargp, subargp->dtypep());
        puts(", '"s + formatAttr.ascii() + '\'');
        إذا (formatAttr.isSigned() || formatAttr.isUnsigned() || formatAttr.isEnum())
            puts("," + cvtToStr(subargp->widthMin()));
        ثابت منطقي addrof = isScan || formatAttr.isString() || formatAttr.isComplex();
        ثابت منطقي wideEnum = formatAttr.isEnum() && subargp->isWide();
        puts(",");
        إذا (wideEnum) puts("static_cast<const EData*>(");
        إذا (addrof) puts("&(");
        إذا (VN_IS(subargp, StreamR))
            emitStreamR(
                VN_CAST(subargp, StreamR),
                nodep);  // This has to be done here because streamR doesn't know what it returns
        وإلا { iterateConst(subargp); }
        إذا (addrof) puts(")");
        إذا (!addrof) emitDatap(argp);
        إذا (wideEnum) puts(")");
        إذا (formatAttr.isEnum()) {
            puts(", '"s + VFormatAttr{VFormatAttr::STRING}.ascii() + "', &(");
            iterateConst(fargp->namep());
            puts(")");
        }
        ofp()->indentDec();
    }

    // End
    إذا (isStmt) {
        puts(");\n");
    } وإلا {
        puts(") ");
    }
}

فراغ EmitCFunc::emitCCallArgs(ثابت Ast_عقدة_CCall* nodep, ثابت string& selfPointer,
                              منطقي inProcess) {
    putns(nodep, "(");
    منطقي comma = خطأ;
    إذا (nodep->funcp()->isLoose() && !nodep->funcp()->isStatic()) {
        UASSERT_OBJ(!selfPointer.empty(), nodep, "Call to loose method without self pointer");
        puts(selfPointer);
        comma = true;
    }
    إذا (nodep->funcp()->needProcess()) {
        إذا (comma) puts(", ");
        إذا (VN_IS(nodep->backp(), CAwait) || !nodep->funcp()->isCoroutine()) {
            puts("vlProcess");
        } وإلا إذا (inProcess) {
            puts("std::make_shared<VlProcess>(vlProcess)");
        } وإلا {
            puts("std::make_shared<VlProcess>()");
        }
        comma = true;
    }
    إذا (!nodep->argTypes().empty()) {
        إذا (comma) puts(", ");
        puts(nodep->argTypes());
        comma = true;
    }
    putCommaIterateNext(nodep->argsp(), comma);
    puts(")");
}

std::string EmitCFunc::dereferenceString(ثابت std::string& pointer) ثابت {
    إذا (pointer[0] == '(' && pointer[1] == '&') {
        // remove "address of" followed by immediate dereference
        // Note: this relies on only the form '(&OBJECT)' being used by Verilator
        إرجاع pointer.substr(2, pointer.length() - 3) + '.';
    } وإلا {
        إذا (pointer == "vlSelf" && m_usevlSelfRef) {
            إرجاع "vlSelfRef.";
        } وإلا {
            إرجاع pointer + "->";
        }
    }
}
فراغ EmitCFunc::emitDereference(Ast_عقدة* nodep, ثابت string& pointer) {
    putns(nodep, dereferenceString(pointer));
}

فراغ EmitCFunc::emitCvtPackStr(Ast_عقدة* nodep) {
    إذا (ثابت AstConst* ثابت constp = VN_CAST(nodep, Const)) {
        emitConstantString(constp);
    } وإلا إذا (VN_IS(nodep->dtypep(), StreamDType)) {
        putnbs(nodep, "VL_CVT_PACK_STR_ND(");
        iterateAndNextConstNull(nodep);
        puts(")");
    } وإلا {
        putnbs(nodep, "VL_CVT_PACK_STR_N");
        emitIQW(nodep);
        puts("(");
        إذا (nodep->isWide()) {
            // Note argument width, not node width (which is always 32)
            puts(cvtToStr(nodep->width_كلمات()));
            puts(", ");
        }
        iterateAndNextConstNull(nodep);
        puts(")");
    }
}

فراغ EmitCFunc::emitConstant(AstConst* nodep) {
    // Put out constant set to the specified variable, or given variable in a string
    ثابت V3_العدد& رقم = nodep->رقم();
    إذا (رقم.isFourState()) {
        nodep->v3warn(E_UNSUPPORTED, "Unsupported: 4-state numbers in this context");
        إرجاع;
    }
    putns(nodep, رقم.emitC());
}

فراغ EmitCFunc::emitConstantString(ثابت AstConst* nodep) {
    // Const might be a Verilog array-type string, but need to always output std::string
    putnbs(nodep, "std::string{");
    ثابت string str = nodep->رقم().toString();
    إذا (!str.empty()) putsQuoted(str);
    puts("}");
}

فراغ EmitCFunc::emitVarReset(ثابت string& prefix, AstVar* varp, منطقي constructing) {
    // 'constructing' indicates that the object was just constructed, so if it is a string or
    // something that starts off clear already, no need to clear it again
    Ast_عقدة_DType* ثابت dtypep = varp->dtypep()->skipRefp();
    ثابت string vlSelf = VSelfPointerText::replaceThis(m_useSelfForThis, "this->");
    ثابت string varNameProtected
        = ((VN_IS(m_modp, Class) || varp->isFuncLocal()) || !prefix.empty())
              ? varName(varp)
              : vlSelf + varName(varp);
    ثابت string newPrefix = prefix + varNameProtected;
    إذا (varp->isIO() && m_modp->is_قمة() && optSystemC()) {
        // System C top I/O doesn't need loading, as the lower level subinst code does it.}
    } وإلا إذا (varp->isParam()) {
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
                                      ثابت string& varNameProtected, Ast_عقدة_DType* dtypep,
                                      صحيح depth, ثابت string& suffix, ثابت Ast_عقدة* valuep) {
    dtypep = dtypep->skipRefp();
    AstBasicDType* ثابت basicp = dtypep->basicp();
    // Returns string to do resetting, empty to do nothing (which caller should handle)
    إذا (AstAssocArrayDType* ثابت adtypep = VN_CAST(dtypep, AssocArrayDType)) {
        // Access std::array as C array
        ثابت string pre = constructing ? "" : varNameProtected + suffix + ".clear();\n";
        إرجاع pre
               + emitVarResetRecurse(varp, constructing, varNameProtected, adtypep->subDTypep(),
                                     depth + 1, suffix + ".atDefault()", nullptr);
    } وإلا إذا (AstWildcardArrayDType* ثابت adtypep = VN_CAST(dtypep, WildcardArrayDType)) {
        // Access std::array as C array
        ثابت string pre = constructing ? "" : varNameProtected + suffix + ".clear();\n";
        إرجاع pre
               + emitVarResetRecurse(varp, constructing, varNameProtected, adtypep->subDTypep(),
                                     depth + 1, suffix + ".atDefault()", nullptr);
    } وإلا إذا (VN_IS(dtypep, CDType)) {
        إرجاع "";  // Constructor does it
    } وإلا إذا (VN_IS(dtypep, CoverCrossDType) || VN_IS(dtypep, CoverpointDType)) {
        إرجاع "";  // Covergroup constructor creates the runtime and assigns the pointer
    } وإلا إذا (ثابت AstClassRefDType* ثابت adtypep = VN_CAST(dtypep, ClassRefDType)) {
        إرجاع adtypep->rawPointer() ? varNameProtected + suffix + " = nullptr;\n" : "";
    } وإلا إذا (VN_IS(dtypep, IfaceRefDType)) {
        إرجاع varNameProtected + suffix + " = nullptr;\n";
    } وإلا إذا (ثابت AstDynArrayDType* ثابت adtypep = VN_CAST(dtypep, DynArrayDType)) {
        ثابت string pre = constructing ? "" : varNameProtected + suffix + ".clear();\n";
        إرجاع pre
               + emitVarResetRecurse(varp, constructing, varNameProtected, adtypep->subDTypep(),
                                     depth + 1, suffix + ".atDefault()", nullptr);
    } وإلا إذا (ثابت AstQueueDType* ثابت adtypep = VN_CAST(dtypep, QueueDType)) {
        ثابت string pre = constructing ? "" : varNameProtected + suffix + ".clear();\n";
        إرجاع pre
               + emitVarResetRecurse(varp, constructing, varNameProtected, adtypep->subDTypep(),
                                     depth + 1, suffix + ".atDefault()", nullptr);
    } وإلا إذا (VN_IS(dtypep, SampleQueueDType)) {
        إرجاع "";
    } وإلا إذا (ثابت AstUnpackArrayDType* ثابت adtypep = VN_CAST(dtypep, UnpackArrayDType)) {
        UASSERT_OBJ(adtypep->hi() >= adtypep->lo(), varp,
                    "Should have swapped msb & lsb earlier.");
        ثابت Ast_عقدة_DType* ثابت elementp = adtypep->subDTypep()->skipRefp();
        إذا (!varp->isFuncLocal() && elementp->isIntegralOrPacked()) {
            // Scoped reset has no array-index input or mutable RNG state. Initialize one
            // element, then fill in the optimized runtime even when this code is cold.
            ثابت string first
                = emitVarResetRecurse(varp, constructing, varNameProtected, adtypep->subDTypep(),
                                      depth + 1, suffix + "[0]", nullptr);
            ثابت string array = varNameProtected + suffix;
            إرجاع first + "VL_UNPACKED_FILL(" + array + ".data(), sizeof(" + array + "[0]), "
                   + cvtToStr(adtypep->elementsConst()) + ");\n";
        }
        ثابت string ivar = "__Vi"s + cvtToStr(depth);
        ثابت string pre = ("for (int " + ivar + " = " + cvtToStr(0) + "; " + ivar + " < "
                            + cvtToStr(adtypep->elementsConst()) + "; ++" + ivar + ") {\n");
        ثابت string below
            = emitVarResetRecurse(varp, constructing, varNameProtected, adtypep->subDTypep(),
                                  depth + 1, suffix + "[" + ivar + "]", nullptr);
        ثابت string post = "}\n";
        إرجاع below.empty() ? "" : pre + below + post;
    } وإلا إذا (VN_IS(dtypep, عقدة_UOr_Struct_DType) && !VN_AS(dtypep, عقدة_UOr_Struct_DType)->packed()) {
        ثابت Ast_عقدة_UOr_Struct_DType* ثابت sdtypep = VN_AS(dtypep, عقدة_UOr_Struct_DType);
        string literal;
        لكل (ثابت AstMemberDType* itemp = sdtypep->membersp(); itemp;
             itemp = VN_AS(itemp->nextp(), MemberDType)) {
            ثابت std::string سطر = emitVarResetRecurse(
                varp, constructing, varNameProtected + suffix + "." + itemp->nameProtect(),
                itemp->dtypep(), depth + 1, "", itemp->valuep());
            إذا (!سطر.empty()) literal += سطر;
        }
        إرجاع literal;
    } وإلا إذا (basicp && basicp->keyword() == VBasicDTypeKwd::STRING) {
        إذا (constructing) إرجاع "";  // String's constructor deals with it
        إرجاع varNameProtected + suffix + ".clear();\n";
    } وإلا إذا (basicp && basicp->isForkSync()) {
        إرجاع "";
    } وإلا إذا (basicp && basicp->isProcessRef()) {
        إرجاع "";
    } وإلا إذا (basicp && basicp->isDelayScheduler()) {
        إرجاع "";
    } وإلا إذا (basicp && basicp->isTriggerScheduler()) {
        إرجاع "";
    } وإلا إذا (basicp && basicp->isDynamicTriggerScheduler()) {
        إرجاع "";
    } وإلا إذا (basicp && (basicp->isRandomGenerator() || basicp->isStdRandomGenerator())) {
        إرجاع "";
    } وإلا إذا (basicp && basicp->isCovergroupInstHandle()) {
        // The handle's own constructor deals with it.
        إرجاع "";
    } وإلا إذا (basicp && (basicp->isEvent())) {
        إرجاع "VlAssignableEvent{};\n";
    } وإلا إذا (basicp) {
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
        إذا (dtypep->isWide()) {  // Handle unpacked; not basicp->isWide
            string out;
            إذا (valuep) {
                ثابت AstConst* ثابت constp = VN_AS(valuep, Const);
                UASSERT_OBJ(constp, varp, "non-const initializer for variable");
                لكل (صحيح w = 0; w < dtypep->width_كلمات(); ++w) {
                    out += varNameProtected + suffix + "[" + cvtToStr(w) + "] = ";
                    out += cvtToStr(constp->رقم().edataWord(w)) + "U;\n";
                }
            } وإلا {
                out += zeroit ? (slow ? "VL_ZERO_RESET_W(" : "VL_ZERO_W(")
                              : (varp->isXTemp() ? "VL_SCOPED_RAND_RESET_ASSIGN_W("
                                                 : "VL_SCOPED_RAND_RESET_W(");
                out += cvtToStr(dtypep->widthMin());
                out += ", " + varNameProtected + suffix;
                إذا (!zeroit) {
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
            إرجاع out;
        } وإلا {
            string out = varNameProtected + suffix;
            إذا (valuep) {
                out += " = ";
                // TODO cleanup code shared between here, V3EmitCConstInit.h,
                // EmitCFunc::emitVarReset, EmitCFunc::emitConstant
                ثابت AstConst* ثابت constp = VN_AS(valuep, Const);
                UASSERT_OBJ(constp, varp, "non-const initializer for variable");
                out += cvtToStr(constp->رقم().edataWord(0)) + "U;\n";
                out += ";\n";
            } وإلا إذا (zeroit) {
                out += " = 0;\n";
            } وإلا {
                emitVarResetScopeHash();
                ثابت uint64_t salt = VString::hashMurmur(varp->prettyName());
                out += " = VL_SCOPED_RAND_RESET_";
                إذا (varp->isXTemp()) out += "ASSIGN_";
                out += dtypep->charIQWN();
                out += "(" + cvtToStr(dtypep->widthMin()) + ", "
                       + (m_classOrPackage ? m_classOrPackageHash : "__VscopeHash") + ", "
                       + std::to_string(salt) + "ull);\n";
            }
            إرجاع out;
        }
    } وإلا {  // LCOV_EXCL_BR_LINE
        v3fatalSrc("Unknown node type in reset generator: " << varp->prettyTypeName());
    }
    إرجاع "";
}

فراغ EmitCFunc::emitVarResetScopeHash() {
    إذا (VL_LIKELY(m_createdScopeHash)) { إرجاع; }
    إذا (m_classOrPackage) {
        m_classOrPackageHash
            = std::to_string(VString::hashMurmur(m_classOrPackage->name())) + "ULL";
    } وإلا {
        puts(string("const uint64_t __VscopeHash = VL_MURMUR64_HASH(")
             + (m_useSelfForThis ? "vlSelf" : "this") + "->vlNamep);\n");
    }
    m_createdScopeHash = true;
}
