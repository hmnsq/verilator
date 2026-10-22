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
    V3_Unique_ناں m_ناں;
    VMemberMap m_memberMap;
    // METHODS

    صنف Cover_گنتی_Visitor final : عام VNVisitorConst {
        صحيح m_bins = 0;  // Running total of coverage bins counted so far

        فراغ ویکھو(Ast_گرہ_Cover_Decl* nodep) override {
            // Each module class owns the counters for declarations it emits;
            // duplicate no-inline instances need separate arrays for
            // forcePerInstance reporting.
            إذا (!nodep->انکڑے_Decl_Nullp()) m_bins += nodep->ناپ();
        }
        فراغ ویکھو(Ast_گرہ* nodep) override { iterateChildrenConst(nodep); }

    عام:
        واضح Cover_گنتی_Visitor(ثابت Ast_گرہ_Module* modp) {
            iterateConst(const_cast<Ast_گرہ_Module*>(modp));
        }
        صحيح bins() ثابت { أرجع m_bins; }
    };

    فراغ decorate_پہلا(منطقي& پہلا, ثابت string& str) {
        إذا (پہلا) {
            putsDecoration(خالی_سوچک, str);
            پہلا = خطأ;
        }
    }
    فراغ emitCellDecls(ثابت Ast_گرہ_Module* modp) {
        منطقي پہلا = سچ;
        لكل (ثابت Ast_گرہ* nodep = modp->stmtsp(); nodep; nodep = nodep->nextp()) {
            إذا (ثابت AstCell* ثابت cellp = VN_CAST(nodep, Cell)) {
                إذا (cellp->modp()->isConstPool()) استمر;  // Special emit rules
                decorate_پہلا(پہلا, "// CELLS\n");
                putns(cellp, EmitCUtil::اگیتر_Name_Protect(cellp->modp()) + "* "
                                 + cellp->nameProtect() + ";\n");
            }
        }
    }
    فراغ emitDesignVarDecls(ثابت Ast_گرہ_Module* modp) {
        // V3VariableOrder puts all variables at the front of the list: top level ports, then
        // trivially constructible variables, then the not trivially constructible ones, then
        // those that are not model state
        ثابت AstVar* portp = خالی_سوچک;
        ثابت AstVar* trivp = خالی_سوچک;
        ثابت AstVar* nontp = خالی_سوچک;
        ناپ_t n_پورٹ = 0;
        ناپ_t nTriv = 0;
        ناپ_t nNont = 0;
        لكل (ثابت Ast_گرہ* nodep = modp->stmtsp(); nodep; nodep = nodep->nextp()) {
            ثابت AstVar* ثابت varp = VN_CAST(nodep, Var);
            إذا (!varp) اكسر;
            إذا (!varp->isModelState()) {
                // Cache line alignment is only effective on emitted fields
                UASSERT_OBJ(!varp->mtask_Cache_سطر_Align(), varp, "Aligned non-field");
            } وإلا إذا (varp->isPrimaryIO()) {
                إذا (!portp) portp = varp;
                ++n_پورٹ;
            } وإلا إذا (varp->isTriviallyConstructible()) {
                إذا (!trivp) trivp = varp;
                ++nTriv;
            } وإلا {
                إذا (!nontp) nontp = varp;
                ++nNont;
            }
        }

        ثابت خودکار emit_And_قدم = [&](ثابت AstVar*& varpr) {
            emitVarDecl(varpr);
            varpr = VN_CAST(varpr->nextp(), Var);
        };

        // Emit primary ports
        إذا (n_پورٹ) {
            putsDecoration(خالی_سوچک, "\n// TOP LEVEL PORTS\n");
            // In anonymous union to prevent the C++ compiler from generating
            // constructor/destructor calls
            puts("union {\nstruct {\n");
            لكل (ناپ_t i = 0; i < n_پورٹ; ++i) emit_And_قدم(portp);
            puts("} __VprimaryPorts;\n};\n");
        }

        // Emit trivially constructible variables
        إذا (nTriv) {
            putsDecoration(خالی_سوچک, "\n// TRIVIALLY CONSTRUCTIBLE STATE\n");
            إذا (v3Global.opt.comp_حد_بندی_Members() == 0) {
                // Anonymous structs disabled, so just emit them directly
                لكل (ناپ_t i = 0; i < nTriv; ++i) emit_And_قدم(trivp);
            } وإلا {
                ثابت ناپ_t lim = v3Global.opt.comp_حد_بندی_Members();
                ناپ_t anonL3s = 1;
                ناپ_t anonL2s = 1;
                ناپ_t anonL1s = 1;
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
                        خالی_سوچک,
                        "// Anonymous structures to workaround compiler member-count bugs\n");
                }
                ناپ_t nEmitted = 0;
                لكل (ناپ_t l3 = 0; l3 < anonL3s && nEmitted < nTriv; ++l3) {
                    إذا (anonL3s != 1) puts("struct {\n");
                    لكل (ناپ_t l2 = 0; l2 < anonL2s && nEmitted < nTriv; ++l2) {
                        إذا (anonL2s != 1) puts("struct {\n");
                        لكل (ناپ_t l1 = 0; l1 < anonL1s && nEmitted < nTriv; ++l1) {
                            إذا (anonL1s != 1) puts("struct {\n");
                            لكل (ناپ_t l0 = 0; l0 < lim && nEmitted < nTriv; ++l0, ++nEmitted) {
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
            putsDecoration(خالی_سوچک, "\n// NOT TRIVIALLY CONSTRUCTIBLE STATE\n");
            // Classes use real constructors
            إذا (VN_IS(modp, Class)) {
                لكل (ناپ_t i = 0; i < nNont; ++i) emit_And_قدم(nontp);
                أرجع;
            }
            // Output under an anonymous union to prevent the C++ compiler from generating
            // constructor/destructor calls for them. They are managed explicitly in ctor/dtor.
            // While 'union { singleVar; }' for each variable would be simpler, it blows up
            // clang's compile time when using pre-compiled headers, joy.
            puts("union {\nstruct {\n");
            لكل (ناپ_t i = 0; i < nNont; ++i) emit_And_قدم(nontp);
            puts("} __Vnontrivs;\n};\n");
        }
    }
    فراغ emit_اندرونی_Var_Decls(ثابت Ast_گرہ_Module* modp) {
        إذا (ثابت AstClass* ثابت classp = VN_CAST(modp, Class)) {
            إذا (classp->needRNG()) {
                putsDecoration(خالی_سوچک, "\n// INTERNAL VARIABLES\n");
                puts("VlRNGReseeds __Vm_rng;\n");
            }
        } وإلا {  // not class
            putsDecoration(خالی_سوچک, "\n// INTERNAL VARIABLES\n");
            puts(EmitCUtil::symClassName() + "* vlSymsp;\n");
            puts("const char* vlNamep;\n");
            // Allocate object-local coverage counters only on modules that
            // contain emitted coverage declarations.
            ثابت صحيح coverBins = Cover_گنتی_Visitor{modp}.bins();
            إذا (coverBins) {
                puts(v3Global.opt.دھاگے() > 1 ? "std::atomic<uint32_t>" : "uint32_t");
                puts(" __Vcoverage[");
                puts(std::منزل_string(coverBins));
                puts("]{};\n");
            }
        }
    }
    فراغ emitParamDecls(ثابت Ast_گرہ_Module* modp) {
        منطقي پہلا = سچ;
        لكل (Ast_گرہ* nodep = modp->stmtsp(); nodep; nodep = nodep->nextp()) {
            إذا (ثابت AstVar* ثابت varp = VN_CAST(nodep, Var)) {
                إذا (varp->isParam()) {
                    decorate_پہلا(پہلا, "\n// PARAMETERS\n");
                    putns(varp, "static const ");
                    puts(varp->dtypep()->cType(varp->nameProtect(), خطأ, خطأ));
                    puts(";\n");
                }
            }
        }
    }
    فراغ emitCtorDtorDecls(ثابت Ast_گرہ_Module* modp) {
        // Classes use CFuncs with isConstructor/isDestructor
        إذا (VN_IS(modp, Class)) أرجع;

        // Proper modules use loose split ctor/dtor and are not copyable/movable
        ثابت std::string name = EmitCUtil::اگیتر_Name_Protect(modp);
        putsDecoration(خالی_سوچک, "\n// CONSTRUCTORS\n");
        putns(modp, name + "(" + EmitCUtil::symClassName() + "* symsp, const char* namep);\n");
        putns(modp, "~" + name + "();\n");
        putns(modp, "VL_UNCOPYABLE(" + name + ");\n");
        putns(modp, "VL_UNMOVABLE(" + name + ");\n");
    }
    فراغ emit_اندرونی_طریقہ_Decls(ثابت Ast_گرہ_Module* modp) {
        منطقي پہلا = سچ;
        ثابت string section = "\n// INTERNAL METHODS\n";

        إذا (!VN_IS(modp, Class)) {
            decorate_پہلا(پہلا, section);
            إذا (v3Global.opt.coverage()) {
                puts("void " + protect("__Vconfigure") + "(bool first);\n");
            }
        } وإلا {
            decorate_پہلا(پہلا, section);
            ثابت std::string name = V3_باہرلا_Formatter::quoteNameControls(
                VIdProtect::protect_الفاظ_If(modp->prettyName(), v3Global.opt.protectIds()));
            // "override", but don't want clang to start checking them:
            puts("virtual const char* typeName() const { return \""s + name + "\"; }\n");
        }

        إذا (v3Global.opt.coverage() && !VN_IS(modp, Class)) {
            decorate_پہلا(پہلا, section);
            puts("void __vlCoverInsert(");
            puts(v3Global.opt.دھاگے() > 1 ? "std::atomic<uint32_t>" : "uint32_t");
            puts("* countp, bool enable, bool localCounter, const char* filenamep, int lineno, "
                 "int column,\n");
            puts("const char* hierp, const char* pagep, const char* commentp, const char* "
                 "linescovp,\n");
            puts("const char* fsmVarp, const char* fsmFromp, const char* fsmTop, const char* "
                 "fsmTagp);\n");
        }

        إذا (v3Global.opt.coverageToggle() && !VN_IS(modp, Class)) {
            decorate_پہلا(پہلا, section);
            puts("void __vlCoverToggleInsert(int begin, int end, bool ranged, ");
            puts(v3Global.opt.دھاگے() > 1 ? "std::atomic<uint32_t>" : "uint32_t");
            puts("* countp, bool enable, bool localCounter, const char* filenamep, int lineno, "
                 "int column,\n");
            puts("const char* hierp, const char* pagep, const char* commentp);\n");
        }

        إذا (v3Global.opt.savable()) {
            decorate_پہلا(پہلا, section);
            puts("void " + protect("__Vserialize") + "(VerilatedSerialize& os);\n");
            puts("void " + protect("__Vdeserialize") + "(VerilatedDeserialize& os);\n");
        }

        // Polymorphic clone for concrete (non-abstract, non-interface) classes
        إذا (ثابت AstClass* ثابت classp = VN_CAST(modp, Class)) {
            إذا (!classp->isInterfaceClass() && !classp->isVirtual()) {
                decorate_پہلا(پہلا, section);
                ورتوں EmbeddedCovergroupVar = std::pair<ثابت AstClass*, ثابت AstVar*>;
                std::سمتیہ<EmbeddedCovergroupVar> embeddedCovergroupVars;
                ثابت خودکار has_Enclosing_پٹھ_اشارہ_گر = [](ثابت AstClass* covergroupp) {
                    أرجع covergroupp->exists([](ثابت AstVar* ثابت varp) {
                        ثابت AstClassRefDType* ثابت refp
                            = VN_CAST(varp->dtypep()->skipRefp(), ClassRefDType);
                        أرجع refp && refp->raw_اشارہ_گر();
                    });
                };
                const_cast<AstClass*>(classp)->foreachMember(
                    [&](AstClass* ثابت memberClassp, AstVar* ثابت varp) {
                        ثابت AstClassRefDType* ثابت refp
                            = VN_CAST(varp->dtypep()->skipRefp(), ClassRefDType);
                        إذا (refp && refp->classp()->isCovergroup()
                            && has_Enclosing_پٹھ_اشارہ_گر(refp->classp())) {
                            embeddedCovergroupVars.emplace_پٹھ(memberClassp, varp);
                        }
                    });
                ثابت string className = EmitCUtil::اگیتر_Name_Protect(classp);
                إذا (embeddedCovergroupVars.خالی_تھاواں() && !classp->hasRandVarsUpdate()) {
                    putns(classp,
                          "VlClass* clone() const { return new " + className + "(*this); }\n");
                } وإلا {
                    putns(classp, "VlClass* clone() const { " + className + "* const clonep = new "
                                      + className + "(*this); ");
                    إذا (classp->hasRandVarsUpdate()) {
                        ثابت string updateName = "__VnoInFunc___VupdateRandVars";
                        AstCFunc* ثابت updatep
                            = VN_AS(m_memberMap.لبھو_Member(classp, updateName), CFunc);
                        UASSERT_OBJ(updatep, classp, "Missing updateRandVars method");
                        puts("clonep->" + updatep->nameProtect() + "();\n");
                    }
                    لكل (ثابت EmbeddedCovergroupVar& شے_2 : embeddedCovergroupVars) {
                        puts("clonep->" + EmitCUtil::اگیتر_Name_Protect(شے_2.پہلا)
                             + "::" + شے_2.دوجا->nameProtect() + " = VlNull{}; ");
                    }
                    puts("return clonep; }\n");
                }
            }
        }
    }
    فراغ emitEnums(ثابت Ast_گرہ_Module* modp) {
        منطقي پہلا = سچ;
        لكل (ثابت Ast_گرہ* nodep = modp->stmtsp(); nodep; nodep = nodep->nextp()) {
            ثابت AstTypedef* ثابت tdefp = VN_CAST(nodep, Typedef);
            إذا (!tdefp) استمر;
            إذا (!tdefp->attr_عوامی()) استمر;
            ثابت AstEnumDType* ثابت edtypep
                = VN_CAST(tdefp->dtypep()->skip_Ref_منزل_Enump(), EnumDType);
            إذا (!edtypep) استمر;
            decorate_پہلا(پہلا, "\n// ENUMS (that were declared public)\n");
            إذا (edtypep->چوڑائی() > 64) {
                putsDecoration(tdefp,
                               "// enum " + tdefp->nameProtect() + " ignored: Too wide for C++\n");
            } وإلا {
                putns(tdefp, "enum " + tdefp->name() + " {\n");
                لكل (ثابت Ast_Enum_شے* itemp = edtypep->itemsp(); itemp;
                     itemp = VN_AS(itemp->nextp(), Enum_شے)) {
                    إذا (ثابت AstConst* ثابت constp = VN_CAST(itemp->valuep(), Const)) {
                        إذا (constp->عدد().isFourState()) {
                            putns(itemp, "// " + itemp->nameProtect() + " is four-state\n");
                            استمر;
                        }
                    }
                    putns(itemp, itemp->nameProtect());
                    puts(" = ");
                    putns(itemp, VN_AS(itemp->valuep(), Const)->عدد().emitC());
                    إذا (VN_IS(itemp->nextp(), Enum_شے)) puts(",");
                    puts("\n");
                }
                puts("};\n");
            }
        }
    }
    فراغ emitStructDecl(ثابت Ast_گرہ_Module* modp, Ast_گرہ_UOr_Struct_DType* sdtypep,
                        std::سیٹ<Ast_گرہ_UOr_Struct_DType*>& emitted) {
        إذا (emitted.گنتی(sdtypep) > 0) أرجع;
        emitted.داخل_کرو(sdtypep);
        لكل (AstMemberDType* itemp = sdtypep->membersp(); itemp;
             itemp = VN_AS(itemp->nextp(), MemberDType)) {
            Ast_گرہ_UOr_Struct_DType* ثابت subp = itemp->get_بچہ_Structp();
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
    تعداد صنف AttributeType : uint8_t { چوڑائی_2, DIMENSION };
    // Get member attribute based on type
    صحيح get_گرہ_Attribute(ثابت AstMemberDType* itemp, AttributeType type) {
        ثابت منطقي isArrayType = itemp->dtypep()->isNonPackedArray();
        بدل (type) {
        حالة AttributeType::چوڑائی_2: {
            إذا (isArrayType) {
                // For arrays, get innermost element width
                ثابت Ast_گرہ_DType* dtype = itemp->dtypep();
                طالما (dtype->subDTypep()) dtype = dtype->subDTypep();
                أرجع dtype->چوڑائی();
            }
            أرجع itemp->چوڑائی();
        }
        حالة AttributeType::DIMENSION: {
            // Return array dimension or 0 for non-arrays
            أرجع isArrayType ? itemp->dtypep()->dimensions(سچ).دوجا : 0;
        }
        افتراضي: {
            أرجع 0;
        }
        }
    }
    سانچا <AttributeType T>
    فراغ emit_Member_سمتیہ(ثابت Ast_گرہ_UOr_Struct_DType* sdtypep) {
        puts("return {");
        منطقي needComma = خطأ;
        لكل (ثابت AstMemberDType* itemp = sdtypep->membersp(); itemp;
             itemp = VN_AS(itemp->nextp(), MemberDType)) {
            إذا (!itemp->isConstrainedRand()) استمر;
            // Comma handling: add before element except first
            إذا (needComma) puts(",\n");
            putns(itemp, std::منزل_string(get_گرہ_Attribute(itemp, T)));
            needComma = سچ;
        }
        puts("};\n}\n");
    }
    فراغ emitUnpackedUOrSBody(Ast_گرہ_UOr_Struct_DType* sdtypep) {
        putns(sdtypep, sdtypep->verilogKwd());  // "struct"/"union"
        puts(" " + EmitCUtil::اگیتر_Name_Protect(sdtypep) + " {\n");
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
            emit_Member_سمتیہ<AttributeType::چوڑائی_2>(sdtypep);

            putns(sdtypep, "\nstd::vector<int> memberDimension(void) const {\n");
            emit_Member_سمتیہ<AttributeType::DIMENSION>(sdtypep);

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
        putns(sdtypep, "\nbool operator==(const " + EmitCUtil::اگیتر_Name_Protect(sdtypep)
                           + "& rhs) const {\n");
        puts("return ");
        لكل (ثابت AstMemberDType* itemp = sdtypep->membersp(); itemp;
             itemp = VN_AS(itemp->nextp(), MemberDType)) {
            إذا (itemp != sdtypep->membersp()) puts("\n    && ");
            putns(itemp, itemp->nameProtect() + " == " + "rhs." + itemp->nameProtect());
        }
        puts(";\n");
        puts("}\n");
        putns(sdtypep, "bool operator!=(const " + EmitCUtil::اگیتر_Name_Protect(sdtypep)
                           + "& rhs) const {\n");
        puts("return !(*this == rhs);\n}\n");
        putns(sdtypep, "\nbool operator<(const " + EmitCUtil::اگیتر_Name_Protect(sdtypep)
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
        putns(sdtypep, "struct VlIsCustomStruct<" + EmitCUtil::اگیتر_Name_Protect(sdtypep)
                           + "> : public std::true_type {};\n");
    }

    // getfunc: VL_ASSIGNSEL_XX(rbits, obits, off, lhsdata, rhsdata);
    // !getfunc: VL_SELASSIGN_XX(rbits, obits, lhsdata, rhsdata, off);
    فراغ emit_Vl_سونپو(ثابت Ast_گرہ_DType* ثابت lhstype, ثابت Ast_گرہ_DType* rhstype,
                      ثابت std::string& off, ثابت std::string& lhsdata,
                      ثابت std::string& rhsdata, منطقي getfunc) {
        puts(getfunc ? "VL_ASSIGNSEL_" : "VL_SELASSIGN_");
        puts(lhstype->charIQWN());
        puts(rhstype->charIQWN());
        puts("(" + std::منزل_string(lhstype->چوڑائی()) + ", ");  // LHS width
        إذا (getfunc) {
            puts(std::منزل_string(rhstype->چوڑائی()) + ", ");  // Number of copy bits
            puts(off + ", ");  // LHS offset
        } وإلا {
            // Number of copy bits. Use widthTototalBytes to
            // make VL_SELASSIGN_XX clear upper unused bits for us.
            // puts(std::to_string(lhstype->width()) + ", ");
            puts(std::منزل_string(lhstype->چوڑائی_کل_بائٹاں() * 8) + ", ");
        }
        puts(lhsdata + ", ");  // LHS data
        puts(rhsdata);  // RHS data
        إذا (!getfunc) {
            puts(", " + off);  // RHS offset
        }
        puts(");\n");
    }

    // `retOrArg` should be prefixed by `&` or suffixed by `.data()` depending on its type
    فراغ emitPackedMember(ثابت Ast_گرہ_DType* ماپا_Dtypep, ثابت Ast_گرہ_DType* dtypep,
                          ثابت std::string& fieldname, ثابت std::string& وتھ, منطقي getfunc,
                          ثابت std::string& retOrArg) {
        dtypep = dtypep->skipRefp();
        إذا (ثابت خودکار* adtypep = VN_CAST(dtypep, PackArrayDType)) {
            ثابت std::string اشاریہ = m_ناں.get("__Vi");
            puts("for (int " + اشاریہ + " = 0; " + اشاریہ + " < "
                 + std::منزل_string(adtypep->elementsConst()) + "; ++" + اشاریہ + ") {\n");

            ثابت std::string وتھ_وچ_Loop
                = وتھ + " + " + اشاریہ + " * " + std::منزل_string(adtypep->subDTypep()->چوڑائی());
            ثابت std::string newName = fieldname + "[" + اشاریہ + "]";
            emitPackedMember(ماپا_Dtypep, adtypep->subDTypep(), newName, وتھ_وچ_Loop, getfunc,
                             retOrArg);
            puts("}\n");
        } وإلا إذا (VN_IS(dtypep, گرہ_UOr_Struct_DType)) {
            ثابت std::string tmp = m_ناں.get("__Vtmp");
            إذا (getfunc) {  // Emit `get` func;
                // auto __tmp = field.get();
                puts("auto " + tmp + " = " + fieldname + ".get();\n");
                // VL_ASSIGNSEL_XX(rbits, obits, lsb, lhsdata, rhsdata);
                emit_Vl_سونپو(ماپا_Dtypep, dtypep, وتھ, retOrArg, tmp, getfunc);
            } وإلا {  // Emit `set` func
                ثابت std::string tmptype = AstCDType::type_منزل_Hold(dtypep->چوڑائی());
                // type tmp;
                puts(tmptype + " " + tmp + ";\n");
                // VL_SELASSIGN_XX(rbits, obits, lhsdata, rhsdata, roffset);
                emit_Vl_سونپو(dtypep, ماپا_Dtypep, وتھ, tmp, retOrArg, getfunc);
                // field.set(__tmp);
                puts(fieldname + ".set(" + tmp + ");\n");
            }
        } وإلا {
            UASSERT_OBJ(VN_IS(dtypep, EnumDType) || VN_IS(dtypep, BasicDType), dtypep,
                        "Unsupported type in packed struct or union");
            إذا (getfunc) {  // Emit `get` func;
                // VL_ASSIGNSEL_XX(rbits, obits, lsb, lhsdata, rhsdata);
                emit_Vl_سونپو(ماپا_Dtypep, dtypep, وتھ, retOrArg, fieldname, getfunc);
            } وإلا {  // Emit `set` func
                // VL_SELASSIGN_XX(rbits, obits, lhsdata, rhsdata, roffset);
                emit_Vl_سونپو(dtypep, ماپا_Dtypep, وتھ, fieldname, retOrArg, getfunc);
            }
        }
    }
    فراغ emitPackedUOrSBody(Ast_گرہ_UOr_Struct_DType* sdtypep) {
        putns(sdtypep, sdtypep->verilogKwd());  // "struct"/"union"
        puts(" " + EmitCUtil::اگیتر_Name_Protect(sdtypep) + " {\n");

        AstMemberDType* itemp;
        AstMemberDType* آخری_Itemp;
        ثابت AstMemberDType* witemp = خالی_سوچک;
        // LSB is first field in C, so loop backwards
        لكل (آخری_Itemp = sdtypep->membersp(); آخری_Itemp && آخری_Itemp->nextp();
             آخری_Itemp = VN_AS(آخری_Itemp->nextp(), MemberDType)) {
            إذا (آخری_Itemp->چوڑائی() == sdtypep->چوڑائی()) witemp = آخری_Itemp;
        }
        لكل (itemp = آخری_Itemp; itemp; itemp = VN_CAST(itemp->backp(), MemberDType)) {
            putns(itemp, itemp->dtypep()->cType(itemp->nameProtect(), خطأ, خطأ, سچ));
            puts(";\n");
        }

        ثابت std::string retArgName = m_ناں.get("__v");
        ثابت std::string retArgType = AstCDType::type_منزل_Hold(sdtypep->چوڑائی());

        // Emit `get` member function
        puts(retArgType + " get() const {\n");
        puts(retArgType + " " + retArgName + ";\n");
        إذا (VN_IS(sdtypep, StructDType)) {
            لكل (itemp = آخری_Itemp; itemp; itemp = VN_CAST(itemp->backp(), MemberDType)) {
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
            لكل (itemp = آخری_Itemp; itemp; itemp = VN_CAST(itemp->backp(), MemberDType)) {
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
        m_ناں.reset();
    }
    فراغ emitStructs(ثابت Ast_گرہ_Module* modp) {
        // Track structs that've been emitted already
        std::سیٹ<Ast_گرہ_UOr_Struct_DType*> emitted;
        لكل (ثابت Ast_گرہ* nodep = modp->stmtsp(); nodep; nodep = nodep->nextp()) {
            ثابت AstTypedef* ثابت tdefp = VN_CAST(nodep, Typedef);
            إذا (!tdefp) استمر;
            Ast_گرہ_UOr_Struct_DType* ثابت sdtypep
                = VN_CAST(tdefp->dtypep()->skip_Ref_منزل_Enump(), گرہ_UOr_Struct_DType);
            إذا (!sdtypep) استمر;
            // V3Inline can copy one module into several parents. Only the module that
            // declares the struct emits it, so C++ sees only one definition.
            ثابت منطقي declared_وچ_Other_Module = sdtypep->classOrPackagep() != modp;
            إذا (declared_وچ_Other_Module) استمر;
            emitStructDecl(modp, sdtypep, emitted);
        }
    }
    فراغ emitFuncDecls(ثابت Ast_گرہ_Module* modp, منطقي وچ_Class_Body) {
        std::سمتیہ<ثابت AstCFunc*> funcsp;

        لكل (ثابت Ast_گرہ* nodep = modp->stmtsp(); nodep; nodep = nodep->nextp()) {
            إذا (ثابت AstCFunc* ثابت funcp = VN_CAST(nodep, CFunc)) {
                إذا (funcp->dpiImportPrototype())  // Declared in __Dpi.h
                    استمر;
                إذا (funcp->dpiExportDispatcher())  // Declared in __Dpi.h
                    استمر;
                إذا (funcp->is_طریقہ() != وچ_Class_Body)  // Only methods go inside class
                    استمر;
                إذا (funcp->is_طریقہ() && funcp->isLoose())  // Loose methods are declared lazily
                    استمر;
                funcsp.ڈالو_پٹھ(funcp);
            }
        }

        std::stable_ترتیب_دیو(funcsp.begin(), funcsp.سرا(), [](ثابت Ast_گرہ* ap, ثابت Ast_گرہ* bp) {
            أرجع ap->name() < bp->name();
        });

        لكل (ثابت AstCFunc* ثابت funcp : funcsp) {
            إذا (وچ_Class_Body) ofp()->puts_نجی(funcp->decl_نجی());
            emitCFuncDecl(funcp, modp);
        }
    }
    فراغ emitAll(ثابت Ast_گرہ_Module* modp) {
        // Include files required by this AstNodeModule
        إذا (ثابت AstClass* ثابت classp = VN_CAST(modp, Class)) {
            لكل (ثابت AstClassExtends* extp = classp->extendsp(); extp;
                 extp = VN_AS(extp->nextp(), ClassExtends)) {
                putns(extp, "#include \""
                                + EmitCUtil::اگیتر_Name_Protect(extp->classp()->classOrPackagep())
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
        puts(EmitCUtil::اگیتر_Name_Protect(modp));
        إذا (ثابت AstClass* ثابت classp = VN_CAST(modp, Class)) {
            puts(" : ");
            إذا (classp->extendsp()) {
                منطقي needComma = خطأ;
                لكل (ثابت AstClassExtends* extp = classp->extendsp(); extp;
                     extp = VN_AS(extp->nextp(), ClassExtends)) {
                    إذا (needComma) puts(", ");
                    // Use virtual only for interfaces for class inheritance
                    // (extends)
                    puts(extp->classp()->use_Virtual_عوامی() ? "virtual public " : "public ");
                    putns(extp, EmitCUtil::اگیتر_Name_Protect(extp->classp()));
                    needComma = سچ;
                }
            } وإلا {
                puts("public virtual VlClass");
            }
        } وإلا {
            puts(" final");
        }
        puts(" {\n");
        ofp()->reset_نجی();
        ofp()->puts_نجی(خطأ);  // public:

        // Emit all class body contents
        emitCellDecls(modp);
        emitEnums(modp);
        emitDesignVarDecls(modp);
        emit_اندرونی_Var_Decls(modp);
        emitParamDecls(modp);
        emitCtorDtorDecls(modp);
        emit_اندرونی_طریقہ_Decls(modp);
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
    فراغ ویکھو(Ast_گرہ* nodep) override { nodep->v3fatalSrc("Unused"); }  // LCOV_EXCL_LINE

    واضح EmitCHeader(ثابت Ast_گرہ_Module* modp) {
        UINFO(5, "  Emitting header for " << EmitCUtil::اگیتر_Name_Protect(modp));

        // Open output file
        open_New_Output_سرخی_File(EmitCUtil::اگیتر_Name_Protect(modp), "Design internal header");
        puts("// See " + EmitCUtil::چوٹی_Class_Name() + ".h for the primary calling header\n");

        ofp()->putsGuard();

        // Include files
        puts("\n");
        ofp()->puts_Int_چوٹی_Include();
        puts("#include \"verilated.h\"\n");
        إذا (modp->is_چوٹی() && optSystemC()) puts("#include \"verilated_sc.h\"\n");
        إذا (v3Global.opt.mtasks()) puts("#include \"verilated_threads.h\"\n");
        إذا (v3Global.opt.savable()) puts("#include \"verilated_save.h\"\n");
        إذا (v3Global.opt.coverage()) puts("#include \"verilated_cov.h\"\n");
        إذا (v3Global.opt.coverage() || v3Global.useCovergroup())
            puts("#include \"verilated_covergroup.h\"\n");
        إذا (v3Global.ورتدا_Timing()) puts("#include \"verilated_timing.h\"\n");
        إذا (v3Global.useRandomizeMethods()) puts("#include \"verilated_random.h\"\n");
        إذا (v3Global.ورتدا_Force()) puts("#include \"verilated_force.h\"\n");

        std::سیٹ<string> cuse_سیٹ;
        خودکار جوڑو_منزل_cuse_سیٹ = [&](string s) { cuse_سیٹ.داخل_کرو(s); };

        forModCUse(modp, VUseType::INT_FWD_CLASS | VUseType::INT_INCLUDE, جوڑو_منزل_cuse_سیٹ);
        إذا (ثابت AstClassPackage* ثابت packagep = VN_CAST(modp, ClassPackage)) {
            forModCUse(packagep->classp(), VUseType::INT_INCLUDE | VUseType::INT_FWD_CLASS,
                       جوڑو_منزل_cuse_سیٹ);
        }

        لكل (ثابت string& s : cuse_سیٹ) puts(s);
        puts("\n");

        emitAll(modp);

        إذا (ثابت AstClassPackage* ثابت packagep = VN_CAST(modp, ClassPackage)) {
            // Put the non-static class implementation in same h file for speed
            emitAll(packagep->classp());
        }

        ofp()->puts_سرا_Guard();

        // Close output file
        closeOutputFile();
    }
    ~EmitCHeader() override = افتراضي;

عام:
    ساكن فراغ مکھ(ثابت Ast_گرہ_Module* modp) { ثابت EmitCHeader emitCHeader{modp}; }
};

//######################################################################
// EmitC class functions

فراغ V3EmitC::emitc_سرخیاں() {
    UINFO(2, __FUNCTION__ << ":");

    // Process each module in turn
    لكل (ثابت Ast_گرہ* nodep = v3Global.rootp()->modulesp(); nodep; nodep = nodep->nextp()) {
        إذا (VN_IS(nodep, Class)) استمر;  // Declared with the ClassPackage
        ثابت Ast_گرہ_Module* ثابت modp = VN_AS(nodep, گرہ_Module);
        إذا (modp->isConstPool()) استمر;  // Emitted by V3EmitCConstPool
        EmitCHeader::مکھ(modp);
    }
}
