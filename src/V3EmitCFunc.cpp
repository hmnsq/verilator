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

तार्किक EmitCFunc::emitSimpleOk(Ast_गाठ_अभिव्यक्ती* nodep) {
    // Can we put out a simple (A + B) instead of VL_ADD_III(A,B)?
    जर (nodep->emitSimpleOperator() == "") परत असत्य;
    जर (nodep->isWide()) परत असत्य;
    जर (nodep->op1p() && nodep->op1p()->isWide()) परत असत्य;
    जर (nodep->op2p() && nodep->op2p()->isWide()) परत असत्य;
    जर (nodep->op3p() && nodep->op3p()->isWide()) परत असत्य;
    परत सत्य;
}

शून्य EmitCFunc::emitOpName(Ast_गाठ* nodep, स्थिरांक string& format, Ast_गाठ* lhsp, Ast_गाठ* rhsp,
                           Ast_गाठ* thsp) {
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
        जर (!nextComma.empty()) {
            out += nextComma;
            nextComma = "";
        }
    };

    स्वतः putOut = [हे, &out]() {
        जर (!out.empty()) माला_दो(out);
        out = "";
    };

    साठी (string::const_iterator pos = format.begin(); pos != format.end(); ++pos) {
        जर (pos[0] == ',') {
            // Remember we need to add one, but don't do yet to avoid ",)"
            जर (needComma) {
                जर (pos[1] == ' ') {
                    nextComma = ", ";
                } अन्यथा {
                    nextComma = ",";
                }
                needComma = असत्य;
            }
            जर (pos[1] == ' ') ++pos;  // Must do even if no nextComma
        } अन्यथा जर (pos[0] == '%') {
            ++pos;
            तार्किक detail = असत्य;
            Ast_गाठ* detailp = nullptr;
            निवड (pos[0]) {
            स्थिती '%': out += '%'; थांबा;
            स्थिती 'k':
                putOut();
                putbs("");
                थांबा;
            स्थिती 'n':
                detail = सत्य;
                detailp = nodep;
                थांबा;
            स्थिती 'l':
                detail = सत्य;
                detailp = lhsp;
                थांबा;
            स्थिती 'r':
                detail = सत्य;
                detailp = rhsp;
                थांबा;
            स्थिती 't':
                detail = सत्य;
                detailp = thsp;
                थांबा;
            स्थिती 'P':
                जर (nodep->isWide() && !usesQueue) {
                    UASSERT_OBJ(m_wideTempRefp, nodep,
                                "Wide op " << nodep->prettyTypeName()
                                           << " w/ no temp, perhaps missing op in V3EmitC?");
                    commaOut();
                    putOut();
                    जर (!m_wideTempRefp->selfPointer().isEmpty()) {
                        emitDereference(m_wideTempRefp,
                                        m_wideTempRefp->selfPointerProtect(m_useSelfForThis));
                    }
                    out += varName(m_wideTempRefp->varp());
                    m_wideTempRefp = nullptr;
                    needComma = सत्य;
                } अन्यथा जर (usesQueue) {
                    commaOut();
                    putOut();
                    iterateAndNextConstNull(nodep->backp()->op2p());
                    needComma = सत्य;
                }

                थांबा;
            पूर्वनिर्धारित: nodep->v3fatalSrc("Unknown emitOperator format code: %" << pos[0]); थांबा;
            }
            जर (detail) {
                // Get next letter of %[nlrt]
                ++pos;
                निवड (pos[0]) {
                स्थिती 'q':
                    putOut();
                    // If we are assigning this to a queue we need to get the return type
                    जर (VN_IS(detailp->backp(), Assign)
                        && VN_IS(detailp->backp()->op2p()->dtypep()->skipRefp(), QueueDType)) {
                        माला_दो("R");  // R for queue
                        usesQueue = सत्य;
                    } अन्यथा जर (VN_IS(detailp->dtypep()->skipRefp(), QueueDType)
                               || VN_IS(detailp->dtypep()->skipRefp(), StreamDType)) {
                        माला_दो("R");  // R for queue
                    } अन्यथा {
                        emitIQW(detailp);
                    }
                    थांबा;
                स्थिती 'w':
                    commaOut();
                    out += cvtToStr(detailp->widthMin());
                    needComma = सत्य;
                    थांबा;
                स्थिती 'W':
                    जर (lhsp->isWide()) {
                        commaOut();
                        out += cvtToStr(lhsp->width_शब्द());
                        needComma = सत्य;
                    } अन्यथा जर (VN_IS(lhsp, StreamR)) {
                        commaOut();
                        out += cvtToStr(rhsp->width_शब्द());
                        needComma = सत्य;
                    }
                    थांबा;
                स्थिती 'i':
                    commaOut();
                    UASSERT_OBJ(detailp, nodep, "emitOperator() references undef node");
                    putOut();
                    iterateAndNextConstNull(detailp);
                    needComma = सत्य;
                    थांबा;
                पूर्वनिर्धारित:
                    nodep->v3fatalSrc("Unknown emitOperator format code: %[nlrt]" << pos[0]);
                    थांबा;
                }
            }
        } अन्यथा जर (pos[0] == ')') {
            nextComma = "";
            out += ')';
        } अन्यथा जर (pos[0] == '(') {
            commaOut();
            needComma = असत्य;
            out += '(';
        } अन्यथा {
            // Normal text
            जर (std::है_अक्षर_अंक(pos[0])) needComma = सत्य;
            commaOut();
            out += pos[0];
        }
    }
    putOut();
}

