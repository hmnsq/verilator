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

तार्किक EmitCFunc::emitSimpleOk(Ast_गाँठो_अभिव्यक्ति* nodep) {
    // Can we put out a simple (A + B) instead of VL_ADD_III(A,B)?
    यदि (nodep->emitSimpleOperator() == "") फर्काऊ असत्य;
    यदि (nodep->isWide()) फर्काऊ असत्य;
    यदि (nodep->op1p() && nodep->op1p()->isWide()) फर्काऊ असत्य;
    यदि (nodep->op2p() && nodep->op2p()->isWide()) फर्काऊ असत्य;
    यदि (nodep->op3p() && nodep->op3p()->isWide()) फर्काऊ असत्य;
    फर्काऊ सत्य;
}

शून्य EmitCFunc::emitOpName(Ast_गाँठो* nodep, अचर string& format, Ast_गाँठो* lhsp, Ast_गाँठो* rhsp,
                           Ast_गाँठो* thsp) {
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

    तार्किक needComma = असत्य;
    तार्किक usesQueue = असत्य;
    string nextComma;
    स्वतः commaOut = [&out, &nextComma]() {
        यदि (!nextComma.empty()) {
            out += nextComma;
            nextComma = "";
        }
    };

    स्वतः putOut = [एतत्, &out]() {
        यदि (!out.empty()) माला_दो(out);
        out = "";
    };

    लागि (string::const_iterator pos = format.begin(); pos != format.end(); ++pos) {
        यदि (pos[0] == ',') {
            // Remember we need to add one, but don't do yet to avoid ",)"
            यदि (needComma) {
                यदि (pos[1] == ' ') {
                    nextComma = ", ";
                } अन्यथा {
                    nextComma = ",";
                }
                needComma = असत्य;
            }
            यदि (pos[1] == ' ') ++pos;  // Must do even if no nextComma
        } अन्यथा यदि (pos[0] == '%') {
            ++pos;
            तार्किक detail = असत्य;
            Ast_गाँठो* detailp = nullptr;
            विकल्प (pos[0]) {
            पक्ष '%': out += '%'; रोक;
            पक्ष 'k':
                putOut();
                putbs("");
                रोक;
            पक्ष 'n':
                detail = सत्य;
                detailp = nodep;
                रोक;
            पक्ष 'l':
                detail = सत्य;
                detailp = lhsp;
                रोक;
            पक्ष 'r':
                detail = सत्य;
                detailp = rhsp;
                रोक;
            पक्ष 't':
                detail = सत्य;
                detailp = thsp;
                रोक;
            पक्ष 'P':
                यदि (nodep->isWide() && !usesQueue) {
                    UASSERT_OBJ(m_wideTempRefp, nodep,
                                "Wide op " << nodep->prettyTypeName()
                                           << " w/ no temp, perhaps missing op in V3EmitC?");
                    commaOut();
                    putOut();
                    यदि (!m_wideTempRefp->selfPointer().isEmpty()) {
                        emitDereference(m_wideTempRefp,
                                        m_wideTempRefp->selfPointerProtect(m_useSelfForThis));
                    }
                    out += varName(m_wideTempRefp->varp());
                    m_wideTempRefp = nullptr;
                    needComma = सत्य;
                } अन्यथा यदि (usesQueue) {
                    commaOut();
                    putOut();
                    iterateAndNextConstNull(nodep->backp()->op2p());
                    needComma = सत्य;
                }

                रोक;
            स्वभाव: nodep->v3fatalSrc("Unknown emitOperator format code: %" << pos[0]); रोक;
            }
            यदि (detail) {
                // Get next letter of %[nlrt]
                ++pos;
                विकल्प (pos[0]) {
                पक्ष 'q':
                    putOut();
                    // If we are assigning this to a queue we need to get the return type
                    यदि (VN_IS(detailp->backp(), Assign)
                        && VN_IS(detailp->backp()->op2p()->dtypep()->skipRefp(), QueueDType)) {
                        माला_दो("R");  // R for queue
                        usesQueue = सत्य;
                    } अन्यथा यदि (VN_IS(detailp->dtypep()->skipRefp(), QueueDType)
                               || VN_IS(detailp->dtypep()->skipRefp(), StreamDType)) {
                        माला_दो("R");  // R for queue
                    } अन्यथा {
                        emitIQW(detailp);
                    }
                    रोक;
                पक्ष 'w':
                    commaOut();
                    out += cvtToStr(detailp->widthMin());
                    needComma = सत्य;
                    रोक;
                पक्ष 'W':
                    यदि (lhsp->isWide()) {
                        commaOut();
                        out += cvtToStr(lhsp->width_शब्दहरू());
                        needComma = सत्य;
                    } अन्यथा यदि (VN_IS(lhsp, StreamR)) {
                        commaOut();
                        out += cvtToStr(rhsp->width_शब्दहरू());
                        needComma = सत्य;
                    }
                    रोक;
                पक्ष 'i':
                    commaOut();
                    UASSERT_OBJ(detailp, nodep, "emitOperator() references undef node");
                    putOut();
                    iterateAndNextConstNull(detailp);
                    needComma = सत्य;
                    रोक;
                स्वभाव:
                    nodep->v3fatalSrc("Unknown emitOperator format code: %[nlrt]" << pos[0]);
                    रोक;
                }
            }
        } अन्यथा यदि (pos[0] == ')') {
            nextComma = "";
            out += ')';
        } अन्यथा यदि (pos[0] == '(') {
            commaOut();
            needComma = असत्य;
            out += '(';
        } अन्यथा {
            // Normal text
            यदि (std::है_अक्षर_अंक(pos[0])) needComma = सत्य;
            commaOut();
            out += pos[0];
        }
    }
    putOut();
}

