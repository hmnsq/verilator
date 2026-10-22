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

#अगर_अघोषित VERILATOR_V3EMITCFUNC_H_
#घोषणा VERILATOR_V3EMITCFUNC_H_

#समावेश "config_build.h"
#समावेश "verilatedos.h"

#समावेश "V3EmitCConstInit.h"
#समावेश "V3Global.h"
#समावेश "V3MemberMap.h"

#समावेश <algorithm>
#समावेश <map>
#समावेश <unordered_map>
#समावेश <unordered_set>
#समावेश <vector>

// Number of VL_CONST_W_*X's in verilated.h (IE VL_CONST_W_8X is last)
constexpr पूर्णांक EMITC_अंक_CONSTW = 8;

//######################################################################
// Emit lazy forward declarations

class EmitCLazyDecls final : खुला VNVisitorConst {
    // NODE STATE/TYPES
    // None allowed to support threaded emitting

    // MEMBERS
    std::unordered_set<string> m_emittedManually;  // Set of names already declared manually.
    EmitCBaseVisitorConst& m_emitter;  // For access to file output
    bool m_needs_Blank_पंक्ति = false;  // Emit blank line if any declarations were emitted (cosmetic)
    std::set<Ast_गाँठ*> m_emitted;  // -> in set. Already emitted decl for symbols.

    // METHODS
    bool declaredOnce(Ast_गाँठ* nodep) { वापस m_emitted.जोड़ो(nodep).second; }

    व्योम lazyDeclare(AstCFunc* funcp) {
        // Already declared in this compilation unit
        अगर (!declaredOnce(funcp)) वापस;
        // Check if this kind of function is lazily declared
        अगर (!(funcp->isMethod() && funcp->isLoose()) && !funcp->dpiImportPrototype()) वापस;
        // Already declared manually
        अगर (m_emittedManually.गिनती(funcp->nameProtect())) वापस;
        // Needs lazy declaration, emit one
        m_emitter.emitCFuncDecl(funcp, EmitCParentModule::get(funcp), funcp->dpiImportPrototype());
        m_needs_Blank_पंक्ति = true;
    }

    व्योम lazyDeclareConstPoolVar(AstVar* varp) {
        अगर (!declaredOnce(varp)) वापस;  // Already declared
        स्थिर string nameProtect = EmitCUtil::constPoolName(varp);
        m_emitter.putns(varp, "extern const ");
        m_emitter.माला_दो(varp->dtypep()->cType(nameProtect, false, false));
        m_emitter.माला_दो(";\n");
        m_needs_Blank_पंक्ति = true;
    }

    // VISITORS
    व्योम visit(Ast_गाँठ_CCall* nodep) override {
        lazyDeclare(nodep->funcp());
        iterateChildrenConst(nodep);
    }

    व्योम visit(AstAddrOfCFunc* nodep) override {
        lazyDeclare(nodep->funcp());
        iterateChildrenConst(nodep);
    }

    व्योम visit(AstVarRef* nodep) override {
        AstVar* स्थिर varp = nodep->varp();
        // Only constant pool symbols are lazy declared for now ...
        अगर (EmitCParentModule::get(varp)->isConstPool()) lazyDeclareConstPoolVar(varp);
    }

    व्योम visit(AstExecGraph* nodep) override {
        अगर (!declaredOnce(nodep)) वापस;
        m_emitter.putns(nodep, "extern VlExecGraph " + EmitCUtil::execGraphName(nodep) + ";\n");
        m_needs_Blank_पंक्ति = true;
    }

    व्योम visit(Ast_गाँठ* nodep) override { iterateChildrenConst(nodep); }

खुला:
    explicit EmitCLazyDecls(EmitCBaseVisitorConst& emitter)
        : m_emitter{emitter} {}
    व्योम emit(Ast_गाँठ* nodep) {
        m_needs_Blank_पंक्ति = false;
        iterateChildrenConst(nodep);
        अगर (m_needs_Blank_पंक्ति) m_emitter.माला_दो("\n");
    }
    व्योम emit(स्थिर string& prefix, स्थिर string& name, स्थिर string& suffix) {
        m_emittedManually.जोड़ो(name);
        m_emitter.ensure_New_पंक्ति();
        m_emitter.माला_दो(prefix);
        m_emitter.माला_दो(name);
        m_emitter.माला_दो(suffix);
        m_emitter.ensure_New_पंक्ति();
    }
    व्योम declared(AstCFunc* nodep) { m_emitted.जोड़ो(nodep); }
    व्योम reset() { m_emitted.clear(); }
};

// ######################################################################
//  Emit statements and expressions

class EmitCFunc VL_NOT_FINAL : खुला EmitCConstInit {
    VMemberMap m_memberMap;
    AstVarRef* m_wide_अस्थायी_Refp = nullptr;  // Variable that _WW macros should be setting
    std::unordered_map<AstJumpBlock*, माप_प्रकार> m_labelNumbers;  // Label numbers for AstJumpBlocks
    bool m_createdScopeHash = false;  // Already created a scope hash

रक्षित:
    VL_DEFINE_DEBUG_FUNCTIONS;

    EmitCLazyDecls m_lazyDecls{*this};  // Visitor for emitting lazy declarations
    bool m_useSelfForThis = false;  // Replace "this" with "vlSelf"
    bool m_usevlSelfRef = false;  // Use vlSelfRef reference instead of vlSelf pointer
    स्थिर Ast_गाँठ_Module* m_modp = nullptr;  // Current module being emitted
    स्थिर AstCFunc* m_cfuncp = nullptr;  // Current function being emitted
    bool m_instantiatesOwnProcess = false;
    स्थिर AstClassPackage* m_classOrPackage = nullptr;  // Pointer to current class or package
    string m_classOrPackageHash;  // Hash of class or package name

    bool constructorNeedsProcess(स्थिर AstClass* स्थिर classp) {
        स्थिर Ast_गाँठ* स्थिर newp = m_memberMap.findMember(classp, "new");
        अगर (!newp) वापस false;
        स्थिर AstCFunc* स्थिर ctorp = VN_CAST(newp, CFunc);
        अगर (!ctorp) वापस false;
        UASSERT_OBJ(ctorp->isConstructor(), ctorp, "`new` is not a constructor!");
        वापस ctorp->needProcess();
    }

    bool constructorNeedsProcess(स्थिर Ast_गाँठ_DType* स्थिर dtypep) {
        अगर (स्थिर AstClassRefDType* स्थिर crefdtypep = VN_CAST(dtypep, ClassRefDType))
            वापस constructorNeedsProcess(crefdtypep->classp());
        वापस false;
    }

    bool coverage_Uses_Local_गणक(Ast_गाँठ_Cover_Decl* स्थिर declp) स्थिर {
        // Only functions with an object receiver can address the module-local
        // coverage array. Static/package/class helper functions still use the
        // symbol-table array, as they may have vlSymsp but no vlSelf/this.
        // Also require the coverage declaration to belong to the current
        // module; cloned task bodies can increment declarations owned by a
        // different module, and those must keep using the global array.
        वापस m_cfuncp && !m_cfuncp->isStatic() && !VN_IS(m_modp, Class)
               && !VN_IS(m_modp, ClassPackage)
               && EmitCParentModule::get(declp->तथ्य_Decl_Thisp()) == m_modp;
    }
    पूर्णांक coverage_Bin_अंक(Ast_गाँठ_Cover_Decl* स्थिर declp, bool forceGlobal = false) स्थिर {
        वापस !forceGlobal && coverage_Uses_Local_गणक(declp)
                   ? declp->तथ्य_Decl_Thisp()->local_Bin_अंक()
                   : declp->तथ्य_Decl_Thisp()->bin_अंक();
    }
    व्योम putCoverageArray(Ast_गाँठ_Cover_Decl* स्थिर declp, bool forceGlobal = false) {
        अगर (!forceGlobal && coverage_Uses_Local_गणक(declp)) {
            // Keep counters on the module object when possible, so every
            // no-inline instance has storage available if coverage output is
            // later written with forcePerInstance.
            माला_दो(m_cfuncp->isLoose() ? "vlSelf->__Vcoverage" : "this->__Vcoverage");
        } अन्यथा {
            // Static/package/class helper functions do not have vlSelf, so
            // keep using the shared symbol-table coverage array there.
            माला_दो("vlSymsp->__Vcoverage");
        }
    }
    व्योम emit_Cover_Other_Decl_जोड़ो(AstCoverOtherDecl* nodep, bool forceGlobal = false) {
        putns(nodep, "vlSelf->__vlCoverInsert(");  // As Declared in emitCoverageDecl
        putCoverageArray(nodep, forceGlobal);
        माला_दो(" + ");
        माला_दो(cvtToStr(coverage_Bin_अंक(nodep, forceGlobal)));
        // first controls whether duplicate module instances are collapsed in
        // the default coverage view. The following flag says whether countp
        // points to an object-local counter; only those counters must be kept
        // real for later instances so forcePerInstance can split hierarchy.
        माला_दो(", first");  // Enable, passed from __Vconfigure parameter
        माला_दो(", ");
        माला_दो(!forceGlobal && coverage_Uses_Local_गणक(nodep) ? "true" : "false");
        माला_दो(", ");
        putsQuoted(protect(nodep->fileline()->filename()));
        माला_दो(", ");
        माला_दो(cvtToStr(nodep->fileline()->lineno()));
        माला_दो(", ");
        माला_दो(cvtToStr(nodep->offset() + nodep->fileline()->firstColumn()));
        माला_दो(", ");
        putsQuoted((!nodep->hier().empty() ? "." : "")
                   + VIdProtect::protect_शब्दगण_If(nodep->hier(), nodep->protect()));
        माला_दो(", ");
        putsQuoted(VIdProtect::protect_शब्दगण_If(nodep->page(), nodep->protect()));
        माला_दो(", ");
        putsQuoted(VIdProtect::protect_शब्दगण_If(nodep->comment(), nodep->protect()));
        माला_दो(", ");
        putsQuoted(nodep->linescov());
        माला_दो(", ");
        putsQuoted(VIdProtect::protect_शब्दगण_If(nodep->fsmVar(), nodep->protect()));
        माला_दो(", ");
        putsQuoted(VIdProtect::protect_शब्दगण_If(nodep->fsmFrom(), nodep->protect()));
        माला_दो(", ");
        putsQuoted(VIdProtect::protect_शब्दगण_If(nodep->fsmTo(), nodep->protect()));
        माला_दो(", ");
        putsQuoted(VIdProtect::protect_शब्दगण_If(nodep->fsmTag(), nodep->protect()));
        माला_दो(");\n");
    }

खुला:
    // METHODS
    bool displayEmitHeader(Ast_गाँठ* nodep);
    व्योम display_गाँठ(Ast_गाँठ* nodep, AstSFormatF* fmtp, स्थिर string& vformat, Ast_गाँठ* exprsp,
                     bool isScan);

