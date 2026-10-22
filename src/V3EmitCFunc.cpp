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

#அந்தர்பாவ "V3PchAstMT.h"

#அந்தர்பாவ "V3EmitCFunc.h"

#அந்தர்பாவ <map>
#அந்தர்பாவ <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// EmitCFunc

தருக்க EmitCFunc::emitSimpleOk(Ast_முனை_கோவை* nodep) {
    // Can we put out a simple (A + B) instead of VL_ADD_III(A,B)?
    எனில் (nodep->emitSimpleOperator() == "") திரும்பு பொய்;
    எனில் (nodep->isWide()) திரும்பு பொய்;
    எனில் (nodep->op1p() && nodep->op1p()->isWide()) திரும்பு பொய்;
    எனில் (nodep->op2p() && nodep->op2p()->isWide()) திரும்பு பொய்;
    எனில் (nodep->op3p() && nodep->op3p()->isWide()) திரும்பு பொய்;
    திரும்பு உண்மை;
}

வெறுமை EmitCFunc::emitOpName(Ast_முனை* nodep, மாறிலி string& format, Ast_முனை* lhsp, Ast_முனை* rhsp,
                           Ast_முனை* thsp) {
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

    தருக்க needComma = பொய்;
    தருக்க usesQueue = பொய்;
    string nextComma;
    auto commaOut = [&out, &nextComma]() {
        எனில் (!nextComma.empty()) {
            out += nextComma;
            nextComma = "";
        }
    };

    auto putOut = [இது, &out]() {
        எனில் (!out.empty()) puts(out);
        out = "";
    };

    க்கு (string::const_iterator pos = format.begin(); pos != format.end(); ++pos) {
        எனில் (pos[0] == ',') {
            // Remember we need to add one, but don't do yet to avoid ",)"
            எனில் (needComma) {
                எனில் (pos[1] == ' ') {
                    nextComma = ", ";
                } இல்லையெனில் {
                    nextComma = ",";
                }
                needComma = பொய்;
            }
            எனில் (pos[1] == ' ') ++pos;  // Must do even if no nextComma
        } இல்லையெனில் எனில் (pos[0] == '%') {
            ++pos;
            தருக்க detail = பொய்;
            Ast_முனை* detailp = nullptr;
            தேர்வு (pos[0]) {
            நிலை '%': out += '%'; நிறுத்து;
            நிலை 'k':
                putOut();
                putbs("");
                நிறுத்து;
            நிலை 'n':
                detail = உண்மை;
                detailp = nodep;
                நிறுத்து;
            நிலை 'l':
                detail = உண்மை;
                detailp = lhsp;
                நிறுத்து;
            நிலை 'r':
                detail = உண்மை;
                detailp = rhsp;
                நிறுத்து;
            நிலை 't':
                detail = உண்மை;
                detailp = thsp;
                நிறுத்து;
            நிலை 'P':
                எனில் (nodep->isWide() && !usesQueue) {
                    UASSERT_OBJ(m_wideTempRefp, nodep,
                                "Wide op " << nodep->prettyTypeName()
                                           << " w/ no temp, perhaps missing op in V3EmitC?");
                    commaOut();
                    putOut();
                    எனில் (!m_wideTempRefp->selfPointer().isEmpty()) {
                        emitDereference(m_wideTempRefp,
                                        m_wideTempRefp->selfPointerProtect(m_useSelfForThis));
                    }
                    out += varName(m_wideTempRefp->varp());
                    m_wideTempRefp = nullptr;
                    needComma = உண்மை;
                } இல்லையெனில் எனில் (usesQueue) {
                    commaOut();
                    putOut();
                    iterateAndNextConstNull(nodep->backp()->op2p());
                    needComma = உண்மை;
                }

                நிறுத்து;
            இயல்பு: nodep->v3fatalSrc("Unknown emitOperator format code: %" << pos[0]); நிறுத்து;
            }
            எனில் (detail) {
                // Get next letter of %[nlrt]
                ++pos;
                தேர்வு (pos[0]) {
                நிலை 'q':
                    putOut();
                    // If we are assigning this to a queue we need to get the return type
                    எனில் (VN_IS(detailp->backp(), Assign)
                        && VN_IS(detailp->backp()->op2p()->dtypep()->skipRefp(), QueueDType)) {
                        puts("R");  // R for queue
                        usesQueue = உண்மை;
                    } இல்லையெனில் எனில் (VN_IS(detailp->dtypep()->skipRefp(), QueueDType)
                               || VN_IS(detailp->dtypep()->skipRefp(), StreamDType)) {
                        puts("R");  // R for queue
                    } இல்லையெனில் {
                        emitIQW(detailp);
                    }
                    நிறுத்து;
                நிலை 'w':
                    commaOut();
                    out += cvtToStr(detailp->widthMin());
                    needComma = உண்மை;
                    நிறுத்து;
                நிலை 'W':
                    எனில் (lhsp->isWide()) {
                        commaOut();
                        out += cvtToStr(lhsp->width_சொற்கள்());
                        needComma = உண்மை;
                    } இல்லையெனில் எனில் (VN_IS(lhsp, StreamR)) {
                        commaOut();
                        out += cvtToStr(rhsp->width_சொற்கள்());
                        needComma = உண்மை;
                    }
                    நிறுத்து;
                நிலை 'i':
                    commaOut();
                    UASSERT_OBJ(detailp, nodep, "emitOperator() references undef node");
                    putOut();
                    iterateAndNextConstNull(detailp);
                    needComma = உண்மை;
                    நிறுத்து;
                இயல்பு:
                    nodep->v3fatalSrc("Unknown emitOperator format code: %[nlrt]" << pos[0]);
                    நிறுத்து;
                }
            }
        } இல்லையெனில் எனில் (pos[0] == ')') {
            nextComma = "";
            out += ')';
        } இல்லையெனில் எனில் (pos[0] == '(') {
            commaOut();
            needComma = பொய்;
            out += '(';
        } இல்லையெனில் {
            // Normal text
            எனில் (std::isalnum(pos[0])) needComma = உண்மை;
            commaOut();
            out += pos[0];
        }
    }
    putOut();
}

