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

#অন্তর্ভাৱ "V3PchAstMT.h"

#অন্তর্ভাৱ "V3EmitC.h"
#অন্তর্ভাৱ "V3EmitCFunc.h"
#অন্তর্ভাৱ "V3ThreadPool.h"
#অন্তর্ভাৱ "V3UniqueNames.h"

#অন্তর্ভাৱ <algorithm>
#অন্তর্ভাৱ <map>
#অন্তর্ভাৱ <set>
#অন্তর্ভাৱ <tuple>
#অন্তর্ভাৱ <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Internal EmitC implementation

শ্ৰেণী EmitCImp final : সার্ৱজনিক EmitCFunc {
    // MEMBERS
    // Base module (For non-classes, same as m_modp. For classes, it's the ClassPackage.)
    ধ্রুৱ Ast_গ্ৰন্থি_Module* ধ্রুৱ m_fileModp;
    ধ্রুৱ তার্কিক m_slow;  // Creating __Slow file
    V3UniqueNames m_uniqueNames;  // Generates unique file names
    ধ্রুৱ std::string m_fileBaseName = EmitCUtil::prefixNameProtect(m_fileModp);
    TextStmts m_ctorStmts;  // Statements constructing members in 'ctor'
    TextStmts m_dtorStmts;  // Statements destroying members in 'dtor'
    std::string m_ctorSplitDecl;  // Declaration of sub-function 'ctor' was split into, if any
    std::string m_dtorSplitDecl;  // Declaration of sub-function 'dtor' was split into, if any

    // METHODS
    শূন্য openNextOutputFile(ধ্রুৱ std::string& fileName) {
        openNewOutputSourceFile(fileName, m_slow, মিছা, "Design implementation internals");
        emitFileHeader();
    }
    শূন্য emitFileHeader() {
        puts("// See " + EmitCUtil::শীৰ্ষ_Class_Name() + ".h for the primary calling header\n");
        puts("\n");
        puts("#include \"" + EmitCUtil::pchClassName() + ".h\"\n");
        emitSystemCSection(m_fileModp, VSystemCSectionType::IMP_HDR);
        // Need to emit new lazy declarations
        m_lazyDecls.reset();
    }

    শূন্য emitParamDefns(ধ্রুৱ Ast_গ্ৰন্থি_Module* modp) {
        তার্কিক first = সঁচা;
        বাবে (ধ্রুৱ Ast_গ্ৰন্থি* nodep = modp->stmtsp(); nodep; nodep = nodep->nextp()) {
            যদি (ধ্রুৱ AstVar* ধ্রুৱ varp = VN_CAST(nodep, Var)) {
                যদি (varp->isParam()) {
                    যদি (first) {
                        puts("\n");
                        ধ্রুৱ string modName = EmitCUtil::prefixNameProtect(modp);
                        putsDecoration(modp, "// Parameter definitions for " + modName + "\n");
                        first = মিছা;
                    }
                    যদি (varp->dtypep()->isLiteralType()) putns(varp, "VL_CONSTINIT_CXX20 ");
                    putns(varp, "const ");
                    emitVarDefn(varp);
                    puts(";\n");
                }
            }
        }
        যদি (!first) puts("\n");
    }
    // Add statements explicitly constructing, or destroying, a member AstVar
    স্থির শূন্য addVarCtorDtorStmts(TextStmts& stmts, ধ্রুৱ AstVar* varp, তার্কিক destroy) {
        ধ্রুৱ std::string name = "vlSelf->" + varName(varp);

        // SystemC port arrays are C arrays, so construct them element by element. They are
        // default constructed (unnamed), as the elements of an array member would be.
        যদি (varp->isSc() && VN_IS(varp->dtypeSkipRefp(), UnpackArrayDType)) {
            std::vector<std::string> elems{name};
            বাবে (ধ্রুৱ AstUnpackArrayDType* adtypep
                 = VN_CAST(varp->dtypeSkipRefp(), UnpackArrayDType);
                 adtypep; adtypep = VN_CAST(adtypep->subDTypep()->skipRefp(), UnpackArrayDType)) {
                std::vector<std::string> subElems;
                বাবে (ধ্রুৱ std::string& elem : elems) {
                    বাবে (পূর্ণাঙ্ক i = 0; i < adtypep->elementsConst(); ++i) {
                        subElems.ঠেলো_back(elem + "[" + std::to_string(i) + "]");
                    }
                }
                elems = std::move(subElems);
            }
            যদি (destroy) std::reverse(elems.begin(), elems.end());
            বাবে (ধ্রুৱ std::string& e : elems) {
                stmts.add((destroy ? "VL_INPLACE_DESTRUCT(" : "VL_INPLACE_CONSTRUCT(") + e + ");");
            }
            ঘূৰাই;
        }

        // Other variables are constructed whole, even if aggregates
        যদি (destroy) {
            stmts.add("VL_INPLACE_DESTRUCT(" + name + ");");
            ঘূৰাই;
        }
        ধ্রুৱ AstBasicDType* ধ্রুৱ basicp = VN_CAST(varp->dtypeSkipRefp(), BasicDType);
        std::string args;
        যদি (varp->isSc()) {
            // SystemC ports need their name
            args = ", \"" + V3OutFormatter::quoteNameControls(varp->nameProtect()) + "\"";
        } অন্যথা যদি (basicp && basicp->keyword().isMTaskState()) {
            // MTask state needs the number of upstream dependencies to wait for
            args = ", " + VN_AS(varp->valuep(), Const)->অঙ্ক().emitC();
        } অন্যথা যদি (basicp && basicp->isDelayScheduler()) {
            // Delay scheduler needs the context pointer
            args = ", *vlSelf->vlSymsp->_vm_contextp__";
        }
        stmts.add("VL_INPLACE_CONSTRUCT(" + name + args + ");");
    }
    // Gather the statements constructing/destroying members in anonymous unions, and split
    // them into sub-functions if needed. As this emits files, no output file can be open.
    শূন্য createCtorDtorStmts(ধ্রুৱ Ast_গ্ৰন্থি_Module* modp) {
        // Construct member variables
        বাবে (ধ্রুৱ Ast_গ্ৰন্থি* nodep = modp->stmtsp(); nodep; nodep = nodep->nextp()) {
            ধ্রুৱ AstVar* ধ্রুৱ varp = VN_CAST(nodep, Var);
            যদি (varp && isExplicitlyConstructed(varp)) {
                addVarCtorDtorStmts(m_ctorStmts, varp, মিছা);
            }
        }

        // Destroy in reverse order
        ধ্রুৱ Ast_গ্ৰন্থি* ধ্রুৱ lastp = modp->stmtsp() ? modp->stmtsp()->lastp() : nullptr;
        বাবে (ধ্রুৱ Ast_গ্ৰন্থি* nodep = lastp; nodep; nodep = nodep->prevp()) {
            ধ্রুৱ AstVar* ধ্রুৱ varp = VN_CAST(nodep, Var);
            যদি (varp && isExplicitlyConstructed(varp)) {
                addVarCtorDtorStmts(m_dtorStmts, varp, সঁচা);
            }
        }

        // Split into sub-functions if needed
        ধ্রুৱ std::string modName = EmitCUtil::prefixNameProtect(modp);
        std::tie(m_ctorSplitDecl, m_dtorSplitDecl) = emitSplitCtorDtor(
            m_ctorStmts, m_dtorStmts, 0, modName, modName + "* vlSelf", "vlSelf", মিছা,
            "Design implementation internals", [এতত্]() { emitFileHeader(); });
    }
    শূন্য emitCtorDtorImp(ধ্রুৱ Ast_গ্ৰন্থি_Module* modp) {
        ধ্রুৱ std::string modName = EmitCUtil::prefixNameProtect(modp);

        // ctor
        যদি (modp->ctorVarReset()) {
            m_lazyDecls.emit("\nvoid " + modName + "__", protect("_ctor_var_reset"),
                             "(" + modName + "* vlSelf);\n");
        }

        যদি (!m_ctorSplitDecl.empty()) puts("\n" + m_ctorSplitDecl + "\n");
        puts("\nvoid " + modName + "__ctor(" + modName + "* vlSelf, " + EmitCUtil::symClassName()
             + "* symsp, const char* namep) {\n");
        puts("vlSelf->vlSymsp = symsp;\n");
        যদি (modp->is_শীৰ্ষ()) {
            puts("vlSelf->vlNamep = strdup(namep);\n");
        } অন্যথা {
            puts("vlSelf->vlNamep = strdup(Verilated::catName(symsp->name(), namep));\n");
        }
        putsDecoration(modp, "// Construct members\n");
        বাবে (ধ্রুৱ TextStmts::Stmt& stmt : m_ctorStmts) puts(stmt.m_text + "\n");

        putsDecoration(modp, "// Reset structure values\n");
        যদি (modp->ctorVarReset()) {
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
        যদি (!m_dtorSplitDecl.empty()) puts("\n" + m_dtorSplitDecl + "\n");
        puts("\nvoid " + modName + "__dtor(" + modName + "* vlSelf) {\n");
        putsDecoration(modp, "// Destroy members\n");
        বাবে (ধ্রুৱ TextStmts::Stmt& stmt : m_dtorStmts) puts(stmt.m_text + "\n");
        puts("VL_DO_DANGLING(std::free(const_cast<char*>(vlSelf->vlNamep)), "
             "vlSelf->vlNamep);\n");
        puts("}\n");

        // C++ destructor
        puts("\n" + modName + "::~" + modName + "() {\n");
        emitSystemCSection(modp, VSystemCSectionType::DTOR);
        puts(modName + "__dtor(this);\n");
        puts("}\n");
    }

    শূন্য emitConfigureImp(ধ্রুৱ Ast_গ্ৰন্থি_Module* modp) {
        ধ্রুৱ string modName = EmitCUtil::prefixNameProtect(modp);

        যদি (v3Global.opt.coverage()) {
            puts("\n");
            m_lazyDecls.emit("void " + modName + "__", protect("_configure_coverage"),
                             "(" + modName + "* vlSelf, bool first);");
        }

        যদি (v3Global.opt.coverage()) {
            puts("\nvoid " + modName + "::" + protect("__Vconfigure") + "(bool first) {\n");
            puts("(void)first;  // Prevent unused variable warning\n");
            puts(modName + "__" + protect("_configure_coverage") + "(this, first);\n");
            puts("}\n");
        }
    }
    শূন্য emitCoverageImp() {
        // Rather than putting out VL_COVER_INSERT calls directly, we do it via this
        // function. This gets around gcc slowness constructing all of the template
        // arguments.
        যদি (v3Global.opt.coverage()) {
            puts("\n// Coverage\n");
            puts("void " + EmitCUtil::prefixNameProtect(m_modp) + "::__vlCoverInsert(");
            puts(v3Global.opt.threads() > 1 ? "std::atomic<uint32_t>" : "uint32_t");
            puts("* countp, bool enable, bool localCounter, const char* filenamep, int lineno, "
                 "int column,\n");
            puts("const char* hierp, const char* pagep, const char* commentp, const char* "
                 "linescovp,\n");
            puts("const char* fsmVarp, const char* fsmFromp, const char* fsmTop, const char* "
                 "fsmTagp) {\n");
            যদি (v3Global.opt.threads() > 1) {
                puts("assert(sizeof(uint32_t) == sizeof(std::atomic<uint32_t>));\n");
                puts("uint32_t* count32p = reinterpret_cast<uint32_t*>(countp);\n");
            } অন্যথা {
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
        যদি (v3Global.opt.coverageToggle()) {
            puts("\n// Toggle Coverage\n");
            puts("void " + EmitCUtil::prefixNameProtect(m_modp) + "::__vlCoverToggleInsert(");
            puts("int begin, int end, bool ranged, ");
            puts(v3Global.opt.threads() > 1 ? "std::atomic<uint32_t>" : "uint32_t");
            puts("* countp, bool enable, bool localCounter, const char* filenamep, int lineno, "
                 "int column,\n");
            puts("const char* hierp, const char* pagep, const char* commentp) {\n");
            যদি (v3Global.opt.threads() > 1) {
                puts("assert(sizeof(uint32_t) == sizeof(std::atomic<uint32_t>));\n");
            }
            puts("int step = (end >= begin) ? 1 : -1;\n");
            // range is inclusive
            puts("for (int i = begin; i != end + step; i += step) {\n");
            puts("for (int j = 0; j < 2; j++) {\n");
            যদি (v3Global.opt.threads() > 1) {
                puts("uint32_t* count32p = reinterpret_cast<uint32_t*>(countp);\n");
            } অন্যথা {
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
    শূন্য emitSavableImp(ধ্রুৱ Ast_গ্ৰন্থি_Module* modp) {
        যদি (v3Global.opt.savable()) {
            puts("\n// Savable\n");
            বাবে (পূর্ণাঙ্ক de = 0; de < 2; ++de) {
                ধ্রুৱ string classname = de ? "VerilatedDeserialize" : "VerilatedSerialize";
                ধ্রুৱ string funcname = de ? "__Vdeserialize" : "__Vserialize";
                ধ্রুৱ string op = de ? ">>" : "<<";
                // NOLINTNEXTLINE(performance-inefficient-string-concatenation)
                putns(modp, "void " + EmitCUtil::prefixNameProtect(modp) + "::" + protect(funcname)
                                + "(" + classname + "& os) {\n");
                // Place a computed checksum to ensure proper structure save/restore formatting
                // OK if this hash includes some things we won't dump, since
                // just looking for loading the wrong model
                VHashSha512 hash;
                বাবে (Ast_গ্ৰন্থি* nodep = modp->stmtsp(); nodep; nodep = nodep->nextp()) {
                    যদি (ধ্রুৱ AstVar* ধ্রুৱ varp = VN_CAST(nodep, Var)) {
                        hash.ঢোকাও(varp->name());
                        hash.ঢোকাও(varp->dtypep()->width());
                    }
                }
                ofp()->printf("uint64_t __Vcheckval = 0x%" PRIx64 "ULL;\n",
                              static_cast<uint64_t>(hash.digestUInt64()));
                যদি (de) {
                    puts("os.readAssert(__Vcheckval);\n");
                } অন্যথা {
                    puts("os << __Vcheckval;\n");
                }

                // Save context
                // If multiple models save the same context we'll save it multiple
                // times, but is harmless, and doing it otherwise would break
                // backwards compatibility.
                puts("os " + op + " vlSymsp->_vm_contextp__;\n");

                // Save all members
                বাবে (Ast_গ্ৰন্থি* nodep = modp->stmtsp(); nodep; nodep = nodep->nextp()) {
                    যদি (ধ্রুৱ AstVar* ধ্রুৱ varp = VN_CAST(nodep, Var)) {
                        যদি (varp->isIO() && modp->is_শীৰ্ষ() && optSystemC()) {
                            // System C top I/O doesn't need loading, as the
                            // lower level subinst code does it.
                        } অন্যথা যদি (varp->isParam()) {
                        } অন্যথা যদি (VN_IS(varp->dtypep(), NBACommitQueueDType)) {
                        } অন্যথা {
                            পূর্ণাঙ্ক vects = 0;
                            Ast_গ্ৰন্থি_DType* elementp = varp->dtypeSkipRefp();
                            বাবে (AstUnpackArrayDType* arrayp = VN_CAST(elementp, UnpackArrayDType);
                                 arrayp; arrayp = VN_CAST(elementp, UnpackArrayDType)) {
                                ধ্রুৱ পূর্ণাঙ্ক vecnum = vects++;
                                UASSERT_OBJ(arrayp->hi() >= arrayp->lo(), varp,
                                            "Should have swapped msb & lsb earlier.");
                                ধ্রুৱ string ivar = "__Vi"s + cvtToStr(vecnum);
                                puts("for (int __Vi" + cvtToStr(vecnum) + " = " + cvtToStr(0));
                                puts("; " + ivar + " < " + cvtToStr(arrayp->elementsConst()));
                                puts("; ++" + ivar + ") {\n");
                                elementp = arrayp->subDTypep()->skipRefp();
                            }
                            ধ্রুৱ AstBasicDType* ধ্রুৱ basicp = elementp->basicp();
                            // Do not save MTask state, only matters within an evaluation
                            যদি (basicp && basicp->keyword().isMTaskState()) অনুৱর্ত;
                            // Want to detect types that are represented as arrays
                            // (i.e. packed types of more than 64 bits).
                            যদি (elementp->isWide()
                                && !(basicp && basicp->keyword() == VBasicDTypeKwd::STRING)) {
                                ধ্রুৱ পূর্ণাঙ্ক vecnum = vects++;
                                ধ্রুৱ string ivar = "__Vi"s + cvtToStr(vecnum);
                                puts("for (int __Vi" + cvtToStr(vecnum) + " = " + cvtToStr(0));
                                puts("; " + ivar + " < " + cvtToStr(elementp->width_শব্দগুলি()));
                                puts("; ++" + ivar + ") {\n");
                            }
                            putns(varp, "os" + op + varName(varp));
                            বাবে (পূর্ণাঙ্ক v = 0; v < vects; ++v) puts("[__Vi" + cvtToStr(v) + "]");
                            puts(";\n");
                            বাবে (পূর্ণাঙ্ক v = 0; v < vects; ++v) puts("}\n");
                        }
                    }
                }

                puts("}\n");
            }
        }
    }
    // Predicate to check if we actually need to emit anything into the common implementation file.
    // Used to avoid creating empty output files.
    তার্কিক hasCommonImp(ধ্রুৱ Ast_গ্ৰন্থি_Module* modp) ধ্রুৱ {
        // Nothing to emit if no module!
        যদি (!modp) ঘূৰাই মিছা;
        // We always need the slow file
        যদি (m_slow) ঘূৰাই সঁচা;
        // The fast file is only required when we have `systemc_implementation nodes
        বাবে (ধ্রুৱ AstSystemCSection* scSectionp = modp->scSectionsp(); scSectionp;
             scSectionp = VN_AS(scSectionp->nextp(), SystemCSection)) {
            যদি (scSectionp->sectionType() == VSystemCSectionType::IMP) ঘূৰাই সঁচা;
        }
        ঘূৰাই মিছা;
    }
    // Actually emit common implementation contents for given AstNodeModule
    শূন্য doCommonImp(ধ্রুৱ Ast_গ্ৰন্থি_Module* modp) {
        যদি (m_slow) {
            যদি (!VN_IS(modp, Class)) {
                emitParamDefns(modp);
                emitCtorDtorImp(modp);
                emitConfigureImp(modp);
                emitCoverageImp();
            }
            emitSavableImp(modp);
        } অন্যথা {
            // From `systemc_implementation
            emitSystemCSection(modp, VSystemCSectionType::IMP);
        }
    }
    শূন্য emitCommonImp(ধ্রুৱ Ast_গ্ৰন্থি_Module* modp) {
        ধ্রুৱ AstClass* ধ্রুৱ classp
            = VN_IS(modp, ClassPackage) ? VN_AS(modp, ClassPackage)->classp() : nullptr;

        যদি (hasCommonImp(modp) || hasCommonImp(classp)) {
            // Create ctor/dtor statements. If these need splitting, they will be emitted
            // during construction so do this before opening the output file.
            যদি (m_slow && !VN_IS(modp, Class)) createCtorDtorStmts(modp);

            openNextOutputFile(m_fileBaseName);

            doCommonImp(modp);
            যদি (classp) {
                VL_RESTORER(m_modp);
                m_modp = classp;
                doCommonImp(classp);
            }

            closeOutputFile();
        }
    }
    শূন্য emitCFuncImp(ধ্রুৱ Ast_গ্ৰন্থি_Module* modp) {
        // Functions to be emitted here
        std::vector<AstCFunc*> funcps;

        ধ্রুৱ auto gather = [এতত্, &funcps](ধ্রুৱ Ast_গ্ৰন্থি_Module* modp) {
            বাবে (Ast_গ্ৰন্থি* nodep = modp->stmtsp(); nodep; nodep = nodep->nextp()) {
                AstCFunc* ধ্রুৱ funcp = VN_CAST(nodep, CFunc);
                যদি (!funcp) অনুৱর্ত;
                // TRACE_* and DPI handled elsewhere
                যদি (funcp->isTrace()) অনুৱর্ত;
                যদি (funcp->dpiImportPrototype()) অনুৱর্ত;
                যদি (funcp->dpiExportDispatcher()) অনুৱর্ত;
                যদি (funcp->slow() != m_slow) অনুৱর্ত;
                funcps.ঠেলো_back(funcp);
            }
        };

        gather(modp);
        VL_RESTORER(m_classOrPackage);
        যদি (ধ্রুৱ AstClassPackage* ধ্রুৱ packagep = VN_CAST(modp, ClassPackage)) {
            m_classOrPackage = packagep;
            gather(packagep->classp());
        }

        // Do not create empty files
        যদি (funcps.empty()) ঘূৰাই;

        // Open output file
        openNextOutputFile(m_uniqueNames.get(m_fileBaseName));
        // Emit all functions
        বাবে (AstCFunc* ধ্রুৱ funcp : funcps) {
            VL_RESTORER(m_modp);
            m_modp = EmitCParentModule::get(funcp);
            iterateConst(funcp);
        }
        // Close output file
        closeOutputFile();
    }

    // VISITORS
    শূন্য visit(AstCFunc* nodep) override {
        যদি (splitNeeded()) {
            // Splitting file, so using parallel build.
            v3Global.useParallelBuild(সঁচা);
            // Close old file
            closeOutputFile();
            // Open a new file
            openNextOutputFile(m_uniqueNames.get(m_fileBaseName));
        }

        EmitCFunc::visit(nodep);
    }

    explicit EmitCImp(ধ্রুৱ Ast_গ্ৰন্থি_Module* modp, তার্কিক slow)
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
    ~EmitCImp() override = স্ৱভাৱ;

সার্ৱজনিক:
    স্থির std::vector<AstCFile*> main(ধ্রুৱ Ast_গ্ৰন্থি_Module* modp, তার্কিক slow) VL_MT_STABLE {
        EmitCImp emitCImp{modp, slow};
        ঘূৰাই emitCImp.getAndClearCfileps();
    }
};

//######################################################################
// Tracing routines

// Trace type descriptors go in a different file as it needs to be written in
// parallel with the actual trace function source files
শ্ৰেণী EmitCTraceTypes final : সার্ৱজনিক EmitCFunc {
    // NODE STATE/TYPES
    // None allowed to support threaded emitting

    // STATE
    পূর্ণাঙ্ক m_enum_অঙ্ক = 0;  // Enumeration number (whole netlist)
    std::unordered_map<Ast_গ্ৰন্থি*, পূর্ণাঙ্ক> m_enum_অঙ্ক_Map;  // EnumDType to enumeration number
    পূর্ণাঙ্ক m_traceTypeSubs = 0;  // Number of trace type declaration sub-functions
    V3UniqueNames m_uniqueNames;  // Generates unique file names
    ধ্রুৱ std::string m_fileBaseName = EmitCUtil::শীৰ্ষ_Class_Name() + "_" + protect("_TraceDecls");
    // This one uses CSplitTrace for file splitting, which is incorrect but historically accurates
    ধ্রুৱ size_t m_splitLimit = v3Global.opt.outputSplitCTrace()
                                    ? static_cast<size_t>(v3Global.opt.outputSplitCTrace())
                                    : std::numeric_limits<size_t>::max();

    শূন্য openNextOutputFile() {
        openNewOutputSourceFile(m_uniqueNames.get(m_fileBaseName), সঁচা, সঁচা,
                                "Tracing declarations");
        puts("\n");
        বাবে (ধ্রুৱ std::string& base : v3Global.opt.traceSourceLangs()) {
            puts("#include \"" + base + ".h\"\n");
        }
        puts("\n");
        puts("\nvoid " + EmitCUtil::prefixNameProtect(m_modp) + "__"
             + protect("traceDeclTypesSub" + std::to_string(m_traceTypeSubs++)) + "("
             + v3Global.opt.traceClassBase() + "* tracep) {\n");
    }

সার্ৱজনিক:
    // METHODS
    পূর্ণাঙ্ক get_Enum_Map_অঙ্ক(AstEnumDType* nodep) {
        পূর্ণাঙ্ক& enumNumr = m_enum_অঙ্ক_Map[nodep];
        যদি (!enumNumr) {
            যদি (splitNeeded(m_splitLimit)) {
                // Splitting file, so using parallel build.
                v3Global.useParallelBuild(সঁচা);
                puts("}\n");
                closeOutputFile();
                openNextOutputFile();
            }

            enumNumr = ++m_enum_অঙ্ক;
            পূর্ণাঙ্ক nvals = 0;
            puts("{\n");
            putns(nodep, "const char* " + protect("__VenumItemNames") + "[]\n");
            puts("= {");
            বাবে (AstEnumItem* itemp = nodep->itemsp(); itemp;
                 itemp = VN_AS(itemp->nextp(), EnumItem)) {
                যদি (++nvals > 1) puts(", ");
                putbs("\"" + itemp->prettyName() + "\"");
            }
            puts("};\n");
            nvals = 0;
            puts("const char* " + protect("__VenumItemValues") + "[]\n");
            puts("= {");
            বাবে (AstEnumItem* itemp = nodep->itemsp(); itemp;
                 itemp = VN_AS(itemp->nextp(), EnumItem)) {
                AstConst* ধ্রুৱ constp = VN_AS(itemp->valuep(), Const);
                যদি (++nvals > 1) puts(", ");
                putbs("\"" + constp->অঙ্ক().displayed(nodep, "%0b") + "\"");
            }
            puts("};\n");
            puts("tracep->declDTypeEnum(" + std::to_string(enumNumr) + ", \"" + nodep->prettyName()
                 + "\", " + std::to_string(nvals) + ", " + std::to_string(nodep->widthMin()) + ", "
                 + protect("__VenumItemNames") + ", " + protect("__VenumItemValues") + ");\n");
            puts("}\n");
            splitSizeInc(Ast_গ্ৰন্থি::INSTR_গুনতি_CALL);
        }
        ঘূৰাই enumNumr;
    }

    // Close output file
    শূন্য finalize() {
        // Close function definition
        puts("}\n");

        ধ্রুৱ std::string modName = EmitCUtil::prefixNameProtect(m_modp);
        ধ্রুৱ std::string args = v3Global.opt.traceClassBase() + "* tracep";

        // Forward declarations for subs in other files
        বাবে (পূর্ণাঙ্ক i = 0; i < m_traceTypeSubs - 1; ++i) {
            puts("void " + modName + "__" + protect("traceDeclTypesSub" + std::to_string(i)) + "("
                 + args + ");\n");
        }

        // Create top level trace_decl_types function and call each sub-function
        puts("\nvoid " + modName + "__" + protect("trace_decl_types") + "(" + args + ") {\n");
        বাবে (পূর্ণাঙ্ক i = 0; i < m_traceTypeSubs; ++i) {
            puts(modName + "__" + protect("traceDeclTypesSub" + std::to_string(i))
                 + "(tracep);\n");
        }
        puts("}\n");

        closeOutputFile();
    }

    EmitCTraceTypes() {
        m_modp = v3Global.rootp()->শীৰ্ষ_Modulep();
        openNextOutputFile();
    }
    ~EmitCTraceTypes() override = স্ৱভাৱ;
};

শ্ৰেণী EmitCTrace final : সার্ৱজনিক EmitCFunc {
    // NODE STATE/TYPES
    // None allowed to support threaded emitting

    // MEMBERS
    ধ্রুৱ তার্কিক m_slow;  // Making slow file
    ধ্রুৱ std::unique_ptr<EmitCTraceTypes> m_emitTypesp{m_slow ? নূতন EmitCTraceTypes{} : nullptr};
    V3UniqueNames m_uniqueNames;  // Generates unique file names
    ধ্রুৱ std::string m_fileBaseName = EmitCUtil::শীৰ্ষ_Class_Name() + "_" + protect("_Trace");

    // METHODS
    শূন্য openNextOutputFile() {
        openNewOutputSourceFile(m_uniqueNames.get(m_fileBaseName), m_slow, সঁচা,
                                "Tracing implementation internals");
        puts("\n");
        বাবে (ধ্রুৱ std::string& base : v3Global.opt.traceSourceLangs()) {
            puts("#include \"" + base + ".h\"\n");
        }
        puts("#include \"" + EmitCUtil::symClassName() + ".h\"\n");
        puts("\n");
        // Need to emit new lazy declarations
        m_lazyDecls.reset();
    }

    তার্কিক emitTraceIsScBv(ধ্রুৱ AstTraceInc* nodep) {
        ধ্রুৱ AstVarRef* ধ্রুৱ varrefp = VN_CAST(nodep->declp()->valuep(), VarRef);
        যদি (!varrefp) ঘূৰাই মিছা;
        ধ্রুৱ AstVar* ধ্রুৱ varp = varrefp->varp();
        ঘূৰাই varp->isSc() && varp->isScBv();
    }

    তার্কিক emitTraceIsScBigUint(ধ্রুৱ AstTraceInc* nodep) {
        ধ্রুৱ AstVarRef* ধ্রুৱ varrefp = VN_CAST(nodep->declp()->valuep(), VarRef);
        যদি (!varrefp) ঘূৰাই মিছা;
        ধ্রুৱ AstVar* ধ্রুৱ varp = varrefp->varp();
        ঘূৰাই varp->isSc() && varp->isScBigUint();
    }

    তার্কিক emitTraceIsScUint(ধ্রুৱ AstTraceInc* nodep) {
        ধ্রুৱ AstVarRef* ধ্রুৱ varrefp = VN_CAST(nodep->declp()->valuep(), VarRef);
        যদি (!varrefp) ঘূৰাই মিছা;
        ধ্রুৱ AstVar* ধ্রুৱ varp = varrefp->varp();
        ঘূৰাই varp->isSc() && (varp->isScUint() || varp->isScUintBool());
    }

    শূন্য emitTraceInitOne(ধ্রুৱ AstTraceDecl* nodep, পূর্ণাঙ্ক enum_অঙ্ক) {
        std::string direction;
        direction = nodep->declDirection().traceSigDirection();

        AstCCall* ধ্রুৱ callp = nodep->dtypeCallp();
        যদি (callp) {
            callp->argTypes(callp->argTypes() + ", " + cvtToStr(nodep->fidx()) + ", c+"
                            + cvtToStr(nodep->code()) + ", " + direction);
            ঘূৰাই;
        }

        যদি (nodep->dtypep()->basicp()->isDouble()) {
            puts("VL_TRACE_DECL_DOUBLE");
        } অন্যথা যদি (nodep->isWide()) {
            puts("VL_TRACE_DECL_WIDE");
        } অন্যথা যদি (nodep->isQuad()) {
            puts("VL_TRACE_DECL_QUAD");
        } অন্যথা যদি (nodep->bitRange().ranged()) {
            puts("VL_TRACE_DECL_BUS");
        } অন্যথা যদি (nodep->dtypep()->basicp()->isEvent()) {
            puts("VL_TRACE_DECL_EVENT");
        } অন্যথা {
            puts("VL_TRACE_DECL_BIT");
        }

        যদি (nodep->arrayRange().ranged()) {
            puts("_ARRAY(tracep");
        } অন্যথা {
            puts("(tracep");
        }

        // Code
        puts(",c+" + cvtToStr(nodep->code()));
        যদি (nodep->arrayRange().ranged()) puts("+i*" + cvtToStr(nodep->width_শব্দগুলি()));

        // Function index
        puts(",");
        যদি (nodep->inDtypeFunc()) {
            puts("fidx");
        } অন্যথা {
            puts(cvtToStr(nodep->fidx()));
        }

        // Name
        puts(",");
        putsQuoted(VIdProtect::protect_শব্দগুলি_If(nodep->showname(), nodep->protect()));

        // Enum number
        puts("," + cvtToStr(enum_অঙ্ক));

        // Direction
        যদি (nodep->inDtypeFunc()) {
            puts(", direction");
        } অন্যথা {
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
        যদি (nodep->arrayRange().ranged()) {
            যদি (nodep->arrayRange().ascending()) {
                puts(", (i + " + std::to_string(nodep->arrayRange().lo()) + ")");
            } অন্যথা {
                puts(", (" + std::to_string(nodep->arrayRange().hi()) + " - i)");
            }
        }

        // Bit range
        যদি (!nodep->dtypep()->basicp()->isDouble() && nodep->bitRange().ranged()) {
            puts(", " + cvtToStr(nodep->bitRange().বাম()) + ","
                 + cvtToStr(nodep->bitRange().ডান()));
        }

        //
        puts(");");
    }

    পূর্ণাঙ্ক emitTraceDeclDType(Ast_গ্ৰন্থি_DType* nodep) {
        // Return enum number or -1 for none
        যদি (v3Global.opt.traceEnabledFst()) {
            // Skip over refs-to-refs, but stop before final ref so can get data type name
            // Alternatively back in V3Width we could push enum names from upper typedefs
            যদি (AstEnumDType* ধ্রুৱ enump = VN_CAST(nodep->skipRefToEnump(), EnumDType)) {
                ঘূৰাই m_emitTypesp->get_Enum_Map_অঙ্ক(enump);
            }
        }
        ঘূৰাই -1;
    }

    শূন্য emitTraceChangeOne(AstTraceInc* nodep, পূর্ণাঙ্ক arrayindex) {
        // Note: Both VTraceType::CHANGE and VTraceType::FULL use the 'full' methods
        ধ্রুৱ std::string func = nodep->traceType() == VTraceType::CHANGE ? "chg" : "full";
        তার্কিক emitWidth = সঁচা;
        string stype;
        যদি (nodep->dtypep()->basicp()->isDouble()) {
            stype = "Double";
            emitWidth = মিছা;
        } অন্যথা যদি (nodep->isWide() || emitTraceIsScBv(nodep) || emitTraceIsScBigUint(nodep)) {
            stype = "WData";
        } অন্যথা যদি (nodep->isQuad()) {
            stype = "QData";
        } অন্যথা যদি (nodep->declp()->widthMin() > 16) {
            stype = "IData";
        } অন্যথা যদি (nodep->declp()->widthMin() > 8) {
            stype = "SData";
        } অন্যথা যদি (nodep->declp()->widthMin() > 1) {
            stype = "CData";
        } অন্যথা যদি (nodep->dtypep()->basicp()->isEvent()) {
            stype = "Event";
            emitWidth = মিছা;
        } অন্যথা {
            stype = "Bit";
            emitWidth = মিছা;
        }
        putns(nodep, "bufp->" + func + stype);

        ধ্রুৱ uint32_t offset = (arrayindex < 0) ? 0 : (arrayindex * nodep->declp()->width_শব্দগুলি());
        ধ্রুৱ uint32_t code = nodep->declp()->code() + offset;
        // Note: Both VTraceType::CHANGE and VTraceType::FULL use the 'full' methods
        puts("(oldp+");
        puts(cvtToStr(code - nodep->baseCode()));
        puts(",");
        ধ্রুৱ VNumRange& arrayRange = nodep->declp()->arrayRange();
        যদি (arrayRange.ranged() && !arrayRange.ascending()) {
            arrayindex = arrayRange.elements() - 1 - arrayindex;
        }
        emit_Trace_মান(nodep, arrayindex);
        যদি (emitWidth) puts("," + cvtToStr(nodep->declp()->widthMin()));
        puts(");\n");
    }

    শূন্য emit_Trace_মান(ধ্রুৱ AstTraceInc* nodep, পূর্ণাঙ্ক arrayindex) {
        যদি (AstVarRef* ধ্রুৱ varrefp = VN_CAST(nodep->valuep(), VarRef)) {
            ধ্রুৱ AstVar* ধ্রুৱ varp = varrefp->varp();
            যদি (varp->isEvent()) puts("&");
            puts("(");
            যদি (emitTraceIsScBigUint(nodep)) {
                puts("(uint32_t*)");
            } অন্যথা যদি (emitTraceIsScBv(nodep)) {
                puts("VL_SC_BV_DATAP(");
            }
            iterateConst(varrefp);  // Put var name out
            emitTraceIndex(nodep, arrayindex);
            যদি (varp->isSc()) puts(".read()");
            যদি (emitTraceIsScUint(nodep)) {
                puts(nodep->isQuad() ? ".to_uint64()" : ".to_uint()");
            } অন্যথা যদি (emitTraceIsScBigUint(nodep)) {
                puts(".get_raw()");
            } অন্যথা যদি (emitTraceIsScBv(nodep)) {
                puts(")");
            }
            puts(")");
        } অন্যথা {
            puts("(");
            iterateConst(nodep->valuep());
            emitTraceIndex(nodep, arrayindex);
            puts(")");
        }
    }

    শূন্য emitTraceIndex(ধ্রুৱ AstTraceInc* ধ্রুৱ nodep, পূর্ণাঙ্ক arrayindex) {
        // Tracing only supports 1D arrays
        যদি (nodep->declp()->arrayRange().ranged()) {
            যদি (arrayindex == -2) {
                puts("[i]");
            } অন্যথা যদি (arrayindex == -1) {
                puts("[0]");
            } অন্যথা {
                puts("[" + cvtToStr(arrayindex) + "]");
            }
        }
    }

    // VISITORS
    using EmitCFunc::visit;  // Suppress hidden overloaded virtual function warning
    শূন্য visit(AstCFunc* nodep) override {
        যদি (!nodep->isTrace()) ঘূৰাই;
        যদি (nodep->slow() != m_slow) ঘূৰাই;

        যদি (splitNeeded()) {
            // Splitting file, so using parallel build.
            v3Global.useParallelBuild(সঁচা);
            // Close old file
            closeOutputFile();
            // Open a new file
            openNextOutputFile();
        }

        EmitCFunc::visit(nodep);
    }
    শূন্য visit(Ast_Trace_ঠেলো_Prefix* nodep) override {
        putns(nodep, "VL_TRACE_PUSH_PREFIX(tracep, ");
        যদি (nodep->quotedPrefix()) {
            putsQuoted(VIdProtect::protect_শব্দগুলি_If(nodep->prefix(), nodep->protect()));
        } অন্যথা {
            puts(nodep->prefix());
        }
        puts(", VerilatedTracePrefixType::");
        puts(nodep->prefixType().ascii());
        puts(", " + std::to_string(nodep->বাম()));
        puts(", " + std::to_string(nodep->ডান()));
        puts(");\n");
    }
    শূন্য visit(Ast_Trace_বেৰ_কৰো_Prefix* nodep) override {  //
        putns(nodep, "VL_TRACE_POP_PREFIX(tracep);\n");
    }
    শূন্য visit(AstTraceDecl* nodep) override {
        ধ্রুৱ পূর্ণাঙ্ক enum_অঙ্ক = emitTraceDeclDType(nodep->dtypep());
        putns(nodep, "");
        যদি (nodep->arrayRange().ranged()) {
            puts("for (int i = 0; i < " + cvtToStr(nodep->arrayRange().elements()) + "; ++i) {\n");
            emitTraceInitOne(nodep, enum_অঙ্ক);
            puts("\n}\n");
        } অন্যথা {
            emitTraceInitOne(nodep, enum_অঙ্ক);
            puts("\n");
        }
    }
    শূন্য visit(AstTraceInc* nodep) override {
        যদি (nodep->declp()->arrayRange().ranged()) {
            // It traces faster if we unroll the loop
            বাবে (পূর্ণাঙ্ক i = 0; i < nodep->declp()->arrayRange().elements(); i++) {
                emitTraceChangeOne(nodep, i);
            }
        } অন্যথা {
            emitTraceChangeOne(nodep, -1);
        }
    }

    explicit EmitCTrace(তার্কিক slow)
        : m_slow{slow} {
        m_modp = v3Global.rootp()->শীৰ্ষ_Modulep();
        // Open output file
        openNextOutputFile();
        // Emit functions
        বাবে (Ast_গ্ৰন্থি* nodep = m_modp->stmtsp(); nodep; nodep = nodep->nextp()) {
            যদি (AstCFunc* ধ্রুৱ funcp = VN_CAST(nodep, CFunc)) iterateConst(funcp);
        }
        // Close output file
        closeOutputFile();
        যদি (m_slow) m_emitTypesp->finalize();
    }
    ~EmitCTrace() override = স্ৱভাৱ;

সার্ৱজনিক:
    স্থির std::vector<AstCFile*> main(তার্কিক slow) VL_MT_STABLE {
        EmitCTrace emitCTrace{slow};
        std::vector<AstCFile*> cfileps = emitCTrace.getAndClearCfileps();
        যদি (slow) {
            বাবে (AstCFile* ধ্রুৱ cfilep : emitCTrace.m_emitTypesp->getAndClearCfileps()) {
                cfileps.emplace_back(cfilep);
            }
        }
        ঘূৰাই cfileps;
    }
};

//######################################################################
// Existing AstCFile emitter

শ্ৰেণী EmitCFile final : সার্ৱজনিক EmitCFunc {
    explicit EmitCFile(AstCFile* cfilep) {
        openOutputFile(cfilep, "Generated C++");
        iterateConst(cfilep->tblockp());
        closeOutputFile();
    }
    ~EmitCFile() override = স্ৱভাৱ;

সার্ৱজনিক:
    স্থির শূন্য main(AstCFile* cfilep) VL_MT_STABLE {
        যদি (!cfilep->tblockp()) ঘূৰাই;
        EmitCFile{cfilep};
    }
};

//######################################################################
// EmitC class functions

শূন্য V3EmitC::emitcImp() {
    UINFO(2, __FUNCTION__ << ":");
    std::list<std::vector<AstCFile*>> cfiles;
    {
        // Make parent module pointers available.
        ধ্রুৱ EmitCParentModule emitCParentModule;
        V3ThreadScope threadScope;

        // Process each module in turn
        বাবে (ধ্রুৱ Ast_গ্ৰন্থি* nodep = v3Global.rootp()->modulesp(); nodep; nodep = nodep->nextp()) {
            যদি (VN_IS(nodep, Class)) অনুৱর্ত;  // Imped with ClassPackage
            ধ্রুৱ Ast_গ্ৰন্থি_Module* ধ্রুৱ modp = VN_AS(nodep, গ্ৰন্থি_Module);
            যদি (modp->isConstPool()) অনুৱর্ত;  // Emitted by V3EmitCConstPool
            cfiles.emplace_back();
            std::vector<AstCFile*>& slow = cfiles.back();
            threadScope.enqueue([modp, &slow] { slow = EmitCImp::main(modp, /* slow: */ সঁচা); });
            cfiles.emplace_back();
            std::vector<AstCFile*>& fast = cfiles.back();
            threadScope.enqueue([modp, &fast] { fast = EmitCImp::main(modp, /* slow: */ মিছা); });
        }

        // Emit trace routines (currently they can only exist in the top module)
        যদি (v3Global.opt.trace() && !v3Global.opt.lintOnly()) {
            cfiles.emplace_back();
            std::vector<AstCFile*>& slow = cfiles.back();
            threadScope.enqueue([&slow] { slow = EmitCTrace::main(/* slow: */ সঁচা); });
            cfiles.emplace_back();
            std::vector<AstCFile*>& fast = cfiles.back();
            threadScope.enqueue([&fast] { fast = EmitCTrace::main(/* slow: */ মিছা); });
        }
    }
    // Add files to netlist
    বাবে (ধ্রুৱ std::vector<AstCFile*>& cfileps : cfiles) {
        বাবে (AstCFile* ধ্রুৱ cfilep : cfileps) v3Global.rootp()->addFilesp(cfilep);
    }
}

শূন্য V3EmitC::emitcFiles() {
    UINFO(2, __FUNCTION__ << ":");
    বাবে (Ast_গ্ৰন্থি_File *filep = v3Global.rootp()->filesp(), *nextp; filep; filep = nextp) {
        nextp = VN_AS(filep->nextp(), গ্ৰন্থি_File);
        যদি (AstCFile* ধ্রুৱ cfilep = VN_CAST(filep, CFile)) EmitCFile::main(cfilep);
    }
}
