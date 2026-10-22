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

#ವ್ಯಾಖ್ಯಾನಿತವಾಗದಿದ್ದರೆ VERILATOR_V3EMITCFUNC_H_
#ವ್ಯಾಖ್ಯಾನಿಸು VERILATOR_V3EMITCFUNC_H_

#ಅನ್ತರ್ಭಾವ "config_build.h"
#ಅನ್ತರ್ಭಾವ "verilatedos.h"

#ಅನ್ತರ್ಭಾವ "V3EmitCConstInit.h"
#ಅನ್ತರ್ಭಾವ "V3Global.h"
#ಅನ್ತರ್ಭಾವ "V3MemberMap.h"

#ಅನ್ತರ್ಭಾವ <algorithm>
#ಅನ್ತರ್ಭಾವ <map>
#ಅನ್ತರ್ಭಾವ <unordered_map>
#ಅನ್ತರ್ಭಾವ <unordered_set>
#ಅನ್ತರ್ಭಾವ <vector>

// Number of VL_CONST_W_*X's in verilated.h (IE VL_CONST_W_8X is last)
constexpr ಪೂರ್ಣಾಂಕ EMITC_ಅಂಕೆ_CONSTW = 8;

//######################################################################
// Emit lazy forward declarations

class EmitCLazyDecls final : ಸಾರ್ವಜನಿಕ VNVisitorConst {
    // NODE STATE/TYPES
    // None allowed to support threaded emitting

    // MEMBERS
    std::unordered_ಗಣ<string> m_emittedManually;  // Set of names already declared manually.
    EmitCBaseVisitorConst& m_emitter;  // For access to file output
    bool m_needs_Blank_ಸಾಲು = false;  // Emit blank line if any declarations were emitted (cosmetic)
    std::ಗಣ<Ast_ಗಂಟು*> m_emitted;  // -> in set. Already emitted decl for symbols.

    // METHODS
    bool declaredOnce(Ast_ಗಂಟು* nodep) { ಹಿಂತಿರುಗಿ m_emitted.ಸೇರಿಸು(nodep).ಎರಡನೇ; }

    ಶೂನ್ಯ lazyDeclare(AstCFunc* funcp) {
        // Already declared in this compilation unit
        ಆದರೆ (!declaredOnce(funcp)) ಹಿಂತಿರುಗಿ;
        // Check if this kind of function is lazily declared
        ಆದರೆ (!(funcp->is_ವಿಧಾನ() && funcp->isLoose()) && !funcp->dpiImportPrototype()) ಹಿಂತಿರುಗಿ;
        // Already declared manually
        ಆದರೆ (m_emittedManually.ಎಣಿಕೆ_2(funcp->nameProtect())) ಹಿಂತಿರುಗಿ;
        // Needs lazy declaration, emit one
        m_emitter.emitCFuncDecl(funcp, EmitCParentModule::get(funcp), funcp->dpiImportPrototype());
        m_needs_Blank_ಸಾಲು = true;
    }

    ಶೂನ್ಯ lazyDeclareConstPoolVar(AstVar* varp) {
        ಆದರೆ (!declaredOnce(varp)) ಹಿಂತಿರುಗಿ;  // Already declared
        ಸ್ಥಿರಾಂಕ string nameProtect = EmitCUtil::constPoolName(varp);
        m_emitter.putns(varp, "extern const ");
        m_emitter.puts(varp->dtypep()->cType(nameProtect, false, false));
        m_emitter.puts(";\n");
        m_needs_Blank_ಸಾಲು = true;
    }

    // VISITORS
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು_CCall* nodep) override {
        lazyDeclare(nodep->funcp());
        iterateChildrenConst(nodep);
    }

    ಶೂನ್ಯ ಭೇಟಿ(Ast_Addr_ದ_CFunc* nodep) override {
        lazyDeclare(nodep->funcp());
        iterateChildrenConst(nodep);
    }

    ಶೂನ್ಯ ಭೇಟಿ(AstVarRef* nodep) override {
        AstVar* ಸ್ಥಿರಾಂಕ varp = nodep->varp();
        // Only constant pool symbols are lazy declared for now ...
        ಆದರೆ (EmitCParentModule::get(varp)->isConstPool()) lazyDeclareConstPoolVar(varp);
    }

    ಶೂನ್ಯ ಭೇಟಿ(Ast_Exec_ನಕ್ಷೆ* nodep) override {
        ಆದರೆ (!declaredOnce(nodep)) ಹಿಂತಿರುಗಿ;
        m_emitter.putns(nodep, "extern VlExecGraph " + EmitCUtil::exec_ನಕ್ಷೆ_Name(nodep) + ";\n");
        m_needs_Blank_ಸಾಲು = true;
    }

    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು* nodep) override { iterateChildrenConst(nodep); }

ಸಾರ್ವಜನಿಕ:
    explicit EmitCLazyDecls(EmitCBaseVisitorConst& emitter)
        : m_emitter{emitter} {}
    ಶೂನ್ಯ emit(Ast_ಗಂಟು* nodep) {
        m_needs_Blank_ಸಾಲು = false;
        iterateChildrenConst(nodep);
        ಆದರೆ (m_needs_Blank_ಸಾಲು) m_emitter.puts("\n");
    }
    ಶೂನ್ಯ emit(ಸ್ಥಿರಾಂಕ string& ಪೂರ್ವಪ್ರತ್ಯಯ, ಸ್ಥಿರಾಂಕ string& name, ಸ್ಥಿರಾಂಕ string& suffix) {
        m_emittedManually.ಸೇರಿಸು(name);
        m_emitter.ensure_New_ಸಾಲು();
        m_emitter.puts(ಪೂರ್ವಪ್ರತ್ಯಯ);
        m_emitter.puts(name);
        m_emitter.puts(suffix);
        m_emitter.ensure_New_ಸಾಲು();
    }
    ಶೂನ್ಯ declared(AstCFunc* nodep) { m_emitted.ಸೇರಿಸು(nodep); }
    ಶೂನ್ಯ reset() { m_emitted.clear(); }
};

// ######################################################################
//  Emit statements and expressions

class EmitCFunc VL_NOT_FINAL : ಸಾರ್ವಜನಿಕ EmitCConstInit {
    VMemberMap m_memberMap;
    AstVarRef* m_wide_ತಾತ್ಕಾಲಿಕ_Refp = nullptr;  // Variable that _WW macros should be setting
    std::unordered_map<Ast_Jump_ಖಂಡ*, ಗಾತ್ರ_t> m_labelNumbers;  // Label numbers for AstJumpBlocks
    bool m_createdScopeHash = false;  // Already created a scope hash

ಸಂರಕ್ಷಿತ:
    VL_DEFINE_DEBUG_FUNCTIONS;

    EmitCLazyDecls m_lazyDecls{*this};  // Visitor for emitting lazy declarations
    bool m_useSelfForThis = false;  // Replace "this" with "vlSelf"
    bool m_usevlSelfRef = false;  // Use vlSelfRef reference instead of vlSelf pointer
    ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_Module* m_modp = nullptr;  // Current module being emitted
    ಸ್ಥಿರಾಂಕ AstCFunc* m_cfuncp = nullptr;  // Current function being emitted
    bool m_instantiatesOwnProcess = false;
    ಸ್ಥಿರಾಂಕ AstClassPackage* m_classOrPackage = nullptr;  // Pointer to current class or package
    string m_classOrPackageHash;  // Hash of class or package name

    bool constructorNeedsProcess(ಸ್ಥಿರಾಂಕ AstClass* ಸ್ಥಿರಾಂಕ classp) {
        ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ newp = m_memberMap.ಕಂಡುಹಿಡಿ_Member(classp, "new");
        ಆದರೆ (!newp) ಹಿಂತಿರುಗಿ false;
        ಸ್ಥಿರಾಂಕ AstCFunc* ಸ್ಥಿರಾಂಕ ctorp = VN_CAST(newp, CFunc);
        ಆದರೆ (!ctorp) ಹಿಂತಿರುಗಿ false;
        UASSERT_OBJ(ctorp->isConstructor(), ctorp, "`new` is not a constructor!");
        ಹಿಂತಿರುಗಿ ctorp->needProcess();
    }

    bool constructorNeedsProcess(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_DType* ಸ್ಥಿರಾಂಕ dtypep) {
        ಆದರೆ (ಸ್ಥಿರಾಂಕ AstClassRefDType* ಸ್ಥಿರಾಂಕ crefdtypep = VN_CAST(dtypep, ClassRefDType))
            ಹಿಂತಿರುಗಿ constructorNeedsProcess(crefdtypep->classp());
        ಹಿಂತಿರುಗಿ false;
    }

    bool coverage_ಬಳಸುತ್ತದೆ_Local_Counter(Ast_ಗಂಟು_Cover_Decl* ಸ್ಥಿರಾಂಕ declp) ಸ್ಥಿರಾಂಕ {
        // Only functions with an object receiver can address the module-local
        // coverage array. Static/package/class helper functions still use the
        // symbol-table array, as they may have vlSymsp but no vlSelf/this.
        // Also require the coverage declaration to belong to the current
        // module; cloned task bodies can increment declarations owned by a
        // different module, and those must keep using the global array.
        ಹಿಂತಿರುಗಿ m_cfuncp && !m_cfuncp->isStatic() && !VN_IS(m_modp, Class)
               && !VN_IS(m_modp, ClassPackage)
               && EmitCParentModule::get(declp->ದತ್ತಾಂಶ_Decl_Thisp()) == m_modp;
    }
    ಪೂರ್ಣಾಂಕ coverage_Bin_ಅಂಕೆ(Ast_ಗಂಟು_Cover_Decl* ಸ್ಥಿರಾಂಕ declp, bool forceGlobal = false) ಸ್ಥಿರಾಂಕ {
        ಹಿಂತಿರುಗಿ !forceGlobal && coverage_ಬಳಸುತ್ತದೆ_Local_Counter(declp)
                   ? declp->ದತ್ತಾಂಶ_Decl_Thisp()->local_Bin_ಅಂಕೆ()
                   : declp->ದತ್ತಾಂಶ_Decl_Thisp()->bin_ಅಂಕೆ();
    }
    ಶೂನ್ಯ putCoverageArray(Ast_ಗಂಟು_Cover_Decl* ಸ್ಥಿರಾಂಕ declp, bool forceGlobal = false) {
        ಆದರೆ (!forceGlobal && coverage_ಬಳಸುತ್ತದೆ_Local_Counter(declp)) {
            // Keep counters on the module object when possible, so every
            // no-inline instance has storage available if coverage output is
            // later written with forcePerInstance.
            puts(m_cfuncp->isLoose() ? "vlSelf->__Vcoverage" : "this->__Vcoverage");
        } ಇಲ್ಲದಿದ್ದರೆ {
            // Static/package/class helper functions do not have vlSelf, so
            // keep using the shared symbol-table coverage array there.
            puts("vlSymsp->__Vcoverage");
        }
    }
    ಶೂನ್ಯ emit_Cover_Other_Decl_ಸೇರಿಸು(AstCoverOtherDecl* nodep, bool forceGlobal = false) {
        putns(nodep, "vlSelf->__vlCoverInsert(");  // As Declared in emitCoverageDecl
        putCoverageArray(nodep, forceGlobal);
        puts(" + ");
        puts(cvt_ಗಮ್ಯ_Str(coverage_Bin_ಅಂಕೆ(nodep, forceGlobal)));
        // first controls whether duplicate module instances are collapsed in
        // the default coverage view. The following flag says whether countp
        // points to an object-local counter; only those counters must be kept
        // real for later instances so forcePerInstance can split hierarchy.
        puts(", first");  // Enable, passed from __Vconfigure parameter
        puts(", ");
        puts(!forceGlobal && coverage_ಬಳಸುತ್ತದೆ_Local_Counter(nodep) ? "true" : "false");
        puts(", ");
        putsQuoted(protect(nodep->fileline()->filename()));
        puts(", ");
        puts(cvt_ಗಮ್ಯ_Str(nodep->fileline()->lineno()));
        puts(", ");
        puts(cvt_ಗಮ್ಯ_Str(nodep->ಸ್ಥಾನಾಂತರ() + nodep->fileline()->ಮೊದಲ_ಸ್ತಂಭ()));
        puts(", ");
        putsQuoted((!nodep->hier().ಖಾಲಿಗಳು() ? "." : "")
                   + VIdProtect::protect_ಪದಗಳು_If(nodep->hier(), nodep->protect()));
        puts(", ");
        putsQuoted(VIdProtect::protect_ಪದಗಳು_If(nodep->ಪುಟ(), nodep->protect()));
        puts(", ");
        putsQuoted(VIdProtect::protect_ಪದಗಳು_If(nodep->comment(), nodep->protect()));
        puts(", ");
        putsQuoted(nodep->linescov());
        puts(", ");
        putsQuoted(VIdProtect::protect_ಪದಗಳು_If(nodep->fsmVar(), nodep->protect()));
        puts(", ");
        putsQuoted(VIdProtect::protect_ಪದಗಳು_If(nodep->fsmFrom(), nodep->protect()));
        puts(", ");
        putsQuoted(VIdProtect::protect_ಪದಗಳು_If(nodep->fsm_ಗಮ್ಯ(), nodep->protect()));
        puts(", ");
        putsQuoted(VIdProtect::protect_ಪದಗಳು_If(nodep->fsm_ಟ್ಯಾಗ್(), nodep->protect()));
        puts(");\n");
    }

ಸಾರ್ವಜನಿಕ:
    // METHODS
    bool display_Emit_ಶೀರ್ಷಿಕೆ(Ast_ಗಂಟು* nodep);
    ಶೂನ್ಯ display_ಗಂಟು(Ast_ಗಂಟು* nodep, AstSFormatF* fmtp, ಸ್ಥಿರಾಂಕ string& vformat, Ast_ಗಂಟು* exprsp,
                     bool is_ಪರಿಶೀಲನೆ);

