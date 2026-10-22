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

#अन्तर्भाव "V3PchAstMT.h"

#अन्तर्भाव "V3EmitCFunc.h"

#अन्तर्भाव <map>
#अन्तर्भाव <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// EmitCFunc

तार्किक EmitCFunc::emitSimpleOk(Ast_ग्रन्थिः_व्यञ्जकम्* nodep) {
    // Can we put out a simple (A + B) instead of VL_ADD_III(A,B)?
    जइ (nodep->emitSimpleOperator() == "") प्रत्यागम मिच्छ;
    जइ (nodep->isWide()) प्रत्यागम मिच्छ;
    जइ (nodep->op1p() && nodep->op1p()->isWide()) प्रत्यागम मिच्छ;
    जइ (nodep->op2p() && nodep->op2p()->isWide()) प्रत्यागम मिच्छ;
    जइ (nodep->op3p() && nodep->op3p()->isWide()) प्रत्यागम मिच्छ;
    प्रत्यागम सच्च;
}

शून्य EmitCFunc::emitOpName(Ast_ग्रन्थिः* nodep, ध्रुव string& format, Ast_ग्रन्थिः* lhsp, Ast_ग्रन्थिः* rhsp,
                           Ast_ग्रन्थिः* thsp) {
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

    तार्किक needComma = मिच्छ;
    तार्किक usesQueue = मिच्छ;
    string nextComma;
    auto commaOut = [&out, &nextComma]() {
        जइ (!nextComma.empty()) {
            out += nextComma;
            nextComma = "";
        }
    };

    auto putOut = [एतत्, &out]() {
        जइ (!out.empty()) puts(out);
        out = "";
    };

    प्रति (string::const_iterator pos = format.begin(); pos != format.end(); ++pos) {
        जइ (pos[0] == ',') {
            // Remember we need to add one, but don't do yet to avoid ",)"
            जइ (needComma) {
                जइ (pos[1] == ' ') {
                    nextComma = ", ";
                } अण्णहा {
                    nextComma = ",";
                }
                needComma = मिच्छ;
            }
            जइ (pos[1] == ' ') ++pos;  // Must do even if no nextComma
        } अण्णहा जइ (pos[0] == '%') {
            ++pos;
            तार्किक detail = मिच्छ;
            Ast_ग्रन्थिः* detailp = nullptr;
            विकल्प (pos[0]) {
            पक्ष '%': out += '%'; भङ्ग;
            पक्ष 'k':
                putOut();
                putbs("");
                भङ्ग;
            पक्ष 'n':
                detail = सच्च;
                detailp = nodep;
                भङ्ग;
            पक्ष 'l':
                detail = सच्च;
                detailp = lhsp;
                भङ्ग;
            पक्ष 'r':
                detail = सच्च;
                detailp = rhsp;
                भङ्ग;
            पक्ष 't':
                detail = सच्च;
                detailp = thsp;
                भङ्ग;
            पक्ष 'P':
                जइ (nodep->isWide() && !usesQueue) {
                    UASSERT_OBJ(m_wideTempRefp, nodep,
                                "Wide op " << nodep->prettyTypeName()
                                           << " w/ no temp, perhaps missing op in V3EmitC?");
                    commaOut();
                    putOut();
                    जइ (!m_wideTempRefp->selfPointer().isEmpty()) {
                        emitDereference(m_wideTempRefp,
                                        m_wideTempRefp->selfPointerProtect(m_useSelfForThis));
                    }
                    out += varName(m_wideTempRefp->varp());
                    m_wideTempRefp = nullptr;
                    needComma = सच्च;
                } अण्णहा जइ (usesQueue) {
                    commaOut();
                    putOut();
                    iterateAndNextConstNull(nodep->backp()->op2p());
                    needComma = सच्च;
                }

                भङ्ग;
            स्वभाव: nodep->v3fatalSrc("Unknown emitOperator format code: %" << pos[0]); भङ्ग;
            }
            जइ (detail) {
                // Get next letter of %[nlrt]
                ++pos;
                विकल्प (pos[0]) {
                पक्ष 'q':
                    putOut();
                    // If we are assigning this to a queue we need to get the return type
                    जइ (VN_IS(detailp->backp(), Assign)
                        && VN_IS(detailp->backp()->op2p()->dtypep()->skipRefp(), QueueDType)) {
                        puts("R");  // R for queue
                        usesQueue = सच्च;
                    } अण्णहा जइ (VN_IS(detailp->dtypep()->skipRefp(), QueueDType)
                               || VN_IS(detailp->dtypep()->skipRefp(), StreamDType)) {
                        puts("R");  // R for queue
                    } अण्णहा {
                        emitIQW(detailp);
                    }
                    भङ्ग;
                पक्ष 'w':
                    commaOut();
                    out += cvtToStr(detailp->widthMin());
                    needComma = सच्च;
                    भङ्ग;
                पक्ष 'W':
                    जइ (lhsp->isWide()) {
                        commaOut();
                        out += cvtToStr(lhsp->width_शब्दाः());
                        needComma = सच्च;
                    } अण्णहा जइ (VN_IS(lhsp, StreamR)) {
                        commaOut();
                        out += cvtToStr(rhsp->width_शब्दाः());
                        needComma = सच्च;
                    }
                    भङ्ग;
                पक्ष 'i':
                    commaOut();
                    UASSERT_OBJ(detailp, nodep, "emitOperator() references undef node");
                    putOut();
                    iterateAndNextConstNull(detailp);
                    needComma = सच्च;
                    भङ्ग;
                स्वभाव:
                    nodep->v3fatalSrc("Unknown emitOperator format code: %[nlrt]" << pos[0]);
                    भङ्ग;
                }
            }
        } अण्णहा जइ (pos[0] == ')') {
            nextComma = "";
            out += ')';
        } अण्णहा जइ (pos[0] == '(') {
            commaOut();
            needComma = मिच्छ;
            out += '(';
        } अण्णहा {
            // Normal text
            जइ (std::isalnum(pos[0])) needComma = सच्च;
            commaOut();
            out += pos[0];
        }
    }
    putOut();
}

