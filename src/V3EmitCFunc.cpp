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

#समावेश "V3PchAstMT.h"

#समावेश "V3EmitCFunc.h"

#समावेश <map>
#समावेश <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// EmitCFunc

bool EmitCFunc::emitSimpleOk(Ast_गाँठ_व्यंजक* nodep) {
    // Can we put out a simple (A + B) instead of VL_ADD_III(A,B)?
    अगर (nodep->emitSimpleOperator() == "") वापस false;
    अगर (nodep->isWide()) वापस false;
    अगर (nodep->op1p() && nodep->op1p()->isWide()) वापस false;
    अगर (nodep->op2p() && nodep->op2p()->isWide()) वापस false;
    अगर (nodep->op3p() && nodep->op3p()->isWide()) वापस false;
    वापस true;
}

व्योम EmitCFunc::emitOpName(Ast_गाँठ* nodep, स्थिर string& format, Ast_गाँठ* lhsp, Ast_गाँठ* rhsp,
                           Ast_गाँठ* thsp) {
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

    bool needComma = false;
    bool usesQueue = false;
    string अग्र_Comma;
    स्वतः commaOut = [&out, &अग्र_Comma]() {
        अगर (!अग्र_Comma.empty()) {
            out += अग्र_Comma;
            अग्र_Comma = "";
        }
    };

    स्वतः putOut = [यह, &out]() {
        अगर (!out.empty()) माला_दो(out);
        out = "";
    };

    क्रम (string::const_iterator pos = format.begin(); pos != format.end(); ++pos) {
        अगर (pos[0] == ',') {
            // Remember we need to add one, but don't do yet to avoid ",)"
            अगर (needComma) {
                अगर (pos[1] == ' ') {
                    अग्र_Comma = ", ";
                } अन्यथा {
                    अग्र_Comma = ",";
                }
                needComma = false;
            }
            अगर (pos[1] == ' ') ++pos;  // Must do even if no nextComma
        } अन्यथा अगर (pos[0] == '%') {
            ++pos;
            bool detail = false;
            Ast_गाँठ* detailp = nullptr;
            चयन (pos[0]) {
            हाल '%': out += '%'; अवरोध;
            हाल 'k':
                putOut();
                putbs("");
                अवरोध;
            हाल 'n':
                detail = true;
                detailp = nodep;
                अवरोध;
            हाल 'l':
                detail = true;
                detailp = lhsp;
                अवरोध;
            हाल 'r':
                detail = true;
                detailp = rhsp;
                अवरोध;
            हाल 't':
                detail = true;
                detailp = thsp;
                अवरोध;
            हाल 'P':
                अगर (nodep->isWide() && !usesQueue) {
                    UASSERT_OBJ(m_wide_अस्थायी_Refp, nodep,
                                "Wide op " << nodep->prettyTypeName()
                                           << " w/ no temp, perhaps missing op in V3EmitC?");
                    commaOut();
                    putOut();
                    अगर (!m_wide_अस्थायी_Refp->selfPointer().isEmpty()) {
                        emitDereference(m_wide_अस्थायी_Refp,
                                        m_wide_अस्थायी_Refp->selfPointerProtect(m_useSelfForThis));
                    }
                    out += varName(m_wide_अस्थायी_Refp->varp());
                    m_wide_अस्थायी_Refp = nullptr;
                    needComma = true;
                } अन्यथा अगर (usesQueue) {
                    commaOut();
                    putOut();
                    iterate_And_अग्र_Const_Null(nodep->backp()->op2p());
                    needComma = true;
                }

                अवरोध;
            शेष: nodep->v3fatalSrc("Unknown emitOperator format code: %" << pos[0]); अवरोध;
            }
            अगर (detail) {
                // Get next letter of %[nlrt]
                ++pos;
                चयन (pos[0]) {
                हाल 'q':
                    putOut();
                    // If we are assigning this to a queue we need to get the return type
                    अगर (VN_IS(detailp->backp(), Assign)
                        && VN_IS(detailp->backp()->op2p()->dtypep()->skipRefp(), QueueDType)) {
                        माला_दो("R");  // R for queue
                        usesQueue = true;
                    } अन्यथा अगर (VN_IS(detailp->dtypep()->skipRefp(), QueueDType)
                               || VN_IS(detailp->dtypep()->skipRefp(), StreamDType)) {
                        माला_दो("R");  // R for queue
                    } अन्यथा {
                        emitIQW(detailp);
                    }
                    अवरोध;
                हाल 'w':
                    commaOut();
                    out += cvtToStr(detailp->widthMin());
                    needComma = true;
                    अवरोध;
                हाल 'W':
                    अगर (lhsp->isWide()) {
                        commaOut();
                        out += cvtToStr(lhsp->width_शब्दगण());
                        needComma = true;
                    } अन्यथा अगर (VN_IS(lhsp, StreamR)) {
                        commaOut();
                        out += cvtToStr(rhsp->width_शब्दगण());
                        needComma = true;
                    }
                    अवरोध;
                हाल 'i':
                    commaOut();
                    UASSERT_OBJ(detailp, nodep, "emitOperator() references undef node");
                    putOut();
                    iterate_And_अग्र_Const_Null(detailp);
                    needComma = true;
                    अवरोध;
                शेष:
                    nodep->v3fatalSrc("Unknown emitOperator format code: %[nlrt]" << pos[0]);
                    अवरोध;
                }
            }
        } अन्यथा अगर (pos[0] == ')') {
            अग्र_Comma = "";
            out += ')';
        } अन्यथा अगर (pos[0] == '(') {
            commaOut();
            needComma = false;
            out += '(';
        } अन्यथा {
            // Normal text
            अगर (std::है_अक्षर_अंक(pos[0])) needComma = true;
            commaOut();
            out += pos[0];
        }
    }
    putOut();
}

