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

#અન્તર્ભાવ "V3PchAstMT.h"

#અન્તર્ભાવ "V3EmitC.h"
#અન્તર્ભાવ "V3EmitCBase.h"
#અન્તર્ભાવ "V3File.h"
#અન્તર્ભાવ "V3MemberMap.h"
#અન્તર્ભાવ "V3UniqueNames.h"

#અન્તર્ભાવ <algorithm>
#અન્તર્ભાવ <cstdint>
#અન્તર્ભાવ <set>
#અન્તર્ભાવ <string>
#અન્તર્ભાવ <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Internal EmitC implementation

વર્ગ EmitCHeader final : જાહેર EmitCBaseVisitorConst {
    V3UniqueNames m_names;
    VMemberMap m_memberMap;
    // METHODS

    વર્ગ Cover_ગણતરી_Visitor final : જાહેર VNVisitorConst {
        પૂર્ણાંક m_bins = 0;  // Running total of coverage bins counted so far

        શૂન્ય visit(Ast_ગાંઠ_Cover_Decl* nodep) override {
            // Each module class owns the counters for declarations it emits;
            // duplicate no-inline instances need separate arrays for
            // forcePerInstance reporting.
            જો (!nodep->dataDeclNullp()) m_bins += nodep->size();
        }
        શૂન્ય visit(Ast_ગાંઠ* nodep) override { iterateChildrenConst(nodep); }

    જાહેર:
        explicit Cover_ગણતરી_Visitor(અચલ Ast_ગાંઠ_Module* modp) {
            iterateConst(const_cast<Ast_ગાંઠ_Module*>(modp));
        }
        પૂર્ણાંક bins() અચલ { પાછા m_bins; }
    };

    શૂન્ય decorateFirst(તાર્કિક& first, અચલ string& str) {
        જો (first) {
            putsDecoration(nullptr, str);
            first = ખોટું;
        }
    }
    શૂન્ય emitCellDecls(અચલ Ast_ગાંઠ_Module* modp) {
        તાર્કિક first = સાચું;
        માટે (અચલ Ast_ગાંઠ* nodep = modp->stmtsp(); nodep; nodep = nodep->nextp()) {
            જો (અચલ AstCell* અચલ cellp = VN_CAST(nodep, Cell)) {
                જો (cellp->modp()->isConstPool()) ચાલુ;  // Special emit rules
                decorateFirst(first, "// CELLS\n");
                putns(cellp, EmitCUtil::prefixNameProtect(cellp->modp()) + "* "
                                 + cellp->nameProtect() + ";\n");
            }
        }
    }
    શૂન્ય emitDesignVarDecls(અચલ Ast_ગાંઠ_Module* modp) {
        // V3VariableOrder puts all variables at the front of the list: top level ports, then
        // trivially constructible variables, then the not trivially constructible ones, then
        // those that are not model state
        અચલ AstVar* portp = nullptr;
        અચલ AstVar* trivp = nullptr;
        અચલ AstVar* nontp = nullptr;
        size_t nPort = 0;
        size_t nTriv = 0;
        size_t nNont = 0;
        માટે (અચલ Ast_ગાંઠ* nodep = modp->stmtsp(); nodep; nodep = nodep->nextp()) {
            અચલ AstVar* અચલ varp = VN_CAST(nodep, Var);
            જો (!varp) તોડો;
            જો (!varp->isModelState()) {
                // Cache line alignment is only effective on emitted fields
                UASSERT_OBJ(!varp->mtask_Cache_લીટી_Align(), varp, "Aligned non-field");
            } નહીંતર જો (varp->isPrimaryIO()) {
                જો (!portp) portp = varp;
                ++nPort;
            } નહીંતર જો (varp->isTriviallyConstructible()) {
                જો (!trivp) trivp = varp;
                ++nTriv;
            } નહીંતર {
                જો (!nontp) nontp = varp;
                ++nNont;
            }
        }

        અચલ auto emitAndStep = [&](અચલ AstVar*& varpr) {
            emitVarDecl(varpr);
            varpr = VN_CAST(varpr->nextp(), Var);
        };

        // Emit primary ports
        જો (nPort) {
            putsDecoration(nullptr, "\n// TOP LEVEL PORTS\n");
            // In anonymous union to prevent the C++ compiler from generating
            // constructor/destructor calls
            puts("union {\nstruct {\n");
            માટે (size_t i = 0; i < nPort; ++i) emitAndStep(portp);
            puts("} __VprimaryPorts;\n};\n");
        }

        // Emit trivially constructible variables
        જો (nTriv) {
            putsDecoration(nullptr, "\n// TRIVIALLY CONSTRUCTIBLE STATE\n");
            જો (v3Global.opt.compLimitMembers() == 0) {
                // Anonymous structs disabled, so just emit them directly
                માટે (size_t i = 0; i < nTriv; ++i) emitAndStep(trivp);
            } નહીંતર {
                અચલ size_t lim = v3Global.opt.compLimitMembers();
                size_t anonL3s = 1;
                size_t anonL2s = 1;
                size_t anonL1s = 1;
                જો (nTriv > (lim * lim * lim)) {
                    anonL3s = (nTriv + (lim * lim * lim) - 1) / (lim * lim * lim);
                    anonL2s = lim;
                    anonL1s = lim;
                } નહીંતર જો (nTriv > (lim * lim)) {
                    anonL2s = (nTriv + (lim * lim) - 1) / (lim * lim);
                    anonL1s = lim;
                } નહીંતર જો (nTriv > lim) {
                    anonL1s = (nTriv + lim - 1) / lim;
                }
                જો (anonL1s != 1) {
                    putsDecoration(
                        nullptr,
                        "// Anonymous structures to workaround compiler member-count bugs\n");
                }
                size_t nEmitted = 0;
                માટે (size_t l3 = 0; l3 < anonL3s && nEmitted < nTriv; ++l3) {
                    જો (anonL3s != 1) puts("struct {\n");
                    માટે (size_t l2 = 0; l2 < anonL2s && nEmitted < nTriv; ++l2) {
                        જો (anonL2s != 1) puts("struct {\n");
                        માટે (size_t l1 = 0; l1 < anonL1s && nEmitted < nTriv; ++l1) {
                            જો (anonL1s != 1) puts("struct {\n");
                            માટે (size_t l0 = 0; l0 < lim && nEmitted < nTriv; ++l0, ++nEmitted) {
                                emitAndStep(trivp);
                            }
                            જો (anonL1s != 1) puts("};\n");
                        }
                        જો (anonL2s != 1) puts("};\n");
                    }
                    જો (anonL3s != 1) puts("};\n");
                }
                UASSERT_OBJ(nEmitted == nTriv, modp, "Not all variables emitted");
            }
        }

        // Emit not trivially constructible variables
        જો (nNont) {
            putsDecoration(nullptr, "\n// NOT TRIVIALLY CONSTRUCTIBLE STATE\n");
            // Classes use real constructors
            જો (VN_IS(modp, Class)) {
                માટે (size_t i = 0; i < nNont; ++i) emitAndStep(nontp);
                પાછા;
            }
            // Output under an anonymous union to prevent the C++ compiler from generating
            // constructor/destructor calls for them. They are managed explicitly in ctor/dtor.
            // While 'union { singleVar; }' for each variable would be simpler, it blows up
            // clang's compile time when using pre-compiled headers, joy.
            puts("union {\nstruct {\n");
            માટે (size_t i = 0; i < nNont; ++i) emitAndStep(nontp);
            puts("} __Vnontrivs;\n};\n");
        }
    }
    શૂન્ય emitInternalVarDecls(અચલ Ast_ગાંઠ_Module* modp) {
        જો (અચલ AstClass* અચલ classp = VN_CAST(modp, Class)) {
            જો (classp->needRNG()) {
                putsDecoration(nullptr, "\n// INTERNAL VARIABLES\n");
                puts("VlRNGReseeds __Vm_rng;\n");
            }
        } નહીંતર {  // not class
            putsDecoration(nullptr, "\n// INTERNAL VARIABLES\n");
            puts(EmitCUtil::symClassName() + "* vlSymsp;\n");
            puts("const char* vlNamep;\n");
            // Allocate object-local coverage counters only on modules that
            // contain emitted coverage declarations.
            અચલ પૂર્ણાંક coverBins = Cover_ગણતરી_Visitor{modp}.bins();
            જો (coverBins) {
                puts(v3Global.opt.threads() > 1 ? "std::atomic<uint32_t>" : "uint32_t");
                puts(" __Vcoverage[");
                puts(std::to_string(coverBins));
                puts("]{};\n");
            }
        }
    }
    શૂન્ય emitParamDecls(અચલ Ast_ગાંઠ_Module* modp) {
        તાર્કિક first = સાચું;
        માટે (Ast_ગાંઠ* nodep = modp->stmtsp(); nodep; nodep = nodep->nextp()) {
            જો (અચલ AstVar* અચલ varp = VN_CAST(nodep, Var)) {
                જો (varp->isParam()) {
                    decorateFirst(first, "\n// PARAMETERS\n");
                    putns(varp, "static const ");
                    puts(varp->dtypep()->cType(varp->nameProtect(), ખોટું, ખોટું));
                    puts(";\n");
                }
            }
        }
    }
    શૂન્ય emitCtorDtorDecls(અચલ Ast_ગાંઠ_Module* modp) {
        // Classes use CFuncs with isConstructor/isDestructor
        જો (VN_IS(modp, Class)) પાછા;

        // Proper modules use loose split ctor/dtor and are not copyable/movable
        અચલ std::string name = EmitCUtil::prefixNameProtect(modp);
        putsDecoration(nullptr, "\n// CONSTRUCTORS\n");
        putns(modp, name + "(" + EmitCUtil::symClassName() + "* symsp, const char* namep);\n");
        putns(modp, "~" + name + "();\n");
        putns(modp, "VL_UNCOPYABLE(" + name + ");\n");
        putns(modp, "VL_UNMOVABLE(" + name + ");\n");
    }
    શૂન્ય emitInternalMethodDecls(અચલ Ast_ગાંઠ_Module* modp) {
        તાર્કિક first = સાચું;
        અચલ string section = "\n// INTERNAL METHODS\n";

        જો (!VN_IS(modp, Class)) {
            decorateFirst(first, section);
            જો (v3Global.opt.coverage()) {
                puts("void " + protect("__Vconfigure") + "(bool first);\n");
            }
        } નહીંતર {
            decorateFirst(first, section);
            અચલ std::string name = V3OutFormatter::quoteNameControls(
                VIdProtect::protect_શબ્દો_If(modp->prettyName(), v3Global.opt.protectIds()));
            // "override", but don't want clang to start checking them:
            puts("virtual const char* typeName() const { return \""s + name + "\"; }\n");
        }

        જો (v3Global.opt.coverage() && !VN_IS(modp, Class)) {
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

        જો (v3Global.opt.coverageToggle() && !VN_IS(modp, Class)) {
            decorateFirst(first, section);
            puts("void __vlCoverToggleInsert(int begin, int end, bool ranged, ");
            puts(v3Global.opt.threads() > 1 ? "std::atomic<uint32_t>" : "uint32_t");
            puts("* countp, bool enable, bool localCounter, const char* filenamep, int lineno, "
                 "int column,\n");
            puts("const char* hierp, const char* pagep, const char* commentp);\n");
        }

        જો (v3Global.opt.savable()) {
            decorateFirst(first, section);
            puts("void " + protect("__Vserialize") + "(VerilatedSerialize& os);\n");
            puts("void " + protect("__Vdeserialize") + "(VerilatedDeserialize& os);\n");
        }

        // Polymorphic clone for concrete (non-abstract, non-interface) classes
        જો (અચલ AstClass* અચલ classp = VN_CAST(modp, Class)) {
            જો (!classp->isInterfaceClass() && !classp->isVirtual()) {
                decorateFirst(first, section);
                using EmbeddedCovergroupVar = std::pair<અચલ AstClass*, અચલ AstVar*>;
                std::vector<EmbeddedCovergroupVar> embeddedCovergroupVars;
                અચલ auto hasEnclosingBackPointer = [](અચલ AstClass* covergroupp) {
                    પાછા covergroupp->exists([](અચલ AstVar* અચલ varp) {
                        અચલ AstClassRefDType* અચલ refp
                            = VN_CAST(varp->dtypep()->skipRefp(), ClassRefDType);
                        પાછા refp && refp->rawPointer();
                    });
                };
                const_cast<AstClass*>(classp)->foreachMember(
                    [&](AstClass* અચલ memberClassp, AstVar* અચલ varp) {
                        અચલ AstClassRefDType* અચલ refp
                            = VN_CAST(varp->dtypep()->skipRefp(), ClassRefDType);
                        જો (refp && refp->classp()->isCovergroup()
                            && hasEnclosingBackPointer(refp->classp())) {
                            embeddedCovergroupVars.emplace_back(memberClassp, varp);
                        }
                    });
                અચલ string className = EmitCUtil::prefixNameProtect(classp);
                જો (embeddedCovergroupVars.empty() && !classp->hasRandVarsUpdate()) {
                    putns(classp,
                          "VlClass* clone() const { return new " + className + "(*this); }\n");
                } નહીંતર {
                    putns(classp, "VlClass* clone() const { " + className + "* const clonep = new "
                                      + className + "(*this); ");
                    જો (classp->hasRandVarsUpdate()) {
                        અચલ string updateName = "__VnoInFunc___VupdateRandVars";
                        AstCFunc* અચલ updatep
                            = VN_AS(m_memberMap.findMember(classp, updateName), CFunc);
                        UASSERT_OBJ(updatep, classp, "Missing updateRandVars method");
                        puts("clonep->" + updatep->nameProtect() + "();\n");
                    }
                    માટે (અચલ EmbeddedCovergroupVar& item : embeddedCovergroupVars) {
                        puts("clonep->" + EmitCUtil::prefixNameProtect(item.first)
                             + "::" + item.second->nameProtect() + " = VlNull{}; ");
                    }
                    puts("return clonep; }\n");
                }
            }
        }
    }
    શૂન્ય emitEnums(અચલ Ast_ગાંઠ_Module* modp) {
        તાર્કિક first = સાચું;
        માટે (અચલ Ast_ગાંઠ* nodep = modp->stmtsp(); nodep; nodep = nodep->nextp()) {
            અચલ AstTypedef* અચલ tdefp = VN_CAST(nodep, Typedef);
            જો (!tdefp) ચાલુ;
            જો (!tdefp->attrPublic()) ચાલુ;
            અચલ AstEnumDType* અચલ edtypep
                = VN_CAST(tdefp->dtypep()->skipRefToEnump(), EnumDType);
            જો (!edtypep) ચાલુ;
            decorateFirst(first, "\n// ENUMS (that were declared public)\n");
            જો (edtypep->width() > 64) {
                putsDecoration(tdefp,
                               "// enum " + tdefp->nameProtect() + " ignored: Too wide for C++\n");
            } નહીંતર {
                putns(tdefp, "enum " + tdefp->name() + " {\n");
                માટે (અચલ AstEnumItem* itemp = edtypep->itemsp(); itemp;
                     itemp = VN_AS(itemp->nextp(), EnumItem)) {
                    જો (અચલ AstConst* અચલ constp = VN_CAST(itemp->valuep(), Const)) {
                        જો (constp->અંક().isFourState()) {
                            putns(itemp, "// " + itemp->nameProtect() + " is four-state\n");
                            ચાલુ;
                        }
                    }
                    putns(itemp, itemp->nameProtect());
                    puts(" = ");
                    putns(itemp, VN_AS(itemp->valuep(), Const)->અંક().emitC());
                    જો (VN_IS(itemp->nextp(), EnumItem)) puts(",");
                    puts("\n");
                }
                puts("};\n");
            }
        }
    }
    શૂન્ય emitStructDecl(અચલ Ast_ગાંઠ_Module* modp, Ast_ગાંઠ_UOr_Struct_DType* sdtypep,
                        std::set<Ast_ગાંઠ_UOr_Struct_DType*>& emitted) {
        જો (emitted.ગણતરી(sdtypep) > 0) પાછા;
        emitted.ઉમેરો(sdtypep);
        માટે (AstMemberDType* itemp = sdtypep->membersp(); itemp;
             itemp = VN_AS(itemp->nextp(), MemberDType)) {
            Ast_ગાંઠ_UOr_Struct_DType* અચલ subp = itemp->getChildStructp();
            જો (subp && (!subp->packed() || sdtypep->packed())) {
                // Recurse if it belongs to the current module
                જો (subp->classOrPackagep() == modp) {
                    emitStructDecl(modp, subp, emitted);
                    puts("\n");
                }
            }
        }
        જો (sdtypep->packed()) {
            emitPackedUOrSBody(sdtypep);
        } નહીંતર {
            emitUnpackedUOrSBody(sdtypep);
        }
    }
    ગણના વર્ગ AttributeType : uint8_t { WIDTH, DIMENSION };
    // Get member attribute based on type
    પૂર્ણાંક get_ગાંઠ_Attribute(અચલ AstMemberDType* itemp, AttributeType type) {
        અચલ તાર્કિક isArrayType = itemp->dtypep()->isNonPackedArray();
        પસંદગી (type) {
        સ્થિતિ AttributeType::WIDTH: {
            જો (isArrayType) {
                // For arrays, get innermost element width
                અચલ Ast_ગાંઠ_DType* dtype = itemp->dtypep();
                જ્યારે (dtype->subDTypep()) dtype = dtype->subDTypep();
                પાછા dtype->width();
            }
            પાછા itemp->width();
        }
        સ્થિતિ AttributeType::DIMENSION: {
            // Return array dimension or 0 for non-arrays
            પાછા isArrayType ? itemp->dtypep()->dimensions(સાચું).second : 0;
        }
        મૂળભૂત: {
            પાછા 0;
        }
        }
    }
    template <AttributeType T>
    શૂન્ય emitMemberVector(અચલ Ast_ગાંઠ_UOr_Struct_DType* sdtypep) {
        puts("return {");
        તાર્કિક needComma = ખોટું;
        માટે (અચલ AstMemberDType* itemp = sdtypep->membersp(); itemp;
             itemp = VN_AS(itemp->nextp(), MemberDType)) {
            જો (!itemp->isConstrainedRand()) ચાલુ;
            // Comma handling: add before element except first
            જો (needComma) puts(",\n");
            putns(itemp, std::to_string(get_ગાંઠ_Attribute(itemp, T)));
            needComma = સાચું;
        }
        puts("};\n}\n");
    }
    શૂન્ય emitUnpackedUOrSBody(Ast_ગાંઠ_UOr_Struct_DType* sdtypep) {
        putns(sdtypep, sdtypep->verilogKwd());  // "struct"/"union"
        puts(" " + EmitCUtil::prefixNameProtect(sdtypep) + " {\n");
        માટે (અચલ AstMemberDType* itemp = sdtypep->membersp(); itemp;
             itemp = VN_AS(itemp->nextp(), MemberDType)) {
            putns(itemp, itemp->dtypep()->cType(itemp->nameProtect(), ખોટું, ખોટું));
            puts(";\n");
        }

        // Three helper functions for struct constrained randomization:
        // - memberNames: Get member names
        // - getMembers: Access member references
        // - memberIndices: Retrieve member indices
        // - memberWidth: Retrieve member width
        // - memberDimension: Retrieve member dimension
        જો (sdtypep->isConstrainedRand()) {
            તાર્કિક needComma = ખોટું;
            putns(sdtypep, "\nstd::vector<std::string> memberNames(void) const {\n");
            puts("return {");
            માટે (અચલ AstMemberDType* itemp = sdtypep->membersp(); itemp;
                 itemp = VN_AS(itemp->nextp(), MemberDType)) {
                જો (!itemp->isConstrainedRand()) ચાલુ;
                જો (needComma) puts(",\n");
                putns(itemp, "\"" + itemp->shortName() + "\"");
                needComma = સાચું;
            }
            puts("};\n}\n");

            putns(sdtypep, "\nstd::vector<int> memberWidth(void) const {\n");
            emitMemberVector<AttributeType::WIDTH>(sdtypep);

            putns(sdtypep, "\nstd::vector<int> memberDimension(void) const {\n");
            emitMemberVector<AttributeType::DIMENSION>(sdtypep);

            needComma = ખોટું;
            putns(sdtypep, "\nauto memberIndices(void) const {\n");
            puts("return std::index_sequence_for<");
            માટે (અચલ AstMemberDType* itemp = sdtypep->membersp(); itemp;
                 itemp = VN_AS(itemp->nextp(), MemberDType)) {
                જો (!itemp->isConstrainedRand()) ચાલુ;
                જો (needComma) puts(",\n");
                putns(itemp, itemp->dtypep()->cType("", ખોટું, ખોટું));
                needComma = સાચું;
            }
            puts(">{};\n}\n");

            needComma = ખોટું;
            putns(sdtypep, "\ntemplate <typename T>");
            putns(sdtypep, "\nauto getMembers(T& obj) {\n");
            puts("return std::tie(");
            માટે (અચલ AstMemberDType* itemp = sdtypep->membersp(); itemp;
                 itemp = VN_AS(itemp->nextp(), MemberDType)) {
                જો (!itemp->isConstrainedRand()) ચાલુ;
                જો (needComma) puts(",\n");
                putns(itemp, "obj." + itemp->nameProtect());
                needComma = સાચું;
            }
            puts(");\n}\n");
        }
        putns(sdtypep, "\nbool operator==(const " + EmitCUtil::prefixNameProtect(sdtypep)
                           + "& rhs) const {\n");
        puts("return ");
        માટે (અચલ AstMemberDType* itemp = sdtypep->membersp(); itemp;
             itemp = VN_AS(itemp->nextp(), MemberDType)) {
            જો (itemp != sdtypep->membersp()) puts("\n    && ");
            putns(itemp, itemp->nameProtect() + " == " + "rhs." + itemp->nameProtect());
        }
        puts(";\n");
        puts("}\n");
        putns(sdtypep, "bool operator!=(const " + EmitCUtil::prefixNameProtect(sdtypep)
                           + "& rhs) const {\n");
        puts("return !(*this == rhs);\n}\n");
        putns(sdtypep, "\nbool operator<(const " + EmitCUtil::prefixNameProtect(sdtypep)
                           + "& rhs) const {\n");
        માટે (અચલ AstMemberDType* itemp = sdtypep->membersp(); itemp;
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
    શૂન્ય emitVlAssign(અચલ Ast_ગાંઠ_DType* અચલ lhstype, અચલ Ast_ગાંઠ_DType* rhstype,
                      અચલ std::string& off, અચલ std::string& lhsdata,
                      અચલ std::string& rhsdata, તાર્કિક getfunc) {
        puts(getfunc ? "VL_ASSIGNSEL_" : "VL_SELASSIGN_");
        puts(lhstype->charIQWN());
        puts(rhstype->charIQWN());
        puts("(" + std::to_string(lhstype->width()) + ", ");  // LHS width
        જો (getfunc) {
            puts(std::to_string(rhstype->width()) + ", ");  // Number of copy bits
            puts(off + ", ");  // LHS offset
        } નહીંતર {
            // Number of copy bits. Use widthTototalBytes to
            // make VL_SELASSIGN_XX clear upper unused bits for us.
            // puts(std::to_string(lhstype->width()) + ", ");
            puts(std::to_string(lhstype->widthTotalBytes() * 8) + ", ");
        }
        puts(lhsdata + ", ");  // LHS data
        puts(rhsdata);  // RHS data
        જો (!getfunc) {
            puts(", " + off);  // RHS offset
        }
        puts(");\n");
    }

    // `retOrArg` should be prefixed by `&` or suffixed by `.data()` depending on its type
    શૂન્ય emitPackedMember(અચલ Ast_ગાંઠ_DType* parentDtypep, અચલ Ast_ગાંઠ_DType* dtypep,
                          અચલ std::string& fieldname, અચલ std::string& offset, તાર્કિક getfunc,
                          અચલ std::string& retOrArg) {
        dtypep = dtypep->skipRefp();
        જો (અચલ auto* adtypep = VN_CAST(dtypep, PackArrayDType)) {
            અચલ std::string index = m_names.get("__Vi");
            puts("for (int " + index + " = 0; " + index + " < "
                 + std::to_string(adtypep->elementsConst()) + "; ++" + index + ") {\n");

            અચલ std::string offsetInLoop
                = offset + " + " + index + " * " + std::to_string(adtypep->subDTypep()->width());
            અચલ std::string newName = fieldname + "[" + index + "]";
            emitPackedMember(parentDtypep, adtypep->subDTypep(), newName, offsetInLoop, getfunc,
                             retOrArg);
            puts("}\n");
        } નહીંતર જો (VN_IS(dtypep, ગાંઠ_UOr_Struct_DType)) {
            અચલ std::string tmp = m_names.get("__Vtmp");
            જો (getfunc) {  // Emit `get` func;
                // auto __tmp = field.get();
                puts("auto " + tmp + " = " + fieldname + ".get();\n");
                // VL_ASSIGNSEL_XX(rbits, obits, lsb, lhsdata, rhsdata);
                emitVlAssign(parentDtypep, dtypep, offset, retOrArg, tmp, getfunc);
            } નહીંતર {  // Emit `set` func
                અચલ std::string tmptype = AstCDType::typeToHold(dtypep->width());
                // type tmp;
                puts(tmptype + " " + tmp + ";\n");
                // VL_SELASSIGN_XX(rbits, obits, lhsdata, rhsdata, roffset);
                emitVlAssign(dtypep, parentDtypep, offset, tmp, retOrArg, getfunc);
                // field.set(__tmp);
                puts(fieldname + ".set(" + tmp + ");\n");
            }
        } નહીંતર {
            UASSERT_OBJ(VN_IS(dtypep, EnumDType) || VN_IS(dtypep, BasicDType), dtypep,
                        "Unsupported type in packed struct or union");
            જો (getfunc) {  // Emit `get` func;
                // VL_ASSIGNSEL_XX(rbits, obits, lsb, lhsdata, rhsdata);
                emitVlAssign(parentDtypep, dtypep, offset, retOrArg, fieldname, getfunc);
            } નહીંતર {  // Emit `set` func
                // VL_SELASSIGN_XX(rbits, obits, lhsdata, rhsdata, roffset);
                emitVlAssign(dtypep, parentDtypep, offset, fieldname, retOrArg, getfunc);
            }
        }
    }
    શૂન્ય emitPackedUOrSBody(Ast_ગાંઠ_UOr_Struct_DType* sdtypep) {
        putns(sdtypep, sdtypep->verilogKwd());  // "struct"/"union"
        puts(" " + EmitCUtil::prefixNameProtect(sdtypep) + " {\n");

        AstMemberDType* itemp;
        AstMemberDType* lastItemp;
        અચલ AstMemberDType* witemp = nullptr;
        // LSB is first field in C, so loop backwards
        માટે (lastItemp = sdtypep->membersp(); lastItemp && lastItemp->nextp();
             lastItemp = VN_AS(lastItemp->nextp(), MemberDType)) {
            જો (lastItemp->width() == sdtypep->width()) witemp = lastItemp;
        }
        માટે (itemp = lastItemp; itemp; itemp = VN_CAST(itemp->backp(), MemberDType)) {
            putns(itemp, itemp->dtypep()->cType(itemp->nameProtect(), ખોટું, ખોટું, સાચું));
            puts(";\n");
        }

        અચલ std::string retArgName = m_names.get("__v");
        અચલ std::string retArgType = AstCDType::typeToHold(sdtypep->width());

        // Emit `get` member function
        puts(retArgType + " get() const {\n");
        puts(retArgType + " " + retArgName + ";\n");
        જો (VN_IS(sdtypep, StructDType)) {
            માટે (itemp = lastItemp; itemp; itemp = VN_CAST(itemp->backp(), MemberDType)) {
                emitPackedMember(sdtypep, itemp->dtypep(), itemp->nameProtect(),
                                 std::to_string(itemp->lsb()), /*getfunc=*/સાચું, retArgName);
            }
        } નહીંતર {
            // We only need to fill the widest field of union
            emitPackedMember(sdtypep, witemp->dtypep(), witemp->nameProtect(),
                             std::to_string(witemp->lsb()), /*getfunc=*/સાચું, retArgName);
        }
        puts("return " + retArgName + ";\n");
        puts("}\n");

        // Emit `set` member function
        puts("void set(const " + retArgType + "& " + retArgName + ") {\n");
        જો (VN_IS(sdtypep, StructDType)) {
            માટે (itemp = lastItemp; itemp; itemp = VN_CAST(itemp->backp(), MemberDType)) {
                emitPackedMember(sdtypep, itemp->dtypep(), itemp->nameProtect(),
                                 std::to_string(itemp->lsb()), /*getfunc=*/ખોટું, retArgName);
            }
        } નહીંતર {
            // We only need to fill the widest field of union
            emitPackedMember(sdtypep, witemp->dtypep(), witemp->nameProtect(),
                             std::to_string(witemp->lsb()), /*getfunc=*/ખોટું, retArgName);
        }

        puts("}\n");

        puts("};\n");
        m_names.reset();
    }
    શૂન્ય emitStructs(અચલ Ast_ગાંઠ_Module* modp) {
        // Track structs that've been emitted already
        std::set<Ast_ગાંઠ_UOr_Struct_DType*> emitted;
        માટે (અચલ Ast_ગાંઠ* nodep = modp->stmtsp(); nodep; nodep = nodep->nextp()) {
            અચલ AstTypedef* અચલ tdefp = VN_CAST(nodep, Typedef);
            જો (!tdefp) ચાલુ;
            Ast_ગાંઠ_UOr_Struct_DType* અચલ sdtypep
                = VN_CAST(tdefp->dtypep()->skipRefToEnump(), ગાંઠ_UOr_Struct_DType);
            જો (!sdtypep) ચાલુ;
            // V3Inline can copy one module into several parents. Only the module that
            // declares the struct emits it, so C++ sees only one definition.
            અચલ તાર્કિક declaredInOtherModule = sdtypep->classOrPackagep() != modp;
            જો (declaredInOtherModule) ચાલુ;
            emitStructDecl(modp, sdtypep, emitted);
        }
    }
    શૂન્ય emitFuncDecls(અચલ Ast_ગાંઠ_Module* modp, તાર્કિક inClassBody) {
        std::vector<અચલ AstCFunc*> funcsp;

        માટે (અચલ Ast_ગાંઠ* nodep = modp->stmtsp(); nodep; nodep = nodep->nextp()) {
            જો (અચલ AstCFunc* અચલ funcp = VN_CAST(nodep, CFunc)) {
                જો (funcp->dpiImportPrototype())  // Declared in __Dpi.h
                    ચાલુ;
                જો (funcp->dpiExportDispatcher())  // Declared in __Dpi.h
                    ચાલુ;
                જો (funcp->isMethod() != inClassBody)  // Only methods go inside class
                    ચાલુ;
                જો (funcp->isMethod() && funcp->isLoose())  // Loose methods are declared lazily
                    ચાલુ;
                funcsp.ધકેલો_back(funcp);
            }
        }

        std::stable_sort(funcsp.begin(), funcsp.end(), [](અચલ Ast_ગાંઠ* ap, અચલ Ast_ગાંઠ* bp) {
            પાછા ap->name() < bp->name();
        });

        માટે (અચલ AstCFunc* અચલ funcp : funcsp) {
            જો (inClassBody) ofp()->putsPrivate(funcp->declPrivate());
            emitCFuncDecl(funcp, modp);
        }
    }
    શૂન્ય emitAll(અચલ Ast_ગાંઠ_Module* modp) {
        // Include files required by this AstNodeModule
        જો (અચલ AstClass* અચલ classp = VN_CAST(modp, Class)) {
            માટે (અચલ AstClassExtends* extp = classp->extendsp(); extp;
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
        જો (!VN_IS(modp, Class)) puts("alignas(VL_CACHE_LINE_BYTES) ");
        puts(EmitCUtil::prefixNameProtect(modp));
        જો (અચલ AstClass* અચલ classp = VN_CAST(modp, Class)) {
            puts(" : ");
            જો (classp->extendsp()) {
                તાર્કિક needComma = ખોટું;
                માટે (અચલ AstClassExtends* extp = classp->extendsp(); extp;
                     extp = VN_AS(extp->nextp(), ClassExtends)) {
                    જો (needComma) puts(", ");
                    // Use virtual only for interfaces for class inheritance
                    // (extends)
                    puts(extp->classp()->useVirtualPublic() ? "virtual public " : "public ");
                    putns(extp, EmitCUtil::prefixNameProtect(extp->classp()));
                    needComma = સાચું;
                }
            } નહીંતર {
                puts("public virtual VlClass");
            }
        } નહીંતર {
            puts(" final");
        }
        puts(" {\n");
        ofp()->resetPrivate();
        ofp()->putsPrivate(ખોટું);  // public:

        // Emit all class body contents
        emitCellDecls(modp);
        emitEnums(modp);
        emitDesignVarDecls(modp);
        emitInternalVarDecls(modp);
        emitParamDecls(modp);
        emitCtorDtorDecls(modp);
        emitInternalMethodDecls(modp);
        emitFuncDecls(modp, /* inClassBody: */ સાચું);

        // From `systemc_interface
        emitSystemCSection(modp, VSystemCSectionType::INT);

        // Close class body
        puts("};\n");
        // }}}

        // Emit out of class function declarations
        puts("\n");
        emitFuncDecls(modp, /* inClassBody: */ ખોટું);
        emitSystemCSection(modp, VSystemCSectionType::HDR_POST);
    }

    // VISITORS
    શૂન્ય visit(Ast_ગાંઠ* nodep) override { nodep->v3fatalSrc("Unused"); }  // LCOV_EXCL_LINE

    explicit EmitCHeader(અચલ Ast_ગાંઠ_Module* modp) {
        UINFO(5, "  Emitting header for " << EmitCUtil::prefixNameProtect(modp));

        // Open output file
        openNewOutputHeaderFile(EmitCUtil::prefixNameProtect(modp), "Design internal header");
        puts("// See " + EmitCUtil::ટોચ_Class_Name() + ".h for the primary calling header\n");

        ofp()->putsGuard();

        // Include files
        puts("\n");
        ofp()->puts_Int_ટોચ_Include();
        puts("#include \"verilated.h\"\n");
        જો (modp->is_ટોચ() && optSystemC()) puts("#include \"verilated_sc.h\"\n");
        જો (v3Global.opt.mtasks()) puts("#include \"verilated_threads.h\"\n");
        જો (v3Global.opt.savable()) puts("#include \"verilated_save.h\"\n");
        જો (v3Global.opt.coverage()) puts("#include \"verilated_cov.h\"\n");
        જો (v3Global.opt.coverage() || v3Global.useCovergroup())
            puts("#include \"verilated_covergroup.h\"\n");
        જો (v3Global.usesTiming()) puts("#include \"verilated_timing.h\"\n");
        જો (v3Global.useRandomizeMethods()) puts("#include \"verilated_random.h\"\n");
        જો (v3Global.usesForce()) puts("#include \"verilated_force.h\"\n");

        std::set<string> cuse_set;
        auto add_to_cuse_set = [&](string s) { cuse_set.ઉમેરો(s); };

        forModCUse(modp, VUseType::INT_FWD_CLASS | VUseType::INT_INCLUDE, add_to_cuse_set);
        જો (અચલ AstClassPackage* અચલ packagep = VN_CAST(modp, ClassPackage)) {
            forModCUse(packagep->classp(), VUseType::INT_INCLUDE | VUseType::INT_FWD_CLASS,
                       add_to_cuse_set);
        }

        માટે (અચલ string& s : cuse_set) puts(s);
        puts("\n");

        emitAll(modp);

        જો (અચલ AstClassPackage* અચલ packagep = VN_CAST(modp, ClassPackage)) {
            // Put the non-static class implementation in same h file for speed
            emitAll(packagep->classp());
        }

        ofp()->putsEndGuard();

        // Close output file
        closeOutputFile();
    }
    ~EmitCHeader() override = મૂળભૂત;

જાહેર:
    સ્થિર શૂન્ય મુખ્ય(અચલ Ast_ગાંઠ_Module* modp) { અચલ EmitCHeader emitCHeader{modp}; }
};

//######################################################################
// EmitC class functions

શૂન્ય V3EmitC::emitcHeaders() {
    UINFO(2, __FUNCTION__ << ":");

    // Process each module in turn
    માટે (અચલ Ast_ગાંઠ* nodep = v3Global.rootp()->modulesp(); nodep; nodep = nodep->nextp()) {
        જો (VN_IS(nodep, Class)) ચાલુ;  // Declared with the ClassPackage
        અચલ Ast_ગાંઠ_Module* અચલ modp = VN_AS(nodep, ગાંઠ_Module);
        જો (modp->isConstPool()) ચાલુ;  // Emitted by V3EmitCConstPool
        EmitCHeader::મુખ્ય(modp);
    }
}
