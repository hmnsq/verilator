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

#അന്തര്ഭാവ "V3PchAstMT.h"

#അന്തര്ഭാവ "V3EmitC.h"
#അന്തര്ഭാവ "V3EmitCFunc.h"
#അന്തര്ഭാവ "V3ThreadPool.h"
#അന്തര്ഭാവ "V3UniqueNames.h"

#അന്തര്ഭാവ <algorithm>
#അന്തര്ഭാവ <map>
#അന്തര്ഭാവ <set>
#അന്തര്ഭാവ <tuple>
#അന്തര്ഭാവ <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Internal EmitC implementation

വർഗ്ഗം EmitCImp final : പൊതു EmitCFunc {
    // MEMBERS
    // Base module (For non-classes, same as m_modp. For classes, it's the ClassPackage.)
    സ്ഥിരം Ast_കണ്ണി_Module* സ്ഥിരം m_fileModp;
    സ്ഥിരം തർക്കം m_slow;  // Creating __Slow file
    V3UniqueNames m_uniqueNames;  // Generates unique file names
    സ്ഥിരം std::string m_fileBaseName = EmitCUtil::prefixNameProtect(m_fileModp);
    TextStmts m_ctorStmts;  // Statements constructing members in 'ctor'
    TextStmts m_dtorStmts;  // Statements destroying members in 'dtor'
    std::string m_ctorSplitDecl;  // Declaration of sub-function 'ctor' was split into, if any
    std::string m_dtorSplitDecl;  // Declaration of sub-function 'dtor' was split into, if any

    // METHODS
    ശൂന്യം openNextOutputFile(സ്ഥിരം std::string& fileName) {
        openNewOutputSourceFile(fileName, m_slow, അസത്യം, "Design implementation internals");
        emitFileHeader();
    }
    ശൂന്യം emitFileHeader() {
        puts("// See " + EmitCUtil::മുകൾ_Class_Name() + ".h for the primary calling header\n");
        puts("\n");
        puts("#include \"" + EmitCUtil::pchClassName() + ".h\"\n");
        emitSystemCSection(m_fileModp, VSystemCSectionType::IMP_HDR);
        // Need to emit new lazy declarations
        m_lazyDecls.reset();
    }

    ശൂന്യം emitParamDefns(സ്ഥിരം Ast_കണ്ണി_Module* modp) {
        തർക്കം first = സത്യം;
        വേണ്ടി (സ്ഥിരം Ast_കണ്ണി* nodep = modp->stmtsp(); nodep; nodep = nodep->nextp()) {
            എങ്കിൽ (സ്ഥിരം AstVar* സ്ഥിരം varp = VN_CAST(nodep, Var)) {
                എങ്കിൽ (varp->isParam()) {
                    എങ്കിൽ (first) {
                        puts("\n");
                        സ്ഥിരം string modName = EmitCUtil::prefixNameProtect(modp);
                        putsDecoration(modp, "// Parameter definitions for " + modName + "\n");
                        first = അസത്യം;
                    }
                    എങ്കിൽ (varp->dtypep()->isLiteralType()) putns(varp, "VL_CONSTINIT_CXX20 ");
                    putns(varp, "const ");
                    emitVarDefn(varp);
                    puts(";\n");
                }
            }
        }
        എങ്കിൽ (!first) puts("\n");
    }
    // Add statements explicitly constructing, or destroying, a member AstVar
    സ്ഥിതം ശൂന്യം addVarCtorDtorStmts(TextStmts& stmts, സ്ഥിരം AstVar* varp, തർക്കം destroy) {
        സ്ഥിരം std::string name = "vlSelf->" + varName(varp);

        // SystemC port arrays are C arrays, so construct them element by element. They are
        // default constructed (unnamed), as the elements of an array member would be.
        എങ്കിൽ (varp->isSc() && VN_IS(varp->dtypeSkipRefp(), UnpackArrayDType)) {
            std::vector<std::string> elems{name};
            വേണ്ടി (സ്ഥിരം AstUnpackArrayDType* adtypep
                 = VN_CAST(varp->dtypeSkipRefp(), UnpackArrayDType);
                 adtypep; adtypep = VN_CAST(adtypep->subDTypep()->skipRefp(), UnpackArrayDType)) {
                std::vector<std::string> subElems;
                വേണ്ടി (സ്ഥിരം std::string& elem : elems) {
                    വേണ്ടി (പൂർണ്ണസംഖ്യ i = 0; i < adtypep->elementsConst(); ++i) {
                        subElems.തള്ളുക_back(elem + "[" + std::to_string(i) + "]");
                    }
                }
                elems = std::move(subElems);
            }
            എങ്കിൽ (destroy) std::reverse(elems.begin(), elems.end());
            വേണ്ടി (സ്ഥിരം std::string& e : elems) {
                stmts.add((destroy ? "VL_INPLACE_DESTRUCT(" : "VL_INPLACE_CONSTRUCT(") + e + ");");
            }
            തിരികെ;
        }

        // Other variables are constructed whole, even if aggregates
        എങ്കിൽ (destroy) {
            stmts.add("VL_INPLACE_DESTRUCT(" + name + ");");
            തിരികെ;
        }
        സ്ഥിരം AstBasicDType* സ്ഥിരം basicp = VN_CAST(varp->dtypeSkipRefp(), BasicDType);
        std::string args;
        എങ്കിൽ (varp->isSc()) {
            // SystemC ports need their name
            args = ", \"" + V3OutFormatter::quoteNameControls(varp->nameProtect()) + "\"";
        } അല്ലെങ്കിൽ എങ്കിൽ (basicp && basicp->keyword().isMTaskState()) {
            // MTask state needs the number of upstream dependencies to wait for
            args = ", " + VN_AS(varp->valuep(), Const)->അക്കം().emitC();
        } അല്ലെങ്കിൽ എങ്കിൽ (basicp && basicp->isDelayScheduler()) {
            // Delay scheduler needs the context pointer
            args = ", *vlSelf->vlSymsp->_vm_contextp__";
        }
        stmts.add("VL_INPLACE_CONSTRUCT(" + name + args + ");");
    }
    // Gather the statements constructing/destroying members in anonymous unions, and split
    // them into sub-functions if needed. As this emits files, no output file can be open.
    ശൂന്യം createCtorDtorStmts(സ്ഥിരം Ast_കണ്ണി_Module* modp) {
        // Construct member variables
        വേണ്ടി (സ്ഥിരം Ast_കണ്ണി* nodep = modp->stmtsp(); nodep; nodep = nodep->nextp()) {
            സ്ഥിരം AstVar* സ്ഥിരം varp = VN_CAST(nodep, Var);
            എങ്കിൽ (varp && isExplicitlyConstructed(varp)) {
                addVarCtorDtorStmts(m_ctorStmts, varp, അസത്യം);
            }
        }

        // Destroy in reverse order
        സ്ഥിരം Ast_കണ്ണി* സ്ഥിരം lastp = modp->stmtsp() ? modp->stmtsp()->lastp() : nullptr;
        വേണ്ടി (സ്ഥിരം Ast_കണ്ണി* nodep = lastp; nodep; nodep = nodep->prevp()) {
            സ്ഥിരം AstVar* സ്ഥിരം varp = VN_CAST(nodep, Var);
            എങ്കിൽ (varp && isExplicitlyConstructed(varp)) {
                addVarCtorDtorStmts(m_dtorStmts, varp, സത്യം);
            }
        }

        // Split into sub-functions if needed
        സ്ഥിരം std::string modName = EmitCUtil::prefixNameProtect(modp);
        std::tie(m_ctorSplitDecl, m_dtorSplitDecl) = emitSplitCtorDtor(
            m_ctorStmts, m_dtorStmts, 0, modName, modName + "* vlSelf", "vlSelf", അസത്യം,
            "Design implementation internals", [ഇത്]() { emitFileHeader(); });
    }
    ശൂന്യം emitCtorDtorImp(സ്ഥിരം Ast_കണ്ണി_Module* modp) {
        സ്ഥിരം std::string modName = EmitCUtil::prefixNameProtect(modp);

        // ctor
        എങ്കിൽ (modp->ctorVarReset()) {
            m_lazyDecls.emit("\nvoid " + modName + "__", protect("_ctor_var_reset"),
                             "(" + modName + "* vlSelf);\n");
        }

        എങ്കിൽ (!m_ctorSplitDecl.empty()) puts("\n" + m_ctorSplitDecl + "\n");
        puts("\nvoid " + modName + "__ctor(" + modName + "* vlSelf, " + EmitCUtil::symClassName()
             + "* symsp, const char* namep) {\n");
        puts("vlSelf->vlSymsp = symsp;\n");
        എങ്കിൽ (modp->is_മുകൾ()) {
            puts("vlSelf->vlNamep = strdup(namep);\n");
        } അല്ലെങ്കിൽ {
            puts("vlSelf->vlNamep = strdup(Verilated::catName(symsp->name(), namep));\n");
        }
        putsDecoration(modp, "// Construct members\n");
        വേണ്ടി (സ്ഥിരം TextStmts::Stmt& stmt : m_ctorStmts) puts(stmt.m_text + "\n");

        putsDecoration(modp, "// Reset structure values\n");
        എങ്കിൽ (modp->ctorVarReset()) {
            puts(modName + "__" + protect("_ctor_var_reset") + "(vlSelf);\n");
        }
        puts("}\n");

        // C++ constructor
        puts("\n" + modName + "::" + modName + "(" + EmitCUtil::symClassName()
             + "* symsp, const char* namep) {\n");
        puts(modName + "__ctor(this, symsp, namep);\n");
        emitSystemCSection(modp, VSystemCSectionType::CTOR);
        puts("}\n");

        // dtor
        എങ്കിൽ (!m_dtorSplitDecl.empty()) puts("\n" + m_dtorSplitDecl + "\n");
        puts("\nvoid " + modName + "__dtor(" + modName + "* vlSelf) {\n");
        putsDecoration(modp, "// Destroy members\n");
        വേണ്ടി (സ്ഥിരം TextStmts::Stmt& stmt : m_dtorStmts) puts(stmt.m_text + "\n");
        puts("VL_DO_DANGLING(std::free(const_cast<char*>(vlSelf->vlNamep)), "
             "vlSelf->vlNamep);\n");
        puts("}\n");

        // C++ destructor
        puts("\n" + modName + "::~" + modName + "() {\n");
        emitSystemCSection(modp, VSystemCSectionType::DTOR);
        puts(modName + "__dtor(this);\n");
        puts("}\n");
    }

    ശൂന്യം emitConfigureImp(സ്ഥിരം Ast_കണ്ണി_Module* modp) {
        സ്ഥിരം string modName = EmitCUtil::prefixNameProtect(modp);

        എങ്കിൽ (v3Global.opt.coverage()) {
            puts("\n");
            m_lazyDecls.emit("void " + modName + "__", protect("_configure_coverage"),
                             "(" + modName + "* vlSelf, bool first);");
        }

        എങ്കിൽ (v3Global.opt.coverage()) {
            puts("\nvoid " + modName + "::" + protect("__Vconfigure") + "(bool first) {\n");
            puts("(void)first;  // Prevent unused variable warning\n");
            puts(modName + "__" + protect("_configure_coverage") + "(this, first);\n");
            puts("}\n");
        }
    }
    ശൂന്യം emitCoverageImp() {
        // Rather than putting out VL_COVER_INSERT calls directly, we do it via this
        // function. This gets around gcc slowness constructing all of the template
        // arguments.
        എങ്കിൽ (v3Global.opt.coverage()) {
            puts("\n// Coverage\n");
            puts("void " + EmitCUtil::prefixNameProtect(m_modp) + "::__vlCoverInsert(");
            puts(v3Global.opt.threads() > 1 ? "std::atomic<uint32_t>" : "uint32_t");
            puts("* countp, bool enable, bool localCounter, const char* filenamep, int lineno, "
                 "int column,\n");
            puts("const char* hierp, const char* pagep, const char* commentp, const char* "
                 "linescovp,\n");
            puts("const char* fsmVarp, const char* fsmFromp, const char* fsmTop, const char* "
                 "fsmTagp) {\n");
            എങ്കിൽ (v3Global.opt.threads() > 1) {
                puts("assert(sizeof(uint32_t) == sizeof(std::atomic<uint32_t>));\n");
                puts("uint32_t* count32p = reinterpret_cast<uint32_t*>(countp);\n");
            } അല്ലെങ്കിൽ {
                puts("uint32_t* count32p = countp;\n");
            }
            // static doesn't need save-restore as is constant
            puts("static uint32_t fake_zero_count = 0;\n");
            puts("std::string fullhier = std::string{vlNamep} + hierp;\n");
            puts("if (!fullhier.empty() && fullhier[0] == '.') fullhier = fullhier.substr(1);\n");
            // Global-counter users still redirect later duplicate instances
            // to fake_zero_count for default collapsed coverage. Object-local
            // counters must keep the real pointer so forcePerInstance can
            // report each hierarchy independently.
            puts("if (!enable && !localCounter) count32p = &fake_zero_count;\n");
            puts("*count32p = 0;\n");
            puts("VL_COVER_INSERT(vlSymsp->_vm_contextp__->coveragep(), vlNamep, count32p,");
            puts("  \"filename\",filenamep,");
            puts("  \"lineno\",lineno,");
            puts("  \"column\",column,\n");
            puts("\"hier\",fullhier.c_str(),");
            puts("  \"page\",pagep,");
            puts("  \"comment\",commentp,");
            puts("  (linescovp[0] ? \"linescov\" : \"\"), linescovp,");
            puts("  (fsmVarp[0] ? \"fsm_var\" : \"\"), fsmVarp,");
            puts("  (fsmFromp[0] ? \"fsm_from\" : \"\"), fsmFromp,");
            puts("  (fsmTop[0] ? \"fsm_to\" : \"\"), fsmTop,");
            puts("  (fsmTagp[0] ? \"fsm_tag\" : \"\"), fsmTagp);\n");
            puts("}\n");
        }
        എങ്കിൽ (v3Global.opt.coverageToggle()) {
            puts("\n// Toggle Coverage\n");
            puts("void " + EmitCUtil::prefixNameProtect(m_modp) + "::__vlCoverToggleInsert(");
            puts("int begin, int end, bool ranged, ");
            puts(v3Global.opt.threads() > 1 ? "std::atomic<uint32_t>" : "uint32_t");
            puts("* countp, bool enable, bool localCounter, const char* filenamep, int lineno, "
                 "int column,\n");
            puts("const char* hierp, const char* pagep, const char* commentp) {\n");
            എങ്കിൽ (v3Global.opt.threads() > 1) {
                puts("assert(sizeof(uint32_t) == sizeof(std::atomic<uint32_t>));\n");
            }
            puts("int step = (end >= begin) ? 1 : -1;\n");
            // range is inclusive
            puts("for (int i = begin; i != end + step; i += step) {\n");
            puts("for (int j = 0; j < 2; j++) {\n");
            എങ്കിൽ (v3Global.opt.threads() > 1) {
                puts("uint32_t* count32p = reinterpret_cast<uint32_t*>(countp);\n");
            } അല്ലെങ്കിൽ {
                puts("uint32_t* count32p = countp;\n");
            }
            // static doesn't need save-restore as is constant
            puts("static uint32_t fake_zero_count = 0;\n");
            puts("std::string fullhier = std::string{vlNamep} + hierp;\n");
            puts("if (!fullhier.empty() && fullhier[0] == '.') fullhier = fullhier.substr(1);\n");
            puts("std::string commentWithIndex = commentp;\n");
            puts("if (ranged) commentWithIndex += '[' + std::to_string(i) + ']';\n");
            puts("commentWithIndex += j ? \":0->1\" : \":1->0\";\n");
            // Global-counter users still redirect later duplicate instances
            // to fake_zero_count for default collapsed coverage. Object-local
            // counters must keep the real pointer so forcePerInstance can
            // report each hierarchy independently.
            puts("if (!enable && !localCounter) count32p = &fake_zero_count;\n");
            puts("*count32p = 0;\n");
            puts("VL_COVER_INSERT(vlSymsp->_vm_contextp__->coveragep(), vlNamep, count32p,");
            puts("  \"filename\",filenamep,");
            puts("  \"lineno\",lineno,");
            puts("  \"column\",column,\n");
            puts("\"hier\",fullhier.c_str(),");
            puts("  \"page\",pagep,");
            puts("  \"comment\",commentWithIndex.c_str(),");
            puts("  \"\", \"\");\n");  //  linescov argument, but in toggle coverage it is always
                                       //  empty
            puts("++countp;\n");
            puts("}\n");
            puts("}\n");
            puts("}\n");
        }
    }
    ശൂന്യം emitSavableImp(സ്ഥിരം Ast_കണ്ണി_Module* modp) {
        എങ്കിൽ (v3Global.opt.savable()) {
            puts("\n// Savable\n");
            വേണ്ടി (പൂർണ്ണസംഖ്യ de = 0; de < 2; ++de) {
                സ്ഥിരം string classname = de ? "VerilatedDeserialize" : "VerilatedSerialize";
                സ്ഥിരം string funcname = de ? "__Vdeserialize" : "__Vserialize";
                സ്ഥിരം string op = de ? ">>" : "<<";
                // NOLINTNEXTLINE(performance-inefficient-string-concatenation)
                putns(modp, "void " + EmitCUtil::prefixNameProtect(modp) + "::" + protect(funcname)
                                + "(" + classname + "& os) {\n");
                // Place a computed checksum to ensure proper structure save/restore formatting
                // OK if this hash includes some things we won't dump, since
                // just looking for loading the wrong model
                VHashSha512 hash;
                വേണ്ടി (Ast_കണ്ണി* nodep = modp->stmtsp(); nodep; nodep = nodep->nextp()) {
                    എങ്കിൽ (സ്ഥിരം AstVar* സ്ഥിരം varp = VN_CAST(nodep, Var)) {
                        hash.തിരുകുക(varp->name());
                        hash.തിരുകുക(varp->dtypep()->width());
                    }
                }
                ofp()->printf("uint64_t __Vcheckval = 0x%" PRIx64 "ULL;\n",
                              static_cast<uint64_t>(hash.digestUInt64()));
                എങ്കിൽ (de) {
                    puts("os.readAssert(__Vcheckval);\n");
                } അല്ലെങ്കിൽ {
                    puts("os << __Vcheckval;\n");
                }

                // Save context
                // If multiple models save the same context we'll save it multiple
                // times, but is harmless, and doing it otherwise would break
                // backwards compatibility.
                puts("os " + op + " vlSymsp->_vm_contextp__;\n");

                // Save all members
                വേണ്ടി (Ast_കണ്ണി* nodep = modp->stmtsp(); nodep; nodep = nodep->nextp()) {
                    എങ്കിൽ (സ്ഥിരം AstVar* സ്ഥിരം varp = VN_CAST(nodep, Var)) {
                        എങ്കിൽ (varp->isIO() && modp->is_മുകൾ() && optSystemC()) {
                            // System C top I/O doesn't need loading, as the
                            // lower level subinst code does it.
                        } അല്ലെങ്കിൽ എങ്കിൽ (varp->isParam()) {
                        } അല്ലെങ്കിൽ എങ്കിൽ (VN_IS(varp->dtypep(), NBACommitQueueDType)) {
                        } അല്ലെങ്കിൽ {
                            പൂർണ്ണസംഖ്യ vects = 0;
                            Ast_കണ്ണി_DType* elementp = varp->dtypeSkipRefp();
                            വേണ്ടി (AstUnpackArrayDType* arrayp = VN_CAST(elementp, UnpackArrayDType);
                                 arrayp; arrayp = VN_CAST(elementp, UnpackArrayDType)) {
                                സ്ഥിരം പൂർണ്ണസംഖ്യ vecnum = vects++;
                                UASSERT_OBJ(arrayp->hi() >= arrayp->lo(), varp,
                                            "Should have swapped msb & lsb earlier.");
                                സ്ഥിരം string ivar = "__Vi"s + cvtToStr(vecnum);
                                puts("for (int __Vi" + cvtToStr(vecnum) + " = " + cvtToStr(0));
                                puts("; " + ivar + " < " + cvtToStr(arrayp->elementsConst()));
                                puts("; ++" + ivar + ") {\n");
                                elementp = arrayp->subDTypep()->skipRefp();
                            }
                            സ്ഥിരം AstBasicDType* സ്ഥിരം basicp = elementp->basicp();
                            // Do not save MTask state, only matters within an evaluation
                            എങ്കിൽ (basicp && basicp->keyword().isMTaskState()) തുടരുക;
                            // Want to detect types that are represented as arrays
                            // (i.e. packed types of more than 64 bits).
                            എങ്കിൽ (elementp->isWide()
                                && !(basicp && basicp->keyword() == VBasicDTypeKwd::STRING)) {
                                സ്ഥിരം പൂർണ്ണസംഖ്യ vecnum = vects++;
                                സ്ഥിരം string ivar = "__Vi"s + cvtToStr(vecnum);
                                puts("for (int __Vi" + cvtToStr(vecnum) + " = " + cvtToStr(0));
                                puts("; " + ivar + " < " + cvtToStr(elementp->width_വാക്കുകൾ()));
                                puts("; ++" + ivar + ") {\n");
                            }
                            putns(varp, "os" + op + varName(varp));
                            വേണ്ടി (പൂർണ്ണസംഖ്യ v = 0; v < vects; ++v) puts("[__Vi" + cvtToStr(v) + "]");
                            puts(";\n");
                            വേണ്ടി (പൂർണ്ണസംഖ്യ v = 0; v < vects; ++v) puts("}\n");
                        }
                    }
                }

                puts("}\n");
            }
        }
    }
    // Predicate to check if we actually need to emit anything into the common implementation file.
    // Used to avoid creating empty output files.
    തർക്കം hasCommonImp(സ്ഥിരം Ast_കണ്ണി_Module* modp) സ്ഥിരം {
        // Nothing to emit if no module!
        എങ്കിൽ (!modp) തിരികെ അസത്യം;
        // We always need the slow file
        എങ്കിൽ (m_slow) തിരികെ സത്യം;
        // The fast file is only required when we have `systemc_implementation nodes
        വേണ്ടി (സ്ഥിരം AstSystemCSection* scSectionp = modp->scSectionsp(); scSectionp;
             scSectionp = VN_AS(scSectionp->nextp(), SystemCSection)) {
            എങ്കിൽ (scSectionp->sectionType() == VSystemCSectionType::IMP) തിരികെ സത്യം;
        }
        തിരികെ അസത്യം;
    }
    // Actually emit common implementation contents for given AstNodeModule
    ശൂന്യം doCommonImp(സ്ഥിരം Ast_കണ്ണി_Module* modp) {
        എങ്കിൽ (m_slow) {
            എങ്കിൽ (!VN_IS(modp, Class)) {
                emitParamDefns(modp);
                emitCtorDtorImp(modp);
                emitConfigureImp(modp);
                emitCoverageImp();
            }
            emitSavableImp(modp);
        } അല്ലെങ്കിൽ {
            // From `systemc_implementation
            emitSystemCSection(modp, VSystemCSectionType::IMP);
        }
    }
    ശൂന്യം emitCommonImp(സ്ഥിരം Ast_കണ്ണി_Module* modp) {
        സ്ഥിരം AstClass* സ്ഥിരം classp
            = VN_IS(modp, ClassPackage) ? VN_AS(modp, ClassPackage)->classp() : nullptr;

        എങ്കിൽ (hasCommonImp(modp) || hasCommonImp(classp)) {
            // Create ctor/dtor statements. If these need splitting, they will be emitted
            // during construction so do this before opening the output file.
            എങ്കിൽ (m_slow && !VN_IS(modp, Class)) createCtorDtorStmts(modp);

            openNextOutputFile(m_fileBaseName);

            doCommonImp(modp);
            എങ്കിൽ (classp) {
                VL_RESTORER(m_modp);
                m_modp = classp;
                doCommonImp(classp);
            }

            closeOutputFile();
        }
    }
    ശൂന്യം emitCFuncImp(സ്ഥിരം Ast_കണ്ണി_Module* modp) {
        // Functions to be emitted here
        std::vector<AstCFunc*> funcps;

        സ്ഥിരം auto gather = [ഇത്, &funcps](സ്ഥിരം Ast_കണ്ണി_Module* modp) {
            വേണ്ടി (Ast_കണ്ണി* nodep = modp->stmtsp(); nodep; nodep = nodep->nextp()) {
                AstCFunc* സ്ഥിരം funcp = VN_CAST(nodep, CFunc);
                എങ്കിൽ (!funcp) തുടരുക;
                // TRACE_* and DPI handled elsewhere
                എങ്കിൽ (funcp->isTrace()) തുടരുക;
                എങ്കിൽ (funcp->dpiImportPrototype()) തുടരുക;
                എങ്കിൽ (funcp->dpiExportDispatcher()) തുടരുക;
                എങ്കിൽ (funcp->slow() != m_slow) തുടരുക;
                funcps.തള്ളുക_back(funcp);
            }
        };

        gather(modp);
        VL_RESTORER(m_classOrPackage);
        എങ്കിൽ (സ്ഥിരം AstClassPackage* സ്ഥിരം packagep = VN_CAST(modp, ClassPackage)) {
            m_classOrPackage = packagep;
            gather(packagep->classp());
        }

        // Do not create empty files
        എങ്കിൽ (funcps.empty()) തിരികെ;

        // Open output file
        openNextOutputFile(m_uniqueNames.get(m_fileBaseName));
        // Emit all functions
        വേണ്ടി (AstCFunc* സ്ഥിരം funcp : funcps) {
            VL_RESTORER(m_modp);
            m_modp = EmitCParentModule::get(funcp);
            iterateConst(funcp);
        }
        // Close output file
        closeOutputFile();
    }

    // VISITORS
    ശൂന്യം visit(AstCFunc* nodep) override {
        എങ്കിൽ (splitNeeded()) {
            // Splitting file, so using parallel build.
            v3Global.useParallelBuild(സത്യം);
            // Close old file
            closeOutputFile();
            // Open a new file
            openNextOutputFile(m_uniqueNames.get(m_fileBaseName));
        }

        EmitCFunc::visit(nodep);
    }

    explicit EmitCImp(സ്ഥിരം Ast_കണ്ണി_Module* modp, തർക്കം slow)
        : m_fileModp{modp}
        , m_slow{slow} {
        UINFO(5, "  Emitting implementation of " << EmitCUtil::prefixNameProtect(modp));

        m_modp = modp;

        // Emit implementation of this module, if this is an AstClassPackage, then put the
        // corresponding AstClass implementation in the same file as often optimizations are
        // possible when both are seen by the compiler
        // TODO: is the above comment still true?

        // Emit implementations of common parts
        emitCommonImp(modp);

        // Emit implementations of all AstCFunc
        emitCFuncImp(modp);
    }
    ~EmitCImp() override = സ്ഥിരസ്ഥിതി;

പൊതു:
    സ്ഥിതം std::vector<AstCFile*> പ്രധാന(സ്ഥിരം Ast_കണ്ണി_Module* modp, തർക്കം slow) VL_MT_STABLE {
        EmitCImp emitCImp{modp, slow};
        തിരികെ emitCImp.getAndClearCfileps();
    }
};