தருக்க EmitCFunc::displayEmitHeader(Ast_முனை* nodep) {
    தருக்க isStmt = பொய்;
    எனில் (மாறிலி AstFScanF* மாறிலி dispp = VN_CAST(nodep, FScanF)) {
        isStmt = பொய்;
        putns(nodep, "VL_FSCANF_INX(");
        iterateConst(dispp->filep());
        puts(",");
    } இல்லையெனில் எனில் (மாறிலி AstSScanF* மாறிலி dispp = VN_CAST(nodep, SScanF)) {
        isStmt = பொய்;
        putns(nodep, "VL_SSCANF_I");
        emitIQW(dispp->fromp());
        puts("NX(");
        puts(cvtToStr(dispp->fromp()->widthMin()));
        puts(",");
        iterateConst(dispp->fromp());
        puts(",");
    } இல்லையெனில் எனில் (மாறிலி AstDisplay* மாறிலி dispp = VN_CAST(nodep, Display)) {
        isStmt = உண்மை;
        எனில் (dispp->filep()) {
            putns(nodep, "VL_FWRITEF_NX(");
            iterateConst(dispp->filep());
            puts(",");
        } இல்லையெனில் {
            putns(nodep, "VL_WRITEF_NX(");
        }
    } இல்லையெனில் எனில் (மாறிலி AstSFormat* மாறிலி dispp = VN_CAST(nodep, SFormat)) {
        isStmt = உண்மை;
        puts("VL_SFORMAT_NX(");
        எனில் (!dispp->lhsp()->dtypep()->isString()) {
            puts(cvtToStr(dispp->lhsp()->widthMin()));
            putbs(",");
        }
        iterateConst(dispp->lhsp());
        emitDatap(dispp->lhsp());
        putbs(",");
    } இல்லையெனில் எனில் (VN_IS(nodep, SFormatF)) {
        isStmt = பொய்;
        putns(nodep, "VL_SFORMATF_N_NX(");
    } இல்லையெனில் {
        nodep->v3fatalSrc("Unknown displayEmit node type");
    }
    திரும்பு isStmt;
}

