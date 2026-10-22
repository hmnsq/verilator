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

#ଅନ୍ତର୍ଭାଵ "config_build.h"
#ଅନ୍ତର୍ଭାଵ "verilatedos.h"

#ଅନ୍ତର୍ଭାଵ "V3EmitCConstInit.h"
#ଅନ୍ତର୍ଭାଵ "V3Global.h"
#ଅନ୍ତର୍ଭାଵ "V3MemberMap.h"

#ଅନ୍ତର୍ଭାଵ <algorithm>
#ଅନ୍ତର୍ଭାଵ <map>
#ଅନ୍ତର୍ଭାଵ <unordered_map>
#ଅନ୍ତର୍ଭାଵ <unordered_set>
#ଅନ୍ତର୍ଭାଵ <vector>

// Number of VL_CONST_W_*X's in verilated.h (IE VL_CONST_W_8X is last)
constexpr ପୂର୍ଣ୍ଣସଂଖ୍ୟା EMITC_ଅଙ୍କ_CONSTW = 8;

//######################################################################
// Emit lazy forward declarations

class EmitCLazyDecls final : ସର୍ବସାଧାରଣ VNVisitorConst {
    // NODE STATE/TYPES
    // None allowed to support threaded emitting

    // MEMBERS
    std::unordered_set<string> m_emittedManually;  // Set of names already declared manually.
    EmitCBaseVisitorConst& m_emitter;  // For access to file output
    bool m_needs_Blank_ଧାଡ଼ି = false;  // Emit blank line if any declarations were emitted (cosmetic)
    std::set<Ast_ଗଣ୍ଠି*> m_emitted;  // -> in set. Already emitted decl for symbols.

    // METHODS
    bool declaredOnce(Ast_ଗଣ୍ଠି* nodep) { ଫେରନ୍ତୁ m_emitted.ଭର୍ତ୍ତି(nodep).second; }

    ଶୂନ୍ୟ lazyDeclare(AstCFunc* funcp) {
        // Already declared in this compilation unit
        ଯଦି (!declaredOnce(funcp)) ଫେରନ୍ତୁ;
        // Check if this kind of function is lazily declared
        ଯଦି (!(funcp->isMethod() && funcp->isLoose()) && !funcp->dpiImportPrototype()) ଫେରନ୍ତୁ;
        // Already declared manually
        ଯଦି (m_emittedManually.ଗଣନା_2(funcp->nameProtect())) ଫେରନ୍ତୁ;
        // Needs lazy declaration, emit one
        m_emitter.emitCFuncDecl(funcp, EmitCParentModule::get(funcp), funcp->dpiImportPrototype());
        m_needs_Blank_ଧାଡ଼ି = true;
    }

    ଶୂନ୍ୟ lazyDeclareConstPoolVar(AstVar* varp) {
        ଯଦି (!declaredOnce(varp)) ଫେରନ୍ତୁ;  // Already declared
        ଅଚଳ string nameProtect = EmitCUtil::constPoolName(varp);
        m_emitter.putns(varp, "extern const ");
        m_emitter.puts(varp->dtypep()->cType(nameProtect, false, false));
        m_emitter.puts(";\n");
        m_needs_Blank_ଧାଡ଼ି = true;
    }

    // VISITORS
    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି_CCall* nodep) override {
        lazyDeclare(nodep->funcp());
        iterateChildrenConst(nodep);
    }

    ଶୂନ୍ୟ visit(AstAddrOfCFunc* nodep) override {
        lazyDeclare(nodep->funcp());
        iterateChildrenConst(nodep);
    }

    ଶୂନ୍ୟ visit(AstVarRef* nodep) override {
        AstVar* ଅଚଳ varp = nodep->varp();
        // Only constant pool symbols are lazy declared for now ...
        ଯଦି (EmitCParentModule::get(varp)->isConstPool()) lazyDeclareConstPoolVar(varp);
    }

    ଶୂନ୍ୟ visit(AstExecGraph* nodep) override {
        ଯଦି (!declaredOnce(nodep)) ଫେରନ୍ତୁ;
        m_emitter.putns(nodep, "extern VlExecGraph " + EmitCUtil::execGraphName(nodep) + ";\n");
        m_needs_Blank_ଧାଡ଼ି = true;
    }

    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି* nodep) override { iterateChildrenConst(nodep); }

ସର୍ବସାଧାରଣ:
    explicit EmitCLazyDecls(EmitCBaseVisitorConst& emitter)
        : m_emitter{emitter} {}
    ଶୂନ୍ୟ emit(Ast_ଗଣ୍ଠି* nodep) {
        m_needs_Blank_ଧାଡ଼ି = false;
        iterateChildrenConst(nodep);
        ଯଦି (m_needs_Blank_ଧାଡ଼ି) m_emitter.puts("\n");
    }
    ଶୂନ୍ୟ emit(ଅଚଳ string& prefix, ଅଚଳ string& name, ଅଚଳ string& suffix) {
        m_emittedManually.ଭର୍ତ୍ତି(name);
        m_emitter.ensure_New_ଧାଡ଼ି();
        m_emitter.puts(prefix);
        m_emitter.puts(name);
        m_emitter.puts(suffix);
        m_emitter.ensure_New_ଧାଡ଼ି();
    }
    ଶୂନ୍ୟ declared(AstCFunc* nodep) { m_emitted.ଭର୍ତ୍ତି(nodep); }
    ଶୂନ୍ୟ reset() { m_emitted.clear(); }
};

// ######################################################################
//  Emit statements and expressions

class EmitCFunc VL_NOT_FINAL : ସର୍ବସାଧାରଣ EmitCConstInit {
    VMemberMap m_memberMap;
    AstVarRef* m_wideTempRefp = nullptr;  // Variable that _WW macros should be setting
    std::unordered_map<AstJumpBlock*, size_t> m_labelNumbers;  // Label numbers for AstJumpBlocks
    bool m_createdScopeHash = false;  // Already created a scope hash

ସୁରକ୍ଷିତ:
    VL_DEFINE_DEBUG_FUNCTIONS;

    EmitCLazyDecls m_lazyDecls{*this};  // Visitor for emitting lazy declarations
    bool m_useSelfForThis = false;  // Replace "this" with "vlSelf"
    bool m_usevlSelfRef = false;  // Use vlSelfRef reference instead of vlSelf pointer
    ଅଚଳ Ast_ଗଣ୍ଠି_Module* m_modp = nullptr;  // Current module being emitted
    ଅଚଳ AstCFunc* m_cfuncp = nullptr;  // Current function being emitted
    bool m_instantiatesOwnProcess = false;
    ଅଚଳ AstClassPackage* m_classOrPackage = nullptr;  // Pointer to current class or package
    string m_classOrPackageHash;  // Hash of class or package name

    bool constructorNeedsProcess(ଅଚଳ AstClass* ଅଚଳ classp) {
        ଅଚଳ Ast_ଗଣ୍ଠି* ଅଚଳ newp = m_memberMap.findMember(classp, "new");
        ଯଦି (!newp) ଫେରନ୍ତୁ false;
        ଅଚଳ AstCFunc* ଅଚଳ ctorp = VN_CAST(newp, CFunc);
        ଯଦି (!ctorp) ଫେରନ୍ତୁ false;
        UASSERT_OBJ(ctorp->isConstructor(), ctorp, "`new` is not a constructor!");
        ଫେରନ୍ତୁ ctorp->needProcess();
    }

    bool constructorNeedsProcess(ଅଚଳ Ast_ଗଣ୍ଠି_DType* ଅଚଳ dtypep) {
        ଯଦି (ଅଚଳ AstClassRefDType* ଅଚଳ crefdtypep = VN_CAST(dtypep, ClassRefDType))
            ଫେରନ୍ତୁ constructorNeedsProcess(crefdtypep->classp());
        ଫେରନ୍ତୁ false;
    }

    bool coverageUsesLocalCounter(Ast_ଗଣ୍ଠି_Cover_Decl* ଅଚଳ declp) ଅଚଳ {
        // Only functions with an object receiver can address the module-local
        // coverage array. Static/package/class helper functions still use the
        // symbol-table array, as they may have vlSymsp but no vlSelf/this.
        // Also require the coverage declaration to belong to the current
        // module; cloned task bodies can increment declarations owned by a
        // different module, and those must keep using the global array.
        ଫେରନ୍ତୁ m_cfuncp && !m_cfuncp->isStatic() && !VN_IS(m_modp, Class)
               && !VN_IS(m_modp, ClassPackage)
               && EmitCParentModule::get(declp->dataDeclThisp()) == m_modp;
    }
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା coverage_Bin_ଅଙ୍କ(Ast_ଗଣ୍ଠି_Cover_Decl* ଅଚଳ declp, bool forceGlobal = false) ଅଚଳ {
        ଫେରନ୍ତୁ !forceGlobal && coverageUsesLocalCounter(declp)
                   ? declp->dataDeclThisp()->local_Bin_ଅଙ୍କ()
                   : declp->dataDeclThisp()->bin_ଅଙ୍କ();
    }
    ଶୂନ୍ୟ putCoverageArray(Ast_ଗଣ୍ଠି_Cover_Decl* ଅଚଳ declp, bool forceGlobal = false) {
        ଯଦି (!forceGlobal && coverageUsesLocalCounter(declp)) {
            // Keep counters on the module object when possible, so every
            // no-inline instance has storage available if coverage output is
            // later written with forcePerInstance.
            puts(m_cfuncp->isLoose() ? "vlSelf->__Vcoverage" : "this->__Vcoverage");
        } ଅନ୍ୟଥା {
            // Static/package/class helper functions do not have vlSelf, so
            // keep using the shared symbol-table coverage array there.
            puts("vlSymsp->__Vcoverage");
        }
    }
    ଶୂନ୍ୟ emit_Cover_Other_Decl_ଭର୍ତ୍ତି(AstCoverOtherDecl* nodep, bool forceGlobal = false) {
        putns(nodep, "vlSelf->__vlCoverInsert(");  // As Declared in emitCoverageDecl
        putCoverageArray(nodep, forceGlobal);
        puts(" + ");
        puts(cvtToStr(coverage_Bin_ଅଙ୍କ(nodep, forceGlobal)));
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
                   + VIdProtect::protect_ଶବ୍ଦଗୁଡ଼ିକ_If(nodep->hier(), nodep->protect()));
        puts(", ");
        putsQuoted(VIdProtect::protect_ଶବ୍ଦଗୁଡ଼ିକ_If(nodep->page(), nodep->protect()));
        puts(", ");
        putsQuoted(VIdProtect::protect_ଶବ୍ଦଗୁଡ଼ିକ_If(nodep->comment(), nodep->protect()));
        puts(", ");
        putsQuoted(nodep->linescov());
        puts(", ");
        putsQuoted(VIdProtect::protect_ଶବ୍ଦଗୁଡ଼ିକ_If(nodep->fsmVar(), nodep->protect()));
        puts(", ");
        putsQuoted(VIdProtect::protect_ଶବ୍ଦଗୁଡ଼ିକ_If(nodep->fsmFrom(), nodep->protect()));
        puts(", ");
        putsQuoted(VIdProtect::protect_ଶବ୍ଦଗୁଡ଼ିକ_If(nodep->fsmTo(), nodep->protect()));
        puts(", ");
        putsQuoted(VIdProtect::protect_ଶବ୍ଦଗୁଡ଼ିକ_If(nodep->fsmTag(), nodep->protect()));
        puts(");\n");
    }

