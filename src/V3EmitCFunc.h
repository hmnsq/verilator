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

#جے_بنا_تعریف VERILATOR_V3EMITCFUNC_H_
#تعریف_کرو VERILATOR_V3EMITCFUNC_H_

#ضمن "config_build.h"
#ضمن "verilatedos.h"

#ضمن "V3EmitCConstInit.h"
#ضمن "V3Global.h"
#ضمن "V3MemberMap.h"

#ضمن <algorithm>
#ضمن <map>
#ضمن <unordered_map>
#ضمن <unordered_set>
#ضمن <vector>

// Number of VL_CONST_W_*X's in verilated.h (IE VL_CONST_W_8X is last)
constexpr صحيح EMITC_عدد_CONSTW = 8;

//######################################################################
// Emit lazy forward declarations

class EmitCLazyDecls final : عام VNVisitorConst {
    // NODE STATE/TYPES
    // None allowed to support threaded emitting

    // MEMBERS
    std::unordered_سیٹ<string> m_emittedManually;  // Set of names already declared manually.
    EmitCBaseVisitorConst& m_emitter;  // For access to file output
    bool m_needs_Blank_سطر = false;  // Emit blank line if any declarations were emitted (cosmetic)
    std::سیٹ<Ast_گرہ*> m_emitted;  // -> in set. Already emitted decl for symbols.

    // METHODS
    bool declaredOnce(Ast_گرہ* nodep) { أرجع m_emitted.داخل_کرو(nodep).دوجا; }

    فراغ lazyDeclare(AstCFunc* funcp) {
        // Already declared in this compilation unit
        إذا (!declaredOnce(funcp)) أرجع;
        // Check if this kind of function is lazily declared
        إذا (!(funcp->is_طریقہ() && funcp->isLoose()) && !funcp->dpiImportPrototype()) أرجع;
        // Already declared manually
        إذا (m_emittedManually.گنتی(funcp->nameProtect())) أرجع;
        // Needs lazy declaration, emit one
        m_emitter.emitCFuncDecl(funcp, EmitCParentModule::get(funcp), funcp->dpiImportPrototype());
        m_needs_Blank_سطر = true;
    }

    فراغ lazyDeclareConstPoolVar(AstVar* varp) {
        إذا (!declaredOnce(varp)) أرجع;  // Already declared
        ثابت string nameProtect = EmitCUtil::constPoolName(varp);
        m_emitter.putns(varp, "extern const ");
        m_emitter.puts(varp->dtypep()->cType(nameProtect, false, false));
        m_emitter.puts(";\n");
        m_needs_Blank_سطر = true;
    }

    // VISITORS
    فراغ ویکھو(Ast_گرہ_CCall* nodep) override {
        lazyDeclare(nodep->funcp());
        iterateChildrenConst(nodep);
    }

    فراغ ویکھو(Ast_Addr_دا_CFunc* nodep) override {
        lazyDeclare(nodep->funcp());
        iterateChildrenConst(nodep);
    }

    فراغ ویکھو(AstVarRef* nodep) override {
        AstVar* ثابت varp = nodep->varp();
        // Only constant pool symbols are lazy declared for now ...
        إذا (EmitCParentModule::get(varp)->isConstPool()) lazyDeclareConstPoolVar(varp);
    }

    فراغ ویکھو(Ast_Exec_گراف* nodep) override {
        إذا (!declaredOnce(nodep)) أرجع;
        m_emitter.putns(nodep, "extern VlExecGraph " + EmitCUtil::exec_گراف_Name(nodep) + ";\n");
        m_needs_Blank_سطر = true;
    }

    فراغ ویکھو(Ast_گرہ* nodep) override { iterateChildrenConst(nodep); }

عام:
    explicit EmitCLazyDecls(EmitCBaseVisitorConst& emitter)
        : m_emitter{emitter} {}
    فراغ emit(Ast_گرہ* nodep) {
        m_needs_Blank_سطر = false;
        iterateChildrenConst(nodep);
        إذا (m_needs_Blank_سطر) m_emitter.puts("\n");
    }
    فراغ emit(ثابت string& اگیتر, ثابت string& name, ثابت string& suffix) {
        m_emittedManually.داخل_کرو(name);
        m_emitter.ensure_New_سطر();
        m_emitter.puts(اگیتر);
        m_emitter.puts(name);
        m_emitter.puts(suffix);
        m_emitter.ensure_New_سطر();
    }
    فراغ declared(AstCFunc* nodep) { m_emitted.داخل_کرو(nodep); }
    فراغ reset() { m_emitted.clear(); }
};

// ######################################################################
//  Emit statements and expressions

class EmitCFunc VL_NOT_FINAL : عام EmitCConstInit {
    VMemberMap m_memberMap;
    AstVarRef* m_wide_عارضی_Refp = nullptr;  // Variable that _WW macros should be setting
    std::unordered_map<Ast_Jump_بلاک*, ناپ_t> m_labelNumbers;  // Label numbers for AstJumpBlocks
    bool m_createdScopeHash = false;  // Already created a scope hash

محمي:
    VL_DEFINE_DEBUG_FUNCTIONS;

    EmitCLazyDecls m_lazyDecls{*this};  // Visitor for emitting lazy declarations
    bool m_useSelfForThis = false;  // Replace "this" with "vlSelf"
    bool m_usevlSelfRef = false;  // Use vlSelfRef reference instead of vlSelf pointer
    ثابت Ast_گرہ_Module* m_modp = nullptr;  // Current module being emitted
    ثابت AstCFunc* m_cfuncp = nullptr;  // Current function being emitted
    bool m_instantiatesOwnProcess = false;
    ثابت AstClassPackage* m_classOrPackage = nullptr;  // Pointer to current class or package
    string m_classOrPackageHash;  // Hash of class or package name

    bool constructorNeedsProcess(ثابت AstClass* ثابت classp) {
        ثابت Ast_گرہ* ثابت newp = m_memberMap.لبھو_Member(classp, "new");
        إذا (!newp) أرجع false;
        ثابت AstCFunc* ثابت ctorp = VN_CAST(newp, CFunc);
        إذا (!ctorp) أرجع false;
        UASSERT_OBJ(ctorp->isConstructor(), ctorp, "`new` is not a constructor!");
        أرجع ctorp->needProcess();
    }

    bool constructorNeedsProcess(ثابت Ast_گرہ_DType* ثابت dtypep) {
        إذا (ثابت AstClassRefDType* ثابت crefdtypep = VN_CAST(dtypep, ClassRefDType))
            أرجع constructorNeedsProcess(crefdtypep->classp());
        أرجع false;
    }

    bool coverage_ورتدا_Local_Counter(Ast_گرہ_Cover_Decl* ثابت declp) ثابت {
        // Only functions with an object receiver can address the module-local
        // coverage array. Static/package/class helper functions still use the
        // symbol-table array, as they may have vlSymsp but no vlSelf/this.
        // Also require the coverage declaration to belong to the current
        // module; cloned task bodies can increment declarations owned by a
        // different module, and those must keep using the global array.
        أرجع m_cfuncp && !m_cfuncp->isStatic() && !VN_IS(m_modp, Class)
               && !VN_IS(m_modp, ClassPackage)
               && EmitCParentModule::get(declp->انکڑے_Decl_Thisp()) == m_modp;
    }
    صحيح coverage_Bin_عدد(Ast_گرہ_Cover_Decl* ثابت declp, bool forceGlobal = false) ثابت {
        أرجع !forceGlobal && coverage_ورتدا_Local_Counter(declp)
                   ? declp->انکڑے_Decl_Thisp()->local_Bin_عدد()
                   : declp->انکڑے_Decl_Thisp()->bin_عدد();
    }
    فراغ putCoverageArray(Ast_گرہ_Cover_Decl* ثابت declp, bool forceGlobal = false) {
        إذا (!forceGlobal && coverage_ورتدا_Local_Counter(declp)) {
            // Keep counters on the module object when possible, so every
            // no-inline instance has storage available if coverage output is
            // later written with forcePerInstance.
            puts(m_cfuncp->isLoose() ? "vlSelf->__Vcoverage" : "this->__Vcoverage");
        } وإلا {
            // Static/package/class helper functions do not have vlSelf, so
            // keep using the shared symbol-table coverage array there.
            puts("vlSymsp->__Vcoverage");
        }
    }
    فراغ emit_Cover_Other_Decl_داخل_کرو(AstCoverOtherDecl* nodep, bool forceGlobal = false) {
        putns(nodep, "vlSelf->__vlCoverInsert(");  // As Declared in emitCoverageDecl
        putCoverageArray(nodep, forceGlobal);
        puts(" + ");
        puts(cvt_منزل_Str(coverage_Bin_عدد(nodep, forceGlobal)));
        // first controls whether duplicate module instances are collapsed in
        // the default coverage view. The following flag says whether countp
        // points to an object-local counter; only those counters must be kept
        // real for later instances so forcePerInstance can split hierarchy.
        puts(", first");  // Enable, passed from __Vconfigure parameter
        puts(", ");
        puts(!forceGlobal && coverage_ورتدا_Local_Counter(nodep) ? "true" : "false");
        puts(", ");
        putsQuoted(protect(nodep->fileline()->filename()));
        puts(", ");
        puts(cvt_منزل_Str(nodep->fileline()->lineno()));
        puts(", ");
        puts(cvt_منزل_Str(nodep->وتھ() + nodep->fileline()->پہلا_کالم()));
        puts(", ");
        putsQuoted((!nodep->hier().خالی_تھاواں() ? "." : "")
                   + VIdProtect::protect_الفاظ_If(nodep->hier(), nodep->protect()));
        puts(", ");
        putsQuoted(VIdProtect::protect_الفاظ_If(nodep->ورقہ(), nodep->protect()));
        puts(", ");
        putsQuoted(VIdProtect::protect_الفاظ_If(nodep->comment(), nodep->protect()));
        puts(", ");
        putsQuoted(nodep->linescov());
        puts(", ");
        putsQuoted(VIdProtect::protect_الفاظ_If(nodep->fsmVar(), nodep->protect()));
        puts(", ");
        putsQuoted(VIdProtect::protect_الفاظ_If(nodep->fsmFrom(), nodep->protect()));
        puts(", ");
        putsQuoted(VIdProtect::protect_الفاظ_If(nodep->fsm_منزل(), nodep->protect()));
        puts(", ");
        putsQuoted(VIdProtect::protect_الفاظ_If(nodep->fsm_ٹیگ(), nodep->protect()));
        puts(");\n");
    }

عام:
    // METHODS
    bool display_Emit_سرخی(Ast_گرہ* nodep);
    فراغ display_گرہ(Ast_گرہ* nodep, AstSFormatF* fmtp, ثابت string& vformat, Ast_گرہ* exprsp,
                     bool is_جانچ);