bool EmitCFunc::displayEmitHeader(Ast_गाँठ* nodep) {
    bool isStmt = false;
    अगर (स्थिर AstFScanF* स्थिर dispp = VN_CAST(nodep, FScanF)) {
        isStmt = false;
        putns(nodep, "VL_FSCANF_INX(");
        iterateConst(dispp->filep());
        माला_दो(",");
    } अन्यथा अगर (स्थिर AstSScanF* स्थिर dispp = VN_CAST(nodep, SScanF)) {
        isStmt = false;
        putns(nodep, "VL_SSCANF_I");
        emitIQW(dispp->fromp());
        माला_दो("NX(");
        माला_दो(cvtToStr(dispp->fromp()->widthMin()));
        माला_दो(",");
        iterateConst(dispp->fromp());
        माला_दो(",");
    } अन्यथा अगर (स्थिर AstDisplay* स्थिर dispp = VN_CAST(nodep, Display)) {
        isStmt = true;
        अगर (dispp->filep()) {
            putns(nodep, "VL_FWRITEF_NX(");
            iterateConst(dispp->filep());
            माला_दो(",");
        } अन्यथा {
            putns(nodep, "VL_WRITEF_NX(");
        }
    } अन्यथा अगर (स्थिर AstSFormat* स्थिर dispp = VN_CAST(nodep, SFormat)) {
        isStmt = true;
        माला_दो("VL_SFORMAT_NX(");
        अगर (!dispp->lhsp()->dtypep()->isString()) {
            माला_दो(cvtToStr(dispp->lhsp()->widthMin()));
            putbs(",");
        }
        iterateConst(dispp->lhsp());
        emitDatap(dispp->lhsp());
        putbs(",");
    } अन्यथा अगर (VN_IS(nodep, SFormatF)) {
        isStmt = false;
        putns(nodep, "VL_SFORMATF_N_NX(");
    } अन्यथा {
        nodep->v3fatalSrc("Unknown displayEmit node type");
    }
    वापस isStmt;
}