வெறுமை EmitCFunc::display_முனை(Ast_முனை* nodep, AstSFormatF* fmtp,  // fmtp is nullptr for AstScan
                            மாறிலி string& vformat, Ast_முனை* exprsp, தருக்க isScan) {
    // Check format, if it exists
    மாறிலி தருக்க கோவை_Format_2 = fmtp && fmtp->கோவை_Format_2();
    தருக்க needsScope = கோவை_Format_2;
    தருக்க needsTimescale = கோவை_Format_2;
    முழு formatArgs = 0;
    எனில் (!கோவை_Format_2) {
        // Check display and argument count to head-off later runtime issues
        தருக்க inPct = பொய்;
        தருக்க ignore = பொய்;
        க்கு (எழுத்து ch : vformat) {
            // UINFO(1, "Parse '" << *pos << "'  IP" << inPct << " List " << cvtToHex(elistp));
            எனில் (!inPct && ch == '%') {
                inPct = உண்மை;
                ignore = பொய்;
            } இல்லையெனில் எனில் (inPct && (std::isdigit(ch) || ch == '.' || ch == '-')) {
            } இல்லையெனில் எனில் (!inPct) {  // Normal text
            } இல்லையெனில் {  // Format character
                inPct = பொய்;
                ch = std::tolower(ch);
                தேர்வு (ch) {
                நிலை '%': நிறுத்து;
                நிலை '*':
                    inPct = உண்மை;  // Get more digits
                    ignore = உண்மை;
                    நிறுத்து;
                நிலை 'm': needsScope = உண்மை; நிறுத்து;
                நிலை 't':
                    needsTimescale = உண்மை;
                    ++formatArgs;
                    நிறுத்து;
                இயல்பு:
                    எனில் (!ignore && V3_எண்::displayedFmtHasArg(ch, isScan)) ++formatArgs;
                    நிறுத்து;
                }
            }
        }
    }

    எனில் (vformat.empty() && VN_IS(nodep, Display))  // not fscanf etc, as they need to return value
        திரும்பு;  // NOP

    மாறிலி தருக்க isStmt = displayEmitHeader(nodep);

    எனில் (கோவை_Format_2) {
        UASSERT_OBJ(exprsp, nodep, "Missing format expression");
        iterateConst(exprsp);
        emitDatap(exprsp);
        exprsp = exprsp->nextp();
    } இல்லையெனில் {
        ofp()->putsQuoted(vformat);
    }

    முழு கோவை_Args = 0;
    க்கு (Ast_முனை* argp = exprsp; argp; argp = argp->nextp()) ++கோவை_Args;
    எனில் (VL_UNCOVERABLE(!கோவை_Format_2 && கோவை_Args != formatArgs)) {  // LCOV_EXCL_START
        // expectDisplay() checks this first, so probably internal error if found here
        nodep->v3error("Internal: Missing or extra arguments for $display-like format, "
                       << " expression had " << கோவை_Args << ", format had " << formatArgs);
    }  // LCOV_EXCL_STOP

    // argc for error check. Also MSVC++ requires va_args to not be off a reference
    முழு argc = 0;
    எனில் (needsScope) ++argc;
    எனில் (needsTimescale) ++argc;
    க்கு (Ast_முனை* argp = exprsp; argp; argp = argp->nextp()) {
        ++argc;
        மாறிலி AstSFormatArg* மாறிலி fargp = VN_CAST(argp, SFormatArg);
        எனில் (fargp && fargp->formatAttr().isEnum()) ++argc;  // Additional name argument
    }
    ofp()->puts("," + std::to_string(argc));

    எனில் (needsScope) {
        AstScopeName* மாறிலி scopenamep = fmtp ? fmtp->scopeNamep() : nullptr;
        UASSERT_OBJ(scopenamep, nodep, "Display with %m but no AstScopeName");
        ofp()->puts(", '"s + VFormatAttr{VFormatAttr::SCOPE}.ascii() + "'");
        எனில் (scopenamep->selfRelative()) {
            // vlNamep is catName(name(), instance), so starts with '.' when the model is unnamed
            மாறிலி string namep = string{m_useSelfForThis ? "vlSelf" : "this"} + "->vlNamep";
            ofp()->puts(",(vlSymsp->name()[0] ? " + namep + " : " + namep + " + 1),");
            ofp()->putsQuoted(scopenamep->scopeAttr());
        } இல்லையெனில் {
            ofp()->puts(",vlSymsp->name(),");
            ofp()->putsQuoted(scopenamep->scopePrettySymName());
        }
    }

    எனில் (needsTimescale) {
        VTimescale timeunit = VTimescale::NONE;
        எனில் (மாறிலி AstDisplay* மாறிலி anodep = VN_CAST(nodep, Display)) {
            timeunit = anodep->fmtp()->timeunit();
        } இல்லையெனில் எனில் (மாறிலி AstSFormat* மாறிலி anodep = VN_CAST(nodep, SFormat)) {
            timeunit = anodep->fmtp()->timeunit();
        } இல்லையெனில் எனில் (மாறிலி AstSScanF* மாறிலி anodep = VN_CAST(nodep, SScanF)) {
            timeunit = anodep->timeunit();
        } இல்லையெனில் எனில் (மாறிலி AstSFormatF* மாறிலி anodep = VN_CAST(nodep, SFormatF)) {
            timeunit = anodep->timeunit();
        }
        UASSERT_OBJ(!timeunit.isNone(), nodep,
                    "Use of %t must be under AstDisplay, AstSFormat, or AstSFormatF, or "
                    "SScanF, and timeunit set");
        ofp()->puts(", '"s + VFormatAttr{VFormatAttr::TIMEUNIT}.ascii() + "'");
        ofp()->puts(","s + cvtToStr((முழு)timeunit.powerOfTen()));
    }

    // Arguments
    க்கு (Ast_முனை* argp = exprsp; argp; argp = argp->nextp()) {
        ofp()->indentInc();
        ofp()->putbs("");
        AstSFormatArg* மாறிலி fargp = VN_CAST(argp, SFormatArg);
        Ast_முனை* மாறிலி subargp = fargp ? fargp->exprp() : argp;
        மாறிலி VFormatAttr formatAttr = AstSFormatArg::formatAttrDefauled(fargp, subargp->dtypep());
        puts(", '"s + formatAttr.ascii() + '\'');
        எனில் (formatAttr.isSigned() || formatAttr.isUnsigned() || formatAttr.isEnum())
            puts("," + cvtToStr(subargp->widthMin()));
        மாறிலி தருக்க addrof = isScan || formatAttr.isString() || formatAttr.isComplex();
        மாறிலி தருக்க wideEnum = formatAttr.isEnum() && subargp->isWide();
        puts(",");
        எனில் (wideEnum) puts("static_cast<const EData*>(");
        எனில் (addrof) puts("&(");
        எனில் (VN_IS(subargp, StreamR))
            emitStreamR(
                VN_CAST(subargp, StreamR),
                nodep);  // This has to be done here because streamR doesn't know what it returns
        இல்லையெனில் { iterateConst(subargp); }
        எனில் (addrof) puts(")");
        எனில் (!addrof) emitDatap(argp);
        எனில் (wideEnum) puts(")");
        எனில் (formatAttr.isEnum()) {
            puts(", '"s + VFormatAttr{VFormatAttr::STRING}.ascii() + "', &(");
            iterateConst(fargp->namep());
            puts(")");
        }
        ofp()->indentDec();
    }

    // End
    எனில் (isStmt) {
        puts(");\n");
    } இல்லையெனில் {
        puts(") ");
    }
}

