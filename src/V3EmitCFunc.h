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

#અન્તર્ભાવ "config_build.h"
#અન્તર્ભાવ "verilatedos.h"

#અન્તર્ભાવ "V3EmitCConstInit.h"
#અન્તર્ભાવ "V3Global.h"
#અન્તર્ભાવ "V3MemberMap.h"

#અન્તર્ભાવ <algorithm>
#અન્તર્ભાવ <map>
#અન્તર્ભાવ <unordered_map>
#અન્તર્ભાવ <unordered_set>
#અન્તર્ભાવ <vector>

// Number of VL_CONST_W_*X's in verilated.h (IE VL_CONST_W_8X is last)
constexpr પૂર્ણાંક EMITC_અંક_CONSTW = 8;

//######################################################################
// Emit lazy forward declarations

class EmitCLazyDecls final : જાહેર VNVisitorConst {
    // NODE STATE/TYPES
    // None allowed to support threaded emitting

    // MEMBERS
    std::unordered_set<string> m_emittedManually;  // Set of names already declared manually.
    EmitCBaseVisitorConst& m_emitter;  // For access to file output
    bool m_needs_Blank_લીટી = false;  // Emit blank line if any declarations were emitted (cosmetic)
    std::set<Ast_ગાંઠ*> m_emitted;  // -> in set. Already emitted decl for symbols.

    // METHODS
    bool declaredOnce(Ast_ગાંઠ* nodep) { પાછા m_emitted.ઉમેરો(nodep).second; }

    શૂન્ય lazyDeclare(AstCFunc* funcp) {
        // Already declared in this compilation unit
        જો (!declaredOnce(funcp)) પાછા;
        // Check if this kind of function is lazily declared
        જો (!(funcp->isMethod() && funcp->isLoose()) && !funcp->dpiImportPrototype()) પાછા;
        // Already declared manually
        જો (m_emittedManually.ગણતરી(funcp->nameProtect())) પાછા;
        // Needs lazy declaration, emit one
        m_emitter.emitCFuncDecl(funcp, EmitCParentModule::get(funcp), funcp->dpiImportPrototype());
        m_needs_Blank_લીટી = true;
    }

    શૂન્ય lazyDeclareConstPoolVar(AstVar* varp) {
        જો (!declaredOnce(varp)) પાછા;  // Already declared
        અચલ string nameProtect = EmitCUtil::constPoolName(varp);
        m_emitter.putns(varp, "extern const ");
        m_emitter.puts(varp->dtypep()->cType(nameProtect, false, false));
        m_emitter.puts(";\n");
        m_needs_Blank_લીટી = true;
    }

    // VISITORS
    શૂન્ય visit(Ast_ગાંઠ_CCall* nodep) override {
        lazyDeclare(nodep->funcp());
        iterateChildrenConst(nodep);
    }

    શૂન્ય visit(AstAddrOfCFunc* nodep) override {
        lazyDeclare(nodep->funcp());
        iterateChildrenConst(nodep);
    }

    શૂન્ય visit(AstVarRef* nodep) override {
        AstVar* અચલ varp = nodep->varp();
        // Only constant pool symbols are lazy declared for now ...
        જો (EmitCParentModule::get(varp)->isConstPool()) lazyDeclareConstPoolVar(varp);
    }

    શૂન્ય visit(AstExecGraph* nodep) override {
        જો (!declaredOnce(nodep)) પાછા;
        m_emitter.putns(nodep, "extern VlExecGraph " + EmitCUtil::execGraphName(nodep) + ";\n");
        m_needs_Blank_લીટી = true;
    }

    શૂન્ય visit(Ast_ગાંઠ* nodep) override { iterateChildrenConst(nodep); }

જાહેર:
    explicit EmitCLazyDecls(EmitCBaseVisitorConst& emitter)
        : m_emitter{emitter} {}
    શૂન્ય emit(Ast_ગાંઠ* nodep) {
        m_needs_Blank_લીટી = false;
        iterateChildrenConst(nodep);
        જો (m_needs_Blank_લીટી) m_emitter.puts("\n");
    }
    શૂન્ય emit(અચલ string& prefix, અચલ string& name, અચલ string& suffix) {
        m_emittedManually.ઉમેરો(name);
        m_emitter.ensure_New_લીટી();
        m_emitter.puts(prefix);
        m_emitter.puts(name);
        m_emitter.puts(suffix);
        m_emitter.ensure_New_લીટી();
    }
    શૂન્ય declared(AstCFunc* nodep) { m_emitted.ઉમેરો(nodep); }
    શૂન્ય reset() { m_emitted.clear(); }
};

// ######################################################################
//  Emit statements and expressions

class EmitCFunc VL_NOT_FINAL : જાહેર EmitCConstInit {
    VMemberMap m_memberMap;
    AstVarRef* m_wideTempRefp = nullptr;  // Variable that _WW macros should be setting
    std::unordered_map<AstJumpBlock*, size_t> m_labelNumbers;  // Label numbers for AstJumpBlocks
    bool m_createdScopeHash = false;  // Already created a scope hash

સુરક્ષિત:
    VL_DEFINE_DEBUG_FUNCTIONS;

    EmitCLazyDecls m_lazyDecls{*this};  // Visitor for emitting lazy declarations
    bool m_useSelfForThis = false;  // Replace "this" with "vlSelf"
    bool m_usevlSelfRef = false;  // Use vlSelfRef reference instead of vlSelf pointer
    અચલ Ast_ગાંઠ_Module* m_modp = nullptr;  // Current module being emitted
    અચલ AstCFunc* m_cfuncp = nullptr;  // Current function being emitted
    bool m_instantiatesOwnProcess = false;
    અચલ AstClassPackage* m_classOrPackage = nullptr;  // Pointer to current class or package
    string m_classOrPackageHash;  // Hash of class or package name

    bool constructorNeedsProcess(અચલ AstClass* અચલ classp) {
        અચલ Ast_ગાંઠ* અચલ newp = m_memberMap.findMember(classp, "new");
        જો (!newp) પાછા false;
        અચલ AstCFunc* અચલ ctorp = VN_CAST(newp, CFunc);
        જો (!ctorp) પાછા false;
        UASSERT_OBJ(ctorp->isConstructor(), ctorp, "`new` is not a constructor!");
        પાછા ctorp->needProcess();
    }

    bool constructorNeedsProcess(અચલ Ast_ગાંઠ_DType* અચલ dtypep) {
        જો (અચલ AstClassRefDType* અચલ crefdtypep = VN_CAST(dtypep, ClassRefDType))
            પાછા constructorNeedsProcess(crefdtypep->classp());
        પાછા false;
    }

    bool coverageUsesLocalCounter(Ast_ગાંઠ_Cover_Decl* અચલ declp) અચલ {
        // Only functions with an object receiver can address the module-local
        // coverage array. Static/package/class helper functions still use the
        // symbol-table array, as they may have vlSymsp but no vlSelf/this.
        // Also require the coverage declaration to belong to the current
        // module; cloned task bodies can increment declarations owned by a
        // different module, and those must keep using the global array.
        પાછા m_cfuncp && !m_cfuncp->isStatic() && !VN_IS(m_modp, Class)
               && !VN_IS(m_modp, ClassPackage)
               && EmitCParentModule::get(declp->dataDeclThisp()) == m_modp;
    }
    પૂર્ણાંક coverage_Bin_અંક(Ast_ગાંઠ_Cover_Decl* અચલ declp, bool forceGlobal = false) અચલ {
        પાછા !forceGlobal && coverageUsesLocalCounter(declp)
                   ? declp->dataDeclThisp()->local_Bin_અંક()
                   : declp->dataDeclThisp()->bin_અંક();
    }
    શૂન્ય putCoverageArray(Ast_ગાંઠ_Cover_Decl* અચલ declp, bool forceGlobal = false) {
        જો (!forceGlobal && coverageUsesLocalCounter(declp)) {
            // Keep counters on the module object when possible, so every
            // no-inline instance has storage available if coverage output is
            // later written with forcePerInstance.
            puts(m_cfuncp->isLoose() ? "vlSelf->__Vcoverage" : "this->__Vcoverage");
        } નહીંતર {
            // Static/package/class helper functions do not have vlSelf, so
            // keep using the shared symbol-table coverage array there.
            puts("vlSymsp->__Vcoverage");
        }
    }
    શૂન્ય emit_Cover_Other_Decl_ઉમેરો(AstCoverOtherDecl* nodep, bool forceGlobal = false) {
        putns(nodep, "vlSelf->__vlCoverInsert(");  // As Declared in emitCoverageDecl
        putCoverageArray(nodep, forceGlobal);
        puts(" + ");
        puts(cvtToStr(coverage_Bin_અંક(nodep, forceGlobal)));
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
                   + VIdProtect::protect_શબ્દો_If(nodep->hier(), nodep->protect()));
        puts(", ");
        putsQuoted(VIdProtect::protect_શબ્દો_If(nodep->page(), nodep->protect()));
        puts(", ");
        putsQuoted(VIdProtect::protect_શબ્દો_If(nodep->comment(), nodep->protect()));
        puts(", ");
        putsQuoted(nodep->linescov());
        puts(", ");
        putsQuoted(VIdProtect::protect_શબ્દો_If(nodep->fsmVar(), nodep->protect()));
        puts(", ");
        putsQuoted(VIdProtect::protect_શબ્દો_If(nodep->fsmFrom(), nodep->protect()));
        puts(", ");
        putsQuoted(VIdProtect::protect_શબ્દો_If(nodep->fsmTo(), nodep->protect()));
        puts(", ");
        putsQuoted(VIdProtect::protect_શબ્દો_If(nodep->fsmTag(), nodep->protect()));
        puts(");\n");
    }