    bool emitSimpleOk(Ast_गाँठ_व्यंजक* nodep);
    व्योम emitIQW(स्थिर Ast_गाँठ* nodep) {
        // See "Type letters" in verilated.h
        // Other abbrevs: "C"har, "S"hort, "F"loat, "D"ouble, stri"N"g, "R"=queue, "U"npacked
        माला_दो(nodep->dtypep()->skipRefp()->charIQWN());
    }
    व्योम emitRU(स्थिर Ast_गाँठ* nodep) {
        स्थिर Ast_गाँठ_DType* dtp = nodep->dtypep()->skipRefp();
        // See "Type letters" in verilated.h
        अगर (VN_IS(dtp, UnpackArrayDType))
            माला_दो("U");
        अन्यथा अगर (VN_IS(dtp, QueueDType) || VN_IS(dtp, DynArrayDType))
            माला_दो("R");
    }
    व्योम emitScIQW(AstVar* nodep) {
        UASSERT_OBJ(nodep->isSc(), nodep, "emitting SystemC operator on non-SC variable");
        माला_दो(nodep->isScBigUint()                           ? "SB"
             : (nodep->isScUint() || nodep->isScUintBool()) ? "SU"
             : nodep->isScBv()                              ? "SW"
             : nodep->isDouble()                            ? "SD"
                                                            : (nodep->isScQuad() ? "SQ" : "SI"));
    }
    व्योम emitDatap(स्थिर Ast_गाँठ* nodep) {
        // When passing to a function with va_args the compiler doesn't
        // know need a pointer so when wide, need to look inside VlWide
        अगर (nodep->isWide()) माला_दो(".data()");
    }
    व्योम emitOpName(Ast_गाँठ* nodep, स्थिर string& format, Ast_गाँठ* lhsp, Ast_गाँठ* rhsp,
                    Ast_गाँठ* thsp);
    व्योम emitCCallArgs(स्थिर Ast_गाँठ_CCall* nodep, स्थिर string& selfPointer, bool inProcess);
    व्योम emitDereference(Ast_गाँठ* nodep, स्थिर string& pointer);
    std::string dereferenceString(स्थिर std::string& pointer) स्थिर;
    व्योम emitCvtPackStr(Ast_गाँठ* nodep);
    व्योम emitConstant(AstConst* nodep);
    व्योम emitConstantString(स्थिर AstConst* nodep);
    व्योम emitVarReset(स्थिर string& prefix, AstVar* varp, bool constructing);
    string emitVarResetRecurse(स्थिर AstVar* varp, bool constructing,
                               स्थिर string& varNameProtected, Ast_गाँठ_DType* dtypep, पूर्णांक depth,
                               स्थिर string& suffix, स्थिर Ast_गाँठ* valuep);
    व्योम emitVarResetScopeHash();
    व्योम emitChangeDet();
    व्योम put_Comma_Iterate_अग्र(Ast_गाँठ* nodep, bool comma = false) {
        क्रम (Ast_गाँठ* subnodep = nodep; subnodep; subnodep = subnodep->nextp()) {
            अगर (comma) माला_दो(", ");
            iterateConst(subnodep);
            comma = true;
        }
    }
    ढाँचा <typename T>
    string optionalProcArg(स्थिर T* स्थिर nodep) {
        वापस (nodep && constructorNeedsProcess(nodep)) ? "vlProcess, " : "";
    }
    स्थिर AstCNew* getSuperNewCallRecursep(Ast_गाँठ* स्थिर nodep) {
        // Get the super.new call
        अगर (!nodep) वापस nullptr;
        अगर (स्थिर AstCNew* स्थिर cnewp = VN_CAST(nodep, CNew)) वापस cnewp;
        अगर (स्थिर Ast_Stmt_व्यंजक* स्थिर stmtp = VN_CAST(nodep, Stmt_व्यंजक)) {
            अगर (स्थिर AstCNew* स्थिर cnewp = VN_CAST(stmtp->exprp(), CNew)) वापस cnewp;
        }
        अगर (स्थिर AstJumpBlock* स्थिर blockp = VN_CAST(nodep, JumpBlock)) {
            अगर (स्थिर AstCNew* स्थिर cnewp = getSuperNewCallRecursep(blockp->stmtsp())) {
                वापस cnewp;
            }
        }
        अगर (स्थिर AstCNew* स्थिर cnewp = getSuperNewCallRecursep(nodep->nextp())) वापस cnewp;
        वापस nullptr;
    }

    व्योम emitConstantW(AstConst* nodep, AstVarRef* assigntop) {
        // For tradition and compilation speed, assign each word directly into
        // output variable instead of using '='
        putns(nodep, "");
        अगर (nodep->अंक().isFourState()) {
            nodep->v3warn(E_UNSUPPORTED, "Unsupported: 4-state numbers in this context");
            वापस;
        }

        पूर्णांक upWidth = nodep->अंक().widthToFit();
        पूर्णांक chunks = 0;
        अगर (upWidth > EMITC_अंक_CONSTW * VL_EDATASIZE) {
            // Output e.g. 8 words in groups of e.g. 8
            chunks = (upWidth - 1) / (EMITC_अंक_CONSTW * VL_EDATASIZE);
            upWidth %= (EMITC_अंक_CONSTW * VL_EDATASIZE);
            अगर (upWidth == 0) upWidth = (EMITC_अंक_CONSTW * VL_EDATASIZE);
        }
        {  // Upper e.g. 8 words
            अगर (chunks) {
                putnbs(nodep, "VL_CONSTHI_W_");
                माला_दो(cvtToStr(VL_शब्दगण_I(upWidth)));
                माला_दो("X(");
                माला_दो(cvtToStr(nodep->widthMin()));
                माला_दो(",");
                माला_दो(cvtToStr(chunks * EMITC_अंक_CONSTW * VL_EDATASIZE));
            } अन्यथा {
                putnbs(nodep, "VL_CONST_W_");
                माला_दो(cvtToStr(VL_शब्दगण_I(upWidth)));
                माला_दो("X(");
                माला_दो(cvtToStr(nodep->widthMin()));
            }
            माला_दो(",");
            अगर (!assigntop->selfPointer().isEmpty()) {
                emitDereference(assigntop, assigntop->selfPointerProtect(m_useSelfForThis));
            }
            माला_दो(varName(assigntop->varp()));
            क्रम (पूर्णांक word = VL_शब्दगण_I(upWidth) - 1; word >= 0; word--) {
                // Only 32 bits - llx + long long here just to appease CPP format warning
                ofp()->म_लिखो(",0x%08" PRIx64, static_cast<uint64_t>(nodep->अंक().edataWord(
                                                   word + chunks * EMITC_अंक_CONSTW)));
            }
            माला_दो(")");
        }
        क्रम (chunks--; chunks >= 0; chunks--) {
            माला_दो(";\n");
            putbs("VL_CONSTLO_W_");
            माला_दो(cvtToStr(EMITC_अंक_CONSTW));
            माला_दो("X(");
            माला_दो(cvtToStr(chunks * EMITC_अंक_CONSTW * VL_EDATASIZE));
            माला_दो(",");
            अगर (!assigntop->selfPointer().isEmpty()) {
                emitDereference(assigntop, assigntop->selfPointerProtect(m_useSelfForThis));
            }
            माला_दो(varName(assigntop->varp()));
            क्रम (पूर्णांक word = EMITC_अंक_CONSTW - 1; word >= 0; word--) {
                // Only 32 bits - llx + long long here just to appease CPP format warning
                ofp()->म_लिखो(",0x%08" PRIx64, static_cast<uint64_t>(nodep->अंक().edataWord(
                                                   word + chunks * EMITC_अंक_CONSTW)));
            }
            माला_दो(")");
        }
    }

    व्योम emitNodesWithText(Ast_गाँठ* nodesp, bool useSelfForThis, bool tracking,
                           स्थिर std::string& separator) {
        क्रम (Ast_गाँठ* nodep = nodesp; nodep; nodep = nodep->nextp()) {
            अगर (स्थिर AstText* स्थिर textp = VN_CAST(nodep, Text)) {
                स्थिर std::string text
                    = VSelfPointerText::replaceThis(useSelfForThis, textp->text());
                अगर (tracking) {
                    माला_दो(text);
                } अन्यथा {
                    ofp()->putsNoTracking(text);
                }
            } अन्यथा {
                iterateConst(nodep);
            }
            अगर (nodep->nextp()) माला_दो(separator);
        }
    }

    व्योम putConstructorSubinit(स्थिर AstClass* classp, AstCFunc* cfuncp) {
        // Direct non-virtual bases in declaration order
        क्रम (स्थिर AstClassExtends* extp = classp->extendsp(); extp;
             extp = VN_AS(extp->nextp(), ClassExtends)) {
            अगर (extp->classp()->isInterfaceClass()) जारी;
            अगर (extp->classp()->useVirtualPublic()) जारी;
            माला_दो(EmitCUtil::prefixNameProtect(extp->classp()));
            माला_दो("::init");
            अगर (constructorNeedsProcess(extp->classp())) {
                माला_दो("(vlProcess, vlSymsp");
            } अन्यथा {
                माला_दो("(vlSymsp");
            }
            // Handle super.new() args for the concrete parent
            अगर (स्थिर AstCNew* स्थिर superNewCallp = getSuperNewCallRecursep(cfuncp->stmtsp())) {
                put_Comma_Iterate_अग्र(superNewCallp->argsp(), true);
            }
            माला_दो(");\n");
            // Since SystemVerilog allows only for extending from one class, for sure there won't
            // be any more constructors to be called. Even if there are some classes which are
            // "implemented", those are interface classes which do not have constructors.
            अवरोध;
        }
    }

    // VISITORS
    using EmitCConstInit::visit;
    व्योम visit(AstCFunc* स्थिर nodep) override {
        अगर (nodep->emptyBody() && !nodep->isLoose()) वापस;
        VL_RESTORER(m_useSelfForThis);
        VL_RESTORER(m_cfuncp);
        VL_RESTORER(m_instantiatesOwnProcess);
        VL_RESTORER(m_createdScopeHash);
        m_cfuncp = nodep;
        m_instantiatesOwnProcess = false;
        m_labelNumbers.clear();  // No need to save/restore, all Jumps must be within the function

        split_आकार_Inc(nodep);

        माला_दो("\n");
        m_lazyDecls.emit(nodep);
        अगर (nodep->ifdef() != "") putns(nodep, "#ifdef " + nodep->ifdef() + "\n");
        emitCFuncHeader(nodep, m_modp, /* withScope: */ true);

        माला_दो(" {\n");
        अगर (nodep->isConstructor()) {
            स्थिर AstClass* स्थिर classp = VN_CAST(nodep->scopep()->modp(), Class);
            अगर (classp && classp->extendsp()) putConstructorSubinit(classp, nodep);
        }

        // "+" in the debug indicates a print from the model
        माला_दो("VL_DEBUG_IF(VL_DBG_MSGF(\"+  ");
        क्रम (पूर्णांक i = 0; i < m_modp->level(); ++i) माला_दो("  ");
        माला_दो(EmitCUtil::prefixNameProtect(m_modp));
        माला_दो(nodep->isLoose() ? "__" : "::");
        माला_दो(nodep->nameProtect() + "\\n\"); );\n");

        अगर (nodep->isLoose()) {
            m_lazyDecls.declared(nodep);  // Defined here, so no longer needs declaration
            अगर (!nodep->isStatic()) {  // Standard prologue
                m_useSelfForThis = true;
                अगर (nodep->voidSelf()) {
                    स्थिर string className = EmitCUtil::prefixNameProtect(m_modp);
                    माला_दो(className + "* const vlSelf = static_cast<" + className
                         + "*>(vlVoidSelf);\n");
                }
                अगर (!VN_IS(m_modp, Class)) {
                    माला_दो(EmitCUtil::symClassAssign());  // Uses vlSelf
                } अन्यथा {
                    माला_दो("(void)vlSelf;  // Prevent unused variable warning\n");
                }
            }
        }

        // Instantiate a process class if it's going to be needed somewhere later
        nodep->forall([&](स्थिर Ast_गाँठ_CCall* ccallp) -> bool {
            अगर (ccallp->funcp()->needProcess()
                && (ccallp->funcp()->isCoroutine() == VN_IS(ccallp->backp(), CAwait))) {
                अगर (!nodep->needProcess() && !m_instantiatesOwnProcess) {
                    m_instantiatesOwnProcess = true;
                    वापस false;
                }
            }
            वापस true;
        });
        अगर (m_instantiatesOwnProcess) {
            AstCStmt* स्थिर vlprocp = new AstCStmt{nodep->fileline()};
            vlprocp->add("VlProcessRef vlProcess = std::make_shared<VlProcess>();\n");
            vlprocp->add("VlProcess::currentp(vlProcess.get());");
            nodep->prependStmtsp(vlprocp);
        } अन्यथा अगर (nodep->needProcess() && nodep->stmtsp()) {
            // Set current process so VlRNG() constructors in this function seed from it
            AstCStmt* स्थिर setProcessp = new AstCStmt{nodep->fileline()};
            setProcessp->add("VlProcess::currentp(vlProcess.get());");
            nodep->prependStmtsp(setProcessp);
        }

        क्रम (Ast_गाँठ* subnodep = nodep->argsp(); subnodep; subnodep = subnodep->nextp()) {
            अगर (AstVar* स्थिर varp = VN_CAST(subnodep, Var)) {
                अगर (varp->isFuncReturn()) emitVarDecl(varp);
            }
        }

        अगर (m_useSelfForThis) {
            m_usevlSelfRef = true;
            // Using reference to the vlSelf pointer will help the C++
            // compiler to have dereferenceable hints, which can help to
            // reduce the need for branch instructions in the generated
            // code to allow the compiler to generate load store after the
            // if condition (including short-circuit evaluation)
            // speculatively and also reduce the data cache pollution when
            // executing in the wrong path to make Verilated code faster.
            माला_दो("auto& vlSelfRef = std::ref(*vlSelf).get();\n");
        }

        अगर (nodep->varsp()) {
            putsDecoration(nodep, "// Locals\n");
            iterate_And_अग्र_Const_Null(nodep->varsp());
        }

        अगर (nodep->stmtsp()) {
            putsDecoration(nodep, "// Body\n");
            iterate_And_अग्र_Const_Null(nodep->stmtsp());
        }

        m_usevlSelfRef = false;

        अगर (nodep->isCoroutine()) {
            // Sometimes coroutines don't have co_awaits,
            // so emit a co_return at the end to avoid compile errors.
            माला_दो("co_return;\n");
        }

        माला_दो("}\n");
        अगर (nodep->ifdef() != "") माला_दो("#endif  // " + nodep->ifdef() + "\n");
    }

