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
    nếu (nodep->emitSimpleOperator() == "") trả_về false;
    nếu (nodep->isWide()) trả_về false;
    nếu (nodep->op1p() && nodep->op1p()->isWide()) trả_về false;
    nếu (nodep->op2p() && nodep->op2p()->isWide()) trả_về false;
    nếu (nodep->op3p() && nodep->op3p()->isWide()) trả_về false;
    trả_về true;
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
        nếu (!nextComma.empty()) {
            out += nextComma;
            nextComma = "";
        }
    };

    auto putOut = [this, &out]() {
        nếu (!out.empty()) puts(out);
        out = "";
    };

    cho (string::const_iterator pos = format.begin(); pos != format.end(); ++pos) {
        nếu (pos[0] == ',') {
            // Remember we need to add one, but don't do yet to avoid ",)"
            nếu (needComma) {
                nếu (pos[1] == ' ') {
                    nextComma = ", ";
                } không_thì {
                    nextComma = ",";
                }
                needComma = false;
            }
            nếu (pos[1] == ' ') ++pos;  // Must do even if no nextComma
        } không_thì nếu (pos[0] == '%') {
            ++pos;
            bool detail = false;
            AstNode* detailp = nullptr;
            switch (pos[0]) {
            case '%': out += '%'; break;
            case 'k':
                putOut();
                putbs("");
                break;
            case 'n':
                detail = true;
                detailp = nodep;
                break;
            case 'l':
                detail = true;
                detailp = lhsp;
                break;
            case 'r':
                detail = true;
                detailp = rhsp;
                break;
            case 't':
                detail = true;
                detailp = thsp;
                break;
            case 'P':
                nếu (nodep->isWide() && !usesQueue) {
                    UASSERT_OBJ(m_wideTempRefp, nodep,
                                "Wide op " << nodep->prettyTypeName()
                                           << " w/ no temp, perhaps missing op in V3EmitC?");
                    commaOut();
                    putOut();
                    nếu (!m_wideTempRefp->selfPointer().isEmpty()) {
                        emitDereference(m_wideTempRefp,
                                        m_wideTempRefp->selfPointerProtect(m_useSelfForThis));
                    }
                    out += varName(m_wideTempRefp->varp());
                    m_wideTempRefp = nullptr;
                    needComma = true;
                } không_thì nếu (usesQueue) {
                    commaOut();
                    putOut();
                    iterateAndNextConstNull(nodep->backp()->op2p());
                    needComma = true;
                }

                break;
            default: nodep->v3fatalSrc("Unknown emitOperator format code: %" << pos[0]); break;
            }
            nếu (detail) {
                // Get next letter of %[nlrt]
                ++pos;
                switch (pos[0]) {
                case 'q':
                    putOut();
                    // If we are assigning this to a queue we need to get the return type
                    nếu (VN_IS(detailp->backp(), Assign)
                        && VN_IS(detailp->backp()->op2p()->dtypep()->skipRefp(), QueueDType)) {
                        puts("R");  // R for queue
                        usesQueue = true;
                    } không_thì nếu (VN_IS(detailp->dtypep()->skipRefp(), QueueDType)
                               || VN_IS(detailp->dtypep()->skipRefp(), StreamDType)) {
                        puts("R");  // R for queue
                    } không_thì {
                        emitIQW(detailp);
                    }
                    break;
                case 'w':
                    commaOut();
                    out += cvtToStr(detailp->widthMin());
                    needComma = true;
                    break;
                case 'W':
                    nếu (lhsp->isWide()) {
                        commaOut();
                        out += cvtToStr(lhsp->widthWords());
                        needComma = true;
                    } không_thì nếu (VN_IS(lhsp, StreamR)) {
                        commaOut();
                        out += cvtToStr(rhsp->widthWords());
                        needComma = true;
                    }
                    break;
                case 'i':
                    commaOut();
                    UASSERT_OBJ(detailp, nodep, "emitOperator() references undef node");
                    putOut();
                    iterateAndNextConstNull(detailp);
                    needComma = true;
                    break;
                default:
                    nodep->v3fatalSrc("Unknown emitOperator format code: %[nlrt]" << pos[0]);
                    break;
                }
            }
        } không_thì nếu (pos[0] == ')') {
            nextComma = "";
            out += ')';
        } không_thì nếu (pos[0] == '(') {
            commaOut();
            needComma = false;
            out += '(';
        } không_thì {
            // Normal text
            nếu (std::isalnum(pos[0])) needComma = true;
            commaOut();
            out += pos[0];
        }
    }
    putOut();
}

