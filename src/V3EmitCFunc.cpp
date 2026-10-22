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

#અન્તર્ભાવ "V3PchAstMT.h"

#અન્તર્ભાવ "V3EmitCFunc.h"

#અન્તર્ભાવ <map>
#અન્તર્ભાવ <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// EmitCFunc

તાર્કિક EmitCFunc::emitSimpleOk(Ast_ગાંઠ_પદાવલિ* nodep) {
    // Can we put out a simple (A + B) instead of VL_ADD_III(A,B)?
    જો (nodep->emitSimpleOperator() == "") પાછા ખોટું;
    જો (nodep->isWide()) પાછા ખોટું;
    જો (nodep->op1p() && nodep->op1p()->isWide()) પાછા ખોટું;
    જો (nodep->op2p() && nodep->op2p()->isWide()) પાછા ખોટું;
    જો (nodep->op3p() && nodep->op3p()->isWide()) પાછા ખોટું;
    પાછા સાચું;
}

શૂન્ય EmitCFunc::emitOpName(Ast_ગાંઠ* nodep, અચલ string& format, Ast_ગાંઠ* lhsp, Ast_ગાંઠ* rhsp,
                           Ast_ગાંઠ* thsp) {
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

    તાર્કિક needComma = ખોટું;
    તાર્કિક usesQueue = ખોટું;
    string nextComma;
    auto commaOut = [&out, &nextComma]() {
        જો (!nextComma.empty()) {
            out += nextComma;
            nextComma = "";
        }
    };

    auto putOut = [આ, &out]() {
        જો (!out.empty()) puts(out);
        out = "";
    };

    માટે (string::const_iterator pos = format.begin(); pos != format.end(); ++pos) {
        જો (pos[0] == ',') {
            // Remember we need to add one, but don't do yet to avoid ",)"
            જો (needComma) {
                જો (pos[1] == ' ') {
                    nextComma = ", ";
                } નહીંતર {
                    nextComma = ",";
                }
                needComma = ખોટું;
            }
            જો (pos[1] == ' ') ++pos;  // Must do even if no nextComma
        } નહીંતર જો (pos[0] == '%') {
            ++pos;
            તાર્કિક detail = ખોટું;
            Ast_ગાંઠ* detailp = nullptr;
            પસંદગી (pos[0]) {
            સ્થિતિ '%': out += '%'; તોડો;
            સ્થિતિ 'k':
                putOut();
                putbs("");
                તોડો;
            સ્થિતિ 'n':
                detail = સાચું;
                detailp = nodep;
                તોડો;
            સ્થિતિ 'l':
                detail = સાચું;
                detailp = lhsp;
                તોડો;
            સ્થિતિ 'r':
                detail = સાચું;
                detailp = rhsp;
                તોડો;
            સ્થિતિ 't':
                detail = સાચું;
                detailp = thsp;
                તોડો;
            સ્થિતિ 'P':
                જો (nodep->isWide() && !usesQueue) {
                    UASSERT_OBJ(m_wideTempRefp, nodep,
                                "Wide op " << nodep->prettyTypeName()
                                           << " w/ no temp, perhaps missing op in V3EmitC?");
                    commaOut();
                    putOut();
                    જો (!m_wideTempRefp->selfPointer().isEmpty()) {
                        emitDereference(m_wideTempRefp,
                                        m_wideTempRefp->selfPointerProtect(m_useSelfForThis));
                    }
                    out += varName(m_wideTempRefp->varp());
                    m_wideTempRefp = nullptr;
                    needComma = સાચું;
                } નહીંતર જો (usesQueue) {
                    commaOut();
                    putOut();
                    iterateAndNextConstNull(nodep->backp()->op2p());
                    needComma = સાચું;
                }

                તોડો;
            મૂળભૂત: nodep->v3fatalSrc("Unknown emitOperator format code: %" << pos[0]); તોડો;
            }
            જો (detail) {
                // Get next letter of %[nlrt]
                ++pos;
                પસંદગી (pos[0]) {
                સ્થિતિ 'q':
                    putOut();
                    // If we are assigning this to a queue we need to get the return type
                    જો (VN_IS(detailp->backp(), Assign)
                        && VN_IS(detailp->backp()->op2p()->dtypep()->skipRefp(), QueueDType)) {
                        puts("R");  // R for queue
                        usesQueue = સાચું;
                    } નહીંતર જો (VN_IS(detailp->dtypep()->skipRefp(), QueueDType)
                               || VN_IS(detailp->dtypep()->skipRefp(), StreamDType)) {
                        puts("R");  // R for queue
                    } નહીંતર {
                        emitIQW(detailp);
                    }
                    તોડો;
                સ્થિતિ 'w':
                    commaOut();
                    out += cvtToStr(detailp->widthMin());
                    needComma = સાચું;
                    તોડો;
                સ્થિતિ 'W':
                    જો (lhsp->isWide()) {
                        commaOut();
                        out += cvtToStr(lhsp->width_શબ્દો());
                        needComma = સાચું;
                    } નહીંતર જો (VN_IS(lhsp, StreamR)) {
                        commaOut();
                        out += cvtToStr(rhsp->width_શબ્દો());
                        needComma = સાચું;
                    }
                    તોડો;
                સ્થિતિ 'i':
                    commaOut();
                    UASSERT_OBJ(detailp, nodep, "emitOperator() references undef node");
                    putOut();
                    iterateAndNextConstNull(detailp);
                    needComma = સાચું;
                    તોડો;
                મૂળભૂત:
                    nodep->v3fatalSrc("Unknown emitOperator format code: %[nlrt]" << pos[0]);
                    તોડો;
                }
            }
        } નહીંતર જો (pos[0] == ')') {
            nextComma = "";
            out += ')';
        } નહીંતર જો (pos[0] == '(') {
            commaOut();
            needComma = ખોટું;
            out += '(';
        } નહીંતર {
            // Normal text
            જો (std::isalnum(pos[0])) needComma = સાચું;
            commaOut();
            out += pos[0];
        }
    }
    putOut();
}

