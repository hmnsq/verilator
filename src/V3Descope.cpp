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

#ଅନ୍ତର୍ଭାଵ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ଅନ୍ତର୍ଭାଵ "V3Descope.h"

#ଅନ୍ତର୍ଭାଵ "V3ConstPool.h"
#ଅନ୍ତର୍ଭାଵ "V3EmitCBase.h"

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################

ଶ୍ରେଣୀ DescopeVisitor final : ସର୍ବସାଧାରଣ VNVisitor {
    // NODE STATE
    //  Cleared entire netlist
    //   AstCFunc::user()               // bool.  Indicates processing completed
    ଅଚଳ VNUser1InUse m_inuser1;

    // TYPES
    using FuncMmap = std::multimap<std::string, AstCFunc*>;
    ସଂରଚନା ScopeSelfPtr final {
        VSelfPointerText thisPtr = VSelfPointerText{VSelfPointerText::Empty()};
        VSelfPointerText vlSymsPtr = VSelfPointerText{VSelfPointerText::Empty()};
    };

    // STATE
    Ast_ଗଣ୍ଠି_Module* m_modp = nullptr;  // Current module
    AstScope* m_scopep = nullptr;  // Current scope
    ଅଚଳ AstCFunc* m_funcp = nullptr;  // Current function
    ତାର୍କିକ m_modSingleton = ମିଥ୍ୟା;  // m_modp is only instantiated once
    FuncMmap m_modFuncs;  // Name of public functions added
    std::map<ଅଚଳ AstScope*, ScopeSelfPtr> m_scopeToSelf;  // Scope to self pointers

    // METHODS

    ସ୍ଥିର ତାର୍କିକ modIsSingleton(ଅଚଳ Ast_ଗଣ୍ଠି_Module* modp) {
        // True iff there's exactly one instance of this module in the design (including top).
        ଯଦି (modp->is_ଶୀର୍ଷ()) ଫେରନ୍ତୁ ସତ୍ୟ;
        ପୂର୍ଣ୍ଣସଂଖ୍ୟା instances = 0;
        ପାଇଁ (ଅଚଳ Ast_ଗଣ୍ଠି* stmtp = modp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            ଯଦି (VN_IS(stmtp, Scope)) {
                ଯଦି (++instances > 1) ଫେରନ୍ତୁ ମିଥ୍ୟା;
            }
        }
        ଫେରନ୍ତୁ (instances == 1);
    }

    // Construct a 'this' self pointer for the given scope
    VSelfPointerText scopeThis(ଅଚଳ AstScope* scopep) {
        auto& ret = m_scopeToSelf[scopep];
        ଯଦି (ret.thisPtr.isEmpty()) {
            string name = scopep->name();
            string::size_type pos = name.rfind('.');
            ଯଦି (pos != string::npos) name.erase(0, pos + 1);
            ret.thisPtr = VSelfPointerText{VSelfPointerText::This{}, name};
        }
        ଫେରନ୍ତୁ ret.thisPtr;
    }
    // Construct a 'vlSyms' self pointer for the given scope
    VSelfPointerText scopeVlSyms(ଅଚଳ AstScope* scopep) {
        auto& ret = m_scopeToSelf[scopep];
        ଯଦି (ret.vlSymsPtr.isEmpty()) {
            ret.vlSymsPtr = VSelfPointerText{VSelfPointerText::VlSyms(), scopep->nameDotless()};
        }
        ଫେରନ୍ତୁ ret.vlSymsPtr;
    }

    // Construct the best self pointer to reference an object in 'scopep'  from a CFunc in
    // 'm_scopep'. Result may be relative ("this->[...]") or absolute ("vlSyms->[...]").
    //
    // Using relative references allows V3Combine'ing code across multiple instances of the same
    // module.
    VSelfPointerText descopedSelfPointer(ଅଚଳ AstScope* scopep) {
        UASSERT(scopep, "Var/Func not scoped");
        // Static functions can't use relative references via 'this->'
        ଅଚଳ ତାର୍କିକ relativeRefOk = !m_funcp->isStatic();

        UINFO(8, "      Descope ref under " << m_scopep);
        UINFO(8, "              ref to    " << scopep);
        UINFO(8, "             aboveScope " << scopep->aboveScopep());

        ଯଦି (VN_IS(scopep->modp(), Class)) {
            // Direct reference to class members are from within the class itself, references from
            // outside the class must go via AstMemberSel
            ଫେରନ୍ତୁ VSelfPointerText{VSelfPointerText::This{}};
        } ଅନ୍ୟଥା ଯଦି (relativeRefOk && scopep == m_scopep) {
            ଫେରନ୍ତୁ VSelfPointerText{VSelfPointerText::This{}};
        } ଅନ୍ୟଥା ଯଦି (relativeRefOk && !m_modSingleton && scopep->aboveScopep() == m_scopep
                   && VN_IS(scopep->modp(), Module)) {
            // Reference to scope of instance directly under this module, can just "this->cell",
            // which can potentially be V3Combined, but note this requires one extra pointer
            // dereference which is slower, so we only use it if the source scope is not a
            // singleton.
            ଫେରନ୍ତୁ scopeThis(scopep);
        } ଅନ୍ୟଥା {
            // Reference to something elsewhere, or relative references are disabled. Use global
            // variable
            ଫେରନ୍ତୁ scopeVlSyms(scopep);
        }
    }

    ଶୂନ୍ୟ makePublicFuncWrappers() {
        // We recorded all public functions in m_modFuncs.
        // If for any given name only one function exists, we can use that function directly.
        // If multiple functions exist, we need to select the appropriate scope.
        ପାଇଁ (FuncMmap::iterator it = m_modFuncs.begin(); it != m_modFuncs.end(); ++it) {
            ଅଚଳ string name = it->first;
            AstCFunc* ଅଚଳ ଶୀର୍ଷ_Funcp = it->second;
            auto nextIt1 = it;
            ++nextIt1;
            ଅଚଳ ତାର୍କିକ moreOfSame1 = (nextIt1 != m_modFuncs.end() && nextIt1->first == name);
            ଯଦି (moreOfSame1) {
                // Multiple functions under this name, need a wrapper function
                UINFO(6, "  Wrapping " << name << " multifuncs");
                AstCFunc* ଅଚଳ newfuncp = ଶୀର୍ଷ_Funcp->clone_ବୃକ୍ଷ(ମିଥ୍ୟା);
                ଯଦି (newfuncp->varsp()) newfuncp->varsp()->unlinkFrBackWithNext()->delete_ବୃକ୍ଷ();
                ଯଦି (newfuncp->stmtsp()) newfuncp->stmtsp()->unlinkFrBackWithNext()->delete_ବୃକ୍ଷ();
                newfuncp->name(name);
                newfuncp->isStatic(ମିଥ୍ୟା);
                ଶୀର୍ଷ_Funcp->addNextHere(newfuncp);
                // In the body, call each function if it matches the given scope
                ପାଇଁ (FuncMmap::iterator eachIt = it;
                     eachIt != m_modFuncs.end() && eachIt->first == name; ++eachIt) {
                    it = eachIt;
                    AstCFunc* ଅଚଳ funcp = eachIt->second;
                    auto nextIt2 = eachIt;
                    ++nextIt2;
                    ଅଚଳ ତାର୍କିକ moreOfSame
                        = (nextIt2 != m_modFuncs.end() && nextIt2->first == name);
                    UASSERT_OBJ(funcp->scopep(), funcp, "Not scoped");

                    UINFO(6, "     Wrapping " << name << " " << funcp);
                    UINFO(6, "  at " << newfuncp->argTypes() << " und " << funcp->argTypes());
                    funcp->declPrivate(ସତ୍ୟ);
                    AstVarRef* argsp = nullptr;
                    ପାଇଁ (Ast_ଗଣ୍ଠି* stmtp = newfuncp->argsp(); stmtp; stmtp = stmtp->nextp()) {
                        ଯଦି (AstVar* ଅଚଳ portp = VN_CAST(stmtp, Var)) {
                            ଯଦି (portp->isIO() && !portp->isFuncReturn()) {
                                AstVarRef* ଅଚଳ newp = ନୂତନ AstVarRef{
                                    portp->fileline(), portp, portp->direction().pinAccess()};
                                argsp = Ast_ଗଣ୍ଠି::addNext(argsp, newp);
                            }
                        }
                    }

                    AstCCall* ଅଚଳ callp = ନୂତନ AstCCall{funcp->fileline(), funcp, argsp};
                    ଯଦି (Ast_ଗଣ୍ଠି_DType* ଅଚଳ dtypep = funcp->dtypep()) {
                        callp->dtypep(dtypep);
                    } ଅନ୍ୟଥା {
                        callp->dtypeSetVoid();
                    }
                    Ast_ଗଣ୍ଠି* ଅଚଳ returnp = ନୂତନ AstCReturn{funcp->fileline(), callp};

                    ଯଦି (moreOfSame) {
                        AstIf* ଅଚଳ ifp = ନୂତନ AstIf{
                            funcp->fileline(),
                            ନୂତନ AstEq{funcp->fileline(),
                                      ନୂତନ AstCExpr{funcp->fileline(), "this", 64},
                                      ନୂତନ AstCExpr{funcp->fileline(),
                                                   scopeVlSyms(funcp->scopep()).asString(), 64}},
                            returnp};
                        newfuncp->addStmtsp(ifp);
                    } ଅନ୍ୟଥା {
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
            } ଅନ୍ୟଥା {
                // Only a single function under this name, we can rename it
                UINFO(6, "  Wrapping " << name << " just one " << ଶୀର୍ଷ_Funcp);
                ଶୀର୍ଷ_Funcp->name(name);
            }
        }
    }

    // VISITORS
    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି_Module* nodep) override {
        VL_RESTORER(m_modp);
        m_modp = nodep;
        m_modFuncs.clear();
        m_modSingleton = modIsSingleton(m_modp);
        iterateChildren(nodep);
        makePublicFuncWrappers();
    }
    ଶୂନ୍ୟ visit(AstScope* nodep) override {
        VL_RESTORER(m_scopep);
        m_scopep = nodep;
        iterateChildren(nodep);
    }
    ଶୂନ୍ୟ visit(AstVarScope* nodep) override {
        // Delete the varscope when we're finished
        VL_DO_DANGLING(ଠେଲ_Deletep(nodep->unlinkFrBack()), nodep);
    }
    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି_Var_Ref* nodep) override {
        iterateChildren(nodep);
        ଯଦି (!nodep->varScopep()) {
            UASSERT_OBJ(nodep->varp()->isFuncLocal(), nodep,
                        "unscoped reference can only appear to function locals at this point");
            ଫେରନ୍ତୁ;
        }
        // Convert the hierch name
        UINFO(9, "  ref-in " << nodep);
        UASSERT_OBJ(m_scopep, nodep, "Node not under scope");
        ଅଚଳ AstVar* ଅଚଳ varp = nodep->varScopep()->varp();
        ଅଚଳ AstScope* ଅଚଳ scopep = nodep->varScopep()->scopep();
        ଯଦି (varp->isFuncLocal()) {
            // Reference to function locals need no self pointer
            nodep->selfPointer(VSelfPointerText{VSelfPointerText::Empty()});
        } ଅନ୍ୟଥା ଯଦି (scopep->modp()->isConstPool()) {
            // Reference to constant pool value need no self pointer
            nodep->selfPointer(VSelfPointerText{VSelfPointerText::Empty()});
        } ଅନ୍ୟଥା {
            nodep->selfPointer(descopedSelfPointer(scopep));
        }
        nodep->varScopep(nullptr);
        UINFO(9, "  refout " << nodep << " selfPtr=" << nodep->selfPointer().asString());
    }
    ଶୂନ୍ୟ visit(AstCCall* nodep) override {
        // UINFO(9, "       " << nodep);
        iterateChildren(nodep);
        // Convert the hierch name
        UASSERT_OBJ(m_scopep, nodep, "Node not under scope");
        ଅଚଳ AstScope* ଅଚଳ scopep = nodep->funcp()->scopep();
        nodep->selfPointer(descopedSelfPointer(scopep));
        // Can't do this, as we may have more calls later
        // nodep->funcp()->scopep(nullptr);
    }
    ଶୂନ୍ୟ visit(AstScopeName* nodep) override {
        // Print a %m under this instance as its vlNamep, which holds its pretty name, plus the
        // rest of the path, so the instances' code can be V3Combined. Not with --protect-ids, as
        // vlNamep then holds the protected name, while %m prints the original.
        ଯଦି (!m_funcp || !nodep->forFormat() || m_modSingleton || m_funcp->isStatic()
            || v3Global.opt.protectIds()) {
            ଫେରନ୍ତୁ;
        }
        std::string path = nodep->scopePrettySymName();
        ଅଚଳ std::string inst = m_scopep->prettyName();
        ଯଦି (path != inst && !VString::startsWith(path, inst + ".")) ଫେରନ୍ତୁ;
        path.erase(0, inst.size() + 1);
        // The DPI scope table keeps the absolute name, via a copy outside any function
        m_scopep->addBlocksp(nodep->clone_ବୃକ୍ଷ(ମିଥ୍ୟା));
        nodep->scopeAttr(path);
        nodep->scopeEntr("");  // Only used by the DPI scope table
        nodep->selfRelative(ସତ୍ୟ);
    }
    ଶୂନ୍ୟ visit(AstCMethodCall* nodep) override { iterateChildren(nodep); }
    ଶୂନ୍ୟ visit(AstCNew* nodep) override { iterateChildren(nodep); }
    ଶୂନ୍ୟ visit(AstCFunc* nodep) override {
        VL_RESTORER(m_funcp);
        ଯଦି (!nodep->user1()) {
            // Static functions should have been moved under the corresponding AstClassPackage
            UASSERT_OBJ(!(nodep->isStatic() && VN_IS(m_modp, Class)), nodep,
                        "Static function under AstClass");
            m_funcp = nodep;
            iterateChildren(nodep);
            nodep->user1(ସତ୍ୟ);
            // If it's under a scope, move it up to the top
            ଯଦି (m_scopep) {
                nodep->unlinkFrBack();
                m_modp->addStmtsp(nodep);

                ଯଦି (nodep->funcPublic()) {
                    // There may be multiple public functions by the same name;
                    // record for later correction or making of shells
                    m_modFuncs.emplace(nodep->name(), nodep);
                    nodep->name(m_scopep->nameDotless() + "__" + nodep->name());
                }
            }
        }
    }
    ଶୂନ୍ୟ visit(AstVar*) override {}
    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି* nodep) override { iterateChildren(nodep); }

ସର୍ବସାଧାରଣ:
    // CONSTRUCTORS
    explicit DescopeVisitor(AstNetlist* nodep) { iterate(nodep); }
    ~DescopeVisitor() override = ପୂର୍ବନିର୍ଦ୍ଧାରିତ;
};

//######################################################################
// Descope class functions

ଶୂନ୍ୟ V3Descope::descopeAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { DescopeVisitor{nodep}; }  // Destruct before checking
    V3ConstPool::setDescoped();
    V3Global::dump_Check_Global_ବୃକ୍ଷ("descope", 0, dump_ବୃକ୍ଷ_Either_Level() >= 3);
}