तार्किक EmitCFunc::displayEmitHeader(Ast_गाँठो* nodep) {
    तार्किक isStmt = असत्य;
    यदि (अचर AstFScanF* अचर dispp = VN_CAST(nodep, FScanF)) {
        isStmt = असत्य;
        putns(nodep, "VL_FSCANF_INX(");
        iterateConst(dispp->filep());
        माला_दो(",");
    } अन्यथा यदि (अचर AstSScanF* अचर dispp = VN_CAST(nodep, SScanF)) {
        isStmt = असत्य;
        putns(nodep, "VL_SSCANF_I");
        emitIQW(dispp->fromp());
        माला_दो("NX(");
        माला_दो(cvtToStr(dispp->fromp()->widthMin()));
        माला_दो(",");
        iterateConst(dispp->fromp());
        माला_दो(",");
    } अन्यथा यदि (अचर AstDisplay* अचर dispp = VN_CAST(nodep, Display)) {
        isStmt = सत्य;
        यदि (dispp->filep()) {
            putns(nodep, "VL_FWRITEF_NX(");
            iterateConst(dispp->filep());
            माला_दो(",");
        } अन्यथा {
            putns(nodep, "VL_WRITEF_NX(");
        }
    } अन्यथा यदि (अचर AstSFormat* अचर dispp = VN_CAST(nodep, SFormat)) {
        isStmt = सत्य;
        माला_दो("VL_SFORMAT_NX(");
        यदि (!dispp->lhsp()->dtypep()->isString()) {
            माला_दो(cvtToStr(dispp->lhsp()->widthMin()));
            putbs(",");
        }
        iterateConst(dispp->lhsp());
        emitDatap(dispp->lhsp());
        putbs(",");
    } अन्यथा यदि (VN_IS(nodep, SFormatF)) {
        isStmt = असत्य;
        putns(nodep, "VL_SFORMATF_N_NX(");
    } अन्यथा {
        nodep->v3fatalSrc("Unknown displayEmit node type");
    }
    फर्काऊ isStmt;
}

