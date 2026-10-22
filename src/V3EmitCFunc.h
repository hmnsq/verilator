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
constexpr صحیح EMITC_رقم_CONSTW = 8;

//######################################################################
// Emit lazy forward declarations

class EmitCLazyDecls final : عمومی VNVisitorConst {
    // NODE STATE/TYPES
    // None allowed to support threaded emitting

    // MEMBERS
    std::unordered_set<string> m_emittedManually;  // Set of names already declared manually.
    EmitCBaseVisitorConst& m_emitter;  // For access to file output
    bool m_needs_Blank_سطر = false;  // Emit blank line if any declarations were emitted (cosmetic)
    std::set<Ast_گره*> m_emitted;  // -> in set. Already emitted decl for symbols.

    // METHODS
    bool declaredOnce(Ast_گره* nodep) { بازگشت m_emitted.درج(nodep).second; }

    تهی lazyDeclare(AstCFunc* funcp) {
        // Already declared in this compilation unit
        اگر (!declaredOnce(funcp)) بازگشت;
        // Check if this kind of function is lazily declared
        اگر (!(funcp->isMethod() && funcp->isLoose()) && !funcp->dpiImportPrototype()) بازگشت;
        // Already declared manually
        اگر (m_emittedManually.شمار(funcp->nameProtect())) بازگشت;
        // Needs lazy declaration, emit one
        m_emitter.emitCFuncDecl(funcp, EmitCParentModule::get(funcp), funcp->dpiImportPrototype());
        m_needs_Blank_سطر = true;
    }

    تهی lazyDeclareConstPoolVar(AstVar* varp) {
        اگر (!declaredOnce(varp)) بازگشت;  // Already declared
        ثابت string nameProtect = EmitCUtil::constPoolName(varp);
        m_emitter.putns(varp, "extern const ");
        m_emitter.puts(varp->dtypep()->cType(nameProtect, false, false));
        m_emitter.puts(";\n");
        m_needs_Blank_سطر = true;
    }

    // VISITORS
    تهی visit(Ast_گره_CCall* nodep) override {
        lazyDeclare(nodep->funcp());
        iterateChildrenConst(nodep);
    }

    تهی visit(AstAddrOfCFunc* nodep) override {
        lazyDeclare(nodep->funcp());
        iterateChildrenConst(nodep);
    }

    تهی visit(AstVarRef* nodep) override {
        AstVar* ثابت varp = nodep->varp();
        // Only constant pool symbols are lazy declared for now ...
        اگر (EmitCParentModule::get(varp)->isConstPool()) lazyDeclareConstPoolVar(varp);
    }

    تهی visit(AstExecGraph* nodep) override {
        اگر (!declaredOnce(nodep)) بازگشت;
        m_emitter.putns(nodep, "extern VlExecGraph " + EmitCUtil::execGraphName(nodep) + ";\n");
        m_needs_Blank_سطر = true;
    }

    تهی visit(Ast_گره* nodep) override { iterateChildrenConst(nodep); }

عمومی:
    explicit EmitCLazyDecls(EmitCBaseVisitorConst& emitter)
        : m_emitter{emitter} {}
    تهی emit(Ast_گره* nodep) {
        m_needs_Blank_سطر = false;
        iterateChildrenConst(nodep);
        اگر (m_needs_Blank_سطر) m_emitter.puts("\n");
    }
    تهی emit(ثابت string& prefix, ثابت string& name, ثابت string& suffix) {
        m_emittedManually.درج(name);
        m_emitter.ensure_New_سطر();
        m_emitter.puts(prefix);
        m_emitter.puts(name);
        m_emitter.puts(suffix);
        m_emitter.ensure_New_سطر();
    }
    تهی declared(AstCFunc* nodep) { m_emitted.درج(nodep); }
    تهی reset() { m_emitted.clear(); }
};

// ######################################################################
//  Emit statements and expressions

class EmitCFunc VL_NOT_FINAL : عمومی EmitCConstInit {
    VMemberMap m_memberMap;
    AstVarRef* m_wideTempRefp = nullptr;  // Variable that _WW macros should be setting
    std::unordered_map<AstJumpBlock*, size_t> m_labelNumbers;  // Label numbers for AstJumpBlocks
    bool m_createdScopeHash = false;  // Already created a scope hash

محافظتشده:
    VL_DEFINE_DEBUG_FUNCTIONS;

    EmitCLazyDecls m_lazyDecls{*this};  // Visitor for emitting lazy declarations
    bool m_useSelfForThis = false;  // Replace "this" with "vlSelf"
    bool m_usevlSelfRef = false;  // Use vlSelfRef reference instead of vlSelf pointer
    ثابت Ast_گره_Module* m_modp = nullptr;  // Current module being emitted
    ثابت AstCFunc* m_cfuncp = nullptr;  // Current function being emitted
    bool m_instantiatesOwnProcess = false;
    ثابت AstClassPackage* m_classOrPackage = nullptr;  // Pointer to current class or package
    string m_classOrPackageHash;  // Hash of class or package name

    bool constructorNeedsProcess(ثابت AstClass* ثابت classp) {
        ثابت Ast_گره* ثابت newp = m_memberMap.findMember(classp, "new");
        اگر (!newp) بازگشت false;
        ثابت AstCFunc* ثابت ctorp = VN_CAST(newp, CFunc);
        اگر (!ctorp) بازگشت false;
        UASSERT_OBJ(ctorp->isConstructor(), ctorp, "`new` is not a constructor!");
        بازگشت ctorp->needProcess();
    }

    bool constructorNeedsProcess(ثابت Ast_گره_DType* ثابت dtypep) {
        اگر (ثابت AstClassRefDType* ثابت crefdtypep = VN_CAST(dtypep, ClassRefDType))
            بازگشت constructorNeedsProcess(crefdtypep->classp());
        بازگشت false;
    }

    bool coverageUsesLocalCounter(Ast_گره_Cover_Decl* ثابت declp) ثابت {
        // Only functions with an object receiver can address the module-local
        // coverage array. Static/package/class helper functions still use the
        // symbol-table array, as they may have vlSymsp but no vlSelf/this.
        // Also require the coverage declaration to belong to the current
        // module; cloned task bodies can increment declarations owned by a
        // different module, and those must keep using the global array.
        بازگشت m_cfuncp && !m_cfuncp->isStatic() && !VN_IS(m_modp, Class)
               && !VN_IS(m_modp, ClassPackage)
               && EmitCParentModule::get(declp->dataDeclThisp()) == m_modp;
    }
    صحیح coverage_Bin_رقم(Ast_گره_Cover_Decl* ثابت declp, bool forceGlobal = false) ثابت {
        بازگشت !forceGlobal && coverageUsesLocalCounter(declp)
                   ? declp->dataDeclThisp()->local_Bin_رقم()
                   : declp->dataDeclThisp()->bin_رقم();
    }
    تهی putCoverageArray(Ast_گره_Cover_Decl* ثابت declp, bool forceGlobal = false) {
        اگر (!forceGlobal && coverageUsesLocalCounter(declp)) {
            // Keep counters on the module object when possible, so every
            // no-inline instance has storage available if coverage output is
            // later written with forcePerInstance.
            puts(m_cfuncp->isLoose() ? "vlSelf->__Vcoverage" : "this->__Vcoverage");
        } وگرنه {
            // Static/package/class helper functions do not have vlSelf, so
            // keep using the shared symbol-table coverage array there.
            puts("vlSymsp->__Vcoverage");
        }
    }
    تهی emit_Cover_Other_Decl_درج(AstCoverOtherDecl* nodep, bool forceGlobal = false) {
        putns(nodep, "vlSelf->__vlCoverInsert(");  // As Declared in emitCoverageDecl
        putCoverageArray(nodep, forceGlobal);
        puts(" + ");
        puts(cvtToStr(coverage_Bin_رقم(nodep, forceGlobal)));
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
                   + VIdProtect::protect_واژگان_If(nodep->hier(), nodep->protect()));
        puts(", ");
        putsQuoted(VIdProtect::protect_واژگان_If(nodep->page(), nodep->protect()));
        puts(", ");
        putsQuoted(VIdProtect::protect_واژگان_If(nodep->comment(), nodep->protect()));
        puts(", ");
        putsQuoted(nodep->linescov());
        puts(", ");
        putsQuoted(VIdProtect::protect_واژگان_If(nodep->fsmVar(), nodep->protect()));
        puts(", ");
        putsQuoted(VIdProtect::protect_واژگان_If(nodep->fsmFrom(), nodep->protect()));
        puts(", ");
        putsQuoted(VIdProtect::protect_واژگان_If(nodep->fsmTo(), nodep->protect()));
        puts(", ");
        putsQuoted(VIdProtect::protect_واژگان_If(nodep->fsmTag(), nodep->protect()));
        puts(");\n");
    }

