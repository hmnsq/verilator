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

#અન્તર્ભાવ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#અન્તર્ભાવ "V3Descope.h"

#અન્તર્ભાવ "V3ConstPool.h"
#અન્તર્ભાવ "V3EmitCBase.h"

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################

વર્ગ DescopeVisitor final : જાહેર VNVisitor {
    // NODE STATE
    //  Cleared entire netlist
    //   AstCFunc::user()               // bool.  Indicates processing completed
    અચલ VNUser1InUse m_inuser1;

    // TYPES
    using FuncMmap = std::multimap<std::string, AstCFunc*>;
    સંરચના ScopeSelfPtr final {
        VSelfPointerText thisPtr = VSelfPointerText{VSelfPointerText::Empty()};
        VSelfPointerText vlSymsPtr = VSelfPointerText{VSelfPointerText::Empty()};
    };

    // STATE
    Ast_ગાંઠ_Module* m_modp = nullptr;  // Current module
    AstScope* m_scopep = nullptr;  // Current scope
    અચલ AstCFunc* m_funcp = nullptr;  // Current function
    તાર્કિક m_modSingleton = ખોટું;  // m_modp is only instantiated once
    FuncMmap m_modFuncs;  // Name of public functions added
    std::map<અચલ AstScope*, ScopeSelfPtr> m_scopeToSelf;  // Scope to self pointers

    // METHODS

    સ્થિર તાર્કિક modIsSingleton(અચલ Ast_ગાંઠ_Module* modp) {
        // True iff there's exactly one instance of this module in the design (including top).
        જો (modp->is_ટોચ()) પાછા સાચું;
        પૂર્ણાંક instances = 0;
        માટે (અચલ Ast_ગાંઠ* stmtp = modp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            જો (VN_IS(stmtp, Scope)) {
                જો (++instances > 1) પાછા ખોટું;
            }
        }
        પાછા (instances == 1);
    }

    // Construct a 'this' self pointer for the given scope
    VSelfPointerText scopeThis(અચલ AstScope* scopep) {
        auto& ret = m_scopeToSelf[scopep];
        જો (ret.thisPtr.isEmpty()) {
            string name = scopep->name();
            string::size_type pos = name.rfind('.');
            જો (pos != string::npos) name.erase(0, pos + 1);
            ret.thisPtr = VSelfPointerText{VSelfPointerText::This{}, name};
        }
        પાછા ret.thisPtr;
    }
    // Construct a 'vlSyms' self pointer for the given scope
    VSelfPointerText scopeVlSyms(અચલ AstScope* scopep) {
        auto& ret = m_scopeToSelf[scopep];
        જો (ret.vlSymsPtr.isEmpty()) {
            ret.vlSymsPtr = VSelfPointerText{VSelfPointerText::VlSyms(), scopep->nameDotless()};
        }
        પાછા ret.vlSymsPtr;
    }

    // Construct the best self pointer to reference an object in 'scopep'  from a CFunc in
    // 'm_scopep'. Result may be relative ("this->[...]") or absolute ("vlSyms->[...]").
    //
    // Using relative references allows V3Combine'ing code across multiple instances of the same
    // module.
    VSelfPointerText descopedSelfPointer(અચલ AstScope* scopep) {
        UASSERT(scopep, "Var/Func not scoped");
        // Static functions can't use relative references via 'this->'
        અચલ તાર્કિક relativeRefOk = !m_funcp->isStatic();

        UINFO(8, "      Descope ref under " << m_scopep);
        UINFO(8, "              ref to    " << scopep);
        UINFO(8, "             aboveScope " << scopep->aboveScopep());

        જો (VN_IS(scopep->modp(), Class)) {
            // Direct reference to class members are from within the class itself, references from
            // outside the class must go via AstMemberSel
            પાછા VSelfPointerText{VSelfPointerText::This{}};
        } નહીંતર જો (relativeRefOk && scopep == m_scopep) {
            પાછા VSelfPointerText{VSelfPointerText::This{}};
        } નહીંતર જો (relativeRefOk && !m_modSingleton && scopep->aboveScopep() == m_scopep
                   && VN_IS(scopep->modp(), Module)) {
            // Reference to scope of instance directly under this module, can just "this->cell",
            // which can potentially be V3Combined, but note this requires one extra pointer
            // dereference which is slower, so we only use it if the source scope is not a
            // singleton.
            પાછા scopeThis(scopep);
        } નહીંતર {
            // Reference to something elsewhere, or relative references are disabled. Use global
            // variable
            પાછા scopeVlSyms(scopep);
        }
    }

    શૂન્ય makePublicFuncWrappers() {
        // We recorded all public functions in m_modFuncs.
        // If for any given name only one function exists, we can use that function directly.
        // If multiple functions exist, we need to select the appropriate scope.
        માટે (FuncMmap::iterator it = m_modFuncs.begin(); it != m_modFuncs.end(); ++it) {
            અચલ string name = it->first;
            AstCFunc* અચલ ટોચ_Funcp = it->second;
            auto nextIt1 = it;
            ++nextIt1;
            અચલ તાર્કિક moreOfSame1 = (nextIt1 != m_modFuncs.end() && nextIt1->first == name);
            જો (moreOfSame1) {
                // Multiple functions under this name, need a wrapper function
                UINFO(6, "  Wrapping " << name << " multifuncs");
                AstCFunc* અચલ newfuncp = ટોચ_Funcp->clone_વૃક્ષ(ખોટું);
                જો (newfuncp->varsp()) newfuncp->varsp()->unlinkFrBackWithNext()->delete_વૃક્ષ();
                જો (newfuncp->stmtsp()) newfuncp->stmtsp()->unlinkFrBackWithNext()->delete_વૃક્ષ();
                newfuncp->name(name);
                newfuncp->isStatic(ખોટું);
                ટોચ_Funcp->addNextHere(newfuncp);
                // In the body, call each function if it matches the given scope
                માટે (FuncMmap::iterator eachIt = it;
                     eachIt != m_modFuncs.end() && eachIt->first == name; ++eachIt) {
                    it = eachIt;
                    AstCFunc* અચલ funcp = eachIt->second;
                    auto nextIt2 = eachIt;
                    ++nextIt2;
                    અચલ તાર્કિક moreOfSame
                        = (nextIt2 != m_modFuncs.end() && nextIt2->first == name);
                    UASSERT_OBJ(funcp->scopep(), funcp, "Not scoped");

                    UINFO(6, "     Wrapping " << name << " " << funcp);
                    UINFO(6, "  at " << newfuncp->argTypes() << " und " << funcp->argTypes());
                    funcp->declPrivate(સાચું);
                    AstVarRef* argsp = nullptr;
                    માટે (Ast_ગાંઠ* stmtp = newfuncp->argsp(); stmtp; stmtp = stmtp->nextp()) {
                        જો (AstVar* અચલ portp = VN_CAST(stmtp, Var)) {
                            જો (portp->isIO() && !portp->isFuncReturn()) {
                                AstVarRef* અચલ newp = નવું AstVarRef{
                                    portp->fileline(), portp, portp->direction().pinAccess()};
                                argsp = Ast_ગાંઠ::addNext(argsp, newp);
                            }
                        }
                    }

                    AstCCall* અચલ callp = નવું AstCCall{funcp->fileline(), funcp, argsp};
                    જો (Ast_ગાંઠ_DType* અચલ dtypep = funcp->dtypep()) {
                        callp->dtypep(dtypep);
                    } નહીંતર {
                        callp->dtypeSetVoid();
                    }
                    Ast_ગાંઠ* અચલ returnp = નવું AstCReturn{funcp->fileline(), callp};

                    જો (moreOfSame) {
                        AstIf* અચલ ifp = નવું AstIf{
                            funcp->fileline(),
                            નવું AstEq{funcp->fileline(),
                                      નવું AstCExpr{funcp->fileline(), "this", 64},
                                      નવું AstCExpr{funcp->fileline(),
                                                   scopeVlSyms(funcp->scopep()).asString(), 64}},
                            returnp};
                        newfuncp->addStmtsp(ifp);
                    } નહીંતર {
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
            } નહીંતર {
                // Only a single function under this name, we can rename it
                UINFO(6, "  Wrapping " << name << " just one " << ટોચ_Funcp);
                ટોચ_Funcp->name(name);
            }
        }
    }

    // VISITORS
    શૂન્ય visit(Ast_ગાંઠ_Module* nodep) override {
        VL_RESTORER(m_modp);
        m_modp = nodep;
        m_modFuncs.clear();
        m_modSingleton = modIsSingleton(m_modp);
        iterateChildren(nodep);
        makePublicFuncWrappers();
    }
    શૂન્ય visit(AstScope* nodep) override {
        VL_RESTORER(m_scopep);
        m_scopep = nodep;
        iterateChildren(nodep);
    }
    શૂન્ય visit(AstVarScope* nodep) override {
        // Delete the varscope when we're finished
        VL_DO_DANGLING(ધકેલો_Deletep(nodep->unlinkFrBack()), nodep);
    }
    શૂન્ય visit(Ast_ગાંઠ_Var_Ref* nodep) override {
        iterateChildren(nodep);
        જો (!nodep->varScopep()) {
            UASSERT_OBJ(nodep->varp()->isFuncLocal(), nodep,
                        "unscoped reference can only appear to function locals at this point");
            પાછા;
        }
        // Convert the hierch name
        UINFO(9, "  ref-in " << nodep);
        UASSERT_OBJ(m_scopep, nodep, "Node not under scope");
        અચલ AstVar* અચલ varp = nodep->varScopep()->varp();
        અચલ AstScope* અચલ scopep = nodep->varScopep()->scopep();
        જો (varp->isFuncLocal()) {
            // Reference to function locals need no self pointer
            nodep->selfPointer(VSelfPointerText{VSelfPointerText::Empty()});
        } નહીંતર જો (scopep->modp()->isConstPool()) {
            // Reference to constant pool value need no self pointer
            nodep->selfPointer(VSelfPointerText{VSelfPointerText::Empty()});
        } નહીંતર {
            nodep->selfPointer(descopedSelfPointer(scopep));
        }
        nodep->varScopep(nullptr);
        UINFO(9, "  refout " << nodep << " selfPtr=" << nodep->selfPointer().asString());
    }
    શૂન્ય visit(AstCCall* nodep) override {
        // UINFO(9, "       " << nodep);
        iterateChildren(nodep);
        // Convert the hierch name
        UASSERT_OBJ(m_scopep, nodep, "Node not under scope");
        અચલ AstScope* અચલ scopep = nodep->funcp()->scopep();
        nodep->selfPointer(descopedSelfPointer(scopep));
        // Can't do this, as we may have more calls later
        // nodep->funcp()->scopep(nullptr);
    }
    શૂન્ય visit(AstScopeName* nodep) override {
        // Print a %m under this instance as its vlNamep, which holds its pretty name, plus the
        // rest of the path, so the instances' code can be V3Combined. Not with --protect-ids, as
        // vlNamep then holds the protected name, while %m prints the original.
        જો (!m_funcp || !nodep->forFormat() || m_modSingleton || m_funcp->isStatic()
            || v3Global.opt.protectIds()) {
            પાછા;
        }
        std::string path = nodep->scopePrettySymName();
        અચલ std::string inst = m_scopep->prettyName();
        જો (path != inst && !VString::startsWith(path, inst + ".")) પાછા;
        path.erase(0, inst.size() + 1);
        // The DPI scope table keeps the absolute name, via a copy outside any function
        m_scopep->addBlocksp(nodep->clone_વૃક્ષ(ખોટું));
        nodep->scopeAttr(path);
        nodep->scopeEntr("");  // Only used by the DPI scope table
        nodep->selfRelative(સાચું);
    }
    શૂન્ય visit(AstCMethodCall* nodep) override { iterateChildren(nodep); }
    શૂન્ય visit(AstCNew* nodep) override { iterateChildren(nodep); }
    શૂન્ય visit(AstCFunc* nodep) override {
        VL_RESTORER(m_funcp);
        જો (!nodep->user1()) {
            // Static functions should have been moved under the corresponding AstClassPackage
            UASSERT_OBJ(!(nodep->isStatic() && VN_IS(m_modp, Class)), nodep,
                        "Static function under AstClass");
            m_funcp = nodep;
            iterateChildren(nodep);
            nodep->user1(સાચું);
            // If it's under a scope, move it up to the top
            જો (m_scopep) {
                nodep->unlinkFrBack();
                m_modp->addStmtsp(nodep);

                જો (nodep->funcPublic()) {
                    // There may be multiple public functions by the same name;
                    // record for later correction or making of shells
                    m_modFuncs.emplace(nodep->name(), nodep);
                    nodep->name(m_scopep->nameDotless() + "__" + nodep->name());
                }
            }
        }
    }
    શૂન્ય visit(AstVar*) override {}
    શૂન્ય visit(Ast_ગાંઠ* nodep) override { iterateChildren(nodep); }

જાહેર:
    // CONSTRUCTORS
    explicit DescopeVisitor(AstNetlist* nodep) { iterate(nodep); }
    ~DescopeVisitor() override = મૂળભૂત;
};

//######################################################################
// Descope class functions

શૂન્ય V3Descope::descopeAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { DescopeVisitor{nodep}; }  // Destruct before checking
    V3ConstPool::setDescoped();
    V3Global::dump_Check_Global_વૃક્ષ("descope", 0, dump_વૃક્ષ_Either_Level() >= 3);
}
