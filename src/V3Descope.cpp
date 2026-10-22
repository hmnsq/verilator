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

#अन्तर्भाव "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#अन्तर्भाव "V3Descope.h"

#अन्तर्भाव "V3ConstPool.h"
#अन्तर्भाव "V3EmitCBase.h"

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################

वर्ग DescopeVisitor final : सार्वजनिक VNVisitor {
    // NODE STATE
    //  Cleared entire netlist
    //   AstCFunc::user()               // bool.  Indicates processing completed
    स्थिरांक VNUser1InUse m_inuser1;

    // TYPES
    using FuncMmap = std::multimap<std::string, AstCFunc*>;
    संरचना ScopeSelfPtr final {
        VSelfPointerText thisPtr = VSelfPointerText{VSelfPointerText::Empty()};
        VSelfPointerText vlSymsPtr = VSelfPointerText{VSelfPointerText::Empty()};
    };

    // STATE
    Ast_गाठ_Module* m_modp = nullptr;  // Current module
    AstScope* m_scopep = nullptr;  // Current scope
    स्थिरांक AstCFunc* m_funcp = nullptr;  // Current function
    तार्किक m_modSingleton = असत्य;  // m_modp is only instantiated once
    FuncMmap m_modFuncs;  // Name of public functions added
    std::map<स्थिरांक AstScope*, ScopeSelfPtr> m_scopeToSelf;  // Scope to self pointers

    // METHODS

    स्थिर तार्किक modIsSingleton(स्थिरांक Ast_गाठ_Module* modp) {
        // True iff there's exactly one instance of this module in the design (including top).
        जर (modp->is_शीर्ष()) परत सत्य;
        पूर्णांक instances = 0;
        साठी (स्थिरांक Ast_गाठ* stmtp = modp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            जर (VN_IS(stmtp, Scope)) {
                जर (++instances > 1) परत असत्य;
            }
        }
        परत (instances == 1);
    }

    // Construct a 'this' self pointer for the given scope
    VSelfPointerText scopeThis(स्थिरांक AstScope* scopep) {
        स्वतः& ret = m_scopeToSelf[scopep];
        जर (ret.thisPtr.isEmpty()) {
            string name = scopep->name();
            string::size_type pos = name.rfind('.');
            जर (pos != string::npos) name.erase(0, pos + 1);
            ret.thisPtr = VSelfPointerText{VSelfPointerText::This{}, name};
        }
        परत ret.thisPtr;
    }
    // Construct a 'vlSyms' self pointer for the given scope
    VSelfPointerText scopeVlSyms(स्थिरांक AstScope* scopep) {
        स्वतः& ret = m_scopeToSelf[scopep];
        जर (ret.vlSymsPtr.isEmpty()) {
            ret.vlSymsPtr = VSelfPointerText{VSelfPointerText::VlSyms(), scopep->nameDotless()};
        }
        परत ret.vlSymsPtr;
    }

    // Construct the best self pointer to reference an object in 'scopep'  from a CFunc in
    // 'm_scopep'. Result may be relative ("this->[...]") or absolute ("vlSyms->[...]").
    //
    // Using relative references allows V3Combine'ing code across multiple instances of the same
    // module.
    VSelfPointerText descopedSelfPointer(स्थिरांक AstScope* scopep) {
        UASSERT(scopep, "Var/Func not scoped");
        // Static functions can't use relative references via 'this->'
        स्थिरांक तार्किक relativeRefOk = !m_funcp->isStatic();

        UINFO(8, "      Descope ref under " << m_scopep);
        UINFO(8, "              ref to    " << scopep);
        UINFO(8, "             aboveScope " << scopep->aboveScopep());

        जर (VN_IS(scopep->modp(), Class)) {
            // Direct reference to class members are from within the class itself, references from
            // outside the class must go via AstMemberSel
            परत VSelfPointerText{VSelfPointerText::This{}};
        } अन्यथा जर (relativeRefOk && scopep == m_scopep) {
            परत VSelfPointerText{VSelfPointerText::This{}};
        } अन्यथा जर (relativeRefOk && !m_modSingleton && scopep->aboveScopep() == m_scopep
                   && VN_IS(scopep->modp(), Module)) {
            // Reference to scope of instance directly under this module, can just "this->cell",
            // which can potentially be V3Combined, but note this requires one extra pointer
            // dereference which is slower, so we only use it if the source scope is not a
            // singleton.
            परत scopeThis(scopep);
        } अन्यथा {
            // Reference to something elsewhere, or relative references are disabled. Use global
            // variable
            परत scopeVlSyms(scopep);
        }
    }

    शून्य makePublicFuncWrappers() {
        // We recorded all public functions in m_modFuncs.
        // If for any given name only one function exists, we can use that function directly.
        // If multiple functions exist, we need to select the appropriate scope.
        साठी (FuncMmap::iterator it = m_modFuncs.begin(); it != m_modFuncs.end(); ++it) {
            स्थिरांक string name = it->first;
            AstCFunc* स्थिरांक शीर्ष_Funcp = it->second;
            स्वतः nextIt1 = it;
            ++nextIt1;
            स्थिरांक तार्किक moreOfSame1 = (nextIt1 != m_modFuncs.end() && nextIt1->first == name);
            जर (moreOfSame1) {
                // Multiple functions under this name, need a wrapper function
                UINFO(6, "  Wrapping " << name << " multifuncs");
                AstCFunc* स्थिरांक newfuncp = शीर्ष_Funcp->clone_वृक्ष(असत्य);
                जर (newfuncp->varsp()) newfuncp->varsp()->unlinkFrBackWithNext()->delete_वृक्ष();
                जर (newfuncp->stmtsp()) newfuncp->stmtsp()->unlinkFrBackWithNext()->delete_वृक्ष();
                newfuncp->name(name);
                newfuncp->isStatic(असत्य);
                शीर्ष_Funcp->addNextHere(newfuncp);
                // In the body, call each function if it matches the given scope
                साठी (FuncMmap::iterator eachIt = it;
                     eachIt != m_modFuncs.end() && eachIt->first == name; ++eachIt) {
                    it = eachIt;
                    AstCFunc* स्थिरांक funcp = eachIt->second;
                    स्वतः nextIt2 = eachIt;
                    ++nextIt2;
                    स्थिरांक तार्किक moreOfSame
                        = (nextIt2 != m_modFuncs.end() && nextIt2->first == name);
                    UASSERT_OBJ(funcp->scopep(), funcp, "Not scoped");

                    UINFO(6, "     Wrapping " << name << " " << funcp);
                    UINFO(6, "  at " << newfuncp->argTypes() << " und " << funcp->argTypes());
                    funcp->declPrivate(सत्य);
                    AstVarRef* argsp = nullptr;
                    साठी (Ast_गाठ* stmtp = newfuncp->argsp(); stmtp; stmtp = stmtp->nextp()) {
                        जर (AstVar* स्थिरांक portp = VN_CAST(stmtp, Var)) {
                            जर (portp->isIO() && !portp->isFuncReturn()) {
                                AstVarRef* स्थिरांक newp = नवीन AstVarRef{
                                    portp->fileline(), portp, portp->direction().pinAccess()};
                                argsp = Ast_गाठ::addNext(argsp, newp);
                            }
                        }
                    }

                    AstCCall* स्थिरांक callp = नवीन AstCCall{funcp->fileline(), funcp, argsp};
                    जर (Ast_गाठ_DType* स्थिरांक dtypep = funcp->dtypep()) {
                        callp->dtypep(dtypep);
                    } अन्यथा {
                        callp->dtypeSetVoid();
                    }
                    Ast_गाठ* स्थिरांक returnp = नवीन AstCReturn{funcp->fileline(), callp};

                    जर (moreOfSame) {
                        AstIf* स्थिरांक ifp = नवीन AstIf{
                            funcp->fileline(),
                            नवीन AstEq{funcp->fileline(),
                                      नवीन AstCExpr{funcp->fileline(), "this", 64},
                                      नवीन AstCExpr{funcp->fileline(),
                                                   scopeVlSyms(funcp->scopep()).asString(), 64}},
                            returnp};
                        newfuncp->addStmtsp(ifp);
                    } अन्यथा {
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
            } अन्यथा {
                // Only a single function under this name, we can rename it
                UINFO(6, "  Wrapping " << name << " just one " << शीर्ष_Funcp);
                शीर्ष_Funcp->name(name);
            }
        }
    }

    // VISITORS
    शून्य visit(Ast_गाठ_Module* nodep) override {
        VL_RESTORER(m_modp);
        m_modp = nodep;
        m_modFuncs.clear();
        m_modSingleton = modIsSingleton(m_modp);
        iterateChildren(nodep);
        makePublicFuncWrappers();
    }
    शून्य visit(AstScope* nodep) override {
        VL_RESTORER(m_scopep);
        m_scopep = nodep;
        iterateChildren(nodep);
    }
    शून्य visit(AstVarScope* nodep) override {
        // Delete the varscope when we're finished
        VL_DO_DANGLING(ढकला_Deletep(nodep->unlinkFrBack()), nodep);
    }
    शून्य visit(Ast_गाठ_Var_Ref* nodep) override {
        iterateChildren(nodep);
        जर (!nodep->varScopep()) {
            UASSERT_OBJ(nodep->varp()->isFuncLocal(), nodep,
                        "unscoped reference can only appear to function locals at this point");
            परत;
        }
        // Convert the hierch name
        UINFO(9, "  ref-in " << nodep);
        UASSERT_OBJ(m_scopep, nodep, "Node not under scope");
        स्थिरांक AstVar* स्थिरांक varp = nodep->varScopep()->varp();
        स्थिरांक AstScope* स्थिरांक scopep = nodep->varScopep()->scopep();
        जर (varp->isFuncLocal()) {
            // Reference to function locals need no self pointer
            nodep->selfPointer(VSelfPointerText{VSelfPointerText::Empty()});
        } अन्यथा जर (scopep->modp()->isConstPool()) {
            // Reference to constant pool value need no self pointer
            nodep->selfPointer(VSelfPointerText{VSelfPointerText::Empty()});
        } अन्यथा {
            nodep->selfPointer(descopedSelfPointer(scopep));
        }
        nodep->varScopep(nullptr);
        UINFO(9, "  refout " << nodep << " selfPtr=" << nodep->selfPointer().asString());
    }
    शून्य visit(AstCCall* nodep) override {
        // UINFO(9, "       " << nodep);
        iterateChildren(nodep);
        // Convert the hierch name
        UASSERT_OBJ(m_scopep, nodep, "Node not under scope");
        स्थिरांक AstScope* स्थिरांक scopep = nodep->funcp()->scopep();
        nodep->selfPointer(descopedSelfPointer(scopep));
        // Can't do this, as we may have more calls later
        // nodep->funcp()->scopep(nullptr);
    }
    शून्य visit(AstScopeName* nodep) override {
        // Print a %m under this instance as its vlNamep, which holds its pretty name, plus the
        // rest of the path, so the instances' code can be V3Combined. Not with --protect-ids, as
        // vlNamep then holds the protected name, while %m prints the original.
        जर (!m_funcp || !nodep->forFormat() || m_modSingleton || m_funcp->isStatic()
            || v3Global.opt.protectIds()) {
            परत;
        }
        std::string path = nodep->scopePrettySymName();
        स्थिरांक std::string inst = m_scopep->prettyName();
        जर (path != inst && !VString::startsWith(path, inst + ".")) परत;
        path.erase(0, inst.size() + 1);
        // The DPI scope table keeps the absolute name, via a copy outside any function
        m_scopep->addBlocksp(nodep->clone_वृक्ष(असत्य));
        nodep->scopeAttr(path);
        nodep->scopeEntr("");  // Only used by the DPI scope table
        nodep->selfRelative(सत्य);
    }
    शून्य visit(AstCMethodCall* nodep) override { iterateChildren(nodep); }
    शून्य visit(AstCNew* nodep) override { iterateChildren(nodep); }
    शून्य visit(AstCFunc* nodep) override {
        VL_RESTORER(m_funcp);
        जर (!nodep->user1()) {
            // Static functions should have been moved under the corresponding AstClassPackage
            UASSERT_OBJ(!(nodep->isStatic() && VN_IS(m_modp, Class)), nodep,
                        "Static function under AstClass");
            m_funcp = nodep;
            iterateChildren(nodep);
            nodep->user1(सत्य);
            // If it's under a scope, move it up to the top
            जर (m_scopep) {
                nodep->unlinkFrBack();
                m_modp->addStmtsp(nodep);

                जर (nodep->funcPublic()) {
                    // There may be multiple public functions by the same name;
                    // record for later correction or making of shells
                    m_modFuncs.emplace(nodep->name(), nodep);
                    nodep->name(m_scopep->nameDotless() + "__" + nodep->name());
                }
            }
        }
    }
    शून्य visit(AstVar*) override {}
    शून्य visit(Ast_गाठ* nodep) override { iterateChildren(nodep); }

सार्वजनिक:
    // CONSTRUCTORS
    explicit DescopeVisitor(AstNetlist* nodep) { iterate(nodep); }
    ~DescopeVisitor() override = पूर्वनिर्धारित;
};

//######################################################################
// Descope class functions

शून्य V3Descope::descopeAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { DescopeVisitor{nodep}; }  // Destruct before checking
    V3ConstPool::setDescoped();
    V3Global::dump_Check_Global_वृक्ष("descope", 0, dump_वृक्ष_Either_Level() >= 3);
}
