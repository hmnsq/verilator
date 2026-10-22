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
    ثابت VNUser1InUse m_inuser1;

    // TYPES
    using FuncMmap = std::multimap<std::string, AstCFunc*>;
    بنية ScopeSelfPtr final {
        VSelfPointerText thisPtr = VSelfPointerText{VSelfPointerText::Empty()};
        VSelfPointerText vlSymsPtr = VSelfPointerText{VSelfPointerText::Empty()};
    };

    // STATE
    Ast_عقدة_Module* m_modp = nullptr;  // Current module
    AstScope* m_scopep = nullptr;  // Current scope
    ثابت AstCFunc* m_funcp = nullptr;  // Current function
    منطقي m_modSingleton = خطأ;  // m_modp is only instantiated once
    FuncMmap m_modFuncs;  // Name of public functions added
    std::map<ثابت AstScope*, ScopeSelfPtr> m_scopeToSelf;  // Scope to self pointers

    // METHODS

    ساكن منطقي modIsSingleton(ثابت Ast_عقدة_Module* modp) {
        // True iff there's exactly one instance of this module in the design (including top).
        إذا (modp->is_قمة()) إرجاع true;
        صحيح instances = 0;
        لكل (ثابت Ast_عقدة* stmtp = modp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            إذا (VN_IS(stmtp, Scope)) {
                إذا (++instances > 1) إرجاع خطأ;
            }
        }
        إرجاع (instances == 1);
    }

    // Construct a 'this' self pointer for the given scope
    VSelfPointerText scopeThis(ثابت AstScope* scopep) {
        auto& ret = m_scopeToSelf[scopep];
        إذا (ret.thisPtr.isEmpty()) {
            string name = scopep->name();
            string::size_type pos = name.rfind('.');
            إذا (pos != string::npos) name.erase(0, pos + 1);
            ret.thisPtr = VSelfPointerText{VSelfPointerText::This{}, name};
        }
        إرجاع ret.thisPtr;
    }
    // Construct a 'vlSyms' self pointer for the given scope
    VSelfPointerText scopeVlSyms(ثابت AstScope* scopep) {
        auto& ret = m_scopeToSelf[scopep];
        إذا (ret.vlSymsPtr.isEmpty()) {
            ret.vlSymsPtr = VSelfPointerText{VSelfPointerText::VlSyms(), scopep->nameDotless()};
        }
        إرجاع ret.vlSymsPtr;
    }

    // Construct the best self pointer to reference an object in 'scopep'  from a CFunc in
    // 'm_scopep'. Result may be relative ("this->[...]") or absolute ("vlSyms->[...]").
    //
    // Using relative references allows V3Combine'ing code across multiple instances of the same
    // module.
    VSelfPointerText descopedSelfPointer(ثابت AstScope* scopep) {
        UASSERT(scopep, "Var/Func not scoped");
        // Static functions can't use relative references via 'this->'
        ثابت منطقي relativeRefOk = !m_funcp->isStatic();

        UINFO(8, "      Descope ref under " << m_scopep);
        UINFO(8, "              ref to    " << scopep);
        UINFO(8, "             aboveScope " << scopep->aboveScopep());

        إذا (VN_IS(scopep->modp(), Class)) {
            // Direct reference to class members are from within the class itself, references from
            // outside the class must go via AstMemberSel
            إرجاع VSelfPointerText{VSelfPointerText::This{}};
        } وإلا إذا (relativeRefOk && scopep == m_scopep) {
            إرجاع VSelfPointerText{VSelfPointerText::This{}};
        } وإلا إذا (relativeRefOk && !m_modSingleton && scopep->aboveScopep() == m_scopep
                   && VN_IS(scopep->modp(), Module)) {
            // Reference to scope of instance directly under this module, can just "this->cell",
            // which can potentially be V3Combined, but note this requires one extra pointer
            // dereference which is slower, so we only use it if the source scope is not a
            // singleton.
            إرجاع scopeThis(scopep);
        } وإلا {
            // Reference to something elsewhere, or relative references are disabled. Use global
            // variable
            إرجاع scopeVlSyms(scopep);
        }
    }

    فراغ makePublicFuncWrappers() {
        // We recorded all public functions in m_modFuncs.
        // If for any given name only one function exists, we can use that function directly.
        // If multiple functions exist, we need to select the appropriate scope.
        لكل (FuncMmap::iterator it = m_modFuncs.begin(); it != m_modFuncs.end(); ++it) {
            ثابت string name = it->first;
            AstCFunc* ثابت قمة_Funcp = it->second;
            auto nextIt1 = it;
            ++nextIt1;
            ثابت منطقي moreOfSame1 = (nextIt1 != m_modFuncs.end() && nextIt1->first == name);
            إذا (moreOfSame1) {
                // Multiple functions under this name, need a wrapper function
                UINFO(6, "  Wrapping " << name << " multifuncs");
                AstCFunc* ثابت newfuncp = قمة_Funcp->clone_شجرة(خطأ);
                إذا (newfuncp->varsp()) newfuncp->varsp()->unlinkFrBackWithNext()->delete_شجرة();
                إذا (newfuncp->stmtsp()) newfuncp->stmtsp()->unlinkFrBackWithNext()->delete_شجرة();
                newfuncp->name(name);
                newfuncp->isStatic(خطأ);
                قمة_Funcp->addNextHere(newfuncp);
                // In the body, call each function if it matches the given scope
                لكل (FuncMmap::iterator eachIt = it;
                     eachIt != m_modFuncs.end() && eachIt->first == name; ++eachIt) {
                    it = eachIt;
                    AstCFunc* ثابت funcp = eachIt->second;
                    auto nextIt2 = eachIt;
                    ++nextIt2;
                    ثابت منطقي moreOfSame
                        = (nextIt2 != m_modFuncs.end() && nextIt2->first == name);
                    UASSERT_OBJ(funcp->scopep(), funcp, "Not scoped");

                    UINFO(6, "     Wrapping " << name << " " << funcp);
                    UINFO(6, "  at " << newfuncp->argTypes() << " und " << funcp->argTypes());
                    funcp->declPrivate(true);
                    AstVarRef* argsp = nullptr;
                    لكل (Ast_عقدة* stmtp = newfuncp->argsp(); stmtp; stmtp = stmtp->nextp()) {
                        إذا (AstVar* ثابت portp = VN_CAST(stmtp, Var)) {
                            إذا (portp->isIO() && !portp->isFuncReturn()) {
                                AstVarRef* ثابت newp = جديد AstVarRef{
                                    portp->fileline(), portp, portp->direction().pinAccess()};
                                argsp = Ast_عقدة::addNext(argsp, newp);
                            }
                        }
                    }

                    AstCCall* ثابت callp = جديد AstCCall{funcp->fileline(), funcp, argsp};
                    إذا (Ast_عقدة_DType* ثابت dtypep = funcp->dtypep()) {
                        callp->dtypep(dtypep);
                    } وإلا {
                        callp->dtypeSetVoid();
                    }
                    Ast_عقدة* ثابت returnp = جديد AstCReturn{funcp->fileline(), callp};

                    إذا (moreOfSame) {
                        AstIf* ثابت ifp = جديد AstIf{
                            funcp->fileline(),
                            جديد AstEq{funcp->fileline(),
                                      جديد AstCExpr{funcp->fileline(), "this", 64},
                                      جديد AstCExpr{funcp->fileline(),
                                                   scopeVlSyms(funcp->scopep()).asString(), 64}},
                            returnp};
                        newfuncp->addStmtsp(ifp);
                    } وإلا {
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
            } وإلا {
                // Only a single function under this name, we can rename it
                UINFO(6, "  Wrapping " << name << " just one " << قمة_Funcp);
                قمة_Funcp->name(name);
            }
        }
    }

    // VISITORS
    فراغ visit(Ast_عقدة_Module* nodep) override {
        VL_RESTORER(m_modp);
        m_modp = nodep;
        m_modFuncs.clear();
        m_modSingleton = modIsSingleton(m_modp);
        iterateChildren(nodep);
        makePublicFuncWrappers();
    }
    فراغ visit(AstScope* nodep) override {
        VL_RESTORER(m_scopep);
        m_scopep = nodep;
        iterateChildren(nodep);
    }
    فراغ visit(AstVarScope* nodep) override {
        // Delete the varscope when we're finished
        VL_DO_DANGLING(ادفع_Deletep(nodep->unlinkFrBack()), nodep);
    }
    فراغ visit(Ast_عقدة_Var_Ref* nodep) override {
        iterateChildren(nodep);
        إذا (!nodep->varScopep()) {
            UASSERT_OBJ(nodep->varp()->isFuncLocal(), nodep,
                        "unscoped reference can only appear to function locals at this point");
            إرجاع;
        }
        // Convert the hierch name
        UINFO(9, "  ref-in " << nodep);
        UASSERT_OBJ(m_scopep, nodep, "Node not under scope");
        ثابت AstVar* ثابت varp = nodep->varScopep()->varp();
        ثابت AstScope* ثابت scopep = nodep->varScopep()->scopep();
        إذا (varp->isFuncLocal()) {
            // Reference to function locals need no self pointer
            nodep->selfPointer(VSelfPointerText{VSelfPointerText::Empty()});
        } وإلا إذا (scopep->modp()->isConstPool()) {
            // Reference to constant pool value need no self pointer
            nodep->selfPointer(VSelfPointerText{VSelfPointerText::Empty()});
        } وإلا {
            nodep->selfPointer(descopedSelfPointer(scopep));
        }
        nodep->varScopep(nullptr);
        UINFO(9, "  refout " << nodep << " selfPtr=" << nodep->selfPointer().asString());
    }
    فراغ visit(AstCCall* nodep) override {
        // UINFO(9, "       " << nodep);
        iterateChildren(nodep);
        // Convert the hierch name
        UASSERT_OBJ(m_scopep, nodep, "Node not under scope");
        ثابت AstScope* ثابت scopep = nodep->funcp()->scopep();
        nodep->selfPointer(descopedSelfPointer(scopep));
        // Can't do this, as we may have more calls later
        // nodep->funcp()->scopep(nullptr);
    }
    فراغ visit(AstScopeName* nodep) override {
        // Print a %m under this instance as its vlNamep, which holds its pretty name, plus the
        // rest of the path, so the instances' code can be V3Combined. Not with --protect-ids, as
        // vlNamep then holds the protected name, while %m prints the original.
        إذا (!m_funcp || !nodep->forFormat() || m_modSingleton || m_funcp->isStatic()
            || v3Global.opt.protectIds()) {
            إرجاع;
        }
        std::string path = nodep->scopePrettySymName();
        ثابت std::string inst = m_scopep->prettyName();
        إذا (path != inst && !VString::startsWith(path, inst + ".")) إرجاع;
        path.erase(0, inst.size() + 1);
        // The DPI scope table keeps the absolute name, via a copy outside any function
        m_scopep->addBlocksp(nodep->clone_شجرة(خطأ));
        nodep->scopeAttr(path);
        nodep->scopeEntr("");  // Only used by the DPI scope table
        nodep->selfRelative(true);
    }
    فراغ visit(AstCMethodCall* nodep) override { iterateChildren(nodep); }
    فراغ visit(AstCNew* nodep) override { iterateChildren(nodep); }
    فراغ visit(AstCFunc* nodep) override {
        VL_RESTORER(m_funcp);
        إذا (!nodep->user1()) {
            // Static functions should have been moved under the corresponding AstClassPackage
            UASSERT_OBJ(!(nodep->isStatic() && VN_IS(m_modp, Class)), nodep,
                        "Static function under AstClass");
            m_funcp = nodep;
            iterateChildren(nodep);
            nodep->user1(true);
            // If it's under a scope, move it up to the top
            إذا (m_scopep) {
                nodep->unlinkFrBack();
                m_modp->addStmtsp(nodep);

                إذا (nodep->funcPublic()) {
                    // There may be multiple public functions by the same name;
                    // record for later correction or making of shells
                    m_modFuncs.emplace(nodep->name(), nodep);
                    nodep->name(m_scopep->nameDotless() + "__" + nodep->name());
                }
            }
        }
    }
    فراغ visit(AstVar*) override {}
    فراغ visit(Ast_عقدة* nodep) override { iterateChildren(nodep); }

عام:
    // CONSTRUCTORS
    explicit DescopeVisitor(AstNetlist* nodep) { iterate(nodep); }
    ~DescopeVisitor() override = افتراضي;
};

//######################################################################
// Descope class functions

فراغ V3Descope::descopeAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { DescopeVisitor{nodep}; }  // Destruct before checking
    V3ConstPool::setDescoped();
    V3Global::dump_Check_Global_شجرة("descope", 0, dump_شجرة_Either_Level() >= 3);
}
