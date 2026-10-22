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

#यदि_अपरिभाषितम् VERILATOR_V3EMITCFUNC_H_
#परिभाषय VERILATOR_V3EMITCFUNC_H_

#अन्तर्भाव "config_build.h"
#अन्तर्भाव "verilatedos.h"

#अन्तर्भाव "V3EmitCConstInit.h"
#अन्तर्भाव "V3Global.h"
#अन्तर्भाव "V3MemberMap.h"

#अन्तर्भाव <algorithm>
#अन्तर्भाव <map>
#अन्तर्भाव <unordered_map>
#अन्तर्भाव <unordered_set>
#अन्तर्भाव <vector>

// Number of VL_CONST_W_*X's in verilated.h (IE VL_CONST_W_8X is last)
constexpr पूर्णाङ्क EMITC_अङ्कः_CONSTW = 8;

//######################################################################
// Emit lazy forward declarations

class EmitCLazyDecls final : सार्वजनिक VNVisitorConst {
    // NODE STATE/TYPES
    // None allowed to support threaded emitting

    // MEMBERS
    std::unordered_समुच्चयः<string> m_emittedManually;  // Set of names already declared manually.
    EmitCBaseVisitorConst& m_emitter;  // For access to file output
    bool m_needs_Blank_पङ्क्तिः = false;  // Emit blank line if any declarations were emitted (cosmetic)
    std::समुच्चयः<Ast_ग्रन्थिः*> m_emitted;  // -> in set. Already emitted decl for symbols.

    // METHODS
    bool declaredOnce(Ast_ग्रन्थिः* nodep) { प्रत्यागम m_emitted.निवेशय(nodep).second; }

    शून्य lazyDeclare(AstCFunc* funcp) {
        // Already declared in this compilation unit
        यदि (!declaredOnce(funcp)) प्रत्यागम;
        // Check if this kind of function is lazily declared
        यदि (!(funcp->is_विधानम्() && funcp->isLoose()) && !funcp->dpiImportPrototype()) प्रत्यागम;
        // Already declared manually
        यदि (m_emittedManually.गणनम्(funcp->nameProtect())) प्रत्यागम;
        // Needs lazy declaration, emit one
        m_emitter.emitCFuncDecl(funcp, EmitCParentModule::get(funcp), funcp->dpiImportPrototype());
        m_needs_Blank_पङ्क्तिः = true;
    }

    शून्य lazyDeclareConstPoolVar(AstVar* varp) {
        यदि (!declaredOnce(varp)) प्रत्यागम;  // Already declared
        ध्रुव string nameProtect = EmitCUtil::constPoolName(varp);
        m_emitter.putns(varp, "extern const ");
        m_emitter.पङ्क्तिं_लिख(varp->dtypep()->cType(nameProtect, false, false));
        m_emitter.पङ्क्तिं_लिख(";\n");
        m_needs_Blank_पङ्क्तिः = true;
    }

    // VISITORS
    शून्य दर्शय(Ast_ग्रन्थिः_CCall* nodep) override {
        lazyDeclare(nodep->funcp());
        iterateChildrenConst(nodep);
    }

    शून्य दर्शय(Ast_Addr_सम्बन्धि_CFunc* nodep) override {
        lazyDeclare(nodep->funcp());
        iterateChildrenConst(nodep);
    }

    शून्य दर्शय(AstVarRef* nodep) override {
        AstVar* ध्रुव varp = nodep->varp();
        // Only constant pool symbols are lazy declared for now ...
        यदि (EmitCParentModule::get(varp)->isConstPool()) lazyDeclareConstPoolVar(varp);
    }

    शून्य दर्शय(AstExecGraph* nodep) override {
        यदि (!declaredOnce(nodep)) प्रत्यागम;
        m_emitter.putns(nodep, "extern VlExecGraph " + EmitCUtil::execGraphName(nodep) + ";\n");
        m_needs_Blank_पङ्क्तिः = true;
    }

    शून्य दर्शय(Ast_ग्रन्थिः* nodep) override { iterateChildrenConst(nodep); }

सार्वजनिक:
    explicit EmitCLazyDecls(EmitCBaseVisitorConst& emitter)
        : m_emitter{emitter} {}
    शून्य emit(Ast_ग्रन्थिः* nodep) {
        m_needs_Blank_पङ्क्तिः = false;
        iterateChildrenConst(nodep);
        यदि (m_needs_Blank_पङ्क्तिः) m_emitter.पङ्क्तिं_लिख("\n");
    }
    शून्य emit(ध्रुव string& उपसर्गः, ध्रुव string& name, ध्रुव string& suffix) {
        m_emittedManually.निवेशय(name);
        m_emitter.ensure_New_पङ्क्तिः();
        m_emitter.पङ्क्तिं_लिख(उपसर्गः);
        m_emitter.पङ्क्तिं_लिख(name);
        m_emitter.पङ्क्तिं_लिख(suffix);
        m_emitter.ensure_New_पङ्क्तिः();
    }
    शून्य declared(AstCFunc* nodep) { m_emitted.निवेशय(nodep); }
    शून्य reset() { m_emitted.clear(); }
};

// ######################################################################
//  Emit statements and expressions

class EmitCFunc VL_NOT_FINAL : सार्वजनिक EmitCConstInit {
    VMemberMap m_memberMap;
    AstVarRef* m_wide_अस्थायी_Refp = nullptr;  // Variable that _WW macros should be setting
    std::unordered_map<Ast_Jump_खण्डम्*, आकार_t> m_labelNumbers;  // Label numbers for AstJumpBlocks
    bool m_createdScopeHash = false;  // Already created a scope hash

रक्षित:
    VL_DEFINE_DEBUG_FUNCTIONS;

    EmitCLazyDecls m_lazyDecls{*this};  // Visitor for emitting lazy declarations
    bool m_useSelfForThis = false;  // Replace "this" with "vlSelf"
    bool m_usevlSelfRef = false;  // Use vlSelfRef reference instead of vlSelf pointer
    ध्रुव Ast_ग्रन्थिः_Module* m_modp = nullptr;  // Current module being emitted
    ध्रुव AstCFunc* m_cfuncp = nullptr;  // Current function being emitted
    bool m_instantiatesOwnProcess = false;
    ध्रुव AstClassPackage* m_classOrPackage = nullptr;  // Pointer to current class or package
    string m_classOrPackageHash;  // Hash of class or package name

    bool constructorNeedsProcess(ध्रुव AstClass* ध्रुव classp) {
        ध्रुव Ast_ग्रन्थिः* ध्रुव newp = m_memberMap.findMember(classp, "new");
        यदि (!newp) प्रत्यागम false;
        ध्रुव AstCFunc* ध्रुव ctorp = VN_CAST(newp, CFunc);
        यदि (!ctorp) प्रत्यागम false;
        UASSERT_OBJ(ctorp->isConstructor(), ctorp, "`new` is not a constructor!");
        प्रत्यागम ctorp->needProcess();
    }

    bool constructorNeedsProcess(ध्रुव Ast_ग्रन्थिः_DType* ध्रुव dtypep) {
        यदि (ध्रुव AstClassRefDType* ध्रुव crefdtypep = VN_CAST(dtypep, ClassRefDType))
            प्रत्यागम constructorNeedsProcess(crefdtypep->classp());
        प्रत्यागम false;
    }

    bool coverage_उपयुङ्क्ते_Local_गणक(Ast_ग्रन्थिः_Cover_Decl* ध्रुव declp) ध्रुव {
        // Only functions with an object receiver can address the module-local
        // coverage array. Static/package/class helper functions still use the
        // symbol-table array, as they may have vlSymsp but no vlSelf/this.
        // Also require the coverage declaration to belong to the current
        // module; cloned task bodies can increment declarations owned by a
        // different module, and those must keep using the global array.
        प्रत्यागम m_cfuncp && !m_cfuncp->isStatic() && !VN_IS(m_modp, Class)
               && !VN_IS(m_modp, ClassPackage)
               && EmitCParentModule::get(declp->दत्त_Decl_Thisp()) == m_modp;
    }
    पूर्णाङ्क coverage_Bin_अङ्कः(Ast_ग्रन्थिः_Cover_Decl* ध्रुव declp, bool forceGlobal = false) ध्रुव {
        प्रत्यागम !forceGlobal && coverage_उपयुङ्क्ते_Local_गणक(declp)
                   ? declp->दत्त_Decl_Thisp()->local_Bin_अङ्कः()
                   : declp->दत्त_Decl_Thisp()->bin_अङ्कः();
    }
    शून्य putCoverageArray(Ast_ग्रन्थिः_Cover_Decl* ध्रुव declp, bool forceGlobal = false) {
        यदि (!forceGlobal && coverage_उपयुङ्क्ते_Local_गणक(declp)) {
            // Keep counters on the module object when possible, so every
            // no-inline instance has storage available if coverage output is
            // later written with forcePerInstance.
            पङ्क्तिं_लिख(m_cfuncp->isLoose() ? "vlSelf->__Vcoverage" : "this->__Vcoverage");
        } अन्यथा {
            // Static/package/class helper functions do not have vlSelf, so
            // keep using the shared symbol-table coverage array there.
            पङ्क्तिं_लिख("vlSymsp->__Vcoverage");
        }
    }
    शून्य emit_Cover_Other_Decl_निवेशय(AstCoverOtherDecl* nodep, bool forceGlobal = false) {
        putns(nodep, "vlSelf->__vlCoverInsert(");  // As Declared in emitCoverageDecl
        putCoverageArray(nodep, forceGlobal);
        पङ्क्तिं_लिख(" + ");
        पङ्क्तिं_लिख(cvt_गन्तव्यम्_Str(coverage_Bin_अङ्कः(nodep, forceGlobal)));
        // first controls whether duplicate module instances are collapsed in
        // the default coverage view. The following flag says whether countp
        // points to an object-local counter; only those counters must be kept
        // real for later instances so forcePerInstance can split hierarchy.
        पङ्क्तिं_लिख(", first");  // Enable, passed from __Vconfigure parameter
        पङ्क्तिं_लिख(", ");
        पङ्क्तिं_लिख(!forceGlobal && coverage_उपयुङ्क्ते_Local_गणक(nodep) ? "true" : "false");
        पङ्क्तिं_लिख(", ");
        putsQuoted(protect(nodep->fileline()->filename()));
        पङ्क्तिं_लिख(", ");
        पङ्क्तिं_लिख(cvt_गन्तव्यम्_Str(nodep->fileline()->lineno()));
        पङ्क्तिं_लिख(", ");
        पङ्क्तिं_लिख(cvt_गन्तव्यम्_Str(nodep->विस्थापनम्() + nodep->fileline()->firstColumn()));
        पङ्क्तिं_लिख(", ");
        putsQuoted((!nodep->hier().रिक्तानि() ? "." : "")
                   + VIdProtect::protect_शब्दाः_If(nodep->hier(), nodep->protect()));
        पङ्क्तिं_लिख(", ");
        putsQuoted(VIdProtect::protect_शब्दाः_If(nodep->page(), nodep->protect()));
        पङ्क्तिं_लिख(", ");
        putsQuoted(VIdProtect::protect_शब्दाः_If(nodep->comment(), nodep->protect()));
        पङ्क्तिं_लिख(", ");
        putsQuoted(nodep->linescov());
        पङ्क्तिं_लिख(", ");
        putsQuoted(VIdProtect::protect_शब्दाः_If(nodep->fsmVar(), nodep->protect()));
        पङ्क्तिं_लिख(", ");
        putsQuoted(VIdProtect::protect_शब्दाः_If(nodep->fsmFrom(), nodep->protect()));
        पङ्क्तिं_लिख(", ");
        putsQuoted(VIdProtect::protect_शब्दाः_If(nodep->fsm_गन्तव्यम्(), nodep->protect()));
        पङ्क्तिं_लिख(", ");
        putsQuoted(VIdProtect::protect_शब्दाः_If(nodep->fsm_चिह्नकम्(), nodep->protect()));
        पङ्क्तिं_लिख(");\n");
    }

सार्वजनिक:
    // METHODS
    bool display_Emit_शीर्षकम्(Ast_ग्रन्थिः* nodep);
    शून्य display_ग्रन्थिः(Ast_ग्रन्थिः* nodep, AstSFormatF* fmtp, ध्रुव string& vformat, Ast_ग्रन्थिः* exprsp,
                     bool is_अवलोकनम्);

