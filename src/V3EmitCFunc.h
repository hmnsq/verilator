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

#অন্তর্ভাব "config_build.h"
#অন্তর্ভাব "verilatedos.h"

#অন্তর্ভাব "V3EmitCConstInit.h"
#অন্তর্ভাব "V3Global.h"
#অন্তর্ভাব "V3MemberMap.h"

#অন্তর্ভাব <algorithm>
#অন্তর্ভাব <map>
#অন্তর্ভাব <unordered_map>
#অন্তর্ভাব <unordered_set>
#অন্তর্ভাব <vector>

// Number of VL_CONST_W_*X's in verilated.h (IE VL_CONST_W_8X is last)
constexpr পূর্ণাঙ্ক EMITC_অঙ্ক_CONSTW = 8;

//######################################################################
// Emit lazy forward declarations

class EmitCLazyDecls final : সার্বজনিক VNVisitorConst {
    // NODE STATE/TYPES
    // None allowed to support threaded emitting

    // MEMBERS
    std::unordered_set<string> m_emittedManually;  // Set of names already declared manually.
    EmitCBaseVisitorConst& m_emitter;  // For access to file output
    bool m_needs_Blank_সারি = false;  // Emit blank line if any declarations were emitted (cosmetic)
    std::set<Ast_গ্রন্থি*> m_emitted;  // -> in set. Already emitted decl for symbols.

    // METHODS
    bool declaredOnce(Ast_গ্রন্থি* nodep) { প্রত্যাগম m_emitted.ঢোকাও(nodep).second; }

    শূন্য lazyDeclare(AstCFunc* funcp) {
        // Already declared in this compilation unit
        যদি (!declaredOnce(funcp)) প্রত্যাগম;
        // Check if this kind of function is lazily declared
        যদি (!(funcp->isMethod() && funcp->isLoose()) && !funcp->dpiImportPrototype()) প্রত্যাগম;
        // Already declared manually
        যদি (m_emittedManually.গুনতি(funcp->nameProtect())) প্রত্যাগম;
        // Needs lazy declaration, emit one
        m_emitter.emitCFuncDecl(funcp, EmitCParentModule::get(funcp), funcp->dpiImportPrototype());
        m_needs_Blank_সারি = true;
    }

    শূন্য lazyDeclareConstPoolVar(AstVar* varp) {
        যদি (!declaredOnce(varp)) প্রত্যাগম;  // Already declared
        ধ্রুব string nameProtect = EmitCUtil::constPoolName(varp);
        m_emitter.putns(varp, "extern const ");
        m_emitter.puts(varp->dtypep()->cType(nameProtect, false, false));
        m_emitter.puts(";\n");
        m_needs_Blank_সারি = true;
    }

    // VISITORS
    শূন্য visit(Ast_গ্রন্থি_CCall* nodep) override {
        lazyDeclare(nodep->funcp());
        iterateChildrenConst(nodep);
    }

    শূন্য visit(AstAddrOfCFunc* nodep) override {
        lazyDeclare(nodep->funcp());
        iterateChildrenConst(nodep);
    }

    শূন্য visit(AstVarRef* nodep) override {
        AstVar* ধ্রুব varp = nodep->varp();
        // Only constant pool symbols are lazy declared for now ...
        যদি (EmitCParentModule::get(varp)->isConstPool()) lazyDeclareConstPoolVar(varp);
    }

    শূন্য visit(AstExecGraph* nodep) override {
        যদি (!declaredOnce(nodep)) প্রত্যাগম;
        m_emitter.putns(nodep, "extern VlExecGraph " + EmitCUtil::execGraphName(nodep) + ";\n");
        m_needs_Blank_সারি = true;
    }

    শূন্য visit(Ast_গ্রন্থি* nodep) override { iterateChildrenConst(nodep); }

সার্বজনিক:
    explicit EmitCLazyDecls(EmitCBaseVisitorConst& emitter)
        : m_emitter{emitter} {}
    শূন্য emit(Ast_গ্রন্থি* nodep) {
        m_needs_Blank_সারি = false;
        iterateChildrenConst(nodep);
        যদি (m_needs_Blank_সারি) m_emitter.puts("\n");
    }
    শূন্য emit(ধ্রুব string& prefix, ধ্রুব string& name, ধ্রুব string& suffix) {
        m_emittedManually.ঢোকাও(name);
        m_emitter.ensure_New_সারি();
        m_emitter.puts(prefix);
        m_emitter.puts(name);
        m_emitter.puts(suffix);
        m_emitter.ensure_New_সারি();
    }
    শূন্য declared(AstCFunc* nodep) { m_emitted.ঢোকাও(nodep); }
    শূন্য reset() { m_emitted.clear(); }
};

// ######################################################################
//  Emit statements and expressions

class EmitCFunc VL_NOT_FINAL : সার্বজনিক EmitCConstInit {
    VMemberMap m_memberMap;
    AstVarRef* m_wideTempRefp = nullptr;  // Variable that _WW macros should be setting
    std::unordered_map<AstJumpBlock*, size_t> m_labelNumbers;  // Label numbers for AstJumpBlocks
    bool m_createdScopeHash = false;  // Already created a scope hash

রক্ষিত:
    VL_DEFINE_DEBUG_FUNCTIONS;

    EmitCLazyDecls m_lazyDecls{*this};  // Visitor for emitting lazy declarations
    bool m_useSelfForThis = false;  // Replace "this" with "vlSelf"
    bool m_usevlSelfRef = false;  // Use vlSelfRef reference instead of vlSelf pointer
    ধ্রুব Ast_গ্রন্থি_Module* m_modp = nullptr;  // Current module being emitted
    ধ্রুব AstCFunc* m_cfuncp = nullptr;  // Current function being emitted
    bool m_instantiatesOwnProcess = false;
    ধ্রুব AstClassPackage* m_classOrPackage = nullptr;  // Pointer to current class or package
    string m_classOrPackageHash;  // Hash of class or package name

    bool constructorNeedsProcess(ধ্রুব AstClass* ধ্রুব classp) {
        ধ্রুব Ast_গ্রন্থি* ধ্রুব newp = m_memberMap.findMember(classp, "new");
        যদি (!newp) প্রত্যাগম false;
        ধ্রুব AstCFunc* ধ্রুব ctorp = VN_CAST(newp, CFunc);
        যদি (!ctorp) প্রত্যাগম false;
        UASSERT_OBJ(ctorp->isConstructor(), ctorp, "`new` is not a constructor!");
        প্রত্যাগম ctorp->needProcess();
    }

    bool constructorNeedsProcess(ধ্রুব Ast_গ্রন্থি_DType* ধ্রুব dtypep) {
        যদি (ধ্রুব AstClassRefDType* ধ্রুব crefdtypep = VN_CAST(dtypep, ClassRefDType))
            প্রত্যাগম constructorNeedsProcess(crefdtypep->classp());
        প্রত্যাগম false;
    }

    bool coverageUsesLocalCounter(Ast_গ্রন্থি_Cover_Decl* ধ্রুব declp) ধ্রুব {
        // Only functions with an object receiver can address the module-local
        // coverage array. Static/package/class helper functions still use the
        // symbol-table array, as they may have vlSymsp but no vlSelf/this.
        // Also require the coverage declaration to belong to the current
        // module; cloned task bodies can increment declarations owned by a
        // different module, and those must keep using the global array.
        প্রত্যাগম m_cfuncp && !m_cfuncp->isStatic() && !VN_IS(m_modp, Class)
               && !VN_IS(m_modp, ClassPackage)
               && EmitCParentModule::get(declp->dataDeclThisp()) == m_modp;
    }
    পূর্ণাঙ্ক coverage_Bin_অঙ্ক(Ast_গ্রন্থি_Cover_Decl* ধ্রুব declp, bool forceGlobal = false) ধ্রুব {
        প্রত্যাগম !forceGlobal && coverageUsesLocalCounter(declp)
                   ? declp->dataDeclThisp()->local_Bin_অঙ্ক()
                   : declp->dataDeclThisp()->bin_অঙ্ক();
    }
    শূন্য putCoverageArray(Ast_গ্রন্থি_Cover_Decl* ধ্রুব declp, bool forceGlobal = false) {
        যদি (!forceGlobal && coverageUsesLocalCounter(declp)) {
            // Keep counters on the module object when possible, so every
            // no-inline instance has storage available if coverage output is
            // later written with forcePerInstance.
            puts(m_cfuncp->isLoose() ? "vlSelf->__Vcoverage" : "this->__Vcoverage");
        } অন্যথা {
            // Static/package/class helper functions do not have vlSelf, so
            // keep using the shared symbol-table coverage array there.
            puts("vlSymsp->__Vcoverage");
        }
    }
    শূন্য emit_Cover_Other_Decl_ঢোকাও(AstCoverOtherDecl* nodep, bool forceGlobal = false) {
        putns(nodep, "vlSelf->__vlCoverInsert(");  // As Declared in emitCoverageDecl
        putCoverageArray(nodep, forceGlobal);
        puts(" + ");
        puts(cvtToStr(coverage_Bin_অঙ্ক(nodep, forceGlobal)));
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
                   + VIdProtect::protect_শব্দগুলি_If(nodep->hier(), nodep->protect()));
        puts(", ");
        putsQuoted(VIdProtect::protect_শব্দগুলি_If(nodep->page(), nodep->protect()));
        puts(", ");
        putsQuoted(VIdProtect::protect_শব্দগুলি_If(nodep->comment(), nodep->protect()));
        puts(", ");
        putsQuoted(nodep->linescov());
        puts(", ");
        putsQuoted(VIdProtect::protect_শব্দগুলি_If(nodep->fsmVar(), nodep->protect()));
        puts(", ");
        putsQuoted(VIdProtect::protect_শব্দগুলি_If(nodep->fsmFrom(), nodep->protect()));
        puts(", ");
        putsQuoted(VIdProtect::protect_শব্দগুলি_If(nodep->fsmTo(), nodep->protect()));
        puts(", ");
        putsQuoted(VIdProtect::protect_শব্দগুলি_If(nodep->fsmTag(), nodep->protect()));
        puts(");\n");
    }

