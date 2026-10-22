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

#ਅਨ੍ਤਰ੍ਭਾਵ "V3PchAstMT.h"

#ਅਨ੍ਤਰ੍ਭਾਵ "V3EmitCBase.h"

#ਅਨ੍ਤਰ੍ਭਾਵ "V3Task.h"

//######################################################################
// EmitCParentModule implementation

EmitCParentModule::EmitCParentModule() {
    ਅਚਲ ਸਵੈਚਾਲਿਤ ਸੈੱਟ_All = [](Ast_ਗੰਢ_Module* modp) -> ਖਾਲੀ {
        ਲਈ (Ast_ਗੰਢ* nodep = modp->stmtsp(); nodep; nodep = nodep->nextp()) {
            ਜੇ (VN_IS(nodep, CFunc) || VN_IS(nodep, Var) || VN_IS(nodep, ਗੰਢ_Cover_Decl)) {
                nodep->user4p(modp);
            }
        }
        modp->foreach([&](Ast_ਗੰਢ_Cover_Decl* nodep) { nodep->user4p(modp); });
    };
    ਲਈ (Ast_ਗੰਢ* modp = v3Global.rootp()->modulesp(); modp; modp = modp->nextp()) {
        ਸੈੱਟ_All(VN_AS(modp, ਗੰਢ_Module));
    }
}

//######################################################################
// EmitCUtil implementation

string EmitCUtil::ਅਗੇਤਰ_Name_Protect(ਅਚਲ Ast_ਗੰਢ* nodep) VL_MT_STABLE {
    ਅਚਲ string ਅਗੇਤਰ = v3Global.opt.mod_ਅਗੇਤਰ() + "_" + VIdProtect::protect(nodep->name());
    // If all-uppercase prefix conflicts with a previous usage of the
    // prefix with different capitalization, rename to avoid conflict.
    // This is to support OSes where filename compares are non-case significant.
    // STATIC:
    ਸਥਿਰ V3Mutex s_mutex;
    ਅਚਲ V3_ਤਾਲਾ_Guard ਤਾਲਾ_2{s_mutex};  // Otherwise map access is unsafe
    ਸਥਿਰ std::map<std::string, std::string> s_memoized;
    ਸਥਿਰ std::map<std::string, std::string> s_uc_ਮੰਜ਼ਿਲ_ਅਗੇਤਰ;
    // Memoize results
    ਅਚਲ ਸਵੈਚਾਲਿਤ mit = s_memoized.ਲੱਭੋ(ਅਗੇਤਰ);
    ਜੇ (mit != s_memoized.ਸਿਰਾ()) ਵਾਪਸ mit->ਦੂਜਾ;
    //
    // Check capitalization
    ਅਚਲ string ਅਗੇਤਰ_ਉੱਤਲਾ = VString::upcase(ਅਗੇਤਰ);
    string ਨਤੀਜਾ;
    {
        ਅਚਲ ਸਵੈਚਾਲਿਤ it = s_uc_ਮੰਜ਼ਿਲ_ਅਗੇਤਰ.ਲੱਭੋ(ਅਗੇਤਰ_ਉੱਤਲਾ);
        ਜੇ (it == s_uc_ਮੰਜ਼ਿਲ_ਅਗੇਤਰ.ਸਿਰਾ()) {
            s_uc_ਮੰਜ਼ਿਲ_ਅਗੇਤਰ.emplace(ਅਗੇਤਰ_ਉੱਤਲਾ, ਅਗੇਤਰ);
            ਨਤੀਜਾ = ਅਗੇਤਰ;
        } ਨਹੀਂਤਾਂ ਜੇ (it->ਦੂਜਾ == ਅਗੇਤਰ) {
            ਨਤੀਜਾ = ਅਗੇਤਰ;  // Same capitialization as last time
        } ਨਹੀਂਤਾਂ {
            VHashSha512 hash{ਅਗੇਤਰ};
            ਨਤੀਜਾ = ਅਗੇਤਰ + "__Vphsh" + hash.digestSymbol24();
        }
    }
    s_memoized.emplace(ਅਗੇਤਰ, ਨਤੀਜਾ);
    ਵਾਪਸ ਨਤੀਜਾ;
}

