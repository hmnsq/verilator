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

#अन्तर्भाव "V3PchAstMT.h"

#अन्तर्भाव "V3EmitCBase.h"

#अन्तर्भाव "V3Task.h"

//######################################################################
// EmitCParentModule implementation

EmitCParentModule::EmitCParentModule() {
    ध्रुव स्वचालित समुच्चयः_All = [](Ast_ग्रन्थिः_Module* modp) -> शून्य {
        प्रति (Ast_ग्रन्थिः* nodep = modp->stmtsp(); nodep; nodep = nodep->nextp()) {
            यदि (VN_IS(nodep, CFunc) || VN_IS(nodep, Var) || VN_IS(nodep, ग्रन्थिः_Cover_Decl)) {
                nodep->user4p(modp);
            }
        }
        modp->foreach([&](Ast_ग्रन्थिः_Cover_Decl* nodep) { nodep->user4p(modp); });
    };
    प्रति (Ast_ग्रन्थिः* modp = v3Global.rootp()->modulesp(); modp; modp = modp->nextp()) {
        समुच्चयः_All(VN_AS(modp, ग्रन्थिः_Module));
    }
}

//######################################################################
// EmitCUtil implementation

string EmitCUtil::उपसर्गः_Name_Protect(ध्रुव Ast_ग्रन्थिः* nodep) VL_MT_STABLE {
    ध्रुव string उपसर्गः = v3Global.opt.mod_उपसर्गः() + "_" + VIdProtect::protect(nodep->name());
    // If all-uppercase prefix conflicts with a previous usage of the
    // prefix with different capitalization, rename to avoid conflict.
    // This is to support OSes where filename compares are non-case significant.
    // STATIC:
    स्थिर V3Mutex s_mutex;
    ध्रुव V3_तालकम्_Guard तालकम्_2{s_mutex};  // Otherwise map access is unsafe
    स्थिर std::map<std::string, std::string> s_memoized;
    स्थिर std::map<std::string, std::string> s_uc_गन्तव्यम्_उपसर्गः;
    // Memoize results
    ध्रुव स्वचालित mit = s_memoized.find(उपसर्गः);
    यदि (mit != s_memoized.अन्तः_2()) प्रत्यागम mit->second;
    //
    // Check capitalization
    ध्रुव string उपसर्गः_ऊर्ध्वस्थम् = VString::upcase(उपसर्गः);
    string परिणाम;
    {
        ध्रुव स्वचालित it = s_uc_गन्तव्यम्_उपसर्गः.find(उपसर्गः_ऊर्ध्वस्थम्);
        यदि (it == s_uc_गन्तव्यम्_उपसर्गः.अन्तः_2()) {
            s_uc_गन्तव्यम्_उपसर्गः.emplace(उपसर्गः_ऊर्ध्वस्थम्, उपसर्गः);
            परिणाम = उपसर्गः;
        } अन्यथा यदि (it->second == उपसर्गः) {
            परिणाम = उपसर्गः;  // Same capitialization as last time
        } अन्यथा {
            VHashSha512 hash{उपसर्गः};
            परिणाम = उपसर्गः + "__Vphsh" + hash.digestSymbol24();
        }
    }
    s_memoized.emplace(उपसर्गः, परिणाम);
    प्रत्यागम परिणाम;
}

//######################################################################
// EmitCBaseVisitor implementation

string EmitCBaseVisitorConst::funcNameProtect(ध्रुव AstCFunc* nodep, ध्रुव Ast_ग्रन्थिः_Module* modp) {
    modp = modp ? modp : EmitCParentModule::get(nodep);
    string name;
    यदि (nodep->isConstructor()) {
        name += "init";
    } अन्यथा यदि (nodep->isDestructor()) {
        name += "~";
        name += EmitCUtil::उपसर्गः_Name_Protect(modp);
    } अन्यथा {
        यदि (nodep->isLoose()) {
            name += EmitCUtil::उपसर्गः_Name_Protect(modp);
            name += "__";
        }
        name += nodep->nameProtect();
    }
    प्रत्यागम name;
}

