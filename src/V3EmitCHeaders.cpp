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

#অন্তর্ভাব "V3PchAstMT.h"

#অন্তর্ভাব "V3EmitC.h"
#অন্তর্ভাব "V3EmitCBase.h"
#অন্তর্ভাব "V3File.h"
#অন্তর্ভাব "V3MemberMap.h"
#অন্তর্ভাব "V3UniqueNames.h"

#অন্তর্ভাব <algorithm>
#অন্তর্ভাব <cstdint>
#অন্তর্ভাব <set>
#অন্তর্ভাব <string>
#অন্তর্ভাব <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Internal EmitC implementation

শ্রেণী EmitCHeader final : সর্বজনীন EmitCBaseVisitorConst {
    V3UniqueNames m_names;
    VMemberMap m_memberMap;
    // METHODS

    শ্রেণী Cover_গুনতি_Visitor final : সর্বজনীন VNVisitorConst {
        পূর্ণসংখ্যা m_bins = 0;  // Running total of coverage bins counted so far

        শূন্য visit(Ast_গ্রন্থি_Cover_Decl* nodep) override {
            // Each module class owns the counters for declarations it emits;
            // duplicate no-inline instances need separate arrays for
            // forcePerInstance reporting.
            যদি (!nodep->তথ্য_Decl_Nullp()) m_bins += nodep->আকার_2();
        }
        শূন্য visit(Ast_গ্রন্থি* nodep) override { iterateChildrenConst(nodep); }

    সর্বজনীন:
        explicit Cover_গুনতি_Visitor(ধ্রুবক Ast_গ্রন্থি_Module* modp) {
            iterateConst(const_cast<Ast_গ্রন্থি_Module*>(modp));
        }
        পূর্ণসংখ্যা bins() ধ্রুবক { ফেরত m_bins; }
    };

    শূন্য decorateFirst(যৌক্তিক& first, ধ্রুবক string& str) {
        যদি (first) {
            putsDecoration(nullptr, str);
            first = মিথ্যা;
        }
    }
    শূন্য emitCellDecls(ধ্রুবক Ast_গ্রন্থি_Module* modp) {
        যৌক্তিক first = সত্য;
        জন্য (ধ্রুবক Ast_গ্রন্থি* nodep = modp->stmtsp(); nodep; nodep = nodep->nextp()) {
            যদি (ধ্রুবক AstCell* ধ্রুবক cellp = VN_CAST(nodep, Cell)) {
                যদি (cellp->modp()->isConstPool()) চালু;  // Special emit rules
                decorateFirst(first, "// CELLS\n");
                putns(cellp, EmitCUtil::prefixNameProtect(cellp->modp()) + "* "
                                 + cellp->nameProtect() + ";\n");
            }
        }
    }
    শূন্য emitDesignVarDecls(ধ্রুবক Ast_গ্রন্থি_Module* modp) {
        // V3VariableOrder puts all variables at the front of the list: top level ports, then
        // trivially constructible variables, then the not trivially constructible ones, then
        // those that are not model state
        ধ্রুবক AstVar* portp = nullptr;
        ধ্রুবক AstVar* trivp = nullptr;
        ধ্রুবক AstVar* nontp = nullptr;
        আকার_t nPort = 0;
        আকার_t nTriv = 0;
        আকার_t nNont = 0;
        জন্য (ধ্রুবক Ast_গ্রন্থি* nodep = modp->stmtsp(); nodep; nodep = nodep->nextp()) {
            ধ্রুবক AstVar* ধ্রুবক varp = VN_CAST(nodep, Var);
            যদি (!varp) বিরতি;
            যদি (!varp->isModelState()) {
                // Cache line alignment is only effective on emitted fields
                UASSERT_OBJ(!varp->mtask_Cache_সারি_Align(), varp, "Aligned non-field");
            } অন্যথা যদি (varp->isPrimaryIO()) {
                যদি (!portp) portp = varp;
                ++nPort;
            } অন্যথা যদি (varp->isTriviallyConstructible()) {
                যদি (!trivp) trivp = varp;
                ++nTriv;
            } অন্যথা {
                যদি (!nontp) nontp = varp;
                ++nNont;
            }
        }

        ধ্রুবক auto emitAndStep = [&](ধ্রুবক AstVar*& varpr) {
            emitVarDecl(varpr);
            varpr = VN_CAST(varpr->nextp(), Var);
        };

        // Emit primary ports
        যদি (nPort) {
            putsDecoration(nullptr, "\n// TOP LEVEL PORTS\n");
            // In anonymous union to prevent the C++ compiler from generating
            // constructor/destructor calls
            puts("union {\nstruct {\n");
            জন্য (আকার_t i = 0; i < nPort; ++i) emitAndStep(portp);
            puts("} __VprimaryPorts;\n};\n");
        }

        // Emit trivially constructible variables
        যদি (nTriv) {
            putsDecoration(nullptr, "\n// TRIVIALLY CONSTRUCTIBLE STATE\n");
            যদি (v3Global.opt.compLimitMembers() == 0) {
                // Anonymous structs disabled, so just emit them directly
                জন্য (আকার_t i = 0; i < nTriv; ++i) emitAndStep(trivp);
            } অন্যথা {
                ধ্রুবক আকার_t lim = v3Global.opt.compLimitMembers();
                আকার_t anonL3s = 1;
                আকার_t anonL2s = 1;
                আকার_t anonL1s = 1;
                যদি (nTriv > (lim * lim * lim)) {
                    anonL3s = (nTriv + (lim * lim * lim) - 1) / (lim * lim * lim);
                    anonL2s = lim;
                    anonL1s = lim;
                } অন্যথা যদি (nTriv > (lim * lim)) {
                    anonL2s = (nTriv + (lim * lim) - 1) / (lim * lim);
                    anonL1s = lim;
                } অন্যথা যদি (nTriv > lim) {
                    anonL1s = (nTriv + lim - 1) / lim;
                }
                যদি (anonL1s != 1) {
                    putsDecoration(
                        nullptr,
                        "// Anonymous structures to workaround compiler member-count bugs\n");
                }
                আকার_t nEmitted = 0;
                জন্য (আকার_t l3 = 0; l3 < anonL3s && nEmitted < nTriv; ++l3) {
                    যদি (anonL3s != 1) puts("struct {\n");
                    জন্য (আকার_t l2 = 0; l2 < anonL2s && nEmitted < nTriv; ++l2) {
                        যদি (anonL2s != 1) puts("struct {\n");
                        জন্য (আকার_t l1 = 0; l1 < anonL1s && nEmitted < nTriv; ++l1) {
                            যদি (anonL1s != 1) puts("struct {\n");
                            জন্য (আকার_t l0 = 0; l0 < lim && nEmitted < nTriv; ++l0, ++nEmitted) {
                                emitAndStep(trivp);
                            }
                            যদি (anonL1s != 1) puts("};\n");
                        }
                        যদি (anonL2s != 1) puts("};\n");
                    }
                    যদি (anonL3s != 1) puts("};\n");
                }
                UASSERT_OBJ(nEmitted == nTriv, modp, "Not all variables emitted");
            }
        }

        // Emit not trivially constructible variables
        যদি (nNont) {
            putsDecoration(nullptr, "\n// NOT TRIVIALLY CONSTRUCTIBLE STATE\n");
            // Classes use real constructors
            যদি (VN_IS(modp, Class)) {
                জন্য (আকার_t i = 0; i < nNont; ++i) emitAndStep(nontp);
                ফেরত;
            }
            // Output under an anonymous union to prevent the C++ compiler from generating
            // constructor/destructor calls for them. They are managed explicitly in ctor/dtor.
            // While 'union { singleVar; }' for each variable would be simpler, it blows up
            // clang's compile time when using pre-compiled headers, joy.
            puts("union {\nstruct {\n");
            জন্য (আকার_t i = 0; i < nNont; ++i) emitAndStep(nontp);
            puts("} __Vnontrivs;\n};\n");
        }
    }
    শূন্য emitInternalVarDecls(ধ্রুবক Ast_গ্রন্থি_Module* modp) {
        যদি (ধ্রুবক AstClass* ধ্রুবক classp = VN_CAST(modp, Class)) {
            যদি (classp->needRNG()) {
                putsDecoration(nullptr, "\n// INTERNAL VARIABLES\n");
                puts("VlRNGReseeds __Vm_rng;\n");
            }
        } অন্যথা {  // not class
            putsDecoration(nullptr, "\n// INTERNAL VARIABLES\n");
            puts(EmitCUtil::symClassName() + "* vlSymsp;\n");
            puts("const char* vlNamep;\n");
            // Allocate object-local coverage counters only on modules that
            // contain emitted coverage declarations.
            ধ্রুবক পূর্ণসংখ্যা coverBins = Cover_গুনতি_Visitor{modp}.bins();
            যদি (coverBins) {
                puts(v3Global.opt.threads() > 1 ? "std::atomic<uint32_t>" : "uint32_t");
                puts(" __Vcoverage[");
                puts(std::to_string(coverBins));
                puts("]{};\n");
            }
        }
    }
    শূন্য emitParamDecls(ধ্রুবক Ast_গ্রন্থি_Module* modp) {
        যৌক্তিক first = সত্য;
        জন্য (Ast_গ্রন্থি* nodep = modp->stmtsp(); nodep; nodep = nodep->nextp()) {
            যদি (ধ্রুবক AstVar* ধ্রুবক varp = VN_CAST(nodep, Var)) {
                যদি (varp->isParam()) {
                    decorateFirst(first, "\n// PARAMETERS\n");
                    putns(varp, "static const ");
                    puts(varp->dtypep()->cType(varp->nameProtect(), মিথ্যা, মিথ্যা));
                    puts(";\n");
                }
            }
        }
    }
    শূন্য emitCtorDtorDecls(ধ্রুবক Ast_গ্রন্থি_Module* modp) {
        // Classes use CFuncs with isConstructor/isDestructor
        যদি (VN_IS(modp, Class)) ফেরত;

        // Proper modules use loose split ctor/dtor and are not copyable/movable
        ধ্রুবক std::string name = EmitCUtil::prefixNameProtect(modp);
        putsDecoration(nullptr, "\n// CONSTRUCTORS\n");
        putns(modp, name + "(" + EmitCUtil::symClassName() + "* symsp, const char* namep);\n");
        putns(modp, "~" + name + "();\n");
        putns(modp, "VL_UNCOPYABLE(" + name + ");\n");
        putns(modp, "VL_UNMOVABLE(" + name + ");\n");
    }
    শূন্য emitInternalMethodDecls(ধ্রুবক Ast_গ্রন্থি_Module* modp) {
        যৌক্তিক first = সত্য;
        ধ্রুবক string section = "\n// INTERNAL METHODS\n";

        যদি (!VN_IS(modp, Class)) {
            decorateFirst(first, section);
            যদি (v3Global.opt.coverage()) {
                puts("void " + protect("__Vconfigure") + "(bool first);\n");
            }
        } অন্যথা {
            decorateFirst(first, section);
            ধ্রুবক std::string name = V3OutFormatter::quoteNameControls(
                VIdProtect::protect_শব্দগুলি_If(modp->prettyName(), v3Global.opt.protectIds()));
            // "override", but don't want clang to start checking them:
            puts("virtual const char* typeName() const { return \""s + name + "\"; }\n");
        }

        যদি (v3Global.opt.coverage() && !VN_IS(modp, Class)) {
            decorateFirst(first, section);
            puts("void __vlCoverInsert(");
            puts(v3Global.opt.threads() > 1 ? "std::atomic<uint32_t>" : "uint32_t");
            puts("* countp, bool enable, bool localCounter, const char* filenamep, int lineno, "
                 "int column,\n");
            puts("const char* hierp, const char* pagep, const char* commentp, const char* "
                 "linescovp,\n");
            puts("const char* fsmVarp, const char* fsmFromp, const char* fsmTop, const char* "
                 "fsmTagp);\n");
        }

        যদি (v3Global.opt.coverageToggle() && !VN_IS(modp, Class)) {
            decorateFirst(first, section);
            puts("void __vlCoverToggleInsert(int begin, int end, bool ranged, ");
            puts(v3Global.opt.threads() > 1 ? "std::atomic<uint32_t>" : "uint32_t");
            puts("* countp, bool enable, bool localCounter, const char* filenamep, int lineno, "
                 "int column,\n");
            puts("const char* hierp, const char* pagep, const char* commentp);\n");
        }

        যদি (v3Global.opt.savable()) {
            decorateFirst(first, section);
            puts("void " + protect("__Vserialize") + "(VerilatedSerialize& os);\n");
            puts("void " + protect("__Vdeserialize") + "(VerilatedDeserialize& os);\n");
        }

        // Polymorphic clone for concrete (non-abstract, non-interface) classes
        যদি (ধ্রুবক AstClass* ধ্রুবক classp = VN_CAST(modp, Class)) {
            যদি (!classp->isInterfaceClass() && !classp->isVirtual()) {
                decorateFirst(first, section);
                using EmbeddedCovergroupVar = std::pair<ধ্রুবক AstClass*, ধ্রুবক AstVar*>;
                std::vector<EmbeddedCovergroupVar> embeddedCovergroupVars;
                ধ্রুবক auto hasEnclosingBackPointer = [](ধ্রুবক AstClass* covergroupp) {
                    ফেরত covergroupp->exists([](ধ্রুবক AstVar* ধ্রুবক varp) {
                        ধ্রুবক AstClassRefDType* ধ্রুবক refp
                            = VN_CAST(varp->dtypep()->skipRefp(), ClassRefDType);
                        ফেরত refp && refp->rawPointer();
                    });
                };
                const_cast<AstClass*>(classp)->foreachMember(
                    [&](AstClass* ধ্রুবক memberClassp, AstVar* ধ্রুবক varp) {
                        ধ্রুবক AstClassRefDType* ধ্রুবক refp
                            = VN_CAST(varp->dtypep()->skipRefp(), ClassRefDType);
                        যদি (refp && refp->classp()->isCovergroup()
                            && hasEnclosingBackPointer(refp->classp())) {
                            embeddedCovergroupVars.emplace_back(memberClassp, varp);
                        }
                    });
                ধ্রুবক string className = EmitCUtil::prefixNameProtect(classp);
                যদি (embeddedCovergroupVars.empty() && !classp->hasRandVarsUpdate()) {
                    putns(classp,
                          "VlClass* clone() const { return new " + className + "(*this); }\n");
                } অন্যথা {
                    putns(classp, "VlClass* clone() const { " + className + "* const clonep = new "
                                      + className + "(*this); ");
                    যদি (classp->hasRandVarsUpdate()) {
                        ধ্রুবক string updateName = "__VnoInFunc___VupdateRandVars";
                        AstCFunc* ধ্রুবক updatep
                            = VN_AS(m_memberMap.findMember(classp, updateName), CFunc);
                        UASSERT_OBJ(updatep, classp, "Missing updateRandVars method");
                        puts("clonep->" + updatep->nameProtect() + "();\n");
                    }
                    জন্য (ধ্রুবক EmbeddedCovergroupVar& item : embeddedCovergroupVars) {
                        puts("clonep->" + EmitCUtil::prefixNameProtect(item.first)
                             + "::" + item.second->nameProtect() + " = VlNull{}; ");
                    }
                    puts("return clonep; }\n");
                }
            }
        }
    }
    শূন্য emitEnums(ধ্রুবক Ast_গ্রন্থি_Module* modp) {
        যৌক্তিক first = সত্য;
        জন্য (ধ্রুবক Ast_গ্রন্থি* nodep = modp->stmtsp(); nodep; nodep = nodep->nextp()) {
            ধ্রুবক AstTypedef* ধ্রুবক tdefp = VN_CAST(nodep, Typedef);
            যদি (!tdefp) চালু;
            যদি (!tdefp->attrPublic()) চালু;
            ধ্রুবক AstEnumDType* ধ্রুবক edtypep
                = VN_CAST(tdefp->dtypep()->skipRefToEnump(), EnumDType);
            যদি (!edtypep) চালু;
            decorateFirst(first, "\n// ENUMS (that were declared public)\n");
            যদি (edtypep->width() > 64) {
                putsDecoration(tdefp,
                               "// enum " + tdefp->nameProtect() + " ignored: Too wide for C++\n");
            } অন্যথা {
                putns(tdefp, "enum " + tdefp->name() + " {\n");
                জন্য (ধ্রুবক AstEnumItem* itemp = edtypep->itemsp(); itemp;
                     itemp = VN_AS(itemp->nextp(), EnumItem)) {
                    যদি (ধ্রুবক AstConst* ধ্রুবক constp = VN_CAST(itemp->valuep(), Const)) {
                        যদি (constp->অঙ্ক().isFourState()) {
                            putns(itemp, "// " + itemp->nameProtect() + " is four-state\n");
                            চালু;
                        }
                    }
                    putns(itemp, itemp->nameProtect());
                    puts(" = ");
                    putns(itemp, VN_AS(itemp->valuep(), Const)->অঙ্ক().emitC());
                    যদি (VN_IS(itemp->nextp(), EnumItem)) puts(",");
                    puts("\n");
                }
                puts("};\n");
            }
        }
    }
    শূন্য emitStructDecl(ধ্রুবক Ast_গ্রন্থি_Module* modp, Ast_গ্রন্থি_UOr_Struct_DType* sdtypep,
                        std::set<Ast_গ্রন্থি_UOr_Struct_DType*>& emitted) {
        যদি (emitted.গুনতি(sdtypep) > 0) ফেরত;
        emitted.ঢোকাও(sdtypep);
        জন্য (AstMemberDType* itemp = sdtypep->membersp(); itemp;
             itemp = VN_AS(itemp->nextp(), MemberDType)) {
            Ast_গ্রন্থি_UOr_Struct_DType* ধ্রুবক subp = itemp->getChildStructp();
            যদি (subp && (!subp->packed() || sdtypep->packed())) {
                // Recurse if it belongs to the current module
                যদি (subp->classOrPackagep() == modp) {
                    emitStructDecl(modp, subp, emitted);
                    puts("\n");
                }
            }
        }
        যদি (sdtypep->packed()) {
            emitPackedUOrSBody(sdtypep);
        } অন্যথা {
            emitUnpackedUOrSBody(sdtypep);
        }
    }
    গণনা শ্রেণী AttributeType : uint8_t { WIDTH, DIMENSION };
    // Get member attribute based on type
    পূর্ণসংখ্যা get_গ্রন্থি_Attribute(ধ্রুবক AstMemberDType* itemp, AttributeType type) {
        ধ্রুবক যৌক্তিক isArrayType = itemp->dtypep()->isNonPackedArray();
        নির্বাচন (type) {
        ক্ষেত্র AttributeType::WIDTH: {
            যদি (isArrayType) {
                // For arrays, get innermost element width
                ধ্রুবক Ast_গ্রন্থি_DType* dtype = itemp->dtypep();
                যখন (dtype->subDTypep()) dtype = dtype->subDTypep();
                ফেরত dtype->width();
            }
            ফেরত itemp->width();
        }
        ক্ষেত্র AttributeType::DIMENSION: {
            // Return array dimension or 0 for non-arrays
            ফেরত isArrayType ? itemp->dtypep()->dimensions(সত্য).second : 0;
        }
        পূর্বনির্ধারিত: {
            ফেরত 0;
        }
        }
    }
    template <AttributeType T>
    শূন্য emitMemberVector(ধ্রুবক Ast_গ্রন্থি_UOr_Struct_DType* sdtypep) {
        puts("return {");
        যৌক্তিক needComma = মিথ্যা;
        জন্য (ধ্রুবক AstMemberDType* itemp = sdtypep->membersp(); itemp;
             itemp = VN_AS(itemp->nextp(), MemberDType)) {
            যদি (!itemp->isConstrainedRand()) চালু;
            // Comma handling: add before element except first
            যদি (needComma) puts(",\n");
            putns(itemp, std::to_string(get_গ্রন্থি_Attribute(itemp, T)));
            needComma = সত্য;
        }
        puts("};\n}\n");
    }
    শূন্য emitUnpackedUOrSBody(Ast_গ্রন্থি_UOr_Struct_DType* sdtypep) {
        putns(sdtypep, sdtypep->verilogKwd());  // "struct"/"union"
        puts(" " + EmitCUtil::prefixNameProtect(sdtypep) + " {\n");
        জন্য (ধ্রুবক AstMemberDType* itemp = sdtypep->membersp(); itemp;
             itemp = VN_AS(itemp->nextp(), MemberDType)) {
            putns(itemp, itemp->dtypep()->cType(itemp->nameProtect(), মিথ্যা, মিথ্যা));
            puts(";\n");
        }

        // Three helper functions for struct constrained randomization:
        // - memberNames: Get member names
        // - getMembers: Access member references
        // - memberIndices: Retrieve member indices
        // - memberWidth: Retrieve member width
        // - memberDimension: Retrieve member dimension
        যদি (sdtypep->isConstrainedRand()) {
            যৌক্তিক needComma = মিথ্যা;
            putns(sdtypep, "\nstd::vector<std::string> memberNames(void) const {\n");
            puts("return {");
            জন্য (ধ্রুবক AstMemberDType* itemp = sdtypep->membersp(); itemp;
                 itemp = VN_AS(itemp->nextp(), MemberDType)) {
                যদি (!itemp->isConstrainedRand()) চালু;
                যদি (needComma) puts(",\n");
                putns(itemp, "\"" + itemp->shortName() + "\"");
                needComma = সত্য;
            }
            puts("};\n}\n");

            putns(sdtypep, "\nstd::vector<int> memberWidth(void) const {\n");
            emitMemberVector<AttributeType::WIDTH>(sdtypep);

            putns(sdtypep, "\nstd::vector<int> memberDimension(void) const {\n");
            emitMemberVector<AttributeType::DIMENSION>(sdtypep);

            needComma = মিথ্যা;
            putns(sdtypep, "\nauto memberIndices(void) const {\n");
            puts("return std::index_sequence_for<");
            জন্য (ধ্রুবক AstMemberDType* itemp = sdtypep->membersp(); itemp;
                 itemp = VN_AS(itemp->nextp(), MemberDType)) {
                যদি (!itemp->isConstrainedRand()) চালু;
                যদি (needComma) puts(",\n");
                putns(itemp, itemp->dtypep()->cType("", মিথ্যা, মিথ্যা));
                needComma = সত্য;
            }
            puts(">{};\n}\n");

            needComma = মিথ্যা;
            putns(sdtypep, "\ntemplate <typename T>");
            putns(sdtypep, "\nauto getMembers(T& obj) {\n");
            puts("return std::tie(");
            জন্য (ধ্রুবক AstMemberDType* itemp = sdtypep->membersp(); itemp;
                 itemp = VN_AS(itemp->nextp(), MemberDType)) {
                যদি (!itemp->isConstrainedRand()) চালু;
                যদি (needComma) puts(",\n");
                putns(itemp, "obj." + itemp->nameProtect());
                needComma = সত্য;
            }
            puts(");\n}\n");
        }
        putns(sdtypep, "\nbool operator==(const " + EmitCUtil::prefixNameProtect(sdtypep)
                           + "& rhs) const {\n");
        puts("return ");
        জন্য (ধ্রুবক AstMemberDType* itemp = sdtypep->membersp(); itemp;
             itemp = VN_AS(itemp->nextp(), MemberDType)) {
            যদি (itemp != sdtypep->membersp()) puts("\n    && ");
            putns(itemp, itemp->nameProtect() + " == " + "rhs." + itemp->nameProtect());
        }
        puts(";\n");
        puts("}\n");
        putns(sdtypep, "bool operator!=(const " + EmitCUtil::prefixNameProtect(sdtypep)
                           + "& rhs) const {\n");
        puts("return !(*this == rhs);\n}\n");
        putns(sdtypep, "\nbool operator<(const " + EmitCUtil::prefixNameProtect(sdtypep)
                           + "& rhs) const {\n");
        জন্য (ধ্রুবক AstMemberDType* itemp = sdtypep->membersp(); itemp;
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
        putns(sdtypep, "struct VlIsCustomStruct<" + EmitCUtil::prefixNameProtect(sdtypep)
                           + "> : public std::true_type {};\n");
    }

    // getfunc: VL_ASSIGNSEL_XX(rbits, obits, off, lhsdata, rhsdata);
    // !getfunc: VL_SELASSIGN_XX(rbits, obits, lhsdata, rhsdata, off);
    শূন্য emitVlAssign(ধ্রুবক Ast_গ্রন্থি_DType* ধ্রুবক lhstype, ধ্রুবক Ast_গ্রন্থি_DType* rhstype,
                      ধ্রুবক std::string& off, ধ্রুবক std::string& lhsdata,
                      ধ্রুবক std::string& rhsdata, যৌক্তিক getfunc) {
        puts(getfunc ? "VL_ASSIGNSEL_" : "VL_SELASSIGN_");
        puts(lhstype->charIQWN());
        puts(rhstype->charIQWN());
        puts("(" + std::to_string(lhstype->width()) + ", ");  // LHS width
        যদি (getfunc) {
            puts(std::to_string(rhstype->width()) + ", ");  // Number of copy bits
            puts(off + ", ");  // LHS offset
        } অন্যথা {
            // Number of copy bits. Use widthTototalBytes to
            // make VL_SELASSIGN_XX clear upper unused bits for us.
            // puts(std::to_string(lhstype->width()) + ", ");
            puts(std::to_string(lhstype->width_যোগ_Bytes() * 8) + ", ");
        }
        puts(lhsdata + ", ");  // LHS data
        puts(rhsdata);  // RHS data
        যদি (!getfunc) {
            puts(", " + off);  // RHS offset
        }
        puts(");\n");
    }

    // `retOrArg` should be prefixed by `&` or suffixed by `.data()` depending on its type
    শূন্য emitPackedMember(ধ্রুবক Ast_গ্রন্থি_DType* parentDtypep, ধ্রুবক Ast_গ্রন্থি_DType* dtypep,
                          ধ্রুবক std::string& fieldname, ধ্রুবক std::string& offset, যৌক্তিক getfunc,
                          ধ্রুবক std::string& retOrArg) {
        dtypep = dtypep->skipRefp();
        যদি (ধ্রুবক auto* adtypep = VN_CAST(dtypep, PackArrayDType)) {
            ধ্রুবক std::string সূচক = m_names.get("__Vi");
            puts("for (int " + সূচক + " = 0; " + সূচক + " < "
                 + std::to_string(adtypep->elementsConst()) + "; ++" + সূচক + ") {\n");

            ধ্রুবক std::string offsetInLoop
                = offset + " + " + সূচক + " * " + std::to_string(adtypep->subDTypep()->width());
            ধ্রুবক std::string newName = fieldname + "[" + সূচক + "]";
            emitPackedMember(parentDtypep, adtypep->subDTypep(), newName, offsetInLoop, getfunc,
                             retOrArg);
            puts("}\n");
        } অন্যথা যদি (VN_IS(dtypep, গ্রন্থি_UOr_Struct_DType)) {
            ধ্রুবক std::string tmp = m_names.get("__Vtmp");
            যদি (getfunc) {  // Emit `get` func;
                // auto __tmp = field.get();
                puts("auto " + tmp + " = " + fieldname + ".get();\n");
                // VL_ASSIGNSEL_XX(rbits, obits, lsb, lhsdata, rhsdata);
                emitVlAssign(parentDtypep, dtypep, offset, retOrArg, tmp, getfunc);
            } অন্যথা {  // Emit `set` func
                ধ্রুবক std::string tmptype = AstCDType::typeToHold(dtypep->width());
                // type tmp;
                puts(tmptype + " " + tmp + ";\n");
                // VL_SELASSIGN_XX(rbits, obits, lhsdata, rhsdata, roffset);
                emitVlAssign(dtypep, parentDtypep, offset, tmp, retOrArg, getfunc);
                // field.set(__tmp);
                puts(fieldname + ".set(" + tmp + ");\n");
            }
        } অন্যথা {
            UASSERT_OBJ(VN_IS(dtypep, EnumDType) || VN_IS(dtypep, BasicDType), dtypep,
                        "Unsupported type in packed struct or union");
            যদি (getfunc) {  // Emit `get` func;
                // VL_ASSIGNSEL_XX(rbits, obits, lsb, lhsdata, rhsdata);
                emitVlAssign(parentDtypep, dtypep, offset, retOrArg, fieldname, getfunc);
            } অন্যথা {  // Emit `set` func
                // VL_SELASSIGN_XX(rbits, obits, lhsdata, rhsdata, roffset);
                emitVlAssign(dtypep, parentDtypep, offset, fieldname, retOrArg, getfunc);
            }
        }
    }
    শূন্য emitPackedUOrSBody(Ast_গ্রন্থি_UOr_Struct_DType* sdtypep) {
        putns(sdtypep, sdtypep->verilogKwd());  // "struct"/"union"
        puts(" " + EmitCUtil::prefixNameProtect(sdtypep) + " {\n");

        AstMemberDType* itemp;
        AstMemberDType* lastItemp;
        ধ্রুবক AstMemberDType* witemp = nullptr;
        // LSB is first field in C, so loop backwards
        জন্য (lastItemp = sdtypep->membersp(); lastItemp && lastItemp->nextp();
             lastItemp = VN_AS(lastItemp->nextp(), MemberDType)) {
            যদি (lastItemp->width() == sdtypep->width()) witemp = lastItemp;
        }
        জন্য (itemp = lastItemp; itemp; itemp = VN_CAST(itemp->backp(), MemberDType)) {
            putns(itemp, itemp->dtypep()->cType(itemp->nameProtect(), মিথ্যা, মিথ্যা, সত্য));
            puts(";\n");
        }

        ধ্রুবক std::string retArgName = m_names.get("__v");
        ধ্রুবক std::string retArgType = AstCDType::typeToHold(sdtypep->width());

        // Emit `get` member function
        puts(retArgType + " get() const {\n");
        puts(retArgType + " " + retArgName + ";\n");
        যদি (VN_IS(sdtypep, StructDType)) {
            জন্য (itemp = lastItemp; itemp; itemp = VN_CAST(itemp->backp(), MemberDType)) {
                emitPackedMember(sdtypep, itemp->dtypep(), itemp->nameProtect(),
                                 std::to_string(itemp->lsb()), /*getfunc=*/সত্য, retArgName);
            }
        } অন্যথা {
            // We only need to fill the widest field of union
            emitPackedMember(sdtypep, witemp->dtypep(), witemp->nameProtect(),
                             std::to_string(witemp->lsb()), /*getfunc=*/সত্য, retArgName);
        }
        puts("return " + retArgName + ";\n");
        puts("}\n");

        // Emit `set` member function
        puts("void set(const " + retArgType + "& " + retArgName + ") {\n");
        যদি (VN_IS(sdtypep, StructDType)) {
            জন্য (itemp = lastItemp; itemp; itemp = VN_CAST(itemp->backp(), MemberDType)) {
                emitPackedMember(sdtypep, itemp->dtypep(), itemp->nameProtect(),
                                 std::to_string(itemp->lsb()), /*getfunc=*/মিথ্যা, retArgName);
            }
        } অন্যথা {
            // We only need to fill the widest field of union
            emitPackedMember(sdtypep, witemp->dtypep(), witemp->nameProtect(),
                             std::to_string(witemp->lsb()), /*getfunc=*/মিথ্যা, retArgName);
        }

        puts("}\n");

        puts("};\n");
        m_names.reset();
    }
    শূন্য emitStructs(ধ্রুবক Ast_গ্রন্থি_Module* modp) {
        // Track structs that've been emitted already
        std::set<Ast_গ্রন্থি_UOr_Struct_DType*> emitted;
        জন্য (ধ্রুবক Ast_গ্রন্থি* nodep = modp->stmtsp(); nodep; nodep = nodep->nextp()) {
            ধ্রুবক AstTypedef* ধ্রুবক tdefp = VN_CAST(nodep, Typedef);
            যদি (!tdefp) চালু;
            Ast_গ্রন্থি_UOr_Struct_DType* ধ্রুবক sdtypep
                = VN_CAST(tdefp->dtypep()->skipRefToEnump(), গ্রন্থি_UOr_Struct_DType);
            যদি (!sdtypep) চালু;
            // V3Inline can copy one module into several parents. Only the module that
            // declares the struct emits it, so C++ sees only one definition.
            ধ্রুবক যৌক্তিক declaredInOtherModule = sdtypep->classOrPackagep() != modp;
            যদি (declaredInOtherModule) চালু;
            emitStructDecl(modp, sdtypep, emitted);
        }
    }
    শূন্য emitFuncDecls(ধ্রুবক Ast_গ্রন্থি_Module* modp, যৌক্তিক inClassBody) {
        std::vector<ধ্রুবক AstCFunc*> funcsp;

        জন্য (ধ্রুবক Ast_গ্রন্থি* nodep = modp->stmtsp(); nodep; nodep = nodep->nextp()) {
            যদি (ধ্রুবক AstCFunc* ধ্রুবক funcp = VN_CAST(nodep, CFunc)) {
                যদি (funcp->dpiImportPrototype())  // Declared in __Dpi.h
                    চালু;
                যদি (funcp->dpiExportDispatcher())  // Declared in __Dpi.h
                    চালু;
                যদি (funcp->isMethod() != inClassBody)  // Only methods go inside class
                    চালু;
                যদি (funcp->isMethod() && funcp->isLoose())  // Loose methods are declared lazily
                    চালু;
                funcsp.ঠেলো_back(funcp);
            }
        }

        std::stable_sort(funcsp.begin(), funcsp.end(), [](ধ্রুবক Ast_গ্রন্থি* ap, ধ্রুবক Ast_গ্রন্থি* bp) {
            ফেরত ap->name() < bp->name();
        });

        জন্য (ধ্রুবক AstCFunc* ধ্রুবক funcp : funcsp) {
            যদি (inClassBody) ofp()->putsPrivate(funcp->declPrivate());
            emitCFuncDecl(funcp, modp);
        }
    }
    শূন্য emitAll(ধ্রুবক Ast_গ্রন্থি_Module* modp) {
        // Include files required by this AstNodeModule
        যদি (ধ্রুবক AstClass* ধ্রুবক classp = VN_CAST(modp, Class)) {
            জন্য (ধ্রুবক AstClassExtends* extp = classp->extendsp(); extp;
                 extp = VN_AS(extp->nextp(), ClassExtends)) {
                putns(extp, "#include \""
                                + EmitCUtil::prefixNameProtect(extp->classp()->classOrPackagep())
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
        যদি (!VN_IS(modp, Class)) puts("alignas(VL_CACHE_LINE_BYTES) ");
        puts(EmitCUtil::prefixNameProtect(modp));
        যদি (ধ্রুবক AstClass* ধ্রুবক classp = VN_CAST(modp, Class)) {
            puts(" : ");
            যদি (classp->extendsp()) {
                যৌক্তিক needComma = মিথ্যা;
                জন্য (ধ্রুবক AstClassExtends* extp = classp->extendsp(); extp;
                     extp = VN_AS(extp->nextp(), ClassExtends)) {
                    যদি (needComma) puts(", ");
                    // Use virtual only for interfaces for class inheritance
                    // (extends)
                    puts(extp->classp()->useVirtualPublic() ? "virtual public " : "public ");
                    putns(extp, EmitCUtil::prefixNameProtect(extp->classp()));
                    needComma = সত্য;
                }
            } অন্যথা {
                puts("public virtual VlClass");
            }
        } অন্যথা {
            puts(" final");
        }
        puts(" {\n");
        ofp()->resetPrivate();
        ofp()->putsPrivate(মিথ্যা);  // public:

        // Emit all class body contents
        emitCellDecls(modp);
        emitEnums(modp);
        emitDesignVarDecls(modp);
        emitInternalVarDecls(modp);
        emitParamDecls(modp);
        emitCtorDtorDecls(modp);
        emitInternalMethodDecls(modp);
        emitFuncDecls(modp, /* inClassBody: */ সত্য);

        // From `systemc_interface
        emitSystemCSection(modp, VSystemCSectionType::INT);

        // Close class body
        puts("};\n");
        // }}}

        // Emit out of class function declarations
        puts("\n");
        emitFuncDecls(modp, /* inClassBody: */ মিথ্যা);
        emitSystemCSection(modp, VSystemCSectionType::HDR_POST);
    }

    // VISITORS
    শূন্য visit(Ast_গ্রন্থি* nodep) override { nodep->v3fatalSrc("Unused"); }  // LCOV_EXCL_LINE

    explicit EmitCHeader(ধ্রুবক Ast_গ্রন্থি_Module* modp) {
        UINFO(5, "  Emitting header for " << EmitCUtil::prefixNameProtect(modp));

        // Open output file
        openNewOutputHeaderFile(EmitCUtil::prefixNameProtect(modp), "Design internal header");
        puts("// See " + EmitCUtil::শীর্ষ_Class_Name() + ".h for the primary calling header\n");

        ofp()->putsGuard();

        // Include files
        puts("\n");
        ofp()->puts_Int_শীর্ষ_Include();
        puts("#include \"verilated.h\"\n");
        যদি (modp->is_শীর্ষ() && optSystemC()) puts("#include \"verilated_sc.h\"\n");
        যদি (v3Global.opt.mtasks()) puts("#include \"verilated_threads.h\"\n");
        যদি (v3Global.opt.savable()) puts("#include \"verilated_save.h\"\n");
        যদি (v3Global.opt.coverage()) puts("#include \"verilated_cov.h\"\n");
        যদি (v3Global.opt.coverage() || v3Global.useCovergroup())
            puts("#include \"verilated_covergroup.h\"\n");
        যদি (v3Global.usesTiming()) puts("#include \"verilated_timing.h\"\n");
        যদি (v3Global.useRandomizeMethods()) puts("#include \"verilated_random.h\"\n");
        যদি (v3Global.usesForce()) puts("#include \"verilated_force.h\"\n");

        std::set<string> cuse_set;
        auto add_to_cuse_set = [&](string s) { cuse_set.ঢোকাও(s); };

        forModCUse(modp, VUseType::INT_FWD_CLASS | VUseType::INT_INCLUDE, add_to_cuse_set);
        যদি (ধ্রুবক AstClassPackage* ধ্রুবক packagep = VN_CAST(modp, ClassPackage)) {
            forModCUse(packagep->classp(), VUseType::INT_INCLUDE | VUseType::INT_FWD_CLASS,
                       add_to_cuse_set);
        }

        জন্য (ধ্রুবক string& s : cuse_set) puts(s);
        puts("\n");

        emitAll(modp);

        যদি (ধ্রুবক AstClassPackage* ধ্রুবক packagep = VN_CAST(modp, ClassPackage)) {
            // Put the non-static class implementation in same h file for speed
            emitAll(packagep->classp());
        }

        ofp()->putsEndGuard();

        // Close output file
        closeOutputFile();
    }
    ~EmitCHeader() override = পূর্বনির্ধারিত;

সর্বজনীন:
    স্থির শূন্য প্রধান(ধ্রুবক Ast_গ্রন্থি_Module* modp) { ধ্রুবক EmitCHeader emitCHeader{modp}; }
};

//######################################################################
// EmitC class functions

শূন্য V3EmitC::emitcHeaders() {
    UINFO(2, __FUNCTION__ << ":");

    // Process each module in turn
    জন্য (ধ্রুবক Ast_গ্রন্থি* nodep = v3Global.rootp()->modulesp(); nodep; nodep = nodep->nextp()) {
        যদি (VN_IS(nodep, Class)) চালু;  // Declared with the ClassPackage
        ধ্রুবক Ast_গ্রন্থি_Module* ধ্রুবক modp = VN_AS(nodep, গ্রন্থি_Module);
        যদি (modp->isConstPool()) চালু;  // Emitted by V3EmitCConstPool
        EmitCHeader::প্রধান(modp);
    }
}
