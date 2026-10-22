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
    यदि (nodep->emitSimpleOperator() == "") प्रत्यागम असत्य;
    यदि (nodep->isWide()) प्रत्यागम असत्य;
    यदि (nodep->op1p() && nodep->op1p()->isWide()) प्रत्यागम असत्य;
    यदि (nodep->op2p() && nodep->op2p()->isWide()) प्रत्यागम असत्य;
    यदि (nodep->op3p() && nodep->op3p()->isWide()) प्रत्यागम असत्य;
    प्रत्यागम सत्य;
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
    string बहिर्गमनम्;
    putnbs(nodep, "");

    तार्किक needComma = असत्य;
    तार्किक उपयुङ्क्ते_Queue = असत्य;
    string अग्र_Comma;
    स्वचालित comma_बहिर्गमनम् = [&बहिर्गमनम्, &अग्र_Comma]() {
        यदि (!अग्र_Comma.रिक्तानि()) {
            बहिर्गमनम् += अग्र_Comma;
            अग्र_Comma = "";
        }
    };

    स्वचालित put_बहिर्गमनम् = [एतत्, &बहिर्गमनम्]() {
        यदि (!बहिर्गमनम्.रिक्तानि()) पङ्क्तिं_लिख(बहिर्गमनम्);
        बहिर्गमनम् = "";
    };

    प्रति (string::const_iterator pos = format.begin(); pos != format.अन्तः_2(); ++pos) {
        यदि (pos[0] == ',') {
            // Remember we need to add one, but don't do yet to avoid ",)"
            यदि (needComma) {
                यदि (pos[1] == ' ') {
                    अग्र_Comma = ", ";
                } अन्यथा {
                    अग्र_Comma = ",";
                }
                needComma = असत्य;
            }
            यदि (pos[1] == ' ') ++pos;  // Must do even if no nextComma
        } अन्यथा यदि (pos[0] == '%') {
            ++pos;
            तार्किक detail = असत्य;
            Ast_ग्रन्थिः* detailp = शून्यसूचकः;
            विकल्प (pos[0]) {
            पक्ष '%': बहिर्गमनम् += '%'; भङ्ग;
            पक्ष 'k':
                put_बहिर्गमनम्();
                putbs("");
                भङ्ग;
            पक्ष 'n':
                detail = सत्य;
                detailp = nodep;
                भङ्ग;
            पक्ष 'l':
                detail = सत्य;
                detailp = lhsp;
                भङ्ग;
            पक्ष 'r':
                detail = सत्य;
                detailp = rhsp;
                भङ्ग;
            पक्ष 't':
                detail = सत्य;
                detailp = thsp;
                भङ्ग;
            पक्ष 'P':
                यदि (nodep->isWide() && !उपयुङ्क्ते_Queue) {
                    UASSERT_OBJ(m_wide_अस्थायी_Refp, nodep,
                                "Wide op " << nodep->prettyTypeName()
                                           << " w/ no temp, perhaps missing op in V3EmitC?");
                    comma_बहिर्गमनम्();
                    put_बहिर्गमनम्();
                    यदि (!m_wide_अस्थायी_Refp->selfPointer().is_रिक्तानि()) {
                        emitDereference(m_wide_अस्थायी_Refp,
                                        m_wide_अस्थायी_Refp->selfPointerProtect(m_useSelfForThis));
                    }
                    बहिर्गमनम् += varName(m_wide_अस्थायी_Refp->varp());
                    m_wide_अस्थायी_Refp = शून्यसूचकः;
                    needComma = सत्य;
                } अन्यथा यदि (उपयुङ्क्ते_Queue) {
                    comma_बहिर्गमनम्();
                    put_बहिर्गमनम्();
                    iterate_And_अग्र_Const_Null(nodep->backp()->op2p());
                    needComma = सत्य;
                }

                भङ्ग;
            स्वभाव: nodep->v3fatalSrc("Unknown emitOperator format code: %" << pos[0]); भङ्ग;
            }
            यदि (detail) {
                // Get next letter of %[nlrt]
                ++pos;
                विकल्प (pos[0]) {
                पक्ष 'q':
                    put_बहिर्गमनम्();
                    // If we are assigning this to a queue we need to get the return type
                    यदि (VN_IS(detailp->backp(), नियोजय)
                        && VN_IS(detailp->backp()->op2p()->dtypep()->skipRefp(), QueueDType)) {
                        पङ्क्तिं_लिख("R");  // R for queue
                        उपयुङ्क्ते_Queue = सत्य;
                    } अन्यथा यदि (VN_IS(detailp->dtypep()->skipRefp(), QueueDType)
                               || VN_IS(detailp->dtypep()->skipRefp(), प्रवाहः_DType)) {
                        पङ्क्तिं_लिख("R");  // R for queue
                    } अन्यथा {
                        emitIQW(detailp);
                    }
                    भङ्ग;
                पक्ष 'w':
                    comma_बहिर्गमनम्();
                    बहिर्गमनम् += cvt_गन्तव्यम्_Str(detailp->विस्तारः_Min());
                    needComma = सत्य;
                    भङ्ग;
                पक्ष 'W':
                    यदि (lhsp->isWide()) {
                        comma_बहिर्गमनम्();
                        बहिर्गमनम् += cvt_गन्तव्यम्_Str(lhsp->विस्तारः_शब्दाः());
                        needComma = सत्य;
                    } अन्यथा यदि (VN_IS(lhsp, प्रवाहः_R)) {
                        comma_बहिर्गमनम्();
                        बहिर्गमनम् += cvt_गन्तव्यम्_Str(rhsp->विस्तारः_शब्दाः());
                        needComma = सत्य;
                    }
                    भङ्ग;
                पक्ष 'i':
                    comma_बहिर्गमनम्();
                    UASSERT_OBJ(detailp, nodep, "emitOperator() references undef node");
                    put_बहिर्गमनम्();
                    iterate_And_अग्र_Const_Null(detailp);
                    needComma = सत्य;
                    भङ्ग;
                स्वभाव:
                    nodep->v3fatalSrc("Unknown emitOperator format code: %[nlrt]" << pos[0]);
                    भङ्ग;
                }
            }
        } अन्यथा यदि (pos[0] == ')') {
            अग्र_Comma = "";
            बहिर्गमनम् += ')';
        } अन्यथा यदि (pos[0] == '(') {
            comma_बहिर्गमनम्();
            needComma = असत्य;
            बहिर्गमनम् += '(';
        } अन्यथा {
            // Normal text
            यदि (std::अक्षरम्_अङ्कः_वा(pos[0])) needComma = सत्य;
            comma_बहिर्गमनम्();
            बहिर्गमनम् += pos[0];
        }
    }
    put_बहिर्गमनम्();
}