string EmitCBaseVisitorConst::c_Func_तर्काः(ध्रुव AstCFunc* nodep, तार्किक cpp) {
    // Return argument list for given C function
    string तर्काः;
    यदि (nodep->isLoose() && !nodep->isStatic()) {
        यदि (nodep->voidSelf()) {
            तर्काः += "void* vlVoidSelf";  // Converted to 'vlSelf' in the function body
        } अन्यथा {
            यदि (nodep->isConst().true_ज्ञातम्()) तर्काः += "const ";
            तर्काः += EmitCUtil::उपसर्गः_Name_Protect(EmitCParentModule::get(nodep));
            तर्काः += "* vlSelf";
        }
    }
    यदि (nodep->needProcess()) {
        यदि (!तर्काः.रिक्तानि()) तर्काः += ", ";
        तर्काः += "VlProcessRef vlProcess";
    }
    यदि (!nodep->argTypes().रिक्तानि()) {
        यदि (!तर्काः.रिक्तानि()) तर्काः += ", ";
        तर्काः += nodep->argTypes();
    }
    // Might be a user function with argument list.
    प्रति (ध्रुव Ast_ग्रन्थिः* stmtp = nodep->argsp(); stmtp; stmtp = stmtp->nextp()) {
        यदि (ध्रुव AstVar* ध्रुव portp = VN_CAST(stmtp, Var)) {
            यदि (portp->isIO() && !portp->isFuncReturn()) {
                यदि (!तर्काः.रिक्तानि()) तर्काः += ", ";
                यदि (nodep->dpiImportPrototype() || nodep->dpiExportDispatcher()) {
                    तर्काः += portp->dpiArgType(सत्य, असत्य, cpp);
                } अन्यथा यदि (nodep->func_लौकिकम्()) {
                    तर्काः += portp->cPubArgType(सत्य, असत्य);
                } अन्यथा {
                    तर्काः += portp->vlArgType(सत्य, असत्य, सत्य);
                }
            }
        }
    }
    प्रत्यागम तर्काः;
}

शून्य EmitCBaseVisitorConst::emitCDefaultConstructor(ध्रुव Ast_ग्रन्थिः_Module* ध्रुव modp) {
    पङ्क्तिं_लिख(EmitCUtil::उपसर्गः_Name_Protect(modp));
    पङ्क्तिं_लिख("() = default;\n");
}

शून्य EmitCBaseVisitorConst::emit_CFunc_शीर्षकम्(ध्रुव AstCFunc* funcp, ध्रुव Ast_ग्रन्थिः_Module* modp,
                                            तार्किक withScope) {
    यदि (funcp->slow()) putns(funcp, "VL_ATTR_COLD ");
    यदि (funcp->dpiCDeclOverride()) {
        putns(funcp, funcp->dpiCDecl() + ";\n");
        प्रत्यागम;
    }
    यदि (!funcp->isDestructor()) {
        putns(funcp, funcp->rtnTypeVoid());
        पङ्क्तिं_लिख(" ");
    }
    यदि (withScope) {
        यदि (funcp->dpiExportDispatcher()) {
            putns(funcp, EmitCUtil::शिखरम्_Class_Name() + "::");
        } अन्यथा यदि (funcp->is_Proper_विधानम्()) {
            putns(funcp, EmitCUtil::उपसर्गः_Name_Protect(modp) + "::");
        }
    }
    putns(funcp, funcNameProtect(funcp, modp));
    पङ्क्तिं_लिख("(" + c_Func_तर्काः(funcp) + ")");
    यदि (funcp->isConst().true_ज्ञातम्() && funcp->is_Proper_विधानम्()) पङ्क्तिं_लिख(" const");
}

शून्य EmitCBaseVisitorConst::emitCFuncDecl(ध्रुव AstCFunc* funcp, ध्रुव Ast_ग्रन्थिः_Module* modp,
                                          तार्किक cLinkage) {
    ensure_New_पङ्क्तिः();
    यदि (!funcp->ifdef().रिक्तानि()) putns(funcp, "#ifdef " + funcp->ifdef() + "\n");
    यदि (funcp->isConstructor()) emitCDefaultConstructor(modp);
    यदि (cLinkage) putns(funcp, "extern \"C\" ");
    यदि (funcp->isStatic() && funcp->is_Proper_विधानम्()) putns(funcp, "static ");
    यदि (funcp->isVirtual()) {
        UASSERT_OBJ(funcp->is_Proper_विधानम्(), funcp, "Virtual function is not a proper method");
        putns(funcp, "virtual ");
        // Intentionally no emit for "override" instead, as clang will then enable warning
        // on other methods where virtual vs override is needed, and this is not tracked yet
    }
    emit_CFunc_शीर्षकम्(funcp, modp, /* withScope: */ असत्य);
    ध्रुव AstClass* ध्रुव classp = VN_CAST(modp, Class);
    यदि (funcp->रिक्तानि_Body() && !funcp->isLoose() && !cLinkage
        && !(funcp->isConstructor() && classp && classp->isInterfaceClass())) {
        putns(funcp, " {}\n");
    } अन्यथा {
        putns(funcp, ";\n");
    }
    यदि (!funcp->ifdef().रिक्तानि()) putns(funcp, "#endif  // " + funcp->ifdef() + "\n");
}