    bool emitSimpleOk(Ast_ग्रन्थिः_व्यञ्जकम्* nodep);
    शून्य emitIQW(ध्रुव Ast_ग्रन्थिः* nodep) {
        // See "Type letters" in verilated.h
        // Other abbrevs: "C"har, "S"hort, "F"loat, "D"ouble, stri"N"g, "R"=queue, "U"npacked
        पङ्क्तिं_लिख(nodep->dtypep()->skipRefp()->charIQWN());
    }
    शून्य emitRU(ध्रुव Ast_ग्रन्थिः* nodep) {
        ध्रुव Ast_ग्रन्थिः_DType* dtp = nodep->dtypep()->skipRefp();
        // See "Type letters" in verilated.h
        यदि (VN_IS(dtp, UnpackArrayDType))
            पङ्क्तिं_लिख("U");
        अन्यथा यदि (VN_IS(dtp, QueueDType) || VN_IS(dtp, DynArrayDType))
            पङ्क्तिं_लिख("R");
    }
    शून्य emitScIQW(AstVar* nodep) {
        UASSERT_OBJ(nodep->isSc(), nodep, "emitting SystemC operator on non-SC variable");
        पङ्क्तिं_लिख(nodep->isScBigUint()                           ? "SB"
             : (nodep->isScUint() || nodep->isScUintBool()) ? "SU"
             : nodep->isScBv()                              ? "SW"
             : nodep->isDouble()                            ? "SD"
                                                            : (nodep->isScQuad() ? "SQ" : "SI"));
    }
    शून्य emitDatap(ध्रुव Ast_ग्रन्थिः* nodep) {
        // When passing to a function with va_args the compiler doesn't
        // know need a pointer so when wide, need to look inside VlWide
        यदि (nodep->isWide()) पङ्क्तिं_लिख(".data()");
    }
    शून्य emitOpName(Ast_ग्रन्थिः* nodep, ध्रुव string& format, Ast_ग्रन्थिः* lhsp, Ast_ग्रन्थिः* rhsp,
                    Ast_ग्रन्थिः* thsp);
    शून्य emit_CCall_तर्काः(ध्रुव Ast_ग्रन्थिः_CCall* nodep, ध्रुव string& selfPointer, bool अन्तर्_Process);
    शून्य emitDereference(Ast_ग्रन्थिः* nodep, ध्रुव string& pointer);
    std::string dereferenceString(ध्रुव std::string& pointer) ध्रुव;
    शून्य emitCvtPackStr(Ast_ग्रन्थिः* nodep);
    शून्य emitConstant(AstConst* nodep);
    शून्य emitConstantString(ध्रुव AstConst* nodep);
    शून्य emitVarReset(ध्रुव string& उपसर्गः, AstVar* varp, bool constructing);
    string emitVarResetRecurse(ध्रुव AstVar* varp, bool constructing,
                               ध्रुव string& varNameProtected, Ast_ग्रन्थिः_DType* dtypep, पूर्णाङ्क गभीरता,
                               ध्रुव string& suffix, ध्रुव Ast_ग्रन्थिः* valuep);
    शून्य emitVarResetScopeHash();
    शून्य emitChangeDet();
    शून्य put_Comma_Iterate_अग्र(Ast_ग्रन्थिः* nodep, bool comma = false) {
        प्रति (Ast_ग्रन्थिः* subnodep = nodep; subnodep; subnodep = subnodep->nextp()) {
            यदि (comma) पङ्क्तिं_लिख(", ");
            iterateConst(subnodep);
            comma = true;
        }
    }
    प्रतिरूपम् <typename T>
    string optionalProcArg(ध्रुव T* ध्रुव nodep) {
        प्रत्यागम (nodep && constructorNeedsProcess(nodep)) ? "vlProcess, " : "";
    }
    ध्रुव AstCNew* getSuperNewCallRecursep(Ast_ग्रन्थिः* ध्रुव nodep) {
        // Get the super.new call
        यदि (!nodep) प्रत्यागम nullptr;
        यदि (ध्रुव AstCNew* ध्रुव cnewp = VN_CAST(nodep, CNew)) प्रत्यागम cnewp;
        यदि (ध्रुव Ast_Stmt_व्यञ्जकम्* ध्रुव stmtp = VN_CAST(nodep, Stmt_व्यञ्जकम्)) {
            यदि (ध्रुव AstCNew* ध्रुव cnewp = VN_CAST(stmtp->exprp(), CNew)) प्रत्यागम cnewp;
        }
        यदि (ध्रुव Ast_Jump_खण्डम्* ध्रुव blockp = VN_CAST(nodep, Jump_खण्डम्)) {
            यदि (ध्रुव AstCNew* ध्रुव cnewp = getSuperNewCallRecursep(blockp->stmtsp())) {
                प्रत्यागम cnewp;
            }
        }
        यदि (ध्रुव AstCNew* ध्रुव cnewp = getSuperNewCallRecursep(nodep->nextp())) प्रत्यागम cnewp;
        प्रत्यागम nullptr;
    }

    शून्य emitConstantW(AstConst* nodep, AstVarRef* assigntop) {
        // For tradition and compilation speed, assign each word directly into
        // output variable instead of using '='
        putns(nodep, "");
        यदि (nodep->अङ्कः().isFourState()) {
            nodep->v3warn(E_UNSUPPORTED, "Unsupported: 4-state numbers in this context");
            प्रत्यागम;
        }

        पूर्णाङ्क ऊर्ध्वम्_विस्तारः = nodep->अङ्कः().विस्तारः_गन्तव्यम्_Fit();
        पूर्णाङ्क chunks = 0;
        यदि (ऊर्ध्वम्_विस्तारः > EMITC_अङ्कः_CONSTW * VL_EDATASIZE) {
            // Output e.g. 8 words in groups of e.g. 8
            chunks = (ऊर्ध्वम्_विस्तारः - 1) / (EMITC_अङ्कः_CONSTW * VL_EDATASIZE);
            ऊर्ध्वम्_विस्तारः %= (EMITC_अङ्कः_CONSTW * VL_EDATASIZE);
            यदि (ऊर्ध्वम्_विस्तारः == 0) ऊर्ध्वम्_विस्तारः = (EMITC_अङ्कः_CONSTW * VL_EDATASIZE);
        }
        {  // Upper e.g. 8 words
            यदि (chunks) {
                putnbs(nodep, "VL_CONSTHI_W_");
                पङ्क्तिं_लिख(cvt_गन्तव्यम्_Str(VL_शब्दाः_I(ऊर्ध्वम्_विस्तारः)));
                पङ्क्तिं_लिख("X(");
                पङ्क्तिं_लिख(cvt_गन्तव्यम्_Str(nodep->विस्तारः_Min()));
                पङ्क्तिं_लिख(",");
                पङ्क्तिं_लिख(cvt_गन्तव्यम्_Str(chunks * EMITC_अङ्कः_CONSTW * VL_EDATASIZE));
            } अन्यथा {
                putnbs(nodep, "VL_CONST_W_");
                पङ्क्तिं_लिख(cvt_गन्तव्यम्_Str(VL_शब्दाः_I(ऊर्ध्वम्_विस्तारः)));
                पङ्क्तिं_लिख("X(");
                पङ्क्तिं_लिख(cvt_गन्तव्यम्_Str(nodep->विस्तारः_Min()));
            }
            पङ्क्तिं_लिख(",");
            यदि (!assigntop->selfPointer().is_रिक्तानि()) {
                emitDereference(assigntop, assigntop->selfPointerProtect(m_useSelfForThis));
            }
            पङ्क्तिं_लिख(varName(assigntop->varp()));
            प्रति (पूर्णाङ्क word = VL_शब्दाः_I(ऊर्ध्वम्_विस्तारः) - 1; word >= 0; word--) {
                // Only 32 bits - llx + long long here just to appease CPP format warning
                ofp()->प्रारूप_लिख(",0x%08" PRIx64, static_cast<uint64_t>(nodep->अङ्कः().edataWord(
                                                   word + chunks * EMITC_अङ्कः_CONSTW)));
            }
            पङ्क्तिं_लिख(")");
        }
        प्रति (chunks--; chunks >= 0; chunks--) {
            पङ्क्तिं_लिख(";\n");
            putbs("VL_CONSTLO_W_");
            पङ्क्तिं_लिख(cvt_गन्तव्यम्_Str(EMITC_अङ्कः_CONSTW));
            पङ्क्तिं_लिख("X(");
            पङ्क्तिं_लिख(cvt_गन्तव्यम्_Str(chunks * EMITC_अङ्कः_CONSTW * VL_EDATASIZE));
            पङ्क्तिं_लिख(",");
            यदि (!assigntop->selfPointer().is_रिक्तानि()) {
                emitDereference(assigntop, assigntop->selfPointerProtect(m_useSelfForThis));
            }
            पङ्क्तिं_लिख(varName(assigntop->varp()));
            प्रति (पूर्णाङ्क word = EMITC_अङ्कः_CONSTW - 1; word >= 0; word--) {
                // Only 32 bits - llx + long long here just to appease CPP format warning
                ofp()->प्रारूप_लिख(",0x%08" PRIx64, static_cast<uint64_t>(nodep->अङ्कः().edataWord(
                                                   word + chunks * EMITC_अङ्कः_CONSTW)));
            }
            पङ्क्तिं_लिख(")");
        }
    }

    शून्य emitNodesWithText(Ast_ग्रन्थिः* nodesp, bool useSelfForThis, bool tracking,
                           ध्रुव std::string& separator) {
        प्रति (Ast_ग्रन्थिः* nodep = nodesp; nodep; nodep = nodep->nextp()) {
            यदि (ध्रुव AstText* ध्रुव textp = VN_CAST(nodep, Text)) {
                ध्रुव std::string text
                    = VSelfPointerText::replaceThis(useSelfForThis, textp->text());
                यदि (tracking) {
                    पङ्क्तिं_लिख(text);
                } अन्यथा {
                    ofp()->putsNoTracking(text);
                }
            } अन्यथा {
                iterateConst(nodep);
            }
            यदि (nodep->nextp()) पङ्क्तिं_लिख(separator);
        }
    }

    शून्य putConstructorSubinit(ध्रुव AstClass* classp, AstCFunc* cfuncp) {
        // Direct non-virtual bases in declaration order
        प्रति (ध्रुव AstClassExtends* extp = classp->extendsp(); extp;
             extp = VN_AS(extp->nextp(), ClassExtends)) {
            यदि (extp->classp()->isInterfaceClass()) अनुवर्त;
            यदि (extp->classp()->use_Virtual_लौकिकम्()) अनुवर्त;
            पङ्क्तिं_लिख(EmitCUtil::उपसर्गः_Name_Protect(extp->classp()));
            पङ्क्तिं_लिख("::init");
            यदि (constructorNeedsProcess(extp->classp())) {
                पङ्क्तिं_लिख("(vlProcess, vlSymsp");
            } अन्यथा {
                पङ्क्तिं_लिख("(vlSymsp");
            }
            // Handle super.new() args for the concrete parent
            यदि (ध्रुव AstCNew* ध्रुव superNewCallp = getSuperNewCallRecursep(cfuncp->stmtsp())) {
                put_Comma_Iterate_अग्र(superNewCallp->argsp(), true);
            }
            पङ्क्तिं_लिख(");\n");
            // Since SystemVerilog allows only for extending from one class, for sure there won't
            // be any more constructors to be called. Even if there are some classes which are
            // "implemented", those are interface classes which do not have constructors.
            भङ्ग;
        }
    }

    // VISITORS
    using EmitCConstInit::दर्शय;
    शून्य दर्शय(AstCFunc* ध्रुव nodep) override {
        यदि (nodep->रिक्तानि_Body() && !nodep->isLoose()) प्रत्यागम;
        VL_RESTORER(m_useSelfForThis);
        VL_RESTORER(m_cfuncp);
        VL_RESTORER(m_instantiatesOwnProcess);
        VL_RESTORER(m_createdScopeHash);
        m_cfuncp = nodep;
        m_instantiatesOwnProcess = false;
        m_labelNumbers.clear();  // No need to save/restore, all Jumps must be within the function

        विभज_आकार_Inc(nodep);

        पङ्क्तिं_लिख("\n");
        m_lazyDecls.emit(nodep);
        यदि (nodep->ifdef() != "") putns(nodep, "#ifdef " + nodep->ifdef() + "\n");
        emit_CFunc_शीर्षकम्(nodep, m_modp, /* withScope: */ true);

        पङ्क्तिं_लिख(" {\n");
        यदि (nodep->isConstructor()) {
            ध्रुव AstClass* ध्रुव classp = VN_CAST(nodep->scopep()->modp(), Class);
            यदि (classp && classp->extendsp()) putConstructorSubinit(classp, nodep);
        }

        // "+" in the debug indicates a print from the model
        पङ्क्तिं_लिख("VL_DEBUG_IF(VL_DBG_MSGF(\"+  ");
        प्रति (पूर्णाङ्क i = 0; i < m_modp->level(); ++i) पङ्क्तिं_लिख("  ");
        पङ्क्तिं_लिख(EmitCUtil::उपसर्गः_Name_Protect(m_modp));
        पङ्क्तिं_लिख(nodep->isLoose() ? "__" : "::");
        पङ्क्तिं_लिख(nodep->nameProtect() + "\\n\"); );\n");

        यदि (nodep->isLoose()) {
            m_lazyDecls.declared(nodep);  // Defined here, so no longer needs declaration
            यदि (!nodep->isStatic()) {  // Standard prologue
                m_useSelfForThis = true;
                यदि (nodep->voidSelf()) {
                    ध्रुव string className = EmitCUtil::उपसर्गः_Name_Protect(m_modp);
                    पङ्क्तिं_लिख(className + "* const vlSelf = static_cast<" + className
                         + "*>(vlVoidSelf);\n");
                }
                यदि (!VN_IS(m_modp, Class)) {
                    पङ्क्तिं_लिख(EmitCUtil::sym_Class_नियोजय());  // Uses vlSelf
                } अन्यथा {
                    पङ्क्तिं_लिख("(void)vlSelf;  // Prevent unused variable warning\n");
                }
            }
        }

        // Instantiate a process class if it's going to be needed somewhere later
        nodep->forall([&](ध्रुव Ast_ग्रन्थिः_CCall* ccallp) -> bool {
            यदि (ccallp->funcp()->needProcess()
                && (ccallp->funcp()->isCoroutine() == VN_IS(ccallp->backp(), CAwait))) {
                यदि (!nodep->needProcess() && !m_instantiatesOwnProcess) {
                    m_instantiatesOwnProcess = true;
                    प्रत्यागम false;
                }
            }
            प्रत्यागम true;
        });
        यदि (m_instantiatesOwnProcess) {
            AstCStmt* ध्रुव vlprocp = new AstCStmt{nodep->fileline()};
            vlprocp->सङ्कलय("VlProcessRef vlProcess = std::make_shared<VlProcess>();\n");
            vlprocp->सङ्कलय("VlProcess::currentp(vlProcess.get());");
            nodep->prependStmtsp(vlprocp);
        } अन्यथा यदि (nodep->needProcess() && nodep->stmtsp()) {
            // Set current process so VlRNG() constructors in this function seed from it
            AstCStmt* ध्रुव समुच्चयः_Processp = new AstCStmt{nodep->fileline()};
            समुच्चयः_Processp->सङ्कलय("VlProcess::currentp(vlProcess.get());");
            nodep->prependStmtsp(समुच्चयः_Processp);
        }

        प्रति (Ast_ग्रन्थिः* subnodep = nodep->argsp(); subnodep; subnodep = subnodep->nextp()) {
            यदि (AstVar* ध्रुव varp = VN_CAST(subnodep, Var)) {
                यदि (varp->isFuncReturn()) emitVarDecl(varp);
            }
        }

        यदि (m_useSelfForThis) {
            m_usevlSelfRef = true;
            // Using reference to the vlSelf pointer will help the C++
            // compiler to have dereferenceable hints, which can help to
            // reduce the need for branch instructions in the generated
            // code to allow the compiler to generate load store after the
            // if condition (including short-circuit evaluation)
            // speculatively and also reduce the data cache pollution when
            // executing in the wrong path to make Verilated code faster.
            पङ्क्तिं_लिख("auto& vlSelfRef = std::ref(*vlSelf).get();\n");
        }

        यदि (nodep->varsp()) {
            putsDecoration(nodep, "// Locals\n");
            iterate_And_अग्र_Const_Null(nodep->varsp());
        }

        यदि (nodep->stmtsp()) {
            putsDecoration(nodep, "// Body\n");
            iterate_And_अग्र_Const_Null(nodep->stmtsp());
        }

        m_usevlSelfRef = false;

        यदि (nodep->isCoroutine()) {
            // Sometimes coroutines don't have co_awaits,
            // so emit a co_return at the end to avoid compile errors.
            पङ्क्तिं_लिख("co_return;\n");
        }

        पङ्क्तिं_लिख("}\n");
        यदि (nodep->ifdef() != "") पङ्क्तिं_लिख("#endif  // " + nodep->ifdef() + "\n");
    }

