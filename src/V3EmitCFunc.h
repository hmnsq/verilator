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

#నిర్వచితం_కాకపోతే VERILATOR_V3EMITCFUNC_H_
#నిర్వచించు VERILATOR_V3EMITCFUNC_H_

#అన్తర్భావ "config_build.h"
#అన్తర్భావ "verilatedos.h"

#అన్తర్భావ "V3EmitCConstInit.h"
#అన్తర్భావ "V3Global.h"
#అన్తర్భావ "V3MemberMap.h"

#అన్తర్భావ <algorithm>
#అన్తర్భావ <map>
#అన్తర్భావ <unordered_map>
#అన్తర్భావ <unordered_set>
#అన్తర్భావ <vector>

// Number of VL_CONST_W_*X's in verilated.h (IE VL_CONST_W_8X is last)
constexpr పూర్ణాంకం EMITC_అంకె_CONSTW = 8;

//######################################################################
// Emit lazy forward declarations

class EmitCLazyDecls final : బహిరంగ VNVisitorConst {
    // NODE STATE/TYPES
    // None allowed to support threaded emitting

    // MEMBERS
    std::unordered_సమితి<string> m_emittedManually;  // Set of names already declared manually.
    EmitCBaseVisitorConst& m_emitter;  // For access to file output
    bool m_needs_Blank_పంక్తి = false;  // Emit blank line if any declarations were emitted (cosmetic)
    std::సమితి<Ast_కణుపు*> m_emitted;  // -> in set. Already emitted decl for symbols.

    // METHODS
    bool declaredOnce(Ast_కణుపు* nodep) { తిరిగి m_emitted.చొప్పించు(nodep).రెండవ; }

    శూన్య lazyDeclare(AstCFunc* funcp) {
        // Already declared in this compilation unit
        ఒకవేళ (!declaredOnce(funcp)) తిరిగి;
        // Check if this kind of function is lazily declared
        ఒకవేళ (!(funcp->is_పద్ధతి() && funcp->isLoose()) && !funcp->dpiImportPrototype()) తిరిగి;
        // Already declared manually
        ఒకవేళ (m_emittedManually.లెక్క(funcp->nameProtect())) తిరిగి;
        // Needs lazy declaration, emit one
        m_emitter.emitCFuncDecl(funcp, EmitCParentModule::get(funcp), funcp->dpiImportPrototype());
        m_needs_Blank_పంక్తి = true;
    }

    శూన్య lazyDeclareConstPoolVar(AstVar* varp) {
        ఒకవేళ (!declaredOnce(varp)) తిరిగి;  // Already declared
        స్థిరరాశి string nameProtect = EmitCUtil::constPoolName(varp);
        m_emitter.putns(varp, "extern const ");
        m_emitter.puts(varp->dtypep()->cType(nameProtect, false, false));
        m_emitter.puts(";\n");
        m_needs_Blank_పంక్తి = true;
    }

    // VISITORS
    శూన్య సందర్శించు(Ast_కణుపు_CCall* nodep) override {
        lazyDeclare(nodep->funcp());
        iterateChildrenConst(nodep);
    }

    శూన్య సందర్శించు(Ast_Addr_యొక్క_CFunc* nodep) override {
        lazyDeclare(nodep->funcp());
        iterateChildrenConst(nodep);
    }

    శూన్య సందర్శించు(AstVarRef* nodep) override {
        AstVar* స్థిరరాశి varp = nodep->varp();
        // Only constant pool symbols are lazy declared for now ...
        ఒకవేళ (EmitCParentModule::get(varp)->isConstPool()) lazyDeclareConstPoolVar(varp);
    }

    శూన్య సందర్శించు(Ast_Exec_లేఖాచిత్రం* nodep) override {
        ఒకవేళ (!declaredOnce(nodep)) తిరిగి;
        m_emitter.putns(nodep, "extern VlExecGraph " + EmitCUtil::exec_లేఖాచిత్రం_Name(nodep) + ";\n");
        m_needs_Blank_పంక్తి = true;
    }

    శూన్య సందర్శించు(Ast_కణుపు* nodep) override { iterateChildrenConst(nodep); }

బహిరంగ:
    explicit EmitCLazyDecls(EmitCBaseVisitorConst& emitter)
        : m_emitter{emitter} {}
    శూన్య emit(Ast_కణుపు* nodep) {
        m_needs_Blank_పంక్తి = false;
        iterateChildrenConst(nodep);
        ఒకవేళ (m_needs_Blank_పంక్తి) m_emitter.puts("\n");
    }
    శూన్య emit(స్థిరరాశి string& ఉపసర్గ, స్థిరరాశి string& name, స్థిరరాశి string& suffix) {
        m_emittedManually.చొప్పించు(name);
        m_emitter.ensure_New_పంక్తి();
        m_emitter.puts(ఉపసర్గ);
        m_emitter.puts(name);
        m_emitter.puts(suffix);
        m_emitter.ensure_New_పంక్తి();
    }
    శూన్య declared(AstCFunc* nodep) { m_emitted.చొప్పించు(nodep); }
    శూన్య reset() { m_emitted.clear(); }
};

// ######################################################################
//  Emit statements and expressions

class EmitCFunc VL_NOT_FINAL : బహిరంగ EmitCConstInit {
    VMemberMap m_memberMap;
    AstVarRef* m_wide_తాత్కాలిక_Refp = nullptr;  // Variable that _WW macros should be setting
    std::unordered_map<Ast_Jump_ఖండం*, కొలత_t> m_labelNumbers;  // Label numbers for AstJumpBlocks
    bool m_createdScopeHash = false;  // Already created a scope hash

రక్షిత:
    VL_DEFINE_DEBUG_FUNCTIONS;

    EmitCLazyDecls m_lazyDecls{*this};  // Visitor for emitting lazy declarations
    bool m_useSelfForThis = false;  // Replace "this" with "vlSelf"
    bool m_usevlSelfRef = false;  // Use vlSelfRef reference instead of vlSelf pointer
    స్థిరరాశి Ast_కణుపు_Module* m_modp = nullptr;  // Current module being emitted
    స్థిరరాశి AstCFunc* m_cfuncp = nullptr;  // Current function being emitted
    bool m_instantiatesOwnProcess = false;
    స్థిరరాశి AstClassPackage* m_classOrPackage = nullptr;  // Pointer to current class or package
    string m_classOrPackageHash;  // Hash of class or package name

    bool constructorNeedsProcess(స్థిరరాశి AstClass* స్థిరరాశి classp) {
        స్థిరరాశి Ast_కణుపు* స్థిరరాశి newp = m_memberMap.కనుగొను_Member(classp, "new");
        ఒకవేళ (!newp) తిరిగి false;
        స్థిరరాశి AstCFunc* స్థిరరాశి ctorp = VN_CAST(newp, CFunc);
        ఒకవేళ (!ctorp) తిరిగి false;
        UASSERT_OBJ(ctorp->isConstructor(), ctorp, "`new` is not a constructor!");
        తిరిగి ctorp->needProcess();
    }

    bool constructorNeedsProcess(స్థిరరాశి Ast_కణుపు_DType* స్థిరరాశి dtypep) {
        ఒకవేళ (స్థిరరాశి AstClassRefDType* స్థిరరాశి crefdtypep = VN_CAST(dtypep, ClassRefDType))
            తిరిగి constructorNeedsProcess(crefdtypep->classp());
        తిరిగి false;
    }

    bool coverage_ఉపయోగిస్తుంది_Local_Counter(Ast_కణుపు_Cover_Decl* స్థిరరాశి declp) స్థిరరాశి {
        // Only functions with an object receiver can address the module-local
        // coverage array. Static/package/class helper functions still use the
        // symbol-table array, as they may have vlSymsp but no vlSelf/this.
        // Also require the coverage declaration to belong to the current
        // module; cloned task bodies can increment declarations owned by a
        // different module, and those must keep using the global array.
        తిరిగి m_cfuncp && !m_cfuncp->isStatic() && !VN_IS(m_modp, Class)
               && !VN_IS(m_modp, ClassPackage)
               && EmitCParentModule::get(declp->దత్తాంశం_Decl_Thisp()) == m_modp;
    }
    పూర్ణాంకం coverage_Bin_అంకె(Ast_కణుపు_Cover_Decl* స్థిరరాశి declp, bool forceGlobal = false) స్థిరరాశి {
        తిరిగి !forceGlobal && coverage_ఉపయోగిస్తుంది_Local_Counter(declp)
                   ? declp->దత్తాంశం_Decl_Thisp()->local_Bin_అంకె()
                   : declp->దత్తాంశం_Decl_Thisp()->bin_అంకె();
    }
    శూన్య putCoverageArray(Ast_కణుపు_Cover_Decl* స్థిరరాశి declp, bool forceGlobal = false) {
        ఒకవేళ (!forceGlobal && coverage_ఉపయోగిస్తుంది_Local_Counter(declp)) {
            // Keep counters on the module object when possible, so every
            // no-inline instance has storage available if coverage output is
            // later written with forcePerInstance.
            puts(m_cfuncp->isLoose() ? "vlSelf->__Vcoverage" : "this->__Vcoverage");
        } లేకపోతే {
            // Static/package/class helper functions do not have vlSelf, so
            // keep using the shared symbol-table coverage array there.
            puts("vlSymsp->__Vcoverage");
        }
    }
    శూన్య emit_Cover_Other_Decl_చొప్పించు(AstCoverOtherDecl* nodep, bool forceGlobal = false) {
        putns(nodep, "vlSelf->__vlCoverInsert(");  // As Declared in emitCoverageDecl
        putCoverageArray(nodep, forceGlobal);
        puts(" + ");
        puts(cvt_గమ్యం_Str(coverage_Bin_అంకె(nodep, forceGlobal)));
        // first controls whether duplicate module instances are collapsed in
        // the default coverage view. The following flag says whether countp
        // points to an object-local counter; only those counters must be kept
        // real for later instances so forcePerInstance can split hierarchy.
        puts(", first");  // Enable, passed from __Vconfigure parameter
        puts(", ");
        puts(!forceGlobal && coverage_ఉపయోగిస్తుంది_Local_Counter(nodep) ? "true" : "false");
        puts(", ");
        putsQuoted(protect(nodep->fileline()->filename()));
        puts(", ");
        puts(cvt_గమ్యం_Str(nodep->fileline()->lineno()));
        puts(", ");
        puts(cvt_గమ్యం_Str(nodep->స్థానభ్రంశం() + nodep->fileline()->మొదటి_స్తంభం()));
        puts(", ");
        putsQuoted((!nodep->hier().ఖాళీలు() ? "." : "")
                   + VIdProtect::protect_పదాలు_If(nodep->hier(), nodep->protect()));
        puts(", ");
        putsQuoted(VIdProtect::protect_పదాలు_If(nodep->పుట(), nodep->protect()));
        puts(", ");
        putsQuoted(VIdProtect::protect_పదాలు_If(nodep->comment(), nodep->protect()));
        puts(", ");
        putsQuoted(nodep->linescov());
        puts(", ");
        putsQuoted(VIdProtect::protect_పదాలు_If(nodep->fsmVar(), nodep->protect()));
        puts(", ");
        putsQuoted(VIdProtect::protect_పదాలు_If(nodep->fsmFrom(), nodep->protect()));
        puts(", ");
        putsQuoted(VIdProtect::protect_పదాలు_If(nodep->fsm_గమ్యం(), nodep->protect()));
        puts(", ");
        putsQuoted(VIdProtect::protect_పదాలు_If(nodep->fsm_ట్యాగ్(), nodep->protect()));
        puts(");\n");
    }

బహిరంగ:
    // METHODS
    bool display_Emit_శీర్షిక(Ast_కణుపు* nodep);
    శూన్య display_కణుపు(Ast_కణుపు* nodep, AstSFormatF* fmtp, స్థిరరాశి string& vformat, Ast_కణుపు* exprsp,
                     bool is_పరిశీలన);