//######################################################################
// EmitCBaseVisitor implementation

string EmitCBaseVisitorConst::funcNameProtect(ਅਚਲ AstCFunc* nodep, ਅਚਲ Ast_ਗੰਢ_Module* modp) {
    modp = modp ? modp : EmitCParentModule::get(nodep);
    string name;
    ਜੇ (nodep->isConstructor()) {
        name += "init";
    } ਨਹੀਂਤਾਂ ਜੇ (nodep->isDestructor()) {
        name += "~";
        name += EmitCUtil::ਅਗੇਤਰ_Name_Protect(modp);
    } ਨਹੀਂਤਾਂ {
        ਜੇ (nodep->isLoose()) {
            name += EmitCUtil::ਅਗੇਤਰ_Name_Protect(modp);
            name += "__";
        }
        name += nodep->nameProtect();
    }
    ਵਾਪਸ name;
}

string EmitCBaseVisitorConst::c_Func_ਦਲੀਲਾਂ(ਅਚਲ AstCFunc* nodep, ਤਾਰਕਿਕ cpp) {
    // Return argument list for given C function
    string ਦਲੀਲਾਂ;
    ਜੇ (nodep->isLoose() && !nodep->isStatic()) {
        ਜੇ (nodep->voidSelf()) {
            ਦਲੀਲਾਂ += "void* vlVoidSelf";  // Converted to 'vlSelf' in the function body
        } ਨਹੀਂਤਾਂ {
            ਜੇ (nodep->isConst().true_ਗਿਆਤ()) ਦਲੀਲਾਂ += "const ";
            ਦਲੀਲਾਂ += EmitCUtil::ਅਗੇਤਰ_Name_Protect(EmitCParentModule::get(nodep));
            ਦਲੀਲਾਂ += "* vlSelf";
        }
    }
    ਜੇ (nodep->needProcess()) {
        ਜੇ (!ਦਲੀਲਾਂ.ਖਾਲੀ_ਥਾਵਾਂ()) ਦਲੀਲਾਂ += ", ";
        ਦਲੀਲਾਂ += "VlProcessRef vlProcess";
    }
    ਜੇ (!nodep->argTypes().ਖਾਲੀ_ਥਾਵਾਂ()) {
        ਜੇ (!ਦਲੀਲਾਂ.ਖਾਲੀ_ਥਾਵਾਂ()) ਦਲੀਲਾਂ += ", ";
        ਦਲੀਲਾਂ += nodep->argTypes();
    }
    // Might be a user function with argument list.
    ਲਈ (ਅਚਲ Ast_ਗੰਢ* stmtp = nodep->argsp(); stmtp; stmtp = stmtp->nextp()) {
        ਜੇ (ਅਚਲ AstVar* ਅਚਲ portp = VN_CAST(stmtp, Var)) {
            ਜੇ (portp->isIO() && !portp->isFuncReturn()) {
                ਜੇ (!ਦਲੀਲਾਂ.ਖਾਲੀ_ਥਾਵਾਂ()) ਦਲੀਲਾਂ += ", ";
                ਜੇ (nodep->dpiImportPrototype() || nodep->dpiExportDispatcher()) {
                    ਦਲੀਲਾਂ += portp->dpiArgType(ਸੱਚ, ਝੂਠ, cpp);
                } ਨਹੀਂਤਾਂ ਜੇ (nodep->func_ਜਨਤਕ()) {
                    ਦਲੀਲਾਂ += portp->cPubArgType(ਸੱਚ, ਝੂਠ);
                } ਨਹੀਂਤਾਂ {
                    ਦਲੀਲਾਂ += portp->vlArgType(ਸੱਚ, ਝੂਠ, ਸੱਚ);
                }
            }
        }
    }
    ਵਾਪਸ ਦਲੀਲਾਂ;
}

ਖਾਲੀ EmitCBaseVisitorConst::emitCDefaultConstructor(ਅਚਲ Ast_ਗੰਢ_Module* ਅਚਲ modp) {
    puts(EmitCUtil::ਅਗੇਤਰ_Name_Protect(modp));
    puts("() = default;\n");
}

