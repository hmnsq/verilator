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

#include "V3PchAstMT.h"

#include "V3EmitC.h"
#include "V3EmitCBase.h"
#include "V3File.h"
#include "V3MemberMap.h"
#include "V3UniqueNames.h"

#include <algorithm>
#include <cstdint>
#include <set>
#include <string>
#include <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Internal EmitC implementation

class EmitCHeader final : public EmitCBaseVisitorConst {
    V3UniqueNames m_names;
    VMemberMap m_memberMap;
    // METHODS

    class CoverCountVisitor final : public VNVisitorConst {
        පූර්ණ_සංඛ්‍යා m_bins = 0;  // Running total of coverage bins counted so far

        void visit(AstNodeCoverDecl* nodep) override {
            // Each module class owns the counters for declarations it emits;
            // duplicate no-inline instances need separate arrays for
            // forcePerInstance reporting.
            නම් (!nodep->dataDeclNullp()) m_bins += nodep->size();
        }
        void visit(AstNode* nodep) override { iterateChildrenConst(nodep); }

    public:
        explicit CoverCountVisitor(const AstNodeModule* modp) {
            iterateConst(const_cast<AstNodeModule*>(modp));
        }
        පූර්ණ_සංඛ්‍යා bins() const { ආපසු m_bins; }
    };