    व्योम visit(AstVar* nodep) override {
        UASSERT_OBJ(m_cfuncp, nodep, "Cannot emit non-local variable");
        emitVarDecl(nodep);
    }

    व्योम visit(AstCvtArrayToArray* nodep) override {
        अगर (nodep->reverse()) {
            माला_दो("VL_REVCLONE_Q(");
        } अन्यथा {
            माला_दो("VL_CLONE_Q(");
        }
        iterate_And_अग्र_Const_Null(nodep->fromp());
        माला_दो(", ");
        माला_दो(cvtToStr(nodep->block_आकार()));
        माला_दो(", ");
        माला_दो(cvtToStr(nodep->srcElementBits()));
        माला_दो(", ");
        माला_दो(cvtToStr(nodep->dstElementBits()));
        माला_दो(")");
    }

    व्योम visit(AstCvtArrayToPacked* nodep) override {
        Ast_गाँठ_DType* स्थिर fromDtp = nodep->fromp()->dtypep()->skipRefp();
        Ast_गाँठ_DType* स्थिर elemDtp = fromDtp->subDTypep()->skipRefp();
        स्थिर bool descending = [&]() {
            अगर (स्थिर AstUnpackArrayDType* स्थिर unpackDtp = VN_CAST(fromDtp, UnpackArrayDType))
                वापस !unpackDtp->declRange().ascending();
            वापस false;
        }();
        माला_दो("VL_PACK_");
        emitIQW(nodep);
        माला_दो("_");
        emitRU(fromDtp);
        emitIQW(elemDtp);
        अगर (descending) {
            // Wrap source in VL_PACK_REVERSED so VL_PACK sees ascending order
            emitOpName(nodep, "(%nw, %rw, %P, VL_PACK_REVERSED(%li))", nodep->fromp(), elemDtp,
                       nullptr);
        } अन्यथा {
            emitOpName(nodep, "(%nw, %rw, %P, %li)", nodep->fromp(), elemDtp, nullptr);
        }
    }

    व्योम visit(AstCvtUnpackToQueue* nodep) override {
        Ast_गाँठ_DType* स्थिर elemDTypep = nodep->fromp()->dtypep()->subDTypep();
        emitOpName(nodep, nodep->emitC(), nodep->fromp(), elemDTypep, nullptr);
    }
    व्योम visit(AstCvtUnpackToUnpack* nodep) override {
        // Whether each dimension is reversed, the first for the outermost, up to the last that is
        std::vector<bool> reverses;
        स्थिर AstUnpackArrayDType* शीर्ष = VN_AS(nodep->dtypep()->skipRefp(), UnpackArrayDType);
        स्थिर AstUnpackArrayDType* fromp
            = VN_AS(nodep->fromp()->dtypep()->skipRefp(), UnpackArrayDType);
        जबतक (शीर्ष) {
            reverses.धकेलो_back(शीर्ष->declRange().ascending() != fromp->declRange().ascending());
            शीर्ष = VN_CAST(शीर्ष->subDTypep()->skipRefp(), UnpackArrayDType);
            fromp = VN_CAST(fromp->subDTypep()->skipRefp(), UnpackArrayDType);
        }
        जबतक (!reverses.back()) reverses.निकालो_back();
        putns(nodep, "VL_CVT_UNPACK_TO_UNPACK<");
        क्रम (माप_प्रकार i = 0; i < reverses.आकार(); ++i) {
            अगर (i) माला_दो(", ");
            माला_दो(reverses[i] ? "1" : "0");
        }
        माला_दो(">(");
        iterate_And_अग्र_Const_Null(nodep->fromp());
        माला_दो(")");
    }

