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

#include "V3PchAstMT.h"

#include "V3EmitCFunc.h"

#include <map>
#include <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// EmitCFunc

bool EmitCFunc::emitSimpleOk(AstNodeExpr* nodep) {
    // Can we put out a simple (A + B) instead of VL_ADD_III(A,B)?
    ᤀᤡ᤺ (nodep->emitSimpleOperator() == "") ᤀᤡᤴ false;
    ᤀᤡ᤺ (nodep->isWide()) ᤀᤡᤴ false;
    ᤀᤡ᤺ (nodep->op1p() && nodep->op1p()->isWide()) ᤀᤡᤴ false;
    ᤀᤡ᤺ (nodep->op2p() && nodep->op2p()->isWide()) ᤀᤡᤴ false;
    ᤀᤡ᤺ (nodep->op3p() && nodep->op3p()->isWide()) ᤀᤡᤴ false;
    ᤀᤡᤴ true;
}

void EmitCFunc::emitOpName(AstNode* nodep, const string& format, AstNode* lhsp, AstNode* rhsp,
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

    bool needComma = false;
    bool usesQueue = false;
    string nextComma;
    auto commaOut = [&out, &nextComma]() {
        ᤀᤡ᤺ (!nextComma.empty()) {
            out += nextComma;
            nextComma = "";
        }
    };

    auto putOut = [this, &out]() {
        ᤀᤡ᤺ (!out.empty()) puts(out);
        out = "";
    };

    for (string::const_iterator pos = format.begin(); pos != format.end(); ++pos) {
        ᤀᤡ᤺ (pos[0] == ',') {
            // Remember we need to add one, but don't do yet to avoid ",)"
            ᤀᤡ᤺ (needComma) {
                ᤀᤡ᤺ (pos[1] == ' ') {
                    nextComma = ", ";
                } ᤀᤡ᤺ᤆᤡ {
                    nextComma = ",";
                }
                needComma = false;
            }
            ᤀᤡ᤺ (pos[1] == ' ') ++pos;  // Must do even if no nextComma
        } ᤀᤡ᤺ᤆᤡ ᤀᤡ᤺ (pos[0] == '%') {
            ++pos;
            bool detail = false;
            AstNode* detailp = nullptr;
            switch (pos[0]) {
            pak_sa '%': out += '%'; bha_nga;
            pak_sa 'k':
                putOut();
                putbs("");
                bha_nga;
            pak_sa 'n':
                detail = true;
                detailp = nodep;
                bha_nga;
            pak_sa 'l':
                detail = true;
                detailp = lhsp;
                bha_nga;
            pak_sa 'r':
                detail = true;
                detailp = rhsp;
                bha_nga;
            pak_sa 't':
                detail = true;
                detailp = thsp;
                bha_nga;
            pak_sa 'P':
                ᤀᤡ᤺ (nodep->isWide() && !usesQueue) {
                    UASSERT_OBJ(m_wideTempRefp, nodep,
                                "Wide op " << nodep->prettyTypeName()
                                           << " w/ no temp, perhaps missing op in V3EmitC?");
                    commaOut();
                    putOut();
                    ᤀᤡ᤺ (!m_wideTempRefp->selfPointer().isEmpty()) {
                        emitDereference(m_wideTempRefp,
                                        m_wideTempRefp->selfPointerProtect(m_useSelfForThis));
                    }
                    out += varName(m_wideTempRefp->varp());
                    m_wideTempRefp = nullptr;
                    needComma = true;
                } ᤀᤡ᤺ᤆᤡ ᤀᤡ᤺ (usesQueue) {
                    commaOut();
                    putOut();
                    iterateAndNextConstNull(nodep->backp()->op2p());
                    needComma = true;
                }

                bha_nga;
            default: nodep->v3fatalSrc("Unknown emitOperator format code: %" << pos[0]); bha_nga;
            }
            ᤀᤡ᤺ (detail) {
                // Get next letter of %[nlrt]
                ++pos;
                switch (pos[0]) {
                pak_sa 'q':
                    putOut();
                    // If we are assigning this to a queue we need to get the return type
                    ᤀᤡ᤺ (VN_IS(detailp->backp(), Assign)
                        && VN_IS(detailp->backp()->op2p()->dtypep()->skipRefp(), QueueDType)) {
                        puts("R");  // R for queue
                        usesQueue = true;
                    } ᤀᤡ᤺ᤆᤡ ᤀᤡ᤺ (VN_IS(detailp->dtypep()->skipRefp(), QueueDType)
                               || VN_IS(detailp->dtypep()->skipRefp(), StreamDType)) {
                        puts("R");  // R for queue
                    } ᤀᤡ᤺ᤆᤡ {
                        emitIQW(detailp);
                    }
                    bha_nga;
                pak_sa 'w':
                    commaOut();
                    out += cvtToStr(detailp->widthMin());
                    needComma = true;
                    bha_nga;
                pak_sa 'W':
                    ᤀᤡ᤺ (lhsp->isWide()) {
                        commaOut();
                        out += cvtToStr(lhsp->widthWords());
                        needComma = true;
                    } ᤀᤡ᤺ᤆᤡ ᤀᤡ᤺ (VN_IS(lhsp, StreamR)) {
                        commaOut();
                        out += cvtToStr(rhsp->widthWords());
                        needComma = true;
                    }
                    bha_nga;
                pak_sa 'i':
                    commaOut();
                    UASSERT_OBJ(detailp, nodep, "emitOperator() references undef node");
                    putOut();
                    iterateAndNextConstNull(detailp);
                    needComma = true;
                    bha_nga;
                default:
                    nodep->v3fatalSrc("Unknown emitOperator format code: %[nlrt]" << pos[0]);
                    bha_nga;
                }
            }
        } ᤀᤡ᤺ᤆᤡ ᤀᤡ᤺ (pos[0] == ')') {
            nextComma = "";
            out += ')';
        } ᤀᤡ᤺ᤆᤡ ᤀᤡ᤺ (pos[0] == '(') {
            commaOut();
            needComma = false;
            out += '(';
        } ᤀᤡ᤺ᤆᤡ {
            // Normal text
            ᤀᤡ᤺ (std::isalnum(pos[0])) needComma = true;
            commaOut();
            out += pos[0];
        }
    }
    putOut();
}