તાર્કિક EmitCFunc::displayEmitHeader(Ast_ગાંઠ* nodep) {
    તાર્કિક isStmt = ખોટું;
    જો (અચલ AstFScanF* અચલ dispp = VN_CAST(nodep, FScanF)) {
        isStmt = ખોટું;
        putns(nodep, "VL_FSCANF_INX(");
        iterateConst(dispp->filep());
        puts(",");
    } નહીંતર જો (અચલ AstSScanF* અચલ dispp = VN_CAST(nodep, SScanF)) {
        isStmt = ખોટું;
        putns(nodep, "VL_SSCANF_I");
        emitIQW(dispp->fromp());
        puts("NX(");
        puts(cvtToStr(dispp->fromp()->widthMin()));
        puts(",");
        iterateConst(dispp->fromp());
        puts(",");
    } નહીંતર જો (અચલ AstDisplay* અચલ dispp = VN_CAST(nodep, Display)) {
        isStmt = સાચું;
        જો (dispp->filep()) {
            putns(nodep, "VL_FWRITEF_NX(");
            iterateConst(dispp->filep());
            puts(",");
        } નહીંતર {
            putns(nodep, "VL_WRITEF_NX(");
        }
    } નહીંતર જો (અચલ AstSFormat* અચલ dispp = VN_CAST(nodep, SFormat)) {
        isStmt = સાચું;
        puts("VL_SFORMAT_NX(");
        જો (!dispp->lhsp()->dtypep()->isString()) {
            puts(cvtToStr(dispp->lhsp()->widthMin()));
            putbs(",");
        }
        iterateConst(dispp->lhsp());
        emitDatap(dispp->lhsp());
        putbs(",");
    } નહીંતર જો (VN_IS(nodep, SFormatF)) {
        isStmt = ખોટું;
        putns(nodep, "VL_SFORMATF_N_NX(");
    } નહીંતર {
        nodep->v3fatalSrc("Unknown displayEmit node type");
    }
    પાછા isStmt;
}

