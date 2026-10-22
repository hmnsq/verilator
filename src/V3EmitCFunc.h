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
constexpr ಪೂರ್ಣಾಙ್ಕ EMITC_ಅಂಕೆ_CONSTW = 8;

//######################################################################
// Emit lazy forward declarations

class EmitCLazyDecls final : ಸಾರ್ವಜನಿಕ VNVisitorConst {
    // NODE STATE/TYPES
    // None allowed to support threaded emitting

    // MEMBERS
    std::unordered_set<string> m_emittedManually;  // Set of names already declared manually.
    EmitCBaseVisitorConst& m_emitter;  // For access to file output
    bool m_needs_Blank_ಸಾಲು = false;  // Emit blank line if any declarations were emitted (cosmetic)
    std::set<Ast_ಗಂಟು*> m_emitted;  // -> in set. Already emitted decl for symbols.

    // METHODS
    bool declaredOnce(Ast_ಗಂಟು* nodep) { ಪ್ರತ್ಯಾಗಮ m_emitted.ಸೇರಿಸು(nodep).second; }

    ಶೂನ್ಯ lazyDeclare(AstCFunc* funcp) {
        // Already declared in this compilation unit
        ಯದಿ (!declaredOnce(funcp)) ಪ್ರತ್ಯಾಗಮ;
        // Check if this kind of function is lazily declared
        ಯದಿ (!(funcp->isMethod() && funcp->isLoose()) && !funcp->dpiImportPrototype()) ಪ್ರತ್ಯಾಗಮ;
        // Already declared manually
        ಯದಿ (m_emittedManually.ಎಣಿಕೆ(funcp->nameProtect())) ಪ್ರತ್ಯಾಗಮ;
        // Needs lazy declaration, emit one
        m_emitter.emitCFuncDecl(funcp, EmitCParentModule::get(funcp), funcp->dpiImportPrototype());
        m_needs_Blank_ಸಾಲು = true;
    }

    ಶೂನ್ಯ lazyDeclareConstPoolVar(AstVar* varp) {
        ಯದಿ (!declaredOnce(varp)) ಪ್ರತ್ಯಾಗಮ;  // Already declared
        ಧ್ರುವ string nameProtect = EmitCUtil::constPoolName(varp);
        m_emitter.putns(varp, "extern const ");
        m_emitter.puts(varp->dtypep()->cType(nameProtect, false, false));
        m_emitter.puts(";\n");
        m_needs_Blank_ಸಾಲು = true;
    }

    // VISITORS
    ಶೂನ್ಯ visit(Ast_ಗಂಟು_CCall* nodep) override {
        lazyDeclare(nodep->funcp());
        iterateChildrenConst(nodep);
    }

    ಶೂನ್ಯ visit(AstAddrOfCFunc* nodep) override {
        lazyDeclare(nodep->funcp());
        iterateChildrenConst(nodep);
    }

    ಶೂನ್ಯ visit(AstVarRef* nodep) override {
        AstVar* ಧ್ರುವ varp = nodep->varp();
        // Only constant pool symbols are lazy declared for now ...
        ಯದಿ (EmitCParentModule::get(varp)->isConstPool()) lazyDeclareConstPoolVar(varp);
    }

    ಶೂನ್ಯ visit(AstExecGraph* nodep) override {
        ಯದಿ (!declaredOnce(nodep)) ಪ್ರತ್ಯಾಗಮ;
        m_emitter.putns(nodep, "extern VlExecGraph " + EmitCUtil::execGraphName(nodep) + ";\n");
        m_needs_Blank_ಸಾಲು = true;
    }

    ಶೂನ್ಯ visit(Ast_ಗಂಟು* nodep) override { iterateChildrenConst(nodep); }

ಸಾರ್ವಜನಿಕ:
    explicit EmitCLazyDecls(EmitCBaseVisitorConst& emitter)
        : m_emitter{emitter} {}
    ಶೂನ್ಯ emit(Ast_ಗಂಟು* nodep) {
        m_needs_Blank_ಸಾಲು = false;
        iterateChildrenConst(nodep);
        ಯದಿ (m_needs_Blank_ಸಾಲು) m_emitter.puts("\n");
    }
    ಶೂನ್ಯ emit(ಧ್ರುವ string& prefix, ಧ್ರುವ string& name, ಧ್ರುವ string& suffix) {
        m_emittedManually.ಸೇರಿಸು(name);
        m_emitter.ensure_New_ಸಾಲು();
        m_emitter.puts(prefix);
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
    AstVarRef* m_wideTempRefp = nullptr;  // Variable that _WW macros should be setting
    std::unordered_map<AstJumpBlock*, size_t> m_labelNumbers;  // Label numbers for AstJumpBlocks
    bool m_createdScopeHash = false;  // Already created a scope hash

ರಕ್ಷಿತ:
    VL_DEFINE_DEBUG_FUNCTIONS;

    EmitCLazyDecls m_lazyDecls{*this};  // Visitor for emitting lazy declarations
    bool m_useSelfForThis = false;  // Replace "this" with "vlSelf"
    bool m_usevlSelfRef = false;  // Use vlSelfRef reference instead of vlSelf pointer
    ಧ್ರುವ Ast_ಗಂಟು_Module* m_modp = nullptr;  // Current module being emitted
    ಧ್ರುವ AstCFunc* m_cfuncp = nullptr;  // Current function being emitted
    bool m_instantiatesOwnProcess = false;
    ಧ್ರುವ AstClassPackage* m_classOrPackage = nullptr;  // Pointer to current class or package
    string m_classOrPackageHash;  // Hash of class or package name

    bool constructorNeedsProcess(ಧ್ರುವ AstClass* ಧ್ರುವ classp) {
        ಧ್ರುವ Ast_ಗಂಟು* ಧ್ರುವ newp = m_memberMap.findMember(classp, "new");
        ಯದಿ (!newp) ಪ್ರತ್ಯಾಗಮ false;
        ಧ್ರುವ AstCFunc* ಧ್ರುವ ctorp = VN_CAST(newp, CFunc);
        ಯದಿ (!ctorp) ಪ್ರತ್ಯಾಗಮ false;
        UASSERT_OBJ(ctorp->isConstructor(), ctorp, "`new` is not a constructor!");
        ಪ್ರತ್ಯಾಗಮ ctorp->needProcess();
    }

    bool constructorNeedsProcess(ಧ್ರುವ Ast_ಗಂಟು_DType* ಧ್ರುವ dtypep) {
        ಯದಿ (ಧ್ರುವ AstClassRefDType* ಧ್ರುವ crefdtypep = VN_CAST(dtypep, ClassRefDType))
            ಪ್ರತ್ಯಾಗಮ constructorNeedsProcess(crefdtypep->classp());
        ಪ್ರತ್ಯಾಗಮ false;
    }

    bool coverageUsesLocalCounter(Ast_ಗಂಟು_Cover_Decl* ಧ್ರುವ declp) ಧ್ರುವ {
        // Only functions with an object receiver can address the module-local
        // coverage array. Static/package/class helper functions still use the
        // symbol-table array, as they may have vlSymsp but no vlSelf/this.
        // Also require the coverage declaration to belong to the current
        // module; cloned task bodies can increment declarations owned by a
        // different module, and those must keep using the global array.
        ಪ್ರತ್ಯಾಗಮ m_cfuncp && !m_cfuncp->isStatic() && !VN_IS(m_modp, Class)
               && !VN_IS(m_modp, ClassPackage)
               && EmitCParentModule::get(declp->dataDeclThisp()) == m_modp;
    }
    ಪೂರ್ಣಾಙ್ಕ coverage_Bin_ಅಂಕೆ(Ast_ಗಂಟು_Cover_Decl* ಧ್ರುವ declp, bool forceGlobal = false) ಧ್ರುವ {
        ಪ್ರತ್ಯಾಗಮ !forceGlobal && coverageUsesLocalCounter(declp)
                   ? declp->dataDeclThisp()->local_Bin_ಅಂಕೆ()
                   : declp->dataDeclThisp()->bin_ಅಂಕೆ();
    }
    ಶೂನ್ಯ putCoverageArray(Ast_ಗಂಟು_Cover_Decl* ಧ್ರುವ declp, bool forceGlobal = false) {
        ಯದಿ (!forceGlobal && coverageUsesLocalCounter(declp)) {
            // Keep counters on the module object when possible, so every
            // no-inline instance has storage available if coverage output is
            // later written with forcePerInstance.
            puts(m_cfuncp->isLoose() ? "vlSelf->__Vcoverage" : "this->__Vcoverage");
        } ಅನ್ಯಥಾ {
            // Static/package/class helper functions do not have vlSelf, so
            // keep using the shared symbol-table coverage array there.
            puts("vlSymsp->__Vcoverage");
        }
    }
    ಶೂನ್ಯ emit_Cover_Other_Decl_ಸೇರಿಸು(AstCoverOtherDecl* nodep, bool forceGlobal = false) {
        putns(nodep, "vlSelf->__vlCoverInsert(");  // As Declared in emitCoverageDecl
        putCoverageArray(nodep, forceGlobal);
        puts(" + ");
        puts(cvtToStr(coverage_Bin_ಅಂಕೆ(nodep, forceGlobal)));
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
                   + VIdProtect::protect_ಪದಗಳು_If(nodep->hier(), nodep->protect()));
        puts(", ");
        putsQuoted(VIdProtect::protect_ಪದಗಳು_If(nodep->page(), nodep->protect()));
        puts(", ");
        putsQuoted(VIdProtect::protect_ಪದಗಳು_If(nodep->comment(), nodep->protect()));
        puts(", ");
        putsQuoted(nodep->linescov());
        puts(", ");
        putsQuoted(VIdProtect::protect_ಪದಗಳು_If(nodep->fsmVar(), nodep->protect()));
        puts(", ");
        putsQuoted(VIdProtect::protect_ಪದಗಳು_If(nodep->fsmFrom(), nodep->protect()));
        puts(", ");
        putsQuoted(VIdProtect::protect_ಪದಗಳು_If(nodep->fsmTo(), nodep->protect()));
        puts(", ");
        putsQuoted(VIdProtect::protect_ಪದಗಳು_If(nodep->fsmTag(), nodep->protect()));
        puts(");\n");
    }

ಸಾರ್ವಜನಿಕ:
    // METHODS
    bool displayEmitHeader(Ast_ಗಂಟು* nodep);
    ಶೂನ್ಯ display_ಗಂಟು(Ast_ಗಂಟು* nodep, AstSFormatF* fmtp, ಧ್ರುವ string& vformat, Ast_ಗಂಟು* exprsp,
                     bool isScan);