शून्य EmitCFunc::display_गाँठो(Ast_गाँठो* nodep, AstSFormatF* fmtp,  // fmtp is nullptr for AstScan
                            अचर string& vformat, Ast_गाँठो* exprsp, तार्किक isScan) {
    // Check format, if it exists
    अचर तार्किक अभिव्यक्ति_Format_2 = fmtp && fmtp->अभिव्यक्ति_Format_2();
    तार्किक needsScope = अभिव्यक्ति_Format_2;
    तार्किक needsTimescale = अभिव्यक्ति_Format_2;
    पूर्णाङ्क formatArgs = 0;
    यदि (!अभिव्यक्ति_Format_2) {
        // Check display and argument count to head-off later runtime issues
        तार्किक inPct = असत्य;
        तार्किक ignore = असत्य;
        लागि (वर्ण ch : vformat) {
            // UINFO(1, "Parse '" << *pos << "'  IP" << inPct << " List " << cvtToHex(elistp));
            यदि (!inPct && ch == '%') {
                inPct = सत्य;
                ignore = असत्य;
            } अन्यथा यदि (inPct && (std::है_अंक(ch) || ch == '.' || ch == '-')) {
            } अन्यथा यदि (!inPct) {  // Normal text
            } अन्यथा {  // Format character
                inPct = असत्य;
                ch = std::छोटे(ch);
                विकल्प (ch) {
                पक्ष '%': रोक;
                पक्ष '*':
                    inPct = सत्य;  // Get more digits
                    ignore = सत्य;
                    रोक;
                पक्ष 'm': needsScope = सत्य; रोक;
                पक्ष 't':
                    needsTimescale = सत्य;
                    ++formatArgs;
                    रोक;
                स्वभाव:
                    यदि (!ignore && V3_सङ्ख्या::displayedFmtHasArg(ch, isScan)) ++formatArgs;
                    रोक;
                }
            }
        }
    }

    यदि (vformat.empty() && VN_IS(nodep, Display))  // not fscanf etc, as they need to return value
        फर्काऊ;  // NOP

    अचर तार्किक isStmt = displayEmitHeader(nodep);

    यदि (अभिव्यक्ति_Format_2) {
        UASSERT_OBJ(exprsp, nodep, "Missing format expression");
        iterateConst(exprsp);
        emitDatap(exprsp);
        exprsp = exprsp->nextp();
    } अन्यथा {
        ofp()->putsQuoted(vformat);
    }

    पूर्णाङ्क अभिव्यक्ति_Args = 0;
    लागि (Ast_गाँठो* argp = exprsp; argp; argp = argp->nextp()) ++अभिव्यक्ति_Args;
    यदि (VL_UNCOVERABLE(!अभिव्यक्ति_Format_2 && अभिव्यक्ति_Args != formatArgs)) {  // LCOV_EXCL_START
        // expectDisplay() checks this first, so probably internal error if found here
        nodep->v3error("Internal: Missing or extra arguments for $display-like format, "
                       << " expression had " << अभिव्यक्ति_Args << ", format had " << formatArgs);
    }  // LCOV_EXCL_STOP

    // argc for error check. Also MSVC++ requires va_args to not be off a reference
    पूर्णाङ्क argc = 0;
    यदि (needsScope) ++argc;
    यदि (needsTimescale) ++argc;
    लागि (Ast_गाँठो* argp = exprsp; argp; argp = argp->nextp()) {
        ++argc;
        अचर AstSFormatArg* अचर fargp = VN_CAST(argp, SFormatArg);
        यदि (fargp && fargp->formatAttr().isEnum()) ++argc;  // Additional name argument
    }
    ofp()->माला_दो("," + std::to_string(argc));

    यदि (needsScope) {
        AstScopeName* अचर scopenamep = fmtp ? fmtp->scopeNamep() : nullptr;
        UASSERT_OBJ(scopenamep, nodep, "Display with %m but no AstScopeName");
        ofp()->माला_दो(", '"s + VFormatAttr{VFormatAttr::SCOPE}.ascii() + "'");
        यदि (scopenamep->selfRelative()) {
            // vlNamep is catName(name(), instance), so starts with '.' when the model is unnamed
            अचर string namep = string{m_useSelfForThis ? "vlSelf" : "this"} + "->vlNamep";
            ofp()->माला_दो(",(vlSymsp->name()[0] ? " + namep + " : " + namep + " + 1),");
            ofp()->putsQuoted(scopenamep->scopeAttr());
        } अन्यथा {
            ofp()->माला_दो(",vlSymsp->name(),");
            ofp()->putsQuoted(scopenamep->scopePrettySymName());
        }
    }

    यदि (needsTimescale) {
        VTimescale timeunit = VTimescale::NONE;
        यदि (अचर AstDisplay* अचर anodep = VN_CAST(nodep, Display)) {
            timeunit = anodep->fmtp()->timeunit();
        } अन्यथा यदि (अचर AstSFormat* अचर anodep = VN_CAST(nodep, SFormat)) {
            timeunit = anodep->fmtp()->timeunit();
        } अन्यथा यदि (अचर AstSScanF* अचर anodep = VN_CAST(nodep, SScanF)) {
            timeunit = anodep->timeunit();
        } अन्यथा यदि (अचर AstSFormatF* अचर anodep = VN_CAST(nodep, SFormatF)) {
            timeunit = anodep->timeunit();
        }
        UASSERT_OBJ(!timeunit.isNone(), nodep,
                    "Use of %t must be under AstDisplay, AstSFormat, or AstSFormatF, or "
                    "SScanF, and timeunit set");
        ofp()->माला_दो(", '"s + VFormatAttr{VFormatAttr::TIMEUNIT}.ascii() + "'");
        ofp()->माला_दो(","s + cvtToStr((पूर्णाङ्क)timeunit.powerOfTen()));
    }

    // Arguments
    लागि (Ast_गाँठो* argp = exprsp; argp; argp = argp->nextp()) {
        ofp()->indentInc();
        ofp()->putbs("");
        AstSFormatArg* अचर fargp = VN_CAST(argp, SFormatArg);
        Ast_गाँठो* अचर subargp = fargp ? fargp->exprp() : argp;
        अचर VFormatAttr formatAttr = AstSFormatArg::formatAttrDefauled(fargp, subargp->dtypep());
        माला_दो(", '"s + formatAttr.ascii() + '\'');
        यदि (formatAttr.isSigned() || formatAttr.isUnsigned() || formatAttr.isEnum())
            माला_दो("," + cvtToStr(subargp->widthMin()));
        अचर तार्किक addrof = isScan || formatAttr.isString() || formatAttr.isComplex();
        अचर तार्किक wideEnum = formatAttr.isEnum() && subargp->isWide();
        माला_दो(",");
        यदि (wideEnum) माला_दो("static_cast<const EData*>(");
        यदि (addrof) माला_दो("&(");
        यदि (VN_IS(subargp, StreamR))
            emitStreamR(
                VN_CAST(subargp, StreamR),
                nodep);  // This has to be done here because streamR doesn't know what it returns
        अन्यथा { iterateConst(subargp); }
        यदि (addrof) माला_दो(")");
        यदि (!addrof) emitDatap(argp);
        यदि (wideEnum) माला_दो(")");
        यदि (formatAttr.isEnum()) {
            माला_दो(", '"s + VFormatAttr{VFormatAttr::STRING}.ascii() + "', &(");
            iterateConst(fargp->namep());
            माला_दो(")");
        }
        ofp()->indentDec();
    }

    // End
    यदि (isStmt) {
        माला_दो(");\n");
    } अन्यथा {
        माला_दो(") ");
    }
}

