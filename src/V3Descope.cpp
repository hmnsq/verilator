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

#شامل "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#شامل "V3Descope.h"

#شامل "V3ConstPool.h"
#شامل "V3EmitCBase.h"

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################

جماعت DescopeVisitor final : عوامی VNVisitor {
    // NODE STATE
    //  Cleared entire netlist
    //   AstCFunc::user()               // bool.  Indicates processing completed
    ثابت VNUser1_اندر_Use m_inuser1;

    // TYPES
    استعمال FuncMmap = std::multimap<std::string, AstCFunc*>;
    ساخت ScopeSelfPtr final {
        VSelf_اشارہ_گر_متن thisPtr = VSelf_اشارہ_گر_متن{VSelf_اشارہ_گر_متن::خالی_خانے_2()};
        VSelf_اشارہ_گر_متن vlSymsPtr = VSelf_اشارہ_گر_متن{VSelf_اشارہ_گر_متن::خالی_خانے_2()};
    };

    // STATE
    Ast_گرہ_Module* m_modp = خالی_اشاریہ;  // Current module
    AstScope* m_scopep = خالی_اشاریہ;  // Current scope
    ثابت AstCFunc* m_funcp = خالی_اشاریہ;  // Current function
    منطقی m_modSingleton = جھوٹ;  // m_modp is only instantiated once
    FuncMmap m_modFuncs;  // Name of public functions added
    std::map<ثابت AstScope*, ScopeSelfPtr> m_scope_مقصد_Self;  // Scope to self pointers

    // METHODS

    ساکن منطقی modIsSingleton(ثابت Ast_گرہ_Module* modp) {
        // True iff there's exactly one instance of this module in the design (including top).
        اگر (modp->is_چوٹی()) واپس سچ;
        صحیح instances = 0;
        برائے (ثابت Ast_گرہ* stmtp = modp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            اگر (VN_IS(stmtp, Scope)) {
                اگر (++instances > 1) واپس جھوٹ;
            }
        }
        واپس (instances == 1);
    }

    // Construct a 'this' self pointer for the given scope
    VSelf_اشارہ_گر_متن scopeThis(ثابت AstScope* scopep) {
        خودکار& ret = m_scope_مقصد_Self[scopep];
        اگر (ret.thisPtr.is_خالی_خانے()) {
            string name = scopep->name();
            string::سائز_type pos = name.rfind('.');
            اگر (pos != string::npos) name.erase(0, pos + 1);
            ret.thisPtr = VSelf_اشارہ_گر_متن{VSelf_اشارہ_گر_متن::This{}, name};
        }
        واپس ret.thisPtr;
    }
    // Construct a 'vlSyms' self pointer for the given scope
    VSelf_اشارہ_گر_متن scopeVlSyms(ثابت AstScope* scopep) {
        خودکار& ret = m_scope_مقصد_Self[scopep];
        اگر (ret.vlSymsPtr.is_خالی_خانے()) {
            ret.vlSymsPtr = VSelf_اشارہ_گر_متن{VSelf_اشارہ_گر_متن::VlSyms(), scopep->nameDotless()};
        }
        واپس ret.vlSymsPtr;
    }

    // Construct the best self pointer to reference an object in 'scopep'  from a CFunc in
    // 'm_scopep'. Result may be relative ("this->[...]") or absolute ("vlSyms->[...]").
    //
    // Using relative references allows V3Combine'ing code across multiple instances of the same
    // module.
    VSelf_اشارہ_گر_متن descoped_Self_اشارہ_گر(ثابت AstScope* scopep) {
        UASSERT(scopep, "Var/Func not scoped");
        // Static functions can't use relative references via 'this->'
        ثابت منطقی relativeRefOk = !m_funcp->isStatic();

        UINFO(8, "      Descope ref under " << m_scopep);
        UINFO(8, "              ref to    " << scopep);
        UINFO(8, "             aboveScope " << scopep->aboveScopep());

        اگر (VN_IS(scopep->modp(), Class)) {
            // Direct reference to class members are from within the class itself, references from
            // outside the class must go via AstMemberSel
            واپس VSelf_اشارہ_گر_متن{VSelf_اشارہ_گر_متن::This{}};
        } ورنہ اگر (relativeRefOk && scopep == m_scopep) {
            واپس VSelf_اشارہ_گر_متن{VSelf_اشارہ_گر_متن::This{}};
        } ورنہ اگر (relativeRefOk && !m_modSingleton && scopep->aboveScopep() == m_scopep
                   && VN_IS(scopep->modp(), Module)) {
            // Reference to scope of instance directly under this module, can just "this->cell",
            // which can potentially be V3Combined, but note this requires one extra pointer
            // dereference which is slower, so we only use it if the source scope is not a
            // singleton.
            واپس scopeThis(scopep);
        } ورنہ {
            // Reference to something elsewhere, or relative references are disabled. Use global
            // variable
            واپس scopeVlSyms(scopep);
        }
    }

    خالی make_عوامی_Func_Wrappers() {
        // We recorded all public functions in m_modFuncs.
        // If for any given name only one function exists, we can use that function directly.
        // If multiple functions exist, we need to select the appropriate scope.
        برائے (FuncMmap::iterator it = m_modFuncs.begin(); it != m_modFuncs.انتہا(); ++it) {
            ثابت string name = it->پہلا;
            AstCFunc* ثابت چوٹی_Funcp = it->دوسرا;
            خودکار اگلا_It1 = it;
            ++اگلا_It1;
            ثابت منطقی مزید_کا_Same1 = (اگلا_It1 != m_modFuncs.انتہا() && اگلا_It1->پہلا == name);
            اگر (مزید_کا_Same1) {
                // Multiple functions under this name, need a wrapper function
                UINFO(6, "  Wrapping " << name << " multifuncs");
                AstCFunc* ثابت newfuncp = چوٹی_Funcp->clone_درخت(جھوٹ);
                اگر (newfuncp->varsp()) newfuncp->varsp()->unlink_Fr_عقب_With_اگلا()->delete_درخت();
                اگر (newfuncp->stmtsp()) newfuncp->stmtsp()->unlink_Fr_عقب_With_اگلا()->delete_درخت();
                newfuncp->name(name);
                newfuncp->isStatic(جھوٹ);
                چوٹی_Funcp->جوڑو_اگلا_یہاں(newfuncp);
                // In the body, call each function if it matches the given scope
                برائے (FuncMmap::iterator eachIt = it;
                     eachIt != m_modFuncs.انتہا() && eachIt->پہلا == name; ++eachIt) {
                    it = eachIt;
                    AstCFunc* ثابت funcp = eachIt->دوسرا;
                    خودکار اگلا_It2 = eachIt;
                    ++اگلا_It2;
                    ثابت منطقی مزید_کا_Same
                        = (اگلا_It2 != m_modFuncs.انتہا() && اگلا_It2->پہلا == name);
                    UASSERT_OBJ(funcp->scopep(), funcp, "Not scoped");

                    UINFO(6, "     Wrapping " << name << " " << funcp);
                    UINFO(6, "  at " << newfuncp->argTypes() << " und " << funcp->argTypes());
                    funcp->decl_نجی(سچ);
                    AstVarRef* argsp = خالی_اشاریہ;
                    برائے (Ast_گرہ* stmtp = newfuncp->argsp(); stmtp; stmtp = stmtp->nextp()) {
                        اگر (AstVar* ثابت portp = VN_CAST(stmtp, Var)) {
                            اگر (portp->isIO() && !portp->isFuncReturn()) {
                                AstVarRef* ثابت newp = نیا AstVarRef{
                                    portp->fileline(), portp, portp->direction().pinAccess()};
                                argsp = Ast_گرہ::جوڑو_اگلا(argsp, newp);
                            }
                        }
                    }

                    AstCCall* ثابت callp = نیا AstCCall{funcp->fileline(), funcp, argsp};
                    اگر (Ast_گرہ_DType* ثابت dtypep = funcp->dtypep()) {
                        callp->dtypep(dtypep);
                    } ورنہ {
                        callp->dtype_سیٹ_Void();
                    }
                    Ast_گرہ* ثابت returnp = نیا AstCReturn{funcp->fileline(), callp};

                    اگر (مزید_کا_Same) {
                        AstIf* ثابت ifp = نیا AstIf{
                            funcp->fileline(),
                            نیا AstEq{funcp->fileline(),
                                      نیا AstCExpr{funcp->fileline(), "this", 64},
                                      نیا AstCExpr{funcp->fileline(),
                                                   scopeVlSyms(funcp->scopep()).asString(), 64}},
                            returnp};
                        newfuncp->جوڑو_Stmtsp(ifp);
                    } ورنہ {
                        newfuncp->جوڑو_Stmtsp(returnp);
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
            } ورنہ {
                // Only a single function under this name, we can rename it
                UINFO(6, "  Wrapping " << name << " just one " << چوٹی_Funcp);
                چوٹی_Funcp->name(name);
            }
        }
    }

    // VISITORS
    خالی دورہ(Ast_گرہ_Module* nodep) override {
        VL_RESTORER(m_modp);
        m_modp = nodep;
        m_modFuncs.clear();
        m_modSingleton = modIsSingleton(m_modp);
        iterateChildren(nodep);
        make_عوامی_Func_Wrappers();
    }
    خالی دورہ(AstScope* nodep) override {
        VL_RESTORER(m_scopep);
        m_scopep = nodep;
        iterateChildren(nodep);
    }
    خالی دورہ(AstVarScope* nodep) override {
        // Delete the varscope when we're finished
        VL_DO_DANGLING(ڈالو_Deletep(nodep->unlink_Fr_عقب()), nodep);
    }
    خالی دورہ(Ast_گرہ_Var_Ref* nodep) override {
        iterateChildren(nodep);
        اگر (!nodep->varScopep()) {
            UASSERT_OBJ(nodep->varp()->isFuncLocal(), nodep,
                        "unscoped reference can only appear to function locals at this point");
            واپس;
        }
        // Convert the hierch name
        UINFO(9, "  ref-in " << nodep);
        UASSERT_OBJ(m_scopep, nodep, "Node not under scope");
        ثابت AstVar* ثابت varp = nodep->varScopep()->varp();
        ثابت AstScope* ثابت scopep = nodep->varScopep()->scopep();
        اگر (varp->isFuncLocal()) {
            // Reference to function locals need no self pointer
            nodep->self_اشارہ_گر(VSelf_اشارہ_گر_متن{VSelf_اشارہ_گر_متن::خالی_خانے_2()});
        } ورنہ اگر (scopep->modp()->isConstPool()) {
            // Reference to constant pool value need no self pointer
            nodep->self_اشارہ_گر(VSelf_اشارہ_گر_متن{VSelf_اشارہ_گر_متن::خالی_خانے_2()});
        } ورنہ {
            nodep->self_اشارہ_گر(descoped_Self_اشارہ_گر(scopep));
        }
        nodep->varScopep(خالی_اشاریہ);
        UINFO(9, "  refout " << nodep << " selfPtr=" << nodep->self_اشارہ_گر().asString());
    }
    خالی دورہ(AstCCall* nodep) override {
        // UINFO(9, "       " << nodep);
        iterateChildren(nodep);
        // Convert the hierch name
        UASSERT_OBJ(m_scopep, nodep, "Node not under scope");
        ثابت AstScope* ثابت scopep = nodep->funcp()->scopep();
        nodep->self_اشارہ_گر(descoped_Self_اشارہ_گر(scopep));
        // Can't do this, as we may have more calls later
        // nodep->funcp()->scopep(nullptr);
    }
    خالی دورہ(AstScopeName* nodep) override {
        // Print a %m under this instance as its vlNamep, which holds its pretty name, plus the
        // rest of the path, so the instances' code can be V3Combined. Not with --protect-ids, as
        // vlNamep then holds the protected name, while %m prints the original.
        اگر (!m_funcp || !nodep->forFormat() || m_modSingleton || m_funcp->isStatic()
            || v3Global.opt.protectIds()) {
            واپس;
        }
        std::string راستہ = nodep->scopePrettySymName();
        ثابت std::string inst = m_scopep->prettyName();
        اگر (راستہ != inst && !VString::startsWith(راستہ, inst + ".")) واپس;
        راستہ.erase(0, inst.سائز() + 1);
        // The DPI scope table keeps the absolute name, via a copy outside any function
        m_scopep->جوڑو_Blocksp(nodep->clone_درخت(جھوٹ));
        nodep->scopeAttr(راستہ);
        nodep->scopeEntr("");  // Only used by the DPI scope table
        nodep->selfRelative(سچ);
    }
    خالی دورہ(AstCMethodCall* nodep) override { iterateChildren(nodep); }
    خالی دورہ(AstCNew* nodep) override { iterateChildren(nodep); }
    خالی دورہ(AstCFunc* nodep) override {
        VL_RESTORER(m_funcp);
        اگر (!nodep->user1()) {
            // Static functions should have been moved under the corresponding AstClassPackage
            UASSERT_OBJ(!(nodep->isStatic() && VN_IS(m_modp, Class)), nodep,
                        "Static function under AstClass");
            m_funcp = nodep;
            iterateChildren(nodep);
            nodep->user1(سچ);
            // If it's under a scope, move it up to the top
            اگر (m_scopep) {
                nodep->unlink_Fr_عقب();
                m_modp->جوڑو_Stmtsp(nodep);

                اگر (nodep->func_عوامی()) {
                    // There may be multiple public functions by the same name;
                    // record for later correction or making of shells
                    m_modFuncs.emplace(nodep->name(), nodep);
                    nodep->name(m_scopep->nameDotless() + "__" + nodep->name());
                }
            }
        }
    }
    خالی دورہ(AstVar*) override {}
    خالی دورہ(Ast_گرہ* nodep) override { iterateChildren(nodep); }

عوامی:
    // CONSTRUCTORS
    صریح DescopeVisitor(AstNetlist* nodep) { iterate(nodep); }
    ~DescopeVisitor() override = طےشدہ;
};

//######################################################################
// Descope class functions

خالی V3Descope::descopeAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { DescopeVisitor{nodep}; }  // Destruct before checking
    V3ConstPool::سیٹ_Descoped();
    V3Global::dump_Check_Global_درخت("descope", 0, dump_درخت_Either_Level() >= 3);
}