    bool emitSimpleOk(Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* nodep);
    ಶೂನ್ಯ emitIQW(ಧ್ರುವ Ast_ಗಂಟು* nodep) {
        // See "Type letters" in verilated.h
        // Other abbrevs: "C"har, "S"hort, "F"loat, "D"ouble, stri"N"g, "R"=queue, "U"npacked
        puts(nodep->dtypep()->skipRefp()->charIQWN());
    }
    ಶೂನ್ಯ emitRU(ಧ್ರುವ Ast_ಗಂಟು* nodep) {
        ಧ್ರುವ Ast_ಗಂಟು_DType* dtp = nodep->dtypep()->skipRefp();
        // See "Type letters" in verilated.h
        ಯದಿ (VN_IS(dtp, UnpackArrayDType))
            puts("U");
        ಅನ್ಯಥಾ ಯದಿ (VN_IS(dtp, QueueDType) || VN_IS(dtp, DynArrayDType))
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
    ಶೂನ್ಯ emitDatap(ಧ್ರುವ Ast_ಗಂಟು* nodep) {
        // When passing to a function with va_args the compiler doesn't
        // know need a pointer so when wide, need to look inside VlWide
        ಯದಿ (nodep->isWide()) puts(".data()");
    }
    ಶೂನ್ಯ emitOpName(Ast_ಗಂಟು* nodep, ಧ್ರುವ string& format, Ast_ಗಂಟು* lhsp, Ast_ಗಂಟು* rhsp,
                    Ast_ಗಂಟು* thsp);
    ಶೂನ್ಯ emitCCallArgs(ಧ್ರುವ Ast_ಗಂಟು_CCall* nodep, ಧ್ರುವ string& selfPointer, bool inProcess);
    ಶೂನ್ಯ emitDereference(Ast_ಗಂಟು* nodep, ಧ್ರುವ string& pointer);
    std::string dereferenceString(ಧ್ರುವ std::string& pointer) ಧ್ರುವ;
    ಶೂನ್ಯ emitCvtPackStr(Ast_ಗಂಟು* nodep);
    ಶೂನ್ಯ emitConstant(AstConst* nodep);
    ಶೂನ್ಯ emitConstantString(ಧ್ರುವ AstConst* nodep);
    ಶೂನ್ಯ emitVarReset(ಧ್ರುವ string& prefix, AstVar* varp, bool constructing);
    string emitVarResetRecurse(ಧ್ರುವ AstVar* varp, bool constructing,
                               ಧ್ರುವ string& varNameProtected, Ast_ಗಂಟು_DType* dtypep, ಪೂರ್ಣಾಙ್ಕ depth,
                               ಧ್ರುವ string& suffix, ಧ್ರುವ Ast_ಗಂಟು* valuep);
    ಶೂನ್ಯ emitVarResetScopeHash();
    ಶೂನ್ಯ emitChangeDet();
    ಶೂನ್ಯ putCommaIterateNext(Ast_ಗಂಟು* nodep, bool comma = false) {
        ಪ್ರತಿ (Ast_ಗಂಟು* subnodep = nodep; subnodep; subnodep = subnodep->nextp()) {
            ಯದಿ (comma) puts(", ");
            iterateConst(subnodep);
            comma = true;
        }
    }
    template <typename T>
    string optionalProcArg(ಧ್ರುವ T* ಧ್ರುವ nodep) {
        ಪ್ರತ್ಯಾಗಮ (nodep && constructorNeedsProcess(nodep)) ? "vlProcess, " : "";
    }
    ಧ್ರುವ AstCNew* getSuperNewCallRecursep(Ast_ಗಂಟು* ಧ್ರುವ nodep) {
        // Get the super.new call
        ಯದಿ (!nodep) ಪ್ರತ್ಯಾಗಮ nullptr;
        ಯದಿ (ಧ್ರುವ AstCNew* ಧ್ರುವ cnewp = VN_CAST(nodep, CNew)) ಪ್ರತ್ಯಾಗಮ cnewp;
        ಯದಿ (ಧ್ರುವ Ast_Stmt_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ stmtp = VN_CAST(nodep, Stmt_ಅಭಿವ್ಯಕ್ತಿ)) {
            ಯದಿ (ಧ್ರುವ AstCNew* ಧ್ರುವ cnewp = VN_CAST(stmtp->exprp(), CNew)) ಪ್ರತ್ಯಾಗಮ cnewp;
        }
        ಯದಿ (ಧ್ರುವ AstJumpBlock* ಧ್ರುವ blockp = VN_CAST(nodep, JumpBlock)) {
            ಯದಿ (ಧ್ರುವ AstCNew* ಧ್ರುವ cnewp = getSuperNewCallRecursep(blockp->stmtsp())) {
                ಪ್ರತ್ಯಾಗಮ cnewp;
            }
        }
        ಯದಿ (ಧ್ರುವ AstCNew* ಧ್ರುವ cnewp = getSuperNewCallRecursep(nodep->nextp())) ಪ್ರತ್ಯಾಗಮ cnewp;
        ಪ್ರತ್ಯಾಗಮ nullptr;
    }

    ಶೂನ್ಯ emitConstantW(AstConst* nodep, AstVarRef* assigntop) {
        // For tradition and compilation speed, assign each word directly into
        // output variable instead of using '='
        putns(nodep, "");
        ಯದಿ (nodep->ಅಂಕೆ().isFourState()) {
            nodep->v3warn(E_UNSUPPORTED, "Unsupported: 4-state numbers in this context");
            ಪ್ರತ್ಯಾಗಮ;
        }

        ಪೂರ್ಣಾಙ್ಕ upWidth = nodep->ಅಂಕೆ().widthToFit();
        ಪೂರ್ಣಾಙ್ಕ chunks = 0;
        ಯದಿ (upWidth > EMITC_ಅಂಕೆ_CONSTW * VL_EDATASIZE) {
            // Output e.g. 8 words in groups of e.g. 8
            chunks = (upWidth - 1) / (EMITC_ಅಂಕೆ_CONSTW * VL_EDATASIZE);
            upWidth %= (EMITC_ಅಂಕೆ_CONSTW * VL_EDATASIZE);
            ಯದಿ (upWidth == 0) upWidth = (EMITC_ಅಂಕೆ_CONSTW * VL_EDATASIZE);
        }
        {  // Upper e.g. 8 words
            ಯದಿ (chunks) {
                putnbs(nodep, "VL_CONSTHI_W_");
                puts(cvtToStr(VL_ಪದಗಳು_I(upWidth)));
                puts("X(");
                puts(cvtToStr(nodep->widthMin()));
                puts(",");
                puts(cvtToStr(chunks * EMITC_ಅಂಕೆ_CONSTW * VL_EDATASIZE));
            } ಅನ್ಯಥಾ {
                putnbs(nodep, "VL_CONST_W_");
                puts(cvtToStr(VL_ಪದಗಳು_I(upWidth)));
                puts("X(");
                puts(cvtToStr(nodep->widthMin()));
            }
            puts(",");
            ಯದಿ (!assigntop->selfPointer().isEmpty()) {
                emitDereference(assigntop, assigntop->selfPointerProtect(m_useSelfForThis));
            }
            puts(varName(assigntop->varp()));
            ಪ್ರತಿ (ಪೂರ್ಣಾಙ್ಕ word = VL_ಪದಗಳು_I(upWidth) - 1; word >= 0; word--) {
                // Only 32 bits - llx + long long here just to appease CPP format warning
                ofp()->printf(",0x%08" PRIx64, static_cast<uint64_t>(nodep->ಅಂಕೆ().edataWord(
                                                   word + chunks * EMITC_ಅಂಕೆ_CONSTW)));
            }
            puts(")");
        }
        ಪ್ರತಿ (chunks--; chunks >= 0; chunks--) {
            puts(";\n");
            putbs("VL_CONSTLO_W_");
            puts(cvtToStr(EMITC_ಅಂಕೆ_CONSTW));
            puts("X(");
            puts(cvtToStr(chunks * EMITC_ಅಂಕೆ_CONSTW * VL_EDATASIZE));
            puts(",");
            ಯದಿ (!assigntop->selfPointer().isEmpty()) {
                emitDereference(assigntop, assigntop->selfPointerProtect(m_useSelfForThis));
            }
            puts(varName(assigntop->varp()));
            ಪ್ರತಿ (ಪೂರ್ಣಾಙ್ಕ word = EMITC_ಅಂಕೆ_CONSTW - 1; word >= 0; word--) {
                // Only 32 bits - llx + long long here just to appease CPP format warning
                ofp()->printf(",0x%08" PRIx64, static_cast<uint64_t>(nodep->ಅಂಕೆ().edataWord(
                                                   word + chunks * EMITC_ಅಂಕೆ_CONSTW)));
            }
            puts(")");
        }
    }

    ಶೂನ್ಯ emitNodesWithText(Ast_ಗಂಟು* nodesp, bool useSelfForThis, bool tracking,
                           ಧ್ರುವ std::string& separator) {
        ಪ್ರತಿ (Ast_ಗಂಟು* nodep = nodesp; nodep; nodep = nodep->nextp()) {
            ಯದಿ (ಧ್ರುವ AstText* ಧ್ರುವ textp = VN_CAST(nodep, Text)) {
                ಧ್ರುವ std::string text
                    = VSelfPointerText::replaceThis(useSelfForThis, textp->text());
                ಯದಿ (tracking) {
                    puts(text);
                } ಅನ್ಯಥಾ {
                    ofp()->putsNoTracking(text);
                }
            } ಅನ್ಯಥಾ {
                iterateConst(nodep);
            }
            ಯದಿ (nodep->nextp()) puts(separator);
        }
    }

    ಶೂನ್ಯ putConstructorSubinit(ಧ್ರುವ AstClass* classp, AstCFunc* cfuncp) {
        // Direct non-virtual bases in declaration order
        ಪ್ರತಿ (ಧ್ರುವ AstClassExtends* extp = classp->extendsp(); extp;
             extp = VN_AS(extp->nextp(), ClassExtends)) {
            ಯದಿ (extp->classp()->isInterfaceClass()) ಅನುವರ್ತ;
            ಯದಿ (extp->classp()->useVirtualPublic()) ಅನುವರ್ತ;
            puts(EmitCUtil::prefixNameProtect(extp->classp()));
            puts("::init");
            ಯದಿ (constructorNeedsProcess(extp->classp())) {
                puts("(vlProcess, vlSymsp");
            } ಅನ್ಯಥಾ {
                puts("(vlSymsp");
            }
            // Handle super.new() args for the concrete parent
            ಯದಿ (ಧ್ರುವ AstCNew* ಧ್ರುವ superNewCallp = getSuperNewCallRecursep(cfuncp->stmtsp())) {
                putCommaIterateNext(superNewCallp->argsp(), true);
            }
            puts(");\n");
            // Since SystemVerilog allows only for extending from one class, for sure there won't
            // be any more constructors to be called. Even if there are some classes which are
            // "implemented", those are interface classes which do not have constructors.
            ಭಙ್ಗ;
        }
    }

    // VISITORS
    using EmitCConstInit::visit;
    ಶೂನ್ಯ visit(AstCFunc* ಧ್ರುವ nodep) override {
        ಯದಿ (nodep->emptyBody() && !nodep->isLoose()) ಪ್ರತ್ಯಾಗಮ;
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
        ಯದಿ (nodep->ifdef() != "") putns(nodep, "#ifdef " + nodep->ifdef() + "\n");
        emitCFuncHeader(nodep, m_modp, /* withScope: */ true);

        puts(" {\n");
        ಯದಿ (nodep->isConstructor()) {
            ಧ್ರುವ AstClass* ಧ್ರುವ classp = VN_CAST(nodep->scopep()->modp(), Class);
            ಯದಿ (classp && classp->extendsp()) putConstructorSubinit(classp, nodep);
        }

        // "+" in the debug indicates a print from the model
        puts("VL_DEBUG_IF(VL_DBG_MSGF(\"+  ");
        ಪ್ರತಿ (ಪೂರ್ಣಾಙ್ಕ i = 0; i < m_modp->level(); ++i) puts("  ");
        puts(EmitCUtil::prefixNameProtect(m_modp));
        puts(nodep->isLoose() ? "__" : "::");
        puts(nodep->nameProtect() + "\\n\"); );\n");

        ಯದಿ (nodep->isLoose()) {
            m_lazyDecls.declared(nodep);  // Defined here, so no longer needs declaration
            ಯದಿ (!nodep->isStatic()) {  // Standard prologue
                m_useSelfForThis = true;
                ಯದಿ (nodep->voidSelf()) {
                    ಧ್ರುವ string className = EmitCUtil::prefixNameProtect(m_modp);
                    puts(className + "* const vlSelf = static_cast<" + className
                         + "*>(vlVoidSelf);\n");
                }
                ಯದಿ (!VN_IS(m_modp, Class)) {
                    puts(EmitCUtil::symClassAssign());  // Uses vlSelf
                } ಅನ್ಯಥಾ {
                    puts("(void)vlSelf;  // Prevent unused variable warning\n");
                }
            }
        }

        // Instantiate a process class if it's going to be needed somewhere later
        nodep->forall([&](ಧ್ರುವ Ast_ಗಂಟು_CCall* ccallp) -> bool {
            ಯದಿ (ccallp->funcp()->needProcess()
                && (ccallp->funcp()->isCoroutine() == VN_IS(ccallp->backp(), CAwait))) {
                ಯದಿ (!nodep->needProcess() && !m_instantiatesOwnProcess) {
                    m_instantiatesOwnProcess = true;
                    ಪ್ರತ್ಯಾಗಮ false;
                }
            }
            ಪ್ರತ್ಯಾಗಮ true;
        });
        ಯದಿ (m_instantiatesOwnProcess) {
            AstCStmt* ಧ್ರುವ vlprocp = new AstCStmt{nodep->fileline()};
            vlprocp->add("VlProcessRef vlProcess = std::make_shared<VlProcess>();\n");
            vlprocp->add("VlProcess::currentp(vlProcess.get());");
            nodep->prependStmtsp(vlprocp);
        } ಅನ್ಯಥಾ ಯದಿ (nodep->needProcess() && nodep->stmtsp()) {
            // Set current process so VlRNG() constructors in this function seed from it
            AstCStmt* ಧ್ರುವ setProcessp = new AstCStmt{nodep->fileline()};
            setProcessp->add("VlProcess::currentp(vlProcess.get());");
            nodep->prependStmtsp(setProcessp);
        }

        ಪ್ರತಿ (Ast_ಗಂಟು* subnodep = nodep->argsp(); subnodep; subnodep = subnodep->nextp()) {
            ಯದಿ (AstVar* ಧ್ರುವ varp = VN_CAST(subnodep, Var)) {
                ಯದಿ (varp->isFuncReturn()) emitVarDecl(varp);
            }
        }

        ಯದಿ (m_useSelfForThis) {
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

        ಯದಿ (nodep->varsp()) {
            putsDecoration(nodep, "// Locals\n");
            iterateAndNextConstNull(nodep->varsp());
        }

        ಯದಿ (nodep->stmtsp()) {
            putsDecoration(nodep, "// Body\n");
            iterateAndNextConstNull(nodep->stmtsp());
        }

        m_usevlSelfRef = false;

        ಯದಿ (nodep->isCoroutine()) {
            // Sometimes coroutines don't have co_awaits,
            // so emit a co_return at the end to avoid compile errors.
            puts("co_return;\n");
        }

        puts("}\n");
        ಯದಿ (nodep->ifdef() != "") puts("#endif  // " + nodep->ifdef() + "\n");
    }

    ಶೂನ್ಯ visit(AstVar* nodep) override {
        UASSERT_OBJ(m_cfuncp, nodep, "Cannot emit non-local variable");
        emitVarDecl(nodep);
    }

    ಶೂನ್ಯ visit(AstCvtArrayToArray* nodep) override {
        ಯದಿ (nodep->reverse()) {
            puts("VL_REVCLONE_Q(");
        } ಅನ್ಯಥಾ {
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

    ಶೂನ್ಯ visit(AstCvtArrayToPacked* nodep) override {
        Ast_ಗಂಟು_DType* ಧ್ರುವ fromDtp = nodep->fromp()->dtypep()->skipRefp();
        Ast_ಗಂಟು_DType* ಧ್ರುವ elemDtp = fromDtp->subDTypep()->skipRefp();
        ಧ್ರುವ bool descending = [&]() {
            ಯದಿ (ಧ್ರುವ AstUnpackArrayDType* ಧ್ರುವ unpackDtp = VN_CAST(fromDtp, UnpackArrayDType))
                ಪ್ರತ್ಯಾಗಮ !unpackDtp->declRange().ascending();
            ಪ್ರತ್ಯಾಗಮ false;
        }();
        puts("VL_PACK_");
        emitIQW(nodep);
        puts("_");
        emitRU(fromDtp);
        emitIQW(elemDtp);
        ಯದಿ (descending) {
            // Wrap source in VL_PACK_REVERSED so VL_PACK sees ascending order
            emitOpName(nodep, "(%nw, %rw, %P, VL_PACK_REVERSED(%li))", nodep->fromp(), elemDtp,
                       nullptr);
        } ಅನ್ಯಥಾ {
            emitOpName(nodep, "(%nw, %rw, %P, %li)", nodep->fromp(), elemDtp, nullptr);
        }
    }

    ಶೂನ್ಯ visit(AstCvtUnpackToQueue* nodep) override {
        Ast_ಗಂಟು_DType* ಧ್ರುವ elemDTypep = nodep->fromp()->dtypep()->subDTypep();
        emitOpName(nodep, nodep->emitC(), nodep->fromp(), elemDTypep, nullptr);
    }
    ಶೂನ್ಯ visit(AstCvtUnpackToUnpack* nodep) override {
        // Whether each dimension is reversed, the first for the outermost, up to the last that is
        std::vector<bool> reverses;
        ಧ್ರುವ AstUnpackArrayDType* ತುದಿ = VN_AS(nodep->dtypep()->skipRefp(), UnpackArrayDType);
        ಧ್ರುವ AstUnpackArrayDType* fromp
            = VN_AS(nodep->fromp()->dtypep()->skipRefp(), UnpackArrayDType);
        ಯಾವತ್ (ತುದಿ) {
            reverses.ತಳ್ಳು_back(ತುದಿ->declRange().ascending() != fromp->declRange().ascending());
            ತುದಿ = VN_CAST(ತುದಿ->subDTypep()->skipRefp(), UnpackArrayDType);
            fromp = VN_CAST(fromp->subDTypep()->skipRefp(), UnpackArrayDType);
        }
        ಯಾವತ್ (!reverses.back()) reverses.ತೆಗೆ_back();
        putns(nodep, "VL_CVT_UNPACK_TO_UNPACK<");
        ಪ್ರತಿ (size_t i = 0; i < reverses.size(); ++i) {
            ಯದಿ (i) puts(", ");
            puts(reverses[i] ? "1" : "0");
        }
        puts(">(");
        iterateAndNextConstNull(nodep->fromp());
        puts(")");
    }

    ಶೂನ್ಯ visit(Ast_ಗಂಟು_Assign* nodep) override {
        ಯದಿ (AstCReset* ಧ್ರುವ resetp = VN_CAST(nodep->rhsp(), CReset)) {
            // TODO get rid of emitVarReset and instead let AstNodeAssign understand how to init
            // anything
            Ast_ಗಂಟು* fromp = nodep->lhsp();
            // Fork needs to use a member select.  Nothing else should be possible before VarRef.
            ಯದಿ (AstMemberSel* ಧ್ರುವ sfromp = VN_CAST(fromp, MemberSel)) {
                // Fork-DynScope generated pointer to previously automatic variable
                AstVar* ಧ್ರುವ memberVarp = sfromp->varp();
                fromp = sfromp->fromp();
                ಯದಿ (AstNullCheck* ಧ್ರುವ sfromp = VN_CAST(fromp, NullCheck)) fromp = sfromp->lhsp();
                Ast_ಗಂಟು_Var_Ref* ಧ್ರುವ fromVarRefp = VN_AS(fromp, ಗಂಟು_Var_Ref);
                emitVarReset(
                    ("VL_NULL_CHECK("s
                     + (fromVarRefp->selfPointer().isEmpty()
                            ? ""
                            : dereferenceString(fromVarRefp->selfPointerProtect(m_useSelfForThis)))
                     + varName(fromVarRefp->varp()) + ", \""
                     + V3OutFormatter::quoteNameControls(protect(nodep->fileline()->filename()))
                     + "\", " + std::to_string(nodep->fileline()->lineno()) + ")->"),
                    memberVarp, resetp->constructing());
            } ಅನ್ಯಥಾ {
                Ast_ಗಂಟು_Var_Ref* ಧ್ರುವ fromVarRefp = VN_AS(fromp, ಗಂಟು_Var_Ref);
                AstVar* ಧ್ರುವ varp = fromVarRefp->varp();
                ಧ್ರುವ string prefix
                    = fromVarRefp->selfPointer().isEmpty()
                          ? ""
                          : dereferenceString(
                                VN_AS(fromp, ಗಂಟು_Var_Ref)->selfPointerProtect(m_useSelfForThis));
                emitVarReset(prefix, varp, resetp->constructing());
            }
            ಪ್ರತ್ಯಾಗಮ;
        }
        bool paren = true;
        bool decind = false;
        bool rhs = true;
        bool reverseUnpack = false;  // Set for descending CvtPackedToArray
        ಧ್ರುವ AstUnpackArrayDType* ಧ್ರುವ unpackDtp
            = VN_CAST(nodep->dtypep()->skipRefp(), UnpackArrayDType);
        ಯದಿ (AstSel* ಧ್ರುವ selp = VN_CAST(nodep->lhsp(), Sel)) {
            UASSERT_OBJ(selp->widthMin() == selp->widthConst(), selp, "Width mismatch");
            ಯದಿ (selp->widthMin() == 1) {
                putnbs(nodep, "VL_ASSIGNBIT_");
                emitIQW(selp->fromp());
                ಯದಿ (nodep->rhsp()->isAllOnesV()) {
                    puts("O(");
                    rhs = false;
                } ಅನ್ಯಥಾ {
                    puts("I(");
                }
                iterateAndNextConstNull(selp->lsbp());
                puts(", ");
                iterateAndNextConstNull(selp->fromp());
                ಯದಿ (rhs) puts(", ");
            } ಅನ್ಯಥಾ {
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
        } ಅನ್ಯಥಾ ಯದಿ (ಧ್ರುವ AstGetcRefN* ಧ್ರುವ selp = VN_CAST(nodep->lhsp(), GetcRefN)) {
            iterateAndNextConstNull(selp->lhsp());
            puts(" = ");
            putnbs(selp, "VL_PUTC_N(");
            iterateAndNextConstNull(selp->lhsp());
            puts(", ");
            iterateAndNextConstNull(selp->rhsp());
            puts(", ");
        } ಅನ್ಯಥಾ ಯದಿ (AstVar* ಧ್ರುವ varp = AstVar::scVarRecurse(nodep->lhsp())) {
            putnbs(varp, "VL_ASSIGN_");  // Set a systemC variable
            emitScIQW(varp);
            emitIQW(nodep);
            puts("(");
            puts(cvtToStr(nodep->widthMin()) + ", ");
            iterateAndNextConstNull(nodep->lhsp());
            puts(", ");
        } ಅನ್ಯಥಾ ಯದಿ (AstVar* ಧ್ರುವ varp = AstVar::scVarRecurse(nodep->rhsp())) {
            putnbs(varp, "VL_ASSIGN_");  // Get a systemC variable
            emitIQW(nodep);
            emitScIQW(varp);
            puts("(");
            puts(cvtToStr(nodep->widthMin()) + ", ");
            iterateAndNextConstNull(nodep->lhsp());
            puts(", ");
        } ಅನ್ಯಥಾ ಯದಿ (ಧ್ರುವ AstCvtPackedToArray* ಧ್ರುವ castp
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
            ಯದಿ (unpackDtp && !unpackDtp->declRange().ascending()) reverseUnpack = true;
        } ಅನ್ಯಥಾ ಯದಿ (ಧ್ರುವ AstCvtArrayToArray* ಧ್ರುವ castp
                   = VN_CAST(nodep->rhsp(), CvtArrayToArray)) {
            ಯದಿ (castp->reverse()) {
                putns(castp, "VL_REVCOPY_Q(");
            } ಅನ್ಯಥಾ {
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
        } ಅನ್ಯಥಾ ಯದಿ (nodep->isWide() && VN_IS(nodep->lhsp(), VarRef)  //
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
            m_wideTempRefp = VN_AS(nodep->lhsp(), VarRef);
            paren = false;
        } ಅನ್ಯಥಾ ಯದಿ (nodep->isWide() && !unpackDtp && !VN_IS(nodep->rhsp(), Const)) {
            putnbs(nodep, "VL_ASSIGN_W(");
            puts(cvtToStr(nodep->widthMin()) + ", ");
            iterateAndNextConstNull(nodep->lhsp());
            puts(", ");
        } ಅನ್ಯಥಾ ಯದಿ (VN_IS(nodep->lhsp()->dtypep()->skipRefp(), QueueDType)
                   && (VN_IS(nodep->rhsp(), StreamL) || VN_IS(nodep->lhsp(), StreamL)
                       || VN_IS(nodep->rhsp(), StreamR) || VN_IS(nodep->lhsp(), StreamR)
                       || VN_IS(nodep->rhsp(), StreamR))) {
            //if either side is streamL or streamR don't emit lhsp everything will be passed by
            //reference

            paren = false;

        } ಅನ್ಯಥಾ {
            paren = false;
            iterateAndNextConstNull(nodep->lhsp());
            puts(" ");
            ofp()->blockInc();
            decind = true;
            ಯದಿ (!VN_IS(nodep->rhsp(), Const)) ofp()->putBreak();
            putns(nodep, "= ");
        }
        ಯದಿ (rhs) iterateAndNextConstNull(nodep->rhsp());
        ಯದಿ (paren) puts(")");
        ಯದಿ (decind) ofp()->blockDec();
        puts(";\n");
        ಯದಿ (reverseUnpack) {
            puts("VL_UNPACK_REVERSED(");
            iterateAndNextConstNull(nodep->lhsp());
            puts(");\n");
        }
    }
    ಶೂನ್ಯ visit(AstAssocSel* nodep) override {
        iterateAndNextConstNull(nodep->fromp());
        ಧ್ರುವ std::string atFunc = nodep->isLValue() ? ".atWrite(" : ".at(";
        putnbs(nodep, atFunc);
        AstAssocArrayDType* ಧ್ರುವ adtypep
            = VN_AS(nodep->fromp()->dtypep()->skipRefp(), AssocArrayDType);
        UASSERT_OBJ(adtypep, nodep, "Associative select on non-associative type");
        iterateAndNextConstNull(nodep->bitp());
        puts(")");
    }
    ಶೂನ್ಯ visit(AstWildcardSel* nodep) override {
        iterateAndNextConstNull(nodep->fromp());
        ಧ್ರುವ std::string atFunc = nodep->isLValue() ? ".atWrite(" : ".at(";
        putnbs(nodep, atFunc);
        AstWildcardArrayDType* ಧ್ರುವ adtypep
            = VN_AS(nodep->fromp()->dtypep()->skipRefp(), WildcardArrayDType);
        UASSERT_OBJ(adtypep, nodep, "Wildcard select on non-wildcard-associative type");
        iterateAndNextConstNull(nodep->bitp());
        puts(")");
    }
    ಶೂನ್ಯ visit(AstCCall* nodep) override {
        ಧ್ರುವ AstCFunc* ಧ್ರುವ funcp = nodep->funcp();
        ಧ್ರುವ Ast_ಗಂಟು_Module* ಧ್ರುವ funcModp = EmitCParentModule::get(funcp);
        putnbs(nodep, "");
        ಯದಿ (funcp->dpiImportPrototype()) {
            // Calling DPI import
            putns(nodep, funcp->name());
        } ಅನ್ಯಥಾ ಯದಿ (funcp->isProperMethod() && funcp->isStatic()) {
            // Call static method via the containing class
            putns(funcModp, EmitCUtil::prefixNameProtect(funcModp) + "::");
            putns(nodep, funcp->nameProtect());
        } ಅನ್ಯಥಾ ಯದಿ (nodep->superReference()) {
            // Calling superclass method
            putns(funcModp, EmitCUtil::prefixNameProtect(funcModp) + "::");
            putns(nodep, funcp->nameProtect());
        } ಅನ್ಯಥಾ ಯದಿ (funcp->isLoose()) {
            // Calling loose method
            putns(nodep, funcNameProtect(funcp));
        } ಅನ್ಯಥಾ {
            // Calling regular method/function
            ಯದಿ (!nodep->selfPointer().isEmpty()) {
                emitDereference(nodep, nodep->selfPointerProtect(m_useSelfForThis));
            }
            putns(nodep, funcp->nameProtect());
        }
        emitCCallArgs(nodep, nodep->selfPointerProtect(m_useSelfForThis), m_cfuncp->needProcess());
    }
    ಶೂನ್ಯ visit(AstCMethodCall* nodep) override {
        ಧ್ರುವ AstCFunc* ಧ್ರುವ funcp = nodep->funcp();
        UASSERT_OBJ(!funcp->isLoose(), nodep, "Loose method called via AstCMethodCall");
        iterateConst(nodep->fromp());
        putnbs(nodep, "->");
        ಯದಿ (nodep->superReference()) {
            // Super call, so the base implementation is used rather than the override
            ಧ್ರುವ Ast_ಗಂಟು_Module* ಧ್ರುವ funcModp = EmitCParentModule::get(funcp);
            putns(funcModp, EmitCUtil::prefixNameProtect(funcModp) + "::");
        }
        putns(funcp, funcp->nameProtect());
        emitCCallArgs(nodep, "", m_cfuncp->needProcess());
    }
    ಶೂನ್ಯ visit(AstCAwait* nodep) override {
        putns(nodep, "co_await ");
        iterateConst(nodep->exprp());
        puts(";\n");
    }
    ಶೂನ್ಯ visit(AstCNew* nodep) override {
        ಯದಿ (VN_IS(nodep->dtypep(), VoidDType)) {  // super.new case
            putsDecoration(nodep, "/*super.new*/");
            ಪ್ರತ್ಯಾಗಮ;
        }
        // assignment case;
        putns(nodep, "VL_NEW(" + EmitCUtil::prefixNameProtect(nodep->dtypep()) + ", "
                         + optionalProcArg(nodep->dtypep()) + "vlSymsp");
        putCommaIterateNext(nodep->argsp(), true);
        puts(")");
    }
    ಶೂನ್ಯ visit(AstCMethodHard* nodep) override {
        iterateConst(nodep->fromp());
        putns(nodep, nodep->usePtr() ? "->" : ".");
        putns(nodep, nodep->name());
        ಯದಿ (nodep->method() == VCMethod::FORCE_READ_SEL) {
            emitIQW(nodep);
            ಯದಿ (nodep->isWide()) puts("<" + cvtToStr(nodep->dtypep()->width_ಪದಗಳು()) + ">");
        } ಅನ್ಯಥಾ ಯದಿ (nodep->method() == VCMethod::ARRAY_SLICE) {
            // VlUnpacked::slice<N_Out>(loIdx) - N_Out is the (fixed) result array size
            ಧ್ರುವ AstUnpackArrayDType* ಧ್ರುವ adtypep
                = VN_AS(nodep->dtypep()->skipRefp(), UnpackArrayDType);
            puts("<" + cvtToStr(adtypep->elementsConst()) + ">");
        } ಅನ್ಯಥಾ ಯದಿ (nodep->method() == VCMethod::COVERGROUP_ADD_COVERPOINT) {
            // The hit-list bound is a template argument of the returned VlCoverpointT<>, and the
            // node's own dtype is that type, so it is the one source of truth for both.
            ಧ್ರುವ AstCoverpointDType* ಧ್ರುವ cpdtypep
                = VN_AS(nodep->dtypep()->skipRefp(), CoverpointDType);
            puts("<" + cvtToStr(cpdtypep->hitBound()) + ">");
        } ಅನ್ಯಥಾ ಯದಿ (nodep->method() == VCMethod::COVERGROUP_ADD_CROSS) {
            ಧ್ರುವ AstCoverCrossDType* ಧ್ರುವ cxdtypep
                = VN_AS(nodep->dtypep()->skipRefp(), CoverCrossDType);
            puts("<" + cxdtypep->cppTemplateArgs() + ">");
        }
        puts("(");
        bool comma = false;
        ಪೂರ್ಣಾಙ್ಕ arg_ಅಂಕೆ = 0;
        ಪ್ರತಿ (Ast_ಗಂಟು* subnodep = nodep->pinsp(); subnodep; subnodep = subnodep->nextp()) {
            ಯದಿ (comma) puts(", ");
            ಯದಿ (nodep->method() == VCMethod::RANDOMIZER_HARD && arg_ಅಂಕೆ == 1) {
                // For RANDOMIZER_HARD's filename argument (2nd arg after constraint),
                // apply protect() similar to VL_STOP to handle --protected flag
                ಯದಿ (ಧ್ರುವ AstCExpr* ಧ್ರುವ cexprp = VN_CAST(subnodep, CExpr)) {
                    // Extract filename from the CExpr (which contains "filename")
                    std::string filename;
                    ಪ್ರತಿ (ಧ್ರುವ Ast_ಗಂಟು* textnodep = cexprp->nodesp(); textnodep;
                         textnodep = textnodep->nextp()) {
                        ಯದಿ (ಧ್ರುವ AstText* ಧ್ರುವ textp = VN_CAST(textnodep, Text)) {
                            filename = textp->text();
                            ಭಙ್ಗ;
                        }
                    }
                    // Remove surrounding quotes if present
                    ಯದಿ (filename.size() >= 2 && filename.front() == '"'
                        && filename.back() == '"') {
                        filename = filename.substr(1, filename.size() - 2);
                    }
                    // Emit with protect()
                    putsQuoted(protect(filename));
                } ಅನ್ಯಥಾ {
                    iterateConst(subnodep);
                }
            } ಅನ್ಯಥಾ {
                iterateConst(subnodep);
            }
            comma = true;
            arg_ಅಂಕೆ++;
        }
        ಯದಿ (nodep->withp()) {
            ಯದಿ (comma) puts(", ");
            iterateConst(nodep->withp());
        }
        puts(")");
    }
    ಶೂನ್ಯ visit(AstCFuncHard* nodep) override {
        putns(nodep, nodep->name());
        puts("(");
        putCommaIterateNext(nodep->pinsp());
        puts(")");
    }
    ಶೂನ್ಯ visit(AstLambdaArgRef* nodep) override { putbs(nodep->nameProtect()); }
    ಶೂನ್ಯ visit(AstWith* nodep) override {
        // With uses a C++11 lambda
        putnbs(nodep, "[&](");
        ಯದಿ (auto* ಧ್ರುವ argrefp = nodep->indexArgRefp()) {
            putnbs(argrefp, argrefp->dtypep()->cType(argrefp->nameProtect(), false, false));
            puts(", ");
        }
        ಯದಿ (auto* ಧ್ರುವ argrefp = nodep->ಮೌಲ್ಯ_Arg_Refp()) {
            putnbs(argrefp, argrefp->dtypep()->cType(argrefp->nameProtect(), false, false));
        }
        puts(") -> ");
        putnbs(nodep, nodep->dtypep()->cType("", false, false));
        puts(" {\n");
        VL_RESTORER(m_createdScopeHash);
        iterateAndNextConstNull(nodep->exprp());
        puts("}\n");
    }
    ಶೂನ್ಯ visit(AstCase* nodep) override {  // LCOV_EXCL_LINE
        // In V3Case...
        nodep->v3fatalSrc("Case statements should have been reduced out");
    }
    ಶೂನ್ಯ visit(AstComment* nodep) override {
        string at;
        ಯದಿ (nodep->showAt()) {
            at = " at " + nodep->fileline()->ascii();
            // If protecting, passthru less information about the design
            ಯದಿ (!v3Global.opt.protectIds()) ಪ್ರತ್ಯಾಗಮ;
        }
        ಯದಿ (!(nodep->protect() && v3Global.opt.protectIds())) {
            putsDecoration(nodep, "// "s + nodep->name() + at + "\n");
        }
        iterateChildrenConst(nodep);
    }
    ಶೂನ್ಯ visit(AstCoverOtherDecl* nodep) override {
        emit_Cover_Other_Decl_ಸೇರಿಸು(nodep);
        ಯದಿ (coverageUsesLocalCounter(nodep)) {
            // Some task bodies are cloned into a caller module where the
            // original instance-local counter is not addressable. Those clones
            // fall back to the global bin number, so register that slot too.
            emit_Cover_Other_Decl_ಸೇರಿಸು(nodep, true);
        }
    }
    ಶೂನ್ಯ visit(AstCoverToggleDecl* nodep) override {
        putns(nodep, "vlSelf->__vlCoverToggleInsert(");  // As Declared in emitCoverageDecl
        puts(cvtToStr(nodep->range().ಬಲ()));
        puts(", ");
        puts(cvtToStr(nodep->range().ಎಡ()));
        puts(", ");
        puts(cvtToStr(nodep->range().ranged()));
        puts(", ");
        putCoverageArray(nodep);
        puts(" + ");
        puts(cvtToStr(coverage_Bin_ಅಂಕೆ(nodep)));
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
                   + VIdProtect::protect_ಪದಗಳು_If(nodep->hier(), nodep->protect()));
        puts(", ");
        putsQuoted(VIdProtect::protect_ಪದಗಳು_If(nodep->page(), nodep->protect()));
        puts(", ");
        putsQuoted(VIdProtect::protect_ಪದಗಳು_If(nodep->comment(), nodep->protect()));
        puts(");\n");
    }
    ಶೂನ್ಯ visit(AstCoverInc* nodep) override {
        ಯದಿ (VN_IS(nodep->declp(), CoverOtherDecl)) {
            ಯದಿ (v3Global.opt.threads() > 1) {
                putns(nodep, "");
                putCoverageArray(nodep->declp());
                puts("[");
                puts(cvtToStr(coverage_Bin_ಅಂಕೆ(nodep->declp())));
                puts("].fetch_add(1, std::memory_order_relaxed);\n");
            } ಅನ್ಯಥಾ {
                putns(nodep, "++(");
                putCoverageArray(nodep->declp());
                puts("[");
                puts(cvtToStr(coverage_Bin_ಅಂಕೆ(nodep->declp())));
                puts("]);\n");
            }
        } ಅನ್ಯಥಾ {
            puts("VL_COV_TOGGLE_CHG_");
            ಯದಿ (v3Global.opt.threads() > 1) {
                puts("MT_");
            } ಅನ್ಯಥಾ {
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
            puts(cvtToStr(coverage_Bin_ಅಂಕೆ(nodep->declp())));
            puts(", ");
            iterateConst(nodep->toggleExprp());
            puts(", ");
            iterateConst(nodep->toggleCovExprp());
            puts(");\n");
        }
    }
    ಶೂನ್ಯ visit(AstDisableFork* nodep) override { putns(nodep, "vlProcess->disableFork();\n"); }
    ಶೂನ್ಯ visit(AstCReturn* nodep) override {
        putns(nodep, "return (");
        iterateAndNextConstNull(nodep->lhsp());
        puts(");\n");
    }
    ಶೂನ್ಯ visit(AstDisplay* nodep) override {
        string text = nodep->fmtp()->text();
        ಯದಿ (nodep->addNewline()) text += "\n";
        display_ಗಂಟು(nodep, nodep->fmtp(), text, nodep->fmtp()->exprsp(), false);
    }
    ಶೂನ್ಯ visit(AstDumpCtl* nodep) override {
        ವಿಕಲ್ಪ (nodep->ctlType()) {
        ಪಕ್ಷ VDumpCtlType::FILE:
            putns(nodep, "vlSymsp->_vm_contextp__->dumpfile(");
            iterateConst(nodep->exprp());
            puts(");\n");
            ಭಙ್ಗ;
        ಪಕ್ಷ VDumpCtlType::VARS:
            // We ignore number of levels to dump in exprp()
            ಯದಿ (v3Global.opt.trace()) {
                putns(nodep, "vlSymsp->_traceDumpOpen();\n");
            } ಅನ್ಯಥಾ {
                putns(nodep, "VL_PRINTF_MT(\"-Info: ");
                puts(V3OutFormatter::quoteNameControls(protect(nodep->fileline()->filename())));
                puts(":");
                puts(cvtToStr(nodep->fileline()->lineno()));
                puts(": $dumpvar ignored, as Verilated without --trace");
                puts("\\n\");\n");
            }
            ಭಙ್ಗ;
        ಪಕ್ಷ VDumpCtlType::ALL:
            // $dumpall currently ignored
            ಭಙ್ಗ;
        ಪಕ್ಷ VDumpCtlType::FLUSH:
            // $dumpall currently ignored; would need rework of VCD single thread,
            // or flag we pass-through to next eval() iteration
            ಭಙ್ಗ;
        ಪಕ್ಷ VDumpCtlType::LIMIT:
            // $dumplimit currently ignored
            ಭಙ್ಗ;
        ಪಕ್ಷ VDumpCtlType::OFF:
            // Currently ignored as both Vcd and Fst do not support them, as would need "X" dump
            ಭಙ್ಗ;
        ಪಕ್ಷ VDumpCtlType::ON:
            // Currently ignored as $dumpoff is also ignored
            ಭಙ್ಗ;
        ಸ್ವಭಾವ: nodep->v3fatalSrc("Bad case, unexpected " << nodep->ctlType().ascii());
        }
    }
    ಶೂನ್ಯ visit(AstScopeName* nodep) override {
        // For use under AstCCalls for dpiImports.  ScopeNames under
        // displays are handled in AstDisplay
        ಯದಿ (!nodep->dpiExport()) {
            // this is where the DPI import context scope is set
            ಧ್ರುವ string scope = nodep->scopeDpiName();
            putnbs(nodep, "(vlSymsp->" + protect("__Vscopep_" + scope) + ")");
        }
    }
    ಶೂನ್ಯ visit(AstSFormat* nodep) override {
        display_ಗಂಟು(nodep, nodep->fmtp(), nodep->fmtp()->text(), nodep->fmtp()->exprsp(), false);
    }
    ಶೂನ್ಯ visit(AstSFormatF* nodep) override {
        display_ಗಂಟು(nodep, nodep, nodep->text(), nodep->exprsp(), false);
    }
    ಶೂನ್ಯ visit(AstFScanF* nodep) override {
        display_ಗಂಟು(nodep, nullptr, nodep->text(), nodep->exprsp(), true);
    }
    ಶೂನ್ಯ visit(AstSScanF* nodep) override {
        display_ಗಂಟು(nodep, nullptr, nodep->text(), nodep->exprsp(), true);
    }
    ಶೂನ್ಯ visit(Ast_ಮೌಲ್ಯ_Plus_Args* nodep) override {
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
    ಶೂನ್ಯ visit(AstTestPlusArgs* nodep) override {
        putns(nodep, "VL_TESTPLUSARGS_I(");
        iterateConst(nodep->searchp());
        puts(")");
    }
    ಶೂನ್ಯ visit(AstFError* nodep) override {
        putns(nodep, "VL_FERROR_I");
        puts(nodep->strp()->isString() ? "N(" : "W(");
        iterateAndNextConstNull(nodep->filep());
        putbs(", ");
        ಯದಿ (nodep->strp()->isWide()) {
            puts(cvtToStr(nodep->strp()->width_ಪದಗಳು()));
            putbs(", ");
        }
        iterateAndNextConstNull(nodep->strp());
        puts(")");
    }
    ಶೂನ್ಯ visit(AstFGetS* nodep) override {
        emitOpName(nodep, nodep->emitC(), nodep->strgp(), nodep->filep(), nullptr);
    }
    ಶೂನ್ಯ visit(AstFOpen* nodep) override {
        putns(nodep, "VL_FOPEN_NN(");
        iterateConst(nodep->filenamep());
        putbs(", ");
        iterateConst(nodep->modep());
        puts(");\n");
    }
    ಶೂನ್ಯ visit(AstFOpenMcd* nodep) override {
        putns(nodep, "VL_FOPEN_MCD_N(");
        iterateConst(nodep->filenamep());
        puts(");\n");
    }
    ಶೂನ್ಯ visit(Ast_ಗಂಟು_Read_Write_Mem* nodep) override {
        putns(nodep, nodep->cFuncPrefixp());
        puts("N(");
        puts(nodep->isHex() ? "true" : "false");
        putbs(", ");
        // Need real storage width
        puts(cvtToStr(nodep->memp()->dtypep()->skipRefp()->subDTypep()->widthMin()));
        uint32_t array_lo = 0;
        bool memUsePtr = true;
        {
            ಧ್ರುವ AstVarRef* ಧ್ರುವ varrefp = VN_CAST(nodep->memp(), VarRef);
            ಯದಿ (!varrefp) {
                nodep->v3error(nodep->verilogKwd() << " loading non-variable");
            } ಅನ್ಯಥಾ ಯದಿ (VN_IS(varrefp->varp()->dtypeSkipRefp(), AssocArrayDType)
                       || VN_IS(varrefp->varp()->dtypeSkipRefp(), DynArrayDType)) {
                // nodep->memp() below will when verilated code is compiled create a C++ template
                memUsePtr = false;
            } ಅನ್ಯಥಾ ಯದಿ (ಧ್ರುವ AstUnpackArrayDType* ಧ್ರುವ adtypep
                       = VN_CAST(varrefp->varp()->dtypeSkipRefp(), UnpackArrayDType)) {
                putbs(", ");
                puts(cvtToStr(varrefp->varp()->dtypep()->arrayUnpackedElements()));
                array_lo = adtypep->lo();
                putbs(", ");
                puts(cvtToStr(array_lo));
            } ಅನ್ಯಥಾ {
                nodep->v3error(
                    nodep->verilogKwd()
                    << " loading other than unpacked/dynamic/associative-array variable");
            }
        }
        putbs(", ");
        iterateConst(nodep->filenamep());
        putbs(", ");
        {
            ಯದಿ (memUsePtr) puts(" &(");
            iterateAndNextConstNull(nodep->memp());
            ಯದಿ (memUsePtr) puts(")");
        }
        putbs(", ");
        ಯದಿ (nodep->lsbp()) {
            iterateAndNextConstNull(nodep->lsbp());
        } ಅನ್ಯಥಾ {
            puts(cvtToStr(array_lo));
        }
        putbs(", ");
        ಯದಿ (nodep->msbp()) {
            iterateAndNextConstNull(nodep->msbp());
        } ಅನ್ಯಥಾ {
            puts("~0ULL");
        }
        puts(");\n");
    }
    ಶೂನ್ಯ visit(AstFClose* nodep) override {
        putns(nodep, "VL_FCLOSE_I(");
        iterateAndNextConstNull(nodep->filep());
        puts("); ");
    }
    ಶೂನ್ಯ visit(AstFFlush* nodep) override {
        ಯದಿ (!nodep->filep()) {
            putns(nodep, "VL_FFLUSH_MT();");
        } ಅನ್ಯಥಾ {
            putns(nodep, "VL_FFLUSH_I(");
            iterateAndNextConstNull(nodep->filep());
            puts(");\n");
        }
    }
    ಶೂನ್ಯ visit(AstFSeek* nodep) override {
        putns(nodep, "(VL_FSEEK_I(");
        iterateAndNextConstNull(nodep->filep());
        puts(", ");
        iterateAndNextConstNull(nodep->offset());
        puts(", ");
        iterateAndNextConstNull(nodep->operation());
        puts(") == -1 ? -1 : 0)");
    }
    ಶೂನ್ಯ visit(AstFTell* nodep) override {
        putns(nodep, "VL_FTELL_I(");
        iterateAndNextConstNull(nodep->filep());
        puts(")");
    }
    ಶೂನ್ಯ visit(AstFRewind* nodep) override {
        putns(nodep, "(VL_FSEEK_I(");
        iterateAndNextConstNull(nodep->filep());
        puts(", 0, 0) == -1 ? -1 : 0)");
    }
    ಶೂನ್ಯ visit(AstFRead* nodep) override {
        putns(nodep, "VL_FREAD_I(");
        puts(cvtToStr(nodep->memp()->widthMin()));  // Need real storage width
        putbs(", ");
        uint32_t array_lo = 0;
        uint32_t array_size = 0;
        {
            ಧ್ರುವ AstVarRef* ಧ್ರುವ varrefp = VN_CAST(nodep->memp(), VarRef);
            ಯದಿ (!varrefp) {
                nodep->v3error(nodep->verilogKwd() << " loading non-variable");
            } ಅನ್ಯಥಾ ಯದಿ (VN_CAST(varrefp->varp()->dtypeSkipRefp(), BasicDType)) {
            } ಅನ್ಯಥಾ ಯದಿ (ಧ್ರುವ AstUnpackArrayDType* ಧ್ರುವ adtypep
                       = VN_CAST(varrefp->varp()->dtypeSkipRefp(), UnpackArrayDType)) {
                array_lo = adtypep->lo();
                array_size = adtypep->elementsConst();
            } ಅನ್ಯಥಾ {
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
        ಯದಿ (nodep->startp()) {
            iterateAndNextConstNull(nodep->startp());
        } ಅನ್ಯಥಾ {
            puts(cvtToStr(array_lo));
        }
        putbs(", ");
        ಯದಿ (nodep->countp()) {
            iterateAndNextConstNull(nodep->countp());
        } ಅನ್ಯಥಾ {
            puts(cvtToStr(array_size));
        }
        puts(")");
    }
    ಶೂನ್ಯ visit(Ast_ರಾಶಿ_Trace_F* nodep) override { putns(nodep, "VL_STACKTRACE_N()"); }
    ಶೂನ್ಯ visit(Ast_ರಾಶಿ_Trace_T* nodep) override { putns(nodep, "VL_STACKTRACE();\n"); }
    ಶೂನ್ಯ visit(AstSystemT* nodep) override {
        putns(nodep, "(void)VL_SYSTEM_I");
        emitIQW(nodep->lhsp());
        puts("(");
        ಯದಿ (nodep->lhsp()->isWide()) {
            puts(cvtToStr(nodep->lhsp()->width_ಪದಗಳು()));
            putbs(", ");
        }
        iterateAndNextConstNull(nodep->lhsp());
        puts(");\n");
    }
    ಶೂನ್ಯ visit(AstSystemF* nodep) override {
        putns(nodep, "VL_SYSTEM_I");
        emitIQW(nodep->lhsp());
        puts("(");
        ಯದಿ (nodep->lhsp()->isWide()) {
            puts(cvtToStr(nodep->lhsp()->width_ಪದಗಳು()));
            putbs(", ");
        }
        iterateAndNextConstNull(nodep->lhsp());
        puts(")");
    }
    ಶೂನ್ಯ visit(Ast_Stmt_ಅಭಿವ್ಯಕ್ತಿ* nodep) override {
        ಯದಿ (VN_IS(nodep->exprp()->dtypep()->skipRefp(), VoidDType)) {
            putns(nodep, "");
        } ಅನ್ಯಥಾ {
            putns(nodep, "std::ignore = ");
        }
        iterateConst(nodep->exprp());
        puts(";\n");
    }
    ಶೂನ್ಯ visit(AstJumpBlock* nodep) override {
        // Allocate label number
        ಧ್ರುವ size_t n = m_labelNumbers.size();
        ಧ್ರುವ bool newEntry = m_labelNumbers.emplace(nodep, n).second;
        UASSERT_OBJ(newEntry, nodep, "AstJumpBlock visited twide");
        // Emit
        putns(nodep, "{\n");  // Make it visually obvious label jumps outside these
        VL_RESTORER(m_createdScopeHash);
        iterateAndNextConstNull(nodep->stmtsp());
        puts("__Vlabel" + std::to_string(n) + ": ;\n");
        puts("}\n");
    }
    ಶೂನ್ಯ visit(AstJumpGo* nodep) override {
        // Retrieve target label number - must already exist (from enclosing AstJumpBlock)
        ಧ್ರುವ size_t n = m_labelNumbers.at(nodep->blockp());
        // Emit
        putns(nodep, "goto __Vlabel" + std::to_string(n) + ";\n");
    }
    ಶೂನ್ಯ visit(AstLoop* nodep) override {
        UASSERT_OBJ(!nodep->contsp(), nodep, "'contsp' only used before LinkJump");
        VL_RESTORER(m_createdScopeHash);
        // Special case when the AstLoopTest is first for output readability
        ಯದಿ (AstLoopTest* ಧ್ರುವ testp = VN_CAST(nodep->stmtsp(), LoopTest)) {
            putns(nodep, "while (");
            iterateConst(testp->condp());
            puts(") {\n");
            iterateAndNextConstNull(testp->nextp());
            puts("}\n");
            ಪ್ರತ್ಯಾಗಮ;
        }
        // Special case when the AstLoopTest is last for output readability
        ಯದಿ (Ast_ಗಂಟು* lastp = nodep->stmtsp()) {
            ಯಾವತ್ (Ast_ಗಂಟು* ಧ್ರುವ nextp = lastp->nextp()) lastp = nextp;
            ಯದಿ (AstLoopTest* ಧ್ರುವ testp = VN_CAST(lastp, LoopTest)) {
                putns(nodep, "do {\n");
                ಪ್ರತಿ (Ast_ಗಂಟು* p = nodep->stmtsp(); p != lastp; p = p->nextp()) iterateConst(p);
                puts("} while (");
                iterateConst(testp->condp());
                puts(");\n");
                ಪ್ರತ್ಯಾಗಮ;
            }
        }
        // Emit generic case directly
        putns(nodep, "while (true) {\n");
        iterateAndNextConstNull(nodep->stmtsp());
        puts("}\n");
    }
    ಶೂನ್ಯ visit(AstLoopTest* nodep) override {
        VL_RESTORER(m_createdScopeHash);
        putns(nodep, "if (!(");
        iterateAndNextConstNull(nodep->condp());
        puts(")) break;\n");
    }
    ಶೂನ್ಯ visit(AstCLocalScope* nodep) override {
        putns(nodep, "{\n");
        VL_RESTORER(m_createdScopeHash);
        iterateAndNextConstNull(nodep->stmtsp());
        puts("}\n");
    }
    ಶೂನ್ಯ visit(Ast_ಗಂಟು_If* nodep) override {
        putns(nodep, "if (");
        ಯದಿ (!nodep->branchPred().unknown()) {
            puts(nodep->branchPred().ascii());
            puts("((");  // Two parens, so that VL_UNLIKELY((class<foo,bar>)) works
        }
        iterateAndNextConstNull(nodep->condp());
        ಯದಿ (!nodep->branchPred().unknown()) puts("))");
        puts(") {\n");
        {
            VL_RESTORER(m_createdScopeHash);
            iterateAndNextConstNull(nodep->thensp());
        }
        puts("}");
        ಯದಿ (!nodep->elsesp()) {
            puts("\n");
        } ಅನ್ಯಥಾ {
            ಯದಿ (VN_IS(nodep->elsesp(), ಗಂಟು_If) && !nodep->elsesp()->nextp()) {
                puts(" else ");
                iterateAndNextConstNull(nodep->elsesp());
            } ಅನ್ಯಥಾ {
                VL_RESTORER(m_createdScopeHash);
                puts(" else {\n");
                iterateAndNextConstNull(nodep->elsesp());
                puts("}\n");
            }
        }
    }
    ಶೂನ್ಯ visit(Ast_ಅಭಿವ್ಯಕ್ತಿ_Stmt* nodep) override {
        VL_RESTORER(m_createdScopeHash);
        ಧ್ರುವ bool containsAwait = nodep->exists([](AstCAwait*) -> bool { ಪ್ರತ್ಯಾಗಮ true; });
        ಯದಿ (containsAwait) {
            UASSERT_OBJ(m_cfuncp && m_cfuncp->isCoroutine(), nodep,
                        "AstExprStmt with CAwait must be in coroutine");
            putnbs(nodep, "(co_await ([&]() -> VlCoroutine {\n");
            iterateAndNextConstNull(nodep->stmtsp());
            puts("co_return;\n");
            ಯದಿ (!nodep->hasResult()) {
                puts("}()))");
                ಪ್ರತ್ಯಾಗಮ;
            }
            puts("}()), ");
            iterateAndNextConstNull(nodep->resultp());
            puts(")");
            ಪ್ರತ್ಯಾಗಮ;
        }
        // GCC allows compound statements in expressions, but this is not standard.
        // So we use an immediate-evaluation lambda and comma operator
        putnbs(nodep, "([&]() {\n");
        ಯದಿ (!nodep->hasResult()) {
            iterateAndNextConstNull(nodep->stmtsp());
            puts("}())");
            ಪ್ರತ್ಯಾಗಮ;
        }
        iterateAndNextConstNull(nodep->stmtsp());
        puts("}(), ");
        iterateAndNextConstNull(nodep->resultp());
        puts(")");
    }
    ಶೂನ್ಯ visit(AstStop* nodep) override {
        putns(nodep, "VL_STOP_MT(");
        putsQuoted(protect(nodep->fileline()->filename()));
        puts(", ");
        puts(cvtToStr(nodep->fileline()->lineno()));
        puts(", \"\"");
        ಯದಿ (nodep->isFatal()) puts(", false");
        puts(");\n");
    }
    ಶೂನ್ಯ visit(AstFinish* nodep) override {
        // Disable all the forks so they don't operate after simulation is finished.
        ಯದಿ (m_cfuncp && m_cfuncp->needProcess()) putns(nodep, "vlProcess->disableFork();\n");
        putns(nodep, "VL_FINISH_MT(");
        putsQuoted(protect(nodep->fileline()->filename()));
        puts(", ");
        puts(cvtToStr(nodep->fileline()->lineno()));
        puts(", \"\");\n");
    }
    ಶೂನ್ಯ visit(AstFinishFork* nodep) override {
        // Disable all the forks so they don't operate after simulation is finished.
        ಯದಿ (m_cfuncp && m_cfuncp->needProcess()) putns(nodep, "vlProcess->disableFork();\n");
        putns(nodep, "VL_FINISH_MT(");
        putsQuoted(protect(nodep->fileline()->filename()));
        puts(", ");
        puts(cvtToStr(nodep->fileline()->lineno()));
        puts(", \"\");\n");
        ಯದಿ (m_cfuncp->isCoroutine()) {
            putns(nodep, "co_return;\n");
        } ಅನ್ಯಥಾ {
            putns(nodep, "return;\n");
        }
    }
    ಶೂನ್ಯ visit(AstPrintTimeScale* nodep) override {
        putns(nodep, "VL_PRINTTIMESCALE(");
        putsQuoted(protect(nodep->prettyName()));
        puts(", ");
        putsQuoted(nodep->timeunit().ascii());
        puts(", vlSymsp->_vm_contextp__);\n");
    }
    ಶೂನ್ಯ visit(AstRand* nodep) override {
        emitOpName(nodep, nodep->emitC(), nodep->seedp(), nullptr, nullptr);
    }
    ಶೂನ್ಯ visit(AstRandRNG* nodep) override {
        emitOpName(nodep, nodep->emitC(), nullptr, nullptr, nullptr);
    }
    ಶೂನ್ಯ visit(AstTime* nodep) override {
        putns(nodep, "VL_TIME_UNITED_Q(");
        UASSERT_OBJ(!nodep->timeunit().isNone(), nodep, "$time has no units");
        ಧ್ರುವ ದ್ವಿಗುಣ time
            = nodep->timeunit().multiplier() / v3Global.rootp()->timeprecision().multiplier();
        UASSERT_OBJ(time >= 1, nodep, "TimeQ is less than 1, will result in division by zero");
        puts(cvtToStr(time));
        puts(")");
    }
    ಶೂನ್ಯ visit(AstTimeD* nodep) override {
        putns(nodep, "VL_TIME_UNITED_D(");
        UASSERT_OBJ(!nodep->timeunit().isNone(), nodep, "$realtime has no units");
        puts(cvtToStr(nodep->timeunit().multiplier()
                      / v3Global.rootp()->timeprecision().multiplier()));
        puts(")");
    }
    ಶೂನ್ಯ visit(AstGetInitialRandomSeed* nodep) override {
        putns(nodep, "vlSymsp->_vm_contextp__->randSeed()");
    }
    ಶೂನ್ಯ visit(AstTimeFormat* nodep) override {
        putns(nodep, "VL_TIMEFORMAT_IINI(");
        ಯದಿ (nodep->unitsp()) {
            puts("true, ");
            iterateAndNextConstNull(nodep->unitsp());
        } ಅನ್ಯಥಾ {
            puts("false, 0");
        }
        puts(", ");
        ಯದಿ (nodep->precisionp()) {
            puts("true, ");
            iterateAndNextConstNull(nodep->precisionp());
        } ಅನ್ಯಥಾ {
            puts("false, 0");
        }
        puts(", ");
        ಯದಿ (nodep->suffixp()) {
            puts("true, ");
            iterateAndNextConstNull(nodep->suffixp());
        } ಅನ್ಯಥಾ {
            puts("false, \"\"");
        }
        puts(", ");
        ಯದಿ (nodep->widthp()) {
            puts("true, ");
            iterateAndNextConstNull(nodep->widthp());
        } ಅನ್ಯಥಾ {
            puts("false, 0");
        }
        puts(", vlSymsp->_vm_contextp__);\n");
    }
    ಶೂನ್ಯ visit(AstTimePrecision* nodep) override {
        putns(nodep, "vlSymsp->_vm_contextp__->timeprecision()");
    }

    // Nodes involing AstText
    ಶೂನ್ಯ visit(AstText* nodep) override {
        // All Text should be under TextBlock/CStmt/CStmtUser/CExpr/CExprUser
        nodep->v3fatalSrc("Text node in unexpected position");
    }
    ಶೂನ್ಯ visit(AstTextBlock* nodep) override {
        putnbs(nodep, "");
        puts(nodep->prefix());
        emitNodesWithText(nodep->nodesp(), false, true, nodep->separator());
        puts(nodep->suffix());
    }
    ಶೂನ್ಯ visit(AstCStmt* nodep) override {
        putnbs(nodep, "");
        emitNodesWithText(nodep->nodesp(), false, true, "");
        ensure_New_ಸಾಲು();
    }
    ಶೂನ್ಯ visit(AstCExpr* nodep) override {
        putnbs(nodep, "");
        emitNodesWithText(nodep->nodesp(), false, true, "");
    }
    ಶೂನ್ಯ visit(AstCStmtUser* nodep) override {
        putnbs(nodep, "");
        ofp()->putsNoTracking("\n");
        ಯದಿ (nodep->fromDollarC() && v3Global.opt.decoration() && !v3Global.opt.protectIds()) {
            ofp()->putsNoTracking("// $c statement at " + nodep->fileline()->ascii() + "\n");
        }
        emitNodesWithText(nodep->nodesp(), m_useSelfForThis, false, "");
        puts("\n");
    }
    ಶೂನ್ಯ visit(AstCExprUser* nodep) override {
        putnbs(nodep, "");
        ofp()->putsNoTracking("\n");
        ಯದಿ (/* is always from $c */ v3Global.opt.decoration() && !v3Global.opt.protectIds()) {
            ofp()->putsNoTracking(
                (nodep->isPure() ? "// $cpure expression at " : "// $c expression at ")
                + nodep->fileline()->ascii() + "\n");
        }
        emitNodesWithText(nodep->nodesp(), m_useSelfForThis, false, "");
        puts("\n");
    }

    // Operators
    ಶೂನ್ಯ visit(Ast_ಗಂಟು_Termop* nodep) override {
        emitOpName(nodep, nodep->emitC(), nullptr, nullptr, nullptr);
    }
    ಶೂನ್ಯ visit(Ast_ಗಂಟು_Uniop* nodep) override {
        ಯದಿ (nodep->emit_Check_Max_ಪದಗಳು()
            && (nodep->width_ಪದಗಳು() > VL_MULS_MAX_ಪದಗಳು
                || nodep->lhsp()->width_ಪದಗಳು() > VL_MULS_MAX_ಪದಗಳು)) {
            nodep->v3warn(
                E_UNSUPPORTED,
                "Unsupported: "
                    << nodep->prettyOperatorName() << " operator of " << nodep->width()
                    << " bits exceeds hardcoded limit VL_MULS_MAX_WORDS in verilatedos.h");
        }
        ಯದಿ (emitSimpleOk(nodep)) {
            putnbs(nodep, "(");
            puts(nodep->emitSimpleOperator());
            puts(" ");
            iterateAndNextConstNull(nodep->lhsp());
            puts(")");
        } ಅನ್ಯಥಾ {
            emitOpName(nodep, nodep->emitC(), nodep->lhsp(), nullptr, nullptr);
        }
    }
    ಶೂನ್ಯ visit(Ast_ಗಂಟು_Biop* nodep) override {
        ಯದಿ (nodep->emit_Check_Max_ಪದಗಳು() && nodep->width_ಪದಗಳು() > VL_MULS_MAX_ಪದಗಳು) {
            nodep->v3warn(
                E_UNSUPPORTED,
                "Unsupported: "
                    << nodep->prettyOperatorName() << " operator of " << nodep->width()
                    << " bits exceeds hardcoded limit VL_MULS_MAX_WORDS in verilatedos.h");
        }
        ಯದಿ (emitSimpleOk(nodep)) {
            putnbs(nodep, "(");
            iterateAndNextConstNull(nodep->lhsp());
            puts(" ");
            putbs(nodep->emitSimpleOperator());
            puts(" ");
            iterateAndNextConstNull(nodep->rhsp());
            puts(")");
        } ಅನ್ಯಥಾ {
            emitOpName(nodep, nodep->emitC(), nodep->lhsp(), nodep->rhsp(), nullptr);
        }
    }
    ಶೂನ್ಯ visit(Ast_ಗಂಟು_Triop* nodep) override {
        UASSERT_OBJ(!emitSimpleOk(nodep), nodep, "Triop cannot be described in a simple way");
        emitOpName(nodep, nodep->emitC(), nodep->lhsp(), nodep->rhsp(), nodep->thsp());
    }
    ಶೂನ್ಯ visit(AstCvtPackString* nodep) override { emitCvtPackStr(nodep->lhsp()); }
    ಶೂನ್ಯ visit(AstRedXor* nodep) override {
        ಯದಿ (nodep->lhsp()->isWide()) {
            visit(static_cast<Ast_ಗಂಟು_Uniop*>(nodep));
        } ಅನ್ಯಥಾ {
            ಧ್ರುವ AstVarRef* ಧ್ರುವ vrefp = VN_CAST(nodep->lhsp(), VarRef);
            ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ widthPow2 = vrefp ? vrefp->varp()->dtypep()->widthPow2()
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
    ಶೂನ್ಯ visit(AstCCast* nodep) override {
        // Extending a value of the same word width is just a NOP.
        ಯದಿ (ಧ್ರುವ AstClassRefDType* ಧ್ರುವ classDtypep
            = VN_CAST(nodep->dtypep()->skipRefp(), ClassRefDType)) {
            putns(nodep, "(" + classDtypep->cType("", false, false) + ")(");
        } ಅನ್ಯಥಾ ಯದಿ (nodep->size() <= VL_BYTESIZE) {
            putns(nodep, "(::CData)(");
        } ಅನ್ಯಥಾ ಯದಿ (nodep->size() <= VL_SHORTSIZE) {
            putns(nodep, "(::SData)(");
        } ಅನ್ಯಥಾ ಯದಿ (nodep->size() <= VL_IDATASIZE) {
            putns(nodep, "(::IData)(");
        } ಅನ್ಯಥಾ {
            putns(nodep, "(::QData)(");
        }
        iterateAndNextConstNull(nodep->lhsp());
        puts(")");
    }
    ಶೂನ್ಯ visit(AstCond* nodep) override {
        // Widths match up already, so we'll just use C++'s operator w/o any temps.
        ಯದಿ (nodep->thenp()->isWide()) {
            emitOpName(nodep, nodep->emitC(), nodep->condp(), nodep->thenp(), nodep->elsep());
        } ಅನ್ಯಥಾ {
            putnbs(nodep, "(");
            iterateAndNextConstNull(nodep->condp());
            putbs(" ? ");
            iterateAndNextConstNull(nodep->thenp());
            putbs(" : ");
            iterateAndNextConstNull(nodep->elsep());
            puts(")");
        }
    }
    ಶೂನ್ಯ visit(AstMatchMasked* nodep) override {
        emitOpName(nodep, nodep->emitC(), nodep->lhsp(), nodep->matchp(), nullptr);
    }
    ಶೂನ್ಯ visit(AstMemberSel* nodep) override {
        ಧ್ರುವ AstVar* ಧ್ರುವ varp = nodep->varp();
        ಧ್ರುವ bool dereferenceCovergroupRef
            = varp->covergroupRefMember() && nodep->access().isReadOrRW();
        ಯದಿ (dereferenceCovergroupRef) putnbs(nodep, "(*");
        iterateAndNextConstNull(nodep->fromp());
        putnbs(nodep, "->");
        ಯದಿ (varp->isIfaceRef()) {
            // varp is the __Viftop companion (e.g. "tx__Viftop"); use the
            // MemberSel name which matches the cell's C++ member (e.g. "tx").
            puts(nodep->nameProtect());
        } ಅನ್ಯಥಾ {
            puts(varName(varp));
        }
        ಯದಿ (dereferenceCovergroupRef) puts(")");
    }
    ಶೂನ್ಯ visit(AstStructSel* nodep) override {
        iterateAndNextConstNull(nodep->fromp());
        putnbs(nodep, ".");
        puts(nodep->nameProtect());
    }
    ಶೂನ್ಯ visit(AstNullCheck* nodep) override {
        putns(nodep, "VL_NULL_CHECK(");
        iterateAndNextConstNull(nodep->lhsp());
        puts(", ");
        putsQuoted(protect(nodep->fileline()->filename()));
        puts(", ");
        puts(cvtToStr(nodep->fileline()->lineno()));
        puts(")");
    }
    ಶೂನ್ಯ visit(AstNewCopy* nodep) override {
        // Polymorphic shallow clone: preserves runtime type via virtual clone()
        // VL_NULL_CHECK enforces null check per IEEE 1800-2023 8.7
        putns(nodep, "VL_NULL_CHECK(");
        ಯದಿ (VN_IS(nodep->rhsp(), Const) && VN_AS(nodep->rhsp(), Const)->isNull()) {
            // V3Const folded rhs to null: emit a typed empty ref so VL_NULL_CHECK fires
            ಧ್ರುವ AstClassRefDType* ಧ್ರುವ refDTypep
                = VN_CAST(nodep->dtypep()->skipRefp(), ClassRefDType);
            puts(refDTypep->cType("", false, false) + "{}");
        } ಅನ್ಯಥಾ {
            iterateAndNextConstNull(nodep->rhsp());
        }
        puts(", ");
        putsQuoted(protect(nodep->fileline()->filename()));
        puts(", ");
        puts(cvtToStr(nodep->fileline()->lineno()));
        puts(").clone(vlSymsp->__Vm_deleter)");
    }
    ಶೂನ್ಯ visit(AstSel* nodep) override {
        // Note ASSIGN checks for this on a LHS
        UASSERT_OBJ(nodep->widthMin() == nodep->widthConst(), nodep, "Width mismatch");
        emitOpName(nodep, nodep->emitC(), nodep->fromp(), nodep->lsbp(), nullptr);
    }
    ಶೂನ್ಯ visit(AstReplicate* nodep) override {
        ಯದಿ (nodep->srcp()->widthMin() == 1 && !nodep->isWide()) {
            UASSERT_OBJ((static_cast<ಪೂರ್ಣಾಙ್ಕ>(VN_AS(nodep->countp(), Const)->toUInt())
                         * nodep->srcp()->widthMin())
                            == nodep->widthMin(),
                        nodep, "Replicate non-constant or width miscomputed");
            putns(nodep, "VL_REPLICATE_");
            emitIQW(nodep);
            puts("OI(");
            ಯದಿ (nodep->srcp()) puts(cvtToStr(nodep->srcp()->widthMin()));
            puts(", ");
            iterateAndNextConstNull(nodep->srcp());
            puts(", ");
            iterateAndNextConstNull(nodep->countp());
            puts(")");
        } ಅನ್ಯಥಾ {
            emitOpName(nodep, nodep->emitC(), nodep->srcp(), nodep->countp(), nullptr);
        }
    }
    ಶೂನ್ಯ emitStreamR(AstStreamR* nodep, Ast_ಗಂಟು* parent) {
        //TODO: This might need to handle more cases like the visit(AstStreamR) function
        emitOpName(nodep, nodep->emitC(), nodep->lhsp(), nodep->rhsp(), nullptr);
    }
    ಶೂನ್ಯ visit(AstStreamR* nodep) override {
        //The parrent node of our AstStreamR will give just enough info for what streamR should
        //output if nodep->backp() is not the parent then emitStreamR should have been used. throw
        //an error
        bool backpIsParent = (nodep->backp()->op1p() == nodep || nodep->backp()->op2p() == nodep);
        UASSERT(backpIsParent, "can not find return type for streamR");
        ಯದಿ (ಧ್ರುವ AstQueueDType* ಧ್ರುವ qdtp
            = VN_CAST(nodep->backp()->dtypep()->skipRefp(), QueueDType)) {
            ಯದಿ (nodep->lhsp()->isWide()) {
                emitOpName(nodep,
                           "VL_STREAMR_%nq%lq%rq(%lw," + std::to_string(qdtp->subDTypep()->width())
                               + ", %P, %li, %ri)",
                           nodep->lhsp(), nodep->rhsp(), nullptr);
            } ಅನ್ಯಥಾ {
                emitOpName(nodep, "VL_STREAMR_%nq%lq%rq(%lw, %P, %li, %ri)", nodep->lhsp(),
                           nodep->rhsp(), nullptr);
            }
        } ಅನ್ಯಥಾ ಯದಿ (VN_IS(nodep->lhsp()->dtypep()->skipRefp(), QueueDType)) {
            ಯದಿ (!((nodep->backp()->op1p() && nodep->backp()->op1p()->isWide())
                  || (nodep->backp()->op2p() && nodep->backp()->op2p()->isWide()))) {
                //If our lhsp is a queue make sure we streamR and return the correct type.
                //If either side is wide or the previous node is string type dont use this case
                emitOpName(nodep->backp(), "VL_STREAMR_%nq%lq%rq(%lw, %P, %li, %ri)",
                           nodep->lhsp(), nodep->rhsp(), nullptr);
            } ಅನ್ಯಥಾ {
                emitOpName(nodep, nodep->emitC(), nodep->lhsp(), nodep->rhsp(), nullptr);
            }
        } ಅನ್ಯಥಾ {
            emitOpName(nodep, nodep->emitC(), nodep->lhsp(), nodep->rhsp(), nullptr);
        }
    }
    ಶೂನ್ಯ visit(AstStreamL* nodep) override {
        // Attempt to use a "fast" stream function for slice size = power of 2
        ಯದಿ (!nodep->isWide()) {
            ಧ್ರುವ uint32_t isPow2 = VN_AS(nodep->rhsp(), Const)->ಅಂಕೆ().ಎಣಿಕೆ_Ones() == 1;
            ಧ್ರುವ uint32_t sliceSize = VN_AS(nodep->rhsp(), Const)->toUInt();
            ಯದಿ (isPow2 && sliceSize <= (nodep->isQuad() ? ಪರಿಮಾಣ(uint64_t) : ಪರಿಮಾಣ(uint32_t))) {
                putns(nodep, "VL_STREAML_FAST_");
                bool usesQueue = false;
                AstQueueDType* qtypep
                    = nodep->backp()->op2p()
                          ? VN_CAST(nodep->backp()->op2p()->dtypep()->skipRefp(), QueueDType)
                          : nullptr;
                ಯದಿ (VN_IS(nodep->backp(), Assign)
                    && qtypep) {  // If we are assigning to a queue then emit the correct symbol
                    puts("R");  // R for queue
                    usesQueue = true;
                } ಅನ್ಯಥಾ {
                    emitIQW(nodep);
                }
                emitIQW(nodep->lhsp());
                puts("I(");
                puts(cvtToStr(nodep->lhsp()->widthMin()));
                puts(", ");
                ಯದಿ (usesQueue) {
                    iterateAndNextConstNull(nodep->backp()->op2p());
                    puts(", ");
                }
                iterateAndNextConstNull(nodep->lhsp());
                puts(", ");
                ಧ್ರುವ uint32_t rd_log2 = V3_ಸಂಖ್ಯೆ::log2b(VN_AS(nodep->rhsp(), Const)->toUInt());
                puts(cvtToStr(rd_log2) + ")");
                ಪ್ರತ್ಯಾಗಮ;
            }
        }
        ಯದಿ (VN_IS(nodep->backp(), Assign)
            && VN_IS(nodep->backp()->op2p()->dtypep()->skipRefp(), QueueDType)) {
            ಪೂರ್ಣಾಙ್ಕ queueWidth
                = nodep->backp()->op2p()->dtypep()->subDTypep()->width();  //We need to know the
                                                                           //width of both sides
            emitOpName(nodep,
                       "VL_STREAML_%nq%lq%rq(%lw," + std::to_string(queueWidth)
                           + ", %P, %li, %ri)",
                       nodep->lhsp(), nodep->rhsp(), nullptr);
        } ಅನ್ಯಥಾ {
            emitOpName(nodep, nodep->emitC(), nodep->lhsp(), nodep->rhsp(), nullptr);
        }
    }
    ಶೂನ್ಯ visit(AstCastDynamic* nodep) override {
        putnbs(nodep, "VL_CAST_DYNAMIC(");
        iterateAndNextConstNull(nodep->fromp());
        puts(", ");
        iterateAndNextConstNull(nodep->ತುದಿ());
        puts(")");
    }
    ಶೂನ್ಯ visit(Ast_ಎಣಿಕೆ_Bits* nodep) override {
        putnbs(nodep, "VL_COUNTBITS_");
        emitIQW(nodep->lhsp());
        puts("(");
        puts(cvtToStr(nodep->lhsp()->widthMin()));
        puts(", ");
        ಯದಿ (nodep->lhsp()->isWide()) {
            puts(cvtToStr(nodep->lhsp()->width_ಪದಗಳು()));  // Note argument width, not node width
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
    ಶೂನ್ಯ visit(AstTriggered* nodep) override {
        // Test each word of the trigger vector with a mask of the relevant bits
        std::map<uint32_t, uint64_t> wordMasks;
        ಪ್ರತಿ (ಧ್ರುವ uint32_t index : nodep->indices()) {
            wordMasks[VL_BITWORD_Q(index)] |= VL_BIT_Q(index);
        }
        putns(nodep, "((");
        bool first = true;
        ಪ್ರತಿ (ಧ್ರುವ auto& wordMask : wordMasks) {
            ಯದಿ (!first) puts("\n| ");
            first = false;
            ಧ್ರುವ bool allOnes = wordMask.second == ~0ULL;
            ಯದಿ (!allOnes) puts("(" + V3_ಸಂಖ್ಯೆ{nodep, 64}.setQuad(wordMask.second).emitC() + " & ");
            iterateConst(nodep->trigRefp());
            puts("[" + std::to_string(wordMask.first) + "U]");
            ಯದಿ (!allOnes) puts(")");
        }
        puts(") != 0ULL)");
    }
    // Terminals
    ಶೂನ್ಯ visit(AstVarRef* nodep) override {
        ಧ್ರುವ AstVar* ಧ್ರುವ varp = nodep->varp();

        // Reference to constant pool variable is special
        ಯದಿ (varp->constPoolEntry()) {
            putns(nodep, EmitCUtil::constPoolName(varp));
            ಪ್ರತ್ಯಾಗಮ;
        }

        ಧ್ರುವ Ast_ಗಂಟು_Module* ಧ್ರುವ varModp = EmitCParentModule::get(varp);
        ಧ್ರುವ bool dereferenceCovergroupRef
            = varp->covergroupRefMember() && nodep->access().isReadOrRW();
        ಯದಿ (dereferenceCovergroupRef) putns(nodep, "(*");
        ಯದಿ (VN_IS(varModp, Class) && varModp != m_modp) {
            // Superclass member reference
            putns(nodep, EmitCUtil::prefixNameProtect(varModp) + "::");
        } ಅನ್ಯಥಾ ಯದಿ (varp->isIfaceRef()) {
            putns(nodep, nodep->selfPointerProtect(m_useSelfForThis));
            ಪ್ರತ್ಯಾಗಮ;
        } ಅನ್ಯಥಾ ಯದಿ (!nodep->selfPointer().isEmpty()) {
            emitDereference(nodep, nodep->selfPointerProtect(m_useSelfForThis));
        }
        putns(nodep, varName(varp));
        ಯದಿ (dereferenceCovergroupRef) puts(")");
    }
    ಶೂನ್ಯ visit(AstAddrOfCFunc* nodep) override {
        // Note: Can be thought to handle more, but this is all that is needed right now
        ಧ್ರುವ AstCFunc* ಧ್ರುವ funcp = nodep->funcp();
        UASSERT_OBJ(funcp->isLoose(), nodep, "Cannot take address of non-loose method");
        putns(nodep, "&");
        puts(funcNameProtect(funcp));
    }
    ಶೂನ್ಯ visit(AstConst* nodep) override {  //
        ಯದಿ (m_wideTempRefp && nodep->isWide()) {
            UASSERT_OBJ(m_wideTempRefp, nodep, "Wide Constant w/ no temp");
            emitConstantW(nodep, m_wideTempRefp);
            m_wideTempRefp = nullptr;  // We used it, fail if set it a second time
        } ಅನ್ಯಥಾ {
            emitConstant(nodep);
        }
    }
    ಶೂನ್ಯ visit(Ast_ಗಂಟು_Cons* nodep) override { emitValCons(nodep); }
    ಶೂನ್ಯ visit(AstThisRef* nodep) override {
        putnbs(nodep, nodep->dtypep()->cType("", false, false));
        puts("{");
        puts(VSelfPointerText::replaceThis(m_useSelfForThis, "this"));
        puts("}");
    }
    ಶೂನ್ಯ visit(Ast_ಗಂಟು_Sel* nodep) override {
        ಯದಿ (!VN_IS(nodep, ArraySel) && !VN_IS(nodep, WordSel)) {
            visit(static_cast<Ast_ಗಂಟು_Biop*>(nodep));
            ಪ್ರತ್ಯಾಗಮ;
        }
        // ArraySel or WordSel
        iterateAndNextConstNull(nodep->fromp());
        // Special case constant index for readability
        ಯದಿ (AstConst* ಧ್ರುವ idxp = VN_CAST(nodep->bitp(), Const)) {
            puts("[" + std::to_string(idxp->toUInt()) + "U]");
            ಪ್ರತ್ಯಾಗಮ;
        }
        putbs("[");
        iterateAndNextConstNull(nodep->bitp());
        puts("]");
    }

    //
    ಶೂನ್ಯ visit(AstExecGraph* nodep) override {
        // The location of the AstExecGraph within the containing AstCFunc is where we want to
        // invoke the graph and wait for it to complete. The run-time library does just that.
        putns(nodep, "vl_invokeExecGraph(" + EmitCUtil::execGraphName(nodep) + ", vlSelf, ");
        iterateConst(nodep->triggerp());
        puts(".data());\n");
    }

    // Default
    ಶೂನ್ಯ visit(Ast_ಗಂಟು* nodep) override {  // LCOV_EXCL_START
        putns(nodep, "\n???? // "s + nodep->prettyTypeName() + "\n");
        iterateChildrenConst(nodep);
        ಯದಿ (!v3Global.opt.lintOnly()) {  // An internal problem, so suppress
            nodep->v3fatalSrc("Unknown node type reached emitter: " << nodep->prettyTypeName());
        }
    }  // LCOV_EXCL_STOP

ರಕ್ಷಿತ:
    EmitCFunc() = ಸ್ವಭಾವ;
    ~EmitCFunc() override = ಸ್ವಭಾವ;
};

#endif  // guard
