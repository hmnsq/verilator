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
#ಅನ್ತರ್ಭಾವ "V3EmitCBase.h"
#ಅನ್ತರ್ಭಾವ "V3File.h"
#ಅನ್ತರ್ಭಾವ "V3MemberMap.h"
#ಅನ್ತರ್ಭಾವ "V3UniqueNames.h"

#ಅನ್ತರ್ಭಾವ <algorithm>
#ಅನ್ತರ್ಭಾವ <cstdint>
#ಅನ್ತರ್ಭಾವ <set>
#ಅನ್ತರ್ಭಾವ <string>
#ಅನ್ತರ್ಭಾವ <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Internal EmitC implementation

ವರ್ಗ EmitCHeader final : ಸಾರ್ವಜನಿಕ EmitCBaseVisitorConst {
    V3_Unique_ಹೆಸರುಗಳು m_ಹೆಸರುಗಳು;
    VMemberMap m_memberMap;
    // METHODS

    ವರ್ಗ Cover_ಎಣಿಕೆ_Visitor final : ಸಾರ್ವಜನಿಕ VNVisitorConst {
        ಪೂರ್ಣಾಂಕ m_bins = 0;  // Running total of coverage bins counted so far

        ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು_Cover_Decl* nodep) override {
            // Each module class owns the counters for declarations it emits;
            // duplicate no-inline instances need separate arrays for
            // forcePerInstance reporting.
            ಆದರೆ (!nodep->ದತ್ತಾಂಶ_Decl_Nullp()) m_bins += nodep->ಗಾತ್ರ_2();
        }
        ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು* nodep) override { iterateChildrenConst(nodep); }

    ಸಾರ್ವಜನಿಕ:
        ಸ್ಪಷ್ಟ Cover_ಎಣಿಕೆ_Visitor(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_Module* modp) {
            iterateConst(const_cast<Ast_ಗಂಟು_Module*>(modp));
        }
        ಪೂರ್ಣಾಂಕ bins() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_bins; }
    };

    ಶೂನ್ಯ decorate_ಮೊದಲ(ತಾರ್ಕಿಕ& ಮೊದಲ, ಸ್ಥಿರಾಂಕ string& str) {
        ಆದರೆ (ಮೊದಲ) {
            putsDecoration(ಶೂನ್ಯಸೂಚಕ, str);
            ಮೊದಲ = ಸುಳ್ಳು;
        }
    }
    ಶೂನ್ಯ emitCellDecls(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_Module* modp) {
        ತಾರ್ಕಿಕ ಮೊದಲ = ನಿಜ;
        ಗಾಗಿ (ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* nodep = modp->stmtsp(); nodep; nodep = nodep->nextp()) {
            ಆದರೆ (ಸ್ಥಿರಾಂಕ AstCell* ಸ್ಥಿರಾಂಕ cellp = VN_CAST(nodep, Cell)) {
                ಆದರೆ (cellp->modp()->isConstPool()) ಮುಂದುವರಿ;  // Special emit rules
                decorate_ಮೊದಲ(ಮೊದಲ, "// CELLS\n");
                putns(cellp, EmitCUtil::ಪೂರ್ವಪ್ರತ್ಯಯ_Name_Protect(cellp->modp()) + "* "
                                 + cellp->nameProtect() + ";\n");
            }
        }
    }
    ಶೂನ್ಯ emitDesignVarDecls(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_Module* modp) {
        // V3VariableOrder puts all variables at the front of the list: top level ports, then
        // trivially constructible variables, then the not trivially constructible ones, then
        // those that are not model state
        ಸ್ಥಿರಾಂಕ AstVar* portp = ಶೂನ್ಯಸೂಚಕ;
        ಸ್ಥಿರಾಂಕ AstVar* trivp = ಶೂನ್ಯಸೂಚಕ;
        ಸ್ಥಿರಾಂಕ AstVar* nontp = ಶೂನ್ಯಸೂಚಕ;
        ಗಾತ್ರ_t n_ಪೋರ್ಟ್ = 0;
        ಗಾತ್ರ_t nTriv = 0;
        ಗಾತ್ರ_t nNont = 0;
        ಗಾಗಿ (ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* nodep = modp->stmtsp(); nodep; nodep = nodep->nextp()) {
            ಸ್ಥಿರಾಂಕ AstVar* ಸ್ಥಿರಾಂಕ varp = VN_CAST(nodep, Var);
            ಆದರೆ (!varp) ನಿಲ್ಲಿಸು;
            ಆದರೆ (!varp->isModelState()) {
                // Cache line alignment is only effective on emitted fields
                UASSERT_OBJ(!varp->mtask_Cache_ಸಾಲು_Align(), varp, "Aligned non-field");
            } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (varp->isPrimaryIO()) {
                ಆದರೆ (!portp) portp = varp;
                ++n_ಪೋರ್ಟ್;
            } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (varp->isTriviallyConstructible()) {
                ಆದರೆ (!trivp) trivp = varp;
                ++nTriv;
            } ಇಲ್ಲದಿದ್ದರೆ {
                ಆದರೆ (!nontp) nontp = varp;
                ++nNont;
            }
        }

        ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ emit_And_ಹೆಜ್ಜೆ = [&](ಸ್ಥಿರಾಂಕ AstVar*& varpr) {
            emitVarDecl(varpr);
            varpr = VN_CAST(varpr->nextp(), Var);
        };

        // Emit primary ports
        ಆದರೆ (n_ಪೋರ್ಟ್) {
            putsDecoration(ಶೂನ್ಯಸೂಚಕ, "\n// TOP LEVEL PORTS\n");
            // In anonymous union to prevent the C++ compiler from generating
            // constructor/destructor calls
            puts("union {\nstruct {\n");
            ಗಾಗಿ (ಗಾತ್ರ_t i = 0; i < n_ಪೋರ್ಟ್; ++i) emit_And_ಹೆಜ್ಜೆ(portp);
            puts("} __VprimaryPorts;\n};\n");
        }

        // Emit trivially constructible variables
        ಆದರೆ (nTriv) {
            putsDecoration(ಶೂನ್ಯಸೂಚಕ, "\n// TRIVIALLY CONSTRUCTIBLE STATE\n");
            ಆದರೆ (v3Global.opt.comp_ಎಲ್ಲೆ_Members() == 0) {
                // Anonymous structs disabled, so just emit them directly
                ಗಾಗಿ (ಗಾತ್ರ_t i = 0; i < nTriv; ++i) emit_And_ಹೆಜ್ಜೆ(trivp);
            } ಇಲ್ಲದಿದ್ದರೆ {
                ಸ್ಥಿರಾಂಕ ಗಾತ್ರ_t lim = v3Global.opt.comp_ಎಲ್ಲೆ_Members();
                ಗಾತ್ರ_t anonL3s = 1;
                ಗಾತ್ರ_t anonL2s = 1;
                ಗಾತ್ರ_t anonL1s = 1;
                ಆದರೆ (nTriv > (lim * lim * lim)) {
                    anonL3s = (nTriv + (lim * lim * lim) - 1) / (lim * lim * lim);
                    anonL2s = lim;
                    anonL1s = lim;
                } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (nTriv > (lim * lim)) {
                    anonL2s = (nTriv + (lim * lim) - 1) / (lim * lim);
                    anonL1s = lim;
                } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (nTriv > lim) {
                    anonL1s = (nTriv + lim - 1) / lim;
                }
                ಆದರೆ (anonL1s != 1) {
                    putsDecoration(
                        ಶೂನ್ಯಸೂಚಕ,
                        "// Anonymous structures to workaround compiler member-count bugs\n");
                }
                ಗಾತ್ರ_t nEmitted = 0;
                ಗಾಗಿ (ಗಾತ್ರ_t l3 = 0; l3 < anonL3s && nEmitted < nTriv; ++l3) {
                    ಆದರೆ (anonL3s != 1) puts("struct {\n");
                    ಗಾಗಿ (ಗಾತ್ರ_t l2 = 0; l2 < anonL2s && nEmitted < nTriv; ++l2) {
                        ಆದರೆ (anonL2s != 1) puts("struct {\n");
                        ಗಾಗಿ (ಗಾತ್ರ_t l1 = 0; l1 < anonL1s && nEmitted < nTriv; ++l1) {
                            ಆದರೆ (anonL1s != 1) puts("struct {\n");
                            ಗಾಗಿ (ಗಾತ್ರ_t l0 = 0; l0 < lim && nEmitted < nTriv; ++l0, ++nEmitted) {
                                emit_And_ಹೆಜ್ಜೆ(trivp);
                            }
                            ಆದರೆ (anonL1s != 1) puts("};\n");
                        }
                        ಆದರೆ (anonL2s != 1) puts("};\n");
                    }
                    ಆದರೆ (anonL3s != 1) puts("};\n");
                }
                UASSERT_OBJ(nEmitted == nTriv, modp, "Not all variables emitted");
            }
        }

        // Emit not trivially constructible variables
        ಆದರೆ (nNont) {
            putsDecoration(ಶೂನ್ಯಸೂಚಕ, "\n// NOT TRIVIALLY CONSTRUCTIBLE STATE\n");
            // Classes use real constructors
            ಆದರೆ (VN_IS(modp, Class)) {
                ಗಾಗಿ (ಗಾತ್ರ_t i = 0; i < nNont; ++i) emit_And_ಹೆಜ್ಜೆ(nontp);
                ಹಿಂತಿರುಗಿ;
            }
            // Output under an anonymous union to prevent the C++ compiler from generating
            // constructor/destructor calls for them. They are managed explicitly in ctor/dtor.
            // While 'union { singleVar; }' for each variable would be simpler, it blows up
            // clang's compile time when using pre-compiled headers, joy.
            puts("union {\nstruct {\n");
            ಗಾಗಿ (ಗಾತ್ರ_t i = 0; i < nNont; ++i) emit_And_ಹೆಜ್ಜೆ(nontp);
            puts("} __Vnontrivs;\n};\n");
        }
    }
    ಶೂನ್ಯ emit_ಆಂತರಿಕ_Var_Decls(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_Module* modp) {
        ಆದರೆ (ಸ್ಥಿರಾಂಕ AstClass* ಸ್ಥಿರಾಂಕ classp = VN_CAST(modp, Class)) {
            ಆದರೆ (classp->needRNG()) {
                putsDecoration(ಶೂನ್ಯಸೂಚಕ, "\n// INTERNAL VARIABLES\n");
                puts("VlRNGReseeds __Vm_rng;\n");
            }
        } ಇಲ್ಲದಿದ್ದರೆ {  // not class
            putsDecoration(ಶೂನ್ಯಸೂಚಕ, "\n// INTERNAL VARIABLES\n");
            puts(EmitCUtil::symClassName() + "* vlSymsp;\n");
            puts("const char* vlNamep;\n");
            // Allocate object-local coverage counters only on modules that
            // contain emitted coverage declarations.
            ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ coverBins = Cover_ಎಣಿಕೆ_Visitor{modp}.bins();
            ಆದರೆ (coverBins) {
                puts(v3Global.opt.ಎಳೆಗಳು() > 1 ? "std::atomic<uint32_t>" : "uint32_t");
                puts(" __Vcoverage[");
                puts(std::ಗಮ್ಯ_string(coverBins));
                puts("]{};\n");
            }
        }
    }
    ಶೂನ್ಯ emitParamDecls(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_Module* modp) {
        ತಾರ್ಕಿಕ ಮೊದಲ = ನಿಜ;
        ಗಾಗಿ (Ast_ಗಂಟು* nodep = modp->stmtsp(); nodep; nodep = nodep->nextp()) {
            ಆದರೆ (ಸ್ಥಿರಾಂಕ AstVar* ಸ್ಥಿರಾಂಕ varp = VN_CAST(nodep, Var)) {
                ಆದರೆ (varp->isParam()) {
                    decorate_ಮೊದಲ(ಮೊದಲ, "\n// PARAMETERS\n");
                    putns(varp, "static const ");
                    puts(varp->dtypep()->cType(varp->nameProtect(), ಸುಳ್ಳು, ಸುಳ್ಳು));
                    puts(";\n");
                }
            }
        }
    }
    ಶೂನ್ಯ emitCtorDtorDecls(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_Module* modp) {
        // Classes use CFuncs with isConstructor/isDestructor
        ಆದರೆ (VN_IS(modp, Class)) ಹಿಂತಿರುಗಿ;

        // Proper modules use loose split ctor/dtor and are not copyable/movable
        ಸ್ಥಿರಾಂಕ std::string name = EmitCUtil::ಪೂರ್ವಪ್ರತ್ಯಯ_Name_Protect(modp);
        putsDecoration(ಶೂನ್ಯಸೂಚಕ, "\n// CONSTRUCTORS\n");
        putns(modp, name + "(" + EmitCUtil::symClassName() + "* symsp, const char* namep);\n");
        putns(modp, "~" + name + "();\n");
        putns(modp, "VL_UNCOPYABLE(" + name + ");\n");
        putns(modp, "VL_UNMOVABLE(" + name + ");\n");
    }
    ಶೂನ್ಯ emit_ಆಂತರಿಕ_ವಿಧಾನ_Decls(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_Module* modp) {
        ತಾರ್ಕಿಕ ಮೊದಲ = ನಿಜ;
        ಸ್ಥಿರಾಂಕ string section = "\n// INTERNAL METHODS\n";

        ಆದರೆ (!VN_IS(modp, Class)) {
            decorate_ಮೊದಲ(ಮೊದಲ, section);
            ಆದರೆ (v3Global.opt.coverage()) {
                puts("void " + protect("__Vconfigure") + "(bool first);\n");
            }
        } ಇಲ್ಲದಿದ್ದರೆ {
            decorate_ಮೊದಲ(ಮೊದಲ, section);
            ಸ್ಥಿರಾಂಕ std::string name = V3_ಹೊರಹೋಗು_Formatter::quoteNameControls(
                VIdProtect::protect_ಪದಗಳು_If(modp->prettyName(), v3Global.opt.protectIds()));
            // "override", but don't want clang to start checking them:
            puts("virtual const char* typeName() const { return \""s + name + "\"; }\n");
        }

        ಆದರೆ (v3Global.opt.coverage() && !VN_IS(modp, Class)) {
            decorate_ಮೊದಲ(ಮೊದಲ, section);
            puts("void __vlCoverInsert(");
            puts(v3Global.opt.ಎಳೆಗಳು() > 1 ? "std::atomic<uint32_t>" : "uint32_t");
            puts("* countp, bool enable, bool localCounter, const char* filenamep, int lineno, "
                 "int column,\n");
            puts("const char* hierp, const char* pagep, const char* commentp, const char* "
                 "linescovp,\n");
            puts("const char* fsmVarp, const char* fsmFromp, const char* fsmTop, const char* "
                 "fsmTagp);\n");
        }

        ಆದರೆ (v3Global.opt.coverageToggle() && !VN_IS(modp, Class)) {
            decorate_ಮೊದಲ(ಮೊದಲ, section);
            puts("void __vlCoverToggleInsert(int begin, int end, bool ranged, ");
            puts(v3Global.opt.ಎಳೆಗಳು() > 1 ? "std::atomic<uint32_t>" : "uint32_t");
            puts("* countp, bool enable, bool localCounter, const char* filenamep, int lineno, "
                 "int column,\n");
            puts("const char* hierp, const char* pagep, const char* commentp);\n");
        }

        ಆದರೆ (v3Global.opt.savable()) {
            decorate_ಮೊದಲ(ಮೊದಲ, section);
            puts("void " + protect("__Vserialize") + "(VerilatedSerialize& os);\n");
            puts("void " + protect("__Vdeserialize") + "(VerilatedDeserialize& os);\n");
        }

        // Polymorphic clone for concrete (non-abstract, non-interface) classes
        ಆದರೆ (ಸ್ಥಿರಾಂಕ AstClass* ಸ್ಥಿರಾಂಕ classp = VN_CAST(modp, Class)) {
            ಆದರೆ (!classp->isInterfaceClass() && !classp->isVirtual()) {
                decorate_ಮೊದಲ(ಮೊದಲ, section);
                ಬಳಸಿ EmbeddedCovergroupVar = std::pair<ಸ್ಥಿರಾಂಕ AstClass*, ಸ್ಥಿರಾಂಕ AstVar*>;
                std::ಸದಿಶ<EmbeddedCovergroupVar> embeddedCovergroupVars;
                ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ has_Enclosing_ಹಿಂಭಾಗ_ಸೂಚಕ = [](ಸ್ಥಿರಾಂಕ AstClass* covergroupp) {
                    ಹಿಂತಿರುಗಿ covergroupp->exists([](ಸ್ಥಿರಾಂಕ AstVar* ಸ್ಥಿರಾಂಕ varp) {
                        ಸ್ಥಿರಾಂಕ AstClassRefDType* ಸ್ಥಿರಾಂಕ refp
                            = VN_CAST(varp->dtypep()->skipRefp(), ClassRefDType);
                        ಹಿಂತಿರುಗಿ refp && refp->raw_ಸೂಚಕ();
                    });
                };
                const_cast<AstClass*>(classp)->foreachMember(
                    [&](AstClass* ಸ್ಥಿರಾಂಕ memberClassp, AstVar* ಸ್ಥಿರಾಂಕ varp) {
                        ಸ್ಥಿರಾಂಕ AstClassRefDType* ಸ್ಥಿರಾಂಕ refp
                            = VN_CAST(varp->dtypep()->skipRefp(), ClassRefDType);
                        ಆದರೆ (refp && refp->classp()->isCovergroup()
                            && has_Enclosing_ಹಿಂಭಾಗ_ಸೂಚಕ(refp->classp())) {
                            embeddedCovergroupVars.emplace_ಹಿಂಭಾಗ(memberClassp, varp);
                        }
                    });
                ಸ್ಥಿರಾಂಕ string className = EmitCUtil::ಪೂರ್ವಪ್ರತ್ಯಯ_Name_Protect(classp);
                ಆದರೆ (embeddedCovergroupVars.ಖಾಲಿಗಳು() && !classp->hasRandVarsUpdate()) {
                    putns(classp,
                          "VlClass* clone() const { return new " + className + "(*this); }\n");
                } ಇಲ್ಲದಿದ್ದರೆ {
                    putns(classp, "VlClass* clone() const { " + className + "* const clonep = new "
                                      + className + "(*this); ");
                    ಆದರೆ (classp->hasRandVarsUpdate()) {
                        ಸ್ಥಿರಾಂಕ string updateName = "__VnoInFunc___VupdateRandVars";
                        AstCFunc* ಸ್ಥಿರಾಂಕ updatep
                            = VN_AS(m_memberMap.ಕಂಡುಹಿಡಿ_Member(classp, updateName), CFunc);
                        UASSERT_OBJ(updatep, classp, "Missing updateRandVars method");
                        puts("clonep->" + updatep->nameProtect() + "();\n");
                    }
                    ಗಾಗಿ (ಸ್ಥಿರಾಂಕ EmbeddedCovergroupVar& ಅಂಶ_2 : embeddedCovergroupVars) {
                        puts("clonep->" + EmitCUtil::ಪೂರ್ವಪ್ರತ್ಯಯ_Name_Protect(ಅಂಶ_2.ಮೊದಲ)
                             + "::" + ಅಂಶ_2.ಎರಡನೇ->nameProtect() + " = VlNull{}; ");
                    }
                    puts("return clonep; }\n");
                }
            }
        }
    }
    ಶೂನ್ಯ emitEnums(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_Module* modp) {
        ತಾರ್ಕಿಕ ಮೊದಲ = ನಿಜ;
        ಗಾಗಿ (ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* nodep = modp->stmtsp(); nodep; nodep = nodep->nextp()) {
            ಸ್ಥಿರಾಂಕ AstTypedef* ಸ್ಥಿರಾಂಕ tdefp = VN_CAST(nodep, Typedef);
            ಆದರೆ (!tdefp) ಮುಂದುವರಿ;
            ಆದರೆ (!tdefp->attr_ಬಹಿರಂಗ()) ಮುಂದುವರಿ;
            ಸ್ಥಿರಾಂಕ AstEnumDType* ಸ್ಥಿರಾಂಕ edtypep
                = VN_CAST(tdefp->dtypep()->skip_Ref_ಗಮ್ಯ_Enump(), EnumDType);
            ಆದರೆ (!edtypep) ಮುಂದುವರಿ;
            decorate_ಮೊದಲ(ಮೊದಲ, "\n// ENUMS (that were declared public)\n");
            ಆದರೆ (edtypep->ಅಗಲ() > 64) {
                putsDecoration(tdefp,
                               "// enum " + tdefp->nameProtect() + " ignored: Too wide for C++\n");
            } ಇಲ್ಲದಿದ್ದರೆ {
                putns(tdefp, "enum " + tdefp->name() + " {\n");
                ಗಾಗಿ (ಸ್ಥಿರಾಂಕ Ast_Enum_ಅಂಶ* itemp = edtypep->itemsp(); itemp;
                     itemp = VN_AS(itemp->nextp(), Enum_ಅಂಶ)) {
                    ಆದರೆ (ಸ್ಥಿರಾಂಕ AstConst* ಸ್ಥಿರಾಂಕ constp = VN_CAST(itemp->valuep(), Const)) {
                        ಆದರೆ (constp->ಅಂಕೆ().isFourState()) {
                            putns(itemp, "// " + itemp->nameProtect() + " is four-state\n");
                            ಮುಂದುವರಿ;
                        }
                    }
                    putns(itemp, itemp->nameProtect());
                    puts(" = ");
                    putns(itemp, VN_AS(itemp->valuep(), Const)->ಅಂಕೆ().emitC());
                    ಆದರೆ (VN_IS(itemp->nextp(), Enum_ಅಂಶ)) puts(",");
                    puts("\n");
                }
                puts("};\n");
            }
        }
    }
    ಶೂನ್ಯ emitStructDecl(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_Module* modp, Ast_ಗಂಟು_UOr_Struct_DType* sdtypep,
                        std::ಗಣ<Ast_ಗಂಟು_UOr_Struct_DType*>& emitted) {
        ಆದರೆ (emitted.ಎಣಿಕೆ_2(sdtypep) > 0) ಹಿಂತಿರುಗಿ;
        emitted.ಸೇರಿಸು(sdtypep);
        ಗಾಗಿ (AstMemberDType* itemp = sdtypep->membersp(); itemp;
             itemp = VN_AS(itemp->nextp(), MemberDType)) {
            Ast_ಗಂಟು_UOr_Struct_DType* ಸ್ಥಿರಾಂಕ subp = itemp->get_ಮಗು_Structp();
            ಆದರೆ (subp && (!subp->packed() || sdtypep->packed())) {
                // Recurse if it belongs to the current module
                ಆದರೆ (subp->classOrPackagep() == modp) {
                    emitStructDecl(modp, subp, emitted);
                    puts("\n");
                }
            }
        }
        ಆದರೆ (sdtypep->packed()) {
            emitPackedUOrSBody(sdtypep);
        } ಇಲ್ಲದಿದ್ದರೆ {
            emitUnpackedUOrSBody(sdtypep);
        }
    }
    ಎಣಿಕೆ ವರ್ಗ AttributeType : uint8_t { ಅಗಲ_2, DIMENSION };
    // Get member attribute based on type
    ಪೂರ್ಣಾಂಕ get_ಗಂಟು_Attribute(ಸ್ಥಿರಾಂಕ AstMemberDType* itemp, AttributeType type) {
        ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ isArrayType = itemp->dtypep()->isNonPackedArray();
        ಆಯ್ಕೆ (type) {
        ಸ್ಥಿತಿ AttributeType::ಅಗಲ_2: {
            ಆದರೆ (isArrayType) {
                // For arrays, get innermost element width
                ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_DType* dtype = itemp->dtypep();
                ಆಗ (dtype->subDTypep()) dtype = dtype->subDTypep();
                ಹಿಂತಿರುಗಿ dtype->ಅಗಲ();
            }
            ಹಿಂತಿರುಗಿ itemp->ಅಗಲ();
        }
        ಸ್ಥಿತಿ AttributeType::DIMENSION: {
            // Return array dimension or 0 for non-arrays
            ಹಿಂತಿರುಗಿ isArrayType ? itemp->dtypep()->dimensions(ನಿಜ).ಎರಡನೇ : 0;
        }
        ಪೂರ್ವನಿಯೋಜಿತ: {
            ಹಿಂತಿರುಗಿ 0;
        }
        }
    }
    ಮಾದರಿ <AttributeType T>
    ಶೂನ್ಯ emit_Member_ಸದಿಶ(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_UOr_Struct_DType* sdtypep) {
        puts("return {");
        ತಾರ್ಕಿಕ needComma = ಸುಳ್ಳು;
        ಗಾಗಿ (ಸ್ಥಿರಾಂಕ AstMemberDType* itemp = sdtypep->membersp(); itemp;
             itemp = VN_AS(itemp->nextp(), MemberDType)) {
            ಆದರೆ (!itemp->isConstrainedRand()) ಮುಂದುವರಿ;
            // Comma handling: add before element except first
            ಆದರೆ (needComma) puts(",\n");
            putns(itemp, std::ಗಮ್ಯ_string(get_ಗಂಟು_Attribute(itemp, T)));
            needComma = ನಿಜ;
        }
        puts("};\n}\n");
    }
    ಶೂನ್ಯ emitUnpackedUOrSBody(Ast_ಗಂಟು_UOr_Struct_DType* sdtypep) {
        putns(sdtypep, sdtypep->verilogKwd());  // "struct"/"union"
        puts(" " + EmitCUtil::ಪೂರ್ವಪ್ರತ್ಯಯ_Name_Protect(sdtypep) + " {\n");
        ಗಾಗಿ (ಸ್ಥಿರಾಂಕ AstMemberDType* itemp = sdtypep->membersp(); itemp;
             itemp = VN_AS(itemp->nextp(), MemberDType)) {
            putns(itemp, itemp->dtypep()->cType(itemp->nameProtect(), ಸುಳ್ಳು, ಸುಳ್ಳು));
            puts(";\n");
        }

        // Three helper functions for struct constrained randomization:
        // - memberNames: Get member names
        // - getMembers: Access member references
        // - memberIndices: Retrieve member indices
        // - memberWidth: Retrieve member width
        // - memberDimension: Retrieve member dimension
        ಆದರೆ (sdtypep->isConstrainedRand()) {
            ತಾರ್ಕಿಕ needComma = ಸುಳ್ಳು;
            putns(sdtypep, "\nstd::vector<std::string> memberNames(void) const {\n");
            puts("return {");
            ಗಾಗಿ (ಸ್ಥಿರಾಂಕ AstMemberDType* itemp = sdtypep->membersp(); itemp;
                 itemp = VN_AS(itemp->nextp(), MemberDType)) {
                ಆದರೆ (!itemp->isConstrainedRand()) ಮುಂದುವರಿ;
                ಆದರೆ (needComma) puts(",\n");
                putns(itemp, "\"" + itemp->shortName() + "\"");
                needComma = ನಿಜ;
            }
            puts("};\n}\n");

            putns(sdtypep, "\nstd::vector<int> memberWidth(void) const {\n");
            emit_Member_ಸದಿಶ<AttributeType::ಅಗಲ_2>(sdtypep);

            putns(sdtypep, "\nstd::vector<int> memberDimension(void) const {\n");
            emit_Member_ಸದಿಶ<AttributeType::DIMENSION>(sdtypep);

            needComma = ಸುಳ್ಳು;
            putns(sdtypep, "\nauto memberIndices(void) const {\n");
            puts("return std::index_sequence_for<");
            ಗಾಗಿ (ಸ್ಥಿರಾಂಕ AstMemberDType* itemp = sdtypep->membersp(); itemp;
                 itemp = VN_AS(itemp->nextp(), MemberDType)) {
                ಆದರೆ (!itemp->isConstrainedRand()) ಮುಂದುವರಿ;
                ಆದರೆ (needComma) puts(",\n");
                putns(itemp, itemp->dtypep()->cType("", ಸುಳ್ಳು, ಸುಳ್ಳು));
                needComma = ನಿಜ;
            }
            puts(">{};\n}\n");

            needComma = ಸುಳ್ಳು;
            putns(sdtypep, "\ntemplate <typename T>");
            putns(sdtypep, "\nauto getMembers(T& obj) {\n");
            puts("return std::tie(");
            ಗಾಗಿ (ಸ್ಥಿರಾಂಕ AstMemberDType* itemp = sdtypep->membersp(); itemp;
                 itemp = VN_AS(itemp->nextp(), MemberDType)) {
                ಆದರೆ (!itemp->isConstrainedRand()) ಮುಂದುವರಿ;
                ಆದರೆ (needComma) puts(",\n");
                putns(itemp, "obj." + itemp->nameProtect());
                needComma = ನಿಜ;
            }
            puts(");\n}\n");
        }
        putns(sdtypep, "\nbool operator==(const " + EmitCUtil::ಪೂರ್ವಪ್ರತ್ಯಯ_Name_Protect(sdtypep)
                           + "& rhs) const {\n");
        puts("return ");
        ಗಾಗಿ (ಸ್ಥಿರಾಂಕ AstMemberDType* itemp = sdtypep->membersp(); itemp;
             itemp = VN_AS(itemp->nextp(), MemberDType)) {
            ಆದರೆ (itemp != sdtypep->membersp()) puts("\n    && ");
            putns(itemp, itemp->nameProtect() + " == " + "rhs." + itemp->nameProtect());
        }
        puts(";\n");
        puts("}\n");
        putns(sdtypep, "bool operator!=(const " + EmitCUtil::ಪೂರ್ವಪ್ರತ್ಯಯ_Name_Protect(sdtypep)
                           + "& rhs) const {\n");
        puts("return !(*this == rhs);\n}\n");
        putns(sdtypep, "\nbool operator<(const " + EmitCUtil::ಪೂರ್ವಪ್ರತ್ಯಯ_Name_Protect(sdtypep)
                           + "& rhs) const {\n");
        ಗಾಗಿ (ಸ್ಥಿರಾಂಕ AstMemberDType* itemp = sdtypep->membersp(); itemp;
             itemp = VN_AS(itemp->nextp(), MemberDType)) {
            putns(itemp, "if (" + itemp->nameProtect() + " < rhs." + itemp->nameProtect()
                             + ") return true;\n");
            putns(itemp, "if (rhs." + itemp->nameProtect() + " < " + itemp->nameProtect()
                             + ") return false;\n");
        }
        puts("return false;\n");
        puts("}\n");
        puts("};\n");
        puts("template <>\n");
        putns(sdtypep, "struct VlIsCustomStruct<" + EmitCUtil::ಪೂರ್ವಪ್ರತ್ಯಯ_Name_Protect(sdtypep)
                           + "> : public std::true_type {};\n");
    }

    // getfunc: VL_ASSIGNSEL_XX(rbits, obits, off, lhsdata, rhsdata);
    // !getfunc: VL_SELASSIGN_XX(rbits, obits, lhsdata, rhsdata, off);
    ಶೂನ್ಯ emit_Vl_ನಿಯೋಜಿಸು(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_DType* ಸ್ಥಿರಾಂಕ lhstype, ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_DType* rhstype,
                      ಸ್ಥಿರಾಂಕ std::string& off, ಸ್ಥಿರಾಂಕ std::string& lhsdata,
                      ಸ್ಥಿರಾಂಕ std::string& rhsdata, ತಾರ್ಕಿಕ getfunc) {
        puts(getfunc ? "VL_ASSIGNSEL_" : "VL_SELASSIGN_");
        puts(lhstype->charIQWN());
        puts(rhstype->charIQWN());
        puts("(" + std::ಗಮ್ಯ_string(lhstype->ಅಗಲ()) + ", ");  // LHS width
        ಆದರೆ (getfunc) {
            puts(std::ಗಮ್ಯ_string(rhstype->ಅಗಲ()) + ", ");  // Number of copy bits
            puts(off + ", ");  // LHS offset
        } ಇಲ್ಲದಿದ್ದರೆ {
            // Number of copy bits. Use widthTototalBytes to
            // make VL_SELASSIGN_XX clear upper unused bits for us.
            // puts(std::to_string(lhstype->width()) + ", ");
            puts(std::ಗಮ್ಯ_string(lhstype->ಅಗಲ_ಒಟ್ಟು_ಬೈಟುಗಳು() * 8) + ", ");
        }
        puts(lhsdata + ", ");  // LHS data
        puts(rhsdata);  // RHS data
        ಆದರೆ (!getfunc) {
            puts(", " + off);  // RHS offset
        }
        puts(");\n");
    }

    // `retOrArg` should be prefixed by `&` or suffixed by `.data()` depending on its type
    ಶೂನ್ಯ emitPackedMember(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_DType* ಪೋಷಕ_Dtypep, ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_DType* dtypep,
                          ಸ್ಥಿರಾಂಕ std::string& fieldname, ಸ್ಥಿರಾಂಕ std::string& ಸ್ಥಾನಾಂತರ, ತಾರ್ಕಿಕ getfunc,
                          ಸ್ಥಿರಾಂಕ std::string& retOrArg) {
        dtypep = dtypep->skipRefp();
        ಆದರೆ (ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ* adtypep = VN_CAST(dtypep, PackArrayDType)) {
            ಸ್ಥಿರಾಂಕ std::string ಸೂಚ್ಯಂಕ = m_ಹೆಸರುಗಳು.get("__Vi");
            puts("for (int " + ಸೂಚ್ಯಂಕ + " = 0; " + ಸೂಚ್ಯಂಕ + " < "
                 + std::ಗಮ್ಯ_string(adtypep->elementsConst()) + "; ++" + ಸೂಚ್ಯಂಕ + ") {\n");

            ಸ್ಥಿರಾಂಕ std::string ಸ್ಥಾನಾಂತರ_ಒಳ_Loop
                = ಸ್ಥಾನಾಂತರ + " + " + ಸೂಚ್ಯಂಕ + " * " + std::ಗಮ್ಯ_string(adtypep->subDTypep()->ಅಗಲ());
            ಸ್ಥಿರಾಂಕ std::string newName = fieldname + "[" + ಸೂಚ್ಯಂಕ + "]";
            emitPackedMember(ಪೋಷಕ_Dtypep, adtypep->subDTypep(), newName, ಸ್ಥಾನಾಂತರ_ಒಳ_Loop, getfunc,
                             retOrArg);
            puts("}\n");
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (VN_IS(dtypep, ಗಂಟು_UOr_Struct_DType)) {
            ಸ್ಥಿರಾಂಕ std::string tmp = m_ಹೆಸರುಗಳು.get("__Vtmp");
            ಆದರೆ (getfunc) {  // Emit `get` func;
                // auto __tmp = field.get();
                puts("auto " + tmp + " = " + fieldname + ".get();\n");
                // VL_ASSIGNSEL_XX(rbits, obits, lsb, lhsdata, rhsdata);
                emit_Vl_ನಿಯೋಜಿಸು(ಪೋಷಕ_Dtypep, dtypep, ಸ್ಥಾನಾಂತರ, retOrArg, tmp, getfunc);
            } ಇಲ್ಲದಿದ್ದರೆ {  // Emit `set` func
                ಸ್ಥಿರಾಂಕ std::string tmptype = AstCDType::type_ಗಮ್ಯ_Hold(dtypep->ಅಗಲ());
                // type tmp;
                puts(tmptype + " " + tmp + ";\n");
                // VL_SELASSIGN_XX(rbits, obits, lhsdata, rhsdata, roffset);
                emit_Vl_ನಿಯೋಜಿಸು(dtypep, ಪೋಷಕ_Dtypep, ಸ್ಥಾನಾಂತರ, tmp, retOrArg, getfunc);
                // field.set(__tmp);
                puts(fieldname + ".set(" + tmp + ");\n");
            }
        } ಇಲ್ಲದಿದ್ದರೆ {
            UASSERT_OBJ(VN_IS(dtypep, EnumDType) || VN_IS(dtypep, BasicDType), dtypep,
                        "Unsupported type in packed struct or union");
            ಆದರೆ (getfunc) {  // Emit `get` func;
                // VL_ASSIGNSEL_XX(rbits, obits, lsb, lhsdata, rhsdata);
                emit_Vl_ನಿಯೋಜಿಸು(ಪೋಷಕ_Dtypep, dtypep, ಸ್ಥಾನಾಂತರ, retOrArg, fieldname, getfunc);
            } ಇಲ್ಲದಿದ್ದರೆ {  // Emit `set` func
                // VL_SELASSIGN_XX(rbits, obits, lhsdata, rhsdata, roffset);
                emit_Vl_ನಿಯೋಜಿಸು(dtypep, ಪೋಷಕ_Dtypep, ಸ್ಥಾನಾಂತರ, fieldname, retOrArg, getfunc);
            }
        }
    }
    ಶೂನ್ಯ emitPackedUOrSBody(Ast_ಗಂಟು_UOr_Struct_DType* sdtypep) {
        putns(sdtypep, sdtypep->verilogKwd());  // "struct"/"union"
        puts(" " + EmitCUtil::ಪೂರ್ವಪ್ರತ್ಯಯ_Name_Protect(sdtypep) + " {\n");

        AstMemberDType* itemp;
        AstMemberDType* ಕೊನೆಯ_Itemp;
        ಸ್ಥಿರಾಂಕ AstMemberDType* witemp = ಶೂನ್ಯಸೂಚಕ;
        // LSB is first field in C, so loop backwards
        ಗಾಗಿ (ಕೊನೆಯ_Itemp = sdtypep->membersp(); ಕೊನೆಯ_Itemp && ಕೊನೆಯ_Itemp->nextp();
             ಕೊನೆಯ_Itemp = VN_AS(ಕೊನೆಯ_Itemp->nextp(), MemberDType)) {
            ಆದರೆ (ಕೊನೆಯ_Itemp->ಅಗಲ() == sdtypep->ಅಗಲ()) witemp = ಕೊನೆಯ_Itemp;
        }
        ಗಾಗಿ (itemp = ಕೊನೆಯ_Itemp; itemp; itemp = VN_CAST(itemp->backp(), MemberDType)) {
            putns(itemp, itemp->dtypep()->cType(itemp->nameProtect(), ಸುಳ್ಳು, ಸುಳ್ಳು, ನಿಜ));
            puts(";\n");
        }

        ಸ್ಥಿರಾಂಕ std::string retArgName = m_ಹೆಸರುಗಳು.get("__v");
        ಸ್ಥಿರಾಂಕ std::string retArgType = AstCDType::type_ಗಮ್ಯ_Hold(sdtypep->ಅಗಲ());

        // Emit `get` member function
        puts(retArgType + " get() const {\n");
        puts(retArgType + " " + retArgName + ";\n");
        ಆದರೆ (VN_IS(sdtypep, StructDType)) {
            ಗಾಗಿ (itemp = ಕೊನೆಯ_Itemp; itemp; itemp = VN_CAST(itemp->backp(), MemberDType)) {
                emitPackedMember(sdtypep, itemp->dtypep(), itemp->nameProtect(),
                                 std::ಗಮ್ಯ_string(itemp->lsb()), /*getfunc=*/ನಿಜ, retArgName);
            }
        } ಇಲ್ಲದಿದ್ದರೆ {
            // We only need to fill the widest field of union
            emitPackedMember(sdtypep, witemp->dtypep(), witemp->nameProtect(),
                             std::ಗಮ್ಯ_string(witemp->lsb()), /*getfunc=*/ನಿಜ, retArgName);
        }
        puts("return " + retArgName + ";\n");
        puts("}\n");

        // Emit `set` member function
        puts("void set(const " + retArgType + "& " + retArgName + ") {\n");
        ಆದರೆ (VN_IS(sdtypep, StructDType)) {
            ಗಾಗಿ (itemp = ಕೊನೆಯ_Itemp; itemp; itemp = VN_CAST(itemp->backp(), MemberDType)) {
                emitPackedMember(sdtypep, itemp->dtypep(), itemp->nameProtect(),
                                 std::ಗಮ್ಯ_string(itemp->lsb()), /*getfunc=*/ಸುಳ್ಳು, retArgName);
            }
        } ಇಲ್ಲದಿದ್ದರೆ {
            // We only need to fill the widest field of union
            emitPackedMember(sdtypep, witemp->dtypep(), witemp->nameProtect(),
                             std::ಗಮ್ಯ_string(witemp->lsb()), /*getfunc=*/ಸುಳ್ಳು, retArgName);
        }

        puts("}\n");

        puts("};\n");
        m_ಹೆಸರುಗಳು.reset();
    }
    ಶೂನ್ಯ emitStructs(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_Module* modp) {
        // Track structs that've been emitted already
        std::ಗಣ<Ast_ಗಂಟು_UOr_Struct_DType*> emitted;
        ಗಾಗಿ (ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* nodep = modp->stmtsp(); nodep; nodep = nodep->nextp()) {
            ಸ್ಥಿರಾಂಕ AstTypedef* ಸ್ಥಿರಾಂಕ tdefp = VN_CAST(nodep, Typedef);
            ಆದರೆ (!tdefp) ಮುಂದುವರಿ;
            Ast_ಗಂಟು_UOr_Struct_DType* ಸ್ಥಿರಾಂಕ sdtypep
                = VN_CAST(tdefp->dtypep()->skip_Ref_ಗಮ್ಯ_Enump(), ಗಂಟು_UOr_Struct_DType);
            ಆದರೆ (!sdtypep) ಮುಂದುವರಿ;
            // V3Inline can copy one module into several parents. Only the module that
            // declares the struct emits it, so C++ sees only one definition.
            ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ declared_ಒಳ_Other_Module = sdtypep->classOrPackagep() != modp;
            ಆದರೆ (declared_ಒಳ_Other_Module) ಮುಂದುವರಿ;
            emitStructDecl(modp, sdtypep, emitted);
        }
    }
    ಶೂನ್ಯ emitFuncDecls(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_Module* modp, ತಾರ್ಕಿಕ ಒಳ_Class_Body) {
        std::ಸದಿಶ<ಸ್ಥಿರಾಂಕ AstCFunc*> funcsp;

        ಗಾಗಿ (ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* nodep = modp->stmtsp(); nodep; nodep = nodep->nextp()) {
            ಆದರೆ (ಸ್ಥಿರಾಂಕ AstCFunc* ಸ್ಥಿರಾಂಕ funcp = VN_CAST(nodep, CFunc)) {
                ಆದರೆ (funcp->dpiImportPrototype())  // Declared in __Dpi.h
                    ಮುಂದುವರಿ;
                ಆದರೆ (funcp->dpiExportDispatcher())  // Declared in __Dpi.h
                    ಮುಂದುವರಿ;
                ಆದರೆ (funcp->is_ವಿಧಾನ() != ಒಳ_Class_Body)  // Only methods go inside class
                    ಮುಂದುವರಿ;
                ಆದರೆ (funcp->is_ವಿಧಾನ() && funcp->isLoose())  // Loose methods are declared lazily
                    ಮುಂದುವರಿ;
                funcsp.ತಳ್ಳು_ಹಿಂಭಾಗ(funcp);
            }
        }

        std::stable_ವಿಂಗಡಿಸು(funcsp.begin(), funcsp.ಕೊನೆ(), [](ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* ap, ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* bp) {
            ಹಿಂತಿರುಗಿ ap->name() < bp->name();
        });

        ಗಾಗಿ (ಸ್ಥಿರಾಂಕ AstCFunc* ಸ್ಥಿರಾಂಕ funcp : funcsp) {
            ಆದರೆ (ಒಳ_Class_Body) ofp()->puts_ಖಾಸಗಿ(funcp->decl_ಖಾಸಗಿ());
            emitCFuncDecl(funcp, modp);
        }
    }
    ಶೂನ್ಯ emitAll(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_Module* modp) {
        // Include files required by this AstNodeModule
        ಆದರೆ (ಸ್ಥಿರಾಂಕ AstClass* ಸ್ಥಿರಾಂಕ classp = VN_CAST(modp, Class)) {
            ಗಾಗಿ (ಸ್ಥಿರಾಂಕ AstClassExtends* extp = classp->extendsp(); extp;
                 extp = VN_AS(extp->nextp(), ClassExtends)) {
                putns(extp, "#include \""
                                + EmitCUtil::ಪೂರ್ವಪ್ರತ್ಯಯ_Name_Protect(extp->classp()->classOrPackagep())
                                + ".h\"\n");
            }
        }

        // Forward declarations required by this AstNodeModule
        puts("\nclass " + EmitCUtil::symClassName() + ";\n");

        // From `systemc_header
        emitSystemCSection(modp, VSystemCSectionType::HDR);

        emitStructs(modp);

        // Open class body {{{
        puts("\n");
        putns(modp, "class ");
        ಆದರೆ (!VN_IS(modp, Class)) puts("alignas(VL_CACHE_LINE_BYTES) ");
        puts(EmitCUtil::ಪೂರ್ವಪ್ರತ್ಯಯ_Name_Protect(modp));
        ಆದರೆ (ಸ್ಥಿರಾಂಕ AstClass* ಸ್ಥಿರಾಂಕ classp = VN_CAST(modp, Class)) {
            puts(" : ");
            ಆದರೆ (classp->extendsp()) {
                ತಾರ್ಕಿಕ needComma = ಸುಳ್ಳು;
                ಗಾಗಿ (ಸ್ಥಿರಾಂಕ AstClassExtends* extp = classp->extendsp(); extp;
                     extp = VN_AS(extp->nextp(), ClassExtends)) {
                    ಆದರೆ (needComma) puts(", ");
                    // Use virtual only for interfaces for class inheritance
                    // (extends)
                    puts(extp->classp()->use_Virtual_ಬಹಿರಂಗ() ? "virtual public " : "public ");
                    putns(extp, EmitCUtil::ಪೂರ್ವಪ್ರತ್ಯಯ_Name_Protect(extp->classp()));
                    needComma = ನಿಜ;
                }
            } ಇಲ್ಲದಿದ್ದರೆ {
                puts("public virtual VlClass");
            }
        } ಇಲ್ಲದಿದ್ದರೆ {
            puts(" final");
        }
        puts(" {\n");
        ofp()->reset_ಖಾಸಗಿ();
        ofp()->puts_ಖಾಸಗಿ(ಸುಳ್ಳು);  // public:

        // Emit all class body contents
        emitCellDecls(modp);
        emitEnums(modp);
        emitDesignVarDecls(modp);
        emit_ಆಂತರಿಕ_Var_Decls(modp);
        emitParamDecls(modp);
        emitCtorDtorDecls(modp);
        emit_ಆಂತರಿಕ_ವಿಧಾನ_Decls(modp);
        emitFuncDecls(modp, /* inClassBody: */ ನಿಜ);

        // From `systemc_interface
        emitSystemCSection(modp, VSystemCSectionType::INT);

        // Close class body
        puts("};\n");
        // }}}

        // Emit out of class function declarations
        puts("\n");
        emitFuncDecls(modp, /* inClassBody: */ ಸುಳ್ಳು);
        emitSystemCSection(modp, VSystemCSectionType::HDR_POST);
    }

    // VISITORS
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು* nodep) override { nodep->v3fatalSrc("Unused"); }  // LCOV_EXCL_LINE

    ಸ್ಪಷ್ಟ EmitCHeader(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_Module* modp) {
        UINFO(5, "  Emitting header for " << EmitCUtil::ಪೂರ್ವಪ್ರತ್ಯಯ_Name_Protect(modp));

        // Open output file
        open_New_Output_ಶೀರ್ಷಿಕೆ_File(EmitCUtil::ಪೂರ್ವಪ್ರತ್ಯಯ_Name_Protect(modp), "Design internal header");
        puts("// See " + EmitCUtil::ತುದಿ_Class_Name() + ".h for the primary calling header\n");

        ofp()->putsGuard();

        // Include files
        puts("\n");
        ofp()->puts_Int_ತುದಿ_Include();
        puts("#include \"verilated.h\"\n");
        ಆದರೆ (modp->is_ತುದಿ() && optSystemC()) puts("#include \"verilated_sc.h\"\n");
        ಆದರೆ (v3Global.opt.mtasks()) puts("#include \"verilated_threads.h\"\n");
        ಆದರೆ (v3Global.opt.savable()) puts("#include \"verilated_save.h\"\n");
        ಆದರೆ (v3Global.opt.coverage()) puts("#include \"verilated_cov.h\"\n");
        ಆದರೆ (v3Global.opt.coverage() || v3Global.useCovergroup())
            puts("#include \"verilated_covergroup.h\"\n");
        ಆದರೆ (v3Global.ಬಳಸುತ್ತದೆ_Timing()) puts("#include \"verilated_timing.h\"\n");
        ಆದರೆ (v3Global.useRandomizeMethods()) puts("#include \"verilated_random.h\"\n");
        ಆದರೆ (v3Global.ಬಳಸುತ್ತದೆ_Force()) puts("#include \"verilated_force.h\"\n");

        std::ಗಣ<string> cuse_ಗಣ;
        ಸ್ವಯಂಚಾಲಿತ ಕೂಡಿಸು_ಗಮ್ಯ_cuse_ಗಣ = [&](string s) { cuse_ಗಣ.ಸೇರಿಸು(s); };

        forModCUse(modp, VUseType::INT_FWD_CLASS | VUseType::INT_INCLUDE, ಕೂಡಿಸು_ಗಮ್ಯ_cuse_ಗಣ);
        ಆದರೆ (ಸ್ಥಿರಾಂಕ AstClassPackage* ಸ್ಥಿರಾಂಕ packagep = VN_CAST(modp, ClassPackage)) {
            forModCUse(packagep->classp(), VUseType::INT_INCLUDE | VUseType::INT_FWD_CLASS,
                       ಕೂಡಿಸು_ಗಮ್ಯ_cuse_ಗಣ);
        }

        ಗಾಗಿ (ಸ್ಥಿರಾಂಕ string& s : cuse_ಗಣ) puts(s);
        puts("\n");

        emitAll(modp);

        ಆದರೆ (ಸ್ಥಿರಾಂಕ AstClassPackage* ಸ್ಥಿರಾಂಕ packagep = VN_CAST(modp, ClassPackage)) {
            // Put the non-static class implementation in same h file for speed
            emitAll(packagep->classp());
        }

        ofp()->puts_ಕೊನೆ_Guard();

        // Close output file
        closeOutputFile();
    }
    ~EmitCHeader() override = ಪೂರ್ವನಿಯೋಜಿತ;

ಸಾರ್ವಜನಿಕ:
    ಸ್ಥಿರ ಶೂನ್ಯ ಮುಖ್ಯ(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_Module* modp) { ಸ್ಥಿರಾಂಕ EmitCHeader emitCHeader{modp}; }
};

//######################################################################
// EmitC class functions

ಶೂನ್ಯ V3EmitC::emitc_ಶೀರ್ಷಿಕೆಗಳು() {
    UINFO(2, __FUNCTION__ << ":");

    // Process each module in turn
    ಗಾಗಿ (ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* nodep = v3Global.rootp()->modulesp(); nodep; nodep = nodep->nextp()) {
        ಆದರೆ (VN_IS(nodep, Class)) ಮುಂದುವರಿ;  // Declared with the ClassPackage
        ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_Module* ಸ್ಥಿರಾಂಕ modp = VN_AS(nodep, ಗಂಟು_Module);
        ಆದರೆ (modp->isConstPool()) ಮುಂದುವರಿ;  // Emitted by V3EmitCConstPool
        EmitCHeader::ಮುಖ್ಯ(modp);
    }
}