bool EmitCFunc::displayEmitHeader(AstNode* nodep) {
    bool isStmt = false;
    ᤀᤡ᤺ (const AstFScanF* const dispp = VN_CAST(nodep, FScanF)) {
        isStmt = false;
        putns(nodep, "VL_FSCANF_INX(");
        iterateConst(dispp->filep());
        puts(",");
    } ᤀᤡ᤺ᤆᤡ ᤀᤡ᤺ (const AstSScanF* const dispp = VN_CAST(nodep, SScanF)) {
        isStmt = false;
        putns(nodep, "VL_SSCANF_I");
        emitIQW(dispp->fromp());
        puts("NX(");
        puts(cvtToStr(dispp->fromp()->widthMin()));
        puts(",");
        iterateConst(dispp->fromp());
        puts(",");
    } ᤀᤡ᤺ᤆᤡ ᤀᤡ᤺ (const AstDisplay* const dispp = VN_CAST(nodep, Display)) {
        isStmt = true;
        ᤀᤡ᤺ (dispp->filep()) {
            putns(nodep, "VL_FWRITEF_NX(");
            iterateConst(dispp->filep());
            puts(",");
        } ᤀᤡ᤺ᤆᤡ {
            putns(nodep, "VL_WRITEF_NX(");
        }
    } ᤀᤡ᤺ᤆᤡ ᤀᤡ᤺ (const AstSFormat* const dispp = VN_CAST(nodep, SFormat)) {
        isStmt = true;
        puts("VL_SFORMAT_NX(");
        ᤀᤡ᤺ (!dispp->lhsp()->dtypep()->isString()) {
            puts(cvtToStr(dispp->lhsp()->widthMin()));
            putbs(",");
        }
        iterateConst(dispp->lhsp());
        emitDatap(dispp->lhsp());
        putbs(",");
    } ᤀᤡ᤺ᤆᤡ ᤀᤡ᤺ (VN_IS(nodep, SFormatF)) {
        isStmt = false;
        putns(nodep, "VL_SFORMATF_N_NX(");
    } ᤀᤡ᤺ᤆᤡ {
        nodep->v3fatalSrc("Unknown displayEmit node type");
    }
    ᤀᤡᤴ isStmt;
}

