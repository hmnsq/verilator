// -*- mode: C++; c-file-style: "cc-mode" -*-
//*************************************************************************
// DESCRIPTION: Verilator: Rename scope references to module-local references
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
// DESCOPE TRANSFORMATIONS:
//      All modules:
//          Each VARREF/FUNCCALL
//              Change varref name() to be relative to current module
//              Remove varScopep()
//          This allows for better V3Combine'ing.
//
//*************************************************************************

#ಅನ್ತರ್ಭಾವ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ಅನ್ತರ್ಭಾವ "V3Descope.h"

#ಅನ್ತರ್ಭಾವ "V3ConstPool.h"
#ಅನ್ತರ್ಭಾವ "V3EmitCBase.h"

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################

ವರ್ಗ DescopeVisitor final : ಸಾರ್ವಜನಿಕ VNVisitor {
    // NODE STATE
    //  Cleared entire netlist
    //   AstCFunc::user()               // bool.  Indicates processing completed
    ಸ್ಥಿರಾಂಕ VNUser1_ಒಳ_Use m_inuser1;

    // TYPES
    ಬಳಸಿ FuncMmap = std::multimap<std::string, AstCFunc*>;
    ರಚನೆ ScopeSelfPtr final {
        VSelf_ಸೂಚಕ_ಪಠ್ಯ thisPtr = VSelf_ಸೂಚಕ_ಪಠ್ಯ{VSelf_ಸೂಚಕ_ಪಠ್ಯ::ಖಾಲಿಗಳು_2()};
        VSelf_ಸೂಚಕ_ಪಠ್ಯ vlSymsPtr = VSelf_ಸೂಚಕ_ಪಠ್ಯ{VSelf_ಸೂಚಕ_ಪಠ್ಯ::ಖಾಲಿಗಳು_2()};
    };

    // STATE
    Ast_ಗಂಟು_Module* m_modp = ಶೂನ್ಯಸೂಚಕ;  // Current module
    AstScope* m_scopep = ಶೂನ್ಯಸೂಚಕ;  // Current scope
    ಸ್ಥಿರಾಂಕ AstCFunc* m_funcp = ಶೂನ್ಯಸೂಚಕ;  // Current function
    ತಾರ್ಕಿಕ m_modSingleton = ಸುಳ್ಳು;  // m_modp is only instantiated once
    FuncMmap m_modFuncs;  // Name of public functions added
    std::map<ಸ್ಥಿರಾಂಕ AstScope*, ScopeSelfPtr> m_scope_ಗಮ್ಯ_Self;  // Scope to self pointers

    // METHODS

    ಸ್ಥಿರ ತಾರ್ಕಿಕ modIsSingleton(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_Module* modp) {
        // True iff there's exactly one instance of this module in the design (including top).
        ಆದರೆ (modp->is_ತುದಿ()) ಹಿಂತಿರುಗಿ ನಿಜ;
        ಪೂರ್ಣಾಂಕ instances = 0;
        ಗಾಗಿ (ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* stmtp = modp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            ಆದರೆ (VN_IS(stmtp, Scope)) {
                ಆದರೆ (++instances > 1) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
            }
        }
        ಹಿಂತಿರುಗಿ (instances == 1);
    }

    // Construct a 'this' self pointer for the given scope
    VSelf_ಸೂಚಕ_ಪಠ್ಯ scopeThis(ಸ್ಥಿರಾಂಕ AstScope* scopep) {
        ಸ್ವಯಂಚಾಲಿತ& ret = m_scope_ಗಮ್ಯ_Self[scopep];
        ಆದರೆ (ret.thisPtr.is_ಖಾಲಿಗಳು()) {
            string name = scopep->name();
            string::ಗಾತ್ರ_type pos = name.rfind('.');
            ಆದರೆ (pos != string::npos) name.erase(0, pos + 1);
            ret.thisPtr = VSelf_ಸೂಚಕ_ಪಠ್ಯ{VSelf_ಸೂಚಕ_ಪಠ್ಯ::This{}, name};
        }
        ಹಿಂತಿರುಗಿ ret.thisPtr;
    }
    // Construct a 'vlSyms' self pointer for the given scope
    VSelf_ಸೂಚಕ_ಪಠ್ಯ scopeVlSyms(ಸ್ಥಿರಾಂಕ AstScope* scopep) {
        ಸ್ವಯಂಚಾಲಿತ& ret = m_scope_ಗಮ್ಯ_Self[scopep];
        ಆದರೆ (ret.vlSymsPtr.is_ಖಾಲಿಗಳು()) {
            ret.vlSymsPtr = VSelf_ಸೂಚಕ_ಪಠ್ಯ{VSelf_ಸೂಚಕ_ಪಠ್ಯ::VlSyms(), scopep->nameDotless()};
        }
        ಹಿಂತಿರುಗಿ ret.vlSymsPtr;
    }

    // Construct the best self pointer to reference an object in 'scopep'  from a CFunc in
    // 'm_scopep'. Result may be relative ("this->[...]") or absolute ("vlSyms->[...]").
    //
    // Using relative references allows V3Combine'ing code across multiple instances of the same
    // module.
    VSelf_ಸೂಚಕ_ಪಠ್ಯ descoped_Self_ಸೂಚಕ(ಸ್ಥಿರಾಂಕ AstScope* scopep) {
        UASSERT(scopep, "Var/Func not scoped");
        // Static functions can't use relative references via 'this->'
        ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ relativeRefOk = !m_funcp->isStatic();

        UINFO(8, "      Descope ref under " << m_scopep);
        UINFO(8, "              ref to    " << scopep);
        UINFO(8, "             aboveScope " << scopep->aboveScopep());

        ಆದರೆ (VN_IS(scopep->modp(), Class)) {
            // Direct reference to class members are from within the class itself, references from
            // outside the class must go via AstMemberSel
            ಹಿಂತಿರುಗಿ VSelf_ಸೂಚಕ_ಪಠ್ಯ{VSelf_ಸೂಚಕ_ಪಠ್ಯ::This{}};
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (relativeRefOk && scopep == m_scopep) {
            ಹಿಂತಿರುಗಿ VSelf_ಸೂಚಕ_ಪಠ್ಯ{VSelf_ಸೂಚಕ_ಪಠ್ಯ::This{}};
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (relativeRefOk && !m_modSingleton && scopep->aboveScopep() == m_scopep
                   && VN_IS(scopep->modp(), Module)) {
            // Reference to scope of instance directly under this module, can just "this->cell",
            // which can potentially be V3Combined, but note this requires one extra pointer
            // dereference which is slower, so we only use it if the source scope is not a
            // singleton.
            ಹಿಂತಿರುಗಿ scopeThis(scopep);
        } ಇಲ್ಲದಿದ್ದರೆ {
            // Reference to something elsewhere, or relative references are disabled. Use global
            // variable
            ಹಿಂತಿರುಗಿ scopeVlSyms(scopep);
        }
    }

    ಶೂನ್ಯ make_ಬಹಿರಂಗ_Func_Wrappers() {
        // We recorded all public functions in m_modFuncs.
        // If for any given name only one function exists, we can use that function directly.
        // If multiple functions exist, we need to select the appropriate scope.
        ಗಾಗಿ (FuncMmap::iterator it = m_modFuncs.begin(); it != m_modFuncs.ಕೊನೆ(); ++it) {
            ಸ್ಥಿರಾಂಕ string name = it->ಮೊದಲ;
            AstCFunc* ಸ್ಥಿರಾಂಕ ತುದಿ_Funcp = it->ಎರಡನೇ;
            ಸ್ವಯಂಚಾಲಿತ ಮುಂದಿನ_It1 = it;
            ++ಮುಂದಿನ_It1;
            ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ ಇನ್ನಷ್ಟು_ದ_Same1 = (ಮುಂದಿನ_It1 != m_modFuncs.ಕೊನೆ() && ಮುಂದಿನ_It1->ಮೊದಲ == name);
            ಆದರೆ (ಇನ್ನಷ್ಟು_ದ_Same1) {
                // Multiple functions under this name, need a wrapper function
                UINFO(6, "  Wrapping " << name << " multifuncs");
                AstCFunc* ಸ್ಥಿರಾಂಕ newfuncp = ತುದಿ_Funcp->clone_ವೃಕ್ಷ(ಸುಳ್ಳು);
                ಆದರೆ (newfuncp->varsp()) newfuncp->varsp()->unlink_Fr_ಹಿಂಭಾಗ_With_ಮುಂದಿನ()->delete_ವೃಕ್ಷ();
                ಆದರೆ (newfuncp->stmtsp()) newfuncp->stmtsp()->unlink_Fr_ಹಿಂಭಾಗ_With_ಮುಂದಿನ()->delete_ವೃಕ್ಷ();
                newfuncp->name(name);
                newfuncp->isStatic(ಸುಳ್ಳು);
                ತುದಿ_Funcp->ಕೂಡಿಸು_ಮುಂದಿನ_ಇಲ್ಲಿ(newfuncp);
                // In the body, call each function if it matches the given scope
                ಗಾಗಿ (FuncMmap::iterator eachIt = it;
                     eachIt != m_modFuncs.ಕೊನೆ() && eachIt->ಮೊದಲ == name; ++eachIt) {
                    it = eachIt;
                    AstCFunc* ಸ್ಥಿರಾಂಕ funcp = eachIt->ಎರಡನೇ;
                    ಸ್ವಯಂಚಾಲಿತ ಮುಂದಿನ_It2 = eachIt;
                    ++ಮುಂದಿನ_It2;
                    ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ ಇನ್ನಷ್ಟು_ದ_Same
                        = (ಮುಂದಿನ_It2 != m_modFuncs.ಕೊನೆ() && ಮುಂದಿನ_It2->ಮೊದಲ == name);
                    UASSERT_OBJ(funcp->scopep(), funcp, "Not scoped");

                    UINFO(6, "     Wrapping " << name << " " << funcp);
                    UINFO(6, "  at " << newfuncp->argTypes() << " und " << funcp->argTypes());
                    funcp->decl_ಖಾಸಗಿ(ನಿಜ);
                    AstVarRef* argsp = ಶೂನ್ಯಸೂಚಕ;
                    ಗಾಗಿ (Ast_ಗಂಟು* stmtp = newfuncp->argsp(); stmtp; stmtp = stmtp->nextp()) {
                        ಆದರೆ (AstVar* ಸ್ಥಿರಾಂಕ portp = VN_CAST(stmtp, Var)) {
                            ಆದರೆ (portp->isIO() && !portp->isFuncReturn()) {
                                AstVarRef* ಸ್ಥಿರಾಂಕ newp = ಹೊಸ AstVarRef{
                                    portp->fileline(), portp, portp->direction().pinAccess()};
                                argsp = Ast_ಗಂಟು::ಕೂಡಿಸು_ಮುಂದಿನ(argsp, newp);
                            }
                        }
                    }

                    AstCCall* ಸ್ಥಿರಾಂಕ callp = ಹೊಸ AstCCall{funcp->fileline(), funcp, argsp};
                    ಆದರೆ (Ast_ಗಂಟು_DType* ಸ್ಥಿರಾಂಕ dtypep = funcp->dtypep()) {
                        callp->dtypep(dtypep);
                    } ಇಲ್ಲದಿದ್ದರೆ {
                        callp->dtype_ಗಣ_Void();
                    }
                    Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ returnp = ಹೊಸ AstCReturn{funcp->fileline(), callp};

                    ಆದರೆ (ಇನ್ನಷ್ಟು_ದ_Same) {
                        AstIf* ಸ್ಥಿರಾಂಕ ifp = ಹೊಸ AstIf{
                            funcp->fileline(),
                            ಹೊಸ AstEq{funcp->fileline(),
                                      ಹೊಸ AstCExpr{funcp->fileline(), "this", 64},
                                      ಹೊಸ AstCExpr{funcp->fileline(),
                                                   scopeVlSyms(funcp->scopep()).asString(), 64}},
                            returnp};
                        newfuncp->ಕೂಡಿಸು_Stmtsp(ifp);
                    } ಇಲ್ಲದಿದ್ದರೆ {
                        newfuncp->ಕೂಡಿಸು_Stmtsp(returnp);
                    }
                }
                // Not really any way the user could do this, and we'd need
                // to come up with some return value
                // newfuncp->addStmtsp(new AstDisplay{newfuncp->fileline(),
                //                                   VDisplayType::DT_WARNING,
                //                                   "%%Error: "s+name+"() called with bad
                //                                   scope", nullptr});
                // newfuncp->addStmtsp(new AstStop{newfuncp->fileline()});
                UINFOTREE(9, newfuncp, "", "newfunc");
            } ಇಲ್ಲದಿದ್ದರೆ {
                // Only a single function under this name, we can rename it
                UINFO(6, "  Wrapping " << name << " just one " << ತುದಿ_Funcp);
                ತುದಿ_Funcp->name(name);
            }
        }
    }

    // VISITORS
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು_Module* nodep) override {
        VL_RESTORER(m_modp);
        m_modp = nodep;
        m_modFuncs.clear();
        m_modSingleton = modIsSingleton(m_modp);
        iterateChildren(nodep);
        make_ಬಹಿರಂಗ_Func_Wrappers();
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstScope* nodep) override {
        VL_RESTORER(m_scopep);
        m_scopep = nodep;
        iterateChildren(nodep);
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstVarScope* nodep) override {
        // Delete the varscope when we're finished
        VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep->unlink_Fr_ಹಿಂಭಾಗ()), nodep);
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು_Var_Ref* nodep) override {
        iterateChildren(nodep);
        ಆದರೆ (!nodep->varScopep()) {
            UASSERT_OBJ(nodep->varp()->isFuncLocal(), nodep,
                        "unscoped reference can only appear to function locals at this point");
            ಹಿಂತಿರುಗಿ;
        }
        // Convert the hierch name
        UINFO(9, "  ref-in " << nodep);
        UASSERT_OBJ(m_scopep, nodep, "Node not under scope");
        ಸ್ಥಿರಾಂಕ AstVar* ಸ್ಥಿರಾಂಕ varp = nodep->varScopep()->varp();
        ಸ್ಥಿರಾಂಕ AstScope* ಸ್ಥಿರಾಂಕ scopep = nodep->varScopep()->scopep();
        ಆದರೆ (varp->isFuncLocal()) {
            // Reference to function locals need no self pointer
            nodep->self_ಸೂಚಕ(VSelf_ಸೂಚಕ_ಪಠ್ಯ{VSelf_ಸೂಚಕ_ಪಠ್ಯ::ಖಾಲಿಗಳು_2()});
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (scopep->modp()->isConstPool()) {
            // Reference to constant pool value need no self pointer
            nodep->self_ಸೂಚಕ(VSelf_ಸೂಚಕ_ಪಠ್ಯ{VSelf_ಸೂಚಕ_ಪಠ್ಯ::ಖಾಲಿಗಳು_2()});
        } ಇಲ್ಲದಿದ್ದರೆ {
            nodep->self_ಸೂಚಕ(descoped_Self_ಸೂಚಕ(scopep));
        }
        nodep->varScopep(ಶೂನ್ಯಸೂಚಕ);
        UINFO(9, "  refout " << nodep << " selfPtr=" << nodep->self_ಸೂಚಕ().asString());
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstCCall* nodep) override {
        // UINFO(9, "       " << nodep);
        iterateChildren(nodep);
        // Convert the hierch name
        UASSERT_OBJ(m_scopep, nodep, "Node not under scope");
        ಸ್ಥಿರಾಂಕ AstScope* ಸ್ಥಿರಾಂಕ scopep = nodep->funcp()->scopep();
        nodep->self_ಸೂಚಕ(descoped_Self_ಸೂಚಕ(scopep));
        // Can't do this, as we may have more calls later
        // nodep->funcp()->scopep(nullptr);
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstScopeName* nodep) override {
        // Print a %m under this instance as its vlNamep, which holds its pretty name, plus the
        // rest of the path, so the instances' code can be V3Combined. Not with --protect-ids, as
        // vlNamep then holds the protected name, while %m prints the original.
        ಆದರೆ (!m_funcp || !nodep->forFormat() || m_modSingleton || m_funcp->isStatic()
            || v3Global.opt.protectIds()) {
            ಹಿಂತಿರುಗಿ;
        }
        std::string ಮಾರ್ಗ = nodep->scopePrettySymName();
        ಸ್ಥಿರಾಂಕ std::string inst = m_scopep->prettyName();
        ಆದರೆ (ಮಾರ್ಗ != inst && !VString::startsWith(ಮಾರ್ಗ, inst + ".")) ಹಿಂತಿರುಗಿ;
        ಮಾರ್ಗ.erase(0, inst.ಗಾತ್ರ_2() + 1);
        // The DPI scope table keeps the absolute name, via a copy outside any function
        m_scopep->ಕೂಡಿಸು_Blocksp(nodep->clone_ವೃಕ್ಷ(ಸುಳ್ಳು));
        nodep->scopeAttr(ಮಾರ್ಗ);
        nodep->scopeEntr("");  // Only used by the DPI scope table
        nodep->selfRelative(ನಿಜ);
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstCMethodCall* nodep) override { iterateChildren(nodep); }
    ಶೂನ್ಯ ಭೇಟಿ(AstCNew* nodep) override { iterateChildren(nodep); }
    ಶೂನ್ಯ ಭೇಟಿ(AstCFunc* nodep) override {
        VL_RESTORER(m_funcp);
        ಆದರೆ (!nodep->user1()) {
            // Static functions should have been moved under the corresponding AstClassPackage
            UASSERT_OBJ(!(nodep->isStatic() && VN_IS(m_modp, Class)), nodep,
                        "Static function under AstClass");
            m_funcp = nodep;
            iterateChildren(nodep);
            nodep->user1(ನಿಜ);
            // If it's under a scope, move it up to the top
            ಆದರೆ (m_scopep) {
                nodep->unlink_Fr_ಹಿಂಭಾಗ();
                m_modp->ಕೂಡಿಸು_Stmtsp(nodep);

                ಆದರೆ (nodep->func_ಬಹಿರಂಗ()) {
                    // There may be multiple public functions by the same name;
                    // record for later correction or making of shells
                    m_modFuncs.emplace(nodep->name(), nodep);
                    nodep->name(m_scopep->nameDotless() + "__" + nodep->name());
                }
            }
        }
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstVar*) override {}
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು* nodep) override { iterateChildren(nodep); }

ಸಾರ್ವಜನಿಕ:
    // CONSTRUCTORS
    ಸ್ಪಷ್ಟ DescopeVisitor(AstNetlist* nodep) { iterate(nodep); }
    ~DescopeVisitor() override = ಪೂರ್ವನಿಯೋಜಿತ;
};

//######################################################################
// Descope class functions

ಶೂನ್ಯ V3Descope::descopeAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { DescopeVisitor{nodep}; }  // Destruct before checking
    V3ConstPool::ಗಣ_Descoped();
    V3Global::dump_Check_Global_ವೃಕ್ಷ("descope", 0, dump_ವೃಕ್ಷ_Either_Level() >= 3);
}