शून्य EmitCFunc::emitCCallArgs(अचर Ast_गाँठो_CCall* nodep, अचर string& selfPointer,
                              तार्किक inProcess) {
    putns(nodep, "(");
    तार्किक comma = असत्य;
    यदि (nodep->funcp()->isLoose() && !nodep->funcp()->isStatic()) {
        UASSERT_OBJ(!selfPointer.empty(), nodep, "Call to loose method without self pointer");
        माला_दो(selfPointer);
        comma = सत्य;
    }
    यदि (nodep->funcp()->needProcess()) {
        यदि (comma) माला_दो(", ");
        यदि (VN_IS(nodep->backp(), CAwait) || !nodep->funcp()->isCoroutine()) {
            माला_दो("vlProcess");
        } अन्यथा यदि (inProcess) {
            माला_दो("std::make_shared<VlProcess>(vlProcess)");
        } अन्यथा {
            माला_दो("std::make_shared<VlProcess>()");
        }
        comma = सत्य;
    }
    यदि (!nodep->argTypes().empty()) {
        यदि (comma) माला_दो(", ");
        माला_दो(nodep->argTypes());
        comma = सत्य;
    }
    putCommaIterateNext(nodep->argsp(), comma);
    माला_दो(")");
}

std::string EmitCFunc::dereferenceString(अचर std::string& pointer) अचर {
    यदि (pointer[0] == '(' && pointer[1] == '&') {
        // remove "address of" followed by immediate dereference
        // Note: this relies on only the form '(&OBJECT)' being used by Verilator
        फर्काऊ pointer.substr(2, pointer.length() - 3) + '.';
    } अन्यथा {
        यदि (pointer == "vlSelf" && m_usevlSelfRef) {
            फर्काऊ "vlSelfRef.";
        } अन्यथा {
            फर्काऊ pointer + "->";
        }
    }
}
शून्य EmitCFunc::emitDereference(Ast_गाँठो* nodep, अचर string& pointer) {
    putns(nodep, dereferenceString(pointer));
}