    व्योम visit(Ast_गाँठ_Assign* nodep) override {
        अगर (AstCReset* स्थिर resetp = VN_CAST(nodep->rhsp(), CReset)) {
            // TODO get rid of emitVarReset and instead let AstNodeAssign understand how to init
            // anything
            Ast_गाँठ* fromp = nodep->lhsp();
            // Fork needs to use a member select.  Nothing else should be possible before VarRef.
            अगर (AstMemberSel* स्थिर sfromp = VN_CAST(fromp, MemberSel)) {
                // Fork-DynScope generated pointer to previously automatic variable
                AstVar* स्थिर memberVarp = sfromp->varp();
                fromp = sfromp->fromp();
                अगर (AstNullCheck* स्थिर sfromp = VN_CAST(fromp, NullCheck)) fromp = sfromp->lhsp();
                Ast_गाँठ_Var_Ref* स्थिर fromVarRefp = VN_AS(fromp, गाँठ_Var_Ref);
                emitVarReset(
                    ("VL_NULL_CHECK("s
                     + (fromVarRefp->selfPointer().isEmpty()
                            ? ""
                            : dereferenceString(fromVarRefp->selfPointerProtect(m_useSelfForThis)))
                     + varName(fromVarRefp->varp()) + ", \""
                     + V3OutFormatter::quoteNameControls(protect(nodep->fileline()->filename()))
                     + "\", " + std::to_string(nodep->fileline()->lineno()) + ")->"),
                    memberVarp, resetp->constructing());
            } अन्यथा {
                Ast_गाँठ_Var_Ref* स्थिर fromVarRefp = VN_AS(fromp, गाँठ_Var_Ref);
                AstVar* स्थिर varp = fromVarRefp->varp();
                स्थिर string prefix
                    = fromVarRefp->selfPointer().isEmpty()
                          ? ""
                          : dereferenceString(
                                VN_AS(fromp, गाँठ_Var_Ref)->selfPointerProtect(m_useSelfForThis));
                emitVarReset(prefix, varp, resetp->constructing());
            }
            वापस;
        }
        bool paren = true;
        bool decind = false;
        bool rhs = true;
        bool reverseUnpack = false;  // Set for descending CvtPackedToArray
        स्थिर AstUnpackArrayDType* स्थिर unpackDtp
            = VN_CAST(nodep->dtypep()->skipRefp(), UnpackArrayDType);
        अगर (AstSel* स्थिर selp = VN_CAST(nodep->lhsp(), Sel)) {
            UASSERT_OBJ(selp->widthMin() == selp->widthConst(), selp, "Width mismatch");
            अगर (selp->widthMin() == 1) {
                putnbs(nodep, "VL_ASSIGNBIT_");
                emitIQW(selp->fromp());
                अगर (nodep->rhsp()->isAllOnesV()) {
                    माला_दो("O(");
                    rhs = false;
                } अन्यथा {
                    माला_दो("I(");
                }
                iterate_And_अग्र_Const_Null(selp->lsbp());
                माला_दो(", ");
                iterate_And_अग्र_Const_Null(selp->fromp());
                अगर (rhs) माला_दो(", ");
            } अन्यथा {
                putnbs(nodep, "VL_ASSIGNSEL_");
                emitIQW(selp->fromp());
                emitIQW(nodep->rhsp());
                माला_दो("(");
                putns(selp->fromp(), cvtToStr(selp->fromp()->widthMin()) + ", ");
                माला_दो(cvtToStr(nodep->widthMin()) + ", ");
                iterate_And_अग्र_Const_Null(selp->lsbp());
                माला_दो(", ");
                iterate_And_अग्र_Const_Null(selp->fromp());
                माला_दो(", ");
            }
        } अन्यथा अगर (स्थिर AstGetcRefN* स्थिर selp = VN_CAST(nodep->lhsp(), GetcRefN)) {
            iterate_And_अग्र_Const_Null(selp->lhsp());
            माला_दो(" = ");
            putnbs(selp, "VL_PUTC_N(");
            iterate_And_अग्र_Const_Null(selp->lhsp());
            माला_दो(", ");
            iterate_And_अग्र_Const_Null(selp->rhsp());
            माला_दो(", ");
        } अन्यथा अगर (AstVar* स्थिर varp = AstVar::scVarRecurse(nodep->lhsp())) {
            putnbs(varp, "VL_ASSIGN_");  // Set a systemC variable
            emitScIQW(varp);
            emitIQW(nodep);
            माला_दो("(");
            माला_दो(cvtToStr(nodep->widthMin()) + ", ");
            iterate_And_अग्र_Const_Null(nodep->lhsp());
            माला_दो(", ");
        } अन्यथा अगर (AstVar* स्थिर varp = AstVar::scVarRecurse(nodep->rhsp())) {
            putnbs(varp, "VL_ASSIGN_");  // Get a systemC variable
            emitIQW(nodep);
            emitScIQW(varp);
            माला_दो("(");
            माला_दो(cvtToStr(nodep->widthMin()) + ", ");
            iterate_And_अग्र_Const_Null(nodep->lhsp());
            माला_दो(", ");
        } अन्यथा अगर (स्थिर AstCvtPackedToArray* स्थिर castp
                   = VN_CAST(nodep->rhsp(), CvtPackedToArray)) {
            putns(castp, "VL_UNPACK_");
            emitRU(nodep);
            emitIQW(nodep->dtypep()->subDTypep());
            माला_दो("_");
            emitRU(castp->fromp());
            emitIQW(castp->fromp());
            माला_दो("(");
            putns(castp->dtypep(), cvtToStr(castp->dtypep()->subDTypep()->widthMin()));
            माला_दो(", ");
            माला_दो(cvtToStr(castp->fromp()->widthMin()));
            माला_दो(", ");
            iterate_And_अग्र_Const_Null(nodep->lhsp());
            माला_दो(", ");
            rhs = false;
            iterate_And_अग्र_Const_Null(castp->fromp());
            // Descending unpacked dest: reverse after unpack
            अगर (unpackDtp && !unpackDtp->declRange().ascending()) reverseUnpack = true;
        } अन्यथा अगर (स्थिर AstCvtArrayToArray* स्थिर castp
                   = VN_CAST(nodep->rhsp(), CvtArrayToArray)) {
            अगर (castp->reverse()) {
                putns(castp, "VL_REVCOPY_Q(");
            } अन्यथा {
                putns(castp, "VL_COPY_Q(");
            }
            iterate_And_अग्र_Const_Null(nodep->lhsp());
            माला_दो(", ");
            rhs = false;
            iterate_And_अग्र_Const_Null(castp->fromp());
            माला_दो(", ");
            माला_दो(cvtToStr(castp->block_आकार()));
            माला_दो(", ");
            माला_दो(cvtToStr(castp->srcElementBits()));
            माला_दो(", ");
            माला_दो(cvtToStr(castp->dstElementBits()));
        } अन्यथा अगर (nodep->isWide() && VN_IS(nodep->lhsp(), VarRef)  //
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
                   && !VN_IS(nodep->rhsp(), व्यंजक_Stmt)) {
            // Wide functions assign into the array directly, don't need separate assign statement
            m_wide_अस्थायी_Refp = VN_AS(nodep->lhsp(), VarRef);
            paren = false;
        } अन्यथा अगर (nodep->isWide() && !unpackDtp && !VN_IS(nodep->rhsp(), Const)) {
            putnbs(nodep, "VL_ASSIGN_W(");
            माला_दो(cvtToStr(nodep->widthMin()) + ", ");
            iterate_And_अग्र_Const_Null(nodep->lhsp());
            माला_दो(", ");
        } अन्यथा अगर (VN_IS(nodep->lhsp()->dtypep()->skipRefp(), QueueDType)
                   && (VN_IS(nodep->rhsp(), StreamL) || VN_IS(nodep->lhsp(), StreamL)
                       || VN_IS(nodep->rhsp(), StreamR) || VN_IS(nodep->lhsp(), StreamR)
                       || VN_IS(nodep->rhsp(), StreamR))) {
            //if either side is streamL or streamR don't emit lhsp everything will be passed by
            //reference

            paren = false;

        } अन्यथा {
            paren = false;
            iterate_And_अग्र_Const_Null(nodep->lhsp());
            माला_दो(" ");
            ofp()->blockInc();
            decind = true;
            अगर (!VN_IS(nodep->rhsp(), Const)) ofp()->putBreak();
            putns(nodep, "= ");
        }
        अगर (rhs) iterate_And_अग्र_Const_Null(nodep->rhsp());
        अगर (paren) माला_दो(")");
        अगर (decind) ofp()->blockDec();
        माला_दो(";\n");
        अगर (reverseUnpack) {
            माला_दो("VL_UNPACK_REVERSED(");
            iterate_And_अग्र_Const_Null(nodep->lhsp());
            माला_दो(");\n");
        }
    }
    व्योम visit(AstAssocSel* nodep) override {
        iterate_And_अग्र_Const_Null(nodep->fromp());
        स्थिर std::string atFunc = nodep->isLValue() ? ".atWrite(" : ".at(";
        putnbs(nodep, atFunc);
        AstAssocArrayDType* स्थिर adtypep
            = VN_AS(nodep->fromp()->dtypep()->skipRefp(), AssocArrayDType);
        UASSERT_OBJ(adtypep, nodep, "Associative select on non-associative type");
        iterate_And_अग्र_Const_Null(nodep->bitp());
        माला_दो(")");
    }
    व्योम visit(AstWildcardSel* nodep) override {
        iterate_And_अग्र_Const_Null(nodep->fromp());
        स्थिर std::string atFunc = nodep->isLValue() ? ".atWrite(" : ".at(";
        putnbs(nodep, atFunc);
        AstWildcardArrayDType* स्थिर adtypep
            = VN_AS(nodep->fromp()->dtypep()->skipRefp(), WildcardArrayDType);
        UASSERT_OBJ(adtypep, nodep, "Wildcard select on non-wildcard-associative type");
        iterate_And_अग्र_Const_Null(nodep->bitp());
        माला_दो(")");
    }
    व्योम visit(AstCCall* nodep) override {
        स्थिर AstCFunc* स्थिर funcp = nodep->funcp();
        स्थिर Ast_गाँठ_Module* स्थिर funcModp = EmitCParentModule::get(funcp);
        putnbs(nodep, "");
        अगर (funcp->dpiImportPrototype()) {
            // Calling DPI import
            putns(nodep, funcp->name());
        } अन्यथा अगर (funcp->isProperMethod() && funcp->isStatic()) {
            // Call static method via the containing class
            putns(funcModp, EmitCUtil::prefixNameProtect(funcModp) + "::");
            putns(nodep, funcp->nameProtect());
        } अन्यथा अगर (nodep->superReference()) {
            // Calling superclass method
            putns(funcModp, EmitCUtil::prefixNameProtect(funcModp) + "::");
            putns(nodep, funcp->nameProtect());
        } अन्यथा अगर (funcp->isLoose()) {
            // Calling loose method
            putns(nodep, funcNameProtect(funcp));
        } अन्यथा {
            // Calling regular method/function
            अगर (!nodep->selfPointer().isEmpty()) {
                emitDereference(nodep, nodep->selfPointerProtect(m_useSelfForThis));
            }
            putns(nodep, funcp->nameProtect());
        }
        emitCCallArgs(nodep, nodep->selfPointerProtect(m_useSelfForThis), m_cfuncp->needProcess());
    }
    व्योम visit(AstCMethodCall* nodep) override {
        स्थिर AstCFunc* स्थिर funcp = nodep->funcp();
        UASSERT_OBJ(!funcp->isLoose(), nodep, "Loose method called via AstCMethodCall");
        iterateConst(nodep->fromp());
        putnbs(nodep, "->");
        अगर (nodep->superReference()) {
            // Super call, so the base implementation is used rather than the override
            स्थिर Ast_गाँठ_Module* स्थिर funcModp = EmitCParentModule::get(funcp);
            putns(funcModp, EmitCUtil::prefixNameProtect(funcModp) + "::");
        }
        putns(funcp, funcp->nameProtect());
        emitCCallArgs(nodep, "", m_cfuncp->needProcess());
    }
    व्योम visit(AstCAwait* nodep) override {
        putns(nodep, "co_await ");
        iterateConst(nodep->exprp());
        माला_दो(";\n");
    }
    व्योम visit(AstCNew* nodep) override {
        अगर (VN_IS(nodep->dtypep(), VoidDType)) {  // super.new case
            putsDecoration(nodep, "/*super.new*/");
            वापस;
        }
        // assignment case;
        putns(nodep, "VL_NEW(" + EmitCUtil::prefixNameProtect(nodep->dtypep()) + ", "
                         + optionalProcArg(nodep->dtypep()) + "vlSymsp");
        put_Comma_Iterate_अग्र(nodep->argsp(), true);
        माला_दो(")");
    }
    व्योम visit(AstCMethodHard* nodep) override {
        iterateConst(nodep->fromp());
        putns(nodep, nodep->usePtr() ? "->" : ".");
        putns(nodep, nodep->name());
        अगर (nodep->method() == VCMethod::FORCE_READ_SEL) {
            emitIQW(nodep);
            अगर (nodep->isWide()) माला_दो("<" + cvtToStr(nodep->dtypep()->width_शब्दगण()) + ">");
        } अन्यथा अगर (nodep->method() == VCMethod::ARRAY_SLICE) {
            // VlUnpacked::slice<N_Out>(loIdx) - N_Out is the (fixed) result array size
            स्थिर AstUnpackArrayDType* स्थिर adtypep
                = VN_AS(nodep->dtypep()->skipRefp(), UnpackArrayDType);
            माला_दो("<" + cvtToStr(adtypep->elementsConst()) + ">");
        } अन्यथा अगर (nodep->method() == VCMethod::COVERGROUP_ADD_COVERPOINT) {
            // The hit-list bound is a template argument of the returned VlCoverpointT<>, and the
            // node's own dtype is that type, so it is the one source of truth for both.
            स्थिर AstCoverpointDType* स्थिर cpdtypep
                = VN_AS(nodep->dtypep()->skipRefp(), CoverpointDType);
            माला_दो("<" + cvtToStr(cpdtypep->hitBound()) + ">");
        } अन्यथा अगर (nodep->method() == VCMethod::COVERGROUP_ADD_CROSS) {
            स्थिर AstCoverCrossDType* स्थिर cxdtypep
                = VN_AS(nodep->dtypep()->skipRefp(), CoverCrossDType);
            माला_दो("<" + cxdtypep->cppTemplateArgs() + ">");
        }
        माला_दो("(");
        bool comma = false;
        पूर्णांक arg_अंक = 0;
        क्रम (Ast_गाँठ* subnodep = nodep->pinsp(); subnodep; subnodep = subnodep->nextp()) {
            अगर (comma) माला_दो(", ");
            अगर (nodep->method() == VCMethod::RANDOMIZER_HARD && arg_अंक == 1) {
                // For RANDOMIZER_HARD's filename argument (2nd arg after constraint),
                // apply protect() similar to VL_STOP to handle --protected flag
                अगर (स्थिर AstCExpr* स्थिर cexprp = VN_CAST(subnodep, CExpr)) {
                    // Extract filename from the CExpr (which contains "filename")
                    std::string filename;
                    क्रम (स्थिर Ast_गाँठ* textnodep = cexprp->nodesp(); textnodep;
                         textnodep = textnodep->nextp()) {
                        अगर (स्थिर AstText* स्थिर textp = VN_CAST(textnodep, Text)) {
                            filename = textp->text();
                            अवरोध;
                        }
                    }
                    // Remove surrounding quotes if present
                    अगर (filename.आकार() >= 2 && filename.front() == '"'
                        && filename.back() == '"') {
                        filename = filename.substr(1, filename.आकार() - 2);
                    }
                    // Emit with protect()
                    putsQuoted(protect(filename));
                } अन्यथा {
                    iterateConst(subnodep);
                }
            } अन्यथा {
                iterateConst(subnodep);
            }
            comma = true;
            arg_अंक++;
        }
        अगर (nodep->withp()) {
            अगर (comma) माला_दो(", ");
            iterateConst(nodep->withp());
        }
        माला_दो(")");
    }
    व्योम visit(AstCFuncHard* nodep) override {
        putns(nodep, nodep->name());
        माला_दो("(");
        put_Comma_Iterate_अग्र(nodep->pinsp());
        माला_दो(")");
    }
    व्योम visit(AstLambdaArgRef* nodep) override { putbs(nodep->nameProtect()); }
    व्योम visit(AstWith* nodep) override {
        // With uses a C++11 lambda
        putnbs(nodep, "[&](");
        अगर (स्वतः* स्थिर argrefp = nodep->सूचकांक_Arg_Refp()) {
            putnbs(argrefp, argrefp->dtypep()->cType(argrefp->nameProtect(), false, false));
            माला_दो(", ");
        }
        अगर (स्वतः* स्थिर argrefp = nodep->मूल्य_Arg_Refp()) {
            putnbs(argrefp, argrefp->dtypep()->cType(argrefp->nameProtect(), false, false));
        }
        माला_दो(") -> ");
        putnbs(nodep, nodep->dtypep()->cType("", false, false));
        माला_दो(" {\n");
        VL_RESTORER(m_createdScopeHash);
        iterate_And_अग्र_Const_Null(nodep->exprp());
        माला_दो("}\n");
    }
    व्योम visit(AstCase* nodep) override {  // LCOV_EXCL_LINE
        // In V3Case...
        nodep->v3fatalSrc("Case statements should have been reduced out");
    }
    व्योम visit(AstComment* nodep) override {
        string at;
        अगर (nodep->showAt()) {
            at = " at " + nodep->fileline()->ascii();
            // If protecting, passthru less information about the design
            अगर (!v3Global.opt.protectIds()) वापस;
        }
        अगर (!(nodep->protect() && v3Global.opt.protectIds())) {
            putsDecoration(nodep, "// "s + nodep->name() + at + "\n");
        }
        iterateChildrenConst(nodep);
    }
    व्योम visit(AstCoverOtherDecl* nodep) override {
        emit_Cover_Other_Decl_जोड़ो(nodep);
        अगर (coverage_Uses_Local_गणक(nodep)) {
            // Some task bodies are cloned into a caller module where the
            // original instance-local counter is not addressable. Those clones
            // fall back to the global bin number, so register that slot too.
            emit_Cover_Other_Decl_जोड़ो(nodep, true);
        }
    }
    व्योम visit(AstCoverToggleDecl* nodep) override {
        putns(nodep, "vlSelf->__vlCoverToggleInsert(");  // As Declared in emitCoverageDecl
        माला_दो(cvtToStr(nodep->range().दायाँ()));
        माला_दो(", ");
        माला_दो(cvtToStr(nodep->range().बायाँ()));
        माला_दो(", ");
        माला_दो(cvtToStr(nodep->range().ranged()));
        माला_दो(", ");
        putCoverageArray(nodep);
        माला_दो(" + ");
        माला_दो(cvtToStr(coverage_Bin_अंक(nodep)));
        // first controls whether duplicate module instances are collapsed in
        // the default coverage view. The following flag says whether countp
        // points to an object-local counter; only those counters must be kept
        // real for later instances so forcePerInstance can split hierarchy.
        माला_दो(", first");  // Enable, passed from __Vconfigure parameter
        माला_दो(", ");
        माला_दो(coverage_Uses_Local_गणक(nodep) ? "true" : "false");
        माला_दो(", ");
        putsQuoted(protect(nodep->fileline()->filename()));
        माला_दो(", ");
        माला_दो(cvtToStr(nodep->fileline()->lineno()));
        माला_दो(", ");
        माला_दो(cvtToStr(nodep->fileline()->firstColumn()));
        माला_दो(", ");
        putsQuoted((!nodep->hier().empty() ? "." : "")
                   + VIdProtect::protect_शब्दगण_If(nodep->hier(), nodep->protect()));
        माला_दो(", ");
        putsQuoted(VIdProtect::protect_शब्दगण_If(nodep->page(), nodep->protect()));
        माला_दो(", ");
        putsQuoted(VIdProtect::protect_शब्दगण_If(nodep->comment(), nodep->protect()));
        माला_दो(");\n");
    }
    व्योम visit(AstCoverInc* nodep) override {
        अगर (VN_IS(nodep->declp(), CoverOtherDecl)) {
            अगर (v3Global.opt.threads() > 1) {
                putns(nodep, "");
                putCoverageArray(nodep->declp());
                माला_दो("[");
                माला_दो(cvtToStr(coverage_Bin_अंक(nodep->declp())));
                माला_दो("].fetch_add(1, std::memory_order_relaxed);\n");
            } अन्यथा {
                putns(nodep, "++(");
                putCoverageArray(nodep->declp());
                माला_दो("[");
                माला_दो(cvtToStr(coverage_Bin_अंक(nodep->declp())));
                माला_दो("]);\n");
            }
        } अन्यथा {
            माला_दो("VL_COV_TOGGLE_CHG_");
            अगर (v3Global.opt.threads() > 1) {
                माला_दो("MT_");
            } अन्यथा {
                माला_दो("ST_");
            }
            emitIQW(nodep->toggleExprp());
            माला_दो("(");
            // Each bit of variable has 2 counters, so the size of variable is half of the size of
            // coverpoint
            माला_दो(cvtToStr(nodep->declp()->आकार() / 2));
            माला_दो(", ");
            // Toggle update uses the same object-local counter array that
            // __vlCoverToggleInsert registered.
            putCoverageArray(nodep->declp());
            माला_दो(" + ");
            माला_दो(cvtToStr(coverage_Bin_अंक(nodep->declp())));
            माला_दो(", ");
            iterateConst(nodep->toggleExprp());
            माला_दो(", ");
            iterateConst(nodep->toggleCovExprp());
            माला_दो(");\n");
        }
    }
    व्योम visit(AstDisableFork* nodep) override { putns(nodep, "vlProcess->disableFork();\n"); }
    व्योम visit(AstCReturn* nodep) override {
        putns(nodep, "return (");
        iterate_And_अग्र_Const_Null(nodep->lhsp());
        माला_दो(");\n");
    }
    व्योम visit(AstDisplay* nodep) override {
        string text = nodep->fmtp()->text();
        अगर (nodep->addNewline()) text += "\n";
        display_गाँठ(nodep, nodep->fmtp(), text, nodep->fmtp()->exprsp(), false);
    }
    व्योम visit(AstDumpCtl* nodep) override {
        चयन (nodep->ctlType()) {
        हाल VDumpCtlType::खाता:
            putns(nodep, "vlSymsp->_vm_contextp__->dumpfile(");
            iterateConst(nodep->exprp());
            माला_दो(");\n");
            अवरोध;
        हाल VDumpCtlType::VARS:
            // We ignore number of levels to dump in exprp()
            अगर (v3Global.opt.trace()) {
                putns(nodep, "vlSymsp->_traceDumpOpen();\n");
            } अन्यथा {
                putns(nodep, "VL_PRINTF_MT(\"-Info: ");
                माला_दो(V3OutFormatter::quoteNameControls(protect(nodep->fileline()->filename())));
                माला_दो(":");
                माला_दो(cvtToStr(nodep->fileline()->lineno()));
                माला_दो(": $dumpvar ignored, as Verilated without --trace");
                माला_दो("\\n\");\n");
            }
            अवरोध;
        हाल VDumpCtlType::ALL:
            // $dumpall currently ignored
            अवरोध;
        हाल VDumpCtlType::FLUSH:
            // $dumpall currently ignored; would need rework of VCD single thread,
            // or flag we pass-through to next eval() iteration
            अवरोध;
        हाल VDumpCtlType::LIMIT:
            // $dumplimit currently ignored
            अवरोध;
        हाल VDumpCtlType::OFF:
            // Currently ignored as both Vcd and Fst do not support them, as would need "X" dump
            अवरोध;
        हाल VDumpCtlType::ON:
            // Currently ignored as $dumpoff is also ignored
            अवरोध;
        शेष: nodep->v3fatalSrc("Bad case, unexpected " << nodep->ctlType().ascii());
        }
    }
    व्योम visit(AstScopeName* nodep) override {
        // For use under AstCCalls for dpiImports.  ScopeNames under
        // displays are handled in AstDisplay
        अगर (!nodep->dpiExport()) {
            // this is where the DPI import context scope is set
            स्थिर string scope = nodep->scopeDpiName();
            putnbs(nodep, "(vlSymsp->" + protect("__Vscopep_" + scope) + ")");
        }
    }
    व्योम visit(AstSFormat* nodep) override {
        display_गाँठ(nodep, nodep->fmtp(), nodep->fmtp()->text(), nodep->fmtp()->exprsp(), false);
    }
    व्योम visit(AstSFormatF* nodep) override {
        display_गाँठ(nodep, nodep, nodep->text(), nodep->exprsp(), false);
    }
    व्योम visit(AstFScanF* nodep) override {
        display_गाँठ(nodep, nullptr, nodep->text(), nodep->exprsp(), true);
    }
    व्योम visit(AstSScanF* nodep) override {
        display_गाँठ(nodep, nullptr, nodep->text(), nodep->exprsp(), true);
    }
    व्योम visit(Ast_मूल्य_Plus_Args* nodep) override {
        putns(nodep, "VL_VALUEPLUSARGS_IN");
        emitIQW(nodep->outp());
        माला_दो("(");
        माला_दो(cvtToStr(nodep->outp()->widthMin()));
        माला_दो(", ");
        iterateConst(nodep->searchp());
        माला_दो(", ");
        putbs("");
        iterate_And_अग्र_Const_Null(nodep->outp());
        माला_दो(")");
    }
    व्योम visit(AstTestPlusArgs* nodep) override {
        putns(nodep, "VL_TESTPLUSARGS_I(");
        iterateConst(nodep->searchp());
        माला_दो(")");
    }
    व्योम visit(AstFError* nodep) override {
        putns(nodep, "VL_FERROR_I");
        माला_दो(nodep->strp()->isString() ? "N(" : "W(");
        iterate_And_अग्र_Const_Null(nodep->filep());
        putbs(", ");
        अगर (nodep->strp()->isWide()) {
            माला_दो(cvtToStr(nodep->strp()->width_शब्दगण()));
            putbs(", ");
        }
        iterate_And_अग्र_Const_Null(nodep->strp());
        माला_दो(")");
    }
    व्योम visit(AstFGetS* nodep) override {
        emitOpName(nodep, nodep->emitC(), nodep->strgp(), nodep->filep(), nullptr);
    }
    व्योम visit(AstFOpen* nodep) override {
        putns(nodep, "VL_FOPEN_NN(");
        iterateConst(nodep->filenamep());
        putbs(", ");
        iterateConst(nodep->modep());
        माला_दो(");\n");
    }
    व्योम visit(AstFOpenMcd* nodep) override {
        putns(nodep, "VL_FOPEN_MCD_N(");
        iterateConst(nodep->filenamep());
        माला_दो(");\n");
    }
    व्योम visit(Ast_गाँठ_Read_Write_Mem* nodep) override {
        putns(nodep, nodep->cFuncPrefixp());
        माला_दो("N(");
        माला_दो(nodep->isHex() ? "true" : "false");
        putbs(", ");
        // Need real storage width
        माला_दो(cvtToStr(nodep->memp()->dtypep()->skipRefp()->subDTypep()->widthMin()));
        uint32_t array_lo = 0;
        bool memUsePtr = true;
        {
            स्थिर AstVarRef* स्थिर varrefp = VN_CAST(nodep->memp(), VarRef);
            अगर (!varrefp) {
                nodep->v3error(nodep->verilogKwd() << " loading non-variable");
            } अन्यथा अगर (VN_IS(varrefp->varp()->dtypeSkipRefp(), AssocArrayDType)
                       || VN_IS(varrefp->varp()->dtypeSkipRefp(), DynArrayDType)) {
                // nodep->memp() below will when verilated code is compiled create a C++ template
                memUsePtr = false;
            } अन्यथा अगर (स्थिर AstUnpackArrayDType* स्थिर adtypep
                       = VN_CAST(varrefp->varp()->dtypeSkipRefp(), UnpackArrayDType)) {
                putbs(", ");
                माला_दो(cvtToStr(varrefp->varp()->dtypep()->arrayUnpackedElements()));
                array_lo = adtypep->lo();
                putbs(", ");
                माला_दो(cvtToStr(array_lo));
            } अन्यथा {
                nodep->v3error(
                    nodep->verilogKwd()
                    << " loading other than unpacked/dynamic/associative-array variable");
            }
        }
        putbs(", ");
        iterateConst(nodep->filenamep());
        putbs(", ");
        {
            अगर (memUsePtr) माला_दो(" &(");
            iterate_And_अग्र_Const_Null(nodep->memp());
            अगर (memUsePtr) माला_दो(")");
        }
        putbs(", ");
        अगर (nodep->lsbp()) {
            iterate_And_अग्र_Const_Null(nodep->lsbp());
        } अन्यथा {
            माला_दो(cvtToStr(array_lo));
        }
        putbs(", ");
        अगर (nodep->msbp()) {
            iterate_And_अग्र_Const_Null(nodep->msbp());
        } अन्यथा {
            माला_दो("~0ULL");
        }
        माला_दो(");\n");
    }
    व्योम visit(AstFClose* nodep) override {
        putns(nodep, "VL_FCLOSE_I(");
        iterate_And_अग्र_Const_Null(nodep->filep());
        माला_दो("); ");
    }
    व्योम visit(AstFFlush* nodep) override {
        अगर (!nodep->filep()) {
            putns(nodep, "VL_FFLUSH_MT();");
        } अन्यथा {
            putns(nodep, "VL_FFLUSH_I(");
            iterate_And_अग्र_Const_Null(nodep->filep());
            माला_दो(");\n");
        }
    }
    व्योम visit(AstFSeek* nodep) override {
        putns(nodep, "(VL_FSEEK_I(");
        iterate_And_अग्र_Const_Null(nodep->filep());
        माला_दो(", ");
        iterate_And_अग्र_Const_Null(nodep->offset());
        माला_दो(", ");
        iterate_And_अग्र_Const_Null(nodep->operation());
        माला_दो(") == -1 ? -1 : 0)");
    }
    व्योम visit(AstFTell* nodep) override {
        putns(nodep, "VL_FTELL_I(");
        iterate_And_अग्र_Const_Null(nodep->filep());
        माला_दो(")");
    }
    व्योम visit(AstFRewind* nodep) override {
        putns(nodep, "(VL_FSEEK_I(");
        iterate_And_अग्र_Const_Null(nodep->filep());
        माला_दो(", 0, 0) == -1 ? -1 : 0)");
    }
    व्योम visit(AstFRead* nodep) override {
        putns(nodep, "VL_FREAD_I(");
        माला_दो(cvtToStr(nodep->memp()->widthMin()));  // Need real storage width
        putbs(", ");
        uint32_t array_lo = 0;
        uint32_t array_आकार = 0;
        {
            स्थिर AstVarRef* स्थिर varrefp = VN_CAST(nodep->memp(), VarRef);
            अगर (!varrefp) {
                nodep->v3error(nodep->verilogKwd() << " loading non-variable");
            } अन्यथा अगर (VN_CAST(varrefp->varp()->dtypeSkipRefp(), BasicDType)) {
            } अन्यथा अगर (स्थिर AstUnpackArrayDType* स्थिर adtypep
                       = VN_CAST(varrefp->varp()->dtypeSkipRefp(), UnpackArrayDType)) {
                array_lo = adtypep->lo();
                array_आकार = adtypep->elementsConst();
            } अन्यथा {
                nodep->v3error(nodep->verilogKwd()
                               << " loading other than unpacked-array variable");
            }
        }
        माला_दो(cvtToStr(array_lo));
        putbs(", ");
        माला_दो(cvtToStr(array_आकार));
        putbs(", ");
        माला_दो("&(");
        iterate_And_अग्र_Const_Null(nodep->memp());
        माला_दो(")");
        putbs(", ");
        iterate_And_अग्र_Const_Null(nodep->filep());
        putbs(", ");
        अगर (nodep->startp()) {
            iterate_And_अग्र_Const_Null(nodep->startp());
        } अन्यथा {
            माला_दो(cvtToStr(array_lo));
        }
        putbs(", ");
        अगर (nodep->countp()) {
            iterate_And_अग्र_Const_Null(nodep->countp());
        } अन्यथा {
            माला_दो(cvtToStr(array_आकार));
        }
        माला_दो(")");
    }
    व्योम visit(Ast_ढेर_Trace_F* nodep) override { putns(nodep, "VL_STACKTRACE_N()"); }
    व्योम visit(Ast_ढेर_Trace_T* nodep) override { putns(nodep, "VL_STACKTRACE();\n"); }
    व्योम visit(AstSystemT* nodep) override {
        putns(nodep, "(void)VL_SYSTEM_I");
        emitIQW(nodep->lhsp());
        माला_दो("(");
        अगर (nodep->lhsp()->isWide()) {
            माला_दो(cvtToStr(nodep->lhsp()->width_शब्दगण()));
            putbs(", ");
        }
        iterate_And_अग्र_Const_Null(nodep->lhsp());
        माला_दो(");\n");
    }
    व्योम visit(AstSystemF* nodep) override {
        putns(nodep, "VL_SYSTEM_I");
        emitIQW(nodep->lhsp());
        माला_दो("(");
        अगर (nodep->lhsp()->isWide()) {
            माला_दो(cvtToStr(nodep->lhsp()->width_शब्दगण()));
            putbs(", ");
        }
        iterate_And_अग्र_Const_Null(nodep->lhsp());
        माला_दो(")");
    }
    व्योम visit(Ast_Stmt_व्यंजक* nodep) override {
        अगर (VN_IS(nodep->exprp()->dtypep()->skipRefp(), VoidDType)) {
            putns(nodep, "");
        } अन्यथा {
            putns(nodep, "std::ignore = ");
        }
        iterateConst(nodep->exprp());
        माला_दो(";\n");
    }
    व्योम visit(AstJumpBlock* nodep) override {
        // Allocate label number
        स्थिर माप_प्रकार n = m_labelNumbers.आकार();
        स्थिर bool newEntry = m_labelNumbers.emplace(nodep, n).second;
        UASSERT_OBJ(newEntry, nodep, "AstJumpBlock visited twide");
        // Emit
        putns(nodep, "{\n");  // Make it visually obvious label jumps outside these
        VL_RESTORER(m_createdScopeHash);
        iterate_And_अग्र_Const_Null(nodep->stmtsp());
        माला_दो("__Vlabel" + std::to_string(n) + ": ;\n");
        माला_दो("}\n");
    }
    व्योम visit(AstJumpGo* nodep) override {
        // Retrieve target label number - must already exist (from enclosing AstJumpBlock)
        स्थिर माप_प्रकार n = m_labelNumbers.at(nodep->blockp());
        // Emit
        putns(nodep, "goto __Vlabel" + std::to_string(n) + ";\n");
    }
    व्योम visit(AstLoop* nodep) override {
        UASSERT_OBJ(!nodep->contsp(), nodep, "'contsp' only used before LinkJump");
        VL_RESTORER(m_createdScopeHash);
        // Special case when the AstLoopTest is first for output readability
        अगर (AstLoopTest* स्थिर testp = VN_CAST(nodep->stmtsp(), LoopTest)) {
            putns(nodep, "while (");
            iterateConst(testp->condp());
            माला_दो(") {\n");
            iterate_And_अग्र_Const_Null(testp->nextp());
            माला_दो("}\n");
            वापस;
        }
        // Special case when the AstLoopTest is last for output readability
        अगर (Ast_गाँठ* lastp = nodep->stmtsp()) {
            जबतक (Ast_गाँठ* स्थिर nextp = lastp->nextp()) lastp = nextp;
            अगर (AstLoopTest* स्थिर testp = VN_CAST(lastp, LoopTest)) {
                putns(nodep, "do {\n");
                क्रम (Ast_गाँठ* p = nodep->stmtsp(); p != lastp; p = p->nextp()) iterateConst(p);
                माला_दो("} while (");
                iterateConst(testp->condp());
                माला_दो(");\n");
                वापस;
            }
        }
        // Emit generic case directly
        putns(nodep, "while (true) {\n");
        iterate_And_अग्र_Const_Null(nodep->stmtsp());
        माला_दो("}\n");
    }
    व्योम visit(AstLoopTest* nodep) override {
        VL_RESTORER(m_createdScopeHash);
        putns(nodep, "if (!(");
        iterate_And_अग्र_Const_Null(nodep->condp());
        माला_दो(")) break;\n");
    }
    व्योम visit(AstCLocalScope* nodep) override {
        putns(nodep, "{\n");
        VL_RESTORER(m_createdScopeHash);
        iterate_And_अग्र_Const_Null(nodep->stmtsp());
        माला_दो("}\n");
    }
    व्योम visit(Ast_गाँठ_If* nodep) override {
        putns(nodep, "if (");
        अगर (!nodep->branchPred().unknown()) {
            माला_दो(nodep->branchPred().ascii());
            माला_दो("((");  // Two parens, so that VL_UNLIKELY((class<foo,bar>)) works
        }
        iterate_And_अग्र_Const_Null(nodep->condp());
        अगर (!nodep->branchPred().unknown()) माला_दो("))");
        माला_दो(") {\n");
        {
            VL_RESTORER(m_createdScopeHash);
            iterate_And_अग्र_Const_Null(nodep->thensp());
        }
        माला_दो("}");
        अगर (!nodep->elsesp()) {
            माला_दो("\n");
        } अन्यथा {
            अगर (VN_IS(nodep->elsesp(), गाँठ_If) && !nodep->elsesp()->nextp()) {
                माला_दो(" else ");
                iterate_And_अग्र_Const_Null(nodep->elsesp());
            } अन्यथा {
                VL_RESTORER(m_createdScopeHash);
                माला_दो(" else {\n");
                iterate_And_अग्र_Const_Null(nodep->elsesp());
                माला_दो("}\n");
            }
        }
    }
    व्योम visit(Ast_व्यंजक_Stmt* nodep) override {
        VL_RESTORER(m_createdScopeHash);
        स्थिर bool containsAwait = nodep->exists([](AstCAwait*) -> bool { वापस true; });
        अगर (containsAwait) {
            UASSERT_OBJ(m_cfuncp && m_cfuncp->isCoroutine(), nodep,
                        "AstExprStmt with CAwait must be in coroutine");
            putnbs(nodep, "(co_await ([&]() -> VlCoroutine {\n");
            iterate_And_अग्र_Const_Null(nodep->stmtsp());
            माला_दो("co_return;\n");
            अगर (!nodep->has_परिणाम()) {
                माला_दो("}()))");
                वापस;
            }
            माला_दो("}()), ");
            iterate_And_अग्र_Const_Null(nodep->resultp());
            माला_दो(")");
            वापस;
        }
        // GCC allows compound statements in expressions, but this is not standard.
        // So we use an immediate-evaluation lambda and comma operator
        putnbs(nodep, "([&]() {\n");
        अगर (!nodep->has_परिणाम()) {
            iterate_And_अग्र_Const_Null(nodep->stmtsp());
            माला_दो("}())");
            वापस;
        }
        iterate_And_अग्र_Const_Null(nodep->stmtsp());
        माला_दो("}(), ");
        iterate_And_अग्र_Const_Null(nodep->resultp());
        माला_दो(")");
    }
    व्योम visit(AstStop* nodep) override {
        putns(nodep, "VL_STOP_MT(");
        putsQuoted(protect(nodep->fileline()->filename()));
        माला_दो(", ");
        माला_दो(cvtToStr(nodep->fileline()->lineno()));
        माला_दो(", \"\"");
        अगर (nodep->isFatal()) माला_दो(", false");
        माला_दो(");\n");
    }
    व्योम visit(AstFinish* nodep) override {
        // Disable all the forks so they don't operate after simulation is finished.
        अगर (m_cfuncp && m_cfuncp->needProcess()) putns(nodep, "vlProcess->disableFork();\n");
        putns(nodep, "VL_FINISH_MT(");
        putsQuoted(protect(nodep->fileline()->filename()));
        माला_दो(", ");
        माला_दो(cvtToStr(nodep->fileline()->lineno()));
        माला_दो(", \"\");\n");
    }
    व्योम visit(AstFinishFork* nodep) override {
        // Disable all the forks so they don't operate after simulation is finished.
        अगर (m_cfuncp && m_cfuncp->needProcess()) putns(nodep, "vlProcess->disableFork();\n");
        putns(nodep, "VL_FINISH_MT(");
        putsQuoted(protect(nodep->fileline()->filename()));
        माला_दो(", ");
        माला_दो(cvtToStr(nodep->fileline()->lineno()));
        माला_दो(", \"\");\n");
        अगर (m_cfuncp->isCoroutine()) {
            putns(nodep, "co_return;\n");
        } अन्यथा {
            putns(nodep, "return;\n");
        }
    }
    व्योम visit(AstPrintTimeScale* nodep) override {
        putns(nodep, "VL_PRINTTIMESCALE(");
        putsQuoted(protect(nodep->prettyName()));
        माला_दो(", ");
        putsQuoted(nodep->timeunit().ascii());
        माला_दो(", vlSymsp->_vm_contextp__);\n");
    }
    व्योम visit(AstRand* nodep) override {
        emitOpName(nodep, nodep->emitC(), nodep->seedp(), nullptr, nullptr);
    }
    व्योम visit(AstRandRNG* nodep) override {
        emitOpName(nodep, nodep->emitC(), nullptr, nullptr, nullptr);
    }
    व्योम visit(AstTime* nodep) override {
        putns(nodep, "VL_TIME_UNITED_Q(");
        UASSERT_OBJ(!nodep->timeunit().isNone(), nodep, "$time has no units");
        स्थिर द्विगुन समय
            = nodep->timeunit().multiplier() / v3Global.rootp()->timeprecision().multiplier();
        UASSERT_OBJ(समय >= 1, nodep, "TimeQ is less than 1, will result in division by zero");
        माला_दो(cvtToStr(समय));
        माला_दो(")");
    }
    व्योम visit(AstTimeD* nodep) override {
        putns(nodep, "VL_TIME_UNITED_D(");
        UASSERT_OBJ(!nodep->timeunit().isNone(), nodep, "$realtime has no units");
        माला_दो(cvtToStr(nodep->timeunit().multiplier()
                      / v3Global.rootp()->timeprecision().multiplier()));
        माला_दो(")");
    }
    व्योम visit(AstGetInitialRandomSeed* nodep) override {
        putns(nodep, "vlSymsp->_vm_contextp__->randSeed()");
    }
    व्योम visit(AstTimeFormat* nodep) override {
        putns(nodep, "VL_TIMEFORMAT_IINI(");
        अगर (nodep->unitsp()) {
            माला_दो("true, ");
            iterate_And_अग्र_Const_Null(nodep->unitsp());
        } अन्यथा {
            माला_दो("false, 0");
        }
        माला_दो(", ");
        अगर (nodep->precisionp()) {
            माला_दो("true, ");
            iterate_And_अग्र_Const_Null(nodep->precisionp());
        } अन्यथा {
            माला_दो("false, 0");
        }
        माला_दो(", ");
        अगर (nodep->suffixp()) {
            माला_दो("true, ");
            iterate_And_अग्र_Const_Null(nodep->suffixp());
        } अन्यथा {
            माला_दो("false, \"\"");
        }
        माला_दो(", ");
        अगर (nodep->widthp()) {
            माला_दो("true, ");
            iterate_And_अग्र_Const_Null(nodep->widthp());
        } अन्यथा {
            माला_दो("false, 0");
        }
        माला_दो(", vlSymsp->_vm_contextp__);\n");
    }
    व्योम visit(AstTimePrecision* nodep) override {
        putns(nodep, "vlSymsp->_vm_contextp__->timeprecision()");
    }