ସର୍ବସାଧାରଣ:
    // METHODS
    bool displayEmitHeader(Ast_ଗଣ୍ଠି* nodep);
    ଶୂନ୍ୟ display_ଗଣ୍ଠି(Ast_ଗଣ୍ଠି* nodep, AstSFormatF* fmtp, ଅଚଳ string& vformat, Ast_ଗଣ୍ଠି* exprsp,
                     bool isScan);

    bool emitSimpleOk(Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* nodep);
    ଶୂନ୍ୟ emitIQW(ଅଚଳ Ast_ଗଣ୍ଠି* nodep) {
        // See "Type letters" in verilated.h
        // Other abbrevs: "C"har, "S"hort, "F"loat, "D"ouble, stri"N"g, "R"=queue, "U"npacked
        puts(nodep->dtypep()->skipRefp()->charIQWN());
    }
    ଶୂନ୍ୟ emitRU(ଅଚଳ Ast_ଗଣ୍ଠି* nodep) {
        ଅଚଳ Ast_ଗଣ୍ଠି_DType* dtp = nodep->dtypep()->skipRefp();
        // See "Type letters" in verilated.h
        ଯଦି (VN_IS(dtp, UnpackArrayDType))
            puts("U");
        ଅନ୍ୟଥା ଯଦି (VN_IS(dtp, QueueDType) || VN_IS(dtp, DynArrayDType))
            puts("R");
    }
    ଶୂନ୍ୟ emitScIQW(AstVar* nodep) {
        UASSERT_OBJ(nodep->isSc(), nodep, "emitting SystemC operator on non-SC variable");
        puts(nodep->isScBigUint()                           ? "SB"
             : (nodep->isScUint() || nodep->isScUintBool()) ? "SU"
             : nodep->isScBv()                              ? "SW"
             : nodep->isDouble()                            ? "SD"
                                                            : (nodep->isScQuad() ? "SQ" : "SI"));
    }
    ଶୂନ୍ୟ emitDatap(ଅଚଳ Ast_ଗଣ୍ଠି* nodep) {
        // When passing to a function with va_args the compiler doesn't
        // know need a pointer so when wide, need to look inside VlWide
        ଯଦି (nodep->isWide()) puts(".data()");
    }
    ଶୂନ୍ୟ emitOpName(Ast_ଗଣ୍ଠି* nodep, ଅଚଳ string& format, Ast_ଗଣ୍ଠି* lhsp, Ast_ଗଣ୍ଠି* rhsp,
                    Ast_ଗଣ୍ଠି* thsp);
    ଶୂନ୍ୟ emitCCallArgs(ଅଚଳ Ast_ଗଣ୍ଠି_CCall* nodep, ଅଚଳ string& selfPointer, bool inProcess);
    ଶୂନ୍ୟ emitDereference(Ast_ଗଣ୍ଠି* nodep, ଅଚଳ string& pointer);
    std::string dereferenceString(ଅଚଳ std::string& pointer) ଅଚଳ;
    ଶୂନ୍ୟ emitCvtPackStr(Ast_ଗଣ୍ଠି* nodep);
    ଶୂନ୍ୟ emitConstant(AstConst* nodep);
    ଶୂନ୍ୟ emitConstantString(ଅଚଳ AstConst* nodep);
    ଶୂନ୍ୟ emitVarReset(ଅଚଳ string& prefix, AstVar* varp, bool constructing);
    string emitVarResetRecurse(ଅଚଳ AstVar* varp, bool constructing,
                               ଅଚଳ string& varNameProtected, Ast_ଗଣ୍ଠି_DType* dtypep, ପୂର୍ଣ୍ଣସଂଖ୍ୟା depth,
                               ଅଚଳ string& suffix, ଅଚଳ Ast_ଗଣ୍ଠି* valuep);
    ଶୂନ୍ୟ emitVarResetScopeHash();
    ଶୂନ୍ୟ emitChangeDet();
    ଶୂନ୍ୟ putCommaIterateNext(Ast_ଗଣ୍ଠି* nodep, bool comma = false) {
        ପାଇଁ (Ast_ଗଣ୍ଠି* subnodep = nodep; subnodep; subnodep = subnodep->nextp()) {
            ଯଦି (comma) puts(", ");
            iterateConst(subnodep);
            comma = true;
        }
    }
    template <typename T>
    string optionalProcArg(ଅଚଳ T* ଅଚଳ nodep) {
        ଫେରନ୍ତୁ (nodep && constructorNeedsProcess(nodep)) ? "vlProcess, " : "";
    }
    ଅଚଳ AstCNew* getSuperNewCallRecursep(Ast_ଗଣ୍ଠି* ଅଚଳ nodep) {
        // Get the super.new call
        ଯଦି (!nodep) ଫେରନ୍ତୁ nullptr;
        ଯଦି (ଅଚଳ AstCNew* ଅଚଳ cnewp = VN_CAST(nodep, CNew)) ଫେରନ୍ତୁ cnewp;
        ଯଦି (ଅଚଳ Ast_Stmt_ପଦସମଷ୍ଟି* ଅଚଳ stmtp = VN_CAST(nodep, Stmt_ପଦସମଷ୍ଟି)) {
            ଯଦି (ଅଚଳ AstCNew* ଅଚଳ cnewp = VN_CAST(stmtp->exprp(), CNew)) ଫେରନ୍ତୁ cnewp;
        }
        ଯଦି (ଅଚଳ AstJumpBlock* ଅଚଳ blockp = VN_CAST(nodep, JumpBlock)) {
            ଯଦି (ଅଚଳ AstCNew* ଅଚଳ cnewp = getSuperNewCallRecursep(blockp->stmtsp())) {
                ଫେରନ୍ତୁ cnewp;
            }
        }
        ଯଦି (ଅଚଳ AstCNew* ଅଚଳ cnewp = getSuperNewCallRecursep(nodep->nextp())) ଫେରନ୍ତୁ cnewp;
        ଫେରନ୍ତୁ nullptr;
    }

    ଶୂନ୍ୟ emitConstantW(AstConst* nodep, AstVarRef* assigntop) {
        // For tradition and compilation speed, assign each word directly into
        // output variable instead of using '='
        putns(nodep, "");
        ଯଦି (nodep->ଅଙ୍କ().isFourState()) {
            nodep->v3warn(E_UNSUPPORTED, "Unsupported: 4-state numbers in this context");
            ଫେରନ୍ତୁ;
        }

        ପୂର୍ଣ୍ଣସଂଖ୍ୟା upWidth = nodep->ଅଙ୍କ().widthToFit();
        ପୂର୍ଣ୍ଣସଂଖ୍ୟା chunks = 0;
        ଯଦି (upWidth > EMITC_ଅଙ୍କ_CONSTW * VL_EDATASIZE) {
            // Output e.g. 8 words in groups of e.g. 8
            chunks = (upWidth - 1) / (EMITC_ଅଙ୍କ_CONSTW * VL_EDATASIZE);
            upWidth %= (EMITC_ଅଙ୍କ_CONSTW * VL_EDATASIZE);
            ଯଦି (upWidth == 0) upWidth = (EMITC_ଅଙ୍କ_CONSTW * VL_EDATASIZE);
        }
        {  // Upper e.g. 8 words
            ଯଦି (chunks) {
                putnbs(nodep, "VL_CONSTHI_W_");
                puts(cvtToStr(VL_ଶବ୍ଦଗୁଡ଼ିକ_I(upWidth)));
                puts("X(");
                puts(cvtToStr(nodep->widthMin()));
                puts(",");
                puts(cvtToStr(chunks * EMITC_ଅଙ୍କ_CONSTW * VL_EDATASIZE));
            } ଅନ୍ୟଥା {
                putnbs(nodep, "VL_CONST_W_");
                puts(cvtToStr(VL_ଶବ୍ଦଗୁଡ଼ିକ_I(upWidth)));
                puts("X(");
                puts(cvtToStr(nodep->widthMin()));
            }
            puts(",");
            ଯଦି (!assigntop->selfPointer().isEmpty()) {
                emitDereference(assigntop, assigntop->selfPointerProtect(m_useSelfForThis));
            }
            puts(varName(assigntop->varp()));
            ପାଇଁ (ପୂର୍ଣ୍ଣସଂଖ୍ୟା word = VL_ଶବ୍ଦଗୁଡ଼ିକ_I(upWidth) - 1; word >= 0; word--) {
                // Only 32 bits - llx + long long here just to appease CPP format warning
                ofp()->printf(",0x%08" PRIx64, static_cast<uint64_t>(nodep->ଅଙ୍କ().edataWord(
                                                   word + chunks * EMITC_ଅଙ୍କ_CONSTW)));
            }
            puts(")");
        }
        ପାଇଁ (chunks--; chunks >= 0; chunks--) {
            puts(";\n");
            putbs("VL_CONSTLO_W_");
            puts(cvtToStr(EMITC_ଅଙ୍କ_CONSTW));
            puts("X(");
            puts(cvtToStr(chunks * EMITC_ଅଙ୍କ_CONSTW * VL_EDATASIZE));
            puts(",");
            ଯଦି (!assigntop->selfPointer().isEmpty()) {
                emitDereference(assigntop, assigntop->selfPointerProtect(m_useSelfForThis));
            }
            puts(varName(assigntop->varp()));
            ପାଇଁ (ପୂର୍ଣ୍ଣସଂଖ୍ୟା word = EMITC_ଅଙ୍କ_CONSTW - 1; word >= 0; word--) {
                // Only 32 bits - llx + long long here just to appease CPP format warning
                ofp()->printf(",0x%08" PRIx64, static_cast<uint64_t>(nodep->ଅଙ୍କ().edataWord(
                                                   word + chunks * EMITC_ଅଙ୍କ_CONSTW)));
            }
            puts(")");
        }
    }

    ଶୂନ୍ୟ emitNodesWithText(Ast_ଗଣ୍ଠି* nodesp, bool useSelfForThis, bool tracking,
                           ଅଚଳ std::string& separator) {
        ପାଇଁ (Ast_ଗଣ୍ଠି* nodep = nodesp; nodep; nodep = nodep->nextp()) {
            ଯଦି (ଅଚଳ AstText* ଅଚଳ textp = VN_CAST(nodep, Text)) {
                ଅଚଳ std::string text
                    = VSelfPointerText::replaceThis(useSelfForThis, textp->text());
                ଯଦି (tracking) {
                    puts(text);
                } ଅନ୍ୟଥା {
                    ofp()->putsNoTracking(text);
                }
            } ଅନ୍ୟଥା {
                iterateConst(nodep);
            }
            ଯଦି (nodep->nextp()) puts(separator);
        }
    }

    ଶୂନ୍ୟ putConstructorSubinit(ଅଚଳ AstClass* classp, AstCFunc* cfuncp) {
        // Direct non-virtual bases in declaration order
        ପାଇଁ (ଅଚଳ AstClassExtends* extp = classp->extendsp(); extp;
             extp = VN_AS(extp->nextp(), ClassExtends)) {
            ଯଦି (extp->classp()->isInterfaceClass()) ଜାରି;
            ଯଦି (extp->classp()->useVirtualPublic()) ଜାରି;
            puts(EmitCUtil::prefixNameProtect(extp->classp()));
            puts("::init");
            ଯଦି (constructorNeedsProcess(extp->classp())) {
                puts("(vlProcess, vlSymsp");
            } ଅନ୍ୟଥା {
                puts("(vlSymsp");
            }
            // Handle super.new() args for the concrete parent
            ଯଦି (ଅଚଳ AstCNew* ଅଚଳ superNewCallp = getSuperNewCallRecursep(cfuncp->stmtsp())) {
                putCommaIterateNext(superNewCallp->argsp(), true);
            }
            puts(");\n");
            // Since SystemVerilog allows only for extending from one class, for sure there won't
            // be any more constructors to be called. Even if there are some classes which are
            // "implemented", those are interface classes which do not have constructors.
            ଭାଙ୍ଗ;
        }
    }

    // VISITORS
    using EmitCConstInit::visit;
    ଶୂନ୍ୟ visit(AstCFunc* ଅଚଳ nodep) override {
        ଯଦି (nodep->emptyBody() && !nodep->isLoose()) ଫେରନ୍ତୁ;
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
        ଯଦି (nodep->ifdef() != "") putns(nodep, "#ifdef " + nodep->ifdef() + "\n");
        emitCFuncHeader(nodep, m_modp, /* withScope: */ true);

        puts(" {\n");
        ଯଦି (nodep->isConstructor()) {
            ଅଚଳ AstClass* ଅଚଳ classp = VN_CAST(nodep->scopep()->modp(), Class);
            ଯଦି (classp && classp->extendsp()) putConstructorSubinit(classp, nodep);
        }

        // "+" in the debug indicates a print from the model
        puts("VL_DEBUG_IF(VL_DBG_MSGF(\"+  ");
        ପାଇଁ (ପୂର୍ଣ୍ଣସଂଖ୍ୟା i = 0; i < m_modp->level(); ++i) puts("  ");
        puts(EmitCUtil::prefixNameProtect(m_modp));
        puts(nodep->isLoose() ? "__" : "::");
        puts(nodep->nameProtect() + "\\n\"); );\n");

        ଯଦି (nodep->isLoose()) {
            m_lazyDecls.declared(nodep);  // Defined here, so no longer needs declaration
            ଯଦି (!nodep->isStatic()) {  // Standard prologue
                m_useSelfForThis = true;
                ଯଦି (nodep->voidSelf()) {
                    ଅଚଳ string className = EmitCUtil::prefixNameProtect(m_modp);
                    puts(className + "* const vlSelf = static_cast<" + className
                         + "*>(vlVoidSelf);\n");
                }
                ଯଦି (!VN_IS(m_modp, Class)) {
                    puts(EmitCUtil::symClassAssign());  // Uses vlSelf
                } ଅନ୍ୟଥା {
                    puts("(void)vlSelf;  // Prevent unused variable warning\n");
                }
            }
        }

        // Instantiate a process class if it's going to be needed somewhere later
        nodep->forall([&](ଅଚଳ Ast_ଗଣ୍ଠି_CCall* ccallp) -> bool {
            ଯଦି (ccallp->funcp()->needProcess()
                && (ccallp->funcp()->isCoroutine() == VN_IS(ccallp->backp(), CAwait))) {
                ଯଦି (!nodep->needProcess() && !m_instantiatesOwnProcess) {
                    m_instantiatesOwnProcess = true;
                    ଫେରନ୍ତୁ false;
                }
            }
            ଫେରନ୍ତୁ true;
        });
        ଯଦି (m_instantiatesOwnProcess) {
            AstCStmt* ଅଚଳ vlprocp = new AstCStmt{nodep->fileline()};
            vlprocp->add("VlProcessRef vlProcess = std::make_shared<VlProcess>();\n");
            vlprocp->add("VlProcess::currentp(vlProcess.get());");
            nodep->prependStmtsp(vlprocp);
        } ଅନ୍ୟଥା ଯଦି (nodep->needProcess() && nodep->stmtsp()) {
            // Set current process so VlRNG() constructors in this function seed from it
            AstCStmt* ଅଚଳ setProcessp = new AstCStmt{nodep->fileline()};
            setProcessp->add("VlProcess::currentp(vlProcess.get());");
            nodep->prependStmtsp(setProcessp);
        }

        ପାଇଁ (Ast_ଗଣ୍ଠି* subnodep = nodep->argsp(); subnodep; subnodep = subnodep->nextp()) {
            ଯଦି (AstVar* ଅଚଳ varp = VN_CAST(subnodep, Var)) {
                ଯଦି (varp->isFuncReturn()) emitVarDecl(varp);
            }
        }

        ଯଦି (m_useSelfForThis) {
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

        ଯଦି (nodep->varsp()) {
            putsDecoration(nodep, "// Locals\n");
            iterateAndNextConstNull(nodep->varsp());
        }

        ଯଦି (nodep->stmtsp()) {
            putsDecoration(nodep, "// Body\n");
            iterateAndNextConstNull(nodep->stmtsp());
        }

        m_usevlSelfRef = false;

        ଯଦି (nodep->isCoroutine()) {
            // Sometimes coroutines don't have co_awaits,
            // so emit a co_return at the end to avoid compile errors.
            puts("co_return;\n");
        }

        puts("}\n");
        ଯଦି (nodep->ifdef() != "") puts("#endif  // " + nodep->ifdef() + "\n");
    }

    ଶୂନ୍ୟ visit(AstVar* nodep) override {
        UASSERT_OBJ(m_cfuncp, nodep, "Cannot emit non-local variable");
        emitVarDecl(nodep);
    }

    ଶୂନ୍ୟ visit(AstCvtArrayToArray* nodep) override {
        ଯଦି (nodep->reverse()) {
            puts("VL_REVCLONE_Q(");
        } ଅନ୍ୟଥା {
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

    ଶୂନ୍ୟ visit(AstCvtArrayToPacked* nodep) override {
        Ast_ଗଣ୍ଠି_DType* ଅଚଳ fromDtp = nodep->fromp()->dtypep()->skipRefp();
        Ast_ଗଣ୍ଠି_DType* ଅଚଳ elemDtp = fromDtp->subDTypep()->skipRefp();
        ଅଚଳ bool descending = [&]() {
            ଯଦି (ଅଚଳ AstUnpackArrayDType* ଅଚଳ unpackDtp = VN_CAST(fromDtp, UnpackArrayDType))
                ଫେରନ୍ତୁ !unpackDtp->declRange().ascending();
            ଫେରନ୍ତୁ false;
        }();
        puts("VL_PACK_");
        emitIQW(nodep);
        puts("_");
        emitRU(fromDtp);
        emitIQW(elemDtp);
        ଯଦି (descending) {
            // Wrap source in VL_PACK_REVERSED so VL_PACK sees ascending order
            emitOpName(nodep, "(%nw, %rw, %P, VL_PACK_REVERSED(%li))", nodep->fromp(), elemDtp,
                       nullptr);
        } ଅନ୍ୟଥା {
            emitOpName(nodep, "(%nw, %rw, %P, %li)", nodep->fromp(), elemDtp, nullptr);
        }
    }

    ଶୂନ୍ୟ visit(AstCvtUnpackToQueue* nodep) override {
        Ast_ଗଣ୍ଠି_DType* ଅଚଳ elemDTypep = nodep->fromp()->dtypep()->subDTypep();
        emitOpName(nodep, nodep->emitC(), nodep->fromp(), elemDTypep, nullptr);
    }
    ଶୂନ୍ୟ visit(AstCvtUnpackToUnpack* nodep) override {
        // Whether each dimension is reversed, the first for the outermost, up to the last that is
        std::vector<bool> reverses;
        ଅଚଳ AstUnpackArrayDType* ଶୀର୍ଷ = VN_AS(nodep->dtypep()->skipRefp(), UnpackArrayDType);
        ଅଚଳ AstUnpackArrayDType* fromp
            = VN_AS(nodep->fromp()->dtypep()->skipRefp(), UnpackArrayDType);
        ଯେତେବେଳେ (ଶୀର୍ଷ) {
            reverses.ଠେଲ_back(ଶୀର୍ଷ->declRange().ascending() != fromp->declRange().ascending());
            ଶୀର୍ଷ = VN_CAST(ଶୀର୍ଷ->subDTypep()->skipRefp(), UnpackArrayDType);
            fromp = VN_CAST(fromp->subDTypep()->skipRefp(), UnpackArrayDType);
        }
        ଯେତେବେଳେ (!reverses.back()) reverses.କାଢ଼_back();
        putns(nodep, "VL_CVT_UNPACK_TO_UNPACK<");
        ପାଇଁ (size_t i = 0; i < reverses.size(); ++i) {
            ଯଦି (i) puts(", ");
            puts(reverses[i] ? "1" : "0");
        }
        puts(">(");
        iterateAndNextConstNull(nodep->fromp());
        puts(")");
    }

    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି_Assign* nodep) override {
        ଯଦି (AstCReset* ଅଚଳ resetp = VN_CAST(nodep->rhsp(), CReset)) {
            // TODO get rid of emitVarReset and instead let AstNodeAssign understand how to init
            // anything
            Ast_ଗଣ୍ଠି* fromp = nodep->lhsp();
            // Fork needs to use a member select.  Nothing else should be possible before VarRef.
            ଯଦି (AstMemberSel* ଅଚଳ sfromp = VN_CAST(fromp, MemberSel)) {
                // Fork-DynScope generated pointer to previously automatic variable
                AstVar* ଅଚଳ memberVarp = sfromp->varp();
                fromp = sfromp->fromp();
                ଯଦି (AstNullCheck* ଅଚଳ sfromp = VN_CAST(fromp, NullCheck)) fromp = sfromp->lhsp();
                Ast_ଗଣ୍ଠି_Var_Ref* ଅଚଳ fromVarRefp = VN_AS(fromp, ଗଣ୍ଠି_Var_Ref);
                emitVarReset(
                    ("VL_NULL_CHECK("s
                     + (fromVarRefp->selfPointer().isEmpty()
                            ? ""
                            : dereferenceString(fromVarRefp->selfPointerProtect(m_useSelfForThis)))
                     + varName(fromVarRefp->varp()) + ", \""
                     + V3OutFormatter::quoteNameControls(protect(nodep->fileline()->filename()))
                     + "\", " + std::to_string(nodep->fileline()->lineno()) + ")->"),
                    memberVarp, resetp->constructing());
            } ଅନ୍ୟଥା {
                Ast_ଗଣ୍ଠି_Var_Ref* ଅଚଳ fromVarRefp = VN_AS(fromp, ଗଣ୍ଠି_Var_Ref);
                AstVar* ଅଚଳ varp = fromVarRefp->varp();
                ଅଚଳ string prefix
                    = fromVarRefp->selfPointer().isEmpty()
                          ? ""
                          : dereferenceString(
                                VN_AS(fromp, ଗଣ୍ଠି_Var_Ref)->selfPointerProtect(m_useSelfForThis));
                emitVarReset(prefix, varp, resetp->constructing());
            }
            ଫେରନ୍ତୁ;
        }
        bool paren = true;
        bool decind = false;
        bool rhs = true;
        bool reverseUnpack = false;  // Set for descending CvtPackedToArray
        ଅଚଳ AstUnpackArrayDType* ଅଚଳ unpackDtp
            = VN_CAST(nodep->dtypep()->skipRefp(), UnpackArrayDType);
        ଯଦି (AstSel* ଅଚଳ selp = VN_CAST(nodep->lhsp(), Sel)) {
            UASSERT_OBJ(selp->widthMin() == selp->widthConst(), selp, "Width mismatch");
            ଯଦି (selp->widthMin() == 1) {
                putnbs(nodep, "VL_ASSIGNBIT_");
                emitIQW(selp->fromp());
                ଯଦି (nodep->rhsp()->isAllOnesV()) {
                    puts("O(");
                    rhs = false;
                } ଅନ୍ୟଥା {
                    puts("I(");
                }
                iterateAndNextConstNull(selp->lsbp());
                puts(", ");
                iterateAndNextConstNull(selp->fromp());
                ଯଦି (rhs) puts(", ");
            } ଅନ୍ୟଥା {
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
        } ଅନ୍ୟଥା ଯଦି (ଅଚଳ AstGetcRefN* ଅଚଳ selp = VN_CAST(nodep->lhsp(), GetcRefN)) {
            iterateAndNextConstNull(selp->lhsp());
            puts(" = ");
            putnbs(selp, "VL_PUTC_N(");
            iterateAndNextConstNull(selp->lhsp());
            puts(", ");
            iterateAndNextConstNull(selp->rhsp());
            puts(", ");
        } ଅନ୍ୟଥା ଯଦି (AstVar* ଅଚଳ varp = AstVar::scVarRecurse(nodep->lhsp())) {
            putnbs(varp, "VL_ASSIGN_");  // Set a systemC variable
            emitScIQW(varp);
            emitIQW(nodep);
            puts("(");
            puts(cvtToStr(nodep->widthMin()) + ", ");
            iterateAndNextConstNull(nodep->lhsp());
            puts(", ");
        } ଅନ୍ୟଥା ଯଦି (AstVar* ଅଚଳ varp = AstVar::scVarRecurse(nodep->rhsp())) {
            putnbs(varp, "VL_ASSIGN_");  // Get a systemC variable
            emitIQW(nodep);
            emitScIQW(varp);
            puts("(");
            puts(cvtToStr(nodep->widthMin()) + ", ");
            iterateAndNextConstNull(nodep->lhsp());
            puts(", ");
        } ଅନ୍ୟଥା ଯଦି (ଅଚଳ AstCvtPackedToArray* ଅଚଳ castp
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
            ଯଦି (unpackDtp && !unpackDtp->declRange().ascending()) reverseUnpack = true;
        } ଅନ୍ୟଥା ଯଦି (ଅଚଳ AstCvtArrayToArray* ଅଚଳ castp
                   = VN_CAST(nodep->rhsp(), CvtArrayToArray)) {
            ଯଦି (castp->reverse()) {
                putns(castp, "VL_REVCOPY_Q(");
            } ଅନ୍ୟଥା {
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
        } ଅନ୍ୟଥା ଯଦି (nodep->isWide() && VN_IS(nodep->lhsp(), VarRef)  //
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
                   && !VN_IS(nodep->rhsp(), ପଦସମଷ୍ଟି_Stmt)) {
            // Wide functions assign into the array directly, don't need separate assign statement
            m_wideTempRefp = VN_AS(nodep->lhsp(), VarRef);
            paren = false;
        } ଅନ୍ୟଥା ଯଦି (nodep->isWide() && !unpackDtp && !VN_IS(nodep->rhsp(), Const)) {
            putnbs(nodep, "VL_ASSIGN_W(");
            puts(cvtToStr(nodep->widthMin()) + ", ");
            iterateAndNextConstNull(nodep->lhsp());
            puts(", ");
        } ଅନ୍ୟଥା ଯଦି (VN_IS(nodep->lhsp()->dtypep()->skipRefp(), QueueDType)
                   && (VN_IS(nodep->rhsp(), StreamL) || VN_IS(nodep->lhsp(), StreamL)
                       || VN_IS(nodep->rhsp(), StreamR) || VN_IS(nodep->lhsp(), StreamR)
                       || VN_IS(nodep->rhsp(), StreamR))) {
            //if either side is streamL or streamR don't emit lhsp everything will be passed by
            //reference

            paren = false;

        } ଅନ୍ୟଥା {
            paren = false;
            iterateAndNextConstNull(nodep->lhsp());
            puts(" ");
            ofp()->blockInc();
            decind = true;
            ଯଦି (!VN_IS(nodep->rhsp(), Const)) ofp()->putBreak();
            putns(nodep, "= ");
        }
        ଯଦି (rhs) iterateAndNextConstNull(nodep->rhsp());
        ଯଦି (paren) puts(")");
        ଯଦି (decind) ofp()->blockDec();
        puts(";\n");
        ଯଦି (reverseUnpack) {
            puts("VL_UNPACK_REVERSED(");
            iterateAndNextConstNull(nodep->lhsp());
            puts(");\n");
        }
    }
    ଶୂନ୍ୟ visit(AstAssocSel* nodep) override {
        iterateAndNextConstNull(nodep->fromp());
        ଅଚଳ std::string atFunc = nodep->isLValue() ? ".atWrite(" : ".at(";
        putnbs(nodep, atFunc);
        AstAssocArrayDType* ଅଚଳ adtypep
            = VN_AS(nodep->fromp()->dtypep()->skipRefp(), AssocArrayDType);
        UASSERT_OBJ(adtypep, nodep, "Associative select on non-associative type");
        iterateAndNextConstNull(nodep->bitp());
        puts(")");
    }
    ଶୂନ୍ୟ visit(AstWildcardSel* nodep) override {
        iterateAndNextConstNull(nodep->fromp());
        ଅଚଳ std::string atFunc = nodep->isLValue() ? ".atWrite(" : ".at(";
        putnbs(nodep, atFunc);
        AstWildcardArrayDType* ଅଚଳ adtypep
            = VN_AS(nodep->fromp()->dtypep()->skipRefp(), WildcardArrayDType);
        UASSERT_OBJ(adtypep, nodep, "Wildcard select on non-wildcard-associative type");
        iterateAndNextConstNull(nodep->bitp());
        puts(")");
    }
    ଶୂନ୍ୟ visit(AstCCall* nodep) override {
        ଅଚଳ AstCFunc* ଅଚଳ funcp = nodep->funcp();
        ଅଚଳ Ast_ଗଣ୍ଠି_Module* ଅଚଳ funcModp = EmitCParentModule::get(funcp);
        putnbs(nodep, "");
        ଯଦି (funcp->dpiImportPrototype()) {
            // Calling DPI import
            putns(nodep, funcp->name());
        } ଅନ୍ୟଥା ଯଦି (funcp->isProperMethod() && funcp->isStatic()) {
            // Call static method via the containing class
            putns(funcModp, EmitCUtil::prefixNameProtect(funcModp) + "::");
            putns(nodep, funcp->nameProtect());
        } ଅନ୍ୟଥା ଯଦି (nodep->superReference()) {
            // Calling superclass method
            putns(funcModp, EmitCUtil::prefixNameProtect(funcModp) + "::");
            putns(nodep, funcp->nameProtect());
        } ଅନ୍ୟଥା ଯଦି (funcp->isLoose()) {
            // Calling loose method
            putns(nodep, funcNameProtect(funcp));
        } ଅନ୍ୟଥା {
            // Calling regular method/function
            ଯଦି (!nodep->selfPointer().isEmpty()) {
                emitDereference(nodep, nodep->selfPointerProtect(m_useSelfForThis));
            }
            putns(nodep, funcp->nameProtect());
        }
        emitCCallArgs(nodep, nodep->selfPointerProtect(m_useSelfForThis), m_cfuncp->needProcess());
    }
    ଶୂନ୍ୟ visit(AstCMethodCall* nodep) override {
        ଅଚଳ AstCFunc* ଅଚଳ funcp = nodep->funcp();
        UASSERT_OBJ(!funcp->isLoose(), nodep, "Loose method called via AstCMethodCall");
        iterateConst(nodep->fromp());
        putnbs(nodep, "->");
        ଯଦି (nodep->superReference()) {
            // Super call, so the base implementation is used rather than the override
            ଅଚଳ Ast_ଗଣ୍ଠି_Module* ଅଚଳ funcModp = EmitCParentModule::get(funcp);
            putns(funcModp, EmitCUtil::prefixNameProtect(funcModp) + "::");
        }
        putns(funcp, funcp->nameProtect());
        emitCCallArgs(nodep, "", m_cfuncp->needProcess());
    }
    ଶୂନ୍ୟ visit(AstCAwait* nodep) override {
        putns(nodep, "co_await ");
        iterateConst(nodep->exprp());
        puts(";\n");
    }
    ଶୂନ୍ୟ visit(AstCNew* nodep) override {
        ଯଦି (VN_IS(nodep->dtypep(), VoidDType)) {  // super.new case
            putsDecoration(nodep, "/*super.new*/");
            ଫେରନ୍ତୁ;
        }
        // assignment case;
        putns(nodep, "VL_NEW(" + EmitCUtil::prefixNameProtect(nodep->dtypep()) + ", "
                         + optionalProcArg(nodep->dtypep()) + "vlSymsp");
        putCommaIterateNext(nodep->argsp(), true);
        puts(")");
    }
    ଶୂନ୍ୟ visit(AstCMethodHard* nodep) override {
        iterateConst(nodep->fromp());
        putns(nodep, nodep->usePtr() ? "->" : ".");
        putns(nodep, nodep->name());
        ଯଦି (nodep->method() == VCMethod::FORCE_READ_SEL) {
            emitIQW(nodep);
            ଯଦି (nodep->isWide()) puts("<" + cvtToStr(nodep->dtypep()->width_ଶବ୍ଦଗୁଡ଼ିକ()) + ">");
        } ଅନ୍ୟଥା ଯଦି (nodep->method() == VCMethod::ARRAY_SLICE) {
            // VlUnpacked::slice<N_Out>(loIdx) - N_Out is the (fixed) result array size
            ଅଚଳ AstUnpackArrayDType* ଅଚଳ adtypep
                = VN_AS(nodep->dtypep()->skipRefp(), UnpackArrayDType);
            puts("<" + cvtToStr(adtypep->elementsConst()) + ">");
        } ଅନ୍ୟଥା ଯଦି (nodep->method() == VCMethod::COVERGROUP_ADD_COVERPOINT) {
            // The hit-list bound is a template argument of the returned VlCoverpointT<>, and the
            // node's own dtype is that type, so it is the one source of truth for both.
            ଅଚଳ AstCoverpointDType* ଅଚଳ cpdtypep
                = VN_AS(nodep->dtypep()->skipRefp(), CoverpointDType);
            puts("<" + cvtToStr(cpdtypep->hitBound()) + ">");
        } ଅନ୍ୟଥା ଯଦି (nodep->method() == VCMethod::COVERGROUP_ADD_CROSS) {
            ଅଚଳ AstCoverCrossDType* ଅଚଳ cxdtypep
                = VN_AS(nodep->dtypep()->skipRefp(), CoverCrossDType);
            puts("<" + cxdtypep->cppTemplateArgs() + ">");
        }
        puts("(");
        bool comma = false;
        ପୂର୍ଣ୍ଣସଂଖ୍ୟା arg_ଅଙ୍କ = 0;
        ପାଇଁ (Ast_ଗଣ୍ଠି* subnodep = nodep->pinsp(); subnodep; subnodep = subnodep->nextp()) {
            ଯଦି (comma) puts(", ");
            ଯଦି (nodep->method() == VCMethod::RANDOMIZER_HARD && arg_ଅଙ୍କ == 1) {
                // For RANDOMIZER_HARD's filename argument (2nd arg after constraint),
                // apply protect() similar to VL_STOP to handle --protected flag
                ଯଦି (ଅଚଳ AstCExpr* ଅଚଳ cexprp = VN_CAST(subnodep, CExpr)) {
                    // Extract filename from the CExpr (which contains "filename")
                    std::string filename;
                    ପାଇଁ (ଅଚଳ Ast_ଗଣ୍ଠି* textnodep = cexprp->nodesp(); textnodep;
                         textnodep = textnodep->nextp()) {
                        ଯଦି (ଅଚଳ AstText* ଅଚଳ textp = VN_CAST(textnodep, Text)) {
                            filename = textp->text();
                            ଭାଙ୍ଗ;
                        }
                    }
                    // Remove surrounding quotes if present
                    ଯଦି (filename.size() >= 2 && filename.front() == '"'
                        && filename.back() == '"') {
                        filename = filename.substr(1, filename.size() - 2);
                    }
                    // Emit with protect()
                    putsQuoted(protect(filename));
                } ଅନ୍ୟଥା {
                    iterateConst(subnodep);
                }
            } ଅନ୍ୟଥା {
                iterateConst(subnodep);
            }
            comma = true;
            arg_ଅଙ୍କ++;
        }
        ଯଦି (nodep->withp()) {
            ଯଦି (comma) puts(", ");
            iterateConst(nodep->withp());
        }
        puts(")");
    }
    ଶୂନ୍ୟ visit(AstCFuncHard* nodep) override {
        putns(nodep, nodep->name());
        puts("(");
        putCommaIterateNext(nodep->pinsp());
        puts(")");
    }
    ଶୂନ୍ୟ visit(AstLambdaArgRef* nodep) override { putbs(nodep->nameProtect()); }
    ଶୂନ୍ୟ visit(AstWith* nodep) override {
        // With uses a C++11 lambda
        putnbs(nodep, "[&](");
        ଯଦି (auto* ଅଚଳ argrefp = nodep->indexArgRefp()) {
            putnbs(argrefp, argrefp->dtypep()->cType(argrefp->nameProtect(), false, false));
            puts(", ");
        }
        ଯଦି (auto* ଅଚଳ argrefp = nodep->ମୂଲ୍ୟ_Arg_Refp()) {
            putnbs(argrefp, argrefp->dtypep()->cType(argrefp->nameProtect(), false, false));
        }
        puts(") -> ");
        putnbs(nodep, nodep->dtypep()->cType("", false, false));
        puts(" {\n");
        VL_RESTORER(m_createdScopeHash);
        iterateAndNextConstNull(nodep->exprp());
        puts("}\n");
    }
    ଶୂନ୍ୟ visit(AstCase* nodep) override {  // LCOV_EXCL_LINE
        // In V3Case...
        nodep->v3fatalSrc("Case statements should have been reduced out");
    }
    ଶୂନ୍ୟ visit(AstComment* nodep) override {
        string at;
        ଯଦି (nodep->showAt()) {
            at = " at " + nodep->fileline()->ascii();
            // If protecting, passthru less information about the design
            ଯଦି (!v3Global.opt.protectIds()) ଫେରନ୍ତୁ;
        }
        ଯଦି (!(nodep->protect() && v3Global.opt.protectIds())) {
            putsDecoration(nodep, "// "s + nodep->name() + at + "\n");
        }
        iterateChildrenConst(nodep);
    }
    ଶୂନ୍ୟ visit(AstCoverOtherDecl* nodep) override {
        emit_Cover_Other_Decl_ଭର୍ତ୍ତି(nodep);
        ଯଦି (coverageUsesLocalCounter(nodep)) {
            // Some task bodies are cloned into a caller module where the
            // original instance-local counter is not addressable. Those clones
            // fall back to the global bin number, so register that slot too.
            emit_Cover_Other_Decl_ଭର୍ତ୍ତି(nodep, true);
        }
    }
    ଶୂନ୍ୟ visit(AstCoverToggleDecl* nodep) override {
        putns(nodep, "vlSelf->__vlCoverToggleInsert(");  // As Declared in emitCoverageDecl
        puts(cvtToStr(nodep->range().ଡାହାଣ()));
        puts(", ");
        puts(cvtToStr(nodep->range().ବାମ()));
        puts(", ");
        puts(cvtToStr(nodep->range().ranged()));
        puts(", ");
        putCoverageArray(nodep);
        puts(" + ");
        puts(cvtToStr(coverage_Bin_ଅଙ୍କ(nodep)));
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
                   + VIdProtect::protect_ଶବ୍ଦଗୁଡ଼ିକ_If(nodep->hier(), nodep->protect()));
        puts(", ");
        putsQuoted(VIdProtect::protect_ଶବ୍ଦଗୁଡ଼ିକ_If(nodep->page(), nodep->protect()));
        puts(", ");
        putsQuoted(VIdProtect::protect_ଶବ୍ଦଗୁଡ଼ିକ_If(nodep->comment(), nodep->protect()));
        puts(");\n");
    }
    ଶୂନ୍ୟ visit(AstCoverInc* nodep) override {
        ଯଦି (VN_IS(nodep->declp(), CoverOtherDecl)) {
            ଯଦି (v3Global.opt.threads() > 1) {
                putns(nodep, "");
                putCoverageArray(nodep->declp());
                puts("[");
                puts(cvtToStr(coverage_Bin_ଅଙ୍କ(nodep->declp())));
                puts("].fetch_add(1, std::memory_order_relaxed);\n");
            } ଅନ୍ୟଥା {
                putns(nodep, "++(");
                putCoverageArray(nodep->declp());
                puts("[");
                puts(cvtToStr(coverage_Bin_ଅଙ୍କ(nodep->declp())));
                puts("]);\n");
            }
        } ଅନ୍ୟଥା {
            puts("VL_COV_TOGGLE_CHG_");
            ଯଦି (v3Global.opt.threads() > 1) {
                puts("MT_");
            } ଅନ୍ୟଥା {
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
            puts(cvtToStr(coverage_Bin_ଅଙ୍କ(nodep->declp())));
            puts(", ");
            iterateConst(nodep->toggleExprp());
            puts(", ");
            iterateConst(nodep->toggleCovExprp());
            puts(");\n");
        }
    }
    ଶୂନ୍ୟ visit(AstDisableFork* nodep) override { putns(nodep, "vlProcess->disableFork();\n"); }
    ଶୂନ୍ୟ visit(AstCReturn* nodep) override {
        putns(nodep, "return (");
        iterateAndNextConstNull(nodep->lhsp());
        puts(");\n");
    }
    ଶୂନ୍ୟ visit(AstDisplay* nodep) override {
        string text = nodep->fmtp()->text();
        ଯଦି (nodep->addNewline()) text += "\n";
        display_ଗଣ୍ଠି(nodep, nodep->fmtp(), text, nodep->fmtp()->exprsp(), false);
    }
    ଶୂନ୍ୟ visit(AstDumpCtl* nodep) override {
        ବାଛ (nodep->ctlType()) {
        ସ୍ଥିତି VDumpCtlType::FILE:
            putns(nodep, "vlSymsp->_vm_contextp__->dumpfile(");
            iterateConst(nodep->exprp());
            puts(");\n");
            ଭାଙ୍ଗ;
        ସ୍ଥିତି VDumpCtlType::VARS:
            // We ignore number of levels to dump in exprp()
            ଯଦି (v3Global.opt.trace()) {
                putns(nodep, "vlSymsp->_traceDumpOpen();\n");
            } ଅନ୍ୟଥା {
                putns(nodep, "VL_PRINTF_MT(\"-Info: ");
                puts(V3OutFormatter::quoteNameControls(protect(nodep->fileline()->filename())));
                puts(":");
                puts(cvtToStr(nodep->fileline()->lineno()));
                puts(": $dumpvar ignored, as Verilated without --trace");
                puts("\\n\");\n");
            }
            ଭାଙ୍ଗ;
        ସ୍ଥିତି VDumpCtlType::ALL:
            // $dumpall currently ignored
            ଭାଙ୍ଗ;
        ସ୍ଥିତି VDumpCtlType::FLUSH:
            // $dumpall currently ignored; would need rework of VCD single thread,
            // or flag we pass-through to next eval() iteration
            ଭାଙ୍ଗ;
        ସ୍ଥିତି VDumpCtlType::LIMIT:
            // $dumplimit currently ignored
            ଭାଙ୍ଗ;
        ସ୍ଥିତି VDumpCtlType::OFF:
            // Currently ignored as both Vcd and Fst do not support them, as would need "X" dump
            ଭାଙ୍ଗ;
        ସ୍ଥିତି VDumpCtlType::ON:
            // Currently ignored as $dumpoff is also ignored
            ଭାଙ୍ଗ;
        ପୂର୍ବନିର୍ଦ୍ଧାରିତ: nodep->v3fatalSrc("Bad case, unexpected " << nodep->ctlType().ascii());
        }
    }
    ଶୂନ୍ୟ visit(AstScopeName* nodep) override {
        // For use under AstCCalls for dpiImports.  ScopeNames under
        // displays are handled in AstDisplay
        ଯଦି (!nodep->dpiExport()) {
            // this is where the DPI import context scope is set
            ଅଚଳ string scope = nodep->scopeDpiName();
            putnbs(nodep, "(vlSymsp->" + protect("__Vscopep_" + scope) + ")");
        }
    }
    ଶୂନ୍ୟ visit(AstSFormat* nodep) override {
        display_ଗଣ୍ଠି(nodep, nodep->fmtp(), nodep->fmtp()->text(), nodep->fmtp()->exprsp(), false);
    }
    ଶୂନ୍ୟ visit(AstSFormatF* nodep) override {
        display_ଗଣ୍ଠି(nodep, nodep, nodep->text(), nodep->exprsp(), false);
    }
    ଶୂନ୍ୟ visit(AstFScanF* nodep) override {
        display_ଗଣ୍ଠି(nodep, nullptr, nodep->text(), nodep->exprsp(), true);
    }
    ଶୂନ୍ୟ visit(AstSScanF* nodep) override {
        display_ଗଣ୍ଠି(nodep, nullptr, nodep->text(), nodep->exprsp(), true);
    }
    ଶୂନ୍ୟ visit(Ast_ମୂଲ୍ୟ_Plus_Args* nodep) override {
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
    ଶୂନ୍ୟ visit(AstTestPlusArgs* nodep) override {
        putns(nodep, "VL_TESTPLUSARGS_I(");
        iterateConst(nodep->searchp());
        puts(")");
    }
    ଶୂନ୍ୟ visit(AstFError* nodep) override {
        putns(nodep, "VL_FERROR_I");
        puts(nodep->strp()->isString() ? "N(" : "W(");
        iterateAndNextConstNull(nodep->filep());
        putbs(", ");
        ଯଦି (nodep->strp()->isWide()) {
            puts(cvtToStr(nodep->strp()->width_ଶବ୍ଦଗୁଡ଼ିକ()));
            putbs(", ");
        }
        iterateAndNextConstNull(nodep->strp());
        puts(")");
    }
    ଶୂନ୍ୟ visit(AstFGetS* nodep) override {
        emitOpName(nodep, nodep->emitC(), nodep->strgp(), nodep->filep(), nullptr);
    }
    ଶୂନ୍ୟ visit(AstFOpen* nodep) override {
        putns(nodep, "VL_FOPEN_NN(");
        iterateConst(nodep->filenamep());
        putbs(", ");
        iterateConst(nodep->modep());
        puts(");\n");
    }
    ଶୂନ୍ୟ visit(AstFOpenMcd* nodep) override {
        putns(nodep, "VL_FOPEN_MCD_N(");
        iterateConst(nodep->filenamep());
        puts(");\n");
    }
    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି_Read_Write_Mem* nodep) override {
        putns(nodep, nodep->cFuncPrefixp());
        puts("N(");
        puts(nodep->isHex() ? "true" : "false");
        putbs(", ");
        // Need real storage width
        puts(cvtToStr(nodep->memp()->dtypep()->skipRefp()->subDTypep()->widthMin()));
        uint32_t array_lo = 0;
        bool memUsePtr = true;
        {
            ଅଚଳ AstVarRef* ଅଚଳ varrefp = VN_CAST(nodep->memp(), VarRef);
            ଯଦି (!varrefp) {
                nodep->v3error(nodep->verilogKwd() << " loading non-variable");
            } ଅନ୍ୟଥା ଯଦି (VN_IS(varrefp->varp()->dtypeSkipRefp(), AssocArrayDType)
                       || VN_IS(varrefp->varp()->dtypeSkipRefp(), DynArrayDType)) {
                // nodep->memp() below will when verilated code is compiled create a C++ template
                memUsePtr = false;
            } ଅନ୍ୟଥା ଯଦି (ଅଚଳ AstUnpackArrayDType* ଅଚଳ adtypep
                       = VN_CAST(varrefp->varp()->dtypeSkipRefp(), UnpackArrayDType)) {
                putbs(", ");
                puts(cvtToStr(varrefp->varp()->dtypep()->arrayUnpackedElements()));
                array_lo = adtypep->lo();
                putbs(", ");
                puts(cvtToStr(array_lo));
            } ଅନ୍ୟଥା {
                nodep->v3error(
                    nodep->verilogKwd()
                    << " loading other than unpacked/dynamic/associative-array variable");
            }
        }
        putbs(", ");
        iterateConst(nodep->filenamep());
        putbs(", ");
        {
            ଯଦି (memUsePtr) puts(" &(");
            iterateAndNextConstNull(nodep->memp());
            ଯଦି (memUsePtr) puts(")");
        }
        putbs(", ");
        ଯଦି (nodep->lsbp()) {
            iterateAndNextConstNull(nodep->lsbp());
        } ଅନ୍ୟଥା {
            puts(cvtToStr(array_lo));
        }
        putbs(", ");
        ଯଦି (nodep->msbp()) {
            iterateAndNextConstNull(nodep->msbp());
        } ଅନ୍ୟଥା {
            puts("~0ULL");
        }
        puts(");\n");
    }
    ଶୂନ୍ୟ visit(AstFClose* nodep) override {
        putns(nodep, "VL_FCLOSE_I(");
        iterateAndNextConstNull(nodep->filep());
        puts("); ");
    }
    ଶୂନ୍ୟ visit(AstFFlush* nodep) override {
        ଯଦି (!nodep->filep()) {
            putns(nodep, "VL_FFLUSH_MT();");
        } ଅନ୍ୟଥା {
            putns(nodep, "VL_FFLUSH_I(");
            iterateAndNextConstNull(nodep->filep());
            puts(");\n");
        }
    }
    ଶୂନ୍ୟ visit(AstFSeek* nodep) override {
        putns(nodep, "(VL_FSEEK_I(");
        iterateAndNextConstNull(nodep->filep());
        puts(", ");
        iterateAndNextConstNull(nodep->offset());
        puts(", ");
        iterateAndNextConstNull(nodep->operation());
        puts(") == -1 ? -1 : 0)");
    }
    ଶୂନ୍ୟ visit(AstFTell* nodep) override {
        putns(nodep, "VL_FTELL_I(");
        iterateAndNextConstNull(nodep->filep());
        puts(")");
    }
    ଶୂନ୍ୟ visit(AstFRewind* nodep) override {
        putns(nodep, "(VL_FSEEK_I(");
        iterateAndNextConstNull(nodep->filep());
        puts(", 0, 0) == -1 ? -1 : 0)");
    }
    ଶୂନ୍ୟ visit(AstFRead* nodep) override {
        putns(nodep, "VL_FREAD_I(");
        puts(cvtToStr(nodep->memp()->widthMin()));  // Need real storage width
        putbs(", ");
        uint32_t array_lo = 0;
        uint32_t array_size = 0;
        {
            ଅଚଳ AstVarRef* ଅଚଳ varrefp = VN_CAST(nodep->memp(), VarRef);
            ଯଦି (!varrefp) {
                nodep->v3error(nodep->verilogKwd() << " loading non-variable");
            } ଅନ୍ୟଥା ଯଦି (VN_CAST(varrefp->varp()->dtypeSkipRefp(), BasicDType)) {
            } ଅନ୍ୟଥା ଯଦି (ଅଚଳ AstUnpackArrayDType* ଅଚଳ adtypep
                       = VN_CAST(varrefp->varp()->dtypeSkipRefp(), UnpackArrayDType)) {
                array_lo = adtypep->lo();
                array_size = adtypep->elementsConst();
            } ଅନ୍ୟଥା {
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
        ଯଦି (nodep->startp()) {
            iterateAndNextConstNull(nodep->startp());
        } ଅନ୍ୟଥା {
            puts(cvtToStr(array_lo));
        }
        putbs(", ");
        ଯଦି (nodep->countp()) {
            iterateAndNextConstNull(nodep->countp());
        } ଅନ୍ୟଥା {
            puts(cvtToStr(array_size));
        }
        puts(")");
    }
    ଶୂନ୍ୟ visit(Ast_ଗଦା_Trace_F* nodep) override { putns(nodep, "VL_STACKTRACE_N()"); }
    ଶୂନ୍ୟ visit(Ast_ଗଦା_Trace_T* nodep) override { putns(nodep, "VL_STACKTRACE();\n"); }
    ଶୂନ୍ୟ visit(AstSystemT* nodep) override {
        putns(nodep, "(void)VL_SYSTEM_I");
        emitIQW(nodep->lhsp());
        puts("(");
        ଯଦି (nodep->lhsp()->isWide()) {
            puts(cvtToStr(nodep->lhsp()->width_ଶବ୍ଦଗୁଡ଼ିକ()));
            putbs(", ");
        }
        iterateAndNextConstNull(nodep->lhsp());
        puts(");\n");
    }
    ଶୂନ୍ୟ visit(AstSystemF* nodep) override {
        putns(nodep, "VL_SYSTEM_I");
        emitIQW(nodep->lhsp());
        puts("(");
        ଯଦି (nodep->lhsp()->isWide()) {
            puts(cvtToStr(nodep->lhsp()->width_ଶବ୍ଦଗୁଡ଼ିକ()));
            putbs(", ");
        }
        iterateAndNextConstNull(nodep->lhsp());
        puts(")");
    }
    ଶୂନ୍ୟ visit(Ast_Stmt_ପଦସମଷ୍ଟି* nodep) override {
        ଯଦି (VN_IS(nodep->exprp()->dtypep()->skipRefp(), VoidDType)) {
            putns(nodep, "");
        } ଅନ୍ୟଥା {
            putns(nodep, "std::ignore = ");
        }
        iterateConst(nodep->exprp());
        puts(";\n");
    }
    ଶୂନ୍ୟ visit(AstJumpBlock* nodep) override {
        // Allocate label number
        ଅଚଳ size_t n = m_labelNumbers.size();
        ଅଚଳ bool newEntry = m_labelNumbers.emplace(nodep, n).second;
        UASSERT_OBJ(newEntry, nodep, "AstJumpBlock visited twide");
        // Emit
        putns(nodep, "{\n");  // Make it visually obvious label jumps outside these
        VL_RESTORER(m_createdScopeHash);
        iterateAndNextConstNull(nodep->stmtsp());
        puts("__Vlabel" + std::to_string(n) + ": ;\n");
        puts("}\n");
    }
    ଶୂନ୍ୟ visit(AstJumpGo* nodep) override {
        // Retrieve target label number - must already exist (from enclosing AstJumpBlock)
        ଅଚଳ size_t n = m_labelNumbers.at(nodep->blockp());
        // Emit
        putns(nodep, "goto __Vlabel" + std::to_string(n) + ";\n");
    }
    ଶୂନ୍ୟ visit(AstLoop* nodep) override {
        UASSERT_OBJ(!nodep->contsp(), nodep, "'contsp' only used before LinkJump");
        VL_RESTORER(m_createdScopeHash);
        // Special case when the AstLoopTest is first for output readability
        ଯଦି (AstLoopTest* ଅଚଳ testp = VN_CAST(nodep->stmtsp(), LoopTest)) {
            putns(nodep, "while (");
            iterateConst(testp->condp());
            puts(") {\n");
            iterateAndNextConstNull(testp->nextp());
            puts("}\n");
            ଫେରନ୍ତୁ;
        }
        // Special case when the AstLoopTest is last for output readability
        ଯଦି (Ast_ଗଣ୍ଠି* lastp = nodep->stmtsp()) {
            ଯେତେବେଳେ (Ast_ଗଣ୍ଠି* ଅଚଳ nextp = lastp->nextp()) lastp = nextp;
            ଯଦି (AstLoopTest* ଅଚଳ testp = VN_CAST(lastp, LoopTest)) {
                putns(nodep, "do {\n");
                ପାଇଁ (Ast_ଗଣ୍ଠି* p = nodep->stmtsp(); p != lastp; p = p->nextp()) iterateConst(p);
                puts("} while (");
                iterateConst(testp->condp());
                puts(");\n");
                ଫେରନ୍ତୁ;
            }
        }
        // Emit generic case directly
        putns(nodep, "while (true) {\n");
        iterateAndNextConstNull(nodep->stmtsp());
        puts("}\n");
    }
    ଶୂନ୍ୟ visit(AstLoopTest* nodep) override {
        VL_RESTORER(m_createdScopeHash);
        putns(nodep, "if (!(");
        iterateAndNextConstNull(nodep->condp());
        puts(")) break;\n");
    }
    ଶୂନ୍ୟ visit(AstCLocalScope* nodep) override {
        putns(nodep, "{\n");
        VL_RESTORER(m_createdScopeHash);
        iterateAndNextConstNull(nodep->stmtsp());
        puts("}\n");
    }
    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି_If* nodep) override {
        putns(nodep, "if (");
        ଯଦି (!nodep->branchPred().unknown()) {
            puts(nodep->branchPred().ascii());
            puts("((");  // Two parens, so that VL_UNLIKELY((class<foo,bar>)) works
        }
        iterateAndNextConstNull(nodep->condp());
        ଯଦି (!nodep->branchPred().unknown()) puts("))");
        puts(") {\n");
        {
            VL_RESTORER(m_createdScopeHash);
            iterateAndNextConstNull(nodep->thensp());
        }
        puts("}");
        ଯଦି (!nodep->elsesp()) {
            puts("\n");
        } ଅନ୍ୟଥା {
            ଯଦି (VN_IS(nodep->elsesp(), ଗଣ୍ଠି_If) && !nodep->elsesp()->nextp()) {
                puts(" else ");
                iterateAndNextConstNull(nodep->elsesp());
            } ଅନ୍ୟଥା {
                VL_RESTORER(m_createdScopeHash);
                puts(" else {\n");
                iterateAndNextConstNull(nodep->elsesp());
                puts("}\n");
            }
        }
    }
    ଶୂନ୍ୟ visit(Ast_ପଦସମଷ୍ଟି_Stmt* nodep) override {
        VL_RESTORER(m_createdScopeHash);
        ଅଚଳ bool containsAwait = nodep->exists([](AstCAwait*) -> bool { ଫେରନ୍ତୁ true; });
        ଯଦି (containsAwait) {
            UASSERT_OBJ(m_cfuncp && m_cfuncp->isCoroutine(), nodep,
                        "AstExprStmt with CAwait must be in coroutine");
            putnbs(nodep, "(co_await ([&]() -> VlCoroutine {\n");
            iterateAndNextConstNull(nodep->stmtsp());
            puts("co_return;\n");
            ଯଦି (!nodep->hasResult()) {
                puts("}()))");
                ଫେରନ୍ତୁ;
            }
            puts("}()), ");
            iterateAndNextConstNull(nodep->resultp());
            puts(")");
            ଫେରନ୍ତୁ;
        }
        // GCC allows compound statements in expressions, but this is not standard.
        // So we use an immediate-evaluation lambda and comma operator
        putnbs(nodep, "([&]() {\n");
        ଯଦି (!nodep->hasResult()) {
            iterateAndNextConstNull(nodep->stmtsp());
            puts("}())");
            ଫେରନ୍ତୁ;
        }
        iterateAndNextConstNull(nodep->stmtsp());
        puts("}(), ");
        iterateAndNextConstNull(nodep->resultp());
        puts(")");
    }
    ଶୂନ୍ୟ visit(AstStop* nodep) override {
        putns(nodep, "VL_STOP_MT(");
        putsQuoted(protect(nodep->fileline()->filename()));
        puts(", ");
        puts(cvtToStr(nodep->fileline()->lineno()));
        puts(", \"\"");
        ଯଦି (nodep->isFatal()) puts(", false");
        puts(");\n");
    }
    ଶୂନ୍ୟ visit(AstFinish* nodep) override {
        // Disable all the forks so they don't operate after simulation is finished.
        ଯଦି (m_cfuncp && m_cfuncp->needProcess()) putns(nodep, "vlProcess->disableFork();\n");
        putns(nodep, "VL_FINISH_MT(");
        putsQuoted(protect(nodep->fileline()->filename()));
        puts(", ");
        puts(cvtToStr(nodep->fileline()->lineno()));
        puts(", \"\");\n");
    }
    ଶୂନ୍ୟ visit(AstFinishFork* nodep) override {
        // Disable all the forks so they don't operate after simulation is finished.
        ଯଦି (m_cfuncp && m_cfuncp->needProcess()) putns(nodep, "vlProcess->disableFork();\n");
        putns(nodep, "VL_FINISH_MT(");
        putsQuoted(protect(nodep->fileline()->filename()));
        puts(", ");
        puts(cvtToStr(nodep->fileline()->lineno()));
        puts(", \"\");\n");
        ଯଦି (m_cfuncp->isCoroutine()) {
            putns(nodep, "co_return;\n");
        } ଅନ୍ୟଥା {
            putns(nodep, "return;\n");
        }
    }
    ଶୂନ୍ୟ visit(AstPrintTimeScale* nodep) override {
        putns(nodep, "VL_PRINTTIMESCALE(");
        putsQuoted(protect(nodep->prettyName()));
        puts(", ");
        putsQuoted(nodep->timeunit().ascii());
        puts(", vlSymsp->_vm_contextp__);\n");
    }
    ଶୂନ୍ୟ visit(AstRand* nodep) override {
        emitOpName(nodep, nodep->emitC(), nodep->seedp(), nullptr, nullptr);
    }
    ଶୂନ୍ୟ visit(AstRandRNG* nodep) override {
        emitOpName(nodep, nodep->emitC(), nullptr, nullptr, nullptr);
    }
    ଶୂନ୍ୟ visit(AstTime* nodep) override {
        putns(nodep, "VL_TIME_UNITED_Q(");
        UASSERT_OBJ(!nodep->timeunit().isNone(), nodep, "$time has no units");
        ଅଚଳ ଦ୍ୱିଗୁଣ time
            = nodep->timeunit().multiplier() / v3Global.rootp()->timeprecision().multiplier();
        UASSERT_OBJ(time >= 1, nodep, "TimeQ is less than 1, will result in division by zero");
        puts(cvtToStr(time));
        puts(")");
    }
    ଶୂନ୍ୟ visit(AstTimeD* nodep) override {
        putns(nodep, "VL_TIME_UNITED_D(");
        UASSERT_OBJ(!nodep->timeunit().isNone(), nodep, "$realtime has no units");
        puts(cvtToStr(nodep->timeunit().multiplier()
                      / v3Global.rootp()->timeprecision().multiplier()));
        puts(")");
    }
    ଶୂନ୍ୟ visit(AstGetInitialRandomSeed* nodep) override {
        putns(nodep, "vlSymsp->_vm_contextp__->randSeed()");
    }
    ଶୂନ୍ୟ visit(AstTimeFormat* nodep) override {
        putns(nodep, "VL_TIMEFORMAT_IINI(");
        ଯଦି (nodep->unitsp()) {
            puts("true, ");
            iterateAndNextConstNull(nodep->unitsp());
        } ଅନ୍ୟଥା {
            puts("false, 0");
        }
        puts(", ");
        ଯଦି (nodep->precisionp()) {
            puts("true, ");
            iterateAndNextConstNull(nodep->precisionp());
        } ଅନ୍ୟଥା {
            puts("false, 0");
        }
        puts(", ");
        ଯଦି (nodep->suffixp()) {
            puts("true, ");
            iterateAndNextConstNull(nodep->suffixp());
        } ଅନ୍ୟଥା {
            puts("false, \"\"");
        }
        puts(", ");
        ଯଦି (nodep->widthp()) {
            puts("true, ");
            iterateAndNextConstNull(nodep->widthp());
        } ଅନ୍ୟଥା {
            puts("false, 0");
        }
        puts(", vlSymsp->_vm_contextp__);\n");
    }
    ଶୂନ୍ୟ visit(AstTimePrecision* nodep) override {
        putns(nodep, "vlSymsp->_vm_contextp__->timeprecision()");
    }

    // Nodes involing AstText
    ଶୂନ୍ୟ visit(AstText* nodep) override {
        // All Text should be under TextBlock/CStmt/CStmtUser/CExpr/CExprUser
        nodep->v3fatalSrc("Text node in unexpected position");
    }
    ଶୂନ୍ୟ visit(AstTextBlock* nodep) override {
        putnbs(nodep, "");
        puts(nodep->prefix());
        emitNodesWithText(nodep->nodesp(), false, true, nodep->separator());
        puts(nodep->suffix());
    }
    ଶୂନ୍ୟ visit(AstCStmt* nodep) override {
        putnbs(nodep, "");
        emitNodesWithText(nodep->nodesp(), false, true, "");
        ensure_New_ଧାଡ଼ି();
    }
    ଶୂନ୍ୟ visit(AstCExpr* nodep) override {
        putnbs(nodep, "");
        emitNodesWithText(nodep->nodesp(), false, true, "");
    }
    ଶୂନ୍ୟ visit(AstCStmtUser* nodep) override {
        putnbs(nodep, "");
        ofp()->putsNoTracking("\n");
        ଯଦି (nodep->fromDollarC() && v3Global.opt.decoration() && !v3Global.opt.protectIds()) {
            ofp()->putsNoTracking("// $c statement at " + nodep->fileline()->ascii() + "\n");
        }
        emitNodesWithText(nodep->nodesp(), m_useSelfForThis, false, "");
        puts("\n");
    }
    ଶୂନ୍ୟ visit(AstCExprUser* nodep) override {
        putnbs(nodep, "");
        ofp()->putsNoTracking("\n");
        ଯଦି (/* is always from $c */ v3Global.opt.decoration() && !v3Global.opt.protectIds()) {
            ofp()->putsNoTracking(
                (nodep->isPure() ? "// $cpure expression at " : "// $c expression at ")
                + nodep->fileline()->ascii() + "\n");
        }
        emitNodesWithText(nodep->nodesp(), m_useSelfForThis, false, "");
        puts("\n");
    }

    // Operators
    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି_Termop* nodep) override {
        emitOpName(nodep, nodep->emitC(), nullptr, nullptr, nullptr);
    }
    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି_Uniop* nodep) override {
        ଯଦି (nodep->emit_Check_Max_ଶବ୍ଦଗୁଡ଼ିକ()
            && (nodep->width_ଶବ୍ଦଗୁଡ଼ିକ() > VL_MULS_MAX_ଶବ୍ଦଗୁଡ଼ିକ
                || nodep->lhsp()->width_ଶବ୍ଦଗୁଡ଼ିକ() > VL_MULS_MAX_ଶବ୍ଦଗୁଡ଼ିକ)) {
            nodep->v3warn(
                E_UNSUPPORTED,
                "Unsupported: "
                    << nodep->prettyOperatorName() << " operator of " << nodep->width()
                    << " bits exceeds hardcoded limit VL_MULS_MAX_WORDS in verilatedos.h");
        }
        ଯଦି (emitSimpleOk(nodep)) {
            putnbs(nodep, "(");
            puts(nodep->emitSimpleOperator());
            puts(" ");
            iterateAndNextConstNull(nodep->lhsp());
            puts(")");
        } ଅନ୍ୟଥା {
            emitOpName(nodep, nodep->emitC(), nodep->lhsp(), nullptr, nullptr);
        }
    }
    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି_Biop* nodep) override {
        ଯଦି (nodep->emit_Check_Max_ଶବ୍ଦଗୁଡ଼ିକ() && nodep->width_ଶବ୍ଦଗୁଡ଼ିକ() > VL_MULS_MAX_ଶବ୍ଦଗୁଡ଼ିକ) {
            nodep->v3warn(
                E_UNSUPPORTED,
                "Unsupported: "
                    << nodep->prettyOperatorName() << " operator of " << nodep->width()
                    << " bits exceeds hardcoded limit VL_MULS_MAX_WORDS in verilatedos.h");
        }
        ଯଦି (emitSimpleOk(nodep)) {
            putnbs(nodep, "(");
            iterateAndNextConstNull(nodep->lhsp());
            puts(" ");
            putbs(nodep->emitSimpleOperator());
            puts(" ");
            iterateAndNextConstNull(nodep->rhsp());
            puts(")");
        } ଅନ୍ୟଥା {
            emitOpName(nodep, nodep->emitC(), nodep->lhsp(), nodep->rhsp(), nullptr);
        }
    }
    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି_Triop* nodep) override {
        UASSERT_OBJ(!emitSimpleOk(nodep), nodep, "Triop cannot be described in a simple way");
        emitOpName(nodep, nodep->emitC(), nodep->lhsp(), nodep->rhsp(), nodep->thsp());
    }
    ଶୂନ୍ୟ visit(AstCvtPackString* nodep) override { emitCvtPackStr(nodep->lhsp()); }
    ଶୂନ୍ୟ visit(AstRedXor* nodep) override {
        ଯଦି (nodep->lhsp()->isWide()) {
            visit(static_cast<Ast_ଗଣ୍ଠି_Uniop*>(nodep));
        } ଅନ୍ୟଥା {
            ଅଚଳ AstVarRef* ଅଚଳ vrefp = VN_CAST(nodep->lhsp(), VarRef);
            ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା widthPow2 = vrefp ? vrefp->varp()->dtypep()->widthPow2()
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
    ଶୂନ୍ୟ visit(AstCCast* nodep) override {
        // Extending a value of the same word width is just a NOP.
        ଯଦି (ଅଚଳ AstClassRefDType* ଅଚଳ classDtypep
            = VN_CAST(nodep->dtypep()->skipRefp(), ClassRefDType)) {
            putns(nodep, "(" + classDtypep->cType("", false, false) + ")(");
        } ଅନ୍ୟଥା ଯଦି (nodep->size() <= VL_BYTESIZE) {
            putns(nodep, "(::CData)(");
        } ଅନ୍ୟଥା ଯଦି (nodep->size() <= VL_SHORTSIZE) {
            putns(nodep, "(::SData)(");
        } ଅନ୍ୟଥା ଯଦି (nodep->size() <= VL_IDATASIZE) {
            putns(nodep, "(::IData)(");
        } ଅନ୍ୟଥା {
            putns(nodep, "(::QData)(");
        }
        iterateAndNextConstNull(nodep->lhsp());
        puts(")");
    }
    ଶୂନ୍ୟ visit(AstCond* nodep) override {
        // Widths match up already, so we'll just use C++'s operator w/o any temps.
        ଯଦି (nodep->thenp()->isWide()) {
            emitOpName(nodep, nodep->emitC(), nodep->condp(), nodep->thenp(), nodep->elsep());
        } ଅନ୍ୟଥା {
            putnbs(nodep, "(");
            iterateAndNextConstNull(nodep->condp());
            putbs(" ? ");
            iterateAndNextConstNull(nodep->thenp());
            putbs(" : ");
            iterateAndNextConstNull(nodep->elsep());
            puts(")");
        }
    }
    ଶୂନ୍ୟ visit(AstMatchMasked* nodep) override {
        emitOpName(nodep, nodep->emitC(), nodep->lhsp(), nodep->matchp(), nullptr);
    }
    ଶୂନ୍ୟ visit(AstMemberSel* nodep) override {
        ଅଚଳ AstVar* ଅଚଳ varp = nodep->varp();
        ଅଚଳ bool dereferenceCovergroupRef
            = varp->covergroupRefMember() && nodep->access().isReadOrRW();
        ଯଦି (dereferenceCovergroupRef) putnbs(nodep, "(*");
        iterateAndNextConstNull(nodep->fromp());
        putnbs(nodep, "->");
        ଯଦି (varp->isIfaceRef()) {
            // varp is the __Viftop companion (e.g. "tx__Viftop"); use the
            // MemberSel name which matches the cell's C++ member (e.g. "tx").
            puts(nodep->nameProtect());
        } ଅନ୍ୟଥା {
            puts(varName(varp));
        }
        ଯଦି (dereferenceCovergroupRef) puts(")");
    }
    ଶୂନ୍ୟ visit(AstStructSel* nodep) override {
        iterateAndNextConstNull(nodep->fromp());
        putnbs(nodep, ".");
        puts(nodep->nameProtect());
    }
    ଶୂନ୍ୟ visit(AstNullCheck* nodep) override {
        putns(nodep, "VL_NULL_CHECK(");
        iterateAndNextConstNull(nodep->lhsp());
        puts(", ");
        putsQuoted(protect(nodep->fileline()->filename()));
        puts(", ");
        puts(cvtToStr(nodep->fileline()->lineno()));
        puts(")");
    }
    ଶୂନ୍ୟ visit(AstNewCopy* nodep) override {
        // Polymorphic shallow clone: preserves runtime type via virtual clone()
        // VL_NULL_CHECK enforces null check per IEEE 1800-2023 8.7
        putns(nodep, "VL_NULL_CHECK(");
        ଯଦି (VN_IS(nodep->rhsp(), Const) && VN_AS(nodep->rhsp(), Const)->isNull()) {
            // V3Const folded rhs to null: emit a typed empty ref so VL_NULL_CHECK fires
            ଅଚଳ AstClassRefDType* ଅଚଳ refDTypep
                = VN_CAST(nodep->dtypep()->skipRefp(), ClassRefDType);
            puts(refDTypep->cType("", false, false) + "{}");
        } ଅନ୍ୟଥା {
            iterateAndNextConstNull(nodep->rhsp());
        }
        puts(", ");
        putsQuoted(protect(nodep->fileline()->filename()));
        puts(", ");
        puts(cvtToStr(nodep->fileline()->lineno()));
        puts(").clone(vlSymsp->__Vm_deleter)");
    }
    ଶୂନ୍ୟ visit(AstSel* nodep) override {
        // Note ASSIGN checks for this on a LHS
        UASSERT_OBJ(nodep->widthMin() == nodep->widthConst(), nodep, "Width mismatch");
        emitOpName(nodep, nodep->emitC(), nodep->fromp(), nodep->lsbp(), nullptr);
    }
    ଶୂନ୍ୟ visit(AstReplicate* nodep) override {
        ଯଦି (nodep->srcp()->widthMin() == 1 && !nodep->isWide()) {
            UASSERT_OBJ((static_cast<ପୂର୍ଣ୍ଣସଂଖ୍ୟା>(VN_AS(nodep->countp(), Const)->toUInt())
                         * nodep->srcp()->widthMin())
                            == nodep->widthMin(),
                        nodep, "Replicate non-constant or width miscomputed");
            putns(nodep, "VL_REPLICATE_");
            emitIQW(nodep);
            puts("OI(");
            ଯଦି (nodep->srcp()) puts(cvtToStr(nodep->srcp()->widthMin()));
            puts(", ");
            iterateAndNextConstNull(nodep->srcp());
            puts(", ");
            iterateAndNextConstNull(nodep->countp());
            puts(")");
        } ଅନ୍ୟଥା {
            emitOpName(nodep, nodep->emitC(), nodep->srcp(), nodep->countp(), nullptr);
        }
    }
    ଶୂନ୍ୟ emitStreamR(AstStreamR* nodep, Ast_ଗଣ୍ଠି* parent) {
        //TODO: This might need to handle more cases like the visit(AstStreamR) function
        emitOpName(nodep, nodep->emitC(), nodep->lhsp(), nodep->rhsp(), nullptr);
    }
    ଶୂନ୍ୟ visit(AstStreamR* nodep) override {
        //The parrent node of our AstStreamR will give just enough info for what streamR should
        //output if nodep->backp() is not the parent then emitStreamR should have been used. throw
        //an error
        bool backpIsParent = (nodep->backp()->op1p() == nodep || nodep->backp()->op2p() == nodep);
        UASSERT(backpIsParent, "can not find return type for streamR");
        ଯଦି (ଅଚଳ AstQueueDType* ଅଚଳ qdtp
            = VN_CAST(nodep->backp()->dtypep()->skipRefp(), QueueDType)) {
            ଯଦି (nodep->lhsp()->isWide()) {
                emitOpName(nodep,
                           "VL_STREAMR_%nq%lq%rq(%lw," + std::to_string(qdtp->subDTypep()->width())
                               + ", %P, %li, %ri)",
                           nodep->lhsp(), nodep->rhsp(), nullptr);
            } ଅନ୍ୟଥା {
                emitOpName(nodep, "VL_STREAMR_%nq%lq%rq(%lw, %P, %li, %ri)", nodep->lhsp(),
                           nodep->rhsp(), nullptr);
            }
        } ଅନ୍ୟଥା ଯଦି (VN_IS(nodep->lhsp()->dtypep()->skipRefp(), QueueDType)) {
            ଯଦି (!((nodep->backp()->op1p() && nodep->backp()->op1p()->isWide())
                  || (nodep->backp()->op2p() && nodep->backp()->op2p()->isWide()))) {
                //If our lhsp is a queue make sure we streamR and return the correct type.
                //If either side is wide or the previous node is string type dont use this case
                emitOpName(nodep->backp(), "VL_STREAMR_%nq%lq%rq(%lw, %P, %li, %ri)",
                           nodep->lhsp(), nodep->rhsp(), nullptr);
            } ଅନ୍ୟଥା {
                emitOpName(nodep, nodep->emitC(), nodep->lhsp(), nodep->rhsp(), nullptr);
            }
        } ଅନ୍ୟଥା {
            emitOpName(nodep, nodep->emitC(), nodep->lhsp(), nodep->rhsp(), nullptr);
        }
    }
    ଶୂନ୍ୟ visit(AstStreamL* nodep) override {
        // Attempt to use a "fast" stream function for slice size = power of 2
        ଯଦି (!nodep->isWide()) {
            ଅଚଳ uint32_t isPow2 = VN_AS(nodep->rhsp(), Const)->ଅଙ୍କ().ଗଣନା_Ones() == 1;
            ଅଚଳ uint32_t sliceSize = VN_AS(nodep->rhsp(), Const)->toUInt();
            ଯଦି (isPow2 && sliceSize <= (nodep->isQuad() ? ଆକାର(uint64_t) : ଆକାର(uint32_t))) {
                putns(nodep, "VL_STREAML_FAST_");
                bool usesQueue = false;
                AstQueueDType* qtypep
                    = nodep->backp()->op2p()
                          ? VN_CAST(nodep->backp()->op2p()->dtypep()->skipRefp(), QueueDType)
                          : nullptr;
                ଯଦି (VN_IS(nodep->backp(), Assign)
                    && qtypep) {  // If we are assigning to a queue then emit the correct symbol
                    puts("R");  // R for queue
                    usesQueue = true;
                } ଅନ୍ୟଥା {
                    emitIQW(nodep);
                }
                emitIQW(nodep->lhsp());
                puts("I(");
                puts(cvtToStr(nodep->lhsp()->widthMin()));
                puts(", ");
                ଯଦି (usesQueue) {
                    iterateAndNextConstNull(nodep->backp()->op2p());
                    puts(", ");
                }
                iterateAndNextConstNull(nodep->lhsp());
                puts(", ");
                ଅଚଳ uint32_t rd_log2 = V3_ସଂଖ୍ୟା::log2b(VN_AS(nodep->rhsp(), Const)->toUInt());
                puts(cvtToStr(rd_log2) + ")");
                ଫେରନ୍ତୁ;
            }
        }
        ଯଦି (VN_IS(nodep->backp(), Assign)
            && VN_IS(nodep->backp()->op2p()->dtypep()->skipRefp(), QueueDType)) {
            ପୂର୍ଣ୍ଣସଂଖ୍ୟା queueWidth
                = nodep->backp()->op2p()->dtypep()->subDTypep()->width();  //We need to know the
                                                                           //width of both sides
            emitOpName(nodep,
                       "VL_STREAML_%nq%lq%rq(%lw," + std::to_string(queueWidth)
                           + ", %P, %li, %ri)",
                       nodep->lhsp(), nodep->rhsp(), nullptr);
        } ଅନ୍ୟଥା {
            emitOpName(nodep, nodep->emitC(), nodep->lhsp(), nodep->rhsp(), nullptr);
        }
    }
    ଶୂନ୍ୟ visit(AstCastDynamic* nodep) override {
        putnbs(nodep, "VL_CAST_DYNAMIC(");
        iterateAndNextConstNull(nodep->fromp());
        puts(", ");
        iterateAndNextConstNull(nodep->ଶୀର୍ଷ());
        puts(")");
    }
    ଶୂନ୍ୟ visit(Ast_ଗଣନା_Bits* nodep) override {
        putnbs(nodep, "VL_COUNTBITS_");
        emitIQW(nodep->lhsp());
        puts("(");
        puts(cvtToStr(nodep->lhsp()->widthMin()));
        puts(", ");
        ଯଦି (nodep->lhsp()->isWide()) {
            puts(cvtToStr(nodep->lhsp()->width_ଶବ୍ଦଗୁଡ଼ିକ()));  // Note argument width, not node width
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
    ଶୂନ୍ୟ visit(AstTriggered* nodep) override {
        // Test each word of the trigger vector with a mask of the relevant bits
        std::map<uint32_t, uint64_t> wordMasks;
        ପାଇଁ (ଅଚଳ uint32_t index : nodep->indices()) {
            wordMasks[VL_BITWORD_Q(index)] |= VL_BIT_Q(index);
        }
        putns(nodep, "((");
        bool first = true;
        ପାଇଁ (ଅଚଳ auto& wordMask : wordMasks) {
            ଯଦି (!first) puts("\n| ");
            first = false;
            ଅଚଳ bool allOnes = wordMask.second == ~0ULL;
            ଯଦି (!allOnes) puts("(" + V3_ସଂଖ୍ୟା{nodep, 64}.setQuad(wordMask.second).emitC() + " & ");
            iterateConst(nodep->trigRefp());
            puts("[" + std::to_string(wordMask.first) + "U]");
            ଯଦି (!allOnes) puts(")");
        }
        puts(") != 0ULL)");
    }
    // Terminals
    ଶୂନ୍ୟ visit(AstVarRef* nodep) override {
        ଅଚଳ AstVar* ଅଚଳ varp = nodep->varp();

        // Reference to constant pool variable is special
        ଯଦି (varp->constPoolEntry()) {
            putns(nodep, EmitCUtil::constPoolName(varp));
            ଫେରନ୍ତୁ;
        }

        ଅଚଳ Ast_ଗଣ୍ଠି_Module* ଅଚଳ varModp = EmitCParentModule::get(varp);
        ଅଚଳ bool dereferenceCovergroupRef
            = varp->covergroupRefMember() && nodep->access().isReadOrRW();
        ଯଦି (dereferenceCovergroupRef) putns(nodep, "(*");
        ଯଦି (VN_IS(varModp, Class) && varModp != m_modp) {
            // Superclass member reference
            putns(nodep, EmitCUtil::prefixNameProtect(varModp) + "::");
        } ଅନ୍ୟଥା ଯଦି (varp->isIfaceRef()) {
            putns(nodep, nodep->selfPointerProtect(m_useSelfForThis));
            ଫେରନ୍ତୁ;
        } ଅନ୍ୟଥା ଯଦି (!nodep->selfPointer().isEmpty()) {
            emitDereference(nodep, nodep->selfPointerProtect(m_useSelfForThis));
        }
        putns(nodep, varName(varp));
        ଯଦି (dereferenceCovergroupRef) puts(")");
    }
    ଶୂନ୍ୟ visit(AstAddrOfCFunc* nodep) override {
        // Note: Can be thought to handle more, but this is all that is needed right now
        ଅଚଳ AstCFunc* ଅଚଳ funcp = nodep->funcp();
        UASSERT_OBJ(funcp->isLoose(), nodep, "Cannot take address of non-loose method");
        putns(nodep, "&");
        puts(funcNameProtect(funcp));
    }
    ଶୂନ୍ୟ visit(AstConst* nodep) override {  //
        ଯଦି (m_wideTempRefp && nodep->isWide()) {
            UASSERT_OBJ(m_wideTempRefp, nodep, "Wide Constant w/ no temp");
            emitConstantW(nodep, m_wideTempRefp);
            m_wideTempRefp = nullptr;  // We used it, fail if set it a second time
        } ଅନ୍ୟଥା {
            emitConstant(nodep);
        }
    }
    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି_Cons* nodep) override { emitValCons(nodep); }
    ଶୂନ୍ୟ visit(AstThisRef* nodep) override {
        putnbs(nodep, nodep->dtypep()->cType("", false, false));
        puts("{");
        puts(VSelfPointerText::replaceThis(m_useSelfForThis, "this"));
        puts("}");
    }
    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି_Sel* nodep) override {
        ଯଦି (!VN_IS(nodep, ArraySel) && !VN_IS(nodep, WordSel)) {
            visit(static_cast<Ast_ଗଣ୍ଠି_Biop*>(nodep));
            ଫେରନ୍ତୁ;
        }
        // ArraySel or WordSel
        iterateAndNextConstNull(nodep->fromp());
        // Special case constant index for readability
        ଯଦି (AstConst* ଅଚଳ idxp = VN_CAST(nodep->bitp(), Const)) {
            puts("[" + std::to_string(idxp->toUInt()) + "U]");
            ଫେରନ୍ତୁ;
        }
        putbs("[");
        iterateAndNextConstNull(nodep->bitp());
        puts("]");
    }

    //
    ଶୂନ୍ୟ visit(AstExecGraph* nodep) override {
        // The location of the AstExecGraph within the containing AstCFunc is where we want to
        // invoke the graph and wait for it to complete. The run-time library does just that.
        putns(nodep, "vl_invokeExecGraph(" + EmitCUtil::execGraphName(nodep) + ", vlSelf, ");
        iterateConst(nodep->triggerp());
        puts(".data());\n");
    }

    // Default
    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି* nodep) override {  // LCOV_EXCL_START
        putns(nodep, "\n???? // "s + nodep->prettyTypeName() + "\n");
        iterateChildrenConst(nodep);
        ଯଦି (!v3Global.opt.lintOnly()) {  // An internal problem, so suppress
            nodep->v3fatalSrc("Unknown node type reached emitter: " << nodep->prettyTypeName());
        }
    }  // LCOV_EXCL_STOP

ସୁରକ୍ଷିତ:
    EmitCFunc() = ପୂର୍ବନିର୍ଦ୍ଧାରିତ;
    ~EmitCFunc() override = ପୂର୍ବନିର୍ଦ୍ଧାରିତ;
};

#endif  // guard