    bool emitSimpleOk(Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* nodep);
    ಶೂನ್ಯ emitIQW(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* nodep) {
        // See "Type letters" in verilated.h
        // Other abbrevs: "C"har, "S"hort, "F"loat, "D"ouble, stri"N"g, "R"=queue, "U"npacked
        puts(nodep->dtypep()->skipRefp()->charIQWN());
    }
    ಶೂನ್ಯ emitRU(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* nodep) {
        ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_DType* dtp = nodep->dtypep()->skipRefp();
        // See "Type letters" in verilated.h
        ಆದರೆ (VN_IS(dtp, UnpackArrayDType))
            puts("U");
        ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (VN_IS(dtp, ಸರತಿ_DType) || VN_IS(dtp, DynArrayDType))
            puts("R");
    }
    ಶೂನ್ಯ emitScIQW(AstVar* nodep) {
        UASSERT_OBJ(nodep->isSc(), nodep, "emitting SystemC operator on non-SC variable");
        puts(nodep->isScBigUint()                           ? "SB"
             : (nodep->isScUint() || nodep->isScUintBool()) ? "SU"
             : nodep->isScBv()                              ? "SW"
             : nodep->isDouble()                            ? "SD"
                                                            : (nodep->isScQuad() ? "SQ" : "SI"));
    }
    ಶೂನ್ಯ emitDatap(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* nodep) {
        // When passing to a function with va_args the compiler doesn't
        // know need a pointer so when wide, need to look inside VlWide
        ಆದರೆ (nodep->isWide()) puts(".data()");
    }
    ಶೂನ್ಯ emitOpName(Ast_ಗಂಟು* nodep, ಸ್ಥಿರಾಂಕ string& format, Ast_ಗಂಟು* lhsp, Ast_ಗಂಟು* rhsp,
                    Ast_ಗಂಟು* thsp);
    ಶೂನ್ಯ emit_CCall_ವಾದಗಳು(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_CCall* nodep, ಸ್ಥಿರಾಂಕ string& self_ಸೂಚಕ, bool ಒಳ_Process);
    ಶೂನ್ಯ emitDereference(Ast_ಗಂಟು* nodep, ಸ್ಥಿರಾಂಕ string& ಸೂಚಕ);
    std::string dereferenceString(ಸ್ಥಿರಾಂಕ std::string& ಸೂಚಕ) ಸ್ಥಿರಾಂಕ;
    ಶೂನ್ಯ emitCvtPackStr(Ast_ಗಂಟು* nodep);
    ಶೂನ್ಯ emitConstant(AstConst* nodep);
    ಶೂನ್ಯ emitConstantString(ಸ್ಥಿರಾಂಕ AstConst* nodep);
    ಶೂನ್ಯ emitVarReset(ಸ್ಥಿರಾಂಕ string& ಪೂರ್ವಪ್ರತ್ಯಯ, AstVar* varp, bool constructing);
    string emitVarResetRecurse(ಸ್ಥಿರಾಂಕ AstVar* varp, bool constructing,
                               ಸ್ಥಿರಾಂಕ string& varNameProtected, Ast_ಗಂಟು_DType* dtypep, ಪೂರ್ಣಾಂಕ ಆಳ,
                               ಸ್ಥಿರಾಂಕ string& suffix, ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* valuep);
    ಶೂನ್ಯ emitVarResetScopeHash();
    ಶೂನ್ಯ emitChangeDet();
    ಶೂನ್ಯ put_Comma_Iterate_ಮುಂದಿನ(Ast_ಗಂಟು* nodep, bool comma = false) {
        ಗಾಗಿ (Ast_ಗಂಟು* subnodep = nodep; subnodep; subnodep = subnodep->nextp()) {
            ಆದರೆ (comma) puts(", ");
            iterateConst(subnodep);
            comma = true;
        }
    }
    ಮಾದರಿ <typename T>
    string optionalProcArg(ಸ್ಥಿರಾಂಕ T* ಸ್ಥಿರಾಂಕ nodep) {
        ಹಿಂತಿರುಗಿ (nodep && constructorNeedsProcess(nodep)) ? "vlProcess, " : "";
    }
    ಸ್ಥಿರಾಂಕ AstCNew* getSuperNewCallRecursep(Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ nodep) {
        // Get the super.new call
        ಆದರೆ (!nodep) ಹಿಂತಿರುಗಿ nullptr;
        ಆದರೆ (ಸ್ಥಿರಾಂಕ AstCNew* ಸ್ಥಿರಾಂಕ cnewp = VN_CAST(nodep, CNew)) ಹಿಂತಿರುಗಿ cnewp;
        ಆದರೆ (ಸ್ಥಿರಾಂಕ Ast_Stmt_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ stmtp = VN_CAST(nodep, Stmt_ಅಭಿವ್ಯಕ್ತಿ)) {
            ಆದರೆ (ಸ್ಥಿರಾಂಕ AstCNew* ಸ್ಥಿರಾಂಕ cnewp = VN_CAST(stmtp->exprp(), CNew)) ಹಿಂತಿರುಗಿ cnewp;
        }
        ಆದರೆ (ಸ್ಥಿರಾಂಕ Ast_Jump_ಖಂಡ* ಸ್ಥಿರಾಂಕ blockp = VN_CAST(nodep, Jump_ಖಂಡ)) {
            ಆದರೆ (ಸ್ಥಿರಾಂಕ AstCNew* ಸ್ಥಿರಾಂಕ cnewp = getSuperNewCallRecursep(blockp->stmtsp())) {
                ಹಿಂತಿರುಗಿ cnewp;
            }
        }
        ಆದರೆ (ಸ್ಥಿರಾಂಕ AstCNew* ಸ್ಥಿರಾಂಕ cnewp = getSuperNewCallRecursep(nodep->nextp())) ಹಿಂತಿರುಗಿ cnewp;
        ಹಿಂತಿರುಗಿ nullptr;
    }

    ಶೂನ್ಯ emitConstantW(AstConst* nodep, AstVarRef* assigntop) {
        // For tradition and compilation speed, assign each word directly into
        // output variable instead of using '='
        putns(nodep, "");
        ಆದರೆ (nodep->ಅಂಕೆ().isFourState()) {
            nodep->v3warn(E_UNSUPPORTED, "Unsupported: 4-state numbers in this context");
            ಹಿಂತಿರುಗಿ;
        }

        ಪೂರ್ಣಾಂಕ ಮೇಲೆ_ಅಗಲ = nodep->ಅಂಕೆ().ಅಗಲ_ಗಮ್ಯ_Fit();
        ಪೂರ್ಣಾಂಕ chunks = 0;
        ಆದರೆ (ಮೇಲೆ_ಅಗಲ > EMITC_ಅಂಕೆ_CONSTW * VL_EDATASIZE) {
            // Output e.g. 8 words in groups of e.g. 8
            chunks = (ಮೇಲೆ_ಅಗಲ - 1) / (EMITC_ಅಂಕೆ_CONSTW * VL_EDATASIZE);
            ಮೇಲೆ_ಅಗಲ %= (EMITC_ಅಂಕೆ_CONSTW * VL_EDATASIZE);
            ಆದರೆ (ಮೇಲೆ_ಅಗಲ == 0) ಮೇಲೆ_ಅಗಲ = (EMITC_ಅಂಕೆ_CONSTW * VL_EDATASIZE);
        }
        {  // Upper e.g. 8 words
            ಆದರೆ (chunks) {
                putnbs(nodep, "VL_CONSTHI_W_");
                puts(cvt_ಗಮ್ಯ_Str(VL_ಪದಗಳು_I(ಮೇಲೆ_ಅಗಲ)));
                puts("X(");
                puts(cvt_ಗಮ್ಯ_Str(nodep->ಅಗಲ_Min()));
                puts(",");
                puts(cvt_ಗಮ್ಯ_Str(chunks * EMITC_ಅಂಕೆ_CONSTW * VL_EDATASIZE));
            } ಇಲ್ಲದಿದ್ದರೆ {
                putnbs(nodep, "VL_CONST_W_");
                puts(cvt_ಗಮ್ಯ_Str(VL_ಪದಗಳು_I(ಮೇಲೆ_ಅಗಲ)));
                puts("X(");
                puts(cvt_ಗಮ್ಯ_Str(nodep->ಅಗಲ_Min()));
            }
            puts(",");
            ಆದರೆ (!assigntop->self_ಸೂಚಕ().is_ಖಾಲಿಗಳು()) {
                emitDereference(assigntop, assigntop->self_ಸೂಚಕ_Protect(m_useSelfForThis));
            }
            puts(varName(assigntop->varp()));
            ಗಾಗಿ (ಪೂರ್ಣಾಂಕ ಶಬ್ದ = VL_ಪದಗಳು_I(ಮೇಲೆ_ಅಗಲ) - 1; ಶಬ್ದ >= 0; ಶಬ್ದ--) {
                // Only 32 bits - llx + long long here just to appease CPP format warning
                ofp()->ಬರೆ(",0x%08" PRIx64, static_cast<uint64_t>(nodep->ಅಂಕೆ().edata_ಶಬ್ದ(
                                                   ಶಬ್ದ + chunks * EMITC_ಅಂಕೆ_CONSTW)));
            }
            puts(")");
        }
        ಗಾಗಿ (chunks--; chunks >= 0; chunks--) {
            puts(";\n");
            putbs("VL_CONSTLO_W_");
            puts(cvt_ಗಮ್ಯ_Str(EMITC_ಅಂಕೆ_CONSTW));
            puts("X(");
            puts(cvt_ಗಮ್ಯ_Str(chunks * EMITC_ಅಂಕೆ_CONSTW * VL_EDATASIZE));
            puts(",");
            ಆದರೆ (!assigntop->self_ಸೂಚಕ().is_ಖಾಲಿಗಳು()) {
                emitDereference(assigntop, assigntop->self_ಸೂಚಕ_Protect(m_useSelfForThis));
            }
            puts(varName(assigntop->varp()));
            ಗಾಗಿ (ಪೂರ್ಣಾಂಕ ಶಬ್ದ = EMITC_ಅಂಕೆ_CONSTW - 1; ಶಬ್ದ >= 0; ಶಬ್ದ--) {
                // Only 32 bits - llx + long long here just to appease CPP format warning
                ofp()->ಬರೆ(",0x%08" PRIx64, static_cast<uint64_t>(nodep->ಅಂಕೆ().edata_ಶಬ್ದ(
                                                   ಶಬ್ದ + chunks * EMITC_ಅಂಕೆ_CONSTW)));
            }
            puts(")");
        }
    }

    ಶೂನ್ಯ emit_Nodes_With_ಪಠ್ಯ(Ast_ಗಂಟು* nodesp, bool useSelfForThis, bool tracking,
                           ಸ್ಥಿರಾಂಕ std::string& separator) {
        ಗಾಗಿ (Ast_ಗಂಟು* nodep = nodesp; nodep; nodep = nodep->nextp()) {
            ಆದರೆ (ಸ್ಥಿರಾಂಕ Ast_ಪಠ್ಯ* ಸ್ಥಿರಾಂಕ textp = VN_CAST(nodep, ಪಠ್ಯ_2)) {
                ಸ್ಥಿರಾಂಕ std::string ಪಠ್ಯ
                    = VSelf_ಸೂಚಕ_ಪಠ್ಯ::replaceThis(useSelfForThis, textp->ಪಠ್ಯ());
                ಆದರೆ (tracking) {
                    puts(ಪಠ್ಯ);
                } ಇಲ್ಲದಿದ್ದರೆ {
                    ofp()->putsNoTracking(ಪಠ್ಯ);
                }
            } ಇಲ್ಲದಿದ್ದರೆ {
                iterateConst(nodep);
            }
            ಆದರೆ (nodep->nextp()) puts(separator);
        }
    }

    ಶೂನ್ಯ putConstructorSubinit(ಸ್ಥಿರಾಂಕ AstClass* classp, AstCFunc* cfuncp) {
        // Direct non-virtual bases in declaration order
        ಗಾಗಿ (ಸ್ಥಿರಾಂಕ AstClassExtends* extp = classp->extendsp(); extp;
             extp = VN_AS(extp->nextp(), ClassExtends)) {
            ಆದರೆ (extp->classp()->isInterfaceClass()) ಮುಂದುವರಿ;
            ಆದರೆ (extp->classp()->use_Virtual_ಬಹಿರಂಗ()) ಮುಂದುವರಿ;
            puts(EmitCUtil::ಪೂರ್ವಪ್ರತ್ಯಯ_Name_Protect(extp->classp()));
            puts("::init");
            ಆದರೆ (constructorNeedsProcess(extp->classp())) {
                puts("(vlProcess, vlSymsp");
            } ಇಲ್ಲದಿದ್ದರೆ {
                puts("(vlSymsp");
            }
            // Handle super.new() args for the concrete parent
            ಆದರೆ (ಸ್ಥಿರಾಂಕ AstCNew* ಸ್ಥಿರಾಂಕ superNewCallp = getSuperNewCallRecursep(cfuncp->stmtsp())) {
                put_Comma_Iterate_ಮುಂದಿನ(superNewCallp->argsp(), true);
            }
            puts(");\n");
            // Since SystemVerilog allows only for extending from one class, for sure there won't
            // be any more constructors to be called. Even if there are some classes which are
            // "implemented", those are interface classes which do not have constructors.
            ನಿಲ್ಲಿಸು;
        }
    }