तार्किक EmitCFunc::displayEmitHeader(Ast_ग्रन्थिः* nodep) {
    तार्किक isStmt = मिच्छ;
    जइ (ध्रुव AstFScanF* ध्रुव dispp = VN_CAST(nodep, FScanF)) {
        isStmt = मिच्छ;
        putns(nodep, "VL_FSCANF_INX(");
        iterateConst(dispp->filep());
        puts(",");
    } अण्णहा जइ (ध्रुव AstSScanF* ध्रुव dispp = VN_CAST(nodep, SScanF)) {
        isStmt = मिच्छ;
        putns(nodep, "VL_SSCANF_I");
        emitIQW(dispp->fromp());
        puts("NX(");
        puts(cvtToStr(dispp->fromp()->widthMin()));
        puts(",");
        iterateConst(dispp->fromp());
        puts(",");
    } अण्णहा जइ (ध्रुव AstDisplay* ध्रुव dispp = VN_CAST(nodep, Display)) {
        isStmt = सच्च;
        जइ (dispp->filep()) {
            putns(nodep, "VL_FWRITEF_NX(");
            iterateConst(dispp->filep());
            puts(",");
        } अण्णहा {
            putns(nodep, "VL_WRITEF_NX(");
        }
    } अण्णहा जइ (ध्रुव AstSFormat* ध्रुव dispp = VN_CAST(nodep, SFormat)) {
        isStmt = सच्च;
        puts("VL_SFORMAT_NX(");
        जइ (!dispp->lhsp()->dtypep()->isString()) {
            puts(cvtToStr(dispp->lhsp()->widthMin()));
            putbs(",");
        }
        iterateConst(dispp->lhsp());
        emitDatap(dispp->lhsp());
        putbs(",");
    } अण्णहा जइ (VN_IS(nodep, SFormatF)) {
        isStmt = मिच्छ;
        putns(nodep, "VL_SFORMATF_N_NX(");
    } अण्णहा {
        nodep->v3fatalSrc("Unknown displayEmit node type");
    }
    प्रत्यागम isStmt;
}

