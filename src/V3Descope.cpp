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

#అన్తర్భావ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#అన్తర్భావ "V3Descope.h"

#అన్తర్భావ "V3ConstPool.h"
#అన్తర్భావ "V3EmitCBase.h"

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################

తరగతి DescopeVisitor final : బహిరంగ VNVisitor {
    // NODE STATE
    //  Cleared entire netlist
    //   AstCFunc::user()               // bool.  Indicates processing completed
    స్థిరరాశి VNUser1_లోపల_Use m_inuser1;

    // TYPES
    ఉపయోగించి FuncMmap = std::multimap<std::string, AstCFunc*>;
    నిర్మాణం ScopeSelfPtr final {
        VSelf_సూచిక_పాఠ్యం thisPtr = VSelf_సూచిక_పాఠ్యం{VSelf_సూచిక_పాఠ్యం::ఖాళీలు_2()};
        VSelf_సూచిక_పాఠ్యం vlSymsPtr = VSelf_సూచిక_పాఠ్యం{VSelf_సూచిక_పాఠ్యం::ఖాళీలు_2()};
    };

    // STATE
    Ast_కణుపు_Module* m_modp = శూన్యసూచిక;  // Current module
    AstScope* m_scopep = శూన్యసూచిక;  // Current scope
    స్థిరరాశి AstCFunc* m_funcp = శూన్యసూచిక;  // Current function
    తార్కిక m_modSingleton = అబద్ధం;  // m_modp is only instantiated once
    FuncMmap m_modFuncs;  // Name of public functions added
    std::map<స్థిరరాశి AstScope*, ScopeSelfPtr> m_scope_గమ్యం_Self;  // Scope to self pointers

    // METHODS

    స్థిర తార్కిక modIsSingleton(స్థిరరాశి Ast_కణుపు_Module* modp) {
        // True iff there's exactly one instance of this module in the design (including top).
        ఒకవేళ (modp->is_శిఖరం()) తిరిగి నిజం;
        పూర్ణాంకం instances = 0;
        కొరకు (స్థిరరాశి Ast_కణుపు* stmtp = modp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            ఒకవేళ (VN_IS(stmtp, Scope)) {
                ఒకవేళ (++instances > 1) తిరిగి అబద్ధం;
            }
        }
        తిరిగి (instances == 1);
    }

    // Construct a 'this' self pointer for the given scope
    VSelf_సూచిక_పాఠ్యం scopeThis(స్థిరరాశి AstScope* scopep) {
        స్వయంచాలక& ret = m_scope_గమ్యం_Self[scopep];
        ఒకవేళ (ret.thisPtr.is_ఖాళీలు()) {
            string name = scopep->name();
            string::కొలత_type pos = name.rfind('.');
            ఒకవేళ (pos != string::npos) name.erase(0, pos + 1);
            ret.thisPtr = VSelf_సూచిక_పాఠ్యం{VSelf_సూచిక_పాఠ్యం::This{}, name};
        }
        తిరిగి ret.thisPtr;
    }
    // Construct a 'vlSyms' self pointer for the given scope
    VSelf_సూచిక_పాఠ్యం scopeVlSyms(స్థిరరాశి AstScope* scopep) {
        స్వయంచాలక& ret = m_scope_గమ్యం_Self[scopep];
        ఒకవేళ (ret.vlSymsPtr.is_ఖాళీలు()) {
            ret.vlSymsPtr = VSelf_సూచిక_పాఠ్యం{VSelf_సూచిక_పాఠ్యం::VlSyms(), scopep->nameDotless()};
        }
        తిరిగి ret.vlSymsPtr;
    }

    // Construct the best self pointer to reference an object in 'scopep'  from a CFunc in
    // 'm_scopep'. Result may be relative ("this->[...]") or absolute ("vlSyms->[...]").
    //
    // Using relative references allows V3Combine'ing code across multiple instances of the same
    // module.
    VSelf_సూచిక_పాఠ్యం descoped_Self_సూచిక(స్థిరరాశి AstScope* scopep) {
        UASSERT(scopep, "Var/Func not scoped");
        // Static functions can't use relative references via 'this->'
        స్థిరరాశి తార్కిక relativeRefOk = !m_funcp->isStatic();

        UINFO(8, "      Descope ref under " << m_scopep);
        UINFO(8, "              ref to    " << scopep);
        UINFO(8, "             aboveScope " << scopep->aboveScopep());

        ఒకవేళ (VN_IS(scopep->modp(), Class)) {
            // Direct reference to class members are from within the class itself, references from
            // outside the class must go via AstMemberSel
            తిరిగి VSelf_సూచిక_పాఠ్యం{VSelf_సూచిక_పాఠ్యం::This{}};
        } లేకపోతే ఒకవేళ (relativeRefOk && scopep == m_scopep) {
            తిరిగి VSelf_సూచిక_పాఠ్యం{VSelf_సూచిక_పాఠ్యం::This{}};
        } లేకపోతే ఒకవేళ (relativeRefOk && !m_modSingleton && scopep->aboveScopep() == m_scopep
                   && VN_IS(scopep->modp(), Module)) {
            // Reference to scope of instance directly under this module, can just "this->cell",
            // which can potentially be V3Combined, but note this requires one extra pointer
            // dereference which is slower, so we only use it if the source scope is not a
            // singleton.
            తిరిగి scopeThis(scopep);
        } లేకపోతే {
            // Reference to something elsewhere, or relative references are disabled. Use global
            // variable
            తిరిగి scopeVlSyms(scopep);
        }
    }

    శూన్య make_ప్రజా_Func_Wrappers() {
        // We recorded all public functions in m_modFuncs.
        // If for any given name only one function exists, we can use that function directly.
        // If multiple functions exist, we need to select the appropriate scope.
        కొరకు (FuncMmap::iterator it = m_modFuncs.begin(); it != m_modFuncs.చివర(); ++it) {
            స్థిరరాశి string name = it->మొదటి;
            AstCFunc* స్థిరరాశి శిఖరం_Funcp = it->రెండవ;
            స్వయంచాలక తదుపరి_It1 = it;
            ++తదుపరి_It1;
            స్థిరరాశి తార్కిక మరిన్ని_యొక్క_Same1 = (తదుపరి_It1 != m_modFuncs.చివర() && తదుపరి_It1->మొదటి == name);
            ఒకవేళ (మరిన్ని_యొక్క_Same1) {
                // Multiple functions under this name, need a wrapper function
                UINFO(6, "  Wrapping " << name << " multifuncs");
                AstCFunc* స్థిరరాశి newfuncp = శిఖరం_Funcp->clone_వృక్షం(అబద్ధం);
                ఒకవేళ (newfuncp->varsp()) newfuncp->varsp()->unlink_Fr_వెనుక_With_తదుపరి()->delete_వృక్షం();
                ఒకవేళ (newfuncp->stmtsp()) newfuncp->stmtsp()->unlink_Fr_వెనుక_With_తదుపరి()->delete_వృక్షం();
                newfuncp->name(name);
                newfuncp->isStatic(అబద్ధం);
                శిఖరం_Funcp->కూడు_తదుపరి_ఇక్కడ(newfuncp);
                // In the body, call each function if it matches the given scope
                కొరకు (FuncMmap::iterator eachIt = it;
                     eachIt != m_modFuncs.చివర() && eachIt->మొదటి == name; ++eachIt) {
                    it = eachIt;
                    AstCFunc* స్థిరరాశి funcp = eachIt->రెండవ;
                    స్వయంచాలక తదుపరి_It2 = eachIt;
                    ++తదుపరి_It2;
                    స్థిరరాశి తార్కిక మరిన్ని_యొక్క_Same
                        = (తదుపరి_It2 != m_modFuncs.చివర() && తదుపరి_It2->మొదటి == name);
                    UASSERT_OBJ(funcp->scopep(), funcp, "Not scoped");

                    UINFO(6, "     Wrapping " << name << " " << funcp);
                    UINFO(6, "  at " << newfuncp->argTypes() << " und " << funcp->argTypes());
                    funcp->decl_వ్యక్తిగత(నిజం);
                    AstVarRef* argsp = శూన్యసూచిక;
                    కొరకు (Ast_కణుపు* stmtp = newfuncp->argsp(); stmtp; stmtp = stmtp->nextp()) {
                        ఒకవేళ (AstVar* స్థిరరాశి portp = VN_CAST(stmtp, Var)) {
                            ఒకవేళ (portp->isIO() && !portp->isFuncReturn()) {
                                AstVarRef* స్థిరరాశి newp = కొత్త AstVarRef{
                                    portp->fileline(), portp, portp->direction().pinAccess()};
                                argsp = Ast_కణుపు::కూడు_తదుపరి(argsp, newp);
                            }
                        }
                    }

                    AstCCall* స్థిరరాశి callp = కొత్త AstCCall{funcp->fileline(), funcp, argsp};
                    ఒకవేళ (Ast_కణుపు_DType* స్థిరరాశి dtypep = funcp->dtypep()) {
                        callp->dtypep(dtypep);
                    } లేకపోతే {
                        callp->dtype_సమితి_Void();
                    }
                    Ast_కణుపు* స్థిరరాశి returnp = కొత్త AstCReturn{funcp->fileline(), callp};

                    ఒకవేళ (మరిన్ని_యొక్క_Same) {
                        AstIf* స్థిరరాశి ifp = కొత్త AstIf{
                            funcp->fileline(),
                            కొత్త AstEq{funcp->fileline(),
                                      కొత్త AstCExpr{funcp->fileline(), "this", 64},
                                      కొత్త AstCExpr{funcp->fileline(),
                                                   scopeVlSyms(funcp->scopep()).asString(), 64}},
                            returnp};
                        newfuncp->కూడు_Stmtsp(ifp);
                    } లేకపోతే {
                        newfuncp->కూడు_Stmtsp(returnp);
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
            } లేకపోతే {
                // Only a single function under this name, we can rename it
                UINFO(6, "  Wrapping " << name << " just one " << శిఖరం_Funcp);
                శిఖరం_Funcp->name(name);
            }
        }
    }

    // VISITORS
    శూన్య సందర్శించు(Ast_కణుపు_Module* nodep) override {
        VL_RESTORER(m_modp);
        m_modp = nodep;
        m_modFuncs.clear();
        m_modSingleton = modIsSingleton(m_modp);
        iterateChildren(nodep);
        make_ప్రజా_Func_Wrappers();
    }
    శూన్య సందర్శించు(AstScope* nodep) override {
        VL_RESTORER(m_scopep);
        m_scopep = nodep;
        iterateChildren(nodep);
    }
    శూన్య సందర్శించు(AstVarScope* nodep) override {
        // Delete the varscope when we're finished
        VL_DO_DANGLING(నెట్టు_Deletep(nodep->unlink_Fr_వెనుక()), nodep);
    }
    శూన్య సందర్శించు(Ast_కణుపు_Var_Ref* nodep) override {
        iterateChildren(nodep);
        ఒకవేళ (!nodep->varScopep()) {
            UASSERT_OBJ(nodep->varp()->isFuncLocal(), nodep,
                        "unscoped reference can only appear to function locals at this point");
            తిరిగి;
        }
        // Convert the hierch name
        UINFO(9, "  ref-in " << nodep);
        UASSERT_OBJ(m_scopep, nodep, "Node not under scope");
        స్థిరరాశి AstVar* స్థిరరాశి varp = nodep->varScopep()->varp();
        స్థిరరాశి AstScope* స్థిరరాశి scopep = nodep->varScopep()->scopep();
        ఒకవేళ (varp->isFuncLocal()) {
            // Reference to function locals need no self pointer
            nodep->self_సూచిక(VSelf_సూచిక_పాఠ్యం{VSelf_సూచిక_పాఠ్యం::ఖాళీలు_2()});
        } లేకపోతే ఒకవేళ (scopep->modp()->isConstPool()) {
            // Reference to constant pool value need no self pointer
            nodep->self_సూచిక(VSelf_సూచిక_పాఠ్యం{VSelf_సూచిక_పాఠ్యం::ఖాళీలు_2()});
        } లేకపోతే {
            nodep->self_సూచిక(descoped_Self_సూచిక(scopep));
        }
        nodep->varScopep(శూన్యసూచిక);
        UINFO(9, "  refout " << nodep << " selfPtr=" << nodep->self_సూచిక().asString());
    }
    శూన్య సందర్శించు(AstCCall* nodep) override {
        // UINFO(9, "       " << nodep);
        iterateChildren(nodep);
        // Convert the hierch name
        UASSERT_OBJ(m_scopep, nodep, "Node not under scope");
        స్థిరరాశి AstScope* స్థిరరాశి scopep = nodep->funcp()->scopep();
        nodep->self_సూచిక(descoped_Self_సూచిక(scopep));
        // Can't do this, as we may have more calls later
        // nodep->funcp()->scopep(nullptr);
    }
    శూన్య సందర్శించు(AstScopeName* nodep) override {
        // Print a %m under this instance as its vlNamep, which holds its pretty name, plus the
        // rest of the path, so the instances' code can be V3Combined. Not with --protect-ids, as
        // vlNamep then holds the protected name, while %m prints the original.
        ఒకవేళ (!m_funcp || !nodep->forFormat() || m_modSingleton || m_funcp->isStatic()
            || v3Global.opt.protectIds()) {
            తిరిగి;
        }
        std::string మార్గం = nodep->scopePrettySymName();
        స్థిరరాశి std::string inst = m_scopep->prettyName();
        ఒకవేళ (మార్గం != inst && !VString::startsWith(మార్గం, inst + ".")) తిరిగి;
        మార్గం.erase(0, inst.కొలత() + 1);
        // The DPI scope table keeps the absolute name, via a copy outside any function
        m_scopep->కూడు_Blocksp(nodep->clone_వృక్షం(అబద్ధం));
        nodep->scopeAttr(మార్గం);
        nodep->scopeEntr("");  // Only used by the DPI scope table
        nodep->selfRelative(నిజం);
    }
    శూన్య సందర్శించు(AstCMethodCall* nodep) override { iterateChildren(nodep); }
    శూన్య సందర్శించు(AstCNew* nodep) override { iterateChildren(nodep); }
    శూన్య సందర్శించు(AstCFunc* nodep) override {
        VL_RESTORER(m_funcp);
        ఒకవేళ (!nodep->user1()) {
            // Static functions should have been moved under the corresponding AstClassPackage
            UASSERT_OBJ(!(nodep->isStatic() && VN_IS(m_modp, Class)), nodep,
                        "Static function under AstClass");
            m_funcp = nodep;
            iterateChildren(nodep);
            nodep->user1(నిజం);
            // If it's under a scope, move it up to the top
            ఒకవేళ (m_scopep) {
                nodep->unlink_Fr_వెనుక();
                m_modp->కూడు_Stmtsp(nodep);

                ఒకవేళ (nodep->func_ప్రజా()) {
                    // There may be multiple public functions by the same name;
                    // record for later correction or making of shells
                    m_modFuncs.emplace(nodep->name(), nodep);
                    nodep->name(m_scopep->nameDotless() + "__" + nodep->name());
                }
            }
        }
    }
    శూన్య సందర్శించు(AstVar*) override {}
    శూన్య సందర్శించు(Ast_కణుపు* nodep) override { iterateChildren(nodep); }

బహిరంగ:
    // CONSTRUCTORS
    స్పష్టమైన DescopeVisitor(AstNetlist* nodep) { iterate(nodep); }
    ~DescopeVisitor() override = అప్రమేయ;
};

//######################################################################
// Descope class functions

శూన్య V3Descope::descopeAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { DescopeVisitor{nodep}; }  // Destruct before checking
    V3ConstPool::సమితి_Descoped();
    V3Global::dump_Check_Global_వృక్షం("descope", 0, dump_వృక్షం_Either_Level() >= 3);
}
