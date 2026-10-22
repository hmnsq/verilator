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

#அந்தர்பாவ "config_build.h"
#அந்தர்பாவ "verilatedos.h"

#அந்தர்பாவ "V3EmitCConstInit.h"
#அந்தர்பாவ "V3Global.h"
#அந்தர்பாவ "V3MemberMap.h"

#அந்தர்பாவ <algorithm>
#அந்தர்பாவ <map>
#அந்தர்பாவ <unordered_map>
#அந்தர்பாவ <unordered_set>
#அந்தர்பாவ <vector>

// Number of VL_CONST_W_*X's in verilated.h (IE VL_CONST_W_8X is last)
constexpr முழு EMITC_இலக்கம்_CONSTW = 8;

//######################################################################
// Emit lazy forward declarations

class EmitCLazyDecls final : பொது VNVisitorConst {
    // NODE STATE/TYPES
    // None allowed to support threaded emitting

    // MEMBERS
    std::unordered_set<string> m_emittedManually;  // Set of names already declared manually.
    EmitCBaseVisitorConst& m_emitter;  // For access to file output
    bool m_needs_Blank_வரி = false;  // Emit blank line if any declarations were emitted (cosmetic)
    std::set<Ast_முனை*> m_emitted;  // -> in set. Already emitted decl for symbols.

    // METHODS
    bool declaredOnce(Ast_முனை* nodep) { திரும்பு m_emitted.செருகு(nodep).second; }

    வெறுமை lazyDeclare(AstCFunc* funcp) {
        // Already declared in this compilation unit
        எனில் (!declaredOnce(funcp)) திரும்பு;
        // Check if this kind of function is lazily declared
        எனில் (!(funcp->isMethod() && funcp->isLoose()) && !funcp->dpiImportPrototype()) திரும்பு;
        // Already declared manually
        எனில் (m_emittedManually.மொத்தம்(funcp->nameProtect())) திரும்பு;
        // Needs lazy declaration, emit one
        m_emitter.emitCFuncDecl(funcp, EmitCParentModule::get(funcp), funcp->dpiImportPrototype());
        m_needs_Blank_வரி = true;
    }

    வெறுமை lazyDeclareConstPoolVar(AstVar* varp) {
        எனில் (!declaredOnce(varp)) திரும்பு;  // Already declared
        மாறிலி string nameProtect = EmitCUtil::constPoolName(varp);
        m_emitter.putns(varp, "extern const ");
        m_emitter.puts(varp->dtypep()->cType(nameProtect, false, false));
        m_emitter.puts(";\n");
        m_needs_Blank_வரி = true;
    }

    // VISITORS
    வெறுமை visit(Ast_முனை_CCall* nodep) override {
        lazyDeclare(nodep->funcp());
        iterateChildrenConst(nodep);
    }

    வெறுமை visit(AstAddrOfCFunc* nodep) override {
        lazyDeclare(nodep->funcp());
        iterateChildrenConst(nodep);
    }

    வெறுமை visit(AstVarRef* nodep) override {
        AstVar* மாறிலி varp = nodep->varp();
        // Only constant pool symbols are lazy declared for now ...
        எனில் (EmitCParentModule::get(varp)->isConstPool()) lazyDeclareConstPoolVar(varp);
    }

    வெறுமை visit(AstExecGraph* nodep) override {
        எனில் (!declaredOnce(nodep)) திரும்பு;
        m_emitter.putns(nodep, "extern VlExecGraph " + EmitCUtil::execGraphName(nodep) + ";\n");
        m_needs_Blank_வரி = true;
    }

    வெறுமை visit(Ast_முனை* nodep) override { iterateChildrenConst(nodep); }

பொது:
    explicit EmitCLazyDecls(EmitCBaseVisitorConst& emitter)
        : m_emitter{emitter} {}
    வெறுமை emit(Ast_முனை* nodep) {
        m_needs_Blank_வரி = false;
        iterateChildrenConst(nodep);
        எனில் (m_needs_Blank_வரி) m_emitter.puts("\n");
    }
    வெறுமை emit(மாறிலி string& prefix, மாறிலி string& name, மாறிலி string& suffix) {
        m_emittedManually.செருகு(name);
        m_emitter.ensure_New_வரி();
        m_emitter.puts(prefix);
        m_emitter.puts(name);
        m_emitter.puts(suffix);
        m_emitter.ensure_New_வரி();
    }
    வெறுமை declared(AstCFunc* nodep) { m_emitted.செருகு(nodep); }
    வெறுமை reset() { m_emitted.clear(); }
};

// ######################################################################
//  Emit statements and expressions

class EmitCFunc VL_NOT_FINAL : பொது EmitCConstInit {
    VMemberMap m_memberMap;
    AstVarRef* m_wideTempRefp = nullptr;  // Variable that _WW macros should be setting
    std::unordered_map<AstJumpBlock*, size_t> m_labelNumbers;  // Label numbers for AstJumpBlocks
    bool m_createdScopeHash = false;  // Already created a scope hash

பாதுகாக்கப்பட்ட:
    VL_DEFINE_DEBUG_FUNCTIONS;

    EmitCLazyDecls m_lazyDecls{*this};  // Visitor for emitting lazy declarations
    bool m_useSelfForThis = false;  // Replace "this" with "vlSelf"
    bool m_usevlSelfRef = false;  // Use vlSelfRef reference instead of vlSelf pointer
    மாறிலி Ast_முனை_Module* m_modp = nullptr;  // Current module being emitted
    மாறிலி AstCFunc* m_cfuncp = nullptr;  // Current function being emitted
    bool m_instantiatesOwnProcess = false;
    மாறிலி AstClassPackage* m_classOrPackage = nullptr;  // Pointer to current class or package
    string m_classOrPackageHash;  // Hash of class or package name

    bool constructorNeedsProcess(மாறிலி AstClass* மாறிலி classp) {
        மாறிலி Ast_முனை* மாறிலி newp = m_memberMap.findMember(classp, "new");
        எனில் (!newp) திரும்பு false;
        மாறிலி AstCFunc* மாறிலி ctorp = VN_CAST(newp, CFunc);
        எனில் (!ctorp) திரும்பு false;
        UASSERT_OBJ(ctorp->isConstructor(), ctorp, "`new` is not a constructor!");
        திரும்பு ctorp->needProcess();
    }

    bool constructorNeedsProcess(மாறிலி Ast_முனை_DType* மாறிலி dtypep) {
        எனில் (மாறிலி AstClassRefDType* மாறிலி crefdtypep = VN_CAST(dtypep, ClassRefDType))
            திரும்பு constructorNeedsProcess(crefdtypep->classp());
        திரும்பு false;
    }

    bool coverageUsesLocalCounter(Ast_முனை_Cover_Decl* மாறிலி declp) மாறிலி {
        // Only functions with an object receiver can address the module-local
        // coverage array. Static/package/class helper functions still use the
        // symbol-table array, as they may have vlSymsp but no vlSelf/this.
        // Also require the coverage declaration to belong to the current
        // module; cloned task bodies can increment declarations owned by a
        // different module, and those must keep using the global array.
        திரும்பு m_cfuncp && !m_cfuncp->isStatic() && !VN_IS(m_modp, Class)
               && !VN_IS(m_modp, ClassPackage)
               && EmitCParentModule::get(declp->dataDeclThisp()) == m_modp;
    }
    முழு coverage_Bin_இலக்கம்(Ast_முனை_Cover_Decl* மாறிலி declp, bool forceGlobal = false) மாறிலி {
        திரும்பு !forceGlobal && coverageUsesLocalCounter(declp)
                   ? declp->dataDeclThisp()->local_Bin_இலக்கம்()
                   : declp->dataDeclThisp()->bin_இலக்கம்();
    }
    வெறுமை putCoverageArray(Ast_முனை_Cover_Decl* மாறிலி declp, bool forceGlobal = false) {
        எனில் (!forceGlobal && coverageUsesLocalCounter(declp)) {
            // Keep counters on the module object when possible, so every
            // no-inline instance has storage available if coverage output is
            // later written with forcePerInstance.
            puts(m_cfuncp->isLoose() ? "vlSelf->__Vcoverage" : "this->__Vcoverage");
        } இல்லையெனில் {
            // Static/package/class helper functions do not have vlSelf, so
            // keep using the shared symbol-table coverage array there.
            puts("vlSymsp->__Vcoverage");
        }
    }
    வெறுமை emit_Cover_Other_Decl_செருகு(AstCoverOtherDecl* nodep, bool forceGlobal = false) {
        putns(nodep, "vlSelf->__vlCoverInsert(");  // As Declared in emitCoverageDecl
        putCoverageArray(nodep, forceGlobal);
        puts(" + ");
        puts(cvtToStr(coverage_Bin_இலக்கம்(nodep, forceGlobal)));
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
                   + VIdProtect::protect_சொற்கள்_If(nodep->hier(), nodep->protect()));
        puts(", ");
        putsQuoted(VIdProtect::protect_சொற்கள்_If(nodep->page(), nodep->protect()));
        puts(", ");
        putsQuoted(VIdProtect::protect_சொற்கள்_If(nodep->comment(), nodep->protect()));
        puts(", ");
        putsQuoted(nodep->linescov());
        puts(", ");
        putsQuoted(VIdProtect::protect_சொற்கள்_If(nodep->fsmVar(), nodep->protect()));
        puts(", ");
        putsQuoted(VIdProtect::protect_சொற்கள்_If(nodep->fsmFrom(), nodep->protect()));
        puts(", ");
        putsQuoted(VIdProtect::protect_சொற்கள்_If(nodep->fsmTo(), nodep->protect()));
        puts(", ");
        putsQuoted(VIdProtect::protect_சொற்கள்_If(nodep->fsmTag(), nodep->protect()));
        puts(");\n");
    }