வெறுமை EmitCFunc::emitCCallArgs(மாறிலி Ast_முனை_CCall* nodep, மாறிலி string& selfPointer,
                              தருக்க inProcess) {
    putns(nodep, "(");
    தருக்க comma = பொய்;
    எனில் (nodep->funcp()->isLoose() && !nodep->funcp()->isStatic()) {
        UASSERT_OBJ(!selfPointer.empty(), nodep, "Call to loose method without self pointer");
        puts(selfPointer);
        comma = உண்மை;
    }
    எனில் (nodep->funcp()->needProcess()) {
        எனில் (comma) puts(", ");
        எனில் (VN_IS(nodep->backp(), CAwait) || !nodep->funcp()->isCoroutine()) {
            puts("vlProcess");
        } இல்லையெனில் எனில் (inProcess) {
            puts("std::make_shared<VlProcess>(vlProcess)");
        } இல்லையெனில் {
            puts("std::make_shared<VlProcess>()");
        }
        comma = உண்மை;
    }
    எனில் (!nodep->argTypes().empty()) {
        எனில் (comma) puts(", ");
        puts(nodep->argTypes());
        comma = உண்மை;
    }
    putCommaIterateNext(nodep->argsp(), comma);
    puts(")");
}

std::string EmitCFunc::dereferenceString(மாறிலி std::string& pointer) மாறிலி {
    எனில் (pointer[0] == '(' && pointer[1] == '&') {
        // remove "address of" followed by immediate dereference
        // Note: this relies on only the form '(&OBJECT)' being used by Verilator
        திரும்பு pointer.substr(2, pointer.length() - 3) + '.';
    } இல்லையெனில் {
        எனில் (pointer == "vlSelf" && m_usevlSelfRef) {
            திரும்பு "vlSelfRef.";
        } இல்லையெனில் {
            திரும்பு pointer + "->";
        }
    }
}
வெறுமை EmitCFunc::emitDereference(Ast_முனை* nodep, மாறிலி string& pointer) {
    putns(nodep, dereferenceString(pointer));
}

