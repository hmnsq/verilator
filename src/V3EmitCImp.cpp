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

#অন্তর্ভাব "V3PchAstMT.h"

#অন্তর্ভাব "V3EmitC.h"
#অন্তর্ভাব "V3EmitCFunc.h"
#অন্তর্ভাব "V3ThreadPool.h"
#অন্তর্ভাব "V3UniqueNames.h"

#অন্তর্ভাব <algorithm>
#অন্তর্ভাব <map>
#অন্তর্ভাব <set>
#অন্তর্ভাব <tuple>
#অন্তর্ভাব <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Internal EmitC implementation

শ্রেণী EmitCImp final : সর্বজনীন EmitCFunc {
    // MEMBERS
    // Base module (For non-classes, same as m_modp. For classes, it's the ClassPackage.)
    ধ্রুবক Ast_গ্রন্থি_Module* ধ্রুবক m_fileModp;
    ধ্রুবক যৌক্তিক m_slow;  // Creating __Slow file
    V3UniqueNames m_uniqueNames;  // Generates unique file names
    ধ্রুবক std::string m_fileBaseName = EmitCUtil::prefixNameProtect(m_fileModp);
    TextStmts m_ctorStmts;  // Statements constructing members in 'ctor'
    TextStmts m_dtorStmts;  // Statements destroying members in 'dtor'
    std::string m_ctorSplitDecl;  // Declaration of sub-function 'ctor' was split into, if any
    std::string m_dtorSplitDecl;  // Declaration of sub-function 'dtor' was split into, if any

    // METHODS
    শূন্য open_পরবর্তী_Output_File(ধ্রুবক std::string& fileName) {
        openNewOutputSourceFile(fileName, m_slow, মিথ্যা, "Design implementation internals");
        emitFileHeader();
    }
    শূন্য emitFileHeader() {
        puts("// See " + EmitCUtil::শীর্ষ_Class_Name() + ".h for the primary calling header\n");
        puts("\n");
        puts("#include \"" + EmitCUtil::pchClassName() + ".h\"\n");
        emitSystemCSection(m_fileModp, VSystemCSectionType::IMP_HDR);
        // Need to emit new lazy declarations
        m_lazyDecls.reset();
    }

    শূন্য emitParamDefns(ধ্রুবক Ast_গ্রন্থি_Module* modp) {
        যৌক্তিক first = সত্য;
        জন্য (ধ্রুবক Ast_গ্রন্থি* nodep = modp->stmtsp(); nodep; nodep = nodep->nextp()) {
            যদি (ধ্রুবক AstVar* ধ্রুবক varp = VN_CAST(nodep, Var)) {
                যদি (varp->isParam()) {
                    যদি (first) {
                        puts("\n");
                        ধ্রুবক string modName = EmitCUtil::prefixNameProtect(modp);
                        putsDecoration(modp, "// Parameter definitions for " + modName + "\n");
                        first = মিথ্যা;
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
    স্থির শূন্য addVarCtorDtorStmts(TextStmts& stmts, ধ্রুবক AstVar* varp, যৌক্তিক destroy) {
        ধ্রুবক std::string name = "vlSelf->" + varName(varp);

        // SystemC port arrays are C arrays, so construct them element by element. They are
        // default constructed (unnamed), as the elements of an array member would be.
        যদি (varp->isSc() && VN_IS(varp->dtypeSkipRefp(), UnpackArrayDType)) {
            std::vector<std::string> elems{name};
            জন্য (ধ্রুবক AstUnpackArrayDType* adtypep
                 = VN_CAST(varp->dtypeSkipRefp(), UnpackArrayDType);
                 adtypep; adtypep = VN_CAST(adtypep->subDTypep()->skipRefp(), UnpackArrayDType)) {
                std::vector<std::string> subElems;
                জন্য (ধ্রুবক std::string& elem : elems) {
                    জন্য (পূর্ণসংখ্যা i = 0; i < adtypep->elementsConst(); ++i) {
                        subElems.ঠেলো_back(elem + "[" + std::to_string(i) + "]");
                    }
                }
                elems = std::move(subElems);
            }
            যদি (destroy) std::reverse(elems.begin(), elems.end());
            জন্য (ধ্রুবক std::string& e : elems) {
                stmts.add((destroy ? "VL_INPLACE_DESTRUCT(" : "VL_INPLACE_CONSTRUCT(") + e + ");");
            }
            ফেরত;
        }

        // Other variables are constructed whole, even if aggregates
        যদি (destroy) {
            stmts.add("VL_INPLACE_DESTRUCT(" + name + ");");
            ফেরত;
        }
        ধ্রুবক AstBasicDType* ধ্রুবক basicp = VN_CAST(varp->dtypeSkipRefp(), BasicDType);
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
    শূন্য createCtorDtorStmts(ধ্রুবক Ast_গ্রন্থি_Module* modp) {
        // Construct member variables
        জন্য (ধ্রুবক Ast_গ্রন্থি* nodep = modp->stmtsp(); nodep; nodep = nodep->nextp()) {
            ধ্রুবক AstVar* ধ্রুবক varp = VN_CAST(nodep, Var);
            যদি (varp && isExplicitlyConstructed(varp)) {
                addVarCtorDtorStmts(m_ctorStmts, varp, মিথ্যা);
            }
        }

        // Destroy in reverse order
        ধ্রুবক Ast_গ্রন্থি* ধ্রুবক lastp = modp->stmtsp() ? modp->stmtsp()->lastp() : nullptr;
        জন্য (ধ্রুবক Ast_গ্রন্থি* nodep = lastp; nodep; nodep = nodep->prevp()) {
            ধ্রুবক AstVar* ধ্রুবক varp = VN_CAST(nodep, Var);
            যদি (varp && isExplicitlyConstructed(varp)) {
                addVarCtorDtorStmts(m_dtorStmts, varp, সত্য);
            }
        }

        // Split into sub-functions if needed
        ধ্রুবক std::string modName = EmitCUtil::prefixNameProtect(modp);
        std::tie(m_ctorSplitDecl, m_dtorSplitDecl) = emitSplitCtorDtor(
            m_ctorStmts, m_dtorStmts, 0, modName, modName + "* vlSelf", "vlSelf", মিথ্যা,
            "Design implementation internals", [এই]() { emitFileHeader(); });
    }
    শূন্য emitCtorDtorImp(ধ্রুবক Ast_গ্রন্থি_Module* modp) {
        ধ্রুবক std::string modName = EmitCUtil::prefixNameProtect(modp);

        // ctor
        যদি (modp->ctorVarReset()) {
            m_lazyDecls.emit("\nvoid " + modName + "__", protect("_ctor_var_reset"),
                             "(" + modName + "* vlSelf);\n");
        }

        যদি (!m_ctorSplitDecl.empty()) puts("\n" + m_ctorSplitDecl + "\n");
        puts("\nvoid " + modName + "__ctor(" + modName + "* vlSelf, " + EmitCUtil::symClassName()
             + "* symsp, const char* namep) {\n");
        puts("vlSelf->vlSymsp = symsp;\n");
        যদি (modp->is_শীর্ষ()) {
            puts("vlSelf->vlNamep = strdup(namep);\n");
        } অন্যথা {
            puts("vlSelf->vlNamep = strdup(Verilated::catName(symsp->name(), namep));\n");
        }
        putsDecoration(modp, "// Construct members\n");
        জন্য (ধ্রুবক TextStmts::Stmt& stmt : m_ctorStmts) puts(stmt.m_text + "\n");

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
        জন্য (ধ্রুবক TextStmts::Stmt& stmt : m_dtorStmts) puts(stmt.m_text + "\n");
        puts("VL_DO_DANGLING(std::free(const_cast<char*>(vlSelf->vlNamep)), "
             "vlSelf->vlNamep);\n");
        puts("}\n");

        // C++ destructor
        puts("\n" + modName + "::~" + modName + "() {\n");
        emitSystemCSection(modp, VSystemCSectionType::DTOR);
        puts(modName + "__dtor(this);\n");
        puts("}\n");
    }

    শূন্য emitConfigureImp(ধ্রুবক Ast_গ্রন্থি_Module* modp) {
        ধ্রুবক string modName = EmitCUtil::prefixNameProtect(modp);

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
    শূন্য emitSavableImp(ধ্রুবক Ast_গ্রন্থি_Module* modp) {
        যদি (v3Global.opt.savable()) {
            puts("\n// Savable\n");
            জন্য (পূর্ণসংখ্যা de = 0; de < 2; ++de) {
                ধ্রুবক string classname = de ? "VerilatedDeserialize" : "VerilatedSerialize";
                ধ্রুবক string funcname = de ? "__Vdeserialize" : "__Vserialize";
                ধ্রুবক string op = de ? ">>" : "<<";
                // NOLINTNEXTLINE(performance-inefficient-string-concatenation)
                putns(modp, "void " + EmitCUtil::prefixNameProtect(modp) + "::" + protect(funcname)
                                + "(" + classname + "& os) {\n");
                // Place a computed checksum to ensure proper structure save/restore formatting
                // OK if this hash includes some things we won't dump, since
                // just looking for loading the wrong model
                VHashSha512 hash;
                জন্য (Ast_গ্রন্থি* nodep = modp->stmtsp(); nodep; nodep = nodep->nextp()) {
                    যদি (ধ্রুবক AstVar* ধ্রুবক varp = VN_CAST(nodep, Var)) {
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
                জন্য (Ast_গ্রন্থি* nodep = modp->stmtsp(); nodep; nodep = nodep->nextp()) {
                    যদি (ধ্রুবক AstVar* ধ্রুবক varp = VN_CAST(nodep, Var)) {
                        যদি (varp->isIO() && modp->is_শীর্ষ() && optSystemC()) {
                            // System C top I/O doesn't need loading, as the
                            // lower level subinst code does it.
                        } অন্যথা যদি (varp->isParam()) {
                        } অন্যথা যদি (VN_IS(varp->dtypep(), NBACommitQueueDType)) {
                        } অন্যথা {
                            পূর্ণসংখ্যা vects = 0;
                            Ast_গ্রন্থি_DType* elementp = varp->dtypeSkipRefp();
                            জন্য (AstUnpackArrayDType* arrayp = VN_CAST(elementp, UnpackArrayDType);
                                 arrayp; arrayp = VN_CAST(elementp, UnpackArrayDType)) {
                                ধ্রুবক পূর্ণসংখ্যা vecnum = vects++;
                                UASSERT_OBJ(arrayp->hi() >= arrayp->lo(), varp,
                                            "Should have swapped msb & lsb earlier.");
                                ধ্রুবক string ivar = "__Vi"s + cvtToStr(vecnum);
                                puts("for (int __Vi" + cvtToStr(vecnum) + " = " + cvtToStr(0));
                                puts("; " + ivar + " < " + cvtToStr(arrayp->elementsConst()));
                                puts("; ++" + ivar + ") {\n");
                                elementp = arrayp->subDTypep()->skipRefp();
                            }
                            ধ্রুবক AstBasicDType* ধ্রুবক basicp = elementp->basicp();
                            // Do not save MTask state, only matters within an evaluation
                            যদি (basicp && basicp->keyword().isMTaskState()) চালু;
                            // Want to detect types that are represented as arrays
                            // (i.e. packed types of more than 64 bits).
                            যদি (elementp->isWide()
                                && !(basicp && basicp->keyword() == VBasicDTypeKwd::STRING)) {
                                ধ্রুবক পূর্ণসংখ্যা vecnum = vects++;
                                ধ্রুবক string ivar = "__Vi"s + cvtToStr(vecnum);
                                puts("for (int __Vi" + cvtToStr(vecnum) + " = " + cvtToStr(0));
                                puts("; " + ivar + " < " + cvtToStr(elementp->width_শব্দগুলি()));
                                puts("; ++" + ivar + ") {\n");
                            }
                            putns(varp, "os" + op + varName(varp));
                            জন্য (পূর্ণসংখ্যা v = 0; v < vects; ++v) puts("[__Vi" + cvtToStr(v) + "]");
                            puts(";\n");
                            জন্য (পূর্ণসংখ্যা v = 0; v < vects; ++v) puts("}\n");
                        }
                    }
                }

                puts("}\n");
            }
        }
    }
    // Predicate to check if we actually need to emit anything into the common implementation file.
    // Used to avoid creating empty output files.
    যৌক্তিক hasCommonImp(ধ্রুবক Ast_গ্রন্থি_Module* modp) ধ্রুবক {
        // Nothing to emit if no module!
        যদি (!modp) ফেরত মিথ্যা;
        // We always need the slow file
        যদি (m_slow) ফেরত সত্য;
        // The fast file is only required when we have `systemc_implementation nodes
        জন্য (ধ্রুবক AstSystemCSection* scSectionp = modp->scSectionsp(); scSectionp;
             scSectionp = VN_AS(scSectionp->nextp(), SystemCSection)) {
            যদি (scSectionp->sectionType() == VSystemCSectionType::IMP) ফেরত সত্য;
        }
        ফেরত মিথ্যা;
    }
    // Actually emit common implementation contents for given AstNodeModule
    শূন্য doCommonImp(ধ্রুবক Ast_গ্রন্থি_Module* modp) {
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
    শূন্য emitCommonImp(ধ্রুবক Ast_গ্রন্থি_Module* modp) {
        ধ্রুবক AstClass* ধ্রুবক classp
            = VN_IS(modp, ClassPackage) ? VN_AS(modp, ClassPackage)->classp() : nullptr;

        যদি (hasCommonImp(modp) || hasCommonImp(classp)) {
            // Create ctor/dtor statements. If these need splitting, they will be emitted
            // during construction so do this before opening the output file.
            যদি (m_slow && !VN_IS(modp, Class)) createCtorDtorStmts(modp);

            open_পরবর্তী_Output_File(m_fileBaseName);

            doCommonImp(modp);
            যদি (classp) {
                VL_RESTORER(m_modp);
                m_modp = classp;
                doCommonImp(classp);
            }

            closeOutputFile();
        }
    }
    শূন্য emitCFuncImp(ধ্রুবক Ast_গ্রন্থি_Module* modp) {
        // Functions to be emitted here
        std::vector<AstCFunc*> funcps;

        ধ্রুবক auto gather = [এই, &funcps](ধ্রুবক Ast_গ্রন্থি_Module* modp) {
            জন্য (Ast_গ্রন্থি* nodep = modp->stmtsp(); nodep; nodep = nodep->nextp()) {
                AstCFunc* ধ্রুবক funcp = VN_CAST(nodep, CFunc);
                যদি (!funcp) চালু;
                // TRACE_* and DPI handled elsewhere
                যদি (funcp->isTrace()) চালু;
                যদি (funcp->dpiImportPrototype()) চালু;
                যদি (funcp->dpiExportDispatcher()) চালু;
                যদি (funcp->slow() != m_slow) চালু;
                funcps.ঠেলো_back(funcp);
            }
        };

        gather(modp);
        VL_RESTORER(m_classOrPackage);
        যদি (ধ্রুবক AstClassPackage* ধ্রুবক packagep = VN_CAST(modp, ClassPackage)) {
            m_classOrPackage = packagep;
            gather(packagep->classp());
        }

        // Do not create empty files
        যদি (funcps.empty()) ফেরত;

        // Open output file
        open_পরবর্তী_Output_File(m_uniqueNames.get(m_fileBaseName));
        // Emit all functions
        জন্য (AstCFunc* ধ্রুবক funcp : funcps) {
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
            v3Global.useParallelBuild(সত্য);
            // Close old file
            closeOutputFile();
            // Open a new file
            open_পরবর্তী_Output_File(m_uniqueNames.get(m_fileBaseName));
        }

        EmitCFunc::visit(nodep);
    }

    explicit EmitCImp(ধ্রুবক Ast_গ্রন্থি_Module* modp, যৌক্তিক slow)
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
    ~EmitCImp() override = পূর্বনির্ধারিত;

সর্বজনীন:
    স্থির std::vector<AstCFile*> প্রধান(ধ্রুবক Ast_গ্রন্থি_Module* modp, যৌক্তিক slow) VL_MT_STABLE {
        EmitCImp emitCImp{modp, slow};
        ফেরত emitCImp.getAndClearCfileps();
    }
};

//######################################################################
// Tracing routines

// Trace type descriptors go in a different file as it needs to be written in
// parallel with the actual trace function source files
শ্রেণী EmitCTraceTypes final : সর্বজনীন EmitCFunc {
    // NODE STATE/TYPES
    // None allowed to support threaded emitting

    // STATE
    পূর্ণসংখ্যা m_enum_অঙ্ক = 0;  // Enumeration number (whole netlist)
    std::unordered_map<Ast_গ্রন্থি*, পূর্ণসংখ্যা> m_enum_অঙ্ক_Map;  // EnumDType to enumeration number
    পূর্ণসংখ্যা m_traceTypeSubs = 0;  // Number of trace type declaration sub-functions
    V3UniqueNames m_uniqueNames;  // Generates unique file names
    ধ্রুবক std::string m_fileBaseName = EmitCUtil::শীর্ষ_Class_Name() + "_" + protect("_TraceDecls");
    // This one uses CSplitTrace for file splitting, which is incorrect but historically accurates
    ধ্রুবক আকার_t m_splitLimit = v3Global.opt.outputSplitCTrace()
                                    ? static_cast<আকার_t>(v3Global.opt.outputSplitCTrace())
                                    : std::numeric_limits<আকার_t>::max();

    শূন্য open_পরবর্তী_Output_File() {
        openNewOutputSourceFile(m_uniqueNames.get(m_fileBaseName), সত্য, সত্য,
                                "Tracing declarations");
        puts("\n");
        জন্য (ধ্রুবক std::string& base : v3Global.opt.traceSourceLangs()) {
            puts("#include \"" + base + ".h\"\n");
        }
        puts("\n");
        puts("\nvoid " + EmitCUtil::prefixNameProtect(m_modp) + "__"
             + protect("traceDeclTypesSub" + std::to_string(m_traceTypeSubs++)) + "("
             + v3Global.opt.traceClassBase() + "* tracep) {\n");
    }

সর্বজনীন:
    // METHODS
    পূর্ণসংখ্যা get_Enum_Map_অঙ্ক(AstEnumDType* nodep) {
        পূর্ণসংখ্যা& enumNumr = m_enum_অঙ্ক_Map[nodep];
        যদি (!enumNumr) {
            যদি (splitNeeded(m_splitLimit)) {
                // Splitting file, so using parallel build.
                v3Global.useParallelBuild(সত্য);
                puts("}\n");
                closeOutputFile();
                open_পরবর্তী_Output_File();
            }

            enumNumr = ++m_enum_অঙ্ক;
            পূর্ণসংখ্যা nvals = 0;
            puts("{\n");
            putns(nodep, "const char* " + protect("__VenumItemNames") + "[]\n");
            puts("= {");
            জন্য (AstEnumItem* itemp = nodep->itemsp(); itemp;
                 itemp = VN_AS(itemp->nextp(), EnumItem)) {
                যদি (++nvals > 1) puts(", ");
                putbs("\"" + itemp->prettyName() + "\"");
            }
            puts("};\n");
            nvals = 0;
            puts("const char* " + protect("__VenumItemValues") + "[]\n");
            puts("= {");
            জন্য (AstEnumItem* itemp = nodep->itemsp(); itemp;
                 itemp = VN_AS(itemp->nextp(), EnumItem)) {
                AstConst* ধ্রুবক constp = VN_AS(itemp->valuep(), Const);
                যদি (++nvals > 1) puts(", ");
                putbs("\"" + constp->অঙ্ক().displayed(nodep, "%0b") + "\"");
            }
            puts("};\n");
            puts("tracep->declDTypeEnum(" + std::to_string(enumNumr) + ", \"" + nodep->prettyName()
                 + "\", " + std::to_string(nvals) + ", " + std::to_string(nodep->widthMin()) + ", "
                 + protect("__VenumItemNames") + ", " + protect("__VenumItemValues") + ");\n");
            puts("}\n");
            split_আকার_Inc(Ast_গ্রন্থি::INSTR_গুনতি_CALL);
        }
        ফেরত enumNumr;
    }

    // Close output file
    শূন্য finalize() {
        // Close function definition
        puts("}\n");

        ধ্রুবক std::string modName = EmitCUtil::prefixNameProtect(m_modp);
        ধ্রুবক std::string args = v3Global.opt.traceClassBase() + "* tracep";

        // Forward declarations for subs in other files
        জন্য (পূর্ণসংখ্যা i = 0; i < m_traceTypeSubs - 1; ++i) {
            puts("void " + modName + "__" + protect("traceDeclTypesSub" + std::to_string(i)) + "("
                 + args + ");\n");
        }

        // Create top level trace_decl_types function and call each sub-function
        puts("\nvoid " + modName + "__" + protect("trace_decl_types") + "(" + args + ") {\n");
        জন্য (পূর্ণসংখ্যা i = 0; i < m_traceTypeSubs; ++i) {
            puts(modName + "__" + protect("traceDeclTypesSub" + std::to_string(i))
                 + "(tracep);\n");
        }
        puts("}\n");

        closeOutputFile();
    }

    EmitCTraceTypes() {
        m_modp = v3Global.rootp()->শীর্ষ_Modulep();
        open_পরবর্তী_Output_File();
    }
    ~EmitCTraceTypes() override = পূর্বনির্ধারিত;
};

শ্রেণী EmitCTrace final : সর্বজনীন EmitCFunc {
    // NODE STATE/TYPES
    // None allowed to support threaded emitting

    // MEMBERS
    ধ্রুবক যৌক্তিক m_slow;  // Making slow file
    ধ্রুবক std::unique_ptr<EmitCTraceTypes> m_emitTypesp{m_slow ? নতুন EmitCTraceTypes{} : nullptr};
    V3UniqueNames m_uniqueNames;  // Generates unique file names
    ধ্রুবক std::string m_fileBaseName = EmitCUtil::শীর্ষ_Class_Name() + "_" + protect("_Trace");

    // METHODS
    শূন্য open_পরবর্তী_Output_File() {
        openNewOutputSourceFile(m_uniqueNames.get(m_fileBaseName), m_slow, সত্য,
                                "Tracing implementation internals");
        puts("\n");
        জন্য (ধ্রুবক std::string& base : v3Global.opt.traceSourceLangs()) {
            puts("#include \"" + base + ".h\"\n");
        }
        puts("#include \"" + EmitCUtil::symClassName() + ".h\"\n");
        puts("\n");
        // Need to emit new lazy declarations
        m_lazyDecls.reset();
    }

    যৌক্তিক emitTraceIsScBv(ধ্রুবক AstTraceInc* nodep) {
        ধ্রুবক AstVarRef* ধ্রুবক varrefp = VN_CAST(nodep->declp()->valuep(), VarRef);
        যদি (!varrefp) ফেরত মিথ্যা;
        ধ্রুবক AstVar* ধ্রুবক varp = varrefp->varp();
        ফেরত varp->isSc() && varp->isScBv();
    }

    যৌক্তিক emitTraceIsScBigUint(ধ্রুবক AstTraceInc* nodep) {
        ধ্রুবক AstVarRef* ধ্রুবক varrefp = VN_CAST(nodep->declp()->valuep(), VarRef);
        যদি (!varrefp) ফেরত মিথ্যা;
        ধ্রুবক AstVar* ধ্রুবক varp = varrefp->varp();
        ফেরত varp->isSc() && varp->isScBigUint();
    }

    যৌক্তিক emitTraceIsScUint(ধ্রুবক AstTraceInc* nodep) {
        ধ্রুবক AstVarRef* ধ্রুবক varrefp = VN_CAST(nodep->declp()->valuep(), VarRef);
        যদি (!varrefp) ফেরত মিথ্যা;
        ধ্রুবক AstVar* ধ্রুবক varp = varrefp->varp();
        ফেরত varp->isSc() && (varp->isScUint() || varp->isScUintBool());
    }

    শূন্য emitTraceInitOne(ধ্রুবক AstTraceDecl* nodep, পূর্ণসংখ্যা enum_অঙ্ক) {
        std::string direction;
        direction = nodep->declDirection().traceSigDirection();

        AstCCall* ধ্রুবক callp = nodep->dtypeCallp();
        যদি (callp) {
            callp->argTypes(callp->argTypes() + ", " + cvtToStr(nodep->fidx()) + ", c+"
                            + cvtToStr(nodep->code()) + ", " + direction);
            ফেরত;
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

    পূর্ণসংখ্যা emitTraceDeclDType(Ast_গ্রন্থি_DType* nodep) {
        // Return enum number or -1 for none
        যদি (v3Global.opt.traceEnabledFst()) {
            // Skip over refs-to-refs, but stop before final ref so can get data type name
            // Alternatively back in V3Width we could push enum names from upper typedefs
            যদি (AstEnumDType* ধ্রুবক enump = VN_CAST(nodep->skipRefToEnump(), EnumDType)) {
                ফেরত m_emitTypesp->get_Enum_Map_অঙ্ক(enump);
            }
        }
        ফেরত -1;
    }

    শূন্য emitTraceChangeOne(AstTraceInc* nodep, পূর্ণসংখ্যা arrayindex) {
        // Note: Both VTraceType::CHANGE and VTraceType::FULL use the 'full' methods
        ধ্রুবক std::string func = nodep->traceType() == VTraceType::CHANGE ? "chg" : "full";
        যৌক্তিক emitWidth = সত্য;
        string stype;
        যদি (nodep->dtypep()->basicp()->isDouble()) {
            stype = "Double";
            emitWidth = মিথ্যা;
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
            emitWidth = মিথ্যা;
        } অন্যথা {
            stype = "Bit";
            emitWidth = মিথ্যা;
        }
        putns(nodep, "bufp->" + func + stype);

        ধ্রুবক uint32_t offset = (arrayindex < 0) ? 0 : (arrayindex * nodep->declp()->width_শব্দগুলি());
        ধ্রুবক uint32_t code = nodep->declp()->code() + offset;
        // Note: Both VTraceType::CHANGE and VTraceType::FULL use the 'full' methods
        puts("(oldp+");
        puts(cvtToStr(code - nodep->baseCode()));
        puts(",");
        ধ্রুবক VNumRange& arrayRange = nodep->declp()->arrayRange();
        যদি (arrayRange.ranged() && !arrayRange.ascending()) {
            arrayindex = arrayRange.elements() - 1 - arrayindex;
        }
        emit_Trace_মান(nodep, arrayindex);
        যদি (emitWidth) puts("," + cvtToStr(nodep->declp()->widthMin()));
        puts(");\n");
    }

    শূন্য emit_Trace_মান(ধ্রুবক AstTraceInc* nodep, পূর্ণসংখ্যা arrayindex) {
        যদি (AstVarRef* ধ্রুবক varrefp = VN_CAST(nodep->valuep(), VarRef)) {
            ধ্রুবক AstVar* ধ্রুবক varp = varrefp->varp();
            যদি (varp->isEvent()) puts("&");
            puts("(");
            যদি (emitTraceIsScBigUint(nodep)) {
                puts("(uint32_t*)");
            } অন্যথা যদি (emitTraceIsScBv(nodep)) {
                puts("VL_SC_BV_DATAP(");
            }
            iterateConst(varrefp);  // Put var name out
            emit_Trace_সূচক(nodep, arrayindex);
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
            emit_Trace_সূচক(nodep, arrayindex);
            puts(")");
        }
    }

    শূন্য emit_Trace_সূচক(ধ্রুবক AstTraceInc* ধ্রুবক nodep, পূর্ণসংখ্যা arrayindex) {
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
        যদি (!nodep->isTrace()) ফেরত;
        যদি (nodep->slow() != m_slow) ফেরত;

        যদি (splitNeeded()) {
            // Splitting file, so using parallel build.
            v3Global.useParallelBuild(সত্য);
            // Close old file
            closeOutputFile();
            // Open a new file
            open_পরবর্তী_Output_File();
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
    শূন্য visit(Ast_Trace_বের_করো_Prefix* nodep) override {  //
        putns(nodep, "VL_TRACE_POP_PREFIX(tracep);\n");
    }
    শূন্য visit(AstTraceDecl* nodep) override {
        ধ্রুবক পূর্ণসংখ্যা enum_অঙ্ক = emitTraceDeclDType(nodep->dtypep());
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
            জন্য (পূর্ণসংখ্যা i = 0; i < nodep->declp()->arrayRange().elements(); i++) {
                emitTraceChangeOne(nodep, i);
            }
        } অন্যথা {
            emitTraceChangeOne(nodep, -1);
        }
    }

    explicit EmitCTrace(যৌক্তিক slow)
        : m_slow{slow} {
        m_modp = v3Global.rootp()->শীর্ষ_Modulep();
        // Open output file
        open_পরবর্তী_Output_File();
        // Emit functions
        জন্য (Ast_গ্রন্থি* nodep = m_modp->stmtsp(); nodep; nodep = nodep->nextp()) {
            যদি (AstCFunc* ধ্রুবক funcp = VN_CAST(nodep, CFunc)) iterateConst(funcp);
        }
        // Close output file
        closeOutputFile();
        যদি (m_slow) m_emitTypesp->finalize();
    }
    ~EmitCTrace() override = পূর্বনির্ধারিত;

সর্বজনীন:
    স্থির std::vector<AstCFile*> প্রধান(যৌক্তিক slow) VL_MT_STABLE {
        EmitCTrace emitCTrace{slow};
        std::vector<AstCFile*> cfileps = emitCTrace.getAndClearCfileps();
        যদি (slow) {
            জন্য (AstCFile* ধ্রুবক cfilep : emitCTrace.m_emitTypesp->getAndClearCfileps()) {
                cfileps.emplace_back(cfilep);
            }
        }
        ফেরত cfileps;
    }
};

//######################################################################
// Existing AstCFile emitter

শ্রেণী EmitCFile final : সর্বজনীন EmitCFunc {
    explicit EmitCFile(AstCFile* cfilep) {
        openOutputFile(cfilep, "Generated C++");
        iterateConst(cfilep->tblockp());
        closeOutputFile();
    }
    ~EmitCFile() override = পূর্বনির্ধারিত;

সর্বজনীন:
    স্থির শূন্য প্রধান(AstCFile* cfilep) VL_MT_STABLE {
        যদি (!cfilep->tblockp()) ফেরত;
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
        ধ্রুবক EmitCParentModule emitCParentModule;
        V3ThreadScope threadScope;

        // Process each module in turn
        জন্য (ধ্রুবক Ast_গ্রন্থি* nodep = v3Global.rootp()->modulesp(); nodep; nodep = nodep->nextp()) {
            যদি (VN_IS(nodep, Class)) চালু;  // Imped with ClassPackage
            ধ্রুবক Ast_গ্রন্থি_Module* ধ্রুবক modp = VN_AS(nodep, গ্রন্থি_Module);
            যদি (modp->isConstPool()) চালু;  // Emitted by V3EmitCConstPool
            cfiles.emplace_back();
            std::vector<AstCFile*>& slow = cfiles.back();
            threadScope.enqueue([modp, &slow] { slow = EmitCImp::প্রধান(modp, /* slow: */ সত্য); });
            cfiles.emplace_back();
            std::vector<AstCFile*>& fast = cfiles.back();
            threadScope.enqueue([modp, &fast] { fast = EmitCImp::প্রধান(modp, /* slow: */ মিথ্যা); });
        }

        // Emit trace routines (currently they can only exist in the top module)
        যদি (v3Global.opt.trace() && !v3Global.opt.lintOnly()) {
            cfiles.emplace_back();
            std::vector<AstCFile*>& slow = cfiles.back();
            threadScope.enqueue([&slow] { slow = EmitCTrace::প্রধান(/* slow: */ সত্য); });
            cfiles.emplace_back();
            std::vector<AstCFile*>& fast = cfiles.back();
            threadScope.enqueue([&fast] { fast = EmitCTrace::প্রধান(/* slow: */ মিথ্যা); });
        }
    }
    // Add files to netlist
    জন্য (ধ্রুবক std::vector<AstCFile*>& cfileps : cfiles) {
        জন্য (AstCFile* ধ্রুবক cfilep : cfileps) v3Global.rootp()->addFilesp(cfilep);
    }
}

শূন্য V3EmitC::emitcFiles() {
    UINFO(2, __FUNCTION__ << ":");
    জন্য (Ast_গ্রন্থি_File *filep = v3Global.rootp()->filesp(), *nextp; filep; filep = nextp) {
        nextp = VN_AS(filep->nextp(), গ্রন্থি_File);
        যদি (AstCFile* ধ্রুবক cfilep = VN_CAST(filep, CFile)) EmitCFile::প্রধান(cfilep);
    }
}
