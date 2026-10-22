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

#അന്തര്ഭാവ "config_build.h"
#അന്തര്ഭാവ "verilatedos.h"

#അന്തര്ഭാവ "V3EmitCConstInit.h"
#അന്തര്ഭാവ "V3Global.h"
#അന്തര്ഭാവ "V3MemberMap.h"

#അന്തര്ഭാവ <algorithm>
#അന്തര്ഭാവ <map>
#അന്തര്ഭാവ <unordered_map>
#അന്തര്ഭാവ <unordered_set>
#അന്തര്ഭാവ <vector>

// Number of VL_CONST_W_*X's in verilated.h (IE VL_CONST_W_8X is last)
constexpr പൂർണ്ണസംഖ്യ EMITC_അക്കം_CONSTW = 8;

//######################################################################
// Emit lazy forward declarations

class EmitCLazyDecls final : പൊതു VNVisitorConst {
    // NODE STATE/TYPES
    // None allowed to support threaded emitting

    // MEMBERS
    std::unordered_set<string> m_emittedManually;  // Set of names already declared manually.
    EmitCBaseVisitorConst& m_emitter;  // For access to file output
    bool m_needs_Blank_വരി = false;  // Emit blank line if any declarations were emitted (cosmetic)
    std::set<Ast_കണ്ണി*> m_emitted;  // -> in set. Already emitted decl for symbols.

    // METHODS
    bool declaredOnce(Ast_കണ്ണി* nodep) { തിരികെ m_emitted.തിരുകുക(nodep).second; }

    ശൂന്യം lazyDeclare(AstCFunc* funcp) {
        // Already declared in this compilation unit
        എങ്കിൽ (!declaredOnce(funcp)) തിരികെ;
        // Check if this kind of function is lazily declared
        എങ്കിൽ (!(funcp->isMethod() && funcp->isLoose()) && !funcp->dpiImportPrototype()) തിരികെ;
        // Already declared manually
        എങ്കിൽ (m_emittedManually.എണ്ണം(funcp->nameProtect())) തിരികെ;
        // Needs lazy declaration, emit one
        m_emitter.emitCFuncDecl(funcp, EmitCParentModule::get(funcp), funcp->dpiImportPrototype());
        m_needs_Blank_വരി = true;
    }

    ശൂന്യം lazyDeclareConstPoolVar(AstVar* varp) {
        എങ്കിൽ (!declaredOnce(varp)) തിരികെ;  // Already declared
        സ്ഥിരം string nameProtect = EmitCUtil::constPoolName(varp);
        m_emitter.putns(varp, "extern const ");
        m_emitter.puts(varp->dtypep()->cType(nameProtect, false, false));
        m_emitter.puts(";\n");
        m_needs_Blank_വരി = true;
    }

    // VISITORS
    ശൂന്യം visit(Ast_കണ്ണി_CCall* nodep) override {
        lazyDeclare(nodep->funcp());
        iterateChildrenConst(nodep);
    }

    ശൂന്യം visit(AstAddrOfCFunc* nodep) override {
        lazyDeclare(nodep->funcp());
        iterateChildrenConst(nodep);
    }

    ശൂന്യം visit(AstVarRef* nodep) override {
        AstVar* സ്ഥിരം varp = nodep->varp();
        // Only constant pool symbols are lazy declared for now ...
        എങ്കിൽ (EmitCParentModule::get(varp)->isConstPool()) lazyDeclareConstPoolVar(varp);
    }

    ശൂന്യം visit(AstExecGraph* nodep) override {
        എങ്കിൽ (!declaredOnce(nodep)) തിരികെ;
        m_emitter.putns(nodep, "extern VlExecGraph " + EmitCUtil::execGraphName(nodep) + ";\n");
        m_needs_Blank_വരി = true;
    }

    ശൂന്യം visit(Ast_കണ്ണി* nodep) override { iterateChildrenConst(nodep); }

പൊതു:
    explicit EmitCLazyDecls(EmitCBaseVisitorConst& emitter)
        : m_emitter{emitter} {}
    ശൂന്യം emit(Ast_കണ്ണി* nodep) {
        m_needs_Blank_വരി = false;
        iterateChildrenConst(nodep);
        എങ്കിൽ (m_needs_Blank_വരി) m_emitter.puts("\n");
    }
    ശൂന്യം emit(സ്ഥിരം string& prefix, സ്ഥിരം string& name, സ്ഥിരം string& suffix) {
        m_emittedManually.തിരുകുക(name);
        m_emitter.ensure_New_വരി();
        m_emitter.puts(prefix);
        m_emitter.puts(name);
        m_emitter.puts(suffix);
        m_emitter.ensure_New_വരി();
    }
    ശൂന്യം declared(AstCFunc* nodep) { m_emitted.തിരുകുക(nodep); }
    ശൂന്യം reset() { m_emitted.clear(); }
};

// ######################################################################
//  Emit statements and expressions

class EmitCFunc VL_NOT_FINAL : പൊതു EmitCConstInit {
    VMemberMap m_memberMap;
    AstVarRef* m_wideTempRefp = nullptr;  // Variable that _WW macros should be setting
    std::unordered_map<AstJumpBlock*, size_t> m_labelNumbers;  // Label numbers for AstJumpBlocks
    bool m_createdScopeHash = false;  // Already created a scope hash

സംരക്ഷിത:
    VL_DEFINE_DEBUG_FUNCTIONS;

    EmitCLazyDecls m_lazyDecls{*this};  // Visitor for emitting lazy declarations
    bool m_useSelfForThis = false;  // Replace "this" with "vlSelf"
    bool m_usevlSelfRef = false;  // Use vlSelfRef reference instead of vlSelf pointer
    സ്ഥിരം Ast_കണ്ണി_Module* m_modp = nullptr;  // Current module being emitted
    സ്ഥിരം AstCFunc* m_cfuncp = nullptr;  // Current function being emitted
    bool m_instantiatesOwnProcess = false;
    സ്ഥിരം AstClassPackage* m_classOrPackage = nullptr;  // Pointer to current class or package
    string m_classOrPackageHash;  // Hash of class or package name

    bool constructorNeedsProcess(സ്ഥിരം AstClass* സ്ഥിരം classp) {
        സ്ഥിരം Ast_കണ്ണി* സ്ഥിരം newp = m_memberMap.findMember(classp, "new");
        എങ്കിൽ (!newp) തിരികെ false;
        സ്ഥിരം AstCFunc* സ്ഥിരം ctorp = VN_CAST(newp, CFunc);
        എങ്കിൽ (!ctorp) തിരികെ false;
        UASSERT_OBJ(ctorp->isConstructor(), ctorp, "`new` is not a constructor!");
        തിരികെ ctorp->needProcess();
    }

    bool constructorNeedsProcess(സ്ഥിരം Ast_കണ്ണി_DType* സ്ഥിരം dtypep) {
        എങ്കിൽ (സ്ഥിരം AstClassRefDType* സ്ഥിരം crefdtypep = VN_CAST(dtypep, ClassRefDType))
            തിരികെ constructorNeedsProcess(crefdtypep->classp());
        തിരികെ false;
    }

    bool coverageUsesLocalCounter(Ast_കണ്ണി_Cover_Decl* സ്ഥിരം declp) സ്ഥിരം {
        // Only functions with an object receiver can address the module-local
        // coverage array. Static/package/class helper functions still use the
        // symbol-table array, as they may have vlSymsp but no vlSelf/this.
        // Also require the coverage declaration to belong to the current
        // module; cloned task bodies can increment declarations owned by a
        // different module, and those must keep using the global array.
        തിരികെ m_cfuncp && !m_cfuncp->isStatic() && !VN_IS(m_modp, Class)
               && !VN_IS(m_modp, ClassPackage)
               && EmitCParentModule::get(declp->dataDeclThisp()) == m_modp;
    }
    പൂർണ്ണസംഖ്യ coverage_Bin_അക്കം(Ast_കണ്ണി_Cover_Decl* സ്ഥിരം declp, bool forceGlobal = false) സ്ഥിരം {
        തിരികെ !forceGlobal && coverageUsesLocalCounter(declp)
                   ? declp->dataDeclThisp()->local_Bin_അക്കം()
                   : declp->dataDeclThisp()->bin_അക്കം();
    }
    ശൂന്യം putCoverageArray(Ast_കണ്ണി_Cover_Decl* സ്ഥിരം declp, bool forceGlobal = false) {
        എങ്കിൽ (!forceGlobal && coverageUsesLocalCounter(declp)) {
            // Keep counters on the module object when possible, so every
            // no-inline instance has storage available if coverage output is
            // later written with forcePerInstance.
            puts(m_cfuncp->isLoose() ? "vlSelf->__Vcoverage" : "this->__Vcoverage");
        } അല്ലെങ്കിൽ {
            // Static/package/class helper functions do not have vlSelf, so
            // keep using the shared symbol-table coverage array there.
            puts("vlSymsp->__Vcoverage");
        }
    }
    ശൂന്യം emit_Cover_Other_Decl_തിരുകുക(AstCoverOtherDecl* nodep, bool forceGlobal = false) {
        putns(nodep, "vlSelf->__vlCoverInsert(");  // As Declared in emitCoverageDecl
        putCoverageArray(nodep, forceGlobal);
        puts(" + ");
        puts(cvtToStr(coverage_Bin_അക്കം(nodep, forceGlobal)));
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
                   + VIdProtect::protect_വാക്കുകൾ_If(nodep->hier(), nodep->protect()));
        puts(", ");
        putsQuoted(VIdProtect::protect_വാക്കുകൾ_If(nodep->page(), nodep->protect()));
        puts(", ");
        putsQuoted(VIdProtect::protect_വാക്കുകൾ_If(nodep->comment(), nodep->protect()));
        puts(", ");
        putsQuoted(nodep->linescov());
        puts(", ");
        putsQuoted(VIdProtect::protect_വാക്കുകൾ_If(nodep->fsmVar(), nodep->protect()));
        puts(", ");
        putsQuoted(VIdProtect::protect_വാക്കുകൾ_If(nodep->fsmFrom(), nodep->protect()));
        puts(", ");
        putsQuoted(VIdProtect::protect_വാക്കുകൾ_If(nodep->fsmTo(), nodep->protect()));
        puts(", ");
        putsQuoted(VIdProtect::protect_വാക്കുകൾ_If(nodep->fsmTag(), nodep->protect()));
        puts(");\n");
    }

