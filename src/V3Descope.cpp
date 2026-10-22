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

#அந்தர்பாவ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#அந்தர்பாவ "V3Descope.h"

#அந்தர்பாவ "V3ConstPool.h"
#அந்தர்பாவ "V3EmitCBase.h"

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################

வகுப்பு DescopeVisitor final : பொது VNVisitor {
    // NODE STATE
    //  Cleared entire netlist
    //   AstCFunc::user()               // bool.  Indicates processing completed
    மாறிலி VNUser1InUse m_inuser1;

    // TYPES
    using FuncMmap = std::multimap<std::string, AstCFunc*>;
    கட்டமைப்பு ScopeSelfPtr final {
        VSelfPointerText thisPtr = VSelfPointerText{VSelfPointerText::Empty()};
        VSelfPointerText vlSymsPtr = VSelfPointerText{VSelfPointerText::Empty()};
    };

    // STATE
    Ast_முனை_Module* m_modp = nullptr;  // Current module
    AstScope* m_scopep = nullptr;  // Current scope
    மாறிலி AstCFunc* m_funcp = nullptr;  // Current function
    தருக்க m_modSingleton = பொய்;  // m_modp is only instantiated once
    FuncMmap m_modFuncs;  // Name of public functions added
    std::map<மாறிலி AstScope*, ScopeSelfPtr> m_scopeToSelf;  // Scope to self pointers

    // METHODS

    நிலையான தருக்க modIsSingleton(மாறிலி Ast_முனை_Module* modp) {
        // True iff there's exactly one instance of this module in the design (including top).
        எனில் (modp->is_உச்சி()) திரும்பு உண்மை;
        முழு instances = 0;
        க்கு (மாறிலி Ast_முனை* stmtp = modp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            எனில் (VN_IS(stmtp, Scope)) {
                எனில் (++instances > 1) திரும்பு பொய்;
            }
        }
        திரும்பு (instances == 1);
    }

    // Construct a 'this' self pointer for the given scope
    VSelfPointerText scopeThis(மாறிலி AstScope* scopep) {
        auto& ret = m_scopeToSelf[scopep];
        எனில் (ret.thisPtr.isEmpty()) {
            string name = scopep->name();
            string::size_type pos = name.rfind('.');
            எனில் (pos != string::npos) name.erase(0, pos + 1);
            ret.thisPtr = VSelfPointerText{VSelfPointerText::This{}, name};
        }
        திரும்பு ret.thisPtr;
    }
    // Construct a 'vlSyms' self pointer for the given scope
    VSelfPointerText scopeVlSyms(மாறிலி AstScope* scopep) {
        auto& ret = m_scopeToSelf[scopep];
        எனில் (ret.vlSymsPtr.isEmpty()) {
            ret.vlSymsPtr = VSelfPointerText{VSelfPointerText::VlSyms(), scopep->nameDotless()};
        }
        திரும்பு ret.vlSymsPtr;
    }

    // Construct the best self pointer to reference an object in 'scopep'  from a CFunc in
    // 'm_scopep'. Result may be relative ("this->[...]") or absolute ("vlSyms->[...]").
    //
    // Using relative references allows V3Combine'ing code across multiple instances of the same
    // module.
    VSelfPointerText descopedSelfPointer(மாறிலி AstScope* scopep) {
        UASSERT(scopep, "Var/Func not scoped");
        // Static functions can't use relative references via 'this->'
        மாறிலி தருக்க relativeRefOk = !m_funcp->isStatic();

        UINFO(8, "      Descope ref under " << m_scopep);
        UINFO(8, "              ref to    " << scopep);
        UINFO(8, "             aboveScope " << scopep->aboveScopep());

        எனில் (VN_IS(scopep->modp(), Class)) {
            // Direct reference to class members are from within the class itself, references from
            // outside the class must go via AstMemberSel
            திரும்பு VSelfPointerText{VSelfPointerText::This{}};
        } இல்லையெனில் எனில் (relativeRefOk && scopep == m_scopep) {
            திரும்பு VSelfPointerText{VSelfPointerText::This{}};
        } இல்லையெனில் எனில் (relativeRefOk && !m_modSingleton && scopep->aboveScopep() == m_scopep
                   && VN_IS(scopep->modp(), Module)) {
            // Reference to scope of instance directly under this module, can just "this->cell",
            // which can potentially be V3Combined, but note this requires one extra pointer
            // dereference which is slower, so we only use it if the source scope is not a
            // singleton.
            திரும்பு scopeThis(scopep);
        } இல்லையெனில் {
            // Reference to something elsewhere, or relative references are disabled. Use global
            // variable
            திரும்பு scopeVlSyms(scopep);
        }
    }

    வெறுமை makePublicFuncWrappers() {
        // We recorded all public functions in m_modFuncs.
        // If for any given name only one function exists, we can use that function directly.
        // If multiple functions exist, we need to select the appropriate scope.
        க்கு (FuncMmap::iterator it = m_modFuncs.begin(); it != m_modFuncs.end(); ++it) {
            மாறிலி string name = it->first;
            AstCFunc* மாறிலி உச்சி_Funcp = it->second;
            auto nextIt1 = it;
            ++nextIt1;
            மாறிலி தருக்க moreOfSame1 = (nextIt1 != m_modFuncs.end() && nextIt1->first == name);
            எனில் (moreOfSame1) {
                // Multiple functions under this name, need a wrapper function
                UINFO(6, "  Wrapping " << name << " multifuncs");
                AstCFunc* மாறிலி newfuncp = உச்சி_Funcp->clone_மரம்(பொய்);
                எனில் (newfuncp->varsp()) newfuncp->varsp()->unlinkFrBackWithNext()->delete_மரம்();
                எனில் (newfuncp->stmtsp()) newfuncp->stmtsp()->unlinkFrBackWithNext()->delete_மரம்();
                newfuncp->name(name);
                newfuncp->isStatic(பொய்);
                உச்சி_Funcp->addNextHere(newfuncp);
                // In the body, call each function if it matches the given scope
                க்கு (FuncMmap::iterator eachIt = it;
                     eachIt != m_modFuncs.end() && eachIt->first == name; ++eachIt) {
                    it = eachIt;
                    AstCFunc* மாறிலி funcp = eachIt->second;
                    auto nextIt2 = eachIt;
                    ++nextIt2;
                    மாறிலி தருக்க moreOfSame
                        = (nextIt2 != m_modFuncs.end() && nextIt2->first == name);
                    UASSERT_OBJ(funcp->scopep(), funcp, "Not scoped");

                    UINFO(6, "     Wrapping " << name << " " << funcp);
                    UINFO(6, "  at " << newfuncp->argTypes() << " und " << funcp->argTypes());
                    funcp->declPrivate(உண்மை);
                    AstVarRef* argsp = nullptr;
                    க்கு (Ast_முனை* stmtp = newfuncp->argsp(); stmtp; stmtp = stmtp->nextp()) {
                        எனில் (AstVar* மாறிலி portp = VN_CAST(stmtp, Var)) {
                            எனில் (portp->isIO() && !portp->isFuncReturn()) {
                                AstVarRef* மாறிலி newp = புதிய AstVarRef{
                                    portp->fileline(), portp, portp->direction().pinAccess()};
                                argsp = Ast_முனை::addNext(argsp, newp);
                            }
                        }
                    }

                    AstCCall* மாறிலி callp = புதிய AstCCall{funcp->fileline(), funcp, argsp};
                    எனில் (Ast_முனை_DType* மாறிலி dtypep = funcp->dtypep()) {
                        callp->dtypep(dtypep);
                    } இல்லையெனில் {
                        callp->dtypeSetVoid();
                    }
                    Ast_முனை* மாறிலி returnp = புதிய AstCReturn{funcp->fileline(), callp};

                    எனில் (moreOfSame) {
                        AstIf* மாறிலி ifp = புதிய AstIf{
                            funcp->fileline(),
                            புதிய AstEq{funcp->fileline(),
                                      புதிய AstCExpr{funcp->fileline(), "this", 64},
                                      புதிய AstCExpr{funcp->fileline(),
                                                   scopeVlSyms(funcp->scopep()).asString(), 64}},
                            returnp};
                        newfuncp->addStmtsp(ifp);
                    } இல்லையெனில் {
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
            } இல்லையெனில் {
                // Only a single function under this name, we can rename it
                UINFO(6, "  Wrapping " << name << " just one " << உச்சி_Funcp);
                உச்சி_Funcp->name(name);
            }
        }
    }

    // VISITORS
    வெறுமை visit(Ast_முனை_Module* nodep) override {
        VL_RESTORER(m_modp);
        m_modp = nodep;
        m_modFuncs.clear();
        m_modSingleton = modIsSingleton(m_modp);
        iterateChildren(nodep);
        makePublicFuncWrappers();
    }
    வெறுமை visit(AstScope* nodep) override {
        VL_RESTORER(m_scopep);
        m_scopep = nodep;
        iterateChildren(nodep);
    }
    வெறுமை visit(AstVarScope* nodep) override {
        // Delete the varscope when we're finished
        VL_DO_DANGLING(தள்ளு_Deletep(nodep->unlinkFrBack()), nodep);
    }
    வெறுமை visit(Ast_முனை_Var_Ref* nodep) override {
        iterateChildren(nodep);
        எனில் (!nodep->varScopep()) {
            UASSERT_OBJ(nodep->varp()->isFuncLocal(), nodep,
                        "unscoped reference can only appear to function locals at this point");
            திரும்பு;
        }
        // Convert the hierch name
        UINFO(9, "  ref-in " << nodep);
        UASSERT_OBJ(m_scopep, nodep, "Node not under scope");
        மாறிலி AstVar* மாறிலி varp = nodep->varScopep()->varp();
        மாறிலி AstScope* மாறிலி scopep = nodep->varScopep()->scopep();
        எனில் (varp->isFuncLocal()) {
            // Reference to function locals need no self pointer
            nodep->selfPointer(VSelfPointerText{VSelfPointerText::Empty()});
        } இல்லையெனில் எனில் (scopep->modp()->isConstPool()) {
            // Reference to constant pool value need no self pointer
            nodep->selfPointer(VSelfPointerText{VSelfPointerText::Empty()});
        } இல்லையெனில் {
            nodep->selfPointer(descopedSelfPointer(scopep));
        }
        nodep->varScopep(nullptr);
        UINFO(9, "  refout " << nodep << " selfPtr=" << nodep->selfPointer().asString());
    }
    வெறுமை visit(AstCCall* nodep) override {
        // UINFO(9, "       " << nodep);
        iterateChildren(nodep);
        // Convert the hierch name
        UASSERT_OBJ(m_scopep, nodep, "Node not under scope");
        மாறிலி AstScope* மாறிலி scopep = nodep->funcp()->scopep();
        nodep->selfPointer(descopedSelfPointer(scopep));
        // Can't do this, as we may have more calls later
        // nodep->funcp()->scopep(nullptr);
    }
    வெறுமை visit(AstScopeName* nodep) override {
        // Print a %m under this instance as its vlNamep, which holds its pretty name, plus the
        // rest of the path, so the instances' code can be V3Combined. Not with --protect-ids, as
        // vlNamep then holds the protected name, while %m prints the original.
        எனில் (!m_funcp || !nodep->forFormat() || m_modSingleton || m_funcp->isStatic()
            || v3Global.opt.protectIds()) {
            திரும்பு;
        }
        std::string path = nodep->scopePrettySymName();
        மாறிலி std::string inst = m_scopep->prettyName();
        எனில் (path != inst && !VString::startsWith(path, inst + ".")) திரும்பு;
        path.erase(0, inst.size() + 1);
        // The DPI scope table keeps the absolute name, via a copy outside any function
        m_scopep->addBlocksp(nodep->clone_மரம்(பொய்));
        nodep->scopeAttr(path);
        nodep->scopeEntr("");  // Only used by the DPI scope table
        nodep->selfRelative(உண்மை);
    }
    வெறுமை visit(AstCMethodCall* nodep) override { iterateChildren(nodep); }
    வெறுமை visit(AstCNew* nodep) override { iterateChildren(nodep); }
    வெறுமை visit(AstCFunc* nodep) override {
        VL_RESTORER(m_funcp);
        எனில் (!nodep->user1()) {
            // Static functions should have been moved under the corresponding AstClassPackage
            UASSERT_OBJ(!(nodep->isStatic() && VN_IS(m_modp, Class)), nodep,
                        "Static function under AstClass");
            m_funcp = nodep;
            iterateChildren(nodep);
            nodep->user1(உண்மை);
            // If it's under a scope, move it up to the top
            எனில் (m_scopep) {
                nodep->unlinkFrBack();
                m_modp->addStmtsp(nodep);

                எனில் (nodep->funcPublic()) {
                    // There may be multiple public functions by the same name;
                    // record for later correction or making of shells
                    m_modFuncs.emplace(nodep->name(), nodep);
                    nodep->name(m_scopep->nameDotless() + "__" + nodep->name());
                }
            }
        }
    }
    வெறுமை visit(AstVar*) override {}
    வெறுமை visit(Ast_முனை* nodep) override { iterateChildren(nodep); }

பொது:
    // CONSTRUCTORS
    explicit DescopeVisitor(AstNetlist* nodep) { iterate(nodep); }
    ~DescopeVisitor() override = இயல்பு;
};

//######################################################################
// Descope class functions

வெறுமை V3Descope::descopeAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { DescopeVisitor{nodep}; }  // Destruct before checking
    V3ConstPool::setDescoped();
    V3Global::dump_Check_Global_மரம்("descope", 0, dump_மரம்_Either_Level() >= 3);
}