જાહેર:
    // METHODS
    bool displayEmitHeader(Ast_ગાંઠ* nodep);
    શૂન્ય display_ગાંઠ(Ast_ગાંઠ* nodep, AstSFormatF* fmtp, અચલ string& vformat, Ast_ગાંઠ* exprsp,
                     bool isScan);

    bool emitSimpleOk(Ast_ગાંઠ_પદાવલિ* nodep);
    શૂન્ય emitIQW(અચલ Ast_ગાંઠ* nodep) {
        // See "Type letters" in verilated.h
        // Other abbrevs: "C"har, "S"hort, "F"loat, "D"ouble, stri"N"g, "R"=queue, "U"npacked
        puts(nodep->dtypep()->skipRefp()->charIQWN());
    }
    શૂન્ય emitRU(અચલ Ast_ગાંઠ* nodep) {
        અચલ Ast_ગાંઠ_DType* dtp = nodep->dtypep()->skipRefp();
        // See "Type letters" in verilated.h
        જો (VN_IS(dtp, UnpackArrayDType))
            puts("U");
        નહીંતર જો (VN_IS(dtp, QueueDType) || VN_IS(dtp, DynArrayDType))
            puts("R");
    }
    શૂન્ય emitScIQW(AstVar* nodep) {
        UASSERT_OBJ(nodep->isSc(), nodep, "emitting SystemC operator on non-SC variable");
        puts(nodep->isScBigUint()                           ? "SB"
             : (nodep->isScUint() || nodep->isScUintBool()) ? "SU"
             : nodep->isScBv()                              ? "SW"
             : nodep->isDouble()                            ? "SD"
                                                            : (nodep->isScQuad() ? "SQ" : "SI"));
    }
    શૂન્ય emitDatap(અચલ Ast_ગાંઠ* nodep) {
        // When passing to a function with va_args the compiler doesn't
        // know need a pointer so when wide, need to look inside VlWide
        જો (nodep->isWide()) puts(".data()");
    }
    શૂન્ય emitOpName(Ast_ગાંઠ* nodep, અચલ string& format, Ast_ગાંઠ* lhsp, Ast_ગાંઠ* rhsp,
                    Ast_ગાંઠ* thsp);
    શૂન્ય emitCCallArgs(અચલ Ast_ગાંઠ_CCall* nodep, અચલ string& selfPointer, bool inProcess);
    શૂન્ય emitDereference(Ast_ગાંઠ* nodep, અચલ string& pointer);
    std::string dereferenceString(અચલ std::string& pointer) અચલ;
    શૂન્ય emitCvtPackStr(Ast_ગાંઠ* nodep);
    શૂન્ય emitConstant(AstConst* nodep);
    શૂન્ય emitConstantString(અચલ AstConst* nodep);
    શૂન્ય emitVarReset(અચલ string& prefix, AstVar* varp, bool constructing);
    string emitVarResetRecurse(અચલ AstVar* varp, bool constructing,
                               અચલ string& varNameProtected, Ast_ગાંઠ_DType* dtypep, પૂર્ણાંક depth,
                               અચલ string& suffix, અચલ Ast_ગાંઠ* valuep);
    શૂન્ય emitVarResetScopeHash();
    શૂન્ય emitChangeDet();
    શૂન્ય putCommaIterateNext(Ast_ગાંઠ* nodep, bool comma = false) {
        માટે (Ast_ગાંઠ* subnodep = nodep; subnodep; subnodep = subnodep->nextp()) {
            જો (comma) puts(", ");
            iterateConst(subnodep);
            comma = true;
        }
    }
    template <typename T>
    string optionalProcArg(અચલ T* અચલ nodep) {
        પાછા (nodep && constructorNeedsProcess(nodep)) ? "vlProcess, " : "";
    }
    અચલ AstCNew* getSuperNewCallRecursep(Ast_ગાંઠ* અચલ nodep) {
        // Get the super.new call
        જો (!nodep) પાછા nullptr;
        જો (અચલ AstCNew* અચલ cnewp = VN_CAST(nodep, CNew)) પાછા cnewp;
        જો (અચલ Ast_Stmt_પદાવલિ* અચલ stmtp = VN_CAST(nodep, Stmt_પદાવલિ)) {
            જો (અચલ AstCNew* અચલ cnewp = VN_CAST(stmtp->exprp(), CNew)) પાછા cnewp;
        }
        જો (અચલ AstJumpBlock* અચલ blockp = VN_CAST(nodep, JumpBlock)) {
            જો (અચલ AstCNew* અચલ cnewp = getSuperNewCallRecursep(blockp->stmtsp())) {
                પાછા cnewp;
            }
        }
        જો (અચલ AstCNew* અચલ cnewp = getSuperNewCallRecursep(nodep->nextp())) પાછા cnewp;
        પાછા nullptr;
    }

    શૂન્ય emitConstantW(AstConst* nodep, AstVarRef* assigntop) {
        // For tradition and compilation speed, assign each word directly into
        // output variable instead of using '='
        putns(nodep, "");
        જો (nodep->અંક().isFourState()) {
            nodep->v3warn(E_UNSUPPORTED, "Unsupported: 4-state numbers in this context");
            પાછા;
        }

        પૂર્ણાંક upWidth = nodep->અંક().widthToFit();
        પૂર્ણાંક chunks = 0;
        જો (upWidth > EMITC_અંક_CONSTW * VL_EDATASIZE) {
            // Output e.g. 8 words in groups of e.g. 8
            chunks = (upWidth - 1) / (EMITC_અંક_CONSTW * VL_EDATASIZE);
            upWidth %= (EMITC_અંક_CONSTW * VL_EDATASIZE);
            જો (upWidth == 0) upWidth = (EMITC_અંક_CONSTW * VL_EDATASIZE);
        }
        {  // Upper e.g. 8 words
            જો (chunks) {
                putnbs(nodep, "VL_CONSTHI_W_");
                puts(cvtToStr(VL_શબ્દો_I(upWidth)));
                puts("X(");
                puts(cvtToStr(nodep->widthMin()));
                puts(",");
                puts(cvtToStr(chunks * EMITC_અંક_CONSTW * VL_EDATASIZE));
            } નહીંતર {
                putnbs(nodep, "VL_CONST_W_");
                puts(cvtToStr(VL_શબ્દો_I(upWidth)));
                puts("X(");
                puts(cvtToStr(nodep->widthMin()));
            }
            puts(",");
            જો (!assigntop->selfPointer().isEmpty()) {
                emitDereference(assigntop, assigntop->selfPointerProtect(m_useSelfForThis));
            }
            puts(varName(assigntop->varp()));
            માટે (પૂર્ણાંક word = VL_શબ્દો_I(upWidth) - 1; word >= 0; word--) {
                // Only 32 bits - llx + long long here just to appease CPP format warning
                ofp()->printf(",0x%08" PRIx64, static_cast<uint64_t>(nodep->અંક().edataWord(
                                                   word + chunks * EMITC_અંક_CONSTW)));
            }
            puts(")");
        }
        માટે (chunks--; chunks >= 0; chunks--) {
            puts(";\n");
            putbs("VL_CONSTLO_W_");
            puts(cvtToStr(EMITC_અંક_CONSTW));
            puts("X(");
            puts(cvtToStr(chunks * EMITC_અંક_CONSTW * VL_EDATASIZE));
            puts(",");
            જો (!assigntop->selfPointer().isEmpty()) {
                emitDereference(assigntop, assigntop->selfPointerProtect(m_useSelfForThis));
            }
            puts(varName(assigntop->varp()));
            માટે (પૂર્ણાંક word = EMITC_અંક_CONSTW - 1; word >= 0; word--) {
                // Only 32 bits - llx + long long here just to appease CPP format warning
                ofp()->printf(",0x%08" PRIx64, static_cast<uint64_t>(nodep->અંક().edataWord(
                                                   word + chunks * EMITC_અંક_CONSTW)));
            }
            puts(")");
        }
    }

    શૂન્ય emitNodesWithText(Ast_ગાંઠ* nodesp, bool useSelfForThis, bool tracking,
                           અચલ std::string& separator) {
        માટે (Ast_ગાંઠ* nodep = nodesp; nodep; nodep = nodep->nextp()) {
            જો (અચલ AstText* અચલ textp = VN_CAST(nodep, Text)) {
                અચલ std::string text
                    = VSelfPointerText::replaceThis(useSelfForThis, textp->text());
                જો (tracking) {
                    puts(text);
                } નહીંતર {
                    ofp()->putsNoTracking(text);
                }
            } નહીંતર {
                iterateConst(nodep);
            }
            જો (nodep->nextp()) puts(separator);
        }
    }

    શૂન્ય putConstructorSubinit(અચલ AstClass* classp, AstCFunc* cfuncp) {
        // Direct non-virtual bases in declaration order
        માટે (અચલ AstClassExtends* extp = classp->extendsp(); extp;
             extp = VN_AS(extp->nextp(), ClassExtends)) {
            જો (extp->classp()->isInterfaceClass()) ચાલુ;
            જો (extp->classp()->useVirtualPublic()) ચાલુ;
            puts(EmitCUtil::prefixNameProtect(extp->classp()));
            puts("::init");
            જો (constructorNeedsProcess(extp->classp())) {
                puts("(vlProcess, vlSymsp");
            } નહીંતર {
                puts("(vlSymsp");
            }
            // Handle super.new() args for the concrete parent
            જો (અચલ AstCNew* અચલ superNewCallp = getSuperNewCallRecursep(cfuncp->stmtsp())) {
                putCommaIterateNext(superNewCallp->argsp(), true);
            }
            puts(");\n");
            // Since SystemVerilog allows only for extending from one class, for sure there won't
            // be any more constructors to be called. Even if there are some classes which are
            // "implemented", those are interface classes which do not have constructors.
            તોડો;
        }
    }

    // VISITORS
    using EmitCConstInit::visit;
    શૂન્ય visit(AstCFunc* અચલ nodep) override {
        જો (nodep->emptyBody() && !nodep->isLoose()) પાછા;
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
        જો (nodep->ifdef() != "") putns(nodep, "#ifdef " + nodep->ifdef() + "\n");
        emitCFuncHeader(nodep, m_modp, /* withScope: */ true);

        puts(" {\n");
        જો (nodep->isConstructor()) {
            અચલ AstClass* અચલ classp = VN_CAST(nodep->scopep()->modp(), Class);
            જો (classp && classp->extendsp()) putConstructorSubinit(classp, nodep);
        }

        // "+" in the debug indicates a print from the model
        puts("VL_DEBUG_IF(VL_DBG_MSGF(\"+  ");
        માટે (પૂર્ણાંક i = 0; i < m_modp->level(); ++i) puts("  ");
        puts(EmitCUtil::prefixNameProtect(m_modp));
        puts(nodep->isLoose() ? "__" : "::");
        puts(nodep->nameProtect() + "\\n\"); );\n");

        જો (nodep->isLoose()) {
            m_lazyDecls.declared(nodep);  // Defined here, so no longer needs declaration
            જો (!nodep->isStatic()) {  // Standard prologue
                m_useSelfForThis = true;
                જો (nodep->voidSelf()) {
                    અચલ string className = EmitCUtil::prefixNameProtect(m_modp);
                    puts(className + "* const vlSelf = static_cast<" + className
                         + "*>(vlVoidSelf);\n");
                }
                જો (!VN_IS(m_modp, Class)) {
                    puts(EmitCUtil::symClassAssign());  // Uses vlSelf
                } નહીંતર {
                    puts("(void)vlSelf;  // Prevent unused variable warning\n");
                }
            }
        }

        // Instantiate a process class if it's going to be needed somewhere later
        nodep->forall([&](અચલ Ast_ગાંઠ_CCall* ccallp) -> bool {
            જો (ccallp->funcp()->needProcess()
                && (ccallp->funcp()->isCoroutine() == VN_IS(ccallp->backp(), CAwait))) {
                જો (!nodep->needProcess() && !m_instantiatesOwnProcess) {
                    m_instantiatesOwnProcess = true;
                    પાછા false;
                }
            }
            પાછા true;
        });
        જો (m_instantiatesOwnProcess) {
            AstCStmt* અચલ vlprocp = new AstCStmt{nodep->fileline()};
            vlprocp->add("VlProcessRef vlProcess = std::make_shared<VlProcess>();\n");
            vlprocp->add("VlProcess::currentp(vlProcess.get());");
            nodep->prependStmtsp(vlprocp);
        } નહીંતર જો (nodep->needProcess() && nodep->stmtsp()) {
            // Set current process so VlRNG() constructors in this function seed from it
            AstCStmt* અચલ setProcessp = new AstCStmt{nodep->fileline()};
            setProcessp->add("VlProcess::currentp(vlProcess.get());");
            nodep->prependStmtsp(setProcessp);
        }

        માટે (Ast_ગાંઠ* subnodep = nodep->argsp(); subnodep; subnodep = subnodep->nextp()) {
            જો (AstVar* અચલ varp = VN_CAST(subnodep, Var)) {
                જો (varp->isFuncReturn()) emitVarDecl(varp);
            }
        }

        જો (m_useSelfForThis) {
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

        જો (nodep->varsp()) {
            putsDecoration(nodep, "// Locals\n");
            iterateAndNextConstNull(nodep->varsp());
        }

        જો (nodep->stmtsp()) {
            putsDecoration(nodep, "// Body\n");
            iterateAndNextConstNull(nodep->stmtsp());
        }

        m_usevlSelfRef = false;

        જો (nodep->isCoroutine()) {
            // Sometimes coroutines don't have co_awaits,
            // so emit a co_return at the end to avoid compile errors.
            puts("co_return;\n");
        }

        puts("}\n");
        જો (nodep->ifdef() != "") puts("#endif  // " + nodep->ifdef() + "\n");
    }

    શૂન્ય visit(AstVar* nodep) override {
        UASSERT_OBJ(m_cfuncp, nodep, "Cannot emit non-local variable");
        emitVarDecl(nodep);
    }

    શૂન્ય visit(AstCvtArrayToArray* nodep) override {
        જો (nodep->reverse()) {
            puts("VL_REVCLONE_Q(");
        } નહીંતર {
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

    શૂન્ય visit(AstCvtArrayToPacked* nodep) override {
        Ast_ગાંઠ_DType* અચલ fromDtp = nodep->fromp()->dtypep()->skipRefp();
        Ast_ગાંઠ_DType* અચલ elemDtp = fromDtp->subDTypep()->skipRefp();
        અચલ bool descending = [&]() {
            જો (અચલ AstUnpackArrayDType* અચલ unpackDtp = VN_CAST(fromDtp, UnpackArrayDType))
                પાછા !unpackDtp->declRange().ascending();
            પાછા false;
        }();
        puts("VL_PACK_");
        emitIQW(nodep);
        puts("_");
        emitRU(fromDtp);
        emitIQW(elemDtp);
        જો (descending) {
            // Wrap source in VL_PACK_REVERSED so VL_PACK sees ascending order
            emitOpName(nodep, "(%nw, %rw, %P, VL_PACK_REVERSED(%li))", nodep->fromp(), elemDtp,
                       nullptr);
        } નહીંતર {
            emitOpName(nodep, "(%nw, %rw, %P, %li)", nodep->fromp(), elemDtp, nullptr);
        }
    }

    શૂન્ય visit(AstCvtUnpackToQueue* nodep) override {
        Ast_ગાંઠ_DType* અચલ elemDTypep = nodep->fromp()->dtypep()->subDTypep();
        emitOpName(nodep, nodep->emitC(), nodep->fromp(), elemDTypep, nullptr);
    }
    શૂન્ય visit(AstCvtUnpackToUnpack* nodep) override {
        // Whether each dimension is reversed, the first for the outermost, up to the last that is
        std::vector<bool> reverses;
        અચલ AstUnpackArrayDType* ટોચ = VN_AS(nodep->dtypep()->skipRefp(), UnpackArrayDType);
        અચલ AstUnpackArrayDType* fromp
            = VN_AS(nodep->fromp()->dtypep()->skipRefp(), UnpackArrayDType);
        જ્યારે (ટોચ) {
            reverses.ધકેલો_back(ટોચ->declRange().ascending() != fromp->declRange().ascending());
            ટોચ = VN_CAST(ટોચ->subDTypep()->skipRefp(), UnpackArrayDType);
            fromp = VN_CAST(fromp->subDTypep()->skipRefp(), UnpackArrayDType);
        }
        જ્યારે (!reverses.back()) reverses.કાઢો_back();
        putns(nodep, "VL_CVT_UNPACK_TO_UNPACK<");
        માટે (size_t i = 0; i < reverses.size(); ++i) {
            જો (i) puts(", ");
            puts(reverses[i] ? "1" : "0");
        }
        puts(">(");
        iterateAndNextConstNull(nodep->fromp());
        puts(")");
    }

    શૂન્ય visit(Ast_ગાંઠ_Assign* nodep) override {
        જો (AstCReset* અચલ resetp = VN_CAST(nodep->rhsp(), CReset)) {
            // TODO get rid of emitVarReset and instead let AstNodeAssign understand how to init
            // anything
            Ast_ગાંઠ* fromp = nodep->lhsp();
            // Fork needs to use a member select.  Nothing else should be possible before VarRef.
            જો (AstMemberSel* અચલ sfromp = VN_CAST(fromp, MemberSel)) {
                // Fork-DynScope generated pointer to previously automatic variable
                AstVar* અચલ memberVarp = sfromp->varp();
                fromp = sfromp->fromp();
                જો (AstNullCheck* અચલ sfromp = VN_CAST(fromp, NullCheck)) fromp = sfromp->lhsp();
                Ast_ગાંઠ_Var_Ref* અચલ fromVarRefp = VN_AS(fromp, ગાંઠ_Var_Ref);
                emitVarReset(
                    ("VL_NULL_CHECK("s
                     + (fromVarRefp->selfPointer().isEmpty()
                            ? ""
                            : dereferenceString(fromVarRefp->selfPointerProtect(m_useSelfForThis)))
                     + varName(fromVarRefp->varp()) + ", \""
                     + V3OutFormatter::quoteNameControls(protect(nodep->fileline()->filename()))
                     + "\", " + std::to_string(nodep->fileline()->lineno()) + ")->"),
                    memberVarp, resetp->constructing());
            } નહીંતર {
                Ast_ગાંઠ_Var_Ref* અચલ fromVarRefp = VN_AS(fromp, ગાંઠ_Var_Ref);
                AstVar* અચલ varp = fromVarRefp->varp();
                અચલ string prefix
                    = fromVarRefp->selfPointer().isEmpty()
                          ? ""
                          : dereferenceString(
                                VN_AS(fromp, ગાંઠ_Var_Ref)->selfPointerProtect(m_useSelfForThis));
                emitVarReset(prefix, varp, resetp->constructing());
            }
            પાછા;
        }
        bool paren = true;
        bool decind = false;
        bool rhs = true;
        bool reverseUnpack = false;  // Set for descending CvtPackedToArray
        અચલ AstUnpackArrayDType* અચલ unpackDtp
            = VN_CAST(nodep->dtypep()->skipRefp(), UnpackArrayDType);
        જો (AstSel* અચલ selp = VN_CAST(nodep->lhsp(), Sel)) {
            UASSERT_OBJ(selp->widthMin() == selp->widthConst(), selp, "Width mismatch");
            જો (selp->widthMin() == 1) {
                putnbs(nodep, "VL_ASSIGNBIT_");
                emitIQW(selp->fromp());
                જો (nodep->rhsp()->isAllOnesV()) {
                    puts("O(");
                    rhs = false;
                } નહીંતર {
                    puts("I(");
                }
                iterateAndNextConstNull(selp->lsbp());
                puts(", ");
                iterateAndNextConstNull(selp->fromp());
                જો (rhs) puts(", ");
            } નહીંતર {
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
        } નહીંતર જો (અચલ AstGetcRefN* અચલ selp = VN_CAST(nodep->lhsp(), GetcRefN)) {
            iterateAndNextConstNull(selp->lhsp());
            puts(" = ");
            putnbs(selp, "VL_PUTC_N(");
            iterateAndNextConstNull(selp->lhsp());
            puts(", ");
            iterateAndNextConstNull(selp->rhsp());
            puts(", ");
        } નહીંતર જો (AstVar* અચલ varp = AstVar::scVarRecurse(nodep->lhsp())) {
            putnbs(varp, "VL_ASSIGN_");  // Set a systemC variable
            emitScIQW(varp);
            emitIQW(nodep);
            puts("(");
            puts(cvtToStr(nodep->widthMin()) + ", ");
            iterateAndNextConstNull(nodep->lhsp());
            puts(", ");
        } નહીંતર જો (AstVar* અચલ varp = AstVar::scVarRecurse(nodep->rhsp())) {
            putnbs(varp, "VL_ASSIGN_");  // Get a systemC variable
            emitIQW(nodep);
            emitScIQW(varp);
            puts("(");
            puts(cvtToStr(nodep->widthMin()) + ", ");
            iterateAndNextConstNull(nodep->lhsp());
            puts(", ");
        } નહીંતર જો (અચલ AstCvtPackedToArray* અચલ castp
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
            જો (unpackDtp && !unpackDtp->declRange().ascending()) reverseUnpack = true;
        } નહીંતર જો (અચલ AstCvtArrayToArray* અચલ castp
                   = VN_CAST(nodep->rhsp(), CvtArrayToArray)) {
            જો (castp->reverse()) {
                putns(castp, "VL_REVCOPY_Q(");
            } નહીંતર {
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
        } નહીંતર જો (nodep->isWide() && VN_IS(nodep->lhsp(), VarRef)  //
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
                   && !VN_IS(nodep->rhsp(), પદાવલિ_Stmt)) {
            // Wide functions assign into the array directly, don't need separate assign statement
            m_wideTempRefp = VN_AS(nodep->lhsp(), VarRef);
            paren = false;
        } નહીંતર જો (nodep->isWide() && !unpackDtp && !VN_IS(nodep->rhsp(), Const)) {
            putnbs(nodep, "VL_ASSIGN_W(");
            puts(cvtToStr(nodep->widthMin()) + ", ");
            iterateAndNextConstNull(nodep->lhsp());
            puts(", ");
        } નહીંતર જો (VN_IS(nodep->lhsp()->dtypep()->skipRefp(), QueueDType)
                   && (VN_IS(nodep->rhsp(), StreamL) || VN_IS(nodep->lhsp(), StreamL)
                       || VN_IS(nodep->rhsp(), StreamR) || VN_IS(nodep->lhsp(), StreamR)
                       || VN_IS(nodep->rhsp(), StreamR))) {
            //if either side is streamL or streamR don't emit lhsp everything will be passed by
            //reference

            paren = false;

        } નહીંતર {
            paren = false;
            iterateAndNextConstNull(nodep->lhsp());
            puts(" ");
            ofp()->blockInc();
            decind = true;
            જો (!VN_IS(nodep->rhsp(), Const)) ofp()->putBreak();
            putns(nodep, "= ");
        }
        જો (rhs) iterateAndNextConstNull(nodep->rhsp());
        જો (paren) puts(")");
        જો (decind) ofp()->blockDec();
        puts(";\n");
        જો (reverseUnpack) {
            puts("VL_UNPACK_REVERSED(");
            iterateAndNextConstNull(nodep->lhsp());
            puts(");\n");
        }
    }
    શૂન્ય visit(AstAssocSel* nodep) override {
        iterateAndNextConstNull(nodep->fromp());
        અચલ std::string atFunc = nodep->isLValue() ? ".atWrite(" : ".at(";
        putnbs(nodep, atFunc);
        AstAssocArrayDType* અચલ adtypep
            = VN_AS(nodep->fromp()->dtypep()->skipRefp(), AssocArrayDType);
        UASSERT_OBJ(adtypep, nodep, "Associative select on non-associative type");
        iterateAndNextConstNull(nodep->bitp());
        puts(")");
    }
    શૂન્ય visit(AstWildcardSel* nodep) override {
        iterateAndNextConstNull(nodep->fromp());
        અચલ std::string atFunc = nodep->isLValue() ? ".atWrite(" : ".at(";
        putnbs(nodep, atFunc);
        AstWildcardArrayDType* અચલ adtypep
            = VN_AS(nodep->fromp()->dtypep()->skipRefp(), WildcardArrayDType);
        UASSERT_OBJ(adtypep, nodep, "Wildcard select on non-wildcard-associative type");
        iterateAndNextConstNull(nodep->bitp());
        puts(")");
    }
    શૂન્ય visit(AstCCall* nodep) override {
        અચલ AstCFunc* અચલ funcp = nodep->funcp();
        અચલ Ast_ગાંઠ_Module* અચલ funcModp = EmitCParentModule::get(funcp);
        putnbs(nodep, "");
        જો (funcp->dpiImportPrototype()) {
            // Calling DPI import
            putns(nodep, funcp->name());
        } નહીંતર જો (funcp->isProperMethod() && funcp->isStatic()) {
            // Call static method via the containing class
            putns(funcModp, EmitCUtil::prefixNameProtect(funcModp) + "::");
            putns(nodep, funcp->nameProtect());
        } નહીંતર જો (nodep->superReference()) {
            // Calling superclass method
            putns(funcModp, EmitCUtil::prefixNameProtect(funcModp) + "::");
            putns(nodep, funcp->nameProtect());
        } નહીંતર જો (funcp->isLoose()) {
            // Calling loose method
            putns(nodep, funcNameProtect(funcp));
        } નહીંતર {
            // Calling regular method/function
            જો (!nodep->selfPointer().isEmpty()) {
                emitDereference(nodep, nodep->selfPointerProtect(m_useSelfForThis));
            }
            putns(nodep, funcp->nameProtect());
        }
        emitCCallArgs(nodep, nodep->selfPointerProtect(m_useSelfForThis), m_cfuncp->needProcess());
    }
    શૂન્ય visit(AstCMethodCall* nodep) override {
        અચલ AstCFunc* અચલ funcp = nodep->funcp();
        UASSERT_OBJ(!funcp->isLoose(), nodep, "Loose method called via AstCMethodCall");
        iterateConst(nodep->fromp());
        putnbs(nodep, "->");
        જો (nodep->superReference()) {
            // Super call, so the base implementation is used rather than the override
            અચલ Ast_ગાંઠ_Module* અચલ funcModp = EmitCParentModule::get(funcp);
            putns(funcModp, EmitCUtil::prefixNameProtect(funcModp) + "::");
        }
        putns(funcp, funcp->nameProtect());
        emitCCallArgs(nodep, "", m_cfuncp->needProcess());
    }
    શૂન્ય visit(AstCAwait* nodep) override {
        putns(nodep, "co_await ");
        iterateConst(nodep->exprp());
        puts(";\n");
    }
    શૂન્ય visit(AstCNew* nodep) override {
        જો (VN_IS(nodep->dtypep(), VoidDType)) {  // super.new case
            putsDecoration(nodep, "/*super.new*/");
            પાછા;
        }
        // assignment case;
        putns(nodep, "VL_NEW(" + EmitCUtil::prefixNameProtect(nodep->dtypep()) + ", "
                         + optionalProcArg(nodep->dtypep()) + "vlSymsp");
        putCommaIterateNext(nodep->argsp(), true);
        puts(")");
    }
    શૂન્ય visit(AstCMethodHard* nodep) override {
        iterateConst(nodep->fromp());
        putns(nodep, nodep->usePtr() ? "->" : ".");
        putns(nodep, nodep->name());
        જો (nodep->method() == VCMethod::FORCE_READ_SEL) {
            emitIQW(nodep);
            જો (nodep->isWide()) puts("<" + cvtToStr(nodep->dtypep()->width_શબ્દો()) + ">");
        } નહીંતર જો (nodep->method() == VCMethod::ARRAY_SLICE) {
            // VlUnpacked::slice<N_Out>(loIdx) - N_Out is the (fixed) result array size
            અચલ AstUnpackArrayDType* અચલ adtypep
                = VN_AS(nodep->dtypep()->skipRefp(), UnpackArrayDType);
            puts("<" + cvtToStr(adtypep->elementsConst()) + ">");
        } નહીંતર જો (nodep->method() == VCMethod::COVERGROUP_ADD_COVERPOINT) {
            // The hit-list bound is a template argument of the returned VlCoverpointT<>, and the
            // node's own dtype is that type, so it is the one source of truth for both.
            અચલ AstCoverpointDType* અચલ cpdtypep
                = VN_AS(nodep->dtypep()->skipRefp(), CoverpointDType);
            puts("<" + cvtToStr(cpdtypep->hitBound()) + ">");
        } નહીંતર જો (nodep->method() == VCMethod::COVERGROUP_ADD_CROSS) {
            અચલ AstCoverCrossDType* અચલ cxdtypep
                = VN_AS(nodep->dtypep()->skipRefp(), CoverCrossDType);
            puts("<" + cxdtypep->cppTemplateArgs() + ">");
        }
        puts("(");
        bool comma = false;
        પૂર્ણાંક arg_અંક = 0;
        માટે (Ast_ગાંઠ* subnodep = nodep->pinsp(); subnodep; subnodep = subnodep->nextp()) {
            જો (comma) puts(", ");
            જો (nodep->method() == VCMethod::RANDOMIZER_HARD && arg_અંક == 1) {
                // For RANDOMIZER_HARD's filename argument (2nd arg after constraint),
                // apply protect() similar to VL_STOP to handle --protected flag
                જો (અચલ AstCExpr* અચલ cexprp = VN_CAST(subnodep, CExpr)) {
                    // Extract filename from the CExpr (which contains "filename")
                    std::string filename;
                    માટે (અચલ Ast_ગાંઠ* textnodep = cexprp->nodesp(); textnodep;
                         textnodep = textnodep->nextp()) {
                        જો (અચલ AstText* અચલ textp = VN_CAST(textnodep, Text)) {
                            filename = textp->text();
                            તોડો;
                        }
                    }
                    // Remove surrounding quotes if present
                    જો (filename.size() >= 2 && filename.front() == '"'
                        && filename.back() == '"') {
                        filename = filename.substr(1, filename.size() - 2);
                    }
                    // Emit with protect()
                    putsQuoted(protect(filename));
                } નહીંતર {
                    iterateConst(subnodep);
                }
            } નહીંતર {
                iterateConst(subnodep);
            }
            comma = true;
            arg_અંક++;
        }
        જો (nodep->withp()) {
            જો (comma) puts(", ");
            iterateConst(nodep->withp());
        }
        puts(")");
    }
    શૂન્ય visit(AstCFuncHard* nodep) override {
        putns(nodep, nodep->name());
        puts("(");
        putCommaIterateNext(nodep->pinsp());
        puts(")");
    }
    શૂન્ય visit(AstLambdaArgRef* nodep) override { putbs(nodep->nameProtect()); }
    શૂન્ય visit(AstWith* nodep) override {
        // With uses a C++11 lambda
        putnbs(nodep, "[&](");
        જો (auto* અચલ argrefp = nodep->indexArgRefp()) {
            putnbs(argrefp, argrefp->dtypep()->cType(argrefp->nameProtect(), false, false));
            puts(", ");
        }
        જો (auto* અચલ argrefp = nodep->કિંમત_Arg_Refp()) {
            putnbs(argrefp, argrefp->dtypep()->cType(argrefp->nameProtect(), false, false));
        }
        puts(") -> ");
        putnbs(nodep, nodep->dtypep()->cType("", false, false));
        puts(" {\n");
        VL_RESTORER(m_createdScopeHash);
        iterateAndNextConstNull(nodep->exprp());
        puts("}\n");
    }
    શૂન્ય visit(AstCase* nodep) override {  // LCOV_EXCL_LINE
        // In V3Case...
        nodep->v3fatalSrc("Case statements should have been reduced out");
    }
    શૂન્ય visit(AstComment* nodep) override {
        string at;
        જો (nodep->showAt()) {
            at = " at " + nodep->fileline()->ascii();
            // If protecting, passthru less information about the design
            જો (!v3Global.opt.protectIds()) પાછા;
        }
        જો (!(nodep->protect() && v3Global.opt.protectIds())) {
            putsDecoration(nodep, "// "s + nodep->name() + at + "\n");
        }
        iterateChildrenConst(nodep);
    }
    શૂન્ય visit(AstCoverOtherDecl* nodep) override {
        emit_Cover_Other_Decl_ઉમેરો(nodep);
        જો (coverageUsesLocalCounter(nodep)) {
            // Some task bodies are cloned into a caller module where the
            // original instance-local counter is not addressable. Those clones
            // fall back to the global bin number, so register that slot too.
            emit_Cover_Other_Decl_ઉમેરો(nodep, true);
        }
    }
    શૂન્ય visit(AstCoverToggleDecl* nodep) override {
        putns(nodep, "vlSelf->__vlCoverToggleInsert(");  // As Declared in emitCoverageDecl
        puts(cvtToStr(nodep->range().જમણું()));
        puts(", ");
        puts(cvtToStr(nodep->range().ડાબું()));
        puts(", ");
        puts(cvtToStr(nodep->range().ranged()));
        puts(", ");
        putCoverageArray(nodep);
        puts(" + ");
        puts(cvtToStr(coverage_Bin_અંક(nodep)));
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
                   + VIdProtect::protect_શબ્દો_If(nodep->hier(), nodep->protect()));
        puts(", ");
        putsQuoted(VIdProtect::protect_શબ્દો_If(nodep->page(), nodep->protect()));
        puts(", ");
        putsQuoted(VIdProtect::protect_શબ્દો_If(nodep->comment(), nodep->protect()));
        puts(");\n");
    }
    શૂન્ય visit(AstCoverInc* nodep) override {
        જો (VN_IS(nodep->declp(), CoverOtherDecl)) {
            જો (v3Global.opt.threads() > 1) {
                putns(nodep, "");
                putCoverageArray(nodep->declp());
                puts("[");
                puts(cvtToStr(coverage_Bin_અંક(nodep->declp())));
                puts("].fetch_add(1, std::memory_order_relaxed);\n");
            } નહીંતર {
                putns(nodep, "++(");
                putCoverageArray(nodep->declp());
                puts("[");
                puts(cvtToStr(coverage_Bin_અંક(nodep->declp())));
                puts("]);\n");
            }
        } નહીંતર {
            puts("VL_COV_TOGGLE_CHG_");
            જો (v3Global.opt.threads() > 1) {
                puts("MT_");
            } નહીંતર {
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
            puts(cvtToStr(coverage_Bin_અંક(nodep->declp())));
            puts(", ");
            iterateConst(nodep->toggleExprp());
            puts(", ");
            iterateConst(nodep->toggleCovExprp());
            puts(");\n");
        }
    }
    શૂન્ય visit(AstDisableFork* nodep) override { putns(nodep, "vlProcess->disableFork();\n"); }
    શૂન્ય visit(AstCReturn* nodep) override {
        putns(nodep, "return (");
        iterateAndNextConstNull(nodep->lhsp());
        puts(");\n");
    }
    શૂન્ય visit(AstDisplay* nodep) override {
        string text = nodep->fmtp()->text();
        જો (nodep->addNewline()) text += "\n";
        display_ગાંઠ(nodep, nodep->fmtp(), text, nodep->fmtp()->exprsp(), false);
    }
    શૂન્ય visit(AstDumpCtl* nodep) override {
        પસંદગી (nodep->ctlType()) {
        સ્થિતિ VDumpCtlType::FILE:
            putns(nodep, "vlSymsp->_vm_contextp__->dumpfile(");
            iterateConst(nodep->exprp());
            puts(");\n");
            તોડો;
        સ્થિતિ VDumpCtlType::VARS:
            // We ignore number of levels to dump in exprp()
            જો (v3Global.opt.trace()) {
                putns(nodep, "vlSymsp->_traceDumpOpen();\n");
            } નહીંતર {
                putns(nodep, "VL_PRINTF_MT(\"-Info: ");
                puts(V3OutFormatter::quoteNameControls(protect(nodep->fileline()->filename())));
                puts(":");
                puts(cvtToStr(nodep->fileline()->lineno()));
                puts(": $dumpvar ignored, as Verilated without --trace");
                puts("\\n\");\n");
            }
            તોડો;
        સ્થિતિ VDumpCtlType::ALL:
            // $dumpall currently ignored
            તોડો;
        સ્થિતિ VDumpCtlType::FLUSH:
            // $dumpall currently ignored; would need rework of VCD single thread,
            // or flag we pass-through to next eval() iteration
            તોડો;
        સ્થિતિ VDumpCtlType::LIMIT:
            // $dumplimit currently ignored
            તોડો;
        સ્થિતિ VDumpCtlType::OFF:
            // Currently ignored as both Vcd and Fst do not support them, as would need "X" dump
            તોડો;
        સ્થિતિ VDumpCtlType::ON:
            // Currently ignored as $dumpoff is also ignored
            તોડો;
        મૂળભૂત: nodep->v3fatalSrc("Bad case, unexpected " << nodep->ctlType().ascii());
        }
    }
    શૂન્ય visit(AstScopeName* nodep) override {
        // For use under AstCCalls for dpiImports.  ScopeNames under
        // displays are handled in AstDisplay
        જો (!nodep->dpiExport()) {
            // this is where the DPI import context scope is set
            અચલ string scope = nodep->scopeDpiName();
            putnbs(nodep, "(vlSymsp->" + protect("__Vscopep_" + scope) + ")");
        }
    }
    શૂન્ય visit(AstSFormat* nodep) override {
        display_ગાંઠ(nodep, nodep->fmtp(), nodep->fmtp()->text(), nodep->fmtp()->exprsp(), false);
    }
    શૂન્ય visit(AstSFormatF* nodep) override {
        display_ગાંઠ(nodep, nodep, nodep->text(), nodep->exprsp(), false);
    }
    શૂન્ય visit(AstFScanF* nodep) override {
        display_ગાંઠ(nodep, nullptr, nodep->text(), nodep->exprsp(), true);
    }
    શૂન્ય visit(AstSScanF* nodep) override {
        display_ગાંઠ(nodep, nullptr, nodep->text(), nodep->exprsp(), true);
    }
    શૂન્ય visit(Ast_કિંમત_Plus_Args* nodep) override {
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
    શૂન્ય visit(AstTestPlusArgs* nodep) override {
        putns(nodep, "VL_TESTPLUSARGS_I(");
        iterateConst(nodep->searchp());
        puts(")");
    }
    શૂન્ય visit(AstFError* nodep) override {
        putns(nodep, "VL_FERROR_I");
        puts(nodep->strp()->isString() ? "N(" : "W(");
        iterateAndNextConstNull(nodep->filep());
        putbs(", ");
        જો (nodep->strp()->isWide()) {
            puts(cvtToStr(nodep->strp()->width_શબ્દો()));
            putbs(", ");
        }
        iterateAndNextConstNull(nodep->strp());
        puts(")");
    }
    શૂન્ય visit(AstFGetS* nodep) override {
        emitOpName(nodep, nodep->emitC(), nodep->strgp(), nodep->filep(), nullptr);
    }
    શૂન્ય visit(AstFOpen* nodep) override {
        putns(nodep, "VL_FOPEN_NN(");
        iterateConst(nodep->filenamep());
        putbs(", ");
        iterateConst(nodep->modep());
        puts(");\n");
    }
    શૂન્ય visit(AstFOpenMcd* nodep) override {
        putns(nodep, "VL_FOPEN_MCD_N(");
        iterateConst(nodep->filenamep());
        puts(");\n");
    }
    શૂન્ય visit(Ast_ગાંઠ_Read_Write_Mem* nodep) override {
        putns(nodep, nodep->cFuncPrefixp());
        puts("N(");
        puts(nodep->isHex() ? "true" : "false");
        putbs(", ");
        // Need real storage width
        puts(cvtToStr(nodep->memp()->dtypep()->skipRefp()->subDTypep()->widthMin()));
        uint32_t array_lo = 0;
        bool memUsePtr = true;
        {
            અચલ AstVarRef* અચલ varrefp = VN_CAST(nodep->memp(), VarRef);
            જો (!varrefp) {
                nodep->v3error(nodep->verilogKwd() << " loading non-variable");
            } નહીંતર જો (VN_IS(varrefp->varp()->dtypeSkipRefp(), AssocArrayDType)
                       || VN_IS(varrefp->varp()->dtypeSkipRefp(), DynArrayDType)) {
                // nodep->memp() below will when verilated code is compiled create a C++ template
                memUsePtr = false;
            } નહીંતર જો (અચલ AstUnpackArrayDType* અચલ adtypep
                       = VN_CAST(varrefp->varp()->dtypeSkipRefp(), UnpackArrayDType)) {
                putbs(", ");
                puts(cvtToStr(varrefp->varp()->dtypep()->arrayUnpackedElements()));
                array_lo = adtypep->lo();
                putbs(", ");
                puts(cvtToStr(array_lo));
            } નહીંતર {
                nodep->v3error(
                    nodep->verilogKwd()
                    << " loading other than unpacked/dynamic/associative-array variable");
            }
        }
        putbs(", ");
        iterateConst(nodep->filenamep());
        putbs(", ");
        {
            જો (memUsePtr) puts(" &(");
            iterateAndNextConstNull(nodep->memp());
            જો (memUsePtr) puts(")");
        }
        putbs(", ");
        જો (nodep->lsbp()) {
            iterateAndNextConstNull(nodep->lsbp());
        } નહીંતર {
            puts(cvtToStr(array_lo));
        }
        putbs(", ");
        જો (nodep->msbp()) {
            iterateAndNextConstNull(nodep->msbp());
        } નહીંતર {
            puts("~0ULL");
        }
        puts(");\n");
    }
    શૂન્ય visit(AstFClose* nodep) override {
        putns(nodep, "VL_FCLOSE_I(");
        iterateAndNextConstNull(nodep->filep());
        puts("); ");
    }
    શૂન્ય visit(AstFFlush* nodep) override {
        જો (!nodep->filep()) {
            putns(nodep, "VL_FFLUSH_MT();");
        } નહીંતર {
            putns(nodep, "VL_FFLUSH_I(");
            iterateAndNextConstNull(nodep->filep());
            puts(");\n");
        }
    }
    શૂન્ય visit(AstFSeek* nodep) override {
        putns(nodep, "(VL_FSEEK_I(");
        iterateAndNextConstNull(nodep->filep());
        puts(", ");
        iterateAndNextConstNull(nodep->offset());
        puts(", ");
        iterateAndNextConstNull(nodep->operation());
        puts(") == -1 ? -1 : 0)");
    }
    શૂન્ય visit(AstFTell* nodep) override {
        putns(nodep, "VL_FTELL_I(");
        iterateAndNextConstNull(nodep->filep());
        puts(")");
    }
    શૂન્ય visit(AstFRewind* nodep) override {
        putns(nodep, "(VL_FSEEK_I(");
        iterateAndNextConstNull(nodep->filep());
        puts(", 0, 0) == -1 ? -1 : 0)");
    }
    શૂન્ય visit(AstFRead* nodep) override {
        putns(nodep, "VL_FREAD_I(");
        puts(cvtToStr(nodep->memp()->widthMin()));  // Need real storage width
        putbs(", ");
        uint32_t array_lo = 0;
        uint32_t array_size = 0;
        {
            અચલ AstVarRef* અચલ varrefp = VN_CAST(nodep->memp(), VarRef);
            જો (!varrefp) {
                nodep->v3error(nodep->verilogKwd() << " loading non-variable");
            } નહીંતર જો (VN_CAST(varrefp->varp()->dtypeSkipRefp(), BasicDType)) {
            } નહીંતર જો (અચલ AstUnpackArrayDType* અચલ adtypep
                       = VN_CAST(varrefp->varp()->dtypeSkipRefp(), UnpackArrayDType)) {
                array_lo = adtypep->lo();
                array_size = adtypep->elementsConst();
            } નહીંતર {
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
        જો (nodep->startp()) {
            iterateAndNextConstNull(nodep->startp());
        } નહીંતર {
            puts(cvtToStr(array_lo));
        }
        putbs(", ");
        જો (nodep->countp()) {
            iterateAndNextConstNull(nodep->countp());
        } નહીંતર {
            puts(cvtToStr(array_size));
        }
        puts(")");
    }
    શૂન્ય visit(Ast_ઢગલો_Trace_F* nodep) override { putns(nodep, "VL_STACKTRACE_N()"); }
    શૂન્ય visit(Ast_ઢગલો_Trace_T* nodep) override { putns(nodep, "VL_STACKTRACE();\n"); }
    શૂન્ય visit(AstSystemT* nodep) override {
        putns(nodep, "(void)VL_SYSTEM_I");
        emitIQW(nodep->lhsp());
        puts("(");
        જો (nodep->lhsp()->isWide()) {
            puts(cvtToStr(nodep->lhsp()->width_શબ્દો()));
            putbs(", ");
        }
        iterateAndNextConstNull(nodep->lhsp());
        puts(");\n");
    }
    શૂન્ય visit(AstSystemF* nodep) override {
        putns(nodep, "VL_SYSTEM_I");
        emitIQW(nodep->lhsp());
        puts("(");
        જો (nodep->lhsp()->isWide()) {
            puts(cvtToStr(nodep->lhsp()->width_શબ્દો()));
            putbs(", ");
        }
        iterateAndNextConstNull(nodep->lhsp());
        puts(")");
    }
    શૂન્ય visit(Ast_Stmt_પદાવલિ* nodep) override {
        જો (VN_IS(nodep->exprp()->dtypep()->skipRefp(), VoidDType)) {
            putns(nodep, "");
        } નહીંતર {
            putns(nodep, "std::ignore = ");
        }
        iterateConst(nodep->exprp());
        puts(";\n");
    }
    શૂન્ય visit(AstJumpBlock* nodep) override {
        // Allocate label number
        અચલ size_t n = m_labelNumbers.size();
        અચલ bool newEntry = m_labelNumbers.emplace(nodep, n).second;
        UASSERT_OBJ(newEntry, nodep, "AstJumpBlock visited twide");
        // Emit
        putns(nodep, "{\n");  // Make it visually obvious label jumps outside these
        VL_RESTORER(m_createdScopeHash);
        iterateAndNextConstNull(nodep->stmtsp());
        puts("__Vlabel" + std::to_string(n) + ": ;\n");
        puts("}\n");
    }
    શૂન્ય visit(AstJumpGo* nodep) override {
        // Retrieve target label number - must already exist (from enclosing AstJumpBlock)
        અચલ size_t n = m_labelNumbers.at(nodep->blockp());
        // Emit
        putns(nodep, "goto __Vlabel" + std::to_string(n) + ";\n");
    }
    શૂન્ય visit(AstLoop* nodep) override {
        UASSERT_OBJ(!nodep->contsp(), nodep, "'contsp' only used before LinkJump");
        VL_RESTORER(m_createdScopeHash);
        // Special case when the AstLoopTest is first for output readability
        જો (AstLoopTest* અચલ testp = VN_CAST(nodep->stmtsp(), LoopTest)) {
            putns(nodep, "while (");
            iterateConst(testp->condp());
            puts(") {\n");
            iterateAndNextConstNull(testp->nextp());
            puts("}\n");
            પાછા;
        }
        // Special case when the AstLoopTest is last for output readability
        જો (Ast_ગાંઠ* lastp = nodep->stmtsp()) {
            જ્યારે (Ast_ગાંઠ* અચલ nextp = lastp->nextp()) lastp = nextp;
            જો (AstLoopTest* અચલ testp = VN_CAST(lastp, LoopTest)) {
                putns(nodep, "do {\n");
                માટે (Ast_ગાંઠ* p = nodep->stmtsp(); p != lastp; p = p->nextp()) iterateConst(p);
                puts("} while (");
                iterateConst(testp->condp());
                puts(");\n");
                પાછા;
            }
        }
        // Emit generic case directly
        putns(nodep, "while (true) {\n");
        iterateAndNextConstNull(nodep->stmtsp());
        puts("}\n");
    }
    શૂન્ય visit(AstLoopTest* nodep) override {
        VL_RESTORER(m_createdScopeHash);
        putns(nodep, "if (!(");
        iterateAndNextConstNull(nodep->condp());
        puts(")) break;\n");
    }
    શૂન્ય visit(AstCLocalScope* nodep) override {
        putns(nodep, "{\n");
        VL_RESTORER(m_createdScopeHash);
        iterateAndNextConstNull(nodep->stmtsp());
        puts("}\n");
    }
    શૂન્ય visit(Ast_ગાંઠ_If* nodep) override {
        putns(nodep, "if (");
        જો (!nodep->branchPred().unknown()) {
            puts(nodep->branchPred().ascii());
            puts("((");  // Two parens, so that VL_UNLIKELY((class<foo,bar>)) works
        }
        iterateAndNextConstNull(nodep->condp());
        જો (!nodep->branchPred().unknown()) puts("))");
        puts(") {\n");
        {
            VL_RESTORER(m_createdScopeHash);
            iterateAndNextConstNull(nodep->thensp());
        }
        puts("}");
        જો (!nodep->elsesp()) {
            puts("\n");
        } નહીંતર {
            જો (VN_IS(nodep->elsesp(), ગાંઠ_If) && !nodep->elsesp()->nextp()) {
                puts(" else ");
                iterateAndNextConstNull(nodep->elsesp());
            } નહીંતર {
                VL_RESTORER(m_createdScopeHash);
                puts(" else {\n");
                iterateAndNextConstNull(nodep->elsesp());
                puts("}\n");
            }
        }
    }
    શૂન્ય visit(Ast_પદાવલિ_Stmt* nodep) override {
        VL_RESTORER(m_createdScopeHash);
        અચલ bool containsAwait = nodep->exists([](AstCAwait*) -> bool { પાછા true; });
        જો (containsAwait) {
            UASSERT_OBJ(m_cfuncp && m_cfuncp->isCoroutine(), nodep,
                        "AstExprStmt with CAwait must be in coroutine");
            putnbs(nodep, "(co_await ([&]() -> VlCoroutine {\n");
            iterateAndNextConstNull(nodep->stmtsp());
            puts("co_return;\n");
            જો (!nodep->hasResult()) {
                puts("}()))");
                પાછા;
            }
            puts("}()), ");
            iterateAndNextConstNull(nodep->resultp());
            puts(")");
            પાછા;
        }
        // GCC allows compound statements in expressions, but this is not standard.
        // So we use an immediate-evaluation lambda and comma operator
        putnbs(nodep, "([&]() {\n");
        જો (!nodep->hasResult()) {
            iterateAndNextConstNull(nodep->stmtsp());
            puts("}())");
            પાછા;
        }
        iterateAndNextConstNull(nodep->stmtsp());
        puts("}(), ");
        iterateAndNextConstNull(nodep->resultp());
        puts(")");
    }
    શૂન્ય visit(AstStop* nodep) override {
        putns(nodep, "VL_STOP_MT(");
        putsQuoted(protect(nodep->fileline()->filename()));
        puts(", ");
        puts(cvtToStr(nodep->fileline()->lineno()));
        puts(", \"\"");
        જો (nodep->isFatal()) puts(", false");
        puts(");\n");
    }
    શૂન્ય visit(AstFinish* nodep) override {
        // Disable all the forks so they don't operate after simulation is finished.
        જો (m_cfuncp && m_cfuncp->needProcess()) putns(nodep, "vlProcess->disableFork();\n");
        putns(nodep, "VL_FINISH_MT(");
        putsQuoted(protect(nodep->fileline()->filename()));
        puts(", ");
        puts(cvtToStr(nodep->fileline()->lineno()));
        puts(", \"\");\n");
    }
    શૂન્ય visit(AstFinishFork* nodep) override {
        // Disable all the forks so they don't operate after simulation is finished.
        જો (m_cfuncp && m_cfuncp->needProcess()) putns(nodep, "vlProcess->disableFork();\n");
        putns(nodep, "VL_FINISH_MT(");
        putsQuoted(protect(nodep->fileline()->filename()));
        puts(", ");
        puts(cvtToStr(nodep->fileline()->lineno()));
        puts(", \"\");\n");
        જો (m_cfuncp->isCoroutine()) {
            putns(nodep, "co_return;\n");
        } નહીંતર {
            putns(nodep, "return;\n");
        }
    }
    શૂન્ય visit(AstPrintTimeScale* nodep) override {
        putns(nodep, "VL_PRINTTIMESCALE(");
        putsQuoted(protect(nodep->prettyName()));
        puts(", ");
        putsQuoted(nodep->timeunit().ascii());
        puts(", vlSymsp->_vm_contextp__);\n");
    }
    શૂન્ય visit(AstRand* nodep) override {
        emitOpName(nodep, nodep->emitC(), nodep->seedp(), nullptr, nullptr);
    }
    શૂન્ય visit(AstRandRNG* nodep) override {
        emitOpName(nodep, nodep->emitC(), nullptr, nullptr, nullptr);
    }
    શૂન્ય visit(AstTime* nodep) override {
        putns(nodep, "VL_TIME_UNITED_Q(");
        UASSERT_OBJ(!nodep->timeunit().isNone(), nodep, "$time has no units");
        અચલ દ્વિગુણ time
            = nodep->timeunit().multiplier() / v3Global.rootp()->timeprecision().multiplier();
        UASSERT_OBJ(time >= 1, nodep, "TimeQ is less than 1, will result in division by zero");
        puts(cvtToStr(time));
        puts(")");
    }
    શૂન્ય visit(AstTimeD* nodep) override {
        putns(nodep, "VL_TIME_UNITED_D(");
        UASSERT_OBJ(!nodep->timeunit().isNone(), nodep, "$realtime has no units");
        puts(cvtToStr(nodep->timeunit().multiplier()
                      / v3Global.rootp()->timeprecision().multiplier()));
        puts(")");
    }
    શૂન્ય visit(AstGetInitialRandomSeed* nodep) override {
        putns(nodep, "vlSymsp->_vm_contextp__->randSeed()");
    }
    શૂન્ય visit(AstTimeFormat* nodep) override {
        putns(nodep, "VL_TIMEFORMAT_IINI(");
        જો (nodep->unitsp()) {
            puts("true, ");
            iterateAndNextConstNull(nodep->unitsp());
        } નહીંતર {
            puts("false, 0");
        }
        puts(", ");
        જો (nodep->precisionp()) {
            puts("true, ");
            iterateAndNextConstNull(nodep->precisionp());
        } નહીંતર {
            puts("false, 0");
        }
        puts(", ");
        જો (nodep->suffixp()) {
            puts("true, ");
            iterateAndNextConstNull(nodep->suffixp());
        } નહીંતર {
            puts("false, \"\"");
        }
        puts(", ");
        જો (nodep->widthp()) {
            puts("true, ");
            iterateAndNextConstNull(nodep->widthp());
        } નહીંતર {
            puts("false, 0");
        }
        puts(", vlSymsp->_vm_contextp__);\n");
    }
    શૂન્ય visit(AstTimePrecision* nodep) override {
        putns(nodep, "vlSymsp->_vm_contextp__->timeprecision()");
    }

    // Nodes involing AstText
    શૂન્ય visit(AstText* nodep) override {
        // All Text should be under TextBlock/CStmt/CStmtUser/CExpr/CExprUser
        nodep->v3fatalSrc("Text node in unexpected position");
    }
    શૂન્ય visit(AstTextBlock* nodep) override {
        putnbs(nodep, "");
        puts(nodep->prefix());
        emitNodesWithText(nodep->nodesp(), false, true, nodep->separator());
        puts(nodep->suffix());
    }
    શૂન્ય visit(AstCStmt* nodep) override {
        putnbs(nodep, "");
        emitNodesWithText(nodep->nodesp(), false, true, "");
        ensure_New_લીટી();
    }
    શૂન્ય visit(AstCExpr* nodep) override {
        putnbs(nodep, "");
        emitNodesWithText(nodep->nodesp(), false, true, "");
    }
    શૂન્ય visit(AstCStmtUser* nodep) override {
        putnbs(nodep, "");
        ofp()->putsNoTracking("\n");
        જો (nodep->fromDollarC() && v3Global.opt.decoration() && !v3Global.opt.protectIds()) {
            ofp()->putsNoTracking("// $c statement at " + nodep->fileline()->ascii() + "\n");
        }
        emitNodesWithText(nodep->nodesp(), m_useSelfForThis, false, "");
        puts("\n");
    }
    શૂન્ય visit(AstCExprUser* nodep) override {
        putnbs(nodep, "");
        ofp()->putsNoTracking("\n");
        જો (/* is always from $c */ v3Global.opt.decoration() && !v3Global.opt.protectIds()) {
            ofp()->putsNoTracking(
                (nodep->isPure() ? "// $cpure expression at " : "// $c expression at ")
                + nodep->fileline()->ascii() + "\n");
        }
        emitNodesWithText(nodep->nodesp(), m_useSelfForThis, false, "");
        puts("\n");
    }

    // Operators
    શૂન્ય visit(Ast_ગાંઠ_Termop* nodep) override {
        emitOpName(nodep, nodep->emitC(), nullptr, nullptr, nullptr);
    }
    શૂન્ય visit(Ast_ગાંઠ_Uniop* nodep) override {
        જો (nodep->emit_Check_Max_શબ્દો()
            && (nodep->width_શબ્દો() > VL_MULS_MAX_શબ્દો
                || nodep->lhsp()->width_શબ્દો() > VL_MULS_MAX_શબ્દો)) {
            nodep->v3warn(
                E_UNSUPPORTED,
                "Unsupported: "
                    << nodep->prettyOperatorName() << " operator of " << nodep->width()
                    << " bits exceeds hardcoded limit VL_MULS_MAX_WORDS in verilatedos.h");
        }
        જો (emitSimpleOk(nodep)) {
            putnbs(nodep, "(");
            puts(nodep->emitSimpleOperator());
            puts(" ");
            iterateAndNextConstNull(nodep->lhsp());
            puts(")");
        } નહીંતર {
            emitOpName(nodep, nodep->emitC(), nodep->lhsp(), nullptr, nullptr);
        }
    }
    શૂન્ય visit(Ast_ગાંઠ_Biop* nodep) override {
        જો (nodep->emit_Check_Max_શબ્દો() && nodep->width_શબ્દો() > VL_MULS_MAX_શબ્દો) {
            nodep->v3warn(
                E_UNSUPPORTED,
                "Unsupported: "
                    << nodep->prettyOperatorName() << " operator of " << nodep->width()
                    << " bits exceeds hardcoded limit VL_MULS_MAX_WORDS in verilatedos.h");
        }
        જો (emitSimpleOk(nodep)) {
            putnbs(nodep, "(");
            iterateAndNextConstNull(nodep->lhsp());
            puts(" ");
            putbs(nodep->emitSimpleOperator());
            puts(" ");
            iterateAndNextConstNull(nodep->rhsp());
            puts(")");
        } નહીંતર {
            emitOpName(nodep, nodep->emitC(), nodep->lhsp(), nodep->rhsp(), nullptr);
        }
    }
    શૂન્ય visit(Ast_ગાંઠ_Triop* nodep) override {
        UASSERT_OBJ(!emitSimpleOk(nodep), nodep, "Triop cannot be described in a simple way");
        emitOpName(nodep, nodep->emitC(), nodep->lhsp(), nodep->rhsp(), nodep->thsp());
    }
    શૂન્ય visit(AstCvtPackString* nodep) override { emitCvtPackStr(nodep->lhsp()); }
    શૂન્ય visit(AstRedXor* nodep) override {
        જો (nodep->lhsp()->isWide()) {
            visit(static_cast<Ast_ગાંઠ_Uniop*>(nodep));
        } નહીંતર {
            અચલ AstVarRef* અચલ vrefp = VN_CAST(nodep->lhsp(), VarRef);
            અચલ પૂર્ણાંક widthPow2 = vrefp ? vrefp->varp()->dtypep()->widthPow2()
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
    શૂન્ય visit(AstCCast* nodep) override {
        // Extending a value of the same word width is just a NOP.
        જો (અચલ AstClassRefDType* અચલ classDtypep
            = VN_CAST(nodep->dtypep()->skipRefp(), ClassRefDType)) {
            putns(nodep, "(" + classDtypep->cType("", false, false) + ")(");
        } નહીંતર જો (nodep->size() <= VL_BYTESIZE) {
            putns(nodep, "(::CData)(");
        } નહીંતર જો (nodep->size() <= VL_SHORTSIZE) {
            putns(nodep, "(::SData)(");
        } નહીંતર જો (nodep->size() <= VL_IDATASIZE) {
            putns(nodep, "(::IData)(");
        } નહીંતર {
            putns(nodep, "(::QData)(");
        }
        iterateAndNextConstNull(nodep->lhsp());
        puts(")");
    }
    શૂન્ય visit(AstCond* nodep) override {
        // Widths match up already, so we'll just use C++'s operator w/o any temps.
        જો (nodep->thenp()->isWide()) {
            emitOpName(nodep, nodep->emitC(), nodep->condp(), nodep->thenp(), nodep->elsep());
        } નહીંતર {
            putnbs(nodep, "(");
            iterateAndNextConstNull(nodep->condp());
            putbs(" ? ");
            iterateAndNextConstNull(nodep->thenp());
            putbs(" : ");
            iterateAndNextConstNull(nodep->elsep());
            puts(")");
        }
    }
    શૂન્ય visit(AstMatchMasked* nodep) override {
        emitOpName(nodep, nodep->emitC(), nodep->lhsp(), nodep->matchp(), nullptr);
    }
    શૂન્ય visit(AstMemberSel* nodep) override {
        અચલ AstVar* અચલ varp = nodep->varp();
        અચલ bool dereferenceCovergroupRef
            = varp->covergroupRefMember() && nodep->access().isReadOrRW();
        જો (dereferenceCovergroupRef) putnbs(nodep, "(*");
        iterateAndNextConstNull(nodep->fromp());
        putnbs(nodep, "->");
        જો (varp->isIfaceRef()) {
            // varp is the __Viftop companion (e.g. "tx__Viftop"); use the
            // MemberSel name which matches the cell's C++ member (e.g. "tx").
            puts(nodep->nameProtect());
        } નહીંતર {
            puts(varName(varp));
        }
        જો (dereferenceCovergroupRef) puts(")");
    }
    શૂન્ય visit(AstStructSel* nodep) override {
        iterateAndNextConstNull(nodep->fromp());
        putnbs(nodep, ".");
        puts(nodep->nameProtect());
    }
    શૂન્ય visit(AstNullCheck* nodep) override {
        putns(nodep, "VL_NULL_CHECK(");
        iterateAndNextConstNull(nodep->lhsp());
        puts(", ");
        putsQuoted(protect(nodep->fileline()->filename()));
        puts(", ");
        puts(cvtToStr(nodep->fileline()->lineno()));
        puts(")");
    }
    શૂન્ય visit(AstNewCopy* nodep) override {
        // Polymorphic shallow clone: preserves runtime type via virtual clone()
        // VL_NULL_CHECK enforces null check per IEEE 1800-2023 8.7
        putns(nodep, "VL_NULL_CHECK(");
        જો (VN_IS(nodep->rhsp(), Const) && VN_AS(nodep->rhsp(), Const)->isNull()) {
            // V3Const folded rhs to null: emit a typed empty ref so VL_NULL_CHECK fires
            અચલ AstClassRefDType* અચલ refDTypep
                = VN_CAST(nodep->dtypep()->skipRefp(), ClassRefDType);
            puts(refDTypep->cType("", false, false) + "{}");
        } નહીંતર {
            iterateAndNextConstNull(nodep->rhsp());
        }
        puts(", ");
        putsQuoted(protect(nodep->fileline()->filename()));
        puts(", ");
        puts(cvtToStr(nodep->fileline()->lineno()));
        puts(").clone(vlSymsp->__Vm_deleter)");
    }
    શૂન્ય visit(AstSel* nodep) override {
        // Note ASSIGN checks for this on a LHS
        UASSERT_OBJ(nodep->widthMin() == nodep->widthConst(), nodep, "Width mismatch");
        emitOpName(nodep, nodep->emitC(), nodep->fromp(), nodep->lsbp(), nullptr);
    }
    શૂન્ય visit(AstReplicate* nodep) override {
        જો (nodep->srcp()->widthMin() == 1 && !nodep->isWide()) {
            UASSERT_OBJ((static_cast<પૂર્ણાંક>(VN_AS(nodep->countp(), Const)->toUInt())
                         * nodep->srcp()->widthMin())
                            == nodep->widthMin(),
                        nodep, "Replicate non-constant or width miscomputed");
            putns(nodep, "VL_REPLICATE_");
            emitIQW(nodep);
            puts("OI(");
            જો (nodep->srcp()) puts(cvtToStr(nodep->srcp()->widthMin()));
            puts(", ");
            iterateAndNextConstNull(nodep->srcp());
            puts(", ");
            iterateAndNextConstNull(nodep->countp());
            puts(")");
        } નહીંતર {
            emitOpName(nodep, nodep->emitC(), nodep->srcp(), nodep->countp(), nullptr);
        }
    }
    શૂન્ય emitStreamR(AstStreamR* nodep, Ast_ગાંઠ* parent) {
        //TODO: This might need to handle more cases like the visit(AstStreamR) function
        emitOpName(nodep, nodep->emitC(), nodep->lhsp(), nodep->rhsp(), nullptr);
    }
    શૂન્ય visit(AstStreamR* nodep) override {
        //The parrent node of our AstStreamR will give just enough info for what streamR should
        //output if nodep->backp() is not the parent then emitStreamR should have been used. throw
        //an error
        bool backpIsParent = (nodep->backp()->op1p() == nodep || nodep->backp()->op2p() == nodep);
        UASSERT(backpIsParent, "can not find return type for streamR");
        જો (અચલ AstQueueDType* અચલ qdtp
            = VN_CAST(nodep->backp()->dtypep()->skipRefp(), QueueDType)) {
            જો (nodep->lhsp()->isWide()) {
                emitOpName(nodep,
                           "VL_STREAMR_%nq%lq%rq(%lw," + std::to_string(qdtp->subDTypep()->width())
                               + ", %P, %li, %ri)",
                           nodep->lhsp(), nodep->rhsp(), nullptr);
            } નહીંતર {
                emitOpName(nodep, "VL_STREAMR_%nq%lq%rq(%lw, %P, %li, %ri)", nodep->lhsp(),
                           nodep->rhsp(), nullptr);
            }
        } નહીંતર જો (VN_IS(nodep->lhsp()->dtypep()->skipRefp(), QueueDType)) {
            જો (!((nodep->backp()->op1p() && nodep->backp()->op1p()->isWide())
                  || (nodep->backp()->op2p() && nodep->backp()->op2p()->isWide()))) {
                //If our lhsp is a queue make sure we streamR and return the correct type.
                //If either side is wide or the previous node is string type dont use this case
                emitOpName(nodep->backp(), "VL_STREAMR_%nq%lq%rq(%lw, %P, %li, %ri)",
                           nodep->lhsp(), nodep->rhsp(), nullptr);
            } નહીંતર {
                emitOpName(nodep, nodep->emitC(), nodep->lhsp(), nodep->rhsp(), nullptr);
            }
        } નહીંતર {
            emitOpName(nodep, nodep->emitC(), nodep->lhsp(), nodep->rhsp(), nullptr);
        }
    }
    શૂન્ય visit(AstStreamL* nodep) override {
        // Attempt to use a "fast" stream function for slice size = power of 2
        જો (!nodep->isWide()) {
            અચલ uint32_t isPow2 = VN_AS(nodep->rhsp(), Const)->અંક().ગણતરી_Ones() == 1;
            અચલ uint32_t sliceSize = VN_AS(nodep->rhsp(), Const)->toUInt();
            જો (isPow2 && sliceSize <= (nodep->isQuad() ? કદ(uint64_t) : કદ(uint32_t))) {
                putns(nodep, "VL_STREAML_FAST_");
                bool usesQueue = false;
                AstQueueDType* qtypep
                    = nodep->backp()->op2p()
                          ? VN_CAST(nodep->backp()->op2p()->dtypep()->skipRefp(), QueueDType)
                          : nullptr;
                જો (VN_IS(nodep->backp(), Assign)
                    && qtypep) {  // If we are assigning to a queue then emit the correct symbol
                    puts("R");  // R for queue
                    usesQueue = true;
                } નહીંતર {
                    emitIQW(nodep);
                }
                emitIQW(nodep->lhsp());
                puts("I(");
                puts(cvtToStr(nodep->lhsp()->widthMin()));
                puts(", ");
                જો (usesQueue) {
                    iterateAndNextConstNull(nodep->backp()->op2p());
                    puts(", ");
                }
                iterateAndNextConstNull(nodep->lhsp());
                puts(", ");
                અચલ uint32_t rd_log2 = V3_સંખ્યા::log2b(VN_AS(nodep->rhsp(), Const)->toUInt());
                puts(cvtToStr(rd_log2) + ")");
                પાછા;
            }
        }
        જો (VN_IS(nodep->backp(), Assign)
            && VN_IS(nodep->backp()->op2p()->dtypep()->skipRefp(), QueueDType)) {
            પૂર્ણાંક queueWidth
                = nodep->backp()->op2p()->dtypep()->subDTypep()->width();  //We need to know the
                                                                           //width of both sides
            emitOpName(nodep,
                       "VL_STREAML_%nq%lq%rq(%lw," + std::to_string(queueWidth)
                           + ", %P, %li, %ri)",
                       nodep->lhsp(), nodep->rhsp(), nullptr);
        } નહીંતર {
            emitOpName(nodep, nodep->emitC(), nodep->lhsp(), nodep->rhsp(), nullptr);
        }
    }
    શૂન્ય visit(AstCastDynamic* nodep) override {
        putnbs(nodep, "VL_CAST_DYNAMIC(");
        iterateAndNextConstNull(nodep->fromp());
        puts(", ");
        iterateAndNextConstNull(nodep->ટોચ());
        puts(")");
    }
    શૂન્ય visit(Ast_ગણતરી_Bits* nodep) override {
        putnbs(nodep, "VL_COUNTBITS_");
        emitIQW(nodep->lhsp());
        puts("(");
        puts(cvtToStr(nodep->lhsp()->widthMin()));
        puts(", ");
        જો (nodep->lhsp()->isWide()) {
            puts(cvtToStr(nodep->lhsp()->width_શબ્દો()));  // Note argument width, not node width
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
    શૂન્ય visit(AstTriggered* nodep) override {
        // Test each word of the trigger vector with a mask of the relevant bits
        std::map<uint32_t, uint64_t> wordMasks;
        માટે (અચલ uint32_t index : nodep->indices()) {
            wordMasks[VL_BITWORD_Q(index)] |= VL_BIT_Q(index);
        }
        putns(nodep, "((");
        bool first = true;
        માટે (અચલ auto& wordMask : wordMasks) {
            જો (!first) puts("\n| ");
            first = false;
            અચલ bool allOnes = wordMask.second == ~0ULL;
            જો (!allOnes) puts("(" + V3_સંખ્યા{nodep, 64}.setQuad(wordMask.second).emitC() + " & ");
            iterateConst(nodep->trigRefp());
            puts("[" + std::to_string(wordMask.first) + "U]");
            જો (!allOnes) puts(")");
        }
        puts(") != 0ULL)");
    }
    // Terminals
    શૂન્ય visit(AstVarRef* nodep) override {
        અચલ AstVar* અચલ varp = nodep->varp();

        // Reference to constant pool variable is special
        જો (varp->constPoolEntry()) {
            putns(nodep, EmitCUtil::constPoolName(varp));
            પાછા;
        }

        અચલ Ast_ગાંઠ_Module* અચલ varModp = EmitCParentModule::get(varp);
        અચલ bool dereferenceCovergroupRef
            = varp->covergroupRefMember() && nodep->access().isReadOrRW();
        જો (dereferenceCovergroupRef) putns(nodep, "(*");
        જો (VN_IS(varModp, Class) && varModp != m_modp) {
            // Superclass member reference
            putns(nodep, EmitCUtil::prefixNameProtect(varModp) + "::");
        } નહીંતર જો (varp->isIfaceRef()) {
            putns(nodep, nodep->selfPointerProtect(m_useSelfForThis));
            પાછા;
        } નહીંતર જો (!nodep->selfPointer().isEmpty()) {
            emitDereference(nodep, nodep->selfPointerProtect(m_useSelfForThis));
        }
        putns(nodep, varName(varp));
        જો (dereferenceCovergroupRef) puts(")");
    }
    શૂન્ય visit(AstAddrOfCFunc* nodep) override {
        // Note: Can be thought to handle more, but this is all that is needed right now
        અચલ AstCFunc* અચલ funcp = nodep->funcp();
        UASSERT_OBJ(funcp->isLoose(), nodep, "Cannot take address of non-loose method");
        putns(nodep, "&");
        puts(funcNameProtect(funcp));
    }
    શૂન્ય visit(AstConst* nodep) override {  //
        જો (m_wideTempRefp && nodep->isWide()) {
            UASSERT_OBJ(m_wideTempRefp, nodep, "Wide Constant w/ no temp");
            emitConstantW(nodep, m_wideTempRefp);
            m_wideTempRefp = nullptr;  // We used it, fail if set it a second time
        } નહીંતર {
            emitConstant(nodep);
        }
    }
    શૂન્ય visit(Ast_ગાંઠ_Cons* nodep) override { emitValCons(nodep); }
    શૂન્ય visit(AstThisRef* nodep) override {
        putnbs(nodep, nodep->dtypep()->cType("", false, false));
        puts("{");
        puts(VSelfPointerText::replaceThis(m_useSelfForThis, "this"));
        puts("}");
    }
    શૂન્ય visit(Ast_ગાંઠ_Sel* nodep) override {
        જો (!VN_IS(nodep, ArraySel) && !VN_IS(nodep, WordSel)) {
            visit(static_cast<Ast_ગાંઠ_Biop*>(nodep));
            પાછા;
        }
        // ArraySel or WordSel
        iterateAndNextConstNull(nodep->fromp());
        // Special case constant index for readability
        જો (AstConst* અચલ idxp = VN_CAST(nodep->bitp(), Const)) {
            puts("[" + std::to_string(idxp->toUInt()) + "U]");
            પાછા;
        }
        putbs("[");
        iterateAndNextConstNull(nodep->bitp());
        puts("]");
    }

    //
    શૂન્ય visit(AstExecGraph* nodep) override {
        // The location of the AstExecGraph within the containing AstCFunc is where we want to
        // invoke the graph and wait for it to complete. The run-time library does just that.
        putns(nodep, "vl_invokeExecGraph(" + EmitCUtil::execGraphName(nodep) + ", vlSelf, ");
        iterateConst(nodep->triggerp());
        puts(".data());\n");
    }

    // Default
    શૂન્ય visit(Ast_ગાંઠ* nodep) override {  // LCOV_EXCL_START
        putns(nodep, "\n???? // "s + nodep->prettyTypeName() + "\n");
        iterateChildrenConst(nodep);
        જો (!v3Global.opt.lintOnly()) {  // An internal problem, so suppress
            nodep->v3fatalSrc("Unknown node type reached emitter: " << nodep->prettyTypeName());
        }
    }  // LCOV_EXCL_STOP

સુરક્ષિત:
    EmitCFunc() = મૂળભૂત;
    ~EmitCFunc() override = મૂળભૂત;
};

#endif  // guard