std::string EmitCBaseVisitorConst::emit_विभज_Text_Stmts(TextStmts& stmts, ध्रुव std::string& उपसर्गः,
                                                      ध्रुव std::string& params,
                                                      ध्रुव std::string& तर्काः,  //
                                                      आकार_t maxCost, तार्किक support,
                                                      ध्रुव वर्ण* descriptionp,
                                                      ध्रुव std::function<शून्य()>& emit_शीर्षकम्) {
    ध्रुव स्वचालित subFuncName = [&](आकार_t n) { प्रत्यागम उपसर्गः + "__" + std::गन्तव्यम्_string(n); };
    आकार_t nSubFunctions = 0;
    std::string funcName;  // Name of last sub-function, which is the root at the end
    // After the first level, statement 'j' is a call to sub-function 'calleeBase + j'
    तार्किक callsSubFuncs = असत्य;
    आकार_t callee_आधारः = 0;
    // Reduce into a balanced tree of sub-function calls until we end up with a single statement
    यावत् (stmts.आकार() > 1) {
        ध्रुव आकार_t level_आधारः = nSubFunctions;
        TextStmts calls;  // Calls to the sub-functions of this level
        ध्रुव आकार_t nStmts = stmts.आकार();
        प्रति (आकार_t विभज_Start = 0, विभज_अन्तः = 0; विभज_Start < nStmts; विभज_Start = विभज_अन्तः) {
            // Gather up at at most 'maxCost' worth of statements in this split,
            // but always at least 2 (if less than 2, the reduction makes no
            // progress and the loop will not terminate).
            आकार_t cost = 0;
            यावत् (((cost < maxCost) || (विभज_अन्तः - विभज_Start < 2)) && विभज_अन्तः < nStmts) {
                cost += stmts[विभज_अन्तः++].m_cost;
            }
            UASSERT(विभज_Start < विभज_अन्तः, "Empty split");

            // Name of sub-function we are emitting now
            funcName = subFuncName(nSubFunctions++);

            // Open split file
            openNewOutputSourceFile(funcName, सत्य, support, descriptionp);
            // Emit header
            emit_शीर्षकम्();
            // Declare called sub-functions
            यदि (callsSubFuncs) {
                पङ्क्तिं_लिख("\n");
                प्रति (आकार_t j = विभज_Start; j < विभज_अन्तः; ++j) {
                    पङ्क्तिं_लिख("void " + subFuncName(callee_आधारः + j) + "(" + params + ");\n");
                }
                पङ्क्तिं_लिख("\n");
            }
            // Open sub-function definition in the split file
            पङ्क्तिं_लिख("void " + funcName + "(" + params + ") {\n");
            // Emit statements
            प्रति (आकार_t j = विभज_Start; j < विभज_अन्तः; ++j) {
                ofp()->putsNoTracking("    ");
                ofp()->putsNoTracking(stmts[j].m_text);
                ofp()->putsNoTracking("\n");
            }
            // Close sub-function
            पङ्क्तिं_लिख("}\n");
            // Close split file
            closeOutputFile();

            // Replace statements with a call to the sub-function
            calls.सङ्कलय(funcName + "(" + तर्काः + ");");
        }
        stmts = std::move(calls);
        callsSubFuncs = सत्य;
        callee_आधारः = level_आधारः;
    }
    प्रत्यागम funcName.रिक्तानि() ? "" : "void " + funcName + "(" + params + ");\n";
}

std::pair<std::string, std::string> EmitCBaseVisitorConst::emit_विभज_Ctor_Dtor(
    TextStmts& ctorStmts, TextStmts& dtorStmts, आकार_t आधारः_Cost, ध्रुव std::string& उपसर्गः,
    ध्रुव std::string& params, ध्रुव std::string& तर्काः, तार्किक support, ध्रुव वर्ण* descriptionp,
    ध्रुव std::function<शून्य()>& emit_शीर्षकम्) {
    ध्रुव आकार_t maxCost = static_cast<आकार_t>(v3Global.opt.output_विभज_CFuncs());
    यदि (!maxCost) प्रत्यागम {};
    आकार_t योग_Cost = आधारः_Cost;
    प्रति (ध्रुव TextStmts::Stmt& stmt : ctorStmts) योग_Cost += stmt.m_cost;
    प्रति (ध्रुव TextStmts::Stmt& stmt : dtorStmts) योग_Cost += stmt.m_cost;
    यदि (योग_Cost <= maxCost) प्रत्यागम {};
    v3Global.useParallelBuild(सत्य);  // Splitting files, so using parallel build.
    std::string ctorDecl = emit_विभज_Text_Stmts(ctorStmts, उपसर्गः + "__ctor", params, तर्काः, maxCost,
                                              support, descriptionp, emit_शीर्षकम्);
    std::string dtorDecl = emit_विभज_Text_Stmts(dtorStmts, उपसर्गः + "__dtor", params, तर्काः, maxCost,
                                              support, descriptionp, emit_शीर्षकम्);
    प्रत्यागम {std::move(ctorDecl), std::move(dtorDecl)};
}