ਖਾਲੀ EmitCBaseVisitorConst::emit_CFunc_ਸਿਰਲੇਖ(ਅਚਲ AstCFunc* funcp, ਅਚਲ Ast_ਗੰਢ_Module* modp,
                                            ਤਾਰਕਿਕ withScope) {
    ਜੇ (funcp->slow()) putns(funcp, "VL_ATTR_COLD ");
    ਜੇ (funcp->dpiCDeclOverride()) {
        putns(funcp, funcp->dpiCDecl() + ";\n");
        ਵਾਪਸ;
    }
    ਜੇ (!funcp->isDestructor()) {
        putns(funcp, funcp->rtnTypeVoid());
        puts(" ");
    }
    ਜੇ (withScope) {
        ਜੇ (funcp->dpiExportDispatcher()) {
            putns(funcp, EmitCUtil::ਸਿਖਰ_Class_Name() + "::");
        } ਨਹੀਂਤਾਂ ਜੇ (funcp->is_Proper_ਵਿਧੀ()) {
            putns(funcp, EmitCUtil::ਅਗੇਤਰ_Name_Protect(modp) + "::");
        }
    }
    putns(funcp, funcNameProtect(funcp, modp));
    puts("(" + c_Func_ਦਲੀਲਾਂ(funcp) + ")");
    ਜੇ (funcp->isConst().true_ਗਿਆਤ() && funcp->is_Proper_ਵਿਧੀ()) puts(" const");
}

ਖਾਲੀ EmitCBaseVisitorConst::emitCFuncDecl(ਅਚਲ AstCFunc* funcp, ਅਚਲ Ast_ਗੰਢ_Module* modp,
                                          ਤਾਰਕਿਕ cLinkage) {
    ensure_New_ਸਤਰ();
    ਜੇ (!funcp->ifdef().ਖਾਲੀ_ਥਾਵਾਂ()) putns(funcp, "#ifdef " + funcp->ifdef() + "\n");
    ਜੇ (funcp->isConstructor()) emitCDefaultConstructor(modp);
    ਜੇ (cLinkage) putns(funcp, "extern \"C\" ");
    ਜੇ (funcp->isStatic() && funcp->is_Proper_ਵਿਧੀ()) putns(funcp, "static ");
    ਜੇ (funcp->isVirtual()) {
        UASSERT_OBJ(funcp->is_Proper_ਵਿਧੀ(), funcp, "Virtual function is not a proper method");
        putns(funcp, "virtual ");
        // Intentionally no emit for "override" instead, as clang will then enable warning
        // on other methods where virtual vs override is needed, and this is not tracked yet
    }
    emit_CFunc_ਸਿਰਲੇਖ(funcp, modp, /* withScope: */ ਝੂਠ);
    ਅਚਲ AstClass* ਅਚਲ classp = VN_CAST(modp, Class);
    ਜੇ (funcp->ਖਾਲੀ_ਥਾਵਾਂ_Body() && !funcp->isLoose() && !cLinkage
        && !(funcp->isConstructor() && classp && classp->isInterfaceClass())) {
        putns(funcp, " {}\n");
    } ਨਹੀਂਤਾਂ {
        putns(funcp, ";\n");
    }
    ਜੇ (!funcp->ifdef().ਖਾਲੀ_ਥਾਵਾਂ()) putns(funcp, "#endif  // " + funcp->ifdef() + "\n");
}

