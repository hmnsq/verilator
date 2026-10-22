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

#ಅನ್ತರ್ಭಾವ "V3EmitC.h"
#ಅನ್ತರ್ಭಾವ "V3EmitCFunc.h"
#ಅನ್ತರ್ಭಾವ "V3ThreadPool.h"
#ಅನ್ತರ್ಭಾವ "V3UniqueNames.h"

#ಅನ್ತರ್ಭಾವ <algorithm>
#ಅನ್ತರ್ಭಾವ <map>
#ಅನ್ತರ್ಭಾವ <set>
#ಅನ್ತರ್ಭಾವ <tuple>
#ಅನ್ತರ್ಭಾವ <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Internal EmitC implementation

ವರ್ಗ EmitCImp final : ಸಾರ್ವಜನಿಕ EmitCFunc {
    // MEMBERS
    // Base module (For non-classes, same as m_modp. For classes, it's the ClassPackage.)
    ಧ್ರುವ Ast_ಗಂಟು_Module* ಧ್ರುವ m_fileModp;
    ಧ್ರುವ ತಾರ್ಕಿಕ m_slow;  // Creating __Slow file
    V3UniqueNames m_uniqueNames;  // Generates unique file names
    ಧ್ರುವ std::string m_fileBaseName = EmitCUtil::prefixNameProtect(m_fileModp);
    TextStmts m_ctorStmts;  // Statements constructing members in 'ctor'
    TextStmts m_dtorStmts;  // Statements destroying members in 'dtor'
    std::string m_ctorSplitDecl;  // Declaration of sub-function 'ctor' was split into, if any
    std::string m_dtorSplitDecl;  // Declaration of sub-function 'dtor' was split into, if any

    // METHODS
    ಶೂನ್ಯ openNextOutputFile(ಧ್ರುವ std::string& fileName) {
        openNewOutputSourceFile(fileName, m_slow, ಅಸತ್ಯ, "Design implementation internals");
        emitFileHeader();
    }
    ಶೂನ್ಯ emitFileHeader() {
        puts("// See " + EmitCUtil::ತುದಿ_Class_Name() + ".h for the primary calling header\n");
        puts("\n");
        puts("#include \"" + EmitCUtil::pchClassName() + ".h\"\n");
        emitSystemCSection(m_fileModp, VSystemCSectionType::IMP_HDR);
        // Need to emit new lazy declarations
        m_lazyDecls.reset();
    }

    ಶೂನ್ಯ emitParamDefns(ಧ್ರುವ Ast_ಗಂಟು_Module* modp) {
        ತಾರ್ಕಿಕ first = ಸತ್ಯ;
        ಪ್ರತಿ (ಧ್ರುವ Ast_ಗಂಟು* nodep = modp->stmtsp(); nodep; nodep = nodep->nextp()) {
            ಯದಿ (ಧ್ರುವ AstVar* ಧ್ರುವ varp = VN_CAST(nodep, Var)) {
                ಯದಿ (varp->isParam()) {
                    ಯದಿ (first) {
                        puts("\n");
                        ಧ್ರುವ string modName = EmitCUtil::prefixNameProtect(modp);
                        putsDecoration(modp, "// Parameter definitions for " + modName + "\n");
                        first = ಅಸತ್ಯ;
                    }
                    ಯದಿ (varp->dtypep()->isLiteralType()) putns(varp, "VL_CONSTINIT_CXX20 ");
                    putns(varp, "const ");
                    emitVarDefn(varp);
                    puts(";\n");
                }
            }
        }
        ಯದಿ (!first) puts("\n");
    }
    // Add statements explicitly constructing, or destroying, a member AstVar
    ಸ್ಥಿರ ಶೂನ್ಯ addVarCtorDtorStmts(TextStmts& stmts, ಧ್ರುವ AstVar* varp, ತಾರ್ಕಿಕ destroy) {
        ಧ್ರುವ std::string name = "vlSelf->" + varName(varp);

        // SystemC port arrays are C arrays, so construct them element by element. They are
        // default constructed (unnamed), as the elements of an array member would be.
        ಯದಿ (varp->isSc() && VN_IS(varp->dtypeSkipRefp(), UnpackArrayDType)) {
            std::vector<std::string> elems{name};
            ಪ್ರತಿ (ಧ್ರುವ AstUnpackArrayDType* adtypep
                 = VN_CAST(varp->dtypeSkipRefp(), UnpackArrayDType);
                 adtypep; adtypep = VN_CAST(adtypep->subDTypep()->skipRefp(), UnpackArrayDType)) {
                std::vector<std::string> subElems;
                ಪ್ರತಿ (ಧ್ರುವ std::string& elem : elems) {
                    ಪ್ರತಿ (ಪೂರ್ಣಾಙ್ಕ i = 0; i < adtypep->elementsConst(); ++i) {
                        subElems.ತಳ್ಳು_back(elem + "[" + std::to_string(i) + "]");
                    }
                }
                elems = std::move(subElems);
            }
            ಯದಿ (destroy) std::reverse(elems.begin(), elems.end());
            ಪ್ರತಿ (ಧ್ರುವ std::string& e : elems) {
                stmts.add((destroy ? "VL_INPLACE_DESTRUCT(" : "VL_INPLACE_CONSTRUCT(") + e + ");");
            }
            ಪ್ರತ್ಯಾಗಮ;
        }

        // Other variables are constructed whole, even if aggregates
        ಯದಿ (destroy) {
            stmts.add("VL_INPLACE_DESTRUCT(" + name + ");");
            ಪ್ರತ್ಯಾಗಮ;
        }
        ಧ್ರುವ AstBasicDType* ಧ್ರುವ basicp = VN_CAST(varp->dtypeSkipRefp(), BasicDType);
        std::string args;
        ಯದಿ (varp->isSc()) {
            // SystemC ports need their name
            args = ", \"" + V3OutFormatter::quoteNameControls(varp->nameProtect()) + "\"";
        } ಅನ್ಯಥಾ ಯದಿ (basicp && basicp->keyword().isMTaskState()) {
            // MTask state needs the number of upstream dependencies to wait for
            args = ", " + VN_AS(varp->valuep(), Const)->ಅಂಕೆ().emitC();
        } ಅನ್ಯಥಾ ಯದಿ (basicp && basicp->isDelayScheduler()) {
            // Delay scheduler needs the context pointer
            args = ", *vlSelf->vlSymsp->_vm_contextp__";
        }
        stmts.add("VL_INPLACE_CONSTRUCT(" + name + args + ");");
    }
    // Gather the statements constructing/destroying members in anonymous unions, and split
    // them into sub-functions if needed. As this emits files, no output file can be open.
    ಶೂನ್ಯ createCtorDtorStmts(ಧ್ರುವ Ast_ಗಂಟು_Module* modp) {
        // Construct member variables
        ಪ್ರತಿ (ಧ್ರುವ Ast_ಗಂಟು* nodep = modp->stmtsp(); nodep; nodep = nodep->nextp()) {
            ಧ್ರುವ AstVar* ಧ್ರುವ varp = VN_CAST(nodep, Var);
            ಯದಿ (varp && isExplicitlyConstructed(varp)) {
                addVarCtorDtorStmts(m_ctorStmts, varp, ಅಸತ್ಯ);
            }
        }

        // Destroy in reverse order
        ಧ್ರುವ Ast_ಗಂಟು* ಧ್ರುವ lastp = modp->stmtsp() ? modp->stmtsp()->lastp() : nullptr;
        ಪ್ರತಿ (ಧ್ರುವ Ast_ಗಂಟು* nodep = lastp; nodep; nodep = nodep->prevp()) {
            ಧ್ರುವ AstVar* ಧ್ರುವ varp = VN_CAST(nodep, Var);
            ಯದಿ (varp && isExplicitlyConstructed(varp)) {
                addVarCtorDtorStmts(m_dtorStmts, varp, ಸತ್ಯ);
            }
        }

        // Split into sub-functions if needed
        ಧ್ರುವ std::string modName = EmitCUtil::prefixNameProtect(modp);
        std::tie(m_ctorSplitDecl, m_dtorSplitDecl) = emitSplitCtorDtor(
            m_ctorStmts, m_dtorStmts, 0, modName, modName + "* vlSelf", "vlSelf", ಅಸತ್ಯ,
            "Design implementation internals", [ಏತತ್]() { emitFileHeader(); });
    }
    ಶೂನ್ಯ emitCtorDtorImp(ಧ್ರುವ Ast_ಗಂಟು_Module* modp) {
        ಧ್ರುವ std::string modName = EmitCUtil::prefixNameProtect(modp);

        // ctor
        ಯದಿ (modp->ctorVarReset()) {
            m_lazyDecls.emit("\nvoid " + modName + "__", protect("_ctor_var_reset"),
                             "(" + modName + "* vlSelf);\n");
        }

        ಯದಿ (!m_ctorSplitDecl.empty()) puts("\n" + m_ctorSplitDecl + "\n");
        puts("\nvoid " + modName + "__ctor(" + modName + "* vlSelf, " + EmitCUtil::symClassName()
             + "* symsp, const char* namep) {\n");
        puts("vlSelf->vlSymsp = symsp;\n");
        ಯದಿ (modp->is_ತುದಿ()) {
            puts("vlSelf->vlNamep = strdup(namep);\n");
        } ಅನ್ಯಥಾ {
            puts("vlSelf->vlNamep = strdup(Verilated::catName(symsp->name(), namep));\n");
        }
        putsDecoration(modp, "// Construct members\n");
        ಪ್ರತಿ (ಧ್ರುವ TextStmts::Stmt& stmt : m_ctorStmts) puts(stmt.m_text + "\n");

        putsDecoration(modp, "// Reset structure values\n");
        ಯದಿ (modp->ctorVarReset()) {
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
        ಯದಿ (!m_dtorSplitDecl.empty()) puts("\n" + m_dtorSplitDecl + "\n");
        puts("\nvoid " + modName + "__dtor(" + modName + "* vlSelf) {\n");
        putsDecoration(modp, "// Destroy members\n");
        ಪ್ರತಿ (ಧ್ರುವ TextStmts::Stmt& stmt : m_dtorStmts) puts(stmt.m_text + "\n");
        puts("VL_DO_DANGLING(std::free(const_cast<char*>(vlSelf->vlNamep)), "
             "vlSelf->vlNamep);\n");
        puts("}\n");

        // C++ destructor
        puts("\n" + modName + "::~" + modName + "() {\n");
        emitSystemCSection(modp, VSystemCSectionType::DTOR);
        puts(modName + "__dtor(this);\n");
        puts("}\n");
    }

    ಶೂನ್ಯ emitConfigureImp(ಧ್ರುವ Ast_ಗಂಟು_Module* modp) {
        ಧ್ರುವ string modName = EmitCUtil::prefixNameProtect(modp);

        ಯದಿ (v3Global.opt.coverage()) {
            puts("\n");
            m_lazyDecls.emit("void " + modName + "__", protect("_configure_coverage"),
                             "(" + modName + "* vlSelf, bool first);");
        }

        ಯದಿ (v3Global.opt.coverage()) {
            puts("\nvoid " + modName + "::" + protect("__Vconfigure") + "(bool first) {\n");
            puts("(void)first;  // Prevent unused variable warning\n");
            puts(modName + "__" + protect("_configure_coverage") + "(this, first);\n");
            puts("}\n");
        }
    }
    ಶೂನ್ಯ emitCoverageImp() {
        // Rather than putting out VL_COVER_INSERT calls directly, we do it via this
        // function. This gets around gcc slowness constructing all of the template
        // arguments.
        ಯದಿ (v3Global.opt.coverage()) {
            puts("\n// Coverage\n");
            puts("void " + EmitCUtil::prefixNameProtect(m_modp) + "::__vlCoverInsert(");
            puts(v3Global.opt.threads() > 1 ? "std::atomic<uint32_t>" : "uint32_t");
            puts("* countp, bool enable, bool localCounter, const char* filenamep, int lineno, "
                 "int column,\n");
            puts("const char* hierp, const char* pagep, const char* commentp, const char* "
                 "linescovp,\n");
            puts("const char* fsmVarp, const char* fsmFromp, const char* fsmTop, const char* "
                 "fsmTagp) {\n");
            ಯದಿ (v3Global.opt.threads() > 1) {
                puts("assert(sizeof(uint32_t) == sizeof(std::atomic<uint32_t>));\n");
                puts("uint32_t* count32p = reinterpret_cast<uint32_t*>(countp);\n");
            } ಅನ್ಯಥಾ {
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
        ಯದಿ (v3Global.opt.coverageToggle()) {
            puts("\n// Toggle Coverage\n");
            puts("void " + EmitCUtil::prefixNameProtect(m_modp) + "::__vlCoverToggleInsert(");
            puts("int begin, int end, bool ranged, ");
            puts(v3Global.opt.threads() > 1 ? "std::atomic<uint32_t>" : "uint32_t");
            puts("* countp, bool enable, bool localCounter, const char* filenamep, int lineno, "
                 "int column,\n");
            puts("const char* hierp, const char* pagep, const char* commentp) {\n");
            ಯದಿ (v3Global.opt.threads() > 1) {
                puts("assert(sizeof(uint32_t) == sizeof(std::atomic<uint32_t>));\n");
            }
            puts("int step = (end >= begin) ? 1 : -1;\n");
            // range is inclusive
            puts("for (int i = begin; i != end + step; i += step) {\n");
            puts("for (int j = 0; j < 2; j++) {\n");
            ಯದಿ (v3Global.opt.threads() > 1) {
                puts("uint32_t* count32p = reinterpret_cast<uint32_t*>(countp);\n");
            } ಅನ್ಯಥಾ {
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
    ಶೂನ್ಯ emitSavableImp(ಧ್ರುವ Ast_ಗಂಟು_Module* modp) {
        ಯದಿ (v3Global.opt.savable()) {
            puts("\n// Savable\n");
            ಪ್ರತಿ (ಪೂರ್ಣಾಙ್ಕ de = 0; de < 2; ++de) {
                ಧ್ರುವ string classname = de ? "VerilatedDeserialize" : "VerilatedSerialize";
                ಧ್ರುವ string funcname = de ? "__Vdeserialize" : "__Vserialize";
                ಧ್ರುವ string op = de ? ">>" : "<<";
                // NOLINTNEXTLINE(performance-inefficient-string-concatenation)
                putns(modp, "void " + EmitCUtil::prefixNameProtect(modp) + "::" + protect(funcname)
                                + "(" + classname + "& os) {\n");
                // Place a computed checksum to ensure proper structure save/restore formatting
                // OK if this hash includes some things we won't dump, since
                // just looking for loading the wrong model
                VHashSha512 hash;
                ಪ್ರತಿ (Ast_ಗಂಟು* nodep = modp->stmtsp(); nodep; nodep = nodep->nextp()) {
                    ಯದಿ (ಧ್ರುವ AstVar* ಧ್ರುವ varp = VN_CAST(nodep, Var)) {
                        hash.ಸೇರಿಸು(varp->name());
                        hash.ಸೇರಿಸು(varp->dtypep()->width());
                    }
                }
                ofp()->printf("uint64_t __Vcheckval = 0x%" PRIx64 "ULL;\n",
                              static_cast<uint64_t>(hash.digestUInt64()));
                ಯದಿ (de) {
                    puts("os.readAssert(__Vcheckval);\n");
                } ಅನ್ಯಥಾ {
                    puts("os << __Vcheckval;\n");
                }

                // Save context
                // If multiple models save the same context we'll save it multiple
                // times, but is harmless, and doing it otherwise would break
                // backwards compatibility.
                puts("os " + op + " vlSymsp->_vm_contextp__;\n");

                // Save all members
                ಪ್ರತಿ (Ast_ಗಂಟು* nodep = modp->stmtsp(); nodep; nodep = nodep->nextp()) {
                    ಯದಿ (ಧ್ರುವ AstVar* ಧ್ರುವ varp = VN_CAST(nodep, Var)) {
                        ಯದಿ (varp->isIO() && modp->is_ತುದಿ() && optSystemC()) {
                            // System C top I/O doesn't need loading, as the
                            // lower level subinst code does it.
                        } ಅನ್ಯಥಾ ಯದಿ (varp->isParam()) {
                        } ಅನ್ಯಥಾ ಯದಿ (VN_IS(varp->dtypep(), NBACommitQueueDType)) {
                        } ಅನ್ಯಥಾ {
                            ಪೂರ್ಣಾಙ್ಕ vects = 0;
                            Ast_ಗಂಟು_DType* elementp = varp->dtypeSkipRefp();
                            ಪ್ರತಿ (AstUnpackArrayDType* arrayp = VN_CAST(elementp, UnpackArrayDType);
                                 arrayp; arrayp = VN_CAST(elementp, UnpackArrayDType)) {
                                ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ vecnum = vects++;
                                UASSERT_OBJ(arrayp->hi() >= arrayp->lo(), varp,
                                            "Should have swapped msb & lsb earlier.");
                                ಧ್ರುವ string ivar = "__Vi"s + cvtToStr(vecnum);
                                puts("for (int __Vi" + cvtToStr(vecnum) + " = " + cvtToStr(0));
                                puts("; " + ivar + " < " + cvtToStr(arrayp->elementsConst()));
                                puts("; ++" + ivar + ") {\n");
                                elementp = arrayp->subDTypep()->skipRefp();
                            }
                            ಧ್ರುವ AstBasicDType* ಧ್ರುವ basicp = elementp->basicp();
                            // Do not save MTask state, only matters within an evaluation
                            ಯದಿ (basicp && basicp->keyword().isMTaskState()) ಅನುವರ್ತ;
                            // Want to detect types that are represented as arrays
                            // (i.e. packed types of more than 64 bits).
                            ಯದಿ (elementp->isWide()
                                && !(basicp && basicp->keyword() == VBasicDTypeKwd::STRING)) {
                                ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ vecnum = vects++;
                                ಧ್ರುವ string ivar = "__Vi"s + cvtToStr(vecnum);
                                puts("for (int __Vi" + cvtToStr(vecnum) + " = " + cvtToStr(0));
                                puts("; " + ivar + " < " + cvtToStr(elementp->width_ಪದಗಳು()));
                                puts("; ++" + ivar + ") {\n");
                            }
                            putns(varp, "os" + op + varName(varp));
                            ಪ್ರತಿ (ಪೂರ್ಣಾಙ್ಕ v = 0; v < vects; ++v) puts("[__Vi" + cvtToStr(v) + "]");
                            puts(";\n");
                            ಪ್ರತಿ (ಪೂರ್ಣಾಙ್ಕ v = 0; v < vects; ++v) puts("}\n");
                        }
                    }
                }

                puts("}\n");
            }
        }
    }
    // Predicate to check if we actually need to emit anything into the common implementation file.
    // Used to avoid creating empty output files.
    ತಾರ್ಕಿಕ hasCommonImp(ಧ್ರುವ Ast_ಗಂಟು_Module* modp) ಧ್ರುವ {
        // Nothing to emit if no module!
        ಯದಿ (!modp) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
        // We always need the slow file
        ಯದಿ (m_slow) ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;
        // The fast file is only required when we have `systemc_implementation nodes
        ಪ್ರತಿ (ಧ್ರುವ AstSystemCSection* scSectionp = modp->scSectionsp(); scSectionp;
             scSectionp = VN_AS(scSectionp->nextp(), SystemCSection)) {
            ಯದಿ (scSectionp->sectionType() == VSystemCSectionType::IMP) ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;
        }
        ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
    }
    // Actually emit common implementation contents for given AstNodeModule
    ಶೂನ್ಯ doCommonImp(ಧ್ರುವ Ast_ಗಂಟು_Module* modp) {
        ಯದಿ (m_slow) {
            ಯದಿ (!VN_IS(modp, Class)) {
                emitParamDefns(modp);
                emitCtorDtorImp(modp);
                emitConfigureImp(modp);
                emitCoverageImp();
            }
            emitSavableImp(modp);
        } ಅನ್ಯಥಾ {
            // From `systemc_implementation
            emitSystemCSection(modp, VSystemCSectionType::IMP);
        }
    }
    ಶೂನ್ಯ emitCommonImp(ಧ್ರುವ Ast_ಗಂಟು_Module* modp) {
        ಧ್ರುವ AstClass* ಧ್ರುವ classp
            = VN_IS(modp, ClassPackage) ? VN_AS(modp, ClassPackage)->classp() : nullptr;

        ಯದಿ (hasCommonImp(modp) || hasCommonImp(classp)) {
            // Create ctor/dtor statements. If these need splitting, they will be emitted
            // during construction so do this before opening the output file.
            ಯದಿ (m_slow && !VN_IS(modp, Class)) createCtorDtorStmts(modp);

            openNextOutputFile(m_fileBaseName);

            doCommonImp(modp);
            ಯದಿ (classp) {
                VL_RESTORER(m_modp);
                m_modp = classp;
                doCommonImp(classp);
            }

            closeOutputFile();
        }
    }
    ಶೂನ್ಯ emitCFuncImp(ಧ್ರುವ Ast_ಗಂಟು_Module* modp) {
        // Functions to be emitted here
        std::vector<AstCFunc*> funcps;

        ಧ್ರುವ auto gather = [ಏತತ್, &funcps](ಧ್ರುವ Ast_ಗಂಟು_Module* modp) {
            ಪ್ರತಿ (Ast_ಗಂಟು* nodep = modp->stmtsp(); nodep; nodep = nodep->nextp()) {
                AstCFunc* ಧ್ರುವ funcp = VN_CAST(nodep, CFunc);
                ಯದಿ (!funcp) ಅನುವರ್ತ;
                // TRACE_* and DPI handled elsewhere
                ಯದಿ (funcp->isTrace()) ಅನುವರ್ತ;
                ಯದಿ (funcp->dpiImportPrototype()) ಅನುವರ್ತ;
                ಯದಿ (funcp->dpiExportDispatcher()) ಅನುವರ್ತ;
                ಯದಿ (funcp->slow() != m_slow) ಅನುವರ್ತ;
                funcps.ತಳ್ಳು_back(funcp);
            }
        };

        gather(modp);
        VL_RESTORER(m_classOrPackage);
        ಯದಿ (ಧ್ರುವ AstClassPackage* ಧ್ರುವ packagep = VN_CAST(modp, ClassPackage)) {
            m_classOrPackage = packagep;
            gather(packagep->classp());
        }

        // Do not create empty files
        ಯದಿ (funcps.empty()) ಪ್ರತ್ಯಾಗಮ;

        // Open output file
        openNextOutputFile(m_uniqueNames.get(m_fileBaseName));
        // Emit all functions
        ಪ್ರತಿ (AstCFunc* ಧ್ರುವ funcp : funcps) {
            VL_RESTORER(m_modp);
            m_modp = EmitCParentModule::get(funcp);
            iterateConst(funcp);
        }
        // Close output file
        closeOutputFile();
    }

    // VISITORS
    ಶೂನ್ಯ visit(AstCFunc* nodep) override {
        ಯದಿ (splitNeeded()) {
            // Splitting file, so using parallel build.
            v3Global.useParallelBuild(ಸತ್ಯ);
            // Close old file
            closeOutputFile();
            // Open a new file
            openNextOutputFile(m_uniqueNames.get(m_fileBaseName));
        }

        EmitCFunc::visit(nodep);
    }

    explicit EmitCImp(ಧ್ರುವ Ast_ಗಂಟು_Module* modp, ತಾರ್ಕಿಕ slow)
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
    ~EmitCImp() override = ಸ್ವಭಾವ;

ಸಾರ್ವಜನಿಕ:
    ಸ್ಥಿರ std::vector<AstCFile*> main(ಧ್ರುವ Ast_ಗಂಟು_Module* modp, ತಾರ್ಕಿಕ slow) VL_MT_STABLE {
        EmitCImp emitCImp{modp, slow};
        ಪ್ರತ್ಯಾಗಮ emitCImp.getAndClearCfileps();
    }
};

//######################################################################
// Tracing routines

// Trace type descriptors go in a different file as it needs to be written in
// parallel with the actual trace function source files
ವರ್ಗ EmitCTraceTypes final : ಸಾರ್ವಜನಿಕ EmitCFunc {
    // NODE STATE/TYPES
    // None allowed to support threaded emitting

    // STATE
    ಪೂರ್ಣಾಙ್ಕ m_enum_ಅಂಕೆ = 0;  // Enumeration number (whole netlist)
    std::unordered_map<Ast_ಗಂಟು*, ಪೂರ್ಣಾಙ್ಕ> m_enum_ಅಂಕೆ_Map;  // EnumDType to enumeration number
    ಪೂರ್ಣಾಙ್ಕ m_traceTypeSubs = 0;  // Number of trace type declaration sub-functions
    V3UniqueNames m_uniqueNames;  // Generates unique file names
    ಧ್ರುವ std::string m_fileBaseName = EmitCUtil::ತುದಿ_Class_Name() + "_" + protect("_TraceDecls");
    // This one uses CSplitTrace for file splitting, which is incorrect but historically accurates
    ಧ್ರುವ size_t m_splitLimit = v3Global.opt.outputSplitCTrace()
                                    ? static_cast<size_t>(v3Global.opt.outputSplitCTrace())
                                    : std::numeric_limits<size_t>::max();

    ಶೂನ್ಯ openNextOutputFile() {
        openNewOutputSourceFile(m_uniqueNames.get(m_fileBaseName), ಸತ್ಯ, ಸತ್ಯ,
                                "Tracing declarations");
        puts("\n");
        ಪ್ರತಿ (ಧ್ರುವ std::string& base : v3Global.opt.traceSourceLangs()) {
            puts("#include \"" + base + ".h\"\n");
        }
        puts("\n");
        puts("\nvoid " + EmitCUtil::prefixNameProtect(m_modp) + "__"
             + protect("traceDeclTypesSub" + std::to_string(m_traceTypeSubs++)) + "("
             + v3Global.opt.traceClassBase() + "* tracep) {\n");
    }

ಸಾರ್ವಜನಿಕ:
    // METHODS
    ಪೂರ್ಣಾಙ್ಕ get_Enum_Map_ಅಂಕೆ(AstEnumDType* nodep) {
        ಪೂರ್ಣಾಙ್ಕ& enumNumr = m_enum_ಅಂಕೆ_Map[nodep];
        ಯದಿ (!enumNumr) {
            ಯದಿ (splitNeeded(m_splitLimit)) {
                // Splitting file, so using parallel build.
                v3Global.useParallelBuild(ಸತ್ಯ);
                puts("}\n");
                closeOutputFile();
                openNextOutputFile();
            }

            enumNumr = ++m_enum_ಅಂಕೆ;
            ಪೂರ್ಣಾಙ್ಕ nvals = 0;
            puts("{\n");
            putns(nodep, "const char* " + protect("__VenumItemNames") + "[]\n");
            puts("= {");
            ಪ್ರತಿ (AstEnumItem* itemp = nodep->itemsp(); itemp;
                 itemp = VN_AS(itemp->nextp(), EnumItem)) {
                ಯದಿ (++nvals > 1) puts(", ");
                putbs("\"" + itemp->prettyName() + "\"");
            }
            puts("};\n");
            nvals = 0;
            puts("const char* " + protect("__VenumItemValues") + "[]\n");
            puts("= {");
            ಪ್ರತಿ (AstEnumItem* itemp = nodep->itemsp(); itemp;
                 itemp = VN_AS(itemp->nextp(), EnumItem)) {
                AstConst* ಧ್ರುವ constp = VN_AS(itemp->valuep(), Const);
                ಯದಿ (++nvals > 1) puts(", ");
                putbs("\"" + constp->ಅಂಕೆ().displayed(nodep, "%0b") + "\"");
            }
            puts("};\n");
            puts("tracep->declDTypeEnum(" + std::to_string(enumNumr) + ", \"" + nodep->prettyName()
                 + "\", " + std::to_string(nvals) + ", " + std::to_string(nodep->widthMin()) + ", "
                 + protect("__VenumItemNames") + ", " + protect("__VenumItemValues") + ");\n");
            puts("}\n");
            splitSizeInc(Ast_ಗಂಟು::INSTR_ಎಣಿಕೆ_CALL);
        }
        ಪ್ರತ್ಯಾಗಮ enumNumr;
    }

    // Close output file
    ಶೂನ್ಯ finalize() {
        // Close function definition
        puts("}\n");

        ಧ್ರುವ std::string modName = EmitCUtil::prefixNameProtect(m_modp);
        ಧ್ರುವ std::string args = v3Global.opt.traceClassBase() + "* tracep";

        // Forward declarations for subs in other files
        ಪ್ರತಿ (ಪೂರ್ಣಾಙ್ಕ i = 0; i < m_traceTypeSubs - 1; ++i) {
            puts("void " + modName + "__" + protect("traceDeclTypesSub" + std::to_string(i)) + "("
                 + args + ");\n");
        }

        // Create top level trace_decl_types function and call each sub-function
        puts("\nvoid " + modName + "__" + protect("trace_decl_types") + "(" + args + ") {\n");
        ಪ್ರತಿ (ಪೂರ್ಣಾಙ್ಕ i = 0; i < m_traceTypeSubs; ++i) {
            puts(modName + "__" + protect("traceDeclTypesSub" + std::to_string(i))
                 + "(tracep);\n");
        }
        puts("}\n");

        closeOutputFile();
    }

    EmitCTraceTypes() {
        m_modp = v3Global.rootp()->ತುದಿ_Modulep();
        openNextOutputFile();
    }
    ~EmitCTraceTypes() override = ಸ್ವಭಾವ;
};

ವರ್ಗ EmitCTrace final : ಸಾರ್ವಜನಿಕ EmitCFunc {
    // NODE STATE/TYPES
    // None allowed to support threaded emitting

    // MEMBERS
    ಧ್ರುವ ತಾರ್ಕಿಕ m_slow;  // Making slow file
    ಧ್ರುವ std::unique_ptr<EmitCTraceTypes> m_emitTypesp{m_slow ? ನೂತನ EmitCTraceTypes{} : nullptr};
    V3UniqueNames m_uniqueNames;  // Generates unique file names
    ಧ್ರುವ std::string m_fileBaseName = EmitCUtil::ತುದಿ_Class_Name() + "_" + protect("_Trace");

    // METHODS
    ಶೂನ್ಯ openNextOutputFile() {
        openNewOutputSourceFile(m_uniqueNames.get(m_fileBaseName), m_slow, ಸತ್ಯ,
                                "Tracing implementation internals");
        puts("\n");
        ಪ್ರತಿ (ಧ್ರುವ std::string& base : v3Global.opt.traceSourceLangs()) {
            puts("#include \"" + base + ".h\"\n");
        }
        puts("#include \"" + EmitCUtil::symClassName() + ".h\"\n");
        puts("\n");
        // Need to emit new lazy declarations
        m_lazyDecls.reset();
    }

    ತಾರ್ಕಿಕ emitTraceIsScBv(ಧ್ರುವ AstTraceInc* nodep) {
        ಧ್ರುವ AstVarRef* ಧ್ರುವ varrefp = VN_CAST(nodep->declp()->valuep(), VarRef);
        ಯದಿ (!varrefp) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
        ಧ್ರುವ AstVar* ಧ್ರುವ varp = varrefp->varp();
        ಪ್ರತ್ಯಾಗಮ varp->isSc() && varp->isScBv();
    }

    ತಾರ್ಕಿಕ emitTraceIsScBigUint(ಧ್ರುವ AstTraceInc* nodep) {
        ಧ್ರುವ AstVarRef* ಧ್ರುವ varrefp = VN_CAST(nodep->declp()->valuep(), VarRef);
        ಯದಿ (!varrefp) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
        ಧ್ರುವ AstVar* ಧ್ರುವ varp = varrefp->varp();
        ಪ್ರತ್ಯಾಗಮ varp->isSc() && varp->isScBigUint();
    }

    ತಾರ್ಕಿಕ emitTraceIsScUint(ಧ್ರುವ AstTraceInc* nodep) {
        ಧ್ರುವ AstVarRef* ಧ್ರುವ varrefp = VN_CAST(nodep->declp()->valuep(), VarRef);
        ಯದಿ (!varrefp) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
        ಧ್ರುವ AstVar* ಧ್ರುವ varp = varrefp->varp();
        ಪ್ರತ್ಯಾಗಮ varp->isSc() && (varp->isScUint() || varp->isScUintBool());
    }

    ಶೂನ್ಯ emitTraceInitOne(ಧ್ರುವ AstTraceDecl* nodep, ಪೂರ್ಣಾಙ್ಕ enum_ಅಂಕೆ) {
        std::string direction;
        direction = nodep->declDirection().traceSigDirection();

        AstCCall* ಧ್ರುವ callp = nodep->dtypeCallp();
        ಯದಿ (callp) {
            callp->argTypes(callp->argTypes() + ", " + cvtToStr(nodep->fidx()) + ", c+"
                            + cvtToStr(nodep->code()) + ", " + direction);
            ಪ್ರತ್ಯಾಗಮ;
        }

        ಯದಿ (nodep->dtypep()->basicp()->isDouble()) {
            puts("VL_TRACE_DECL_DOUBLE");
        } ಅನ್ಯಥಾ ಯದಿ (nodep->isWide()) {
            puts("VL_TRACE_DECL_WIDE");
        } ಅನ್ಯಥಾ ಯದಿ (nodep->isQuad()) {
            puts("VL_TRACE_DECL_QUAD");
        } ಅನ್ಯಥಾ ಯದಿ (nodep->bitRange().ranged()) {
            puts("VL_TRACE_DECL_BUS");
        } ಅನ್ಯಥಾ ಯದಿ (nodep->dtypep()->basicp()->isEvent()) {
            puts("VL_TRACE_DECL_EVENT");
        } ಅನ್ಯಥಾ {
            puts("VL_TRACE_DECL_BIT");
        }

        ಯದಿ (nodep->arrayRange().ranged()) {
            puts("_ARRAY(tracep");
        } ಅನ್ಯಥಾ {
            puts("(tracep");
        }

        // Code
        puts(",c+" + cvtToStr(nodep->code()));
        ಯದಿ (nodep->arrayRange().ranged()) puts("+i*" + cvtToStr(nodep->width_ಪದಗಳು()));

        // Function index
        puts(",");
        ಯದಿ (nodep->inDtypeFunc()) {
            puts("fidx");
        } ಅನ್ಯಥಾ {
            puts(cvtToStr(nodep->fidx()));
        }

        // Name
        puts(",");
        putsQuoted(VIdProtect::protect_ಪದಗಳು_If(nodep->showname(), nodep->protect()));

        // Enum number
        puts("," + cvtToStr(enum_ಅಂಕೆ));

        // Direction
        ಯದಿ (nodep->inDtypeFunc()) {
            puts(", direction");
        } ಅನ್ಯಥಾ {
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
        ಯದಿ (nodep->arrayRange().ranged()) {
            ಯದಿ (nodep->arrayRange().ascending()) {
                puts(", (i + " + std::to_string(nodep->arrayRange().lo()) + ")");
            } ಅನ್ಯಥಾ {
                puts(", (" + std::to_string(nodep->arrayRange().hi()) + " - i)");
            }
        }

        // Bit range
        ಯದಿ (!nodep->dtypep()->basicp()->isDouble() && nodep->bitRange().ranged()) {
            puts(", " + cvtToStr(nodep->bitRange().ಎಡ()) + ","
                 + cvtToStr(nodep->bitRange().ಬಲ()));
        }

        //
        puts(");");
    }

    ಪೂರ್ಣಾಙ್ಕ emitTraceDeclDType(Ast_ಗಂಟು_DType* nodep) {
        // Return enum number or -1 for none
        ಯದಿ (v3Global.opt.traceEnabledFst()) {
            // Skip over refs-to-refs, but stop before final ref so can get data type name
            // Alternatively back in V3Width we could push enum names from upper typedefs
            ಯದಿ (AstEnumDType* ಧ್ರುವ enump = VN_CAST(nodep->skipRefToEnump(), EnumDType)) {
                ಪ್ರತ್ಯಾಗಮ m_emitTypesp->get_Enum_Map_ಅಂಕೆ(enump);
            }
        }
        ಪ್ರತ್ಯಾಗಮ -1;
    }

    ಶೂನ್ಯ emitTraceChangeOne(AstTraceInc* nodep, ಪೂರ್ಣಾಙ್ಕ arrayindex) {
        // Note: Both VTraceType::CHANGE and VTraceType::FULL use the 'full' methods
        ಧ್ರುವ std::string func = nodep->traceType() == VTraceType::CHANGE ? "chg" : "full";
        ತಾರ್ಕಿಕ emitWidth = ಸತ್ಯ;
        string stype;
        ಯದಿ (nodep->dtypep()->basicp()->isDouble()) {
            stype = "Double";
            emitWidth = ಅಸತ್ಯ;
        } ಅನ್ಯಥಾ ಯದಿ (nodep->isWide() || emitTraceIsScBv(nodep) || emitTraceIsScBigUint(nodep)) {
            stype = "WData";
        } ಅನ್ಯಥಾ ಯದಿ (nodep->isQuad()) {
            stype = "QData";
        } ಅನ್ಯಥಾ ಯದಿ (nodep->declp()->widthMin() > 16) {
            stype = "IData";
        } ಅನ್ಯಥಾ ಯದಿ (nodep->declp()->widthMin() > 8) {
            stype = "SData";
        } ಅನ್ಯಥಾ ಯದಿ (nodep->declp()->widthMin() > 1) {
            stype = "CData";
        } ಅನ್ಯಥಾ ಯದಿ (nodep->dtypep()->basicp()->isEvent()) {
            stype = "Event";
            emitWidth = ಅಸತ್ಯ;
        } ಅನ್ಯಥಾ {
            stype = "Bit";
            emitWidth = ಅಸತ್ಯ;
        }
        putns(nodep, "bufp->" + func + stype);

        ಧ್ರುವ uint32_t offset = (arrayindex < 0) ? 0 : (arrayindex * nodep->declp()->width_ಪದಗಳು());
        ಧ್ರುವ uint32_t code = nodep->declp()->code() + offset;
        // Note: Both VTraceType::CHANGE and VTraceType::FULL use the 'full' methods
        puts("(oldp+");
        puts(cvtToStr(code - nodep->baseCode()));
        puts(",");
        ಧ್ರುವ VNumRange& arrayRange = nodep->declp()->arrayRange();
        ಯದಿ (arrayRange.ranged() && !arrayRange.ascending()) {
            arrayindex = arrayRange.elements() - 1 - arrayindex;
        }
        emit_Trace_ಮೌಲ್ಯ(nodep, arrayindex);
        ಯದಿ (emitWidth) puts("," + cvtToStr(nodep->declp()->widthMin()));
        puts(");\n");
    }

    ಶೂನ್ಯ emit_Trace_ಮೌಲ್ಯ(ಧ್ರುವ AstTraceInc* nodep, ಪೂರ್ಣಾಙ್ಕ arrayindex) {
        ಯದಿ (AstVarRef* ಧ್ರುವ varrefp = VN_CAST(nodep->valuep(), VarRef)) {
            ಧ್ರುವ AstVar* ಧ್ರುವ varp = varrefp->varp();
            ಯದಿ (varp->isEvent()) puts("&");
            puts("(");
            ಯದಿ (emitTraceIsScBigUint(nodep)) {
                puts("(uint32_t*)");
            } ಅನ್ಯಥಾ ಯದಿ (emitTraceIsScBv(nodep)) {
                puts("VL_SC_BV_DATAP(");
            }
            iterateConst(varrefp);  // Put var name out
            emitTraceIndex(nodep, arrayindex);
            ಯದಿ (varp->isSc()) puts(".read()");
            ಯದಿ (emitTraceIsScUint(nodep)) {
                puts(nodep->isQuad() ? ".to_uint64()" : ".to_uint()");
            } ಅನ್ಯಥಾ ಯದಿ (emitTraceIsScBigUint(nodep)) {
                puts(".get_raw()");
            } ಅನ್ಯಥಾ ಯದಿ (emitTraceIsScBv(nodep)) {
                puts(")");
            }
            puts(")");
        } ಅನ್ಯಥಾ {
            puts("(");
            iterateConst(nodep->valuep());
            emitTraceIndex(nodep, arrayindex);
            puts(")");
        }
    }

    ಶೂನ್ಯ emitTraceIndex(ಧ್ರುವ AstTraceInc* ಧ್ರುವ nodep, ಪೂರ್ಣಾಙ್ಕ arrayindex) {
        // Tracing only supports 1D arrays
        ಯದಿ (nodep->declp()->arrayRange().ranged()) {
            ಯದಿ (arrayindex == -2) {
                puts("[i]");
            } ಅನ್ಯಥಾ ಯದಿ (arrayindex == -1) {
                puts("[0]");
            } ಅನ್ಯಥಾ {
                puts("[" + cvtToStr(arrayindex) + "]");
            }
        }
    }

    // VISITORS
    using EmitCFunc::visit;  // Suppress hidden overloaded virtual function warning
    ಶೂನ್ಯ visit(AstCFunc* nodep) override {
        ಯದಿ (!nodep->isTrace()) ಪ್ರತ್ಯಾಗಮ;
        ಯದಿ (nodep->slow() != m_slow) ಪ್ರತ್ಯಾಗಮ;

        ಯದಿ (splitNeeded()) {
            // Splitting file, so using parallel build.
            v3Global.useParallelBuild(ಸತ್ಯ);
            // Close old file
            closeOutputFile();
            // Open a new file
            openNextOutputFile();
        }

        EmitCFunc::visit(nodep);
    }
    ಶೂನ್ಯ visit(Ast_Trace_ತಳ್ಳು_Prefix* nodep) override {
        putns(nodep, "VL_TRACE_PUSH_PREFIX(tracep, ");
        ಯದಿ (nodep->quotedPrefix()) {
            putsQuoted(VIdProtect::protect_ಪದಗಳು_If(nodep->prefix(), nodep->protect()));
        } ಅನ್ಯಥಾ {
            puts(nodep->prefix());
        }
        puts(", VerilatedTracePrefixType::");
        puts(nodep->prefixType().ascii());
        puts(", " + std::to_string(nodep->ಎಡ()));
        puts(", " + std::to_string(nodep->ಬಲ()));
        puts(");\n");
    }
    ಶೂನ್ಯ visit(Ast_Trace_ತೆಗೆ_Prefix* nodep) override {  //
        putns(nodep, "VL_TRACE_POP_PREFIX(tracep);\n");
    }
    ಶೂನ್ಯ visit(AstTraceDecl* nodep) override {
        ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ enum_ಅಂಕೆ = emitTraceDeclDType(nodep->dtypep());
        putns(nodep, "");
        ಯದಿ (nodep->arrayRange().ranged()) {
            puts("for (int i = 0; i < " + cvtToStr(nodep->arrayRange().elements()) + "; ++i) {\n");
            emitTraceInitOne(nodep, enum_ಅಂಕೆ);
            puts("\n}\n");
        } ಅನ್ಯಥಾ {
            emitTraceInitOne(nodep, enum_ಅಂಕೆ);
            puts("\n");
        }
    }
    ಶೂನ್ಯ visit(AstTraceInc* nodep) override {
        ಯದಿ (nodep->declp()->arrayRange().ranged()) {
            // It traces faster if we unroll the loop
            ಪ್ರತಿ (ಪೂರ್ಣಾಙ್ಕ i = 0; i < nodep->declp()->arrayRange().elements(); i++) {
                emitTraceChangeOne(nodep, i);
            }
        } ಅನ್ಯಥಾ {
            emitTraceChangeOne(nodep, -1);
        }
    }

    explicit EmitCTrace(ತಾರ್ಕಿಕ slow)
        : m_slow{slow} {
        m_modp = v3Global.rootp()->ತುದಿ_Modulep();
        // Open output file
        openNextOutputFile();
        // Emit functions
        ಪ್ರತಿ (Ast_ಗಂಟು* nodep = m_modp->stmtsp(); nodep; nodep = nodep->nextp()) {
            ಯದಿ (AstCFunc* ಧ್ರುವ funcp = VN_CAST(nodep, CFunc)) iterateConst(funcp);
        }
        // Close output file
        closeOutputFile();
        ಯದಿ (m_slow) m_emitTypesp->finalize();
    }
    ~EmitCTrace() override = ಸ್ವಭಾವ;

ಸಾರ್ವಜನಿಕ:
    ಸ್ಥಿರ std::vector<AstCFile*> main(ತಾರ್ಕಿಕ slow) VL_MT_STABLE {
        EmitCTrace emitCTrace{slow};
        std::vector<AstCFile*> cfileps = emitCTrace.getAndClearCfileps();
        ಯದಿ (slow) {
            ಪ್ರತಿ (AstCFile* ಧ್ರುವ cfilep : emitCTrace.m_emitTypesp->getAndClearCfileps()) {
                cfileps.emplace_back(cfilep);
            }
        }
        ಪ್ರತ್ಯಾಗಮ cfileps;
    }
};

//######################################################################
// Existing AstCFile emitter

ವರ್ಗ EmitCFile final : ಸಾರ್ವಜನಿಕ EmitCFunc {
    explicit EmitCFile(AstCFile* cfilep) {
        openOutputFile(cfilep, "Generated C++");
        iterateConst(cfilep->tblockp());
        closeOutputFile();
    }
    ~EmitCFile() override = ಸ್ವಭಾವ;

ಸಾರ್ವಜನಿಕ:
    ಸ್ಥಿರ ಶೂನ್ಯ main(AstCFile* cfilep) VL_MT_STABLE {
        ಯದಿ (!cfilep->tblockp()) ಪ್ರತ್ಯಾಗಮ;
        EmitCFile{cfilep};
    }
};

//######################################################################
// EmitC class functions

ಶೂನ್ಯ V3EmitC::emitcImp() {
    UINFO(2, __FUNCTION__ << ":");
    std::list<std::vector<AstCFile*>> cfiles;
    {
        // Make parent module pointers available.
        ಧ್ರುವ EmitCParentModule emitCParentModule;
        V3ThreadScope threadScope;

        // Process each module in turn
        ಪ್ರತಿ (ಧ್ರುವ Ast_ಗಂಟು* nodep = v3Global.rootp()->modulesp(); nodep; nodep = nodep->nextp()) {
            ಯದಿ (VN_IS(nodep, Class)) ಅನುವರ್ತ;  // Imped with ClassPackage
            ಧ್ರುವ Ast_ಗಂಟು_Module* ಧ್ರುವ modp = VN_AS(nodep, ಗಂಟು_Module);
            ಯದಿ (modp->isConstPool()) ಅನುವರ್ತ;  // Emitted by V3EmitCConstPool
            cfiles.emplace_back();
            std::vector<AstCFile*>& slow = cfiles.back();
            threadScope.enqueue([modp, &slow] { slow = EmitCImp::main(modp, /* slow: */ ಸತ್ಯ); });
            cfiles.emplace_back();
            std::vector<AstCFile*>& fast = cfiles.back();
            threadScope.enqueue([modp, &fast] { fast = EmitCImp::main(modp, /* slow: */ ಅಸತ್ಯ); });
        }

        // Emit trace routines (currently they can only exist in the top module)
        ಯದಿ (v3Global.opt.trace() && !v3Global.opt.lintOnly()) {
            cfiles.emplace_back();
            std::vector<AstCFile*>& slow = cfiles.back();
            threadScope.enqueue([&slow] { slow = EmitCTrace::main(/* slow: */ ಸತ್ಯ); });
            cfiles.emplace_back();
            std::vector<AstCFile*>& fast = cfiles.back();
            threadScope.enqueue([&fast] { fast = EmitCTrace::main(/* slow: */ ಅಸತ್ಯ); });
        }
    }
    // Add files to netlist
    ಪ್ರತಿ (ಧ್ರುವ std::vector<AstCFile*>& cfileps : cfiles) {
        ಪ್ರತಿ (AstCFile* ಧ್ರುವ cfilep : cfileps) v3Global.rootp()->addFilesp(cfilep);
    }
}

ಶೂನ್ಯ V3EmitC::emitcFiles() {
    UINFO(2, __FUNCTION__ << ":");
    ಪ್ರತಿ (Ast_ಗಂಟು_File *filep = v3Global.rootp()->filesp(), *nextp; filep; filep = nextp) {
        nextp = VN_AS(filep->nextp(), ಗಂಟು_File);
        ಯದಿ (AstCFile* ಧ್ರುವ cfilep = VN_CAST(filep, CFile)) EmitCFile::main(cfilep);
    }
}