શૂન્ય EmitCFunc::display_ગાંઠ(Ast_ગાંઠ* nodep, AstSFormatF* fmtp,  // fmtp is nullptr for AstScan
                            અચલ string& vformat, Ast_ગાંઠ* exprsp, તાર્કિક isScan) {
    // Check format, if it exists
    અચલ તાર્કિક પદાવલિ_Format_2 = fmtp && fmtp->પદાવલિ_Format_2();
    તાર્કિક needsScope = પદાવલિ_Format_2;
    તાર્કિક needsTimescale = પદાવલિ_Format_2;
    પૂર્ણાંક formatArgs = 0;
    જો (!પદાવલિ_Format_2) {
        // Check display and argument count to head-off later runtime issues
        તાર્કિક inPct = ખોટું;
        તાર્કિક ignore = ખોટું;
        માટે (અક્ષર ch : vformat) {
            // UINFO(1, "Parse '" << *pos << "'  IP" << inPct << " List " << cvtToHex(elistp));
            જો (!inPct && ch == '%') {
                inPct = સાચું;
                ignore = ખોટું;
            } નહીંતર જો (inPct && (std::isdigit(ch) || ch == '.' || ch == '-')) {
            } નહીંતર જો (!inPct) {  // Normal text
            } નહીંતર {  // Format character
                inPct = ખોટું;
                ch = std::tolower(ch);
                પસંદગી (ch) {
                સ્થિતિ '%': તોડો;
                સ્થિતિ '*':
                    inPct = સાચું;  // Get more digits
                    ignore = સાચું;
                    તોડો;
                સ્થિતિ 'm': needsScope = સાચું; તોડો;
                સ્થિતિ 't':
                    needsTimescale = સાચું;
                    ++formatArgs;
                    તોડો;
                મૂળભૂત:
                    જો (!ignore && V3_સંખ્યા::displayedFmtHasArg(ch, isScan)) ++formatArgs;
                    તોડો;
                }
            }
        }
    }

    જો (vformat.empty() && VN_IS(nodep, Display))  // not fscanf etc, as they need to return value
        પાછા;  // NOP

    અચલ તાર્કિક isStmt = displayEmitHeader(nodep);

    જો (પદાવલિ_Format_2) {
        UASSERT_OBJ(exprsp, nodep, "Missing format expression");
        iterateConst(exprsp);
        emitDatap(exprsp);
        exprsp = exprsp->nextp();
    } નહીંતર {
        ofp()->putsQuoted(vformat);
    }

    પૂર્ણાંક પદાવલિ_Args = 0;
    માટે (Ast_ગાંઠ* argp = exprsp; argp; argp = argp->nextp()) ++પદાવલિ_Args;
    જો (VL_UNCOVERABLE(!પદાવલિ_Format_2 && પદાવલિ_Args != formatArgs)) {  // LCOV_EXCL_START
        // expectDisplay() checks this first, so probably internal error if found here
        nodep->v3error("Internal: Missing or extra arguments for $display-like format, "
                       << " expression had " << પદાવલિ_Args << ", format had " << formatArgs);
    }  // LCOV_EXCL_STOP

    // argc for error check. Also MSVC++ requires va_args to not be off a reference
    પૂર્ણાંક argc = 0;
    જો (needsScope) ++argc;
    જો (needsTimescale) ++argc;
    માટે (Ast_ગાંઠ* argp = exprsp; argp; argp = argp->nextp()) {
        ++argc;
        અચલ AstSFormatArg* અચલ fargp = VN_CAST(argp, SFormatArg);
        જો (fargp && fargp->formatAttr().isEnum()) ++argc;  // Additional name argument
    }
    ofp()->puts("," + std::to_string(argc));

    જો (needsScope) {
        AstScopeName* અચલ scopenamep = fmtp ? fmtp->scopeNamep() : nullptr;
        UASSERT_OBJ(scopenamep, nodep, "Display with %m but no AstScopeName");
        ofp()->puts(", '"s + VFormatAttr{VFormatAttr::SCOPE}.ascii() + "'");
        જો (scopenamep->selfRelative()) {
            // vlNamep is catName(name(), instance), so starts with '.' when the model is unnamed
            અચલ string namep = string{m_useSelfForThis ? "vlSelf" : "this"} + "->vlNamep";
            ofp()->puts(",(vlSymsp->name()[0] ? " + namep + " : " + namep + " + 1),");
            ofp()->putsQuoted(scopenamep->scopeAttr());
        } નહીંતર {
            ofp()->puts(",vlSymsp->name(),");
            ofp()->putsQuoted(scopenamep->scopePrettySymName());
        }
    }

    જો (needsTimescale) {
        VTimescale timeunit = VTimescale::NONE;
        જો (અચલ AstDisplay* અચલ anodep = VN_CAST(nodep, Display)) {
            timeunit = anodep->fmtp()->timeunit();
        } નહીંતર જો (અચલ AstSFormat* અચલ anodep = VN_CAST(nodep, SFormat)) {
            timeunit = anodep->fmtp()->timeunit();
        } નહીંતર જો (અચલ AstSScanF* અચલ anodep = VN_CAST(nodep, SScanF)) {
            timeunit = anodep->timeunit();
        } નહીંતર જો (અચલ AstSFormatF* અચલ anodep = VN_CAST(nodep, SFormatF)) {
            timeunit = anodep->timeunit();
        }
        UASSERT_OBJ(!timeunit.isNone(), nodep,
                    "Use of %t must be under AstDisplay, AstSFormat, or AstSFormatF, or "
                    "SScanF, and timeunit set");
        ofp()->puts(", '"s + VFormatAttr{VFormatAttr::TIMEUNIT}.ascii() + "'");
        ofp()->puts(","s + cvtToStr((પૂર્ણાંક)timeunit.powerOfTen()));
    }

    // Arguments
    માટે (Ast_ગાંઠ* argp = exprsp; argp; argp = argp->nextp()) {
        ofp()->indentInc();
        ofp()->putbs("");
        AstSFormatArg* અચલ fargp = VN_CAST(argp, SFormatArg);
        Ast_ગાંઠ* અચલ subargp = fargp ? fargp->exprp() : argp;
        અચલ VFormatAttr formatAttr = AstSFormatArg::formatAttrDefauled(fargp, subargp->dtypep());
        puts(", '"s + formatAttr.ascii() + '\'');
        જો (formatAttr.isSigned() || formatAttr.isUnsigned() || formatAttr.isEnum())
            puts("," + cvtToStr(subargp->widthMin()));
        અચલ તાર્કિક addrof = isScan || formatAttr.isString() || formatAttr.isComplex();
        અચલ તાર્કિક wideEnum = formatAttr.isEnum() && subargp->isWide();
        puts(",");
        જો (wideEnum) puts("static_cast<const EData*>(");
        જો (addrof) puts("&(");
        જો (VN_IS(subargp, StreamR))
            emitStreamR(
                VN_CAST(subargp, StreamR),
                nodep);  // This has to be done here because streamR doesn't know what it returns
        નહીંતર { iterateConst(subargp); }
        જો (addrof) puts(")");
        જો (!addrof) emitDatap(argp);
        જો (wideEnum) puts(")");
        જો (formatAttr.isEnum()) {
            puts(", '"s + VFormatAttr{VFormatAttr::STRING}.ascii() + "', &(");
            iterateConst(fargp->namep());
            puts(")");
        }
        ofp()->indentDec();
    }

    // End
    જો (isStmt) {
        puts(");\n");
    } નહીંતર {
        puts(") ");
    }
}