    // Nodes involing AstText
    व्योम visit(AstText* nodep) override {
        // All Text should be under TextBlock/CStmt/CStmtUser/CExpr/CExprUser
        nodep->v3fatalSrc("Text node in unexpected position");
    }
    व्योम visit(AstTextBlock* nodep) override {
        putnbs(nodep, "");
        माला_दो(nodep->prefix());
        emitNodesWithText(nodep->nodesp(), false, true, nodep->separator());
        माला_दो(nodep->suffix());
    }
    व्योम visit(AstCStmt* nodep) override {
        putnbs(nodep, "");
        emitNodesWithText(nodep->nodesp(), false, true, "");
        ensure_New_पंक्ति();
    }
    व्योम visit(AstCExpr* nodep) override {
        putnbs(nodep, "");
        emitNodesWithText(nodep->nodesp(), false, true, "");
    }
    व्योम visit(AstCStmtUser* nodep) override {
        putnbs(nodep, "");
        ofp()->putsNoTracking("\n");
        अगर (nodep->fromDollarC() && v3Global.opt.decoration() && !v3Global.opt.protectIds()) {
            ofp()->putsNoTracking("// $c statement at " + nodep->fileline()->ascii() + "\n");
        }
        emitNodesWithText(nodep->nodesp(), m_useSelfForThis, false, "");
        माला_दो("\n");
    }
    व्योम visit(AstCExprUser* nodep) override {
        putnbs(nodep, "");
        ofp()->putsNoTracking("\n");
        अगर (/* is always from $c */ v3Global.opt.decoration() && !v3Global.opt.protectIds()) {
            ofp()->putsNoTracking(
                (nodep->isPure() ? "// $cpure expression at " : "// $c expression at ")
                + nodep->fileline()->ascii() + "\n");
        }
        emitNodesWithText(nodep->nodesp(), m_useSelfForThis, false, "");
        माला_दो("\n");
    }

