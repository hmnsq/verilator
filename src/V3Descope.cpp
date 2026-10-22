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
    ಧ್ರುವ VNUser1InUse m_inuser1;

    // TYPES
    using FuncMmap = std::multimap<std::string, AstCFunc*>;
    ಸಂರಚನಾ ScopeSelfPtr final {
        VSelfPointerText thisPtr = VSelfPointerText{VSelfPointerText::Empty()};
        VSelfPointerText vlSymsPtr = VSelfPointerText{VSelfPointerText::Empty()};
    };

    // STATE
    Ast_ಗಂಟು_Module* m_modp = nullptr;  // Current module
    AstScope* m_scopep = nullptr;  // Current scope
    ಧ್ರುವ AstCFunc* m_funcp = nullptr;  // Current function
    ತಾರ್ಕಿಕ m_modSingleton = ಅಸತ್ಯ;  // m_modp is only instantiated once
    FuncMmap m_modFuncs;  // Name of public functions added
    std::map<ಧ್ರುವ AstScope*, ScopeSelfPtr> m_scopeToSelf;  // Scope to self pointers

    // METHODS

    ಸ್ಥಿರ ತಾರ್ಕಿಕ modIsSingleton(ಧ್ರುವ Ast_ಗಂಟು_Module* modp) {
        // True iff there's exactly one instance of this module in the design (including top).
        ಯದಿ (modp->is_ತುದಿ()) ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;
        ಪೂರ್ಣಾಙ್ಕ instances = 0;
        ಪ್ರತಿ (ಧ್ರುವ Ast_ಗಂಟು* stmtp = modp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            ಯದಿ (VN_IS(stmtp, Scope)) {
                ಯದಿ (++instances > 1) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
            }
        }
        ಪ್ರತ್ಯಾಗಮ (instances == 1);
    }

    // Construct a 'this' self pointer for the given scope
    VSelfPointerText scopeThis(ಧ್ರುವ AstScope* scopep) {
        auto& ret = m_scopeToSelf[scopep];
        ಯದಿ (ret.thisPtr.isEmpty()) {
            string name = scopep->name();
            string::size_type pos = name.rfind('.');
            ಯದಿ (pos != string::npos) name.erase(0, pos + 1);
            ret.thisPtr = VSelfPointerText{VSelfPointerText::This{}, name};
        }
        ಪ್ರತ್ಯಾಗಮ ret.thisPtr;
    }
    // Construct a 'vlSyms' self pointer for the given scope
    VSelfPointerText scopeVlSyms(ಧ್ರುವ AstScope* scopep) {
        auto& ret = m_scopeToSelf[scopep];
        ಯದಿ (ret.vlSymsPtr.isEmpty()) {
            ret.vlSymsPtr = VSelfPointerText{VSelfPointerText::VlSyms(), scopep->nameDotless()};
        }
        ಪ್ರತ್ಯಾಗಮ ret.vlSymsPtr;
    }

    // Construct the best self pointer to reference an object in 'scopep'  from a CFunc in
    // 'm_scopep'. Result may be relative ("this->[...]") or absolute ("vlSyms->[...]").
    //
    // Using relative references allows V3Combine'ing code across multiple instances of the same
    // module.
    VSelfPointerText descopedSelfPointer(ಧ್ರುವ AstScope* scopep) {
        UASSERT(scopep, "Var/Func not scoped");
        // Static functions can't use relative references via 'this->'
        ಧ್ರುವ ತಾರ್ಕಿಕ relativeRefOk = !m_funcp->isStatic();

        UINFO(8, "      Descope ref under " << m_scopep);
        UINFO(8, "              ref to    " << scopep);
        UINFO(8, "             aboveScope " << scopep->aboveScopep());

        ಯದಿ (VN_IS(scopep->modp(), Class)) {
            // Direct reference to class members are from within the class itself, references from
            // outside the class must go via AstMemberSel
            ಪ್ರತ್ಯಾಗಮ VSelfPointerText{VSelfPointerText::This{}};
        } ಅನ್ಯಥಾ ಯದಿ (relativeRefOk && scopep == m_scopep) {
            ಪ್ರತ್ಯಾಗಮ VSelfPointerText{VSelfPointerText::This{}};
        } ಅನ್ಯಥಾ ಯದಿ (relativeRefOk && !m_modSingleton && scopep->aboveScopep() == m_scopep
                   && VN_IS(scopep->modp(), Module)) {
            // Reference to scope of instance directly under this module, can just "this->cell",
            // which can potentially be V3Combined, but note this requires one extra pointer
            // dereference which is slower, so we only use it if the source scope is not a
            // singleton.
            ಪ್ರತ್ಯಾಗಮ scopeThis(scopep);
        } ಅನ್ಯಥಾ {
            // Reference to something elsewhere, or relative references are disabled. Use global
            // variable
            ಪ್ರತ್ಯಾಗಮ scopeVlSyms(scopep);
        }
    }

    ಶೂನ್ಯ makePublicFuncWrappers() {
        // We recorded all public functions in m_modFuncs.
        // If for any given name only one function exists, we can use that function directly.
        // If multiple functions exist, we need to select the appropriate scope.
        ಪ್ರತಿ (FuncMmap::iterator it = m_modFuncs.begin(); it != m_modFuncs.end(); ++it) {
            ಧ್ರುವ string name = it->first;
            AstCFunc* ಧ್ರುವ ತುದಿ_Funcp = it->second;
            auto nextIt1 = it;
            ++nextIt1;
            ಧ್ರುವ ತಾರ್ಕಿಕ moreOfSame1 = (nextIt1 != m_modFuncs.end() && nextIt1->first == name);
            ಯದಿ (moreOfSame1) {
                // Multiple functions under this name, need a wrapper function
                UINFO(6, "  Wrapping " << name << " multifuncs");
                AstCFunc* ಧ್ರುವ newfuncp = ತುದಿ_Funcp->clone_ವೃಕ್ಷ(ಅಸತ್ಯ);
                ಯದಿ (newfuncp->varsp()) newfuncp->varsp()->unlinkFrBackWithNext()->delete_ವೃಕ್ಷ();
                ಯದಿ (newfuncp->stmtsp()) newfuncp->stmtsp()->unlinkFrBackWithNext()->delete_ವೃಕ್ಷ();
                newfuncp->name(name);
                newfuncp->isStatic(ಅಸತ್ಯ);
                ತುದಿ_Funcp->addNextHere(newfuncp);
                // In the body, call each function if it matches the given scope
                ಪ್ರತಿ (FuncMmap::iterator eachIt = it;
                     eachIt != m_modFuncs.end() && eachIt->first == name; ++eachIt) {
                    it = eachIt;
                    AstCFunc* ಧ್ರುವ funcp = eachIt->second;
                    auto nextIt2 = eachIt;
                    ++nextIt2;
                    ಧ್ರುವ ತಾರ್ಕಿಕ moreOfSame
                        = (nextIt2 != m_modFuncs.end() && nextIt2->first == name);
                    UASSERT_OBJ(funcp->scopep(), funcp, "Not scoped");

                    UINFO(6, "     Wrapping " << name << " " << funcp);
                    UINFO(6, "  at " << newfuncp->argTypes() << " und " << funcp->argTypes());
                    funcp->declPrivate(ಸತ್ಯ);
                    AstVarRef* argsp = nullptr;
                    ಪ್ರತಿ (Ast_ಗಂಟು* stmtp = newfuncp->argsp(); stmtp; stmtp = stmtp->nextp()) {
                        ಯದಿ (AstVar* ಧ್ರುವ portp = VN_CAST(stmtp, Var)) {
                            ಯದಿ (portp->isIO() && !portp->isFuncReturn()) {
                                AstVarRef* ಧ್ರುವ newp = ನೂತನ AstVarRef{
                                    portp->fileline(), portp, portp->direction().pinAccess()};
                                argsp = Ast_ಗಂಟು::addNext(argsp, newp);
                            }
                        }
                    }

                    AstCCall* ಧ್ರುವ callp = ನೂತನ AstCCall{funcp->fileline(), funcp, argsp};
                    ಯದಿ (Ast_ಗಂಟು_DType* ಧ್ರುವ dtypep = funcp->dtypep()) {
                        callp->dtypep(dtypep);
                    } ಅನ್ಯಥಾ {
                        callp->dtypeSetVoid();
                    }
                    Ast_ಗಂಟು* ಧ್ರುವ returnp = ನೂತನ AstCReturn{funcp->fileline(), callp};

                    ಯದಿ (moreOfSame) {
                        AstIf* ಧ್ರುವ ifp = ನೂತನ AstIf{
                            funcp->fileline(),
                            ನೂತನ AstEq{funcp->fileline(),
                                      ನೂತನ AstCExpr{funcp->fileline(), "this", 64},
                                      ನೂತನ AstCExpr{funcp->fileline(),
                                                   scopeVlSyms(funcp->scopep()).asString(), 64}},
                            returnp};
                        newfuncp->addStmtsp(ifp);
                    } ಅನ್ಯಥಾ {
                        newfuncp->addStmtsp(returnp);
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
            } ಅನ್ಯಥಾ {
                // Only a single function under this name, we can rename it
                UINFO(6, "  Wrapping " << name << " just one " << ತುದಿ_Funcp);
                ತುದಿ_Funcp->name(name);
            }
        }
    }

    // VISITORS
    ಶೂನ್ಯ visit(Ast_ಗಂಟು_Module* nodep) override {
        VL_RESTORER(m_modp);
        m_modp = nodep;
        m_modFuncs.clear();
        m_modSingleton = modIsSingleton(m_modp);
        iterateChildren(nodep);
        makePublicFuncWrappers();
    }
    ಶೂನ್ಯ visit(AstScope* nodep) override {
        VL_RESTORER(m_scopep);
        m_scopep = nodep;
        iterateChildren(nodep);
    }
    ಶೂನ್ಯ visit(AstVarScope* nodep) override {
        // Delete the varscope when we're finished
        VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep->unlinkFrBack()), nodep);
    }
    ಶೂನ್ಯ visit(Ast_ಗಂಟು_Var_Ref* nodep) override {
        iterateChildren(nodep);
        ಯದಿ (!nodep->varScopep()) {
            UASSERT_OBJ(nodep->varp()->isFuncLocal(), nodep,
                        "unscoped reference can only appear to function locals at this point");
            ಪ್ರತ್ಯಾಗಮ;
        }
        // Convert the hierch name
        UINFO(9, "  ref-in " << nodep);
        UASSERT_OBJ(m_scopep, nodep, "Node not under scope");
        ಧ್ರುವ AstVar* ಧ್ರುವ varp = nodep->varScopep()->varp();
        ಧ್ರುವ AstScope* ಧ್ರುವ scopep = nodep->varScopep()->scopep();
        ಯದಿ (varp->isFuncLocal()) {
            // Reference to function locals need no self pointer
            nodep->selfPointer(VSelfPointerText{VSelfPointerText::Empty()});
        } ಅನ್ಯಥಾ ಯದಿ (scopep->modp()->isConstPool()) {
            // Reference to constant pool value need no self pointer
            nodep->selfPointer(VSelfPointerText{VSelfPointerText::Empty()});
        } ಅನ್ಯಥಾ {
            nodep->selfPointer(descopedSelfPointer(scopep));
        }
        nodep->varScopep(nullptr);
        UINFO(9, "  refout " << nodep << " selfPtr=" << nodep->selfPointer().asString());
    }
    ಶೂನ್ಯ visit(AstCCall* nodep) override {
        // UINFO(9, "       " << nodep);
        iterateChildren(nodep);
        // Convert the hierch name
        UASSERT_OBJ(m_scopep, nodep, "Node not under scope");
        ಧ್ರುವ AstScope* ಧ್ರುವ scopep = nodep->funcp()->scopep();
        nodep->selfPointer(descopedSelfPointer(scopep));
        // Can't do this, as we may have more calls later
        // nodep->funcp()->scopep(nullptr);
    }
    ಶೂನ್ಯ visit(AstScopeName* nodep) override {
        // Print a %m under this instance as its vlNamep, which holds its pretty name, plus the
        // rest of the path, so the instances' code can be V3Combined. Not with --protect-ids, as
        // vlNamep then holds the protected name, while %m prints the original.
        ಯದಿ (!m_funcp || !nodep->forFormat() || m_modSingleton || m_funcp->isStatic()
            || v3Global.opt.protectIds()) {
            ಪ್ರತ್ಯಾಗಮ;
        }
        std::string path = nodep->scopePrettySymName();
        ಧ್ರುವ std::string inst = m_scopep->prettyName();
        ಯದಿ (path != inst && !VString::startsWith(path, inst + ".")) ಪ್ರತ್ಯಾಗಮ;
        path.erase(0, inst.size() + 1);
        // The DPI scope table keeps the absolute name, via a copy outside any function
        m_scopep->addBlocksp(nodep->clone_ವೃಕ್ಷ(ಅಸತ್ಯ));
        nodep->scopeAttr(path);
        nodep->scopeEntr("");  // Only used by the DPI scope table
        nodep->selfRelative(ಸತ್ಯ);
    }
    ಶೂನ್ಯ visit(AstCMethodCall* nodep) override { iterateChildren(nodep); }
    ಶೂನ್ಯ visit(AstCNew* nodep) override { iterateChildren(nodep); }
    ಶೂನ್ಯ visit(AstCFunc* nodep) override {
        VL_RESTORER(m_funcp);
        ಯದಿ (!nodep->user1()) {
            // Static functions should have been moved under the corresponding AstClassPackage
            UASSERT_OBJ(!(nodep->isStatic() && VN_IS(m_modp, Class)), nodep,
                        "Static function under AstClass");
            m_funcp = nodep;
            iterateChildren(nodep);
            nodep->user1(ಸತ್ಯ);
            // If it's under a scope, move it up to the top
            ಯದಿ (m_scopep) {
                nodep->unlinkFrBack();
                m_modp->addStmtsp(nodep);

                ಯದಿ (nodep->funcPublic()) {
                    // There may be multiple public functions by the same name;
                    // record for later correction or making of shells
                    m_modFuncs.emplace(nodep->name(), nodep);
                    nodep->name(m_scopep->nameDotless() + "__" + nodep->name());
                }
            }
        }
    }
    ಶೂನ್ಯ visit(AstVar*) override {}
    ಶೂನ್ಯ visit(Ast_ಗಂಟು* nodep) override { iterateChildren(nodep); }

ಸಾರ್ವಜನಿಕ:
    // CONSTRUCTORS
    explicit DescopeVisitor(AstNetlist* nodep) { iterate(nodep); }
    ~DescopeVisitor() override = ಸ್ವಭಾವ;
};

//######################################################################
// Descope class functions

ಶೂನ್ಯ V3Descope::descopeAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { DescopeVisitor{nodep}; }  // Destruct before checking
    V3ConstPool::setDescoped();
    V3Global::dump_Check_Global_ವೃಕ್ಷ("descope", 0, dump_ವೃಕ್ಷ_Either_Level() >= 3);
}