શૂન્ય EmitCFunc::emitCCallArgs(અચલ Ast_ગાંઠ_CCall* nodep, અચલ string& selfPointer,
                              તાર્કિક inProcess) {
    putns(nodep, "(");
    તાર્કિક comma = ખોટું;
    જો (nodep->funcp()->isLoose() && !nodep->funcp()->isStatic()) {
        UASSERT_OBJ(!selfPointer.empty(), nodep, "Call to loose method without self pointer");
        puts(selfPointer);
        comma = સાચું;
    }
    જો (nodep->funcp()->needProcess()) {
        જો (comma) puts(", ");
        જો (VN_IS(nodep->backp(), CAwait) || !nodep->funcp()->isCoroutine()) {
            puts("vlProcess");
        } નહીંતર જો (inProcess) {
            puts("std::make_shared<VlProcess>(vlProcess)");
        } નહીંતર {
            puts("std::make_shared<VlProcess>()");
        }
        comma = સાચું;
    }
    જો (!nodep->argTypes().empty()) {
        જો (comma) puts(", ");
        puts(nodep->argTypes());
        comma = સાચું;
    }
    putCommaIterateNext(nodep->argsp(), comma);
    puts(")");
}

std::string EmitCFunc::dereferenceString(અચલ std::string& pointer) અચલ {
    જો (pointer[0] == '(' && pointer[1] == '&') {
        // remove "address of" followed by immediate dereference
        // Note: this relies on only the form '(&OBJECT)' being used by Verilator
        પાછા pointer.substr(2, pointer.length() - 3) + '.';
    } નહીંતર {
        જો (pointer == "vlSelf" && m_usevlSelfRef) {
            પાછા "vlSelfRef.";
        } નહીંતર {
            પાછા pointer + "->";
        }
    }
}
શૂન્ય EmitCFunc::emitDereference(Ast_ગાંઠ* nodep, અચલ string& pointer) {
    putns(nodep, dereferenceString(pointer));
}