तार्किक EmitCFunc::displayEmitHeader(Ast_गाठ* nodep) {
    तार्किक isStmt = असत्य;
    जर (स्थिरांक AstFScanF* स्थिरांक dispp = VN_CAST(nodep, FScanF)) {
        isStmt = असत्य;
        putns(nodep, "VL_FSCANF_INX(");
        iterateConst(dispp->filep());
        माला_दो(",");
    } अन्यथा जर (स्थिरांक AstSScanF* स्थिरांक dispp = VN_CAST(nodep, SScanF)) {
        isStmt = असत्य;
        putns(nodep, "VL_SSCANF_I");
        emitIQW(dispp->fromp());
        माला_दो("NX(");
        माला_दो(cvtToStr(dispp->fromp()->widthMin()));
        माला_दो(",");
        iterateConst(dispp->fromp());
        माला_दो(",");
    } अन्यथा जर (स्थिरांक AstDisplay* स्थिरांक dispp = VN_CAST(nodep, Display)) {
        isStmt = सत्य;
        जर (dispp->filep()) {
            putns(nodep, "VL_FWRITEF_NX(");
            iterateConst(dispp->filep());
            माला_दो(",");
        } अन्यथा {
            putns(nodep, "VL_WRITEF_NX(");
        }
    } अन्यथा जर (स्थिरांक AstSFormat* स्थिरांक dispp = VN_CAST(nodep, SFormat)) {
        isStmt = सत्य;
        माला_दो("VL_SFORMAT_NX(");
        जर (!dispp->lhsp()->dtypep()->isString()) {
            माला_दो(cvtToStr(dispp->lhsp()->widthMin()));
            putbs(",");
        }
        iterateConst(dispp->lhsp());
        emitDatap(dispp->lhsp());
        putbs(",");
    } अन्यथा जर (VN_IS(nodep, SFormatF)) {
        isStmt = असत्य;
        putns(nodep, "VL_SFORMATF_N_NX(");
    } अन्यथा {
        nodep->v3fatalSrc("Unknown displayEmit node type");
    }
    परत isStmt;
}