    void decorateFirst(bool& first, const string& str) {
        නම් (first) {
            putsDecoration(nullptr, str);
            first = false;
        }
    }
    void emitCellDecls(const AstNodeModule* modp) {
        bool first = true;
        සඳහා (const AstNode* nodep = modp->stmtsp(); nodep; nodep = nodep->nextp()) {
            නම් (const AstCell* const cellp = VN_CAST(nodep, Cell)) {
                නම් (cellp->modp()->isConstPool()) continue;  // Special emit rules
                decorateFirst(first, "// CELLS\n");
                putns(cellp, EmitCUtil::prefixNameProtect(cellp->modp()) + "* "
                                 + cellp->nameProtect() + ";\n");
            }
        }
    }
    void emitDesignVarDecls(const AstNodeModule* modp) {
        // V3VariableOrder puts all variables at the front of the list: top level ports, then
        // trivially constructible variables, then the not trivially constructible ones, then
        // those that are not model state
        const AstVar* portp = nullptr;
        const AstVar* trivp = nullptr;
        const AstVar* nontp = nullptr;
        size_t nPort = 0;
        size_t nTriv = 0;
        size_t nNont = 0;
        සඳහා (const AstNode* nodep = modp->stmtsp(); nodep; nodep = nodep->nextp()) {
            const AstVar* const varp = VN_CAST(nodep, Var);
            නම් (!varp) bha_nga;
            නම් (!varp->isModelState()) {
                // Cache line alignment is only effective on emitted fields
                UASSERT_OBJ(!varp->mtaskCacheLineAlign(), varp, "Aligned non-field");
            } නැතහොත් නම් (varp->isPrimaryIO()) {
                නම් (!portp) portp = varp;
                ++nPort;
            } නැතහොත් නම් (varp->isTriviallyConstructible()) {
                නම් (!trivp) trivp = varp;
                ++nTriv;
            } නැතහොත් {
                නම් (!nontp) nontp = varp;
                ++nNont;
            }
        }

        const auto emitAndStep = [&](const AstVar*& varpr) {
            emitVarDecl(varpr);
            varpr = VN_CAST(varpr->nextp(), Var);
        };

        // Emit primary ports
        නම් (nPort) {
            putsDecoration(nullptr, "\n// TOP LEVEL PORTS\n");
            // In anonymous union to prevent the C++ compiler from generating
            // constructor/destructor calls
            puts("union {\nstruct {\n");
            සඳහා (size_t i = 0; i < nPort; ++i) emitAndStep(portp);
            puts("} __VprimaryPorts;\n};\n");
        }

        // Emit trivially constructible variables
        නම් (nTriv) {
            putsDecoration(nullptr, "\n// TRIVIALLY CONSTRUCTIBLE STATE\n");
            නම් (v3Global.opt.compLimitMembers() == 0) {
                // Anonymous structs disabled, so just emit them directly
                සඳහා (size_t i = 0; i < nTriv; ++i) emitAndStep(trivp);
            } නැතහොත් {
                const size_t lim = v3Global.opt.compLimitMembers();
                size_t anonL3s = 1;
                size_t anonL2s = 1;
                size_t anonL1s = 1;
                නම් (nTriv > (lim * lim * lim)) {
                    anonL3s = (nTriv + (lim * lim * lim) - 1) / (lim * lim * lim);
                    anonL2s = lim;
                    anonL1s = lim;
                } නැතහොත් නම් (nTriv > (lim * lim)) {
                    anonL2s = (nTriv + (lim * lim) - 1) / (lim * lim);
                    anonL1s = lim;
                } නැතහොත් නම් (nTriv > lim) {
                    anonL1s = (nTriv + lim - 1) / lim;
                }
                නම් (anonL1s != 1) {
                    putsDecoration(
                        nullptr,
                        "// Anonymous structures to workaround compiler member-count bugs\n");
                }
                size_t nEmitted = 0;
                සඳහා (size_t l3 = 0; l3 < anonL3s && nEmitted < nTriv; ++l3) {
                    නම් (anonL3s != 1) puts("struct {\n");
                    සඳහා (size_t l2 = 0; l2 < anonL2s && nEmitted < nTriv; ++l2) {
                        නම් (anonL2s != 1) puts("struct {\n");
                        සඳහා (size_t l1 = 0; l1 < anonL1s && nEmitted < nTriv; ++l1) {
                            නම් (anonL1s != 1) puts("struct {\n");
                            සඳහා (size_t l0 = 0; l0 < lim && nEmitted < nTriv; ++l0, ++nEmitted) {
                                emitAndStep(trivp);
                            }
                            නම් (anonL1s != 1) puts("};\n");
                        }
                        නම් (anonL2s != 1) puts("};\n");
                    }
                    නම් (anonL3s != 1) puts("};\n");
                }
                UASSERT_OBJ(nEmitted == nTriv, modp, "Not all variables emitted");
            }
        }

        // Emit not trivially constructible variables
        නම් (nNont) {
            putsDecoration(nullptr, "\n// NOT TRIVIALLY CONSTRUCTIBLE STATE\n");
            // Classes use real constructors
            නම් (VN_IS(modp, Class)) {
                සඳහා (size_t i = 0; i < nNont; ++i) emitAndStep(nontp);
                ආපසු;
            }
            // Output under an anonymous union to prevent the C++ compiler from generating
            // constructor/destructor calls for them. They are managed explicitly in ctor/dtor.
            // While 'union { singleVar; }' for each variable would be simpler, it blows up
            // clang's compile time when using pre-compiled headers, joy.
            puts("union {\nstruct {\n");
            සඳහා (size_t i = 0; i < nNont; ++i) emitAndStep(nontp);
            puts("} __Vnontrivs;\n};\n");
        }
    }
    void emitInternalVarDecls(const AstNodeModule* modp) {
        නම් (const AstClass* const classp = VN_CAST(modp, Class)) {
            නම් (classp->needRNG()) {
                putsDecoration(nullptr, "\n// INTERNAL VARIABLES\n");
                puts("VlRNGReseeds __Vm_rng;\n");
            }
        } නැතහොත් {  // not class
            putsDecoration(nullptr, "\n// INTERNAL VARIABLES\n");
            puts(EmitCUtil::symClassName() + "* vlSymsp;\n");
            puts("const char* vlNamep;\n");
            // Allocate object-local coverage counters only on modules that
            // contain emitted coverage declarations.
            const පූර්ණ_සංඛ්‍යා coverBins = CoverCountVisitor{modp}.bins();
            නම් (coverBins) {
                puts(v3Global.opt.threads() > 1 ? "std::atomic<uint32_t>" : "uint32_t");
                puts(" __Vcoverage[");
                puts(std::to_string(coverBins));
                puts("]{};\n");
            }
        }
    }
    void emitParamDecls(const AstNodeModule* modp) {
        bool first = true;
        සඳහා (AstNode* nodep = modp->stmtsp(); nodep; nodep = nodep->nextp()) {
            නම් (const AstVar* const varp = VN_CAST(nodep, Var)) {
                නම් (varp->isParam()) {
                    decorateFirst(first, "\n// PARAMETERS\n");
                    putns(varp, "static const ");
                    puts(varp->dtypep()->cType(varp->nameProtect(), false, false));
                    puts(";\n");
                }
            }
        }
    }
    void emitCtorDtorDecls(const AstNodeModule* modp) {
        // Classes use CFuncs with isConstructor/isDestructor
        නම් (VN_IS(modp, Class)) ආපසු;

        // Proper modules use loose split ctor/dtor and are not copyable/movable
        const std::string name = EmitCUtil::prefixNameProtect(modp);
        putsDecoration(nullptr, "\n// CONSTRUCTORS\n");
        putns(modp, name + "(" + EmitCUtil::symClassName() + "* symsp, const char* namep);\n");
        putns(modp, "~" + name + "();\n");
        putns(modp, "VL_UNCOPYABLE(" + name + ");\n");
        putns(modp, "VL_UNMOVABLE(" + name + ");\n");
    }
    void emitInternalMethodDecls(const AstNodeModule* modp) {
        bool first = true;
        const string section = "\n// INTERNAL METHODS\n";

        නම් (!VN_IS(modp, Class)) {
            decorateFirst(first, section);
            නම් (v3Global.opt.coverage()) {
                puts("void " + protect("__Vconfigure") + "(bool first);\n");
            }
        } නැතහොත් {
            decorateFirst(first, section);
            const std::string name = V3OutFormatter::quoteNameControls(
                VIdProtect::protectWordsIf(modp->prettyName(), v3Global.opt.protectIds()));
            // "override", but don't want clang to start checking them:
            puts("virtual const char* typeName() const { return \""s + name + "\"; }\n");
        }

        නම් (v3Global.opt.coverage() && !VN_IS(modp, Class)) {
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

        නම් (v3Global.opt.coverageToggle() && !VN_IS(modp, Class)) {
            decorateFirst(first, section);
            puts("void __vlCoverToggleInsert(int begin, int end, bool ranged, ");
            puts(v3Global.opt.threads() > 1 ? "std::atomic<uint32_t>" : "uint32_t");
            puts("* countp, bool enable, bool localCounter, const char* filenamep, int lineno, "
                 "int column,\n");
            puts("const char* hierp, const char* pagep, const char* commentp);\n");
        }

        නම් (v3Global.opt.savable()) {
            decorateFirst(first, section);
            puts("void " + protect("__Vserialize") + "(VerilatedSerialize& os);\n");
            puts("void " + protect("__Vdeserialize") + "(VerilatedDeserialize& os);\n");
        }

        // Polymorphic clone for concrete (non-abstract, non-interface) classes
        නම් (const AstClass* const classp = VN_CAST(modp, Class)) {
            නම් (!classp->isInterfaceClass() && !classp->isVirtual()) {
                decorateFirst(first, section);
                using EmbeddedCovergroupVar = std::pair<const AstClass*, const AstVar*>;
                std::vector<EmbeddedCovergroupVar> embeddedCovergroupVars;
                const auto hasEnclosingBackPointer = [](const AstClass* covergroupp) {
                    ආපසු covergroupp->exists([](const AstVar* const varp) {
                        const AstClassRefDType* const refp
                            = VN_CAST(varp->dtypep()->skipRefp(), ClassRefDType);
                        ආපසු refp && refp->rawPointer();
                    });
                };
                const_cast<AstClass*>(classp)->foreachMember(
                    [&](AstClass* const memberClassp, AstVar* const varp) {
                        const AstClassRefDType* const refp
                            = VN_CAST(varp->dtypep()->skipRefp(), ClassRefDType);
                        නම් (refp && refp->classp()->isCovergroup()
                            && hasEnclosingBackPointer(refp->classp())) {
                            embeddedCovergroupVars.emplace_back(memberClassp, varp);
                        }
                    });
                const string className = EmitCUtil::prefixNameProtect(classp);
                නම් (embeddedCovergroupVars.empty() && !classp->hasRandVarsUpdate()) {
                    putns(classp,
                          "VlClass* clone() const { return new " + className + "(*this); }\n");
                } නැතහොත් {
                    putns(classp, "VlClass* clone() const { " + className + "* const clonep = new "
                                      + className + "(*this); ");
                    නම් (classp->hasRandVarsUpdate()) {
                        const string updateName = "__VnoInFunc___VupdateRandVars";
                        AstCFunc* const updatep
                            = VN_AS(m_memberMap.findMember(classp, updateName), CFunc);
                        UASSERT_OBJ(updatep, classp, "Missing updateRandVars method");
                        puts("clonep->" + updatep->nameProtect() + "();\n");
                    }
                    සඳහා (const EmbeddedCovergroupVar& item : embeddedCovergroupVars) {
                        puts("clonep->" + EmitCUtil::prefixNameProtect(item.first)
                             + "::" + item.second->nameProtect() + " = VlNull{}; ");
                    }
                    puts("return clonep; }\n");
                }
            }
        }
    }
    void emitEnums(const AstNodeModule* modp) {
        bool first = true;
        සඳහා (const AstNode* nodep = modp->stmtsp(); nodep; nodep = nodep->nextp()) {
            const AstTypedef* const tdefp = VN_CAST(nodep, Typedef);
            නම් (!tdefp) continue;
            නම් (!tdefp->attrPublic()) continue;
            const AstEnumDType* const edtypep
                = VN_CAST(tdefp->dtypep()->skipRefToEnump(), EnumDType);
            නම් (!edtypep) continue;
            decorateFirst(first, "\n// ENUMS (that were declared public)\n");
            නම් (edtypep->width() > 64) {
                putsDecoration(tdefp,
                               "// enum " + tdefp->nameProtect() + " ignored: Too wide for C++\n");
            } නැතහොත් {
                putns(tdefp, "enum " + tdefp->name() + " {\n");
                සඳහා (const AstEnumItem* itemp = edtypep->itemsp(); itemp;
                     itemp = VN_AS(itemp->nextp(), EnumItem)) {
                    නම් (const AstConst* const constp = VN_CAST(itemp->valuep(), Const)) {
                        නම් (constp->num().isFourState()) {
                            putns(itemp, "// " + itemp->nameProtect() + " is four-state\n");
                            continue;
                        }
                    }
                    putns(itemp, itemp->nameProtect());
                    puts(" = ");
                    putns(itemp, VN_AS(itemp->valuep(), Const)->num().emitC());
                    නම් (VN_IS(itemp->nextp(), EnumItem)) puts(",");
                    puts("\n");
                }
                puts("};\n");
            }
        }
    }
    void emitStructDecl(const AstNodeModule* modp, AstNodeUOrStructDType* sdtypep,
                        std::set<AstNodeUOrStructDType*>& emitted) {
        නම් (emitted.count(sdtypep) > 0) ආපසු;
        emitted.insert(sdtypep);
        සඳහා (AstMemberDType* itemp = sdtypep->membersp(); itemp;
             itemp = VN_AS(itemp->nextp(), MemberDType)) {
            AstNodeUOrStructDType* const subp = itemp->getChildStructp();
            නම් (subp && (!subp->packed() || sdtypep->packed())) {
                // Recurse if it belongs to the current module
                නම් (subp->classOrPackagep() == modp) {
                    emitStructDecl(modp, subp, emitted);
                    puts("\n");
                }
            }
        }
        නම් (sdtypep->packed()) {
            emitPackedUOrSBody(sdtypep);
        } නැතහොත් {
            emitUnpackedUOrSBody(sdtypep);
        }
    }
    ga_nanaa class AttributeType : uint8_t { WIDTH, DIMENSION };
    // Get member attribute based on type
    පූර්ණ_සංඛ්‍යා getNodeAttribute(const AstMemberDType* itemp, AttributeType type) {
        const bool isArrayType = itemp->dtypep()->isNonPackedArray();
        switch (type) {
        pak_sa AttributeType::WIDTH: {
            නම් (isArrayType) {
                // For arrays, get innermost element width
                const AstNodeDType* dtype = itemp->dtypep();
                අතරතුර (dtype->subDTypep()) dtype = dtype->subDTypep();
                ආපසු dtype->width();
            }
            ආපසු itemp->width();
        }
        pak_sa AttributeType::DIMENSION: {
            // Return array dimension or 0 for non-arrays
            ආපසු isArrayType ? itemp->dtypep()->dimensions(true).second : 0;
        }
        default: {
            ආපසු 0;
        }
        }
    }
    template <AttributeType T>
    void emitMemberVector(const AstNodeUOrStructDType* sdtypep) {
        puts("return {");
        bool needComma = false;
        සඳහා (const AstMemberDType* itemp = sdtypep->membersp(); itemp;
             itemp = VN_AS(itemp->nextp(), MemberDType)) {
            නම් (!itemp->isConstrainedRand()) continue;
            // Comma handling: add before element except first
            නම් (needComma) puts(",\n");
            putns(itemp, std::to_string(getNodeAttribute(itemp, T)));
            needComma = true;
        }
        puts("};\n}\n");
    }
    void emitUnpackedUOrSBody(AstNodeUOrStructDType* sdtypep) {
        putns(sdtypep, sdtypep->verilogKwd());  // "struct"/"union"
        puts(" " + EmitCUtil::prefixNameProtect(sdtypep) + " {\n");
        සඳහා (const AstMemberDType* itemp = sdtypep->membersp(); itemp;
             itemp = VN_AS(itemp->nextp(), MemberDType)) {
            putns(itemp, itemp->dtypep()->cType(itemp->nameProtect(), false, false));
            puts(";\n");
        }

        // Three helper functions for struct constrained randomization:
        // - memberNames: Get member names
        // - getMembers: Access member references
        // - memberIndices: Retrieve member indices
        // - memberWidth: Retrieve member width
        // - memberDimension: Retrieve member dimension
        නම් (sdtypep->isConstrainedRand()) {
            bool needComma = false;
            putns(sdtypep, "\nstd::vector<std::string> memberNames(void) const {\n");
            puts("return {");
            සඳහා (const AstMemberDType* itemp = sdtypep->membersp(); itemp;
                 itemp = VN_AS(itemp->nextp(), MemberDType)) {
                නම් (!itemp->isConstrainedRand()) continue;
                නම් (needComma) puts(",\n");
                putns(itemp, "\"" + itemp->shortName() + "\"");
                needComma = true;
            }
            puts("};\n}\n");

            putns(sdtypep, "\nstd::vector<int> memberWidth(void) const {\n");
            emitMemberVector<AttributeType::WIDTH>(sdtypep);

            putns(sdtypep, "\nstd::vector<int> memberDimension(void) const {\n");
            emitMemberVector<AttributeType::DIMENSION>(sdtypep);

            needComma = false;
            putns(sdtypep, "\nauto memberIndices(void) const {\n");
            puts("return std::index_sequence_for<");
            සඳහා (const AstMemberDType* itemp = sdtypep->membersp(); itemp;
                 itemp = VN_AS(itemp->nextp(), MemberDType)) {
                නම් (!itemp->isConstrainedRand()) continue;
                නම් (needComma) puts(",\n");
                putns(itemp, itemp->dtypep()->cType("", false, false));
                needComma = true;
            }
            puts(">{};\n}\n");

            needComma = false;
            putns(sdtypep, "\ntemplate <typename T>");
            putns(sdtypep, "\nauto getMembers(T& obj) {\n");
            puts("return std::tie(");
            සඳහා (const AstMemberDType* itemp = sdtypep->membersp(); itemp;
                 itemp = VN_AS(itemp->nextp(), MemberDType)) {
                නම් (!itemp->isConstrainedRand()) continue;
                නම් (needComma) puts(",\n");
                putns(itemp, "obj." + itemp->nameProtect());
                needComma = true;
            }
            puts(");\n}\n");
        }
        putns(sdtypep, "\nbool operator==(const " + EmitCUtil::prefixNameProtect(sdtypep)
                           + "& rhs) const {\n");
        puts("return ");
        සඳහා (const AstMemberDType* itemp = sdtypep->membersp(); itemp;
             itemp = VN_AS(itemp->nextp(), MemberDType)) {
            නම් (itemp != sdtypep->membersp()) puts("\n    && ");
            putns(itemp, itemp->nameProtect() + " == " + "rhs." + itemp->nameProtect());
        }
        puts(";\n");
        puts("}\n");
        putns(sdtypep, "bool operator!=(const " + EmitCUtil::prefixNameProtect(sdtypep)
                           + "& rhs) const {\n");
        puts("return !(*this == rhs);\n}\n");
        putns(sdtypep, "\nbool operator<(const " + EmitCUtil::prefixNameProtect(sdtypep)
                           + "& rhs) const {\n");
        සඳහා (const AstMemberDType* itemp = sdtypep->membersp(); itemp;
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
    void emitVlAssign(const AstNodeDType* const lhstype, const AstNodeDType* rhstype,
                      const std::string& off, const std::string& lhsdata,
                      const std::string& rhsdata, bool getfunc) {
        puts(getfunc ? "VL_ASSIGNSEL_" : "VL_SELASSIGN_");
        puts(lhstype->charIQWN());
        puts(rhstype->charIQWN());
        puts("(" + std::to_string(lhstype->width()) + ", ");  // LHS width
        නම් (getfunc) {
            puts(std::to_string(rhstype->width()) + ", ");  // Number of copy bits
            puts(off + ", ");  // LHS offset
        } නැතහොත් {
            // Number of copy bits. Use widthTototalBytes to
            // make VL_SELASSIGN_XX clear upper unused bits for us.
            // puts(std::to_string(lhstype->width()) + ", ");
            puts(std::to_string(lhstype->widthTotalBytes() * 8) + ", ");
        }
        puts(lhsdata + ", ");  // LHS data
        puts(rhsdata);  // RHS data
        නම් (!getfunc) {
            puts(", " + off);  // RHS offset
        }
        puts(");\n");
    }

    // `retOrArg` should be prefixed by `&` or suffixed by `.data()` depending on its type
    void emitPackedMember(const AstNodeDType* parentDtypep, const AstNodeDType* dtypep,
                          const std::string& fieldname, const std::string& offset, bool getfunc,
                          const std::string& retOrArg) {
        dtypep = dtypep->skipRefp();
        නම් (const auto* adtypep = VN_CAST(dtypep, PackArrayDType)) {
            const std::string index = m_names.get("__Vi");
            puts("for (int " + index + " = 0; " + index + " < "
                 + std::to_string(adtypep->elementsConst()) + "; ++" + index + ") {\n");

            const std::string offsetInLoop
                = offset + " + " + index + " * " + std::to_string(adtypep->subDTypep()->width());
            const std::string newName = fieldname + "[" + index + "]";
            emitPackedMember(parentDtypep, adtypep->subDTypep(), newName, offsetInLoop, getfunc,
                             retOrArg);
            puts("}\n");
        } නැතහොත් නම් (VN_IS(dtypep, NodeUOrStructDType)) {
            const std::string tmp = m_names.get("__Vtmp");
            නම් (getfunc) {  // Emit `get` func;
                // auto __tmp = field.get();
                puts("auto " + tmp + " = " + fieldname + ".get();\n");
                // VL_ASSIGNSEL_XX(rbits, obits, lsb, lhsdata, rhsdata);
                emitVlAssign(parentDtypep, dtypep, offset, retOrArg, tmp, getfunc);
            } නැතහොත් {  // Emit `set` func
                const std::string tmptype = AstCDType::typeToHold(dtypep->width());
                // type tmp;
                puts(tmptype + " " + tmp + ";\n");
                // VL_SELASSIGN_XX(rbits, obits, lhsdata, rhsdata, roffset);
                emitVlAssign(dtypep, parentDtypep, offset, tmp, retOrArg, getfunc);
                // field.set(__tmp);
                puts(fieldname + ".set(" + tmp + ");\n");
            }
        } නැතහොත් {
            UASSERT_OBJ(VN_IS(dtypep, EnumDType) || VN_IS(dtypep, BasicDType), dtypep,
                        "Unsupported type in packed struct or union");
            නම් (getfunc) {  // Emit `get` func;
                // VL_ASSIGNSEL_XX(rbits, obits, lsb, lhsdata, rhsdata);
                emitVlAssign(parentDtypep, dtypep, offset, retOrArg, fieldname, getfunc);
            } නැතහොත් {  // Emit `set` func
                // VL_SELASSIGN_XX(rbits, obits, lhsdata, rhsdata, roffset);
                emitVlAssign(dtypep, parentDtypep, offset, fieldname, retOrArg, getfunc);
            }
        }
    }
    void emitPackedUOrSBody(AstNodeUOrStructDType* sdtypep) {
        putns(sdtypep, sdtypep->verilogKwd());  // "struct"/"union"
        puts(" " + EmitCUtil::prefixNameProtect(sdtypep) + " {\n");

        AstMemberDType* itemp;
        AstMemberDType* lastItemp;
        const AstMemberDType* witemp = nullptr;
        // LSB is first field in C, so loop backwards
        සඳහා (lastItemp = sdtypep->membersp(); lastItemp && lastItemp->nextp();
             lastItemp = VN_AS(lastItemp->nextp(), MemberDType)) {
            නම් (lastItemp->width() == sdtypep->width()) witemp = lastItemp;
        }
        සඳහා (itemp = lastItemp; itemp; itemp = VN_CAST(itemp->backp(), MemberDType)) {
            putns(itemp, itemp->dtypep()->cType(itemp->nameProtect(), false, false, true));
            puts(";\n");
        }

        const std::string retArgName = m_names.get("__v");
        const std::string retArgType = AstCDType::typeToHold(sdtypep->width());

        // Emit `get` member function
        puts(retArgType + " get() const {\n");
        puts(retArgType + " " + retArgName + ";\n");
        නම් (VN_IS(sdtypep, StructDType)) {
            සඳහා (itemp = lastItemp; itemp; itemp = VN_CAST(itemp->backp(), MemberDType)) {
                emitPackedMember(sdtypep, itemp->dtypep(), itemp->nameProtect(),
                                 std::to_string(itemp->lsb()), /*getfunc=*/true, retArgName);
            }
        } නැතහොත් {
            // We only need to fill the widest field of union
            emitPackedMember(sdtypep, witemp->dtypep(), witemp->nameProtect(),
                             std::to_string(witemp->lsb()), /*getfunc=*/true, retArgName);
        }
        puts("return " + retArgName + ";\n");
        puts("}\n");

        // Emit `set` member function
        puts("void set(const " + retArgType + "& " + retArgName + ") {\n");
        නම් (VN_IS(sdtypep, StructDType)) {
            සඳහා (itemp = lastItemp; itemp; itemp = VN_CAST(itemp->backp(), MemberDType)) {
                emitPackedMember(sdtypep, itemp->dtypep(), itemp->nameProtect(),
                                 std::to_string(itemp->lsb()), /*getfunc=*/false, retArgName);
            }
        } නැතහොත් {
            // We only need to fill the widest field of union
            emitPackedMember(sdtypep, witemp->dtypep(), witemp->nameProtect(),
                             std::to_string(witemp->lsb()), /*getfunc=*/false, retArgName);
        }

        puts("}\n");

        puts("};\n");
        m_names.reset();
    }
    void emitStructs(const AstNodeModule* modp) {
        // Track structs that've been emitted already
        std::set<AstNodeUOrStructDType*> emitted;
        සඳහා (const AstNode* nodep = modp->stmtsp(); nodep; nodep = nodep->nextp()) {
            const AstTypedef* const tdefp = VN_CAST(nodep, Typedef);
            නම් (!tdefp) continue;
            AstNodeUOrStructDType* const sdtypep
                = VN_CAST(tdefp->dtypep()->skipRefToEnump(), NodeUOrStructDType);
            නම් (!sdtypep) continue;
            // V3Inline can copy one module into several parents. Only the module that
            // declares the struct emits it, so C++ sees only one definition.
            const bool declaredInOtherModule = sdtypep->classOrPackagep() != modp;
            නම් (declaredInOtherModule) continue;
            emitStructDecl(modp, sdtypep, emitted);
        }
    }
    void emitFuncDecls(const AstNodeModule* modp, bool inClassBody) {
        std::vector<const AstCFunc*> funcsp;

        සඳහා (const AstNode* nodep = modp->stmtsp(); nodep; nodep = nodep->nextp()) {
            නම් (const AstCFunc* const funcp = VN_CAST(nodep, CFunc)) {
                නම් (funcp->dpiImportPrototype())  // Declared in __Dpi.h
                    continue;
                නම් (funcp->dpiExportDispatcher())  // Declared in __Dpi.h
                    continue;
                නම් (funcp->isMethod() != inClassBody)  // Only methods go inside class
                    continue;
                නම් (funcp->isMethod() && funcp->isLoose())  // Loose methods are declared lazily
                    continue;
                funcsp.push_back(funcp);
            }
        }

        std::stable_sort(funcsp.begin(), funcsp.end(), [](const AstNode* ap, const AstNode* bp) {
            ආපසු ap->name() < bp->name();
        });

        සඳහා (const AstCFunc* const funcp : funcsp) {
            නම් (inClassBody) ofp()->putsPrivate(funcp->declPrivate());
            emitCFuncDecl(funcp, modp);
        }
    }
    void emitAll(const AstNodeModule* modp) {
        // Include files required by this AstNodeModule
        නම් (const AstClass* const classp = VN_CAST(modp, Class)) {
            සඳහා (const AstClassExtends* extp = classp->extendsp(); extp;
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
        නම් (!VN_IS(modp, Class)) puts("alignas(VL_CACHE_LINE_BYTES) ");
        puts(EmitCUtil::prefixNameProtect(modp));
        නම් (const AstClass* const classp = VN_CAST(modp, Class)) {
            puts(" : ");
            නම් (classp->extendsp()) {
                bool needComma = false;
                සඳහා (const AstClassExtends* extp = classp->extendsp(); extp;
                     extp = VN_AS(extp->nextp(), ClassExtends)) {
                    නම් (needComma) puts(", ");
                    // Use virtual only for interfaces for class inheritance
                    // (extends)
                    puts(extp->classp()->useVirtualPublic() ? "virtual public " : "public ");
                    putns(extp, EmitCUtil::prefixNameProtect(extp->classp()));
                    needComma = true;
                }
            } නැතහොත් {
                puts("public virtual VlClass");
            }
        } නැතහොත් {
            puts(" final");
        }
        puts(" {\n");
        ofp()->resetPrivate();
        ofp()->putsPrivate(false);  // public:

        // Emit all class body contents
        emitCellDecls(modp);
        emitEnums(modp);
        emitDesignVarDecls(modp);
        emitInternalVarDecls(modp);
        emitParamDecls(modp);
        emitCtorDtorDecls(modp);
        emitInternalMethodDecls(modp);
        emitFuncDecls(modp, /* inClassBody: */ true);

        // From `systemc_interface
        emitSystemCSection(modp, VSystemCSectionType::INT);

        // Close class body
        puts("};\n");
        // }}}

        // Emit out of class function declarations
        puts("\n");
        emitFuncDecls(modp, /* inClassBody: */ false);
        emitSystemCSection(modp, VSystemCSectionType::HDR_POST);
    }

    // VISITORS
    void visit(AstNode* nodep) override { nodep->v3fatalSrc("Unused"); }  // LCOV_EXCL_LINE

    explicit EmitCHeader(const AstNodeModule* modp) {
        UINFO(5, "  Emitting header for " << EmitCUtil::prefixNameProtect(modp));

        // Open output file
        openNewOutputHeaderFile(EmitCUtil::prefixNameProtect(modp), "Design internal header");
        puts("// See " + EmitCUtil::topClassName() + ".h for the primary calling header\n");

        ofp()->putsGuard();

        // Include files
        puts("\n");
        ofp()->putsIntTopInclude();
        puts("#include \"verilated.h\"\n");
        නම් (modp->isTop() && optSystemC()) puts("#include \"verilated_sc.h\"\n");
        නම් (v3Global.opt.mtasks()) puts("#include \"verilated_threads.h\"\n");
        නම් (v3Global.opt.savable()) puts("#include \"verilated_save.h\"\n");
        නම් (v3Global.opt.coverage()) puts("#include \"verilated_cov.h\"\n");
        නම් (v3Global.opt.coverage() || v3Global.useCovergroup())
            puts("#include \"verilated_covergroup.h\"\n");
        නම් (v3Global.usesTiming()) puts("#include \"verilated_timing.h\"\n");
        නම් (v3Global.useRandomizeMethods()) puts("#include \"verilated_random.h\"\n");
        නම් (v3Global.usesForce()) puts("#include \"verilated_force.h\"\n");

        std::set<string> cuse_set;
        auto add_to_cuse_set = [&](string s) { cuse_set.insert(s); };

        forModCUse(modp, VUseType::INT_FWD_CLASS | VUseType::INT_INCLUDE, add_to_cuse_set);
        නම් (const AstClassPackage* const packagep = VN_CAST(modp, ClassPackage)) {
            forModCUse(packagep->classp(), VUseType::INT_INCLUDE | VUseType::INT_FWD_CLASS,
                       add_to_cuse_set);
        }

        සඳහා (const string& s : cuse_set) puts(s);
        puts("\n");

        emitAll(modp);

        නම් (const AstClassPackage* const packagep = VN_CAST(modp, ClassPackage)) {
            // Put the non-static class implementation in same h file for speed
            emitAll(packagep->classp());
        }

        ofp()->putsEndGuard();

        // Close output file
        closeOutputFile();
    }
    ~EmitCHeader() override = default;

public:
    static void main(const AstNodeModule* modp) { const EmitCHeader emitCHeader{modp}; }
};

//######################################################################
// EmitC class functions

void V3EmitC::emitcHeaders() {
    UINFO(2, __FUNCTION__ << ":");

    // Process each module in turn
    සඳහා (const AstNode* nodep = v3Global.rootp()->modulesp(); nodep; nodep = nodep->nextp()) {
        නම් (VN_IS(nodep, Class)) continue;  // Declared with the ClassPackage
        const AstNodeModule* const modp = VN_AS(nodep, NodeModule);
        නම් (modp->isConstPool()) continue;  // Emitted by V3EmitCConstPool
        EmitCHeader::main(modp);
    }
}
