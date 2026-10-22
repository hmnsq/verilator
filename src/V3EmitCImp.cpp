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

#ଅନ୍ତର୍ଭାଵ "V3PchAstMT.h"

#ଅନ୍ତର୍ଭାଵ "V3EmitC.h"
#ଅନ୍ତର୍ଭାଵ "V3EmitCFunc.h"
#ଅନ୍ତର୍ଭାଵ "V3ThreadPool.h"
#ଅନ୍ତର୍ଭାଵ "V3UniqueNames.h"

#ଅନ୍ତର୍ଭାଵ <algorithm>
#ଅନ୍ତର୍ଭାଵ <map>
#ଅନ୍ତର୍ଭାଵ <set>
#ଅନ୍ତର୍ଭାଵ <tuple>
#ଅନ୍ତର୍ଭାଵ <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Internal EmitC implementation

ଶ୍ରେଣୀ EmitCImp final : ସର୍ବସାଧାରଣ EmitCFunc {
    // MEMBERS
    // Base module (For non-classes, same as m_modp. For classes, it's the ClassPackage.)
    ଅଚଳ Ast_ଗଣ୍ଠି_Module* ଅଚଳ m_fileModp;
    ଅଚଳ ତାର୍କିକ m_slow;  // Creating __Slow file
    V3UniqueNames m_uniqueNames;  // Generates unique file names
    ଅଚଳ std::string m_fileBaseName = EmitCUtil::prefixNameProtect(m_fileModp);
    TextStmts m_ctorStmts;  // Statements constructing members in 'ctor'
    TextStmts m_dtorStmts;  // Statements destroying members in 'dtor'
    std::string m_ctorSplitDecl;  // Declaration of sub-function 'ctor' was split into, if any
    std::string m_dtorSplitDecl;  // Declaration of sub-function 'dtor' was split into, if any

    // METHODS
    ଶୂନ୍ୟ openNextOutputFile(ଅଚଳ std::string& fileName) {
        openNewOutputSourceFile(fileName, m_slow, ମିଥ୍ୟା, "Design implementation internals");
        emitFileHeader();
    }
    ଶୂନ୍ୟ emitFileHeader() {
        puts("// See " + EmitCUtil::ଶୀର୍ଷ_Class_Name() + ".h for the primary calling header\n");
        puts("\n");
        puts("#include \"" + EmitCUtil::pchClassName() + ".h\"\n");
        emitSystemCSection(m_fileModp, VSystemCSectionType::IMP_HDR);
        // Need to emit new lazy declarations
        m_lazyDecls.reset();
    }

    ଶୂନ୍ୟ emitParamDefns(ଅଚଳ Ast_ଗଣ୍ଠି_Module* modp) {
        ତାର୍କିକ first = ସତ୍ୟ;
        ପାଇଁ (ଅଚଳ Ast_ଗଣ୍ଠି* nodep = modp->stmtsp(); nodep; nodep = nodep->nextp()) {
            ଯଦି (ଅଚଳ AstVar* ଅଚଳ varp = VN_CAST(nodep, Var)) {
                ଯଦି (varp->isParam()) {
                    ଯଦି (first) {
                        puts("\n");
                        ଅଚଳ string modName = EmitCUtil::prefixNameProtect(modp);
                        putsDecoration(modp, "// Parameter definitions for " + modName + "\n");
                        first = ମିଥ୍ୟା;
                    }
                    ଯଦି (varp->dtypep()->isLiteralType()) putns(varp, "VL_CONSTINIT_CXX20 ");
                    putns(varp, "const ");
                    emitVarDefn(varp);
                    puts(";\n");
                }
            }
        }
        ଯଦି (!first) puts("\n");
    }
    // Add statements explicitly constructing, or destroying, a member AstVar
    ସ୍ଥିର ଶୂନ୍ୟ addVarCtorDtorStmts(TextStmts& stmts, ଅଚଳ AstVar* varp, ତାର୍କିକ destroy) {
        ଅଚଳ std::string name = "vlSelf->" + varName(varp);

        // SystemC port arrays are C arrays, so construct them element by element. They are
        // default constructed (unnamed), as the elements of an array member would be.
        ଯଦି (varp->isSc() && VN_IS(varp->dtypeSkipRefp(), UnpackArrayDType)) {
            std::vector<std::string> elems{name};
            ପାଇଁ (ଅଚଳ AstUnpackArrayDType* adtypep
                 = VN_CAST(varp->dtypeSkipRefp(), UnpackArrayDType);
                 adtypep; adtypep = VN_CAST(adtypep->subDTypep()->skipRefp(), UnpackArrayDType)) {
                std::vector<std::string> subElems;
                ପାଇଁ (ଅଚଳ std::string& elem : elems) {
                    ପାଇଁ (ପୂର୍ଣ୍ଣସଂଖ୍ୟା i = 0; i < adtypep->elementsConst(); ++i) {
                        subElems.ଠେଲ_back(elem + "[" + std::to_string(i) + "]");
                    }
                }
                elems = std::move(subElems);
            }
            ଯଦି (destroy) std::reverse(elems.begin(), elems.end());
            ପାଇଁ (ଅଚଳ std::string& e : elems) {
                stmts.add((destroy ? "VL_INPLACE_DESTRUCT(" : "VL_INPLACE_CONSTRUCT(") + e + ");");
            }
            ଫେରନ୍ତୁ;
        }

        // Other variables are constructed whole, even if aggregates
        ଯଦି (destroy) {
            stmts.add("VL_INPLACE_DESTRUCT(" + name + ");");
            ଫେରନ୍ତୁ;
        }
        ଅଚଳ AstBasicDType* ଅଚଳ basicp = VN_CAST(varp->dtypeSkipRefp(), BasicDType);
        std::string args;
        ଯଦି (varp->isSc()) {
            // SystemC ports need their name
            args = ", \"" + V3OutFormatter::quoteNameControls(varp->nameProtect()) + "\"";
        } ଅନ୍ୟଥା ଯଦି (basicp && basicp->keyword().isMTaskState()) {
            // MTask state needs the number of upstream dependencies to wait for
            args = ", " + VN_AS(varp->valuep(), Const)->ଅଙ୍କ().emitC();
        } ଅନ୍ୟଥା ଯଦି (basicp && basicp->isDelayScheduler()) {
            // Delay scheduler needs the context pointer
            args = ", *vlSelf->vlSymsp->_vm_contextp__";
        }
        stmts.add("VL_INPLACE_CONSTRUCT(" + name + args + ");");
    }
    // Gather the statements constructing/destroying members in anonymous unions, and split
    // them into sub-functions if needed. As this emits files, no output file can be open.
    ଶୂନ୍ୟ createCtorDtorStmts(ଅଚଳ Ast_ଗଣ୍ଠି_Module* modp) {
        // Construct member variables
        ପାଇଁ (ଅଚଳ Ast_ଗଣ୍ଠି* nodep = modp->stmtsp(); nodep; nodep = nodep->nextp()) {
            ଅଚଳ AstVar* ଅଚଳ varp = VN_CAST(nodep, Var);
            ଯଦି (varp && isExplicitlyConstructed(varp)) {
                addVarCtorDtorStmts(m_ctorStmts, varp, ମିଥ୍ୟା);
            }
        }

        // Destroy in reverse order
        ଅଚଳ Ast_ଗଣ୍ଠି* ଅଚଳ lastp = modp->stmtsp() ? modp->stmtsp()->lastp() : nullptr;
        ପାଇଁ (ଅଚଳ Ast_ଗଣ୍ଠି* nodep = lastp; nodep; nodep = nodep->prevp()) {
            ଅଚଳ AstVar* ଅଚଳ varp = VN_CAST(nodep, Var);
            ଯଦି (varp && isExplicitlyConstructed(varp)) {
                addVarCtorDtorStmts(m_dtorStmts, varp, ସତ୍ୟ);
            }
        }

        // Split into sub-functions if needed
        ଅଚଳ std::string modName = EmitCUtil::prefixNameProtect(modp);
        std::tie(m_ctorSplitDecl, m_dtorSplitDecl) = emitSplitCtorDtor(
            m_ctorStmts, m_dtorStmts, 0, modName, modName + "* vlSelf", "vlSelf", ମିଥ୍ୟା,
            "Design implementation internals", [ଏହି]() { emitFileHeader(); });
    }
    ଶୂନ୍ୟ emitCtorDtorImp(ଅଚଳ Ast_ଗଣ୍ଠି_Module* modp) {
        ଅଚଳ std::string modName = EmitCUtil::prefixNameProtect(modp);

        // ctor
        ଯଦି (modp->ctorVarReset()) {
            m_lazyDecls.emit("\nvoid " + modName + "__", protect("_ctor_var_reset"),
                             "(" + modName + "* vlSelf);\n");
        }

        ଯଦି (!m_ctorSplitDecl.empty()) puts("\n" + m_ctorSplitDecl + "\n");
        puts("\nvoid " + modName + "__ctor(" + modName + "* vlSelf, " + EmitCUtil::symClassName()
             + "* symsp, const char* namep) {\n");
        puts("vlSelf->vlSymsp = symsp;\n");
        ଯଦି (modp->is_ଶୀର୍ଷ()) {
            puts("vlSelf->vlNamep = strdup(namep);\n");
        } ଅନ୍ୟଥା {
            puts("vlSelf->vlNamep = strdup(Verilated::catName(symsp->name(), namep));\n");
        }
        putsDecoration(modp, "// Construct members\n");
        ପାଇଁ (ଅଚଳ TextStmts::Stmt& stmt : m_ctorStmts) puts(stmt.m_text + "\n");

        putsDecoration(modp, "// Reset structure values\n");
        ଯଦି (modp->ctorVarReset()) {
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
        ଯଦି (!m_dtorSplitDecl.empty()) puts("\n" + m_dtorSplitDecl + "\n");
        puts("\nvoid " + modName + "__dtor(" + modName + "* vlSelf) {\n");
        putsDecoration(modp, "// Destroy members\n");
        ପାଇଁ (ଅଚଳ TextStmts::Stmt& stmt : m_dtorStmts) puts(stmt.m_text + "\n");
        puts("VL_DO_DANGLING(std::free(const_cast<char*>(vlSelf->vlNamep)), "
             "vlSelf->vlNamep);\n");
        puts("}\n");

        // C++ destructor
        puts("\n" + modName + "::~" + modName + "() {\n");
        emitSystemCSection(modp, VSystemCSectionType::DTOR);
        puts(modName + "__dtor(this);\n");
        puts("}\n");
    }

    ଶୂନ୍ୟ emitConfigureImp(ଅଚଳ Ast_ଗଣ୍ଠି_Module* modp) {
        ଅଚଳ string modName = EmitCUtil::prefixNameProtect(modp);

        ଯଦି (v3Global.opt.coverage()) {
            puts("\n");
            m_lazyDecls.emit("void " + modName + "__", protect("_configure_coverage"),
                             "(" + modName + "* vlSelf, bool first);");
        }

        ଯଦି (v3Global.opt.coverage()) {
            puts("\nvoid " + modName + "::" + protect("__Vconfigure") + "(bool first) {\n");
            puts("(void)first;  // Prevent unused variable warning\n");
            puts(modName + "__" + protect("_configure_coverage") + "(this, first);\n");
            puts("}\n");
        }
    }
    ଶୂନ୍ୟ emitCoverageImp() {
        // Rather than putting out VL_COVER_INSERT calls directly, we do it via this
        // function. This gets around gcc slowness constructing all of the template
        // arguments.
        ଯଦି (v3Global.opt.coverage()) {
            puts("\n// Coverage\n");
            puts("void " + EmitCUtil::prefixNameProtect(m_modp) + "::__vlCoverInsert(");
            puts(v3Global.opt.threads() > 1 ? "std::atomic<uint32_t>" : "uint32_t");
            puts("* countp, bool enable, bool localCounter, const char* filenamep, int lineno, "
                 "int column,\n");
            puts("const char* hierp, const char* pagep, const char* commentp, const char* "
                 "linescovp,\n");
            puts("const char* fsmVarp, const char* fsmFromp, const char* fsmTop, const char* "
                 "fsmTagp) {\n");
            ଯଦି (v3Global.opt.threads() > 1) {
                puts("assert(sizeof(uint32_t) == sizeof(std::atomic<uint32_t>));\n");
                puts("uint32_t* count32p = reinterpret_cast<uint32_t*>(countp);\n");
            } ଅନ୍ୟଥା {
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
        ଯଦି (v3Global.opt.coverageToggle()) {
            puts("\n// Toggle Coverage\n");
            puts("void " + EmitCUtil::prefixNameProtect(m_modp) + "::__vlCoverToggleInsert(");
            puts("int begin, int end, bool ranged, ");
            puts(v3Global.opt.threads() > 1 ? "std::atomic<uint32_t>" : "uint32_t");
            puts("* countp, bool enable, bool localCounter, const char* filenamep, int lineno, "
                 "int column,\n");
            puts("const char* hierp, const char* pagep, const char* commentp) {\n");
            ଯଦି (v3Global.opt.threads() > 1) {
                puts("assert(sizeof(uint32_t) == sizeof(std::atomic<uint32_t>));\n");
            }
            puts("int step = (end >= begin) ? 1 : -1;\n");
            // range is inclusive
            puts("for (int i = begin; i != end + step; i += step) {\n");
            puts("for (int j = 0; j < 2; j++) {\n");
            ଯଦି (v3Global.opt.threads() > 1) {
                puts("uint32_t* count32p = reinterpret_cast<uint32_t*>(countp);\n");
            } ଅନ୍ୟଥା {
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
    ଶୂନ୍ୟ emitSavableImp(ଅଚଳ Ast_ଗଣ୍ଠି_Module* modp) {
        ଯଦି (v3Global.opt.savable()) {
            puts("\n// Savable\n");
            ପାଇଁ (ପୂର୍ଣ୍ଣସଂଖ୍ୟା de = 0; de < 2; ++de) {
                ଅଚଳ string classname = de ? "VerilatedDeserialize" : "VerilatedSerialize";
                ଅଚଳ string funcname = de ? "__Vdeserialize" : "__Vserialize";
                ଅଚଳ string op = de ? ">>" : "<<";
                // NOLINTNEXTLINE(performance-inefficient-string-concatenation)
                putns(modp, "void " + EmitCUtil::prefixNameProtect(modp) + "::" + protect(funcname)
                                + "(" + classname + "& os) {\n");
                // Place a computed checksum to ensure proper structure save/restore formatting
                // OK if this hash includes some things we won't dump, since
                // just looking for loading the wrong model
                VHashSha512 hash;
                ପାଇଁ (Ast_ଗଣ୍ଠି* nodep = modp->stmtsp(); nodep; nodep = nodep->nextp()) {
                    ଯଦି (ଅଚଳ AstVar* ଅଚଳ varp = VN_CAST(nodep, Var)) {
                        hash.ଭର୍ତ୍ତି(varp->name());
                        hash.ଭର୍ତ୍ତି(varp->dtypep()->width());
                    }
                }
                ofp()->printf("uint64_t __Vcheckval = 0x%" PRIx64 "ULL;\n",
                              static_cast<uint64_t>(hash.digestUInt64()));
                ଯଦି (de) {
                    puts("os.readAssert(__Vcheckval);\n");
                } ଅନ୍ୟଥା {
                    puts("os << __Vcheckval;\n");
                }

                // Save context
                // If multiple models save the same context we'll save it multiple
                // times, but is harmless, and doing it otherwise would break
                // backwards compatibility.
                puts("os " + op + " vlSymsp->_vm_contextp__;\n");

                // Save all members
                ପାଇଁ (Ast_ଗଣ୍ଠି* nodep = modp->stmtsp(); nodep; nodep = nodep->nextp()) {
                    ଯଦି (ଅଚଳ AstVar* ଅଚଳ varp = VN_CAST(nodep, Var)) {
                        ଯଦି (varp->isIO() && modp->is_ଶୀର୍ଷ() && optSystemC()) {
                            // System C top I/O doesn't need loading, as the
                            // lower level subinst code does it.
                        } ଅନ୍ୟଥା ଯଦି (varp->isParam()) {
                        } ଅନ୍ୟଥା ଯଦି (VN_IS(varp->dtypep(), NBACommitQueueDType)) {
                        } ଅନ୍ୟଥା {
                            ପୂର୍ଣ୍ଣସଂଖ୍ୟା vects = 0;
                            Ast_ଗଣ୍ଠି_DType* elementp = varp->dtypeSkipRefp();
                            ପାଇଁ (AstUnpackArrayDType* arrayp = VN_CAST(elementp, UnpackArrayDType);
                                 arrayp; arrayp = VN_CAST(elementp, UnpackArrayDType)) {
                                ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା vecnum = vects++;
                                UASSERT_OBJ(arrayp->hi() >= arrayp->lo(), varp,
                                            "Should have swapped msb & lsb earlier.");
                                ଅଚଳ string ivar = "__Vi"s + cvtToStr(vecnum);
                                puts("for (int __Vi" + cvtToStr(vecnum) + " = " + cvtToStr(0));
                                puts("; " + ivar + " < " + cvtToStr(arrayp->elementsConst()));
                                puts("; ++" + ivar + ") {\n");
                                elementp = arrayp->subDTypep()->skipRefp();
                            }
                            ଅଚଳ AstBasicDType* ଅଚଳ basicp = elementp->basicp();
                            // Do not save MTask state, only matters within an evaluation
                            ଯଦି (basicp && basicp->keyword().isMTaskState()) ଜାରି;
                            // Want to detect types that are represented as arrays
                            // (i.e. packed types of more than 64 bits).
                            ଯଦି (elementp->isWide()
                                && !(basicp && basicp->keyword() == VBasicDTypeKwd::STRING)) {
                                ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା vecnum = vects++;
                                ଅଚଳ string ivar = "__Vi"s + cvtToStr(vecnum);
                                puts("for (int __Vi" + cvtToStr(vecnum) + " = " + cvtToStr(0));
                                puts("; " + ivar + " < " + cvtToStr(elementp->width_ଶବ୍ଦଗୁଡ଼ିକ()));
                                puts("; ++" + ivar + ") {\n");
                            }
                            putns(varp, "os" + op + varName(varp));
                            ପାଇଁ (ପୂର୍ଣ୍ଣସଂଖ୍ୟା v = 0; v < vects; ++v) puts("[__Vi" + cvtToStr(v) + "]");
                            puts(";\n");
                            ପାଇଁ (ପୂର୍ଣ୍ଣସଂଖ୍ୟା v = 0; v < vects; ++v) puts("}\n");
                        }
                    }
                }

                puts("}\n");
            }
        }
    }
    // Predicate to check if we actually need to emit anything into the common implementation file.
    // Used to avoid creating empty output files.
    ତାର୍କିକ hasCommonImp(ଅଚଳ Ast_ଗଣ୍ଠି_Module* modp) ଅଚଳ {
        // Nothing to emit if no module!
        ଯଦି (!modp) ଫେରନ୍ତୁ ମିଥ୍ୟା;
        // We always need the slow file
        ଯଦି (m_slow) ଫେରନ୍ତୁ ସତ୍ୟ;
        // The fast file is only required when we have `systemc_implementation nodes
        ପାଇଁ (ଅଚଳ AstSystemCSection* scSectionp = modp->scSectionsp(); scSectionp;
             scSectionp = VN_AS(scSectionp->nextp(), SystemCSection)) {
            ଯଦି (scSectionp->sectionType() == VSystemCSectionType::IMP) ଫେରନ୍ତୁ ସତ୍ୟ;
        }
        ଫେରନ୍ତୁ ମିଥ୍ୟା;
    }
    // Actually emit common implementation contents for given AstNodeModule
    ଶୂନ୍ୟ doCommonImp(ଅଚଳ Ast_ଗଣ୍ଠି_Module* modp) {
        ଯଦି (m_slow) {
            ଯଦି (!VN_IS(modp, Class)) {
                emitParamDefns(modp);
                emitCtorDtorImp(modp);
                emitConfigureImp(modp);
                emitCoverageImp();
            }
            emitSavableImp(modp);
        } ଅନ୍ୟଥା {
            // From `systemc_implementation
            emitSystemCSection(modp, VSystemCSectionType::IMP);
        }
    }
    ଶୂନ୍ୟ emitCommonImp(ଅଚଳ Ast_ଗଣ୍ଠି_Module* modp) {
        ଅଚଳ AstClass* ଅଚଳ classp
            = VN_IS(modp, ClassPackage) ? VN_AS(modp, ClassPackage)->classp() : nullptr;

        ଯଦି (hasCommonImp(modp) || hasCommonImp(classp)) {
            // Create ctor/dtor statements. If these need splitting, they will be emitted
            // during construction so do this before opening the output file.
            ଯଦି (m_slow && !VN_IS(modp, Class)) createCtorDtorStmts(modp);

            openNextOutputFile(m_fileBaseName);

            doCommonImp(modp);
            ଯଦି (classp) {
                VL_RESTORER(m_modp);
                m_modp = classp;
                doCommonImp(classp);
            }

            closeOutputFile();
        }
    }
    ଶୂନ୍ୟ emitCFuncImp(ଅଚଳ Ast_ଗଣ୍ଠି_Module* modp) {
        // Functions to be emitted here
        std::vector<AstCFunc*> funcps;

        ଅଚଳ auto gather = [ଏହି, &funcps](ଅଚଳ Ast_ଗଣ୍ଠି_Module* modp) {
            ପାଇଁ (Ast_ଗଣ୍ଠି* nodep = modp->stmtsp(); nodep; nodep = nodep->nextp()) {
                AstCFunc* ଅଚଳ funcp = VN_CAST(nodep, CFunc);
                ଯଦି (!funcp) ଜାରି;
                // TRACE_* and DPI handled elsewhere
                ଯଦି (funcp->isTrace()) ଜାରି;
                ଯଦି (funcp->dpiImportPrototype()) ଜାରି;
                ଯଦି (funcp->dpiExportDispatcher()) ଜାରି;
                ଯଦି (funcp->slow() != m_slow) ଜାରି;
                funcps.ଠେଲ_back(funcp);
            }
        };

        gather(modp);
        VL_RESTORER(m_classOrPackage);
        ଯଦି (ଅଚଳ AstClassPackage* ଅଚଳ packagep = VN_CAST(modp, ClassPackage)) {
            m_classOrPackage = packagep;
            gather(packagep->classp());
        }

        // Do not create empty files
        ଯଦି (funcps.empty()) ଫେରନ୍ତୁ;

        // Open output file
        openNextOutputFile(m_uniqueNames.get(m_fileBaseName));
        // Emit all functions
        ପାଇଁ (AstCFunc* ଅଚଳ funcp : funcps) {
            VL_RESTORER(m_modp);
            m_modp = EmitCParentModule::get(funcp);
            iterateConst(funcp);
        }
        // Close output file
        closeOutputFile();
    }

    // VISITORS
    ଶୂନ୍ୟ visit(AstCFunc* nodep) override {
        ଯଦି (splitNeeded()) {
            // Splitting file, so using parallel build.
            v3Global.useParallelBuild(ସତ୍ୟ);
            // Close old file
            closeOutputFile();
            // Open a new file
            openNextOutputFile(m_uniqueNames.get(m_fileBaseName));
        }

        EmitCFunc::visit(nodep);
    }

    explicit EmitCImp(ଅଚଳ Ast_ଗଣ୍ଠି_Module* modp, ତାର୍କିକ slow)
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
    ~EmitCImp() override = ପୂର୍ବନିର୍ଦ୍ଧାରିତ;

ସର୍ବସାଧାରଣ:
    ସ୍ଥିର std::vector<AstCFile*> ମୁଖ୍ୟ(ଅଚଳ Ast_ଗଣ୍ଠି_Module* modp, ତାର୍କିକ slow) VL_MT_STABLE {
        EmitCImp emitCImp{modp, slow};
        ଫେରନ୍ତୁ emitCImp.getAndClearCfileps();
    }
};

//######################################################################
// Tracing routines

// Trace type descriptors go in a different file as it needs to be written in
// parallel with the actual trace function source files
ଶ୍ରେଣୀ EmitCTraceTypes final : ସର୍ବସାଧାରଣ EmitCFunc {
    // NODE STATE/TYPES
    // None allowed to support threaded emitting

    // STATE
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା m_enum_ଅଙ୍କ = 0;  // Enumeration number (whole netlist)
    std::unordered_map<Ast_ଗଣ୍ଠି*, ପୂର୍ଣ୍ଣସଂଖ୍ୟା> m_enum_ଅଙ୍କ_Map;  // EnumDType to enumeration number
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା m_traceTypeSubs = 0;  // Number of trace type declaration sub-functions
    V3UniqueNames m_uniqueNames;  // Generates unique file names
    ଅଚଳ std::string m_fileBaseName = EmitCUtil::ଶୀର୍ଷ_Class_Name() + "_" + protect("_TraceDecls");
    // This one uses CSplitTrace for file splitting, which is incorrect but historically accurates
    ଅଚଳ size_t m_splitLimit = v3Global.opt.outputSplitCTrace()
                                    ? static_cast<size_t>(v3Global.opt.outputSplitCTrace())
                                    : std::numeric_limits<size_t>::max();

    ଶୂନ୍ୟ openNextOutputFile() {
        openNewOutputSourceFile(m_uniqueNames.get(m_fileBaseName), ସତ୍ୟ, ସତ୍ୟ,
                                "Tracing declarations");
        puts("\n");
        ପାଇଁ (ଅଚଳ std::string& base : v3Global.opt.traceSourceLangs()) {
            puts("#include \"" + base + ".h\"\n");
        }
        puts("\n");
        puts("\nvoid " + EmitCUtil::prefixNameProtect(m_modp) + "__"
             + protect("traceDeclTypesSub" + std::to_string(m_traceTypeSubs++)) + "("
             + v3Global.opt.traceClassBase() + "* tracep) {\n");
    }

ସର୍ବସାଧାରଣ:
    // METHODS
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା get_Enum_Map_ଅଙ୍କ(AstEnumDType* nodep) {
        ପୂର୍ଣ୍ଣସଂଖ୍ୟା& enumNumr = m_enum_ଅଙ୍କ_Map[nodep];
        ଯଦି (!enumNumr) {
            ଯଦି (splitNeeded(m_splitLimit)) {
                // Splitting file, so using parallel build.
                v3Global.useParallelBuild(ସତ୍ୟ);
                puts("}\n");
                closeOutputFile();
                openNextOutputFile();
            }

            enumNumr = ++m_enum_ଅଙ୍କ;
            ପୂର୍ଣ୍ଣସଂଖ୍ୟା nvals = 0;
            puts("{\n");
            putns(nodep, "const char* " + protect("__VenumItemNames") + "[]\n");
            puts("= {");
            ପାଇଁ (AstEnumItem* itemp = nodep->itemsp(); itemp;
                 itemp = VN_AS(itemp->nextp(), EnumItem)) {
                ଯଦି (++nvals > 1) puts(", ");
                putbs("\"" + itemp->prettyName() + "\"");
            }
            puts("};\n");
            nvals = 0;
            puts("const char* " + protect("__VenumItemValues") + "[]\n");
            puts("= {");
            ପାଇଁ (AstEnumItem* itemp = nodep->itemsp(); itemp;
                 itemp = VN_AS(itemp->nextp(), EnumItem)) {
                AstConst* ଅଚଳ constp = VN_AS(itemp->valuep(), Const);
                ଯଦି (++nvals > 1) puts(", ");
                putbs("\"" + constp->ଅଙ୍କ().displayed(nodep, "%0b") + "\"");
            }
            puts("};\n");
            puts("tracep->declDTypeEnum(" + std::to_string(enumNumr) + ", \"" + nodep->prettyName()
                 + "\", " + std::to_string(nvals) + ", " + std::to_string(nodep->widthMin()) + ", "
                 + protect("__VenumItemNames") + ", " + protect("__VenumItemValues") + ");\n");
            puts("}\n");
            splitSizeInc(Ast_ଗଣ୍ଠି::INSTR_ଗଣନା_CALL);
        }
        ଫେରନ୍ତୁ enumNumr;
    }

    // Close output file
    ଶୂନ୍ୟ finalize() {
        // Close function definition
        puts("}\n");

        ଅଚଳ std::string modName = EmitCUtil::prefixNameProtect(m_modp);
        ଅଚଳ std::string args = v3Global.opt.traceClassBase() + "* tracep";

        // Forward declarations for subs in other files
        ପାଇଁ (ପୂର୍ଣ୍ଣସଂଖ୍ୟା i = 0; i < m_traceTypeSubs - 1; ++i) {
            puts("void " + modName + "__" + protect("traceDeclTypesSub" + std::to_string(i)) + "("
                 + args + ");\n");
        }

        // Create top level trace_decl_types function and call each sub-function
        puts("\nvoid " + modName + "__" + protect("trace_decl_types") + "(" + args + ") {\n");
        ପାଇଁ (ପୂର୍ଣ୍ଣସଂଖ୍ୟା i = 0; i < m_traceTypeSubs; ++i) {
            puts(modName + "__" + protect("traceDeclTypesSub" + std::to_string(i))
                 + "(tracep);\n");
        }
        puts("}\n");

        closeOutputFile();
    }

    EmitCTraceTypes() {
        m_modp = v3Global.rootp()->ଶୀର୍ଷ_Modulep();
        openNextOutputFile();
    }
    ~EmitCTraceTypes() override = ପୂର୍ବନିର୍ଦ୍ଧାରିତ;
};

ଶ୍ରେଣୀ EmitCTrace final : ସର୍ବସାଧାରଣ EmitCFunc {
    // NODE STATE/TYPES
    // None allowed to support threaded emitting

    // MEMBERS
    ଅଚଳ ତାର୍କିକ m_slow;  // Making slow file
    ଅଚଳ std::unique_ptr<EmitCTraceTypes> m_emitTypesp{m_slow ? ନୂତନ EmitCTraceTypes{} : nullptr};
    V3UniqueNames m_uniqueNames;  // Generates unique file names
    ଅଚଳ std::string m_fileBaseName = EmitCUtil::ଶୀର୍ଷ_Class_Name() + "_" + protect("_Trace");

    // METHODS
    ଶୂନ୍ୟ openNextOutputFile() {
        openNewOutputSourceFile(m_uniqueNames.get(m_fileBaseName), m_slow, ସତ୍ୟ,
                                "Tracing implementation internals");
        puts("\n");
        ପାଇଁ (ଅଚଳ std::string& base : v3Global.opt.traceSourceLangs()) {
            puts("#include \"" + base + ".h\"\n");
        }
        puts("#include \"" + EmitCUtil::symClassName() + ".h\"\n");
        puts("\n");
        // Need to emit new lazy declarations
        m_lazyDecls.reset();
    }

    ତାର୍କିକ emitTraceIsScBv(ଅଚଳ AstTraceInc* nodep) {
        ଅଚଳ AstVarRef* ଅଚଳ varrefp = VN_CAST(nodep->declp()->valuep(), VarRef);
        ଯଦି (!varrefp) ଫେରନ୍ତୁ ମିଥ୍ୟା;
        ଅଚଳ AstVar* ଅଚଳ varp = varrefp->varp();
        ଫେରନ୍ତୁ varp->isSc() && varp->isScBv();
    }

    ତାର୍କିକ emitTraceIsScBigUint(ଅଚଳ AstTraceInc* nodep) {
        ଅଚଳ AstVarRef* ଅଚଳ varrefp = VN_CAST(nodep->declp()->valuep(), VarRef);
        ଯଦି (!varrefp) ଫେରନ୍ତୁ ମିଥ୍ୟା;
        ଅଚଳ AstVar* ଅଚଳ varp = varrefp->varp();
        ଫେରନ୍ତୁ varp->isSc() && varp->isScBigUint();
    }

    ତାର୍କିକ emitTraceIsScUint(ଅଚଳ AstTraceInc* nodep) {
        ଅଚଳ AstVarRef* ଅଚଳ varrefp = VN_CAST(nodep->declp()->valuep(), VarRef);
        ଯଦି (!varrefp) ଫେରନ୍ତୁ ମିଥ୍ୟା;
        ଅଚଳ AstVar* ଅଚଳ varp = varrefp->varp();
        ଫେରନ୍ତୁ varp->isSc() && (varp->isScUint() || varp->isScUintBool());
    }

    ଶୂନ୍ୟ emitTraceInitOne(ଅଚଳ AstTraceDecl* nodep, ପୂର୍ଣ୍ଣସଂଖ୍ୟା enum_ଅଙ୍କ) {
        std::string direction;
        direction = nodep->declDirection().traceSigDirection();

        AstCCall* ଅଚଳ callp = nodep->dtypeCallp();
        ଯଦି (callp) {
            callp->argTypes(callp->argTypes() + ", " + cvtToStr(nodep->fidx()) + ", c+"
                            + cvtToStr(nodep->code()) + ", " + direction);
            ଫେରନ୍ତୁ;
        }

        ଯଦି (nodep->dtypep()->basicp()->isDouble()) {
            puts("VL_TRACE_DECL_DOUBLE");
        } ଅନ୍ୟଥା ଯଦି (nodep->isWide()) {
            puts("VL_TRACE_DECL_WIDE");
        } ଅନ୍ୟଥା ଯଦି (nodep->isQuad()) {
            puts("VL_TRACE_DECL_QUAD");
        } ଅନ୍ୟଥା ଯଦି (nodep->bitRange().ranged()) {
            puts("VL_TRACE_DECL_BUS");
        } ଅନ୍ୟଥା ଯଦି (nodep->dtypep()->basicp()->isEvent()) {
            puts("VL_TRACE_DECL_EVENT");
        } ଅନ୍ୟଥା {
            puts("VL_TRACE_DECL_BIT");
        }

        ଯଦି (nodep->arrayRange().ranged()) {
            puts("_ARRAY(tracep");
        } ଅନ୍ୟଥା {
            puts("(tracep");
        }

        // Code
        puts(",c+" + cvtToStr(nodep->code()));
        ଯଦି (nodep->arrayRange().ranged()) puts("+i*" + cvtToStr(nodep->width_ଶବ୍ଦଗୁଡ଼ିକ()));

        // Function index
        puts(",");
        ଯଦି (nodep->inDtypeFunc()) {
            puts("fidx");
        } ଅନ୍ୟଥା {
            puts(cvtToStr(nodep->fidx()));
        }

        // Name
        puts(",");
        putsQuoted(VIdProtect::protect_ଶବ୍ଦଗୁଡ଼ିକ_If(nodep->showname(), nodep->protect()));

        // Enum number
        puts("," + cvtToStr(enum_ଅଙ୍କ));

        // Direction
        ଯଦି (nodep->inDtypeFunc()) {
            puts(", direction");
        } ଅନ୍ୟଥା {
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
        ଯଦି (nodep->arrayRange().ranged()) {
            ଯଦି (nodep->arrayRange().ascending()) {
                puts(", (i + " + std::to_string(nodep->arrayRange().lo()) + ")");
            } ଅନ୍ୟଥା {
                puts(", (" + std::to_string(nodep->arrayRange().hi()) + " - i)");
            }
        }

        // Bit range
        ଯଦି (!nodep->dtypep()->basicp()->isDouble() && nodep->bitRange().ranged()) {
            puts(", " + cvtToStr(nodep->bitRange().ବାମ()) + ","
                 + cvtToStr(nodep->bitRange().ଡାହାଣ()));
        }

        //
        puts(");");
    }

    ପୂର୍ଣ୍ଣସଂଖ୍ୟା emitTraceDeclDType(Ast_ଗଣ୍ଠି_DType* nodep) {
        // Return enum number or -1 for none
        ଯଦି (v3Global.opt.traceEnabledFst()) {
            // Skip over refs-to-refs, but stop before final ref so can get data type name
            // Alternatively back in V3Width we could push enum names from upper typedefs
            ଯଦି (AstEnumDType* ଅଚଳ enump = VN_CAST(nodep->skipRefToEnump(), EnumDType)) {
                ଫେରନ୍ତୁ m_emitTypesp->get_Enum_Map_ଅଙ୍କ(enump);
            }
        }
        ଫେରନ୍ତୁ -1;
    }

    ଶୂନ୍ୟ emitTraceChangeOne(AstTraceInc* nodep, ପୂର୍ଣ୍ଣସଂଖ୍ୟା arrayindex) {
        // Note: Both VTraceType::CHANGE and VTraceType::FULL use the 'full' methods
        ଅଚଳ std::string func = nodep->traceType() == VTraceType::CHANGE ? "chg" : "full";
        ତାର୍କିକ emitWidth = ସତ୍ୟ;
        string stype;
        ଯଦି (nodep->dtypep()->basicp()->isDouble()) {
            stype = "Double";
            emitWidth = ମିଥ୍ୟା;
        } ଅନ୍ୟଥା ଯଦି (nodep->isWide() || emitTraceIsScBv(nodep) || emitTraceIsScBigUint(nodep)) {
            stype = "WData";
        } ଅନ୍ୟଥା ଯଦି (nodep->isQuad()) {
            stype = "QData";
        } ଅନ୍ୟଥା ଯଦି (nodep->declp()->widthMin() > 16) {
            stype = "IData";
        } ଅନ୍ୟଥା ଯଦି (nodep->declp()->widthMin() > 8) {
            stype = "SData";
        } ଅନ୍ୟଥା ଯଦି (nodep->declp()->widthMin() > 1) {
            stype = "CData";
        } ଅନ୍ୟଥା ଯଦି (nodep->dtypep()->basicp()->isEvent()) {
            stype = "Event";
            emitWidth = ମିଥ୍ୟା;
        } ଅନ୍ୟଥା {
            stype = "Bit";
            emitWidth = ମିଥ୍ୟା;
        }
        putns(nodep, "bufp->" + func + stype);

        ଅଚଳ uint32_t offset = (arrayindex < 0) ? 0 : (arrayindex * nodep->declp()->width_ଶବ୍ଦଗୁଡ଼ିକ());
        ଅଚଳ uint32_t code = nodep->declp()->code() + offset;
        // Note: Both VTraceType::CHANGE and VTraceType::FULL use the 'full' methods
        puts("(oldp+");
        puts(cvtToStr(code - nodep->baseCode()));
        puts(",");
        ଅଚଳ VNumRange& arrayRange = nodep->declp()->arrayRange();
        ଯଦି (arrayRange.ranged() && !arrayRange.ascending()) {
            arrayindex = arrayRange.elements() - 1 - arrayindex;
        }
        emit_Trace_ମୂଲ୍ୟ(nodep, arrayindex);
        ଯଦି (emitWidth) puts("," + cvtToStr(nodep->declp()->widthMin()));
        puts(");\n");
    }

    ଶୂନ୍ୟ emit_Trace_ମୂଲ୍ୟ(ଅଚଳ AstTraceInc* nodep, ପୂର୍ଣ୍ଣସଂଖ୍ୟା arrayindex) {
        ଯଦି (AstVarRef* ଅଚଳ varrefp = VN_CAST(nodep->valuep(), VarRef)) {
            ଅଚଳ AstVar* ଅଚଳ varp = varrefp->varp();
            ଯଦି (varp->isEvent()) puts("&");
            puts("(");
            ଯଦି (emitTraceIsScBigUint(nodep)) {
                puts("(uint32_t*)");
            } ଅନ୍ୟଥା ଯଦି (emitTraceIsScBv(nodep)) {
                puts("VL_SC_BV_DATAP(");
            }
            iterateConst(varrefp);  // Put var name out
            emitTraceIndex(nodep, arrayindex);
            ଯଦି (varp->isSc()) puts(".read()");
            ଯଦି (emitTraceIsScUint(nodep)) {
                puts(nodep->isQuad() ? ".to_uint64()" : ".to_uint()");
            } ଅନ୍ୟଥା ଯଦି (emitTraceIsScBigUint(nodep)) {
                puts(".get_raw()");
            } ଅନ୍ୟଥା ଯଦି (emitTraceIsScBv(nodep)) {
                puts(")");
            }
            puts(")");
        } ଅନ୍ୟଥା {
            puts("(");
            iterateConst(nodep->valuep());
            emitTraceIndex(nodep, arrayindex);
            puts(")");
        }
    }

    ଶୂନ୍ୟ emitTraceIndex(ଅଚଳ AstTraceInc* ଅଚଳ nodep, ପୂର୍ଣ୍ଣସଂଖ୍ୟା arrayindex) {
        // Tracing only supports 1D arrays
        ଯଦି (nodep->declp()->arrayRange().ranged()) {
            ଯଦି (arrayindex == -2) {
                puts("[i]");
            } ଅନ୍ୟଥା ଯଦି (arrayindex == -1) {
                puts("[0]");
            } ଅନ୍ୟଥା {
                puts("[" + cvtToStr(arrayindex) + "]");
            }
        }
    }

    // VISITORS
    using EmitCFunc::visit;  // Suppress hidden overloaded virtual function warning
    ଶୂନ୍ୟ visit(AstCFunc* nodep) override {
        ଯଦି (!nodep->isTrace()) ଫେରନ୍ତୁ;
        ଯଦି (nodep->slow() != m_slow) ଫେରନ୍ତୁ;

        ଯଦି (splitNeeded()) {
            // Splitting file, so using parallel build.
            v3Global.useParallelBuild(ସତ୍ୟ);
            // Close old file
            closeOutputFile();
            // Open a new file
            openNextOutputFile();
        }

        EmitCFunc::visit(nodep);
    }
    ଶୂନ୍ୟ visit(Ast_Trace_ଠେଲ_Prefix* nodep) override {
        putns(nodep, "VL_TRACE_PUSH_PREFIX(tracep, ");
        ଯଦି (nodep->quotedPrefix()) {
            putsQuoted(VIdProtect::protect_ଶବ୍ଦଗୁଡ଼ିକ_If(nodep->prefix(), nodep->protect()));
        } ଅନ୍ୟଥା {
            puts(nodep->prefix());
        }
        puts(", VerilatedTracePrefixType::");
        puts(nodep->prefixType().ascii());
        puts(", " + std::to_string(nodep->ବାମ()));
        puts(", " + std::to_string(nodep->ଡାହାଣ()));
        puts(");\n");
    }
    ଶୂନ୍ୟ visit(Ast_Trace_କାଢ଼_Prefix* nodep) override {  //
        putns(nodep, "VL_TRACE_POP_PREFIX(tracep);\n");
    }
    ଶୂନ୍ୟ visit(AstTraceDecl* nodep) override {
        ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା enum_ଅଙ୍କ = emitTraceDeclDType(nodep->dtypep());
        putns(nodep, "");
        ଯଦି (nodep->arrayRange().ranged()) {
            puts("for (int i = 0; i < " + cvtToStr(nodep->arrayRange().elements()) + "; ++i) {\n");
            emitTraceInitOne(nodep, enum_ଅଙ୍କ);
            puts("\n}\n");
        } ଅନ୍ୟଥା {
            emitTraceInitOne(nodep, enum_ଅଙ୍କ);
            puts("\n");
        }
    }
    ଶୂନ୍ୟ visit(AstTraceInc* nodep) override {
        ଯଦି (nodep->declp()->arrayRange().ranged()) {
            // It traces faster if we unroll the loop
            ପାଇଁ (ପୂର୍ଣ୍ଣସଂଖ୍ୟା i = 0; i < nodep->declp()->arrayRange().elements(); i++) {
                emitTraceChangeOne(nodep, i);
            }
        } ଅନ୍ୟଥା {
            emitTraceChangeOne(nodep, -1);
        }
    }

    explicit EmitCTrace(ତାର୍କିକ slow)
        : m_slow{slow} {
        m_modp = v3Global.rootp()->ଶୀର୍ଷ_Modulep();
        // Open output file
        openNextOutputFile();
        // Emit functions
        ପାଇଁ (Ast_ଗଣ୍ଠି* nodep = m_modp->stmtsp(); nodep; nodep = nodep->nextp()) {
            ଯଦି (AstCFunc* ଅଚଳ funcp = VN_CAST(nodep, CFunc)) iterateConst(funcp);
        }
        // Close output file
        closeOutputFile();
        ଯଦି (m_slow) m_emitTypesp->finalize();
    }
    ~EmitCTrace() override = ପୂର୍ବନିର୍ଦ୍ଧାରିତ;

ସର୍ବସାଧାରଣ:
    ସ୍ଥିର std::vector<AstCFile*> ମୁଖ୍ୟ(ତାର୍କିକ slow) VL_MT_STABLE {
        EmitCTrace emitCTrace{slow};
        std::vector<AstCFile*> cfileps = emitCTrace.getAndClearCfileps();
        ଯଦି (slow) {
            ପାଇଁ (AstCFile* ଅଚଳ cfilep : emitCTrace.m_emitTypesp->getAndClearCfileps()) {
                cfileps.emplace_back(cfilep);
            }
        }
        ଫେରନ୍ତୁ cfileps;
    }
};

//######################################################################
// Existing AstCFile emitter

ଶ୍ରେଣୀ EmitCFile final : ସର୍ବସାଧାରଣ EmitCFunc {
    explicit EmitCFile(AstCFile* cfilep) {
        openOutputFile(cfilep, "Generated C++");
        iterateConst(cfilep->tblockp());
        closeOutputFile();
    }
    ~EmitCFile() override = ପୂର୍ବନିର୍ଦ୍ଧାରିତ;

ସର୍ବସାଧାରଣ:
    ସ୍ଥିର ଶୂନ୍ୟ ମୁଖ୍ୟ(AstCFile* cfilep) VL_MT_STABLE {
        ଯଦି (!cfilep->tblockp()) ଫେରନ୍ତୁ;
        EmitCFile{cfilep};
    }
};

//######################################################################
// EmitC class functions

ଶୂନ୍ୟ V3EmitC::emitcImp() {
    UINFO(2, __FUNCTION__ << ":");
    std::list<std::vector<AstCFile*>> cfiles;
    {
        // Make parent module pointers available.
        ଅଚଳ EmitCParentModule emitCParentModule;
        V3ThreadScope threadScope;

        // Process each module in turn
        ପାଇଁ (ଅଚଳ Ast_ଗଣ୍ଠି* nodep = v3Global.rootp()->modulesp(); nodep; nodep = nodep->nextp()) {
            ଯଦି (VN_IS(nodep, Class)) ଜାରି;  // Imped with ClassPackage
            ଅଚଳ Ast_ଗଣ୍ଠି_Module* ଅଚଳ modp = VN_AS(nodep, ଗଣ୍ଠି_Module);
            ଯଦି (modp->isConstPool()) ଜାରି;  // Emitted by V3EmitCConstPool
            cfiles.emplace_back();
            std::vector<AstCFile*>& slow = cfiles.back();
            threadScope.enqueue([modp, &slow] { slow = EmitCImp::ମୁଖ୍ୟ(modp, /* slow: */ ସତ୍ୟ); });
            cfiles.emplace_back();
            std::vector<AstCFile*>& fast = cfiles.back();
            threadScope.enqueue([modp, &fast] { fast = EmitCImp::ମୁଖ୍ୟ(modp, /* slow: */ ମିଥ୍ୟା); });
        }

        // Emit trace routines (currently they can only exist in the top module)
        ଯଦି (v3Global.opt.trace() && !v3Global.opt.lintOnly()) {
            cfiles.emplace_back();
            std::vector<AstCFile*>& slow = cfiles.back();
            threadScope.enqueue([&slow] { slow = EmitCTrace::ମୁଖ୍ୟ(/* slow: */ ସତ୍ୟ); });
            cfiles.emplace_back();
            std::vector<AstCFile*>& fast = cfiles.back();
            threadScope.enqueue([&fast] { fast = EmitCTrace::ମୁଖ୍ୟ(/* slow: */ ମିଥ୍ୟା); });
        }
    }
    // Add files to netlist
    ପାଇଁ (ଅଚଳ std::vector<AstCFile*>& cfileps : cfiles) {
        ପାଇଁ (AstCFile* ଅଚଳ cfilep : cfileps) v3Global.rootp()->addFilesp(cfilep);
    }
}

ଶୂନ୍ୟ V3EmitC::emitcFiles() {
    UINFO(2, __FUNCTION__ << ":");
    ପାଇଁ (Ast_ଗଣ୍ଠି_File *filep = v3Global.rootp()->filesp(), *nextp; filep; filep = nextp) {
        nextp = VN_AS(filep->nextp(), ଗଣ୍ଠି_File);
        ଯଦି (AstCFile* ଅଚଳ cfilep = VN_CAST(filep, CFile)) EmitCFile::ମୁଖ୍ୟ(cfilep);
    }
}