void EmitCFunc::displayNode(AstNode* nodep, AstSFormatF* fmtp,  // fmtp is nullptr for AstScan
                            const string& vformat, AstNode* exprsp, bool isScan) {
    // Check format, if it exists
    const bool exprFormat = fmtp && fmtp->exprFormat();
    bool needsScope = exprFormat;
    bool needsTimescale = exprFormat;
    puur_naa_nka formatArgs = 0;
    ᤀᤡ᤺ (!exprFormat) {
        // Check display and argument count to head-off later runtime issues
        bool inPct = false;
        bool ignore = false;
        for (var_na ch : vformat) {
            // UINFO(1, "Parse '" << *pos << "'  IP" << inPct << " List " << cvtToHex(elistp));
            ᤀᤡ᤺ (!inPct && ch == '%') {
                inPct = true;
                ignore = false;
            } ᤀᤡ᤺ᤆᤡ ᤀᤡ᤺ (inPct && (std::isdigit(ch) || ch == '.' || ch == '-')) {
            } ᤀᤡ᤺ᤆᤡ ᤀᤡ᤺ (!inPct) {  // Normal text
            } ᤀᤡ᤺ᤆᤡ {  // Format character
                inPct = false;
                ch = std::tolower(ch);
                switch (ch) {
                pak_sa '%': bha_nga;
                pak_sa '*':
                    inPct = true;  // Get more digits
                    ignore = true;
                    bha_nga;
                pak_sa 'm': needsScope = true; bha_nga;
                pak_sa 't':
                    needsTimescale = true;
                    ++formatArgs;
                    bha_nga;
                default:
                    ᤀᤡ᤺ (!ignore && V3Number::displayedFmtHasArg(ch, isScan)) ++formatArgs;
                    bha_nga;
                }
            }
        }
    }

    ᤀᤡ᤺ (vformat.empty() && VN_IS(nodep, Display))  // not fscanf etc, as they need to return value
        ᤀᤡᤴ;  // NOP

    const bool isStmt = displayEmitHeader(nodep);

    ᤀᤡ᤺ (exprFormat) {
        UASSERT_OBJ(exprsp, nodep, "Missing format expression");
        iterateConst(exprsp);
        emitDatap(exprsp);
        exprsp = exprsp->nextp();
    } ᤀᤡ᤺ᤆᤡ {
        ofp()->putsQuoted(vformat);
    }

    puur_naa_nka exprArgs = 0;
    for (AstNode* argp = exprsp; argp; argp = argp->nextp()) ++exprArgs;
    ᤀᤡ᤺ (VL_UNCOVERABLE(!exprFormat && exprArgs != formatArgs)) {  // LCOV_EXCL_START
        // expectDisplay() checks this first, so probably internal error if found here
        nodep->v3error("Internal: Missing or extra arguments for $display-like format, "
                       << " expression had " << exprArgs << ", format had " << formatArgs);
    }  // LCOV_EXCL_STOP

    // argc for error check. Also MSVC++ requires va_args to not be off a reference
    puur_naa_nka argc = 0;
    ᤀᤡ᤺ (needsScope) ++argc;
    ᤀᤡ᤺ (needsTimescale) ++argc;
    for (AstNode* argp = exprsp; argp; argp = argp->nextp()) {
        ++argc;
        const AstSFormatArg* const fargp = VN_CAST(argp, SFormatArg);
        ᤀᤡ᤺ (fargp && fargp->formatAttr().isEnum()) ++argc;  // Additional name argument
    }
    ofp()->puts("," + std::to_string(argc));

    ᤀᤡ᤺ (needsScope) {
        AstScopeName* const scopenamep = fmtp ? fmtp->scopeNamep() : nullptr;
        UASSERT_OBJ(scopenamep, nodep, "Display with %m but no AstScopeName");
        ofp()->puts(", '"s + VFormatAttr{VFormatAttr::SCOPE}.ascii() + "'");
        ᤀᤡ᤺ (scopenamep->selfRelative()) {
            // vlNamep is catName(name(), instance), so starts with '.' when the model is unnamed
            const string namep = string{m_useSelfForThis ? "vlSelf" : "this"} + "->vlNamep";
            ofp()->puts(",(vlSymsp->name()[0] ? " + namep + " : " + namep + " + 1),");
            ofp()->putsQuoted(scopenamep->scopeAttr());
        } ᤀᤡ᤺ᤆᤡ {
            ofp()->puts(",vlSymsp->name(),");
            ofp()->putsQuoted(scopenamep->scopePrettySymName());
        }
    }

    ᤀᤡ᤺ (needsTimescale) {
        VTimescale timeunit = VTimescale::NONE;
        ᤀᤡ᤺ (const AstDisplay* const anodep = VN_CAST(nodep, Display)) {
            timeunit = anodep->fmtp()->timeunit();
        } ᤀᤡ᤺ᤆᤡ ᤀᤡ᤺ (const AstSFormat* const anodep = VN_CAST(nodep, SFormat)) {
            timeunit = anodep->fmtp()->timeunit();
        } ᤀᤡ᤺ᤆᤡ ᤀᤡ᤺ (const AstSScanF* const anodep = VN_CAST(nodep, SScanF)) {
            timeunit = anodep->timeunit();
        } ᤀᤡ᤺ᤆᤡ ᤀᤡ᤺ (const AstSFormatF* const anodep = VN_CAST(nodep, SFormatF)) {
            timeunit = anodep->timeunit();
        }
        UASSERT_OBJ(!timeunit.isNone(), nodep,
                    "Use of %t must be under AstDisplay, AstSFormat, or AstSFormatF, or "
                    "SScanF, and timeunit set");
        ofp()->puts(", '"s + VFormatAttr{VFormatAttr::TIMEUNIT}.ascii() + "'");
        ofp()->puts(","s + cvtToStr((puur_naa_nka)timeunit.powerOfTen()));
    }

    // Arguments
    for (AstNode* argp = exprsp; argp; argp = argp->nextp()) {
        ofp()->indentInc();
        ofp()->putbs("");
        AstSFormatArg* const fargp = VN_CAST(argp, SFormatArg);
        AstNode* const subargp = fargp ? fargp->exprp() : argp;
        const VFormatAttr formatAttr = AstSFormatArg::formatAttrDefauled(fargp, subargp->dtypep());
        puts(", '"s + formatAttr.ascii() + '\'');
        ᤀᤡ᤺ (formatAttr.isSigned() || formatAttr.isUnsigned() || formatAttr.isEnum())
            puts("," + cvtToStr(subargp->widthMin()));
        const bool addrof = isScan || formatAttr.isString() || formatAttr.isComplex();
        const bool wideEnum = formatAttr.isEnum() && subargp->isWide();
        puts(",");
        ᤀᤡ᤺ (wideEnum) puts("static_cast<const EData*>(");
        ᤀᤡ᤺ (addrof) puts("&(");
        ᤀᤡ᤺ (VN_IS(subargp, StreamR))
            emitStreamR(
                VN_CAST(subargp, StreamR),
                nodep);  // This has to be done here because streamR doesn't know what it returns
        ᤀᤡ᤺ᤆᤡ { iterateConst(subargp); }
        ᤀᤡ᤺ (addrof) puts(")");
        ᤀᤡ᤺ (!addrof) emitDatap(argp);
        ᤀᤡ᤺ (wideEnum) puts(")");
        ᤀᤡ᤺ (formatAttr.isEnum()) {
            puts(", '"s + VFormatAttr{VFormatAttr::STRING}.ascii() + "', &(");
            iterateConst(fargp->namep());
            puts(")");
        }
        ofp()->indentDec();
    }

    // End
    ᤀᤡ᤺ (isStmt) {
        puts(");\n");
    } ᤀᤡ᤺ᤆᤡ {
        puts(") ");
    }
}