//######################################################################
// Tracing routines

// Trace type descriptors go in a different file as it needs to be written in
// parallel with the actual trace function source files
വർഗ്ഗം EmitCTraceTypes final : പൊതു EmitCFunc {
    // NODE STATE/TYPES
    // None allowed to support threaded emitting

    // STATE
    പൂർണ്ണസംഖ്യ m_enum_അക്കം = 0;  // Enumeration number (whole netlist)
    std::unordered_map<Ast_കണ്ണി*, പൂർണ്ണസംഖ്യ> m_enum_അക്കം_Map;  // EnumDType to enumeration number
    പൂർണ്ണസംഖ്യ m_traceTypeSubs = 0;  // Number of trace type declaration sub-functions
    V3UniqueNames m_uniqueNames;  // Generates unique file names
    സ്ഥിരം std::string m_fileBaseName = EmitCUtil::മുകൾ_Class_Name() + "_" + protect("_TraceDecls");
    // This one uses CSplitTrace for file splitting, which is incorrect but historically accurates
    സ്ഥിരം size_t m_splitLimit = v3Global.opt.outputSplitCTrace()
                                    ? static_cast<size_t>(v3Global.opt.outputSplitCTrace())
                                    : std::numeric_limits<size_t>::max();

    ശൂന്യം openNextOutputFile() {
        openNewOutputSourceFile(m_uniqueNames.get(m_fileBaseName), സത്യം, സത്യം,
                                "Tracing declarations");
        puts("\n");
        വേണ്ടി (സ്ഥിരം std::string& base : v3Global.opt.traceSourceLangs()) {
            puts("#include \"" + base + ".h\"\n");
        }
        puts("\n");
        puts("\nvoid " + EmitCUtil::prefixNameProtect(m_modp) + "__"
             + protect("traceDeclTypesSub" + std::to_string(m_traceTypeSubs++)) + "("
             + v3Global.opt.traceClassBase() + "* tracep) {\n");
    }

പൊതു:
    // METHODS
    പൂർണ്ണസംഖ്യ get_Enum_Map_അക്കം(AstEnumDType* nodep) {
        പൂർണ്ണസംഖ്യ& enumNumr = m_enum_അക്കം_Map[nodep];
        എങ്കിൽ (!enumNumr) {
            എങ്കിൽ (splitNeeded(m_splitLimit)) {
                // Splitting file, so using parallel build.
                v3Global.useParallelBuild(സത്യം);
                puts("}\n");
                closeOutputFile();
                openNextOutputFile();
            }

            enumNumr = ++m_enum_അക്കം;
            പൂർണ്ണസംഖ്യ nvals = 0;
            puts("{\n");
            putns(nodep, "const char* " + protect("__VenumItemNames") + "[]\n");
            puts("= {");
            വേണ്ടി (AstEnumItem* itemp = nodep->itemsp(); itemp;
                 itemp = VN_AS(itemp->nextp(), EnumItem)) {
                എങ്കിൽ (++nvals > 1) puts(", ");
                putbs("\"" + itemp->prettyName() + "\"");
            }
            puts("};\n");
            nvals = 0;
            puts("const char* " + protect("__VenumItemValues") + "[]\n");
            puts("= {");
            വേണ്ടി (AstEnumItem* itemp = nodep->itemsp(); itemp;
                 itemp = VN_AS(itemp->nextp(), EnumItem)) {
                AstConst* സ്ഥിരം constp = VN_AS(itemp->valuep(), Const);
                എങ്കിൽ (++nvals > 1) puts(", ");
                putbs("\"" + constp->അക്കം().displayed(nodep, "%0b") + "\"");
            }
            puts("};\n");
            puts("tracep->declDTypeEnum(" + std::to_string(enumNumr) + ", \"" + nodep->prettyName()
                 + "\", " + std::to_string(nvals) + ", " + std::to_string(nodep->widthMin()) + ", "
                 + protect("__VenumItemNames") + ", " + protect("__VenumItemValues") + ");\n");
            puts("}\n");
            splitSizeInc(Ast_കണ്ണി::INSTR_എണ്ണം_CALL);
        }
        തിരികെ enumNumr;
    }

    // Close output file
    ശൂന്യം finalize() {
        // Close function definition
        puts("}\n");

        സ്ഥിരം std::string modName = EmitCUtil::prefixNameProtect(m_modp);
        സ്ഥിരം std::string args = v3Global.opt.traceClassBase() + "* tracep";

        // Forward declarations for subs in other files
        വേണ്ടി (പൂർണ്ണസംഖ്യ i = 0; i < m_traceTypeSubs - 1; ++i) {
            puts("void " + modName + "__" + protect("traceDeclTypesSub" + std::to_string(i)) + "("
                 + args + ");\n");
        }

        // Create top level trace_decl_types function and call each sub-function
        puts("\nvoid " + modName + "__" + protect("trace_decl_types") + "(" + args + ") {\n");
        വേണ്ടി (പൂർണ്ണസംഖ്യ i = 0; i < m_traceTypeSubs; ++i) {
            puts(modName + "__" + protect("traceDeclTypesSub" + std::to_string(i))
                 + "(tracep);\n");
        }
        puts("}\n");

        closeOutputFile();
    }

    EmitCTraceTypes() {
        m_modp = v3Global.rootp()->മുകൾ_Modulep();
        openNextOutputFile();
    }
    ~EmitCTraceTypes() override = സ്ഥിരസ്ഥിതി;
};