शून्य EmitCFunc::display_ग्रन्थिः(Ast_ग्रन्थिः* nodep, AstSFormatF* fmtp,  // fmtp is nullptr for AstScan
                            ध्रुव string& vformat, Ast_ग्रन्थिः* exprsp, तार्किक isScan) {
    // Check format, if it exists
    ध्रुव तार्किक व्यञ्जकम्_Format_2 = fmtp && fmtp->व्यञ्जकम्_Format_2();
    तार्किक needsScope = व्यञ्जकम्_Format_2;
    तार्किक needsTimescale = व्यञ्जकम्_Format_2;
    पूर्णाङ्क formatArgs = 0;
    जइ (!व्यञ्जकम्_Format_2) {
        // Check display and argument count to head-off later runtime issues
        तार्किक inPct = मिच्छ;
        तार्किक ignore = मिच्छ;
        प्रति (वर्ण ch : vformat) {
            // UINFO(1, "Parse '" << *pos << "'  IP" << inPct << " List " << cvtToHex(elistp));
            जइ (!inPct && ch == '%') {
                inPct = सच्च;
                ignore = मिच्छ;
            } अण्णहा जइ (inPct && (std::isdigit(ch) || ch == '.' || ch == '-')) {
            } अण्णहा जइ (!inPct) {  // Normal text
            } अण्णहा {  // Format character
                inPct = मिच्छ;
                ch = std::tolower(ch);
                विकल्प (ch) {
                पक्ष '%': भङ्ग;
                पक्ष '*':
                    inPct = सच्च;  // Get more digits
                    ignore = सच्च;
                    भङ्ग;
                पक्ष 'm': needsScope = सच्च; भङ्ग;
                पक्ष 't':
                    needsTimescale = सच्च;
                    ++formatArgs;
                    भङ्ग;
                स्वभाव:
                    जइ (!ignore && V3_सङ्ख्या::displayedFmtHasArg(ch, isScan)) ++formatArgs;
                    भङ्ग;
                }
            }
        }
    }

    जइ (vformat.empty() && VN_IS(nodep, Display))  // not fscanf etc, as they need to return value
        प्रत्यागम;  // NOP

    ध्रुव तार्किक isStmt = displayEmitHeader(nodep);

    जइ (व्यञ्जकम्_Format_2) {
        UASSERT_OBJ(exprsp, nodep, "Missing format expression");
        iterateConst(exprsp);
        emitDatap(exprsp);
        exprsp = exprsp->nextp();
    } अण्णहा {
        ofp()->putsQuoted(vformat);
    }

    पूर्णाङ्क व्यञ्जकम्_Args = 0;
    प्रति (Ast_ग्रन्थिः* argp = exprsp; argp; argp = argp->nextp()) ++व्यञ्जकम्_Args;
    जइ (VL_UNCOVERABLE(!व्यञ्जकम्_Format_2 && व्यञ्जकम्_Args != formatArgs)) {  // LCOV_EXCL_START
        // expectDisplay() checks this first, so probably internal error if found here
        nodep->v3error("Internal: Missing or extra arguments for $display-like format, "
                       << " expression had " << व्यञ्जकम्_Args << ", format had " << formatArgs);
    }  // LCOV_EXCL_STOP

    // argc for error check. Also MSVC++ requires va_args to not be off a reference
    पूर्णाङ्क argc = 0;
    जइ (needsScope) ++argc;
    जइ (needsTimescale) ++argc;
    प्रति (Ast_ग्रन्थिः* argp = exprsp; argp; argp = argp->nextp()) {
        ++argc;
        ध्रुव AstSFormatArg* ध्रुव fargp = VN_CAST(argp, SFormatArg);
        जइ (fargp && fargp->formatAttr().isEnum()) ++argc;  // Additional name argument
    }
    ofp()->puts("," + std::to_string(argc));

    जइ (needsScope) {
        AstScopeName* ध्रुव scopenamep = fmtp ? fmtp->scopeNamep() : nullptr;
        UASSERT_OBJ(scopenamep, nodep, "Display with %m but no AstScopeName");
        ofp()->puts(", '"s + VFormatAttr{VFormatAttr::SCOPE}.ascii() + "'");
        जइ (scopenamep->selfRelative()) {
            // vlNamep is catName(name(), instance), so starts with '.' when the model is unnamed
            ध्रुव string namep = string{m_useSelfForThis ? "vlSelf" : "this"} + "->vlNamep";
            ofp()->puts(",(vlSymsp->name()[0] ? " + namep + " : " + namep + " + 1),");
            ofp()->putsQuoted(scopenamep->scopeAttr());
        } अण्णहा {
            ofp()->puts(",vlSymsp->name(),");
            ofp()->putsQuoted(scopenamep->scopePrettySymName());
        }
    }

    जइ (needsTimescale) {
        VTimescale timeunit = VTimescale::NONE;
        जइ (ध्रुव AstDisplay* ध्रुव anodep = VN_CAST(nodep, Display)) {
            timeunit = anodep->fmtp()->timeunit();
        } अण्णहा जइ (ध्रुव AstSFormat* ध्रुव anodep = VN_CAST(nodep, SFormat)) {
            timeunit = anodep->fmtp()->timeunit();
        } अण्णहा जइ (ध्रुव AstSScanF* ध्रुव anodep = VN_CAST(nodep, SScanF)) {
            timeunit = anodep->timeunit();
        } अण्णहा जइ (ध्रुव AstSFormatF* ध्रुव anodep = VN_CAST(nodep, SFormatF)) {
            timeunit = anodep->timeunit();
        }
        UASSERT_OBJ(!timeunit.isNone(), nodep,
                    "Use of %t must be under AstDisplay, AstSFormat, or AstSFormatF, or "
                    "SScanF, and timeunit set");
        ofp()->puts(", '"s + VFormatAttr{VFormatAttr::TIMEUNIT}.ascii() + "'");
        ofp()->puts(","s + cvtToStr((पूर्णाङ्क)timeunit.powerOfTen()));
    }

    // Arguments
    प्रति (Ast_ग्रन्थिः* argp = exprsp; argp; argp = argp->nextp()) {
        ofp()->indentInc();
        ofp()->putbs("");
        AstSFormatArg* ध्रुव fargp = VN_CAST(argp, SFormatArg);
        Ast_ग्रन्थिः* ध्रुव subargp = fargp ? fargp->exprp() : argp;
        ध्रुव VFormatAttr formatAttr = AstSFormatArg::formatAttrDefauled(fargp, subargp->dtypep());
        puts(", '"s + formatAttr.ascii() + '\'');
        जइ (formatAttr.isSigned() || formatAttr.isUnsigned() || formatAttr.isEnum())
            puts("," + cvtToStr(subargp->widthMin()));
        ध्रुव तार्किक addrof = isScan || formatAttr.isString() || formatAttr.isComplex();
        ध्रुव तार्किक wideEnum = formatAttr.isEnum() && subargp->isWide();
        puts(",");
        जइ (wideEnum) puts("static_cast<const EData*>(");
        जइ (addrof) puts("&(");
        जइ (VN_IS(subargp, StreamR))
            emitStreamR(
                VN_CAST(subargp, StreamR),
                nodep);  // This has to be done here because streamR doesn't know what it returns
        अण्णहा { iterateConst(subargp); }
        जइ (addrof) puts(")");
        जइ (!addrof) emitDatap(argp);
        जइ (wideEnum) puts(")");
        जइ (formatAttr.isEnum()) {
            puts(", '"s + VFormatAttr{VFormatAttr::STRING}.ascii() + "', &(");
            iterateConst(fargp->namep());
            puts(")");
        }
        ofp()->indentDec();
    }

    // End
    जइ (isStmt) {
        puts(");\n");
    } अण्णहा {
        puts(") ");
    }
}