പൊതു:
    // METHODS
    bool displayEmitHeader(Ast_കണ്ണി* nodep);
    ശൂന്യം display_കണ്ണി(Ast_കണ്ണി* nodep, AstSFormatF* fmtp, സ്ഥിരം string& vformat, Ast_കണ്ണി* exprsp,
                     bool isScan);

    bool emitSimpleOk(Ast_കണ്ണി_വ്യഞ്ജകം* nodep);
    ശൂന്യം emitIQW(സ്ഥിരം Ast_കണ്ണി* nodep) {
        // See "Type letters" in verilated.h
        // Other abbrevs: "C"har, "S"hort, "F"loat, "D"ouble, stri"N"g, "R"=queue, "U"npacked
        puts(nodep->dtypep()->skipRefp()->charIQWN());
    }
    ശൂന്യം emitRU(സ്ഥിരം Ast_കണ്ണി* nodep) {
        സ്ഥിരം Ast_കണ്ണി_DType* dtp = nodep->dtypep()->skipRefp();
        // See "Type letters" in verilated.h
        എങ്കിൽ (VN_IS(dtp, UnpackArrayDType))
            puts("U");
        അല്ലെങ്കിൽ എങ്കിൽ (VN_IS(dtp, QueueDType) || VN_IS(dtp, DynArrayDType))
            puts("R");
    }
    ശൂന്യം emitScIQW(AstVar* nodep) {
        UASSERT_OBJ(nodep->isSc(), nodep, "emitting SystemC operator on non-SC variable");
        puts(nodep->isScBigUint()                           ? "SB"
             : (nodep->isScUint() || nodep->isScUintBool()) ? "SU"
             : nodep->isScBv()                              ? "SW"
             : nodep->isDouble()                            ? "SD"
                                                            : (nodep->isScQuad() ? "SQ" : "SI"));
    }
    ശൂന്യം emitDatap(സ്ഥിരം Ast_കണ്ണി* nodep) {
        // When passing to a function with va_args the compiler doesn't
        // know need a pointer so when wide, need to look inside VlWide
        എങ്കിൽ (nodep->isWide()) puts(".data()");
    }
    ശൂന്യം emitOpName(Ast_കണ്ണി* nodep, സ്ഥിരം string& format, Ast_കണ്ണി* lhsp, Ast_കണ്ണി* rhsp,
                    Ast_കണ്ണി* thsp);
    ശൂന്യം emitCCallArgs(സ്ഥിരം Ast_കണ്ണി_CCall* nodep, സ്ഥിരം string& selfPointer, bool inProcess);
    ശൂന്യം emitDereference(Ast_കണ്ണി* nodep, സ്ഥിരം string& pointer);
    std::string dereferenceString(സ്ഥിരം std::string& pointer) സ്ഥിരം;
    ശൂന്യം emitCvtPackStr(Ast_കണ്ണി* nodep);
    ശൂന്യം emitConstant(AstConst* nodep);
    ശൂന്യം emitConstantString(സ്ഥിരം AstConst* nodep);
    ശൂന്യം emitVarReset(സ്ഥിരം string& prefix, AstVar* varp, bool constructing);
    string emitVarResetRecurse(സ്ഥിരം AstVar* varp, bool constructing,
                               സ്ഥിരം string& varNameProtected, Ast_കണ്ണി_DType* dtypep, പൂർണ്ണസംഖ്യ depth,
                               സ്ഥിരം string& suffix, സ്ഥിരം Ast_കണ്ണി* valuep);
    ശൂന്യം emitVarResetScopeHash();
    ശൂന്യം emitChangeDet();
    ശൂന്യം putCommaIterateNext(Ast_കണ്ണി* nodep, bool comma = false) {
        വേണ്ടി (Ast_കണ്ണി* subnodep = nodep; subnodep; subnodep = subnodep->nextp()) {
            എങ്കിൽ (comma) puts(", ");
            iterateConst(subnodep);
            comma = true;
        }
    }
    template <typename T>
    string optionalProcArg(സ്ഥിരം T* സ്ഥിരം nodep) {
        തിരികെ (nodep && constructorNeedsProcess(nodep)) ? "vlProcess, " : "";
    }
    സ്ഥിരം AstCNew* getSuperNewCallRecursep(Ast_കണ്ണി* സ്ഥിരം nodep) {
        // Get the super.new call
        എങ്കിൽ (!nodep) തിരികെ nullptr;
        എങ്കിൽ (സ്ഥിരം AstCNew* സ്ഥിരം cnewp = VN_CAST(nodep, CNew)) തിരികെ cnewp;
        എങ്കിൽ (സ്ഥിരം Ast_Stmt_വ്യഞ്ജകം* സ്ഥിരം stmtp = VN_CAST(nodep, Stmt_വ്യഞ്ജകം)) {
            എങ്കിൽ (സ്ഥിരം AstCNew* സ്ഥിരം cnewp = VN_CAST(stmtp->exprp(), CNew)) തിരികെ cnewp;
        }
        എങ്കിൽ (സ്ഥിരം AstJumpBlock* സ്ഥിരം blockp = VN_CAST(nodep, JumpBlock)) {
            എങ്കിൽ (സ്ഥിരം AstCNew* സ്ഥിരം cnewp = getSuperNewCallRecursep(blockp->stmtsp())) {
                തിരികെ cnewp;
            }
        }
        എങ്കിൽ (സ്ഥിരം AstCNew* സ്ഥിരം cnewp = getSuperNewCallRecursep(nodep->nextp())) തിരികെ cnewp;
        തിരികെ nullptr;
    }

    ശൂന്യം emitConstantW(AstConst* nodep, AstVarRef* assigntop) {
        // For tradition and compilation speed, assign each word directly into
        // output variable instead of using '='
        putns(nodep, "");
        എങ്കിൽ (nodep->അക്കം().isFourState()) {
            nodep->v3warn(E_UNSUPPORTED, "Unsupported: 4-state numbers in this context");
            തിരികെ;
        }

        പൂർണ്ണസംഖ്യ upWidth = nodep->അക്കം().widthToFit();
        പൂർണ്ണസംഖ്യ chunks = 0;
        എങ്കിൽ (upWidth > EMITC_അക്കം_CONSTW * VL_EDATASIZE) {
            // Output e.g. 8 words in groups of e.g. 8
            chunks = (upWidth - 1) / (EMITC_അക്കം_CONSTW * VL_EDATASIZE);
            upWidth %= (EMITC_അക്കം_CONSTW * VL_EDATASIZE);
            എങ്കിൽ (upWidth == 0) upWidth = (EMITC_അക്കം_CONSTW * VL_EDATASIZE);
        }
        {  // Upper e.g. 8 words
            എങ്കിൽ (chunks) {
                putnbs(nodep, "VL_CONSTHI_W_");
                puts(cvtToStr(VL_വാക്കുകൾ_I(upWidth)));
                puts("X(");
                puts(cvtToStr(nodep->widthMin()));
                puts(",");
                puts(cvtToStr(chunks * EMITC_അക്കം_CONSTW * VL_EDATASIZE));
            } അല്ലെങ്കിൽ {
                putnbs(nodep, "VL_CONST_W_");
                puts(cvtToStr(VL_വാക്കുകൾ_I(upWidth)));
                puts("X(");
                puts(cvtToStr(nodep->widthMin()));
            }
            puts(",");
            എങ്കിൽ (!assigntop->selfPointer().isEmpty()) {
                emitDereference(assigntop, assigntop->selfPointerProtect(m_useSelfForThis));
            }
            puts(varName(assigntop->varp()));
            വേണ്ടി (പൂർണ്ണസംഖ്യ word = VL_വാക്കുകൾ_I(upWidth) - 1; word >= 0; word--) {
                // Only 32 bits - llx + long long here just to appease CPP format warning
                ofp()->printf(",0x%08" PRIx64, static_cast<uint64_t>(nodep->അക്കം().edataWord(
                                                   word + chunks * EMITC_അക്കം_CONSTW)));
            }
            puts(")");
        }
        വേണ്ടി (chunks--; chunks >= 0; chunks--) {
            puts(";\n");
            putbs("VL_CONSTLO_W_");
            puts(cvtToStr(EMITC_അക്കം_CONSTW));
            puts("X(");
            puts(cvtToStr(chunks * EMITC_അക്കം_CONSTW * VL_EDATASIZE));
            puts(",");
            എങ്കിൽ (!assigntop->selfPointer().isEmpty()) {
                emitDereference(assigntop, assigntop->selfPointerProtect(m_useSelfForThis));
            }
            puts(varName(assigntop->varp()));
            വേണ്ടി (പൂർണ്ണസംഖ്യ word = EMITC_അക്കം_CONSTW - 1; word >= 0; word--) {
                // Only 32 bits - llx + long long here just to appease CPP format warning
                ofp()->printf(",0x%08" PRIx64, static_cast<uint64_t>(nodep->അക്കം().edataWord(
                                                   word + chunks * EMITC_അക്കം_CONSTW)));
            }
            puts(")");
        }
    }

    ശൂന്യം emitNodesWithText(Ast_കണ്ണി* nodesp, bool useSelfForThis, bool tracking,
                           സ്ഥിരം std::string& separator) {
        വേണ്ടി (Ast_കണ്ണി* nodep = nodesp; nodep; nodep = nodep->nextp()) {
            എങ്കിൽ (സ്ഥിരം AstText* സ്ഥിരം textp = VN_CAST(nodep, Text)) {
                സ്ഥിരം std::string text
                    = VSelfPointerText::replaceThis(useSelfForThis, textp->text());
                എങ്കിൽ (tracking) {
                    puts(text);
                } അല്ലെങ്കിൽ {
                    ofp()->putsNoTracking(text);
                }
            } അല്ലെങ്കിൽ {
                iterateConst(nodep);
            }
            എങ്കിൽ (nodep->nextp()) puts(separator);
        }
    }

    ശൂന്യം putConstructorSubinit(സ്ഥിരം AstClass* classp, AstCFunc* cfuncp) {
        // Direct non-virtual bases in declaration order
        വേണ്ടി (സ്ഥിരം AstClassExtends* extp = classp->extendsp(); extp;
             extp = VN_AS(extp->nextp(), ClassExtends)) {
            എങ്കിൽ (extp->classp()->isInterfaceClass()) തുടരുക;
            എങ്കിൽ (extp->classp()->useVirtualPublic()) തുടരുക;
            puts(EmitCUtil::prefixNameProtect(extp->classp()));
            puts("::init");
            എങ്കിൽ (constructorNeedsProcess(extp->classp())) {
                puts("(vlProcess, vlSymsp");
            } അല്ലെങ്കിൽ {
                puts("(vlSymsp");
            }
            // Handle super.new() args for the concrete parent
            എങ്കിൽ (സ്ഥിരം AstCNew* സ്ഥിരം superNewCallp = getSuperNewCallRecursep(cfuncp->stmtsp())) {
                putCommaIterateNext(superNewCallp->argsp(), true);
            }
            puts(");\n");
            // Since SystemVerilog allows only for extending from one class, for sure there won't
            // be any more constructors to be called. Even if there are some classes which are
            // "implemented", those are interface classes which do not have constructors.
            നിർത്തുക;
        }
    }

    // VISITORS
    using EmitCConstInit::visit;
    ശൂന്യം visit(AstCFunc* സ്ഥിരം nodep) override {
        എങ്കിൽ (nodep->emptyBody() && !nodep->isLoose()) തിരികെ;
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
        എങ്കിൽ (nodep->ifdef() != "") putns(nodep, "#ifdef " + nodep->ifdef() + "\n");
        emitCFuncHeader(nodep, m_modp, /* withScope: */ true);

        puts(" {\n");
        എങ്കിൽ (nodep->isConstructor()) {
            സ്ഥിരം AstClass* സ്ഥിരം classp = VN_CAST(nodep->scopep()->modp(), Class);
            എങ്കിൽ (classp && classp->extendsp()) putConstructorSubinit(classp, nodep);
        }

        // "+" in the debug indicates a print from the model
        puts("VL_DEBUG_IF(VL_DBG_MSGF(\"+  ");
        വേണ്ടി (പൂർണ്ണസംഖ്യ i = 0; i < m_modp->level(); ++i) puts("  ");
        puts(EmitCUtil::prefixNameProtect(m_modp));
        puts(nodep->isLoose() ? "__" : "::");
        puts(nodep->nameProtect() + "\\n\"); );\n");

        എങ്കിൽ (nodep->isLoose()) {
            m_lazyDecls.declared(nodep);  // Defined here, so no longer needs declaration
            എങ്കിൽ (!nodep->isStatic()) {  // Standard prologue
                m_useSelfForThis = true;
                എങ്കിൽ (nodep->voidSelf()) {
                    സ്ഥിരം string className = EmitCUtil::prefixNameProtect(m_modp);
                    puts(className + "* const vlSelf = static_cast<" + className
                         + "*>(vlVoidSelf);\n");
                }
                എങ്കിൽ (!VN_IS(m_modp, Class)) {
                    puts(EmitCUtil::symClassAssign());  // Uses vlSelf
                } അല്ലെങ്കിൽ {
                    puts("(void)vlSelf;  // Prevent unused variable warning\n");
                }
            }
        }

        // Instantiate a process class if it's going to be needed somewhere later
        nodep->forall([&](സ്ഥിരം Ast_കണ്ണി_CCall* ccallp) -> bool {
            എങ്കിൽ (ccallp->funcp()->needProcess()
                && (ccallp->funcp()->isCoroutine() == VN_IS(ccallp->backp(), CAwait))) {
                എങ്കിൽ (!nodep->needProcess() && !m_instantiatesOwnProcess) {
                    m_instantiatesOwnProcess = true;
                    തിരികെ false;
                }
            }
            തിരികെ true;
        });
        എങ്കിൽ (m_instantiatesOwnProcess) {
            AstCStmt* സ്ഥിരം vlprocp = new AstCStmt{nodep->fileline()};
            vlprocp->add("VlProcessRef vlProcess = std::make_shared<VlProcess>();\n");
            vlprocp->add("VlProcess::currentp(vlProcess.get());");
            nodep->prependStmtsp(vlprocp);
        } അല്ലെങ്കിൽ എങ്കിൽ (nodep->needProcess() && nodep->stmtsp()) {
            // Set current process so VlRNG() constructors in this function seed from it
            AstCStmt* സ്ഥിരം setProcessp = new AstCStmt{nodep->fileline()};
            setProcessp->add("VlProcess::currentp(vlProcess.get());");
            nodep->prependStmtsp(setProcessp);
        }

        വേണ്ടി (Ast_കണ്ണി* subnodep = nodep->argsp(); subnodep; subnodep = subnodep->nextp()) {
            എങ്കിൽ (AstVar* സ്ഥിരം varp = VN_CAST(subnodep, Var)) {
                എങ്കിൽ (varp->isFuncReturn()) emitVarDecl(varp);
            }
        }

        എങ്കിൽ (m_useSelfForThis) {
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

        എങ്കിൽ (nodep->varsp()) {
            putsDecoration(nodep, "// Locals\n");
            iterateAndNextConstNull(nodep->varsp());
        }

        എങ്കിൽ (nodep->stmtsp()) {
            putsDecoration(nodep, "// Body\n");
            iterateAndNextConstNull(nodep->stmtsp());
        }

        m_usevlSelfRef = false;

        എങ്കിൽ (nodep->isCoroutine()) {
            // Sometimes coroutines don't have co_awaits,
            // so emit a co_return at the end to avoid compile errors.
            puts("co_return;\n");
        }

        puts("}\n");
        എങ്കിൽ (nodep->ifdef() != "") puts("#endif  // " + nodep->ifdef() + "\n");
    }

    ശൂന്യം visit(AstVar* nodep) override {
        UASSERT_OBJ(m_cfuncp, nodep, "Cannot emit non-local variable");
        emitVarDecl(nodep);
    }

    ശൂന്യം visit(AstCvtArrayToArray* nodep) override {
        എങ്കിൽ (nodep->reverse()) {
            puts("VL_REVCLONE_Q(");
        } അല്ലെങ്കിൽ {
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

    ശൂന്യം visit(AstCvtArrayToPacked* nodep) override {
        Ast_കണ്ണി_DType* സ്ഥിരം fromDtp = nodep->fromp()->dtypep()->skipRefp();
        Ast_കണ്ണി_DType* സ്ഥിരം elemDtp = fromDtp->subDTypep()->skipRefp();
        സ്ഥിരം bool descending = [&]() {
            എങ്കിൽ (സ്ഥിരം AstUnpackArrayDType* സ്ഥിരം unpackDtp = VN_CAST(fromDtp, UnpackArrayDType))
                തിരികെ !unpackDtp->declRange().ascending();
            തിരികെ false;
        }();
        puts("VL_PACK_");
        emitIQW(nodep);
        puts("_");
        emitRU(fromDtp);
        emitIQW(elemDtp);
        എങ്കിൽ (descending) {
            // Wrap source in VL_PACK_REVERSED so VL_PACK sees ascending order
            emitOpName(nodep, "(%nw, %rw, %P, VL_PACK_REVERSED(%li))", nodep->fromp(), elemDtp,
                       nullptr);
        } അല്ലെങ്കിൽ {
            emitOpName(nodep, "(%nw, %rw, %P, %li)", nodep->fromp(), elemDtp, nullptr);
        }
    }

    ശൂന്യം visit(AstCvtUnpackToQueue* nodep) override {
        Ast_കണ്ണി_DType* സ്ഥിരം elemDTypep = nodep->fromp()->dtypep()->subDTypep();
        emitOpName(nodep, nodep->emitC(), nodep->fromp(), elemDTypep, nullptr);
    }
    ശൂന്യം visit(AstCvtUnpackToUnpack* nodep) override {
        // Whether each dimension is reversed, the first for the outermost, up to the last that is
        std::vector<bool> reverses;
        സ്ഥിരം AstUnpackArrayDType* മുകൾ = VN_AS(nodep->dtypep()->skipRefp(), UnpackArrayDType);
        സ്ഥിരം AstUnpackArrayDType* fromp
            = VN_AS(nodep->fromp()->dtypep()->skipRefp(), UnpackArrayDType);
        ആകുമ്പോൾ (മുകൾ) {
            reverses.തള്ളുക_back(മുകൾ->declRange().ascending() != fromp->declRange().ascending());
            മുകൾ = VN_CAST(മുകൾ->subDTypep()->skipRefp(), UnpackArrayDType);
            fromp = VN_CAST(fromp->subDTypep()->skipRefp(), UnpackArrayDType);
        }
        ആകുമ്പോൾ (!reverses.back()) reverses.എടുക്കുക_back();
        putns(nodep, "VL_CVT_UNPACK_TO_UNPACK<");
        വേണ്ടി (size_t i = 0; i < reverses.size(); ++i) {
            എങ്കിൽ (i) puts(", ");
            puts(reverses[i] ? "1" : "0");
        }
        puts(">(");
        iterateAndNextConstNull(nodep->fromp());
        puts(")");
    }

    ശൂന്യം visit(Ast_കണ്ണി_Assign* nodep) override {
        എങ്കിൽ (AstCReset* സ്ഥിരം resetp = VN_CAST(nodep->rhsp(), CReset)) {
            // TODO get rid of emitVarReset and instead let AstNodeAssign understand how to init
            // anything
            Ast_കണ്ണി* fromp = nodep->lhsp();
            // Fork needs to use a member select.  Nothing else should be possible before VarRef.
            എങ്കിൽ (AstMemberSel* സ്ഥിരം sfromp = VN_CAST(fromp, MemberSel)) {
                // Fork-DynScope generated pointer to previously automatic variable
                AstVar* സ്ഥിരം memberVarp = sfromp->varp();
                fromp = sfromp->fromp();
                എങ്കിൽ (AstNullCheck* സ്ഥിരം sfromp = VN_CAST(fromp, NullCheck)) fromp = sfromp->lhsp();
                Ast_കണ്ണി_Var_Ref* സ്ഥിരം fromVarRefp = VN_AS(fromp, കണ്ണി_Var_Ref);
                emitVarReset(
                    ("VL_NULL_CHECK("s
                     + (fromVarRefp->selfPointer().isEmpty()
                            ? ""
                            : dereferenceString(fromVarRefp->selfPointerProtect(m_useSelfForThis)))
                     + varName(fromVarRefp->varp()) + ", \""
                     + V3OutFormatter::quoteNameControls(protect(nodep->fileline()->filename()))
                     + "\", " + std::to_string(nodep->fileline()->lineno()) + ")->"),
                    memberVarp, resetp->constructing());
            } അല്ലെങ്കിൽ {
                Ast_കണ്ണി_Var_Ref* സ്ഥിരം fromVarRefp = VN_AS(fromp, കണ്ണി_Var_Ref);
                AstVar* സ്ഥിരം varp = fromVarRefp->varp();
                സ്ഥിരം string prefix
                    = fromVarRefp->selfPointer().isEmpty()
                          ? ""
                          : dereferenceString(
                                VN_AS(fromp, കണ്ണി_Var_Ref)->selfPointerProtect(m_useSelfForThis));
                emitVarReset(prefix, varp, resetp->constructing());
            }
            തിരികെ;
        }
        bool paren = true;
        bool decind = false;
        bool rhs = true;
        bool reverseUnpack = false;  // Set for descending CvtPackedToArray
        സ്ഥിരം AstUnpackArrayDType* സ്ഥിരം unpackDtp
            = VN_CAST(nodep->dtypep()->skipRefp(), UnpackArrayDType);
        എങ്കിൽ (AstSel* സ്ഥിരം selp = VN_CAST(nodep->lhsp(), Sel)) {
            UASSERT_OBJ(selp->widthMin() == selp->widthConst(), selp, "Width mismatch");
            എങ്കിൽ (selp->widthMin() == 1) {
                putnbs(nodep, "VL_ASSIGNBIT_");
                emitIQW(selp->fromp());
                എങ്കിൽ (nodep->rhsp()->isAllOnesV()) {
                    puts("O(");
                    rhs = false;
                } അല്ലെങ്കിൽ {
                    puts("I(");
                }
                iterateAndNextConstNull(selp->lsbp());
                puts(", ");
                iterateAndNextConstNull(selp->fromp());
                എങ്കിൽ (rhs) puts(", ");
            } അല്ലെങ്കിൽ {
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
        } അല്ലെങ്കിൽ എങ്കിൽ (സ്ഥിരം AstGetcRefN* സ്ഥിരം selp = VN_CAST(nodep->lhsp(), GetcRefN)) {
            iterateAndNextConstNull(selp->lhsp());
            puts(" = ");
            putnbs(selp, "VL_PUTC_N(");
            iterateAndNextConstNull(selp->lhsp());
            puts(", ");
            iterateAndNextConstNull(selp->rhsp());
            puts(", ");
        } അല്ലെങ്കിൽ എങ്കിൽ (AstVar* സ്ഥിരം varp = AstVar::scVarRecurse(nodep->lhsp())) {
            putnbs(varp, "VL_ASSIGN_");  // Set a systemC variable
            emitScIQW(varp);
            emitIQW(nodep);
            puts("(");
            puts(cvtToStr(nodep->widthMin()) + ", ");
            iterateAndNextConstNull(nodep->lhsp());
            puts(", ");
        } അല്ലെങ്കിൽ എങ്കിൽ (AstVar* സ്ഥിരം varp = AstVar::scVarRecurse(nodep->rhsp())) {
            putnbs(varp, "VL_ASSIGN_");  // Get a systemC variable
            emitIQW(nodep);
            emitScIQW(varp);
            puts("(");
            puts(cvtToStr(nodep->widthMin()) + ", ");
            iterateAndNextConstNull(nodep->lhsp());
            puts(", ");
        } അല്ലെങ്കിൽ എങ്കിൽ (സ്ഥിരം AstCvtPackedToArray* സ്ഥിരം castp
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
            എങ്കിൽ (unpackDtp && !unpackDtp->declRange().ascending()) reverseUnpack = true;
        } അല്ലെങ്കിൽ എങ്കിൽ (സ്ഥിരം AstCvtArrayToArray* സ്ഥിരം castp
                   = VN_CAST(nodep->rhsp(), CvtArrayToArray)) {
            എങ്കിൽ (castp->reverse()) {
                putns(castp, "VL_REVCOPY_Q(");
            } അല്ലെങ്കിൽ {
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
        } അല്ലെങ്കിൽ എങ്കിൽ (nodep->isWide() && VN_IS(nodep->lhsp(), VarRef)  //
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
                   && !VN_IS(nodep->rhsp(), വ്യഞ്ജകം_Stmt)) {
            // Wide functions assign into the array directly, don't need separate assign statement
            m_wideTempRefp = VN_AS(nodep->lhsp(), VarRef);
            paren = false;
        } അല്ലെങ്കിൽ എങ്കിൽ (nodep->isWide() && !unpackDtp && !VN_IS(nodep->rhsp(), Const)) {
            putnbs(nodep, "VL_ASSIGN_W(");
            puts(cvtToStr(nodep->widthMin()) + ", ");
            iterateAndNextConstNull(nodep->lhsp());
            puts(", ");
        } അല്ലെങ്കിൽ എങ്കിൽ (VN_IS(nodep->lhsp()->dtypep()->skipRefp(), QueueDType)
                   && (VN_IS(nodep->rhsp(), StreamL) || VN_IS(nodep->lhsp(), StreamL)
                       || VN_IS(nodep->rhsp(), StreamR) || VN_IS(nodep->lhsp(), StreamR)
                       || VN_IS(nodep->rhsp(), StreamR))) {
            //if either side is streamL or streamR don't emit lhsp everything will be passed by
            //reference

            paren = false;

        } അല്ലെങ്കിൽ {
            paren = false;
            iterateAndNextConstNull(nodep->lhsp());
            puts(" ");
            ofp()->blockInc();
            decind = true;
            എങ്കിൽ (!VN_IS(nodep->rhsp(), Const)) ofp()->putBreak();
            putns(nodep, "= ");
        }
        എങ്കിൽ (rhs) iterateAndNextConstNull(nodep->rhsp());
        എങ്കിൽ (paren) puts(")");
        എങ്കിൽ (decind) ofp()->blockDec();
        puts(";\n");
        എങ്കിൽ (reverseUnpack) {
            puts("VL_UNPACK_REVERSED(");
            iterateAndNextConstNull(nodep->lhsp());
            puts(");\n");
        }
    }
    ശൂന്യം visit(AstAssocSel* nodep) override {
        iterateAndNextConstNull(nodep->fromp());
        സ്ഥിരം std::string atFunc = nodep->isLValue() ? ".atWrite(" : ".at(";
        putnbs(nodep, atFunc);
        AstAssocArrayDType* സ്ഥിരം adtypep
            = VN_AS(nodep->fromp()->dtypep()->skipRefp(), AssocArrayDType);
        UASSERT_OBJ(adtypep, nodep, "Associative select on non-associative type");
        iterateAndNextConstNull(nodep->bitp());
        puts(")");
    }
    ശൂന്യം visit(AstWildcardSel* nodep) override {
        iterateAndNextConstNull(nodep->fromp());
        സ്ഥിരം std::string atFunc = nodep->isLValue() ? ".atWrite(" : ".at(";
        putnbs(nodep, atFunc);
        AstWildcardArrayDType* സ്ഥിരം adtypep
            = VN_AS(nodep->fromp()->dtypep()->skipRefp(), WildcardArrayDType);
        UASSERT_OBJ(adtypep, nodep, "Wildcard select on non-wildcard-associative type");
        iterateAndNextConstNull(nodep->bitp());
        puts(")");
    }
    ശൂന്യം visit(AstCCall* nodep) override {
        സ്ഥിരം AstCFunc* സ്ഥിരം funcp = nodep->funcp();
        സ്ഥിരം Ast_കണ്ണി_Module* സ്ഥിരം funcModp = EmitCParentModule::get(funcp);
        putnbs(nodep, "");
        എങ്കിൽ (funcp->dpiImportPrototype()) {
            // Calling DPI import
            putns(nodep, funcp->name());
        } അല്ലെങ്കിൽ എങ്കിൽ (funcp->isProperMethod() && funcp->isStatic()) {
            // Call static method via the containing class
            putns(funcModp, EmitCUtil::prefixNameProtect(funcModp) + "::");
            putns(nodep, funcp->nameProtect());
        } അല്ലെങ്കിൽ എങ്കിൽ (nodep->superReference()) {
            // Calling superclass method
            putns(funcModp, EmitCUtil::prefixNameProtect(funcModp) + "::");
            putns(nodep, funcp->nameProtect());
        } അല്ലെങ്കിൽ എങ്കിൽ (funcp->isLoose()) {
            // Calling loose method
            putns(nodep, funcNameProtect(funcp));
        } അല്ലെങ്കിൽ {
            // Calling regular method/function
            എങ്കിൽ (!nodep->selfPointer().isEmpty()) {
                emitDereference(nodep, nodep->selfPointerProtect(m_useSelfForThis));
            }
            putns(nodep, funcp->nameProtect());
        }
        emitCCallArgs(nodep, nodep->selfPointerProtect(m_useSelfForThis), m_cfuncp->needProcess());
    }
    ശൂന്യം visit(AstCMethodCall* nodep) override {
        സ്ഥിരം AstCFunc* സ്ഥിരം funcp = nodep->funcp();
        UASSERT_OBJ(!funcp->isLoose(), nodep, "Loose method called via AstCMethodCall");
        iterateConst(nodep->fromp());
        putnbs(nodep, "->");
        എങ്കിൽ (nodep->superReference()) {
            // Super call, so the base implementation is used rather than the override
            സ്ഥിരം Ast_കണ്ണി_Module* സ്ഥിരം funcModp = EmitCParentModule::get(funcp);
            putns(funcModp, EmitCUtil::prefixNameProtect(funcModp) + "::");
        }
        putns(funcp, funcp->nameProtect());
        emitCCallArgs(nodep, "", m_cfuncp->needProcess());
    }
    ശൂന്യം visit(AstCAwait* nodep) override {
        putns(nodep, "co_await ");
        iterateConst(nodep->exprp());
        puts(";\n");
    }
    ശൂന്യം visit(AstCNew* nodep) override {
        എങ്കിൽ (VN_IS(nodep->dtypep(), VoidDType)) {  // super.new case
            putsDecoration(nodep, "/*super.new*/");
            തിരികെ;
        }
        // assignment case;
        putns(nodep, "VL_NEW(" + EmitCUtil::prefixNameProtect(nodep->dtypep()) + ", "
                         + optionalProcArg(nodep->dtypep()) + "vlSymsp");
        putCommaIterateNext(nodep->argsp(), true);
        puts(")");
    }
    ശൂന്യം visit(AstCMethodHard* nodep) override {
        iterateConst(nodep->fromp());
        putns(nodep, nodep->usePtr() ? "->" : ".");
        putns(nodep, nodep->name());
        എങ്കിൽ (nodep->method() == VCMethod::FORCE_READ_SEL) {
            emitIQW(nodep);
            എങ്കിൽ (nodep->isWide()) puts("<" + cvtToStr(nodep->dtypep()->width_വാക്കുകൾ()) + ">");
        } അല്ലെങ്കിൽ എങ്കിൽ (nodep->method() == VCMethod::ARRAY_SLICE) {
            // VlUnpacked::slice<N_Out>(loIdx) - N_Out is the (fixed) result array size
            സ്ഥിരം AstUnpackArrayDType* സ്ഥിരം adtypep
                = VN_AS(nodep->dtypep()->skipRefp(), UnpackArrayDType);
            puts("<" + cvtToStr(adtypep->elementsConst()) + ">");
        } അല്ലെങ്കിൽ എങ്കിൽ (nodep->method() == VCMethod::COVERGROUP_ADD_COVERPOINT) {
            // The hit-list bound is a template argument of the returned VlCoverpointT<>, and the
            // node's own dtype is that type, so it is the one source of truth for both.
            സ്ഥിരം AstCoverpointDType* സ്ഥിരം cpdtypep
                = VN_AS(nodep->dtypep()->skipRefp(), CoverpointDType);
            puts("<" + cvtToStr(cpdtypep->hitBound()) + ">");
        } അല്ലെങ്കിൽ എങ്കിൽ (nodep->method() == VCMethod::COVERGROUP_ADD_CROSS) {
            സ്ഥിരം AstCoverCrossDType* സ്ഥിരം cxdtypep
                = VN_AS(nodep->dtypep()->skipRefp(), CoverCrossDType);
            puts("<" + cxdtypep->cppTemplateArgs() + ">");
        }
        puts("(");
        bool comma = false;
        പൂർണ്ണസംഖ്യ arg_അക്കം = 0;
        വേണ്ടി (Ast_കണ്ണി* subnodep = nodep->pinsp(); subnodep; subnodep = subnodep->nextp()) {
            എങ്കിൽ (comma) puts(", ");
            എങ്കിൽ (nodep->method() == VCMethod::RANDOMIZER_HARD && arg_അക്കം == 1) {
                // For RANDOMIZER_HARD's filename argument (2nd arg after constraint),
                // apply protect() similar to VL_STOP to handle --protected flag
                എങ്കിൽ (സ്ഥിരം AstCExpr* സ്ഥിരം cexprp = VN_CAST(subnodep, CExpr)) {
                    // Extract filename from the CExpr (which contains "filename")
                    std::string filename;
                    വേണ്ടി (സ്ഥിരം Ast_കണ്ണി* textnodep = cexprp->nodesp(); textnodep;
                         textnodep = textnodep->nextp()) {
                        എങ്കിൽ (സ്ഥിരം AstText* സ്ഥിരം textp = VN_CAST(textnodep, Text)) {
                            filename = textp->text();
                            നിർത്തുക;
                        }
                    }
                    // Remove surrounding quotes if present
                    എങ്കിൽ (filename.size() >= 2 && filename.front() == '"'
                        && filename.back() == '"') {
                        filename = filename.substr(1, filename.size() - 2);
                    }
                    // Emit with protect()
                    putsQuoted(protect(filename));
                } അല്ലെങ്കിൽ {
                    iterateConst(subnodep);
                }
            } അല്ലെങ്കിൽ {
                iterateConst(subnodep);
            }
            comma = true;
            arg_അക്കം++;
        }
        എങ്കിൽ (nodep->withp()) {
            എങ്കിൽ (comma) puts(", ");
            iterateConst(nodep->withp());
        }
        puts(")");
    }
    ശൂന്യം visit(AstCFuncHard* nodep) override {
        putns(nodep, nodep->name());
        puts("(");
        putCommaIterateNext(nodep->pinsp());
        puts(")");
    }
    ശൂന്യം visit(AstLambdaArgRef* nodep) override { putbs(nodep->nameProtect()); }
    ശൂന്യം visit(AstWith* nodep) override {
        // With uses a C++11 lambda
        putnbs(nodep, "[&](");
        എങ്കിൽ (auto* സ്ഥിരം argrefp = nodep->indexArgRefp()) {
            putnbs(argrefp, argrefp->dtypep()->cType(argrefp->nameProtect(), false, false));
            puts(", ");
        }
        എങ്കിൽ (auto* സ്ഥിരം argrefp = nodep->മൂല്യം_Arg_Refp()) {
            putnbs(argrefp, argrefp->dtypep()->cType(argrefp->nameProtect(), false, false));
        }
        puts(") -> ");
        putnbs(nodep, nodep->dtypep()->cType("", false, false));
        puts(" {\n");
        VL_RESTORER(m_createdScopeHash);
        iterateAndNextConstNull(nodep->exprp());
        puts("}\n");
    }
    ശൂന്യം visit(AstCase* nodep) override {  // LCOV_EXCL_LINE
        // In V3Case...
        nodep->v3fatalSrc("Case statements should have been reduced out");
    }
    ശൂന്യം visit(AstComment* nodep) override {
        string at;
        എങ്കിൽ (nodep->showAt()) {
            at = " at " + nodep->fileline()->ascii();
            // If protecting, passthru less information about the design
            എങ്കിൽ (!v3Global.opt.protectIds()) തിരികെ;
        }
        എങ്കിൽ (!(nodep->protect() && v3Global.opt.protectIds())) {
            putsDecoration(nodep, "// "s + nodep->name() + at + "\n");
        }
        iterateChildrenConst(nodep);
    }
    ശൂന്യം visit(AstCoverOtherDecl* nodep) override {
        emit_Cover_Other_Decl_തിരുകുക(nodep);
        എങ്കിൽ (coverageUsesLocalCounter(nodep)) {
            // Some task bodies are cloned into a caller module where the
            // original instance-local counter is not addressable. Those clones
            // fall back to the global bin number, so register that slot too.
            emit_Cover_Other_Decl_തിരുകുക(nodep, true);
        }
    }
    ശൂന്യം visit(AstCoverToggleDecl* nodep) override {
        putns(nodep, "vlSelf->__vlCoverToggleInsert(");  // As Declared in emitCoverageDecl
        puts(cvtToStr(nodep->range().വലത്()));
        puts(", ");
        puts(cvtToStr(nodep->range().ഇടത്()));
        puts(", ");
        puts(cvtToStr(nodep->range().ranged()));
        puts(", ");
        putCoverageArray(nodep);
        puts(" + ");
        puts(cvtToStr(coverage_Bin_അക്കം(nodep)));
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
                   + VIdProtect::protect_വാക്കുകൾ_If(nodep->hier(), nodep->protect()));
        puts(", ");
        putsQuoted(VIdProtect::protect_വാക്കുകൾ_If(nodep->page(), nodep->protect()));
        puts(", ");
        putsQuoted(VIdProtect::protect_വാക്കുകൾ_If(nodep->comment(), nodep->protect()));
        puts(");\n");
    }
    ശൂന്യം visit(AstCoverInc* nodep) override {
        എങ്കിൽ (VN_IS(nodep->declp(), CoverOtherDecl)) {
            എങ്കിൽ (v3Global.opt.threads() > 1) {
                putns(nodep, "");
                putCoverageArray(nodep->declp());
                puts("[");
                puts(cvtToStr(coverage_Bin_അക്കം(nodep->declp())));
                puts("].fetch_add(1, std::memory_order_relaxed);\n");
            } അല്ലെങ്കിൽ {
                putns(nodep, "++(");
                putCoverageArray(nodep->declp());
                puts("[");
                puts(cvtToStr(coverage_Bin_അക്കം(nodep->declp())));
                puts("]);\n");
            }
        } അല്ലെങ്കിൽ {
            puts("VL_COV_TOGGLE_CHG_");
            എങ്കിൽ (v3Global.opt.threads() > 1) {
                puts("MT_");
            } അല്ലെങ്കിൽ {
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
            puts(cvtToStr(coverage_Bin_അക്കം(nodep->declp())));
            puts(", ");
            iterateConst(nodep->toggleExprp());
            puts(", ");
            iterateConst(nodep->toggleCovExprp());
            puts(");\n");
        }
    }
    ശൂന്യം visit(AstDisableFork* nodep) override { putns(nodep, "vlProcess->disableFork();\n"); }
    ശൂന്യം visit(AstCReturn* nodep) override {
        putns(nodep, "return (");
        iterateAndNextConstNull(nodep->lhsp());
        puts(");\n");
    }
    ശൂന്യം visit(AstDisplay* nodep) override {
        string text = nodep->fmtp()->text();
        എങ്കിൽ (nodep->addNewline()) text += "\n";
        display_കണ്ണി(nodep, nodep->fmtp(), text, nodep->fmtp()->exprsp(), false);
    }
    ശൂന്യം visit(AstDumpCtl* nodep) override {
        തിരഞ്ഞെടുപ്പ് (nodep->ctlType()) {
        അവസ്ഥ VDumpCtlType::FILE:
            putns(nodep, "vlSymsp->_vm_contextp__->dumpfile(");
            iterateConst(nodep->exprp());
            puts(");\n");
            നിർത്തുക;
        അവസ്ഥ VDumpCtlType::VARS:
            // We ignore number of levels to dump in exprp()
            എങ്കിൽ (v3Global.opt.trace()) {
                putns(nodep, "vlSymsp->_traceDumpOpen();\n");
            } അല്ലെങ്കിൽ {
                putns(nodep, "VL_PRINTF_MT(\"-Info: ");
                puts(V3OutFormatter::quoteNameControls(protect(nodep->fileline()->filename())));
                puts(":");
                puts(cvtToStr(nodep->fileline()->lineno()));
                puts(": $dumpvar ignored, as Verilated without --trace");
                puts("\\n\");\n");
            }
            നിർത്തുക;
        അവസ്ഥ VDumpCtlType::ALL:
            // $dumpall currently ignored
            നിർത്തുക;
        അവസ്ഥ VDumpCtlType::FLUSH:
            // $dumpall currently ignored; would need rework of VCD single thread,
            // or flag we pass-through to next eval() iteration
            നിർത്തുക;
        അവസ്ഥ VDumpCtlType::LIMIT:
            // $dumplimit currently ignored
            നിർത്തുക;
        അവസ്ഥ VDumpCtlType::OFF:
            // Currently ignored as both Vcd and Fst do not support them, as would need "X" dump
            നിർത്തുക;
        അവസ്ഥ VDumpCtlType::ON:
            // Currently ignored as $dumpoff is also ignored
            നിർത്തുക;
        സ്ഥിരസ്ഥിതി: nodep->v3fatalSrc("Bad case, unexpected " << nodep->ctlType().ascii());
        }
    }
    ശൂന്യം visit(AstScopeName* nodep) override {
        // For use under AstCCalls for dpiImports.  ScopeNames under
        // displays are handled in AstDisplay
        എങ്കിൽ (!nodep->dpiExport()) {
            // this is where the DPI import context scope is set
            സ്ഥിരം string scope = nodep->scopeDpiName();
            putnbs(nodep, "(vlSymsp->" + protect("__Vscopep_" + scope) + ")");
        }
    }
    ശൂന്യം visit(AstSFormat* nodep) override {
        display_കണ്ണി(nodep, nodep->fmtp(), nodep->fmtp()->text(), nodep->fmtp()->exprsp(), false);
    }
    ശൂന്യം visit(AstSFormatF* nodep) override {
        display_കണ്ണി(nodep, nodep, nodep->text(), nodep->exprsp(), false);
    }
    ശൂന്യം visit(AstFScanF* nodep) override {
        display_കണ്ണി(nodep, nullptr, nodep->text(), nodep->exprsp(), true);
    }
    ശൂന്യം visit(AstSScanF* nodep) override {
        display_കണ്ണി(nodep, nullptr, nodep->text(), nodep->exprsp(), true);
    }
    ശൂന്യം visit(Ast_മൂല്യം_Plus_Args* nodep) override {
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
    ശൂന്യം visit(AstTestPlusArgs* nodep) override {
        putns(nodep, "VL_TESTPLUSARGS_I(");
        iterateConst(nodep->searchp());
        puts(")");
    }
    ശൂന്യം visit(AstFError* nodep) override {
        putns(nodep, "VL_FERROR_I");
        puts(nodep->strp()->isString() ? "N(" : "W(");
        iterateAndNextConstNull(nodep->filep());
        putbs(", ");
        എങ്കിൽ (nodep->strp()->isWide()) {
            puts(cvtToStr(nodep->strp()->width_വാക്കുകൾ()));
            putbs(", ");
        }
        iterateAndNextConstNull(nodep->strp());
        puts(")");
    }
    ശൂന്യം visit(AstFGetS* nodep) override {
        emitOpName(nodep, nodep->emitC(), nodep->strgp(), nodep->filep(), nullptr);
    }
    ശൂന്യം visit(AstFOpen* nodep) override {
        putns(nodep, "VL_FOPEN_NN(");
        iterateConst(nodep->filenamep());
        putbs(", ");
        iterateConst(nodep->modep());
        puts(");\n");
    }
    ശൂന്യം visit(AstFOpenMcd* nodep) override {
        putns(nodep, "VL_FOPEN_MCD_N(");
        iterateConst(nodep->filenamep());
        puts(");\n");
    }
    ശൂന്യം visit(Ast_കണ്ണി_Read_Write_Mem* nodep) override {
        putns(nodep, nodep->cFuncPrefixp());
        puts("N(");
        puts(nodep->isHex() ? "true" : "false");
        putbs(", ");
        // Need real storage width
        puts(cvtToStr(nodep->memp()->dtypep()->skipRefp()->subDTypep()->widthMin()));
        uint32_t array_lo = 0;
        bool memUsePtr = true;
        {
            സ്ഥിരം AstVarRef* സ്ഥിരം varrefp = VN_CAST(nodep->memp(), VarRef);
            എങ്കിൽ (!varrefp) {
                nodep->v3error(nodep->verilogKwd() << " loading non-variable");
            } അല്ലെങ്കിൽ എങ്കിൽ (VN_IS(varrefp->varp()->dtypeSkipRefp(), AssocArrayDType)
                       || VN_IS(varrefp->varp()->dtypeSkipRefp(), DynArrayDType)) {
                // nodep->memp() below will when verilated code is compiled create a C++ template
                memUsePtr = false;
            } അല്ലെങ്കിൽ എങ്കിൽ (സ്ഥിരം AstUnpackArrayDType* സ്ഥിരം adtypep
                       = VN_CAST(varrefp->varp()->dtypeSkipRefp(), UnpackArrayDType)) {
                putbs(", ");
                puts(cvtToStr(varrefp->varp()->dtypep()->arrayUnpackedElements()));
                array_lo = adtypep->lo();
                putbs(", ");
                puts(cvtToStr(array_lo));
            } അല്ലെങ്കിൽ {
                nodep->v3error(
                    nodep->verilogKwd()
                    << " loading other than unpacked/dynamic/associative-array variable");
            }
        }
        putbs(", ");
        iterateConst(nodep->filenamep());
        putbs(", ");
        {
            എങ്കിൽ (memUsePtr) puts(" &(");
            iterateAndNextConstNull(nodep->memp());
            എങ്കിൽ (memUsePtr) puts(")");
        }
        putbs(", ");
        എങ്കിൽ (nodep->lsbp()) {
            iterateAndNextConstNull(nodep->lsbp());
        } അല്ലെങ്കിൽ {
            puts(cvtToStr(array_lo));
        }
        putbs(", ");
        എങ്കിൽ (nodep->msbp()) {
            iterateAndNextConstNull(nodep->msbp());
        } അല്ലെങ്കിൽ {
            puts("~0ULL");
        }
        puts(");\n");
    }
    ശൂന്യം visit(AstFClose* nodep) override {
        putns(nodep, "VL_FCLOSE_I(");
        iterateAndNextConstNull(nodep->filep());
        puts("); ");
    }
    ശൂന്യം visit(AstFFlush* nodep) override {
        എങ്കിൽ (!nodep->filep()) {
            putns(nodep, "VL_FFLUSH_MT();");
        } അല്ലെങ്കിൽ {
            putns(nodep, "VL_FFLUSH_I(");
            iterateAndNextConstNull(nodep->filep());
            puts(");\n");
        }
    }
    ശൂന്യം visit(AstFSeek* nodep) override {
        putns(nodep, "(VL_FSEEK_I(");
        iterateAndNextConstNull(nodep->filep());
        puts(", ");
        iterateAndNextConstNull(nodep->offset());
        puts(", ");
        iterateAndNextConstNull(nodep->operation());
        puts(") == -1 ? -1 : 0)");
    }
    ശൂന്യം visit(AstFTell* nodep) override {
        putns(nodep, "VL_FTELL_I(");
        iterateAndNextConstNull(nodep->filep());
        puts(")");
    }
    ശൂന്യം visit(AstFRewind* nodep) override {
        putns(nodep, "(VL_FSEEK_I(");
        iterateAndNextConstNull(nodep->filep());
        puts(", 0, 0) == -1 ? -1 : 0)");
    }
    ശൂന്യം visit(AstFRead* nodep) override {
        putns(nodep, "VL_FREAD_I(");
        puts(cvtToStr(nodep->memp()->widthMin()));  // Need real storage width
        putbs(", ");
        uint32_t array_lo = 0;
        uint32_t array_size = 0;
        {
            സ്ഥിരം AstVarRef* സ്ഥിരം varrefp = VN_CAST(nodep->memp(), VarRef);
            എങ്കിൽ (!varrefp) {
                nodep->v3error(nodep->verilogKwd() << " loading non-variable");
            } അല്ലെങ്കിൽ എങ്കിൽ (VN_CAST(varrefp->varp()->dtypeSkipRefp(), BasicDType)) {
            } അല്ലെങ്കിൽ എങ്കിൽ (സ്ഥിരം AstUnpackArrayDType* സ്ഥിരം adtypep
                       = VN_CAST(varrefp->varp()->dtypeSkipRefp(), UnpackArrayDType)) {
                array_lo = adtypep->lo();
                array_size = adtypep->elementsConst();
            } അല്ലെങ്കിൽ {
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
        എങ്കിൽ (nodep->startp()) {
            iterateAndNextConstNull(nodep->startp());
        } അല്ലെങ്കിൽ {
            puts(cvtToStr(array_lo));
        }
        putbs(", ");
        എങ്കിൽ (nodep->countp()) {
            iterateAndNextConstNull(nodep->countp());
        } അല്ലെങ്കിൽ {
            puts(cvtToStr(array_size));
        }
        puts(")");
    }
    ശൂന്യം visit(Ast_അട്ടി_Trace_F* nodep) override { putns(nodep, "VL_STACKTRACE_N()"); }
    ശൂന്യം visit(Ast_അട്ടി_Trace_T* nodep) override { putns(nodep, "VL_STACKTRACE();\n"); }
    ശൂന്യം visit(AstSystemT* nodep) override {
        putns(nodep, "(void)VL_SYSTEM_I");
        emitIQW(nodep->lhsp());
        puts("(");
        എങ്കിൽ (nodep->lhsp()->isWide()) {
            puts(cvtToStr(nodep->lhsp()->width_വാക്കുകൾ()));
            putbs(", ");
        }
        iterateAndNextConstNull(nodep->lhsp());
        puts(");\n");
    }
    ശൂന്യം visit(AstSystemF* nodep) override {
        putns(nodep, "VL_SYSTEM_I");
        emitIQW(nodep->lhsp());
        puts("(");
        എങ്കിൽ (nodep->lhsp()->isWide()) {
            puts(cvtToStr(nodep->lhsp()->width_വാക്കുകൾ()));
            putbs(", ");
        }
        iterateAndNextConstNull(nodep->lhsp());
        puts(")");
    }
    ശൂന്യം visit(Ast_Stmt_വ്യഞ്ജകം* nodep) override {
        എങ്കിൽ (VN_IS(nodep->exprp()->dtypep()->skipRefp(), VoidDType)) {
            putns(nodep, "");
        } അല്ലെങ്കിൽ {
            putns(nodep, "std::ignore = ");
        }
        iterateConst(nodep->exprp());
        puts(";\n");
    }
    ശൂന്യം visit(AstJumpBlock* nodep) override {
        // Allocate label number
        സ്ഥിരം size_t n = m_labelNumbers.size();
        സ്ഥിരം bool newEntry = m_labelNumbers.emplace(nodep, n).second;
        UASSERT_OBJ(newEntry, nodep, "AstJumpBlock visited twide");
        // Emit
        putns(nodep, "{\n");  // Make it visually obvious label jumps outside these
        VL_RESTORER(m_createdScopeHash);
        iterateAndNextConstNull(nodep->stmtsp());
        puts("__Vlabel" + std::to_string(n) + ": ;\n");
        puts("}\n");
    }
    ശൂന്യം visit(AstJumpGo* nodep) override {
        // Retrieve target label number - must already exist (from enclosing AstJumpBlock)
        സ്ഥിരം size_t n = m_labelNumbers.at(nodep->blockp());
        // Emit
        putns(nodep, "goto __Vlabel" + std::to_string(n) + ";\n");
    }
    ശൂന്യം visit(AstLoop* nodep) override {
        UASSERT_OBJ(!nodep->contsp(), nodep, "'contsp' only used before LinkJump");
        VL_RESTORER(m_createdScopeHash);
        // Special case when the AstLoopTest is first for output readability
        എങ്കിൽ (AstLoopTest* സ്ഥിരം testp = VN_CAST(nodep->stmtsp(), LoopTest)) {
            putns(nodep, "while (");
            iterateConst(testp->condp());
            puts(") {\n");
            iterateAndNextConstNull(testp->nextp());
            puts("}\n");
            തിരികെ;
        }
        // Special case when the AstLoopTest is last for output readability
        എങ്കിൽ (Ast_കണ്ണി* lastp = nodep->stmtsp()) {
            ആകുമ്പോൾ (Ast_കണ്ണി* സ്ഥിരം nextp = lastp->nextp()) lastp = nextp;
            എങ്കിൽ (AstLoopTest* സ്ഥിരം testp = VN_CAST(lastp, LoopTest)) {
                putns(nodep, "do {\n");
                വേണ്ടി (Ast_കണ്ണി* p = nodep->stmtsp(); p != lastp; p = p->nextp()) iterateConst(p);
                puts("} while (");
                iterateConst(testp->condp());
                puts(");\n");
                തിരികെ;
            }
        }
        // Emit generic case directly
        putns(nodep, "while (true) {\n");
        iterateAndNextConstNull(nodep->stmtsp());
        puts("}\n");
    }
    ശൂന്യം visit(AstLoopTest* nodep) override {
        VL_RESTORER(m_createdScopeHash);
        putns(nodep, "if (!(");
        iterateAndNextConstNull(nodep->condp());
        puts(")) break;\n");
    }
    ശൂന്യം visit(AstCLocalScope* nodep) override {
        putns(nodep, "{\n");
        VL_RESTORER(m_createdScopeHash);
        iterateAndNextConstNull(nodep->stmtsp());
        puts("}\n");
    }
    ശൂന്യം visit(Ast_കണ്ണി_If* nodep) override {
        putns(nodep, "if (");
        എങ്കിൽ (!nodep->branchPred().unknown()) {
            puts(nodep->branchPred().ascii());
            puts("((");  // Two parens, so that VL_UNLIKELY((class<foo,bar>)) works
        }
        iterateAndNextConstNull(nodep->condp());
        എങ്കിൽ (!nodep->branchPred().unknown()) puts("))");
        puts(") {\n");
        {
            VL_RESTORER(m_createdScopeHash);
            iterateAndNextConstNull(nodep->thensp());
        }
        puts("}");
        എങ്കിൽ (!nodep->elsesp()) {
            puts("\n");
        } അല്ലെങ്കിൽ {
            എങ്കിൽ (VN_IS(nodep->elsesp(), കണ്ണി_If) && !nodep->elsesp()->nextp()) {
                puts(" else ");
                iterateAndNextConstNull(nodep->elsesp());
            } അല്ലെങ്കിൽ {
                VL_RESTORER(m_createdScopeHash);
                puts(" else {\n");
                iterateAndNextConstNull(nodep->elsesp());
                puts("}\n");
            }
        }
    }
    ശൂന്യം visit(Ast_വ്യഞ്ജകം_Stmt* nodep) override {
        VL_RESTORER(m_createdScopeHash);
        സ്ഥിരം bool containsAwait = nodep->exists([](AstCAwait*) -> bool { തിരികെ true; });
        എങ്കിൽ (containsAwait) {
            UASSERT_OBJ(m_cfuncp && m_cfuncp->isCoroutine(), nodep,
                        "AstExprStmt with CAwait must be in coroutine");
            putnbs(nodep, "(co_await ([&]() -> VlCoroutine {\n");
            iterateAndNextConstNull(nodep->stmtsp());
            puts("co_return;\n");
            എങ്കിൽ (!nodep->hasResult()) {
                puts("}()))");
                തിരികെ;
            }
            puts("}()), ");
            iterateAndNextConstNull(nodep->resultp());
            puts(")");
            തിരികെ;
        }
        // GCC allows compound statements in expressions, but this is not standard.
        // So we use an immediate-evaluation lambda and comma operator
        putnbs(nodep, "([&]() {\n");
        എങ്കിൽ (!nodep->hasResult()) {
            iterateAndNextConstNull(nodep->stmtsp());
            puts("}())");
            തിരികെ;
        }
        iterateAndNextConstNull(nodep->stmtsp());
        puts("}(), ");
        iterateAndNextConstNull(nodep->resultp());
        puts(")");
    }
    ശൂന്യം visit(AstStop* nodep) override {
        putns(nodep, "VL_STOP_MT(");
        putsQuoted(protect(nodep->fileline()->filename()));
        puts(", ");
        puts(cvtToStr(nodep->fileline()->lineno()));
        puts(", \"\"");
        എങ്കിൽ (nodep->isFatal()) puts(", false");
        puts(");\n");
    }
    ശൂന്യം visit(AstFinish* nodep) override {
        // Disable all the forks so they don't operate after simulation is finished.
        എങ്കിൽ (m_cfuncp && m_cfuncp->needProcess()) putns(nodep, "vlProcess->disableFork();\n");
        putns(nodep, "VL_FINISH_MT(");
        putsQuoted(protect(nodep->fileline()->filename()));
        puts(", ");
        puts(cvtToStr(nodep->fileline()->lineno()));
        puts(", \"\");\n");
    }
    ശൂന്യം visit(AstFinishFork* nodep) override {
        // Disable all the forks so they don't operate after simulation is finished.
        എങ്കിൽ (m_cfuncp && m_cfuncp->needProcess()) putns(nodep, "vlProcess->disableFork();\n");
        putns(nodep, "VL_FINISH_MT(");
        putsQuoted(protect(nodep->fileline()->filename()));
        puts(", ");
        puts(cvtToStr(nodep->fileline()->lineno()));
        puts(", \"\");\n");
        എങ്കിൽ (m_cfuncp->isCoroutine()) {
            putns(nodep, "co_return;\n");
        } അല്ലെങ്കിൽ {
            putns(nodep, "return;\n");
        }
    }
    ശൂന്യം visit(AstPrintTimeScale* nodep) override {
        putns(nodep, "VL_PRINTTIMESCALE(");
        putsQuoted(protect(nodep->prettyName()));
        puts(", ");
        putsQuoted(nodep->timeunit().ascii());
        puts(", vlSymsp->_vm_contextp__);\n");
    }
    ശൂന്യം visit(AstRand* nodep) override {
        emitOpName(nodep, nodep->emitC(), nodep->seedp(), nullptr, nullptr);
    }
    ശൂന്യം visit(AstRandRNG* nodep) override {
        emitOpName(nodep, nodep->emitC(), nullptr, nullptr, nullptr);
    }
    ശൂന്യം visit(AstTime* nodep) override {
        putns(nodep, "VL_TIME_UNITED_Q(");
        UASSERT_OBJ(!nodep->timeunit().isNone(), nodep, "$time has no units");
        സ്ഥിരം ഇരട്ട time
            = nodep->timeunit().multiplier() / v3Global.rootp()->timeprecision().multiplier();
        UASSERT_OBJ(time >= 1, nodep, "TimeQ is less than 1, will result in division by zero");
        puts(cvtToStr(time));
        puts(")");
    }
    ശൂന്യം visit(AstTimeD* nodep) override {
        putns(nodep, "VL_TIME_UNITED_D(");
        UASSERT_OBJ(!nodep->timeunit().isNone(), nodep, "$realtime has no units");
        puts(cvtToStr(nodep->timeunit().multiplier()
                      / v3Global.rootp()->timeprecision().multiplier()));
        puts(")");
    }
    ശൂന്യം visit(AstGetInitialRandomSeed* nodep) override {
        putns(nodep, "vlSymsp->_vm_contextp__->randSeed()");
    }
    ശൂന്യം visit(AstTimeFormat* nodep) override {
        putns(nodep, "VL_TIMEFORMAT_IINI(");
        എങ്കിൽ (nodep->unitsp()) {
            puts("true, ");
            iterateAndNextConstNull(nodep->unitsp());
        } അല്ലെങ്കിൽ {
            puts("false, 0");
        }
        puts(", ");
        എങ്കിൽ (nodep->precisionp()) {
            puts("true, ");
            iterateAndNextConstNull(nodep->precisionp());
        } അല്ലെങ്കിൽ {
            puts("false, 0");
        }
        puts(", ");
        എങ്കിൽ (nodep->suffixp()) {
            puts("true, ");
            iterateAndNextConstNull(nodep->suffixp());
        } അല്ലെങ്കിൽ {
            puts("false, \"\"");
        }
        puts(", ");
        എങ്കിൽ (nodep->widthp()) {
            puts("true, ");
            iterateAndNextConstNull(nodep->widthp());
        } അല്ലെങ്കിൽ {
            puts("false, 0");
        }
        puts(", vlSymsp->_vm_contextp__);\n");
    }
    ശൂന്യം visit(AstTimePrecision* nodep) override {
        putns(nodep, "vlSymsp->_vm_contextp__->timeprecision()");
    }

    // Nodes involing AstText
    ശൂന്യം visit(AstText* nodep) override {
        // All Text should be under TextBlock/CStmt/CStmtUser/CExpr/CExprUser
        nodep->v3fatalSrc("Text node in unexpected position");
    }
    ശൂന്യം visit(AstTextBlock* nodep) override {
        putnbs(nodep, "");
        puts(nodep->prefix());
        emitNodesWithText(nodep->nodesp(), false, true, nodep->separator());
        puts(nodep->suffix());
    }
    ശൂന്യം visit(AstCStmt* nodep) override {
        putnbs(nodep, "");
        emitNodesWithText(nodep->nodesp(), false, true, "");
        ensure_New_വരി();
    }
    ശൂന്യം visit(AstCExpr* nodep) override {
        putnbs(nodep, "");
        emitNodesWithText(nodep->nodesp(), false, true, "");
    }
    ശൂന്യം visit(AstCStmtUser* nodep) override {
        putnbs(nodep, "");
        ofp()->putsNoTracking("\n");
        എങ്കിൽ (nodep->fromDollarC() && v3Global.opt.decoration() && !v3Global.opt.protectIds()) {
            ofp()->putsNoTracking("// $c statement at " + nodep->fileline()->ascii() + "\n");
        }
        emitNodesWithText(nodep->nodesp(), m_useSelfForThis, false, "");
        puts("\n");
    }
    ശൂന്യം visit(AstCExprUser* nodep) override {
        putnbs(nodep, "");
        ofp()->putsNoTracking("\n");
        എങ്കിൽ (/* is always from $c */ v3Global.opt.decoration() && !v3Global.opt.protectIds()) {
            ofp()->putsNoTracking(
                (nodep->isPure() ? "// $cpure expression at " : "// $c expression at ")
                + nodep->fileline()->ascii() + "\n");
        }
        emitNodesWithText(nodep->nodesp(), m_useSelfForThis, false, "");
        puts("\n");
    }

    // Operators
    ശൂന്യം visit(Ast_കണ്ണി_Termop* nodep) override {
        emitOpName(nodep, nodep->emitC(), nullptr, nullptr, nullptr);
    }
    ശൂന്യം visit(Ast_കണ്ണി_Uniop* nodep) override {
        എങ്കിൽ (nodep->emit_Check_Max_വാക്കുകൾ()
            && (nodep->width_വാക്കുകൾ() > VL_MULS_MAX_വാക്കുകൾ
                || nodep->lhsp()->width_വാക്കുകൾ() > VL_MULS_MAX_വാക്കുകൾ)) {
            nodep->v3warn(
                E_UNSUPPORTED,
                "Unsupported: "
                    << nodep->prettyOperatorName() << " operator of " << nodep->width()
                    << " bits exceeds hardcoded limit VL_MULS_MAX_WORDS in verilatedos.h");
        }
        എങ്കിൽ (emitSimpleOk(nodep)) {
            putnbs(nodep, "(");
            puts(nodep->emitSimpleOperator());
            puts(" ");
            iterateAndNextConstNull(nodep->lhsp());
            puts(")");
        } അല്ലെങ്കിൽ {
            emitOpName(nodep, nodep->emitC(), nodep->lhsp(), nullptr, nullptr);
        }
    }
    ശൂന്യം visit(Ast_കണ്ണി_Biop* nodep) override {
        എങ്കിൽ (nodep->emit_Check_Max_വാക്കുകൾ() && nodep->width_വാക്കുകൾ() > VL_MULS_MAX_വാക്കുകൾ) {
            nodep->v3warn(
                E_UNSUPPORTED,
                "Unsupported: "
                    << nodep->prettyOperatorName() << " operator of " << nodep->width()
                    << " bits exceeds hardcoded limit VL_MULS_MAX_WORDS in verilatedos.h");
        }
        എങ്കിൽ (emitSimpleOk(nodep)) {
            putnbs(nodep, "(");
            iterateAndNextConstNull(nodep->lhsp());
            puts(" ");
            putbs(nodep->emitSimpleOperator());
            puts(" ");
            iterateAndNextConstNull(nodep->rhsp());
            puts(")");
        } അല്ലെങ്കിൽ {
            emitOpName(nodep, nodep->emitC(), nodep->lhsp(), nodep->rhsp(), nullptr);
        }
    }
    ശൂന്യം visit(Ast_കണ്ണി_Triop* nodep) override {
        UASSERT_OBJ(!emitSimpleOk(nodep), nodep, "Triop cannot be described in a simple way");
        emitOpName(nodep, nodep->emitC(), nodep->lhsp(), nodep->rhsp(), nodep->thsp());
    }
    ശൂന്യം visit(AstCvtPackString* nodep) override { emitCvtPackStr(nodep->lhsp()); }
    ശൂന്യം visit(AstRedXor* nodep) override {
        എങ്കിൽ (nodep->lhsp()->isWide()) {
            visit(static_cast<Ast_കണ്ണി_Uniop*>(nodep));
        } അല്ലെങ്കിൽ {
            സ്ഥിരം AstVarRef* സ്ഥിരം vrefp = VN_CAST(nodep->lhsp(), VarRef);
            സ്ഥിരം പൂർണ്ണസംഖ്യ widthPow2 = vrefp ? vrefp->varp()->dtypep()->widthPow2()
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
    ശൂന്യം visit(AstCCast* nodep) override {
        // Extending a value of the same word width is just a NOP.
        എങ്കിൽ (സ്ഥിരം AstClassRefDType* സ്ഥിരം classDtypep
            = VN_CAST(nodep->dtypep()->skipRefp(), ClassRefDType)) {
            putns(nodep, "(" + classDtypep->cType("", false, false) + ")(");
        } അല്ലെങ്കിൽ എങ്കിൽ (nodep->size() <= VL_BYTESIZE) {
            putns(nodep, "(::CData)(");
        } അല്ലെങ്കിൽ എങ്കിൽ (nodep->size() <= VL_SHORTSIZE) {
            putns(nodep, "(::SData)(");
        } അല്ലെങ്കിൽ എങ്കിൽ (nodep->size() <= VL_IDATASIZE) {
            putns(nodep, "(::IData)(");
        } അല്ലെങ്കിൽ {
            putns(nodep, "(::QData)(");
        }
        iterateAndNextConstNull(nodep->lhsp());
        puts(")");
    }
    ശൂന്യം visit(AstCond* nodep) override {
        // Widths match up already, so we'll just use C++'s operator w/o any temps.
        എങ്കിൽ (nodep->thenp()->isWide()) {
            emitOpName(nodep, nodep->emitC(), nodep->condp(), nodep->thenp(), nodep->elsep());
        } അല്ലെങ്കിൽ {
            putnbs(nodep, "(");
            iterateAndNextConstNull(nodep->condp());
            putbs(" ? ");
            iterateAndNextConstNull(nodep->thenp());
            putbs(" : ");
            iterateAndNextConstNull(nodep->elsep());
            puts(")");
        }
    }
    ശൂന്യം visit(AstMatchMasked* nodep) override {
        emitOpName(nodep, nodep->emitC(), nodep->lhsp(), nodep->matchp(), nullptr);
    }
    ശൂന്യം visit(AstMemberSel* nodep) override {
        സ്ഥിരം AstVar* സ്ഥിരം varp = nodep->varp();
        സ്ഥിരം bool dereferenceCovergroupRef
            = varp->covergroupRefMember() && nodep->access().isReadOrRW();
        എങ്കിൽ (dereferenceCovergroupRef) putnbs(nodep, "(*");
        iterateAndNextConstNull(nodep->fromp());
        putnbs(nodep, "->");
        എങ്കിൽ (varp->isIfaceRef()) {
            // varp is the __Viftop companion (e.g. "tx__Viftop"); use the
            // MemberSel name which matches the cell's C++ member (e.g. "tx").
            puts(nodep->nameProtect());
        } അല്ലെങ്കിൽ {
            puts(varName(varp));
        }
        എങ്കിൽ (dereferenceCovergroupRef) puts(")");
    }
    ശൂന്യം visit(AstStructSel* nodep) override {
        iterateAndNextConstNull(nodep->fromp());
        putnbs(nodep, ".");
        puts(nodep->nameProtect());
    }
    ശൂന്യം visit(AstNullCheck* nodep) override {
        putns(nodep, "VL_NULL_CHECK(");
        iterateAndNextConstNull(nodep->lhsp());
        puts(", ");
        putsQuoted(protect(nodep->fileline()->filename()));
        puts(", ");
        puts(cvtToStr(nodep->fileline()->lineno()));
        puts(")");
    }
    ശൂന്യം visit(AstNewCopy* nodep) override {
        // Polymorphic shallow clone: preserves runtime type via virtual clone()
        // VL_NULL_CHECK enforces null check per IEEE 1800-2023 8.7
        putns(nodep, "VL_NULL_CHECK(");
        എങ്കിൽ (VN_IS(nodep->rhsp(), Const) && VN_AS(nodep->rhsp(), Const)->isNull()) {
            // V3Const folded rhs to null: emit a typed empty ref so VL_NULL_CHECK fires
            സ്ഥിരം AstClassRefDType* സ്ഥിരം refDTypep
                = VN_CAST(nodep->dtypep()->skipRefp(), ClassRefDType);
            puts(refDTypep->cType("", false, false) + "{}");
        } അല്ലെങ്കിൽ {
            iterateAndNextConstNull(nodep->rhsp());
        }
        puts(", ");
        putsQuoted(protect(nodep->fileline()->filename()));
        puts(", ");
        puts(cvtToStr(nodep->fileline()->lineno()));
        puts(").clone(vlSymsp->__Vm_deleter)");
    }
    ശൂന്യം visit(AstSel* nodep) override {
        // Note ASSIGN checks for this on a LHS
        UASSERT_OBJ(nodep->widthMin() == nodep->widthConst(), nodep, "Width mismatch");
        emitOpName(nodep, nodep->emitC(), nodep->fromp(), nodep->lsbp(), nullptr);
    }
    ശൂന്യം visit(AstReplicate* nodep) override {
        എങ്കിൽ (nodep->srcp()->widthMin() == 1 && !nodep->isWide()) {
            UASSERT_OBJ((static_cast<പൂർണ്ണസംഖ്യ>(VN_AS(nodep->countp(), Const)->toUInt())
                         * nodep->srcp()->widthMin())
                            == nodep->widthMin(),
                        nodep, "Replicate non-constant or width miscomputed");
            putns(nodep, "VL_REPLICATE_");
            emitIQW(nodep);
            puts("OI(");
            എങ്കിൽ (nodep->srcp()) puts(cvtToStr(nodep->srcp()->widthMin()));
            puts(", ");
            iterateAndNextConstNull(nodep->srcp());
            puts(", ");
            iterateAndNextConstNull(nodep->countp());
            puts(")");
        } അല്ലെങ്കിൽ {
            emitOpName(nodep, nodep->emitC(), nodep->srcp(), nodep->countp(), nullptr);
        }
    }
    ശൂന്യം emitStreamR(AstStreamR* nodep, Ast_കണ്ണി* parent) {
        //TODO: This might need to handle more cases like the visit(AstStreamR) function
        emitOpName(nodep, nodep->emitC(), nodep->lhsp(), nodep->rhsp(), nullptr);
    }
    ശൂന്യം visit(AstStreamR* nodep) override {
        //The parrent node of our AstStreamR will give just enough info for what streamR should
        //output if nodep->backp() is not the parent then emitStreamR should have been used. throw
        //an error
        bool backpIsParent = (nodep->backp()->op1p() == nodep || nodep->backp()->op2p() == nodep);
        UASSERT(backpIsParent, "can not find return type for streamR");
        എങ്കിൽ (സ്ഥിരം AstQueueDType* സ്ഥിരം qdtp
            = VN_CAST(nodep->backp()->dtypep()->skipRefp(), QueueDType)) {
            എങ്കിൽ (nodep->lhsp()->isWide()) {
                emitOpName(nodep,
                           "VL_STREAMR_%nq%lq%rq(%lw," + std::to_string(qdtp->subDTypep()->width())
                               + ", %P, %li, %ri)",
                           nodep->lhsp(), nodep->rhsp(), nullptr);
            } അല്ലെങ്കിൽ {
                emitOpName(nodep, "VL_STREAMR_%nq%lq%rq(%lw, %P, %li, %ri)", nodep->lhsp(),
                           nodep->rhsp(), nullptr);
            }
        } അല്ലെങ്കിൽ എങ്കിൽ (VN_IS(nodep->lhsp()->dtypep()->skipRefp(), QueueDType)) {
            എങ്കിൽ (!((nodep->backp()->op1p() && nodep->backp()->op1p()->isWide())
                  || (nodep->backp()->op2p() && nodep->backp()->op2p()->isWide()))) {
                //If our lhsp is a queue make sure we streamR and return the correct type.
                //If either side is wide or the previous node is string type dont use this case
                emitOpName(nodep->backp(), "VL_STREAMR_%nq%lq%rq(%lw, %P, %li, %ri)",
                           nodep->lhsp(), nodep->rhsp(), nullptr);
            } അല്ലെങ്കിൽ {
                emitOpName(nodep, nodep->emitC(), nodep->lhsp(), nodep->rhsp(), nullptr);
            }
        } അല്ലെങ്കിൽ {
            emitOpName(nodep, nodep->emitC(), nodep->lhsp(), nodep->rhsp(), nullptr);
        }
    }
    ശൂന്യം visit(AstStreamL* nodep) override {
        // Attempt to use a "fast" stream function for slice size = power of 2
        എങ്കിൽ (!nodep->isWide()) {
            സ്ഥിരം uint32_t isPow2 = VN_AS(nodep->rhsp(), Const)->അക്കം().എണ്ണം_Ones() == 1;
            സ്ഥിരം uint32_t sliceSize = VN_AS(nodep->rhsp(), Const)->toUInt();
            എങ്കിൽ (isPow2 && sliceSize <= (nodep->isQuad() ? വലുപ്പം(uint64_t) : വലുപ്പം(uint32_t))) {
                putns(nodep, "VL_STREAML_FAST_");
                bool usesQueue = false;
                AstQueueDType* qtypep
                    = nodep->backp()->op2p()
                          ? VN_CAST(nodep->backp()->op2p()->dtypep()->skipRefp(), QueueDType)
                          : nullptr;
                എങ്കിൽ (VN_IS(nodep->backp(), Assign)
                    && qtypep) {  // If we are assigning to a queue then emit the correct symbol
                    puts("R");  // R for queue
                    usesQueue = true;
                } അല്ലെങ്കിൽ {
                    emitIQW(nodep);
                }
                emitIQW(nodep->lhsp());
                puts("I(");
                puts(cvtToStr(nodep->lhsp()->widthMin()));
                puts(", ");
                എങ്കിൽ (usesQueue) {
                    iterateAndNextConstNull(nodep->backp()->op2p());
                    puts(", ");
                }
                iterateAndNextConstNull(nodep->lhsp());
                puts(", ");
                സ്ഥിരം uint32_t rd_log2 = V3_സംഖ്യ::log2b(VN_AS(nodep->rhsp(), Const)->toUInt());
                puts(cvtToStr(rd_log2) + ")");
                തിരികെ;
            }
        }
        എങ്കിൽ (VN_IS(nodep->backp(), Assign)
            && VN_IS(nodep->backp()->op2p()->dtypep()->skipRefp(), QueueDType)) {
            പൂർണ്ണസംഖ്യ queueWidth
                = nodep->backp()->op2p()->dtypep()->subDTypep()->width();  //We need to know the
                                                                           //width of both sides
            emitOpName(nodep,
                       "VL_STREAML_%nq%lq%rq(%lw," + std::to_string(queueWidth)
                           + ", %P, %li, %ri)",
                       nodep->lhsp(), nodep->rhsp(), nullptr);
        } അല്ലെങ്കിൽ {
            emitOpName(nodep, nodep->emitC(), nodep->lhsp(), nodep->rhsp(), nullptr);
        }
    }
    ശൂന്യം visit(AstCastDynamic* nodep) override {
        putnbs(nodep, "VL_CAST_DYNAMIC(");
        iterateAndNextConstNull(nodep->fromp());
        puts(", ");
        iterateAndNextConstNull(nodep->മുകൾ());
        puts(")");
    }
    ശൂന്യം visit(Ast_എണ്ണം_Bits* nodep) override {
        putnbs(nodep, "VL_COUNTBITS_");
        emitIQW(nodep->lhsp());
        puts("(");
        puts(cvtToStr(nodep->lhsp()->widthMin()));
        puts(", ");
        എങ്കിൽ (nodep->lhsp()->isWide()) {
            puts(cvtToStr(nodep->lhsp()->width_വാക്കുകൾ()));  // Note argument width, not node width
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
    ശൂന്യം visit(AstTriggered* nodep) override {
        // Test each word of the trigger vector with a mask of the relevant bits
        std::map<uint32_t, uint64_t> wordMasks;
        വേണ്ടി (സ്ഥിരം uint32_t index : nodep->indices()) {
            wordMasks[VL_BITWORD_Q(index)] |= VL_BIT_Q(index);
        }
        putns(nodep, "((");
        bool first = true;
        വേണ്ടി (സ്ഥിരം auto& wordMask : wordMasks) {
            എങ്കിൽ (!first) puts("\n| ");
            first = false;
            സ്ഥിരം bool allOnes = wordMask.second == ~0ULL;
            എങ്കിൽ (!allOnes) puts("(" + V3_സംഖ്യ{nodep, 64}.setQuad(wordMask.second).emitC() + " & ");
            iterateConst(nodep->trigRefp());
            puts("[" + std::to_string(wordMask.first) + "U]");
            എങ്കിൽ (!allOnes) puts(")");
        }
        puts(") != 0ULL)");
    }
    // Terminals
    ശൂന്യം visit(AstVarRef* nodep) override {
        സ്ഥിരം AstVar* സ്ഥിരം varp = nodep->varp();

        // Reference to constant pool variable is special
        എങ്കിൽ (varp->constPoolEntry()) {
            putns(nodep, EmitCUtil::constPoolName(varp));
            തിരികെ;
        }

        സ്ഥിരം Ast_കണ്ണി_Module* സ്ഥിരം varModp = EmitCParentModule::get(varp);
        സ്ഥിരം bool dereferenceCovergroupRef
            = varp->covergroupRefMember() && nodep->access().isReadOrRW();
        എങ്കിൽ (dereferenceCovergroupRef) putns(nodep, "(*");
        എങ്കിൽ (VN_IS(varModp, Class) && varModp != m_modp) {
            // Superclass member reference
            putns(nodep, EmitCUtil::prefixNameProtect(varModp) + "::");
        } അല്ലെങ്കിൽ എങ്കിൽ (varp->isIfaceRef()) {
            putns(nodep, nodep->selfPointerProtect(m_useSelfForThis));
            തിരികെ;
        } അല്ലെങ്കിൽ എങ്കിൽ (!nodep->selfPointer().isEmpty()) {
            emitDereference(nodep, nodep->selfPointerProtect(m_useSelfForThis));
        }
        putns(nodep, varName(varp));
        എങ്കിൽ (dereferenceCovergroupRef) puts(")");
    }
    ശൂന്യം visit(AstAddrOfCFunc* nodep) override {
        // Note: Can be thought to handle more, but this is all that is needed right now
        സ്ഥിരം AstCFunc* സ്ഥിരം funcp = nodep->funcp();
        UASSERT_OBJ(funcp->isLoose(), nodep, "Cannot take address of non-loose method");
        putns(nodep, "&");
        puts(funcNameProtect(funcp));
    }
    ശൂന്യം visit(AstConst* nodep) override {  //
        എങ്കിൽ (m_wideTempRefp && nodep->isWide()) {
            UASSERT_OBJ(m_wideTempRefp, nodep, "Wide Constant w/ no temp");
            emitConstantW(nodep, m_wideTempRefp);
            m_wideTempRefp = nullptr;  // We used it, fail if set it a second time
        } അല്ലെങ്കിൽ {
            emitConstant(nodep);
        }
    }
    ശൂന്യം visit(Ast_കണ്ണി_Cons* nodep) override { emitValCons(nodep); }
    ശൂന്യം visit(AstThisRef* nodep) override {
        putnbs(nodep, nodep->dtypep()->cType("", false, false));
        puts("{");
        puts(VSelfPointerText::replaceThis(m_useSelfForThis, "this"));
        puts("}");
    }
    ശൂന്യം visit(Ast_കണ്ണി_Sel* nodep) override {
        എങ്കിൽ (!VN_IS(nodep, ArraySel) && !VN_IS(nodep, WordSel)) {
            visit(static_cast<Ast_കണ്ണി_Biop*>(nodep));
            തിരികെ;
        }
        // ArraySel or WordSel
        iterateAndNextConstNull(nodep->fromp());
        // Special case constant index for readability
        എങ്കിൽ (AstConst* സ്ഥിരം idxp = VN_CAST(nodep->bitp(), Const)) {
            puts("[" + std::to_string(idxp->toUInt()) + "U]");
            തിരികെ;
        }
        putbs("[");
        iterateAndNextConstNull(nodep->bitp());
        puts("]");
    }

    //
    ശൂന്യം visit(AstExecGraph* nodep) override {
        // The location of the AstExecGraph within the containing AstCFunc is where we want to
        // invoke the graph and wait for it to complete. The run-time library does just that.
        putns(nodep, "vl_invokeExecGraph(" + EmitCUtil::execGraphName(nodep) + ", vlSelf, ");
        iterateConst(nodep->triggerp());
        puts(".data());\n");
    }

    // Default
    ശൂന്യം visit(Ast_കണ്ണി* nodep) override {  // LCOV_EXCL_START
        putns(nodep, "\n???? // "s + nodep->prettyTypeName() + "\n");
        iterateChildrenConst(nodep);
        എങ്കിൽ (!v3Global.opt.lintOnly()) {  // An internal problem, so suppress
            nodep->v3fatalSrc("Unknown node type reached emitter: " << nodep->prettyTypeName());
        }
    }  // LCOV_EXCL_STOP

സംരക്ഷിത:
    EmitCFunc() = സ്ഥിരസ്ഥിതി;
    ~EmitCFunc() override = സ്ഥിരസ്ഥിതി;
};

#endif  // guard