bool EmitCFunc::displayEmitHeader(AstNode* nodep) {
    bool isStmt = false;
    nếu (const AstFScanF* const dispp = VN_CAST(nodep, FScanF)) {
        isStmt = false;
        putns(nodep, "VL_FSCANF_INX(");
        iterateConst(dispp->filep());
        puts(",");
    } không_thì nếu (const AstSScanF* const dispp = VN_CAST(nodep, SScanF)) {
        isStmt = false;
        putns(nodep, "VL_SSCANF_I");
        emitIQW(dispp->fromp());
        puts("NX(");
        puts(cvtToStr(dispp->fromp()->widthMin()));
        puts(",");
        iterateConst(dispp->fromp());
        puts(",");
    } không_thì nếu (const AstDisplay* const dispp = VN_CAST(nodep, Display)) {
        isStmt = true;
        nếu (dispp->filep()) {
            putns(nodep, "VL_FWRITEF_NX(");
            iterateConst(dispp->filep());
            puts(",");
        } không_thì {
            putns(nodep, "VL_WRITEF_NX(");
        }
    } không_thì nếu (const AstSFormat* const dispp = VN_CAST(nodep, SFormat)) {
        isStmt = true;
        puts("VL_SFORMAT_NX(");
        nếu (!dispp->lhsp()->dtypep()->isString()) {
            puts(cvtToStr(dispp->lhsp()->widthMin()));
            putbs(",");
        }
        iterateConst(dispp->lhsp());
        emitDatap(dispp->lhsp());
        putbs(",");
    } không_thì nếu (VN_IS(nodep, SFormatF)) {
        isStmt = false;
        putns(nodep, "VL_SFORMATF_N_NX(");
    } không_thì {
        nodep->v3fatalSrc("Unknown displayEmit node type");
    }
    trả_về isStmt;
}

