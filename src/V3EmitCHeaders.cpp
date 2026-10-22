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
#شامل "V3EmitCBase.h"
#شامل "V3File.h"
#شامل "V3MemberMap.h"
#شامل "V3UniqueNames.h"

#شامل <algorithm>
#شامل <cstdint>
#شامل <set>
#شامل <string>
#شامل <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Internal EmitC implementation

جماعت EmitCHeader final : عوامی EmitCBaseVisitorConst {
    V3_Unique_نام m_نام;
    VMemberMap m_memberMap;
    // METHODS

    جماعت Cover_گنتی_Visitor final : عوامی VNVisitorConst {
        صحیح m_bins = 0;  // Running total of coverage bins counted so far

        خالی دورہ(Ast_گرہ_Cover_Decl* nodep) override {
            // Each module class owns the counters for declarations it emits;
            // duplicate no-inline instances need separate arrays for
            // forcePerInstance reporting.
            اگر (!nodep->ڈیٹا_Decl_Nullp()) m_bins += nodep->سائز();
        }
        خالی دورہ(Ast_گرہ* nodep) override { iterateChildrenConst(nodep); }

    عوامی:
        صریح Cover_گنتی_Visitor(ثابت Ast_گرہ_Module* modp) {
            iterateConst(const_cast<Ast_گرہ_Module*>(modp));
        }
        صحیح bins() ثابت { واپس m_bins; }
    };

    خالی decorate_پہلا(منطقی& پہلا, ثابت string& str) {
        اگر (پہلا) {
            putsDecoration(خالی_اشاریہ, str);
            پہلا = جھوٹ;
        }
    }
    خالی emitCellDecls(ثابت Ast_گرہ_Module* modp) {
        منطقی پہلا = سچ;
        برائے (ثابت Ast_گرہ* nodep = modp->stmtsp(); nodep; nodep = nodep->nextp()) {
            اگر (ثابت AstCell* ثابت cellp = VN_CAST(nodep, Cell)) {
                اگر (cellp->modp()->isConstPool()) جاری;  // Special emit rules
                decorate_پہلا(پہلا, "// CELLS\n");
                putns(cellp, EmitCUtil::سابقہ_Name_Protect(cellp->modp()) + "* "
                                 + cellp->nameProtect() + ";\n");
            }
        }
    }
    خالی emitDesignVarDecls(ثابت Ast_گرہ_Module* modp) {
        // V3VariableOrder puts all variables at the front of the list: top level ports, then
        // trivially constructible variables, then the not trivially constructible ones, then
        // those that are not model state
        ثابت AstVar* portp = خالی_اشاریہ;
        ثابت AstVar* trivp = خالی_اشاریہ;
        ثابت AstVar* nontp = خالی_اشاریہ;
        سائز_t n_پورٹ = 0;
        سائز_t nTriv = 0;
        سائز_t nNont = 0;
        برائے (ثابت Ast_گرہ* nodep = modp->stmtsp(); nodep; nodep = nodep->nextp()) {
            ثابت AstVar* ثابت varp = VN_CAST(nodep, Var);
            اگر (!varp) توڑو;
            اگر (!varp->isModelState()) {
                // Cache line alignment is only effective on emitted fields
                UASSERT_OBJ(!varp->mtask_Cache_سطر_Align(), varp, "Aligned non-field");
            } ورنہ اگر (varp->isPrimaryIO()) {
                اگر (!portp) portp = varp;
                ++n_پورٹ;
            } ورنہ اگر (varp->isTriviallyConstructible()) {
                اگر (!trivp) trivp = varp;
                ++nTriv;
            } ورنہ {
                اگر (!nontp) nontp = varp;
                ++nNont;
            }
        }

        ثابت خودکار emit_And_قدم = [&](ثابت AstVar*& varpr) {
            emitVarDecl(varpr);
            varpr = VN_CAST(varpr->nextp(), Var);
        };

        // Emit primary ports
        اگر (n_پورٹ) {
            putsDecoration(خالی_اشاریہ, "\n// TOP LEVEL PORTS\n");
            // In anonymous union to prevent the C++ compiler from generating
            // constructor/destructor calls
            puts("union {\nstruct {\n");
            برائے (سائز_t i = 0; i < n_پورٹ; ++i) emit_And_قدم(portp);
            puts("} __VprimaryPorts;\n};\n");
        }

        // Emit trivially constructible variables
        اگر (nTriv) {
            putsDecoration(خالی_اشاریہ, "\n// TRIVIALLY CONSTRUCTIBLE STATE\n");
            اگر (v3Global.opt.comp_حد_بندی_Members() == 0) {
                // Anonymous structs disabled, so just emit them directly
                برائے (سائز_t i = 0; i < nTriv; ++i) emit_And_قدم(trivp);
            } ورنہ {
                ثابت سائز_t lim = v3Global.opt.comp_حد_بندی_Members();
                سائز_t anonL3s = 1;
                سائز_t anonL2s = 1;
                سائز_t anonL1s = 1;
                اگر (nTriv > (lim * lim * lim)) {
                    anonL3s = (nTriv + (lim * lim * lim) - 1) / (lim * lim * lim);
                    anonL2s = lim;
                    anonL1s = lim;
                } ورنہ اگر (nTriv > (lim * lim)) {
                    anonL2s = (nTriv + (lim * lim) - 1) / (lim * lim);
                    anonL1s = lim;
                } ورنہ اگر (nTriv > lim) {
                    anonL1s = (nTriv + lim - 1) / lim;
                }
                اگر (anonL1s != 1) {
                    putsDecoration(
                        خالی_اشاریہ,
                        "// Anonymous structures to workaround compiler member-count bugs\n");
                }
                سائز_t nEmitted = 0;
                برائے (سائز_t l3 = 0; l3 < anonL3s && nEmitted < nTriv; ++l3) {
                    اگر (anonL3s != 1) puts("struct {\n");
                    برائے (سائز_t l2 = 0; l2 < anonL2s && nEmitted < nTriv; ++l2) {
                        اگر (anonL2s != 1) puts("struct {\n");
                        برائے (سائز_t l1 = 0; l1 < anonL1s && nEmitted < nTriv; ++l1) {
                            اگر (anonL1s != 1) puts("struct {\n");
                            برائے (سائز_t l0 = 0; l0 < lim && nEmitted < nTriv; ++l0, ++nEmitted) {
                                emit_And_قدم(trivp);
                            }
                            اگر (anonL1s != 1) puts("};\n");
                        }
                        اگر (anonL2s != 1) puts("};\n");
                    }
                    اگر (anonL3s != 1) puts("};\n");
                }
                UASSERT_OBJ(nEmitted == nTriv, modp, "Not all variables emitted");
            }
        }

        // Emit not trivially constructible variables
        اگر (nNont) {
            putsDecoration(خالی_اشاریہ, "\n// NOT TRIVIALLY CONSTRUCTIBLE STATE\n");
            // Classes use real constructors
            اگر (VN_IS(modp, Class)) {
                برائے (سائز_t i = 0; i < nNont; ++i) emit_And_قدم(nontp);
                واپس;
            }
            // Output under an anonymous union to prevent the C++ compiler from generating
            // constructor/destructor calls for them. They are managed explicitly in ctor/dtor.
            // While 'union { singleVar; }' for each variable would be simpler, it blows up
            // clang's compile time when using pre-compiled headers, joy.
            puts("union {\nstruct {\n");
            برائے (سائز_t i = 0; i < nNont; ++i) emit_And_قدم(nontp);
            puts("} __Vnontrivs;\n};\n");
        }
    }
    خالی emit_اندرونی_Var_Decls(ثابت Ast_گرہ_Module* modp) {
        اگر (ثابت AstClass* ثابت classp = VN_CAST(modp, Class)) {
            اگر (classp->needRNG()) {
                putsDecoration(خالی_اشاریہ, "\n// INTERNAL VARIABLES\n");
                puts("VlRNGReseeds __Vm_rng;\n");
            }
        } ورنہ {  // not class
            putsDecoration(خالی_اشاریہ, "\n// INTERNAL VARIABLES\n");
            puts(EmitCUtil::symClassName() + "* vlSymsp;\n");
            puts("const char* vlNamep;\n");
            // Allocate object-local coverage counters only on modules that
            // contain emitted coverage declarations.
            ثابت صحیح coverBins = Cover_گنتی_Visitor{modp}.bins();
            اگر (coverBins) {
                puts(v3Global.opt.دھاگے() > 1 ? "std::atomic<uint32_t>" : "uint32_t");
                puts(" __Vcoverage[");
                puts(std::مقصد_string(coverBins));
                puts("]{};\n");
            }
        }
    }
    خالی emitParamDecls(ثابت Ast_گرہ_Module* modp) {
        منطقی پہلا = سچ;
        برائے (Ast_گرہ* nodep = modp->stmtsp(); nodep; nodep = nodep->nextp()) {
            اگر (ثابت AstVar* ثابت varp = VN_CAST(nodep, Var)) {
                اگر (varp->isParam()) {
                    decorate_پہلا(پہلا, "\n// PARAMETERS\n");
                    putns(varp, "static const ");
                    puts(varp->dtypep()->cType(varp->nameProtect(), جھوٹ, جھوٹ));
                    puts(";\n");
                }
            }
        }
    }
    خالی emitCtorDtorDecls(ثابت Ast_گرہ_Module* modp) {
        // Classes use CFuncs with isConstructor/isDestructor
        اگر (VN_IS(modp, Class)) واپس;

        // Proper modules use loose split ctor/dtor and are not copyable/movable
        ثابت std::string name = EmitCUtil::سابقہ_Name_Protect(modp);
        putsDecoration(خالی_اشاریہ, "\n// CONSTRUCTORS\n");
        putns(modp, name + "(" + EmitCUtil::symClassName() + "* symsp, const char* namep);\n");
        putns(modp, "~" + name + "();\n");
        putns(modp, "VL_UNCOPYABLE(" + name + ");\n");
        putns(modp, "VL_UNMOVABLE(" + name + ");\n");
    }
    خالی emit_اندرونی_طریقہ_Decls(ثابت Ast_گرہ_Module* modp) {
        منطقی پہلا = سچ;
        ثابت string section = "\n// INTERNAL METHODS\n";

        اگر (!VN_IS(modp, Class)) {
            decorate_پہلا(پہلا, section);
            اگر (v3Global.opt.coverage()) {
                puts("void " + protect("__Vconfigure") + "(bool first);\n");
            }
        } ورنہ {
            decorate_پہلا(پہلا, section);
            ثابت std::string name = V3_برآمد_Formatter::quoteNameControls(
                VIdProtect::protect_الفاظ_If(modp->prettyName(), v3Global.opt.protectIds()));
            // "override", but don't want clang to start checking them:
            puts("virtual const char* typeName() const { return \""s + name + "\"; }\n");
        }

        اگر (v3Global.opt.coverage() && !VN_IS(modp, Class)) {
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

        اگر (v3Global.opt.coverageToggle() && !VN_IS(modp, Class)) {
            decorate_پہلا(پہلا, section);
            puts("void __vlCoverToggleInsert(int begin, int end, bool ranged, ");
            puts(v3Global.opt.دھاگے() > 1 ? "std::atomic<uint32_t>" : "uint32_t");
            puts("* countp, bool enable, bool localCounter, const char* filenamep, int lineno, "
                 "int column,\n");
            puts("const char* hierp, const char* pagep, const char* commentp);\n");
        }

        اگر (v3Global.opt.savable()) {
            decorate_پہلا(پہلا, section);
            puts("void " + protect("__Vserialize") + "(VerilatedSerialize& os);\n");
            puts("void " + protect("__Vdeserialize") + "(VerilatedDeserialize& os);\n");
        }

        // Polymorphic clone for concrete (non-abstract, non-interface) classes
        اگر (ثابت AstClass* ثابت classp = VN_CAST(modp, Class)) {
            اگر (!classp->isInterfaceClass() && !classp->isVirtual()) {
                decorate_پہلا(پہلا, section);
                استعمال EmbeddedCovergroupVar = std::pair<ثابت AstClass*, ثابت AstVar*>;
                std::سمتیہ<EmbeddedCovergroupVar> embeddedCovergroupVars;
                ثابت خودکار has_Enclosing_عقب_اشارہ_گر = [](ثابت AstClass* covergroupp) {
                    واپس covergroupp->exists([](ثابت AstVar* ثابت varp) {
                        ثابت AstClassRefDType* ثابت refp
                            = VN_CAST(varp->dtypep()->skipRefp(), ClassRefDType);
                        واپس refp && refp->raw_اشارہ_گر();
                    });
                };
                const_cast<AstClass*>(classp)->foreachMember(
                    [&](AstClass* ثابت memberClassp, AstVar* ثابت varp) {
                        ثابت AstClassRefDType* ثابت refp
                            = VN_CAST(varp->dtypep()->skipRefp(), ClassRefDType);
                        اگر (refp && refp->classp()->isCovergroup()
                            && has_Enclosing_عقب_اشارہ_گر(refp->classp())) {
                            embeddedCovergroupVars.emplace_عقب(memberClassp, varp);
                        }
                    });
                ثابت string className = EmitCUtil::سابقہ_Name_Protect(classp);
                اگر (embeddedCovergroupVars.خالی_خانے() && !classp->hasRandVarsUpdate()) {
                    putns(classp,
                          "VlClass* clone() const { return new " + className + "(*this); }\n");
                } ورنہ {
                    putns(classp, "VlClass* clone() const { " + className + "* const clonep = new "
                                      + className + "(*this); ");
                    اگر (classp->hasRandVarsUpdate()) {
                        ثابت string updateName = "__VnoInFunc___VupdateRandVars";
                        AstCFunc* ثابت updatep
                            = VN_AS(m_memberMap.ڈھونڈو_Member(classp, updateName), CFunc);
                        UASSERT_OBJ(updatep, classp, "Missing updateRandVars method");
                        puts("clonep->" + updatep->nameProtect() + "();\n");
                    }
                    برائے (ثابت EmbeddedCovergroupVar& شے_3 : embeddedCovergroupVars) {
                        puts("clonep->" + EmitCUtil::سابقہ_Name_Protect(شے_3.پہلا)
                             + "::" + شے_3.دوسرا->nameProtect() + " = VlNull{}; ");
                    }
                    puts("return clonep; }\n");
                }
            }
        }
    }
    خالی emitEnums(ثابت Ast_گرہ_Module* modp) {
        منطقی پہلا = سچ;
        برائے (ثابت Ast_گرہ* nodep = modp->stmtsp(); nodep; nodep = nodep->nextp()) {
            ثابت AstTypedef* ثابت tdefp = VN_CAST(nodep, Typedef);
            اگر (!tdefp) جاری;
            اگر (!tdefp->attr_عوامی()) جاری;
            ثابت AstEnumDType* ثابت edtypep
                = VN_CAST(tdefp->dtypep()->skip_Ref_مقصد_Enump(), EnumDType);
            اگر (!edtypep) جاری;
            decorate_پہلا(پہلا, "\n// ENUMS (that were declared public)\n");
            اگر (edtypep->چوڑائی() > 64) {
                putsDecoration(tdefp,
                               "// enum " + tdefp->nameProtect() + " ignored: Too wide for C++\n");
            } ورنہ {
                putns(tdefp, "enum " + tdefp->name() + " {\n");
                برائے (ثابت Ast_Enum_شے* itemp = edtypep->itemsp(); itemp;
                     itemp = VN_AS(itemp->nextp(), Enum_شے)) {
                    اگر (ثابت AstConst* ثابت constp = VN_CAST(itemp->valuep(), Const)) {
                        اگر (constp->عدد().isFourState()) {
                            putns(itemp, "// " + itemp->nameProtect() + " is four-state\n");
                            جاری;
                        }
                    }
                    putns(itemp, itemp->nameProtect());
                    puts(" = ");
                    putns(itemp, VN_AS(itemp->valuep(), Const)->عدد().emitC());
                    اگر (VN_IS(itemp->nextp(), Enum_شے)) puts(",");
                    puts("\n");
                }
                puts("};\n");
            }
        }
    }
    خالی emitStructDecl(ثابت Ast_گرہ_Module* modp, Ast_گرہ_UOr_Struct_DType* sdtypep,
                        std::سیٹ<Ast_گرہ_UOr_Struct_DType*>& emitted) {
        اگر (emitted.گنتی(sdtypep) > 0) واپس;
        emitted.داخل_کرو(sdtypep);
        برائے (AstMemberDType* itemp = sdtypep->membersp(); itemp;
             itemp = VN_AS(itemp->nextp(), MemberDType)) {
            Ast_گرہ_UOr_Struct_DType* ثابت subp = itemp->get_بچہ_Structp();
            اگر (subp && (!subp->packed() || sdtypep->packed())) {
                // Recurse if it belongs to the current module
                اگر (subp->classOrPackagep() == modp) {
                    emitStructDecl(modp, subp, emitted);
                    puts("\n");
                }
            }
        }
        اگر (sdtypep->packed()) {
            emitPackedUOrSBody(sdtypep);
        } ورنہ {
            emitUnpackedUOrSBody(sdtypep);
        }
    }
    شمار جماعت AttributeType : uint8_t { چوڑائی_2, DIMENSION };
    // Get member attribute based on type
    صحیح get_گرہ_Attribute(ثابت AstMemberDType* itemp, AttributeType type) {
        ثابت منطقی isArrayType = itemp->dtypep()->isNonPackedArray();
        انتخاب (type) {
        حالت AttributeType::چوڑائی_2: {
            اگر (isArrayType) {
                // For arrays, get innermost element width
                ثابت Ast_گرہ_DType* dtype = itemp->dtypep();
                جب (dtype->subDTypep()) dtype = dtype->subDTypep();
                واپس dtype->چوڑائی();
            }
            واپس itemp->چوڑائی();
        }
        حالت AttributeType::DIMENSION: {
            // Return array dimension or 0 for non-arrays
            واپس isArrayType ? itemp->dtypep()->dimensions(سچ).دوسرا : 0;
        }
        طےشدہ: {
            واپس 0;
        }
        }
    }
    سانچہ <AttributeType T>
    خالی emit_Member_سمتیہ(ثابت Ast_گرہ_UOr_Struct_DType* sdtypep) {
        puts("return {");
        منطقی needComma = جھوٹ;
        برائے (ثابت AstMemberDType* itemp = sdtypep->membersp(); itemp;
             itemp = VN_AS(itemp->nextp(), MemberDType)) {
            اگر (!itemp->isConstrainedRand()) جاری;
            // Comma handling: add before element except first
            اگر (needComma) puts(",\n");
            putns(itemp, std::مقصد_string(get_گرہ_Attribute(itemp, T)));
            needComma = سچ;
        }
        puts("};\n}\n");
    }
    خالی emitUnpackedUOrSBody(Ast_گرہ_UOr_Struct_DType* sdtypep) {
        putns(sdtypep, sdtypep->verilogKwd());  // "struct"/"union"
        puts(" " + EmitCUtil::سابقہ_Name_Protect(sdtypep) + " {\n");
        برائے (ثابت AstMemberDType* itemp = sdtypep->membersp(); itemp;
             itemp = VN_AS(itemp->nextp(), MemberDType)) {
            putns(itemp, itemp->dtypep()->cType(itemp->nameProtect(), جھوٹ, جھوٹ));
            puts(";\n");
        }

        // Three helper functions for struct constrained randomization:
        // - memberNames: Get member names
        // - getMembers: Access member references
        // - memberIndices: Retrieve member indices
        // - memberWidth: Retrieve member width
        // - memberDimension: Retrieve member dimension
        اگر (sdtypep->isConstrainedRand()) {
            منطقی needComma = جھوٹ;
            putns(sdtypep, "\nstd::vector<std::string> memberNames(void) const {\n");
            puts("return {");
            برائے (ثابت AstMemberDType* itemp = sdtypep->membersp(); itemp;
                 itemp = VN_AS(itemp->nextp(), MemberDType)) {
                اگر (!itemp->isConstrainedRand()) جاری;
                اگر (needComma) puts(",\n");
                putns(itemp, "\"" + itemp->shortName() + "\"");
                needComma = سچ;
            }
            puts("};\n}\n");

            putns(sdtypep, "\nstd::vector<int> memberWidth(void) const {\n");
            emit_Member_سمتیہ<AttributeType::چوڑائی_2>(sdtypep);

            putns(sdtypep, "\nstd::vector<int> memberDimension(void) const {\n");
            emit_Member_سمتیہ<AttributeType::DIMENSION>(sdtypep);

            needComma = جھوٹ;
            putns(sdtypep, "\nauto memberIndices(void) const {\n");
            puts("return std::index_sequence_for<");
            برائے (ثابت AstMemberDType* itemp = sdtypep->membersp(); itemp;
                 itemp = VN_AS(itemp->nextp(), MemberDType)) {
                اگر (!itemp->isConstrainedRand()) جاری;
                اگر (needComma) puts(",\n");
                putns(itemp, itemp->dtypep()->cType("", جھوٹ, جھوٹ));
                needComma = سچ;
            }
            puts(">{};\n}\n");

            needComma = جھوٹ;
            putns(sdtypep, "\ntemplate <typename T>");
            putns(sdtypep, "\nauto getMembers(T& obj) {\n");
            puts("return std::tie(");
            برائے (ثابت AstMemberDType* itemp = sdtypep->membersp(); itemp;
                 itemp = VN_AS(itemp->nextp(), MemberDType)) {
                اگر (!itemp->isConstrainedRand()) جاری;
                اگر (needComma) puts(",\n");
                putns(itemp, "obj." + itemp->nameProtect());
                needComma = سچ;
            }
            puts(");\n}\n");
        }
        putns(sdtypep, "\nbool operator==(const " + EmitCUtil::سابقہ_Name_Protect(sdtypep)
                           + "& rhs) const {\n");
        puts("return ");
        برائے (ثابت AstMemberDType* itemp = sdtypep->membersp(); itemp;
             itemp = VN_AS(itemp->nextp(), MemberDType)) {
            اگر (itemp != sdtypep->membersp()) puts("\n    && ");
            putns(itemp, itemp->nameProtect() + " == " + "rhs." + itemp->nameProtect());
        }
        puts(";\n");
        puts("}\n");
        putns(sdtypep, "bool operator!=(const " + EmitCUtil::سابقہ_Name_Protect(sdtypep)
                           + "& rhs) const {\n");
        puts("return !(*this == rhs);\n}\n");
        putns(sdtypep, "\nbool operator<(const " + EmitCUtil::سابقہ_Name_Protect(sdtypep)
                           + "& rhs) const {\n");
        برائے (ثابت AstMemberDType* itemp = sdtypep->membersp(); itemp;
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
        putns(sdtypep, "struct VlIsCustomStruct<" + EmitCUtil::سابقہ_Name_Protect(sdtypep)
                           + "> : public std::true_type {};\n");
    }

    // getfunc: VL_ASSIGNSEL_XX(rbits, obits, off, lhsdata, rhsdata);
    // !getfunc: VL_SELASSIGN_XX(rbits, obits, lhsdata, rhsdata, off);
    خالی emit_Vl_تفویض(ثابت Ast_گرہ_DType* ثابت lhstype, ثابت Ast_گرہ_DType* rhstype,
                      ثابت std::string& off, ثابت std::string& lhsdata,
                      ثابت std::string& rhsdata, منطقی getfunc) {
        puts(getfunc ? "VL_ASSIGNSEL_" : "VL_SELASSIGN_");
        puts(lhstype->charIQWN());
        puts(rhstype->charIQWN());
        puts("(" + std::مقصد_string(lhstype->چوڑائی()) + ", ");  // LHS width
        اگر (getfunc) {
            puts(std::مقصد_string(rhstype->چوڑائی()) + ", ");  // Number of copy bits
            puts(off + ", ");  // LHS offset
        } ورنہ {
            // Number of copy bits. Use widthTototalBytes to
            // make VL_SELASSIGN_XX clear upper unused bits for us.
            // puts(std::to_string(lhstype->width()) + ", ");
            puts(std::مقصد_string(lhstype->چوڑائی_کل_بائٹیں() * 8) + ", ");
        }
        puts(lhsdata + ", ");  // LHS data
        puts(rhsdata);  // RHS data
        اگر (!getfunc) {
            puts(", " + off);  // RHS offset
        }
        puts(");\n");
    }

    // `retOrArg` should be prefixed by `&` or suffixed by `.data()` depending on its type
    خالی emitPackedMember(ثابت Ast_گرہ_DType* والد_Dtypep, ثابت Ast_گرہ_DType* dtypep,
                          ثابت std::string& fieldname, ثابت std::string& انحراف, منطقی getfunc,
                          ثابت std::string& retOrArg) {
        dtypep = dtypep->skipRefp();
        اگر (ثابت خودکار* adtypep = VN_CAST(dtypep, PackArrayDType)) {
            ثابت std::string اشاریہ = m_نام.get("__Vi");
            puts("for (int " + اشاریہ + " = 0; " + اشاریہ + " < "
                 + std::مقصد_string(adtypep->elementsConst()) + "; ++" + اشاریہ + ") {\n");

            ثابت std::string انحراف_اندر_Loop
                = انحراف + " + " + اشاریہ + " * " + std::مقصد_string(adtypep->subDTypep()->چوڑائی());
            ثابت std::string newName = fieldname + "[" + اشاریہ + "]";
            emitPackedMember(والد_Dtypep, adtypep->subDTypep(), newName, انحراف_اندر_Loop, getfunc,
                             retOrArg);
            puts("}\n");
        } ورنہ اگر (VN_IS(dtypep, گرہ_UOr_Struct_DType)) {
            ثابت std::string tmp = m_نام.get("__Vtmp");
            اگر (getfunc) {  // Emit `get` func;
                // auto __tmp = field.get();
                puts("auto " + tmp + " = " + fieldname + ".get();\n");
                // VL_ASSIGNSEL_XX(rbits, obits, lsb, lhsdata, rhsdata);
                emit_Vl_تفویض(والد_Dtypep, dtypep, انحراف, retOrArg, tmp, getfunc);
            } ورنہ {  // Emit `set` func
                ثابت std::string tmptype = AstCDType::type_مقصد_Hold(dtypep->چوڑائی());
                // type tmp;
                puts(tmptype + " " + tmp + ";\n");
                // VL_SELASSIGN_XX(rbits, obits, lhsdata, rhsdata, roffset);
                emit_Vl_تفویض(dtypep, والد_Dtypep, انحراف, tmp, retOrArg, getfunc);
                // field.set(__tmp);
                puts(fieldname + ".set(" + tmp + ");\n");
            }
        } ورنہ {
            UASSERT_OBJ(VN_IS(dtypep, EnumDType) || VN_IS(dtypep, BasicDType), dtypep,
                        "Unsupported type in packed struct or union");
            اگر (getfunc) {  // Emit `get` func;
                // VL_ASSIGNSEL_XX(rbits, obits, lsb, lhsdata, rhsdata);
                emit_Vl_تفویض(والد_Dtypep, dtypep, انحراف, retOrArg, fieldname, getfunc);
            } ورنہ {  // Emit `set` func
                // VL_SELASSIGN_XX(rbits, obits, lhsdata, rhsdata, roffset);
                emit_Vl_تفویض(dtypep, والد_Dtypep, انحراف, fieldname, retOrArg, getfunc);
            }
        }
    }
    خالی emitPackedUOrSBody(Ast_گرہ_UOr_Struct_DType* sdtypep) {
        putns(sdtypep, sdtypep->verilogKwd());  // "struct"/"union"
        puts(" " + EmitCUtil::سابقہ_Name_Protect(sdtypep) + " {\n");

        AstMemberDType* itemp;
        AstMemberDType* آخری_Itemp;
        ثابت AstMemberDType* witemp = خالی_اشاریہ;
        // LSB is first field in C, so loop backwards
        برائے (آخری_Itemp = sdtypep->membersp(); آخری_Itemp && آخری_Itemp->nextp();
             آخری_Itemp = VN_AS(آخری_Itemp->nextp(), MemberDType)) {
            اگر (آخری_Itemp->چوڑائی() == sdtypep->چوڑائی()) witemp = آخری_Itemp;
        }
        برائے (itemp = آخری_Itemp; itemp; itemp = VN_CAST(itemp->backp(), MemberDType)) {
            putns(itemp, itemp->dtypep()->cType(itemp->nameProtect(), جھوٹ, جھوٹ, سچ));
            puts(";\n");
        }

        ثابت std::string retArgName = m_نام.get("__v");
        ثابت std::string retArgType = AstCDType::type_مقصد_Hold(sdtypep->چوڑائی());

        // Emit `get` member function
        puts(retArgType + " get() const {\n");
        puts(retArgType + " " + retArgName + ";\n");
        اگر (VN_IS(sdtypep, StructDType)) {
            برائے (itemp = آخری_Itemp; itemp; itemp = VN_CAST(itemp->backp(), MemberDType)) {
                emitPackedMember(sdtypep, itemp->dtypep(), itemp->nameProtect(),
                                 std::مقصد_string(itemp->lsb()), /*getfunc=*/سچ, retArgName);
            }
        } ورنہ {
            // We only need to fill the widest field of union
            emitPackedMember(sdtypep, witemp->dtypep(), witemp->nameProtect(),
                             std::مقصد_string(witemp->lsb()), /*getfunc=*/سچ, retArgName);
        }
        puts("return " + retArgName + ";\n");
        puts("}\n");

        // Emit `set` member function
        puts("void set(const " + retArgType + "& " + retArgName + ") {\n");
        اگر (VN_IS(sdtypep, StructDType)) {
            برائے (itemp = آخری_Itemp; itemp; itemp = VN_CAST(itemp->backp(), MemberDType)) {
                emitPackedMember(sdtypep, itemp->dtypep(), itemp->nameProtect(),
                                 std::مقصد_string(itemp->lsb()), /*getfunc=*/جھوٹ, retArgName);
            }
        } ورنہ {
            // We only need to fill the widest field of union
            emitPackedMember(sdtypep, witemp->dtypep(), witemp->nameProtect(),
                             std::مقصد_string(witemp->lsb()), /*getfunc=*/جھوٹ, retArgName);
        }

        puts("}\n");

        puts("};\n");
        m_نام.reset();
    }
    خالی emitStructs(ثابت Ast_گرہ_Module* modp) {
        // Track structs that've been emitted already
        std::سیٹ<Ast_گرہ_UOr_Struct_DType*> emitted;
        برائے (ثابت Ast_گرہ* nodep = modp->stmtsp(); nodep; nodep = nodep->nextp()) {
            ثابت AstTypedef* ثابت tdefp = VN_CAST(nodep, Typedef);
            اگر (!tdefp) جاری;
            Ast_گرہ_UOr_Struct_DType* ثابت sdtypep
                = VN_CAST(tdefp->dtypep()->skip_Ref_مقصد_Enump(), گرہ_UOr_Struct_DType);
            اگر (!sdtypep) جاری;
            // V3Inline can copy one module into several parents. Only the module that
            // declares the struct emits it, so C++ sees only one definition.
            ثابت منطقی declared_اندر_Other_Module = sdtypep->classOrPackagep() != modp;
            اگر (declared_اندر_Other_Module) جاری;
            emitStructDecl(modp, sdtypep, emitted);
        }
    }
    خالی emitFuncDecls(ثابت Ast_گرہ_Module* modp, منطقی اندر_Class_Body) {
        std::سمتیہ<ثابت AstCFunc*> funcsp;

        برائے (ثابت Ast_گرہ* nodep = modp->stmtsp(); nodep; nodep = nodep->nextp()) {
            اگر (ثابت AstCFunc* ثابت funcp = VN_CAST(nodep, CFunc)) {
                اگر (funcp->dpiImportPrototype())  // Declared in __Dpi.h
                    جاری;
                اگر (funcp->dpiExportDispatcher())  // Declared in __Dpi.h
                    جاری;
                اگر (funcp->is_طریقہ() != اندر_Class_Body)  // Only methods go inside class
                    جاری;
                اگر (funcp->is_طریقہ() && funcp->isLoose())  // Loose methods are declared lazily
                    جاری;
                funcsp.ڈالو_عقب(funcp);
            }
        }

        std::stable_ترتیب_دو(funcsp.begin(), funcsp.انتہا(), [](ثابت Ast_گرہ* ap, ثابت Ast_گرہ* bp) {
            واپس ap->name() < bp->name();
        });

        برائے (ثابت AstCFunc* ثابت funcp : funcsp) {
            اگر (اندر_Class_Body) ofp()->puts_نجی(funcp->decl_نجی());
            emitCFuncDecl(funcp, modp);
        }
    }
    خالی emitAll(ثابت Ast_گرہ_Module* modp) {
        // Include files required by this AstNodeModule
        اگر (ثابت AstClass* ثابت classp = VN_CAST(modp, Class)) {
            برائے (ثابت AstClassExtends* extp = classp->extendsp(); extp;
                 extp = VN_AS(extp->nextp(), ClassExtends)) {
                putns(extp, "#include \""
                                + EmitCUtil::سابقہ_Name_Protect(extp->classp()->classOrPackagep())
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
        اگر (!VN_IS(modp, Class)) puts("alignas(VL_CACHE_LINE_BYTES) ");
        puts(EmitCUtil::سابقہ_Name_Protect(modp));
        اگر (ثابت AstClass* ثابت classp = VN_CAST(modp, Class)) {
            puts(" : ");
            اگر (classp->extendsp()) {
                منطقی needComma = جھوٹ;
                برائے (ثابت AstClassExtends* extp = classp->extendsp(); extp;
                     extp = VN_AS(extp->nextp(), ClassExtends)) {
                    اگر (needComma) puts(", ");
                    // Use virtual only for interfaces for class inheritance
                    // (extends)
                    puts(extp->classp()->use_Virtual_عوامی() ? "virtual public " : "public ");
                    putns(extp, EmitCUtil::سابقہ_Name_Protect(extp->classp()));
                    needComma = سچ;
                }
            } ورنہ {
                puts("public virtual VlClass");
            }
        } ورنہ {
            puts(" final");
        }
        puts(" {\n");
        ofp()->reset_نجی();
        ofp()->puts_نجی(جھوٹ);  // public:

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
        emitFuncDecls(modp, /* inClassBody: */ جھوٹ);
        emitSystemCSection(modp, VSystemCSectionType::HDR_POST);
    }

    // VISITORS
    خالی دورہ(Ast_گرہ* nodep) override { nodep->v3fatalSrc("Unused"); }  // LCOV_EXCL_LINE

    صریح EmitCHeader(ثابت Ast_گرہ_Module* modp) {
        UINFO(5, "  Emitting header for " << EmitCUtil::سابقہ_Name_Protect(modp));

        // Open output file
        open_New_Output_سرخی_File(EmitCUtil::سابقہ_Name_Protect(modp), "Design internal header");
        puts("// See " + EmitCUtil::چوٹی_Class_Name() + ".h for the primary calling header\n");

        ofp()->putsGuard();

        // Include files
        puts("\n");
        ofp()->puts_Int_چوٹی_Include();
        puts("#include \"verilated.h\"\n");
        اگر (modp->is_چوٹی() && optSystemC()) puts("#include \"verilated_sc.h\"\n");
        اگر (v3Global.opt.mtasks()) puts("#include \"verilated_threads.h\"\n");
        اگر (v3Global.opt.savable()) puts("#include \"verilated_save.h\"\n");
        اگر (v3Global.opt.coverage()) puts("#include \"verilated_cov.h\"\n");
        اگر (v3Global.opt.coverage() || v3Global.useCovergroup())
            puts("#include \"verilated_covergroup.h\"\n");
        اگر (v3Global.استعمال_کرتا_Timing()) puts("#include \"verilated_timing.h\"\n");
        اگر (v3Global.useRandomizeMethods()) puts("#include \"verilated_random.h\"\n");
        اگر (v3Global.استعمال_کرتا_Force()) puts("#include \"verilated_force.h\"\n");

        std::سیٹ<string> cuse_سیٹ;
        خودکار جوڑو_مقصد_cuse_سیٹ = [&](string s) { cuse_سیٹ.داخل_کرو(s); };

        forModCUse(modp, VUseType::INT_FWD_CLASS | VUseType::INT_INCLUDE, جوڑو_مقصد_cuse_سیٹ);
        اگر (ثابت AstClassPackage* ثابت packagep = VN_CAST(modp, ClassPackage)) {
            forModCUse(packagep->classp(), VUseType::INT_INCLUDE | VUseType::INT_FWD_CLASS,
                       جوڑو_مقصد_cuse_سیٹ);
        }

        برائے (ثابت string& s : cuse_سیٹ) puts(s);
        puts("\n");

        emitAll(modp);

        اگر (ثابت AstClassPackage* ثابت packagep = VN_CAST(modp, ClassPackage)) {
            // Put the non-static class implementation in same h file for speed
            emitAll(packagep->classp());
        }

        ofp()->puts_انتہا_Guard();

        // Close output file
        closeOutputFile();
    }
    ~EmitCHeader() override = طےشدہ;

عوامی:
    ساکن خالی مین(ثابت Ast_گرہ_Module* modp) { ثابت EmitCHeader emitCHeader{modp}; }
};

//######################################################################
// EmitC class functions

خالی V3EmitC::emitc_سرخیاں() {
    UINFO(2, __FUNCTION__ << ":");

    // Process each module in turn
    برائے (ثابت Ast_گرہ* nodep = v3Global.rootp()->modulesp(); nodep; nodep = nodep->nextp()) {
        اگر (VN_IS(nodep, Class)) جاری;  // Declared with the ClassPackage
        ثابت Ast_گرہ_Module* ثابت modp = VN_AS(nodep, گرہ_Module);
        اگر (modp->isConstPool()) جاری;  // Emitted by V3EmitCConstPool
        EmitCHeader::مین(modp);
    }
}