வெறுமை EmitCFunc::emitCvtPackStr(Ast_முனை* nodep) {
    எனில் (மாறிலி AstConst* மாறிலி constp = VN_CAST(nodep, Const)) {
        emitConstantString(constp);
    } இல்லையெனில் எனில் (VN_IS(nodep->dtypep(), StreamDType)) {
        putnbs(nodep, "VL_CVT_PACK_STR_ND(");
        iterateAndNextConstNull(nodep);
        puts(")");
    } இல்லையெனில் {
        putnbs(nodep, "VL_CVT_PACK_STR_N");
        emitIQW(nodep);
        puts("(");
        எனில் (nodep->isWide()) {
            // Note argument width, not node width (which is always 32)
            puts(cvtToStr(nodep->width_சொற்கள்()));
            puts(", ");
        }
        iterateAndNextConstNull(nodep);
        puts(")");
    }
}

வெறுமை EmitCFunc::emitConstant(AstConst* nodep) {
    // Put out constant set to the specified variable, or given variable in a string
    மாறிலி V3_எண்& இலக்கம் = nodep->இலக்கம்();
    எனில் (இலக்கம்.isFourState()) {
        nodep->v3warn(E_UNSUPPORTED, "Unsupported: 4-state numbers in this context");
        திரும்பு;
    }
    putns(nodep, இலக்கம்.emitC());
}

வெறுமை EmitCFunc::emitConstantString(மாறிலி AstConst* nodep) {
    // Const might be a Verilog array-type string, but need to always output std::string
    putnbs(nodep, "std::string{");
    மாறிலி string str = nodep->இலக்கம்().toString();
    எனில் (!str.empty()) putsQuoted(str);
    puts("}");
}