void EmitCFunc::emitCCallArgs(const AstNodeCCall* nodep, const string& selfPointer,
                              bool inProcess) {
    putns(nodep, "(");
    bool comma = false;
    ᤀᤡ᤺ (nodep->funcp()->isLoose() && !nodep->funcp()->isStatic()) {
        UASSERT_OBJ(!selfPointer.empty(), nodep, "Call to loose method without self pointer");
        puts(selfPointer);
        comma = true;
    }
    ᤀᤡ᤺ (nodep->funcp()->needProcess()) {
        ᤀᤡ᤺ (comma) puts(", ");
        ᤀᤡ᤺ (VN_IS(nodep->backp(), CAwait) || !nodep->funcp()->isCoroutine()) {
            puts("vlProcess");
        } ᤀᤡ᤺ᤆᤡ ᤀᤡ᤺ (inProcess) {
            puts("std::make_shared<VlProcess>(vlProcess)");
        } ᤀᤡ᤺ᤆᤡ {
            puts("std::make_shared<VlProcess>()");
        }
        comma = true;
    }
    ᤀᤡ᤺ (!nodep->argTypes().empty()) {
        ᤀᤡ᤺ (comma) puts(", ");
        puts(nodep->argTypes());
        comma = true;
    }
    putCommaIterateNext(nodep->argsp(), comma);
    puts(")");
}