std::string EmitCBaseVisitorConst::emit_ਵੰਡੋ_ਪਾਠ_Stmts(ਪਾਠ_Stmts& stmts, ਅਚਲ std::string& ਅਗੇਤਰ,
                                                      ਅਚਲ std::string& params,
                                                      ਅਚਲ std::string& ਦਲੀਲਾਂ,  //
                                                      ਮਾਪ_t max_ਲਾਗਤ, ਤਾਰਕਿਕ support,
                                                      ਅਚਲ ਅੱਖਰ* descriptionp,
                                                      ਅਚਲ std::function<ਖਾਲੀ()>& emit_ਸਿਰਲੇਖ) {
    ਅਚਲ ਸਵੈਚਾਲਿਤ subFuncName = [&](ਮਾਪ_t n) { ਵਾਪਸ ਅਗੇਤਰ + "__" + std::ਮੰਜ਼ਿਲ_string(n); };
    ਮਾਪ_t nSubFunctions = 0;
    std::string funcName;  // Name of last sub-function, which is the root at the end
    // After the first level, statement 'j' is a call to sub-function 'calleeBase + j'
    ਤਾਰਕਿਕ callsSubFuncs = ਝੂਠ;
    ਮਾਪ_t callee_ਅਧਾਰ = 0;
    // Reduce into a balanced tree of sub-function calls until we end up with a single statement
    ਜਦੋਂ (stmts.ਮਾਪ() > 1) {
        ਅਚਲ ਮਾਪ_t level_ਅਧਾਰ = nSubFunctions;
        ਪਾਠ_Stmts calls;  // Calls to the sub-functions of this level
        ਅਚਲ ਮਾਪ_t nStmts = stmts.ਮਾਪ();
        ਲਈ (ਮਾਪ_t ਵੰਡੋ_ਸ਼ੁਰੂ = 0, ਵੰਡੋ_ਸਿਰਾ = 0; ਵੰਡੋ_ਸ਼ੁਰੂ < nStmts; ਵੰਡੋ_ਸ਼ੁਰੂ = ਵੰਡੋ_ਸਿਰਾ) {
            // Gather up at at most 'maxCost' worth of statements in this split,
            // but always at least 2 (if less than 2, the reduction makes no
            // progress and the loop will not terminate).
            ਮਾਪ_t ਲਾਗਤ = 0;
            ਜਦੋਂ (((ਲਾਗਤ < max_ਲਾਗਤ) || (ਵੰਡੋ_ਸਿਰਾ - ਵੰਡੋ_ਸ਼ੁਰੂ < 2)) && ਵੰਡੋ_ਸਿਰਾ < nStmts) {
                ਲਾਗਤ += stmts[ਵੰਡੋ_ਸਿਰਾ++].m_ਲਾਗਤ;
            }
            UASSERT(ਵੰਡੋ_ਸ਼ੁਰੂ < ਵੰਡੋ_ਸਿਰਾ, "Empty split");

            // Name of sub-function we are emitting now
            funcName = subFuncName(nSubFunctions++);

            // Open split file
            open_New_Output_ਸਰੋਤ_File(funcName, ਸੱਚ, support, descriptionp);
            // Emit header
            emit_ਸਿਰਲੇਖ();
            // Declare called sub-functions
            ਜੇ (callsSubFuncs) {
                puts("\n");
                ਲਈ (ਮਾਪ_t j = ਵੰਡੋ_ਸ਼ੁਰੂ; j < ਵੰਡੋ_ਸਿਰਾ; ++j) {
                    puts("void " + subFuncName(callee_ਅਧਾਰ + j) + "(" + params + ");\n");
                }
                puts("\n");
            }
            // Open sub-function definition in the split file
            puts("void " + funcName + "(" + params + ") {\n");
            // Emit statements
            ਲਈ (ਮਾਪ_t j = ਵੰਡੋ_ਸ਼ੁਰੂ; j < ਵੰਡੋ_ਸਿਰਾ; ++j) {
                ofp()->putsNoTracking("    ");
                ofp()->putsNoTracking(stmts[j].m_ਪਾਠ);
                ofp()->putsNoTracking("\n");
            }
            // Close sub-function
            puts("}\n");
            // Close split file
            closeOutputFile();

            // Replace statements with a call to the sub-function
            calls.ਜੋੜੋ(funcName + "(" + ਦਲੀਲਾਂ + ");");
        }
        stmts = std::move(calls);
        callsSubFuncs = ਸੱਚ;
        callee_ਅਧਾਰ = level_ਅਧਾਰ;
    }
    ਵਾਪਸ funcName.ਖਾਲੀ_ਥਾਵਾਂ() ? "" : "void " + funcName + "(" + params + ");\n";
}

