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

#ਜੇ_ਅਪਰਿਭਾਸ਼ਿਤ VERILATOR_V3EMITCFUNC_H_
#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ VERILATOR_V3EMITCFUNC_H_

#ਅਨ੍ਤਰ੍ਭਾਵ "config_build.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "verilatedos.h"

#ਅਨ੍ਤਰ੍ਭਾਵ "V3EmitCConstInit.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3Global.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3MemberMap.h"

#ਅਨ੍ਤਰ੍ਭਾਵ <algorithm>
#ਅਨ੍ਤਰ੍ਭਾਵ <map>
#ਅਨ੍ਤਰ੍ਭਾਵ <unordered_map>
#ਅਨ੍ਤਰ੍ਭਾਵ <unordered_set>
#ਅਨ੍ਤਰ੍ਭਾਵ <vector>

// Number of VL_CONST_W_*X's in verilated.h (IE VL_CONST_W_8X is last)
constexpr ਪੂਰਨ EMITC_ਅੰਕ_CONSTW = 8;

//######################################################################
// Emit lazy forward declarations

class EmitCLazyDecls final : ਜਨਤਕ VNVisitorConst {
    // NODE STATE/TYPES
    // None allowed to support threaded emitting

    // MEMBERS
    std::unordered_ਸੈੱਟ<string> m_emittedManually;  // Set of names already declared manually.
    EmitCBaseVisitorConst& m_emitter;  // For access to file output
    bool m_needs_Blank_ਸਤਰ = false;  // Emit blank line if any declarations were emitted (cosmetic)
    std::ਸੈੱਟ<Ast_ਗੰਢ*> m_emitted;  // -> in set. Already emitted decl for symbols.

    // METHODS
    bool declaredOnce(Ast_ਗੰਢ* nodep) { ਵਾਪਸ m_emitted.ਪਾਓ(nodep).ਦੂਜਾ; }

    ਖਾਲੀ lazyDeclare(AstCFunc* funcp) {
        // Already declared in this compilation unit
        ਜੇ (!declaredOnce(funcp)) ਵਾਪਸ;
        // Check if this kind of function is lazily declared
        ਜੇ (!(funcp->is_ਵਿਧੀ() && funcp->isLoose()) && !funcp->dpiImportPrototype()) ਵਾਪਸ;
        // Already declared manually
        ਜੇ (m_emittedManually.ਗਿਣਤੀ_2(funcp->nameProtect())) ਵਾਪਸ;
        // Needs lazy declaration, emit one
        m_emitter.emitCFuncDecl(funcp, EmitCParentModule::get(funcp), funcp->dpiImportPrototype());
        m_needs_Blank_ਸਤਰ = true;
    }

    ਖਾਲੀ lazyDeclareConstPoolVar(AstVar* varp) {
        ਜੇ (!declaredOnce(varp)) ਵਾਪਸ;  // Already declared
        ਅਚਲ string nameProtect = EmitCUtil::constPoolName(varp);
        m_emitter.putns(varp, "extern const ");
        m_emitter.puts(varp->dtypep()->cType(nameProtect, false, false));
        m_emitter.puts(";\n");
        m_needs_Blank_ਸਤਰ = true;
    }

    // VISITORS
    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ_CCall* nodep) override {
        lazyDeclare(nodep->funcp());
        iterateChildrenConst(nodep);
    }

    ਖਾਲੀ ਦੇਖੋ(Ast_Addr_ਦਾ_CFunc* nodep) override {
        lazyDeclare(nodep->funcp());
        iterateChildrenConst(nodep);
    }

    ਖਾਲੀ ਦੇਖੋ(AstVarRef* nodep) override {
        AstVar* ਅਚਲ varp = nodep->varp();
        // Only constant pool symbols are lazy declared for now ...
        ਜੇ (EmitCParentModule::get(varp)->isConstPool()) lazyDeclareConstPoolVar(varp);
    }

    ਖਾਲੀ ਦੇਖੋ(Ast_Exec_ਗ੍ਰਾਫ* nodep) override {
        ਜੇ (!declaredOnce(nodep)) ਵਾਪਸ;
        m_emitter.putns(nodep, "extern VlExecGraph " + EmitCUtil::exec_ਗ੍ਰਾਫ_Name(nodep) + ";\n");
        m_needs_Blank_ਸਤਰ = true;
    }

    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ* nodep) override { iterateChildrenConst(nodep); }

ਜਨਤਕ:
    explicit EmitCLazyDecls(EmitCBaseVisitorConst& emitter)
        : m_emitter{emitter} {}
    ਖਾਲੀ emit(Ast_ਗੰਢ* nodep) {
        m_needs_Blank_ਸਤਰ = false;
        iterateChildrenConst(nodep);
        ਜੇ (m_needs_Blank_ਸਤਰ) m_emitter.puts("\n");
    }
    ਖਾਲੀ emit(ਅਚਲ string& ਅਗੇਤਰ, ਅਚਲ string& name, ਅਚਲ string& suffix) {
        m_emittedManually.ਪਾਓ(name);
        m_emitter.ensure_New_ਸਤਰ();
        m_emitter.puts(ਅਗੇਤਰ);
        m_emitter.puts(name);
        m_emitter.puts(suffix);
        m_emitter.ensure_New_ਸਤਰ();
    }
    ਖਾਲੀ declared(AstCFunc* nodep) { m_emitted.ਪਾਓ(nodep); }
    ਖਾਲੀ reset() { m_emitted.clear(); }
};

// ######################################################################
//  Emit statements and expressions

class EmitCFunc VL_NOT_FINAL : ਜਨਤਕ EmitCConstInit {
    VMemberMap m_memberMap;
    AstVarRef* m_wide_ਆਰਜ਼ੀ_Refp = nullptr;  // Variable that _WW macros should be setting
    std::unordered_map<Ast_Jump_ਖੰਡ*, ਮਾਪ_t> m_labelNumbers;  // Label numbers for AstJumpBlocks
    bool m_createdScopeHash = false;  // Already created a scope hash

ਸੁਰੱਖਿਅਤ:
    VL_DEFINE_DEBUG_FUNCTIONS;

    EmitCLazyDecls m_lazyDecls{*this};  // Visitor for emitting lazy declarations
    bool m_useSelfForThis = false;  // Replace "this" with "vlSelf"
    bool m_usevlSelfRef = false;  // Use vlSelfRef reference instead of vlSelf pointer
    ਅਚਲ Ast_ਗੰਢ_Module* m_modp = nullptr;  // Current module being emitted
    ਅਚਲ AstCFunc* m_cfuncp = nullptr;  // Current function being emitted
    bool m_instantiatesOwnProcess = false;
    ਅਚਲ AstClassPackage* m_classOrPackage = nullptr;  // Pointer to current class or package
    string m_classOrPackageHash;  // Hash of class or package name

    bool constructorNeedsProcess(ਅਚਲ AstClass* ਅਚਲ classp) {
        ਅਚਲ Ast_ਗੰਢ* ਅਚਲ newp = m_memberMap.ਲੱਭੋ_Member(classp, "new");
        ਜੇ (!newp) ਵਾਪਸ false;
        ਅਚਲ AstCFunc* ਅਚਲ ctorp = VN_CAST(newp, CFunc);
        ਜੇ (!ctorp) ਵਾਪਸ false;
        UASSERT_OBJ(ctorp->isConstructor(), ctorp, "`new` is not a constructor!");
        ਵਾਪਸ ctorp->needProcess();
    }

    bool constructorNeedsProcess(ਅਚਲ Ast_ਗੰਢ_DType* ਅਚਲ dtypep) {
        ਜੇ (ਅਚਲ AstClassRefDType* ਅਚਲ crefdtypep = VN_CAST(dtypep, ClassRefDType))
            ਵਾਪਸ constructorNeedsProcess(crefdtypep->classp());
        ਵਾਪਸ false;
    }

    bool coverage_ਵਰਤਦਾ_Local_Counter(Ast_ਗੰਢ_Cover_Decl* ਅਚਲ declp) ਅਚਲ {
        // Only functions with an object receiver can address the module-local
        // coverage array. Static/package/class helper functions still use the
        // symbol-table array, as they may have vlSymsp but no vlSelf/this.
        // Also require the coverage declaration to belong to the current
        // module; cloned task bodies can increment declarations owned by a
        // different module, and those must keep using the global array.
        ਵਾਪਸ m_cfuncp && !m_cfuncp->isStatic() && !VN_IS(m_modp, Class)
               && !VN_IS(m_modp, ClassPackage)
               && EmitCParentModule::get(declp->ਅੰਕੜੇ_Decl_Thisp()) == m_modp;
    }
    ਪੂਰਨ coverage_Bin_ਅੰਕ(Ast_ਗੰਢ_Cover_Decl* ਅਚਲ declp, bool forceGlobal = false) ਅਚਲ {
        ਵਾਪਸ !forceGlobal && coverage_ਵਰਤਦਾ_Local_Counter(declp)
                   ? declp->ਅੰਕੜੇ_Decl_Thisp()->local_Bin_ਅੰਕ()
                   : declp->ਅੰਕੜੇ_Decl_Thisp()->bin_ਅੰਕ();
    }
    ਖਾਲੀ putCoverageArray(Ast_ਗੰਢ_Cover_Decl* ਅਚਲ declp, bool forceGlobal = false) {
        ਜੇ (!forceGlobal && coverage_ਵਰਤਦਾ_Local_Counter(declp)) {
            // Keep counters on the module object when possible, so every
            // no-inline instance has storage available if coverage output is
            // later written with forcePerInstance.
            puts(m_cfuncp->isLoose() ? "vlSelf->__Vcoverage" : "this->__Vcoverage");
        } ਨਹੀਂਤਾਂ {
            // Static/package/class helper functions do not have vlSelf, so
            // keep using the shared symbol-table coverage array there.
            puts("vlSymsp->__Vcoverage");
        }
    }
    ਖਾਲੀ emit_Cover_Other_Decl_ਪਾਓ(AstCoverOtherDecl* nodep, bool forceGlobal = false) {
        putns(nodep, "vlSelf->__vlCoverInsert(");  // As Declared in emitCoverageDecl
        putCoverageArray(nodep, forceGlobal);
        puts(" + ");
        puts(cvt_ਮੰਜ਼ਿਲ_Str(coverage_Bin_ਅੰਕ(nodep, forceGlobal)));
        // first controls whether duplicate module instances are collapsed in
        // the default coverage view. The following flag says whether countp
        // points to an object-local counter; only those counters must be kept
        // real for later instances so forcePerInstance can split hierarchy.
        puts(", first");  // Enable, passed from __Vconfigure parameter
        puts(", ");
        puts(!forceGlobal && coverage_ਵਰਤਦਾ_Local_Counter(nodep) ? "true" : "false");
        puts(", ");
        putsQuoted(protect(nodep->fileline()->filename()));
        puts(", ");
        puts(cvt_ਮੰਜ਼ਿਲ_Str(nodep->fileline()->lineno()));
        puts(", ");
        puts(cvt_ਮੰਜ਼ਿਲ_Str(nodep->ਵਿੱਥ() + nodep->fileline()->ਪਹਿਲਾ_ਕਾਲਮ()));
        puts(", ");
        putsQuoted((!nodep->hier().ਖਾਲੀ_ਥਾਵਾਂ() ? "." : "")
                   + VIdProtect::protect_ਸ਼ਬਦ_If(nodep->hier(), nodep->protect()));
        puts(", ");
        putsQuoted(VIdProtect::protect_ਸ਼ਬਦ_If(nodep->ਪੰਨਾ(), nodep->protect()));
        puts(", ");
        putsQuoted(VIdProtect::protect_ਸ਼ਬਦ_If(nodep->comment(), nodep->protect()));
        puts(", ");
        putsQuoted(nodep->linescov());
        puts(", ");
        putsQuoted(VIdProtect::protect_ਸ਼ਬਦ_If(nodep->fsmVar(), nodep->protect()));
        puts(", ");
        putsQuoted(VIdProtect::protect_ਸ਼ਬਦ_If(nodep->fsmFrom(), nodep->protect()));
        puts(", ");
        putsQuoted(VIdProtect::protect_ਸ਼ਬਦ_If(nodep->fsm_ਮੰਜ਼ਿਲ(), nodep->protect()));
        puts(", ");
        putsQuoted(VIdProtect::protect_ਸ਼ਬਦ_If(nodep->fsm_ਟੈਗ(), nodep->protect()));
        puts(");\n");
    }

ਜਨਤਕ:
    // METHODS
    bool display_Emit_ਸਿਰਲੇਖ(Ast_ਗੰਢ* nodep);
    ਖਾਲੀ display_ਗੰਢ(Ast_ਗੰਢ* nodep, AstSFormatF* fmtp, ਅਚਲ string& vformat, Ast_ਗੰਢ* exprsp,
                     bool is_ਜਾਂਚ);