तार्किक EmitCFunc::display_Emit_शीर्षकम्(Ast_ग्रन्थिः* nodep) {
    तार्किक isStmt = असत्य;
    यदि (ध्रुव AstFScanF* ध्रुव dispp = VN_CAST(nodep, FScanF)) {
        isStmt = असत्य;
        putns(nodep, "VL_FSCANF_INX(");
        iterateConst(dispp->filep());
        पङ्क्तिं_लिख(",");
    } अन्यथा यदि (ध्रुव AstSScanF* ध्रुव dispp = VN_CAST(nodep, SScanF)) {
        isStmt = असत्य;
        putns(nodep, "VL_SSCANF_I");
        emitIQW(dispp->fromp());
        पङ्क्तिं_लिख("NX(");
        पङ्क्तिं_लिख(cvt_गन्तव्यम्_Str(dispp->fromp()->विस्तारः_Min()));
        पङ्क्तिं_लिख(",");
        iterateConst(dispp->fromp());
        पङ्क्तिं_लिख(",");
    } अन्यथा यदि (ध्रुव AstDisplay* ध्रुव dispp = VN_CAST(nodep, Display)) {
        isStmt = सत्य;
        यदि (dispp->filep()) {
            putns(nodep, "VL_FWRITEF_NX(");
            iterateConst(dispp->filep());
            पङ्क्तिं_लिख(",");
        } अन्यथा {
            putns(nodep, "VL_WRITEF_NX(");
        }
    } अन्यथा यदि (ध्रुव AstSFormat* ध्रुव dispp = VN_CAST(nodep, SFormat)) {
        isStmt = सत्य;
        पङ्क्तिं_लिख("VL_SFORMAT_NX(");
        यदि (!dispp->lhsp()->dtypep()->isString()) {
            पङ्क्तिं_लिख(cvt_गन्तव्यम्_Str(dispp->lhsp()->विस्तारः_Min()));
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
    प्रत्यागम isStmt;
}

शून्य EmitCFunc::display_ग्रन्थिः(Ast_ग्रन्थिः* nodep, AstSFormatF* fmtp,  // fmtp is nullptr for AstScan
                            ध्रुव string& vformat, Ast_ग्रन्थिः* exprsp, तार्किक is_अवलोकनम्) {
    // Check format, if it exists
    ध्रुव तार्किक व्यञ्जकम्_Format_2 = fmtp && fmtp->व्यञ्जकम्_Format_2();
    तार्किक needsScope = व्यञ्जकम्_Format_2;
    तार्किक needsTimescale = व्यञ्जकम्_Format_2;
    पूर्णाङ्क format_तर्काः = 0;
    यदि (!व्यञ्जकम्_Format_2) {
        // Check display and argument count to head-off later runtime issues
        तार्किक अन्तर्_Pct = असत्य;
        तार्किक ignore = असत्य;
        प्रति (वर्ण ch : vformat) {
            // UINFO(1, "Parse '" << *pos << "'  IP" << inPct << " List " << cvtToHex(elistp));
            यदि (!अन्तर्_Pct && ch == '%') {
                अन्तर्_Pct = सत्य;
                ignore = असत्य;
            } अन्यथा यदि (अन्तर्_Pct && (std::अङ्कः_वा(ch) || ch == '.' || ch == '-')) {
            } अन्यथा यदि (!अन्तर्_Pct) {  // Normal text
            } अन्यथा {  // Format character
                अन्तर्_Pct = असत्य;
                ch = std::tolower(ch);
                विकल्प (ch) {
                पक्ष '%': भङ्ग;
                पक्ष '*':
                    अन्तर्_Pct = सत्य;  // Get more digits
                    ignore = सत्य;
                    भङ्ग;
                पक्ष 'm': needsScope = सत्य; भङ्ग;
                पक्ष 't':
                    needsTimescale = सत्य;
                    ++format_तर्काः;
                    भङ्ग;
                स्वभाव:
                    यदि (!ignore && V3_सङ्ख्या::displayedFmtHasArg(ch, is_अवलोकनम्)) ++format_तर्काः;
                    भङ्ग;
                }
            }
        }
    }

    यदि (vformat.रिक्तानि() && VN_IS(nodep, Display))  // not fscanf etc, as they need to return value
        प्रत्यागम;  // NOP

    ध्रुव तार्किक isStmt = display_Emit_शीर्षकम्(nodep);

    यदि (व्यञ्जकम्_Format_2) {
        UASSERT_OBJ(exprsp, nodep, "Missing format expression");
        iterateConst(exprsp);
        emitDatap(exprsp);
        exprsp = exprsp->nextp();
    } अन्यथा {
        ofp()->putsQuoted(vformat);
    }

    पूर्णाङ्क व्यञ्जकम्_तर्काः = 0;
    प्रति (Ast_ग्रन्थिः* argp = exprsp; argp; argp = argp->nextp()) ++व्यञ्जकम्_तर्काः;
    यदि (VL_UNCOVERABLE(!व्यञ्जकम्_Format_2 && व्यञ्जकम्_तर्काः != format_तर्काः)) {  // LCOV_EXCL_START
        // expectDisplay() checks this first, so probably internal error if found here
        nodep->v3error("Internal: Missing or extra arguments for $display-like format, "
                       << " expression had " << व्यञ्जकम्_तर्काः << ", format had " << format_तर्काः);
    }  // LCOV_EXCL_STOP

    // argc for error check. Also MSVC++ requires va_args to not be off a reference
    पूर्णाङ्क argc = 0;
    यदि (needsScope) ++argc;
    यदि (needsTimescale) ++argc;
    प्रति (Ast_ग्रन्थिः* argp = exprsp; argp; argp = argp->nextp()) {
        ++argc;
        ध्रुव AstSFormatArg* ध्रुव fargp = VN_CAST(argp, SFormatArg);
        यदि (fargp && fargp->formatAttr().isEnum()) ++argc;  // Additional name argument
    }
    ofp()->पङ्क्तिं_लिख("," + std::गन्तव्यम्_string(argc));

    यदि (needsScope) {
        AstScopeName* ध्रुव scopenamep = fmtp ? fmtp->scopeNamep() : शून्यसूचकः;
        UASSERT_OBJ(scopenamep, nodep, "Display with %m but no AstScopeName");
        ofp()->पङ्क्तिं_लिख(", '"s + VFormatAttr{VFormatAttr::SCOPE}.ascii() + "'");
        यदि (scopenamep->selfRelative()) {
            // vlNamep is catName(name(), instance), so starts with '.' when the model is unnamed
            ध्रुव string namep = string{m_useSelfForThis ? "vlSelf" : "this"} + "->vlNamep";
            ofp()->पङ्क्तिं_लिख(",(vlSymsp->name()[0] ? " + namep + " : " + namep + " + 1),");
            ofp()->putsQuoted(scopenamep->scopeAttr());
        } अन्यथा {
            ofp()->पङ्क्तिं_लिख(",vlSymsp->name(),");
            ofp()->putsQuoted(scopenamep->scopePrettySymName());
        }
    }

    यदि (needsTimescale) {
        VTimescale timeunit = VTimescale::NONE;
        यदि (ध्रुव AstDisplay* ध्रुव anodep = VN_CAST(nodep, Display)) {
            timeunit = anodep->fmtp()->timeunit();
        } अन्यथा यदि (ध्रुव AstSFormat* ध्रुव anodep = VN_CAST(nodep, SFormat)) {
            timeunit = anodep->fmtp()->timeunit();
        } अन्यथा यदि (ध्रुव AstSScanF* ध्रुव anodep = VN_CAST(nodep, SScanF)) {
            timeunit = anodep->timeunit();
        } अन्यथा यदि (ध्रुव AstSFormatF* ध्रुव anodep = VN_CAST(nodep, SFormatF)) {
            timeunit = anodep->timeunit();
        }
        UASSERT_OBJ(!timeunit.isNone(), nodep,
                    "Use of %t must be under AstDisplay, AstSFormat, or AstSFormatF, or "
                    "SScanF, and timeunit set");
        ofp()->पङ्क्तिं_लिख(", '"s + VFormatAttr{VFormatAttr::TIMEUNIT}.ascii() + "'");
        ofp()->पङ्क्तिं_लिख(","s + cvt_गन्तव्यम्_Str((पूर्णाङ्क)timeunit.power_सम्बन्धि_Ten()));
    }

    // Arguments
    प्रति (Ast_ग्रन्थिः* argp = exprsp; argp; argp = argp->nextp()) {
        ofp()->indentInc();
        ofp()->putbs("");
        AstSFormatArg* ध्रुव fargp = VN_CAST(argp, SFormatArg);
        Ast_ग्रन्थिः* ध्रुव subargp = fargp ? fargp->exprp() : argp;
        ध्रुव VFormatAttr formatAttr = AstSFormatArg::formatAttrDefauled(fargp, subargp->dtypep());
        पङ्क्तिं_लिख(", '"s + formatAttr.ascii() + '\'');
        यदि (formatAttr.isSigned() || formatAttr.isUnsigned() || formatAttr.isEnum())
            पङ्क्तिं_लिख("," + cvt_गन्तव्यम्_Str(subargp->विस्तारः_Min()));
        ध्रुव तार्किक addrof = is_अवलोकनम् || formatAttr.isString() || formatAttr.isComplex();
        ध्रुव तार्किक wideEnum = formatAttr.isEnum() && subargp->isWide();
        पङ्क्तिं_लिख(",");
        यदि (wideEnum) पङ्क्तिं_लिख("static_cast<const EData*>(");
        यदि (addrof) पङ्क्तिं_लिख("&(");
        यदि (VN_IS(subargp, प्रवाहः_R))
            emit_प्रवाहः_R(
                VN_CAST(subargp, प्रवाहः_R),
                nodep);  // This has to be done here because streamR doesn't know what it returns
        अन्यथा { iterateConst(subargp); }
        यदि (addrof) पङ्क्तिं_लिख(")");
        यदि (!addrof) emitDatap(argp);
        यदि (wideEnum) पङ्क्तिं_लिख(")");
        यदि (formatAttr.isEnum()) {
            पङ्क्तिं_लिख(", '"s + VFormatAttr{VFormatAttr::STRING}.ascii() + "', &(");
            iterateConst(fargp->namep());
            पङ्क्तिं_लिख(")");
        }
        ofp()->indentDec();
    }

    // End
    यदि (isStmt) {
        पङ्क्तिं_लिख(");\n");
    } अन्यथा {
        पङ्क्तिं_लिख(") ");
    }
}

शून्य EmitCFunc::emit_CCall_तर्काः(ध्रुव Ast_ग्रन्थिः_CCall* nodep, ध्रुव string& selfPointer,
                              तार्किक अन्तर्_Process) {
    putns(nodep, "(");
    तार्किक comma = असत्य;
    यदि (nodep->funcp()->isLoose() && !nodep->funcp()->isStatic()) {
        UASSERT_OBJ(!selfPointer.रिक्तानि(), nodep, "Call to loose method without self pointer");
        पङ्क्तिं_लिख(selfPointer);
        comma = सत्य;
    }
    यदि (nodep->funcp()->needProcess()) {
        यदि (comma) पङ्क्तिं_लिख(", ");
        यदि (VN_IS(nodep->backp(), CAwait) || !nodep->funcp()->isCoroutine()) {
            पङ्क्तिं_लिख("vlProcess");
        } अन्यथा यदि (अन्तर्_Process) {
            पङ्क्तिं_लिख("std::make_shared<VlProcess>(vlProcess)");
        } अन्यथा {
            पङ्क्तिं_लिख("std::make_shared<VlProcess>()");
        }
        comma = सत्य;
    }
    यदि (!nodep->argTypes().रिक्तानि()) {
        यदि (comma) पङ्क्तिं_लिख(", ");
        पङ्क्तिं_लिख(nodep->argTypes());
        comma = सत्य;
    }
    put_Comma_Iterate_अग्र(nodep->argsp(), comma);
    पङ्क्तिं_लिख(")");
}

std::string EmitCFunc::dereferenceString(ध्रुव std::string& pointer) ध्रुव {
    यदि (pointer[0] == '(' && pointer[1] == '&') {
        // remove "address of" followed by immediate dereference
        // Note: this relies on only the form '(&OBJECT)' being used by Verilator
        प्रत्यागम pointer.substr(2, pointer.दीर्घता_2() - 3) + '.';
    } अन्यथा {
        यदि (pointer == "vlSelf" && m_usevlSelfRef) {
            प्रत्यागम "vlSelfRef.";
        } अन्यथा {
            प्रत्यागम pointer + "->";
        }
    }
}
शून्य EmitCFunc::emitDereference(Ast_ग्रन्थिः* nodep, ध्रुव string& pointer) {
    putns(nodep, dereferenceString(pointer));
}

शून्य EmitCFunc::emitCvtPackStr(Ast_ग्रन्थिः* nodep) {
    यदि (ध्रुव AstConst* ध्रुव constp = VN_CAST(nodep, Const)) {
        emitConstantString(constp);
    } अन्यथा यदि (VN_IS(nodep->dtypep(), प्रवाहः_DType)) {
        putnbs(nodep, "VL_CVT_PACK_STR_ND(");
        iterate_And_अग्र_Const_Null(nodep);
        पङ्क्तिं_लिख(")");
    } अन्यथा {
        putnbs(nodep, "VL_CVT_PACK_STR_N");
        emitIQW(nodep);
        पङ्क्तिं_लिख("(");
        यदि (nodep->isWide()) {
            // Note argument width, not node width (which is always 32)
            पङ्क्तिं_लिख(cvt_गन्तव्यम्_Str(nodep->विस्तारः_शब्दाः()));
            पङ्क्तिं_लिख(", ");
        }
        iterate_And_अग्र_Const_Null(nodep);
        पङ्क्तिं_लिख(")");
    }
}

शून्य EmitCFunc::emitConstant(AstConst* nodep) {
    // Put out constant set to the specified variable, or given variable in a string
    ध्रुव V3_सङ्ख्या& अङ्कः = nodep->अङ्कः();
    यदि (अङ्कः.isFourState()) {
        nodep->v3warn(E_UNSUPPORTED, "Unsupported: 4-state numbers in this context");
        प्रत्यागम;
    }
    putns(nodep, अङ्कः.emitC());
}

शून्य EmitCFunc::emitConstantString(ध्रुव AstConst* nodep) {
    // Const might be a Verilog array-type string, but need to always output std::string
    putnbs(nodep, "std::string{");
    ध्रुव string str = nodep->अङ्कः().गन्तव्यम्_String();
    यदि (!str.रिक्तानि()) putsQuoted(str);
    पङ्क्तिं_लिख("}");
}

शून्य EmitCFunc::emitVarReset(ध्रुव string& उपसर्गः, AstVar* varp, तार्किक constructing) {
    // 'constructing' indicates that the object was just constructed, so if it is a string or
    // something that starts off clear already, no need to clear it again
    Ast_ग्रन्थिः_DType* ध्रुव dtypep = varp->dtypep()->skipRefp();
    ध्रुव string vlSelf = VSelfPointerText::replaceThis(m_useSelfForThis, "this->");
    ध्रुव string varNameProtected
        = ((VN_IS(m_modp, Class) || varp->isFuncLocal()) || !उपसर्गः.रिक्तानि())
              ? varName(varp)
              : vlSelf + varName(varp);
    ध्रुव string new_उपसर्गः = उपसर्गः + varNameProtected;
    यदि (varp->isIO() && m_modp->is_शिखरम्() && optSystemC()) {
        // System C top I/O doesn't need loading, as the lower level subinst code does it.}
    } अन्यथा यदि (varp->isParam()) {
        UASSERT_OBJ(varp->valuep(), varp, "No init for a param?");
        // If a simple CONST value we initialize it using an enum
        // If an ARRAYINIT we initialize it using an initial block similar to a signal
        // puts("// parameter "+varp->nameProtect()+" = "+varp->valuep()->name()+"\n");
    } अन्यथा {
        putns(varp,
              emitVarResetRecurse(varp, constructing, new_उपसर्गः, dtypep, 0, "", varp->valuep()));
    }
}

string EmitCFunc::emitVarResetRecurse(ध्रुव AstVar* varp, तार्किक constructing,
                                      ध्रुव string& varNameProtected, Ast_ग्रन्थिः_DType* dtypep,
                                      पूर्णाङ्क गभीरता, ध्रुव string& suffix, ध्रुव Ast_ग्रन्थिः* valuep) {
    dtypep = dtypep->skipRefp();
    AstBasicDType* ध्रुव basicp = dtypep->basicp();
    // Returns string to do resetting, empty to do nothing (which caller should handle)
    यदि (AstAssocArrayDType* ध्रुव adtypep = VN_CAST(dtypep, AssocArrayDType)) {
        // Access std::array as C array
        ध्रुव string pre = constructing ? "" : varNameProtected + suffix + ".clear();\n";
        प्रत्यागम pre
               + emitVarResetRecurse(varp, constructing, varNameProtected, adtypep->subDTypep(),
                                     गभीरता + 1, suffix + ".atDefault()", शून्यसूचकः);
    } अन्यथा यदि (AstWildcardArrayDType* ध्रुव adtypep = VN_CAST(dtypep, WildcardArrayDType)) {
        // Access std::array as C array
        ध्रुव string pre = constructing ? "" : varNameProtected + suffix + ".clear();\n";
        प्रत्यागम pre
               + emitVarResetRecurse(varp, constructing, varNameProtected, adtypep->subDTypep(),
                                     गभीरता + 1, suffix + ".atDefault()", शून्यसूचकः);
    } अन्यथा यदि (VN_IS(dtypep, CDType)) {
        प्रत्यागम "";  // Constructor does it
    } अन्यथा यदि (VN_IS(dtypep, CoverCrossDType) || VN_IS(dtypep, CoverpointDType)) {
        प्रत्यागम "";  // Covergroup constructor creates the runtime and assigns the pointer
    } अन्यथा यदि (ध्रुव AstClassRefDType* ध्रुव adtypep = VN_CAST(dtypep, ClassRefDType)) {
        प्रत्यागम adtypep->rawPointer() ? varNameProtected + suffix + " = nullptr;\n" : "";
    } अन्यथा यदि (VN_IS(dtypep, IfaceRefDType)) {
        प्रत्यागम varNameProtected + suffix + " = nullptr;\n";
    } अन्यथा यदि (ध्रुव AstDynArrayDType* ध्रुव adtypep = VN_CAST(dtypep, DynArrayDType)) {
        ध्रुव string pre = constructing ? "" : varNameProtected + suffix + ".clear();\n";
        प्रत्यागम pre
               + emitVarResetRecurse(varp, constructing, varNameProtected, adtypep->subDTypep(),
                                     गभीरता + 1, suffix + ".atDefault()", शून्यसूचकः);
    } अन्यथा यदि (ध्रुव AstQueueDType* ध्रुव adtypep = VN_CAST(dtypep, QueueDType)) {
        ध्रुव string pre = constructing ? "" : varNameProtected + suffix + ".clear();\n";
        प्रत्यागम pre
               + emitVarResetRecurse(varp, constructing, varNameProtected, adtypep->subDTypep(),
                                     गभीरता + 1, suffix + ".atDefault()", शून्यसूचकः);
    } अन्यथा यदि (VN_IS(dtypep, SampleQueueDType)) {
        प्रत्यागम "";
    } अन्यथा यदि (ध्रुव AstUnpackArrayDType* ध्रुव adtypep = VN_CAST(dtypep, UnpackArrayDType)) {
        UASSERT_OBJ(adtypep->hi() >= adtypep->lo(), varp,
                    "Should have swapped msb & lsb earlier.");
        ध्रुव Ast_ग्रन्थिः_DType* ध्रुव elementp = adtypep->subDTypep()->skipRefp();
        यदि (!varp->isFuncLocal() && elementp->isIntegralOrPacked()) {
            // Scoped reset has no array-index input or mutable RNG state. Initialize one
            // element, then fill in the optimized runtime even when this code is cold.
            ध्रुव string first
                = emitVarResetRecurse(varp, constructing, varNameProtected, adtypep->subDTypep(),
                                      गभीरता + 1, suffix + "[0]", शून्यसूचकः);
            ध्रुव string array = varNameProtected + suffix;
            प्रत्यागम first + "VL_UNPACKED_FILL(" + array + ".data(), sizeof(" + array + "[0]), "
                   + cvt_गन्तव्यम्_Str(adtypep->elementsConst()) + ");\n";
        }
        ध्रुव string ivar = "__Vi"s + cvt_गन्तव्यम्_Str(गभीरता);
        ध्रुव string pre = ("for (int " + ivar + " = " + cvt_गन्तव्यम्_Str(0) + "; " + ivar + " < "
                            + cvt_गन्तव्यम्_Str(adtypep->elementsConst()) + "; ++" + ivar + ") {\n");
        ध्रुव string अधः
            = emitVarResetRecurse(varp, constructing, varNameProtected, adtypep->subDTypep(),
                                  गभीरता + 1, suffix + "[" + ivar + "]", शून्यसूचकः);
        ध्रुव string post = "}\n";
        प्रत्यागम अधः.रिक्तानि() ? "" : pre + अधः + post;
    } अन्यथा यदि (VN_IS(dtypep, ग्रन्थिः_UOr_Struct_DType) && !VN_AS(dtypep, ग्रन्थिः_UOr_Struct_DType)->packed()) {
        ध्रुव Ast_ग्रन्थिः_UOr_Struct_DType* ध्रुव sdtypep = VN_AS(dtypep, ग्रन्थिः_UOr_Struct_DType);
        string literal;
        प्रति (ध्रुव AstMemberDType* itemp = sdtypep->membersp(); itemp;
             itemp = VN_AS(itemp->nextp(), MemberDType)) {
            ध्रुव std::string पङ्क्तिः = emitVarResetRecurse(
                varp, constructing, varNameProtected + suffix + "." + itemp->nameProtect(),
                itemp->dtypep(), गभीरता + 1, "", itemp->valuep());
            यदि (!पङ्क्तिः.रिक्तानि()) literal += पङ्क्तिः;
        }
        प्रत्यागम literal;
    } अन्यथा यदि (basicp && basicp->keyword() == VBasicDTypeKwd::STRING) {
        यदि (constructing) प्रत्यागम "";  // String's constructor deals with it
        प्रत्यागम varNameProtected + suffix + ".clear();\n";
    } अन्यथा यदि (basicp && basicp->isForkSync()) {
        प्रत्यागम "";
    } अन्यथा यदि (basicp && basicp->isProcessRef()) {
        प्रत्यागम "";
    } अन्यथा यदि (basicp && basicp->isDelayScheduler()) {
        प्रत्यागम "";
    } अन्यथा यदि (basicp && basicp->isTriggerScheduler()) {
        प्रत्यागम "";
    } अन्यथा यदि (basicp && basicp->isDynamicTriggerScheduler()) {
        प्रत्यागम "";
    } अन्यथा यदि (basicp && (basicp->isRandomGenerator() || basicp->isStdRandomGenerator())) {
        प्रत्यागम "";
    } अन्यथा यदि (basicp && basicp->isCovergroupInstHandle()) {
        // The handle's own constructor deals with it.
        प्रत्यागम "";
    } अन्यथा यदि (basicp && (basicp->isEvent())) {
        प्रत्यागम "VlAssignableEvent{};\n";
    } अन्यथा यदि (basicp) {
        ध्रुव तार्किक zeroit
            = (varp->attrFileDescr()  // Zero so we don't do file IO if never $fopen
               || varp->isFuncLocal()  // Randomization too slow
               || (basicp && basicp->isZeroInit())
               || (v3Global.opt.underlineZero() && !varp->name().रिक्तानि() && varp->name()[0] == '_')
               || (varp->varType().is_अस्थायी() && !varp->isXTemp())
               || (varp->isXTemp()
                       ? (v3Global.opt.x_नियोजय() != "unique")
                       : (v3Global.opt.xInitial() == "fast" || v3Global.opt.xInitial() == "0")));
        ध्रुव तार्किक slow = !varp->isFuncLocal() && !varp->isClassMember();
        विभज_आकार_Inc(1);
        यदि (dtypep->isWide()) {  // Handle unpacked; not basicp->isWide
            string बहिर्गमनम्;
            यदि (valuep) {
                ध्रुव AstConst* ध्रुव constp = VN_AS(valuep, Const);
                UASSERT_OBJ(constp, varp, "non-const initializer for variable");
                प्रति (पूर्णाङ्क w = 0; w < dtypep->विस्तारः_शब्दाः(); ++w) {
                    बहिर्गमनम् += varNameProtected + suffix + "[" + cvt_गन्तव्यम्_Str(w) + "] = ";
                    बहिर्गमनम् += cvt_गन्तव्यम्_Str(constp->अङ्कः().edataWord(w)) + "U;\n";
                }
            } अन्यथा {
                बहिर्गमनम् += zeroit ? (slow ? "VL_ZERO_RESET_W(" : "VL_ZERO_W(")
                              : (varp->isXTemp() ? "VL_SCOPED_RAND_RESET_ASSIGN_W("
                                                 : "VL_SCOPED_RAND_RESET_W(");
                बहिर्गमनम् += cvt_गन्तव्यम्_Str(dtypep->विस्तारः_Min());
                बहिर्गमनम् += ", " + varNameProtected + suffix;
                यदि (!zeroit) {
                    emitVarResetScopeHash();
                    ध्रुव uint64_t salt = VString::hashMurmur(varp->prettyName());
                    बहिर्गमनम् += ", ";
                    बहिर्गमनम् += m_classOrPackage ? m_classOrPackageHash : "__VscopeHash";
                    बहिर्गमनम् += ", ";
                    बहिर्गमनम् += std::गन्तव्यम्_string(salt);
                    बहिर्गमनम् += "ull";
                }
                बहिर्गमनम् += ");\n";
            }
            प्रत्यागम बहिर्गमनम्;
        } अन्यथा {
            string बहिर्गमनम् = varNameProtected + suffix;
            यदि (valuep) {
                बहिर्गमनम् += " = ";
                // TODO cleanup code shared between here, V3EmitCConstInit.h,
                // EmitCFunc::emitVarReset, EmitCFunc::emitConstant
                ध्रुव AstConst* ध्रुव constp = VN_AS(valuep, Const);
                UASSERT_OBJ(constp, varp, "non-const initializer for variable");
                बहिर्गमनम् += cvt_गन्तव्यम्_Str(constp->अङ्कः().edataWord(0)) + "U;\n";
                बहिर्गमनम् += ";\n";
            } अन्यथा यदि (zeroit) {
                बहिर्गमनम् += " = 0;\n";
            } अन्यथा {
                emitVarResetScopeHash();
                ध्रुव uint64_t salt = VString::hashMurmur(varp->prettyName());
                बहिर्गमनम् += " = VL_SCOPED_RAND_RESET_";
                यदि (varp->isXTemp()) बहिर्गमनम् += "ASSIGN_";
                बहिर्गमनम् += dtypep->charIQWN();
                बहिर्गमनम् += "(" + cvt_गन्तव्यम्_Str(dtypep->विस्तारः_Min()) + ", "
                       + (m_classOrPackage ? m_classOrPackageHash : "__VscopeHash") + ", "
                       + std::गन्तव्यम्_string(salt) + "ull);\n";
            }
            प्रत्यागम बहिर्गमनम्;
        }
    } अन्यथा {  // LCOV_EXCL_BR_LINE
        v3fatalSrc("Unknown node type in reset generator: " << varp->prettyTypeName());
    }
    प्रत्यागम "";
}

शून्य EmitCFunc::emitVarResetScopeHash() {
    यदि (VL_LIKELY(m_createdScopeHash)) { प्रत्यागम; }
    यदि (m_classOrPackage) {
        m_classOrPackageHash
            = std::गन्तव्यम्_string(VString::hashMurmur(m_classOrPackage->name())) + "ULL";
    } अन्यथा {
        पङ्क्तिं_लिख(string("const uint64_t __VscopeHash = VL_MURMUR64_HASH(")
             + (m_useSelfForThis ? "vlSelf" : "this") + "->vlNamep);\n");
    }
    m_createdScopeHash = सत्य;
}