std::pair<std::string, std::string> EmitCBaseVisitorConst::emit_ਵੰਡੋ_Ctor_Dtor(
    ਪਾਠ_Stmts& ctorStmts, ਪਾਠ_Stmts& dtorStmts, ਮਾਪ_t ਅਧਾਰ_ਲਾਗਤ, ਅਚਲ std::string& ਅਗੇਤਰ,
    ਅਚਲ std::string& params, ਅਚਲ std::string& ਦਲੀਲਾਂ, ਤਾਰਕਿਕ support, ਅਚਲ ਅੱਖਰ* descriptionp,
    ਅਚਲ std::function<ਖਾਲੀ()>& emit_ਸਿਰਲੇਖ) {
    ਅਚਲ ਮਾਪ_t max_ਲਾਗਤ = static_cast<ਮਾਪ_t>(v3Global.opt.output_ਵੰਡੋ_CFuncs());
    ਜੇ (!max_ਲਾਗਤ) ਵਾਪਸ {};
    ਮਾਪ_t ਕੁੱਲ_ਲਾਗਤ = ਅਧਾਰ_ਲਾਗਤ;
    ਲਈ (ਅਚਲ ਪਾਠ_Stmts::Stmt& stmt : ctorStmts) ਕੁੱਲ_ਲਾਗਤ += stmt.m_ਲਾਗਤ;
    ਲਈ (ਅਚਲ ਪਾਠ_Stmts::Stmt& stmt : dtorStmts) ਕੁੱਲ_ਲਾਗਤ += stmt.m_ਲਾਗਤ;
    ਜੇ (ਕੁੱਲ_ਲਾਗਤ <= max_ਲਾਗਤ) ਵਾਪਸ {};
    v3Global.useParallelBuild(ਸੱਚ);  // Splitting files, so using parallel build.
    std::string ctorDecl = emit_ਵੰਡੋ_ਪਾਠ_Stmts(ctorStmts, ਅਗੇਤਰ + "__ctor", params, ਦਲੀਲਾਂ, max_ਲਾਗਤ,
                                              support, descriptionp, emit_ਸਿਰਲੇਖ);
    std::string dtorDecl = emit_ਵੰਡੋ_ਪਾਠ_Stmts(dtorStmts, ਅਗੇਤਰ + "__dtor", params, ਦਲੀਲਾਂ, max_ਲਾਗਤ,
                                              support, descriptionp, emit_ਸਿਰਲੇਖ);
    ਵਾਪਸ {std::move(ctorDecl), std::move(dtorDecl)};
}