शून्य EmitCFunc::emitCvtPackStr(Ast_गाँठो* nodep) {
    यदि (अचर AstConst* अचर constp = VN_CAST(nodep, Const)) {
        emitConstantString(constp);
    } अन्यथा यदि (VN_IS(nodep->dtypep(), StreamDType)) {
        putnbs(nodep, "VL_CVT_PACK_STR_ND(");
        iterateAndNextConstNull(nodep);
        माला_दो(")");
    } अन्यथा {
        putnbs(nodep, "VL_CVT_PACK_STR_N");
        emitIQW(nodep);
        माला_दो("(");
        यदि (nodep->isWide()) {
            // Note argument width, not node width (which is always 32)
            माला_दो(cvtToStr(nodep->width_शब्दहरू()));
            माला_दो(", ");
        }
        iterateAndNextConstNull(nodep);
        माला_दो(")");
    }
}

शून्य EmitCFunc::emitConstant(AstConst* nodep) {
    // Put out constant set to the specified variable, or given variable in a string
    अचर V3_सङ्ख्या& अङ्क = nodep->अङ्क();
    यदि (अङ्क.isFourState()) {
        nodep->v3warn(E_UNSUPPORTED, "Unsupported: 4-state numbers in this context");
        फर्काऊ;
    }
    putns(nodep, अङ्क.emitC());
}

शून्य EmitCFunc::emitConstantString(अचर AstConst* nodep) {
    // Const might be a Verilog array-type string, but need to always output std::string
    putnbs(nodep, "std::string{");
    अचर string str = nodep->अङ्क().toString();
    यदि (!str.empty()) putsQuoted(str);
    माला_दो("}");
}