शून्य EmitCFunc::emitCCallArgs(ध्रुव Ast_ग्रन्थिः_CCall* nodep, ध्रुव string& selfPointer,
                              तार्किक inProcess) {
    putns(nodep, "(");
    तार्किक comma = मिच्छ;
    जइ (nodep->funcp()->isLoose() && !nodep->funcp()->isStatic()) {
        UASSERT_OBJ(!selfPointer.empty(), nodep, "Call to loose method without self pointer");
        puts(selfPointer);
        comma = सच्च;
    }
    जइ (nodep->funcp()->needProcess()) {
        जइ (comma) puts(", ");
        जइ (VN_IS(nodep->backp(), CAwait) || !nodep->funcp()->isCoroutine()) {
            puts("vlProcess");
        } अण्णहा जइ (inProcess) {
            puts("std::make_shared<VlProcess>(vlProcess)");
        } अण्णहा {
            puts("std::make_shared<VlProcess>()");
        }
        comma = सच्च;
    }
    जइ (!nodep->argTypes().empty()) {
        जइ (comma) puts(", ");
        puts(nodep->argTypes());
        comma = सच्च;
    }
    putCommaIterateNext(nodep->argsp(), comma);
    puts(")");
}

std::string EmitCFunc::dereferenceString(ध्रुव std::string& pointer) ध्रुव {
    जइ (pointer[0] == '(' && pointer[1] == '&') {
        // remove "address of" followed by immediate dereference
        // Note: this relies on only the form '(&OBJECT)' being used by Verilator
        प्रत्यागम pointer.substr(2, pointer.length() - 3) + '.';
    } अण्णहा {
        जइ (pointer == "vlSelf" && m_usevlSelfRef) {
            प्रत्यागम "vlSelfRef.";
        } अण्णहा {
            प्रत्यागम pointer + "->";
        }
    }
}
शून्य EmitCFunc::emitDereference(Ast_ग्रन्थिः* nodep, ध्रुव string& pointer) {
    putns(nodep, dereferenceString(pointer));
}

