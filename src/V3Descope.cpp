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
    अचर VNUser1InUse m_inuser1;

    // TYPES
    using FuncMmap = std::multimap<std::string, AstCFunc*>;
    संरचना ScopeSelfPtr final {
        VSelfPointerText thisPtr = VSelfPointerText{VSelfPointerText::Empty()};
        VSelfPointerText vlSymsPtr = VSelfPointerText{VSelfPointerText::Empty()};
    };

    // STATE
    Ast_गाँठो_Module* m_modp = nullptr;  // Current module
    AstScope* m_scopep = nullptr;  // Current scope
    अचर AstCFunc* m_funcp = nullptr;  // Current function
    तार्किक m_modSingleton = असत्य;  // m_modp is only instantiated once
    FuncMmap m_modFuncs;  // Name of public functions added
    std::map<अचर AstScope*, ScopeSelfPtr> m_scopeToSelf;  // Scope to self pointers

    // METHODS

    स्थिर तार्किक modIsSingleton(अचर Ast_गाँठो_Module* modp) {
        // True iff there's exactly one instance of this module in the design (including top).
        यदि (modp->is_शीर्ष()) फर्काऊ सत्य;
        पूर्णाङ्क instances = 0;
        लागि (अचर Ast_गाँठो* stmtp = modp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            यदि (VN_IS(stmtp, Scope)) {
                यदि (++instances > 1) फर्काऊ असत्य;
            }
        }
        फर्काऊ (instances == 1);
    }

    // Construct a 'this' self pointer for the given scope
    VSelfPointerText scopeThis(अचर AstScope* scopep) {
        स्वतः& ret = m_scopeToSelf[scopep];
        यदि (ret.thisPtr.isEmpty()) {
            string name = scopep->name();
            string::size_type pos = name.rfind('.');
            यदि (pos != string::npos) name.erase(0, pos + 1);
            ret.thisPtr = VSelfPointerText{VSelfPointerText::This{}, name};
        }
        फर्काऊ ret.thisPtr;
    }
    // Construct a 'vlSyms' self pointer for the given scope
    VSelfPointerText scopeVlSyms(अचर AstScope* scopep) {
        स्वतः& ret = m_scopeToSelf[scopep];
        यदि (ret.vlSymsPtr.isEmpty()) {
            ret.vlSymsPtr = VSelfPointerText{VSelfPointerText::VlSyms(), scopep->nameDotless()};
        }
        फर्काऊ ret.vlSymsPtr;
    }

    // Construct the best self pointer to reference an object in 'scopep'  from a CFunc in
    // 'm_scopep'. Result may be relative ("this->[...]") or absolute ("vlSyms->[...]").
    //
    // Using relative references allows V3Combine'ing code across multiple instances of the same
    // module.
    VSelfPointerText descopedSelfPointer(अचर AstScope* scopep) {
        UASSERT(scopep, "Var/Func not scoped");
        // Static functions can't use relative references via 'this->'
        अचर तार्किक relativeRefOk = !m_funcp->isStatic();

        UINFO(8, "      Descope ref under " << m_scopep);
        UINFO(8, "              ref to    " << scopep);
        UINFO(8, "             aboveScope " << scopep->aboveScopep());

        यदि (VN_IS(scopep->modp(), Class)) {
            // Direct reference to class members are from within the class itself, references from
            // outside the class must go via AstMemberSel
            फर्काऊ VSelfPointerText{VSelfPointerText::This{}};
        } अन्यथा यदि (relativeRefOk && scopep == m_scopep) {
            फर्काऊ VSelfPointerText{VSelfPointerText::This{}};
        } अन्यथा यदि (relativeRefOk && !m_modSingleton && scopep->aboveScopep() == m_scopep
                   && VN_IS(scopep->modp(), Module)) {
            // Reference to scope of instance directly under this module, can just "this->cell",
            // which can potentially be V3Combined, but note this requires one extra pointer
            // dereference which is slower, so we only use it if the source scope is not a
            // singleton.
            फर्काऊ scopeThis(scopep);
        } अन्यथा {
            // Reference to something elsewhere, or relative references are disabled. Use global
            // variable
            फर्काऊ scopeVlSyms(scopep);
        }
    }

    शून्य makePublicFuncWrappers() {
        // We recorded all public functions in m_modFuncs.
        // If for any given name only one function exists, we can use that function directly.
        // If multiple functions exist, we need to select the appropriate scope.
        लागि (FuncMmap::iterator it = m_modFuncs.begin(); it != m_modFuncs.end(); ++it) {
            अचर string name = it->first;
            AstCFunc* अचर शीर्ष_Funcp = it->second;
            स्वतः nextIt1 = it;
            ++nextIt1;
            अचर तार्किक moreOfSame1 = (nextIt1 != m_modFuncs.end() && nextIt1->first == name);
            यदि (moreOfSame1) {
                // Multiple functions under this name, need a wrapper function
                UINFO(6, "  Wrapping " << name << " multifuncs");
                AstCFunc* अचर newfuncp = शीर्ष_Funcp->clone_रूख(असत्य);
                यदि (newfuncp->varsp()) newfuncp->varsp()->unlinkFrBackWithNext()->delete_रूख();
                यदि (newfuncp->stmtsp()) newfuncp->stmtsp()->unlinkFrBackWithNext()->delete_रूख();
                newfuncp->name(name);
                newfuncp->isStatic(असत्य);
                शीर्ष_Funcp->addNextHere(newfuncp);
                // In the body, call each function if it matches the given scope
                लागि (FuncMmap::iterator eachIt = it;
                     eachIt != m_modFuncs.end() && eachIt->first == name; ++eachIt) {
                    it = eachIt;
                    AstCFunc* अचर funcp = eachIt->second;
                    स्वतः nextIt2 = eachIt;
                    ++nextIt2;
                    अचर तार्किक moreOfSame
                        = (nextIt2 != m_modFuncs.end() && nextIt2->first == name);
                    UASSERT_OBJ(funcp->scopep(), funcp, "Not scoped");

                    UINFO(6, "     Wrapping " << name << " " << funcp);
                    UINFO(6, "  at " << newfuncp->argTypes() << " und " << funcp->argTypes());
                    funcp->declPrivate(सत्य);
                    AstVarRef* argsp = nullptr;
                    लागि (Ast_गाँठो* stmtp = newfuncp->argsp(); stmtp; stmtp = stmtp->nextp()) {
                        यदि (AstVar* अचर portp = VN_CAST(stmtp, Var)) {
                            यदि (portp->isIO() && !portp->isFuncReturn()) {
                                AstVarRef* अचर newp = नूतन AstVarRef{
                                    portp->fileline(), portp, portp->direction().pinAccess()};
                                argsp = Ast_गाँठो::addNext(argsp, newp);
                            }
                        }
                    }

                    AstCCall* अचर callp = नूतन AstCCall{funcp->fileline(), funcp, argsp};
                    यदि (Ast_गाँठो_DType* अचर dtypep = funcp->dtypep()) {
                        callp->dtypep(dtypep);
                    } अन्यथा {
                        callp->dtypeSetVoid();
                    }
                    Ast_गाँठो* अचर returnp = नूतन AstCReturn{funcp->fileline(), callp};

                    यदि (moreOfSame) {
                        AstIf* अचर ifp = नूतन AstIf{
                            funcp->fileline(),
                            नूतन AstEq{funcp->fileline(),
                                      नूतन AstCExpr{funcp->fileline(), "this", 64},
                                      नूतन AstCExpr{funcp->fileline(),
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
    शून्य visit(Ast_गाँठो_Module* nodep) override {
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
        VL_DO_DANGLING(थप_Deletep(nodep->unlinkFrBack()), nodep);
    }
    शून्य visit(Ast_गाँठो_Var_Ref* nodep) override {
        iterateChildren(nodep);
        यदि (!nodep->varScopep()) {
            UASSERT_OBJ(nodep->varp()->isFuncLocal(), nodep,
                        "unscoped reference can only appear to function locals at this point");
            फर्काऊ;
        }
        // Convert the hierch name
        UINFO(9, "  ref-in " << nodep);
        UASSERT_OBJ(m_scopep, nodep, "Node not under scope");
        अचर AstVar* अचर varp = nodep->varScopep()->varp();
        अचर AstScope* अचर scopep = nodep->varScopep()->scopep();
        यदि (varp->isFuncLocal()) {
            // Reference to function locals need no self pointer
            nodep->selfPointer(VSelfPointerText{VSelfPointerText::Empty()});
        } अन्यथा यदि (scopep->modp()->isConstPool()) {
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
        अचर AstScope* अचर scopep = nodep->funcp()->scopep();
        nodep->selfPointer(descopedSelfPointer(scopep));
        // Can't do this, as we may have more calls later
        // nodep->funcp()->scopep(nullptr);
    }
    शून्य visit(AstScopeName* nodep) override {
        // Print a %m under this instance as its vlNamep, which holds its pretty name, plus the
        // rest of the path, so the instances' code can be V3Combined. Not with --protect-ids, as
        // vlNamep then holds the protected name, while %m prints the original.
        यदि (!m_funcp || !nodep->forFormat() || m_modSingleton || m_funcp->isStatic()
            || v3Global.opt.protectIds()) {
            फर्काऊ;
        }
        std::string path = nodep->scopePrettySymName();
        अचर std::string inst = m_scopep->prettyName();
        यदि (path != inst && !VString::startsWith(path, inst + ".")) फर्काऊ;
        path.erase(0, inst.size() + 1);
        // The DPI scope table keeps the absolute name, via a copy outside any function
        m_scopep->addBlocksp(nodep->clone_रूख(असत्य));
        nodep->scopeAttr(path);
        nodep->scopeEntr("");  // Only used by the DPI scope table
        nodep->selfRelative(सत्य);
    }
    शून्य visit(AstCMethodCall* nodep) override { iterateChildren(nodep); }
    शून्य visit(AstCNew* nodep) override { iterateChildren(nodep); }
    शून्य visit(AstCFunc* nodep) override {
        VL_RESTORER(m_funcp);
        यदि (!nodep->user1()) {
            // Static functions should have been moved under the corresponding AstClassPackage
            UASSERT_OBJ(!(nodep->isStatic() && VN_IS(m_modp, Class)), nodep,
                        "Static function under AstClass");
            m_funcp = nodep;
            iterateChildren(nodep);
            nodep->user1(सत्य);
            // If it's under a scope, move it up to the top
            यदि (m_scopep) {
                nodep->unlinkFrBack();
                m_modp->addStmtsp(nodep);

                यदि (nodep->funcPublic()) {
                    // There may be multiple public functions by the same name;
                    // record for later correction or making of shells
                    m_modFuncs.emplace(nodep->name(), nodep);
                    nodep->name(m_scopep->nameDotless() + "__" + nodep->name());
                }
            }
        }
    }
    शून्य visit(AstVar*) override {}
    शून्य visit(Ast_गाँठो* nodep) override { iterateChildren(nodep); }

सार्वजनिक:
    // CONSTRUCTORS
    explicit DescopeVisitor(AstNetlist* nodep) { iterate(nodep); }
    ~DescopeVisitor() override = स्वभाव;
};

//######################################################################
// Descope class functions

शून्य V3Descope::descopeAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { DescopeVisitor{nodep}; }  // Destruct before checking
    V3ConstPool::setDescoped();
    V3Global::dump_Check_Global_रूख("descope", 0, dump_रूख_Either_Level() >= 3);
}