    // Operators
    व्योम visit(Ast_गाँठ_Termop* nodep) override {
        emitOpName(nodep, nodep->emitC(), nullptr, nullptr, nullptr);
    }
    व्योम visit(Ast_गाँठ_Uniop* nodep) override {
        अगर (nodep->emit_Check_Max_शब्दगण()
            && (nodep->width_शब्दगण() > VL_MULS_MAX_शब्दगण
                || nodep->lhsp()->width_शब्दगण() > VL_MULS_MAX_शब्दगण)) {
            nodep->v3warn(
                E_UNSUPPORTED,
                "Unsupported: "
                    << nodep->prettyOperatorName() << " operator of " << nodep->width()
                    << " bits exceeds hardcoded limit VL_MULS_MAX_WORDS in verilatedos.h");
        }
        अगर (emitSimpleOk(nodep)) {
            putnbs(nodep, "(");
            माला_दो(nodep->emitSimpleOperator());
            माला_दो(" ");
            iterate_And_अग्र_Const_Null(nodep->lhsp());
            माला_दो(")");
        } अन्यथा {
            emitOpName(nodep, nodep->emitC(), nodep->lhsp(), nullptr, nullptr);
        }
    }
    व्योम visit(Ast_गाँठ_Biop* nodep) override {
        अगर (nodep->emit_Check_Max_शब्दगण() && nodep->width_शब्दगण() > VL_MULS_MAX_शब्दगण) {
            nodep->v3warn(
                E_UNSUPPORTED,
                "Unsupported: "
                    << nodep->prettyOperatorName() << " operator of " << nodep->width()
                    << " bits exceeds hardcoded limit VL_MULS_MAX_WORDS in verilatedos.h");
        }
        अगर (emitSimpleOk(nodep)) {
            putnbs(nodep, "(");
            iterate_And_अग्र_Const_Null(nodep->lhsp());
            माला_दो(" ");
            putbs(nodep->emitSimpleOperator());
            माला_दो(" ");
            iterate_And_अग्र_Const_Null(nodep->rhsp());
            माला_दो(")");
        } अन्यथा {
            emitOpName(nodep, nodep->emitC(), nodep->lhsp(), nodep->rhsp(), nullptr);
        }
    }
    व्योम visit(Ast_गाँठ_Triop* nodep) override {
        UASSERT_OBJ(!emitSimpleOk(nodep), nodep, "Triop cannot be described in a simple way");
        emitOpName(nodep, nodep->emitC(), nodep->lhsp(), nodep->rhsp(), nodep->thsp());
    }
    व्योम visit(AstCvtPackString* nodep) override { emitCvtPackStr(nodep->lhsp()); }
    व्योम visit(AstRedXor* nodep) override {
        अगर (nodep->lhsp()->isWide()) {
            visit(static_cast<Ast_गाँठ_Uniop*>(nodep));
        } अन्यथा {
            स्थिर AstVarRef* स्थिर vrefp = VN_CAST(nodep->lhsp(), VarRef);
            स्थिर पूर्णांक widthPow2 = vrefp ? vrefp->varp()->dtypep()->widthPow2()
                                        : nodep->lhsp()->dtypep()->widthPow2();
            UASSERT_OBJ(widthPow2 > 1, nodep,
                        "Reduction over single bit value should have been folded");
            putnbs(nodep, "VL_REDXOR_");
            माला_दो(cvtToStr(widthPow2));
            माला_दो("(");
            iterate_And_अग्र_Const_Null(nodep->lhsp());
            माला_दो(")");
        }
    }
    व्योम visit(AstCCast* nodep) override {
        // Extending a value of the same word width is just a NOP.
        अगर (स्थिर AstClassRefDType* स्थिर classDtypep
            = VN_CAST(nodep->dtypep()->skipRefp(), ClassRefDType)) {
            putns(nodep, "(" + classDtypep->cType("", false, false) + ")(");
        } अन्यथा अगर (nodep->आकार() <= VL_BYTESIZE) {
            putns(nodep, "(::CData)(");
        } अन्यथा अगर (nodep->आकार() <= VL_SHORTSIZE) {
            putns(nodep, "(::SData)(");
        } अन्यथा अगर (nodep->आकार() <= VL_IDATASIZE) {
            putns(nodep, "(::IData)(");
        } अन्यथा {
            putns(nodep, "(::QData)(");
        }
        iterate_And_अग्र_Const_Null(nodep->lhsp());
        माला_दो(")");
    }
    व्योम visit(AstCond* nodep) override {
        // Widths match up already, so we'll just use C++'s operator w/o any temps.
        अगर (nodep->thenp()->isWide()) {
            emitOpName(nodep, nodep->emitC(), nodep->condp(), nodep->thenp(), nodep->elsep());
        } अन्यथा {
            putnbs(nodep, "(");
            iterate_And_अग्र_Const_Null(nodep->condp());
            putbs(" ? ");
            iterate_And_अग्र_Const_Null(nodep->thenp());
            putbs(" : ");
            iterate_And_अग्र_Const_Null(nodep->elsep());
            माला_दो(")");
        }
    }
    व्योम visit(AstMatchMasked* nodep) override {
        emitOpName(nodep, nodep->emitC(), nodep->lhsp(), nodep->matchp(), nullptr);
    }
    व्योम visit(AstMemberSel* nodep) override {
        स्थिर AstVar* स्थिर varp = nodep->varp();
        स्थिर bool dereferenceCovergroupRef
            = varp->covergroupRefMember() && nodep->access().isReadOrRW();
        अगर (dereferenceCovergroupRef) putnbs(nodep, "(*");
        iterate_And_अग्र_Const_Null(nodep->fromp());
        putnbs(nodep, "->");
        अगर (varp->isIfaceRef()) {
            // varp is the __Viftop companion (e.g. "tx__Viftop"); use the
            // MemberSel name which matches the cell's C++ member (e.g. "tx").
            माला_दो(nodep->nameProtect());
        } अन्यथा {
            माला_दो(varName(varp));
        }
        अगर (dereferenceCovergroupRef) माला_दो(")");
    }
    व्योम visit(AstStructSel* nodep) override {
        iterate_And_अग्र_Const_Null(nodep->fromp());
        putnbs(nodep, ".");
        माला_दो(nodep->nameProtect());
    }
    व्योम visit(AstNullCheck* nodep) override {
        putns(nodep, "VL_NULL_CHECK(");
        iterate_And_अग्र_Const_Null(nodep->lhsp());
        माला_दो(", ");
        putsQuoted(protect(nodep->fileline()->filename()));
        माला_दो(", ");
        माला_दो(cvtToStr(nodep->fileline()->lineno()));
        माला_दो(")");
    }
    व्योम visit(AstNewCopy* nodep) override {
        // Polymorphic shallow clone: preserves runtime type via virtual clone()
        // VL_NULL_CHECK enforces null check per IEEE 1800-2023 8.7
        putns(nodep, "VL_NULL_CHECK(");
        अगर (VN_IS(nodep->rhsp(), Const) && VN_AS(nodep->rhsp(), Const)->isNull()) {
            // V3Const folded rhs to null: emit a typed empty ref so VL_NULL_CHECK fires
            स्थिर AstClassRefDType* स्थिर refDTypep
                = VN_CAST(nodep->dtypep()->skipRefp(), ClassRefDType);
            माला_दो(refDTypep->cType("", false, false) + "{}");
        } अन्यथा {
            iterate_And_अग्र_Const_Null(nodep->rhsp());
        }
        माला_दो(", ");
        putsQuoted(protect(nodep->fileline()->filename()));
        माला_दो(", ");
        माला_दो(cvtToStr(nodep->fileline()->lineno()));
        माला_दो(").clone(vlSymsp->__Vm_deleter)");
    }
    व्योम visit(AstSel* nodep) override {
        // Note ASSIGN checks for this on a LHS
        UASSERT_OBJ(nodep->widthMin() == nodep->widthConst(), nodep, "Width mismatch");
        emitOpName(nodep, nodep->emitC(), nodep->fromp(), nodep->lsbp(), nullptr);
    }
    व्योम visit(AstReplicate* nodep) override {
        अगर (nodep->srcp()->widthMin() == 1 && !nodep->isWide()) {
            UASSERT_OBJ((static_cast<पूर्णांक>(VN_AS(nodep->countp(), Const)->toUInt())
                         * nodep->srcp()->widthMin())
                            == nodep->widthMin(),
                        nodep, "Replicate non-constant or width miscomputed");
            putns(nodep, "VL_REPLICATE_");
            emitIQW(nodep);
            माला_दो("OI(");
            अगर (nodep->srcp()) माला_दो(cvtToStr(nodep->srcp()->widthMin()));
            माला_दो(", ");
            iterate_And_अग्र_Const_Null(nodep->srcp());
            माला_दो(", ");
            iterate_And_अग्र_Const_Null(nodep->countp());
            माला_दो(")");
        } अन्यथा {
            emitOpName(nodep, nodep->emitC(), nodep->srcp(), nodep->countp(), nullptr);
        }
    }
    व्योम emitStreamR(AstStreamR* nodep, Ast_गाँठ* parent) {
        //TODO: This might need to handle more cases like the visit(AstStreamR) function
        emitOpName(nodep, nodep->emitC(), nodep->lhsp(), nodep->rhsp(), nullptr);
    }
    व्योम visit(AstStreamR* nodep) override {
        //The parrent node of our AstStreamR will give just enough info for what streamR should
        //output if nodep->backp() is not the parent then emitStreamR should have been used. throw
        //an error
        bool backpIsParent = (nodep->backp()->op1p() == nodep || nodep->backp()->op2p() == nodep);
        UASSERT(backpIsParent, "can not find return type for streamR");
        अगर (स्थिर AstQueueDType* स्थिर qdtp
            = VN_CAST(nodep->backp()->dtypep()->skipRefp(), QueueDType)) {
            अगर (nodep->lhsp()->isWide()) {
                emitOpName(nodep,
                           "VL_STREAMR_%nq%lq%rq(%lw," + std::to_string(qdtp->subDTypep()->width())
                               + ", %P, %li, %ri)",
                           nodep->lhsp(), nodep->rhsp(), nullptr);
            } अन्यथा {
                emitOpName(nodep, "VL_STREAMR_%nq%lq%rq(%lw, %P, %li, %ri)", nodep->lhsp(),
                           nodep->rhsp(), nullptr);
            }
        } अन्यथा अगर (VN_IS(nodep->lhsp()->dtypep()->skipRefp(), QueueDType)) {
            अगर (!((nodep->backp()->op1p() && nodep->backp()->op1p()->isWide())
                  || (nodep->backp()->op2p() && nodep->backp()->op2p()->isWide()))) {
                //If our lhsp is a queue make sure we streamR and return the correct type.
                //If either side is wide or the previous node is string type dont use this case
                emitOpName(nodep->backp(), "VL_STREAMR_%nq%lq%rq(%lw, %P, %li, %ri)",
                           nodep->lhsp(), nodep->rhsp(), nullptr);
            } अन्यथा {
                emitOpName(nodep, nodep->emitC(), nodep->lhsp(), nodep->rhsp(), nullptr);
            }
        } अन्यथा {
            emitOpName(nodep, nodep->emitC(), nodep->lhsp(), nodep->rhsp(), nullptr);
        }
    }
    व्योम visit(AstStreamL* nodep) override {
        // Attempt to use a "fast" stream function for slice size = power of 2
        अगर (!nodep->isWide()) {
            स्थिर uint32_t isPow2 = VN_AS(nodep->rhsp(), Const)->अंक().गिनती_Ones() == 1;
            स्थिर uint32_t slice_आकार = VN_AS(nodep->rhsp(), Const)->toUInt();
            अगर (isPow2 && slice_आकार <= (nodep->isQuad() ? माप(uint64_t) : माप(uint32_t))) {
                putns(nodep, "VL_STREAML_FAST_");
                bool usesQueue = false;
                AstQueueDType* qtypep
                    = nodep->backp()->op2p()
                          ? VN_CAST(nodep->backp()->op2p()->dtypep()->skipRefp(), QueueDType)
                          : nullptr;
                अगर (VN_IS(nodep->backp(), Assign)
                    && qtypep) {  // If we are assigning to a queue then emit the correct symbol
                    माला_दो("R");  // R for queue
                    usesQueue = true;
                } अन्यथा {
                    emitIQW(nodep);
                }
                emitIQW(nodep->lhsp());
                माला_दो("I(");
                माला_दो(cvtToStr(nodep->lhsp()->widthMin()));
                माला_दो(", ");
                अगर (usesQueue) {
                    iterate_And_अग्र_Const_Null(nodep->backp()->op2p());
                    माला_दो(", ");
                }
                iterate_And_अग्र_Const_Null(nodep->lhsp());
                माला_दो(", ");
                स्थिर uint32_t rd_log2 = V3_संख्या::log2b(VN_AS(nodep->rhsp(), Const)->toUInt());
                माला_दो(cvtToStr(rd_log2) + ")");
                वापस;
            }
        }
        अगर (VN_IS(nodep->backp(), Assign)
            && VN_IS(nodep->backp()->op2p()->dtypep()->skipRefp(), QueueDType)) {
            पूर्णांक queueWidth
                = nodep->backp()->op2p()->dtypep()->subDTypep()->width();  //We need to know the
                                                                           //width of both sides
            emitOpName(nodep,
                       "VL_STREAML_%nq%lq%rq(%lw," + std::to_string(queueWidth)
                           + ", %P, %li, %ri)",
                       nodep->lhsp(), nodep->rhsp(), nullptr);
        } अन्यथा {
            emitOpName(nodep, nodep->emitC(), nodep->lhsp(), nodep->rhsp(), nullptr);
        }
    }
    व्योम visit(AstCastDynamic* nodep) override {
        putnbs(nodep, "VL_CAST_DYNAMIC(");
        iterate_And_अग्र_Const_Null(nodep->fromp());
        माला_दो(", ");
        iterate_And_अग्र_Const_Null(nodep->शीर्ष());
        माला_दो(")");
    }
    व्योम visit(Ast_गिनती_Bits* nodep) override {
        putnbs(nodep, "VL_COUNTBITS_");
        emitIQW(nodep->lhsp());
        माला_दो("(");
        माला_दो(cvtToStr(nodep->lhsp()->widthMin()));
        माला_दो(", ");
        अगर (nodep->lhsp()->isWide()) {
            माला_दो(cvtToStr(nodep->lhsp()->width_शब्दगण()));  // Note argument width, not node width
                                                          // (which is always 32)
            माला_दो(", ");
        }
        iterate_And_अग्र_Const_Null(nodep->lhsp());
        माला_दो(", ");
        iterate_And_अग्र_Const_Null(nodep->rhsp());
        माला_दो(", ");
        iterate_And_अग्र_Const_Null(nodep->thsp());
        माला_दो(", ");
        iterate_And_अग्र_Const_Null(nodep->fhsp());
        माला_दो(")");
    }
    व्योम visit(AstTriggered* nodep) override {
        // Test each word of the trigger vector with a mask of the relevant bits
        std::map<uint32_t, uint64_t> wordMasks;
        क्रम (स्थिर uint32_t सूचकांक_2 : nodep->indices()) {
            wordMasks[VL_BITWORD_Q(सूचकांक_2)] |= VL_BIT_Q(सूचकांक_2);
        }
        putns(nodep, "((");
        bool first = true;
        क्रम (स्थिर स्वतः& wordMask : wordMasks) {
            अगर (!first) माला_दो("\n| ");
            first = false;
            स्थिर bool allOnes = wordMask.second == ~0ULL;
            अगर (!allOnes) माला_दो("(" + V3_संख्या{nodep, 64}.setQuad(wordMask.second).emitC() + " & ");
            iterateConst(nodep->trigRefp());
            माला_दो("[" + std::to_string(wordMask.first) + "U]");
            अगर (!allOnes) माला_दो(")");
        }
        माला_दो(") != 0ULL)");
    }
    // Terminals
    व्योम visit(AstVarRef* nodep) override {
        स्थिर AstVar* स्थिर varp = nodep->varp();

        // Reference to constant pool variable is special
        अगर (varp->constPoolEntry()) {
            putns(nodep, EmitCUtil::constPoolName(varp));
            वापस;
        }

        स्थिर Ast_गाँठ_Module* स्थिर varModp = EmitCParentModule::get(varp);
        स्थिर bool dereferenceCovergroupRef
            = varp->covergroupRefMember() && nodep->access().isReadOrRW();
        अगर (dereferenceCovergroupRef) putns(nodep, "(*");
        अगर (VN_IS(varModp, Class) && varModp != m_modp) {
            // Superclass member reference
            putns(nodep, EmitCUtil::prefixNameProtect(varModp) + "::");
        } अन्यथा अगर (varp->isIfaceRef()) {
            putns(nodep, nodep->selfPointerProtect(m_useSelfForThis));
            वापस;
        } अन्यथा अगर (!nodep->selfPointer().isEmpty()) {
            emitDereference(nodep, nodep->selfPointerProtect(m_useSelfForThis));
        }
        putns(nodep, varName(varp));
        अगर (dereferenceCovergroupRef) माला_दो(")");
    }
    व्योम visit(AstAddrOfCFunc* nodep) override {
        // Note: Can be thought to handle more, but this is all that is needed right now
        स्थिर AstCFunc* स्थिर funcp = nodep->funcp();
        UASSERT_OBJ(funcp->isLoose(), nodep, "Cannot take address of non-loose method");
        putns(nodep, "&");
        माला_दो(funcNameProtect(funcp));
    }
    व्योम visit(AstConst* nodep) override {  //
        अगर (m_wide_अस्थायी_Refp && nodep->isWide()) {
            UASSERT_OBJ(m_wide_अस्थायी_Refp, nodep, "Wide Constant w/ no temp");
            emitConstantW(nodep, m_wide_अस्थायी_Refp);
            m_wide_अस्थायी_Refp = nullptr;  // We used it, fail if set it a second time
        } अन्यथा {
            emitConstant(nodep);
        }
    }
    व्योम visit(Ast_गाँठ_Cons* nodep) override { emitValCons(nodep); }
    व्योम visit(AstThisRef* nodep) override {
        putnbs(nodep, nodep->dtypep()->cType("", false, false));
        माला_दो("{");
        माला_दो(VSelfPointerText::replaceThis(m_useSelfForThis, "this"));
        माला_दो("}");
    }
    व्योम visit(Ast_गाँठ_Sel* nodep) override {
        अगर (!VN_IS(nodep, ArraySel) && !VN_IS(nodep, WordSel)) {
            visit(static_cast<Ast_गाँठ_Biop*>(nodep));
            वापस;
        }
        // ArraySel or WordSel
        iterate_And_अग्र_Const_Null(nodep->fromp());
        // Special case constant index for readability
        अगर (AstConst* स्थिर idxp = VN_CAST(nodep->bitp(), Const)) {
            माला_दो("[" + std::to_string(idxp->toUInt()) + "U]");
            वापस;
        }
        putbs("[");
        iterate_And_अग्र_Const_Null(nodep->bitp());
        माला_दो("]");
    }

    //
    व्योम visit(AstExecGraph* nodep) override {
        // The location of the AstExecGraph within the containing AstCFunc is where we want to
        // invoke the graph and wait for it to complete. The run-time library does just that.
        putns(nodep, "vl_invokeExecGraph(" + EmitCUtil::execGraphName(nodep) + ", vlSelf, ");
        iterateConst(nodep->triggerp());
        माला_दो(".data());\n");
    }

    // Default
    व्योम visit(Ast_गाँठ* nodep) override {  // LCOV_EXCL_START
        putns(nodep, "\n???? // "s + nodep->prettyTypeName() + "\n");
        iterateChildrenConst(nodep);
        अगर (!v3Global.opt.lintOnly()) {  // An internal problem, so suppress
            nodep->v3fatalSrc("Unknown node type reached emitter: " << nodep->prettyTypeName());
        }
    }  // LCOV_EXCL_STOP

रक्षित:
    EmitCFunc() = शेष;
    ~EmitCFunc() override = शेष;
};

#पूर्ण_अगर  // guard