शून्य EmitCFunc::emitCvtPackStr(Ast_ग्रन्थिः* nodep) {
    जइ (ध्रुव AstConst* ध्रुव constp = VN_CAST(nodep, Const)) {
        emitConstantString(constp);
    } अण्णहा जइ (VN_IS(nodep->dtypep(), StreamDType)) {
        putnbs(nodep, "VL_CVT_PACK_STR_ND(");
        iterateAndNextConstNull(nodep);
        puts(")");
    } अण्णहा {
        putnbs(nodep, "VL_CVT_PACK_STR_N");
        emitIQW(nodep);
        puts("(");
        जइ (nodep->isWide()) {
            // Note argument width, not node width (which is always 32)
            puts(cvtToStr(nodep->width_शब्दाः()));
            puts(", ");
        }
        iterateAndNextConstNull(nodep);
        puts(")");
    }
}

शून्य EmitCFunc::emitConstant(AstConst* nodep) {
    // Put out constant set to the specified variable, or given variable in a string
    ध्रुव V3_सङ्ख्या& अङ्कः = nodep->अङ्कः();
    जइ (अङ्कः.isFourState()) {
        nodep->v3warn(E_UNSUPPORTED, "Unsupported: 4-state numbers in this context");
        प्रत्यागम;
    }
    putns(nodep, अङ्कः.emitC());
}

शून्य EmitCFunc::emitConstantString(ध्रुव AstConst* nodep) {
    // Const might be a Verilog array-type string, but need to always output std::string
    putnbs(nodep, "std::string{");
    ध्रुव string str = nodep->अङ्कः().toString();
    जइ (!str.empty()) putsQuoted(str);
    puts("}");
}

