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

#అన్తర్భావ "V3PchAstMT.h"

#అన్తర్భావ "V3EmitC.h"
#అన్తర్భావ "V3EmitCFunc.h"
#అన్తర్భావ "V3ThreadPool.h"
#అన్తర్భావ "V3UniqueNames.h"

#అన్తర్భావ <algorithm>
#అన్తర్భావ <map>
#అన్తర్భావ <set>
#అన్తర్భావ <tuple>
#అన్తర్భావ <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Internal EmitC implementation

తరగతి EmitCImp final : బహిరంగ EmitCFunc {
    // MEMBERS
    // Base module (For non-classes, same as m_modp. For classes, it's the ClassPackage.)
    స్థిరరాశి Ast_కణుపు_Module* స్థిరరాశి m_fileModp;
    స్థిరరాశి తార్కిక m_slow;  // Creating __Slow file
    V3_Unique_పేర్లు m_unique_పేర్లు;  // Generates unique file names
    స్థిరరాశి std::string m_file_పునాది_Name = EmitCUtil::ఉపసర్గ_Name_Protect(m_fileModp);
    పాఠ్యం_Stmts m_ctorStmts;  // Statements constructing members in 'ctor'
    పాఠ్యం_Stmts m_dtorStmts;  // Statements destroying members in 'dtor'
    std::string m_ctor_విభజించు_Decl;  // Declaration of sub-function 'ctor' was split into, if any
    std::string m_dtor_విభజించు_Decl;  // Declaration of sub-function 'dtor' was split into, if any

    // METHODS
    శూన్య open_తదుపరి_Output_File(స్థిరరాశి std::string& fileName) {
        open_New_Output_ఆధారం_File(fileName, m_slow, అబద్ధం, "Design implementation internals");
        emit_File_శీర్షిక();
    }
    శూన్య emit_File_శీర్షిక() {
        puts("// See " + EmitCUtil::శిఖరం_Class_Name() + ".h for the primary calling header\n");
        puts("\n");
        puts("#include \"" + EmitCUtil::pchClassName() + ".h\"\n");
        emitSystemCSection(m_fileModp, VSystemCSectionType::IMP_HDR);
        // Need to emit new lazy declarations
        m_lazyDecls.reset();
    }

    శూన్య emitParamDefns(స్థిరరాశి Ast_కణుపు_Module* modp) {
        తార్కిక మొదటి = నిజం;
        కొరకు (స్థిరరాశి Ast_కణుపు* nodep = modp->stmtsp(); nodep; nodep = nodep->nextp()) {
            ఒకవేళ (స్థిరరాశి AstVar* స్థిరరాశి varp = VN_CAST(nodep, Var)) {
                ఒకవేళ (varp->isParam()) {
                    ఒకవేళ (మొదటి) {
                        puts("\n");
                        స్థిరరాశి string modName = EmitCUtil::ఉపసర్గ_Name_Protect(modp);
                        putsDecoration(modp, "// Parameter definitions for " + modName + "\n");
                        మొదటి = అబద్ధం;
                    }
                    ఒకవేళ (varp->dtypep()->isLiteralType()) putns(varp, "VL_CONSTINIT_CXX20 ");
                    putns(varp, "const ");
                    emitVarDefn(varp);
                    puts(";\n");
                }
            }
        }
        ఒకవేళ (!మొదటి) puts("\n");
    }
    // Add statements explicitly constructing, or destroying, a member AstVar
    స్థిర శూన్య కూడు_Var_Ctor_Dtor_Stmts(పాఠ్యం_Stmts& stmts, స్థిరరాశి AstVar* varp, తార్కిక destroy) {
        స్థిరరాశి std::string name = "vlSelf->" + varName(varp);

        // SystemC port arrays are C arrays, so construct them element by element. They are
        // default constructed (unnamed), as the elements of an array member would be.
        ఒకవేళ (varp->isSc() && VN_IS(varp->dtypeSkipRefp(), UnpackArrayDType)) {
            std::సదిశ<std::string> elems{name};
            కొరకు (స్థిరరాశి AstUnpackArrayDType* adtypep
                 = VN_CAST(varp->dtypeSkipRefp(), UnpackArrayDType);
                 adtypep; adtypep = VN_CAST(adtypep->subDTypep()->skipRefp(), UnpackArrayDType)) {
                std::సదిశ<std::string> subElems;
                కొరకు (స్థిరరాశి std::string& elem : elems) {
                    కొరకు (పూర్ణాంకం i = 0; i < adtypep->elementsConst(); ++i) {
                        subElems.నెట్టు_వెనుక(elem + "[" + std::గమ్యం_string(i) + "]");
                    }
                }
                elems = std::move(subElems);
            }
            ఒకవేళ (destroy) std::తిరగేయి(elems.begin(), elems.చివర());
            కొరకు (స్థిరరాశి std::string& e : elems) {
                stmts.కూడు((destroy ? "VL_INPLACE_DESTRUCT(" : "VL_INPLACE_CONSTRUCT(") + e + ");");
            }
            తిరిగి;
        }

        // Other variables are constructed whole, even if aggregates
        ఒకవేళ (destroy) {
            stmts.కూడు("VL_INPLACE_DESTRUCT(" + name + ");");
            తిరిగి;
        }
        స్థిరరాశి AstBasicDType* స్థిరరాశి basicp = VN_CAST(varp->dtypeSkipRefp(), BasicDType);
        std::string వాదనలు;
        ఒకవేళ (varp->isSc()) {
            // SystemC ports need their name
            వాదనలు = ", \"" + V3_బహిర్గతం_Formatter::quoteNameControls(varp->nameProtect()) + "\"";
        } లేకపోతే ఒకవేళ (basicp && basicp->keyword().isMTaskState()) {
            // MTask state needs the number of upstream dependencies to wait for
            వాదనలు = ", " + VN_AS(varp->valuep(), Const)->అంకె().emitC();
        } లేకపోతే ఒకవేళ (basicp && basicp->isDelayScheduler()) {
            // Delay scheduler needs the context pointer
            వాదనలు = ", *vlSelf->vlSymsp->_vm_contextp__";
        }
        stmts.కూడు("VL_INPLACE_CONSTRUCT(" + name + వాదనలు + ");");
    }
    // Gather the statements constructing/destroying members in anonymous unions, and split
    // them into sub-functions if needed. As this emits files, no output file can be open.
    శూన్య createCtorDtorStmts(స్థిరరాశి Ast_కణుపు_Module* modp) {
        // Construct member variables
        కొరకు (స్థిరరాశి Ast_కణుపు* nodep = modp->stmtsp(); nodep; nodep = nodep->nextp()) {
            స్థిరరాశి AstVar* స్థిరరాశి varp = VN_CAST(nodep, Var);
            ఒకవేళ (varp && isExplicitlyConstructed(varp)) {
                కూడు_Var_Ctor_Dtor_Stmts(m_ctorStmts, varp, అబద్ధం);
            }
        }

        // Destroy in reverse order
        స్థిరరాశి Ast_కణుపు* స్థిరరాశి lastp = modp->stmtsp() ? modp->stmtsp()->lastp() : శూన్యసూచిక;
        కొరకు (స్థిరరాశి Ast_కణుపు* nodep = lastp; nodep; nodep = nodep->prevp()) {
            స్థిరరాశి AstVar* స్థిరరాశి varp = VN_CAST(nodep, Var);
            ఒకవేళ (varp && isExplicitlyConstructed(varp)) {
                కూడు_Var_Ctor_Dtor_Stmts(m_dtorStmts, varp, నిజం);
            }
        }

        // Split into sub-functions if needed
        స్థిరరాశి std::string modName = EmitCUtil::ఉపసర్గ_Name_Protect(modp);
        std::tie(m_ctor_విభజించు_Decl, m_dtor_విభజించు_Decl) = emit_విభజించు_Ctor_Dtor(
            m_ctorStmts, m_dtorStmts, 0, modName, modName + "* vlSelf", "vlSelf", అబద్ధం,
            "Design implementation internals", [ఇది]() { emit_File_శీర్షిక(); });
    }
    శూన్య emitCtorDtorImp(స్థిరరాశి Ast_కణుపు_Module* modp) {
        స్థిరరాశి std::string modName = EmitCUtil::ఉపసర్గ_Name_Protect(modp);

        // ctor
        ఒకవేళ (modp->ctorVarReset()) {
            m_lazyDecls.emit("\nvoid " + modName + "__", protect("_ctor_var_reset"),
                             "(" + modName + "* vlSelf);\n");
        }

        ఒకవేళ (!m_ctor_విభజించు_Decl.ఖాళీలు()) puts("\n" + m_ctor_విభజించు_Decl + "\n");
        puts("\nvoid " + modName + "__ctor(" + modName + "* vlSelf, " + EmitCUtil::symClassName()
             + "* symsp, const char* namep) {\n");
        puts("vlSelf->vlSymsp = symsp;\n");
        ఒకవేళ (modp->is_శిఖరం()) {
            puts("vlSelf->vlNamep = strdup(namep);\n");
        } లేకపోతే {
            puts("vlSelf->vlNamep = strdup(Verilated::catName(symsp->name(), namep));\n");
        }
        putsDecoration(modp, "// Construct members\n");
        కొరకు (స్థిరరాశి పాఠ్యం_Stmts::Stmt& stmt : m_ctorStmts) puts(stmt.m_పాఠ్యం + "\n");

        putsDecoration(modp, "// Reset structure values\n");
        ఒకవేళ (modp->ctorVarReset()) {
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
        ఒకవేళ (!m_dtor_విభజించు_Decl.ఖాళీలు()) puts("\n" + m_dtor_విభజించు_Decl + "\n");
        puts("\nvoid " + modName + "__dtor(" + modName + "* vlSelf) {\n");
        putsDecoration(modp, "// Destroy members\n");
        కొరకు (స్థిరరాశి పాఠ్యం_Stmts::Stmt& stmt : m_dtorStmts) puts(stmt.m_పాఠ్యం + "\n");
        puts("VL_DO_DANGLING(std::free(const_cast<char*>(vlSelf->vlNamep)), "
             "vlSelf->vlNamep);\n");
        puts("}\n");

        // C++ destructor
        puts("\n" + modName + "::~" + modName + "() {\n");
        emitSystemCSection(modp, VSystemCSectionType::DTOR);
        puts(modName + "__dtor(this);\n");
        puts("}\n");
    }

    శూన్య emitConfigureImp(స్థిరరాశి Ast_కణుపు_Module* modp) {
        స్థిరరాశి string modName = EmitCUtil::ఉపసర్గ_Name_Protect(modp);

        ఒకవేళ (v3Global.opt.coverage()) {
            puts("\n");
            m_lazyDecls.emit("void " + modName + "__", protect("_configure_coverage"),
                             "(" + modName + "* vlSelf, bool first);");
        }

        ఒకవేళ (v3Global.opt.coverage()) {
            puts("\nvoid " + modName + "::" + protect("__Vconfigure") + "(bool first) {\n");
            puts("(void)first;  // Prevent unused variable warning\n");
            puts(modName + "__" + protect("_configure_coverage") + "(this, first);\n");
            puts("}\n");
        }
    }
    శూన్య emitCoverageImp() {
        // Rather than putting out VL_COVER_INSERT calls directly, we do it via this
        // function. This gets around gcc slowness constructing all of the template
        // arguments.
        ఒకవేళ (v3Global.opt.coverage()) {
            puts("\n// Coverage\n");
            puts("void " + EmitCUtil::ఉపసర్గ_Name_Protect(m_modp) + "::__vlCoverInsert(");
            puts(v3Global.opt.దారాలు() > 1 ? "std::atomic<uint32_t>" : "uint32_t");
            puts("* countp, bool enable, bool localCounter, const char* filenamep, int lineno, "
                 "int column,\n");
            puts("const char* hierp, const char* pagep, const char* commentp, const char* "
                 "linescovp,\n");
            puts("const char* fsmVarp, const char* fsmFromp, const char* fsmTop, const char* "
                 "fsmTagp) {\n");
            ఒకవేళ (v3Global.opt.దారాలు() > 1) {
                puts("assert(sizeof(uint32_t) == sizeof(std::atomic<uint32_t>));\n");
                puts("uint32_t* count32p = reinterpret_cast<uint32_t*>(countp);\n");
            } లేకపోతే {
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
        ఒకవేళ (v3Global.opt.coverageToggle()) {
            puts("\n// Toggle Coverage\n");
            puts("void " + EmitCUtil::ఉపసర్గ_Name_Protect(m_modp) + "::__vlCoverToggleInsert(");
            puts("int begin, int end, bool ranged, ");
            puts(v3Global.opt.దారాలు() > 1 ? "std::atomic<uint32_t>" : "uint32_t");
            puts("* countp, bool enable, bool localCounter, const char* filenamep, int lineno, "
                 "int column,\n");
            puts("const char* hierp, const char* pagep, const char* commentp) {\n");
            ఒకవేళ (v3Global.opt.దారాలు() > 1) {
                puts("assert(sizeof(uint32_t) == sizeof(std::atomic<uint32_t>));\n");
            }
            puts("int step = (end >= begin) ? 1 : -1;\n");
            // range is inclusive
            puts("for (int i = begin; i != end + step; i += step) {\n");
            puts("for (int j = 0; j < 2; j++) {\n");
            ఒకవేళ (v3Global.opt.దారాలు() > 1) {
                puts("uint32_t* count32p = reinterpret_cast<uint32_t*>(countp);\n");
            } లేకపోతే {
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
                                       //ఖాళీలు
            puts("++countp;\n");
            puts("}\n");
            puts("}\n");
            puts("}\n");
        }
    }
    శూన్య emitSavableImp(స్థిరరాశి Ast_కణుపు_Module* modp) {
        ఒకవేళ (v3Global.opt.savable()) {
            puts("\n// Savable\n");
            కొరకు (పూర్ణాంకం de = 0; de < 2; ++de) {
                స్థిరరాశి string classname = de ? "VerilatedDeserialize" : "VerilatedSerialize";
                స్థిరరాశి string funcname = de ? "__Vdeserialize" : "__Vserialize";
                స్థిరరాశి string op = de ? ">>" : "<<";
                // NOLINTNEXTLINE(performance-inefficient-string-concatenation)
                putns(modp, "void " + EmitCUtil::ఉపసర్గ_Name_Protect(modp) + "::" + protect(funcname)
                                + "(" + classname + "& os) {\n");
                // Place a computed checksum to ensure proper structure save/restore formatting
                // OK if this hash includes some things we won't dump, since
                // just looking for loading the wrong model
                VHashSha512 hash;
                కొరకు (Ast_కణుపు* nodep = modp->stmtsp(); nodep; nodep = nodep->nextp()) {
                    ఒకవేళ (స్థిరరాశి AstVar* స్థిరరాశి varp = VN_CAST(nodep, Var)) {
                        hash.చొప్పించు(varp->name());
                        hash.చొప్పించు(varp->dtypep()->వెడల్పు());
                    }
                }
                ofp()->రాయి("uint64_t __Vcheckval = 0x%" PRIx64 "ULL;\n",
                              static_cast<uint64_t>(hash.digestUInt64()));
                ఒకవేళ (de) {
                    puts("os.readAssert(__Vcheckval);\n");
                } లేకపోతే {
                    puts("os << __Vcheckval;\n");
                }

                // Save context
                // If multiple models save the same context we'll save it multiple
                // times, but is harmless, and doing it otherwise would break
                // backwards compatibility.
                puts("os " + op + " vlSymsp->_vm_contextp__;\n");

                // Save all members
                కొరకు (Ast_కణుపు* nodep = modp->stmtsp(); nodep; nodep = nodep->nextp()) {
                    ఒకవేళ (స్థిరరాశి AstVar* స్థిరరాశి varp = VN_CAST(nodep, Var)) {
                        ఒకవేళ (varp->isIO() && modp->is_శిఖరం() && optSystemC()) {
                            // System C top I/O doesn't need loading, as the
                            // lower level subinst code does it.
                        } లేకపోతే ఒకవేళ (varp->isParam()) {
                        } లేకపోతే ఒకవేళ (VN_IS(varp->dtypep(), NBACommit_వరుస_DType)) {
                        } లేకపోతే {
                            పూర్ణాంకం vects = 0;
                            Ast_కణుపు_DType* elementp = varp->dtypeSkipRefp();
                            కొరకు (AstUnpackArrayDType* arrayp = VN_CAST(elementp, UnpackArrayDType);
                                 arrayp; arrayp = VN_CAST(elementp, UnpackArrayDType)) {
                                స్థిరరాశి పూర్ణాంకం vecnum = vects++;
                                UASSERT_OBJ(arrayp->hi() >= arrayp->lo(), varp,
                                            "Should have swapped msb & lsb earlier.");
                                స్థిరరాశి string ivar = "__Vi"s + cvt_గమ్యం_Str(vecnum);
                                puts("for (int __Vi" + cvt_గమ్యం_Str(vecnum) + " = " + cvt_గమ్యం_Str(0));
                                puts("; " + ivar + " < " + cvt_గమ్యం_Str(arrayp->elementsConst()));
                                puts("; ++" + ivar + ") {\n");
                                elementp = arrayp->subDTypep()->skipRefp();
                            }
                            స్థిరరాశి AstBasicDType* స్థిరరాశి basicp = elementp->basicp();
                            // Do not save MTask state, only matters within an evaluation
                            ఒకవేళ (basicp && basicp->keyword().isMTaskState()) కొనసాగు;
                            // Want to detect types that are represented as arrays
                            // (i.e. packed types of more than 64 bits).
                            ఒకవేళ (elementp->isWide()
                                && !(basicp && basicp->keyword() == VBasicDTypeKwd::STRING)) {
                                స్థిరరాశి పూర్ణాంకం vecnum = vects++;
                                స్థిరరాశి string ivar = "__Vi"s + cvt_గమ్యం_Str(vecnum);
                                puts("for (int __Vi" + cvt_గమ్యం_Str(vecnum) + " = " + cvt_గమ్యం_Str(0));
                                puts("; " + ivar + " < " + cvt_గమ్యం_Str(elementp->వెడల్పు_పదాలు()));
                                puts("; ++" + ivar + ") {\n");
                            }
                            putns(varp, "os" + op + varName(varp));
                            కొరకు (పూర్ణాంకం v = 0; v < vects; ++v) puts("[__Vi" + cvt_గమ్యం_Str(v) + "]");
                            puts(";\n");
                            కొరకు (పూర్ణాంకం v = 0; v < vects; ++v) puts("}\n");
                        }
                    }
                }

                puts("}\n");
            }
        }
    }
    // Predicate to check if we actually need to emit anything into the common implementation file.
    // Used to avoid creating empty output files.
    తార్కిక hasCommonImp(స్థిరరాశి Ast_కణుపు_Module* modp) స్థిరరాశి {
        // Nothing to emit if no module!
        ఒకవేళ (!modp) తిరిగి అబద్ధం;
        // We always need the slow file
        ఒకవేళ (m_slow) తిరిగి నిజం;
        // The fast file is only required when we have `systemc_implementation nodes
        కొరకు (స్థిరరాశి AstSystemCSection* scSectionp = modp->scSectionsp(); scSectionp;
             scSectionp = VN_AS(scSectionp->nextp(), SystemCSection)) {
            ఒకవేళ (scSectionp->sectionType() == VSystemCSectionType::IMP) తిరిగి నిజం;
        }
        తిరిగి అబద్ధం;
    }
    // Actually emit common implementation contents for given AstNodeModule
    శూన్య doCommonImp(స్థిరరాశి Ast_కణుపు_Module* modp) {
        ఒకవేళ (m_slow) {
            ఒకవేళ (!VN_IS(modp, Class)) {
                emitParamDefns(modp);
                emitCtorDtorImp(modp);
                emitConfigureImp(modp);
                emitCoverageImp();
            }
            emitSavableImp(modp);
        } లేకపోతే {
            // From `systemc_implementation
            emitSystemCSection(modp, VSystemCSectionType::IMP);
        }
    }
    శూన్య emitCommonImp(స్థిరరాశి Ast_కణుపు_Module* modp) {
        స్థిరరాశి AstClass* స్థిరరాశి classp
            = VN_IS(modp, ClassPackage) ? VN_AS(modp, ClassPackage)->classp() : శూన్యసూచిక;

        ఒకవేళ (hasCommonImp(modp) || hasCommonImp(classp)) {
            // Create ctor/dtor statements. If these need splitting, they will be emitted
            // during construction so do this before opening the output file.
            ఒకవేళ (m_slow && !VN_IS(modp, Class)) createCtorDtorStmts(modp);

            open_తదుపరి_Output_File(m_file_పునాది_Name);

            doCommonImp(modp);
            ఒకవేళ (classp) {
                VL_RESTORER(m_modp);
                m_modp = classp;
                doCommonImp(classp);
            }

            closeOutputFile();
        }
    }
    శూన్య emitCFuncImp(స్థిరరాశి Ast_కణుపు_Module* modp) {
        // Functions to be emitted here
        std::సదిశ<AstCFunc*> funcps;

        స్థిరరాశి స్వయంచాలక gather = [ఇది, &funcps](స్థిరరాశి Ast_కణుపు_Module* modp) {
            కొరకు (Ast_కణుపు* nodep = modp->stmtsp(); nodep; nodep = nodep->nextp()) {
                AstCFunc* స్థిరరాశి funcp = VN_CAST(nodep, CFunc);
                ఒకవేళ (!funcp) కొనసాగు;
                // TRACE_* and DPI handled elsewhere
                ఒకవేళ (funcp->isTrace()) కొనసాగు;
                ఒకవేళ (funcp->dpiImportPrototype()) కొనసాగు;
                ఒకవేళ (funcp->dpiExportDispatcher()) కొనసాగు;
                ఒకవేళ (funcp->slow() != m_slow) కొనసాగు;
                funcps.నెట్టు_వెనుక(funcp);
            }
        };

        gather(modp);
        VL_RESTORER(m_classOrPackage);
        ఒకవేళ (స్థిరరాశి AstClassPackage* స్థిరరాశి packagep = VN_CAST(modp, ClassPackage)) {
            m_classOrPackage = packagep;
            gather(packagep->classp());
        }

        // Do not create empty files
        ఒకవేళ (funcps.ఖాళీలు()) తిరిగి;

        // Open output file
        open_తదుపరి_Output_File(m_unique_పేర్లు.get(m_file_పునాది_Name));
        // Emit all functions
        కొరకు (AstCFunc* స్థిరరాశి funcp : funcps) {
            VL_RESTORER(m_modp);
            m_modp = EmitCParentModule::get(funcp);
            iterateConst(funcp);
        }
        // Close output file
        closeOutputFile();
    }

    // VISITORS
    శూన్య సందర్శించు(AstCFunc* nodep) override {
        ఒకవేళ (విభజించు_Needed()) {
            // Splitting file, so using parallel build.
            v3Global.useParallelBuild(నిజం);
            // Close old file
            closeOutputFile();
            // Open a new file
            open_తదుపరి_Output_File(m_unique_పేర్లు.get(m_file_పునాది_Name));
        }

        EmitCFunc::సందర్శించు(nodep);
    }

    స్పష్టమైన EmitCImp(స్థిరరాశి Ast_కణుపు_Module* modp, తార్కిక slow)
        : m_fileModp{modp}
        , m_slow{slow} {
        UINFO(5, "  Emitting implementation of " << EmitCUtil::ఉపసర్గ_Name_Protect(modp));

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
    ~EmitCImp() override = అప్రమేయ;

బహిరంగ:
    స్థిర std::సదిశ<AstCFile*> ప్రధాన(స్థిరరాశి Ast_కణుపు_Module* modp, తార్కిక slow) VL_MT_STABLE {
        EmitCImp emitCImp{modp, slow};
        తిరిగి emitCImp.getAndClearCfileps();
    }
};

//######################################################################
// Tracing routines

// Trace type descriptors go in a different file as it needs to be written in
// parallel with the actual trace function source files
తరగతి EmitCTraceTypes final : బహిరంగ EmitCFunc {
    // NODE STATE/TYPES
    // None allowed to support threaded emitting

    // STATE
    పూర్ణాంకం m_enum_అంకె = 0;  // Enumeration number (whole netlist)
    std::unordered_map<Ast_కణుపు*, పూర్ణాంకం> m_enum_అంకె_Map;  // EnumDType to enumeration number
    పూర్ణాంకం m_traceTypeSubs = 0;  // Number of trace type declaration sub-functions
    V3_Unique_పేర్లు m_unique_పేర్లు;  // Generates unique file names
    స్థిరరాశి std::string m_file_పునాది_Name = EmitCUtil::శిఖరం_Class_Name() + "_" + protect("_TraceDecls");
    // This one uses CSplitTrace for file splitting, which is incorrect but historically accurates
    స్థిరరాశి కొలత_t m_విభజించు_హద్దు = v3Global.opt.output_విభజించు_CTrace()
                                    ? static_cast<కొలత_t>(v3Global.opt.output_విభజించు_CTrace())
                                    : std::numeric_limits<కొలత_t>::max();

    శూన్య open_తదుపరి_Output_File() {
        open_New_Output_ఆధారం_File(m_unique_పేర్లు.get(m_file_పునాది_Name), నిజం, నిజం,
                                "Tracing declarations");
        puts("\n");
        కొరకు (స్థిరరాశి std::string& పునాది : v3Global.opt.trace_ఆధారం_Langs()) {
            puts("#include \"" + పునాది + ".h\"\n");
        }
        puts("\n");
        puts("\nvoid " + EmitCUtil::ఉపసర్గ_Name_Protect(m_modp) + "__"
             + protect("traceDeclTypesSub" + std::గమ్యం_string(m_traceTypeSubs++)) + "("
             + v3Global.opt.trace_Class_పునాది() + "* tracep) {\n");
    }

బహిరంగ:
    // METHODS
    పూర్ణాంకం get_Enum_Map_అంకె(AstEnumDType* nodep) {
        పూర్ణాంకం& enumNumr = m_enum_అంకె_Map[nodep];
        ఒకవేళ (!enumNumr) {
            ఒకవేళ (విభజించు_Needed(m_విభజించు_హద్దు)) {
                // Splitting file, so using parallel build.
                v3Global.useParallelBuild(నిజం);
                puts("}\n");
                closeOutputFile();
                open_తదుపరి_Output_File();
            }

            enumNumr = ++m_enum_అంకె;
            పూర్ణాంకం nvals = 0;
            puts("{\n");
            putns(nodep, "const char* " + protect("__VenumItemNames") + "[]\n");
            puts("= {");
            కొరకు (Ast_Enum_అంశం* itemp = nodep->itemsp(); itemp;
                 itemp = VN_AS(itemp->nextp(), Enum_అంశం)) {
                ఒకవేళ (++nvals > 1) puts(", ");
                putbs("\"" + itemp->prettyName() + "\"");
            }
            puts("};\n");
            nvals = 0;
            puts("const char* " + protect("__VenumItemValues") + "[]\n");
            puts("= {");
            కొరకు (Ast_Enum_అంశం* itemp = nodep->itemsp(); itemp;
                 itemp = VN_AS(itemp->nextp(), Enum_అంశం)) {
                AstConst* స్థిరరాశి constp = VN_AS(itemp->valuep(), Const);
                ఒకవేళ (++nvals > 1) puts(", ");
                putbs("\"" + constp->అంకె().displayed(nodep, "%0b") + "\"");
            }
            puts("};\n");
            puts("tracep->declDTypeEnum(" + std::గమ్యం_string(enumNumr) + ", \"" + nodep->prettyName()
                 + "\", " + std::గమ్యం_string(nvals) + ", " + std::గమ్యం_string(nodep->వెడల్పు_Min()) + ", "
                 + protect("__VenumItemNames") + ", " + protect("__VenumItemValues") + ");\n");
            puts("}\n");
            విభజించు_కొలత_Inc(Ast_కణుపు::INSTR_లెక్క_CALL);
        }
        తిరిగి enumNumr;
    }

    // Close output file
    శూన్య finalize() {
        // Close function definition
        puts("}\n");

        స్థిరరాశి std::string modName = EmitCUtil::ఉపసర్గ_Name_Protect(m_modp);
        స్థిరరాశి std::string వాదనలు = v3Global.opt.trace_Class_పునాది() + "* tracep";

        // Forward declarations for subs in other files
        కొరకు (పూర్ణాంకం i = 0; i < m_traceTypeSubs - 1; ++i) {
            puts("void " + modName + "__" + protect("traceDeclTypesSub" + std::గమ్యం_string(i)) + "("
                 + వాదనలు + ");\n");
        }

        // Create top level trace_decl_types function and call each sub-function
        puts("\nvoid " + modName + "__" + protect("trace_decl_types") + "(" + వాదనలు + ") {\n");
        కొరకు (పూర్ణాంకం i = 0; i < m_traceTypeSubs; ++i) {
            puts(modName + "__" + protect("traceDeclTypesSub" + std::గమ్యం_string(i))
                 + "(tracep);\n");
        }
        puts("}\n");

        closeOutputFile();
    }

    EmitCTraceTypes() {
        m_modp = v3Global.rootp()->శిఖరం_Modulep();
        open_తదుపరి_Output_File();
    }
    ~EmitCTraceTypes() override = అప్రమేయ;
};

తరగతి EmitCTrace final : బహిరంగ EmitCFunc {
    // NODE STATE/TYPES
    // None allowed to support threaded emitting

    // MEMBERS
    స్థిరరాశి తార్కిక m_slow;  // Making slow file
    స్థిరరాశి std::unique_ptr<EmitCTraceTypes> m_emitTypesp{m_slow ? కొత్త EmitCTraceTypes{} : శూన్యసూచిక};
    V3_Unique_పేర్లు m_unique_పేర్లు;  // Generates unique file names
    స్థిరరాశి std::string m_file_పునాది_Name = EmitCUtil::శిఖరం_Class_Name() + "_" + protect("_Trace");

    // METHODS
    శూన్య open_తదుపరి_Output_File() {
        open_New_Output_ఆధారం_File(m_unique_పేర్లు.get(m_file_పునాది_Name), m_slow, నిజం,
                                "Tracing implementation internals");
        puts("\n");
        కొరకు (స్థిరరాశి std::string& పునాది : v3Global.opt.trace_ఆధారం_Langs()) {
            puts("#include \"" + పునాది + ".h\"\n");
        }
        puts("#include \"" + EmitCUtil::symClassName() + ".h\"\n");
        puts("\n");
        // Need to emit new lazy declarations
        m_lazyDecls.reset();
    }

    తార్కిక emitTraceIsScBv(స్థిరరాశి AstTraceInc* nodep) {
        స్థిరరాశి AstVarRef* స్థిరరాశి varrefp = VN_CAST(nodep->declp()->valuep(), VarRef);
        ఒకవేళ (!varrefp) తిరిగి అబద్ధం;
        స్థిరరాశి AstVar* స్థిరరాశి varp = varrefp->varp();
        తిరిగి varp->isSc() && varp->isScBv();
    }

    తార్కిక emitTraceIsScBigUint(స్థిరరాశి AstTraceInc* nodep) {
        స్థిరరాశి AstVarRef* స్థిరరాశి varrefp = VN_CAST(nodep->declp()->valuep(), VarRef);
        ఒకవేళ (!varrefp) తిరిగి అబద్ధం;
        స్థిరరాశి AstVar* స్థిరరాశి varp = varrefp->varp();
        తిరిగి varp->isSc() && varp->isScBigUint();
    }

    తార్కిక emitTraceIsScUint(స్థిరరాశి AstTraceInc* nodep) {
        స్థిరరాశి AstVarRef* స్థిరరాశి varrefp = VN_CAST(nodep->declp()->valuep(), VarRef);
        ఒకవేళ (!varrefp) తిరిగి అబద్ధం;
        స్థిరరాశి AstVar* స్థిరరాశి varp = varrefp->varp();
        తిరిగి varp->isSc() && (varp->isScUint() || varp->isScUintBool());
    }

    శూన్య emitTraceInitOne(స్థిరరాశి AstTraceDecl* nodep, పూర్ణాంకం enum_అంకె) {
        std::string direction;
        direction = nodep->declDirection().traceSigDirection();

        AstCCall* స్థిరరాశి callp = nodep->dtypeCallp();
        ఒకవేళ (callp) {
            callp->argTypes(callp->argTypes() + ", " + cvt_గమ్యం_Str(nodep->fidx()) + ", c+"
                            + cvt_గమ్యం_Str(nodep->సంకేతం_2()) + ", " + direction);
            తిరిగి;
        }

        ఒకవేళ (nodep->dtypep()->basicp()->isDouble()) {
            puts("VL_TRACE_DECL_DOUBLE");
        } లేకపోతే ఒకవేళ (nodep->isWide()) {
            puts("VL_TRACE_DECL_WIDE");
        } లేకపోతే ఒకవేళ (nodep->isQuad()) {
            puts("VL_TRACE_DECL_QUAD");
        } లేకపోతే ఒకవేళ (nodep->బిట్టు_Range().ranged()) {
            puts("VL_TRACE_DECL_BUS");
        } లేకపోతే ఒకవేళ (nodep->dtypep()->basicp()->isEvent()) {
            puts("VL_TRACE_DECL_EVENT");
        } లేకపోతే {
            puts("VL_TRACE_DECL_BIT");
        }

        ఒకవేళ (nodep->arrayRange().ranged()) {
            puts("_ARRAY(tracep");
        } లేకపోతే {
            puts("(tracep");
        }

        //సంకేతం
        puts(",c+" + cvt_గమ్యం_Str(nodep->సంకేతం_2()));
        ఒకవేళ (nodep->arrayRange().ranged()) puts("+i*" + cvt_గమ్యం_Str(nodep->వెడల్పు_పదాలు()));

        // Function index
        puts(",");
        ఒకవేళ (nodep->లోపల_Dtype_Func()) {
            puts("fidx");
        } లేకపోతే {
            puts(cvt_గమ్యం_Str(nodep->fidx()));
        }

        // Name
        puts(",");
        putsQuoted(VIdProtect::protect_పదాలు_If(nodep->showname(), nodep->protect()));

        // Enum number
        puts("," + cvt_గమ్యం_Str(enum_అంకె));

        // Direction
        ఒకవేళ (nodep->లోపల_Dtype_Func()) {
            puts(", direction");
        } లేకపోతే {
            puts(", " + direction);
        }

        //రకం
        puts(", VerilatedTraceSigKind::");
        puts(nodep->varType().trace_Sig_రకం());

        // Type
        puts(", VerilatedTraceSigType::");
        UASSERT_OBJ(nodep->dtypep()->basicp()->keyword() != VBasicDTypeKwd::UNKNOWN, nodep,
                    "DType is unknown");
        puts(nodep->dtypep()->basicp()->keyword().traceSigType());

        // Array range
        ఒకవేళ (nodep->arrayRange().ranged()) {
            ఒకవేళ (nodep->arrayRange().ascending()) {
                puts(", (i + " + std::గమ్యం_string(nodep->arrayRange().lo()) + ")");
            } లేకపోతే {
                puts(", (" + std::గమ్యం_string(nodep->arrayRange().hi()) + " - i)");
            }
        }

        // Bit range
        ఒకవేళ (!nodep->dtypep()->basicp()->isDouble() && nodep->బిట్టు_Range().ranged()) {
            puts(", " + cvt_గమ్యం_Str(nodep->బిట్టు_Range().ఎడమ()) + ","
                 + cvt_గమ్యం_Str(nodep->బిట్టు_Range().కుడి()));
        }

        //
        puts(");");
    }

    పూర్ణాంకం emitTraceDeclDType(Ast_కణుపు_DType* nodep) {
        // Return enum number or -1 for none
        ఒకవేళ (v3Global.opt.traceEnabledFst()) {
            // Skip over refs-to-refs, but stop before final ref so can get data type name
            // Alternatively back in V3Width we could push enum names from upper typedefs
            ఒకవేళ (AstEnumDType* స్థిరరాశి enump = VN_CAST(nodep->skip_Ref_గమ్యం_Enump(), EnumDType)) {
                తిరిగి m_emitTypesp->get_Enum_Map_అంకె(enump);
            }
        }
        తిరిగి -1;
    }

    శూన్య emitTraceChangeOne(AstTraceInc* nodep, పూర్ణాంకం arrayindex) {
        // Note: Both VTraceType::CHANGE and VTraceType::FULL use the 'full' methods
        స్థిరరాశి std::string func = nodep->traceType() == VTraceType::CHANGE ? "chg" : "full";
        తార్కిక emit_వెడల్పు = నిజం;
        string stype;
        ఒకవేళ (nodep->dtypep()->basicp()->isDouble()) {
            stype = "Double";
            emit_వెడల్పు = అబద్ధం;
        } లేకపోతే ఒకవేళ (nodep->isWide() || emitTraceIsScBv(nodep) || emitTraceIsScBigUint(nodep)) {
            stype = "WData";
        } లేకపోతే ఒకవేళ (nodep->isQuad()) {
            stype = "QData";
        } లేకపోతే ఒకవేళ (nodep->declp()->వెడల్పు_Min() > 16) {
            stype = "IData";
        } లేకపోతే ఒకవేళ (nodep->declp()->వెడల్పు_Min() > 8) {
            stype = "SData";
        } లేకపోతే ఒకవేళ (nodep->declp()->వెడల్పు_Min() > 1) {
            stype = "CData";
        } లేకపోతే ఒకవేళ (nodep->dtypep()->basicp()->isEvent()) {
            stype = "Event";
            emit_వెడల్పు = అబద్ధం;
        } లేకపోతే {
            stype = "Bit";
            emit_వెడల్పు = అబద్ధం;
        }
        putns(nodep, "bufp->" + func + stype);

        స్థిరరాశి uint32_t స్థానభ్రంశం = (arrayindex < 0) ? 0 : (arrayindex * nodep->declp()->వెడల్పు_పదాలు());
        స్థిరరాశి uint32_t సంకేతం_2 = nodep->declp()->సంకేతం_2() + స్థానభ్రంశం;
        // Note: Both VTraceType::CHANGE and VTraceType::FULL use the 'full' methods
        puts("(oldp+");
        puts(cvt_గమ్యం_Str(సంకేతం_2 - nodep->పునాది_సంకేతం()));
        puts(",");
        స్థిరరాశి VNumRange& arrayRange = nodep->declp()->arrayRange();
        ఒకవేళ (arrayRange.ranged() && !arrayRange.ascending()) {
            arrayindex = arrayRange.elements() - 1 - arrayindex;
        }
        emit_Trace_విలువ(nodep, arrayindex);
        ఒకవేళ (emit_వెడల్పు) puts("," + cvt_గమ్యం_Str(nodep->declp()->వెడల్పు_Min()));
        puts(");\n");
    }

    శూన్య emit_Trace_విలువ(స్థిరరాశి AstTraceInc* nodep, పూర్ణాంకం arrayindex) {
        ఒకవేళ (AstVarRef* స్థిరరాశి varrefp = VN_CAST(nodep->valuep(), VarRef)) {
            స్థిరరాశి AstVar* స్థిరరాశి varp = varrefp->varp();
            ఒకవేళ (varp->isEvent()) puts("&");
            puts("(");
            ఒకవేళ (emitTraceIsScBigUint(nodep)) {
                puts("(uint32_t*)");
            } లేకపోతే ఒకవేళ (emitTraceIsScBv(nodep)) {
                puts("VL_SC_BV_DATAP(");
            }
            iterateConst(varrefp);  // Put var name out
            emit_Trace_సూచీ(nodep, arrayindex);
            ఒకవేళ (varp->isSc()) puts(".read()");
            ఒకవేళ (emitTraceIsScUint(nodep)) {
                puts(nodep->isQuad() ? ".to_uint64()" : ".to_uint()");
            } లేకపోతే ఒకవేళ (emitTraceIsScBigUint(nodep)) {
                puts(".get_raw()");
            } లేకపోతే ఒకవేళ (emitTraceIsScBv(nodep)) {
                puts(")");
            }
            puts(")");
        } లేకపోతే {
            puts("(");
            iterateConst(nodep->valuep());
            emit_Trace_సూచీ(nodep, arrayindex);
            puts(")");
        }
    }

    శూన్య emit_Trace_సూచీ(స్థిరరాశి AstTraceInc* స్థిరరాశి nodep, పూర్ణాంకం arrayindex) {
        // Tracing only supports 1D arrays
        ఒకవేళ (nodep->declp()->arrayRange().ranged()) {
            ఒకవేళ (arrayindex == -2) {
                puts("[i]");
            } లేకపోతే ఒకవేళ (arrayindex == -1) {
                puts("[0]");
            } లేకపోతే {
                puts("[" + cvt_గమ్యం_Str(arrayindex) + "]");
            }
        }
    }

    // VISITORS
    ఉపయోగించి EmitCFunc::సందర్శించు;  // Suppress hidden overloaded virtual function warning
    శూన్య సందర్శించు(AstCFunc* nodep) override {
        ఒకవేళ (!nodep->isTrace()) తిరిగి;
        ఒకవేళ (nodep->slow() != m_slow) తిరిగి;

        ఒకవేళ (విభజించు_Needed()) {
            // Splitting file, so using parallel build.
            v3Global.useParallelBuild(నిజం);
            // Close old file
            closeOutputFile();
            // Open a new file
            open_తదుపరి_Output_File();
        }

        EmitCFunc::సందర్శించు(nodep);
    }
    శూన్య సందర్శించు(Ast_Trace_నెట్టు_ఉపసర్గ* nodep) override {
        putns(nodep, "VL_TRACE_PUSH_PREFIX(tracep, ");
        ఒకవేళ (nodep->quoted_ఉపసర్గ()) {
            putsQuoted(VIdProtect::protect_పదాలు_If(nodep->ఉపసర్గ(), nodep->protect()));
        } లేకపోతే {
            puts(nodep->ఉపసర్గ());
        }
        puts(", VerilatedTracePrefixType::");
        puts(nodep->ఉపసర్గ_Type().ascii());
        puts(", " + std::గమ్యం_string(nodep->ఎడమ()));
        puts(", " + std::గమ్యం_string(nodep->కుడి()));
        puts(");\n");
    }
    శూన్య సందర్శించు(Ast_Trace_తీయి_ఉపసర్గ* nodep) override {  //
        putns(nodep, "VL_TRACE_POP_PREFIX(tracep);\n");
    }
    శూన్య సందర్శించు(AstTraceDecl* nodep) override {
        స్థిరరాశి పూర్ణాంకం enum_అంకె = emitTraceDeclDType(nodep->dtypep());
        putns(nodep, "");
        ఒకవేళ (nodep->arrayRange().ranged()) {
            puts("for (int i = 0; i < " + cvt_గమ్యం_Str(nodep->arrayRange().elements()) + "; ++i) {\n");
            emitTraceInitOne(nodep, enum_అంకె);
            puts("\n}\n");
        } లేకపోతే {
            emitTraceInitOne(nodep, enum_అంకె);
            puts("\n");
        }
    }
    శూన్య సందర్శించు(AstTraceInc* nodep) override {
        ఒకవేళ (nodep->declp()->arrayRange().ranged()) {
            // It traces faster if we unroll the loop
            కొరకు (పూర్ణాంకం i = 0; i < nodep->declp()->arrayRange().elements(); i++) {
                emitTraceChangeOne(nodep, i);
            }
        } లేకపోతే {
            emitTraceChangeOne(nodep, -1);
        }
    }

    స్పష్టమైన EmitCTrace(తార్కిక slow)
        : m_slow{slow} {
        m_modp = v3Global.rootp()->శిఖరం_Modulep();
        // Open output file
        open_తదుపరి_Output_File();
        // Emit functions
        కొరకు (Ast_కణుపు* nodep = m_modp->stmtsp(); nodep; nodep = nodep->nextp()) {
            ఒకవేళ (AstCFunc* స్థిరరాశి funcp = VN_CAST(nodep, CFunc)) iterateConst(funcp);
        }
        // Close output file
        closeOutputFile();
        ఒకవేళ (m_slow) m_emitTypesp->finalize();
    }
    ~EmitCTrace() override = అప్రమేయ;

బహిరంగ:
    స్థిర std::సదిశ<AstCFile*> ప్రధాన(తార్కిక slow) VL_MT_STABLE {
        EmitCTrace emitCTrace{slow};
        std::సదిశ<AstCFile*> cfileps = emitCTrace.getAndClearCfileps();
        ఒకవేళ (slow) {
            కొరకు (AstCFile* స్థిరరాశి cfilep : emitCTrace.m_emitTypesp->getAndClearCfileps()) {
                cfileps.emplace_వెనుక(cfilep);
            }
        }
        తిరిగి cfileps;
    }
};

//######################################################################
// Existing AstCFile emitter

తరగతి EmitCFile final : బహిరంగ EmitCFunc {
    స్పష్టమైన EmitCFile(AstCFile* cfilep) {
        openOutputFile(cfilep, "Generated C++");
        iterateConst(cfilep->tblockp());
        closeOutputFile();
    }
    ~EmitCFile() override = అప్రమేయ;

బహిరంగ:
    స్థిర శూన్య ప్రధాన(AstCFile* cfilep) VL_MT_STABLE {
        ఒకవేళ (!cfilep->tblockp()) తిరిగి;
        EmitCFile{cfilep};
    }
};

//######################################################################
// EmitC class functions

శూన్య V3EmitC::emitcImp() {
    UINFO(2, __FUNCTION__ << ":");
    std::list<std::సదిశ<AstCFile*>> cfiles;
    {
        // Make parent module pointers available.
        స్థిరరాశి EmitCParentModule emitCParentModule;
        V3ThreadScope threadScope;

        // Process each module in turn
        కొరకు (స్థిరరాశి Ast_కణుపు* nodep = v3Global.rootp()->modulesp(); nodep; nodep = nodep->nextp()) {
            ఒకవేళ (VN_IS(nodep, Class)) కొనసాగు;  // Imped with ClassPackage
            స్థిరరాశి Ast_కణుపు_Module* స్థిరరాశి modp = VN_AS(nodep, కణుపు_Module);
            ఒకవేళ (modp->isConstPool()) కొనసాగు;  // Emitted by V3EmitCConstPool
            cfiles.emplace_వెనుక();
            std::సదిశ<AstCFile*>& slow = cfiles.వెనుక();
            threadScope.వరుసలో_చేర్చు([modp, &slow] { slow = EmitCImp::ప్రధాన(modp, /* slow: */ నిజం); });
            cfiles.emplace_వెనుక();
            std::సదిశ<AstCFile*>& fast = cfiles.వెనుక();
            threadScope.వరుసలో_చేర్చు([modp, &fast] { fast = EmitCImp::ప్రధాన(modp, /* slow: */ అబద్ధం); });
        }

        // Emit trace routines (currently they can only exist in the top module)
        ఒకవేళ (v3Global.opt.trace() && !v3Global.opt.lintOnly()) {
            cfiles.emplace_వెనుక();
            std::సదిశ<AstCFile*>& slow = cfiles.వెనుక();
            threadScope.వరుసలో_చేర్చు([&slow] { slow = EmitCTrace::ప్రధాన(/* slow: */ నిజం); });
            cfiles.emplace_వెనుక();
            std::సదిశ<AstCFile*>& fast = cfiles.వెనుక();
            threadScope.వరుసలో_చేర్చు([&fast] { fast = EmitCTrace::ప్రధాన(/* slow: */ అబద్ధం); });
        }
    }
    // Add files to netlist
    కొరకు (స్థిరరాశి std::సదిశ<AstCFile*>& cfileps : cfiles) {
        కొరకు (AstCFile* స్థిరరాశి cfilep : cfileps) v3Global.rootp()->కూడు_Filesp(cfilep);
    }
}

శూన్య V3EmitC::emitcFiles() {
    UINFO(2, __FUNCTION__ << ":");
    కొరకు (Ast_కణుపు_File *filep = v3Global.rootp()->filesp(), *nextp; filep; filep = nextp) {
        nextp = VN_AS(filep->nextp(), కణుపు_File);
        ఒకవేళ (AstCFile* స్థిరరాశి cfilep = VN_CAST(filep, CFile)) EmitCFile::ప్రధాన(cfilep);
    }
}