શૂન્ય EmitCFunc::emitCvtPackStr(Ast_ગાંઠ* nodep) {
    જો (અચલ AstConst* અચલ constp = VN_CAST(nodep, Const)) {
        emitConstantString(constp);
    } નહીંતર જો (VN_IS(nodep->dtypep(), StreamDType)) {
        putnbs(nodep, "VL_CVT_PACK_STR_ND(");
        iterateAndNextConstNull(nodep);
        puts(")");
    } નહીંતર {
        putnbs(nodep, "VL_CVT_PACK_STR_N");
        emitIQW(nodep);
        puts("(");
        જો (nodep->isWide()) {
            // Note argument width, not node width (which is always 32)
            puts(cvtToStr(nodep->width_શબ્દો()));
            puts(", ");
        }
        iterateAndNextConstNull(nodep);
        puts(")");
    }
}

શૂન્ય EmitCFunc::emitConstant(AstConst* nodep) {
    // Put out constant set to the specified variable, or given variable in a string
    અચલ V3_સંખ્યા& અંક = nodep->અંક();
    જો (અંક.isFourState()) {
        nodep->v3warn(E_UNSUPPORTED, "Unsupported: 4-state numbers in this context");
        પાછા;
    }
    putns(nodep, અંક.emitC());
}

શૂન્ય EmitCFunc::emitConstantString(અચલ AstConst* nodep) {
    // Const might be a Verilog array-type string, but need to always output std::string
    putnbs(nodep, "std::string{");
    અચલ string str = nodep->અંક().toString();
    જો (!str.empty()) putsQuoted(str);
    puts("}");
}

