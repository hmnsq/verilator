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
    V3_Unique_نامها m_نامها;
    VMemberMap m_memberMap;
    // METHODS

    صنف Cover_شمار_Visitor final : عام VNVisitorConst {
        صحيح m_bins = 0;  // Running total of coverage bins counted so far

        فراغ بازدید(Ast_گره_Cover_Decl* nodep) override {
            // Each module class owns the counters for declarations it emits;
            // duplicate no-inline instances need separate arrays for
            // forcePerInstance reporting.
            إذا (!nodep->داده_Decl_Nullp()) m_bins += nodep->اندازه();
        }
        فراغ بازدید(Ast_گره* nodep) override { iterateChildrenConst(nodep); }

    عام:
        صریح Cover_شمار_Visitor(ثابت Ast_گره_Module* modp) {
            iterateConst(const_cast<Ast_گره_Module*>(modp));
        }
        صحيح bins() ثابت { أرجع m_bins; }
    };

    فراغ decorate_اولی(منطقي& اولی, ثابت string& str) {
        إذا (اولی) {
            putsDecoration(اشاره_تهی, str);
            اولی = خطأ;
        }
    }
    فراغ emitCellDecls(ثابت Ast_گره_Module* modp) {
        منطقي اولی = درست;
        لكل (ثابت Ast_گره* nodep = modp->stmtsp(); nodep; nodep = nodep->nextp()) {
            إذا (ثابت AstCell* ثابت cellp = VN_CAST(nodep, Cell)) {
                إذا (cellp->modp()->isConstPool()) استمر;  // Special emit rules
                decorate_اولی(اولی, "// CELLS\n");
                putns(cellp, EmitCUtil::پیشوند_Name_Protect(cellp->modp()) + "* "
                                 + cellp->nameProtect() + ";\n");
            }
        }
    }
    فراغ emitDesignVarDecls(ثابت Ast_گره_Module* modp) {
        // V3VariableOrder puts all variables at the front of the list: top level ports, then
        // trivially constructible variables, then the not trivially constructible ones, then
        // those that are not model state
        ثابت AstVar* portp = اشاره_تهی;
        ثابت AstVar* trivp = اشاره_تهی;
        ثابت AstVar* nontp = اشاره_تهی;
        اندازه_t n_پورت = 0;
        اندازه_t nTriv = 0;
        اندازه_t nNont = 0;
        لكل (ثابت Ast_گره* nodep = modp->stmtsp(); nodep; nodep = nodep->nextp()) {
            ثابت AstVar* ثابت varp = VN_CAST(nodep, Var);
            إذا (!varp) اكسر;
            إذا (!varp->isModelState()) {
                // Cache line alignment is only effective on emitted fields
                UASSERT_OBJ(!varp->mtask_Cache_سطر_Align(), varp, "Aligned non-field");
            } وإلا إذا (varp->isPrimaryIO()) {
                إذا (!portp) portp = varp;
                ++n_پورت;
            } وإلا إذا (varp->isTriviallyConstructible()) {
                إذا (!trivp) trivp = varp;
                ++nTriv;
            } وإلا {
                إذا (!nontp) nontp = varp;
                ++nNont;
            }
        }

        ثابت خودکار emit_And_گام = [&](ثابت AstVar*& varpr) {
            emitVarDecl(varpr);
            varpr = VN_CAST(varpr->nextp(), Var);
        };

        // Emit primary ports
        إذا (n_پورت) {
            putsDecoration(اشاره_تهی, "\n// TOP LEVEL PORTS\n");
            // In anonymous union to prevent the C++ compiler from generating
            // constructor/destructor calls
            puts("union {\nstruct {\n");
            لكل (اندازه_t i = 0; i < n_پورت; ++i) emit_And_گام(portp);
            puts("} __VprimaryPorts;\n};\n");
        }

        // Emit trivially constructible variables
        إذا (nTriv) {
            putsDecoration(اشاره_تهی, "\n// TRIVIALLY CONSTRUCTIBLE STATE\n");
            إذا (v3Global.opt.comp_حد_Members() == 0) {
                // Anonymous structs disabled, so just emit them directly
                لكل (اندازه_t i = 0; i < nTriv; ++i) emit_And_گام(trivp);
            } وإلا {
                ثابت اندازه_t lim = v3Global.opt.comp_حد_Members();
                اندازه_t anonL3s = 1;
                اندازه_t anonL2s = 1;
                اندازه_t anonL1s = 1;
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
                        اشاره_تهی,
                        "// Anonymous structures to workaround compiler member-count bugs\n");
                }
                اندازه_t nEmitted = 0;
                لكل (اندازه_t l3 = 0; l3 < anonL3s && nEmitted < nTriv; ++l3) {
                    إذا (anonL3s != 1) puts("struct {\n");
                    لكل (اندازه_t l2 = 0; l2 < anonL2s && nEmitted < nTriv; ++l2) {
                        إذا (anonL2s != 1) puts("struct {\n");
                        لكل (اندازه_t l1 = 0; l1 < anonL1s && nEmitted < nTriv; ++l1) {
                            إذا (anonL1s != 1) puts("struct {\n");
                            لكل (اندازه_t l0 = 0; l0 < lim && nEmitted < nTriv; ++l0, ++nEmitted) {
                                emit_And_گام(trivp);
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
            putsDecoration(اشاره_تهی, "\n// NOT TRIVIALLY CONSTRUCTIBLE STATE\n");
            // Classes use real constructors
            إذا (VN_IS(modp, Class)) {
                لكل (اندازه_t i = 0; i < nNont; ++i) emit_And_گام(nontp);
                أرجع;
            }
            // Output under an anonymous union to prevent the C++ compiler from generating
            // constructor/destructor calls for them. They are managed explicitly in ctor/dtor.
            // While 'union { singleVar; }' for each variable would be simpler, it blows up
            // clang's compile time when using pre-compiled headers, joy.
            puts("union {\nstruct {\n");
            لكل (اندازه_t i = 0; i < nNont; ++i) emit_And_گام(nontp);
            puts("} __Vnontrivs;\n};\n");
        }
    }
    فراغ emit_داخلی_Var_Decls(ثابت Ast_گره_Module* modp) {
        إذا (ثابت AstClass* ثابت classp = VN_CAST(modp, Class)) {
            إذا (classp->needRNG()) {
                putsDecoration(اشاره_تهی, "\n// INTERNAL VARIABLES\n");
                puts("VlRNGReseeds __Vm_rng;\n");
            }
        } وإلا {  // not class
            putsDecoration(اشاره_تهی, "\n// INTERNAL VARIABLES\n");
            puts(EmitCUtil::symClassName() + "* vlSymsp;\n");
            puts("const char* vlNamep;\n");
            // Allocate object-local coverage counters only on modules that
            // contain emitted coverage declarations.
            ثابت صحيح coverBins = Cover_شمار_Visitor{modp}.bins();
            إذا (coverBins) {
                puts(v3Global.opt.نخها() > 1 ? "std::atomic<uint32_t>" : "uint32_t");
                puts(" __Vcoverage[");
                puts(std::مقصد_string(coverBins));
                puts("]{};\n");
            }
        }
    }
    فراغ emitParamDecls(ثابت Ast_گره_Module* modp) {
        منطقي اولی = درست;
        لكل (Ast_گره* nodep = modp->stmtsp(); nodep; nodep = nodep->nextp()) {
            إذا (ثابت AstVar* ثابت varp = VN_CAST(nodep, Var)) {
                إذا (varp->isParam()) {
                    decorate_اولی(اولی, "\n// PARAMETERS\n");
                    putns(varp, "static const ");
                    puts(varp->dtypep()->cType(varp->nameProtect(), خطأ, خطأ));
                    puts(";\n");
                }
            }
        }
    }
    فراغ emitCtorDtorDecls(ثابت Ast_گره_Module* modp) {
        // Classes use CFuncs with isConstructor/isDestructor
        إذا (VN_IS(modp, Class)) أرجع;

        // Proper modules use loose split ctor/dtor and are not copyable/movable
        ثابت std::string name = EmitCUtil::پیشوند_Name_Protect(modp);
        putsDecoration(اشاره_تهی, "\n// CONSTRUCTORS\n");
        putns(modp, name + "(" + EmitCUtil::symClassName() + "* symsp, const char* namep);\n");
        putns(modp, "~" + name + "();\n");
        putns(modp, "VL_UNCOPYABLE(" + name + ");\n");
        putns(modp, "VL_UNMOVABLE(" + name + ");\n");
    }
    فراغ emit_داخلی_روش_Decls(ثابت Ast_گره_Module* modp) {
        منطقي اولی = درست;
        ثابت string section = "\n// INTERNAL METHODS\n";

        إذا (!VN_IS(modp, Class)) {
            decorate_اولی(اولی, section);
            إذا (v3Global.opt.coverage()) {
                puts("void " + protect("__Vconfigure") + "(bool first);\n");
            }
        } وإلا {
            decorate_اولی(اولی, section);
            ثابت std::string name = V3_بیرون_Formatter::quoteNameControls(
                VIdProtect::protect_واژگان_If(modp->prettyName(), v3Global.opt.protectIds()));
            // "override", but don't want clang to start checking them:
            puts("virtual const char* typeName() const { return \""s + name + "\"; }\n");
        }

        إذا (v3Global.opt.coverage() && !VN_IS(modp, Class)) {
            decorate_اولی(اولی, section);
            puts("void __vlCoverInsert(");
            puts(v3Global.opt.نخها() > 1 ? "std::atomic<uint32_t>" : "uint32_t");
            puts("* countp, bool enable, bool localCounter, const char* filenamep, int lineno, "
                 "int column,\n");
            puts("const char* hierp, const char* pagep, const char* commentp, const char* "
                 "linescovp,\n");
            puts("const char* fsmVarp, const char* fsmFromp, const char* fsmTop, const char* "
                 "fsmTagp);\n");
        }

        إذا (v3Global.opt.coverageToggle() && !VN_IS(modp, Class)) {
            decorate_اولی(اولی, section);
            puts("void __vlCoverToggleInsert(int begin, int end, bool ranged, ");
            puts(v3Global.opt.نخها() > 1 ? "std::atomic<uint32_t>" : "uint32_t");
            puts("* countp, bool enable, bool localCounter, const char* filenamep, int lineno, "
                 "int column,\n");
            puts("const char* hierp, const char* pagep, const char* commentp);\n");
        }

        إذا (v3Global.opt.savable()) {
            decorate_اولی(اولی, section);
            puts("void " + protect("__Vserialize") + "(VerilatedSerialize& os);\n");
            puts("void " + protect("__Vdeserialize") + "(VerilatedDeserialize& os);\n");
        }

        // Polymorphic clone for concrete (non-abstract, non-interface) classes
        إذا (ثابت AstClass* ثابت classp = VN_CAST(modp, Class)) {
            إذا (!classp->isInterfaceClass() && !classp->isVirtual()) {
                decorate_اولی(اولی, section);
                استفاده EmbeddedCovergroupVar = std::pair<ثابت AstClass*, ثابت AstVar*>;
                std::وکتور<EmbeddedCovergroupVar> embeddedCovergroupVars;
                ثابت خودکار has_Enclosing_پشت_اشاره_گر = [](ثابت AstClass* covergroupp) {
                    أرجع covergroupp->exists([](ثابت AstVar* ثابت varp) {
                        ثابت AstClassRefDType* ثابت refp
                            = VN_CAST(varp->dtypep()->skipRefp(), ClassRefDType);
                        أرجع refp && refp->raw_اشاره_گر();
                    });
                };
                const_cast<AstClass*>(classp)->foreachMember(
                    [&](AstClass* ثابت memberClassp, AstVar* ثابت varp) {
                        ثابت AstClassRefDType* ثابت refp
                            = VN_CAST(varp->dtypep()->skipRefp(), ClassRefDType);
                        إذا (refp && refp->classp()->isCovergroup()
                            && has_Enclosing_پشت_اشاره_گر(refp->classp())) {
                            embeddedCovergroupVars.emplace_پشت(memberClassp, varp);
                        }
                    });
                ثابت string className = EmitCUtil::پیشوند_Name_Protect(classp);
                إذا (embeddedCovergroupVars.خالی() && !classp->hasRandVarsUpdate()) {
                    putns(classp,
                          "VlClass* clone() const { return new " + className + "(*this); }\n");
                } وإلا {
                    putns(classp, "VlClass* clone() const { " + className + "* const clonep = new "
                                      + className + "(*this); ");
                    إذا (classp->hasRandVarsUpdate()) {
                        ثابت string updateName = "__VnoInFunc___VupdateRandVars";
                        AstCFunc* ثابت updatep
                            = VN_AS(m_memberMap.پیدا_کن_Member(classp, updateName), CFunc);
                        UASSERT_OBJ(updatep, classp, "Missing updateRandVars method");
                        puts("clonep->" + updatep->nameProtect() + "();\n");
                    }
                    لكل (ثابت EmbeddedCovergroupVar& قلم_2 : embeddedCovergroupVars) {
                        puts("clonep->" + EmitCUtil::پیشوند_Name_Protect(قلم_2.اولی)
                             + "::" + قلم_2.دومی->nameProtect() + " = VlNull{}; ");
                    }
                    puts("return clonep; }\n");
                }
            }
        }
    }
    فراغ emitEnums(ثابت Ast_گره_Module* modp) {
        منطقي اولی = درست;
        لكل (ثابت Ast_گره* nodep = modp->stmtsp(); nodep; nodep = nodep->nextp()) {
            ثابت AstTypedef* ثابت tdefp = VN_CAST(nodep, Typedef);
            إذا (!tdefp) استمر;
            إذا (!tdefp->attr_عمومی()) استمر;
            ثابت AstEnumDType* ثابت edtypep
                = VN_CAST(tdefp->dtypep()->skip_Ref_مقصد_Enump(), EnumDType);
            إذا (!edtypep) استمر;
            decorate_اولی(اولی, "\n// ENUMS (that were declared public)\n");
            إذا (edtypep->عرض() > 64) {
                putsDecoration(tdefp,
                               "// enum " + tdefp->nameProtect() + " ignored: Too wide for C++\n");
            } وإلا {
                putns(tdefp, "enum " + tdefp->name() + " {\n");
                لكل (ثابت Ast_Enum_قلم* itemp = edtypep->itemsp(); itemp;
                     itemp = VN_AS(itemp->nextp(), Enum_قلم)) {
                    إذا (ثابت AstConst* ثابت constp = VN_CAST(itemp->valuep(), Const)) {
                        إذا (constp->رقم().isFourState()) {
                            putns(itemp, "// " + itemp->nameProtect() + " is four-state\n");
                            استمر;
                        }
                    }
                    putns(itemp, itemp->nameProtect());
                    puts(" = ");
                    putns(itemp, VN_AS(itemp->valuep(), Const)->رقم().emitC());
                    إذا (VN_IS(itemp->nextp(), Enum_قلم)) puts(",");
                    puts("\n");
                }
                puts("};\n");
            }
        }
    }
    فراغ emitStructDecl(ثابت Ast_گره_Module* modp, Ast_گره_UOr_Struct_DType* sdtypep,
                        std::مجموعه<Ast_گره_UOr_Struct_DType*>& emitted) {
        إذا (emitted.شمار(sdtypep) > 0) أرجع;
        emitted.درج(sdtypep);
        لكل (AstMemberDType* itemp = sdtypep->membersp(); itemp;
             itemp = VN_AS(itemp->nextp(), MemberDType)) {
            Ast_گره_UOr_Struct_DType* ثابت subp = itemp->get_فرزند_Structp();
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
    تعداد صنف AttributeType : uint8_t { عرض_2, DIMENSION };
    // Get member attribute based on type
    صحيح get_گره_Attribute(ثابت AstMemberDType* itemp, AttributeType type) {
        ثابت منطقي isArrayType = itemp->dtypep()->isNonPackedArray();
        بدل (type) {
        حالة AttributeType::عرض_2: {
            إذا (isArrayType) {
                // For arrays, get innermost element width
                ثابت Ast_گره_DType* dtype = itemp->dtypep();
                طالما (dtype->subDTypep()) dtype = dtype->subDTypep();
                أرجع dtype->عرض();
            }
            أرجع itemp->عرض();
        }
        حالة AttributeType::DIMENSION: {
            // Return array dimension or 0 for non-arrays
            أرجع isArrayType ? itemp->dtypep()->dimensions(درست).دومی : 0;
        }
        افتراضي: {
            أرجع 0;
        }
        }
    }
    قالب <AttributeType T>
    فراغ emit_Member_وکتور(ثابت Ast_گره_UOr_Struct_DType* sdtypep) {
        puts("return {");
        منطقي needComma = خطأ;
        لكل (ثابت AstMemberDType* itemp = sdtypep->membersp(); itemp;
             itemp = VN_AS(itemp->nextp(), MemberDType)) {
            إذا (!itemp->isConstrainedRand()) استمر;
            // Comma handling: add before element except first
            إذا (needComma) puts(",\n");
            putns(itemp, std::مقصد_string(get_گره_Attribute(itemp, T)));
            needComma = درست;
        }
        puts("};\n}\n");
    }
    فراغ emitUnpackedUOrSBody(Ast_گره_UOr_Struct_DType* sdtypep) {
        putns(sdtypep, sdtypep->verilogKwd());  // "struct"/"union"
        puts(" " + EmitCUtil::پیشوند_Name_Protect(sdtypep) + " {\n");
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
                needComma = درست;
            }
            puts("};\n}\n");

            putns(sdtypep, "\nstd::vector<int> memberWidth(void) const {\n");
            emit_Member_وکتور<AttributeType::عرض_2>(sdtypep);

            putns(sdtypep, "\nstd::vector<int> memberDimension(void) const {\n");
            emit_Member_وکتور<AttributeType::DIMENSION>(sdtypep);

            needComma = خطأ;
            putns(sdtypep, "\nauto memberIndices(void) const {\n");
            puts("return std::index_sequence_for<");
            لكل (ثابت AstMemberDType* itemp = sdtypep->membersp(); itemp;
                 itemp = VN_AS(itemp->nextp(), MemberDType)) {
                إذا (!itemp->isConstrainedRand()) استمر;
                إذا (needComma) puts(",\n");
                putns(itemp, itemp->dtypep()->cType("", خطأ, خطأ));
                needComma = درست;
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
                needComma = درست;
            }
            puts(");\n}\n");
        }
        putns(sdtypep, "\nbool operator==(const " + EmitCUtil::پیشوند_Name_Protect(sdtypep)
                           + "& rhs) const {\n");
        puts("return ");
        لكل (ثابت AstMemberDType* itemp = sdtypep->membersp(); itemp;
             itemp = VN_AS(itemp->nextp(), MemberDType)) {
            إذا (itemp != sdtypep->membersp()) puts("\n    && ");
            putns(itemp, itemp->nameProtect() + " == " + "rhs." + itemp->nameProtect());
        }
        puts(";\n");
        puts("}\n");
        putns(sdtypep, "bool operator!=(const " + EmitCUtil::پیشوند_Name_Protect(sdtypep)
                           + "& rhs) const {\n");
        puts("return !(*this == rhs);\n}\n");
        putns(sdtypep, "\nbool operator<(const " + EmitCUtil::پیشوند_Name_Protect(sdtypep)
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
        putns(sdtypep, "struct VlIsCustomStruct<" + EmitCUtil::پیشوند_Name_Protect(sdtypep)
                           + "> : public std::true_type {};\n");
    }

    // getfunc: VL_ASSIGNSEL_XX(rbits, obits, off, lhsdata, rhsdata);
    // !getfunc: VL_SELASSIGN_XX(rbits, obits, lhsdata, rhsdata, off);
    فراغ emit_Vl_تخصیص_بده(ثابت Ast_گره_DType* ثابت lhstype, ثابت Ast_گره_DType* rhstype,
                      ثابت std::string& off, ثابت std::string& lhsdata,
                      ثابت std::string& rhsdata, منطقي getfunc) {
        puts(getfunc ? "VL_ASSIGNSEL_" : "VL_SELASSIGN_");
        puts(lhstype->charIQWN());
        puts(rhstype->charIQWN());
        puts("(" + std::مقصد_string(lhstype->عرض()) + ", ");  // LHS width
        إذا (getfunc) {
            puts(std::مقصد_string(rhstype->عرض()) + ", ");  // Number of copy bits
            puts(off + ", ");  // LHS offset
        } وإلا {
            // Number of copy bits. Use widthTototalBytes to
            // make VL_SELASSIGN_XX clear upper unused bits for us.
            // puts(std::to_string(lhstype->width()) + ", ");
            puts(std::مقصد_string(lhstype->عرض_مجموع_بایتها() * 8) + ", ");
        }
        puts(lhsdata + ", ");  // LHS data
        puts(rhsdata);  // RHS data
        إذا (!getfunc) {
            puts(", " + off);  // RHS offset
        }
        puts(");\n");
    }

    // `retOrArg` should be prefixed by `&` or suffixed by `.data()` depending on its type
    فراغ emitPackedMember(ثابت Ast_گره_DType* والد_Dtypep, ثابت Ast_گره_DType* dtypep,
                          ثابت std::string& fieldname, ثابت std::string& جابه_جایی, منطقي getfunc,
                          ثابت std::string& retOrArg) {
        dtypep = dtypep->skipRefp();
        إذا (ثابت خودکار* adtypep = VN_CAST(dtypep, PackArrayDType)) {
            ثابت std::string شاخص = m_نامها.get("__Vi");
            puts("for (int " + شاخص + " = 0; " + شاخص + " < "
                 + std::مقصد_string(adtypep->elementsConst()) + "; ++" + شاخص + ") {\n");

            ثابت std::string جابه_جایی_درون_Loop
                = جابه_جایی + " + " + شاخص + " * " + std::مقصد_string(adtypep->subDTypep()->عرض());
            ثابت std::string newName = fieldname + "[" + شاخص + "]";
            emitPackedMember(والد_Dtypep, adtypep->subDTypep(), newName, جابه_جایی_درون_Loop, getfunc,
                             retOrArg);
            puts("}\n");
        } وإلا إذا (VN_IS(dtypep, گره_UOr_Struct_DType)) {
            ثابت std::string tmp = m_نامها.get("__Vtmp");
            إذا (getfunc) {  // Emit `get` func;
                // auto __tmp = field.get();
                puts("auto " + tmp + " = " + fieldname + ".get();\n");
                // VL_ASSIGNSEL_XX(rbits, obits, lsb, lhsdata, rhsdata);
                emit_Vl_تخصیص_بده(والد_Dtypep, dtypep, جابه_جایی, retOrArg, tmp, getfunc);
            } وإلا {  // Emit `set` func
                ثابت std::string tmptype = AstCDType::type_مقصد_Hold(dtypep->عرض());
                // type tmp;
                puts(tmptype + " " + tmp + ";\n");
                // VL_SELASSIGN_XX(rbits, obits, lhsdata, rhsdata, roffset);
                emit_Vl_تخصیص_بده(dtypep, والد_Dtypep, جابه_جایی, tmp, retOrArg, getfunc);
                // field.set(__tmp);
                puts(fieldname + ".set(" + tmp + ");\n");
            }
        } وإلا {
            UASSERT_OBJ(VN_IS(dtypep, EnumDType) || VN_IS(dtypep, BasicDType), dtypep,
                        "Unsupported type in packed struct or union");
            إذا (getfunc) {  // Emit `get` func;
                // VL_ASSIGNSEL_XX(rbits, obits, lsb, lhsdata, rhsdata);
                emit_Vl_تخصیص_بده(والد_Dtypep, dtypep, جابه_جایی, retOrArg, fieldname, getfunc);
            } وإلا {  // Emit `set` func
                // VL_SELASSIGN_XX(rbits, obits, lhsdata, rhsdata, roffset);
                emit_Vl_تخصیص_بده(dtypep, والد_Dtypep, جابه_جایی, fieldname, retOrArg, getfunc);
            }
        }
    }
    فراغ emitPackedUOrSBody(Ast_گره_UOr_Struct_DType* sdtypep) {
        putns(sdtypep, sdtypep->verilogKwd());  // "struct"/"union"
        puts(" " + EmitCUtil::پیشوند_Name_Protect(sdtypep) + " {\n");

        AstMemberDType* itemp;
        AstMemberDType* آخرین_Itemp;
        ثابت AstMemberDType* witemp = اشاره_تهی;
        // LSB is first field in C, so loop backwards
        لكل (آخرین_Itemp = sdtypep->membersp(); آخرین_Itemp && آخرین_Itemp->nextp();
             آخرین_Itemp = VN_AS(آخرین_Itemp->nextp(), MemberDType)) {
            إذا (آخرین_Itemp->عرض() == sdtypep->عرض()) witemp = آخرین_Itemp;
        }
        لكل (itemp = آخرین_Itemp; itemp; itemp = VN_CAST(itemp->backp(), MemberDType)) {
            putns(itemp, itemp->dtypep()->cType(itemp->nameProtect(), خطأ, خطأ, درست));
            puts(";\n");
        }

        ثابت std::string retArgName = m_نامها.get("__v");
        ثابت std::string retArgType = AstCDType::type_مقصد_Hold(sdtypep->عرض());

        // Emit `get` member function
        puts(retArgType + " get() const {\n");
        puts(retArgType + " " + retArgName + ";\n");
        إذا (VN_IS(sdtypep, StructDType)) {
            لكل (itemp = آخرین_Itemp; itemp; itemp = VN_CAST(itemp->backp(), MemberDType)) {
                emitPackedMember(sdtypep, itemp->dtypep(), itemp->nameProtect(),
                                 std::مقصد_string(itemp->lsb()), /*getfunc=*/درست, retArgName);
            }
        } وإلا {
            // We only need to fill the widest field of union
            emitPackedMember(sdtypep, witemp->dtypep(), witemp->nameProtect(),
                             std::مقصد_string(witemp->lsb()), /*getfunc=*/درست, retArgName);
        }
        puts("return " + retArgName + ";\n");
        puts("}\n");

        // Emit `set` member function
        puts("void set(const " + retArgType + "& " + retArgName + ") {\n");
        إذا (VN_IS(sdtypep, StructDType)) {
            لكل (itemp = آخرین_Itemp; itemp; itemp = VN_CAST(itemp->backp(), MemberDType)) {
                emitPackedMember(sdtypep, itemp->dtypep(), itemp->nameProtect(),
                                 std::مقصد_string(itemp->lsb()), /*getfunc=*/خطأ, retArgName);
            }
        } وإلا {
            // We only need to fill the widest field of union
            emitPackedMember(sdtypep, witemp->dtypep(), witemp->nameProtect(),
                             std::مقصد_string(witemp->lsb()), /*getfunc=*/خطأ, retArgName);
        }

        puts("}\n");

        puts("};\n");
        m_نامها.reset();
    }
    فراغ emitStructs(ثابت Ast_گره_Module* modp) {
        // Track structs that've been emitted already
        std::مجموعه<Ast_گره_UOr_Struct_DType*> emitted;
        لكل (ثابت Ast_گره* nodep = modp->stmtsp(); nodep; nodep = nodep->nextp()) {
            ثابت AstTypedef* ثابت tdefp = VN_CAST(nodep, Typedef);
            إذا (!tdefp) استمر;
            Ast_گره_UOr_Struct_DType* ثابت sdtypep
                = VN_CAST(tdefp->dtypep()->skip_Ref_مقصد_Enump(), گره_UOr_Struct_DType);
            إذا (!sdtypep) استمر;
            // V3Inline can copy one module into several parents. Only the module that
            // declares the struct emits it, so C++ sees only one definition.
            ثابت منطقي declared_درون_Other_Module = sdtypep->classOrPackagep() != modp;
            إذا (declared_درون_Other_Module) استمر;
            emitStructDecl(modp, sdtypep, emitted);
        }
    }
    فراغ emitFuncDecls(ثابت Ast_گره_Module* modp, منطقي درون_Class_Body) {
        std::وکتور<ثابت AstCFunc*> funcsp;

        لكل (ثابت Ast_گره* nodep = modp->stmtsp(); nodep; nodep = nodep->nextp()) {
            إذا (ثابت AstCFunc* ثابت funcp = VN_CAST(nodep, CFunc)) {
                إذا (funcp->dpiImportPrototype())  // Declared in __Dpi.h
                    استمر;
                إذا (funcp->dpiExportDispatcher())  // Declared in __Dpi.h
                    استمر;
                إذا (funcp->is_روش() != درون_Class_Body)  // Only methods go inside class
                    استمر;
                إذا (funcp->is_روش() && funcp->isLoose())  // Loose methods are declared lazily
                    استمر;
                funcsp.افزودن_پشت(funcp);
            }
        }

        std::stable_مرتب_سازی(funcsp.begin(), funcsp.انتها(), [](ثابت Ast_گره* ap, ثابت Ast_گره* bp) {
            أرجع ap->name() < bp->name();
        });

        لكل (ثابت AstCFunc* ثابت funcp : funcsp) {
            إذا (درون_Class_Body) ofp()->puts_خصوصی(funcp->decl_خصوصی());
            emitCFuncDecl(funcp, modp);
        }
    }
    فراغ emitAll(ثابت Ast_گره_Module* modp) {
        // Include files required by this AstNodeModule
        إذا (ثابت AstClass* ثابت classp = VN_CAST(modp, Class)) {
            لكل (ثابت AstClassExtends* extp = classp->extendsp(); extp;
                 extp = VN_AS(extp->nextp(), ClassExtends)) {
                putns(extp, "#include \""
                                + EmitCUtil::پیشوند_Name_Protect(extp->classp()->classOrPackagep())
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
        puts(EmitCUtil::پیشوند_Name_Protect(modp));
        إذا (ثابت AstClass* ثابت classp = VN_CAST(modp, Class)) {
            puts(" : ");
            إذا (classp->extendsp()) {
                منطقي needComma = خطأ;
                لكل (ثابت AstClassExtends* extp = classp->extendsp(); extp;
                     extp = VN_AS(extp->nextp(), ClassExtends)) {
                    إذا (needComma) puts(", ");
                    // Use virtual only for interfaces for class inheritance
                    // (extends)
                    puts(extp->classp()->use_Virtual_عمومی() ? "virtual public " : "public ");
                    putns(extp, EmitCUtil::پیشوند_Name_Protect(extp->classp()));
                    needComma = درست;
                }
            } وإلا {
                puts("public virtual VlClass");
            }
        } وإلا {
            puts(" final");
        }
        puts(" {\n");
        ofp()->reset_خصوصی();
        ofp()->puts_خصوصی(خطأ);  // public:

        // Emit all class body contents
        emitCellDecls(modp);
        emitEnums(modp);
        emitDesignVarDecls(modp);
        emit_داخلی_Var_Decls(modp);
        emitParamDecls(modp);
        emitCtorDtorDecls(modp);
        emit_داخلی_روش_Decls(modp);
        emitFuncDecls(modp, /* inClassBody: */ درست);

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
    فراغ بازدید(Ast_گره* nodep) override { nodep->v3fatalSrc("Unused"); }  // LCOV_EXCL_LINE

    صریح EmitCHeader(ثابت Ast_گره_Module* modp) {
        UINFO(5, "  Emitting header for " << EmitCUtil::پیشوند_Name_Protect(modp));

        // Open output file
        open_New_Output_سرآیند_File(EmitCUtil::پیشوند_Name_Protect(modp), "Design internal header");
        puts("// See " + EmitCUtil::بالاترین_Class_Name() + ".h for the primary calling header\n");

        ofp()->putsGuard();

        // Include files
        puts("\n");
        ofp()->puts_Int_بالاترین_Include();
        puts("#include \"verilated.h\"\n");
        إذا (modp->is_بالاترین() && optSystemC()) puts("#include \"verilated_sc.h\"\n");
        إذا (v3Global.opt.mtasks()) puts("#include \"verilated_threads.h\"\n");
        إذا (v3Global.opt.savable()) puts("#include \"verilated_save.h\"\n");
        إذا (v3Global.opt.coverage()) puts("#include \"verilated_cov.h\"\n");
        إذا (v3Global.opt.coverage() || v3Global.useCovergroup())
            puts("#include \"verilated_covergroup.h\"\n");
        إذا (v3Global.استفاده_میکند_Timing()) puts("#include \"verilated_timing.h\"\n");
        إذا (v3Global.useRandomizeMethods()) puts("#include \"verilated_random.h\"\n");
        إذا (v3Global.استفاده_میکند_Force()) puts("#include \"verilated_force.h\"\n");

        std::مجموعه<string> cuse_مجموعه;
        خودکار جمع_کن_مقصد_cuse_مجموعه = [&](string s) { cuse_مجموعه.درج(s); };

        forModCUse(modp, VUseType::INT_FWD_CLASS | VUseType::INT_INCLUDE, جمع_کن_مقصد_cuse_مجموعه);
        إذا (ثابت AstClassPackage* ثابت packagep = VN_CAST(modp, ClassPackage)) {
            forModCUse(packagep->classp(), VUseType::INT_INCLUDE | VUseType::INT_FWD_CLASS,
                       جمع_کن_مقصد_cuse_مجموعه);
        }

        لكل (ثابت string& s : cuse_مجموعه) puts(s);
        puts("\n");

        emitAll(modp);

        إذا (ثابت AstClassPackage* ثابت packagep = VN_CAST(modp, ClassPackage)) {
            // Put the non-static class implementation in same h file for speed
            emitAll(packagep->classp());
        }

        ofp()->puts_انتها_Guard();

        // Close output file
        closeOutputFile();
    }
    ~EmitCHeader() override = افتراضي;

عام:
    ساكن فراغ اصلی(ثابت Ast_گره_Module* modp) { ثابت EmitCHeader emitCHeader{modp}; }
};

//######################################################################
// EmitC class functions

فراغ V3EmitC::emitc_سرآیندها() {
    UINFO(2, __FUNCTION__ << ":");

    // Process each module in turn
    لكل (ثابت Ast_گره* nodep = v3Global.rootp()->modulesp(); nodep; nodep = nodep->nextp()) {
        إذا (VN_IS(nodep, Class)) استمر;  // Declared with the ClassPackage
        ثابت Ast_گره_Module* ثابت modp = VN_AS(nodep, گره_Module);
        إذا (modp->isConstPool()) استمر;  // Emitted by V3EmitCConstPool
        EmitCHeader::اصلی(modp);
    }
}
