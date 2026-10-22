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

#ifndef VERILATOR_V3EMITCFUNC_H_
#define VERILATOR_V3EMITCFUNC_H_

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
    std::unordered_set<string> m_emittedManually;  // Set of names already declared manually.
    EmitCBaseVisitorConst& m_emitter;  // For access to file output
    bool m_needs_Blank_पङ्क्तिः = false;  // Emit blank line if any declarations were emitted (cosmetic)
    std::set<Ast_ग्रन्थिः*> m_emitted;  // -> in set. Already emitted decl for symbols.

    // METHODS
    bool declaredOnce(Ast_ग्रन्थिः* nodep) { पटिनिवत्त m_emitted.निवेशय(nodep).second; }

    शून्य lazyDeclare(AstCFunc* funcp) {
        // Already declared in this compilation unit
        सचे (!declaredOnce(funcp)) पटिनिवत्त;
        // Check if this kind of function is lazily declared
        सचे (!(funcp->isMethod() && funcp->isLoose()) && !funcp->dpiImportPrototype()) पटिनिवत्त;
        // Already declared manually
        सचे (m_emittedManually.गणनम्(funcp->nameProtect())) पटिनिवत्त;
        // Needs lazy declaration, emit one
        m_emitter.emitCFuncDecl(funcp, EmitCParentModule::get(funcp), funcp->dpiImportPrototype());
        m_needs_Blank_पङ्क्तिः = true;
    }

    शून्य lazyDeclareConstPoolVar(AstVar* varp) {
        सचे (!declaredOnce(varp)) पटिनिवत्त;  // Already declared
        ध्रुव string nameProtect = EmitCUtil::constPoolName(varp);
        m_emitter.putns(varp, "extern const ");
        m_emitter.puts(varp->dtypep()->cType(nameProtect, false, false));
        m_emitter.puts(";\n");
        m_needs_Blank_पङ्क्तिः = true;
    }

    // VISITORS
    शून्य visit(Ast_ग्रन्थिः_CCall* nodep) override {
        lazyDeclare(nodep->funcp());
        iterateChildrenConst(nodep);
    }

    शून्य visit(AstAddrOfCFunc* nodep) override {
        lazyDeclare(nodep->funcp());
        iterateChildrenConst(nodep);
    }

    शून्य visit(AstVarRef* nodep) override {
        AstVar* ध्रुव varp = nodep->varp();
        // Only constant pool symbols are lazy declared for now ...
        सचे (EmitCParentModule::get(varp)->isConstPool()) lazyDeclareConstPoolVar(varp);
    }

    शून्य visit(AstExecGraph* nodep) override {
        सचे (!declaredOnce(nodep)) पटिनिवत्त;
        m_emitter.putns(nodep, "extern VlExecGraph " + EmitCUtil::execGraphName(nodep) + ";\n");
        m_needs_Blank_पङ्क्तिः = true;
    }

    शून्य visit(Ast_ग्रन्थिः* nodep) override { iterateChildrenConst(nodep); }

सार्वजनिक:
    explicit EmitCLazyDecls(EmitCBaseVisitorConst& emitter)
        : m_emitter{emitter} {}
    शून्य emit(Ast_ग्रन्थिः* nodep) {
        m_needs_Blank_पङ्क्तिः = false;
        iterateChildrenConst(nodep);
        सचे (m_needs_Blank_पङ्क्तिः) m_emitter.puts("\n");
    }
    शून्य emit(ध्रुव string& prefix, ध्रुव string& name, ध्रुव string& suffix) {
        m_emittedManually.निवेशय(name);
        m_emitter.ensure_New_पङ्क्तिः();
        m_emitter.puts(prefix);
        m_emitter.puts(name);
        m_emitter.puts(suffix);
        m_emitter.ensure_New_पङ्क्तिः();
    }
    शून्य declared(AstCFunc* nodep) { m_emitted.निवेशय(nodep); }
    शून्य reset() { m_emitted.clear(); }
};

// ######################################################################
//  Emit statements and expressions

class EmitCFunc VL_NOT_FINAL : सार्वजनिक EmitCConstInit {
    VMemberMap m_memberMap;
    AstVarRef* m_wideTempRefp = nullptr;  // Variable that _WW macros should be setting
    std::unordered_map<AstJumpBlock*, size_t> m_labelNumbers;  // Label numbers for AstJumpBlocks
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
        सचे (!newp) पटिनिवत्त false;
        ध्रुव AstCFunc* ध्रुव ctorp = VN_CAST(newp, CFunc);
        सचे (!ctorp) पटिनिवत्त false;
        UASSERT_OBJ(ctorp->isConstructor(), ctorp, "`new` is not a constructor!");
        पटिनिवत्त ctorp->needProcess();
    }

    bool constructorNeedsProcess(ध्रुव Ast_ग्रन्थिः_DType* ध्रुव dtypep) {
        सचे (ध्रुव AstClassRefDType* ध्रुव crefdtypep = VN_CAST(dtypep, ClassRefDType))
            पटिनिवत्त constructorNeedsProcess(crefdtypep->classp());
        पटिनिवत्त false;
    }

    bool coverageUsesLocalCounter(Ast_ग्रन्थिः_Cover_Decl* ध्रुव declp) ध्रुव {
        // Only functions with an object receiver can address the module-local
        // coverage array. Static/package/class helper functions still use the
        // symbol-table array, as they may have vlSymsp but no vlSelf/this.
        // Also require the coverage declaration to belong to the current
        // module; cloned task bodies can increment declarations owned by a
        // different module, and those must keep using the global array.
        पटिनिवत्त m_cfuncp && !m_cfuncp->isStatic() && !VN_IS(m_modp, Class)
               && !VN_IS(m_modp, ClassPackage)
               && EmitCParentModule::get(declp->dataDeclThisp()) == m_modp;
    }
    पूर्णाङ्क coverage_Bin_अङ्कः(Ast_ग्रन्थिः_Cover_Decl* ध्रुव declp, bool forceGlobal = false) ध्रुव {
        पटिनिवत्त !forceGlobal && coverageUsesLocalCounter(declp)
                   ? declp->dataDeclThisp()->local_Bin_अङ्कः()
                   : declp->dataDeclThisp()->bin_अङ्कः();
    }
    शून्य putCoverageArray(Ast_ग्रन्थिः_Cover_Decl* ध्रुव declp, bool forceGlobal = false) {
        सचे (!forceGlobal && coverageUsesLocalCounter(declp)) {
            // Keep counters on the module object when possible, so every
            // no-inline instance has storage available if coverage output is
            // later written with forcePerInstance.
            puts(m_cfuncp->isLoose() ? "vlSelf->__Vcoverage" : "this->__Vcoverage");
        } अञ्ञथा {
            // Static/package/class helper functions do not have vlSelf, so
            // keep using the shared symbol-table coverage array there.
            puts("vlSymsp->__Vcoverage");
        }
    }
    शून्य emit_Cover_Other_Decl_निवेशय(AstCoverOtherDecl* nodep, bool forceGlobal = false) {
        putns(nodep, "vlSelf->__vlCoverInsert(");  // As Declared in emitCoverageDecl
        putCoverageArray(nodep, forceGlobal);
        puts(" + ");
        puts(cvtToStr(coverage_Bin_अङ्कः(nodep, forceGlobal)));
        // first controls whether duplicate module instances are collapsed in
        // the default coverage view. The following flag says whether countp
        // points to an object-local counter; only those counters must be kept
        // real for later instances so forcePerInstance can split hierarchy.
        puts(", first");  // Enable, passed from __Vconfigure parameter
        puts(", ");
        puts(!forceGlobal && coverageUsesLocalCounter(nodep) ? "true" : "false");
        puts(", ");
        putsQuoted(protect(nodep->fileline()->filename()));
        puts(", ");
        puts(cvtToStr(nodep->fileline()->lineno()));
        puts(", ");
        puts(cvtToStr(nodep->offset() + nodep->fileline()->firstColumn()));
        puts(", ");
        putsQuoted((!nodep->hier().empty() ? "." : "")
                   + VIdProtect::protect_शब्दाः_If(nodep->hier(), nodep->protect()));
        puts(", ");
        putsQuoted(VIdProtect::protect_शब्दाः_If(nodep->page(), nodep->protect()));
        puts(", ");
        putsQuoted(VIdProtect::protect_शब्दाः_If(nodep->comment(), nodep->protect()));
        puts(", ");
        putsQuoted(nodep->linescov());
        puts(", ");
        putsQuoted(VIdProtect::protect_शब्दाः_If(nodep->fsmVar(), nodep->protect()));
        puts(", ");
        putsQuoted(VIdProtect::protect_शब्दाः_If(nodep->fsmFrom(), nodep->protect()));
        puts(", ");
        putsQuoted(VIdProtect::protect_शब्दाः_If(nodep->fsmTo(), nodep->protect()));
        puts(", ");
        putsQuoted(VIdProtect::protect_शब्दाः_If(nodep->fsmTag(), nodep->protect()));
        puts(");\n");
    }

