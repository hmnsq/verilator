// -*- mode: C++; c-file-style: "cc-mode" -*-
//*************************************************************************
// DESCRIPTION: Verilator: Emit C++ for tree
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

#ಅನ್ತರ್ಭಾವ "V3PchAstMT.h"

#ಅನ್ತರ್ಭಾವ "V3EmitCBase.h"

#ಅನ್ತರ್ಭಾವ "V3Task.h"

//######################################################################
// EmitCParentModule implementation

EmitCParentModule::EmitCParentModule() {
    ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ ಗಣ_All = [](Ast_ಗಂಟು_Module* modp) -> ಶೂನ್ಯ {
        ಗಾಗಿ (Ast_ಗಂಟು* nodep = modp->stmtsp(); nodep; nodep = nodep->nextp()) {
            ಆದರೆ (VN_IS(nodep, CFunc) || VN_IS(nodep, Var) || VN_IS(nodep, ಗಂಟು_Cover_Decl)) {
                nodep->user4p(modp);
            }
        }
        modp->foreach([&](Ast_ಗಂಟು_Cover_Decl* nodep) { nodep->user4p(modp); });
    };
    ಗಾಗಿ (Ast_ಗಂಟು* modp = v3Global.rootp()->modulesp(); modp; modp = modp->nextp()) {
        ಗಣ_All(VN_AS(modp, ಗಂಟು_Module));
    }
}

//######################################################################
// EmitCUtil implementation