ਖਾਲੀ EmitCBaseVisitorConst::emitVarDecl(ਅਚਲ AstVar* nodep, ਤਾਰਕਿਕ asRef) {
    ਅਚਲ AstBasicDType* ਅਚਲ basicp = nodep->basicp();
    ਅਚਲ ਤਾਰਕਿਕ refNeedParens = VN_IS(nodep->dtypeSkipRefp(), UnpackArrayDType);
    ਜੇ (nodep->mtask_Cache_ਸਤਰ_Align() && !asRef) putns(nodep, "alignas(VL_CACHE_LINE_BYTES) ");
    ਜੇ (nodep->covergroupRefMember()) {
        UASSERT_OBJ(!asRef, nodep, "Covergroup reference member emitted as C++ reference");
        putns(nodep, nodep->dtypep()->cType("", ਝੂਠ, ਝੂਠ));
        puts(" const* " + nodep->nameProtect() + " = nullptr;\n");
        ਵਾਪਸ;
    }

    ਅਚਲ ਸਵੈਚਾਲਿਤ emitDeclArrayBrackets = [ਇਹ](ਅਚਲ AstVar* nodep) -> ਖਾਲੀ {
        // This isn't very robust and may need cleanup for other data types
        ਲਈ (ਅਚਲ AstUnpackArrayDType* arrayp = VN_CAST(nodep->dtypeSkipRefp(), UnpackArrayDType);
             arrayp; arrayp = VN_CAST(arrayp->subDTypep()->skipRefp(), UnpackArrayDType)) {
            putns(arrayp, "[" + cvt_ਮੰਜ਼ਿਲ_Str(arrayp->elementsConst()) + "]");
        }
    };

    ਜੇ (nodep->isPrimaryIO() && nodep->isSc()) {
        UASSERT_OBJ(basicp, nodep, "Unimplemented: Outputting this data type");
        ਜੇ (nodep->isInout()) {
            putns(nodep, "sc_core::sc_inout<");
        } ਨਹੀਂਤਾਂ ਜੇ (nodep->isWritable()) {
            putns(nodep, "sc_core::sc_out<");
        } ਨਹੀਂਤਾਂ ਜੇ (nodep->isNonOutput()) {
            putns(nodep, "sc_core::sc_in<");
        } ਨਹੀਂਤਾਂ {
            nodep->v3fatalSrc("Unknown type");
        }
        puts(nodep->scType());
        puts("> ");
        ਜੇ (asRef) {
            ਜੇ (refNeedParens) putns(nodep, "(");
            putns(nodep, "&");
        }
        putns(nodep, nodep->nameProtect());
        ਜੇ (asRef && refNeedParens) puts(")");
        emitDeclArrayBrackets(nodep);
        puts(";\n");
    } ਨਹੀਂਤਾਂ ਜੇ (nodep->isVLIO()) {
        ਜੇ (nodep->isInout()) {
            putns(nodep, "VL_INOUT");
        } ਨਹੀਂਤਾਂ ਜੇ (nodep->isWritable()) {
            putns(nodep, "VL_OUT");
        } ਨਹੀਂਤਾਂ ਜੇ (nodep->isNonOutput()) {
            putns(nodep, "VL_IN");
        } ਨਹੀਂਤਾਂ {
            nodep->v3fatalSrc("Unknown type");
        }

        ਜੇ (nodep->isQuad()) {
            puts("64");
        } ਨਹੀਂਤਾਂ ਜੇ (nodep->ਚੌੜਾਈ_Min() <= 8) {
            puts("8");
        } ਨਹੀਂਤਾਂ ਜੇ (nodep->ਚੌੜਾਈ_Min() <= 16) {
            puts("16");
        } ਨਹੀਂਤਾਂ ਜੇ (nodep->isWide()) {
            puts("W");
        }

        puts("(");
        ਜੇ (asRef) {
            ਜੇ (refNeedParens) puts("(");
            puts("&");
        }
        puts(nodep->nameProtect());
        ਜੇ (asRef && refNeedParens) puts(")");
        emitDeclArrayBrackets(nodep);
        // If it's a packed struct/array then nodep->width is the whole
        // thing, msb/lsb is just lowest dimension
        puts("," + cvt_ਮੰਜ਼ਿਲ_Str(basicp->lo() + nodep->ਚੌੜਾਈ() - 1) + "," + cvt_ਮੰਜ਼ਿਲ_Str(basicp->lo()));
        ਜੇ (nodep->isWide()) puts("," + cvt_ਮੰਜ਼ਿਲ_Str(nodep->ਚੌੜਾਈ_ਸ਼ਬਦ()));
        puts(");\n");
    } ਨਹੀਂਤਾਂ {
        // Strings and other fundamental C types
        ਜੇ (nodep->isFuncLocal() && nodep->dtypep()->ਸ਼ਾਮਲ_ਹੈ_String()) {
            ਅਚਲ string name = nodep->name();
            ਅਚਲ string suffix = V3Task::dpiTemporaryVarSuffix();
            // Temporary variable for DPI-C containing strings needs to be static because c_str()
            // will be passed to C code and the lifetime of the variable must be long enough. See
            // also Issue 2622.
            ਅਚਲ ਤਾਰਕਿਕ beStatic = name.ਮਾਪ() >= suffix.ਮਾਪ()
                                  && name.substr(name.ਮਾਪ() - suffix.ਮਾਪ()) == suffix;
            ਜੇ (beStatic) puts("static thread_local ");
        }
        putns(nodep, nodep->vlArgType(ਸੱਚ, ਝੂਠ, ਝੂਠ, asRef));
        puts(";\n");
    }
}