शून्य EmitCFunc::display_गाठ(Ast_गाठ* nodep, AstSFormatF* fmtp,  // fmtp is nullptr for AstScan
                            स्थिरांक string& vformat, Ast_गाठ* exprsp, तार्किक isScan) {
    // Check format, if it exists
    स्थिरांक तार्किक अभिव्यक्ती_Format_2 = fmtp && fmtp->अभिव्यक्ती_Format_2();
    तार्किक needsScope = अभिव्यक्ती_Format_2;
    तार्किक needsTimescale = अभिव्यक्ती_Format_2;
    पूर्णांक formatArgs = 0;
    जर (!अभिव्यक्ती_Format_2) {
        // Check display and argument count to head-off later runtime issues
        तार्किक inPct = असत्य;
        तार्किक ignore = असत्य;
        साठी (अक्षर ch : vformat) {
            // UINFO(1, "Parse '" << *pos << "'  IP" << inPct << " List " << cvtToHex(elistp));
            जर (!inPct && ch == '%') {
                inPct = सत्य;
                ignore = असत्य;
            } अन्यथा जर (inPct && (std::है_अंक(ch) || ch == '.' || ch == '-')) {
            } अन्यथा जर (!inPct) {  // Normal text
            } अन्यथा {  // Format character
                inPct = असत्य;
                ch = std::छोटे(ch);
                निवड (ch) {
                स्थिती '%': थांबा;
                स्थिती '*':
                    inPct = सत्य;  // Get more digits
                    ignore = सत्य;
                    थांबा;
                स्थिती 'm': needsScope = सत्य; थांबा;
                स्थिती 't':
                    needsTimescale = सत्य;
                    ++formatArgs;
                    थांबा;
                पूर्वनिर्धारित:
                    जर (!ignore && V3_संख्या::displayedFmtHasArg(ch, isScan)) ++formatArgs;
                    थांबा;
                }
            }
        }
    }

    जर (vformat.empty() && VN_IS(nodep, Display))  // not fscanf etc, as they need to return value
        परत;  // NOP

    स्थिरांक तार्किक isStmt = displayEmitHeader(nodep);

    जर (अभिव्यक्ती_Format_2) {
        UASSERT_OBJ(exprsp, nodep, "Missing format expression");
        iterateConst(exprsp);
        emitDatap(exprsp);
        exprsp = exprsp->nextp();
    } अन्यथा {
        ofp()->putsQuoted(vformat);
    }

    पूर्णांक अभिव्यक्ती_Args = 0;
    साठी (Ast_गाठ* argp = exprsp; argp; argp = argp->nextp()) ++अभिव्यक्ती_Args;
    जर (VL_UNCOVERABLE(!अभिव्यक्ती_Format_2 && अभिव्यक्ती_Args != formatArgs)) {  // LCOV_EXCL_START
        // expectDisplay() checks this first, so probably internal error if found here
        nodep->v3error("Internal: Missing or extra arguments for $display-like format, "
                       << " expression had " << अभिव्यक्ती_Args << ", format had " << formatArgs);
    }  // LCOV_EXCL_STOP

    // argc for error check. Also MSVC++ requires va_args to not be off a reference
    पूर्णांक argc = 0;
    जर (needsScope) ++argc;
    जर (needsTimescale) ++argc;
    साठी (Ast_गाठ* argp = exprsp; argp; argp = argp->nextp()) {
        ++argc;
        स्थिरांक AstSFormatArg* स्थिरांक fargp = VN_CAST(argp, SFormatArg);
        जर (fargp && fargp->formatAttr().isEnum()) ++argc;  // Additional name argument
    }
    ofp()->माला_दो("," + std::to_string(argc));

    जर (needsScope) {
        AstScopeName* स्थिरांक scopenamep = fmtp ? fmtp->scopeNamep() : nullptr;
        UASSERT_OBJ(scopenamep, nodep, "Display with %m but no AstScopeName");
        ofp()->माला_दो(", '"s + VFormatAttr{VFormatAttr::SCOPE}.ascii() + "'");
        जर (scopenamep->selfRelative()) {
            // vlNamep is catName(name(), instance), so starts with '.' when the model is unnamed
            स्थिरांक string namep = string{m_useSelfForThis ? "vlSelf" : "this"} + "->vlNamep";
            ofp()->माला_दो(",(vlSymsp->name()[0] ? " + namep + " : " + namep + " + 1),");
            ofp()->putsQuoted(scopenamep->scopeAttr());
        } अन्यथा {
            ofp()->माला_दो(",vlSymsp->name(),");
            ofp()->putsQuoted(scopenamep->scopePrettySymName());
        }
    }

    जर (needsTimescale) {
        VTimescale timeunit = VTimescale::NONE;
        जर (स्थिरांक AstDisplay* स्थिरांक anodep = VN_CAST(nodep, Display)) {
            timeunit = anodep->fmtp()->timeunit();
        } अन्यथा जर (स्थिरांक AstSFormat* स्थिरांक anodep = VN_CAST(nodep, SFormat)) {
            timeunit = anodep->fmtp()->timeunit();
        } अन्यथा जर (स्थिरांक AstSScanF* स्थिरांक anodep = VN_CAST(nodep, SScanF)) {
            timeunit = anodep->timeunit();
        } अन्यथा जर (स्थिरांक AstSFormatF* स्थिरांक anodep = VN_CAST(nodep, SFormatF)) {
            timeunit = anodep->timeunit();
        }
        UASSERT_OBJ(!timeunit.isNone(), nodep,
                    "Use of %t must be under AstDisplay, AstSFormat, or AstSFormatF, or "
                    "SScanF, and timeunit set");
        ofp()->माला_दो(", '"s + VFormatAttr{VFormatAttr::TIMEUNIT}.ascii() + "'");
        ofp()->माला_दो(","s + cvtToStr((पूर्णांक)timeunit.powerOfTen()));
    }

    // Arguments
    साठी (Ast_गाठ* argp = exprsp; argp; argp = argp->nextp()) {
        ofp()->indentInc();
        ofp()->putbs("");
        AstSFormatArg* स्थिरांक fargp = VN_CAST(argp, SFormatArg);
        Ast_गाठ* स्थिरांक subargp = fargp ? fargp->exprp() : argp;
        स्थिरांक VFormatAttr formatAttr = AstSFormatArg::formatAttrDefauled(fargp, subargp->dtypep());
        माला_दो(", '"s + formatAttr.ascii() + '\'');
        जर (formatAttr.isSigned() || formatAttr.isUnsigned() || formatAttr.isEnum())
            माला_दो("," + cvtToStr(subargp->widthMin()));
        स्थिरांक तार्किक addrof = isScan || formatAttr.isString() || formatAttr.isComplex();
        स्थिरांक तार्किक wideEnum = formatAttr.isEnum() && subargp->isWide();
        माला_दो(",");
        जर (wideEnum) माला_दो("static_cast<const EData*>(");
        जर (addrof) माला_दो("&(");
        जर (VN_IS(subargp, StreamR))
            emitStreamR(
                VN_CAST(subargp, StreamR),
                nodep);  // This has to be done here because streamR doesn't know what it returns
        अन्यथा { iterateConst(subargp); }
        जर (addrof) माला_दो(")");
        जर (!addrof) emitDatap(argp);
        जर (wideEnum) माला_दो(")");
        जर (formatAttr.isEnum()) {
            माला_दो(", '"s + VFormatAttr{VFormatAttr::STRING}.ascii() + "', &(");
            iterateConst(fargp->namep());
            माला_दो(")");
        }
        ofp()->indentDec();
    }

    // End
    जर (isStmt) {
        माला_दो(");\n");
    } अन्यथा {
        माला_दो(") ");
    }
}

