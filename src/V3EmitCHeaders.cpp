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
#ضمن "V3EmitCBase.h"
#ضمن "V3File.h"
#ضمن "V3MemberMap.h"
#ضمن "V3UniqueNames.h"

#ضمن <algorithm>
#ضمن <cstdint>
#ضمن <set>
#ضمن <string>
#ضمن <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Internal EmitC implementation

صنف EmitCHeader final : عام EmitCBaseVisitorConst {
    V3_Unique_نالا m_نالا;
    VMemberMap m_memberMap;
    // METHODS

    صنف Cover_ڳڻپ_Visitor final : عام VNVisitorConst {
        صحيح m_bins = 0;  // Running total of coverage bins counted so far

        فراغ گهمو(Ast_ڳنڍ_Cover_Decl* nodep) override {
            // Each module class owns the counters for declarations it emits;
            // duplicate no-inline instances need separate arrays for
            // forcePerInstance reporting.
            إذا (!nodep->معلومات_Decl_Nullp()) m_bins += nodep->سائيز();
        }
        فراغ گهمو(Ast_ڳنڍ* nodep) override { iterateChildrenConst(nodep); }

    عام:
        واضح Cover_ڳڻپ_Visitor(ثابت Ast_ڳنڍ_Module* modp) {
            iterateConst(const_cast<Ast_ڳنڍ_Module*>(modp));
        }
        صحيح bins() ثابت { أرجع m_bins; }
    };

    فراغ decorate_پهريون(منطقي& پهريون, ثابت string& str) {
        إذا (پهريون) {
            putsDecoration(خالي_اشارو, str);
            پهريون = خطأ;
        }
    }
    فراغ emitCellDecls(ثابت Ast_ڳنڍ_Module* modp) {
        منطقي پهريون = سچ;
        لكل (ثابت Ast_ڳنڍ* nodep = modp->stmtsp(); nodep; nodep = nodep->nextp()) {
            إذا (ثابت AstCell* ثابت cellp = VN_CAST(nodep, Cell)) {
                إذا (cellp->modp()->isConstPool()) استمر;  // Special emit rules
                decorate_پهريون(پهريون, "// CELLS\n");
                putns(cellp, EmitCUtil::اڳياڙي_Name_Protect(cellp->modp()) + "* "
                                 + cellp->nameProtect() + ";\n");
            }
        }
    }
    فراغ emitDesignVarDecls(ثابت Ast_ڳنڍ_Module* modp) {
        // V3VariableOrder puts all variables at the front of the list: top level ports, then
        // trivially constructible variables, then the not trivially constructible ones, then
        // those that are not model state
        ثابت AstVar* portp = خالي_اشارو;
        ثابت AstVar* trivp = خالي_اشارو;
        ثابت AstVar* nontp = خالي_اشارو;
        سائيز_t n_پورٽ = 0;
        سائيز_t nTriv = 0;
        سائيز_t nNont = 0;
        لكل (ثابت Ast_ڳنڍ* nodep = modp->stmtsp(); nodep; nodep = nodep->nextp()) {
            ثابت AstVar* ثابت varp = VN_CAST(nodep, Var);
            إذا (!varp) اكسر;
            إذا (!varp->isModelState()) {
                // Cache line alignment is only effective on emitted fields
                UASSERT_OBJ(!varp->mtask_Cache_سٽ_Align(), varp, "Aligned non-field");
            } وإلا إذا (varp->isPrimaryIO()) {
                إذا (!portp) portp = varp;
                ++n_پورٽ;
            } وإلا إذا (varp->isTriviallyConstructible()) {
                إذا (!trivp) trivp = varp;
                ++nTriv;
            } وإلا {
                إذا (!nontp) nontp = varp;
                ++nNont;
            }
        }

        ثابت خودڪار emit_And_قدم = [&](ثابت AstVar*& varpr) {
            emitVarDecl(varpr);
            varpr = VN_CAST(varpr->nextp(), Var);
        };

        // Emit primary ports
        إذا (n_پورٽ) {
            putsDecoration(خالي_اشارو, "\n// TOP LEVEL PORTS\n");
            // In anonymous union to prevent the C++ compiler from generating
            // constructor/destructor calls
            puts("union {\nstruct {\n");
            لكل (سائيز_t i = 0; i < n_پورٽ; ++i) emit_And_قدم(portp);
            puts("} __VprimaryPorts;\n};\n");
        }

        // Emit trivially constructible variables
        إذا (nTriv) {
            putsDecoration(خالي_اشارو, "\n// TRIVIALLY CONSTRUCTIBLE STATE\n");
            إذا (v3Global.opt.comp_حد_بندي_Members() == 0) {
                // Anonymous structs disabled, so just emit them directly
                لكل (سائيز_t i = 0; i < nTriv; ++i) emit_And_قدم(trivp);
            } وإلا {
                ثابت سائيز_t lim = v3Global.opt.comp_حد_بندي_Members();
                سائيز_t anonL3s = 1;
                سائيز_t anonL2s = 1;
                سائيز_t anonL1s = 1;
                إذا (nTriv > (lim * lim * lim)) {
                    anonL3s = (nTriv + (lim * lim * lim) - 1) / (lim * lim * lim);
                    anonL2s = lim;
                    anonL1s = lim;
                } وإلا إذا (nTriv > (lim * lim)) {
                    anonL2s = (nTriv + (lim * lim) - 1) / (lim * lim);
                    anonL1s = lim;
                } وإلا إذا (nTriv > lim) {
                    anonL1s = (nTriv + lim - 1) / lim;
                }
                إذا (anonL1s != 1) {
                    putsDecoration(
                        خالي_اشارو,
                        "// Anonymous structures to workaround compiler member-count bugs\n");
                }
                سائيز_t nEmitted = 0;
                لكل (سائيز_t l3 = 0; l3 < anonL3s && nEmitted < nTriv; ++l3) {
                    إذا (anonL3s != 1) puts("struct {\n");
                    لكل (سائيز_t l2 = 0; l2 < anonL2s && nEmitted < nTriv; ++l2) {
                        إذا (anonL2s != 1) puts("struct {\n");
                        لكل (سائيز_t l1 = 0; l1 < anonL1s && nEmitted < nTriv; ++l1) {
                            إذا (anonL1s != 1) puts("struct {\n");
                            لكل (سائيز_t l0 = 0; l0 < lim && nEmitted < nTriv; ++l0, ++nEmitted) {
                                emit_And_قدم(trivp);
                            }
                            إذا (anonL1s != 1) puts("};\n");
                        }
                        إذا (anonL2s != 1) puts("};\n");
                    }
                    إذا (anonL3s != 1) puts("};\n");
                }
                UASSERT_OBJ(nEmitted == nTriv, modp, "Not all variables emitted");
            }
        }

        // Emit not trivially constructible variables
        إذا (nNont) {
            putsDecoration(خالي_اشارو, "\n// NOT TRIVIALLY CONSTRUCTIBLE STATE\n");
            // Classes use real constructors
            إذا (VN_IS(modp, Class)) {
                لكل (سائيز_t i = 0; i < nNont; ++i) emit_And_قدم(nontp);
                أرجع;
            }
            // Output under an anonymous union to prevent the C++ compiler from generating
            // constructor/destructor calls for them. They are managed explicitly in ctor/dtor.
            // While 'union { singleVar; }' for each variable would be simpler, it blows up
            // clang's compile time when using pre-compiled headers, joy.
            puts("union {\nstruct {\n");
            لكل (سائيز_t i = 0; i < nNont; ++i) emit_And_قدم(nontp);
            puts("} __Vnontrivs;\n};\n");
        }
    }
    فراغ emit_اندروني_Var_Decls(ثابت Ast_ڳنڍ_Module* modp) {
        إذا (ثابت AstClass* ثابت classp = VN_CAST(modp, Class)) {
            إذا (classp->needRNG()) {
                putsDecoration(خالي_اشارو, "\n// INTERNAL VARIABLES\n");
                puts("VlRNGReseeds __Vm_rng;\n");
            }
        } وإلا {  // not class
            putsDecoration(خالي_اشارو, "\n// INTERNAL VARIABLES\n");
            puts(EmitCUtil::symClassName() + "* vlSymsp;\n");
            puts("const char* vlNamep;\n");
            // Allocate object-local coverage counters only on modules that
            // contain emitted coverage declarations.
            ثابت صحيح coverBins = Cover_ڳڻپ_Visitor{modp}.bins();
            إذا (coverBins) {
                puts(v3Global.opt.ڌاڳا() > 1 ? "std::atomic<uint32_t>" : "uint32_t");
                puts(" __Vcoverage[");
                puts(std::منزل_string(coverBins));
                puts("]{};\n");
            }
        }
    }
    فراغ emitParamDecls(ثابت Ast_ڳنڍ_Module* modp) {
        منطقي پهريون = سچ;
        لكل (Ast_ڳنڍ* nodep = modp->stmtsp(); nodep; nodep = nodep->nextp()) {
            إذا (ثابت AstVar* ثابت varp = VN_CAST(nodep, Var)) {
                إذا (varp->isParam()) {
                    decorate_پهريون(پهريون, "\n// PARAMETERS\n");
                    putns(varp, "static const ");
                    puts(varp->dtypep()->cType(varp->nameProtect(), خطأ, خطأ));
                    puts(";\n");
                }
            }
        }
    }
    فراغ emitCtorDtorDecls(ثابت Ast_ڳنڍ_Module* modp) {
        // Classes use CFuncs with isConstructor/isDestructor
        إذا (VN_IS(modp, Class)) أرجع;

        // Proper modules use loose split ctor/dtor and are not copyable/movable
        ثابت std::string name = EmitCUtil::اڳياڙي_Name_Protect(modp);
        putsDecoration(خالي_اشارو, "\n// CONSTRUCTORS\n");
        putns(modp, name + "(" + EmitCUtil::symClassName() + "* symsp, const char* namep);\n");
        putns(modp, "~" + name + "();\n");
        putns(modp, "VL_UNCOPYABLE(" + name + ");\n");
        putns(modp, "VL_UNMOVABLE(" + name + ");\n");
    }
    فراغ emit_اندروني_طريقو_Decls(ثابت Ast_ڳنڍ_Module* modp) {
        منطقي پهريون = سچ;
        ثابت string section = "\n// INTERNAL METHODS\n";

        إذا (!VN_IS(modp, Class)) {
            decorate_پهريون(پهريون, section);
            إذا (v3Global.opt.coverage()) {
                puts("void " + protect("__Vconfigure") + "(bool first);\n");
            }
        } وإلا {
            decorate_پهريون(پهريون, section);
            ثابت std::string name = V3_ٻاهريون_Formatter::quoteNameControls(
                VIdProtect::protect_لفظ_If(modp->prettyName(), v3Global.opt.protectIds()));
            // "override", but don't want clang to start checking them:
            puts("virtual const char* typeName() const { return \""s + name + "\"; }\n");
        }

        إذا (v3Global.opt.coverage() && !VN_IS(modp, Class)) {
            decorate_پهريون(پهريون, section);
            puts("void __vlCoverInsert(");
            puts(v3Global.opt.ڌاڳا() > 1 ? "std::atomic<uint32_t>" : "uint32_t");
            puts("* countp, bool enable, bool localCounter, const char* filenamep, int lineno, "
                 "int column,\n");
            puts("const char* hierp, const char* pagep, const char* commentp, const char* "
                 "linescovp,\n");
            puts("const char* fsmVarp, const char* fsmFromp, const char* fsmTop, const char* "
                 "fsmTagp);\n");
        }

        إذا (v3Global.opt.coverageToggle() && !VN_IS(modp, Class)) {
            decorate_پهريون(پهريون, section);
            puts("void __vlCoverToggleInsert(int begin, int end, bool ranged, ");
            puts(v3Global.opt.ڌاڳا() > 1 ? "std::atomic<uint32_t>" : "uint32_t");
            puts("* countp, bool enable, bool localCounter, const char* filenamep, int lineno, "
                 "int column,\n");
            puts("const char* hierp, const char* pagep, const char* commentp);\n");
        }

        إذا (v3Global.opt.savable()) {
            decorate_پهريون(پهريون, section);
            puts("void " + protect("__Vserialize") + "(VerilatedSerialize& os);\n");
            puts("void " + protect("__Vdeserialize") + "(VerilatedDeserialize& os);\n");
        }

        // Polymorphic clone for concrete (non-abstract, non-interface) classes
        إذا (ثابت AstClass* ثابت classp = VN_CAST(modp, Class)) {
            إذا (!classp->isInterfaceClass() && !classp->isVirtual()) {
                decorate_پهريون(پهريون, section);
                استعمال EmbeddedCovergroupVar = std::pair<ثابت AstClass*, ثابت AstVar*>;
                std::ويڪٽر<EmbeddedCovergroupVar> embeddedCovergroupVars;
                ثابت خودڪار has_Enclosing_پٺ_اشارو = [](ثابت AstClass* covergroupp) {
                    أرجع covergroupp->exists([](ثابت AstVar* ثابت varp) {
                        ثابت AstClassRefDType* ثابت refp
                            = VN_CAST(varp->dtypep()->skipRefp(), ClassRefDType);
                        أرجع refp && refp->raw_اشارو();
                    });
                };
                const_cast<AstClass*>(classp)->foreachMember(
                    [&](AstClass* ثابت memberClassp, AstVar* ثابت varp) {
                        ثابت AstClassRefDType* ثابت refp
                            = VN_CAST(varp->dtypep()->skipRefp(), ClassRefDType);
                        إذا (refp && refp->classp()->isCovergroup()
                            && has_Enclosing_پٺ_اشارو(refp->classp())) {
                            embeddedCovergroupVars.emplace_پٺ(memberClassp, varp);
                        }
                    });
                ثابت string className = EmitCUtil::اڳياڙي_Name_Protect(classp);
                إذا (embeddedCovergroupVars.خالي_جايون() && !classp->hasRandVarsUpdate()) {
                    putns(classp,
                          "VlClass* clone() const { return new " + className + "(*this); }\n");
                } وإلا {
                    putns(classp, "VlClass* clone() const { " + className + "* const clonep = new "
                                      + className + "(*this); ");
                    إذا (classp->hasRandVarsUpdate()) {
                        ثابت string updateName = "__VnoInFunc___VupdateRandVars";
                        AstCFunc* ثابت updatep
                            = VN_AS(m_memberMap.ڳوليو_Member(classp, updateName), CFunc);
                        UASSERT_OBJ(updatep, classp, "Missing updateRandVars method");
                        puts("clonep->" + updatep->nameProtect() + "();\n");
                    }
                    لكل (ثابت EmbeddedCovergroupVar& شي_2 : embeddedCovergroupVars) {
                        puts("clonep->" + EmitCUtil::اڳياڙي_Name_Protect(شي_2.پهريون)
                             + "::" + شي_2.ٻيو->nameProtect() + " = VlNull{}; ");
                    }
                    puts("return clonep; }\n");
                }
            }
        }
    }
    فراغ emitEnums(ثابت Ast_ڳنڍ_Module* modp) {
        منطقي پهريون = سچ;
        لكل (ثابت Ast_ڳنڍ* nodep = modp->stmtsp(); nodep; nodep = nodep->nextp()) {
            ثابت AstTypedef* ثابت tdefp = VN_CAST(nodep, Typedef);
            إذا (!tdefp) استمر;
            إذا (!tdefp->attr_عوامي()) استمر;
            ثابت AstEnumDType* ثابت edtypep
                = VN_CAST(tdefp->dtypep()->skip_Ref_منزل_Enump(), EnumDType);
            إذا (!edtypep) استمر;
            decorate_پهريون(پهريون, "\n// ENUMS (that were declared public)\n");
            إذا (edtypep->ويڪر() > 64) {
                putsDecoration(tdefp,
                               "// enum " + tdefp->nameProtect() + " ignored: Too wide for C++\n");
            } وإلا {
                putns(tdefp, "enum " + tdefp->name() + " {\n");
                لكل (ثابت Ast_Enum_شي* itemp = edtypep->itemsp(); itemp;
                     itemp = VN_AS(itemp->nextp(), Enum_شي)) {
                    إذا (ثابت AstConst* ثابت constp = VN_CAST(itemp->valuep(), Const)) {
                        إذا (constp->انگ().isFourState()) {
                            putns(itemp, "// " + itemp->nameProtect() + " is four-state\n");
                            استمر;
                        }
                    }
                    putns(itemp, itemp->nameProtect());
                    puts(" = ");
                    putns(itemp, VN_AS(itemp->valuep(), Const)->انگ().emitC());
                    إذا (VN_IS(itemp->nextp(), Enum_شي)) puts(",");
                    puts("\n");
                }
                puts("};\n");
            }
        }
    }
    فراغ emitStructDecl(ثابت Ast_ڳنڍ_Module* modp, Ast_ڳنڍ_UOr_Struct_DType* sdtypep,
                        std::سيٽ<Ast_ڳنڍ_UOr_Struct_DType*>& emitted) {
        إذا (emitted.ڳڻپ(sdtypep) > 0) أرجع;
        emitted.وجهو(sdtypep);
        لكل (AstMemberDType* itemp = sdtypep->membersp(); itemp;
             itemp = VN_AS(itemp->nextp(), MemberDType)) {
            Ast_ڳنڍ_UOr_Struct_DType* ثابت subp = itemp->get_ٻار_Structp();
            إذا (subp && (!subp->packed() || sdtypep->packed())) {
                // Recurse if it belongs to the current module
                إذا (subp->classOrPackagep() == modp) {
                    emitStructDecl(modp, subp, emitted);
                    puts("\n");
                }
            }
        }
        إذا (sdtypep->packed()) {
            emitPackedUOrSBody(sdtypep);
        } وإلا {
            emitUnpackedUOrSBody(sdtypep);
        }
    }
    تعداد صنف AttributeType : uint8_t { ويڪر_2, DIMENSION };
    // Get member attribute based on type
    صحيح get_ڳنڍ_Attribute(ثابت AstMemberDType* itemp, AttributeType type) {
        ثابت منطقي isArrayType = itemp->dtypep()->isNonPackedArray();
        بدل (type) {
        حالة AttributeType::ويڪر_2: {
            إذا (isArrayType) {
                // For arrays, get innermost element width
                ثابت Ast_ڳنڍ_DType* dtype = itemp->dtypep();
                طالما (dtype->subDTypep()) dtype = dtype->subDTypep();
                أرجع dtype->ويڪر();
            }
            أرجع itemp->ويڪر();
        }
        حالة AttributeType::DIMENSION: {
            // Return array dimension or 0 for non-arrays
            أرجع isArrayType ? itemp->dtypep()->dimensions(سچ).ٻيو : 0;
        }
        افتراضي: {
            أرجع 0;
        }
        }
    }
    سانچو <AttributeType T>
    فراغ emit_Member_ويڪٽر(ثابت Ast_ڳنڍ_UOr_Struct_DType* sdtypep) {
        puts("return {");
        منطقي needComma = خطأ;
        لكل (ثابت AstMemberDType* itemp = sdtypep->membersp(); itemp;
             itemp = VN_AS(itemp->nextp(), MemberDType)) {
            إذا (!itemp->isConstrainedRand()) استمر;
            // Comma handling: add before element except first
            إذا (needComma) puts(",\n");
            putns(itemp, std::منزل_string(get_ڳنڍ_Attribute(itemp, T)));
            needComma = سچ;
        }
        puts("};\n}\n");
    }
    فراغ emitUnpackedUOrSBody(Ast_ڳنڍ_UOr_Struct_DType* sdtypep) {
        putns(sdtypep, sdtypep->verilogKwd());  // "struct"/"union"
        puts(" " + EmitCUtil::اڳياڙي_Name_Protect(sdtypep) + " {\n");
        لكل (ثابت AstMemberDType* itemp = sdtypep->membersp(); itemp;
             itemp = VN_AS(itemp->nextp(), MemberDType)) {
            putns(itemp, itemp->dtypep()->cType(itemp->nameProtect(), خطأ, خطأ));
            puts(";\n");
        }

        // Three helper functions for struct constrained randomization:
        // - memberNames: Get member names
        // - getMembers: Access member references
        // - memberIndices: Retrieve member indices
        // - memberWidth: Retrieve member width
        // - memberDimension: Retrieve member dimension
        إذا (sdtypep->isConstrainedRand()) {
            منطقي needComma = خطأ;
            putns(sdtypep, "\nstd::vector<std::string> memberNames(void) const {\n");
            puts("return {");
            لكل (ثابت AstMemberDType* itemp = sdtypep->membersp(); itemp;
                 itemp = VN_AS(itemp->nextp(), MemberDType)) {
                إذا (!itemp->isConstrainedRand()) استمر;
                إذا (needComma) puts(",\n");
                putns(itemp, "\"" + itemp->shortName() + "\"");
                needComma = سچ;
            }
            puts("};\n}\n");

            putns(sdtypep, "\nstd::vector<int> memberWidth(void) const {\n");
            emit_Member_ويڪٽر<AttributeType::ويڪر_2>(sdtypep);

            putns(sdtypep, "\nstd::vector<int> memberDimension(void) const {\n");
            emit_Member_ويڪٽر<AttributeType::DIMENSION>(sdtypep);

            needComma = خطأ;
            putns(sdtypep, "\nauto memberIndices(void) const {\n");
            puts("return std::index_sequence_for<");
            لكل (ثابت AstMemberDType* itemp = sdtypep->membersp(); itemp;
                 itemp = VN_AS(itemp->nextp(), MemberDType)) {
                إذا (!itemp->isConstrainedRand()) استمر;
                إذا (needComma) puts(",\n");
                putns(itemp, itemp->dtypep()->cType("", خطأ, خطأ));
                needComma = سچ;
            }
            puts(">{};\n}\n");

            needComma = خطأ;
            putns(sdtypep, "\ntemplate <typename T>");
            putns(sdtypep, "\nauto getMembers(T& obj) {\n");
            puts("return std::tie(");
            لكل (ثابت AstMemberDType* itemp = sdtypep->membersp(); itemp;
                 itemp = VN_AS(itemp->nextp(), MemberDType)) {
                إذا (!itemp->isConstrainedRand()) استمر;
                إذا (needComma) puts(",\n");
                putns(itemp, "obj." + itemp->nameProtect());
                needComma = سچ;
            }
            puts(");\n}\n");
        }
        putns(sdtypep, "\nbool operator==(const " + EmitCUtil::اڳياڙي_Name_Protect(sdtypep)
                           + "& rhs) const {\n");
        puts("return ");
        لكل (ثابت AstMemberDType* itemp = sdtypep->membersp(); itemp;
             itemp = VN_AS(itemp->nextp(), MemberDType)) {
            إذا (itemp != sdtypep->membersp()) puts("\n    && ");
            putns(itemp, itemp->nameProtect() + " == " + "rhs." + itemp->nameProtect());
        }
        puts(";\n");
        puts("}\n");
        putns(sdtypep, "bool operator!=(const " + EmitCUtil::اڳياڙي_Name_Protect(sdtypep)
                           + "& rhs) const {\n");
        puts("return !(*this == rhs);\n}\n");
        putns(sdtypep, "\nbool operator<(const " + EmitCUtil::اڳياڙي_Name_Protect(sdtypep)
                           + "& rhs) const {\n");
        لكل (ثابت AstMemberDType* itemp = sdtypep->membersp(); itemp;
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
        putns(sdtypep, "struct VlIsCustomStruct<" + EmitCUtil::اڳياڙي_Name_Protect(sdtypep)
                           + "> : public std::true_type {};\n");
    }

    // getfunc: VL_ASSIGNSEL_XX(rbits, obits, off, lhsdata, rhsdata);
    // !getfunc: VL_SELASSIGN_XX(rbits, obits, lhsdata, rhsdata, off);
    فراغ emit_Vl_سونپيو(ثابت Ast_ڳنڍ_DType* ثابت lhstype, ثابت Ast_ڳنڍ_DType* rhstype,
                      ثابت std::string& off, ثابت std::string& lhsdata,
                      ثابت std::string& rhsdata, منطقي getfunc) {
        puts(getfunc ? "VL_ASSIGNSEL_" : "VL_SELASSIGN_");
        puts(lhstype->charIQWN());
        puts(rhstype->charIQWN());
        puts("(" + std::منزل_string(lhstype->ويڪر()) + ", ");  // LHS width
        إذا (getfunc) {
            puts(std::منزل_string(rhstype->ويڪر()) + ", ");  // Number of copy bits
            puts(off + ", ");  // LHS offset
        } وإلا {
            // Number of copy bits. Use widthTototalBytes to
            // make VL_SELASSIGN_XX clear upper unused bits for us.
            // puts(std::to_string(lhstype->width()) + ", ");
            puts(std::منزل_string(lhstype->ويڪر_ڪل_بائيٽون() * 8) + ", ");
        }
        puts(lhsdata + ", ");  // LHS data
        puts(rhsdata);  // RHS data
        إذا (!getfunc) {
            puts(", " + off);  // RHS offset
        }
        puts(");\n");
    }

    // `retOrArg` should be prefixed by `&` or suffixed by `.data()` depending on its type
    فراغ emitPackedMember(ثابت Ast_ڳنڍ_DType* والد_Dtypep, ثابت Ast_ڳنڍ_DType* dtypep,
                          ثابت std::string& fieldname, ثابت std::string& وٿي, منطقي getfunc,
                          ثابت std::string& retOrArg) {
        dtypep = dtypep->skipRefp();
        إذا (ثابت خودڪار* adtypep = VN_CAST(dtypep, PackArrayDType)) {
            ثابت std::string اشاريو = m_نالا.get("__Vi");
            puts("for (int " + اشاريو + " = 0; " + اشاريو + " < "
                 + std::منزل_string(adtypep->elementsConst()) + "; ++" + اشاريو + ") {\n");

            ثابت std::string وٿي_اندريون_Loop
                = وٿي + " + " + اشاريو + " * " + std::منزل_string(adtypep->subDTypep()->ويڪر());
            ثابت std::string newName = fieldname + "[" + اشاريو + "]";
            emitPackedMember(والد_Dtypep, adtypep->subDTypep(), newName, وٿي_اندريون_Loop, getfunc,
                             retOrArg);
            puts("}\n");
        } وإلا إذا (VN_IS(dtypep, ڳنڍ_UOr_Struct_DType)) {
            ثابت std::string tmp = m_نالا.get("__Vtmp");
            إذا (getfunc) {  // Emit `get` func;
                // auto __tmp = field.get();
                puts("auto " + tmp + " = " + fieldname + ".get();\n");
                // VL_ASSIGNSEL_XX(rbits, obits, lsb, lhsdata, rhsdata);
                emit_Vl_سونپيو(والد_Dtypep, dtypep, وٿي, retOrArg, tmp, getfunc);
            } وإلا {  // Emit `set` func
                ثابت std::string tmptype = AstCDType::type_منزل_Hold(dtypep->ويڪر());
                // type tmp;
                puts(tmptype + " " + tmp + ";\n");
                // VL_SELASSIGN_XX(rbits, obits, lhsdata, rhsdata, roffset);
                emit_Vl_سونپيو(dtypep, والد_Dtypep, وٿي, tmp, retOrArg, getfunc);
                // field.set(__tmp);
                puts(fieldname + ".set(" + tmp + ");\n");
            }
        } وإلا {
            UASSERT_OBJ(VN_IS(dtypep, EnumDType) || VN_IS(dtypep, BasicDType), dtypep,
                        "Unsupported type in packed struct or union");
            إذا (getfunc) {  // Emit `get` func;
                // VL_ASSIGNSEL_XX(rbits, obits, lsb, lhsdata, rhsdata);
                emit_Vl_سونپيو(والد_Dtypep, dtypep, وٿي, retOrArg, fieldname, getfunc);
            } وإلا {  // Emit `set` func
                // VL_SELASSIGN_XX(rbits, obits, lhsdata, rhsdata, roffset);
                emit_Vl_سونپيو(dtypep, والد_Dtypep, وٿي, fieldname, retOrArg, getfunc);
            }
        }
    }
    فراغ emitPackedUOrSBody(Ast_ڳنڍ_UOr_Struct_DType* sdtypep) {
        putns(sdtypep, sdtypep->verilogKwd());  // "struct"/"union"
        puts(" " + EmitCUtil::اڳياڙي_Name_Protect(sdtypep) + " {\n");

        AstMemberDType* itemp;
        AstMemberDType* آخري_Itemp;
        ثابت AstMemberDType* witemp = خالي_اشارو;
        // LSB is first field in C, so loop backwards
        لكل (آخري_Itemp = sdtypep->membersp(); آخري_Itemp && آخري_Itemp->nextp();
             آخري_Itemp = VN_AS(آخري_Itemp->nextp(), MemberDType)) {
            إذا (آخري_Itemp->ويڪر() == sdtypep->ويڪر()) witemp = آخري_Itemp;
        }
        لكل (itemp = آخري_Itemp; itemp; itemp = VN_CAST(itemp->backp(), MemberDType)) {
            putns(itemp, itemp->dtypep()->cType(itemp->nameProtect(), خطأ, خطأ, سچ));
            puts(";\n");
        }

        ثابت std::string retArgName = m_نالا.get("__v");
        ثابت std::string retArgType = AstCDType::type_منزل_Hold(sdtypep->ويڪر());

        // Emit `get` member function
        puts(retArgType + " get() const {\n");
        puts(retArgType + " " + retArgName + ";\n");
        إذا (VN_IS(sdtypep, StructDType)) {
            لكل (itemp = آخري_Itemp; itemp; itemp = VN_CAST(itemp->backp(), MemberDType)) {
                emitPackedMember(sdtypep, itemp->dtypep(), itemp->nameProtect(),
                                 std::منزل_string(itemp->lsb()), /*getfunc=*/سچ, retArgName);
            }
        } وإلا {
            // We only need to fill the widest field of union
            emitPackedMember(sdtypep, witemp->dtypep(), witemp->nameProtect(),
                             std::منزل_string(witemp->lsb()), /*getfunc=*/سچ, retArgName);
        }
        puts("return " + retArgName + ";\n");
        puts("}\n");

        // Emit `set` member function
        puts("void set(const " + retArgType + "& " + retArgName + ") {\n");
        إذا (VN_IS(sdtypep, StructDType)) {
            لكل (itemp = آخري_Itemp; itemp; itemp = VN_CAST(itemp->backp(), MemberDType)) {
                emitPackedMember(sdtypep, itemp->dtypep(), itemp->nameProtect(),
                                 std::منزل_string(itemp->lsb()), /*getfunc=*/خطأ, retArgName);
            }
        } وإلا {
            // We only need to fill the widest field of union
            emitPackedMember(sdtypep, witemp->dtypep(), witemp->nameProtect(),
                             std::منزل_string(witemp->lsb()), /*getfunc=*/خطأ, retArgName);
        }

        puts("}\n");

        puts("};\n");
        m_نالا.reset();
    }
    فراغ emitStructs(ثابت Ast_ڳنڍ_Module* modp) {
        // Track structs that've been emitted already
        std::سيٽ<Ast_ڳنڍ_UOr_Struct_DType*> emitted;
        لكل (ثابت Ast_ڳنڍ* nodep = modp->stmtsp(); nodep; nodep = nodep->nextp()) {
            ثابت AstTypedef* ثابت tdefp = VN_CAST(nodep, Typedef);
            إذا (!tdefp) استمر;
            Ast_ڳنڍ_UOr_Struct_DType* ثابت sdtypep
                = VN_CAST(tdefp->dtypep()->skip_Ref_منزل_Enump(), ڳنڍ_UOr_Struct_DType);
            إذا (!sdtypep) استمر;
            // V3Inline can copy one module into several parents. Only the module that
            // declares the struct emits it, so C++ sees only one definition.
            ثابت منطقي declared_اندريون_Other_Module = sdtypep->classOrPackagep() != modp;
            إذا (declared_اندريون_Other_Module) استمر;
            emitStructDecl(modp, sdtypep, emitted);
        }
    }
    فراغ emitFuncDecls(ثابت Ast_ڳنڍ_Module* modp, منطقي اندريون_Class_Body) {
        std::ويڪٽر<ثابت AstCFunc*> funcsp;

        لكل (ثابت Ast_ڳنڍ* nodep = modp->stmtsp(); nodep; nodep = nodep->nextp()) {
            إذا (ثابت AstCFunc* ثابت funcp = VN_CAST(nodep, CFunc)) {
                إذا (funcp->dpiImportPrototype())  // Declared in __Dpi.h
                    استمر;
                إذا (funcp->dpiExportDispatcher())  // Declared in __Dpi.h
                    استمر;
                إذا (funcp->is_طريقو() != اندريون_Class_Body)  // Only methods go inside class
                    استمر;
                إذا (funcp->is_طريقو() && funcp->isLoose())  // Loose methods are declared lazily
                    استمر;
                funcsp.ڌڪو_پٺ(funcp);
            }
        }

        std::stable_ترتيب_ڏيو(funcsp.begin(), funcsp.پڇاڙي(), [](ثابت Ast_ڳنڍ* ap, ثابت Ast_ڳنڍ* bp) {
            أرجع ap->name() < bp->name();
        });

        لكل (ثابت AstCFunc* ثابت funcp : funcsp) {
            إذا (اندريون_Class_Body) ofp()->puts_ذاتي(funcp->decl_ذاتي());
            emitCFuncDecl(funcp, modp);
        }
    }
    فراغ emitAll(ثابت Ast_ڳنڍ_Module* modp) {
        // Include files required by this AstNodeModule
        إذا (ثابت AstClass* ثابت classp = VN_CAST(modp, Class)) {
            لكل (ثابت AstClassExtends* extp = classp->extendsp(); extp;
                 extp = VN_AS(extp->nextp(), ClassExtends)) {
                putns(extp, "#include \""
                                + EmitCUtil::اڳياڙي_Name_Protect(extp->classp()->classOrPackagep())
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
        إذا (!VN_IS(modp, Class)) puts("alignas(VL_CACHE_LINE_BYTES) ");
        puts(EmitCUtil::اڳياڙي_Name_Protect(modp));
        إذا (ثابت AstClass* ثابت classp = VN_CAST(modp, Class)) {
            puts(" : ");
            إذا (classp->extendsp()) {
                منطقي needComma = خطأ;
                لكل (ثابت AstClassExtends* extp = classp->extendsp(); extp;
                     extp = VN_AS(extp->nextp(), ClassExtends)) {
                    إذا (needComma) puts(", ");
                    // Use virtual only for interfaces for class inheritance
                    // (extends)
                    puts(extp->classp()->use_Virtual_عوامي() ? "virtual public " : "public ");
                    putns(extp, EmitCUtil::اڳياڙي_Name_Protect(extp->classp()));
                    needComma = سچ;
                }
            } وإلا {
                puts("public virtual VlClass");
            }
        } وإلا {
            puts(" final");
        }
        puts(" {\n");
        ofp()->reset_ذاتي();
        ofp()->puts_ذاتي(خطأ);  // public:

        // Emit all class body contents
        emitCellDecls(modp);
        emitEnums(modp);
        emitDesignVarDecls(modp);
        emit_اندروني_Var_Decls(modp);
        emitParamDecls(modp);
        emitCtorDtorDecls(modp);
        emit_اندروني_طريقو_Decls(modp);
        emitFuncDecls(modp, /* inClassBody: */ سچ);

        // From `systemc_interface
        emitSystemCSection(modp, VSystemCSectionType::INT);

        // Close class body
        puts("};\n");
        // }}}

        // Emit out of class function declarations
        puts("\n");
        emitFuncDecls(modp, /* inClassBody: */ خطأ);
        emitSystemCSection(modp, VSystemCSectionType::HDR_POST);
    }

    // VISITORS
    فراغ گهمو(Ast_ڳنڍ* nodep) override { nodep->v3fatalSrc("Unused"); }  // LCOV_EXCL_LINE

    واضح EmitCHeader(ثابت Ast_ڳنڍ_Module* modp) {
        UINFO(5, "  Emitting header for " << EmitCUtil::اڳياڙي_Name_Protect(modp));

        // Open output file
        open_New_Output_سرو_File(EmitCUtil::اڳياڙي_Name_Protect(modp), "Design internal header");
        puts("// See " + EmitCUtil::چوٽي_Class_Name() + ".h for the primary calling header\n");

        ofp()->putsGuard();

        // Include files
        puts("\n");
        ofp()->puts_Int_چوٽي_Include();
        puts("#include \"verilated.h\"\n");
        إذا (modp->is_چوٽي() && optSystemC()) puts("#include \"verilated_sc.h\"\n");
        إذا (v3Global.opt.mtasks()) puts("#include \"verilated_threads.h\"\n");
        إذا (v3Global.opt.savable()) puts("#include \"verilated_save.h\"\n");
        إذا (v3Global.opt.coverage()) puts("#include \"verilated_cov.h\"\n");
        إذا (v3Global.opt.coverage() || v3Global.useCovergroup())
            puts("#include \"verilated_covergroup.h\"\n");
        إذا (v3Global.استعمال_ڪري_Timing()) puts("#include \"verilated_timing.h\"\n");
        إذا (v3Global.useRandomizeMethods()) puts("#include \"verilated_random.h\"\n");
        إذا (v3Global.استعمال_ڪري_Force()) puts("#include \"verilated_force.h\"\n");

        std::سيٽ<string> cuse_سيٽ;
        خودڪار جوڙيو_منزل_cuse_سيٽ = [&](string s) { cuse_سيٽ.وجهو(s); };

        forModCUse(modp, VUseType::INT_FWD_CLASS | VUseType::INT_INCLUDE, جوڙيو_منزل_cuse_سيٽ);
        إذا (ثابت AstClassPackage* ثابت packagep = VN_CAST(modp, ClassPackage)) {
            forModCUse(packagep->classp(), VUseType::INT_INCLUDE | VUseType::INT_FWD_CLASS,
                       جوڙيو_منزل_cuse_سيٽ);
        }

        لكل (ثابت string& s : cuse_سيٽ) puts(s);
        puts("\n");

        emitAll(modp);

        إذا (ثابت AstClassPackage* ثابت packagep = VN_CAST(modp, ClassPackage)) {
            // Put the non-static class implementation in same h file for speed
            emitAll(packagep->classp());
        }

        ofp()->puts_پڇاڙي_Guard();

        // Close output file
        closeOutputFile();
    }
    ~EmitCHeader() override = افتراضي;

عام:
    ساكن فراغ مکيه(ثابت Ast_ڳنڍ_Module* modp) { ثابت EmitCHeader emitCHeader{modp}; }
};

//######################################################################
// EmitC class functions

فراغ V3EmitC::emitc_سرا() {
    UINFO(2, __FUNCTION__ << ":");

    // Process each module in turn
    لكل (ثابت Ast_ڳنڍ* nodep = v3Global.rootp()->modulesp(); nodep; nodep = nodep->nextp()) {
        إذا (VN_IS(nodep, Class)) استمر;  // Declared with the ClassPackage
        ثابت Ast_ڳنڍ_Module* ثابت modp = VN_AS(nodep, ڳنڍ_Module);
        إذا (modp->isConstPool()) استمر;  // Emitted by V3EmitCConstPool
        EmitCHeader::مکيه(modp);
    }
}
