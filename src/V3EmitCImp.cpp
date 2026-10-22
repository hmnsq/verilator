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

#ضمن "V3PchAstMT.h"

#ضمن "V3EmitC.h"
#ضمن "V3EmitCFunc.h"
#ضمن "V3ThreadPool.h"
#ضمن "V3UniqueNames.h"

#ضمن <algorithm>
#ضمن <map>
#ضمن <set>
#ضمن <tuple>
#ضمن <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Internal EmitC implementation

صنف EmitCImp final : عام EmitCFunc {
    // MEMBERS
    // Base module (For non-classes, same as m_modp. For classes, it's the ClassPackage.)
    ثابت Ast_گره_Module* ثابت m_fileModp;
    ثابت منطقي m_slow;  // Creating __Slow file
    V3_Unique_نامها m_unique_نامها;  // Generates unique file names
    ثابت std::string m_file_پایه_Name = EmitCUtil::پیشوند_Name_Protect(m_fileModp);
    متن_Stmts m_ctorStmts;  // Statements constructing members in 'ctor'
    متن_Stmts m_dtorStmts;  // Statements destroying members in 'dtor'
    std::string m_ctor_تقسیم_Decl;  // Declaration of sub-function 'ctor' was split into, if any
    std::string m_dtor_تقسیم_Decl;  // Declaration of sub-function 'dtor' was split into, if any

    // METHODS
    فراغ open_بعدی_Output_File(ثابت std::string& fileName) {
        open_New_Output_منبع_File(fileName, m_slow, خطأ, "Design implementation internals");
        emit_File_سرآیند();
    }
    فراغ emit_File_سرآیند() {
        puts("// See " + EmitCUtil::بالاترین_Class_Name() + ".h for the primary calling header\n");
        puts("\n");
        puts("#include \"" + EmitCUtil::pchClassName() + ".h\"\n");
        emitSystemCSection(m_fileModp, VSystemCSectionType::IMP_HDR);
        // Need to emit new lazy declarations
        m_lazyDecls.reset();
    }

    فراغ emitParamDefns(ثابت Ast_گره_Module* modp) {
        منطقي اولی = درست;
        لكل (ثابت Ast_گره* nodep = modp->stmtsp(); nodep; nodep = nodep->nextp()) {
            إذا (ثابت AstVar* ثابت varp = VN_CAST(nodep, Var)) {
                إذا (varp->isParam()) {
                    إذا (اولی) {
                        puts("\n");
                        ثابت string modName = EmitCUtil::پیشوند_Name_Protect(modp);
                        putsDecoration(modp, "// Parameter definitions for " + modName + "\n");
                        اولی = خطأ;
                    }
                    إذا (varp->dtypep()->isLiteralType()) putns(varp, "VL_CONSTINIT_CXX20 ");
                    putns(varp, "const ");
                    emitVarDefn(varp);
                    puts(";\n");
                }
            }
        }
        إذا (!اولی) puts("\n");
    }
    // Add statements explicitly constructing, or destroying, a member AstVar
    ساكن فراغ جمع_کن_Var_Ctor_Dtor_Stmts(متن_Stmts& stmts, ثابت AstVar* varp, منطقي destroy) {
        ثابت std::string name = "vlSelf->" + varName(varp);

        // SystemC port arrays are C arrays, so construct them element by element. They are
        // default constructed (unnamed), as the elements of an array member would be.
        إذا (varp->isSc() && VN_IS(varp->dtypeSkipRefp(), UnpackArrayDType)) {
            std::وکتور<std::string> elems{name};
            لكل (ثابت AstUnpackArrayDType* adtypep
                 = VN_CAST(varp->dtypeSkipRefp(), UnpackArrayDType);
                 adtypep; adtypep = VN_CAST(adtypep->subDTypep()->skipRefp(), UnpackArrayDType)) {
                std::وکتور<std::string> subElems;
                لكل (ثابت std::string& elem : elems) {
                    لكل (صحيح i = 0; i < adtypep->elementsConst(); ++i) {
                        subElems.افزودن_پشت(elem + "[" + std::مقصد_string(i) + "]");
                    }
                }
                elems = std::move(subElems);
            }
            إذا (destroy) std::معکوس(elems.begin(), elems.انتها());
            لكل (ثابت std::string& e : elems) {
                stmts.جمع_کن((destroy ? "VL_INPLACE_DESTRUCT(" : "VL_INPLACE_CONSTRUCT(") + e + ");");
            }
            أرجع;
        }

        // Other variables are constructed whole, even if aggregates
        إذا (destroy) {
            stmts.جمع_کن("VL_INPLACE_DESTRUCT(" + name + ");");
            أرجع;
        }
        ثابت AstBasicDType* ثابت basicp = VN_CAST(varp->dtypeSkipRefp(), BasicDType);
        std::string آرگومانها;
        إذا (varp->isSc()) {
            // SystemC ports need their name
            آرگومانها = ", \"" + V3_بیرون_Formatter::quoteNameControls(varp->nameProtect()) + "\"";
        } وإلا إذا (basicp && basicp->keyword().isMTaskState()) {
            // MTask state needs the number of upstream dependencies to wait for
            آرگومانها = ", " + VN_AS(varp->valuep(), Const)->رقم().emitC();
        } وإلا إذا (basicp && basicp->isDelayScheduler()) {
            // Delay scheduler needs the context pointer
            آرگومانها = ", *vlSelf->vlSymsp->_vm_contextp__";
        }
        stmts.جمع_کن("VL_INPLACE_CONSTRUCT(" + name + آرگومانها + ");");
    }
    // Gather the statements constructing/destroying members in anonymous unions, and split
    // them into sub-functions if needed. As this emits files, no output file can be open.
    فراغ createCtorDtorStmts(ثابت Ast_گره_Module* modp) {
        // Construct member variables
        لكل (ثابت Ast_گره* nodep = modp->stmtsp(); nodep; nodep = nodep->nextp()) {
            ثابت AstVar* ثابت varp = VN_CAST(nodep, Var);
            إذا (varp && isExplicitlyConstructed(varp)) {
                جمع_کن_Var_Ctor_Dtor_Stmts(m_ctorStmts, varp, خطأ);
            }
        }

        // Destroy in reverse order
        ثابت Ast_گره* ثابت lastp = modp->stmtsp() ? modp->stmtsp()->lastp() : اشاره_تهی;
        لكل (ثابت Ast_گره* nodep = lastp; nodep; nodep = nodep->prevp()) {
            ثابت AstVar* ثابت varp = VN_CAST(nodep, Var);
            إذا (varp && isExplicitlyConstructed(varp)) {
                جمع_کن_Var_Ctor_Dtor_Stmts(m_dtorStmts, varp, درست);
            }
        }

        // Split into sub-functions if needed
        ثابت std::string modName = EmitCUtil::پیشوند_Name_Protect(modp);
        std::tie(m_ctor_تقسیم_Decl, m_dtor_تقسیم_Decl) = emit_تقسیم_Ctor_Dtor(
            m_ctorStmts, m_dtorStmts, 0, modName, modName + "* vlSelf", "vlSelf", خطأ,
            "Design implementation internals", [هذا]() { emit_File_سرآیند(); });
    }
    فراغ emitCtorDtorImp(ثابت Ast_گره_Module* modp) {
        ثابت std::string modName = EmitCUtil::پیشوند_Name_Protect(modp);

        // ctor
        إذا (modp->ctorVarReset()) {
            m_lazyDecls.emit("\nvoid " + modName + "__", protect("_ctor_var_reset"),
                             "(" + modName + "* vlSelf);\n");
        }

        إذا (!m_ctor_تقسیم_Decl.خالی()) puts("\n" + m_ctor_تقسیم_Decl + "\n");
        puts("\nvoid " + modName + "__ctor(" + modName + "* vlSelf, " + EmitCUtil::symClassName()
             + "* symsp, const char* namep) {\n");
        puts("vlSelf->vlSymsp = symsp;\n");
        إذا (modp->is_بالاترین()) {
            puts("vlSelf->vlNamep = strdup(namep);\n");
        } وإلا {
            puts("vlSelf->vlNamep = strdup(Verilated::catName(symsp->name(), namep));\n");
        }
        putsDecoration(modp, "// Construct members\n");
        لكل (ثابت متن_Stmts::Stmt& stmt : m_ctorStmts) puts(stmt.m_متن + "\n");

        putsDecoration(modp, "// Reset structure values\n");
        إذا (modp->ctorVarReset()) {
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
        إذا (!m_dtor_تقسیم_Decl.خالی()) puts("\n" + m_dtor_تقسیم_Decl + "\n");
        puts("\nvoid " + modName + "__dtor(" + modName + "* vlSelf) {\n");
        putsDecoration(modp, "// Destroy members\n");
        لكل (ثابت متن_Stmts::Stmt& stmt : m_dtorStmts) puts(stmt.m_متن + "\n");
        puts("VL_DO_DANGLING(std::free(const_cast<char*>(vlSelf->vlNamep)), "
             "vlSelf->vlNamep);\n");
        puts("}\n");

        // C++ destructor
        puts("\n" + modName + "::~" + modName + "() {\n");
        emitSystemCSection(modp, VSystemCSectionType::DTOR);
        puts(modName + "__dtor(this);\n");
        puts("}\n");
    }

    فراغ emitConfigureImp(ثابت Ast_گره_Module* modp) {
        ثابت string modName = EmitCUtil::پیشوند_Name_Protect(modp);

        إذا (v3Global.opt.coverage()) {
            puts("\n");
            m_lazyDecls.emit("void " + modName + "__", protect("_configure_coverage"),
                             "(" + modName + "* vlSelf, bool first);");
        }

        إذا (v3Global.opt.coverage()) {
            puts("\nvoid " + modName + "::" + protect("__Vconfigure") + "(bool first) {\n");
            puts("(void)first;  // Prevent unused variable warning\n");
            puts(modName + "__" + protect("_configure_coverage") + "(this, first);\n");
            puts("}\n");
        }
    }
    فراغ emitCoverageImp() {
        // Rather than putting out VL_COVER_INSERT calls directly, we do it via this
        // function. This gets around gcc slowness constructing all of the template
        // arguments.
        إذا (v3Global.opt.coverage()) {
            puts("\n// Coverage\n");
            puts("void " + EmitCUtil::پیشوند_Name_Protect(m_modp) + "::__vlCoverInsert(");
            puts(v3Global.opt.نخها() > 1 ? "std::atomic<uint32_t>" : "uint32_t");
            puts("* countp, bool enable, bool localCounter, const char* filenamep, int lineno, "
                 "int column,\n");
            puts("const char* hierp, const char* pagep, const char* commentp, const char* "
                 "linescovp,\n");
            puts("const char* fsmVarp, const char* fsmFromp, const char* fsmTop, const char* "
                 "fsmTagp) {\n");
            إذا (v3Global.opt.نخها() > 1) {
                puts("assert(sizeof(uint32_t) == sizeof(std::atomic<uint32_t>));\n");
                puts("uint32_t* count32p = reinterpret_cast<uint32_t*>(countp);\n");
            } وإلا {
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
        إذا (v3Global.opt.coverageToggle()) {
            puts("\n// Toggle Coverage\n");
            puts("void " + EmitCUtil::پیشوند_Name_Protect(m_modp) + "::__vlCoverToggleInsert(");
            puts("int begin, int end, bool ranged, ");
            puts(v3Global.opt.نخها() > 1 ? "std::atomic<uint32_t>" : "uint32_t");
            puts("* countp, bool enable, bool localCounter, const char* filenamep, int lineno, "
                 "int column,\n");
            puts("const char* hierp, const char* pagep, const char* commentp) {\n");
            إذا (v3Global.opt.نخها() > 1) {
                puts("assert(sizeof(uint32_t) == sizeof(std::atomic<uint32_t>));\n");
            }
            puts("int step = (end >= begin) ? 1 : -1;\n");
            // range is inclusive
            puts("for (int i = begin; i != end + step; i += step) {\n");
            puts("for (int j = 0; j < 2; j++) {\n");
            إذا (v3Global.opt.نخها() > 1) {
                puts("uint32_t* count32p = reinterpret_cast<uint32_t*>(countp);\n");
            } وإلا {
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
                                       //خالی
            puts("++countp;\n");
            puts("}\n");
            puts("}\n");
            puts("}\n");
        }
    }
    فراغ emitSavableImp(ثابت Ast_گره_Module* modp) {
        إذا (v3Global.opt.savable()) {
            puts("\n// Savable\n");
            لكل (صحيح de = 0; de < 2; ++de) {
                ثابت string classname = de ? "VerilatedDeserialize" : "VerilatedSerialize";
                ثابت string funcname = de ? "__Vdeserialize" : "__Vserialize";
                ثابت string op = de ? ">>" : "<<";
                // NOLINTNEXTLINE(performance-inefficient-string-concatenation)
                putns(modp, "void " + EmitCUtil::پیشوند_Name_Protect(modp) + "::" + protect(funcname)
                                + "(" + classname + "& os) {\n");
                // Place a computed checksum to ensure proper structure save/restore formatting
                // OK if this hash includes some things we won't dump, since
                // just looking for loading the wrong model
                VHashSha512 hash;
                لكل (Ast_گره* nodep = modp->stmtsp(); nodep; nodep = nodep->nextp()) {
                    إذا (ثابت AstVar* ثابت varp = VN_CAST(nodep, Var)) {
                        hash.درج(varp->name());
                        hash.درج(varp->dtypep()->عرض());
                    }
                }
                ofp()->چاپ_کن("uint64_t __Vcheckval = 0x%" PRIx64 "ULL;\n",
                              static_cast<uint64_t>(hash.digestUInt64()));
                إذا (de) {
                    puts("os.readAssert(__Vcheckval);\n");
                } وإلا {
                    puts("os << __Vcheckval;\n");
                }

                // Save context
                // If multiple models save the same context we'll save it multiple
                // times, but is harmless, and doing it otherwise would break
                // backwards compatibility.
                puts("os " + op + " vlSymsp->_vm_contextp__;\n");

                // Save all members
                لكل (Ast_گره* nodep = modp->stmtsp(); nodep; nodep = nodep->nextp()) {
                    إذا (ثابت AstVar* ثابت varp = VN_CAST(nodep, Var)) {
                        إذا (varp->isIO() && modp->is_بالاترین() && optSystemC()) {
                            // System C top I/O doesn't need loading, as the
                            // lower level subinst code does it.
                        } وإلا إذا (varp->isParam()) {
                        } وإلا إذا (VN_IS(varp->dtypep(), NBACommit_صف_DType)) {
                        } وإلا {
                            صحيح vects = 0;
                            Ast_گره_DType* elementp = varp->dtypeSkipRefp();
                            لكل (AstUnpackArrayDType* arrayp = VN_CAST(elementp, UnpackArrayDType);
                                 arrayp; arrayp = VN_CAST(elementp, UnpackArrayDType)) {
                                ثابت صحيح vecnum = vects++;
                                UASSERT_OBJ(arrayp->hi() >= arrayp->lo(), varp,
                                            "Should have swapped msb & lsb earlier.");
                                ثابت string ivar = "__Vi"s + cvt_مقصد_Str(vecnum);
                                puts("for (int __Vi" + cvt_مقصد_Str(vecnum) + " = " + cvt_مقصد_Str(0));
                                puts("; " + ivar + " < " + cvt_مقصد_Str(arrayp->elementsConst()));
                                puts("; ++" + ivar + ") {\n");
                                elementp = arrayp->subDTypep()->skipRefp();
                            }
                            ثابت AstBasicDType* ثابت basicp = elementp->basicp();
                            // Do not save MTask state, only matters within an evaluation
                            إذا (basicp && basicp->keyword().isMTaskState()) استمر;
                            // Want to detect types that are represented as arrays
                            // (i.e. packed types of more than 64 bits).
                            إذا (elementp->isWide()
                                && !(basicp && basicp->keyword() == VBasicDTypeKwd::STRING)) {
                                ثابت صحيح vecnum = vects++;
                                ثابت string ivar = "__Vi"s + cvt_مقصد_Str(vecnum);
                                puts("for (int __Vi" + cvt_مقصد_Str(vecnum) + " = " + cvt_مقصد_Str(0));
                                puts("; " + ivar + " < " + cvt_مقصد_Str(elementp->عرض_واژگان()));
                                puts("; ++" + ivar + ") {\n");
                            }
                            putns(varp, "os" + op + varName(varp));
                            لكل (صحيح v = 0; v < vects; ++v) puts("[__Vi" + cvt_مقصد_Str(v) + "]");
                            puts(";\n");
                            لكل (صحيح v = 0; v < vects; ++v) puts("}\n");
                        }
                    }
                }

                puts("}\n");
            }
        }
    }
    // Predicate to check if we actually need to emit anything into the common implementation file.
    // Used to avoid creating empty output files.
    منطقي hasCommonImp(ثابت Ast_گره_Module* modp) ثابت {
        // Nothing to emit if no module!
        إذا (!modp) أرجع خطأ;
        // We always need the slow file
        إذا (m_slow) أرجع درست;
        // The fast file is only required when we have `systemc_implementation nodes
        لكل (ثابت AstSystemCSection* scSectionp = modp->scSectionsp(); scSectionp;
             scSectionp = VN_AS(scSectionp->nextp(), SystemCSection)) {
            إذا (scSectionp->sectionType() == VSystemCSectionType::IMP) أرجع درست;
        }
        أرجع خطأ;
    }
    // Actually emit common implementation contents for given AstNodeModule
    فراغ doCommonImp(ثابت Ast_گره_Module* modp) {
        إذا (m_slow) {
            إذا (!VN_IS(modp, Class)) {
                emitParamDefns(modp);
                emitCtorDtorImp(modp);
                emitConfigureImp(modp);
                emitCoverageImp();
            }
            emitSavableImp(modp);
        } وإلا {
            // From `systemc_implementation
            emitSystemCSection(modp, VSystemCSectionType::IMP);
        }
    }
    فراغ emitCommonImp(ثابت Ast_گره_Module* modp) {
        ثابت AstClass* ثابت classp
            = VN_IS(modp, ClassPackage) ? VN_AS(modp, ClassPackage)->classp() : اشاره_تهی;

        إذا (hasCommonImp(modp) || hasCommonImp(classp)) {
            // Create ctor/dtor statements. If these need splitting, they will be emitted
            // during construction so do this before opening the output file.
            إذا (m_slow && !VN_IS(modp, Class)) createCtorDtorStmts(modp);

            open_بعدی_Output_File(m_file_پایه_Name);

            doCommonImp(modp);
            إذا (classp) {
                VL_RESTORER(m_modp);
                m_modp = classp;
                doCommonImp(classp);
            }

            closeOutputFile();
        }
    }
    فراغ emitCFuncImp(ثابت Ast_گره_Module* modp) {
        // Functions to be emitted here
        std::وکتور<AstCFunc*> funcps;

        ثابت خودکار gather = [هذا, &funcps](ثابت Ast_گره_Module* modp) {
            لكل (Ast_گره* nodep = modp->stmtsp(); nodep; nodep = nodep->nextp()) {
                AstCFunc* ثابت funcp = VN_CAST(nodep, CFunc);
                إذا (!funcp) استمر;
                // TRACE_* and DPI handled elsewhere
                إذا (funcp->isTrace()) استمر;
                إذا (funcp->dpiImportPrototype()) استمر;
                إذا (funcp->dpiExportDispatcher()) استمر;
                إذا (funcp->slow() != m_slow) استمر;
                funcps.افزودن_پشت(funcp);
            }
        };

        gather(modp);
        VL_RESTORER(m_classOrPackage);
        إذا (ثابت AstClassPackage* ثابت packagep = VN_CAST(modp, ClassPackage)) {
            m_classOrPackage = packagep;
            gather(packagep->classp());
        }

        // Do not create empty files
        إذا (funcps.خالی()) أرجع;

        // Open output file
        open_بعدی_Output_File(m_unique_نامها.get(m_file_پایه_Name));
        // Emit all functions
        لكل (AstCFunc* ثابت funcp : funcps) {
            VL_RESTORER(m_modp);
            m_modp = EmitCParentModule::get(funcp);
            iterateConst(funcp);
        }
        // Close output file
        closeOutputFile();
    }

    // VISITORS
    فراغ بازدید(AstCFunc* nodep) override {
        إذا (تقسیم_Needed()) {
            // Splitting file, so using parallel build.
            v3Global.useParallelBuild(درست);
            // Close old file
            closeOutputFile();
            // Open a new file
            open_بعدی_Output_File(m_unique_نامها.get(m_file_پایه_Name));
        }

        EmitCFunc::بازدید(nodep);
    }

    صریح EmitCImp(ثابت Ast_گره_Module* modp, منطقي slow)
        : m_fileModp{modp}
        , m_slow{slow} {
        UINFO(5, "  Emitting implementation of " << EmitCUtil::پیشوند_Name_Protect(modp));

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
    ~EmitCImp() override = افتراضي;

عام:
    ساكن std::وکتور<AstCFile*> اصلی(ثابت Ast_گره_Module* modp, منطقي slow) VL_MT_STABLE {
        EmitCImp emitCImp{modp, slow};
        أرجع emitCImp.getAndClearCfileps();
    }
};

//######################################################################
// Tracing routines

// Trace type descriptors go in a different file as it needs to be written in
// parallel with the actual trace function source files
صنف EmitCTraceTypes final : عام EmitCFunc {
    // NODE STATE/TYPES
    // None allowed to support threaded emitting

    // STATE
    صحيح m_enum_رقم = 0;  // Enumeration number (whole netlist)
    std::unordered_map<Ast_گره*, صحيح> m_enum_رقم_Map;  // EnumDType to enumeration number
    صحيح m_traceTypeSubs = 0;  // Number of trace type declaration sub-functions
    V3_Unique_نامها m_unique_نامها;  // Generates unique file names
    ثابت std::string m_file_پایه_Name = EmitCUtil::بالاترین_Class_Name() + "_" + protect("_TraceDecls");
    // This one uses CSplitTrace for file splitting, which is incorrect but historically accurates
    ثابت اندازه_t m_تقسیم_حد = v3Global.opt.output_تقسیم_CTrace()
                                    ? static_cast<اندازه_t>(v3Global.opt.output_تقسیم_CTrace())
                                    : std::numeric_limits<اندازه_t>::max();

    فراغ open_بعدی_Output_File() {
        open_New_Output_منبع_File(m_unique_نامها.get(m_file_پایه_Name), درست, درست,
                                "Tracing declarations");
        puts("\n");
        لكل (ثابت std::string& پایه : v3Global.opt.trace_منبع_Langs()) {
            puts("#include \"" + پایه + ".h\"\n");
        }
        puts("\n");
        puts("\nvoid " + EmitCUtil::پیشوند_Name_Protect(m_modp) + "__"
             + protect("traceDeclTypesSub" + std::مقصد_string(m_traceTypeSubs++)) + "("
             + v3Global.opt.trace_Class_پایه() + "* tracep) {\n");
    }

عام:
    // METHODS
    صحيح get_Enum_Map_رقم(AstEnumDType* nodep) {
        صحيح& enumNumr = m_enum_رقم_Map[nodep];
        إذا (!enumNumr) {
            إذا (تقسیم_Needed(m_تقسیم_حد)) {
                // Splitting file, so using parallel build.
                v3Global.useParallelBuild(درست);
                puts("}\n");
                closeOutputFile();
                open_بعدی_Output_File();
            }

            enumNumr = ++m_enum_رقم;
            صحيح nvals = 0;
            puts("{\n");
            putns(nodep, "const char* " + protect("__VenumItemNames") + "[]\n");
            puts("= {");
            لكل (Ast_Enum_قلم* itemp = nodep->itemsp(); itemp;
                 itemp = VN_AS(itemp->nextp(), Enum_قلم)) {
                إذا (++nvals > 1) puts(", ");
                putbs("\"" + itemp->prettyName() + "\"");
            }
            puts("};\n");
            nvals = 0;
            puts("const char* " + protect("__VenumItemValues") + "[]\n");
            puts("= {");
            لكل (Ast_Enum_قلم* itemp = nodep->itemsp(); itemp;
                 itemp = VN_AS(itemp->nextp(), Enum_قلم)) {
                AstConst* ثابت constp = VN_AS(itemp->valuep(), Const);
                إذا (++nvals > 1) puts(", ");
                putbs("\"" + constp->رقم().displayed(nodep, "%0b") + "\"");
            }
            puts("};\n");
            puts("tracep->declDTypeEnum(" + std::مقصد_string(enumNumr) + ", \"" + nodep->prettyName()
                 + "\", " + std::مقصد_string(nvals) + ", " + std::مقصد_string(nodep->عرض_Min()) + ", "
                 + protect("__VenumItemNames") + ", " + protect("__VenumItemValues") + ");\n");
            puts("}\n");
            تقسیم_اندازه_Inc(Ast_گره::INSTR_شمار_CALL);
        }
        أرجع enumNumr;
    }

    // Close output file
    فراغ finalize() {
        // Close function definition
        puts("}\n");

        ثابت std::string modName = EmitCUtil::پیشوند_Name_Protect(m_modp);
        ثابت std::string آرگومانها = v3Global.opt.trace_Class_پایه() + "* tracep";

        // Forward declarations for subs in other files
        لكل (صحيح i = 0; i < m_traceTypeSubs - 1; ++i) {
            puts("void " + modName + "__" + protect("traceDeclTypesSub" + std::مقصد_string(i)) + "("
                 + آرگومانها + ");\n");
        }

        // Create top level trace_decl_types function and call each sub-function
        puts("\nvoid " + modName + "__" + protect("trace_decl_types") + "(" + آرگومانها + ") {\n");
        لكل (صحيح i = 0; i < m_traceTypeSubs; ++i) {
            puts(modName + "__" + protect("traceDeclTypesSub" + std::مقصد_string(i))
                 + "(tracep);\n");
        }
        puts("}\n");

        closeOutputFile();
    }

    EmitCTraceTypes() {
        m_modp = v3Global.rootp()->بالاترین_Modulep();
        open_بعدی_Output_File();
    }
    ~EmitCTraceTypes() override = افتراضي;
};

صنف EmitCTrace final : عام EmitCFunc {
    // NODE STATE/TYPES
    // None allowed to support threaded emitting

    // MEMBERS
    ثابت منطقي m_slow;  // Making slow file
    ثابت std::unique_ptr<EmitCTraceTypes> m_emitTypesp{m_slow ? جديد EmitCTraceTypes{} : اشاره_تهی};
    V3_Unique_نامها m_unique_نامها;  // Generates unique file names
    ثابت std::string m_file_پایه_Name = EmitCUtil::بالاترین_Class_Name() + "_" + protect("_Trace");

    // METHODS
    فراغ open_بعدی_Output_File() {
        open_New_Output_منبع_File(m_unique_نامها.get(m_file_پایه_Name), m_slow, درست,
                                "Tracing implementation internals");
        puts("\n");
        لكل (ثابت std::string& پایه : v3Global.opt.trace_منبع_Langs()) {
            puts("#include \"" + پایه + ".h\"\n");
        }
        puts("#include \"" + EmitCUtil::symClassName() + ".h\"\n");
        puts("\n");
        // Need to emit new lazy declarations
        m_lazyDecls.reset();
    }

    منطقي emitTraceIsScBv(ثابت AstTraceInc* nodep) {
        ثابت AstVarRef* ثابت varrefp = VN_CAST(nodep->declp()->valuep(), VarRef);
        إذا (!varrefp) أرجع خطأ;
        ثابت AstVar* ثابت varp = varrefp->varp();
        أرجع varp->isSc() && varp->isScBv();
    }

    منطقي emitTraceIsScBigUint(ثابت AstTraceInc* nodep) {
        ثابت AstVarRef* ثابت varrefp = VN_CAST(nodep->declp()->valuep(), VarRef);
        إذا (!varrefp) أرجع خطأ;
        ثابت AstVar* ثابت varp = varrefp->varp();
        أرجع varp->isSc() && varp->isScBigUint();
    }

    منطقي emitTraceIsScUint(ثابت AstTraceInc* nodep) {
        ثابت AstVarRef* ثابت varrefp = VN_CAST(nodep->declp()->valuep(), VarRef);
        إذا (!varrefp) أرجع خطأ;
        ثابت AstVar* ثابت varp = varrefp->varp();
        أرجع varp->isSc() && (varp->isScUint() || varp->isScUintBool());
    }

    فراغ emitTraceInitOne(ثابت AstTraceDecl* nodep, صحيح enum_رقم) {
        std::string direction;
        direction = nodep->declDirection().traceSigDirection();

        AstCCall* ثابت callp = nodep->dtypeCallp();
        إذا (callp) {
            callp->argTypes(callp->argTypes() + ", " + cvt_مقصد_Str(nodep->fidx()) + ", c+"
                            + cvt_مقصد_Str(nodep->کد()) + ", " + direction);
            أرجع;
        }

        إذا (nodep->dtypep()->basicp()->isDouble()) {
            puts("VL_TRACE_DECL_DOUBLE");
        } وإلا إذا (nodep->isWide()) {
            puts("VL_TRACE_DECL_WIDE");
        } وإلا إذا (nodep->isQuad()) {
            puts("VL_TRACE_DECL_QUAD");
        } وإلا إذا (nodep->بیت_Range().ranged()) {
            puts("VL_TRACE_DECL_BUS");
        } وإلا إذا (nodep->dtypep()->basicp()->isEvent()) {
            puts("VL_TRACE_DECL_EVENT");
        } وإلا {
            puts("VL_TRACE_DECL_BIT");
        }

        إذا (nodep->arrayRange().ranged()) {
            puts("_ARRAY(tracep");
        } وإلا {
            puts("(tracep");
        }

        //کد
        puts(",c+" + cvt_مقصد_Str(nodep->کد()));
        إذا (nodep->arrayRange().ranged()) puts("+i*" + cvt_مقصد_Str(nodep->عرض_واژگان()));

        // Function index
        puts(",");
        إذا (nodep->درون_Dtype_Func()) {
            puts("fidx");
        } وإلا {
            puts(cvt_مقصد_Str(nodep->fidx()));
        }

        // Name
        puts(",");
        putsQuoted(VIdProtect::protect_واژگان_If(nodep->showname(), nodep->protect()));

        // Enum number
        puts("," + cvt_مقصد_Str(enum_رقم));

        // Direction
        إذا (nodep->درون_Dtype_Func()) {
            puts(", direction");
        } وإلا {
            puts(", " + direction);
        }

        //نوع
        puts(", VerilatedTraceSigKind::");
        puts(nodep->varType().trace_Sig_نوع());

        // Type
        puts(", VerilatedTraceSigType::");
        UASSERT_OBJ(nodep->dtypep()->basicp()->keyword() != VBasicDTypeKwd::UNKNOWN, nodep,
                    "DType is unknown");
        puts(nodep->dtypep()->basicp()->keyword().traceSigType());

        // Array range
        إذا (nodep->arrayRange().ranged()) {
            إذا (nodep->arrayRange().ascending()) {
                puts(", (i + " + std::مقصد_string(nodep->arrayRange().lo()) + ")");
            } وإلا {
                puts(", (" + std::مقصد_string(nodep->arrayRange().hi()) + " - i)");
            }
        }

        // Bit range
        إذا (!nodep->dtypep()->basicp()->isDouble() && nodep->بیت_Range().ranged()) {
            puts(", " + cvt_مقصد_Str(nodep->بیت_Range().چپ()) + ","
                 + cvt_مقصد_Str(nodep->بیت_Range().راست()));
        }

        //
        puts(");");
    }

    صحيح emitTraceDeclDType(Ast_گره_DType* nodep) {
        // Return enum number or -1 for none
        إذا (v3Global.opt.traceEnabledFst()) {
            // Skip over refs-to-refs, but stop before final ref so can get data type name
            // Alternatively back in V3Width we could push enum names from upper typedefs
            إذا (AstEnumDType* ثابت enump = VN_CAST(nodep->skip_Ref_مقصد_Enump(), EnumDType)) {
                أرجع m_emitTypesp->get_Enum_Map_رقم(enump);
            }
        }
        أرجع -1;
    }

    فراغ emitTraceChangeOne(AstTraceInc* nodep, صحيح arrayindex) {
        // Note: Both VTraceType::CHANGE and VTraceType::FULL use the 'full' methods
        ثابت std::string func = nodep->traceType() == VTraceType::CHANGE ? "chg" : "full";
        منطقي emit_عرض = درست;
        string stype;
        إذا (nodep->dtypep()->basicp()->isDouble()) {
            stype = "Double";
            emit_عرض = خطأ;
        } وإلا إذا (nodep->isWide() || emitTraceIsScBv(nodep) || emitTraceIsScBigUint(nodep)) {
            stype = "WData";
        } وإلا إذا (nodep->isQuad()) {
            stype = "QData";
        } وإلا إذا (nodep->declp()->عرض_Min() > 16) {
            stype = "IData";
        } وإلا إذا (nodep->declp()->عرض_Min() > 8) {
            stype = "SData";
        } وإلا إذا (nodep->declp()->عرض_Min() > 1) {
            stype = "CData";
        } وإلا إذا (nodep->dtypep()->basicp()->isEvent()) {
            stype = "Event";
            emit_عرض = خطأ;
        } وإلا {
            stype = "Bit";
            emit_عرض = خطأ;
        }
        putns(nodep, "bufp->" + func + stype);

        ثابت uint32_t جابه_جایی = (arrayindex < 0) ? 0 : (arrayindex * nodep->declp()->عرض_واژگان());
        ثابت uint32_t کد = nodep->declp()->کد() + جابه_جایی;
        // Note: Both VTraceType::CHANGE and VTraceType::FULL use the 'full' methods
        puts("(oldp+");
        puts(cvt_مقصد_Str(کد - nodep->پایه_کد()));
        puts(",");
        ثابت VNumRange& arrayRange = nodep->declp()->arrayRange();
        إذا (arrayRange.ranged() && !arrayRange.ascending()) {
            arrayindex = arrayRange.elements() - 1 - arrayindex;
        }
        emit_Trace_مقدار(nodep, arrayindex);
        إذا (emit_عرض) puts("," + cvt_مقصد_Str(nodep->declp()->عرض_Min()));
        puts(");\n");
    }

    فراغ emit_Trace_مقدار(ثابت AstTraceInc* nodep, صحيح arrayindex) {
        إذا (AstVarRef* ثابت varrefp = VN_CAST(nodep->valuep(), VarRef)) {
            ثابت AstVar* ثابت varp = varrefp->varp();
            إذا (varp->isEvent()) puts("&");
            puts("(");
            إذا (emitTraceIsScBigUint(nodep)) {
                puts("(uint32_t*)");
            } وإلا إذا (emitTraceIsScBv(nodep)) {
                puts("VL_SC_BV_DATAP(");
            }
            iterateConst(varrefp);  // Put var name out
            emit_Trace_شاخص(nodep, arrayindex);
            إذا (varp->isSc()) puts(".read()");
            إذا (emitTraceIsScUint(nodep)) {
                puts(nodep->isQuad() ? ".to_uint64()" : ".to_uint()");
            } وإلا إذا (emitTraceIsScBigUint(nodep)) {
                puts(".get_raw()");
            } وإلا إذا (emitTraceIsScBv(nodep)) {
                puts(")");
            }
            puts(")");
        } وإلا {
            puts("(");
            iterateConst(nodep->valuep());
            emit_Trace_شاخص(nodep, arrayindex);
            puts(")");
        }
    }

    فراغ emit_Trace_شاخص(ثابت AstTraceInc* ثابت nodep, صحيح arrayindex) {
        // Tracing only supports 1D arrays
        إذا (nodep->declp()->arrayRange().ranged()) {
            إذا (arrayindex == -2) {
                puts("[i]");
            } وإلا إذا (arrayindex == -1) {
                puts("[0]");
            } وإلا {
                puts("[" + cvt_مقصد_Str(arrayindex) + "]");
            }
        }
    }

    // VISITORS
    استفاده EmitCFunc::بازدید;  // Suppress hidden overloaded virtual function warning
    فراغ بازدید(AstCFunc* nodep) override {
        إذا (!nodep->isTrace()) أرجع;
        إذا (nodep->slow() != m_slow) أرجع;

        إذا (تقسیم_Needed()) {
            // Splitting file, so using parallel build.
            v3Global.useParallelBuild(درست);
            // Close old file
            closeOutputFile();
            // Open a new file
            open_بعدی_Output_File();
        }

        EmitCFunc::بازدید(nodep);
    }
    فراغ بازدید(Ast_Trace_افزودن_پیشوند* nodep) override {
        putns(nodep, "VL_TRACE_PUSH_PREFIX(tracep, ");
        إذا (nodep->quoted_پیشوند()) {
            putsQuoted(VIdProtect::protect_واژگان_If(nodep->پیشوند(), nodep->protect()));
        } وإلا {
            puts(nodep->پیشوند());
        }
        puts(", VerilatedTracePrefixType::");
        puts(nodep->پیشوند_Type().ascii());
        puts(", " + std::مقصد_string(nodep->چپ()));
        puts(", " + std::مقصد_string(nodep->راست()));
        puts(");\n");
    }
    فراغ بازدید(Ast_Trace_برداشت_پیشوند* nodep) override {  //
        putns(nodep, "VL_TRACE_POP_PREFIX(tracep);\n");
    }
    فراغ بازدید(AstTraceDecl* nodep) override {
        ثابت صحيح enum_رقم = emitTraceDeclDType(nodep->dtypep());
        putns(nodep, "");
        إذا (nodep->arrayRange().ranged()) {
            puts("for (int i = 0; i < " + cvt_مقصد_Str(nodep->arrayRange().elements()) + "; ++i) {\n");
            emitTraceInitOne(nodep, enum_رقم);
            puts("\n}\n");
        } وإلا {
            emitTraceInitOne(nodep, enum_رقم);
            puts("\n");
        }
    }
    فراغ بازدید(AstTraceInc* nodep) override {
        إذا (nodep->declp()->arrayRange().ranged()) {
            // It traces faster if we unroll the loop
            لكل (صحيح i = 0; i < nodep->declp()->arrayRange().elements(); i++) {
                emitTraceChangeOne(nodep, i);
            }
        } وإلا {
            emitTraceChangeOne(nodep, -1);
        }
    }

    صریح EmitCTrace(منطقي slow)
        : m_slow{slow} {
        m_modp = v3Global.rootp()->بالاترین_Modulep();
        // Open output file
        open_بعدی_Output_File();
        // Emit functions
        لكل (Ast_گره* nodep = m_modp->stmtsp(); nodep; nodep = nodep->nextp()) {
            إذا (AstCFunc* ثابت funcp = VN_CAST(nodep, CFunc)) iterateConst(funcp);
        }
        // Close output file
        closeOutputFile();
        إذا (m_slow) m_emitTypesp->finalize();
    }
    ~EmitCTrace() override = افتراضي;

عام:
    ساكن std::وکتور<AstCFile*> اصلی(منطقي slow) VL_MT_STABLE {
        EmitCTrace emitCTrace{slow};
        std::وکتور<AstCFile*> cfileps = emitCTrace.getAndClearCfileps();
        إذا (slow) {
            لكل (AstCFile* ثابت cfilep : emitCTrace.m_emitTypesp->getAndClearCfileps()) {
                cfileps.emplace_پشت(cfilep);
            }
        }
        أرجع cfileps;
    }
};

//######################################################################
// Existing AstCFile emitter

صنف EmitCFile final : عام EmitCFunc {
    صریح EmitCFile(AstCFile* cfilep) {
        openOutputFile(cfilep, "Generated C++");
        iterateConst(cfilep->tblockp());
        closeOutputFile();
    }
    ~EmitCFile() override = افتراضي;

عام:
    ساكن فراغ اصلی(AstCFile* cfilep) VL_MT_STABLE {
        إذا (!cfilep->tblockp()) أرجع;
        EmitCFile{cfilep};
    }
};

//######################################################################
// EmitC class functions

فراغ V3EmitC::emitcImp() {
    UINFO(2, __FUNCTION__ << ":");
    std::list<std::وکتور<AstCFile*>> cfiles;
    {
        // Make parent module pointers available.
        ثابت EmitCParentModule emitCParentModule;
        V3ThreadScope threadScope;

        // Process each module in turn
        لكل (ثابت Ast_گره* nodep = v3Global.rootp()->modulesp(); nodep; nodep = nodep->nextp()) {
            إذا (VN_IS(nodep, Class)) استمر;  // Imped with ClassPackage
            ثابت Ast_گره_Module* ثابت modp = VN_AS(nodep, گره_Module);
            إذا (modp->isConstPool()) استمر;  // Emitted by V3EmitCConstPool
            cfiles.emplace_پشت();
            std::وکتور<AstCFile*>& slow = cfiles.پشت();
            threadScope.افزودن_به_صف([modp, &slow] { slow = EmitCImp::اصلی(modp, /* slow: */ درست); });
            cfiles.emplace_پشت();
            std::وکتور<AstCFile*>& fast = cfiles.پشت();
            threadScope.افزودن_به_صف([modp, &fast] { fast = EmitCImp::اصلی(modp, /* slow: */ خطأ); });
        }

        // Emit trace routines (currently they can only exist in the top module)
        إذا (v3Global.opt.trace() && !v3Global.opt.lintOnly()) {
            cfiles.emplace_پشت();
            std::وکتور<AstCFile*>& slow = cfiles.پشت();
            threadScope.افزودن_به_صف([&slow] { slow = EmitCTrace::اصلی(/* slow: */ درست); });
            cfiles.emplace_پشت();
            std::وکتور<AstCFile*>& fast = cfiles.پشت();
            threadScope.افزودن_به_صف([&fast] { fast = EmitCTrace::اصلی(/* slow: */ خطأ); });
        }
    }
    // Add files to netlist
    لكل (ثابت std::وکتور<AstCFile*>& cfileps : cfiles) {
        لكل (AstCFile* ثابت cfilep : cfileps) v3Global.rootp()->جمع_کن_Filesp(cfilep);
    }
}

فراغ V3EmitC::emitcFiles() {
    UINFO(2, __FUNCTION__ << ":");
    لكل (Ast_گره_File *filep = v3Global.rootp()->filesp(), *nextp; filep; filep = nextp) {
        nextp = VN_AS(filep->nextp(), گره_File);
        إذا (AstCFile* ثابت cfilep = VN_CAST(filep, CFile)) EmitCFile::اصلی(cfilep);
    }
}