शून्य EmitCFunc::emitCCallArgs(स्थिरांक Ast_गाठ_CCall* nodep, स्थिरांक string& selfPointer,
                              तार्किक inProcess) {
    putns(nodep, "(");
    तार्किक comma = असत्य;
    जर (nodep->funcp()->isLoose() && !nodep->funcp()->isStatic()) {
        UASSERT_OBJ(!selfPointer.empty(), nodep, "Call to loose method without self pointer");
        माला_दो(selfPointer);
        comma = सत्य;
    }
    जर (nodep->funcp()->needProcess()) {
        जर (comma) माला_दो(", ");
        जर (VN_IS(nodep->backp(), CAwait) || !nodep->funcp()->isCoroutine()) {
            माला_दो("vlProcess");
        } अन्यथा जर (inProcess) {
            माला_दो("std::make_shared<VlProcess>(vlProcess)");
        } अन्यथा {
            माला_दो("std::make_shared<VlProcess>()");
        }
        comma = सत्य;
    }
    जर (!nodep->argTypes().empty()) {
        जर (comma) माला_दो(", ");
        माला_दो(nodep->argTypes());
        comma = सत्य;
    }
    putCommaIterateNext(nodep->argsp(), comma);
    माला_दो(")");
}

std::string EmitCFunc::dereferenceString(स्थिरांक std::string& pointer) स्थिरांक {
    जर (pointer[0] == '(' && pointer[1] == '&') {
        // remove "address of" followed by immediate dereference
        // Note: this relies on only the form '(&OBJECT)' being used by Verilator
        परत pointer.substr(2, pointer.length() - 3) + '.';
    } अन्यथा {
        जर (pointer == "vlSelf" && m_usevlSelfRef) {
            परत "vlSelfRef.";
        } अन्यथा {
            परत pointer + "->";
        }
    }
}
शून्य EmitCFunc::emitDereference(Ast_गाठ* nodep, स्थिरांक string& pointer) {
    putns(nodep, dereferenceString(pointer));
}