void EmitCFunc::displayNode(AstNode* nodep, AstSFormatF* fmtp,  // fmtp is nullptr for AstScan
                            const string& vformat, AstNode* exprsp, bool isScan) {
    // Check format, if it exists
    const bool exprFormat = fmtp && fmtp->exprFormat();
    bool needsScope = exprFormat;
    bool needsTimescale = exprFormat;
    số_nguyên formatArgs = 0;
    nếu (!exprFormat) {
        // Check display and argument count to head-off later runtime issues
        bool inPct = false;
        bool ignore = false;
        cho (char ch : vformat) {
            // UINFO(1, "Parse '" << *pos << "'  IP" << inPct << " List " << cvtToHex(elistp));
            nếu (!inPct && ch == '%') {
                inPct = true;
                ignore = false;
            } không_thì nếu (inPct && (std::isdigit(ch) || ch == '.' || ch == '-')) {
            } không_thì nếu (!inPct) {  // Normal text
            } không_thì {  // Format character
                inPct = false;
                ch = std::tolower(ch);
                switch (ch) {
                case '%': break;
                case '*':
                    inPct = true;  // Get more digits
                    ignore = true;
                    break;
                case 'm': needsScope = true; break;
                case 't':
                    needsTimescale = true;
                    ++formatArgs;
                    break;
                default:
                    nếu (!ignore && V3Number::displayedFmtHasArg(ch, isScan)) ++formatArgs;
                    break;
                }
            }
        }
    }

    nếu (vformat.empty() && VN_IS(nodep, Display))  // not fscanf etc, as they need to return value
        trả_về;  // NOP

    const bool isStmt = displayEmitHeader(nodep);

    nếu (exprFormat) {
        UASSERT_OBJ(exprsp, nodep, "Missing format expression");
        iterateConst(exprsp);
        emitDatap(exprsp);
        exprsp = exprsp->nextp();
    } không_thì {
        ofp()->putsQuoted(vformat);
    }

    số_nguyên exprArgs = 0;
    cho (AstNode* argp = exprsp; argp; argp = argp->nextp()) ++exprArgs;
    nếu (VL_UNCOVERABLE(!exprFormat && exprArgs != formatArgs)) {  // LCOV_EXCL_START
        // expectDisplay() checks this first, so probably internal error if found here
        nodep->v3error("Internal: Missing or extra arguments for $display-like format, "
                       << " expression had " << exprArgs << ", format had " << formatArgs);
    }  // LCOV_EXCL_STOP

    // argc for error check. Also MSVC++ requires va_args to not be off a reference
    số_nguyên argc = 0;
    nếu (needsScope) ++argc;
    nếu (needsTimescale) ++argc;
    cho (AstNode* argp = exprsp; argp; argp = argp->nextp()) {
        ++argc;
        const AstSFormatArg* const fargp = VN_CAST(argp, SFormatArg);
        nếu (fargp && fargp->formatAttr().isEnum()) ++argc;  // Additional name argument
    }
    ofp()->puts("," + std::to_string(argc));

    nếu (needsScope) {
        AstScopeName* const scopenamep = fmtp ? fmtp->scopeNamep() : nullptr;
        UASSERT_OBJ(scopenamep, nodep, "Display with %m but no AstScopeName");
        ofp()->puts(", '"s + VFormatAttr{VFormatAttr::SCOPE}.ascii() + "'");
        nếu (scopenamep->selfRelative()) {
            // vlNamep is catName(name(), instance), so starts with '.' when the model is unnamed
            const string namep = string{m_useSelfForThis ? "vlSelf" : "this"} + "->vlNamep";
            ofp()->puts(",(vlSymsp->name()[0] ? " + namep + " : " + namep + " + 1),");
            ofp()->putsQuoted(scopenamep->scopeAttr());
        } không_thì {
            ofp()->puts(",vlSymsp->name(),");
            ofp()->putsQuoted(scopenamep->scopePrettySymName());
        }
    }

    nếu (needsTimescale) {
        VTimescale timeunit = VTimescale::NONE;
        nếu (const AstDisplay* const anodep = VN_CAST(nodep, Display)) {
            timeunit = anodep->fmtp()->timeunit();
        } không_thì nếu (const AstSFormat* const anodep = VN_CAST(nodep, SFormat)) {
            timeunit = anodep->fmtp()->timeunit();
        } không_thì nếu (const AstSScanF* const anodep = VN_CAST(nodep, SScanF)) {
            timeunit = anodep->timeunit();
        } không_thì nếu (const AstSFormatF* const anodep = VN_CAST(nodep, SFormatF)) {
            timeunit = anodep->timeunit();
        }
        UASSERT_OBJ(!timeunit.isNone(), nodep,
                    "Use of %t must be under AstDisplay, AstSFormat, or AstSFormatF, or "
                    "SScanF, and timeunit set");
        ofp()->puts(", '"s + VFormatAttr{VFormatAttr::TIMEUNIT}.ascii() + "'");
        ofp()->puts(","s + cvtToStr((số_nguyên)timeunit.powerOfTen()));
    }

    // Arguments
    cho (AstNode* argp = exprsp; argp; argp = argp->nextp()) {
        ofp()->indentInc();
        ofp()->putbs("");
        AstSFormatArg* const fargp = VN_CAST(argp, SFormatArg);
        AstNode* const subargp = fargp ? fargp->exprp() : argp;
        const VFormatAttr formatAttr = AstSFormatArg::formatAttrDefauled(fargp, subargp->dtypep());
        puts(", '"s + formatAttr.ascii() + '\'');
        nếu (formatAttr.isSigned() || formatAttr.isUnsigned() || formatAttr.isEnum())
            puts("," + cvtToStr(subargp->widthMin()));
        const bool addrof = isScan || formatAttr.isString() || formatAttr.isComplex();
        const bool wideEnum = formatAttr.isEnum() && subargp->isWide();
        puts(",");
        nếu (wideEnum) puts("static_cast<const EData*>(");
        nếu (addrof) puts("&(");
        nếu (VN_IS(subargp, StreamR))
            emitStreamR(
                VN_CAST(subargp, StreamR),
                nodep);  // This has to be done here because streamR doesn't know what it returns
        không_thì { iterateConst(subargp); }
        nếu (addrof) puts(")");
        nếu (!addrof) emitDatap(argp);
        nếu (wideEnum) puts(")");
        nếu (formatAttr.isEnum()) {
            puts(", '"s + VFormatAttr{VFormatAttr::STRING}.ascii() + "', &(");
            iterateConst(fargp->namep());
            puts(")");
        }
        ofp()->indentDec();
    }

    // End
    nếu (isStmt) {
        puts(");\n");
    } không_thì {
        puts(") ");
    }
}

