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

বর্গ EmitCHeader final : সার্বজনিক EmitCBaseVisitorConst {
    V3UniqueNames m_names;
    VMemberMap m_memberMap;
    // METHODS

    বর্গ Cover_গুনতি_Visitor final : সার্বজনিক VNVisitorConst {
        পূর্ণাঙ্ক m_bins = 0;  // Running total of coverage bins counted so far

        শূন্য visit(Ast_গ্রন্থি_Cover_Decl* nodep) override {
            // Each module class owns the counters for declarations it emits;
            // duplicate no-inline instances need separate arrays for
            // forcePerInstance reporting.
            যদি (!nodep->dataDeclNullp()) m_bins += nodep->size();
        }
        শূন্য visit(Ast_গ্রন্থি* nodep) override { iterateChildrenConst(nodep); }

    সার্বজনিক:
        explicit Cover_গুনতি_Visitor(ধ্রুব Ast_গ্রন্থি_Module* modp) {
            iterateConst(const_cast<Ast_গ্রন্থি_Module*>(modp));
        }
        পূর্ণাঙ্ক bins() ধ্রুব { প্রত্যাগম m_bins; }
    };

    শূন্য decorateFirst(তার্কিক& first, ধ্রুব string& str) {
        যদি (first) {
            putsDecoration(nullptr, str);
            first = অসত্য;
        }
    }
    শূন্য emitCellDecls(ধ্রুব Ast_গ্রন্থি_Module* modp) {
        তার্কিক first = সত্য;
        প্রতি (ধ্রুব Ast_গ্রন্থি* nodep = modp->stmtsp(); nodep; nodep = nodep->nextp()) {
            যদি (ধ্রুব AstCell* ধ্রুব cellp = VN_CAST(nodep, Cell)) {
                যদি (cellp->modp()->isConstPool()) অনুবর্ত;  // Special emit rules
                decorateFirst(first, "// CELLS\n");
                putns(cellp, EmitCUtil::prefixNameProtect(cellp->modp()) + "* "
                                 + cellp->nameProtect() + ";\n");
            }
        }
    }
    শূন্য emitDesignVarDecls(ধ্রুব Ast_গ্রন্থি_Module* modp) {
        // V3VariableOrder puts all variables at the front of the list: top level ports, then
        // trivially constructible variables, then the not trivially constructible ones, then
        // those that are not model state
        ধ্রুব AstVar* portp = nullptr;
        ধ্রুব AstVar* trivp = nullptr;
        ধ্রুব AstVar* nontp = nullptr;
        size_t nPort = 0;
        size_t nTriv = 0;
        size_t nNont = 0;
        প্রতি (ধ্রুব Ast_গ্রন্থি* nodep = modp->stmtsp(); nodep; nodep = nodep->nextp()) {
            ধ্রুব AstVar* ধ্রুব varp = VN_CAST(nodep, Var);
            যদি (!varp) ভঙ্গ;
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

        ধ্রুব auto emitAndStep = [&](ধ্রুব AstVar*& varpr) {
            emitVarDecl(varpr);
            varpr = VN_CAST(varpr->nextp(), Var);
        };

        // Emit primary ports
        যদি (nPort) {
            putsDecoration(nullptr, "\n// TOP LEVEL PORTS\n");
            // In anonymous union to prevent the C++ compiler from generating
            // constructor/destructor calls
            puts("union {\nstruct {\n");
            প্রতি (size_t i = 0; i < nPort; ++i) emitAndStep(portp);
            puts("} __VprimaryPorts;\n};\n");
        }

        // Emit trivially constructible variables
        যদি (nTriv) {
            putsDecoration(nullptr, "\n// TRIVIALLY CONSTRUCTIBLE STATE\n");
            যদি (v3Global.opt.compLimitMembers() == 0) {
                // Anonymous structs disabled, so just emit them directly
                প্রতি (size_t i = 0; i < nTriv; ++i) emitAndStep(trivp);
            } অন্যথা {
                ধ্রুব size_t lim = v3Global.opt.compLimitMembers();
                size_t anonL3s = 1;
                size_t anonL2s = 1;
                size_t anonL1s = 1;
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
                size_t nEmitted = 0;
                প্রতি (size_t l3 = 0; l3 < anonL3s && nEmitted < nTriv; ++l3) {
                    যদি (anonL3s != 1) puts("struct {\n");
                    প্রতি (size_t l2 = 0; l2 < anonL2s && nEmitted < nTriv; ++l2) {
                        যদি (anonL2s != 1) puts("struct {\n");
                        প্রতি (size_t l1 = 0; l1 < anonL1s && nEmitted < nTriv; ++l1) {
                            যদি (anonL1s != 1) puts("struct {\n");
                            প্রতি (size_t l0 = 0; l0 < lim && nEmitted < nTriv; ++l0, ++nEmitted) {
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
                প্রতি (size_t i = 0; i < nNont; ++i) emitAndStep(nontp);
                প্রত্যাগম;
            }
            // Output under an anonymous union to prevent the C++ compiler from generating
            // constructor/destructor calls for them. They are managed explicitly in ctor/dtor.
            // While 'union { singleVar; }' for each variable would be simpler, it blows up
            // clang's compile time when using pre-compiled headers, joy.
            puts("union {\nstruct {\n");
            প্রতি (size_t i = 0; i < nNont; ++i) emitAndStep(nontp);
            puts("} __Vnontrivs;\n};\n");
        }
    }
    শূন্য emitInternalVarDecls(ধ্রুব Ast_গ্রন্থি_Module* modp) {
        যদি (ধ্রুব AstClass* ধ্রুব classp = VN_CAST(modp, Class)) {
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
            ধ্রুব পূর্ণাঙ্ক coverBins = Cover_গুনতি_Visitor{modp}.bins();
            যদি (coverBins) {
                puts(v3Global.opt.threads() > 1 ? "std::atomic<uint32_t>" : "uint32_t");
                puts(" __Vcoverage[");
                puts(std::to_string(coverBins));
                puts("]{};\n");
            }
        }
    }
    শূন্য emitParamDecls(ধ্রুব Ast_গ্রন্থি_Module* modp) {
        তার্কিক first = সত্য;
        প্রতি (Ast_গ্রন্থি* nodep = modp->stmtsp(); nodep; nodep = nodep->nextp()) {
            যদি (ধ্রুব AstVar* ধ্রুব varp = VN_CAST(nodep, Var)) {
                যদি (varp->isParam()) {
                    decorateFirst(first, "\n// PARAMETERS\n");
                    putns(varp, "static const ");
                    puts(varp->dtypep()->cType(varp->nameProtect(), অসত্য, অসত্য));
                    puts(";\n");
                }
            }
        }
    }
    শূন্য emitCtorDtorDecls(ধ্রুব Ast_গ্রন্থি_Module* modp) {
        // Classes use CFuncs with isConstructor/isDestructor
        যদি (VN_IS(modp, Class)) প্রত্যাগম;

        // Proper modules use loose split ctor/dtor and are not copyable/movable
        ধ্রুব std::string name = EmitCUtil::prefixNameProtect(modp);
        putsDecoration(nullptr, "\n// CONSTRUCTORS\n");
        putns(modp, name + "(" + EmitCUtil::symClassName() + "* symsp, const char* namep);\n");
        putns(modp, "~" + name + "();\n");
        putns(modp, "VL_UNCOPYABLE(" + name + ");\n");
        putns(modp, "VL_UNMOVABLE(" + name + ");\n");
    }
    শূন্য emitInternalMethodDecls(ধ্রুব Ast_গ্রন্থি_Module* modp) {
        তার্কিক first = সত্য;
        ধ্রুব string section = "\n// INTERNAL METHODS\n";

        যদি (!VN_IS(modp, Class)) {
            decorateFirst(first, section);
            যদি (v3Global.opt.coverage()) {
                puts("void " + protect("__Vconfigure") + "(bool first);\n");
            }
        } অন্যথা {
            decorateFirst(first, section);
            ধ্রুব std::string name = V3OutFormatter::quoteNameControls(
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
        যদি (ধ্রুব AstClass* ধ্রুব classp = VN_CAST(modp, Class)) {
            যদি (!classp->isInterfaceClass() && !classp->isVirtual()) {
                decorateFirst(first, section);
                using EmbeddedCovergroupVar = std::pair<ধ্রুব AstClass*, ধ্রুব AstVar*>;
                std::vector<EmbeddedCovergroupVar> embeddedCovergroupVars;
                ধ্রুব auto hasEnclosingBackPointer = [](ধ্রুব AstClass* covergroupp) {
                    প্রত্যাগম covergroupp->exists([](ধ্রুব AstVar* ধ্রুব varp) {
                        ধ্রুব AstClassRefDType* ধ্রুব refp
                            = VN_CAST(varp->dtypep()->skipRefp(), ClassRefDType);
                        প্রত্যাগম refp && refp->rawPointer();
                    });
                };
                const_cast<AstClass*>(classp)->foreachMember(
                    [&](AstClass* ধ্রুব memberClassp, AstVar* ধ্রুব varp) {
                        ধ্রুব AstClassRefDType* ধ্রুব refp
                            = VN_CAST(varp->dtypep()->skipRefp(), ClassRefDType);
                        যদি (refp && refp->classp()->isCovergroup()
                            && hasEnclosingBackPointer(refp->classp())) {
                            embeddedCovergroupVars.emplace_back(memberClassp, varp);
                        }
                    });
                ধ্রুব string className = EmitCUtil::prefixNameProtect(classp);
                যদি (embeddedCovergroupVars.empty() && !classp->hasRandVarsUpdate()) {
                    putns(classp,
                          "VlClass* clone() const { return new " + className + "(*this); }\n");
                } অন্যথা {
                    putns(classp, "VlClass* clone() const { " + className + "* const clonep = new "
                                      + className + "(*this); ");
                    যদি (classp->hasRandVarsUpdate()) {
                        ধ্রুব string updateName = "__VnoInFunc___VupdateRandVars";
                        AstCFunc* ধ্রুব updatep
                            = VN_AS(m_memberMap.findMember(classp, updateName), CFunc);
                        UASSERT_OBJ(updatep, classp, "Missing updateRandVars method");
                        puts("clonep->" + updatep->nameProtect() + "();\n");
                    }
                    প্রতি (ধ্রুব EmbeddedCovergroupVar& item : embeddedCovergroupVars) {
                        puts("clonep->" + EmitCUtil::prefixNameProtect(item.first)
                             + "::" + item.second->nameProtect() + " = VlNull{}; ");
                    }
                    puts("return clonep; }\n");
                }
            }
        }
    }
    শূন্য emitEnums(ধ্রুব Ast_গ্রন্থি_Module* modp) {
        তার্কিক first = সত্য;
        প্রতি (ধ্রুব Ast_গ্রন্থি* nodep = modp->stmtsp(); nodep; nodep = nodep->nextp()) {
            ধ্রুব AstTypedef* ধ্রুব tdefp = VN_CAST(nodep, Typedef);
            যদি (!tdefp) অনুবর্ত;
            যদি (!tdefp->attrPublic()) অনুবর্ত;
            ধ্রুব AstEnumDType* ধ্রুব edtypep
                = VN_CAST(tdefp->dtypep()->skipRefToEnump(), EnumDType);
            যদি (!edtypep) অনুবর্ত;
            decorateFirst(first, "\n// ENUMS (that were declared public)\n");
            যদি (edtypep->width() > 64) {
                putsDecoration(tdefp,
                               "// enum " + tdefp->nameProtect() + " ignored: Too wide for C++\n");
            } অন্যথা {
                putns(tdefp, "enum " + tdefp->name() + " {\n");
                প্রতি (ধ্রুব AstEnumItem* itemp = edtypep->itemsp(); itemp;
                     itemp = VN_AS(itemp->nextp(), EnumItem)) {
                    যদি (ধ্রুব AstConst* ধ্রুব constp = VN_CAST(itemp->valuep(), Const)) {
                        যদি (constp->অঙ্ক().isFourState()) {
                            putns(itemp, "// " + itemp->nameProtect() + " is four-state\n");
                            অনুবর্ত;
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
    শূন্য emitStructDecl(ধ্রুব Ast_গ্রন্থি_Module* modp, Ast_গ্রন্থি_UOr_Struct_DType* sdtypep,
                        std::set<Ast_গ্রন্থি_UOr_Struct_DType*>& emitted) {
        যদি (emitted.গুনতি(sdtypep) > 0) প্রত্যাগম;
        emitted.ঢোকাও(sdtypep);
        প্রতি (AstMemberDType* itemp = sdtypep->membersp(); itemp;
             itemp = VN_AS(itemp->nextp(), MemberDType)) {
            Ast_গ্রন্থি_UOr_Struct_DType* ধ্রুব subp = itemp->getChildStructp();
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
    গণনা বর্গ AttributeType : uint8_t { WIDTH, DIMENSION };
    // Get member attribute based on type
    পূর্ণাঙ্ক get_গ্রন্থি_Attribute(ধ্রুব AstMemberDType* itemp, AttributeType type) {
        ধ্রুব তার্কিক isArrayType = itemp->dtypep()->isNonPackedArray();
        বিকল্প (type) {
        পক্ষ AttributeType::WIDTH: {
            যদি (isArrayType) {
                // For arrays, get innermost element width
                ধ্রুব Ast_গ্রন্থি_DType* dtype = itemp->dtypep();
                যাবত্ (dtype->subDTypep()) dtype = dtype->subDTypep();
                প্রত্যাগম dtype->width();
            }
            প্রত্যাগম itemp->width();
        }
        পক্ষ AttributeType::DIMENSION: {
            // Return array dimension or 0 for non-arrays
            প্রত্যাগম isArrayType ? itemp->dtypep()->dimensions(সত্য).second : 0;
        }
        স্বভাব: {
            প্রত্যাগম 0;
        }
        }
    }
    template <AttributeType T>
    শূন্য emitMemberVector(ধ্রুব Ast_গ্রন্থি_UOr_Struct_DType* sdtypep) {
        puts("return {");
        তার্কিক needComma = অসত্য;
        প্রতি (ধ্রুব AstMemberDType* itemp = sdtypep->membersp(); itemp;
             itemp = VN_AS(itemp->nextp(), MemberDType)) {
            যদি (!itemp->isConstrainedRand()) অনুবর্ত;
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
        প্রতি (ধ্রুব AstMemberDType* itemp = sdtypep->membersp(); itemp;
             itemp = VN_AS(itemp->nextp(), MemberDType)) {
            putns(itemp, itemp->dtypep()->cType(itemp->nameProtect(), অসত্য, অসত্য));
            puts(";\n");
        }

        // Three helper functions for struct constrained randomization:
        // - memberNames: Get member names
        // - getMembers: Access member references
        // - memberIndices: Retrieve member indices
        // - memberWidth: Retrieve member width
        // - memberDimension: Retrieve member dimension
        যদি (sdtypep->isConstrainedRand()) {
            তার্কিক needComma = অসত্য;
            putns(sdtypep, "\nstd::vector<std::string> memberNames(void) const {\n");
            puts("return {");
            প্রতি (ধ্রুব AstMemberDType* itemp = sdtypep->membersp(); itemp;
                 itemp = VN_AS(itemp->nextp(), MemberDType)) {
                যদি (!itemp->isConstrainedRand()) অনুবর্ত;
                যদি (needComma) puts(",\n");
                putns(itemp, "\"" + itemp->shortName() + "\"");
                needComma = সত্য;
            }
            puts("};\n}\n");

            putns(sdtypep, "\nstd::vector<int> memberWidth(void) const {\n");
            emitMemberVector<AttributeType::WIDTH>(sdtypep);

            putns(sdtypep, "\nstd::vector<int> memberDimension(void) const {\n");
            emitMemberVector<AttributeType::DIMENSION>(sdtypep);

            needComma = অসত্য;
            putns(sdtypep, "\nauto memberIndices(void) const {\n");
            puts("return std::index_sequence_for<");
            প্রতি (ধ্রুব AstMemberDType* itemp = sdtypep->membersp(); itemp;
                 itemp = VN_AS(itemp->nextp(), MemberDType)) {
                যদি (!itemp->isConstrainedRand()) অনুবর্ত;
                যদি (needComma) puts(",\n");
                putns(itemp, itemp->dtypep()->cType("", অসত্য, অসত্য));
                needComma = সত্য;
            }
            puts(">{};\n}\n");

            needComma = অসত্য;
            putns(sdtypep, "\ntemplate <typename T>");
            putns(sdtypep, "\nauto getMembers(T& obj) {\n");
            puts("return std::tie(");
            প্রতি (ধ্রুব AstMemberDType* itemp = sdtypep->membersp(); itemp;
                 itemp = VN_AS(itemp->nextp(), MemberDType)) {
                যদি (!itemp->isConstrainedRand()) অনুবর্ত;
                যদি (needComma) puts(",\n");
                putns(itemp, "obj." + itemp->nameProtect());
                needComma = সত্য;
            }
            puts(");\n}\n");
        }
        putns(sdtypep, "\nbool operator==(const " + EmitCUtil::prefixNameProtect(sdtypep)
                           + "& rhs) const {\n");
        puts("return ");
        প্রতি (ধ্রুব AstMemberDType* itemp = sdtypep->membersp(); itemp;
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
        প্রতি (ধ্রুব AstMemberDType* itemp = sdtypep->membersp(); itemp;
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
    শূন্য emitVlAssign(ধ্রুব Ast_গ্রন্থি_DType* ধ্রুব lhstype, ধ্রুব Ast_গ্রন্থি_DType* rhstype,
                      ধ্রুব std::string& off, ধ্রুব std::string& lhsdata,
                      ধ্রুব std::string& rhsdata, তার্কিক getfunc) {
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
            puts(std::to_string(lhstype->widthTotalBytes() * 8) + ", ");
        }
        puts(lhsdata + ", ");  // LHS data
        puts(rhsdata);  // RHS data
        যদি (!getfunc) {
            puts(", " + off);  // RHS offset
        }
        puts(");\n");
    }

    // `retOrArg` should be prefixed by `&` or suffixed by `.data()` depending on its type
    শূন্য emitPackedMember(ধ্রুব Ast_গ্রন্থি_DType* parentDtypep, ধ্রুব Ast_গ্রন্থি_DType* dtypep,
                          ধ্রুব std::string& fieldname, ধ্রুব std::string& offset, তার্কিক getfunc,
                          ধ্রুব std::string& retOrArg) {
        dtypep = dtypep->skipRefp();
        যদি (ধ্রুব auto* adtypep = VN_CAST(dtypep, PackArrayDType)) {
            ধ্রুব std::string index = m_names.get("__Vi");
            puts("for (int " + index + " = 0; " + index + " < "
                 + std::to_string(adtypep->elementsConst()) + "; ++" + index + ") {\n");

            ধ্রুব std::string offsetInLoop
                = offset + " + " + index + " * " + std::to_string(adtypep->subDTypep()->width());
            ধ্রুব std::string newName = fieldname + "[" + index + "]";
            emitPackedMember(parentDtypep, adtypep->subDTypep(), newName, offsetInLoop, getfunc,
                             retOrArg);
            puts("}\n");
        } অন্যথা যদি (VN_IS(dtypep, গ্রন্থি_UOr_Struct_DType)) {
            ধ্রুব std::string tmp = m_names.get("__Vtmp");
            যদি (getfunc) {  // Emit `get` func;
                // auto __tmp = field.get();
                puts("auto " + tmp + " = " + fieldname + ".get();\n");
                // VL_ASSIGNSEL_XX(rbits, obits, lsb, lhsdata, rhsdata);
                emitVlAssign(parentDtypep, dtypep, offset, retOrArg, tmp, getfunc);
            } অন্যথা {  // Emit `set` func
                ধ্রুব std::string tmptype = AstCDType::typeToHold(dtypep->width());
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
        ধ্রুব AstMemberDType* witemp = nullptr;
        // LSB is first field in C, so loop backwards
        প্রতি (lastItemp = sdtypep->membersp(); lastItemp && lastItemp->nextp();
             lastItemp = VN_AS(lastItemp->nextp(), MemberDType)) {
            যদি (lastItemp->width() == sdtypep->width()) witemp = lastItemp;
        }
        প্রতি (itemp = lastItemp; itemp; itemp = VN_CAST(itemp->backp(), MemberDType)) {
            putns(itemp, itemp->dtypep()->cType(itemp->nameProtect(), অসত্য, অসত্য, সত্য));
            puts(";\n");
        }

        ধ্রুব std::string retArgName = m_names.get("__v");
        ধ্রুব std::string retArgType = AstCDType::typeToHold(sdtypep->width());

        // Emit `get` member function
        puts(retArgType + " get() const {\n");
        puts(retArgType + " " + retArgName + ";\n");
        যদি (VN_IS(sdtypep, StructDType)) {
            প্রতি (itemp = lastItemp; itemp; itemp = VN_CAST(itemp->backp(), MemberDType)) {
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
            প্রতি (itemp = lastItemp; itemp; itemp = VN_CAST(itemp->backp(), MemberDType)) {
                emitPackedMember(sdtypep, itemp->dtypep(), itemp->nameProtect(),
                                 std::to_string(itemp->lsb()), /*getfunc=*/অসত্য, retArgName);
            }
        } অন্যথা {
            // We only need to fill the widest field of union
            emitPackedMember(sdtypep, witemp->dtypep(), witemp->nameProtect(),
                             std::to_string(witemp->lsb()), /*getfunc=*/অসত্য, retArgName);
        }

        puts("}\n");

        puts("};\n");
        m_names.reset();
    }
    শূন্য emitStructs(ধ্রুব Ast_গ্রন্থি_Module* modp) {
        // Track structs that've been emitted already
        std::set<Ast_গ্রন্থি_UOr_Struct_DType*> emitted;
        প্রতি (ধ্রুব Ast_গ্রন্থি* nodep = modp->stmtsp(); nodep; nodep = nodep->nextp()) {
            ধ্রুব AstTypedef* ধ্রুব tdefp = VN_CAST(nodep, Typedef);
            যদি (!tdefp) অনুবর্ত;
            Ast_গ্রন্থি_UOr_Struct_DType* ধ্রুব sdtypep
                = VN_CAST(tdefp->dtypep()->skipRefToEnump(), গ্রন্থি_UOr_Struct_DType);
            যদি (!sdtypep) অনুবর্ত;
            // V3Inline can copy one module into several parents. Only the module that
            // declares the struct emits it, so C++ sees only one definition.
            ধ্রুব তার্কিক declaredInOtherModule = sdtypep->classOrPackagep() != modp;
            যদি (declaredInOtherModule) অনুবর্ত;
            emitStructDecl(modp, sdtypep, emitted);
        }
    }
    শূন্য emitFuncDecls(ধ্রুব Ast_গ্রন্থি_Module* modp, তার্কিক inClassBody) {
        std::vector<ধ্রুব AstCFunc*> funcsp;

        প্রতি (ধ্রুব Ast_গ্রন্থি* nodep = modp->stmtsp(); nodep; nodep = nodep->nextp()) {
            যদি (ধ্রুব AstCFunc* ধ্রুব funcp = VN_CAST(nodep, CFunc)) {
                যদি (funcp->dpiImportPrototype())  // Declared in __Dpi.h
                    অনুবর্ত;
                যদি (funcp->dpiExportDispatcher())  // Declared in __Dpi.h
                    অনুবর্ত;
                যদি (funcp->isMethod() != inClassBody)  // Only methods go inside class
                    অনুবর্ত;
                যদি (funcp->isMethod() && funcp->isLoose())  // Loose methods are declared lazily
                    অনুবর্ত;
                funcsp.ঠেলো_back(funcp);
            }
        }

        std::stable_sort(funcsp.begin(), funcsp.end(), [](ধ্রুব Ast_গ্রন্থি* ap, ধ্রুব Ast_গ্রন্থি* bp) {
            প্রত্যাগম ap->name() < bp->name();
        });

        প্রতি (ধ্রুব AstCFunc* ধ্রুব funcp : funcsp) {
            যদি (inClassBody) ofp()->putsPrivate(funcp->declPrivate());
            emitCFuncDecl(funcp, modp);
        }
    }
    শূন্য emitAll(ধ্রুব Ast_গ্রন্থি_Module* modp) {
        // Include files required by this AstNodeModule
        যদি (ধ্রুব AstClass* ধ্রুব classp = VN_CAST(modp, Class)) {
            প্রতি (ধ্রুব AstClassExtends* extp = classp->extendsp(); extp;
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
        যদি (ধ্রুব AstClass* ধ্রুব classp = VN_CAST(modp, Class)) {
            puts(" : ");
            যদি (classp->extendsp()) {
                তার্কিক needComma = অসত্য;
                প্রতি (ধ্রুব AstClassExtends* extp = classp->extendsp(); extp;
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
        ofp()->putsPrivate(অসত্য);  // public:

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
        emitFuncDecls(modp, /* inClassBody: */ অসত্য);
        emitSystemCSection(modp, VSystemCSectionType::HDR_POST);
    }

    // VISITORS
    শূন্য visit(Ast_গ্রন্থি* nodep) override { nodep->v3fatalSrc("Unused"); }  // LCOV_EXCL_LINE

    explicit EmitCHeader(ধ্রুব Ast_গ্রন্থি_Module* modp) {
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
        যদি (ধ্রুব AstClassPackage* ধ্রুব packagep = VN_CAST(modp, ClassPackage)) {
            forModCUse(packagep->classp(), VUseType::INT_INCLUDE | VUseType::INT_FWD_CLASS,
                       add_to_cuse_set);
        }

        প্রতি (ধ্রুব string& s : cuse_set) puts(s);
        puts("\n");

        emitAll(modp);

        যদি (ধ্রুব AstClassPackage* ধ্রুব packagep = VN_CAST(modp, ClassPackage)) {
            // Put the non-static class implementation in same h file for speed
            emitAll(packagep->classp());
        }

        ofp()->putsEndGuard();

        // Close output file
        closeOutputFile();
    }
    ~EmitCHeader() override = স্বভাব;

সার্বজনিক:
    স্থির শূন্য main(ধ্রুব Ast_গ্রন্থি_Module* modp) { ধ্রুব EmitCHeader emitCHeader{modp}; }
};

//######################################################################
// EmitC class functions

শূন্য V3EmitC::emitcHeaders() {
    UINFO(2, __FUNCTION__ << ":");

    // Process each module in turn
    প্রতি (ধ্রুব Ast_গ্রন্থি* nodep = v3Global.rootp()->modulesp(); nodep; nodep = nodep->nextp()) {
        যদি (VN_IS(nodep, Class)) অনুবর্ত;  // Declared with the ClassPackage
        ধ্রুব Ast_গ্রন্থি_Module* ধ্রুব modp = VN_AS(nodep, গ্রন্থি_Module);
        যদি (modp->isConstPool()) অনুবর্ত;  // Emitted by V3EmitCConstPool
        EmitCHeader::main(modp);
    }
}