    शून्य दर्शय(AstVar* nodep) override {
        UASSERT_OBJ(m_cfuncp, nodep, "Cannot emit non-local variable");
        emitVarDecl(nodep);
    }

    शून्य दर्शय(Ast_Cvt_Array_गन्तव्यम्_Array* nodep) override {
        यदि (nodep->reverse()) {
            पङ्क्तिं_लिख("VL_REVCLONE_Q(");
        } अन्यथा {
            पङ्क्तिं_लिख("VL_CLONE_Q(");
        }
        iterate_And_अग्र_Const_Null(nodep->fromp());
        पङ्क्तिं_लिख(", ");
        पङ्क्तिं_लिख(cvt_गन्तव्यम्_Str(nodep->खण्डम्_आकार()));
        पङ्क्तिं_लिख(", ");
        पङ्क्तिं_लिख(cvt_गन्तव्यम्_Str(nodep->src_Element_द्वयङ्काः()));
        पङ्क्तिं_लिख(", ");
        पङ्क्तिं_लिख(cvt_गन्तव्यम्_Str(nodep->dst_Element_द्वयङ्काः()));
        पङ्क्तिं_लिख(")");
    }

    शून्य दर्शय(Ast_Cvt_Array_गन्तव्यम्_Packed* nodep) override {
        Ast_ग्रन्थिः_DType* ध्रुव fromDtp = nodep->fromp()->dtypep()->skipRefp();
        Ast_ग्रन्थिः_DType* ध्रुव elemDtp = fromDtp->subDTypep()->skipRefp();
        ध्रुव bool descending = [&]() {
            यदि (ध्रुव AstUnpackArrayDType* ध्रुव unpackDtp = VN_CAST(fromDtp, UnpackArrayDType))
                प्रत्यागम !unpackDtp->declRange().ascending();
            प्रत्यागम false;
        }();
        पङ्क्तिं_लिख("VL_PACK_");
        emitIQW(nodep);
        पङ्क्तिं_लिख("_");
        emitRU(fromDtp);
        emitIQW(elemDtp);
        यदि (descending) {
            // Wrap source in VL_PACK_REVERSED so VL_PACK sees ascending order
            emitOpName(nodep, "(%nw, %rw, %P, VL_PACK_REVERSED(%li))", nodep->fromp(), elemDtp,
                       nullptr);
        } अन्यथा {
            emitOpName(nodep, "(%nw, %rw, %P, %li)", nodep->fromp(), elemDtp, nullptr);
        }
    }

    शून्य दर्शय(Ast_Cvt_Unpack_गन्तव्यम्_Queue* nodep) override {
        Ast_ग्रन्थिः_DType* ध्रुव elemDTypep = nodep->fromp()->dtypep()->subDTypep();
        emitOpName(nodep, nodep->emitC(), nodep->fromp(), elemDTypep, nullptr);
    }
    शून्य दर्शय(Ast_Cvt_Unpack_गन्तव्यम्_Unpack* nodep) override {
        // Whether each dimension is reversed, the first for the outermost, up to the last that is
        std::सदिशम्<bool> reverses;
        ध्रुव AstUnpackArrayDType* शिखरम् = VN_AS(nodep->dtypep()->skipRefp(), UnpackArrayDType);
        ध्रुव AstUnpackArrayDType* fromp
            = VN_AS(nodep->fromp()->dtypep()->skipRefp(), UnpackArrayDType);
        यावत् (शिखरम्) {
            reverses.प्रक्षिप_पृष्ठभागः(शिखरम्->declRange().ascending() != fromp->declRange().ascending());
            शिखरम् = VN_CAST(शिखरम्->subDTypep()->skipRefp(), UnpackArrayDType);
            fromp = VN_CAST(fromp->subDTypep()->skipRefp(), UnpackArrayDType);
        }
        यावत् (!reverses.पृष्ठभागः()) reverses.निष्कासय_पृष्ठभागः();
        putns(nodep, "VL_CVT_UNPACK_TO_UNPACK<");
        प्रति (आकार_t i = 0; i < reverses.आकार(); ++i) {
            यदि (i) पङ्क्तिं_लिख(", ");
            पङ्क्तिं_लिख(reverses[i] ? "1" : "0");
        }
        पङ्क्तिं_लिख(">(");
        iterate_And_अग्र_Const_Null(nodep->fromp());
        पङ्क्तिं_लिख(")");
    }