शून्य EmitCFunc::emitCvtPackStr(Ast_गाठ* nodep) {
    जर (स्थिरांक AstConst* स्थिरांक constp = VN_CAST(nodep, Const)) {
        emitConstantString(constp);
    } अन्यथा जर (VN_IS(nodep->dtypep(), StreamDType)) {
        putnbs(nodep, "VL_CVT_PACK_STR_ND(");
        iterateAndNextConstNull(nodep);
        माला_दो(")");
    } अन्यथा {
        putnbs(nodep, "VL_CVT_PACK_STR_N");
        emitIQW(nodep);
        माला_दो("(");
        जर (nodep->isWide()) {
            // Note argument width, not node width (which is always 32)
            माला_दो(cvtToStr(nodep->width_शब्द()));
            माला_दो(", ");
        }
        iterateAndNextConstNull(nodep);
        माला_दो(")");
    }
}

शून्य EmitCFunc::emitConstant(AstConst* nodep) {
    // Put out constant set to the specified variable, or given variable in a string
    स्थिरांक V3_संख्या& अंक = nodep->अंक();
    जर (अंक.isFourState()) {
        nodep->v3warn(E_UNSUPPORTED, "Unsupported: 4-state numbers in this context");
        परत;
    }
    putns(nodep, अंक.emitC());
}

शून्य EmitCFunc::emitConstantString(स्थिरांक AstConst* nodep) {
    // Const might be a Verilog array-type string, but need to always output std::string
    putnbs(nodep, "std::string{");
    स्थिरांक string str = nodep->अंक().toString();
    जर (!str.empty()) putsQuoted(str);
    माला_दो("}");
}