string EmitCUtil::ಪೂರ್ವಪ್ರತ್ಯಯ_Name_Protect(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* nodep) VL_MT_STABLE {
    ಸ್ಥಿರಾಂಕ string ಪೂರ್ವಪ್ರತ್ಯಯ = v3Global.opt.mod_ಪೂರ್ವಪ್ರತ್ಯಯ() + "_" + VIdProtect::protect(nodep->name());
    // If all-uppercase prefix conflicts with a previous usage of the
    // prefix with different capitalization, rename to avoid conflict.
    // This is to support OSes where filename compares are non-case significant.
    // STATIC:
    ಸ್ಥಿರ V3Mutex s_mutex;
    ಸ್ಥಿರಾಂಕ V3_ಬೀಗ_Guard ಬೀಗ_2{s_mutex};  // Otherwise map access is unsafe
    ಸ್ಥಿರ std::map<std::string, std::string> s_memoized;
    ಸ್ಥಿರ std::map<std::string, std::string> s_uc_ಗಮ್ಯ_ಪೂರ್ವಪ್ರತ್ಯಯ;
    // Memoize results
    ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ mit = s_memoized.ಕಂಡುಹಿಡಿ(ಪೂರ್ವಪ್ರತ್ಯಯ);
    ಆದರೆ (mit != s_memoized.ಕೊನೆ()) ಹಿಂತಿರುಗಿ mit->ಎರಡನೇ;
    //
    // Check capitalization
    ಸ್ಥಿರಾಂಕ string ಪೂರ್ವಪ್ರತ್ಯಯ_ಊರ್ಧ್ವ = VString::upcase(ಪೂರ್ವಪ್ರತ್ಯಯ);
    string ಫಲಿತಾಂಶ;
    {
        ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ it = s_uc_ಗಮ್ಯ_ಪೂರ್ವಪ್ರತ್ಯಯ.ಕಂಡುಹಿಡಿ(ಪೂರ್ವಪ್ರತ್ಯಯ_ಊರ್ಧ್ವ);
        ಆದರೆ (it == s_uc_ಗಮ್ಯ_ಪೂರ್ವಪ್ರತ್ಯಯ.ಕೊನೆ()) {
            s_uc_ಗಮ್ಯ_ಪೂರ್ವಪ್ರತ್ಯಯ.emplace(ಪೂರ್ವಪ್ರತ್ಯಯ_ಊರ್ಧ್ವ, ಪೂರ್ವಪ್ರತ್ಯಯ);
            ಫಲಿತಾಂಶ = ಪೂರ್ವಪ್ರತ್ಯಯ;
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (it->ಎರಡನೇ == ಪೂರ್ವಪ್ರತ್ಯಯ) {
            ಫಲಿತಾಂಶ = ಪೂರ್ವಪ್ರತ್ಯಯ;  // Same capitialization as last time
        } ಇಲ್ಲದಿದ್ದರೆ {
            VHashSha512 hash{ಪೂರ್ವಪ್ರತ್ಯಯ};
            ಫಲಿತಾಂಶ = ಪೂರ್ವಪ್ರತ್ಯಯ + "__Vphsh" + hash.digestSymbol24();
        }
    }
    s_memoized.emplace(ಪೂರ್ವಪ್ರತ್ಯಯ, ಫಲಿತಾಂಶ);
    ಹಿಂತಿರುಗಿ ಫಲಿತಾಂಶ;
}

//######################################################################
// EmitCBaseVisitor implementation

string EmitCBaseVisitorConst::funcNameProtect(ಸ್ಥಿರಾಂಕ AstCFunc* nodep, ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_Module* modp) {
    modp = modp ? modp : EmitCParentModule::get(nodep);
    string name;
    ಆದರೆ (nodep->isConstructor()) {
        name += "init";
    } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (nodep->isDestructor()) {
        name += "~";
        name += EmitCUtil::ಪೂರ್ವಪ್ರತ್ಯಯ_Name_Protect(modp);
    } ಇಲ್ಲದಿದ್ದರೆ {
        ಆದರೆ (nodep->isLoose()) {
            name += EmitCUtil::ಪೂರ್ವಪ್ರತ್ಯಯ_Name_Protect(modp);
            name += "__";
        }
        name += nodep->nameProtect();
    }
    ಹಿಂತಿರುಗಿ name;
}

string EmitCBaseVisitorConst::c_Func_ವಾದಗಳು(ಸ್ಥಿರಾಂಕ AstCFunc* nodep, ತಾರ್ಕಿಕ cpp) {
    // Return argument list for given C function
    string ವಾದಗಳು;
    ಆದರೆ (nodep->isLoose() && !nodep->isStatic()) {
        ಆದರೆ (nodep->voidSelf()) {
            ವಾದಗಳು += "void* vlVoidSelf";  // Converted to 'vlSelf' in the function body
        } ಇಲ್ಲದಿದ್ದರೆ {
            ಆದರೆ (nodep->isConst().true_ತಿಳಿದ()) ವಾದಗಳು += "const ";
            ವಾದಗಳು += EmitCUtil::ಪೂರ್ವಪ್ರತ್ಯಯ_Name_Protect(EmitCParentModule::get(nodep));
            ವಾದಗಳು += "* vlSelf";
        }
    }
    ಆದರೆ (nodep->needProcess()) {
        ಆದರೆ (!ವಾದಗಳು.ಖಾಲಿಗಳು()) ವಾದಗಳು += ", ";
        ವಾದಗಳು += "VlProcessRef vlProcess";
    }
    ಆದರೆ (!nodep->argTypes().ಖಾಲಿಗಳು()) {
        ಆದರೆ (!ವಾದಗಳು.ಖಾಲಿಗಳು()) ವಾದಗಳು += ", ";
        ವಾದಗಳು += nodep->argTypes();
    }
    // Might be a user function with argument list.
    ಗಾಗಿ (ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* stmtp = nodep->argsp(); stmtp; stmtp = stmtp->nextp()) {
        ಆದರೆ (ಸ್ಥಿರಾಂಕ AstVar* ಸ್ಥಿರಾಂಕ portp = VN_CAST(stmtp, Var)) {
            ಆದರೆ (portp->isIO() && !portp->isFuncReturn()) {
                ಆದರೆ (!ವಾದಗಳು.ಖಾಲಿಗಳು()) ವಾದಗಳು += ", ";
                ಆದರೆ (nodep->dpiImportPrototype() || nodep->dpiExportDispatcher()) {
                    ವಾದಗಳು += portp->dpiArgType(ನಿಜ, ಸುಳ್ಳು, cpp);
                } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (nodep->func_ಬಹಿರಂಗ()) {
                    ವಾದಗಳು += portp->cPubArgType(ನಿಜ, ಸುಳ್ಳು);
                } ಇಲ್ಲದಿದ್ದರೆ {
                    ವಾದಗಳು += portp->vlArgType(ನಿಜ, ಸುಳ್ಳು, ನಿಜ);
                }
            }
        }
    }
    ಹಿಂತಿರುಗಿ ವಾದಗಳು;
}

ಶೂನ್ಯ EmitCBaseVisitorConst::emitCDefaultConstructor(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_Module* ಸ್ಥಿರಾಂಕ modp) {
    puts(EmitCUtil::ಪೂರ್ವಪ್ರತ್ಯಯ_Name_Protect(modp));
    puts("() = default;\n");
}

ಶೂನ್ಯ EmitCBaseVisitorConst::emit_CFunc_ಶೀರ್ಷಿಕೆ(ಸ್ಥಿರಾಂಕ AstCFunc* funcp, ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_Module* modp,
                                            ತಾರ್ಕಿಕ withScope) {
    ಆದರೆ (funcp->slow()) putns(funcp, "VL_ATTR_COLD ");
    ಆದರೆ (funcp->dpiCDeclOverride()) {
        putns(funcp, funcp->dpiCDecl() + ";\n");
        ಹಿಂತಿರುಗಿ;
    }
    ಆದರೆ (!funcp->isDestructor()) {
        putns(funcp, funcp->rtnTypeVoid());
        puts(" ");
    }
    ಆದರೆ (withScope) {
        ಆದರೆ (funcp->dpiExportDispatcher()) {
            putns(funcp, EmitCUtil::ತುದಿ_Class_Name() + "::");
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (funcp->is_Proper_ವಿಧಾನ()) {
            putns(funcp, EmitCUtil::ಪೂರ್ವಪ್ರತ್ಯಯ_Name_Protect(modp) + "::");
        }
    }
    putns(funcp, funcNameProtect(funcp, modp));
    puts("(" + c_Func_ವಾದಗಳು(funcp) + ")");
    ಆದರೆ (funcp->isConst().true_ತಿಳಿದ() && funcp->is_Proper_ವಿಧಾನ()) puts(" const");
}

ಶೂನ್ಯ EmitCBaseVisitorConst::emitCFuncDecl(ಸ್ಥಿರಾಂಕ AstCFunc* funcp, ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_Module* modp,
                                          ತಾರ್ಕಿಕ cLinkage) {
    ensure_New_ಸಾಲು();
    ಆದರೆ (!funcp->ifdef().ಖಾಲಿಗಳು()) putns(funcp, "#ifdef " + funcp->ifdef() + "\n");
    ಆದರೆ (funcp->isConstructor()) emitCDefaultConstructor(modp);
    ಆದರೆ (cLinkage) putns(funcp, "extern \"C\" ");
    ಆದರೆ (funcp->isStatic() && funcp->is_Proper_ವಿಧಾನ()) putns(funcp, "static ");
    ಆದರೆ (funcp->isVirtual()) {
        UASSERT_OBJ(funcp->is_Proper_ವಿಧಾನ(), funcp, "Virtual function is not a proper method");
        putns(funcp, "virtual ");
        // Intentionally no emit for "override" instead, as clang will then enable warning
        // on other methods where virtual vs override is needed, and this is not tracked yet
    }
    emit_CFunc_ಶೀರ್ಷಿಕೆ(funcp, modp, /* withScope: */ ಸುಳ್ಳು);
    ಸ್ಥಿರಾಂಕ AstClass* ಸ್ಥಿರಾಂಕ classp = VN_CAST(modp, Class);
    ಆದರೆ (funcp->ಖಾಲಿಗಳು_Body() && !funcp->isLoose() && !cLinkage
        && !(funcp->isConstructor() && classp && classp->isInterfaceClass())) {
        putns(funcp, " {}\n");
    } ಇಲ್ಲದಿದ್ದರೆ {
        putns(funcp, ";\n");
    }
    ಆದರೆ (!funcp->ifdef().ಖಾಲಿಗಳು()) putns(funcp, "#endif  // " + funcp->ifdef() + "\n");
}

std::string EmitCBaseVisitorConst::emit_ವಿಭಜಿಸು_ಪಠ್ಯ_Stmts(ಪಠ್ಯ_Stmts& stmts, ಸ್ಥಿರಾಂಕ std::string& ಪೂರ್ವಪ್ರತ್ಯಯ,
                                                      ಸ್ಥಿರಾಂಕ std::string& params,
                                                      ಸ್ಥಿರಾಂಕ std::string& ವಾದಗಳು,  //
                                                      ಗಾತ್ರ_t max_ವೆಚ್ಚ, ತಾರ್ಕಿಕ support,
                                                      ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* descriptionp,
                                                      ಸ್ಥಿರಾಂಕ std::function<ಶೂನ್ಯ()>& emit_ಶೀರ್ಷಿಕೆ) {
    ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ subFuncName = [&](ಗಾತ್ರ_t n) { ಹಿಂತಿರುಗಿ ಪೂರ್ವಪ್ರತ್ಯಯ + "__" + std::ಗಮ್ಯ_string(n); };
    ಗಾತ್ರ_t nSubFunctions = 0;
    std::string funcName;  // Name of last sub-function, which is the root at the end
    // After the first level, statement 'j' is a call to sub-function 'calleeBase + j'
    ತಾರ್ಕಿಕ callsSubFuncs = ಸುಳ್ಳು;
    ಗಾತ್ರ_t callee_ತಳ = 0;
    // Reduce into a balanced tree of sub-function calls until we end up with a single statement
    ಆಗ (stmts.ಗಾತ್ರ_2() > 1) {
        ಸ್ಥಿರಾಂಕ ಗಾತ್ರ_t level_ತಳ = nSubFunctions;
        ಪಠ್ಯ_Stmts calls;  // Calls to the sub-functions of this level
        ಸ್ಥಿರಾಂಕ ಗಾತ್ರ_t nStmts = stmts.ಗಾತ್ರ_2();
        ಗಾಗಿ (ಗಾತ್ರ_t ವಿಭಜಿಸು_ಆರಂಭ = 0, ವಿಭಜಿಸು_ಕೊನೆ = 0; ವಿಭಜಿಸು_ಆರಂಭ < nStmts; ವಿಭಜಿಸು_ಆರಂಭ = ವಿಭಜಿಸು_ಕೊನೆ) {
            // Gather up at at most 'maxCost' worth of statements in this split,
            // but always at least 2 (if less than 2, the reduction makes no
            // progress and the loop will not terminate).
            ಗಾತ್ರ_t ವೆಚ್ಚ = 0;
            ಆಗ (((ವೆಚ್ಚ < max_ವೆಚ್ಚ) || (ವಿಭಜಿಸು_ಕೊನೆ - ವಿಭಜಿಸು_ಆರಂಭ < 2)) && ವಿಭಜಿಸು_ಕೊನೆ < nStmts) {
                ವೆಚ್ಚ += stmts[ವಿಭಜಿಸು_ಕೊನೆ++].m_ವೆಚ್ಚ;
            }
            UASSERT(ವಿಭಜಿಸು_ಆರಂಭ < ವಿಭಜಿಸು_ಕೊನೆ, "Empty split");

            // Name of sub-function we are emitting now
            funcName = subFuncName(nSubFunctions++);

            // Open split file
            open_New_Output_ಮೂಲ_File(funcName, ನಿಜ, support, descriptionp);
            // Emit header
            emit_ಶೀರ್ಷಿಕೆ();
            // Declare called sub-functions
            ಆದರೆ (callsSubFuncs) {
                puts("\n");
                ಗಾಗಿ (ಗಾತ್ರ_t j = ವಿಭಜಿಸು_ಆರಂಭ; j < ವಿಭಜಿಸು_ಕೊನೆ; ++j) {
                    puts("void " + subFuncName(callee_ತಳ + j) + "(" + params + ");\n");
                }
                puts("\n");
            }
            // Open sub-function definition in the split file
            puts("void " + funcName + "(" + params + ") {\n");
            // Emit statements
            ಗಾಗಿ (ಗಾತ್ರ_t j = ವಿಭಜಿಸು_ಆರಂಭ; j < ವಿಭಜಿಸು_ಕೊನೆ; ++j) {
                ofp()->putsNoTracking("    ");
                ofp()->putsNoTracking(stmts[j].m_ಪಠ್ಯ);
                ofp()->putsNoTracking("\n");
            }
            // Close sub-function
            puts("}\n");
            // Close split file
            closeOutputFile();

            // Replace statements with a call to the sub-function
            calls.ಕೂಡಿಸು(funcName + "(" + ವಾದಗಳು + ");");
        }
        stmts = std::move(calls);
        callsSubFuncs = ನಿಜ;
        callee_ತಳ = level_ತಳ;
    }
    ಹಿಂತಿರುಗಿ funcName.ಖಾಲಿಗಳು() ? "" : "void " + funcName + "(" + params + ");\n";
}

std::pair<std::string, std::string> EmitCBaseVisitorConst::emit_ವಿಭಜಿಸು_Ctor_Dtor(
    ಪಠ್ಯ_Stmts& ctorStmts, ಪಠ್ಯ_Stmts& dtorStmts, ಗಾತ್ರ_t ತಳ_ವೆಚ್ಚ, ಸ್ಥಿರಾಂಕ std::string& ಪೂರ್ವಪ್ರತ್ಯಯ,
    ಸ್ಥಿರಾಂಕ std::string& params, ಸ್ಥಿರಾಂಕ std::string& ವಾದಗಳು, ತಾರ್ಕಿಕ support, ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* descriptionp,
    ಸ್ಥಿರಾಂಕ std::function<ಶೂನ್ಯ()>& emit_ಶೀರ್ಷಿಕೆ) {
    ಸ್ಥಿರಾಂಕ ಗಾತ್ರ_t max_ವೆಚ್ಚ = static_cast<ಗಾತ್ರ_t>(v3Global.opt.output_ವಿಭಜಿಸು_CFuncs());
    ಆದರೆ (!max_ವೆಚ್ಚ) ಹಿಂತಿರುಗಿ {};
    ಗಾತ್ರ_t ಒಟ್ಟು_ವೆಚ್ಚ = ತಳ_ವೆಚ್ಚ;
    ಗಾಗಿ (ಸ್ಥಿರಾಂಕ ಪಠ್ಯ_Stmts::Stmt& stmt : ctorStmts) ಒಟ್ಟು_ವೆಚ್ಚ += stmt.m_ವೆಚ್ಚ;
    ಗಾಗಿ (ಸ್ಥಿರಾಂಕ ಪಠ್ಯ_Stmts::Stmt& stmt : dtorStmts) ಒಟ್ಟು_ವೆಚ್ಚ += stmt.m_ವೆಚ್ಚ;
    ಆದರೆ (ಒಟ್ಟು_ವೆಚ್ಚ <= max_ವೆಚ್ಚ) ಹಿಂತಿರುಗಿ {};
    v3Global.useParallelBuild(ನಿಜ);  // Splitting files, so using parallel build.
    std::string ctorDecl = emit_ವಿಭಜಿಸು_ಪಠ್ಯ_Stmts(ctorStmts, ಪೂರ್ವಪ್ರತ್ಯಯ + "__ctor", params, ವಾದಗಳು, max_ವೆಚ್ಚ,
                                              support, descriptionp, emit_ಶೀರ್ಷಿಕೆ);
    std::string dtorDecl = emit_ವಿಭಜಿಸು_ಪಠ್ಯ_Stmts(dtorStmts, ಪೂರ್ವಪ್ರತ್ಯಯ + "__dtor", params, ವಾದಗಳು, max_ವೆಚ್ಚ,
                                              support, descriptionp, emit_ಶೀರ್ಷಿಕೆ);
    ಹಿಂತಿರುಗಿ {std::move(ctorDecl), std::move(dtorDecl)};
}

ಶೂನ್ಯ EmitCBaseVisitorConst::emitVarDecl(ಸ್ಥಿರಾಂಕ AstVar* nodep, ತಾರ್ಕಿಕ asRef) {
    ಸ್ಥಿರಾಂಕ AstBasicDType* ಸ್ಥಿರಾಂಕ basicp = nodep->basicp();
    ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ refNeedParens = VN_IS(nodep->dtypeSkipRefp(), UnpackArrayDType);
    ಆದರೆ (nodep->mtask_Cache_ಸಾಲು_Align() && !asRef) putns(nodep, "alignas(VL_CACHE_LINE_BYTES) ");
    ಆದರೆ (nodep->covergroupRefMember()) {
        UASSERT_OBJ(!asRef, nodep, "Covergroup reference member emitted as C++ reference");
        putns(nodep, nodep->dtypep()->cType("", ಸುಳ್ಳು, ಸುಳ್ಳು));
        puts(" const* " + nodep->nameProtect() + " = nullptr;\n");
        ಹಿಂತಿರುಗಿ;
    }

    ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ emitDeclArrayBrackets = [ಇದು](ಸ್ಥಿರಾಂಕ AstVar* nodep) -> ಶೂನ್ಯ {
        // This isn't very robust and may need cleanup for other data types
        ಗಾಗಿ (ಸ್ಥಿರಾಂಕ AstUnpackArrayDType* arrayp = VN_CAST(nodep->dtypeSkipRefp(), UnpackArrayDType);
             arrayp; arrayp = VN_CAST(arrayp->subDTypep()->skipRefp(), UnpackArrayDType)) {
            putns(arrayp, "[" + cvt_ಗಮ್ಯ_Str(arrayp->elementsConst()) + "]");
        }
    };

    ಆದರೆ (nodep->isPrimaryIO() && nodep->isSc()) {
        UASSERT_OBJ(basicp, nodep, "Unimplemented: Outputting this data type");
        ಆದರೆ (nodep->isInout()) {
            putns(nodep, "sc_core::sc_inout<");
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (nodep->isWritable()) {
            putns(nodep, "sc_core::sc_out<");
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (nodep->isNonOutput()) {
            putns(nodep, "sc_core::sc_in<");
        } ಇಲ್ಲದಿದ್ದರೆ {
            nodep->v3fatalSrc("Unknown type");
        }
        puts(nodep->scType());
        puts("> ");
        ಆದರೆ (asRef) {
            ಆದರೆ (refNeedParens) putns(nodep, "(");
            putns(nodep, "&");
        }
        putns(nodep, nodep->nameProtect());
        ಆದರೆ (asRef && refNeedParens) puts(")");
        emitDeclArrayBrackets(nodep);
        puts(";\n");
    } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (nodep->isVLIO()) {
        ಆದರೆ (nodep->isInout()) {
            putns(nodep, "VL_INOUT");
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (nodep->isWritable()) {
            putns(nodep, "VL_OUT");
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (nodep->isNonOutput()) {
            putns(nodep, "VL_IN");
        } ಇಲ್ಲದಿದ್ದರೆ {
            nodep->v3fatalSrc("Unknown type");
        }

        ಆದರೆ (nodep->isQuad()) {
            puts("64");
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (nodep->ಅಗಲ_Min() <= 8) {
            puts("8");
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (nodep->ಅಗಲ_Min() <= 16) {
            puts("16");
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (nodep->isWide()) {
            puts("W");
        }

        puts("(");
        ಆದರೆ (asRef) {
            ಆದರೆ (refNeedParens) puts("(");
            puts("&");
        }
        puts(nodep->nameProtect());
        ಆದರೆ (asRef && refNeedParens) puts(")");
        emitDeclArrayBrackets(nodep);
        // If it's a packed struct/array then nodep->width is the whole
        // thing, msb/lsb is just lowest dimension
        puts("," + cvt_ಗಮ್ಯ_Str(basicp->lo() + nodep->ಅಗಲ() - 1) + "," + cvt_ಗಮ್ಯ_Str(basicp->lo()));
        ಆದರೆ (nodep->isWide()) puts("," + cvt_ಗಮ್ಯ_Str(nodep->ಅಗಲ_ಪದಗಳು()));
        puts(");\n");
    } ಇಲ್ಲದಿದ್ದರೆ {
        // Strings and other fundamental C types
        ಆದರೆ (nodep->isFuncLocal() && nodep->dtypep()->ಒಳಗೊಂಡಿದೆ_String()) {
            ಸ್ಥಿರಾಂಕ string name = nodep->name();
            ಸ್ಥಿರಾಂಕ string suffix = V3Task::dpiTemporaryVarSuffix();
            // Temporary variable for DPI-C containing strings needs to be static because c_str()
            // will be passed to C code and the lifetime of the variable must be long enough. See
            // also Issue 2622.
            ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ beStatic = name.ಗಾತ್ರ_2() >= suffix.ಗಾತ್ರ_2()
                                  && name.substr(name.ಗಾತ್ರ_2() - suffix.ಗಾತ್ರ_2()) == suffix;
            ಆದರೆ (beStatic) puts("static thread_local ");
        }
        putns(nodep, nodep->vlArgType(ನಿಜ, ಸುಳ್ಳು, ಸುಳ್ಳು, asRef));
        puts(";\n");
    }
}

ಶೂನ್ಯ EmitCBaseVisitorConst::emitVarAccessors(ಸ್ಥಿರಾಂಕ AstVar* nodep) {
    ಪ್ರತಿಜ್ಞಾ(nodep->name().rfind("__Vm_sig_") == 0 && nodep->isIO());
    ಸ್ಥಿರಾಂಕ string ಖಾಸಗಿ_Name = nodep->name();
    ಸ್ಥಿರಾಂಕ string ಬಹಿರಂಗ_Name = nodep->name().substr(ಅಕ್ಷರಸರಣಿ_ಉದ್ದ("__Vm_sig_"));

    puts("decltype("s + ಖಾಸಗಿ_Name + ") "s + ಬಹಿರಂಗ_Name + "() {return "s + ಖಾಸಗಿ_Name + ";}\n");
    puts("void "s + ಬಹಿರಂಗ_Name + "(decltype(" + ಖಾಸಗಿ_Name + ") v) {"s + ಖಾಸಗಿ_Name + "=v;}\n");
}

ಶೂನ್ಯ EmitCBaseVisitorConst::emitModCUse(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_Module* modp, VUseType useType) {
    ತಾರ್ಕಿಕ nl = ಸುಳ್ಳು;
    forModCUse(modp, useType, [&](ಸ್ಥಿರಾಂಕ string& ನಮೂದು) {
        puts(ನಮೂದು);
        nl = ನಿಜ;
    });
    ಆದರೆ (nl) puts("\n");
}

std::pair<string, File_ಸಾಲು*> EmitCBaseVisitorConst::scSection(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_Module* modp,
                                                              VSystemCSectionType type) {
    string ಪಠ್ಯ;
    File_ಸಾಲು* fl = ಶೂನ್ಯಸೂಚಕ;
    ಪೂರ್ಣಾಂಕ ಕೊನೆಯ_ಸಾಲು = -999;
    ಗಾಗಿ (ಸ್ಥಿರಾಂಕ AstSystemCSection* scSectionp = modp->scSectionsp(); scSectionp;
         scSectionp = VN_AS(scSectionp->nextp(), SystemCSection)) {
        ಆದರೆ (scSectionp->sectionType() != type) ಮುಂದುವರಿ;
        ಆದರೆ (ಪಠ್ಯ.ಖಾಲಿಗಳು()) {
            fl = scSectionp->fileline();
            ಪಠ್ಯ += "\n";
            ಆದರೆ (v3Global.opt.decoration()) {
                ಪಠ್ಯ += "\n//*** Below code from `systemc in Verilog file\n";
            }
        }
        ಆದರೆ (ಕೊನೆಯ_ಸಾಲು + 1 != scSectionp->fileline()->lineno() && v3Global.opt.decoration()) {
            ಪಠ್ಯ += "// From `systemc at " + scSectionp->fileline()->ascii() + "\n";
        }
        ಕೊನೆಯ_ಸಾಲು = scSectionp->fileline()->lineno();
        ಪಠ್ಯ += scSectionp->ಪಠ್ಯ();
    }
    ಆದರೆ (!ಪಠ್ಯ.ಖಾಲಿಗಳು()) {
        ಆದರೆ (v3Global.opt.decoration()) ಪಠ್ಯ += "//*** Above code from `systemc in Verilog file\n";
        ಪಠ್ಯ += "\n";
        // Substitute `systemc_class_name
        string::ಗಾತ್ರ_type pos;
        ಆಗ ((pos = ಪಠ್ಯ.ಕಂಡುಹಿಡಿ("`systemc_class_name")) != string::npos) {
            ಪಠ್ಯ.replace(pos, std::ಅಕ್ಷರಸರಣಿ_ಉದ್ದ("`systemc_class_name"),
                         EmitCUtil::ಪೂರ್ವಪ್ರತ್ಯಯ_Name_Protect(modp));
        }
    }
    ಹಿಂತಿರುಗಿ std::make_pair(ಪಠ್ಯ, fl);
}

ಶೂನ್ಯ EmitCBaseVisitorConst::emitSystemCSection(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_Module* modp,
                                               VSystemCSectionType type) {
    ಸ್ವಯಂಚಾಲಿತ ಪಠ್ಯ_And_Fileline = scSection(modp, type);
    ಆದರೆ (!ಪಠ್ಯ_And_Fileline.ಮೊದಲ.ಖಾಲಿಗಳು()) ofp()->putsNoTracking(ಪಠ್ಯ_And_Fileline.ಮೊದಲ);
}
