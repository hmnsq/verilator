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

#ضمن "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ضمن "V3Descope.h"

#ضمن "V3ConstPool.h"
#ضمن "V3EmitCBase.h"

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################

صنف DescopeVisitor final : عام VNVisitor {
    // NODE STATE
    //  Cleared entire netlist
    //   AstCFunc::user()               // bool.  Indicates processing completed
    ثابت VNUser1_درون_Use m_inuser1;

    // TYPES
    استفاده FuncMmap = std::multimap<std::string, AstCFunc*>;
    بنية ScopeSelfPtr final {
        VSelf_اشاره_گر_متن thisPtr = VSelf_اشاره_گر_متن{VSelf_اشاره_گر_متن::خالی_2()};
        VSelf_اشاره_گر_متن vlSymsPtr = VSelf_اشاره_گر_متن{VSelf_اشاره_گر_متن::خالی_2()};
    };

    // STATE
    Ast_گره_Module* m_modp = اشاره_تهی;  // Current module
    AstScope* m_scopep = اشاره_تهی;  // Current scope
    ثابت AstCFunc* m_funcp = اشاره_تهی;  // Current function
    منطقي m_modSingleton = خطأ;  // m_modp is only instantiated once
    FuncMmap m_modFuncs;  // Name of public functions added
    std::map<ثابت AstScope*, ScopeSelfPtr> m_scope_مقصد_Self;  // Scope to self pointers

    // METHODS

    ساكن منطقي modIsSingleton(ثابت Ast_گره_Module* modp) {
        // True iff there's exactly one instance of this module in the design (including top).
        إذا (modp->is_بالاترین()) أرجع درست;
        صحيح instances = 0;
        لكل (ثابت Ast_گره* stmtp = modp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            إذا (VN_IS(stmtp, Scope)) {
                إذا (++instances > 1) أرجع خطأ;
            }
        }
        أرجع (instances == 1);
    }

    // Construct a 'this' self pointer for the given scope
    VSelf_اشاره_گر_متن scopeThis(ثابت AstScope* scopep) {
        خودکار& ret = m_scope_مقصد_Self[scopep];
        إذا (ret.thisPtr.is_خالی()) {
            string name = scopep->name();
            string::اندازه_type pos = name.rfind('.');
            إذا (pos != string::npos) name.erase(0, pos + 1);
            ret.thisPtr = VSelf_اشاره_گر_متن{VSelf_اشاره_گر_متن::This{}, name};
        }
        أرجع ret.thisPtr;
    }
    // Construct a 'vlSyms' self pointer for the given scope
    VSelf_اشاره_گر_متن scopeVlSyms(ثابت AstScope* scopep) {
        خودکار& ret = m_scope_مقصد_Self[scopep];
        إذا (ret.vlSymsPtr.is_خالی()) {
            ret.vlSymsPtr = VSelf_اشاره_گر_متن{VSelf_اشاره_گر_متن::VlSyms(), scopep->nameDotless()};
        }
        أرجع ret.vlSymsPtr;
    }

    // Construct the best self pointer to reference an object in 'scopep'  from a CFunc in
    // 'm_scopep'. Result may be relative ("this->[...]") or absolute ("vlSyms->[...]").
    //
    // Using relative references allows V3Combine'ing code across multiple instances of the same
    // module.
    VSelf_اشاره_گر_متن descoped_Self_اشاره_گر(ثابت AstScope* scopep) {
        UASSERT(scopep, "Var/Func not scoped");
        // Static functions can't use relative references via 'this->'
        ثابت منطقي relativeRefOk = !m_funcp->isStatic();

        UINFO(8, "      Descope ref under " << m_scopep);
        UINFO(8, "              ref to    " << scopep);
        UINFO(8, "             aboveScope " << scopep->aboveScopep());

        إذا (VN_IS(scopep->modp(), Class)) {
            // Direct reference to class members are from within the class itself, references from
            // outside the class must go via AstMemberSel
            أرجع VSelf_اشاره_گر_متن{VSelf_اشاره_گر_متن::This{}};
        } وإلا إذا (relativeRefOk && scopep == m_scopep) {
            أرجع VSelf_اشاره_گر_متن{VSelf_اشاره_گر_متن::This{}};
        } وإلا إذا (relativeRefOk && !m_modSingleton && scopep->aboveScopep() == m_scopep
                   && VN_IS(scopep->modp(), Module)) {
            // Reference to scope of instance directly under this module, can just "this->cell",
            // which can potentially be V3Combined, but note this requires one extra pointer
            // dereference which is slower, so we only use it if the source scope is not a
            // singleton.
            أرجع scopeThis(scopep);
        } وإلا {
            // Reference to something elsewhere, or relative references are disabled. Use global
            // variable
            أرجع scopeVlSyms(scopep);
        }
    }

    فراغ make_عمومی_Func_Wrappers() {
        // We recorded all public functions in m_modFuncs.
        // If for any given name only one function exists, we can use that function directly.
        // If multiple functions exist, we need to select the appropriate scope.
        لكل (FuncMmap::iterator it = m_modFuncs.begin(); it != m_modFuncs.انتها(); ++it) {
            ثابت string name = it->اولی;
            AstCFunc* ثابت بالاترین_Funcp = it->دومی;
            خودکار بعدی_It1 = it;
            ++بعدی_It1;
            ثابت منطقي بیشتر_از_Same1 = (بعدی_It1 != m_modFuncs.انتها() && بعدی_It1->اولی == name);
            إذا (بیشتر_از_Same1) {
                // Multiple functions under this name, need a wrapper function
                UINFO(6, "  Wrapping " << name << " multifuncs");
                AstCFunc* ثابت newfuncp = بالاترین_Funcp->clone_درخت(خطأ);
                إذا (newfuncp->varsp()) newfuncp->varsp()->unlink_Fr_پشت_With_بعدی()->delete_درخت();
                إذا (newfuncp->stmtsp()) newfuncp->stmtsp()->unlink_Fr_پشت_With_بعدی()->delete_درخت();
                newfuncp->name(name);
                newfuncp->isStatic(خطأ);
                بالاترین_Funcp->جمع_کن_بعدی_اینجا(newfuncp);
                // In the body, call each function if it matches the given scope
                لكل (FuncMmap::iterator eachIt = it;
                     eachIt != m_modFuncs.انتها() && eachIt->اولی == name; ++eachIt) {
                    it = eachIt;
                    AstCFunc* ثابت funcp = eachIt->دومی;
                    خودکار بعدی_It2 = eachIt;
                    ++بعدی_It2;
                    ثابت منطقي بیشتر_از_Same
                        = (بعدی_It2 != m_modFuncs.انتها() && بعدی_It2->اولی == name);
                    UASSERT_OBJ(funcp->scopep(), funcp, "Not scoped");

                    UINFO(6, "     Wrapping " << name << " " << funcp);
                    UINFO(6, "  at " << newfuncp->argTypes() << " und " << funcp->argTypes());
                    funcp->decl_خصوصی(درست);
                    AstVarRef* argsp = اشاره_تهی;
                    لكل (Ast_گره* stmtp = newfuncp->argsp(); stmtp; stmtp = stmtp->nextp()) {
                        إذا (AstVar* ثابت portp = VN_CAST(stmtp, Var)) {
                            إذا (portp->isIO() && !portp->isFuncReturn()) {
                                AstVarRef* ثابت newp = جديد AstVarRef{
                                    portp->fileline(), portp, portp->direction().pinAccess()};
                                argsp = Ast_گره::جمع_کن_بعدی(argsp, newp);
                            }
                        }
                    }

                    AstCCall* ثابت callp = جديد AstCCall{funcp->fileline(), funcp, argsp};
                    إذا (Ast_گره_DType* ثابت dtypep = funcp->dtypep()) {
                        callp->dtypep(dtypep);
                    } وإلا {
                        callp->dtype_مجموعه_Void();
                    }
                    Ast_گره* ثابت returnp = جديد AstCReturn{funcp->fileline(), callp};

                    إذا (بیشتر_از_Same) {
                        AstIf* ثابت ifp = جديد AstIf{
                            funcp->fileline(),
                            جديد AstEq{funcp->fileline(),
                                      جديد AstCExpr{funcp->fileline(), "this", 64},
                                      جديد AstCExpr{funcp->fileline(),
                                                   scopeVlSyms(funcp->scopep()).asString(), 64}},
                            returnp};
                        newfuncp->جمع_کن_Stmtsp(ifp);
                    } وإلا {
                        newfuncp->جمع_کن_Stmtsp(returnp);
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
            } وإلا {
                // Only a single function under this name, we can rename it
                UINFO(6, "  Wrapping " << name << " just one " << بالاترین_Funcp);
                بالاترین_Funcp->name(name);
            }
        }
    }

    // VISITORS
    فراغ بازدید(Ast_گره_Module* nodep) override {
        VL_RESTORER(m_modp);
        m_modp = nodep;
        m_modFuncs.clear();
        m_modSingleton = modIsSingleton(m_modp);
        iterateChildren(nodep);
        make_عمومی_Func_Wrappers();
    }
    فراغ بازدید(AstScope* nodep) override {
        VL_RESTORER(m_scopep);
        m_scopep = nodep;
        iterateChildren(nodep);
    }
    فراغ بازدید(AstVarScope* nodep) override {
        // Delete the varscope when we're finished
        VL_DO_DANGLING(افزودن_Deletep(nodep->unlink_Fr_پشت()), nodep);
    }
    فراغ بازدید(Ast_گره_Var_Ref* nodep) override {
        iterateChildren(nodep);
        إذا (!nodep->varScopep()) {
            UASSERT_OBJ(nodep->varp()->isFuncLocal(), nodep,
                        "unscoped reference can only appear to function locals at this point");
            أرجع;
        }
        // Convert the hierch name
        UINFO(9, "  ref-in " << nodep);
        UASSERT_OBJ(m_scopep, nodep, "Node not under scope");
        ثابت AstVar* ثابت varp = nodep->varScopep()->varp();
        ثابت AstScope* ثابت scopep = nodep->varScopep()->scopep();
        إذا (varp->isFuncLocal()) {
            // Reference to function locals need no self pointer
            nodep->self_اشاره_گر(VSelf_اشاره_گر_متن{VSelf_اشاره_گر_متن::خالی_2()});
        } وإلا إذا (scopep->modp()->isConstPool()) {
            // Reference to constant pool value need no self pointer
            nodep->self_اشاره_گر(VSelf_اشاره_گر_متن{VSelf_اشاره_گر_متن::خالی_2()});
        } وإلا {
            nodep->self_اشاره_گر(descoped_Self_اشاره_گر(scopep));
        }
        nodep->varScopep(اشاره_تهی);
        UINFO(9, "  refout " << nodep << " selfPtr=" << nodep->self_اشاره_گر().asString());
    }
    فراغ بازدید(AstCCall* nodep) override {
        // UINFO(9, "       " << nodep);
        iterateChildren(nodep);
        // Convert the hierch name
        UASSERT_OBJ(m_scopep, nodep, "Node not under scope");
        ثابت AstScope* ثابت scopep = nodep->funcp()->scopep();
        nodep->self_اشاره_گر(descoped_Self_اشاره_گر(scopep));
        // Can't do this, as we may have more calls later
        // nodep->funcp()->scopep(nullptr);
    }
    فراغ بازدید(AstScopeName* nodep) override {
        // Print a %m under this instance as its vlNamep, which holds its pretty name, plus the
        // rest of the path, so the instances' code can be V3Combined. Not with --protect-ids, as
        // vlNamep then holds the protected name, while %m prints the original.
        إذا (!m_funcp || !nodep->forFormat() || m_modSingleton || m_funcp->isStatic()
            || v3Global.opt.protectIds()) {
            أرجع;
        }
        std::string مسیر = nodep->scopePrettySymName();
        ثابت std::string inst = m_scopep->prettyName();
        إذا (مسیر != inst && !VString::startsWith(مسیر, inst + ".")) أرجع;
        مسیر.erase(0, inst.اندازه() + 1);
        // The DPI scope table keeps the absolute name, via a copy outside any function
        m_scopep->جمع_کن_Blocksp(nodep->clone_درخت(خطأ));
        nodep->scopeAttr(مسیر);
        nodep->scopeEntr("");  // Only used by the DPI scope table
        nodep->selfRelative(درست);
    }
    فراغ بازدید(AstCMethodCall* nodep) override { iterateChildren(nodep); }
    فراغ بازدید(AstCNew* nodep) override { iterateChildren(nodep); }
    فراغ بازدید(AstCFunc* nodep) override {
        VL_RESTORER(m_funcp);
        إذا (!nodep->user1()) {
            // Static functions should have been moved under the corresponding AstClassPackage
            UASSERT_OBJ(!(nodep->isStatic() && VN_IS(m_modp, Class)), nodep,
                        "Static function under AstClass");
            m_funcp = nodep;
            iterateChildren(nodep);
            nodep->user1(درست);
            // If it's under a scope, move it up to the top
            إذا (m_scopep) {
                nodep->unlink_Fr_پشت();
                m_modp->جمع_کن_Stmtsp(nodep);

                إذا (nodep->func_عمومی()) {
                    // There may be multiple public functions by the same name;
                    // record for later correction or making of shells
                    m_modFuncs.emplace(nodep->name(), nodep);
                    nodep->name(m_scopep->nameDotless() + "__" + nodep->name());
                }
            }
        }
    }
    فراغ بازدید(AstVar*) override {}
    فراغ بازدید(Ast_گره* nodep) override { iterateChildren(nodep); }

عام:
    // CONSTRUCTORS
    صریح DescopeVisitor(AstNetlist* nodep) { iterate(nodep); }
    ~DescopeVisitor() override = افتراضي;
};

//######################################################################
// Descope class functions

فراغ V3Descope::descopeAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { DescopeVisitor{nodep}; }  // Destruct before checking
    V3ConstPool::مجموعه_Descoped();
    V3Global::dump_Check_Global_درخت("descope", 0, dump_درخت_Either_Level() >= 3);
}