    bool emitSimpleOk(Ast_گرہ_اظہاریہ* nodep);
    فراغ emitIQW(ثابت Ast_گرہ* nodep) {
        // See "Type letters" in verilated.h
        // Other abbrevs: "C"har, "S"hort, "F"loat, "D"ouble, stri"N"g, "R"=queue, "U"npacked
        puts(nodep->dtypep()->skipRefp()->charIQWN());
    }
    فراغ emitRU(ثابت Ast_گرہ* nodep) {
        ثابت Ast_گرہ_DType* dtp = nodep->dtypep()->skipRefp();
        // See "Type letters" in verilated.h
        إذا (VN_IS(dtp, UnpackArrayDType))
            puts("U");
        وإلا إذا (VN_IS(dtp, قطار_DType) || VN_IS(dtp, DynArrayDType))
            puts("R");
    }
    فراغ emitScIQW(AstVar* nodep) {
        UASSERT_OBJ(nodep->isSc(), nodep, "emitting SystemC operator on non-SC variable");
        puts(nodep->isScBigUint()                           ? "SB"
             : (nodep->isScUint() || nodep->isScUintBool()) ? "SU"
             : nodep->isScBv()                              ? "SW"
             : nodep->isDouble()                            ? "SD"
                                                            : (nodep->isScQuad() ? "SQ" : "SI"));
    }
    فراغ emitDatap(ثابت Ast_گرہ* nodep) {
        // When passing to a function with va_args the compiler doesn't
        // know need a pointer so when wide, need to look inside VlWide
        إذا (nodep->isWide()) puts(".data()");
    }
    فراغ emitOpName(Ast_گرہ* nodep, ثابت string& format, Ast_گرہ* lhsp, Ast_گرہ* rhsp,
                    Ast_گرہ* thsp);
    فراغ emit_CCall_دلیلاں(ثابت Ast_گرہ_CCall* nodep, ثابت string& self_اشارہ_گر, bool وچ_Process);
    فراغ emitDereference(Ast_گرہ* nodep, ثابت string& اشارہ_گر);
    std::string dereferenceString(ثابت std::string& اشارہ_گر) ثابت;
    فراغ emitCvtPackStr(Ast_گرہ* nodep);
    فراغ emitConstant(AstConst* nodep);
    فراغ emitConstantString(ثابت AstConst* nodep);
    فراغ emitVarReset(ثابت string& اگیتر, AstVar* varp, bool constructing);
    string emitVarResetRecurse(ثابت AstVar* varp, bool constructing,
                               ثابت string& varNameProtected, Ast_گرہ_DType* dtypep, صحيح ڈونگھائی,
                               ثابت string& suffix, ثابت Ast_گرہ* valuep);
    فراغ emitVarResetScopeHash();
    فراغ emitChangeDet();
    فراغ put_Comma_Iterate_اگلا(Ast_گرہ* nodep, bool comma = false) {
        لكل (Ast_گرہ* subnodep = nodep; subnodep; subnodep = subnodep->nextp()) {
            إذا (comma) puts(", ");
            iterateConst(subnodep);
            comma = true;
        }
    }
    سانچا <typename T>
    string optionalProcArg(ثابت T* ثابت nodep) {
        أرجع (nodep && constructorNeedsProcess(nodep)) ? "vlProcess, " : "";
    }
    ثابت AstCNew* getSuperNewCallRecursep(Ast_گرہ* ثابت nodep) {
        // Get the super.new call
        إذا (!nodep) أرجع nullptr;
        إذا (ثابت AstCNew* ثابت cnewp = VN_CAST(nodep, CNew)) أرجع cnewp;
        إذا (ثابت Ast_Stmt_اظہاریہ* ثابت stmtp = VN_CAST(nodep, Stmt_اظہاریہ)) {
            إذا (ثابت AstCNew* ثابت cnewp = VN_CAST(stmtp->exprp(), CNew)) أرجع cnewp;
        }
        إذا (ثابت Ast_Jump_بلاک* ثابت blockp = VN_CAST(nodep, Jump_بلاک)) {
            إذا (ثابت AstCNew* ثابت cnewp = getSuperNewCallRecursep(blockp->stmtsp())) {
                أرجع cnewp;
            }
        }
        إذا (ثابت AstCNew* ثابت cnewp = getSuperNewCallRecursep(nodep->nextp())) أرجع cnewp;
        أرجع nullptr;
    }

    فراغ emitConstantW(AstConst* nodep, AstVarRef* assigntop) {
        // For tradition and compilation speed, assign each word directly into
        // output variable instead of using '='
        putns(nodep, "");
        إذا (nodep->عدد().isFourState()) {
            nodep->v3warn(E_UNSUPPORTED, "Unsupported: 4-state numbers in this context");
            أرجع;
        }

        صحيح اتے_چوڑائی = nodep->عدد().چوڑائی_منزل_Fit();
        صحيح chunks = 0;
        إذا (اتے_چوڑائی > EMITC_عدد_CONSTW * VL_EDATASIZE) {
            // Output e.g. 8 words in groups of e.g. 8
            chunks = (اتے_چوڑائی - 1) / (EMITC_عدد_CONSTW * VL_EDATASIZE);
            اتے_چوڑائی %= (EMITC_عدد_CONSTW * VL_EDATASIZE);
            إذا (اتے_چوڑائی == 0) اتے_چوڑائی = (EMITC_عدد_CONSTW * VL_EDATASIZE);
        }
        {  // Upper e.g. 8 words
            إذا (chunks) {
                putnbs(nodep, "VL_CONSTHI_W_");
                puts(cvt_منزل_Str(VL_الفاظ_I(اتے_چوڑائی)));
                puts("X(");
                puts(cvt_منزل_Str(nodep->چوڑائی_Min()));
                puts(",");
                puts(cvt_منزل_Str(chunks * EMITC_عدد_CONSTW * VL_EDATASIZE));
            } وإلا {
                putnbs(nodep, "VL_CONST_W_");
                puts(cvt_منزل_Str(VL_الفاظ_I(اتے_چوڑائی)));
                puts("X(");
                puts(cvt_منزل_Str(nodep->چوڑائی_Min()));
            }
            puts(",");
            إذا (!assigntop->self_اشارہ_گر().is_خالی_تھاواں()) {
                emitDereference(assigntop, assigntop->self_اشارہ_گر_Protect(m_useSelfForThis));
            }
            puts(varName(assigntop->varp()));
            لكل (صحيح لفظ = VL_الفاظ_I(اتے_چوڑائی) - 1; لفظ >= 0; لفظ--) {
                // Only 32 bits - llx + long long here just to appease CPP format warning
                ofp()->لکھو(",0x%08" PRIx64, static_cast<uint64_t>(nodep->عدد().edata_لفظ(
                                                   لفظ + chunks * EMITC_عدد_CONSTW)));
            }
            puts(")");
        }
        لكل (chunks--; chunks >= 0; chunks--) {
            puts(";\n");
            putbs("VL_CONSTLO_W_");
            puts(cvt_منزل_Str(EMITC_عدد_CONSTW));
            puts("X(");
            puts(cvt_منزل_Str(chunks * EMITC_عدد_CONSTW * VL_EDATASIZE));
            puts(",");
            إذا (!assigntop->self_اشارہ_گر().is_خالی_تھاواں()) {
                emitDereference(assigntop, assigntop->self_اشارہ_گر_Protect(m_useSelfForThis));
            }
            puts(varName(assigntop->varp()));
            لكل (صحيح لفظ = EMITC_عدد_CONSTW - 1; لفظ >= 0; لفظ--) {
                // Only 32 bits - llx + long long here just to appease CPP format warning
                ofp()->لکھو(",0x%08" PRIx64, static_cast<uint64_t>(nodep->عدد().edata_لفظ(
                                                   لفظ + chunks * EMITC_عدد_CONSTW)));
            }
            puts(")");
        }
    }

    فراغ emit_Nodes_With_متن(Ast_گرہ* nodesp, bool useSelfForThis, bool tracking,
                           ثابت std::string& separator) {
        لكل (Ast_گرہ* nodep = nodesp; nodep; nodep = nodep->nextp()) {
            إذا (ثابت Ast_متن* ثابت textp = VN_CAST(nodep, متن_2)) {
                ثابت std::string متن
                    = VSelf_اشارہ_گر_متن::replaceThis(useSelfForThis, textp->متن());
                إذا (tracking) {
                    puts(متن);
                } وإلا {
                    ofp()->putsNoTracking(متن);
                }
            } وإلا {
                iterateConst(nodep);
            }
            إذا (nodep->nextp()) puts(separator);
        }
    }

    فراغ putConstructorSubinit(ثابت AstClass* classp, AstCFunc* cfuncp) {
        // Direct non-virtual bases in declaration order
        لكل (ثابت AstClassExtends* extp = classp->extendsp(); extp;
             extp = VN_AS(extp->nextp(), ClassExtends)) {
            إذا (extp->classp()->isInterfaceClass()) استمر;
            إذا (extp->classp()->use_Virtual_عوامی()) استمر;
            puts(EmitCUtil::اگیتر_Name_Protect(extp->classp()));
            puts("::init");
            إذا (constructorNeedsProcess(extp->classp())) {
                puts("(vlProcess, vlSymsp");
            } وإلا {
                puts("(vlSymsp");
            }
            // Handle super.new() args for the concrete parent
            إذا (ثابت AstCNew* ثابت superNewCallp = getSuperNewCallRecursep(cfuncp->stmtsp())) {
                put_Comma_Iterate_اگلا(superNewCallp->argsp(), true);
            }
            puts(");\n");
            // Since SystemVerilog allows only for extending from one class, for sure there won't
            // be any more constructors to be called. Even if there are some classes which are
            // "implemented", those are interface classes which do not have constructors.
            اكسر;
        }
    }

