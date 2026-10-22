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

#অন্তর্ভাৱ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#অন্তর্ভাৱ "V3Descope.h"

#অন্তর্ভাৱ "V3ConstPool.h"
#অন্তর্ভাৱ "V3EmitCBase.h"

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################

শ্ৰেণী DescopeVisitor final : সার্ৱজনিক VNVisitor {
    // NODE STATE
    //  Cleared entire netlist
    //   AstCFunc::user()               // bool.  Indicates processing completed
    ধ্রুৱ VNUser1InUse m_inuser1;

    // TYPES
    using FuncMmap = std::multimap<std::string, AstCFunc*>;
    সংরচনা ScopeSelfPtr final {
        VSelfPointerText thisPtr = VSelfPointerText{VSelfPointerText::Empty()};
        VSelfPointerText vlSymsPtr = VSelfPointerText{VSelfPointerText::Empty()};
    };

    // STATE
    Ast_গ্ৰন্থি_Module* m_modp = nullptr;  // Current module
    AstScope* m_scopep = nullptr;  // Current scope
    ধ্রুৱ AstCFunc* m_funcp = nullptr;  // Current function
    তার্কিক m_modSingleton = মিছা;  // m_modp is only instantiated once
    FuncMmap m_modFuncs;  // Name of public functions added
    std::map<ধ্রুৱ AstScope*, ScopeSelfPtr> m_scopeToSelf;  // Scope to self pointers

    // METHODS

    স্থির তার্কিক modIsSingleton(ধ্রুৱ Ast_গ্ৰন্থি_Module* modp) {
        // True iff there's exactly one instance of this module in the design (including top).
        যদি (modp->is_শীৰ্ষ()) ঘূৰাই সঁচা;
        পূর্ণাঙ্ক instances = 0;
        বাবে (ধ্রুৱ Ast_গ্ৰন্থি* stmtp = modp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            যদি (VN_IS(stmtp, Scope)) {
                যদি (++instances > 1) ঘূৰাই মিছা;
            }
        }
        ঘূৰাই (instances == 1);
    }

    // Construct a 'this' self pointer for the given scope
    VSelfPointerText scopeThis(ধ্রুৱ AstScope* scopep) {
        auto& ret = m_scopeToSelf[scopep];
        যদি (ret.thisPtr.isEmpty()) {
            string name = scopep->name();
            string::size_type pos = name.rfind('.');
            যদি (pos != string::npos) name.erase(0, pos + 1);
            ret.thisPtr = VSelfPointerText{VSelfPointerText::This{}, name};
        }
        ঘূৰাই ret.thisPtr;
    }
    // Construct a 'vlSyms' self pointer for the given scope
    VSelfPointerText scopeVlSyms(ধ্রুৱ AstScope* scopep) {
        auto& ret = m_scopeToSelf[scopep];
        যদি (ret.vlSymsPtr.isEmpty()) {
            ret.vlSymsPtr = VSelfPointerText{VSelfPointerText::VlSyms(), scopep->nameDotless()};
        }
        ঘূৰাই ret.vlSymsPtr;
    }

    // Construct the best self pointer to reference an object in 'scopep'  from a CFunc in
    // 'm_scopep'. Result may be relative ("this->[...]") or absolute ("vlSyms->[...]").
    //
    // Using relative references allows V3Combine'ing code across multiple instances of the same
    // module.
    VSelfPointerText descopedSelfPointer(ধ্রুৱ AstScope* scopep) {
        UASSERT(scopep, "Var/Func not scoped");
        // Static functions can't use relative references via 'this->'
        ধ্রুৱ তার্কিক relativeRefOk = !m_funcp->isStatic();

        UINFO(8, "      Descope ref under " << m_scopep);
        UINFO(8, "              ref to    " << scopep);
        UINFO(8, "             aboveScope " << scopep->aboveScopep());

        যদি (VN_IS(scopep->modp(), Class)) {
            // Direct reference to class members are from within the class itself, references from
            // outside the class must go via AstMemberSel
            ঘূৰাই VSelfPointerText{VSelfPointerText::This{}};
        } অন্যথা যদি (relativeRefOk && scopep == m_scopep) {
            ঘূৰাই VSelfPointerText{VSelfPointerText::This{}};
        } অন্যথা যদি (relativeRefOk && !m_modSingleton && scopep->aboveScopep() == m_scopep
                   && VN_IS(scopep->modp(), Module)) {
            // Reference to scope of instance directly under this module, can just "this->cell",
            // which can potentially be V3Combined, but note this requires one extra pointer
            // dereference which is slower, so we only use it if the source scope is not a
            // singleton.
            ঘূৰাই scopeThis(scopep);
        } অন্যথা {
            // Reference to something elsewhere, or relative references are disabled. Use global
            // variable
            ঘূৰাই scopeVlSyms(scopep);
        }
    }

    শূন্য makePublicFuncWrappers() {
        // We recorded all public functions in m_modFuncs.
        // If for any given name only one function exists, we can use that function directly.
        // If multiple functions exist, we need to select the appropriate scope.
        বাবে (FuncMmap::iterator it = m_modFuncs.begin(); it != m_modFuncs.end(); ++it) {
            ধ্রুৱ string name = it->first;
            AstCFunc* ধ্রুৱ শীৰ্ষ_Funcp = it->second;
            auto nextIt1 = it;
            ++nextIt1;
            ধ্রুৱ তার্কিক moreOfSame1 = (nextIt1 != m_modFuncs.end() && nextIt1->first == name);
            যদি (moreOfSame1) {
                // Multiple functions under this name, need a wrapper function
                UINFO(6, "  Wrapping " << name << " multifuncs");
                AstCFunc* ধ্রুৱ newfuncp = শীৰ্ষ_Funcp->clone_বৃক্ষ(মিছা);
                যদি (newfuncp->varsp()) newfuncp->varsp()->unlinkFrBackWithNext()->delete_বৃক্ষ();
                যদি (newfuncp->stmtsp()) newfuncp->stmtsp()->unlinkFrBackWithNext()->delete_বৃক্ষ();
                newfuncp->name(name);
                newfuncp->isStatic(মিছা);
                শীৰ্ষ_Funcp->addNextHere(newfuncp);
                // In the body, call each function if it matches the given scope
                বাবে (FuncMmap::iterator eachIt = it;
                     eachIt != m_modFuncs.end() && eachIt->first == name; ++eachIt) {
                    it = eachIt;
                    AstCFunc* ধ্রুৱ funcp = eachIt->second;
                    auto nextIt2 = eachIt;
                    ++nextIt2;
                    ধ্রুৱ তার্কিক moreOfSame
                        = (nextIt2 != m_modFuncs.end() && nextIt2->first == name);
                    UASSERT_OBJ(funcp->scopep(), funcp, "Not scoped");

                    UINFO(6, "     Wrapping " << name << " " << funcp);
                    UINFO(6, "  at " << newfuncp->argTypes() << " und " << funcp->argTypes());
                    funcp->declPrivate(সঁচা);
                    AstVarRef* argsp = nullptr;
                    বাবে (Ast_গ্ৰন্থি* stmtp = newfuncp->argsp(); stmtp; stmtp = stmtp->nextp()) {
                        যদি (AstVar* ধ্রুৱ portp = VN_CAST(stmtp, Var)) {
                            যদি (portp->isIO() && !portp->isFuncReturn()) {
                                AstVarRef* ধ্রুৱ newp = নূতন AstVarRef{
                                    portp->fileline(), portp, portp->direction().pinAccess()};
                                argsp = Ast_গ্ৰন্থি::addNext(argsp, newp);
                            }
                        }
                    }

                    AstCCall* ধ্রুৱ callp = নূতন AstCCall{funcp->fileline(), funcp, argsp};
                    যদি (Ast_গ্ৰন্থি_DType* ধ্রুৱ dtypep = funcp->dtypep()) {
                        callp->dtypep(dtypep);
                    } অন্যথা {
                        callp->dtypeSetVoid();
                    }
                    Ast_গ্ৰন্থি* ধ্রুৱ returnp = নূতন AstCReturn{funcp->fileline(), callp};

                    যদি (moreOfSame) {
                        AstIf* ধ্রুৱ ifp = নূতন AstIf{
                            funcp->fileline(),
                            নূতন AstEq{funcp->fileline(),
                                      নূতন AstCExpr{funcp->fileline(), "this", 64},
                                      নূতন AstCExpr{funcp->fileline(),
                                                   scopeVlSyms(funcp->scopep()).asString(), 64}},
                            returnp};
                        newfuncp->addStmtsp(ifp);
                    } অন্যথা {
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
            } অন্যথা {
                // Only a single function under this name, we can rename it
                UINFO(6, "  Wrapping " << name << " just one " << শীৰ্ষ_Funcp);
                শীৰ্ষ_Funcp->name(name);
            }
        }
    }

    // VISITORS
    শূন্য visit(Ast_গ্ৰন্থি_Module* nodep) override {
        VL_RESTORER(m_modp);
        m_modp = nodep;
        m_modFuncs.clear();
        m_modSingleton = modIsSingleton(m_modp);
        iterateChildren(nodep);
        makePublicFuncWrappers();
    }
    শূন্য visit(AstScope* nodep) override {
        VL_RESTORER(m_scopep);
        m_scopep = nodep;
        iterateChildren(nodep);
    }
    শূন্য visit(AstVarScope* nodep) override {
        // Delete the varscope when we're finished
        VL_DO_DANGLING(ঠেলো_Deletep(nodep->unlinkFrBack()), nodep);
    }
    শূন্য visit(Ast_গ্ৰন্থি_Var_Ref* nodep) override {
        iterateChildren(nodep);
        যদি (!nodep->varScopep()) {
            UASSERT_OBJ(nodep->varp()->isFuncLocal(), nodep,
                        "unscoped reference can only appear to function locals at this point");
            ঘূৰাই;
        }
        // Convert the hierch name
        UINFO(9, "  ref-in " << nodep);
        UASSERT_OBJ(m_scopep, nodep, "Node not under scope");
        ধ্রুৱ AstVar* ধ্রুৱ varp = nodep->varScopep()->varp();
        ধ্রুৱ AstScope* ধ্রুৱ scopep = nodep->varScopep()->scopep();
        যদি (varp->isFuncLocal()) {
            // Reference to function locals need no self pointer
            nodep->selfPointer(VSelfPointerText{VSelfPointerText::Empty()});
        } অন্যথা যদি (scopep->modp()->isConstPool()) {
            // Reference to constant pool value need no self pointer
            nodep->selfPointer(VSelfPointerText{VSelfPointerText::Empty()});
        } অন্যথা {
            nodep->selfPointer(descopedSelfPointer(scopep));
        }
        nodep->varScopep(nullptr);
        UINFO(9, "  refout " << nodep << " selfPtr=" << nodep->selfPointer().asString());
    }
    শূন্য visit(AstCCall* nodep) override {
        // UINFO(9, "       " << nodep);
        iterateChildren(nodep);
        // Convert the hierch name
        UASSERT_OBJ(m_scopep, nodep, "Node not under scope");
        ধ্রুৱ AstScope* ধ্রুৱ scopep = nodep->funcp()->scopep();
        nodep->selfPointer(descopedSelfPointer(scopep));
        // Can't do this, as we may have more calls later
        // nodep->funcp()->scopep(nullptr);
    }
    শূন্য visit(AstScopeName* nodep) override {
        // Print a %m under this instance as its vlNamep, which holds its pretty name, plus the
        // rest of the path, so the instances' code can be V3Combined. Not with --protect-ids, as
        // vlNamep then holds the protected name, while %m prints the original.
        যদি (!m_funcp || !nodep->forFormat() || m_modSingleton || m_funcp->isStatic()
            || v3Global.opt.protectIds()) {
            ঘূৰাই;
        }
        std::string path = nodep->scopePrettySymName();
        ধ্রুৱ std::string inst = m_scopep->prettyName();
        যদি (path != inst && !VString::startsWith(path, inst + ".")) ঘূৰাই;
        path.erase(0, inst.size() + 1);
        // The DPI scope table keeps the absolute name, via a copy outside any function
        m_scopep->addBlocksp(nodep->clone_বৃক্ষ(মিছা));
        nodep->scopeAttr(path);
        nodep->scopeEntr("");  // Only used by the DPI scope table
        nodep->selfRelative(সঁচা);
    }
    শূন্য visit(AstCMethodCall* nodep) override { iterateChildren(nodep); }
    শূন্য visit(AstCNew* nodep) override { iterateChildren(nodep); }
    শূন্য visit(AstCFunc* nodep) override {
        VL_RESTORER(m_funcp);
        যদি (!nodep->user1()) {
            // Static functions should have been moved under the corresponding AstClassPackage
            UASSERT_OBJ(!(nodep->isStatic() && VN_IS(m_modp, Class)), nodep,
                        "Static function under AstClass");
            m_funcp = nodep;
            iterateChildren(nodep);
            nodep->user1(সঁচা);
            // If it's under a scope, move it up to the top
            যদি (m_scopep) {
                nodep->unlinkFrBack();
                m_modp->addStmtsp(nodep);

                যদি (nodep->funcPublic()) {
                    // There may be multiple public functions by the same name;
                    // record for later correction or making of shells
                    m_modFuncs.emplace(nodep->name(), nodep);
                    nodep->name(m_scopep->nameDotless() + "__" + nodep->name());
                }
            }
        }
    }
    শূন্য visit(AstVar*) override {}
    শূন্য visit(Ast_গ্ৰন্থি* nodep) override { iterateChildren(nodep); }

সার্ৱজনিক:
    // CONSTRUCTORS
    explicit DescopeVisitor(AstNetlist* nodep) { iterate(nodep); }
    ~DescopeVisitor() override = স্ৱভাৱ;
};

//######################################################################
// Descope class functions

শূন্য V3Descope::descopeAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { DescopeVisitor{nodep}; }  // Destruct before checking
    V3ConstPool::setDescoped();
    V3Global::dump_Check_Global_বৃক্ষ("descope", 0, dump_বৃক্ষ_Either_Level() >= 3);
}