शून्य EmitCBaseVisitorConst::emitVarDecl(ध्रुव AstVar* nodep, तार्किक asRef) {
    ध्रुव AstBasicDType* ध्रुव basicp = nodep->basicp();
    ध्रुव तार्किक refNeedParens = VN_IS(nodep->dtypeSkipRefp(), UnpackArrayDType);
    यदि (nodep->mtask_Cache_पङ्क्तिः_Align() && !asRef) putns(nodep, "alignas(VL_CACHE_LINE_BYTES) ");
    यदि (nodep->covergroupRefMember()) {
        UASSERT_OBJ(!asRef, nodep, "Covergroup reference member emitted as C++ reference");
        putns(nodep, nodep->dtypep()->cType("", असत्य, असत्य));
        पङ्क्तिं_लिख(" const* " + nodep->nameProtect() + " = nullptr;\n");
        प्रत्यागम;
    }

    ध्रुव स्वचालित emitDeclArrayBrackets = [एतत्](ध्रुव AstVar* nodep) -> शून्य {
        // This isn't very robust and may need cleanup for other data types
        प्रति (ध्रुव AstUnpackArrayDType* arrayp = VN_CAST(nodep->dtypeSkipRefp(), UnpackArrayDType);
             arrayp; arrayp = VN_CAST(arrayp->subDTypep()->skipRefp(), UnpackArrayDType)) {
            putns(arrayp, "[" + cvt_गन्तव्यम्_Str(arrayp->elementsConst()) + "]");
        }
    };

    यदि (nodep->isPrimaryIO() && nodep->isSc()) {
        UASSERT_OBJ(basicp, nodep, "Unimplemented: Outputting this data type");
        यदि (nodep->isInout()) {
            putns(nodep, "sc_core::sc_inout<");
        } अन्यथा यदि (nodep->isWritable()) {
            putns(nodep, "sc_core::sc_out<");
        } अन्यथा यदि (nodep->isNonOutput()) {
            putns(nodep, "sc_core::sc_in<");
        } अन्यथा {
            nodep->v3fatalSrc("Unknown type");
        }
        पङ्क्तिं_लिख(nodep->scType());
        पङ्क्तिं_लिख("> ");
        यदि (asRef) {
            यदि (refNeedParens) putns(nodep, "(");
            putns(nodep, "&");
        }
        putns(nodep, nodep->nameProtect());
        यदि (asRef && refNeedParens) पङ्क्तिं_लिख(")");
        emitDeclArrayBrackets(nodep);
        पङ्क्तिं_लिख(";\n");
    } अन्यथा यदि (nodep->isVLIO()) {
        यदि (nodep->isInout()) {
            putns(nodep, "VL_INOUT");
        } अन्यथा यदि (nodep->isWritable()) {
            putns(nodep, "VL_OUT");
        } अन्यथा यदि (nodep->isNonOutput()) {
            putns(nodep, "VL_IN");
        } अन्यथा {
            nodep->v3fatalSrc("Unknown type");
        }

        यदि (nodep->isQuad()) {
            पङ्क्तिं_लिख("64");
        } अन्यथा यदि (nodep->विस्तारः_Min() <= 8) {
            पङ्क्तिं_लिख("8");
        } अन्यथा यदि (nodep->विस्तारः_Min() <= 16) {
            पङ्क्तिं_लिख("16");
        } अन्यथा यदि (nodep->isWide()) {
            पङ्क्तिं_लिख("W");
        }

        पङ्क्तिं_लिख("(");
        यदि (asRef) {
            यदि (refNeedParens) पङ्क्तिं_लिख("(");
            पङ्क्तिं_लिख("&");
        }
        पङ्क्तिं_लिख(nodep->nameProtect());
        यदि (asRef && refNeedParens) पङ्क्तिं_लिख(")");
        emitDeclArrayBrackets(nodep);
        // If it's a packed struct/array then nodep->width is the whole
        // thing, msb/lsb is just lowest dimension
        पङ्क्तिं_लिख("," + cvt_गन्तव्यम्_Str(basicp->lo() + nodep->विस्तारः() - 1) + "," + cvt_गन्तव्यम्_Str(basicp->lo()));
        यदि (nodep->isWide()) पङ्क्तिं_लिख("," + cvt_गन्तव्यम्_Str(nodep->विस्तारः_शब्दाः()));
        पङ्क्तिं_लिख(");\n");
    } अन्यथा {
        // Strings and other fundamental C types
        यदि (nodep->isFuncLocal() && nodep->dtypep()->अन्तर्भवति_String()) {
            ध्रुव string name = nodep->name();
            ध्रुव string suffix = V3Task::dpiTemporaryVarSuffix();
            // Temporary variable for DPI-C containing strings needs to be static because c_str()
            // will be passed to C code and the lifetime of the variable must be long enough. See
            // also Issue 2622.
            ध्रुव तार्किक beStatic = name.आकार() >= suffix.आकार()
                                  && name.substr(name.आकार() - suffix.आकार()) == suffix;
            यदि (beStatic) पङ्क्तिं_लिख("static thread_local ");
        }
        putns(nodep, nodep->vlArgType(सत्य, असत्य, असत्य, asRef));
        पङ्क्तिं_लिख(";\n");
    }
}

