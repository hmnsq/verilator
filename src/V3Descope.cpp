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

#അന്തര്ഭാവ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#അന്തര്ഭാവ "V3Descope.h"

#അന്തര്ഭാവ "V3ConstPool.h"
#അന്തര്ഭാവ "V3EmitCBase.h"

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################

വർഗ്ഗം DescopeVisitor final : പൊതു VNVisitor {
    // NODE STATE
    //  Cleared entire netlist
    //   AstCFunc::user()               // bool.  Indicates processing completed
    സ്ഥിരം VNUser1InUse m_inuser1;

    // TYPES
    using FuncMmap = std::multimap<std::string, AstCFunc*>;
    ഘടന ScopeSelfPtr final {
        VSelfPointerText thisPtr = VSelfPointerText{VSelfPointerText::Empty()};
        VSelfPointerText vlSymsPtr = VSelfPointerText{VSelfPointerText::Empty()};
    };

    // STATE
    Ast_കണ്ണി_Module* m_modp = nullptr;  // Current module
    AstScope* m_scopep = nullptr;  // Current scope
    സ്ഥിരം AstCFunc* m_funcp = nullptr;  // Current function
    തർക്കം m_modSingleton = അസത്യം;  // m_modp is only instantiated once
    FuncMmap m_modFuncs;  // Name of public functions added
    std::map<സ്ഥിരം AstScope*, ScopeSelfPtr> m_scopeToSelf;  // Scope to self pointers

    // METHODS

    സ്ഥിതം തർക്കം modIsSingleton(സ്ഥിരം Ast_കണ്ണി_Module* modp) {
        // True iff there's exactly one instance of this module in the design (including top).
        എങ്കിൽ (modp->is_മുകൾ()) തിരികെ സത്യം;
        പൂർണ്ണസംഖ്യ instances = 0;
        വേണ്ടി (സ്ഥിരം Ast_കണ്ണി* stmtp = modp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            എങ്കിൽ (VN_IS(stmtp, Scope)) {
                എങ്കിൽ (++instances > 1) തിരികെ അസത്യം;
            }
        }
        തിരികെ (instances == 1);
    }

    // Construct a 'this' self pointer for the given scope
    VSelfPointerText scopeThis(സ്ഥിരം AstScope* scopep) {
        auto& ret = m_scopeToSelf[scopep];
        എങ്കിൽ (ret.thisPtr.isEmpty()) {
            string name = scopep->name();
            string::size_type pos = name.rfind('.');
            എങ്കിൽ (pos != string::npos) name.erase(0, pos + 1);
            ret.thisPtr = VSelfPointerText{VSelfPointerText::This{}, name};
        }
        തിരികെ ret.thisPtr;
    }
    // Construct a 'vlSyms' self pointer for the given scope
    VSelfPointerText scopeVlSyms(സ്ഥിരം AstScope* scopep) {
        auto& ret = m_scopeToSelf[scopep];
        എങ്കിൽ (ret.vlSymsPtr.isEmpty()) {
            ret.vlSymsPtr = VSelfPointerText{VSelfPointerText::VlSyms(), scopep->nameDotless()};
        }
        തിരികെ ret.vlSymsPtr;
    }

    // Construct the best self pointer to reference an object in 'scopep'  from a CFunc in
    // 'm_scopep'. Result may be relative ("this->[...]") or absolute ("vlSyms->[...]").
    //
    // Using relative references allows V3Combine'ing code across multiple instances of the same
    // module.
    VSelfPointerText descopedSelfPointer(സ്ഥിരം AstScope* scopep) {
        UASSERT(scopep, "Var/Func not scoped");
        // Static functions can't use relative references via 'this->'
        സ്ഥിരം തർക്കം relativeRefOk = !m_funcp->isStatic();

        UINFO(8, "      Descope ref under " << m_scopep);
        UINFO(8, "              ref to    " << scopep);
        UINFO(8, "             aboveScope " << scopep->aboveScopep());

        എങ്കിൽ (VN_IS(scopep->modp(), Class)) {
            // Direct reference to class members are from within the class itself, references from
            // outside the class must go via AstMemberSel
            തിരികെ VSelfPointerText{VSelfPointerText::This{}};
        } അല്ലെങ്കിൽ എങ്കിൽ (relativeRefOk && scopep == m_scopep) {
            തിരികെ VSelfPointerText{VSelfPointerText::This{}};
        } അല്ലെങ്കിൽ എങ്കിൽ (relativeRefOk && !m_modSingleton && scopep->aboveScopep() == m_scopep
                   && VN_IS(scopep->modp(), Module)) {
            // Reference to scope of instance directly under this module, can just "this->cell",
            // which can potentially be V3Combined, but note this requires one extra pointer
            // dereference which is slower, so we only use it if the source scope is not a
            // singleton.
            തിരികെ scopeThis(scopep);
        } അല്ലെങ്കിൽ {
            // Reference to something elsewhere, or relative references are disabled. Use global
            // variable
            തിരികെ scopeVlSyms(scopep);
        }
    }

    ശൂന്യം makePublicFuncWrappers() {
        // We recorded all public functions in m_modFuncs.
        // If for any given name only one function exists, we can use that function directly.
        // If multiple functions exist, we need to select the appropriate scope.
        വേണ്ടി (FuncMmap::iterator it = m_modFuncs.begin(); it != m_modFuncs.end(); ++it) {
            സ്ഥിരം string name = it->first;
            AstCFunc* സ്ഥിരം മുകൾ_Funcp = it->second;
            auto nextIt1 = it;
            ++nextIt1;
            സ്ഥിരം തർക്കം moreOfSame1 = (nextIt1 != m_modFuncs.end() && nextIt1->first == name);
            എങ്കിൽ (moreOfSame1) {
                // Multiple functions under this name, need a wrapper function
                UINFO(6, "  Wrapping " << name << " multifuncs");
                AstCFunc* സ്ഥിരം newfuncp = മുകൾ_Funcp->clone_വൃക്ഷം(അസത്യം);
                എങ്കിൽ (newfuncp->varsp()) newfuncp->varsp()->unlinkFrBackWithNext()->delete_വൃക്ഷം();
                എങ്കിൽ (newfuncp->stmtsp()) newfuncp->stmtsp()->unlinkFrBackWithNext()->delete_വൃക്ഷം();
                newfuncp->name(name);
                newfuncp->isStatic(അസത്യം);
                മുകൾ_Funcp->addNextHere(newfuncp);
                // In the body, call each function if it matches the given scope
                വേണ്ടി (FuncMmap::iterator eachIt = it;
                     eachIt != m_modFuncs.end() && eachIt->first == name; ++eachIt) {
                    it = eachIt;
                    AstCFunc* സ്ഥിരം funcp = eachIt->second;
                    auto nextIt2 = eachIt;
                    ++nextIt2;
                    സ്ഥിരം തർക്കം moreOfSame
                        = (nextIt2 != m_modFuncs.end() && nextIt2->first == name);
                    UASSERT_OBJ(funcp->scopep(), funcp, "Not scoped");

                    UINFO(6, "     Wrapping " << name << " " << funcp);
                    UINFO(6, "  at " << newfuncp->argTypes() << " und " << funcp->argTypes());
                    funcp->declPrivate(സത്യം);
                    AstVarRef* argsp = nullptr;
                    വേണ്ടി (Ast_കണ്ണി* stmtp = newfuncp->argsp(); stmtp; stmtp = stmtp->nextp()) {
                        എങ്കിൽ (AstVar* സ്ഥിരം portp = VN_CAST(stmtp, Var)) {
                            എങ്കിൽ (portp->isIO() && !portp->isFuncReturn()) {
                                AstVarRef* സ്ഥിരം newp = പുതിയ AstVarRef{
                                    portp->fileline(), portp, portp->direction().pinAccess()};
                                argsp = Ast_കണ്ണി::addNext(argsp, newp);
                            }
                        }
                    }

                    AstCCall* സ്ഥിരം callp = പുതിയ AstCCall{funcp->fileline(), funcp, argsp};
                    എങ്കിൽ (Ast_കണ്ണി_DType* സ്ഥിരം dtypep = funcp->dtypep()) {
                        callp->dtypep(dtypep);
                    } അല്ലെങ്കിൽ {
                        callp->dtypeSetVoid();
                    }
                    Ast_കണ്ണി* സ്ഥിരം returnp = പുതിയ AstCReturn{funcp->fileline(), callp};

                    എങ്കിൽ (moreOfSame) {
                        AstIf* സ്ഥിരം ifp = പുതിയ AstIf{
                            funcp->fileline(),
                            പുതിയ AstEq{funcp->fileline(),
                                      പുതിയ AstCExpr{funcp->fileline(), "this", 64},
                                      പുതിയ AstCExpr{funcp->fileline(),
                                                   scopeVlSyms(funcp->scopep()).asString(), 64}},
                            returnp};
                        newfuncp->addStmtsp(ifp);
                    } അല്ലെങ്കിൽ {
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
            } അല്ലെങ്കിൽ {
                // Only a single function under this name, we can rename it
                UINFO(6, "  Wrapping " << name << " just one " << മുകൾ_Funcp);
                മുകൾ_Funcp->name(name);
            }
        }
    }

    // VISITORS
    ശൂന്യം visit(Ast_കണ്ണി_Module* nodep) override {
        VL_RESTORER(m_modp);
        m_modp = nodep;
        m_modFuncs.clear();
        m_modSingleton = modIsSingleton(m_modp);
        iterateChildren(nodep);
        makePublicFuncWrappers();
    }
    ശൂന്യം visit(AstScope* nodep) override {
        VL_RESTORER(m_scopep);
        m_scopep = nodep;
        iterateChildren(nodep);
    }
    ശൂന്യം visit(AstVarScope* nodep) override {
        // Delete the varscope when we're finished
        VL_DO_DANGLING(തള്ളുക_Deletep(nodep->unlinkFrBack()), nodep);
    }
    ശൂന്യം visit(Ast_കണ്ണി_Var_Ref* nodep) override {
        iterateChildren(nodep);
        എങ്കിൽ (!nodep->varScopep()) {
            UASSERT_OBJ(nodep->varp()->isFuncLocal(), nodep,
                        "unscoped reference can only appear to function locals at this point");
            തിരികെ;
        }
        // Convert the hierch name
        UINFO(9, "  ref-in " << nodep);
        UASSERT_OBJ(m_scopep, nodep, "Node not under scope");
        സ്ഥിരം AstVar* സ്ഥിരം varp = nodep->varScopep()->varp();
        സ്ഥിരം AstScope* സ്ഥിരം scopep = nodep->varScopep()->scopep();
        എങ്കിൽ (varp->isFuncLocal()) {
            // Reference to function locals need no self pointer
            nodep->selfPointer(VSelfPointerText{VSelfPointerText::Empty()});
        } അല്ലെങ്കിൽ എങ്കിൽ (scopep->modp()->isConstPool()) {
            // Reference to constant pool value need no self pointer
            nodep->selfPointer(VSelfPointerText{VSelfPointerText::Empty()});
        } അല്ലെങ്കിൽ {
            nodep->selfPointer(descopedSelfPointer(scopep));
        }
        nodep->varScopep(nullptr);
        UINFO(9, "  refout " << nodep << " selfPtr=" << nodep->selfPointer().asString());
    }
    ശൂന്യം visit(AstCCall* nodep) override {
        // UINFO(9, "       " << nodep);
        iterateChildren(nodep);
        // Convert the hierch name
        UASSERT_OBJ(m_scopep, nodep, "Node not under scope");
        സ്ഥിരം AstScope* സ്ഥിരം scopep = nodep->funcp()->scopep();
        nodep->selfPointer(descopedSelfPointer(scopep));
        // Can't do this, as we may have more calls later
        // nodep->funcp()->scopep(nullptr);
    }
    ശൂന്യം visit(AstScopeName* nodep) override {
        // Print a %m under this instance as its vlNamep, which holds its pretty name, plus the
        // rest of the path, so the instances' code can be V3Combined. Not with --protect-ids, as
        // vlNamep then holds the protected name, while %m prints the original.
        എങ്കിൽ (!m_funcp || !nodep->forFormat() || m_modSingleton || m_funcp->isStatic()
            || v3Global.opt.protectIds()) {
            തിരികെ;
        }
        std::string path = nodep->scopePrettySymName();
        സ്ഥിരം std::string inst = m_scopep->prettyName();
        എങ്കിൽ (path != inst && !VString::startsWith(path, inst + ".")) തിരികെ;
        path.erase(0, inst.size() + 1);
        // The DPI scope table keeps the absolute name, via a copy outside any function
        m_scopep->addBlocksp(nodep->clone_വൃക്ഷം(അസത്യം));
        nodep->scopeAttr(path);
        nodep->scopeEntr("");  // Only used by the DPI scope table
        nodep->selfRelative(സത്യം);
    }
    ശൂന്യം visit(AstCMethodCall* nodep) override { iterateChildren(nodep); }
    ശൂന്യം visit(AstCNew* nodep) override { iterateChildren(nodep); }
    ശൂന്യം visit(AstCFunc* nodep) override {
        VL_RESTORER(m_funcp);
        എങ്കിൽ (!nodep->user1()) {
            // Static functions should have been moved under the corresponding AstClassPackage
            UASSERT_OBJ(!(nodep->isStatic() && VN_IS(m_modp, Class)), nodep,
                        "Static function under AstClass");
            m_funcp = nodep;
            iterateChildren(nodep);
            nodep->user1(സത്യം);
            // If it's under a scope, move it up to the top
            എങ്കിൽ (m_scopep) {
                nodep->unlinkFrBack();
                m_modp->addStmtsp(nodep);

                എങ്കിൽ (nodep->funcPublic()) {
                    // There may be multiple public functions by the same name;
                    // record for later correction or making of shells
                    m_modFuncs.emplace(nodep->name(), nodep);
                    nodep->name(m_scopep->nameDotless() + "__" + nodep->name());
                }
            }
        }
    }
    ശൂന്യം visit(AstVar*) override {}
    ശൂന്യം visit(Ast_കണ്ണി* nodep) override { iterateChildren(nodep); }

പൊതു:
    // CONSTRUCTORS
    explicit DescopeVisitor(AstNetlist* nodep) { iterate(nodep); }
    ~DescopeVisitor() override = സ്ഥിരസ്ഥിതി;
};

//######################################################################
// Descope class functions

ശൂന്യം V3Descope::descopeAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { DescopeVisitor{nodep}; }  // Destruct before checking
    V3ConstPool::setDescoped();
    V3Global::dump_Check_Global_വൃക്ഷം("descope", 0, dump_വൃക്ഷം_Either_Level() >= 3);
}
