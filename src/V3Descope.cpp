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

#समावेश "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#समावेश "V3Descope.h"

#समावेश "V3ConstPool.h"
#समावेश "V3EmitCBase.h"

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################

श्रेणी DescopeVisitor final : खुला VNVisitor {
    // NODE STATE
    //  Cleared entire netlist
    //   AstCFunc::user()               // bool.  Indicates processing completed
    स्थिर VNUser1InUse m_inuser1;

    // TYPES
    using FuncMmap = std::multimap<std::string, AstCFunc*>;
    काष्ठा ScopeSelfPtr final {
        VSelfPointerText thisPtr = VSelfPointerText{VSelfPointerText::Empty()};
        VSelfPointerText vlSymsPtr = VSelfPointerText{VSelfPointerText::Empty()};
    };

    // STATE
    Ast_गाँठ_Module* m_modp = nullptr;  // Current module
    AstScope* m_scopep = nullptr;  // Current scope
    स्थिर AstCFunc* m_funcp = nullptr;  // Current function
    bool m_modSingleton = false;  // m_modp is only instantiated once
    FuncMmap m_modFuncs;  // Name of public functions added
    std::map<स्थिर AstScope*, ScopeSelfPtr> m_scopeToSelf;  // Scope to self pointers

    // METHODS

    अटल bool modIsSingleton(स्थिर Ast_गाँठ_Module* modp) {
        // True iff there's exactly one instance of this module in the design (including top).
        अगर (modp->is_शीर्ष()) वापस true;
        पूर्णांक instances = 0;
        क्रम (स्थिर Ast_गाँठ* stmtp = modp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            अगर (VN_IS(stmtp, Scope)) {
                अगर (++instances > 1) वापस false;
            }
        }
        वापस (instances == 1);
    }

    // Construct a 'this' self pointer for the given scope
    VSelfPointerText scopeThis(स्थिर AstScope* scopep) {
        स्वतः& ret = m_scopeToSelf[scopep];
        अगर (ret.thisPtr.isEmpty()) {
            string name = scopep->name();
            string::आकार_type pos = name.rfind('.');
            अगर (pos != string::npos) name.erase(0, pos + 1);
            ret.thisPtr = VSelfPointerText{VSelfPointerText::This{}, name};
        }
        वापस ret.thisPtr;
    }
    // Construct a 'vlSyms' self pointer for the given scope
    VSelfPointerText scopeVlSyms(स्थिर AstScope* scopep) {
        स्वतः& ret = m_scopeToSelf[scopep];
        अगर (ret.vlSymsPtr.isEmpty()) {
            ret.vlSymsPtr = VSelfPointerText{VSelfPointerText::VlSyms(), scopep->nameDotless()};
        }
        वापस ret.vlSymsPtr;
    }

    // Construct the best self pointer to reference an object in 'scopep'  from a CFunc in
    // 'm_scopep'. Result may be relative ("this->[...]") or absolute ("vlSyms->[...]").
    //
    // Using relative references allows V3Combine'ing code across multiple instances of the same
    // module.
    VSelfPointerText descopedSelfPointer(स्थिर AstScope* scopep) {
        UASSERT(scopep, "Var/Func not scoped");
        // Static functions can't use relative references via 'this->'
        स्थिर bool relativeRefOk = !m_funcp->isStatic();

        UINFO(8, "      Descope ref under " << m_scopep);
        UINFO(8, "              ref to    " << scopep);
        UINFO(8, "             aboveScope " << scopep->aboveScopep());

        अगर (VN_IS(scopep->modp(), Class)) {
            // Direct reference to class members are from within the class itself, references from
            // outside the class must go via AstMemberSel
            वापस VSelfPointerText{VSelfPointerText::This{}};
        } अन्यथा अगर (relativeRefOk && scopep == m_scopep) {
            वापस VSelfPointerText{VSelfPointerText::This{}};
        } अन्यथा अगर (relativeRefOk && !m_modSingleton && scopep->aboveScopep() == m_scopep
                   && VN_IS(scopep->modp(), Module)) {
            // Reference to scope of instance directly under this module, can just "this->cell",
            // which can potentially be V3Combined, but note this requires one extra pointer
            // dereference which is slower, so we only use it if the source scope is not a
            // singleton.
            वापस scopeThis(scopep);
        } अन्यथा {
            // Reference to something elsewhere, or relative references are disabled. Use global
            // variable
            वापस scopeVlSyms(scopep);
        }
    }

    व्योम makePublicFuncWrappers() {
        // We recorded all public functions in m_modFuncs.
        // If for any given name only one function exists, we can use that function directly.
        // If multiple functions exist, we need to select the appropriate scope.
        क्रम (FuncMmap::iterator it = m_modFuncs.begin(); it != m_modFuncs.end(); ++it) {
            स्थिर string name = it->first;
            AstCFunc* स्थिर शीर्ष_Funcp = it->second;
            स्वतः अग्र_It1 = it;
            ++अग्र_It1;
            स्थिर bool moreOfSame1 = (अग्र_It1 != m_modFuncs.end() && अग्र_It1->first == name);
            अगर (moreOfSame1) {
                // Multiple functions under this name, need a wrapper function
                UINFO(6, "  Wrapping " << name << " multifuncs");
                AstCFunc* स्थिर newfuncp = शीर्ष_Funcp->clone_वृक्ष(false);
                अगर (newfuncp->varsp()) newfuncp->varsp()->unlink_Fr_Back_With_अग्र()->delete_वृक्ष();
                अगर (newfuncp->stmtsp()) newfuncp->stmtsp()->unlink_Fr_Back_With_अग्र()->delete_वृक्ष();
                newfuncp->name(name);
                newfuncp->isStatic(false);
                शीर्ष_Funcp->add_अग्र_Here(newfuncp);
                // In the body, call each function if it matches the given scope
                क्रम (FuncMmap::iterator eachIt = it;
                     eachIt != m_modFuncs.end() && eachIt->first == name; ++eachIt) {
                    it = eachIt;
                    AstCFunc* स्थिर funcp = eachIt->second;
                    स्वतः अग्र_It2 = eachIt;
                    ++अग्र_It2;
                    स्थिर bool moreOfSame
                        = (अग्र_It2 != m_modFuncs.end() && अग्र_It2->first == name);
                    UASSERT_OBJ(funcp->scopep(), funcp, "Not scoped");

                    UINFO(6, "     Wrapping " << name << " " << funcp);
                    UINFO(6, "  at " << newfuncp->argTypes() << " und " << funcp->argTypes());
                    funcp->declPrivate(true);
                    AstVarRef* argsp = nullptr;
                    क्रम (Ast_गाँठ* stmtp = newfuncp->argsp(); stmtp; stmtp = stmtp->nextp()) {
                        अगर (AstVar* स्थिर portp = VN_CAST(stmtp, Var)) {
                            अगर (portp->isIO() && !portp->isFuncReturn()) {
                                AstVarRef* स्थिर newp = नया AstVarRef{
                                    portp->fileline(), portp, portp->direction().pinAccess()};
                                argsp = Ast_गाँठ::add_अग्र(argsp, newp);
                            }
                        }
                    }

                    AstCCall* स्थिर callp = नया AstCCall{funcp->fileline(), funcp, argsp};
                    अगर (Ast_गाँठ_DType* स्थिर dtypep = funcp->dtypep()) {
                        callp->dtypep(dtypep);
                    } अन्यथा {
                        callp->dtypeSetVoid();
                    }
                    Ast_गाँठ* स्थिर returnp = नया AstCReturn{funcp->fileline(), callp};

                    अगर (moreOfSame) {
                        AstIf* स्थिर ifp = नया AstIf{
                            funcp->fileline(),
                            नया AstEq{funcp->fileline(),
                                      नया AstCExpr{funcp->fileline(), "this", 64},
                                      नया AstCExpr{funcp->fileline(),
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
    व्योम visit(Ast_गाँठ_Module* nodep) override {
        VL_RESTORER(m_modp);
        m_modp = nodep;
        m_modFuncs.सफा();
        m_modSingleton = modIsSingleton(m_modp);
        iterateChildren(nodep);
        makePublicFuncWrappers();
    }
    व्योम visit(AstScope* nodep) override {
        VL_RESTORER(m_scopep);
        m_scopep = nodep;
        iterateChildren(nodep);
    }
    व्योम visit(AstVarScope* nodep) override {
        // Delete the varscope when we're finished
        VL_DO_DANGLING(धकेलो_Deletep(nodep->unlinkFrBack()), nodep);
    }
    व्योम visit(Ast_गाँठ_Var_Ref* nodep) override {
        iterateChildren(nodep);
        अगर (!nodep->varScopep()) {
            UASSERT_OBJ(nodep->varp()->isFuncLocal(), nodep,
                        "unscoped reference can only appear to function locals at this point");
            वापस;
        }
        // Convert the hierch name
        UINFO(9, "  ref-in " << nodep);
        UASSERT_OBJ(m_scopep, nodep, "Node not under scope");
        स्थिर AstVar* स्थिर varp = nodep->varScopep()->varp();
        स्थिर AstScope* स्थिर scopep = nodep->varScopep()->scopep();
        अगर (varp->isFuncLocal()) {
            // Reference to function locals need no self pointer
            nodep->selfPointer(VSelfPointerText{VSelfPointerText::Empty()});
        } अन्यथा अगर (scopep->modp()->isConstPool()) {
            // Reference to constant pool value need no self pointer
            nodep->selfPointer(VSelfPointerText{VSelfPointerText::Empty()});
        } अन्यथा {
            nodep->selfPointer(descopedSelfPointer(scopep));
        }
        nodep->varScopep(nullptr);
        UINFO(9, "  refout " << nodep << " selfPtr=" << nodep->selfPointer().asString());
    }
    व्योम visit(AstCCall* nodep) override {
        // UINFO(9, "       " << nodep);
        iterateChildren(nodep);
        // Convert the hierch name
        UASSERT_OBJ(m_scopep, nodep, "Node not under scope");
        स्थिर AstScope* स्थिर scopep = nodep->funcp()->scopep();
        nodep->selfPointer(descopedSelfPointer(scopep));
        // Can't do this, as we may have more calls later
        // nodep->funcp()->scopep(nullptr);
    }
    व्योम visit(AstScopeName* nodep) override {
        // Print a %m under this instance as its vlNamep, which holds its pretty name, plus the
        // rest of the path, so the instances' code can be V3Combined. Not with --protect-ids, as
        // vlNamep then holds the protected name, while %m prints the original.
        अगर (!m_funcp || !nodep->forFormat() || m_modSingleton || m_funcp->isStatic()
            || v3Global.opt.protectIds()) {
            वापस;
        }
        std::string path = nodep->scopePrettySymName();
        स्थिर std::string inst = m_scopep->prettyName();
        अगर (path != inst && !VString::startsWith(path, inst + ".")) वापस;
        path.erase(0, inst.आकार() + 1);
        // The DPI scope table keeps the absolute name, via a copy outside any function
        m_scopep->addBlocksp(nodep->clone_वृक्ष(false));
        nodep->scopeAttr(path);
        nodep->scopeEntr("");  // Only used by the DPI scope table
        nodep->selfRelative(true);
    }
    व्योम visit(AstCMethodCall* nodep) override { iterateChildren(nodep); }
    व्योम visit(AstCNew* nodep) override { iterateChildren(nodep); }
    व्योम visit(AstCFunc* nodep) override {
        VL_RESTORER(m_funcp);
        अगर (!nodep->user1()) {
            // Static functions should have been moved under the corresponding AstClassPackage
            UASSERT_OBJ(!(nodep->isStatic() && VN_IS(m_modp, Class)), nodep,
                        "Static function under AstClass");
            m_funcp = nodep;
            iterateChildren(nodep);
            nodep->user1(true);
            // If it's under a scope, move it up to the top
            अगर (m_scopep) {
                nodep->unlinkFrBack();
                m_modp->addStmtsp(nodep);

                अगर (nodep->funcPublic()) {
                    // There may be multiple public functions by the same name;
                    // record for later correction or making of shells
                    m_modFuncs.emplace(nodep->name(), nodep);
                    nodep->name(m_scopep->nameDotless() + "__" + nodep->name());
                }
            }
        }
    }
    व्योम visit(AstVar*) override {}
    व्योम visit(Ast_गाँठ* nodep) override { iterateChildren(nodep); }

खुला:
    // CONSTRUCTORS
    explicit DescopeVisitor(AstNetlist* nodep) { iterate(nodep); }
    ~DescopeVisitor() override = शेष;
};

//######################################################################
// Descope class functions

व्योम V3Descope::descopeAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { DescopeVisitor{nodep}; }  // Destruct before checking
    V3ConstPool::setDescoped();
    V3Global::dump_Check_Global_वृक्ष("descope", 0, dump_वृक्ष_Either_Level() >= 3);
}
