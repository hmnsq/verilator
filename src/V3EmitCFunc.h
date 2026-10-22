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

#اگر_غیر_تعریف_شدہ VERILATOR_V3EMITCFUNC_H_
#تعریف_کرو VERILATOR_V3EMITCFUNC_H_

#شامل "config_build.h"
#شامل "verilatedos.h"

#شامل "V3EmitCConstInit.h"
#شامل "V3Global.h"
#شامل "V3MemberMap.h"

#شامل <algorithm>
#شامل <map>
#شامل <unordered_map>
#شامل <unordered_set>
#شامل <vector>

// Number of VL_CONST_W_*X's in verilated.h (IE VL_CONST_W_8X is last)
constexpr صحیح EMITC_عدد_CONSTW = 8;

//######################################################################
// Emit lazy forward declarations

class EmitCLazyDecls final : عوامی VNVisitorConst {
    // NODE STATE/TYPES
    // None allowed to support threaded emitting

    // MEMBERS
    std::unordered_سیٹ<string> m_emittedManually;  // Set of names already declared manually.
    EmitCBaseVisitorConst& m_emitter;  // For access to file output
    bool m_needs_Blank_سطر = false;  // Emit blank line if any declarations were emitted (cosmetic)
    std::سیٹ<Ast_گرہ*> m_emitted;  // -> in set. Already emitted decl for symbols.

    // METHODS
    bool declaredOnce(Ast_گرہ* nodep) { واپس m_emitted.داخل_کرو(nodep).دوسرا; }

    خالی lazyDeclare(AstCFunc* funcp) {
        // Already declared in this compilation unit
        اگر (!declaredOnce(funcp)) واپس;
        // Check if this kind of function is lazily declared
        اگر (!(funcp->is_طریقہ() && funcp->isLoose()) && !funcp->dpiImportPrototype()) واپس;
        // Already declared manually
        اگر (m_emittedManually.گنتی(funcp->nameProtect())) واپس;
        // Needs lazy declaration, emit one
        m_emitter.emitCFuncDecl(funcp, EmitCParentModule::get(funcp), funcp->dpiImportPrototype());
        m_needs_Blank_سطر = true;
    }

    خالی lazyDeclareConstPoolVar(AstVar* varp) {
        اگر (!declaredOnce(varp)) واپس;  // Already declared
        ثابت string nameProtect = EmitCUtil::constPoolName(varp);
        m_emitter.putns(varp, "extern const ");
        m_emitter.puts(varp->dtypep()->cType(nameProtect, false, false));
        m_emitter.puts(";\n");
        m_needs_Blank_سطر = true;
    }

    // VISITORS
    خالی دورہ(Ast_گرہ_CCall* nodep) override {
        lazyDeclare(nodep->funcp());
        iterateChildrenConst(nodep);
    }

    خالی دورہ(Ast_Addr_کا_CFunc* nodep) override {
        lazyDeclare(nodep->funcp());
        iterateChildrenConst(nodep);
    }

    خالی دورہ(AstVarRef* nodep) override {
        AstVar* ثابت varp = nodep->varp();
        // Only constant pool symbols are lazy declared for now ...
        اگر (EmitCParentModule::get(varp)->isConstPool()) lazyDeclareConstPoolVar(varp);
    }

    خالی دورہ(Ast_Exec_گراف* nodep) override {
        اگر (!declaredOnce(nodep)) واپس;
        m_emitter.putns(nodep, "extern VlExecGraph " + EmitCUtil::exec_گراف_Name(nodep) + ";\n");
        m_needs_Blank_سطر = true;
    }

    خالی دورہ(Ast_گرہ* nodep) override { iterateChildrenConst(nodep); }

عوامی:
    explicit EmitCLazyDecls(EmitCBaseVisitorConst& emitter)
        : m_emitter{emitter} {}
    خالی emit(Ast_گرہ* nodep) {
        m_needs_Blank_سطر = false;
        iterateChildrenConst(nodep);
        اگر (m_needs_Blank_سطر) m_emitter.puts("\n");
    }
    خالی emit(ثابت string& سابقہ, ثابت string& name, ثابت string& suffix) {
        m_emittedManually.داخل_کرو(name);
        m_emitter.ensure_New_سطر();
        m_emitter.puts(سابقہ);
        m_emitter.puts(name);
        m_emitter.puts(suffix);
        m_emitter.ensure_New_سطر();
    }
    خالی declared(AstCFunc* nodep) { m_emitted.داخل_کرو(nodep); }
    خالی reset() { m_emitted.clear(); }
};

// ######################################################################
//  Emit statements and expressions

class EmitCFunc VL_NOT_FINAL : عوامی EmitCConstInit {
    VMemberMap m_memberMap;
    AstVarRef* m_wide_عارضی_Refp = nullptr;  // Variable that _WW macros should be setting
    std::unordered_map<Ast_Jump_بلاک*, سائز_t> m_labelNumbers;  // Label numbers for AstJumpBlocks
    bool m_createdScopeHash = false;  // Already created a scope hash

محفوظ:
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
        ثابت Ast_گرہ* ثابت newp = m_memberMap.ڈھونڈو_Member(classp, "new");
        اگر (!newp) واپس false;
        ثابت AstCFunc* ثابت ctorp = VN_CAST(newp, CFunc);
        اگر (!ctorp) واپس false;
        UASSERT_OBJ(ctorp->isConstructor(), ctorp, "`new` is not a constructor!");
        واپس ctorp->needProcess();
    }

    bool constructorNeedsProcess(ثابت Ast_گرہ_DType* ثابت dtypep) {
        اگر (ثابت AstClassRefDType* ثابت crefdtypep = VN_CAST(dtypep, ClassRefDType))
            واپس constructorNeedsProcess(crefdtypep->classp());
        واپس false;
    }

    bool coverage_استعمال_کرتا_Local_شمار(Ast_گرہ_Cover_Decl* ثابت declp) ثابت {
        // Only functions with an object receiver can address the module-local
        // coverage array. Static/package/class helper functions still use the
        // symbol-table array, as they may have vlSymsp but no vlSelf/this.
        // Also require the coverage declaration to belong to the current
        // module; cloned task bodies can increment declarations owned by a
        // different module, and those must keep using the global array.
        واپس m_cfuncp && !m_cfuncp->isStatic() && !VN_IS(m_modp, Class)
               && !VN_IS(m_modp, ClassPackage)
               && EmitCParentModule::get(declp->ڈیٹا_Decl_Thisp()) == m_modp;
    }
    صحیح coverage_Bin_عدد(Ast_گرہ_Cover_Decl* ثابت declp, bool forceGlobal = false) ثابت {
        واپس !forceGlobal && coverage_استعمال_کرتا_Local_شمار(declp)
                   ? declp->ڈیٹا_Decl_Thisp()->local_Bin_عدد()
                   : declp->ڈیٹا_Decl_Thisp()->bin_عدد();
    }
    خالی putCoverageArray(Ast_گرہ_Cover_Decl* ثابت declp, bool forceGlobal = false) {
        اگر (!forceGlobal && coverage_استعمال_کرتا_Local_شمار(declp)) {
            // Keep counters on the module object when possible, so every
            // no-inline instance has storage available if coverage output is
            // later written with forcePerInstance.
            puts(m_cfuncp->isLoose() ? "vlSelf->__Vcoverage" : "this->__Vcoverage");
        } ورنہ {
            // Static/package/class helper functions do not have vlSelf, so
            // keep using the shared symbol-table coverage array there.
            puts("vlSymsp->__Vcoverage");
        }
    }
    خالی emit_Cover_Other_Decl_داخل_کرو(AstCoverOtherDecl* nodep, bool forceGlobal = false) {
        putns(nodep, "vlSelf->__vlCoverInsert(");  // As Declared in emitCoverageDecl
        putCoverageArray(nodep, forceGlobal);
        puts(" + ");
        puts(cvt_مقصد_Str(coverage_Bin_عدد(nodep, forceGlobal)));
        // first controls whether duplicate module instances are collapsed in
        // the default coverage view. The following flag says whether countp
        // points to an object-local counter; only those counters must be kept
        // real for later instances so forcePerInstance can split hierarchy.
        puts(", first");  // Enable, passed from __Vconfigure parameter
        puts(", ");
        puts(!forceGlobal && coverage_استعمال_کرتا_Local_شمار(nodep) ? "true" : "false");
        puts(", ");
        putsQuoted(protect(nodep->fileline()->filename()));
        puts(", ");
        puts(cvt_مقصد_Str(nodep->fileline()->lineno()));
        puts(", ");
        puts(cvt_مقصد_Str(nodep->انحراف() + nodep->fileline()->پہلا_عمود()));
        puts(", ");
        putsQuoted((!nodep->hier().خالی_خانے() ? "." : "")
                   + VIdProtect::protect_الفاظ_If(nodep->hier(), nodep->protect()));
        puts(", ");
        putsQuoted(VIdProtect::protect_الفاظ_If(nodep->صفحہ(), nodep->protect()));
        puts(", ");
        putsQuoted(VIdProtect::protect_الفاظ_If(nodep->comment(), nodep->protect()));
        puts(", ");
        putsQuoted(nodep->linescov());
        puts(", ");
        putsQuoted(VIdProtect::protect_الفاظ_If(nodep->fsmVar(), nodep->protect()));
        puts(", ");
        putsQuoted(VIdProtect::protect_الفاظ_If(nodep->fsmFrom(), nodep->protect()));
        puts(", ");
        putsQuoted(VIdProtect::protect_الفاظ_If(nodep->fsm_مقصد(), nodep->protect()));
        puts(", ");
        putsQuoted(VIdProtect::protect_الفاظ_If(nodep->fsm_لیبل(), nodep->protect()));
        puts(");\n");
    }

