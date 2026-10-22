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

#अन्तर्भाव "config_build.h"
#अन्तर्भाव "verilatedos.h"

#अन्तर्भाव "V3EmitCConstInit.h"
#अन्तर्भाव "V3Global.h"
#अन्तर्भाव "V3MemberMap.h"

#अन्तर्भाव <algorithm>
#अन्तर्भाव <map>
#अन्तर्भाव <unordered_map>
#अन्तर्भाव <unordered_set>
#अन्तर्भाव <vector>

// Number of VL_CONST_W_*X's in verilated.h (IE VL_CONST_W_8X is last)
constexpr पूर्णांक EMITC_अंक_CONSTW = 8;

//######################################################################
// Emit lazy forward declarations

class EmitCLazyDecls final : सार्वजनिक VNVisitorConst {
    // NODE STATE/TYPES
    // None allowed to support threaded emitting

    // MEMBERS
    std::unordered_set<string> m_emittedManually;  // Set of names already declared manually.
    EmitCBaseVisitorConst& m_emitter;  // For access to file output
    bool m_needs_Blank_ओळ = false;  // Emit blank line if any declarations were emitted (cosmetic)
    std::set<Ast_गाठ*> m_emitted;  // -> in set. Already emitted decl for symbols.

    // METHODS
    bool declaredOnce(Ast_गाठ* nodep) { परत m_emitted.घाला(nodep).second; }

    शून्य lazyDeclare(AstCFunc* funcp) {
        // Already declared in this compilation unit
        जर (!declaredOnce(funcp)) परत;
        // Check if this kind of function is lazily declared
        जर (!(funcp->isMethod() && funcp->isLoose()) && !funcp->dpiImportPrototype()) परत;
        // Already declared manually
        जर (m_emittedManually.मोजणी(funcp->nameProtect())) परत;
        // Needs lazy declaration, emit one
        m_emitter.emitCFuncDecl(funcp, EmitCParentModule::get(funcp), funcp->dpiImportPrototype());
        m_needs_Blank_ओळ = true;
    }

    शून्य lazyDeclareConstPoolVar(AstVar* varp) {
        जर (!declaredOnce(varp)) परत;  // Already declared
        स्थिरांक string nameProtect = EmitCUtil::constPoolName(varp);
        m_emitter.putns(varp, "extern const ");
        m_emitter.माला_दो(varp->dtypep()->cType(nameProtect, false, false));
        m_emitter.माला_दो(";\n");
        m_needs_Blank_ओळ = true;
    }

    // VISITORS
    शून्य visit(Ast_गाठ_CCall* nodep) override {
        lazyDeclare(nodep->funcp());
        iterateChildrenConst(nodep);
    }

    शून्य visit(AstAddrOfCFunc* nodep) override {
        lazyDeclare(nodep->funcp());
        iterateChildrenConst(nodep);
    }

    शून्य visit(AstVarRef* nodep) override {
        AstVar* स्थिरांक varp = nodep->varp();
        // Only constant pool symbols are lazy declared for now ...
        जर (EmitCParentModule::get(varp)->isConstPool()) lazyDeclareConstPoolVar(varp);
    }

    शून्य visit(AstExecGraph* nodep) override {
        जर (!declaredOnce(nodep)) परत;
        m_emitter.putns(nodep, "extern VlExecGraph " + EmitCUtil::execGraphName(nodep) + ";\n");
        m_needs_Blank_ओळ = true;
    }

    शून्य visit(Ast_गाठ* nodep) override { iterateChildrenConst(nodep); }

सार्वजनिक:
    explicit EmitCLazyDecls(EmitCBaseVisitorConst& emitter)
        : m_emitter{emitter} {}
    शून्य emit(Ast_गाठ* nodep) {
        m_needs_Blank_ओळ = false;
        iterateChildrenConst(nodep);
        जर (m_needs_Blank_ओळ) m_emitter.माला_दो("\n");
    }
    शून्य emit(स्थिरांक string& prefix, स्थिरांक string& name, स्थिरांक string& suffix) {
        m_emittedManually.घाला(name);
        m_emitter.ensure_New_ओळ();
        m_emitter.माला_दो(prefix);
        m_emitter.माला_दो(name);
        m_emitter.माला_दो(suffix);
        m_emitter.ensure_New_ओळ();
    }
    शून्य declared(AstCFunc* nodep) { m_emitted.घाला(nodep); }
    शून्य reset() { m_emitted.clear(); }
};

// ######################################################################
//  Emit statements and expressions

class EmitCFunc VL_NOT_FINAL : सार्वजनिक EmitCConstInit {
    VMemberMap m_memberMap;
    AstVarRef* m_wideTempRefp = nullptr;  // Variable that _WW macros should be setting
    std::unordered_map<AstJumpBlock*, माप_प्रकार> m_labelNumbers;  // Label numbers for AstJumpBlocks
    bool m_createdScopeHash = false;  // Already created a scope hash

संरक्षित:
    VL_DEFINE_DEBUG_FUNCTIONS;

    EmitCLazyDecls m_lazyDecls{*this};  // Visitor for emitting lazy declarations
    bool m_useSelfForThis = false;  // Replace "this" with "vlSelf"
    bool m_usevlSelfRef = false;  // Use vlSelfRef reference instead of vlSelf pointer
    स्थिरांक Ast_गाठ_Module* m_modp = nullptr;  // Current module being emitted
    स्थिरांक AstCFunc* m_cfuncp = nullptr;  // Current function being emitted
    bool m_instantiatesOwnProcess = false;
    स्थिरांक AstClassPackage* m_classOrPackage = nullptr;  // Pointer to current class or package
    string m_classOrPackageHash;  // Hash of class or package name

    bool constructorNeedsProcess(स्थिरांक AstClass* स्थिरांक classp) {
        स्थिरांक Ast_गाठ* स्थिरांक newp = m_memberMap.findMember(classp, "new");
        जर (!newp) परत false;
        स्थिरांक AstCFunc* स्थिरांक ctorp = VN_CAST(newp, CFunc);
        जर (!ctorp) परत false;
        UASSERT_OBJ(ctorp->isConstructor(), ctorp, "`new` is not a constructor!");
        परत ctorp->needProcess();
    }

    bool constructorNeedsProcess(स्थिरांक Ast_गाठ_DType* स्थिरांक dtypep) {
        जर (स्थिरांक AstClassRefDType* स्थिरांक crefdtypep = VN_CAST(dtypep, ClassRefDType))
            परत constructorNeedsProcess(crefdtypep->classp());
        परत false;
    }