    शून्य दर्शय(Ast_ग्रन्थिः_नियोजय* nodep) override {
        यदि (AstCReset* ध्रुव resetp = VN_CAST(nodep->rhsp(), CReset)) {
            // TODO get rid of emitVarReset and instead let AstNodeAssign understand how to init
            // anything
            Ast_ग्रन्थिः* fromp = nodep->lhsp();
            // Fork needs to use a member select.  Nothing else should be possible before VarRef.
            यदि (AstMemberSel* ध्रुव sfromp = VN_CAST(fromp, MemberSel)) {
                // Fork-DynScope generated pointer to previously automatic variable
                AstVar* ध्रुव memberVarp = sfromp->varp();
                fromp = sfromp->fromp();
                यदि (AstNullCheck* ध्रुव sfromp = VN_CAST(fromp, NullCheck)) fromp = sfromp->lhsp();
                Ast_ग्रन्थिः_Var_Ref* ध्रुव fromVarRefp = VN_AS(fromp, ग्रन्थिः_Var_Ref);
                emitVarReset(
                    ("VL_NULL_CHECK("s
                     + (fromVarRefp->selfPointer().is_रिक्तानि()
                            ? ""
                            : dereferenceString(fromVarRefp->selfPointerProtect(m_useSelfForThis)))
                     + varName(fromVarRefp->varp()) + ", \""
                     + V3_बहिर्गमनम्_Formatter::quoteNameControls(protect(nodep->fileline()->filename()))
                     + "\", " + std::गन्तव्यम्_string(nodep->fileline()->lineno()) + ")->"),
                    memberVarp, resetp->constructing());
            } अन्यथा {
                Ast_ग्रन्थिः_Var_Ref* ध्रुव fromVarRefp = VN_AS(fromp, ग्रन्थिः_Var_Ref);
                AstVar* ध्रुव varp = fromVarRefp->varp();
                ध्रुव string उपसर्गः
                    = fromVarRefp->selfPointer().is_रिक्तानि()
                          ? ""
                          : dereferenceString(
                                VN_AS(fromp, ग्रन्थिः_Var_Ref)->selfPointerProtect(m_useSelfForThis));
                emitVarReset(उपसर्गः, varp, resetp->constructing());
            }
            प्रत्यागम;
        }
        bool paren = true;
        bool decind = false;
        bool rhs = true;
        bool reverseUnpack = false;  // Set for descending CvtPackedToArray
        ध्रुव AstUnpackArrayDType* ध्रुव unpackDtp
            = VN_CAST(nodep->dtypep()->skipRefp(), UnpackArrayDType);
        यदि (AstSel* ध्रुव selp = VN_CAST(nodep->lhsp(), Sel)) {
            UASSERT_OBJ(selp->विस्तारः_Min() == selp->विस्तारः_Const(), selp, "Width mismatch");
            यदि (selp->विस्तारः_Min() == 1) {
                putnbs(nodep, "VL_ASSIGNBIT_");
                emitIQW(selp->fromp());
                यदि (nodep->rhsp()->isAllOnesV()) {
                    पङ्क्तिं_लिख("O(");
                    rhs = false;
                } अन्यथा {
                    पङ्क्तिं_लिख("I(");
                }
                iterate_And_अग्र_Const_Null(selp->lsbp());
                पङ्क्तिं_लिख(", ");
                iterate_And_अग्र_Const_Null(selp->fromp());
                यदि (rhs) पङ्क्तिं_लिख(", ");
            } अन्यथा {
                putnbs(nodep, "VL_ASSIGNSEL_");
                emitIQW(selp->fromp());
                emitIQW(nodep->rhsp());
                पङ्क्तिं_लिख("(");
                putns(selp->fromp(), cvt_गन्तव्यम्_Str(selp->fromp()->विस्तारः_Min()) + ", ");
                पङ्क्तिं_लिख(cvt_गन्तव्यम्_Str(nodep->विस्तारः_Min()) + ", ");
                iterate_And_अग्र_Const_Null(selp->lsbp());
                पङ्क्तिं_लिख(", ");
                iterate_And_अग्र_Const_Null(selp->fromp());
                पङ्क्तिं_लिख(", ");
            }
        } अन्यथा यदि (ध्रुव AstGetcRefN* ध्रुव selp = VN_CAST(nodep->lhsp(), GetcRefN)) {
            iterate_And_अग्र_Const_Null(selp->lhsp());
            पङ्क्तिं_लिख(" = ");
            putnbs(selp, "VL_PUTC_N(");
            iterate_And_अग्र_Const_Null(selp->lhsp());
            पङ्क्तिं_लिख(", ");
            iterate_And_अग्र_Const_Null(selp->rhsp());
            पङ्क्तिं_लिख(", ");
        } अन्यथा यदि (AstVar* ध्रुव varp = AstVar::scVarRecurse(nodep->lhsp())) {
            putnbs(varp, "VL_ASSIGN_");  // Set a systemC variable
            emitScIQW(varp);
            emitIQW(nodep);
            पङ्क्तिं_लिख("(");
            पङ्क्तिं_लिख(cvt_गन्तव्यम्_Str(nodep->विस्तारः_Min()) + ", ");
            iterate_And_अग्र_Const_Null(nodep->lhsp());
            पङ्क्तिं_लिख(", ");
        } अन्यथा यदि (AstVar* ध्रुव varp = AstVar::scVarRecurse(nodep->rhsp())) {
            putnbs(varp, "VL_ASSIGN_");  // Get a systemC variable
            emitIQW(nodep);
            emitScIQW(varp);
            पङ्क्तिं_लिख("(");
            पङ्क्तिं_लिख(cvt_गन्तव्यम्_Str(nodep->विस्तारः_Min()) + ", ");
            iterate_And_अग्र_Const_Null(nodep->lhsp());
            पङ्क्तिं_लिख(", ");
        } अन्यथा यदि (ध्रुव Ast_Cvt_Packed_गन्तव्यम्_Array* ध्रुव castp
                   = VN_CAST(nodep->rhsp(), Cvt_Packed_गन्तव्यम्_Array)) {
            putns(castp, "VL_UNPACK_");
            emitRU(nodep);
            emitIQW(nodep->dtypep()->subDTypep());
            पङ्क्तिं_लिख("_");
            emitRU(castp->fromp());
            emitIQW(castp->fromp());
            पङ्क्तिं_लिख("(");
            putns(castp->dtypep(), cvt_गन्तव्यम्_Str(castp->dtypep()->subDTypep()->विस्तारः_Min()));
            पङ्क्तिं_लिख(", ");
            पङ्क्तिं_लिख(cvt_गन्तव्यम्_Str(castp->fromp()->विस्तारः_Min()));
            पङ्क्तिं_लिख(", ");
            iterate_And_अग्र_Const_Null(nodep->lhsp());
            पङ्क्तिं_लिख(", ");
            rhs = false;
            iterate_And_अग्र_Const_Null(castp->fromp());
            // Descending unpacked dest: reverse after unpack
            यदि (unpackDtp && !unpackDtp->declRange().ascending()) reverseUnpack = true;
        } अन्यथा यदि (ध्रुव Ast_Cvt_Array_गन्तव्यम्_Array* ध्रुव castp
                   = VN_CAST(nodep->rhsp(), Cvt_Array_गन्तव्यम्_Array)) {
            यदि (castp->reverse()) {
                putns(castp, "VL_REVCOPY_Q(");
            } अन्यथा {
                putns(castp, "VL_COPY_Q(");
            }
            iterate_And_अग्र_Const_Null(nodep->lhsp());
            पङ्क्तिं_लिख(", ");
            rhs = false;
            iterate_And_अग्र_Const_Null(castp->fromp());
            पङ्क्तिं_लिख(", ");
            पङ्क्तिं_लिख(cvt_गन्तव्यम्_Str(castp->खण्डम्_आकार()));
            पङ्क्तिं_लिख(", ");
            पङ्क्तिं_लिख(cvt_गन्तव्यम्_Str(castp->src_Element_द्वयङ्काः()));
            पङ्क्तिं_लिख(", ");
            पङ्क्तिं_लिख(cvt_गन्तव्यम्_Str(castp->dst_Element_द्वयङ्काः()));
        } अन्यथा यदि (nodep->isWide() && VN_IS(nodep->lhsp(), VarRef)  //
                   && !VN_IS(nodep->rhsp(), CExpr)  //
                   && !VN_IS(nodep->rhsp(), CMethodHard)  //
                   // Although not here currently, note putting !VN_IS(Const) works,
                   // and means using '=' and bypasses using emitConstantW.
                   // Whuch we don't want to do as slows compiler down.
                   && !VN_IS(nodep->rhsp(), VarRef)  //
                   && !VN_IS(nodep->rhsp(), ConsArray)  //
                   && !VN_IS(nodep->rhsp(), AssocSel)  //
                   && !VN_IS(nodep->rhsp(), MemberSel)  //
                   && !VN_IS(nodep->rhsp(), StructSel)  //
                   && !VN_IS(nodep->rhsp(), ArraySel)  //
                   && !VN_IS(nodep->rhsp(), व्यञ्जकम्_Stmt)) {
            // Wide functions assign into the array directly, don't need separate assign statement
            m_wide_अस्थायी_Refp = VN_AS(nodep->lhsp(), VarRef);
            paren = false;
        } अन्यथा यदि (nodep->isWide() && !unpackDtp && !VN_IS(nodep->rhsp(), Const)) {
            putnbs(nodep, "VL_ASSIGN_W(");
            पङ्क्तिं_लिख(cvt_गन्तव्यम्_Str(nodep->विस्तारः_Min()) + ", ");
            iterate_And_अग्र_Const_Null(nodep->lhsp());
            पङ्क्तिं_लिख(", ");
        } अन्यथा यदि (VN_IS(nodep->lhsp()->dtypep()->skipRefp(), QueueDType)
                   && (VN_IS(nodep->rhsp(), प्रवाहः_L) || VN_IS(nodep->lhsp(), प्रवाहः_L)
                       || VN_IS(nodep->rhsp(), प्रवाहः_R) || VN_IS(nodep->lhsp(), प्रवाहः_R)
                       || VN_IS(nodep->rhsp(), प्रवाहः_R))) {
            //if either side is streamL or streamR don't emit lhsp everything will be passed by
            //reference

            paren = false;

        } अन्यथा {
            paren = false;
            iterate_And_अग्र_Const_Null(nodep->lhsp());
            पङ्क्तिं_लिख(" ");
            ofp()->खण्डम्_Inc();
            decind = true;
            यदि (!VN_IS(nodep->rhsp(), Const)) ofp()->putBreak();
            putns(nodep, "= ");
        }
        यदि (rhs) iterate_And_अग्र_Const_Null(nodep->rhsp());
        यदि (paren) पङ्क्तिं_लिख(")");
        यदि (decind) ofp()->खण्डम्_Dec();
        पङ्क्तिं_लिख(";\n");
        यदि (reverseUnpack) {
            पङ्क्तिं_लिख("VL_UNPACK_REVERSED(");
            iterate_And_अग्र_Const_Null(nodep->lhsp());
            पङ्क्तिं_लिख(");\n");
        }
    }
    शून्य दर्शय(AstAssocSel* nodep) override {
        iterate_And_अग्र_Const_Null(nodep->fromp());
        ध्रुव std::string atFunc = nodep->isLValue() ? ".atWrite(" : ".at(";
        putnbs(nodep, atFunc);
        AstAssocArrayDType* ध्रुव adtypep
            = VN_AS(nodep->fromp()->dtypep()->skipRefp(), AssocArrayDType);
        UASSERT_OBJ(adtypep, nodep, "Associative select on non-associative type");
        iterate_And_अग्र_Const_Null(nodep->bitp());
        पङ्क्तिं_लिख(")");
    }
    शून्य दर्शय(AstWildcardSel* nodep) override {
        iterate_And_अग्र_Const_Null(nodep->fromp());
        ध्रुव std::string atFunc = nodep->isLValue() ? ".atWrite(" : ".at(";
        putnbs(nodep, atFunc);
        AstWildcardArrayDType* ध्रुव adtypep
            = VN_AS(nodep->fromp()->dtypep()->skipRefp(), WildcardArrayDType);
        UASSERT_OBJ(adtypep, nodep, "Wildcard select on non-wildcard-associative type");
        iterate_And_अग्र_Const_Null(nodep->bitp());
        पङ्क्तिं_लिख(")");
    }
    शून्य दर्शय(AstCCall* nodep) override {
        ध्रुव AstCFunc* ध्रुव funcp = nodep->funcp();
        ध्रुव Ast_ग्रन्थिः_Module* ध्रुव funcModp = EmitCParentModule::get(funcp);
        putnbs(nodep, "");
        यदि (funcp->dpiImportPrototype()) {
            // Calling DPI import
            putns(nodep, funcp->name());
        } अन्यथा यदि (funcp->is_Proper_विधानम्() && funcp->isStatic()) {
            // Call static method via the containing class
            putns(funcModp, EmitCUtil::उपसर्गः_Name_Protect(funcModp) + "::");
            putns(nodep, funcp->nameProtect());
        } अन्यथा यदि (nodep->superReference()) {
            // Calling superclass method
            putns(funcModp, EmitCUtil::उपसर्गः_Name_Protect(funcModp) + "::");
            putns(nodep, funcp->nameProtect());
        } अन्यथा यदि (funcp->isLoose()) {
            // Calling loose method
            putns(nodep, funcNameProtect(funcp));
        } अन्यथा {
            // Calling regular method/function
            यदि (!nodep->selfPointer().is_रिक्तानि()) {
                emitDereference(nodep, nodep->selfPointerProtect(m_useSelfForThis));
            }
            putns(nodep, funcp->nameProtect());
        }
        emit_CCall_तर्काः(nodep, nodep->selfPointerProtect(m_useSelfForThis), m_cfuncp->needProcess());
    }
    शून्य दर्शय(AstCMethodCall* nodep) override {
        ध्रुव AstCFunc* ध्रुव funcp = nodep->funcp();
        UASSERT_OBJ(!funcp->isLoose(), nodep, "Loose method called via AstCMethodCall");
        iterateConst(nodep->fromp());
        putnbs(nodep, "->");
        यदि (nodep->superReference()) {
            // Super call, so the base implementation is used rather than the override
            ध्रुव Ast_ग्रन्थिः_Module* ध्रुव funcModp = EmitCParentModule::get(funcp);
            putns(funcModp, EmitCUtil::उपसर्गः_Name_Protect(funcModp) + "::");
        }
        putns(funcp, funcp->nameProtect());
        emit_CCall_तर्काः(nodep, "", m_cfuncp->needProcess());
    }
    शून्य दर्शय(AstCAwait* nodep) override {
        putns(nodep, "co_await ");
        iterateConst(nodep->exprp());
        पङ्क्तिं_लिख(";\n");
    }
    शून्य दर्शय(AstCNew* nodep) override {
        यदि (VN_IS(nodep->dtypep(), VoidDType)) {  // super.new case
            putsDecoration(nodep, "/*super.new*/");
            प्रत्यागम;
        }
        // assignment case;
        putns(nodep, "VL_NEW(" + EmitCUtil::उपसर्गः_Name_Protect(nodep->dtypep()) + ", "
                         + optionalProcArg(nodep->dtypep()) + "vlSymsp");
        put_Comma_Iterate_अग्र(nodep->argsp(), true);
        पङ्क्तिं_लिख(")");
    }
    शून्य दर्शय(AstCMethodHard* nodep) override {
        iterateConst(nodep->fromp());
        putns(nodep, nodep->usePtr() ? "->" : ".");
        putns(nodep, nodep->name());
        यदि (nodep->विधानम्() == VCMethod::FORCE_READ_SEL) {
            emitIQW(nodep);
            यदि (nodep->isWide()) पङ्क्तिं_लिख("<" + cvt_गन्तव्यम्_Str(nodep->dtypep()->विस्तारः_शब्दाः()) + ">");
        } अन्यथा यदि (nodep->विधानम्() == VCMethod::ARRAY_खण्डिका) {
            // VlUnpacked::slice<N_Out>(loIdx) - N_Out is the (fixed) result array size
            ध्रुव AstUnpackArrayDType* ध्रुव adtypep
                = VN_AS(nodep->dtypep()->skipRefp(), UnpackArrayDType);
            पङ्क्तिं_लिख("<" + cvt_गन्तव्यम्_Str(adtypep->elementsConst()) + ">");
        } अन्यथा यदि (nodep->विधानम्() == VCMethod::COVERGROUP_सङ्कलय_COVERPOINT) {
            // The hit-list bound is a template argument of the returned VlCoverpointT<>, and the
            // node's own dtype is that type, so it is the one source of truth for both.
            ध्रुव AstCoverpointDType* ध्रुव cpdtypep
                = VN_AS(nodep->dtypep()->skipRefp(), CoverpointDType);
            पङ्क्तिं_लिख("<" + cvt_गन्तव्यम्_Str(cpdtypep->लब्धम्_Bound()) + ">");
        } अन्यथा यदि (nodep->विधानम्() == VCMethod::COVERGROUP_सङ्कलय_CROSS) {
            ध्रुव AstCoverCrossDType* ध्रुव cxdtypep
                = VN_AS(nodep->dtypep()->skipRefp(), CoverCrossDType);
            पङ्क्तिं_लिख("<" + cxdtypep->cpp_Template_तर्काः() + ">");
        }
        पङ्क्तिं_लिख("(");
        bool comma = false;
        पूर्णाङ्क arg_अङ्कः = 0;
        प्रति (Ast_ग्रन्थिः* subnodep = nodep->pinsp(); subnodep; subnodep = subnodep->nextp()) {
            यदि (comma) पङ्क्तिं_लिख(", ");
            यदि (nodep->विधानम्() == VCMethod::RANDOMIZER_HARD && arg_अङ्कः == 1) {
                // For RANDOMIZER_HARD's filename argument (2nd arg after constraint),
                // apply protect() similar to VL_STOP to handle --protected flag
                यदि (ध्रुव AstCExpr* ध्रुव cexprp = VN_CAST(subnodep, CExpr)) {
                    // Extract filename from the CExpr (which contains "filename")
                    std::string filename;
                    प्रति (ध्रुव Ast_ग्रन्थिः* textnodep = cexprp->nodesp(); textnodep;
                         textnodep = textnodep->nextp()) {
                        यदि (ध्रुव AstText* ध्रुव textp = VN_CAST(textnodep, Text)) {
                            filename = textp->text();
                            भङ्ग;
                        }
                    }
                    // Remove surrounding quotes if present
                    यदि (filename.आकार() >= 2 && filename.front() == '"'
                        && filename.पृष्ठभागः() == '"') {
                        filename = filename.substr(1, filename.आकार() - 2);
                    }
                    // Emit with protect()
                    putsQuoted(protect(filename));
                } अन्यथा {
                    iterateConst(subnodep);
                }
            } अन्यथा {
                iterateConst(subnodep);
            }
            comma = true;
            arg_अङ्कः++;
        }
        यदि (nodep->withp()) {
            यदि (comma) पङ्क्तिं_लिख(", ");
            iterateConst(nodep->withp());
        }
        पङ्क्तिं_लिख(")");
    }
    शून्य दर्शय(AstCFuncHard* nodep) override {
        putns(nodep, nodep->name());
        पङ्क्तिं_लिख("(");
        put_Comma_Iterate_अग्र(nodep->pinsp());
        पङ्क्तिं_लिख(")");
    }
    शून्य दर्शय(AstLambdaArgRef* nodep) override { putbs(nodep->nameProtect()); }
    शून्य दर्शय(AstWith* nodep) override {
        // With uses a C++11 lambda
        putnbs(nodep, "[&](");
        यदि (स्वचालित* ध्रुव argrefp = nodep->सूचकांक_Arg_Refp()) {
            putnbs(argrefp, argrefp->dtypep()->cType(argrefp->nameProtect(), false, false));
            पङ्क्तिं_लिख(", ");
        }
        यदि (स्वचालित* ध्रुव argrefp = nodep->मानम्_Arg_Refp()) {
            putnbs(argrefp, argrefp->dtypep()->cType(argrefp->nameProtect(), false, false));
        }
        पङ्क्तिं_लिख(") -> ");
        putnbs(nodep, nodep->dtypep()->cType("", false, false));
        पङ्क्तिं_लिख(" {\n");
        VL_RESTORER(m_createdScopeHash);
        iterate_And_अग्र_Const_Null(nodep->exprp());
        पङ्क्तिं_लिख("}\n");
    }
    शून्य दर्शय(AstCase* nodep) override {  // LCOV_EXCL_LINE
        // In V3Case...
        nodep->v3fatalSrc("Case statements should have been reduced out");
    }
    शून्य दर्शय(AstComment* nodep) override {
        string at;
        यदि (nodep->showAt()) {
            at = " at " + nodep->fileline()->ascii();
            // If protecting, passthru less information about the design
            यदि (!v3Global.opt.protectIds()) प्रत्यागम;
        }
        यदि (!(nodep->protect() && v3Global.opt.protectIds())) {
            putsDecoration(nodep, "// "s + nodep->name() + at + "\n");
        }
        iterateChildrenConst(nodep);
    }
    शून्य दर्शय(AstCoverOtherDecl* nodep) override {
        emit_Cover_Other_Decl_निवेशय(nodep);
        यदि (coverage_उपयुङ्क्ते_Local_गणक(nodep)) {
            // Some task bodies are cloned into a caller module where the
            // original instance-local counter is not addressable. Those clones
            // fall back to the global bin number, so register that slot too.
            emit_Cover_Other_Decl_निवेशय(nodep, true);
        }
    }
    शून्य दर्शय(AstCoverToggleDecl* nodep) override {
        putns(nodep, "vlSelf->__vlCoverToggleInsert(");  // As Declared in emitCoverageDecl
        पङ्क्तिं_लिख(cvt_गन्तव्यम्_Str(nodep->range().दक्षिणः()));
        पङ्क्तिं_लिख(", ");
        पङ्क्तिं_लिख(cvt_गन्तव्यम्_Str(nodep->range().वामः()));
        पङ्क्तिं_लिख(", ");
        पङ्क्तिं_लिख(cvt_गन्तव्यम्_Str(nodep->range().ranged()));
        पङ्क्तिं_लिख(", ");
        putCoverageArray(nodep);
        पङ्क्तिं_लिख(" + ");
        पङ्क्तिं_लिख(cvt_गन्तव्यम्_Str(coverage_Bin_अङ्कः(nodep)));
        // first controls whether duplicate module instances are collapsed in
        // the default coverage view. The following flag says whether countp
        // points to an object-local counter; only those counters must be kept
        // real for later instances so forcePerInstance can split hierarchy.
        पङ्क्तिं_लिख(", first");  // Enable, passed from __Vconfigure parameter
        पङ्क्तिं_लिख(", ");
        पङ्क्तिं_लिख(coverage_उपयुङ्क्ते_Local_गणक(nodep) ? "true" : "false");
        पङ्क्तिं_लिख(", ");
        putsQuoted(protect(nodep->fileline()->filename()));
        पङ्क्तिं_लिख(", ");
        पङ्क्तिं_लिख(cvt_गन्तव्यम्_Str(nodep->fileline()->lineno()));
        पङ्क्तिं_लिख(", ");
        पङ्क्तिं_लिख(cvt_गन्तव्यम्_Str(nodep->fileline()->firstColumn()));
        पङ्क्तिं_लिख(", ");
        putsQuoted((!nodep->hier().रिक्तानि() ? "." : "")
                   + VIdProtect::protect_शब्दाः_If(nodep->hier(), nodep->protect()));
        पङ्क्तिं_लिख(", ");
        putsQuoted(VIdProtect::protect_शब्दाः_If(nodep->page(), nodep->protect()));
        पङ्क्तिं_लिख(", ");
        putsQuoted(VIdProtect::protect_शब्दाः_If(nodep->comment(), nodep->protect()));
        पङ्क्तिं_लिख(");\n");
    }
    शून्य दर्शय(AstCoverInc* nodep) override {
        यदि (VN_IS(nodep->declp(), CoverOtherDecl)) {
            यदि (v3Global.opt.तन्तवः() > 1) {
                putns(nodep, "");
                putCoverageArray(nodep->declp());
                पङ्क्तिं_लिख("[");
                पङ्क्तिं_लिख(cvt_गन्तव्यम्_Str(coverage_Bin_अङ्कः(nodep->declp())));
                पङ्क्तिं_लिख("].fetch_add(1, std::memory_order_relaxed);\n");
            } अन्यथा {
                putns(nodep, "++(");
                putCoverageArray(nodep->declp());
                पङ्क्तिं_लिख("[");
                पङ्क्तिं_लिख(cvt_गन्तव्यम्_Str(coverage_Bin_अङ्कः(nodep->declp())));
                पङ्क्तिं_लिख("]);\n");
            }
        } अन्यथा {
            पङ्क्तिं_लिख("VL_COV_TOGGLE_CHG_");
            यदि (v3Global.opt.तन्तवः() > 1) {
                पङ्क्तिं_लिख("MT_");
            } अन्यथा {
                पङ्क्तिं_लिख("ST_");
            }
            emitIQW(nodep->toggleExprp());
            पङ्क्तिं_लिख("(");
            // Each bit of variable has 2 counters, so the size of variable is half of the size of
            // coverpoint
            पङ्क्तिं_लिख(cvt_गन्तव्यम्_Str(nodep->declp()->आकार() / 2));
            पङ्क्तिं_लिख(", ");
            // Toggle update uses the same object-local counter array that
            // __vlCoverToggleInsert registered.
            putCoverageArray(nodep->declp());
            पङ्क्तिं_लिख(" + ");
            पङ्क्तिं_लिख(cvt_गन्तव्यम्_Str(coverage_Bin_अङ्कः(nodep->declp())));
            पङ्क्तिं_लिख(", ");
            iterateConst(nodep->toggleExprp());
            पङ्क्तिं_लिख(", ");
            iterateConst(nodep->toggleCovExprp());
            पङ्क्तिं_लिख(");\n");
        }
    }
    शून्य दर्शय(AstDisableFork* nodep) override { putns(nodep, "vlProcess->disableFork();\n"); }
    शून्य दर्शय(AstCReturn* nodep) override {
        putns(nodep, "return (");
        iterate_And_अग्र_Const_Null(nodep->lhsp());
        पङ्क्तिं_लिख(");\n");
    }
    शून्य दर्शय(AstDisplay* nodep) override {
        string text = nodep->fmtp()->text();
        यदि (nodep->सङ्कलय_Newline()) text += "\n";
        display_ग्रन्थिः(nodep, nodep->fmtp(), text, nodep->fmtp()->exprsp(), false);
    }
    शून्य दर्शय(AstDumpCtl* nodep) override {
        विकल्प (nodep->ctlType()) {
        पक्ष VDumpCtlType::FILE:
            putns(nodep, "vlSymsp->_vm_contextp__->dumpfile(");
            iterateConst(nodep->exprp());
            पङ्क्तिं_लिख(");\n");
            भङ्ग;
        पक्ष VDumpCtlType::VARS:
            // We ignore number of levels to dump in exprp()
            यदि (v3Global.opt.trace()) {
                putns(nodep, "vlSymsp->_traceDumpOpen();\n");
            } अन्यथा {
                putns(nodep, "VL_PRINTF_MT(\"-Info: ");
                पङ्क्तिं_लिख(V3_बहिर्गमनम्_Formatter::quoteNameControls(protect(nodep->fileline()->filename())));
                पङ्क्तिं_लिख(":");
                पङ्क्तिं_लिख(cvt_गन्तव्यम्_Str(nodep->fileline()->lineno()));
                पङ्क्तिं_लिख(": $dumpvar ignored, as Verilated without --trace");
                पङ्क्तिं_लिख("\\n\");\n");
            }
            भङ्ग;
        पक्ष VDumpCtlType::ALL:
            // $dumpall currently ignored
            भङ्ग;
        पक्ष VDumpCtlType::FLUSH:
            // $dumpall currently ignored; would need rework of VCD single thread,
            // or flag we pass-through to next eval() iteration
            भङ्ग;
        पक्ष VDumpCtlType::सीमा_2:
            // $dumplimit currently ignored
            भङ्ग;
        पक्ष VDumpCtlType::OFF:
            // Currently ignored as both Vcd and Fst do not support them, as would need "X" dump
            भङ्ग;
        पक्ष VDumpCtlType::ON:
            // Currently ignored as $dumpoff is also ignored
            भङ्ग;
        स्वभाव: nodep->v3fatalSrc("Bad case, unexpected " << nodep->ctlType().ascii());
        }
    }
    शून्य दर्शय(AstScopeName* nodep) override {
        // For use under AstCCalls for dpiImports.  ScopeNames under
        // displays are handled in AstDisplay
        यदि (!nodep->dpiExport()) {
            // this is where the DPI import context scope is set
            ध्रुव string scope = nodep->scopeDpiName();
            putnbs(nodep, "(vlSymsp->" + protect("__Vscopep_" + scope) + ")");
        }
    }
    शून्य दर्शय(AstSFormat* nodep) override {
        display_ग्रन्थिः(nodep, nodep->fmtp(), nodep->fmtp()->text(), nodep->fmtp()->exprsp(), false);
    }
    शून्य दर्शय(AstSFormatF* nodep) override {
        display_ग्रन्थिः(nodep, nodep, nodep->text(), nodep->exprsp(), false);
    }
    शून्य दर्शय(AstFScanF* nodep) override {
        display_ग्रन्थिः(nodep, nullptr, nodep->text(), nodep->exprsp(), true);
    }
    शून्य दर्शय(AstSScanF* nodep) override {
        display_ग्रन्थिः(nodep, nullptr, nodep->text(), nodep->exprsp(), true);
    }
    शून्य दर्शय(Ast_मानम्_Plus_तर्काः* nodep) override {
        putns(nodep, "VL_VALUEPLUSARGS_IN");
        emitIQW(nodep->outp());
        पङ्क्तिं_लिख("(");
        पङ्क्तिं_लिख(cvt_गन्तव्यम्_Str(nodep->outp()->विस्तारः_Min()));
        पङ्क्तिं_लिख(", ");
        iterateConst(nodep->searchp());
        पङ्क्तिं_लिख(", ");
        putbs("");
        iterate_And_अग्र_Const_Null(nodep->outp());
        पङ्क्तिं_लिख(")");
    }
    शून्य दर्शय(Ast_Test_Plus_तर्काः* nodep) override {
        putns(nodep, "VL_TESTPLUSARGS_I(");
        iterateConst(nodep->searchp());
        पङ्क्तिं_लिख(")");
    }
    शून्य दर्शय(AstFError* nodep) override {
        putns(nodep, "VL_FERROR_I");
        पङ्क्तिं_लिख(nodep->strp()->isString() ? "N(" : "W(");
        iterate_And_अग्र_Const_Null(nodep->filep());
        putbs(", ");
        यदि (nodep->strp()->isWide()) {
            पङ्क्तिं_लिख(cvt_गन्तव्यम्_Str(nodep->strp()->विस्तारः_शब्दाः()));
            putbs(", ");
        }
        iterate_And_अग्र_Const_Null(nodep->strp());
        पङ्क्तिं_लिख(")");
    }
    शून्य दर्शय(AstFGetS* nodep) override {
        emitOpName(nodep, nodep->emitC(), nodep->strgp(), nodep->filep(), nullptr);
    }
    शून्य दर्शय(AstFOpen* nodep) override {
        putns(nodep, "VL_FOPEN_NN(");
        iterateConst(nodep->filenamep());
        putbs(", ");
        iterateConst(nodep->modep());
        पङ्क्तिं_लिख(");\n");
    }
    शून्य दर्शय(AstFOpenMcd* nodep) override {
        putns(nodep, "VL_FOPEN_MCD_N(");
        iterateConst(nodep->filenamep());
        पङ्क्तिं_लिख(");\n");
    }
    शून्य दर्शय(Ast_ग्रन्थिः_Read_Write_Mem* nodep) override {
        putns(nodep, nodep->cFuncPrefixp());
        पङ्क्तिं_लिख("N(");
        पङ्क्तिं_लिख(nodep->isHex() ? "true" : "false");
        putbs(", ");
        // Need real storage width
        पङ्क्तिं_लिख(cvt_गन्तव्यम्_Str(nodep->memp()->dtypep()->skipRefp()->subDTypep()->विस्तारः_Min()));
        uint32_t array_lo = 0;
        bool memUsePtr = true;
        {
            ध्रुव AstVarRef* ध्रुव varrefp = VN_CAST(nodep->memp(), VarRef);
            यदि (!varrefp) {
                nodep->v3error(nodep->verilogKwd() << " loading non-variable");
            } अन्यथा यदि (VN_IS(varrefp->varp()->dtypeSkipRefp(), AssocArrayDType)
                       || VN_IS(varrefp->varp()->dtypeSkipRefp(), DynArrayDType)) {
                // nodep->memp() below will when verilated code is compiled create a C++ template
                memUsePtr = false;
            } अन्यथा यदि (ध्रुव AstUnpackArrayDType* ध्रुव adtypep
                       = VN_CAST(varrefp->varp()->dtypeSkipRefp(), UnpackArrayDType)) {
                putbs(", ");
                पङ्क्तिं_लिख(cvt_गन्तव्यम्_Str(varrefp->varp()->dtypep()->arrayUnpackedElements()));
                array_lo = adtypep->lo();
                putbs(", ");
                पङ्क्तिं_लिख(cvt_गन्तव्यम्_Str(array_lo));
            } अन्यथा {
                nodep->v3error(
                    nodep->verilogKwd()
                    << " loading other than unpacked/dynamic/associative-array variable");
            }
        }
        putbs(", ");
        iterateConst(nodep->filenamep());
        putbs(", ");
        {
            यदि (memUsePtr) पङ्क्तिं_लिख(" &(");
            iterate_And_अग्र_Const_Null(nodep->memp());
            यदि (memUsePtr) पङ्क्तिं_लिख(")");
        }
        putbs(", ");
        यदि (nodep->lsbp()) {
            iterate_And_अग्र_Const_Null(nodep->lsbp());
        } अन्यथा {
            पङ्क्तिं_लिख(cvt_गन्तव्यम्_Str(array_lo));
        }
        putbs(", ");
        यदि (nodep->msbp()) {
            iterate_And_अग्र_Const_Null(nodep->msbp());
        } अन्यथा {
            पङ्क्तिं_लिख("~0ULL");
        }
        पङ्क्तिं_लिख(");\n");
    }
    शून्य दर्शय(AstFClose* nodep) override {
        putns(nodep, "VL_FCLOSE_I(");
        iterate_And_अग्र_Const_Null(nodep->filep());
        पङ्क्तिं_लिख("); ");
    }
    शून्य दर्शय(AstFFlush* nodep) override {
        यदि (!nodep->filep()) {
            putns(nodep, "VL_FFLUSH_MT();");
        } अन्यथा {
            putns(nodep, "VL_FFLUSH_I(");
            iterate_And_अग्र_Const_Null(nodep->filep());
            पङ्क्तिं_लिख(");\n");
        }
    }
    शून्य दर्शय(AstFSeek* nodep) override {
        putns(nodep, "(VL_FSEEK_I(");
        iterate_And_अग्र_Const_Null(nodep->filep());
        पङ्क्तिं_लिख(", ");
        iterate_And_अग्र_Const_Null(nodep->विस्थापनम्());
        पङ्क्तिं_लिख(", ");
        iterate_And_अग्र_Const_Null(nodep->operation());
        पङ्क्तिं_लिख(") == -1 ? -1 : 0)");
    }
    शून्य दर्शय(AstFTell* nodep) override {
        putns(nodep, "VL_FTELL_I(");
        iterate_And_अग्र_Const_Null(nodep->filep());
        पङ्क्तिं_लिख(")");
    }
    शून्य दर्शय(AstFRewind* nodep) override {
        putns(nodep, "(VL_FSEEK_I(");
        iterate_And_अग्र_Const_Null(nodep->filep());
        पङ्क्तिं_लिख(", 0, 0) == -1 ? -1 : 0)");
    }
    शून्य दर्शय(AstFRead* nodep) override {
        putns(nodep, "VL_FREAD_I(");
        पङ्क्तिं_लिख(cvt_गन्तव्यम्_Str(nodep->memp()->विस्तारः_Min()));  // Need real storage width
        putbs(", ");
        uint32_t array_lo = 0;
        uint32_t array_आकार = 0;
        {
            ध्रुव AstVarRef* ध्रुव varrefp = VN_CAST(nodep->memp(), VarRef);
            यदि (!varrefp) {
                nodep->v3error(nodep->verilogKwd() << " loading non-variable");
            } अन्यथा यदि (VN_CAST(varrefp->varp()->dtypeSkipRefp(), BasicDType)) {
            } अन्यथा यदि (ध्रुव AstUnpackArrayDType* ध्रुव adtypep
                       = VN_CAST(varrefp->varp()->dtypeSkipRefp(), UnpackArrayDType)) {
                array_lo = adtypep->lo();
                array_आकार = adtypep->elementsConst();
            } अन्यथा {
                nodep->v3error(nodep->verilogKwd()
                               << " loading other than unpacked-array variable");
            }
        }
        पङ्क्तिं_लिख(cvt_गन्तव्यम्_Str(array_lo));
        putbs(", ");
        पङ्क्तिं_लिख(cvt_गन्तव्यम्_Str(array_आकार));
        putbs(", ");
        पङ्क्तिं_लिख("&(");
        iterate_And_अग्र_Const_Null(nodep->memp());
        पङ्क्तिं_लिख(")");
        putbs(", ");
        iterate_And_अग्र_Const_Null(nodep->filep());
        putbs(", ");
        यदि (nodep->startp()) {
            iterate_And_अग्र_Const_Null(nodep->startp());
        } अन्यथा {
            पङ्क्तिं_लिख(cvt_गन्तव्यम्_Str(array_lo));
        }
        putbs(", ");
        यदि (nodep->countp()) {
            iterate_And_अग्र_Const_Null(nodep->countp());
        } अन्यथा {
            पङ्क्तिं_लिख(cvt_गन्तव्यम्_Str(array_आकार));
        }
        पङ्क्तिं_लिख(")");
    }
    शून्य दर्शय(Ast_राशिः_Trace_F* nodep) override { putns(nodep, "VL_STACKTRACE_N()"); }
    शून्य दर्शय(Ast_राशिः_Trace_T* nodep) override { putns(nodep, "VL_STACKTRACE();\n"); }
    शून्य दर्शय(AstSystemT* nodep) override {
        putns(nodep, "(void)VL_SYSTEM_I");
        emitIQW(nodep->lhsp());
        पङ्क्तिं_लिख("(");
        यदि (nodep->lhsp()->isWide()) {
            पङ्क्तिं_लिख(cvt_गन्तव्यम्_Str(nodep->lhsp()->विस्तारः_शब्दाः()));
            putbs(", ");
        }
        iterate_And_अग्र_Const_Null(nodep->lhsp());
        पङ्क्तिं_लिख(");\n");
    }
    शून्य दर्शय(AstSystemF* nodep) override {
        putns(nodep, "VL_SYSTEM_I");
        emitIQW(nodep->lhsp());
        पङ्क्तिं_लिख("(");
        यदि (nodep->lhsp()->isWide()) {
            पङ्क्तिं_लिख(cvt_गन्तव्यम्_Str(nodep->lhsp()->विस्तारः_शब्दाः()));
            putbs(", ");
        }
        iterate_And_अग्र_Const_Null(nodep->lhsp());
        पङ्क्तिं_लिख(")");
    }
    शून्य दर्शय(Ast_Stmt_व्यञ्जकम्* nodep) override {
        यदि (VN_IS(nodep->exprp()->dtypep()->skipRefp(), VoidDType)) {
            putns(nodep, "");
        } अन्यथा {
            putns(nodep, "std::ignore = ");
        }
        iterateConst(nodep->exprp());
        पङ्क्तिं_लिख(";\n");
    }
    शून्य दर्शय(Ast_Jump_खण्डम्* nodep) override {
        // Allocate label number
        ध्रुव आकार_t n = m_labelNumbers.आकार();
        ध्रुव bool new_प्रविष्टिः = m_labelNumbers.emplace(nodep, n).second;
        UASSERT_OBJ(new_प्रविष्टिः, nodep, "AstJumpBlock visited twide");
        // Emit
        putns(nodep, "{\n");  // Make it visually obvious label jumps outside these
        VL_RESTORER(m_createdScopeHash);
        iterate_And_अग्र_Const_Null(nodep->stmtsp());
        पङ्क्तिं_लिख("__Vlabel" + std::गन्तव्यम्_string(n) + ": ;\n");
        पङ्क्तिं_लिख("}\n");
    }
    शून्य दर्शय(AstJumpGo* nodep) override {
        // Retrieve target label number - must already exist (from enclosing AstJumpBlock)
        ध्रुव आकार_t n = m_labelNumbers.at(nodep->blockp());
        // Emit
        putns(nodep, "goto __Vlabel" + std::गन्तव्यम्_string(n) + ";\n");
    }
    शून्य दर्शय(AstLoop* nodep) override {
        UASSERT_OBJ(!nodep->contsp(), nodep, "'contsp' only used before LinkJump");
        VL_RESTORER(m_createdScopeHash);
        // Special case when the AstLoopTest is first for output readability
        यदि (AstLoopTest* ध्रुव testp = VN_CAST(nodep->stmtsp(), LoopTest)) {
            putns(nodep, "while (");
            iterateConst(testp->condp());
            पङ्क्तिं_लिख(") {\n");
            iterate_And_अग्र_Const_Null(testp->nextp());
            पङ्क्तिं_लिख("}\n");
            प्रत्यागम;
        }
        // Special case when the AstLoopTest is last for output readability
        यदि (Ast_ग्रन्थिः* lastp = nodep->stmtsp()) {
            यावत् (Ast_ग्रन्थिः* ध्रुव nextp = lastp->nextp()) lastp = nextp;
            यदि (AstLoopTest* ध्रुव testp = VN_CAST(lastp, LoopTest)) {
                putns(nodep, "do {\n");
                प्रति (Ast_ग्रन्थिः* p = nodep->stmtsp(); p != lastp; p = p->nextp()) iterateConst(p);
                पङ्क्तिं_लिख("} while (");
                iterateConst(testp->condp());
                पङ्क्तिं_लिख(");\n");
                प्रत्यागम;
            }
        }
        // Emit generic case directly
        putns(nodep, "while (true) {\n");
        iterate_And_अग्र_Const_Null(nodep->stmtsp());
        पङ्क्तिं_लिख("}\n");
    }
    शून्य दर्शय(AstLoopTest* nodep) override {
        VL_RESTORER(m_createdScopeHash);
        putns(nodep, "if (!(");
        iterate_And_अग्र_Const_Null(nodep->condp());
        पङ्क्तिं_लिख(")) break;\n");
    }
    शून्य दर्शय(AstCLocalScope* nodep) override {
        putns(nodep, "{\n");
        VL_RESTORER(m_createdScopeHash);
        iterate_And_अग्र_Const_Null(nodep->stmtsp());
        पङ्क्तिं_लिख("}\n");
    }
    शून्य दर्शय(Ast_ग्रन्थिः_If* nodep) override {
        putns(nodep, "if (");
        यदि (!nodep->branchPred().unknown()) {
            पङ्क्तिं_लिख(nodep->branchPred().ascii());
            पङ्क्तिं_लिख("((");  // Two parens, so that VL_UNLIKELY((class<foo,bar>)) works
        }
        iterate_And_अग्र_Const_Null(nodep->condp());
        यदि (!nodep->branchPred().unknown()) पङ्क्तिं_लिख("))");
        पङ्क्तिं_लिख(") {\n");
        {
            VL_RESTORER(m_createdScopeHash);
            iterate_And_अग्र_Const_Null(nodep->thensp());
        }
        पङ्क्तिं_लिख("}");
        यदि (!nodep->elsesp()) {
            पङ्क्तिं_लिख("\n");
        } अन्यथा {
            यदि (VN_IS(nodep->elsesp(), ग्रन्थिः_If) && !nodep->elsesp()->nextp()) {
                पङ्क्तिं_लिख(" else ");
                iterate_And_अग्र_Const_Null(nodep->elsesp());
            } अन्यथा {
                VL_RESTORER(m_createdScopeHash);
                पङ्क्तिं_लिख(" else {\n");
                iterate_And_अग्र_Const_Null(nodep->elsesp());
                पङ्क्तिं_लिख("}\n");
            }
        }
    }
    शून्य दर्शय(Ast_व्यञ्जकम्_Stmt* nodep) override {
        VL_RESTORER(m_createdScopeHash);
        ध्रुव bool अन्तर्भवति_Await = nodep->exists([](AstCAwait*) -> bool { प्रत्यागम true; });
        यदि (अन्तर्भवति_Await) {
            UASSERT_OBJ(m_cfuncp && m_cfuncp->isCoroutine(), nodep,
                        "AstExprStmt with CAwait must be in coroutine");
            putnbs(nodep, "(co_await ([&]() -> VlCoroutine {\n");
            iterate_And_अग्र_Const_Null(nodep->stmtsp());
            पङ्क्तिं_लिख("co_return;\n");
            यदि (!nodep->has_परिणाम()) {
                पङ्क्तिं_लिख("}()))");
                प्रत्यागम;
            }
            पङ्क्तिं_लिख("}()), ");
            iterate_And_अग्र_Const_Null(nodep->resultp());
            पङ्क्तिं_लिख(")");
            प्रत्यागम;
        }
        // GCC allows compound statements in expressions, but this is not standard.
        // So we use an immediate-evaluation lambda and comma operator
        putnbs(nodep, "([&]() {\n");
        यदि (!nodep->has_परिणाम()) {
            iterate_And_अग्र_Const_Null(nodep->stmtsp());
            पङ्क्तिं_लिख("}())");
            प्रत्यागम;
        }
        iterate_And_अग्र_Const_Null(nodep->stmtsp());
        पङ्क्तिं_लिख("}(), ");
        iterate_And_अग्र_Const_Null(nodep->resultp());
        पङ्क्तिं_लिख(")");
    }
    शून्य दर्शय(AstStop* nodep) override {
        putns(nodep, "VL_STOP_MT(");
        putsQuoted(protect(nodep->fileline()->filename()));
        पङ्क्तिं_लिख(", ");
        पङ्क्तिं_लिख(cvt_गन्तव्यम्_Str(nodep->fileline()->lineno()));
        पङ्क्तिं_लिख(", \"\"");
        यदि (nodep->isFatal()) पङ्क्तिं_लिख(", false");
        पङ्क्तिं_लिख(");\n");
    }
    शून्य दर्शय(AstFinish* nodep) override {
        // Disable all the forks so they don't operate after simulation is finished.
        यदि (m_cfuncp && m_cfuncp->needProcess()) putns(nodep, "vlProcess->disableFork();\n");
        putns(nodep, "VL_FINISH_MT(");
        putsQuoted(protect(nodep->fileline()->filename()));
        पङ्क्तिं_लिख(", ");
        पङ्क्तिं_लिख(cvt_गन्तव्यम्_Str(nodep->fileline()->lineno()));
        पङ्क्तिं_लिख(", \"\");\n");
    }
    शून्य दर्शय(AstFinishFork* nodep) override {
        // Disable all the forks so they don't operate after simulation is finished.
        यदि (m_cfuncp && m_cfuncp->needProcess()) putns(nodep, "vlProcess->disableFork();\n");
        putns(nodep, "VL_FINISH_MT(");
        putsQuoted(protect(nodep->fileline()->filename()));
        पङ्क्तिं_लिख(", ");
        पङ्क्तिं_लिख(cvt_गन्तव्यम्_Str(nodep->fileline()->lineno()));
        पङ्क्तिं_लिख(", \"\");\n");
        यदि (m_cfuncp->isCoroutine()) {
            putns(nodep, "co_return;\n");
        } अन्यथा {
            putns(nodep, "return;\n");
        }
    }
    शून्य दर्शय(AstPrintTimeScale* nodep) override {
        putns(nodep, "VL_PRINTTIMESCALE(");
        putsQuoted(protect(nodep->prettyName()));
        पङ्क्तिं_लिख(", ");
        putsQuoted(nodep->timeunit().ascii());
        पङ्क्तिं_लिख(", vlSymsp->_vm_contextp__);\n");
    }
    शून्य दर्शय(AstRand* nodep) override {
        emitOpName(nodep, nodep->emitC(), nodep->seedp(), nullptr, nullptr);
    }
    शून्य दर्शय(AstRandRNG* nodep) override {
        emitOpName(nodep, nodep->emitC(), nullptr, nullptr, nullptr);
    }
    शून्य दर्शय(AstTime* nodep) override {
        putns(nodep, "VL_TIME_UNITED_Q(");
        UASSERT_OBJ(!nodep->timeunit().isNone(), nodep, "$time has no units");
        ध्रुव द्विगुण time
            = nodep->timeunit().multiplier() / v3Global.rootp()->timeprecision().multiplier();
        UASSERT_OBJ(time >= 1, nodep, "TimeQ is less than 1, will result in division by zero");
        पङ्क्तिं_लिख(cvt_गन्तव्यम्_Str(time));
        पङ्क्तिं_लिख(")");
    }
    शून्य दर्शय(AstTimeD* nodep) override {
        putns(nodep, "VL_TIME_UNITED_D(");
        UASSERT_OBJ(!nodep->timeunit().isNone(), nodep, "$realtime has no units");
        पङ्क्तिं_लिख(cvt_गन्तव्यम्_Str(nodep->timeunit().multiplier()
                      / v3Global.rootp()->timeprecision().multiplier()));
        पङ्क्तिं_लिख(")");
    }
    शून्य दर्शय(Ast_Get_Initial_Random_बीजम्* nodep) override {
        putns(nodep, "vlSymsp->_vm_contextp__->randSeed()");
    }
    शून्य दर्शय(AstTimeFormat* nodep) override {
        putns(nodep, "VL_TIMEFORMAT_IINI(");
        यदि (nodep->unitsp()) {
            पङ्क्तिं_लिख("true, ");
            iterate_And_अग्र_Const_Null(nodep->unitsp());
        } अन्यथा {
            पङ्क्तिं_लिख("false, 0");
        }
        पङ्क्तिं_लिख(", ");
        यदि (nodep->precisionp()) {
            पङ्क्तिं_लिख("true, ");
            iterate_And_अग्र_Const_Null(nodep->precisionp());
        } अन्यथा {
            पङ्क्तिं_लिख("false, 0");
        }
        पङ्क्तिं_लिख(", ");
        यदि (nodep->suffixp()) {
            पङ्क्तिं_लिख("true, ");
            iterate_And_अग्र_Const_Null(nodep->suffixp());
        } अन्यथा {
            पङ्क्तिं_लिख("false, \"\"");
        }
        पङ्क्तिं_लिख(", ");
        यदि (nodep->widthp()) {
            पङ्क्तिं_लिख("true, ");
            iterate_And_अग्र_Const_Null(nodep->widthp());
        } अन्यथा {
            पङ्क्तिं_लिख("false, 0");
        }
        पङ्क्तिं_लिख(", vlSymsp->_vm_contextp__);\n");
    }
    शून्य दर्शय(AstTimePrecision* nodep) override {
        putns(nodep, "vlSymsp->_vm_contextp__->timeprecision()");
    }

