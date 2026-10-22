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

#شامل "V3PchAstMT.h"

#شامل "V3EmitC.h"
#شامل "V3EmitCFunc.h"
#شامل "V3ThreadPool.h"
#شامل "V3UniqueNames.h"

#شامل <algorithm>
#شامل <map>
#شامل <set>
#شامل <tuple>
#شامل <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Internal EmitC implementation

جماعت EmitCImp final : عوامی EmitCFunc {
    // MEMBERS
    // Base module (For non-classes, same as m_modp. For classes, it's the ClassPackage.)
    ثابت Ast_گرہ_Module* ثابت m_fileModp;
    ثابت منطقی m_slow;  // Creating __Slow file
    V3_Unique_نام m_unique_نام;  // Generates unique file names
    ثابت std::string m_file_بنیاد_Name = EmitCUtil::سابقہ_Name_Protect(m_fileModp);
    متن_Stmts m_ctorStmts;  // Statements constructing members in 'ctor'
    متن_Stmts m_dtorStmts;  // Statements destroying members in 'dtor'
    std::string m_ctor_بانٹو_Decl;  // Declaration of sub-function 'ctor' was split into, if any
    std::string m_dtor_بانٹو_Decl;  // Declaration of sub-function 'dtor' was split into, if any

    // METHODS
    خالی open_اگلا_Output_File(ثابت std::string& fileName) {
        open_New_Output_ماخذ_File(fileName, m_slow, جھوٹ, "Design implementation internals");
        emit_File_سرخی();
    }
    خالی emit_File_سرخی() {
        puts("// See " + EmitCUtil::چوٹی_Class_Name() + ".h for the primary calling header\n");
        puts("\n");
        puts("#include \"" + EmitCUtil::pchClassName() + ".h\"\n");
        emitSystemCSection(m_fileModp, VSystemCSectionType::IMP_HDR);
        // Need to emit new lazy declarations
        m_lazyDecls.reset();
    }

    خالی emitParamDefns(ثابت Ast_گرہ_Module* modp) {
        منطقی پہلا = سچ;
        برائے (ثابت Ast_گرہ* nodep = modp->stmtsp(); nodep; nodep = nodep->nextp()) {
            اگر (ثابت AstVar* ثابت varp = VN_CAST(nodep, Var)) {
                اگر (varp->isParam()) {
                    اگر (پہلا) {
                        puts("\n");
                        ثابت string modName = EmitCUtil::سابقہ_Name_Protect(modp);
                        putsDecoration(modp, "// Parameter definitions for " + modName + "\n");
                        پہلا = جھوٹ;
                    }
                    اگر (varp->dtypep()->isLiteralType()) putns(varp, "VL_CONSTINIT_CXX20 ");
                    putns(varp, "const ");
                    emitVarDefn(varp);
                    puts(";\n");
                }
            }
        }
        اگر (!پہلا) puts("\n");
    }
    // Add statements explicitly constructing, or destroying, a member AstVar
    ساکن خالی جوڑو_Var_Ctor_Dtor_Stmts(متن_Stmts& stmts, ثابت AstVar* varp, منطقی destroy) {
        ثابت std::string name = "vlSelf->" + varName(varp);

        // SystemC port arrays are C arrays, so construct them element by element. They are
        // default constructed (unnamed), as the elements of an array member would be.
        اگر (varp->isSc() && VN_IS(varp->dtypeSkipRefp(), UnpackArrayDType)) {
            std::سمتیہ<std::string> elems{name};
            برائے (ثابت AstUnpackArrayDType* adtypep
                 = VN_CAST(varp->dtypeSkipRefp(), UnpackArrayDType);
                 adtypep; adtypep = VN_CAST(adtypep->subDTypep()->skipRefp(), UnpackArrayDType)) {
                std::سمتیہ<std::string> subElems;
                برائے (ثابت std::string& elem : elems) {
                    برائے (صحیح i = 0; i < adtypep->elementsConst(); ++i) {
                        subElems.ڈالو_عقب(elem + "[" + std::مقصد_string(i) + "]");
                    }
                }
                elems = std::move(subElems);
            }
            اگر (destroy) std::الٹ(elems.begin(), elems.انتہا());
            برائے (ثابت std::string& e : elems) {
                stmts.جوڑو((destroy ? "VL_INPLACE_DESTRUCT(" : "VL_INPLACE_CONSTRUCT(") + e + ");");
            }
            واپس;
        }

        // Other variables are constructed whole, even if aggregates
        اگر (destroy) {
            stmts.جوڑو("VL_INPLACE_DESTRUCT(" + name + ");");
            واپس;
        }
        ثابت AstBasicDType* ثابت basicp = VN_CAST(varp->dtypeSkipRefp(), BasicDType);
        std::string دلائل;
        اگر (varp->isSc()) {
            // SystemC ports need their name
            دلائل = ", \"" + V3_برآمد_Formatter::quoteNameControls(varp->nameProtect()) + "\"";
        } ورنہ اگر (basicp && basicp->keyword().isMTaskState()) {
            // MTask state needs the number of upstream dependencies to wait for
            دلائل = ", " + VN_AS(varp->valuep(), Const)->عدد().emitC();
        } ورنہ اگر (basicp && basicp->isDelayScheduler()) {
            // Delay scheduler needs the context pointer
            دلائل = ", *vlSelf->vlSymsp->_vm_contextp__";
        }
        stmts.جوڑو("VL_INPLACE_CONSTRUCT(" + name + دلائل + ");");
    }
    // Gather the statements constructing/destroying members in anonymous unions, and split
    // them into sub-functions if needed. As this emits files, no output file can be open.
    خالی createCtorDtorStmts(ثابت Ast_گرہ_Module* modp) {
        // Construct member variables
        برائے (ثابت Ast_گرہ* nodep = modp->stmtsp(); nodep; nodep = nodep->nextp()) {
            ثابت AstVar* ثابت varp = VN_CAST(nodep, Var);
            اگر (varp && isExplicitlyConstructed(varp)) {
                جوڑو_Var_Ctor_Dtor_Stmts(m_ctorStmts, varp, جھوٹ);
            }
        }

        // Destroy in reverse order
        ثابت Ast_گرہ* ثابت lastp = modp->stmtsp() ? modp->stmtsp()->lastp() : خالی_اشاریہ;
        برائے (ثابت Ast_گرہ* nodep = lastp; nodep; nodep = nodep->prevp()) {
            ثابت AstVar* ثابت varp = VN_CAST(nodep, Var);
            اگر (varp && isExplicitlyConstructed(varp)) {
                جوڑو_Var_Ctor_Dtor_Stmts(m_dtorStmts, varp, سچ);
            }
        }

        // Split into sub-functions if needed
        ثابت std::string modName = EmitCUtil::سابقہ_Name_Protect(modp);
        std::tie(m_ctor_بانٹو_Decl, m_dtor_بانٹو_Decl) = emit_بانٹو_Ctor_Dtor(
            m_ctorStmts, m_dtorStmts, 0, modName, modName + "* vlSelf", "vlSelf", جھوٹ,
            "Design implementation internals", [یہ]() { emit_File_سرخی(); });
    }
    خالی emitCtorDtorImp(ثابت Ast_گرہ_Module* modp) {
        ثابت std::string modName = EmitCUtil::سابقہ_Name_Protect(modp);

        // ctor
        اگر (modp->ctorVarReset()) {
            m_lazyDecls.emit("\nvoid " + modName + "__", protect("_ctor_var_reset"),
                             "(" + modName + "* vlSelf);\n");
        }

        اگر (!m_ctor_بانٹو_Decl.خالی_خانے()) puts("\n" + m_ctor_بانٹو_Decl + "\n");
        puts("\nvoid " + modName + "__ctor(" + modName + "* vlSelf, " + EmitCUtil::symClassName()
             + "* symsp, const char* namep) {\n");
        puts("vlSelf->vlSymsp = symsp;\n");
        اگر (modp->is_چوٹی()) {
            puts("vlSelf->vlNamep = strdup(namep);\n");
        } ورنہ {
            puts("vlSelf->vlNamep = strdup(Verilated::catName(symsp->name(), namep));\n");
        }
        putsDecoration(modp, "// Construct members\n");
        برائے (ثابت متن_Stmts::Stmt& stmt : m_ctorStmts) puts(stmt.m_متن + "\n");

        putsDecoration(modp, "// Reset structure values\n");
        اگر (modp->ctorVarReset()) {
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
        اگر (!m_dtor_بانٹو_Decl.خالی_خانے()) puts("\n" + m_dtor_بانٹو_Decl + "\n");
        puts("\nvoid " + modName + "__dtor(" + modName + "* vlSelf) {\n");
        putsDecoration(modp, "// Destroy members\n");
        برائے (ثابت متن_Stmts::Stmt& stmt : m_dtorStmts) puts(stmt.m_متن + "\n");
        puts("VL_DO_DANGLING(std::free(const_cast<char*>(vlSelf->vlNamep)), "
             "vlSelf->vlNamep);\n");
        puts("}\n");

        // C++ destructor
        puts("\n" + modName + "::~" + modName + "() {\n");
        emitSystemCSection(modp, VSystemCSectionType::DTOR);
        puts(modName + "__dtor(this);\n");
        puts("}\n");
    }

    خالی emitConfigureImp(ثابت Ast_گرہ_Module* modp) {
        ثابت string modName = EmitCUtil::سابقہ_Name_Protect(modp);

        اگر (v3Global.opt.coverage()) {
            puts("\n");
            m_lazyDecls.emit("void " + modName + "__", protect("_configure_coverage"),
                             "(" + modName + "* vlSelf, bool first);");
        }

        اگر (v3Global.opt.coverage()) {
            puts("\nvoid " + modName + "::" + protect("__Vconfigure") + "(bool first) {\n");
            puts("(void)first;  // Prevent unused variable warning\n");
            puts(modName + "__" + protect("_configure_coverage") + "(this, first);\n");
            puts("}\n");
        }
    }
    خالی emitCoverageImp() {
        // Rather than putting out VL_COVER_INSERT calls directly, we do it via this
        // function. This gets around gcc slowness constructing all of the template
        // arguments.
        اگر (v3Global.opt.coverage()) {
            puts("\n// Coverage\n");
            puts("void " + EmitCUtil::سابقہ_Name_Protect(m_modp) + "::__vlCoverInsert(");
            puts(v3Global.opt.دھاگے() > 1 ? "std::atomic<uint32_t>" : "uint32_t");
            puts("* countp, bool enable, bool localCounter, const char* filenamep, int lineno, "
                 "int column,\n");
            puts("const char* hierp, const char* pagep, const char* commentp, const char* "
                 "linescovp,\n");
            puts("const char* fsmVarp, const char* fsmFromp, const char* fsmTop, const char* "
                 "fsmTagp) {\n");
            اگر (v3Global.opt.دھاگے() > 1) {
                puts("assert(sizeof(uint32_t) == sizeof(std::atomic<uint32_t>));\n");
                puts("uint32_t* count32p = reinterpret_cast<uint32_t*>(countp);\n");
            } ورنہ {
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
        اگر (v3Global.opt.coverageToggle()) {
            puts("\n// Toggle Coverage\n");
            puts("void " + EmitCUtil::سابقہ_Name_Protect(m_modp) + "::__vlCoverToggleInsert(");
            puts("int begin, int end, bool ranged, ");
            puts(v3Global.opt.دھاگے() > 1 ? "std::atomic<uint32_t>" : "uint32_t");
            puts("* countp, bool enable, bool localCounter, const char* filenamep, int lineno, "
                 "int column,\n");
            puts("const char* hierp, const char* pagep, const char* commentp) {\n");
            اگر (v3Global.opt.دھاگے() > 1) {
                puts("assert(sizeof(uint32_t) == sizeof(std::atomic<uint32_t>));\n");
            }
            puts("int step = (end >= begin) ? 1 : -1;\n");
            // range is inclusive
            puts("for (int i = begin; i != end + step; i += step) {\n");
            puts("for (int j = 0; j < 2; j++) {\n");
            اگر (v3Global.opt.دھاگے() > 1) {
                puts("uint32_t* count32p = reinterpret_cast<uint32_t*>(countp);\n");
            } ورنہ {
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
                                       //خالی_خانے
            puts("++countp;\n");
            puts("}\n");
            puts("}\n");
            puts("}\n");
        }
    }
    خالی emitSavableImp(ثابت Ast_گرہ_Module* modp) {
        اگر (v3Global.opt.savable()) {
            puts("\n// Savable\n");
            برائے (صحیح de = 0; de < 2; ++de) {
                ثابت string classname = de ? "VerilatedDeserialize" : "VerilatedSerialize";
                ثابت string funcname = de ? "__Vdeserialize" : "__Vserialize";
                ثابت string op = de ? ">>" : "<<";
                // NOLINTNEXTLINE(performance-inefficient-string-concatenation)
                putns(modp, "void " + EmitCUtil::سابقہ_Name_Protect(modp) + "::" + protect(funcname)
                                + "(" + classname + "& os) {\n");
                // Place a computed checksum to ensure proper structure save/restore formatting
                // OK if this hash includes some things we won't dump, since
                // just looking for loading the wrong model
                VHashSha512 hash;
                برائے (Ast_گرہ* nodep = modp->stmtsp(); nodep; nodep = nodep->nextp()) {
                    اگر (ثابت AstVar* ثابت varp = VN_CAST(nodep, Var)) {
                        hash.داخل_کرو(varp->name());
                        hash.داخل_کرو(varp->dtypep()->چوڑائی());
                    }
                }
                ofp()->لکھو("uint64_t __Vcheckval = 0x%" PRIx64 "ULL;\n",
                              static_cast<uint64_t>(hash.digestUInt64()));
                اگر (de) {
                    puts("os.readAssert(__Vcheckval);\n");
                } ورنہ {
                    puts("os << __Vcheckval;\n");
                }

                // Save context
                // If multiple models save the same context we'll save it multiple
                // times, but is harmless, and doing it otherwise would break
                // backwards compatibility.
                puts("os " + op + " vlSymsp->_vm_contextp__;\n");

                // Save all members
                برائے (Ast_گرہ* nodep = modp->stmtsp(); nodep; nodep = nodep->nextp()) {
                    اگر (ثابت AstVar* ثابت varp = VN_CAST(nodep, Var)) {
                        اگر (varp->isIO() && modp->is_چوٹی() && optSystemC()) {
                            // System C top I/O doesn't need loading, as the
                            // lower level subinst code does it.
                        } ورنہ اگر (varp->isParam()) {
                        } ورنہ اگر (VN_IS(varp->dtypep(), NBACommit_قطار_DType)) {
                        } ورنہ {
                            صحیح vects = 0;
                            Ast_گرہ_DType* elementp = varp->dtypeSkipRefp();
                            برائے (AstUnpackArrayDType* arrayp = VN_CAST(elementp, UnpackArrayDType);
                                 arrayp; arrayp = VN_CAST(elementp, UnpackArrayDType)) {
                                ثابت صحیح vecnum = vects++;
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
                            اگر (basicp && basicp->keyword().isMTaskState()) جاری;
                            // Want to detect types that are represented as arrays
                            // (i.e. packed types of more than 64 bits).
                            اگر (elementp->isWide()
                                && !(basicp && basicp->keyword() == VBasicDTypeKwd::STRING)) {
                                ثابت صحیح vecnum = vects++;
                                ثابت string ivar = "__Vi"s + cvt_مقصد_Str(vecnum);
                                puts("for (int __Vi" + cvt_مقصد_Str(vecnum) + " = " + cvt_مقصد_Str(0));
                                puts("; " + ivar + " < " + cvt_مقصد_Str(elementp->چوڑائی_الفاظ()));
                                puts("; ++" + ivar + ") {\n");
                            }
                            putns(varp, "os" + op + varName(varp));
                            برائے (صحیح v = 0; v < vects; ++v) puts("[__Vi" + cvt_مقصد_Str(v) + "]");
                            puts(";\n");
                            برائے (صحیح v = 0; v < vects; ++v) puts("}\n");
                        }
                    }
                }

                puts("}\n");
            }
        }
    }
    // Predicate to check if we actually need to emit anything into the common implementation file.
    // Used to avoid creating empty output files.
    منطقی hasCommonImp(ثابت Ast_گرہ_Module* modp) ثابت {
        // Nothing to emit if no module!
        اگر (!modp) واپس جھوٹ;
        // We always need the slow file
        اگر (m_slow) واپس سچ;
        // The fast file is only required when we have `systemc_implementation nodes
        برائے (ثابت AstSystemCSection* scSectionp = modp->scSectionsp(); scSectionp;
             scSectionp = VN_AS(scSectionp->nextp(), SystemCSection)) {
            اگر (scSectionp->sectionType() == VSystemCSectionType::IMP) واپس سچ;
        }
        واپس جھوٹ;
    }
    // Actually emit common implementation contents for given AstNodeModule
    خالی doCommonImp(ثابت Ast_گرہ_Module* modp) {
        اگر (m_slow) {
            اگر (!VN_IS(modp, Class)) {
                emitParamDefns(modp);
                emitCtorDtorImp(modp);
                emitConfigureImp(modp);
                emitCoverageImp();
            }
            emitSavableImp(modp);
        } ورنہ {
            // From `systemc_implementation
            emitSystemCSection(modp, VSystemCSectionType::IMP);
        }
    }
    خالی emitCommonImp(ثابت Ast_گرہ_Module* modp) {
        ثابت AstClass* ثابت classp
            = VN_IS(modp, ClassPackage) ? VN_AS(modp, ClassPackage)->classp() : خالی_اشاریہ;

        اگر (hasCommonImp(modp) || hasCommonImp(classp)) {
            // Create ctor/dtor statements. If these need splitting, they will be emitted
            // during construction so do this before opening the output file.
            اگر (m_slow && !VN_IS(modp, Class)) createCtorDtorStmts(modp);

            open_اگلا_Output_File(m_file_بنیاد_Name);

            doCommonImp(modp);
            اگر (classp) {
                VL_RESTORER(m_modp);
                m_modp = classp;
                doCommonImp(classp);
            }

            closeOutputFile();
        }
    }
    خالی emitCFuncImp(ثابت Ast_گرہ_Module* modp) {
        // Functions to be emitted here
        std::سمتیہ<AstCFunc*> funcps;

        ثابت خودکار gather = [یہ, &funcps](ثابت Ast_گرہ_Module* modp) {
            برائے (Ast_گرہ* nodep = modp->stmtsp(); nodep; nodep = nodep->nextp()) {
                AstCFunc* ثابت funcp = VN_CAST(nodep, CFunc);
                اگر (!funcp) جاری;
                // TRACE_* and DPI handled elsewhere
                اگر (funcp->isTrace()) جاری;
                اگر (funcp->dpiImportPrototype()) جاری;
                اگر (funcp->dpiExportDispatcher()) جاری;
                اگر (funcp->slow() != m_slow) جاری;
                funcps.ڈالو_عقب(funcp);
            }
        };

        gather(modp);
        VL_RESTORER(m_classOrPackage);
        اگر (ثابت AstClassPackage* ثابت packagep = VN_CAST(modp, ClassPackage)) {
            m_classOrPackage = packagep;
            gather(packagep->classp());
        }

        // Do not create empty files
        اگر (funcps.خالی_خانے()) واپس;

        // Open output file
        open_اگلا_Output_File(m_unique_نام.get(m_file_بنیاد_Name));
        // Emit all functions
        برائے (AstCFunc* ثابت funcp : funcps) {
            VL_RESTORER(m_modp);
            m_modp = EmitCParentModule::get(funcp);
            iterateConst(funcp);
        }
        // Close output file
        closeOutputFile();
    }

    // VISITORS
    خالی دورہ(AstCFunc* nodep) override {
        اگر (بانٹو_Needed()) {
            // Splitting file, so using parallel build.
            v3Global.useParallelBuild(سچ);
            // Close old file
            closeOutputFile();
            // Open a new file
            open_اگلا_Output_File(m_unique_نام.get(m_file_بنیاد_Name));
        }

        EmitCFunc::دورہ(nodep);
    }

    صریح EmitCImp(ثابت Ast_گرہ_Module* modp, منطقی slow)
        : m_fileModp{modp}
        , m_slow{slow} {
        UINFO(5, "  Emitting implementation of " << EmitCUtil::سابقہ_Name_Protect(modp));

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
    ~EmitCImp() override = طےشدہ;

عوامی:
    ساکن std::سمتیہ<AstCFile*> مین(ثابت Ast_گرہ_Module* modp, منطقی slow) VL_MT_STABLE {
        EmitCImp emitCImp{modp, slow};
        واپس emitCImp.getAndClearCfileps();
    }
};

//######################################################################
// Tracing routines

// Trace type descriptors go in a different file as it needs to be written in
// parallel with the actual trace function source files
جماعت EmitCTraceTypes final : عوامی EmitCFunc {
    // NODE STATE/TYPES
    // None allowed to support threaded emitting

    // STATE
    صحیح m_enum_عدد = 0;  // Enumeration number (whole netlist)
    std::unordered_map<Ast_گرہ*, صحیح> m_enum_عدد_Map;  // EnumDType to enumeration number
    صحیح m_traceTypeSubs = 0;  // Number of trace type declaration sub-functions
    V3_Unique_نام m_unique_نام;  // Generates unique file names
    ثابت std::string m_file_بنیاد_Name = EmitCUtil::چوٹی_Class_Name() + "_" + protect("_TraceDecls");
    // This one uses CSplitTrace for file splitting, which is incorrect but historically accurates
    ثابت سائز_t m_بانٹو_حد_بندی = v3Global.opt.output_بانٹو_CTrace()
                                    ? static_cast<سائز_t>(v3Global.opt.output_بانٹو_CTrace())
                                    : std::numeric_limits<سائز_t>::max();

    خالی open_اگلا_Output_File() {
        open_New_Output_ماخذ_File(m_unique_نام.get(m_file_بنیاد_Name), سچ, سچ,
                                "Tracing declarations");
        puts("\n");
        برائے (ثابت std::string& بنیاد : v3Global.opt.trace_ماخذ_Langs()) {
            puts("#include \"" + بنیاد + ".h\"\n");
        }
        puts("\n");
        puts("\nvoid " + EmitCUtil::سابقہ_Name_Protect(m_modp) + "__"
             + protect("traceDeclTypesSub" + std::مقصد_string(m_traceTypeSubs++)) + "("
             + v3Global.opt.trace_Class_بنیاد() + "* tracep) {\n");
    }

عوامی:
    // METHODS
    صحیح get_Enum_Map_عدد(AstEnumDType* nodep) {
        صحیح& enumNumr = m_enum_عدد_Map[nodep];
        اگر (!enumNumr) {
            اگر (بانٹو_Needed(m_بانٹو_حد_بندی)) {
                // Splitting file, so using parallel build.
                v3Global.useParallelBuild(سچ);
                puts("}\n");
                closeOutputFile();
                open_اگلا_Output_File();
            }

            enumNumr = ++m_enum_عدد;
            صحیح nvals = 0;
            puts("{\n");
            putns(nodep, "const char* " + protect("__VenumItemNames") + "[]\n");
            puts("= {");
            برائے (Ast_Enum_شے* itemp = nodep->itemsp(); itemp;
                 itemp = VN_AS(itemp->nextp(), Enum_شے)) {
                اگر (++nvals > 1) puts(", ");
                putbs("\"" + itemp->prettyName() + "\"");
            }
            puts("};\n");
            nvals = 0;
            puts("const char* " + protect("__VenumItemValues") + "[]\n");
            puts("= {");
            برائے (Ast_Enum_شے* itemp = nodep->itemsp(); itemp;
                 itemp = VN_AS(itemp->nextp(), Enum_شے)) {
                AstConst* ثابت constp = VN_AS(itemp->valuep(), Const);
                اگر (++nvals > 1) puts(", ");
                putbs("\"" + constp->عدد().displayed(nodep, "%0b") + "\"");
            }
            puts("};\n");
            puts("tracep->declDTypeEnum(" + std::مقصد_string(enumNumr) + ", \"" + nodep->prettyName()
                 + "\", " + std::مقصد_string(nvals) + ", " + std::مقصد_string(nodep->چوڑائی_Min()) + ", "
                 + protect("__VenumItemNames") + ", " + protect("__VenumItemValues") + ");\n");
            puts("}\n");
            بانٹو_سائز_Inc(Ast_گرہ::INSTR_گنتی_CALL);
        }
        واپس enumNumr;
    }

    // Close output file
    خالی finalize() {
        // Close function definition
        puts("}\n");

        ثابت std::string modName = EmitCUtil::سابقہ_Name_Protect(m_modp);
        ثابت std::string دلائل = v3Global.opt.trace_Class_بنیاد() + "* tracep";

        // Forward declarations for subs in other files
        برائے (صحیح i = 0; i < m_traceTypeSubs - 1; ++i) {
            puts("void " + modName + "__" + protect("traceDeclTypesSub" + std::مقصد_string(i)) + "("
                 + دلائل + ");\n");
        }

        // Create top level trace_decl_types function and call each sub-function
        puts("\nvoid " + modName + "__" + protect("trace_decl_types") + "(" + دلائل + ") {\n");
        برائے (صحیح i = 0; i < m_traceTypeSubs; ++i) {
            puts(modName + "__" + protect("traceDeclTypesSub" + std::مقصد_string(i))
                 + "(tracep);\n");
        }
        puts("}\n");

        closeOutputFile();
    }

    EmitCTraceTypes() {
        m_modp = v3Global.rootp()->چوٹی_Modulep();
        open_اگلا_Output_File();
    }
    ~EmitCTraceTypes() override = طےشدہ;
};

جماعت EmitCTrace final : عوامی EmitCFunc {
    // NODE STATE/TYPES
    // None allowed to support threaded emitting

    // MEMBERS
    ثابت منطقی m_slow;  // Making slow file
    ثابت std::unique_ptr<EmitCTraceTypes> m_emitTypesp{m_slow ? نیا EmitCTraceTypes{} : خالی_اشاریہ};
    V3_Unique_نام m_unique_نام;  // Generates unique file names
    ثابت std::string m_file_بنیاد_Name = EmitCUtil::چوٹی_Class_Name() + "_" + protect("_Trace");

    // METHODS
    خالی open_اگلا_Output_File() {
        open_New_Output_ماخذ_File(m_unique_نام.get(m_file_بنیاد_Name), m_slow, سچ,
                                "Tracing implementation internals");
        puts("\n");
        برائے (ثابت std::string& بنیاد : v3Global.opt.trace_ماخذ_Langs()) {
            puts("#include \"" + بنیاد + ".h\"\n");
        }
        puts("#include \"" + EmitCUtil::symClassName() + ".h\"\n");
        puts("\n");
        // Need to emit new lazy declarations
        m_lazyDecls.reset();
    }

    منطقی emitTraceIsScBv(ثابت AstTraceInc* nodep) {
        ثابت AstVarRef* ثابت varrefp = VN_CAST(nodep->declp()->valuep(), VarRef);
        اگر (!varrefp) واپس جھوٹ;
        ثابت AstVar* ثابت varp = varrefp->varp();
        واپس varp->isSc() && varp->isScBv();
    }

    منطقی emitTraceIsScBigUint(ثابت AstTraceInc* nodep) {
        ثابت AstVarRef* ثابت varrefp = VN_CAST(nodep->declp()->valuep(), VarRef);
        اگر (!varrefp) واپس جھوٹ;
        ثابت AstVar* ثابت varp = varrefp->varp();
        واپس varp->isSc() && varp->isScBigUint();
    }

    منطقی emitTraceIsScUint(ثابت AstTraceInc* nodep) {
        ثابت AstVarRef* ثابت varrefp = VN_CAST(nodep->declp()->valuep(), VarRef);
        اگر (!varrefp) واپس جھوٹ;
        ثابت AstVar* ثابت varp = varrefp->varp();
        واپس varp->isSc() && (varp->isScUint() || varp->isScUintBool());
    }

    خالی emitTraceInitOne(ثابت AstTraceDecl* nodep, صحیح enum_عدد) {
        std::string direction;
        direction = nodep->declDirection().traceSigDirection();

        AstCCall* ثابت callp = nodep->dtypeCallp();
        اگر (callp) {
            callp->argTypes(callp->argTypes() + ", " + cvt_مقصد_Str(nodep->fidx()) + ", c+"
                            + cvt_مقصد_Str(nodep->رمز()) + ", " + direction);
            واپس;
        }

        اگر (nodep->dtypep()->basicp()->isDouble()) {
            puts("VL_TRACE_DECL_DOUBLE");
        } ورنہ اگر (nodep->isWide()) {
            puts("VL_TRACE_DECL_WIDE");
        } ورنہ اگر (nodep->isQuad()) {
            puts("VL_TRACE_DECL_QUAD");
        } ورنہ اگر (nodep->ثنائی_ہندسہ_Range().ranged()) {
            puts("VL_TRACE_DECL_BUS");
        } ورنہ اگر (nodep->dtypep()->basicp()->isEvent()) {
            puts("VL_TRACE_DECL_EVENT");
        } ورنہ {
            puts("VL_TRACE_DECL_BIT");
        }

        اگر (nodep->arrayRange().ranged()) {
            puts("_ARRAY(tracep");
        } ورنہ {
            puts("(tracep");
        }

        //رمز
        puts(",c+" + cvt_مقصد_Str(nodep->رمز()));
        اگر (nodep->arrayRange().ranged()) puts("+i*" + cvt_مقصد_Str(nodep->چوڑائی_الفاظ()));

        // Function index
        puts(",");
        اگر (nodep->اندر_Dtype_Func()) {
            puts("fidx");
        } ورنہ {
            puts(cvt_مقصد_Str(nodep->fidx()));
        }

        // Name
        puts(",");
        putsQuoted(VIdProtect::protect_الفاظ_If(nodep->showname(), nodep->protect()));

        // Enum number
        puts("," + cvt_مقصد_Str(enum_عدد));

        // Direction
        اگر (nodep->اندر_Dtype_Func()) {
            puts(", direction");
        } ورنہ {
            puts(", " + direction);
        }

        //قسم
        puts(", VerilatedTraceSigKind::");
        puts(nodep->varType().trace_Sig_قسم());

        // Type
        puts(", VerilatedTraceSigType::");
        UASSERT_OBJ(nodep->dtypep()->basicp()->keyword() != VBasicDTypeKwd::UNKNOWN, nodep,
                    "DType is unknown");
        puts(nodep->dtypep()->basicp()->keyword().traceSigType());

        // Array range
        اگر (nodep->arrayRange().ranged()) {
            اگر (nodep->arrayRange().ascending()) {
                puts(", (i + " + std::مقصد_string(nodep->arrayRange().lo()) + ")");
            } ورنہ {
                puts(", (" + std::مقصد_string(nodep->arrayRange().hi()) + " - i)");
            }
        }

        // Bit range
        اگر (!nodep->dtypep()->basicp()->isDouble() && nodep->ثنائی_ہندسہ_Range().ranged()) {
            puts(", " + cvt_مقصد_Str(nodep->ثنائی_ہندسہ_Range().بائیں()) + ","
                 + cvt_مقصد_Str(nodep->ثنائی_ہندسہ_Range().دائیں()));
        }

        //
        puts(");");
    }

    صحیح emitTraceDeclDType(Ast_گرہ_DType* nodep) {
        // Return enum number or -1 for none
        اگر (v3Global.opt.traceEnabledFst()) {
            // Skip over refs-to-refs, but stop before final ref so can get data type name
            // Alternatively back in V3Width we could push enum names from upper typedefs
            اگر (AstEnumDType* ثابت enump = VN_CAST(nodep->skip_Ref_مقصد_Enump(), EnumDType)) {
                واپس m_emitTypesp->get_Enum_Map_عدد(enump);
            }
        }
        واپس -1;
    }

    خالی emitTraceChangeOne(AstTraceInc* nodep, صحیح arrayindex) {
        // Note: Both VTraceType::CHANGE and VTraceType::FULL use the 'full' methods
        ثابت std::string func = nodep->traceType() == VTraceType::CHANGE ? "chg" : "full";
        منطقی emit_چوڑائی = سچ;
        string stype;
        اگر (nodep->dtypep()->basicp()->isDouble()) {
            stype = "Double";
            emit_چوڑائی = جھوٹ;
        } ورنہ اگر (nodep->isWide() || emitTraceIsScBv(nodep) || emitTraceIsScBigUint(nodep)) {
            stype = "WData";
        } ورنہ اگر (nodep->isQuad()) {
            stype = "QData";
        } ورنہ اگر (nodep->declp()->چوڑائی_Min() > 16) {
            stype = "IData";
        } ورنہ اگر (nodep->declp()->چوڑائی_Min() > 8) {
            stype = "SData";
        } ورنہ اگر (nodep->declp()->چوڑائی_Min() > 1) {
            stype = "CData";
        } ورنہ اگر (nodep->dtypep()->basicp()->isEvent()) {
            stype = "Event";
            emit_چوڑائی = جھوٹ;
        } ورنہ {
            stype = "Bit";
            emit_چوڑائی = جھوٹ;
        }
        putns(nodep, "bufp->" + func + stype);

        ثابت uint32_t انحراف = (arrayindex < 0) ? 0 : (arrayindex * nodep->declp()->چوڑائی_الفاظ());
        ثابت uint32_t رمز = nodep->declp()->رمز() + انحراف;
        // Note: Both VTraceType::CHANGE and VTraceType::FULL use the 'full' methods
        puts("(oldp+");
        puts(cvt_مقصد_Str(رمز - nodep->بنیاد_رمز()));
        puts(",");
        ثابت VNumRange& arrayRange = nodep->declp()->arrayRange();
        اگر (arrayRange.ranged() && !arrayRange.ascending()) {
            arrayindex = arrayRange.elements() - 1 - arrayindex;
        }
        emit_Trace_قدر(nodep, arrayindex);
        اگر (emit_چوڑائی) puts("," + cvt_مقصد_Str(nodep->declp()->چوڑائی_Min()));
        puts(");\n");
    }

    خالی emit_Trace_قدر(ثابت AstTraceInc* nodep, صحیح arrayindex) {
        اگر (AstVarRef* ثابت varrefp = VN_CAST(nodep->valuep(), VarRef)) {
            ثابت AstVar* ثابت varp = varrefp->varp();
            اگر (varp->isEvent()) puts("&");
            puts("(");
            اگر (emitTraceIsScBigUint(nodep)) {
                puts("(uint32_t*)");
            } ورنہ اگر (emitTraceIsScBv(nodep)) {
                puts("VL_SC_BV_DATAP(");
            }
            iterateConst(varrefp);  // Put var name out
            emit_Trace_اشاریہ(nodep, arrayindex);
            اگر (varp->isSc()) puts(".read()");
            اگر (emitTraceIsScUint(nodep)) {
                puts(nodep->isQuad() ? ".to_uint64()" : ".to_uint()");
            } ورنہ اگر (emitTraceIsScBigUint(nodep)) {
                puts(".get_raw()");
            } ورنہ اگر (emitTraceIsScBv(nodep)) {
                puts(")");
            }
            puts(")");
        } ورنہ {
            puts("(");
            iterateConst(nodep->valuep());
            emit_Trace_اشاریہ(nodep, arrayindex);
            puts(")");
        }
    }

    خالی emit_Trace_اشاریہ(ثابت AstTraceInc* ثابت nodep, صحیح arrayindex) {
        // Tracing only supports 1D arrays
        اگر (nodep->declp()->arrayRange().ranged()) {
            اگر (arrayindex == -2) {
                puts("[i]");
            } ورنہ اگر (arrayindex == -1) {
                puts("[0]");
            } ورنہ {
                puts("[" + cvt_مقصد_Str(arrayindex) + "]");
            }
        }
    }

    // VISITORS
    استعمال EmitCFunc::دورہ;  // Suppress hidden overloaded virtual function warning
    خالی دورہ(AstCFunc* nodep) override {
        اگر (!nodep->isTrace()) واپس;
        اگر (nodep->slow() != m_slow) واپس;

        اگر (بانٹو_Needed()) {
            // Splitting file, so using parallel build.
            v3Global.useParallelBuild(سچ);
            // Close old file
            closeOutputFile();
            // Open a new file
            open_اگلا_Output_File();
        }

        EmitCFunc::دورہ(nodep);
    }
    خالی دورہ(Ast_Trace_ڈالو_سابقہ* nodep) override {
        putns(nodep, "VL_TRACE_PUSH_PREFIX(tracep, ");
        اگر (nodep->quoted_سابقہ()) {
            putsQuoted(VIdProtect::protect_الفاظ_If(nodep->سابقہ(), nodep->protect()));
        } ورنہ {
            puts(nodep->سابقہ());
        }
        puts(", VerilatedTracePrefixType::");
        puts(nodep->سابقہ_Type().ascii());
        puts(", " + std::مقصد_string(nodep->بائیں()));
        puts(", " + std::مقصد_string(nodep->دائیں()));
        puts(");\n");
    }
    خالی دورہ(Ast_Trace_نکالو_سابقہ* nodep) override {  //
        putns(nodep, "VL_TRACE_POP_PREFIX(tracep);\n");
    }
    خالی دورہ(AstTraceDecl* nodep) override {
        ثابت صحیح enum_عدد = emitTraceDeclDType(nodep->dtypep());
        putns(nodep, "");
        اگر (nodep->arrayRange().ranged()) {
            puts("for (int i = 0; i < " + cvt_مقصد_Str(nodep->arrayRange().elements()) + "; ++i) {\n");
            emitTraceInitOne(nodep, enum_عدد);
            puts("\n}\n");
        } ورنہ {
            emitTraceInitOne(nodep, enum_عدد);
            puts("\n");
        }
    }
    خالی دورہ(AstTraceInc* nodep) override {
        اگر (nodep->declp()->arrayRange().ranged()) {
            // It traces faster if we unroll the loop
            برائے (صحیح i = 0; i < nodep->declp()->arrayRange().elements(); i++) {
                emitTraceChangeOne(nodep, i);
            }
        } ورنہ {
            emitTraceChangeOne(nodep, -1);
        }
    }

    صریح EmitCTrace(منطقی slow)
        : m_slow{slow} {
        m_modp = v3Global.rootp()->چوٹی_Modulep();
        // Open output file
        open_اگلا_Output_File();
        // Emit functions
        برائے (Ast_گرہ* nodep = m_modp->stmtsp(); nodep; nodep = nodep->nextp()) {
            اگر (AstCFunc* ثابت funcp = VN_CAST(nodep, CFunc)) iterateConst(funcp);
        }
        // Close output file
        closeOutputFile();
        اگر (m_slow) m_emitTypesp->finalize();
    }
    ~EmitCTrace() override = طےشدہ;

عوامی:
    ساکن std::سمتیہ<AstCFile*> مین(منطقی slow) VL_MT_STABLE {
        EmitCTrace emitCTrace{slow};
        std::سمتیہ<AstCFile*> cfileps = emitCTrace.getAndClearCfileps();
        اگر (slow) {
            برائے (AstCFile* ثابت cfilep : emitCTrace.m_emitTypesp->getAndClearCfileps()) {
                cfileps.emplace_عقب(cfilep);
            }
        }
        واپس cfileps;
    }
};

//######################################################################
// Existing AstCFile emitter

جماعت EmitCFile final : عوامی EmitCFunc {
    صریح EmitCFile(AstCFile* cfilep) {
        openOutputFile(cfilep, "Generated C++");
        iterateConst(cfilep->tblockp());
        closeOutputFile();
    }
    ~EmitCFile() override = طےشدہ;

عوامی:
    ساکن خالی مین(AstCFile* cfilep) VL_MT_STABLE {
        اگر (!cfilep->tblockp()) واپس;
        EmitCFile{cfilep};
    }
};

//######################################################################
// EmitC class functions

خالی V3EmitC::emitcImp() {
    UINFO(2, __FUNCTION__ << ":");
    std::list<std::سمتیہ<AstCFile*>> cfiles;
    {
        // Make parent module pointers available.
        ثابت EmitCParentModule emitCParentModule;
        V3ThreadScope threadScope;

        // Process each module in turn
        برائے (ثابت Ast_گرہ* nodep = v3Global.rootp()->modulesp(); nodep; nodep = nodep->nextp()) {
            اگر (VN_IS(nodep, Class)) جاری;  // Imped with ClassPackage
            ثابت Ast_گرہ_Module* ثابت modp = VN_AS(nodep, گرہ_Module);
            اگر (modp->isConstPool()) جاری;  // Emitted by V3EmitCConstPool
            cfiles.emplace_عقب();
            std::سمتیہ<AstCFile*>& slow = cfiles.عقب();
            threadScope.قطار_میں_ڈالو([modp, &slow] { slow = EmitCImp::مین(modp, /* slow: */ سچ); });
            cfiles.emplace_عقب();
            std::سمتیہ<AstCFile*>& fast = cfiles.عقب();
            threadScope.قطار_میں_ڈالو([modp, &fast] { fast = EmitCImp::مین(modp, /* slow: */ جھوٹ); });
        }

        // Emit trace routines (currently they can only exist in the top module)
        اگر (v3Global.opt.trace() && !v3Global.opt.lintOnly()) {
            cfiles.emplace_عقب();
            std::سمتیہ<AstCFile*>& slow = cfiles.عقب();
            threadScope.قطار_میں_ڈالو([&slow] { slow = EmitCTrace::مین(/* slow: */ سچ); });
            cfiles.emplace_عقب();
            std::سمتیہ<AstCFile*>& fast = cfiles.عقب();
            threadScope.قطار_میں_ڈالو([&fast] { fast = EmitCTrace::مین(/* slow: */ جھوٹ); });
        }
    }
    // Add files to netlist
    برائے (ثابت std::سمتیہ<AstCFile*>& cfileps : cfiles) {
        برائے (AstCFile* ثابت cfilep : cfileps) v3Global.rootp()->جوڑو_Filesp(cfilep);
    }
}

خالی V3EmitC::emitcFiles() {
    UINFO(2, __FUNCTION__ << ":");
    برائے (Ast_گرہ_File *filep = v3Global.rootp()->filesp(), *nextp; filep; filep = nextp) {
        nextp = VN_AS(filep->nextp(), گرہ_File);
        اگر (AstCFile* ثابت cfilep = VN_CAST(filep, CFile)) EmitCFile::مین(cfilep);
    }
}