void EmitCFunc::emitCCallArgs(const AstNodeCCall* nodep, const string& selfPointer,
                              bool inProcess) {
    putns(nodep, "(");
    bool comma = false;
    nếu (nodep->funcp()->isLoose() && !nodep->funcp()->isStatic()) {
        UASSERT_OBJ(!selfPointer.empty(), nodep, "Call to loose method without self pointer");
        puts(selfPointer);
        comma = true;
    }
    nếu (nodep->funcp()->needProcess()) {
        nếu (comma) puts(", ");
        nếu (VN_IS(nodep->backp(), CAwait) || !nodep->funcp()->isCoroutine()) {
            puts("vlProcess");
        } không_thì nếu (inProcess) {
            puts("std::make_shared<VlProcess>(vlProcess)");
        } không_thì {
            puts("std::make_shared<VlProcess>()");
        }
        comma = true;
    }
    nếu (!nodep->argTypes().empty()) {
        nếu (comma) puts(", ");
        puts(nodep->argTypes());
        comma = true;
    }
    putCommaIterateNext(nodep->argsp(), comma);
    puts(")");
}

std::string EmitCFunc::dereferenceString(const std::string& pointer) const {
    nếu (pointer[0] == '(' && pointer[1] == '&') {
        // remove "address of" followed by immediate dereference
        // Note: this relies on only the form '(&OBJECT)' being used by Verilator
        trả_về pointer.substr(2, pointer.length() - 3) + '.';
    } không_thì {
        nếu (pointer == "vlSelf" && m_usevlSelfRef) {
            trả_về "vlSelfRef.";
        } không_thì {
            trả_về pointer + "->";
        }
    }
}
void EmitCFunc::emitDereference(AstNode* nodep, const string& pointer) {
    putns(nodep, dereferenceString(pointer));
}