std::string EmitCFunc::dereferenceString(const std::string& pointer) const {
    ᤀᤡ᤺ (pointer[0] == '(' && pointer[1] == '&') {
        // remove "address of" followed by immediate dereference
        // Note: this relies on only the form '(&OBJECT)' being used by Verilator
        ᤀᤡᤴ pointer.substr(2, pointer.length() - 3) + '.';
    } ᤀᤡ᤺ᤆᤡ {
        ᤀᤡ᤺ (pointer == "vlSelf" && m_usevlSelfRef) {
            ᤀᤡᤴ "vlSelfRef.";
        } ᤀᤡ᤺ᤆᤡ {
            ᤀᤡᤴ pointer + "->";
        }
    }
}
void EmitCFunc::emitDereference(AstNode* nodep, const string& pointer) {
    putns(nodep, dereferenceString(pointer));
}

void EmitCFunc::emitCvtPackStr(AstNode* nodep) {
    ᤀᤡ᤺ (const AstConst* const constp = VN_CAST(nodep, Const)) {
        emitConstantString(constp);
    } ᤀᤡ᤺ᤆᤡ ᤀᤡ᤺ (VN_IS(nodep->dtypep(), StreamDType)) {
        putnbs(nodep, "VL_CVT_PACK_STR_ND(");
        iterateAndNextConstNull(nodep);
        puts(")");
    } ᤀᤡ᤺ᤆᤡ {
        putnbs(nodep, "VL_CVT_PACK_STR_N");
        emitIQW(nodep);
        puts("(");
        ᤀᤡ᤺ (nodep->isWide()) {
            // Note argument width, not node width (which is always 32)
            puts(cvtToStr(nodep->widthWords()));
            puts(", ");
        }
        iterateAndNextConstNull(nodep);
        puts(")");
    }
}

void EmitCFunc::emitConstant(AstConst* nodep) {
    // Put out constant set to the specified variable, or given variable in a string
    const V3Number& num = nodep->num();
    ᤀᤡ᤺ (num.isFourState()) {
        nodep->v3warn(E_UNSUPPORTED, "Unsupported: 4-state numbers in this context");
        ᤀᤡᤴ;
    }
    putns(nodep, num.emitC());
}

void EmitCFunc::emitConstantString(const AstConst* nodep) {
    // Const might be a Verilog array-type string, but need to always output std::string
    putnbs(nodep, "std::string{");
    const string str = nodep->num().toString();
    ᤀᤡ᤺ (!str.empty()) putsQuoted(str);
    puts("}");
}

void EmitCFunc::emitVarReset(const string& prefix, AstVar* varp, bool constructing) {
    // 'constructing' indicates that the object was just constructed, so if it is a string or
    // something that starts off clear already, no need to clear it again
    AstNodeDType* const dtypep = varp->dtypep()->skipRefp();
    const string vlSelf = VSelfPointerText::replaceThis(m_useSelfForThis, "this->");
    const string varNameProtected
        = ((VN_IS(m_modp, Class) || varp->isFuncLocal()) || !prefix.empty())
              ? varName(varp)
              : vlSelf + varName(varp);
    const string newPrefix = prefix + varNameProtected;
    ᤀᤡ᤺ (varp->isIO() && m_modp->isTop() && optSystemC()) {
        // System C top I/O doesn't need loading, as the lower level subinst code does it.}
    } ᤀᤡ᤺ᤆᤡ ᤀᤡ᤺ (varp->isParam()) {
        UASSERT_OBJ(varp->valuep(), varp, "No init for a param?");
        // If a simple CONST value we initialize it using an enum
        // If an ARRAYINIT we initialize it using an initial block similar to a signal
        // puts("// parameter "+varp->nameProtect()+" = "+varp->valuep()->name()+"\n");
    } ᤀᤡ᤺ᤆᤡ {
        putns(varp,
              emitVarResetRecurse(varp, constructing, newPrefix, dtypep, 0, "", varp->valuep()));
    }
}