വർഗ്ഗം EmitCTrace final : പൊതു EmitCFunc {
    // NODE STATE/TYPES
    // None allowed to support threaded emitting

    // MEMBERS
    സ്ഥിരം തർക്കം m_slow;  // Making slow file
    സ്ഥിരം std::unique_ptr<EmitCTraceTypes> m_emitTypesp{m_slow ? പുതിയ EmitCTraceTypes{} : nullptr};
    V3UniqueNames m_uniqueNames;  // Generates unique file names
    സ്ഥിരം std::string m_fileBaseName = EmitCUtil::മുകൾ_Class_Name() + "_" + protect("_Trace");

    // METHODS
    ശൂന്യം openNextOutputFile() {
        openNewOutputSourceFile(m_uniqueNames.get(m_fileBaseName), m_slow, സത്യം,
                                "Tracing implementation internals");
        puts("\n");
        വേണ്ടി (സ്ഥിരം std::string& base : v3Global.opt.traceSourceLangs()) {
            puts("#include \"" + base + ".h\"\n");
        }
        puts("#include \"" + EmitCUtil::symClassName() + ".h\"\n");
        puts("\n");
        // Need to emit new lazy declarations
        m_lazyDecls.reset();
    }

    തർക്കം emitTraceIsScBv(സ്ഥിരം AstTraceInc* nodep) {
        സ്ഥിരം AstVarRef* സ്ഥിരം varrefp = VN_CAST(nodep->declp()->valuep(), VarRef);
        എങ്കിൽ (!varrefp) തിരികെ അസത്യം;
        സ്ഥിരം AstVar* സ്ഥിരം varp = varrefp->varp();
        തിരികെ varp->isSc() && varp->isScBv();
    }

    തർക്കം emitTraceIsScBigUint(സ്ഥിരം AstTraceInc* nodep) {
        സ്ഥിരം AstVarRef* സ്ഥിരം varrefp = VN_CAST(nodep->declp()->valuep(), VarRef);
        എങ്കിൽ (!varrefp) തിരികെ അസത്യം;
        സ്ഥിരം AstVar* സ്ഥിരം varp = varrefp->varp();
        തിരികെ varp->isSc() && varp->isScBigUint();
    }

    തർക്കം emitTraceIsScUint(സ്ഥിരം AstTraceInc* nodep) {
        സ്ഥിരം AstVarRef* സ്ഥിരം varrefp = VN_CAST(nodep->declp()->valuep(), VarRef);
        എങ്കിൽ (!varrefp) തിരികെ അസത്യം;
        സ്ഥിരം AstVar* സ്ഥിരം varp = varrefp->varp();
        തിരികെ varp->isSc() && (varp->isScUint() || varp->isScUintBool());
    }

    ശൂന്യം emitTraceInitOne(സ്ഥിരം AstTraceDecl* nodep, പൂർണ്ണസംഖ്യ enum_അക്കം) {
        std::string direction;
        direction = nodep->declDirection().traceSigDirection();

        AstCCall* സ്ഥിരം callp = nodep->dtypeCallp();
        എങ്കിൽ (callp) {
            callp->argTypes(callp->argTypes() + ", " + cvtToStr(nodep->fidx()) + ", c+"
                            + cvtToStr(nodep->code()) + ", " + direction);
            തിരികെ;
        }

        എങ്കിൽ (nodep->dtypep()->basicp()->isDouble()) {
            puts("VL_TRACE_DECL_DOUBLE");
        } അല്ലെങ്കിൽ എങ്കിൽ (nodep->isWide()) {
            puts("VL_TRACE_DECL_WIDE");
        } അല്ലെങ്കിൽ എങ്കിൽ (nodep->isQuad()) {
            puts("VL_TRACE_DECL_QUAD");
        } അല്ലെങ്കിൽ എങ്കിൽ (nodep->bitRange().ranged()) {
            puts("VL_TRACE_DECL_BUS");
        } അല്ലെങ്കിൽ എങ്കിൽ (nodep->dtypep()->basicp()->isEvent()) {
            puts("VL_TRACE_DECL_EVENT");
        } അല്ലെങ്കിൽ {
            puts("VL_TRACE_DECL_BIT");
        }

        എങ്കിൽ (nodep->arrayRange().ranged()) {
            puts("_ARRAY(tracep");
        } അല്ലെങ്കിൽ {
            puts("(tracep");
        }

        // Code
        puts(",c+" + cvtToStr(nodep->code()));
        എങ്കിൽ (nodep->arrayRange().ranged()) puts("+i*" + cvtToStr(nodep->width_വാക്കുകൾ()));

        // Function index
        puts(",");
        എങ്കിൽ (nodep->inDtypeFunc()) {
            puts("fidx");
        } അല്ലെങ്കിൽ {
            puts(cvtToStr(nodep->fidx()));
        }

        // Name
        puts(",");
        putsQuoted(VIdProtect::protect_വാക്കുകൾ_If(nodep->showname(), nodep->protect()));

        // Enum number
        puts("," + cvtToStr(enum_അക്കം));

        // Direction
        എങ്കിൽ (nodep->inDtypeFunc()) {
            puts(", direction");
        } അല്ലെങ്കിൽ {
            puts(", " + direction);
        }

        // Kind
        puts(", VerilatedTraceSigKind::");
        puts(nodep->varType().traceSigKind());

        // Type
        puts(", VerilatedTraceSigType::");
        UASSERT_OBJ(nodep->dtypep()->basicp()->keyword() != VBasicDTypeKwd::UNKNOWN, nodep,
                    "DType is unknown");
        puts(nodep->dtypep()->basicp()->keyword().traceSigType());

        // Array range
        എങ്കിൽ (nodep->arrayRange().ranged()) {
            എങ്കിൽ (nodep->arrayRange().ascending()) {
                puts(", (i + " + std::to_string(nodep->arrayRange().lo()) + ")");
            } അല്ലെങ്കിൽ {
                puts(", (" + std::to_string(nodep->arrayRange().hi()) + " - i)");
            }
        }

        // Bit range
        എങ്കിൽ (!nodep->dtypep()->basicp()->isDouble() && nodep->bitRange().ranged()) {
            puts(", " + cvtToStr(nodep->bitRange().ഇടത്()) + ","
                 + cvtToStr(nodep->bitRange().വലത്()));
        }

        //
        puts(");");
    }

    പൂർണ്ണസംഖ്യ emitTraceDeclDType(Ast_കണ്ണി_DType* nodep) {
        // Return enum number or -1 for none
        എങ്കിൽ (v3Global.opt.traceEnabledFst()) {
            // Skip over refs-to-refs, but stop before final ref so can get data type name
            // Alternatively back in V3Width we could push enum names from upper typedefs
            എങ്കിൽ (AstEnumDType* സ്ഥിരം enump = VN_CAST(nodep->skipRefToEnump(), EnumDType)) {
                തിരികെ m_emitTypesp->get_Enum_Map_അക്കം(enump);
            }
        }
        തിരികെ -1;
    }

    ശൂന്യം emitTraceChangeOne(AstTraceInc* nodep, പൂർണ്ണസംഖ്യ arrayindex) {
        // Note: Both VTraceType::CHANGE and VTraceType::FULL use the 'full' methods
        സ്ഥിരം std::string func = nodep->traceType() == VTraceType::CHANGE ? "chg" : "full";
        തർക്കം emitWidth = സത്യം;
        string stype;
        എങ്കിൽ (nodep->dtypep()->basicp()->isDouble()) {
            stype = "Double";
            emitWidth = അസത്യം;
        } അല്ലെങ്കിൽ എങ്കിൽ (nodep->isWide() || emitTraceIsScBv(nodep) || emitTraceIsScBigUint(nodep)) {
            stype = "WData";
        } അല്ലെങ്കിൽ എങ്കിൽ (nodep->isQuad()) {
            stype = "QData";
        } അല്ലെങ്കിൽ എങ്കിൽ (nodep->declp()->widthMin() > 16) {
            stype = "IData";
        } അല്ലെങ്കിൽ എങ്കിൽ (nodep->declp()->widthMin() > 8) {
            stype = "SData";
        } അല്ലെങ്കിൽ എങ്കിൽ (nodep->declp()->widthMin() > 1) {
            stype = "CData";
        } അല്ലെങ്കിൽ എങ്കിൽ (nodep->dtypep()->basicp()->isEvent()) {
            stype = "Event";
            emitWidth = അസത്യം;
        } അല്ലെങ്കിൽ {
            stype = "Bit";
            emitWidth = അസത്യം;
        }
        putns(nodep, "bufp->" + func + stype);

        സ്ഥിരം uint32_t offset = (arrayindex < 0) ? 0 : (arrayindex * nodep->declp()->width_വാക്കുകൾ());
        സ്ഥിരം uint32_t code = nodep->declp()->code() + offset;
        // Note: Both VTraceType::CHANGE and VTraceType::FULL use the 'full' methods
        puts("(oldp+");
        puts(cvtToStr(code - nodep->baseCode()));
        puts(",");
        സ്ഥിരം VNumRange& arrayRange = nodep->declp()->arrayRange();
        എങ്കിൽ (arrayRange.ranged() && !arrayRange.ascending()) {
            arrayindex = arrayRange.elements() - 1 - arrayindex;
        }
        emit_Trace_മൂല്യം(nodep, arrayindex);
        എങ്കിൽ (emitWidth) puts("," + cvtToStr(nodep->declp()->widthMin()));
        puts(");\n");
    }

    ശൂന്യം emit_Trace_മൂല്യം(സ്ഥിരം AstTraceInc* nodep, പൂർണ്ണസംഖ്യ arrayindex) {
        എങ്കിൽ (AstVarRef* സ്ഥിരം varrefp = VN_CAST(nodep->valuep(), VarRef)) {
            സ്ഥിരം AstVar* സ്ഥിരം varp = varrefp->varp();
            എങ്കിൽ (varp->isEvent()) puts("&");
            puts("(");
            എങ്കിൽ (emitTraceIsScBigUint(nodep)) {
                puts("(uint32_t*)");
            } അല്ലെങ്കിൽ എങ്കിൽ (emitTraceIsScBv(nodep)) {
                puts("VL_SC_BV_DATAP(");
            }
            iterateConst(varrefp);  // Put var name out
            emitTraceIndex(nodep, arrayindex);
            എങ്കിൽ (varp->isSc()) puts(".read()");
            എങ്കിൽ (emitTraceIsScUint(nodep)) {
                puts(nodep->isQuad() ? ".to_uint64()" : ".to_uint()");
            } അല്ലെങ്കിൽ എങ്കിൽ (emitTraceIsScBigUint(nodep)) {
                puts(".get_raw()");
            } അല്ലെങ്കിൽ എങ്കിൽ (emitTraceIsScBv(nodep)) {
                puts(")");
            }
            puts(")");
        } അല്ലെങ്കിൽ {
            puts("(");
            iterateConst(nodep->valuep());
            emitTraceIndex(nodep, arrayindex);
            puts(")");
        }
    }

    ശൂന്യം emitTraceIndex(സ്ഥിരം AstTraceInc* സ്ഥിരം nodep, പൂർണ്ണസംഖ്യ arrayindex) {
        // Tracing only supports 1D arrays
        എങ്കിൽ (nodep->declp()->arrayRange().ranged()) {
            എങ്കിൽ (arrayindex == -2) {
                puts("[i]");
            } അല്ലെങ്കിൽ എങ്കിൽ (arrayindex == -1) {
                puts("[0]");
            } അല്ലെങ്കിൽ {
                puts("[" + cvtToStr(arrayindex) + "]");
            }
        }
    }

    // VISITORS
    using EmitCFunc::visit;  // Suppress hidden overloaded virtual function warning
    ശൂന്യം visit(AstCFunc* nodep) override {
        എങ്കിൽ (!nodep->isTrace()) തിരികെ;
        എങ്കിൽ (nodep->slow() != m_slow) തിരികെ;

        എങ്കിൽ (splitNeeded()) {
            // Splitting file, so using parallel build.
            v3Global.useParallelBuild(സത്യം);
            // Close old file
            closeOutputFile();
            // Open a new file
            openNextOutputFile();
        }

        EmitCFunc::visit(nodep);
    }
    ശൂന്യം visit(Ast_Trace_തള്ളുക_Prefix* nodep) override {
        putns(nodep, "VL_TRACE_PUSH_PREFIX(tracep, ");
        എങ്കിൽ (nodep->quotedPrefix()) {
            putsQuoted(VIdProtect::protect_വാക്കുകൾ_If(nodep->prefix(), nodep->protect()));
        } അല്ലെങ്കിൽ {
            puts(nodep->prefix());
        }
        puts(", VerilatedTracePrefixType::");
        puts(nodep->prefixType().ascii());
        puts(", " + std::to_string(nodep->ഇടത്()));
        puts(", " + std::to_string(nodep->വലത്()));
        puts(");\n");
    }
    ശൂന്യം visit(Ast_Trace_എടുക്കുക_Prefix* nodep) override {  //
        putns(nodep, "VL_TRACE_POP_PREFIX(tracep);\n");
    }
    ശൂന്യം visit(AstTraceDecl* nodep) override {
        സ്ഥിരം പൂർണ്ണസംഖ്യ enum_അക്കം = emitTraceDeclDType(nodep->dtypep());
        putns(nodep, "");
        എങ്കിൽ (nodep->arrayRange().ranged()) {
            puts("for (int i = 0; i < " + cvtToStr(nodep->arrayRange().elements()) + "; ++i) {\n");
            emitTraceInitOne(nodep, enum_അക്കം);
            puts("\n}\n");
        } അല്ലെങ്കിൽ {
            emitTraceInitOne(nodep, enum_അക്കം);
            puts("\n");
        }
    }
    ശൂന്യം visit(AstTraceInc* nodep) override {
        എങ്കിൽ (nodep->declp()->arrayRange().ranged()) {
            // It traces faster if we unroll the loop
            വേണ്ടി (പൂർണ്ണസംഖ്യ i = 0; i < nodep->declp()->arrayRange().elements(); i++) {
                emitTraceChangeOne(nodep, i);
            }
        } അല്ലെങ്കിൽ {
            emitTraceChangeOne(nodep, -1);
        }
    }

    explicit EmitCTrace(തർക്കം slow)
        : m_slow{slow} {
        m_modp = v3Global.rootp()->മുകൾ_Modulep();
        // Open output file
        openNextOutputFile();
        // Emit functions
        വേണ്ടി (Ast_കണ്ണി* nodep = m_modp->stmtsp(); nodep; nodep = nodep->nextp()) {
            എങ്കിൽ (AstCFunc* സ്ഥിരം funcp = VN_CAST(nodep, CFunc)) iterateConst(funcp);
        }
        // Close output file
        closeOutputFile();
        എങ്കിൽ (m_slow) m_emitTypesp->finalize();
    }
    ~EmitCTrace() override = സ്ഥിരസ്ഥിതി;

പൊതു:
    സ്ഥിതം std::vector<AstCFile*> പ്രധാന(തർക്കം slow) VL_MT_STABLE {
        EmitCTrace emitCTrace{slow};
        std::vector<AstCFile*> cfileps = emitCTrace.getAndClearCfileps();
        എങ്കിൽ (slow) {
            വേണ്ടി (AstCFile* സ്ഥിരം cfilep : emitCTrace.m_emitTypesp->getAndClearCfileps()) {
                cfileps.emplace_back(cfilep);
            }
        }
        തിരികെ cfileps;
    }
};