व्योम EmitCFunc::display_गाँठ(Ast_गाँठ* nodep, AstSFormatF* fmtp,  // fmtp is nullptr for AstScan
                            स्थिर string& vformat, Ast_गाँठ* exprsp, bool isScan) {
    // Check format, if it exists
    स्थिर bool व्यंजक_Format_2 = fmtp && fmtp->व्यंजक_Format_2();
    bool needsScope = व्यंजक_Format_2;
    bool needsTimescale = व्यंजक_Format_2;
    पूर्णांक formatArgs = 0;
    अगर (!व्यंजक_Format_2) {
        // Check display and argument count to head-off later runtime issues
        bool inPct = false;
        bool ignore = false;
        क्रम (अक्षर ch : vformat) {
            // UINFO(1, "Parse '" << *pos << "'  IP" << inPct << " List " << cvtToHex(elistp));
            अगर (!inPct && ch == '%') {
                inPct = true;
                ignore = false;
            } अन्यथा अगर (inPct && (std::है_अंक(ch) || ch == '.' || ch == '-')) {
            } अन्यथा अगर (!inPct) {  // Normal text
            } अन्यथा {  // Format character
                inPct = false;
                ch = std::छोटे(ch);
                चयन (ch) {
                हाल '%': अवरोध;
                हाल '*':
                    inPct = true;  // Get more digits
                    ignore = true;
                    अवरोध;
                हाल 'm': needsScope = true; अवरोध;
                हाल 't':
                    needsTimescale = true;
                    ++formatArgs;
                    अवरोध;
                शेष:
                    अगर (!ignore && V3_संख्या::displayedFmtHasArg(ch, isScan)) ++formatArgs;
                    अवरोध;
                }
            }
        }
    }

    अगर (vformat.empty() && VN_IS(nodep, Display))  // not fscanf etc, as they need to return value
        वापस;  // NOP

    स्थिर bool isStmt = displayEmitHeader(nodep);

    अगर (व्यंजक_Format_2) {
        UASSERT_OBJ(exprsp, nodep, "Missing format expression");
        iterateConst(exprsp);
        emitDatap(exprsp);
        exprsp = exprsp->nextp();
    } अन्यथा {
        ofp()->putsQuoted(vformat);
    }

    पूर्णांक व्यंजक_Args = 0;
    क्रम (Ast_गाँठ* argp = exprsp; argp; argp = argp->nextp()) ++व्यंजक_Args;
    अगर (VL_UNCOVERABLE(!व्यंजक_Format_2 && व्यंजक_Args != formatArgs)) {  // LCOV_EXCL_START
        // expectDisplay() checks this first, so probably internal error if found here
        nodep->v3error("Internal: Missing or extra arguments for $display-like format, "
                       << " expression had " << व्यंजक_Args << ", format had " << formatArgs);
    }  // LCOV_EXCL_STOP

    // argc for error check. Also MSVC++ requires va_args to not be off a reference
    पूर्णांक argc = 0;
    अगर (needsScope) ++argc;
    अगर (needsTimescale) ++argc;
    क्रम (Ast_गाँठ* argp = exprsp; argp; argp = argp->nextp()) {
        ++argc;
        स्थिर AstSFormatArg* स्थिर fargp = VN_CAST(argp, SFormatArg);
        अगर (fargp && fargp->formatAttr().isEnum()) ++argc;  // Additional name argument
    }
    ofp()->माला_दो("," + std::to_string(argc));

    अगर (needsScope) {
        AstScopeName* स्थिर scopenamep = fmtp ? fmtp->scopeNamep() : nullptr;
        UASSERT_OBJ(scopenamep, nodep, "Display with %m but no AstScopeName");
        ofp()->माला_दो(", '"s + VFormatAttr{VFormatAttr::SCOPE}.ascii() + "'");
        अगर (scopenamep->selfRelative()) {
            // vlNamep is catName(name(), instance), so starts with '.' when the model is unnamed
            स्थिर string namep = string{m_useSelfForThis ? "vlSelf" : "this"} + "->vlNamep";
            ofp()->माला_दो(",(vlSymsp->name()[0] ? " + namep + " : " + namep + " + 1),");
            ofp()->putsQuoted(scopenamep->scopeAttr());
        } अन्यथा {
            ofp()->माला_दो(",vlSymsp->name(),");
            ofp()->putsQuoted(scopenamep->scopePrettySymName());
        }
    }

    अगर (needsTimescale) {
        VTimescale timeunit = VTimescale::NONE;
        अगर (स्थिर AstDisplay* स्थिर anodep = VN_CAST(nodep, Display)) {
            timeunit = anodep->fmtp()->timeunit();
        } अन्यथा अगर (स्थिर AstSFormat* स्थिर anodep = VN_CAST(nodep, SFormat)) {
            timeunit = anodep->fmtp()->timeunit();
        } अन्यथा अगर (स्थिर AstSScanF* स्थिर anodep = VN_CAST(nodep, SScanF)) {
            timeunit = anodep->timeunit();
        } अन्यथा अगर (स्थिर AstSFormatF* स्थिर anodep = VN_CAST(nodep, SFormatF)) {
            timeunit = anodep->timeunit();
        }
        UASSERT_OBJ(!timeunit.isNone(), nodep,
                    "Use of %t must be under AstDisplay, AstSFormat, or AstSFormatF, or "
                    "SScanF, and timeunit set");
        ofp()->माला_दो(", '"s + VFormatAttr{VFormatAttr::TIMEUNIT}.ascii() + "'");
        ofp()->माला_दो(","s + cvtToStr((पूर्णांक)timeunit.powerOfTen()));
    }

    // Arguments
    क्रम (Ast_गाँठ* argp = exprsp; argp; argp = argp->nextp()) {
        ofp()->indentInc();
        ofp()->putbs("");
        AstSFormatArg* स्थिर fargp = VN_CAST(argp, SFormatArg);
        Ast_गाँठ* स्थिर subargp = fargp ? fargp->exprp() : argp;
        स्थिर VFormatAttr formatAttr = AstSFormatArg::formatAttrDefauled(fargp, subargp->dtypep());
        माला_दो(", '"s + formatAttr.ascii() + '\'');
        अगर (formatAttr.isSigned() || formatAttr.isUnsigned() || formatAttr.isEnum())
            माला_दो("," + cvtToStr(subargp->widthMin()));
        स्थिर bool addrof = isScan || formatAttr.isString() || formatAttr.isComplex();
        स्थिर bool wideEnum = formatAttr.isEnum() && subargp->isWide();
        माला_दो(",");
        अगर (wideEnum) माला_दो("static_cast<const EData*>(");
        अगर (addrof) माला_दो("&(");
        अगर (VN_IS(subargp, StreamR))
            emitStreamR(
                VN_CAST(subargp, StreamR),
                nodep);  // This has to be done here because streamR doesn't know what it returns
        अन्यथा { iterateConst(subargp); }
        अगर (addrof) माला_दो(")");
        अगर (!addrof) emitDatap(argp);
        अगर (wideEnum) माला_दो(")");
        अगर (formatAttr.isEnum()) {
            माला_दो(", '"s + VFormatAttr{VFormatAttr::STRING}.ascii() + "', &(");
            iterateConst(fargp->namep());
            माला_दो(")");
        }
        ofp()->indentDec();
    }

    // End
    अगर (isStmt) {
        माला_दो(");\n");
    } अन्यथा {
        माला_दो(") ");
    }
}