ਖਾਲੀ EmitCBaseVisitorConst::emitVarAccessors(ਅਚਲ AstVar* nodep) {
    ਪ੍ਰਤਿਜ੍ਞਾ(nodep->name().rfind("__Vm_sig_") == 0 && nodep->isIO());
    ਅਚਲ string ਨਿੱਜੀ_Name = nodep->name();
    ਅਚਲ string ਜਨਤਕ_Name = nodep->name().substr(ਲੜੀ_ਲੰਬਾਈ("__Vm_sig_"));

    puts("decltype("s + ਨਿੱਜੀ_Name + ") "s + ਜਨਤਕ_Name + "() {return "s + ਨਿੱਜੀ_Name + ";}\n");
    puts("void "s + ਜਨਤਕ_Name + "(decltype(" + ਨਿੱਜੀ_Name + ") v) {"s + ਨਿੱਜੀ_Name + "=v;}\n");
}

ਖਾਲੀ EmitCBaseVisitorConst::emitModCUse(ਅਚਲ Ast_ਗੰਢ_Module* modp, VUseType useType) {
    ਤਾਰਕਿਕ nl = ਝੂਠ;
    forModCUse(modp, useType, [&](ਅਚਲ string& ਇੰਦਰਾਜ) {
        puts(ਇੰਦਰਾਜ);
        nl = ਸੱਚ;
    });
    ਜੇ (nl) puts("\n");
}

std::pair<string, File_ਸਤਰ*> EmitCBaseVisitorConst::scSection(ਅਚਲ Ast_ਗੰਢ_Module* modp,
                                                              VSystemCSectionType type) {
    string ਪਾਠ;
    File_ਸਤਰ* fl = ਖਾਲੀ_ਸੂਚਕ;
    ਪੂਰਨ ਆਖਰੀ_ਸਤਰ = -999;
    ਲਈ (ਅਚਲ AstSystemCSection* scSectionp = modp->scSectionsp(); scSectionp;
         scSectionp = VN_AS(scSectionp->nextp(), SystemCSection)) {
        ਜੇ (scSectionp->sectionType() != type) ਜਾਰੀ;
        ਜੇ (ਪਾਠ.ਖਾਲੀ_ਥਾਵਾਂ()) {
            fl = scSectionp->fileline();
            ਪਾਠ += "\n";
            ਜੇ (v3Global.opt.decoration()) {
                ਪਾਠ += "\n//*** Below code from `systemc in Verilog file\n";
            }
        }
        ਜੇ (ਆਖਰੀ_ਸਤਰ + 1 != scSectionp->fileline()->lineno() && v3Global.opt.decoration()) {
            ਪਾਠ += "// From `systemc at " + scSectionp->fileline()->ascii() + "\n";
        }
        ਆਖਰੀ_ਸਤਰ = scSectionp->fileline()->lineno();
        ਪਾਠ += scSectionp->ਪਾਠ();
    }
    ਜੇ (!ਪਾਠ.ਖਾਲੀ_ਥਾਵਾਂ()) {
        ਜੇ (v3Global.opt.decoration()) ਪਾਠ += "//*** Above code from `systemc in Verilog file\n";
        ਪਾਠ += "\n";
        // Substitute `systemc_class_name
        string::ਮਾਪ_type pos;
        ਜਦੋਂ ((pos = ਪਾਠ.ਲੱਭੋ("`systemc_class_name")) != string::npos) {
            ਪਾਠ.replace(pos, std::ਲੜੀ_ਲੰਬਾਈ("`systemc_class_name"),
                         EmitCUtil::ਅਗੇਤਰ_Name_Protect(modp));
        }
    }
    ਵਾਪਸ std::make_pair(ਪਾਠ, fl);
}

ਖਾਲੀ EmitCBaseVisitorConst::emitSystemCSection(ਅਚਲ Ast_ਗੰਢ_Module* modp,
                                               VSystemCSectionType type) {
    ਸਵੈਚਾਲਿਤ ਪਾਠ_And_Fileline = scSection(modp, type);
    ਜੇ (!ਪਾਠ_And_Fileline.ਪਹਿਲਾ.ਖਾਲੀ_ਥਾਵਾਂ()) ofp()->putsNoTracking(ਪਾਠ_And_Fileline.ਪਹਿਲਾ);
}