வெறுமை EmitCFunc::emitVarReset(மாறிலி string& prefix, AstVar* varp, தருக்க constructing) {
    // 'constructing' indicates that the object was just constructed, so if it is a string or
    // something that starts off clear already, no need to clear it again
    Ast_முனை_DType* மாறிலி dtypep = varp->dtypep()->skipRefp();
    மாறிலி string vlSelf = VSelfPointerText::replaceThis(m_useSelfForThis, "this->");
    மாறிலி string varNameProtected
        = ((VN_IS(m_modp, Class) || varp->isFuncLocal()) || !prefix.empty())
              ? varName(varp)
              : vlSelf + varName(varp);
    மாறிலி string newPrefix = prefix + varNameProtected;
    எனில் (varp->isIO() && m_modp->is_உச்சி() && optSystemC()) {
        // System C top I/O doesn't need loading, as the lower level subinst code does it.}
    } இல்லையெனில் எனில் (varp->isParam()) {
        UASSERT_OBJ(varp->valuep(), varp, "No init for a param?");
        // If a simple CONST value we initialize it using an enum
        // If an ARRAYINIT we initialize it using an initial block similar to a signal
        // puts("// parameter "+varp->nameProtect()+" = "+varp->valuep()->name()+"\n");
    } இல்லையெனில் {
        putns(varp,
              emitVarResetRecurse(varp, constructing, newPrefix, dtypep, 0, "", varp->valuep()));
    }
}