    bool emitSimpleOk(Ast_ਗੰਢ_ਵਿਅੰਜਕ* nodep);
    ਖਾਲੀ emitIQW(ਅਚਲ Ast_ਗੰਢ* nodep) {
        // See "Type letters" in verilated.h
        // Other abbrevs: "C"har, "S"hort, "F"loat, "D"ouble, stri"N"g, "R"=queue, "U"npacked
        puts(nodep->dtypep()->skipRefp()->charIQWN());
    }
    ਖਾਲੀ emitRU(ਅਚਲ Ast_ਗੰਢ* nodep) {
        ਅਚਲ Ast_ਗੰਢ_DType* dtp = nodep->dtypep()->skipRefp();
        // See "Type letters" in verilated.h
        ਜੇ (VN_IS(dtp, UnpackArrayDType))
            puts("U");
        ਨਹੀਂਤਾਂ ਜੇ (VN_IS(dtp, ਕਤਾਰ_DType) || VN_IS(dtp, DynArrayDType))
            puts("R");
    }
    ਖਾਲੀ emitScIQW(AstVar* nodep) {
        UASSERT_OBJ(nodep->isSc(), nodep, "emitting SystemC operator on non-SC variable");
        puts(nodep->isScBigUint()                           ? "SB"
             : (nodep->isScUint() || nodep->isScUintBool()) ? "SU"
             : nodep->isScBv()                              ? "SW"
             : nodep->isDouble()                            ? "SD"
                                                            : (nodep->isScQuad() ? "SQ" : "SI"));
    }
    ਖਾਲੀ emitDatap(ਅਚਲ Ast_ਗੰਢ* nodep) {
        // When passing to a function with va_args the compiler doesn't
        // know need a pointer so when wide, need to look inside VlWide
        ਜੇ (nodep->isWide()) puts(".data()");
    }
    ਖਾਲੀ emitOpName(Ast_ਗੰਢ* nodep, ਅਚਲ string& format, Ast_ਗੰਢ* lhsp, Ast_ਗੰਢ* rhsp,
                    Ast_ਗੰਢ* thsp);
    ਖਾਲੀ emit_CCall_ਦਲੀਲਾਂ(ਅਚਲ Ast_ਗੰਢ_CCall* nodep, ਅਚਲ string& self_ਸੂਚਕ, bool ਵਿੱਚ_Process);
    ਖਾਲੀ emitDereference(Ast_ਗੰਢ* nodep, ਅਚਲ string& ਸੂਚਕ);
    std::string dereferenceString(ਅਚਲ std::string& ਸੂਚਕ) ਅਚਲ;
    ਖਾਲੀ emitCvtPackStr(Ast_ਗੰਢ* nodep);
    ਖਾਲੀ emitConstant(AstConst* nodep);
    ਖਾਲੀ emitConstantString(ਅਚਲ AstConst* nodep);
    ਖਾਲੀ emitVarReset(ਅਚਲ string& ਅਗੇਤਰ, AstVar* varp, bool constructing);
    string emitVarResetRecurse(ਅਚਲ AstVar* varp, bool constructing,
                               ਅਚਲ string& varNameProtected, Ast_ਗੰਢ_DType* dtypep, ਪੂਰਨ ਡੂੰਘਾਈ,
                               ਅਚਲ string& suffix, ਅਚਲ Ast_ਗੰਢ* valuep);
    ਖਾਲੀ emitVarResetScopeHash();
    ਖਾਲੀ emitChangeDet();
    ਖਾਲੀ put_Comma_Iterate_ਅਗਲਾ(Ast_ਗੰਢ* nodep, bool comma = false) {
        ਲਈ (Ast_ਗੰਢ* subnodep = nodep; subnodep; subnodep = subnodep->nextp()) {
            ਜੇ (comma) puts(", ");
            iterateConst(subnodep);
            comma = true;
        }
    }
    ਸਾਂਚਾ <typename T>
    string optionalProcArg(ਅਚਲ T* ਅਚਲ nodep) {
        ਵਾਪਸ (nodep && constructorNeedsProcess(nodep)) ? "vlProcess, " : "";
    }
    ਅਚਲ AstCNew* getSuperNewCallRecursep(Ast_ਗੰਢ* ਅਚਲ nodep) {
        // Get the super.new call
        ਜੇ (!nodep) ਵਾਪਸ nullptr;
        ਜੇ (ਅਚਲ AstCNew* ਅਚਲ cnewp = VN_CAST(nodep, CNew)) ਵਾਪਸ cnewp;
        ਜੇ (ਅਚਲ Ast_Stmt_ਵਿਅੰਜਕ* ਅਚਲ stmtp = VN_CAST(nodep, Stmt_ਵਿਅੰਜਕ)) {
            ਜੇ (ਅਚਲ AstCNew* ਅਚਲ cnewp = VN_CAST(stmtp->exprp(), CNew)) ਵਾਪਸ cnewp;
        }
        ਜੇ (ਅਚਲ Ast_Jump_ਖੰਡ* ਅਚਲ blockp = VN_CAST(nodep, Jump_ਖੰਡ)) {
            ਜੇ (ਅਚਲ AstCNew* ਅਚਲ cnewp = getSuperNewCallRecursep(blockp->stmtsp())) {
                ਵਾਪਸ cnewp;
            }
        }
        ਜੇ (ਅਚਲ AstCNew* ਅਚਲ cnewp = getSuperNewCallRecursep(nodep->nextp())) ਵਾਪਸ cnewp;
        ਵਾਪਸ nullptr;
    }

    ਖਾਲੀ emitConstantW(AstConst* nodep, AstVarRef* assigntop) {
        // For tradition and compilation speed, assign each word directly into
        // output variable instead of using '='
        putns(nodep, "");
        ਜੇ (nodep->ਅੰਕ().isFourState()) {
            nodep->v3warn(E_UNSUPPORTED, "Unsupported: 4-state numbers in this context");
            ਵਾਪਸ;
        }

        ਪੂਰਨ ਉੱਪਰ_ਚੌੜਾਈ = nodep->ਅੰਕ().ਚੌੜਾਈ_ਮੰਜ਼ਿਲ_Fit();
        ਪੂਰਨ chunks = 0;
        ਜੇ (ਉੱਪਰ_ਚੌੜਾਈ > EMITC_ਅੰਕ_CONSTW * VL_EDATASIZE) {
            // Output e.g. 8 words in groups of e.g. 8
            chunks = (ਉੱਪਰ_ਚੌੜਾਈ - 1) / (EMITC_ਅੰਕ_CONSTW * VL_EDATASIZE);
            ਉੱਪਰ_ਚੌੜਾਈ %= (EMITC_ਅੰਕ_CONSTW * VL_EDATASIZE);
            ਜੇ (ਉੱਪਰ_ਚੌੜਾਈ == 0) ਉੱਪਰ_ਚੌੜਾਈ = (EMITC_ਅੰਕ_CONSTW * VL_EDATASIZE);
        }
        {  // Upper e.g. 8 words
            ਜੇ (chunks) {
                putnbs(nodep, "VL_CONSTHI_W_");
                puts(cvt_ਮੰਜ਼ਿਲ_Str(VL_ਸ਼ਬਦ_I(ਉੱਪਰ_ਚੌੜਾਈ)));
                puts("X(");
                puts(cvt_ਮੰਜ਼ਿਲ_Str(nodep->ਚੌੜਾਈ_Min()));
                puts(",");
                puts(cvt_ਮੰਜ਼ਿਲ_Str(chunks * EMITC_ਅੰਕ_CONSTW * VL_EDATASIZE));
            } ਨਹੀਂਤਾਂ {
                putnbs(nodep, "VL_CONST_W_");
                puts(cvt_ਮੰਜ਼ਿਲ_Str(VL_ਸ਼ਬਦ_I(ਉੱਪਰ_ਚੌੜਾਈ)));
                puts("X(");
                puts(cvt_ਮੰਜ਼ਿਲ_Str(nodep->ਚੌੜਾਈ_Min()));
            }
            puts(",");
            ਜੇ (!assigntop->self_ਸੂਚਕ().is_ਖਾਲੀ_ਥਾਵਾਂ()) {
                emitDereference(assigntop, assigntop->self_ਸੂਚਕ_Protect(m_useSelfForThis));
            }
            puts(varName(assigntop->varp()));
            ਲਈ (ਪੂਰਨ ਸ਼ਬਦ = VL_ਸ਼ਬਦ_I(ਉੱਪਰ_ਚੌੜਾਈ) - 1; ਸ਼ਬਦ >= 0; ਸ਼ਬਦ--) {
                // Only 32 bits - llx + long long here just to appease CPP format warning
                ofp()->ਲਿਖੋ(",0x%08" PRIx64, static_cast<uint64_t>(nodep->ਅੰਕ().edata_ਸ਼ਬਦ(
                                                   ਸ਼ਬਦ + chunks * EMITC_ਅੰਕ_CONSTW)));
            }
            puts(")");
        }
        ਲਈ (chunks--; chunks >= 0; chunks--) {
            puts(";\n");
            putbs("VL_CONSTLO_W_");
            puts(cvt_ਮੰਜ਼ਿਲ_Str(EMITC_ਅੰਕ_CONSTW));
            puts("X(");
            puts(cvt_ਮੰਜ਼ਿਲ_Str(chunks * EMITC_ਅੰਕ_CONSTW * VL_EDATASIZE));
            puts(",");
            ਜੇ (!assigntop->self_ਸੂਚਕ().is_ਖਾਲੀ_ਥਾਵਾਂ()) {
                emitDereference(assigntop, assigntop->self_ਸੂਚਕ_Protect(m_useSelfForThis));
            }
            puts(varName(assigntop->varp()));
            ਲਈ (ਪੂਰਨ ਸ਼ਬਦ = EMITC_ਅੰਕ_CONSTW - 1; ਸ਼ਬਦ >= 0; ਸ਼ਬਦ--) {
                // Only 32 bits - llx + long long here just to appease CPP format warning
                ofp()->ਲਿਖੋ(",0x%08" PRIx64, static_cast<uint64_t>(nodep->ਅੰਕ().edata_ਸ਼ਬਦ(
                                                   ਸ਼ਬਦ + chunks * EMITC_ਅੰਕ_CONSTW)));
            }
            puts(")");
        }
    }

    ਖਾਲੀ emit_Nodes_With_ਪਾਠ(Ast_ਗੰਢ* nodesp, bool useSelfForThis, bool tracking,
                           ਅਚਲ std::string& separator) {
        ਲਈ (Ast_ਗੰਢ* nodep = nodesp; nodep; nodep = nodep->nextp()) {
            ਜੇ (ਅਚਲ Ast_ਪਾਠ* ਅਚਲ textp = VN_CAST(nodep, ਪਾਠ_2)) {
                ਅਚਲ std::string ਪਾਠ
                    = VSelf_ਸੂਚਕ_ਪਾਠ::replaceThis(useSelfForThis, textp->ਪਾਠ());
                ਜੇ (tracking) {
                    puts(ਪਾਠ);
                } ਨਹੀਂਤਾਂ {
                    ofp()->putsNoTracking(ਪਾਠ);
                }
            } ਨਹੀਂਤਾਂ {
                iterateConst(nodep);
            }
            ਜੇ (nodep->nextp()) puts(separator);
        }
    }

    ਖਾਲੀ putConstructorSubinit(ਅਚਲ AstClass* classp, AstCFunc* cfuncp) {
        // Direct non-virtual bases in declaration order
        ਲਈ (ਅਚਲ AstClassExtends* extp = classp->extendsp(); extp;
             extp = VN_AS(extp->nextp(), ClassExtends)) {
            ਜੇ (extp->classp()->isInterfaceClass()) ਜਾਰੀ;
            ਜੇ (extp->classp()->use_Virtual_ਜਨਤਕ()) ਜਾਰੀ;
            puts(EmitCUtil::ਅਗੇਤਰ_Name_Protect(extp->classp()));
            puts("::init");
            ਜੇ (constructorNeedsProcess(extp->classp())) {
                puts("(vlProcess, vlSymsp");
            } ਨਹੀਂਤਾਂ {
                puts("(vlSymsp");
            }
            // Handle super.new() args for the concrete parent
            ਜੇ (ਅਚਲ AstCNew* ਅਚਲ superNewCallp = getSuperNewCallRecursep(cfuncp->stmtsp())) {
                put_Comma_Iterate_ਅਗਲਾ(superNewCallp->argsp(), true);
            }
            puts(");\n");
            // Since SystemVerilog allows only for extending from one class, for sure there won't
            // be any more constructors to be called. Even if there are some classes which are
            // "implemented", those are interface classes which do not have constructors.
            ਰੋਕੋ;
        }
    }