शून्य EmitCFunc::emitVarReset(अचर string& prefix, AstVar* varp, तार्किक constructing) {
    // 'constructing' indicates that the object was just constructed, so if it is a string or
    // something that starts off clear already, no need to clear it again
    Ast_गाँठो_DType* अचर dtypep = varp->dtypep()->skipRefp();
    अचर string vlSelf = VSelfPointerText::replaceThis(m_useSelfForThis, "this->");
    अचर string varNameProtected
        = ((VN_IS(m_modp, Class) || varp->isFuncLocal()) || !prefix.empty())
              ? varName(varp)
              : vlSelf + varName(varp);
    अचर string newPrefix = prefix + varNameProtected;
    यदि (varp->isIO() && m_modp->is_शीर्ष() && optSystemC()) {
        // System C top I/O doesn't need loading, as the lower level subinst code does it.}
    } अन्यथा यदि (varp->isParam()) {
        UASSERT_OBJ(varp->valuep(), varp, "No init for a param?");
        // If a simple CONST value we initialize it using an enum
        // If an ARRAYINIT we initialize it using an initial block similar to a signal
        // puts("// parameter "+varp->nameProtect()+" = "+varp->valuep()->name()+"\n");
    } अन्यथा {
        putns(varp,
              emitVarResetRecurse(varp, constructing, newPrefix, dtypep, 0, "", varp->valuep()));
    }
}