व्योम EmitCFunc::emitCCallArgs(स्थिर Ast_गाँठ_CCall* nodep, स्थिर string& selfPointer,
                              bool inProcess) {
    putns(nodep, "(");
    bool comma = false;
    अगर (nodep->funcp()->isLoose() && !nodep->funcp()->isStatic()) {
        UASSERT_OBJ(!selfPointer.empty(), nodep, "Call to loose method without self pointer");
        माला_दो(selfPointer);
        comma = true;
    }
    अगर (nodep->funcp()->needProcess()) {
        अगर (comma) माला_दो(", ");
        अगर (VN_IS(nodep->backp(), CAwait) || !nodep->funcp()->isCoroutine()) {
            माला_दो("vlProcess");
        } अन्यथा अगर (inProcess) {
            माला_दो("std::make_shared<VlProcess>(vlProcess)");
        } अन्यथा {
            माला_दो("std::make_shared<VlProcess>()");
        }
        comma = true;
    }
    अगर (!nodep->argTypes().empty()) {
        अगर (comma) माला_दो(", ");
        माला_दो(nodep->argTypes());
        comma = true;
    }
    put_Comma_Iterate_अग्र(nodep->argsp(), comma);
    माला_दो(")");
}

std::string EmitCFunc::dereferenceString(स्थिर std::string& pointer) स्थिर {
    अगर (pointer[0] == '(' && pointer[1] == '&') {
        // remove "address of" followed by immediate dereference
        // Note: this relies on only the form '(&OBJECT)' being used by Verilator
        वापस pointer.substr(2, pointer.लंबाई() - 3) + '.';
    } अन्यथा {
        अगर (pointer == "vlSelf" && m_usevlSelfRef) {
            वापस "vlSelfRef.";
        } अन्यथा {
            वापस pointer + "->";
        }
    }
}
व्योम EmitCFunc::emitDereference(Ast_गाँठ* nodep, स्थिर string& pointer) {
    putns(nodep, dereferenceString(pointer));
}