    // VISITORS
    using EmitCConstInit::ਦੇਖੋ;
    ਖਾਲੀ ਦੇਖੋ(AstCFunc* ਅਚਲ nodep) override {
        ਜੇ (nodep->ਖਾਲੀ_ਥਾਵਾਂ_Body() && !nodep->isLoose()) ਵਾਪਸ;
        VL_RESTORER(m_useSelfForThis);
        VL_RESTORER(m_cfuncp);
        VL_RESTORER(m_instantiatesOwnProcess);
        VL_RESTORER(m_createdScopeHash);
        m_cfuncp = nodep;
        m_instantiatesOwnProcess = false;
        m_labelNumbers.clear();  // No need to save/restore, all Jumps must be within the function

        ਵੰਡੋ_ਮਾਪ_Inc(nodep);

        puts("\n");
        m_lazyDecls.emit(nodep);
        ਜੇ (nodep->ifdef() != "") putns(nodep, "#ifdef " + nodep->ifdef() + "\n");
        emit_CFunc_ਸਿਰਲੇਖ(nodep, m_modp, /* withScope: */ true);

        puts(" {\n");
        ਜੇ (nodep->isConstructor()) {
            ਅਚਲ AstClass* ਅਚਲ classp = VN_CAST(nodep->scopep()->modp(), Class);
            ਜੇ (classp && classp->extendsp()) putConstructorSubinit(classp, nodep);
        }

        // "+" in the debug indicates a print from the model
        puts("VL_DEBUG_IF(VL_DBG_MSGF(\"+  ");
        ਲਈ (ਪੂਰਨ i = 0; i < m_modp->level(); ++i) puts("  ");
        puts(EmitCUtil::ਅਗੇਤਰ_Name_Protect(m_modp));
        puts(nodep->isLoose() ? "__" : "::");
        puts(nodep->nameProtect() + "\\n\"); );\n");

        ਜੇ (nodep->isLoose()) {
            m_lazyDecls.declared(nodep);  // Defined here, so no longer needs declaration
            ਜੇ (!nodep->isStatic()) {  // Standard prologue
                m_useSelfForThis = true;
                ਜੇ (nodep->voidSelf()) {
                    ਅਚਲ string className = EmitCUtil::ਅਗੇਤਰ_Name_Protect(m_modp);
                    puts(className + "* const vlSelf = static_cast<" + className
                         + "*>(vlVoidSelf);\n");
                }
                ਜੇ (!VN_IS(m_modp, Class)) {
                    puts(EmitCUtil::sym_Class_ਸੌਂਪੋ());  // Uses vlSelf
                } ਨਹੀਂਤਾਂ {
                    puts("(void)vlSelf;  // Prevent unused variable warning\n");
                }
            }
        }

        // Instantiate a process class if it's going to be needed somewhere later
        nodep->forall([&](ਅਚਲ Ast_ਗੰਢ_CCall* ccallp) -> bool {
            ਜੇ (ccallp->funcp()->needProcess()
                && (ccallp->funcp()->isCoroutine() == VN_IS(ccallp->backp(), CAwait))) {
                ਜੇ (!nodep->needProcess() && !m_instantiatesOwnProcess) {
                    m_instantiatesOwnProcess = true;
                    ਵਾਪਸ false;
                }
            }
            ਵਾਪਸ true;
        });
        ਜੇ (m_instantiatesOwnProcess) {
            AstCStmt* ਅਚਲ vlprocp = new AstCStmt{nodep->fileline()};
            vlprocp->ਜੋੜੋ("VlProcessRef vlProcess = std::make_shared<VlProcess>();\n");
            vlprocp->ਜੋੜੋ("VlProcess::currentp(vlProcess.get());");
            nodep->prependStmtsp(vlprocp);
        } ਨਹੀਂਤਾਂ ਜੇ (nodep->needProcess() && nodep->stmtsp()) {
            // Set current process so VlRNG() constructors in this function seed from it
            AstCStmt* ਅਚਲ ਸੈੱਟ_Processp = new AstCStmt{nodep->fileline()};
            ਸੈੱਟ_Processp->ਜੋੜੋ("VlProcess::currentp(vlProcess.get());");
            nodep->prependStmtsp(ਸੈੱਟ_Processp);
        }

        ਲਈ (Ast_ਗੰਢ* subnodep = nodep->argsp(); subnodep; subnodep = subnodep->nextp()) {
            ਜੇ (AstVar* ਅਚਲ varp = VN_CAST(subnodep, Var)) {
                ਜੇ (varp->isFuncReturn()) emitVarDecl(varp);
            }
        }

        ਜੇ (m_useSelfForThis) {
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

        ਜੇ (nodep->varsp()) {
            putsDecoration(nodep, "// Locals\n");
            iterate_And_ਅਗਲਾ_Const_Null(nodep->varsp());
        }

        ਜੇ (nodep->stmtsp()) {
            putsDecoration(nodep, "// Body\n");
            iterate_And_ਅਗਲਾ_Const_Null(nodep->stmtsp());
        }

        m_usevlSelfRef = false;

        ਜੇ (nodep->isCoroutine()) {
            // Sometimes coroutines don't have co_awaits,
            // so emit a co_return at the end to avoid compile errors.
            puts("co_return;\n");
        }

        puts("}\n");
        ਜੇ (nodep->ifdef() != "") puts("#endif  // " + nodep->ifdef() + "\n");
    }

    ਖਾਲੀ ਦੇਖੋ(AstVar* nodep) override {
        UASSERT_OBJ(m_cfuncp, nodep, "Cannot emit non-local variable");
        emitVarDecl(nodep);
    }

    ਖਾਲੀ ਦੇਖੋ(Ast_Cvt_Array_ਮੰਜ਼ਿਲ_Array* nodep) override {
        ਜੇ (nodep->ਉਲਟ()) {
            puts("VL_REVCLONE_Q(");
        } ਨਹੀਂਤਾਂ {
            puts("VL_CLONE_Q(");
        }
        iterate_And_ਅਗਲਾ_Const_Null(nodep->fromp());
        puts(", ");
        puts(cvt_ਮੰਜ਼ਿਲ_Str(nodep->ਖੰਡ_ਮਾਪ()));
        puts(", ");
        puts(cvt_ਮੰਜ਼ਿਲ_Str(nodep->src_Element_ਬਿੱਟ()));
        puts(", ");
        puts(cvt_ਮੰਜ਼ਿਲ_Str(nodep->dst_Element_ਬਿੱਟ()));
        puts(")");
    }

    ਖਾਲੀ ਦੇਖੋ(Ast_Cvt_Array_ਮੰਜ਼ਿਲ_Packed* nodep) override {
        Ast_ਗੰਢ_DType* ਅਚਲ fromDtp = nodep->fromp()->dtypep()->skipRefp();
        Ast_ਗੰਢ_DType* ਅਚਲ elemDtp = fromDtp->subDTypep()->skipRefp();
        ਅਚਲ bool descending = [&]() {
            ਜੇ (ਅਚਲ AstUnpackArrayDType* ਅਚਲ unpackDtp = VN_CAST(fromDtp, UnpackArrayDType))
                ਵਾਪਸ !unpackDtp->declRange().ascending();
            ਵਾਪਸ false;
        }();
        puts("VL_PACK_");
        emitIQW(nodep);
        puts("_");
        emitRU(fromDtp);
        emitIQW(elemDtp);
        ਜੇ (descending) {
            // Wrap source in VL_PACK_REVERSED so VL_PACK sees ascending order
            emitOpName(nodep, "(%nw, %rw, %P, VL_PACK_REVERSED(%li))", nodep->fromp(), elemDtp,
                       nullptr);
        } ਨਹੀਂਤਾਂ {
            emitOpName(nodep, "(%nw, %rw, %P, %li)", nodep->fromp(), elemDtp, nullptr);
        }
    }

    ਖਾਲੀ ਦੇਖੋ(Ast_Cvt_Unpack_ਮੰਜ਼ਿਲ_ਕਤਾਰ* nodep) override {
        Ast_ਗੰਢ_DType* ਅਚਲ elemDTypep = nodep->fromp()->dtypep()->subDTypep();
        emitOpName(nodep, nodep->emitC(), nodep->fromp(), elemDTypep, nullptr);
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_Cvt_Unpack_ਮੰਜ਼ਿਲ_Unpack* nodep) override {
        // Whether each dimension is reversed, the first for the outermost, up to the last that is
        std::ਸਦਿਸ਼<bool> reverses;
        ਅਚਲ AstUnpackArrayDType* ਸਿਖਰ = VN_AS(nodep->dtypep()->skipRefp(), UnpackArrayDType);
        ਅਚਲ AstUnpackArrayDType* fromp
            = VN_AS(nodep->fromp()->dtypep()->skipRefp(), UnpackArrayDType);
        ਜਦੋਂ (ਸਿਖਰ) {
            reverses.ਧੱਕੋ_ਪਿੱਠ(ਸਿਖਰ->declRange().ascending() != fromp->declRange().ascending());
            ਸਿਖਰ = VN_CAST(ਸਿਖਰ->subDTypep()->skipRefp(), UnpackArrayDType);
            fromp = VN_CAST(fromp->subDTypep()->skipRefp(), UnpackArrayDType);
        }
        ਜਦੋਂ (!reverses.ਪਿੱਠ()) reverses.ਕੱਢੋ_ਪਿੱਠ();
        putns(nodep, "VL_CVT_UNPACK_TO_UNPACK<");
        ਲਈ (ਮਾਪ_t i = 0; i < reverses.ਮਾਪ(); ++i) {
            ਜੇ (i) puts(", ");
            puts(reverses[i] ? "1" : "0");
        }
        puts(">(");
        iterate_And_ਅਗਲਾ_Const_Null(nodep->fromp());
        puts(")");
    }

    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ_ਸੌਂਪੋ* nodep) override {
        ਜੇ (AstCReset* ਅਚਲ resetp = VN_CAST(nodep->rhsp(), CReset)) {
            // TODO get rid of emitVarReset and instead let AstNodeAssign understand how to init
            // anything
            Ast_ਗੰਢ* fromp = nodep->lhsp();
            // Fork needs to use a member select.  Nothing else should be possible before VarRef.
            ਜੇ (AstMemberSel* ਅਚਲ sfromp = VN_CAST(fromp, MemberSel)) {
                // Fork-DynScope generated pointer to previously automatic variable
                AstVar* ਅਚਲ memberVarp = sfromp->varp();
                fromp = sfromp->fromp();
                ਜੇ (AstNullCheck* ਅਚਲ sfromp = VN_CAST(fromp, NullCheck)) fromp = sfromp->lhsp();
                Ast_ਗੰਢ_Var_Ref* ਅਚਲ fromVarRefp = VN_AS(fromp, ਗੰਢ_Var_Ref);
                emitVarReset(
                    ("VL_NULL_CHECK("s
                     + (fromVarRefp->self_ਸੂਚਕ().is_ਖਾਲੀ_ਥਾਵਾਂ()
                            ? ""
                            : dereferenceString(fromVarRefp->self_ਸੂਚਕ_Protect(m_useSelfForThis)))
                     + varName(fromVarRefp->varp()) + ", \""
                     + V3_ਬਾਹਰਲਾ_Formatter::quoteNameControls(protect(nodep->fileline()->filename()))
                     + "\", " + std::ਮੰਜ਼ਿਲ_string(nodep->fileline()->lineno()) + ")->"),
                    memberVarp, resetp->constructing());
            } ਨਹੀਂਤਾਂ {
                Ast_ਗੰਢ_Var_Ref* ਅਚਲ fromVarRefp = VN_AS(fromp, ਗੰਢ_Var_Ref);
                AstVar* ਅਚਲ varp = fromVarRefp->varp();
                ਅਚਲ string ਅਗੇਤਰ
                    = fromVarRefp->self_ਸੂਚਕ().is_ਖਾਲੀ_ਥਾਵਾਂ()
                          ? ""
                          : dereferenceString(
                                VN_AS(fromp, ਗੰਢ_Var_Ref)->self_ਸੂਚਕ_Protect(m_useSelfForThis));
                emitVarReset(ਅਗੇਤਰ, varp, resetp->constructing());
            }
            ਵਾਪਸ;
        }
        bool paren = true;
        bool decind = false;
        bool rhs = true;
        bool ਉਲਟ_Unpack = false;  // Set for descending CvtPackedToArray
        ਅਚਲ AstUnpackArrayDType* ਅਚਲ unpackDtp
            = VN_CAST(nodep->dtypep()->skipRefp(), UnpackArrayDType);
        ਜੇ (AstSel* ਅਚਲ selp = VN_CAST(nodep->lhsp(), Sel)) {
            UASSERT_OBJ(selp->ਚੌੜਾਈ_Min() == selp->ਚੌੜਾਈ_Const(), selp, "Width mismatch");
            ਜੇ (selp->ਚੌੜਾਈ_Min() == 1) {
                putnbs(nodep, "VL_ASSIGNBIT_");
                emitIQW(selp->fromp());
                ਜੇ (nodep->rhsp()->isAllOnesV()) {
                    puts("O(");
                    rhs = false;
                } ਨਹੀਂਤਾਂ {
                    puts("I(");
                }
                iterate_And_ਅਗਲਾ_Const_Null(selp->lsbp());
                puts(", ");
                iterate_And_ਅਗਲਾ_Const_Null(selp->fromp());
                ਜੇ (rhs) puts(", ");
            } ਨਹੀਂਤਾਂ {
                putnbs(nodep, "VL_ASSIGNSEL_");
                emitIQW(selp->fromp());
                emitIQW(nodep->rhsp());
                puts("(");
                putns(selp->fromp(), cvt_ਮੰਜ਼ਿਲ_Str(selp->fromp()->ਚੌੜਾਈ_Min()) + ", ");
                puts(cvt_ਮੰਜ਼ਿਲ_Str(nodep->ਚੌੜਾਈ_Min()) + ", ");
                iterate_And_ਅਗਲਾ_Const_Null(selp->lsbp());
                puts(", ");
                iterate_And_ਅਗਲਾ_Const_Null(selp->fromp());
                puts(", ");
            }
        } ਨਹੀਂਤਾਂ ਜੇ (ਅਚਲ AstGetcRefN* ਅਚਲ selp = VN_CAST(nodep->lhsp(), GetcRefN)) {
            iterate_And_ਅਗਲਾ_Const_Null(selp->lhsp());
            puts(" = ");
            putnbs(selp, "VL_PUTC_N(");
            iterate_And_ਅਗਲਾ_Const_Null(selp->lhsp());
            puts(", ");
            iterate_And_ਅਗਲਾ_Const_Null(selp->rhsp());
            puts(", ");
        } ਨਹੀਂਤਾਂ ਜੇ (AstVar* ਅਚਲ varp = AstVar::scVarRecurse(nodep->lhsp())) {
            putnbs(varp, "VL_ASSIGN_");  // Set a systemC variable
            emitScIQW(varp);
            emitIQW(nodep);
            puts("(");
            puts(cvt_ਮੰਜ਼ਿਲ_Str(nodep->ਚੌੜਾਈ_Min()) + ", ");
            iterate_And_ਅਗਲਾ_Const_Null(nodep->lhsp());
            puts(", ");
        } ਨਹੀਂਤਾਂ ਜੇ (AstVar* ਅਚਲ varp = AstVar::scVarRecurse(nodep->rhsp())) {
            putnbs(varp, "VL_ASSIGN_");  // Get a systemC variable
            emitIQW(nodep);
            emitScIQW(varp);
            puts("(");
            puts(cvt_ਮੰਜ਼ਿਲ_Str(nodep->ਚੌੜਾਈ_Min()) + ", ");
            iterate_And_ਅਗਲਾ_Const_Null(nodep->lhsp());
            puts(", ");
        } ਨਹੀਂਤਾਂ ਜੇ (ਅਚਲ Ast_Cvt_Packed_ਮੰਜ਼ਿਲ_Array* ਅਚਲ castp
                   = VN_CAST(nodep->rhsp(), Cvt_Packed_ਮੰਜ਼ਿਲ_Array)) {
            putns(castp, "VL_UNPACK_");
            emitRU(nodep);
            emitIQW(nodep->dtypep()->subDTypep());
            puts("_");
            emitRU(castp->fromp());
            emitIQW(castp->fromp());
            puts("(");
            putns(castp->dtypep(), cvt_ਮੰਜ਼ਿਲ_Str(castp->dtypep()->subDTypep()->ਚੌੜਾਈ_Min()));
            puts(", ");
            puts(cvt_ਮੰਜ਼ਿਲ_Str(castp->fromp()->ਚੌੜਾਈ_Min()));
            puts(", ");
            iterate_And_ਅਗਲਾ_Const_Null(nodep->lhsp());
            puts(", ");
            rhs = false;
            iterate_And_ਅਗਲਾ_Const_Null(castp->fromp());
            // Descending unpacked dest: reverse after unpack
            ਜੇ (unpackDtp && !unpackDtp->declRange().ascending()) ਉਲਟ_Unpack = true;
        } ਨਹੀਂਤਾਂ ਜੇ (ਅਚਲ Ast_Cvt_Array_ਮੰਜ਼ਿਲ_Array* ਅਚਲ castp
                   = VN_CAST(nodep->rhsp(), Cvt_Array_ਮੰਜ਼ਿਲ_Array)) {
            ਜੇ (castp->ਉਲਟ()) {
                putns(castp, "VL_REVCOPY_Q(");
            } ਨਹੀਂਤਾਂ {
                putns(castp, "VL_COPY_Q(");
            }
            iterate_And_ਅਗਲਾ_Const_Null(nodep->lhsp());
            puts(", ");
            rhs = false;
            iterate_And_ਅਗਲਾ_Const_Null(castp->fromp());
            puts(", ");
            puts(cvt_ਮੰਜ਼ਿਲ_Str(castp->ਖੰਡ_ਮਾਪ()));
            puts(", ");
            puts(cvt_ਮੰਜ਼ਿਲ_Str(castp->src_Element_ਬਿੱਟ()));
            puts(", ");
            puts(cvt_ਮੰਜ਼ਿਲ_Str(castp->dst_Element_ਬਿੱਟ()));
        } ਨਹੀਂਤਾਂ ਜੇ (nodep->isWide() && VN_IS(nodep->lhsp(), VarRef)  //
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
                   && !VN_IS(nodep->rhsp(), ਵਿਅੰਜਕ_Stmt)) {
            // Wide functions assign into the array directly, don't need separate assign statement
            m_wide_ਆਰਜ਼ੀ_Refp = VN_AS(nodep->lhsp(), VarRef);
            paren = false;
        } ਨਹੀਂਤਾਂ ਜੇ (nodep->isWide() && !unpackDtp && !VN_IS(nodep->rhsp(), Const)) {
            putnbs(nodep, "VL_ASSIGN_W(");
            puts(cvt_ਮੰਜ਼ਿਲ_Str(nodep->ਚੌੜਾਈ_Min()) + ", ");
            iterate_And_ਅਗਲਾ_Const_Null(nodep->lhsp());
            puts(", ");
        } ਨਹੀਂਤਾਂ ਜੇ (VN_IS(nodep->lhsp()->dtypep()->skipRefp(), ਕਤਾਰ_DType)
                   && (VN_IS(nodep->rhsp(), ਧਾਰਾ_L) || VN_IS(nodep->lhsp(), ਧਾਰਾ_L)
                       || VN_IS(nodep->rhsp(), ਧਾਰਾ_R) || VN_IS(nodep->lhsp(), ਧਾਰਾ_R)
                       || VN_IS(nodep->rhsp(), ਧਾਰਾ_R))) {
            //if either side is streamL or streamR don't emit lhsp everything will be passed by
            //reference

            paren = false;

        } ਨਹੀਂਤਾਂ {
            paren = false;
            iterate_And_ਅਗਲਾ_Const_Null(nodep->lhsp());
            puts(" ");
            ofp()->ਖੰਡ_Inc();
            decind = true;
            ਜੇ (!VN_IS(nodep->rhsp(), Const)) ofp()->putBreak();
            putns(nodep, "= ");
        }
        ਜੇ (rhs) iterate_And_ਅਗਲਾ_Const_Null(nodep->rhsp());
        ਜੇ (paren) puts(")");
        ਜੇ (decind) ofp()->ਖੰਡ_Dec();
        puts(";\n");
        ਜੇ (ਉਲਟ_Unpack) {
            puts("VL_UNPACK_REVERSED(");
            iterate_And_ਅਗਲਾ_Const_Null(nodep->lhsp());
            puts(");\n");
        }
    }
    ਖਾਲੀ ਦੇਖੋ(AstAssocSel* nodep) override {
        iterate_And_ਅਗਲਾ_Const_Null(nodep->fromp());
        ਅਚਲ std::string atFunc = nodep->isLValue() ? ".atWrite(" : ".at(";
        putnbs(nodep, atFunc);
        AstAssocArrayDType* ਅਚਲ adtypep
            = VN_AS(nodep->fromp()->dtypep()->skipRefp(), AssocArrayDType);
        UASSERT_OBJ(adtypep, nodep, "Associative select on non-associative type");
        iterate_And_ਅਗਲਾ_Const_Null(nodep->bitp());
        puts(")");
    }
    ਖਾਲੀ ਦੇਖੋ(AstWildcardSel* nodep) override {
        iterate_And_ਅਗਲਾ_Const_Null(nodep->fromp());
        ਅਚਲ std::string atFunc = nodep->isLValue() ? ".atWrite(" : ".at(";
        putnbs(nodep, atFunc);
        AstWildcardArrayDType* ਅਚਲ adtypep
            = VN_AS(nodep->fromp()->dtypep()->skipRefp(), WildcardArrayDType);
        UASSERT_OBJ(adtypep, nodep, "Wildcard select on non-wildcard-associative type");
        iterate_And_ਅਗਲਾ_Const_Null(nodep->bitp());
        puts(")");
    }
    ਖਾਲੀ ਦੇਖੋ(AstCCall* nodep) override {
        ਅਚਲ AstCFunc* ਅਚਲ funcp = nodep->funcp();
        ਅਚਲ Ast_ਗੰਢ_Module* ਅਚਲ funcModp = EmitCParentModule::get(funcp);
        putnbs(nodep, "");
        ਜੇ (funcp->dpiImportPrototype()) {
            // Calling DPI import
            putns(nodep, funcp->name());
        } ਨਹੀਂਤਾਂ ਜੇ (funcp->is_Proper_ਵਿਧੀ() && funcp->isStatic()) {
            // Call static method via the containing class
            putns(funcModp, EmitCUtil::ਅਗੇਤਰ_Name_Protect(funcModp) + "::");
            putns(nodep, funcp->nameProtect());
        } ਨਹੀਂਤਾਂ ਜੇ (nodep->superReference()) {
            // Calling superclass method
            putns(funcModp, EmitCUtil::ਅਗੇਤਰ_Name_Protect(funcModp) + "::");
            putns(nodep, funcp->nameProtect());
        } ਨਹੀਂਤਾਂ ਜੇ (funcp->isLoose()) {
            // Calling loose method
            putns(nodep, funcNameProtect(funcp));
        } ਨਹੀਂਤਾਂ {
            // Calling regular method/function
            ਜੇ (!nodep->self_ਸੂਚਕ().is_ਖਾਲੀ_ਥਾਵਾਂ()) {
                emitDereference(nodep, nodep->self_ਸੂਚਕ_Protect(m_useSelfForThis));
            }
            putns(nodep, funcp->nameProtect());
        }
        emit_CCall_ਦਲੀਲਾਂ(nodep, nodep->self_ਸੂਚਕ_Protect(m_useSelfForThis), m_cfuncp->needProcess());
    }
    ਖਾਲੀ ਦੇਖੋ(AstCMethodCall* nodep) override {
        ਅਚਲ AstCFunc* ਅਚਲ funcp = nodep->funcp();
        UASSERT_OBJ(!funcp->isLoose(), nodep, "Loose method called via AstCMethodCall");
        iterateConst(nodep->fromp());
        putnbs(nodep, "->");
        ਜੇ (nodep->superReference()) {
            // Super call, so the base implementation is used rather than the override
            ਅਚਲ Ast_ਗੰਢ_Module* ਅਚਲ funcModp = EmitCParentModule::get(funcp);
            putns(funcModp, EmitCUtil::ਅਗੇਤਰ_Name_Protect(funcModp) + "::");
        }
        putns(funcp, funcp->nameProtect());
        emit_CCall_ਦਲੀਲਾਂ(nodep, "", m_cfuncp->needProcess());
    }
    ਖਾਲੀ ਦੇਖੋ(AstCAwait* nodep) override {
        putns(nodep, "co_await ");
        iterateConst(nodep->exprp());
        puts(";\n");
    }
    ਖਾਲੀ ਦੇਖੋ(AstCNew* nodep) override {
        ਜੇ (VN_IS(nodep->dtypep(), VoidDType)) {  // super.new case
            putsDecoration(nodep, "/*super.new*/");
            ਵਾਪਸ;
        }
        // assignment case;
        putns(nodep, "VL_NEW(" + EmitCUtil::ਅਗੇਤਰ_Name_Protect(nodep->dtypep()) + ", "
                         + optionalProcArg(nodep->dtypep()) + "vlSymsp");
        put_Comma_Iterate_ਅਗਲਾ(nodep->argsp(), true);
        puts(")");
    }
    ਖਾਲੀ ਦੇਖੋ(AstCMethodHard* nodep) override {
        iterateConst(nodep->fromp());
        putns(nodep, nodep->usePtr() ? "->" : ".");
        putns(nodep, nodep->name());
        ਜੇ (nodep->ਵਿਧੀ() == VCMethod::FORCE_READ_SEL) {
            emitIQW(nodep);
            ਜੇ (nodep->isWide()) puts("<" + cvt_ਮੰਜ਼ਿਲ_Str(nodep->dtypep()->ਚੌੜਾਈ_ਸ਼ਬਦ()) + ">");
        } ਨਹੀਂਤਾਂ ਜੇ (nodep->ਵਿਧੀ() == VCMethod::ARRAY_ਟੁਕੜਾ) {
            // VlUnpacked::slice<N_Out>(loIdx) - N_Out is the (fixed) result array size
            ਅਚਲ AstUnpackArrayDType* ਅਚਲ adtypep
                = VN_AS(nodep->dtypep()->skipRefp(), UnpackArrayDType);
            puts("<" + cvt_ਮੰਜ਼ਿਲ_Str(adtypep->elementsConst()) + ">");
        } ਨਹੀਂਤਾਂ ਜੇ (nodep->ਵਿਧੀ() == VCMethod::COVERGROUP_ਜੋੜੋ_COVERPOINT) {
            // The hit-list bound is a template argument of the returned VlCoverpointT<>, and the
            // node's own dtype is that type, so it is the one source of truth for both.
            ਅਚਲ AstCoverpointDType* ਅਚਲ cpdtypep
                = VN_AS(nodep->dtypep()->skipRefp(), CoverpointDType);
            puts("<" + cvt_ਮੰਜ਼ਿਲ_Str(cpdtypep->ਪ੍ਰਾਪਤੀ_Bound()) + ">");
        } ਨਹੀਂਤਾਂ ਜੇ (nodep->ਵਿਧੀ() == VCMethod::COVERGROUP_ਜੋੜੋ_CROSS) {
            ਅਚਲ AstCoverCrossDType* ਅਚਲ cxdtypep
                = VN_AS(nodep->dtypep()->skipRefp(), CoverCrossDType);
            puts("<" + cxdtypep->cpp_Template_ਦਲੀਲਾਂ() + ">");
        }
        puts("(");
        bool comma = false;
        ਪੂਰਨ arg_ਅੰਕ = 0;
        ਲਈ (Ast_ਗੰਢ* subnodep = nodep->pinsp(); subnodep; subnodep = subnodep->nextp()) {
            ਜੇ (comma) puts(", ");
            ਜੇ (nodep->ਵਿਧੀ() == VCMethod::RANDOMIZER_HARD && arg_ਅੰਕ == 1) {
                // For RANDOMIZER_HARD's filename argument (2nd arg after constraint),
                // apply protect() similar to VL_STOP to handle --protected flag
                ਜੇ (ਅਚਲ AstCExpr* ਅਚਲ cexprp = VN_CAST(subnodep, CExpr)) {
                    // Extract filename from the CExpr (which contains "filename")
                    std::string filename;
                    ਲਈ (ਅਚਲ Ast_ਗੰਢ* textnodep = cexprp->nodesp(); textnodep;
                         textnodep = textnodep->nextp()) {
                        ਜੇ (ਅਚਲ Ast_ਪਾਠ* ਅਚਲ textp = VN_CAST(textnodep, ਪਾਠ_2)) {
                            filename = textp->ਪਾਠ();
                            ਰੋਕੋ;
                        }
                    }
                    // Remove surrounding quotes if present
                    ਜੇ (filename.ਮਾਪ() >= 2 && filename.front() == '"'
                        && filename.ਪਿੱਠ() == '"') {
                        filename = filename.substr(1, filename.ਮਾਪ() - 2);
                    }
                    // Emit with protect()
                    putsQuoted(protect(filename));
                } ਨਹੀਂਤਾਂ {
                    iterateConst(subnodep);
                }
            } ਨਹੀਂਤਾਂ {
                iterateConst(subnodep);
            }
            comma = true;
            arg_ਅੰਕ++;
        }
        ਜੇ (nodep->withp()) {
            ਜੇ (comma) puts(", ");
            iterateConst(nodep->withp());
        }
        puts(")");
    }
    ਖਾਲੀ ਦੇਖੋ(AstCFuncHard* nodep) override {
        putns(nodep, nodep->name());
        puts("(");
        put_Comma_Iterate_ਅਗਲਾ(nodep->pinsp());
        puts(")");
    }
    ਖਾਲੀ ਦੇਖੋ(AstLambdaArgRef* nodep) override { putbs(nodep->nameProtect()); }
    ਖਾਲੀ ਦੇਖੋ(AstWith* nodep) override {
        // With uses a C++11 lambda
        putnbs(nodep, "[&](");
        ਜੇ (ਸਵੈਚਾਲਿਤ* ਅਚਲ argrefp = nodep->ਸੂਚਕਾਂਕ_Arg_Refp()) {
            putnbs(argrefp, argrefp->dtypep()->cType(argrefp->nameProtect(), false, false));
            puts(", ");
        }
        ਜੇ (ਸਵੈਚਾਲਿਤ* ਅਚਲ argrefp = nodep->ਮੁੱਲ_Arg_Refp()) {
            putnbs(argrefp, argrefp->dtypep()->cType(argrefp->nameProtect(), false, false));
        }
        puts(") -> ");
        putnbs(nodep, nodep->dtypep()->cType("", false, false));
        puts(" {\n");
        VL_RESTORER(m_createdScopeHash);
        iterate_And_ਅਗਲਾ_Const_Null(nodep->exprp());
        puts("}\n");
    }
    ਖਾਲੀ ਦੇਖੋ(AstCase* nodep) override {  // LCOV_EXCL_LINE
        // In V3Case...
        nodep->v3fatalSrc("Case statements should have been reduced out");
    }
    ਖਾਲੀ ਦੇਖੋ(AstComment* nodep) override {
        string at;
        ਜੇ (nodep->showAt()) {
            at = " at " + nodep->fileline()->ascii();
            // If protecting, passthru less information about the design
            ਜੇ (!v3Global.opt.protectIds()) ਵਾਪਸ;
        }
        ਜੇ (!(nodep->protect() && v3Global.opt.protectIds())) {
            putsDecoration(nodep, "// "s + nodep->name() + at + "\n");
        }
        iterateChildrenConst(nodep);
    }
    ਖਾਲੀ ਦੇਖੋ(AstCoverOtherDecl* nodep) override {
        emit_Cover_Other_Decl_ਪਾਓ(nodep);
        ਜੇ (coverage_ਵਰਤਦਾ_Local_Counter(nodep)) {
            // Some task bodies are cloned into a caller module where the
            // original instance-local counter is not addressable. Those clones
            // fall back to the global bin number, so register that slot too.
            emit_Cover_Other_Decl_ਪਾਓ(nodep, true);
        }
    }
    ਖਾਲੀ ਦੇਖੋ(AstCoverToggleDecl* nodep) override {
        putns(nodep, "vlSelf->__vlCoverToggleInsert(");  // As Declared in emitCoverageDecl
        puts(cvt_ਮੰਜ਼ਿਲ_Str(nodep->range().ਸੱਜਾ()));
        puts(", ");
        puts(cvt_ਮੰਜ਼ਿਲ_Str(nodep->range().ਖੱਬਾ()));
        puts(", ");
        puts(cvt_ਮੰਜ਼ਿਲ_Str(nodep->range().ranged()));
        puts(", ");
        putCoverageArray(nodep);
        puts(" + ");
        puts(cvt_ਮੰਜ਼ਿਲ_Str(coverage_Bin_ਅੰਕ(nodep)));
        // first controls whether duplicate module instances are collapsed in
        // the default coverage view. The following flag says whether countp
        // points to an object-local counter; only those counters must be kept
        // real for later instances so forcePerInstance can split hierarchy.
        puts(", first");  // Enable, passed from __Vconfigure parameter
        puts(", ");
        puts(coverage_ਵਰਤਦਾ_Local_Counter(nodep) ? "true" : "false");
        puts(", ");
        putsQuoted(protect(nodep->fileline()->filename()));
        puts(", ");
        puts(cvt_ਮੰਜ਼ਿਲ_Str(nodep->fileline()->lineno()));
        puts(", ");
        puts(cvt_ਮੰਜ਼ਿਲ_Str(nodep->fileline()->ਪਹਿਲਾ_ਕਾਲਮ()));
        puts(", ");
        putsQuoted((!nodep->hier().ਖਾਲੀ_ਥਾਵਾਂ() ? "." : "")
                   + VIdProtect::protect_ਸ਼ਬਦ_If(nodep->hier(), nodep->protect()));
        puts(", ");
        putsQuoted(VIdProtect::protect_ਸ਼ਬਦ_If(nodep->ਪੰਨਾ(), nodep->protect()));
        puts(", ");
        putsQuoted(VIdProtect::protect_ਸ਼ਬਦ_If(nodep->comment(), nodep->protect()));
        puts(");\n");
    }
    ਖਾਲੀ ਦੇਖੋ(AstCoverInc* nodep) override {
        ਜੇ (VN_IS(nodep->declp(), CoverOtherDecl)) {
            ਜੇ (v3Global.opt.ਧਾਗੇ() > 1) {
                putns(nodep, "");
                putCoverageArray(nodep->declp());
                puts("[");
                puts(cvt_ਮੰਜ਼ਿਲ_Str(coverage_Bin_ਅੰਕ(nodep->declp())));
                puts("].fetch_add(1, std::memory_order_relaxed);\n");
            } ਨਹੀਂਤਾਂ {
                putns(nodep, "++(");
                putCoverageArray(nodep->declp());
                puts("[");
                puts(cvt_ਮੰਜ਼ਿਲ_Str(coverage_Bin_ਅੰਕ(nodep->declp())));
                puts("]);\n");
            }
        } ਨਹੀਂਤਾਂ {
            puts("VL_COV_TOGGLE_CHG_");
            ਜੇ (v3Global.opt.ਧਾਗੇ() > 1) {
                puts("MT_");
            } ਨਹੀਂਤਾਂ {
                puts("ST_");
            }
            emitIQW(nodep->toggleExprp());
            puts("(");
            // Each bit of variable has 2 counters, so the size of variable is half of the size of
            // coverpoint
            puts(cvt_ਮੰਜ਼ਿਲ_Str(nodep->declp()->ਮਾਪ() / 2));
            puts(", ");
            // Toggle update uses the same object-local counter array that
            // __vlCoverToggleInsert registered.
            putCoverageArray(nodep->declp());
            puts(" + ");
            puts(cvt_ਮੰਜ਼ਿਲ_Str(coverage_Bin_ਅੰਕ(nodep->declp())));
            puts(", ");
            iterateConst(nodep->toggleExprp());
            puts(", ");
            iterateConst(nodep->toggleCovExprp());
            puts(");\n");
        }
    }
    ਖਾਲੀ ਦੇਖੋ(AstDisableFork* nodep) override { putns(nodep, "vlProcess->disableFork();\n"); }
    ਖਾਲੀ ਦੇਖੋ(AstCReturn* nodep) override {
        putns(nodep, "return (");
        iterate_And_ਅਗਲਾ_Const_Null(nodep->lhsp());
        puts(");\n");
    }
    ਖਾਲੀ ਦੇਖੋ(AstDisplay* nodep) override {
        string ਪਾਠ = nodep->fmtp()->ਪਾਠ();
        ਜੇ (nodep->ਜੋੜੋ_Newline()) ਪਾਠ += "\n";
        display_ਗੰਢ(nodep, nodep->fmtp(), ਪਾਠ, nodep->fmtp()->exprsp(), false);
    }
    ਖਾਲੀ ਦੇਖੋ(AstDumpCtl* nodep) override {
        ਚੋਣ (nodep->ctlType()) {
        ਹਾਲਤ VDumpCtlType::FILE:
            putns(nodep, "vlSymsp->_vm_contextp__->dumpfile(");
            iterateConst(nodep->exprp());
            puts(");\n");
            ਰੋਕੋ;
        ਹਾਲਤ VDumpCtlType::VARS:
            // We ignore number of levels to dump in exprp()
            ਜੇ (v3Global.opt.trace()) {
                putns(nodep, "vlSymsp->_traceDumpOpen();\n");
            } ਨਹੀਂਤਾਂ {
                putns(nodep, "VL_PRINTF_MT(\"-Info: ");
                puts(V3_ਬਾਹਰਲਾ_Formatter::quoteNameControls(protect(nodep->fileline()->filename())));
                puts(":");
                puts(cvt_ਮੰਜ਼ਿਲ_Str(nodep->fileline()->lineno()));
                puts(": $dumpvar ignored, as Verilated without --trace");
                puts("\\n\");\n");
            }
            ਰੋਕੋ;
        ਹਾਲਤ VDumpCtlType::ALL:
            // $dumpall currently ignored
            ਰੋਕੋ;
        ਹਾਲਤ VDumpCtlType::FLUSH:
            // $dumpall currently ignored; would need rework of VCD single thread,
            // or flag we pass-through to next eval() iteration
            ਰੋਕੋ;
        ਹਾਲਤ VDumpCtlType::ਹੱਦਬੰਦੀ_2:
            // $dumplimit currently ignored
            ਰੋਕੋ;
        ਹਾਲਤ VDumpCtlType::OFF:
            // Currently ignored as both Vcd and Fst do not support them, as would need "X" dump
            ਰੋਕੋ;
        ਹਾਲਤ VDumpCtlType::ON:
            // Currently ignored as $dumpoff is also ignored
            ਰੋਕੋ;
        ਮੂਲ: nodep->v3fatalSrc("Bad case, unexpected " << nodep->ctlType().ascii());
        }
    }
    ਖਾਲੀ ਦੇਖੋ(AstScopeName* nodep) override {
        // For use under AstCCalls for dpiImports.  ScopeNames under
        // displays are handled in AstDisplay
        ਜੇ (!nodep->dpiExport()) {
            // this is where the DPI import context scope is set
            ਅਚਲ string scope = nodep->scopeDpiName();
            putnbs(nodep, "(vlSymsp->" + protect("__Vscopep_" + scope) + ")");
        }
    }
    ਖਾਲੀ ਦੇਖੋ(AstSFormat* nodep) override {
        display_ਗੰਢ(nodep, nodep->fmtp(), nodep->fmtp()->ਪਾਠ(), nodep->fmtp()->exprsp(), false);
    }
    ਖਾਲੀ ਦੇਖੋ(AstSFormatF* nodep) override {
        display_ਗੰਢ(nodep, nodep, nodep->ਪਾਠ(), nodep->exprsp(), false);
    }
    ਖਾਲੀ ਦੇਖੋ(AstFScanF* nodep) override {
        display_ਗੰਢ(nodep, nullptr, nodep->ਪਾਠ(), nodep->exprsp(), true);
    }
    ਖਾਲੀ ਦੇਖੋ(AstSScanF* nodep) override {
        display_ਗੰਢ(nodep, nullptr, nodep->ਪਾਠ(), nodep->exprsp(), true);
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_ਮੁੱਲ_Plus_ਦਲੀਲਾਂ* nodep) override {
        putns(nodep, "VL_VALUEPLUSARGS_IN");
        emitIQW(nodep->outp());
        puts("(");
        puts(cvt_ਮੰਜ਼ਿਲ_Str(nodep->outp()->ਚੌੜਾਈ_Min()));
        puts(", ");
        iterateConst(nodep->searchp());
        puts(", ");
        putbs("");
        iterate_And_ਅਗਲਾ_Const_Null(nodep->outp());
        puts(")");
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_Test_Plus_ਦਲੀਲਾਂ* nodep) override {
        putns(nodep, "VL_TESTPLUSARGS_I(");
        iterateConst(nodep->searchp());
        puts(")");
    }
    ਖਾਲੀ ਦੇਖੋ(AstFError* nodep) override {
        putns(nodep, "VL_FERROR_I");
        puts(nodep->strp()->isString() ? "N(" : "W(");
        iterate_And_ਅਗਲਾ_Const_Null(nodep->filep());
        putbs(", ");
        ਜੇ (nodep->strp()->isWide()) {
            puts(cvt_ਮੰਜ਼ਿਲ_Str(nodep->strp()->ਚੌੜਾਈ_ਸ਼ਬਦ()));
            putbs(", ");
        }
        iterate_And_ਅਗਲਾ_Const_Null(nodep->strp());
        puts(")");
    }
    ਖਾਲੀ ਦੇਖੋ(AstFGetS* nodep) override {
        emitOpName(nodep, nodep->emitC(), nodep->strgp(), nodep->filep(), nullptr);
    }
    ਖਾਲੀ ਦੇਖੋ(AstFOpen* nodep) override {
        putns(nodep, "VL_FOPEN_NN(");
        iterateConst(nodep->filenamep());
        putbs(", ");
        iterateConst(nodep->modep());
        puts(");\n");
    }
    ਖਾਲੀ ਦੇਖੋ(AstFOpenMcd* nodep) override {
        putns(nodep, "VL_FOPEN_MCD_N(");
        iterateConst(nodep->filenamep());
        puts(");\n");
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ_Read_Write_Mem* nodep) override {
        putns(nodep, nodep->cFuncPrefixp());
        puts("N(");
        puts(nodep->isHex() ? "true" : "false");
        putbs(", ");
        // Need real storage width
        puts(cvt_ਮੰਜ਼ਿਲ_Str(nodep->memp()->dtypep()->skipRefp()->subDTypep()->ਚੌੜਾਈ_Min()));
        uint32_t array_lo = 0;
        bool memUsePtr = true;
        {
            ਅਚਲ AstVarRef* ਅਚਲ varrefp = VN_CAST(nodep->memp(), VarRef);
            ਜੇ (!varrefp) {
                nodep->v3error(nodep->verilogKwd() << " loading non-variable");
            } ਨਹੀਂਤਾਂ ਜੇ (VN_IS(varrefp->varp()->dtypeSkipRefp(), AssocArrayDType)
                       || VN_IS(varrefp->varp()->dtypeSkipRefp(), DynArrayDType)) {
                // nodep->memp() below will when verilated code is compiled create a C++ template
                memUsePtr = false;
            } ਨਹੀਂਤਾਂ ਜੇ (ਅਚਲ AstUnpackArrayDType* ਅਚਲ adtypep
                       = VN_CAST(varrefp->varp()->dtypeSkipRefp(), UnpackArrayDType)) {
                putbs(", ");
                puts(cvt_ਮੰਜ਼ਿਲ_Str(varrefp->varp()->dtypep()->arrayUnpackedElements()));
                array_lo = adtypep->lo();
                putbs(", ");
                puts(cvt_ਮੰਜ਼ਿਲ_Str(array_lo));
            } ਨਹੀਂਤਾਂ {
                nodep->v3error(
                    nodep->verilogKwd()
                    << " loading other than unpacked/dynamic/associative-array variable");
            }
        }
        putbs(", ");
        iterateConst(nodep->filenamep());
        putbs(", ");
        {
            ਜੇ (memUsePtr) puts(" &(");
            iterate_And_ਅਗਲਾ_Const_Null(nodep->memp());
            ਜੇ (memUsePtr) puts(")");
        }
        putbs(", ");
        ਜੇ (nodep->lsbp()) {
            iterate_And_ਅਗਲਾ_Const_Null(nodep->lsbp());
        } ਨਹੀਂਤਾਂ {
            puts(cvt_ਮੰਜ਼ਿਲ_Str(array_lo));
        }
        putbs(", ");
        ਜੇ (nodep->msbp()) {
            iterate_And_ਅਗਲਾ_Const_Null(nodep->msbp());
        } ਨਹੀਂਤਾਂ {
            puts("~0ULL");
        }
        puts(");\n");
    }
    ਖਾਲੀ ਦੇਖੋ(AstFClose* nodep) override {
        putns(nodep, "VL_FCLOSE_I(");
        iterate_And_ਅਗਲਾ_Const_Null(nodep->filep());
        puts("); ");
    }
    ਖਾਲੀ ਦੇਖੋ(AstFFlush* nodep) override {
        ਜੇ (!nodep->filep()) {
            putns(nodep, "VL_FFLUSH_MT();");
        } ਨਹੀਂਤਾਂ {
            putns(nodep, "VL_FFLUSH_I(");
            iterate_And_ਅਗਲਾ_Const_Null(nodep->filep());
            puts(");\n");
        }
    }
    ਖਾਲੀ ਦੇਖੋ(AstFSeek* nodep) override {
        putns(nodep, "(VL_FSEEK_I(");
        iterate_And_ਅਗਲਾ_Const_Null(nodep->filep());
        puts(", ");
        iterate_And_ਅਗਲਾ_Const_Null(nodep->ਵਿੱਥ());
        puts(", ");
        iterate_And_ਅਗਲਾ_Const_Null(nodep->operation());
        puts(") == -1 ? -1 : 0)");
    }
    ਖਾਲੀ ਦੇਖੋ(AstFTell* nodep) override {
        putns(nodep, "VL_FTELL_I(");
        iterate_And_ਅਗਲਾ_Const_Null(nodep->filep());
        puts(")");
    }
    ਖਾਲੀ ਦੇਖੋ(AstFRewind* nodep) override {
        putns(nodep, "(VL_FSEEK_I(");
        iterate_And_ਅਗਲਾ_Const_Null(nodep->filep());
        puts(", 0, 0) == -1 ? -1 : 0)");
    }
    ਖਾਲੀ ਦੇਖੋ(AstFRead* nodep) override {
        putns(nodep, "VL_FREAD_I(");
        puts(cvt_ਮੰਜ਼ਿਲ_Str(nodep->memp()->ਚੌੜਾਈ_Min()));  // Need real storage width
        putbs(", ");
        uint32_t array_lo = 0;
        uint32_t array_ਮਾਪ = 0;
        {
            ਅਚਲ AstVarRef* ਅਚਲ varrefp = VN_CAST(nodep->memp(), VarRef);
            ਜੇ (!varrefp) {
                nodep->v3error(nodep->verilogKwd() << " loading non-variable");
            } ਨਹੀਂਤਾਂ ਜੇ (VN_CAST(varrefp->varp()->dtypeSkipRefp(), BasicDType)) {
            } ਨਹੀਂਤਾਂ ਜੇ (ਅਚਲ AstUnpackArrayDType* ਅਚਲ adtypep
                       = VN_CAST(varrefp->varp()->dtypeSkipRefp(), UnpackArrayDType)) {
                array_lo = adtypep->lo();
                array_ਮਾਪ = adtypep->elementsConst();
            } ਨਹੀਂਤਾਂ {
                nodep->v3error(nodep->verilogKwd()
                               << " loading other than unpacked-array variable");
            }
        }
        puts(cvt_ਮੰਜ਼ਿਲ_Str(array_lo));
        putbs(", ");
        puts(cvt_ਮੰਜ਼ਿਲ_Str(array_ਮਾਪ));
        putbs(", ");
        puts("&(");
        iterate_And_ਅਗਲਾ_Const_Null(nodep->memp());
        puts(")");
        putbs(", ");
        iterate_And_ਅਗਲਾ_Const_Null(nodep->filep());
        putbs(", ");
        ਜੇ (nodep->startp()) {
            iterate_And_ਅਗਲਾ_Const_Null(nodep->startp());
        } ਨਹੀਂਤਾਂ {
            puts(cvt_ਮੰਜ਼ਿਲ_Str(array_lo));
        }
        putbs(", ");
        ਜੇ (nodep->countp()) {
            iterate_And_ਅਗਲਾ_Const_Null(nodep->countp());
        } ਨਹੀਂਤਾਂ {
            puts(cvt_ਮੰਜ਼ਿਲ_Str(array_ਮਾਪ));
        }
        puts(")");
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_ਢੇਰ_Trace_F* nodep) override { putns(nodep, "VL_STACKTRACE_N()"); }
    ਖਾਲੀ ਦੇਖੋ(Ast_ਢੇਰ_Trace_T* nodep) override { putns(nodep, "VL_STACKTRACE();\n"); }
    ਖਾਲੀ ਦੇਖੋ(AstSystemT* nodep) override {
        putns(nodep, "(void)VL_SYSTEM_I");
        emitIQW(nodep->lhsp());
        puts("(");
        ਜੇ (nodep->lhsp()->isWide()) {
            puts(cvt_ਮੰਜ਼ਿਲ_Str(nodep->lhsp()->ਚੌੜਾਈ_ਸ਼ਬਦ()));
            putbs(", ");
        }
        iterate_And_ਅਗਲਾ_Const_Null(nodep->lhsp());
        puts(");\n");
    }
    ਖਾਲੀ ਦੇਖੋ(AstSystemF* nodep) override {
        putns(nodep, "VL_SYSTEM_I");
        emitIQW(nodep->lhsp());
        puts("(");
        ਜੇ (nodep->lhsp()->isWide()) {
            puts(cvt_ਮੰਜ਼ਿਲ_Str(nodep->lhsp()->ਚੌੜਾਈ_ਸ਼ਬਦ()));
            putbs(", ");
        }
        iterate_And_ਅਗਲਾ_Const_Null(nodep->lhsp());
        puts(")");
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_Stmt_ਵਿਅੰਜਕ* nodep) override {
        ਜੇ (VN_IS(nodep->exprp()->dtypep()->skipRefp(), VoidDType)) {
            putns(nodep, "");
        } ਨਹੀਂਤਾਂ {
            putns(nodep, "std::ignore = ");
        }
        iterateConst(nodep->exprp());
        puts(";\n");
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_Jump_ਖੰਡ* nodep) override {
        // Allocate label number
        ਅਚਲ ਮਾਪ_t n = m_labelNumbers.ਮਾਪ();
        ਅਚਲ bool new_ਇੰਦਰਾਜ = m_labelNumbers.emplace(nodep, n).ਦੂਜਾ;
        UASSERT_OBJ(new_ਇੰਦਰਾਜ, nodep, "AstJumpBlock visited twide");
        // Emit
        putns(nodep, "{\n");  // Make it visually obvious label jumps outside these
        VL_RESTORER(m_createdScopeHash);
        iterate_And_ਅਗਲਾ_Const_Null(nodep->stmtsp());
        puts("__Vlabel" + std::ਮੰਜ਼ਿਲ_string(n) + ": ;\n");
        puts("}\n");
    }
    ਖਾਲੀ ਦੇਖੋ(AstJumpGo* nodep) override {
        // Retrieve target label number - must already exist (from enclosing AstJumpBlock)
        ਅਚਲ ਮਾਪ_t n = m_labelNumbers.at(nodep->blockp());
        // Emit
        putns(nodep, "goto __Vlabel" + std::ਮੰਜ਼ਿਲ_string(n) + ";\n");
    }
    ਖਾਲੀ ਦੇਖੋ(AstLoop* nodep) override {
        UASSERT_OBJ(!nodep->contsp(), nodep, "'contsp' only used before LinkJump");
        VL_RESTORER(m_createdScopeHash);
        // Special case when the AstLoopTest is first for output readability
        ਜੇ (AstLoopTest* ਅਚਲ testp = VN_CAST(nodep->stmtsp(), LoopTest)) {
            putns(nodep, "while (");
            iterateConst(testp->condp());
            puts(") {\n");
            iterate_And_ਅਗਲਾ_Const_Null(testp->nextp());
            puts("}\n");
            ਵਾਪਸ;
        }
        // Special case when the AstLoopTest is last for output readability
        ਜੇ (Ast_ਗੰਢ* lastp = nodep->stmtsp()) {
            ਜਦੋਂ (Ast_ਗੰਢ* ਅਚਲ nextp = lastp->nextp()) lastp = nextp;
            ਜੇ (AstLoopTest* ਅਚਲ testp = VN_CAST(lastp, LoopTest)) {
                putns(nodep, "do {\n");
                ਲਈ (Ast_ਗੰਢ* p = nodep->stmtsp(); p != lastp; p = p->nextp()) iterateConst(p);
                puts("} while (");
                iterateConst(testp->condp());
                puts(");\n");
                ਵਾਪਸ;
            }
        }
        // Emit generic case directly
        putns(nodep, "while (true) {\n");
        iterate_And_ਅਗਲਾ_Const_Null(nodep->stmtsp());
        puts("}\n");
    }
    ਖਾਲੀ ਦੇਖੋ(AstLoopTest* nodep) override {
        VL_RESTORER(m_createdScopeHash);
        putns(nodep, "if (!(");
        iterate_And_ਅਗਲਾ_Const_Null(nodep->condp());
        puts(")) break;\n");
    }
    ਖਾਲੀ ਦੇਖੋ(AstCLocalScope* nodep) override {
        putns(nodep, "{\n");
        VL_RESTORER(m_createdScopeHash);
        iterate_And_ਅਗਲਾ_Const_Null(nodep->stmtsp());
        puts("}\n");
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ_If* nodep) override {
        putns(nodep, "if (");
        ਜੇ (!nodep->branchPred().unknown()) {
            puts(nodep->branchPred().ascii());
            puts("((");  // Two parens, so that VL_UNLIKELY((class<foo,bar>)) works
        }
        iterate_And_ਅਗਲਾ_Const_Null(nodep->condp());
        ਜੇ (!nodep->branchPred().unknown()) puts("))");
        puts(") {\n");
        {
            VL_RESTORER(m_createdScopeHash);
            iterate_And_ਅਗਲਾ_Const_Null(nodep->thensp());
        }
        puts("}");
        ਜੇ (!nodep->elsesp()) {
            puts("\n");
        } ਨਹੀਂਤਾਂ {
            ਜੇ (VN_IS(nodep->elsesp(), ਗੰਢ_If) && !nodep->elsesp()->nextp()) {
                puts(" else ");
                iterate_And_ਅਗਲਾ_Const_Null(nodep->elsesp());
            } ਨਹੀਂਤਾਂ {
                VL_RESTORER(m_createdScopeHash);
                puts(" else {\n");
                iterate_And_ਅਗਲਾ_Const_Null(nodep->elsesp());
                puts("}\n");
            }
        }
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_ਵਿਅੰਜਕ_Stmt* nodep) override {
        VL_RESTORER(m_createdScopeHash);
        ਅਚਲ bool ਸ਼ਾਮਲ_ਹੈ_Await = nodep->exists([](AstCAwait*) -> bool { ਵਾਪਸ true; });
        ਜੇ (ਸ਼ਾਮਲ_ਹੈ_Await) {
            UASSERT_OBJ(m_cfuncp && m_cfuncp->isCoroutine(), nodep,
                        "AstExprStmt with CAwait must be in coroutine");
            putnbs(nodep, "(co_await ([&]() -> VlCoroutine {\n");
            iterate_And_ਅਗਲਾ_Const_Null(nodep->stmtsp());
            puts("co_return;\n");
            ਜੇ (!nodep->has_ਨਤੀਜਾ()) {
                puts("}()))");
                ਵਾਪਸ;
            }
            puts("}()), ");
            iterate_And_ਅਗਲਾ_Const_Null(nodep->resultp());
            puts(")");
            ਵਾਪਸ;
        }
        // GCC allows compound statements in expressions, but this is not standard.
        // So we use an immediate-evaluation lambda and comma operator
        putnbs(nodep, "([&]() {\n");
        ਜੇ (!nodep->has_ਨਤੀਜਾ()) {
            iterate_And_ਅਗਲਾ_Const_Null(nodep->stmtsp());
            puts("}())");
            ਵਾਪਸ;
        }
        iterate_And_ਅਗਲਾ_Const_Null(nodep->stmtsp());
        puts("}(), ");
        iterate_And_ਅਗਲਾ_Const_Null(nodep->resultp());
        puts(")");
    }
    ਖਾਲੀ ਦੇਖੋ(AstStop* nodep) override {
        putns(nodep, "VL_STOP_MT(");
        putsQuoted(protect(nodep->fileline()->filename()));
        puts(", ");
        puts(cvt_ਮੰਜ਼ਿਲ_Str(nodep->fileline()->lineno()));
        puts(", \"\"");
        ਜੇ (nodep->isFatal()) puts(", false");
        puts(");\n");
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_ਅੰਤ* nodep) override {
        // Disable all the forks so they don't operate after simulation is finished.
        ਜੇ (m_cfuncp && m_cfuncp->needProcess()) putns(nodep, "vlProcess->disableFork();\n");
        putns(nodep, "VL_FINISH_MT(");
        putsQuoted(protect(nodep->fileline()->filename()));
        puts(", ");
        puts(cvt_ਮੰਜ਼ਿਲ_Str(nodep->fileline()->lineno()));
        puts(", \"\");\n");
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_ਅੰਤ_Fork* nodep) override {
        // Disable all the forks so they don't operate after simulation is finished.
        ਜੇ (m_cfuncp && m_cfuncp->needProcess()) putns(nodep, "vlProcess->disableFork();\n");
        putns(nodep, "VL_FINISH_MT(");
        putsQuoted(protect(nodep->fileline()->filename()));
        puts(", ");
        puts(cvt_ਮੰਜ਼ਿਲ_Str(nodep->fileline()->lineno()));
        puts(", \"\");\n");
        ਜੇ (m_cfuncp->isCoroutine()) {
            putns(nodep, "co_return;\n");
        } ਨਹੀਂਤਾਂ {
            putns(nodep, "return;\n");
        }
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_Print_ਸਮਾਂ_Scale* nodep) override {
        putns(nodep, "VL_PRINTTIMESCALE(");
        putsQuoted(protect(nodep->prettyName()));
        puts(", ");
        putsQuoted(nodep->timeunit().ascii());
        puts(", vlSymsp->_vm_contextp__);\n");
    }
    ਖਾਲੀ ਦੇਖੋ(AstRand* nodep) override {
        emitOpName(nodep, nodep->emitC(), nodep->seedp(), nullptr, nullptr);
    }
    ਖਾਲੀ ਦੇਖੋ(AstRandRNG* nodep) override {
        emitOpName(nodep, nodep->emitC(), nullptr, nullptr, nullptr);
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_ਸਮਾਂ* nodep) override {
        putns(nodep, "VL_TIME_UNITED_Q(");
        UASSERT_OBJ(!nodep->timeunit().isNone(), nodep, "$time has no units");
        ਅਚਲ ਦੁੱਗਣਾ ਸਮਾਂ_2
            = nodep->timeunit().multiplier() / v3Global.rootp()->timeprecision().multiplier();
        UASSERT_OBJ(ਸਮਾਂ_2 >= 1, nodep, "TimeQ is less than 1, will result in division by zero");
        puts(cvt_ਮੰਜ਼ਿਲ_Str(ਸਮਾਂ_2));
        puts(")");
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_ਸਮਾਂ_D* nodep) override {
        putns(nodep, "VL_TIME_UNITED_D(");
        UASSERT_OBJ(!nodep->timeunit().isNone(), nodep, "$realtime has no units");
        puts(cvt_ਮੰਜ਼ਿਲ_Str(nodep->timeunit().multiplier()
                      / v3Global.rootp()->timeprecision().multiplier()));
        puts(")");
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_Get_Initial_Random_ਬੀਜ* nodep) override {
        putns(nodep, "vlSymsp->_vm_contextp__->randSeed()");
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_ਸਮਾਂ_Format* nodep) override {
        putns(nodep, "VL_TIMEFORMAT_IINI(");
        ਜੇ (nodep->unitsp()) {
            puts("true, ");
            iterate_And_ਅਗਲਾ_Const_Null(nodep->unitsp());
        } ਨਹੀਂਤਾਂ {
            puts("false, 0");
        }
        puts(", ");
        ਜੇ (nodep->precisionp()) {
            puts("true, ");
            iterate_And_ਅਗਲਾ_Const_Null(nodep->precisionp());
        } ਨਹੀਂਤਾਂ {
            puts("false, 0");
        }
        puts(", ");
        ਜੇ (nodep->suffixp()) {
            puts("true, ");
            iterate_And_ਅਗਲਾ_Const_Null(nodep->suffixp());
        } ਨਹੀਂਤਾਂ {
            puts("false, \"\"");
        }
        puts(", ");
        ਜੇ (nodep->widthp()) {
            puts("true, ");
            iterate_And_ਅਗਲਾ_Const_Null(nodep->widthp());
        } ਨਹੀਂਤਾਂ {
            puts("false, 0");
        }
        puts(", vlSymsp->_vm_contextp__);\n");
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_ਸਮਾਂ_Precision* nodep) override {
        putns(nodep, "vlSymsp->_vm_contextp__->timeprecision()");
    }

