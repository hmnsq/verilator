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

#شامل "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#شامل "V3Descope.h"

#شامل "V3ConstPool.h"
#شامل "V3EmitCBase.h"

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################

رده DescopeVisitor final : عمومی VNVisitor {
    // NODE STATE
    //  Cleared entire netlist
    //   AstCFunc::user()               // bool.  Indicates processing completed
    ثابت VNUser1InUse m_inuser1;

    // TYPES
    using FuncMmap = std::multimap<std::string, AstCFunc*>;
    ساختار ScopeSelfPtr final {
        VSelfPointerText thisPtr = VSelfPointerText{VSelfPointerText::Empty()};
        VSelfPointerText vlSymsPtr = VSelfPointerText{VSelfPointerText::Empty()};
    };

    // STATE
    Ast_گره_Module* m_modp = nullptr;  // Current module
    AstScope* m_scopep = nullptr;  // Current scope
    ثابت AstCFunc* m_funcp = nullptr;  // Current function
    منطقی m_modSingleton = نادرست;  // m_modp is only instantiated once
    FuncMmap m_modFuncs;  // Name of public functions added
    std::map<ثابت AstScope*, ScopeSelfPtr> m_scopeToSelf;  // Scope to self pointers

    // METHODS

    ایستا منطقی modIsSingleton(ثابت Ast_گره_Module* modp) {
        // True iff there's exactly one instance of this module in the design (including top).
        اگر (modp->is_بالاترین()) بازگشت درست;
        صحیح instances = 0;
        برای (ثابت Ast_گره* stmtp = modp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            اگر (VN_IS(stmtp, Scope)) {
                اگر (++instances > 1) بازگشت نادرست;
            }
        }
        بازگشت (instances == 1);
    }

    // Construct a 'this' self pointer for the given scope
    VSelfPointerText scopeThis(ثابت AstScope* scopep) {
        auto& ret = m_scopeToSelf[scopep];
        اگر (ret.thisPtr.isEmpty()) {
            string name = scopep->name();
            string::size_type pos = name.rfind('.');
            اگر (pos != string::npos) name.erase(0, pos + 1);
            ret.thisPtr = VSelfPointerText{VSelfPointerText::This{}, name};
        }
        بازگشت ret.thisPtr;
    }
    // Construct a 'vlSyms' self pointer for the given scope
    VSelfPointerText scopeVlSyms(ثابت AstScope* scopep) {
        auto& ret = m_scopeToSelf[scopep];
        اگر (ret.vlSymsPtr.isEmpty()) {
            ret.vlSymsPtr = VSelfPointerText{VSelfPointerText::VlSyms(), scopep->nameDotless()};
        }
        بازگشت ret.vlSymsPtr;
    }

    // Construct the best self pointer to reference an object in 'scopep'  from a CFunc in
    // 'm_scopep'. Result may be relative ("this->[...]") or absolute ("vlSyms->[...]").
    //
    // Using relative references allows V3Combine'ing code across multiple instances of the same
    // module.
    VSelfPointerText descopedSelfPointer(ثابت AstScope* scopep) {
        UASSERT(scopep, "Var/Func not scoped");
        // Static functions can't use relative references via 'this->'
        ثابت منطقی relativeRefOk = !m_funcp->isStatic();

        UINFO(8, "      Descope ref under " << m_scopep);
        UINFO(8, "              ref to    " << scopep);
        UINFO(8, "             aboveScope " << scopep->aboveScopep());

        اگر (VN_IS(scopep->modp(), Class)) {
            // Direct reference to class members are from within the class itself, references from
            // outside the class must go via AstMemberSel
            بازگشت VSelfPointerText{VSelfPointerText::This{}};
        } وگرنه اگر (relativeRefOk && scopep == m_scopep) {
            بازگشت VSelfPointerText{VSelfPointerText::This{}};
        } وگرنه اگر (relativeRefOk && !m_modSingleton && scopep->aboveScopep() == m_scopep
                   && VN_IS(scopep->modp(), Module)) {
            // Reference to scope of instance directly under this module, can just "this->cell",
            // which can potentially be V3Combined, but note this requires one extra pointer
            // dereference which is slower, so we only use it if the source scope is not a
            // singleton.
            بازگشت scopeThis(scopep);
        } وگرنه {
            // Reference to something elsewhere, or relative references are disabled. Use global
            // variable
            بازگشت scopeVlSyms(scopep);
        }
    }

    تهی makePublicFuncWrappers() {
        // We recorded all public functions in m_modFuncs.
        // If for any given name only one function exists, we can use that function directly.
        // If multiple functions exist, we need to select the appropriate scope.
        برای (FuncMmap::iterator it = m_modFuncs.begin(); it != m_modFuncs.end(); ++it) {
            ثابت string name = it->first;
            AstCFunc* ثابت بالاترین_Funcp = it->second;
            auto nextIt1 = it;
            ++nextIt1;
            ثابت منطقی moreOfSame1 = (nextIt1 != m_modFuncs.end() && nextIt1->first == name);
            اگر (moreOfSame1) {
                // Multiple functions under this name, need a wrapper function
                UINFO(6, "  Wrapping " << name << " multifuncs");
                AstCFunc* ثابت newfuncp = بالاترین_Funcp->clone_درخت(نادرست);
                اگر (newfuncp->varsp()) newfuncp->varsp()->unlinkFrBackWithNext()->delete_درخت();
                اگر (newfuncp->stmtsp()) newfuncp->stmtsp()->unlinkFrBackWithNext()->delete_درخت();
                newfuncp->name(name);
                newfuncp->isStatic(نادرست);
                بالاترین_Funcp->addNextHere(newfuncp);
                // In the body, call each function if it matches the given scope
                برای (FuncMmap::iterator eachIt = it;
                     eachIt != m_modFuncs.end() && eachIt->first == name; ++eachIt) {
                    it = eachIt;
                    AstCFunc* ثابت funcp = eachIt->second;
                    auto nextIt2 = eachIt;
                    ++nextIt2;
                    ثابت منطقی moreOfSame
                        = (nextIt2 != m_modFuncs.end() && nextIt2->first == name);
                    UASSERT_OBJ(funcp->scopep(), funcp, "Not scoped");

                    UINFO(6, "     Wrapping " << name << " " << funcp);
                    UINFO(6, "  at " << newfuncp->argTypes() << " und " << funcp->argTypes());
                    funcp->declPrivate(درست);
                    AstVarRef* argsp = nullptr;
                    برای (Ast_گره* stmtp = newfuncp->argsp(); stmtp; stmtp = stmtp->nextp()) {
                        اگر (AstVar* ثابت portp = VN_CAST(stmtp, Var)) {
                            اگر (portp->isIO() && !portp->isFuncReturn()) {
                                AstVarRef* ثابت newp = جدید AstVarRef{
                                    portp->fileline(), portp, portp->direction().pinAccess()};
                                argsp = Ast_گره::addNext(argsp, newp);
                            }
                        }
                    }

                    AstCCall* ثابت callp = جدید AstCCall{funcp->fileline(), funcp, argsp};
                    اگر (Ast_گره_DType* ثابت dtypep = funcp->dtypep()) {
                        callp->dtypep(dtypep);
                    } وگرنه {
                        callp->dtypeSetVoid();
                    }
                    Ast_گره* ثابت returnp = جدید AstCReturn{funcp->fileline(), callp};

                    اگر (moreOfSame) {
                        AstIf* ثابت ifp = جدید AstIf{
                            funcp->fileline(),
                            جدید AstEq{funcp->fileline(),
                                      جدید AstCExpr{funcp->fileline(), "this", 64},
                                      جدید AstCExpr{funcp->fileline(),
                                                   scopeVlSyms(funcp->scopep()).asString(), 64}},
                            returnp};
                        newfuncp->addStmtsp(ifp);
                    } وگرنه {
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
            } وگرنه {
                // Only a single function under this name, we can rename it
                UINFO(6, "  Wrapping " << name << " just one " << بالاترین_Funcp);
                بالاترین_Funcp->name(name);
            }
        }
    }

    // VISITORS
    تهی visit(Ast_گره_Module* nodep) override {
        VL_RESTORER(m_modp);
        m_modp = nodep;
        m_modFuncs.clear();
        m_modSingleton = modIsSingleton(m_modp);
        iterateChildren(nodep);
        makePublicFuncWrappers();
    }
    تهی visit(AstScope* nodep) override {
        VL_RESTORER(m_scopep);
        m_scopep = nodep;
        iterateChildren(nodep);
    }
    تهی visit(AstVarScope* nodep) override {
        // Delete the varscope when we're finished
        VL_DO_DANGLING(افزودن_Deletep(nodep->unlinkFrBack()), nodep);
    }
    تهی visit(Ast_گره_Var_Ref* nodep) override {
        iterateChildren(nodep);
        اگر (!nodep->varScopep()) {
            UASSERT_OBJ(nodep->varp()->isFuncLocal(), nodep,
                        "unscoped reference can only appear to function locals at this point");
            بازگشت;
        }
        // Convert the hierch name
        UINFO(9, "  ref-in " << nodep);
        UASSERT_OBJ(m_scopep, nodep, "Node not under scope");
        ثابت AstVar* ثابت varp = nodep->varScopep()->varp();
        ثابت AstScope* ثابت scopep = nodep->varScopep()->scopep();
        اگر (varp->isFuncLocal()) {
            // Reference to function locals need no self pointer
            nodep->selfPointer(VSelfPointerText{VSelfPointerText::Empty()});
        } وگرنه اگر (scopep->modp()->isConstPool()) {
            // Reference to constant pool value need no self pointer
            nodep->selfPointer(VSelfPointerText{VSelfPointerText::Empty()});
        } وگرنه {
            nodep->selfPointer(descopedSelfPointer(scopep));
        }
        nodep->varScopep(nullptr);
        UINFO(9, "  refout " << nodep << " selfPtr=" << nodep->selfPointer().asString());
    }
    تهی visit(AstCCall* nodep) override {
        // UINFO(9, "       " << nodep);
        iterateChildren(nodep);
        // Convert the hierch name
        UASSERT_OBJ(m_scopep, nodep, "Node not under scope");
        ثابت AstScope* ثابت scopep = nodep->funcp()->scopep();
        nodep->selfPointer(descopedSelfPointer(scopep));
        // Can't do this, as we may have more calls later
        // nodep->funcp()->scopep(nullptr);
    }
    تهی visit(AstScopeName* nodep) override {
        // Print a %m under this instance as its vlNamep, which holds its pretty name, plus the
        // rest of the path, so the instances' code can be V3Combined. Not with --protect-ids, as
        // vlNamep then holds the protected name, while %m prints the original.
        اگر (!m_funcp || !nodep->forFormat() || m_modSingleton || m_funcp->isStatic()
            || v3Global.opt.protectIds()) {
            بازگشت;
        }
        std::string path = nodep->scopePrettySymName();
        ثابت std::string inst = m_scopep->prettyName();
        اگر (path != inst && !VString::startsWith(path, inst + ".")) بازگشت;
        path.erase(0, inst.size() + 1);
        // The DPI scope table keeps the absolute name, via a copy outside any function
        m_scopep->addBlocksp(nodep->clone_درخت(نادرست));
        nodep->scopeAttr(path);
        nodep->scopeEntr("");  // Only used by the DPI scope table
        nodep->selfRelative(درست);
    }
    تهی visit(AstCMethodCall* nodep) override { iterateChildren(nodep); }
    تهی visit(AstCNew* nodep) override { iterateChildren(nodep); }
    تهی visit(AstCFunc* nodep) override {
        VL_RESTORER(m_funcp);
        اگر (!nodep->user1()) {
            // Static functions should have been moved under the corresponding AstClassPackage
            UASSERT_OBJ(!(nodep->isStatic() && VN_IS(m_modp, Class)), nodep,
                        "Static function under AstClass");
            m_funcp = nodep;
            iterateChildren(nodep);
            nodep->user1(درست);
            // If it's under a scope, move it up to the top
            اگر (m_scopep) {
                nodep->unlinkFrBack();
                m_modp->addStmtsp(nodep);

                اگر (nodep->funcPublic()) {
                    // There may be multiple public functions by the same name;
                    // record for later correction or making of shells
                    m_modFuncs.emplace(nodep->name(), nodep);
                    nodep->name(m_scopep->nameDotless() + "__" + nodep->name());
                }
            }
        }
    }
    تهی visit(AstVar*) override {}
    تهی visit(Ast_گره* nodep) override { iterateChildren(nodep); }

عمومی:
    // CONSTRUCTORS
    explicit DescopeVisitor(AstNetlist* nodep) { iterate(nodep); }
    ~DescopeVisitor() override = پیشفرض;
};

//######################################################################
// Descope class functions

تهی V3Descope::descopeAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { DescopeVisitor{nodep}; }  // Destruct before checking
    V3ConstPool::setDescoped();
    V3Global::dump_Check_Global_درخت("descope", 0, dump_درخت_Either_Level() >= 3);
}