সার্বজনিক:
    // METHODS
    bool displayEmitHeader(Ast_গ্রন্থি* nodep);
    শূন্য display_গ্রন্থি(Ast_গ্রন্থি* nodep, AstSFormatF* fmtp, ধ্রুব string& vformat, Ast_গ্রন্থি* exprsp,
                     bool isScan);

    bool emitSimpleOk(Ast_গ্রন্থি_রাশিমালা* nodep);
    শূন্য emitIQW(ধ্রুব Ast_গ্রন্থি* nodep) {
        // See "Type letters" in verilated.h
        // Other abbrevs: "C"har, "S"hort, "F"loat, "D"ouble, stri"N"g, "R"=queue, "U"npacked
        puts(nodep->dtypep()->skipRefp()->charIQWN());
    }
    শূন্য emitRU(ধ্রুব Ast_গ্রন্থি* nodep) {
        ধ্রুব Ast_গ্রন্থি_DType* dtp = nodep->dtypep()->skipRefp();
        // See "Type letters" in verilated.h
        যদি (VN_IS(dtp, UnpackArrayDType))
            puts("U");
        অন্যথা যদি (VN_IS(dtp, QueueDType) || VN_IS(dtp, DynArrayDType))
            puts("R");
    }
    শূন্য emitScIQW(AstVar* nodep) {
        UASSERT_OBJ(nodep->isSc(), nodep, "emitting SystemC operator on non-SC variable");
        puts(nodep->isScBigUint()                           ? "SB"
             : (nodep->isScUint() || nodep->isScUintBool()) ? "SU"
             : nodep->isScBv()                              ? "SW"
             : nodep->isDouble()                            ? "SD"
                                                            : (nodep->isScQuad() ? "SQ" : "SI"));
    }
    শূন্য emitDatap(ধ্রুব Ast_গ্রন্থি* nodep) {
        // When passing to a function with va_args the compiler doesn't
        // know need a pointer so when wide, need to look inside VlWide
        যদি (nodep->isWide()) puts(".data()");
    }
    শূন্য emitOpName(Ast_গ্রন্থি* nodep, ধ্রুব string& format, Ast_গ্রন্থি* lhsp, Ast_গ্রন্থি* rhsp,
                    Ast_গ্রন্থি* thsp);
    শূন্য emitCCallArgs(ধ্রুব Ast_গ্রন্থি_CCall* nodep, ধ্রুব string& selfPointer, bool inProcess);
    শূন্য emitDereference(Ast_গ্রন্থি* nodep, ধ্রুব string& pointer);
    std::string dereferenceString(ধ্রুব std::string& pointer) ধ্রুব;
    শূন্য emitCvtPackStr(Ast_গ্রন্থি* nodep);
    শূন্য emitConstant(AstConst* nodep);
    শূন্য emitConstantString(ধ্রুব AstConst* nodep);
    শূন্য emitVarReset(ধ্রুব string& prefix, AstVar* varp, bool constructing);
    string emitVarResetRecurse(ধ্রুব AstVar* varp, bool constructing,
                               ধ্রুব string& varNameProtected, Ast_গ্রন্থি_DType* dtypep, পূর্ণাঙ্ক depth,
                               ধ্রুব string& suffix, ধ্রুব Ast_গ্রন্থি* valuep);
    শূন্য emitVarResetScopeHash();
    শূন্য emitChangeDet();
    শূন্য putCommaIterateNext(Ast_গ্রন্থি* nodep, bool comma = false) {
        প্রতি (Ast_গ্রন্থি* subnodep = nodep; subnodep; subnodep = subnodep->nextp()) {
            যদি (comma) puts(", ");
            iterateConst(subnodep);
            comma = true;
        }
    }
    template <typename T>
    string optionalProcArg(ধ্রুব T* ধ্রুব nodep) {
        প্রত্যাগম (nodep && constructorNeedsProcess(nodep)) ? "vlProcess, " : "";
    }
    ধ্রুব AstCNew* getSuperNewCallRecursep(Ast_গ্রন্থি* ধ্রুব nodep) {
        // Get the super.new call
        যদি (!nodep) প্রত্যাগম nullptr;
        যদি (ধ্রুব AstCNew* ধ্রুব cnewp = VN_CAST(nodep, CNew)) প্রত্যাগম cnewp;
        যদি (ধ্রুব Ast_Stmt_রাশিমালা* ধ্রুব stmtp = VN_CAST(nodep, Stmt_রাশিমালা)) {
            যদি (ধ্রুব AstCNew* ধ্রুব cnewp = VN_CAST(stmtp->exprp(), CNew)) প্রত্যাগম cnewp;
        }
        যদি (ধ্রুব AstJumpBlock* ধ্রুব blockp = VN_CAST(nodep, JumpBlock)) {
            যদি (ধ্রুব AstCNew* ধ্রুব cnewp = getSuperNewCallRecursep(blockp->stmtsp())) {
                প্রত্যাগম cnewp;
            }
        }
        যদি (ধ্রুব AstCNew* ধ্রুব cnewp = getSuperNewCallRecursep(nodep->nextp())) প্রত্যাগম cnewp;
        প্রত্যাগম nullptr;
    }

    শূন্য emitConstantW(AstConst* nodep, AstVarRef* assigntop) {
        // For tradition and compilation speed, assign each word directly into
        // output variable instead of using '='
        putns(nodep, "");
        যদি (nodep->অঙ্ক().isFourState()) {
            nodep->v3warn(E_UNSUPPORTED, "Unsupported: 4-state numbers in this context");
            প্রত্যাগম;
        }

        পূর্ণাঙ্ক upWidth = nodep->অঙ্ক().widthToFit();
        পূর্ণাঙ্ক chunks = 0;
        যদি (upWidth > EMITC_অঙ্ক_CONSTW * VL_EDATASIZE) {
            // Output e.g. 8 words in groups of e.g. 8
            chunks = (upWidth - 1) / (EMITC_অঙ্ক_CONSTW * VL_EDATASIZE);
            upWidth %= (EMITC_অঙ্ক_CONSTW * VL_EDATASIZE);
            যদি (upWidth == 0) upWidth = (EMITC_অঙ্ক_CONSTW * VL_EDATASIZE);
        }
        {  // Upper e.g. 8 words
            যদি (chunks) {
                putnbs(nodep, "VL_CONSTHI_W_");
                puts(cvtToStr(VL_শব্দগুলি_I(upWidth)));
                puts("X(");
                puts(cvtToStr(nodep->widthMin()));
                puts(",");
                puts(cvtToStr(chunks * EMITC_অঙ্ক_CONSTW * VL_EDATASIZE));
            } অন্যথা {
                putnbs(nodep, "VL_CONST_W_");
                puts(cvtToStr(VL_শব্দগুলি_I(upWidth)));
                puts("X(");
                puts(cvtToStr(nodep->widthMin()));
            }
            puts(",");
            যদি (!assigntop->selfPointer().isEmpty()) {
                emitDereference(assigntop, assigntop->selfPointerProtect(m_useSelfForThis));
            }
            puts(varName(assigntop->varp()));
            প্রতি (পূর্ণাঙ্ক word = VL_শব্দগুলি_I(upWidth) - 1; word >= 0; word--) {
                // Only 32 bits - llx + long long here just to appease CPP format warning
                ofp()->printf(",0x%08" PRIx64, static_cast<uint64_t>(nodep->অঙ্ক().edataWord(
                                                   word + chunks * EMITC_অঙ্ক_CONSTW)));
            }
            puts(")");
        }
        প্রতি (chunks--; chunks >= 0; chunks--) {
            puts(";\n");
            putbs("VL_CONSTLO_W_");
            puts(cvtToStr(EMITC_অঙ্ক_CONSTW));
            puts("X(");
            puts(cvtToStr(chunks * EMITC_অঙ্ক_CONSTW * VL_EDATASIZE));
            puts(",");
            যদি (!assigntop->selfPointer().isEmpty()) {
                emitDereference(assigntop, assigntop->selfPointerProtect(m_useSelfForThis));
            }
            puts(varName(assigntop->varp()));
            প্রতি (পূর্ণাঙ্ক word = EMITC_অঙ্ক_CONSTW - 1; word >= 0; word--) {
                // Only 32 bits - llx + long long here just to appease CPP format warning
                ofp()->printf(",0x%08" PRIx64, static_cast<uint64_t>(nodep->অঙ্ক().edataWord(
                                                   word + chunks * EMITC_অঙ্ক_CONSTW)));
            }
            puts(")");
        }
    }

    শূন্য emitNodesWithText(Ast_গ্রন্থি* nodesp, bool useSelfForThis, bool tracking,
                           ধ্রুব std::string& separator) {
        প্রতি (Ast_গ্রন্থি* nodep = nodesp; nodep; nodep = nodep->nextp()) {
            যদি (ধ্রুব AstText* ধ্রুব textp = VN_CAST(nodep, Text)) {
                ধ্রুব std::string text
                    = VSelfPointerText::replaceThis(useSelfForThis, textp->text());
                যদি (tracking) {
                    puts(text);
                } অন্যথা {
                    ofp()->putsNoTracking(text);
                }
            } অন্যথা {
                iterateConst(nodep);
            }
            যদি (nodep->nextp()) puts(separator);
        }
    }

    শূন্য putConstructorSubinit(ধ্রুব AstClass* classp, AstCFunc* cfuncp) {
        // Direct non-virtual bases in declaration order
        প্রতি (ধ্রুব AstClassExtends* extp = classp->extendsp(); extp;
             extp = VN_AS(extp->nextp(), ClassExtends)) {
            যদি (extp->classp()->isInterfaceClass()) অনুবর্ত;
            যদি (extp->classp()->useVirtualPublic()) অনুবর্ত;
            puts(EmitCUtil::prefixNameProtect(extp->classp()));
            puts("::init");
            যদি (constructorNeedsProcess(extp->classp())) {
                puts("(vlProcess, vlSymsp");
            } অন্যথা {
                puts("(vlSymsp");
            }
            // Handle super.new() args for the concrete parent
            যদি (ধ্রুব AstCNew* ধ্রুব superNewCallp = getSuperNewCallRecursep(cfuncp->stmtsp())) {
                putCommaIterateNext(superNewCallp->argsp(), true);
            }
            puts(");\n");
            // Since SystemVerilog allows only for extending from one class, for sure there won't
            // be any more constructors to be called. Even if there are some classes which are
            // "implemented", those are interface classes which do not have constructors.
            ভঙ্গ;
        }
    }

    // VISITORS
    using EmitCConstInit::visit;
    শূন্য visit(AstCFunc* ধ্রুব nodep) override {
        যদি (nodep->emptyBody() && !nodep->isLoose()) প্রত্যাগম;
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
        যদি (nodep->ifdef() != "") putns(nodep, "#ifdef " + nodep->ifdef() + "\n");
        emitCFuncHeader(nodep, m_modp, /* withScope: */ true);

        puts(" {\n");
        যদি (nodep->isConstructor()) {
            ধ্রুব AstClass* ধ্রুব classp = VN_CAST(nodep->scopep()->modp(), Class);
            যদি (classp && classp->extendsp()) putConstructorSubinit(classp, nodep);
        }

        // "+" in the debug indicates a print from the model
        puts("VL_DEBUG_IF(VL_DBG_MSGF(\"+  ");
        প্রতি (পূর্ণাঙ্ক i = 0; i < m_modp->level(); ++i) puts("  ");
        puts(EmitCUtil::prefixNameProtect(m_modp));
        puts(nodep->isLoose() ? "__" : "::");
        puts(nodep->nameProtect() + "\\n\"); );\n");

        যদি (nodep->isLoose()) {
            m_lazyDecls.declared(nodep);  // Defined here, so no longer needs declaration
            যদি (!nodep->isStatic()) {  // Standard prologue
                m_useSelfForThis = true;
                যদি (nodep->voidSelf()) {
                    ধ্রুব string className = EmitCUtil::prefixNameProtect(m_modp);
                    puts(className + "* const vlSelf = static_cast<" + className
                         + "*>(vlVoidSelf);\n");
                }
                যদি (!VN_IS(m_modp, Class)) {
                    puts(EmitCUtil::symClassAssign());  // Uses vlSelf
                } অন্যথা {
                    puts("(void)vlSelf;  // Prevent unused variable warning\n");
                }
            }
        }

        // Instantiate a process class if it's going to be needed somewhere later
        nodep->forall([&](ধ্রুব Ast_গ্রন্থি_CCall* ccallp) -> bool {
            যদি (ccallp->funcp()->needProcess()
                && (ccallp->funcp()->isCoroutine() == VN_IS(ccallp->backp(), CAwait))) {
                যদি (!nodep->needProcess() && !m_instantiatesOwnProcess) {
                    m_instantiatesOwnProcess = true;
                    প্রত্যাগম false;
                }
            }
            প্রত্যাগম true;
        });
        যদি (m_instantiatesOwnProcess) {
            AstCStmt* ধ্রুব vlprocp = new AstCStmt{nodep->fileline()};
            vlprocp->add("VlProcessRef vlProcess = std::make_shared<VlProcess>();\n");
            vlprocp->add("VlProcess::currentp(vlProcess.get());");
            nodep->prependStmtsp(vlprocp);
        } অন্যথা যদি (nodep->needProcess() && nodep->stmtsp()) {
            // Set current process so VlRNG() constructors in this function seed from it
            AstCStmt* ধ্রুব setProcessp = new AstCStmt{nodep->fileline()};
            setProcessp->add("VlProcess::currentp(vlProcess.get());");
            nodep->prependStmtsp(setProcessp);
        }

        প্রতি (Ast_গ্রন্থি* subnodep = nodep->argsp(); subnodep; subnodep = subnodep->nextp()) {
            যদি (AstVar* ধ্রুব varp = VN_CAST(subnodep, Var)) {
                যদি (varp->isFuncReturn()) emitVarDecl(varp);
            }
        }

        যদি (m_useSelfForThis) {
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

        যদি (nodep->varsp()) {
            putsDecoration(nodep, "// Locals\n");
            iterateAndNextConstNull(nodep->varsp());
        }

        যদি (nodep->stmtsp()) {
            putsDecoration(nodep, "// Body\n");
            iterateAndNextConstNull(nodep->stmtsp());
        }

        m_usevlSelfRef = false;

        যদি (nodep->isCoroutine()) {
            // Sometimes coroutines don't have co_awaits,
            // so emit a co_return at the end to avoid compile errors.
            puts("co_return;\n");
        }

        puts("}\n");
        যদি (nodep->ifdef() != "") puts("#endif  // " + nodep->ifdef() + "\n");
    }

    শূন্য visit(AstVar* nodep) override {
        UASSERT_OBJ(m_cfuncp, nodep, "Cannot emit non-local variable");
        emitVarDecl(nodep);
    }

    শূন্য visit(AstCvtArrayToArray* nodep) override {
        যদি (nodep->reverse()) {
            puts("VL_REVCLONE_Q(");
        } অন্যথা {
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

    শূন্য visit(AstCvtArrayToPacked* nodep) override {
        Ast_গ্রন্থি_DType* ধ্রুব fromDtp = nodep->fromp()->dtypep()->skipRefp();
        Ast_গ্রন্থি_DType* ধ্রুব elemDtp = fromDtp->subDTypep()->skipRefp();
        ধ্রুব bool descending = [&]() {
            যদি (ধ্রুব AstUnpackArrayDType* ধ্রুব unpackDtp = VN_CAST(fromDtp, UnpackArrayDType))
                প্রত্যাগম !unpackDtp->declRange().ascending();
            প্রত্যাগম false;
        }();
        puts("VL_PACK_");
        emitIQW(nodep);
        puts("_");
        emitRU(fromDtp);
        emitIQW(elemDtp);
        যদি (descending) {
            // Wrap source in VL_PACK_REVERSED so VL_PACK sees ascending order
            emitOpName(nodep, "(%nw, %rw, %P, VL_PACK_REVERSED(%li))", nodep->fromp(), elemDtp,
                       nullptr);
        } অন্যথা {
            emitOpName(nodep, "(%nw, %rw, %P, %li)", nodep->fromp(), elemDtp, nullptr);
        }
    }

    শূন্য visit(AstCvtUnpackToQueue* nodep) override {
        Ast_গ্রন্থি_DType* ধ্রুব elemDTypep = nodep->fromp()->dtypep()->subDTypep();
        emitOpName(nodep, nodep->emitC(), nodep->fromp(), elemDTypep, nullptr);
    }
    শূন্য visit(AstCvtUnpackToUnpack* nodep) override {
        // Whether each dimension is reversed, the first for the outermost, up to the last that is
        std::vector<bool> reverses;
        ধ্রুব AstUnpackArrayDType* শীর্ষ = VN_AS(nodep->dtypep()->skipRefp(), UnpackArrayDType);
        ধ্রুব AstUnpackArrayDType* fromp
            = VN_AS(nodep->fromp()->dtypep()->skipRefp(), UnpackArrayDType);
        যাবত্ (শীর্ষ) {
            reverses.ঠেলো_back(শীর্ষ->declRange().ascending() != fromp->declRange().ascending());
            শীর্ষ = VN_CAST(শীর্ষ->subDTypep()->skipRefp(), UnpackArrayDType);
            fromp = VN_CAST(fromp->subDTypep()->skipRefp(), UnpackArrayDType);
        }
        যাবত্ (!reverses.back()) reverses.বের_করো_back();
        putns(nodep, "VL_CVT_UNPACK_TO_UNPACK<");
        প্রতি (size_t i = 0; i < reverses.size(); ++i) {
            যদি (i) puts(", ");
            puts(reverses[i] ? "1" : "0");
        }
        puts(">(");
        iterateAndNextConstNull(nodep->fromp());
        puts(")");
    }

    শূন্য visit(Ast_গ্রন্থি_Assign* nodep) override {
        যদি (AstCReset* ধ্রুব resetp = VN_CAST(nodep->rhsp(), CReset)) {
            // TODO get rid of emitVarReset and instead let AstNodeAssign understand how to init
            // anything
            Ast_গ্রন্থি* fromp = nodep->lhsp();
            // Fork needs to use a member select.  Nothing else should be possible before VarRef.
            যদি (AstMemberSel* ধ্রুব sfromp = VN_CAST(fromp, MemberSel)) {
                // Fork-DynScope generated pointer to previously automatic variable
                AstVar* ধ্রুব memberVarp = sfromp->varp();
                fromp = sfromp->fromp();
                যদি (AstNullCheck* ধ্রুব sfromp = VN_CAST(fromp, NullCheck)) fromp = sfromp->lhsp();
                Ast_গ্রন্থি_Var_Ref* ধ্রুব fromVarRefp = VN_AS(fromp, গ্রন্থি_Var_Ref);
                emitVarReset(
                    ("VL_NULL_CHECK("s
                     + (fromVarRefp->selfPointer().isEmpty()
                            ? ""
                            : dereferenceString(fromVarRefp->selfPointerProtect(m_useSelfForThis)))
                     + varName(fromVarRefp->varp()) + ", \""
                     + V3OutFormatter::quoteNameControls(protect(nodep->fileline()->filename()))
                     + "\", " + std::to_string(nodep->fileline()->lineno()) + ")->"),
                    memberVarp, resetp->constructing());
            } অন্যথা {
                Ast_গ্রন্থি_Var_Ref* ধ্রুব fromVarRefp = VN_AS(fromp, গ্রন্থি_Var_Ref);
                AstVar* ধ্রুব varp = fromVarRefp->varp();
                ধ্রুব string prefix
                    = fromVarRefp->selfPointer().isEmpty()
                          ? ""
                          : dereferenceString(
                                VN_AS(fromp, গ্রন্থি_Var_Ref)->selfPointerProtect(m_useSelfForThis));
                emitVarReset(prefix, varp, resetp->constructing());
            }
            প্রত্যাগম;
        }
        bool paren = true;
        bool decind = false;
        bool rhs = true;
        bool reverseUnpack = false;  // Set for descending CvtPackedToArray
        ধ্রুব AstUnpackArrayDType* ধ্রুব unpackDtp
            = VN_CAST(nodep->dtypep()->skipRefp(), UnpackArrayDType);
        যদি (AstSel* ধ্রুব selp = VN_CAST(nodep->lhsp(), Sel)) {
            UASSERT_OBJ(selp->widthMin() == selp->widthConst(), selp, "Width mismatch");
            যদি (selp->widthMin() == 1) {
                putnbs(nodep, "VL_ASSIGNBIT_");
                emitIQW(selp->fromp());
                যদি (nodep->rhsp()->isAllOnesV()) {
                    puts("O(");
                    rhs = false;
                } অন্যথা {
                    puts("I(");
                }
                iterateAndNextConstNull(selp->lsbp());
                puts(", ");
                iterateAndNextConstNull(selp->fromp());
                যদি (rhs) puts(", ");
            } অন্যথা {
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
        } অন্যথা যদি (ধ্রুব AstGetcRefN* ধ্রুব selp = VN_CAST(nodep->lhsp(), GetcRefN)) {
            iterateAndNextConstNull(selp->lhsp());
            puts(" = ");
            putnbs(selp, "VL_PUTC_N(");
            iterateAndNextConstNull(selp->lhsp());
            puts(", ");
            iterateAndNextConstNull(selp->rhsp());
            puts(", ");
        } অন্যথা যদি (AstVar* ধ্রুব varp = AstVar::scVarRecurse(nodep->lhsp())) {
            putnbs(varp, "VL_ASSIGN_");  // Set a systemC variable
            emitScIQW(varp);
            emitIQW(nodep);
            puts("(");
            puts(cvtToStr(nodep->widthMin()) + ", ");
            iterateAndNextConstNull(nodep->lhsp());
            puts(", ");
        } অন্যথা যদি (AstVar* ধ্রুব varp = AstVar::scVarRecurse(nodep->rhsp())) {
            putnbs(varp, "VL_ASSIGN_");  // Get a systemC variable
            emitIQW(nodep);
            emitScIQW(varp);
            puts("(");
            puts(cvtToStr(nodep->widthMin()) + ", ");
            iterateAndNextConstNull(nodep->lhsp());
            puts(", ");
        } অন্যথা যদি (ধ্রুব AstCvtPackedToArray* ধ্রুব castp
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
            যদি (unpackDtp && !unpackDtp->declRange().ascending()) reverseUnpack = true;
        } অন্যথা যদি (ধ্রুব AstCvtArrayToArray* ধ্রুব castp
                   = VN_CAST(nodep->rhsp(), CvtArrayToArray)) {
            যদি (castp->reverse()) {
                putns(castp, "VL_REVCOPY_Q(");
            } অন্যথা {
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
        } অন্যথা যদি (nodep->isWide() && VN_IS(nodep->lhsp(), VarRef)  //
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
                   && !VN_IS(nodep->rhsp(), রাশিমালা_Stmt)) {
            // Wide functions assign into the array directly, don't need separate assign statement
            m_wideTempRefp = VN_AS(nodep->lhsp(), VarRef);
            paren = false;
        } অন্যথা যদি (nodep->isWide() && !unpackDtp && !VN_IS(nodep->rhsp(), Const)) {
            putnbs(nodep, "VL_ASSIGN_W(");
            puts(cvtToStr(nodep->widthMin()) + ", ");
            iterateAndNextConstNull(nodep->lhsp());
            puts(", ");
        } অন্যথা যদি (VN_IS(nodep->lhsp()->dtypep()->skipRefp(), QueueDType)
                   && (VN_IS(nodep->rhsp(), StreamL) || VN_IS(nodep->lhsp(), StreamL)
                       || VN_IS(nodep->rhsp(), StreamR) || VN_IS(nodep->lhsp(), StreamR)
                       || VN_IS(nodep->rhsp(), StreamR))) {
            //if either side is streamL or streamR don't emit lhsp everything will be passed by
            //reference

            paren = false;

        } অন্যথা {
            paren = false;
            iterateAndNextConstNull(nodep->lhsp());
            puts(" ");
            ofp()->blockInc();
            decind = true;
            যদি (!VN_IS(nodep->rhsp(), Const)) ofp()->putBreak();
            putns(nodep, "= ");
        }
        যদি (rhs) iterateAndNextConstNull(nodep->rhsp());
        যদি (paren) puts(")");
        যদি (decind) ofp()->blockDec();
        puts(";\n");
        যদি (reverseUnpack) {
            puts("VL_UNPACK_REVERSED(");
            iterateAndNextConstNull(nodep->lhsp());
            puts(");\n");
        }
    }
    শূন্য visit(AstAssocSel* nodep) override {
        iterateAndNextConstNull(nodep->fromp());
        ধ্রুব std::string atFunc = nodep->isLValue() ? ".atWrite(" : ".at(";
        putnbs(nodep, atFunc);
        AstAssocArrayDType* ধ্রুব adtypep
            = VN_AS(nodep->fromp()->dtypep()->skipRefp(), AssocArrayDType);
        UASSERT_OBJ(adtypep, nodep, "Associative select on non-associative type");
        iterateAndNextConstNull(nodep->bitp());
        puts(")");
    }
    শূন্য visit(AstWildcardSel* nodep) override {
        iterateAndNextConstNull(nodep->fromp());
        ধ্রুব std::string atFunc = nodep->isLValue() ? ".atWrite(" : ".at(";
        putnbs(nodep, atFunc);
        AstWildcardArrayDType* ধ্রুব adtypep
            = VN_AS(nodep->fromp()->dtypep()->skipRefp(), WildcardArrayDType);
        UASSERT_OBJ(adtypep, nodep, "Wildcard select on non-wildcard-associative type");
        iterateAndNextConstNull(nodep->bitp());
        puts(")");
    }
    শূন্য visit(AstCCall* nodep) override {
        ধ্রুব AstCFunc* ধ্রুব funcp = nodep->funcp();
        ধ্রুব Ast_গ্রন্থি_Module* ধ্রুব funcModp = EmitCParentModule::get(funcp);
        putnbs(nodep, "");
        যদি (funcp->dpiImportPrototype()) {
            // Calling DPI import
            putns(nodep, funcp->name());
        } অন্যথা যদি (funcp->isProperMethod() && funcp->isStatic()) {
            // Call static method via the containing class
            putns(funcModp, EmitCUtil::prefixNameProtect(funcModp) + "::");
            putns(nodep, funcp->nameProtect());
        } অন্যথা যদি (nodep->superReference()) {
            // Calling superclass method
            putns(funcModp, EmitCUtil::prefixNameProtect(funcModp) + "::");
            putns(nodep, funcp->nameProtect());
        } অন্যথা যদি (funcp->isLoose()) {
            // Calling loose method
            putns(nodep, funcNameProtect(funcp));
        } অন্যথা {
            // Calling regular method/function
            যদি (!nodep->selfPointer().isEmpty()) {
                emitDereference(nodep, nodep->selfPointerProtect(m_useSelfForThis));
            }
            putns(nodep, funcp->nameProtect());
        }
        emitCCallArgs(nodep, nodep->selfPointerProtect(m_useSelfForThis), m_cfuncp->needProcess());
    }
    শূন্য visit(AstCMethodCall* nodep) override {
        ধ্রুব AstCFunc* ধ্রুব funcp = nodep->funcp();
        UASSERT_OBJ(!funcp->isLoose(), nodep, "Loose method called via AstCMethodCall");
        iterateConst(nodep->fromp());
        putnbs(nodep, "->");
        যদি (nodep->superReference()) {
            // Super call, so the base implementation is used rather than the override
            ধ্রুব Ast_গ্রন্থি_Module* ধ্রুব funcModp = EmitCParentModule::get(funcp);
            putns(funcModp, EmitCUtil::prefixNameProtect(funcModp) + "::");
        }
        putns(funcp, funcp->nameProtect());
        emitCCallArgs(nodep, "", m_cfuncp->needProcess());
    }
    শূন্য visit(AstCAwait* nodep) override {
        putns(nodep, "co_await ");
        iterateConst(nodep->exprp());
        puts(";\n");
    }
    শূন্য visit(AstCNew* nodep) override {
        যদি (VN_IS(nodep->dtypep(), VoidDType)) {  // super.new case
            putsDecoration(nodep, "/*super.new*/");
            প্রত্যাগম;
        }
        // assignment case;
        putns(nodep, "VL_NEW(" + EmitCUtil::prefixNameProtect(nodep->dtypep()) + ", "
                         + optionalProcArg(nodep->dtypep()) + "vlSymsp");
        putCommaIterateNext(nodep->argsp(), true);
        puts(")");
    }
    শূন্য visit(AstCMethodHard* nodep) override {
        iterateConst(nodep->fromp());
        putns(nodep, nodep->usePtr() ? "->" : ".");
        putns(nodep, nodep->name());
        যদি (nodep->method() == VCMethod::FORCE_READ_SEL) {
            emitIQW(nodep);
            যদি (nodep->isWide()) puts("<" + cvtToStr(nodep->dtypep()->width_শব্দগুলি()) + ">");
        } অন্যথা যদি (nodep->method() == VCMethod::ARRAY_SLICE) {
            // VlUnpacked::slice<N_Out>(loIdx) - N_Out is the (fixed) result array size
            ধ্রুব AstUnpackArrayDType* ধ্রুব adtypep
                = VN_AS(nodep->dtypep()->skipRefp(), UnpackArrayDType);
            puts("<" + cvtToStr(adtypep->elementsConst()) + ">");
        } অন্যথা যদি (nodep->method() == VCMethod::COVERGROUP_ADD_COVERPOINT) {
            // The hit-list bound is a template argument of the returned VlCoverpointT<>, and the
            // node's own dtype is that type, so it is the one source of truth for both.
            ধ্রুব AstCoverpointDType* ধ্রুব cpdtypep
                = VN_AS(nodep->dtypep()->skipRefp(), CoverpointDType);
            puts("<" + cvtToStr(cpdtypep->hitBound()) + ">");
        } অন্যথা যদি (nodep->method() == VCMethod::COVERGROUP_ADD_CROSS) {
            ধ্রুব AstCoverCrossDType* ধ্রুব cxdtypep
                = VN_AS(nodep->dtypep()->skipRefp(), CoverCrossDType);
            puts("<" + cxdtypep->cppTemplateArgs() + ">");
        }
        puts("(");
        bool comma = false;
        পূর্ণাঙ্ক arg_অঙ্ক = 0;
        প্রতি (Ast_গ্রন্থি* subnodep = nodep->pinsp(); subnodep; subnodep = subnodep->nextp()) {
            যদি (comma) puts(", ");
            যদি (nodep->method() == VCMethod::RANDOMIZER_HARD && arg_অঙ্ক == 1) {
                // For RANDOMIZER_HARD's filename argument (2nd arg after constraint),
                // apply protect() similar to VL_STOP to handle --protected flag
                যদি (ধ্রুব AstCExpr* ধ্রুব cexprp = VN_CAST(subnodep, CExpr)) {
                    // Extract filename from the CExpr (which contains "filename")
                    std::string filename;
                    প্রতি (ধ্রুব Ast_গ্রন্থি* textnodep = cexprp->nodesp(); textnodep;
                         textnodep = textnodep->nextp()) {
                        যদি (ধ্রুব AstText* ধ্রুব textp = VN_CAST(textnodep, Text)) {
                            filename = textp->text();
                            ভঙ্গ;
                        }
                    }
                    // Remove surrounding quotes if present
                    যদি (filename.size() >= 2 && filename.front() == '"'
                        && filename.back() == '"') {
                        filename = filename.substr(1, filename.size() - 2);
                    }
                    // Emit with protect()
                    putsQuoted(protect(filename));
                } অন্যথা {
                    iterateConst(subnodep);
                }
            } অন্যথা {
                iterateConst(subnodep);
            }
            comma = true;
            arg_অঙ্ক++;
        }
        যদি (nodep->withp()) {
            যদি (comma) puts(", ");
            iterateConst(nodep->withp());
        }
        puts(")");
    }
    শূন্য visit(AstCFuncHard* nodep) override {
        putns(nodep, nodep->name());
        puts("(");
        putCommaIterateNext(nodep->pinsp());
        puts(")");
    }
    শূন্য visit(AstLambdaArgRef* nodep) override { putbs(nodep->nameProtect()); }
    শূন্য visit(AstWith* nodep) override {
        // With uses a C++11 lambda
        putnbs(nodep, "[&](");
        যদি (auto* ধ্রুব argrefp = nodep->indexArgRefp()) {
            putnbs(argrefp, argrefp->dtypep()->cType(argrefp->nameProtect(), false, false));
            puts(", ");
        }
        যদি (auto* ধ্রুব argrefp = nodep->মান_Arg_Refp()) {
            putnbs(argrefp, argrefp->dtypep()->cType(argrefp->nameProtect(), false, false));
        }
        puts(") -> ");
        putnbs(nodep, nodep->dtypep()->cType("", false, false));
        puts(" {\n");
        VL_RESTORER(m_createdScopeHash);
        iterateAndNextConstNull(nodep->exprp());
        puts("}\n");
    }
    শূন্য visit(AstCase* nodep) override {  // LCOV_EXCL_LINE
        // In V3Case...
        nodep->v3fatalSrc("Case statements should have been reduced out");
    }
    শূন্য visit(AstComment* nodep) override {
        string at;
        যদি (nodep->showAt()) {
            at = " at " + nodep->fileline()->ascii();
            // If protecting, passthru less information about the design
            যদি (!v3Global.opt.protectIds()) প্রত্যাগম;
        }
        যদি (!(nodep->protect() && v3Global.opt.protectIds())) {
            putsDecoration(nodep, "// "s + nodep->name() + at + "\n");
        }
        iterateChildrenConst(nodep);
    }
    শূন্য visit(AstCoverOtherDecl* nodep) override {
        emit_Cover_Other_Decl_ঢোকাও(nodep);
        যদি (coverageUsesLocalCounter(nodep)) {
            // Some task bodies are cloned into a caller module where the
            // original instance-local counter is not addressable. Those clones
            // fall back to the global bin number, so register that slot too.
            emit_Cover_Other_Decl_ঢোকাও(nodep, true);
        }
    }
    শূন্য visit(AstCoverToggleDecl* nodep) override {
        putns(nodep, "vlSelf->__vlCoverToggleInsert(");  // As Declared in emitCoverageDecl
        puts(cvtToStr(nodep->range().ডান()));
        puts(", ");
        puts(cvtToStr(nodep->range().বাম()));
        puts(", ");
        puts(cvtToStr(nodep->range().ranged()));
        puts(", ");
        putCoverageArray(nodep);
        puts(" + ");
        puts(cvtToStr(coverage_Bin_অঙ্ক(nodep)));
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
                   + VIdProtect::protect_শব্দগুলি_If(nodep->hier(), nodep->protect()));
        puts(", ");
        putsQuoted(VIdProtect::protect_শব্দগুলি_If(nodep->page(), nodep->protect()));
        puts(", ");
        putsQuoted(VIdProtect::protect_শব্দগুলি_If(nodep->comment(), nodep->protect()));
        puts(");\n");
    }
    শূন্য visit(AstCoverInc* nodep) override {
        যদি (VN_IS(nodep->declp(), CoverOtherDecl)) {
            যদি (v3Global.opt.threads() > 1) {
                putns(nodep, "");
                putCoverageArray(nodep->declp());
                puts("[");
                puts(cvtToStr(coverage_Bin_অঙ্ক(nodep->declp())));
                puts("].fetch_add(1, std::memory_order_relaxed);\n");
            } অন্যথা {
                putns(nodep, "++(");
                putCoverageArray(nodep->declp());
                puts("[");
                puts(cvtToStr(coverage_Bin_অঙ্ক(nodep->declp())));
                puts("]);\n");
            }
        } অন্যথা {
            puts("VL_COV_TOGGLE_CHG_");
            যদি (v3Global.opt.threads() > 1) {
                puts("MT_");
            } অন্যথা {
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
            puts(cvtToStr(coverage_Bin_অঙ্ক(nodep->declp())));
            puts(", ");
            iterateConst(nodep->toggleExprp());
            puts(", ");
            iterateConst(nodep->toggleCovExprp());
            puts(");\n");
        }
    }
    শূন্য visit(AstDisableFork* nodep) override { putns(nodep, "vlProcess->disableFork();\n"); }
    শূন্য visit(AstCReturn* nodep) override {
        putns(nodep, "return (");
        iterateAndNextConstNull(nodep->lhsp());
        puts(");\n");
    }
    শূন্য visit(AstDisplay* nodep) override {
        string text = nodep->fmtp()->text();
        যদি (nodep->addNewline()) text += "\n";
        display_গ্রন্থি(nodep, nodep->fmtp(), text, nodep->fmtp()->exprsp(), false);
    }
    শূন্য visit(AstDumpCtl* nodep) override {
        বিকল্প (nodep->ctlType()) {
        পক্ষ VDumpCtlType::FILE:
            putns(nodep, "vlSymsp->_vm_contextp__->dumpfile(");
            iterateConst(nodep->exprp());
            puts(");\n");
            ভঙ্গ;
        পক্ষ VDumpCtlType::VARS:
            // We ignore number of levels to dump in exprp()
            যদি (v3Global.opt.trace()) {
                putns(nodep, "vlSymsp->_traceDumpOpen();\n");
            } অন্যথা {
                putns(nodep, "VL_PRINTF_MT(\"-Info: ");
                puts(V3OutFormatter::quoteNameControls(protect(nodep->fileline()->filename())));
                puts(":");
                puts(cvtToStr(nodep->fileline()->lineno()));
                puts(": $dumpvar ignored, as Verilated without --trace");
                puts("\\n\");\n");
            }
            ভঙ্গ;
        পক্ষ VDumpCtlType::ALL:
            // $dumpall currently ignored
            ভঙ্গ;
        পক্ষ VDumpCtlType::FLUSH:
            // $dumpall currently ignored; would need rework of VCD single thread,
            // or flag we pass-through to next eval() iteration
            ভঙ্গ;
        পক্ষ VDumpCtlType::LIMIT:
            // $dumplimit currently ignored
            ভঙ্গ;
        পক্ষ VDumpCtlType::OFF:
            // Currently ignored as both Vcd and Fst do not support them, as would need "X" dump
            ভঙ্গ;
        পক্ষ VDumpCtlType::ON:
            // Currently ignored as $dumpoff is also ignored
            ভঙ্গ;
        স্বভাব: nodep->v3fatalSrc("Bad case, unexpected " << nodep->ctlType().ascii());
        }
    }
    শূন্য visit(AstScopeName* nodep) override {
        // For use under AstCCalls for dpiImports.  ScopeNames under
        // displays are handled in AstDisplay
        যদি (!nodep->dpiExport()) {
            // this is where the DPI import context scope is set
            ধ্রুব string scope = nodep->scopeDpiName();
            putnbs(nodep, "(vlSymsp->" + protect("__Vscopep_" + scope) + ")");
        }
    }
    শূন্য visit(AstSFormat* nodep) override {
        display_গ্রন্থি(nodep, nodep->fmtp(), nodep->fmtp()->text(), nodep->fmtp()->exprsp(), false);
    }
    শূন্য visit(AstSFormatF* nodep) override {
        display_গ্রন্থি(nodep, nodep, nodep->text(), nodep->exprsp(), false);
    }
    শূন্য visit(AstFScanF* nodep) override {
        display_গ্রন্থি(nodep, nullptr, nodep->text(), nodep->exprsp(), true);
    }
    শূন্য visit(AstSScanF* nodep) override {
        display_গ্রন্থি(nodep, nullptr, nodep->text(), nodep->exprsp(), true);
    }
    শূন্য visit(Ast_মান_Plus_Args* nodep) override {
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
    শূন্য visit(AstTestPlusArgs* nodep) override {
        putns(nodep, "VL_TESTPLUSARGS_I(");
        iterateConst(nodep->searchp());
        puts(")");
    }
    শূন্য visit(AstFError* nodep) override {
        putns(nodep, "VL_FERROR_I");
        puts(nodep->strp()->isString() ? "N(" : "W(");
        iterateAndNextConstNull(nodep->filep());
        putbs(", ");
        যদি (nodep->strp()->isWide()) {
            puts(cvtToStr(nodep->strp()->width_শব্দগুলি()));
            putbs(", ");
        }
        iterateAndNextConstNull(nodep->strp());
        puts(")");
    }
    শূন্য visit(AstFGetS* nodep) override {
        emitOpName(nodep, nodep->emitC(), nodep->strgp(), nodep->filep(), nullptr);
    }
    শূন্য visit(AstFOpen* nodep) override {
        putns(nodep, "VL_FOPEN_NN(");
        iterateConst(nodep->filenamep());
        putbs(", ");
        iterateConst(nodep->modep());
        puts(");\n");
    }
    শূন্য visit(AstFOpenMcd* nodep) override {
        putns(nodep, "VL_FOPEN_MCD_N(");
        iterateConst(nodep->filenamep());
        puts(");\n");
    }
    শূন্য visit(Ast_গ্রন্থি_Read_Write_Mem* nodep) override {
        putns(nodep, nodep->cFuncPrefixp());
        puts("N(");
        puts(nodep->isHex() ? "true" : "false");
        putbs(", ");
        // Need real storage width
        puts(cvtToStr(nodep->memp()->dtypep()->skipRefp()->subDTypep()->widthMin()));
        uint32_t array_lo = 0;
        bool memUsePtr = true;
        {
            ধ্রুব AstVarRef* ধ্রুব varrefp = VN_CAST(nodep->memp(), VarRef);
            যদি (!varrefp) {
                nodep->v3error(nodep->verilogKwd() << " loading non-variable");
            } অন্যথা যদি (VN_IS(varrefp->varp()->dtypeSkipRefp(), AssocArrayDType)
                       || VN_IS(varrefp->varp()->dtypeSkipRefp(), DynArrayDType)) {
                // nodep->memp() below will when verilated code is compiled create a C++ template
                memUsePtr = false;
            } অন্যথা যদি (ধ্রুব AstUnpackArrayDType* ধ্রুব adtypep
                       = VN_CAST(varrefp->varp()->dtypeSkipRefp(), UnpackArrayDType)) {
                putbs(", ");
                puts(cvtToStr(varrefp->varp()->dtypep()->arrayUnpackedElements()));
                array_lo = adtypep->lo();
                putbs(", ");
                puts(cvtToStr(array_lo));
            } অন্যথা {
                nodep->v3error(
                    nodep->verilogKwd()
                    << " loading other than unpacked/dynamic/associative-array variable");
            }
        }
        putbs(", ");
        iterateConst(nodep->filenamep());
        putbs(", ");
        {
            যদি (memUsePtr) puts(" &(");
            iterateAndNextConstNull(nodep->memp());
            যদি (memUsePtr) puts(")");
        }
        putbs(", ");
        যদি (nodep->lsbp()) {
            iterateAndNextConstNull(nodep->lsbp());
        } অন্যথা {
            puts(cvtToStr(array_lo));
        }
        putbs(", ");
        যদি (nodep->msbp()) {
            iterateAndNextConstNull(nodep->msbp());
        } অন্যথা {
            puts("~0ULL");
        }
        puts(");\n");
    }
    শূন্য visit(AstFClose* nodep) override {
        putns(nodep, "VL_FCLOSE_I(");
        iterateAndNextConstNull(nodep->filep());
        puts("); ");
    }
    শূন্য visit(AstFFlush* nodep) override {
        যদি (!nodep->filep()) {
            putns(nodep, "VL_FFLUSH_MT();");
        } অন্যথা {
            putns(nodep, "VL_FFLUSH_I(");
            iterateAndNextConstNull(nodep->filep());
            puts(");\n");
        }
    }
    শূন্য visit(AstFSeek* nodep) override {
        putns(nodep, "(VL_FSEEK_I(");
        iterateAndNextConstNull(nodep->filep());
        puts(", ");
        iterateAndNextConstNull(nodep->offset());
        puts(", ");
        iterateAndNextConstNull(nodep->operation());
        puts(") == -1 ? -1 : 0)");
    }
    শূন্য visit(AstFTell* nodep) override {
        putns(nodep, "VL_FTELL_I(");
        iterateAndNextConstNull(nodep->filep());
        puts(")");
    }
    শূন্য visit(AstFRewind* nodep) override {
        putns(nodep, "(VL_FSEEK_I(");
        iterateAndNextConstNull(nodep->filep());
        puts(", 0, 0) == -1 ? -1 : 0)");
    }
    শূন্য visit(AstFRead* nodep) override {
        putns(nodep, "VL_FREAD_I(");
        puts(cvtToStr(nodep->memp()->widthMin()));  // Need real storage width
        putbs(", ");
        uint32_t array_lo = 0;
        uint32_t array_size = 0;
        {
            ধ্রুব AstVarRef* ধ্রুব varrefp = VN_CAST(nodep->memp(), VarRef);
            যদি (!varrefp) {
                nodep->v3error(nodep->verilogKwd() << " loading non-variable");
            } অন্যথা যদি (VN_CAST(varrefp->varp()->dtypeSkipRefp(), BasicDType)) {
            } অন্যথা যদি (ধ্রুব AstUnpackArrayDType* ধ্রুব adtypep
                       = VN_CAST(varrefp->varp()->dtypeSkipRefp(), UnpackArrayDType)) {
                array_lo = adtypep->lo();
                array_size = adtypep->elementsConst();
            } অন্যথা {
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
        যদি (nodep->startp()) {
            iterateAndNextConstNull(nodep->startp());
        } অন্যথা {
            puts(cvtToStr(array_lo));
        }
        putbs(", ");
        যদি (nodep->countp()) {
            iterateAndNextConstNull(nodep->countp());
        } অন্যথা {
            puts(cvtToStr(array_size));
        }
        puts(")");
    }
    শূন্য visit(Ast_স্তূপ_Trace_F* nodep) override { putns(nodep, "VL_STACKTRACE_N()"); }
    শূন্য visit(Ast_স্তূপ_Trace_T* nodep) override { putns(nodep, "VL_STACKTRACE();\n"); }
    শূন্য visit(AstSystemT* nodep) override {
        putns(nodep, "(void)VL_SYSTEM_I");
        emitIQW(nodep->lhsp());
        puts("(");
        যদি (nodep->lhsp()->isWide()) {
            puts(cvtToStr(nodep->lhsp()->width_শব্দগুলি()));
            putbs(", ");
        }
        iterateAndNextConstNull(nodep->lhsp());
        puts(");\n");
    }
    শূন্য visit(AstSystemF* nodep) override {
        putns(nodep, "VL_SYSTEM_I");
        emitIQW(nodep->lhsp());
        puts("(");
        যদি (nodep->lhsp()->isWide()) {
            puts(cvtToStr(nodep->lhsp()->width_শব্দগুলি()));
            putbs(", ");
        }
        iterateAndNextConstNull(nodep->lhsp());
        puts(")");
    }
    শূন্য visit(Ast_Stmt_রাশিমালা* nodep) override {
        যদি (VN_IS(nodep->exprp()->dtypep()->skipRefp(), VoidDType)) {
            putns(nodep, "");
        } অন্যথা {
            putns(nodep, "std::ignore = ");
        }
        iterateConst(nodep->exprp());
        puts(";\n");
    }
    শূন্য visit(AstJumpBlock* nodep) override {
        // Allocate label number
        ধ্রুব size_t n = m_labelNumbers.size();
        ধ্রুব bool newEntry = m_labelNumbers.emplace(nodep, n).second;
        UASSERT_OBJ(newEntry, nodep, "AstJumpBlock visited twide");
        // Emit
        putns(nodep, "{\n");  // Make it visually obvious label jumps outside these
        VL_RESTORER(m_createdScopeHash);
        iterateAndNextConstNull(nodep->stmtsp());
        puts("__Vlabel" + std::to_string(n) + ": ;\n");
        puts("}\n");
    }
    শূন্য visit(AstJumpGo* nodep) override {
        // Retrieve target label number - must already exist (from enclosing AstJumpBlock)
        ধ্রুব size_t n = m_labelNumbers.at(nodep->blockp());
        // Emit
        putns(nodep, "goto __Vlabel" + std::to_string(n) + ";\n");
    }
    শূন্য visit(AstLoop* nodep) override {
        UASSERT_OBJ(!nodep->contsp(), nodep, "'contsp' only used before LinkJump");
        VL_RESTORER(m_createdScopeHash);
        // Special case when the AstLoopTest is first for output readability
        যদি (AstLoopTest* ধ্রুব testp = VN_CAST(nodep->stmtsp(), LoopTest)) {
            putns(nodep, "while (");
            iterateConst(testp->condp());
            puts(") {\n");
            iterateAndNextConstNull(testp->nextp());
            puts("}\n");
            প্রত্যাগম;
        }
        // Special case when the AstLoopTest is last for output readability
        যদি (Ast_গ্রন্থি* lastp = nodep->stmtsp()) {
            যাবত্ (Ast_গ্রন্থি* ধ্রুব nextp = lastp->nextp()) lastp = nextp;
            যদি (AstLoopTest* ধ্রুব testp = VN_CAST(lastp, LoopTest)) {
                putns(nodep, "do {\n");
                প্রতি (Ast_গ্রন্থি* p = nodep->stmtsp(); p != lastp; p = p->nextp()) iterateConst(p);
                puts("} while (");
                iterateConst(testp->condp());
                puts(");\n");
                প্রত্যাগম;
            }
        }
        // Emit generic case directly
        putns(nodep, "while (true) {\n");
        iterateAndNextConstNull(nodep->stmtsp());
        puts("}\n");
    }
    শূন্য visit(AstLoopTest* nodep) override {
        VL_RESTORER(m_createdScopeHash);
        putns(nodep, "if (!(");
        iterateAndNextConstNull(nodep->condp());
        puts(")) break;\n");
    }
    শূন্য visit(AstCLocalScope* nodep) override {
        putns(nodep, "{\n");
        VL_RESTORER(m_createdScopeHash);
        iterateAndNextConstNull(nodep->stmtsp());
        puts("}\n");
    }
    শূন্য visit(Ast_গ্রন্থি_If* nodep) override {
        putns(nodep, "if (");
        যদি (!nodep->branchPred().unknown()) {
            puts(nodep->branchPred().ascii());
            puts("((");  // Two parens, so that VL_UNLIKELY((class<foo,bar>)) works
        }
        iterateAndNextConstNull(nodep->condp());
        যদি (!nodep->branchPred().unknown()) puts("))");
        puts(") {\n");
        {
            VL_RESTORER(m_createdScopeHash);
            iterateAndNextConstNull(nodep->thensp());
        }
        puts("}");
        যদি (!nodep->elsesp()) {
            puts("\n");
        } অন্যথা {
            যদি (VN_IS(nodep->elsesp(), গ্রন্থি_If) && !nodep->elsesp()->nextp()) {
                puts(" else ");
                iterateAndNextConstNull(nodep->elsesp());
            } অন্যথা {
                VL_RESTORER(m_createdScopeHash);
                puts(" else {\n");
                iterateAndNextConstNull(nodep->elsesp());
                puts("}\n");
            }
        }
    }
    শূন্য visit(Ast_রাশিমালা_Stmt* nodep) override {
        VL_RESTORER(m_createdScopeHash);
        ধ্রুব bool containsAwait = nodep->exists([](AstCAwait*) -> bool { প্রত্যাগম true; });
        যদি (containsAwait) {
            UASSERT_OBJ(m_cfuncp && m_cfuncp->isCoroutine(), nodep,
                        "AstExprStmt with CAwait must be in coroutine");
            putnbs(nodep, "(co_await ([&]() -> VlCoroutine {\n");
            iterateAndNextConstNull(nodep->stmtsp());
            puts("co_return;\n");
            যদি (!nodep->hasResult()) {
                puts("}()))");
                প্রত্যাগম;
            }
            puts("}()), ");
            iterateAndNextConstNull(nodep->resultp());
            puts(")");
            প্রত্যাগম;
        }
        // GCC allows compound statements in expressions, but this is not standard.
        // So we use an immediate-evaluation lambda and comma operator
        putnbs(nodep, "([&]() {\n");
        যদি (!nodep->hasResult()) {
            iterateAndNextConstNull(nodep->stmtsp());
            puts("}())");
            প্রত্যাগম;
        }
        iterateAndNextConstNull(nodep->stmtsp());
        puts("}(), ");
        iterateAndNextConstNull(nodep->resultp());
        puts(")");
    }
    শূন্য visit(AstStop* nodep) override {
        putns(nodep, "VL_STOP_MT(");
        putsQuoted(protect(nodep->fileline()->filename()));
        puts(", ");
        puts(cvtToStr(nodep->fileline()->lineno()));
        puts(", \"\"");
        যদি (nodep->isFatal()) puts(", false");
        puts(");\n");
    }
    শূন্য visit(AstFinish* nodep) override {
        // Disable all the forks so they don't operate after simulation is finished.
        যদি (m_cfuncp && m_cfuncp->needProcess()) putns(nodep, "vlProcess->disableFork();\n");
        putns(nodep, "VL_FINISH_MT(");
        putsQuoted(protect(nodep->fileline()->filename()));
        puts(", ");
        puts(cvtToStr(nodep->fileline()->lineno()));
        puts(", \"\");\n");
    }
    শূন্য visit(AstFinishFork* nodep) override {
        // Disable all the forks so they don't operate after simulation is finished.
        যদি (m_cfuncp && m_cfuncp->needProcess()) putns(nodep, "vlProcess->disableFork();\n");
        putns(nodep, "VL_FINISH_MT(");
        putsQuoted(protect(nodep->fileline()->filename()));
        puts(", ");
        puts(cvtToStr(nodep->fileline()->lineno()));
        puts(", \"\");\n");
        যদি (m_cfuncp->isCoroutine()) {
            putns(nodep, "co_return;\n");
        } অন্যথা {
            putns(nodep, "return;\n");
        }
    }
    শূন্য visit(AstPrintTimeScale* nodep) override {
        putns(nodep, "VL_PRINTTIMESCALE(");
        putsQuoted(protect(nodep->prettyName()));
        puts(", ");
        putsQuoted(nodep->timeunit().ascii());
        puts(", vlSymsp->_vm_contextp__);\n");
    }
    শূন্য visit(AstRand* nodep) override {
        emitOpName(nodep, nodep->emitC(), nodep->seedp(), nullptr, nullptr);
    }
    শূন্য visit(AstRandRNG* nodep) override {
        emitOpName(nodep, nodep->emitC(), nullptr, nullptr, nullptr);
    }
    শূন্য visit(AstTime* nodep) override {
        putns(nodep, "VL_TIME_UNITED_Q(");
        UASSERT_OBJ(!nodep->timeunit().isNone(), nodep, "$time has no units");
        ধ্রুব দ্বিগুণ time
            = nodep->timeunit().multiplier() / v3Global.rootp()->timeprecision().multiplier();
        UASSERT_OBJ(time >= 1, nodep, "TimeQ is less than 1, will result in division by zero");
        puts(cvtToStr(time));
        puts(")");
    }
    শূন্য visit(AstTimeD* nodep) override {
        putns(nodep, "VL_TIME_UNITED_D(");
        UASSERT_OBJ(!nodep->timeunit().isNone(), nodep, "$realtime has no units");
        puts(cvtToStr(nodep->timeunit().multiplier()
                      / v3Global.rootp()->timeprecision().multiplier()));
        puts(")");
    }
    শূন্য visit(AstGetInitialRandomSeed* nodep) override {
        putns(nodep, "vlSymsp->_vm_contextp__->randSeed()");
    }
    শূন্য visit(AstTimeFormat* nodep) override {
        putns(nodep, "VL_TIMEFORMAT_IINI(");
        যদি (nodep->unitsp()) {
            puts("true, ");
            iterateAndNextConstNull(nodep->unitsp());
        } অন্যথা {
            puts("false, 0");
        }
        puts(", ");
        যদি (nodep->precisionp()) {
            puts("true, ");
            iterateAndNextConstNull(nodep->precisionp());
        } অন্যথা {
            puts("false, 0");
        }
        puts(", ");
        যদি (nodep->suffixp()) {
            puts("true, ");
            iterateAndNextConstNull(nodep->suffixp());
        } অন্যথা {
            puts("false, \"\"");
        }
        puts(", ");
        যদি (nodep->widthp()) {
            puts("true, ");
            iterateAndNextConstNull(nodep->widthp());
        } অন্যথা {
            puts("false, 0");
        }
        puts(", vlSymsp->_vm_contextp__);\n");
    }
    শূন্য visit(AstTimePrecision* nodep) override {
        putns(nodep, "vlSymsp->_vm_contextp__->timeprecision()");
    }

    // Nodes involing AstText
    শূন্য visit(AstText* nodep) override {
        // All Text should be under TextBlock/CStmt/CStmtUser/CExpr/CExprUser
        nodep->v3fatalSrc("Text node in unexpected position");
    }
    শূন্য visit(AstTextBlock* nodep) override {
        putnbs(nodep, "");
        puts(nodep->prefix());
        emitNodesWithText(nodep->nodesp(), false, true, nodep->separator());
        puts(nodep->suffix());
    }
    শূন্য visit(AstCStmt* nodep) override {
        putnbs(nodep, "");
        emitNodesWithText(nodep->nodesp(), false, true, "");
        ensure_New_সারি();
    }
    শূন্য visit(AstCExpr* nodep) override {
        putnbs(nodep, "");
        emitNodesWithText(nodep->nodesp(), false, true, "");
    }
    শূন্য visit(AstCStmtUser* nodep) override {
        putnbs(nodep, "");
        ofp()->putsNoTracking("\n");
        যদি (nodep->fromDollarC() && v3Global.opt.decoration() && !v3Global.opt.protectIds()) {
            ofp()->putsNoTracking("// $c statement at " + nodep->fileline()->ascii() + "\n");
        }
        emitNodesWithText(nodep->nodesp(), m_useSelfForThis, false, "");
        puts("\n");
    }
    শূন্য visit(AstCExprUser* nodep) override {
        putnbs(nodep, "");
        ofp()->putsNoTracking("\n");
        যদি (/* is always from $c */ v3Global.opt.decoration() && !v3Global.opt.protectIds()) {
            ofp()->putsNoTracking(
                (nodep->isPure() ? "// $cpure expression at " : "// $c expression at ")
                + nodep->fileline()->ascii() + "\n");
        }
        emitNodesWithText(nodep->nodesp(), m_useSelfForThis, false, "");
        puts("\n");
    }

    // Operators
    শূন্য visit(Ast_গ্রন্থি_Termop* nodep) override {
        emitOpName(nodep, nodep->emitC(), nullptr, nullptr, nullptr);
    }
    শূন্য visit(Ast_গ্রন্থি_Uniop* nodep) override {
        যদি (nodep->emit_Check_Max_শব্দগুলি()
            && (nodep->width_শব্দগুলি() > VL_MULS_MAX_শব্দগুলি
                || nodep->lhsp()->width_শব্দগুলি() > VL_MULS_MAX_শব্দগুলি)) {
            nodep->v3warn(
                E_UNSUPPORTED,
                "Unsupported: "
                    << nodep->prettyOperatorName() << " operator of " << nodep->width()
                    << " bits exceeds hardcoded limit VL_MULS_MAX_WORDS in verilatedos.h");
        }
        যদি (emitSimpleOk(nodep)) {
            putnbs(nodep, "(");
            puts(nodep->emitSimpleOperator());
            puts(" ");
            iterateAndNextConstNull(nodep->lhsp());
            puts(")");
        } অন্যথা {
            emitOpName(nodep, nodep->emitC(), nodep->lhsp(), nullptr, nullptr);
        }
    }
    শূন্য visit(Ast_গ্রন্থি_Biop* nodep) override {
        যদি (nodep->emit_Check_Max_শব্দগুলি() && nodep->width_শব্দগুলি() > VL_MULS_MAX_শব্দগুলি) {
            nodep->v3warn(
                E_UNSUPPORTED,
                "Unsupported: "
                    << nodep->prettyOperatorName() << " operator of " << nodep->width()
                    << " bits exceeds hardcoded limit VL_MULS_MAX_WORDS in verilatedos.h");
        }
        যদি (emitSimpleOk(nodep)) {
            putnbs(nodep, "(");
            iterateAndNextConstNull(nodep->lhsp());
            puts(" ");
            putbs(nodep->emitSimpleOperator());
            puts(" ");
            iterateAndNextConstNull(nodep->rhsp());
            puts(")");
        } অন্যথা {
            emitOpName(nodep, nodep->emitC(), nodep->lhsp(), nodep->rhsp(), nullptr);
        }
    }
    শূন্য visit(Ast_গ্রন্থি_Triop* nodep) override {
        UASSERT_OBJ(!emitSimpleOk(nodep), nodep, "Triop cannot be described in a simple way");
        emitOpName(nodep, nodep->emitC(), nodep->lhsp(), nodep->rhsp(), nodep->thsp());
    }
    শূন্য visit(AstCvtPackString* nodep) override { emitCvtPackStr(nodep->lhsp()); }
    শূন্য visit(AstRedXor* nodep) override {
        যদি (nodep->lhsp()->isWide()) {
            visit(static_cast<Ast_গ্রন্থি_Uniop*>(nodep));
        } অন্যথা {
            ধ্রুব AstVarRef* ধ্রুব vrefp = VN_CAST(nodep->lhsp(), VarRef);
            ধ্রুব পূর্ণাঙ্ক widthPow2 = vrefp ? vrefp->varp()->dtypep()->widthPow2()
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
    শূন্য visit(AstCCast* nodep) override {
        // Extending a value of the same word width is just a NOP.
        যদি (ধ্রুব AstClassRefDType* ধ্রুব classDtypep
            = VN_CAST(nodep->dtypep()->skipRefp(), ClassRefDType)) {
            putns(nodep, "(" + classDtypep->cType("", false, false) + ")(");
        } অন্যথা যদি (nodep->size() <= VL_BYTESIZE) {
            putns(nodep, "(::CData)(");
        } অন্যথা যদি (nodep->size() <= VL_SHORTSIZE) {
            putns(nodep, "(::SData)(");
        } অন্যথা যদি (nodep->size() <= VL_IDATASIZE) {
            putns(nodep, "(::IData)(");
        } অন্যথা {
            putns(nodep, "(::QData)(");
        }
        iterateAndNextConstNull(nodep->lhsp());
        puts(")");
    }
    শূন্য visit(AstCond* nodep) override {
        // Widths match up already, so we'll just use C++'s operator w/o any temps.
        যদি (nodep->thenp()->isWide()) {
            emitOpName(nodep, nodep->emitC(), nodep->condp(), nodep->thenp(), nodep->elsep());
        } অন্যথা {
            putnbs(nodep, "(");
            iterateAndNextConstNull(nodep->condp());
            putbs(" ? ");
            iterateAndNextConstNull(nodep->thenp());
            putbs(" : ");
            iterateAndNextConstNull(nodep->elsep());
            puts(")");
        }
    }
    শূন্য visit(AstMatchMasked* nodep) override {
        emitOpName(nodep, nodep->emitC(), nodep->lhsp(), nodep->matchp(), nullptr);
    }
    শূন্য visit(AstMemberSel* nodep) override {
        ধ্রুব AstVar* ধ্রুব varp = nodep->varp();
        ধ্রুব bool dereferenceCovergroupRef
            = varp->covergroupRefMember() && nodep->access().isReadOrRW();
        যদি (dereferenceCovergroupRef) putnbs(nodep, "(*");
        iterateAndNextConstNull(nodep->fromp());
        putnbs(nodep, "->");
        যদি (varp->isIfaceRef()) {
            // varp is the __Viftop companion (e.g. "tx__Viftop"); use the
            // MemberSel name which matches the cell's C++ member (e.g. "tx").
            puts(nodep->nameProtect());
        } অন্যথা {
            puts(varName(varp));
        }
        যদি (dereferenceCovergroupRef) puts(")");
    }
    শূন্য visit(AstStructSel* nodep) override {
        iterateAndNextConstNull(nodep->fromp());
        putnbs(nodep, ".");
        puts(nodep->nameProtect());
    }
    শূন্য visit(AstNullCheck* nodep) override {
        putns(nodep, "VL_NULL_CHECK(");
        iterateAndNextConstNull(nodep->lhsp());
        puts(", ");
        putsQuoted(protect(nodep->fileline()->filename()));
        puts(", ");
        puts(cvtToStr(nodep->fileline()->lineno()));
        puts(")");
    }
    শূন্য visit(AstNewCopy* nodep) override {
        // Polymorphic shallow clone: preserves runtime type via virtual clone()
        // VL_NULL_CHECK enforces null check per IEEE 1800-2023 8.7
        putns(nodep, "VL_NULL_CHECK(");
        যদি (VN_IS(nodep->rhsp(), Const) && VN_AS(nodep->rhsp(), Const)->isNull()) {
            // V3Const folded rhs to null: emit a typed empty ref so VL_NULL_CHECK fires
            ধ্রুব AstClassRefDType* ধ্রুব refDTypep
                = VN_CAST(nodep->dtypep()->skipRefp(), ClassRefDType);
            puts(refDTypep->cType("", false, false) + "{}");
        } অন্যথা {
            iterateAndNextConstNull(nodep->rhsp());
        }
        puts(", ");
        putsQuoted(protect(nodep->fileline()->filename()));
        puts(", ");
        puts(cvtToStr(nodep->fileline()->lineno()));
        puts(").clone(vlSymsp->__Vm_deleter)");
    }
    শূন্য visit(AstSel* nodep) override {
        // Note ASSIGN checks for this on a LHS
        UASSERT_OBJ(nodep->widthMin() == nodep->widthConst(), nodep, "Width mismatch");
        emitOpName(nodep, nodep->emitC(), nodep->fromp(), nodep->lsbp(), nullptr);
    }
    শূন্য visit(AstReplicate* nodep) override {
        যদি (nodep->srcp()->widthMin() == 1 && !nodep->isWide()) {
            UASSERT_OBJ((static_cast<পূর্ণাঙ্ক>(VN_AS(nodep->countp(), Const)->toUInt())
                         * nodep->srcp()->widthMin())
                            == nodep->widthMin(),
                        nodep, "Replicate non-constant or width miscomputed");
            putns(nodep, "VL_REPLICATE_");
            emitIQW(nodep);
            puts("OI(");
            যদি (nodep->srcp()) puts(cvtToStr(nodep->srcp()->widthMin()));
            puts(", ");
            iterateAndNextConstNull(nodep->srcp());
            puts(", ");
            iterateAndNextConstNull(nodep->countp());
            puts(")");
        } অন্যথা {
            emitOpName(nodep, nodep->emitC(), nodep->srcp(), nodep->countp(), nullptr);
        }
    }
    শূন্য emitStreamR(AstStreamR* nodep, Ast_গ্রন্থি* parent) {
        //TODO: This might need to handle more cases like the visit(AstStreamR) function
        emitOpName(nodep, nodep->emitC(), nodep->lhsp(), nodep->rhsp(), nullptr);
    }
    শূন্য visit(AstStreamR* nodep) override {
        //The parrent node of our AstStreamR will give just enough info for what streamR should
        //output if nodep->backp() is not the parent then emitStreamR should have been used. throw
        //an error
        bool backpIsParent = (nodep->backp()->op1p() == nodep || nodep->backp()->op2p() == nodep);
        UASSERT(backpIsParent, "can not find return type for streamR");
        যদি (ধ্রুব AstQueueDType* ধ্রুব qdtp
            = VN_CAST(nodep->backp()->dtypep()->skipRefp(), QueueDType)) {
            যদি (nodep->lhsp()->isWide()) {
                emitOpName(nodep,
                           "VL_STREAMR_%nq%lq%rq(%lw," + std::to_string(qdtp->subDTypep()->width())
                               + ", %P, %li, %ri)",
                           nodep->lhsp(), nodep->rhsp(), nullptr);
            } অন্যথা {
                emitOpName(nodep, "VL_STREAMR_%nq%lq%rq(%lw, %P, %li, %ri)", nodep->lhsp(),
                           nodep->rhsp(), nullptr);
            }
        } অন্যথা যদি (VN_IS(nodep->lhsp()->dtypep()->skipRefp(), QueueDType)) {
            যদি (!((nodep->backp()->op1p() && nodep->backp()->op1p()->isWide())
                  || (nodep->backp()->op2p() && nodep->backp()->op2p()->isWide()))) {
                //If our lhsp is a queue make sure we streamR and return the correct type.
                //If either side is wide or the previous node is string type dont use this case
                emitOpName(nodep->backp(), "VL_STREAMR_%nq%lq%rq(%lw, %P, %li, %ri)",
                           nodep->lhsp(), nodep->rhsp(), nullptr);
            } অন্যথা {
                emitOpName(nodep, nodep->emitC(), nodep->lhsp(), nodep->rhsp(), nullptr);
            }
        } অন্যথা {
            emitOpName(nodep, nodep->emitC(), nodep->lhsp(), nodep->rhsp(), nullptr);
        }
    }
    শূন্য visit(AstStreamL* nodep) override {
        // Attempt to use a "fast" stream function for slice size = power of 2
        যদি (!nodep->isWide()) {
            ধ্রুব uint32_t isPow2 = VN_AS(nodep->rhsp(), Const)->অঙ্ক().গুনতি_Ones() == 1;
            ধ্রুব uint32_t sliceSize = VN_AS(nodep->rhsp(), Const)->toUInt();
            যদি (isPow2 && sliceSize <= (nodep->isQuad() ? পরিমাণ(uint64_t) : পরিমাণ(uint32_t))) {
                putns(nodep, "VL_STREAML_FAST_");
                bool usesQueue = false;
                AstQueueDType* qtypep
                    = nodep->backp()->op2p()
                          ? VN_CAST(nodep->backp()->op2p()->dtypep()->skipRefp(), QueueDType)
                          : nullptr;
                যদি (VN_IS(nodep->backp(), Assign)
                    && qtypep) {  // If we are assigning to a queue then emit the correct symbol
                    puts("R");  // R for queue
                    usesQueue = true;
                } অন্যথা {
                    emitIQW(nodep);
                }
                emitIQW(nodep->lhsp());
                puts("I(");
                puts(cvtToStr(nodep->lhsp()->widthMin()));
                puts(", ");
                যদি (usesQueue) {
                    iterateAndNextConstNull(nodep->backp()->op2p());
                    puts(", ");
                }
                iterateAndNextConstNull(nodep->lhsp());
                puts(", ");
                ধ্রুব uint32_t rd_log2 = V3_সংখ্যা::log2b(VN_AS(nodep->rhsp(), Const)->toUInt());
                puts(cvtToStr(rd_log2) + ")");
                প্রত্যাগম;
            }
        }
        যদি (VN_IS(nodep->backp(), Assign)
            && VN_IS(nodep->backp()->op2p()->dtypep()->skipRefp(), QueueDType)) {
            পূর্ণাঙ্ক queueWidth
                = nodep->backp()->op2p()->dtypep()->subDTypep()->width();  //We need to know the
                                                                           //width of both sides
            emitOpName(nodep,
                       "VL_STREAML_%nq%lq%rq(%lw," + std::to_string(queueWidth)
                           + ", %P, %li, %ri)",
                       nodep->lhsp(), nodep->rhsp(), nullptr);
        } অন্যথা {
            emitOpName(nodep, nodep->emitC(), nodep->lhsp(), nodep->rhsp(), nullptr);
        }
    }
    শূন্য visit(AstCastDynamic* nodep) override {
        putnbs(nodep, "VL_CAST_DYNAMIC(");
        iterateAndNextConstNull(nodep->fromp());
        puts(", ");
        iterateAndNextConstNull(nodep->শীর্ষ());
        puts(")");
    }
    শূন্য visit(Ast_গুনতি_Bits* nodep) override {
        putnbs(nodep, "VL_COUNTBITS_");
        emitIQW(nodep->lhsp());
        puts("(");
        puts(cvtToStr(nodep->lhsp()->widthMin()));
        puts(", ");
        যদি (nodep->lhsp()->isWide()) {
            puts(cvtToStr(nodep->lhsp()->width_শব্দগুলি()));  // Note argument width, not node width
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
    শূন্য visit(AstTriggered* nodep) override {
        // Test each word of the trigger vector with a mask of the relevant bits
        std::map<uint32_t, uint64_t> wordMasks;
        প্রতি (ধ্রুব uint32_t index : nodep->indices()) {
            wordMasks[VL_BITWORD_Q(index)] |= VL_BIT_Q(index);
        }
        putns(nodep, "((");
        bool first = true;
        প্রতি (ধ্রুব auto& wordMask : wordMasks) {
            যদি (!first) puts("\n| ");
            first = false;
            ধ্রুব bool allOnes = wordMask.second == ~0ULL;
            যদি (!allOnes) puts("(" + V3_সংখ্যা{nodep, 64}.setQuad(wordMask.second).emitC() + " & ");
            iterateConst(nodep->trigRefp());
            puts("[" + std::to_string(wordMask.first) + "U]");
            যদি (!allOnes) puts(")");
        }
        puts(") != 0ULL)");
    }
    // Terminals
    শূন্য visit(AstVarRef* nodep) override {
        ধ্রুব AstVar* ধ্রুব varp = nodep->varp();

        // Reference to constant pool variable is special
        যদি (varp->constPoolEntry()) {
            putns(nodep, EmitCUtil::constPoolName(varp));
            প্রত্যাগম;
        }

        ধ্রুব Ast_গ্রন্থি_Module* ধ্রুব varModp = EmitCParentModule::get(varp);
        ধ্রুব bool dereferenceCovergroupRef
            = varp->covergroupRefMember() && nodep->access().isReadOrRW();
        যদি (dereferenceCovergroupRef) putns(nodep, "(*");
        যদি (VN_IS(varModp, Class) && varModp != m_modp) {
            // Superclass member reference
            putns(nodep, EmitCUtil::prefixNameProtect(varModp) + "::");
        } অন্যথা যদি (varp->isIfaceRef()) {
            putns(nodep, nodep->selfPointerProtect(m_useSelfForThis));
            প্রত্যাগম;
        } অন্যথা যদি (!nodep->selfPointer().isEmpty()) {
            emitDereference(nodep, nodep->selfPointerProtect(m_useSelfForThis));
        }
        putns(nodep, varName(varp));
        যদি (dereferenceCovergroupRef) puts(")");
    }
    শূন্য visit(AstAddrOfCFunc* nodep) override {
        // Note: Can be thought to handle more, but this is all that is needed right now
        ধ্রুব AstCFunc* ধ্রুব funcp = nodep->funcp();
        UASSERT_OBJ(funcp->isLoose(), nodep, "Cannot take address of non-loose method");
        putns(nodep, "&");
        puts(funcNameProtect(funcp));
    }
    শূন্য visit(AstConst* nodep) override {  //
        যদি (m_wideTempRefp && nodep->isWide()) {
            UASSERT_OBJ(m_wideTempRefp, nodep, "Wide Constant w/ no temp");
            emitConstantW(nodep, m_wideTempRefp);
            m_wideTempRefp = nullptr;  // We used it, fail if set it a second time
        } অন্যথা {
            emitConstant(nodep);
        }
    }
    শূন্য visit(Ast_গ্রন্থি_Cons* nodep) override { emitValCons(nodep); }
    শূন্য visit(AstThisRef* nodep) override {
        putnbs(nodep, nodep->dtypep()->cType("", false, false));
        puts("{");
        puts(VSelfPointerText::replaceThis(m_useSelfForThis, "this"));
        puts("}");
    }
    শূন্য visit(Ast_গ্রন্থি_Sel* nodep) override {
        যদি (!VN_IS(nodep, ArraySel) && !VN_IS(nodep, WordSel)) {
            visit(static_cast<Ast_গ্রন্থি_Biop*>(nodep));
            প্রত্যাগম;
        }
        // ArraySel or WordSel
        iterateAndNextConstNull(nodep->fromp());
        // Special case constant index for readability
        যদি (AstConst* ধ্রুব idxp = VN_CAST(nodep->bitp(), Const)) {
            puts("[" + std::to_string(idxp->toUInt()) + "U]");
            প্রত্যাগম;
        }
        putbs("[");
        iterateAndNextConstNull(nodep->bitp());
        puts("]");
    }

    //
    শূন্য visit(AstExecGraph* nodep) override {
        // The location of the AstExecGraph within the containing AstCFunc is where we want to
        // invoke the graph and wait for it to complete. The run-time library does just that.
        putns(nodep, "vl_invokeExecGraph(" + EmitCUtil::execGraphName(nodep) + ", vlSelf, ");
        iterateConst(nodep->triggerp());
        puts(".data());\n");
    }

    // Default
    শূন্য visit(Ast_গ্রন্থি* nodep) override {  // LCOV_EXCL_START
        putns(nodep, "\n???? // "s + nodep->prettyTypeName() + "\n");
        iterateChildrenConst(nodep);
        যদি (!v3Global.opt.lintOnly()) {  // An internal problem, so suppress
            nodep->v3fatalSrc("Unknown node type reached emitter: " << nodep->prettyTypeName());
        }
    }  // LCOV_EXCL_STOP

রক্ষিত:
    EmitCFunc() = স্বভাব;
    ~EmitCFunc() override = স্বভাব;
};

#endif  // guard