शून्य EmitCBaseVisitorConst::emitVarAccessors(ध्रुव AstVar* nodep) {
    प्रतिज्ञा(nodep->name().rfind("__Vm_sig_") == 0 && nodep->isIO());
    ध्रुव string वैयक्तिकम्_Name = nodep->name();
    ध्रुव string लौकिकम्_Name = nodep->name().substr(वाक्यदैर्घ्यम्("__Vm_sig_"));

    पङ्क्तिं_लिख("decltype("s + वैयक्तिकम्_Name + ") "s + लौकिकम्_Name + "() {return "s + वैयक्तिकम्_Name + ";}\n");
    पङ्क्तिं_लिख("void "s + लौकिकम्_Name + "(decltype(" + वैयक्तिकम्_Name + ") v) {"s + वैयक्तिकम्_Name + "=v;}\n");
}

शून्य EmitCBaseVisitorConst::emitModCUse(ध्रुव Ast_ग्रन्थिः_Module* modp, VUseType useType) {
    तार्किक nl = असत्य;
    forModCUse(modp, useType, [&](ध्रुव string& प्रविष्टिः) {
        पङ्क्तिं_लिख(प्रविष्टिः);
        nl = सत्य;
    });
    यदि (nl) पङ्क्तिं_लिख("\n");
}

std::pair<string, File_पङ्क्तिः*> EmitCBaseVisitorConst::scSection(ध्रुव Ast_ग्रन्थिः_Module* modp,
                                                              VSystemCSectionType type) {
    string text;
    File_पङ्क्तिः* fl = शून्यसूचकः;
    पूर्णाङ्क चरमः_पङ्क्तिः = -999;
    प्रति (ध्रुव AstSystemCSection* scSectionp = modp->scSectionsp(); scSectionp;
         scSectionp = VN_AS(scSectionp->nextp(), SystemCSection)) {
        यदि (scSectionp->sectionType() != type) अनुवर्त;
        यदि (text.रिक्तानि()) {
            fl = scSectionp->fileline();
            text += "\n";
            यदि (v3Global.opt.decoration()) {
                text += "\n//*** Below code from `systemc in Verilog file\n";
            }
        }
        यदि (चरमः_पङ्क्तिः + 1 != scSectionp->fileline()->lineno() && v3Global.opt.decoration()) {
            text += "// From `systemc at " + scSectionp->fileline()->ascii() + "\n";
        }
        चरमः_पङ्क्तिः = scSectionp->fileline()->lineno();
        text += scSectionp->text();
    }
    यदि (!text.रिक्तानि()) {
        यदि (v3Global.opt.decoration()) text += "//*** Above code from `systemc in Verilog file\n";
        text += "\n";
        // Substitute `systemc_class_name
        string::आकार_type pos;
        यावत् ((pos = text.find("`systemc_class_name")) != string::npos) {
            text.replace(pos, std::वाक्यदैर्घ्यम्("`systemc_class_name"),
                         EmitCUtil::उपसर्गः_Name_Protect(modp));
        }
    }
    प्रत्यागम std::make_pair(text, fl);
}

शून्य EmitCBaseVisitorConst::emitSystemCSection(ध्रुव Ast_ग्रन्थिः_Module* modp,
                                               VSystemCSectionType type) {
    स्वचालित textAndFileline = scSection(modp, type);
    यदि (!textAndFileline.first.रिक्तानि()) ofp()->putsNoTracking(textAndFileline.first);
}
