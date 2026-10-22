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

#ਅਨ੍ਤਰ੍ਭਾਵ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ਅਨ੍ਤਰ੍ਭਾਵ "V3Descope.h"

#ਅਨ੍ਤਰ੍ਭਾਵ "V3ConstPool.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3EmitCBase.h"

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################

ਵਰਗ DescopeVisitor final : ਜਨਤਕ VNVisitor {
    // NODE STATE
    //  Cleared entire netlist
    //   AstCFunc::user()               // bool.  Indicates processing completed
    ਅਚਲ VNUser1_ਵਿੱਚ_Use m_inuser1;

    // TYPES
    ਵਰਤੋਂ FuncMmap = std::multimap<std::string, AstCFunc*>;
    ਬਣਤਰ ScopeSelfPtr final {
        VSelf_ਸੂਚਕ_ਪਾਠ thisPtr = VSelf_ਸੂਚਕ_ਪਾਠ{VSelf_ਸੂਚਕ_ਪਾਠ::ਖਾਲੀ_ਥਾਵਾਂ_2()};
        VSelf_ਸੂਚਕ_ਪਾਠ vlSymsPtr = VSelf_ਸੂਚਕ_ਪਾਠ{VSelf_ਸੂਚਕ_ਪਾਠ::ਖਾਲੀ_ਥਾਵਾਂ_2()};
    };

    // STATE
    Ast_ਗੰਢ_Module* m_modp = ਖਾਲੀ_ਸੂਚਕ;  // Current module
    AstScope* m_scopep = ਖਾਲੀ_ਸੂਚਕ;  // Current scope
    ਅਚਲ AstCFunc* m_funcp = ਖਾਲੀ_ਸੂਚਕ;  // Current function
    ਤਾਰਕਿਕ m_modSingleton = ਝੂਠ;  // m_modp is only instantiated once
    FuncMmap m_modFuncs;  // Name of public functions added
    std::map<ਅਚਲ AstScope*, ScopeSelfPtr> m_scope_ਮੰਜ਼ਿਲ_Self;  // Scope to self pointers

    // METHODS

    ਸਥਿਰ ਤਾਰਕਿਕ modIsSingleton(ਅਚਲ Ast_ਗੰਢ_Module* modp) {
        // True iff there's exactly one instance of this module in the design (including top).
        ਜੇ (modp->is_ਸਿਖਰ()) ਵਾਪਸ ਸੱਚ;
        ਪੂਰਨ instances = 0;
        ਲਈ (ਅਚਲ Ast_ਗੰਢ* stmtp = modp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            ਜੇ (VN_IS(stmtp, Scope)) {
                ਜੇ (++instances > 1) ਵਾਪਸ ਝੂਠ;
            }
        }
        ਵਾਪਸ (instances == 1);
    }

    // Construct a 'this' self pointer for the given scope
    VSelf_ਸੂਚਕ_ਪਾਠ scopeThis(ਅਚਲ AstScope* scopep) {
        ਸਵੈਚਾਲਿਤ& ret = m_scope_ਮੰਜ਼ਿਲ_Self[scopep];
        ਜੇ (ret.thisPtr.is_ਖਾਲੀ_ਥਾਵਾਂ()) {
            string name = scopep->name();
            string::ਮਾਪ_type pos = name.rfind('.');
            ਜੇ (pos != string::npos) name.erase(0, pos + 1);
            ret.thisPtr = VSelf_ਸੂਚਕ_ਪਾਠ{VSelf_ਸੂਚਕ_ਪਾਠ::This{}, name};
        }
        ਵਾਪਸ ret.thisPtr;
    }
    // Construct a 'vlSyms' self pointer for the given scope
    VSelf_ਸੂਚਕ_ਪਾਠ scopeVlSyms(ਅਚਲ AstScope* scopep) {
        ਸਵੈਚਾਲਿਤ& ret = m_scope_ਮੰਜ਼ਿਲ_Self[scopep];
        ਜੇ (ret.vlSymsPtr.is_ਖਾਲੀ_ਥਾਵਾਂ()) {
            ret.vlSymsPtr = VSelf_ਸੂਚਕ_ਪਾਠ{VSelf_ਸੂਚਕ_ਪਾਠ::VlSyms(), scopep->nameDotless()};
        }
        ਵਾਪਸ ret.vlSymsPtr;
    }

    // Construct the best self pointer to reference an object in 'scopep'  from a CFunc in
    // 'm_scopep'. Result may be relative ("this->[...]") or absolute ("vlSyms->[...]").
    //
    // Using relative references allows V3Combine'ing code across multiple instances of the same
    // module.
    VSelf_ਸੂਚਕ_ਪਾਠ descoped_Self_ਸੂਚਕ(ਅਚਲ AstScope* scopep) {
        UASSERT(scopep, "Var/Func not scoped");
        // Static functions can't use relative references via 'this->'
        ਅਚਲ ਤਾਰਕਿਕ relativeRefOk = !m_funcp->isStatic();

        UINFO(8, "      Descope ref under " << m_scopep);
        UINFO(8, "              ref to    " << scopep);
        UINFO(8, "             aboveScope " << scopep->aboveScopep());

        ਜੇ (VN_IS(scopep->modp(), Class)) {
            // Direct reference to class members are from within the class itself, references from
            // outside the class must go via AstMemberSel
            ਵਾਪਸ VSelf_ਸੂਚਕ_ਪਾਠ{VSelf_ਸੂਚਕ_ਪਾਠ::This{}};
        } ਨਹੀਂਤਾਂ ਜੇ (relativeRefOk && scopep == m_scopep) {
            ਵਾਪਸ VSelf_ਸੂਚਕ_ਪਾਠ{VSelf_ਸੂਚਕ_ਪਾਠ::This{}};
        } ਨਹੀਂਤਾਂ ਜੇ (relativeRefOk && !m_modSingleton && scopep->aboveScopep() == m_scopep
                   && VN_IS(scopep->modp(), Module)) {
            // Reference to scope of instance directly under this module, can just "this->cell",
            // which can potentially be V3Combined, but note this requires one extra pointer
            // dereference which is slower, so we only use it if the source scope is not a
            // singleton.
            ਵਾਪਸ scopeThis(scopep);
        } ਨਹੀਂਤਾਂ {
            // Reference to something elsewhere, or relative references are disabled. Use global
            // variable
            ਵਾਪਸ scopeVlSyms(scopep);
        }
    }

    ਖਾਲੀ make_ਜਨਤਕ_Func_Wrappers() {
        // We recorded all public functions in m_modFuncs.
        // If for any given name only one function exists, we can use that function directly.
        // If multiple functions exist, we need to select the appropriate scope.
        ਲਈ (FuncMmap::iterator it = m_modFuncs.begin(); it != m_modFuncs.ਸਿਰਾ(); ++it) {
            ਅਚਲ string name = it->ਪਹਿਲਾ;
            AstCFunc* ਅਚਲ ਸਿਖਰ_Funcp = it->ਦੂਜਾ;
            ਸਵੈਚਾਲਿਤ ਅਗਲਾ_It1 = it;
            ++ਅਗਲਾ_It1;
            ਅਚਲ ਤਾਰਕਿਕ ਹੋਰ_ਦਾ_Same1 = (ਅਗਲਾ_It1 != m_modFuncs.ਸਿਰਾ() && ਅਗਲਾ_It1->ਪਹਿਲਾ == name);
            ਜੇ (ਹੋਰ_ਦਾ_Same1) {
                // Multiple functions under this name, need a wrapper function
                UINFO(6, "  Wrapping " << name << " multifuncs");
                AstCFunc* ਅਚਲ newfuncp = ਸਿਖਰ_Funcp->clone_ਰੁੱਖ(ਝੂਠ);
                ਜੇ (newfuncp->varsp()) newfuncp->varsp()->unlink_Fr_ਪਿੱਠ_With_ਅਗਲਾ()->delete_ਰੁੱਖ();
                ਜੇ (newfuncp->stmtsp()) newfuncp->stmtsp()->unlink_Fr_ਪਿੱਠ_With_ਅਗਲਾ()->delete_ਰੁੱਖ();
                newfuncp->name(name);
                newfuncp->isStatic(ਝੂਠ);
                ਸਿਖਰ_Funcp->ਜੋੜੋ_ਅਗਲਾ_ਇੱਥੇ(newfuncp);
                // In the body, call each function if it matches the given scope
                ਲਈ (FuncMmap::iterator eachIt = it;
                     eachIt != m_modFuncs.ਸਿਰਾ() && eachIt->ਪਹਿਲਾ == name; ++eachIt) {
                    it = eachIt;
                    AstCFunc* ਅਚਲ funcp = eachIt->ਦੂਜਾ;
                    ਸਵੈਚਾਲਿਤ ਅਗਲਾ_It2 = eachIt;
                    ++ਅਗਲਾ_It2;
                    ਅਚਲ ਤਾਰਕਿਕ ਹੋਰ_ਦਾ_Same
                        = (ਅਗਲਾ_It2 != m_modFuncs.ਸਿਰਾ() && ਅਗਲਾ_It2->ਪਹਿਲਾ == name);
                    UASSERT_OBJ(funcp->scopep(), funcp, "Not scoped");

                    UINFO(6, "     Wrapping " << name << " " << funcp);
                    UINFO(6, "  at " << newfuncp->argTypes() << " und " << funcp->argTypes());
                    funcp->decl_ਨਿੱਜੀ(ਸੱਚ);
                    AstVarRef* argsp = ਖਾਲੀ_ਸੂਚਕ;
                    ਲਈ (Ast_ਗੰਢ* stmtp = newfuncp->argsp(); stmtp; stmtp = stmtp->nextp()) {
                        ਜੇ (AstVar* ਅਚਲ portp = VN_CAST(stmtp, Var)) {
                            ਜੇ (portp->isIO() && !portp->isFuncReturn()) {
                                AstVarRef* ਅਚਲ newp = ਨਵਾਂ AstVarRef{
                                    portp->fileline(), portp, portp->direction().pinAccess()};
                                argsp = Ast_ਗੰਢ::ਜੋੜੋ_ਅਗਲਾ(argsp, newp);
                            }
                        }
                    }

                    AstCCall* ਅਚਲ callp = ਨਵਾਂ AstCCall{funcp->fileline(), funcp, argsp};
                    ਜੇ (Ast_ਗੰਢ_DType* ਅਚਲ dtypep = funcp->dtypep()) {
                        callp->dtypep(dtypep);
                    } ਨਹੀਂਤਾਂ {
                        callp->dtype_ਸੈੱਟ_Void();
                    }
                    Ast_ਗੰਢ* ਅਚਲ returnp = ਨਵਾਂ AstCReturn{funcp->fileline(), callp};

                    ਜੇ (ਹੋਰ_ਦਾ_Same) {
                        AstIf* ਅਚਲ ifp = ਨਵਾਂ AstIf{
                            funcp->fileline(),
                            ਨਵਾਂ AstEq{funcp->fileline(),
                                      ਨਵਾਂ AstCExpr{funcp->fileline(), "this", 64},
                                      ਨਵਾਂ AstCExpr{funcp->fileline(),
                                                   scopeVlSyms(funcp->scopep()).asString(), 64}},
                            returnp};
                        newfuncp->ਜੋੜੋ_Stmtsp(ifp);
                    } ਨਹੀਂਤਾਂ {
                        newfuncp->ਜੋੜੋ_Stmtsp(returnp);
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
            } ਨਹੀਂਤਾਂ {
                // Only a single function under this name, we can rename it
                UINFO(6, "  Wrapping " << name << " just one " << ਸਿਖਰ_Funcp);
                ਸਿਖਰ_Funcp->name(name);
            }
        }
    }

    // VISITORS
    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ_Module* nodep) override {
        VL_RESTORER(m_modp);
        m_modp = nodep;
        m_modFuncs.clear();
        m_modSingleton = modIsSingleton(m_modp);
        iterateChildren(nodep);
        make_ਜਨਤਕ_Func_Wrappers();
    }
    ਖਾਲੀ ਦੇਖੋ(AstScope* nodep) override {
        VL_RESTORER(m_scopep);
        m_scopep = nodep;
        iterateChildren(nodep);
    }
    ਖਾਲੀ ਦੇਖੋ(AstVarScope* nodep) override {
        // Delete the varscope when we're finished
        VL_DO_DANGLING(ਧੱਕੋ_Deletep(nodep->unlink_Fr_ਪਿੱਠ()), nodep);
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ_Var_Ref* nodep) override {
        iterateChildren(nodep);
        ਜੇ (!nodep->varScopep()) {
            UASSERT_OBJ(nodep->varp()->isFuncLocal(), nodep,
                        "unscoped reference can only appear to function locals at this point");
            ਵਾਪਸ;
        }
        // Convert the hierch name
        UINFO(9, "  ref-in " << nodep);
        UASSERT_OBJ(m_scopep, nodep, "Node not under scope");
        ਅਚਲ AstVar* ਅਚਲ varp = nodep->varScopep()->varp();
        ਅਚਲ AstScope* ਅਚਲ scopep = nodep->varScopep()->scopep();
        ਜੇ (varp->isFuncLocal()) {
            // Reference to function locals need no self pointer
            nodep->self_ਸੂਚਕ(VSelf_ਸੂਚਕ_ਪਾਠ{VSelf_ਸੂਚਕ_ਪਾਠ::ਖਾਲੀ_ਥਾਵਾਂ_2()});
        } ਨਹੀਂਤਾਂ ਜੇ (scopep->modp()->isConstPool()) {
            // Reference to constant pool value need no self pointer
            nodep->self_ਸੂਚਕ(VSelf_ਸੂਚਕ_ਪਾਠ{VSelf_ਸੂਚਕ_ਪਾਠ::ਖਾਲੀ_ਥਾਵਾਂ_2()});
        } ਨਹੀਂਤਾਂ {
            nodep->self_ਸੂਚਕ(descoped_Self_ਸੂਚਕ(scopep));
        }
        nodep->varScopep(ਖਾਲੀ_ਸੂਚਕ);
        UINFO(9, "  refout " << nodep << " selfPtr=" << nodep->self_ਸੂਚਕ().asString());
    }
    ਖਾਲੀ ਦੇਖੋ(AstCCall* nodep) override {
        // UINFO(9, "       " << nodep);
        iterateChildren(nodep);
        // Convert the hierch name
        UASSERT_OBJ(m_scopep, nodep, "Node not under scope");
        ਅਚਲ AstScope* ਅਚਲ scopep = nodep->funcp()->scopep();
        nodep->self_ਸੂਚਕ(descoped_Self_ਸੂਚਕ(scopep));
        // Can't do this, as we may have more calls later
        // nodep->funcp()->scopep(nullptr);
    }
    ਖਾਲੀ ਦੇਖੋ(AstScopeName* nodep) override {
        // Print a %m under this instance as its vlNamep, which holds its pretty name, plus the
        // rest of the path, so the instances' code can be V3Combined. Not with --protect-ids, as
        // vlNamep then holds the protected name, while %m prints the original.
        ਜੇ (!m_funcp || !nodep->forFormat() || m_modSingleton || m_funcp->isStatic()
            || v3Global.opt.protectIds()) {
            ਵਾਪਸ;
        }
        std::string ਰਸਤਾ = nodep->scopePrettySymName();
        ਅਚਲ std::string inst = m_scopep->prettyName();
        ਜੇ (ਰਸਤਾ != inst && !VString::startsWith(ਰਸਤਾ, inst + ".")) ਵਾਪਸ;
        ਰਸਤਾ.erase(0, inst.ਮਾਪ() + 1);
        // The DPI scope table keeps the absolute name, via a copy outside any function
        m_scopep->ਜੋੜੋ_Blocksp(nodep->clone_ਰੁੱਖ(ਝੂਠ));
        nodep->scopeAttr(ਰਸਤਾ);
        nodep->scopeEntr("");  // Only used by the DPI scope table
        nodep->selfRelative(ਸੱਚ);
    }
    ਖਾਲੀ ਦੇਖੋ(AstCMethodCall* nodep) override { iterateChildren(nodep); }
    ਖਾਲੀ ਦੇਖੋ(AstCNew* nodep) override { iterateChildren(nodep); }
    ਖਾਲੀ ਦੇਖੋ(AstCFunc* nodep) override {
        VL_RESTORER(m_funcp);
        ਜੇ (!nodep->user1()) {
            // Static functions should have been moved under the corresponding AstClassPackage
            UASSERT_OBJ(!(nodep->isStatic() && VN_IS(m_modp, Class)), nodep,
                        "Static function under AstClass");
            m_funcp = nodep;
            iterateChildren(nodep);
            nodep->user1(ਸੱਚ);
            // If it's under a scope, move it up to the top
            ਜੇ (m_scopep) {
                nodep->unlink_Fr_ਪਿੱਠ();
                m_modp->ਜੋੜੋ_Stmtsp(nodep);

                ਜੇ (nodep->func_ਜਨਤਕ()) {
                    // There may be multiple public functions by the same name;
                    // record for later correction or making of shells
                    m_modFuncs.emplace(nodep->name(), nodep);
                    nodep->name(m_scopep->nameDotless() + "__" + nodep->name());
                }
            }
        }
    }
    ਖਾਲੀ ਦੇਖੋ(AstVar*) override {}
    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ* nodep) override { iterateChildren(nodep); }

ਜਨਤਕ:
    // CONSTRUCTORS
    ਸਪਸ਼ਟ DescopeVisitor(AstNetlist* nodep) { iterate(nodep); }
    ~DescopeVisitor() override = ਮੂਲ;
};

//######################################################################
// Descope class functions

ਖਾਲੀ V3Descope::descopeAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { DescopeVisitor{nodep}; }  // Destruct before checking
    V3ConstPool::ਸੈੱਟ_Descoped();
    V3Global::dump_Check_Global_ਰੁੱਖ("descope", 0, dump_ਰੁੱਖ_Either_Level() >= 3);
}