    // Nodes involing AstText
    ਖਾਲੀ ਦੇਖੋ(Ast_ਪਾਠ* nodep) override {
        // All Text should be under TextBlock/CStmt/CStmtUser/CExpr/CExprUser
        nodep->v3fatalSrc("Text node in unexpected position");
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_ਪਾਠ_ਖੰਡ* nodep) override {
        putnbs(nodep, "");
        puts(nodep->ਅਗੇਤਰ());
        emit_Nodes_With_ਪਾਠ(nodep->nodesp(), false, true, nodep->separator());
        puts(nodep->suffix());
    }
    ਖਾਲੀ ਦੇਖੋ(AstCStmt* nodep) override {
        putnbs(nodep, "");
        emit_Nodes_With_ਪਾਠ(nodep->nodesp(), false, true, "");
        ensure_New_ਸਤਰ();
    }
    ਖਾਲੀ ਦੇਖੋ(AstCExpr* nodep) override {
        putnbs(nodep, "");
        emit_Nodes_With_ਪਾਠ(nodep->nodesp(), false, true, "");
    }
    ਖਾਲੀ ਦੇਖੋ(AstCStmtUser* nodep) override {
        putnbs(nodep, "");
        ofp()->putsNoTracking("\n");
        ਜੇ (nodep->fromDollarC() && v3Global.opt.decoration() && !v3Global.opt.protectIds()) {
            ofp()->putsNoTracking("// $c statement at " + nodep->fileline()->ascii() + "\n");
        }
        emit_Nodes_With_ਪਾਠ(nodep->nodesp(), m_useSelfForThis, false, "");
        puts("\n");
    }
    ਖਾਲੀ ਦੇਖੋ(AstCExprUser* nodep) override {
        putnbs(nodep, "");
        ofp()->putsNoTracking("\n");
        ਜੇ (/* is always from $c */ v3Global.opt.decoration() && !v3Global.opt.protectIds()) {
            ofp()->putsNoTracking(
                (nodep->is_ਸ਼ੁੱਧ() ? "// $cpure expression at " : "// $c expression at ")
                + nodep->fileline()->ascii() + "\n");
        }
        emit_Nodes_With_ਪਾਠ(nodep->nodesp(), m_useSelfForThis, false, "");
        puts("\n");
    }