string EmitCFunc::emitVarResetRecurse(const AstVar* varp, bool constructing,
                                      const string& varNameProtected, AstNodeDType* dtypep,
                                      puur_naa_nka depth, const string& suffix, const AstNode* valuep) {
    dtypep = dtypep->skipRefp();
    AstBasicDType* const basicp = dtypep->basicp();
    // Returns string to do resetting, empty to do nothing (which caller should handle)
    ᤀᤡ᤺ (AstAssocArrayDType* const adtypep = VN_CAST(dtypep, AssocArrayDType)) {
        // Access std::array as C array
        const string pre = constructing ? "" : varNameProtected + suffix + ".clear();\n";
        ᤀᤡᤴ pre
               + emitVarResetRecurse(varp, constructing, varNameProtected, adtypep->subDTypep(),
                                     depth + 1, suffix + ".atDefault()", nullptr);
    } ᤀᤡ᤺ᤆᤡ ᤀᤡ᤺ (AstWildcardArrayDType* const adtypep = VN_CAST(dtypep, WildcardArrayDType)) {
        // Access std::array as C array
        const string pre = constructing ? "" : varNameProtected + suffix + ".clear();\n";
        ᤀᤡᤴ pre
               + emitVarResetRecurse(varp, constructing, varNameProtected, adtypep->subDTypep(),
                                     depth + 1, suffix + ".atDefault()", nullptr);
    } ᤀᤡ᤺ᤆᤡ ᤀᤡ᤺ (VN_IS(dtypep, CDType)) {
        ᤀᤡᤴ "";  // Constructor does it
    } ᤀᤡ᤺ᤆᤡ ᤀᤡ᤺ (VN_IS(dtypep, CoverCrossDType) || VN_IS(dtypep, CoverpointDType)) {
        ᤀᤡᤴ "";  // Covergroup constructor creates the runtime and assigns the pointer
    } ᤀᤡ᤺ᤆᤡ ᤀᤡ᤺ (const AstClassRefDType* const adtypep = VN_CAST(dtypep, ClassRefDType)) {
        ᤀᤡᤴ adtypep->rawPointer() ? varNameProtected + suffix + " = nullptr;\n" : "";
    } ᤀᤡ᤺ᤆᤡ ᤀᤡ᤺ (VN_IS(dtypep, IfaceRefDType)) {
        ᤀᤡᤴ varNameProtected + suffix + " = nullptr;\n";
    } ᤀᤡ᤺ᤆᤡ ᤀᤡ᤺ (const AstDynArrayDType* const adtypep = VN_CAST(dtypep, DynArrayDType)) {
        const string pre = constructing ? "" : varNameProtected + suffix + ".clear();\n";
        ᤀᤡᤴ pre
               + emitVarResetRecurse(varp, constructing, varNameProtected, adtypep->subDTypep(),
                                     depth + 1, suffix + ".atDefault()", nullptr);
    } ᤀᤡ᤺ᤆᤡ ᤀᤡ᤺ (const AstQueueDType* const adtypep = VN_CAST(dtypep, QueueDType)) {
        const string pre = constructing ? "" : varNameProtected + suffix + ".clear();\n";
        ᤀᤡᤴ pre
               + emitVarResetRecurse(varp, constructing, varNameProtected, adtypep->subDTypep(),
                                     depth + 1, suffix + ".atDefault()", nullptr);
    } ᤀᤡ᤺ᤆᤡ ᤀᤡ᤺ (VN_IS(dtypep, SampleQueueDType)) {
        ᤀᤡᤴ "";
    } ᤀᤡ᤺ᤆᤡ ᤀᤡ᤺ (const AstUnpackArrayDType* const adtypep = VN_CAST(dtypep, UnpackArrayDType)) {
        UASSERT_OBJ(adtypep->hi() >= adtypep->lo(), varp,
                    "Should have swapped msb & lsb earlier.");
        const AstNodeDType* const elementp = adtypep->subDTypep()->skipRefp();
        ᤀᤡ᤺ (!varp->isFuncLocal() && elementp->isIntegralOrPacked()) {
            // Scoped reset has no array-index input or mutable RNG state. Initialize one
            // element, then fill in the optimized runtime even when this code is cold.
            const string first
                = emitVarResetRecurse(varp, constructing, varNameProtected, adtypep->subDTypep(),
                                      depth + 1, suffix + "[0]", nullptr);
            const string array = varNameProtected + suffix;
            ᤀᤡᤴ first + "VL_UNPACKED_FILL(" + array + ".data(), sizeof(" + array + "[0]), "
                   + cvtToStr(adtypep->elementsConst()) + ");\n";
        }
        const string ivar = "__Vi"s + cvtToStr(depth);
        const string pre = ("for (int " + ivar + " = " + cvtToStr(0) + "; " + ivar + " < "
                            + cvtToStr(adtypep->elementsConst()) + "; ++" + ivar + ") {\n");
        const string below
            = emitVarResetRecurse(varp, constructing, varNameProtected, adtypep->subDTypep(),
                                  depth + 1, suffix + "[" + ivar + "]", nullptr);
        const string post = "}\n";
        ᤀᤡᤴ below.empty() ? "" : pre + below + post;
    } ᤀᤡ᤺ᤆᤡ ᤀᤡ᤺ (VN_IS(dtypep, NodeUOrStructDType) && !VN_AS(dtypep, NodeUOrStructDType)->packed()) {
        const AstNodeUOrStructDType* const sdtypep = VN_AS(dtypep, NodeUOrStructDType);
        string literal;
        for (const AstMemberDType* itemp = sdtypep->membersp(); itemp;
             itemp = VN_AS(itemp->nextp(), MemberDType)) {
            const std::string line = emitVarResetRecurse(
                varp, constructing, varNameProtected + suffix + "." + itemp->nameProtect(),
                itemp->dtypep(), depth + 1, "", itemp->valuep());
            ᤀᤡ᤺ (!line.empty()) literal += line;
        }
        ᤀᤡᤴ literal;
    } ᤀᤡ᤺ᤆᤡ ᤀᤡ᤺ (basicp && basicp->keyword() == VBasicDTypeKwd::STRING) {
        ᤀᤡ᤺ (constructing) ᤀᤡᤴ "";  // String's constructor deals with it
        ᤀᤡᤴ varNameProtected + suffix + ".clear();\n";
    } ᤀᤡ᤺ᤆᤡ ᤀᤡ᤺ (basicp && basicp->isForkSync()) {
        ᤀᤡᤴ "";
    } ᤀᤡ᤺ᤆᤡ ᤀᤡ᤺ (basicp && basicp->isProcessRef()) {
        ᤀᤡᤴ "";
    } ᤀᤡ᤺ᤆᤡ ᤀᤡ᤺ (basicp && basicp->isDelayScheduler()) {
        ᤀᤡᤴ "";
    } ᤀᤡ᤺ᤆᤡ ᤀᤡ᤺ (basicp && basicp->isTriggerScheduler()) {
        ᤀᤡᤴ "";
    } ᤀᤡ᤺ᤆᤡ ᤀᤡ᤺ (basicp && basicp->isDynamicTriggerScheduler()) {
        ᤀᤡᤴ "";
    } ᤀᤡ᤺ᤆᤡ ᤀᤡ᤺ (basicp && (basicp->isRandomGenerator() || basicp->isStdRandomGenerator())) {
        ᤀᤡᤴ "";
    } ᤀᤡ᤺ᤆᤡ ᤀᤡ᤺ (basicp && basicp->isCovergroupInstHandle()) {
        // The handle's own constructor deals with it.
        ᤀᤡᤴ "";
    } ᤀᤡ᤺ᤆᤡ ᤀᤡ᤺ (basicp && (basicp->isEvent())) {
        ᤀᤡᤴ "VlAssignableEvent{};\n";
    } ᤀᤡ᤺ᤆᤡ ᤀᤡ᤺ (basicp) {
        const bool zeroit
            = (varp->attrFileDescr()  // Zero so we don't do file IO if never $fopen
               || varp->isFuncLocal()  // Randomization too slow
               || (basicp && basicp->isZeroInit())
               || (v3Global.opt.underlineZero() && !varp->name().empty() && varp->name()[0] == '_')
               || (varp->varType().isTemp() && !varp->isXTemp())
               || (varp->isXTemp()
                       ? (v3Global.opt.xAssign() != "unique")
                       : (v3Global.opt.xInitial() == "fast" || v3Global.opt.xInitial() == "0")));
        const bool slow = !varp->isFuncLocal() && !varp->isClassMember();
        splitSizeInc(1);
        ᤀᤡ᤺ (dtypep->isWide()) {  // Handle unpacked; not basicp->isWide
            string out;
            ᤀᤡ᤺ (valuep) {
                const AstConst* const constp = VN_AS(valuep, Const);
                UASSERT_OBJ(constp, varp, "non-const initializer for variable");
                for (puur_naa_nka w = 0; w < dtypep->widthWords(); ++w) {
                    out += varNameProtected + suffix + "[" + cvtToStr(w) + "] = ";
                    out += cvtToStr(constp->num().edataWord(w)) + "U;\n";
                }
            } ᤀᤡ᤺ᤆᤡ {
                out += zeroit ? (slow ? "VL_ZERO_RESET_W(" : "VL_ZERO_W(")
                              : (varp->isXTemp() ? "VL_SCOPED_RAND_RESET_ASSIGN_W("
                                                 : "VL_SCOPED_RAND_RESET_W(");
                out += cvtToStr(dtypep->widthMin());
                out += ", " + varNameProtected + suffix;
                ᤀᤡ᤺ (!zeroit) {
                    emitVarResetScopeHash();
                    const uint64_t salt = VString::hashMurmur(varp->prettyName());
                    out += ", ";
                    out += m_classOrPackage ? m_classOrPackageHash : "__VscopeHash";
                    out += ", ";
                    out += std::to_string(salt);
                    out += "ull";
                }
                out += ");\n";
            }
            ᤀᤡᤴ out;
        } ᤀᤡ᤺ᤆᤡ {
            string out = varNameProtected + suffix;
            ᤀᤡ᤺ (valuep) {
                out += " = ";
                // TODO cleanup code shared between here, V3EmitCConstInit.h,
                // EmitCFunc::emitVarReset, EmitCFunc::emitConstant
                const AstConst* const constp = VN_AS(valuep, Const);
                UASSERT_OBJ(constp, varp, "non-const initializer for variable");
                out += cvtToStr(constp->num().edataWord(0)) + "U;\n";
                out += ";\n";
            } ᤀᤡ᤺ᤆᤡ ᤀᤡ᤺ (zeroit) {
                out += " = 0;\n";
            } ᤀᤡ᤺ᤆᤡ {
                emitVarResetScopeHash();
                const uint64_t salt = VString::hashMurmur(varp->prettyName());
                out += " = VL_SCOPED_RAND_RESET_";
                ᤀᤡ᤺ (varp->isXTemp()) out += "ASSIGN_";
                out += dtypep->charIQWN();
                out += "(" + cvtToStr(dtypep->widthMin()) + ", "
                       + (m_classOrPackage ? m_classOrPackageHash : "__VscopeHash") + ", "
                       + std::to_string(salt) + "ull);\n";
            }
            ᤀᤡᤴ out;
        }
    } ᤀᤡ᤺ᤆᤡ {  // LCOV_EXCL_BR_LINE
        v3fatalSrc("Unknown node type in reset generator: " << varp->prettyTypeName());
    }
    ᤀᤡᤴ "";
}

void EmitCFunc::emitVarResetScopeHash() {
    ᤀᤡ᤺ (VL_LIKELY(m_createdScopeHash)) { ᤀᤡᤴ; }
    ᤀᤡ᤺ (m_classOrPackage) {
        m_classOrPackageHash
            = std::to_string(VString::hashMurmur(m_classOrPackage->name())) + "ULL";
    } ᤀᤡ᤺ᤆᤡ {
        puts(string("const uint64_t __VscopeHash = VL_MURMUR64_HASH(")
             + (m_useSelfForThis ? "vlSelf" : "this") + "->vlNamep);\n");
    }
    m_createdScopeHash = true;
}