शून्य EmitCFunc::emitVarReset(स्थिरांक string& prefix, AstVar* varp, तार्किक constructing) {
    // 'constructing' indicates that the object was just constructed, so if it is a string or
    // something that starts off clear already, no need to clear it again
    Ast_गाठ_DType* स्थिरांक dtypep = varp->dtypep()->skipRefp();
    स्थिरांक string vlSelf = VSelfPointerText::replaceThis(m_useSelfForThis, "this->");
    स्थिरांक string varNameProtected
        = ((VN_IS(m_modp, Class) || varp->isFuncLocal()) || !prefix.empty())
              ? varName(varp)
              : vlSelf + varName(varp);
    स्थिरांक string newPrefix = prefix + varNameProtected;
    जर (varp->isIO() && m_modp->is_शीर्ष() && optSystemC()) {
        // System C top I/O doesn't need loading, as the lower level subinst code does it.}
    } अन्यथा जर (varp->isParam()) {
        UASSERT_OBJ(varp->valuep(), varp, "No init for a param?");
        // If a simple CONST value we initialize it using an enum
        // If an ARRAYINIT we initialize it using an initial block similar to a signal
        // puts("// parameter "+varp->nameProtect()+" = "+varp->valuep()->name()+"\n");
    } अन्यथा {
        putns(varp,
              emitVarResetRecurse(varp, constructing, newPrefix, dtypep, 0, "", varp->valuep()));
    }
}