    // Nodes involing AstText
    शून्य दर्शय(AstText* nodep) override {
        // All Text should be under TextBlock/CStmt/CStmtUser/CExpr/CExprUser
        nodep->v3fatalSrc("Text node in unexpected position");
    }
    शून्य दर्शय(Ast_Text_खण्डम्* nodep) override {
        putnbs(nodep, "");
        पङ्क्तिं_लिख(nodep->उपसर्गः());
        emitNodesWithText(nodep->nodesp(), false, true, nodep->separator());
        पङ्क्तिं_लिख(nodep->suffix());
    }
    शून्य दर्शय(AstCStmt* nodep) override {
        putnbs(nodep, "");
        emitNodesWithText(nodep->nodesp(), false, true, "");
        ensure_New_पङ्क्तिः();
    }
    शून्य दर्शय(AstCExpr* nodep) override {
        putnbs(nodep, "");
        emitNodesWithText(nodep->nodesp(), false, true, "");
    }
    शून्य दर्शय(AstCStmtUser* nodep) override {
        putnbs(nodep, "");
        ofp()->putsNoTracking("\n");
        यदि (nodep->fromDollarC() && v3Global.opt.decoration() && !v3Global.opt.protectIds()) {
            ofp()->putsNoTracking("// $c statement at " + nodep->fileline()->ascii() + "\n");
        }
        emitNodesWithText(nodep->nodesp(), m_useSelfForThis, false, "");
        पङ्क्तिं_लिख("\n");
    }
    शून्य दर्शय(AstCExprUser* nodep) override {
        putnbs(nodep, "");
        ofp()->putsNoTracking("\n");
        यदि (/* is always from $c */ v3Global.opt.decoration() && !v3Global.opt.protectIds()) {
            ofp()->putsNoTracking(
                (nodep->is_विशुद्धम्() ? "// $cpure expression at " : "// $c expression at ")
                + nodep->fileline()->ascii() + "\n");
        }
        emitNodesWithText(nodep->nodesp(), m_useSelfForThis, false, "");
        पङ्क्तिं_लिख("\n");
    }