व्योम EmitCFunc::emitCvtPackStr(Ast_गाँठ* nodep) {
    अगर (स्थिर AstConst* स्थिर constp = VN_CAST(nodep, Const)) {
        emitConstantString(constp);
    } अन्यथा अगर (VN_IS(nodep->dtypep(), StreamDType)) {
        putnbs(nodep, "VL_CVT_PACK_STR_ND(");
        iterate_And_अग्र_Const_Null(nodep);
        माला_दो(")");
    } अन्यथा {
        putnbs(nodep, "VL_CVT_PACK_STR_N");
        emitIQW(nodep);
        माला_दो("(");
        अगर (nodep->isWide()) {
            // Note argument width, not node width (which is always 32)
            माला_दो(cvtToStr(nodep->width_शब्दगण()));
            माला_दो(", ");
        }
        iterate_And_अग्र_Const_Null(nodep);
        माला_दो(")");
    }
}

व्योम EmitCFunc::emitConstant(AstConst* nodep) {
    // Put out constant set to the specified variable, or given variable in a string
    स्थिर V3_संख्या& अंक = nodep->अंक();
    अगर (अंक.isFourState()) {
        nodep->v3warn(E_UNSUPPORTED, "Unsupported: 4-state numbers in this context");
        वापस;
    }
    putns(nodep, अंक.emitC());
}

व्योम EmitCFunc::emitConstantString(स्थिर AstConst* nodep) {
    // Const might be a Verilog array-type string, but need to always output std::string
    putnbs(nodep, "std::string{");
    स्थिर string str = nodep->अंक().toString();
    अगर (!str.empty()) putsQuoted(str);
    माला_दो("}");
}

