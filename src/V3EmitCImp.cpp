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

#ਅਨ੍ਤਰ੍ਭਾਵ "V3PchAstMT.h"

#ਅਨ੍ਤਰ੍ਭਾਵ "V3EmitC.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3EmitCFunc.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3ThreadPool.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3UniqueNames.h"

#ਅਨ੍ਤਰ੍ਭਾਵ <algorithm>
#ਅਨ੍ਤਰ੍ਭਾਵ <map>
#ਅਨ੍ਤਰ੍ਭਾਵ <set>
#ਅਨ੍ਤਰ੍ਭਾਵ <tuple>
#ਅਨ੍ਤਰ੍ਭਾਵ <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Internal EmitC implementation

ਵਰਗ EmitCImp final : ਜਨਤਕ EmitCFunc {
    // MEMBERS
    // Base module (For non-classes, same as m_modp. For classes, it's the ClassPackage.)
    ਅਚਲ Ast_ਗੰਢ_Module* ਅਚਲ m_fileModp;
    ਅਚਲ ਤਾਰਕਿਕ m_slow;  // Creating __Slow file
    V3_Unique_ਨਾਂ m_unique_ਨਾਂ;  // Generates unique file names
    ਅਚਲ std::string m_file_ਅਧਾਰ_Name = EmitCUtil::ਅਗੇਤਰ_Name_Protect(m_fileModp);
    ਪਾਠ_Stmts m_ctorStmts;  // Statements constructing members in 'ctor'
    ਪਾਠ_Stmts m_dtorStmts;  // Statements destroying members in 'dtor'
    std::string m_ctor_ਵੰਡੋ_Decl;  // Declaration of sub-function 'ctor' was split into, if any
    std::string m_dtor_ਵੰਡੋ_Decl;  // Declaration of sub-function 'dtor' was split into, if any

    // METHODS
    ਖਾਲੀ open_ਅਗਲਾ_Output_File(ਅਚਲ std::string& fileName) {
        open_New_Output_ਸਰੋਤ_File(fileName, m_slow, ਝੂਠ, "Design implementation internals");
        emit_File_ਸਿਰਲੇਖ();
    }
    ਖਾਲੀ emit_File_ਸਿਰਲੇਖ() {
        puts("// See " + EmitCUtil::ਸਿਖਰ_Class_Name() + ".h for the primary calling header\n");
        puts("\n");
        puts("#include \"" + EmitCUtil::pchClassName() + ".h\"\n");
        emitSystemCSection(m_fileModp, VSystemCSectionType::IMP_HDR);
        // Need to emit new lazy declarations
        m_lazyDecls.reset();
    }

    ਖਾਲੀ emitParamDefns(ਅਚਲ Ast_ਗੰਢ_Module* modp) {
        ਤਾਰਕਿਕ ਪਹਿਲਾ = ਸੱਚ;
        ਲਈ (ਅਚਲ Ast_ਗੰਢ* nodep = modp->stmtsp(); nodep; nodep = nodep->nextp()) {
            ਜੇ (ਅਚਲ AstVar* ਅਚਲ varp = VN_CAST(nodep, Var)) {
                ਜੇ (varp->isParam()) {
                    ਜੇ (ਪਹਿਲਾ) {
                        puts("\n");
                        ਅਚਲ string modName = EmitCUtil::ਅਗੇਤਰ_Name_Protect(modp);
                        putsDecoration(modp, "// Parameter definitions for " + modName + "\n");
                        ਪਹਿਲਾ = ਝੂਠ;
                    }
                    ਜੇ (varp->dtypep()->isLiteralType()) putns(varp, "VL_CONSTINIT_CXX20 ");
                    putns(varp, "const ");
                    emitVarDefn(varp);
                    puts(";\n");
                }
            }
        }
        ਜੇ (!ਪਹਿਲਾ) puts("\n");
    }
    // Add statements explicitly constructing, or destroying, a member AstVar
    ਸਥਿਰ ਖਾਲੀ ਜੋੜੋ_Var_Ctor_Dtor_Stmts(ਪਾਠ_Stmts& stmts, ਅਚਲ AstVar* varp, ਤਾਰਕਿਕ destroy) {
        ਅਚਲ std::string name = "vlSelf->" + varName(varp);

        // SystemC port arrays are C arrays, so construct them element by element. They are
        // default constructed (unnamed), as the elements of an array member would be.
        ਜੇ (varp->isSc() && VN_IS(varp->dtypeSkipRefp(), UnpackArrayDType)) {
            std::ਸਦਿਸ਼<std::string> elems{name};
            ਲਈ (ਅਚਲ AstUnpackArrayDType* adtypep
                 = VN_CAST(varp->dtypeSkipRefp(), UnpackArrayDType);
                 adtypep; adtypep = VN_CAST(adtypep->subDTypep()->skipRefp(), UnpackArrayDType)) {
                std::ਸਦਿਸ਼<std::string> subElems;
                ਲਈ (ਅਚਲ std::string& elem : elems) {
                    ਲਈ (ਪੂਰਨ i = 0; i < adtypep->elementsConst(); ++i) {
                        subElems.ਧੱਕੋ_ਪਿੱਠ(elem + "[" + std::ਮੰਜ਼ਿਲ_string(i) + "]");
                    }
                }
                elems = std::move(subElems);
            }
            ਜੇ (destroy) std::ਉਲਟ(elems.begin(), elems.ਸਿਰਾ());
            ਲਈ (ਅਚਲ std::string& e : elems) {
                stmts.ਜੋੜੋ((destroy ? "VL_INPLACE_DESTRUCT(" : "VL_INPLACE_CONSTRUCT(") + e + ");");
            }
            ਵਾਪਸ;
        }

        // Other variables are constructed whole, even if aggregates
        ਜੇ (destroy) {
            stmts.ਜੋੜੋ("VL_INPLACE_DESTRUCT(" + name + ");");
            ਵਾਪਸ;
        }
        ਅਚਲ AstBasicDType* ਅਚਲ basicp = VN_CAST(varp->dtypeSkipRefp(), BasicDType);
        std::string ਦਲੀਲਾਂ;
        ਜੇ (varp->isSc()) {
            // SystemC ports need their name
            ਦਲੀਲਾਂ = ", \"" + V3_ਬਾਹਰਲਾ_Formatter::quoteNameControls(varp->nameProtect()) + "\"";
        } ਨਹੀਂਤਾਂ ਜੇ (basicp && basicp->keyword().isMTaskState()) {
            // MTask state needs the number of upstream dependencies to wait for
            ਦਲੀਲਾਂ = ", " + VN_AS(varp->valuep(), Const)->ਅੰਕ().emitC();
        } ਨਹੀਂਤਾਂ ਜੇ (basicp && basicp->isDelayScheduler()) {
            // Delay scheduler needs the context pointer
            ਦਲੀਲਾਂ = ", *vlSelf->vlSymsp->_vm_contextp__";
        }
        stmts.ਜੋੜੋ("VL_INPLACE_CONSTRUCT(" + name + ਦਲੀਲਾਂ + ");");
    }
    // Gather the statements constructing/destroying members in anonymous unions, and split
    // them into sub-functions if needed. As this emits files, no output file can be open.
    ਖਾਲੀ createCtorDtorStmts(ਅਚਲ Ast_ਗੰਢ_Module* modp) {
        // Construct member variables
        ਲਈ (ਅਚਲ Ast_ਗੰਢ* nodep = modp->stmtsp(); nodep; nodep = nodep->nextp()) {
            ਅਚਲ AstVar* ਅਚਲ varp = VN_CAST(nodep, Var);
            ਜੇ (varp && isExplicitlyConstructed(varp)) {
                ਜੋੜੋ_Var_Ctor_Dtor_Stmts(m_ctorStmts, varp, ਝੂਠ);
            }
        }

        // Destroy in reverse order
        ਅਚਲ Ast_ਗੰਢ* ਅਚਲ lastp = modp->stmtsp() ? modp->stmtsp()->lastp() : ਖਾਲੀ_ਸੂਚਕ;
        ਲਈ (ਅਚਲ Ast_ਗੰਢ* nodep = lastp; nodep; nodep = nodep->prevp()) {
            ਅਚਲ AstVar* ਅਚਲ varp = VN_CAST(nodep, Var);
            ਜੇ (varp && isExplicitlyConstructed(varp)) {
                ਜੋੜੋ_Var_Ctor_Dtor_Stmts(m_dtorStmts, varp, ਸੱਚ);
            }
        }

        // Split into sub-functions if needed
        ਅਚਲ std::string modName = EmitCUtil::ਅਗੇਤਰ_Name_Protect(modp);
        std::tie(m_ctor_ਵੰਡੋ_Decl, m_dtor_ਵੰਡੋ_Decl) = emit_ਵੰਡੋ_Ctor_Dtor(
            m_ctorStmts, m_dtorStmts, 0, modName, modName + "* vlSelf", "vlSelf", ਝੂਠ,
            "Design implementation internals", [ਇਹ]() { emit_File_ਸਿਰਲੇਖ(); });
    }
    ਖਾਲੀ emitCtorDtorImp(ਅਚਲ Ast_ਗੰਢ_Module* modp) {
        ਅਚਲ std::string modName = EmitCUtil::ਅਗੇਤਰ_Name_Protect(modp);

        // ctor
        ਜੇ (modp->ctorVarReset()) {
            m_lazyDecls.emit("\nvoid " + modName + "__", protect("_ctor_var_reset"),
                             "(" + modName + "* vlSelf);\n");
        }

        ਜੇ (!m_ctor_ਵੰਡੋ_Decl.ਖਾਲੀ_ਥਾਵਾਂ()) puts("\n" + m_ctor_ਵੰਡੋ_Decl + "\n");
        puts("\nvoid " + modName + "__ctor(" + modName + "* vlSelf, " + EmitCUtil::symClassName()
             + "* symsp, const char* namep) {\n");
        puts("vlSelf->vlSymsp = symsp;\n");
        ਜੇ (modp->is_ਸਿਖਰ()) {
            puts("vlSelf->vlNamep = strdup(namep);\n");
        } ਨਹੀਂਤਾਂ {
            puts("vlSelf->vlNamep = strdup(Verilated::catName(symsp->name(), namep));\n");
        }
        putsDecoration(modp, "// Construct members\n");
        ਲਈ (ਅਚਲ ਪਾਠ_Stmts::Stmt& stmt : m_ctorStmts) puts(stmt.m_ਪਾਠ + "\n");

        putsDecoration(modp, "// Reset structure values\n");
        ਜੇ (modp->ctorVarReset()) {
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
        ਜੇ (!m_dtor_ਵੰਡੋ_Decl.ਖਾਲੀ_ਥਾਵਾਂ()) puts("\n" + m_dtor_ਵੰਡੋ_Decl + "\n");
        puts("\nvoid " + modName + "__dtor(" + modName + "* vlSelf) {\n");
        putsDecoration(modp, "// Destroy members\n");
        ਲਈ (ਅਚਲ ਪਾਠ_Stmts::Stmt& stmt : m_dtorStmts) puts(stmt.m_ਪਾਠ + "\n");
        puts("VL_DO_DANGLING(std::free(const_cast<char*>(vlSelf->vlNamep)), "
             "vlSelf->vlNamep);\n");
        puts("}\n");

        // C++ destructor
        puts("\n" + modName + "::~" + modName + "() {\n");
        emitSystemCSection(modp, VSystemCSectionType::DTOR);
        puts(modName + "__dtor(this);\n");
        puts("}\n");
    }

    ਖਾਲੀ emitConfigureImp(ਅਚਲ Ast_ਗੰਢ_Module* modp) {
        ਅਚਲ string modName = EmitCUtil::ਅਗੇਤਰ_Name_Protect(modp);

        ਜੇ (v3Global.opt.coverage()) {
            puts("\n");
            m_lazyDecls.emit("void " + modName + "__", protect("_configure_coverage"),
                             "(" + modName + "* vlSelf, bool first);");
        }

        ਜੇ (v3Global.opt.coverage()) {
            puts("\nvoid " + modName + "::" + protect("__Vconfigure") + "(bool first) {\n");
            puts("(void)first;  // Prevent unused variable warning\n");
            puts(modName + "__" + protect("_configure_coverage") + "(this, first);\n");
            puts("}\n");
        }
    }
    ਖਾਲੀ emitCoverageImp() {
        // Rather than putting out VL_COVER_INSERT calls directly, we do it via this
        // function. This gets around gcc slowness constructing all of the template
        // arguments.
        ਜੇ (v3Global.opt.coverage()) {
            puts("\n// Coverage\n");
            puts("void " + EmitCUtil::ਅਗੇਤਰ_Name_Protect(m_modp) + "::__vlCoverInsert(");
            puts(v3Global.opt.ਧਾਗੇ() > 1 ? "std::atomic<uint32_t>" : "uint32_t");
            puts("* countp, bool enable, bool localCounter, const char* filenamep, int lineno, "
                 "int column,\n");
            puts("const char* hierp, const char* pagep, const char* commentp, const char* "
                 "linescovp,\n");
            puts("const char* fsmVarp, const char* fsmFromp, const char* fsmTop, const char* "
                 "fsmTagp) {\n");
            ਜੇ (v3Global.opt.ਧਾਗੇ() > 1) {
                puts("assert(sizeof(uint32_t) == sizeof(std::atomic<uint32_t>));\n");
                puts("uint32_t* count32p = reinterpret_cast<uint32_t*>(countp);\n");
            } ਨਹੀਂਤਾਂ {
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
        ਜੇ (v3Global.opt.coverageToggle()) {
            puts("\n// Toggle Coverage\n");
            puts("void " + EmitCUtil::ਅਗੇਤਰ_Name_Protect(m_modp) + "::__vlCoverToggleInsert(");
            puts("int begin, int end, bool ranged, ");
            puts(v3Global.opt.ਧਾਗੇ() > 1 ? "std::atomic<uint32_t>" : "uint32_t");
            puts("* countp, bool enable, bool localCounter, const char* filenamep, int lineno, "
                 "int column,\n");
            puts("const char* hierp, const char* pagep, const char* commentp) {\n");
            ਜੇ (v3Global.opt.ਧਾਗੇ() > 1) {
                puts("assert(sizeof(uint32_t) == sizeof(std::atomic<uint32_t>));\n");
            }
            puts("int step = (end >= begin) ? 1 : -1;\n");
            // range is inclusive
            puts("for (int i = begin; i != end + step; i += step) {\n");
            puts("for (int j = 0; j < 2; j++) {\n");
            ਜੇ (v3Global.opt.ਧਾਗੇ() > 1) {
                puts("uint32_t* count32p = reinterpret_cast<uint32_t*>(countp);\n");
            } ਨਹੀਂਤਾਂ {
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
                                       //ਖਾਲੀ_ਥਾਵਾਂ
            puts("++countp;\n");
            puts("}\n");
            puts("}\n");
            puts("}\n");
        }
    }
    ਖਾਲੀ emitSavableImp(ਅਚਲ Ast_ਗੰਢ_Module* modp) {
        ਜੇ (v3Global.opt.savable()) {
            puts("\n// Savable\n");
            ਲਈ (ਪੂਰਨ de = 0; de < 2; ++de) {
                ਅਚਲ string classname = de ? "VerilatedDeserialize" : "VerilatedSerialize";
                ਅਚਲ string funcname = de ? "__Vdeserialize" : "__Vserialize";
                ਅਚਲ string op = de ? ">>" : "<<";
                // NOLINTNEXTLINE(performance-inefficient-string-concatenation)
                putns(modp, "void " + EmitCUtil::ਅਗੇਤਰ_Name_Protect(modp) + "::" + protect(funcname)
                                + "(" + classname + "& os) {\n");
                // Place a computed checksum to ensure proper structure save/restore formatting
                // OK if this hash includes some things we won't dump, since
                // just looking for loading the wrong model
                VHashSha512 hash;
                ਲਈ (Ast_ਗੰਢ* nodep = modp->stmtsp(); nodep; nodep = nodep->nextp()) {
                    ਜੇ (ਅਚਲ AstVar* ਅਚਲ varp = VN_CAST(nodep, Var)) {
                        hash.ਪਾਓ(varp->name());
                        hash.ਪਾਓ(varp->dtypep()->ਚੌੜਾਈ());
                    }
                }
                ofp()->ਲਿਖੋ("uint64_t __Vcheckval = 0x%" PRIx64 "ULL;\n",
                              static_cast<uint64_t>(hash.digestUInt64()));
                ਜੇ (de) {
                    puts("os.readAssert(__Vcheckval);\n");
                } ਨਹੀਂਤਾਂ {
                    puts("os << __Vcheckval;\n");
                }

                // Save context
                // If multiple models save the same context we'll save it multiple
                // times, but is harmless, and doing it otherwise would break
                // backwards compatibility.
                puts("os " + op + " vlSymsp->_vm_contextp__;\n");

                // Save all members
                ਲਈ (Ast_ਗੰਢ* nodep = modp->stmtsp(); nodep; nodep = nodep->nextp()) {
                    ਜੇ (ਅਚਲ AstVar* ਅਚਲ varp = VN_CAST(nodep, Var)) {
                        ਜੇ (varp->isIO() && modp->is_ਸਿਖਰ() && optSystemC()) {
                            // System C top I/O doesn't need loading, as the
                            // lower level subinst code does it.
                        } ਨਹੀਂਤਾਂ ਜੇ (varp->isParam()) {
                        } ਨਹੀਂਤਾਂ ਜੇ (VN_IS(varp->dtypep(), NBACommit_ਕਤਾਰ_DType)) {
                        } ਨਹੀਂਤਾਂ {
                            ਪੂਰਨ vects = 0;
                            Ast_ਗੰਢ_DType* elementp = varp->dtypeSkipRefp();
                            ਲਈ (AstUnpackArrayDType* arrayp = VN_CAST(elementp, UnpackArrayDType);
                                 arrayp; arrayp = VN_CAST(elementp, UnpackArrayDType)) {
                                ਅਚਲ ਪੂਰਨ vecnum = vects++;
                                UASSERT_OBJ(arrayp->hi() >= arrayp->lo(), varp,
                                            "Should have swapped msb & lsb earlier.");
                                ਅਚਲ string ivar = "__Vi"s + cvt_ਮੰਜ਼ਿਲ_Str(vecnum);
                                puts("for (int __Vi" + cvt_ਮੰਜ਼ਿਲ_Str(vecnum) + " = " + cvt_ਮੰਜ਼ਿਲ_Str(0));
                                puts("; " + ivar + " < " + cvt_ਮੰਜ਼ਿਲ_Str(arrayp->elementsConst()));
                                puts("; ++" + ivar + ") {\n");
                                elementp = arrayp->subDTypep()->skipRefp();
                            }
                            ਅਚਲ AstBasicDType* ਅਚਲ basicp = elementp->basicp();
                            // Do not save MTask state, only matters within an evaluation
                            ਜੇ (basicp && basicp->keyword().isMTaskState()) ਜਾਰੀ;
                            // Want to detect types that are represented as arrays
                            // (i.e. packed types of more than 64 bits).
                            ਜੇ (elementp->isWide()
                                && !(basicp && basicp->keyword() == VBasicDTypeKwd::STRING)) {
                                ਅਚਲ ਪੂਰਨ vecnum = vects++;
                                ਅਚਲ string ivar = "__Vi"s + cvt_ਮੰਜ਼ਿਲ_Str(vecnum);
                                puts("for (int __Vi" + cvt_ਮੰਜ਼ਿਲ_Str(vecnum) + " = " + cvt_ਮੰਜ਼ਿਲ_Str(0));
                                puts("; " + ivar + " < " + cvt_ਮੰਜ਼ਿਲ_Str(elementp->ਚੌੜਾਈ_ਸ਼ਬਦ()));
                                puts("; ++" + ivar + ") {\n");
                            }
                            putns(varp, "os" + op + varName(varp));
                            ਲਈ (ਪੂਰਨ v = 0; v < vects; ++v) puts("[__Vi" + cvt_ਮੰਜ਼ਿਲ_Str(v) + "]");
                            puts(";\n");
                            ਲਈ (ਪੂਰਨ v = 0; v < vects; ++v) puts("}\n");
                        }
                    }
                }

                puts("}\n");
            }
        }
    }
    // Predicate to check if we actually need to emit anything into the common implementation file.
    // Used to avoid creating empty output files.
    ਤਾਰਕਿਕ hasCommonImp(ਅਚਲ Ast_ਗੰਢ_Module* modp) ਅਚਲ {
        // Nothing to emit if no module!
        ਜੇ (!modp) ਵਾਪਸ ਝੂਠ;
        // We always need the slow file
        ਜੇ (m_slow) ਵਾਪਸ ਸੱਚ;
        // The fast file is only required when we have `systemc_implementation nodes
        ਲਈ (ਅਚਲ AstSystemCSection* scSectionp = modp->scSectionsp(); scSectionp;
             scSectionp = VN_AS(scSectionp->nextp(), SystemCSection)) {
            ਜੇ (scSectionp->sectionType() == VSystemCSectionType::IMP) ਵਾਪਸ ਸੱਚ;
        }
        ਵਾਪਸ ਝੂਠ;
    }
    // Actually emit common implementation contents for given AstNodeModule
    ਖਾਲੀ doCommonImp(ਅਚਲ Ast_ਗੰਢ_Module* modp) {
        ਜੇ (m_slow) {
            ਜੇ (!VN_IS(modp, Class)) {
                emitParamDefns(modp);
                emitCtorDtorImp(modp);
                emitConfigureImp(modp);
                emitCoverageImp();
            }
            emitSavableImp(modp);
        } ਨਹੀਂਤਾਂ {
            // From `systemc_implementation
            emitSystemCSection(modp, VSystemCSectionType::IMP);
        }
    }
    ਖਾਲੀ emitCommonImp(ਅਚਲ Ast_ਗੰਢ_Module* modp) {
        ਅਚਲ AstClass* ਅਚਲ classp
            = VN_IS(modp, ClassPackage) ? VN_AS(modp, ClassPackage)->classp() : ਖਾਲੀ_ਸੂਚਕ;

        ਜੇ (hasCommonImp(modp) || hasCommonImp(classp)) {
            // Create ctor/dtor statements. If these need splitting, they will be emitted
            // during construction so do this before opening the output file.
            ਜੇ (m_slow && !VN_IS(modp, Class)) createCtorDtorStmts(modp);

            open_ਅਗਲਾ_Output_File(m_file_ਅਧਾਰ_Name);

            doCommonImp(modp);
            ਜੇ (classp) {
                VL_RESTORER(m_modp);
                m_modp = classp;
                doCommonImp(classp);
            }

            closeOutputFile();
        }
    }
    ਖਾਲੀ emitCFuncImp(ਅਚਲ Ast_ਗੰਢ_Module* modp) {
        // Functions to be emitted here
        std::ਸਦਿਸ਼<AstCFunc*> funcps;

        ਅਚਲ ਸਵੈਚਾਲਿਤ gather = [ਇਹ, &funcps](ਅਚਲ Ast_ਗੰਢ_Module* modp) {
            ਲਈ (Ast_ਗੰਢ* nodep = modp->stmtsp(); nodep; nodep = nodep->nextp()) {
                AstCFunc* ਅਚਲ funcp = VN_CAST(nodep, CFunc);
                ਜੇ (!funcp) ਜਾਰੀ;
                // TRACE_* and DPI handled elsewhere
                ਜੇ (funcp->isTrace()) ਜਾਰੀ;
                ਜੇ (funcp->dpiImportPrototype()) ਜਾਰੀ;
                ਜੇ (funcp->dpiExportDispatcher()) ਜਾਰੀ;
                ਜੇ (funcp->slow() != m_slow) ਜਾਰੀ;
                funcps.ਧੱਕੋ_ਪਿੱਠ(funcp);
            }
        };

        gather(modp);
        VL_RESTORER(m_classOrPackage);
        ਜੇ (ਅਚਲ AstClassPackage* ਅਚਲ packagep = VN_CAST(modp, ClassPackage)) {
            m_classOrPackage = packagep;
            gather(packagep->classp());
        }

        // Do not create empty files
        ਜੇ (funcps.ਖਾਲੀ_ਥਾਵਾਂ()) ਵਾਪਸ;

        // Open output file
        open_ਅਗਲਾ_Output_File(m_unique_ਨਾਂ.get(m_file_ਅਧਾਰ_Name));
        // Emit all functions
        ਲਈ (AstCFunc* ਅਚਲ funcp : funcps) {
            VL_RESTORER(m_modp);
            m_modp = EmitCParentModule::get(funcp);
            iterateConst(funcp);
        }
        // Close output file
        closeOutputFile();
    }

    // VISITORS
    ਖਾਲੀ ਦੇਖੋ(AstCFunc* nodep) override {
        ਜੇ (ਵੰਡੋ_Needed()) {
            // Splitting file, so using parallel build.
            v3Global.useParallelBuild(ਸੱਚ);
            // Close old file
            closeOutputFile();
            // Open a new file
            open_ਅਗਲਾ_Output_File(m_unique_ਨਾਂ.get(m_file_ਅਧਾਰ_Name));
        }

        EmitCFunc::ਦੇਖੋ(nodep);
    }

    ਸਪਸ਼ਟ EmitCImp(ਅਚਲ Ast_ਗੰਢ_Module* modp, ਤਾਰਕਿਕ slow)
        : m_fileModp{modp}
        , m_slow{slow} {
        UINFO(5, "  Emitting implementation of " << EmitCUtil::ਅਗੇਤਰ_Name_Protect(modp));

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
    ~EmitCImp() override = ਮੂਲ;

ਜਨਤਕ:
    ਸਥਿਰ std::ਸਦਿਸ਼<AstCFile*> ਮੁੱਖ(ਅਚਲ Ast_ਗੰਢ_Module* modp, ਤਾਰਕਿਕ slow) VL_MT_STABLE {
        EmitCImp emitCImp{modp, slow};
        ਵਾਪਸ emitCImp.getAndClearCfileps();
    }
};

//######################################################################
// Tracing routines

// Trace type descriptors go in a different file as it needs to be written in
// parallel with the actual trace function source files
ਵਰਗ EmitCTraceTypes final : ਜਨਤਕ EmitCFunc {
    // NODE STATE/TYPES
    // None allowed to support threaded emitting

    // STATE
    ਪੂਰਨ m_enum_ਅੰਕ = 0;  // Enumeration number (whole netlist)
    std::unordered_map<Ast_ਗੰਢ*, ਪੂਰਨ> m_enum_ਅੰਕ_Map;  // EnumDType to enumeration number
    ਪੂਰਨ m_traceTypeSubs = 0;  // Number of trace type declaration sub-functions
    V3_Unique_ਨਾਂ m_unique_ਨਾਂ;  // Generates unique file names
    ਅਚਲ std::string m_file_ਅਧਾਰ_Name = EmitCUtil::ਸਿਖਰ_Class_Name() + "_" + protect("_TraceDecls");
    // This one uses CSplitTrace for file splitting, which is incorrect but historically accurates
    ਅਚਲ ਮਾਪ_t m_ਵੰਡੋ_ਹੱਦਬੰਦੀ = v3Global.opt.output_ਵੰਡੋ_CTrace()
                                    ? static_cast<ਮਾਪ_t>(v3Global.opt.output_ਵੰਡੋ_CTrace())
                                    : std::numeric_limits<ਮਾਪ_t>::max();

    ਖਾਲੀ open_ਅਗਲਾ_Output_File() {
        open_New_Output_ਸਰੋਤ_File(m_unique_ਨਾਂ.get(m_file_ਅਧਾਰ_Name), ਸੱਚ, ਸੱਚ,
                                "Tracing declarations");
        puts("\n");
        ਲਈ (ਅਚਲ std::string& ਅਧਾਰ : v3Global.opt.trace_ਸਰੋਤ_Langs()) {
            puts("#include \"" + ਅਧਾਰ + ".h\"\n");
        }
        puts("\n");
        puts("\nvoid " + EmitCUtil::ਅਗੇਤਰ_Name_Protect(m_modp) + "__"
             + protect("traceDeclTypesSub" + std::ਮੰਜ਼ਿਲ_string(m_traceTypeSubs++)) + "("
             + v3Global.opt.trace_Class_ਅਧਾਰ() + "* tracep) {\n");
    }

ਜਨਤਕ:
    // METHODS
    ਪੂਰਨ get_Enum_Map_ਅੰਕ(AstEnumDType* nodep) {
        ਪੂਰਨ& enumNumr = m_enum_ਅੰਕ_Map[nodep];
        ਜੇ (!enumNumr) {
            ਜੇ (ਵੰਡੋ_Needed(m_ਵੰਡੋ_ਹੱਦਬੰਦੀ)) {
                // Splitting file, so using parallel build.
                v3Global.useParallelBuild(ਸੱਚ);
                puts("}\n");
                closeOutputFile();
                open_ਅਗਲਾ_Output_File();
            }

            enumNumr = ++m_enum_ਅੰਕ;
            ਪੂਰਨ nvals = 0;
            puts("{\n");
            putns(nodep, "const char* " + protect("__VenumItemNames") + "[]\n");
            puts("= {");
            ਲਈ (Ast_Enum_ਵਸਤੂ* itemp = nodep->itemsp(); itemp;
                 itemp = VN_AS(itemp->nextp(), Enum_ਵਸਤੂ)) {
                ਜੇ (++nvals > 1) puts(", ");
                putbs("\"" + itemp->prettyName() + "\"");
            }
            puts("};\n");
            nvals = 0;
            puts("const char* " + protect("__VenumItemValues") + "[]\n");
            puts("= {");
            ਲਈ (Ast_Enum_ਵਸਤੂ* itemp = nodep->itemsp(); itemp;
                 itemp = VN_AS(itemp->nextp(), Enum_ਵਸਤੂ)) {
                AstConst* ਅਚਲ constp = VN_AS(itemp->valuep(), Const);
                ਜੇ (++nvals > 1) puts(", ");
                putbs("\"" + constp->ਅੰਕ().displayed(nodep, "%0b") + "\"");
            }
            puts("};\n");
            puts("tracep->declDTypeEnum(" + std::ਮੰਜ਼ਿਲ_string(enumNumr) + ", \"" + nodep->prettyName()
                 + "\", " + std::ਮੰਜ਼ਿਲ_string(nvals) + ", " + std::ਮੰਜ਼ਿਲ_string(nodep->ਚੌੜਾਈ_Min()) + ", "
                 + protect("__VenumItemNames") + ", " + protect("__VenumItemValues") + ");\n");
            puts("}\n");
            ਵੰਡੋ_ਮਾਪ_Inc(Ast_ਗੰਢ::INSTR_ਗਿਣਤੀ_CALL);
        }
        ਵਾਪਸ enumNumr;
    }

    // Close output file
    ਖਾਲੀ finalize() {
        // Close function definition
        puts("}\n");

        ਅਚਲ std::string modName = EmitCUtil::ਅਗੇਤਰ_Name_Protect(m_modp);
        ਅਚਲ std::string ਦਲੀਲਾਂ = v3Global.opt.trace_Class_ਅਧਾਰ() + "* tracep";

        // Forward declarations for subs in other files
        ਲਈ (ਪੂਰਨ i = 0; i < m_traceTypeSubs - 1; ++i) {
            puts("void " + modName + "__" + protect("traceDeclTypesSub" + std::ਮੰਜ਼ਿਲ_string(i)) + "("
                 + ਦਲੀਲਾਂ + ");\n");
        }

        // Create top level trace_decl_types function and call each sub-function
        puts("\nvoid " + modName + "__" + protect("trace_decl_types") + "(" + ਦਲੀਲਾਂ + ") {\n");
        ਲਈ (ਪੂਰਨ i = 0; i < m_traceTypeSubs; ++i) {
            puts(modName + "__" + protect("traceDeclTypesSub" + std::ਮੰਜ਼ਿਲ_string(i))
                 + "(tracep);\n");
        }
        puts("}\n");

        closeOutputFile();
    }

    EmitCTraceTypes() {
        m_modp = v3Global.rootp()->ਸਿਖਰ_Modulep();
        open_ਅਗਲਾ_Output_File();
    }
    ~EmitCTraceTypes() override = ਮੂਲ;
};

ਵਰਗ EmitCTrace final : ਜਨਤਕ EmitCFunc {
    // NODE STATE/TYPES
    // None allowed to support threaded emitting

    // MEMBERS
    ਅਚਲ ਤਾਰਕਿਕ m_slow;  // Making slow file
    ਅਚਲ std::unique_ptr<EmitCTraceTypes> m_emitTypesp{m_slow ? ਨਵਾਂ EmitCTraceTypes{} : ਖਾਲੀ_ਸੂਚਕ};
    V3_Unique_ਨਾਂ m_unique_ਨਾਂ;  // Generates unique file names
    ਅਚਲ std::string m_file_ਅਧਾਰ_Name = EmitCUtil::ਸਿਖਰ_Class_Name() + "_" + protect("_Trace");

    // METHODS
    ਖਾਲੀ open_ਅਗਲਾ_Output_File() {
        open_New_Output_ਸਰੋਤ_File(m_unique_ਨਾਂ.get(m_file_ਅਧਾਰ_Name), m_slow, ਸੱਚ,
                                "Tracing implementation internals");
        puts("\n");
        ਲਈ (ਅਚਲ std::string& ਅਧਾਰ : v3Global.opt.trace_ਸਰੋਤ_Langs()) {
            puts("#include \"" + ਅਧਾਰ + ".h\"\n");
        }
        puts("#include \"" + EmitCUtil::symClassName() + ".h\"\n");
        puts("\n");
        // Need to emit new lazy declarations
        m_lazyDecls.reset();
    }

    ਤਾਰਕਿਕ emitTraceIsScBv(ਅਚਲ AstTraceInc* nodep) {
        ਅਚਲ AstVarRef* ਅਚਲ varrefp = VN_CAST(nodep->declp()->valuep(), VarRef);
        ਜੇ (!varrefp) ਵਾਪਸ ਝੂਠ;
        ਅਚਲ AstVar* ਅਚਲ varp = varrefp->varp();
        ਵਾਪਸ varp->isSc() && varp->isScBv();
    }

    ਤਾਰਕਿਕ emitTraceIsScBigUint(ਅਚਲ AstTraceInc* nodep) {
        ਅਚਲ AstVarRef* ਅਚਲ varrefp = VN_CAST(nodep->declp()->valuep(), VarRef);
        ਜੇ (!varrefp) ਵਾਪਸ ਝੂਠ;
        ਅਚਲ AstVar* ਅਚਲ varp = varrefp->varp();
        ਵਾਪਸ varp->isSc() && varp->isScBigUint();
    }

    ਤਾਰਕਿਕ emitTraceIsScUint(ਅਚਲ AstTraceInc* nodep) {
        ਅਚਲ AstVarRef* ਅਚਲ varrefp = VN_CAST(nodep->declp()->valuep(), VarRef);
        ਜੇ (!varrefp) ਵਾਪਸ ਝੂਠ;
        ਅਚਲ AstVar* ਅਚਲ varp = varrefp->varp();
        ਵਾਪਸ varp->isSc() && (varp->isScUint() || varp->isScUintBool());
    }

    ਖਾਲੀ emitTraceInitOne(ਅਚਲ AstTraceDecl* nodep, ਪੂਰਨ enum_ਅੰਕ) {
        std::string direction;
        direction = nodep->declDirection().traceSigDirection();

        AstCCall* ਅਚਲ callp = nodep->dtypeCallp();
        ਜੇ (callp) {
            callp->argTypes(callp->argTypes() + ", " + cvt_ਮੰਜ਼ਿਲ_Str(nodep->fidx()) + ", c+"
                            + cvt_ਮੰਜ਼ਿਲ_Str(nodep->ਕੋਡ()) + ", " + direction);
            ਵਾਪਸ;
        }

        ਜੇ (nodep->dtypep()->basicp()->isDouble()) {
            puts("VL_TRACE_DECL_DOUBLE");
        } ਨਹੀਂਤਾਂ ਜੇ (nodep->isWide()) {
            puts("VL_TRACE_DECL_WIDE");
        } ਨਹੀਂਤਾਂ ਜੇ (nodep->isQuad()) {
            puts("VL_TRACE_DECL_QUAD");
        } ਨਹੀਂਤਾਂ ਜੇ (nodep->ਦੋਆਧਾਰੀ_ਅੰਕ_Range().ranged()) {
            puts("VL_TRACE_DECL_BUS");
        } ਨਹੀਂਤਾਂ ਜੇ (nodep->dtypep()->basicp()->isEvent()) {
            puts("VL_TRACE_DECL_EVENT");
        } ਨਹੀਂਤਾਂ {
            puts("VL_TRACE_DECL_BIT");
        }

        ਜੇ (nodep->arrayRange().ranged()) {
            puts("_ARRAY(tracep");
        } ਨਹੀਂਤਾਂ {
            puts("(tracep");
        }

        //ਕੋਡ
        puts(",c+" + cvt_ਮੰਜ਼ਿਲ_Str(nodep->ਕੋਡ()));
        ਜੇ (nodep->arrayRange().ranged()) puts("+i*" + cvt_ਮੰਜ਼ਿਲ_Str(nodep->ਚੌੜਾਈ_ਸ਼ਬਦ()));

        // Function index
        puts(",");
        ਜੇ (nodep->ਵਿੱਚ_Dtype_Func()) {
            puts("fidx");
        } ਨਹੀਂਤਾਂ {
            puts(cvt_ਮੰਜ਼ਿਲ_Str(nodep->fidx()));
        }

        // Name
        puts(",");
        putsQuoted(VIdProtect::protect_ਸ਼ਬਦ_If(nodep->showname(), nodep->protect()));

        // Enum number
        puts("," + cvt_ਮੰਜ਼ਿਲ_Str(enum_ਅੰਕ));

        // Direction
        ਜੇ (nodep->ਵਿੱਚ_Dtype_Func()) {
            puts(", direction");
        } ਨਹੀਂਤਾਂ {
            puts(", " + direction);
        }

        //ਕਿਸਮ
        puts(", VerilatedTraceSigKind::");
        puts(nodep->varType().trace_Sig_ਕਿਸਮ());

        // Type
        puts(", VerilatedTraceSigType::");
        UASSERT_OBJ(nodep->dtypep()->basicp()->keyword() != VBasicDTypeKwd::UNKNOWN, nodep,
                    "DType is unknown");
        puts(nodep->dtypep()->basicp()->keyword().traceSigType());

        // Array range
        ਜੇ (nodep->arrayRange().ranged()) {
            ਜੇ (nodep->arrayRange().ascending()) {
                puts(", (i + " + std::ਮੰਜ਼ਿਲ_string(nodep->arrayRange().lo()) + ")");
            } ਨਹੀਂਤਾਂ {
                puts(", (" + std::ਮੰਜ਼ਿਲ_string(nodep->arrayRange().hi()) + " - i)");
            }
        }

        // Bit range
        ਜੇ (!nodep->dtypep()->basicp()->isDouble() && nodep->ਦੋਆਧਾਰੀ_ਅੰਕ_Range().ranged()) {
            puts(", " + cvt_ਮੰਜ਼ਿਲ_Str(nodep->ਦੋਆਧਾਰੀ_ਅੰਕ_Range().ਖੱਬੇ()) + ","
                 + cvt_ਮੰਜ਼ਿਲ_Str(nodep->ਦੋਆਧਾਰੀ_ਅੰਕ_Range().ਸੱਜੇ()));
        }

        //
        puts(");");
    }

    ਪੂਰਨ emitTraceDeclDType(Ast_ਗੰਢ_DType* nodep) {
        // Return enum number or -1 for none
        ਜੇ (v3Global.opt.traceEnabledFst()) {
            // Skip over refs-to-refs, but stop before final ref so can get data type name
            // Alternatively back in V3Width we could push enum names from upper typedefs
            ਜੇ (AstEnumDType* ਅਚਲ enump = VN_CAST(nodep->skip_Ref_ਮੰਜ਼ਿਲ_Enump(), EnumDType)) {
                ਵਾਪਸ m_emitTypesp->get_Enum_Map_ਅੰਕ(enump);
            }
        }
        ਵਾਪਸ -1;
    }

    ਖਾਲੀ emitTraceChangeOne(AstTraceInc* nodep, ਪੂਰਨ arrayindex) {
        // Note: Both VTraceType::CHANGE and VTraceType::FULL use the 'full' methods
        ਅਚਲ std::string func = nodep->traceType() == VTraceType::CHANGE ? "chg" : "full";
        ਤਾਰਕਿਕ emit_ਚੌੜਾਈ = ਸੱਚ;
        string stype;
        ਜੇ (nodep->dtypep()->basicp()->isDouble()) {
            stype = "Double";
            emit_ਚੌੜਾਈ = ਝੂਠ;
        } ਨਹੀਂਤਾਂ ਜੇ (nodep->isWide() || emitTraceIsScBv(nodep) || emitTraceIsScBigUint(nodep)) {
            stype = "WData";
        } ਨਹੀਂਤਾਂ ਜੇ (nodep->isQuad()) {
            stype = "QData";
        } ਨਹੀਂਤਾਂ ਜੇ (nodep->declp()->ਚੌੜਾਈ_Min() > 16) {
            stype = "IData";
        } ਨਹੀਂਤਾਂ ਜੇ (nodep->declp()->ਚੌੜਾਈ_Min() > 8) {
            stype = "SData";
        } ਨਹੀਂਤਾਂ ਜੇ (nodep->declp()->ਚੌੜਾਈ_Min() > 1) {
            stype = "CData";
        } ਨਹੀਂਤਾਂ ਜੇ (nodep->dtypep()->basicp()->isEvent()) {
            stype = "Event";
            emit_ਚੌੜਾਈ = ਝੂਠ;
        } ਨਹੀਂਤਾਂ {
            stype = "Bit";
            emit_ਚੌੜਾਈ = ਝੂਠ;
        }
        putns(nodep, "bufp->" + func + stype);

        ਅਚਲ uint32_t ਵਿੱਥ = (arrayindex < 0) ? 0 : (arrayindex * nodep->declp()->ਚੌੜਾਈ_ਸ਼ਬਦ());
        ਅਚਲ uint32_t ਕੋਡ = nodep->declp()->ਕੋਡ() + ਵਿੱਥ;
        // Note: Both VTraceType::CHANGE and VTraceType::FULL use the 'full' methods
        puts("(oldp+");
        puts(cvt_ਮੰਜ਼ਿਲ_Str(ਕੋਡ - nodep->ਅਧਾਰ_ਕੋਡ()));
        puts(",");
        ਅਚਲ VNumRange& arrayRange = nodep->declp()->arrayRange();
        ਜੇ (arrayRange.ranged() && !arrayRange.ascending()) {
            arrayindex = arrayRange.elements() - 1 - arrayindex;
        }
        emit_Trace_ਮੁੱਲ(nodep, arrayindex);
        ਜੇ (emit_ਚੌੜਾਈ) puts("," + cvt_ਮੰਜ਼ਿਲ_Str(nodep->declp()->ਚੌੜਾਈ_Min()));
        puts(");\n");
    }

    ਖਾਲੀ emit_Trace_ਮੁੱਲ(ਅਚਲ AstTraceInc* nodep, ਪੂਰਨ arrayindex) {
        ਜੇ (AstVarRef* ਅਚਲ varrefp = VN_CAST(nodep->valuep(), VarRef)) {
            ਅਚਲ AstVar* ਅਚਲ varp = varrefp->varp();
            ਜੇ (varp->isEvent()) puts("&");
            puts("(");
            ਜੇ (emitTraceIsScBigUint(nodep)) {
                puts("(uint32_t*)");
            } ਨਹੀਂਤਾਂ ਜੇ (emitTraceIsScBv(nodep)) {
                puts("VL_SC_BV_DATAP(");
            }
            iterateConst(varrefp);  // Put var name out
            emit_Trace_ਸੂਚਕਾਂਕ(nodep, arrayindex);
            ਜੇ (varp->isSc()) puts(".read()");
            ਜੇ (emitTraceIsScUint(nodep)) {
                puts(nodep->isQuad() ? ".to_uint64()" : ".to_uint()");
            } ਨਹੀਂਤਾਂ ਜੇ (emitTraceIsScBigUint(nodep)) {
                puts(".get_raw()");
            } ਨਹੀਂਤਾਂ ਜੇ (emitTraceIsScBv(nodep)) {
                puts(")");
            }
            puts(")");
        } ਨਹੀਂਤਾਂ {
            puts("(");
            iterateConst(nodep->valuep());
            emit_Trace_ਸੂਚਕਾਂਕ(nodep, arrayindex);
            puts(")");
        }
    }

    ਖਾਲੀ emit_Trace_ਸੂਚਕਾਂਕ(ਅਚਲ AstTraceInc* ਅਚਲ nodep, ਪੂਰਨ arrayindex) {
        // Tracing only supports 1D arrays
        ਜੇ (nodep->declp()->arrayRange().ranged()) {
            ਜੇ (arrayindex == -2) {
                puts("[i]");
            } ਨਹੀਂਤਾਂ ਜੇ (arrayindex == -1) {
                puts("[0]");
            } ਨਹੀਂਤਾਂ {
                puts("[" + cvt_ਮੰਜ਼ਿਲ_Str(arrayindex) + "]");
            }
        }
    }

    // VISITORS
    ਵਰਤੋਂ EmitCFunc::ਦੇਖੋ;  // Suppress hidden overloaded virtual function warning
    ਖਾਲੀ ਦੇਖੋ(AstCFunc* nodep) override {
        ਜੇ (!nodep->isTrace()) ਵਾਪਸ;
        ਜੇ (nodep->slow() != m_slow) ਵਾਪਸ;

        ਜੇ (ਵੰਡੋ_Needed()) {
            // Splitting file, so using parallel build.
            v3Global.useParallelBuild(ਸੱਚ);
            // Close old file
            closeOutputFile();
            // Open a new file
            open_ਅਗਲਾ_Output_File();
        }

        EmitCFunc::ਦੇਖੋ(nodep);
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_Trace_ਧੱਕੋ_ਅਗੇਤਰ* nodep) override {
        putns(nodep, "VL_TRACE_PUSH_PREFIX(tracep, ");
        ਜੇ (nodep->quoted_ਅਗੇਤਰ()) {
            putsQuoted(VIdProtect::protect_ਸ਼ਬਦ_If(nodep->ਅਗੇਤਰ(), nodep->protect()));
        } ਨਹੀਂਤਾਂ {
            puts(nodep->ਅਗੇਤਰ());
        }
        puts(", VerilatedTracePrefixType::");
        puts(nodep->ਅਗੇਤਰ_Type().ascii());
        puts(", " + std::ਮੰਜ਼ਿਲ_string(nodep->ਖੱਬੇ()));
        puts(", " + std::ਮੰਜ਼ਿਲ_string(nodep->ਸੱਜੇ()));
        puts(");\n");
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_Trace_ਕੱਢੋ_ਅਗੇਤਰ* nodep) override {  //
        putns(nodep, "VL_TRACE_POP_PREFIX(tracep);\n");
    }
    ਖਾਲੀ ਦੇਖੋ(AstTraceDecl* nodep) override {
        ਅਚਲ ਪੂਰਨ enum_ਅੰਕ = emitTraceDeclDType(nodep->dtypep());
        putns(nodep, "");
        ਜੇ (nodep->arrayRange().ranged()) {
            puts("for (int i = 0; i < " + cvt_ਮੰਜ਼ਿਲ_Str(nodep->arrayRange().elements()) + "; ++i) {\n");
            emitTraceInitOne(nodep, enum_ਅੰਕ);
            puts("\n}\n");
        } ਨਹੀਂਤਾਂ {
            emitTraceInitOne(nodep, enum_ਅੰਕ);
            puts("\n");
        }
    }
    ਖਾਲੀ ਦੇਖੋ(AstTraceInc* nodep) override {
        ਜੇ (nodep->declp()->arrayRange().ranged()) {
            // It traces faster if we unroll the loop
            ਲਈ (ਪੂਰਨ i = 0; i < nodep->declp()->arrayRange().elements(); i++) {
                emitTraceChangeOne(nodep, i);
            }
        } ਨਹੀਂਤਾਂ {
            emitTraceChangeOne(nodep, -1);
        }
    }

    ਸਪਸ਼ਟ EmitCTrace(ਤਾਰਕਿਕ slow)
        : m_slow{slow} {
        m_modp = v3Global.rootp()->ਸਿਖਰ_Modulep();
        // Open output file
        open_ਅਗਲਾ_Output_File();
        // Emit functions
        ਲਈ (Ast_ਗੰਢ* nodep = m_modp->stmtsp(); nodep; nodep = nodep->nextp()) {
            ਜੇ (AstCFunc* ਅਚਲ funcp = VN_CAST(nodep, CFunc)) iterateConst(funcp);
        }
        // Close output file
        closeOutputFile();
        ਜੇ (m_slow) m_emitTypesp->finalize();
    }
    ~EmitCTrace() override = ਮੂਲ;

ਜਨਤਕ:
    ਸਥਿਰ std::ਸਦਿਸ਼<AstCFile*> ਮੁੱਖ(ਤਾਰਕਿਕ slow) VL_MT_STABLE {
        EmitCTrace emitCTrace{slow};
        std::ਸਦਿਸ਼<AstCFile*> cfileps = emitCTrace.getAndClearCfileps();
        ਜੇ (slow) {
            ਲਈ (AstCFile* ਅਚਲ cfilep : emitCTrace.m_emitTypesp->getAndClearCfileps()) {
                cfileps.emplace_ਪਿੱਠ(cfilep);
            }
        }
        ਵਾਪਸ cfileps;
    }
};

//######################################################################
// Existing AstCFile emitter

ਵਰਗ EmitCFile final : ਜਨਤਕ EmitCFunc {
    ਸਪਸ਼ਟ EmitCFile(AstCFile* cfilep) {
        openOutputFile(cfilep, "Generated C++");
        iterateConst(cfilep->tblockp());
        closeOutputFile();
    }
    ~EmitCFile() override = ਮੂਲ;

ਜਨਤਕ:
    ਸਥਿਰ ਖਾਲੀ ਮੁੱਖ(AstCFile* cfilep) VL_MT_STABLE {
        ਜੇ (!cfilep->tblockp()) ਵਾਪਸ;
        EmitCFile{cfilep};
    }
};

//######################################################################
// EmitC class functions

ਖਾਲੀ V3EmitC::emitcImp() {
    UINFO(2, __FUNCTION__ << ":");
    std::list<std::ਸਦਿਸ਼<AstCFile*>> cfiles;
    {
        // Make parent module pointers available.
        ਅਚਲ EmitCParentModule emitCParentModule;
        V3ThreadScope threadScope;

        // Process each module in turn
        ਲਈ (ਅਚਲ Ast_ਗੰਢ* nodep = v3Global.rootp()->modulesp(); nodep; nodep = nodep->nextp()) {
            ਜੇ (VN_IS(nodep, Class)) ਜਾਰੀ;  // Imped with ClassPackage
            ਅਚਲ Ast_ਗੰਢ_Module* ਅਚਲ modp = VN_AS(nodep, ਗੰਢ_Module);
            ਜੇ (modp->isConstPool()) ਜਾਰੀ;  // Emitted by V3EmitCConstPool
            cfiles.emplace_ਪਿੱਠ();
            std::ਸਦਿਸ਼<AstCFile*>& slow = cfiles.ਪਿੱਠ();
            threadScope.ਕਤਾਰ_ਵਿੱਚ_ਪਾਓ([modp, &slow] { slow = EmitCImp::ਮੁੱਖ(modp, /* slow: */ ਸੱਚ); });
            cfiles.emplace_ਪਿੱਠ();
            std::ਸਦਿਸ਼<AstCFile*>& fast = cfiles.ਪਿੱਠ();
            threadScope.ਕਤਾਰ_ਵਿੱਚ_ਪਾਓ([modp, &fast] { fast = EmitCImp::ਮੁੱਖ(modp, /* slow: */ ਝੂਠ); });
        }

        // Emit trace routines (currently they can only exist in the top module)
        ਜੇ (v3Global.opt.trace() && !v3Global.opt.lintOnly()) {
            cfiles.emplace_ਪਿੱਠ();
            std::ਸਦਿਸ਼<AstCFile*>& slow = cfiles.ਪਿੱਠ();
            threadScope.ਕਤਾਰ_ਵਿੱਚ_ਪਾਓ([&slow] { slow = EmitCTrace::ਮੁੱਖ(/* slow: */ ਸੱਚ); });
            cfiles.emplace_ਪਿੱਠ();
            std::ਸਦਿਸ਼<AstCFile*>& fast = cfiles.ਪਿੱਠ();
            threadScope.ਕਤਾਰ_ਵਿੱਚ_ਪਾਓ([&fast] { fast = EmitCTrace::ਮੁੱਖ(/* slow: */ ਝੂਠ); });
        }
    }
    // Add files to netlist
    ਲਈ (ਅਚਲ std::ਸਦਿਸ਼<AstCFile*>& cfileps : cfiles) {
        ਲਈ (AstCFile* ਅਚਲ cfilep : cfileps) v3Global.rootp()->ਜੋੜੋ_Filesp(cfilep);
    }
}

ਖਾਲੀ V3EmitC::emitcFiles() {
    UINFO(2, __FUNCTION__ << ":");
    ਲਈ (Ast_ਗੰਢ_File *filep = v3Global.rootp()->filesp(), *nextp; filep; filep = nextp) {
        nextp = VN_AS(filep->nextp(), ਗੰਢ_File);
        ਜੇ (AstCFile* ਅਚਲ cfilep = VN_CAST(filep, CFile)) EmitCFile::ਮੁੱਖ(cfilep);
    }
}
