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
#అన్తర్భావ "V3EmitCBase.h"
#అన్తర్భావ "V3File.h"
#అన్తర్భావ "V3MemberMap.h"
#అన్తర్భావ "V3UniqueNames.h"

#అన్తర్భావ <algorithm>
#అన్తర్భావ <cstdint>
#అన్తర్భావ <set>
#అన్తర్భావ <string>
#అన్తర్భావ <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Internal EmitC implementation

తరగతి EmitCHeader final : బహిరంగ EmitCBaseVisitorConst {
    V3_Unique_పేర్లు m_పేర్లు;
    VMemberMap m_memberMap;
    // METHODS

    తరగతి Cover_లెక్క_Visitor final : బహిరంగ VNVisitorConst {
        పూర్ణాంకం m_bins = 0;  // Running total of coverage bins counted so far

        శూన్య సందర్శించు(Ast_కణుపు_Cover_Decl* nodep) override {
            // Each module class owns the counters for declarations it emits;
            // duplicate no-inline instances need separate arrays for
            // forcePerInstance reporting.
            ఒకవేళ (!nodep->దత్తాంశం_Decl_Nullp()) m_bins += nodep->కొలత();
        }
        శూన్య సందర్శించు(Ast_కణుపు* nodep) override { iterateChildrenConst(nodep); }

    బహిరంగ:
        స్పష్టమైన Cover_లెక్క_Visitor(స్థిరరాశి Ast_కణుపు_Module* modp) {
            iterateConst(const_cast<Ast_కణుపు_Module*>(modp));
        }
        పూర్ణాంకం bins() స్థిరరాశి { తిరిగి m_bins; }
    };

    శూన్య decorate_మొదటి(తార్కిక& మొదటి, స్థిరరాశి string& str) {
        ఒకవేళ (మొదటి) {
            putsDecoration(శూన్యసూచిక, str);
            మొదటి = అబద్ధం;
        }
    }
    శూన్య emitCellDecls(స్థిరరాశి Ast_కణుపు_Module* modp) {
        తార్కిక మొదటి = నిజం;
        కొరకు (స్థిరరాశి Ast_కణుపు* nodep = modp->stmtsp(); nodep; nodep = nodep->nextp()) {
            ఒకవేళ (స్థిరరాశి AstCell* స్థిరరాశి cellp = VN_CAST(nodep, Cell)) {
                ఒకవేళ (cellp->modp()->isConstPool()) కొనసాగు;  // Special emit rules
                decorate_మొదటి(మొదటి, "// CELLS\n");
                putns(cellp, EmitCUtil::ఉపసర్గ_Name_Protect(cellp->modp()) + "* "
                                 + cellp->nameProtect() + ";\n");
            }
        }
    }
    శూన్య emitDesignVarDecls(స్థిరరాశి Ast_కణుపు_Module* modp) {
        // V3VariableOrder puts all variables at the front of the list: top level ports, then
        // trivially constructible variables, then the not trivially constructible ones, then
        // those that are not model state
        స్థిరరాశి AstVar* portp = శూన్యసూచిక;
        స్థిరరాశి AstVar* trivp = శూన్యసూచిక;
        స్థిరరాశి AstVar* nontp = శూన్యసూచిక;
        కొలత_t n_పోర్టు = 0;
        కొలత_t nTriv = 0;
        కొలత_t nNont = 0;
        కొరకు (స్థిరరాశి Ast_కణుపు* nodep = modp->stmtsp(); nodep; nodep = nodep->nextp()) {
            స్థిరరాశి AstVar* స్థిరరాశి varp = VN_CAST(nodep, Var);
            ఒకవేళ (!varp) ఆపు;
            ఒకవేళ (!varp->isModelState()) {
                // Cache line alignment is only effective on emitted fields
                UASSERT_OBJ(!varp->mtask_Cache_పంక్తి_Align(), varp, "Aligned non-field");
            } లేకపోతే ఒకవేళ (varp->isPrimaryIO()) {
                ఒకవేళ (!portp) portp = varp;
                ++n_పోర్టు;
            } లేకపోతే ఒకవేళ (varp->isTriviallyConstructible()) {
                ఒకవేళ (!trivp) trivp = varp;
                ++nTriv;
            } లేకపోతే {
                ఒకవేళ (!nontp) nontp = varp;
                ++nNont;
            }
        }

        స్థిరరాశి స్వయంచాలక emit_And_అడుగు = [&](స్థిరరాశి AstVar*& varpr) {
            emitVarDecl(varpr);
            varpr = VN_CAST(varpr->nextp(), Var);
        };

        // Emit primary ports
        ఒకవేళ (n_పోర్టు) {
            putsDecoration(శూన్యసూచిక, "\n// TOP LEVEL PORTS\n");
            // In anonymous union to prevent the C++ compiler from generating
            // constructor/destructor calls
            puts("union {\nstruct {\n");
            కొరకు (కొలత_t i = 0; i < n_పోర్టు; ++i) emit_And_అడుగు(portp);
            puts("} __VprimaryPorts;\n};\n");
        }

        // Emit trivially constructible variables
        ఒకవేళ (nTriv) {
            putsDecoration(శూన్యసూచిక, "\n// TRIVIALLY CONSTRUCTIBLE STATE\n");
            ఒకవేళ (v3Global.opt.comp_హద్దు_Members() == 0) {
                // Anonymous structs disabled, so just emit them directly
                కొరకు (కొలత_t i = 0; i < nTriv; ++i) emit_And_అడుగు(trivp);
            } లేకపోతే {
                స్థిరరాశి కొలత_t lim = v3Global.opt.comp_హద్దు_Members();
                కొలత_t anonL3s = 1;
                కొలత_t anonL2s = 1;
                కొలత_t anonL1s = 1;
                ఒకవేళ (nTriv > (lim * lim * lim)) {
                    anonL3s = (nTriv + (lim * lim * lim) - 1) / (lim * lim * lim);
                    anonL2s = lim;
                    anonL1s = lim;
                } లేకపోతే ఒకవేళ (nTriv > (lim * lim)) {
                    anonL2s = (nTriv + (lim * lim) - 1) / (lim * lim);
                    anonL1s = lim;
                } లేకపోతే ఒకవేళ (nTriv > lim) {
                    anonL1s = (nTriv + lim - 1) / lim;
                }
                ఒకవేళ (anonL1s != 1) {
                    putsDecoration(
                        శూన్యసూచిక,
                        "// Anonymous structures to workaround compiler member-count bugs\n");
                }
                కొలత_t nEmitted = 0;
                కొరకు (కొలత_t l3 = 0; l3 < anonL3s && nEmitted < nTriv; ++l3) {
                    ఒకవేళ (anonL3s != 1) puts("struct {\n");
                    కొరకు (కొలత_t l2 = 0; l2 < anonL2s && nEmitted < nTriv; ++l2) {
                        ఒకవేళ (anonL2s != 1) puts("struct {\n");
                        కొరకు (కొలత_t l1 = 0; l1 < anonL1s && nEmitted < nTriv; ++l1) {
                            ఒకవేళ (anonL1s != 1) puts("struct {\n");
                            కొరకు (కొలత_t l0 = 0; l0 < lim && nEmitted < nTriv; ++l0, ++nEmitted) {
                                emit_And_అడుగు(trivp);
                            }
                            ఒకవేళ (anonL1s != 1) puts("};\n");
                        }
                        ఒకవేళ (anonL2s != 1) puts("};\n");
                    }
                    ఒకవేళ (anonL3s != 1) puts("};\n");
                }
                UASSERT_OBJ(nEmitted == nTriv, modp, "Not all variables emitted");
            }
        }

        // Emit not trivially constructible variables
        ఒకవేళ (nNont) {
            putsDecoration(శూన్యసూచిక, "\n// NOT TRIVIALLY CONSTRUCTIBLE STATE\n");
            // Classes use real constructors
            ఒకవేళ (VN_IS(modp, Class)) {
                కొరకు (కొలత_t i = 0; i < nNont; ++i) emit_And_అడుగు(nontp);
                తిరిగి;
            }
            // Output under an anonymous union to prevent the C++ compiler from generating
            // constructor/destructor calls for them. They are managed explicitly in ctor/dtor.
            // While 'union { singleVar; }' for each variable would be simpler, it blows up
            // clang's compile time when using pre-compiled headers, joy.
            puts("union {\nstruct {\n");
            కొరకు (కొలత_t i = 0; i < nNont; ++i) emit_And_అడుగు(nontp);
            puts("} __Vnontrivs;\n};\n");
        }
    }
    శూన్య emit_అంతర్గత_Var_Decls(స్థిరరాశి Ast_కణుపు_Module* modp) {
        ఒకవేళ (స్థిరరాశి AstClass* స్థిరరాశి classp = VN_CAST(modp, Class)) {
            ఒకవేళ (classp->needRNG()) {
                putsDecoration(శూన్యసూచిక, "\n// INTERNAL VARIABLES\n");
                puts("VlRNGReseeds __Vm_rng;\n");
            }
        } లేకపోతే {  // not class
            putsDecoration(శూన్యసూచిక, "\n// INTERNAL VARIABLES\n");
            puts(EmitCUtil::symClassName() + "* vlSymsp;\n");
            puts("const char* vlNamep;\n");
            // Allocate object-local coverage counters only on modules that
            // contain emitted coverage declarations.
            స్థిరరాశి పూర్ణాంకం coverBins = Cover_లెక్క_Visitor{modp}.bins();
            ఒకవేళ (coverBins) {
                puts(v3Global.opt.దారాలు() > 1 ? "std::atomic<uint32_t>" : "uint32_t");
                puts(" __Vcoverage[");
                puts(std::గమ్యం_string(coverBins));
                puts("]{};\n");
            }
        }
    }
    శూన్య emitParamDecls(స్థిరరాశి Ast_కణుపు_Module* modp) {
        తార్కిక మొదటి = నిజం;
        కొరకు (Ast_కణుపు* nodep = modp->stmtsp(); nodep; nodep = nodep->nextp()) {
            ఒకవేళ (స్థిరరాశి AstVar* స్థిరరాశి varp = VN_CAST(nodep, Var)) {
                ఒకవేళ (varp->isParam()) {
                    decorate_మొదటి(మొదటి, "\n// PARAMETERS\n");
                    putns(varp, "static const ");
                    puts(varp->dtypep()->cType(varp->nameProtect(), అబద్ధం, అబద్ధం));
                    puts(";\n");
                }
            }
        }
    }
    శూన్య emitCtorDtorDecls(స్థిరరాశి Ast_కణుపు_Module* modp) {
        // Classes use CFuncs with isConstructor/isDestructor
        ఒకవేళ (VN_IS(modp, Class)) తిరిగి;

        // Proper modules use loose split ctor/dtor and are not copyable/movable
        స్థిరరాశి std::string name = EmitCUtil::ఉపసర్గ_Name_Protect(modp);
        putsDecoration(శూన్యసూచిక, "\n// CONSTRUCTORS\n");
        putns(modp, name + "(" + EmitCUtil::symClassName() + "* symsp, const char* namep);\n");
        putns(modp, "~" + name + "();\n");
        putns(modp, "VL_UNCOPYABLE(" + name + ");\n");
        putns(modp, "VL_UNMOVABLE(" + name + ");\n");
    }
    శూన్య emit_అంతర్గత_పద్ధతి_Decls(స్థిరరాశి Ast_కణుపు_Module* modp) {
        తార్కిక మొదటి = నిజం;
        స్థిరరాశి string section = "\n// INTERNAL METHODS\n";

        ఒకవేళ (!VN_IS(modp, Class)) {
            decorate_మొదటి(మొదటి, section);
            ఒకవేళ (v3Global.opt.coverage()) {
                puts("void " + protect("__Vconfigure") + "(bool first);\n");
            }
        } లేకపోతే {
            decorate_మొదటి(మొదటి, section);
            స్థిరరాశి std::string name = V3_బహిర్గతం_Formatter::quoteNameControls(
                VIdProtect::protect_పదాలు_If(modp->prettyName(), v3Global.opt.protectIds()));
            // "override", but don't want clang to start checking them:
            puts("virtual const char* typeName() const { return \""s + name + "\"; }\n");
        }

        ఒకవేళ (v3Global.opt.coverage() && !VN_IS(modp, Class)) {
            decorate_మొదటి(మొదటి, section);
            puts("void __vlCoverInsert(");
            puts(v3Global.opt.దారాలు() > 1 ? "std::atomic<uint32_t>" : "uint32_t");
            puts("* countp, bool enable, bool localCounter, const char* filenamep, int lineno, "
                 "int column,\n");
            puts("const char* hierp, const char* pagep, const char* commentp, const char* "
                 "linescovp,\n");
            puts("const char* fsmVarp, const char* fsmFromp, const char* fsmTop, const char* "
                 "fsmTagp);\n");
        }

        ఒకవేళ (v3Global.opt.coverageToggle() && !VN_IS(modp, Class)) {
            decorate_మొదటి(మొదటి, section);
            puts("void __vlCoverToggleInsert(int begin, int end, bool ranged, ");
            puts(v3Global.opt.దారాలు() > 1 ? "std::atomic<uint32_t>" : "uint32_t");
            puts("* countp, bool enable, bool localCounter, const char* filenamep, int lineno, "
                 "int column,\n");
            puts("const char* hierp, const char* pagep, const char* commentp);\n");
        }

        ఒకవేళ (v3Global.opt.savable()) {
            decorate_మొదటి(మొదటి, section);
            puts("void " + protect("__Vserialize") + "(VerilatedSerialize& os);\n");
            puts("void " + protect("__Vdeserialize") + "(VerilatedDeserialize& os);\n");
        }

        // Polymorphic clone for concrete (non-abstract, non-interface) classes
        ఒకవేళ (స్థిరరాశి AstClass* స్థిరరాశి classp = VN_CAST(modp, Class)) {
            ఒకవేళ (!classp->isInterfaceClass() && !classp->isVirtual()) {
                decorate_మొదటి(మొదటి, section);
                ఉపయోగించి EmbeddedCovergroupVar = std::pair<స్థిరరాశి AstClass*, స్థిరరాశి AstVar*>;
                std::సదిశ<EmbeddedCovergroupVar> embeddedCovergroupVars;
                స్థిరరాశి స్వయంచాలక has_Enclosing_వెనుక_సూచిక = [](స్థిరరాశి AstClass* covergroupp) {
                    తిరిగి covergroupp->exists([](స్థిరరాశి AstVar* స్థిరరాశి varp) {
                        స్థిరరాశి AstClassRefDType* స్థిరరాశి refp
                            = VN_CAST(varp->dtypep()->skipRefp(), ClassRefDType);
                        తిరిగి refp && refp->raw_సూచిక();
                    });
                };
                const_cast<AstClass*>(classp)->foreachMember(
                    [&](AstClass* స్థిరరాశి memberClassp, AstVar* స్థిరరాశి varp) {
                        స్థిరరాశి AstClassRefDType* స్థిరరాశి refp
                            = VN_CAST(varp->dtypep()->skipRefp(), ClassRefDType);
                        ఒకవేళ (refp && refp->classp()->isCovergroup()
                            && has_Enclosing_వెనుక_సూచిక(refp->classp())) {
                            embeddedCovergroupVars.emplace_వెనుక(memberClassp, varp);
                        }
                    });
                స్థిరరాశి string className = EmitCUtil::ఉపసర్గ_Name_Protect(classp);
                ఒకవేళ (embeddedCovergroupVars.ఖాళీలు() && !classp->hasRandVarsUpdate()) {
                    putns(classp,
                          "VlClass* clone() const { return new " + className + "(*this); }\n");
                } లేకపోతే {
                    putns(classp, "VlClass* clone() const { " + className + "* const clonep = new "
                                      + className + "(*this); ");
                    ఒకవేళ (classp->hasRandVarsUpdate()) {
                        స్థిరరాశి string updateName = "__VnoInFunc___VupdateRandVars";
                        AstCFunc* స్థిరరాశి updatep
                            = VN_AS(m_memberMap.కనుగొను_Member(classp, updateName), CFunc);
                        UASSERT_OBJ(updatep, classp, "Missing updateRandVars method");
                        puts("clonep->" + updatep->nameProtect() + "();\n");
                    }
                    కొరకు (స్థిరరాశి EmbeddedCovergroupVar& అంశం_2 : embeddedCovergroupVars) {
                        puts("clonep->" + EmitCUtil::ఉపసర్గ_Name_Protect(అంశం_2.మొదటి)
                             + "::" + అంశం_2.రెండవ->nameProtect() + " = VlNull{}; ");
                    }
                    puts("return clonep; }\n");
                }
            }
        }
    }
    శూన్య emitEnums(స్థిరరాశి Ast_కణుపు_Module* modp) {
        తార్కిక మొదటి = నిజం;
        కొరకు (స్థిరరాశి Ast_కణుపు* nodep = modp->stmtsp(); nodep; nodep = nodep->nextp()) {
            స్థిరరాశి AstTypedef* స్థిరరాశి tdefp = VN_CAST(nodep, Typedef);
            ఒకవేళ (!tdefp) కొనసాగు;
            ఒకవేళ (!tdefp->attr_ప్రజా()) కొనసాగు;
            స్థిరరాశి AstEnumDType* స్థిరరాశి edtypep
                = VN_CAST(tdefp->dtypep()->skip_Ref_గమ్యం_Enump(), EnumDType);
            ఒకవేళ (!edtypep) కొనసాగు;
            decorate_మొదటి(మొదటి, "\n// ENUMS (that were declared public)\n");
            ఒకవేళ (edtypep->వెడల్పు() > 64) {
                putsDecoration(tdefp,
                               "// enum " + tdefp->nameProtect() + " ignored: Too wide for C++\n");
            } లేకపోతే {
                putns(tdefp, "enum " + tdefp->name() + " {\n");
                కొరకు (స్థిరరాశి Ast_Enum_అంశం* itemp = edtypep->itemsp(); itemp;
                     itemp = VN_AS(itemp->nextp(), Enum_అంశం)) {
                    ఒకవేళ (స్థిరరాశి AstConst* స్థిరరాశి constp = VN_CAST(itemp->valuep(), Const)) {
                        ఒకవేళ (constp->అంకె().isFourState()) {
                            putns(itemp, "// " + itemp->nameProtect() + " is four-state\n");
                            కొనసాగు;
                        }
                    }
                    putns(itemp, itemp->nameProtect());
                    puts(" = ");
                    putns(itemp, VN_AS(itemp->valuep(), Const)->అంకె().emitC());
                    ఒకవేళ (VN_IS(itemp->nextp(), Enum_అంశం)) puts(",");
                    puts("\n");
                }
                puts("};\n");
            }
        }
    }
    శూన్య emitStructDecl(స్థిరరాశి Ast_కణుపు_Module* modp, Ast_కణుపు_UOr_Struct_DType* sdtypep,
                        std::సమితి<Ast_కణుపు_UOr_Struct_DType*>& emitted) {
        ఒకవేళ (emitted.లెక్క(sdtypep) > 0) తిరిగి;
        emitted.చొప్పించు(sdtypep);
        కొరకు (AstMemberDType* itemp = sdtypep->membersp(); itemp;
             itemp = VN_AS(itemp->nextp(), MemberDType)) {
            Ast_కణుపు_UOr_Struct_DType* స్థిరరాశి subp = itemp->get_బిడ్డ_Structp();
            ఒకవేళ (subp && (!subp->packed() || sdtypep->packed())) {
                // Recurse if it belongs to the current module
                ఒకవేళ (subp->classOrPackagep() == modp) {
                    emitStructDecl(modp, subp, emitted);
                    puts("\n");
                }
            }
        }
        ఒకవేళ (sdtypep->packed()) {
            emitPackedUOrSBody(sdtypep);
        } లేకపోతే {
            emitUnpackedUOrSBody(sdtypep);
        }
    }
    గణన తరగతి AttributeType : uint8_t { వెడల్పు_2, DIMENSION };
    // Get member attribute based on type
    పూర్ణాంకం get_కణుపు_Attribute(స్థిరరాశి AstMemberDType* itemp, AttributeType type) {
        స్థిరరాశి తార్కిక isArrayType = itemp->dtypep()->isNonPackedArray();
        ఎంపిక (type) {
        స్థితి AttributeType::వెడల్పు_2: {
            ఒకవేళ (isArrayType) {
                // For arrays, get innermost element width
                స్థిరరాశి Ast_కణుపు_DType* dtype = itemp->dtypep();
                అయితే (dtype->subDTypep()) dtype = dtype->subDTypep();
                తిరిగి dtype->వెడల్పు();
            }
            తిరిగి itemp->వెడల్పు();
        }
        స్థితి AttributeType::DIMENSION: {
            // Return array dimension or 0 for non-arrays
            తిరిగి isArrayType ? itemp->dtypep()->dimensions(నిజం).రెండవ : 0;
        }
        అప్రమేయ: {
            తిరిగి 0;
        }
        }
    }
    మూస <AttributeType T>
    శూన్య emit_Member_సదిశ(స్థిరరాశి Ast_కణుపు_UOr_Struct_DType* sdtypep) {
        puts("return {");
        తార్కిక needComma = అబద్ధం;
        కొరకు (స్థిరరాశి AstMemberDType* itemp = sdtypep->membersp(); itemp;
             itemp = VN_AS(itemp->nextp(), MemberDType)) {
            ఒకవేళ (!itemp->isConstrainedRand()) కొనసాగు;
            // Comma handling: add before element except first
            ఒకవేళ (needComma) puts(",\n");
            putns(itemp, std::గమ్యం_string(get_కణుపు_Attribute(itemp, T)));
            needComma = నిజం;
        }
        puts("};\n}\n");
    }
    శూన్య emitUnpackedUOrSBody(Ast_కణుపు_UOr_Struct_DType* sdtypep) {
        putns(sdtypep, sdtypep->verilogKwd());  // "struct"/"union"
        puts(" " + EmitCUtil::ఉపసర్గ_Name_Protect(sdtypep) + " {\n");
        కొరకు (స్థిరరాశి AstMemberDType* itemp = sdtypep->membersp(); itemp;
             itemp = VN_AS(itemp->nextp(), MemberDType)) {
            putns(itemp, itemp->dtypep()->cType(itemp->nameProtect(), అబద్ధం, అబద్ధం));
            puts(";\n");
        }

        // Three helper functions for struct constrained randomization:
        // - memberNames: Get member names
        // - getMembers: Access member references
        // - memberIndices: Retrieve member indices
        // - memberWidth: Retrieve member width
        // - memberDimension: Retrieve member dimension
        ఒకవేళ (sdtypep->isConstrainedRand()) {
            తార్కిక needComma = అబద్ధం;
            putns(sdtypep, "\nstd::vector<std::string> memberNames(void) const {\n");
            puts("return {");
            కొరకు (స్థిరరాశి AstMemberDType* itemp = sdtypep->membersp(); itemp;
                 itemp = VN_AS(itemp->nextp(), MemberDType)) {
                ఒకవేళ (!itemp->isConstrainedRand()) కొనసాగు;
                ఒకవేళ (needComma) puts(",\n");
                putns(itemp, "\"" + itemp->shortName() + "\"");
                needComma = నిజం;
            }
            puts("};\n}\n");

            putns(sdtypep, "\nstd::vector<int> memberWidth(void) const {\n");
            emit_Member_సదిశ<AttributeType::వెడల్పు_2>(sdtypep);

            putns(sdtypep, "\nstd::vector<int> memberDimension(void) const {\n");
            emit_Member_సదిశ<AttributeType::DIMENSION>(sdtypep);

            needComma = అబద్ధం;
            putns(sdtypep, "\nauto memberIndices(void) const {\n");
            puts("return std::index_sequence_for<");
            కొరకు (స్థిరరాశి AstMemberDType* itemp = sdtypep->membersp(); itemp;
                 itemp = VN_AS(itemp->nextp(), MemberDType)) {
                ఒకవేళ (!itemp->isConstrainedRand()) కొనసాగు;
                ఒకవేళ (needComma) puts(",\n");
                putns(itemp, itemp->dtypep()->cType("", అబద్ధం, అబద్ధం));
                needComma = నిజం;
            }
            puts(">{};\n}\n");

            needComma = అబద్ధం;
            putns(sdtypep, "\ntemplate <typename T>");
            putns(sdtypep, "\nauto getMembers(T& obj) {\n");
            puts("return std::tie(");
            కొరకు (స్థిరరాశి AstMemberDType* itemp = sdtypep->membersp(); itemp;
                 itemp = VN_AS(itemp->nextp(), MemberDType)) {
                ఒకవేళ (!itemp->isConstrainedRand()) కొనసాగు;
                ఒకవేళ (needComma) puts(",\n");
                putns(itemp, "obj." + itemp->nameProtect());
                needComma = నిజం;
            }
            puts(");\n}\n");
        }
        putns(sdtypep, "\nbool operator==(const " + EmitCUtil::ఉపసర్గ_Name_Protect(sdtypep)
                           + "& rhs) const {\n");
        puts("return ");
        కొరకు (స్థిరరాశి AstMemberDType* itemp = sdtypep->membersp(); itemp;
             itemp = VN_AS(itemp->nextp(), MemberDType)) {
            ఒకవేళ (itemp != sdtypep->membersp()) puts("\n    && ");
            putns(itemp, itemp->nameProtect() + " == " + "rhs." + itemp->nameProtect());
        }
        puts(";\n");
        puts("}\n");
        putns(sdtypep, "bool operator!=(const " + EmitCUtil::ఉపసర్గ_Name_Protect(sdtypep)
                           + "& rhs) const {\n");
        puts("return !(*this == rhs);\n}\n");
        putns(sdtypep, "\nbool operator<(const " + EmitCUtil::ఉపసర్గ_Name_Protect(sdtypep)
                           + "& rhs) const {\n");
        కొరకు (స్థిరరాశి AstMemberDType* itemp = sdtypep->membersp(); itemp;
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
        putns(sdtypep, "struct VlIsCustomStruct<" + EmitCUtil::ఉపసర్గ_Name_Protect(sdtypep)
                           + "> : public std::true_type {};\n");
    }

    // getfunc: VL_ASSIGNSEL_XX(rbits, obits, off, lhsdata, rhsdata);
    // !getfunc: VL_SELASSIGN_XX(rbits, obits, lhsdata, rhsdata, off);
    శూన్య emit_Vl_కేటాయించు(స్థిరరాశి Ast_కణుపు_DType* స్థిరరాశి lhstype, స్థిరరాశి Ast_కణుపు_DType* rhstype,
                      స్థిరరాశి std::string& off, స్థిరరాశి std::string& lhsdata,
                      స్థిరరాశి std::string& rhsdata, తార్కిక getfunc) {
        puts(getfunc ? "VL_ASSIGNSEL_" : "VL_SELASSIGN_");
        puts(lhstype->charIQWN());
        puts(rhstype->charIQWN());
        puts("(" + std::గమ్యం_string(lhstype->వెడల్పు()) + ", ");  // LHS width
        ఒకవేళ (getfunc) {
            puts(std::గమ్యం_string(rhstype->వెడల్పు()) + ", ");  // Number of copy bits
            puts(off + ", ");  // LHS offset
        } లేకపోతే {
            // Number of copy bits. Use widthTototalBytes to
            // make VL_SELASSIGN_XX clear upper unused bits for us.
            // puts(std::to_string(lhstype->width()) + ", ");
            puts(std::గమ్యం_string(lhstype->వెడల్పు_మొత్తం_బైట్లు() * 8) + ", ");
        }
        puts(lhsdata + ", ");  // LHS data
        puts(rhsdata);  // RHS data
        ఒకవేళ (!getfunc) {
            puts(", " + off);  // RHS offset
        }
        puts(");\n");
    }

    // `retOrArg` should be prefixed by `&` or suffixed by `.data()` depending on its type
    శూన్య emitPackedMember(స్థిరరాశి Ast_కణుపు_DType* మాతృ_Dtypep, స్థిరరాశి Ast_కణుపు_DType* dtypep,
                          స్థిరరాశి std::string& fieldname, స్థిరరాశి std::string& స్థానభ్రంశం, తార్కిక getfunc,
                          స్థిరరాశి std::string& retOrArg) {
        dtypep = dtypep->skipRefp();
        ఒకవేళ (స్థిరరాశి స్వయంచాలక* adtypep = VN_CAST(dtypep, PackArrayDType)) {
            స్థిరరాశి std::string సూచీ = m_పేర్లు.get("__Vi");
            puts("for (int " + సూచీ + " = 0; " + సూచీ + " < "
                 + std::గమ్యం_string(adtypep->elementsConst()) + "; ++" + సూచీ + ") {\n");

            స్థిరరాశి std::string స్థానభ్రంశం_లోపల_Loop
                = స్థానభ్రంశం + " + " + సూచీ + " * " + std::గమ్యం_string(adtypep->subDTypep()->వెడల్పు());
            స్థిరరాశి std::string newName = fieldname + "[" + సూచీ + "]";
            emitPackedMember(మాతృ_Dtypep, adtypep->subDTypep(), newName, స్థానభ్రంశం_లోపల_Loop, getfunc,
                             retOrArg);
            puts("}\n");
        } లేకపోతే ఒకవేళ (VN_IS(dtypep, కణుపు_UOr_Struct_DType)) {
            స్థిరరాశి std::string tmp = m_పేర్లు.get("__Vtmp");
            ఒకవేళ (getfunc) {  // Emit `get` func;
                // auto __tmp = field.get();
                puts("auto " + tmp + " = " + fieldname + ".get();\n");
                // VL_ASSIGNSEL_XX(rbits, obits, lsb, lhsdata, rhsdata);
                emit_Vl_కేటాయించు(మాతృ_Dtypep, dtypep, స్థానభ్రంశం, retOrArg, tmp, getfunc);
            } లేకపోతే {  // Emit `set` func
                స్థిరరాశి std::string tmptype = AstCDType::type_గమ్యం_Hold(dtypep->వెడల్పు());
                // type tmp;
                puts(tmptype + " " + tmp + ";\n");
                // VL_SELASSIGN_XX(rbits, obits, lhsdata, rhsdata, roffset);
                emit_Vl_కేటాయించు(dtypep, మాతృ_Dtypep, స్థానభ్రంశం, tmp, retOrArg, getfunc);
                // field.set(__tmp);
                puts(fieldname + ".set(" + tmp + ");\n");
            }
        } లేకపోతే {
            UASSERT_OBJ(VN_IS(dtypep, EnumDType) || VN_IS(dtypep, BasicDType), dtypep,
                        "Unsupported type in packed struct or union");
            ఒకవేళ (getfunc) {  // Emit `get` func;
                // VL_ASSIGNSEL_XX(rbits, obits, lsb, lhsdata, rhsdata);
                emit_Vl_కేటాయించు(మాతృ_Dtypep, dtypep, స్థానభ్రంశం, retOrArg, fieldname, getfunc);
            } లేకపోతే {  // Emit `set` func
                // VL_SELASSIGN_XX(rbits, obits, lhsdata, rhsdata, roffset);
                emit_Vl_కేటాయించు(dtypep, మాతృ_Dtypep, స్థానభ్రంశం, fieldname, retOrArg, getfunc);
            }
        }
    }
    శూన్య emitPackedUOrSBody(Ast_కణుపు_UOr_Struct_DType* sdtypep) {
        putns(sdtypep, sdtypep->verilogKwd());  // "struct"/"union"
        puts(" " + EmitCUtil::ఉపసర్గ_Name_Protect(sdtypep) + " {\n");

        AstMemberDType* itemp;
        AstMemberDType* చివరి_Itemp;
        స్థిరరాశి AstMemberDType* witemp = శూన్యసూచిక;
        // LSB is first field in C, so loop backwards
        కొరకు (చివరి_Itemp = sdtypep->membersp(); చివరి_Itemp && చివరి_Itemp->nextp();
             చివరి_Itemp = VN_AS(చివరి_Itemp->nextp(), MemberDType)) {
            ఒకవేళ (చివరి_Itemp->వెడల్పు() == sdtypep->వెడల్పు()) witemp = చివరి_Itemp;
        }
        కొరకు (itemp = చివరి_Itemp; itemp; itemp = VN_CAST(itemp->backp(), MemberDType)) {
            putns(itemp, itemp->dtypep()->cType(itemp->nameProtect(), అబద్ధం, అబద్ధం, నిజం));
            puts(";\n");
        }

        స్థిరరాశి std::string retArgName = m_పేర్లు.get("__v");
        స్థిరరాశి std::string retArgType = AstCDType::type_గమ్యం_Hold(sdtypep->వెడల్పు());

        // Emit `get` member function
        puts(retArgType + " get() const {\n");
        puts(retArgType + " " + retArgName + ";\n");
        ఒకవేళ (VN_IS(sdtypep, StructDType)) {
            కొరకు (itemp = చివరి_Itemp; itemp; itemp = VN_CAST(itemp->backp(), MemberDType)) {
                emitPackedMember(sdtypep, itemp->dtypep(), itemp->nameProtect(),
                                 std::గమ్యం_string(itemp->lsb()), /*getfunc=*/నిజం, retArgName);
            }
        } లేకపోతే {
            // We only need to fill the widest field of union
            emitPackedMember(sdtypep, witemp->dtypep(), witemp->nameProtect(),
                             std::గమ్యం_string(witemp->lsb()), /*getfunc=*/నిజం, retArgName);
        }
        puts("return " + retArgName + ";\n");
        puts("}\n");

        // Emit `set` member function
        puts("void set(const " + retArgType + "& " + retArgName + ") {\n");
        ఒకవేళ (VN_IS(sdtypep, StructDType)) {
            కొరకు (itemp = చివరి_Itemp; itemp; itemp = VN_CAST(itemp->backp(), MemberDType)) {
                emitPackedMember(sdtypep, itemp->dtypep(), itemp->nameProtect(),
                                 std::గమ్యం_string(itemp->lsb()), /*getfunc=*/అబద్ధం, retArgName);
            }
        } లేకపోతే {
            // We only need to fill the widest field of union
            emitPackedMember(sdtypep, witemp->dtypep(), witemp->nameProtect(),
                             std::గమ్యం_string(witemp->lsb()), /*getfunc=*/అబద్ధం, retArgName);
        }

        puts("}\n");

        puts("};\n");
        m_పేర్లు.reset();
    }
    శూన్య emitStructs(స్థిరరాశి Ast_కణుపు_Module* modp) {
        // Track structs that've been emitted already
        std::సమితి<Ast_కణుపు_UOr_Struct_DType*> emitted;
        కొరకు (స్థిరరాశి Ast_కణుపు* nodep = modp->stmtsp(); nodep; nodep = nodep->nextp()) {
            స్థిరరాశి AstTypedef* స్థిరరాశి tdefp = VN_CAST(nodep, Typedef);
            ఒకవేళ (!tdefp) కొనసాగు;
            Ast_కణుపు_UOr_Struct_DType* స్థిరరాశి sdtypep
                = VN_CAST(tdefp->dtypep()->skip_Ref_గమ్యం_Enump(), కణుపు_UOr_Struct_DType);
            ఒకవేళ (!sdtypep) కొనసాగు;
            // V3Inline can copy one module into several parents. Only the module that
            // declares the struct emits it, so C++ sees only one definition.
            స్థిరరాశి తార్కిక declared_లోపల_Other_Module = sdtypep->classOrPackagep() != modp;
            ఒకవేళ (declared_లోపల_Other_Module) కొనసాగు;
            emitStructDecl(modp, sdtypep, emitted);
        }
    }
    శూన్య emitFuncDecls(స్థిరరాశి Ast_కణుపు_Module* modp, తార్కిక లోపల_Class_Body) {
        std::సదిశ<స్థిరరాశి AstCFunc*> funcsp;

        కొరకు (స్థిరరాశి Ast_కణుపు* nodep = modp->stmtsp(); nodep; nodep = nodep->nextp()) {
            ఒకవేళ (స్థిరరాశి AstCFunc* స్థిరరాశి funcp = VN_CAST(nodep, CFunc)) {
                ఒకవేళ (funcp->dpiImportPrototype())  // Declared in __Dpi.h
                    కొనసాగు;
                ఒకవేళ (funcp->dpiExportDispatcher())  // Declared in __Dpi.h
                    కొనసాగు;
                ఒకవేళ (funcp->is_పద్ధతి() != లోపల_Class_Body)  // Only methods go inside class
                    కొనసాగు;
                ఒకవేళ (funcp->is_పద్ధతి() && funcp->isLoose())  // Loose methods are declared lazily
                    కొనసాగు;
                funcsp.నెట్టు_వెనుక(funcp);
            }
        }

        std::stable_క్రమీకరించు(funcsp.begin(), funcsp.చివర(), [](స్థిరరాశి Ast_కణుపు* ap, స్థిరరాశి Ast_కణుపు* bp) {
            తిరిగి ap->name() < bp->name();
        });

        కొరకు (స్థిరరాశి AstCFunc* స్థిరరాశి funcp : funcsp) {
            ఒకవేళ (లోపల_Class_Body) ofp()->puts_వ్యక్తిగత(funcp->decl_వ్యక్తిగత());
            emitCFuncDecl(funcp, modp);
        }
    }
    శూన్య emitAll(స్థిరరాశి Ast_కణుపు_Module* modp) {
        // Include files required by this AstNodeModule
        ఒకవేళ (స్థిరరాశి AstClass* స్థిరరాశి classp = VN_CAST(modp, Class)) {
            కొరకు (స్థిరరాశి AstClassExtends* extp = classp->extendsp(); extp;
                 extp = VN_AS(extp->nextp(), ClassExtends)) {
                putns(extp, "#include \""
                                + EmitCUtil::ఉపసర్గ_Name_Protect(extp->classp()->classOrPackagep())
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
        ఒకవేళ (!VN_IS(modp, Class)) puts("alignas(VL_CACHE_LINE_BYTES) ");
        puts(EmitCUtil::ఉపసర్గ_Name_Protect(modp));
        ఒకవేళ (స్థిరరాశి AstClass* స్థిరరాశి classp = VN_CAST(modp, Class)) {
            puts(" : ");
            ఒకవేళ (classp->extendsp()) {
                తార్కిక needComma = అబద్ధం;
                కొరకు (స్థిరరాశి AstClassExtends* extp = classp->extendsp(); extp;
                     extp = VN_AS(extp->nextp(), ClassExtends)) {
                    ఒకవేళ (needComma) puts(", ");
                    // Use virtual only for interfaces for class inheritance
                    // (extends)
                    puts(extp->classp()->use_Virtual_ప్రజా() ? "virtual public " : "public ");
                    putns(extp, EmitCUtil::ఉపసర్గ_Name_Protect(extp->classp()));
                    needComma = నిజం;
                }
            } లేకపోతే {
                puts("public virtual VlClass");
            }
        } లేకపోతే {
            puts(" final");
        }
        puts(" {\n");
        ofp()->reset_వ్యక్తిగత();
        ofp()->puts_వ్యక్తిగత(అబద్ధం);  // public:

        // Emit all class body contents
        emitCellDecls(modp);
        emitEnums(modp);
        emitDesignVarDecls(modp);
        emit_అంతర్గత_Var_Decls(modp);
        emitParamDecls(modp);
        emitCtorDtorDecls(modp);
        emit_అంతర్గత_పద్ధతి_Decls(modp);
        emitFuncDecls(modp, /* inClassBody: */ నిజం);

        // From `systemc_interface
        emitSystemCSection(modp, VSystemCSectionType::INT);

        // Close class body
        puts("};\n");
        // }}}

        // Emit out of class function declarations
        puts("\n");
        emitFuncDecls(modp, /* inClassBody: */ అబద్ధం);
        emitSystemCSection(modp, VSystemCSectionType::HDR_POST);
    }

    // VISITORS
    శూన్య సందర్శించు(Ast_కణుపు* nodep) override { nodep->v3fatalSrc("Unused"); }  // LCOV_EXCL_LINE

    స్పష్టమైన EmitCHeader(స్థిరరాశి Ast_కణుపు_Module* modp) {
        UINFO(5, "  Emitting header for " << EmitCUtil::ఉపసర్గ_Name_Protect(modp));

        // Open output file
        open_New_Output_శీర్షిక_File(EmitCUtil::ఉపసర్గ_Name_Protect(modp), "Design internal header");
        puts("// See " + EmitCUtil::శిఖరం_Class_Name() + ".h for the primary calling header\n");

        ofp()->putsGuard();

        // Include files
        puts("\n");
        ofp()->puts_Int_శిఖరం_Include();
        puts("#include \"verilated.h\"\n");
        ఒకవేళ (modp->is_శిఖరం() && optSystemC()) puts("#include \"verilated_sc.h\"\n");
        ఒకవేళ (v3Global.opt.mtasks()) puts("#include \"verilated_threads.h\"\n");
        ఒకవేళ (v3Global.opt.savable()) puts("#include \"verilated_save.h\"\n");
        ఒకవేళ (v3Global.opt.coverage()) puts("#include \"verilated_cov.h\"\n");
        ఒకవేళ (v3Global.opt.coverage() || v3Global.useCovergroup())
            puts("#include \"verilated_covergroup.h\"\n");
        ఒకవేళ (v3Global.ఉపయోగిస్తుంది_Timing()) puts("#include \"verilated_timing.h\"\n");
        ఒకవేళ (v3Global.useRandomizeMethods()) puts("#include \"verilated_random.h\"\n");
        ఒకవేళ (v3Global.ఉపయోగిస్తుంది_Force()) puts("#include \"verilated_force.h\"\n");

        std::సమితి<string> cuse_సమితి;
        స్వయంచాలక కూడు_గమ్యం_cuse_సమితి = [&](string s) { cuse_సమితి.చొప్పించు(s); };

        forModCUse(modp, VUseType::INT_FWD_CLASS | VUseType::INT_INCLUDE, కూడు_గమ్యం_cuse_సమితి);
        ఒకవేళ (స్థిరరాశి AstClassPackage* స్థిరరాశి packagep = VN_CAST(modp, ClassPackage)) {
            forModCUse(packagep->classp(), VUseType::INT_INCLUDE | VUseType::INT_FWD_CLASS,
                       కూడు_గమ్యం_cuse_సమితి);
        }

        కొరకు (స్థిరరాశి string& s : cuse_సమితి) puts(s);
        puts("\n");

        emitAll(modp);

        ఒకవేళ (స్థిరరాశి AstClassPackage* స్థిరరాశి packagep = VN_CAST(modp, ClassPackage)) {
            // Put the non-static class implementation in same h file for speed
            emitAll(packagep->classp());
        }

        ofp()->puts_చివర_Guard();

        // Close output file
        closeOutputFile();
    }
    ~EmitCHeader() override = అప్రమేయ;

బహిరంగ:
    స్థిర శూన్య ప్రధాన(స్థిరరాశి Ast_కణుపు_Module* modp) { స్థిరరాశి EmitCHeader emitCHeader{modp}; }
};

//######################################################################
// EmitC class functions

శూన్య V3EmitC::emitc_శీర్షికలు() {
    UINFO(2, __FUNCTION__ << ":");

    // Process each module in turn
    కొరకు (స్థిరరాశి Ast_కణుపు* nodep = v3Global.rootp()->modulesp(); nodep; nodep = nodep->nextp()) {
        ఒకవేళ (VN_IS(nodep, Class)) కొనసాగు;  // Declared with the ClassPackage
        స్థిరరాశి Ast_కణుపు_Module* స్థిరరాశి modp = VN_AS(nodep, కణుపు_Module);
        ఒకవేళ (modp->isConstPool()) కొనసాగు;  // Emitted by V3EmitCConstPool
        EmitCHeader::ప్రధాన(modp);
    }
}