व्योम EmitCFunc::emitVarReset(स्थिर string& prefix, AstVar* varp, bool constructing) {
    // 'constructing' indicates that the object was just constructed, so if it is a string or
    // something that starts off clear already, no need to clear it again
    Ast_गाँठ_DType* स्थिर dtypep = varp->dtypep()->skipRefp();
    स्थिर string vlSelf = VSelfPointerText::replaceThis(m_useSelfForThis, "this->");
    स्थिर string varNameProtected
        = ((VN_IS(m_modp, Class) || varp->isFuncLocal()) || !prefix.empty())
              ? varName(varp)
              : vlSelf + varName(varp);
    स्थिर string newPrefix = prefix + varNameProtected;
    अगर (varp->isIO() && m_modp->is_शीर्ष() && optSystemC()) {
        // System C top I/O doesn't need loading, as the lower level subinst code does it.}
    } अन्यथा अगर (varp->isParam()) {
        UASSERT_OBJ(varp->valuep(), varp, "No init for a param?");
        // If a simple CONST value we initialize it using an enum
        // If an ARRAYINIT we initialize it using an initial block similar to a signal
        // puts("// parameter "+varp->nameProtect()+" = "+varp->valuep()->name()+"\n");
    } अन्यथा {
        putns(varp,
              emitVarResetRecurse(varp, constructing, newPrefix, dtypep, 0, "", varp->valuep()));
    }
}

