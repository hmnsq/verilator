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

#അന്തര്ഭാവ "V3PchAstMT.h"

#അന്തര്ഭാവ "V3EmitC.h"
#അന്തര്ഭാവ "V3EmitCBase.h"
#അന്തര്ഭാവ "V3File.h"
#അന്തര്ഭാവ "V3MemberMap.h"
#അന്തര്ഭാവ "V3UniqueNames.h"

#അന്തര്ഭാവ <algorithm>
#അന്തര്ഭാവ <cstdint>
#അന്തര്ഭാവ <set>
#അന്തര്ഭാവ <string>
#അന്തര്ഭാവ <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Internal EmitC implementation

വർഗ്ഗം EmitCHeader final : പൊതു EmitCBaseVisitorConst {
    V3UniqueNames m_names;
    VMemberMap m_memberMap;
    // METHODS

    വർഗ്ഗം Cover_എണ്ണം_Visitor final : പൊതു VNVisitorConst {
        പൂർണ്ണസംഖ്യ m_bins = 0;  // Running total of coverage bins counted so far

        ശൂന്യം visit(Ast_കണ്ണി_Cover_Decl* nodep) override {
            // Each module class owns the counters for declarations it emits;
            // duplicate no-inline instances need separate arrays for
            // forcePerInstance reporting.
            എങ്കിൽ (!nodep->dataDeclNullp()) m_bins += nodep->size();
        }
        ശൂന്യം visit(Ast_കണ്ണി* nodep) override { iterateChildrenConst(nodep); }

    പൊതു:
        explicit Cover_എണ്ണം_Visitor(സ്ഥിരം Ast_കണ്ണി_Module* modp) {
            iterateConst(const_cast<Ast_കണ്ണി_Module*>(modp));
        }
        പൂർണ്ണസംഖ്യ bins() സ്ഥിരം { തിരികെ m_bins; }
    };

    ശൂന്യം decorateFirst(തർക്കം& first, സ്ഥിരം string& str) {
        എങ്കിൽ (first) {
            putsDecoration(nullptr, str);
            first = അസത്യം;
        }
    }
    ശൂന്യം emitCellDecls(സ്ഥിരം Ast_കണ്ണി_Module* modp) {
        തർക്കം first = സത്യം;
        വേണ്ടി (സ്ഥിരം Ast_കണ്ണി* nodep = modp->stmtsp(); nodep; nodep = nodep->nextp()) {
            എങ്കിൽ (സ്ഥിരം AstCell* സ്ഥിരം cellp = VN_CAST(nodep, Cell)) {
                എങ്കിൽ (cellp->modp()->isConstPool()) തുടരുക;  // Special emit rules
                decorateFirst(first, "// CELLS\n");
                putns(cellp, EmitCUtil::prefixNameProtect(cellp->modp()) + "* "
                                 + cellp->nameProtect() + ";\n");
            }
        }
    }
    ശൂന്യം emitDesignVarDecls(സ്ഥിരം Ast_കണ്ണി_Module* modp) {
        // V3VariableOrder puts all variables at the front of the list: top level ports, then
        // trivially constructible variables, then the not trivially constructible ones, then
        // those that are not model state
        സ്ഥിരം AstVar* portp = nullptr;
        സ്ഥിരം AstVar* trivp = nullptr;
        സ്ഥിരം AstVar* nontp = nullptr;
        size_t nPort = 0;
        size_t nTriv = 0;
        size_t nNont = 0;
        വേണ്ടി (സ്ഥിരം Ast_കണ്ണി* nodep = modp->stmtsp(); nodep; nodep = nodep->nextp()) {
            സ്ഥിരം AstVar* സ്ഥിരം varp = VN_CAST(nodep, Var);
            എങ്കിൽ (!varp) നിർത്തുക;
            എങ്കിൽ (!varp->isModelState()) {
                // Cache line alignment is only effective on emitted fields
                UASSERT_OBJ(!varp->mtask_Cache_വരി_Align(), varp, "Aligned non-field");
            } അല്ലെങ്കിൽ എങ്കിൽ (varp->isPrimaryIO()) {
                എങ്കിൽ (!portp) portp = varp;
                ++nPort;
            } അല്ലെങ്കിൽ എങ്കിൽ (varp->isTriviallyConstructible()) {
                എങ്കിൽ (!trivp) trivp = varp;
                ++nTriv;
            } അല്ലെങ്കിൽ {
                എങ്കിൽ (!nontp) nontp = varp;
                ++nNont;
            }
        }

        സ്ഥിരം auto emitAndStep = [&](സ്ഥിരം AstVar*& varpr) {
            emitVarDecl(varpr);
            varpr = VN_CAST(varpr->nextp(), Var);
        };

        // Emit primary ports
        എങ്കിൽ (nPort) {
            putsDecoration(nullptr, "\n// TOP LEVEL PORTS\n");
            // In anonymous union to prevent the C++ compiler from generating
            // constructor/destructor calls
            puts("union {\nstruct {\n");
            വേണ്ടി (size_t i = 0; i < nPort; ++i) emitAndStep(portp);
            puts("} __VprimaryPorts;\n};\n");
        }

        // Emit trivially constructible variables
        എങ്കിൽ (nTriv) {
            putsDecoration(nullptr, "\n// TRIVIALLY CONSTRUCTIBLE STATE\n");
            എങ്കിൽ (v3Global.opt.compLimitMembers() == 0) {
                // Anonymous structs disabled, so just emit them directly
                വേണ്ടി (size_t i = 0; i < nTriv; ++i) emitAndStep(trivp);
            } അല്ലെങ്കിൽ {
                സ്ഥിരം size_t lim = v3Global.opt.compLimitMembers();
                size_t anonL3s = 1;
                size_t anonL2s = 1;
                size_t anonL1s = 1;
                എങ്കിൽ (nTriv > (lim * lim * lim)) {
                    anonL3s = (nTriv + (lim * lim * lim) - 1) / (lim * lim * lim);
                    anonL2s = lim;
                    anonL1s = lim;
                } അല്ലെങ്കിൽ എങ്കിൽ (nTriv > (lim * lim)) {
                    anonL2s = (nTriv + (lim * lim) - 1) / (lim * lim);
                    anonL1s = lim;
                } അല്ലെങ്കിൽ എങ്കിൽ (nTriv > lim) {
                    anonL1s = (nTriv + lim - 1) / lim;
                }
                എങ്കിൽ (anonL1s != 1) {
                    putsDecoration(
                        nullptr,
                        "// Anonymous structures to workaround compiler member-count bugs\n");
                }
                size_t nEmitted = 0;
                വേണ്ടി (size_t l3 = 0; l3 < anonL3s && nEmitted < nTriv; ++l3) {
                    എങ്കിൽ (anonL3s != 1) puts("struct {\n");
                    വേണ്ടി (size_t l2 = 0; l2 < anonL2s && nEmitted < nTriv; ++l2) {
                        എങ്കിൽ (anonL2s != 1) puts("struct {\n");
                        വേണ്ടി (size_t l1 = 0; l1 < anonL1s && nEmitted < nTriv; ++l1) {
                            എങ്കിൽ (anonL1s != 1) puts("struct {\n");
                            വേണ്ടി (size_t l0 = 0; l0 < lim && nEmitted < nTriv; ++l0, ++nEmitted) {
                                emitAndStep(trivp);
                            }
                            എങ്കിൽ (anonL1s != 1) puts("};\n");
                        }
                        എങ്കിൽ (anonL2s != 1) puts("};\n");
                    }
                    എങ്കിൽ (anonL3s != 1) puts("};\n");
                }
                UASSERT_OBJ(nEmitted == nTriv, modp, "Not all variables emitted");
            }
        }

        // Emit not trivially constructible variables
        എങ്കിൽ (nNont) {
            putsDecoration(nullptr, "\n// NOT TRIVIALLY CONSTRUCTIBLE STATE\n");
            // Classes use real constructors
            എങ്കിൽ (VN_IS(modp, Class)) {
                വേണ്ടി (size_t i = 0; i < nNont; ++i) emitAndStep(nontp);
                തിരികെ;
            }
            // Output under an anonymous union to prevent the C++ compiler from generating
            // constructor/destructor calls for them. They are managed explicitly in ctor/dtor.
            // While 'union { singleVar; }' for each variable would be simpler, it blows up
            // clang's compile time when using pre-compiled headers, joy.
            puts("union {\nstruct {\n");
            വേണ്ടി (size_t i = 0; i < nNont; ++i) emitAndStep(nontp);
            puts("} __Vnontrivs;\n};\n");
        }
    }
    ശൂന്യം emitInternalVarDecls(സ്ഥിരം Ast_കണ്ണി_Module* modp) {
        എങ്കിൽ (സ്ഥിരം AstClass* സ്ഥിരം classp = VN_CAST(modp, Class)) {
            എങ്കിൽ (classp->needRNG()) {
                putsDecoration(nullptr, "\n// INTERNAL VARIABLES\n");
                puts("VlRNGReseeds __Vm_rng;\n");
            }
        } അല്ലെങ്കിൽ {  // not class
            putsDecoration(nullptr, "\n// INTERNAL VARIABLES\n");
            puts(EmitCUtil::symClassName() + "* vlSymsp;\n");
            puts("const char* vlNamep;\n");
            // Allocate object-local coverage counters only on modules that
            // contain emitted coverage declarations.
            സ്ഥിരം പൂർണ്ണസംഖ്യ coverBins = Cover_എണ്ണം_Visitor{modp}.bins();
            എങ്കിൽ (coverBins) {
                puts(v3Global.opt.threads() > 1 ? "std::atomic<uint32_t>" : "uint32_t");
                puts(" __Vcoverage[");
                puts(std::to_string(coverBins));
                puts("]{};\n");
            }
        }
    }
    ശൂന്യം emitParamDecls(സ്ഥിരം Ast_കണ്ണി_Module* modp) {
        തർക്കം first = സത്യം;
        വേണ്ടി (Ast_കണ്ണി* nodep = modp->stmtsp(); nodep; nodep = nodep->nextp()) {
            എങ്കിൽ (സ്ഥിരം AstVar* സ്ഥിരം varp = VN_CAST(nodep, Var)) {
                എങ്കിൽ (varp->isParam()) {
                    decorateFirst(first, "\n// PARAMETERS\n");
                    putns(varp, "static const ");
                    puts(varp->dtypep()->cType(varp->nameProtect(), അസത്യം, അസത്യം));
                    puts(";\n");
                }
            }
        }
    }
    ശൂന്യം emitCtorDtorDecls(സ്ഥിരം Ast_കണ്ണി_Module* modp) {
        // Classes use CFuncs with isConstructor/isDestructor
        എങ്കിൽ (VN_IS(modp, Class)) തിരികെ;

        // Proper modules use loose split ctor/dtor and are not copyable/movable
        സ്ഥിരം std::string name = EmitCUtil::prefixNameProtect(modp);
        putsDecoration(nullptr, "\n// CONSTRUCTORS\n");
        putns(modp, name + "(" + EmitCUtil::symClassName() + "* symsp, const char* namep);\n");
        putns(modp, "~" + name + "();\n");
        putns(modp, "VL_UNCOPYABLE(" + name + ");\n");
        putns(modp, "VL_UNMOVABLE(" + name + ");\n");
    }
    ശൂന്യം emitInternalMethodDecls(സ്ഥിരം Ast_കണ്ണി_Module* modp) {
        തർക്കം first = സത്യം;
        സ്ഥിരം string section = "\n// INTERNAL METHODS\n";

        എങ്കിൽ (!VN_IS(modp, Class)) {
            decorateFirst(first, section);
            എങ്കിൽ (v3Global.opt.coverage()) {
                puts("void " + protect("__Vconfigure") + "(bool first);\n");
            }
        } അല്ലെങ്കിൽ {
            decorateFirst(first, section);
            സ്ഥിരം std::string name = V3OutFormatter::quoteNameControls(
                VIdProtect::protect_വാക്കുകൾ_If(modp->prettyName(), v3Global.opt.protectIds()));
            // "override", but don't want clang to start checking them:
            puts("virtual const char* typeName() const { return \""s + name + "\"; }\n");
        }

        എങ്കിൽ (v3Global.opt.coverage() && !VN_IS(modp, Class)) {
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

        എങ്കിൽ (v3Global.opt.coverageToggle() && !VN_IS(modp, Class)) {
            decorateFirst(first, section);
            puts("void __vlCoverToggleInsert(int begin, int end, bool ranged, ");
            puts(v3Global.opt.threads() > 1 ? "std::atomic<uint32_t>" : "uint32_t");
            puts("* countp, bool enable, bool localCounter, const char* filenamep, int lineno, "
                 "int column,\n");
            puts("const char* hierp, const char* pagep, const char* commentp);\n");
        }

        എങ്കിൽ (v3Global.opt.savable()) {
            decorateFirst(first, section);
            puts("void " + protect("__Vserialize") + "(VerilatedSerialize& os);\n");
            puts("void " + protect("__Vdeserialize") + "(VerilatedDeserialize& os);\n");
        }

        // Polymorphic clone for concrete (non-abstract, non-interface) classes
        എങ്കിൽ (സ്ഥിരം AstClass* സ്ഥിരം classp = VN_CAST(modp, Class)) {
            എങ്കിൽ (!classp->isInterfaceClass() && !classp->isVirtual()) {
                decorateFirst(first, section);
                using EmbeddedCovergroupVar = std::pair<സ്ഥിരം AstClass*, സ്ഥിരം AstVar*>;
                std::vector<EmbeddedCovergroupVar> embeddedCovergroupVars;
                സ്ഥിരം auto hasEnclosingBackPointer = [](സ്ഥിരം AstClass* covergroupp) {
                    തിരികെ covergroupp->exists([](സ്ഥിരം AstVar* സ്ഥിരം varp) {
                        സ്ഥിരം AstClassRefDType* സ്ഥിരം refp
                            = VN_CAST(varp->dtypep()->skipRefp(), ClassRefDType);
                        തിരികെ refp && refp->rawPointer();
                    });
                };
                const_cast<AstClass*>(classp)->foreachMember(
                    [&](AstClass* സ്ഥിരം memberClassp, AstVar* സ്ഥിരം varp) {
                        സ്ഥിരം AstClassRefDType* സ്ഥിരം refp
                            = VN_CAST(varp->dtypep()->skipRefp(), ClassRefDType);
                        എങ്കിൽ (refp && refp->classp()->isCovergroup()
                            && hasEnclosingBackPointer(refp->classp())) {
                            embeddedCovergroupVars.emplace_back(memberClassp, varp);
                        }
                    });
                സ്ഥിരം string className = EmitCUtil::prefixNameProtect(classp);
                എങ്കിൽ (embeddedCovergroupVars.empty() && !classp->hasRandVarsUpdate()) {
                    putns(classp,
                          "VlClass* clone() const { return new " + className + "(*this); }\n");
                } അല്ലെങ്കിൽ {
                    putns(classp, "VlClass* clone() const { " + className + "* const clonep = new "
                                      + className + "(*this); ");
                    എങ്കിൽ (classp->hasRandVarsUpdate()) {
                        സ്ഥിരം string updateName = "__VnoInFunc___VupdateRandVars";
                        AstCFunc* സ്ഥിരം updatep
                            = VN_AS(m_memberMap.findMember(classp, updateName), CFunc);
                        UASSERT_OBJ(updatep, classp, "Missing updateRandVars method");
                        puts("clonep->" + updatep->nameProtect() + "();\n");
                    }
                    വേണ്ടി (സ്ഥിരം EmbeddedCovergroupVar& item : embeddedCovergroupVars) {
                        puts("clonep->" + EmitCUtil::prefixNameProtect(item.first)
                             + "::" + item.second->nameProtect() + " = VlNull{}; ");
                    }
                    puts("return clonep; }\n");
                }
            }
        }
    }
    ശൂന്യം emitEnums(സ്ഥിരം Ast_കണ്ണി_Module* modp) {
        തർക്കം first = സത്യം;
        വേണ്ടി (സ്ഥിരം Ast_കണ്ണി* nodep = modp->stmtsp(); nodep; nodep = nodep->nextp()) {
            സ്ഥിരം AstTypedef* സ്ഥിരം tdefp = VN_CAST(nodep, Typedef);
            എങ്കിൽ (!tdefp) തുടരുക;
            എങ്കിൽ (!tdefp->attrPublic()) തുടരുക;
            സ്ഥിരം AstEnumDType* സ്ഥിരം edtypep
                = VN_CAST(tdefp->dtypep()->skipRefToEnump(), EnumDType);
            എങ്കിൽ (!edtypep) തുടരുക;
            decorateFirst(first, "\n// ENUMS (that were declared public)\n");
            എങ്കിൽ (edtypep->width() > 64) {
                putsDecoration(tdefp,
                               "// enum " + tdefp->nameProtect() + " ignored: Too wide for C++\n");
            } അല്ലെങ്കിൽ {
                putns(tdefp, "enum " + tdefp->name() + " {\n");
                വേണ്ടി (സ്ഥിരം AstEnumItem* itemp = edtypep->itemsp(); itemp;
                     itemp = VN_AS(itemp->nextp(), EnumItem)) {
                    എങ്കിൽ (സ്ഥിരം AstConst* സ്ഥിരം constp = VN_CAST(itemp->valuep(), Const)) {
                        എങ്കിൽ (constp->അക്കം().isFourState()) {
                            putns(itemp, "// " + itemp->nameProtect() + " is four-state\n");
                            തുടരുക;
                        }
                    }
                    putns(itemp, itemp->nameProtect());
                    puts(" = ");
                    putns(itemp, VN_AS(itemp->valuep(), Const)->അക്കം().emitC());
                    എങ്കിൽ (VN_IS(itemp->nextp(), EnumItem)) puts(",");
                    puts("\n");
                }
                puts("};\n");
            }
        }
    }
    ശൂന്യം emitStructDecl(സ്ഥിരം Ast_കണ്ണി_Module* modp, Ast_കണ്ണി_UOr_Struct_DType* sdtypep,
                        std::set<Ast_കണ്ണി_UOr_Struct_DType*>& emitted) {
        എങ്കിൽ (emitted.എണ്ണം(sdtypep) > 0) തിരികെ;
        emitted.തിരുകുക(sdtypep);
        വേണ്ടി (AstMemberDType* itemp = sdtypep->membersp(); itemp;
             itemp = VN_AS(itemp->nextp(), MemberDType)) {
            Ast_കണ്ണി_UOr_Struct_DType* സ്ഥിരം subp = itemp->getChildStructp();
            എങ്കിൽ (subp && (!subp->packed() || sdtypep->packed())) {
                // Recurse if it belongs to the current module
                എങ്കിൽ (subp->classOrPackagep() == modp) {
                    emitStructDecl(modp, subp, emitted);
                    puts("\n");
                }
            }
        }
        എങ്കിൽ (sdtypep->packed()) {
            emitPackedUOrSBody(sdtypep);
        } അല്ലെങ്കിൽ {
            emitUnpackedUOrSBody(sdtypep);
        }
    }
    എണ്ണൽ വർഗ്ഗം AttributeType : uint8_t { WIDTH, DIMENSION };
    // Get member attribute based on type
    പൂർണ്ണസംഖ്യ get_കണ്ണി_Attribute(സ്ഥിരം AstMemberDType* itemp, AttributeType type) {
        സ്ഥിരം തർക്കം isArrayType = itemp->dtypep()->isNonPackedArray();
        തിരഞ്ഞെടുപ്പ് (type) {
        അവസ്ഥ AttributeType::WIDTH: {
            എങ്കിൽ (isArrayType) {
                // For arrays, get innermost element width
                സ്ഥിരം Ast_കണ്ണി_DType* dtype = itemp->dtypep();
                ആകുമ്പോൾ (dtype->subDTypep()) dtype = dtype->subDTypep();
                തിരികെ dtype->width();
            }
            തിരികെ itemp->width();
        }
        അവസ്ഥ AttributeType::DIMENSION: {
            // Return array dimension or 0 for non-arrays
            തിരികെ isArrayType ? itemp->dtypep()->dimensions(സത്യം).second : 0;
        }
        സ്ഥിരസ്ഥിതി: {
            തിരികെ 0;
        }
        }
    }
    template <AttributeType T>
    ശൂന്യം emitMemberVector(സ്ഥിരം Ast_കണ്ണി_UOr_Struct_DType* sdtypep) {
        puts("return {");
        തർക്കം needComma = അസത്യം;
        വേണ്ടി (സ്ഥിരം AstMemberDType* itemp = sdtypep->membersp(); itemp;
             itemp = VN_AS(itemp->nextp(), MemberDType)) {
            എങ്കിൽ (!itemp->isConstrainedRand()) തുടരുക;
            // Comma handling: add before element except first
            എങ്കിൽ (needComma) puts(",\n");
            putns(itemp, std::to_string(get_കണ്ണി_Attribute(itemp, T)));
            needComma = സത്യം;
        }
        puts("};\n}\n");
    }
    ശൂന്യം emitUnpackedUOrSBody(Ast_കണ്ണി_UOr_Struct_DType* sdtypep) {
        putns(sdtypep, sdtypep->verilogKwd());  // "struct"/"union"
        puts(" " + EmitCUtil::prefixNameProtect(sdtypep) + " {\n");
        വേണ്ടി (സ്ഥിരം AstMemberDType* itemp = sdtypep->membersp(); itemp;
             itemp = VN_AS(itemp->nextp(), MemberDType)) {
            putns(itemp, itemp->dtypep()->cType(itemp->nameProtect(), അസത്യം, അസത്യം));
            puts(";\n");
        }

        // Three helper functions for struct constrained randomization:
        // - memberNames: Get member names
        // - getMembers: Access member references
        // - memberIndices: Retrieve member indices
        // - memberWidth: Retrieve member width
        // - memberDimension: Retrieve member dimension
        എങ്കിൽ (sdtypep->isConstrainedRand()) {
            തർക്കം needComma = അസത്യം;
            putns(sdtypep, "\nstd::vector<std::string> memberNames(void) const {\n");
            puts("return {");
            വേണ്ടി (സ്ഥിരം AstMemberDType* itemp = sdtypep->membersp(); itemp;
                 itemp = VN_AS(itemp->nextp(), MemberDType)) {
                എങ്കിൽ (!itemp->isConstrainedRand()) തുടരുക;
                എങ്കിൽ (needComma) puts(",\n");
                putns(itemp, "\"" + itemp->shortName() + "\"");
                needComma = സത്യം;
            }
            puts("};\n}\n");

            putns(sdtypep, "\nstd::vector<int> memberWidth(void) const {\n");
            emitMemberVector<AttributeType::WIDTH>(sdtypep);

            putns(sdtypep, "\nstd::vector<int> memberDimension(void) const {\n");
            emitMemberVector<AttributeType::DIMENSION>(sdtypep);

            needComma = അസത്യം;
            putns(sdtypep, "\nauto memberIndices(void) const {\n");
            puts("return std::index_sequence_for<");
            വേണ്ടി (സ്ഥിരം AstMemberDType* itemp = sdtypep->membersp(); itemp;
                 itemp = VN_AS(itemp->nextp(), MemberDType)) {
                എങ്കിൽ (!itemp->isConstrainedRand()) തുടരുക;
                എങ്കിൽ (needComma) puts(",\n");
                putns(itemp, itemp->dtypep()->cType("", അസത്യം, അസത്യം));
                needComma = സത്യം;
            }
            puts(">{};\n}\n");

            needComma = അസത്യം;
            putns(sdtypep, "\ntemplate <typename T>");
            putns(sdtypep, "\nauto getMembers(T& obj) {\n");
            puts("return std::tie(");
            വേണ്ടി (സ്ഥിരം AstMemberDType* itemp = sdtypep->membersp(); itemp;
                 itemp = VN_AS(itemp->nextp(), MemberDType)) {
                എങ്കിൽ (!itemp->isConstrainedRand()) തുടരുക;
                എങ്കിൽ (needComma) puts(",\n");
                putns(itemp, "obj." + itemp->nameProtect());
                needComma = സത്യം;
            }
            puts(");\n}\n");
        }
        putns(sdtypep, "\nbool operator==(const " + EmitCUtil::prefixNameProtect(sdtypep)
                           + "& rhs) const {\n");
        puts("return ");
        വേണ്ടി (സ്ഥിരം AstMemberDType* itemp = sdtypep->membersp(); itemp;
             itemp = VN_AS(itemp->nextp(), MemberDType)) {
            എങ്കിൽ (itemp != sdtypep->membersp()) puts("\n    && ");
            putns(itemp, itemp->nameProtect() + " == " + "rhs." + itemp->nameProtect());
        }
        puts(";\n");
        puts("}\n");
        putns(sdtypep, "bool operator!=(const " + EmitCUtil::prefixNameProtect(sdtypep)
                           + "& rhs) const {\n");
        puts("return !(*this == rhs);\n}\n");
        putns(sdtypep, "\nbool operator<(const " + EmitCUtil::prefixNameProtect(sdtypep)
                           + "& rhs) const {\n");
        വേണ്ടി (സ്ഥിരം AstMemberDType* itemp = sdtypep->membersp(); itemp;
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
    ശൂന്യം emitVlAssign(സ്ഥിരം Ast_കണ്ണി_DType* സ്ഥിരം lhstype, സ്ഥിരം Ast_കണ്ണി_DType* rhstype,
                      സ്ഥിരം std::string& off, സ്ഥിരം std::string& lhsdata,
                      സ്ഥിരം std::string& rhsdata, തർക്കം getfunc) {
        puts(getfunc ? "VL_ASSIGNSEL_" : "VL_SELASSIGN_");
        puts(lhstype->charIQWN());
        puts(rhstype->charIQWN());
        puts("(" + std::to_string(lhstype->width()) + ", ");  // LHS width
        എങ്കിൽ (getfunc) {
            puts(std::to_string(rhstype->width()) + ", ");  // Number of copy bits
            puts(off + ", ");  // LHS offset
        } അല്ലെങ്കിൽ {
            // Number of copy bits. Use widthTototalBytes to
            // make VL_SELASSIGN_XX clear upper unused bits for us.
            // puts(std::to_string(lhstype->width()) + ", ");
            puts(std::to_string(lhstype->widthTotalBytes() * 8) + ", ");
        }
        puts(lhsdata + ", ");  // LHS data
        puts(rhsdata);  // RHS data
        എങ്കിൽ (!getfunc) {
            puts(", " + off);  // RHS offset
        }
        puts(");\n");
    }

    // `retOrArg` should be prefixed by `&` or suffixed by `.data()` depending on its type
    ശൂന്യം emitPackedMember(സ്ഥിരം Ast_കണ്ണി_DType* parentDtypep, സ്ഥിരം Ast_കണ്ണി_DType* dtypep,
                          സ്ഥിരം std::string& fieldname, സ്ഥിരം std::string& offset, തർക്കം getfunc,
                          സ്ഥിരം std::string& retOrArg) {
        dtypep = dtypep->skipRefp();
        എങ്കിൽ (സ്ഥിരം auto* adtypep = VN_CAST(dtypep, PackArrayDType)) {
            സ്ഥിരം std::string index = m_names.get("__Vi");
            puts("for (int " + index + " = 0; " + index + " < "
                 + std::to_string(adtypep->elementsConst()) + "; ++" + index + ") {\n");

            സ്ഥിരം std::string offsetInLoop
                = offset + " + " + index + " * " + std::to_string(adtypep->subDTypep()->width());
            സ്ഥിരം std::string newName = fieldname + "[" + index + "]";
            emitPackedMember(parentDtypep, adtypep->subDTypep(), newName, offsetInLoop, getfunc,
                             retOrArg);
            puts("}\n");
        } അല്ലെങ്കിൽ എങ്കിൽ (VN_IS(dtypep, കണ്ണി_UOr_Struct_DType)) {
            സ്ഥിരം std::string tmp = m_names.get("__Vtmp");
            എങ്കിൽ (getfunc) {  // Emit `get` func;
                // auto __tmp = field.get();
                puts("auto " + tmp + " = " + fieldname + ".get();\n");
                // VL_ASSIGNSEL_XX(rbits, obits, lsb, lhsdata, rhsdata);
                emitVlAssign(parentDtypep, dtypep, offset, retOrArg, tmp, getfunc);
            } അല്ലെങ്കിൽ {  // Emit `set` func
                സ്ഥിരം std::string tmptype = AstCDType::typeToHold(dtypep->width());
                // type tmp;
                puts(tmptype + " " + tmp + ";\n");
                // VL_SELASSIGN_XX(rbits, obits, lhsdata, rhsdata, roffset);
                emitVlAssign(dtypep, parentDtypep, offset, tmp, retOrArg, getfunc);
                // field.set(__tmp);
                puts(fieldname + ".set(" + tmp + ");\n");
            }
        } അല്ലെങ്കിൽ {
            UASSERT_OBJ(VN_IS(dtypep, EnumDType) || VN_IS(dtypep, BasicDType), dtypep,
                        "Unsupported type in packed struct or union");
            എങ്കിൽ (getfunc) {  // Emit `get` func;
                // VL_ASSIGNSEL_XX(rbits, obits, lsb, lhsdata, rhsdata);
                emitVlAssign(parentDtypep, dtypep, offset, retOrArg, fieldname, getfunc);
            } അല്ലെങ്കിൽ {  // Emit `set` func
                // VL_SELASSIGN_XX(rbits, obits, lhsdata, rhsdata, roffset);
                emitVlAssign(dtypep, parentDtypep, offset, fieldname, retOrArg, getfunc);
            }
        }
    }
    ശൂന്യം emitPackedUOrSBody(Ast_കണ്ണി_UOr_Struct_DType* sdtypep) {
        putns(sdtypep, sdtypep->verilogKwd());  // "struct"/"union"
        puts(" " + EmitCUtil::prefixNameProtect(sdtypep) + " {\n");

        AstMemberDType* itemp;
        AstMemberDType* lastItemp;
        സ്ഥിരം AstMemberDType* witemp = nullptr;
        // LSB is first field in C, so loop backwards
        വേണ്ടി (lastItemp = sdtypep->membersp(); lastItemp && lastItemp->nextp();
             lastItemp = VN_AS(lastItemp->nextp(), MemberDType)) {
            എങ്കിൽ (lastItemp->width() == sdtypep->width()) witemp = lastItemp;
        }
        വേണ്ടി (itemp = lastItemp; itemp; itemp = VN_CAST(itemp->backp(), MemberDType)) {
            putns(itemp, itemp->dtypep()->cType(itemp->nameProtect(), അസത്യം, അസത്യം, സത്യം));
            puts(";\n");
        }

        സ്ഥിരം std::string retArgName = m_names.get("__v");
        സ്ഥിരം std::string retArgType = AstCDType::typeToHold(sdtypep->width());

        // Emit `get` member function
        puts(retArgType + " get() const {\n");
        puts(retArgType + " " + retArgName + ";\n");
        എങ്കിൽ (VN_IS(sdtypep, StructDType)) {
            വേണ്ടി (itemp = lastItemp; itemp; itemp = VN_CAST(itemp->backp(), MemberDType)) {
                emitPackedMember(sdtypep, itemp->dtypep(), itemp->nameProtect(),
                                 std::to_string(itemp->lsb()), /*getfunc=*/സത്യം, retArgName);
            }
        } അല്ലെങ്കിൽ {
            // We only need to fill the widest field of union
            emitPackedMember(sdtypep, witemp->dtypep(), witemp->nameProtect(),
                             std::to_string(witemp->lsb()), /*getfunc=*/സത്യം, retArgName);
        }
        puts("return " + retArgName + ";\n");
        puts("}\n");

        // Emit `set` member function
        puts("void set(const " + retArgType + "& " + retArgName + ") {\n");
        എങ്കിൽ (VN_IS(sdtypep, StructDType)) {
            വേണ്ടി (itemp = lastItemp; itemp; itemp = VN_CAST(itemp->backp(), MemberDType)) {
                emitPackedMember(sdtypep, itemp->dtypep(), itemp->nameProtect(),
                                 std::to_string(itemp->lsb()), /*getfunc=*/അസത്യം, retArgName);
            }
        } അല്ലെങ്കിൽ {
            // We only need to fill the widest field of union
            emitPackedMember(sdtypep, witemp->dtypep(), witemp->nameProtect(),
                             std::to_string(witemp->lsb()), /*getfunc=*/അസത്യം, retArgName);
        }

        puts("}\n");

        puts("};\n");
        m_names.reset();
    }
    ശൂന്യം emitStructs(സ്ഥിരം Ast_കണ്ണി_Module* modp) {
        // Track structs that've been emitted already
        std::set<Ast_കണ്ണി_UOr_Struct_DType*> emitted;
        വേണ്ടി (സ്ഥിരം Ast_കണ്ണി* nodep = modp->stmtsp(); nodep; nodep = nodep->nextp()) {
            സ്ഥിരം AstTypedef* സ്ഥിരം tdefp = VN_CAST(nodep, Typedef);
            എങ്കിൽ (!tdefp) തുടരുക;
            Ast_കണ്ണി_UOr_Struct_DType* സ്ഥിരം sdtypep
                = VN_CAST(tdefp->dtypep()->skipRefToEnump(), കണ്ണി_UOr_Struct_DType);
            എങ്കിൽ (!sdtypep) തുടരുക;
            // V3Inline can copy one module into several parents. Only the module that
            // declares the struct emits it, so C++ sees only one definition.
            സ്ഥിരം തർക്കം declaredInOtherModule = sdtypep->classOrPackagep() != modp;
            എങ്കിൽ (declaredInOtherModule) തുടരുക;
            emitStructDecl(modp, sdtypep, emitted);
        }
    }
    ശൂന്യം emitFuncDecls(സ്ഥിരം Ast_കണ്ണി_Module* modp, തർക്കം inClassBody) {
        std::vector<സ്ഥിരം AstCFunc*> funcsp;

        വേണ്ടി (സ്ഥിരം Ast_കണ്ണി* nodep = modp->stmtsp(); nodep; nodep = nodep->nextp()) {
            എങ്കിൽ (സ്ഥിരം AstCFunc* സ്ഥിരം funcp = VN_CAST(nodep, CFunc)) {
                എങ്കിൽ (funcp->dpiImportPrototype())  // Declared in __Dpi.h
                    തുടരുക;
                എങ്കിൽ (funcp->dpiExportDispatcher())  // Declared in __Dpi.h
                    തുടരുക;
                എങ്കിൽ (funcp->isMethod() != inClassBody)  // Only methods go inside class
                    തുടരുക;
                എങ്കിൽ (funcp->isMethod() && funcp->isLoose())  // Loose methods are declared lazily
                    തുടരുക;
                funcsp.തള്ളുക_back(funcp);
            }
        }

        std::stable_sort(funcsp.begin(), funcsp.end(), [](സ്ഥിരം Ast_കണ്ണി* ap, സ്ഥിരം Ast_കണ്ണി* bp) {
            തിരികെ ap->name() < bp->name();
        });

        വേണ്ടി (സ്ഥിരം AstCFunc* സ്ഥിരം funcp : funcsp) {
            എങ്കിൽ (inClassBody) ofp()->putsPrivate(funcp->declPrivate());
            emitCFuncDecl(funcp, modp);
        }
    }
    ശൂന്യം emitAll(സ്ഥിരം Ast_കണ്ണി_Module* modp) {
        // Include files required by this AstNodeModule
        എങ്കിൽ (സ്ഥിരം AstClass* സ്ഥിരം classp = VN_CAST(modp, Class)) {
            വേണ്ടി (സ്ഥിരം AstClassExtends* extp = classp->extendsp(); extp;
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
        എങ്കിൽ (!VN_IS(modp, Class)) puts("alignas(VL_CACHE_LINE_BYTES) ");
        puts(EmitCUtil::prefixNameProtect(modp));
        എങ്കിൽ (സ്ഥിരം AstClass* സ്ഥിരം classp = VN_CAST(modp, Class)) {
            puts(" : ");
            എങ്കിൽ (classp->extendsp()) {
                തർക്കം needComma = അസത്യം;
                വേണ്ടി (സ്ഥിരം AstClassExtends* extp = classp->extendsp(); extp;
                     extp = VN_AS(extp->nextp(), ClassExtends)) {
                    എങ്കിൽ (needComma) puts(", ");
                    // Use virtual only for interfaces for class inheritance
                    // (extends)
                    puts(extp->classp()->useVirtualPublic() ? "virtual public " : "public ");
                    putns(extp, EmitCUtil::prefixNameProtect(extp->classp()));
                    needComma = സത്യം;
                }
            } അല്ലെങ്കിൽ {
                puts("public virtual VlClass");
            }
        } അല്ലെങ്കിൽ {
            puts(" final");
        }
        puts(" {\n");
        ofp()->resetPrivate();
        ofp()->putsPrivate(അസത്യം);  // public:

        // Emit all class body contents
        emitCellDecls(modp);
        emitEnums(modp);
        emitDesignVarDecls(modp);
        emitInternalVarDecls(modp);
        emitParamDecls(modp);
        emitCtorDtorDecls(modp);
        emitInternalMethodDecls(modp);
        emitFuncDecls(modp, /* inClassBody: */ സത്യം);

        // From `systemc_interface
        emitSystemCSection(modp, VSystemCSectionType::INT);

        // Close class body
        puts("};\n");
        // }}}

        // Emit out of class function declarations
        puts("\n");
        emitFuncDecls(modp, /* inClassBody: */ അസത്യം);
        emitSystemCSection(modp, VSystemCSectionType::HDR_POST);
    }

    // VISITORS
    ശൂന്യം visit(Ast_കണ്ണി* nodep) override { nodep->v3fatalSrc("Unused"); }  // LCOV_EXCL_LINE

    explicit EmitCHeader(സ്ഥിരം Ast_കണ്ണി_Module* modp) {
        UINFO(5, "  Emitting header for " << EmitCUtil::prefixNameProtect(modp));

        // Open output file
        openNewOutputHeaderFile(EmitCUtil::prefixNameProtect(modp), "Design internal header");
        puts("// See " + EmitCUtil::മുകൾ_Class_Name() + ".h for the primary calling header\n");

        ofp()->putsGuard();

        // Include files
        puts("\n");
        ofp()->puts_Int_മുകൾ_Include();
        puts("#include \"verilated.h\"\n");
        എങ്കിൽ (modp->is_മുകൾ() && optSystemC()) puts("#include \"verilated_sc.h\"\n");
        എങ്കിൽ (v3Global.opt.mtasks()) puts("#include \"verilated_threads.h\"\n");
        എങ്കിൽ (v3Global.opt.savable()) puts("#include \"verilated_save.h\"\n");
        എങ്കിൽ (v3Global.opt.coverage()) puts("#include \"verilated_cov.h\"\n");
        എങ്കിൽ (v3Global.opt.coverage() || v3Global.useCovergroup())
            puts("#include \"verilated_covergroup.h\"\n");
        എങ്കിൽ (v3Global.usesTiming()) puts("#include \"verilated_timing.h\"\n");
        എങ്കിൽ (v3Global.useRandomizeMethods()) puts("#include \"verilated_random.h\"\n");
        എങ്കിൽ (v3Global.usesForce()) puts("#include \"verilated_force.h\"\n");

        std::set<string> cuse_set;
        auto add_to_cuse_set = [&](string s) { cuse_set.തിരുകുക(s); };

        forModCUse(modp, VUseType::INT_FWD_CLASS | VUseType::INT_INCLUDE, add_to_cuse_set);
        എങ്കിൽ (സ്ഥിരം AstClassPackage* സ്ഥിരം packagep = VN_CAST(modp, ClassPackage)) {
            forModCUse(packagep->classp(), VUseType::INT_INCLUDE | VUseType::INT_FWD_CLASS,
                       add_to_cuse_set);
        }

        വേണ്ടി (സ്ഥിരം string& s : cuse_set) puts(s);
        puts("\n");

        emitAll(modp);

        എങ്കിൽ (സ്ഥിരം AstClassPackage* സ്ഥിരം packagep = VN_CAST(modp, ClassPackage)) {
            // Put the non-static class implementation in same h file for speed
            emitAll(packagep->classp());
        }

        ofp()->putsEndGuard();

        // Close output file
        closeOutputFile();
    }
    ~EmitCHeader() override = സ്ഥിരസ്ഥിതി;

പൊതു:
    സ്ഥിതം ശൂന്യം പ്രധാന(സ്ഥിരം Ast_കണ്ണി_Module* modp) { സ്ഥിരം EmitCHeader emitCHeader{modp}; }
};

//######################################################################
// EmitC class functions

ശൂന്യം V3EmitC::emitcHeaders() {
    UINFO(2, __FUNCTION__ << ":");

    // Process each module in turn
    വേണ്ടി (സ്ഥിരം Ast_കണ്ണി* nodep = v3Global.rootp()->modulesp(); nodep; nodep = nodep->nextp()) {
        എങ്കിൽ (VN_IS(nodep, Class)) തുടരുക;  // Declared with the ClassPackage
        സ്ഥിരം Ast_കണ്ണി_Module* സ്ഥിരം modp = VN_AS(nodep, കണ്ണി_Module);
        എങ്കിൽ (modp->isConstPool()) തുടരുക;  // Emitted by V3EmitCConstPool
        EmitCHeader::പ്രധാന(modp);
    }
}
