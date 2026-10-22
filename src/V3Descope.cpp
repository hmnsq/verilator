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
    ध्रुव VNUser1_अन्तर्_Use m_inuser1;

    // TYPES
    उपयुज्य FuncMmap = std::multimap<std::string, AstCFunc*>;
    संरचना ScopeSelfPtr final {
        VSelfPointerText thisPtr = VSelfPointerText{VSelfPointerText::रिक्तानि_2()};
        VSelfPointerText vlSymsPtr = VSelfPointerText{VSelfPointerText::रिक्तानि_2()};
    };

    // STATE
    Ast_ग्रन्थिः_Module* m_modp = शून्यसूचकः;  // Current module
    AstScope* m_scopep = शून्यसूचकः;  // Current scope
    ध्रुव AstCFunc* m_funcp = शून्यसूचकः;  // Current function
    तार्किक m_modSingleton = असत्य;  // m_modp is only instantiated once
    FuncMmap m_modFuncs;  // Name of public functions added
    std::map<ध्रुव AstScope*, ScopeSelfPtr> m_scope_गन्तव्यम्_Self;  // Scope to self pointers

    // METHODS

    स्थिर तार्किक modIsSingleton(ध्रुव Ast_ग्रन्थिः_Module* modp) {
        // True iff there's exactly one instance of this module in the design (including top).
        यदि (modp->is_शिखरम्()) प्रत्यागम सत्य;
        पूर्णाङ्क instances = 0;
        प्रति (ध्रुव Ast_ग्रन्थिः* stmtp = modp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            यदि (VN_IS(stmtp, Scope)) {
                यदि (++instances > 1) प्रत्यागम असत्य;
            }
        }
        प्रत्यागम (instances == 1);
    }

    // Construct a 'this' self pointer for the given scope
    VSelfPointerText scopeThis(ध्रुव AstScope* scopep) {
        स्वचालित& ret = m_scope_गन्तव्यम्_Self[scopep];
        यदि (ret.thisPtr.is_रिक्तानि()) {
            string name = scopep->name();
            string::आकार_type pos = name.rfind('.');
            यदि (pos != string::npos) name.erase(0, pos + 1);
            ret.thisPtr = VSelfPointerText{VSelfPointerText::This{}, name};
        }
        प्रत्यागम ret.thisPtr;
    }
    // Construct a 'vlSyms' self pointer for the given scope
    VSelfPointerText scopeVlSyms(ध्रुव AstScope* scopep) {
        स्वचालित& ret = m_scope_गन्तव्यम्_Self[scopep];
        यदि (ret.vlSymsPtr.is_रिक्तानि()) {
            ret.vlSymsPtr = VSelfPointerText{VSelfPointerText::VlSyms(), scopep->nameDotless()};
        }
        प्रत्यागम ret.vlSymsPtr;
    }

    // Construct the best self pointer to reference an object in 'scopep'  from a CFunc in
    // 'm_scopep'. Result may be relative ("this->[...]") or absolute ("vlSyms->[...]").
    //
    // Using relative references allows V3Combine'ing code across multiple instances of the same
    // module.
    VSelfPointerText descopedSelfPointer(ध्रुव AstScope* scopep) {
        UASSERT(scopep, "Var/Func not scoped");
        // Static functions can't use relative references via 'this->'
        ध्रुव तार्किक relativeRefOk = !m_funcp->isStatic();

        UINFO(8, "      Descope ref under " << m_scopep);
        UINFO(8, "              ref to    " << scopep);
        UINFO(8, "             aboveScope " << scopep->aboveScopep());

        यदि (VN_IS(scopep->modp(), Class)) {
            // Direct reference to class members are from within the class itself, references from
            // outside the class must go via AstMemberSel
            प्रत्यागम VSelfPointerText{VSelfPointerText::This{}};
        } अन्यथा यदि (relativeRefOk && scopep == m_scopep) {
            प्रत्यागम VSelfPointerText{VSelfPointerText::This{}};
        } अन्यथा यदि (relativeRefOk && !m_modSingleton && scopep->aboveScopep() == m_scopep
                   && VN_IS(scopep->modp(), Module)) {
            // Reference to scope of instance directly under this module, can just "this->cell",
            // which can potentially be V3Combined, but note this requires one extra pointer
            // dereference which is slower, so we only use it if the source scope is not a
            // singleton.
            प्रत्यागम scopeThis(scopep);
        } अन्यथा {
            // Reference to something elsewhere, or relative references are disabled. Use global
            // variable
            प्रत्यागम scopeVlSyms(scopep);
        }
    }

    शून्य make_लौकिकम्_Func_Wrappers() {
        // We recorded all public functions in m_modFuncs.
        // If for any given name only one function exists, we can use that function directly.
        // If multiple functions exist, we need to select the appropriate scope.
        प्रति (FuncMmap::iterator it = m_modFuncs.begin(); it != m_modFuncs.अन्तः_2(); ++it) {
            ध्रुव string name = it->first;
            AstCFunc* ध्रुव शिखरम्_Funcp = it->second;
            स्वचालित अग्र_It1 = it;
            ++अग्र_It1;
            ध्रुव तार्किक अधिकम्_सम्बन्धि_Same1 = (अग्र_It1 != m_modFuncs.अन्तः_2() && अग्र_It1->first == name);
            यदि (अधिकम्_सम्बन्धि_Same1) {
                // Multiple functions under this name, need a wrapper function
                UINFO(6, "  Wrapping " << name << " multifuncs");
                AstCFunc* ध्रुव newfuncp = शिखरम्_Funcp->clone_वृक्षः(असत्य);
                यदि (newfuncp->varsp()) newfuncp->varsp()->unlink_Fr_पृष्ठभागः_With_अग्र()->delete_वृक्षः();
                यदि (newfuncp->stmtsp()) newfuncp->stmtsp()->unlink_Fr_पृष्ठभागः_With_अग्र()->delete_वृक्षः();
                newfuncp->name(name);
                newfuncp->isStatic(असत्य);
                शिखरम्_Funcp->सङ्कलय_अग्र_अत्र(newfuncp);
                // In the body, call each function if it matches the given scope
                प्रति (FuncMmap::iterator eachIt = it;
                     eachIt != m_modFuncs.अन्तः_2() && eachIt->first == name; ++eachIt) {
                    it = eachIt;
                    AstCFunc* ध्रुव funcp = eachIt->second;
                    स्वचालित अग्र_It2 = eachIt;
                    ++अग्र_It2;
                    ध्रुव तार्किक अधिकम्_सम्बन्धि_Same
                        = (अग्र_It2 != m_modFuncs.अन्तः_2() && अग्र_It2->first == name);
                    UASSERT_OBJ(funcp->scopep(), funcp, "Not scoped");

                    UINFO(6, "     Wrapping " << name << " " << funcp);
                    UINFO(6, "  at " << newfuncp->argTypes() << " und " << funcp->argTypes());
                    funcp->decl_वैयक्तिकम्(सत्य);
                    AstVarRef* argsp = शून्यसूचकः;
                    प्रति (Ast_ग्रन्थिः* stmtp = newfuncp->argsp(); stmtp; stmtp = stmtp->nextp()) {
                        यदि (AstVar* ध्रुव portp = VN_CAST(stmtp, Var)) {
                            यदि (portp->isIO() && !portp->isFuncReturn()) {
                                AstVarRef* ध्रुव newp = नूतन AstVarRef{
                                    portp->fileline(), portp, portp->direction().pinAccess()};
                                argsp = Ast_ग्रन्थिः::सङ्कलय_अग्र(argsp, newp);
                            }
                        }
                    }

                    AstCCall* ध्रुव callp = नूतन AstCCall{funcp->fileline(), funcp, argsp};
                    यदि (Ast_ग्रन्थिः_DType* ध्रुव dtypep = funcp->dtypep()) {
                        callp->dtypep(dtypep);
                    } अन्यथा {
                        callp->dtype_समुच्चयः_Void();
                    }
                    Ast_ग्रन्थिः* ध्रुव returnp = नूतन AstCReturn{funcp->fileline(), callp};

                    यदि (अधिकम्_सम्बन्धि_Same) {
                        AstIf* ध्रुव ifp = नूतन AstIf{
                            funcp->fileline(),
                            नूतन AstEq{funcp->fileline(),
                                      नूतन AstCExpr{funcp->fileline(), "this", 64},
                                      नूतन AstCExpr{funcp->fileline(),
                                                   scopeVlSyms(funcp->scopep()).asString(), 64}},
                            returnp};
                        newfuncp->सङ्कलय_Stmtsp(ifp);
                    } अन्यथा {
                        newfuncp->सङ्कलय_Stmtsp(returnp);
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
                UINFO(6, "  Wrapping " << name << " just one " << शिखरम्_Funcp);
                शिखरम्_Funcp->name(name);
            }
        }
    }

    // VISITORS
    शून्य दर्शय(Ast_ग्रन्थिः_Module* nodep) override {
        VL_RESTORER(m_modp);
        m_modp = nodep;
        m_modFuncs.clear();
        m_modSingleton = modIsSingleton(m_modp);
        iterateChildren(nodep);
        make_लौकिकम्_Func_Wrappers();
    }
    शून्य दर्शय(AstScope* nodep) override {
        VL_RESTORER(m_scopep);
        m_scopep = nodep;
        iterateChildren(nodep);
    }
    शून्य दर्शय(AstVarScope* nodep) override {
        // Delete the varscope when we're finished
        VL_DO_DANGLING(प्रक्षिप_Deletep(nodep->unlink_Fr_पृष्ठभागः()), nodep);
    }
    शून्य दर्शय(Ast_ग्रन्थिः_Var_Ref* nodep) override {
        iterateChildren(nodep);
        यदि (!nodep->varScopep()) {
            UASSERT_OBJ(nodep->varp()->isFuncLocal(), nodep,
                        "unscoped reference can only appear to function locals at this point");
            प्रत्यागम;
        }
        // Convert the hierch name
        UINFO(9, "  ref-in " << nodep);
        UASSERT_OBJ(m_scopep, nodep, "Node not under scope");
        ध्रुव AstVar* ध्रुव varp = nodep->varScopep()->varp();
        ध्रुव AstScope* ध्रुव scopep = nodep->varScopep()->scopep();
        यदि (varp->isFuncLocal()) {
            // Reference to function locals need no self pointer
            nodep->selfPointer(VSelfPointerText{VSelfPointerText::रिक्तानि_2()});
        } अन्यथा यदि (scopep->modp()->isConstPool()) {
            // Reference to constant pool value need no self pointer
            nodep->selfPointer(VSelfPointerText{VSelfPointerText::रिक्तानि_2()});
        } अन्यथा {
            nodep->selfPointer(descopedSelfPointer(scopep));
        }
        nodep->varScopep(शून्यसूचकः);
        UINFO(9, "  refout " << nodep << " selfPtr=" << nodep->selfPointer().asString());
    }
    शून्य दर्शय(AstCCall* nodep) override {
        // UINFO(9, "       " << nodep);
        iterateChildren(nodep);
        // Convert the hierch name
        UASSERT_OBJ(m_scopep, nodep, "Node not under scope");
        ध्रुव AstScope* ध्रुव scopep = nodep->funcp()->scopep();
        nodep->selfPointer(descopedSelfPointer(scopep));
        // Can't do this, as we may have more calls later
        // nodep->funcp()->scopep(nullptr);
    }
    शून्य दर्शय(AstScopeName* nodep) override {
        // Print a %m under this instance as its vlNamep, which holds its pretty name, plus the
        // rest of the path, so the instances' code can be V3Combined. Not with --protect-ids, as
        // vlNamep then holds the protected name, while %m prints the original.
        यदि (!m_funcp || !nodep->forFormat() || m_modSingleton || m_funcp->isStatic()
            || v3Global.opt.protectIds()) {
            प्रत्यागम;
        }
        std::string मार्गः = nodep->scopePrettySymName();
        ध्रुव std::string inst = m_scopep->prettyName();
        यदि (मार्गः != inst && !VString::startsWith(मार्गः, inst + ".")) प्रत्यागम;
        मार्गः.erase(0, inst.आकार() + 1);
        // The DPI scope table keeps the absolute name, via a copy outside any function
        m_scopep->सङ्कलय_Blocksp(nodep->clone_वृक्षः(असत्य));
        nodep->scopeAttr(मार्गः);
        nodep->scopeEntr("");  // Only used by the DPI scope table
        nodep->selfRelative(सत्य);
    }
    शून्य दर्शय(AstCMethodCall* nodep) override { iterateChildren(nodep); }
    शून्य दर्शय(AstCNew* nodep) override { iterateChildren(nodep); }
    शून्य दर्शय(AstCFunc* nodep) override {
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
                nodep->unlink_Fr_पृष्ठभागः();
                m_modp->सङ्कलय_Stmtsp(nodep);

                यदि (nodep->func_लौकिकम्()) {
                    // There may be multiple public functions by the same name;
                    // record for later correction or making of shells
                    m_modFuncs.emplace(nodep->name(), nodep);
                    nodep->name(m_scopep->nameDotless() + "__" + nodep->name());
                }
            }
        }
    }
    शून्य दर्शय(AstVar*) override {}
    शून्य दर्शय(Ast_ग्रन्थिः* nodep) override { iterateChildren(nodep); }

सार्वजनिक:
    // CONSTRUCTORS
    स्पष्ट DescopeVisitor(AstNetlist* nodep) { iterate(nodep); }
    ~DescopeVisitor() override = स्वभाव;
};

//######################################################################
// Descope class functions

शून्य V3Descope::descopeAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { DescopeVisitor{nodep}; }  // Destruct before checking
    V3ConstPool::समुच्चयः_Descoped();
    V3Global::dump_Check_Global_वृक्षः("descope", 0, dump_वृक्षः_Either_Level() >= 3);
}