શૂન્ય EmitCFunc::emitVarReset(અચલ string& prefix, AstVar* varp, તાર્કિક constructing) {
    // 'constructing' indicates that the object was just constructed, so if it is a string or
    // something that starts off clear already, no need to clear it again
    Ast_ગાંઠ_DType* અચલ dtypep = varp->dtypep()->skipRefp();
    અચલ string vlSelf = VSelfPointerText::replaceThis(m_useSelfForThis, "this->");
    અચલ string varNameProtected
        = ((VN_IS(m_modp, Class) || varp->isFuncLocal()) || !prefix.empty())
              ? varName(varp)
              : vlSelf + varName(varp);
    અચલ string newPrefix = prefix + varNameProtected;
    જો (varp->isIO() && m_modp->is_ટોચ() && optSystemC()) {
        // System C top I/O doesn't need loading, as the lower level subinst code does it.}
    } નહીંતર જો (varp->isParam()) {
        UASSERT_OBJ(varp->valuep(), varp, "No init for a param?");
        // If a simple CONST value we initialize it using an enum
        // If an ARRAYINIT we initialize it using an initial block similar to a signal
        // puts("// parameter "+varp->nameProtect()+" = "+varp->valuep()->name()+"\n");
    } નહીંતર {
        putns(varp,
              emitVarResetRecurse(varp, constructing, newPrefix, dtypep, 0, "", varp->valuep()));
    }
}