    // VISITORS
    using EmitCConstInit::ویکھو;
    فراغ ویکھو(AstCFunc* ثابت nodep) override {
        إذا (nodep->خالی_تھاواں_Body() && !nodep->isLoose()) أرجع;
        VL_RESTORER(m_useSelfForThis);
        VL_RESTORER(m_cfuncp);
        VL_RESTORER(m_instantiatesOwnProcess);
        VL_RESTORER(m_createdScopeHash);
        m_cfuncp = nodep;
        m_instantiatesOwnProcess = false;
        m_labelNumbers.clear();  // No need to save/restore, all Jumps must be within the function

        ونڈو_ناپ_Inc(nodep);

        puts("\n");
        m_lazyDecls.emit(nodep);
        إذا (nodep->ifdef() != "") putns(nodep, "#ifdef " + nodep->ifdef() + "\n");
        emit_CFunc_سرخی(nodep, m_modp, /* withScope: */ true);

        puts(" {\n");
        إذا (nodep->isConstructor()) {
            ثابت AstClass* ثابت classp = VN_CAST(nodep->scopep()->modp(), Class);
            إذا (classp && classp->extendsp()) putConstructorSubinit(classp, nodep);
        }

        // "+" in the debug indicates a print from the model
        puts("VL_DEBUG_IF(VL_DBG_MSGF(\"+  ");
        لكل (صحيح i = 0; i < m_modp->level(); ++i) puts("  ");
        puts(EmitCUtil::اگیتر_Name_Protect(m_modp));
        puts(nodep->isLoose() ? "__" : "::");
        puts(nodep->nameProtect() + "\\n\"); );\n");

        إذا (nodep->isLoose()) {
            m_lazyDecls.declared(nodep);  // Defined here, so no longer needs declaration
            إذا (!nodep->isStatic()) {  // Standard prologue
                m_useSelfForThis = true;
                إذا (nodep->voidSelf()) {
                    ثابت string className = EmitCUtil::اگیتر_Name_Protect(m_modp);
                    puts(className + "* const vlSelf = static_cast<" + className
                         + "*>(vlVoidSelf);\n");
                }
                إذا (!VN_IS(m_modp, Class)) {
                    puts(EmitCUtil::sym_Class_سونپو());  // Uses vlSelf
                } وإلا {
                    puts("(void)vlSelf;  // Prevent unused variable warning\n");
                }
            }
        }

        // Instantiate a process class if it's going to be needed somewhere later
        nodep->forall([&](ثابت Ast_گرہ_CCall* ccallp) -> bool {
            إذا (ccallp->funcp()->needProcess()
                && (ccallp->funcp()->isCoroutine() == VN_IS(ccallp->backp(), CAwait))) {
                إذا (!nodep->needProcess() && !m_instantiatesOwnProcess) {
                    m_instantiatesOwnProcess = true;
                    أرجع false;
                }
            }
            أرجع true;
        });
        إذا (m_instantiatesOwnProcess) {
            AstCStmt* ثابت vlprocp = new AstCStmt{nodep->fileline()};
            vlprocp->جوڑو("VlProcessRef vlProcess = std::make_shared<VlProcess>();\n");
            vlprocp->جوڑو("VlProcess::currentp(vlProcess.get());");
            nodep->prependStmtsp(vlprocp);
        } وإلا إذا (nodep->needProcess() && nodep->stmtsp()) {
            // Set current process so VlRNG() constructors in this function seed from it
            AstCStmt* ثابت سیٹ_Processp = new AstCStmt{nodep->fileline()};
            سیٹ_Processp->جوڑو("VlProcess::currentp(vlProcess.get());");
            nodep->prependStmtsp(سیٹ_Processp);
        }

        لكل (Ast_گرہ* subnodep = nodep->argsp(); subnodep; subnodep = subnodep->nextp()) {
            إذا (AstVar* ثابت varp = VN_CAST(subnodep, Var)) {
                إذا (varp->isFuncReturn()) emitVarDecl(varp);
            }
        }

        إذا (m_useSelfForThis) {
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

        إذا (nodep->varsp()) {
            putsDecoration(nodep, "// Locals\n");
            iterate_And_اگلا_Const_Null(nodep->varsp());
        }

        إذا (nodep->stmtsp()) {
            putsDecoration(nodep, "// Body\n");
            iterate_And_اگلا_Const_Null(nodep->stmtsp());
        }

        m_usevlSelfRef = false;

        إذا (nodep->isCoroutine()) {
            // Sometimes coroutines don't have co_awaits,
            // so emit a co_return at the end to avoid compile errors.
            puts("co_return;\n");
        }

        puts("}\n");
        إذا (nodep->ifdef() != "") puts("#endif  // " + nodep->ifdef() + "\n");
    }

    فراغ ویکھو(AstVar* nodep) override {
        UASSERT_OBJ(m_cfuncp, nodep, "Cannot emit non-local variable");
        emitVarDecl(nodep);
    }

    فراغ ویکھو(Ast_Cvt_Array_منزل_Array* nodep) override {
        إذا (nodep->الٹ()) {
            puts("VL_REVCLONE_Q(");
        } وإلا {
            puts("VL_CLONE_Q(");
        }
        iterate_And_اگلا_Const_Null(nodep->fromp());
        puts(", ");
        puts(cvt_منزل_Str(nodep->بلاک_ناپ()));
        puts(", ");
        puts(cvt_منزل_Str(nodep->src_Element_بٹ()));
        puts(", ");
        puts(cvt_منزل_Str(nodep->dst_Element_بٹ()));
        puts(")");
    }

    فراغ ویکھو(Ast_Cvt_Array_منزل_Packed* nodep) override {
        Ast_گرہ_DType* ثابت fromDtp = nodep->fromp()->dtypep()->skipRefp();
        Ast_گرہ_DType* ثابت elemDtp = fromDtp->subDTypep()->skipRefp();
        ثابت bool descending = [&]() {
            إذا (ثابت AstUnpackArrayDType* ثابت unpackDtp = VN_CAST(fromDtp, UnpackArrayDType))
                أرجع !unpackDtp->declRange().ascending();
            أرجع false;
        }();
        puts("VL_PACK_");
        emitIQW(nodep);
        puts("_");
        emitRU(fromDtp);
        emitIQW(elemDtp);
        إذا (descending) {
            // Wrap source in VL_PACK_REVERSED so VL_PACK sees ascending order
            emitOpName(nodep, "(%nw, %rw, %P, VL_PACK_REVERSED(%li))", nodep->fromp(), elemDtp,
                       nullptr);
        } وإلا {
            emitOpName(nodep, "(%nw, %rw, %P, %li)", nodep->fromp(), elemDtp, nullptr);
        }
    }

    فراغ ویکھو(Ast_Cvt_Unpack_منزل_قطار* nodep) override {
        Ast_گرہ_DType* ثابت elemDTypep = nodep->fromp()->dtypep()->subDTypep();
        emitOpName(nodep, nodep->emitC(), nodep->fromp(), elemDTypep, nullptr);
    }
    فراغ ویکھو(Ast_Cvt_Unpack_منزل_Unpack* nodep) override {
        // Whether each dimension is reversed, the first for the outermost, up to the last that is
        std::سمتیہ<bool> reverses;
        ثابت AstUnpackArrayDType* چوٹی = VN_AS(nodep->dtypep()->skipRefp(), UnpackArrayDType);
        ثابت AstUnpackArrayDType* fromp
            = VN_AS(nodep->fromp()->dtypep()->skipRefp(), UnpackArrayDType);
        طالما (چوٹی) {
            reverses.ڈالو_پٹھ(چوٹی->declRange().ascending() != fromp->declRange().ascending());
            چوٹی = VN_CAST(چوٹی->subDTypep()->skipRefp(), UnpackArrayDType);
            fromp = VN_CAST(fromp->subDTypep()->skipRefp(), UnpackArrayDType);
        }
        طالما (!reverses.پٹھ()) reverses.نکالو_پٹھ();
        putns(nodep, "VL_CVT_UNPACK_TO_UNPACK<");
        لكل (ناپ_t i = 0; i < reverses.ناپ(); ++i) {
            إذا (i) puts(", ");
            puts(reverses[i] ? "1" : "0");
        }
        puts(">(");
        iterate_And_اگلا_Const_Null(nodep->fromp());
        puts(")");
    }

    فراغ ویکھو(Ast_گرہ_سونپو* nodep) override {
        إذا (AstCReset* ثابت resetp = VN_CAST(nodep->rhsp(), CReset)) {
            // TODO get rid of emitVarReset and instead let AstNodeAssign understand how to init
            // anything
            Ast_گرہ* fromp = nodep->lhsp();
            // Fork needs to use a member select.  Nothing else should be possible before VarRef.
            إذا (AstMemberSel* ثابت sfromp = VN_CAST(fromp, MemberSel)) {
                // Fork-DynScope generated pointer to previously automatic variable
                AstVar* ثابت memberVarp = sfromp->varp();
                fromp = sfromp->fromp();
                إذا (AstNullCheck* ثابت sfromp = VN_CAST(fromp, NullCheck)) fromp = sfromp->lhsp();
                Ast_گرہ_Var_Ref* ثابت fromVarRefp = VN_AS(fromp, گرہ_Var_Ref);
                emitVarReset(
                    ("VL_NULL_CHECK("s
                     + (fromVarRefp->self_اشارہ_گر().is_خالی_تھاواں()
                            ? ""
                            : dereferenceString(fromVarRefp->self_اشارہ_گر_Protect(m_useSelfForThis)))
                     + varName(fromVarRefp->varp()) + ", \""
                     + V3_باہرلا_Formatter::quoteNameControls(protect(nodep->fileline()->filename()))
                     + "\", " + std::منزل_string(nodep->fileline()->lineno()) + ")->"),
                    memberVarp, resetp->constructing());
            } وإلا {
                Ast_گرہ_Var_Ref* ثابت fromVarRefp = VN_AS(fromp, گرہ_Var_Ref);
                AstVar* ثابت varp = fromVarRefp->varp();
                ثابت string اگیتر
                    = fromVarRefp->self_اشارہ_گر().is_خالی_تھاواں()
                          ? ""
                          : dereferenceString(
                                VN_AS(fromp, گرہ_Var_Ref)->self_اشارہ_گر_Protect(m_useSelfForThis));
                emitVarReset(اگیتر, varp, resetp->constructing());
            }
            أرجع;
        }
        bool paren = true;
        bool decind = false;
        bool rhs = true;
        bool الٹ_Unpack = false;  // Set for descending CvtPackedToArray
        ثابت AstUnpackArrayDType* ثابت unpackDtp
            = VN_CAST(nodep->dtypep()->skipRefp(), UnpackArrayDType);
        إذا (AstSel* ثابت selp = VN_CAST(nodep->lhsp(), Sel)) {
            UASSERT_OBJ(selp->چوڑائی_Min() == selp->چوڑائی_Const(), selp, "Width mismatch");
            إذا (selp->چوڑائی_Min() == 1) {
                putnbs(nodep, "VL_ASSIGNBIT_");
                emitIQW(selp->fromp());
                إذا (nodep->rhsp()->isAllOnesV()) {
                    puts("O(");
                    rhs = false;
                } وإلا {
                    puts("I(");
                }
                iterate_And_اگلا_Const_Null(selp->lsbp());
                puts(", ");
                iterate_And_اگلا_Const_Null(selp->fromp());
                إذا (rhs) puts(", ");
            } وإلا {
                putnbs(nodep, "VL_ASSIGNSEL_");
                emitIQW(selp->fromp());
                emitIQW(nodep->rhsp());
                puts("(");
                putns(selp->fromp(), cvt_منزل_Str(selp->fromp()->چوڑائی_Min()) + ", ");
                puts(cvt_منزل_Str(nodep->چوڑائی_Min()) + ", ");
                iterate_And_اگلا_Const_Null(selp->lsbp());
                puts(", ");
                iterate_And_اگلا_Const_Null(selp->fromp());
                puts(", ");
            }
        } وإلا إذا (ثابت AstGetcRefN* ثابت selp = VN_CAST(nodep->lhsp(), GetcRefN)) {
            iterate_And_اگلا_Const_Null(selp->lhsp());
            puts(" = ");
            putnbs(selp, "VL_PUTC_N(");
            iterate_And_اگلا_Const_Null(selp->lhsp());
            puts(", ");
            iterate_And_اگلا_Const_Null(selp->rhsp());
            puts(", ");
        } وإلا إذا (AstVar* ثابت varp = AstVar::scVarRecurse(nodep->lhsp())) {
            putnbs(varp, "VL_ASSIGN_");  // Set a systemC variable
            emitScIQW(varp);
            emitIQW(nodep);
            puts("(");
            puts(cvt_منزل_Str(nodep->چوڑائی_Min()) + ", ");
            iterate_And_اگلا_Const_Null(nodep->lhsp());
            puts(", ");
        } وإلا إذا (AstVar* ثابت varp = AstVar::scVarRecurse(nodep->rhsp())) {
            putnbs(varp, "VL_ASSIGN_");  // Get a systemC variable
            emitIQW(nodep);
            emitScIQW(varp);
            puts("(");
            puts(cvt_منزل_Str(nodep->چوڑائی_Min()) + ", ");
            iterate_And_اگلا_Const_Null(nodep->lhsp());
            puts(", ");
        } وإلا إذا (ثابت Ast_Cvt_Packed_منزل_Array* ثابت castp
                   = VN_CAST(nodep->rhsp(), Cvt_Packed_منزل_Array)) {
            putns(castp, "VL_UNPACK_");
            emitRU(nodep);
            emitIQW(nodep->dtypep()->subDTypep());
            puts("_");
            emitRU(castp->fromp());
            emitIQW(castp->fromp());
            puts("(");
            putns(castp->dtypep(), cvt_منزل_Str(castp->dtypep()->subDTypep()->چوڑائی_Min()));
            puts(", ");
            puts(cvt_منزل_Str(castp->fromp()->چوڑائی_Min()));
            puts(", ");
            iterate_And_اگلا_Const_Null(nodep->lhsp());
            puts(", ");
            rhs = false;
            iterate_And_اگلا_Const_Null(castp->fromp());
            // Descending unpacked dest: reverse after unpack
            إذا (unpackDtp && !unpackDtp->declRange().ascending()) الٹ_Unpack = true;
        } وإلا إذا (ثابت Ast_Cvt_Array_منزل_Array* ثابت castp
                   = VN_CAST(nodep->rhsp(), Cvt_Array_منزل_Array)) {
            إذا (castp->الٹ()) {
                putns(castp, "VL_REVCOPY_Q(");
            } وإلا {
                putns(castp, "VL_COPY_Q(");
            }
            iterate_And_اگلا_Const_Null(nodep->lhsp());
            puts(", ");
            rhs = false;
            iterate_And_اگلا_Const_Null(castp->fromp());
            puts(", ");
            puts(cvt_منزل_Str(castp->بلاک_ناپ()));
            puts(", ");
            puts(cvt_منزل_Str(castp->src_Element_بٹ()));
            puts(", ");
            puts(cvt_منزل_Str(castp->dst_Element_بٹ()));
        } وإلا إذا (nodep->isWide() && VN_IS(nodep->lhsp(), VarRef)  //
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
                   && !VN_IS(nodep->rhsp(), اظہاریہ_Stmt)) {
            // Wide functions assign into the array directly, don't need separate assign statement
            m_wide_عارضی_Refp = VN_AS(nodep->lhsp(), VarRef);
            paren = false;
        } وإلا إذا (nodep->isWide() && !unpackDtp && !VN_IS(nodep->rhsp(), Const)) {
            putnbs(nodep, "VL_ASSIGN_W(");
            puts(cvt_منزل_Str(nodep->چوڑائی_Min()) + ", ");
            iterate_And_اگلا_Const_Null(nodep->lhsp());
            puts(", ");
        } وإلا إذا (VN_IS(nodep->lhsp()->dtypep()->skipRefp(), قطار_DType)
                   && (VN_IS(nodep->rhsp(), دھارا_L) || VN_IS(nodep->lhsp(), دھارا_L)
                       || VN_IS(nodep->rhsp(), دھارا_R) || VN_IS(nodep->lhsp(), دھارا_R)
                       || VN_IS(nodep->rhsp(), دھارا_R))) {
            //if either side is streamL or streamR don't emit lhsp everything will be passed by
            //reference

            paren = false;

        } وإلا {
            paren = false;
            iterate_And_اگلا_Const_Null(nodep->lhsp());
            puts(" ");
            ofp()->بلاک_Inc();
            decind = true;
            إذا (!VN_IS(nodep->rhsp(), Const)) ofp()->putBreak();
            putns(nodep, "= ");
        }
        إذا (rhs) iterate_And_اگلا_Const_Null(nodep->rhsp());
        إذا (paren) puts(")");
        إذا (decind) ofp()->بلاک_Dec();
        puts(";\n");
        إذا (الٹ_Unpack) {
            puts("VL_UNPACK_REVERSED(");
            iterate_And_اگلا_Const_Null(nodep->lhsp());
            puts(");\n");
        }
    }
    فراغ ویکھو(AstAssocSel* nodep) override {
        iterate_And_اگلا_Const_Null(nodep->fromp());
        ثابت std::string atFunc = nodep->isLValue() ? ".atWrite(" : ".at(";
        putnbs(nodep, atFunc);
        AstAssocArrayDType* ثابت adtypep
            = VN_AS(nodep->fromp()->dtypep()->skipRefp(), AssocArrayDType);
        UASSERT_OBJ(adtypep, nodep, "Associative select on non-associative type");
        iterate_And_اگلا_Const_Null(nodep->bitp());
        puts(")");
    }
    فراغ ویکھو(AstWildcardSel* nodep) override {
        iterate_And_اگلا_Const_Null(nodep->fromp());
        ثابت std::string atFunc = nodep->isLValue() ? ".atWrite(" : ".at(";
        putnbs(nodep, atFunc);
        AstWildcardArrayDType* ثابت adtypep
            = VN_AS(nodep->fromp()->dtypep()->skipRefp(), WildcardArrayDType);
        UASSERT_OBJ(adtypep, nodep, "Wildcard select on non-wildcard-associative type");
        iterate_And_اگلا_Const_Null(nodep->bitp());
        puts(")");
    }
    فراغ ویکھو(AstCCall* nodep) override {
        ثابت AstCFunc* ثابت funcp = nodep->funcp();
        ثابت Ast_گرہ_Module* ثابت funcModp = EmitCParentModule::get(funcp);
        putnbs(nodep, "");
        إذا (funcp->dpiImportPrototype()) {
            // Calling DPI import
            putns(nodep, funcp->name());
        } وإلا إذا (funcp->is_Proper_طریقہ() && funcp->isStatic()) {
            // Call static method via the containing class
            putns(funcModp, EmitCUtil::اگیتر_Name_Protect(funcModp) + "::");
            putns(nodep, funcp->nameProtect());
        } وإلا إذا (nodep->superReference()) {
            // Calling superclass method
            putns(funcModp, EmitCUtil::اگیتر_Name_Protect(funcModp) + "::");
            putns(nodep, funcp->nameProtect());
        } وإلا إذا (funcp->isLoose()) {
            // Calling loose method
            putns(nodep, funcNameProtect(funcp));
        } وإلا {
            // Calling regular method/function
            إذا (!nodep->self_اشارہ_گر().is_خالی_تھاواں()) {
                emitDereference(nodep, nodep->self_اشارہ_گر_Protect(m_useSelfForThis));
            }
            putns(nodep, funcp->nameProtect());
        }
        emit_CCall_دلیلاں(nodep, nodep->self_اشارہ_گر_Protect(m_useSelfForThis), m_cfuncp->needProcess());
    }
    فراغ ویکھو(AstCMethodCall* nodep) override {
        ثابت AstCFunc* ثابت funcp = nodep->funcp();
        UASSERT_OBJ(!funcp->isLoose(), nodep, "Loose method called via AstCMethodCall");
        iterateConst(nodep->fromp());
        putnbs(nodep, "->");
        إذا (nodep->superReference()) {
            // Super call, so the base implementation is used rather than the override
            ثابت Ast_گرہ_Module* ثابت funcModp = EmitCParentModule::get(funcp);
            putns(funcModp, EmitCUtil::اگیتر_Name_Protect(funcModp) + "::");
        }
        putns(funcp, funcp->nameProtect());
        emit_CCall_دلیلاں(nodep, "", m_cfuncp->needProcess());
    }
    فراغ ویکھو(AstCAwait* nodep) override {
        putns(nodep, "co_await ");
        iterateConst(nodep->exprp());
        puts(";\n");
    }
    فراغ ویکھو(AstCNew* nodep) override {
        إذا (VN_IS(nodep->dtypep(), VoidDType)) {  // super.new case
            putsDecoration(nodep, "/*super.new*/");
            أرجع;
        }
        // assignment case;
        putns(nodep, "VL_NEW(" + EmitCUtil::اگیتر_Name_Protect(nodep->dtypep()) + ", "
                         + optionalProcArg(nodep->dtypep()) + "vlSymsp");
        put_Comma_Iterate_اگلا(nodep->argsp(), true);
        puts(")");
    }
    فراغ ویکھو(AstCMethodHard* nodep) override {
        iterateConst(nodep->fromp());
        putns(nodep, nodep->usePtr() ? "->" : ".");
        putns(nodep, nodep->name());
        إذا (nodep->طریقہ() == VCMethod::FORCE_READ_SEL) {
            emitIQW(nodep);
            إذا (nodep->isWide()) puts("<" + cvt_منزل_Str(nodep->dtypep()->چوڑائی_الفاظ()) + ">");
        } وإلا إذا (nodep->طریقہ() == VCMethod::ARRAY_ٹکڑا) {
            // VlUnpacked::slice<N_Out>(loIdx) - N_Out is the (fixed) result array size
            ثابت AstUnpackArrayDType* ثابت adtypep
                = VN_AS(nodep->dtypep()->skipRefp(), UnpackArrayDType);
            puts("<" + cvt_منزل_Str(adtypep->elementsConst()) + ">");
        } وإلا إذا (nodep->طریقہ() == VCMethod::COVERGROUP_جوڑو_COVERPOINT) {
            // The hit-list bound is a template argument of the returned VlCoverpointT<>, and the
            // node's own dtype is that type, so it is the one source of truth for both.
            ثابت AstCoverpointDType* ثابت cpdtypep
                = VN_AS(nodep->dtypep()->skipRefp(), CoverpointDType);
            puts("<" + cvt_منزل_Str(cpdtypep->کامیابی_Bound()) + ">");
        } وإلا إذا (nodep->طریقہ() == VCMethod::COVERGROUP_جوڑو_CROSS) {
            ثابت AstCoverCrossDType* ثابت cxdtypep
                = VN_AS(nodep->dtypep()->skipRefp(), CoverCrossDType);
            puts("<" + cxdtypep->cpp_Template_دلیلاں() + ">");
        }
        puts("(");
        bool comma = false;
        صحيح arg_عدد = 0;
        لكل (Ast_گرہ* subnodep = nodep->pinsp(); subnodep; subnodep = subnodep->nextp()) {
            إذا (comma) puts(", ");
            إذا (nodep->طریقہ() == VCMethod::RANDOMIZER_HARD && arg_عدد == 1) {
                // For RANDOMIZER_HARD's filename argument (2nd arg after constraint),
                // apply protect() similar to VL_STOP to handle --protected flag
                إذا (ثابت AstCExpr* ثابت cexprp = VN_CAST(subnodep, CExpr)) {
                    // Extract filename from the CExpr (which contains "filename")
                    std::string filename;
                    لكل (ثابت Ast_گرہ* textnodep = cexprp->nodesp(); textnodep;
                         textnodep = textnodep->nextp()) {
                        إذا (ثابت Ast_متن* ثابت textp = VN_CAST(textnodep, متن_2)) {
                            filename = textp->متن();
                            اكسر;
                        }
                    }
                    // Remove surrounding quotes if present
                    إذا (filename.ناپ() >= 2 && filename.front() == '"'
                        && filename.پٹھ() == '"') {
                        filename = filename.substr(1, filename.ناپ() - 2);
                    }
                    // Emit with protect()
                    putsQuoted(protect(filename));
                } وإلا {
                    iterateConst(subnodep);
                }
            } وإلا {
                iterateConst(subnodep);
            }
            comma = true;
            arg_عدد++;
        }
        إذا (nodep->withp()) {
            إذا (comma) puts(", ");
            iterateConst(nodep->withp());
        }
        puts(")");
    }
    فراغ ویکھو(AstCFuncHard* nodep) override {
        putns(nodep, nodep->name());
        puts("(");
        put_Comma_Iterate_اگلا(nodep->pinsp());
        puts(")");
    }
    فراغ ویکھو(AstLambdaArgRef* nodep) override { putbs(nodep->nameProtect()); }
    فراغ ویکھو(AstWith* nodep) override {
        // With uses a C++11 lambda
        putnbs(nodep, "[&](");
        إذا (خودکار* ثابت argrefp = nodep->اشاریہ_Arg_Refp()) {
            putnbs(argrefp, argrefp->dtypep()->cType(argrefp->nameProtect(), false, false));
            puts(", ");
        }
        إذا (خودکار* ثابت argrefp = nodep->قدر_Arg_Refp()) {
            putnbs(argrefp, argrefp->dtypep()->cType(argrefp->nameProtect(), false, false));
        }
        puts(") -> ");
        putnbs(nodep, nodep->dtypep()->cType("", false, false));
        puts(" {\n");
        VL_RESTORER(m_createdScopeHash);
        iterate_And_اگلا_Const_Null(nodep->exprp());
        puts("}\n");
    }
    فراغ ویکھو(AstCase* nodep) override {  // LCOV_EXCL_LINE
        // In V3Case...
        nodep->v3fatalSrc("Case statements should have been reduced out");
    }
    فراغ ویکھو(AstComment* nodep) override {
        string at;
        إذا (nodep->showAt()) {
            at = " at " + nodep->fileline()->ascii();
            // If protecting, passthru less information about the design
            إذا (!v3Global.opt.protectIds()) أرجع;
        }
        إذا (!(nodep->protect() && v3Global.opt.protectIds())) {
            putsDecoration(nodep, "// "s + nodep->name() + at + "\n");
        }
        iterateChildrenConst(nodep);
    }
    فراغ ویکھو(AstCoverOtherDecl* nodep) override {
        emit_Cover_Other_Decl_داخل_کرو(nodep);
        إذا (coverage_ورتدا_Local_Counter(nodep)) {
            // Some task bodies are cloned into a caller module where the
            // original instance-local counter is not addressable. Those clones
            // fall back to the global bin number, so register that slot too.
            emit_Cover_Other_Decl_داخل_کرو(nodep, true);
        }
    }
    فراغ ویکھو(AstCoverToggleDecl* nodep) override {
        putns(nodep, "vlSelf->__vlCoverToggleInsert(");  // As Declared in emitCoverageDecl
        puts(cvt_منزل_Str(nodep->range().دایاں()));
        puts(", ");
        puts(cvt_منزل_Str(nodep->range().بایاں()));
        puts(", ");
        puts(cvt_منزل_Str(nodep->range().ranged()));
        puts(", ");
        putCoverageArray(nodep);
        puts(" + ");
        puts(cvt_منزل_Str(coverage_Bin_عدد(nodep)));
        // first controls whether duplicate module instances are collapsed in
        // the default coverage view. The following flag says whether countp
        // points to an object-local counter; only those counters must be kept
        // real for later instances so forcePerInstance can split hierarchy.
        puts(", first");  // Enable, passed from __Vconfigure parameter
        puts(", ");
        puts(coverage_ورتدا_Local_Counter(nodep) ? "true" : "false");
        puts(", ");
        putsQuoted(protect(nodep->fileline()->filename()));
        puts(", ");
        puts(cvt_منزل_Str(nodep->fileline()->lineno()));
        puts(", ");
        puts(cvt_منزل_Str(nodep->fileline()->پہلا_کالم()));
        puts(", ");
        putsQuoted((!nodep->hier().خالی_تھاواں() ? "." : "")
                   + VIdProtect::protect_الفاظ_If(nodep->hier(), nodep->protect()));
        puts(", ");
        putsQuoted(VIdProtect::protect_الفاظ_If(nodep->ورقہ(), nodep->protect()));
        puts(", ");
        putsQuoted(VIdProtect::protect_الفاظ_If(nodep->comment(), nodep->protect()));
        puts(");\n");
    }
    فراغ ویکھو(AstCoverInc* nodep) override {
        إذا (VN_IS(nodep->declp(), CoverOtherDecl)) {
            إذا (v3Global.opt.دھاگے() > 1) {
                putns(nodep, "");
                putCoverageArray(nodep->declp());
                puts("[");
                puts(cvt_منزل_Str(coverage_Bin_عدد(nodep->declp())));
                puts("].fetch_add(1, std::memory_order_relaxed);\n");
            } وإلا {
                putns(nodep, "++(");
                putCoverageArray(nodep->declp());
                puts("[");
                puts(cvt_منزل_Str(coverage_Bin_عدد(nodep->declp())));
                puts("]);\n");
            }
        } وإلا {
            puts("VL_COV_TOGGLE_CHG_");
            إذا (v3Global.opt.دھاگے() > 1) {
                puts("MT_");
            } وإلا {
                puts("ST_");
            }
            emitIQW(nodep->toggleExprp());
            puts("(");
            // Each bit of variable has 2 counters, so the size of variable is half of the size of
            // coverpoint
            puts(cvt_منزل_Str(nodep->declp()->ناپ() / 2));
            puts(", ");
            // Toggle update uses the same object-local counter array that
            // __vlCoverToggleInsert registered.
            putCoverageArray(nodep->declp());
            puts(" + ");
            puts(cvt_منزل_Str(coverage_Bin_عدد(nodep->declp())));
            puts(", ");
            iterateConst(nodep->toggleExprp());
            puts(", ");
            iterateConst(nodep->toggleCovExprp());
            puts(");\n");
        }
    }
    فراغ ویکھو(AstDisableFork* nodep) override { putns(nodep, "vlProcess->disableFork();\n"); }
    فراغ ویکھو(AstCReturn* nodep) override {
        putns(nodep, "return (");
        iterate_And_اگلا_Const_Null(nodep->lhsp());
        puts(");\n");
    }
    فراغ ویکھو(AstDisplay* nodep) override {
        string متن = nodep->fmtp()->متن();
        إذا (nodep->جوڑو_Newline()) متن += "\n";
        display_گرہ(nodep, nodep->fmtp(), متن, nodep->fmtp()->exprsp(), false);
    }
    فراغ ویکھو(AstDumpCtl* nodep) override {
        بدل (nodep->ctlType()) {
        حالة VDumpCtlType::FILE:
            putns(nodep, "vlSymsp->_vm_contextp__->dumpfile(");
            iterateConst(nodep->exprp());
            puts(");\n");
            اكسر;
        حالة VDumpCtlType::VARS:
            // We ignore number of levels to dump in exprp()
            إذا (v3Global.opt.trace()) {
                putns(nodep, "vlSymsp->_traceDumpOpen();\n");
            } وإلا {
                putns(nodep, "VL_PRINTF_MT(\"-Info: ");
                puts(V3_باہرلا_Formatter::quoteNameControls(protect(nodep->fileline()->filename())));
                puts(":");
                puts(cvt_منزل_Str(nodep->fileline()->lineno()));
                puts(": $dumpvar ignored, as Verilated without --trace");
                puts("\\n\");\n");
            }
            اكسر;
        حالة VDumpCtlType::ALL:
            // $dumpall currently ignored
            اكسر;
        حالة VDumpCtlType::FLUSH:
            // $dumpall currently ignored; would need rework of VCD single thread,
            // or flag we pass-through to next eval() iteration
            اكسر;
        حالة VDumpCtlType::حد_بندی_2:
            // $dumplimit currently ignored
            اكسر;
        حالة VDumpCtlType::OFF:
            // Currently ignored as both Vcd and Fst do not support them, as would need "X" dump
            اكسر;
        حالة VDumpCtlType::ON:
            // Currently ignored as $dumpoff is also ignored
            اكسر;
        افتراضي: nodep->v3fatalSrc("Bad case, unexpected " << nodep->ctlType().ascii());
        }
    }
    فراغ ویکھو(AstScopeName* nodep) override {
        // For use under AstCCalls for dpiImports.  ScopeNames under
        // displays are handled in AstDisplay
        إذا (!nodep->dpiExport()) {
            // this is where the DPI import context scope is set
            ثابت string scope = nodep->scopeDpiName();
            putnbs(nodep, "(vlSymsp->" + protect("__Vscopep_" + scope) + ")");
        }
    }
    فراغ ویکھو(AstSFormat* nodep) override {
        display_گرہ(nodep, nodep->fmtp(), nodep->fmtp()->متن(), nodep->fmtp()->exprsp(), false);
    }
    فراغ ویکھو(AstSFormatF* nodep) override {
        display_گرہ(nodep, nodep, nodep->متن(), nodep->exprsp(), false);
    }
    فراغ ویکھو(AstFScanF* nodep) override {
        display_گرہ(nodep, nullptr, nodep->متن(), nodep->exprsp(), true);
    }
    فراغ ویکھو(AstSScanF* nodep) override {
        display_گرہ(nodep, nullptr, nodep->متن(), nodep->exprsp(), true);
    }
    فراغ ویکھو(Ast_قدر_Plus_دلیلاں* nodep) override {
        putns(nodep, "VL_VALUEPLUSARGS_IN");
        emitIQW(nodep->outp());
        puts("(");
        puts(cvt_منزل_Str(nodep->outp()->چوڑائی_Min()));
        puts(", ");
        iterateConst(nodep->searchp());
        puts(", ");
        putbs("");
        iterate_And_اگلا_Const_Null(nodep->outp());
        puts(")");
    }
    فراغ ویکھو(Ast_Test_Plus_دلیلاں* nodep) override {
        putns(nodep, "VL_TESTPLUSARGS_I(");
        iterateConst(nodep->searchp());
        puts(")");
    }
    فراغ ویکھو(AstFError* nodep) override {
        putns(nodep, "VL_FERROR_I");
        puts(nodep->strp()->isString() ? "N(" : "W(");
        iterate_And_اگلا_Const_Null(nodep->filep());
        putbs(", ");
        إذا (nodep->strp()->isWide()) {
            puts(cvt_منزل_Str(nodep->strp()->چوڑائی_الفاظ()));
            putbs(", ");
        }
        iterate_And_اگلا_Const_Null(nodep->strp());
        puts(")");
    }
    فراغ ویکھو(AstFGetS* nodep) override {
        emitOpName(nodep, nodep->emitC(), nodep->strgp(), nodep->filep(), nullptr);
    }
    فراغ ویکھو(AstFOpen* nodep) override {
        putns(nodep, "VL_FOPEN_NN(");
        iterateConst(nodep->filenamep());
        putbs(", ");
        iterateConst(nodep->modep());
        puts(");\n");
    }
    فراغ ویکھو(AstFOpenMcd* nodep) override {
        putns(nodep, "VL_FOPEN_MCD_N(");
        iterateConst(nodep->filenamep());
        puts(");\n");
    }
    فراغ ویکھو(Ast_گرہ_Read_Write_Mem* nodep) override {
        putns(nodep, nodep->cFuncPrefixp());
        puts("N(");
        puts(nodep->isHex() ? "true" : "false");
        putbs(", ");
        // Need real storage width
        puts(cvt_منزل_Str(nodep->memp()->dtypep()->skipRefp()->subDTypep()->چوڑائی_Min()));
        uint32_t array_lo = 0;
        bool memUsePtr = true;
        {
            ثابت AstVarRef* ثابت varrefp = VN_CAST(nodep->memp(), VarRef);
            إذا (!varrefp) {
                nodep->v3error(nodep->verilogKwd() << " loading non-variable");
            } وإلا إذا (VN_IS(varrefp->varp()->dtypeSkipRefp(), AssocArrayDType)
                       || VN_IS(varrefp->varp()->dtypeSkipRefp(), DynArrayDType)) {
                // nodep->memp() below will when verilated code is compiled create a C++ template
                memUsePtr = false;
            } وإلا إذا (ثابت AstUnpackArrayDType* ثابت adtypep
                       = VN_CAST(varrefp->varp()->dtypeSkipRefp(), UnpackArrayDType)) {
                putbs(", ");
                puts(cvt_منزل_Str(varrefp->varp()->dtypep()->arrayUnpackedElements()));
                array_lo = adtypep->lo();
                putbs(", ");
                puts(cvt_منزل_Str(array_lo));
            } وإلا {
                nodep->v3error(
                    nodep->verilogKwd()
                    << " loading other than unpacked/dynamic/associative-array variable");
            }
        }
        putbs(", ");
        iterateConst(nodep->filenamep());
        putbs(", ");
        {
            إذا (memUsePtr) puts(" &(");
            iterate_And_اگلا_Const_Null(nodep->memp());
            إذا (memUsePtr) puts(")");
        }
        putbs(", ");
        إذا (nodep->lsbp()) {
            iterate_And_اگلا_Const_Null(nodep->lsbp());
        } وإلا {
            puts(cvt_منزل_Str(array_lo));
        }
        putbs(", ");
        إذا (nodep->msbp()) {
            iterate_And_اگلا_Const_Null(nodep->msbp());
        } وإلا {
            puts("~0ULL");
        }
        puts(");\n");
    }
    فراغ ویکھو(AstFClose* nodep) override {
        putns(nodep, "VL_FCLOSE_I(");
        iterate_And_اگلا_Const_Null(nodep->filep());
        puts("); ");
    }
    فراغ ویکھو(AstFFlush* nodep) override {
        إذا (!nodep->filep()) {
            putns(nodep, "VL_FFLUSH_MT();");
        } وإلا {
            putns(nodep, "VL_FFLUSH_I(");
            iterate_And_اگلا_Const_Null(nodep->filep());
            puts(");\n");
        }
    }
    فراغ ویکھو(AstFSeek* nodep) override {
        putns(nodep, "(VL_FSEEK_I(");
        iterate_And_اگلا_Const_Null(nodep->filep());
        puts(", ");
        iterate_And_اگلا_Const_Null(nodep->وتھ());
        puts(", ");
        iterate_And_اگلا_Const_Null(nodep->operation());
        puts(") == -1 ? -1 : 0)");
    }
    فراغ ویکھو(AstFTell* nodep) override {
        putns(nodep, "VL_FTELL_I(");
        iterate_And_اگلا_Const_Null(nodep->filep());
        puts(")");
    }
    فراغ ویکھو(AstFRewind* nodep) override {
        putns(nodep, "(VL_FSEEK_I(");
        iterate_And_اگلا_Const_Null(nodep->filep());
        puts(", 0, 0) == -1 ? -1 : 0)");
    }
    فراغ ویکھو(AstFRead* nodep) override {
        putns(nodep, "VL_FREAD_I(");
        puts(cvt_منزل_Str(nodep->memp()->چوڑائی_Min()));  // Need real storage width
        putbs(", ");
        uint32_t array_lo = 0;
        uint32_t array_ناپ = 0;
        {
            ثابت AstVarRef* ثابت varrefp = VN_CAST(nodep->memp(), VarRef);
            إذا (!varrefp) {
                nodep->v3error(nodep->verilogKwd() << " loading non-variable");
            } وإلا إذا (VN_CAST(varrefp->varp()->dtypeSkipRefp(), BasicDType)) {
            } وإلا إذا (ثابت AstUnpackArrayDType* ثابت adtypep
                       = VN_CAST(varrefp->varp()->dtypeSkipRefp(), UnpackArrayDType)) {
                array_lo = adtypep->lo();
                array_ناپ = adtypep->elementsConst();
            } وإلا {
                nodep->v3error(nodep->verilogKwd()
                               << " loading other than unpacked-array variable");
            }
        }
        puts(cvt_منزل_Str(array_lo));
        putbs(", ");
        puts(cvt_منزل_Str(array_ناپ));
        putbs(", ");
        puts("&(");
        iterate_And_اگلا_Const_Null(nodep->memp());
        puts(")");
        putbs(", ");
        iterate_And_اگلا_Const_Null(nodep->filep());
        putbs(", ");
        إذا (nodep->startp()) {
            iterate_And_اگلا_Const_Null(nodep->startp());
        } وإلا {
            puts(cvt_منزل_Str(array_lo));
        }
        putbs(", ");
        إذا (nodep->countp()) {
            iterate_And_اگلا_Const_Null(nodep->countp());
        } وإلا {
            puts(cvt_منزل_Str(array_ناپ));
        }
        puts(")");
    }
    فراغ ویکھو(Ast_ڈھیر_Trace_F* nodep) override { putns(nodep, "VL_STACKTRACE_N()"); }
    فراغ ویکھو(Ast_ڈھیر_Trace_T* nodep) override { putns(nodep, "VL_STACKTRACE();\n"); }
    فراغ ویکھو(AstSystemT* nodep) override {
        putns(nodep, "(void)VL_SYSTEM_I");
        emitIQW(nodep->lhsp());
        puts("(");
        إذا (nodep->lhsp()->isWide()) {
            puts(cvt_منزل_Str(nodep->lhsp()->چوڑائی_الفاظ()));
            putbs(", ");
        }
        iterate_And_اگلا_Const_Null(nodep->lhsp());
        puts(");\n");
    }
    فراغ ویکھو(AstSystemF* nodep) override {
        putns(nodep, "VL_SYSTEM_I");
        emitIQW(nodep->lhsp());
        puts("(");
        إذا (nodep->lhsp()->isWide()) {
            puts(cvt_منزل_Str(nodep->lhsp()->چوڑائی_الفاظ()));
            putbs(", ");
        }
        iterate_And_اگلا_Const_Null(nodep->lhsp());
        puts(")");
    }
    فراغ ویکھو(Ast_Stmt_اظہاریہ* nodep) override {
        إذا (VN_IS(nodep->exprp()->dtypep()->skipRefp(), VoidDType)) {
            putns(nodep, "");
        } وإلا {
            putns(nodep, "std::ignore = ");
        }
        iterateConst(nodep->exprp());
        puts(";\n");
    }
    فراغ ویکھو(Ast_Jump_بلاک* nodep) override {
        // Allocate label number
        ثابت ناپ_t n = m_labelNumbers.ناپ();
        ثابت bool new_اندراج = m_labelNumbers.emplace(nodep, n).دوجا;
        UASSERT_OBJ(new_اندراج, nodep, "AstJumpBlock visited twide");
        // Emit
        putns(nodep, "{\n");  // Make it visually obvious label jumps outside these
        VL_RESTORER(m_createdScopeHash);
        iterate_And_اگلا_Const_Null(nodep->stmtsp());
        puts("__Vlabel" + std::منزل_string(n) + ": ;\n");
        puts("}\n");
    }
    فراغ ویکھو(AstJumpGo* nodep) override {
        // Retrieve target label number - must already exist (from enclosing AstJumpBlock)
        ثابت ناپ_t n = m_labelNumbers.at(nodep->blockp());
        // Emit
        putns(nodep, "goto __Vlabel" + std::منزل_string(n) + ";\n");
    }
    فراغ ویکھو(AstLoop* nodep) override {
        UASSERT_OBJ(!nodep->contsp(), nodep, "'contsp' only used before LinkJump");
        VL_RESTORER(m_createdScopeHash);
        // Special case when the AstLoopTest is first for output readability
        إذا (AstLoopTest* ثابت testp = VN_CAST(nodep->stmtsp(), LoopTest)) {
            putns(nodep, "while (");
            iterateConst(testp->condp());
            puts(") {\n");
            iterate_And_اگلا_Const_Null(testp->nextp());
            puts("}\n");
            أرجع;
        }
        // Special case when the AstLoopTest is last for output readability
        إذا (Ast_گرہ* lastp = nodep->stmtsp()) {
            طالما (Ast_گرہ* ثابت nextp = lastp->nextp()) lastp = nextp;
            إذا (AstLoopTest* ثابت testp = VN_CAST(lastp, LoopTest)) {
                putns(nodep, "do {\n");
                لكل (Ast_گرہ* p = nodep->stmtsp(); p != lastp; p = p->nextp()) iterateConst(p);
                puts("} while (");
                iterateConst(testp->condp());
                puts(");\n");
                أرجع;
            }
        }
        // Emit generic case directly
        putns(nodep, "while (true) {\n");
        iterate_And_اگلا_Const_Null(nodep->stmtsp());
        puts("}\n");
    }
    فراغ ویکھو(AstLoopTest* nodep) override {
        VL_RESTORER(m_createdScopeHash);
        putns(nodep, "if (!(");
        iterate_And_اگلا_Const_Null(nodep->condp());
        puts(")) break;\n");
    }
    فراغ ویکھو(AstCLocalScope* nodep) override {
        putns(nodep, "{\n");
        VL_RESTORER(m_createdScopeHash);
        iterate_And_اگلا_Const_Null(nodep->stmtsp());
        puts("}\n");
    }
    فراغ ویکھو(Ast_گرہ_If* nodep) override {
        putns(nodep, "if (");
        إذا (!nodep->branchPred().unknown()) {
            puts(nodep->branchPred().ascii());
            puts("((");  // Two parens, so that VL_UNLIKELY((class<foo,bar>)) works
        }
        iterate_And_اگلا_Const_Null(nodep->condp());
        إذا (!nodep->branchPred().unknown()) puts("))");
        puts(") {\n");
        {
            VL_RESTORER(m_createdScopeHash);
            iterate_And_اگلا_Const_Null(nodep->thensp());
        }
        puts("}");
        إذا (!nodep->elsesp()) {
            puts("\n");
        } وإلا {
            إذا (VN_IS(nodep->elsesp(), گرہ_If) && !nodep->elsesp()->nextp()) {
                puts(" else ");
                iterate_And_اگلا_Const_Null(nodep->elsesp());
            } وإلا {
                VL_RESTORER(m_createdScopeHash);
                puts(" else {\n");
                iterate_And_اگلا_Const_Null(nodep->elsesp());
                puts("}\n");
            }
        }
    }
    فراغ ویکھو(Ast_اظہاریہ_Stmt* nodep) override {
        VL_RESTORER(m_createdScopeHash);
        ثابت bool شامل_اے_Await = nodep->exists([](AstCAwait*) -> bool { أرجع true; });
        إذا (شامل_اے_Await) {
            UASSERT_OBJ(m_cfuncp && m_cfuncp->isCoroutine(), nodep,
                        "AstExprStmt with CAwait must be in coroutine");
            putnbs(nodep, "(co_await ([&]() -> VlCoroutine {\n");
            iterate_And_اگلا_Const_Null(nodep->stmtsp());
            puts("co_return;\n");
            إذا (!nodep->has_نتیجہ()) {
                puts("}()))");
                أرجع;
            }
            puts("}()), ");
            iterate_And_اگلا_Const_Null(nodep->resultp());
            puts(")");
            أرجع;
        }
        // GCC allows compound statements in expressions, but this is not standard.
        // So we use an immediate-evaluation lambda and comma operator
        putnbs(nodep, "([&]() {\n");
        إذا (!nodep->has_نتیجہ()) {
            iterate_And_اگلا_Const_Null(nodep->stmtsp());
            puts("}())");
            أرجع;
        }
        iterate_And_اگلا_Const_Null(nodep->stmtsp());
        puts("}(), ");
        iterate_And_اگلا_Const_Null(nodep->resultp());
        puts(")");
    }
    فراغ ویکھو(AstStop* nodep) override {
        putns(nodep, "VL_STOP_MT(");
        putsQuoted(protect(nodep->fileline()->filename()));
        puts(", ");
        puts(cvt_منزل_Str(nodep->fileline()->lineno()));
        puts(", \"\"");
        إذا (nodep->isFatal()) puts(", false");
        puts(");\n");
    }
    فراغ ویکھو(Ast_انت* nodep) override {
        // Disable all the forks so they don't operate after simulation is finished.
        إذا (m_cfuncp && m_cfuncp->needProcess()) putns(nodep, "vlProcess->disableFork();\n");
        putns(nodep, "VL_FINISH_MT(");
        putsQuoted(protect(nodep->fileline()->filename()));
        puts(", ");
        puts(cvt_منزل_Str(nodep->fileline()->lineno()));
        puts(", \"\");\n");
    }
    فراغ ویکھو(Ast_انت_Fork* nodep) override {
        // Disable all the forks so they don't operate after simulation is finished.
        إذا (m_cfuncp && m_cfuncp->needProcess()) putns(nodep, "vlProcess->disableFork();\n");
        putns(nodep, "VL_FINISH_MT(");
        putsQuoted(protect(nodep->fileline()->filename()));
        puts(", ");
        puts(cvt_منزل_Str(nodep->fileline()->lineno()));
        puts(", \"\");\n");
        إذا (m_cfuncp->isCoroutine()) {
            putns(nodep, "co_return;\n");
        } وإلا {
            putns(nodep, "return;\n");
        }
    }
    فراغ ویکھو(Ast_Print_ویلا_Scale* nodep) override {
        putns(nodep, "VL_PRINTTIMESCALE(");
        putsQuoted(protect(nodep->prettyName()));
        puts(", ");
        putsQuoted(nodep->timeunit().ascii());
        puts(", vlSymsp->_vm_contextp__);\n");
    }
    فراغ ویکھو(AstRand* nodep) override {
        emitOpName(nodep, nodep->emitC(), nodep->seedp(), nullptr, nullptr);
    }
    فراغ ویکھو(AstRandRNG* nodep) override {
        emitOpName(nodep, nodep->emitC(), nullptr, nullptr, nullptr);
    }
    فراغ ویکھو(Ast_ویلا* nodep) override {
        putns(nodep, "VL_TIME_UNITED_Q(");
        UASSERT_OBJ(!nodep->timeunit().isNone(), nodep, "$time has no units");
        ثابت مزدوج ویلا_2
            = nodep->timeunit().multiplier() / v3Global.rootp()->timeprecision().multiplier();
        UASSERT_OBJ(ویلا_2 >= 1, nodep, "TimeQ is less than 1, will result in division by zero");
        puts(cvt_منزل_Str(ویلا_2));
        puts(")");
    }
    فراغ ویکھو(Ast_ویلا_D* nodep) override {
        putns(nodep, "VL_TIME_UNITED_D(");
        UASSERT_OBJ(!nodep->timeunit().isNone(), nodep, "$realtime has no units");
        puts(cvt_منزل_Str(nodep->timeunit().multiplier()
                      / v3Global.rootp()->timeprecision().multiplier()));
        puts(")");
    }
    فراغ ویکھو(Ast_Get_Initial_Random_بیج* nodep) override {
        putns(nodep, "vlSymsp->_vm_contextp__->randSeed()");
    }
    فراغ ویکھو(Ast_ویلا_Format* nodep) override {
        putns(nodep, "VL_TIMEFORMAT_IINI(");
        إذا (nodep->unitsp()) {
            puts("true, ");
            iterate_And_اگلا_Const_Null(nodep->unitsp());
        } وإلا {
            puts("false, 0");
        }
        puts(", ");
        إذا (nodep->precisionp()) {
            puts("true, ");
            iterate_And_اگلا_Const_Null(nodep->precisionp());
        } وإلا {
            puts("false, 0");
        }
        puts(", ");
        إذا (nodep->suffixp()) {
            puts("true, ");
            iterate_And_اگلا_Const_Null(nodep->suffixp());
        } وإلا {
            puts("false, \"\"");
        }
        puts(", ");
        إذا (nodep->widthp()) {
            puts("true, ");
            iterate_And_اگلا_Const_Null(nodep->widthp());
        } وإلا {
            puts("false, 0");
        }
        puts(", vlSymsp->_vm_contextp__);\n");
    }
    فراغ ویکھو(Ast_ویلا_Precision* nodep) override {
        putns(nodep, "vlSymsp->_vm_contextp__->timeprecision()");
    }

    // Nodes involing AstText
    فراغ ویکھو(Ast_متن* nodep) override {
        // All Text should be under TextBlock/CStmt/CStmtUser/CExpr/CExprUser
        nodep->v3fatalSrc("Text node in unexpected position");
    }
    فراغ ویکھو(Ast_متن_بلاک* nodep) override {
        putnbs(nodep, "");
        puts(nodep->اگیتر());
        emit_Nodes_With_متن(nodep->nodesp(), false, true, nodep->separator());
        puts(nodep->suffix());
    }
    فراغ ویکھو(AstCStmt* nodep) override {
        putnbs(nodep, "");
        emit_Nodes_With_متن(nodep->nodesp(), false, true, "");
        ensure_New_سطر();
    }
    فراغ ویکھو(AstCExpr* nodep) override {
        putnbs(nodep, "");
        emit_Nodes_With_متن(nodep->nodesp(), false, true, "");
    }
    فراغ ویکھو(AstCStmtUser* nodep) override {
        putnbs(nodep, "");
        ofp()->putsNoTracking("\n");
        إذا (nodep->fromDollarC() && v3Global.opt.decoration() && !v3Global.opt.protectIds()) {
            ofp()->putsNoTracking("// $c statement at " + nodep->fileline()->ascii() + "\n");
        }
        emit_Nodes_With_متن(nodep->nodesp(), m_useSelfForThis, false, "");
        puts("\n");
    }
    فراغ ویکھو(AstCExprUser* nodep) override {
        putnbs(nodep, "");
        ofp()->putsNoTracking("\n");
        إذا (/* is always from $c */ v3Global.opt.decoration() && !v3Global.opt.protectIds()) {
            ofp()->putsNoTracking(
                (nodep->is_خالص() ? "// $cpure expression at " : "// $c expression at ")
                + nodep->fileline()->ascii() + "\n");
        }
        emit_Nodes_With_متن(nodep->nodesp(), m_useSelfForThis, false, "");
        puts("\n");
    }

    // Operators
    فراغ ویکھو(Ast_گرہ_Termop* nodep) override {
        emitOpName(nodep, nodep->emitC(), nullptr, nullptr, nullptr);
    }
    فراغ ویکھو(Ast_گرہ_Uniop* nodep) override {
        إذا (nodep->emit_Check_Max_الفاظ()
            && (nodep->چوڑائی_الفاظ() > VL_MULS_MAX_الفاظ
                || nodep->lhsp()->چوڑائی_الفاظ() > VL_MULS_MAX_الفاظ)) {
            nodep->v3warn(
                E_UNSUPPORTED,
                "Unsupported: "
                    << nodep->prettyOperatorName() << " operator of " << nodep->چوڑائی()
                    << " bits exceeds hardcoded limit VL_MULS_MAX_WORDS in verilatedos.h");
        }
        إذا (emitSimpleOk(nodep)) {
            putnbs(nodep, "(");
            puts(nodep->emitSimpleOperator());
            puts(" ");
            iterate_And_اگلا_Const_Null(nodep->lhsp());
            puts(")");
        } وإلا {
            emitOpName(nodep, nodep->emitC(), nodep->lhsp(), nullptr, nullptr);
        }
    }
    فراغ ویکھو(Ast_گرہ_Biop* nodep) override {
        إذا (nodep->emit_Check_Max_الفاظ() && nodep->چوڑائی_الفاظ() > VL_MULS_MAX_الفاظ) {
            nodep->v3warn(
                E_UNSUPPORTED,
                "Unsupported: "
                    << nodep->prettyOperatorName() << " operator of " << nodep->چوڑائی()
                    << " bits exceeds hardcoded limit VL_MULS_MAX_WORDS in verilatedos.h");
        }
        إذا (emitSimpleOk(nodep)) {
            putnbs(nodep, "(");
            iterate_And_اگلا_Const_Null(nodep->lhsp());
            puts(" ");
            putbs(nodep->emitSimpleOperator());
            puts(" ");
            iterate_And_اگلا_Const_Null(nodep->rhsp());
            puts(")");
        } وإلا {
            emitOpName(nodep, nodep->emitC(), nodep->lhsp(), nodep->rhsp(), nullptr);
        }
    }
    فراغ ویکھو(Ast_گرہ_Triop* nodep) override {
        UASSERT_OBJ(!emitSimpleOk(nodep), nodep, "Triop cannot be described in a simple way");
        emitOpName(nodep, nodep->emitC(), nodep->lhsp(), nodep->rhsp(), nodep->thsp());
    }
    فراغ ویکھو(AstCvtPackString* nodep) override { emitCvtPackStr(nodep->lhsp()); }
    فراغ ویکھو(AstRedXor* nodep) override {
        إذا (nodep->lhsp()->isWide()) {
            ویکھو(static_cast<Ast_گرہ_Uniop*>(nodep));
        } وإلا {
            ثابت AstVarRef* ثابت vrefp = VN_CAST(nodep->lhsp(), VarRef);
            ثابت صحيح چوڑائی_Pow2 = vrefp ? vrefp->varp()->dtypep()->چوڑائی_Pow2()
                                        : nodep->lhsp()->dtypep()->چوڑائی_Pow2();
            UASSERT_OBJ(چوڑائی_Pow2 > 1, nodep,
                        "Reduction over single bit value should have been folded");
            putnbs(nodep, "VL_REDXOR_");
            puts(cvt_منزل_Str(چوڑائی_Pow2));
            puts("(");
            iterate_And_اگلا_Const_Null(nodep->lhsp());
            puts(")");
        }
    }
    فراغ ویکھو(AstCCast* nodep) override {
        // Extending a value of the same word width is just a NOP.
        إذا (ثابت AstClassRefDType* ثابت classDtypep
            = VN_CAST(nodep->dtypep()->skipRefp(), ClassRefDType)) {
            putns(nodep, "(" + classDtypep->cType("", false, false) + ")(");
        } وإلا إذا (nodep->ناپ() <= VL_BYTESIZE) {
            putns(nodep, "(::CData)(");
        } وإلا إذا (nodep->ناپ() <= VL_SHORTSIZE) {
            putns(nodep, "(::SData)(");
        } وإلا إذا (nodep->ناپ() <= VL_IDATASIZE) {
            putns(nodep, "(::IData)(");
        } وإلا {
            putns(nodep, "(::QData)(");
        }
        iterate_And_اگلا_Const_Null(nodep->lhsp());
        puts(")");
    }
    فراغ ویکھو(AstCond* nodep) override {
        // Widths match up already, so we'll just use C++'s operator w/o any temps.
        إذا (nodep->thenp()->isWide()) {
            emitOpName(nodep, nodep->emitC(), nodep->condp(), nodep->thenp(), nodep->elsep());
        } وإلا {
            putnbs(nodep, "(");
            iterate_And_اگلا_Const_Null(nodep->condp());
            putbs(" ? ");
            iterate_And_اگلا_Const_Null(nodep->thenp());
            putbs(" : ");
            iterate_And_اگلا_Const_Null(nodep->elsep());
            puts(")");
        }
    }
    فراغ ویکھو(Ast_میل_Masked* nodep) override {
        emitOpName(nodep, nodep->emitC(), nodep->lhsp(), nodep->matchp(), nullptr);
    }
    فراغ ویکھو(AstMemberSel* nodep) override {
        ثابت AstVar* ثابت varp = nodep->varp();
        ثابت bool dereferenceCovergroupRef
            = varp->covergroupRefMember() && nodep->access().isReadOrRW();
        إذا (dereferenceCovergroupRef) putnbs(nodep, "(*");
        iterate_And_اگلا_Const_Null(nodep->fromp());
        putnbs(nodep, "->");
        إذا (varp->isIfaceRef()) {
            // varp is the __Viftop companion (e.g. "tx__Viftop"); use the
            // MemberSel name which matches the cell's C++ member (e.g. "tx").
            puts(nodep->nameProtect());
        } وإلا {
            puts(varName(varp));
        }
        إذا (dereferenceCovergroupRef) puts(")");
    }
    فراغ ویکھو(AstStructSel* nodep) override {
        iterate_And_اگلا_Const_Null(nodep->fromp());
        putnbs(nodep, ".");
        puts(nodep->nameProtect());
    }
    فراغ ویکھو(AstNullCheck* nodep) override {
        putns(nodep, "VL_NULL_CHECK(");
        iterate_And_اگلا_Const_Null(nodep->lhsp());
        puts(", ");
        putsQuoted(protect(nodep->fileline()->filename()));
        puts(", ");
        puts(cvt_منزل_Str(nodep->fileline()->lineno()));
        puts(")");
    }
    فراغ ویکھو(AstNewCopy* nodep) override {
        // Polymorphic shallow clone: preserves runtime type via virtual clone()
        // VL_NULL_CHECK enforces null check per IEEE 1800-2023 8.7
        putns(nodep, "VL_NULL_CHECK(");
        إذا (VN_IS(nodep->rhsp(), Const) && VN_AS(nodep->rhsp(), Const)->isNull()) {
            // V3Const folded rhs to null: emit a typed empty ref so VL_NULL_CHECK fires
            ثابت AstClassRefDType* ثابت refDTypep
                = VN_CAST(nodep->dtypep()->skipRefp(), ClassRefDType);
            puts(refDTypep->cType("", false, false) + "{}");
        } وإلا {
            iterate_And_اگلا_Const_Null(nodep->rhsp());
        }
        puts(", ");
        putsQuoted(protect(nodep->fileline()->filename()));
        puts(", ");
        puts(cvt_منزل_Str(nodep->fileline()->lineno()));
        puts(").clone(vlSymsp->__Vm_deleter)");
    }
    فراغ ویکھو(AstSel* nodep) override {
        // Note ASSIGN checks for this on a LHS
        UASSERT_OBJ(nodep->چوڑائی_Min() == nodep->چوڑائی_Const(), nodep, "Width mismatch");
        emitOpName(nodep, nodep->emitC(), nodep->fromp(), nodep->lsbp(), nullptr);
    }
    فراغ ویکھو(AstReplicate* nodep) override {
        إذا (nodep->srcp()->چوڑائی_Min() == 1 && !nodep->isWide()) {
            UASSERT_OBJ((static_cast<صحيح>(VN_AS(nodep->countp(), Const)->منزل_UInt())
                         * nodep->srcp()->چوڑائی_Min())
                            == nodep->چوڑائی_Min(),
                        nodep, "Replicate non-constant or width miscomputed");
            putns(nodep, "VL_REPLICATE_");
            emitIQW(nodep);
            puts("OI(");
            إذا (nodep->srcp()) puts(cvt_منزل_Str(nodep->srcp()->چوڑائی_Min()));
            puts(", ");
            iterate_And_اگلا_Const_Null(nodep->srcp());
            puts(", ");
            iterate_And_اگلا_Const_Null(nodep->countp());
            puts(")");
        } وإلا {
            emitOpName(nodep, nodep->emitC(), nodep->srcp(), nodep->countp(), nullptr);
        }
    }
    فراغ emit_دھارا_R(Ast_دھارا_R* nodep, Ast_گرہ* ماپا) {
        //TODO: This might need to handle more cases like the visit(AstStreamR) function
        emitOpName(nodep, nodep->emitC(), nodep->lhsp(), nodep->rhsp(), nullptr);
    }
    فراغ ویکھو(Ast_دھارا_R* nodep) override {
        //The parrent node of our AstStreamR will give just enough info for what streamR should
        //output if nodep->backp() is not the parent then emitStreamR should have been used. throw
        //an error
        bool backp_Is_ماپا = (nodep->backp()->op1p() == nodep || nodep->backp()->op2p() == nodep);
        UASSERT(backp_Is_ماپا, "can not find return type for streamR");
        إذا (ثابت Ast_قطار_DType* ثابت qdtp
            = VN_CAST(nodep->backp()->dtypep()->skipRefp(), قطار_DType)) {
            إذا (nodep->lhsp()->isWide()) {
                emitOpName(nodep,
                           "VL_STREAMR_%nq%lq%rq(%lw," + std::منزل_string(qdtp->subDTypep()->چوڑائی())
                               + ", %P, %li, %ri)",
                           nodep->lhsp(), nodep->rhsp(), nullptr);
            } وإلا {
                emitOpName(nodep, "VL_STREAMR_%nq%lq%rq(%lw, %P, %li, %ri)", nodep->lhsp(),
                           nodep->rhsp(), nullptr);
            }
        } وإلا إذا (VN_IS(nodep->lhsp()->dtypep()->skipRefp(), قطار_DType)) {
            إذا (!((nodep->backp()->op1p() && nodep->backp()->op1p()->isWide())
                  || (nodep->backp()->op2p() && nodep->backp()->op2p()->isWide()))) {
                //If our lhsp is a queue make sure we streamR and return the correct type.
                //If either side is wide or the previous node is string type dont use this case
                emitOpName(nodep->backp(), "VL_STREAMR_%nq%lq%rq(%lw, %P, %li, %ri)",
                           nodep->lhsp(), nodep->rhsp(), nullptr);
            } وإلا {
                emitOpName(nodep, nodep->emitC(), nodep->lhsp(), nodep->rhsp(), nullptr);
            }
        } وإلا {
            emitOpName(nodep, nodep->emitC(), nodep->lhsp(), nodep->rhsp(), nullptr);
        }
    }
    فراغ ویکھو(Ast_دھارا_L* nodep) override {
        // Attempt to use a "fast" stream function for slice size = power of 2
        إذا (!nodep->isWide()) {
            ثابت uint32_t isPow2 = VN_AS(nodep->rhsp(), Const)->عدد().گنتی_Ones() == 1;
            ثابت uint32_t ٹکڑا_ناپ = VN_AS(nodep->rhsp(), Const)->منزل_UInt();
            إذا (isPow2 && ٹکڑا_ناپ <= (nodep->isQuad() ? حجم(uint64_t) : حجم(uint32_t))) {
                putns(nodep, "VL_STREAML_FAST_");
                bool ورتدا_قطار = false;
                Ast_قطار_DType* qtypep
                    = nodep->backp()->op2p()
                          ? VN_CAST(nodep->backp()->op2p()->dtypep()->skipRefp(), قطار_DType)
                          : nullptr;
                إذا (VN_IS(nodep->backp(), سونپو)
                    && qtypep) {  // If we are assigning to a queue then emit the correct symbol
                    puts("R");  // R for queue
                    ورتدا_قطار = true;
                } وإلا {
                    emitIQW(nodep);
                }
                emitIQW(nodep->lhsp());
                puts("I(");
                puts(cvt_منزل_Str(nodep->lhsp()->چوڑائی_Min()));
                puts(", ");
                إذا (ورتدا_قطار) {
                    iterate_And_اگلا_Const_Null(nodep->backp()->op2p());
                    puts(", ");
                }
                iterate_And_اگلا_Const_Null(nodep->lhsp());
                puts(", ");
                ثابت uint32_t rd_log2 = V3_نمبر::log2b(VN_AS(nodep->rhsp(), Const)->منزل_UInt());
                puts(cvt_منزل_Str(rd_log2) + ")");
                أرجع;
            }
        }
        إذا (VN_IS(nodep->backp(), سونپو)
            && VN_IS(nodep->backp()->op2p()->dtypep()->skipRefp(), قطار_DType)) {
            صحيح قطار_چوڑائی
                = nodep->backp()->op2p()->dtypep()->subDTypep()->چوڑائی();  //We need to know the
                                                                           //width of both sides
            emitOpName(nodep,
                       "VL_STREAML_%nq%lq%rq(%lw," + std::منزل_string(قطار_چوڑائی)
                           + ", %P, %li, %ri)",
                       nodep->lhsp(), nodep->rhsp(), nullptr);
        } وإلا {
            emitOpName(nodep, nodep->emitC(), nodep->lhsp(), nodep->rhsp(), nullptr);
        }
    }
    فراغ ویکھو(AstCastDynamic* nodep) override {
        putnbs(nodep, "VL_CAST_DYNAMIC(");
        iterate_And_اگلا_Const_Null(nodep->fromp());
        puts(", ");
        iterate_And_اگلا_Const_Null(nodep->چوٹی());
        puts(")");
    }
    فراغ ویکھو(Ast_گنتی_بٹ* nodep) override {
        putnbs(nodep, "VL_COUNTBITS_");
        emitIQW(nodep->lhsp());
        puts("(");
        puts(cvt_منزل_Str(nodep->lhsp()->چوڑائی_Min()));
        puts(", ");
        إذا (nodep->lhsp()->isWide()) {
            puts(cvt_منزل_Str(nodep->lhsp()->چوڑائی_الفاظ()));  // Note argument width, not node width
                                                          // (which is always 32)
            puts(", ");
        }
        iterate_And_اگلا_Const_Null(nodep->lhsp());
        puts(", ");
        iterate_And_اگلا_Const_Null(nodep->rhsp());
        puts(", ");
        iterate_And_اگلا_Const_Null(nodep->thsp());
        puts(", ");
        iterate_And_اگلا_Const_Null(nodep->fhsp());
        puts(")");
    }
    فراغ ویکھو(AstTriggered* nodep) override {
        // Test each word of the trigger vector with a mask of the relevant bits
        std::map<uint32_t, uint64_t> لفظ_Masks;
        لكل (ثابت uint32_t اشاریہ : nodep->indices()) {
            لفظ_Masks[VL_BITWORD_Q(اشاریہ)] |= VL_ثنائی_ہندسہ_Q(اشاریہ);
        }
        putns(nodep, "((");
        bool پہلا = true;
        لكل (ثابت خودکار& لفظ_نقاب : لفظ_Masks) {
            إذا (!پہلا) puts("\n| ");
            پہلا = false;
            ثابت bool allOnes = لفظ_نقاب.دوجا == ~0ULL;
            إذا (!allOnes) puts("(" + V3_نمبر{nodep, 64}.سیٹ_Quad(لفظ_نقاب.دوجا).emitC() + " & ");
            iterateConst(nodep->trigRefp());
            puts("[" + std::منزل_string(لفظ_نقاب.پہلا) + "U]");
            إذا (!allOnes) puts(")");
        }
        puts(") != 0ULL)");
    }
    // Terminals
    فراغ ویکھو(AstVarRef* nodep) override {
        ثابت AstVar* ثابت varp = nodep->varp();

        // Reference to constant pool variable is special
        إذا (varp->const_Pool_اندراج()) {
            putns(nodep, EmitCUtil::constPoolName(varp));
            أرجع;
        }

        ثابت Ast_گرہ_Module* ثابت varModp = EmitCParentModule::get(varp);
        ثابت bool dereferenceCovergroupRef
            = varp->covergroupRefMember() && nodep->access().isReadOrRW();
        إذا (dereferenceCovergroupRef) putns(nodep, "(*");
        إذا (VN_IS(varModp, Class) && varModp != m_modp) {
            // Superclass member reference
            putns(nodep, EmitCUtil::اگیتر_Name_Protect(varModp) + "::");
        } وإلا إذا (varp->isIfaceRef()) {
            putns(nodep, nodep->self_اشارہ_گر_Protect(m_useSelfForThis));
            أرجع;
        } وإلا إذا (!nodep->self_اشارہ_گر().is_خالی_تھاواں()) {
            emitDereference(nodep, nodep->self_اشارہ_گر_Protect(m_useSelfForThis));
        }
        putns(nodep, varName(varp));
        إذا (dereferenceCovergroupRef) puts(")");
    }
    فراغ ویکھو(Ast_Addr_دا_CFunc* nodep) override {
        // Note: Can be thought to handle more, but this is all that is needed right now
        ثابت AstCFunc* ثابت funcp = nodep->funcp();
        UASSERT_OBJ(funcp->isLoose(), nodep, "Cannot take address of non-loose method");
        putns(nodep, "&");
        puts(funcNameProtect(funcp));
    }
    فراغ ویکھو(AstConst* nodep) override {  //
        إذا (m_wide_عارضی_Refp && nodep->isWide()) {
            UASSERT_OBJ(m_wide_عارضی_Refp, nodep, "Wide Constant w/ no temp");
            emitConstantW(nodep, m_wide_عارضی_Refp);
            m_wide_عارضی_Refp = nullptr;  // We used it, fail if set it a second time
        } وإلا {
            emitConstant(nodep);
        }
    }
    فراغ ویکھو(Ast_گرہ_Cons* nodep) override { emitValCons(nodep); }
    فراغ ویکھو(AstThisRef* nodep) override {
        putnbs(nodep, nodep->dtypep()->cType("", false, false));
        puts("{");
        puts(VSelf_اشارہ_گر_متن::replaceThis(m_useSelfForThis, "this"));
        puts("}");
    }
    فراغ ویکھو(Ast_گرہ_Sel* nodep) override {
        إذا (!VN_IS(nodep, ArraySel) && !VN_IS(nodep, لفظ_Sel)) {
            ویکھو(static_cast<Ast_گرہ_Biop*>(nodep));
            أرجع;
        }
        // ArraySel or WordSel
        iterate_And_اگلا_Const_Null(nodep->fromp());
        // Special case constant index for readability
        إذا (AstConst* ثابت idxp = VN_CAST(nodep->bitp(), Const)) {
            puts("[" + std::منزل_string(idxp->منزل_UInt()) + "U]");
            أرجع;
        }
        putbs("[");
        iterate_And_اگلا_Const_Null(nodep->bitp());
        puts("]");
    }

    //
    فراغ ویکھو(Ast_Exec_گراف* nodep) override {
        // The location of the AstExecGraph within the containing AstCFunc is where we want to
        // invoke the graph and wait for it to complete. The run-time library does just that.
        putns(nodep, "vl_invokeExecGraph(" + EmitCUtil::exec_گراف_Name(nodep) + ", vlSelf, ");
        iterateConst(nodep->triggerp());
        puts(".data());\n");
    }

    // Default
    فراغ ویکھو(Ast_گرہ* nodep) override {  // LCOV_EXCL_START
        putns(nodep, "\n???? // "s + nodep->prettyTypeName() + "\n");
        iterateChildrenConst(nodep);
        إذا (!v3Global.opt.lintOnly()) {  // An internal problem, so suppress
            nodep->v3fatalSrc("Unknown node type reached emitter: " << nodep->prettyTypeName());
        }
    }  // LCOV_EXCL_STOP

محمي:
    EmitCFunc() = افتراضي;
    ~EmitCFunc() override = افتراضي;
};

#جے_انت  // guard