void EmitCFunc::emitCvtPackStr(AstNode* nodep) {
    nếu (const AstConst* const constp = VN_CAST(nodep, Const)) {
        emitConstantString(constp);
    } không_thì nếu (VN_IS(nodep->dtypep(), StreamDType)) {
        putnbs(nodep, "VL_CVT_PACK_STR_ND(");
        iterateAndNextConstNull(nodep);
        puts(")");
    } không_thì {
        putnbs(nodep, "VL_CVT_PACK_STR_N");
        emitIQW(nodep);
        puts("(");
        nếu (nodep->isWide()) {
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
    nếu (num.isFourState()) {
        nodep->v3warn(E_UNSUPPORTED, "Unsupported: 4-state numbers in this context");
        trả_về;
    }
    putns(nodep, num.emitC());
}

void EmitCFunc::emitConstantString(const AstConst* nodep) {
    // Const might be a Verilog array-type string, but need to always output std::string
    putnbs(nodep, "std::string{");
    const string str = nodep->num().toString();
    nếu (!str.empty()) putsQuoted(str);
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
    nếu (varp->isIO() && m_modp->isTop() && optSystemC()) {
        // System C top I/O doesn't need loading, as the lower level subinst code does it.}
    } không_thì nếu (varp->isParam()) {
        UASSERT_OBJ(varp->valuep(), varp, "No init for a param?");
        // If a simple CONST value we initialize it using an enum
        // If an ARRAYINIT we initialize it using an initial block similar to a signal
        // puts("// parameter "+varp->nameProtect()+" = "+varp->valuep()->name()+"\n");
    } không_thì {
        putns(varp,
              emitVarResetRecurse(varp, constructing, newPrefix, dtypep, 0, "", varp->valuep()));
    }
}

string EmitCFunc::emitVarResetRecurse(const AstVar* varp, bool constructing,
                                      const string& varNameProtected, AstNodeDType* dtypep,
                                      số_nguyên depth, const string& suffix, const AstNode* valuep) {
    dtypep = dtypep->skipRefp();
    AstBasicDType* const basicp = dtypep->basicp();
    // Returns string to do resetting, empty to do nothing (which caller should handle)
    nếu (AstAssocArrayDType* const adtypep = VN_CAST(dtypep, AssocArrayDType)) {
        // Access std::array as C array
        const string pre = constructing ? "" : varNameProtected + suffix + ".clear();\n";
        trả_về pre
               + emitVarResetRecurse(varp, constructing, varNameProtected, adtypep->subDTypep(),
                                     depth + 1, suffix + ".atDefault()", nullptr);
    } không_thì nếu (AstWildcardArrayDType* const adtypep = VN_CAST(dtypep, WildcardArrayDType)) {
        // Access std::array as C array
        const string pre = constructing ? "" : varNameProtected + suffix + ".clear();\n";
        trả_về pre
               + emitVarResetRecurse(varp, constructing, varNameProtected, adtypep->subDTypep(),
                                     depth + 1, suffix + ".atDefault()", nullptr);
    } không_thì nếu (VN_IS(dtypep, CDType)) {
        trả_về "";  // Constructor does it
    } không_thì nếu (VN_IS(dtypep, CoverCrossDType) || VN_IS(dtypep, CoverpointDType)) {
        trả_về "";  // Covergroup constructor creates the runtime and assigns the pointer
    } không_thì nếu (const AstClassRefDType* const adtypep = VN_CAST(dtypep, ClassRefDType)) {
        trả_về adtypep->rawPointer() ? varNameProtected + suffix + " = nullptr;\n" : "";
    } không_thì nếu (VN_IS(dtypep, IfaceRefDType)) {
        trả_về varNameProtected + suffix + " = nullptr;\n";
    } không_thì nếu (const AstDynArrayDType* const adtypep = VN_CAST(dtypep, DynArrayDType)) {
        const string pre = constructing ? "" : varNameProtected + suffix + ".clear();\n";
        trả_về pre
               + emitVarResetRecurse(varp, constructing, varNameProtected, adtypep->subDTypep(),
                                     depth + 1, suffix + ".atDefault()", nullptr);
    } không_thì nếu (const AstQueueDType* const adtypep = VN_CAST(dtypep, QueueDType)) {
        const string pre = constructing ? "" : varNameProtected + suffix + ".clear();\n";
        trả_về pre
               + emitVarResetRecurse(varp, constructing, varNameProtected, adtypep->subDTypep(),
                                     depth + 1, suffix + ".atDefault()", nullptr);
    } không_thì nếu (VN_IS(dtypep, SampleQueueDType)) {
        trả_về "";
    } không_thì nếu (const AstUnpackArrayDType* const adtypep = VN_CAST(dtypep, UnpackArrayDType)) {
        UASSERT_OBJ(adtypep->hi() >= adtypep->lo(), varp,
                    "Should have swapped msb & lsb earlier.");
        const AstNodeDType* const elementp = adtypep->subDTypep()->skipRefp();
        nếu (!varp->isFuncLocal() && elementp->isIntegralOrPacked()) {
            // Scoped reset has no array-index input or mutable RNG state. Initialize one
            // element, then fill in the optimized runtime even when this code is cold.
            const string first
                = emitVarResetRecurse(varp, constructing, varNameProtected, adtypep->subDTypep(),
                                      depth + 1, suffix + "[0]", nullptr);
            const string array = varNameProtected + suffix;
            trả_về first + "VL_UNPACKED_FILL(" + array + ".data(), sizeof(" + array + "[0]), "
                   + cvtToStr(adtypep->elementsConst()) + ");\n";
        }
        const string ivar = "__Vi"s + cvtToStr(depth);
        const string pre = ("for (int " + ivar + " = " + cvtToStr(0) + "; " + ivar + " < "
                            + cvtToStr(adtypep->elementsConst()) + "; ++" + ivar + ") {\n");
        const string below
            = emitVarResetRecurse(varp, constructing, varNameProtected, adtypep->subDTypep(),
                                  depth + 1, suffix + "[" + ivar + "]", nullptr);
        const string post = "}\n";
        trả_về below.empty() ? "" : pre + below + post;
    } không_thì nếu (VN_IS(dtypep, NodeUOrStructDType) && !VN_AS(dtypep, NodeUOrStructDType)->packed()) {
        const AstNodeUOrStructDType* const sdtypep = VN_AS(dtypep, NodeUOrStructDType);
        string literal;
        cho (const AstMemberDType* itemp = sdtypep->membersp(); itemp;
             itemp = VN_AS(itemp->nextp(), MemberDType)) {
            const std::string line = emitVarResetRecurse(
                varp, constructing, varNameProtected + suffix + "." + itemp->nameProtect(),
                itemp->dtypep(), depth + 1, "", itemp->valuep());
            nếu (!line.empty()) literal += line;
        }
        trả_về literal;
    } không_thì nếu (basicp && basicp->keyword() == VBasicDTypeKwd::STRING) {
        nếu (constructing) trả_về "";  // String's constructor deals with it
        trả_về varNameProtected + suffix + ".clear();\n";
    } không_thì nếu (basicp && basicp->isForkSync()) {
        trả_về "";
    } không_thì nếu (basicp && basicp->isProcessRef()) {
        trả_về "";
    } không_thì nếu (basicp && basicp->isDelayScheduler()) {
        trả_về "";
    } không_thì nếu (basicp && basicp->isTriggerScheduler()) {
        trả_về "";
    } không_thì nếu (basicp && basicp->isDynamicTriggerScheduler()) {
        trả_về "";
    } không_thì nếu (basicp && (basicp->isRandomGenerator() || basicp->isStdRandomGenerator())) {
        trả_về "";
    } không_thì nếu (basicp && basicp->isCovergroupInstHandle()) {
        // The handle's own constructor deals with it.
        trả_về "";
    } không_thì nếu (basicp && (basicp->isEvent())) {
        trả_về "VlAssignableEvent{};\n";
    } không_thì nếu (basicp) {
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
        nếu (dtypep->isWide()) {  // Handle unpacked; not basicp->isWide
            string out;
            nếu (valuep) {
                const AstConst* const constp = VN_AS(valuep, Const);
                UASSERT_OBJ(constp, varp, "non-const initializer for variable");
                cho (số_nguyên w = 0; w < dtypep->widthWords(); ++w) {
                    out += varNameProtected + suffix + "[" + cvtToStr(w) + "] = ";
                    out += cvtToStr(constp->num().edataWord(w)) + "U;\n";
                }
            } không_thì {
                out += zeroit ? (slow ? "VL_ZERO_RESET_W(" : "VL_ZERO_W(")
                              : (varp->isXTemp() ? "VL_SCOPED_RAND_RESET_ASSIGN_W("
                                                 : "VL_SCOPED_RAND_RESET_W(");
                out += cvtToStr(dtypep->widthMin());
                out += ", " + varNameProtected + suffix;
                nếu (!zeroit) {
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
            trả_về out;
        } không_thì {
            string out = varNameProtected + suffix;
            nếu (valuep) {
                out += " = ";
                // TODO cleanup code shared between here, V3EmitCConstInit.h,
                // EmitCFunc::emitVarReset, EmitCFunc::emitConstant
                const AstConst* const constp = VN_AS(valuep, Const);
                UASSERT_OBJ(constp, varp, "non-const initializer for variable");
                out += cvtToStr(constp->num().edataWord(0)) + "U;\n";
                out += ";\n";
            } không_thì nếu (zeroit) {
                out += " = 0;\n";
            } không_thì {
                emitVarResetScopeHash();
                const uint64_t salt = VString::hashMurmur(varp->prettyName());
                out += " = VL_SCOPED_RAND_RESET_";
                nếu (varp->isXTemp()) out += "ASSIGN_";
                out += dtypep->charIQWN();
                out += "(" + cvtToStr(dtypep->widthMin()) + ", "
                       + (m_classOrPackage ? m_classOrPackageHash : "__VscopeHash") + ", "
                       + std::to_string(salt) + "ull);\n";
            }
            trả_về out;
        }
    } không_thì {  // LCOV_EXCL_BR_LINE
        v3fatalSrc("Unknown node type in reset generator: " << varp->prettyTypeName());
    }
    trả_về "";
}

void EmitCFunc::emitVarResetScopeHash() {
    nếu (VL_LIKELY(m_createdScopeHash)) { trả_về; }
    nếu (m_classOrPackage) {
        m_classOrPackageHash
            = std::to_string(VString::hashMurmur(m_classOrPackage->name())) + "ULL";
    } không_thì {
        puts(string("const uint64_t __VscopeHash = VL_MURMUR64_HASH(")
             + (m_useSelfForThis ? "vlSelf" : "this") + "->vlNamep);\n");
    }
    m_createdScopeHash = true;
}