//######################################################################
// Existing AstCFile emitter

വർഗ്ഗം EmitCFile final : പൊതു EmitCFunc {
    explicit EmitCFile(AstCFile* cfilep) {
        openOutputFile(cfilep, "Generated C++");
        iterateConst(cfilep->tblockp());
        closeOutputFile();
    }
    ~EmitCFile() override = സ്ഥിരസ്ഥിതി;

പൊതു:
    സ്ഥിതം ശൂന്യം പ്രധാന(AstCFile* cfilep) VL_MT_STABLE {
        എങ്കിൽ (!cfilep->tblockp()) തിരികെ;
        EmitCFile{cfilep};
    }
};

//######################################################################
// EmitC class functions

ശൂന്യം V3EmitC::emitcImp() {
    UINFO(2, __FUNCTION__ << ":");
    std::list<std::vector<AstCFile*>> cfiles;
    {
        // Make parent module pointers available.
        സ്ഥിരം EmitCParentModule emitCParentModule;
        V3ThreadScope threadScope;

        // Process each module in turn
        വേണ്ടി (സ്ഥിരം Ast_കണ്ണി* nodep = v3Global.rootp()->modulesp(); nodep; nodep = nodep->nextp()) {
            എങ്കിൽ (VN_IS(nodep, Class)) തുടരുക;  // Imped with ClassPackage
            സ്ഥിരം Ast_കണ്ണി_Module* സ്ഥിരം modp = VN_AS(nodep, കണ്ണി_Module);
            എങ്കിൽ (modp->isConstPool()) തുടരുക;  // Emitted by V3EmitCConstPool
            cfiles.emplace_back();
            std::vector<AstCFile*>& slow = cfiles.back();
            threadScope.enqueue([modp, &slow] { slow = EmitCImp::പ്രധാന(modp, /* slow: */ സത്യം); });
            cfiles.emplace_back();
            std::vector<AstCFile*>& fast = cfiles.back();
            threadScope.enqueue([modp, &fast] { fast = EmitCImp::പ്രധാന(modp, /* slow: */ അസത്യം); });
        }

        // Emit trace routines (currently they can only exist in the top module)
        എങ്കിൽ (v3Global.opt.trace() && !v3Global.opt.lintOnly()) {
            cfiles.emplace_back();
            std::vector<AstCFile*>& slow = cfiles.back();
            threadScope.enqueue([&slow] { slow = EmitCTrace::പ്രധാന(/* slow: */ സത്യം); });
            cfiles.emplace_back();
            std::vector<AstCFile*>& fast = cfiles.back();
            threadScope.enqueue([&fast] { fast = EmitCTrace::പ്രധാന(/* slow: */ അസത്യം); });
        }
    }
    // Add files to netlist
    വേണ്ടി (സ്ഥിരം std::vector<AstCFile*>& cfileps : cfiles) {
        വേണ്ടി (AstCFile* സ്ഥിരം cfilep : cfileps) v3Global.rootp()->addFilesp(cfilep);
    }
}

ശൂന്യം V3EmitC::emitcFiles() {
    UINFO(2, __FUNCTION__ << ":");
    വേണ്ടി (Ast_കണ്ണി_File *filep = v3Global.rootp()->filesp(), *nextp; filep; filep = nextp) {
        nextp = VN_AS(filep->nextp(), കണ്ണി_File);
        എങ്കിൽ (AstCFile* സ്ഥിരം cfilep = VN_CAST(filep, CFile)) EmitCFile::പ്രധാന(cfilep);
    }
}