string EmitCFunc::emitVarResetRecurse(स्थिर AstVar* varp, bool constructing,
                                      स्थिर string& varNameProtected, Ast_गाँठ_DType* dtypep,
                                      पूर्णांक depth, स्थिर string& suffix, स्थिर Ast_गाँठ* valuep) {
    dtypep = dtypep->skipRefp();
    AstBasicDType* स्थिर basicp = dtypep->basicp();
    // Returns string to do resetting, empty to do nothing (which caller should handle)
    अगर (AstAssocArrayDType* स्थिर adtypep = VN_CAST(dtypep, AssocArrayDType)) {
        // Access std::array as C array
        स्थिर string pre = constructing ? "" : varNameProtected + suffix + ".clear();\n";
        वापस pre
               + emitVarResetRecurse(varp, constructing, varNameProtected, adtypep->subDTypep(),
                                     depth + 1, suffix + ".atDefault()", nullptr);
    } अन्यथा अगर (AstWildcardArrayDType* स्थिर adtypep = VN_CAST(dtypep, WildcardArrayDType)) {
        // Access std::array as C array
        स्थिर string pre = constructing ? "" : varNameProtected + suffix + ".clear();\n";
        वापस pre
               + emitVarResetRecurse(varp, constructing, varNameProtected, adtypep->subDTypep(),
                                     depth + 1, suffix + ".atDefault()", nullptr);
    } अन्यथा अगर (VN_IS(dtypep, CDType)) {
        वापस "";  // Constructor does it
    } अन्यथा अगर (VN_IS(dtypep, CoverCrossDType) || VN_IS(dtypep, CoverpointDType)) {
        वापस "";  // Covergroup constructor creates the runtime and assigns the pointer
    } अन्यथा अगर (स्थिर AstClassRefDType* स्थिर adtypep = VN_CAST(dtypep, ClassRefDType)) {
        वापस adtypep->rawPointer() ? varNameProtected + suffix + " = nullptr;\n" : "";
    } अन्यथा अगर (VN_IS(dtypep, IfaceRefDType)) {
        वापस varNameProtected + suffix + " = nullptr;\n";
    } अन्यथा अगर (स्थिर AstDynArrayDType* स्थिर adtypep = VN_CAST(dtypep, DynArrayDType)) {
        स्थिर string pre = constructing ? "" : varNameProtected + suffix + ".clear();\n";
        वापस pre
               + emitVarResetRecurse(varp, constructing, varNameProtected, adtypep->subDTypep(),
                                     depth + 1, suffix + ".atDefault()", nullptr);
    } अन्यथा अगर (स्थिर AstQueueDType* स्थिर adtypep = VN_CAST(dtypep, QueueDType)) {
        स्थिर string pre = constructing ? "" : varNameProtected + suffix + ".clear();\n";
        वापस pre
               + emitVarResetRecurse(varp, constructing, varNameProtected, adtypep->subDTypep(),
                                     depth + 1, suffix + ".atDefault()", nullptr);
    } अन्यथा अगर (VN_IS(dtypep, SampleQueueDType)) {
        वापस "";
    } अन्यथा अगर (स्थिर AstUnpackArrayDType* स्थिर adtypep = VN_CAST(dtypep, UnpackArrayDType)) {
        UASSERT_OBJ(adtypep->hi() >= adtypep->lo(), varp,
                    "Should have swapped msb & lsb earlier.");
        स्थिर Ast_गाँठ_DType* स्थिर elementp = adtypep->subDTypep()->skipRefp();
        अगर (!varp->isFuncLocal() && elementp->isIntegralOrPacked()) {
            // Scoped reset has no array-index input or mutable RNG state. Initialize one
            // element, then fill in the optimized runtime even when this code is cold.
            स्थिर string first
                = emitVarResetRecurse(varp, constructing, varNameProtected, adtypep->subDTypep(),
                                      depth + 1, suffix + "[0]", nullptr);
            स्थिर string array = varNameProtected + suffix;
            वापस first + "VL_UNPACKED_FILL(" + array + ".data(), sizeof(" + array + "[0]), "
                   + cvtToStr(adtypep->elementsConst()) + ");\n";
        }
        स्थिर string ivar = "__Vi"s + cvtToStr(depth);
        स्थिर string pre = ("for (int " + ivar + " = " + cvtToStr(0) + "; " + ivar + " < "
                            + cvtToStr(adtypep->elementsConst()) + "; ++" + ivar + ") {\n");
        स्थिर string below
            = emitVarResetRecurse(varp, constructing, varNameProtected, adtypep->subDTypep(),
                                  depth + 1, suffix + "[" + ivar + "]", nullptr);
        स्थिर string post = "}\n";
        वापस below.empty() ? "" : pre + below + post;
    } अन्यथा अगर (VN_IS(dtypep, गाँठ_UOr_Struct_DType) && !VN_AS(dtypep, गाँठ_UOr_Struct_DType)->packed()) {
        स्थिर Ast_गाँठ_UOr_Struct_DType* स्थिर sdtypep = VN_AS(dtypep, गाँठ_UOr_Struct_DType);
        string literal;
        क्रम (स्थिर AstMemberDType* itemp = sdtypep->membersp(); itemp;
             itemp = VN_AS(itemp->nextp(), MemberDType)) {
            स्थिर std::string पंक्ति = emitVarResetRecurse(
                varp, constructing, varNameProtected + suffix + "." + itemp->nameProtect(),
                itemp->dtypep(), depth + 1, "", itemp->valuep());
            अगर (!पंक्ति.empty()) literal += पंक्ति;
        }
        वापस literal;
    } अन्यथा अगर (basicp && basicp->keyword() == VBasicDTypeKwd::STRING) {
        अगर (constructing) वापस "";  // String's constructor deals with it
        वापस varNameProtected + suffix + ".clear();\n";
    } अन्यथा अगर (basicp && basicp->isForkSync()) {
        वापस "";
    } अन्यथा अगर (basicp && basicp->isProcessRef()) {
        वापस "";
    } अन्यथा अगर (basicp && basicp->isDelayScheduler()) {
        वापस "";
    } अन्यथा अगर (basicp && basicp->isTriggerScheduler()) {
        वापस "";
    } अन्यथा अगर (basicp && basicp->isDynamicTriggerScheduler()) {
        वापस "";
    } अन्यथा अगर (basicp && (basicp->isRandomGenerator() || basicp->isStdRandomGenerator())) {
        वापस "";
    } अन्यथा अगर (basicp && basicp->isCovergroupInstHandle()) {
        // The handle's own constructor deals with it.
        वापस "";
    } अन्यथा अगर (basicp && (basicp->isEvent())) {
        वापस "VlAssignableEvent{};\n";
    } अन्यथा अगर (basicp) {
        स्थिर bool zeroit
            = (varp->attrFileDescr()  // Zero so we don't do file IO if never $fopen
               || varp->isFuncLocal()  // Randomization too slow
               || (basicp && basicp->isZeroInit())
               || (v3Global.opt.underlineZero() && !varp->name().empty() && varp->name()[0] == '_')
               || (varp->varType().is_अस्थायी() && !varp->isXTemp())
               || (varp->isXTemp()
                       ? (v3Global.opt.xAssign() != "unique")
                       : (v3Global.opt.xInitial() == "fast" || v3Global.opt.xInitial() == "0")));
        स्थिर bool slow = !varp->isFuncLocal() && !varp->isClassMember();
        split_आकार_Inc(1);
        अगर (dtypep->isWide()) {  // Handle unpacked; not basicp->isWide
            string out;
            अगर (valuep) {
                स्थिर AstConst* स्थिर constp = VN_AS(valuep, Const);
                UASSERT_OBJ(constp, varp, "non-const initializer for variable");
                क्रम (पूर्णांक w = 0; w < dtypep->width_शब्दगण(); ++w) {
                    out += varNameProtected + suffix + "[" + cvtToStr(w) + "] = ";
                    out += cvtToStr(constp->अंक().edataWord(w)) + "U;\n";
                }
            } अन्यथा {
                out += zeroit ? (slow ? "VL_ZERO_RESET_W(" : "VL_ZERO_W(")
                              : (varp->isXTemp() ? "VL_SCOPED_RAND_RESET_ASSIGN_W("
                                                 : "VL_SCOPED_RAND_RESET_W(");
                out += cvtToStr(dtypep->widthMin());
                out += ", " + varNameProtected + suffix;
                अगर (!zeroit) {
                    emitVarResetScopeHash();
                    स्थिर uint64_t salt = VString::hashMurmur(varp->prettyName());
                    out += ", ";
                    out += m_classOrPackage ? m_classOrPackageHash : "__VscopeHash";
                    out += ", ";
                    out += std::to_string(salt);
                    out += "ull";
                }
                out += ");\n";
            }
            वापस out;
        } अन्यथा {
            string out = varNameProtected + suffix;
            अगर (valuep) {
                out += " = ";
                // TODO cleanup code shared between here, V3EmitCConstInit.h,
                // EmitCFunc::emitVarReset, EmitCFunc::emitConstant
                स्थिर AstConst* स्थिर constp = VN_AS(valuep, Const);
                UASSERT_OBJ(constp, varp, "non-const initializer for variable");
                out += cvtToStr(constp->अंक().edataWord(0)) + "U;\n";
                out += ";\n";
            } अन्यथा अगर (zeroit) {
                out += " = 0;\n";
            } अन्यथा {
                emitVarResetScopeHash();
                स्थिर uint64_t salt = VString::hashMurmur(varp->prettyName());
                out += " = VL_SCOPED_RAND_RESET_";
                अगर (varp->isXTemp()) out += "ASSIGN_";
                out += dtypep->charIQWN();
                out += "(" + cvtToStr(dtypep->widthMin()) + ", "
                       + (m_classOrPackage ? m_classOrPackageHash : "__VscopeHash") + ", "
                       + std::to_string(salt) + "ull);\n";
            }
            वापस out;
        }
    } अन्यथा {  // LCOV_EXCL_BR_LINE
        v3fatalSrc("Unknown node type in reset generator: " << varp->prettyTypeName());
    }
    वापस "";
}

व्योम EmitCFunc::emitVarResetScopeHash() {
    अगर (VL_LIKELY(m_createdScopeHash)) { वापस; }
    अगर (m_classOrPackage) {
        m_classOrPackageHash
            = std::to_string(VString::hashMurmur(m_classOrPackage->name())) + "ULL";
    } अन्यथा {
        माला_दो(string("const uint64_t __VscopeHash = VL_MURMUR64_HASH(")
             + (m_useSelfForThis ? "vlSelf" : "this") + "->vlNamep);\n");
    }
    m_createdScopeHash = true;
}