शून्य EmitCFunc::emitVarReset(ध्रुव string& prefix, AstVar* varp, तार्किक constructing) {
    // 'constructing' indicates that the object was just constructed, so if it is a string or
    // something that starts off clear already, no need to clear it again
    Ast_ग्रन्थिः_DType* ध्रुव dtypep = varp->dtypep()->skipRefp();
    ध्रुव string vlSelf = VSelfPointerText::replaceThis(m_useSelfForThis, "this->");
    ध्रुव string varNameProtected
        = ((VN_IS(m_modp, Class) || varp->isFuncLocal()) || !prefix.empty())
              ? varName(varp)
              : vlSelf + varName(varp);
    ध्रुव string newPrefix = prefix + varNameProtected;
    जइ (varp->isIO() && m_modp->is_शिखरम्() && optSystemC()) {
        // System C top I/O doesn't need loading, as the lower level subinst code does it.}
    } अण्णहा जइ (varp->isParam()) {
        UASSERT_OBJ(varp->valuep(), varp, "No init for a param?");
        // If a simple CONST value we initialize it using an enum
        // If an ARRAYINIT we initialize it using an initial block similar to a signal
        // puts("// parameter "+varp->nameProtect()+" = "+varp->valuep()->name()+"\n");
    } अण्णहा {
        putns(varp,
              emitVarResetRecurse(varp, constructing, newPrefix, dtypep, 0, "", varp->valuep()));
    }
}