عمومی:
    // METHODS
    bool displayEmitHeader(Ast_گره* nodep);
    تهی display_گره(Ast_گره* nodep, AstSFormatF* fmtp, ثابت string& vformat, Ast_گره* exprsp,
                     bool isScan);

    bool emitSimpleOk(Ast_گره_عبارت* nodep);
    تهی emitIQW(ثابت Ast_گره* nodep) {
        // See "Type letters" in verilated.h
        // Other abbrevs: "C"har, "S"hort, "F"loat, "D"ouble, stri"N"g, "R"=queue, "U"npacked
        puts(nodep->dtypep()->skipRefp()->charIQWN());
    }
    تهی emitRU(ثابت Ast_گره* nodep) {
        ثابت Ast_گره_DType* dtp = nodep->dtypep()->skipRefp();
        // See "Type letters" in verilated.h
        اگر (VN_IS(dtp, UnpackArrayDType))
            puts("U");
        وگرنه اگر (VN_IS(dtp, QueueDType) || VN_IS(dtp, DynArrayDType))
            puts("R");
    }
    تهی emitScIQW(AstVar* nodep) {
        UASSERT_OBJ(nodep->isSc(), nodep, "emitting SystemC operator on non-SC variable");
        puts(nodep->isScBigUint()                           ? "SB"
             : (nodep->isScUint() || nodep->isScUintBool()) ? "SU"
             : nodep->isScBv()                              ? "SW"
             : nodep->isDouble()                            ? "SD"
                                                            : (nodep->isScQuad() ? "SQ" : "SI"));
    }
    تهی emitDatap(ثابت Ast_گره* nodep) {
        // When passing to a function with va_args the compiler doesn't
        // know need a pointer so when wide, need to look inside VlWide
        اگر (nodep->isWide()) puts(".data()");
    }
    تهی emitOpName(Ast_گره* nodep, ثابت string& format, Ast_گره* lhsp, Ast_گره* rhsp,
                    Ast_گره* thsp);
    تهی emitCCallArgs(ثابت Ast_گره_CCall* nodep, ثابت string& selfPointer, bool inProcess);
    تهی emitDereference(Ast_گره* nodep, ثابت string& pointer);
    std::string dereferenceString(ثابت std::string& pointer) ثابت;
    تهی emitCvtPackStr(Ast_گره* nodep);
    تهی emitConstant(AstConst* nodep);
    تهی emitConstantString(ثابت AstConst* nodep);
    تهی emitVarReset(ثابت string& prefix, AstVar* varp, bool constructing);
    string emitVarResetRecurse(ثابت AstVar* varp, bool constructing,
                               ثابت string& varNameProtected, Ast_گره_DType* dtypep, صحیح depth,
                               ثابت string& suffix, ثابت Ast_گره* valuep);
    تهی emitVarResetScopeHash();
    تهی emitChangeDet();
    تهی putCommaIterateNext(Ast_گره* nodep, bool comma = false) {
        برای (Ast_گره* subnodep = nodep; subnodep; subnodep = subnodep->nextp()) {
            اگر (comma) puts(", ");
            iterateConst(subnodep);
            comma = true;
        }
    }
    template <typename T>
    string optionalProcArg(ثابت T* ثابت nodep) {
        بازگشت (nodep && constructorNeedsProcess(nodep)) ? "vlProcess, " : "";
    }
    ثابت AstCNew* getSuperNewCallRecursep(Ast_گره* ثابت nodep) {
        // Get the super.new call
        اگر (!nodep) بازگشت nullptr;
        اگر (ثابت AstCNew* ثابت cnewp = VN_CAST(nodep, CNew)) بازگشت cnewp;
        اگر (ثابت Ast_Stmt_عبارت* ثابت stmtp = VN_CAST(nodep, Stmt_عبارت)) {
            اگر (ثابت AstCNew* ثابت cnewp = VN_CAST(stmtp->exprp(), CNew)) بازگشت cnewp;
        }
        اگر (ثابت AstJumpBlock* ثابت blockp = VN_CAST(nodep, JumpBlock)) {
            اگر (ثابت AstCNew* ثابت cnewp = getSuperNewCallRecursep(blockp->stmtsp())) {
                بازگشت cnewp;
            }
        }
        اگر (ثابت AstCNew* ثابت cnewp = getSuperNewCallRecursep(nodep->nextp())) بازگشت cnewp;
        بازگشت nullptr;
    }

    تهی emitConstantW(AstConst* nodep, AstVarRef* assigntop) {
        // For tradition and compilation speed, assign each word directly into
        // output variable instead of using '='
        putns(nodep, "");
        اگر (nodep->رقم().isFourState()) {
            nodep->v3warn(E_UNSUPPORTED, "Unsupported: 4-state numbers in this context");
            بازگشت;
        }

        صحیح upWidth = nodep->رقم().widthToFit();
        صحیح chunks = 0;
        اگر (upWidth > EMITC_رقم_CONSTW * VL_EDATASIZE) {
            // Output e.g. 8 words in groups of e.g. 8
            chunks = (upWidth - 1) / (EMITC_رقم_CONSTW * VL_EDATASIZE);
            upWidth %= (EMITC_رقم_CONSTW * VL_EDATASIZE);
            اگر (upWidth == 0) upWidth = (EMITC_رقم_CONSTW * VL_EDATASIZE);
        }
        {  // Upper e.g. 8 words
            اگر (chunks) {
                putnbs(nodep, "VL_CONSTHI_W_");
                puts(cvtToStr(VL_واژگان_I(upWidth)));
                puts("X(");
                puts(cvtToStr(nodep->widthMin()));
                puts(",");
                puts(cvtToStr(chunks * EMITC_رقم_CONSTW * VL_EDATASIZE));
            } وگرنه {
                putnbs(nodep, "VL_CONST_W_");
                puts(cvtToStr(VL_واژگان_I(upWidth)));
                puts("X(");
                puts(cvtToStr(nodep->widthMin()));
            }
            puts(",");
            اگر (!assigntop->selfPointer().isEmpty()) {
                emitDereference(assigntop, assigntop->selfPointerProtect(m_useSelfForThis));
            }
            puts(varName(assigntop->varp()));
            برای (صحیح word = VL_واژگان_I(upWidth) - 1; word >= 0; word--) {
                // Only 32 bits - llx + long long here just to appease CPP format warning
                ofp()->printf(",0x%08" PRIx64, static_cast<uint64_t>(nodep->رقم().edataWord(
                                                   word + chunks * EMITC_رقم_CONSTW)));
            }
            puts(")");
        }
        برای (chunks--; chunks >= 0; chunks--) {
            puts(";\n");
            putbs("VL_CONSTLO_W_");
            puts(cvtToStr(EMITC_رقم_CONSTW));
            puts("X(");
            puts(cvtToStr(chunks * EMITC_رقم_CONSTW * VL_EDATASIZE));
            puts(",");
            اگر (!assigntop->selfPointer().isEmpty()) {
                emitDereference(assigntop, assigntop->selfPointerProtect(m_useSelfForThis));
            }
            puts(varName(assigntop->varp()));
            برای (صحیح word = EMITC_رقم_CONSTW - 1; word >= 0; word--) {
                // Only 32 bits - llx + long long here just to appease CPP format warning
                ofp()->printf(",0x%08" PRIx64, static_cast<uint64_t>(nodep->رقم().edataWord(
                                                   word + chunks * EMITC_رقم_CONSTW)));
            }
            puts(")");
        }
    }

    تهی emitNodesWithText(Ast_گره* nodesp, bool useSelfForThis, bool tracking,
                           ثابت std::string& separator) {
        برای (Ast_گره* nodep = nodesp; nodep; nodep = nodep->nextp()) {
            اگر (ثابت AstText* ثابت textp = VN_CAST(nodep, Text)) {
                ثابت std::string text
                    = VSelfPointerText::replaceThis(useSelfForThis, textp->text());
                اگر (tracking) {
                    puts(text);
                } وگرنه {
                    ofp()->putsNoTracking(text);
                }
            } وگرنه {
                iterateConst(nodep);
            }
            اگر (nodep->nextp()) puts(separator);
        }
    }

    تهی putConstructorSubinit(ثابت AstClass* classp, AstCFunc* cfuncp) {
        // Direct non-virtual bases in declaration order
        برای (ثابت AstClassExtends* extp = classp->extendsp(); extp;
             extp = VN_AS(extp->nextp(), ClassExtends)) {
            اگر (extp->classp()->isInterfaceClass()) ادامه;
            اگر (extp->classp()->useVirtualPublic()) ادامه;
            puts(EmitCUtil::prefixNameProtect(extp->classp()));
            puts("::init");
            اگر (constructorNeedsProcess(extp->classp())) {
                puts("(vlProcess, vlSymsp");
            } وگرنه {
                puts("(vlSymsp");
            }
            // Handle super.new() args for the concrete parent
            اگر (ثابت AstCNew* ثابت superNewCallp = getSuperNewCallRecursep(cfuncp->stmtsp())) {
                putCommaIterateNext(superNewCallp->argsp(), true);
            }
            puts(");\n");
            // Since SystemVerilog allows only for extending from one class, for sure there won't
            // be any more constructors to be called. Even if there are some classes which are
            // "implemented", those are interface classes which do not have constructors.
            بشکن;
        }
    }

    // VISITORS
    using EmitCConstInit::visit;
    تهی visit(AstCFunc* ثابت nodep) override {
        اگر (nodep->emptyBody() && !nodep->isLoose()) بازگشت;
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
        اگر (nodep->ifdef() != "") putns(nodep, "#ifdef " + nodep->ifdef() + "\n");
        emitCFuncHeader(nodep, m_modp, /* withScope: */ true);

        puts(" {\n");
        اگر (nodep->isConstructor()) {
            ثابت AstClass* ثابت classp = VN_CAST(nodep->scopep()->modp(), Class);
            اگر (classp && classp->extendsp()) putConstructorSubinit(classp, nodep);
        }

        // "+" in the debug indicates a print from the model
        puts("VL_DEBUG_IF(VL_DBG_MSGF(\"+  ");
        برای (صحیح i = 0; i < m_modp->level(); ++i) puts("  ");
        puts(EmitCUtil::prefixNameProtect(m_modp));
        puts(nodep->isLoose() ? "__" : "::");
        puts(nodep->nameProtect() + "\\n\"); );\n");

        اگر (nodep->isLoose()) {
            m_lazyDecls.declared(nodep);  // Defined here, so no longer needs declaration
            اگر (!nodep->isStatic()) {  // Standard prologue
                m_useSelfForThis = true;
                اگر (nodep->voidSelf()) {
                    ثابت string className = EmitCUtil::prefixNameProtect(m_modp);
                    puts(className + "* const vlSelf = static_cast<" + className
                         + "*>(vlVoidSelf);\n");
                }
                اگر (!VN_IS(m_modp, Class)) {
                    puts(EmitCUtil::symClassAssign());  // Uses vlSelf
                } وگرنه {
                    puts("(void)vlSelf;  // Prevent unused variable warning\n");
                }
            }
        }

        // Instantiate a process class if it's going to be needed somewhere later
        nodep->forall([&](ثابت Ast_گره_CCall* ccallp) -> bool {
            اگر (ccallp->funcp()->needProcess()
                && (ccallp->funcp()->isCoroutine() == VN_IS(ccallp->backp(), CAwait))) {
                اگر (!nodep->needProcess() && !m_instantiatesOwnProcess) {
                    m_instantiatesOwnProcess = true;
                    بازگشت false;
                }
            }
            بازگشت true;
        });
        اگر (m_instantiatesOwnProcess) {
            AstCStmt* ثابت vlprocp = new AstCStmt{nodep->fileline()};
            vlprocp->add("VlProcessRef vlProcess = std::make_shared<VlProcess>();\n");
            vlprocp->add("VlProcess::currentp(vlProcess.get());");
            nodep->prependStmtsp(vlprocp);
        } وگرنه اگر (nodep->needProcess() && nodep->stmtsp()) {
            // Set current process so VlRNG() constructors in this function seed from it
            AstCStmt* ثابت setProcessp = new AstCStmt{nodep->fileline()};
            setProcessp->add("VlProcess::currentp(vlProcess.get());");
            nodep->prependStmtsp(setProcessp);
        }

        برای (Ast_گره* subnodep = nodep->argsp(); subnodep; subnodep = subnodep->nextp()) {
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
            iterateAndNextConstNull(nodep->varsp());
        }

        اگر (nodep->stmtsp()) {
            putsDecoration(nodep, "// Body\n");
            iterateAndNextConstNull(nodep->stmtsp());
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

    تهی visit(AstVar* nodep) override {
        UASSERT_OBJ(m_cfuncp, nodep, "Cannot emit non-local variable");
        emitVarDecl(nodep);
    }

    تهی visit(AstCvtArrayToArray* nodep) override {
        اگر (nodep->reverse()) {
            puts("VL_REVCLONE_Q(");
        } وگرنه {
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

    تهی visit(AstCvtArrayToPacked* nodep) override {
        Ast_گره_DType* ثابت fromDtp = nodep->fromp()->dtypep()->skipRefp();
        Ast_گره_DType* ثابت elemDtp = fromDtp->subDTypep()->skipRefp();
        ثابت bool descending = [&]() {
            اگر (ثابت AstUnpackArrayDType* ثابت unpackDtp = VN_CAST(fromDtp, UnpackArrayDType))
                بازگشت !unpackDtp->declRange().ascending();
            بازگشت false;
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
        } وگرنه {
            emitOpName(nodep, "(%nw, %rw, %P, %li)", nodep->fromp(), elemDtp, nullptr);
        }
    }

    تهی visit(AstCvtUnpackToQueue* nodep) override {
        Ast_گره_DType* ثابت elemDTypep = nodep->fromp()->dtypep()->subDTypep();
        emitOpName(nodep, nodep->emitC(), nodep->fromp(), elemDTypep, nullptr);
    }
    تهی visit(AstCvtUnpackToUnpack* nodep) override {
        // Whether each dimension is reversed, the first for the outermost, up to the last that is
        std::vector<bool> reverses;
        ثابت AstUnpackArrayDType* بالاترین = VN_AS(nodep->dtypep()->skipRefp(), UnpackArrayDType);
        ثابت AstUnpackArrayDType* fromp
            = VN_AS(nodep->fromp()->dtypep()->skipRefp(), UnpackArrayDType);
        وقتی (بالاترین) {
            reverses.افزودن_back(بالاترین->declRange().ascending() != fromp->declRange().ascending());
            بالاترین = VN_CAST(بالاترین->subDTypep()->skipRefp(), UnpackArrayDType);
            fromp = VN_CAST(fromp->subDTypep()->skipRefp(), UnpackArrayDType);
        }
        وقتی (!reverses.back()) reverses.برداشت_back();
        putns(nodep, "VL_CVT_UNPACK_TO_UNPACK<");
        برای (size_t i = 0; i < reverses.size(); ++i) {
            اگر (i) puts(", ");
            puts(reverses[i] ? "1" : "0");
        }
        puts(">(");
        iterateAndNextConstNull(nodep->fromp());
        puts(")");
    }

    تهی visit(Ast_گره_Assign* nodep) override {
        اگر (AstCReset* ثابت resetp = VN_CAST(nodep->rhsp(), CReset)) {
            // TODO get rid of emitVarReset and instead let AstNodeAssign understand how to init
            // anything
            Ast_گره* fromp = nodep->lhsp();
            // Fork needs to use a member select.  Nothing else should be possible before VarRef.
            اگر (AstMemberSel* ثابت sfromp = VN_CAST(fromp, MemberSel)) {
                // Fork-DynScope generated pointer to previously automatic variable
                AstVar* ثابت memberVarp = sfromp->varp();
                fromp = sfromp->fromp();
                اگر (AstNullCheck* ثابت sfromp = VN_CAST(fromp, NullCheck)) fromp = sfromp->lhsp();
                Ast_گره_Var_Ref* ثابت fromVarRefp = VN_AS(fromp, گره_Var_Ref);
                emitVarReset(
                    ("VL_NULL_CHECK("s
                     + (fromVarRefp->selfPointer().isEmpty()
                            ? ""
                            : dereferenceString(fromVarRefp->selfPointerProtect(m_useSelfForThis)))
                     + varName(fromVarRefp->varp()) + ", \""
                     + V3OutFormatter::quoteNameControls(protect(nodep->fileline()->filename()))
                     + "\", " + std::to_string(nodep->fileline()->lineno()) + ")->"),
                    memberVarp, resetp->constructing());
            } وگرنه {
                Ast_گره_Var_Ref* ثابت fromVarRefp = VN_AS(fromp, گره_Var_Ref);
                AstVar* ثابت varp = fromVarRefp->varp();
                ثابت string prefix
                    = fromVarRefp->selfPointer().isEmpty()
                          ? ""
                          : dereferenceString(
                                VN_AS(fromp, گره_Var_Ref)->selfPointerProtect(m_useSelfForThis));
                emitVarReset(prefix, varp, resetp->constructing());
            }
            بازگشت;
        }
        bool paren = true;
        bool decind = false;
        bool rhs = true;
        bool reverseUnpack = false;  // Set for descending CvtPackedToArray
        ثابت AstUnpackArrayDType* ثابت unpackDtp
            = VN_CAST(nodep->dtypep()->skipRefp(), UnpackArrayDType);
        اگر (AstSel* ثابت selp = VN_CAST(nodep->lhsp(), Sel)) {
            UASSERT_OBJ(selp->widthMin() == selp->widthConst(), selp, "Width mismatch");
            اگر (selp->widthMin() == 1) {
                putnbs(nodep, "VL_ASSIGNBIT_");
                emitIQW(selp->fromp());
                اگر (nodep->rhsp()->isAllOnesV()) {
                    puts("O(");
                    rhs = false;
                } وگرنه {
                    puts("I(");
                }
                iterateAndNextConstNull(selp->lsbp());
                puts(", ");
                iterateAndNextConstNull(selp->fromp());
                اگر (rhs) puts(", ");
            } وگرنه {
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
        } وگرنه اگر (ثابت AstGetcRefN* ثابت selp = VN_CAST(nodep->lhsp(), GetcRefN)) {
            iterateAndNextConstNull(selp->lhsp());
            puts(" = ");
            putnbs(selp, "VL_PUTC_N(");
            iterateAndNextConstNull(selp->lhsp());
            puts(", ");
            iterateAndNextConstNull(selp->rhsp());
            puts(", ");
        } وگرنه اگر (AstVar* ثابت varp = AstVar::scVarRecurse(nodep->lhsp())) {
            putnbs(varp, "VL_ASSIGN_");  // Set a systemC variable
            emitScIQW(varp);
            emitIQW(nodep);
            puts("(");
            puts(cvtToStr(nodep->widthMin()) + ", ");
            iterateAndNextConstNull(nodep->lhsp());
            puts(", ");
        } وگرنه اگر (AstVar* ثابت varp = AstVar::scVarRecurse(nodep->rhsp())) {
            putnbs(varp, "VL_ASSIGN_");  // Get a systemC variable
            emitIQW(nodep);
            emitScIQW(varp);
            puts("(");
            puts(cvtToStr(nodep->widthMin()) + ", ");
            iterateAndNextConstNull(nodep->lhsp());
            puts(", ");
        } وگرنه اگر (ثابت AstCvtPackedToArray* ثابت castp
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
            اگر (unpackDtp && !unpackDtp->declRange().ascending()) reverseUnpack = true;
        } وگرنه اگر (ثابت AstCvtArrayToArray* ثابت castp
                   = VN_CAST(nodep->rhsp(), CvtArrayToArray)) {
            اگر (castp->reverse()) {
                putns(castp, "VL_REVCOPY_Q(");
            } وگرنه {
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
        } وگرنه اگر (nodep->isWide() && VN_IS(nodep->lhsp(), VarRef)  //
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
                   && !VN_IS(nodep->rhsp(), عبارت_Stmt)) {
            // Wide functions assign into the array directly, don't need separate assign statement
            m_wideTempRefp = VN_AS(nodep->lhsp(), VarRef);
            paren = false;
        } وگرنه اگر (nodep->isWide() && !unpackDtp && !VN_IS(nodep->rhsp(), Const)) {
            putnbs(nodep, "VL_ASSIGN_W(");
            puts(cvtToStr(nodep->widthMin()) + ", ");
            iterateAndNextConstNull(nodep->lhsp());
            puts(", ");
        } وگرنه اگر (VN_IS(nodep->lhsp()->dtypep()->skipRefp(), QueueDType)
                   && (VN_IS(nodep->rhsp(), StreamL) || VN_IS(nodep->lhsp(), StreamL)
                       || VN_IS(nodep->rhsp(), StreamR) || VN_IS(nodep->lhsp(), StreamR)
                       || VN_IS(nodep->rhsp(), StreamR))) {
            //if either side is streamL or streamR don't emit lhsp everything will be passed by
            //reference

            paren = false;

        } وگرنه {
            paren = false;
            iterateAndNextConstNull(nodep->lhsp());
            puts(" ");
            ofp()->blockInc();
            decind = true;
            اگر (!VN_IS(nodep->rhsp(), Const)) ofp()->putBreak();
            putns(nodep, "= ");
        }
        اگر (rhs) iterateAndNextConstNull(nodep->rhsp());
        اگر (paren) puts(")");
        اگر (decind) ofp()->blockDec();
        puts(";\n");
        اگر (reverseUnpack) {
            puts("VL_UNPACK_REVERSED(");
            iterateAndNextConstNull(nodep->lhsp());
            puts(");\n");
        }
    }
    تهی visit(AstAssocSel* nodep) override {
        iterateAndNextConstNull(nodep->fromp());
        ثابت std::string atFunc = nodep->isLValue() ? ".atWrite(" : ".at(";
        putnbs(nodep, atFunc);
        AstAssocArrayDType* ثابت adtypep
            = VN_AS(nodep->fromp()->dtypep()->skipRefp(), AssocArrayDType);
        UASSERT_OBJ(adtypep, nodep, "Associative select on non-associative type");
        iterateAndNextConstNull(nodep->bitp());
        puts(")");
    }
    تهی visit(AstWildcardSel* nodep) override {
        iterateAndNextConstNull(nodep->fromp());
        ثابت std::string atFunc = nodep->isLValue() ? ".atWrite(" : ".at(";
        putnbs(nodep, atFunc);
        AstWildcardArrayDType* ثابت adtypep
            = VN_AS(nodep->fromp()->dtypep()->skipRefp(), WildcardArrayDType);
        UASSERT_OBJ(adtypep, nodep, "Wildcard select on non-wildcard-associative type");
        iterateAndNextConstNull(nodep->bitp());
        puts(")");
    }
    تهی visit(AstCCall* nodep) override {
        ثابت AstCFunc* ثابت funcp = nodep->funcp();
        ثابت Ast_گره_Module* ثابت funcModp = EmitCParentModule::get(funcp);
        putnbs(nodep, "");
        اگر (funcp->dpiImportPrototype()) {
            // Calling DPI import
            putns(nodep, funcp->name());
        } وگرنه اگر (funcp->isProperMethod() && funcp->isStatic()) {
            // Call static method via the containing class
            putns(funcModp, EmitCUtil::prefixNameProtect(funcModp) + "::");
            putns(nodep, funcp->nameProtect());
        } وگرنه اگر (nodep->superReference()) {
            // Calling superclass method
            putns(funcModp, EmitCUtil::prefixNameProtect(funcModp) + "::");
            putns(nodep, funcp->nameProtect());
        } وگرنه اگر (funcp->isLoose()) {
            // Calling loose method
            putns(nodep, funcNameProtect(funcp));
        } وگرنه {
            // Calling regular method/function
            اگر (!nodep->selfPointer().isEmpty()) {
                emitDereference(nodep, nodep->selfPointerProtect(m_useSelfForThis));
            }
            putns(nodep, funcp->nameProtect());
        }
        emitCCallArgs(nodep, nodep->selfPointerProtect(m_useSelfForThis), m_cfuncp->needProcess());
    }
    تهی visit(AstCMethodCall* nodep) override {
        ثابت AstCFunc* ثابت funcp = nodep->funcp();
        UASSERT_OBJ(!funcp->isLoose(), nodep, "Loose method called via AstCMethodCall");
        iterateConst(nodep->fromp());
        putnbs(nodep, "->");
        اگر (nodep->superReference()) {
            // Super call, so the base implementation is used rather than the override
            ثابت Ast_گره_Module* ثابت funcModp = EmitCParentModule::get(funcp);
            putns(funcModp, EmitCUtil::prefixNameProtect(funcModp) + "::");
        }
        putns(funcp, funcp->nameProtect());
        emitCCallArgs(nodep, "", m_cfuncp->needProcess());
    }
    تهی visit(AstCAwait* nodep) override {
        putns(nodep, "co_await ");
        iterateConst(nodep->exprp());
        puts(";\n");
    }
    تهی visit(AstCNew* nodep) override {
        اگر (VN_IS(nodep->dtypep(), VoidDType)) {  // super.new case
            putsDecoration(nodep, "/*super.new*/");
            بازگشت;
        }
        // assignment case;
        putns(nodep, "VL_NEW(" + EmitCUtil::prefixNameProtect(nodep->dtypep()) + ", "
                         + optionalProcArg(nodep->dtypep()) + "vlSymsp");
        putCommaIterateNext(nodep->argsp(), true);
        puts(")");
    }
    تهی visit(AstCMethodHard* nodep) override {
        iterateConst(nodep->fromp());
        putns(nodep, nodep->usePtr() ? "->" : ".");
        putns(nodep, nodep->name());
        اگر (nodep->method() == VCMethod::FORCE_READ_SEL) {
            emitIQW(nodep);
            اگر (nodep->isWide()) puts("<" + cvtToStr(nodep->dtypep()->width_واژگان()) + ">");
        } وگرنه اگر (nodep->method() == VCMethod::ARRAY_SLICE) {
            // VlUnpacked::slice<N_Out>(loIdx) - N_Out is the (fixed) result array size
            ثابت AstUnpackArrayDType* ثابت adtypep
                = VN_AS(nodep->dtypep()->skipRefp(), UnpackArrayDType);
            puts("<" + cvtToStr(adtypep->elementsConst()) + ">");
        } وگرنه اگر (nodep->method() == VCMethod::COVERGROUP_ADD_COVERPOINT) {
            // The hit-list bound is a template argument of the returned VlCoverpointT<>, and the
            // node's own dtype is that type, so it is the one source of truth for both.
            ثابت AstCoverpointDType* ثابت cpdtypep
                = VN_AS(nodep->dtypep()->skipRefp(), CoverpointDType);
            puts("<" + cvtToStr(cpdtypep->hitBound()) + ">");
        } وگرنه اگر (nodep->method() == VCMethod::COVERGROUP_ADD_CROSS) {
            ثابت AstCoverCrossDType* ثابت cxdtypep
                = VN_AS(nodep->dtypep()->skipRefp(), CoverCrossDType);
            puts("<" + cxdtypep->cppTemplateArgs() + ">");
        }
        puts("(");
        bool comma = false;
        صحیح arg_رقم = 0;
        برای (Ast_گره* subnodep = nodep->pinsp(); subnodep; subnodep = subnodep->nextp()) {
            اگر (comma) puts(", ");
            اگر (nodep->method() == VCMethod::RANDOMIZER_HARD && arg_رقم == 1) {
                // For RANDOMIZER_HARD's filename argument (2nd arg after constraint),
                // apply protect() similar to VL_STOP to handle --protected flag
                اگر (ثابت AstCExpr* ثابت cexprp = VN_CAST(subnodep, CExpr)) {
                    // Extract filename from the CExpr (which contains "filename")
                    std::string filename;
                    برای (ثابت Ast_گره* textnodep = cexprp->nodesp(); textnodep;
                         textnodep = textnodep->nextp()) {
                        اگر (ثابت AstText* ثابت textp = VN_CAST(textnodep, Text)) {
                            filename = textp->text();
                            بشکن;
                        }
                    }
                    // Remove surrounding quotes if present
                    اگر (filename.size() >= 2 && filename.front() == '"'
                        && filename.back() == '"') {
                        filename = filename.substr(1, filename.size() - 2);
                    }
                    // Emit with protect()
                    putsQuoted(protect(filename));
                } وگرنه {
                    iterateConst(subnodep);
                }
            } وگرنه {
                iterateConst(subnodep);
            }
            comma = true;
            arg_رقم++;
        }
        اگر (nodep->withp()) {
            اگر (comma) puts(", ");
            iterateConst(nodep->withp());
        }
        puts(")");
    }
    تهی visit(AstCFuncHard* nodep) override {
        putns(nodep, nodep->name());
        puts("(");
        putCommaIterateNext(nodep->pinsp());
        puts(")");
    }
    تهی visit(AstLambdaArgRef* nodep) override { putbs(nodep->nameProtect()); }
    تهی visit(AstWith* nodep) override {
        // With uses a C++11 lambda
        putnbs(nodep, "[&](");
        اگر (auto* ثابت argrefp = nodep->indexArgRefp()) {
            putnbs(argrefp, argrefp->dtypep()->cType(argrefp->nameProtect(), false, false));
            puts(", ");
        }
        اگر (auto* ثابت argrefp = nodep->مقدار_Arg_Refp()) {
            putnbs(argrefp, argrefp->dtypep()->cType(argrefp->nameProtect(), false, false));
        }
        puts(") -> ");
        putnbs(nodep, nodep->dtypep()->cType("", false, false));
        puts(" {\n");
        VL_RESTORER(m_createdScopeHash);
        iterateAndNextConstNull(nodep->exprp());
        puts("}\n");
    }
    تهی visit(AstCase* nodep) override {  // LCOV_EXCL_LINE
        // In V3Case...
        nodep->v3fatalSrc("Case statements should have been reduced out");
    }
    تهی visit(AstComment* nodep) override {
        string at;
        اگر (nodep->showAt()) {
            at = " at " + nodep->fileline()->ascii();
            // If protecting, passthru less information about the design
            اگر (!v3Global.opt.protectIds()) بازگشت;
        }
        اگر (!(nodep->protect() && v3Global.opt.protectIds())) {
            putsDecoration(nodep, "// "s + nodep->name() + at + "\n");
        }
        iterateChildrenConst(nodep);
    }
    تهی visit(AstCoverOtherDecl* nodep) override {
        emit_Cover_Other_Decl_درج(nodep);
        اگر (coverageUsesLocalCounter(nodep)) {
            // Some task bodies are cloned into a caller module where the
            // original instance-local counter is not addressable. Those clones
            // fall back to the global bin number, so register that slot too.
            emit_Cover_Other_Decl_درج(nodep, true);
        }
    }
    تهی visit(AstCoverToggleDecl* nodep) override {
        putns(nodep, "vlSelf->__vlCoverToggleInsert(");  // As Declared in emitCoverageDecl
        puts(cvtToStr(nodep->range().راست()));
        puts(", ");
        puts(cvtToStr(nodep->range().چپ()));
        puts(", ");
        puts(cvtToStr(nodep->range().ranged()));
        puts(", ");
        putCoverageArray(nodep);
        puts(" + ");
        puts(cvtToStr(coverage_Bin_رقم(nodep)));
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
                   + VIdProtect::protect_واژگان_If(nodep->hier(), nodep->protect()));
        puts(", ");
        putsQuoted(VIdProtect::protect_واژگان_If(nodep->page(), nodep->protect()));
        puts(", ");
        putsQuoted(VIdProtect::protect_واژگان_If(nodep->comment(), nodep->protect()));
        puts(");\n");
    }
    تهی visit(AstCoverInc* nodep) override {
        اگر (VN_IS(nodep->declp(), CoverOtherDecl)) {
            اگر (v3Global.opt.threads() > 1) {
                putns(nodep, "");
                putCoverageArray(nodep->declp());
                puts("[");
                puts(cvtToStr(coverage_Bin_رقم(nodep->declp())));
                puts("].fetch_add(1, std::memory_order_relaxed);\n");
            } وگرنه {
                putns(nodep, "++(");
                putCoverageArray(nodep->declp());
                puts("[");
                puts(cvtToStr(coverage_Bin_رقم(nodep->declp())));
                puts("]);\n");
            }
        } وگرنه {
            puts("VL_COV_TOGGLE_CHG_");
            اگر (v3Global.opt.threads() > 1) {
                puts("MT_");
            } وگرنه {
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
            puts(cvtToStr(coverage_Bin_رقم(nodep->declp())));
            puts(", ");
            iterateConst(nodep->toggleExprp());
            puts(", ");
            iterateConst(nodep->toggleCovExprp());
            puts(");\n");
        }
    }
    تهی visit(AstDisableFork* nodep) override { putns(nodep, "vlProcess->disableFork();\n"); }
    تهی visit(AstCReturn* nodep) override {
        putns(nodep, "return (");
        iterateAndNextConstNull(nodep->lhsp());
        puts(");\n");
    }
    تهی visit(AstDisplay* nodep) override {
        string text = nodep->fmtp()->text();
        اگر (nodep->addNewline()) text += "\n";
        display_گره(nodep, nodep->fmtp(), text, nodep->fmtp()->exprsp(), false);
    }
    تهی visit(AstDumpCtl* nodep) override {
        انتخاب (nodep->ctlType()) {
        حالت VDumpCtlType::FILE:
            putns(nodep, "vlSymsp->_vm_contextp__->dumpfile(");
            iterateConst(nodep->exprp());
            puts(");\n");
            بشکن;
        حالت VDumpCtlType::VARS:
            // We ignore number of levels to dump in exprp()
            اگر (v3Global.opt.trace()) {
                putns(nodep, "vlSymsp->_traceDumpOpen();\n");
            } وگرنه {
                putns(nodep, "VL_PRINTF_MT(\"-Info: ");
                puts(V3OutFormatter::quoteNameControls(protect(nodep->fileline()->filename())));
                puts(":");
                puts(cvtToStr(nodep->fileline()->lineno()));
                puts(": $dumpvar ignored, as Verilated without --trace");
                puts("\\n\");\n");
            }
            بشکن;
        حالت VDumpCtlType::ALL:
            // $dumpall currently ignored
            بشکن;
        حالت VDumpCtlType::FLUSH:
            // $dumpall currently ignored; would need rework of VCD single thread,
            // or flag we pass-through to next eval() iteration
            بشکن;
        حالت VDumpCtlType::LIMIT:
            // $dumplimit currently ignored
            بشکن;
        حالت VDumpCtlType::OFF:
            // Currently ignored as both Vcd and Fst do not support them, as would need "X" dump
            بشکن;
        حالت VDumpCtlType::ON:
            // Currently ignored as $dumpoff is also ignored
            بشکن;
        پیشفرض: nodep->v3fatalSrc("Bad case, unexpected " << nodep->ctlType().ascii());
        }
    }
    تهی visit(AstScopeName* nodep) override {
        // For use under AstCCalls for dpiImports.  ScopeNames under
        // displays are handled in AstDisplay
        اگر (!nodep->dpiExport()) {
            // this is where the DPI import context scope is set
            ثابت string scope = nodep->scopeDpiName();
            putnbs(nodep, "(vlSymsp->" + protect("__Vscopep_" + scope) + ")");
        }
    }
    تهی visit(AstSFormat* nodep) override {
        display_گره(nodep, nodep->fmtp(), nodep->fmtp()->text(), nodep->fmtp()->exprsp(), false);
    }
    تهی visit(AstSFormatF* nodep) override {
        display_گره(nodep, nodep, nodep->text(), nodep->exprsp(), false);
    }
    تهی visit(AstFScanF* nodep) override {
        display_گره(nodep, nullptr, nodep->text(), nodep->exprsp(), true);
    }
    تهی visit(AstSScanF* nodep) override {
        display_گره(nodep, nullptr, nodep->text(), nodep->exprsp(), true);
    }
    تهی visit(Ast_مقدار_Plus_Args* nodep) override {
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
    تهی visit(AstTestPlusArgs* nodep) override {
        putns(nodep, "VL_TESTPLUSARGS_I(");
        iterateConst(nodep->searchp());
        puts(")");
    }
    تهی visit(AstFError* nodep) override {
        putns(nodep, "VL_FERROR_I");
        puts(nodep->strp()->isString() ? "N(" : "W(");
        iterateAndNextConstNull(nodep->filep());
        putbs(", ");
        اگر (nodep->strp()->isWide()) {
            puts(cvtToStr(nodep->strp()->width_واژگان()));
            putbs(", ");
        }
        iterateAndNextConstNull(nodep->strp());
        puts(")");
    }
    تهی visit(AstFGetS* nodep) override {
        emitOpName(nodep, nodep->emitC(), nodep->strgp(), nodep->filep(), nullptr);
    }
    تهی visit(AstFOpen* nodep) override {
        putns(nodep, "VL_FOPEN_NN(");
        iterateConst(nodep->filenamep());
        putbs(", ");
        iterateConst(nodep->modep());
        puts(");\n");
    }
    تهی visit(AstFOpenMcd* nodep) override {
        putns(nodep, "VL_FOPEN_MCD_N(");
        iterateConst(nodep->filenamep());
        puts(");\n");
    }
    تهی visit(Ast_گره_Read_Write_Mem* nodep) override {
        putns(nodep, nodep->cFuncPrefixp());
        puts("N(");
        puts(nodep->isHex() ? "true" : "false");
        putbs(", ");
        // Need real storage width
        puts(cvtToStr(nodep->memp()->dtypep()->skipRefp()->subDTypep()->widthMin()));
        uint32_t array_lo = 0;
        bool memUsePtr = true;
        {
            ثابت AstVarRef* ثابت varrefp = VN_CAST(nodep->memp(), VarRef);
            اگر (!varrefp) {
                nodep->v3error(nodep->verilogKwd() << " loading non-variable");
            } وگرنه اگر (VN_IS(varrefp->varp()->dtypeSkipRefp(), AssocArrayDType)
                       || VN_IS(varrefp->varp()->dtypeSkipRefp(), DynArrayDType)) {
                // nodep->memp() below will when verilated code is compiled create a C++ template
                memUsePtr = false;
            } وگرنه اگر (ثابت AstUnpackArrayDType* ثابت adtypep
                       = VN_CAST(varrefp->varp()->dtypeSkipRefp(), UnpackArrayDType)) {
                putbs(", ");
                puts(cvtToStr(varrefp->varp()->dtypep()->arrayUnpackedElements()));
                array_lo = adtypep->lo();
                putbs(", ");
                puts(cvtToStr(array_lo));
            } وگرنه {
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
            iterateAndNextConstNull(nodep->memp());
            اگر (memUsePtr) puts(")");
        }
        putbs(", ");
        اگر (nodep->lsbp()) {
            iterateAndNextConstNull(nodep->lsbp());
        } وگرنه {
            puts(cvtToStr(array_lo));
        }
        putbs(", ");
        اگر (nodep->msbp()) {
            iterateAndNextConstNull(nodep->msbp());
        } وگرنه {
            puts("~0ULL");
        }
        puts(");\n");
    }
    تهی visit(AstFClose* nodep) override {
        putns(nodep, "VL_FCLOSE_I(");
        iterateAndNextConstNull(nodep->filep());
        puts("); ");
    }
    تهی visit(AstFFlush* nodep) override {
        اگر (!nodep->filep()) {
            putns(nodep, "VL_FFLUSH_MT();");
        } وگرنه {
            putns(nodep, "VL_FFLUSH_I(");
            iterateAndNextConstNull(nodep->filep());
            puts(");\n");
        }
    }
    تهی visit(AstFSeek* nodep) override {
        putns(nodep, "(VL_FSEEK_I(");
        iterateAndNextConstNull(nodep->filep());
        puts(", ");
        iterateAndNextConstNull(nodep->offset());
        puts(", ");
        iterateAndNextConstNull(nodep->operation());
        puts(") == -1 ? -1 : 0)");
    }
    تهی visit(AstFTell* nodep) override {
        putns(nodep, "VL_FTELL_I(");
        iterateAndNextConstNull(nodep->filep());
        puts(")");
    }
    تهی visit(AstFRewind* nodep) override {
        putns(nodep, "(VL_FSEEK_I(");
        iterateAndNextConstNull(nodep->filep());
        puts(", 0, 0) == -1 ? -1 : 0)");
    }
    تهی visit(AstFRead* nodep) override {
        putns(nodep, "VL_FREAD_I(");
        puts(cvtToStr(nodep->memp()->widthMin()));  // Need real storage width
        putbs(", ");
        uint32_t array_lo = 0;
        uint32_t array_size = 0;
        {
            ثابت AstVarRef* ثابت varrefp = VN_CAST(nodep->memp(), VarRef);
            اگر (!varrefp) {
                nodep->v3error(nodep->verilogKwd() << " loading non-variable");
            } وگرنه اگر (VN_CAST(varrefp->varp()->dtypeSkipRefp(), BasicDType)) {
            } وگرنه اگر (ثابت AstUnpackArrayDType* ثابت adtypep
                       = VN_CAST(varrefp->varp()->dtypeSkipRefp(), UnpackArrayDType)) {
                array_lo = adtypep->lo();
                array_size = adtypep->elementsConst();
            } وگرنه {
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
        اگر (nodep->startp()) {
            iterateAndNextConstNull(nodep->startp());
        } وگرنه {
            puts(cvtToStr(array_lo));
        }
        putbs(", ");
        اگر (nodep->countp()) {
            iterateAndNextConstNull(nodep->countp());
        } وگرنه {
            puts(cvtToStr(array_size));
        }
        puts(")");
    }
    تهی visit(Ast_پشته_Trace_F* nodep) override { putns(nodep, "VL_STACKTRACE_N()"); }
    تهی visit(Ast_پشته_Trace_T* nodep) override { putns(nodep, "VL_STACKTRACE();\n"); }
    تهی visit(AstSystemT* nodep) override {
        putns(nodep, "(void)VL_SYSTEM_I");
        emitIQW(nodep->lhsp());
        puts("(");
        اگر (nodep->lhsp()->isWide()) {
            puts(cvtToStr(nodep->lhsp()->width_واژگان()));
            putbs(", ");
        }
        iterateAndNextConstNull(nodep->lhsp());
        puts(");\n");
    }
    تهی visit(AstSystemF* nodep) override {
        putns(nodep, "VL_SYSTEM_I");
        emitIQW(nodep->lhsp());
        puts("(");
        اگر (nodep->lhsp()->isWide()) {
            puts(cvtToStr(nodep->lhsp()->width_واژگان()));
            putbs(", ");
        }
        iterateAndNextConstNull(nodep->lhsp());
        puts(")");
    }
    تهی visit(Ast_Stmt_عبارت* nodep) override {
        اگر (VN_IS(nodep->exprp()->dtypep()->skipRefp(), VoidDType)) {
            putns(nodep, "");
        } وگرنه {
            putns(nodep, "std::ignore = ");
        }
        iterateConst(nodep->exprp());
        puts(";\n");
    }
    تهی visit(AstJumpBlock* nodep) override {
        // Allocate label number
        ثابت size_t n = m_labelNumbers.size();
        ثابت bool newEntry = m_labelNumbers.emplace(nodep, n).second;
        UASSERT_OBJ(newEntry, nodep, "AstJumpBlock visited twide");
        // Emit
        putns(nodep, "{\n");  // Make it visually obvious label jumps outside these
        VL_RESTORER(m_createdScopeHash);
        iterateAndNextConstNull(nodep->stmtsp());
        puts("__Vlabel" + std::to_string(n) + ": ;\n");
        puts("}\n");
    }
    تهی visit(AstJumpGo* nodep) override {
        // Retrieve target label number - must already exist (from enclosing AstJumpBlock)
        ثابت size_t n = m_labelNumbers.at(nodep->blockp());
        // Emit
        putns(nodep, "goto __Vlabel" + std::to_string(n) + ";\n");
    }
    تهی visit(AstLoop* nodep) override {
        UASSERT_OBJ(!nodep->contsp(), nodep, "'contsp' only used before LinkJump");
        VL_RESTORER(m_createdScopeHash);
        // Special case when the AstLoopTest is first for output readability
        اگر (AstLoopTest* ثابت testp = VN_CAST(nodep->stmtsp(), LoopTest)) {
            putns(nodep, "while (");
            iterateConst(testp->condp());
            puts(") {\n");
            iterateAndNextConstNull(testp->nextp());
            puts("}\n");
            بازگشت;
        }
        // Special case when the AstLoopTest is last for output readability
        اگر (Ast_گره* lastp = nodep->stmtsp()) {
            وقتی (Ast_گره* ثابت nextp = lastp->nextp()) lastp = nextp;
            اگر (AstLoopTest* ثابت testp = VN_CAST(lastp, LoopTest)) {
                putns(nodep, "do {\n");
                برای (Ast_گره* p = nodep->stmtsp(); p != lastp; p = p->nextp()) iterateConst(p);
                puts("} while (");
                iterateConst(testp->condp());
                puts(");\n");
                بازگشت;
            }
        }
        // Emit generic case directly
        putns(nodep, "while (true) {\n");
        iterateAndNextConstNull(nodep->stmtsp());
        puts("}\n");
    }
    تهی visit(AstLoopTest* nodep) override {
        VL_RESTORER(m_createdScopeHash);
        putns(nodep, "if (!(");
        iterateAndNextConstNull(nodep->condp());
        puts(")) break;\n");
    }
    تهی visit(AstCLocalScope* nodep) override {
        putns(nodep, "{\n");
        VL_RESTORER(m_createdScopeHash);
        iterateAndNextConstNull(nodep->stmtsp());
        puts("}\n");
    }
    تهی visit(Ast_گره_If* nodep) override {
        putns(nodep, "if (");
        اگر (!nodep->branchPred().unknown()) {
            puts(nodep->branchPred().ascii());
            puts("((");  // Two parens, so that VL_UNLIKELY((class<foo,bar>)) works
        }
        iterateAndNextConstNull(nodep->condp());
        اگر (!nodep->branchPred().unknown()) puts("))");
        puts(") {\n");
        {
            VL_RESTORER(m_createdScopeHash);
            iterateAndNextConstNull(nodep->thensp());
        }
        puts("}");
        اگر (!nodep->elsesp()) {
            puts("\n");
        } وگرنه {
            اگر (VN_IS(nodep->elsesp(), گره_If) && !nodep->elsesp()->nextp()) {
                puts(" else ");
                iterateAndNextConstNull(nodep->elsesp());
            } وگرنه {
                VL_RESTORER(m_createdScopeHash);
                puts(" else {\n");
                iterateAndNextConstNull(nodep->elsesp());
                puts("}\n");
            }
        }
    }
    تهی visit(Ast_عبارت_Stmt* nodep) override {
        VL_RESTORER(m_createdScopeHash);
        ثابت bool containsAwait = nodep->exists([](AstCAwait*) -> bool { بازگشت true; });
        اگر (containsAwait) {
            UASSERT_OBJ(m_cfuncp && m_cfuncp->isCoroutine(), nodep,
                        "AstExprStmt with CAwait must be in coroutine");
            putnbs(nodep, "(co_await ([&]() -> VlCoroutine {\n");
            iterateAndNextConstNull(nodep->stmtsp());
            puts("co_return;\n");
            اگر (!nodep->hasResult()) {
                puts("}()))");
                بازگشت;
            }
            puts("}()), ");
            iterateAndNextConstNull(nodep->resultp());
            puts(")");
            بازگشت;
        }
        // GCC allows compound statements in expressions, but this is not standard.
        // So we use an immediate-evaluation lambda and comma operator
        putnbs(nodep, "([&]() {\n");
        اگر (!nodep->hasResult()) {
            iterateAndNextConstNull(nodep->stmtsp());
            puts("}())");
            بازگشت;
        }
        iterateAndNextConstNull(nodep->stmtsp());
        puts("}(), ");
        iterateAndNextConstNull(nodep->resultp());
        puts(")");
    }
    تهی visit(AstStop* nodep) override {
        putns(nodep, "VL_STOP_MT(");
        putsQuoted(protect(nodep->fileline()->filename()));
        puts(", ");
        puts(cvtToStr(nodep->fileline()->lineno()));
        puts(", \"\"");
        اگر (nodep->isFatal()) puts(", false");
        puts(");\n");
    }
    تهی visit(AstFinish* nodep) override {
        // Disable all the forks so they don't operate after simulation is finished.
        اگر (m_cfuncp && m_cfuncp->needProcess()) putns(nodep, "vlProcess->disableFork();\n");
        putns(nodep, "VL_FINISH_MT(");
        putsQuoted(protect(nodep->fileline()->filename()));
        puts(", ");
        puts(cvtToStr(nodep->fileline()->lineno()));
        puts(", \"\");\n");
    }
    تهی visit(AstFinishFork* nodep) override {
        // Disable all the forks so they don't operate after simulation is finished.
        اگر (m_cfuncp && m_cfuncp->needProcess()) putns(nodep, "vlProcess->disableFork();\n");
        putns(nodep, "VL_FINISH_MT(");
        putsQuoted(protect(nodep->fileline()->filename()));
        puts(", ");
        puts(cvtToStr(nodep->fileline()->lineno()));
        puts(", \"\");\n");
        اگر (m_cfuncp->isCoroutine()) {
            putns(nodep, "co_return;\n");
        } وگرنه {
            putns(nodep, "return;\n");
        }
    }
    تهی visit(AstPrintTimeScale* nodep) override {
        putns(nodep, "VL_PRINTTIMESCALE(");
        putsQuoted(protect(nodep->prettyName()));
        puts(", ");
        putsQuoted(nodep->timeunit().ascii());
        puts(", vlSymsp->_vm_contextp__);\n");
    }
    تهی visit(AstRand* nodep) override {
        emitOpName(nodep, nodep->emitC(), nodep->seedp(), nullptr, nullptr);
    }
    تهی visit(AstRandRNG* nodep) override {
        emitOpName(nodep, nodep->emitC(), nullptr, nullptr, nullptr);
    }
    تهی visit(AstTime* nodep) override {
        putns(nodep, "VL_TIME_UNITED_Q(");
        UASSERT_OBJ(!nodep->timeunit().isNone(), nodep, "$time has no units");
        ثابت دوگانه time
            = nodep->timeunit().multiplier() / v3Global.rootp()->timeprecision().multiplier();
        UASSERT_OBJ(time >= 1, nodep, "TimeQ is less than 1, will result in division by zero");
        puts(cvtToStr(time));
        puts(")");
    }
    تهی visit(AstTimeD* nodep) override {
        putns(nodep, "VL_TIME_UNITED_D(");
        UASSERT_OBJ(!nodep->timeunit().isNone(), nodep, "$realtime has no units");
        puts(cvtToStr(nodep->timeunit().multiplier()
                      / v3Global.rootp()->timeprecision().multiplier()));
        puts(")");
    }
    تهی visit(AstGetInitialRandomSeed* nodep) override {
        putns(nodep, "vlSymsp->_vm_contextp__->randSeed()");
    }
    تهی visit(AstTimeFormat* nodep) override {
        putns(nodep, "VL_TIMEFORMAT_IINI(");
        اگر (nodep->unitsp()) {
            puts("true, ");
            iterateAndNextConstNull(nodep->unitsp());
        } وگرنه {
            puts("false, 0");
        }
        puts(", ");
        اگر (nodep->precisionp()) {
            puts("true, ");
            iterateAndNextConstNull(nodep->precisionp());
        } وگرنه {
            puts("false, 0");
        }
        puts(", ");
        اگر (nodep->suffixp()) {
            puts("true, ");
            iterateAndNextConstNull(nodep->suffixp());
        } وگرنه {
            puts("false, \"\"");
        }
        puts(", ");
        اگر (nodep->widthp()) {
            puts("true, ");
            iterateAndNextConstNull(nodep->widthp());
        } وگرنه {
            puts("false, 0");
        }
        puts(", vlSymsp->_vm_contextp__);\n");
    }
    تهی visit(AstTimePrecision* nodep) override {
        putns(nodep, "vlSymsp->_vm_contextp__->timeprecision()");
    }

    // Nodes involing AstText
    تهی visit(AstText* nodep) override {
        // All Text should be under TextBlock/CStmt/CStmtUser/CExpr/CExprUser
        nodep->v3fatalSrc("Text node in unexpected position");
    }
    تهی visit(AstTextBlock* nodep) override {
        putnbs(nodep, "");
        puts(nodep->prefix());
        emitNodesWithText(nodep->nodesp(), false, true, nodep->separator());
        puts(nodep->suffix());
    }
    تهی visit(AstCStmt* nodep) override {
        putnbs(nodep, "");
        emitNodesWithText(nodep->nodesp(), false, true, "");
        ensure_New_سطر();
    }
    تهی visit(AstCExpr* nodep) override {
        putnbs(nodep, "");
        emitNodesWithText(nodep->nodesp(), false, true, "");
    }
    تهی visit(AstCStmtUser* nodep) override {
        putnbs(nodep, "");
        ofp()->putsNoTracking("\n");
        اگر (nodep->fromDollarC() && v3Global.opt.decoration() && !v3Global.opt.protectIds()) {
            ofp()->putsNoTracking("// $c statement at " + nodep->fileline()->ascii() + "\n");
        }
        emitNodesWithText(nodep->nodesp(), m_useSelfForThis, false, "");
        puts("\n");
    }
    تهی visit(AstCExprUser* nodep) override {
        putnbs(nodep, "");
        ofp()->putsNoTracking("\n");
        اگر (/* is always from $c */ v3Global.opt.decoration() && !v3Global.opt.protectIds()) {
            ofp()->putsNoTracking(
                (nodep->isPure() ? "// $cpure expression at " : "// $c expression at ")
                + nodep->fileline()->ascii() + "\n");
        }
        emitNodesWithText(nodep->nodesp(), m_useSelfForThis, false, "");
        puts("\n");
    }

    // Operators
    تهی visit(Ast_گره_Termop* nodep) override {
        emitOpName(nodep, nodep->emitC(), nullptr, nullptr, nullptr);
    }
    تهی visit(Ast_گره_Uniop* nodep) override {
        اگر (nodep->emit_Check_Max_واژگان()
            && (nodep->width_واژگان() > VL_MULS_MAX_واژگان
                || nodep->lhsp()->width_واژگان() > VL_MULS_MAX_واژگان)) {
            nodep->v3warn(
                E_UNSUPPORTED,
                "Unsupported: "
                    << nodep->prettyOperatorName() << " operator of " << nodep->width()
                    << " bits exceeds hardcoded limit VL_MULS_MAX_WORDS in verilatedos.h");
        }
        اگر (emitSimpleOk(nodep)) {
            putnbs(nodep, "(");
            puts(nodep->emitSimpleOperator());
            puts(" ");
            iterateAndNextConstNull(nodep->lhsp());
            puts(")");
        } وگرنه {
            emitOpName(nodep, nodep->emitC(), nodep->lhsp(), nullptr, nullptr);
        }
    }
    تهی visit(Ast_گره_Biop* nodep) override {
        اگر (nodep->emit_Check_Max_واژگان() && nodep->width_واژگان() > VL_MULS_MAX_واژگان) {
            nodep->v3warn(
                E_UNSUPPORTED,
                "Unsupported: "
                    << nodep->prettyOperatorName() << " operator of " << nodep->width()
                    << " bits exceeds hardcoded limit VL_MULS_MAX_WORDS in verilatedos.h");
        }
        اگر (emitSimpleOk(nodep)) {
            putnbs(nodep, "(");
            iterateAndNextConstNull(nodep->lhsp());
            puts(" ");
            putbs(nodep->emitSimpleOperator());
            puts(" ");
            iterateAndNextConstNull(nodep->rhsp());
            puts(")");
        } وگرنه {
            emitOpName(nodep, nodep->emitC(), nodep->lhsp(), nodep->rhsp(), nullptr);
        }
    }
    تهی visit(Ast_گره_Triop* nodep) override {
        UASSERT_OBJ(!emitSimpleOk(nodep), nodep, "Triop cannot be described in a simple way");
        emitOpName(nodep, nodep->emitC(), nodep->lhsp(), nodep->rhsp(), nodep->thsp());
    }
    تهی visit(AstCvtPackString* nodep) override { emitCvtPackStr(nodep->lhsp()); }
    تهی visit(AstRedXor* nodep) override {
        اگر (nodep->lhsp()->isWide()) {
            visit(static_cast<Ast_گره_Uniop*>(nodep));
        } وگرنه {
            ثابت AstVarRef* ثابت vrefp = VN_CAST(nodep->lhsp(), VarRef);
            ثابت صحیح widthPow2 = vrefp ? vrefp->varp()->dtypep()->widthPow2()
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
    تهی visit(AstCCast* nodep) override {
        // Extending a value of the same word width is just a NOP.
        اگر (ثابت AstClassRefDType* ثابت classDtypep
            = VN_CAST(nodep->dtypep()->skipRefp(), ClassRefDType)) {
            putns(nodep, "(" + classDtypep->cType("", false, false) + ")(");
        } وگرنه اگر (nodep->size() <= VL_BYTESIZE) {
            putns(nodep, "(::CData)(");
        } وگرنه اگر (nodep->size() <= VL_SHORTSIZE) {
            putns(nodep, "(::SData)(");
        } وگرنه اگر (nodep->size() <= VL_IDATASIZE) {
            putns(nodep, "(::IData)(");
        } وگرنه {
            putns(nodep, "(::QData)(");
        }
        iterateAndNextConstNull(nodep->lhsp());
        puts(")");
    }
    تهی visit(AstCond* nodep) override {
        // Widths match up already, so we'll just use C++'s operator w/o any temps.
        اگر (nodep->thenp()->isWide()) {
            emitOpName(nodep, nodep->emitC(), nodep->condp(), nodep->thenp(), nodep->elsep());
        } وگرنه {
            putnbs(nodep, "(");
            iterateAndNextConstNull(nodep->condp());
            putbs(" ? ");
            iterateAndNextConstNull(nodep->thenp());
            putbs(" : ");
            iterateAndNextConstNull(nodep->elsep());
            puts(")");
        }
    }
    تهی visit(AstMatchMasked* nodep) override {
        emitOpName(nodep, nodep->emitC(), nodep->lhsp(), nodep->matchp(), nullptr);
    }
    تهی visit(AstMemberSel* nodep) override {
        ثابت AstVar* ثابت varp = nodep->varp();
        ثابت bool dereferenceCovergroupRef
            = varp->covergroupRefMember() && nodep->access().isReadOrRW();
        اگر (dereferenceCovergroupRef) putnbs(nodep, "(*");
        iterateAndNextConstNull(nodep->fromp());
        putnbs(nodep, "->");
        اگر (varp->isIfaceRef()) {
            // varp is the __Viftop companion (e.g. "tx__Viftop"); use the
            // MemberSel name which matches the cell's C++ member (e.g. "tx").
            puts(nodep->nameProtect());
        } وگرنه {
            puts(varName(varp));
        }
        اگر (dereferenceCovergroupRef) puts(")");
    }
    تهی visit(AstStructSel* nodep) override {
        iterateAndNextConstNull(nodep->fromp());
        putnbs(nodep, ".");
        puts(nodep->nameProtect());
    }
    تهی visit(AstNullCheck* nodep) override {
        putns(nodep, "VL_NULL_CHECK(");
        iterateAndNextConstNull(nodep->lhsp());
        puts(", ");
        putsQuoted(protect(nodep->fileline()->filename()));
        puts(", ");
        puts(cvtToStr(nodep->fileline()->lineno()));
        puts(")");
    }
    تهی visit(AstNewCopy* nodep) override {
        // Polymorphic shallow clone: preserves runtime type via virtual clone()
        // VL_NULL_CHECK enforces null check per IEEE 1800-2023 8.7
        putns(nodep, "VL_NULL_CHECK(");
        اگر (VN_IS(nodep->rhsp(), Const) && VN_AS(nodep->rhsp(), Const)->isNull()) {
            // V3Const folded rhs to null: emit a typed empty ref so VL_NULL_CHECK fires
            ثابت AstClassRefDType* ثابت refDTypep
                = VN_CAST(nodep->dtypep()->skipRefp(), ClassRefDType);
            puts(refDTypep->cType("", false, false) + "{}");
        } وگرنه {
            iterateAndNextConstNull(nodep->rhsp());
        }
        puts(", ");
        putsQuoted(protect(nodep->fileline()->filename()));
        puts(", ");
        puts(cvtToStr(nodep->fileline()->lineno()));
        puts(").clone(vlSymsp->__Vm_deleter)");
    }
    تهی visit(AstSel* nodep) override {
        // Note ASSIGN checks for this on a LHS
        UASSERT_OBJ(nodep->widthMin() == nodep->widthConst(), nodep, "Width mismatch");
        emitOpName(nodep, nodep->emitC(), nodep->fromp(), nodep->lsbp(), nullptr);
    }
    تهی visit(AstReplicate* nodep) override {
        اگر (nodep->srcp()->widthMin() == 1 && !nodep->isWide()) {
            UASSERT_OBJ((static_cast<صحیح>(VN_AS(nodep->countp(), Const)->toUInt())
                         * nodep->srcp()->widthMin())
                            == nodep->widthMin(),
                        nodep, "Replicate non-constant or width miscomputed");
            putns(nodep, "VL_REPLICATE_");
            emitIQW(nodep);
            puts("OI(");
            اگر (nodep->srcp()) puts(cvtToStr(nodep->srcp()->widthMin()));
            puts(", ");
            iterateAndNextConstNull(nodep->srcp());
            puts(", ");
            iterateAndNextConstNull(nodep->countp());
            puts(")");
        } وگرنه {
            emitOpName(nodep, nodep->emitC(), nodep->srcp(), nodep->countp(), nullptr);
        }
    }
    تهی emitStreamR(AstStreamR* nodep, Ast_گره* parent) {
        //TODO: This might need to handle more cases like the visit(AstStreamR) function
        emitOpName(nodep, nodep->emitC(), nodep->lhsp(), nodep->rhsp(), nullptr);
    }
    تهی visit(AstStreamR* nodep) override {
        //The parrent node of our AstStreamR will give just enough info for what streamR should
        //output if nodep->backp() is not the parent then emitStreamR should have been used. throw
        //an error
        bool backpIsParent = (nodep->backp()->op1p() == nodep || nodep->backp()->op2p() == nodep);
        UASSERT(backpIsParent, "can not find return type for streamR");
        اگر (ثابت AstQueueDType* ثابت qdtp
            = VN_CAST(nodep->backp()->dtypep()->skipRefp(), QueueDType)) {
            اگر (nodep->lhsp()->isWide()) {
                emitOpName(nodep,
                           "VL_STREAMR_%nq%lq%rq(%lw," + std::to_string(qdtp->subDTypep()->width())
                               + ", %P, %li, %ri)",
                           nodep->lhsp(), nodep->rhsp(), nullptr);
            } وگرنه {
                emitOpName(nodep, "VL_STREAMR_%nq%lq%rq(%lw, %P, %li, %ri)", nodep->lhsp(),
                           nodep->rhsp(), nullptr);
            }
        } وگرنه اگر (VN_IS(nodep->lhsp()->dtypep()->skipRefp(), QueueDType)) {
            اگر (!((nodep->backp()->op1p() && nodep->backp()->op1p()->isWide())
                  || (nodep->backp()->op2p() && nodep->backp()->op2p()->isWide()))) {
                //If our lhsp is a queue make sure we streamR and return the correct type.
                //If either side is wide or the previous node is string type dont use this case
                emitOpName(nodep->backp(), "VL_STREAMR_%nq%lq%rq(%lw, %P, %li, %ri)",
                           nodep->lhsp(), nodep->rhsp(), nullptr);
            } وگرنه {
                emitOpName(nodep, nodep->emitC(), nodep->lhsp(), nodep->rhsp(), nullptr);
            }
        } وگرنه {
            emitOpName(nodep, nodep->emitC(), nodep->lhsp(), nodep->rhsp(), nullptr);
        }
    }
    تهی visit(AstStreamL* nodep) override {
        // Attempt to use a "fast" stream function for slice size = power of 2
        اگر (!nodep->isWide()) {
            ثابت uint32_t isPow2 = VN_AS(nodep->rhsp(), Const)->رقم().شمار_Ones() == 1;
            ثابت uint32_t sliceSize = VN_AS(nodep->rhsp(), Const)->toUInt();
            اگر (isPow2 && sliceSize <= (nodep->isQuad() ? اندازه(uint64_t) : اندازه(uint32_t))) {
                putns(nodep, "VL_STREAML_FAST_");
                bool usesQueue = false;
                AstQueueDType* qtypep
                    = nodep->backp()->op2p()
                          ? VN_CAST(nodep->backp()->op2p()->dtypep()->skipRefp(), QueueDType)
                          : nullptr;
                اگر (VN_IS(nodep->backp(), Assign)
                    && qtypep) {  // If we are assigning to a queue then emit the correct symbol
                    puts("R");  // R for queue
                    usesQueue = true;
                } وگرنه {
                    emitIQW(nodep);
                }
                emitIQW(nodep->lhsp());
                puts("I(");
                puts(cvtToStr(nodep->lhsp()->widthMin()));
                puts(", ");
                اگر (usesQueue) {
                    iterateAndNextConstNull(nodep->backp()->op2p());
                    puts(", ");
                }
                iterateAndNextConstNull(nodep->lhsp());
                puts(", ");
                ثابت uint32_t rd_log2 = V3_عدد::log2b(VN_AS(nodep->rhsp(), Const)->toUInt());
                puts(cvtToStr(rd_log2) + ")");
                بازگشت;
            }
        }
        اگر (VN_IS(nodep->backp(), Assign)
            && VN_IS(nodep->backp()->op2p()->dtypep()->skipRefp(), QueueDType)) {
            صحیح queueWidth
                = nodep->backp()->op2p()->dtypep()->subDTypep()->width();  //We need to know the
                                                                           //width of both sides
            emitOpName(nodep,
                       "VL_STREAML_%nq%lq%rq(%lw," + std::to_string(queueWidth)
                           + ", %P, %li, %ri)",
                       nodep->lhsp(), nodep->rhsp(), nullptr);
        } وگرنه {
            emitOpName(nodep, nodep->emitC(), nodep->lhsp(), nodep->rhsp(), nullptr);
        }
    }
    تهی visit(AstCastDynamic* nodep) override {
        putnbs(nodep, "VL_CAST_DYNAMIC(");
        iterateAndNextConstNull(nodep->fromp());
        puts(", ");
        iterateAndNextConstNull(nodep->بالاترین());
        puts(")");
    }
    تهی visit(Ast_شمار_Bits* nodep) override {
        putnbs(nodep, "VL_COUNTBITS_");
        emitIQW(nodep->lhsp());
        puts("(");
        puts(cvtToStr(nodep->lhsp()->widthMin()));
        puts(", ");
        اگر (nodep->lhsp()->isWide()) {
            puts(cvtToStr(nodep->lhsp()->width_واژگان()));  // Note argument width, not node width
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
    تهی visit(AstTriggered* nodep) override {
        // Test each word of the trigger vector with a mask of the relevant bits
        std::map<uint32_t, uint64_t> wordMasks;
        برای (ثابت uint32_t index : nodep->indices()) {
            wordMasks[VL_BITWORD_Q(index)] |= VL_BIT_Q(index);
        }
        putns(nodep, "((");
        bool first = true;
        برای (ثابت auto& wordMask : wordMasks) {
            اگر (!first) puts("\n| ");
            first = false;
            ثابت bool allOnes = wordMask.second == ~0ULL;
            اگر (!allOnes) puts("(" + V3_عدد{nodep, 64}.setQuad(wordMask.second).emitC() + " & ");
            iterateConst(nodep->trigRefp());
            puts("[" + std::to_string(wordMask.first) + "U]");
            اگر (!allOnes) puts(")");
        }
        puts(") != 0ULL)");
    }
    // Terminals
    تهی visit(AstVarRef* nodep) override {
        ثابت AstVar* ثابت varp = nodep->varp();

        // Reference to constant pool variable is special
        اگر (varp->constPoolEntry()) {
            putns(nodep, EmitCUtil::constPoolName(varp));
            بازگشت;
        }

        ثابت Ast_گره_Module* ثابت varModp = EmitCParentModule::get(varp);
        ثابت bool dereferenceCovergroupRef
            = varp->covergroupRefMember() && nodep->access().isReadOrRW();
        اگر (dereferenceCovergroupRef) putns(nodep, "(*");
        اگر (VN_IS(varModp, Class) && varModp != m_modp) {
            // Superclass member reference
            putns(nodep, EmitCUtil::prefixNameProtect(varModp) + "::");
        } وگرنه اگر (varp->isIfaceRef()) {
            putns(nodep, nodep->selfPointerProtect(m_useSelfForThis));
            بازگشت;
        } وگرنه اگر (!nodep->selfPointer().isEmpty()) {
            emitDereference(nodep, nodep->selfPointerProtect(m_useSelfForThis));
        }
        putns(nodep, varName(varp));
        اگر (dereferenceCovergroupRef) puts(")");
    }
    تهی visit(AstAddrOfCFunc* nodep) override {
        // Note: Can be thought to handle more, but this is all that is needed right now
        ثابت AstCFunc* ثابت funcp = nodep->funcp();
        UASSERT_OBJ(funcp->isLoose(), nodep, "Cannot take address of non-loose method");
        putns(nodep, "&");
        puts(funcNameProtect(funcp));
    }
    تهی visit(AstConst* nodep) override {  //
        اگر (m_wideTempRefp && nodep->isWide()) {
            UASSERT_OBJ(m_wideTempRefp, nodep, "Wide Constant w/ no temp");
            emitConstantW(nodep, m_wideTempRefp);
            m_wideTempRefp = nullptr;  // We used it, fail if set it a second time
        } وگرنه {
            emitConstant(nodep);
        }
    }
    تهی visit(Ast_گره_Cons* nodep) override { emitValCons(nodep); }
    تهی visit(AstThisRef* nodep) override {
        putnbs(nodep, nodep->dtypep()->cType("", false, false));
        puts("{");
        puts(VSelfPointerText::replaceThis(m_useSelfForThis, "this"));
        puts("}");
    }
    تهی visit(Ast_گره_Sel* nodep) override {
        اگر (!VN_IS(nodep, ArraySel) && !VN_IS(nodep, WordSel)) {
            visit(static_cast<Ast_گره_Biop*>(nodep));
            بازگشت;
        }
        // ArraySel or WordSel
        iterateAndNextConstNull(nodep->fromp());
        // Special case constant index for readability
        اگر (AstConst* ثابت idxp = VN_CAST(nodep->bitp(), Const)) {
            puts("[" + std::to_string(idxp->toUInt()) + "U]");
            بازگشت;
        }
        putbs("[");
        iterateAndNextConstNull(nodep->bitp());
        puts("]");
    }

    //
    تهی visit(AstExecGraph* nodep) override {
        // The location of the AstExecGraph within the containing AstCFunc is where we want to
        // invoke the graph and wait for it to complete. The run-time library does just that.
        putns(nodep, "vl_invokeExecGraph(" + EmitCUtil::execGraphName(nodep) + ", vlSelf, ");
        iterateConst(nodep->triggerp());
        puts(".data());\n");
    }

    // Default
    تهی visit(Ast_گره* nodep) override {  // LCOV_EXCL_START
        putns(nodep, "\n???? // "s + nodep->prettyTypeName() + "\n");
        iterateChildrenConst(nodep);
        اگر (!v3Global.opt.lintOnly()) {  // An internal problem, so suppress
            nodep->v3fatalSrc("Unknown node type reached emitter: " << nodep->prettyTypeName());
        }
    }  // LCOV_EXCL_STOP

محافظتشده:
    EmitCFunc() = پیشفرض;
    ~EmitCFunc() override = پیشفرض;
};

#endif  // guard