string EmitCFunc::emitVarResetRecurse(અચલ AstVar* varp, તાર્કિક constructing,
                                      અચલ string& varNameProtected, Ast_ગાંઠ_DType* dtypep,
                                      પૂર્ણાંક depth, અચલ string& suffix, અચલ Ast_ગાંઠ* valuep) {
    dtypep = dtypep->skipRefp();
    AstBasicDType* અચલ basicp = dtypep->basicp();
    // Returns string to do resetting, empty to do nothing (which caller should handle)
    જો (AstAssocArrayDType* અચલ adtypep = VN_CAST(dtypep, AssocArrayDType)) {
        // Access std::array as C array
        અચલ string pre = constructing ? "" : varNameProtected + suffix + ".clear();\n";
        પાછા pre
               + emitVarResetRecurse(varp, constructing, varNameProtected, adtypep->subDTypep(),
                                     depth + 1, suffix + ".atDefault()", nullptr);
    } નહીંતર જો (AstWildcardArrayDType* અચલ adtypep = VN_CAST(dtypep, WildcardArrayDType)) {
        // Access std::array as C array
        અચલ string pre = constructing ? "" : varNameProtected + suffix + ".clear();\n";
        પાછા pre
               + emitVarResetRecurse(varp, constructing, varNameProtected, adtypep->subDTypep(),
                                     depth + 1, suffix + ".atDefault()", nullptr);
    } નહીંતર જો (VN_IS(dtypep, CDType)) {
        પાછા "";  // Constructor does it
    } નહીંતર જો (VN_IS(dtypep, CoverCrossDType) || VN_IS(dtypep, CoverpointDType)) {
        પાછા "";  // Covergroup constructor creates the runtime and assigns the pointer
    } નહીંતર જો (અચલ AstClassRefDType* અચલ adtypep = VN_CAST(dtypep, ClassRefDType)) {
        પાછા adtypep->rawPointer() ? varNameProtected + suffix + " = nullptr;\n" : "";
    } નહીંતર જો (VN_IS(dtypep, IfaceRefDType)) {
        પાછા varNameProtected + suffix + " = nullptr;\n";
    } નહીંતર જો (અચલ AstDynArrayDType* અચલ adtypep = VN_CAST(dtypep, DynArrayDType)) {
        અચલ string pre = constructing ? "" : varNameProtected + suffix + ".clear();\n";
        પાછા pre
               + emitVarResetRecurse(varp, constructing, varNameProtected, adtypep->subDTypep(),
                                     depth + 1, suffix + ".atDefault()", nullptr);
    } નહીંતર જો (અચલ AstQueueDType* અચલ adtypep = VN_CAST(dtypep, QueueDType)) {
        અચલ string pre = constructing ? "" : varNameProtected + suffix + ".clear();\n";
        પાછા pre
               + emitVarResetRecurse(varp, constructing, varNameProtected, adtypep->subDTypep(),
                                     depth + 1, suffix + ".atDefault()", nullptr);
    } નહીંતર જો (VN_IS(dtypep, SampleQueueDType)) {
        પાછા "";
    } નહીંતર જો (અચલ AstUnpackArrayDType* અચલ adtypep = VN_CAST(dtypep, UnpackArrayDType)) {
        UASSERT_OBJ(adtypep->hi() >= adtypep->lo(), varp,
                    "Should have swapped msb & lsb earlier.");
        અચલ Ast_ગાંઠ_DType* અચલ elementp = adtypep->subDTypep()->skipRefp();
        જો (!varp->isFuncLocal() && elementp->isIntegralOrPacked()) {
            // Scoped reset has no array-index input or mutable RNG state. Initialize one
            // element, then fill in the optimized runtime even when this code is cold.
            અચલ string first
                = emitVarResetRecurse(varp, constructing, varNameProtected, adtypep->subDTypep(),
                                      depth + 1, suffix + "[0]", nullptr);
            અચલ string array = varNameProtected + suffix;
            પાછા first + "VL_UNPACKED_FILL(" + array + ".data(), sizeof(" + array + "[0]), "
                   + cvtToStr(adtypep->elementsConst()) + ");\n";
        }
        અચલ string ivar = "__Vi"s + cvtToStr(depth);
        અચલ string pre = ("for (int " + ivar + " = " + cvtToStr(0) + "; " + ivar + " < "
                            + cvtToStr(adtypep->elementsConst()) + "; ++" + ivar + ") {\n");
        અચલ string below
            = emitVarResetRecurse(varp, constructing, varNameProtected, adtypep->subDTypep(),
                                  depth + 1, suffix + "[" + ivar + "]", nullptr);
        અચલ string post = "}\n";
        પાછા below.empty() ? "" : pre + below + post;
    } નહીંતર જો (VN_IS(dtypep, ગાંઠ_UOr_Struct_DType) && !VN_AS(dtypep, ગાંઠ_UOr_Struct_DType)->packed()) {
        અચલ Ast_ગાંઠ_UOr_Struct_DType* અચલ sdtypep = VN_AS(dtypep, ગાંઠ_UOr_Struct_DType);
        string literal;
        માટે (અચલ AstMemberDType* itemp = sdtypep->membersp(); itemp;
             itemp = VN_AS(itemp->nextp(), MemberDType)) {
            અચલ std::string લીટી = emitVarResetRecurse(
                varp, constructing, varNameProtected + suffix + "." + itemp->nameProtect(),
                itemp->dtypep(), depth + 1, "", itemp->valuep());
            જો (!લીટી.empty()) literal += લીટી;
        }
        પાછા literal;
    } નહીંતર જો (basicp && basicp->keyword() == VBasicDTypeKwd::STRING) {
        જો (constructing) પાછા "";  // String's constructor deals with it
        પાછા varNameProtected + suffix + ".clear();\n";
    } નહીંતર જો (basicp && basicp->isForkSync()) {
        પાછા "";
    } નહીંતર જો (basicp && basicp->isProcessRef()) {
        પાછા "";
    } નહીંતર જો (basicp && basicp->isDelayScheduler()) {
        પાછા "";
    } નહીંતર જો (basicp && basicp->isTriggerScheduler()) {
        પાછા "";
    } નહીંતર જો (basicp && basicp->isDynamicTriggerScheduler()) {
        પાછા "";
    } નહીંતર જો (basicp && (basicp->isRandomGenerator() || basicp->isStdRandomGenerator())) {
        પાછા "";
    } નહીંતર જો (basicp && basicp->isCovergroupInstHandle()) {
        // The handle's own constructor deals with it.
        પાછા "";
    } નહીંતર જો (basicp && (basicp->isEvent())) {
        પાછા "VlAssignableEvent{};\n";
    } નહીંતર જો (basicp) {
        અચલ તાર્કિક zeroit
            = (varp->attrFileDescr()  // Zero so we don't do file IO if never $fopen
               || varp->isFuncLocal()  // Randomization too slow
               || (basicp && basicp->isZeroInit())
               || (v3Global.opt.underlineZero() && !varp->name().empty() && varp->name()[0] == '_')
               || (varp->varType().isTemp() && !varp->isXTemp())
               || (varp->isXTemp()
                       ? (v3Global.opt.xAssign() != "unique")
                       : (v3Global.opt.xInitial() == "fast" || v3Global.opt.xInitial() == "0")));
        અચલ તાર્કિક slow = !varp->isFuncLocal() && !varp->isClassMember();
        splitSizeInc(1);
        જો (dtypep->isWide()) {  // Handle unpacked; not basicp->isWide
            string out;
            જો (valuep) {
                અચલ AstConst* અચલ constp = VN_AS(valuep, Const);
                UASSERT_OBJ(constp, varp, "non-const initializer for variable");
                માટે (પૂર્ણાંક w = 0; w < dtypep->width_શબ્દો(); ++w) {
                    out += varNameProtected + suffix + "[" + cvtToStr(w) + "] = ";
                    out += cvtToStr(constp->અંક().edataWord(w)) + "U;\n";
                }
            } નહીંતર {
                out += zeroit ? (slow ? "VL_ZERO_RESET_W(" : "VL_ZERO_W(")
                              : (varp->isXTemp() ? "VL_SCOPED_RAND_RESET_ASSIGN_W("
                                                 : "VL_SCOPED_RAND_RESET_W(");
                out += cvtToStr(dtypep->widthMin());
                out += ", " + varNameProtected + suffix;
                જો (!zeroit) {
                    emitVarResetScopeHash();
                    અચલ uint64_t salt = VString::hashMurmur(varp->prettyName());
                    out += ", ";
                    out += m_classOrPackage ? m_classOrPackageHash : "__VscopeHash";
                    out += ", ";
                    out += std::to_string(salt);
                    out += "ull";
                }
                out += ");\n";
            }
            પાછા out;
        } નહીંતર {
            string out = varNameProtected + suffix;
            જો (valuep) {
                out += " = ";
                // TODO cleanup code shared between here, V3EmitCConstInit.h,
                // EmitCFunc::emitVarReset, EmitCFunc::emitConstant
                અચલ AstConst* અચલ constp = VN_AS(valuep, Const);
                UASSERT_OBJ(constp, varp, "non-const initializer for variable");
                out += cvtToStr(constp->અંક().edataWord(0)) + "U;\n";
                out += ";\n";
            } નહીંતર જો (zeroit) {
                out += " = 0;\n";
            } નહીંતર {
                emitVarResetScopeHash();
                અચલ uint64_t salt = VString::hashMurmur(varp->prettyName());
                out += " = VL_SCOPED_RAND_RESET_";
                જો (varp->isXTemp()) out += "ASSIGN_";
                out += dtypep->charIQWN();
                out += "(" + cvtToStr(dtypep->widthMin()) + ", "
                       + (m_classOrPackage ? m_classOrPackageHash : "__VscopeHash") + ", "
                       + std::to_string(salt) + "ull);\n";
            }
            પાછા out;
        }
    } નહીંતર {  // LCOV_EXCL_BR_LINE
        v3fatalSrc("Unknown node type in reset generator: " << varp->prettyTypeName());
    }
    પાછા "";
}

શૂન્ય EmitCFunc::emitVarResetScopeHash() {
    જો (VL_LIKELY(m_createdScopeHash)) { પાછા; }
    જો (m_classOrPackage) {
        m_classOrPackageHash
            = std::to_string(VString::hashMurmur(m_classOrPackage->name())) + "ULL";
    } નહીંતર {
        puts(string("const uint64_t __VscopeHash = VL_MURMUR64_HASH(")
             + (m_useSelfForThis ? "vlSelf" : "this") + "->vlNamep);\n");
    }
    m_createdScopeHash = સાચું;
}
