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
#ਅਨ੍ਤਰ੍ਭਾਵ "V3EmitCBase.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3File.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3MemberMap.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3UniqueNames.h"

#ਅਨ੍ਤਰ੍ਭਾਵ <algorithm>
#ਅਨ੍ਤਰ੍ਭਾਵ <cstdint>
#ਅਨ੍ਤਰ੍ਭਾਵ <set>
#ਅਨ੍ਤਰ੍ਭਾਵ <string>
#ਅਨ੍ਤਰ੍ਭਾਵ <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Internal EmitC implementation

ਵਰਗ EmitCHeader final : ਜਨਤਕ EmitCBaseVisitorConst {
    V3_Unique_ਨਾਂ m_ਨਾਂ;
    VMemberMap m_memberMap;
    // METHODS

    ਵਰਗ Cover_ਗਿਣਤੀ_Visitor final : ਜਨਤਕ VNVisitorConst {
        ਪੂਰਨ m_bins = 0;  // Running total of coverage bins counted so far

        ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ_Cover_Decl* nodep) override {
            // Each module class owns the counters for declarations it emits;
            // duplicate no-inline instances need separate arrays for
            // forcePerInstance reporting.
            ਜੇ (!nodep->ਅੰਕੜੇ_Decl_Nullp()) m_bins += nodep->ਮਾਪ();
        }
        ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ* nodep) override { iterateChildrenConst(nodep); }

    ਜਨਤਕ:
        ਸਪਸ਼ਟ Cover_ਗਿਣਤੀ_Visitor(ਅਚਲ Ast_ਗੰਢ_Module* modp) {
            iterateConst(const_cast<Ast_ਗੰਢ_Module*>(modp));
        }
        ਪੂਰਨ bins() ਅਚਲ { ਵਾਪਸ m_bins; }
    };

    ਖਾਲੀ decorate_ਪਹਿਲਾ(ਤਾਰਕਿਕ& ਪਹਿਲਾ, ਅਚਲ string& str) {
        ਜੇ (ਪਹਿਲਾ) {
            putsDecoration(ਖਾਲੀ_ਸੂਚਕ, str);
            ਪਹਿਲਾ = ਝੂਠ;
        }
    }
    ਖਾਲੀ emitCellDecls(ਅਚਲ Ast_ਗੰਢ_Module* modp) {
        ਤਾਰਕਿਕ ਪਹਿਲਾ = ਸੱਚ;
        ਲਈ (ਅਚਲ Ast_ਗੰਢ* nodep = modp->stmtsp(); nodep; nodep = nodep->nextp()) {
            ਜੇ (ਅਚਲ AstCell* ਅਚਲ cellp = VN_CAST(nodep, Cell)) {
                ਜੇ (cellp->modp()->isConstPool()) ਜਾਰੀ;  // Special emit rules
                decorate_ਪਹਿਲਾ(ਪਹਿਲਾ, "// CELLS\n");
                putns(cellp, EmitCUtil::ਅਗੇਤਰ_Name_Protect(cellp->modp()) + "* "
                                 + cellp->nameProtect() + ";\n");
            }
        }
    }
    ਖਾਲੀ emitDesignVarDecls(ਅਚਲ Ast_ਗੰਢ_Module* modp) {
        // V3VariableOrder puts all variables at the front of the list: top level ports, then
        // trivially constructible variables, then the not trivially constructible ones, then
        // those that are not model state
        ਅਚਲ AstVar* portp = ਖਾਲੀ_ਸੂਚਕ;
        ਅਚਲ AstVar* trivp = ਖਾਲੀ_ਸੂਚਕ;
        ਅਚਲ AstVar* nontp = ਖਾਲੀ_ਸੂਚਕ;
        ਮਾਪ_t n_ਪੋਰਟ = 0;
        ਮਾਪ_t nTriv = 0;
        ਮਾਪ_t nNont = 0;
        ਲਈ (ਅਚਲ Ast_ਗੰਢ* nodep = modp->stmtsp(); nodep; nodep = nodep->nextp()) {
            ਅਚਲ AstVar* ਅਚਲ varp = VN_CAST(nodep, Var);
            ਜੇ (!varp) ਰੋਕੋ;
            ਜੇ (!varp->isModelState()) {
                // Cache line alignment is only effective on emitted fields
                UASSERT_OBJ(!varp->mtask_Cache_ਸਤਰ_Align(), varp, "Aligned non-field");
            } ਨਹੀਂਤਾਂ ਜੇ (varp->isPrimaryIO()) {
                ਜੇ (!portp) portp = varp;
                ++n_ਪੋਰਟ;
            } ਨਹੀਂਤਾਂ ਜੇ (varp->isTriviallyConstructible()) {
                ਜੇ (!trivp) trivp = varp;
                ++nTriv;
            } ਨਹੀਂਤਾਂ {
                ਜੇ (!nontp) nontp = varp;
                ++nNont;
            }
        }

        ਅਚਲ ਸਵੈਚਾਲਿਤ emit_And_ਕਦਮ = [&](ਅਚਲ AstVar*& varpr) {
            emitVarDecl(varpr);
            varpr = VN_CAST(varpr->nextp(), Var);
        };

        // Emit primary ports
        ਜੇ (n_ਪੋਰਟ) {
            putsDecoration(ਖਾਲੀ_ਸੂਚਕ, "\n// TOP LEVEL PORTS\n");
            // In anonymous union to prevent the C++ compiler from generating
            // constructor/destructor calls
            puts("union {\nstruct {\n");
            ਲਈ (ਮਾਪ_t i = 0; i < n_ਪੋਰਟ; ++i) emit_And_ਕਦਮ(portp);
            puts("} __VprimaryPorts;\n};\n");
        }

        // Emit trivially constructible variables
        ਜੇ (nTriv) {
            putsDecoration(ਖਾਲੀ_ਸੂਚਕ, "\n// TRIVIALLY CONSTRUCTIBLE STATE\n");
            ਜੇ (v3Global.opt.comp_ਹੱਦਬੰਦੀ_Members() == 0) {
                // Anonymous structs disabled, so just emit them directly
                ਲਈ (ਮਾਪ_t i = 0; i < nTriv; ++i) emit_And_ਕਦਮ(trivp);
            } ਨਹੀਂਤਾਂ {
                ਅਚਲ ਮਾਪ_t lim = v3Global.opt.comp_ਹੱਦਬੰਦੀ_Members();
                ਮਾਪ_t anonL3s = 1;
                ਮਾਪ_t anonL2s = 1;
                ਮਾਪ_t anonL1s = 1;
                ਜੇ (nTriv > (lim * lim * lim)) {
                    anonL3s = (nTriv + (lim * lim * lim) - 1) / (lim * lim * lim);
                    anonL2s = lim;
                    anonL1s = lim;
                } ਨਹੀਂਤਾਂ ਜੇ (nTriv > (lim * lim)) {
                    anonL2s = (nTriv + (lim * lim) - 1) / (lim * lim);
                    anonL1s = lim;
                } ਨਹੀਂਤਾਂ ਜੇ (nTriv > lim) {
                    anonL1s = (nTriv + lim - 1) / lim;
                }
                ਜੇ (anonL1s != 1) {
                    putsDecoration(
                        ਖਾਲੀ_ਸੂਚਕ,
                        "// Anonymous structures to workaround compiler member-count bugs\n");
                }
                ਮਾਪ_t nEmitted = 0;
                ਲਈ (ਮਾਪ_t l3 = 0; l3 < anonL3s && nEmitted < nTriv; ++l3) {
                    ਜੇ (anonL3s != 1) puts("struct {\n");
                    ਲਈ (ਮਾਪ_t l2 = 0; l2 < anonL2s && nEmitted < nTriv; ++l2) {
                        ਜੇ (anonL2s != 1) puts("struct {\n");
                        ਲਈ (ਮਾਪ_t l1 = 0; l1 < anonL1s && nEmitted < nTriv; ++l1) {
                            ਜੇ (anonL1s != 1) puts("struct {\n");
                            ਲਈ (ਮਾਪ_t l0 = 0; l0 < lim && nEmitted < nTriv; ++l0, ++nEmitted) {
                                emit_And_ਕਦਮ(trivp);
                            }
                            ਜੇ (anonL1s != 1) puts("};\n");
                        }
                        ਜੇ (anonL2s != 1) puts("};\n");
                    }
                    ਜੇ (anonL3s != 1) puts("};\n");
                }
                UASSERT_OBJ(nEmitted == nTriv, modp, "Not all variables emitted");
            }
        }

        // Emit not trivially constructible variables
        ਜੇ (nNont) {
            putsDecoration(ਖਾਲੀ_ਸੂਚਕ, "\n// NOT TRIVIALLY CONSTRUCTIBLE STATE\n");
            // Classes use real constructors
            ਜੇ (VN_IS(modp, Class)) {
                ਲਈ (ਮਾਪ_t i = 0; i < nNont; ++i) emit_And_ਕਦਮ(nontp);
                ਵਾਪਸ;
            }
            // Output under an anonymous union to prevent the C++ compiler from generating
            // constructor/destructor calls for them. They are managed explicitly in ctor/dtor.
            // While 'union { singleVar; }' for each variable would be simpler, it blows up
            // clang's compile time when using pre-compiled headers, joy.
            puts("union {\nstruct {\n");
            ਲਈ (ਮਾਪ_t i = 0; i < nNont; ++i) emit_And_ਕਦਮ(nontp);
            puts("} __Vnontrivs;\n};\n");
        }
    }
    ਖਾਲੀ emit_ਅੰਦਰੂਨੀ_Var_Decls(ਅਚਲ Ast_ਗੰਢ_Module* modp) {
        ਜੇ (ਅਚਲ AstClass* ਅਚਲ classp = VN_CAST(modp, Class)) {
            ਜੇ (classp->needRNG()) {
                putsDecoration(ਖਾਲੀ_ਸੂਚਕ, "\n// INTERNAL VARIABLES\n");
                puts("VlRNGReseeds __Vm_rng;\n");
            }
        } ਨਹੀਂਤਾਂ {  // not class
            putsDecoration(ਖਾਲੀ_ਸੂਚਕ, "\n// INTERNAL VARIABLES\n");
            puts(EmitCUtil::symClassName() + "* vlSymsp;\n");
            puts("const char* vlNamep;\n");
            // Allocate object-local coverage counters only on modules that
            // contain emitted coverage declarations.
            ਅਚਲ ਪੂਰਨ coverBins = Cover_ਗਿਣਤੀ_Visitor{modp}.bins();
            ਜੇ (coverBins) {
                puts(v3Global.opt.ਧਾਗੇ() > 1 ? "std::atomic<uint32_t>" : "uint32_t");
                puts(" __Vcoverage[");
                puts(std::ਮੰਜ਼ਿਲ_string(coverBins));
                puts("]{};\n");
            }
        }
    }
    ਖਾਲੀ emitParamDecls(ਅਚਲ Ast_ਗੰਢ_Module* modp) {
        ਤਾਰਕਿਕ ਪਹਿਲਾ = ਸੱਚ;
        ਲਈ (Ast_ਗੰਢ* nodep = modp->stmtsp(); nodep; nodep = nodep->nextp()) {
            ਜੇ (ਅਚਲ AstVar* ਅਚਲ varp = VN_CAST(nodep, Var)) {
                ਜੇ (varp->isParam()) {
                    decorate_ਪਹਿਲਾ(ਪਹਿਲਾ, "\n// PARAMETERS\n");
                    putns(varp, "static const ");
                    puts(varp->dtypep()->cType(varp->nameProtect(), ਝੂਠ, ਝੂਠ));
                    puts(";\n");
                }
            }
        }
    }
    ਖਾਲੀ emitCtorDtorDecls(ਅਚਲ Ast_ਗੰਢ_Module* modp) {
        // Classes use CFuncs with isConstructor/isDestructor
        ਜੇ (VN_IS(modp, Class)) ਵਾਪਸ;

        // Proper modules use loose split ctor/dtor and are not copyable/movable
        ਅਚਲ std::string name = EmitCUtil::ਅਗੇਤਰ_Name_Protect(modp);
        putsDecoration(ਖਾਲੀ_ਸੂਚਕ, "\n// CONSTRUCTORS\n");
        putns(modp, name + "(" + EmitCUtil::symClassName() + "* symsp, const char* namep);\n");
        putns(modp, "~" + name + "();\n");
        putns(modp, "VL_UNCOPYABLE(" + name + ");\n");
        putns(modp, "VL_UNMOVABLE(" + name + ");\n");
    }
    ਖਾਲੀ emit_ਅੰਦਰੂਨੀ_ਵਿਧੀ_Decls(ਅਚਲ Ast_ਗੰਢ_Module* modp) {
        ਤਾਰਕਿਕ ਪਹਿਲਾ = ਸੱਚ;
        ਅਚਲ string section = "\n// INTERNAL METHODS\n";

        ਜੇ (!VN_IS(modp, Class)) {
            decorate_ਪਹਿਲਾ(ਪਹਿਲਾ, section);
            ਜੇ (v3Global.opt.coverage()) {
                puts("void " + protect("__Vconfigure") + "(bool first);\n");
            }
        } ਨਹੀਂਤਾਂ {
            decorate_ਪਹਿਲਾ(ਪਹਿਲਾ, section);
            ਅਚਲ std::string name = V3_ਬਾਹਰਲਾ_Formatter::quoteNameControls(
                VIdProtect::protect_ਸ਼ਬਦ_If(modp->prettyName(), v3Global.opt.protectIds()));
            // "override", but don't want clang to start checking them:
            puts("virtual const char* typeName() const { return \""s + name + "\"; }\n");
        }

        ਜੇ (v3Global.opt.coverage() && !VN_IS(modp, Class)) {
            decorate_ਪਹਿਲਾ(ਪਹਿਲਾ, section);
            puts("void __vlCoverInsert(");
            puts(v3Global.opt.ਧਾਗੇ() > 1 ? "std::atomic<uint32_t>" : "uint32_t");
            puts("* countp, bool enable, bool localCounter, const char* filenamep, int lineno, "
                 "int column,\n");
            puts("const char* hierp, const char* pagep, const char* commentp, const char* "
                 "linescovp,\n");
            puts("const char* fsmVarp, const char* fsmFromp, const char* fsmTop, const char* "
                 "fsmTagp);\n");
        }

        ਜੇ (v3Global.opt.coverageToggle() && !VN_IS(modp, Class)) {
            decorate_ਪਹਿਲਾ(ਪਹਿਲਾ, section);
            puts("void __vlCoverToggleInsert(int begin, int end, bool ranged, ");
            puts(v3Global.opt.ਧਾਗੇ() > 1 ? "std::atomic<uint32_t>" : "uint32_t");
            puts("* countp, bool enable, bool localCounter, const char* filenamep, int lineno, "
                 "int column,\n");
            puts("const char* hierp, const char* pagep, const char* commentp);\n");
        }

        ਜੇ (v3Global.opt.savable()) {
            decorate_ਪਹਿਲਾ(ਪਹਿਲਾ, section);
            puts("void " + protect("__Vserialize") + "(VerilatedSerialize& os);\n");
            puts("void " + protect("__Vdeserialize") + "(VerilatedDeserialize& os);\n");
        }

        // Polymorphic clone for concrete (non-abstract, non-interface) classes
        ਜੇ (ਅਚਲ AstClass* ਅਚਲ classp = VN_CAST(modp, Class)) {
            ਜੇ (!classp->isInterfaceClass() && !classp->isVirtual()) {
                decorate_ਪਹਿਲਾ(ਪਹਿਲਾ, section);
                ਵਰਤੋਂ EmbeddedCovergroupVar = std::pair<ਅਚਲ AstClass*, ਅਚਲ AstVar*>;
                std::ਸਦਿਸ਼<EmbeddedCovergroupVar> embeddedCovergroupVars;
                ਅਚਲ ਸਵੈਚਾਲਿਤ has_Enclosing_ਪਿੱਠ_ਸੂਚਕ = [](ਅਚਲ AstClass* covergroupp) {
                    ਵਾਪਸ covergroupp->exists([](ਅਚਲ AstVar* ਅਚਲ varp) {
                        ਅਚਲ AstClassRefDType* ਅਚਲ refp
                            = VN_CAST(varp->dtypep()->skipRefp(), ClassRefDType);
                        ਵਾਪਸ refp && refp->raw_ਸੂਚਕ();
                    });
                };
                const_cast<AstClass*>(classp)->foreachMember(
                    [&](AstClass* ਅਚਲ memberClassp, AstVar* ਅਚਲ varp) {
                        ਅਚਲ AstClassRefDType* ਅਚਲ refp
                            = VN_CAST(varp->dtypep()->skipRefp(), ClassRefDType);
                        ਜੇ (refp && refp->classp()->isCovergroup()
                            && has_Enclosing_ਪਿੱਠ_ਸੂਚਕ(refp->classp())) {
                            embeddedCovergroupVars.emplace_ਪਿੱਠ(memberClassp, varp);
                        }
                    });
                ਅਚਲ string className = EmitCUtil::ਅਗੇਤਰ_Name_Protect(classp);
                ਜੇ (embeddedCovergroupVars.ਖਾਲੀ_ਥਾਵਾਂ() && !classp->hasRandVarsUpdate()) {
                    putns(classp,
                          "VlClass* clone() const { return new " + className + "(*this); }\n");
                } ਨਹੀਂਤਾਂ {
                    putns(classp, "VlClass* clone() const { " + className + "* const clonep = new "
                                      + className + "(*this); ");
                    ਜੇ (classp->hasRandVarsUpdate()) {
                        ਅਚਲ string updateName = "__VnoInFunc___VupdateRandVars";
                        AstCFunc* ਅਚਲ updatep
                            = VN_AS(m_memberMap.ਲੱਭੋ_Member(classp, updateName), CFunc);
                        UASSERT_OBJ(updatep, classp, "Missing updateRandVars method");
                        puts("clonep->" + updatep->nameProtect() + "();\n");
                    }
                    ਲਈ (ਅਚਲ EmbeddedCovergroupVar& ਵਸਤੂ_3 : embeddedCovergroupVars) {
                        puts("clonep->" + EmitCUtil::ਅਗੇਤਰ_Name_Protect(ਵਸਤੂ_3.ਪਹਿਲਾ)
                             + "::" + ਵਸਤੂ_3.ਦੂਜਾ->nameProtect() + " = VlNull{}; ");
                    }
                    puts("return clonep; }\n");
                }
            }
        }
    }
    ਖਾਲੀ emitEnums(ਅਚਲ Ast_ਗੰਢ_Module* modp) {
        ਤਾਰਕਿਕ ਪਹਿਲਾ = ਸੱਚ;
        ਲਈ (ਅਚਲ Ast_ਗੰਢ* nodep = modp->stmtsp(); nodep; nodep = nodep->nextp()) {
            ਅਚਲ AstTypedef* ਅਚਲ tdefp = VN_CAST(nodep, Typedef);
            ਜੇ (!tdefp) ਜਾਰੀ;
            ਜੇ (!tdefp->attr_ਜਨਤਕ()) ਜਾਰੀ;
            ਅਚਲ AstEnumDType* ਅਚਲ edtypep
                = VN_CAST(tdefp->dtypep()->skip_Ref_ਮੰਜ਼ਿਲ_Enump(), EnumDType);
            ਜੇ (!edtypep) ਜਾਰੀ;
            decorate_ਪਹਿਲਾ(ਪਹਿਲਾ, "\n// ENUMS (that were declared public)\n");
            ਜੇ (edtypep->ਚੌੜਾਈ() > 64) {
                putsDecoration(tdefp,
                               "// enum " + tdefp->nameProtect() + " ignored: Too wide for C++\n");
            } ਨਹੀਂਤਾਂ {
                putns(tdefp, "enum " + tdefp->name() + " {\n");
                ਲਈ (ਅਚਲ Ast_Enum_ਵਸਤੂ* itemp = edtypep->itemsp(); itemp;
                     itemp = VN_AS(itemp->nextp(), Enum_ਵਸਤੂ)) {
                    ਜੇ (ਅਚਲ AstConst* ਅਚਲ constp = VN_CAST(itemp->valuep(), Const)) {
                        ਜੇ (constp->ਅੰਕ().isFourState()) {
                            putns(itemp, "// " + itemp->nameProtect() + " is four-state\n");
                            ਜਾਰੀ;
                        }
                    }
                    putns(itemp, itemp->nameProtect());
                    puts(" = ");
                    putns(itemp, VN_AS(itemp->valuep(), Const)->ਅੰਕ().emitC());
                    ਜੇ (VN_IS(itemp->nextp(), Enum_ਵਸਤੂ)) puts(",");
                    puts("\n");
                }
                puts("};\n");
            }
        }
    }
    ਖਾਲੀ emitStructDecl(ਅਚਲ Ast_ਗੰਢ_Module* modp, Ast_ਗੰਢ_UOr_Struct_DType* sdtypep,
                        std::ਸੈੱਟ<Ast_ਗੰਢ_UOr_Struct_DType*>& emitted) {
        ਜੇ (emitted.ਗਿਣਤੀ_2(sdtypep) > 0) ਵਾਪਸ;
        emitted.ਪਾਓ(sdtypep);
        ਲਈ (AstMemberDType* itemp = sdtypep->membersp(); itemp;
             itemp = VN_AS(itemp->nextp(), MemberDType)) {
            Ast_ਗੰਢ_UOr_Struct_DType* ਅਚਲ subp = itemp->get_ਬੱਚਾ_Structp();
            ਜੇ (subp && (!subp->packed() || sdtypep->packed())) {
                // Recurse if it belongs to the current module
                ਜੇ (subp->classOrPackagep() == modp) {
                    emitStructDecl(modp, subp, emitted);
                    puts("\n");
                }
            }
        }
        ਜੇ (sdtypep->packed()) {
            emitPackedUOrSBody(sdtypep);
        } ਨਹੀਂਤਾਂ {
            emitUnpackedUOrSBody(sdtypep);
        }
    }
    ਗਿਣਤੀ ਵਰਗ AttributeType : uint8_t { ਚੌੜਾਈ_2, DIMENSION };
    // Get member attribute based on type
    ਪੂਰਨ get_ਗੰਢ_Attribute(ਅਚਲ AstMemberDType* itemp, AttributeType type) {
        ਅਚਲ ਤਾਰਕਿਕ isArrayType = itemp->dtypep()->isNonPackedArray();
        ਚੋਣ (type) {
        ਹਾਲਤ AttributeType::ਚੌੜਾਈ_2: {
            ਜੇ (isArrayType) {
                // For arrays, get innermost element width
                ਅਚਲ Ast_ਗੰਢ_DType* dtype = itemp->dtypep();
                ਜਦੋਂ (dtype->subDTypep()) dtype = dtype->subDTypep();
                ਵਾਪਸ dtype->ਚੌੜਾਈ();
            }
            ਵਾਪਸ itemp->ਚੌੜਾਈ();
        }
        ਹਾਲਤ AttributeType::DIMENSION: {
            // Return array dimension or 0 for non-arrays
            ਵਾਪਸ isArrayType ? itemp->dtypep()->dimensions(ਸੱਚ).ਦੂਜਾ : 0;
        }
        ਮੂਲ: {
            ਵਾਪਸ 0;
        }
        }
    }
    ਸਾਂਚਾ <AttributeType T>
    ਖਾਲੀ emit_Member_ਸਦਿਸ਼(ਅਚਲ Ast_ਗੰਢ_UOr_Struct_DType* sdtypep) {
        puts("return {");
        ਤਾਰਕਿਕ needComma = ਝੂਠ;
        ਲਈ (ਅਚਲ AstMemberDType* itemp = sdtypep->membersp(); itemp;
             itemp = VN_AS(itemp->nextp(), MemberDType)) {
            ਜੇ (!itemp->isConstrainedRand()) ਜਾਰੀ;
            // Comma handling: add before element except first
            ਜੇ (needComma) puts(",\n");
            putns(itemp, std::ਮੰਜ਼ਿਲ_string(get_ਗੰਢ_Attribute(itemp, T)));
            needComma = ਸੱਚ;
        }
        puts("};\n}\n");
    }
    ਖਾਲੀ emitUnpackedUOrSBody(Ast_ਗੰਢ_UOr_Struct_DType* sdtypep) {
        putns(sdtypep, sdtypep->verilogKwd());  // "struct"/"union"
        puts(" " + EmitCUtil::ਅਗੇਤਰ_Name_Protect(sdtypep) + " {\n");
        ਲਈ (ਅਚਲ AstMemberDType* itemp = sdtypep->membersp(); itemp;
             itemp = VN_AS(itemp->nextp(), MemberDType)) {
            putns(itemp, itemp->dtypep()->cType(itemp->nameProtect(), ਝੂਠ, ਝੂਠ));
            puts(";\n");
        }

        // Three helper functions for struct constrained randomization:
        // - memberNames: Get member names
        // - getMembers: Access member references
        // - memberIndices: Retrieve member indices
        // - memberWidth: Retrieve member width
        // - memberDimension: Retrieve member dimension
        ਜੇ (sdtypep->isConstrainedRand()) {
            ਤਾਰਕਿਕ needComma = ਝੂਠ;
            putns(sdtypep, "\nstd::vector<std::string> memberNames(void) const {\n");
            puts("return {");
            ਲਈ (ਅਚਲ AstMemberDType* itemp = sdtypep->membersp(); itemp;
                 itemp = VN_AS(itemp->nextp(), MemberDType)) {
                ਜੇ (!itemp->isConstrainedRand()) ਜਾਰੀ;
                ਜੇ (needComma) puts(",\n");
                putns(itemp, "\"" + itemp->shortName() + "\"");
                needComma = ਸੱਚ;
            }
            puts("};\n}\n");

            putns(sdtypep, "\nstd::vector<int> memberWidth(void) const {\n");
            emit_Member_ਸਦਿਸ਼<AttributeType::ਚੌੜਾਈ_2>(sdtypep);

            putns(sdtypep, "\nstd::vector<int> memberDimension(void) const {\n");
            emit_Member_ਸਦਿਸ਼<AttributeType::DIMENSION>(sdtypep);

            needComma = ਝੂਠ;
            putns(sdtypep, "\nauto memberIndices(void) const {\n");
            puts("return std::index_sequence_for<");
            ਲਈ (ਅਚਲ AstMemberDType* itemp = sdtypep->membersp(); itemp;
                 itemp = VN_AS(itemp->nextp(), MemberDType)) {
                ਜੇ (!itemp->isConstrainedRand()) ਜਾਰੀ;
                ਜੇ (needComma) puts(",\n");
                putns(itemp, itemp->dtypep()->cType("", ਝੂਠ, ਝੂਠ));
                needComma = ਸੱਚ;
            }
            puts(">{};\n}\n");

            needComma = ਝੂਠ;
            putns(sdtypep, "\ntemplate <typename T>");
            putns(sdtypep, "\nauto getMembers(T& obj) {\n");
            puts("return std::tie(");
            ਲਈ (ਅਚਲ AstMemberDType* itemp = sdtypep->membersp(); itemp;
                 itemp = VN_AS(itemp->nextp(), MemberDType)) {
                ਜੇ (!itemp->isConstrainedRand()) ਜਾਰੀ;
                ਜੇ (needComma) puts(",\n");
                putns(itemp, "obj." + itemp->nameProtect());
                needComma = ਸੱਚ;
            }
            puts(");\n}\n");
        }
        putns(sdtypep, "\nbool operator==(const " + EmitCUtil::ਅਗੇਤਰ_Name_Protect(sdtypep)
                           + "& rhs) const {\n");
        puts("return ");
        ਲਈ (ਅਚਲ AstMemberDType* itemp = sdtypep->membersp(); itemp;
             itemp = VN_AS(itemp->nextp(), MemberDType)) {
            ਜੇ (itemp != sdtypep->membersp()) puts("\n    && ");
            putns(itemp, itemp->nameProtect() + " == " + "rhs." + itemp->nameProtect());
        }
        puts(";\n");
        puts("}\n");
        putns(sdtypep, "bool operator!=(const " + EmitCUtil::ਅਗੇਤਰ_Name_Protect(sdtypep)
                           + "& rhs) const {\n");
        puts("return !(*this == rhs);\n}\n");
        putns(sdtypep, "\nbool operator<(const " + EmitCUtil::ਅਗੇਤਰ_Name_Protect(sdtypep)
                           + "& rhs) const {\n");
        ਲਈ (ਅਚਲ AstMemberDType* itemp = sdtypep->membersp(); itemp;
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
        putns(sdtypep, "struct VlIsCustomStruct<" + EmitCUtil::ਅਗੇਤਰ_Name_Protect(sdtypep)
                           + "> : public std::true_type {};\n");
    }

    // getfunc: VL_ASSIGNSEL_XX(rbits, obits, off, lhsdata, rhsdata);
    // !getfunc: VL_SELASSIGN_XX(rbits, obits, lhsdata, rhsdata, off);
    ਖਾਲੀ emit_Vl_ਸੌਂਪੋ(ਅਚਲ Ast_ਗੰਢ_DType* ਅਚਲ lhstype, ਅਚਲ Ast_ਗੰਢ_DType* rhstype,
                      ਅਚਲ std::string& off, ਅਚਲ std::string& lhsdata,
                      ਅਚਲ std::string& rhsdata, ਤਾਰਕਿਕ getfunc) {
        puts(getfunc ? "VL_ASSIGNSEL_" : "VL_SELASSIGN_");
        puts(lhstype->charIQWN());
        puts(rhstype->charIQWN());
        puts("(" + std::ਮੰਜ਼ਿਲ_string(lhstype->ਚੌੜਾਈ()) + ", ");  // LHS width
        ਜੇ (getfunc) {
            puts(std::ਮੰਜ਼ਿਲ_string(rhstype->ਚੌੜਾਈ()) + ", ");  // Number of copy bits
            puts(off + ", ");  // LHS offset
        } ਨਹੀਂਤਾਂ {
            // Number of copy bits. Use widthTototalBytes to
            // make VL_SELASSIGN_XX clear upper unused bits for us.
            // puts(std::to_string(lhstype->width()) + ", ");
            puts(std::ਮੰਜ਼ਿਲ_string(lhstype->ਚੌੜਾਈ_ਕੁੱਲ_ਬਾਈਟਾਂ() * 8) + ", ");
        }
        puts(lhsdata + ", ");  // LHS data
        puts(rhsdata);  // RHS data
        ਜੇ (!getfunc) {
            puts(", " + off);  // RHS offset
        }
        puts(");\n");
    }

    // `retOrArg` should be prefixed by `&` or suffixed by `.data()` depending on its type
    ਖਾਲੀ emitPackedMember(ਅਚਲ Ast_ਗੰਢ_DType* ਮਾਪਾ_Dtypep, ਅਚਲ Ast_ਗੰਢ_DType* dtypep,
                          ਅਚਲ std::string& fieldname, ਅਚਲ std::string& ਵਿੱਥ, ਤਾਰਕਿਕ getfunc,
                          ਅਚਲ std::string& retOrArg) {
        dtypep = dtypep->skipRefp();
        ਜੇ (ਅਚਲ ਸਵੈਚਾਲਿਤ* adtypep = VN_CAST(dtypep, PackArrayDType)) {
            ਅਚਲ std::string ਸੂਚਕਾਂਕ = m_ਨਾਂ.get("__Vi");
            puts("for (int " + ਸੂਚਕਾਂਕ + " = 0; " + ਸੂਚਕਾਂਕ + " < "
                 + std::ਮੰਜ਼ਿਲ_string(adtypep->elementsConst()) + "; ++" + ਸੂਚਕਾਂਕ + ") {\n");

            ਅਚਲ std::string ਵਿੱਥ_ਵਿੱਚ_Loop
                = ਵਿੱਥ + " + " + ਸੂਚਕਾਂਕ + " * " + std::ਮੰਜ਼ਿਲ_string(adtypep->subDTypep()->ਚੌੜਾਈ());
            ਅਚਲ std::string newName = fieldname + "[" + ਸੂਚਕਾਂਕ + "]";
            emitPackedMember(ਮਾਪਾ_Dtypep, adtypep->subDTypep(), newName, ਵਿੱਥ_ਵਿੱਚ_Loop, getfunc,
                             retOrArg);
            puts("}\n");
        } ਨਹੀਂਤਾਂ ਜੇ (VN_IS(dtypep, ਗੰਢ_UOr_Struct_DType)) {
            ਅਚਲ std::string tmp = m_ਨਾਂ.get("__Vtmp");
            ਜੇ (getfunc) {  // Emit `get` func;
                // auto __tmp = field.get();
                puts("auto " + tmp + " = " + fieldname + ".get();\n");
                // VL_ASSIGNSEL_XX(rbits, obits, lsb, lhsdata, rhsdata);
                emit_Vl_ਸੌਂਪੋ(ਮਾਪਾ_Dtypep, dtypep, ਵਿੱਥ, retOrArg, tmp, getfunc);
            } ਨਹੀਂਤਾਂ {  // Emit `set` func
                ਅਚਲ std::string tmptype = AstCDType::type_ਮੰਜ਼ਿਲ_Hold(dtypep->ਚੌੜਾਈ());
                // type tmp;
                puts(tmptype + " " + tmp + ";\n");
                // VL_SELASSIGN_XX(rbits, obits, lhsdata, rhsdata, roffset);
                emit_Vl_ਸੌਂਪੋ(dtypep, ਮਾਪਾ_Dtypep, ਵਿੱਥ, tmp, retOrArg, getfunc);
                // field.set(__tmp);
                puts(fieldname + ".set(" + tmp + ");\n");
            }
        } ਨਹੀਂਤਾਂ {
            UASSERT_OBJ(VN_IS(dtypep, EnumDType) || VN_IS(dtypep, BasicDType), dtypep,
                        "Unsupported type in packed struct or union");
            ਜੇ (getfunc) {  // Emit `get` func;
                // VL_ASSIGNSEL_XX(rbits, obits, lsb, lhsdata, rhsdata);
                emit_Vl_ਸੌਂਪੋ(ਮਾਪਾ_Dtypep, dtypep, ਵਿੱਥ, retOrArg, fieldname, getfunc);
            } ਨਹੀਂਤਾਂ {  // Emit `set` func
                // VL_SELASSIGN_XX(rbits, obits, lhsdata, rhsdata, roffset);
                emit_Vl_ਸੌਂਪੋ(dtypep, ਮਾਪਾ_Dtypep, ਵਿੱਥ, fieldname, retOrArg, getfunc);
            }
        }
    }
    ਖਾਲੀ emitPackedUOrSBody(Ast_ਗੰਢ_UOr_Struct_DType* sdtypep) {
        putns(sdtypep, sdtypep->verilogKwd());  // "struct"/"union"
        puts(" " + EmitCUtil::ਅਗੇਤਰ_Name_Protect(sdtypep) + " {\n");

        AstMemberDType* itemp;
        AstMemberDType* ਆਖਰੀ_Itemp;
        ਅਚਲ AstMemberDType* witemp = ਖਾਲੀ_ਸੂਚਕ;
        // LSB is first field in C, so loop backwards
        ਲਈ (ਆਖਰੀ_Itemp = sdtypep->membersp(); ਆਖਰੀ_Itemp && ਆਖਰੀ_Itemp->nextp();
             ਆਖਰੀ_Itemp = VN_AS(ਆਖਰੀ_Itemp->nextp(), MemberDType)) {
            ਜੇ (ਆਖਰੀ_Itemp->ਚੌੜਾਈ() == sdtypep->ਚੌੜਾਈ()) witemp = ਆਖਰੀ_Itemp;
        }
        ਲਈ (itemp = ਆਖਰੀ_Itemp; itemp; itemp = VN_CAST(itemp->backp(), MemberDType)) {
            putns(itemp, itemp->dtypep()->cType(itemp->nameProtect(), ਝੂਠ, ਝੂਠ, ਸੱਚ));
            puts(";\n");
        }

        ਅਚਲ std::string retArgName = m_ਨਾਂ.get("__v");
        ਅਚਲ std::string retArgType = AstCDType::type_ਮੰਜ਼ਿਲ_Hold(sdtypep->ਚੌੜਾਈ());

        // Emit `get` member function
        puts(retArgType + " get() const {\n");
        puts(retArgType + " " + retArgName + ";\n");
        ਜੇ (VN_IS(sdtypep, StructDType)) {
            ਲਈ (itemp = ਆਖਰੀ_Itemp; itemp; itemp = VN_CAST(itemp->backp(), MemberDType)) {
                emitPackedMember(sdtypep, itemp->dtypep(), itemp->nameProtect(),
                                 std::ਮੰਜ਼ਿਲ_string(itemp->lsb()), /*getfunc=*/ਸੱਚ, retArgName);
            }
        } ਨਹੀਂਤਾਂ {
            // We only need to fill the widest field of union
            emitPackedMember(sdtypep, witemp->dtypep(), witemp->nameProtect(),
                             std::ਮੰਜ਼ਿਲ_string(witemp->lsb()), /*getfunc=*/ਸੱਚ, retArgName);
        }
        puts("return " + retArgName + ";\n");
        puts("}\n");

        // Emit `set` member function
        puts("void set(const " + retArgType + "& " + retArgName + ") {\n");
        ਜੇ (VN_IS(sdtypep, StructDType)) {
            ਲਈ (itemp = ਆਖਰੀ_Itemp; itemp; itemp = VN_CAST(itemp->backp(), MemberDType)) {
                emitPackedMember(sdtypep, itemp->dtypep(), itemp->nameProtect(),
                                 std::ਮੰਜ਼ਿਲ_string(itemp->lsb()), /*getfunc=*/ਝੂਠ, retArgName);
            }
        } ਨਹੀਂਤਾਂ {
            // We only need to fill the widest field of union
            emitPackedMember(sdtypep, witemp->dtypep(), witemp->nameProtect(),
                             std::ਮੰਜ਼ਿਲ_string(witemp->lsb()), /*getfunc=*/ਝੂਠ, retArgName);
        }

        puts("}\n");

        puts("};\n");
        m_ਨਾਂ.reset();
    }
    ਖਾਲੀ emitStructs(ਅਚਲ Ast_ਗੰਢ_Module* modp) {
        // Track structs that've been emitted already
        std::ਸੈੱਟ<Ast_ਗੰਢ_UOr_Struct_DType*> emitted;
        ਲਈ (ਅਚਲ Ast_ਗੰਢ* nodep = modp->stmtsp(); nodep; nodep = nodep->nextp()) {
            ਅਚਲ AstTypedef* ਅਚਲ tdefp = VN_CAST(nodep, Typedef);
            ਜੇ (!tdefp) ਜਾਰੀ;
            Ast_ਗੰਢ_UOr_Struct_DType* ਅਚਲ sdtypep
                = VN_CAST(tdefp->dtypep()->skip_Ref_ਮੰਜ਼ਿਲ_Enump(), ਗੰਢ_UOr_Struct_DType);
            ਜੇ (!sdtypep) ਜਾਰੀ;
            // V3Inline can copy one module into several parents. Only the module that
            // declares the struct emits it, so C++ sees only one definition.
            ਅਚਲ ਤਾਰਕਿਕ declared_ਵਿੱਚ_Other_Module = sdtypep->classOrPackagep() != modp;
            ਜੇ (declared_ਵਿੱਚ_Other_Module) ਜਾਰੀ;
            emitStructDecl(modp, sdtypep, emitted);
        }
    }
    ਖਾਲੀ emitFuncDecls(ਅਚਲ Ast_ਗੰਢ_Module* modp, ਤਾਰਕਿਕ ਵਿੱਚ_Class_Body) {
        std::ਸਦਿਸ਼<ਅਚਲ AstCFunc*> funcsp;

        ਲਈ (ਅਚਲ Ast_ਗੰਢ* nodep = modp->stmtsp(); nodep; nodep = nodep->nextp()) {
            ਜੇ (ਅਚਲ AstCFunc* ਅਚਲ funcp = VN_CAST(nodep, CFunc)) {
                ਜੇ (funcp->dpiImportPrototype())  // Declared in __Dpi.h
                    ਜਾਰੀ;
                ਜੇ (funcp->dpiExportDispatcher())  // Declared in __Dpi.h
                    ਜਾਰੀ;
                ਜੇ (funcp->is_ਵਿਧੀ() != ਵਿੱਚ_Class_Body)  // Only methods go inside class
                    ਜਾਰੀ;
                ਜੇ (funcp->is_ਵਿਧੀ() && funcp->isLoose())  // Loose methods are declared lazily
                    ਜਾਰੀ;
                funcsp.ਧੱਕੋ_ਪਿੱਠ(funcp);
            }
        }

        std::stable_ਛਾਂਟੋ(funcsp.begin(), funcsp.ਸਿਰਾ(), [](ਅਚਲ Ast_ਗੰਢ* ap, ਅਚਲ Ast_ਗੰਢ* bp) {
            ਵਾਪਸ ap->name() < bp->name();
        });

        ਲਈ (ਅਚਲ AstCFunc* ਅਚਲ funcp : funcsp) {
            ਜੇ (ਵਿੱਚ_Class_Body) ofp()->puts_ਨਿੱਜੀ(funcp->decl_ਨਿੱਜੀ());
            emitCFuncDecl(funcp, modp);
        }
    }
    ਖਾਲੀ emitAll(ਅਚਲ Ast_ਗੰਢ_Module* modp) {
        // Include files required by this AstNodeModule
        ਜੇ (ਅਚਲ AstClass* ਅਚਲ classp = VN_CAST(modp, Class)) {
            ਲਈ (ਅਚਲ AstClassExtends* extp = classp->extendsp(); extp;
                 extp = VN_AS(extp->nextp(), ClassExtends)) {
                putns(extp, "#include \""
                                + EmitCUtil::ਅਗੇਤਰ_Name_Protect(extp->classp()->classOrPackagep())
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
        ਜੇ (!VN_IS(modp, Class)) puts("alignas(VL_CACHE_LINE_BYTES) ");
        puts(EmitCUtil::ਅਗੇਤਰ_Name_Protect(modp));
        ਜੇ (ਅਚਲ AstClass* ਅਚਲ classp = VN_CAST(modp, Class)) {
            puts(" : ");
            ਜੇ (classp->extendsp()) {
                ਤਾਰਕਿਕ needComma = ਝੂਠ;
                ਲਈ (ਅਚਲ AstClassExtends* extp = classp->extendsp(); extp;
                     extp = VN_AS(extp->nextp(), ClassExtends)) {
                    ਜੇ (needComma) puts(", ");
                    // Use virtual only for interfaces for class inheritance
                    // (extends)
                    puts(extp->classp()->use_Virtual_ਜਨਤਕ() ? "virtual public " : "public ");
                    putns(extp, EmitCUtil::ਅਗੇਤਰ_Name_Protect(extp->classp()));
                    needComma = ਸੱਚ;
                }
            } ਨਹੀਂਤਾਂ {
                puts("public virtual VlClass");
            }
        } ਨਹੀਂਤਾਂ {
            puts(" final");
        }
        puts(" {\n");
        ofp()->reset_ਨਿੱਜੀ();
        ofp()->puts_ਨਿੱਜੀ(ਝੂਠ);  // public:

        // Emit all class body contents
        emitCellDecls(modp);
        emitEnums(modp);
        emitDesignVarDecls(modp);
        emit_ਅੰਦਰੂਨੀ_Var_Decls(modp);
        emitParamDecls(modp);
        emitCtorDtorDecls(modp);
        emit_ਅੰਦਰੂਨੀ_ਵਿਧੀ_Decls(modp);
        emitFuncDecls(modp, /* inClassBody: */ ਸੱਚ);

        // From `systemc_interface
        emitSystemCSection(modp, VSystemCSectionType::INT);

        // Close class body
        puts("};\n");
        // }}}

        // Emit out of class function declarations
        puts("\n");
        emitFuncDecls(modp, /* inClassBody: */ ਝੂਠ);
        emitSystemCSection(modp, VSystemCSectionType::HDR_POST);
    }

    // VISITORS
    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ* nodep) override { nodep->v3fatalSrc("Unused"); }  // LCOV_EXCL_LINE

    ਸਪਸ਼ਟ EmitCHeader(ਅਚਲ Ast_ਗੰਢ_Module* modp) {
        UINFO(5, "  Emitting header for " << EmitCUtil::ਅਗੇਤਰ_Name_Protect(modp));

        // Open output file
        open_New_Output_ਸਿਰਲੇਖ_File(EmitCUtil::ਅਗੇਤਰ_Name_Protect(modp), "Design internal header");
        puts("// See " + EmitCUtil::ਸਿਖਰ_Class_Name() + ".h for the primary calling header\n");

        ofp()->putsGuard();

        // Include files
        puts("\n");
        ofp()->puts_Int_ਸਿਖਰ_Include();
        puts("#include \"verilated.h\"\n");
        ਜੇ (modp->is_ਸਿਖਰ() && optSystemC()) puts("#include \"verilated_sc.h\"\n");
        ਜੇ (v3Global.opt.mtasks()) puts("#include \"verilated_threads.h\"\n");
        ਜੇ (v3Global.opt.savable()) puts("#include \"verilated_save.h\"\n");
        ਜੇ (v3Global.opt.coverage()) puts("#include \"verilated_cov.h\"\n");
        ਜੇ (v3Global.opt.coverage() || v3Global.useCovergroup())
            puts("#include \"verilated_covergroup.h\"\n");
        ਜੇ (v3Global.ਵਰਤਦਾ_Timing()) puts("#include \"verilated_timing.h\"\n");
        ਜੇ (v3Global.useRandomizeMethods()) puts("#include \"verilated_random.h\"\n");
        ਜੇ (v3Global.ਵਰਤਦਾ_Force()) puts("#include \"verilated_force.h\"\n");

        std::ਸੈੱਟ<string> cuse_ਸੈੱਟ;
        ਸਵੈਚਾਲਿਤ ਜੋੜੋ_ਮੰਜ਼ਿਲ_cuse_ਸੈੱਟ = [&](string s) { cuse_ਸੈੱਟ.ਪਾਓ(s); };

        forModCUse(modp, VUseType::INT_FWD_CLASS | VUseType::INT_INCLUDE, ਜੋੜੋ_ਮੰਜ਼ਿਲ_cuse_ਸੈੱਟ);
        ਜੇ (ਅਚਲ AstClassPackage* ਅਚਲ packagep = VN_CAST(modp, ClassPackage)) {
            forModCUse(packagep->classp(), VUseType::INT_INCLUDE | VUseType::INT_FWD_CLASS,
                       ਜੋੜੋ_ਮੰਜ਼ਿਲ_cuse_ਸੈੱਟ);
        }

        ਲਈ (ਅਚਲ string& s : cuse_ਸੈੱਟ) puts(s);
        puts("\n");

        emitAll(modp);

        ਜੇ (ਅਚਲ AstClassPackage* ਅਚਲ packagep = VN_CAST(modp, ClassPackage)) {
            // Put the non-static class implementation in same h file for speed
            emitAll(packagep->classp());
        }

        ofp()->puts_ਸਿਰਾ_Guard();

        // Close output file
        closeOutputFile();
    }
    ~EmitCHeader() override = ਮੂਲ;

ਜਨਤਕ:
    ਸਥਿਰ ਖਾਲੀ ਮੁੱਖ(ਅਚਲ Ast_ਗੰਢ_Module* modp) { ਅਚਲ EmitCHeader emitCHeader{modp}; }
};

//######################################################################
// EmitC class functions

ਖਾਲੀ V3EmitC::emitc_ਸਿਰਲੇਖਾਂ() {
    UINFO(2, __FUNCTION__ << ":");

    // Process each module in turn
    ਲਈ (ਅਚਲ Ast_ਗੰਢ* nodep = v3Global.rootp()->modulesp(); nodep; nodep = nodep->nextp()) {
        ਜੇ (VN_IS(nodep, Class)) ਜਾਰੀ;  // Declared with the ClassPackage
        ਅਚਲ Ast_ਗੰਢ_Module* ਅਚਲ modp = VN_AS(nodep, ਗੰਢ_Module);
        ਜੇ (modp->isConstPool()) ਜਾਰੀ;  // Emitted by V3EmitCConstPool
        EmitCHeader::ਮੁੱਖ(modp);
    }
}