عوامی:
    // METHODS
    bool display_Emit_سرخی(Ast_گرہ* nodep);
    خالی display_گرہ(Ast_گرہ* nodep, AstSFormatF* fmtp, ثابت string& vformat, Ast_گرہ* exprsp,
                     bool is_جائزہ);

    bool emitSimpleOk(Ast_گرہ_اظہاریہ* nodep);
    خالی emitIQW(ثابت Ast_گرہ* nodep) {
        // See "Type letters" in verilated.h
        // Other abbrevs: "C"har, "S"hort, "F"loat, "D"ouble, stri"N"g, "R"=queue, "U"npacked
        puts(nodep->dtypep()->skipRefp()->charIQWN());
    }
    خالی emitRU(ثابت Ast_گرہ* nodep) {
        ثابت Ast_گرہ_DType* dtp = nodep->dtypep()->skipRefp();
        // See "Type letters" in verilated.h
        اگر (VN_IS(dtp, UnpackArrayDType))
            puts("U");
        ورنہ اگر (VN_IS(dtp, قطار_DType) || VN_IS(dtp, DynArrayDType))
            puts("R");
    }
    خالی emitScIQW(AstVar* nodep) {
        UASSERT_OBJ(nodep->isSc(), nodep, "emitting SystemC operator on non-SC variable");
        puts(nodep->isScBigUint()                           ? "SB"
             : (nodep->isScUint() || nodep->isScUintBool()) ? "SU"
             : nodep->isScBv()                              ? "SW"
             : nodep->isDouble()                            ? "SD"
                                                            : (nodep->isScQuad() ? "SQ" : "SI"));
    }
    خالی emitDatap(ثابت Ast_گرہ* nodep) {
        // When passing to a function with va_args the compiler doesn't
        // know need a pointer so when wide, need to look inside VlWide
        اگر (nodep->isWide()) puts(".data()");
    }
    خالی emitOpName(Ast_گرہ* nodep, ثابت string& format, Ast_گرہ* lhsp, Ast_گرہ* rhsp,
                    Ast_گرہ* thsp);
    خالی emit_CCall_دلائل(ثابت Ast_گرہ_CCall* nodep, ثابت string& self_اشارہ_گر, bool اندر_Process);
    خالی emitDereference(Ast_گرہ* nodep, ثابت string& اشارہ_گر);
    std::string dereferenceString(ثابت std::string& اشارہ_گر) ثابت;
    خالی emitCvtPackStr(Ast_گرہ* nodep);
    خالی emitConstant(AstConst* nodep);
    خالی emitConstantString(ثابت AstConst* nodep);
    خالی emitVarReset(ثابت string& سابقہ, AstVar* varp, bool constructing);
    string emitVarResetRecurse(ثابت AstVar* varp, bool constructing,
                               ثابت string& varNameProtected, Ast_گرہ_DType* dtypep, صحیح گہرائی,
                               ثابت string& suffix, ثابت Ast_گرہ* valuep);
    خالی emitVarResetScopeHash();
    خالی emitChangeDet();
    خالی put_Comma_Iterate_اگلا(Ast_گرہ* nodep, bool comma = false) {
        برائے (Ast_گرہ* subnodep = nodep; subnodep; subnodep = subnodep->nextp()) {
            اگر (comma) puts(", ");
            iterateConst(subnodep);
            comma = true;
        }
    }
    سانچہ <typename T>
    string optionalProcArg(ثابت T* ثابت nodep) {
        واپس (nodep && constructorNeedsProcess(nodep)) ? "vlProcess, " : "";
    }
    ثابت AstCNew* getSuperNewCallRecursep(Ast_گرہ* ثابت nodep) {
        // Get the super.new call
        اگر (!nodep) واپس nullptr;
        اگر (ثابت AstCNew* ثابت cnewp = VN_CAST(nodep, CNew)) واپس cnewp;
        اگر (ثابت Ast_Stmt_اظہاریہ* ثابت stmtp = VN_CAST(nodep, Stmt_اظہاریہ)) {
            اگر (ثابت AstCNew* ثابت cnewp = VN_CAST(stmtp->exprp(), CNew)) واپس cnewp;
        }
        اگر (ثابت Ast_Jump_بلاک* ثابت blockp = VN_CAST(nodep, Jump_بلاک)) {
            اگر (ثابت AstCNew* ثابت cnewp = getSuperNewCallRecursep(blockp->stmtsp())) {
                واپس cnewp;
            }
        }
        اگر (ثابت AstCNew* ثابت cnewp = getSuperNewCallRecursep(nodep->nextp())) واپس cnewp;
        واپس nullptr;
    }

    خالی emitConstantW(AstConst* nodep, AstVarRef* assigntop) {
        // For tradition and compilation speed, assign each word directly into
        // output variable instead of using '='
        putns(nodep, "");
        اگر (nodep->عدد().isFourState()) {
            nodep->v3warn(E_UNSUPPORTED, "Unsupported: 4-state numbers in this context");
            واپس;
        }

        صحیح اوپر_چوڑائی = nodep->عدد().چوڑائی_مقصد_Fit();
        صحیح chunks = 0;
        اگر (اوپر_چوڑائی > EMITC_عدد_CONSTW * VL_EDATASIZE) {
            // Output e.g. 8 words in groups of e.g. 8
            chunks = (اوپر_چوڑائی - 1) / (EMITC_عدد_CONSTW * VL_EDATASIZE);
            اوپر_چوڑائی %= (EMITC_عدد_CONSTW * VL_EDATASIZE);
            اگر (اوپر_چوڑائی == 0) اوپر_چوڑائی = (EMITC_عدد_CONSTW * VL_EDATASIZE);
        }
        {  // Upper e.g. 8 words
            اگر (chunks) {
                putnbs(nodep, "VL_CONSTHI_W_");
                puts(cvt_مقصد_Str(VL_الفاظ_I(اوپر_چوڑائی)));
                puts("X(");
                puts(cvt_مقصد_Str(nodep->چوڑائی_Min()));
                puts(",");
                puts(cvt_مقصد_Str(chunks * EMITC_عدد_CONSTW * VL_EDATASIZE));
            } ورنہ {
                putnbs(nodep, "VL_CONST_W_");
                puts(cvt_مقصد_Str(VL_الفاظ_I(اوپر_چوڑائی)));
                puts("X(");
                puts(cvt_مقصد_Str(nodep->چوڑائی_Min()));
            }
            puts(",");
            اگر (!assigntop->self_اشارہ_گر().is_خالی_خانے()) {
                emitDereference(assigntop, assigntop->self_اشارہ_گر_Protect(m_useSelfForThis));
            }
            puts(varName(assigntop->varp()));
            برائے (صحیح لفظ = VL_الفاظ_I(اوپر_چوڑائی) - 1; لفظ >= 0; لفظ--) {
                // Only 32 bits - llx + long long here just to appease CPP format warning
                ofp()->لکھو(",0x%08" PRIx64, static_cast<uint64_t>(nodep->عدد().edata_لفظ(
                                                   لفظ + chunks * EMITC_عدد_CONSTW)));
            }
            puts(")");
        }
        برائے (chunks--; chunks >= 0; chunks--) {
            puts(";\n");
            putbs("VL_CONSTLO_W_");
            puts(cvt_مقصد_Str(EMITC_عدد_CONSTW));
            puts("X(");
            puts(cvt_مقصد_Str(chunks * EMITC_عدد_CONSTW * VL_EDATASIZE));
            puts(",");
            اگر (!assigntop->self_اشارہ_گر().is_خالی_خانے()) {
                emitDereference(assigntop, assigntop->self_اشارہ_گر_Protect(m_useSelfForThis));
            }
            puts(varName(assigntop->varp()));
            برائے (صحیح لفظ = EMITC_عدد_CONSTW - 1; لفظ >= 0; لفظ--) {
                // Only 32 bits - llx + long long here just to appease CPP format warning
                ofp()->لکھو(",0x%08" PRIx64, static_cast<uint64_t>(nodep->عدد().edata_لفظ(
                                                   لفظ + chunks * EMITC_عدد_CONSTW)));
            }
            puts(")");
        }
    }

    خالی emit_Nodes_With_متن(Ast_گرہ* nodesp, bool useSelfForThis, bool tracking,
                           ثابت std::string& separator) {
        برائے (Ast_گرہ* nodep = nodesp; nodep; nodep = nodep->nextp()) {
            اگر (ثابت Ast_متن* ثابت textp = VN_CAST(nodep, متن_2)) {
                ثابت std::string متن
                    = VSelf_اشارہ_گر_متن::replaceThis(useSelfForThis, textp->متن());
                اگر (tracking) {
                    puts(متن);
                } ورنہ {
                    ofp()->putsNoTracking(متن);
                }
            } ورنہ {
                iterateConst(nodep);
            }
            اگر (nodep->nextp()) puts(separator);
        }
    }

    خالی putConstructorSubinit(ثابت AstClass* classp, AstCFunc* cfuncp) {
        // Direct non-virtual bases in declaration order
        برائے (ثابت AstClassExtends* extp = classp->extendsp(); extp;
             extp = VN_AS(extp->nextp(), ClassExtends)) {
            اگر (extp->classp()->isInterfaceClass()) جاری;
            اگر (extp->classp()->use_Virtual_عوامی()) جاری;
            puts(EmitCUtil::سابقہ_Name_Protect(extp->classp()));
            puts("::init");
            اگر (constructorNeedsProcess(extp->classp())) {
                puts("(vlProcess, vlSymsp");
            } ورنہ {
                puts("(vlSymsp");
            }
            // Handle super.new() args for the concrete parent
            اگر (ثابت AstCNew* ثابت superNewCallp = getSuperNewCallRecursep(cfuncp->stmtsp())) {
                put_Comma_Iterate_اگلا(superNewCallp->argsp(), true);
            }
            puts(");\n");
            // Since SystemVerilog allows only for extending from one class, for sure there won't
            // be any more constructors to be called. Even if there are some classes which are
            // "implemented", those are interface classes which do not have constructors.
            توڑو;
        }
    }

    // VISITORS
    using EmitCConstInit::دورہ;
    خالی دورہ(AstCFunc* ثابت nodep) override {
        اگر (nodep->خالی_خانے_Body() && !nodep->isLoose()) واپس;
        VL_RESTORER(m_useSelfForThis);
        VL_RESTORER(m_cfuncp);
        VL_RESTORER(m_instantiatesOwnProcess);
        VL_RESTORER(m_createdScopeHash);
        m_cfuncp = nodep;
        m_instantiatesOwnProcess = false;
        m_labelNumbers.clear();  // No need to save/restore, all Jumps must be within the function

        بانٹو_سائز_Inc(nodep);

        puts("\n");
        m_lazyDecls.emit(nodep);
        اگر (nodep->ifdef() != "") putns(nodep, "#ifdef " + nodep->ifdef() + "\n");
        emit_CFunc_سرخی(nodep, m_modp, /* withScope: */ true);

        puts(" {\n");
        اگر (nodep->isConstructor()) {
            ثابت AstClass* ثابت classp = VN_CAST(nodep->scopep()->modp(), Class);
            اگر (classp && classp->extendsp()) putConstructorSubinit(classp, nodep);
        }

        // "+" in the debug indicates a print from the model
        puts("VL_DEBUG_IF(VL_DBG_MSGF(\"+  ");
        برائے (صحیح i = 0; i < m_modp->level(); ++i) puts("  ");
        puts(EmitCUtil::سابقہ_Name_Protect(m_modp));
        puts(nodep->isLoose() ? "__" : "::");
        puts(nodep->nameProtect() + "\\n\"); );\n");

        اگر (nodep->isLoose()) {
            m_lazyDecls.declared(nodep);  // Defined here, so no longer needs declaration
            اگر (!nodep->isStatic()) {  // Standard prologue
                m_useSelfForThis = true;
                اگر (nodep->voidSelf()) {
                    ثابت string className = EmitCUtil::سابقہ_Name_Protect(m_modp);
                    puts(className + "* const vlSelf = static_cast<" + className
                         + "*>(vlVoidSelf);\n");
                }
                اگر (!VN_IS(m_modp, Class)) {
                    puts(EmitCUtil::sym_Class_تفویض());  // Uses vlSelf
                } ورنہ {
                    puts("(void)vlSelf;  // Prevent unused variable warning\n");
                }
            }
        }

        // Instantiate a process class if it's going to be needed somewhere later
        nodep->forall([&](ثابت Ast_گرہ_CCall* ccallp) -> bool {
            اگر (ccallp->funcp()->needProcess()
                && (ccallp->funcp()->isCoroutine() == VN_IS(ccallp->backp(), CAwait))) {
                اگر (!nodep->needProcess() && !m_instantiatesOwnProcess) {
                    m_instantiatesOwnProcess = true;
                    واپس false;
                }
            }
            واپس true;
        });
        اگر (m_instantiatesOwnProcess) {
            AstCStmt* ثابت vlprocp = new AstCStmt{nodep->fileline()};
            vlprocp->جوڑو("VlProcessRef vlProcess = std::make_shared<VlProcess>();\n");
            vlprocp->جوڑو("VlProcess::currentp(vlProcess.get());");
            nodep->prependStmtsp(vlprocp);
        } ورنہ اگر (nodep->needProcess() && nodep->stmtsp()) {
            // Set current process so VlRNG() constructors in this function seed from it
            AstCStmt* ثابت سیٹ_Processp = new AstCStmt{nodep->fileline()};
            سیٹ_Processp->جوڑو("VlProcess::currentp(vlProcess.get());");
            nodep->prependStmtsp(سیٹ_Processp);
        }

        برائے (Ast_گرہ* subnodep = nodep->argsp(); subnodep; subnodep = subnodep->nextp()) {
            اگر (AstVar* ثابت varp = VN_CAST(subnodep, Var)) {
                اگر (varp->isFuncReturn()) emitVarDecl(varp);
            }
        }

        اگر (m_useSelfForThis) {
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

        اگر (nodep->varsp()) {
            putsDecoration(nodep, "// Locals\n");
            iterate_And_اگلا_Const_Null(nodep->varsp());
        }

        اگر (nodep->stmtsp()) {
            putsDecoration(nodep, "// Body\n");
            iterate_And_اگلا_Const_Null(nodep->stmtsp());
        }

        m_usevlSelfRef = false;

        اگر (nodep->isCoroutine()) {
            // Sometimes coroutines don't have co_awaits,
            // so emit a co_return at the end to avoid compile errors.
            puts("co_return;\n");
        }

        puts("}\n");
        اگر (nodep->ifdef() != "") puts("#endif  // " + nodep->ifdef() + "\n");
    }

    خالی دورہ(AstVar* nodep) override {
        UASSERT_OBJ(m_cfuncp, nodep, "Cannot emit non-local variable");
        emitVarDecl(nodep);
    }

    خالی دورہ(Ast_Cvt_Array_مقصد_Array* nodep) override {
        اگر (nodep->الٹ()) {
            puts("VL_REVCLONE_Q(");
        } ورنہ {
            puts("VL_CLONE_Q(");
        }
        iterate_And_اگلا_Const_Null(nodep->fromp());
        puts(", ");
        puts(cvt_مقصد_Str(nodep->بلاک_سائز()));
        puts(", ");
        puts(cvt_مقصد_Str(nodep->src_Element_بٹ()));
        puts(", ");
        puts(cvt_مقصد_Str(nodep->dst_Element_بٹ()));
        puts(")");
    }

    خالی دورہ(Ast_Cvt_Array_مقصد_Packed* nodep) override {
        Ast_گرہ_DType* ثابت fromDtp = nodep->fromp()->dtypep()->skipRefp();
        Ast_گرہ_DType* ثابت elemDtp = fromDtp->subDTypep()->skipRefp();
        ثابت bool descending = [&]() {
            اگر (ثابت AstUnpackArrayDType* ثابت unpackDtp = VN_CAST(fromDtp, UnpackArrayDType))
                واپس !unpackDtp->declRange().ascending();
            واپس false;
        }();
        puts("VL_PACK_");
        emitIQW(nodep);
        puts("_");
        emitRU(fromDtp);
        emitIQW(elemDtp);
        اگر (descending) {
            // Wrap source in VL_PACK_REVERSED so VL_PACK sees ascending order
            emitOpName(nodep, "(%nw, %rw, %P, VL_PACK_REVERSED(%li))", nodep->fromp(), elemDtp,
                       nullptr);
        } ورنہ {
            emitOpName(nodep, "(%nw, %rw, %P, %li)", nodep->fromp(), elemDtp, nullptr);
        }
    }

    خالی دورہ(Ast_Cvt_Unpack_مقصد_قطار* nodep) override {
        Ast_گرہ_DType* ثابت elemDTypep = nodep->fromp()->dtypep()->subDTypep();
        emitOpName(nodep, nodep->emitC(), nodep->fromp(), elemDTypep, nullptr);
    }
    خالی دورہ(Ast_Cvt_Unpack_مقصد_Unpack* nodep) override {
        // Whether each dimension is reversed, the first for the outermost, up to the last that is
        std::سمتیہ<bool> reverses;
        ثابت AstUnpackArrayDType* چوٹی = VN_AS(nodep->dtypep()->skipRefp(), UnpackArrayDType);
        ثابت AstUnpackArrayDType* fromp
            = VN_AS(nodep->fromp()->dtypep()->skipRefp(), UnpackArrayDType);
        جب (چوٹی) {
            reverses.ڈالو_عقب(چوٹی->declRange().ascending() != fromp->declRange().ascending());
            چوٹی = VN_CAST(چوٹی->subDTypep()->skipRefp(), UnpackArrayDType);
            fromp = VN_CAST(fromp->subDTypep()->skipRefp(), UnpackArrayDType);
        }
        جب (!reverses.عقب()) reverses.نکالو_عقب();
        putns(nodep, "VL_CVT_UNPACK_TO_UNPACK<");
        برائے (سائز_t i = 0; i < reverses.سائز(); ++i) {
            اگر (i) puts(", ");
            puts(reverses[i] ? "1" : "0");
        }
        puts(">(");
        iterate_And_اگلا_Const_Null(nodep->fromp());
        puts(")");
    }

    خالی دورہ(Ast_گرہ_تفویض* nodep) override {
        اگر (AstCReset* ثابت resetp = VN_CAST(nodep->rhsp(), CReset)) {
            // TODO get rid of emitVarReset and instead let AstNodeAssign understand how to init
            // anything
            Ast_گرہ* fromp = nodep->lhsp();
            // Fork needs to use a member select.  Nothing else should be possible before VarRef.
            اگر (AstMemberSel* ثابت sfromp = VN_CAST(fromp, MemberSel)) {
                // Fork-DynScope generated pointer to previously automatic variable
                AstVar* ثابت memberVarp = sfromp->varp();
                fromp = sfromp->fromp();
                اگر (AstNullCheck* ثابت sfromp = VN_CAST(fromp, NullCheck)) fromp = sfromp->lhsp();
                Ast_گرہ_Var_Ref* ثابت fromVarRefp = VN_AS(fromp, گرہ_Var_Ref);
                emitVarReset(
                    ("VL_NULL_CHECK("s
                     + (fromVarRefp->self_اشارہ_گر().is_خالی_خانے()
                            ? ""
                            : dereferenceString(fromVarRefp->self_اشارہ_گر_Protect(m_useSelfForThis)))
                     + varName(fromVarRefp->varp()) + ", \""
                     + V3_برآمد_Formatter::quoteNameControls(protect(nodep->fileline()->filename()))
                     + "\", " + std::مقصد_string(nodep->fileline()->lineno()) + ")->"),
                    memberVarp, resetp->constructing());
            } ورنہ {
                Ast_گرہ_Var_Ref* ثابت fromVarRefp = VN_AS(fromp, گرہ_Var_Ref);
                AstVar* ثابت varp = fromVarRefp->varp();
                ثابت string سابقہ
                    = fromVarRefp->self_اشارہ_گر().is_خالی_خانے()
                          ? ""
                          : dereferenceString(
                                VN_AS(fromp, گرہ_Var_Ref)->self_اشارہ_گر_Protect(m_useSelfForThis));
                emitVarReset(سابقہ, varp, resetp->constructing());
            }
            واپس;
        }
        bool paren = true;
        bool decind = false;
        bool rhs = true;
        bool الٹ_Unpack = false;  // Set for descending CvtPackedToArray
        ثابت AstUnpackArrayDType* ثابت unpackDtp
            = VN_CAST(nodep->dtypep()->skipRefp(), UnpackArrayDType);
        اگر (AstSel* ثابت selp = VN_CAST(nodep->lhsp(), Sel)) {
            UASSERT_OBJ(selp->چوڑائی_Min() == selp->چوڑائی_Const(), selp, "Width mismatch");
            اگر (selp->چوڑائی_Min() == 1) {
                putnbs(nodep, "VL_ASSIGNBIT_");
                emitIQW(selp->fromp());
                اگر (nodep->rhsp()->isAllOnesV()) {
                    puts("O(");
                    rhs = false;
                } ورنہ {
                    puts("I(");
                }
                iterate_And_اگلا_Const_Null(selp->lsbp());
                puts(", ");
                iterate_And_اگلا_Const_Null(selp->fromp());
                اگر (rhs) puts(", ");
            } ورنہ {
                putnbs(nodep, "VL_ASSIGNSEL_");
                emitIQW(selp->fromp());
                emitIQW(nodep->rhsp());
                puts("(");
                putns(selp->fromp(), cvt_مقصد_Str(selp->fromp()->چوڑائی_Min()) + ", ");
                puts(cvt_مقصد_Str(nodep->چوڑائی_Min()) + ", ");
                iterate_And_اگلا_Const_Null(selp->lsbp());
                puts(", ");
                iterate_And_اگلا_Const_Null(selp->fromp());
                puts(", ");
            }
        } ورنہ اگر (ثابت AstGetcRefN* ثابت selp = VN_CAST(nodep->lhsp(), GetcRefN)) {
            iterate_And_اگلا_Const_Null(selp->lhsp());
            puts(" = ");
            putnbs(selp, "VL_PUTC_N(");
            iterate_And_اگلا_Const_Null(selp->lhsp());
            puts(", ");
            iterate_And_اگلا_Const_Null(selp->rhsp());
            puts(", ");
        } ورنہ اگر (AstVar* ثابت varp = AstVar::scVarRecurse(nodep->lhsp())) {
            putnbs(varp, "VL_ASSIGN_");  // Set a systemC variable
            emitScIQW(varp);
            emitIQW(nodep);
            puts("(");
            puts(cvt_مقصد_Str(nodep->چوڑائی_Min()) + ", ");
            iterate_And_اگلا_Const_Null(nodep->lhsp());
            puts(", ");
        } ورنہ اگر (AstVar* ثابت varp = AstVar::scVarRecurse(nodep->rhsp())) {
            putnbs(varp, "VL_ASSIGN_");  // Get a systemC variable
            emitIQW(nodep);
            emitScIQW(varp);
            puts("(");
            puts(cvt_مقصد_Str(nodep->چوڑائی_Min()) + ", ");
            iterate_And_اگلا_Const_Null(nodep->lhsp());
            puts(", ");
        } ورنہ اگر (ثابت Ast_Cvt_Packed_مقصد_Array* ثابت castp
                   = VN_CAST(nodep->rhsp(), Cvt_Packed_مقصد_Array)) {
            putns(castp, "VL_UNPACK_");
            emitRU(nodep);
            emitIQW(nodep->dtypep()->subDTypep());
            puts("_");
            emitRU(castp->fromp());
            emitIQW(castp->fromp());
            puts("(");
            putns(castp->dtypep(), cvt_مقصد_Str(castp->dtypep()->subDTypep()->چوڑائی_Min()));
            puts(", ");
            puts(cvt_مقصد_Str(castp->fromp()->چوڑائی_Min()));
            puts(", ");
            iterate_And_اگلا_Const_Null(nodep->lhsp());
            puts(", ");
            rhs = false;
            iterate_And_اگلا_Const_Null(castp->fromp());
            // Descending unpacked dest: reverse after unpack
            اگر (unpackDtp && !unpackDtp->declRange().ascending()) الٹ_Unpack = true;
        } ورنہ اگر (ثابت Ast_Cvt_Array_مقصد_Array* ثابت castp
                   = VN_CAST(nodep->rhsp(), Cvt_Array_مقصد_Array)) {
            اگر (castp->الٹ()) {
                putns(castp, "VL_REVCOPY_Q(");
            } ورنہ {
                putns(castp, "VL_COPY_Q(");
            }
            iterate_And_اگلا_Const_Null(nodep->lhsp());
            puts(", ");
            rhs = false;
            iterate_And_اگلا_Const_Null(castp->fromp());
            puts(", ");
            puts(cvt_مقصد_Str(castp->بلاک_سائز()));
            puts(", ");
            puts(cvt_مقصد_Str(castp->src_Element_بٹ()));
            puts(", ");
            puts(cvt_مقصد_Str(castp->dst_Element_بٹ()));
        } ورنہ اگر (nodep->isWide() && VN_IS(nodep->lhsp(), VarRef)  //
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
        } ورنہ اگر (nodep->isWide() && !unpackDtp && !VN_IS(nodep->rhsp(), Const)) {
            putnbs(nodep, "VL_ASSIGN_W(");
            puts(cvt_مقصد_Str(nodep->چوڑائی_Min()) + ", ");
            iterate_And_اگلا_Const_Null(nodep->lhsp());
            puts(", ");
        } ورنہ اگر (VN_IS(nodep->lhsp()->dtypep()->skipRefp(), قطار_DType)
                   && (VN_IS(nodep->rhsp(), دھارا_L) || VN_IS(nodep->lhsp(), دھارا_L)
                       || VN_IS(nodep->rhsp(), دھارا_R) || VN_IS(nodep->lhsp(), دھارا_R)
                       || VN_IS(nodep->rhsp(), دھارا_R))) {
            //if either side is streamL or streamR don't emit lhsp everything will be passed by
            //reference

            paren = false;

        } ورنہ {
            paren = false;
            iterate_And_اگلا_Const_Null(nodep->lhsp());
            puts(" ");
            ofp()->بلاک_Inc();
            decind = true;
            اگر (!VN_IS(nodep->rhsp(), Const)) ofp()->putBreak();
            putns(nodep, "= ");
        }
        اگر (rhs) iterate_And_اگلا_Const_Null(nodep->rhsp());
        اگر (paren) puts(")");
        اگر (decind) ofp()->بلاک_Dec();
        puts(";\n");
        اگر (الٹ_Unpack) {
            puts("VL_UNPACK_REVERSED(");
            iterate_And_اگلا_Const_Null(nodep->lhsp());
            puts(");\n");
        }
    }
    خالی دورہ(AstAssocSel* nodep) override {
        iterate_And_اگلا_Const_Null(nodep->fromp());
        ثابت std::string atFunc = nodep->isLValue() ? ".atWrite(" : ".at(";
        putnbs(nodep, atFunc);
        AstAssocArrayDType* ثابت adtypep
            = VN_AS(nodep->fromp()->dtypep()->skipRefp(), AssocArrayDType);
        UASSERT_OBJ(adtypep, nodep, "Associative select on non-associative type");
        iterate_And_اگلا_Const_Null(nodep->bitp());
        puts(")");
    }
    خالی دورہ(AstWildcardSel* nodep) override {
        iterate_And_اگلا_Const_Null(nodep->fromp());
        ثابت std::string atFunc = nodep->isLValue() ? ".atWrite(" : ".at(";
        putnbs(nodep, atFunc);
        AstWildcardArrayDType* ثابت adtypep
            = VN_AS(nodep->fromp()->dtypep()->skipRefp(), WildcardArrayDType);
        UASSERT_OBJ(adtypep, nodep, "Wildcard select on non-wildcard-associative type");
        iterate_And_اگلا_Const_Null(nodep->bitp());
        puts(")");
    }
    خالی دورہ(AstCCall* nodep) override {
        ثابت AstCFunc* ثابت funcp = nodep->funcp();
        ثابت Ast_گرہ_Module* ثابت funcModp = EmitCParentModule::get(funcp);
        putnbs(nodep, "");
        اگر (funcp->dpiImportPrototype()) {
            // Calling DPI import
            putns(nodep, funcp->name());
        } ورنہ اگر (funcp->is_Proper_طریقہ() && funcp->isStatic()) {
            // Call static method via the containing class
            putns(funcModp, EmitCUtil::سابقہ_Name_Protect(funcModp) + "::");
            putns(nodep, funcp->nameProtect());
        } ورنہ اگر (nodep->superReference()) {
            // Calling superclass method
            putns(funcModp, EmitCUtil::سابقہ_Name_Protect(funcModp) + "::");
            putns(nodep, funcp->nameProtect());
        } ورنہ اگر (funcp->isLoose()) {
            // Calling loose method
            putns(nodep, funcNameProtect(funcp));
        } ورنہ {
            // Calling regular method/function
            اگر (!nodep->self_اشارہ_گر().is_خالی_خانے()) {
                emitDereference(nodep, nodep->self_اشارہ_گر_Protect(m_useSelfForThis));
            }
            putns(nodep, funcp->nameProtect());
        }
        emit_CCall_دلائل(nodep, nodep->self_اشارہ_گر_Protect(m_useSelfForThis), m_cfuncp->needProcess());
    }
    خالی دورہ(AstCMethodCall* nodep) override {
        ثابت AstCFunc* ثابت funcp = nodep->funcp();
        UASSERT_OBJ(!funcp->isLoose(), nodep, "Loose method called via AstCMethodCall");
        iterateConst(nodep->fromp());
        putnbs(nodep, "->");
        اگر (nodep->superReference()) {
            // Super call, so the base implementation is used rather than the override
            ثابت Ast_گرہ_Module* ثابت funcModp = EmitCParentModule::get(funcp);
            putns(funcModp, EmitCUtil::سابقہ_Name_Protect(funcModp) + "::");
        }
        putns(funcp, funcp->nameProtect());
        emit_CCall_دلائل(nodep, "", m_cfuncp->needProcess());
    }
    خالی دورہ(AstCAwait* nodep) override {
        putns(nodep, "co_await ");
        iterateConst(nodep->exprp());
        puts(";\n");
    }
    خالی دورہ(AstCNew* nodep) override {
        اگر (VN_IS(nodep->dtypep(), VoidDType)) {  // super.new case
            putsDecoration(nodep, "/*super.new*/");
            واپس;
        }
        // assignment case;
        putns(nodep, "VL_NEW(" + EmitCUtil::سابقہ_Name_Protect(nodep->dtypep()) + ", "
                         + optionalProcArg(nodep->dtypep()) + "vlSymsp");
        put_Comma_Iterate_اگلا(nodep->argsp(), true);
        puts(")");
    }
    خالی دورہ(AstCMethodHard* nodep) override {
        iterateConst(nodep->fromp());
        putns(nodep, nodep->usePtr() ? "->" : ".");
        putns(nodep, nodep->name());
        اگر (nodep->طریقہ() == VCMethod::FORCE_READ_SEL) {
            emitIQW(nodep);
            اگر (nodep->isWide()) puts("<" + cvt_مقصد_Str(nodep->dtypep()->چوڑائی_الفاظ()) + ">");
        } ورنہ اگر (nodep->طریقہ() == VCMethod::ARRAY_قاش) {
            // VlUnpacked::slice<N_Out>(loIdx) - N_Out is the (fixed) result array size
            ثابت AstUnpackArrayDType* ثابت adtypep
                = VN_AS(nodep->dtypep()->skipRefp(), UnpackArrayDType);
            puts("<" + cvt_مقصد_Str(adtypep->elementsConst()) + ">");
        } ورنہ اگر (nodep->طریقہ() == VCMethod::COVERGROUP_جوڑو_COVERPOINT) {
            // The hit-list bound is a template argument of the returned VlCoverpointT<>, and the
            // node's own dtype is that type, so it is the one source of truth for both.
            ثابت AstCoverpointDType* ثابت cpdtypep
                = VN_AS(nodep->dtypep()->skipRefp(), CoverpointDType);
            puts("<" + cvt_مقصد_Str(cpdtypep->کامیابی_Bound()) + ">");
        } ورنہ اگر (nodep->طریقہ() == VCMethod::COVERGROUP_جوڑو_CROSS) {
            ثابت AstCoverCrossDType* ثابت cxdtypep
                = VN_AS(nodep->dtypep()->skipRefp(), CoverCrossDType);
            puts("<" + cxdtypep->cpp_Template_دلائل() + ">");
        }
        puts("(");
        bool comma = false;
        صحیح arg_عدد = 0;
        برائے (Ast_گرہ* subnodep = nodep->pinsp(); subnodep; subnodep = subnodep->nextp()) {
            اگر (comma) puts(", ");
            اگر (nodep->طریقہ() == VCMethod::RANDOMIZER_HARD && arg_عدد == 1) {
                // For RANDOMIZER_HARD's filename argument (2nd arg after constraint),
                // apply protect() similar to VL_STOP to handle --protected flag
                اگر (ثابت AstCExpr* ثابت cexprp = VN_CAST(subnodep, CExpr)) {
                    // Extract filename from the CExpr (which contains "filename")
                    std::string filename;
                    برائے (ثابت Ast_گرہ* textnodep = cexprp->nodesp(); textnodep;
                         textnodep = textnodep->nextp()) {
                        اگر (ثابت Ast_متن* ثابت textp = VN_CAST(textnodep, متن_2)) {
                            filename = textp->متن();
                            توڑو;
                        }
                    }
                    // Remove surrounding quotes if present
                    اگر (filename.سائز() >= 2 && filename.front() == '"'
                        && filename.عقب() == '"') {
                        filename = filename.substr(1, filename.سائز() - 2);
                    }
                    // Emit with protect()
                    putsQuoted(protect(filename));
                } ورنہ {
                    iterateConst(subnodep);
                }
            } ورنہ {
                iterateConst(subnodep);
            }
            comma = true;
            arg_عدد++;
        }
        اگر (nodep->withp()) {
            اگر (comma) puts(", ");
            iterateConst(nodep->withp());
        }
        puts(")");
    }
    خالی دورہ(AstCFuncHard* nodep) override {
        putns(nodep, nodep->name());
        puts("(");
        put_Comma_Iterate_اگلا(nodep->pinsp());
        puts(")");
    }
    خالی دورہ(AstLambdaArgRef* nodep) override { putbs(nodep->nameProtect()); }
    خالی دورہ(AstWith* nodep) override {
        // With uses a C++11 lambda
        putnbs(nodep, "[&](");
        اگر (خودکار* ثابت argrefp = nodep->اشاریہ_Arg_Refp()) {
            putnbs(argrefp, argrefp->dtypep()->cType(argrefp->nameProtect(), false, false));
            puts(", ");
        }
        اگر (خودکار* ثابت argrefp = nodep->قدر_Arg_Refp()) {
            putnbs(argrefp, argrefp->dtypep()->cType(argrefp->nameProtect(), false, false));
        }
        puts(") -> ");
        putnbs(nodep, nodep->dtypep()->cType("", false, false));
        puts(" {\n");
        VL_RESTORER(m_createdScopeHash);
        iterate_And_اگلا_Const_Null(nodep->exprp());
        puts("}\n");
    }
    خالی دورہ(AstCase* nodep) override {  // LCOV_EXCL_LINE
        // In V3Case...
        nodep->v3fatalSrc("Case statements should have been reduced out");
    }
    خالی دورہ(AstComment* nodep) override {
        string at;
        اگر (nodep->showAt()) {
            at = " at " + nodep->fileline()->ascii();
            // If protecting, passthru less information about the design
            اگر (!v3Global.opt.protectIds()) واپس;
        }
        اگر (!(nodep->protect() && v3Global.opt.protectIds())) {
            putsDecoration(nodep, "// "s + nodep->name() + at + "\n");
        }
        iterateChildrenConst(nodep);
    }
    خالی دورہ(AstCoverOtherDecl* nodep) override {
        emit_Cover_Other_Decl_داخل_کرو(nodep);
        اگر (coverage_استعمال_کرتا_Local_شمار(nodep)) {
            // Some task bodies are cloned into a caller module where the
            // original instance-local counter is not addressable. Those clones
            // fall back to the global bin number, so register that slot too.
            emit_Cover_Other_Decl_داخل_کرو(nodep, true);
        }
    }
    خالی دورہ(AstCoverToggleDecl* nodep) override {
        putns(nodep, "vlSelf->__vlCoverToggleInsert(");  // As Declared in emitCoverageDecl
        puts(cvt_مقصد_Str(nodep->range().دایاں()));
        puts(", ");
        puts(cvt_مقصد_Str(nodep->range().بایاں()));
        puts(", ");
        puts(cvt_مقصد_Str(nodep->range().ranged()));
        puts(", ");
        putCoverageArray(nodep);
        puts(" + ");
        puts(cvt_مقصد_Str(coverage_Bin_عدد(nodep)));
        // first controls whether duplicate module instances are collapsed in
        // the default coverage view. The following flag says whether countp
        // points to an object-local counter; only those counters must be kept
        // real for later instances so forcePerInstance can split hierarchy.
        puts(", first");  // Enable, passed from __Vconfigure parameter
        puts(", ");
        puts(coverage_استعمال_کرتا_Local_شمار(nodep) ? "true" : "false");
        puts(", ");
        putsQuoted(protect(nodep->fileline()->filename()));
        puts(", ");
        puts(cvt_مقصد_Str(nodep->fileline()->lineno()));
        puts(", ");
        puts(cvt_مقصد_Str(nodep->fileline()->پہلا_عمود()));
        puts(", ");
        putsQuoted((!nodep->hier().خالی_خانے() ? "." : "")
                   + VIdProtect::protect_الفاظ_If(nodep->hier(), nodep->protect()));
        puts(", ");
        putsQuoted(VIdProtect::protect_الفاظ_If(nodep->صفحہ(), nodep->protect()));
        puts(", ");
        putsQuoted(VIdProtect::protect_الفاظ_If(nodep->comment(), nodep->protect()));
        puts(");\n");
    }
    خالی دورہ(AstCoverInc* nodep) override {
        اگر (VN_IS(nodep->declp(), CoverOtherDecl)) {
            اگر (v3Global.opt.دھاگے() > 1) {
                putns(nodep, "");
                putCoverageArray(nodep->declp());
                puts("[");
                puts(cvt_مقصد_Str(coverage_Bin_عدد(nodep->declp())));
                puts("].fetch_add(1, std::memory_order_relaxed);\n");
            } ورنہ {
                putns(nodep, "++(");
                putCoverageArray(nodep->declp());
                puts("[");
                puts(cvt_مقصد_Str(coverage_Bin_عدد(nodep->declp())));
                puts("]);\n");
            }
        } ورنہ {
            puts("VL_COV_TOGGLE_CHG_");
            اگر (v3Global.opt.دھاگے() > 1) {
                puts("MT_");
            } ورنہ {
                puts("ST_");
            }
            emitIQW(nodep->toggleExprp());
            puts("(");
            // Each bit of variable has 2 counters, so the size of variable is half of the size of
            // coverpoint
            puts(cvt_مقصد_Str(nodep->declp()->سائز() / 2));
            puts(", ");
            // Toggle update uses the same object-local counter array that
            // __vlCoverToggleInsert registered.
            putCoverageArray(nodep->declp());
            puts(" + ");
            puts(cvt_مقصد_Str(coverage_Bin_عدد(nodep->declp())));
            puts(", ");
            iterateConst(nodep->toggleExprp());
            puts(", ");
            iterateConst(nodep->toggleCovExprp());
            puts(");\n");
        }
    }
    خالی دورہ(AstDisableFork* nodep) override { putns(nodep, "vlProcess->disableFork();\n"); }
    خالی دورہ(AstCReturn* nodep) override {
        putns(nodep, "return (");
        iterate_And_اگلا_Const_Null(nodep->lhsp());
        puts(");\n");
    }
    خالی دورہ(AstDisplay* nodep) override {
        string متن = nodep->fmtp()->متن();
        اگر (nodep->جوڑو_Newline()) متن += "\n";
        display_گرہ(nodep, nodep->fmtp(), متن, nodep->fmtp()->exprsp(), false);
    }
    خالی دورہ(AstDumpCtl* nodep) override {
        انتخاب (nodep->ctlType()) {
        حالت VDumpCtlType::FILE:
            putns(nodep, "vlSymsp->_vm_contextp__->dumpfile(");
            iterateConst(nodep->exprp());
            puts(");\n");
            توڑو;
        حالت VDumpCtlType::VARS:
            // We ignore number of levels to dump in exprp()
            اگر (v3Global.opt.trace()) {
                putns(nodep, "vlSymsp->_traceDumpOpen();\n");
            } ورنہ {
                putns(nodep, "VL_PRINTF_MT(\"-Info: ");
                puts(V3_برآمد_Formatter::quoteNameControls(protect(nodep->fileline()->filename())));
                puts(":");
                puts(cvt_مقصد_Str(nodep->fileline()->lineno()));
                puts(": $dumpvar ignored, as Verilated without --trace");
                puts("\\n\");\n");
            }
            توڑو;
        حالت VDumpCtlType::ALL:
            // $dumpall currently ignored
            توڑو;
        حالت VDumpCtlType::FLUSH:
            // $dumpall currently ignored; would need rework of VCD single thread,
            // or flag we pass-through to next eval() iteration
            توڑو;
        حالت VDumpCtlType::حد_بندی_2:
            // $dumplimit currently ignored
            توڑو;
        حالت VDumpCtlType::OFF:
            // Currently ignored as both Vcd and Fst do not support them, as would need "X" dump
            توڑو;
        حالت VDumpCtlType::ON:
            // Currently ignored as $dumpoff is also ignored
            توڑو;
        طےشدہ: nodep->v3fatalSrc("Bad case, unexpected " << nodep->ctlType().ascii());
        }
    }
    خالی دورہ(AstScopeName* nodep) override {
        // For use under AstCCalls for dpiImports.  ScopeNames under
        // displays are handled in AstDisplay
        اگر (!nodep->dpiExport()) {
            // this is where the DPI import context scope is set
            ثابت string scope = nodep->scopeDpiName();
            putnbs(nodep, "(vlSymsp->" + protect("__Vscopep_" + scope) + ")");
        }
    }
    خالی دورہ(AstSFormat* nodep) override {
        display_گرہ(nodep, nodep->fmtp(), nodep->fmtp()->متن(), nodep->fmtp()->exprsp(), false);
    }
    خالی دورہ(AstSFormatF* nodep) override {
        display_گرہ(nodep, nodep, nodep->متن(), nodep->exprsp(), false);
    }
    خالی دورہ(AstFScanF* nodep) override {
        display_گرہ(nodep, nullptr, nodep->متن(), nodep->exprsp(), true);
    }
    خالی دورہ(AstSScanF* nodep) override {
        display_گرہ(nodep, nullptr, nodep->متن(), nodep->exprsp(), true);
    }
    خالی دورہ(Ast_قدر_Plus_دلائل* nodep) override {
        putns(nodep, "VL_VALUEPLUSARGS_IN");
        emitIQW(nodep->outp());
        puts("(");
        puts(cvt_مقصد_Str(nodep->outp()->چوڑائی_Min()));
        puts(", ");
        iterateConst(nodep->searchp());
        puts(", ");
        putbs("");
        iterate_And_اگلا_Const_Null(nodep->outp());
        puts(")");
    }
    خالی دورہ(Ast_Test_Plus_دلائل* nodep) override {
        putns(nodep, "VL_TESTPLUSARGS_I(");
        iterateConst(nodep->searchp());
        puts(")");
    }
    خالی دورہ(AstFError* nodep) override {
        putns(nodep, "VL_FERROR_I");
        puts(nodep->strp()->isString() ? "N(" : "W(");
        iterate_And_اگلا_Const_Null(nodep->filep());
        putbs(", ");
        اگر (nodep->strp()->isWide()) {
            puts(cvt_مقصد_Str(nodep->strp()->چوڑائی_الفاظ()));
            putbs(", ");
        }
        iterate_And_اگلا_Const_Null(nodep->strp());
        puts(")");
    }
    خالی دورہ(AstFGetS* nodep) override {
        emitOpName(nodep, nodep->emitC(), nodep->strgp(), nodep->filep(), nullptr);
    }
    خالی دورہ(AstFOpen* nodep) override {
        putns(nodep, "VL_FOPEN_NN(");
        iterateConst(nodep->filenamep());
        putbs(", ");
        iterateConst(nodep->modep());
        puts(");\n");
    }
    خالی دورہ(AstFOpenMcd* nodep) override {
        putns(nodep, "VL_FOPEN_MCD_N(");
        iterateConst(nodep->filenamep());
        puts(");\n");
    }
    خالی دورہ(Ast_گرہ_Read_Write_Mem* nodep) override {
        putns(nodep, nodep->cFuncPrefixp());
        puts("N(");
        puts(nodep->isHex() ? "true" : "false");
        putbs(", ");
        // Need real storage width
        puts(cvt_مقصد_Str(nodep->memp()->dtypep()->skipRefp()->subDTypep()->چوڑائی_Min()));
        uint32_t array_lo = 0;
        bool memUsePtr = true;
        {
            ثابت AstVarRef* ثابت varrefp = VN_CAST(nodep->memp(), VarRef);
            اگر (!varrefp) {
                nodep->v3error(nodep->verilogKwd() << " loading non-variable");
            } ورنہ اگر (VN_IS(varrefp->varp()->dtypeSkipRefp(), AssocArrayDType)
                       || VN_IS(varrefp->varp()->dtypeSkipRefp(), DynArrayDType)) {
                // nodep->memp() below will when verilated code is compiled create a C++ template
                memUsePtr = false;
            } ورنہ اگر (ثابت AstUnpackArrayDType* ثابت adtypep
                       = VN_CAST(varrefp->varp()->dtypeSkipRefp(), UnpackArrayDType)) {
                putbs(", ");
                puts(cvt_مقصد_Str(varrefp->varp()->dtypep()->arrayUnpackedElements()));
                array_lo = adtypep->lo();
                putbs(", ");
                puts(cvt_مقصد_Str(array_lo));
            } ورنہ {
                nodep->v3error(
                    nodep->verilogKwd()
                    << " loading other than unpacked/dynamic/associative-array variable");
            }
        }
        putbs(", ");
        iterateConst(nodep->filenamep());
        putbs(", ");
        {
            اگر (memUsePtr) puts(" &(");
            iterate_And_اگلا_Const_Null(nodep->memp());
            اگر (memUsePtr) puts(")");
        }
        putbs(", ");
        اگر (nodep->lsbp()) {
            iterate_And_اگلا_Const_Null(nodep->lsbp());
        } ورنہ {
            puts(cvt_مقصد_Str(array_lo));
        }
        putbs(", ");
        اگر (nodep->msbp()) {
            iterate_And_اگلا_Const_Null(nodep->msbp());
        } ورنہ {
            puts("~0ULL");
        }
        puts(");\n");
    }
    خالی دورہ(AstFClose* nodep) override {
        putns(nodep, "VL_FCLOSE_I(");
        iterate_And_اگلا_Const_Null(nodep->filep());
        puts("); ");
    }
    خالی دورہ(AstFFlush* nodep) override {
        اگر (!nodep->filep()) {
            putns(nodep, "VL_FFLUSH_MT();");
        } ورنہ {
            putns(nodep, "VL_FFLUSH_I(");
            iterate_And_اگلا_Const_Null(nodep->filep());
            puts(");\n");
        }
    }
    خالی دورہ(AstFSeek* nodep) override {
        putns(nodep, "(VL_FSEEK_I(");
        iterate_And_اگلا_Const_Null(nodep->filep());
        puts(", ");
        iterate_And_اگلا_Const_Null(nodep->انحراف());
        puts(", ");
        iterate_And_اگلا_Const_Null(nodep->operation());
        puts(") == -1 ? -1 : 0)");
    }
    خالی دورہ(AstFTell* nodep) override {
        putns(nodep, "VL_FTELL_I(");
        iterate_And_اگلا_Const_Null(nodep->filep());
        puts(")");
    }
    خالی دورہ(AstFRewind* nodep) override {
        putns(nodep, "(VL_FSEEK_I(");
        iterate_And_اگلا_Const_Null(nodep->filep());
        puts(", 0, 0) == -1 ? -1 : 0)");
    }
    خالی دورہ(AstFRead* nodep) override {
        putns(nodep, "VL_FREAD_I(");
        puts(cvt_مقصد_Str(nodep->memp()->چوڑائی_Min()));  // Need real storage width
        putbs(", ");
        uint32_t array_lo = 0;
        uint32_t array_سائز = 0;
        {
            ثابت AstVarRef* ثابت varrefp = VN_CAST(nodep->memp(), VarRef);
            اگر (!varrefp) {
                nodep->v3error(nodep->verilogKwd() << " loading non-variable");
            } ورنہ اگر (VN_CAST(varrefp->varp()->dtypeSkipRefp(), BasicDType)) {
            } ورنہ اگر (ثابت AstUnpackArrayDType* ثابت adtypep
                       = VN_CAST(varrefp->varp()->dtypeSkipRefp(), UnpackArrayDType)) {
                array_lo = adtypep->lo();
                array_سائز = adtypep->elementsConst();
            } ورنہ {
                nodep->v3error(nodep->verilogKwd()
                               << " loading other than unpacked-array variable");
            }
        }
        puts(cvt_مقصد_Str(array_lo));
        putbs(", ");
        puts(cvt_مقصد_Str(array_سائز));
        putbs(", ");
        puts("&(");
        iterate_And_اگلا_Const_Null(nodep->memp());
        puts(")");
        putbs(", ");
        iterate_And_اگلا_Const_Null(nodep->filep());
        putbs(", ");
        اگر (nodep->startp()) {
            iterate_And_اگلا_Const_Null(nodep->startp());
        } ورنہ {
            puts(cvt_مقصد_Str(array_lo));
        }
        putbs(", ");
        اگر (nodep->countp()) {
            iterate_And_اگلا_Const_Null(nodep->countp());
        } ورنہ {
            puts(cvt_مقصد_Str(array_سائز));
        }
        puts(")");
    }
    خالی دورہ(Ast_ڈھیر_Trace_F* nodep) override { putns(nodep, "VL_STACKTRACE_N()"); }
    خالی دورہ(Ast_ڈھیر_Trace_T* nodep) override { putns(nodep, "VL_STACKTRACE();\n"); }
    خالی دورہ(AstSystemT* nodep) override {
        putns(nodep, "(void)VL_SYSTEM_I");
        emitIQW(nodep->lhsp());
        puts("(");
        اگر (nodep->lhsp()->isWide()) {
            puts(cvt_مقصد_Str(nodep->lhsp()->چوڑائی_الفاظ()));
            putbs(", ");
        }
        iterate_And_اگلا_Const_Null(nodep->lhsp());
        puts(");\n");
    }
    خالی دورہ(AstSystemF* nodep) override {
        putns(nodep, "VL_SYSTEM_I");
        emitIQW(nodep->lhsp());
        puts("(");
        اگر (nodep->lhsp()->isWide()) {
            puts(cvt_مقصد_Str(nodep->lhsp()->چوڑائی_الفاظ()));
            putbs(", ");
        }
        iterate_And_اگلا_Const_Null(nodep->lhsp());
        puts(")");
    }
    خالی دورہ(Ast_Stmt_اظہاریہ* nodep) override {
        اگر (VN_IS(nodep->exprp()->dtypep()->skipRefp(), VoidDType)) {
            putns(nodep, "");
        } ورنہ {
            putns(nodep, "std::ignore = ");
        }
        iterateConst(nodep->exprp());
        puts(";\n");
    }
    خالی دورہ(Ast_Jump_بلاک* nodep) override {
        // Allocate label number
        ثابت سائز_t n = m_labelNumbers.سائز();
        ثابت bool new_اندراج = m_labelNumbers.emplace(nodep, n).دوسرا;
        UASSERT_OBJ(new_اندراج, nodep, "AstJumpBlock visited twide");
        // Emit
        putns(nodep, "{\n");  // Make it visually obvious label jumps outside these
        VL_RESTORER(m_createdScopeHash);
        iterate_And_اگلا_Const_Null(nodep->stmtsp());
        puts("__Vlabel" + std::مقصد_string(n) + ": ;\n");
        puts("}\n");
    }
    خالی دورہ(AstJumpGo* nodep) override {
        // Retrieve target label number - must already exist (from enclosing AstJumpBlock)
        ثابت سائز_t n = m_labelNumbers.at(nodep->blockp());
        // Emit
        putns(nodep, "goto __Vlabel" + std::مقصد_string(n) + ";\n");
    }
    خالی دورہ(AstLoop* nodep) override {
        UASSERT_OBJ(!nodep->contsp(), nodep, "'contsp' only used before LinkJump");
        VL_RESTORER(m_createdScopeHash);
        // Special case when the AstLoopTest is first for output readability
        اگر (AstLoopTest* ثابت testp = VN_CAST(nodep->stmtsp(), LoopTest)) {
            putns(nodep, "while (");
            iterateConst(testp->condp());
            puts(") {\n");
            iterate_And_اگلا_Const_Null(testp->nextp());
            puts("}\n");
            واپس;
        }
        // Special case when the AstLoopTest is last for output readability
        اگر (Ast_گرہ* lastp = nodep->stmtsp()) {
            جب (Ast_گرہ* ثابت nextp = lastp->nextp()) lastp = nextp;
            اگر (AstLoopTest* ثابت testp = VN_CAST(lastp, LoopTest)) {
                putns(nodep, "do {\n");
                برائے (Ast_گرہ* p = nodep->stmtsp(); p != lastp; p = p->nextp()) iterateConst(p);
                puts("} while (");
                iterateConst(testp->condp());
                puts(");\n");
                واپس;
            }
        }
        // Emit generic case directly
        putns(nodep, "while (true) {\n");
        iterate_And_اگلا_Const_Null(nodep->stmtsp());
        puts("}\n");
    }
    خالی دورہ(AstLoopTest* nodep) override {
        VL_RESTORER(m_createdScopeHash);
        putns(nodep, "if (!(");
        iterate_And_اگلا_Const_Null(nodep->condp());
        puts(")) break;\n");
    }
    خالی دورہ(AstCLocalScope* nodep) override {
        putns(nodep, "{\n");
        VL_RESTORER(m_createdScopeHash);
        iterate_And_اگلا_Const_Null(nodep->stmtsp());
        puts("}\n");
    }
    خالی دورہ(Ast_گرہ_If* nodep) override {
        putns(nodep, "if (");
        اگر (!nodep->branchPred().unknown()) {
            puts(nodep->branchPred().ascii());
            puts("((");  // Two parens, so that VL_UNLIKELY((class<foo,bar>)) works
        }
        iterate_And_اگلا_Const_Null(nodep->condp());
        اگر (!nodep->branchPred().unknown()) puts("))");
        puts(") {\n");
        {
            VL_RESTORER(m_createdScopeHash);
            iterate_And_اگلا_Const_Null(nodep->thensp());
        }
        puts("}");
        اگر (!nodep->elsesp()) {
            puts("\n");
        } ورنہ {
            اگر (VN_IS(nodep->elsesp(), گرہ_If) && !nodep->elsesp()->nextp()) {
                puts(" else ");
                iterate_And_اگلا_Const_Null(nodep->elsesp());
            } ورنہ {
                VL_RESTORER(m_createdScopeHash);
                puts(" else {\n");
                iterate_And_اگلا_Const_Null(nodep->elsesp());
                puts("}\n");
            }
        }
    }
    خالی دورہ(Ast_اظہاریہ_Stmt* nodep) override {
        VL_RESTORER(m_createdScopeHash);
        ثابت bool شامل_ہے_Await = nodep->exists([](AstCAwait*) -> bool { واپس true; });
        اگر (شامل_ہے_Await) {
            UASSERT_OBJ(m_cfuncp && m_cfuncp->isCoroutine(), nodep,
                        "AstExprStmt with CAwait must be in coroutine");
            putnbs(nodep, "(co_await ([&]() -> VlCoroutine {\n");
            iterate_And_اگلا_Const_Null(nodep->stmtsp());
            puts("co_return;\n");
            اگر (!nodep->has_نتیجہ()) {
                puts("}()))");
                واپس;
            }
            puts("}()), ");
            iterate_And_اگلا_Const_Null(nodep->resultp());
            puts(")");
            واپس;
        }
        // GCC allows compound statements in expressions, but this is not standard.
        // So we use an immediate-evaluation lambda and comma operator
        putnbs(nodep, "([&]() {\n");
        اگر (!nodep->has_نتیجہ()) {
            iterate_And_اگلا_Const_Null(nodep->stmtsp());
            puts("}())");
            واپس;
        }
        iterate_And_اگلا_Const_Null(nodep->stmtsp());
        puts("}(), ");
        iterate_And_اگلا_Const_Null(nodep->resultp());
        puts(")");
    }
    خالی دورہ(AstStop* nodep) override {
        putns(nodep, "VL_STOP_MT(");
        putsQuoted(protect(nodep->fileline()->filename()));
        puts(", ");
        puts(cvt_مقصد_Str(nodep->fileline()->lineno()));
        puts(", \"\"");
        اگر (nodep->isFatal()) puts(", false");
        puts(");\n");
    }
    خالی دورہ(Ast_اختتام* nodep) override {
        // Disable all the forks so they don't operate after simulation is finished.
        اگر (m_cfuncp && m_cfuncp->needProcess()) putns(nodep, "vlProcess->disableFork();\n");
        putns(nodep, "VL_FINISH_MT(");
        putsQuoted(protect(nodep->fileline()->filename()));
        puts(", ");
        puts(cvt_مقصد_Str(nodep->fileline()->lineno()));
        puts(", \"\");\n");
    }
    خالی دورہ(Ast_اختتام_Fork* nodep) override {
        // Disable all the forks so they don't operate after simulation is finished.
        اگر (m_cfuncp && m_cfuncp->needProcess()) putns(nodep, "vlProcess->disableFork();\n");
        putns(nodep, "VL_FINISH_MT(");
        putsQuoted(protect(nodep->fileline()->filename()));
        puts(", ");
        puts(cvt_مقصد_Str(nodep->fileline()->lineno()));
        puts(", \"\");\n");
        اگر (m_cfuncp->isCoroutine()) {
            putns(nodep, "co_return;\n");
        } ورنہ {
            putns(nodep, "return;\n");
        }
    }
    خالی دورہ(Ast_Print_وقت_Scale* nodep) override {
        putns(nodep, "VL_PRINTTIMESCALE(");
        putsQuoted(protect(nodep->prettyName()));
        puts(", ");
        putsQuoted(nodep->timeunit().ascii());
        puts(", vlSymsp->_vm_contextp__);\n");
    }
    خالی دورہ(AstRand* nodep) override {
        emitOpName(nodep, nodep->emitC(), nodep->seedp(), nullptr, nullptr);
    }
    خالی دورہ(AstRandRNG* nodep) override {
        emitOpName(nodep, nodep->emitC(), nullptr, nullptr, nullptr);
    }
    خالی دورہ(Ast_وقت* nodep) override {
        putns(nodep, "VL_TIME_UNITED_Q(");
        UASSERT_OBJ(!nodep->timeunit().isNone(), nodep, "$time has no units");
        ثابت دوہرا وقت_2
            = nodep->timeunit().multiplier() / v3Global.rootp()->timeprecision().multiplier();
        UASSERT_OBJ(وقت_2 >= 1, nodep, "TimeQ is less than 1, will result in division by zero");
        puts(cvt_مقصد_Str(وقت_2));
        puts(")");
    }
    خالی دورہ(Ast_وقت_D* nodep) override {
        putns(nodep, "VL_TIME_UNITED_D(");
        UASSERT_OBJ(!nodep->timeunit().isNone(), nodep, "$realtime has no units");
        puts(cvt_مقصد_Str(nodep->timeunit().multiplier()
                      / v3Global.rootp()->timeprecision().multiplier()));
        puts(")");
    }
    خالی دورہ(Ast_Get_Initial_Random_بیج* nodep) override {
        putns(nodep, "vlSymsp->_vm_contextp__->randSeed()");
    }
    خالی دورہ(Ast_وقت_Format* nodep) override {
        putns(nodep, "VL_TIMEFORMAT_IINI(");
        اگر (nodep->unitsp()) {
            puts("true, ");
            iterate_And_اگلا_Const_Null(nodep->unitsp());
        } ورنہ {
            puts("false, 0");
        }
        puts(", ");
        اگر (nodep->precisionp()) {
            puts("true, ");
            iterate_And_اگلا_Const_Null(nodep->precisionp());
        } ورنہ {
            puts("false, 0");
        }
        puts(", ");
        اگر (nodep->suffixp()) {
            puts("true, ");
            iterate_And_اگلا_Const_Null(nodep->suffixp());
        } ورنہ {
            puts("false, \"\"");
        }
        puts(", ");
        اگر (nodep->widthp()) {
            puts("true, ");
            iterate_And_اگلا_Const_Null(nodep->widthp());
        } ورنہ {
            puts("false, 0");
        }
        puts(", vlSymsp->_vm_contextp__);\n");
    }
    خالی دورہ(Ast_وقت_Precision* nodep) override {
        putns(nodep, "vlSymsp->_vm_contextp__->timeprecision()");
    }

    // Nodes involing AstText
    خالی دورہ(Ast_متن* nodep) override {
        // All Text should be under TextBlock/CStmt/CStmtUser/CExpr/CExprUser
        nodep->v3fatalSrc("Text node in unexpected position");
    }
    خالی دورہ(Ast_متن_بلاک* nodep) override {
        putnbs(nodep, "");
        puts(nodep->سابقہ());
        emit_Nodes_With_متن(nodep->nodesp(), false, true, nodep->separator());
        puts(nodep->suffix());
    }
    خالی دورہ(AstCStmt* nodep) override {
        putnbs(nodep, "");
        emit_Nodes_With_متن(nodep->nodesp(), false, true, "");
        ensure_New_سطر();
    }
    خالی دورہ(AstCExpr* nodep) override {
        putnbs(nodep, "");
        emit_Nodes_With_متن(nodep->nodesp(), false, true, "");
    }
    خالی دورہ(AstCStmtUser* nodep) override {
        putnbs(nodep, "");
        ofp()->putsNoTracking("\n");
        اگر (nodep->fromDollarC() && v3Global.opt.decoration() && !v3Global.opt.protectIds()) {
            ofp()->putsNoTracking("// $c statement at " + nodep->fileline()->ascii() + "\n");
        }
        emit_Nodes_With_متن(nodep->nodesp(), m_useSelfForThis, false, "");
        puts("\n");
    }
    خالی دورہ(AstCExprUser* nodep) override {
        putnbs(nodep, "");
        ofp()->putsNoTracking("\n");
        اگر (/* is always from $c */ v3Global.opt.decoration() && !v3Global.opt.protectIds()) {
            ofp()->putsNoTracking(
                (nodep->is_خالص() ? "// $cpure expression at " : "// $c expression at ")
                + nodep->fileline()->ascii() + "\n");
        }
        emit_Nodes_With_متن(nodep->nodesp(), m_useSelfForThis, false, "");
        puts("\n");
    }

    // Operators
    خالی دورہ(Ast_گرہ_Termop* nodep) override {
        emitOpName(nodep, nodep->emitC(), nullptr, nullptr, nullptr);
    }
    خالی دورہ(Ast_گرہ_Uniop* nodep) override {
        اگر (nodep->emit_Check_Max_الفاظ()
            && (nodep->چوڑائی_الفاظ() > VL_MULS_MAX_الفاظ
                || nodep->lhsp()->چوڑائی_الفاظ() > VL_MULS_MAX_الفاظ)) {
            nodep->v3warn(
                E_UNSUPPORTED,
                "Unsupported: "
                    << nodep->prettyOperatorName() << " operator of " << nodep->چوڑائی()
                    << " bits exceeds hardcoded limit VL_MULS_MAX_WORDS in verilatedos.h");
        }
        اگر (emitSimpleOk(nodep)) {
            putnbs(nodep, "(");
            puts(nodep->emitSimpleOperator());
            puts(" ");
            iterate_And_اگلا_Const_Null(nodep->lhsp());
            puts(")");
        } ورنہ {
            emitOpName(nodep, nodep->emitC(), nodep->lhsp(), nullptr, nullptr);
        }
    }
    خالی دورہ(Ast_گرہ_Biop* nodep) override {
        اگر (nodep->emit_Check_Max_الفاظ() && nodep->چوڑائی_الفاظ() > VL_MULS_MAX_الفاظ) {
            nodep->v3warn(
                E_UNSUPPORTED,
                "Unsupported: "
                    << nodep->prettyOperatorName() << " operator of " << nodep->چوڑائی()
                    << " bits exceeds hardcoded limit VL_MULS_MAX_WORDS in verilatedos.h");
        }
        اگر (emitSimpleOk(nodep)) {
            putnbs(nodep, "(");
            iterate_And_اگلا_Const_Null(nodep->lhsp());
            puts(" ");
            putbs(nodep->emitSimpleOperator());
            puts(" ");
            iterate_And_اگلا_Const_Null(nodep->rhsp());
            puts(")");
        } ورنہ {
            emitOpName(nodep, nodep->emitC(), nodep->lhsp(), nodep->rhsp(), nullptr);
        }
    }
    خالی دورہ(Ast_گرہ_Triop* nodep) override {
        UASSERT_OBJ(!emitSimpleOk(nodep), nodep, "Triop cannot be described in a simple way");
        emitOpName(nodep, nodep->emitC(), nodep->lhsp(), nodep->rhsp(), nodep->thsp());
    }
    خالی دورہ(AstCvtPackString* nodep) override { emitCvtPackStr(nodep->lhsp()); }
    خالی دورہ(AstRedXor* nodep) override {
        اگر (nodep->lhsp()->isWide()) {
            دورہ(static_cast<Ast_گرہ_Uniop*>(nodep));
        } ورنہ {
            ثابت AstVarRef* ثابت vrefp = VN_CAST(nodep->lhsp(), VarRef);
            ثابت صحیح چوڑائی_Pow2 = vrefp ? vrefp->varp()->dtypep()->چوڑائی_Pow2()
                                        : nodep->lhsp()->dtypep()->چوڑائی_Pow2();
            UASSERT_OBJ(چوڑائی_Pow2 > 1, nodep,
                        "Reduction over single bit value should have been folded");
            putnbs(nodep, "VL_REDXOR_");
            puts(cvt_مقصد_Str(چوڑائی_Pow2));
            puts("(");
            iterate_And_اگلا_Const_Null(nodep->lhsp());
            puts(")");
        }
    }
    خالی دورہ(AstCCast* nodep) override {
        // Extending a value of the same word width is just a NOP.
        اگر (ثابت AstClassRefDType* ثابت classDtypep
            = VN_CAST(nodep->dtypep()->skipRefp(), ClassRefDType)) {
            putns(nodep, "(" + classDtypep->cType("", false, false) + ")(");
        } ورنہ اگر (nodep->سائز() <= VL_BYTESIZE) {
            putns(nodep, "(::CData)(");
        } ورنہ اگر (nodep->سائز() <= VL_SHORTSIZE) {
            putns(nodep, "(::SData)(");
        } ورنہ اگر (nodep->سائز() <= VL_IDATASIZE) {
            putns(nodep, "(::IData)(");
        } ورنہ {
            putns(nodep, "(::QData)(");
        }
        iterate_And_اگلا_Const_Null(nodep->lhsp());
        puts(")");
    }
    خالی دورہ(AstCond* nodep) override {
        // Widths match up already, so we'll just use C++'s operator w/o any temps.
        اگر (nodep->thenp()->isWide()) {
            emitOpName(nodep, nodep->emitC(), nodep->condp(), nodep->thenp(), nodep->elsep());
        } ورنہ {
            putnbs(nodep, "(");
            iterate_And_اگلا_Const_Null(nodep->condp());
            putbs(" ? ");
            iterate_And_اگلا_Const_Null(nodep->thenp());
            putbs(" : ");
            iterate_And_اگلا_Const_Null(nodep->elsep());
            puts(")");
        }
    }
    خالی دورہ(Ast_مطابقت_Masked* nodep) override {
        emitOpName(nodep, nodep->emitC(), nodep->lhsp(), nodep->matchp(), nullptr);
    }
    خالی دورہ(AstMemberSel* nodep) override {
        ثابت AstVar* ثابت varp = nodep->varp();
        ثابت bool dereferenceCovergroupRef
            = varp->covergroupRefMember() && nodep->access().isReadOrRW();
        اگر (dereferenceCovergroupRef) putnbs(nodep, "(*");
        iterate_And_اگلا_Const_Null(nodep->fromp());
        putnbs(nodep, "->");
        اگر (varp->isIfaceRef()) {
            // varp is the __Viftop companion (e.g. "tx__Viftop"); use the
            // MemberSel name which matches the cell's C++ member (e.g. "tx").
            puts(nodep->nameProtect());
        } ورنہ {
            puts(varName(varp));
        }
        اگر (dereferenceCovergroupRef) puts(")");
    }
    خالی دورہ(AstStructSel* nodep) override {
        iterate_And_اگلا_Const_Null(nodep->fromp());
        putnbs(nodep, ".");
        puts(nodep->nameProtect());
    }
    خالی دورہ(AstNullCheck* nodep) override {
        putns(nodep, "VL_NULL_CHECK(");
        iterate_And_اگلا_Const_Null(nodep->lhsp());
        puts(", ");
        putsQuoted(protect(nodep->fileline()->filename()));
        puts(", ");
        puts(cvt_مقصد_Str(nodep->fileline()->lineno()));
        puts(")");
    }
    خالی دورہ(AstNewCopy* nodep) override {
        // Polymorphic shallow clone: preserves runtime type via virtual clone()
        // VL_NULL_CHECK enforces null check per IEEE 1800-2023 8.7
        putns(nodep, "VL_NULL_CHECK(");
        اگر (VN_IS(nodep->rhsp(), Const) && VN_AS(nodep->rhsp(), Const)->isNull()) {
            // V3Const folded rhs to null: emit a typed empty ref so VL_NULL_CHECK fires
            ثابت AstClassRefDType* ثابت refDTypep
                = VN_CAST(nodep->dtypep()->skipRefp(), ClassRefDType);
            puts(refDTypep->cType("", false, false) + "{}");
        } ورنہ {
            iterate_And_اگلا_Const_Null(nodep->rhsp());
        }
        puts(", ");
        putsQuoted(protect(nodep->fileline()->filename()));
        puts(", ");
        puts(cvt_مقصد_Str(nodep->fileline()->lineno()));
        puts(").clone(vlSymsp->__Vm_deleter)");
    }
    خالی دورہ(AstSel* nodep) override {
        // Note ASSIGN checks for this on a LHS
        UASSERT_OBJ(nodep->چوڑائی_Min() == nodep->چوڑائی_Const(), nodep, "Width mismatch");
        emitOpName(nodep, nodep->emitC(), nodep->fromp(), nodep->lsbp(), nullptr);
    }
    خالی دورہ(AstReplicate* nodep) override {
        اگر (nodep->srcp()->چوڑائی_Min() == 1 && !nodep->isWide()) {
            UASSERT_OBJ((static_cast<صحیح>(VN_AS(nodep->countp(), Const)->مقصد_UInt())
                         * nodep->srcp()->چوڑائی_Min())
                            == nodep->چوڑائی_Min(),
                        nodep, "Replicate non-constant or width miscomputed");
            putns(nodep, "VL_REPLICATE_");
            emitIQW(nodep);
            puts("OI(");
            اگر (nodep->srcp()) puts(cvt_مقصد_Str(nodep->srcp()->چوڑائی_Min()));
            puts(", ");
            iterate_And_اگلا_Const_Null(nodep->srcp());
            puts(", ");
            iterate_And_اگلا_Const_Null(nodep->countp());
            puts(")");
        } ورنہ {
            emitOpName(nodep, nodep->emitC(), nodep->srcp(), nodep->countp(), nullptr);
        }
    }
    خالی emit_دھارا_R(Ast_دھارا_R* nodep, Ast_گرہ* والد) {
        //TODO: This might need to handle more cases like the visit(AstStreamR) function
        emitOpName(nodep, nodep->emitC(), nodep->lhsp(), nodep->rhsp(), nullptr);
    }
    خالی دورہ(Ast_دھارا_R* nodep) override {
        //The parrent node of our AstStreamR will give just enough info for what streamR should
        //output if nodep->backp() is not the parent then emitStreamR should have been used. throw
        //an error
        bool backp_Is_والد = (nodep->backp()->op1p() == nodep || nodep->backp()->op2p() == nodep);
        UASSERT(backp_Is_والد, "can not find return type for streamR");
        اگر (ثابت Ast_قطار_DType* ثابت qdtp
            = VN_CAST(nodep->backp()->dtypep()->skipRefp(), قطار_DType)) {
            اگر (nodep->lhsp()->isWide()) {
                emitOpName(nodep,
                           "VL_STREAMR_%nq%lq%rq(%lw," + std::مقصد_string(qdtp->subDTypep()->چوڑائی())
                               + ", %P, %li, %ri)",
                           nodep->lhsp(), nodep->rhsp(), nullptr);
            } ورنہ {
                emitOpName(nodep, "VL_STREAMR_%nq%lq%rq(%lw, %P, %li, %ri)", nodep->lhsp(),
                           nodep->rhsp(), nullptr);
            }
        } ورنہ اگر (VN_IS(nodep->lhsp()->dtypep()->skipRefp(), قطار_DType)) {
            اگر (!((nodep->backp()->op1p() && nodep->backp()->op1p()->isWide())
                  || (nodep->backp()->op2p() && nodep->backp()->op2p()->isWide()))) {
                //If our lhsp is a queue make sure we streamR and return the correct type.
                //If either side is wide or the previous node is string type dont use this case
                emitOpName(nodep->backp(), "VL_STREAMR_%nq%lq%rq(%lw, %P, %li, %ri)",
                           nodep->lhsp(), nodep->rhsp(), nullptr);
            } ورنہ {
                emitOpName(nodep, nodep->emitC(), nodep->lhsp(), nodep->rhsp(), nullptr);
            }
        } ورنہ {
            emitOpName(nodep, nodep->emitC(), nodep->lhsp(), nodep->rhsp(), nullptr);
        }
    }
    خالی دورہ(Ast_دھارا_L* nodep) override {
        // Attempt to use a "fast" stream function for slice size = power of 2
        اگر (!nodep->isWide()) {
            ثابت uint32_t isPow2 = VN_AS(nodep->rhsp(), Const)->عدد().گنتی_Ones() == 1;
            ثابت uint32_t قاش_سائز = VN_AS(nodep->rhsp(), Const)->مقصد_UInt();
            اگر (isPow2 && قاش_سائز <= (nodep->isQuad() ? حجم(uint64_t) : حجم(uint32_t))) {
                putns(nodep, "VL_STREAML_FAST_");
                bool استعمال_کرتا_قطار = false;
                Ast_قطار_DType* qtypep
                    = nodep->backp()->op2p()
                          ? VN_CAST(nodep->backp()->op2p()->dtypep()->skipRefp(), قطار_DType)
                          : nullptr;
                اگر (VN_IS(nodep->backp(), تفویض)
                    && qtypep) {  // If we are assigning to a queue then emit the correct symbol
                    puts("R");  // R for queue
                    استعمال_کرتا_قطار = true;
                } ورنہ {
                    emitIQW(nodep);
                }
                emitIQW(nodep->lhsp());
                puts("I(");
                puts(cvt_مقصد_Str(nodep->lhsp()->چوڑائی_Min()));
                puts(", ");
                اگر (استعمال_کرتا_قطار) {
                    iterate_And_اگلا_Const_Null(nodep->backp()->op2p());
                    puts(", ");
                }
                iterate_And_اگلا_Const_Null(nodep->lhsp());
                puts(", ");
                ثابت uint32_t rd_log2 = V3_نمبر::log2b(VN_AS(nodep->rhsp(), Const)->مقصد_UInt());
                puts(cvt_مقصد_Str(rd_log2) + ")");
                واپس;
            }
        }
        اگر (VN_IS(nodep->backp(), تفویض)
            && VN_IS(nodep->backp()->op2p()->dtypep()->skipRefp(), قطار_DType)) {
            صحیح قطار_چوڑائی
                = nodep->backp()->op2p()->dtypep()->subDTypep()->چوڑائی();  //We need to know the
                                                                           //width of both sides
            emitOpName(nodep,
                       "VL_STREAML_%nq%lq%rq(%lw," + std::مقصد_string(قطار_چوڑائی)
                           + ", %P, %li, %ri)",
                       nodep->lhsp(), nodep->rhsp(), nullptr);
        } ورنہ {
            emitOpName(nodep, nodep->emitC(), nodep->lhsp(), nodep->rhsp(), nullptr);
        }
    }
    خالی دورہ(AstCastDynamic* nodep) override {
        putnbs(nodep, "VL_CAST_DYNAMIC(");
        iterate_And_اگلا_Const_Null(nodep->fromp());
        puts(", ");
        iterate_And_اگلا_Const_Null(nodep->چوٹی());
        puts(")");
    }
    خالی دورہ(Ast_گنتی_بٹ* nodep) override {
        putnbs(nodep, "VL_COUNTBITS_");
        emitIQW(nodep->lhsp());
        puts("(");
        puts(cvt_مقصد_Str(nodep->lhsp()->چوڑائی_Min()));
        puts(", ");
        اگر (nodep->lhsp()->isWide()) {
            puts(cvt_مقصد_Str(nodep->lhsp()->چوڑائی_الفاظ()));  // Note argument width, not node width
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
    خالی دورہ(AstTriggered* nodep) override {
        // Test each word of the trigger vector with a mask of the relevant bits
        std::map<uint32_t, uint64_t> لفظ_Masks;
        برائے (ثابت uint32_t اشاریہ : nodep->indices()) {
            لفظ_Masks[VL_BITWORD_Q(اشاریہ)] |= VL_ثنائی_ہندسہ_Q(اشاریہ);
        }
        putns(nodep, "((");
        bool پہلا = true;
        برائے (ثابت خودکار& لفظ_نقاب : لفظ_Masks) {
            اگر (!پہلا) puts("\n| ");
            پہلا = false;
            ثابت bool allOnes = لفظ_نقاب.دوسرا == ~0ULL;
            اگر (!allOnes) puts("(" + V3_نمبر{nodep, 64}.سیٹ_Quad(لفظ_نقاب.دوسرا).emitC() + " & ");
            iterateConst(nodep->trigRefp());
            puts("[" + std::مقصد_string(لفظ_نقاب.پہلا) + "U]");
            اگر (!allOnes) puts(")");
        }
        puts(") != 0ULL)");
    }
    // Terminals
    خالی دورہ(AstVarRef* nodep) override {
        ثابت AstVar* ثابت varp = nodep->varp();

        // Reference to constant pool variable is special
        اگر (varp->const_Pool_اندراج()) {
            putns(nodep, EmitCUtil::constPoolName(varp));
            واپس;
        }

        ثابت Ast_گرہ_Module* ثابت varModp = EmitCParentModule::get(varp);
        ثابت bool dereferenceCovergroupRef
            = varp->covergroupRefMember() && nodep->access().isReadOrRW();
        اگر (dereferenceCovergroupRef) putns(nodep, "(*");
        اگر (VN_IS(varModp, Class) && varModp != m_modp) {
            // Superclass member reference
            putns(nodep, EmitCUtil::سابقہ_Name_Protect(varModp) + "::");
        } ورنہ اگر (varp->isIfaceRef()) {
            putns(nodep, nodep->self_اشارہ_گر_Protect(m_useSelfForThis));
            واپس;
        } ورنہ اگر (!nodep->self_اشارہ_گر().is_خالی_خانے()) {
            emitDereference(nodep, nodep->self_اشارہ_گر_Protect(m_useSelfForThis));
        }
        putns(nodep, varName(varp));
        اگر (dereferenceCovergroupRef) puts(")");
    }
    خالی دورہ(Ast_Addr_کا_CFunc* nodep) override {
        // Note: Can be thought to handle more, but this is all that is needed right now
        ثابت AstCFunc* ثابت funcp = nodep->funcp();
        UASSERT_OBJ(funcp->isLoose(), nodep, "Cannot take address of non-loose method");
        putns(nodep, "&");
        puts(funcNameProtect(funcp));
    }
    خالی دورہ(AstConst* nodep) override {  //
        اگر (m_wide_عارضی_Refp && nodep->isWide()) {
            UASSERT_OBJ(m_wide_عارضی_Refp, nodep, "Wide Constant w/ no temp");
            emitConstantW(nodep, m_wide_عارضی_Refp);
            m_wide_عارضی_Refp = nullptr;  // We used it, fail if set it a second time
        } ورنہ {
            emitConstant(nodep);
        }
    }
    خالی دورہ(Ast_گرہ_Cons* nodep) override { emitValCons(nodep); }
    خالی دورہ(AstThisRef* nodep) override {
        putnbs(nodep, nodep->dtypep()->cType("", false, false));
        puts("{");
        puts(VSelf_اشارہ_گر_متن::replaceThis(m_useSelfForThis, "this"));
        puts("}");
    }
    خالی دورہ(Ast_گرہ_Sel* nodep) override {
        اگر (!VN_IS(nodep, ArraySel) && !VN_IS(nodep, لفظ_Sel)) {
            دورہ(static_cast<Ast_گرہ_Biop*>(nodep));
            واپس;
        }
        // ArraySel or WordSel
        iterate_And_اگلا_Const_Null(nodep->fromp());
        // Special case constant index for readability
        اگر (AstConst* ثابت idxp = VN_CAST(nodep->bitp(), Const)) {
            puts("[" + std::مقصد_string(idxp->مقصد_UInt()) + "U]");
            واپس;
        }
        putbs("[");
        iterate_And_اگلا_Const_Null(nodep->bitp());
        puts("]");
    }

    //
    خالی دورہ(Ast_Exec_گراف* nodep) override {
        // The location of the AstExecGraph within the containing AstCFunc is where we want to
        // invoke the graph and wait for it to complete. The run-time library does just that.
        putns(nodep, "vl_invokeExecGraph(" + EmitCUtil::exec_گراف_Name(nodep) + ", vlSelf, ");
        iterateConst(nodep->triggerp());
        puts(".data());\n");
    }

    // Default
    خالی دورہ(Ast_گرہ* nodep) override {  // LCOV_EXCL_START
        putns(nodep, "\n???? // "s + nodep->prettyTypeName() + "\n");
        iterateChildrenConst(nodep);
        اگر (!v3Global.opt.lintOnly()) {  // An internal problem, so suppress
            nodep->v3fatalSrc("Unknown node type reached emitter: " << nodep->prettyTypeName());
        }
    }  // LCOV_EXCL_STOP

محفوظ:
    EmitCFunc() = طےشدہ;
    ~EmitCFunc() override = طےشدہ;
};

#اگر_ختم  // guard