    // VISITORS
    using EmitCConstInit::ಭೇಟಿ;
    ಶೂನ್ಯ ಭೇಟಿ(AstCFunc* ಸ್ಥಿರಾಂಕ nodep) override {
        ಆದರೆ (nodep->ಖಾಲಿಗಳು_Body() && !nodep->isLoose()) ಹಿಂತಿರುಗಿ;
        VL_RESTORER(m_useSelfForThis);
        VL_RESTORER(m_cfuncp);
        VL_RESTORER(m_instantiatesOwnProcess);
        VL_RESTORER(m_createdScopeHash);
        m_cfuncp = nodep;
        m_instantiatesOwnProcess = false;
        m_labelNumbers.clear();  // No need to save/restore, all Jumps must be within the function

        ವಿಭಜಿಸು_ಗಾತ್ರ_Inc(nodep);

        puts("\n");
        m_lazyDecls.emit(nodep);
        ಆದರೆ (nodep->ifdef() != "") putns(nodep, "#ifdef " + nodep->ifdef() + "\n");
        emit_CFunc_ಶೀರ್ಷಿಕೆ(nodep, m_modp, /* withScope: */ true);

        puts(" {\n");
        ಆದರೆ (nodep->isConstructor()) {
            ಸ್ಥಿರಾಂಕ AstClass* ಸ್ಥಿರಾಂಕ classp = VN_CAST(nodep->scopep()->modp(), Class);
            ಆದರೆ (classp && classp->extendsp()) putConstructorSubinit(classp, nodep);
        }

        // "+" in the debug indicates a print from the model
        puts("VL_DEBUG_IF(VL_DBG_MSGF(\"+  ");
        ಗಾಗಿ (ಪೂರ್ಣಾಂಕ i = 0; i < m_modp->level(); ++i) puts("  ");
        puts(EmitCUtil::ಪೂರ್ವಪ್ರತ್ಯಯ_Name_Protect(m_modp));
        puts(nodep->isLoose() ? "__" : "::");
        puts(nodep->nameProtect() + "\\n\"); );\n");

        ಆದರೆ (nodep->isLoose()) {
            m_lazyDecls.declared(nodep);  // Defined here, so no longer needs declaration
            ಆದರೆ (!nodep->isStatic()) {  // Standard prologue
                m_useSelfForThis = true;
                ಆದರೆ (nodep->voidSelf()) {
                    ಸ್ಥಿರಾಂಕ string className = EmitCUtil::ಪೂರ್ವಪ್ರತ್ಯಯ_Name_Protect(m_modp);
                    puts(className + "* const vlSelf = static_cast<" + className
                         + "*>(vlVoidSelf);\n");
                }
                ಆದರೆ (!VN_IS(m_modp, Class)) {
                    puts(EmitCUtil::sym_Class_ನಿಯೋಜಿಸು());  // Uses vlSelf
                } ಇಲ್ಲದಿದ್ದರೆ {
                    puts("(void)vlSelf;  // Prevent unused variable warning\n");
                }
            }
        }

        // Instantiate a process class if it's going to be needed somewhere later
        nodep->forall([&](ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_CCall* ccallp) -> bool {
            ಆದರೆ (ccallp->funcp()->needProcess()
                && (ccallp->funcp()->isCoroutine() == VN_IS(ccallp->backp(), CAwait))) {
                ಆದರೆ (!nodep->needProcess() && !m_instantiatesOwnProcess) {
                    m_instantiatesOwnProcess = true;
                    ಹಿಂತಿರುಗಿ false;
                }
            }
            ಹಿಂತಿರುಗಿ true;
        });
        ಆದರೆ (m_instantiatesOwnProcess) {
            AstCStmt* ಸ್ಥಿರಾಂಕ vlprocp = new AstCStmt{nodep->fileline()};
            vlprocp->ಕೂಡಿಸು("VlProcessRef vlProcess = std::make_shared<VlProcess>();\n");
            vlprocp->ಕೂಡಿಸು("VlProcess::currentp(vlProcess.get());");
            nodep->prependStmtsp(vlprocp);
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (nodep->needProcess() && nodep->stmtsp()) {
            // Set current process so VlRNG() constructors in this function seed from it
            AstCStmt* ಸ್ಥಿರಾಂಕ ಗಣ_Processp = new AstCStmt{nodep->fileline()};
            ಗಣ_Processp->ಕೂಡಿಸು("VlProcess::currentp(vlProcess.get());");
            nodep->prependStmtsp(ಗಣ_Processp);
        }

        ಗಾಗಿ (Ast_ಗಂಟು* subnodep = nodep->argsp(); subnodep; subnodep = subnodep->nextp()) {
            ಆದರೆ (AstVar* ಸ್ಥಿರಾಂಕ varp = VN_CAST(subnodep, Var)) {
                ಆದರೆ (varp->isFuncReturn()) emitVarDecl(varp);
            }
        }

        ಆದರೆ (m_useSelfForThis) {
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

        ಆದರೆ (nodep->varsp()) {
            putsDecoration(nodep, "// Locals\n");
            iterate_And_ಮುಂದಿನ_Const_Null(nodep->varsp());
        }

        ಆದರೆ (nodep->stmtsp()) {
            putsDecoration(nodep, "// Body\n");
            iterate_And_ಮುಂದಿನ_Const_Null(nodep->stmtsp());
        }

        m_usevlSelfRef = false;

        ಆದರೆ (nodep->isCoroutine()) {
            // Sometimes coroutines don't have co_awaits,
            // so emit a co_return at the end to avoid compile errors.
            puts("co_return;\n");
        }

        puts("}\n");
        ಆದರೆ (nodep->ifdef() != "") puts("#endif  // " + nodep->ifdef() + "\n");
    }

    ಶೂನ್ಯ ಭೇಟಿ(AstVar* nodep) override {
        UASSERT_OBJ(m_cfuncp, nodep, "Cannot emit non-local variable");
        emitVarDecl(nodep);
    }

    ಶೂನ್ಯ ಭೇಟಿ(Ast_Cvt_Array_ಗಮ್ಯ_Array* nodep) override {
        ಆದರೆ (nodep->ಹಿಮ್ಮುಖ()) {
            puts("VL_REVCLONE_Q(");
        } ಇಲ್ಲದಿದ್ದರೆ {
            puts("VL_CLONE_Q(");
        }
        iterate_And_ಮುಂದಿನ_Const_Null(nodep->fromp());
        puts(", ");
        puts(cvt_ಗಮ್ಯ_Str(nodep->ಖಂಡ_ಗಾತ್ರ()));
        puts(", ");
        puts(cvt_ಗಮ್ಯ_Str(nodep->src_Element_ಬಿಟ್ಟುಗಳು()));
        puts(", ");
        puts(cvt_ಗಮ್ಯ_Str(nodep->dst_Element_ಬಿಟ್ಟುಗಳು()));
        puts(")");
    }

    ಶೂನ್ಯ ಭೇಟಿ(Ast_Cvt_Array_ಗಮ್ಯ_Packed* nodep) override {
        Ast_ಗಂಟು_DType* ಸ್ಥಿರಾಂಕ fromDtp = nodep->fromp()->dtypep()->skipRefp();
        Ast_ಗಂಟು_DType* ಸ್ಥಿರಾಂಕ elemDtp = fromDtp->subDTypep()->skipRefp();
        ಸ್ಥಿರಾಂಕ bool descending = [&]() {
            ಆದರೆ (ಸ್ಥಿರಾಂಕ AstUnpackArrayDType* ಸ್ಥಿರಾಂಕ unpackDtp = VN_CAST(fromDtp, UnpackArrayDType))
                ಹಿಂತಿರುಗಿ !unpackDtp->declRange().ascending();
            ಹಿಂತಿರುಗಿ false;
        }();
        puts("VL_PACK_");
        emitIQW(nodep);
        puts("_");
        emitRU(fromDtp);
        emitIQW(elemDtp);
        ಆದರೆ (descending) {
            // Wrap source in VL_PACK_REVERSED so VL_PACK sees ascending order
            emitOpName(nodep, "(%nw, %rw, %P, VL_PACK_REVERSED(%li))", nodep->fromp(), elemDtp,
                       nullptr);
        } ಇಲ್ಲದಿದ್ದರೆ {
            emitOpName(nodep, "(%nw, %rw, %P, %li)", nodep->fromp(), elemDtp, nullptr);
        }
    }

    ಶೂನ್ಯ ಭೇಟಿ(Ast_Cvt_Unpack_ಗಮ್ಯ_ಸರತಿ* nodep) override {
        Ast_ಗಂಟು_DType* ಸ್ಥಿರಾಂಕ elemDTypep = nodep->fromp()->dtypep()->subDTypep();
        emitOpName(nodep, nodep->emitC(), nodep->fromp(), elemDTypep, nullptr);
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_Cvt_Unpack_ಗಮ್ಯ_Unpack* nodep) override {
        // Whether each dimension is reversed, the first for the outermost, up to the last that is
        std::ಸದಿಶ<bool> reverses;
        ಸ್ಥಿರಾಂಕ AstUnpackArrayDType* ತುದಿ = VN_AS(nodep->dtypep()->skipRefp(), UnpackArrayDType);
        ಸ್ಥಿರಾಂಕ AstUnpackArrayDType* fromp
            = VN_AS(nodep->fromp()->dtypep()->skipRefp(), UnpackArrayDType);
        ಆಗ (ತುದಿ) {
            reverses.ತಳ್ಳು_ಹಿಂಭಾಗ(ತುದಿ->declRange().ascending() != fromp->declRange().ascending());
            ತುದಿ = VN_CAST(ತುದಿ->subDTypep()->skipRefp(), UnpackArrayDType);
            fromp = VN_CAST(fromp->subDTypep()->skipRefp(), UnpackArrayDType);
        }
        ಆಗ (!reverses.ಹಿಂಭಾಗ()) reverses.ತೆಗೆ_ಹಿಂಭಾಗ();
        putns(nodep, "VL_CVT_UNPACK_TO_UNPACK<");
        ಗಾಗಿ (ಗಾತ್ರ_t i = 0; i < reverses.ಗಾತ್ರ_2(); ++i) {
            ಆದರೆ (i) puts(", ");
            puts(reverses[i] ? "1" : "0");
        }
        puts(">(");
        iterate_And_ಮುಂದಿನ_Const_Null(nodep->fromp());
        puts(")");
    }

    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು_ನಿಯೋಜಿಸು* nodep) override {
        ಆದರೆ (AstCReset* ಸ್ಥಿರಾಂಕ resetp = VN_CAST(nodep->rhsp(), CReset)) {
            // TODO get rid of emitVarReset and instead let AstNodeAssign understand how to init
            // anything
            Ast_ಗಂಟು* fromp = nodep->lhsp();
            // Fork needs to use a member select.  Nothing else should be possible before VarRef.
            ಆದರೆ (AstMemberSel* ಸ್ಥಿರಾಂಕ sfromp = VN_CAST(fromp, MemberSel)) {
                // Fork-DynScope generated pointer to previously automatic variable
                AstVar* ಸ್ಥಿರಾಂಕ memberVarp = sfromp->varp();
                fromp = sfromp->fromp();
                ಆದರೆ (AstNullCheck* ಸ್ಥಿರಾಂಕ sfromp = VN_CAST(fromp, NullCheck)) fromp = sfromp->lhsp();
                Ast_ಗಂಟು_Var_Ref* ಸ್ಥಿರಾಂಕ fromVarRefp = VN_AS(fromp, ಗಂಟು_Var_Ref);
                emitVarReset(
                    ("VL_NULL_CHECK("s
                     + (fromVarRefp->self_ಸೂಚಕ().is_ಖಾಲಿಗಳು()
                            ? ""
                            : dereferenceString(fromVarRefp->self_ಸೂಚಕ_Protect(m_useSelfForThis)))
                     + varName(fromVarRefp->varp()) + ", \""
                     + V3_ಹೊರಹೋಗು_Formatter::quoteNameControls(protect(nodep->fileline()->filename()))
                     + "\", " + std::ಗಮ್ಯ_string(nodep->fileline()->lineno()) + ")->"),
                    memberVarp, resetp->constructing());
            } ಇಲ್ಲದಿದ್ದರೆ {
                Ast_ಗಂಟು_Var_Ref* ಸ್ಥಿರಾಂಕ fromVarRefp = VN_AS(fromp, ಗಂಟು_Var_Ref);
                AstVar* ಸ್ಥಿರಾಂಕ varp = fromVarRefp->varp();
                ಸ್ಥಿರಾಂಕ string ಪೂರ್ವಪ್ರತ್ಯಯ
                    = fromVarRefp->self_ಸೂಚಕ().is_ಖಾಲಿಗಳು()
                          ? ""
                          : dereferenceString(
                                VN_AS(fromp, ಗಂಟು_Var_Ref)->self_ಸೂಚಕ_Protect(m_useSelfForThis));
                emitVarReset(ಪೂರ್ವಪ್ರತ್ಯಯ, varp, resetp->constructing());
            }
            ಹಿಂತಿರುಗಿ;
        }
        bool paren = true;
        bool decind = false;
        bool rhs = true;
        bool ಹಿಮ್ಮುಖ_Unpack = false;  // Set for descending CvtPackedToArray
        ಸ್ಥಿರಾಂಕ AstUnpackArrayDType* ಸ್ಥಿರಾಂಕ unpackDtp
            = VN_CAST(nodep->dtypep()->skipRefp(), UnpackArrayDType);
        ಆದರೆ (AstSel* ಸ್ಥಿರಾಂಕ selp = VN_CAST(nodep->lhsp(), Sel)) {
            UASSERT_OBJ(selp->ಅಗಲ_Min() == selp->ಅಗಲ_Const(), selp, "Width mismatch");
            ಆದರೆ (selp->ಅಗಲ_Min() == 1) {
                putnbs(nodep, "VL_ASSIGNBIT_");
                emitIQW(selp->fromp());
                ಆದರೆ (nodep->rhsp()->isAllOnesV()) {
                    puts("O(");
                    rhs = false;
                } ಇಲ್ಲದಿದ್ದರೆ {
                    puts("I(");
                }
                iterate_And_ಮುಂದಿನ_Const_Null(selp->lsbp());
                puts(", ");
                iterate_And_ಮುಂದಿನ_Const_Null(selp->fromp());
                ಆದರೆ (rhs) puts(", ");
            } ಇಲ್ಲದಿದ್ದರೆ {
                putnbs(nodep, "VL_ASSIGNSEL_");
                emitIQW(selp->fromp());
                emitIQW(nodep->rhsp());
                puts("(");
                putns(selp->fromp(), cvt_ಗಮ್ಯ_Str(selp->fromp()->ಅಗಲ_Min()) + ", ");
                puts(cvt_ಗಮ್ಯ_Str(nodep->ಅಗಲ_Min()) + ", ");
                iterate_And_ಮುಂದಿನ_Const_Null(selp->lsbp());
                puts(", ");
                iterate_And_ಮುಂದಿನ_Const_Null(selp->fromp());
                puts(", ");
            }
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (ಸ್ಥಿರಾಂಕ AstGetcRefN* ಸ್ಥಿರಾಂಕ selp = VN_CAST(nodep->lhsp(), GetcRefN)) {
            iterate_And_ಮುಂದಿನ_Const_Null(selp->lhsp());
            puts(" = ");
            putnbs(selp, "VL_PUTC_N(");
            iterate_And_ಮುಂದಿನ_Const_Null(selp->lhsp());
            puts(", ");
            iterate_And_ಮುಂದಿನ_Const_Null(selp->rhsp());
            puts(", ");
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (AstVar* ಸ್ಥಿರಾಂಕ varp = AstVar::scVarRecurse(nodep->lhsp())) {
            putnbs(varp, "VL_ASSIGN_");  // Set a systemC variable
            emitScIQW(varp);
            emitIQW(nodep);
            puts("(");
            puts(cvt_ಗಮ್ಯ_Str(nodep->ಅಗಲ_Min()) + ", ");
            iterate_And_ಮುಂದಿನ_Const_Null(nodep->lhsp());
            puts(", ");
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (AstVar* ಸ್ಥಿರಾಂಕ varp = AstVar::scVarRecurse(nodep->rhsp())) {
            putnbs(varp, "VL_ASSIGN_");  // Get a systemC variable
            emitIQW(nodep);
            emitScIQW(varp);
            puts("(");
            puts(cvt_ಗಮ್ಯ_Str(nodep->ಅಗಲ_Min()) + ", ");
            iterate_And_ಮುಂದಿನ_Const_Null(nodep->lhsp());
            puts(", ");
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (ಸ್ಥಿರಾಂಕ Ast_Cvt_Packed_ಗಮ್ಯ_Array* ಸ್ಥಿರಾಂಕ castp
                   = VN_CAST(nodep->rhsp(), Cvt_Packed_ಗಮ್ಯ_Array)) {
            putns(castp, "VL_UNPACK_");
            emitRU(nodep);
            emitIQW(nodep->dtypep()->subDTypep());
            puts("_");
            emitRU(castp->fromp());
            emitIQW(castp->fromp());
            puts("(");
            putns(castp->dtypep(), cvt_ಗಮ್ಯ_Str(castp->dtypep()->subDTypep()->ಅಗಲ_Min()));
            puts(", ");
            puts(cvt_ಗಮ್ಯ_Str(castp->fromp()->ಅಗಲ_Min()));
            puts(", ");
            iterate_And_ಮುಂದಿನ_Const_Null(nodep->lhsp());
            puts(", ");
            rhs = false;
            iterate_And_ಮುಂದಿನ_Const_Null(castp->fromp());
            // Descending unpacked dest: reverse after unpack
            ಆದರೆ (unpackDtp && !unpackDtp->declRange().ascending()) ಹಿಮ್ಮುಖ_Unpack = true;
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (ಸ್ಥಿರಾಂಕ Ast_Cvt_Array_ಗಮ್ಯ_Array* ಸ್ಥಿರಾಂಕ castp
                   = VN_CAST(nodep->rhsp(), Cvt_Array_ಗಮ್ಯ_Array)) {
            ಆದರೆ (castp->ಹಿಮ್ಮುಖ()) {
                putns(castp, "VL_REVCOPY_Q(");
            } ಇಲ್ಲದಿದ್ದರೆ {
                putns(castp, "VL_COPY_Q(");
            }
            iterate_And_ಮುಂದಿನ_Const_Null(nodep->lhsp());
            puts(", ");
            rhs = false;
            iterate_And_ಮುಂದಿನ_Const_Null(castp->fromp());
            puts(", ");
            puts(cvt_ಗಮ್ಯ_Str(castp->ಖಂಡ_ಗಾತ್ರ()));
            puts(", ");
            puts(cvt_ಗಮ್ಯ_Str(castp->src_Element_ಬಿಟ್ಟುಗಳು()));
            puts(", ");
            puts(cvt_ಗಮ್ಯ_Str(castp->dst_Element_ಬಿಟ್ಟುಗಳು()));
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (nodep->isWide() && VN_IS(nodep->lhsp(), VarRef)  //
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
                   && !VN_IS(nodep->rhsp(), ಅಭಿವ್ಯಕ್ತಿ_Stmt)) {
            // Wide functions assign into the array directly, don't need separate assign statement
            m_wide_ತಾತ್ಕಾಲಿಕ_Refp = VN_AS(nodep->lhsp(), VarRef);
            paren = false;
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (nodep->isWide() && !unpackDtp && !VN_IS(nodep->rhsp(), Const)) {
            putnbs(nodep, "VL_ASSIGN_W(");
            puts(cvt_ಗಮ್ಯ_Str(nodep->ಅಗಲ_Min()) + ", ");
            iterate_And_ಮುಂದಿನ_Const_Null(nodep->lhsp());
            puts(", ");
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (VN_IS(nodep->lhsp()->dtypep()->skipRefp(), ಸರತಿ_DType)
                   && (VN_IS(nodep->rhsp(), ಹರಿವು_L) || VN_IS(nodep->lhsp(), ಹರಿವು_L)
                       || VN_IS(nodep->rhsp(), ಹರಿವು_R) || VN_IS(nodep->lhsp(), ಹರಿವು_R)
                       || VN_IS(nodep->rhsp(), ಹರಿವು_R))) {
            //if either side is streamL or streamR don't emit lhsp everything will be passed by
            //reference

            paren = false;

        } ಇಲ್ಲದಿದ್ದರೆ {
            paren = false;
            iterate_And_ಮುಂದಿನ_Const_Null(nodep->lhsp());
            puts(" ");
            ofp()->ಖಂಡ_Inc();
            decind = true;
            ಆದರೆ (!VN_IS(nodep->rhsp(), Const)) ofp()->putBreak();
            putns(nodep, "= ");
        }
        ಆದರೆ (rhs) iterate_And_ಮುಂದಿನ_Const_Null(nodep->rhsp());
        ಆದರೆ (paren) puts(")");
        ಆದರೆ (decind) ofp()->ಖಂಡ_Dec();
        puts(";\n");
        ಆದರೆ (ಹಿಮ್ಮುಖ_Unpack) {
            puts("VL_UNPACK_REVERSED(");
            iterate_And_ಮುಂದಿನ_Const_Null(nodep->lhsp());
            puts(");\n");
        }
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstAssocSel* nodep) override {
        iterate_And_ಮುಂದಿನ_Const_Null(nodep->fromp());
        ಸ್ಥಿರಾಂಕ std::string atFunc = nodep->isLValue() ? ".atWrite(" : ".at(";
        putnbs(nodep, atFunc);
        AstAssocArrayDType* ಸ್ಥಿರಾಂಕ adtypep
            = VN_AS(nodep->fromp()->dtypep()->skipRefp(), AssocArrayDType);
        UASSERT_OBJ(adtypep, nodep, "Associative select on non-associative type");
        iterate_And_ಮುಂದಿನ_Const_Null(nodep->bitp());
        puts(")");
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstWildcardSel* nodep) override {
        iterate_And_ಮುಂದಿನ_Const_Null(nodep->fromp());
        ಸ್ಥಿರಾಂಕ std::string atFunc = nodep->isLValue() ? ".atWrite(" : ".at(";
        putnbs(nodep, atFunc);
        AstWildcardArrayDType* ಸ್ಥಿರಾಂಕ adtypep
            = VN_AS(nodep->fromp()->dtypep()->skipRefp(), WildcardArrayDType);
        UASSERT_OBJ(adtypep, nodep, "Wildcard select on non-wildcard-associative type");
        iterate_And_ಮುಂದಿನ_Const_Null(nodep->bitp());
        puts(")");
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstCCall* nodep) override {
        ಸ್ಥಿರಾಂಕ AstCFunc* ಸ್ಥಿರಾಂಕ funcp = nodep->funcp();
        ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_Module* ಸ್ಥಿರಾಂಕ funcModp = EmitCParentModule::get(funcp);
        putnbs(nodep, "");
        ಆದರೆ (funcp->dpiImportPrototype()) {
            // Calling DPI import
            putns(nodep, funcp->name());
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (funcp->is_Proper_ವಿಧಾನ() && funcp->isStatic()) {
            // Call static method via the containing class
            putns(funcModp, EmitCUtil::ಪೂರ್ವಪ್ರತ್ಯಯ_Name_Protect(funcModp) + "::");
            putns(nodep, funcp->nameProtect());
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (nodep->superReference()) {
            // Calling superclass method
            putns(funcModp, EmitCUtil::ಪೂರ್ವಪ್ರತ್ಯಯ_Name_Protect(funcModp) + "::");
            putns(nodep, funcp->nameProtect());
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (funcp->isLoose()) {
            // Calling loose method
            putns(nodep, funcNameProtect(funcp));
        } ಇಲ್ಲದಿದ್ದರೆ {
            // Calling regular method/function
            ಆದರೆ (!nodep->self_ಸೂಚಕ().is_ಖಾಲಿಗಳು()) {
                emitDereference(nodep, nodep->self_ಸೂಚಕ_Protect(m_useSelfForThis));
            }
            putns(nodep, funcp->nameProtect());
        }
        emit_CCall_ವಾದಗಳು(nodep, nodep->self_ಸೂಚಕ_Protect(m_useSelfForThis), m_cfuncp->needProcess());
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstCMethodCall* nodep) override {
        ಸ್ಥಿರಾಂಕ AstCFunc* ಸ್ಥಿರಾಂಕ funcp = nodep->funcp();
        UASSERT_OBJ(!funcp->isLoose(), nodep, "Loose method called via AstCMethodCall");
        iterateConst(nodep->fromp());
        putnbs(nodep, "->");
        ಆದರೆ (nodep->superReference()) {
            // Super call, so the base implementation is used rather than the override
            ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_Module* ಸ್ಥಿರಾಂಕ funcModp = EmitCParentModule::get(funcp);
            putns(funcModp, EmitCUtil::ಪೂರ್ವಪ್ರತ್ಯಯ_Name_Protect(funcModp) + "::");
        }
        putns(funcp, funcp->nameProtect());
        emit_CCall_ವಾದಗಳು(nodep, "", m_cfuncp->needProcess());
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstCAwait* nodep) override {
        putns(nodep, "co_await ");
        iterateConst(nodep->exprp());
        puts(";\n");
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstCNew* nodep) override {
        ಆದರೆ (VN_IS(nodep->dtypep(), VoidDType)) {  // super.new case
            putsDecoration(nodep, "/*super.new*/");
            ಹಿಂತಿರುಗಿ;
        }
        // assignment case;
        putns(nodep, "VL_NEW(" + EmitCUtil::ಪೂರ್ವಪ್ರತ್ಯಯ_Name_Protect(nodep->dtypep()) + ", "
                         + optionalProcArg(nodep->dtypep()) + "vlSymsp");
        put_Comma_Iterate_ಮುಂದಿನ(nodep->argsp(), true);
        puts(")");
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstCMethodHard* nodep) override {
        iterateConst(nodep->fromp());
        putns(nodep, nodep->usePtr() ? "->" : ".");
        putns(nodep, nodep->name());
        ಆದರೆ (nodep->ವಿಧಾನ() == VCMethod::FORCE_READ_SEL) {
            emitIQW(nodep);
            ಆದರೆ (nodep->isWide()) puts("<" + cvt_ಗಮ್ಯ_Str(nodep->dtypep()->ಅಗಲ_ಪದಗಳು()) + ">");
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (nodep->ವಿಧಾನ() == VCMethod::ARRAY_ತುಂಡು) {
            // VlUnpacked::slice<N_Out>(loIdx) - N_Out is the (fixed) result array size
            ಸ್ಥಿರಾಂಕ AstUnpackArrayDType* ಸ್ಥಿರಾಂಕ adtypep
                = VN_AS(nodep->dtypep()->skipRefp(), UnpackArrayDType);
            puts("<" + cvt_ಗಮ್ಯ_Str(adtypep->elementsConst()) + ">");
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (nodep->ವಿಧಾನ() == VCMethod::COVERGROUP_ಕೂಡಿಸು_COVERPOINT) {
            // The hit-list bound is a template argument of the returned VlCoverpointT<>, and the
            // node's own dtype is that type, so it is the one source of truth for both.
            ಸ್ಥಿರಾಂಕ AstCoverpointDType* ಸ್ಥಿರಾಂಕ cpdtypep
                = VN_AS(nodep->dtypep()->skipRefp(), CoverpointDType);
            puts("<" + cvt_ಗಮ್ಯ_Str(cpdtypep->ದೊರಕಿತು_Bound()) + ">");
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (nodep->ವಿಧಾನ() == VCMethod::COVERGROUP_ಕೂಡಿಸು_CROSS) {
            ಸ್ಥಿರಾಂಕ AstCoverCrossDType* ಸ್ಥಿರಾಂಕ cxdtypep
                = VN_AS(nodep->dtypep()->skipRefp(), CoverCrossDType);
            puts("<" + cxdtypep->cpp_Template_ವಾದಗಳು() + ">");
        }
        puts("(");
        bool comma = false;
        ಪೂರ್ಣಾಂಕ arg_ಅಂಕೆ = 0;
        ಗಾಗಿ (Ast_ಗಂಟು* subnodep = nodep->pinsp(); subnodep; subnodep = subnodep->nextp()) {
            ಆದರೆ (comma) puts(", ");
            ಆದರೆ (nodep->ವಿಧಾನ() == VCMethod::RANDOMIZER_HARD && arg_ಅಂಕೆ == 1) {
                // For RANDOMIZER_HARD's filename argument (2nd arg after constraint),
                // apply protect() similar to VL_STOP to handle --protected flag
                ಆದರೆ (ಸ್ಥಿರಾಂಕ AstCExpr* ಸ್ಥಿರಾಂಕ cexprp = VN_CAST(subnodep, CExpr)) {
                    // Extract filename from the CExpr (which contains "filename")
                    std::string filename;
                    ಗಾಗಿ (ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* textnodep = cexprp->nodesp(); textnodep;
                         textnodep = textnodep->nextp()) {
                        ಆದರೆ (ಸ್ಥಿರಾಂಕ Ast_ಪಠ್ಯ* ಸ್ಥಿರಾಂಕ textp = VN_CAST(textnodep, ಪಠ್ಯ_2)) {
                            filename = textp->ಪಠ್ಯ();
                            ನಿಲ್ಲಿಸು;
                        }
                    }
                    // Remove surrounding quotes if present
                    ಆದರೆ (filename.ಗಾತ್ರ_2() >= 2 && filename.front() == '"'
                        && filename.ಹಿಂಭಾಗ() == '"') {
                        filename = filename.substr(1, filename.ಗಾತ್ರ_2() - 2);
                    }
                    // Emit with protect()
                    putsQuoted(protect(filename));
                } ಇಲ್ಲದಿದ್ದರೆ {
                    iterateConst(subnodep);
                }
            } ಇಲ್ಲದಿದ್ದರೆ {
                iterateConst(subnodep);
            }
            comma = true;
            arg_ಅಂಕೆ++;
        }
        ಆದರೆ (nodep->withp()) {
            ಆದರೆ (comma) puts(", ");
            iterateConst(nodep->withp());
        }
        puts(")");
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstCFuncHard* nodep) override {
        putns(nodep, nodep->name());
        puts("(");
        put_Comma_Iterate_ಮುಂದಿನ(nodep->pinsp());
        puts(")");
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstLambdaArgRef* nodep) override { putbs(nodep->nameProtect()); }
    ಶೂನ್ಯ ಭೇಟಿ(AstWith* nodep) override {
        // With uses a C++11 lambda
        putnbs(nodep, "[&](");
        ಆದರೆ (ಸ್ವಯಂಚಾಲಿತ* ಸ್ಥಿರಾಂಕ argrefp = nodep->ಸೂಚ್ಯಂಕ_Arg_Refp()) {
            putnbs(argrefp, argrefp->dtypep()->cType(argrefp->nameProtect(), false, false));
            puts(", ");
        }
        ಆದರೆ (ಸ್ವಯಂಚಾಲಿತ* ಸ್ಥಿರಾಂಕ argrefp = nodep->ಮೌಲ್ಯ_Arg_Refp()) {
            putnbs(argrefp, argrefp->dtypep()->cType(argrefp->nameProtect(), false, false));
        }
        puts(") -> ");
        putnbs(nodep, nodep->dtypep()->cType("", false, false));
        puts(" {\n");
        VL_RESTORER(m_createdScopeHash);
        iterate_And_ಮುಂದಿನ_Const_Null(nodep->exprp());
        puts("}\n");
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstCase* nodep) override {  // LCOV_EXCL_LINE
        // In V3Case...
        nodep->v3fatalSrc("Case statements should have been reduced out");
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstComment* nodep) override {
        string at;
        ಆದರೆ (nodep->showAt()) {
            at = " at " + nodep->fileline()->ascii();
            // If protecting, passthru less information about the design
            ಆದರೆ (!v3Global.opt.protectIds()) ಹಿಂತಿರುಗಿ;
        }
        ಆದರೆ (!(nodep->protect() && v3Global.opt.protectIds())) {
            putsDecoration(nodep, "// "s + nodep->name() + at + "\n");
        }
        iterateChildrenConst(nodep);
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstCoverOtherDecl* nodep) override {
        emit_Cover_Other_Decl_ಸೇರಿಸು(nodep);
        ಆದರೆ (coverage_ಬಳಸುತ್ತದೆ_Local_Counter(nodep)) {
            // Some task bodies are cloned into a caller module where the
            // original instance-local counter is not addressable. Those clones
            // fall back to the global bin number, so register that slot too.
            emit_Cover_Other_Decl_ಸೇರಿಸು(nodep, true);
        }
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstCoverToggleDecl* nodep) override {
        putns(nodep, "vlSelf->__vlCoverToggleInsert(");  // As Declared in emitCoverageDecl
        puts(cvt_ಗಮ್ಯ_Str(nodep->range().ಬಲ_2()));
        puts(", ");
        puts(cvt_ಗಮ್ಯ_Str(nodep->range().ಎಡ_2()));
        puts(", ");
        puts(cvt_ಗಮ್ಯ_Str(nodep->range().ranged()));
        puts(", ");
        putCoverageArray(nodep);
        puts(" + ");
        puts(cvt_ಗಮ್ಯ_Str(coverage_Bin_ಅಂಕೆ(nodep)));
        // first controls whether duplicate module instances are collapsed in
        // the default coverage view. The following flag says whether countp
        // points to an object-local counter; only those counters must be kept
        // real for later instances so forcePerInstance can split hierarchy.
        puts(", first");  // Enable, passed from __Vconfigure parameter
        puts(", ");
        puts(coverage_ಬಳಸುತ್ತದೆ_Local_Counter(nodep) ? "true" : "false");
        puts(", ");
        putsQuoted(protect(nodep->fileline()->filename()));
        puts(", ");
        puts(cvt_ಗಮ್ಯ_Str(nodep->fileline()->lineno()));
        puts(", ");
        puts(cvt_ಗಮ್ಯ_Str(nodep->fileline()->ಮೊದಲ_ಸ್ತಂಭ()));
        puts(", ");
        putsQuoted((!nodep->hier().ಖಾಲಿಗಳು() ? "." : "")
                   + VIdProtect::protect_ಪದಗಳು_If(nodep->hier(), nodep->protect()));
        puts(", ");
        putsQuoted(VIdProtect::protect_ಪದಗಳು_If(nodep->ಪುಟ(), nodep->protect()));
        puts(", ");
        putsQuoted(VIdProtect::protect_ಪದಗಳು_If(nodep->comment(), nodep->protect()));
        puts(");\n");
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstCoverInc* nodep) override {
        ಆದರೆ (VN_IS(nodep->declp(), CoverOtherDecl)) {
            ಆದರೆ (v3Global.opt.ಎಳೆಗಳು() > 1) {
                putns(nodep, "");
                putCoverageArray(nodep->declp());
                puts("[");
                puts(cvt_ಗಮ್ಯ_Str(coverage_Bin_ಅಂಕೆ(nodep->declp())));
                puts("].fetch_add(1, std::memory_order_relaxed);\n");
            } ಇಲ್ಲದಿದ್ದರೆ {
                putns(nodep, "++(");
                putCoverageArray(nodep->declp());
                puts("[");
                puts(cvt_ಗಮ್ಯ_Str(coverage_Bin_ಅಂಕೆ(nodep->declp())));
                puts("]);\n");
            }
        } ಇಲ್ಲದಿದ್ದರೆ {
            puts("VL_COV_TOGGLE_CHG_");
            ಆದರೆ (v3Global.opt.ಎಳೆಗಳು() > 1) {
                puts("MT_");
            } ಇಲ್ಲದಿದ್ದರೆ {
                puts("ST_");
            }
            emitIQW(nodep->toggleExprp());
            puts("(");
            // Each bit of variable has 2 counters, so the size of variable is half of the size of
            // coverpoint
            puts(cvt_ಗಮ್ಯ_Str(nodep->declp()->ಗಾತ್ರ_2() / 2));
            puts(", ");
            // Toggle update uses the same object-local counter array that
            // __vlCoverToggleInsert registered.
            putCoverageArray(nodep->declp());
            puts(" + ");
            puts(cvt_ಗಮ್ಯ_Str(coverage_Bin_ಅಂಕೆ(nodep->declp())));
            puts(", ");
            iterateConst(nodep->toggleExprp());
            puts(", ");
            iterateConst(nodep->toggleCovExprp());
            puts(");\n");
        }
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstDisableFork* nodep) override { putns(nodep, "vlProcess->disableFork();\n"); }
    ಶೂನ್ಯ ಭೇಟಿ(AstCReturn* nodep) override {
        putns(nodep, "return (");
        iterate_And_ಮುಂದಿನ_Const_Null(nodep->lhsp());
        puts(");\n");
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstDisplay* nodep) override {
        string ಪಠ್ಯ = nodep->fmtp()->ಪಠ್ಯ();
        ಆದರೆ (nodep->ಕೂಡಿಸು_Newline()) ಪಠ್ಯ += "\n";
        display_ಗಂಟು(nodep, nodep->fmtp(), ಪಠ್ಯ, nodep->fmtp()->exprsp(), false);
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstDumpCtl* nodep) override {
        ಆಯ್ಕೆ (nodep->ctlType()) {
        ಸ್ಥಿತಿ VDumpCtlType::FILE:
            putns(nodep, "vlSymsp->_vm_contextp__->dumpfile(");
            iterateConst(nodep->exprp());
            puts(");\n");
            ನಿಲ್ಲಿಸು;
        ಸ್ಥಿತಿ VDumpCtlType::VARS:
            // We ignore number of levels to dump in exprp()
            ಆದರೆ (v3Global.opt.trace()) {
                putns(nodep, "vlSymsp->_traceDumpOpen();\n");
            } ಇಲ್ಲದಿದ್ದರೆ {
                putns(nodep, "VL_PRINTF_MT(\"-Info: ");
                puts(V3_ಹೊರಹೋಗು_Formatter::quoteNameControls(protect(nodep->fileline()->filename())));
                puts(":");
                puts(cvt_ಗಮ್ಯ_Str(nodep->fileline()->lineno()));
                puts(": $dumpvar ignored, as Verilated without --trace");
                puts("\\n\");\n");
            }
            ನಿಲ್ಲಿಸು;
        ಸ್ಥಿತಿ VDumpCtlType::ALL:
            // $dumpall currently ignored
            ನಿಲ್ಲಿಸು;
        ಸ್ಥಿತಿ VDumpCtlType::FLUSH:
            // $dumpall currently ignored; would need rework of VCD single thread,
            // or flag we pass-through to next eval() iteration
            ನಿಲ್ಲಿಸು;
        ಸ್ಥಿತಿ VDumpCtlType::ಎಲ್ಲೆ_2:
            // $dumplimit currently ignored
            ನಿಲ್ಲಿಸು;
        ಸ್ಥಿತಿ VDumpCtlType::OFF:
            // Currently ignored as both Vcd and Fst do not support them, as would need "X" dump
            ನಿಲ್ಲಿಸು;
        ಸ್ಥಿತಿ VDumpCtlType::ON:
            // Currently ignored as $dumpoff is also ignored
            ನಿಲ್ಲಿಸು;
        ಪೂರ್ವನಿಯೋಜಿತ: nodep->v3fatalSrc("Bad case, unexpected " << nodep->ctlType().ascii());
        }
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstScopeName* nodep) override {
        // For use under AstCCalls for dpiImports.  ScopeNames under
        // displays are handled in AstDisplay
        ಆದರೆ (!nodep->dpiExport()) {
            // this is where the DPI import context scope is set
            ಸ್ಥಿರಾಂಕ string scope = nodep->scopeDpiName();
            putnbs(nodep, "(vlSymsp->" + protect("__Vscopep_" + scope) + ")");
        }
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstSFormat* nodep) override {
        display_ಗಂಟು(nodep, nodep->fmtp(), nodep->fmtp()->ಪಠ್ಯ(), nodep->fmtp()->exprsp(), false);
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstSFormatF* nodep) override {
        display_ಗಂಟು(nodep, nodep, nodep->ಪಠ್ಯ(), nodep->exprsp(), false);
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstFScanF* nodep) override {
        display_ಗಂಟು(nodep, nullptr, nodep->ಪಠ್ಯ(), nodep->exprsp(), true);
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstSScanF* nodep) override {
        display_ಗಂಟು(nodep, nullptr, nodep->ಪಠ್ಯ(), nodep->exprsp(), true);
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಮೌಲ್ಯ_Plus_ವಾದಗಳು* nodep) override {
        putns(nodep, "VL_VALUEPLUSARGS_IN");
        emitIQW(nodep->outp());
        puts("(");
        puts(cvt_ಗಮ್ಯ_Str(nodep->outp()->ಅಗಲ_Min()));
        puts(", ");
        iterateConst(nodep->searchp());
        puts(", ");
        putbs("");
        iterate_And_ಮುಂದಿನ_Const_Null(nodep->outp());
        puts(")");
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_Test_Plus_ವಾದಗಳು* nodep) override {
        putns(nodep, "VL_TESTPLUSARGS_I(");
        iterateConst(nodep->searchp());
        puts(")");
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstFError* nodep) override {
        putns(nodep, "VL_FERROR_I");
        puts(nodep->strp()->isString() ? "N(" : "W(");
        iterate_And_ಮುಂದಿನ_Const_Null(nodep->filep());
        putbs(", ");
        ಆದರೆ (nodep->strp()->isWide()) {
            puts(cvt_ಗಮ್ಯ_Str(nodep->strp()->ಅಗಲ_ಪದಗಳು()));
            putbs(", ");
        }
        iterate_And_ಮುಂದಿನ_Const_Null(nodep->strp());
        puts(")");
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstFGetS* nodep) override {
        emitOpName(nodep, nodep->emitC(), nodep->strgp(), nodep->filep(), nullptr);
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstFOpen* nodep) override {
        putns(nodep, "VL_FOPEN_NN(");
        iterateConst(nodep->filenamep());
        putbs(", ");
        iterateConst(nodep->modep());
        puts(");\n");
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstFOpenMcd* nodep) override {
        putns(nodep, "VL_FOPEN_MCD_N(");
        iterateConst(nodep->filenamep());
        puts(");\n");
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು_Read_Write_Mem* nodep) override {
        putns(nodep, nodep->cFuncPrefixp());
        puts("N(");
        puts(nodep->isHex() ? "true" : "false");
        putbs(", ");
        // Need real storage width
        puts(cvt_ಗಮ್ಯ_Str(nodep->memp()->dtypep()->skipRefp()->subDTypep()->ಅಗಲ_Min()));
        uint32_t array_lo = 0;
        bool memUsePtr = true;
        {
            ಸ್ಥಿರಾಂಕ AstVarRef* ಸ್ಥಿರಾಂಕ varrefp = VN_CAST(nodep->memp(), VarRef);
            ಆದರೆ (!varrefp) {
                nodep->v3error(nodep->verilogKwd() << " loading non-variable");
            } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (VN_IS(varrefp->varp()->dtypeSkipRefp(), AssocArrayDType)
                       || VN_IS(varrefp->varp()->dtypeSkipRefp(), DynArrayDType)) {
                // nodep->memp() below will when verilated code is compiled create a C++ template
                memUsePtr = false;
            } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (ಸ್ಥಿರಾಂಕ AstUnpackArrayDType* ಸ್ಥಿರಾಂಕ adtypep
                       = VN_CAST(varrefp->varp()->dtypeSkipRefp(), UnpackArrayDType)) {
                putbs(", ");
                puts(cvt_ಗಮ್ಯ_Str(varrefp->varp()->dtypep()->arrayUnpackedElements()));
                array_lo = adtypep->lo();
                putbs(", ");
                puts(cvt_ಗಮ್ಯ_Str(array_lo));
            } ಇಲ್ಲದಿದ್ದರೆ {
                nodep->v3error(
                    nodep->verilogKwd()
                    << " loading other than unpacked/dynamic/associative-array variable");
            }
        }
        putbs(", ");
        iterateConst(nodep->filenamep());
        putbs(", ");
        {
            ಆದರೆ (memUsePtr) puts(" &(");
            iterate_And_ಮುಂದಿನ_Const_Null(nodep->memp());
            ಆದರೆ (memUsePtr) puts(")");
        }
        putbs(", ");
        ಆದರೆ (nodep->lsbp()) {
            iterate_And_ಮುಂದಿನ_Const_Null(nodep->lsbp());
        } ಇಲ್ಲದಿದ್ದರೆ {
            puts(cvt_ಗಮ್ಯ_Str(array_lo));
        }
        putbs(", ");
        ಆದರೆ (nodep->msbp()) {
            iterate_And_ಮುಂದಿನ_Const_Null(nodep->msbp());
        } ಇಲ್ಲದಿದ್ದರೆ {
            puts("~0ULL");
        }
        puts(");\n");
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstFClose* nodep) override {
        putns(nodep, "VL_FCLOSE_I(");
        iterate_And_ಮುಂದಿನ_Const_Null(nodep->filep());
        puts("); ");
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstFFlush* nodep) override {
        ಆದರೆ (!nodep->filep()) {
            putns(nodep, "VL_FFLUSH_MT();");
        } ಇಲ್ಲದಿದ್ದರೆ {
            putns(nodep, "VL_FFLUSH_I(");
            iterate_And_ಮುಂದಿನ_Const_Null(nodep->filep());
            puts(");\n");
        }
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstFSeek* nodep) override {
        putns(nodep, "(VL_FSEEK_I(");
        iterate_And_ಮುಂದಿನ_Const_Null(nodep->filep());
        puts(", ");
        iterate_And_ಮುಂದಿನ_Const_Null(nodep->ಸ್ಥಾನಾಂತರ());
        puts(", ");
        iterate_And_ಮುಂದಿನ_Const_Null(nodep->operation());
        puts(") == -1 ? -1 : 0)");
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstFTell* nodep) override {
        putns(nodep, "VL_FTELL_I(");
        iterate_And_ಮುಂದಿನ_Const_Null(nodep->filep());
        puts(")");
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstFRewind* nodep) override {
        putns(nodep, "(VL_FSEEK_I(");
        iterate_And_ಮುಂದಿನ_Const_Null(nodep->filep());
        puts(", 0, 0) == -1 ? -1 : 0)");
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstFRead* nodep) override {
        putns(nodep, "VL_FREAD_I(");
        puts(cvt_ಗಮ್ಯ_Str(nodep->memp()->ಅಗಲ_Min()));  // Need real storage width
        putbs(", ");
        uint32_t array_lo = 0;
        uint32_t array_ಗಾತ್ರ = 0;
        {
            ಸ್ಥಿರಾಂಕ AstVarRef* ಸ್ಥಿರಾಂಕ varrefp = VN_CAST(nodep->memp(), VarRef);
            ಆದರೆ (!varrefp) {
                nodep->v3error(nodep->verilogKwd() << " loading non-variable");
            } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (VN_CAST(varrefp->varp()->dtypeSkipRefp(), BasicDType)) {
            } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (ಸ್ಥಿರಾಂಕ AstUnpackArrayDType* ಸ್ಥಿರಾಂಕ adtypep
                       = VN_CAST(varrefp->varp()->dtypeSkipRefp(), UnpackArrayDType)) {
                array_lo = adtypep->lo();
                array_ಗಾತ್ರ = adtypep->elementsConst();
            } ಇಲ್ಲದಿದ್ದರೆ {
                nodep->v3error(nodep->verilogKwd()
                               << " loading other than unpacked-array variable");
            }
        }
        puts(cvt_ಗಮ್ಯ_Str(array_lo));
        putbs(", ");
        puts(cvt_ಗಮ್ಯ_Str(array_ಗಾತ್ರ));
        putbs(", ");
        puts("&(");
        iterate_And_ಮುಂದಿನ_Const_Null(nodep->memp());
        puts(")");
        putbs(", ");
        iterate_And_ಮುಂದಿನ_Const_Null(nodep->filep());
        putbs(", ");
        ಆದರೆ (nodep->startp()) {
            iterate_And_ಮುಂದಿನ_Const_Null(nodep->startp());
        } ಇಲ್ಲದಿದ್ದರೆ {
            puts(cvt_ಗಮ್ಯ_Str(array_lo));
        }
        putbs(", ");
        ಆದರೆ (nodep->countp()) {
            iterate_And_ಮುಂದಿನ_Const_Null(nodep->countp());
        } ಇಲ್ಲದಿದ್ದರೆ {
            puts(cvt_ಗಮ್ಯ_Str(array_ಗಾತ್ರ));
        }
        puts(")");
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ರಾಶಿ_Trace_F* nodep) override { putns(nodep, "VL_STACKTRACE_N()"); }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ರಾಶಿ_Trace_T* nodep) override { putns(nodep, "VL_STACKTRACE();\n"); }
    ಶೂನ್ಯ ಭೇಟಿ(AstSystemT* nodep) override {
        putns(nodep, "(void)VL_SYSTEM_I");
        emitIQW(nodep->lhsp());
        puts("(");
        ಆದರೆ (nodep->lhsp()->isWide()) {
            puts(cvt_ಗಮ್ಯ_Str(nodep->lhsp()->ಅಗಲ_ಪದಗಳು()));
            putbs(", ");
        }
        iterate_And_ಮುಂದಿನ_Const_Null(nodep->lhsp());
        puts(");\n");
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstSystemF* nodep) override {
        putns(nodep, "VL_SYSTEM_I");
        emitIQW(nodep->lhsp());
        puts("(");
        ಆದರೆ (nodep->lhsp()->isWide()) {
            puts(cvt_ಗಮ್ಯ_Str(nodep->lhsp()->ಅಗಲ_ಪದಗಳು()));
            putbs(", ");
        }
        iterate_And_ಮುಂದಿನ_Const_Null(nodep->lhsp());
        puts(")");
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_Stmt_ಅಭಿವ್ಯಕ್ತಿ* nodep) override {
        ಆದರೆ (VN_IS(nodep->exprp()->dtypep()->skipRefp(), VoidDType)) {
            putns(nodep, "");
        } ಇಲ್ಲದಿದ್ದರೆ {
            putns(nodep, "std::ignore = ");
        }
        iterateConst(nodep->exprp());
        puts(";\n");
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_Jump_ಖಂಡ* nodep) override {
        // Allocate label number
        ಸ್ಥಿರಾಂಕ ಗಾತ್ರ_t n = m_labelNumbers.ಗಾತ್ರ_2();
        ಸ್ಥಿರಾಂಕ bool new_ನಮೂದು = m_labelNumbers.emplace(nodep, n).ಎರಡನೇ;
        UASSERT_OBJ(new_ನಮೂದು, nodep, "AstJumpBlock visited twide");
        // Emit
        putns(nodep, "{\n");  // Make it visually obvious label jumps outside these
        VL_RESTORER(m_createdScopeHash);
        iterate_And_ಮುಂದಿನ_Const_Null(nodep->stmtsp());
        puts("__Vlabel" + std::ಗಮ್ಯ_string(n) + ": ;\n");
        puts("}\n");
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstJumpGo* nodep) override {
        // Retrieve target label number - must already exist (from enclosing AstJumpBlock)
        ಸ್ಥಿರಾಂಕ ಗಾತ್ರ_t n = m_labelNumbers.at(nodep->blockp());
        // Emit
        putns(nodep, "goto __Vlabel" + std::ಗಮ್ಯ_string(n) + ";\n");
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstLoop* nodep) override {
        UASSERT_OBJ(!nodep->contsp(), nodep, "'contsp' only used before LinkJump");
        VL_RESTORER(m_createdScopeHash);
        // Special case when the AstLoopTest is first for output readability
        ಆದರೆ (AstLoopTest* ಸ್ಥಿರಾಂಕ testp = VN_CAST(nodep->stmtsp(), LoopTest)) {
            putns(nodep, "while (");
            iterateConst(testp->condp());
            puts(") {\n");
            iterate_And_ಮುಂದಿನ_Const_Null(testp->nextp());
            puts("}\n");
            ಹಿಂತಿರುಗಿ;
        }
        // Special case when the AstLoopTest is last for output readability
        ಆದರೆ (Ast_ಗಂಟು* lastp = nodep->stmtsp()) {
            ಆಗ (Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ nextp = lastp->nextp()) lastp = nextp;
            ಆದರೆ (AstLoopTest* ಸ್ಥಿರಾಂಕ testp = VN_CAST(lastp, LoopTest)) {
                putns(nodep, "do {\n");
                ಗಾಗಿ (Ast_ಗಂಟು* p = nodep->stmtsp(); p != lastp; p = p->nextp()) iterateConst(p);
                puts("} while (");
                iterateConst(testp->condp());
                puts(");\n");
                ಹಿಂತಿರುಗಿ;
            }
        }
        // Emit generic case directly
        putns(nodep, "while (true) {\n");
        iterate_And_ಮುಂದಿನ_Const_Null(nodep->stmtsp());
        puts("}\n");
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstLoopTest* nodep) override {
        VL_RESTORER(m_createdScopeHash);
        putns(nodep, "if (!(");
        iterate_And_ಮುಂದಿನ_Const_Null(nodep->condp());
        puts(")) break;\n");
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstCLocalScope* nodep) override {
        putns(nodep, "{\n");
        VL_RESTORER(m_createdScopeHash);
        iterate_And_ಮುಂದಿನ_Const_Null(nodep->stmtsp());
        puts("}\n");
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು_If* nodep) override {
        putns(nodep, "if (");
        ಆದರೆ (!nodep->branchPred().unknown()) {
            puts(nodep->branchPred().ascii());
            puts("((");  // Two parens, so that VL_UNLIKELY((class<foo,bar>)) works
        }
        iterate_And_ಮುಂದಿನ_Const_Null(nodep->condp());
        ಆದರೆ (!nodep->branchPred().unknown()) puts("))");
        puts(") {\n");
        {
            VL_RESTORER(m_createdScopeHash);
            iterate_And_ಮುಂದಿನ_Const_Null(nodep->thensp());
        }
        puts("}");
        ಆದರೆ (!nodep->elsesp()) {
            puts("\n");
        } ಇಲ್ಲದಿದ್ದರೆ {
            ಆದರೆ (VN_IS(nodep->elsesp(), ಗಂಟು_If) && !nodep->elsesp()->nextp()) {
                puts(" else ");
                iterate_And_ಮುಂದಿನ_Const_Null(nodep->elsesp());
            } ಇಲ್ಲದಿದ್ದರೆ {
                VL_RESTORER(m_createdScopeHash);
                puts(" else {\n");
                iterate_And_ಮುಂದಿನ_Const_Null(nodep->elsesp());
                puts("}\n");
            }
        }
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಅಭಿವ್ಯಕ್ತಿ_Stmt* nodep) override {
        VL_RESTORER(m_createdScopeHash);
        ಸ್ಥಿರಾಂಕ bool ಒಳಗೊಂಡಿದೆ_Await = nodep->exists([](AstCAwait*) -> bool { ಹಿಂತಿರುಗಿ true; });
        ಆದರೆ (ಒಳಗೊಂಡಿದೆ_Await) {
            UASSERT_OBJ(m_cfuncp && m_cfuncp->isCoroutine(), nodep,
                        "AstExprStmt with CAwait must be in coroutine");
            putnbs(nodep, "(co_await ([&]() -> VlCoroutine {\n");
            iterate_And_ಮುಂದಿನ_Const_Null(nodep->stmtsp());
            puts("co_return;\n");
            ಆದರೆ (!nodep->has_ಫಲಿತಾಂಶ()) {
                puts("}()))");
                ಹಿಂತಿರುಗಿ;
            }
            puts("}()), ");
            iterate_And_ಮುಂದಿನ_Const_Null(nodep->resultp());
            puts(")");
            ಹಿಂತಿರುಗಿ;
        }
        // GCC allows compound statements in expressions, but this is not standard.
        // So we use an immediate-evaluation lambda and comma operator
        putnbs(nodep, "([&]() {\n");
        ಆದರೆ (!nodep->has_ಫಲಿತಾಂಶ()) {
            iterate_And_ಮುಂದಿನ_Const_Null(nodep->stmtsp());
            puts("}())");
            ಹಿಂತಿರುಗಿ;
        }
        iterate_And_ಮುಂದಿನ_Const_Null(nodep->stmtsp());
        puts("}(), ");
        iterate_And_ಮುಂದಿನ_Const_Null(nodep->resultp());
        puts(")");
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstStop* nodep) override {
        putns(nodep, "VL_STOP_MT(");
        putsQuoted(protect(nodep->fileline()->filename()));
        puts(", ");
        puts(cvt_ಗಮ್ಯ_Str(nodep->fileline()->lineno()));
        puts(", \"\"");
        ಆದರೆ (nodep->isFatal()) puts(", false");
        puts(");\n");
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಮುಕ್ತಾಯ* nodep) override {
        // Disable all the forks so they don't operate after simulation is finished.
        ಆದರೆ (m_cfuncp && m_cfuncp->needProcess()) putns(nodep, "vlProcess->disableFork();\n");
        putns(nodep, "VL_FINISH_MT(");
        putsQuoted(protect(nodep->fileline()->filename()));
        puts(", ");
        puts(cvt_ಗಮ್ಯ_Str(nodep->fileline()->lineno()));
        puts(", \"\");\n");
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಮುಕ್ತಾಯ_Fork* nodep) override {
        // Disable all the forks so they don't operate after simulation is finished.
        ಆದರೆ (m_cfuncp && m_cfuncp->needProcess()) putns(nodep, "vlProcess->disableFork();\n");
        putns(nodep, "VL_FINISH_MT(");
        putsQuoted(protect(nodep->fileline()->filename()));
        puts(", ");
        puts(cvt_ಗಮ್ಯ_Str(nodep->fileline()->lineno()));
        puts(", \"\");\n");
        ಆದರೆ (m_cfuncp->isCoroutine()) {
            putns(nodep, "co_return;\n");
        } ಇಲ್ಲದಿದ್ದರೆ {
            putns(nodep, "return;\n");
        }
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_Print_ಸಮಯ_Scale* nodep) override {
        putns(nodep, "VL_PRINTTIMESCALE(");
        putsQuoted(protect(nodep->prettyName()));
        puts(", ");
        putsQuoted(nodep->timeunit().ascii());
        puts(", vlSymsp->_vm_contextp__);\n");
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstRand* nodep) override {
        emitOpName(nodep, nodep->emitC(), nodep->seedp(), nullptr, nullptr);
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstRandRNG* nodep) override {
        emitOpName(nodep, nodep->emitC(), nullptr, nullptr, nullptr);
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಸಮಯ* nodep) override {
        putns(nodep, "VL_TIME_UNITED_Q(");
        UASSERT_OBJ(!nodep->timeunit().isNone(), nodep, "$time has no units");
        ಸ್ಥಿರಾಂಕ ದ್ವಿಗುಣ ಸಮಯ_2
            = nodep->timeunit().multiplier() / v3Global.rootp()->timeprecision().multiplier();
        UASSERT_OBJ(ಸಮಯ_2 >= 1, nodep, "TimeQ is less than 1, will result in division by zero");
        puts(cvt_ಗಮ್ಯ_Str(ಸಮಯ_2));
        puts(")");
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಸಮಯ_D* nodep) override {
        putns(nodep, "VL_TIME_UNITED_D(");
        UASSERT_OBJ(!nodep->timeunit().isNone(), nodep, "$realtime has no units");
        puts(cvt_ಗಮ್ಯ_Str(nodep->timeunit().multiplier()
                      / v3Global.rootp()->timeprecision().multiplier()));
        puts(")");
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_Get_Initial_Random_ಬೀಜ* nodep) override {
        putns(nodep, "vlSymsp->_vm_contextp__->randSeed()");
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಸಮಯ_Format* nodep) override {
        putns(nodep, "VL_TIMEFORMAT_IINI(");
        ಆದರೆ (nodep->unitsp()) {
            puts("true, ");
            iterate_And_ಮುಂದಿನ_Const_Null(nodep->unitsp());
        } ಇಲ್ಲದಿದ್ದರೆ {
            puts("false, 0");
        }
        puts(", ");
        ಆದರೆ (nodep->precisionp()) {
            puts("true, ");
            iterate_And_ಮುಂದಿನ_Const_Null(nodep->precisionp());
        } ಇಲ್ಲದಿದ್ದರೆ {
            puts("false, 0");
        }
        puts(", ");
        ಆದರೆ (nodep->suffixp()) {
            puts("true, ");
            iterate_And_ಮುಂದಿನ_Const_Null(nodep->suffixp());
        } ಇಲ್ಲದಿದ್ದರೆ {
            puts("false, \"\"");
        }
        puts(", ");
        ಆದರೆ (nodep->widthp()) {
            puts("true, ");
            iterate_And_ಮುಂದಿನ_Const_Null(nodep->widthp());
        } ಇಲ್ಲದಿದ್ದರೆ {
            puts("false, 0");
        }
        puts(", vlSymsp->_vm_contextp__);\n");
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಸಮಯ_Precision* nodep) override {
        putns(nodep, "vlSymsp->_vm_contextp__->timeprecision()");
    }

    // Nodes involing AstText
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಪಠ್ಯ* nodep) override {
        // All Text should be under TextBlock/CStmt/CStmtUser/CExpr/CExprUser
        nodep->v3fatalSrc("Text node in unexpected position");
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಪಠ್ಯ_ಖಂಡ* nodep) override {
        putnbs(nodep, "");
        puts(nodep->ಪೂರ್ವಪ್ರತ್ಯಯ());
        emit_Nodes_With_ಪಠ್ಯ(nodep->nodesp(), false, true, nodep->separator());
        puts(nodep->suffix());
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstCStmt* nodep) override {
        putnbs(nodep, "");
        emit_Nodes_With_ಪಠ್ಯ(nodep->nodesp(), false, true, "");
        ensure_New_ಸಾಲು();
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstCExpr* nodep) override {
        putnbs(nodep, "");
        emit_Nodes_With_ಪಠ್ಯ(nodep->nodesp(), false, true, "");
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstCStmtUser* nodep) override {
        putnbs(nodep, "");
        ofp()->putsNoTracking("\n");
        ಆದರೆ (nodep->fromDollarC() && v3Global.opt.decoration() && !v3Global.opt.protectIds()) {
            ofp()->putsNoTracking("// $c statement at " + nodep->fileline()->ascii() + "\n");
        }
        emit_Nodes_With_ಪಠ್ಯ(nodep->nodesp(), m_useSelfForThis, false, "");
        puts("\n");
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstCExprUser* nodep) override {
        putnbs(nodep, "");
        ofp()->putsNoTracking("\n");
        ಆದರೆ (/* is always from $c */ v3Global.opt.decoration() && !v3Global.opt.protectIds()) {
            ofp()->putsNoTracking(
                (nodep->is_ಶುದ್ಧ() ? "// $cpure expression at " : "// $c expression at ")
                + nodep->fileline()->ascii() + "\n");
        }
        emit_Nodes_With_ಪಠ್ಯ(nodep->nodesp(), m_useSelfForThis, false, "");
        puts("\n");
    }

    // Operators
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು_Termop* nodep) override {
        emitOpName(nodep, nodep->emitC(), nullptr, nullptr, nullptr);
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು_Uniop* nodep) override {
        ಆದರೆ (nodep->emit_Check_Max_ಪದಗಳು()
            && (nodep->ಅಗಲ_ಪದಗಳು() > VL_MULS_MAX_ಪದಗಳು
                || nodep->lhsp()->ಅಗಲ_ಪದಗಳು() > VL_MULS_MAX_ಪದಗಳು)) {
            nodep->v3warn(
                E_UNSUPPORTED,
                "Unsupported: "
                    << nodep->prettyOperatorName() << " operator of " << nodep->ಅಗಲ()
                    << " bits exceeds hardcoded limit VL_MULS_MAX_WORDS in verilatedos.h");
        }
        ಆದರೆ (emitSimpleOk(nodep)) {
            putnbs(nodep, "(");
            puts(nodep->emitSimpleOperator());
            puts(" ");
            iterate_And_ಮುಂದಿನ_Const_Null(nodep->lhsp());
            puts(")");
        } ಇಲ್ಲದಿದ್ದರೆ {
            emitOpName(nodep, nodep->emitC(), nodep->lhsp(), nullptr, nullptr);
        }
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು_Biop* nodep) override {
        ಆದರೆ (nodep->emit_Check_Max_ಪದಗಳು() && nodep->ಅಗಲ_ಪದಗಳು() > VL_MULS_MAX_ಪದಗಳು) {
            nodep->v3warn(
                E_UNSUPPORTED,
                "Unsupported: "
                    << nodep->prettyOperatorName() << " operator of " << nodep->ಅಗಲ()
                    << " bits exceeds hardcoded limit VL_MULS_MAX_WORDS in verilatedos.h");
        }
        ಆದರೆ (emitSimpleOk(nodep)) {
            putnbs(nodep, "(");
            iterate_And_ಮುಂದಿನ_Const_Null(nodep->lhsp());
            puts(" ");
            putbs(nodep->emitSimpleOperator());
            puts(" ");
            iterate_And_ಮುಂದಿನ_Const_Null(nodep->rhsp());
            puts(")");
        } ಇಲ್ಲದಿದ್ದರೆ {
            emitOpName(nodep, nodep->emitC(), nodep->lhsp(), nodep->rhsp(), nullptr);
        }
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು_Triop* nodep) override {
        UASSERT_OBJ(!emitSimpleOk(nodep), nodep, "Triop cannot be described in a simple way");
        emitOpName(nodep, nodep->emitC(), nodep->lhsp(), nodep->rhsp(), nodep->thsp());
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstCvtPackString* nodep) override { emitCvtPackStr(nodep->lhsp()); }
    ಶೂನ್ಯ ಭೇಟಿ(AstRedXor* nodep) override {
        ಆದರೆ (nodep->lhsp()->isWide()) {
            ಭೇಟಿ(static_cast<Ast_ಗಂಟು_Uniop*>(nodep));
        } ಇಲ್ಲದಿದ್ದರೆ {
            ಸ್ಥಿರಾಂಕ AstVarRef* ಸ್ಥಿರಾಂಕ vrefp = VN_CAST(nodep->lhsp(), VarRef);
            ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ ಅಗಲ_Pow2 = vrefp ? vrefp->varp()->dtypep()->ಅಗಲ_Pow2()
                                        : nodep->lhsp()->dtypep()->ಅಗಲ_Pow2();
            UASSERT_OBJ(ಅಗಲ_Pow2 > 1, nodep,
                        "Reduction over single bit value should have been folded");
            putnbs(nodep, "VL_REDXOR_");
            puts(cvt_ಗಮ್ಯ_Str(ಅಗಲ_Pow2));
            puts("(");
            iterate_And_ಮುಂದಿನ_Const_Null(nodep->lhsp());
            puts(")");
        }
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstCCast* nodep) override {
        // Extending a value of the same word width is just a NOP.
        ಆದರೆ (ಸ್ಥಿರಾಂಕ AstClassRefDType* ಸ್ಥಿರಾಂಕ classDtypep
            = VN_CAST(nodep->dtypep()->skipRefp(), ClassRefDType)) {
            putns(nodep, "(" + classDtypep->cType("", false, false) + ")(");
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (nodep->ಗಾತ್ರ_2() <= VL_BYTESIZE) {
            putns(nodep, "(::CData)(");
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (nodep->ಗಾತ್ರ_2() <= VL_SHORTSIZE) {
            putns(nodep, "(::SData)(");
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (nodep->ಗಾತ್ರ_2() <= VL_IDATASIZE) {
            putns(nodep, "(::IData)(");
        } ಇಲ್ಲದಿದ್ದರೆ {
            putns(nodep, "(::QData)(");
        }
        iterate_And_ಮುಂದಿನ_Const_Null(nodep->lhsp());
        puts(")");
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstCond* nodep) override {
        // Widths match up already, so we'll just use C++'s operator w/o any temps.
        ಆದರೆ (nodep->thenp()->isWide()) {
            emitOpName(nodep, nodep->emitC(), nodep->condp(), nodep->thenp(), nodep->elsep());
        } ಇಲ್ಲದಿದ್ದರೆ {
            putnbs(nodep, "(");
            iterate_And_ಮುಂದಿನ_Const_Null(nodep->condp());
            putbs(" ? ");
            iterate_And_ಮುಂದಿನ_Const_Null(nodep->thenp());
            putbs(" : ");
            iterate_And_ಮುಂದಿನ_Const_Null(nodep->elsep());
            puts(")");
        }
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಹೊಂದಿಕೆ_Masked* nodep) override {
        emitOpName(nodep, nodep->emitC(), nodep->lhsp(), nodep->matchp(), nullptr);
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstMemberSel* nodep) override {
        ಸ್ಥಿರಾಂಕ AstVar* ಸ್ಥಿರಾಂಕ varp = nodep->varp();
        ಸ್ಥಿರಾಂಕ bool dereferenceCovergroupRef
            = varp->covergroupRefMember() && nodep->access().isReadOrRW();
        ಆದರೆ (dereferenceCovergroupRef) putnbs(nodep, "(*");
        iterate_And_ಮುಂದಿನ_Const_Null(nodep->fromp());
        putnbs(nodep, "->");
        ಆದರೆ (varp->isIfaceRef()) {
            // varp is the __Viftop companion (e.g. "tx__Viftop"); use the
            // MemberSel name which matches the cell's C++ member (e.g. "tx").
            puts(nodep->nameProtect());
        } ಇಲ್ಲದಿದ್ದರೆ {
            puts(varName(varp));
        }
        ಆದರೆ (dereferenceCovergroupRef) puts(")");
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstStructSel* nodep) override {
        iterate_And_ಮುಂದಿನ_Const_Null(nodep->fromp());
        putnbs(nodep, ".");
        puts(nodep->nameProtect());
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstNullCheck* nodep) override {
        putns(nodep, "VL_NULL_CHECK(");
        iterate_And_ಮುಂದಿನ_Const_Null(nodep->lhsp());
        puts(", ");
        putsQuoted(protect(nodep->fileline()->filename()));
        puts(", ");
        puts(cvt_ಗಮ್ಯ_Str(nodep->fileline()->lineno()));
        puts(")");
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstNewCopy* nodep) override {
        // Polymorphic shallow clone: preserves runtime type via virtual clone()
        // VL_NULL_CHECK enforces null check per IEEE 1800-2023 8.7
        putns(nodep, "VL_NULL_CHECK(");
        ಆದರೆ (VN_IS(nodep->rhsp(), Const) && VN_AS(nodep->rhsp(), Const)->isNull()) {
            // V3Const folded rhs to null: emit a typed empty ref so VL_NULL_CHECK fires
            ಸ್ಥಿರಾಂಕ AstClassRefDType* ಸ್ಥಿರಾಂಕ refDTypep
                = VN_CAST(nodep->dtypep()->skipRefp(), ClassRefDType);
            puts(refDTypep->cType("", false, false) + "{}");
        } ಇಲ್ಲದಿದ್ದರೆ {
            iterate_And_ಮುಂದಿನ_Const_Null(nodep->rhsp());
        }
        puts(", ");
        putsQuoted(protect(nodep->fileline()->filename()));
        puts(", ");
        puts(cvt_ಗಮ್ಯ_Str(nodep->fileline()->lineno()));
        puts(").clone(vlSymsp->__Vm_deleter)");
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstSel* nodep) override {
        // Note ASSIGN checks for this on a LHS
        UASSERT_OBJ(nodep->ಅಗಲ_Min() == nodep->ಅಗಲ_Const(), nodep, "Width mismatch");
        emitOpName(nodep, nodep->emitC(), nodep->fromp(), nodep->lsbp(), nullptr);
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstReplicate* nodep) override {
        ಆದರೆ (nodep->srcp()->ಅಗಲ_Min() == 1 && !nodep->isWide()) {
            UASSERT_OBJ((static_cast<ಪೂರ್ಣಾಂಕ>(VN_AS(nodep->countp(), Const)->ಗಮ್ಯ_UInt())
                         * nodep->srcp()->ಅಗಲ_Min())
                            == nodep->ಅಗಲ_Min(),
                        nodep, "Replicate non-constant or width miscomputed");
            putns(nodep, "VL_REPLICATE_");
            emitIQW(nodep);
            puts("OI(");
            ಆದರೆ (nodep->srcp()) puts(cvt_ಗಮ್ಯ_Str(nodep->srcp()->ಅಗಲ_Min()));
            puts(", ");
            iterate_And_ಮುಂದಿನ_Const_Null(nodep->srcp());
            puts(", ");
            iterate_And_ಮುಂದಿನ_Const_Null(nodep->countp());
            puts(")");
        } ಇಲ್ಲದಿದ್ದರೆ {
            emitOpName(nodep, nodep->emitC(), nodep->srcp(), nodep->countp(), nullptr);
        }
    }
    ಶೂನ್ಯ emit_ಹರಿವು_R(Ast_ಹರಿವು_R* nodep, Ast_ಗಂಟು* ಪೋಷಕ) {
        //TODO: This might need to handle more cases like the visit(AstStreamR) function
        emitOpName(nodep, nodep->emitC(), nodep->lhsp(), nodep->rhsp(), nullptr);
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಹರಿವು_R* nodep) override {
        //The parrent node of our AstStreamR will give just enough info for what streamR should
        //output if nodep->backp() is not the parent then emitStreamR should have been used. throw
        //an error
        bool backp_Is_ಪೋಷಕ = (nodep->backp()->op1p() == nodep || nodep->backp()->op2p() == nodep);
        UASSERT(backp_Is_ಪೋಷಕ, "can not find return type for streamR");
        ಆದರೆ (ಸ್ಥಿರಾಂಕ Ast_ಸರತಿ_DType* ಸ್ಥಿರಾಂಕ qdtp
            = VN_CAST(nodep->backp()->dtypep()->skipRefp(), ಸರತಿ_DType)) {
            ಆದರೆ (nodep->lhsp()->isWide()) {
                emitOpName(nodep,
                           "VL_STREAMR_%nq%lq%rq(%lw," + std::ಗಮ್ಯ_string(qdtp->subDTypep()->ಅಗಲ())
                               + ", %P, %li, %ri)",
                           nodep->lhsp(), nodep->rhsp(), nullptr);
            } ಇಲ್ಲದಿದ್ದರೆ {
                emitOpName(nodep, "VL_STREAMR_%nq%lq%rq(%lw, %P, %li, %ri)", nodep->lhsp(),
                           nodep->rhsp(), nullptr);
            }
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (VN_IS(nodep->lhsp()->dtypep()->skipRefp(), ಸರತಿ_DType)) {
            ಆದರೆ (!((nodep->backp()->op1p() && nodep->backp()->op1p()->isWide())
                  || (nodep->backp()->op2p() && nodep->backp()->op2p()->isWide()))) {
                //If our lhsp is a queue make sure we streamR and return the correct type.
                //If either side is wide or the previous node is string type dont use this case
                emitOpName(nodep->backp(), "VL_STREAMR_%nq%lq%rq(%lw, %P, %li, %ri)",
                           nodep->lhsp(), nodep->rhsp(), nullptr);
            } ಇಲ್ಲದಿದ್ದರೆ {
                emitOpName(nodep, nodep->emitC(), nodep->lhsp(), nodep->rhsp(), nullptr);
            }
        } ಇಲ್ಲದಿದ್ದರೆ {
            emitOpName(nodep, nodep->emitC(), nodep->lhsp(), nodep->rhsp(), nullptr);
        }
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಹರಿವು_L* nodep) override {
        // Attempt to use a "fast" stream function for slice size = power of 2
        ಆದರೆ (!nodep->isWide()) {
            ಸ್ಥಿರಾಂಕ uint32_t isPow2 = VN_AS(nodep->rhsp(), Const)->ಅಂಕೆ().ಎಣಿಕೆ_Ones() == 1;
            ಸ್ಥಿರಾಂಕ uint32_t ತುಂಡು_ಗಾತ್ರ = VN_AS(nodep->rhsp(), Const)->ಗಮ್ಯ_UInt();
            ಆದರೆ (isPow2 && ತುಂಡು_ಗಾತ್ರ <= (nodep->isQuad() ? ಗಾತ್ರ(uint64_t) : ಗಾತ್ರ(uint32_t))) {
                putns(nodep, "VL_STREAML_FAST_");
                bool ಬಳಸುತ್ತದೆ_ಸರತಿ = false;
                Ast_ಸರತಿ_DType* qtypep
                    = nodep->backp()->op2p()
                          ? VN_CAST(nodep->backp()->op2p()->dtypep()->skipRefp(), ಸರತಿ_DType)
                          : nullptr;
                ಆದರೆ (VN_IS(nodep->backp(), ನಿಯೋಜಿಸು)
                    && qtypep) {  // If we are assigning to a queue then emit the correct symbol
                    puts("R");  // R for queue
                    ಬಳಸುತ್ತದೆ_ಸರತಿ = true;
                } ಇಲ್ಲದಿದ್ದರೆ {
                    emitIQW(nodep);
                }
                emitIQW(nodep->lhsp());
                puts("I(");
                puts(cvt_ಗಮ್ಯ_Str(nodep->lhsp()->ಅಗಲ_Min()));
                puts(", ");
                ಆದರೆ (ಬಳಸುತ್ತದೆ_ಸರತಿ) {
                    iterate_And_ಮುಂದಿನ_Const_Null(nodep->backp()->op2p());
                    puts(", ");
                }
                iterate_And_ಮುಂದಿನ_Const_Null(nodep->lhsp());
                puts(", ");
                ಸ್ಥಿರಾಂಕ uint32_t rd_log2 = V3_ಸಂಖ್ಯೆ::log2b(VN_AS(nodep->rhsp(), Const)->ಗಮ್ಯ_UInt());
                puts(cvt_ಗಮ್ಯ_Str(rd_log2) + ")");
                ಹಿಂತಿರುಗಿ;
            }
        }
        ಆದರೆ (VN_IS(nodep->backp(), ನಿಯೋಜಿಸು)
            && VN_IS(nodep->backp()->op2p()->dtypep()->skipRefp(), ಸರತಿ_DType)) {
            ಪೂರ್ಣಾಂಕ ಸರತಿ_ಅಗಲ
                = nodep->backp()->op2p()->dtypep()->subDTypep()->ಅಗಲ();  //We need to know the
                                                                           //width of both sides
            emitOpName(nodep,
                       "VL_STREAML_%nq%lq%rq(%lw," + std::ಗಮ್ಯ_string(ಸರತಿ_ಅಗಲ)
                           + ", %P, %li, %ri)",
                       nodep->lhsp(), nodep->rhsp(), nullptr);
        } ಇಲ್ಲದಿದ್ದರೆ {
            emitOpName(nodep, nodep->emitC(), nodep->lhsp(), nodep->rhsp(), nullptr);
        }
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstCastDynamic* nodep) override {
        putnbs(nodep, "VL_CAST_DYNAMIC(");
        iterate_And_ಮುಂದಿನ_Const_Null(nodep->fromp());
        puts(", ");
        iterate_And_ಮುಂದಿನ_Const_Null(nodep->ತುದಿ());
        puts(")");
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಎಣಿಕೆ_ಬಿಟ್ಟುಗಳು* nodep) override {
        putnbs(nodep, "VL_COUNTBITS_");
        emitIQW(nodep->lhsp());
        puts("(");
        puts(cvt_ಗಮ್ಯ_Str(nodep->lhsp()->ಅಗಲ_Min()));
        puts(", ");
        ಆದರೆ (nodep->lhsp()->isWide()) {
            puts(cvt_ಗಮ್ಯ_Str(nodep->lhsp()->ಅಗಲ_ಪದಗಳು()));  // Note argument width, not node width
                                                          // (which is always 32)
            puts(", ");
        }
        iterate_And_ಮುಂದಿನ_Const_Null(nodep->lhsp());
        puts(", ");
        iterate_And_ಮುಂದಿನ_Const_Null(nodep->rhsp());
        puts(", ");
        iterate_And_ಮುಂದಿನ_Const_Null(nodep->thsp());
        puts(", ");
        iterate_And_ಮುಂದಿನ_Const_Null(nodep->fhsp());
        puts(")");
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstTriggered* nodep) override {
        // Test each word of the trigger vector with a mask of the relevant bits
        std::map<uint32_t, uint64_t> ಶಬ್ದ_Masks;
        ಗಾಗಿ (ಸ್ಥಿರಾಂಕ uint32_t ಸೂಚ್ಯಂಕ : nodep->indices()) {
            ಶಬ್ದ_Masks[VL_BITWORD_Q(ಸೂಚ್ಯಂಕ)] |= VL_ಬಿಟ್_Q(ಸೂಚ್ಯಂಕ);
        }
        putns(nodep, "((");
        bool ಮೊದಲ = true;
        ಗಾಗಿ (ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ& ಶಬ್ದ_ಮುಖವಾಡ : ಶಬ್ದ_Masks) {
            ಆದರೆ (!ಮೊದಲ) puts("\n| ");
            ಮೊದಲ = false;
            ಸ್ಥಿರಾಂಕ bool allOnes = ಶಬ್ದ_ಮುಖವಾಡ.ಎರಡನೇ == ~0ULL;
            ಆದರೆ (!allOnes) puts("(" + V3_ಸಂಖ್ಯೆ{nodep, 64}.ಗಣ_Quad(ಶಬ್ದ_ಮುಖವಾಡ.ಎರಡನೇ).emitC() + " & ");
            iterateConst(nodep->trigRefp());
            puts("[" + std::ಗಮ್ಯ_string(ಶಬ್ದ_ಮುಖವಾಡ.ಮೊದಲ) + "U]");
            ಆದರೆ (!allOnes) puts(")");
        }
        puts(") != 0ULL)");
    }
    // Terminals
    ಶೂನ್ಯ ಭೇಟಿ(AstVarRef* nodep) override {
        ಸ್ಥಿರಾಂಕ AstVar* ಸ್ಥಿರಾಂಕ varp = nodep->varp();

        // Reference to constant pool variable is special
        ಆದರೆ (varp->const_Pool_ನಮೂದು()) {
            putns(nodep, EmitCUtil::constPoolName(varp));
            ಹಿಂತಿರುಗಿ;
        }

        ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_Module* ಸ್ಥಿರಾಂಕ varModp = EmitCParentModule::get(varp);
        ಸ್ಥಿರಾಂಕ bool dereferenceCovergroupRef
            = varp->covergroupRefMember() && nodep->access().isReadOrRW();
        ಆದರೆ (dereferenceCovergroupRef) putns(nodep, "(*");
        ಆದರೆ (VN_IS(varModp, Class) && varModp != m_modp) {
            // Superclass member reference
            putns(nodep, EmitCUtil::ಪೂರ್ವಪ್ರತ್ಯಯ_Name_Protect(varModp) + "::");
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (varp->isIfaceRef()) {
            putns(nodep, nodep->self_ಸೂಚಕ_Protect(m_useSelfForThis));
            ಹಿಂತಿರುಗಿ;
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (!nodep->self_ಸೂಚಕ().is_ಖಾಲಿಗಳು()) {
            emitDereference(nodep, nodep->self_ಸೂಚಕ_Protect(m_useSelfForThis));
        }
        putns(nodep, varName(varp));
        ಆದರೆ (dereferenceCovergroupRef) puts(")");
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_Addr_ದ_CFunc* nodep) override {
        // Note: Can be thought to handle more, but this is all that is needed right now
        ಸ್ಥಿರಾಂಕ AstCFunc* ಸ್ಥಿರಾಂಕ funcp = nodep->funcp();
        UASSERT_OBJ(funcp->isLoose(), nodep, "Cannot take address of non-loose method");
        putns(nodep, "&");
        puts(funcNameProtect(funcp));
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstConst* nodep) override {  //
        ಆದರೆ (m_wide_ತಾತ್ಕಾಲಿಕ_Refp && nodep->isWide()) {
            UASSERT_OBJ(m_wide_ತಾತ್ಕಾಲಿಕ_Refp, nodep, "Wide Constant w/ no temp");
            emitConstantW(nodep, m_wide_ತಾತ್ಕಾಲಿಕ_Refp);
            m_wide_ತಾತ್ಕಾಲಿಕ_Refp = nullptr;  // We used it, fail if set it a second time
        } ಇಲ್ಲದಿದ್ದರೆ {
            emitConstant(nodep);
        }
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು_Cons* nodep) override { emitValCons(nodep); }
    ಶೂನ್ಯ ಭೇಟಿ(AstThisRef* nodep) override {
        putnbs(nodep, nodep->dtypep()->cType("", false, false));
        puts("{");
        puts(VSelf_ಸೂಚಕ_ಪಠ್ಯ::replaceThis(m_useSelfForThis, "this"));
        puts("}");
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು_Sel* nodep) override {
        ಆದರೆ (!VN_IS(nodep, ArraySel) && !VN_IS(nodep, ಶಬ್ದ_Sel)) {
            ಭೇಟಿ(static_cast<Ast_ಗಂಟು_Biop*>(nodep));
            ಹಿಂತಿರುಗಿ;
        }
        // ArraySel or WordSel
        iterate_And_ಮುಂದಿನ_Const_Null(nodep->fromp());
        // Special case constant index for readability
        ಆದರೆ (AstConst* ಸ್ಥಿರಾಂಕ idxp = VN_CAST(nodep->bitp(), Const)) {
            puts("[" + std::ಗಮ್ಯ_string(idxp->ಗಮ್ಯ_UInt()) + "U]");
            ಹಿಂತಿರುಗಿ;
        }
        putbs("[");
        iterate_And_ಮುಂದಿನ_Const_Null(nodep->bitp());
        puts("]");
    }

    //
    ಶೂನ್ಯ ಭೇಟಿ(Ast_Exec_ನಕ್ಷೆ* nodep) override {
        // The location of the AstExecGraph within the containing AstCFunc is where we want to
        // invoke the graph and wait for it to complete. The run-time library does just that.
        putns(nodep, "vl_invokeExecGraph(" + EmitCUtil::exec_ನಕ್ಷೆ_Name(nodep) + ", vlSelf, ");
        iterateConst(nodep->triggerp());
        puts(".data());\n");
    }

    // Default
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು* nodep) override {  // LCOV_EXCL_START
        putns(nodep, "\n???? // "s + nodep->prettyTypeName() + "\n");
        iterateChildrenConst(nodep);
        ಆದರೆ (!v3Global.opt.lintOnly()) {  // An internal problem, so suppress
            nodep->v3fatalSrc("Unknown node type reached emitter: " << nodep->prettyTypeName());
        }
    }  // LCOV_EXCL_STOP

ಸಂರಕ್ಷಿತ:
    EmitCFunc() = ಪೂರ್ವನಿಯೋಜಿತ;
    ~EmitCFunc() override = ಪೂರ್ವನಿಯೋಜಿತ;
};

#ಆದರೆ_ಅಂತ್ಯ  // guard
