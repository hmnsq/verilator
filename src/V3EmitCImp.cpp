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
    ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_Module* ಸ್ಥಿರಾಂಕ m_fileModp;
    ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ m_slow;  // Creating __Slow file
    V3_Unique_ಹೆಸರುಗಳು m_unique_ಹೆಸರುಗಳು;  // Generates unique file names
    ಸ್ಥಿರಾಂಕ std::string m_file_ತಳ_Name = EmitCUtil::ಪೂರ್ವಪ್ರತ್ಯಯ_Name_Protect(m_fileModp);
    ಪಠ್ಯ_Stmts m_ctorStmts;  // Statements constructing members in 'ctor'
    ಪಠ್ಯ_Stmts m_dtorStmts;  // Statements destroying members in 'dtor'
    std::string m_ctor_ವಿಭಜಿಸು_Decl;  // Declaration of sub-function 'ctor' was split into, if any
    std::string m_dtor_ವಿಭಜಿಸು_Decl;  // Declaration of sub-function 'dtor' was split into, if any

    // METHODS
    ಶೂನ್ಯ open_ಮುಂದಿನ_Output_File(ಸ್ಥಿರಾಂಕ std::string& fileName) {
        open_New_Output_ಮೂಲ_File(fileName, m_slow, ಸುಳ್ಳು, "Design implementation internals");
        emit_File_ಶೀರ್ಷಿಕೆ();
    }
    ಶೂನ್ಯ emit_File_ಶೀರ್ಷಿಕೆ() {
        puts("// See " + EmitCUtil::ತುದಿ_Class_Name() + ".h for the primary calling header\n");
        puts("\n");
        puts("#include \"" + EmitCUtil::pchClassName() + ".h\"\n");
        emitSystemCSection(m_fileModp, VSystemCSectionType::IMP_HDR);
        // Need to emit new lazy declarations
        m_lazyDecls.reset();
    }

    ಶೂನ್ಯ emitParamDefns(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_Module* modp) {
        ತಾರ್ಕಿಕ ಮೊದಲ = ನಿಜ;
        ಗಾಗಿ (ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* nodep = modp->stmtsp(); nodep; nodep = nodep->nextp()) {
            ಆದರೆ (ಸ್ಥಿರಾಂಕ AstVar* ಸ್ಥಿರಾಂಕ varp = VN_CAST(nodep, Var)) {
                ಆದರೆ (varp->isParam()) {
                    ಆದರೆ (ಮೊದಲ) {
                        puts("\n");
                        ಸ್ಥಿರಾಂಕ string modName = EmitCUtil::ಪೂರ್ವಪ್ರತ್ಯಯ_Name_Protect(modp);
                        putsDecoration(modp, "// Parameter definitions for " + modName + "\n");
                        ಮೊದಲ = ಸುಳ್ಳು;
                    }
                    ಆದರೆ (varp->dtypep()->isLiteralType()) putns(varp, "VL_CONSTINIT_CXX20 ");
                    putns(varp, "const ");
                    emitVarDefn(varp);
                    puts(";\n");
                }
            }
        }
        ಆದರೆ (!ಮೊದಲ) puts("\n");
    }
    // Add statements explicitly constructing, or destroying, a member AstVar
    ಸ್ಥಿರ ಶೂನ್ಯ ಕೂಡಿಸು_Var_Ctor_Dtor_Stmts(ಪಠ್ಯ_Stmts& stmts, ಸ್ಥಿರಾಂಕ AstVar* varp, ತಾರ್ಕಿಕ destroy) {
        ಸ್ಥಿರಾಂಕ std::string name = "vlSelf->" + varName(varp);

        // SystemC port arrays are C arrays, so construct them element by element. They are
        // default constructed (unnamed), as the elements of an array member would be.
        ಆದರೆ (varp->isSc() && VN_IS(varp->dtypeSkipRefp(), UnpackArrayDType)) {
            std::ಸದಿಶ<std::string> elems{name};
            ಗಾಗಿ (ಸ್ಥಿರಾಂಕ AstUnpackArrayDType* adtypep
                 = VN_CAST(varp->dtypeSkipRefp(), UnpackArrayDType);
                 adtypep; adtypep = VN_CAST(adtypep->subDTypep()->skipRefp(), UnpackArrayDType)) {
                std::ಸದಿಶ<std::string> subElems;
                ಗಾಗಿ (ಸ್ಥಿರಾಂಕ std::string& elem : elems) {
                    ಗಾಗಿ (ಪೂರ್ಣಾಂಕ i = 0; i < adtypep->elementsConst(); ++i) {
                        subElems.ತಳ್ಳು_ಹಿಂಭಾಗ(elem + "[" + std::ಗಮ್ಯ_string(i) + "]");
                    }
                }
                elems = std::move(subElems);
            }
            ಆದರೆ (destroy) std::ಹಿಮ್ಮುಖ(elems.begin(), elems.ಕೊನೆ());
            ಗಾಗಿ (ಸ್ಥಿರಾಂಕ std::string& e : elems) {
                stmts.ಕೂಡಿಸು((destroy ? "VL_INPLACE_DESTRUCT(" : "VL_INPLACE_CONSTRUCT(") + e + ");");
            }
            ಹಿಂತಿರುಗಿ;
        }

        // Other variables are constructed whole, even if aggregates
        ಆದರೆ (destroy) {
            stmts.ಕೂಡಿಸು("VL_INPLACE_DESTRUCT(" + name + ");");
            ಹಿಂತಿರುಗಿ;
        }
        ಸ್ಥಿರಾಂಕ AstBasicDType* ಸ್ಥಿರಾಂಕ basicp = VN_CAST(varp->dtypeSkipRefp(), BasicDType);
        std::string ವಾದಗಳು;
        ಆದರೆ (varp->isSc()) {
            // SystemC ports need their name
            ವಾದಗಳು = ", \"" + V3_ಹೊರಹೋಗು_Formatter::quoteNameControls(varp->nameProtect()) + "\"";
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (basicp && basicp->keyword().isMTaskState()) {
            // MTask state needs the number of upstream dependencies to wait for
            ವಾದಗಳು = ", " + VN_AS(varp->valuep(), Const)->ಅಂಕೆ().emitC();
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (basicp && basicp->isDelayScheduler()) {
            // Delay scheduler needs the context pointer
            ವಾದಗಳು = ", *vlSelf->vlSymsp->_vm_contextp__";
        }
        stmts.ಕೂಡಿಸು("VL_INPLACE_CONSTRUCT(" + name + ವಾದಗಳು + ");");
    }
    // Gather the statements constructing/destroying members in anonymous unions, and split
    // them into sub-functions if needed. As this emits files, no output file can be open.
    ಶೂನ್ಯ createCtorDtorStmts(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_Module* modp) {
        // Construct member variables
        ಗಾಗಿ (ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* nodep = modp->stmtsp(); nodep; nodep = nodep->nextp()) {
            ಸ್ಥಿರಾಂಕ AstVar* ಸ್ಥಿರಾಂಕ varp = VN_CAST(nodep, Var);
            ಆದರೆ (varp && isExplicitlyConstructed(varp)) {
                ಕೂಡಿಸು_Var_Ctor_Dtor_Stmts(m_ctorStmts, varp, ಸುಳ್ಳು);
            }
        }

        // Destroy in reverse order
        ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ lastp = modp->stmtsp() ? modp->stmtsp()->lastp() : ಶೂನ್ಯಸೂಚಕ;
        ಗಾಗಿ (ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* nodep = lastp; nodep; nodep = nodep->prevp()) {
            ಸ್ಥಿರಾಂಕ AstVar* ಸ್ಥಿರಾಂಕ varp = VN_CAST(nodep, Var);
            ಆದರೆ (varp && isExplicitlyConstructed(varp)) {
                ಕೂಡಿಸು_Var_Ctor_Dtor_Stmts(m_dtorStmts, varp, ನಿಜ);
            }
        }

        // Split into sub-functions if needed
        ಸ್ಥಿರಾಂಕ std::string modName = EmitCUtil::ಪೂರ್ವಪ್ರತ್ಯಯ_Name_Protect(modp);
        std::tie(m_ctor_ವಿಭಜಿಸು_Decl, m_dtor_ವಿಭಜಿಸು_Decl) = emit_ವಿಭಜಿಸು_Ctor_Dtor(
            m_ctorStmts, m_dtorStmts, 0, modName, modName + "* vlSelf", "vlSelf", ಸುಳ್ಳು,
            "Design implementation internals", [ಇದು]() { emit_File_ಶೀರ್ಷಿಕೆ(); });
    }
    ಶೂನ್ಯ emitCtorDtorImp(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_Module* modp) {
        ಸ್ಥಿರಾಂಕ std::string modName = EmitCUtil::ಪೂರ್ವಪ್ರತ್ಯಯ_Name_Protect(modp);

        // ctor
        ಆದರೆ (modp->ctorVarReset()) {
            m_lazyDecls.emit("\nvoid " + modName + "__", protect("_ctor_var_reset"),
                             "(" + modName + "* vlSelf);\n");
        }

        ಆದರೆ (!m_ctor_ವಿಭಜಿಸು_Decl.ಖಾಲಿಗಳು()) puts("\n" + m_ctor_ವಿಭಜಿಸು_Decl + "\n");
        puts("\nvoid " + modName + "__ctor(" + modName + "* vlSelf, " + EmitCUtil::symClassName()
             + "* symsp, const char* namep) {\n");
        puts("vlSelf->vlSymsp = symsp;\n");
        ಆದರೆ (modp->is_ತುದಿ()) {
            puts("vlSelf->vlNamep = strdup(namep);\n");
        } ಇಲ್ಲದಿದ್ದರೆ {
            puts("vlSelf->vlNamep = strdup(Verilated::catName(symsp->name(), namep));\n");
        }
        putsDecoration(modp, "// Construct members\n");
        ಗಾಗಿ (ಸ್ಥಿರಾಂಕ ಪಠ್ಯ_Stmts::Stmt& stmt : m_ctorStmts) puts(stmt.m_ಪಠ್ಯ + "\n");

        putsDecoration(modp, "// Reset structure values\n");
        ಆದರೆ (modp->ctorVarReset()) {
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
        ಆದರೆ (!m_dtor_ವಿಭಜಿಸು_Decl.ಖಾಲಿಗಳು()) puts("\n" + m_dtor_ವಿಭಜಿಸು_Decl + "\n");
        puts("\nvoid " + modName + "__dtor(" + modName + "* vlSelf) {\n");
        putsDecoration(modp, "// Destroy members\n");
        ಗಾಗಿ (ಸ್ಥಿರಾಂಕ ಪಠ್ಯ_Stmts::Stmt& stmt : m_dtorStmts) puts(stmt.m_ಪಠ್ಯ + "\n");
        puts("VL_DO_DANGLING(std::free(const_cast<char*>(vlSelf->vlNamep)), "
             "vlSelf->vlNamep);\n");
        puts("}\n");

        // C++ destructor
        puts("\n" + modName + "::~" + modName + "() {\n");
        emitSystemCSection(modp, VSystemCSectionType::DTOR);
        puts(modName + "__dtor(this);\n");
        puts("}\n");
    }

    ಶೂನ್ಯ emitConfigureImp(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_Module* modp) {
        ಸ್ಥಿರಾಂಕ string modName = EmitCUtil::ಪೂರ್ವಪ್ರತ್ಯಯ_Name_Protect(modp);

        ಆದರೆ (v3Global.opt.coverage()) {
            puts("\n");
            m_lazyDecls.emit("void " + modName + "__", protect("_configure_coverage"),
                             "(" + modName + "* vlSelf, bool first);");
        }

        ಆದರೆ (v3Global.opt.coverage()) {
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
        ಆದರೆ (v3Global.opt.coverage()) {
            puts("\n// Coverage\n");
            puts("void " + EmitCUtil::ಪೂರ್ವಪ್ರತ್ಯಯ_Name_Protect(m_modp) + "::__vlCoverInsert(");
            puts(v3Global.opt.ಎಳೆಗಳು() > 1 ? "std::atomic<uint32_t>" : "uint32_t");
            puts("* countp, bool enable, bool localCounter, const char* filenamep, int lineno, "
                 "int column,\n");
            puts("const char* hierp, const char* pagep, const char* commentp, const char* "
                 "linescovp,\n");
            puts("const char* fsmVarp, const char* fsmFromp, const char* fsmTop, const char* "
                 "fsmTagp) {\n");
            ಆದರೆ (v3Global.opt.ಎಳೆಗಳು() > 1) {
                puts("assert(sizeof(uint32_t) == sizeof(std::atomic<uint32_t>));\n");
                puts("uint32_t* count32p = reinterpret_cast<uint32_t*>(countp);\n");
            } ಇಲ್ಲದಿದ್ದರೆ {
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
        ಆದರೆ (v3Global.opt.coverageToggle()) {
            puts("\n// Toggle Coverage\n");
            puts("void " + EmitCUtil::ಪೂರ್ವಪ್ರತ್ಯಯ_Name_Protect(m_modp) + "::__vlCoverToggleInsert(");
            puts("int begin, int end, bool ranged, ");
            puts(v3Global.opt.ಎಳೆಗಳು() > 1 ? "std::atomic<uint32_t>" : "uint32_t");
            puts("* countp, bool enable, bool localCounter, const char* filenamep, int lineno, "
                 "int column,\n");
            puts("const char* hierp, const char* pagep, const char* commentp) {\n");
            ಆದರೆ (v3Global.opt.ಎಳೆಗಳು() > 1) {
                puts("assert(sizeof(uint32_t) == sizeof(std::atomic<uint32_t>));\n");
            }
            puts("int step = (end >= begin) ? 1 : -1;\n");
            // range is inclusive
            puts("for (int i = begin; i != end + step; i += step) {\n");
            puts("for (int j = 0; j < 2; j++) {\n");
            ಆದರೆ (v3Global.opt.ಎಳೆಗಳು() > 1) {
                puts("uint32_t* count32p = reinterpret_cast<uint32_t*>(countp);\n");
            } ಇಲ್ಲದಿದ್ದರೆ {
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
                                       //ಖಾಲಿಗಳು
            puts("++countp;\n");
            puts("}\n");
            puts("}\n");
            puts("}\n");
        }
    }
    ಶೂನ್ಯ emitSavableImp(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_Module* modp) {
        ಆದರೆ (v3Global.opt.savable()) {
            puts("\n// Savable\n");
            ಗಾಗಿ (ಪೂರ್ಣಾಂಕ de = 0; de < 2; ++de) {
                ಸ್ಥಿರಾಂಕ string classname = de ? "VerilatedDeserialize" : "VerilatedSerialize";
                ಸ್ಥಿರಾಂಕ string funcname = de ? "__Vdeserialize" : "__Vserialize";
                ಸ್ಥಿರಾಂಕ string op = de ? ">>" : "<<";
                // NOLINTNEXTLINE(performance-inefficient-string-concatenation)
                putns(modp, "void " + EmitCUtil::ಪೂರ್ವಪ್ರತ್ಯಯ_Name_Protect(modp) + "::" + protect(funcname)
                                + "(" + classname + "& os) {\n");
                // Place a computed checksum to ensure proper structure save/restore formatting
                // OK if this hash includes some things we won't dump, since
                // just looking for loading the wrong model
                VHashSha512 hash;
                ಗಾಗಿ (Ast_ಗಂಟು* nodep = modp->stmtsp(); nodep; nodep = nodep->nextp()) {
                    ಆದರೆ (ಸ್ಥಿರಾಂಕ AstVar* ಸ್ಥಿರಾಂಕ varp = VN_CAST(nodep, Var)) {
                        hash.ಸೇರಿಸು(varp->name());
                        hash.ಸೇರಿಸು(varp->dtypep()->ಅಗಲ());
                    }
                }
                ofp()->ಬರೆ("uint64_t __Vcheckval = 0x%" PRIx64 "ULL;\n",
                              static_cast<uint64_t>(hash.digestUInt64()));
                ಆದರೆ (de) {
                    puts("os.readAssert(__Vcheckval);\n");
                } ಇಲ್ಲದಿದ್ದರೆ {
                    puts("os << __Vcheckval;\n");
                }

                // Save context
                // If multiple models save the same context we'll save it multiple
                // times, but is harmless, and doing it otherwise would break
                // backwards compatibility.
                puts("os " + op + " vlSymsp->_vm_contextp__;\n");

                // Save all members
                ಗಾಗಿ (Ast_ಗಂಟು* nodep = modp->stmtsp(); nodep; nodep = nodep->nextp()) {
                    ಆದರೆ (ಸ್ಥಿರಾಂಕ AstVar* ಸ್ಥಿರಾಂಕ varp = VN_CAST(nodep, Var)) {
                        ಆದರೆ (varp->isIO() && modp->is_ತುದಿ() && optSystemC()) {
                            // System C top I/O doesn't need loading, as the
                            // lower level subinst code does it.
                        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (varp->isParam()) {
                        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (VN_IS(varp->dtypep(), NBACommit_ಸರತಿ_DType)) {
                        } ಇಲ್ಲದಿದ್ದರೆ {
                            ಪೂರ್ಣಾಂಕ vects = 0;
                            Ast_ಗಂಟು_DType* elementp = varp->dtypeSkipRefp();
                            ಗಾಗಿ (AstUnpackArrayDType* arrayp = VN_CAST(elementp, UnpackArrayDType);
                                 arrayp; arrayp = VN_CAST(elementp, UnpackArrayDType)) {
                                ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ vecnum = vects++;
                                UASSERT_OBJ(arrayp->hi() >= arrayp->lo(), varp,
                                            "Should have swapped msb & lsb earlier.");
                                ಸ್ಥಿರಾಂಕ string ivar = "__Vi"s + cvt_ಗಮ್ಯ_Str(vecnum);
                                puts("for (int __Vi" + cvt_ಗಮ್ಯ_Str(vecnum) + " = " + cvt_ಗಮ್ಯ_Str(0));
                                puts("; " + ivar + " < " + cvt_ಗಮ್ಯ_Str(arrayp->elementsConst()));
                                puts("; ++" + ivar + ") {\n");
                                elementp = arrayp->subDTypep()->skipRefp();
                            }
                            ಸ್ಥಿರಾಂಕ AstBasicDType* ಸ್ಥಿರಾಂಕ basicp = elementp->basicp();
                            // Do not save MTask state, only matters within an evaluation
                            ಆದರೆ (basicp && basicp->keyword().isMTaskState()) ಮುಂದುವರಿ;
                            // Want to detect types that are represented as arrays
                            // (i.e. packed types of more than 64 bits).
                            ಆದರೆ (elementp->isWide()
                                && !(basicp && basicp->keyword() == VBasicDTypeKwd::STRING)) {
                                ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ vecnum = vects++;
                                ಸ್ಥಿರಾಂಕ string ivar = "__Vi"s + cvt_ಗಮ್ಯ_Str(vecnum);
                                puts("for (int __Vi" + cvt_ಗಮ್ಯ_Str(vecnum) + " = " + cvt_ಗಮ್ಯ_Str(0));
                                puts("; " + ivar + " < " + cvt_ಗಮ್ಯ_Str(elementp->ಅಗಲ_ಪದಗಳು()));
                                puts("; ++" + ivar + ") {\n");
                            }
                            putns(varp, "os" + op + varName(varp));
                            ಗಾಗಿ (ಪೂರ್ಣಾಂಕ v = 0; v < vects; ++v) puts("[__Vi" + cvt_ಗಮ್ಯ_Str(v) + "]");
                            puts(";\n");
                            ಗಾಗಿ (ಪೂರ್ಣಾಂಕ v = 0; v < vects; ++v) puts("}\n");
                        }
                    }
                }

                puts("}\n");
            }
        }
    }
    // Predicate to check if we actually need to emit anything into the common implementation file.
    // Used to avoid creating empty output files.
    ತಾರ್ಕಿಕ hasCommonImp(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_Module* modp) ಸ್ಥಿರಾಂಕ {
        // Nothing to emit if no module!
        ಆದರೆ (!modp) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
        // We always need the slow file
        ಆದರೆ (m_slow) ಹಿಂತಿರುಗಿ ನಿಜ;
        // The fast file is only required when we have `systemc_implementation nodes
        ಗಾಗಿ (ಸ್ಥಿರಾಂಕ AstSystemCSection* scSectionp = modp->scSectionsp(); scSectionp;
             scSectionp = VN_AS(scSectionp->nextp(), SystemCSection)) {
            ಆದರೆ (scSectionp->sectionType() == VSystemCSectionType::IMP) ಹಿಂತಿರುಗಿ ನಿಜ;
        }
        ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
    }
    // Actually emit common implementation contents for given AstNodeModule
    ಶೂನ್ಯ doCommonImp(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_Module* modp) {
        ಆದರೆ (m_slow) {
            ಆದರೆ (!VN_IS(modp, Class)) {
                emitParamDefns(modp);
                emitCtorDtorImp(modp);
                emitConfigureImp(modp);
                emitCoverageImp();
            }
            emitSavableImp(modp);
        } ಇಲ್ಲದಿದ್ದರೆ {
            // From `systemc_implementation
            emitSystemCSection(modp, VSystemCSectionType::IMP);
        }
    }
    ಶೂನ್ಯ emitCommonImp(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_Module* modp) {
        ಸ್ಥಿರಾಂಕ AstClass* ಸ್ಥಿರಾಂಕ classp
            = VN_IS(modp, ClassPackage) ? VN_AS(modp, ClassPackage)->classp() : ಶೂನ್ಯಸೂಚಕ;

        ಆದರೆ (hasCommonImp(modp) || hasCommonImp(classp)) {
            // Create ctor/dtor statements. If these need splitting, they will be emitted
            // during construction so do this before opening the output file.
            ಆದರೆ (m_slow && !VN_IS(modp, Class)) createCtorDtorStmts(modp);

            open_ಮುಂದಿನ_Output_File(m_file_ತಳ_Name);

            doCommonImp(modp);
            ಆದರೆ (classp) {
                VL_RESTORER(m_modp);
                m_modp = classp;
                doCommonImp(classp);
            }

            closeOutputFile();
        }
    }
    ಶೂನ್ಯ emitCFuncImp(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_Module* modp) {
        // Functions to be emitted here
        std::ಸದಿಶ<AstCFunc*> funcps;

        ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ gather = [ಇದು, &funcps](ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_Module* modp) {
            ಗಾಗಿ (Ast_ಗಂಟು* nodep = modp->stmtsp(); nodep; nodep = nodep->nextp()) {
                AstCFunc* ಸ್ಥಿರಾಂಕ funcp = VN_CAST(nodep, CFunc);
                ಆದರೆ (!funcp) ಮುಂದುವರಿ;
                // TRACE_* and DPI handled elsewhere
                ಆದರೆ (funcp->isTrace()) ಮುಂದುವರಿ;
                ಆದರೆ (funcp->dpiImportPrototype()) ಮುಂದುವರಿ;
                ಆದರೆ (funcp->dpiExportDispatcher()) ಮುಂದುವರಿ;
                ಆದರೆ (funcp->slow() != m_slow) ಮುಂದುವರಿ;
                funcps.ತಳ್ಳು_ಹಿಂಭಾಗ(funcp);
            }
        };

        gather(modp);
        VL_RESTORER(m_classOrPackage);
        ಆದರೆ (ಸ್ಥಿರಾಂಕ AstClassPackage* ಸ್ಥಿರಾಂಕ packagep = VN_CAST(modp, ClassPackage)) {
            m_classOrPackage = packagep;
            gather(packagep->classp());
        }

        // Do not create empty files
        ಆದರೆ (funcps.ಖಾಲಿಗಳು()) ಹಿಂತಿರುಗಿ;

        // Open output file
        open_ಮುಂದಿನ_Output_File(m_unique_ಹೆಸರುಗಳು.get(m_file_ತಳ_Name));
        // Emit all functions
        ಗಾಗಿ (AstCFunc* ಸ್ಥಿರಾಂಕ funcp : funcps) {
            VL_RESTORER(m_modp);
            m_modp = EmitCParentModule::get(funcp);
            iterateConst(funcp);
        }
        // Close output file
        closeOutputFile();
    }

    // VISITORS
    ಶೂನ್ಯ ಭೇಟಿ(AstCFunc* nodep) override {
        ಆದರೆ (ವಿಭಜಿಸು_Needed()) {
            // Splitting file, so using parallel build.
            v3Global.useParallelBuild(ನಿಜ);
            // Close old file
            closeOutputFile();
            // Open a new file
            open_ಮುಂದಿನ_Output_File(m_unique_ಹೆಸರುಗಳು.get(m_file_ತಳ_Name));
        }

        EmitCFunc::ಭೇಟಿ(nodep);
    }

    ಸ್ಪಷ್ಟ EmitCImp(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_Module* modp, ತಾರ್ಕಿಕ slow)
        : m_fileModp{modp}
        , m_slow{slow} {
        UINFO(5, "  Emitting implementation of " << EmitCUtil::ಪೂರ್ವಪ್ರತ್ಯಯ_Name_Protect(modp));

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
    ~EmitCImp() override = ಪೂರ್ವನಿಯೋಜಿತ;

ಸಾರ್ವಜನಿಕ:
    ಸ್ಥಿರ std::ಸದಿಶ<AstCFile*> ಮುಖ್ಯ(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_Module* modp, ತಾರ್ಕಿಕ slow) VL_MT_STABLE {
        EmitCImp emitCImp{modp, slow};
        ಹಿಂತಿರುಗಿ emitCImp.getAndClearCfileps();
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
    ಪೂರ್ಣಾಂಕ m_enum_ಅಂಕೆ = 0;  // Enumeration number (whole netlist)
    std::unordered_map<Ast_ಗಂಟು*, ಪೂರ್ಣಾಂಕ> m_enum_ಅಂಕೆ_Map;  // EnumDType to enumeration number
    ಪೂರ್ಣಾಂಕ m_traceTypeSubs = 0;  // Number of trace type declaration sub-functions
    V3_Unique_ಹೆಸರುಗಳು m_unique_ಹೆಸರುಗಳು;  // Generates unique file names
    ಸ್ಥಿರಾಂಕ std::string m_file_ತಳ_Name = EmitCUtil::ತುದಿ_Class_Name() + "_" + protect("_TraceDecls");
    // This one uses CSplitTrace for file splitting, which is incorrect but historically accurates
    ಸ್ಥಿರಾಂಕ ಗಾತ್ರ_t m_ವಿಭಜಿಸು_ಎಲ್ಲೆ = v3Global.opt.output_ವಿಭಜಿಸು_CTrace()
                                    ? static_cast<ಗಾತ್ರ_t>(v3Global.opt.output_ವಿಭಜಿಸು_CTrace())
                                    : std::numeric_limits<ಗಾತ್ರ_t>::max();

    ಶೂನ್ಯ open_ಮುಂದಿನ_Output_File() {
        open_New_Output_ಮೂಲ_File(m_unique_ಹೆಸರುಗಳು.get(m_file_ತಳ_Name), ನಿಜ, ನಿಜ,
                                "Tracing declarations");
        puts("\n");
        ಗಾಗಿ (ಸ್ಥಿರಾಂಕ std::string& ತಳ : v3Global.opt.trace_ಮೂಲ_Langs()) {
            puts("#include \"" + ತಳ + ".h\"\n");
        }
        puts("\n");
        puts("\nvoid " + EmitCUtil::ಪೂರ್ವಪ್ರತ್ಯಯ_Name_Protect(m_modp) + "__"
             + protect("traceDeclTypesSub" + std::ಗಮ್ಯ_string(m_traceTypeSubs++)) + "("
             + v3Global.opt.trace_Class_ತಳ() + "* tracep) {\n");
    }

ಸಾರ್ವಜನಿಕ:
    // METHODS
    ಪೂರ್ಣಾಂಕ get_Enum_Map_ಅಂಕೆ(AstEnumDType* nodep) {
        ಪೂರ್ಣಾಂಕ& enumNumr = m_enum_ಅಂಕೆ_Map[nodep];
        ಆದರೆ (!enumNumr) {
            ಆದರೆ (ವಿಭಜಿಸು_Needed(m_ವಿಭಜಿಸು_ಎಲ್ಲೆ)) {
                // Splitting file, so using parallel build.
                v3Global.useParallelBuild(ನಿಜ);
                puts("}\n");
                closeOutputFile();
                open_ಮುಂದಿನ_Output_File();
            }

            enumNumr = ++m_enum_ಅಂಕೆ;
            ಪೂರ್ಣಾಂಕ nvals = 0;
            puts("{\n");
            putns(nodep, "const char* " + protect("__VenumItemNames") + "[]\n");
            puts("= {");
            ಗಾಗಿ (Ast_Enum_ಅಂಶ* itemp = nodep->itemsp(); itemp;
                 itemp = VN_AS(itemp->nextp(), Enum_ಅಂಶ)) {
                ಆದರೆ (++nvals > 1) puts(", ");
                putbs("\"" + itemp->prettyName() + "\"");
            }
            puts("};\n");
            nvals = 0;
            puts("const char* " + protect("__VenumItemValues") + "[]\n");
            puts("= {");
            ಗಾಗಿ (Ast_Enum_ಅಂಶ* itemp = nodep->itemsp(); itemp;
                 itemp = VN_AS(itemp->nextp(), Enum_ಅಂಶ)) {
                AstConst* ಸ್ಥಿರಾಂಕ constp = VN_AS(itemp->valuep(), Const);
                ಆದರೆ (++nvals > 1) puts(", ");
                putbs("\"" + constp->ಅಂಕೆ().displayed(nodep, "%0b") + "\"");
            }
            puts("};\n");
            puts("tracep->declDTypeEnum(" + std::ಗಮ್ಯ_string(enumNumr) + ", \"" + nodep->prettyName()
                 + "\", " + std::ಗಮ್ಯ_string(nvals) + ", " + std::ಗಮ್ಯ_string(nodep->ಅಗಲ_Min()) + ", "
                 + protect("__VenumItemNames") + ", " + protect("__VenumItemValues") + ");\n");
            puts("}\n");
            ವಿಭಜಿಸು_ಗಾತ್ರ_Inc(Ast_ಗಂಟು::INSTR_ಎಣಿಕೆ_CALL);
        }
        ಹಿಂತಿರುಗಿ enumNumr;
    }

    // Close output file
    ಶೂನ್ಯ finalize() {
        // Close function definition
        puts("}\n");

        ಸ್ಥಿರಾಂಕ std::string modName = EmitCUtil::ಪೂರ್ವಪ್ರತ್ಯಯ_Name_Protect(m_modp);
        ಸ್ಥಿರಾಂಕ std::string ವಾದಗಳು = v3Global.opt.trace_Class_ತಳ() + "* tracep";

        // Forward declarations for subs in other files
        ಗಾಗಿ (ಪೂರ್ಣಾಂಕ i = 0; i < m_traceTypeSubs - 1; ++i) {
            puts("void " + modName + "__" + protect("traceDeclTypesSub" + std::ಗಮ್ಯ_string(i)) + "("
                 + ವಾದಗಳು + ");\n");
        }

        // Create top level trace_decl_types function and call each sub-function
        puts("\nvoid " + modName + "__" + protect("trace_decl_types") + "(" + ವಾದಗಳು + ") {\n");
        ಗಾಗಿ (ಪೂರ್ಣಾಂಕ i = 0; i < m_traceTypeSubs; ++i) {
            puts(modName + "__" + protect("traceDeclTypesSub" + std::ಗಮ್ಯ_string(i))
                 + "(tracep);\n");
        }
        puts("}\n");

        closeOutputFile();
    }

    EmitCTraceTypes() {
        m_modp = v3Global.rootp()->ತುದಿ_Modulep();
        open_ಮುಂದಿನ_Output_File();
    }
    ~EmitCTraceTypes() override = ಪೂರ್ವನಿಯೋಜಿತ;
};

ವರ್ಗ EmitCTrace final : ಸಾರ್ವಜನಿಕ EmitCFunc {
    // NODE STATE/TYPES
    // None allowed to support threaded emitting

    // MEMBERS
    ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ m_slow;  // Making slow file
    ಸ್ಥಿರಾಂಕ std::unique_ptr<EmitCTraceTypes> m_emitTypesp{m_slow ? ಹೊಸ EmitCTraceTypes{} : ಶೂನ್ಯಸೂಚಕ};
    V3_Unique_ಹೆಸರುಗಳು m_unique_ಹೆಸರುಗಳು;  // Generates unique file names
    ಸ್ಥಿರಾಂಕ std::string m_file_ತಳ_Name = EmitCUtil::ತುದಿ_Class_Name() + "_" + protect("_Trace");

    // METHODS
    ಶೂನ್ಯ open_ಮುಂದಿನ_Output_File() {
        open_New_Output_ಮೂಲ_File(m_unique_ಹೆಸರುಗಳು.get(m_file_ತಳ_Name), m_slow, ನಿಜ,
                                "Tracing implementation internals");
        puts("\n");
        ಗಾಗಿ (ಸ್ಥಿರಾಂಕ std::string& ತಳ : v3Global.opt.trace_ಮೂಲ_Langs()) {
            puts("#include \"" + ತಳ + ".h\"\n");
        }
        puts("#include \"" + EmitCUtil::symClassName() + ".h\"\n");
        puts("\n");
        // Need to emit new lazy declarations
        m_lazyDecls.reset();
    }

    ತಾರ್ಕಿಕ emitTraceIsScBv(ಸ್ಥಿರಾಂಕ AstTraceInc* nodep) {
        ಸ್ಥಿರಾಂಕ AstVarRef* ಸ್ಥಿರಾಂಕ varrefp = VN_CAST(nodep->declp()->valuep(), VarRef);
        ಆದರೆ (!varrefp) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
        ಸ್ಥಿರಾಂಕ AstVar* ಸ್ಥಿರಾಂಕ varp = varrefp->varp();
        ಹಿಂತಿರುಗಿ varp->isSc() && varp->isScBv();
    }

    ತಾರ್ಕಿಕ emitTraceIsScBigUint(ಸ್ಥಿರಾಂಕ AstTraceInc* nodep) {
        ಸ್ಥಿರಾಂಕ AstVarRef* ಸ್ಥಿರಾಂಕ varrefp = VN_CAST(nodep->declp()->valuep(), VarRef);
        ಆದರೆ (!varrefp) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
        ಸ್ಥಿರಾಂಕ AstVar* ಸ್ಥಿರಾಂಕ varp = varrefp->varp();
        ಹಿಂತಿರುಗಿ varp->isSc() && varp->isScBigUint();
    }

    ತಾರ್ಕಿಕ emitTraceIsScUint(ಸ್ಥಿರಾಂಕ AstTraceInc* nodep) {
        ಸ್ಥಿರಾಂಕ AstVarRef* ಸ್ಥಿರಾಂಕ varrefp = VN_CAST(nodep->declp()->valuep(), VarRef);
        ಆದರೆ (!varrefp) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
        ಸ್ಥಿರಾಂಕ AstVar* ಸ್ಥಿರಾಂಕ varp = varrefp->varp();
        ಹಿಂತಿರುಗಿ varp->isSc() && (varp->isScUint() || varp->isScUintBool());
    }

    ಶೂನ್ಯ emitTraceInitOne(ಸ್ಥಿರಾಂಕ AstTraceDecl* nodep, ಪೂರ್ಣಾಂಕ enum_ಅಂಕೆ) {
        std::string direction;
        direction = nodep->declDirection().traceSigDirection();

        AstCCall* ಸ್ಥಿರಾಂಕ callp = nodep->dtypeCallp();
        ಆದರೆ (callp) {
            callp->argTypes(callp->argTypes() + ", " + cvt_ಗಮ್ಯ_Str(nodep->fidx()) + ", c+"
                            + cvt_ಗಮ್ಯ_Str(nodep->ಸಂಕೇತ_2()) + ", " + direction);
            ಹಿಂತಿರುಗಿ;
        }

        ಆದರೆ (nodep->dtypep()->basicp()->isDouble()) {
            puts("VL_TRACE_DECL_DOUBLE");
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (nodep->isWide()) {
            puts("VL_TRACE_DECL_WIDE");
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (nodep->isQuad()) {
            puts("VL_TRACE_DECL_QUAD");
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (nodep->ಬಿಟ್_Range().ranged()) {
            puts("VL_TRACE_DECL_BUS");
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (nodep->dtypep()->basicp()->isEvent()) {
            puts("VL_TRACE_DECL_EVENT");
        } ಇಲ್ಲದಿದ್ದರೆ {
            puts("VL_TRACE_DECL_BIT");
        }

        ಆದರೆ (nodep->arrayRange().ranged()) {
            puts("_ARRAY(tracep");
        } ಇಲ್ಲದಿದ್ದರೆ {
            puts("(tracep");
        }

        //ಸಂಕೇತ
        puts(",c+" + cvt_ಗಮ್ಯ_Str(nodep->ಸಂಕೇತ_2()));
        ಆದರೆ (nodep->arrayRange().ranged()) puts("+i*" + cvt_ಗಮ್ಯ_Str(nodep->ಅಗಲ_ಪದಗಳು()));

        // Function index
        puts(",");
        ಆದರೆ (nodep->ಒಳ_Dtype_Func()) {
            puts("fidx");
        } ಇಲ್ಲದಿದ್ದರೆ {
            puts(cvt_ಗಮ್ಯ_Str(nodep->fidx()));
        }

        // Name
        puts(",");
        putsQuoted(VIdProtect::protect_ಪದಗಳು_If(nodep->showname(), nodep->protect()));

        // Enum number
        puts("," + cvt_ಗಮ್ಯ_Str(enum_ಅಂಕೆ));

        // Direction
        ಆದರೆ (nodep->ಒಳ_Dtype_Func()) {
            puts(", direction");
        } ಇಲ್ಲದಿದ್ದರೆ {
            puts(", " + direction);
        }

        //ಬಗೆ
        puts(", VerilatedTraceSigKind::");
        puts(nodep->varType().trace_Sig_ಬಗೆ());

        // Type
        puts(", VerilatedTraceSigType::");
        UASSERT_OBJ(nodep->dtypep()->basicp()->keyword() != VBasicDTypeKwd::UNKNOWN, nodep,
                    "DType is unknown");
        puts(nodep->dtypep()->basicp()->keyword().traceSigType());

        // Array range
        ಆದರೆ (nodep->arrayRange().ranged()) {
            ಆದರೆ (nodep->arrayRange().ascending()) {
                puts(", (i + " + std::ಗಮ್ಯ_string(nodep->arrayRange().lo()) + ")");
            } ಇಲ್ಲದಿದ್ದರೆ {
                puts(", (" + std::ಗಮ್ಯ_string(nodep->arrayRange().hi()) + " - i)");
            }
        }

        // Bit range
        ಆದರೆ (!nodep->dtypep()->basicp()->isDouble() && nodep->ಬಿಟ್_Range().ranged()) {
            puts(", " + cvt_ಗಮ್ಯ_Str(nodep->ಬಿಟ್_Range().ಎಡ()) + ","
                 + cvt_ಗಮ್ಯ_Str(nodep->ಬಿಟ್_Range().ಬಲ()));
        }

        //
        puts(");");
    }

    ಪೂರ್ಣಾಂಕ emitTraceDeclDType(Ast_ಗಂಟು_DType* nodep) {
        // Return enum number or -1 for none
        ಆದರೆ (v3Global.opt.traceEnabledFst()) {
            // Skip over refs-to-refs, but stop before final ref so can get data type name
            // Alternatively back in V3Width we could push enum names from upper typedefs
            ಆದರೆ (AstEnumDType* ಸ್ಥಿರಾಂಕ enump = VN_CAST(nodep->skip_Ref_ಗಮ್ಯ_Enump(), EnumDType)) {
                ಹಿಂತಿರುಗಿ m_emitTypesp->get_Enum_Map_ಅಂಕೆ(enump);
            }
        }
        ಹಿಂತಿರುಗಿ -1;
    }

    ಶೂನ್ಯ emitTraceChangeOne(AstTraceInc* nodep, ಪೂರ್ಣಾಂಕ arrayindex) {
        // Note: Both VTraceType::CHANGE and VTraceType::FULL use the 'full' methods
        ಸ್ಥಿರಾಂಕ std::string func = nodep->traceType() == VTraceType::CHANGE ? "chg" : "full";
        ತಾರ್ಕಿಕ emit_ಅಗಲ = ನಿಜ;
        string stype;
        ಆದರೆ (nodep->dtypep()->basicp()->isDouble()) {
            stype = "Double";
            emit_ಅಗಲ = ಸುಳ್ಳು;
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (nodep->isWide() || emitTraceIsScBv(nodep) || emitTraceIsScBigUint(nodep)) {
            stype = "WData";
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (nodep->isQuad()) {
            stype = "QData";
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (nodep->declp()->ಅಗಲ_Min() > 16) {
            stype = "IData";
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (nodep->declp()->ಅಗಲ_Min() > 8) {
            stype = "SData";
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (nodep->declp()->ಅಗಲ_Min() > 1) {
            stype = "CData";
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (nodep->dtypep()->basicp()->isEvent()) {
            stype = "Event";
            emit_ಅಗಲ = ಸುಳ್ಳು;
        } ಇಲ್ಲದಿದ್ದರೆ {
            stype = "Bit";
            emit_ಅಗಲ = ಸುಳ್ಳು;
        }
        putns(nodep, "bufp->" + func + stype);

        ಸ್ಥಿರಾಂಕ uint32_t ಸ್ಥಾನಾಂತರ = (arrayindex < 0) ? 0 : (arrayindex * nodep->declp()->ಅಗಲ_ಪದಗಳು());
        ಸ್ಥಿರಾಂಕ uint32_t ಸಂಕೇತ_2 = nodep->declp()->ಸಂಕೇತ_2() + ಸ್ಥಾನಾಂತರ;
        // Note: Both VTraceType::CHANGE and VTraceType::FULL use the 'full' methods
        puts("(oldp+");
        puts(cvt_ಗಮ್ಯ_Str(ಸಂಕೇತ_2 - nodep->ತಳ_ಸಂಕೇತ()));
        puts(",");
        ಸ್ಥಿರಾಂಕ VNumRange& arrayRange = nodep->declp()->arrayRange();
        ಆದರೆ (arrayRange.ranged() && !arrayRange.ascending()) {
            arrayindex = arrayRange.elements() - 1 - arrayindex;
        }
        emit_Trace_ಮೌಲ್ಯ(nodep, arrayindex);
        ಆದರೆ (emit_ಅಗಲ) puts("," + cvt_ಗಮ್ಯ_Str(nodep->declp()->ಅಗಲ_Min()));
        puts(");\n");
    }

    ಶೂನ್ಯ emit_Trace_ಮೌಲ್ಯ(ಸ್ಥಿರಾಂಕ AstTraceInc* nodep, ಪೂರ್ಣಾಂಕ arrayindex) {
        ಆದರೆ (AstVarRef* ಸ್ಥಿರಾಂಕ varrefp = VN_CAST(nodep->valuep(), VarRef)) {
            ಸ್ಥಿರಾಂಕ AstVar* ಸ್ಥಿರಾಂಕ varp = varrefp->varp();
            ಆದರೆ (varp->isEvent()) puts("&");
            puts("(");
            ಆದರೆ (emitTraceIsScBigUint(nodep)) {
                puts("(uint32_t*)");
            } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (emitTraceIsScBv(nodep)) {
                puts("VL_SC_BV_DATAP(");
            }
            iterateConst(varrefp);  // Put var name out
            emit_Trace_ಸೂಚ್ಯಂಕ(nodep, arrayindex);
            ಆದರೆ (varp->isSc()) puts(".read()");
            ಆದರೆ (emitTraceIsScUint(nodep)) {
                puts(nodep->isQuad() ? ".to_uint64()" : ".to_uint()");
            } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (emitTraceIsScBigUint(nodep)) {
                puts(".get_raw()");
            } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (emitTraceIsScBv(nodep)) {
                puts(")");
            }
            puts(")");
        } ಇಲ್ಲದಿದ್ದರೆ {
            puts("(");
            iterateConst(nodep->valuep());
            emit_Trace_ಸೂಚ್ಯಂಕ(nodep, arrayindex);
            puts(")");
        }
    }

    ಶೂನ್ಯ emit_Trace_ಸೂಚ್ಯಂಕ(ಸ್ಥಿರಾಂಕ AstTraceInc* ಸ್ಥಿರಾಂಕ nodep, ಪೂರ್ಣಾಂಕ arrayindex) {
        // Tracing only supports 1D arrays
        ಆದರೆ (nodep->declp()->arrayRange().ranged()) {
            ಆದರೆ (arrayindex == -2) {
                puts("[i]");
            } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (arrayindex == -1) {
                puts("[0]");
            } ಇಲ್ಲದಿದ್ದರೆ {
                puts("[" + cvt_ಗಮ್ಯ_Str(arrayindex) + "]");
            }
        }
    }

    // VISITORS
    ಬಳಸಿ EmitCFunc::ಭೇಟಿ;  // Suppress hidden overloaded virtual function warning
    ಶೂನ್ಯ ಭೇಟಿ(AstCFunc* nodep) override {
        ಆದರೆ (!nodep->isTrace()) ಹಿಂತಿರುಗಿ;
        ಆದರೆ (nodep->slow() != m_slow) ಹಿಂತಿರುಗಿ;

        ಆದರೆ (ವಿಭಜಿಸು_Needed()) {
            // Splitting file, so using parallel build.
            v3Global.useParallelBuild(ನಿಜ);
            // Close old file
            closeOutputFile();
            // Open a new file
            open_ಮುಂದಿನ_Output_File();
        }

        EmitCFunc::ಭೇಟಿ(nodep);
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_Trace_ತಳ್ಳು_ಪೂರ್ವಪ್ರತ್ಯಯ* nodep) override {
        putns(nodep, "VL_TRACE_PUSH_PREFIX(tracep, ");
        ಆದರೆ (nodep->quoted_ಪೂರ್ವಪ್ರತ್ಯಯ()) {
            putsQuoted(VIdProtect::protect_ಪದಗಳು_If(nodep->ಪೂರ್ವಪ್ರತ್ಯಯ(), nodep->protect()));
        } ಇಲ್ಲದಿದ್ದರೆ {
            puts(nodep->ಪೂರ್ವಪ್ರತ್ಯಯ());
        }
        puts(", VerilatedTracePrefixType::");
        puts(nodep->ಪೂರ್ವಪ್ರತ್ಯಯ_Type().ascii());
        puts(", " + std::ಗಮ್ಯ_string(nodep->ಎಡ()));
        puts(", " + std::ಗಮ್ಯ_string(nodep->ಬಲ()));
        puts(");\n");
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_Trace_ತೆಗೆ_ಪೂರ್ವಪ್ರತ್ಯಯ* nodep) override {  //
        putns(nodep, "VL_TRACE_POP_PREFIX(tracep);\n");
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstTraceDecl* nodep) override {
        ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ enum_ಅಂಕೆ = emitTraceDeclDType(nodep->dtypep());
        putns(nodep, "");
        ಆದರೆ (nodep->arrayRange().ranged()) {
            puts("for (int i = 0; i < " + cvt_ಗಮ್ಯ_Str(nodep->arrayRange().elements()) + "; ++i) {\n");
            emitTraceInitOne(nodep, enum_ಅಂಕೆ);
            puts("\n}\n");
        } ಇಲ್ಲದಿದ್ದರೆ {
            emitTraceInitOne(nodep, enum_ಅಂಕೆ);
            puts("\n");
        }
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstTraceInc* nodep) override {
        ಆದರೆ (nodep->declp()->arrayRange().ranged()) {
            // It traces faster if we unroll the loop
            ಗಾಗಿ (ಪೂರ್ಣಾಂಕ i = 0; i < nodep->declp()->arrayRange().elements(); i++) {
                emitTraceChangeOne(nodep, i);
            }
        } ಇಲ್ಲದಿದ್ದರೆ {
            emitTraceChangeOne(nodep, -1);
        }
    }

    ಸ್ಪಷ್ಟ EmitCTrace(ತಾರ್ಕಿಕ slow)
        : m_slow{slow} {
        m_modp = v3Global.rootp()->ತುದಿ_Modulep();
        // Open output file
        open_ಮುಂದಿನ_Output_File();
        // Emit functions
        ಗಾಗಿ (Ast_ಗಂಟು* nodep = m_modp->stmtsp(); nodep; nodep = nodep->nextp()) {
            ಆದರೆ (AstCFunc* ಸ್ಥಿರಾಂಕ funcp = VN_CAST(nodep, CFunc)) iterateConst(funcp);
        }
        // Close output file
        closeOutputFile();
        ಆದರೆ (m_slow) m_emitTypesp->finalize();
    }
    ~EmitCTrace() override = ಪೂರ್ವನಿಯೋಜಿತ;

ಸಾರ್ವಜನಿಕ:
    ಸ್ಥಿರ std::ಸದಿಶ<AstCFile*> ಮುಖ್ಯ(ತಾರ್ಕಿಕ slow) VL_MT_STABLE {
        EmitCTrace emitCTrace{slow};
        std::ಸದಿಶ<AstCFile*> cfileps = emitCTrace.getAndClearCfileps();
        ಆದರೆ (slow) {
            ಗಾಗಿ (AstCFile* ಸ್ಥಿರಾಂಕ cfilep : emitCTrace.m_emitTypesp->getAndClearCfileps()) {
                cfileps.emplace_ಹಿಂಭಾಗ(cfilep);
            }
        }
        ಹಿಂತಿರುಗಿ cfileps;
    }
};

//######################################################################
// Existing AstCFile emitter

ವರ್ಗ EmitCFile final : ಸಾರ್ವಜನಿಕ EmitCFunc {
    ಸ್ಪಷ್ಟ EmitCFile(AstCFile* cfilep) {
        openOutputFile(cfilep, "Generated C++");
        iterateConst(cfilep->tblockp());
        closeOutputFile();
    }
    ~EmitCFile() override = ಪೂರ್ವನಿಯೋಜಿತ;

ಸಾರ್ವಜನಿಕ:
    ಸ್ಥಿರ ಶೂನ್ಯ ಮುಖ್ಯ(AstCFile* cfilep) VL_MT_STABLE {
        ಆದರೆ (!cfilep->tblockp()) ಹಿಂತಿರುಗಿ;
        EmitCFile{cfilep};
    }
};

//######################################################################
// EmitC class functions

ಶೂನ್ಯ V3EmitC::emitcImp() {
    UINFO(2, __FUNCTION__ << ":");
    std::list<std::ಸದಿಶ<AstCFile*>> cfiles;
    {
        // Make parent module pointers available.
        ಸ್ಥಿರಾಂಕ EmitCParentModule emitCParentModule;
        V3ThreadScope threadScope;

        // Process each module in turn
        ಗಾಗಿ (ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* nodep = v3Global.rootp()->modulesp(); nodep; nodep = nodep->nextp()) {
            ಆದರೆ (VN_IS(nodep, Class)) ಮುಂದುವರಿ;  // Imped with ClassPackage
            ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_Module* ಸ್ಥಿರಾಂಕ modp = VN_AS(nodep, ಗಂಟು_Module);
            ಆದರೆ (modp->isConstPool()) ಮುಂದುವರಿ;  // Emitted by V3EmitCConstPool
            cfiles.emplace_ಹಿಂಭಾಗ();
            std::ಸದಿಶ<AstCFile*>& slow = cfiles.ಹಿಂಭಾಗ();
            threadScope.ಸರತಿಗೆ_ಸೇರಿಸು([modp, &slow] { slow = EmitCImp::ಮುಖ್ಯ(modp, /* slow: */ ನಿಜ); });
            cfiles.emplace_ಹಿಂಭಾಗ();
            std::ಸದಿಶ<AstCFile*>& fast = cfiles.ಹಿಂಭಾಗ();
            threadScope.ಸರತಿಗೆ_ಸೇರಿಸು([modp, &fast] { fast = EmitCImp::ಮುಖ್ಯ(modp, /* slow: */ ಸುಳ್ಳು); });
        }

        // Emit trace routines (currently they can only exist in the top module)
        ಆದರೆ (v3Global.opt.trace() && !v3Global.opt.lintOnly()) {
            cfiles.emplace_ಹಿಂಭಾಗ();
            std::ಸದಿಶ<AstCFile*>& slow = cfiles.ಹಿಂಭಾಗ();
            threadScope.ಸರತಿಗೆ_ಸೇರಿಸು([&slow] { slow = EmitCTrace::ಮುಖ್ಯ(/* slow: */ ನಿಜ); });
            cfiles.emplace_ಹಿಂಭಾಗ();
            std::ಸದಿಶ<AstCFile*>& fast = cfiles.ಹಿಂಭಾಗ();
            threadScope.ಸರತಿಗೆ_ಸೇರಿಸು([&fast] { fast = EmitCTrace::ಮುಖ್ಯ(/* slow: */ ಸುಳ್ಳು); });
        }
    }
    // Add files to netlist
    ಗಾಗಿ (ಸ್ಥಿರಾಂಕ std::ಸದಿಶ<AstCFile*>& cfileps : cfiles) {
        ಗಾಗಿ (AstCFile* ಸ್ಥಿರಾಂಕ cfilep : cfileps) v3Global.rootp()->ಕೂಡಿಸು_Filesp(cfilep);
    }
}

ಶೂನ್ಯ V3EmitC::emitcFiles() {
    UINFO(2, __FUNCTION__ << ":");
    ಗಾಗಿ (Ast_ಗಂಟು_File *filep = v3Global.rootp()->filesp(), *nextp; filep; filep = nextp) {
        nextp = VN_AS(filep->nextp(), ಗಂಟು_File);
        ಆದರೆ (AstCFile* ಸ್ಥಿರಾಂಕ cfilep = VN_CAST(filep, CFile)) EmitCFile::ಮುಖ್ಯ(cfilep);
    }
}