string EmitCFunc::emitVarResetRecurse(ध्रुव AstVar* varp, तार्किक constructing,
                                      ध्रुव string& varNameProtected, Ast_ग्रन्थिः_DType* dtypep,
                                      पूर्णाङ्क depth, ध्रुव string& suffix, ध्रुव Ast_ग्रन्थिः* valuep) {
    dtypep = dtypep->skipRefp();
    AstBasicDType* ध्रुव basicp = dtypep->basicp();
    // Returns string to do resetting, empty to do nothing (which caller should handle)
    जइ (AstAssocArrayDType* ध्रुव adtypep = VN_CAST(dtypep, AssocArrayDType)) {
        // Access std::array as C array
        ध्रुव string pre = constructing ? "" : varNameProtected + suffix + ".clear();\n";
        प्रत्यागम pre
               + emitVarResetRecurse(varp, constructing, varNameProtected, adtypep->subDTypep(),
                                     depth + 1, suffix + ".atDefault()", nullptr);
    } अण्णहा जइ (AstWildcardArrayDType* ध्रुव adtypep = VN_CAST(dtypep, WildcardArrayDType)) {
        // Access std::array as C array
        ध्रुव string pre = constructing ? "" : varNameProtected + suffix + ".clear();\n";
        प्रत्यागम pre
               + emitVarResetRecurse(varp, constructing, varNameProtected, adtypep->subDTypep(),
                                     depth + 1, suffix + ".atDefault()", nullptr);
    } अण्णहा जइ (VN_IS(dtypep, CDType)) {
        प्रत्यागम "";  // Constructor does it
    } अण्णहा जइ (VN_IS(dtypep, CoverCrossDType) || VN_IS(dtypep, CoverpointDType)) {
        प्रत्यागम "";  // Covergroup constructor creates the runtime and assigns the pointer
    } अण्णहा जइ (ध्रुव AstClassRefDType* ध्रुव adtypep = VN_CAST(dtypep, ClassRefDType)) {
        प्रत्यागम adtypep->rawPointer() ? varNameProtected + suffix + " = nullptr;\n" : "";
    } अण्णहा जइ (VN_IS(dtypep, IfaceRefDType)) {
        प्रत्यागम varNameProtected + suffix + " = nullptr;\n";
    } अण्णहा जइ (ध्रुव AstDynArrayDType* ध्रुव adtypep = VN_CAST(dtypep, DynArrayDType)) {
        ध्रुव string pre = constructing ? "" : varNameProtected + suffix + ".clear();\n";
        प्रत्यागम pre
               + emitVarResetRecurse(varp, constructing, varNameProtected, adtypep->subDTypep(),
                                     depth + 1, suffix + ".atDefault()", nullptr);
    } अण्णहा जइ (ध्रुव AstQueueDType* ध्रुव adtypep = VN_CAST(dtypep, QueueDType)) {
        ध्रुव string pre = constructing ? "" : varNameProtected + suffix + ".clear();\n";
        प्रत्यागम pre
               + emitVarResetRecurse(varp, constructing, varNameProtected, adtypep->subDTypep(),
                                     depth + 1, suffix + ".atDefault()", nullptr);
    } अण्णहा जइ (VN_IS(dtypep, SampleQueueDType)) {
        प्रत्यागम "";
    } अण्णहा जइ (ध्रुव AstUnpackArrayDType* ध्रुव adtypep = VN_CAST(dtypep, UnpackArrayDType)) {
        UASSERT_OBJ(adtypep->hi() >= adtypep->lo(), varp,
                    "Should have swapped msb & lsb earlier.");
        ध्रुव Ast_ग्रन्थिः_DType* ध्रुव elementp = adtypep->subDTypep()->skipRefp();
        जइ (!varp->isFuncLocal() && elementp->isIntegralOrPacked()) {
            // Scoped reset has no array-index input or mutable RNG state. Initialize one
            // element, then fill in the optimized runtime even when this code is cold.
            ध्रुव string first
                = emitVarResetRecurse(varp, constructing, varNameProtected, adtypep->subDTypep(),
                                      depth + 1, suffix + "[0]", nullptr);
            ध्रुव string array = varNameProtected + suffix;
            प्रत्यागम first + "VL_UNPACKED_FILL(" + array + ".data(), sizeof(" + array + "[0]), "
                   + cvtToStr(adtypep->elementsConst()) + ");\n";
        }
        ध्रुव string ivar = "__Vi"s + cvtToStr(depth);
        ध्रुव string pre = ("for (int " + ivar + " = " + cvtToStr(0) + "; " + ivar + " < "
                            + cvtToStr(adtypep->elementsConst()) + "; ++" + ivar + ") {\n");
        ध्रुव string below
            = emitVarResetRecurse(varp, constructing, varNameProtected, adtypep->subDTypep(),
                                  depth + 1, suffix + "[" + ivar + "]", nullptr);
        ध्रुव string post = "}\n";
        प्रत्यागम below.empty() ? "" : pre + below + post;
    } अण्णहा जइ (VN_IS(dtypep, ग्रन्थिः_UOr_Struct_DType) && !VN_AS(dtypep, ग्रन्थिः_UOr_Struct_DType)->packed()) {
        ध्रुव Ast_ग्रन्थिः_UOr_Struct_DType* ध्रुव sdtypep = VN_AS(dtypep, ग्रन्थिः_UOr_Struct_DType);
        string literal;
        प्रति (ध्रुव AstMemberDType* itemp = sdtypep->membersp(); itemp;
             itemp = VN_AS(itemp->nextp(), MemberDType)) {
            ध्रुव std::string पङ्क्तिः = emitVarResetRecurse(
                varp, constructing, varNameProtected + suffix + "." + itemp->nameProtect(),
                itemp->dtypep(), depth + 1, "", itemp->valuep());
            जइ (!पङ्क्तिः.empty()) literal += पङ्क्तिः;
        }
        प्रत्यागम literal;
    } अण्णहा जइ (basicp && basicp->keyword() == VBasicDTypeKwd::STRING) {
        जइ (constructing) प्रत्यागम "";  // String's constructor deals with it
        प्रत्यागम varNameProtected + suffix + ".clear();\n";
    } अण्णहा जइ (basicp && basicp->isForkSync()) {
        प्रत्यागम "";
    } अण्णहा जइ (basicp && basicp->isProcessRef()) {
        प्रत्यागम "";
    } अण्णहा जइ (basicp && basicp->isDelayScheduler()) {
        प्रत्यागम "";
    } अण्णहा जइ (basicp && basicp->isTriggerScheduler()) {
        प्रत्यागम "";
    } अण्णहा जइ (basicp && basicp->isDynamicTriggerScheduler()) {
        प्रत्यागम "";
    } अण्णहा जइ (basicp && (basicp->isRandomGenerator() || basicp->isStdRandomGenerator())) {
        प्रत्यागम "";
    } अण्णहा जइ (basicp && basicp->isCovergroupInstHandle()) {
        // The handle's own constructor deals with it.
        प्रत्यागम "";
    } अण्णहा जइ (basicp && (basicp->isEvent())) {
        प्रत्यागम "VlAssignableEvent{};\n";
    } अण्णहा जइ (basicp) {
        ध्रुव तार्किक zeroit
            = (varp->attrFileDescr()  // Zero so we don't do file IO if never $fopen
               || varp->isFuncLocal()  // Randomization too slow
               || (basicp && basicp->isZeroInit())
               || (v3Global.opt.underlineZero() && !varp->name().empty() && varp->name()[0] == '_')
               || (varp->varType().isTemp() && !varp->isXTemp())
               || (varp->isXTemp()
                       ? (v3Global.opt.xAssign() != "unique")
                       : (v3Global.opt.xInitial() == "fast" || v3Global.opt.xInitial() == "0")));
        ध्रुव तार्किक slow = !varp->isFuncLocal() && !varp->isClassMember();
        splitSizeInc(1);
        जइ (dtypep->isWide()) {  // Handle unpacked; not basicp->isWide
            string out;
            जइ (valuep) {
                ध्रुव AstConst* ध्रुव constp = VN_AS(valuep, Const);
                UASSERT_OBJ(constp, varp, "non-const initializer for variable");
                प्रति (पूर्णाङ्क w = 0; w < dtypep->width_शब्दाः(); ++w) {
                    out += varNameProtected + suffix + "[" + cvtToStr(w) + "] = ";
                    out += cvtToStr(constp->अङ्कः().edataWord(w)) + "U;\n";
                }
            } अण्णहा {
                out += zeroit ? (slow ? "VL_ZERO_RESET_W(" : "VL_ZERO_W(")
                              : (varp->isXTemp() ? "VL_SCOPED_RAND_RESET_ASSIGN_W("
                                                 : "VL_SCOPED_RAND_RESET_W(");
                out += cvtToStr(dtypep->widthMin());
                out += ", " + varNameProtected + suffix;
                जइ (!zeroit) {
                    emitVarResetScopeHash();
                    ध्रुव uint64_t salt = VString::hashMurmur(varp->prettyName());
                    out += ", ";
                    out += m_classOrPackage ? m_classOrPackageHash : "__VscopeHash";
                    out += ", ";
                    out += std::to_string(salt);
                    out += "ull";
                }
                out += ");\n";
            }
            प्रत्यागम out;
        } अण्णहा {
            string out = varNameProtected + suffix;
            जइ (valuep) {
                out += " = ";
                // TODO cleanup code shared between here, V3EmitCConstInit.h,
                // EmitCFunc::emitVarReset, EmitCFunc::emitConstant
                ध्रुव AstConst* ध्रुव constp = VN_AS(valuep, Const);
                UASSERT_OBJ(constp, varp, "non-const initializer for variable");
                out += cvtToStr(constp->अङ्कः().edataWord(0)) + "U;\n";
                out += ";\n";
            } अण्णहा जइ (zeroit) {
                out += " = 0;\n";
            } अण्णहा {
                emitVarResetScopeHash();
                ध्रुव uint64_t salt = VString::hashMurmur(varp->prettyName());
                out += " = VL_SCOPED_RAND_RESET_";
                जइ (varp->isXTemp()) out += "ASSIGN_";
                out += dtypep->charIQWN();
                out += "(" + cvtToStr(dtypep->widthMin()) + ", "
                       + (m_classOrPackage ? m_classOrPackageHash : "__VscopeHash") + ", "
                       + std::to_string(salt) + "ull);\n";
            }
            प्रत्यागम out;
        }
    } अण्णहा {  // LCOV_EXCL_BR_LINE
        v3fatalSrc("Unknown node type in reset generator: " << varp->prettyTypeName());
    }
    प्रत्यागम "";
}

शून्य EmitCFunc::emitVarResetScopeHash() {
    जइ (VL_LIKELY(m_createdScopeHash)) { प्रत्यागम; }
    जइ (m_classOrPackage) {
        m_classOrPackageHash
            = std::to_string(VString::hashMurmur(m_classOrPackage->name())) + "ULL";
    } अण्णहा {
        puts(string("const uint64_t __VscopeHash = VL_MURMUR64_HASH(")
             + (m_useSelfForThis ? "vlSelf" : "this") + "->vlNamep);\n");
    }
    m_createdScopeHash = सच्च;
}