string EmitCFunc::emitVarResetRecurse(अचर AstVar* varp, तार्किक constructing,
                                      अचर string& varNameProtected, Ast_गाँठो_DType* dtypep,
                                      पूर्णाङ्क depth, अचर string& suffix, अचर Ast_गाँठो* valuep) {
    dtypep = dtypep->skipRefp();
    AstBasicDType* अचर basicp = dtypep->basicp();
    // Returns string to do resetting, empty to do nothing (which caller should handle)
    यदि (AstAssocArrayDType* अचर adtypep = VN_CAST(dtypep, AssocArrayDType)) {
        // Access std::array as C array
        अचर string pre = constructing ? "" : varNameProtected + suffix + ".clear();\n";
        फर्काऊ pre
               + emitVarResetRecurse(varp, constructing, varNameProtected, adtypep->subDTypep(),
                                     depth + 1, suffix + ".atDefault()", nullptr);
    } अन्यथा यदि (AstWildcardArrayDType* अचर adtypep = VN_CAST(dtypep, WildcardArrayDType)) {
        // Access std::array as C array
        अचर string pre = constructing ? "" : varNameProtected + suffix + ".clear();\n";
        फर्काऊ pre
               + emitVarResetRecurse(varp, constructing, varNameProtected, adtypep->subDTypep(),
                                     depth + 1, suffix + ".atDefault()", nullptr);
    } अन्यथा यदि (VN_IS(dtypep, CDType)) {
        फर्काऊ "";  // Constructor does it
    } अन्यथा यदि (VN_IS(dtypep, CoverCrossDType) || VN_IS(dtypep, CoverpointDType)) {
        फर्काऊ "";  // Covergroup constructor creates the runtime and assigns the pointer
    } अन्यथा यदि (अचर AstClassRefDType* अचर adtypep = VN_CAST(dtypep, ClassRefDType)) {
        फर्काऊ adtypep->rawPointer() ? varNameProtected + suffix + " = nullptr;\n" : "";
    } अन्यथा यदि (VN_IS(dtypep, IfaceRefDType)) {
        फर्काऊ varNameProtected + suffix + " = nullptr;\n";
    } अन्यथा यदि (अचर AstDynArrayDType* अचर adtypep = VN_CAST(dtypep, DynArrayDType)) {
        अचर string pre = constructing ? "" : varNameProtected + suffix + ".clear();\n";
        फर्काऊ pre
               + emitVarResetRecurse(varp, constructing, varNameProtected, adtypep->subDTypep(),
                                     depth + 1, suffix + ".atDefault()", nullptr);
    } अन्यथा यदि (अचर AstQueueDType* अचर adtypep = VN_CAST(dtypep, QueueDType)) {
        अचर string pre = constructing ? "" : varNameProtected + suffix + ".clear();\n";
        फर्काऊ pre
               + emitVarResetRecurse(varp, constructing, varNameProtected, adtypep->subDTypep(),
                                     depth + 1, suffix + ".atDefault()", nullptr);
    } अन्यथा यदि (VN_IS(dtypep, SampleQueueDType)) {
        फर्काऊ "";
    } अन्यथा यदि (अचर AstUnpackArrayDType* अचर adtypep = VN_CAST(dtypep, UnpackArrayDType)) {
        UASSERT_OBJ(adtypep->hi() >= adtypep->lo(), varp,
                    "Should have swapped msb & lsb earlier.");
        अचर Ast_गाँठो_DType* अचर elementp = adtypep->subDTypep()->skipRefp();
        यदि (!varp->isFuncLocal() && elementp->isIntegralOrPacked()) {
            // Scoped reset has no array-index input or mutable RNG state. Initialize one
            // element, then fill in the optimized runtime even when this code is cold.
            अचर string first
                = emitVarResetRecurse(varp, constructing, varNameProtected, adtypep->subDTypep(),
                                      depth + 1, suffix + "[0]", nullptr);
            अचर string array = varNameProtected + suffix;
            फर्काऊ first + "VL_UNPACKED_FILL(" + array + ".data(), sizeof(" + array + "[0]), "
                   + cvtToStr(adtypep->elementsConst()) + ");\n";
        }
        अचर string ivar = "__Vi"s + cvtToStr(depth);
        अचर string pre = ("for (int " + ivar + " = " + cvtToStr(0) + "; " + ivar + " < "
                            + cvtToStr(adtypep->elementsConst()) + "; ++" + ivar + ") {\n");
        अचर string below
            = emitVarResetRecurse(varp, constructing, varNameProtected, adtypep->subDTypep(),
                                  depth + 1, suffix + "[" + ivar + "]", nullptr);
        अचर string post = "}\n";
        फर्काऊ below.empty() ? "" : pre + below + post;
    } अन्यथा यदि (VN_IS(dtypep, गाँठो_UOr_Struct_DType) && !VN_AS(dtypep, गाँठो_UOr_Struct_DType)->packed()) {
        अचर Ast_गाँठो_UOr_Struct_DType* अचर sdtypep = VN_AS(dtypep, गाँठो_UOr_Struct_DType);
        string literal;
        लागि (अचर AstMemberDType* itemp = sdtypep->membersp(); itemp;
             itemp = VN_AS(itemp->nextp(), MemberDType)) {
            अचर std::string पङ्क्ति = emitVarResetRecurse(
                varp, constructing, varNameProtected + suffix + "." + itemp->nameProtect(),
                itemp->dtypep(), depth + 1, "", itemp->valuep());
            यदि (!पङ्क्ति.empty()) literal += पङ्क्ति;
        }
        फर्काऊ literal;
    } अन्यथा यदि (basicp && basicp->keyword() == VBasicDTypeKwd::STRING) {
        यदि (constructing) फर्काऊ "";  // String's constructor deals with it
        फर्काऊ varNameProtected + suffix + ".clear();\n";
    } अन्यथा यदि (basicp && basicp->isForkSync()) {
        फर्काऊ "";
    } अन्यथा यदि (basicp && basicp->isProcessRef()) {
        फर्काऊ "";
    } अन्यथा यदि (basicp && basicp->isDelayScheduler()) {
        फर्काऊ "";
    } अन्यथा यदि (basicp && basicp->isTriggerScheduler()) {
        फर्काऊ "";
    } अन्यथा यदि (basicp && basicp->isDynamicTriggerScheduler()) {
        फर्काऊ "";
    } अन्यथा यदि (basicp && (basicp->isRandomGenerator() || basicp->isStdRandomGenerator())) {
        फर्काऊ "";
    } अन्यथा यदि (basicp && basicp->isCovergroupInstHandle()) {
        // The handle's own constructor deals with it.
        फर्काऊ "";
    } अन्यथा यदि (basicp && (basicp->isEvent())) {
        फर्काऊ "VlAssignableEvent{};\n";
    } अन्यथा यदि (basicp) {
        अचर तार्किक zeroit
            = (varp->attrFileDescr()  // Zero so we don't do file IO if never $fopen
               || varp->isFuncLocal()  // Randomization too slow
               || (basicp && basicp->isZeroInit())
               || (v3Global.opt.underlineZero() && !varp->name().empty() && varp->name()[0] == '_')
               || (varp->varType().isTemp() && !varp->isXTemp())
               || (varp->isXTemp()
                       ? (v3Global.opt.xAssign() != "unique")
                       : (v3Global.opt.xInitial() == "fast" || v3Global.opt.xInitial() == "0")));
        अचर तार्किक slow = !varp->isFuncLocal() && !varp->isClassMember();
        splitSizeInc(1);
        यदि (dtypep->isWide()) {  // Handle unpacked; not basicp->isWide
            string out;
            यदि (valuep) {
                अचर AstConst* अचर constp = VN_AS(valuep, Const);
                UASSERT_OBJ(constp, varp, "non-const initializer for variable");
                लागि (पूर्णाङ्क w = 0; w < dtypep->width_शब्दहरू(); ++w) {
                    out += varNameProtected + suffix + "[" + cvtToStr(w) + "] = ";
                    out += cvtToStr(constp->अङ्क().edataWord(w)) + "U;\n";
                }
            } अन्यथा {
                out += zeroit ? (slow ? "VL_ZERO_RESET_W(" : "VL_ZERO_W(")
                              : (varp->isXTemp() ? "VL_SCOPED_RAND_RESET_ASSIGN_W("
                                                 : "VL_SCOPED_RAND_RESET_W(");
                out += cvtToStr(dtypep->widthMin());
                out += ", " + varNameProtected + suffix;
                यदि (!zeroit) {
                    emitVarResetScopeHash();
                    अचर uint64_t salt = VString::hashMurmur(varp->prettyName());
                    out += ", ";
                    out += m_classOrPackage ? m_classOrPackageHash : "__VscopeHash";
                    out += ", ";
                    out += std::to_string(salt);
                    out += "ull";
                }
                out += ");\n";
            }
            फर्काऊ out;
        } अन्यथा {
            string out = varNameProtected + suffix;
            यदि (valuep) {
                out += " = ";
                // TODO cleanup code shared between here, V3EmitCConstInit.h,
                // EmitCFunc::emitVarReset, EmitCFunc::emitConstant
                अचर AstConst* अचर constp = VN_AS(valuep, Const);
                UASSERT_OBJ(constp, varp, "non-const initializer for variable");
                out += cvtToStr(constp->अङ्क().edataWord(0)) + "U;\n";
                out += ";\n";
            } अन्यथा यदि (zeroit) {
                out += " = 0;\n";
            } अन्यथा {
                emitVarResetScopeHash();
                अचर uint64_t salt = VString::hashMurmur(varp->prettyName());
                out += " = VL_SCOPED_RAND_RESET_";
                यदि (varp->isXTemp()) out += "ASSIGN_";
                out += dtypep->charIQWN();
                out += "(" + cvtToStr(dtypep->widthMin()) + ", "
                       + (m_classOrPackage ? m_classOrPackageHash : "__VscopeHash") + ", "
                       + std::to_string(salt) + "ull);\n";
            }
            फर्काऊ out;
        }
    } अन्यथा {  // LCOV_EXCL_BR_LINE
        v3fatalSrc("Unknown node type in reset generator: " << varp->prettyTypeName());
    }
    फर्काऊ "";
}

शून्य EmitCFunc::emitVarResetScopeHash() {
    यदि (VL_LIKELY(m_createdScopeHash)) { फर्काऊ; }
    यदि (m_classOrPackage) {
        m_classOrPackageHash
            = std::to_string(VString::hashMurmur(m_classOrPackage->name())) + "ULL";
    } अन्यथा {
        माला_दो(string("const uint64_t __VscopeHash = VL_MURMUR64_HASH(")
             + (m_useSelfForThis ? "vlSelf" : "this") + "->vlNamep);\n");
    }
    m_createdScopeHash = सत्य;
}