string EmitCFunc::emitVarResetRecurse(மாறிலி AstVar* varp, தருக்க constructing,
                                      மாறிலி string& varNameProtected, Ast_முனை_DType* dtypep,
                                      முழு depth, மாறிலி string& suffix, மாறிலி Ast_முனை* valuep) {
    dtypep = dtypep->skipRefp();
    AstBasicDType* மாறிலி basicp = dtypep->basicp();
    // Returns string to do resetting, empty to do nothing (which caller should handle)
    எனில் (AstAssocArrayDType* மாறிலி adtypep = VN_CAST(dtypep, AssocArrayDType)) {
        // Access std::array as C array
        மாறிலி string pre = constructing ? "" : varNameProtected + suffix + ".clear();\n";
        திரும்பு pre
               + emitVarResetRecurse(varp, constructing, varNameProtected, adtypep->subDTypep(),
                                     depth + 1, suffix + ".atDefault()", nullptr);
    } இல்லையெனில் எனில் (AstWildcardArrayDType* மாறிலி adtypep = VN_CAST(dtypep, WildcardArrayDType)) {
        // Access std::array as C array
        மாறிலி string pre = constructing ? "" : varNameProtected + suffix + ".clear();\n";
        திரும்பு pre
               + emitVarResetRecurse(varp, constructing, varNameProtected, adtypep->subDTypep(),
                                     depth + 1, suffix + ".atDefault()", nullptr);
    } இல்லையெனில் எனில் (VN_IS(dtypep, CDType)) {
        திரும்பு "";  // Constructor does it
    } இல்லையெனில் எனில் (VN_IS(dtypep, CoverCrossDType) || VN_IS(dtypep, CoverpointDType)) {
        திரும்பு "";  // Covergroup constructor creates the runtime and assigns the pointer
    } இல்லையெனில் எனில் (மாறிலி AstClassRefDType* மாறிலி adtypep = VN_CAST(dtypep, ClassRefDType)) {
        திரும்பு adtypep->rawPointer() ? varNameProtected + suffix + " = nullptr;\n" : "";
    } இல்லையெனில் எனில் (VN_IS(dtypep, IfaceRefDType)) {
        திரும்பு varNameProtected + suffix + " = nullptr;\n";
    } இல்லையெனில் எனில் (மாறிலி AstDynArrayDType* மாறிலி adtypep = VN_CAST(dtypep, DynArrayDType)) {
        மாறிலி string pre = constructing ? "" : varNameProtected + suffix + ".clear();\n";
        திரும்பு pre
               + emitVarResetRecurse(varp, constructing, varNameProtected, adtypep->subDTypep(),
                                     depth + 1, suffix + ".atDefault()", nullptr);
    } இல்லையெனில் எனில் (மாறிலி AstQueueDType* மாறிலி adtypep = VN_CAST(dtypep, QueueDType)) {
        மாறிலி string pre = constructing ? "" : varNameProtected + suffix + ".clear();\n";
        திரும்பு pre
               + emitVarResetRecurse(varp, constructing, varNameProtected, adtypep->subDTypep(),
                                     depth + 1, suffix + ".atDefault()", nullptr);
    } இல்லையெனில் எனில் (VN_IS(dtypep, SampleQueueDType)) {
        திரும்பு "";
    } இல்லையெனில் எனில் (மாறிலி AstUnpackArrayDType* மாறிலி adtypep = VN_CAST(dtypep, UnpackArrayDType)) {
        UASSERT_OBJ(adtypep->hi() >= adtypep->lo(), varp,
                    "Should have swapped msb & lsb earlier.");
        மாறிலி Ast_முனை_DType* மாறிலி elementp = adtypep->subDTypep()->skipRefp();
        எனில் (!varp->isFuncLocal() && elementp->isIntegralOrPacked()) {
            // Scoped reset has no array-index input or mutable RNG state. Initialize one
            // element, then fill in the optimized runtime even when this code is cold.
            மாறிலி string first
                = emitVarResetRecurse(varp, constructing, varNameProtected, adtypep->subDTypep(),
                                      depth + 1, suffix + "[0]", nullptr);
            மாறிலி string array = varNameProtected + suffix;
            திரும்பு first + "VL_UNPACKED_FILL(" + array + ".data(), sizeof(" + array + "[0]), "
                   + cvtToStr(adtypep->elementsConst()) + ");\n";
        }
        மாறிலி string ivar = "__Vi"s + cvtToStr(depth);
        மாறிலி string pre = ("for (int " + ivar + " = " + cvtToStr(0) + "; " + ivar + " < "
                            + cvtToStr(adtypep->elementsConst()) + "; ++" + ivar + ") {\n");
        மாறிலி string below
            = emitVarResetRecurse(varp, constructing, varNameProtected, adtypep->subDTypep(),
                                  depth + 1, suffix + "[" + ivar + "]", nullptr);
        மாறிலி string post = "}\n";
        திரும்பு below.empty() ? "" : pre + below + post;
    } இல்லையெனில் எனில் (VN_IS(dtypep, முனை_UOr_Struct_DType) && !VN_AS(dtypep, முனை_UOr_Struct_DType)->packed()) {
        மாறிலி Ast_முனை_UOr_Struct_DType* மாறிலி sdtypep = VN_AS(dtypep, முனை_UOr_Struct_DType);
        string literal;
        க்கு (மாறிலி AstMemberDType* itemp = sdtypep->membersp(); itemp;
             itemp = VN_AS(itemp->nextp(), MemberDType)) {
            மாறிலி std::string வரி = emitVarResetRecurse(
                varp, constructing, varNameProtected + suffix + "." + itemp->nameProtect(),
                itemp->dtypep(), depth + 1, "", itemp->valuep());
            எனில் (!வரி.empty()) literal += வரி;
        }
        திரும்பு literal;
    } இல்லையெனில் எனில் (basicp && basicp->keyword() == VBasicDTypeKwd::STRING) {
        எனில் (constructing) திரும்பு "";  // String's constructor deals with it
        திரும்பு varNameProtected + suffix + ".clear();\n";
    } இல்லையெனில் எனில் (basicp && basicp->isForkSync()) {
        திரும்பு "";
    } இல்லையெனில் எனில் (basicp && basicp->isProcessRef()) {
        திரும்பு "";
    } இல்லையெனில் எனில் (basicp && basicp->isDelayScheduler()) {
        திரும்பு "";
    } இல்லையெனில் எனில் (basicp && basicp->isTriggerScheduler()) {
        திரும்பு "";
    } இல்லையெனில் எனில் (basicp && basicp->isDynamicTriggerScheduler()) {
        திரும்பு "";
    } இல்லையெனில் எனில் (basicp && (basicp->isRandomGenerator() || basicp->isStdRandomGenerator())) {
        திரும்பு "";
    } இல்லையெனில் எனில் (basicp && basicp->isCovergroupInstHandle()) {
        // The handle's own constructor deals with it.
        திரும்பு "";
    } இல்லையெனில் எனில் (basicp && (basicp->isEvent())) {
        திரும்பு "VlAssignableEvent{};\n";
    } இல்லையெனில் எனில் (basicp) {
        மாறிலி தருக்க zeroit
            = (varp->attrFileDescr()  // Zero so we don't do file IO if never $fopen
               || varp->isFuncLocal()  // Randomization too slow
               || (basicp && basicp->isZeroInit())
               || (v3Global.opt.underlineZero() && !varp->name().empty() && varp->name()[0] == '_')
               || (varp->varType().isTemp() && !varp->isXTemp())
               || (varp->isXTemp()
                       ? (v3Global.opt.xAssign() != "unique")
                       : (v3Global.opt.xInitial() == "fast" || v3Global.opt.xInitial() == "0")));
        மாறிலி தருக்க slow = !varp->isFuncLocal() && !varp->isClassMember();
        splitSizeInc(1);
        எனில் (dtypep->isWide()) {  // Handle unpacked; not basicp->isWide
            string out;
            எனில் (valuep) {
                மாறிலி AstConst* மாறிலி constp = VN_AS(valuep, Const);
                UASSERT_OBJ(constp, varp, "non-const initializer for variable");
                க்கு (முழு w = 0; w < dtypep->width_சொற்கள்(); ++w) {
                    out += varNameProtected + suffix + "[" + cvtToStr(w) + "] = ";
                    out += cvtToStr(constp->இலக்கம்().edataWord(w)) + "U;\n";
                }
            } இல்லையெனில் {
                out += zeroit ? (slow ? "VL_ZERO_RESET_W(" : "VL_ZERO_W(")
                              : (varp->isXTemp() ? "VL_SCOPED_RAND_RESET_ASSIGN_W("
                                                 : "VL_SCOPED_RAND_RESET_W(");
                out += cvtToStr(dtypep->widthMin());
                out += ", " + varNameProtected + suffix;
                எனில் (!zeroit) {
                    emitVarResetScopeHash();
                    மாறிலி uint64_t salt = VString::hashMurmur(varp->prettyName());
                    out += ", ";
                    out += m_classOrPackage ? m_classOrPackageHash : "__VscopeHash";
                    out += ", ";
                    out += std::to_string(salt);
                    out += "ull";
                }
                out += ");\n";
            }
            திரும்பு out;
        } இல்லையெனில் {
            string out = varNameProtected + suffix;
            எனில் (valuep) {
                out += " = ";
                // TODO cleanup code shared between here, V3EmitCConstInit.h,
                // EmitCFunc::emitVarReset, EmitCFunc::emitConstant
                மாறிலி AstConst* மாறிலி constp = VN_AS(valuep, Const);
                UASSERT_OBJ(constp, varp, "non-const initializer for variable");
                out += cvtToStr(constp->இலக்கம்().edataWord(0)) + "U;\n";
                out += ";\n";
            } இல்லையெனில் எனில் (zeroit) {
                out += " = 0;\n";
            } இல்லையெனில் {
                emitVarResetScopeHash();
                மாறிலி uint64_t salt = VString::hashMurmur(varp->prettyName());
                out += " = VL_SCOPED_RAND_RESET_";
                எனில் (varp->isXTemp()) out += "ASSIGN_";
                out += dtypep->charIQWN();
                out += "(" + cvtToStr(dtypep->widthMin()) + ", "
                       + (m_classOrPackage ? m_classOrPackageHash : "__VscopeHash") + ", "
                       + std::to_string(salt) + "ull);\n";
            }
            திரும்பு out;
        }
    } இல்லையெனில் {  // LCOV_EXCL_BR_LINE
        v3fatalSrc("Unknown node type in reset generator: " << varp->prettyTypeName());
    }
    திரும்பு "";
}

வெறுமை EmitCFunc::emitVarResetScopeHash() {
    எனில் (VL_LIKELY(m_createdScopeHash)) { திரும்பு; }
    எனில் (m_classOrPackage) {
        m_classOrPackageHash
            = std::to_string(VString::hashMurmur(m_classOrPackage->name())) + "ULL";
    } இல்லையெனில் {
        puts(string("const uint64_t __VscopeHash = VL_MURMUR64_HASH(")
             + (m_useSelfForThis ? "vlSelf" : "this") + "->vlNamep);\n");
    }
    m_createdScopeHash = உண்மை;
}