    // Operators
    शून्य दर्शय(Ast_ग्रन्थिः_Termop* nodep) override {
        emitOpName(nodep, nodep->emitC(), nullptr, nullptr, nullptr);
    }
    शून्य दर्शय(Ast_ग्रन्थिः_Uniop* nodep) override {
        यदि (nodep->emit_Check_Max_शब्दाः()
            && (nodep->विस्तारः_शब्दाः() > VL_MULS_MAX_शब्दाः
                || nodep->lhsp()->विस्तारः_शब्दाः() > VL_MULS_MAX_शब्दाः)) {
            nodep->v3warn(
                E_UNSUPPORTED,
                "Unsupported: "
                    << nodep->prettyOperatorName() << " operator of " << nodep->विस्तारः()
                    << " bits exceeds hardcoded limit VL_MULS_MAX_WORDS in verilatedos.h");
        }
        यदि (emitSimpleOk(nodep)) {
            putnbs(nodep, "(");
            पङ्क्तिं_लिख(nodep->emitSimpleOperator());
            पङ्क्तिं_लिख(" ");
            iterate_And_अग्र_Const_Null(nodep->lhsp());
            पङ्क्तिं_लिख(")");
        } अन्यथा {
            emitOpName(nodep, nodep->emitC(), nodep->lhsp(), nullptr, nullptr);
        }
    }
    शून्य दर्शय(Ast_ग्रन्थिः_Biop* nodep) override {
        यदि (nodep->emit_Check_Max_शब्दाः() && nodep->विस्तारः_शब्दाः() > VL_MULS_MAX_शब्दाः) {
            nodep->v3warn(
                E_UNSUPPORTED,
                "Unsupported: "
                    << nodep->prettyOperatorName() << " operator of " << nodep->विस्तारः()
                    << " bits exceeds hardcoded limit VL_MULS_MAX_WORDS in verilatedos.h");
        }
        यदि (emitSimpleOk(nodep)) {
            putnbs(nodep, "(");
            iterate_And_अग्र_Const_Null(nodep->lhsp());
            पङ्क्तिं_लिख(" ");
            putbs(nodep->emitSimpleOperator());
            पङ्क्तिं_लिख(" ");
            iterate_And_अग्र_Const_Null(nodep->rhsp());
            पङ्क्तिं_लिख(")");
        } अन्यथा {
            emitOpName(nodep, nodep->emitC(), nodep->lhsp(), nodep->rhsp(), nullptr);
        }
    }
    शून्य दर्शय(Ast_ग्रन्थिः_Triop* nodep) override {
        UASSERT_OBJ(!emitSimpleOk(nodep), nodep, "Triop cannot be described in a simple way");
        emitOpName(nodep, nodep->emitC(), nodep->lhsp(), nodep->rhsp(), nodep->thsp());
    }
    शून्य दर्शय(AstCvtPackString* nodep) override { emitCvtPackStr(nodep->lhsp()); }
    शून्य दर्शय(AstRedXor* nodep) override {
        यदि (nodep->lhsp()->isWide()) {
            दर्शय(static_cast<Ast_ग्रन्थिः_Uniop*>(nodep));
        } अन्यथा {
            ध्रुव AstVarRef* ध्रुव vrefp = VN_CAST(nodep->lhsp(), VarRef);
            ध्रुव पूर्णाङ्क विस्तारः_Pow2 = vrefp ? vrefp->varp()->dtypep()->विस्तारः_Pow2()
                                        : nodep->lhsp()->dtypep()->विस्तारः_Pow2();
            UASSERT_OBJ(विस्तारः_Pow2 > 1, nodep,
                        "Reduction over single bit value should have been folded");
            putnbs(nodep, "VL_REDXOR_");
            पङ्क्तिं_लिख(cvt_गन्तव्यम्_Str(विस्तारः_Pow2));
            पङ्क्तिं_लिख("(");
            iterate_And_अग्र_Const_Null(nodep->lhsp());
            पङ्क्तिं_लिख(")");
        }
    }
    शून्य दर्शय(AstCCast* nodep) override {
        // Extending a value of the same word width is just a NOP.
        यदि (ध्रुव AstClassRefDType* ध्रुव classDtypep
            = VN_CAST(nodep->dtypep()->skipRefp(), ClassRefDType)) {
            putns(nodep, "(" + classDtypep->cType("", false, false) + ")(");
        } अन्यथा यदि (nodep->आकार() <= VL_BYTESIZE) {
            putns(nodep, "(::CData)(");
        } अन्यथा यदि (nodep->आकार() <= VL_SHORTSIZE) {
            putns(nodep, "(::SData)(");
        } अन्यथा यदि (nodep->आकार() <= VL_IDATASIZE) {
            putns(nodep, "(::IData)(");
        } अन्यथा {
            putns(nodep, "(::QData)(");
        }
        iterate_And_अग्र_Const_Null(nodep->lhsp());
        पङ्क्तिं_लिख(")");
    }
    शून्य दर्शय(AstCond* nodep) override {
        // Widths match up already, so we'll just use C++'s operator w/o any temps.
        यदि (nodep->thenp()->isWide()) {
            emitOpName(nodep, nodep->emitC(), nodep->condp(), nodep->thenp(), nodep->elsep());
        } अन्यथा {
            putnbs(nodep, "(");
            iterate_And_अग्र_Const_Null(nodep->condp());
            putbs(" ? ");
            iterate_And_अग्र_Const_Null(nodep->thenp());
            putbs(" : ");
            iterate_And_अग्र_Const_Null(nodep->elsep());
            पङ्क्तिं_लिख(")");
        }
    }
    शून्य दर्शय(Ast_मेलनम्_Masked* nodep) override {
        emitOpName(nodep, nodep->emitC(), nodep->lhsp(), nodep->matchp(), nullptr);
    }
    शून्य दर्शय(AstMemberSel* nodep) override {
        ध्रुव AstVar* ध्रुव varp = nodep->varp();
        ध्रुव bool dereferenceCovergroupRef
            = varp->covergroupRefMember() && nodep->access().isReadOrRW();
        यदि (dereferenceCovergroupRef) putnbs(nodep, "(*");
        iterate_And_अग्र_Const_Null(nodep->fromp());
        putnbs(nodep, "->");
        यदि (varp->isIfaceRef()) {
            // varp is the __Viftop companion (e.g. "tx__Viftop"); use the
            // MemberSel name which matches the cell's C++ member (e.g. "tx").
            पङ्क्तिं_लिख(nodep->nameProtect());
        } अन्यथा {
            पङ्क्तिं_लिख(varName(varp));
        }
        यदि (dereferenceCovergroupRef) पङ्क्तिं_लिख(")");
    }
    शून्य दर्शय(AstStructSel* nodep) override {
        iterate_And_अग्र_Const_Null(nodep->fromp());
        putnbs(nodep, ".");
        पङ्क्तिं_लिख(nodep->nameProtect());
    }
    शून्य दर्शय(AstNullCheck* nodep) override {
        putns(nodep, "VL_NULL_CHECK(");
        iterate_And_अग्र_Const_Null(nodep->lhsp());
        पङ्क्तिं_लिख(", ");
        putsQuoted(protect(nodep->fileline()->filename()));
        पङ्क्तिं_लिख(", ");
        पङ्क्तिं_लिख(cvt_गन्तव्यम्_Str(nodep->fileline()->lineno()));
        पङ्क्तिं_लिख(")");
    }
    शून्य दर्शय(AstNewCopy* nodep) override {
        // Polymorphic shallow clone: preserves runtime type via virtual clone()
        // VL_NULL_CHECK enforces null check per IEEE 1800-2023 8.7
        putns(nodep, "VL_NULL_CHECK(");
        यदि (VN_IS(nodep->rhsp(), Const) && VN_AS(nodep->rhsp(), Const)->isNull()) {
            // V3Const folded rhs to null: emit a typed empty ref so VL_NULL_CHECK fires
            ध्रुव AstClassRefDType* ध्रुव refDTypep
                = VN_CAST(nodep->dtypep()->skipRefp(), ClassRefDType);
            पङ्क्तिं_लिख(refDTypep->cType("", false, false) + "{}");
        } अन्यथा {
            iterate_And_अग्र_Const_Null(nodep->rhsp());
        }
        पङ्क्तिं_लिख(", ");
        putsQuoted(protect(nodep->fileline()->filename()));
        पङ्क्तिं_लिख(", ");
        पङ्क्तिं_लिख(cvt_गन्तव्यम्_Str(nodep->fileline()->lineno()));
        पङ्क्तिं_लिख(").clone(vlSymsp->__Vm_deleter)");
    }
    शून्य दर्शय(AstSel* nodep) override {
        // Note ASSIGN checks for this on a LHS
        UASSERT_OBJ(nodep->विस्तारः_Min() == nodep->विस्तारः_Const(), nodep, "Width mismatch");
        emitOpName(nodep, nodep->emitC(), nodep->fromp(), nodep->lsbp(), nullptr);
    }
    शून्य दर्शय(AstReplicate* nodep) override {
        यदि (nodep->srcp()->विस्तारः_Min() == 1 && !nodep->isWide()) {
            UASSERT_OBJ((static_cast<पूर्णाङ्क>(VN_AS(nodep->countp(), Const)->गन्तव्यम्_UInt())
                         * nodep->srcp()->विस्तारः_Min())
                            == nodep->विस्तारः_Min(),
                        nodep, "Replicate non-constant or width miscomputed");
            putns(nodep, "VL_REPLICATE_");
            emitIQW(nodep);
            पङ्क्तिं_लिख("OI(");
            यदि (nodep->srcp()) पङ्क्तिं_लिख(cvt_गन्तव्यम्_Str(nodep->srcp()->विस्तारः_Min()));
            पङ्क्तिं_लिख(", ");
            iterate_And_अग्र_Const_Null(nodep->srcp());
            पङ्क्तिं_लिख(", ");
            iterate_And_अग्र_Const_Null(nodep->countp());
            पङ्क्तिं_लिख(")");
        } अन्यथा {
            emitOpName(nodep, nodep->emitC(), nodep->srcp(), nodep->countp(), nullptr);
        }
    }
    शून्य emit_प्रवाहः_R(Ast_प्रवाहः_R* nodep, Ast_ग्रन्थिः* parent) {
        //TODO: This might need to handle more cases like the visit(AstStreamR) function
        emitOpName(nodep, nodep->emitC(), nodep->lhsp(), nodep->rhsp(), nullptr);
    }
    शून्य दर्शय(Ast_प्रवाहः_R* nodep) override {
        //The parrent node of our AstStreamR will give just enough info for what streamR should
        //output if nodep->backp() is not the parent then emitStreamR should have been used. throw
        //an error
        bool backpIsParent = (nodep->backp()->op1p() == nodep || nodep->backp()->op2p() == nodep);
        UASSERT(backpIsParent, "can not find return type for streamR");
        यदि (ध्रुव AstQueueDType* ध्रुव qdtp
            = VN_CAST(nodep->backp()->dtypep()->skipRefp(), QueueDType)) {
            यदि (nodep->lhsp()->isWide()) {
                emitOpName(nodep,
                           "VL_STREAMR_%nq%lq%rq(%lw," + std::गन्तव्यम्_string(qdtp->subDTypep()->विस्तारः())
                               + ", %P, %li, %ri)",
                           nodep->lhsp(), nodep->rhsp(), nullptr);
            } अन्यथा {
                emitOpName(nodep, "VL_STREAMR_%nq%lq%rq(%lw, %P, %li, %ri)", nodep->lhsp(),
                           nodep->rhsp(), nullptr);
            }
        } अन्यथा यदि (VN_IS(nodep->lhsp()->dtypep()->skipRefp(), QueueDType)) {
            यदि (!((nodep->backp()->op1p() && nodep->backp()->op1p()->isWide())
                  || (nodep->backp()->op2p() && nodep->backp()->op2p()->isWide()))) {
                //If our lhsp is a queue make sure we streamR and return the correct type.
                //If either side is wide or the previous node is string type dont use this case
                emitOpName(nodep->backp(), "VL_STREAMR_%nq%lq%rq(%lw, %P, %li, %ri)",
                           nodep->lhsp(), nodep->rhsp(), nullptr);
            } अन्यथा {
                emitOpName(nodep, nodep->emitC(), nodep->lhsp(), nodep->rhsp(), nullptr);
            }
        } अन्यथा {
            emitOpName(nodep, nodep->emitC(), nodep->lhsp(), nodep->rhsp(), nullptr);
        }
    }
    शून्य दर्शय(Ast_प्रवाहः_L* nodep) override {
        // Attempt to use a "fast" stream function for slice size = power of 2
        यदि (!nodep->isWide()) {
            ध्रुव uint32_t isPow2 = VN_AS(nodep->rhsp(), Const)->अङ्कः().गणनम्_Ones() == 1;
            ध्रुव uint32_t खण्डिका_आकार = VN_AS(nodep->rhsp(), Const)->गन्तव्यम्_UInt();
            यदि (isPow2 && खण्डिका_आकार <= (nodep->isQuad() ? परिमाण(uint64_t) : परिमाण(uint32_t))) {
                putns(nodep, "VL_STREAML_FAST_");
                bool उपयुङ्क्ते_Queue = false;
                AstQueueDType* qtypep
                    = nodep->backp()->op2p()
                          ? VN_CAST(nodep->backp()->op2p()->dtypep()->skipRefp(), QueueDType)
                          : nullptr;
                यदि (VN_IS(nodep->backp(), नियोजय)
                    && qtypep) {  // If we are assigning to a queue then emit the correct symbol
                    पङ्क्तिं_लिख("R");  // R for queue
                    उपयुङ्क्ते_Queue = true;
                } अन्यथा {
                    emitIQW(nodep);
                }
                emitIQW(nodep->lhsp());
                पङ्क्तिं_लिख("I(");
                पङ्क्तिं_लिख(cvt_गन्तव्यम्_Str(nodep->lhsp()->विस्तारः_Min()));
                पङ्क्तिं_लिख(", ");
                यदि (उपयुङ्क्ते_Queue) {
                    iterate_And_अग्र_Const_Null(nodep->backp()->op2p());
                    पङ्क्तिं_लिख(", ");
                }
                iterate_And_अग्र_Const_Null(nodep->lhsp());
                पङ्क्तिं_लिख(", ");
                ध्रुव uint32_t rd_log2 = V3_सङ्ख्या::log2b(VN_AS(nodep->rhsp(), Const)->गन्तव्यम्_UInt());
                पङ्क्तिं_लिख(cvt_गन्तव्यम्_Str(rd_log2) + ")");
                प्रत्यागम;
            }
        }
        यदि (VN_IS(nodep->backp(), नियोजय)
            && VN_IS(nodep->backp()->op2p()->dtypep()->skipRefp(), QueueDType)) {
            पूर्णाङ्क queue_विस्तारः
                = nodep->backp()->op2p()->dtypep()->subDTypep()->विस्तारः();  //We need to know the
                                                                           //width of both sides
            emitOpName(nodep,
                       "VL_STREAML_%nq%lq%rq(%lw," + std::गन्तव्यम्_string(queue_विस्तारः)
                           + ", %P, %li, %ri)",
                       nodep->lhsp(), nodep->rhsp(), nullptr);
        } अन्यथा {
            emitOpName(nodep, nodep->emitC(), nodep->lhsp(), nodep->rhsp(), nullptr);
        }
    }
    शून्य दर्शय(AstCastDynamic* nodep) override {
        putnbs(nodep, "VL_CAST_DYNAMIC(");
        iterate_And_अग्र_Const_Null(nodep->fromp());
        पङ्क्तिं_लिख(", ");
        iterate_And_अग्र_Const_Null(nodep->शिखरम्());
        पङ्क्तिं_लिख(")");
    }
    शून्य दर्शय(Ast_गणनम्_द्वयङ्काः* nodep) override {
        putnbs(nodep, "VL_COUNTBITS_");
        emitIQW(nodep->lhsp());
        पङ्क्तिं_लिख("(");
        पङ्क्तिं_लिख(cvt_गन्तव्यम्_Str(nodep->lhsp()->विस्तारः_Min()));
        पङ्क्तिं_लिख(", ");
        यदि (nodep->lhsp()->isWide()) {
            पङ्क्तिं_लिख(cvt_गन्तव्यम्_Str(nodep->lhsp()->विस्तारः_शब्दाः()));  // Note argument width, not node width
                                                          // (which is always 32)
            पङ्क्तिं_लिख(", ");
        }
        iterate_And_अग्र_Const_Null(nodep->lhsp());
        पङ्क्तिं_लिख(", ");
        iterate_And_अग्र_Const_Null(nodep->rhsp());
        पङ्क्तिं_लिख(", ");
        iterate_And_अग्र_Const_Null(nodep->thsp());
        पङ्क्तिं_लिख(", ");
        iterate_And_अग्र_Const_Null(nodep->fhsp());
        पङ्क्तिं_लिख(")");
    }
    शून्य दर्शय(AstTriggered* nodep) override {
        // Test each word of the trigger vector with a mask of the relevant bits
        std::map<uint32_t, uint64_t> wordMasks;
        प्रति (ध्रुव uint32_t सूचकांक : nodep->indices()) {
            wordMasks[VL_BITWORD_Q(सूचकांक)] |= VL_BIT_Q(सूचकांक);
        }
        putns(nodep, "((");
        bool first = true;
        प्रति (ध्रुव स्वचालित& word_आवरणम् : wordMasks) {
            यदि (!first) पङ्क्तिं_लिख("\n| ");
            first = false;
            ध्रुव bool allOnes = word_आवरणम्.second == ~0ULL;
            यदि (!allOnes) पङ्क्तिं_लिख("(" + V3_सङ्ख्या{nodep, 64}.समुच्चयः_Quad(word_आवरणम्.second).emitC() + " & ");
            iterateConst(nodep->trigRefp());
            पङ्क्तिं_लिख("[" + std::गन्तव्यम्_string(word_आवरणम्.first) + "U]");
            यदि (!allOnes) पङ्क्तिं_लिख(")");
        }
        पङ्क्तिं_लिख(") != 0ULL)");
    }
    // Terminals
    शून्य दर्शय(AstVarRef* nodep) override {
        ध्रुव AstVar* ध्रुव varp = nodep->varp();

        // Reference to constant pool variable is special
        यदि (varp->const_Pool_प्रविष्टिः()) {
            putns(nodep, EmitCUtil::constPoolName(varp));
            प्रत्यागम;
        }

        ध्रुव Ast_ग्रन्थिः_Module* ध्रुव varModp = EmitCParentModule::get(varp);
        ध्रुव bool dereferenceCovergroupRef
            = varp->covergroupRefMember() && nodep->access().isReadOrRW();
        यदि (dereferenceCovergroupRef) putns(nodep, "(*");
        यदि (VN_IS(varModp, Class) && varModp != m_modp) {
            // Superclass member reference
            putns(nodep, EmitCUtil::उपसर्गः_Name_Protect(varModp) + "::");
        } अन्यथा यदि (varp->isIfaceRef()) {
            putns(nodep, nodep->selfPointerProtect(m_useSelfForThis));
            प्रत्यागम;
        } अन्यथा यदि (!nodep->selfPointer().is_रिक्तानि()) {
            emitDereference(nodep, nodep->selfPointerProtect(m_useSelfForThis));
        }
        putns(nodep, varName(varp));
        यदि (dereferenceCovergroupRef) पङ्क्तिं_लिख(")");
    }
    शून्य दर्शय(Ast_Addr_सम्बन्धि_CFunc* nodep) override {
        // Note: Can be thought to handle more, but this is all that is needed right now
        ध्रुव AstCFunc* ध्रुव funcp = nodep->funcp();
        UASSERT_OBJ(funcp->isLoose(), nodep, "Cannot take address of non-loose method");
        putns(nodep, "&");
        पङ्क्तिं_लिख(funcNameProtect(funcp));
    }
    शून्य दर्शय(AstConst* nodep) override {  //
        यदि (m_wide_अस्थायी_Refp && nodep->isWide()) {
            UASSERT_OBJ(m_wide_अस्थायी_Refp, nodep, "Wide Constant w/ no temp");
            emitConstantW(nodep, m_wide_अस्थायी_Refp);
            m_wide_अस्थायी_Refp = nullptr;  // We used it, fail if set it a second time
        } अन्यथा {
            emitConstant(nodep);
        }
    }
    शून्य दर्शय(Ast_ग्रन्थिः_Cons* nodep) override { emitValCons(nodep); }
    शून्य दर्शय(AstThisRef* nodep) override {
        putnbs(nodep, nodep->dtypep()->cType("", false, false));
        पङ्क्तिं_लिख("{");
        पङ्क्तिं_लिख(VSelfPointerText::replaceThis(m_useSelfForThis, "this"));
        पङ्क्तिं_लिख("}");
    }
    शून्य दर्शय(Ast_ग्रन्थिः_Sel* nodep) override {
        यदि (!VN_IS(nodep, ArraySel) && !VN_IS(nodep, WordSel)) {
            दर्शय(static_cast<Ast_ग्रन्थिः_Biop*>(nodep));
            प्रत्यागम;
        }
        // ArraySel or WordSel
        iterate_And_अग्र_Const_Null(nodep->fromp());
        // Special case constant index for readability
        यदि (AstConst* ध्रुव idxp = VN_CAST(nodep->bitp(), Const)) {
            पङ्क्तिं_लिख("[" + std::गन्तव्यम्_string(idxp->गन्तव्यम्_UInt()) + "U]");
            प्रत्यागम;
        }
        putbs("[");
        iterate_And_अग्र_Const_Null(nodep->bitp());
        पङ्क्तिं_लिख("]");
    }

    //
    शून्य दर्शय(AstExecGraph* nodep) override {
        // The location of the AstExecGraph within the containing AstCFunc is where we want to
        // invoke the graph and wait for it to complete. The run-time library does just that.
        putns(nodep, "vl_invokeExecGraph(" + EmitCUtil::execGraphName(nodep) + ", vlSelf, ");
        iterateConst(nodep->triggerp());
        पङ्क्तिं_लिख(".data());\n");
    }

    // Default
    शून्य दर्शय(Ast_ग्रन्थिः* nodep) override {  // LCOV_EXCL_START
        putns(nodep, "\n???? // "s + nodep->prettyTypeName() + "\n");
        iterateChildrenConst(nodep);
        यदि (!v3Global.opt.lintOnly()) {  // An internal problem, so suppress
            nodep->v3fatalSrc("Unknown node type reached emitter: " << nodep->prettyTypeName());
        }
    }  // LCOV_EXCL_STOP

रक्षित:
    EmitCFunc() = स्वभाव;
    ~EmitCFunc() override = स्वभाव;
};

#यदि_अन्तः  // guard