பொது:
    // METHODS
    bool displayEmitHeader(Ast_முனை* nodep);
    வெறுமை display_முனை(Ast_முனை* nodep, AstSFormatF* fmtp, மாறிலி string& vformat, Ast_முனை* exprsp,
                     bool isScan);

    bool emitSimpleOk(Ast_முனை_கோவை* nodep);
    வெறுமை emitIQW(மாறிலி Ast_முனை* nodep) {
        // See "Type letters" in verilated.h
        // Other abbrevs: "C"har, "S"hort, "F"loat, "D"ouble, stri"N"g, "R"=queue, "U"npacked
        puts(nodep->dtypep()->skipRefp()->charIQWN());
    }
    வெறுமை emitRU(மாறிலி Ast_முனை* nodep) {
        மாறிலி Ast_முனை_DType* dtp = nodep->dtypep()->skipRefp();
        // See "Type letters" in verilated.h
        எனில் (VN_IS(dtp, UnpackArrayDType))
            puts("U");
        இல்லையெனில் எனில் (VN_IS(dtp, QueueDType) || VN_IS(dtp, DynArrayDType))
            puts("R");
    }
    வெறுமை emitScIQW(AstVar* nodep) {
        UASSERT_OBJ(nodep->isSc(), nodep, "emitting SystemC operator on non-SC variable");
        puts(nodep->isScBigUint()                           ? "SB"
             : (nodep->isScUint() || nodep->isScUintBool()) ? "SU"
             : nodep->isScBv()                              ? "SW"
             : nodep->isDouble()                            ? "SD"
                                                            : (nodep->isScQuad() ? "SQ" : "SI"));
    }
    வெறுமை emitDatap(மாறிலி Ast_முனை* nodep) {
        // When passing to a function with va_args the compiler doesn't
        // know need a pointer so when wide, need to look inside VlWide
        எனில் (nodep->isWide()) puts(".data()");
    }
    வெறுமை emitOpName(Ast_முனை* nodep, மாறிலி string& format, Ast_முனை* lhsp, Ast_முனை* rhsp,
                    Ast_முனை* thsp);
    வெறுமை emitCCallArgs(மாறிலி Ast_முனை_CCall* nodep, மாறிலி string& selfPointer, bool inProcess);
    வெறுமை emitDereference(Ast_முனை* nodep, மாறிலி string& pointer);
    std::string dereferenceString(மாறிலி std::string& pointer) மாறிலி;
    வெறுமை emitCvtPackStr(Ast_முனை* nodep);
    வெறுமை emitConstant(AstConst* nodep);
    வெறுமை emitConstantString(மாறிலி AstConst* nodep);
    வெறுமை emitVarReset(மாறிலி string& prefix, AstVar* varp, bool constructing);
    string emitVarResetRecurse(மாறிலி AstVar* varp, bool constructing,
                               மாறிலி string& varNameProtected, Ast_முனை_DType* dtypep, முழு depth,
                               மாறிலி string& suffix, மாறிலி Ast_முனை* valuep);
    வெறுமை emitVarResetScopeHash();
    வெறுமை emitChangeDet();
    வெறுமை putCommaIterateNext(Ast_முனை* nodep, bool comma = false) {
        க்கு (Ast_முனை* subnodep = nodep; subnodep; subnodep = subnodep->nextp()) {
            எனில் (comma) puts(", ");
            iterateConst(subnodep);
            comma = true;
        }
    }
    template <typename T>
    string optionalProcArg(மாறிலி T* மாறிலி nodep) {
        திரும்பு (nodep && constructorNeedsProcess(nodep)) ? "vlProcess, " : "";
    }
    மாறிலி AstCNew* getSuperNewCallRecursep(Ast_முனை* மாறிலி nodep) {
        // Get the super.new call
        எனில் (!nodep) திரும்பு nullptr;
        எனில் (மாறிலி AstCNew* மாறிலி cnewp = VN_CAST(nodep, CNew)) திரும்பு cnewp;
        எனில் (மாறிலி Ast_Stmt_கோவை* மாறிலி stmtp = VN_CAST(nodep, Stmt_கோவை)) {
            எனில் (மாறிலி AstCNew* மாறிலி cnewp = VN_CAST(stmtp->exprp(), CNew)) திரும்பு cnewp;
        }
        எனில் (மாறிலி AstJumpBlock* மாறிலி blockp = VN_CAST(nodep, JumpBlock)) {
            எனில் (மாறிலி AstCNew* மாறிலி cnewp = getSuperNewCallRecursep(blockp->stmtsp())) {
                திரும்பு cnewp;
            }
        }
        எனில் (மாறிலி AstCNew* மாறிலி cnewp = getSuperNewCallRecursep(nodep->nextp())) திரும்பு cnewp;
        திரும்பு nullptr;
    }

    வெறுமை emitConstantW(AstConst* nodep, AstVarRef* assigntop) {
        // For tradition and compilation speed, assign each word directly into
        // output variable instead of using '='
        putns(nodep, "");
        எனில் (nodep->இலக்கம்().isFourState()) {
            nodep->v3warn(E_UNSUPPORTED, "Unsupported: 4-state numbers in this context");
            திரும்பு;
        }

        முழு upWidth = nodep->இலக்கம்().widthToFit();
        முழு chunks = 0;
        எனில் (upWidth > EMITC_இலக்கம்_CONSTW * VL_EDATASIZE) {
            // Output e.g. 8 words in groups of e.g. 8
            chunks = (upWidth - 1) / (EMITC_இலக்கம்_CONSTW * VL_EDATASIZE);
            upWidth %= (EMITC_இலக்கம்_CONSTW * VL_EDATASIZE);
            எனில் (upWidth == 0) upWidth = (EMITC_இலக்கம்_CONSTW * VL_EDATASIZE);
        }
        {  // Upper e.g. 8 words
            எனில் (chunks) {
                putnbs(nodep, "VL_CONSTHI_W_");
                puts(cvtToStr(VL_சொற்கள்_I(upWidth)));
                puts("X(");
                puts(cvtToStr(nodep->widthMin()));
                puts(",");
                puts(cvtToStr(chunks * EMITC_இலக்கம்_CONSTW * VL_EDATASIZE));
            } இல்லையெனில் {
                putnbs(nodep, "VL_CONST_W_");
                puts(cvtToStr(VL_சொற்கள்_I(upWidth)));
                puts("X(");
                puts(cvtToStr(nodep->widthMin()));
            }
            puts(",");
            எனில் (!assigntop->selfPointer().isEmpty()) {
                emitDereference(assigntop, assigntop->selfPointerProtect(m_useSelfForThis));
            }
            puts(varName(assigntop->varp()));
            க்கு (முழு word = VL_சொற்கள்_I(upWidth) - 1; word >= 0; word--) {
                // Only 32 bits - llx + long long here just to appease CPP format warning
                ofp()->printf(",0x%08" PRIx64, static_cast<uint64_t>(nodep->இலக்கம்().edataWord(
                                                   word + chunks * EMITC_இலக்கம்_CONSTW)));
            }
            puts(")");
        }
        க்கு (chunks--; chunks >= 0; chunks--) {
            puts(";\n");
            putbs("VL_CONSTLO_W_");
            puts(cvtToStr(EMITC_இலக்கம்_CONSTW));
            puts("X(");
            puts(cvtToStr(chunks * EMITC_இலக்கம்_CONSTW * VL_EDATASIZE));
            puts(",");
            எனில் (!assigntop->selfPointer().isEmpty()) {
                emitDereference(assigntop, assigntop->selfPointerProtect(m_useSelfForThis));
            }
            puts(varName(assigntop->varp()));
            க்கு (முழு word = EMITC_இலக்கம்_CONSTW - 1; word >= 0; word--) {
                // Only 32 bits - llx + long long here just to appease CPP format warning
                ofp()->printf(",0x%08" PRIx64, static_cast<uint64_t>(nodep->இலக்கம்().edataWord(
                                                   word + chunks * EMITC_இலக்கம்_CONSTW)));
            }
            puts(")");
        }
    }

    வெறுமை emitNodesWithText(Ast_முனை* nodesp, bool useSelfForThis, bool tracking,
                           மாறிலி std::string& separator) {
        க்கு (Ast_முனை* nodep = nodesp; nodep; nodep = nodep->nextp()) {
            எனில் (மாறிலி AstText* மாறிலி textp = VN_CAST(nodep, Text)) {
                மாறிலி std::string text
                    = VSelfPointerText::replaceThis(useSelfForThis, textp->text());
                எனில் (tracking) {
                    puts(text);
                } இல்லையெனில் {
                    ofp()->putsNoTracking(text);
                }
            } இல்லையெனில் {
                iterateConst(nodep);
            }
            எனில் (nodep->nextp()) puts(separator);
        }
    }

    வெறுமை putConstructorSubinit(மாறிலி AstClass* classp, AstCFunc* cfuncp) {
        // Direct non-virtual bases in declaration order
        க்கு (மாறிலி AstClassExtends* extp = classp->extendsp(); extp;
             extp = VN_AS(extp->nextp(), ClassExtends)) {
            எனில் (extp->classp()->isInterfaceClass()) தொடர்;
            எனில் (extp->classp()->useVirtualPublic()) தொடர்;
            puts(EmitCUtil::prefixNameProtect(extp->classp()));
            puts("::init");
            எனில் (constructorNeedsProcess(extp->classp())) {
                puts("(vlProcess, vlSymsp");
            } இல்லையெனில் {
                puts("(vlSymsp");
            }
            // Handle super.new() args for the concrete parent
            எனில் (மாறிலி AstCNew* மாறிலி superNewCallp = getSuperNewCallRecursep(cfuncp->stmtsp())) {
                putCommaIterateNext(superNewCallp->argsp(), true);
            }
            puts(");\n");
            // Since SystemVerilog allows only for extending from one class, for sure there won't
            // be any more constructors to be called. Even if there are some classes which are
            // "implemented", those are interface classes which do not have constructors.
            நிறுத்து;
        }
    }

    // VISITORS
    using EmitCConstInit::visit;
    வெறுமை visit(AstCFunc* மாறிலி nodep) override {
        எனில் (nodep->emptyBody() && !nodep->isLoose()) திரும்பு;
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
        எனில் (nodep->ifdef() != "") putns(nodep, "#ifdef " + nodep->ifdef() + "\n");
        emitCFuncHeader(nodep, m_modp, /* withScope: */ true);

        puts(" {\n");
        எனில் (nodep->isConstructor()) {
            மாறிலி AstClass* மாறிலி classp = VN_CAST(nodep->scopep()->modp(), Class);
            எனில் (classp && classp->extendsp()) putConstructorSubinit(classp, nodep);
        }

        // "+" in the debug indicates a print from the model
        puts("VL_DEBUG_IF(VL_DBG_MSGF(\"+  ");
        க்கு (முழு i = 0; i < m_modp->level(); ++i) puts("  ");
        puts(EmitCUtil::prefixNameProtect(m_modp));
        puts(nodep->isLoose() ? "__" : "::");
        puts(nodep->nameProtect() + "\\n\"); );\n");

        எனில் (nodep->isLoose()) {
            m_lazyDecls.declared(nodep);  // Defined here, so no longer needs declaration
            எனில் (!nodep->isStatic()) {  // Standard prologue
                m_useSelfForThis = true;
                எனில் (nodep->voidSelf()) {
                    மாறிலி string className = EmitCUtil::prefixNameProtect(m_modp);
                    puts(className + "* const vlSelf = static_cast<" + className
                         + "*>(vlVoidSelf);\n");
                }
                எனில் (!VN_IS(m_modp, Class)) {
                    puts(EmitCUtil::symClassAssign());  // Uses vlSelf
                } இல்லையெனில் {
                    puts("(void)vlSelf;  // Prevent unused variable warning\n");
                }
            }
        }

        // Instantiate a process class if it's going to be needed somewhere later
        nodep->forall([&](மாறிலி Ast_முனை_CCall* ccallp) -> bool {
            எனில் (ccallp->funcp()->needProcess()
                && (ccallp->funcp()->isCoroutine() == VN_IS(ccallp->backp(), CAwait))) {
                எனில் (!nodep->needProcess() && !m_instantiatesOwnProcess) {
                    m_instantiatesOwnProcess = true;
                    திரும்பு false;
                }
            }
            திரும்பு true;
        });
        எனில் (m_instantiatesOwnProcess) {
            AstCStmt* மாறிலி vlprocp = new AstCStmt{nodep->fileline()};
            vlprocp->add("VlProcessRef vlProcess = std::make_shared<VlProcess>();\n");
            vlprocp->add("VlProcess::currentp(vlProcess.get());");
            nodep->prependStmtsp(vlprocp);
        } இல்லையெனில் எனில் (nodep->needProcess() && nodep->stmtsp()) {
            // Set current process so VlRNG() constructors in this function seed from it
            AstCStmt* மாறிலி setProcessp = new AstCStmt{nodep->fileline()};
            setProcessp->add("VlProcess::currentp(vlProcess.get());");
            nodep->prependStmtsp(setProcessp);
        }

        க்கு (Ast_முனை* subnodep = nodep->argsp(); subnodep; subnodep = subnodep->nextp()) {
            எனில் (AstVar* மாறிலி varp = VN_CAST(subnodep, Var)) {
                எனில் (varp->isFuncReturn()) emitVarDecl(varp);
            }
        }

        எனில் (m_useSelfForThis) {
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

        எனில் (nodep->varsp()) {
            putsDecoration(nodep, "// Locals\n");
            iterateAndNextConstNull(nodep->varsp());
        }

        எனில் (nodep->stmtsp()) {
            putsDecoration(nodep, "// Body\n");
            iterateAndNextConstNull(nodep->stmtsp());
        }

        m_usevlSelfRef = false;

        எனில் (nodep->isCoroutine()) {
            // Sometimes coroutines don't have co_awaits,
            // so emit a co_return at the end to avoid compile errors.
            puts("co_return;\n");
        }

        puts("}\n");
        எனில் (nodep->ifdef() != "") puts("#endif  // " + nodep->ifdef() + "\n");
    }

    வெறுமை visit(AstVar* nodep) override {
        UASSERT_OBJ(m_cfuncp, nodep, "Cannot emit non-local variable");
        emitVarDecl(nodep);
    }

    வெறுமை visit(AstCvtArrayToArray* nodep) override {
        எனில் (nodep->reverse()) {
            puts("VL_REVCLONE_Q(");
        } இல்லையெனில் {
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

    வெறுமை visit(AstCvtArrayToPacked* nodep) override {
        Ast_முனை_DType* மாறிலி fromDtp = nodep->fromp()->dtypep()->skipRefp();
        Ast_முனை_DType* மாறிலி elemDtp = fromDtp->subDTypep()->skipRefp();
        மாறிலி bool descending = [&]() {
            எனில் (மாறிலி AstUnpackArrayDType* மாறிலி unpackDtp = VN_CAST(fromDtp, UnpackArrayDType))
                திரும்பு !unpackDtp->declRange().ascending();
            திரும்பு false;
        }();
        puts("VL_PACK_");
        emitIQW(nodep);
        puts("_");
        emitRU(fromDtp);
        emitIQW(elemDtp);
        எனில் (descending) {
            // Wrap source in VL_PACK_REVERSED so VL_PACK sees ascending order
            emitOpName(nodep, "(%nw, %rw, %P, VL_PACK_REVERSED(%li))", nodep->fromp(), elemDtp,
                       nullptr);
        } இல்லையெனில் {
            emitOpName(nodep, "(%nw, %rw, %P, %li)", nodep->fromp(), elemDtp, nullptr);
        }
    }

    வெறுமை visit(AstCvtUnpackToQueue* nodep) override {
        Ast_முனை_DType* மாறிலி elemDTypep = nodep->fromp()->dtypep()->subDTypep();
        emitOpName(nodep, nodep->emitC(), nodep->fromp(), elemDTypep, nullptr);
    }
    வெறுமை visit(AstCvtUnpackToUnpack* nodep) override {
        // Whether each dimension is reversed, the first for the outermost, up to the last that is
        std::vector<bool> reverses;
        மாறிலி AstUnpackArrayDType* உச்சி = VN_AS(nodep->dtypep()->skipRefp(), UnpackArrayDType);
        மாறிலி AstUnpackArrayDType* fromp
            = VN_AS(nodep->fromp()->dtypep()->skipRefp(), UnpackArrayDType);
        போது (உச்சி) {
            reverses.தள்ளு_back(உச்சி->declRange().ascending() != fromp->declRange().ascending());
            உச்சி = VN_CAST(உச்சி->subDTypep()->skipRefp(), UnpackArrayDType);
            fromp = VN_CAST(fromp->subDTypep()->skipRefp(), UnpackArrayDType);
        }
        போது (!reverses.back()) reverses.எடு_back();
        putns(nodep, "VL_CVT_UNPACK_TO_UNPACK<");
        க்கு (size_t i = 0; i < reverses.size(); ++i) {
            எனில் (i) puts(", ");
            puts(reverses[i] ? "1" : "0");
        }
        puts(">(");
        iterateAndNextConstNull(nodep->fromp());
        puts(")");
    }

    வெறுமை visit(Ast_முனை_Assign* nodep) override {
        எனில் (AstCReset* மாறிலி resetp = VN_CAST(nodep->rhsp(), CReset)) {
            // TODO get rid of emitVarReset and instead let AstNodeAssign understand how to init
            // anything
            Ast_முனை* fromp = nodep->lhsp();
            // Fork needs to use a member select.  Nothing else should be possible before VarRef.
            எனில் (AstMemberSel* மாறிலி sfromp = VN_CAST(fromp, MemberSel)) {
                // Fork-DynScope generated pointer to previously automatic variable
                AstVar* மாறிலி memberVarp = sfromp->varp();
                fromp = sfromp->fromp();
                எனில் (AstNullCheck* மாறிலி sfromp = VN_CAST(fromp, NullCheck)) fromp = sfromp->lhsp();
                Ast_முனை_Var_Ref* மாறிலி fromVarRefp = VN_AS(fromp, முனை_Var_Ref);
                emitVarReset(
                    ("VL_NULL_CHECK("s
                     + (fromVarRefp->selfPointer().isEmpty()
                            ? ""
                            : dereferenceString(fromVarRefp->selfPointerProtect(m_useSelfForThis)))
                     + varName(fromVarRefp->varp()) + ", \""
                     + V3OutFormatter::quoteNameControls(protect(nodep->fileline()->filename()))
                     + "\", " + std::to_string(nodep->fileline()->lineno()) + ")->"),
                    memberVarp, resetp->constructing());
            } இல்லையெனில் {
                Ast_முனை_Var_Ref* மாறிலி fromVarRefp = VN_AS(fromp, முனை_Var_Ref);
                AstVar* மாறிலி varp = fromVarRefp->varp();
                மாறிலி string prefix
                    = fromVarRefp->selfPointer().isEmpty()
                          ? ""
                          : dereferenceString(
                                VN_AS(fromp, முனை_Var_Ref)->selfPointerProtect(m_useSelfForThis));
                emitVarReset(prefix, varp, resetp->constructing());
            }
            திரும்பு;
        }
        bool paren = true;
        bool decind = false;
        bool rhs = true;
        bool reverseUnpack = false;  // Set for descending CvtPackedToArray
        மாறிலி AstUnpackArrayDType* மாறிலி unpackDtp
            = VN_CAST(nodep->dtypep()->skipRefp(), UnpackArrayDType);
        எனில் (AstSel* மாறிலி selp = VN_CAST(nodep->lhsp(), Sel)) {
            UASSERT_OBJ(selp->widthMin() == selp->widthConst(), selp, "Width mismatch");
            எனில் (selp->widthMin() == 1) {
                putnbs(nodep, "VL_ASSIGNBIT_");
                emitIQW(selp->fromp());
                எனில் (nodep->rhsp()->isAllOnesV()) {
                    puts("O(");
                    rhs = false;
                } இல்லையெனில் {
                    puts("I(");
                }
                iterateAndNextConstNull(selp->lsbp());
                puts(", ");
                iterateAndNextConstNull(selp->fromp());
                எனில் (rhs) puts(", ");
            } இல்லையெனில் {
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
        } இல்லையெனில் எனில் (மாறிலி AstGetcRefN* மாறிலி selp = VN_CAST(nodep->lhsp(), GetcRefN)) {
            iterateAndNextConstNull(selp->lhsp());
            puts(" = ");
            putnbs(selp, "VL_PUTC_N(");
            iterateAndNextConstNull(selp->lhsp());
            puts(", ");
            iterateAndNextConstNull(selp->rhsp());
            puts(", ");
        } இல்லையெனில் எனில் (AstVar* மாறிலி varp = AstVar::scVarRecurse(nodep->lhsp())) {
            putnbs(varp, "VL_ASSIGN_");  // Set a systemC variable
            emitScIQW(varp);
            emitIQW(nodep);
            puts("(");
            puts(cvtToStr(nodep->widthMin()) + ", ");
            iterateAndNextConstNull(nodep->lhsp());
            puts(", ");
        } இல்லையெனில் எனில் (AstVar* மாறிலி varp = AstVar::scVarRecurse(nodep->rhsp())) {
            putnbs(varp, "VL_ASSIGN_");  // Get a systemC variable
            emitIQW(nodep);
            emitScIQW(varp);
            puts("(");
            puts(cvtToStr(nodep->widthMin()) + ", ");
            iterateAndNextConstNull(nodep->lhsp());
            puts(", ");
        } இல்லையெனில் எனில் (மாறிலி AstCvtPackedToArray* மாறிலி castp
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
            எனில் (unpackDtp && !unpackDtp->declRange().ascending()) reverseUnpack = true;
        } இல்லையெனில் எனில் (மாறிலி AstCvtArrayToArray* மாறிலி castp
                   = VN_CAST(nodep->rhsp(), CvtArrayToArray)) {
            எனில் (castp->reverse()) {
                putns(castp, "VL_REVCOPY_Q(");
            } இல்லையெனில் {
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
        } இல்லையெனில் எனில் (nodep->isWide() && VN_IS(nodep->lhsp(), VarRef)  //
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
                   && !VN_IS(nodep->rhsp(), கோவை_Stmt)) {
            // Wide functions assign into the array directly, don't need separate assign statement
            m_wideTempRefp = VN_AS(nodep->lhsp(), VarRef);
            paren = false;
        } இல்லையெனில் எனில் (nodep->isWide() && !unpackDtp && !VN_IS(nodep->rhsp(), Const)) {
            putnbs(nodep, "VL_ASSIGN_W(");
            puts(cvtToStr(nodep->widthMin()) + ", ");
            iterateAndNextConstNull(nodep->lhsp());
            puts(", ");
        } இல்லையெனில் எனில் (VN_IS(nodep->lhsp()->dtypep()->skipRefp(), QueueDType)
                   && (VN_IS(nodep->rhsp(), StreamL) || VN_IS(nodep->lhsp(), StreamL)
                       || VN_IS(nodep->rhsp(), StreamR) || VN_IS(nodep->lhsp(), StreamR)
                       || VN_IS(nodep->rhsp(), StreamR))) {
            //if either side is streamL or streamR don't emit lhsp everything will be passed by
            //reference

            paren = false;

        } இல்லையெனில் {
            paren = false;
            iterateAndNextConstNull(nodep->lhsp());
            puts(" ");
            ofp()->blockInc();
            decind = true;
            எனில் (!VN_IS(nodep->rhsp(), Const)) ofp()->putBreak();
            putns(nodep, "= ");
        }
        எனில் (rhs) iterateAndNextConstNull(nodep->rhsp());
        எனில் (paren) puts(")");
        எனில் (decind) ofp()->blockDec();
        puts(";\n");
        எனில் (reverseUnpack) {
            puts("VL_UNPACK_REVERSED(");
            iterateAndNextConstNull(nodep->lhsp());
            puts(");\n");
        }
    }
    வெறுமை visit(AstAssocSel* nodep) override {
        iterateAndNextConstNull(nodep->fromp());
        மாறிலி std::string atFunc = nodep->isLValue() ? ".atWrite(" : ".at(";
        putnbs(nodep, atFunc);
        AstAssocArrayDType* மாறிலி adtypep
            = VN_AS(nodep->fromp()->dtypep()->skipRefp(), AssocArrayDType);
        UASSERT_OBJ(adtypep, nodep, "Associative select on non-associative type");
        iterateAndNextConstNull(nodep->bitp());
        puts(")");
    }
    வெறுமை visit(AstWildcardSel* nodep) override {
        iterateAndNextConstNull(nodep->fromp());
        மாறிலி std::string atFunc = nodep->isLValue() ? ".atWrite(" : ".at(";
        putnbs(nodep, atFunc);
        AstWildcardArrayDType* மாறிலி adtypep
            = VN_AS(nodep->fromp()->dtypep()->skipRefp(), WildcardArrayDType);
        UASSERT_OBJ(adtypep, nodep, "Wildcard select on non-wildcard-associative type");
        iterateAndNextConstNull(nodep->bitp());
        puts(")");
    }
    வெறுமை visit(AstCCall* nodep) override {
        மாறிலி AstCFunc* மாறிலி funcp = nodep->funcp();
        மாறிலி Ast_முனை_Module* மாறிலி funcModp = EmitCParentModule::get(funcp);
        putnbs(nodep, "");
        எனில் (funcp->dpiImportPrototype()) {
            // Calling DPI import
            putns(nodep, funcp->name());
        } இல்லையெனில் எனில் (funcp->isProperMethod() && funcp->isStatic()) {
            // Call static method via the containing class
            putns(funcModp, EmitCUtil::prefixNameProtect(funcModp) + "::");
            putns(nodep, funcp->nameProtect());
        } இல்லையெனில் எனில் (nodep->superReference()) {
            // Calling superclass method
            putns(funcModp, EmitCUtil::prefixNameProtect(funcModp) + "::");
            putns(nodep, funcp->nameProtect());
        } இல்லையெனில் எனில் (funcp->isLoose()) {
            // Calling loose method
            putns(nodep, funcNameProtect(funcp));
        } இல்லையெனில் {
            // Calling regular method/function
            எனில் (!nodep->selfPointer().isEmpty()) {
                emitDereference(nodep, nodep->selfPointerProtect(m_useSelfForThis));
            }
            putns(nodep, funcp->nameProtect());
        }
        emitCCallArgs(nodep, nodep->selfPointerProtect(m_useSelfForThis), m_cfuncp->needProcess());
    }
    வெறுமை visit(AstCMethodCall* nodep) override {
        மாறிலி AstCFunc* மாறிலி funcp = nodep->funcp();
        UASSERT_OBJ(!funcp->isLoose(), nodep, "Loose method called via AstCMethodCall");
        iterateConst(nodep->fromp());
        putnbs(nodep, "->");
        எனில் (nodep->superReference()) {
            // Super call, so the base implementation is used rather than the override
            மாறிலி Ast_முனை_Module* மாறிலி funcModp = EmitCParentModule::get(funcp);
            putns(funcModp, EmitCUtil::prefixNameProtect(funcModp) + "::");
        }
        putns(funcp, funcp->nameProtect());
        emitCCallArgs(nodep, "", m_cfuncp->needProcess());
    }
    வெறுமை visit(AstCAwait* nodep) override {
        putns(nodep, "co_await ");
        iterateConst(nodep->exprp());
        puts(";\n");
    }
    வெறுமை visit(AstCNew* nodep) override {
        எனில் (VN_IS(nodep->dtypep(), VoidDType)) {  // super.new case
            putsDecoration(nodep, "/*super.new*/");
            திரும்பு;
        }
        // assignment case;
        putns(nodep, "VL_NEW(" + EmitCUtil::prefixNameProtect(nodep->dtypep()) + ", "
                         + optionalProcArg(nodep->dtypep()) + "vlSymsp");
        putCommaIterateNext(nodep->argsp(), true);
        puts(")");
    }
    வெறுமை visit(AstCMethodHard* nodep) override {
        iterateConst(nodep->fromp());
        putns(nodep, nodep->usePtr() ? "->" : ".");
        putns(nodep, nodep->name());
        எனில் (nodep->method() == VCMethod::FORCE_READ_SEL) {
            emitIQW(nodep);
            எனில் (nodep->isWide()) puts("<" + cvtToStr(nodep->dtypep()->width_சொற்கள்()) + ">");
        } இல்லையெனில் எனில் (nodep->method() == VCMethod::ARRAY_SLICE) {
            // VlUnpacked::slice<N_Out>(loIdx) - N_Out is the (fixed) result array size
            மாறிலி AstUnpackArrayDType* மாறிலி adtypep
                = VN_AS(nodep->dtypep()->skipRefp(), UnpackArrayDType);
            puts("<" + cvtToStr(adtypep->elementsConst()) + ">");
        } இல்லையெனில் எனில் (nodep->method() == VCMethod::COVERGROUP_ADD_COVERPOINT) {
            // The hit-list bound is a template argument of the returned VlCoverpointT<>, and the
            // node's own dtype is that type, so it is the one source of truth for both.
            மாறிலி AstCoverpointDType* மாறிலி cpdtypep
                = VN_AS(nodep->dtypep()->skipRefp(), CoverpointDType);
            puts("<" + cvtToStr(cpdtypep->hitBound()) + ">");
        } இல்லையெனில் எனில் (nodep->method() == VCMethod::COVERGROUP_ADD_CROSS) {
            மாறிலி AstCoverCrossDType* மாறிலி cxdtypep
                = VN_AS(nodep->dtypep()->skipRefp(), CoverCrossDType);
            puts("<" + cxdtypep->cppTemplateArgs() + ">");
        }
        puts("(");
        bool comma = false;
        முழு arg_இலக்கம் = 0;
        க்கு (Ast_முனை* subnodep = nodep->pinsp(); subnodep; subnodep = subnodep->nextp()) {
            எனில் (comma) puts(", ");
            எனில் (nodep->method() == VCMethod::RANDOMIZER_HARD && arg_இலக்கம் == 1) {
                // For RANDOMIZER_HARD's filename argument (2nd arg after constraint),
                // apply protect() similar to VL_STOP to handle --protected flag
                எனில் (மாறிலி AstCExpr* மாறிலி cexprp = VN_CAST(subnodep, CExpr)) {
                    // Extract filename from the CExpr (which contains "filename")
                    std::string filename;
                    க்கு (மாறிலி Ast_முனை* textnodep = cexprp->nodesp(); textnodep;
                         textnodep = textnodep->nextp()) {
                        எனில் (மாறிலி AstText* மாறிலி textp = VN_CAST(textnodep, Text)) {
                            filename = textp->text();
                            நிறுத்து;
                        }
                    }
                    // Remove surrounding quotes if present
                    எனில் (filename.size() >= 2 && filename.front() == '"'
                        && filename.back() == '"') {
                        filename = filename.substr(1, filename.size() - 2);
                    }
                    // Emit with protect()
                    putsQuoted(protect(filename));
                } இல்லையெனில் {
                    iterateConst(subnodep);
                }
            } இல்லையெனில் {
                iterateConst(subnodep);
            }
            comma = true;
            arg_இலக்கம்++;
        }
        எனில் (nodep->withp()) {
            எனில் (comma) puts(", ");
            iterateConst(nodep->withp());
        }
        puts(")");
    }
    வெறுமை visit(AstCFuncHard* nodep) override {
        putns(nodep, nodep->name());
        puts("(");
        putCommaIterateNext(nodep->pinsp());
        puts(")");
    }
    வெறுமை visit(AstLambdaArgRef* nodep) override { putbs(nodep->nameProtect()); }
    வெறுமை visit(AstWith* nodep) override {
        // With uses a C++11 lambda
        putnbs(nodep, "[&](");
        எனில் (auto* மாறிலி argrefp = nodep->indexArgRefp()) {
            putnbs(argrefp, argrefp->dtypep()->cType(argrefp->nameProtect(), false, false));
            puts(", ");
        }
        எனில் (auto* மாறிலி argrefp = nodep->மதிப்பு_Arg_Refp()) {
            putnbs(argrefp, argrefp->dtypep()->cType(argrefp->nameProtect(), false, false));
        }
        puts(") -> ");
        putnbs(nodep, nodep->dtypep()->cType("", false, false));
        puts(" {\n");
        VL_RESTORER(m_createdScopeHash);
        iterateAndNextConstNull(nodep->exprp());
        puts("}\n");
    }
    வெறுமை visit(AstCase* nodep) override {  // LCOV_EXCL_LINE
        // In V3Case...
        nodep->v3fatalSrc("Case statements should have been reduced out");
    }
    வெறுமை visit(AstComment* nodep) override {
        string at;
        எனில் (nodep->showAt()) {
            at = " at " + nodep->fileline()->ascii();
            // If protecting, passthru less information about the design
            எனில் (!v3Global.opt.protectIds()) திரும்பு;
        }
        எனில் (!(nodep->protect() && v3Global.opt.protectIds())) {
            putsDecoration(nodep, "// "s + nodep->name() + at + "\n");
        }
        iterateChildrenConst(nodep);
    }
    வெறுமை visit(AstCoverOtherDecl* nodep) override {
        emit_Cover_Other_Decl_செருகு(nodep);
        எனில் (coverageUsesLocalCounter(nodep)) {
            // Some task bodies are cloned into a caller module where the
            // original instance-local counter is not addressable. Those clones
            // fall back to the global bin number, so register that slot too.
            emit_Cover_Other_Decl_செருகு(nodep, true);
        }
    }
    வெறுமை visit(AstCoverToggleDecl* nodep) override {
        putns(nodep, "vlSelf->__vlCoverToggleInsert(");  // As Declared in emitCoverageDecl
        puts(cvtToStr(nodep->range().வலது()));
        puts(", ");
        puts(cvtToStr(nodep->range().இடது()));
        puts(", ");
        puts(cvtToStr(nodep->range().ranged()));
        puts(", ");
        putCoverageArray(nodep);
        puts(" + ");
        puts(cvtToStr(coverage_Bin_இலக்கம்(nodep)));
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
                   + VIdProtect::protect_சொற்கள்_If(nodep->hier(), nodep->protect()));
        puts(", ");
        putsQuoted(VIdProtect::protect_சொற்கள்_If(nodep->page(), nodep->protect()));
        puts(", ");
        putsQuoted(VIdProtect::protect_சொற்கள்_If(nodep->comment(), nodep->protect()));
        puts(");\n");
    }
    வெறுமை visit(AstCoverInc* nodep) override {
        எனில் (VN_IS(nodep->declp(), CoverOtherDecl)) {
            எனில் (v3Global.opt.threads() > 1) {
                putns(nodep, "");
                putCoverageArray(nodep->declp());
                puts("[");
                puts(cvtToStr(coverage_Bin_இலக்கம்(nodep->declp())));
                puts("].fetch_add(1, std::memory_order_relaxed);\n");
            } இல்லையெனில் {
                putns(nodep, "++(");
                putCoverageArray(nodep->declp());
                puts("[");
                puts(cvtToStr(coverage_Bin_இலக்கம்(nodep->declp())));
                puts("]);\n");
            }
        } இல்லையெனில் {
            puts("VL_COV_TOGGLE_CHG_");
            எனில் (v3Global.opt.threads() > 1) {
                puts("MT_");
            } இல்லையெனில் {
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
            puts(cvtToStr(coverage_Bin_இலக்கம்(nodep->declp())));
            puts(", ");
            iterateConst(nodep->toggleExprp());
            puts(", ");
            iterateConst(nodep->toggleCovExprp());
            puts(");\n");
        }
    }
    வெறுமை visit(AstDisableFork* nodep) override { putns(nodep, "vlProcess->disableFork();\n"); }
    வெறுமை visit(AstCReturn* nodep) override {
        putns(nodep, "return (");
        iterateAndNextConstNull(nodep->lhsp());
        puts(");\n");
    }
    வெறுமை visit(AstDisplay* nodep) override {
        string text = nodep->fmtp()->text();
        எனில் (nodep->addNewline()) text += "\n";
        display_முனை(nodep, nodep->fmtp(), text, nodep->fmtp()->exprsp(), false);
    }
    வெறுமை visit(AstDumpCtl* nodep) override {
        தேர்வு (nodep->ctlType()) {
        நிலை VDumpCtlType::FILE:
            putns(nodep, "vlSymsp->_vm_contextp__->dumpfile(");
            iterateConst(nodep->exprp());
            puts(");\n");
            நிறுத்து;
        நிலை VDumpCtlType::VARS:
            // We ignore number of levels to dump in exprp()
            எனில் (v3Global.opt.trace()) {
                putns(nodep, "vlSymsp->_traceDumpOpen();\n");
            } இல்லையெனில் {
                putns(nodep, "VL_PRINTF_MT(\"-Info: ");
                puts(V3OutFormatter::quoteNameControls(protect(nodep->fileline()->filename())));
                puts(":");
                puts(cvtToStr(nodep->fileline()->lineno()));
                puts(": $dumpvar ignored, as Verilated without --trace");
                puts("\\n\");\n");
            }
            நிறுத்து;
        நிலை VDumpCtlType::ALL:
            // $dumpall currently ignored
            நிறுத்து;
        நிலை VDumpCtlType::FLUSH:
            // $dumpall currently ignored; would need rework of VCD single thread,
            // or flag we pass-through to next eval() iteration
            நிறுத்து;
        நிலை VDumpCtlType::LIMIT:
            // $dumplimit currently ignored
            நிறுத்து;
        நிலை VDumpCtlType::OFF:
            // Currently ignored as both Vcd and Fst do not support them, as would need "X" dump
            நிறுத்து;
        நிலை VDumpCtlType::ON:
            // Currently ignored as $dumpoff is also ignored
            நிறுத்து;
        இயல்பு: nodep->v3fatalSrc("Bad case, unexpected " << nodep->ctlType().ascii());
        }
    }
    வெறுமை visit(AstScopeName* nodep) override {
        // For use under AstCCalls for dpiImports.  ScopeNames under
        // displays are handled in AstDisplay
        எனில் (!nodep->dpiExport()) {
            // this is where the DPI import context scope is set
            மாறிலி string scope = nodep->scopeDpiName();
            putnbs(nodep, "(vlSymsp->" + protect("__Vscopep_" + scope) + ")");
        }
    }
    வெறுமை visit(AstSFormat* nodep) override {
        display_முனை(nodep, nodep->fmtp(), nodep->fmtp()->text(), nodep->fmtp()->exprsp(), false);
    }
    வெறுமை visit(AstSFormatF* nodep) override {
        display_முனை(nodep, nodep, nodep->text(), nodep->exprsp(), false);
    }
    வெறுமை visit(AstFScanF* nodep) override {
        display_முனை(nodep, nullptr, nodep->text(), nodep->exprsp(), true);
    }
    வெறுமை visit(AstSScanF* nodep) override {
        display_முனை(nodep, nullptr, nodep->text(), nodep->exprsp(), true);
    }
    வெறுமை visit(Ast_மதிப்பு_Plus_Args* nodep) override {
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
    வெறுமை visit(AstTestPlusArgs* nodep) override {
        putns(nodep, "VL_TESTPLUSARGS_I(");
        iterateConst(nodep->searchp());
        puts(")");
    }
    வெறுமை visit(AstFError* nodep) override {
        putns(nodep, "VL_FERROR_I");
        puts(nodep->strp()->isString() ? "N(" : "W(");
        iterateAndNextConstNull(nodep->filep());
        putbs(", ");
        எனில் (nodep->strp()->isWide()) {
            puts(cvtToStr(nodep->strp()->width_சொற்கள்()));
            putbs(", ");
        }
        iterateAndNextConstNull(nodep->strp());
        puts(")");
    }
    வெறுமை visit(AstFGetS* nodep) override {
        emitOpName(nodep, nodep->emitC(), nodep->strgp(), nodep->filep(), nullptr);
    }
    வெறுமை visit(AstFOpen* nodep) override {
        putns(nodep, "VL_FOPEN_NN(");
        iterateConst(nodep->filenamep());
        putbs(", ");
        iterateConst(nodep->modep());
        puts(");\n");
    }
    வெறுமை visit(AstFOpenMcd* nodep) override {
        putns(nodep, "VL_FOPEN_MCD_N(");
        iterateConst(nodep->filenamep());
        puts(");\n");
    }
    வெறுமை visit(Ast_முனை_Read_Write_Mem* nodep) override {
        putns(nodep, nodep->cFuncPrefixp());
        puts("N(");
        puts(nodep->isHex() ? "true" : "false");
        putbs(", ");
        // Need real storage width
        puts(cvtToStr(nodep->memp()->dtypep()->skipRefp()->subDTypep()->widthMin()));
        uint32_t array_lo = 0;
        bool memUsePtr = true;
        {
            மாறிலி AstVarRef* மாறிலி varrefp = VN_CAST(nodep->memp(), VarRef);
            எனில் (!varrefp) {
                nodep->v3error(nodep->verilogKwd() << " loading non-variable");
            } இல்லையெனில் எனில் (VN_IS(varrefp->varp()->dtypeSkipRefp(), AssocArrayDType)
                       || VN_IS(varrefp->varp()->dtypeSkipRefp(), DynArrayDType)) {
                // nodep->memp() below will when verilated code is compiled create a C++ template
                memUsePtr = false;
            } இல்லையெனில் எனில் (மாறிலி AstUnpackArrayDType* மாறிலி adtypep
                       = VN_CAST(varrefp->varp()->dtypeSkipRefp(), UnpackArrayDType)) {
                putbs(", ");
                puts(cvtToStr(varrefp->varp()->dtypep()->arrayUnpackedElements()));
                array_lo = adtypep->lo();
                putbs(", ");
                puts(cvtToStr(array_lo));
            } இல்லையெனில் {
                nodep->v3error(
                    nodep->verilogKwd()
                    << " loading other than unpacked/dynamic/associative-array variable");
            }
        }
        putbs(", ");
        iterateConst(nodep->filenamep());
        putbs(", ");
        {
            எனில் (memUsePtr) puts(" &(");
            iterateAndNextConstNull(nodep->memp());
            எனில் (memUsePtr) puts(")");
        }
        putbs(", ");
        எனில் (nodep->lsbp()) {
            iterateAndNextConstNull(nodep->lsbp());
        } இல்லையெனில் {
            puts(cvtToStr(array_lo));
        }
        putbs(", ");
        எனில் (nodep->msbp()) {
            iterateAndNextConstNull(nodep->msbp());
        } இல்லையெனில் {
            puts("~0ULL");
        }
        puts(");\n");
    }
    வெறுமை visit(AstFClose* nodep) override {
        putns(nodep, "VL_FCLOSE_I(");
        iterateAndNextConstNull(nodep->filep());
        puts("); ");
    }
    வெறுமை visit(AstFFlush* nodep) override {
        எனில் (!nodep->filep()) {
            putns(nodep, "VL_FFLUSH_MT();");
        } இல்லையெனில் {
            putns(nodep, "VL_FFLUSH_I(");
            iterateAndNextConstNull(nodep->filep());
            puts(");\n");
        }
    }
    வெறுமை visit(AstFSeek* nodep) override {
        putns(nodep, "(VL_FSEEK_I(");
        iterateAndNextConstNull(nodep->filep());
        puts(", ");
        iterateAndNextConstNull(nodep->offset());
        puts(", ");
        iterateAndNextConstNull(nodep->operation());
        puts(") == -1 ? -1 : 0)");
    }
    வெறுமை visit(AstFTell* nodep) override {
        putns(nodep, "VL_FTELL_I(");
        iterateAndNextConstNull(nodep->filep());
        puts(")");
    }
    வெறுமை visit(AstFRewind* nodep) override {
        putns(nodep, "(VL_FSEEK_I(");
        iterateAndNextConstNull(nodep->filep());
        puts(", 0, 0) == -1 ? -1 : 0)");
    }
    வெறுமை visit(AstFRead* nodep) override {
        putns(nodep, "VL_FREAD_I(");
        puts(cvtToStr(nodep->memp()->widthMin()));  // Need real storage width
        putbs(", ");
        uint32_t array_lo = 0;
        uint32_t array_size = 0;
        {
            மாறிலி AstVarRef* மாறிலி varrefp = VN_CAST(nodep->memp(), VarRef);
            எனில் (!varrefp) {
                nodep->v3error(nodep->verilogKwd() << " loading non-variable");
            } இல்லையெனில் எனில் (VN_CAST(varrefp->varp()->dtypeSkipRefp(), BasicDType)) {
            } இல்லையெனில் எனில் (மாறிலி AstUnpackArrayDType* மாறிலி adtypep
                       = VN_CAST(varrefp->varp()->dtypeSkipRefp(), UnpackArrayDType)) {
                array_lo = adtypep->lo();
                array_size = adtypep->elementsConst();
            } இல்லையெனில் {
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
        எனில் (nodep->startp()) {
            iterateAndNextConstNull(nodep->startp());
        } இல்லையெனில் {
            puts(cvtToStr(array_lo));
        }
        putbs(", ");
        எனில் (nodep->countp()) {
            iterateAndNextConstNull(nodep->countp());
        } இல்லையெனில் {
            puts(cvtToStr(array_size));
        }
        puts(")");
    }
    வெறுமை visit(Ast_அடுக்கு_Trace_F* nodep) override { putns(nodep, "VL_STACKTRACE_N()"); }
    வெறுமை visit(Ast_அடுக்கு_Trace_T* nodep) override { putns(nodep, "VL_STACKTRACE();\n"); }
    வெறுமை visit(AstSystemT* nodep) override {
        putns(nodep, "(void)VL_SYSTEM_I");
        emitIQW(nodep->lhsp());
        puts("(");
        எனில் (nodep->lhsp()->isWide()) {
            puts(cvtToStr(nodep->lhsp()->width_சொற்கள்()));
            putbs(", ");
        }
        iterateAndNextConstNull(nodep->lhsp());
        puts(");\n");
    }
    வெறுமை visit(AstSystemF* nodep) override {
        putns(nodep, "VL_SYSTEM_I");
        emitIQW(nodep->lhsp());
        puts("(");
        எனில் (nodep->lhsp()->isWide()) {
            puts(cvtToStr(nodep->lhsp()->width_சொற்கள்()));
            putbs(", ");
        }
        iterateAndNextConstNull(nodep->lhsp());
        puts(")");
    }
    வெறுமை visit(Ast_Stmt_கோவை* nodep) override {
        எனில் (VN_IS(nodep->exprp()->dtypep()->skipRefp(), VoidDType)) {
            putns(nodep, "");
        } இல்லையெனில் {
            putns(nodep, "std::ignore = ");
        }
        iterateConst(nodep->exprp());
        puts(";\n");
    }
    வெறுமை visit(AstJumpBlock* nodep) override {
        // Allocate label number
        மாறிலி size_t n = m_labelNumbers.size();
        மாறிலி bool newEntry = m_labelNumbers.emplace(nodep, n).second;
        UASSERT_OBJ(newEntry, nodep, "AstJumpBlock visited twide");
        // Emit
        putns(nodep, "{\n");  // Make it visually obvious label jumps outside these
        VL_RESTORER(m_createdScopeHash);
        iterateAndNextConstNull(nodep->stmtsp());
        puts("__Vlabel" + std::to_string(n) + ": ;\n");
        puts("}\n");
    }
    வெறுமை visit(AstJumpGo* nodep) override {
        // Retrieve target label number - must already exist (from enclosing AstJumpBlock)
        மாறிலி size_t n = m_labelNumbers.at(nodep->blockp());
        // Emit
        putns(nodep, "goto __Vlabel" + std::to_string(n) + ";\n");
    }
    வெறுமை visit(AstLoop* nodep) override {
        UASSERT_OBJ(!nodep->contsp(), nodep, "'contsp' only used before LinkJump");
        VL_RESTORER(m_createdScopeHash);
        // Special case when the AstLoopTest is first for output readability
        எனில் (AstLoopTest* மாறிலி testp = VN_CAST(nodep->stmtsp(), LoopTest)) {
            putns(nodep, "while (");
            iterateConst(testp->condp());
            puts(") {\n");
            iterateAndNextConstNull(testp->nextp());
            puts("}\n");
            திரும்பு;
        }
        // Special case when the AstLoopTest is last for output readability
        எனில் (Ast_முனை* lastp = nodep->stmtsp()) {
            போது (Ast_முனை* மாறிலி nextp = lastp->nextp()) lastp = nextp;
            எனில் (AstLoopTest* மாறிலி testp = VN_CAST(lastp, LoopTest)) {
                putns(nodep, "do {\n");
                க்கு (Ast_முனை* p = nodep->stmtsp(); p != lastp; p = p->nextp()) iterateConst(p);
                puts("} while (");
                iterateConst(testp->condp());
                puts(");\n");
                திரும்பு;
            }
        }
        // Emit generic case directly
        putns(nodep, "while (true) {\n");
        iterateAndNextConstNull(nodep->stmtsp());
        puts("}\n");
    }
    வெறுமை visit(AstLoopTest* nodep) override {
        VL_RESTORER(m_createdScopeHash);
        putns(nodep, "if (!(");
        iterateAndNextConstNull(nodep->condp());
        puts(")) break;\n");
    }
    வெறுமை visit(AstCLocalScope* nodep) override {
        putns(nodep, "{\n");
        VL_RESTORER(m_createdScopeHash);
        iterateAndNextConstNull(nodep->stmtsp());
        puts("}\n");
    }
    வெறுமை visit(Ast_முனை_If* nodep) override {
        putns(nodep, "if (");
        எனில் (!nodep->branchPred().unknown()) {
            puts(nodep->branchPred().ascii());
            puts("((");  // Two parens, so that VL_UNLIKELY((class<foo,bar>)) works
        }
        iterateAndNextConstNull(nodep->condp());
        எனில் (!nodep->branchPred().unknown()) puts("))");
        puts(") {\n");
        {
            VL_RESTORER(m_createdScopeHash);
            iterateAndNextConstNull(nodep->thensp());
        }
        puts("}");
        எனில் (!nodep->elsesp()) {
            puts("\n");
        } இல்லையெனில் {
            எனில் (VN_IS(nodep->elsesp(), முனை_If) && !nodep->elsesp()->nextp()) {
                puts(" else ");
                iterateAndNextConstNull(nodep->elsesp());
            } இல்லையெனில் {
                VL_RESTORER(m_createdScopeHash);
                puts(" else {\n");
                iterateAndNextConstNull(nodep->elsesp());
                puts("}\n");
            }
        }
    }
    வெறுமை visit(Ast_கோவை_Stmt* nodep) override {
        VL_RESTORER(m_createdScopeHash);
        மாறிலி bool containsAwait = nodep->exists([](AstCAwait*) -> bool { திரும்பு true; });
        எனில் (containsAwait) {
            UASSERT_OBJ(m_cfuncp && m_cfuncp->isCoroutine(), nodep,
                        "AstExprStmt with CAwait must be in coroutine");
            putnbs(nodep, "(co_await ([&]() -> VlCoroutine {\n");
            iterateAndNextConstNull(nodep->stmtsp());
            puts("co_return;\n");
            எனில் (!nodep->hasResult()) {
                puts("}()))");
                திரும்பு;
            }
            puts("}()), ");
            iterateAndNextConstNull(nodep->resultp());
            puts(")");
            திரும்பு;
        }
        // GCC allows compound statements in expressions, but this is not standard.
        // So we use an immediate-evaluation lambda and comma operator
        putnbs(nodep, "([&]() {\n");
        எனில் (!nodep->hasResult()) {
            iterateAndNextConstNull(nodep->stmtsp());
            puts("}())");
            திரும்பு;
        }
        iterateAndNextConstNull(nodep->stmtsp());
        puts("}(), ");
        iterateAndNextConstNull(nodep->resultp());
        puts(")");
    }
    வெறுமை visit(AstStop* nodep) override {
        putns(nodep, "VL_STOP_MT(");
        putsQuoted(protect(nodep->fileline()->filename()));
        puts(", ");
        puts(cvtToStr(nodep->fileline()->lineno()));
        puts(", \"\"");
        எனில் (nodep->isFatal()) puts(", false");
        puts(");\n");
    }
    வெறுமை visit(AstFinish* nodep) override {
        // Disable all the forks so they don't operate after simulation is finished.
        எனில் (m_cfuncp && m_cfuncp->needProcess()) putns(nodep, "vlProcess->disableFork();\n");
        putns(nodep, "VL_FINISH_MT(");
        putsQuoted(protect(nodep->fileline()->filename()));
        puts(", ");
        puts(cvtToStr(nodep->fileline()->lineno()));
        puts(", \"\");\n");
    }
    வெறுமை visit(AstFinishFork* nodep) override {
        // Disable all the forks so they don't operate after simulation is finished.
        எனில் (m_cfuncp && m_cfuncp->needProcess()) putns(nodep, "vlProcess->disableFork();\n");
        putns(nodep, "VL_FINISH_MT(");
        putsQuoted(protect(nodep->fileline()->filename()));
        puts(", ");
        puts(cvtToStr(nodep->fileline()->lineno()));
        puts(", \"\");\n");
        எனில் (m_cfuncp->isCoroutine()) {
            putns(nodep, "co_return;\n");
        } இல்லையெனில் {
            putns(nodep, "return;\n");
        }
    }
    வெறுமை visit(AstPrintTimeScale* nodep) override {
        putns(nodep, "VL_PRINTTIMESCALE(");
        putsQuoted(protect(nodep->prettyName()));
        puts(", ");
        putsQuoted(nodep->timeunit().ascii());
        puts(", vlSymsp->_vm_contextp__);\n");
    }
    வெறுமை visit(AstRand* nodep) override {
        emitOpName(nodep, nodep->emitC(), nodep->seedp(), nullptr, nullptr);
    }
    வெறுமை visit(AstRandRNG* nodep) override {
        emitOpName(nodep, nodep->emitC(), nullptr, nullptr, nullptr);
    }
    வெறுமை visit(AstTime* nodep) override {
        putns(nodep, "VL_TIME_UNITED_Q(");
        UASSERT_OBJ(!nodep->timeunit().isNone(), nodep, "$time has no units");
        மாறிலி இரட்டை time
            = nodep->timeunit().multiplier() / v3Global.rootp()->timeprecision().multiplier();
        UASSERT_OBJ(time >= 1, nodep, "TimeQ is less than 1, will result in division by zero");
        puts(cvtToStr(time));
        puts(")");
    }
    வெறுமை visit(AstTimeD* nodep) override {
        putns(nodep, "VL_TIME_UNITED_D(");
        UASSERT_OBJ(!nodep->timeunit().isNone(), nodep, "$realtime has no units");
        puts(cvtToStr(nodep->timeunit().multiplier()
                      / v3Global.rootp()->timeprecision().multiplier()));
        puts(")");
    }
    வெறுமை visit(AstGetInitialRandomSeed* nodep) override {
        putns(nodep, "vlSymsp->_vm_contextp__->randSeed()");
    }
    வெறுமை visit(AstTimeFormat* nodep) override {
        putns(nodep, "VL_TIMEFORMAT_IINI(");
        எனில் (nodep->unitsp()) {
            puts("true, ");
            iterateAndNextConstNull(nodep->unitsp());
        } இல்லையெனில் {
            puts("false, 0");
        }
        puts(", ");
        எனில் (nodep->precisionp()) {
            puts("true, ");
            iterateAndNextConstNull(nodep->precisionp());
        } இல்லையெனில் {
            puts("false, 0");
        }
        puts(", ");
        எனில் (nodep->suffixp()) {
            puts("true, ");
            iterateAndNextConstNull(nodep->suffixp());
        } இல்லையெனில் {
            puts("false, \"\"");
        }
        puts(", ");
        எனில் (nodep->widthp()) {
            puts("true, ");
            iterateAndNextConstNull(nodep->widthp());
        } இல்லையெனில் {
            puts("false, 0");
        }
        puts(", vlSymsp->_vm_contextp__);\n");
    }
    வெறுமை visit(AstTimePrecision* nodep) override {
        putns(nodep, "vlSymsp->_vm_contextp__->timeprecision()");
    }

    // Nodes involing AstText
    வெறுமை visit(AstText* nodep) override {
        // All Text should be under TextBlock/CStmt/CStmtUser/CExpr/CExprUser
        nodep->v3fatalSrc("Text node in unexpected position");
    }
    வெறுமை visit(AstTextBlock* nodep) override {
        putnbs(nodep, "");
        puts(nodep->prefix());
        emitNodesWithText(nodep->nodesp(), false, true, nodep->separator());
        puts(nodep->suffix());
    }
    வெறுமை visit(AstCStmt* nodep) override {
        putnbs(nodep, "");
        emitNodesWithText(nodep->nodesp(), false, true, "");
        ensure_New_வரி();
    }
    வெறுமை visit(AstCExpr* nodep) override {
        putnbs(nodep, "");
        emitNodesWithText(nodep->nodesp(), false, true, "");
    }
    வெறுமை visit(AstCStmtUser* nodep) override {
        putnbs(nodep, "");
        ofp()->putsNoTracking("\n");
        எனில் (nodep->fromDollarC() && v3Global.opt.decoration() && !v3Global.opt.protectIds()) {
            ofp()->putsNoTracking("// $c statement at " + nodep->fileline()->ascii() + "\n");
        }
        emitNodesWithText(nodep->nodesp(), m_useSelfForThis, false, "");
        puts("\n");
    }
    வெறுமை visit(AstCExprUser* nodep) override {
        putnbs(nodep, "");
        ofp()->putsNoTracking("\n");
        எனில் (/* is always from $c */ v3Global.opt.decoration() && !v3Global.opt.protectIds()) {
            ofp()->putsNoTracking(
                (nodep->isPure() ? "// $cpure expression at " : "// $c expression at ")
                + nodep->fileline()->ascii() + "\n");
        }
        emitNodesWithText(nodep->nodesp(), m_useSelfForThis, false, "");
        puts("\n");
    }

    // Operators
    வெறுமை visit(Ast_முனை_Termop* nodep) override {
        emitOpName(nodep, nodep->emitC(), nullptr, nullptr, nullptr);
    }
    வெறுமை visit(Ast_முனை_Uniop* nodep) override {
        எனில் (nodep->emit_Check_Max_சொற்கள்()
            && (nodep->width_சொற்கள்() > VL_MULS_MAX_சொற்கள்
                || nodep->lhsp()->width_சொற்கள்() > VL_MULS_MAX_சொற்கள்)) {
            nodep->v3warn(
                E_UNSUPPORTED,
                "Unsupported: "
                    << nodep->prettyOperatorName() << " operator of " << nodep->width()
                    << " bits exceeds hardcoded limit VL_MULS_MAX_WORDS in verilatedos.h");
        }
        எனில் (emitSimpleOk(nodep)) {
            putnbs(nodep, "(");
            puts(nodep->emitSimpleOperator());
            puts(" ");
            iterateAndNextConstNull(nodep->lhsp());
            puts(")");
        } இல்லையெனில் {
            emitOpName(nodep, nodep->emitC(), nodep->lhsp(), nullptr, nullptr);
        }
    }
    வெறுமை visit(Ast_முனை_Biop* nodep) override {
        எனில் (nodep->emit_Check_Max_சொற்கள்() && nodep->width_சொற்கள்() > VL_MULS_MAX_சொற்கள்) {
            nodep->v3warn(
                E_UNSUPPORTED,
                "Unsupported: "
                    << nodep->prettyOperatorName() << " operator of " << nodep->width()
                    << " bits exceeds hardcoded limit VL_MULS_MAX_WORDS in verilatedos.h");
        }
        எனில் (emitSimpleOk(nodep)) {
            putnbs(nodep, "(");
            iterateAndNextConstNull(nodep->lhsp());
            puts(" ");
            putbs(nodep->emitSimpleOperator());
            puts(" ");
            iterateAndNextConstNull(nodep->rhsp());
            puts(")");
        } இல்லையெனில் {
            emitOpName(nodep, nodep->emitC(), nodep->lhsp(), nodep->rhsp(), nullptr);
        }
    }
    வெறுமை visit(Ast_முனை_Triop* nodep) override {
        UASSERT_OBJ(!emitSimpleOk(nodep), nodep, "Triop cannot be described in a simple way");
        emitOpName(nodep, nodep->emitC(), nodep->lhsp(), nodep->rhsp(), nodep->thsp());
    }
    வெறுமை visit(AstCvtPackString* nodep) override { emitCvtPackStr(nodep->lhsp()); }
    வெறுமை visit(AstRedXor* nodep) override {
        எனில் (nodep->lhsp()->isWide()) {
            visit(static_cast<Ast_முனை_Uniop*>(nodep));
        } இல்லையெனில் {
            மாறிலி AstVarRef* மாறிலி vrefp = VN_CAST(nodep->lhsp(), VarRef);
            மாறிலி முழு widthPow2 = vrefp ? vrefp->varp()->dtypep()->widthPow2()
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
    வெறுமை visit(AstCCast* nodep) override {
        // Extending a value of the same word width is just a NOP.
        எனில் (மாறிலி AstClassRefDType* மாறிலி classDtypep
            = VN_CAST(nodep->dtypep()->skipRefp(), ClassRefDType)) {
            putns(nodep, "(" + classDtypep->cType("", false, false) + ")(");
        } இல்லையெனில் எனில் (nodep->size() <= VL_BYTESIZE) {
            putns(nodep, "(::CData)(");
        } இல்லையெனில் எனில் (nodep->size() <= VL_SHORTSIZE) {
            putns(nodep, "(::SData)(");
        } இல்லையெனில் எனில் (nodep->size() <= VL_IDATASIZE) {
            putns(nodep, "(::IData)(");
        } இல்லையெனில் {
            putns(nodep, "(::QData)(");
        }
        iterateAndNextConstNull(nodep->lhsp());
        puts(")");
    }
    வெறுமை visit(AstCond* nodep) override {
        // Widths match up already, so we'll just use C++'s operator w/o any temps.
        எனில் (nodep->thenp()->isWide()) {
            emitOpName(nodep, nodep->emitC(), nodep->condp(), nodep->thenp(), nodep->elsep());
        } இல்லையெனில் {
            putnbs(nodep, "(");
            iterateAndNextConstNull(nodep->condp());
            putbs(" ? ");
            iterateAndNextConstNull(nodep->thenp());
            putbs(" : ");
            iterateAndNextConstNull(nodep->elsep());
            puts(")");
        }
    }
    வெறுமை visit(AstMatchMasked* nodep) override {
        emitOpName(nodep, nodep->emitC(), nodep->lhsp(), nodep->matchp(), nullptr);
    }
    வெறுமை visit(AstMemberSel* nodep) override {
        மாறிலி AstVar* மாறிலி varp = nodep->varp();
        மாறிலி bool dereferenceCovergroupRef
            = varp->covergroupRefMember() && nodep->access().isReadOrRW();
        எனில் (dereferenceCovergroupRef) putnbs(nodep, "(*");
        iterateAndNextConstNull(nodep->fromp());
        putnbs(nodep, "->");
        எனில் (varp->isIfaceRef()) {
            // varp is the __Viftop companion (e.g. "tx__Viftop"); use the
            // MemberSel name which matches the cell's C++ member (e.g. "tx").
            puts(nodep->nameProtect());
        } இல்லையெனில் {
            puts(varName(varp));
        }
        எனில் (dereferenceCovergroupRef) puts(")");
    }
    வெறுமை visit(AstStructSel* nodep) override {
        iterateAndNextConstNull(nodep->fromp());
        putnbs(nodep, ".");
        puts(nodep->nameProtect());
    }
    வெறுமை visit(AstNullCheck* nodep) override {
        putns(nodep, "VL_NULL_CHECK(");
        iterateAndNextConstNull(nodep->lhsp());
        puts(", ");
        putsQuoted(protect(nodep->fileline()->filename()));
        puts(", ");
        puts(cvtToStr(nodep->fileline()->lineno()));
        puts(")");
    }
    வெறுமை visit(AstNewCopy* nodep) override {
        // Polymorphic shallow clone: preserves runtime type via virtual clone()
        // VL_NULL_CHECK enforces null check per IEEE 1800-2023 8.7
        putns(nodep, "VL_NULL_CHECK(");
        எனில் (VN_IS(nodep->rhsp(), Const) && VN_AS(nodep->rhsp(), Const)->isNull()) {
            // V3Const folded rhs to null: emit a typed empty ref so VL_NULL_CHECK fires
            மாறிலி AstClassRefDType* மாறிலி refDTypep
                = VN_CAST(nodep->dtypep()->skipRefp(), ClassRefDType);
            puts(refDTypep->cType("", false, false) + "{}");
        } இல்லையெனில் {
            iterateAndNextConstNull(nodep->rhsp());
        }
        puts(", ");
        putsQuoted(protect(nodep->fileline()->filename()));
        puts(", ");
        puts(cvtToStr(nodep->fileline()->lineno()));
        puts(").clone(vlSymsp->__Vm_deleter)");
    }
    வெறுமை visit(AstSel* nodep) override {
        // Note ASSIGN checks for this on a LHS
        UASSERT_OBJ(nodep->widthMin() == nodep->widthConst(), nodep, "Width mismatch");
        emitOpName(nodep, nodep->emitC(), nodep->fromp(), nodep->lsbp(), nullptr);
    }
    வெறுமை visit(AstReplicate* nodep) override {
        எனில் (nodep->srcp()->widthMin() == 1 && !nodep->isWide()) {
            UASSERT_OBJ((static_cast<முழு>(VN_AS(nodep->countp(), Const)->toUInt())
                         * nodep->srcp()->widthMin())
                            == nodep->widthMin(),
                        nodep, "Replicate non-constant or width miscomputed");
            putns(nodep, "VL_REPLICATE_");
            emitIQW(nodep);
            puts("OI(");
            எனில் (nodep->srcp()) puts(cvtToStr(nodep->srcp()->widthMin()));
            puts(", ");
            iterateAndNextConstNull(nodep->srcp());
            puts(", ");
            iterateAndNextConstNull(nodep->countp());
            puts(")");
        } இல்லையெனில் {
            emitOpName(nodep, nodep->emitC(), nodep->srcp(), nodep->countp(), nullptr);
        }
    }
    வெறுமை emitStreamR(AstStreamR* nodep, Ast_முனை* parent) {
        //TODO: This might need to handle more cases like the visit(AstStreamR) function
        emitOpName(nodep, nodep->emitC(), nodep->lhsp(), nodep->rhsp(), nullptr);
    }
    வெறுமை visit(AstStreamR* nodep) override {
        //The parrent node of our AstStreamR will give just enough info for what streamR should
        //output if nodep->backp() is not the parent then emitStreamR should have been used. throw
        //an error
        bool backpIsParent = (nodep->backp()->op1p() == nodep || nodep->backp()->op2p() == nodep);
        UASSERT(backpIsParent, "can not find return type for streamR");
        எனில் (மாறிலி AstQueueDType* மாறிலி qdtp
            = VN_CAST(nodep->backp()->dtypep()->skipRefp(), QueueDType)) {
            எனில் (nodep->lhsp()->isWide()) {
                emitOpName(nodep,
                           "VL_STREAMR_%nq%lq%rq(%lw," + std::to_string(qdtp->subDTypep()->width())
                               + ", %P, %li, %ri)",
                           nodep->lhsp(), nodep->rhsp(), nullptr);
            } இல்லையெனில் {
                emitOpName(nodep, "VL_STREAMR_%nq%lq%rq(%lw, %P, %li, %ri)", nodep->lhsp(),
                           nodep->rhsp(), nullptr);
            }
        } இல்லையெனில் எனில் (VN_IS(nodep->lhsp()->dtypep()->skipRefp(), QueueDType)) {
            எனில் (!((nodep->backp()->op1p() && nodep->backp()->op1p()->isWide())
                  || (nodep->backp()->op2p() && nodep->backp()->op2p()->isWide()))) {
                //If our lhsp is a queue make sure we streamR and return the correct type.
                //If either side is wide or the previous node is string type dont use this case
                emitOpName(nodep->backp(), "VL_STREAMR_%nq%lq%rq(%lw, %P, %li, %ri)",
                           nodep->lhsp(), nodep->rhsp(), nullptr);
            } இல்லையெனில் {
                emitOpName(nodep, nodep->emitC(), nodep->lhsp(), nodep->rhsp(), nullptr);
            }
        } இல்லையெனில் {
            emitOpName(nodep, nodep->emitC(), nodep->lhsp(), nodep->rhsp(), nullptr);
        }
    }
    வெறுமை visit(AstStreamL* nodep) override {
        // Attempt to use a "fast" stream function for slice size = power of 2
        எனில் (!nodep->isWide()) {
            மாறிலி uint32_t isPow2 = VN_AS(nodep->rhsp(), Const)->இலக்கம்().மொத்தம்_Ones() == 1;
            மாறிலி uint32_t sliceSize = VN_AS(nodep->rhsp(), Const)->toUInt();
            எனில் (isPow2 && sliceSize <= (nodep->isQuad() ? அளவு(uint64_t) : அளவு(uint32_t))) {
                putns(nodep, "VL_STREAML_FAST_");
                bool usesQueue = false;
                AstQueueDType* qtypep
                    = nodep->backp()->op2p()
                          ? VN_CAST(nodep->backp()->op2p()->dtypep()->skipRefp(), QueueDType)
                          : nullptr;
                எனில் (VN_IS(nodep->backp(), Assign)
                    && qtypep) {  // If we are assigning to a queue then emit the correct symbol
                    puts("R");  // R for queue
                    usesQueue = true;
                } இல்லையெனில் {
                    emitIQW(nodep);
                }
                emitIQW(nodep->lhsp());
                puts("I(");
                puts(cvtToStr(nodep->lhsp()->widthMin()));
                puts(", ");
                எனில் (usesQueue) {
                    iterateAndNextConstNull(nodep->backp()->op2p());
                    puts(", ");
                }
                iterateAndNextConstNull(nodep->lhsp());
                puts(", ");
                மாறிலி uint32_t rd_log2 = V3_எண்::log2b(VN_AS(nodep->rhsp(), Const)->toUInt());
                puts(cvtToStr(rd_log2) + ")");
                திரும்பு;
            }
        }
        எனில் (VN_IS(nodep->backp(), Assign)
            && VN_IS(nodep->backp()->op2p()->dtypep()->skipRefp(), QueueDType)) {
            முழு queueWidth
                = nodep->backp()->op2p()->dtypep()->subDTypep()->width();  //We need to know the
                                                                           //width of both sides
            emitOpName(nodep,
                       "VL_STREAML_%nq%lq%rq(%lw," + std::to_string(queueWidth)
                           + ", %P, %li, %ri)",
                       nodep->lhsp(), nodep->rhsp(), nullptr);
        } இல்லையெனில் {
            emitOpName(nodep, nodep->emitC(), nodep->lhsp(), nodep->rhsp(), nullptr);
        }
    }
    வெறுமை visit(AstCastDynamic* nodep) override {
        putnbs(nodep, "VL_CAST_DYNAMIC(");
        iterateAndNextConstNull(nodep->fromp());
        puts(", ");
        iterateAndNextConstNull(nodep->உச்சி());
        puts(")");
    }
    வெறுமை visit(Ast_மொத்தம்_Bits* nodep) override {
        putnbs(nodep, "VL_COUNTBITS_");
        emitIQW(nodep->lhsp());
        puts("(");
        puts(cvtToStr(nodep->lhsp()->widthMin()));
        puts(", ");
        எனில் (nodep->lhsp()->isWide()) {
            puts(cvtToStr(nodep->lhsp()->width_சொற்கள்()));  // Note argument width, not node width
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
    வெறுமை visit(AstTriggered* nodep) override {
        // Test each word of the trigger vector with a mask of the relevant bits
        std::map<uint32_t, uint64_t> wordMasks;
        க்கு (மாறிலி uint32_t index : nodep->indices()) {
            wordMasks[VL_BITWORD_Q(index)] |= VL_BIT_Q(index);
        }
        putns(nodep, "((");
        bool first = true;
        க்கு (மாறிலி auto& wordMask : wordMasks) {
            எனில் (!first) puts("\n| ");
            first = false;
            மாறிலி bool allOnes = wordMask.second == ~0ULL;
            எனில் (!allOnes) puts("(" + V3_எண்{nodep, 64}.setQuad(wordMask.second).emitC() + " & ");
            iterateConst(nodep->trigRefp());
            puts("[" + std::to_string(wordMask.first) + "U]");
            எனில் (!allOnes) puts(")");
        }
        puts(") != 0ULL)");
    }
    // Terminals
    வெறுமை visit(AstVarRef* nodep) override {
        மாறிலி AstVar* மாறிலி varp = nodep->varp();

        // Reference to constant pool variable is special
        எனில் (varp->constPoolEntry()) {
            putns(nodep, EmitCUtil::constPoolName(varp));
            திரும்பு;
        }

        மாறிலி Ast_முனை_Module* மாறிலி varModp = EmitCParentModule::get(varp);
        மாறிலி bool dereferenceCovergroupRef
            = varp->covergroupRefMember() && nodep->access().isReadOrRW();
        எனில் (dereferenceCovergroupRef) putns(nodep, "(*");
        எனில் (VN_IS(varModp, Class) && varModp != m_modp) {
            // Superclass member reference
            putns(nodep, EmitCUtil::prefixNameProtect(varModp) + "::");
        } இல்லையெனில் எனில் (varp->isIfaceRef()) {
            putns(nodep, nodep->selfPointerProtect(m_useSelfForThis));
            திரும்பு;
        } இல்லையெனில் எனில் (!nodep->selfPointer().isEmpty()) {
            emitDereference(nodep, nodep->selfPointerProtect(m_useSelfForThis));
        }
        putns(nodep, varName(varp));
        எனில் (dereferenceCovergroupRef) puts(")");
    }
    வெறுமை visit(AstAddrOfCFunc* nodep) override {
        // Note: Can be thought to handle more, but this is all that is needed right now
        மாறிலி AstCFunc* மாறிலி funcp = nodep->funcp();
        UASSERT_OBJ(funcp->isLoose(), nodep, "Cannot take address of non-loose method");
        putns(nodep, "&");
        puts(funcNameProtect(funcp));
    }
    வெறுமை visit(AstConst* nodep) override {  //
        எனில் (m_wideTempRefp && nodep->isWide()) {
            UASSERT_OBJ(m_wideTempRefp, nodep, "Wide Constant w/ no temp");
            emitConstantW(nodep, m_wideTempRefp);
            m_wideTempRefp = nullptr;  // We used it, fail if set it a second time
        } இல்லையெனில் {
            emitConstant(nodep);
        }
    }
    வெறுமை visit(Ast_முனை_Cons* nodep) override { emitValCons(nodep); }
    வெறுமை visit(AstThisRef* nodep) override {
        putnbs(nodep, nodep->dtypep()->cType("", false, false));
        puts("{");
        puts(VSelfPointerText::replaceThis(m_useSelfForThis, "this"));
        puts("}");
    }
    வெறுமை visit(Ast_முனை_Sel* nodep) override {
        எனில் (!VN_IS(nodep, ArraySel) && !VN_IS(nodep, WordSel)) {
            visit(static_cast<Ast_முனை_Biop*>(nodep));
            திரும்பு;
        }
        // ArraySel or WordSel
        iterateAndNextConstNull(nodep->fromp());
        // Special case constant index for readability
        எனில் (AstConst* மாறிலி idxp = VN_CAST(nodep->bitp(), Const)) {
            puts("[" + std::to_string(idxp->toUInt()) + "U]");
            திரும்பு;
        }
        putbs("[");
        iterateAndNextConstNull(nodep->bitp());
        puts("]");
    }

    //
    வெறுமை visit(AstExecGraph* nodep) override {
        // The location of the AstExecGraph within the containing AstCFunc is where we want to
        // invoke the graph and wait for it to complete. The run-time library does just that.
        putns(nodep, "vl_invokeExecGraph(" + EmitCUtil::execGraphName(nodep) + ", vlSelf, ");
        iterateConst(nodep->triggerp());
        puts(".data());\n");
    }

    // Default
    வெறுமை visit(Ast_முனை* nodep) override {  // LCOV_EXCL_START
        putns(nodep, "\n???? // "s + nodep->prettyTypeName() + "\n");
        iterateChildrenConst(nodep);
        எனில் (!v3Global.opt.lintOnly()) {  // An internal problem, so suppress
            nodep->v3fatalSrc("Unknown node type reached emitter: " << nodep->prettyTypeName());
        }
    }  // LCOV_EXCL_STOP

பாதுகாக்கப்பட்ட:
    EmitCFunc() = இயல்பு;
    ~EmitCFunc() override = இயல்பு;
};

#endif  // guard