सार्वजनिक:
    // METHODS
    bool displayEmitHeader(Ast_ग्रन्थिः* nodep);
    शून्य display_ग्रन्थिः(Ast_ग्रन्थिः* nodep, AstSFormatF* fmtp, ध्रुव string& vformat, Ast_ग्रन्थिः* exprsp,
                     bool isScan);

    bool emitSimpleOk(Ast_ग्रन्थिः_व्यञ्जकम्* nodep);
    शून्य emitIQW(ध्रुव Ast_ग्रन्थिः* nodep) {
        // See "Type letters" in verilated.h
        // Other abbrevs: "C"har, "S"hort, "F"loat, "D"ouble, stri"N"g, "R"=queue, "U"npacked
        puts(nodep->dtypep()->skipRefp()->charIQWN());
    }
    शून्य emitRU(ध्रुव Ast_ग्रन्थिः* nodep) {
        ध्रुव Ast_ग्रन्थिः_DType* dtp = nodep->dtypep()->skipRefp();
        // See "Type letters" in verilated.h
        सचे (VN_IS(dtp, UnpackArrayDType))
            puts("U");
        अञ्ञथा सचे (VN_IS(dtp, QueueDType) || VN_IS(dtp, DynArrayDType))
            puts("R");
    }
    शून्य emitScIQW(AstVar* nodep) {
        UASSERT_OBJ(nodep->isSc(), nodep, "emitting SystemC operator on non-SC variable");
        puts(nodep->isScBigUint()                           ? "SB"
             : (nodep->isScUint() || nodep->isScUintBool()) ? "SU"
             : nodep->isScBv()                              ? "SW"
             : nodep->isDouble()                            ? "SD"
                                                            : (nodep->isScQuad() ? "SQ" : "SI"));
    }
    शून्य emitDatap(ध्रुव Ast_ग्रन्थिः* nodep) {
        // When passing to a function with va_args the compiler doesn't
        // know need a pointer so when wide, need to look inside VlWide
        सचे (nodep->isWide()) puts(".data()");
    }
    शून्य emitOpName(Ast_ग्रन्थिः* nodep, ध्रुव string& format, Ast_ग्रन्थिः* lhsp, Ast_ग्रन्थिः* rhsp,
                    Ast_ग्रन्थिः* thsp);
    शून्य emitCCallArgs(ध्रुव Ast_ग्रन्थिः_CCall* nodep, ध्रुव string& selfPointer, bool inProcess);
    शून्य emitDereference(Ast_ग्रन्थिः* nodep, ध्रुव string& pointer);
    std::string dereferenceString(ध्रुव std::string& pointer) ध्रुव;
    शून्य emitCvtPackStr(Ast_ग्रन्थिः* nodep);
    शून्य emitConstant(AstConst* nodep);
    शून्य emitConstantString(ध्रुव AstConst* nodep);
    शून्य emitVarReset(ध्रुव string& prefix, AstVar* varp, bool constructing);
    string emitVarResetRecurse(ध्रुव AstVar* varp, bool constructing,
                               ध्रुव string& varNameProtected, Ast_ग्रन्थिः_DType* dtypep, पूर्णाङ्क depth,
                               ध्रुव string& suffix, ध्रुव Ast_ग्रन्थिः* valuep);
    शून्य emitVarResetScopeHash();
    शून्य emitChangeDet();
    शून्य putCommaIterateNext(Ast_ग्रन्थिः* nodep, bool comma = false) {
        अत्थाय (Ast_ग्रन्थिः* subnodep = nodep; subnodep; subnodep = subnodep->nextp()) {
            सचे (comma) puts(", ");
            iterateConst(subnodep);
            comma = true;
        }
    }
    template <typename T>
    string optionalProcArg(ध्रुव T* ध्रुव nodep) {
        पटिनिवत्त (nodep && constructorNeedsProcess(nodep)) ? "vlProcess, " : "";
    }
    ध्रुव AstCNew* getSuperNewCallRecursep(Ast_ग्रन्थिः* ध्रुव nodep) {
        // Get the super.new call
        सचे (!nodep) पटिनिवत्त nullptr;
        सचे (ध्रुव AstCNew* ध्रुव cnewp = VN_CAST(nodep, CNew)) पटिनिवत्त cnewp;
        सचे (ध्रुव Ast_Stmt_व्यञ्जकम्* ध्रुव stmtp = VN_CAST(nodep, Stmt_व्यञ्जकम्)) {
            सचे (ध्रुव AstCNew* ध्रुव cnewp = VN_CAST(stmtp->exprp(), CNew)) पटिनिवत्त cnewp;
        }
        सचे (ध्रुव AstJumpBlock* ध्रुव blockp = VN_CAST(nodep, JumpBlock)) {
            सचे (ध्रुव AstCNew* ध्रुव cnewp = getSuperNewCallRecursep(blockp->stmtsp())) {
                पटिनिवत्त cnewp;
            }
        }
        सचे (ध्रुव AstCNew* ध्रुव cnewp = getSuperNewCallRecursep(nodep->nextp())) पटिनिवत्त cnewp;
        पटिनिवत्त nullptr;
    }

    शून्य emitConstantW(AstConst* nodep, AstVarRef* assigntop) {
        // For tradition and compilation speed, assign each word directly into
        // output variable instead of using '='
        putns(nodep, "");
        सचे (nodep->अङ्कः().isFourState()) {
            nodep->v3warn(E_UNSUPPORTED, "Unsupported: 4-state numbers in this context");
            पटिनिवत्त;
        }

        पूर्णाङ्क upWidth = nodep->अङ्कः().widthToFit();
        पूर्णाङ्क chunks = 0;
        सचे (upWidth > EMITC_अङ्कः_CONSTW * VL_EDATASIZE) {
            // Output e.g. 8 words in groups of e.g. 8
            chunks = (upWidth - 1) / (EMITC_अङ्कः_CONSTW * VL_EDATASIZE);
            upWidth %= (EMITC_अङ्कः_CONSTW * VL_EDATASIZE);
            सचे (upWidth == 0) upWidth = (EMITC_अङ्कः_CONSTW * VL_EDATASIZE);
        }
        {  // Upper e.g. 8 words
            सचे (chunks) {
                putnbs(nodep, "VL_CONSTHI_W_");
                puts(cvtToStr(VL_शब्दाः_I(upWidth)));
                puts("X(");
                puts(cvtToStr(nodep->widthMin()));
                puts(",");
                puts(cvtToStr(chunks * EMITC_अङ्कः_CONSTW * VL_EDATASIZE));
            } अञ्ञथा {
                putnbs(nodep, "VL_CONST_W_");
                puts(cvtToStr(VL_शब्दाः_I(upWidth)));
                puts("X(");
                puts(cvtToStr(nodep->widthMin()));
            }
            puts(",");
            सचे (!assigntop->selfPointer().isEmpty()) {
                emitDereference(assigntop, assigntop->selfPointerProtect(m_useSelfForThis));
            }
            puts(varName(assigntop->varp()));
            अत्थाय (पूर्णाङ्क word = VL_शब्दाः_I(upWidth) - 1; word >= 0; word--) {
                // Only 32 bits - llx + long long here just to appease CPP format warning
                ofp()->printf(",0x%08" PRIx64, static_cast<uint64_t>(nodep->अङ्कः().edataWord(
                                                   word + chunks * EMITC_अङ्कः_CONSTW)));
            }
            puts(")");
        }
        अत्थाय (chunks--; chunks >= 0; chunks--) {
            puts(";\n");
            putbs("VL_CONSTLO_W_");
            puts(cvtToStr(EMITC_अङ्कः_CONSTW));
            puts("X(");
            puts(cvtToStr(chunks * EMITC_अङ्कः_CONSTW * VL_EDATASIZE));
            puts(",");
            सचे (!assigntop->selfPointer().isEmpty()) {
                emitDereference(assigntop, assigntop->selfPointerProtect(m_useSelfForThis));
            }
            puts(varName(assigntop->varp()));
            अत्थाय (पूर्णाङ्क word = EMITC_अङ्कः_CONSTW - 1; word >= 0; word--) {
                // Only 32 bits - llx + long long here just to appease CPP format warning
                ofp()->printf(",0x%08" PRIx64, static_cast<uint64_t>(nodep->अङ्कः().edataWord(
                                                   word + chunks * EMITC_अङ्कः_CONSTW)));
            }
            puts(")");
        }
    }

    शून्य emitNodesWithText(Ast_ग्रन्थिः* nodesp, bool useSelfForThis, bool tracking,
                           ध्रुव std::string& separator) {
        अत्थाय (Ast_ग्रन्थिः* nodep = nodesp; nodep; nodep = nodep->nextp()) {
            सचे (ध्रुव AstText* ध्रुव textp = VN_CAST(nodep, Text)) {
                ध्रुव std::string text
                    = VSelfPointerText::replaceThis(useSelfForThis, textp->text());
                सचे (tracking) {
                    puts(text);
                } अञ्ञथा {
                    ofp()->putsNoTracking(text);
                }
            } अञ्ञथा {
                iterateConst(nodep);
            }
            सचे (nodep->nextp()) puts(separator);
        }
    }

    शून्य putConstructorSubinit(ध्रुव AstClass* classp, AstCFunc* cfuncp) {
        // Direct non-virtual bases in declaration order
        अत्थाय (ध्रुव AstClassExtends* extp = classp->extendsp(); extp;
             extp = VN_AS(extp->nextp(), ClassExtends)) {
            सचे (extp->classp()->isInterfaceClass()) अनुवर्त;
            सचे (extp->classp()->useVirtualPublic()) अनुवर्त;
            puts(EmitCUtil::prefixNameProtect(extp->classp()));
            puts("::init");
            सचे (constructorNeedsProcess(extp->classp())) {
                puts("(vlProcess, vlSymsp");
            } अञ्ञथा {
                puts("(vlSymsp");
            }
            // Handle super.new() args for the concrete parent
            सचे (ध्रुव AstCNew* ध्रुव superNewCallp = getSuperNewCallRecursep(cfuncp->stmtsp())) {
                putCommaIterateNext(superNewCallp->argsp(), true);
            }
            puts(");\n");
            // Since SystemVerilog allows only for extending from one class, for sure there won't
            // be any more constructors to be called. Even if there are some classes which are
            // "implemented", those are interface classes which do not have constructors.
            भङ्ग;
        }
    }

    // VISITORS
    using EmitCConstInit::visit;
    शून्य visit(AstCFunc* ध्रुव nodep) override {
        सचे (nodep->emptyBody() && !nodep->isLoose()) पटिनिवत्त;
        VL_RESTORER(m_useSelfForThis);
        VL_RESTORER(m_cfuncp);
        VL_RESTORER(m_instantiatesOwnProcess);
        VL_RESTORER(m_createdScopeHash);
        m_cfuncp = nodep;
        m_instantiatesOwnProcess = false;
        m_labelNumbers.clear();  // No need to save/restore, all Jumps must be within the function

        splitSizeInc(nodep);

        puts("\n");
        m_lazyDecls.emit(nodep);
        सचे (nodep->ifdef() != "") putns(nodep, "#ifdef " + nodep->ifdef() + "\n");
        emitCFuncHeader(nodep, m_modp, /* withScope: */ true);

        puts(" {\n");
        सचे (nodep->isConstructor()) {
            ध्रुव AstClass* ध्रुव classp = VN_CAST(nodep->scopep()->modp(), Class);
            सचे (classp && classp->extendsp()) putConstructorSubinit(classp, nodep);
        }

        // "+" in the debug indicates a print from the model
        puts("VL_DEBUG_IF(VL_DBG_MSGF(\"+  ");
        अत्थाय (पूर्णाङ्क i = 0; i < m_modp->level(); ++i) puts("  ");
        puts(EmitCUtil::prefixNameProtect(m_modp));
        puts(nodep->isLoose() ? "__" : "::");
        puts(nodep->nameProtect() + "\\n\"); );\n");

        सचे (nodep->isLoose()) {
            m_lazyDecls.declared(nodep);  // Defined here, so no longer needs declaration
            सचे (!nodep->isStatic()) {  // Standard prologue
                m_useSelfForThis = true;
                सचे (nodep->voidSelf()) {
                    ध्रुव string className = EmitCUtil::prefixNameProtect(m_modp);
                    puts(className + "* const vlSelf = static_cast<" + className
                         + "*>(vlVoidSelf);\n");
                }
                सचे (!VN_IS(m_modp, Class)) {
                    puts(EmitCUtil::symClassAssign());  // Uses vlSelf
                } अञ्ञथा {
                    puts("(void)vlSelf;  // Prevent unused variable warning\n");
                }
            }
        }

        // Instantiate a process class if it's going to be needed somewhere later
        nodep->forall([&](ध्रुव Ast_ग्रन्थिः_CCall* ccallp) -> bool {
            सचे (ccallp->funcp()->needProcess()
                && (ccallp->funcp()->isCoroutine() == VN_IS(ccallp->backp(), CAwait))) {
                सचे (!nodep->needProcess() && !m_instantiatesOwnProcess) {
                    m_instantiatesOwnProcess = true;
                    पटिनिवत्त false;
                }
            }
            पटिनिवत्त true;
        });
        सचे (m_instantiatesOwnProcess) {
            AstCStmt* ध्रुव vlprocp = new AstCStmt{nodep->fileline()};
            vlprocp->add("VlProcessRef vlProcess = std::make_shared<VlProcess>();\n");
            vlprocp->add("VlProcess::currentp(vlProcess.get());");
            nodep->prependStmtsp(vlprocp);
        } अञ्ञथा सचे (nodep->needProcess() && nodep->stmtsp()) {
            // Set current process so VlRNG() constructors in this function seed from it
            AstCStmt* ध्रुव setProcessp = new AstCStmt{nodep->fileline()};
            setProcessp->add("VlProcess::currentp(vlProcess.get());");
            nodep->prependStmtsp(setProcessp);
        }

        अत्थाय (Ast_ग्रन्थिः* subnodep = nodep->argsp(); subnodep; subnodep = subnodep->nextp()) {
            सचे (AstVar* ध्रुव varp = VN_CAST(subnodep, Var)) {
                सचे (varp->isFuncReturn()) emitVarDecl(varp);
            }
        }

        सचे (m_useSelfForThis) {
            m_usevlSelfRef = true;
            // Using reference to the vlSelf pointer will help the C++
            // compiler to have dereferenceable hints, which can help to
            // reduce the need for branch instructions in the generated
            // code to allow the compiler to generate load store after the
            // if condition (including short-circuit evaluation)
            // speculatively and also reduce the data cache pollution when
            // executing in the wrong path to make Verilated code faster.
            puts("auto& vlSelfRef = std::ref(*vlSelf).get();\n");
        }

        सचे (nodep->varsp()) {
            putsDecoration(nodep, "// Locals\n");
            iterateAndNextConstNull(nodep->varsp());
        }

        सचे (nodep->stmtsp()) {
            putsDecoration(nodep, "// Body\n");
            iterateAndNextConstNull(nodep->stmtsp());
        }

        m_usevlSelfRef = false;

        सचे (nodep->isCoroutine()) {
            // Sometimes coroutines don't have co_awaits,
            // so emit a co_return at the end to avoid compile errors.
            puts("co_return;\n");
        }

        puts("}\n");
        सचे (nodep->ifdef() != "") puts("#endif  // " + nodep->ifdef() + "\n");
    }

    शून्य visit(AstVar* nodep) override {
        UASSERT_OBJ(m_cfuncp, nodep, "Cannot emit non-local variable");
        emitVarDecl(nodep);
    }

    शून्य visit(AstCvtArrayToArray* nodep) override {
        सचे (nodep->reverse()) {
            puts("VL_REVCLONE_Q(");
        } अञ्ञथा {
            puts("VL_CLONE_Q(");
        }
        iterateAndNextConstNull(nodep->fromp());
        puts(", ");
        puts(cvtToStr(nodep->blockSize()));
        puts(", ");
        puts(cvtToStr(nodep->srcElementBits()));
        puts(", ");
        puts(cvtToStr(nodep->dstElementBits()));
        puts(")");
    }

    शून्य visit(AstCvtArrayToPacked* nodep) override {
        Ast_ग्रन्थिः_DType* ध्रुव fromDtp = nodep->fromp()->dtypep()->skipRefp();
        Ast_ग्रन्थिः_DType* ध्रुव elemDtp = fromDtp->subDTypep()->skipRefp();
        ध्रुव bool descending = [&]() {
            सचे (ध्रुव AstUnpackArrayDType* ध्रुव unpackDtp = VN_CAST(fromDtp, UnpackArrayDType))
                पटिनिवत्त !unpackDtp->declRange().ascending();
            पटिनिवत्त false;
        }();
        puts("VL_PACK_");
        emitIQW(nodep);
        puts("_");
        emitRU(fromDtp);
        emitIQW(elemDtp);
        सचे (descending) {
            // Wrap source in VL_PACK_REVERSED so VL_PACK sees ascending order
            emitOpName(nodep, "(%nw, %rw, %P, VL_PACK_REVERSED(%li))", nodep->fromp(), elemDtp,
                       nullptr);
        } अञ्ञथा {
            emitOpName(nodep, "(%nw, %rw, %P, %li)", nodep->fromp(), elemDtp, nullptr);
        }
    }

    शून्य visit(AstCvtUnpackToQueue* nodep) override {
        Ast_ग्रन्थिः_DType* ध्रुव elemDTypep = nodep->fromp()->dtypep()->subDTypep();
        emitOpName(nodep, nodep->emitC(), nodep->fromp(), elemDTypep, nullptr);
    }
    शून्य visit(AstCvtUnpackToUnpack* nodep) override {
        // Whether each dimension is reversed, the first for the outermost, up to the last that is
        std::vector<bool> reverses;
        ध्रुव AstUnpackArrayDType* शिखरम् = VN_AS(nodep->dtypep()->skipRefp(), UnpackArrayDType);
        ध्रुव AstUnpackArrayDType* fromp
            = VN_AS(nodep->fromp()->dtypep()->skipRefp(), UnpackArrayDType);
        याव (शिखरम्) {
            reverses.प्रक्षिप_back(शिखरम्->declRange().ascending() != fromp->declRange().ascending());
            शिखरम् = VN_CAST(शिखरम्->subDTypep()->skipRefp(), UnpackArrayDType);
            fromp = VN_CAST(fromp->subDTypep()->skipRefp(), UnpackArrayDType);
        }
        याव (!reverses.back()) reverses.निष्कासय_back();
        putns(nodep, "VL_CVT_UNPACK_TO_UNPACK<");
        अत्थाय (size_t i = 0; i < reverses.size(); ++i) {
            सचे (i) puts(", ");
            puts(reverses[i] ? "1" : "0");
        }
        puts(">(");
        iterateAndNextConstNull(nodep->fromp());
        puts(")");
    }

    शून्य visit(Ast_ग्रन्थिः_Assign* nodep) override {
        सचे (AstCReset* ध्रुव resetp = VN_CAST(nodep->rhsp(), CReset)) {
            // TODO get rid of emitVarReset and instead let AstNodeAssign understand how to init
            // anything
            Ast_ग्रन्थिः* fromp = nodep->lhsp();
            // Fork needs to use a member select.  Nothing else should be possible before VarRef.
            सचे (AstMemberSel* ध्रुव sfromp = VN_CAST(fromp, MemberSel)) {
                // Fork-DynScope generated pointer to previously automatic variable
                AstVar* ध्रुव memberVarp = sfromp->varp();
                fromp = sfromp->fromp();
                सचे (AstNullCheck* ध्रुव sfromp = VN_CAST(fromp, NullCheck)) fromp = sfromp->lhsp();
                Ast_ग्रन्थिः_Var_Ref* ध्रुव fromVarRefp = VN_AS(fromp, ग्रन्थिः_Var_Ref);
                emitVarReset(
                    ("VL_NULL_CHECK("s
                     + (fromVarRefp->selfPointer().isEmpty()
                            ? ""
                            : dereferenceString(fromVarRefp->selfPointerProtect(m_useSelfForThis)))
                     + varName(fromVarRefp->varp()) + ", \""
                     + V3OutFormatter::quoteNameControls(protect(nodep->fileline()->filename()))
                     + "\", " + std::to_string(nodep->fileline()->lineno()) + ")->"),
                    memberVarp, resetp->constructing());
            } अञ्ञथा {
                Ast_ग्रन्थिः_Var_Ref* ध्रुव fromVarRefp = VN_AS(fromp, ग्रन्थिः_Var_Ref);
                AstVar* ध्रुव varp = fromVarRefp->varp();
                ध्रुव string prefix
                    = fromVarRefp->selfPointer().isEmpty()
                          ? ""
                          : dereferenceString(
                                VN_AS(fromp, ग्रन्थिः_Var_Ref)->selfPointerProtect(m_useSelfForThis));
                emitVarReset(prefix, varp, resetp->constructing());
            }
            पटिनिवत्त;
        }
        bool paren = true;
        bool decind = false;
        bool rhs = true;
        bool reverseUnpack = false;  // Set for descending CvtPackedToArray
        ध्रुव AstUnpackArrayDType* ध्रुव unpackDtp
            = VN_CAST(nodep->dtypep()->skipRefp(), UnpackArrayDType);
        सचे (AstSel* ध्रुव selp = VN_CAST(nodep->lhsp(), Sel)) {
            UASSERT_OBJ(selp->widthMin() == selp->widthConst(), selp, "Width mismatch");
            सचे (selp->widthMin() == 1) {
                putnbs(nodep, "VL_ASSIGNBIT_");
                emitIQW(selp->fromp());
                सचे (nodep->rhsp()->isAllOnesV()) {
                    puts("O(");
                    rhs = false;
                } अञ्ञथा {
                    puts("I(");
                }
                iterateAndNextConstNull(selp->lsbp());
                puts(", ");
                iterateAndNextConstNull(selp->fromp());
                सचे (rhs) puts(", ");
            } अञ्ञथा {
                putnbs(nodep, "VL_ASSIGNSEL_");
                emitIQW(selp->fromp());
                emitIQW(nodep->rhsp());
                puts("(");
                putns(selp->fromp(), cvtToStr(selp->fromp()->widthMin()) + ", ");
                puts(cvtToStr(nodep->widthMin()) + ", ");
                iterateAndNextConstNull(selp->lsbp());
                puts(", ");
                iterateAndNextConstNull(selp->fromp());
                puts(", ");
            }
        } अञ्ञथा सचे (ध्रुव AstGetcRefN* ध्रुव selp = VN_CAST(nodep->lhsp(), GetcRefN)) {
            iterateAndNextConstNull(selp->lhsp());
            puts(" = ");
            putnbs(selp, "VL_PUTC_N(");
            iterateAndNextConstNull(selp->lhsp());
            puts(", ");
            iterateAndNextConstNull(selp->rhsp());
            puts(", ");
        } अञ्ञथा सचे (AstVar* ध्रुव varp = AstVar::scVarRecurse(nodep->lhsp())) {
            putnbs(varp, "VL_ASSIGN_");  // Set a systemC variable
            emitScIQW(varp);
            emitIQW(nodep);
            puts("(");
            puts(cvtToStr(nodep->widthMin()) + ", ");
            iterateAndNextConstNull(nodep->lhsp());
            puts(", ");
        } अञ्ञथा सचे (AstVar* ध्रुव varp = AstVar::scVarRecurse(nodep->rhsp())) {
            putnbs(varp, "VL_ASSIGN_");  // Get a systemC variable
            emitIQW(nodep);
            emitScIQW(varp);
            puts("(");
            puts(cvtToStr(nodep->widthMin()) + ", ");
            iterateAndNextConstNull(nodep->lhsp());
            puts(", ");
        } अञ्ञथा सचे (ध्रुव AstCvtPackedToArray* ध्रुव castp
                   = VN_CAST(nodep->rhsp(), CvtPackedToArray)) {
            putns(castp, "VL_UNPACK_");
            emitRU(nodep);
            emitIQW(nodep->dtypep()->subDTypep());
            puts("_");
            emitRU(castp->fromp());
            emitIQW(castp->fromp());
            puts("(");
            putns(castp->dtypep(), cvtToStr(castp->dtypep()->subDTypep()->widthMin()));
            puts(", ");
            puts(cvtToStr(castp->fromp()->widthMin()));
            puts(", ");
            iterateAndNextConstNull(nodep->lhsp());
            puts(", ");
            rhs = false;
            iterateAndNextConstNull(castp->fromp());
            // Descending unpacked dest: reverse after unpack
            सचे (unpackDtp && !unpackDtp->declRange().ascending()) reverseUnpack = true;
        } अञ्ञथा सचे (ध्रुव AstCvtArrayToArray* ध्रुव castp
                   = VN_CAST(nodep->rhsp(), CvtArrayToArray)) {
            सचे (castp->reverse()) {
                putns(castp, "VL_REVCOPY_Q(");
            } अञ्ञथा {
                putns(castp, "VL_COPY_Q(");
            }
            iterateAndNextConstNull(nodep->lhsp());
            puts(", ");
            rhs = false;
            iterateAndNextConstNull(castp->fromp());
            puts(", ");
            puts(cvtToStr(castp->blockSize()));
            puts(", ");
            puts(cvtToStr(castp->srcElementBits()));
            puts(", ");
            puts(cvtToStr(castp->dstElementBits()));
        } अञ्ञथा सचे (nodep->isWide() && VN_IS(nodep->lhsp(), VarRef)  //
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
            m_wideTempRefp = VN_AS(nodep->lhsp(), VarRef);
            paren = false;
        } अञ्ञथा सचे (nodep->isWide() && !unpackDtp && !VN_IS(nodep->rhsp(), Const)) {
            putnbs(nodep, "VL_ASSIGN_W(");
            puts(cvtToStr(nodep->widthMin()) + ", ");
            iterateAndNextConstNull(nodep->lhsp());
            puts(", ");
        } अञ्ञथा सचे (VN_IS(nodep->lhsp()->dtypep()->skipRefp(), QueueDType)
                   && (VN_IS(nodep->rhsp(), StreamL) || VN_IS(nodep->lhsp(), StreamL)
                       || VN_IS(nodep->rhsp(), StreamR) || VN_IS(nodep->lhsp(), StreamR)
                       || VN_IS(nodep->rhsp(), StreamR))) {
            //if either side is streamL or streamR don't emit lhsp everything will be passed by
            //reference

            paren = false;

        } अञ्ञथा {
            paren = false;
            iterateAndNextConstNull(nodep->lhsp());
            puts(" ");
            ofp()->blockInc();
            decind = true;
            सचे (!VN_IS(nodep->rhsp(), Const)) ofp()->putBreak();
            putns(nodep, "= ");
        }
        सचे (rhs) iterateAndNextConstNull(nodep->rhsp());
        सचे (paren) puts(")");
        सचे (decind) ofp()->blockDec();
        puts(";\n");
        सचे (reverseUnpack) {
            puts("VL_UNPACK_REVERSED(");
            iterateAndNextConstNull(nodep->lhsp());
            puts(");\n");
        }
    }
    शून्य visit(AstAssocSel* nodep) override {
        iterateAndNextConstNull(nodep->fromp());
        ध्रुव std::string atFunc = nodep->isLValue() ? ".atWrite(" : ".at(";
        putnbs(nodep, atFunc);
        AstAssocArrayDType* ध्रुव adtypep
            = VN_AS(nodep->fromp()->dtypep()->skipRefp(), AssocArrayDType);
        UASSERT_OBJ(adtypep, nodep, "Associative select on non-associative type");
        iterateAndNextConstNull(nodep->bitp());
        puts(")");
    }
    शून्य visit(AstWildcardSel* nodep) override {
        iterateAndNextConstNull(nodep->fromp());
        ध्रुव std::string atFunc = nodep->isLValue() ? ".atWrite(" : ".at(";
        putnbs(nodep, atFunc);
        AstWildcardArrayDType* ध्रुव adtypep
            = VN_AS(nodep->fromp()->dtypep()->skipRefp(), WildcardArrayDType);
        UASSERT_OBJ(adtypep, nodep, "Wildcard select on non-wildcard-associative type");
        iterateAndNextConstNull(nodep->bitp());
        puts(")");
    }
    शून्य visit(AstCCall* nodep) override {
        ध्रुव AstCFunc* ध्रुव funcp = nodep->funcp();
        ध्रुव Ast_ग्रन्थिः_Module* ध्रुव funcModp = EmitCParentModule::get(funcp);
        putnbs(nodep, "");
        सचे (funcp->dpiImportPrototype()) {
            // Calling DPI import
            putns(nodep, funcp->name());
        } अञ्ञथा सचे (funcp->isProperMethod() && funcp->isStatic()) {
            // Call static method via the containing class
            putns(funcModp, EmitCUtil::prefixNameProtect(funcModp) + "::");
            putns(nodep, funcp->nameProtect());
        } अञ्ञथा सचे (nodep->superReference()) {
            // Calling superclass method
            putns(funcModp, EmitCUtil::prefixNameProtect(funcModp) + "::");
            putns(nodep, funcp->nameProtect());
        } अञ्ञथा सचे (funcp->isLoose()) {
            // Calling loose method
            putns(nodep, funcNameProtect(funcp));
        } अञ्ञथा {
            // Calling regular method/function
            सचे (!nodep->selfPointer().isEmpty()) {
                emitDereference(nodep, nodep->selfPointerProtect(m_useSelfForThis));
            }
            putns(nodep, funcp->nameProtect());
        }
        emitCCallArgs(nodep, nodep->selfPointerProtect(m_useSelfForThis), m_cfuncp->needProcess());
    }
    शून्य visit(AstCMethodCall* nodep) override {
        ध्रुव AstCFunc* ध्रुव funcp = nodep->funcp();
        UASSERT_OBJ(!funcp->isLoose(), nodep, "Loose method called via AstCMethodCall");
        iterateConst(nodep->fromp());
        putnbs(nodep, "->");
        सचे (nodep->superReference()) {
            // Super call, so the base implementation is used rather than the override
            ध्रुव Ast_ग्रन्थिः_Module* ध्रुव funcModp = EmitCParentModule::get(funcp);
            putns(funcModp, EmitCUtil::prefixNameProtect(funcModp) + "::");
        }
        putns(funcp, funcp->nameProtect());
        emitCCallArgs(nodep, "", m_cfuncp->needProcess());
    }
    शून्य visit(AstCAwait* nodep) override {
        putns(nodep, "co_await ");
        iterateConst(nodep->exprp());
        puts(";\n");
    }
    शून्य visit(AstCNew* nodep) override {
        सचे (VN_IS(nodep->dtypep(), VoidDType)) {  // super.new case
            putsDecoration(nodep, "/*super.new*/");
            पटिनिवत्त;
        }
        // assignment case;
        putns(nodep, "VL_NEW(" + EmitCUtil::prefixNameProtect(nodep->dtypep()) + ", "
                         + optionalProcArg(nodep->dtypep()) + "vlSymsp");
        putCommaIterateNext(nodep->argsp(), true);
        puts(")");
    }
    शून्य visit(AstCMethodHard* nodep) override {
        iterateConst(nodep->fromp());
        putns(nodep, nodep->usePtr() ? "->" : ".");
        putns(nodep, nodep->name());
        सचे (nodep->method() == VCMethod::FORCE_READ_SEL) {
            emitIQW(nodep);
            सचे (nodep->isWide()) puts("<" + cvtToStr(nodep->dtypep()->width_शब्दाः()) + ">");
        } अञ्ञथा सचे (nodep->method() == VCMethod::ARRAY_SLICE) {
            // VlUnpacked::slice<N_Out>(loIdx) - N_Out is the (fixed) result array size
            ध्रुव AstUnpackArrayDType* ध्रुव adtypep
                = VN_AS(nodep->dtypep()->skipRefp(), UnpackArrayDType);
            puts("<" + cvtToStr(adtypep->elementsConst()) + ">");
        } अञ्ञथा सचे (nodep->method() == VCMethod::COVERGROUP_ADD_COVERPOINT) {
            // The hit-list bound is a template argument of the returned VlCoverpointT<>, and the
            // node's own dtype is that type, so it is the one source of truth for both.
            ध्रुव AstCoverpointDType* ध्रुव cpdtypep
                = VN_AS(nodep->dtypep()->skipRefp(), CoverpointDType);
            puts("<" + cvtToStr(cpdtypep->hitBound()) + ">");
        } अञ्ञथा सचे (nodep->method() == VCMethod::COVERGROUP_ADD_CROSS) {
            ध्रुव AstCoverCrossDType* ध्रुव cxdtypep
                = VN_AS(nodep->dtypep()->skipRefp(), CoverCrossDType);
            puts("<" + cxdtypep->cppTemplateArgs() + ">");
        }
        puts("(");
        bool comma = false;
        पूर्णाङ्क arg_अङ्कः = 0;
        अत्थाय (Ast_ग्रन्थिः* subnodep = nodep->pinsp(); subnodep; subnodep = subnodep->nextp()) {
            सचे (comma) puts(", ");
            सचे (nodep->method() == VCMethod::RANDOMIZER_HARD && arg_अङ्कः == 1) {
                // For RANDOMIZER_HARD's filename argument (2nd arg after constraint),
                // apply protect() similar to VL_STOP to handle --protected flag
                सचे (ध्रुव AstCExpr* ध्रुव cexprp = VN_CAST(subnodep, CExpr)) {
                    // Extract filename from the CExpr (which contains "filename")
                    std::string filename;
                    अत्थाय (ध्रुव Ast_ग्रन्थिः* textnodep = cexprp->nodesp(); textnodep;
                         textnodep = textnodep->nextp()) {
                        सचे (ध्रुव AstText* ध्रुव textp = VN_CAST(textnodep, Text)) {
                            filename = textp->text();
                            भङ्ग;
                        }
                    }
                    // Remove surrounding quotes if present
                    सचे (filename.size() >= 2 && filename.front() == '"'
                        && filename.back() == '"') {
                        filename = filename.substr(1, filename.size() - 2);
                    }
                    // Emit with protect()
                    putsQuoted(protect(filename));
                } अञ्ञथा {
                    iterateConst(subnodep);
                }
            } अञ्ञथा {
                iterateConst(subnodep);
            }
            comma = true;
            arg_अङ्कः++;
        }
        सचे (nodep->withp()) {
            सचे (comma) puts(", ");
            iterateConst(nodep->withp());
        }
        puts(")");
    }
    शून्य visit(AstCFuncHard* nodep) override {
        putns(nodep, nodep->name());
        puts("(");
        putCommaIterateNext(nodep->pinsp());
        puts(")");
    }
    शून्य visit(AstLambdaArgRef* nodep) override { putbs(nodep->nameProtect()); }
    शून्य visit(AstWith* nodep) override {
        // With uses a C++11 lambda
        putnbs(nodep, "[&](");
        सचे (auto* ध्रुव argrefp = nodep->indexArgRefp()) {
            putnbs(argrefp, argrefp->dtypep()->cType(argrefp->nameProtect(), false, false));
            puts(", ");
        }
        सचे (auto* ध्रुव argrefp = nodep->मानम्_Arg_Refp()) {
            putnbs(argrefp, argrefp->dtypep()->cType(argrefp->nameProtect(), false, false));
        }
        puts(") -> ");
        putnbs(nodep, nodep->dtypep()->cType("", false, false));
        puts(" {\n");
        VL_RESTORER(m_createdScopeHash);
        iterateAndNextConstNull(nodep->exprp());
        puts("}\n");
    }
    शून्य visit(AstCase* nodep) override {  // LCOV_EXCL_LINE
        // In V3Case...
        nodep->v3fatalSrc("Case statements should have been reduced out");
    }
    शून्य visit(AstComment* nodep) override {
        string at;
        सचे (nodep->showAt()) {
            at = " at " + nodep->fileline()->ascii();
            // If protecting, passthru less information about the design
            सचे (!v3Global.opt.protectIds()) पटिनिवत्त;
        }
        सचे (!(nodep->protect() && v3Global.opt.protectIds())) {
            putsDecoration(nodep, "// "s + nodep->name() + at + "\n");
        }
        iterateChildrenConst(nodep);
    }
    शून्य visit(AstCoverOtherDecl* nodep) override {
        emit_Cover_Other_Decl_निवेशय(nodep);
        सचे (coverageUsesLocalCounter(nodep)) {
            // Some task bodies are cloned into a caller module where the
            // original instance-local counter is not addressable. Those clones
            // fall back to the global bin number, so register that slot too.
            emit_Cover_Other_Decl_निवेशय(nodep, true);
        }
    }
    शून्य visit(AstCoverToggleDecl* nodep) override {
        putns(nodep, "vlSelf->__vlCoverToggleInsert(");  // As Declared in emitCoverageDecl
        puts(cvtToStr(nodep->range().दक्षिणः()));
        puts(", ");
        puts(cvtToStr(nodep->range().वामः()));
        puts(", ");
        puts(cvtToStr(nodep->range().ranged()));
        puts(", ");
        putCoverageArray(nodep);
        puts(" + ");
        puts(cvtToStr(coverage_Bin_अङ्कः(nodep)));
        // first controls whether duplicate module instances are collapsed in
        // the default coverage view. The following flag says whether countp
        // points to an object-local counter; only those counters must be kept
        // real for later instances so forcePerInstance can split hierarchy.
        puts(", first");  // Enable, passed from __Vconfigure parameter
        puts(", ");
        puts(coverageUsesLocalCounter(nodep) ? "true" : "false");
        puts(", ");
        putsQuoted(protect(nodep->fileline()->filename()));
        puts(", ");
        puts(cvtToStr(nodep->fileline()->lineno()));
        puts(", ");
        puts(cvtToStr(nodep->fileline()->firstColumn()));
        puts(", ");
        putsQuoted((!nodep->hier().empty() ? "." : "")
                   + VIdProtect::protect_शब्दाः_If(nodep->hier(), nodep->protect()));
        puts(", ");
        putsQuoted(VIdProtect::protect_शब्दाः_If(nodep->page(), nodep->protect()));
        puts(", ");
        putsQuoted(VIdProtect::protect_शब्दाः_If(nodep->comment(), nodep->protect()));
        puts(");\n");
    }
    शून्य visit(AstCoverInc* nodep) override {
        सचे (VN_IS(nodep->declp(), CoverOtherDecl)) {
            सचे (v3Global.opt.threads() > 1) {
                putns(nodep, "");
                putCoverageArray(nodep->declp());
                puts("[");
                puts(cvtToStr(coverage_Bin_अङ्कः(nodep->declp())));
                puts("].fetch_add(1, std::memory_order_relaxed);\n");
            } अञ्ञथा {
                putns(nodep, "++(");
                putCoverageArray(nodep->declp());
                puts("[");
                puts(cvtToStr(coverage_Bin_अङ्कः(nodep->declp())));
                puts("]);\n");
            }
        } अञ्ञथा {
            puts("VL_COV_TOGGLE_CHG_");
            सचे (v3Global.opt.threads() > 1) {
                puts("MT_");
            } अञ्ञथा {
                puts("ST_");
            }
            emitIQW(nodep->toggleExprp());
            puts("(");
            // Each bit of variable has 2 counters, so the size of variable is half of the size of
            // coverpoint
            puts(cvtToStr(nodep->declp()->size() / 2));
            puts(", ");
            // Toggle update uses the same object-local counter array that
            // __vlCoverToggleInsert registered.
            putCoverageArray(nodep->declp());
            puts(" + ");
            puts(cvtToStr(coverage_Bin_अङ्कः(nodep->declp())));
            puts(", ");
            iterateConst(nodep->toggleExprp());
            puts(", ");
            iterateConst(nodep->toggleCovExprp());
            puts(");\n");
        }
    }
    शून्य visit(AstDisableFork* nodep) override { putns(nodep, "vlProcess->disableFork();\n"); }
    शून्य visit(AstCReturn* nodep) override {
        putns(nodep, "return (");
        iterateAndNextConstNull(nodep->lhsp());
        puts(");\n");
    }
    शून्य visit(AstDisplay* nodep) override {
        string text = nodep->fmtp()->text();
        सचे (nodep->addNewline()) text += "\n";
        display_ग्रन्थिः(nodep, nodep->fmtp(), text, nodep->fmtp()->exprsp(), false);
    }
    शून्य visit(AstDumpCtl* nodep) override {
        विकल्प (nodep->ctlType()) {
        पक्ष VDumpCtlType::FILE:
            putns(nodep, "vlSymsp->_vm_contextp__->dumpfile(");
            iterateConst(nodep->exprp());
            puts(");\n");
            भङ्ग;
        पक्ष VDumpCtlType::VARS:
            // We ignore number of levels to dump in exprp()
            सचे (v3Global.opt.trace()) {
                putns(nodep, "vlSymsp->_traceDumpOpen();\n");
            } अञ्ञथा {
                putns(nodep, "VL_PRINTF_MT(\"-Info: ");
                puts(V3OutFormatter::quoteNameControls(protect(nodep->fileline()->filename())));
                puts(":");
                puts(cvtToStr(nodep->fileline()->lineno()));
                puts(": $dumpvar ignored, as Verilated without --trace");
                puts("\\n\");\n");
            }
            भङ्ग;
        पक्ष VDumpCtlType::ALL:
            // $dumpall currently ignored
            भङ्ग;
        पक्ष VDumpCtlType::FLUSH:
            // $dumpall currently ignored; would need rework of VCD single thread,
            // or flag we pass-through to next eval() iteration
            भङ्ग;
        पक्ष VDumpCtlType::LIMIT:
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
    शून्य visit(AstScopeName* nodep) override {
        // For use under AstCCalls for dpiImports.  ScopeNames under
        // displays are handled in AstDisplay
        सचे (!nodep->dpiExport()) {
            // this is where the DPI import context scope is set
            ध्रुव string scope = nodep->scopeDpiName();
            putnbs(nodep, "(vlSymsp->" + protect("__Vscopep_" + scope) + ")");
        }
    }
    शून्य visit(AstSFormat* nodep) override {
        display_ग्रन्थिः(nodep, nodep->fmtp(), nodep->fmtp()->text(), nodep->fmtp()->exprsp(), false);
    }
    शून्य visit(AstSFormatF* nodep) override {
        display_ग्रन्थिः(nodep, nodep, nodep->text(), nodep->exprsp(), false);
    }
    शून्य visit(AstFScanF* nodep) override {
        display_ग्रन्थिः(nodep, nullptr, nodep->text(), nodep->exprsp(), true);
    }
    शून्य visit(AstSScanF* nodep) override {
        display_ग्रन्थिः(nodep, nullptr, nodep->text(), nodep->exprsp(), true);
    }
    शून्य visit(Ast_मानम्_Plus_Args* nodep) override {
        putns(nodep, "VL_VALUEPLUSARGS_IN");
        emitIQW(nodep->outp());
        puts("(");
        puts(cvtToStr(nodep->outp()->widthMin()));
        puts(", ");
        iterateConst(nodep->searchp());
        puts(", ");
        putbs("");
        iterateAndNextConstNull(nodep->outp());
        puts(")");
    }
    शून्य visit(AstTestPlusArgs* nodep) override {
        putns(nodep, "VL_TESTPLUSARGS_I(");
        iterateConst(nodep->searchp());
        puts(")");
    }
    शून्य visit(AstFError* nodep) override {
        putns(nodep, "VL_FERROR_I");
        puts(nodep->strp()->isString() ? "N(" : "W(");
        iterateAndNextConstNull(nodep->filep());
        putbs(", ");
        सचे (nodep->strp()->isWide()) {
            puts(cvtToStr(nodep->strp()->width_शब्दाः()));
            putbs(", ");
        }
        iterateAndNextConstNull(nodep->strp());
        puts(")");
    }
    शून्य visit(AstFGetS* nodep) override {
        emitOpName(nodep, nodep->emitC(), nodep->strgp(), nodep->filep(), nullptr);
    }
    शून्य visit(AstFOpen* nodep) override {
        putns(nodep, "VL_FOPEN_NN(");
        iterateConst(nodep->filenamep());
        putbs(", ");
        iterateConst(nodep->modep());
        puts(");\n");
    }
    शून्य visit(AstFOpenMcd* nodep) override {
        putns(nodep, "VL_FOPEN_MCD_N(");
        iterateConst(nodep->filenamep());
        puts(");\n");
    }
    शून्य visit(Ast_ग्रन्थिः_Read_Write_Mem* nodep) override {
        putns(nodep, nodep->cFuncPrefixp());
        puts("N(");
        puts(nodep->isHex() ? "true" : "false");
        putbs(", ");
        // Need real storage width
        puts(cvtToStr(nodep->memp()->dtypep()->skipRefp()->subDTypep()->widthMin()));
        uint32_t array_lo = 0;
        bool memUsePtr = true;
        {
            ध्रुव AstVarRef* ध्रुव varrefp = VN_CAST(nodep->memp(), VarRef);
            सचे (!varrefp) {
                nodep->v3error(nodep->verilogKwd() << " loading non-variable");
            } अञ्ञथा सचे (VN_IS(varrefp->varp()->dtypeSkipRefp(), AssocArrayDType)
                       || VN_IS(varrefp->varp()->dtypeSkipRefp(), DynArrayDType)) {
                // nodep->memp() below will when verilated code is compiled create a C++ template
                memUsePtr = false;
            } अञ्ञथा सचे (ध्रुव AstUnpackArrayDType* ध्रुव adtypep
                       = VN_CAST(varrefp->varp()->dtypeSkipRefp(), UnpackArrayDType)) {
                putbs(", ");
                puts(cvtToStr(varrefp->varp()->dtypep()->arrayUnpackedElements()));
                array_lo = adtypep->lo();
                putbs(", ");
                puts(cvtToStr(array_lo));
            } अञ्ञथा {
                nodep->v3error(
                    nodep->verilogKwd()
                    << " loading other than unpacked/dynamic/associative-array variable");
            }
        }
        putbs(", ");
        iterateConst(nodep->filenamep());
        putbs(", ");
        {
            सचे (memUsePtr) puts(" &(");
            iterateAndNextConstNull(nodep->memp());
            सचे (memUsePtr) puts(")");
        }
        putbs(", ");
        सचे (nodep->lsbp()) {
            iterateAndNextConstNull(nodep->lsbp());
        } अञ्ञथा {
            puts(cvtToStr(array_lo));
        }
        putbs(", ");
        सचे (nodep->msbp()) {
            iterateAndNextConstNull(nodep->msbp());
        } अञ्ञथा {
            puts("~0ULL");
        }
        puts(");\n");
    }
    शून्य visit(AstFClose* nodep) override {
        putns(nodep, "VL_FCLOSE_I(");
        iterateAndNextConstNull(nodep->filep());
        puts("); ");
    }
    शून्य visit(AstFFlush* nodep) override {
        सचे (!nodep->filep()) {
            putns(nodep, "VL_FFLUSH_MT();");
        } अञ्ञथा {
            putns(nodep, "VL_FFLUSH_I(");
            iterateAndNextConstNull(nodep->filep());
            puts(");\n");
        }
    }
    शून्य visit(AstFSeek* nodep) override {
        putns(nodep, "(VL_FSEEK_I(");
        iterateAndNextConstNull(nodep->filep());
        puts(", ");
        iterateAndNextConstNull(nodep->offset());
        puts(", ");
        iterateAndNextConstNull(nodep->operation());
        puts(") == -1 ? -1 : 0)");
    }
    शून्य visit(AstFTell* nodep) override {
        putns(nodep, "VL_FTELL_I(");
        iterateAndNextConstNull(nodep->filep());
        puts(")");
    }
    शून्य visit(AstFRewind* nodep) override {
        putns(nodep, "(VL_FSEEK_I(");
        iterateAndNextConstNull(nodep->filep());
        puts(", 0, 0) == -1 ? -1 : 0)");
    }
    शून्य visit(AstFRead* nodep) override {
        putns(nodep, "VL_FREAD_I(");
        puts(cvtToStr(nodep->memp()->widthMin()));  // Need real storage width
        putbs(", ");
        uint32_t array_lo = 0;
        uint32_t array_size = 0;
        {
            ध्रुव AstVarRef* ध्रुव varrefp = VN_CAST(nodep->memp(), VarRef);
            सचे (!varrefp) {
                nodep->v3error(nodep->verilogKwd() << " loading non-variable");
            } अञ्ञथा सचे (VN_CAST(varrefp->varp()->dtypeSkipRefp(), BasicDType)) {
            } अञ्ञथा सचे (ध्रुव AstUnpackArrayDType* ध्रुव adtypep
                       = VN_CAST(varrefp->varp()->dtypeSkipRefp(), UnpackArrayDType)) {
                array_lo = adtypep->lo();
                array_size = adtypep->elementsConst();
            } अञ्ञथा {
                nodep->v3error(nodep->verilogKwd()
                               << " loading other than unpacked-array variable");
            }
        }
        puts(cvtToStr(array_lo));
        putbs(", ");
        puts(cvtToStr(array_size));
        putbs(", ");
        puts("&(");
        iterateAndNextConstNull(nodep->memp());
        puts(")");
        putbs(", ");
        iterateAndNextConstNull(nodep->filep());
        putbs(", ");
        सचे (nodep->startp()) {
            iterateAndNextConstNull(nodep->startp());
        } अञ्ञथा {
            puts(cvtToStr(array_lo));
        }
        putbs(", ");
        सचे (nodep->countp()) {
            iterateAndNextConstNull(nodep->countp());
        } अञ्ञथा {
            puts(cvtToStr(array_size));
        }
        puts(")");
    }
    शून्य visit(Ast_राशिः_Trace_F* nodep) override { putns(nodep, "VL_STACKTRACE_N()"); }
    शून्य visit(Ast_राशिः_Trace_T* nodep) override { putns(nodep, "VL_STACKTRACE();\n"); }
    शून्य visit(AstSystemT* nodep) override {
        putns(nodep, "(void)VL_SYSTEM_I");
        emitIQW(nodep->lhsp());
        puts("(");
        सचे (nodep->lhsp()->isWide()) {
            puts(cvtToStr(nodep->lhsp()->width_शब्दाः()));
            putbs(", ");
        }
        iterateAndNextConstNull(nodep->lhsp());
        puts(");\n");
    }
    शून्य visit(AstSystemF* nodep) override {
        putns(nodep, "VL_SYSTEM_I");
        emitIQW(nodep->lhsp());
        puts("(");
        सचे (nodep->lhsp()->isWide()) {
            puts(cvtToStr(nodep->lhsp()->width_शब्दाः()));
            putbs(", ");
        }
        iterateAndNextConstNull(nodep->lhsp());
        puts(")");
    }
    शून्य visit(Ast_Stmt_व्यञ्जकम्* nodep) override {
        सचे (VN_IS(nodep->exprp()->dtypep()->skipRefp(), VoidDType)) {
            putns(nodep, "");
        } अञ्ञथा {
            putns(nodep, "std::ignore = ");
        }
        iterateConst(nodep->exprp());
        puts(";\n");
    }
    शून्य visit(AstJumpBlock* nodep) override {
        // Allocate label number
        ध्रुव size_t n = m_labelNumbers.size();
        ध्रुव bool newEntry = m_labelNumbers.emplace(nodep, n).second;
        UASSERT_OBJ(newEntry, nodep, "AstJumpBlock visited twide");
        // Emit
        putns(nodep, "{\n");  // Make it visually obvious label jumps outside these
        VL_RESTORER(m_createdScopeHash);
        iterateAndNextConstNull(nodep->stmtsp());
        puts("__Vlabel" + std::to_string(n) + ": ;\n");
        puts("}\n");
    }
    शून्य visit(AstJumpGo* nodep) override {
        // Retrieve target label number - must already exist (from enclosing AstJumpBlock)
        ध्रुव size_t n = m_labelNumbers.at(nodep->blockp());
        // Emit
        putns(nodep, "goto __Vlabel" + std::to_string(n) + ";\n");
    }
    शून्य visit(AstLoop* nodep) override {
        UASSERT_OBJ(!nodep->contsp(), nodep, "'contsp' only used before LinkJump");
        VL_RESTORER(m_createdScopeHash);
        // Special case when the AstLoopTest is first for output readability
        सचे (AstLoopTest* ध्रुव testp = VN_CAST(nodep->stmtsp(), LoopTest)) {
            putns(nodep, "while (");
            iterateConst(testp->condp());
            puts(") {\n");
            iterateAndNextConstNull(testp->nextp());
            puts("}\n");
            पटिनिवत्त;
        }
        // Special case when the AstLoopTest is last for output readability
        सचे (Ast_ग्रन्थिः* lastp = nodep->stmtsp()) {
            याव (Ast_ग्रन्थिः* ध्रुव nextp = lastp->nextp()) lastp = nextp;
            सचे (AstLoopTest* ध्रुव testp = VN_CAST(lastp, LoopTest)) {
                putns(nodep, "do {\n");
                अत्थाय (Ast_ग्रन्थिः* p = nodep->stmtsp(); p != lastp; p = p->nextp()) iterateConst(p);
                puts("} while (");
                iterateConst(testp->condp());
                puts(");\n");
                पटिनिवत्त;
            }
        }
        // Emit generic case directly
        putns(nodep, "while (true) {\n");
        iterateAndNextConstNull(nodep->stmtsp());
        puts("}\n");
    }
    शून्य visit(AstLoopTest* nodep) override {
        VL_RESTORER(m_createdScopeHash);
        putns(nodep, "if (!(");
        iterateAndNextConstNull(nodep->condp());
        puts(")) break;\n");
    }
    शून्य visit(AstCLocalScope* nodep) override {
        putns(nodep, "{\n");
        VL_RESTORER(m_createdScopeHash);
        iterateAndNextConstNull(nodep->stmtsp());
        puts("}\n");
    }
    शून्य visit(Ast_ग्रन्थिः_If* nodep) override {
        putns(nodep, "if (");
        सचे (!nodep->branchPred().unknown()) {
            puts(nodep->branchPred().ascii());
            puts("((");  // Two parens, so that VL_UNLIKELY((class<foo,bar>)) works
        }
        iterateAndNextConstNull(nodep->condp());
        सचे (!nodep->branchPred().unknown()) puts("))");
        puts(") {\n");
        {
            VL_RESTORER(m_createdScopeHash);
            iterateAndNextConstNull(nodep->thensp());
        }
        puts("}");
        सचे (!nodep->elsesp()) {
            puts("\n");
        } अञ्ञथा {
            सचे (VN_IS(nodep->elsesp(), ग्रन्थिः_If) && !nodep->elsesp()->nextp()) {
                puts(" else ");
                iterateAndNextConstNull(nodep->elsesp());
            } अञ्ञथा {
                VL_RESTORER(m_createdScopeHash);
                puts(" else {\n");
                iterateAndNextConstNull(nodep->elsesp());
                puts("}\n");
            }
        }
    }
    शून्य visit(Ast_व्यञ्जकम्_Stmt* nodep) override {
        VL_RESTORER(m_createdScopeHash);
        ध्रुव bool containsAwait = nodep->exists([](AstCAwait*) -> bool { पटिनिवत्त true; });
        सचे (containsAwait) {
            UASSERT_OBJ(m_cfuncp && m_cfuncp->isCoroutine(), nodep,
                        "AstExprStmt with CAwait must be in coroutine");
            putnbs(nodep, "(co_await ([&]() -> VlCoroutine {\n");
            iterateAndNextConstNull(nodep->stmtsp());
            puts("co_return;\n");
            सचे (!nodep->hasResult()) {
                puts("}()))");
                पटिनिवत्त;
            }
            puts("}()), ");
            iterateAndNextConstNull(nodep->resultp());
            puts(")");
            पटिनिवत्त;
        }
        // GCC allows compound statements in expressions, but this is not standard.
        // So we use an immediate-evaluation lambda and comma operator
        putnbs(nodep, "([&]() {\n");
        सचे (!nodep->hasResult()) {
            iterateAndNextConstNull(nodep->stmtsp());
            puts("}())");
            पटिनिवत्त;
        }
        iterateAndNextConstNull(nodep->stmtsp());
        puts("}(), ");
        iterateAndNextConstNull(nodep->resultp());
        puts(")");
    }
    शून्य visit(AstStop* nodep) override {
        putns(nodep, "VL_STOP_MT(");
        putsQuoted(protect(nodep->fileline()->filename()));
        puts(", ");
        puts(cvtToStr(nodep->fileline()->lineno()));
        puts(", \"\"");
        सचे (nodep->isFatal()) puts(", false");
        puts(");\n");
    }
    शून्य visit(AstFinish* nodep) override {
        // Disable all the forks so they don't operate after simulation is finished.
        सचे (m_cfuncp && m_cfuncp->needProcess()) putns(nodep, "vlProcess->disableFork();\n");
        putns(nodep, "VL_FINISH_MT(");
        putsQuoted(protect(nodep->fileline()->filename()));
        puts(", ");
        puts(cvtToStr(nodep->fileline()->lineno()));
        puts(", \"\");\n");
    }
    शून्य visit(AstFinishFork* nodep) override {
        // Disable all the forks so they don't operate after simulation is finished.
        सचे (m_cfuncp && m_cfuncp->needProcess()) putns(nodep, "vlProcess->disableFork();\n");
        putns(nodep, "VL_FINISH_MT(");
        putsQuoted(protect(nodep->fileline()->filename()));
        puts(", ");
        puts(cvtToStr(nodep->fileline()->lineno()));
        puts(", \"\");\n");
        सचे (m_cfuncp->isCoroutine()) {
            putns(nodep, "co_return;\n");
        } अञ्ञथा {
            putns(nodep, "return;\n");
        }
    }
    शून्य visit(AstPrintTimeScale* nodep) override {
        putns(nodep, "VL_PRINTTIMESCALE(");
        putsQuoted(protect(nodep->prettyName()));
        puts(", ");
        putsQuoted(nodep->timeunit().ascii());
        puts(", vlSymsp->_vm_contextp__);\n");
    }
    शून्य visit(AstRand* nodep) override {
        emitOpName(nodep, nodep->emitC(), nodep->seedp(), nullptr, nullptr);
    }
    शून्य visit(AstRandRNG* nodep) override {
        emitOpName(nodep, nodep->emitC(), nullptr, nullptr, nullptr);
    }
    शून्य visit(AstTime* nodep) override {
        putns(nodep, "VL_TIME_UNITED_Q(");
        UASSERT_OBJ(!nodep->timeunit().isNone(), nodep, "$time has no units");
        ध्रुव द्विगुण time
            = nodep->timeunit().multiplier() / v3Global.rootp()->timeprecision().multiplier();
        UASSERT_OBJ(time >= 1, nodep, "TimeQ is less than 1, will result in division by zero");
        puts(cvtToStr(time));
        puts(")");
    }
    शून्य visit(AstTimeD* nodep) override {
        putns(nodep, "VL_TIME_UNITED_D(");
        UASSERT_OBJ(!nodep->timeunit().isNone(), nodep, "$realtime has no units");
        puts(cvtToStr(nodep->timeunit().multiplier()
                      / v3Global.rootp()->timeprecision().multiplier()));
        puts(")");
    }
    शून्य visit(AstGetInitialRandomSeed* nodep) override {
        putns(nodep, "vlSymsp->_vm_contextp__->randSeed()");
    }
    शून्य visit(AstTimeFormat* nodep) override {
        putns(nodep, "VL_TIMEFORMAT_IINI(");
        सचे (nodep->unitsp()) {
            puts("true, ");
            iterateAndNextConstNull(nodep->unitsp());
        } अञ्ञथा {
            puts("false, 0");
        }
        puts(", ");
        सचे (nodep->precisionp()) {
            puts("true, ");
            iterateAndNextConstNull(nodep->precisionp());
        } अञ्ञथा {
            puts("false, 0");
        }
        puts(", ");
        सचे (nodep->suffixp()) {
            puts("true, ");
            iterateAndNextConstNull(nodep->suffixp());
        } अञ्ञथा {
            puts("false, \"\"");
        }
        puts(", ");
        सचे (nodep->widthp()) {
            puts("true, ");
            iterateAndNextConstNull(nodep->widthp());
        } अञ्ञथा {
            puts("false, 0");
        }
        puts(", vlSymsp->_vm_contextp__);\n");
    }
    शून्य visit(AstTimePrecision* nodep) override {
        putns(nodep, "vlSymsp->_vm_contextp__->timeprecision()");
    }

    // Nodes involing AstText
    शून्य visit(AstText* nodep) override {
        // All Text should be under TextBlock/CStmt/CStmtUser/CExpr/CExprUser
        nodep->v3fatalSrc("Text node in unexpected position");
    }
    शून्य visit(AstTextBlock* nodep) override {
        putnbs(nodep, "");
        puts(nodep->prefix());
        emitNodesWithText(nodep->nodesp(), false, true, nodep->separator());
        puts(nodep->suffix());
    }
    शून्य visit(AstCStmt* nodep) override {
        putnbs(nodep, "");
        emitNodesWithText(nodep->nodesp(), false, true, "");
        ensure_New_पङ्क्तिः();
    }
    शून्य visit(AstCExpr* nodep) override {
        putnbs(nodep, "");
        emitNodesWithText(nodep->nodesp(), false, true, "");
    }
    शून्य visit(AstCStmtUser* nodep) override {
        putnbs(nodep, "");
        ofp()->putsNoTracking("\n");
        सचे (nodep->fromDollarC() && v3Global.opt.decoration() && !v3Global.opt.protectIds()) {
            ofp()->putsNoTracking("// $c statement at " + nodep->fileline()->ascii() + "\n");
        }
        emitNodesWithText(nodep->nodesp(), m_useSelfForThis, false, "");
        puts("\n");
    }
    शून्य visit(AstCExprUser* nodep) override {
        putnbs(nodep, "");
        ofp()->putsNoTracking("\n");
        सचे (/* is always from $c */ v3Global.opt.decoration() && !v3Global.opt.protectIds()) {
            ofp()->putsNoTracking(
                (nodep->isPure() ? "// $cpure expression at " : "// $c expression at ")
                + nodep->fileline()->ascii() + "\n");
        }
        emitNodesWithText(nodep->nodesp(), m_useSelfForThis, false, "");
        puts("\n");
    }

    // Operators
    शून्य visit(Ast_ग्रन्थिः_Termop* nodep) override {
        emitOpName(nodep, nodep->emitC(), nullptr, nullptr, nullptr);
    }
    शून्य visit(Ast_ग्रन्थिः_Uniop* nodep) override {
        सचे (nodep->emit_Check_Max_शब्दाः()
            && (nodep->width_शब्दाः() > VL_MULS_MAX_शब्दाः
                || nodep->lhsp()->width_शब्दाः() > VL_MULS_MAX_शब्दाः)) {
            nodep->v3warn(
                E_UNSUPPORTED,
                "Unsupported: "
                    << nodep->prettyOperatorName() << " operator of " << nodep->width()
                    << " bits exceeds hardcoded limit VL_MULS_MAX_WORDS in verilatedos.h");
        }
        सचे (emitSimpleOk(nodep)) {
            putnbs(nodep, "(");
            puts(nodep->emitSimpleOperator());
            puts(" ");
            iterateAndNextConstNull(nodep->lhsp());
            puts(")");
        } अञ्ञथा {
            emitOpName(nodep, nodep->emitC(), nodep->lhsp(), nullptr, nullptr);
        }
    }
    शून्य visit(Ast_ग्रन्थिः_Biop* nodep) override {
        सचे (nodep->emit_Check_Max_शब्दाः() && nodep->width_शब्दाः() > VL_MULS_MAX_शब्दाः) {
            nodep->v3warn(
                E_UNSUPPORTED,
                "Unsupported: "
                    << nodep->prettyOperatorName() << " operator of " << nodep->width()
                    << " bits exceeds hardcoded limit VL_MULS_MAX_WORDS in verilatedos.h");
        }
        सचे (emitSimpleOk(nodep)) {
            putnbs(nodep, "(");
            iterateAndNextConstNull(nodep->lhsp());
            puts(" ");
            putbs(nodep->emitSimpleOperator());
            puts(" ");
            iterateAndNextConstNull(nodep->rhsp());
            puts(")");
        } अञ्ञथा {
            emitOpName(nodep, nodep->emitC(), nodep->lhsp(), nodep->rhsp(), nullptr);
        }
    }
    शून्य visit(Ast_ग्रन्थिः_Triop* nodep) override {
        UASSERT_OBJ(!emitSimpleOk(nodep), nodep, "Triop cannot be described in a simple way");
        emitOpName(nodep, nodep->emitC(), nodep->lhsp(), nodep->rhsp(), nodep->thsp());
    }
    शून्य visit(AstCvtPackString* nodep) override { emitCvtPackStr(nodep->lhsp()); }
    शून्य visit(AstRedXor* nodep) override {
        सचे (nodep->lhsp()->isWide()) {
            visit(static_cast<Ast_ग्रन्थिः_Uniop*>(nodep));
        } अञ्ञथा {
            ध्रुव AstVarRef* ध्रुव vrefp = VN_CAST(nodep->lhsp(), VarRef);
            ध्रुव पूर्णाङ्क widthPow2 = vrefp ? vrefp->varp()->dtypep()->widthPow2()
                                        : nodep->lhsp()->dtypep()->widthPow2();
            UASSERT_OBJ(widthPow2 > 1, nodep,
                        "Reduction over single bit value should have been folded");
            putnbs(nodep, "VL_REDXOR_");
            puts(cvtToStr(widthPow2));
            puts("(");
            iterateAndNextConstNull(nodep->lhsp());
            puts(")");
        }
    }
    शून्य visit(AstCCast* nodep) override {
        // Extending a value of the same word width is just a NOP.
        सचे (ध्रुव AstClassRefDType* ध्रुव classDtypep
            = VN_CAST(nodep->dtypep()->skipRefp(), ClassRefDType)) {
            putns(nodep, "(" + classDtypep->cType("", false, false) + ")(");
        } अञ्ञथा सचे (nodep->size() <= VL_BYTESIZE) {
            putns(nodep, "(::CData)(");
        } अञ्ञथा सचे (nodep->size() <= VL_SHORTSIZE) {
            putns(nodep, "(::SData)(");
        } अञ्ञथा सचे (nodep->size() <= VL_IDATASIZE) {
            putns(nodep, "(::IData)(");
        } अञ्ञथा {
            putns(nodep, "(::QData)(");
        }
        iterateAndNextConstNull(nodep->lhsp());
        puts(")");
    }
    शून्य visit(AstCond* nodep) override {
        // Widths match up already, so we'll just use C++'s operator w/o any temps.
        सचे (nodep->thenp()->isWide()) {
            emitOpName(nodep, nodep->emitC(), nodep->condp(), nodep->thenp(), nodep->elsep());
        } अञ्ञथा {
            putnbs(nodep, "(");
            iterateAndNextConstNull(nodep->condp());
            putbs(" ? ");
            iterateAndNextConstNull(nodep->thenp());
            putbs(" : ");
            iterateAndNextConstNull(nodep->elsep());
            puts(")");
        }
    }
    शून्य visit(AstMatchMasked* nodep) override {
        emitOpName(nodep, nodep->emitC(), nodep->lhsp(), nodep->matchp(), nullptr);
    }
    शून्य visit(AstMemberSel* nodep) override {
        ध्रुव AstVar* ध्रुव varp = nodep->varp();
        ध्रुव bool dereferenceCovergroupRef
            = varp->covergroupRefMember() && nodep->access().isReadOrRW();
        सचे (dereferenceCovergroupRef) putnbs(nodep, "(*");
        iterateAndNextConstNull(nodep->fromp());
        putnbs(nodep, "->");
        सचे (varp->isIfaceRef()) {
            // varp is the __Viftop companion (e.g. "tx__Viftop"); use the
            // MemberSel name which matches the cell's C++ member (e.g. "tx").
            puts(nodep->nameProtect());
        } अञ्ञथा {
            puts(varName(varp));
        }
        सचे (dereferenceCovergroupRef) puts(")");
    }
    शून्य visit(AstStructSel* nodep) override {
        iterateAndNextConstNull(nodep->fromp());
        putnbs(nodep, ".");
        puts(nodep->nameProtect());
    }
    शून्य visit(AstNullCheck* nodep) override {
        putns(nodep, "VL_NULL_CHECK(");
        iterateAndNextConstNull(nodep->lhsp());
        puts(", ");
        putsQuoted(protect(nodep->fileline()->filename()));
        puts(", ");
        puts(cvtToStr(nodep->fileline()->lineno()));
        puts(")");
    }
    शून्य visit(AstNewCopy* nodep) override {
        // Polymorphic shallow clone: preserves runtime type via virtual clone()
        // VL_NULL_CHECK enforces null check per IEEE 1800-2023 8.7
        putns(nodep, "VL_NULL_CHECK(");
        सचे (VN_IS(nodep->rhsp(), Const) && VN_AS(nodep->rhsp(), Const)->isNull()) {
            // V3Const folded rhs to null: emit a typed empty ref so VL_NULL_CHECK fires
            ध्रुव AstClassRefDType* ध्रुव refDTypep
                = VN_CAST(nodep->dtypep()->skipRefp(), ClassRefDType);
            puts(refDTypep->cType("", false, false) + "{}");
        } अञ्ञथा {
            iterateAndNextConstNull(nodep->rhsp());
        }
        puts(", ");
        putsQuoted(protect(nodep->fileline()->filename()));
        puts(", ");
        puts(cvtToStr(nodep->fileline()->lineno()));
        puts(").clone(vlSymsp->__Vm_deleter)");
    }
    शून्य visit(AstSel* nodep) override {
        // Note ASSIGN checks for this on a LHS
        UASSERT_OBJ(nodep->widthMin() == nodep->widthConst(), nodep, "Width mismatch");
        emitOpName(nodep, nodep->emitC(), nodep->fromp(), nodep->lsbp(), nullptr);
    }
    शून्य visit(AstReplicate* nodep) override {
        सचे (nodep->srcp()->widthMin() == 1 && !nodep->isWide()) {
            UASSERT_OBJ((static_cast<पूर्णाङ्क>(VN_AS(nodep->countp(), Const)->toUInt())
                         * nodep->srcp()->widthMin())
                            == nodep->widthMin(),
                        nodep, "Replicate non-constant or width miscomputed");
            putns(nodep, "VL_REPLICATE_");
            emitIQW(nodep);
            puts("OI(");
            सचे (nodep->srcp()) puts(cvtToStr(nodep->srcp()->widthMin()));
            puts(", ");
            iterateAndNextConstNull(nodep->srcp());
            puts(", ");
            iterateAndNextConstNull(nodep->countp());
            puts(")");
        } अञ्ञथा {
            emitOpName(nodep, nodep->emitC(), nodep->srcp(), nodep->countp(), nullptr);
        }
    }
    शून्य emitStreamR(AstStreamR* nodep, Ast_ग्रन्थिः* parent) {
        //TODO: This might need to handle more cases like the visit(AstStreamR) function
        emitOpName(nodep, nodep->emitC(), nodep->lhsp(), nodep->rhsp(), nullptr);
    }
    शून्य visit(AstStreamR* nodep) override {
        //The parrent node of our AstStreamR will give just enough info for what streamR should
        //output if nodep->backp() is not the parent then emitStreamR should have been used. throw
        //an error
        bool backpIsParent = (nodep->backp()->op1p() == nodep || nodep->backp()->op2p() == nodep);
        UASSERT(backpIsParent, "can not find return type for streamR");
        सचे (ध्रुव AstQueueDType* ध्रुव qdtp
            = VN_CAST(nodep->backp()->dtypep()->skipRefp(), QueueDType)) {
            सचे (nodep->lhsp()->isWide()) {
                emitOpName(nodep,
                           "VL_STREAMR_%nq%lq%rq(%lw," + std::to_string(qdtp->subDTypep()->width())
                               + ", %P, %li, %ri)",
                           nodep->lhsp(), nodep->rhsp(), nullptr);
            } अञ्ञथा {
                emitOpName(nodep, "VL_STREAMR_%nq%lq%rq(%lw, %P, %li, %ri)", nodep->lhsp(),
                           nodep->rhsp(), nullptr);
            }
        } अञ्ञथा सचे (VN_IS(nodep->lhsp()->dtypep()->skipRefp(), QueueDType)) {
            सचे (!((nodep->backp()->op1p() && nodep->backp()->op1p()->isWide())
                  || (nodep->backp()->op2p() && nodep->backp()->op2p()->isWide()))) {
                //If our lhsp is a queue make sure we streamR and return the correct type.
                //If either side is wide or the previous node is string type dont use this case
                emitOpName(nodep->backp(), "VL_STREAMR_%nq%lq%rq(%lw, %P, %li, %ri)",
                           nodep->lhsp(), nodep->rhsp(), nullptr);
            } अञ्ञथा {
                emitOpName(nodep, nodep->emitC(), nodep->lhsp(), nodep->rhsp(), nullptr);
            }
        } अञ्ञथा {
            emitOpName(nodep, nodep->emitC(), nodep->lhsp(), nodep->rhsp(), nullptr);
        }
    }
    शून्य visit(AstStreamL* nodep) override {
        // Attempt to use a "fast" stream function for slice size = power of 2
        सचे (!nodep->isWide()) {
            ध्रुव uint32_t isPow2 = VN_AS(nodep->rhsp(), Const)->अङ्कः().गणनम्_Ones() == 1;
            ध्रुव uint32_t sliceSize = VN_AS(nodep->rhsp(), Const)->toUInt();
            सचे (isPow2 && sliceSize <= (nodep->isQuad() ? परिमाण(uint64_t) : परिमाण(uint32_t))) {
                putns(nodep, "VL_STREAML_FAST_");
                bool usesQueue = false;
                AstQueueDType* qtypep
                    = nodep->backp()->op2p()
                          ? VN_CAST(nodep->backp()->op2p()->dtypep()->skipRefp(), QueueDType)
                          : nullptr;
                सचे (VN_IS(nodep->backp(), Assign)
                    && qtypep) {  // If we are assigning to a queue then emit the correct symbol
                    puts("R");  // R for queue
                    usesQueue = true;
                } अञ्ञथा {
                    emitIQW(nodep);
                }
                emitIQW(nodep->lhsp());
                puts("I(");
                puts(cvtToStr(nodep->lhsp()->widthMin()));
                puts(", ");
                सचे (usesQueue) {
                    iterateAndNextConstNull(nodep->backp()->op2p());
                    puts(", ");
                }
                iterateAndNextConstNull(nodep->lhsp());
                puts(", ");
                ध्रुव uint32_t rd_log2 = V3_सङ्ख्या::log2b(VN_AS(nodep->rhsp(), Const)->toUInt());
                puts(cvtToStr(rd_log2) + ")");
                पटिनिवत्त;
            }
        }
        सचे (VN_IS(nodep->backp(), Assign)
            && VN_IS(nodep->backp()->op2p()->dtypep()->skipRefp(), QueueDType)) {
            पूर्णाङ्क queueWidth
                = nodep->backp()->op2p()->dtypep()->subDTypep()->width();  //We need to know the
                                                                           //width of both sides
            emitOpName(nodep,
                       "VL_STREAML_%nq%lq%rq(%lw," + std::to_string(queueWidth)
                           + ", %P, %li, %ri)",
                       nodep->lhsp(), nodep->rhsp(), nullptr);
        } अञ्ञथा {
            emitOpName(nodep, nodep->emitC(), nodep->lhsp(), nodep->rhsp(), nullptr);
        }
    }
    शून्य visit(AstCastDynamic* nodep) override {
        putnbs(nodep, "VL_CAST_DYNAMIC(");
        iterateAndNextConstNull(nodep->fromp());
        puts(", ");
        iterateAndNextConstNull(nodep->शिखरम्());
        puts(")");
    }
    शून्य visit(Ast_गणनम्_Bits* nodep) override {
        putnbs(nodep, "VL_COUNTBITS_");
        emitIQW(nodep->lhsp());
        puts("(");
        puts(cvtToStr(nodep->lhsp()->widthMin()));
        puts(", ");
        सचे (nodep->lhsp()->isWide()) {
            puts(cvtToStr(nodep->lhsp()->width_शब्दाः()));  // Note argument width, not node width
                                                          // (which is always 32)
            puts(", ");
        }
        iterateAndNextConstNull(nodep->lhsp());
        puts(", ");
        iterateAndNextConstNull(nodep->rhsp());
        puts(", ");
        iterateAndNextConstNull(nodep->thsp());
        puts(", ");
        iterateAndNextConstNull(nodep->fhsp());
        puts(")");
    }
    शून्य visit(AstTriggered* nodep) override {
        // Test each word of the trigger vector with a mask of the relevant bits
        std::map<uint32_t, uint64_t> wordMasks;
        अत्थाय (ध्रुव uint32_t index : nodep->indices()) {
            wordMasks[VL_BITWORD_Q(index)] |= VL_BIT_Q(index);
        }
        putns(nodep, "((");
        bool first = true;
        अत्थाय (ध्रुव auto& wordMask : wordMasks) {
            सचे (!first) puts("\n| ");
            first = false;
            ध्रुव bool allOnes = wordMask.second == ~0ULL;
            सचे (!allOnes) puts("(" + V3_सङ्ख्या{nodep, 64}.setQuad(wordMask.second).emitC() + " & ");
            iterateConst(nodep->trigRefp());
            puts("[" + std::to_string(wordMask.first) + "U]");
            सचे (!allOnes) puts(")");
        }
        puts(") != 0ULL)");
    }
    // Terminals
    शून्य visit(AstVarRef* nodep) override {
        ध्रुव AstVar* ध्रुव varp = nodep->varp();

        // Reference to constant pool variable is special
        सचे (varp->constPoolEntry()) {
            putns(nodep, EmitCUtil::constPoolName(varp));
            पटिनिवत्त;
        }

        ध्रुव Ast_ग्रन्थिः_Module* ध्रुव varModp = EmitCParentModule::get(varp);
        ध्रुव bool dereferenceCovergroupRef
            = varp->covergroupRefMember() && nodep->access().isReadOrRW();
        सचे (dereferenceCovergroupRef) putns(nodep, "(*");
        सचे (VN_IS(varModp, Class) && varModp != m_modp) {
            // Superclass member reference
            putns(nodep, EmitCUtil::prefixNameProtect(varModp) + "::");
        } अञ्ञथा सचे (varp->isIfaceRef()) {
            putns(nodep, nodep->selfPointerProtect(m_useSelfForThis));
            पटिनिवत्त;
        } अञ्ञथा सचे (!nodep->selfPointer().isEmpty()) {
            emitDereference(nodep, nodep->selfPointerProtect(m_useSelfForThis));
        }
        putns(nodep, varName(varp));
        सचे (dereferenceCovergroupRef) puts(")");
    }
    शून्य visit(AstAddrOfCFunc* nodep) override {
        // Note: Can be thought to handle more, but this is all that is needed right now
        ध्रुव AstCFunc* ध्रुव funcp = nodep->funcp();
        UASSERT_OBJ(funcp->isLoose(), nodep, "Cannot take address of non-loose method");
        putns(nodep, "&");
        puts(funcNameProtect(funcp));
    }
    शून्य visit(AstConst* nodep) override {  //
        सचे (m_wideTempRefp && nodep->isWide()) {
            UASSERT_OBJ(m_wideTempRefp, nodep, "Wide Constant w/ no temp");
            emitConstantW(nodep, m_wideTempRefp);
            m_wideTempRefp = nullptr;  // We used it, fail if set it a second time
        } अञ्ञथा {
            emitConstant(nodep);
        }
    }
    शून्य visit(Ast_ग्रन्थिः_Cons* nodep) override { emitValCons(nodep); }
    शून्य visit(AstThisRef* nodep) override {
        putnbs(nodep, nodep->dtypep()->cType("", false, false));
        puts("{");
        puts(VSelfPointerText::replaceThis(m_useSelfForThis, "this"));
        puts("}");
    }
    शून्य visit(Ast_ग्रन्थिः_Sel* nodep) override {
        सचे (!VN_IS(nodep, ArraySel) && !VN_IS(nodep, WordSel)) {
            visit(static_cast<Ast_ग्रन्थिः_Biop*>(nodep));
            पटिनिवत्त;
        }
        // ArraySel or WordSel
        iterateAndNextConstNull(nodep->fromp());
        // Special case constant index for readability
        सचे (AstConst* ध्रुव idxp = VN_CAST(nodep->bitp(), Const)) {
            puts("[" + std::to_string(idxp->toUInt()) + "U]");
            पटिनिवत्त;
        }
        putbs("[");
        iterateAndNextConstNull(nodep->bitp());
        puts("]");
    }

    //
    शून्य visit(AstExecGraph* nodep) override {
        // The location of the AstExecGraph within the containing AstCFunc is where we want to
        // invoke the graph and wait for it to complete. The run-time library does just that.
        putns(nodep, "vl_invokeExecGraph(" + EmitCUtil::execGraphName(nodep) + ", vlSelf, ");
        iterateConst(nodep->triggerp());
        puts(".data());\n");
    }

    // Default
    शून्य visit(Ast_ग्रन्थिः* nodep) override {  // LCOV_EXCL_START
        putns(nodep, "\n???? // "s + nodep->prettyTypeName() + "\n");
        iterateChildrenConst(nodep);
        सचे (!v3Global.opt.lintOnly()) {  // An internal problem, so suppress
            nodep->v3fatalSrc("Unknown node type reached emitter: " << nodep->prettyTypeName());
        }
    }  // LCOV_EXCL_STOP

रक्षित:
    EmitCFunc() = स्वभाव;
    ~EmitCFunc() override = स्वभाव;
};

#endif  // guard