string EmitCFunc::emitVarResetRecurse(स्थिरांक AstVar* varp, तार्किक constructing,
                                      स्थिरांक string& varNameProtected, Ast_गाठ_DType* dtypep,
                                      पूर्णांक depth, स्थिरांक string& suffix, स्थिरांक Ast_गाठ* valuep) {
    dtypep = dtypep->skipRefp();
    AstBasicDType* स्थिरांक basicp = dtypep->basicp();
    // Returns string to do resetting, empty to do nothing (which caller should handle)
    जर (AstAssocArrayDType* स्थिरांक adtypep = VN_CAST(dtypep, AssocArrayDType)) {
        // Access std::array as C array
        स्थिरांक string pre = constructing ? "" : varNameProtected + suffix + ".clear();\n";
        परत pre
               + emitVarResetRecurse(varp, constructing, varNameProtected, adtypep->subDTypep(),
                                     depth + 1, suffix + ".atDefault()", nullptr);
    } अन्यथा जर (AstWildcardArrayDType* स्थिरांक adtypep = VN_CAST(dtypep, WildcardArrayDType)) {
        // Access std::array as C array
        स्थिरांक string pre = constructing ? "" : varNameProtected + suffix + ".clear();\n";
        परत pre
               + emitVarResetRecurse(varp, constructing, varNameProtected, adtypep->subDTypep(),
                                     depth + 1, suffix + ".atDefault()", nullptr);
    } अन्यथा जर (VN_IS(dtypep, CDType)) {
        परत "";  // Constructor does it
    } अन्यथा जर (VN_IS(dtypep, CoverCrossDType) || VN_IS(dtypep, CoverpointDType)) {
        परत "";  // Covergroup constructor creates the runtime and assigns the pointer
    } अन्यथा जर (स्थिरांक AstClassRefDType* स्थिरांक adtypep = VN_CAST(dtypep, ClassRefDType)) {
        परत adtypep->rawPointer() ? varNameProtected + suffix + " = nullptr;\n" : "";
    } अन्यथा जर (VN_IS(dtypep, IfaceRefDType)) {
        परत varNameProtected + suffix + " = nullptr;\n";
    } अन्यथा जर (स्थिरांक AstDynArrayDType* स्थिरांक adtypep = VN_CAST(dtypep, DynArrayDType)) {
        स्थिरांक string pre = constructing ? "" : varNameProtected + suffix + ".clear();\n";
        परत pre
               + emitVarResetRecurse(varp, constructing, varNameProtected, adtypep->subDTypep(),
                                     depth + 1, suffix + ".atDefault()", nullptr);
    } अन्यथा जर (स्थिरांक AstQueueDType* स्थिरांक adtypep = VN_CAST(dtypep, QueueDType)) {
        स्थिरांक string pre = constructing ? "" : varNameProtected + suffix + ".clear();\n";
        परत pre
               + emitVarResetRecurse(varp, constructing, varNameProtected, adtypep->subDTypep(),
                                     depth + 1, suffix + ".atDefault()", nullptr);
    } अन्यथा जर (VN_IS(dtypep, SampleQueueDType)) {
        परत "";
    } अन्यथा जर (स्थिरांक AstUnpackArrayDType* स्थिरांक adtypep = VN_CAST(dtypep, UnpackArrayDType)) {
        UASSERT_OBJ(adtypep->hi() >= adtypep->lo(), varp,
                    "Should have swapped msb & lsb earlier.");
        स्थिरांक Ast_गाठ_DType* स्थिरांक elementp = adtypep->subDTypep()->skipRefp();
        जर (!varp->isFuncLocal() && elementp->isIntegralOrPacked()) {
            // Scoped reset has no array-index input or mutable RNG state. Initialize one
            // element, then fill in the optimized runtime even when this code is cold.
            स्थिरांक string first
                = emitVarResetRecurse(varp, constructing, varNameProtected, adtypep->subDTypep(),
                                      depth + 1, suffix + "[0]", nullptr);
            स्थिरांक string array = varNameProtected + suffix;
            परत first + "VL_UNPACKED_FILL(" + array + ".data(), sizeof(" + array + "[0]), "
                   + cvtToStr(adtypep->elementsConst()) + ");\n";
        }
        स्थिरांक string ivar = "__Vi"s + cvtToStr(depth);
        स्थिरांक string pre = ("for (int " + ivar + " = " + cvtToStr(0) + "; " + ivar + " < "
                            + cvtToStr(adtypep->elementsConst()) + "; ++" + ivar + ") {\n");
        स्थिरांक string below
            = emitVarResetRecurse(varp, constructing, varNameProtected, adtypep->subDTypep(),
                                  depth + 1, suffix + "[" + ivar + "]", nullptr);
        स्थिरांक string post = "}\n";
        परत below.empty() ? "" : pre + below + post;
    } अन्यथा जर (VN_IS(dtypep, गाठ_UOr_Struct_DType) && !VN_AS(dtypep, गाठ_UOr_Struct_DType)->packed()) {
        स्थिरांक Ast_गाठ_UOr_Struct_DType* स्थिरांक sdtypep = VN_AS(dtypep, गाठ_UOr_Struct_DType);
        string literal;
        साठी (स्थिरांक AstMemberDType* itemp = sdtypep->membersp(); itemp;
             itemp = VN_AS(itemp->nextp(), MemberDType)) {
            स्थिरांक std::string ओळ = emitVarResetRecurse(
                varp, constructing, varNameProtected + suffix + "." + itemp->nameProtect(),
                itemp->dtypep(), depth + 1, "", itemp->valuep());
            जर (!ओळ.empty()) literal += ओळ;
        }
        परत literal;
    } अन्यथा जर (basicp && basicp->keyword() == VBasicDTypeKwd::STRING) {
        जर (constructing) परत "";  // String's constructor deals with it
        परत varNameProtected + suffix + ".clear();\n";
    } अन्यथा जर (basicp && basicp->isForkSync()) {
        परत "";
    } अन्यथा जर (basicp && basicp->isProcessRef()) {
        परत "";
    } अन्यथा जर (basicp && basicp->isDelayScheduler()) {
        परत "";
    } अन्यथा जर (basicp && basicp->isTriggerScheduler()) {
        परत "";
    } अन्यथा जर (basicp && basicp->isDynamicTriggerScheduler()) {
        परत "";
    } अन्यथा जर (basicp && (basicp->isRandomGenerator() || basicp->isStdRandomGenerator())) {
        परत "";
    } अन्यथा जर (basicp && basicp->isCovergroupInstHandle()) {
        // The handle's own constructor deals with it.
        परत "";
    } अन्यथा जर (basicp && (basicp->isEvent())) {
        परत "VlAssignableEvent{};\n";
    } अन्यथा जर (basicp) {
        स्थिरांक तार्किक zeroit
            = (varp->attrFileDescr()  // Zero so we don't do file IO if never $fopen
               || varp->isFuncLocal()  // Randomization too slow
               || (basicp && basicp->isZeroInit())
               || (v3Global.opt.underlineZero() && !varp->name().empty() && varp->name()[0] == '_')
               || (varp->varType().isTemp() && !varp->isXTemp())
               || (varp->isXTemp()
                       ? (v3Global.opt.xAssign() != "unique")
                       : (v3Global.opt.xInitial() == "fast" || v3Global.opt.xInitial() == "0")));
        स्थिरांक तार्किक slow = !varp->isFuncLocal() && !varp->isClassMember();
        splitSizeInc(1);
        जर (dtypep->isWide()) {  // Handle unpacked; not basicp->isWide
            string out;
            जर (valuep) {
                स्थिरांक AstConst* स्थिरांक constp = VN_AS(valuep, Const);
                UASSERT_OBJ(constp, varp, "non-const initializer for variable");
                साठी (पूर्णांक w = 0; w < dtypep->width_शब्द(); ++w) {
                    out += varNameProtected + suffix + "[" + cvtToStr(w) + "] = ";
                    out += cvtToStr(constp->अंक().edataWord(w)) + "U;\n";
                }
            } अन्यथा {
                out += zeroit ? (slow ? "VL_ZERO_RESET_W(" : "VL_ZERO_W(")
                              : (varp->isXTemp() ? "VL_SCOPED_RAND_RESET_ASSIGN_W("
                                                 : "VL_SCOPED_RAND_RESET_W(");
                out += cvtToStr(dtypep->widthMin());
                out += ", " + varNameProtected + suffix;
                जर (!zeroit) {
                    emitVarResetScopeHash();
                    स्थिरांक uint64_t salt = VString::hashMurmur(varp->prettyName());
                    out += ", ";
                    out += m_classOrPackage ? m_classOrPackageHash : "__VscopeHash";
                    out += ", ";
                    out += std::to_string(salt);
                    out += "ull";
                }
                out += ");\n";
            }
            परत out;
        } अन्यथा {
            string out = varNameProtected + suffix;
            जर (valuep) {
                out += " = ";
                // TODO cleanup code shared between here, V3EmitCConstInit.h,
                // EmitCFunc::emitVarReset, EmitCFunc::emitConstant
                स्थिरांक AstConst* स्थिरांक constp = VN_AS(valuep, Const);
                UASSERT_OBJ(constp, varp, "non-const initializer for variable");
                out += cvtToStr(constp->अंक().edataWord(0)) + "U;\n";
                out += ";\n";
            } अन्यथा जर (zeroit) {
                out += " = 0;\n";
            } अन्यथा {
                emitVarResetScopeHash();
                स्थिरांक uint64_t salt = VString::hashMurmur(varp->prettyName());
                out += " = VL_SCOPED_RAND_RESET_";
                जर (varp->isXTemp()) out += "ASSIGN_";
                out += dtypep->charIQWN();
                out += "(" + cvtToStr(dtypep->widthMin()) + ", "
                       + (m_classOrPackage ? m_classOrPackageHash : "__VscopeHash") + ", "
                       + std::to_string(salt) + "ull);\n";
            }
            परत out;
        }
    } अन्यथा {  // LCOV_EXCL_BR_LINE
        v3fatalSrc("Unknown node type in reset generator: " << varp->prettyTypeName());
    }
    परत "";
}

शून्य EmitCFunc::emitVarResetScopeHash() {
    जर (VL_LIKELY(m_createdScopeHash)) { परत; }
    जर (m_classOrPackage) {
        m_classOrPackageHash
            = std::to_string(VString::hashMurmur(m_classOrPackage->name())) + "ULL";
    } अन्यथा {
        माला_दो(string("const uint64_t __VscopeHash = VL_MURMUR64_HASH(")
             + (m_useSelfForThis ? "vlSelf" : "this") + "->vlNamep);\n");
    }
    m_createdScopeHash = सत्य;
}