    bool coverageUsesLocalCounter(Ast_गाठ_Cover_Decl* स्थिरांक declp) स्थिरांक {
        // Only functions with an object receiver can address the module-local
        // coverage array. Static/package/class helper functions still use the
        // symbol-table array, as they may have vlSymsp but no vlSelf/this.
        // Also require the coverage declaration to belong to the current
        // module; cloned task bodies can increment declarations owned by a
        // different module, and those must keep using the global array.
        परत m_cfuncp && !m_cfuncp->isStatic() && !VN_IS(m_modp, Class)
               && !VN_IS(m_modp, ClassPackage)
               && EmitCParentModule::get(declp->dataDeclThisp()) == m_modp;
    }
    पूर्णांक coverage_Bin_अंक(Ast_गाठ_Cover_Decl* स्थिरांक declp, bool forceGlobal = false) स्थिरांक {
        परत !forceGlobal && coverageUsesLocalCounter(declp)
                   ? declp->dataDeclThisp()->local_Bin_अंक()
                   : declp->dataDeclThisp()->bin_अंक();
    }
    शून्य putCoverageArray(Ast_गाठ_Cover_Decl* स्थिरांक declp, bool forceGlobal = false) {
        जर (!forceGlobal && coverageUsesLocalCounter(declp)) {
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
    शून्य emit_Cover_Other_Decl_घाला(AstCoverOtherDecl* nodep, bool forceGlobal = false) {
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
        माला_दो(!forceGlobal && coverageUsesLocalCounter(nodep) ? "true" : "false");
        माला_दो(", ");
        putsQuoted(protect(nodep->fileline()->filename()));
        माला_दो(", ");
        माला_दो(cvtToStr(nodep->fileline()->lineno()));
        माला_दो(", ");
        माला_दो(cvtToStr(nodep->offset() + nodep->fileline()->firstColumn()));
        माला_दो(", ");
        putsQuoted((!nodep->hier().empty() ? "." : "")
                   + VIdProtect::protect_शब्द_If(nodep->hier(), nodep->protect()));
        माला_दो(", ");
        putsQuoted(VIdProtect::protect_शब्द_If(nodep->page(), nodep->protect()));
        माला_दो(", ");
        putsQuoted(VIdProtect::protect_शब्द_If(nodep->comment(), nodep->protect()));
        माला_दो(", ");
        putsQuoted(nodep->linescov());
        माला_दो(", ");
        putsQuoted(VIdProtect::protect_शब्द_If(nodep->fsmVar(), nodep->protect()));
        माला_दो(", ");
        putsQuoted(VIdProtect::protect_शब्द_If(nodep->fsmFrom(), nodep->protect()));
        माला_दो(", ");
        putsQuoted(VIdProtect::protect_शब्द_If(nodep->fsmTo(), nodep->protect()));
        माला_दो(", ");
        putsQuoted(VIdProtect::protect_शब्द_If(nodep->fsmTag(), nodep->protect()));
        माला_दो(");\n");
    }

सार्वजनिक:
    // METHODS
    bool displayEmitHeader(Ast_गाठ* nodep);
    शून्य display_गाठ(Ast_गाठ* nodep, AstSFormatF* fmtp, स्थिरांक string& vformat, Ast_गाठ* exprsp,
                     bool isScan);

    bool emitSimpleOk(Ast_गाठ_अभिव्यक्ती* nodep);
    शून्य emitIQW(स्थिरांक Ast_गाठ* nodep) {
        // See "Type letters" in verilated.h
        // Other abbrevs: "C"har, "S"hort, "F"loat, "D"ouble, stri"N"g, "R"=queue, "U"npacked
        माला_दो(nodep->dtypep()->skipRefp()->charIQWN());
    }
    शून्य emitRU(स्थिरांक Ast_गाठ* nodep) {
        स्थिरांक Ast_गाठ_DType* dtp = nodep->dtypep()->skipRefp();
        // See "Type letters" in verilated.h
        जर (VN_IS(dtp, UnpackArrayDType))
            माला_दो("U");
        अन्यथा जर (VN_IS(dtp, QueueDType) || VN_IS(dtp, DynArrayDType))
            माला_दो("R");
    }
    शून्य emitScIQW(AstVar* nodep) {
        UASSERT_OBJ(nodep->isSc(), nodep, "emitting SystemC operator on non-SC variable");
        माला_दो(nodep->isScBigUint()                           ? "SB"
             : (nodep->isScUint() || nodep->isScUintBool()) ? "SU"
             : nodep->isScBv()                              ? "SW"
             : nodep->isDouble()                            ? "SD"
                                                            : (nodep->isScQuad() ? "SQ" : "SI"));
    }
    शून्य emitDatap(स्थिरांक Ast_गाठ* nodep) {
        // When passing to a function with va_args the compiler doesn't
        // know need a pointer so when wide, need to look inside VlWide
        जर (nodep->isWide()) माला_दो(".data()");
    }
    शून्य emitOpName(Ast_गाठ* nodep, स्थिरांक string& format, Ast_गाठ* lhsp, Ast_गाठ* rhsp,
                    Ast_गाठ* thsp);
    शून्य emitCCallArgs(स्थिरांक Ast_गाठ_CCall* nodep, स्थिरांक string& selfPointer, bool inProcess);
    शून्य emitDereference(Ast_गाठ* nodep, स्थिरांक string& pointer);
    std::string dereferenceString(स्थिरांक std::string& pointer) स्थिरांक;
    शून्य emitCvtPackStr(Ast_गाठ* nodep);
    शून्य emitConstant(AstConst* nodep);
    शून्य emitConstantString(स्थिरांक AstConst* nodep);
    शून्य emitVarReset(स्थिरांक string& prefix, AstVar* varp, bool constructing);
    string emitVarResetRecurse(स्थिरांक AstVar* varp, bool constructing,
                               स्थिरांक string& varNameProtected, Ast_गाठ_DType* dtypep, पूर्णांक depth,
                               स्थिरांक string& suffix, स्थिरांक Ast_गाठ* valuep);
    शून्य emitVarResetScopeHash();
    शून्य emitChangeDet();
    शून्य putCommaIterateNext(Ast_गाठ* nodep, bool comma = false) {
        साठी (Ast_गाठ* subnodep = nodep; subnodep; subnodep = subnodep->nextp()) {
            जर (comma) माला_दो(", ");
            iterateConst(subnodep);
            comma = true;
        }
    }
    ढाँचा <typename T>
    string optionalProcArg(स्थिरांक T* स्थिरांक nodep) {
        परत (nodep && constructorNeedsProcess(nodep)) ? "vlProcess, " : "";
    }
    स्थिरांक AstCNew* getSuperNewCallRecursep(Ast_गाठ* स्थिरांक nodep) {
        // Get the super.new call
        जर (!nodep) परत nullptr;
        जर (स्थिरांक AstCNew* स्थिरांक cnewp = VN_CAST(nodep, CNew)) परत cnewp;
        जर (स्थिरांक Ast_Stmt_अभिव्यक्ती* स्थिरांक stmtp = VN_CAST(nodep, Stmt_अभिव्यक्ती)) {
            जर (स्थिरांक AstCNew* स्थिरांक cnewp = VN_CAST(stmtp->exprp(), CNew)) परत cnewp;
        }
        जर (स्थिरांक AstJumpBlock* स्थिरांक blockp = VN_CAST(nodep, JumpBlock)) {
            जर (स्थिरांक AstCNew* स्थिरांक cnewp = getSuperNewCallRecursep(blockp->stmtsp())) {
                परत cnewp;
            }
        }
        जर (स्थिरांक AstCNew* स्थिरांक cnewp = getSuperNewCallRecursep(nodep->nextp())) परत cnewp;
        परत nullptr;
    }

    शून्य emitConstantW(AstConst* nodep, AstVarRef* assigntop) {
        // For tradition and compilation speed, assign each word directly into
        // output variable instead of using '='
        putns(nodep, "");
        जर (nodep->अंक().isFourState()) {
            nodep->v3warn(E_UNSUPPORTED, "Unsupported: 4-state numbers in this context");
            परत;
        }

        पूर्णांक upWidth = nodep->अंक().widthToFit();
        पूर्णांक chunks = 0;
        जर (upWidth > EMITC_अंक_CONSTW * VL_EDATASIZE) {
            // Output e.g. 8 words in groups of e.g. 8
            chunks = (upWidth - 1) / (EMITC_अंक_CONSTW * VL_EDATASIZE);
            upWidth %= (EMITC_अंक_CONSTW * VL_EDATASIZE);
            जर (upWidth == 0) upWidth = (EMITC_अंक_CONSTW * VL_EDATASIZE);
        }
        {  // Upper e.g. 8 words
            जर (chunks) {
                putnbs(nodep, "VL_CONSTHI_W_");
                माला_दो(cvtToStr(VL_शब्द_I(upWidth)));
                माला_दो("X(");
                माला_दो(cvtToStr(nodep->widthMin()));
                माला_दो(",");
                माला_दो(cvtToStr(chunks * EMITC_अंक_CONSTW * VL_EDATASIZE));
            } अन्यथा {
                putnbs(nodep, "VL_CONST_W_");
                माला_दो(cvtToStr(VL_शब्द_I(upWidth)));
                माला_दो("X(");
                माला_दो(cvtToStr(nodep->widthMin()));
            }
            माला_दो(",");
            जर (!assigntop->selfPointer().isEmpty()) {
                emitDereference(assigntop, assigntop->selfPointerProtect(m_useSelfForThis));
            }
            माला_दो(varName(assigntop->varp()));
            साठी (पूर्णांक word = VL_शब्द_I(upWidth) - 1; word >= 0; word--) {
                // Only 32 bits - llx + long long here just to appease CPP format warning
                ofp()->म_लिखो(",0x%08" PRIx64, static_cast<uint64_t>(nodep->अंक().edataWord(
                                                   word + chunks * EMITC_अंक_CONSTW)));
            }
            माला_दो(")");
        }
        साठी (chunks--; chunks >= 0; chunks--) {
            माला_दो(";\n");
            putbs("VL_CONSTLO_W_");
            माला_दो(cvtToStr(EMITC_अंक_CONSTW));
            माला_दो("X(");
            माला_दो(cvtToStr(chunks * EMITC_अंक_CONSTW * VL_EDATASIZE));
            माला_दो(",");
            जर (!assigntop->selfPointer().isEmpty()) {
                emitDereference(assigntop, assigntop->selfPointerProtect(m_useSelfForThis));
            }
            माला_दो(varName(assigntop->varp()));
            साठी (पूर्णांक word = EMITC_अंक_CONSTW - 1; word >= 0; word--) {
                // Only 32 bits - llx + long long here just to appease CPP format warning
                ofp()->म_लिखो(",0x%08" PRIx64, static_cast<uint64_t>(nodep->अंक().edataWord(
                                                   word + chunks * EMITC_अंक_CONSTW)));
            }
            माला_दो(")");
        }
    }

    शून्य emitNodesWithText(Ast_गाठ* nodesp, bool useSelfForThis, bool tracking,
                           स्थिरांक std::string& separator) {
        साठी (Ast_गाठ* nodep = nodesp; nodep; nodep = nodep->nextp()) {
            जर (स्थिरांक AstText* स्थिरांक textp = VN_CAST(nodep, Text)) {
                स्थिरांक std::string text
                    = VSelfPointerText::replaceThis(useSelfForThis, textp->text());
                जर (tracking) {
                    माला_दो(text);
                } अन्यथा {
                    ofp()->putsNoTracking(text);
                }
            } अन्यथा {
                iterateConst(nodep);
            }
            जर (nodep->nextp()) माला_दो(separator);
        }
    }

    शून्य putConstructorSubinit(स्थिरांक AstClass* classp, AstCFunc* cfuncp) {
        // Direct non-virtual bases in declaration order
        साठी (स्थिरांक AstClassExtends* extp = classp->extendsp(); extp;
             extp = VN_AS(extp->nextp(), ClassExtends)) {
            जर (extp->classp()->isInterfaceClass()) सुरूठेवा;
            जर (extp->classp()->useVirtualPublic()) सुरूठेवा;
            माला_दो(EmitCUtil::prefixNameProtect(extp->classp()));
            माला_दो("::init");
            जर (constructorNeedsProcess(extp->classp())) {
                माला_दो("(vlProcess, vlSymsp");
            } अन्यथा {
                माला_दो("(vlSymsp");
            }
            // Handle super.new() args for the concrete parent
            जर (स्थिरांक AstCNew* स्थिरांक superNewCallp = getSuperNewCallRecursep(cfuncp->stmtsp())) {
                putCommaIterateNext(superNewCallp->argsp(), true);
            }
            माला_दो(");\n");
            // Since SystemVerilog allows only for extending from one class, for sure there won't
            // be any more constructors to be called. Even if there are some classes which are
            // "implemented", those are interface classes which do not have constructors.
            थांबा;
        }
    }

    // VISITORS
    using EmitCConstInit::visit;
    शून्य visit(AstCFunc* स्थिरांक nodep) override {
        जर (nodep->emptyBody() && !nodep->isLoose()) परत;
        VL_RESTORER(m_useSelfForThis);
        VL_RESTORER(m_cfuncp);
        VL_RESTORER(m_instantiatesOwnProcess);
        VL_RESTORER(m_createdScopeHash);
        m_cfuncp = nodep;
        m_instantiatesOwnProcess = false;
        m_labelNumbers.clear();  // No need to save/restore, all Jumps must be within the function

        splitSizeInc(nodep);

        माला_दो("\n");
        m_lazyDecls.emit(nodep);
        जर (nodep->ifdef() != "") putns(nodep, "#ifdef " + nodep->ifdef() + "\n");
        emitCFuncHeader(nodep, m_modp, /* withScope: */ true);

        माला_दो(" {\n");
        जर (nodep->isConstructor()) {
            स्थिरांक AstClass* स्थिरांक classp = VN_CAST(nodep->scopep()->modp(), Class);
            जर (classp && classp->extendsp()) putConstructorSubinit(classp, nodep);
        }

        // "+" in the debug indicates a print from the model
        माला_दो("VL_DEBUG_IF(VL_DBG_MSGF(\"+  ");
        साठी (पूर्णांक i = 0; i < m_modp->level(); ++i) माला_दो("  ");
        माला_दो(EmitCUtil::prefixNameProtect(m_modp));
        माला_दो(nodep->isLoose() ? "__" : "::");
        माला_दो(nodep->nameProtect() + "\\n\"); );\n");

        जर (nodep->isLoose()) {
            m_lazyDecls.declared(nodep);  // Defined here, so no longer needs declaration
            जर (!nodep->isStatic()) {  // Standard prologue
                m_useSelfForThis = true;
                जर (nodep->voidSelf()) {
                    स्थिरांक string className = EmitCUtil::prefixNameProtect(m_modp);
                    माला_दो(className + "* const vlSelf = static_cast<" + className
                         + "*>(vlVoidSelf);\n");
                }
                जर (!VN_IS(m_modp, Class)) {
                    माला_दो(EmitCUtil::symClassAssign());  // Uses vlSelf
                } अन्यथा {
                    माला_दो("(void)vlSelf;  // Prevent unused variable warning\n");
                }
            }
        }

        // Instantiate a process class if it's going to be needed somewhere later
        nodep->forall([&](स्थिरांक Ast_गाठ_CCall* ccallp) -> bool {
            जर (ccallp->funcp()->needProcess()
                && (ccallp->funcp()->isCoroutine() == VN_IS(ccallp->backp(), CAwait))) {
                जर (!nodep->needProcess() && !m_instantiatesOwnProcess) {
                    m_instantiatesOwnProcess = true;
                    परत false;
                }
            }
            परत true;
        });
        जर (m_instantiatesOwnProcess) {
            AstCStmt* स्थिरांक vlprocp = new AstCStmt{nodep->fileline()};
            vlprocp->add("VlProcessRef vlProcess = std::make_shared<VlProcess>();\n");
            vlprocp->add("VlProcess::currentp(vlProcess.get());");
            nodep->prependStmtsp(vlprocp);
        } अन्यथा जर (nodep->needProcess() && nodep->stmtsp()) {
            // Set current process so VlRNG() constructors in this function seed from it
            AstCStmt* स्थिरांक setProcessp = new AstCStmt{nodep->fileline()};
            setProcessp->add("VlProcess::currentp(vlProcess.get());");
            nodep->prependStmtsp(setProcessp);
        }

        साठी (Ast_गाठ* subnodep = nodep->argsp(); subnodep; subnodep = subnodep->nextp()) {
            जर (AstVar* स्थिरांक varp = VN_CAST(subnodep, Var)) {
                जर (varp->isFuncReturn()) emitVarDecl(varp);
            }
        }

        जर (m_useSelfForThis) {
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

        जर (nodep->varsp()) {
            putsDecoration(nodep, "// Locals\n");
            iterateAndNextConstNull(nodep->varsp());
        }

        जर (nodep->stmtsp()) {
            putsDecoration(nodep, "// Body\n");
            iterateAndNextConstNull(nodep->stmtsp());
        }

        m_usevlSelfRef = false;

        जर (nodep->isCoroutine()) {
            // Sometimes coroutines don't have co_awaits,
            // so emit a co_return at the end to avoid compile errors.
            माला_दो("co_return;\n");
        }

        माला_दो("}\n");
        जर (nodep->ifdef() != "") माला_दो("#endif  // " + nodep->ifdef() + "\n");
    }

    शून्य visit(AstVar* nodep) override {
        UASSERT_OBJ(m_cfuncp, nodep, "Cannot emit non-local variable");
        emitVarDecl(nodep);
    }

    शून्य visit(AstCvtArrayToArray* nodep) override {
        जर (nodep->reverse()) {
            माला_दो("VL_REVCLONE_Q(");
        } अन्यथा {
            माला_दो("VL_CLONE_Q(");
        }
        iterateAndNextConstNull(nodep->fromp());
        माला_दो(", ");
        माला_दो(cvtToStr(nodep->blockSize()));
        माला_दो(", ");
        माला_दो(cvtToStr(nodep->srcElementBits()));
        माला_दो(", ");
        माला_दो(cvtToStr(nodep->dstElementBits()));
        माला_दो(")");
    }

    शून्य visit(AstCvtArrayToPacked* nodep) override {
        Ast_गाठ_DType* स्थिरांक fromDtp = nodep->fromp()->dtypep()->skipRefp();
        Ast_गाठ_DType* स्थिरांक elemDtp = fromDtp->subDTypep()->skipRefp();
        स्थिरांक bool descending = [&]() {
            जर (स्थिरांक AstUnpackArrayDType* स्थिरांक unpackDtp = VN_CAST(fromDtp, UnpackArrayDType))
                परत !unpackDtp->declRange().ascending();
            परत false;
        }();
        माला_दो("VL_PACK_");
        emitIQW(nodep);
        माला_दो("_");
        emitRU(fromDtp);
        emitIQW(elemDtp);
        जर (descending) {
            // Wrap source in VL_PACK_REVERSED so VL_PACK sees ascending order
            emitOpName(nodep, "(%nw, %rw, %P, VL_PACK_REVERSED(%li))", nodep->fromp(), elemDtp,
                       nullptr);
        } अन्यथा {
            emitOpName(nodep, "(%nw, %rw, %P, %li)", nodep->fromp(), elemDtp, nullptr);
        }
    }

    शून्य visit(AstCvtUnpackToQueue* nodep) override {
        Ast_गाठ_DType* स्थिरांक elemDTypep = nodep->fromp()->dtypep()->subDTypep();
        emitOpName(nodep, nodep->emitC(), nodep->fromp(), elemDTypep, nullptr);
    }
    शून्य visit(AstCvtUnpackToUnpack* nodep) override {
        // Whether each dimension is reversed, the first for the outermost, up to the last that is
        std::vector<bool> reverses;
        स्थिरांक AstUnpackArrayDType* शीर्ष = VN_AS(nodep->dtypep()->skipRefp(), UnpackArrayDType);
        स्थिरांक AstUnpackArrayDType* fromp
            = VN_AS(nodep->fromp()->dtypep()->skipRefp(), UnpackArrayDType);
        पर्यंत (शीर्ष) {
            reverses.ढकला_back(शीर्ष->declRange().ascending() != fromp->declRange().ascending());
            शीर्ष = VN_CAST(शीर्ष->subDTypep()->skipRefp(), UnpackArrayDType);
            fromp = VN_CAST(fromp->subDTypep()->skipRefp(), UnpackArrayDType);
        }
        पर्यंत (!reverses.back()) reverses.काढा_back();
        putns(nodep, "VL_CVT_UNPACK_TO_UNPACK<");
        साठी (माप_प्रकार i = 0; i < reverses.size(); ++i) {
            जर (i) माला_दो(", ");
            माला_दो(reverses[i] ? "1" : "0");
        }
        माला_दो(">(");
        iterateAndNextConstNull(nodep->fromp());
        माला_दो(")");
    }

    शून्य visit(Ast_गाठ_Assign* nodep) override {
        जर (AstCReset* स्थिरांक resetp = VN_CAST(nodep->rhsp(), CReset)) {
            // TODO get rid of emitVarReset and instead let AstNodeAssign understand how to init
            // anything
            Ast_गाठ* fromp = nodep->lhsp();
            // Fork needs to use a member select.  Nothing else should be possible before VarRef.
            जर (AstMemberSel* स्थिरांक sfromp = VN_CAST(fromp, MemberSel)) {
                // Fork-DynScope generated pointer to previously automatic variable
                AstVar* स्थिरांक memberVarp = sfromp->varp();
                fromp = sfromp->fromp();
                जर (AstNullCheck* स्थिरांक sfromp = VN_CAST(fromp, NullCheck)) fromp = sfromp->lhsp();
                Ast_गाठ_Var_Ref* स्थिरांक fromVarRefp = VN_AS(fromp, गाठ_Var_Ref);
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
                Ast_गाठ_Var_Ref* स्थिरांक fromVarRefp = VN_AS(fromp, गाठ_Var_Ref);
                AstVar* स्थिरांक varp = fromVarRefp->varp();
                स्थिरांक string prefix
                    = fromVarRefp->selfPointer().isEmpty()
                          ? ""
                          : dereferenceString(
                                VN_AS(fromp, गाठ_Var_Ref)->selfPointerProtect(m_useSelfForThis));
                emitVarReset(prefix, varp, resetp->constructing());
            }
            परत;
        }
        bool paren = true;
        bool decind = false;
        bool rhs = true;
        bool reverseUnpack = false;  // Set for descending CvtPackedToArray
        स्थिरांक AstUnpackArrayDType* स्थिरांक unpackDtp
            = VN_CAST(nodep->dtypep()->skipRefp(), UnpackArrayDType);
        जर (AstSel* स्थिरांक selp = VN_CAST(nodep->lhsp(), Sel)) {
            UASSERT_OBJ(selp->widthMin() == selp->widthConst(), selp, "Width mismatch");
            जर (selp->widthMin() == 1) {
                putnbs(nodep, "VL_ASSIGNBIT_");
                emitIQW(selp->fromp());
                जर (nodep->rhsp()->isAllOnesV()) {
                    माला_दो("O(");
                    rhs = false;
                } अन्यथा {
                    माला_दो("I(");
                }
                iterateAndNextConstNull(selp->lsbp());
                माला_दो(", ");
                iterateAndNextConstNull(selp->fromp());
                जर (rhs) माला_दो(", ");
            } अन्यथा {
                putnbs(nodep, "VL_ASSIGNSEL_");
                emitIQW(selp->fromp());
                emitIQW(nodep->rhsp());
                माला_दो("(");
                putns(selp->fromp(), cvtToStr(selp->fromp()->widthMin()) + ", ");
                माला_दो(cvtToStr(nodep->widthMin()) + ", ");
                iterateAndNextConstNull(selp->lsbp());
                माला_दो(", ");
                iterateAndNextConstNull(selp->fromp());
                माला_दो(", ");
            }
        } अन्यथा जर (स्थिरांक AstGetcRefN* स्थिरांक selp = VN_CAST(nodep->lhsp(), GetcRefN)) {
            iterateAndNextConstNull(selp->lhsp());
            माला_दो(" = ");
            putnbs(selp, "VL_PUTC_N(");
            iterateAndNextConstNull(selp->lhsp());
            माला_दो(", ");
            iterateAndNextConstNull(selp->rhsp());
            माला_दो(", ");
        } अन्यथा जर (AstVar* स्थिरांक varp = AstVar::scVarRecurse(nodep->lhsp())) {
            putnbs(varp, "VL_ASSIGN_");  // Set a systemC variable
            emitScIQW(varp);
            emitIQW(nodep);
            माला_दो("(");
            माला_दो(cvtToStr(nodep->widthMin()) + ", ");
            iterateAndNextConstNull(nodep->lhsp());
            माला_दो(", ");
        } अन्यथा जर (AstVar* स्थिरांक varp = AstVar::scVarRecurse(nodep->rhsp())) {
            putnbs(varp, "VL_ASSIGN_");  // Get a systemC variable
            emitIQW(nodep);
            emitScIQW(varp);
            माला_दो("(");
            माला_दो(cvtToStr(nodep->widthMin()) + ", ");
            iterateAndNextConstNull(nodep->lhsp());
            माला_दो(", ");
        } अन्यथा जर (स्थिरांक AstCvtPackedToArray* स्थिरांक castp
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
            iterateAndNextConstNull(nodep->lhsp());
            माला_दो(", ");
            rhs = false;
            iterateAndNextConstNull(castp->fromp());
            // Descending unpacked dest: reverse after unpack
            जर (unpackDtp && !unpackDtp->declRange().ascending()) reverseUnpack = true;
        } अन्यथा जर (स्थिरांक AstCvtArrayToArray* स्थिरांक castp
                   = VN_CAST(nodep->rhsp(), CvtArrayToArray)) {
            जर (castp->reverse()) {
                putns(castp, "VL_REVCOPY_Q(");
            } अन्यथा {
                putns(castp, "VL_COPY_Q(");
            }
            iterateAndNextConstNull(nodep->lhsp());
            माला_दो(", ");
            rhs = false;
            iterateAndNextConstNull(castp->fromp());
            माला_दो(", ");
            माला_दो(cvtToStr(castp->blockSize()));
            माला_दो(", ");
            माला_दो(cvtToStr(castp->srcElementBits()));
            माला_दो(", ");
            माला_दो(cvtToStr(castp->dstElementBits()));
        } अन्यथा जर (nodep->isWide() && VN_IS(nodep->lhsp(), VarRef)  //
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
                   && !VN_IS(nodep->rhsp(), अभिव्यक्ती_Stmt)) {
            // Wide functions assign into the array directly, don't need separate assign statement
            m_wideTempRefp = VN_AS(nodep->lhsp(), VarRef);
            paren = false;
        } अन्यथा जर (nodep->isWide() && !unpackDtp && !VN_IS(nodep->rhsp(), Const)) {
            putnbs(nodep, "VL_ASSIGN_W(");
            माला_दो(cvtToStr(nodep->widthMin()) + ", ");
            iterateAndNextConstNull(nodep->lhsp());
            माला_दो(", ");
        } अन्यथा जर (VN_IS(nodep->lhsp()->dtypep()->skipRefp(), QueueDType)
                   && (VN_IS(nodep->rhsp(), StreamL) || VN_IS(nodep->lhsp(), StreamL)
                       || VN_IS(nodep->rhsp(), StreamR) || VN_IS(nodep->lhsp(), StreamR)
                       || VN_IS(nodep->rhsp(), StreamR))) {
            //if either side is streamL or streamR don't emit lhsp everything will be passed by
            //reference

            paren = false;

        } अन्यथा {
            paren = false;
            iterateAndNextConstNull(nodep->lhsp());
            माला_दो(" ");
            ofp()->blockInc();
            decind = true;
            जर (!VN_IS(nodep->rhsp(), Const)) ofp()->putBreak();
            putns(nodep, "= ");
        }
        जर (rhs) iterateAndNextConstNull(nodep->rhsp());
        जर (paren) माला_दो(")");
        जर (decind) ofp()->blockDec();
        माला_दो(";\n");
        जर (reverseUnpack) {
            माला_दो("VL_UNPACK_REVERSED(");
            iterateAndNextConstNull(nodep->lhsp());
            माला_दो(");\n");
        }
    }
    शून्य visit(AstAssocSel* nodep) override {
        iterateAndNextConstNull(nodep->fromp());
        स्थिरांक std::string atFunc = nodep->isLValue() ? ".atWrite(" : ".at(";
        putnbs(nodep, atFunc);
        AstAssocArrayDType* स्थिरांक adtypep
            = VN_AS(nodep->fromp()->dtypep()->skipRefp(), AssocArrayDType);
        UASSERT_OBJ(adtypep, nodep, "Associative select on non-associative type");
        iterateAndNextConstNull(nodep->bitp());
        माला_दो(")");
    }
    शून्य visit(AstWildcardSel* nodep) override {
        iterateAndNextConstNull(nodep->fromp());
        स्थिरांक std::string atFunc = nodep->isLValue() ? ".atWrite(" : ".at(";
        putnbs(nodep, atFunc);
        AstWildcardArrayDType* स्थिरांक adtypep
            = VN_AS(nodep->fromp()->dtypep()->skipRefp(), WildcardArrayDType);
        UASSERT_OBJ(adtypep, nodep, "Wildcard select on non-wildcard-associative type");
        iterateAndNextConstNull(nodep->bitp());
        माला_दो(")");
    }
    शून्य visit(AstCCall* nodep) override {
        स्थिरांक AstCFunc* स्थिरांक funcp = nodep->funcp();
        स्थिरांक Ast_गाठ_Module* स्थिरांक funcModp = EmitCParentModule::get(funcp);
        putnbs(nodep, "");
        जर (funcp->dpiImportPrototype()) {
            // Calling DPI import
            putns(nodep, funcp->name());
        } अन्यथा जर (funcp->isProperMethod() && funcp->isStatic()) {
            // Call static method via the containing class
            putns(funcModp, EmitCUtil::prefixNameProtect(funcModp) + "::");
            putns(nodep, funcp->nameProtect());
        } अन्यथा जर (nodep->superReference()) {
            // Calling superclass method
            putns(funcModp, EmitCUtil::prefixNameProtect(funcModp) + "::");
            putns(nodep, funcp->nameProtect());
        } अन्यथा जर (funcp->isLoose()) {
            // Calling loose method
            putns(nodep, funcNameProtect(funcp));
        } अन्यथा {
            // Calling regular method/function
            जर (!nodep->selfPointer().isEmpty()) {
                emitDereference(nodep, nodep->selfPointerProtect(m_useSelfForThis));
            }
            putns(nodep, funcp->nameProtect());
        }
        emitCCallArgs(nodep, nodep->selfPointerProtect(m_useSelfForThis), m_cfuncp->needProcess());
    }
    शून्य visit(AstCMethodCall* nodep) override {
        स्थिरांक AstCFunc* स्थिरांक funcp = nodep->funcp();
        UASSERT_OBJ(!funcp->isLoose(), nodep, "Loose method called via AstCMethodCall");
        iterateConst(nodep->fromp());
        putnbs(nodep, "->");
        जर (nodep->superReference()) {
            // Super call, so the base implementation is used rather than the override
            स्थिरांक Ast_गाठ_Module* स्थिरांक funcModp = EmitCParentModule::get(funcp);
            putns(funcModp, EmitCUtil::prefixNameProtect(funcModp) + "::");
        }
        putns(funcp, funcp->nameProtect());
        emitCCallArgs(nodep, "", m_cfuncp->needProcess());
    }
    शून्य visit(AstCAwait* nodep) override {
        putns(nodep, "co_await ");
        iterateConst(nodep->exprp());
        माला_दो(";\n");
    }
    शून्य visit(AstCNew* nodep) override {
        जर (VN_IS(nodep->dtypep(), VoidDType)) {  // super.new case
            putsDecoration(nodep, "/*super.new*/");
            परत;
        }
        // assignment case;
        putns(nodep, "VL_NEW(" + EmitCUtil::prefixNameProtect(nodep->dtypep()) + ", "
                         + optionalProcArg(nodep->dtypep()) + "vlSymsp");
        putCommaIterateNext(nodep->argsp(), true);
        माला_दो(")");
    }
    शून्य visit(AstCMethodHard* nodep) override {
        iterateConst(nodep->fromp());
        putns(nodep, nodep->usePtr() ? "->" : ".");
        putns(nodep, nodep->name());
        जर (nodep->method() == VCMethod::FORCE_READ_SEL) {
            emitIQW(nodep);
            जर (nodep->isWide()) माला_दो("<" + cvtToStr(nodep->dtypep()->width_शब्द()) + ">");
        } अन्यथा जर (nodep->method() == VCMethod::ARRAY_SLICE) {
            // VlUnpacked::slice<N_Out>(loIdx) - N_Out is the (fixed) result array size
            स्थिरांक AstUnpackArrayDType* स्थिरांक adtypep
                = VN_AS(nodep->dtypep()->skipRefp(), UnpackArrayDType);
            माला_दो("<" + cvtToStr(adtypep->elementsConst()) + ">");
        } अन्यथा जर (nodep->method() == VCMethod::COVERGROUP_ADD_COVERPOINT) {
            // The hit-list bound is a template argument of the returned VlCoverpointT<>, and the
            // node's own dtype is that type, so it is the one source of truth for both.
            स्थिरांक AstCoverpointDType* स्थिरांक cpdtypep
                = VN_AS(nodep->dtypep()->skipRefp(), CoverpointDType);
            माला_दो("<" + cvtToStr(cpdtypep->hitBound()) + ">");
        } अन्यथा जर (nodep->method() == VCMethod::COVERGROUP_ADD_CROSS) {
            स्थिरांक AstCoverCrossDType* स्थिरांक cxdtypep
                = VN_AS(nodep->dtypep()->skipRefp(), CoverCrossDType);
            माला_दो("<" + cxdtypep->cppTemplateArgs() + ">");
        }
        माला_दो("(");
        bool comma = false;
        पूर्णांक arg_अंक = 0;
        साठी (Ast_गाठ* subnodep = nodep->pinsp(); subnodep; subnodep = subnodep->nextp()) {
            जर (comma) माला_दो(", ");
            जर (nodep->method() == VCMethod::RANDOMIZER_HARD && arg_अंक == 1) {
                // For RANDOMIZER_HARD's filename argument (2nd arg after constraint),
                // apply protect() similar to VL_STOP to handle --protected flag
                जर (स्थिरांक AstCExpr* स्थिरांक cexprp = VN_CAST(subnodep, CExpr)) {
                    // Extract filename from the CExpr (which contains "filename")
                    std::string filename;
                    साठी (स्थिरांक Ast_गाठ* textnodep = cexprp->nodesp(); textnodep;
                         textnodep = textnodep->nextp()) {
                        जर (स्थिरांक AstText* स्थिरांक textp = VN_CAST(textnodep, Text)) {
                            filename = textp->text();
                            थांबा;
                        }
                    }
                    // Remove surrounding quotes if present
                    जर (filename.size() >= 2 && filename.front() == '"'
                        && filename.back() == '"') {
                        filename = filename.substr(1, filename.size() - 2);
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
        जर (nodep->withp()) {
            जर (comma) माला_दो(", ");
            iterateConst(nodep->withp());
        }
        माला_दो(")");
    }
    शून्य visit(AstCFuncHard* nodep) override {
        putns(nodep, nodep->name());
        माला_दो("(");
        putCommaIterateNext(nodep->pinsp());
        माला_दो(")");
    }
    शून्य visit(AstLambdaArgRef* nodep) override { putbs(nodep->nameProtect()); }
    शून्य visit(AstWith* nodep) override {
        // With uses a C++11 lambda
        putnbs(nodep, "[&](");
        जर (स्वतः* स्थिरांक argrefp = nodep->indexArgRefp()) {
            putnbs(argrefp, argrefp->dtypep()->cType(argrefp->nameProtect(), false, false));
            माला_दो(", ");
        }
        जर (स्वतः* स्थिरांक argrefp = nodep->मूल्य_Arg_Refp()) {
            putnbs(argrefp, argrefp->dtypep()->cType(argrefp->nameProtect(), false, false));
        }
        माला_दो(") -> ");
        putnbs(nodep, nodep->dtypep()->cType("", false, false));
        माला_दो(" {\n");
        VL_RESTORER(m_createdScopeHash);
        iterateAndNextConstNull(nodep->exprp());
        माला_दो("}\n");
    }
    शून्य visit(AstCase* nodep) override {  // LCOV_EXCL_LINE
        // In V3Case...
        nodep->v3fatalSrc("Case statements should have been reduced out");
    }
    शून्य visit(AstComment* nodep) override {
        string at;
        जर (nodep->showAt()) {
            at = " at " + nodep->fileline()->ascii();
            // If protecting, passthru less information about the design
            जर (!v3Global.opt.protectIds()) परत;
        }
        जर (!(nodep->protect() && v3Global.opt.protectIds())) {
            putsDecoration(nodep, "// "s + nodep->name() + at + "\n");
        }
        iterateChildrenConst(nodep);
    }
    शून्य visit(AstCoverOtherDecl* nodep) override {
        emit_Cover_Other_Decl_घाला(nodep);
        जर (coverageUsesLocalCounter(nodep)) {
            // Some task bodies are cloned into a caller module where the
            // original instance-local counter is not addressable. Those clones
            // fall back to the global bin number, so register that slot too.
            emit_Cover_Other_Decl_घाला(nodep, true);
        }
    }
    शून्य visit(AstCoverToggleDecl* nodep) override {
        putns(nodep, "vlSelf->__vlCoverToggleInsert(");  // As Declared in emitCoverageDecl
        माला_दो(cvtToStr(nodep->range().उजवा()));
        माला_दो(", ");
        माला_दो(cvtToStr(nodep->range().डावा()));
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
        माला_दो(coverageUsesLocalCounter(nodep) ? "true" : "false");
        माला_दो(", ");
        putsQuoted(protect(nodep->fileline()->filename()));
        माला_दो(", ");
        माला_दो(cvtToStr(nodep->fileline()->lineno()));
        माला_दो(", ");
        माला_दो(cvtToStr(nodep->fileline()->firstColumn()));
        माला_दो(", ");
        putsQuoted((!nodep->hier().empty() ? "." : "")
                   + VIdProtect::protect_शब्द_If(nodep->hier(), nodep->protect()));
        माला_दो(", ");
        putsQuoted(VIdProtect::protect_शब्द_If(nodep->page(), nodep->protect()));
        माला_दो(", ");
        putsQuoted(VIdProtect::protect_शब्द_If(nodep->comment(), nodep->protect()));
        माला_दो(");\n");
    }
    शून्य visit(AstCoverInc* nodep) override {
        जर (VN_IS(nodep->declp(), CoverOtherDecl)) {
            जर (v3Global.opt.threads() > 1) {
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
            जर (v3Global.opt.threads() > 1) {
                माला_दो("MT_");
            } अन्यथा {
                माला_दो("ST_");
            }
            emitIQW(nodep->toggleExprp());
            माला_दो("(");
            // Each bit of variable has 2 counters, so the size of variable is half of the size of
            // coverpoint
            माला_दो(cvtToStr(nodep->declp()->size() / 2));
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
    शून्य visit(AstDisableFork* nodep) override { putns(nodep, "vlProcess->disableFork();\n"); }
    शून्य visit(AstCReturn* nodep) override {
        putns(nodep, "return (");
        iterateAndNextConstNull(nodep->lhsp());
        माला_दो(");\n");
    }
    शून्य visit(AstDisplay* nodep) override {
        string text = nodep->fmtp()->text();
        जर (nodep->addNewline()) text += "\n";
        display_गाठ(nodep, nodep->fmtp(), text, nodep->fmtp()->exprsp(), false);
    }
    शून्य visit(AstDumpCtl* nodep) override {
        निवड (nodep->ctlType()) {
        स्थिती VDumpCtlType::खाता:
            putns(nodep, "vlSymsp->_vm_contextp__->dumpfile(");
            iterateConst(nodep->exprp());
            माला_दो(");\n");
            थांबा;
        स्थिती VDumpCtlType::VARS:
            // We ignore number of levels to dump in exprp()
            जर (v3Global.opt.trace()) {
                putns(nodep, "vlSymsp->_traceDumpOpen();\n");
            } अन्यथा {
                putns(nodep, "VL_PRINTF_MT(\"-Info: ");
                माला_दो(V3OutFormatter::quoteNameControls(protect(nodep->fileline()->filename())));
                माला_दो(":");
                माला_दो(cvtToStr(nodep->fileline()->lineno()));
                माला_दो(": $dumpvar ignored, as Verilated without --trace");
                माला_दो("\\n\");\n");
            }
            थांबा;
        स्थिती VDumpCtlType::ALL:
            // $dumpall currently ignored
            थांबा;
        स्थिती VDumpCtlType::FLUSH:
            // $dumpall currently ignored; would need rework of VCD single thread,
            // or flag we pass-through to next eval() iteration
            थांबा;
        स्थिती VDumpCtlType::LIMIT:
            // $dumplimit currently ignored
            थांबा;
        स्थिती VDumpCtlType::OFF:
            // Currently ignored as both Vcd and Fst do not support them, as would need "X" dump
            थांबा;
        स्थिती VDumpCtlType::ON:
            // Currently ignored as $dumpoff is also ignored
            थांबा;
        पूर्वनिर्धारित: nodep->v3fatalSrc("Bad case, unexpected " << nodep->ctlType().ascii());
        }
    }
    शून्य visit(AstScopeName* nodep) override {
        // For use under AstCCalls for dpiImports.  ScopeNames under
        // displays are handled in AstDisplay
        जर (!nodep->dpiExport()) {
            // this is where the DPI import context scope is set
            स्थिरांक string scope = nodep->scopeDpiName();
            putnbs(nodep, "(vlSymsp->" + protect("__Vscopep_" + scope) + ")");
        }
    }
    शून्य visit(AstSFormat* nodep) override {
        display_गाठ(nodep, nodep->fmtp(), nodep->fmtp()->text(), nodep->fmtp()->exprsp(), false);
    }
    शून्य visit(AstSFormatF* nodep) override {
        display_गाठ(nodep, nodep, nodep->text(), nodep->exprsp(), false);
    }
    शून्य visit(AstFScanF* nodep) override {
        display_गाठ(nodep, nullptr, nodep->text(), nodep->exprsp(), true);
    }
    शून्य visit(AstSScanF* nodep) override {
        display_गाठ(nodep, nullptr, nodep->text(), nodep->exprsp(), true);
    }
    शून्य visit(Ast_मूल्य_Plus_Args* nodep) override {
        putns(nodep, "VL_VALUEPLUSARGS_IN");
        emitIQW(nodep->outp());
        माला_दो("(");
        माला_दो(cvtToStr(nodep->outp()->widthMin()));
        माला_दो(", ");
        iterateConst(nodep->searchp());
        माला_दो(", ");
        putbs("");
        iterateAndNextConstNull(nodep->outp());
        माला_दो(")");
    }
    शून्य visit(AstTestPlusArgs* nodep) override {
        putns(nodep, "VL_TESTPLUSARGS_I(");
        iterateConst(nodep->searchp());
        माला_दो(")");
    }
    शून्य visit(AstFError* nodep) override {
        putns(nodep, "VL_FERROR_I");
        माला_दो(nodep->strp()->isString() ? "N(" : "W(");
        iterateAndNextConstNull(nodep->filep());
        putbs(", ");
        जर (nodep->strp()->isWide()) {
            माला_दो(cvtToStr(nodep->strp()->width_शब्द()));
            putbs(", ");
        }
        iterateAndNextConstNull(nodep->strp());
        माला_दो(")");
    }
    शून्य visit(AstFGetS* nodep) override {
        emitOpName(nodep, nodep->emitC(), nodep->strgp(), nodep->filep(), nullptr);
    }
    शून्य visit(AstFOpen* nodep) override {
        putns(nodep, "VL_FOPEN_NN(");
        iterateConst(nodep->filenamep());
        putbs(", ");
        iterateConst(nodep->modep());
        माला_दो(");\n");
    }
    शून्य visit(AstFOpenMcd* nodep) override {
        putns(nodep, "VL_FOPEN_MCD_N(");
        iterateConst(nodep->filenamep());
        माला_दो(");\n");
    }
    शून्य visit(Ast_गाठ_Read_Write_Mem* nodep) override {
        putns(nodep, nodep->cFuncPrefixp());
        माला_दो("N(");
        माला_दो(nodep->isHex() ? "true" : "false");
        putbs(", ");
        // Need real storage width
        माला_दो(cvtToStr(nodep->memp()->dtypep()->skipRefp()->subDTypep()->widthMin()));
        uint32_t array_lo = 0;
        bool memUsePtr = true;
        {
            स्थिरांक AstVarRef* स्थिरांक varrefp = VN_CAST(nodep->memp(), VarRef);
            जर (!varrefp) {
                nodep->v3error(nodep->verilogKwd() << " loading non-variable");
            } अन्यथा जर (VN_IS(varrefp->varp()->dtypeSkipRefp(), AssocArrayDType)
                       || VN_IS(varrefp->varp()->dtypeSkipRefp(), DynArrayDType)) {
                // nodep->memp() below will when verilated code is compiled create a C++ template
                memUsePtr = false;
            } अन्यथा जर (स्थिरांक AstUnpackArrayDType* स्थिरांक adtypep
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
            जर (memUsePtr) माला_दो(" &(");
            iterateAndNextConstNull(nodep->memp());
            जर (memUsePtr) माला_दो(")");
        }
        putbs(", ");
        जर (nodep->lsbp()) {
            iterateAndNextConstNull(nodep->lsbp());
        } अन्यथा {
            माला_दो(cvtToStr(array_lo));
        }
        putbs(", ");
        जर (nodep->msbp()) {
            iterateAndNextConstNull(nodep->msbp());
        } अन्यथा {
            माला_दो("~0ULL");
        }
        माला_दो(");\n");
    }
    शून्य visit(AstFClose* nodep) override {
        putns(nodep, "VL_FCLOSE_I(");
        iterateAndNextConstNull(nodep->filep());
        माला_दो("); ");
    }
    शून्य visit(AstFFlush* nodep) override {
        जर (!nodep->filep()) {
            putns(nodep, "VL_FFLUSH_MT();");
        } अन्यथा {
            putns(nodep, "VL_FFLUSH_I(");
            iterateAndNextConstNull(nodep->filep());
            माला_दो(");\n");
        }
    }
    शून्य visit(AstFSeek* nodep) override {
        putns(nodep, "(VL_FSEEK_I(");
        iterateAndNextConstNull(nodep->filep());
        माला_दो(", ");
        iterateAndNextConstNull(nodep->offset());
        माला_दो(", ");
        iterateAndNextConstNull(nodep->operation());
        माला_दो(") == -1 ? -1 : 0)");
    }
    शून्य visit(AstFTell* nodep) override {
        putns(nodep, "VL_FTELL_I(");
        iterateAndNextConstNull(nodep->filep());
        माला_दो(")");
    }
    शून्य visit(AstFRewind* nodep) override {
        putns(nodep, "(VL_FSEEK_I(");
        iterateAndNextConstNull(nodep->filep());
        माला_दो(", 0, 0) == -1 ? -1 : 0)");
    }
    शून्य visit(AstFRead* nodep) override {
        putns(nodep, "VL_FREAD_I(");
        माला_दो(cvtToStr(nodep->memp()->widthMin()));  // Need real storage width
        putbs(", ");
        uint32_t array_lo = 0;
        uint32_t array_size = 0;
        {
            स्थिरांक AstVarRef* स्थिरांक varrefp = VN_CAST(nodep->memp(), VarRef);
            जर (!varrefp) {
                nodep->v3error(nodep->verilogKwd() << " loading non-variable");
            } अन्यथा जर (VN_CAST(varrefp->varp()->dtypeSkipRefp(), BasicDType)) {
            } अन्यथा जर (स्थिरांक AstUnpackArrayDType* स्थिरांक adtypep
                       = VN_CAST(varrefp->varp()->dtypeSkipRefp(), UnpackArrayDType)) {
                array_lo = adtypep->lo();
                array_size = adtypep->elementsConst();
            } अन्यथा {
                nodep->v3error(nodep->verilogKwd()
                               << " loading other than unpacked-array variable");
            }
        }
        माला_दो(cvtToStr(array_lo));
        putbs(", ");
        माला_दो(cvtToStr(array_size));
        putbs(", ");
        माला_दो("&(");
        iterateAndNextConstNull(nodep->memp());
        माला_दो(")");
        putbs(", ");
        iterateAndNextConstNull(nodep->filep());
        putbs(", ");
        जर (nodep->startp()) {
            iterateAndNextConstNull(nodep->startp());
        } अन्यथा {
            माला_दो(cvtToStr(array_lo));
        }
        putbs(", ");
        जर (nodep->countp()) {
            iterateAndNextConstNull(nodep->countp());
        } अन्यथा {
            माला_दो(cvtToStr(array_size));
        }
        माला_दो(")");
    }
    शून्य visit(Ast_चळत_Trace_F* nodep) override { putns(nodep, "VL_STACKTRACE_N()"); }
    शून्य visit(Ast_चळत_Trace_T* nodep) override { putns(nodep, "VL_STACKTRACE();\n"); }
    शून्य visit(AstSystemT* nodep) override {
        putns(nodep, "(void)VL_SYSTEM_I");
        emitIQW(nodep->lhsp());
        माला_दो("(");
        जर (nodep->lhsp()->isWide()) {
            माला_दो(cvtToStr(nodep->lhsp()->width_शब्द()));
            putbs(", ");
        }
        iterateAndNextConstNull(nodep->lhsp());
        माला_दो(");\n");
    }
    शून्य visit(AstSystemF* nodep) override {
        putns(nodep, "VL_SYSTEM_I");
        emitIQW(nodep->lhsp());
        माला_दो("(");
        जर (nodep->lhsp()->isWide()) {
            माला_दो(cvtToStr(nodep->lhsp()->width_शब्द()));
            putbs(", ");
        }
        iterateAndNextConstNull(nodep->lhsp());
        माला_दो(")");
    }
    शून्य visit(Ast_Stmt_अभिव्यक्ती* nodep) override {
        जर (VN_IS(nodep->exprp()->dtypep()->skipRefp(), VoidDType)) {
            putns(nodep, "");
        } अन्यथा {
            putns(nodep, "std::ignore = ");
        }
        iterateConst(nodep->exprp());
        माला_दो(";\n");
    }
    शून्य visit(AstJumpBlock* nodep) override {
        // Allocate label number
        स्थिरांक माप_प्रकार n = m_labelNumbers.size();
        स्थिरांक bool newEntry = m_labelNumbers.emplace(nodep, n).second;
        UASSERT_OBJ(newEntry, nodep, "AstJumpBlock visited twide");
        // Emit
        putns(nodep, "{\n");  // Make it visually obvious label jumps outside these
        VL_RESTORER(m_createdScopeHash);
        iterateAndNextConstNull(nodep->stmtsp());
        माला_दो("__Vlabel" + std::to_string(n) + ": ;\n");
        माला_दो("}\n");
    }
    शून्य visit(AstJumpGo* nodep) override {
        // Retrieve target label number - must already exist (from enclosing AstJumpBlock)
        स्थिरांक माप_प्रकार n = m_labelNumbers.at(nodep->blockp());
        // Emit
        putns(nodep, "goto __Vlabel" + std::to_string(n) + ";\n");
    }
    शून्य visit(AstLoop* nodep) override {
        UASSERT_OBJ(!nodep->contsp(), nodep, "'contsp' only used before LinkJump");
        VL_RESTORER(m_createdScopeHash);
        // Special case when the AstLoopTest is first for output readability
        जर (AstLoopTest* स्थिरांक testp = VN_CAST(nodep->stmtsp(), LoopTest)) {
            putns(nodep, "while (");
            iterateConst(testp->condp());
            माला_दो(") {\n");
            iterateAndNextConstNull(testp->nextp());
            माला_दो("}\n");
            परत;
        }
        // Special case when the AstLoopTest is last for output readability
        जर (Ast_गाठ* lastp = nodep->stmtsp()) {
            पर्यंत (Ast_गाठ* स्थिरांक nextp = lastp->nextp()) lastp = nextp;
            जर (AstLoopTest* स्थिरांक testp = VN_CAST(lastp, LoopTest)) {
                putns(nodep, "do {\n");
                साठी (Ast_गाठ* p = nodep->stmtsp(); p != lastp; p = p->nextp()) iterateConst(p);
                माला_दो("} while (");
                iterateConst(testp->condp());
                माला_दो(");\n");
                परत;
            }
        }
        // Emit generic case directly
        putns(nodep, "while (true) {\n");
        iterateAndNextConstNull(nodep->stmtsp());
        माला_दो("}\n");
    }
    शून्य visit(AstLoopTest* nodep) override {
        VL_RESTORER(m_createdScopeHash);
        putns(nodep, "if (!(");
        iterateAndNextConstNull(nodep->condp());
        माला_दो(")) break;\n");
    }
    शून्य visit(AstCLocalScope* nodep) override {
        putns(nodep, "{\n");
        VL_RESTORER(m_createdScopeHash);
        iterateAndNextConstNull(nodep->stmtsp());
        माला_दो("}\n");
    }
    शून्य visit(Ast_गाठ_If* nodep) override {
        putns(nodep, "if (");
        जर (!nodep->branchPred().unknown()) {
            माला_दो(nodep->branchPred().ascii());
            माला_दो("((");  // Two parens, so that VL_UNLIKELY((class<foo,bar>)) works
        }
        iterateAndNextConstNull(nodep->condp());
        जर (!nodep->branchPred().unknown()) माला_दो("))");
        माला_दो(") {\n");
        {
            VL_RESTORER(m_createdScopeHash);
            iterateAndNextConstNull(nodep->thensp());
        }
        माला_दो("}");
        जर (!nodep->elsesp()) {
            माला_दो("\n");
        } अन्यथा {
            जर (VN_IS(nodep->elsesp(), गाठ_If) && !nodep->elsesp()->nextp()) {
                माला_दो(" else ");
                iterateAndNextConstNull(nodep->elsesp());
            } अन्यथा {
                VL_RESTORER(m_createdScopeHash);
                माला_दो(" else {\n");
                iterateAndNextConstNull(nodep->elsesp());
                माला_दो("}\n");
            }
        }
    }
    शून्य visit(Ast_अभिव्यक्ती_Stmt* nodep) override {
        VL_RESTORER(m_createdScopeHash);
        स्थिरांक bool containsAwait = nodep->exists([](AstCAwait*) -> bool { परत true; });
        जर (containsAwait) {
            UASSERT_OBJ(m_cfuncp && m_cfuncp->isCoroutine(), nodep,
                        "AstExprStmt with CAwait must be in coroutine");
            putnbs(nodep, "(co_await ([&]() -> VlCoroutine {\n");
            iterateAndNextConstNull(nodep->stmtsp());
            माला_दो("co_return;\n");
            जर (!nodep->hasResult()) {
                माला_दो("}()))");
                परत;
            }
            माला_दो("}()), ");
            iterateAndNextConstNull(nodep->resultp());
            माला_दो(")");
            परत;
        }
        // GCC allows compound statements in expressions, but this is not standard.
        // So we use an immediate-evaluation lambda and comma operator
        putnbs(nodep, "([&]() {\n");
        जर (!nodep->hasResult()) {
            iterateAndNextConstNull(nodep->stmtsp());
            माला_दो("}())");
            परत;
        }
        iterateAndNextConstNull(nodep->stmtsp());
        माला_दो("}(), ");
        iterateAndNextConstNull(nodep->resultp());
        माला_दो(")");
    }
    शून्य visit(AstStop* nodep) override {
        putns(nodep, "VL_STOP_MT(");
        putsQuoted(protect(nodep->fileline()->filename()));
        माला_दो(", ");
        माला_दो(cvtToStr(nodep->fileline()->lineno()));
        माला_दो(", \"\"");
        जर (nodep->isFatal()) माला_दो(", false");
        माला_दो(");\n");
    }
    शून्य visit(AstFinish* nodep) override {
        // Disable all the forks so they don't operate after simulation is finished.
        जर (m_cfuncp && m_cfuncp->needProcess()) putns(nodep, "vlProcess->disableFork();\n");
        putns(nodep, "VL_FINISH_MT(");
        putsQuoted(protect(nodep->fileline()->filename()));
        माला_दो(", ");
        माला_दो(cvtToStr(nodep->fileline()->lineno()));
        माला_दो(", \"\");\n");
    }
    शून्य visit(AstFinishFork* nodep) override {
        // Disable all the forks so they don't operate after simulation is finished.
        जर (m_cfuncp && m_cfuncp->needProcess()) putns(nodep, "vlProcess->disableFork();\n");
        putns(nodep, "VL_FINISH_MT(");
        putsQuoted(protect(nodep->fileline()->filename()));
        माला_दो(", ");
        माला_दो(cvtToStr(nodep->fileline()->lineno()));
        माला_दो(", \"\");\n");
        जर (m_cfuncp->isCoroutine()) {
            putns(nodep, "co_return;\n");
        } अन्यथा {
            putns(nodep, "return;\n");
        }
    }
    शून्य visit(AstPrintTimeScale* nodep) override {
        putns(nodep, "VL_PRINTTIMESCALE(");
        putsQuoted(protect(nodep->prettyName()));
        माला_दो(", ");
        putsQuoted(nodep->timeunit().ascii());
        माला_दो(", vlSymsp->_vm_contextp__);\n");
    }
    शून्य visit(AstRand* nodep) override {
        emitOpName(nodep, nodep->emitC(), nodep->seedp(), nullptr, nullptr);
    }
    शून्य visit(AstRandRNG* nodep) override {
        emitOpName(nodep, nodep->emitC(), nullptr, nullptr, nullptr);
    }
    शून्य visit(AstTime* nodep) override {
        putns(nodep, "VL_TIME_UNITED_Q(");
        UASSERT_OBJ(!nodep->timeunit().isNone(), nodep, "$time has no units");
        स्थिरांक द्विगुण समय
            = nodep->timeunit().multiplier() / v3Global.rootp()->timeprecision().multiplier();
        UASSERT_OBJ(समय >= 1, nodep, "TimeQ is less than 1, will result in division by zero");
        माला_दो(cvtToStr(समय));
        माला_दो(")");
    }
    शून्य visit(AstTimeD* nodep) override {
        putns(nodep, "VL_TIME_UNITED_D(");
        UASSERT_OBJ(!nodep->timeunit().isNone(), nodep, "$realtime has no units");
        माला_दो(cvtToStr(nodep->timeunit().multiplier()
                      / v3Global.rootp()->timeprecision().multiplier()));
        माला_दो(")");
    }
    शून्य visit(AstGetInitialRandomSeed* nodep) override {
        putns(nodep, "vlSymsp->_vm_contextp__->randSeed()");
    }
    शून्य visit(AstTimeFormat* nodep) override {
        putns(nodep, "VL_TIMEFORMAT_IINI(");
        जर (nodep->unitsp()) {
            माला_दो("true, ");
            iterateAndNextConstNull(nodep->unitsp());
        } अन्यथा {
            माला_दो("false, 0");
        }
        माला_दो(", ");
        जर (nodep->precisionp()) {
            माला_दो("true, ");
            iterateAndNextConstNull(nodep->precisionp());
        } अन्यथा {
            माला_दो("false, 0");
        }
        माला_दो(", ");
        जर (nodep->suffixp()) {
            माला_दो("true, ");
            iterateAndNextConstNull(nodep->suffixp());
        } अन्यथा {
            माला_दो("false, \"\"");
        }
        माला_दो(", ");
        जर (nodep->widthp()) {
            माला_दो("true, ");
            iterateAndNextConstNull(nodep->widthp());
        } अन्यथा {
            माला_दो("false, 0");
        }
        माला_दो(", vlSymsp->_vm_contextp__);\n");
    }
    शून्य visit(AstTimePrecision* nodep) override {
        putns(nodep, "vlSymsp->_vm_contextp__->timeprecision()");
    }

    // Nodes involing AstText
    शून्य visit(AstText* nodep) override {
        // All Text should be under TextBlock/CStmt/CStmtUser/CExpr/CExprUser
        nodep->v3fatalSrc("Text node in unexpected position");
    }
    शून्य visit(AstTextBlock* nodep) override {
        putnbs(nodep, "");
        माला_दो(nodep->prefix());
        emitNodesWithText(nodep->nodesp(), false, true, nodep->separator());
        माला_दो(nodep->suffix());
    }
    शून्य visit(AstCStmt* nodep) override {
        putnbs(nodep, "");
        emitNodesWithText(nodep->nodesp(), false, true, "");
        ensure_New_ओळ();
    }
    शून्य visit(AstCExpr* nodep) override {
        putnbs(nodep, "");
        emitNodesWithText(nodep->nodesp(), false, true, "");
    }
    शून्य visit(AstCStmtUser* nodep) override {
        putnbs(nodep, "");
        ofp()->putsNoTracking("\n");
        जर (nodep->fromDollarC() && v3Global.opt.decoration() && !v3Global.opt.protectIds()) {
            ofp()->putsNoTracking("// $c statement at " + nodep->fileline()->ascii() + "\n");
        }
        emitNodesWithText(nodep->nodesp(), m_useSelfForThis, false, "");
        माला_दो("\n");
    }
    शून्य visit(AstCExprUser* nodep) override {
        putnbs(nodep, "");
        ofp()->putsNoTracking("\n");
        जर (/* is always from $c */ v3Global.opt.decoration() && !v3Global.opt.protectIds()) {
            ofp()->putsNoTracking(
                (nodep->isPure() ? "// $cpure expression at " : "// $c expression at ")
                + nodep->fileline()->ascii() + "\n");
        }
        emitNodesWithText(nodep->nodesp(), m_useSelfForThis, false, "");
        माला_दो("\n");
    }

    // Operators
    शून्य visit(Ast_गाठ_Termop* nodep) override {
        emitOpName(nodep, nodep->emitC(), nullptr, nullptr, nullptr);
    }
    शून्य visit(Ast_गाठ_Uniop* nodep) override {
        जर (nodep->emit_Check_Max_शब्द()
            && (nodep->width_शब्द() > VL_MULS_MAX_शब्द
                || nodep->lhsp()->width_शब्द() > VL_MULS_MAX_शब्द)) {
            nodep->v3warn(
                E_UNSUPPORTED,
                "Unsupported: "
                    << nodep->prettyOperatorName() << " operator of " << nodep->width()
                    << " bits exceeds hardcoded limit VL_MULS_MAX_WORDS in verilatedos.h");
        }
        जर (emitSimpleOk(nodep)) {
            putnbs(nodep, "(");
            माला_दो(nodep->emitSimpleOperator());
            माला_दो(" ");
            iterateAndNextConstNull(nodep->lhsp());
            माला_दो(")");
        } अन्यथा {
            emitOpName(nodep, nodep->emitC(), nodep->lhsp(), nullptr, nullptr);
        }
    }
    शून्य visit(Ast_गाठ_Biop* nodep) override {
        जर (nodep->emit_Check_Max_शब्द() && nodep->width_शब्द() > VL_MULS_MAX_शब्द) {
            nodep->v3warn(
                E_UNSUPPORTED,
                "Unsupported: "
                    << nodep->prettyOperatorName() << " operator of " << nodep->width()
                    << " bits exceeds hardcoded limit VL_MULS_MAX_WORDS in verilatedos.h");
        }
        जर (emitSimpleOk(nodep)) {
            putnbs(nodep, "(");
            iterateAndNextConstNull(nodep->lhsp());
            माला_दो(" ");
            putbs(nodep->emitSimpleOperator());
            माला_दो(" ");
            iterateAndNextConstNull(nodep->rhsp());
            माला_दो(")");
        } अन्यथा {
            emitOpName(nodep, nodep->emitC(), nodep->lhsp(), nodep->rhsp(), nullptr);
        }
    }
    शून्य visit(Ast_गाठ_Triop* nodep) override {
        UASSERT_OBJ(!emitSimpleOk(nodep), nodep, "Triop cannot be described in a simple way");
        emitOpName(nodep, nodep->emitC(), nodep->lhsp(), nodep->rhsp(), nodep->thsp());
    }
    शून्य visit(AstCvtPackString* nodep) override { emitCvtPackStr(nodep->lhsp()); }
    शून्य visit(AstRedXor* nodep) override {
        जर (nodep->lhsp()->isWide()) {
            visit(static_cast<Ast_गाठ_Uniop*>(nodep));
        } अन्यथा {
            स्थिरांक AstVarRef* स्थिरांक vrefp = VN_CAST(nodep->lhsp(), VarRef);
            स्थिरांक पूर्णांक widthPow2 = vrefp ? vrefp->varp()->dtypep()->widthPow2()
                                        : nodep->lhsp()->dtypep()->widthPow2();
            UASSERT_OBJ(widthPow2 > 1, nodep,
                        "Reduction over single bit value should have been folded");
            putnbs(nodep, "VL_REDXOR_");
            माला_दो(cvtToStr(widthPow2));
            माला_दो("(");
            iterateAndNextConstNull(nodep->lhsp());
            माला_दो(")");
        }
    }
    शून्य visit(AstCCast* nodep) override {
        // Extending a value of the same word width is just a NOP.
        जर (स्थिरांक AstClassRefDType* स्थिरांक classDtypep
            = VN_CAST(nodep->dtypep()->skipRefp(), ClassRefDType)) {
            putns(nodep, "(" + classDtypep->cType("", false, false) + ")(");
        } अन्यथा जर (nodep->size() <= VL_BYTESIZE) {
            putns(nodep, "(::CData)(");
        } अन्यथा जर (nodep->size() <= VL_SHORTSIZE) {
            putns(nodep, "(::SData)(");
        } अन्यथा जर (nodep->size() <= VL_IDATASIZE) {
            putns(nodep, "(::IData)(");
        } अन्यथा {
            putns(nodep, "(::QData)(");
        }
        iterateAndNextConstNull(nodep->lhsp());
        माला_दो(")");
    }
    शून्य visit(AstCond* nodep) override {
        // Widths match up already, so we'll just use C++'s operator w/o any temps.
        जर (nodep->thenp()->isWide()) {
            emitOpName(nodep, nodep->emitC(), nodep->condp(), nodep->thenp(), nodep->elsep());
        } अन्यथा {
            putnbs(nodep, "(");
            iterateAndNextConstNull(nodep->condp());
            putbs(" ? ");
            iterateAndNextConstNull(nodep->thenp());
            putbs(" : ");
            iterateAndNextConstNull(nodep->elsep());
            माला_दो(")");
        }
    }
    शून्य visit(AstMatchMasked* nodep) override {
        emitOpName(nodep, nodep->emitC(), nodep->lhsp(), nodep->matchp(), nullptr);
    }
    शून्य visit(AstMemberSel* nodep) override {
        स्थिरांक AstVar* स्थिरांक varp = nodep->varp();
        स्थिरांक bool dereferenceCovergroupRef
            = varp->covergroupRefMember() && nodep->access().isReadOrRW();
        जर (dereferenceCovergroupRef) putnbs(nodep, "(*");
        iterateAndNextConstNull(nodep->fromp());
        putnbs(nodep, "->");
        जर (varp->isIfaceRef()) {
            // varp is the __Viftop companion (e.g. "tx__Viftop"); use the
            // MemberSel name which matches the cell's C++ member (e.g. "tx").
            माला_दो(nodep->nameProtect());
        } अन्यथा {
            माला_दो(varName(varp));
        }
        जर (dereferenceCovergroupRef) माला_दो(")");
    }
    शून्य visit(AstStructSel* nodep) override {
        iterateAndNextConstNull(nodep->fromp());
        putnbs(nodep, ".");
        माला_दो(nodep->nameProtect());
    }
    शून्य visit(AstNullCheck* nodep) override {
        putns(nodep, "VL_NULL_CHECK(");
        iterateAndNextConstNull(nodep->lhsp());
        माला_दो(", ");
        putsQuoted(protect(nodep->fileline()->filename()));
        माला_दो(", ");
        माला_दो(cvtToStr(nodep->fileline()->lineno()));
        माला_दो(")");
    }
    शून्य visit(AstNewCopy* nodep) override {
        // Polymorphic shallow clone: preserves runtime type via virtual clone()
        // VL_NULL_CHECK enforces null check per IEEE 1800-2023 8.7
        putns(nodep, "VL_NULL_CHECK(");
        जर (VN_IS(nodep->rhsp(), Const) && VN_AS(nodep->rhsp(), Const)->isNull()) {
            // V3Const folded rhs to null: emit a typed empty ref so VL_NULL_CHECK fires
            स्थिरांक AstClassRefDType* स्थिरांक refDTypep
                = VN_CAST(nodep->dtypep()->skipRefp(), ClassRefDType);
            माला_दो(refDTypep->cType("", false, false) + "{}");
        } अन्यथा {
            iterateAndNextConstNull(nodep->rhsp());
        }
        माला_दो(", ");
        putsQuoted(protect(nodep->fileline()->filename()));
        माला_दो(", ");
        माला_दो(cvtToStr(nodep->fileline()->lineno()));
        माला_दो(").clone(vlSymsp->__Vm_deleter)");
    }
    शून्य visit(AstSel* nodep) override {
        // Note ASSIGN checks for this on a LHS
        UASSERT_OBJ(nodep->widthMin() == nodep->widthConst(), nodep, "Width mismatch");
        emitOpName(nodep, nodep->emitC(), nodep->fromp(), nodep->lsbp(), nullptr);
    }
    शून्य visit(AstReplicate* nodep) override {
        जर (nodep->srcp()->widthMin() == 1 && !nodep->isWide()) {
            UASSERT_OBJ((static_cast<पूर्णांक>(VN_AS(nodep->countp(), Const)->toUInt())
                         * nodep->srcp()->widthMin())
                            == nodep->widthMin(),
                        nodep, "Replicate non-constant or width miscomputed");
            putns(nodep, "VL_REPLICATE_");
            emitIQW(nodep);
            माला_दो("OI(");
            जर (nodep->srcp()) माला_दो(cvtToStr(nodep->srcp()->widthMin()));
            माला_दो(", ");
            iterateAndNextConstNull(nodep->srcp());
            माला_दो(", ");
            iterateAndNextConstNull(nodep->countp());
            माला_दो(")");
        } अन्यथा {
            emitOpName(nodep, nodep->emitC(), nodep->srcp(), nodep->countp(), nullptr);
        }
    }
    शून्य emitStreamR(AstStreamR* nodep, Ast_गाठ* parent) {
        //TODO: This might need to handle more cases like the visit(AstStreamR) function
        emitOpName(nodep, nodep->emitC(), nodep->lhsp(), nodep->rhsp(), nullptr);
    }
    शून्य visit(AstStreamR* nodep) override {
        //The parrent node of our AstStreamR will give just enough info for what streamR should
        //output if nodep->backp() is not the parent then emitStreamR should have been used. throw
        //an error
        bool backpIsParent = (nodep->backp()->op1p() == nodep || nodep->backp()->op2p() == nodep);
        UASSERT(backpIsParent, "can not find return type for streamR");
        जर (स्थिरांक AstQueueDType* स्थिरांक qdtp
            = VN_CAST(nodep->backp()->dtypep()->skipRefp(), QueueDType)) {
            जर (nodep->lhsp()->isWide()) {
                emitOpName(nodep,
                           "VL_STREAMR_%nq%lq%rq(%lw," + std::to_string(qdtp->subDTypep()->width())
                               + ", %P, %li, %ri)",
                           nodep->lhsp(), nodep->rhsp(), nullptr);
            } अन्यथा {
                emitOpName(nodep, "VL_STREAMR_%nq%lq%rq(%lw, %P, %li, %ri)", nodep->lhsp(),
                           nodep->rhsp(), nullptr);
            }
        } अन्यथा जर (VN_IS(nodep->lhsp()->dtypep()->skipRefp(), QueueDType)) {
            जर (!((nodep->backp()->op1p() && nodep->backp()->op1p()->isWide())
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
    शून्य visit(AstStreamL* nodep) override {
        // Attempt to use a "fast" stream function for slice size = power of 2
        जर (!nodep->isWide()) {
            स्थिरांक uint32_t isPow2 = VN_AS(nodep->rhsp(), Const)->अंक().मोजणी_Ones() == 1;
            स्थिरांक uint32_t sliceSize = VN_AS(nodep->rhsp(), Const)->toUInt();
            जर (isPow2 && sliceSize <= (nodep->isQuad() ? आकार(uint64_t) : आकार(uint32_t))) {
                putns(nodep, "VL_STREAML_FAST_");
                bool usesQueue = false;
                AstQueueDType* qtypep
                    = nodep->backp()->op2p()
                          ? VN_CAST(nodep->backp()->op2p()->dtypep()->skipRefp(), QueueDType)
                          : nullptr;
                जर (VN_IS(nodep->backp(), Assign)
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
                जर (usesQueue) {
                    iterateAndNextConstNull(nodep->backp()->op2p());
                    माला_दो(", ");
                }
                iterateAndNextConstNull(nodep->lhsp());
                माला_दो(", ");
                स्थिरांक uint32_t rd_log2 = V3_संख्या::log2b(VN_AS(nodep->rhsp(), Const)->toUInt());
                माला_दो(cvtToStr(rd_log2) + ")");
                परत;
            }
        }
        जर (VN_IS(nodep->backp(), Assign)
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
    शून्य visit(AstCastDynamic* nodep) override {
        putnbs(nodep, "VL_CAST_DYNAMIC(");
        iterateAndNextConstNull(nodep->fromp());
        माला_दो(", ");
        iterateAndNextConstNull(nodep->शीर्ष());
        माला_दो(")");
    }
    शून्य visit(Ast_मोजणी_Bits* nodep) override {
        putnbs(nodep, "VL_COUNTBITS_");
        emitIQW(nodep->lhsp());
        माला_दो("(");
        माला_दो(cvtToStr(nodep->lhsp()->widthMin()));
        माला_दो(", ");
        जर (nodep->lhsp()->isWide()) {
            माला_दो(cvtToStr(nodep->lhsp()->width_शब्द()));  // Note argument width, not node width
                                                          // (which is always 32)
            माला_दो(", ");
        }
        iterateAndNextConstNull(nodep->lhsp());
        माला_दो(", ");
        iterateAndNextConstNull(nodep->rhsp());
        माला_दो(", ");
        iterateAndNextConstNull(nodep->thsp());
        माला_दो(", ");
        iterateAndNextConstNull(nodep->fhsp());
        माला_दो(")");
    }
    शून्य visit(AstTriggered* nodep) override {
        // Test each word of the trigger vector with a mask of the relevant bits
        std::map<uint32_t, uint64_t> wordMasks;
        साठी (स्थिरांक uint32_t index : nodep->indices()) {
            wordMasks[VL_BITWORD_Q(index)] |= VL_BIT_Q(index);
        }
        putns(nodep, "((");
        bool first = true;
        साठी (स्थिरांक स्वतः& wordMask : wordMasks) {
            जर (!first) माला_दो("\n| ");
            first = false;
            स्थिरांक bool allOnes = wordMask.second == ~0ULL;
            जर (!allOnes) माला_दो("(" + V3_संख्या{nodep, 64}.setQuad(wordMask.second).emitC() + " & ");
            iterateConst(nodep->trigRefp());
            माला_दो("[" + std::to_string(wordMask.first) + "U]");
            जर (!allOnes) माला_दो(")");
        }
        माला_दो(") != 0ULL)");
    }
    // Terminals
    शून्य visit(AstVarRef* nodep) override {
        स्थिरांक AstVar* स्थिरांक varp = nodep->varp();

        // Reference to constant pool variable is special
        जर (varp->constPoolEntry()) {
            putns(nodep, EmitCUtil::constPoolName(varp));
            परत;
        }

        स्थिरांक Ast_गाठ_Module* स्थिरांक varModp = EmitCParentModule::get(varp);
        स्थिरांक bool dereferenceCovergroupRef
            = varp->covergroupRefMember() && nodep->access().isReadOrRW();
        जर (dereferenceCovergroupRef) putns(nodep, "(*");
        जर (VN_IS(varModp, Class) && varModp != m_modp) {
            // Superclass member reference
            putns(nodep, EmitCUtil::prefixNameProtect(varModp) + "::");
        } अन्यथा जर (varp->isIfaceRef()) {
            putns(nodep, nodep->selfPointerProtect(m_useSelfForThis));
            परत;
        } अन्यथा जर (!nodep->selfPointer().isEmpty()) {
            emitDereference(nodep, nodep->selfPointerProtect(m_useSelfForThis));
        }
        putns(nodep, varName(varp));
        जर (dereferenceCovergroupRef) माला_दो(")");
    }
    शून्य visit(AstAddrOfCFunc* nodep) override {
        // Note: Can be thought to handle more, but this is all that is needed right now
        स्थिरांक AstCFunc* स्थिरांक funcp = nodep->funcp();
        UASSERT_OBJ(funcp->isLoose(), nodep, "Cannot take address of non-loose method");
        putns(nodep, "&");
        माला_दो(funcNameProtect(funcp));
    }
    शून्य visit(AstConst* nodep) override {  //
        जर (m_wideTempRefp && nodep->isWide()) {
            UASSERT_OBJ(m_wideTempRefp, nodep, "Wide Constant w/ no temp");
            emitConstantW(nodep, m_wideTempRefp);
            m_wideTempRefp = nullptr;  // We used it, fail if set it a second time
        } अन्यथा {
            emitConstant(nodep);
        }
    }
    शून्य visit(Ast_गाठ_Cons* nodep) override { emitValCons(nodep); }
    शून्य visit(AstThisRef* nodep) override {
        putnbs(nodep, nodep->dtypep()->cType("", false, false));
        माला_दो("{");
        माला_दो(VSelfPointerText::replaceThis(m_useSelfForThis, "this"));
        माला_दो("}");
    }
    शून्य visit(Ast_गाठ_Sel* nodep) override {
        जर (!VN_IS(nodep, ArraySel) && !VN_IS(nodep, WordSel)) {
            visit(static_cast<Ast_गाठ_Biop*>(nodep));
            परत;
        }
        // ArraySel or WordSel
        iterateAndNextConstNull(nodep->fromp());
        // Special case constant index for readability
        जर (AstConst* स्थिरांक idxp = VN_CAST(nodep->bitp(), Const)) {
            माला_दो("[" + std::to_string(idxp->toUInt()) + "U]");
            परत;
        }
        putbs("[");
        iterateAndNextConstNull(nodep->bitp());
        माला_दो("]");
    }

    //
    शून्य visit(AstExecGraph* nodep) override {
        // The location of the AstExecGraph within the containing AstCFunc is where we want to
        // invoke the graph and wait for it to complete. The run-time library does just that.
        putns(nodep, "vl_invokeExecGraph(" + EmitCUtil::execGraphName(nodep) + ", vlSelf, ");
        iterateConst(nodep->triggerp());
        माला_दो(".data());\n");
    }

    // Default
    शून्य visit(Ast_गाठ* nodep) override {  // LCOV_EXCL_START
        putns(nodep, "\n???? // "s + nodep->prettyTypeName() + "\n");
        iterateChildrenConst(nodep);
        जर (!v3Global.opt.lintOnly()) {  // An internal problem, so suppress
            nodep->v3fatalSrc("Unknown node type reached emitter: " << nodep->prettyTypeName());
        }
    }  // LCOV_EXCL_STOP

संरक्षित:
    EmitCFunc() = पूर्वनिर्धारित;
    ~EmitCFunc() override = पूर्वनिर्धारित;
};

#endif  // guard