    // Operators
    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ_Termop* nodep) override {
        emitOpName(nodep, nodep->emitC(), nullptr, nullptr, nullptr);
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ_Uniop* nodep) override {
        ਜੇ (nodep->emit_Check_Max_ਸ਼ਬਦ()
            && (nodep->ਚੌੜਾਈ_ਸ਼ਬਦ() > VL_MULS_MAX_ਸ਼ਬਦ
                || nodep->lhsp()->ਚੌੜਾਈ_ਸ਼ਬਦ() > VL_MULS_MAX_ਸ਼ਬਦ)) {
            nodep->v3warn(
                E_UNSUPPORTED,
                "Unsupported: "
                    << nodep->prettyOperatorName() << " operator of " << nodep->ਚੌੜਾਈ()
                    << " bits exceeds hardcoded limit VL_MULS_MAX_WORDS in verilatedos.h");
        }
        ਜੇ (emitSimpleOk(nodep)) {
            putnbs(nodep, "(");
            puts(nodep->emitSimpleOperator());
            puts(" ");
            iterate_And_ਅਗਲਾ_Const_Null(nodep->lhsp());
            puts(")");
        } ਨਹੀਂਤਾਂ {
            emitOpName(nodep, nodep->emitC(), nodep->lhsp(), nullptr, nullptr);
        }
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ_Biop* nodep) override {
        ਜੇ (nodep->emit_Check_Max_ਸ਼ਬਦ() && nodep->ਚੌੜਾਈ_ਸ਼ਬਦ() > VL_MULS_MAX_ਸ਼ਬਦ) {
            nodep->v3warn(
                E_UNSUPPORTED,
                "Unsupported: "
                    << nodep->prettyOperatorName() << " operator of " << nodep->ਚੌੜਾਈ()
                    << " bits exceeds hardcoded limit VL_MULS_MAX_WORDS in verilatedos.h");
        }
        ਜੇ (emitSimpleOk(nodep)) {
            putnbs(nodep, "(");
            iterate_And_ਅਗਲਾ_Const_Null(nodep->lhsp());
            puts(" ");
            putbs(nodep->emitSimpleOperator());
            puts(" ");
            iterate_And_ਅਗਲਾ_Const_Null(nodep->rhsp());
            puts(")");
        } ਨਹੀਂਤਾਂ {
            emitOpName(nodep, nodep->emitC(), nodep->lhsp(), nodep->rhsp(), nullptr);
        }
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ_Triop* nodep) override {
        UASSERT_OBJ(!emitSimpleOk(nodep), nodep, "Triop cannot be described in a simple way");
        emitOpName(nodep, nodep->emitC(), nodep->lhsp(), nodep->rhsp(), nodep->thsp());
    }
    ਖਾਲੀ ਦੇਖੋ(AstCvtPackString* nodep) override { emitCvtPackStr(nodep->lhsp()); }
    ਖਾਲੀ ਦੇਖੋ(AstRedXor* nodep) override {
        ਜੇ (nodep->lhsp()->isWide()) {
            ਦੇਖੋ(static_cast<Ast_ਗੰਢ_Uniop*>(nodep));
        } ਨਹੀਂਤਾਂ {
            ਅਚਲ AstVarRef* ਅਚਲ vrefp = VN_CAST(nodep->lhsp(), VarRef);
            ਅਚਲ ਪੂਰਨ ਚੌੜਾਈ_Pow2 = vrefp ? vrefp->varp()->dtypep()->ਚੌੜਾਈ_Pow2()
                                        : nodep->lhsp()->dtypep()->ਚੌੜਾਈ_Pow2();
            UASSERT_OBJ(ਚੌੜਾਈ_Pow2 > 1, nodep,
                        "Reduction over single bit value should have been folded");
            putnbs(nodep, "VL_REDXOR_");
            puts(cvt_ਮੰਜ਼ਿਲ_Str(ਚੌੜਾਈ_Pow2));
            puts("(");
            iterate_And_ਅਗਲਾ_Const_Null(nodep->lhsp());
            puts(")");
        }
    }
    ਖਾਲੀ ਦੇਖੋ(AstCCast* nodep) override {
        // Extending a value of the same word width is just a NOP.
        ਜੇ (ਅਚਲ AstClassRefDType* ਅਚਲ classDtypep
            = VN_CAST(nodep->dtypep()->skipRefp(), ClassRefDType)) {
            putns(nodep, "(" + classDtypep->cType("", false, false) + ")(");
        } ਨਹੀਂਤਾਂ ਜੇ (nodep->ਮਾਪ() <= VL_BYTESIZE) {
            putns(nodep, "(::CData)(");
        } ਨਹੀਂਤਾਂ ਜੇ (nodep->ਮਾਪ() <= VL_SHORTSIZE) {
            putns(nodep, "(::SData)(");
        } ਨਹੀਂਤਾਂ ਜੇ (nodep->ਮਾਪ() <= VL_IDATASIZE) {
            putns(nodep, "(::IData)(");
        } ਨਹੀਂਤਾਂ {
            putns(nodep, "(::QData)(");
        }
        iterate_And_ਅਗਲਾ_Const_Null(nodep->lhsp());
        puts(")");
    }
    ਖਾਲੀ ਦੇਖੋ(AstCond* nodep) override {
        // Widths match up already, so we'll just use C++'s operator w/o any temps.
        ਜੇ (nodep->thenp()->isWide()) {
            emitOpName(nodep, nodep->emitC(), nodep->condp(), nodep->thenp(), nodep->elsep());
        } ਨਹੀਂਤਾਂ {
            putnbs(nodep, "(");
            iterate_And_ਅਗਲਾ_Const_Null(nodep->condp());
            putbs(" ? ");
            iterate_And_ਅਗਲਾ_Const_Null(nodep->thenp());
            putbs(" : ");
            iterate_And_ਅਗਲਾ_Const_Null(nodep->elsep());
            puts(")");
        }
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_ਮੇਲ_Masked* nodep) override {
        emitOpName(nodep, nodep->emitC(), nodep->lhsp(), nodep->matchp(), nullptr);
    }
    ਖਾਲੀ ਦੇਖੋ(AstMemberSel* nodep) override {
        ਅਚਲ AstVar* ਅਚਲ varp = nodep->varp();
        ਅਚਲ bool dereferenceCovergroupRef
            = varp->covergroupRefMember() && nodep->access().isReadOrRW();
        ਜੇ (dereferenceCovergroupRef) putnbs(nodep, "(*");
        iterate_And_ਅਗਲਾ_Const_Null(nodep->fromp());
        putnbs(nodep, "->");
        ਜੇ (varp->isIfaceRef()) {
            // varp is the __Viftop companion (e.g. "tx__Viftop"); use the
            // MemberSel name which matches the cell's C++ member (e.g. "tx").
            puts(nodep->nameProtect());
        } ਨਹੀਂਤਾਂ {
            puts(varName(varp));
        }
        ਜੇ (dereferenceCovergroupRef) puts(")");
    }
    ਖਾਲੀ ਦੇਖੋ(AstStructSel* nodep) override {
        iterate_And_ਅਗਲਾ_Const_Null(nodep->fromp());
        putnbs(nodep, ".");
        puts(nodep->nameProtect());
    }
    ਖਾਲੀ ਦੇਖੋ(AstNullCheck* nodep) override {
        putns(nodep, "VL_NULL_CHECK(");
        iterate_And_ਅਗਲਾ_Const_Null(nodep->lhsp());
        puts(", ");
        putsQuoted(protect(nodep->fileline()->filename()));
        puts(", ");
        puts(cvt_ਮੰਜ਼ਿਲ_Str(nodep->fileline()->lineno()));
        puts(")");
    }
    ਖਾਲੀ ਦੇਖੋ(AstNewCopy* nodep) override {
        // Polymorphic shallow clone: preserves runtime type via virtual clone()
        // VL_NULL_CHECK enforces null check per IEEE 1800-2023 8.7
        putns(nodep, "VL_NULL_CHECK(");
        ਜੇ (VN_IS(nodep->rhsp(), Const) && VN_AS(nodep->rhsp(), Const)->isNull()) {
            // V3Const folded rhs to null: emit a typed empty ref so VL_NULL_CHECK fires
            ਅਚਲ AstClassRefDType* ਅਚਲ refDTypep
                = VN_CAST(nodep->dtypep()->skipRefp(), ClassRefDType);
            puts(refDTypep->cType("", false, false) + "{}");
        } ਨਹੀਂਤਾਂ {
            iterate_And_ਅਗਲਾ_Const_Null(nodep->rhsp());
        }
        puts(", ");
        putsQuoted(protect(nodep->fileline()->filename()));
        puts(", ");
        puts(cvt_ਮੰਜ਼ਿਲ_Str(nodep->fileline()->lineno()));
        puts(").clone(vlSymsp->__Vm_deleter)");
    }
    ਖਾਲੀ ਦੇਖੋ(AstSel* nodep) override {
        // Note ASSIGN checks for this on a LHS
        UASSERT_OBJ(nodep->ਚੌੜਾਈ_Min() == nodep->ਚੌੜਾਈ_Const(), nodep, "Width mismatch");
        emitOpName(nodep, nodep->emitC(), nodep->fromp(), nodep->lsbp(), nullptr);
    }
    ਖਾਲੀ ਦੇਖੋ(AstReplicate* nodep) override {
        ਜੇ (nodep->srcp()->ਚੌੜਾਈ_Min() == 1 && !nodep->isWide()) {
            UASSERT_OBJ((static_cast<ਪੂਰਨ>(VN_AS(nodep->countp(), Const)->ਮੰਜ਼ਿਲ_UInt())
                         * nodep->srcp()->ਚੌੜਾਈ_Min())
                            == nodep->ਚੌੜਾਈ_Min(),
                        nodep, "Replicate non-constant or width miscomputed");
            putns(nodep, "VL_REPLICATE_");
            emitIQW(nodep);
            puts("OI(");
            ਜੇ (nodep->srcp()) puts(cvt_ਮੰਜ਼ਿਲ_Str(nodep->srcp()->ਚੌੜਾਈ_Min()));
            puts(", ");
            iterate_And_ਅਗਲਾ_Const_Null(nodep->srcp());
            puts(", ");
            iterate_And_ਅਗਲਾ_Const_Null(nodep->countp());
            puts(")");
        } ਨਹੀਂਤਾਂ {
            emitOpName(nodep, nodep->emitC(), nodep->srcp(), nodep->countp(), nullptr);
        }
    }
    ਖਾਲੀ emit_ਧਾਰਾ_R(Ast_ਧਾਰਾ_R* nodep, Ast_ਗੰਢ* ਮਾਪਾ) {
        //TODO: This might need to handle more cases like the visit(AstStreamR) function
        emitOpName(nodep, nodep->emitC(), nodep->lhsp(), nodep->rhsp(), nullptr);
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_ਧਾਰਾ_R* nodep) override {
        //The parrent node of our AstStreamR will give just enough info for what streamR should
        //output if nodep->backp() is not the parent then emitStreamR should have been used. throw
        //an error
        bool backp_Is_ਮਾਪਾ = (nodep->backp()->op1p() == nodep || nodep->backp()->op2p() == nodep);
        UASSERT(backp_Is_ਮਾਪਾ, "can not find return type for streamR");
        ਜੇ (ਅਚਲ Ast_ਕਤਾਰ_DType* ਅਚਲ qdtp
            = VN_CAST(nodep->backp()->dtypep()->skipRefp(), ਕਤਾਰ_DType)) {
            ਜੇ (nodep->lhsp()->isWide()) {
                emitOpName(nodep,
                           "VL_STREAMR_%nq%lq%rq(%lw," + std::ਮੰਜ਼ਿਲ_string(qdtp->subDTypep()->ਚੌੜਾਈ())
                               + ", %P, %li, %ri)",
                           nodep->lhsp(), nodep->rhsp(), nullptr);
            } ਨਹੀਂਤਾਂ {
                emitOpName(nodep, "VL_STREAMR_%nq%lq%rq(%lw, %P, %li, %ri)", nodep->lhsp(),
                           nodep->rhsp(), nullptr);
            }
        } ਨਹੀਂਤਾਂ ਜੇ (VN_IS(nodep->lhsp()->dtypep()->skipRefp(), ਕਤਾਰ_DType)) {
            ਜੇ (!((nodep->backp()->op1p() && nodep->backp()->op1p()->isWide())
                  || (nodep->backp()->op2p() && nodep->backp()->op2p()->isWide()))) {
                //If our lhsp is a queue make sure we streamR and return the correct type.
                //If either side is wide or the previous node is string type dont use this case
                emitOpName(nodep->backp(), "VL_STREAMR_%nq%lq%rq(%lw, %P, %li, %ri)",
                           nodep->lhsp(), nodep->rhsp(), nullptr);
            } ਨਹੀਂਤਾਂ {
                emitOpName(nodep, nodep->emitC(), nodep->lhsp(), nodep->rhsp(), nullptr);
            }
        } ਨਹੀਂਤਾਂ {
            emitOpName(nodep, nodep->emitC(), nodep->lhsp(), nodep->rhsp(), nullptr);
        }
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_ਧਾਰਾ_L* nodep) override {
        // Attempt to use a "fast" stream function for slice size = power of 2
        ਜੇ (!nodep->isWide()) {
            ਅਚਲ uint32_t isPow2 = VN_AS(nodep->rhsp(), Const)->ਅੰਕ().ਗਿਣਤੀ_Ones() == 1;
            ਅਚਲ uint32_t ਟੁਕੜਾ_ਮਾਪ = VN_AS(nodep->rhsp(), Const)->ਮੰਜ਼ਿਲ_UInt();
            ਜੇ (isPow2 && ਟੁਕੜਾ_ਮਾਪ <= (nodep->isQuad() ? ਆਕਾਰ(uint64_t) : ਆਕਾਰ(uint32_t))) {
                putns(nodep, "VL_STREAML_FAST_");
                bool ਵਰਤਦਾ_ਕਤਾਰ = false;
                Ast_ਕਤਾਰ_DType* qtypep
                    = nodep->backp()->op2p()
                          ? VN_CAST(nodep->backp()->op2p()->dtypep()->skipRefp(), ਕਤਾਰ_DType)
                          : nullptr;
                ਜੇ (VN_IS(nodep->backp(), ਸੌਂਪੋ)
                    && qtypep) {  // If we are assigning to a queue then emit the correct symbol
                    puts("R");  // R for queue
                    ਵਰਤਦਾ_ਕਤਾਰ = true;
                } ਨਹੀਂਤਾਂ {
                    emitIQW(nodep);
                }
                emitIQW(nodep->lhsp());
                puts("I(");
                puts(cvt_ਮੰਜ਼ਿਲ_Str(nodep->lhsp()->ਚੌੜਾਈ_Min()));
                puts(", ");
                ਜੇ (ਵਰਤਦਾ_ਕਤਾਰ) {
                    iterate_And_ਅਗਲਾ_Const_Null(nodep->backp()->op2p());
                    puts(", ");
                }
                iterate_And_ਅਗਲਾ_Const_Null(nodep->lhsp());
                puts(", ");
                ਅਚਲ uint32_t rd_log2 = V3_ਸੰਖਿਆ::log2b(VN_AS(nodep->rhsp(), Const)->ਮੰਜ਼ਿਲ_UInt());
                puts(cvt_ਮੰਜ਼ਿਲ_Str(rd_log2) + ")");
                ਵਾਪਸ;
            }
        }
        ਜੇ (VN_IS(nodep->backp(), ਸੌਂਪੋ)
            && VN_IS(nodep->backp()->op2p()->dtypep()->skipRefp(), ਕਤਾਰ_DType)) {
            ਪੂਰਨ ਕਤਾਰ_ਚੌੜਾਈ
                = nodep->backp()->op2p()->dtypep()->subDTypep()->ਚੌੜਾਈ();  //We need to know the
                                                                           //width of both sides
            emitOpName(nodep,
                       "VL_STREAML_%nq%lq%rq(%lw," + std::ਮੰਜ਼ਿਲ_string(ਕਤਾਰ_ਚੌੜਾਈ)
                           + ", %P, %li, %ri)",
                       nodep->lhsp(), nodep->rhsp(), nullptr);
        } ਨਹੀਂਤਾਂ {
            emitOpName(nodep, nodep->emitC(), nodep->lhsp(), nodep->rhsp(), nullptr);
        }
    }
    ਖਾਲੀ ਦੇਖੋ(AstCastDynamic* nodep) override {
        putnbs(nodep, "VL_CAST_DYNAMIC(");
        iterate_And_ਅਗਲਾ_Const_Null(nodep->fromp());
        puts(", ");
        iterate_And_ਅਗਲਾ_Const_Null(nodep->ਸਿਖਰ());
        puts(")");
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_ਗਿਣਤੀ_ਬਿੱਟ* nodep) override {
        putnbs(nodep, "VL_COUNTBITS_");
        emitIQW(nodep->lhsp());
        puts("(");
        puts(cvt_ਮੰਜ਼ਿਲ_Str(nodep->lhsp()->ਚੌੜਾਈ_Min()));
        puts(", ");
        ਜੇ (nodep->lhsp()->isWide()) {
            puts(cvt_ਮੰਜ਼ਿਲ_Str(nodep->lhsp()->ਚੌੜਾਈ_ਸ਼ਬਦ()));  // Note argument width, not node width
                                                          // (which is always 32)
            puts(", ");
        }
        iterate_And_ਅਗਲਾ_Const_Null(nodep->lhsp());
        puts(", ");
        iterate_And_ਅਗਲਾ_Const_Null(nodep->rhsp());
        puts(", ");
        iterate_And_ਅਗਲਾ_Const_Null(nodep->thsp());
        puts(", ");
        iterate_And_ਅਗਲਾ_Const_Null(nodep->fhsp());
        puts(")");
    }
    ਖਾਲੀ ਦੇਖੋ(AstTriggered* nodep) override {
        // Test each word of the trigger vector with a mask of the relevant bits
        std::map<uint32_t, uint64_t> ਸ਼ਬਦ_Masks;
        ਲਈ (ਅਚਲ uint32_t ਸੂਚਕਾਂਕ : nodep->indices()) {
            ਸ਼ਬਦ_Masks[VL_BITWORD_Q(ਸੂਚਕਾਂਕ)] |= VL_ਦੋਆਧਾਰੀ_ਅੰਕ_Q(ਸੂਚਕਾਂਕ);
        }
        putns(nodep, "((");
        bool ਪਹਿਲਾ = true;
        ਲਈ (ਅਚਲ ਸਵੈਚਾਲਿਤ& ਸ਼ਬਦ_ਨਕਾਬ : ਸ਼ਬਦ_Masks) {
            ਜੇ (!ਪਹਿਲਾ) puts("\n| ");
            ਪਹਿਲਾ = false;
            ਅਚਲ bool allOnes = ਸ਼ਬਦ_ਨਕਾਬ.ਦੂਜਾ == ~0ULL;
            ਜੇ (!allOnes) puts("(" + V3_ਸੰਖਿਆ{nodep, 64}.ਸੈੱਟ_Quad(ਸ਼ਬਦ_ਨਕਾਬ.ਦੂਜਾ).emitC() + " & ");
            iterateConst(nodep->trigRefp());
            puts("[" + std::ਮੰਜ਼ਿਲ_string(ਸ਼ਬਦ_ਨਕਾਬ.ਪਹਿਲਾ) + "U]");
            ਜੇ (!allOnes) puts(")");
        }
        puts(") != 0ULL)");
    }
    // Terminals
    ਖਾਲੀ ਦੇਖੋ(AstVarRef* nodep) override {
        ਅਚਲ AstVar* ਅਚਲ varp = nodep->varp();

        // Reference to constant pool variable is special
        ਜੇ (varp->const_Pool_ਇੰਦਰਾਜ()) {
            putns(nodep, EmitCUtil::constPoolName(varp));
            ਵਾਪਸ;
        }

        ਅਚਲ Ast_ਗੰਢ_Module* ਅਚਲ varModp = EmitCParentModule::get(varp);
        ਅਚਲ bool dereferenceCovergroupRef
            = varp->covergroupRefMember() && nodep->access().isReadOrRW();
        ਜੇ (dereferenceCovergroupRef) putns(nodep, "(*");
        ਜੇ (VN_IS(varModp, Class) && varModp != m_modp) {
            // Superclass member reference
            putns(nodep, EmitCUtil::ਅਗੇਤਰ_Name_Protect(varModp) + "::");
        } ਨਹੀਂਤਾਂ ਜੇ (varp->isIfaceRef()) {
            putns(nodep, nodep->self_ਸੂਚਕ_Protect(m_useSelfForThis));
            ਵਾਪਸ;
        } ਨਹੀਂਤਾਂ ਜੇ (!nodep->self_ਸੂਚਕ().is_ਖਾਲੀ_ਥਾਵਾਂ()) {
            emitDereference(nodep, nodep->self_ਸੂਚਕ_Protect(m_useSelfForThis));
        }
        putns(nodep, varName(varp));
        ਜੇ (dereferenceCovergroupRef) puts(")");
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_Addr_ਦਾ_CFunc* nodep) override {
        // Note: Can be thought to handle more, but this is all that is needed right now
        ਅਚਲ AstCFunc* ਅਚਲ funcp = nodep->funcp();
        UASSERT_OBJ(funcp->isLoose(), nodep, "Cannot take address of non-loose method");
        putns(nodep, "&");
        puts(funcNameProtect(funcp));
    }
    ਖਾਲੀ ਦੇਖੋ(AstConst* nodep) override {  //
        ਜੇ (m_wide_ਆਰਜ਼ੀ_Refp && nodep->isWide()) {
            UASSERT_OBJ(m_wide_ਆਰਜ਼ੀ_Refp, nodep, "Wide Constant w/ no temp");
            emitConstantW(nodep, m_wide_ਆਰਜ਼ੀ_Refp);
            m_wide_ਆਰਜ਼ੀ_Refp = nullptr;  // We used it, fail if set it a second time
        } ਨਹੀਂਤਾਂ {
            emitConstant(nodep);
        }
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ_Cons* nodep) override { emitValCons(nodep); }
    ਖਾਲੀ ਦੇਖੋ(AstThisRef* nodep) override {
        putnbs(nodep, nodep->dtypep()->cType("", false, false));
        puts("{");
        puts(VSelf_ਸੂਚਕ_ਪਾਠ::replaceThis(m_useSelfForThis, "this"));
        puts("}");
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ_Sel* nodep) override {
        ਜੇ (!VN_IS(nodep, ArraySel) && !VN_IS(nodep, ਸ਼ਬਦ_Sel)) {
            ਦੇਖੋ(static_cast<Ast_ਗੰਢ_Biop*>(nodep));
            ਵਾਪਸ;
        }
        // ArraySel or WordSel
        iterate_And_ਅਗਲਾ_Const_Null(nodep->fromp());
        // Special case constant index for readability
        ਜੇ (AstConst* ਅਚਲ idxp = VN_CAST(nodep->bitp(), Const)) {
            puts("[" + std::ਮੰਜ਼ਿਲ_string(idxp->ਮੰਜ਼ਿਲ_UInt()) + "U]");
            ਵਾਪਸ;
        }
        putbs("[");
        iterate_And_ਅਗਲਾ_Const_Null(nodep->bitp());
        puts("]");
    }

    //
    ਖਾਲੀ ਦੇਖੋ(Ast_Exec_ਗ੍ਰਾਫ* nodep) override {
        // The location of the AstExecGraph within the containing AstCFunc is where we want to
        // invoke the graph and wait for it to complete. The run-time library does just that.
        putns(nodep, "vl_invokeExecGraph(" + EmitCUtil::exec_ਗ੍ਰਾਫ_Name(nodep) + ", vlSelf, ");
        iterateConst(nodep->triggerp());
        puts(".data());\n");
    }

    // Default
    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ* nodep) override {  // LCOV_EXCL_START
        putns(nodep, "\n???? // "s + nodep->prettyTypeName() + "\n");
        iterateChildrenConst(nodep);
        ਜੇ (!v3Global.opt.lintOnly()) {  // An internal problem, so suppress
            nodep->v3fatalSrc("Unknown node type reached emitter: " << nodep->prettyTypeName());
        }
    }  // LCOV_EXCL_STOP

ਸੁਰੱਖਿਅਤ:
    EmitCFunc() = ਮੂਲ;
    ~EmitCFunc() override = ਮੂਲ;
};

#ਜੇ_ਅੰਤ  // guard