    bool emitSimpleOk(Ast_కణుపు_సమాసం* nodep);
    శూన్య emitIQW(స్థిరరాశి Ast_కణుపు* nodep) {
        // See "Type letters" in verilated.h
        // Other abbrevs: "C"har, "S"hort, "F"loat, "D"ouble, stri"N"g, "R"=queue, "U"npacked
        puts(nodep->dtypep()->skipRefp()->charIQWN());
    }
    శూన్య emitRU(స్థిరరాశి Ast_కణుపు* nodep) {
        స్థిరరాశి Ast_కణుపు_DType* dtp = nodep->dtypep()->skipRefp();
        // See "Type letters" in verilated.h
        ఒకవేళ (VN_IS(dtp, UnpackArrayDType))
            puts("U");
        లేకపోతే ఒకవేళ (VN_IS(dtp, వరుస_DType) || VN_IS(dtp, DynArrayDType))
            puts("R");
    }
    శూన్య emitScIQW(AstVar* nodep) {
        UASSERT_OBJ(nodep->isSc(), nodep, "emitting SystemC operator on non-SC variable");
        puts(nodep->isScBigUint()                           ? "SB"
             : (nodep->isScUint() || nodep->isScUintBool()) ? "SU"
             : nodep->isScBv()                              ? "SW"
             : nodep->isDouble()                            ? "SD"
                                                            : (nodep->isScQuad() ? "SQ" : "SI"));
    }
    శూన్య emitDatap(స్థిరరాశి Ast_కణుపు* nodep) {
        // When passing to a function with va_args the compiler doesn't
        // know need a pointer so when wide, need to look inside VlWide
        ఒకవేళ (nodep->isWide()) puts(".data()");
    }
    శూన్య emitOpName(Ast_కణుపు* nodep, స్థిరరాశి string& format, Ast_కణుపు* lhsp, Ast_కణుపు* rhsp,
                    Ast_కణుపు* thsp);
    శూన్య emit_CCall_వాదనలు(స్థిరరాశి Ast_కణుపు_CCall* nodep, స్థిరరాశి string& self_సూచిక, bool లోపల_Process);
    శూన్య emitDereference(Ast_కణుపు* nodep, స్థిరరాశి string& సూచిక);
    std::string dereferenceString(స్థిరరాశి std::string& సూచిక) స్థిరరాశి;
    శూన్య emitCvtPackStr(Ast_కణుపు* nodep);
    శూన్య emitConstant(AstConst* nodep);
    శూన్య emitConstantString(స్థిరరాశి AstConst* nodep);
    శూన్య emitVarReset(స్థిరరాశి string& ఉపసర్గ, AstVar* varp, bool constructing);
    string emitVarResetRecurse(స్థిరరాశి AstVar* varp, bool constructing,
                               స్థిరరాశి string& varNameProtected, Ast_కణుపు_DType* dtypep, పూర్ణాంకం లోతు,
                               స్థిరరాశి string& suffix, స్థిరరాశి Ast_కణుపు* valuep);
    శూన్య emitVarResetScopeHash();
    శూన్య emitChangeDet();
    శూన్య put_Comma_Iterate_తదుపరి(Ast_కణుపు* nodep, bool comma = false) {
        కొరకు (Ast_కణుపు* subnodep = nodep; subnodep; subnodep = subnodep->nextp()) {
            ఒకవేళ (comma) puts(", ");
            iterateConst(subnodep);
            comma = true;
        }
    }
    మూస <typename T>
    string optionalProcArg(స్థిరరాశి T* స్థిరరాశి nodep) {
        తిరిగి (nodep && constructorNeedsProcess(nodep)) ? "vlProcess, " : "";
    }
    స్థిరరాశి AstCNew* getSuperNewCallRecursep(Ast_కణుపు* స్థిరరాశి nodep) {
        // Get the super.new call
        ఒకవేళ (!nodep) తిరిగి nullptr;
        ఒకవేళ (స్థిరరాశి AstCNew* స్థిరరాశి cnewp = VN_CAST(nodep, CNew)) తిరిగి cnewp;
        ఒకవేళ (స్థిరరాశి Ast_Stmt_సమాసం* స్థిరరాశి stmtp = VN_CAST(nodep, Stmt_సమాసం)) {
            ఒకవేళ (స్థిరరాశి AstCNew* స్థిరరాశి cnewp = VN_CAST(stmtp->exprp(), CNew)) తిరిగి cnewp;
        }
        ఒకవేళ (స్థిరరాశి Ast_Jump_ఖండం* స్థిరరాశి blockp = VN_CAST(nodep, Jump_ఖండం)) {
            ఒకవేళ (స్థిరరాశి AstCNew* స్థిరరాశి cnewp = getSuperNewCallRecursep(blockp->stmtsp())) {
                తిరిగి cnewp;
            }
        }
        ఒకవేళ (స్థిరరాశి AstCNew* స్థిరరాశి cnewp = getSuperNewCallRecursep(nodep->nextp())) తిరిగి cnewp;
        తిరిగి nullptr;
    }

    శూన్య emitConstantW(AstConst* nodep, AstVarRef* assigntop) {
        // For tradition and compilation speed, assign each word directly into
        // output variable instead of using '='
        putns(nodep, "");
        ఒకవేళ (nodep->అంకె().isFourState()) {
            nodep->v3warn(E_UNSUPPORTED, "Unsupported: 4-state numbers in this context");
            తిరిగి;
        }

        పూర్ణాంకం పైకి_వెడల్పు = nodep->అంకె().వెడల్పు_గమ్యం_Fit();
        పూర్ణాంకం chunks = 0;
        ఒకవేళ (పైకి_వెడల్పు > EMITC_అంకె_CONSTW * VL_EDATASIZE) {
            // Output e.g. 8 words in groups of e.g. 8
            chunks = (పైకి_వెడల్పు - 1) / (EMITC_అంకె_CONSTW * VL_EDATASIZE);
            పైకి_వెడల్పు %= (EMITC_అంకె_CONSTW * VL_EDATASIZE);
            ఒకవేళ (పైకి_వెడల్పు == 0) పైకి_వెడల్పు = (EMITC_అంకె_CONSTW * VL_EDATASIZE);
        }
        {  // Upper e.g. 8 words
            ఒకవేళ (chunks) {
                putnbs(nodep, "VL_CONSTHI_W_");
                puts(cvt_గమ్యం_Str(VL_పదాలు_I(పైకి_వెడల్పు)));
                puts("X(");
                puts(cvt_గమ్యం_Str(nodep->వెడల్పు_Min()));
                puts(",");
                puts(cvt_గమ్యం_Str(chunks * EMITC_అంకె_CONSTW * VL_EDATASIZE));
            } లేకపోతే {
                putnbs(nodep, "VL_CONST_W_");
                puts(cvt_గమ్యం_Str(VL_పదాలు_I(పైకి_వెడల్పు)));
                puts("X(");
                puts(cvt_గమ్యం_Str(nodep->వెడల్పు_Min()));
            }
            puts(",");
            ఒకవేళ (!assigntop->self_సూచిక().is_ఖాళీలు()) {
                emitDereference(assigntop, assigntop->self_సూచిక_Protect(m_useSelfForThis));
            }
            puts(varName(assigntop->varp()));
            కొరకు (పూర్ణాంకం మాట = VL_పదాలు_I(పైకి_వెడల్పు) - 1; మాట >= 0; మాట--) {
                // Only 32 bits - llx + long long here just to appease CPP format warning
                ofp()->రాయి(",0x%08" PRIx64, static_cast<uint64_t>(nodep->అంకె().edata_మాట(
                                                   మాట + chunks * EMITC_అంకె_CONSTW)));
            }
            puts(")");
        }
        కొరకు (chunks--; chunks >= 0; chunks--) {
            puts(";\n");
            putbs("VL_CONSTLO_W_");
            puts(cvt_గమ్యం_Str(EMITC_అంకె_CONSTW));
            puts("X(");
            puts(cvt_గమ్యం_Str(chunks * EMITC_అంకె_CONSTW * VL_EDATASIZE));
            puts(",");
            ఒకవేళ (!assigntop->self_సూచిక().is_ఖాళీలు()) {
                emitDereference(assigntop, assigntop->self_సూచిక_Protect(m_useSelfForThis));
            }
            puts(varName(assigntop->varp()));
            కొరకు (పూర్ణాంకం మాట = EMITC_అంకె_CONSTW - 1; మాట >= 0; మాట--) {
                // Only 32 bits - llx + long long here just to appease CPP format warning
                ofp()->రాయి(",0x%08" PRIx64, static_cast<uint64_t>(nodep->అంకె().edata_మాట(
                                                   మాట + chunks * EMITC_అంకె_CONSTW)));
            }
            puts(")");
        }
    }

    శూన్య emit_Nodes_With_పాఠ్యం(Ast_కణుపు* nodesp, bool useSelfForThis, bool tracking,
                           స్థిరరాశి std::string& separator) {
        కొరకు (Ast_కణుపు* nodep = nodesp; nodep; nodep = nodep->nextp()) {
            ఒకవేళ (స్థిరరాశి Ast_పాఠ్యం* స్థిరరాశి textp = VN_CAST(nodep, పాఠ్యం_2)) {
                స్థిరరాశి std::string పాఠ్యం
                    = VSelf_సూచిక_పాఠ్యం::replaceThis(useSelfForThis, textp->పాఠ్యం());
                ఒకవేళ (tracking) {
                    puts(పాఠ్యం);
                } లేకపోతే {
                    ofp()->putsNoTracking(పాఠ్యం);
                }
            } లేకపోతే {
                iterateConst(nodep);
            }
            ఒకవేళ (nodep->nextp()) puts(separator);
        }
    }

    శూన్య putConstructorSubinit(స్థిరరాశి AstClass* classp, AstCFunc* cfuncp) {
        // Direct non-virtual bases in declaration order
        కొరకు (స్థిరరాశి AstClassExtends* extp = classp->extendsp(); extp;
             extp = VN_AS(extp->nextp(), ClassExtends)) {
            ఒకవేళ (extp->classp()->isInterfaceClass()) కొనసాగు;
            ఒకవేళ (extp->classp()->use_Virtual_ప్రజా()) కొనసాగు;
            puts(EmitCUtil::ఉపసర్గ_Name_Protect(extp->classp()));
            puts("::init");
            ఒకవేళ (constructorNeedsProcess(extp->classp())) {
                puts("(vlProcess, vlSymsp");
            } లేకపోతే {
                puts("(vlSymsp");
            }
            // Handle super.new() args for the concrete parent
            ఒకవేళ (స్థిరరాశి AstCNew* స్థిరరాశి superNewCallp = getSuperNewCallRecursep(cfuncp->stmtsp())) {
                put_Comma_Iterate_తదుపరి(superNewCallp->argsp(), true);
            }
            puts(");\n");
            // Since SystemVerilog allows only for extending from one class, for sure there won't
            // be any more constructors to be called. Even if there are some classes which are
            // "implemented", those are interface classes which do not have constructors.
            ఆపు;
        }
    }

    // VISITORS
    using EmitCConstInit::సందర్శించు;
    శూన్య సందర్శించు(AstCFunc* స్థిరరాశి nodep) override {
        ఒకవేళ (nodep->ఖాళీలు_Body() && !nodep->isLoose()) తిరిగి;
        VL_RESTORER(m_useSelfForThis);
        VL_RESTORER(m_cfuncp);
        VL_RESTORER(m_instantiatesOwnProcess);
        VL_RESTORER(m_createdScopeHash);
        m_cfuncp = nodep;
        m_instantiatesOwnProcess = false;
        m_labelNumbers.clear();  // No need to save/restore, all Jumps must be within the function

        విభజించు_కొలత_Inc(nodep);

        puts("\n");
        m_lazyDecls.emit(nodep);
        ఒకవేళ (nodep->ifdef() != "") putns(nodep, "#ifdef " + nodep->ifdef() + "\n");
        emit_CFunc_శీర్షిక(nodep, m_modp, /* withScope: */ true);

        puts(" {\n");
        ఒకవేళ (nodep->isConstructor()) {
            స్థిరరాశి AstClass* స్థిరరాశి classp = VN_CAST(nodep->scopep()->modp(), Class);
            ఒకవేళ (classp && classp->extendsp()) putConstructorSubinit(classp, nodep);
        }

        // "+" in the debug indicates a print from the model
        puts("VL_DEBUG_IF(VL_DBG_MSGF(\"+  ");
        కొరకు (పూర్ణాంకం i = 0; i < m_modp->level(); ++i) puts("  ");
        puts(EmitCUtil::ఉపసర్గ_Name_Protect(m_modp));
        puts(nodep->isLoose() ? "__" : "::");
        puts(nodep->nameProtect() + "\\n\"); );\n");

        ఒకవేళ (nodep->isLoose()) {
            m_lazyDecls.declared(nodep);  // Defined here, so no longer needs declaration
            ఒకవేళ (!nodep->isStatic()) {  // Standard prologue
                m_useSelfForThis = true;
                ఒకవేళ (nodep->voidSelf()) {
                    స్థిరరాశి string className = EmitCUtil::ఉపసర్గ_Name_Protect(m_modp);
                    puts(className + "* const vlSelf = static_cast<" + className
                         + "*>(vlVoidSelf);\n");
                }
                ఒకవేళ (!VN_IS(m_modp, Class)) {
                    puts(EmitCUtil::sym_Class_కేటాయించు());  // Uses vlSelf
                } లేకపోతే {
                    puts("(void)vlSelf;  // Prevent unused variable warning\n");
                }
            }
        }

        // Instantiate a process class if it's going to be needed somewhere later
        nodep->forall([&](స్థిరరాశి Ast_కణుపు_CCall* ccallp) -> bool {
            ఒకవేళ (ccallp->funcp()->needProcess()
                && (ccallp->funcp()->isCoroutine() == VN_IS(ccallp->backp(), CAwait))) {
                ఒకవేళ (!nodep->needProcess() && !m_instantiatesOwnProcess) {
                    m_instantiatesOwnProcess = true;
                    తిరిగి false;
                }
            }
            తిరిగి true;
        });
        ఒకవేళ (m_instantiatesOwnProcess) {
            AstCStmt* స్థిరరాశి vlprocp = new AstCStmt{nodep->fileline()};
            vlprocp->కూడు("VlProcessRef vlProcess = std::make_shared<VlProcess>();\n");
            vlprocp->కూడు("VlProcess::currentp(vlProcess.get());");
            nodep->prependStmtsp(vlprocp);
        } లేకపోతే ఒకవేళ (nodep->needProcess() && nodep->stmtsp()) {
            // Set current process so VlRNG() constructors in this function seed from it
            AstCStmt* స్థిరరాశి సమితి_Processp = new AstCStmt{nodep->fileline()};
            సమితి_Processp->కూడు("VlProcess::currentp(vlProcess.get());");
            nodep->prependStmtsp(సమితి_Processp);
        }

        కొరకు (Ast_కణుపు* subnodep = nodep->argsp(); subnodep; subnodep = subnodep->nextp()) {
            ఒకవేళ (AstVar* స్థిరరాశి varp = VN_CAST(subnodep, Var)) {
                ఒకవేళ (varp->isFuncReturn()) emitVarDecl(varp);
            }
        }

        ఒకవేళ (m_useSelfForThis) {
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

        ఒకవేళ (nodep->varsp()) {
            putsDecoration(nodep, "// Locals\n");
            iterate_And_తదుపరి_Const_Null(nodep->varsp());
        }

        ఒకవేళ (nodep->stmtsp()) {
            putsDecoration(nodep, "// Body\n");
            iterate_And_తదుపరి_Const_Null(nodep->stmtsp());
        }

        m_usevlSelfRef = false;

        ఒకవేళ (nodep->isCoroutine()) {
            // Sometimes coroutines don't have co_awaits,
            // so emit a co_return at the end to avoid compile errors.
            puts("co_return;\n");
        }

        puts("}\n");
        ఒకవేళ (nodep->ifdef() != "") puts("#endif  // " + nodep->ifdef() + "\n");
    }

    శూన్య సందర్శించు(AstVar* nodep) override {
        UASSERT_OBJ(m_cfuncp, nodep, "Cannot emit non-local variable");
        emitVarDecl(nodep);
    }

    శూన్య సందర్శించు(Ast_Cvt_Array_గమ్యం_Array* nodep) override {
        ఒకవేళ (nodep->తిరగేయి()) {
            puts("VL_REVCLONE_Q(");
        } లేకపోతే {
            puts("VL_CLONE_Q(");
        }
        iterate_And_తదుపరి_Const_Null(nodep->fromp());
        puts(", ");
        puts(cvt_గమ్యం_Str(nodep->ఖండం_కొలత()));
        puts(", ");
        puts(cvt_గమ్యం_Str(nodep->src_Element_బిట్లు()));
        puts(", ");
        puts(cvt_గమ్యం_Str(nodep->dst_Element_బిట్లు()));
        puts(")");
    }

    శూన్య సందర్శించు(Ast_Cvt_Array_గమ్యం_Packed* nodep) override {
        Ast_కణుపు_DType* స్థిరరాశి fromDtp = nodep->fromp()->dtypep()->skipRefp();
        Ast_కణుపు_DType* స్థిరరాశి elemDtp = fromDtp->subDTypep()->skipRefp();
        స్థిరరాశి bool descending = [&]() {
            ఒకవేళ (స్థిరరాశి AstUnpackArrayDType* స్థిరరాశి unpackDtp = VN_CAST(fromDtp, UnpackArrayDType))
                తిరిగి !unpackDtp->declRange().ascending();
            తిరిగి false;
        }();
        puts("VL_PACK_");
        emitIQW(nodep);
        puts("_");
        emitRU(fromDtp);
        emitIQW(elemDtp);
        ఒకవేళ (descending) {
            // Wrap source in VL_PACK_REVERSED so VL_PACK sees ascending order
            emitOpName(nodep, "(%nw, %rw, %P, VL_PACK_REVERSED(%li))", nodep->fromp(), elemDtp,
                       nullptr);
        } లేకపోతే {
            emitOpName(nodep, "(%nw, %rw, %P, %li)", nodep->fromp(), elemDtp, nullptr);
        }
    }

    శూన్య సందర్శించు(Ast_Cvt_Unpack_గమ్యం_వరుస* nodep) override {
        Ast_కణుపు_DType* స్థిరరాశి elemDTypep = nodep->fromp()->dtypep()->subDTypep();
        emitOpName(nodep, nodep->emitC(), nodep->fromp(), elemDTypep, nullptr);
    }
    శూన్య సందర్శించు(Ast_Cvt_Unpack_గమ్యం_Unpack* nodep) override {
        // Whether each dimension is reversed, the first for the outermost, up to the last that is
        std::సదిశ<bool> reverses;
        స్థిరరాశి AstUnpackArrayDType* శిఖరం = VN_AS(nodep->dtypep()->skipRefp(), UnpackArrayDType);
        స్థిరరాశి AstUnpackArrayDType* fromp
            = VN_AS(nodep->fromp()->dtypep()->skipRefp(), UnpackArrayDType);
        అయితే (శిఖరం) {
            reverses.నెట్టు_వెనుక(శిఖరం->declRange().ascending() != fromp->declRange().ascending());
            శిఖరం = VN_CAST(శిఖరం->subDTypep()->skipRefp(), UnpackArrayDType);
            fromp = VN_CAST(fromp->subDTypep()->skipRefp(), UnpackArrayDType);
        }
        అయితే (!reverses.వెనుక()) reverses.తీయి_వెనుక();
        putns(nodep, "VL_CVT_UNPACK_TO_UNPACK<");
        కొరకు (కొలత_t i = 0; i < reverses.కొలత(); ++i) {
            ఒకవేళ (i) puts(", ");
            puts(reverses[i] ? "1" : "0");
        }
        puts(">(");
        iterate_And_తదుపరి_Const_Null(nodep->fromp());
        puts(")");
    }

    శూన్య సందర్శించు(Ast_కణుపు_కేటాయించు* nodep) override {
        ఒకవేళ (AstCReset* స్థిరరాశి resetp = VN_CAST(nodep->rhsp(), CReset)) {
            // TODO get rid of emitVarReset and instead let AstNodeAssign understand how to init
            // anything
            Ast_కణుపు* fromp = nodep->lhsp();
            // Fork needs to use a member select.  Nothing else should be possible before VarRef.
            ఒకవేళ (AstMemberSel* స్థిరరాశి sfromp = VN_CAST(fromp, MemberSel)) {
                // Fork-DynScope generated pointer to previously automatic variable
                AstVar* స్థిరరాశి memberVarp = sfromp->varp();
                fromp = sfromp->fromp();
                ఒకవేళ (AstNullCheck* స్థిరరాశి sfromp = VN_CAST(fromp, NullCheck)) fromp = sfromp->lhsp();
                Ast_కణుపు_Var_Ref* స్థిరరాశి fromVarRefp = VN_AS(fromp, కణుపు_Var_Ref);
                emitVarReset(
                    ("VL_NULL_CHECK("s
                     + (fromVarRefp->self_సూచిక().is_ఖాళీలు()
                            ? ""
                            : dereferenceString(fromVarRefp->self_సూచిక_Protect(m_useSelfForThis)))
                     + varName(fromVarRefp->varp()) + ", \""
                     + V3_బహిర్గతం_Formatter::quoteNameControls(protect(nodep->fileline()->filename()))
                     + "\", " + std::గమ్యం_string(nodep->fileline()->lineno()) + ")->"),
                    memberVarp, resetp->constructing());
            } లేకపోతే {
                Ast_కణుపు_Var_Ref* స్థిరరాశి fromVarRefp = VN_AS(fromp, కణుపు_Var_Ref);
                AstVar* స్థిరరాశి varp = fromVarRefp->varp();
                స్థిరరాశి string ఉపసర్గ
                    = fromVarRefp->self_సూచిక().is_ఖాళీలు()
                          ? ""
                          : dereferenceString(
                                VN_AS(fromp, కణుపు_Var_Ref)->self_సూచిక_Protect(m_useSelfForThis));
                emitVarReset(ఉపసర్గ, varp, resetp->constructing());
            }
            తిరిగి;
        }
        bool paren = true;
        bool decind = false;
        bool rhs = true;
        bool తిరగేయి_Unpack = false;  // Set for descending CvtPackedToArray
        స్థిరరాశి AstUnpackArrayDType* స్థిరరాశి unpackDtp
            = VN_CAST(nodep->dtypep()->skipRefp(), UnpackArrayDType);
        ఒకవేళ (AstSel* స్థిరరాశి selp = VN_CAST(nodep->lhsp(), Sel)) {
            UASSERT_OBJ(selp->వెడల్పు_Min() == selp->వెడల్పు_Const(), selp, "Width mismatch");
            ఒకవేళ (selp->వెడల్పు_Min() == 1) {
                putnbs(nodep, "VL_ASSIGNBIT_");
                emitIQW(selp->fromp());
                ఒకవేళ (nodep->rhsp()->isAllOnesV()) {
                    puts("O(");
                    rhs = false;
                } లేకపోతే {
                    puts("I(");
                }
                iterate_And_తదుపరి_Const_Null(selp->lsbp());
                puts(", ");
                iterate_And_తదుపరి_Const_Null(selp->fromp());
                ఒకవేళ (rhs) puts(", ");
            } లేకపోతే {
                putnbs(nodep, "VL_ASSIGNSEL_");
                emitIQW(selp->fromp());
                emitIQW(nodep->rhsp());
                puts("(");
                putns(selp->fromp(), cvt_గమ్యం_Str(selp->fromp()->వెడల్పు_Min()) + ", ");
                puts(cvt_గమ్యం_Str(nodep->వెడల్పు_Min()) + ", ");
                iterate_And_తదుపరి_Const_Null(selp->lsbp());
                puts(", ");
                iterate_And_తదుపరి_Const_Null(selp->fromp());
                puts(", ");
            }
        } లేకపోతే ఒకవేళ (స్థిరరాశి AstGetcRefN* స్థిరరాశి selp = VN_CAST(nodep->lhsp(), GetcRefN)) {
            iterate_And_తదుపరి_Const_Null(selp->lhsp());
            puts(" = ");
            putnbs(selp, "VL_PUTC_N(");
            iterate_And_తదుపరి_Const_Null(selp->lhsp());
            puts(", ");
            iterate_And_తదుపరి_Const_Null(selp->rhsp());
            puts(", ");
        } లేకపోతే ఒకవేళ (AstVar* స్థిరరాశి varp = AstVar::scVarRecurse(nodep->lhsp())) {
            putnbs(varp, "VL_ASSIGN_");  // Set a systemC variable
            emitScIQW(varp);
            emitIQW(nodep);
            puts("(");
            puts(cvt_గమ్యం_Str(nodep->వెడల్పు_Min()) + ", ");
            iterate_And_తదుపరి_Const_Null(nodep->lhsp());
            puts(", ");
        } లేకపోతే ఒకవేళ (AstVar* స్థిరరాశి varp = AstVar::scVarRecurse(nodep->rhsp())) {
            putnbs(varp, "VL_ASSIGN_");  // Get a systemC variable
            emitIQW(nodep);
            emitScIQW(varp);
            puts("(");
            puts(cvt_గమ్యం_Str(nodep->వెడల్పు_Min()) + ", ");
            iterate_And_తదుపరి_Const_Null(nodep->lhsp());
            puts(", ");
        } లేకపోతే ఒకవేళ (స్థిరరాశి Ast_Cvt_Packed_గమ్యం_Array* స్థిరరాశి castp
                   = VN_CAST(nodep->rhsp(), Cvt_Packed_గమ్యం_Array)) {
            putns(castp, "VL_UNPACK_");
            emitRU(nodep);
            emitIQW(nodep->dtypep()->subDTypep());
            puts("_");
            emitRU(castp->fromp());
            emitIQW(castp->fromp());
            puts("(");
            putns(castp->dtypep(), cvt_గమ్యం_Str(castp->dtypep()->subDTypep()->వెడల్పు_Min()));
            puts(", ");
            puts(cvt_గమ్యం_Str(castp->fromp()->వెడల్పు_Min()));
            puts(", ");
            iterate_And_తదుపరి_Const_Null(nodep->lhsp());
            puts(", ");
            rhs = false;
            iterate_And_తదుపరి_Const_Null(castp->fromp());
            // Descending unpacked dest: reverse after unpack
            ఒకవేళ (unpackDtp && !unpackDtp->declRange().ascending()) తిరగేయి_Unpack = true;
        } లేకపోతే ఒకవేళ (స్థిరరాశి Ast_Cvt_Array_గమ్యం_Array* స్థిరరాశి castp
                   = VN_CAST(nodep->rhsp(), Cvt_Array_గమ్యం_Array)) {
            ఒకవేళ (castp->తిరగేయి()) {
                putns(castp, "VL_REVCOPY_Q(");
            } లేకపోతే {
                putns(castp, "VL_COPY_Q(");
            }
            iterate_And_తదుపరి_Const_Null(nodep->lhsp());
            puts(", ");
            rhs = false;
            iterate_And_తదుపరి_Const_Null(castp->fromp());
            puts(", ");
            puts(cvt_గమ్యం_Str(castp->ఖండం_కొలత()));
            puts(", ");
            puts(cvt_గమ్యం_Str(castp->src_Element_బిట్లు()));
            puts(", ");
            puts(cvt_గమ్యం_Str(castp->dst_Element_బిట్లు()));
        } లేకపోతే ఒకవేళ (nodep->isWide() && VN_IS(nodep->lhsp(), VarRef)  //
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
                   && !VN_IS(nodep->rhsp(), సమాసం_Stmt)) {
            // Wide functions assign into the array directly, don't need separate assign statement
            m_wide_తాత్కాలిక_Refp = VN_AS(nodep->lhsp(), VarRef);
            paren = false;
        } లేకపోతే ఒకవేళ (nodep->isWide() && !unpackDtp && !VN_IS(nodep->rhsp(), Const)) {
            putnbs(nodep, "VL_ASSIGN_W(");
            puts(cvt_గమ్యం_Str(nodep->వెడల్పు_Min()) + ", ");
            iterate_And_తదుపరి_Const_Null(nodep->lhsp());
            puts(", ");
        } లేకపోతే ఒకవేళ (VN_IS(nodep->lhsp()->dtypep()->skipRefp(), వరుస_DType)
                   && (VN_IS(nodep->rhsp(), ప్రవాహం_L) || VN_IS(nodep->lhsp(), ప్రవాహం_L)
                       || VN_IS(nodep->rhsp(), ప్రవాహం_R) || VN_IS(nodep->lhsp(), ప్రవాహం_R)
                       || VN_IS(nodep->rhsp(), ప్రవాహం_R))) {
            //if either side is streamL or streamR don't emit lhsp everything will be passed by
            //reference

            paren = false;

        } లేకపోతే {
            paren = false;
            iterate_And_తదుపరి_Const_Null(nodep->lhsp());
            puts(" ");
            ofp()->ఖండం_Inc();
            decind = true;
            ఒకవేళ (!VN_IS(nodep->rhsp(), Const)) ofp()->putBreak();
            putns(nodep, "= ");
        }
        ఒకవేళ (rhs) iterate_And_తదుపరి_Const_Null(nodep->rhsp());
        ఒకవేళ (paren) puts(")");
        ఒకవేళ (decind) ofp()->ఖండం_Dec();
        puts(";\n");
        ఒకవేళ (తిరగేయి_Unpack) {
            puts("VL_UNPACK_REVERSED(");
            iterate_And_తదుపరి_Const_Null(nodep->lhsp());
            puts(");\n");
        }
    }
    శూన్య సందర్శించు(AstAssocSel* nodep) override {
        iterate_And_తదుపరి_Const_Null(nodep->fromp());
        స్థిరరాశి std::string atFunc = nodep->isLValue() ? ".atWrite(" : ".at(";
        putnbs(nodep, atFunc);
        AstAssocArrayDType* స్థిరరాశి adtypep
            = VN_AS(nodep->fromp()->dtypep()->skipRefp(), AssocArrayDType);
        UASSERT_OBJ(adtypep, nodep, "Associative select on non-associative type");
        iterate_And_తదుపరి_Const_Null(nodep->bitp());
        puts(")");
    }
    శూన్య సందర్శించు(AstWildcardSel* nodep) override {
        iterate_And_తదుపరి_Const_Null(nodep->fromp());
        స్థిరరాశి std::string atFunc = nodep->isLValue() ? ".atWrite(" : ".at(";
        putnbs(nodep, atFunc);
        AstWildcardArrayDType* స్థిరరాశి adtypep
            = VN_AS(nodep->fromp()->dtypep()->skipRefp(), WildcardArrayDType);
        UASSERT_OBJ(adtypep, nodep, "Wildcard select on non-wildcard-associative type");
        iterate_And_తదుపరి_Const_Null(nodep->bitp());
        puts(")");
    }
    శూన్య సందర్శించు(AstCCall* nodep) override {
        స్థిరరాశి AstCFunc* స్థిరరాశి funcp = nodep->funcp();
        స్థిరరాశి Ast_కణుపు_Module* స్థిరరాశి funcModp = EmitCParentModule::get(funcp);
        putnbs(nodep, "");
        ఒకవేళ (funcp->dpiImportPrototype()) {
            // Calling DPI import
            putns(nodep, funcp->name());
        } లేకపోతే ఒకవేళ (funcp->is_Proper_పద్ధతి() && funcp->isStatic()) {
            // Call static method via the containing class
            putns(funcModp, EmitCUtil::ఉపసర్గ_Name_Protect(funcModp) + "::");
            putns(nodep, funcp->nameProtect());
        } లేకపోతే ఒకవేళ (nodep->superReference()) {
            // Calling superclass method
            putns(funcModp, EmitCUtil::ఉపసర్గ_Name_Protect(funcModp) + "::");
            putns(nodep, funcp->nameProtect());
        } లేకపోతే ఒకవేళ (funcp->isLoose()) {
            // Calling loose method
            putns(nodep, funcNameProtect(funcp));
        } లేకపోతే {
            // Calling regular method/function
            ఒకవేళ (!nodep->self_సూచిక().is_ఖాళీలు()) {
                emitDereference(nodep, nodep->self_సూచిక_Protect(m_useSelfForThis));
            }
            putns(nodep, funcp->nameProtect());
        }
        emit_CCall_వాదనలు(nodep, nodep->self_సూచిక_Protect(m_useSelfForThis), m_cfuncp->needProcess());
    }
    శూన్య సందర్శించు(AstCMethodCall* nodep) override {
        స్థిరరాశి AstCFunc* స్థిరరాశి funcp = nodep->funcp();
        UASSERT_OBJ(!funcp->isLoose(), nodep, "Loose method called via AstCMethodCall");
        iterateConst(nodep->fromp());
        putnbs(nodep, "->");
        ఒకవేళ (nodep->superReference()) {
            // Super call, so the base implementation is used rather than the override
            స్థిరరాశి Ast_కణుపు_Module* స్థిరరాశి funcModp = EmitCParentModule::get(funcp);
            putns(funcModp, EmitCUtil::ఉపసర్గ_Name_Protect(funcModp) + "::");
        }
        putns(funcp, funcp->nameProtect());
        emit_CCall_వాదనలు(nodep, "", m_cfuncp->needProcess());
    }
    శూన్య సందర్శించు(AstCAwait* nodep) override {
        putns(nodep, "co_await ");
        iterateConst(nodep->exprp());
        puts(";\n");
    }
    శూన్య సందర్శించు(AstCNew* nodep) override {
        ఒకవేళ (VN_IS(nodep->dtypep(), VoidDType)) {  // super.new case
            putsDecoration(nodep, "/*super.new*/");
            తిరిగి;
        }
        // assignment case;
        putns(nodep, "VL_NEW(" + EmitCUtil::ఉపసర్గ_Name_Protect(nodep->dtypep()) + ", "
                         + optionalProcArg(nodep->dtypep()) + "vlSymsp");
        put_Comma_Iterate_తదుపరి(nodep->argsp(), true);
        puts(")");
    }
    శూన్య సందర్శించు(AstCMethodHard* nodep) override {
        iterateConst(nodep->fromp());
        putns(nodep, nodep->usePtr() ? "->" : ".");
        putns(nodep, nodep->name());
        ఒకవేళ (nodep->పద్ధతి() == VCMethod::FORCE_READ_SEL) {
            emitIQW(nodep);
            ఒకవేళ (nodep->isWide()) puts("<" + cvt_గమ్యం_Str(nodep->dtypep()->వెడల్పు_పదాలు()) + ">");
        } లేకపోతే ఒకవేళ (nodep->పద్ధతి() == VCMethod::ARRAY_ముక్క) {
            // VlUnpacked::slice<N_Out>(loIdx) - N_Out is the (fixed) result array size
            స్థిరరాశి AstUnpackArrayDType* స్థిరరాశి adtypep
                = VN_AS(nodep->dtypep()->skipRefp(), UnpackArrayDType);
            puts("<" + cvt_గమ్యం_Str(adtypep->elementsConst()) + ">");
        } లేకపోతే ఒకవేళ (nodep->పద్ధతి() == VCMethod::COVERGROUP_కూడు_COVERPOINT) {
            // The hit-list bound is a template argument of the returned VlCoverpointT<>, and the
            // node's own dtype is that type, so it is the one source of truth for both.
            స్థిరరాశి AstCoverpointDType* స్థిరరాశి cpdtypep
                = VN_AS(nodep->dtypep()->skipRefp(), CoverpointDType);
            puts("<" + cvt_గమ్యం_Str(cpdtypep->తగిలింది_Bound()) + ">");
        } లేకపోతే ఒకవేళ (nodep->పద్ధతి() == VCMethod::COVERGROUP_కూడు_CROSS) {
            స్థిరరాశి AstCoverCrossDType* స్థిరరాశి cxdtypep
                = VN_AS(nodep->dtypep()->skipRefp(), CoverCrossDType);
            puts("<" + cxdtypep->cpp_Template_వాదనలు() + ">");
        }
        puts("(");
        bool comma = false;
        పూర్ణాంకం arg_అంకె = 0;
        కొరకు (Ast_కణుపు* subnodep = nodep->pinsp(); subnodep; subnodep = subnodep->nextp()) {
            ఒకవేళ (comma) puts(", ");
            ఒకవేళ (nodep->పద్ధతి() == VCMethod::RANDOMIZER_HARD && arg_అంకె == 1) {
                // For RANDOMIZER_HARD's filename argument (2nd arg after constraint),
                // apply protect() similar to VL_STOP to handle --protected flag
                ఒకవేళ (స్థిరరాశి AstCExpr* స్థిరరాశి cexprp = VN_CAST(subnodep, CExpr)) {
                    // Extract filename from the CExpr (which contains "filename")
                    std::string filename;
                    కొరకు (స్థిరరాశి Ast_కణుపు* textnodep = cexprp->nodesp(); textnodep;
                         textnodep = textnodep->nextp()) {
                        ఒకవేళ (స్థిరరాశి Ast_పాఠ్యం* స్థిరరాశి textp = VN_CAST(textnodep, పాఠ్యం_2)) {
                            filename = textp->పాఠ్యం();
                            ఆపు;
                        }
                    }
                    // Remove surrounding quotes if present
                    ఒకవేళ (filename.కొలత() >= 2 && filename.front() == '"'
                        && filename.వెనుక() == '"') {
                        filename = filename.substr(1, filename.కొలత() - 2);
                    }
                    // Emit with protect()
                    putsQuoted(protect(filename));
                } లేకపోతే {
                    iterateConst(subnodep);
                }
            } లేకపోతే {
                iterateConst(subnodep);
            }
            comma = true;
            arg_అంకె++;
        }
        ఒకవేళ (nodep->withp()) {
            ఒకవేళ (comma) puts(", ");
            iterateConst(nodep->withp());
        }
        puts(")");
    }
    శూన్య సందర్శించు(AstCFuncHard* nodep) override {
        putns(nodep, nodep->name());
        puts("(");
        put_Comma_Iterate_తదుపరి(nodep->pinsp());
        puts(")");
    }
    శూన్య సందర్శించు(AstLambdaArgRef* nodep) override { putbs(nodep->nameProtect()); }
    శూన్య సందర్శించు(AstWith* nodep) override {
        // With uses a C++11 lambda
        putnbs(nodep, "[&](");
        ఒకవేళ (స్వయంచాలక* స్థిరరాశి argrefp = nodep->సూచీ_Arg_Refp()) {
            putnbs(argrefp, argrefp->dtypep()->cType(argrefp->nameProtect(), false, false));
            puts(", ");
        }
        ఒకవేళ (స్వయంచాలక* స్థిరరాశి argrefp = nodep->విలువ_Arg_Refp()) {
            putnbs(argrefp, argrefp->dtypep()->cType(argrefp->nameProtect(), false, false));
        }
        puts(") -> ");
        putnbs(nodep, nodep->dtypep()->cType("", false, false));
        puts(" {\n");
        VL_RESTORER(m_createdScopeHash);
        iterate_And_తదుపరి_Const_Null(nodep->exprp());
        puts("}\n");
    }
    శూన్య సందర్శించు(AstCase* nodep) override {  // LCOV_EXCL_LINE
        // In V3Case...
        nodep->v3fatalSrc("Case statements should have been reduced out");
    }
    శూన్య సందర్శించు(AstComment* nodep) override {
        string at;
        ఒకవేళ (nodep->showAt()) {
            at = " at " + nodep->fileline()->ascii();
            // If protecting, passthru less information about the design
            ఒకవేళ (!v3Global.opt.protectIds()) తిరిగి;
        }
        ఒకవేళ (!(nodep->protect() && v3Global.opt.protectIds())) {
            putsDecoration(nodep, "// "s + nodep->name() + at + "\n");
        }
        iterateChildrenConst(nodep);
    }
    శూన్య సందర్శించు(AstCoverOtherDecl* nodep) override {
        emit_Cover_Other_Decl_చొప్పించు(nodep);
        ఒకవేళ (coverage_ఉపయోగిస్తుంది_Local_Counter(nodep)) {
            // Some task bodies are cloned into a caller module where the
            // original instance-local counter is not addressable. Those clones
            // fall back to the global bin number, so register that slot too.
            emit_Cover_Other_Decl_చొప్పించు(nodep, true);
        }
    }
    శూన్య సందర్శించు(AstCoverToggleDecl* nodep) override {
        putns(nodep, "vlSelf->__vlCoverToggleInsert(");  // As Declared in emitCoverageDecl
        puts(cvt_గమ్యం_Str(nodep->range().కుడి_2()));
        puts(", ");
        puts(cvt_గమ్యం_Str(nodep->range().ఎడమ_2()));
        puts(", ");
        puts(cvt_గమ్యం_Str(nodep->range().ranged()));
        puts(", ");
        putCoverageArray(nodep);
        puts(" + ");
        puts(cvt_గమ్యం_Str(coverage_Bin_అంకె(nodep)));
        // first controls whether duplicate module instances are collapsed in
        // the default coverage view. The following flag says whether countp
        // points to an object-local counter; only those counters must be kept
        // real for later instances so forcePerInstance can split hierarchy.
        puts(", first");  // Enable, passed from __Vconfigure parameter
        puts(", ");
        puts(coverage_ఉపయోగిస్తుంది_Local_Counter(nodep) ? "true" : "false");
        puts(", ");
        putsQuoted(protect(nodep->fileline()->filename()));
        puts(", ");
        puts(cvt_గమ్యం_Str(nodep->fileline()->lineno()));
        puts(", ");
        puts(cvt_గమ్యం_Str(nodep->fileline()->మొదటి_స్తంభం()));
        puts(", ");
        putsQuoted((!nodep->hier().ఖాళీలు() ? "." : "")
                   + VIdProtect::protect_పదాలు_If(nodep->hier(), nodep->protect()));
        puts(", ");
        putsQuoted(VIdProtect::protect_పదాలు_If(nodep->పుట(), nodep->protect()));
        puts(", ");
        putsQuoted(VIdProtect::protect_పదాలు_If(nodep->comment(), nodep->protect()));
        puts(");\n");
    }
    శూన్య సందర్శించు(AstCoverInc* nodep) override {
        ఒకవేళ (VN_IS(nodep->declp(), CoverOtherDecl)) {
            ఒకవేళ (v3Global.opt.దారాలు() > 1) {
                putns(nodep, "");
                putCoverageArray(nodep->declp());
                puts("[");
                puts(cvt_గమ్యం_Str(coverage_Bin_అంకె(nodep->declp())));
                puts("].fetch_add(1, std::memory_order_relaxed);\n");
            } లేకపోతే {
                putns(nodep, "++(");
                putCoverageArray(nodep->declp());
                puts("[");
                puts(cvt_గమ్యం_Str(coverage_Bin_అంకె(nodep->declp())));
                puts("]);\n");
            }
        } లేకపోతే {
            puts("VL_COV_TOGGLE_CHG_");
            ఒకవేళ (v3Global.opt.దారాలు() > 1) {
                puts("MT_");
            } లేకపోతే {
                puts("ST_");
            }
            emitIQW(nodep->toggleExprp());
            puts("(");
            // Each bit of variable has 2 counters, so the size of variable is half of the size of
            // coverpoint
            puts(cvt_గమ్యం_Str(nodep->declp()->కొలత() / 2));
            puts(", ");
            // Toggle update uses the same object-local counter array that
            // __vlCoverToggleInsert registered.
            putCoverageArray(nodep->declp());
            puts(" + ");
            puts(cvt_గమ్యం_Str(coverage_Bin_అంకె(nodep->declp())));
            puts(", ");
            iterateConst(nodep->toggleExprp());
            puts(", ");
            iterateConst(nodep->toggleCovExprp());
            puts(");\n");
        }
    }
    శూన్య సందర్శించు(AstDisableFork* nodep) override { putns(nodep, "vlProcess->disableFork();\n"); }
    శూన్య సందర్శించు(AstCReturn* nodep) override {
        putns(nodep, "return (");
        iterate_And_తదుపరి_Const_Null(nodep->lhsp());
        puts(");\n");
    }
    శూన్య సందర్శించు(AstDisplay* nodep) override {
        string పాఠ్యం = nodep->fmtp()->పాఠ్యం();
        ఒకవేళ (nodep->కూడు_Newline()) పాఠ్యం += "\n";
        display_కణుపు(nodep, nodep->fmtp(), పాఠ్యం, nodep->fmtp()->exprsp(), false);
    }
    శూన్య సందర్శించు(AstDumpCtl* nodep) override {
        ఎంపిక (nodep->ctlType()) {
        స్థితి VDumpCtlType::FILE:
            putns(nodep, "vlSymsp->_vm_contextp__->dumpfile(");
            iterateConst(nodep->exprp());
            puts(");\n");
            ఆపు;
        స్థితి VDumpCtlType::VARS:
            // We ignore number of levels to dump in exprp()
            ఒకవేళ (v3Global.opt.trace()) {
                putns(nodep, "vlSymsp->_traceDumpOpen();\n");
            } లేకపోతే {
                putns(nodep, "VL_PRINTF_MT(\"-Info: ");
                puts(V3_బహిర్గతం_Formatter::quoteNameControls(protect(nodep->fileline()->filename())));
                puts(":");
                puts(cvt_గమ్యం_Str(nodep->fileline()->lineno()));
                puts(": $dumpvar ignored, as Verilated without --trace");
                puts("\\n\");\n");
            }
            ఆపు;
        స్థితి VDumpCtlType::ALL:
            // $dumpall currently ignored
            ఆపు;
        స్థితి VDumpCtlType::FLUSH:
            // $dumpall currently ignored; would need rework of VCD single thread,
            // or flag we pass-through to next eval() iteration
            ఆపు;
        స్థితి VDumpCtlType::హద్దు_2:
            // $dumplimit currently ignored
            ఆపు;
        స్థితి VDumpCtlType::OFF:
            // Currently ignored as both Vcd and Fst do not support them, as would need "X" dump
            ఆపు;
        స్థితి VDumpCtlType::ON:
            // Currently ignored as $dumpoff is also ignored
            ఆపు;
        అప్రమేయ: nodep->v3fatalSrc("Bad case, unexpected " << nodep->ctlType().ascii());
        }
    }
    శూన్య సందర్శించు(AstScopeName* nodep) override {
        // For use under AstCCalls for dpiImports.  ScopeNames under
        // displays are handled in AstDisplay
        ఒకవేళ (!nodep->dpiExport()) {
            // this is where the DPI import context scope is set
            స్థిరరాశి string scope = nodep->scopeDpiName();
            putnbs(nodep, "(vlSymsp->" + protect("__Vscopep_" + scope) + ")");
        }
    }
    శూన్య సందర్శించు(AstSFormat* nodep) override {
        display_కణుపు(nodep, nodep->fmtp(), nodep->fmtp()->పాఠ్యం(), nodep->fmtp()->exprsp(), false);
    }
    శూన్య సందర్శించు(AstSFormatF* nodep) override {
        display_కణుపు(nodep, nodep, nodep->పాఠ్యం(), nodep->exprsp(), false);
    }
    శూన్య సందర్శించు(AstFScanF* nodep) override {
        display_కణుపు(nodep, nullptr, nodep->పాఠ్యం(), nodep->exprsp(), true);
    }
    శూన్య సందర్శించు(AstSScanF* nodep) override {
        display_కణుపు(nodep, nullptr, nodep->పాఠ్యం(), nodep->exprsp(), true);
    }
    శూన్య సందర్శించు(Ast_విలువ_Plus_వాదనలు* nodep) override {
        putns(nodep, "VL_VALUEPLUSARGS_IN");
        emitIQW(nodep->outp());
        puts("(");
        puts(cvt_గమ్యం_Str(nodep->outp()->వెడల్పు_Min()));
        puts(", ");
        iterateConst(nodep->searchp());
        puts(", ");
        putbs("");
        iterate_And_తదుపరి_Const_Null(nodep->outp());
        puts(")");
    }
    శూన్య సందర్శించు(Ast_Test_Plus_వాదనలు* nodep) override {
        putns(nodep, "VL_TESTPLUSARGS_I(");
        iterateConst(nodep->searchp());
        puts(")");
    }
    శూన్య సందర్శించు(AstFError* nodep) override {
        putns(nodep, "VL_FERROR_I");
        puts(nodep->strp()->isString() ? "N(" : "W(");
        iterate_And_తదుపరి_Const_Null(nodep->filep());
        putbs(", ");
        ఒకవేళ (nodep->strp()->isWide()) {
            puts(cvt_గమ్యం_Str(nodep->strp()->వెడల్పు_పదాలు()));
            putbs(", ");
        }
        iterate_And_తదుపరి_Const_Null(nodep->strp());
        puts(")");
    }
    శూన్య సందర్శించు(AstFGetS* nodep) override {
        emitOpName(nodep, nodep->emitC(), nodep->strgp(), nodep->filep(), nullptr);
    }
    శూన్య సందర్శించు(AstFOpen* nodep) override {
        putns(nodep, "VL_FOPEN_NN(");
        iterateConst(nodep->filenamep());
        putbs(", ");
        iterateConst(nodep->modep());
        puts(");\n");
    }
    శూన్య సందర్శించు(AstFOpenMcd* nodep) override {
        putns(nodep, "VL_FOPEN_MCD_N(");
        iterateConst(nodep->filenamep());
        puts(");\n");
    }
    శూన్య సందర్శించు(Ast_కణుపు_Read_Write_Mem* nodep) override {
        putns(nodep, nodep->cFuncPrefixp());
        puts("N(");
        puts(nodep->isHex() ? "true" : "false");
        putbs(", ");
        // Need real storage width
        puts(cvt_గమ్యం_Str(nodep->memp()->dtypep()->skipRefp()->subDTypep()->వెడల్పు_Min()));
        uint32_t array_lo = 0;
        bool memUsePtr = true;
        {
            స్థిరరాశి AstVarRef* స్థిరరాశి varrefp = VN_CAST(nodep->memp(), VarRef);
            ఒకవేళ (!varrefp) {
                nodep->v3error(nodep->verilogKwd() << " loading non-variable");
            } లేకపోతే ఒకవేళ (VN_IS(varrefp->varp()->dtypeSkipRefp(), AssocArrayDType)
                       || VN_IS(varrefp->varp()->dtypeSkipRefp(), DynArrayDType)) {
                // nodep->memp() below will when verilated code is compiled create a C++ template
                memUsePtr = false;
            } లేకపోతే ఒకవేళ (స్థిరరాశి AstUnpackArrayDType* స్థిరరాశి adtypep
                       = VN_CAST(varrefp->varp()->dtypeSkipRefp(), UnpackArrayDType)) {
                putbs(", ");
                puts(cvt_గమ్యం_Str(varrefp->varp()->dtypep()->arrayUnpackedElements()));
                array_lo = adtypep->lo();
                putbs(", ");
                puts(cvt_గమ్యం_Str(array_lo));
            } లేకపోతే {
                nodep->v3error(
                    nodep->verilogKwd()
                    << " loading other than unpacked/dynamic/associative-array variable");
            }
        }
        putbs(", ");
        iterateConst(nodep->filenamep());
        putbs(", ");
        {
            ఒకవేళ (memUsePtr) puts(" &(");
            iterate_And_తదుపరి_Const_Null(nodep->memp());
            ఒకవేళ (memUsePtr) puts(")");
        }
        putbs(", ");
        ఒకవేళ (nodep->lsbp()) {
            iterate_And_తదుపరి_Const_Null(nodep->lsbp());
        } లేకపోతే {
            puts(cvt_గమ్యం_Str(array_lo));
        }
        putbs(", ");
        ఒకవేళ (nodep->msbp()) {
            iterate_And_తదుపరి_Const_Null(nodep->msbp());
        } లేకపోతే {
            puts("~0ULL");
        }
        puts(");\n");
    }
    శూన్య సందర్శించు(AstFClose* nodep) override {
        putns(nodep, "VL_FCLOSE_I(");
        iterate_And_తదుపరి_Const_Null(nodep->filep());
        puts("); ");
    }
    శూన్య సందర్శించు(AstFFlush* nodep) override {
        ఒకవేళ (!nodep->filep()) {
            putns(nodep, "VL_FFLUSH_MT();");
        } లేకపోతే {
            putns(nodep, "VL_FFLUSH_I(");
            iterate_And_తదుపరి_Const_Null(nodep->filep());
            puts(");\n");
        }
    }
    శూన్య సందర్శించు(AstFSeek* nodep) override {
        putns(nodep, "(VL_FSEEK_I(");
        iterate_And_తదుపరి_Const_Null(nodep->filep());
        puts(", ");
        iterate_And_తదుపరి_Const_Null(nodep->స్థానభ్రంశం());
        puts(", ");
        iterate_And_తదుపరి_Const_Null(nodep->operation());
        puts(") == -1 ? -1 : 0)");
    }
    శూన్య సందర్శించు(AstFTell* nodep) override {
        putns(nodep, "VL_FTELL_I(");
        iterate_And_తదుపరి_Const_Null(nodep->filep());
        puts(")");
    }
    శూన్య సందర్శించు(AstFRewind* nodep) override {
        putns(nodep, "(VL_FSEEK_I(");
        iterate_And_తదుపరి_Const_Null(nodep->filep());
        puts(", 0, 0) == -1 ? -1 : 0)");
    }
    శూన్య సందర్శించు(AstFRead* nodep) override {
        putns(nodep, "VL_FREAD_I(");
        puts(cvt_గమ్యం_Str(nodep->memp()->వెడల్పు_Min()));  // Need real storage width
        putbs(", ");
        uint32_t array_lo = 0;
        uint32_t array_కొలత = 0;
        {
            స్థిరరాశి AstVarRef* స్థిరరాశి varrefp = VN_CAST(nodep->memp(), VarRef);
            ఒకవేళ (!varrefp) {
                nodep->v3error(nodep->verilogKwd() << " loading non-variable");
            } లేకపోతే ఒకవేళ (VN_CAST(varrefp->varp()->dtypeSkipRefp(), BasicDType)) {
            } లేకపోతే ఒకవేళ (స్థిరరాశి AstUnpackArrayDType* స్థిరరాశి adtypep
                       = VN_CAST(varrefp->varp()->dtypeSkipRefp(), UnpackArrayDType)) {
                array_lo = adtypep->lo();
                array_కొలత = adtypep->elementsConst();
            } లేకపోతే {
                nodep->v3error(nodep->verilogKwd()
                               << " loading other than unpacked-array variable");
            }
        }
        puts(cvt_గమ్యం_Str(array_lo));
        putbs(", ");
        puts(cvt_గమ్యం_Str(array_కొలత));
        putbs(", ");
        puts("&(");
        iterate_And_తదుపరి_Const_Null(nodep->memp());
        puts(")");
        putbs(", ");
        iterate_And_తదుపరి_Const_Null(nodep->filep());
        putbs(", ");
        ఒకవేళ (nodep->startp()) {
            iterate_And_తదుపరి_Const_Null(nodep->startp());
        } లేకపోతే {
            puts(cvt_గమ్యం_Str(array_lo));
        }
        putbs(", ");
        ఒకవేళ (nodep->countp()) {
            iterate_And_తదుపరి_Const_Null(nodep->countp());
        } లేకపోతే {
            puts(cvt_గమ్యం_Str(array_కొలత));
        }
        puts(")");
    }
    శూన్య సందర్శించు(Ast_దొంతర_Trace_F* nodep) override { putns(nodep, "VL_STACKTRACE_N()"); }
    శూన్య సందర్శించు(Ast_దొంతర_Trace_T* nodep) override { putns(nodep, "VL_STACKTRACE();\n"); }
    శూన్య సందర్శించు(AstSystemT* nodep) override {
        putns(nodep, "(void)VL_SYSTEM_I");
        emitIQW(nodep->lhsp());
        puts("(");
        ఒకవేళ (nodep->lhsp()->isWide()) {
            puts(cvt_గమ్యం_Str(nodep->lhsp()->వెడల్పు_పదాలు()));
            putbs(", ");
        }
        iterate_And_తదుపరి_Const_Null(nodep->lhsp());
        puts(");\n");
    }
    శూన్య సందర్శించు(AstSystemF* nodep) override {
        putns(nodep, "VL_SYSTEM_I");
        emitIQW(nodep->lhsp());
        puts("(");
        ఒకవేళ (nodep->lhsp()->isWide()) {
            puts(cvt_గమ్యం_Str(nodep->lhsp()->వెడల్పు_పదాలు()));
            putbs(", ");
        }
        iterate_And_తదుపరి_Const_Null(nodep->lhsp());
        puts(")");
    }
    శూన్య సందర్శించు(Ast_Stmt_సమాసం* nodep) override {
        ఒకవేళ (VN_IS(nodep->exprp()->dtypep()->skipRefp(), VoidDType)) {
            putns(nodep, "");
        } లేకపోతే {
            putns(nodep, "std::ignore = ");
        }
        iterateConst(nodep->exprp());
        puts(";\n");
    }
    శూన్య సందర్శించు(Ast_Jump_ఖండం* nodep) override {
        // Allocate label number
        స్థిరరాశి కొలత_t n = m_labelNumbers.కొలత();
        స్థిరరాశి bool new_నమోదు = m_labelNumbers.emplace(nodep, n).రెండవ;
        UASSERT_OBJ(new_నమోదు, nodep, "AstJumpBlock visited twide");
        // Emit
        putns(nodep, "{\n");  // Make it visually obvious label jumps outside these
        VL_RESTORER(m_createdScopeHash);
        iterate_And_తదుపరి_Const_Null(nodep->stmtsp());
        puts("__Vlabel" + std::గమ్యం_string(n) + ": ;\n");
        puts("}\n");
    }
    శూన్య సందర్శించు(AstJumpGo* nodep) override {
        // Retrieve target label number - must already exist (from enclosing AstJumpBlock)
        స్థిరరాశి కొలత_t n = m_labelNumbers.at(nodep->blockp());
        // Emit
        putns(nodep, "goto __Vlabel" + std::గమ్యం_string(n) + ";\n");
    }
    శూన్య సందర్శించు(AstLoop* nodep) override {
        UASSERT_OBJ(!nodep->contsp(), nodep, "'contsp' only used before LinkJump");
        VL_RESTORER(m_createdScopeHash);
        // Special case when the AstLoopTest is first for output readability
        ఒకవేళ (AstLoopTest* స్థిరరాశి testp = VN_CAST(nodep->stmtsp(), LoopTest)) {
            putns(nodep, "while (");
            iterateConst(testp->condp());
            puts(") {\n");
            iterate_And_తదుపరి_Const_Null(testp->nextp());
            puts("}\n");
            తిరిగి;
        }
        // Special case when the AstLoopTest is last for output readability
        ఒకవేళ (Ast_కణుపు* lastp = nodep->stmtsp()) {
            అయితే (Ast_కణుపు* స్థిరరాశి nextp = lastp->nextp()) lastp = nextp;
            ఒకవేళ (AstLoopTest* స్థిరరాశి testp = VN_CAST(lastp, LoopTest)) {
                putns(nodep, "do {\n");
                కొరకు (Ast_కణుపు* p = nodep->stmtsp(); p != lastp; p = p->nextp()) iterateConst(p);
                puts("} while (");
                iterateConst(testp->condp());
                puts(");\n");
                తిరిగి;
            }
        }
        // Emit generic case directly
        putns(nodep, "while (true) {\n");
        iterate_And_తదుపరి_Const_Null(nodep->stmtsp());
        puts("}\n");
    }
    శూన్య సందర్శించు(AstLoopTest* nodep) override {
        VL_RESTORER(m_createdScopeHash);
        putns(nodep, "if (!(");
        iterate_And_తదుపరి_Const_Null(nodep->condp());
        puts(")) break;\n");
    }
    శూన్య సందర్శించు(AstCLocalScope* nodep) override {
        putns(nodep, "{\n");
        VL_RESTORER(m_createdScopeHash);
        iterate_And_తదుపరి_Const_Null(nodep->stmtsp());
        puts("}\n");
    }
    శూన్య సందర్శించు(Ast_కణుపు_If* nodep) override {
        putns(nodep, "if (");
        ఒకవేళ (!nodep->branchPred().unknown()) {
            puts(nodep->branchPred().ascii());
            puts("((");  // Two parens, so that VL_UNLIKELY((class<foo,bar>)) works
        }
        iterate_And_తదుపరి_Const_Null(nodep->condp());
        ఒకవేళ (!nodep->branchPred().unknown()) puts("))");
        puts(") {\n");
        {
            VL_RESTORER(m_createdScopeHash);
            iterate_And_తదుపరి_Const_Null(nodep->thensp());
        }
        puts("}");
        ఒకవేళ (!nodep->elsesp()) {
            puts("\n");
        } లేకపోతే {
            ఒకవేళ (VN_IS(nodep->elsesp(), కణుపు_If) && !nodep->elsesp()->nextp()) {
                puts(" else ");
                iterate_And_తదుపరి_Const_Null(nodep->elsesp());
            } లేకపోతే {
                VL_RESTORER(m_createdScopeHash);
                puts(" else {\n");
                iterate_And_తదుపరి_Const_Null(nodep->elsesp());
                puts("}\n");
            }
        }
    }
    శూన్య సందర్శించు(Ast_సమాసం_Stmt* nodep) override {
        VL_RESTORER(m_createdScopeHash);
        స్థిరరాశి bool కలిగియుంది_Await = nodep->exists([](AstCAwait*) -> bool { తిరిగి true; });
        ఒకవేళ (కలిగియుంది_Await) {
            UASSERT_OBJ(m_cfuncp && m_cfuncp->isCoroutine(), nodep,
                        "AstExprStmt with CAwait must be in coroutine");
            putnbs(nodep, "(co_await ([&]() -> VlCoroutine {\n");
            iterate_And_తదుపరి_Const_Null(nodep->stmtsp());
            puts("co_return;\n");
            ఒకవేళ (!nodep->has_ఫలితం()) {
                puts("}()))");
                తిరిగి;
            }
            puts("}()), ");
            iterate_And_తదుపరి_Const_Null(nodep->resultp());
            puts(")");
            తిరిగి;
        }
        // GCC allows compound statements in expressions, but this is not standard.
        // So we use an immediate-evaluation lambda and comma operator
        putnbs(nodep, "([&]() {\n");
        ఒకవేళ (!nodep->has_ఫలితం()) {
            iterate_And_తదుపరి_Const_Null(nodep->stmtsp());
            puts("}())");
            తిరిగి;
        }
        iterate_And_తదుపరి_Const_Null(nodep->stmtsp());
        puts("}(), ");
        iterate_And_తదుపరి_Const_Null(nodep->resultp());
        puts(")");
    }
    శూన్య సందర్శించు(AstStop* nodep) override {
        putns(nodep, "VL_STOP_MT(");
        putsQuoted(protect(nodep->fileline()->filename()));
        puts(", ");
        puts(cvt_గమ్యం_Str(nodep->fileline()->lineno()));
        puts(", \"\"");
        ఒకవేళ (nodep->isFatal()) puts(", false");
        puts(");\n");
    }
    శూన్య సందర్శించు(Ast_ముగింపు* nodep) override {
        // Disable all the forks so they don't operate after simulation is finished.
        ఒకవేళ (m_cfuncp && m_cfuncp->needProcess()) putns(nodep, "vlProcess->disableFork();\n");
        putns(nodep, "VL_FINISH_MT(");
        putsQuoted(protect(nodep->fileline()->filename()));
        puts(", ");
        puts(cvt_గమ్యం_Str(nodep->fileline()->lineno()));
        puts(", \"\");\n");
    }
    శూన్య సందర్శించు(Ast_ముగింపు_Fork* nodep) override {
        // Disable all the forks so they don't operate after simulation is finished.
        ఒకవేళ (m_cfuncp && m_cfuncp->needProcess()) putns(nodep, "vlProcess->disableFork();\n");
        putns(nodep, "VL_FINISH_MT(");
        putsQuoted(protect(nodep->fileline()->filename()));
        puts(", ");
        puts(cvt_గమ్యం_Str(nodep->fileline()->lineno()));
        puts(", \"\");\n");
        ఒకవేళ (m_cfuncp->isCoroutine()) {
            putns(nodep, "co_return;\n");
        } లేకపోతే {
            putns(nodep, "return;\n");
        }
    }
    శూన్య సందర్శించు(Ast_Print_సమయం_Scale* nodep) override {
        putns(nodep, "VL_PRINTTIMESCALE(");
        putsQuoted(protect(nodep->prettyName()));
        puts(", ");
        putsQuoted(nodep->timeunit().ascii());
        puts(", vlSymsp->_vm_contextp__);\n");
    }
    శూన్య సందర్శించు(AstRand* nodep) override {
        emitOpName(nodep, nodep->emitC(), nodep->seedp(), nullptr, nullptr);
    }
    శూన్య సందర్శించు(AstRandRNG* nodep) override {
        emitOpName(nodep, nodep->emitC(), nullptr, nullptr, nullptr);
    }
    శూన్య సందర్శించు(Ast_సమయం* nodep) override {
        putns(nodep, "VL_TIME_UNITED_Q(");
        UASSERT_OBJ(!nodep->timeunit().isNone(), nodep, "$time has no units");
        స్థిరరాశి ద్విగుణ సమయం_2
            = nodep->timeunit().multiplier() / v3Global.rootp()->timeprecision().multiplier();
        UASSERT_OBJ(సమయం_2 >= 1, nodep, "TimeQ is less than 1, will result in division by zero");
        puts(cvt_గమ్యం_Str(సమయం_2));
        puts(")");
    }
    శూన్య సందర్శించు(Ast_సమయం_D* nodep) override {
        putns(nodep, "VL_TIME_UNITED_D(");
        UASSERT_OBJ(!nodep->timeunit().isNone(), nodep, "$realtime has no units");
        puts(cvt_గమ్యం_Str(nodep->timeunit().multiplier()
                      / v3Global.rootp()->timeprecision().multiplier()));
        puts(")");
    }
    శూన్య సందర్శించు(Ast_Get_Initial_Random_విత్తనం* nodep) override {
        putns(nodep, "vlSymsp->_vm_contextp__->randSeed()");
    }
    శూన్య సందర్శించు(Ast_సమయం_Format* nodep) override {
        putns(nodep, "VL_TIMEFORMAT_IINI(");
        ఒకవేళ (nodep->unitsp()) {
            puts("true, ");
            iterate_And_తదుపరి_Const_Null(nodep->unitsp());
        } లేకపోతే {
            puts("false, 0");
        }
        puts(", ");
        ఒకవేళ (nodep->precisionp()) {
            puts("true, ");
            iterate_And_తదుపరి_Const_Null(nodep->precisionp());
        } లేకపోతే {
            puts("false, 0");
        }
        puts(", ");
        ఒకవేళ (nodep->suffixp()) {
            puts("true, ");
            iterate_And_తదుపరి_Const_Null(nodep->suffixp());
        } లేకపోతే {
            puts("false, \"\"");
        }
        puts(", ");
        ఒకవేళ (nodep->widthp()) {
            puts("true, ");
            iterate_And_తదుపరి_Const_Null(nodep->widthp());
        } లేకపోతే {
            puts("false, 0");
        }
        puts(", vlSymsp->_vm_contextp__);\n");
    }
    శూన్య సందర్శించు(Ast_సమయం_Precision* nodep) override {
        putns(nodep, "vlSymsp->_vm_contextp__->timeprecision()");
    }

    // Nodes involing AstText
    శూన్య సందర్శించు(Ast_పాఠ్యం* nodep) override {
        // All Text should be under TextBlock/CStmt/CStmtUser/CExpr/CExprUser
        nodep->v3fatalSrc("Text node in unexpected position");
    }
    శూన్య సందర్శించు(Ast_పాఠ్యం_ఖండం* nodep) override {
        putnbs(nodep, "");
        puts(nodep->ఉపసర్గ());
        emit_Nodes_With_పాఠ్యం(nodep->nodesp(), false, true, nodep->separator());
        puts(nodep->suffix());
    }
    శూన్య సందర్శించు(AstCStmt* nodep) override {
        putnbs(nodep, "");
        emit_Nodes_With_పాఠ్యం(nodep->nodesp(), false, true, "");
        ensure_New_పంక్తి();
    }
    శూన్య సందర్శించు(AstCExpr* nodep) override {
        putnbs(nodep, "");
        emit_Nodes_With_పాఠ్యం(nodep->nodesp(), false, true, "");
    }
    శూన్య సందర్శించు(AstCStmtUser* nodep) override {
        putnbs(nodep, "");
        ofp()->putsNoTracking("\n");
        ఒకవేళ (nodep->fromDollarC() && v3Global.opt.decoration() && !v3Global.opt.protectIds()) {
            ofp()->putsNoTracking("// $c statement at " + nodep->fileline()->ascii() + "\n");
        }
        emit_Nodes_With_పాఠ్యం(nodep->nodesp(), m_useSelfForThis, false, "");
        puts("\n");
    }
    శూన్య సందర్శించు(AstCExprUser* nodep) override {
        putnbs(nodep, "");
        ofp()->putsNoTracking("\n");
        ఒకవేళ (/* is always from $c */ v3Global.opt.decoration() && !v3Global.opt.protectIds()) {
            ofp()->putsNoTracking(
                (nodep->is_శుద్ధ() ? "// $cpure expression at " : "// $c expression at ")
                + nodep->fileline()->ascii() + "\n");
        }
        emit_Nodes_With_పాఠ్యం(nodep->nodesp(), m_useSelfForThis, false, "");
        puts("\n");
    }

    // Operators
    శూన్య సందర్శించు(Ast_కణుపు_Termop* nodep) override {
        emitOpName(nodep, nodep->emitC(), nullptr, nullptr, nullptr);
    }
    శూన్య సందర్శించు(Ast_కణుపు_Uniop* nodep) override {
        ఒకవేళ (nodep->emit_Check_Max_పదాలు()
            && (nodep->వెడల్పు_పదాలు() > VL_MULS_MAX_పదాలు
                || nodep->lhsp()->వెడల్పు_పదాలు() > VL_MULS_MAX_పదాలు)) {
            nodep->v3warn(
                E_UNSUPPORTED,
                "Unsupported: "
                    << nodep->prettyOperatorName() << " operator of " << nodep->వెడల్పు()
                    << " bits exceeds hardcoded limit VL_MULS_MAX_WORDS in verilatedos.h");
        }
        ఒకవేళ (emitSimpleOk(nodep)) {
            putnbs(nodep, "(");
            puts(nodep->emitSimpleOperator());
            puts(" ");
            iterate_And_తదుపరి_Const_Null(nodep->lhsp());
            puts(")");
        } లేకపోతే {
            emitOpName(nodep, nodep->emitC(), nodep->lhsp(), nullptr, nullptr);
        }
    }
    శూన్య సందర్శించు(Ast_కణుపు_Biop* nodep) override {
        ఒకవేళ (nodep->emit_Check_Max_పదాలు() && nodep->వెడల్పు_పదాలు() > VL_MULS_MAX_పదాలు) {
            nodep->v3warn(
                E_UNSUPPORTED,
                "Unsupported: "
                    << nodep->prettyOperatorName() << " operator of " << nodep->వెడల్పు()
                    << " bits exceeds hardcoded limit VL_MULS_MAX_WORDS in verilatedos.h");
        }
        ఒకవేళ (emitSimpleOk(nodep)) {
            putnbs(nodep, "(");
            iterate_And_తదుపరి_Const_Null(nodep->lhsp());
            puts(" ");
            putbs(nodep->emitSimpleOperator());
            puts(" ");
            iterate_And_తదుపరి_Const_Null(nodep->rhsp());
            puts(")");
        } లేకపోతే {
            emitOpName(nodep, nodep->emitC(), nodep->lhsp(), nodep->rhsp(), nullptr);
        }
    }
    శూన్య సందర్శించు(Ast_కణుపు_Triop* nodep) override {
        UASSERT_OBJ(!emitSimpleOk(nodep), nodep, "Triop cannot be described in a simple way");
        emitOpName(nodep, nodep->emitC(), nodep->lhsp(), nodep->rhsp(), nodep->thsp());
    }
    శూన్య సందర్శించు(AstCvtPackString* nodep) override { emitCvtPackStr(nodep->lhsp()); }
    శూన్య సందర్శించు(AstRedXor* nodep) override {
        ఒకవేళ (nodep->lhsp()->isWide()) {
            సందర్శించు(static_cast<Ast_కణుపు_Uniop*>(nodep));
        } లేకపోతే {
            స్థిరరాశి AstVarRef* స్థిరరాశి vrefp = VN_CAST(nodep->lhsp(), VarRef);
            స్థిరరాశి పూర్ణాంకం వెడల్పు_Pow2 = vrefp ? vrefp->varp()->dtypep()->వెడల్పు_Pow2()
                                        : nodep->lhsp()->dtypep()->వెడల్పు_Pow2();
            UASSERT_OBJ(వెడల్పు_Pow2 > 1, nodep,
                        "Reduction over single bit value should have been folded");
            putnbs(nodep, "VL_REDXOR_");
            puts(cvt_గమ్యం_Str(వెడల్పు_Pow2));
            puts("(");
            iterate_And_తదుపరి_Const_Null(nodep->lhsp());
            puts(")");
        }
    }
    శూన్య సందర్శించు(AstCCast* nodep) override {
        // Extending a value of the same word width is just a NOP.
        ఒకవేళ (స్థిరరాశి AstClassRefDType* స్థిరరాశి classDtypep
            = VN_CAST(nodep->dtypep()->skipRefp(), ClassRefDType)) {
            putns(nodep, "(" + classDtypep->cType("", false, false) + ")(");
        } లేకపోతే ఒకవేళ (nodep->కొలత() <= VL_BYTESIZE) {
            putns(nodep, "(::CData)(");
        } లేకపోతే ఒకవేళ (nodep->కొలత() <= VL_SHORTSIZE) {
            putns(nodep, "(::SData)(");
        } లేకపోతే ఒకవేళ (nodep->కొలత() <= VL_IDATASIZE) {
            putns(nodep, "(::IData)(");
        } లేకపోతే {
            putns(nodep, "(::QData)(");
        }
        iterate_And_తదుపరి_Const_Null(nodep->lhsp());
        puts(")");
    }
    శూన్య సందర్శించు(AstCond* nodep) override {
        // Widths match up already, so we'll just use C++'s operator w/o any temps.
        ఒకవేళ (nodep->thenp()->isWide()) {
            emitOpName(nodep, nodep->emitC(), nodep->condp(), nodep->thenp(), nodep->elsep());
        } లేకపోతే {
            putnbs(nodep, "(");
            iterate_And_తదుపరి_Const_Null(nodep->condp());
            putbs(" ? ");
            iterate_And_తదుపరి_Const_Null(nodep->thenp());
            putbs(" : ");
            iterate_And_తదుపరి_Const_Null(nodep->elsep());
            puts(")");
        }
    }
    శూన్య సందర్శించు(Ast_సరిపోలిక_Masked* nodep) override {
        emitOpName(nodep, nodep->emitC(), nodep->lhsp(), nodep->matchp(), nullptr);
    }
    శూన్య సందర్శించు(AstMemberSel* nodep) override {
        స్థిరరాశి AstVar* స్థిరరాశి varp = nodep->varp();
        స్థిరరాశి bool dereferenceCovergroupRef
            = varp->covergroupRefMember() && nodep->access().isReadOrRW();
        ఒకవేళ (dereferenceCovergroupRef) putnbs(nodep, "(*");
        iterate_And_తదుపరి_Const_Null(nodep->fromp());
        putnbs(nodep, "->");
        ఒకవేళ (varp->isIfaceRef()) {
            // varp is the __Viftop companion (e.g. "tx__Viftop"); use the
            // MemberSel name which matches the cell's C++ member (e.g. "tx").
            puts(nodep->nameProtect());
        } లేకపోతే {
            puts(varName(varp));
        }
        ఒకవేళ (dereferenceCovergroupRef) puts(")");
    }
    శూన్య సందర్శించు(AstStructSel* nodep) override {
        iterate_And_తదుపరి_Const_Null(nodep->fromp());
        putnbs(nodep, ".");
        puts(nodep->nameProtect());
    }
    శూన్య సందర్శించు(AstNullCheck* nodep) override {
        putns(nodep, "VL_NULL_CHECK(");
        iterate_And_తదుపరి_Const_Null(nodep->lhsp());
        puts(", ");
        putsQuoted(protect(nodep->fileline()->filename()));
        puts(", ");
        puts(cvt_గమ్యం_Str(nodep->fileline()->lineno()));
        puts(")");
    }
    శూన్య సందర్శించు(AstNewCopy* nodep) override {
        // Polymorphic shallow clone: preserves runtime type via virtual clone()
        // VL_NULL_CHECK enforces null check per IEEE 1800-2023 8.7
        putns(nodep, "VL_NULL_CHECK(");
        ఒకవేళ (VN_IS(nodep->rhsp(), Const) && VN_AS(nodep->rhsp(), Const)->isNull()) {
            // V3Const folded rhs to null: emit a typed empty ref so VL_NULL_CHECK fires
            స్థిరరాశి AstClassRefDType* స్థిరరాశి refDTypep
                = VN_CAST(nodep->dtypep()->skipRefp(), ClassRefDType);
            puts(refDTypep->cType("", false, false) + "{}");
        } లేకపోతే {
            iterate_And_తదుపరి_Const_Null(nodep->rhsp());
        }
        puts(", ");
        putsQuoted(protect(nodep->fileline()->filename()));
        puts(", ");
        puts(cvt_గమ్యం_Str(nodep->fileline()->lineno()));
        puts(").clone(vlSymsp->__Vm_deleter)");
    }
    శూన్య సందర్శించు(AstSel* nodep) override {
        // Note ASSIGN checks for this on a LHS
        UASSERT_OBJ(nodep->వెడల్పు_Min() == nodep->వెడల్పు_Const(), nodep, "Width mismatch");
        emitOpName(nodep, nodep->emitC(), nodep->fromp(), nodep->lsbp(), nullptr);
    }
    శూన్య సందర్శించు(AstReplicate* nodep) override {
        ఒకవేళ (nodep->srcp()->వెడల్పు_Min() == 1 && !nodep->isWide()) {
            UASSERT_OBJ((static_cast<పూర్ణాంకం>(VN_AS(nodep->countp(), Const)->గమ్యం_UInt())
                         * nodep->srcp()->వెడల్పు_Min())
                            == nodep->వెడల్పు_Min(),
                        nodep, "Replicate non-constant or width miscomputed");
            putns(nodep, "VL_REPLICATE_");
            emitIQW(nodep);
            puts("OI(");
            ఒకవేళ (nodep->srcp()) puts(cvt_గమ్యం_Str(nodep->srcp()->వెడల్పు_Min()));
            puts(", ");
            iterate_And_తదుపరి_Const_Null(nodep->srcp());
            puts(", ");
            iterate_And_తదుపరి_Const_Null(nodep->countp());
            puts(")");
        } లేకపోతే {
            emitOpName(nodep, nodep->emitC(), nodep->srcp(), nodep->countp(), nullptr);
        }
    }
    శూన్య emit_ప్రవాహం_R(Ast_ప్రవాహం_R* nodep, Ast_కణుపు* మాతృ) {
        //TODO: This might need to handle more cases like the visit(AstStreamR) function
        emitOpName(nodep, nodep->emitC(), nodep->lhsp(), nodep->rhsp(), nullptr);
    }
    శూన్య సందర్శించు(Ast_ప్రవాహం_R* nodep) override {
        //The parrent node of our AstStreamR will give just enough info for what streamR should
        //output if nodep->backp() is not the parent then emitStreamR should have been used. throw
        //an error
        bool backp_Is_మాతృ = (nodep->backp()->op1p() == nodep || nodep->backp()->op2p() == nodep);
        UASSERT(backp_Is_మాతృ, "can not find return type for streamR");
        ఒకవేళ (స్థిరరాశి Ast_వరుస_DType* స్థిరరాశి qdtp
            = VN_CAST(nodep->backp()->dtypep()->skipRefp(), వరుస_DType)) {
            ఒకవేళ (nodep->lhsp()->isWide()) {
                emitOpName(nodep,
                           "VL_STREAMR_%nq%lq%rq(%lw," + std::గమ్యం_string(qdtp->subDTypep()->వెడల్పు())
                               + ", %P, %li, %ri)",
                           nodep->lhsp(), nodep->rhsp(), nullptr);
            } లేకపోతే {
                emitOpName(nodep, "VL_STREAMR_%nq%lq%rq(%lw, %P, %li, %ri)", nodep->lhsp(),
                           nodep->rhsp(), nullptr);
            }
        } లేకపోతే ఒకవేళ (VN_IS(nodep->lhsp()->dtypep()->skipRefp(), వరుస_DType)) {
            ఒకవేళ (!((nodep->backp()->op1p() && nodep->backp()->op1p()->isWide())
                  || (nodep->backp()->op2p() && nodep->backp()->op2p()->isWide()))) {
                //If our lhsp is a queue make sure we streamR and return the correct type.
                //If either side is wide or the previous node is string type dont use this case
                emitOpName(nodep->backp(), "VL_STREAMR_%nq%lq%rq(%lw, %P, %li, %ri)",
                           nodep->lhsp(), nodep->rhsp(), nullptr);
            } లేకపోతే {
                emitOpName(nodep, nodep->emitC(), nodep->lhsp(), nodep->rhsp(), nullptr);
            }
        } లేకపోతే {
            emitOpName(nodep, nodep->emitC(), nodep->lhsp(), nodep->rhsp(), nullptr);
        }
    }
    శూన్య సందర్శించు(Ast_ప్రవాహం_L* nodep) override {
        // Attempt to use a "fast" stream function for slice size = power of 2
        ఒకవేళ (!nodep->isWide()) {
            స్థిరరాశి uint32_t isPow2 = VN_AS(nodep->rhsp(), Const)->అంకె().లెక్క_Ones() == 1;
            స్థిరరాశి uint32_t ముక్క_కొలత = VN_AS(nodep->rhsp(), Const)->గమ్యం_UInt();
            ఒకవేళ (isPow2 && ముక్క_కొలత <= (nodep->isQuad() ? పరిమాణం(uint64_t) : పరిమాణం(uint32_t))) {
                putns(nodep, "VL_STREAML_FAST_");
                bool ఉపయోగిస్తుంది_వరుస = false;
                Ast_వరుస_DType* qtypep
                    = nodep->backp()->op2p()
                          ? VN_CAST(nodep->backp()->op2p()->dtypep()->skipRefp(), వరుస_DType)
                          : nullptr;
                ఒకవేళ (VN_IS(nodep->backp(), కేటాయించు)
                    && qtypep) {  // If we are assigning to a queue then emit the correct symbol
                    puts("R");  // R for queue
                    ఉపయోగిస్తుంది_వరుస = true;
                } లేకపోతే {
                    emitIQW(nodep);
                }
                emitIQW(nodep->lhsp());
                puts("I(");
                puts(cvt_గమ్యం_Str(nodep->lhsp()->వెడల్పు_Min()));
                puts(", ");
                ఒకవేళ (ఉపయోగిస్తుంది_వరుస) {
                    iterate_And_తదుపరి_Const_Null(nodep->backp()->op2p());
                    puts(", ");
                }
                iterate_And_తదుపరి_Const_Null(nodep->lhsp());
                puts(", ");
                స్థిరరాశి uint32_t rd_log2 = V3_సంఖ్య::log2b(VN_AS(nodep->rhsp(), Const)->గమ్యం_UInt());
                puts(cvt_గమ్యం_Str(rd_log2) + ")");
                తిరిగి;
            }
        }
        ఒకవేళ (VN_IS(nodep->backp(), కేటాయించు)
            && VN_IS(nodep->backp()->op2p()->dtypep()->skipRefp(), వరుస_DType)) {
            పూర్ణాంకం వరుస_వెడల్పు
                = nodep->backp()->op2p()->dtypep()->subDTypep()->వెడల్పు();  //We need to know the
                                                                           //width of both sides
            emitOpName(nodep,
                       "VL_STREAML_%nq%lq%rq(%lw," + std::గమ్యం_string(వరుస_వెడల్పు)
                           + ", %P, %li, %ri)",
                       nodep->lhsp(), nodep->rhsp(), nullptr);
        } లేకపోతే {
            emitOpName(nodep, nodep->emitC(), nodep->lhsp(), nodep->rhsp(), nullptr);
        }
    }
    శూన్య సందర్శించు(AstCastDynamic* nodep) override {
        putnbs(nodep, "VL_CAST_DYNAMIC(");
        iterate_And_తదుపరి_Const_Null(nodep->fromp());
        puts(", ");
        iterate_And_తదుపరి_Const_Null(nodep->శిఖరం());
        puts(")");
    }
    శూన్య సందర్శించు(Ast_లెక్క_బిట్లు* nodep) override {
        putnbs(nodep, "VL_COUNTBITS_");
        emitIQW(nodep->lhsp());
        puts("(");
        puts(cvt_గమ్యం_Str(nodep->lhsp()->వెడల్పు_Min()));
        puts(", ");
        ఒకవేళ (nodep->lhsp()->isWide()) {
            puts(cvt_గమ్యం_Str(nodep->lhsp()->వెడల్పు_పదాలు()));  // Note argument width, not node width
                                                          // (which is always 32)
            puts(", ");
        }
        iterate_And_తదుపరి_Const_Null(nodep->lhsp());
        puts(", ");
        iterate_And_తదుపరి_Const_Null(nodep->rhsp());
        puts(", ");
        iterate_And_తదుపరి_Const_Null(nodep->thsp());
        puts(", ");
        iterate_And_తదుపరి_Const_Null(nodep->fhsp());
        puts(")");
    }
    శూన్య సందర్శించు(AstTriggered* nodep) override {
        // Test each word of the trigger vector with a mask of the relevant bits
        std::map<uint32_t, uint64_t> మాట_Masks;
        కొరకు (స్థిరరాశి uint32_t సూచీ : nodep->indices()) {
            మాట_Masks[VL_BITWORD_Q(సూచీ)] |= VL_బిట్టు_Q(సూచీ);
        }
        putns(nodep, "((");
        bool మొదటి = true;
        కొరకు (స్థిరరాశి స్వయంచాలక& మాట_ముసుగు : మాట_Masks) {
            ఒకవేళ (!మొదటి) puts("\n| ");
            మొదటి = false;
            స్థిరరాశి bool allOnes = మాట_ముసుగు.రెండవ == ~0ULL;
            ఒకవేళ (!allOnes) puts("(" + V3_సంఖ్య{nodep, 64}.సమితి_Quad(మాట_ముసుగు.రెండవ).emitC() + " & ");
            iterateConst(nodep->trigRefp());
            puts("[" + std::గమ్యం_string(మాట_ముసుగు.మొదటి) + "U]");
            ఒకవేళ (!allOnes) puts(")");
        }
        puts(") != 0ULL)");
    }
    // Terminals
    శూన్య సందర్శించు(AstVarRef* nodep) override {
        స్థిరరాశి AstVar* స్థిరరాశి varp = nodep->varp();

        // Reference to constant pool variable is special
        ఒకవేళ (varp->const_Pool_నమోదు()) {
            putns(nodep, EmitCUtil::constPoolName(varp));
            తిరిగి;
        }

        స్థిరరాశి Ast_కణుపు_Module* స్థిరరాశి varModp = EmitCParentModule::get(varp);
        స్థిరరాశి bool dereferenceCovergroupRef
            = varp->covergroupRefMember() && nodep->access().isReadOrRW();
        ఒకవేళ (dereferenceCovergroupRef) putns(nodep, "(*");
        ఒకవేళ (VN_IS(varModp, Class) && varModp != m_modp) {
            // Superclass member reference
            putns(nodep, EmitCUtil::ఉపసర్గ_Name_Protect(varModp) + "::");
        } లేకపోతే ఒకవేళ (varp->isIfaceRef()) {
            putns(nodep, nodep->self_సూచిక_Protect(m_useSelfForThis));
            తిరిగి;
        } లేకపోతే ఒకవేళ (!nodep->self_సూచిక().is_ఖాళీలు()) {
            emitDereference(nodep, nodep->self_సూచిక_Protect(m_useSelfForThis));
        }
        putns(nodep, varName(varp));
        ఒకవేళ (dereferenceCovergroupRef) puts(")");
    }
    శూన్య సందర్శించు(Ast_Addr_యొక్క_CFunc* nodep) override {
        // Note: Can be thought to handle more, but this is all that is needed right now
        స్థిరరాశి AstCFunc* స్థిరరాశి funcp = nodep->funcp();
        UASSERT_OBJ(funcp->isLoose(), nodep, "Cannot take address of non-loose method");
        putns(nodep, "&");
        puts(funcNameProtect(funcp));
    }
    శూన్య సందర్శించు(AstConst* nodep) override {  //
        ఒకవేళ (m_wide_తాత్కాలిక_Refp && nodep->isWide()) {
            UASSERT_OBJ(m_wide_తాత్కాలిక_Refp, nodep, "Wide Constant w/ no temp");
            emitConstantW(nodep, m_wide_తాత్కాలిక_Refp);
            m_wide_తాత్కాలిక_Refp = nullptr;  // We used it, fail if set it a second time
        } లేకపోతే {
            emitConstant(nodep);
        }
    }
    శూన్య సందర్శించు(Ast_కణుపు_Cons* nodep) override { emitValCons(nodep); }
    శూన్య సందర్శించు(AstThisRef* nodep) override {
        putnbs(nodep, nodep->dtypep()->cType("", false, false));
        puts("{");
        puts(VSelf_సూచిక_పాఠ్యం::replaceThis(m_useSelfForThis, "this"));
        puts("}");
    }
    శూన్య సందర్శించు(Ast_కణుపు_Sel* nodep) override {
        ఒకవేళ (!VN_IS(nodep, ArraySel) && !VN_IS(nodep, మాట_Sel)) {
            సందర్శించు(static_cast<Ast_కణుపు_Biop*>(nodep));
            తిరిగి;
        }
        // ArraySel or WordSel
        iterate_And_తదుపరి_Const_Null(nodep->fromp());
        // Special case constant index for readability
        ఒకవేళ (AstConst* స్థిరరాశి idxp = VN_CAST(nodep->bitp(), Const)) {
            puts("[" + std::గమ్యం_string(idxp->గమ్యం_UInt()) + "U]");
            తిరిగి;
        }
        putbs("[");
        iterate_And_తదుపరి_Const_Null(nodep->bitp());
        puts("]");
    }

    //
    శూన్య సందర్శించు(Ast_Exec_లేఖాచిత్రం* nodep) override {
        // The location of the AstExecGraph within the containing AstCFunc is where we want to
        // invoke the graph and wait for it to complete. The run-time library does just that.
        putns(nodep, "vl_invokeExecGraph(" + EmitCUtil::exec_లేఖాచిత్రం_Name(nodep) + ", vlSelf, ");
        iterateConst(nodep->triggerp());
        puts(".data());\n");
    }

    // Default
    శూన్య సందర్శించు(Ast_కణుపు* nodep) override {  // LCOV_EXCL_START
        putns(nodep, "\n???? // "s + nodep->prettyTypeName() + "\n");
        iterateChildrenConst(nodep);
        ఒకవేళ (!v3Global.opt.lintOnly()) {  // An internal problem, so suppress
            nodep->v3fatalSrc("Unknown node type reached emitter: " << nodep->prettyTypeName());
        }
    }  // LCOV_EXCL_STOP

రక్షిత:
    EmitCFunc() = అప్రమేయ;
    ~EmitCFunc() override = అప్రమేయ;
};

#అయితే_ముగింపు  // guard
