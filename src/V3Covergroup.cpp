// -*- mode: C++; c-file-style: "cc-mode" -*-
//*************************************************************************
// DESCRIPTION: Verilator: Functional coverage implementation
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
// FUNCTIONAL COVERAGE TRANSFORMATIONS:
//      For each covergroup (AstClass with isCovergroup()):
//          For each coverpoint (AstCoverpoint):
//              Generate member variable for VerilatedCoverpoint
//              Generate initialization in constructor
//              Generate sample code in sample() method
//
//*************************************************************************

#ਅਨ੍ਤਰ੍ਭਾਵ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ਅਨ੍ਤਰ੍ਭਾਵ "V3Covergroup.h"

#ਅਨ੍ਤਰ੍ਭਾਵ "V3Const.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3Error.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3File.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3MemberMap.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3UniqueNames.h"

#ਅਨ੍ਤਰ੍ਭਾਵ <bitset>
#ਅਨ੍ਤਰ੍ਭਾਵ <cmath>
#ਅਨ੍ਤਰ੍ਭਾਵ <deque>
#ਅਨ੍ਤਰ੍ਭਾਵ <set>
#ਅਨ੍ਤਰ੍ਭਾਵ <tuple>
#ਅਨ੍ਤਰ੍ਭਾਵ <unordered_map>
#ਅਨ੍ਤਰ੍ਭਾਵ <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Embedded covergroup assignment validation; and in the same pass over the netlist, the
// type_option variables through which SystemVerilog may set type_option.merge_instances

ਵਰਗ Covergroup_ਸੌਂਪੋ_Valid_Visitor final : ਜਨਤਕ VNVisitorConst {
    VMemberMap m_memberMap;
    std::map<ਅਚਲ AstVar*, ਅਚਲ Ast_ਗੰਢ_FTask*>
        m_constructors;  // Implicit instance -> constructor
    std::ਸੈੱਟ<ਅਚਲ AstVar*> m_mergeableTypeOptions;  // See mergeableTypeOptions()
    ਅਚਲ Ast_ਗੰਢ_FTask* m_ftaskp = ਖਾਲੀ_ਸੂਚਕ;
    ਤਾਰਕਿਕ m_collecting = ਸੱਚ;
    ਤਾਰਕਿਕ m_valid = ਸੱਚ;

    ਖਾਲੀ ਦੇਖੋ(AstClass* nodep) override {
        VL_RESTORER(m_ftaskp);
        m_ftaskp = ਖਾਲੀ_ਸੂਚਕ;
        ਜੇ (m_collecting) {
            ਅਚਲ Ast_ਗੰਢ_FTask* ਅਚਲ constructorp
                = VN_CAST(m_memberMap.ਲੱਭੋ_Member(nodep, "new"), ਗੰਢ_FTask);
            ਲਈ (ਅਚਲ Ast_ਗੰਢ* itemp = nodep->membersp(); itemp; itemp = itemp->nextp()) {
                ਅਚਲ AstVar* ਅਚਲ varp = VN_CAST(itemp, Var);
                // Only the implicit instance is restricted, not explicitly typed aliases.
                ਜੇ (!varp || !varp->isClassMember() || varp->isDeclTyped()) ਜਾਰੀ;
                ਅਚਲ AstClassRefDType* ਅਚਲ refp
                    = VN_CAST(varp->dtypep()->skipRefp(), ClassRefDType);
                ਜੇ (refp && refp->classp()->covergroupEnclosingClassp() == nodep) {
                    m_constructors.emplace(varp, constructorp);
                }
            }
        }
        iterateChildrenConst(nodep);
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ_FTask* nodep) override {
        VL_RESTORER(m_ftaskp);
        m_ftaskp = nodep;
        iterateChildrenConst(nodep);
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ_ਸੌਂਪੋ* nodep) override {
        ਜੇ (!m_collecting) {
            ਅਚਲ AstVar* varp = ਖਾਲੀ_ਸੂਚਕ;
            ਜੇ (ਅਚਲ Ast_ਗੰਢ_Var_Ref* ਅਚਲ refp = VN_CAST(nodep->lhsp(), ਗੰਢ_Var_Ref)) {
                varp = refp->varp();
            } ਨਹੀਂਤਾਂ ਜੇ (ਅਚਲ AstMemberSel* ਅਚਲ selp = VN_CAST(nodep->lhsp(), MemberSel)) {
                varp = selp->varp();
            }
            ਅਚਲ ਸਵੈਚਾਲਿਤ it = m_constructors.ਲੱਭੋ(varp);
            ਜੇ (it != m_constructors.ਸਿਰਾ() && (!m_ftaskp || m_ftaskp != it->ਦੂਜਾ)) {
                m_valid = ਝੂਠ;
                nodep->v3error("Embedded covergroup variable "
                               << varp->prettyNameQ()
                               << " may only be assigned in the enclosing class's 'new' method "
                                  "(IEEE 1800-2023 19.4).");
            }
        }
        iterateChildrenConst(nodep);
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ_Var_Ref* nodep) override {
        // Type options may be set at any time (IEEE 1800-2023 19.7.1), so SystemVerilog may set
        // type_option.merge_instances with any use of a type_option but a read of another
        // member, as not every write is marked yet (std::randomize)
        ਜੇ (m_collecting && nodep->varp()->name() == "type_option") {
            ਅਚਲ AstStructSel* ਅਚਲ selp = VN_CAST(nodep->backp(), StructSel);
            ਜੇ (!selp || selp->fromp() != nodep || selp->name() == "merge_instances"
                || !nodep->access().isReadOnly()) {
                m_mergeableTypeOptions.ਪਾਓ(nodep->varp());
            }
        }
        iterateChildrenConst(nodep);
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ* nodep) override { iterateChildrenConst(nodep); }

ਜਨਤਕ:
    ਸਪਸ਼ਟ Covergroup_ਸੌਂਪੋ_Valid_Visitor(AstNetlist* nodep) {
        // Uses can precede their enclosing class in the tree.
        iterateConst(nodep);
        m_collecting = ਝੂਠ;
        ਜੇ (!m_constructors.ਖਾਲੀ_ਥਾਵਾਂ()) iterateConst(nodep);
    }
    ਤਾਰਕਿਕ valid() ਅਚਲ { ਵਾਪਸ m_valid; }
    // type_option variables through which SystemVerilog may set type_option.merge_instances, so
    // that covergroups are tested with optionVar(true)
    ਅਚਲ std::ਸੈੱਟ<ਅਚਲ AstVar*>& mergeableTypeOptions() ਅਚਲ { ਵਾਪਸ m_mergeableTypeOptions; }
};

//######################################################################
// Covergroup expression validation visitor

ਵਰਗ Covergroup_ਵਿਅੰਜਕ_Valid_Visitor final : ਜਨਤਕ VNVisitor {
    ਅਚਲ std::ਸੈੱਟ<ਅਚਲ AstVar*>& m_sampleMembers;
    ਅਚਲ std::ਸੈੱਟ<ਅਚਲ AstVar*>& m_constructorRefMembers;
    ਤਾਰਕਿਕ m_ਵਿੱਚ_Coverage_ਵਿਅੰਜਕ = ਝੂਠ;
    ਤਾਰਕਿਕ m_sampleFormalAllowed = ਝੂਠ;

    ਖਾਲੀ ਜਾਂਚ_Sample_ਵਿਅੰਜਕ(Ast_ਗੰਢ* nodep) {
        ਜੇ (!nodep) ਵਾਪਸ;
        VL_RESTORER(m_sampleFormalAllowed);
        m_sampleFormalAllowed = ਸੱਚ;
        iterate_And_ਅਗਲਾ_Null(nodep);
    }

    ਖਾਲੀ ਜਾਂਚ_Coverage_ਵਿਅੰਜਕ(Ast_ਗੰਢ* nodep) {
        ਜੇ (!nodep) ਵਾਪਸ;
        VL_RESTORER(m_ਵਿੱਚ_Coverage_ਵਿਅੰਜਕ);
        m_ਵਿੱਚ_Coverage_ਵਿਅੰਜਕ = ਸੱਚ;
        iterate_And_ਅਗਲਾ_Null(nodep);
    }

    ਖਾਲੀ ਦੇਖੋ(AstCoverpoint* nodep) override {
        ਜਾਂਚ_Sample_ਵਿਅੰਜਕ(nodep->exprp());
        ਜਾਂਚ_Sample_ਵਿਅੰਜਕ(nodep->iffp());
        iterate_And_ਅਗਲਾ_Null(nodep->binsp());
        iterate_And_ਅਗਲਾ_Null(nodep->optionsp());
    }
    ਖਾਲੀ ਦੇਖੋ(AstCoverCross* nodep) override {
        iterate_And_ਅਗਲਾ_Null(nodep->itemsp());
        ਜਾਂਚ_Sample_ਵਿਅੰਜਕ(nodep->iffp());
        iterate_And_ਅਗਲਾ_Null(nodep->optionsp());
        iterate_And_ਅਗਲਾ_Null(nodep->binsp());
    }
    ਖਾਲੀ ਦੇਖੋ(AstCoverCrossBin* nodep) override {
        iterate_And_ਅਗਲਾ_Null(nodep->selectp());
        ਜਾਂਚ_Sample_ਵਿਅੰਜਕ(nodep->iffp());
    }
    ਖਾਲੀ ਦੇਖੋ(AstCoverCrossWith* nodep) override {
        iterate_And_ਅਗਲਾ_Null(nodep->selectp());
        ਜਾਂਚ_Coverage_ਵਿਅੰਜਕ(nodep->filterp());
        ਜਾਂਚ_Coverage_ਵਿਅੰਜਕ(nodep->countp());
    }
    ਖਾਲੀ ਦੇਖੋ(AstCoverBinsof* nodep) override { ਜਾਂਚ_Coverage_ਵਿਅੰਜਕ(nodep->rangesp()); }
    ਖਾਲੀ ਦੇਖੋ(AstCoverBin* nodep) override {
        ਜਾਂਚ_Coverage_ਵਿਅੰਜਕ(nodep->rangesp());
        ਜਾਂਚ_Sample_ਵਿਅੰਜਕ(nodep->iffp());
        ਜਾਂਚ_Coverage_ਵਿਅੰਜਕ(nodep->arraySizep());
        ਜਾਂਚ_Coverage_ਵਿਅੰਜਕ(nodep->transp());
    }
    ਖਾਲੀ ਦੇਖੋ(AstVarRef* nodep) override {
        ਜੇ (!m_sampleFormalAllowed && m_sampleMembers.ਗਿਣਤੀ_2(nodep->varp())) {
            nodep->v3error("Covergroup sample formal argument "
                           << nodep->varp()->prettyNameQ()
                           << " may only be used in a coverpoint or conditional guard "
                              "expression (IEEE 1800-2023 19.8.1).");
        }
        ਜੇ (m_ਵਿੱਚ_Coverage_ਵਿਅੰਜਕ && m_constructorRefMembers.ਗਿਣਤੀ_2(nodep->varp())) {
            nodep->v3error("Ref covergroup constructor formal argument "
                           << nodep->varp()->prettyNameQ()
                           << " may not be used in a covergroup expression "
                              "(IEEE 1800-2023 19.5).");
        }
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ_FTask_Ref* nodep) override {
        ਜੇ (m_ਵਿੱਚ_Coverage_ਵਿਅੰਜਕ && nodep->taskp()) {
            ਤਾਰਕਿਕ invalidDirection = ਝੂਠ;
            ਲਈ (Ast_ਗੰਢ* stmtp = nodep->taskp()->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
                ਅਚਲ AstVar* ਅਚਲ varp = VN_CAST(stmtp, Var);
                ਜੇ (varp && varp->isIO() && varp->isWritable()) {
                    invalidDirection = ਸੱਚ;
                    ਰੋਕੋ;
                }
            }
            ਜੇ (invalidDirection) {
                nodep->v3error("Function " << nodep->taskp()->prettyNameQ()
                                           << " called in a covergroup expression has an "
                                              "output, inout, or non-const ref argument "
                                              "(IEEE 1800-2023 19.5).");
            }
        }
        iterateChildren(nodep);
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ* nodep) override { iterateChildren(nodep); }

ਜਨਤਕ:
    Covergroup_ਵਿਅੰਜਕ_Valid_Visitor(ਅਚਲ std::ਸੈੱਟ<ਅਚਲ AstVar*>& sampleMembers,
                               ਅਚਲ std::ਸੈੱਟ<ਅਚਲ AstVar*>& constructorRefMembers)
        : m_sampleMembers{sampleMembers}
        , m_constructorRefMembers{constructorRefMembers} {}
    ਖਾਲੀ ਜਾਂਚ(Ast_ਗੰਢ* nodep) { iterate(nodep); }
};

//######################################################################
// Bins of one declaration whose values are computed rather than listed: bin k covers
// [m_lo + k * m_stride, m_lo + (k + 1) * m_stride - 1], and the last bin extends to m_hi.  An
// array bin element is a run of single-value bins; automatic bins partition the coverpoint
// domain.  Bounds are coverpoint values at FunctionalCoverageVisitor::runWidth(),
// sign-extended like a CrossValueRange's.

ਵਰਗ Bin_ਲੜੀਵਾਰ final {
ਜਨਤਕ:
    // MEMBERS
    uint32_t m_ਗਿਣਤੀ;  // Number of bins
    V3_ਸੰਖਿਆ m_lo;  // Lowest value of the first bin
    V3_ਸੰਖਿਆ m_stride;  // Number of values of each bin but the last
    V3_ਸੰਖਿਆ m_hi;  // Highest value of the last bin
    ਤਾਰਕਿਕ m_ਖਾਲੀ_ਥਾਵਾਂ = ਝੂਠ;  // A single bin without a value of the coverpoint type
    uint32_t m_declared = 0;  // Runtime index of the first bin, once generated

    // CONSTRUCTORS
    Bin_ਲੜੀਵਾਰ(Ast_ਗੰਢ* nodep, ਪੂਰਨ ਚੌੜਾਈ, uint32_t ਗਿਣਤੀ_2)
        : m_ਗਿਣਤੀ{ਗਿਣਤੀ_2}
        , m_lo{nodep, ਚੌੜਾਈ}
        , m_stride{nodep, ਚੌੜਾਈ, 1}
        , m_hi{nodep, ਚੌੜਾਈ} {}
};

//######################################################################
// Functional coverage visitor

ਵਰਗ FunctionalCoverageVisitor final : ਜਨਤਕ VNVisitor {
    // NODE STATE
    // Entire netlist:
    //  AstCoverpoint::user1p()  -> AstVar*.  Previous-value variable for transition bins
    //  AstCoverpoint::user2()   -> bool.  Had a bins declaration ignored, so no automatic bins
    ਅਚਲ VNUser1_ਵਿੱਚ_Use m_inuser1;
    ਅਚਲ VNUser2_ਵਿੱਚ_Use m_inuser2;

    // STATE
    std::ਸੈੱਟ<AstCoverpoint*>
        m_runtimePoints;  // Points needing value metadata and live-bin mapping
    std::ਸੈੱਟ<AstCoverCross*> m_runtimeCrosses;  // Crosses over finalized live-bin dimensions
    std::map<AstVar*, AstVar*> m_excludedVars;  // Sample-time state-exclusion flags
    AstClass* m_covergroupp = ਖਾਲੀ_ਸੂਚਕ;  // Current covergroup being processed
    Ast_ਗੰਢ_Module* m_unitp = ਖਾਲੀ_ਸੂਚਕ;  // Design unit declaring the current class, outside classes
    AstClass* m_enclosingClassp = ਖਾਲੀ_ਸੂਚਕ;  // Class lexically enclosing the covergroup, if any
    AstVar* m_embeddedVarp = ਖਾਲੀ_ਸੂਚਕ;  // Embedded covergroup member of m_enclosingClassp, if any
    std::string m_covergroupName;  // Current covergroup's type name, see covergroupTypeName()
    AstFunc* m_sampleFuncp = ਖਾਲੀ_ਸੂਚਕ;  // Current sample() function
    AstIf* m_cpGuardp = ਖਾਲੀ_ਸੂਚਕ;  // Current coverpoint's iff guard, holding its sampling
    AstFunc* m_constructorp = ਖਾਲੀ_ਸੂਚਕ;  // Current constructor
    std::ਸਦਿਸ਼<AstCoverpoint*> m_coverpoints;  // Coverpoints in current covergroup
    std::map<std::string, AstCoverpoint*> m_coverpointMap;  // Name -> coverpoint for fast lookup
    std::ਸਦਿਸ਼<AstCoverCross*> m_coverCrosses;  // Cross coverage items in current covergroup
    std::ਸਦਿਸ਼<Ast_Cg_Option_ਸੌਂਪੋ*> m_cgOptions;  // Covergroup-level options, before lowering
    uint32_t m_cg_Type_ਭਾਰ = 1;  // The covergroup's type_option.weight, a constant
    ਤਾਰਕਿਕ m_cg_ਮਿਲਾਓ_Instances = ਝੂਠ;  // The covergroup's type_option.merge_instances, a constant
    // The covergroup's type_option.merge_instances is, or SystemVerilog may make it, true
    ਤਾਰਕਿਕ m_cg_May_ਮਿਲਾਓ = ਝੂਠ;
    // type_option members through which SystemVerilog may set type_option.merge_instances,
    // see CovergroupAssignValidVisitor::mergeableTypeOptions()
    ਅਚਲ std::ਸੈੱਟ<ਅਚਲ AstVar*>& m_mergeableTypeOptions;
    ਪੂਰਨ m_cg_At_ਘੱਟ_ਤੋਂ_ਘੱਟ = 1;  // Covergroup option.at_least

    ਬਣਤਰ EmbeddedEventTrigger final {
        File_ਸਤਰ* eventFl;  // Clocking-event source location
        AstVar* ਅਧਾਰ_Varp;  // Base enclosing-class member in the event expression
        AstVar* memberVarp;  // Selected member in a 'base.member' expression, or nullptr
        VEdgeType ਕਿਨਾਰਾ_Type;  // Clocking-event edge qualifier
        AstVar* prevVarp;  // Member containing the previous event value, or nullptr
        EmbeddedEventTrigger(File_ਸਤਰ* eventFl, AstVar* ਅਧਾਰ_Varp, AstVar* memberVarp,
                             VEdgeType ਕਿਨਾਰਾ_Type)
            : eventFl{eventFl}
            , ਅਧਾਰ_Varp{ਅਧਾਰ_Varp}
            , memberVarp{memberVarp}
            , ਕਿਨਾਰਾ_Type{ਕਿਨਾਰਾ_Type}
            , prevVarp{ਖਾਲੀ_ਸੂਚਕ} {}
    };

    std::ਸੈੱਟ<std::string> m_crossed_Cp_ਨਾਂ;  // Coverpoints referenced by a cross
    std::map<std::string, AstVar*> m_cpVarMap;  // Coverpoint name -> its VlCoverpoint member
    ਬਣਤਰ BinRuns final {
        std::ਸਦਿਸ਼<Bin_ਲੜੀਵਾਰ> runs;  // Runs of an array or automatic bins declaration, in order
        uint32_t ਗਿਣਤੀ_2 = 0;  // Bins across all runs
        ਤਾਰਕਿਕ unsupported = ਝੂਠ;  // Too many bins, or invalid: the declaration is ignored
        // The value of each bin, which names it, of a wildcard array; else the bins are indexed
        std::ਸਦਿਸ਼<std::string> ਮੁੱਲਾਂ;
        // Too many values of an ignore or illegal wildcard array, which is then one bin
        ਤਾਰਕਿਕ single = ਝੂਠ;
    };
    ਬਣਤਰ Cross_Bin_ਮੁੱਲਾਂ final {
        AstCoverBin* binp;  // Declaration owning this Normal bin
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* valuep;  // Individual array-bin value, or nullptr for a scalar bin
        ਅਚਲ Bin_ਲੜੀਵਾਰ* runp = ਖਾਲੀ_ਸੂਚਕ;  // Run computing the bin's values, if any
        uint32_t element = 0;  // Index of the bin within runp
    };
    ਬਣਤਰ Bin_ਵਿਸਤਾਰ final {
        uint32_t ਪਹਿਲਾ = 0;  // First Normal index of the bin declaration
        uint32_t ਗਿਣਤੀ_2 = 0;  // Number of Normal bins of the declaration
        uint32_t declared = 0;  // First runtime bin index, across all bin kinds
        int32_t sized = -1;  // Index of the sized array, placed at construction; or -1
    };
    ਬਣਤਰ CoverpointBins final {
        uint32_t ਕੁੱਲ = 0;  // Number of Normal bins
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* exprp = ਖਾਲੀ_ਸੂਚਕ;  // Sampled expression, for the value domain
        ਤਾਰਕਿਕ crossed = ਝੂਠ;  // Feeds a cross, which needs 'values'
        std::ਸਦਿਸ਼<Cross_Bin_ਮੁੱਲਾਂ> ਮੁੱਲਾਂ;  // Values in runtime Normal-bin index order
        std::unordered_map<std::string, Bin_ਵਿਸਤਾਰ> spans;  // Declared bin name -> index span
        Bin_ਵਿਸਤਾਰ implicitAuto{0, 0, 0};  // Implicit automatic bins, each named 'auto_<i>'
        std::deque<Bin_ਲੜੀਵਾਰ> runs;  // Runs 'values' refers to
    };
    std::map<AstVar*, CoverpointBins> m_cpBins;  // Runtime coverpoint -> binsof index ranges
    // Names of the bins declarations each coverpoint ignored, which binsof selects as no bins
    std::map<ਅਚਲ AstCoverpoint*, std::ਸਦਿਸ਼<std::string>> m_droppedBins;
    // Prefixes of the constructor temporaries of constructed bins, as coverpoint and bin names
    // alone may repeat: coverpoint 'a_' bins 'b', and coverpoint 'a' bins '_b'
    V3_Unique_ਨਾਂ m_sized_ਨਾਂ{"__Vsized"};
    std::ਸਦਿਸ਼<Ast_ਗੰਢ_ਵਿਅੰਜਕ*> m_detached_ਮੁੱਲਾਂ;  // Array-bin values m_cpBins refers to
    std::ਸੈੱਟ<AstCoverCross*>
        m_droppedCrosses;  // Crosses with a bare-variable item: drop (COVERIGN)
    std::map<uint32_t, AstCoverpointDType*> m_cpDTypes;  // Hit-list bound -> interned dtype
    ਵਰਤੋਂ CrossShape = std::tuple<uint32_t, uint32_t, uint32_t, uint32_t, uint64_t, ਤਾਰਕਿਕ>;
    std::map<CrossShape, AstCoverCrossDType*> m_cxDTypes;
    AstVar* m_cgInstVarp = ਖਾਲੀ_ਸੂਚਕ;  // __Vcg_inst handle member of the current covergroup

    VMemberMap m_memberMap;  // Member names cached for fast lookup

    // METHODS
    // The covergroup's 'option' or static 'type_option' member (V3LinkParse creates both)
    AstVar* optionVar(ਤਾਰਕਿਕ typeOption) {
        AstVar* ਅਚਲ varp = VN_AS(
            m_memberMap.ਲੱਭੋ_Member(m_covergroupp, typeOption ? "type_option" : "option"), Var);
        UASSERT_OBJ(varp, m_covergroupp, "Covergroup missing option member");
        ਵਾਪਸ varp;
    }

    // 'option.<member>' or 'type_option.<member>', per optionVarp
    AstStructSel* newOptionSel(File_ਸਤਰ* fl, AstVar* optionVarp, ਅਚਲ std::string& member,
                               VAccess access) {
        ਅਚਲ AstMemberDType* ਅਚਲ memberp
            = VN_AS(m_memberMap.ਲੱਭੋ_Member(optionVarp->dtypep()->skipRefp(), member), MemberDType);
        UASSERT_OBJ(memberp, optionVarp, "Coverage option structure missing '" << member << "'");
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ fromp = optionVarp->lifetime().isStatic()
                                       ? ਨਵਾਂ AstVarRef{fl, optionVarp, access}
                                       : memberRef(fl, optionVarp, access);
        AstStructSel* ਅਚਲ selp = ਨਵਾਂ AstStructSel{fl, fromp, member};
        selp->dtypep(memberp->subDTypep()->skip_Ref_ਮੰਜ਼ਿਲ_Enump());
        selp->did_ਚੌੜਾਈ(ਸੱਚ);
        ਵਾਪਸ selp;
    }

    // A constant option.at_least; a negative count of hits has no meaning, so takes the default
    ਸਥਿਰ ਪੂਰਨ const_At_ਘੱਟ_ਤੋਂ_ਘੱਟ(ਅਚਲ AstConst* constp) {
        ਅਚਲ ਪੂਰਨ ਮੁੱਲ = constp->ਮੰਜ਼ਿਲ_SInt();
        ਜੇ (ਮੁੱਲ >= 0) ਵਾਪਸ ਮੁੱਲ;
        constp->v3warn(COVERIGN, "Ignoring negative 'option.at_least'; using 1");
        ਵਾਪਸ 1;
    }

    // An option's constant value, or nullptr after a warning, as non-constant is unsupported
    ਸਥਿਰ ਅਚਲ AstConst* constOption(ਅਚਲ Ast_ਗੰਢ* valuep, VCoverOptionType optType) {
        ਜੇ (ਅਚਲ AstConst* ਅਚਲ constp = VN_CAST(valuep, Const)) ਵਾਪਸ constp;
        valuep->v3warn(COVERIGN, "Ignoring unsupported: non-constant 'option."
                                     << optType.ascii() << "'; using default value");
        ਵਾਪਸ ਖਾਲੀ_ਸੂਚਕ;
    }

    // Store the covergroup-level options (IEEE 1800-2023 19.7) where SystemVerilog and the
    // runtime read them.  The instance options, option.weight and option.get_inst_coverage,
    // are evaluated by the constructor, as are the other instance options; the type options,
    // type_option.weight and type_option.merge_instances, are constant, and initialize the
    // static member.  Without its own type_option.merge_instances, a covergroup has the
    // default of --coverage-merge-instances.  A constant option.at_least is the default of
    // the coverpoints and crosses.
    ਖਾਲੀ ਹੇਠਲਾ_Covergroup_Options() {
        ਤਾਰਕਿਕ ਮਿਲਾਓ_ਸੈੱਟ = ਝੂਠ;  // The covergroup sets type_option.merge_instances
        m_cg_At_ਘੱਟ_ਤੋਂ_ਘੱਟ = 1;
        ਲਈ (Ast_Cg_Option_ਸੌਂਪੋ* ਅਚਲ optp : m_cgOptions) {
            ਜੇ (optp->optType() == VCoverOptionType::AT_ਘੱਟ_ਤੋਂ_ਘੱਟ) {
                ਜੇ (ਅਚਲ AstConst* ਅਚਲ constp = constOption(optp->valuep(), optp->optType())) {
                    m_cg_At_ਘੱਟ_ਤੋਂ_ਘੱਟ = const_At_ਘੱਟ_ਤੋਂ_ਘੱਟ(constp);
                }
                VL_DO_DANGLING(ਧੱਕੋ_Deletep(optp->unlink_Fr_ਪਿੱਠ()), optp);
                ਜਾਰੀ;
            }
            File_ਸਤਰ* ਅਚਲ fl = optp->fileline();
            // V3Width left the type options constant, and type_option.weight non-negative
            ਅਚਲ AstConst* ਅਚਲ constp = VN_CAST(optp->valuep(), Const);
            std::string member = "weight";
            ਜੇ (optp->optType() == VCoverOptionType::ਮਿਲਾਓ_INSTANCES) {
                member = "merge_instances";
                m_cg_ਮਿਲਾਓ_Instances = !constp->ਅੰਕ().isEqZero();
                ਮਿਲਾਓ_ਸੈੱਟ = ਸੱਚ;
            } ਨਹੀਂਤਾਂ ਜੇ (optp->optType() == VCoverOptionType::GET_INST_COVERAGE) {
                member = "get_inst_coverage";
            } ਨਹੀਂਤਾਂ {
                UASSERT_OBJ(optp->optType() == VCoverOptionType::ਭਾਰ, optp,
                            "Unexpected covergroup option reaching V3Covergroup");
                ਜੇ (optp->typeOption()) m_cg_Type_ਭਾਰ = constp->ਮੰਜ਼ਿਲ_UInt();
            }
            Ast_ਸੌਂਪੋ* ਅਚਲ assignp = ਨਵਾਂ Ast_ਸੌਂਪੋ{
                fl, newOptionSel(fl, optionVar(optp->typeOption()), member, VAccess::WRITE),
                optp->valuep()->unlink_Fr_ਪਿੱਠ()};
            ਜੇ (optp->typeOption()) {
                m_covergroupp->ਜੋੜੋ_Membersp(ਨਵਾਂ AstInitialStatic{fl, assignp});
                VL_DO_DANGLING(ਧੱਕੋ_Deletep(optp->unlink_Fr_ਪਿੱਠ()), optp);
            } ਨਹੀਂਤਾਂ {
                optp->replaceWith(assignp);
                VL_DO_DANGLING(ਧੱਕੋ_Deletep(optp), optp);
            }
        }
        m_cgOptions.clear();
        ਜੇ (!ਮਿਲਾਓ_ਸੈੱਟ && v3Global.opt.coverage_ਮਿਲਾਓ_Instances()) {
            File_ਸਤਰ* ਅਚਲ fl = m_covergroupp->fileline();
            m_covergroupp->ਜੋੜੋ_Membersp(ਨਵਾਂ AstInitialStatic{
                fl, ਨਵਾਂ Ast_ਸੌਂਪੋ{
                        fl, newOptionSel(fl, optionVar(ਸੱਚ), "merge_instances", VAccess::WRITE),
                        ਨਵਾਂ AstConst{fl, AstConst::ਦੋਆਧਾਰੀ_ਅੰਕ_True{}}}});
            m_cg_ਮਿਲਾਓ_Instances = ਸੱਚ;
        }
        m_cg_May_ਮਿਲਾਓ = m_cg_ਮਿਲਾਓ_Instances || m_mergeableTypeOptions.ਗਿਣਤੀ_2(optionVar(ਸੱਚ));
    }

    // The weight of an item in the coverage database, which merges the instances: its
    // type_option.weight if the covergroup merges them too (IEEE 1800-2023 19.11.3); else its
    // option.weight if a constant, and so of every instance; else its type_option.weight
    uint32_t ਵਸਤੂ_Database_ਭਾਰ(Ast_ਗੰਢ* optionsp) ਅਚਲ {
        ਅਚਲ Ast_ਗੰਢ_ਵਿਅੰਜਕ* weightp = ਖਾਲੀ_ਸੂਚਕ;  // The option.weight in effect
        uint32_t type_ਭਾਰ = 1;
        ਲਈ (Ast_ਗੰਢ* nodep = optionsp; nodep; nodep = nodep->nextp()) {
            ਅਚਲ AstCoverOption* ਅਚਲ optp = VN_AS(nodep, CoverOption);
            ਜੇ (!(optp->optType() == VCoverOptionType::ਭਾਰ)) ਜਾਰੀ;
            // V3Width left type_option.weight a non-negative constant
            ਜੇ (optp->typeOption()) {
                type_ਭਾਰ = VN_AS(optp->valuep(), Const)->ਮੰਜ਼ਿਲ_UInt();
            } ਨਹੀਂਤਾਂ {
                weightp = optp->valuep();
            }
        }
        ਜੇ (m_cg_ਮਿਲਾਓ_Instances) ਵਾਪਸ type_ਭਾਰ;
        ਜੇ (!weightp) ਵਾਪਸ 1;
        ਜੇ (ਅਚਲ AstConst* ਅਚਲ constp = VN_CAST(weightp, Const)) ਵਾਪਸ constp->ਮੰਜ਼ਿਲ_UInt();
        ਵਾਪਸ type_ਭਾਰ;
    }

    // Configure an item's option.weight, its weight in instance coverage (IEEE 1800-2023
    // 19.11), and, if the covergroup may merge its instances, its type_option.weight, its
    // weight in type coverage then (19.11.3).
    ਖਾਲੀ generate_ਵਸਤੂ_ਭਾਰ(File_ਸਤਰ* fl, AstVar* ਵਸਤੂ_Varp, Ast_ਗੰਢ* optionsp) {
        ਲਈ (Ast_ਗੰਢ* nodep = optionsp; nodep; nodep = nodep->nextp()) {
            ਅਚਲ AstCoverOption* ਅਚਲ optp = VN_AS(nodep, CoverOption);
            ਜੇ (!(optp->optType() == VCoverOptionType::ਭਾਰ)) ਜਾਰੀ;
            ਜੇ (!optp->typeOption()) {
                m_constructorp->ਜੋੜੋ_Stmtsp(
                    ਵਸਤੂ_Call(fl, ਵਸਤੂ_Varp, VCMethod::COVERGROUP_ਭਾਰ,
                             {optp->valuep()->clone_ਰੁੱਖ(ਝੂਠ), file_ਸਤਰ_Debug(optp->fileline())})
                        ->makeStmt());
            } ਨਹੀਂਤਾਂ ਜੇ (m_cg_May_ਮਿਲਾਓ) {
                // V3Width left type_option.weight a non-negative constant
                m_constructorp->ਜੋੜੋ_Stmtsp(ਵਸਤੂ_Call(fl, ਵਸਤੂ_Varp, VCMethod::COVERGROUP_TYPE_ਭਾਰ,
                                                   {optp->valuep()->clone_ਰੁੱਖ(ਝੂਠ)})
                                              ->makeStmt());
            }
        }
    }

    ਖਾਲੀ processCovergroup() {
        UINFO(4, "Processing covergroup: " << m_covergroupp->name() << " with "
                                           << m_coverpoints.ਮਾਪ() << " coverpoints and "
                                           << m_coverCrosses.ਮਾਪ() << " crosses");

        m_crossed_Cp_ਨਾਂ.clear();
        m_cpVarMap.clear();
        m_cpBins.clear();
        m_droppedBins.clear();
        m_sized_ਨਾਂ.reset();
        m_runtimePoints.clear();
        m_runtimeCrosses.clear();
        m_excludedVars.clear();
        m_droppedCrosses.clear();
        m_cgInstVarp = ਖਾਲੀ_ਸੂਚਕ;
        m_cg_Type_ਭਾਰ = 1;
        m_cg_ਮਿਲਾਓ_Instances = ਝੂਠ;
        m_cg_May_ਮਿਲਾਓ = ਝੂਠ;

        ਹੇਠਲਾ_Covergroup_Options();

        // Scan every cross item to record the coverpoints it references (the cross dimensions)
        // and to flag any cross naming a bare variable -- a would-be implicit coverpoint, which
        // Verilator does not synthesize.  An unresolvable item drops only that one cross (with a
        // COVERIGN in generateCrossCode), leaving the rest of the covergroup intact.
        ਲਈ (AstCoverCross* crossp : m_coverCrosses) {
            ਲਈ (Ast_ਗੰਢ* itemp = crossp->itemsp(); itemp; itemp = itemp->nextp()) {
                ਅਚਲ AstCoverpointRef* ਅਚਲ refp = VN_AS(itemp, CoverpointRef);
                ਜੇ (refp->exprp()) ਜਾਰੀ;  // hierarchical ref: dropped in generateCrossCode
                ਜੇ (m_coverpointMap.ਲੱਭੋ(refp->name()) == m_coverpointMap.ਸਿਰਾ()) {
                    m_droppedCrosses.ਪਾਓ(crossp);  // bare variable: drop this cross only
                } ਨਹੀਂਤਾਂ {
                    m_crossed_Cp_ਨਾਂ.ਪਾਓ(refp->name());
                }
            }
        }

        std::ਸਦਿਸ਼<Ast_ਗੰਢ*> pending;
        std::map<AstCoverpoint*, std::ਸਦਿਸ਼<AstCoverCross*>> consumers;
        std::map<AstCoverCross*, std::ਸਦਿਸ਼<AstCoverpoint*>> inputs;
        ਲਈ (AstCoverpoint* ਅਚਲ cpp : m_coverpoints) {
            check_Bin_ਨਾਂ(cpp);
            checkConstructedBins(cpp);
            ਜੇ (!cpp->exprp()->dtypep()->skipRefp()->isIntegralOrPacked()) ਜਾਰੀ;
            // Bins without values leave the report (IEEE 1800-2023 19.11.1), exclusions or not.
            // Constructed bins get their values when the covergroup is constructed.
            ਜੇ (!coverpointHasStateExclusions(cpp) && !coverpoint_Has_ਖਾਲੀ_ਥਾਵਾਂ_Bins(cpp)
                && !coverpointHasConstructedBins(cpp)) {
                ਜਾਰੀ;
            }
            m_runtimePoints.ਪਾਓ(cpp);
            pending.ਧੱਕੋ_ਪਿੱਠ(cpp);
        }
        ਲਈ (AstCoverCross* ਅਚਲ crossp : m_coverCrosses) {
            ਜੇ (m_droppedCrosses.ਗਿਣਤੀ_2(crossp)) ਜਾਰੀ;
            ਲਈ (Ast_ਗੰਢ* itemp = crossp->itemsp(); itemp; itemp = itemp->nextp()) {
                ਅਚਲ AstCoverpointRef* ਅਚਲ refp = VN_AS(itemp, CoverpointRef);
                ਜੇ (refp->exprp()) ਜਾਰੀ;
                ਅਚਲ ਸਵੈਚਾਲਿਤ ਬਿੰਦੂ = m_coverpointMap.ਲੱਭੋ(refp->name());
                ਜੇ (ਬਿੰਦੂ != m_coverpointMap.ਸਿਰਾ()) {
                    consumers[ਬਿੰਦੂ->ਦੂਜਾ].ਧੱਕੋ_ਪਿੱਠ(crossp);
                    inputs[crossp].ਧੱਕੋ_ਪਿੱਠ(ਬਿੰਦੂ->ਦੂਜਾ);
                }
            }
            // The values of a filter's bin tuples are known when the covergroup is constructed
            ਜੇ (crossp->exists([](ਅਚਲ AstCoverCrossWith*) { ਵਾਪਸ ਸੱਚ; })) {
                m_runtimeCrosses.emplace(crossp);
                pending.ਧੱਕੋ_ਪਿੱਠ(crossp);
            }
        }
        ਲਈ (ਮਾਪ_t ਅਗਲਾ = 0; ਅਗਲਾ < pending.ਮਾਪ(); ++ਅਗਲਾ) {
            ਜੇ (AstCoverpoint* ਅਚਲ pointp = VN_CAST(pending[ਅਗਲਾ], Coverpoint)) {
                ਲਈ (AstCoverCross* ਅਚਲ crossp : consumers[pointp]) {
                    ਜੇ (m_runtimeCrosses.emplace(crossp).ਦੂਜਾ) pending.ਧੱਕੋ_ਪਿੱਠ(crossp);
                }
            } ਨਹੀਂਤਾਂ {
                ਲਈ (AstCoverpoint* ਅਚਲ pointp : inputs[VN_AS(pending[ਅਗਲਾ], CoverCross)]) {
                    ਜੇ (pointp->exprp()->dtypep()->skipRefp()->isIntegralOrPacked()
                        && m_runtimePoints.emplace(pointp).ਦੂਜਾ) {
                        pending.ਧੱਕੋ_ਪਿੱਠ(pointp);
                    }
                }
            }
        }

        // The instance node owns this instance's coverpoint/cross runtimes, so it must exist
        // before any of them is created.  Emitted first, ahead of both generate loops.
        generateInstanceAttach();

        // For each coverpoint, generate sampling code
        ਲਈ (AstCoverpoint* cpp : m_coverpoints) generate_Coverpoint_ਕੋਡ(cpp);

        // For each cross, generate sampling code
        ਲਈ (AstCoverCross* crossp : m_coverCrosses) generate_Cross_ਕੋਡ(crossp);

        // Every cross has been built, so runtime points only need their exclusions from here.
        ਲਈ (AstCoverpoint* ਅਚਲ cpp : m_coverpoints) {
            ਜੇ (!m_runtimePoints.ਗਿਣਤੀ_2(cpp)) ਜਾਰੀ;
            m_constructorp->ਜੋੜੋ_Stmtsp(ਵਸਤੂ_Call(cpp->fileline(), m_cpVarMap.at(cpp->name()),
                                               VCMethod::COVERGROUP_ਮੁੱਲ_RELEASE)
                                          ->makeStmt());
        }
        ਲਈ (Ast_ਗੰਢ_ਵਿਅੰਜਕ* valuep : m_detached_ਮੁੱਲਾਂ) VL_DO_DANGLING(ਧੱਕੋ_Deletep(valuep), valuep);
        m_detached_ਮੁੱਲਾਂ.clear();

        // Generate coverage computation code (even for empty covergroups).  Bin registration
        // with the coverage database is handled per coverpoint/cross by their runtime
        // registerBins() calls (emitted in generateCoverpoint/generateCross).
        generate_Coverage_Computation_ਕੋਡ();
    }

    ਸਥਿਰ constexpr ਮਾਪ_t ਮੁੱਲ_LIST_ENTRIES = 256;  // Metadata entries per constructor call

    // The value of a constant count, of bins or value tuples: -1 if it is negative, and
    // saturated above the largest limit
    ਸਥਿਰ int64_t const_ਗਿਣਤੀ(ਅਚਲ AstConst* constp) {
        ਅਚਲ V3_ਸੰਖਿਆ& ਅੰਕ = constp->ਅੰਕ();
        ਜੇ (constp->isSigned() && ਅੰਕ.isNegative()) ਵਾਪਸ -1;
        ਵਾਪਸ ਅੰਕ.most_ਸੈੱਟ_ਦੋਆਧਾਰੀ_ਅੰਕ_P1() > 32 ? INT64_MAX : static_cast<int64_t>(ਅੰਕ.ਮੰਜ਼ਿਲ_UQuad());
    }

    // Whether a count, 'whatp', is integral and, if constant, positive, as the standard's clause
    // 'clausep' requires; else errors
    ਸਥਿਰ ਤਾਰਕਿਕ check_ਗਿਣਤੀ(ਅਚਲ Ast_ਗੰਢ_ਵਿਅੰਜਕ* countp, ਅਚਲ ਅੱਖਰ* whatp, ਅਚਲ ਅੱਖਰ* clausep) {
        ਅਚਲ AstConst* ਅਚਲ constp = VN_CAST(countp, Const);
        ਜੇ (!countp->dtypep()->skipRefp()->isIntegralOrPacked()) {
            countp->v3error(whatp << " must be integral (IEEE 1800-2023 " << clausep << ")");
            ਵਾਪਸ ਝੂਠ;
        } ਨਹੀਂਤਾਂ ਜੇ (constp && (constp->ਅੰਕ().isFourState() || const_ਗਿਣਤੀ(constp) < 1)) {
            countp->v3error(whatp << " must be >= 1, got "
                                  << (constp->ਅੰਕ().isFourState() ? constp->ਅੰਕ().ascii(ਝੂਠ)
                                                                  : constp->ਅੰਕ().ਮੰਜ਼ਿਲ_Decimal_S())
                                  << " (IEEE 1800-2023 " << clausep << ")");
            ਵਾਪਸ ਝੂਠ;
        }
        ਵਾਪਸ ਸੱਚ;
    }

    // The number of bins requested by a valid 'bins auto[N]', or 0
    ਸਥਿਰ uint32_t autoBinsRequested(ਅਚਲ AstCoverBin* binp) {
        ਅਚਲ AstConst* ਅਚਲ constp = VN_CAST(binp->arraySizep(), Const);
        ਜੇ (!constp) ਵਾਪਸ 0;
        ਅਚਲ int64_t ਗਿਣਤੀ_2 = const_ਗਿਣਤੀ(constp);
        ਵਾਪਸ ਗਿਣਤੀ_2 < 1 || ਗਿਣਤੀ_2 > v3Global.opt.coverageMaxBins() ? 0
                                                                   : static_cast<uint32_t>(ਗਿਣਤੀ_2);
    }

    // True for a 'bins auto[N]' declaration, or the implicit automatic bins of a coverpoint
    ਸਥਿਰ ਤਾਰਕਿਕ isAutoBins(ਅਚਲ AstCoverBin* binp) {
        ਵਾਪਸ binp->binsType() == VCoverBinsType::BINS_AUTO
               || binp->binsType() == VCoverBinsType::BINS_AUTO_IMPLICIT;
    }

    // True for a sized array of bins, 'bins b[N] = {...}', whose values an integral coverpoint
    // distributes over N bins when the covergroup is constructed (IEEE 1800-2023 19.5.1)
    ਸਥਿਰ ਤਾਰਕਿਕ isSizedArray(ਅਚਲ AstCoverBin* binp) {
        ਵਾਪਸ binp->arraySizep() && !isAutoBins(binp);
    }

    // The 'with' filter of a bin, if any (IEEE 1800-2023 19.5.1.1)
    ਸਥਿਰ AstCoverWith* binWith(ਅਚਲ AstCoverBin* binp) {
        ਵਾਪਸ VN_CAST(binp->rangesp(), CoverWith);
    }

    // True for bins whose values the covergroup constructor computes: a sized array, or bins of
    // the values a 'with' filter keeps
    ਸਥਿਰ ਤਾਰਕਿਕ isConstructedBins(ਅਚਲ AstCoverBin* binp) {
        ਵਾਪਸ isSizedArray(binp) || binWith(binp);
    }

    // The range list of a bin's values, or of the candidates of its 'with' filter; null for all
    // of the coverpoint's values, which a filter of the coverpoint's name has
    ਸਥਿਰ Ast_ਗੰਢ* binRangesp(ਅਚਲ AstCoverBin* binp) {
        ਅਚਲ AstCoverWith* ਅਚਲ withp = binWith(binp);
        ਜੇ (!withp) ਵਾਪਸ binp->rangesp();
        ਵਾਪਸ VN_IS(withp->subp(), CoverpointRef) ? ਖਾਲੀ_ਸੂਚਕ : withp->subp();
    }

    // Report and delete a bins declaration of a name that another of its coverpoint has
    ਖਾਲੀ check_Bin_ਨਾਂ(AstCoverpoint* coverpointp) {
        std::ਸੈੱਟ<std::string> ਨਾਂ;
        ਲਈ (Ast_ਗੰਢ* nodep = coverpointp->binsp(); nodep;) {
            AstCoverBin* ਅਚਲ binp = VN_AS(nodep, CoverBin);
            nodep = nodep->nextp();
            ਜੇ (ਨਾਂ.emplace(binp->name()).ਦੂਜਾ) ਜਾਰੀ;
            binp->v3error("Duplicate bin " << binp->prettyNameQ() << " in coverpoint "
                                           << coverpointp->prettyNameQ()
                                           << " (IEEE 1800-2023 3.13)");
            VL_DO_DANGLING(ਧੱਕੋ_Deletep(binp->unlink_Fr_ਪਿੱਠ()), binp);
        }
    }

    // Delete an ignored bins declaration, which binsof then selects as no bins
    ਖਾਲੀ dropBins(ਅਚਲ AstCoverpoint* coverpointp, AstCoverBin* binp) {
        m_droppedBins[coverpointp].ਧੱਕੋ_ਪਿੱਠ(binp->name());
        VL_DO_DANGLING(ਧੱਕੋ_Deletep(binp->unlink_Fr_ਪਿੱਠ()), binp);
    }

    // Check the size of a sized array of bins, which drops an invalid array.  A real coverpoint's
    // are unsupported, and treated as arrays of a bin per value.  False unless it stays sized.
    ਤਾਰਕਿਕ check_Bins_Array_ਮਾਪ(ਅਚਲ AstCoverpoint* coverpointp, AstCoverBin* binp, ਤਾਰਕਿਕ integral) {
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ sizep = binp->arraySizep();
        ਜੇ (VN_IS(sizep, Unbounded)) {  // A parameter of '$'; see bins_orBraE
            binp->v3error("Bins array size must be integral, not '$' (IEEE 1800-2023 19.5.1)");
        } ਨਹੀਂਤਾਂ ਜੇ (check_ਗਿਣਤੀ(sizep, "Bins array size", "19.5.1")) {
            ਜੇ (integral) ਵਾਪਸ ਸੱਚ;
            binp->v3warn(COVERIGN, "Unsupported: 'bins' explicit array size of a real "
                                   "coverpoint (treated as '[]')");
            VL_DO_DANGLING(ਧੱਕੋ_Deletep(sizep->unlink_Fr_ਪਿੱਠ()), sizep);
            ਵਾਪਸ ਝੂਠ;
        }
        dropBins(coverpointp, binp);
        ਵਾਪਸ ਝੂਠ;
    }

    // Check the bins of a coverpoint whose values the constructor computes, dropping invalid
    // ones.  A wildcard array of too many ranges of values is ignored, or if ignore or illegal,
    // treated as one bin; filtering too many ranges of values ignores the bins.
    ਖਾਲੀ checkConstructedBins(AstCoverpoint* coverpointp) {
        ਅਚਲ ਤਾਰਕਿਕ integral = coverpointp->exprp()->dtypep()->skipRefp()->isIntegralOrPacked();
        ਲਈ (Ast_ਗੰਢ* nodep = coverpointp->binsp(); nodep;) {
            AstCoverBin* ਅਚਲ binp = VN_AS(nodep, CoverBin);
            nodep = nodep->nextp();
            ਜੇ (!isConstructedBins(binp)) ਜਾਰੀ;
            ਜੇ (isSizedArray(binp) && !check_Bins_Array_ਮਾਪ(coverpointp, binp, integral)) ਜਾਰੀ;
            ਜੇ (!binp->isWildcard()
                || sizedWildcardRuns(binp, coverpointp->exprp())
                       <= v3Global.opt.coverageMaxBins()) {
                ਜੇ (binWith(binp) && withCandidatesOver(binp, coverpointp->exprp())) {
                    binp->v3warn(COVERIGN, "Unsupported: 'with' filter of more than 2**32 "
                                           "candidate values; bin "
                                               << binp->prettyNameQ() << " ignored");
                    ਜੇ (binp->binsType().bin_Is_ਸਾਧਾਰਨ()) coverpointp->user2(ਸੱਚ);
                    dropBins(coverpointp, binp);
                }
                ਜਾਰੀ;
            }
            // An ignore or illegal array still excludes or checks its values, as one bin
            ਅਚਲ ਤਾਰਕਿਕ single = !binp->binsType().bin_Is_ਸਾਧਾਰਨ() && !binWith(binp);
            binp->v3warn(COVERIGN,
                         "Unsupported: " << (binWith(binp) ? "'with' filter of wildcard '"
                                                           : "sized wildcard array '")
                                         << binp->binsType().verilogKwd()
                                         << "' of more than --coverage-max-bins of "
                                         << v3Global.opt.coverageMaxBins()
                                         << " ranges of values; bin " << binp->prettyNameQ()
                                         << (single ? " treated as one bin" : " ignored") << "\n"
                                         << binp->warn_ਹੋਰ()
                                         << "... Suggest a larger --coverage-max-bins");
            ਜੇ (single) {
                Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ sizep = binp->arraySizep();
                VL_DO_DANGLING(ਧੱਕੋ_Deletep(sizep->unlink_Fr_ਪਿੱਠ()), sizep);
                binp->isArray(ਝੂਠ);
                ਜਾਰੀ;
            }
            ਜੇ (binp->binsType().bin_Is_ਸਾਧਾਰਨ()) coverpointp->user2(ਸੱਚ);
            dropBins(coverpointp, binp);
        }
    }

    // True if a 'with' filter would be evaluated for more than 2**32 candidate values, known
    // now for the coverpoint's name, or for a range list of constants: each is evaluated once
    // but for an array 'b[N]', which keeps their order and duplicates (see withBegin())
    ਸਥਿਰ ਤਾਰਕਿਕ withCandidatesOver(AstCoverBin* binp, Ast_ਗੰਢ_ਵਿਅੰਜਕ* exprp) {
        ਅਚਲ ਪੂਰਨ ਚੌੜਾਈ = ਲੜੀਵਾਰ_ਚੌੜਾਈ(exprp);
        std::ਸਦਿਸ਼<std::pair<V3_ਸੰਖਿਆ, V3_ਸੰਖਿਆ>> runs;
        ਜੇ (!binRangesp(binp)) runs = coverpoint_ਮੁੱਲਾਂ(binp, exprp);
        ਅਚਲ ਸਵੈਚਾਲਿਤ constant
            = [](ਅਚਲ Ast_ਗੰਢ* nodep) { ਵਾਪਸ VN_IS(nodep, Const) || VN_IS(nodep, Unbounded); };
        ਲਈ (Ast_ਗੰਢ* rangep = binRangesp(binp); rangep; rangep = rangep->nextp()) {
            ਅਚਲ AstInsideRange* ਅਚਲ irp = VN_CAST(rangep, InsideRange);
            // Else known when constructed
            ਜੇ (irp ? !constant(irp->lhsp()) || !constant(irp->rhsp()) : !VN_IS(rangep, Const)) {
                ਵਾਪਸ ਝੂਠ;
            }
            Cross_ਮੁੱਲ_Range range{rangep, resolve_ਚੌੜਾਈ(rangep, exprp)};
            ਜੇ (!resolve_ਮੁੱਲ(rangep, exprp, ਸੱਚ, binp->isWildcard(), range)
                || cross_Range_ਖਾਲੀ_ਥਾਵਾਂ(range)) {
                ਜਾਰੀ;
            }
            std::ਸਦਿਸ਼<std::pair<V3_ਸੰਖਿਆ, V3_ਸੰਖਿਆ>> ਮਿਲਿਆ{{range.lo, range.hi}};
            ਜੇ (range.wildcard) {  // checkConstructedBins bounded the runs
                ਮਿਲਿਆ.clear();
                crossRangeRuns(range, v3Global.opt.coverageMaxBins(), ਮਿਲਿਆ);
            }
            ਲਈ (ਅਚਲ std::pair<V3_ਸੰਖਿਆ, V3_ਸੰਖਿਆ>& ਲੜੀਵਾਰ : ਮਿਲਿਆ) {
                runs.emplace_ਪਿੱਠ(V3_ਸੰਖਿਆ{rangep, ਚੌੜਾਈ, ਲੜੀਵਾਰ.ਪਹਿਲਾ},
                                  V3_ਸੰਖਿਆ{rangep, ਚੌੜਾਈ, ਲੜੀਵਾਰ.ਦੂਜਾ});
            }
        }
        ਜੇ (!isSizedArray(binp)) {  // The union of the values
            std::ਛਾਂਟੋ(runs.begin(), runs.ਸਿਰਾ(), [](ਅਚਲ ਸਵੈਚਾਲਿਤ& lhs, ਅਚਲ ਸਵੈਚਾਲਿਤ& rhs) {
                ਵਾਪਸ cross_ਮੁੱਲ_Less(lhs.ਪਹਿਲਾ, rhs.ਪਹਿਲਾ);
            });
            std::ਸਦਿਸ਼<std::pair<V3_ਸੰਖਿਆ, V3_ਸੰਖਿਆ>> merged;
            ਲਈ (ਅਚਲ std::pair<V3_ਸੰਖਿਆ, V3_ਸੰਖਿਆ>& ਲੜੀਵਾਰ : runs) {
                ਜੇ (merged.ਖਾਲੀ_ਥਾਵਾਂ() || cross_ਮੁੱਲ_Less(merged.ਪਿੱਠ().ਦੂਜਾ, ਲੜੀਵਾਰ.ਪਹਿਲਾ)) {
                    merged.ਧੱਕੋ_ਪਿੱਠ(ਲੜੀਵਾਰ);
                } ਨਹੀਂਤਾਂ ਜੇ (cross_ਮੁੱਲ_Less(merged.ਪਿੱਠ().ਦੂਜਾ, ਲੜੀਵਾਰ.ਦੂਜਾ)) {
                    merged.ਪਿੱਠ().ਦੂਜਾ = ਲੜੀਵਾਰ.ਦੂਜਾ;
                }
            }
            runs = std::move(merged);
        }
        uint64_t ਗਿਣਤੀ_2 = 0;
        ਲਈ (ਅਚਲ std::pair<V3_ਸੰਖਿਆ, V3_ਸੰਖਿਆ>& ਲੜੀਵਾਰ : runs) {
            V3_ਸੰਖਿਆ ਵਿਸਤਾਰ{exprp, ਚੌੜਾਈ};
            ਵਿਸਤਾਰ.opSub(ਲੜੀਵਾਰ.ਦੂਜਾ, ਲੜੀਵਾਰ.ਪਹਿਲਾ);
            ਜੇ (ਵਿਸਤਾਰ.most_ਸੈੱਟ_ਦੋਆਧਾਰੀ_ਅੰਕ_P1() > 32) ਵਾਪਸ ਸੱਚ;  // More than 2**32 values
            ਗਿਣਤੀ_2 += ਵਿਸਤਾਰ.ਮੰਜ਼ਿਲ_UQuad() + 1;
            ਜੇ (ਗਿਣਤੀ_2 > (uint64_t{1} << 32)) ਵਾਪਸ ਸੱਚ;
        }
        ਵਾਪਸ ਝੂਠ;
    }

    // The ranges of values the wildcard patterns of a sized wildcard array, or of a 'with'
    // filter's candidates, give, counted up to more than --coverage-max-bins
    ਸਥਿਰ ਮਾਪ_t sizedWildcardRuns(ਅਚਲ AstCoverBin* binp, Ast_ਗੰਢ_ਵਿਅੰਜਕ* exprp) {
        std::ਸਦਿਸ਼<std::pair<V3_ਸੰਖਿਆ, V3_ਸੰਖਿਆ>> runs;
        ਲਈ (Ast_ਗੰਢ* rangep = binRangesp(binp); rangep; rangep = rangep->nextp()) {
            ਜੇ (!VN_IS(rangep, Const)) ਜਾਰੀ;  // A range, or a value known at construction
            Cross_ਮੁੱਲ_Range range{rangep, resolve_ਚੌੜਾਈ(rangep, exprp)};
            ਜੇ (resolve_ਮੁੱਲ(rangep, exprp, ਸੱਚ, ਸੱਚ, range) && !cross_Range_ਖਾਲੀ_ਥਾਵਾਂ(range)) {
                crossRangeRuns(range, v3Global.opt.coverageMaxBins(), runs);
            }
        }
        ਵਾਪਸ runs.ਮਾਪ();
    }

    // Check the automatic bins declarations of a coverpoint.  Each stays one declaration, which
    // generates as a partition of the coverpoint domain (see autoBinRuns).
    ਖਾਲੀ checkAutomaticBins(AstCoverpoint* coverpointp, ਅਚਲ Ast_ਗੰਢ_ਵਿਅੰਜਕ* exprp) {
        ਲਈ (Ast_ਗੰਢ* binp = coverpointp->binsp(); binp; binp = binp->nextp()) {
            AstCoverBin* ਅਚਲ cbinp = VN_AS(binp, CoverBin);
            ਜੇ (cbinp->binsType() != VCoverBinsType::BINS_AUTO) ਜਾਰੀ;
            ਅਚਲ AstConst* ਅਚਲ constp = VN_CAST(cbinp->arraySizep(), Const);
            ਜੇ (!constp) {
                cbinp->v3error("Automatic bins array size must be a constant");
            } ਨਹੀਂਤਾਂ ਜੇ (const_ਗਿਣਤੀ(constp) < 1) {
                cbinp->v3error("Automatic bins array size must be >= 1, got "
                               << constp->ਅੰਕ().ਮੰਜ਼ਿਲ_Decimal_S());
            } ਨਹੀਂਤਾਂ ਜੇ (const_ਗਿਣਤੀ(constp) > v3Global.opt.coverageMaxBins()) {
                cbinp->v3error("Automatic bins array size of "
                               << constp->ਅੰਕ().ਮੰਜ਼ਿਲ_Decimal_U() << " exceeds limit of "
                               << v3Global.opt.coverageMaxBins() << '\n'
                               << cbinp->warn_ਹੋਰ() << "... Suggest a larger --coverage-max-bins");
            } ਨਹੀਂਤਾਂ ਜੇ (!exprp->dtypep()->skipRefp()->isIntegralOrPacked()) {
                cbinp->v3error("Automatic bins are not allowed on a coverpoint of a non-integral "
                               "expression (IEEE 1800-2023 19.5.3).");
            }
        }
    }

    // Extract all option values of a coverpoint or cross in a single pass.
    // atLeastOut: option.at_least (default the covergroup's)
    // autoBinMaxOut: option.auto_bin_max (coverpoint overrides covergroup, default 64)
    ਖਾਲੀ ਕੱਢੋ_ਵਸਤੂ_Options(Ast_ਗੰਢ* optionsp, ਪੂਰਨ& at_ਘੱਟ_ਤੋਂ_ਘੱਟ_ਬਾਹਰਲਾ, ਪੂਰਨ& auto_Bin_Max_ਬਾਹਰਲਾ) {
        at_ਘੱਟ_ਤੋਂ_ਘੱਟ_ਬਾਹਰਲਾ = m_cg_At_ਘੱਟ_ਤੋਂ_ਘੱਟ;
        auto_Bin_Max_ਬਾਹਰਲਾ = -1;  // -1 = not set at coverpoint level
        ਲਈ (Ast_ਗੰਢ* optionp = optionsp; optionp; optionp = optionp->nextp()) {
            AstCoverOption* ਅਚਲ optp = VN_AS(optionp, CoverOption);
            // Weights may be non-constant; generateItemWeight() handles them.  A cross keeps
            // its unsupported options, which V3LinkParse warned of.
            ਜੇ (!(optp->optType() == VCoverOptionType::AT_ਘੱਟ_ਤੋਂ_ਘੱਟ
                  || optp->optType() == VCoverOptionType::AUTO_BIN_MAX)) {
                ਜਾਰੀ;
            }
            ਅਚਲ AstConst* ਅਚਲ constp = constOption(optp->valuep(), optp->optType());
            ਜੇ (!constp) ਜਾਰੀ;
            ਜੇ (optp->optType() == VCoverOptionType::AT_ਘੱਟ_ਤੋਂ_ਘੱਟ) {
                at_ਘੱਟ_ਤੋਂ_ਘੱਟ_ਬਾਹਰਲਾ = const_At_ਘੱਟ_ਤੋਂ_ਘੱਟ(constp);
            } ਨਹੀਂਤਾਂ {
                auto_Bin_Max_ਬਾਹਰਲਾ = constp->ਮੰਜ਼ਿਲ_SInt();
            }
        }
        // Fall back to covergroup-level auto_bin_max if not set at coverpoint level
        ਜੇ (auto_Bin_Max_ਬਾਹਰਲਾ < 0) {
            ਜੇ (m_covergroupp->cgAutoBinMax() >= 0) {
                auto_Bin_Max_ਬਾਹਰਲਾ = m_covergroupp->cgAutoBinMax();
            } ਨਹੀਂਤਾਂ {
                auto_Bin_Max_ਬਾਹਰਲਾ = 64;  // Default per IEEE 1800-2023 Table 19-1
            }
        }
    }

    // IEEE 1800-2023 19.5.2: an enum coverpoint has one automatic bin per enumeration value
    ਖਾਲੀ createEnumAutoBins(AstCoverpoint* coverpointp, Ast_ਗੰਢ_ਵਿਅੰਜਕ* exprp,
                            ਅਚਲ AstEnumDType* enump) {
        File_ਸਤਰ* ਅਚਲ fl = coverpointp->fileline();
        ਲਈ (ਅਚਲ Ast_Enum_ਵਸਤੂ* itemp = enump->itemsp(); itemp;
             itemp = VN_AS(itemp->nextp(), Enum_ਵਸਤੂ)) {
            AstConst* ਅਚਲ lop = new_ਮੁੱਲ_Const(fl, VN_AS(itemp->valuep(), Const)->ਅੰਕ(), exprp);
            AstInsideRange* ਅਚਲ rangep = ਨਵਾਂ AstInsideRange{fl, lop, lop->clone_ਰੁੱਖ(ਝੂਠ)};
            rangep->dtypeFrom(exprp);
            coverpointp->ਜੋੜੋ_Binsp(
                ਨਵਾਂ AstCoverBin{fl, "auto[" + itemp->name() + "]", rangep, ਝੂਠ, ਝੂਠ});
        }
    }

    // IEEE 1800-2023 19.5.3/19.11.1: partition first, then apply exclusions.  The partition is one
    // automatic bins declaration, generated as a run like 'bins auto[N]' but numbering its bins.
    ਖਾਲੀ createImplicitAutoBins(AstCoverpoint* coverpointp, Ast_ਗੰਢ_ਵਿਅੰਜਕ* exprp, ਪੂਰਨ autoBinMax) {
        ਜੇ (coverpointp->user2()) ਵਾਪਸ;  // Declared bins, ignored, leave no bins
        ਲਈ (Ast_ਗੰਢ* nodep = coverpointp->binsp(); nodep; nodep = nodep->nextp()) {
            ਅਚਲ VCoverBinsType ਕਿਸਮ = VN_AS(nodep, CoverBin)->binsType();
            ਜੇ (ਕਿਸਮ != VCoverBinsType::BINS_IGNORE && ਕਿਸਮ != VCoverBinsType::BINS_ILLEGAL)
                ਵਾਪਸ;
        }
        ਜੇ (ਅਚਲ AstEnumDType* ਅਚਲ enump
            = VN_CAST(exprp->dtypep()->skip_Ref_ਮੰਜ਼ਿਲ_Enump(), EnumDType)) {
            createEnumAutoBins(coverpointp, exprp, enump);
            ਵਾਪਸ;
        }
        ਅਚਲ ਪੂਰਨ ਚੌੜਾਈ = exprp->ਚੌੜਾਈ();
        uint32_t ਗਿਣਤੀ_2 = ਚੌੜਾਈ < 31 ? std::min<uint32_t>(uint32_t{1} << ਚੌੜਾਈ, autoBinMax)
                                    : static_cast<uint32_t>(autoBinMax);
        ਜੇ (!ਗਿਣਤੀ_2) ਵਾਪਸ;
        ਜੇ (!exprp->dtypep()->skipRefp()->isIntegralOrPacked()) {
            coverpointp->v3error("Coverpoint of a non-integral expression requires explicit bins "
                                 "(IEEE 1800-2023 19.5.3).");
            ਵਾਪਸ;
        }
        ਜੇ (ਗਿਣਤੀ_2 > v3Global.opt.coverageMaxBins()) {
            coverpointp->v3warn(COVERIGN, "Unsupported: more than "
                                              << v3Global.opt.coverageMaxBins()
                                              << " automatic bins from 'option.auto_bin_max'; "
                                                 "using "
                                              << v3Global.opt.coverageMaxBins() << ".\n"
                                              << coverpointp->warn_ਹੋਰ()
                                              << "... Suggest a larger --coverage-max-bins");
            ਗਿਣਤੀ_2 = v3Global.opt.coverageMaxBins();
        }
        File_ਸਤਰ* ਅਚਲ fl = coverpointp->fileline();
        coverpointp->ਜੋੜੋ_Binsp(ਨਵਾਂ AstCoverBin{fl, "auto", ਨਵਾਂ AstConst{fl, ਗਿਣਤੀ_2},
                                              VCoverBinsType::BINS_AUTO_IMPLICIT});
    }

    // Sanitize generated names to be valid C++ identifiers
    ਸਥਿਰ string sanitizeGeneratedName(string name) {
        std::replace(name.begin(), name.ਸਿਰਾ(), '[', '_');
        std::replace(name.begin(), name.ਸਿਰਾ(), ']', '_');
        ਵਾਪਸ name;
    }

    // Capture an iff guard in a function-local temporary so it is evaluated once per sample()
    AstVarRef* capture_Iff_ਮੰਜ਼ਿਲ_ਆਰਜ਼ੀ(Ast_ਗੰਢ_ਵਿਅੰਜਕ* iffp, ਅਚਲ string& ਆਰਜ਼ੀ_Name) {
        File_ਸਤਰ* ਅਚਲ fl = iffp->fileline();
        AstVar* ਅਚਲ iffVarp
            = ਨਵਾਂ AstVar{fl, VVarType::BLOCKTEMP, ਆਰਜ਼ੀ_Name, iffp->ਲੱਭੋ_ਦੋਆਧਾਰੀ_ਅੰਕ_DType()};
        iffVarp->funcLocal(ਸੱਚ);
        m_sampleFuncp->ਜੋੜੋ_Stmtsp(iffVarp);
        iffp->unlink_Fr_ਪਿੱਠ();
        m_sampleFuncp->ਜੋੜੋ_Stmtsp(
            ਨਵਾਂ Ast_ਸੌਂਪੋ{fl, ਨਵਾਂ AstVarRef{fl, iffVarp, VAccess::WRITE}, iffp});
        ਵਾਪਸ ਨਵਾਂ AstVarRef{fl, iffVarp, VAccess::READ};
    }

    // Add a statement sampling the current coverpoint to sample(), under its iff guard if any
    ਖਾਲੀ ਜੋੜੋ_Sample_Stmt(Ast_ਗੰਢ* stmtp) {
        ਜੇ (m_cpGuardp) {
            m_cpGuardp->ਜੋੜੋ_Thensp(stmtp);
        } ਨਹੀਂਤਾਂ {
            m_sampleFuncp->ਜੋੜੋ_Stmtsp(stmtp);
        }
    }

    // Create previous value variable for transition tracking
    AstVar* create_Prev_ਮੁੱਲ_Var(AstCoverpoint* coverpointp, Ast_ਗੰਢ_ਵਿਅੰਜਕ* exprp) {
        // Check if already created
        ਜੇ (AstVar* ਅਚਲ prevVarp = VN_CAST(coverpointp->user1p(), Var)) ਵਾਪਸ prevVarp;

        // Create variable to store previous sampled value
        ਅਚਲ string varName = "__Vprev_" + coverpointp->name();
        AstVar* prevVarp
            = ਨਵਾਂ AstVar{coverpointp->fileline(), VVarType::MEMBER, varName, exprp->dtypep()};
        m_covergroupp->ਜੋੜੋ_Membersp(prevVarp);

        UINFO(4, "    Created previous value variable: " << varName);

        // Initialize to zero in constructor
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ initExprp
            = ਨਵਾਂ AstConst{prevVarp->fileline(), AstConst::Widthed_ਮੁੱਲ{}, prevVarp->ਚੌੜਾਈ(), 0};
        Ast_ਗੰਢ_Stmt* ਅਚਲ initStmtp = ਨਵਾਂ Ast_ਸੌਂਪੋ{
            prevVarp->fileline(), ਨਵਾਂ AstVarRef{prevVarp->fileline(), prevVarp, VAccess::WRITE},
            initExprp};
        m_constructorp->ਜੋੜੋ_Stmtsp(initStmtp);

        coverpointp->user1p(prevVarp);
        ਵਾਪਸ prevVarp;
    }

    // Create state position variable for multi-value transition bins
    // Tracks position in sequence: 0=not started, 1=seen first item, etc.
    AstVar* createSequenceStateVar(AstCoverpoint* coverpointp, AstCoverBin* binp) {
        // Create variable to track sequence position
        ਅਚਲ string varName = "__Vseqpos_" + coverpointp->name() + "_" + binp->name();
        // Use 8-bit integer for state position (sequences rarely > 255 items)
        AstVar* stateVarp
            = ਨਵਾਂ AstVar{binp->fileline(), VVarType::MEMBER, varName, VFlagLogicPacked{}, 8};
        m_covergroupp->ਜੋੜੋ_Membersp(stateVarp);

        UINFO(4, "    Created sequence state variable: " << varName);

        // Initialize to 0 (not started) in constructor
        Ast_ਗੰਢ_Stmt* ਅਚਲ initStmtp = ਨਵਾਂ Ast_ਸੌਂਪੋ{
            stateVarp->fileline(), ਨਵਾਂ AstVarRef{stateVarp->fileline(), stateVarp, VAccess::WRITE},
            ਨਵਾਂ AstConst{stateVarp->fileline(), AstConst::Widthed_ਮੁੱਲ{}, 8, 0}};
        m_constructorp->ਜੋੜੋ_Stmtsp(initStmtp);

        ਵਾਪਸ stateVarp;
    }

    ਖਾਲੀ generate_Coverpoint_ਕੋਡ(AstCoverpoint* coverpointp) {
        UINFO(4, "  Generating code for coverpoint: " << coverpointp->name());

        // Get the coverpoint expression
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* exprp = coverpointp->exprp();

        // Check automatic bins before processing
        checkAutomaticBins(coverpointp, exprp);

        // Extract all coverpoint options in a single pass
        ਪੂਰਨ at_ਘੱਟ_ਤੋਂ_ਘੱਟ_ਮੁੱਲ;
        ਪੂਰਨ autoBinMax;
        ਕੱਢੋ_ਵਸਤੂ_Options(coverpointp->optionsp(), at_ਘੱਟ_ਤੋਂ_ਘੱਟ_ਮੁੱਲ, autoBinMax);
        UINFO(6, "    Coverpoint at_least = " << at_ਘੱਟ_ਤੋਂ_ਘੱਟ_ਮੁੱਲ << " auto_bin_max = " << autoBinMax);

        // Create implicit automatic bins if no regular bins exist
        createImplicitAutoBins(coverpointp, exprp, autoBinMax);

        // A false iff guard disables sampling of the coverpoint (IEEE 1800-2023 19.5), so
        // neither is its expression evaluated, nor are its bins or transition state updated
        VL_RESTORER(m_cpGuardp);
        ਜੇ (Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ iffp = coverpointp->iffp()) {
            m_cpGuardp = ਨਵਾਂ AstIf{iffp->fileline(), iffp->unlink_Fr_ਪਿੱਠ()};
        }

        AstVar* ਅਚਲ ਮੁੱਲ_Varp = ਨਵਾਂ AstVar{
            coverpointp->fileline(), VVarType::BLOCKTEMP,
            "__VcpValue_" + sanitizeGeneratedName(coverpointp->name()), exprp->dtypep()};
        ਮੁੱਲ_Varp->funcLocal(ਸੱਚ);
        m_sampleFuncp->ਜੋੜੋ_Stmtsp(ਮੁੱਲ_Varp);
        exprp->unlink_Fr_ਪਿੱਠ();
        ਜੋੜੋ_Sample_Stmt(ਨਵਾਂ Ast_ਸੌਂਪੋ{
            coverpointp->fileline(),
            ਨਵਾਂ AstVarRef{coverpointp->fileline(), ਮੁੱਲ_Varp, VAccess::WRITE}, exprp});
        coverpointp->exprp(ਨਵਾਂ AstVarRef{coverpointp->fileline(), ਮੁੱਲ_Varp, VAccess::READ});
        exprp = coverpointp->exprp();

        // Every coverpoint routes through the VlCoverpoint runtime.  Transition coverpoints are
        // included: their per-value matching is still generated as a state machine in sample()
        // (see generateCoverpoint), but the bin hit is recorded in the runtime bin
        // rather than a bare counter.
        generateCoverpoint(coverpointp, exprp, at_ਘੱਟ_ਤੋਂ_ਘੱਟ_ਮੁੱਲ);
        // Sampling follows what generateCoverpoint added unguarded, such as clearing the hit list
        ਜੇ (m_cpGuardp) m_sampleFuncp->ਜੋੜੋ_Stmtsp(m_cpGuardp);
    }

    // Build the condition under which a default bin matches: NOT(OR of all normal bins).
    Ast_ਗੰਢ_ਵਿਅੰਜਕ* buildDefaultCondition(AstCoverpoint* coverpointp, Ast_ਗੰਢ_ਵਿਅੰਜਕ* exprp,
                                       File_ਸਤਰ* fl) {
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* anyBinMatchp = ਖਾਲੀ_ਸੂਚਕ;
        ਲਈ (Ast_ਗੰਢ* binp = coverpointp->binsp(); binp; binp = binp->nextp()) {
            AstCoverBin* ਅਚਲ cbinp = VN_AS(binp, CoverBin);
            ਜੇ (cbinp->binsType() == VCoverBinsType::BINS_DEFAULT
                || cbinp->binsType() == VCoverBinsType::BINS_IGNORE
                || cbinp->binsType() == VCoverBinsType::BINS_ILLEGAL)
                ਜਾਰੀ;
            // The values of constructed bins are known at construction; see emitSizedSample
            ਜੇ (isConstructedBins(cbinp)) ਜਾਰੀ;
            ਜੇ (isAutoBins(cbinp)) {
                // Automatic bins partition the whole domain, leaving no default value
                ਜੇ (anyBinMatchp) VL_DO_DANGLING(ਧੱਕੋ_Deletep(anyBinMatchp), anyBinMatchp);
                ਵਾਪਸ ਨਵਾਂ AstConst{fl, AstConst::ਦੋਆਧਾਰੀ_ਅੰਕ_False{}};
            }
            Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ binCondp = buildBinCondition(cbinp, exprp);
            UASSERT_OBJ(binCondp, cbinp,
                        "buildBinCondition returned nullptr for non-ignore/non-illegal bin");
            anyBinMatchp = anyBinMatchp ? ਨਵਾਂ AstOr{fl, anyBinMatchp, binCondp} : binCondp;
        }
        ਵਾਪਸ anyBinMatchp ? static_cast<Ast_ਗੰਢ_ਵਿਅੰਜਕ*>(ਨਵਾਂ AstNot{fl, anyBinMatchp})
                            : static_cast<Ast_ਗੰਢ_ਵਿਅੰਜਕ*>(ਨਵਾਂ AstConst{fl, AstConst::ਦੋਆਧਾਰੀ_ਅੰਕ_True{}});
    }

    //====================================================================
    // VlCoverpoint conversion

    ਸਥਿਰ ਤਾਰਕਿਕ coverpointHasStateExclusions(ਅਚਲ AstCoverpoint* coverpointp) {
        ਲਈ (ਅਚਲ Ast_ਗੰਢ* nodep = coverpointp->binsp(); nodep; nodep = nodep->nextp()) {
            ਅਚਲ AstCoverBin* ਅਚਲ binp = VN_AS(nodep, CoverBin);
            ਜੇ (!binp->transp() && binp->rangesp()
                && (binp->binsType() == VCoverBinsType::BINS_IGNORE
                    || binp->binsType() == VCoverBinsType::BINS_ILLEGAL)) {
                ਵਾਪਸ ਸੱਚ;
            }
        }
        ਵਾਪਸ ਝੂਠ;
    }

    ਸਥਿਰ ਤਾਰਕਿਕ coverpointHasConstructedBins(ਅਚਲ AstCoverpoint* coverpointp) {
        ਲਈ (ਅਚਲ Ast_ਗੰਢ* nodep = coverpointp->binsp(); nodep; nodep = nodep->nextp()) {
            ਜੇ (isConstructedBins(VN_AS(nodep, CoverBin))) ਵਾਪਸ ਸੱਚ;
        }
        ਵਾਪਸ ਝੂਠ;
    }

    // True if a Normal state bin, or an array-bin element, has no value of the coverpoint's
    // type (IEEE 1800-2023 19.5.7).  Array ranges enumerate in-type values, so cannot vanish.
    ਸਥਿਰ ਤਾਰਕਿਕ coverpoint_Has_ਖਾਲੀ_ਥਾਵਾਂ_Bins(ਅਚਲ AstCoverpoint* coverpointp) {
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ exprp = coverpointp->exprp();
        ਲਈ (Ast_ਗੰਢ* nodep = coverpointp->binsp(); nodep; nodep = nodep->nextp()) {
            ਅਚਲ AstCoverBin* ਅਚਲ binp = VN_AS(nodep, CoverBin);
            ਜੇ (!binp->binsType().bin_Is_ਸਾਧਾਰਨ() || binp->transp() || !binp->rangesp()) ਜਾਰੀ;
            // A wildcard array has a bin for each value its elements match, so none empty, and
            // the constructor creates no bin without values of a 'with' filter
            ਜੇ ((binp->isArray() && binp->isWildcard()) || binWith(binp)) ਜਾਰੀ;
            ਤਾਰਕਿਕ ਖਾਲੀ_ਥਾਵਾਂ = ਸੱਚ;
            ਲਈ (Ast_ਗੰਢ* valuep = binp->rangesp(); valuep; valuep = valuep->nextp()) {
                ਜੇ (binp->isArray() && VN_IS(valuep, InsideRange)) ਜਾਰੀ;
                Cross_ਮੁੱਲ_Range range{valuep, resolve_ਚੌੜਾਈ(valuep, exprp)};
                ਅਚਲ ਤਾਰਕਿਕ none = resolve_ਮੁੱਲ(valuep, exprp, ਸੱਚ, binp->isWildcard(), range)
                                  && cross_Range_ਖਾਲੀ_ਥਾਵਾਂ(range);
                ਜੇ (binp->isArray() && none) ਵਾਪਸ ਸੱਚ;
                ਖਾਲੀ_ਥਾਵਾਂ &= none;
            }
            ਜੇ (ਖਾਲੀ_ਥਾਵਾਂ && !binp->isArray()) ਵਾਪਸ ਸੱਚ;
        }
        ਵਾਪਸ ਝੂਠ;
    }

    // True if a coverpoint has any transition bin.  Used to decide whether sample() emits the
    // end-of-sample previous-value update that transition matching needs.
    ਸਥਿਰ ਤਾਰਕਿਕ coverpointHasTransition(AstCoverpoint* coverpointp) {
        ਲਈ (Ast_ਗੰਢ* binp = coverpointp->binsp(); binp; binp = binp->nextp()) {
            ਜੇ (VN_AS(binp, CoverBin)->transp()) ਵਾਪਸ ਸੱਚ;
        }
        ਵਾਪਸ ਝੂਠ;
    }

    // The interned AstBasicDType for one of the covergroup runtime keywords.
    ਸਥਿਰ AstBasicDType* basicDType(File_ਸਤਰ* fl, VBasicDTypeKwd kwd) {
        ਵਾਪਸ v3Global.rootp()->typeTablep()->ਲੱਭੋ_Basic_DType(fl, kwd);
    }

    // Get (or create) the coverpoint dtype for a hit-list bound, interned so each distinct bound
    // yields one node.
    AstCoverpointDType* coverpointDType(File_ਸਤਰ* fl, uint32_t ਪ੍ਰਾਪਤੀ_Bound) {
        AstCoverpointDType*& typep = m_cpDTypes[ਪ੍ਰਾਪਤੀ_Bound];
        ਜੇ (!typep) {
            typep = ਨਵਾਂ AstCoverpointDType{fl, ਪ੍ਰਾਪਤੀ_Bound};
            v3Global.rootp()->typeTablep()->ਜੋੜੋ_Typesp(typep);
        }
        ਵਾਪਸ typep;
    }

    // The key of the covergroup's type in the coverage registry, which get_coverage() queries
    std::string covergroupProtectedName() ਅਚਲ {
        ਵਾਪਸ VIdProtect::protect_ਸ਼ਬਦ_If(m_covergroupName, v3Global.opt.protectIds());
    }

    // The name of the covergroup's type, which keys the coverage registry and database: as
    // $typename names it (IEEE 1800-2023 20.6.1), so apart for covergroups of distinct scopes and
    // specializations, and alike in each Verilator run of a hierarchical design.  As design units
    // of distinct libraries may share a name, one of a library other than the default is prefixed
    // by its library, as '%l' prints it (IEEE 1800-2023 33.4).
    std::string covergroupTypeName() ਅਚਲ {
        UASSERT_OBJ(m_unitp, m_covergroupp, "Covergroup declared outside of a design unit");
        ਅਚਲ std::string& libname = m_unitp->libname();
        ਅਚਲ std::string name = m_covergroupp->dtypeName(ਸੱਚ);
        ਵਾਪਸ libname == "work" ? name : libname + "." + name;
    }

    // Emit the covergroup's instance handle member and the constructor statement that creates
    // its node in the per-context coverage registry.  Runs before any coverpoint or cross is
    // generated, so their runtimes can be added to the node as they are created.
    ਖਾਲੀ generateInstanceAttach() {
        File_ਸਤਰ* ਅਚਲ fl = m_covergroupp->fileline();
        // V3LinkParse synthesizes a 'new' for every covergroup; the item generators below already
        // rely on that, and this attach runs even for a covergroup with no coverpoints at all.
        UASSERT_OBJ(m_constructorp, m_covergroupp, "Covergroup missing synthesized constructor");
        m_cgInstVarp = ਨਵਾਂ AstVar{fl, VVarType::MEMBER, "__Vcg_inst",
                                  basicDType(fl, VBasicDTypeKwd::COVERGROUP_INSTHANDLE)};
        m_covergroupp->ਜੋੜੋ_Membersp(m_cgInstVarp);

        m_constructorp->ਜੋੜੋ_Stmtsp(
            ਵਸਤੂ_Call(fl, m_cgInstVarp, VCMethod::COVERGROUP_ATTACH,
                     {ctext(fl, "vlSymsp->_vm_contextp__->covergroupRegistryp()"
                                "->newCovergroupInst("
                                    + quoted(covergroupProtectedName())
                                    + (m_cg_May_ਮਿਲਾਓ ? ", true" : ", false") + ")")},
                     /*usePtr=*/ਝੂਠ)
                ->makeStmt());
        // The node reads option.weight in place, so procedural assignments take effect
        AstCExpr* ਅਚਲ ਭਾਰ_Addrp = ਨਵਾਂ AstCExpr{fl, "&"};
        ਭਾਰ_Addrp->ਜੋੜੋ(newOptionSel(fl, optionVar(ਝੂਠ), "weight", VAccess::READ));
        m_constructorp->ਜੋੜੋ_Stmtsp(ਵਸਤੂ_Call(fl, m_cgInstVarp, VCMethod::COVERGROUP_LEND_ਭਾਰ,
                                           {ਭਾਰ_Addrp, file_ਸਤਰ_Debug(fl)}, /*usePtr=*/ਝੂਠ)
                                      ->makeStmt());
    }

    // A '__Vcg_inst.p()-><method>()' call on the covergroup's instance node
    AstCMethodHard* instanceCall(File_ਸਤਰ* fl, VCMethod ਵਿਧੀ) {
        // '__Vcg_inst.p()' -- a value handle, so '.' not '->'
        AstCMethodHard* ਅਚਲ instp
            = ਨਵਾਂ AstCMethodHard{fl, memberRef(fl, m_cgInstVarp), VCMethod::COVERGROUP_INST_P};
        instp->usePtr(ਝੂਠ);
        instp->dtype_ਸੈੱਟ_Void();  // Opaque receiver; only ever the 'fromp' of the call below
        AstCMethodHard* ਅਚਲ callp = ਨਵਾਂ AstCMethodHard{fl, instp, ਵਿਧੀ};
        callp->usePtr(ਸੱਚ);
        ਵਾਪਸ callp;
    }

    // Emit 'this->__Vcp_x = this->__Vcg_inst.p()->addCoverpoint<K>();' (or addCross), which
    // creates the item runtime in the instance node and borrows a pointer to it.
    Ast_ਸੌਂਪੋ* make_ਵਸਤੂ_Create(File_ਸਤਰ* fl, AstVar* ਵਸਤੂ_Varp, VCMethod ਵਿਧੀ) {
        AstCMethodHard* ਅਚਲ createp = instanceCall(fl, ਵਿਧੀ);
        createp->dtypep(ਵਸਤੂ_Varp->dtypep());
        ਵਾਪਸ ਨਵਾਂ Ast_ਸੌਂਪੋ{fl, memberRef(fl, ਵਸਤੂ_Varp, VAccess::WRITE), createp};
    }

    // Constant bounds of one rangesp() element (an InsideRange or a single Const).  Each bound is
    // the raw AST node -- an AstConst or an AstUnbounded ('$') -- with the const/unbounded view
    // derived on demand, so there is one source of truth per bound.  After a successful
    // constRangeBounds() neither node is null; a single Const has both bounds aliasing one node.
    ਬਣਤਰ RangeBounds final {
        Ast_ਗੰਢ* loNodep = ਖਾਲੀ_ਸੂਚਕ;  // low bound: AstConst or AstUnbounded
        Ast_ਗੰਢ* hiNodep = ਖਾਲੀ_ਸੂਚਕ;  // high bound: AstConst or AstUnbounded
        ਤਾਰਕਿਕ loUnbounded() ਅਚਲ { ਵਾਪਸ VN_IS(loNodep, Unbounded); }
        ਤਾਰਕਿਕ hiUnbounded() ਅਚਲ { ਵਾਪਸ VN_IS(hiNodep, Unbounded); }
        AstConst* loConstp() ਅਚਲ { ਵਾਪਸ VN_CAST(loNodep, Const); }
        AstConst* hiConstp() ਅਚਲ { ਵਾਪਸ VN_CAST(hiNodep, Const); }
    };

    // Decode one rangesp() element into its constant bounds.  Returns false if rp is neither an
    // InsideRange nor a single Const, or if a present bound is non-constant or 4-state.  '$'
    // bounds are left as AstUnbounded (not resolved) -- the caller applies its own policy.
    // Centralizes the InsideRange/Const/Unbounded decode shared by the hit-list-bound paths.
    ਸਥਿਰ ਤਾਰਕਿਕ constRangeBounds(Ast_ਗੰਢ* rp, RangeBounds& rb) {
        ਜੇ (AstInsideRange* ਅਚਲ irp = VN_CAST(rp, InsideRange)) {
            rb.loNodep = irp->lhsp();
            rb.hiNodep = irp->rhsp();
        } ਨਹੀਂਤਾਂ ਜੇ (AstConst* ਅਚਲ cp = VN_CAST(rp, Const)) {
            rb.loNodep = rb.hiNodep = cp;
        } ਨਹੀਂਤਾਂ {
            ਵਾਪਸ ਝੂਠ;
        }
        // Each bound must be a constant unless it is '$'; reject non-const and 4-state.
        AstConst* ਅਚਲ lc = rb.loConstp();
        AstConst* ਅਚਲ hc = rb.hiConstp();
        ਜੇ ((!lc && !rb.loUnbounded()) || (!hc && !rb.hiUnbounded())) ਵਾਪਸ ਝੂਠ;
        ਜੇ ((lc && lc->ਅੰਕ().isFourState()) || (hc && hc->ਅੰਕ().isFourState())) ਵਾਪਸ ਝੂਠ;
        ਵਾਪਸ ਸੱਚ;
    }

    // Collect the covered value intervals of a single (non-array) Normal bin.  Returns false
    // if any range isn't a constant/open InsideRange or single Const (e.g. wildcard, non-const).
    ਸਥਿਰ ਤਾਰਕਿਕ ਕੱਢੋ_Range_Intervals(AstCoverBin* cbinp, uint64_t maxVal,
                                      std::ਸਦਿਸ਼<std::pair<uint64_t, uint64_t>>& ਬਾਹਰਲਾ) {
        ਜੇ (!cbinp->rangesp()) ਵਾਪਸ ਝੂਠ;
        ਲਈ (Ast_ਗੰਢ* rp = cbinp->rangesp(); rp; rp = rp->nextp()) {
            RangeBounds rb;
            ਜੇ (!constRangeBounds(rp, rb)) ਵਾਪਸ ਝੂਠ;
            ਜੇ ((rb.loConstp() && rb.loConstp()->ਚੌੜਾਈ() > 64)
                || (rb.hiConstp() && rb.hiConstp()->ਚੌੜਾਈ() > 64)) {
                ਵਾਪਸ ਝੂਠ;  // Use the safe slot-count bound for wide values.
            }
            ਅਚਲ uint64_t lo = rb.loUnbounded() ? 0 : rb.loConstp()->ਮੰਜ਼ਿਲ_UQuad();
            ਅਚਲ uint64_t hi = rb.hiUnbounded() ? maxVal : rb.hiConstp()->ਮੰਜ਼ਿਲ_UQuad();
            ਜੇ (lo > hi) ਵਾਪਸ ਝੂਠ;
            ਬਾਹਰਲਾ.emplace_ਪਿੱਠ(lo, hi);
        }
        ਵਾਪਸ ਸੱਚ;
    }

    // Append one Normal bin's cross-slot interval-sets to `bins` and bump `slotCount` by an upper
    // bound of the bin's slots holding one value.  Returns false if any part isn't statically
    // enumerable (the caller then falls back to the always-safe slot count).  A non-array bin
    // is one slot covering the union of its intervals; the bins of an array element or of an
    // automatic bins declaration hold disjoint values, so the element or declaration counts once.
    ਤਾਰਕਿਕ appendBinCrossSlots(AstCoverBin* cbinp, uint64_t maxVal, Ast_ਗੰਢ_ਵਿਅੰਜਕ* exprp,
                             std::ਸਦਿਸ਼<std::ਸਦਿਸ਼<std::pair<uint64_t, uint64_t>>>& bins,
                             ਪੂਰਨ& ਥਾਂ_ਗਿਣਤੀ) {
        ਜੇ (isAutoBins(cbinp)) {
            ++ਥਾਂ_ਗਿਣਤੀ;
            bins.ਧੱਕੋ_ਪਿੱਠ({{0, maxVal}});
            ਵਾਪਸ exprp->ਚੌੜਾਈ() <= 64;
        }
        ਜੇ (binWith(cbinp)) {
            // A value is in one bin of a filter's, but of a sized array, in one for each range
            // list element holding it; the coverpoint's name is one element
            ਪੂਰਨ elements = 0;
            ਲਈ (ਅਚਲ Ast_ਗੰਢ* rp = binRangesp(cbinp); rp && isSizedArray(cbinp);
                 rp = rp->nextp()) {
                ++elements;
            }
            ਥਾਂ_ਗਿਣਤੀ += std::max(1, elements);
            ਵਾਪਸ ਝੂਠ;
        }
        ਜੇ (cbinp->isArray() && cbinp->isWildcard() && !cbinp->arraySizep()) {
            // A value is in at most one bin of a wildcard array: one slot covering its values.
            // Signed values are sign-extended, not unsigned intervals (see computeHitListBound)
            ++ਥਾਂ_ਗਿਣਤੀ;
            ਜੇ (exprp->isSigned() || exprp->ਚੌੜਾਈ() > 64) ਵਾਪਸ ਝੂਠ;
            ਅਚਲ BinRuns runs = wildcardBinRuns(cbinp, exprp, ਝੂਠ);
            ਜੇ (runs.unsupported) ਵਾਪਸ ਝੂਠ;
            std::ਸਦਿਸ਼<std::pair<uint64_t, uint64_t>> ivs;
            ਲਈ (ਅਚਲ Bin_ਲੜੀਵਾਰ& ਲੜੀਵਾਰ : runs.runs) {
                ivs.emplace_ਪਿੱਠ(ਲੜੀਵਾਰ.m_lo.ਮੰਜ਼ਿਲ_UQuad(), ਲੜੀਵਾਰ.m_hi.ਮੰਜ਼ਿਲ_UQuad());
            }
            bins.ਧੱਕੋ_ਪਿੱਠ(std::move(ivs));
            ਵਾਪਸ ਸੱਚ;
        }
        ਜੇ (cbinp->isArray()) ਵਾਪਸ appendArrayBinCrossSlots(cbinp, exprp, bins, ਥਾਂ_ਗਿਣਤੀ);
        // Non-array bin: one slot covering the union of its intervals.
        ++ਥਾਂ_ਗਿਣਤੀ;
        std::ਸਦਿਸ਼<std::pair<uint64_t, uint64_t>> ivs;
        ਜੇ (cbinp->isWildcard() || !ਕੱਢੋ_Range_Intervals(cbinp, maxVal, ivs)) ਵਾਪਸ ਝੂਠ;
        bins.ਧੱਕੋ_ਪਿੱਠ(std::move(ivs));
        ਵਾਪਸ ਸੱਚ;
    }

    // Append the cross slots of an array Normal bin: each element is a slot covering its
    // values, as at most one of its single-value bins holds a value.  An element holds the
    // values arrayBinRuns() gives it: those of the coverpoint type (IEEE 1800-2023 19.5.7).
    // Elements of a signed coverpoint, non-constant elements, and elements with values beyond
    // 64 bits can't be enumerated, so they count one slot but lose exactness.  Returns false if
    // any element wasn't enumerable to exact values.
    ਤਾਰਕਿਕ appendArrayBinCrossSlots(AstCoverBin* cbinp, Ast_ਗੰਢ_ਵਿਅੰਜਕ* exprp,
                                  std::ਸਦਿਸ਼<std::ਸਦਿਸ਼<std::pair<uint64_t, uint64_t>>>& bins,
                                  ਪੂਰਨ& ਥਾਂ_ਗਿਣਤੀ) {
        // Signed values resolve sign-extended, not as unsigned intervals, and wildcard patterns
        // are not intervals
        ਤਾਰਕਿਕ exact = !exprp->isSigned() && !cbinp->isWildcard();
        ਲਈ (Ast_ਗੰਢ* rp = cbinp->rangesp(); rp; rp = rp->nextp()) {
            ++ਥਾਂ_ਗਿਣਤੀ;
            RangeBounds rb;
            Cross_ਮੁੱਲ_Range range{rp, resolve_ਚੌੜਾਈ(rp, exprp)};
            ਜੇ (!exact || !constRangeBounds(rp, rb)
                || !resolve_ਮੁੱਲ(rp, exprp, ਸੱਚ, ਝੂਠ, range)) {
                exact = ਝੂਠ;
                ਜਾਰੀ;
            }
            ਜੇ (cross_Range_ਖਾਲੀ_ਥਾਵਾਂ(range)) ਜਾਰੀ;  // Its bin, if any, holds no value
            ਜੇ (range.hi.most_ਸੈੱਟ_ਦੋਆਧਾਰੀ_ਅੰਕ_P1() > 64) {
                exact = ਝੂਠ;
                ਜਾਰੀ;
            }
            bins.ਧੱਕੋ_ਪਿੱਠ({{range.lo.ਮੰਜ਼ਿਲ_UQuad(), range.hi.ਮੰਜ਼ਿਲ_UQuad()}});
        }
        ਵਾਪਸ exact;
    }

    // Compute the hit-list bound for a coverpoint: the maximum number of Normal
    // bins one sample value can match.  Non-cross-fed coverpoints don't feed a cross, so
    // their hit list is unused -> 1.  Otherwise compute the exact max bin overlap; fall back
    // to the (always-safe) Normal-slot count when any bin isn't statically analyzable.
    ਪੂਰਨ compute_ਪ੍ਰਾਪਤੀ_List_Bound(AstCoverpoint* coverpointp, Ast_ਗੰਢ_ਵਿਅੰਜਕ* exprp, ਤਾਰਕਿਕ crossFed) {
        ਜੇ (!crossFed) ਵਾਪਸ 1;
        ਅਚਲ ਪੂਰਨ ਚੌੜਾਈ = exprp->ਚੌੜਾਈ();
        ਅਚਲ uint64_t maxVal = (ਚੌੜਾਈ >= 64) ? UINT64_MAX : ((1ULL << ਚੌੜਾਈ) - 1);
        // One entry per Normal bin (cross slot): its covered intervals.
        std::ਸਦਿਸ਼<std::ਸਦਿਸ਼<std::pair<uint64_t, uint64_t>>> bins;
        ਪੂਰਨ ਥਾਂ_ਗਿਣਤੀ = 0;  // == runtime m_normal; the safe fallback bound
        // Unsigned intervals cannot establish overlap between differently sized signed values.
        ਤਾਰਕਿਕ exact = !exprp->isSigned();
        ਲਈ (Ast_ਗੰਢ* binp = coverpointp->binsp(); binp; binp = binp->nextp()) {
            AstCoverBin* ਅਚਲ cbinp = VN_AS(binp, CoverBin);
            ਜੇ (!cbinp->binsType().bin_Is_ਸਾਧਾਰਨ())
                ਜਾਰੀ;  // ignore/illegal/default: not hit-listed
            ਜੇ (!appendBinCrossSlots(cbinp, maxVal, exprp, bins, ਥਾਂ_ਗਿਣਤੀ)) exact = ਝੂਠ;
        }
        ਜੇ (!exact) ਵਾਪਸ std::max(1, ਥਾਂ_ਗਿਣਤੀ);
        ਜੇ (bins.ਖਾਲੀ_ਥਾਵਾਂ()) ਵਾਪਸ 1;
        // Max overlap occurs at some interval start; count covering bins at each lo.
        std::ਸਦਿਸ਼<uint64_t> pts;
        ਲਈ (ਅਚਲ ਸਵੈਚਾਲਿਤ& b : bins)
            ਲਈ (ਅਚਲ ਸਵੈਚਾਲਿਤ& iv : b) pts.ਧੱਕੋ_ਪਿੱਠ(iv.ਪਹਿਲਾ);
        std::ਛਾਂਟੋ(pts.begin(), pts.ਸਿਰਾ());
        pts.erase(std::unique(pts.begin(), pts.ਸਿਰਾ()), pts.ਸਿਰਾ());
        ਪੂਰਨ maxOverlap = 1;
        ਲਈ (ਅਚਲ uint64_t p : pts) {
            ਪੂਰਨ cnt = 0;
            ਲਈ (ਅਚਲ ਸਵੈਚਾਲਿਤ& b : bins) {
                ਲਈ (ਅਚਲ ਸਵੈਚਾਲਿਤ& iv : b) {
                    ਜੇ (iv.ਪਹਿਲਾ <= p && p <= iv.ਦੂਜਾ) {
                        ++cnt;
                        ਰੋਕੋ;  // count each bin at most once
                    }
                }
            }
            ਜੇ (cnt > maxOverlap) maxOverlap = cnt;
        }
        ਵਾਪਸ maxOverlap;
    }

    // A 'this->m_member' reference for embedding in an AstCStmt
    AstVarRef* memberRef(File_ਸਤਰ* fl, AstVar* varp, VAccess access = VAccess::READ) {
        AstVarRef* ਅਚਲ refp = ਨਵਾਂ AstVarRef{fl, varp, access};
        refp->self_ਸੂਚਕ(VSelf_ਸੂਚਕ_ਪਾਠ{VSelf_ਸੂਚਕ_ਪਾਠ::This{}});
        ਵਾਪਸ refp;
    }

    // A 'this->m_member-><method>(args...)' call on one member.  usePtr is false only for
    // __Vcg_inst, which is a value handle; the item members are borrowed pointers into the
    // instance node.  Numeric arguments are AstConst; the rest are C++ text that has no AST
    // form (see ctext).
    AstCMethodHard* ਵਸਤੂ_Call(File_ਸਤਰ* fl, AstVar* varp, VCMethod ਵਿਧੀ,
                             ਅਚਲ std::ਸਦਿਸ਼<Ast_ਗੰਢ_ਵਿਅੰਜਕ*>& ਦਲੀਲਾਂ = {}, ਤਾਰਕਿਕ usePtr = ਸੱਚ) {
        AstCMethodHard* ਅਚਲ callp = ਨਵਾਂ AstCMethodHard{fl, memberRef(fl, varp), ਵਿਧੀ};
        ਲਈ (Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ argp : ਦਲੀਲਾਂ) callp->ਜੋੜੋ_Pinsp(argp);
        callp->usePtr(usePtr);
        callp->dtype_ਸੈੱਟ_Void();
        ਵਾਪਸ callp;
    }

    // An unsigned integer argument.
    ਸਥਿਰ AstConst* cnum(File_ਸਤਰ* fl, uint32_t ਮੁੱਲ) { ਵਾਪਸ ਨਵਾਂ AstConst{fl, ਮੁੱਲ}; }

    // A literal C++ argument with no AST equivalent: a 'const char*' string literal (an SV
    // string AstConst emits '"..."s', a std::string temporary the runtime cannot borrow), a
    // VlCovBinKind enum token, a constant selection-word initializer list, a VlFileLineDebug, or
    // a '__V' temporary declared by the enclosing AstCStmt.
    ਸਥਿਰ AstCExpr* ctext(File_ਸਤਰ* fl, ਅਚਲ std::string& ਪਾਠ) {
        ਵਾਪਸ ਨਵਾਂ AstCExpr{fl, ਪਾਠ};
    }

    // A C++ string literal.  Escapes control characters as the emitter does elsewhere -- bin
    // names and filenames reach the generated code verbatim when --protect-ids is off, and an
    // SV escaped identifier may hold a quote or backslash.
    ਸਥਿਰ std::string quoted(ਅਚਲ std::string& ਪਾਠ) {
        ਵਾਪਸ "\"" + V3_ਬਾਹਰਲਾ_Formatter::quoteNameControls(ਪਾਠ) + "\"";
    }

    // A 'VlFileLineDebug' argument: where the runtime reports an error about fl's construct
    ਸਥਿਰ AstCExpr* file_ਸਤਰ_Debug(File_ਸਤਰ* fl) {
        ਅਚਲ std::string filename
            = VIdProtect::protectIf(fl->filename(), v3Global.opt.protectIds());
        ਵਾਪਸ ctext(fl, "VlFileLineDebug{" + quoted(filename) + ", "
                             + std::ਮੰਜ਼ਿਲ_string(fl->lineno()) + "}");
    }

    // Check that an element of an array bin (bins b[] = {values/ranges}) is a two-state
    // constant value or range; false if not, after reporting it if 'report'.
    ਸਥਿਰ ਤਾਰਕਿਕ checkArrayBinElement(AstCoverBin* arrayBinp, Ast_ਗੰਢ* rangep, ਤਾਰਕਿਕ report = ਸੱਚ) {
        ਜੇ (ਅਚਲ AstInsideRange* ਅਚਲ irp = VN_CAST(rangep, InsideRange)) {
            ਅਚਲ AstConst* ਅਚਲ minp = VN_CAST(irp->lhsp(), Const);
            ਅਚਲ AstConst* ਅਚਲ maxp = VN_CAST(irp->rhsp(), Const);
            ਜੇ ((!minp && !VN_IS(irp->lhsp(), Unbounded))
                || (!maxp && !VN_IS(irp->rhsp(), Unbounded))) {
                ਜੇ (report) {
                    arrayBinp->v3error("Non-constant expression in array bins range; "
                                       "range bounds must be constants (IEEE 1800-2023 19.5)");
                }
                ਵਾਪਸ ਝੂਠ;
            }
            ਜੇ ((minp && minp->ਅੰਕ().isFourState()) || (maxp && maxp->ਅੰਕ().isFourState())) {
                ਜੇ (report) {
                    arrayBinp->v3error("Four-state (x/z) value in array bins range bound; "
                                       "range bounds must be two-state constants");
                }
                ਵਾਪਸ ਝੂਠ;
            }
        } ਨਹੀਂਤਾਂ ਜੇ (!VN_IS(rangep, Const)) {
            ਜੇ (report) {
                arrayBinp->v3error("Non-constant expression in array bins value list; "
                                   "values must be constants (IEEE 1800-2023 19.5)");
            }
            ਵਾਪਸ ਝੂਠ;
        }
        ਵਾਪਸ ਸੱਚ;
    }

    // Individual equality targets of an array bin (bins b[] = {values/ranges}) of a real
    // coverpoint, in order; integral coverpoints generate array bins as runs (see arrayBinRuns).
    // An open-ended bound ('$', AstUnbounded) resolves to the coverpoint domain: '[lo:$]'
    // covers [lo:maxVal] and '[$:hi]' covers [0:hi].  One target is produced per value; ranges
    // whose resolved size would exceed --coverage-max-real-bins (e.g. an open '[lo:$]') are
    // unsupported -- emits COVERIGN, sets unsupportedOut, yields nothing.
    std::ਸਦਿਸ਼<Ast_ਗੰਢ_ਵਿਅੰਜਕ*> ਕੱਢੋ_Array_ਮੁੱਲਾਂ(AstCoverBin* arrayBinp, Ast_ਗੰਢ_ਵਿਅੰਜਕ* exprp,
                                                 ਤਾਰਕਿਕ& unsupported_ਬਾਹਰਲਾ) {
        unsupported_ਬਾਹਰਲਾ = ਝੂਠ;
        ਅਚਲ ਪੂਰਨ ਚੌੜਾਈ = exprp->ਚੌੜਾਈ();
        ਅਚਲ uint64_t maxVal = (ਚੌੜਾਈ >= 64) ? UINT64_MAX : ((1ULL << ਚੌੜਾਈ) - 1);
        std::ਸਦਿਸ਼<Ast_ਗੰਢ_ਵਿਅੰਜਕ*> ਮੁੱਲਾਂ;
        ਲਈ (Ast_ਗੰਢ* rangep = arrayBinp->rangesp(); rangep; rangep = rangep->nextp()) {
            rangep = V3Const::constifyEdit(rangep);
            ਜੇ (!checkArrayBinElement(arrayBinp, rangep)) ਵਾਪਸ ਮੁੱਲਾਂ;
            ਜੇ (AstInsideRange* ਅਚਲ irp = VN_CAST(rangep, InsideRange)) {
                ਅਚਲ ਤਾਰਕਿਕ loUnb = VN_IS(irp->lhsp(), Unbounded);
                ਅਚਲ ਤਾਰਕਿਕ hiUnb = VN_IS(irp->rhsp(), Unbounded);
                ਅਚਲ uint64_t lo = loUnb ? 0 : VN_AS(irp->lhsp(), Const)->ਮੰਜ਼ਿਲ_UQuad();
                ਅਚਲ uint64_t hi = hiUnb ? maxVal : VN_AS(irp->rhsp(), Const)->ਮੰਜ਼ਿਲ_UQuad();
                ਜੇ (hi < lo) ਜਾਰੀ;  // empty range contributes no bins
                // Guard against a '$'-bounded or otherwise huge range exploding the bin count.
                ਅਚਲ uint64_t ਵਿਸਤਾਰ = hi - lo;  // == valueCount - 1 (no overflow: hi >= lo)
                ਜੇ (ਵਿਸਤਾਰ >= v3Global.opt.coverageMaxRealBins()
                    || ਮੁੱਲਾਂ.ਮਾਪ() + ਵਿਸਤਾਰ + 1 > v3Global.opt.coverageMaxRealBins()) {
                    arrayBinp->v3warn(COVERIGN,
                                      "Unsupported: array 'bins' of a real coverpoint "
                                      "covering more than "
                                          << v3Global.opt.coverageMaxRealBins() << " values; bin "
                                          << arrayBinp->prettyNameQ() << " ignored.\n"
                                          << arrayBinp->warn_ਹੋਰ()
                                          << "... Suggest a larger --coverage-max-real-bins");
                    unsupported_ਬਾਹਰਲਾ = ਸੱਚ;
                    ਲਈ (Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ vp : ਮੁੱਲਾਂ) VL_DO_DANGLING(ਧੱਕੋ_Deletep(vp), vp);
                    ਮੁੱਲਾਂ.clear();
                    ਵਾਪਸ ਮੁੱਲਾਂ;
                }
                ਲਈ (uint64_t v = lo; v <= hi; ++v)
                    ਮੁੱਲਾਂ.ਧੱਕੋ_ਪਿੱਠ(ਨਵਾਂ AstConst{irp->fileline(), AstConst::Widthed_ਮੁੱਲ{}, ਚੌੜਾਈ,
                                                  static_cast<uint32_t>(v)});
            } ਨਹੀਂਤਾਂ {
                ਮੁੱਲਾਂ.ਧੱਕੋ_ਪਿੱਠ(VN_AS(rangep->clone_ਰੁੱਖ(ਝੂਠ), ਗੰਢ_ਵਿਅੰਜਕ));
            }
        }
        ਵਾਪਸ ਮੁੱਲਾਂ;
    }

    ਸਥਿਰ ਪੂਰਨ ਲੜੀਵਾਰ_ਚੌੜਾਈ(ਅਚਲ Ast_ਗੰਢ_ਵਿਅੰਜਕ* exprp) { ਵਾਪਸ exprp->ਚੌੜਾਈ() + 1; }

    // Automatic bins partition the coverpoint domain in value order (IEEE 1800-2023 19.5.3): N
    // bins, capped at the number of values, each hold 2^width / N values, and the last bin also
    // holds the remainder.  False for an invalid declaration, already reported.
    ਤਾਰਕਿਕ autoBinRuns(AstCoverBin* binp, Ast_ਗੰਢ_ਵਿਅੰਜਕ* exprp, BinRuns& ਬਾਹਰਲਾ) {
        ਅਚਲ uint32_t requested = autoBinsRequested(binp);
        ਜੇ (!requested || !exprp->dtypep()->skipRefp()->isIntegralOrPacked()) ਵਾਪਸ ਝੂਠ;
        ਅਚਲ ਪੂਰਨ ਚੌੜਾਈ = exprp->ਚੌੜਾਈ();
        ਅਚਲ ਪੂਰਨ arithmetic_ਚੌੜਾਈ = ਲੜੀਵਾਰ_ਚੌੜਾਈ(exprp);
        ਅਚਲ uint32_t ਗਿਣਤੀ_2
            = ਚੌੜਾਈ < 32
                  ? static_cast<uint32_t>(std::min<uint64_t>(uint64_t{1} << ਚੌੜਾਈ, requested))
                  : requested;
        Bin_ਲੜੀਵਾਰ ਲੜੀਵਾਰ{binp, arithmetic_ਚੌੜਾਈ, ਗਿਣਤੀ_2};
        V3_ਸੰਖਿਆ ਕੁੱਲ{binp, arithmetic_ਚੌੜਾਈ};
        ਕੁੱਲ.ਸੈੱਟ_ਦੋਆਧਾਰੀ_ਅੰਕ(ਚੌੜਾਈ, 1);
        ਲੜੀਵਾਰ.m_stride.opDiv(ਕੁੱਲ, V3_ਸੰਖਿਆ{binp, arithmetic_ਚੌੜਾਈ, ਗਿਣਤੀ_2});
        ਅਚਲ Cross_ਮੁੱਲ_Range domain
            = cross_ਮੁੱਲ_Domain(binp, ਚੌੜਾਈ, exprp->isSigned(), arithmetic_ਚੌੜਾਈ);
        ਲੜੀਵਾਰ.m_lo = domain.lo;
        ਲੜੀਵਾਰ.m_hi = domain.hi;
        ਬਾਹਰਲਾ.runs.ਧੱਕੋ_ਪਿੱਠ(std::move(ਲੜੀਵਾਰ));
        ਬਾਹਰਲਾ.ਗਿਣਤੀ_2 = ਗਿਣਤੀ_2;
        ਵਾਪਸ ਸੱਚ;
    }

    // The elements of an array bin (bins b[] = {values/ranges}), in order, as runs of
    // single-value bins.  A range holds the values of the coverpoint type it contains (IEEE
    // 1800-2023 19.5.7), while a singleton names a bin even without such a value.  Errors on a
    // non-constant element.  More than --coverage-max-bins bins (e.g. an open '[lo:$]' range over
    // a wide coverpoint) are unsupported -- emits COVERIGN, and sets unsupported.
    BinRuns arrayBinRuns(AstCoverBin* arrayBinp, Ast_ਗੰਢ_ਵਿਅੰਜਕ* exprp) {
        BinRuns ਬਾਹਰਲਾ;
        ਅਚਲ ਪੂਰਨ ਚੌੜਾਈ = ਲੜੀਵਾਰ_ਚੌੜਾਈ(exprp);
        ਲਈ (Ast_ਗੰਢ* rangep = arrayBinp->rangesp(); rangep; rangep = rangep->nextp()) {
            rangep = V3Const::constifyEdit(rangep);
            ਜੇ (!checkArrayBinElement(arrayBinp, rangep)) ਵਾਪਸ ਬਾਹਰਲਾ;
            ਅਚਲ AstInsideRange* ਅਚਲ irp = VN_CAST(rangep, InsideRange);
            Cross_ਮੁੱਲ_Range range{rangep, resolve_ਚੌੜਾਈ(rangep, exprp)};
            ਤਾਰਕਿਕ ਖਾਲੀ_ਥਾਵਾਂ = ਸੱਚ;
            ਜੇ (!resolve_ਮੁੱਲ(rangep, exprp, ਸੱਚ, ਝੂਠ, range)) {
                rangep->v3warn(E_UNSUPPORTED, "Unsupported: non-integral value in a coverage bin "
                                              "of an integral coverpoint.");
            } ਨਹੀਂਤਾਂ {
                ਖਾਲੀ_ਥਾਵਾਂ = cross_Range_ਖਾਲੀ_ਥਾਵਾਂ(range);
            }
            ਜੇ (ਖਾਲੀ_ਥਾਵਾਂ && irp) ਜਾਰੀ;  // A range without values contributes no bins
            Bin_ਲੜੀਵਾਰ ਲੜੀਵਾਰ{rangep, ਚੌੜਾਈ, 1};
            ਲੜੀਵਾਰ.m_ਖਾਲੀ_ਥਾਵਾਂ = ਖਾਲੀ_ਥਾਵਾਂ;
            uint64_t ਗਿਣਤੀ_2 = 1;
            ਜੇ (!ਖਾਲੀ_ਥਾਵਾਂ) {
                ਲੜੀਵਾਰ.m_lo.op_ਸੌਂਪੋ(range.lo);
                ਲੜੀਵਾਰ.m_hi.op_ਸੌਂਪੋ(range.hi);
                V3_ਸੰਖਿਆ ਵਿਸਤਾਰ{rangep, ਚੌੜਾਈ};
                ਵਿਸਤਾਰ.opSub(ਲੜੀਵਾਰ.m_hi, ਲੜੀਵਾਰ.m_lo);
                // Wider spans exceed any limit
                ਗਿਣਤੀ_2 = ਵਿਸਤਾਰ.most_ਸੈੱਟ_ਦੋਆਧਾਰੀ_ਅੰਕ_P1() > 32 ? UINT64_MAX : ਵਿਸਤਾਰ.ਮੰਜ਼ਿਲ_UQuad() + 1;
            }
            ਜੇ (ਗਿਣਤੀ_2 > v3Global.opt.coverageMaxBins() - ਬਾਹਰਲਾ.ਗਿਣਤੀ_2) {
                arrayBinp->v3warn(COVERIGN, "Unsupported: array 'bins' covering more than "
                                                << v3Global.opt.coverageMaxBins()
                                                << " values (e.g. an open '[lo:$]' range over "
                                                   "a wide coverpoint); bin "
                                                << arrayBinp->prettyNameQ() << " ignored\n"
                                                << arrayBinp->warn_ਹੋਰ()
                                                << "... Suggest a larger --coverage-max-bins");
                ਬਾਹਰਲਾ.runs.clear();
                ਬਾਹਰਲਾ.ਗਿਣਤੀ_2 = 0;
                ਬਾਹਰਲਾ.unsupported = ਸੱਚ;
                ਵਾਪਸ ਬਾਹਰਲਾ;
            }
            ਲੜੀਵਾਰ.m_ਗਿਣਤੀ = static_cast<uint32_t>(ਗਿਣਤੀ_2);
            ਬਾਹਰਲਾ.ਗਿਣਤੀ_2 += ਲੜੀਵਾਰ.m_ਗਿਣਤੀ;
            ਬਾਹਰਲਾ.runs.ਧੱਕੋ_ਪਿੱਠ(std::move(ਲੜੀਵਾਰ));
        }
        ਵਾਪਸ ਬਾਹਰਲਾ;
    }

    // The bins of a wildcard array (wildcard bins b[] = {...}): one for each coverpoint value
    // an element matches (IEEE 1800-2023 19.5.4, 19.5.7), in value order, and named by the value
    // (19.5.1), as runs of single-value bins.  Errors on a non-constant element.  More than
    // --coverage-max-bins values are unsupported -- emits COVERIGN, and sets unsupported, and for
    // an ignore or illegal array, single.  'report' false omits these diagnostics.
    ਸਥਿਰ BinRuns wildcardBinRuns(AstCoverBin* arrayBinp, Ast_ਗੰਢ_ਵਿਅੰਜਕ* exprp, ਤਾਰਕਿਕ report) {
        BinRuns ਬਾਹਰਲਾ;
        ਅਚਲ ਪੂਰਨ ਚੌੜਾਈ = ਲੜੀਵਾਰ_ਚੌੜਾਈ(exprp);
        ਅਚਲ V3_ਸੰਖਿਆ one{arrayBinp, ਚੌੜਾਈ, 1};
        // Disjoint runs of the values, in value order, each value once
        std::ਸਦਿਸ਼<std::pair<V3_ਸੰਖਿਆ, V3_ਸੰਖਿਆ>> spans;
        uint64_t ਗਿਣਤੀ_2 = 0;  // Values of 'spans'
        ਲਈ (Ast_ਗੰਢ* rangep = arrayBinp->rangesp(); rangep; rangep = rangep->nextp()) {
            rangep = V3Const::constifyEdit(rangep);
            ਜੇ (!checkArrayBinElement(arrayBinp, rangep, report)) {
                ਬਾਹਰਲਾ.unsupported = ਸੱਚ;
                ਵਾਪਸ ਬਾਹਰਲਾ;
            }
            Cross_ਮੁੱਲ_Range range{rangep, resolve_ਚੌੜਾਈ(rangep, exprp)};
            ਜੇ (!resolve_ਮੁੱਲ(rangep, exprp, ਸੱਚ, ਸੱਚ, range)) {
                ਜੇ (report) {
                    rangep->v3warn(E_UNSUPPORTED, "Unsupported: non-integral value in a "
                                                  "coverage bin of an integral coverpoint.");
                }
                ਜਾਰੀ;
            }
            ਜੇ (cross_Range_ਖਾਲੀ_ਥਾਵਾਂ(range)) ਜਾਰੀ;
            std::ਸਦਿਸ਼<std::pair<V3_ਸੰਖਿਆ, V3_ਸੰਖਿਆ>> ਮਿਲਿਆ;
            crossRangeRuns(range, v3Global.opt.coverageMaxBins(), ਮਿਲਿਆ);
            ਲਈ (ਅਚਲ std::pair<V3_ਸੰਖਿਆ, V3_ਸੰਖਿਆ>& ਲੜੀਵਾਰ : ਮਿਲਿਆ) {
                // Coverpoint values, sign-extended in both widths
                spans.emplace_ਪਿੱਠ(V3_ਸੰਖਿਆ{rangep, ਚੌੜਾਈ, ਲੜੀਵਾਰ.ਪਹਿਲਾ},
                                   V3_ਸੰਖਿਆ{rangep, ਚੌੜਾਈ, ਲੜੀਵਾਰ.ਦੂਜਾ});
            }
            std::ਛਾਂਟੋ(spans.begin(), spans.ਸਿਰਾ(), [](ਅਚਲ ਸਵੈਚਾਲਿਤ& lhs, ਅਚਲ ਸਵੈਚਾਲਿਤ& rhs) {
                ਵਾਪਸ cross_ਮੁੱਲ_Less(lhs.ਪਹਿਲਾ, rhs.ਪਹਿਲਾ);
            });
            std::ਸਦਿਸ਼<std::pair<V3_ਸੰਖਿਆ, V3_ਸੰਖਿਆ>> merged;
            ਲਈ (ਅਚਲ std::pair<V3_ਸੰਖਿਆ, V3_ਸੰਖਿਆ>& ਵਿਸਤਾਰ : spans) {
                // Adjacent or overlapping values join a run; 'first - 1' cannot overflow
                V3_ਸੰਖਿਆ before{rangep, ਚੌੜਾਈ};
                before.opSub(ਵਿਸਤਾਰ.ਪਹਿਲਾ, one);
                ਜੇ (merged.ਖਾਲੀ_ਥਾਵਾਂ() || cross_ਮੁੱਲ_Less(merged.ਪਿੱਠ().ਦੂਜਾ, before)) {
                    merged.ਧੱਕੋ_ਪਿੱਠ(ਵਿਸਤਾਰ);
                } ਨਹੀਂਤਾਂ ਜੇ (cross_ਮੁੱਲ_Less(merged.ਪਿੱਠ().ਦੂਜਾ, ਵਿਸਤਾਰ.ਦੂਜਾ)) {
                    merged.ਪਿੱਠ().ਦੂਜਾ = ਵਿਸਤਾਰ.ਦੂਜਾ;
                }
            }
            ਗਿਣਤੀ_2 = 0;
            ਲਈ (ਅਚਲ std::pair<V3_ਸੰਖਿਆ, V3_ਸੰਖਿਆ>& ਵਿਸਤਾਰ : merged) {
                V3_ਸੰਖਿਆ ਮਾਪ{rangep, ਚੌੜਾਈ};
                ਮਾਪ.opSub(ਵਿਸਤਾਰ.ਦੂਜਾ, ਵਿਸਤਾਰ.ਪਹਿਲਾ);
                // Beyond 2^32 values exceed any limit
                ਗਿਣਤੀ_2 += ਮਾਪ.most_ਸੈੱਟ_ਦੋਆਧਾਰੀ_ਅੰਕ_P1() > 32 ? uint64_t{1} << 33 : ਮਾਪ.ਮੰਜ਼ਿਲ_UQuad() + 1;
                ਜੇ (ਗਿਣਤੀ_2 > v3Global.opt.coverageMaxBins()) ਰੋਕੋ;
            }
            spans = std::move(merged);
            ਜੇ (ਗਿਣਤੀ_2 > v3Global.opt.coverageMaxBins()) {
                // An ignore or illegal array still excludes or checks its values, as one bin
                ਬਾਹਰਲਾ.single = !arrayBinp->binsType().bin_Is_ਸਾਧਾਰਨ();
                ਜੇ (report) {
                    arrayBinp->v3warn(
                        COVERIGN, "Unsupported: wildcard array '"
                                      << arrayBinp->binsType().verilogKwd()
                                      << "' of more than --coverage-max-bins of "
                                      << v3Global.opt.coverageMaxBins() << " values; bin "
                                      << arrayBinp->prettyNameQ()
                                      << (ਬਾਹਰਲਾ.single ? " treated as one bin" : " ignored") << "\n"
                                      << arrayBinp->warn_ਹੋਰ()
                                      << "... Suggest a larger --coverage-max-bins");
                }
                ਬਾਹਰਲਾ.unsupported = ਸੱਚ;
                ਵਾਪਸ ਬਾਹਰਲਾ;
            }
        }
        ਲਈ (ਅਚਲ std::pair<V3_ਸੰਖਿਆ, V3_ਸੰਖਿਆ>& ਵਿਸਤਾਰ : spans) {
            ਬਾਹਰਲਾ.runs.emplace_ਪਿੱਠ(arrayBinp, ਚੌੜਾਈ, 0);
            Bin_ਲੜੀਵਾਰ& ਲੜੀਵਾਰ = ਬਾਹਰਲਾ.runs.ਪਿੱਠ();
            ਲੜੀਵਾਰ.m_lo = ਵਿਸਤਾਰ.ਪਹਿਲਾ;
            ਲੜੀਵਾਰ.m_hi = ਵਿਸਤਾਰ.ਦੂਜਾ;
            // A bin for each value, named by it in the coverpoint's type
            V3_ਸੰਖਿਆ ਮੁੱਲ = ਵਿਸਤਾਰ.ਪਹਿਲਾ;
            ਜਦੋਂ (ਸੱਚ) {
                ਅਚਲ V3_ਸੰਖਿਆ typed{arrayBinp, exprp->ਚੌੜਾਈ(), ਮੁੱਲ};
                ਬਾਹਰਲਾ.ਮੁੱਲਾਂ.ਧੱਕੋ_ਪਿੱਠ(exprp->isSigned() ? typed.ਮੰਜ਼ਿਲ_Decimal_S() : typed.ਮੰਜ਼ਿਲ_Decimal_U());
                ++ਲੜੀਵਾਰ.m_ਗਿਣਤੀ;
                ਜੇ (ਮੁੱਲ.isCaseEq(ਵਿਸਤਾਰ.ਦੂਜਾ)) ਰੋਕੋ;
                V3_ਸੰਖਿਆ ਅਗਲਾ{arrayBinp, ਚੌੜਾਈ};
                ਮੁੱਲ = ਅਗਲਾ.op_ਜੋੜੋ(ਮੁੱਲ, one);
            }
        }
        ਬਾਹਰਲਾ.ਗਿਣਤੀ_2 = static_cast<uint32_t>(ਗਿਣਤੀ_2);
        ਵਾਪਸ ਬਾਹਰਲਾ;
    }

    // The runs of an automatic bins declaration, or of an array bin of an integral coverpoint.
    // False for other bins, which do not generate as runs, including a wildcard array then one
    // bin (see BinRuns::single).
    ਤਾਰਕਿਕ binRunsFor(AstCoverBin* binp, Ast_ਗੰਢ_ਵਿਅੰਜਕ* exprp, BinRuns& ਬਾਹਰਲਾ) {
        ਜੇ (isAutoBins(binp)) {
            ਜੇ (!autoBinRuns(binp, exprp, ਬਾਹਰਲਾ)) ਬਾਹਰਲਾ.unsupported = ਸੱਚ;
            ਵਾਪਸ ਸੱਚ;
        }
        ਜੇ (binp->isArray() && binp->isWildcard()) {
            ਜੇ (!exprp->dtypep()->skipRefp()->isIntegralOrPacked()) {
                Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ falsep = wildcardTypeError(binp, exprp);
                VL_DO_DANGLING(ਧੱਕੋ_Deletep(falsep), falsep);
                ਬਾਹਰਲਾ.unsupported = ਸੱਚ;
            } ਨਹੀਂਤਾਂ {
                ਬਾਹਰਲਾ = wildcardBinRuns(binp, exprp, ਸੱਚ);
                ਜੇ (ਬਾਹਰਲਾ.single) {
                    binp->isArray(ਝੂਠ);  // Generates as one bin
                    ਵਾਪਸ ਝੂਠ;
                }
            }
            ਵਾਪਸ ਸੱਚ;
        }
        ਜੇ (!binp->isArray() || binp->transp() || binp->isWildcard()
            || !exprp->dtypep()->skipRefp()->isIntegralOrPacked()) {
            ਵਾਪਸ ਝੂਠ;
        }
        ਬਾਹਰਲਾ = arrayBinRuns(binp, exprp);
        ਵਾਪਸ ਸੱਚ;
    }

    // Emit a 'this->m_cp->addSingleNamer/addArrayNamer(...)' statement for one bin whose first
    // runtime bin index is 'declared'; or with 'valueNames', those naming each bin of an array
    Ast_ਗੰਢ_Stmt* makeNamer(AstVar* cpVarp, AstCoverBin* binp, int64_t ਗਿਣਤੀ_2, uint32_t declared,
                           ਅਚਲ std::ਸਦਿਸ਼<Ast_ਗੰਢ_ਵਿਅੰਜਕ*>& ਮੁੱਲਾਂ = {},
                           ਅਚਲ std::ਸਦਿਸ਼<std::string>& ਮੁੱਲ_ਨਾਂ = {}) {
        File_ਸਤਰ* ਅਚਲ fl = binp->fileline();
        CoverpointBins& bins = m_cpBins.at(cpVarp);
        ਅਚਲ uint32_t ਸਾਧਾਰਨ_ਗਿਣਤੀ
            = binp->binsType().bin_Is_ਸਾਧਾਰਨ() ? static_cast<uint32_t>(ਗਿਣਤੀ_2 < 0 ? 1 : ਗਿਣਤੀ_2) : 0;
        ਅਚਲ Bin_ਵਿਸਤਾਰ ਵਿਸਤਾਰ{bins.ਕੁੱਲ, ਸਾਧਾਰਨ_ਗਿਣਤੀ, declared};
        ਜੇ (binp->binsType() == VCoverBinsType::BINS_AUTO_IMPLICIT) {
            bins.implicitAuto = ਵਿਸਤਾਰ;  // Selected by bin, see implicitAutoBinSpan
        } ਨਹੀਂਤਾਂ {
            bins.spans.emplace(binp->name(), ਵਿਸਤਾਰ);
        }
        bins.ਕੁੱਲ += ਸਾਧਾਰਨ_ਗਿਣਤੀ;
        ਲਈ (uint32_t i = 0; bins.crossed && i < ਸਾਧਾਰਨ_ਗਿਣਤੀ; ++i) {
            bins.ਮੁੱਲਾਂ.ਧੱਕੋ_ਪਿੱਠ({binp, ਮੁੱਲਾਂ.ਖਾਲੀ_ਥਾਵਾਂ() ? ਖਾਲੀ_ਸੂਚਕ : ਮੁੱਲਾਂ[i]});
        }
        // Under --protect-ids the filename and bin name flow into the coverage database
        // verbatim, so obfuscate them exactly as line/toggle coverage points are (whole-
        // unit filename, per-word bin name).  A no-op when --protect-ids is off.
        ਅਚਲ ਤਾਰਕਿਕ prot = v3Global.opt.protectIds();
        ਅਚਲ ਤਾਰਕਿਕ single = ਗਿਣਤੀ_2 < 0;
        ਜੇ (!ਮੁੱਲ_ਨਾਂ.ਖਾਲੀ_ਥਾਵਾਂ()) {
            // A bin of a wildcard array is named by its value (IEEE 1800-2023 19.5.1)
            Ast_ਗੰਢ_Stmt* stmtsp = ਖਾਲੀ_ਸੂਚਕ;
            ਲਈ (ਅਚਲ std::string& ਮੁੱਲ : ਮੁੱਲ_ਨਾਂ) {
                ਅਚਲ std::string name
                    = VIdProtect::protect_ਸ਼ਬਦ_If(binp->name(), prot) + "[" + ਮੁੱਲ + "]";
                stmtsp = Ast_ਗੰਢ::ਜੋੜੋ_ਅਗਲਾ(
                    stmtsp,
                    ਵਸਤੂ_Call(fl, cpVarp, VCMethod::COVERGROUP_ਜੋੜੋ_SINGLE_NAMER,
                             {ctext(fl, binp->binsType().bin_ਸੈੱਟ_Enum()), ctext(fl, quoted(name)),
                              ctext(fl, quoted(VIdProtect::protectIf(fl->filename(), prot))),
                              cnum(fl, static_cast<uint32_t>(fl->lineno())),
                              cnum(fl, static_cast<uint32_t>(fl->ਪਹਿਲਾ_ਕਾਲਮ()))})
                        ->makeStmt());
            }
            ਵਾਪਸ stmtsp;
        }
        std::ਸਦਿਸ਼<Ast_ਗੰਢ_ਵਿਅੰਜਕ*> ਦਲੀਲਾਂ{ctext(fl, binp->binsType().bin_ਸੈੱਟ_Enum())};
        ਜੇ (!single) ਦਲੀਲਾਂ.ਧੱਕੋ_ਪਿੱਠ(cnum(fl, static_cast<uint32_t>(ਗਿਣਤੀ_2)));  // value array bin
        ਦਲੀਲਾਂ.ਧੱਕੋ_ਪਿੱਠ(ctext(fl, quoted(VIdProtect::protect_ਸ਼ਬਦ_If(binp->name(), prot))));
        ਦਲੀਲਾਂ.ਧੱਕੋ_ਪਿੱਠ(ctext(fl, quoted(VIdProtect::protectIf(fl->filename(), prot))));
        ਦਲੀਲਾਂ.ਧੱਕੋ_ਪਿੱਠ(cnum(fl, static_cast<uint32_t>(fl->lineno())));
        ਦਲੀਲਾਂ.ਧੱਕੋ_ਪਿੱਠ(cnum(fl, static_cast<uint32_t>(fl->ਪਹਿਲਾ_ਕਾਲਮ())));
        ਵਾਪਸ ਵਸਤੂ_Call(fl, cpVarp,
                        single ? VCMethod::COVERGROUP_ਜੋੜੋ_SINGLE_NAMER
                        : binp->binsType() == VCoverBinsType::BINS_AUTO_IMPLICIT
                            ? VCMethod::COVERGROUP_ਜੋੜੋ_NUMBERED_NAMER
                            : VCMethod::COVERGROUP_ਜੋੜੋ_ARRAY_NAMER,
                        ਦਲੀਲਾਂ)
            ->makeStmt();
    }

    // Emit 'if (iff && cond) m_cp.incrementBin(idx);' (or recordHit, + illegal action) in sample()
    // Where a bin's hit is recorded in the runtime VlCoverpoint member.
    ਬਣਤਰ Conv_Bin_ਨਿਸ਼ਾਨਾ final {
        AstVar* cpVarp;  // the __Vcp_<coverpoint> member
        uint32_t idx;  // bin index within that coverpoint
        ਤਾਰਕਿਕ is_ਸਾਧਾਰਨ;  // Normal -> incrementBin (count + cross hit list); else recordHit (count)
    };

    // Emit 'this->m_cp.incrementBin(idx);' (Normal) or '.recordHit(idx);'
    // (ignore/illegal/default).
    Ast_ਗੰਢ_Stmt* make_Runtime_Bin_ਪ੍ਰਾਪਤੀ(File_ਸਤਰ* fl, AstVar* cpVarp, Ast_ਗੰਢ_ਵਿਅੰਜਕ* idxp,
                                   ਤਾਰਕਿਕ is_ਸਾਧਾਰਨ) {
        ਵਾਪਸ ਵਸਤੂ_Call(fl, cpVarp,
                        is_ਸਾਧਾਰਨ ? VCMethod::COVERGROUP_INCREMENT_BIN
                                 : VCMethod::COVERGROUP_ਰਿਕਾਰਡ_ਪ੍ਰਾਪਤੀ,
                        {idxp})
            ->makeStmt();
    }
    Ast_ਗੰਢ_Stmt* make_Runtime_Bin_ਪ੍ਰਾਪਤੀ(File_ਸਤਰ* fl, ਅਚਲ Conv_Bin_ਨਿਸ਼ਾਨਾ& tgt) {
        ਵਾਪਸ make_Runtime_Bin_ਪ੍ਰਾਪਤੀ(fl, tgt.cpVarp, cnum(fl, static_cast<uint32_t>(tgt.idx)),
                                 tgt.is_ਸਾਧਾਰਨ);
    }

    // The condition under which a bin counts a sample: condp, and the bin's iff, and for a
    // Normal or default state bin, that the value is not excluded
    Ast_ਗੰਢ_ਵਿਅੰਜਕ* binCondition(AstCoverBin* binp, AstVar* cpVarp, Ast_ਗੰਢ_ਵਿਅੰਜਕ* condp) {
        File_ਸਤਰ* ਅਚਲ fl = binp->fileline();
        ਜੇ (binp->iffp()) condp = ਨਵਾਂ AstLogAnd{fl, binp->iffp()->clone_ਰੁੱਖ(ਝੂਠ), condp};
        ਅਚਲ ਸਵੈਚਾਲਿਤ excluded = m_excludedVars.ਲੱਭੋ(cpVarp);
        ਜੇ (excluded != m_excludedVars.ਸਿਰਾ() && !binp->transp()
            && (binp->binsType().bin_Is_ਸਾਧਾਰਨ()
                || binp->binsType() == VCoverBinsType::BINS_DEFAULT)) {
            condp = ਨਵਾਂ AstLogAnd{
                fl, ਨਵਾਂ AstNot{fl, ਨਵਾਂ AstVarRef{fl, excluded->ਦੂਜਾ, VAccess::READ}}, condp};
        }
        ਵਾਪਸ condp;
    }

    ਖਾਲੀ emit_Conv_ਪ੍ਰਾਪਤੀ_If(AstCoverpoint* coverpointp, AstCoverBin* binp, AstVar* cpVarp,
                       Ast_ਗੰਢ_ਵਿਅੰਜਕ* idxp, Ast_ਗੰਢ_ਵਿਅੰਜਕ* condp) {
        File_ਸਤਰ* ਅਚਲ fl = binp->fileline();
        Ast_ਗੰਢ* actionp = make_Runtime_Bin_ਪ੍ਰਾਪਤੀ(fl, cpVarp, idxp, binp->binsType().bin_Is_ਸਾਧਾਰਨ());
        ਜੇ (binp->binsType() == VCoverBinsType::BINS_ILLEGAL) {
            actionp->ਜੋੜੋ_ਅਗਲਾ(makeIllegalBinAction(fl, "Illegal bin " + binp->prettyNameQ()
                                                          + " hit in coverpoint "
                                                          + coverpointp->prettyNameQ()));
        }
        UASSERT_OBJ(m_sampleFuncp, binp, "sample() CFunc not set for coverpoint");
        ਜੋੜੋ_Sample_Stmt(ਨਵਾਂ AstIf{fl, binCondition(binp, cpVarp, condp), actionp, ਖਾਲੀ_ਸੂਚਕ});
    }

    // Emit the sample() code of sized array 'sized': count the value in its bins holding it when
    // enabled, as other bins are, and for default bins note in matchedp whether any holds it
    ਖਾਲੀ emitSizedSample(AstCoverpoint* coverpointp, AstCoverBin* binp, AstVar* cpVarp,
                         Ast_ਗੰਢ_ਵਿਅੰਜਕ* exprp, uint32_t sized, AstVar* matchedp) {
        File_ਸਤਰ* ਅਚਲ fl = binp->fileline();
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* enabledp = binCondition(binp, cpVarp, ਨਵਾਂ AstConst{fl, AstConst::ਦੋਆਧਾਰੀ_ਅੰਕ_True{}});
        UASSERT_OBJ(m_sampleFuncp, binp, "sample() CFunc not set for coverpoint");
        ਅਚਲ ਤਾਰਕਿਕ illegal = binp->binsType() == VCoverBinsType::BINS_ILLEGAL;
        ਜੇ (illegal) {
            // The illegal action reads the condition too, which is evaluated once, as the guard
            // may have side effects
            AstVar* ਅਚਲ varp
                = ਨਵਾਂ AstVar{fl, VVarType::BLOCKTEMP,
                             "__VcpEnabled_" + sanitizeGeneratedName(coverpointp->name()) + "_"
                                 + cvt_ਮੰਜ਼ਿਲ_Str(sized),
                             binp->ਲੱਭੋ_ਦੋਆਧਾਰੀ_ਅੰਕ_DType()};
            varp->funcLocal(ਸੱਚ);
            m_sampleFuncp->ਜੋੜੋ_Stmtsp(varp);
            ਜੋੜੋ_Sample_Stmt(ਨਵਾਂ Ast_ਸੌਂਪੋ{fl, ਨਵਾਂ AstVarRef{fl, varp, VAccess::WRITE}, enabledp});
            enabledp = ਨਵਾਂ AstVarRef{fl, varp, VAccess::READ};
        }
        AstCMethodHard* ਅਚਲ callp
            = ਵਸਤੂ_Call(fl, cpVarp,
                       exprp->isWide() ? VCMethod::COVERGROUP_SIZED_SAMPLE_W
                                       : VCMethod::COVERGROUP_SIZED_SAMPLE,
                       {cnum(fl, sized), exprp->clone_ਰੁੱਖ(ਝੂਠ), enabledp});
        callp->dtype_ਸੈੱਟ_ਦੋਆਧਾਰੀ_ਅੰਕ();
        ਜੇ (illegal) {
            ਜੋੜੋ_Sample_Stmt(ਨਵਾਂ AstIf{fl, ਨਵਾਂ AstLogAnd{fl, callp, enabledp->clone_ਰੁੱਖ(ਝੂਠ)},
                                    makeIllegalBinAction(fl, "Illegal bin " + binp->prettyNameQ()
                                                                 + " hit in coverpoint "
                                                                 + coverpointp->prettyNameQ())});
        } ਨਹੀਂਤਾਂ ਜੇ (matchedp && binp->binsType().bin_Is_ਸਾਧਾਰਨ()) {
            ਜੋੜੋ_Sample_Stmt(
                ਨਵਾਂ Ast_ਸੌਂਪੋ{fl, ਨਵਾਂ AstVarRef{fl, matchedp, VAccess::WRITE},
                              ਨਵਾਂ AstOr{fl, ਨਵਾਂ AstVarRef{fl, matchedp, VAccess::READ}, callp}});
        } ਨਹੀਂਤਾਂ {
            ਜੋੜੋ_Sample_Stmt(callp->makeStmt());
        }
    }

    // A variable local to the constructor
    AstVar* constructor_ਆਰਜ਼ੀ(File_ਸਤਰ* fl, ਅਚਲ string& name, Ast_ਗੰਢ_DType* dtypep) {
        AstVar* ਅਚਲ varp = ਨਵਾਂ AstVar{fl, VVarType::BLOCKTEMP, name, dtypep};
        varp->funcLocal(ਸੱਚ);
        m_constructorp->ਜੋੜੋ_Stmtsp(varp);
        ਵਾਪਸ varp;
    }

    // Truncate or extend, as its signedness sets, a value to a type
    ਸਥਿਰ Ast_ਗੰਢ_ਵਿਅੰਜਕ* resize_ਮੁੱਲ(Ast_ਗੰਢ_ਵਿਅੰਜਕ* valuep, Ast_ਗੰਢ_DType* dtypep) {
        File_ਸਤਰ* ਅਚਲ fl = valuep->fileline();
        ਜੇ (valuep->ਚੌੜਾਈ() > dtypep->ਚੌੜਾਈ()) {
            valuep = ਨਵਾਂ AstSel{fl, valuep, 0, dtypep->ਚੌੜਾਈ()};
        } ਨਹੀਂਤਾਂ ਜੇ (valuep->ਚੌੜਾਈ() < dtypep->ਚੌੜਾਈ()) {
            valuep = valuep->isSigned()
                         ? static_cast<Ast_ਗੰਢ_ਵਿਅੰਜਕ*>(ਨਵਾਂ AstExtendS{fl, valuep, dtypep->ਚੌੜਾਈ()})
                         : ਨਵਾਂ AstExtend{fl, valuep, dtypep->ਚੌੜਾਈ()};
        }
        valuep->dtypep(dtypep);
        ਵਾਪਸ valuep;
    }

    // The runtime arguments of a count, which the constructor evaluates once into a temporary:
    // its value, a count wider than 64 bits saturated over any limit, and whether it is positive
    std::pair<Ast_ਗੰਢ_ਵਿਅੰਜਕ*, Ast_ਗੰਢ_ਵਿਅੰਜਕ*> new_ਗਿਣਤੀ_ਦਲੀਲਾਂ(File_ਸਤਰ* fl, ਅਚਲ string& name,
                                                       Ast_ਗੰਢ_ਵਿਅੰਜਕ* exprp) {
        AstVar* ਅਚਲ countp = constructor_ਆਰਜ਼ੀ(fl, name, exprp->dtypep());
        m_constructorp->ਜੋੜੋ_Stmtsp(
            ਨਵਾਂ Ast_ਸੌਂਪੋ{fl, ਨਵਾਂ AstVarRef{fl, countp, VAccess::WRITE}, exprp->clone_ਰੁੱਖ(ਝੂਠ)});
        ਅਚਲ ਸਵੈਚਾਲਿਤ ਗਿਣਤੀ_Ref = [&]() { ਵਾਪਸ ਨਵਾਂ AstVarRef{fl, countp, VAccess::READ}; };
        AstConst* ਅਚਲ zerop = ਨਵਾਂ AstConst{fl, AstConst::DTyped{}, countp->dtypep()};
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ positivep
            = countp->isSigned() ? static_cast<Ast_ਗੰਢ_ਵਿਅੰਜਕ*>(ਨਵਾਂ AstGtS{fl, ਗਿਣਤੀ_Ref(), zerop})
                                 : ਨਵਾਂ AstNeq{fl, ਗਿਣਤੀ_Ref(), zerop};
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* valuep = resize_ਮੁੱਲ(ਗਿਣਤੀ_Ref(), countp->ਲੱਭੋ_UInt64_DType());
        ਜੇ (countp->ਚੌੜਾਈ() > VL_QUADSIZE) {
            valuep = ਨਵਾਂ AstCond{
                fl,
                ਨਵਾਂ AstRedOr{
                    fl, ਨਵਾਂ AstSel{fl, ਗਿਣਤੀ_Ref(), VL_QUADSIZE, countp->ਚੌੜਾਈ() - VL_QUADSIZE}},
                ਨਵਾਂ AstConst{fl, AstConst::Unsized64{}, std::numeric_limits<uint64_t>::max()},
                valuep};
            valuep->dtype_ਸੈੱਟ_UInt64();
        }
        ਵਾਪਸ {valuep, positivep};
    }
    // The runtime arguments of a constant count
    ਸਥਿਰ std::pair<Ast_ਗੰਢ_ਵਿਅੰਜਕ*, Ast_ਗੰਢ_ਵਿਅੰਜਕ*> new_ਗਿਣਤੀ_ਦਲੀਲਾਂ(File_ਸਤਰ* fl, uint64_t ਗਿਣਤੀ_2) {
        ਵਾਪਸ {ਨਵਾਂ AstConst{fl, AstConst::Unsized64{}, ਗਿਣਤੀ_2},
                ਨਵਾਂ AstConst{fl, AstConst::ਦੋਆਧਾਰੀ_ਅੰਕ_True{}}};
    }

    // The values of a coverpoint, which its name denotes (IEEE 1800-2023 19.5.1.1), as runs in
    // value order: an enumerated type's values (6.19), else all values of its type.  Bounds
    // are at runWidth(), sign-extended like a CrossValueRange's.
    ਸਥਿਰ std::ਸਦਿਸ਼<std::pair<V3_ਸੰਖਿਆ, V3_ਸੰਖਿਆ>> coverpoint_ਮੁੱਲਾਂ(Ast_ਗੰਢ* nodep,
                                                                       Ast_ਗੰਢ_ਵਿਅੰਜਕ* exprp) {
        ਅਚਲ ਪੂਰਨ ਚੌੜਾਈ = ਲੜੀਵਾਰ_ਚੌੜਾਈ(exprp);
        std::ਸਦਿਸ਼<std::pair<V3_ਸੰਖਿਆ, V3_ਸੰਖਿਆ>> runs;
        ਅਚਲ AstEnumDType* ਅਚਲ enump = VN_CAST(exprp->dtypep()->skip_Ref_ਮੰਜ਼ਿਲ_Enump(), EnumDType);
        ਜੇ (!enump) {
            ਅਚਲ Cross_ਮੁੱਲ_Range domain
                = cross_ਮੁੱਲ_Domain(nodep, exprp->ਚੌੜਾਈ(), exprp->isSigned(), ਚੌੜਾਈ);
            runs.emplace_ਪਿੱਠ(domain.lo, domain.hi);
            ਵਾਪਸ runs;
        }
        std::ਸਦਿਸ਼<V3_ਸੰਖਿਆ> ਮੁੱਲਾਂ;
        ਲਈ (ਅਚਲ Ast_Enum_ਵਸਤੂ* itemp = enump->itemsp(); itemp;
             itemp = VN_AS(itemp->nextp(), Enum_ਵਸਤੂ)) {
            ਅਚਲ V3_ਸੰਖਿਆ& ਅੰਕ = VN_AS(itemp->valuep(), Const)->ਅੰਕ();
            ਜੇ (ਅੰਕ.isFourState()) ਜਾਰੀ;  // Not a coverpoint value (19.5.7)
            ਮੁੱਲਾਂ.emplace_ਪਿੱਠ(nodep, ਚੌੜਾਈ);
            ਜੇ (exprp->isSigned()) {
                ਮੁੱਲਾਂ.ਪਿੱਠ().opExtendS(ਅੰਕ, ਅੰਕ.ਚੌੜਾਈ());
            } ਨਹੀਂਤਾਂ {
                ਮੁੱਲਾਂ.ਪਿੱਠ().op_ਸੌਂਪੋ(ਅੰਕ);
            }
        }
        std::ਛਾਂਟੋ(ਮੁੱਲਾਂ.begin(), ਮੁੱਲਾਂ.ਸਿਰਾ(), cross_ਮੁੱਲ_Less);
        ਅਚਲ V3_ਸੰਖਿਆ one{nodep, ਚੌੜਾਈ, 1};
        ਲਈ (ਅਚਲ V3_ਸੰਖਿਆ& ਮੁੱਲ : ਮੁੱਲਾਂ) {
            V3_ਸੰਖਿਆ ਅਗਲਾ{nodep, ਚੌੜਾਈ};
            ਜੇ (!runs.ਖਾਲੀ_ਥਾਵਾਂ() && ਅਗਲਾ.op_ਜੋੜੋ(runs.ਪਿੱਠ().ਦੂਜਾ, one).isCaseEq(ਮੁੱਲ)) {
                runs.ਪਿੱਠ().ਦੂਜਾ = ਮੁੱਲ;
            } ਨਹੀਂਤਾਂ {
                runs.emplace_ਪਿੱਠ(ਮੁੱਲ, ਮੁੱਲ);
            }
        }
        ਵਾਪਸ runs;
    }

    // Emit the constructor code building the bins 'binp' whose values it computes: the values of
    // each element that are coverpoint values (IEEE 1800-2023 19.5.7), those a 'with' filter
    // keeps (19.5.1.1), then its bins
    ਖਾਲੀ generateConstructedBins(AstCoverpoint* coverpointp, AstCoverBin* binp, AstVar* cpVarp,
                                 Ast_ਗੰਢ_ਵਿਅੰਜਕ* exprp) {
        File_ਸਤਰ* ਅਚਲ fl = binp->fileline();
        ਅਚਲ string ਅਗੇਤਰ
            = m_sized_ਨਾਂ.get(sanitizeGeneratedName(coverpointp->name() + "__" + binp->name()));
        // The count, and whether it is positive; one of a filter's scalar bin, or bin per value
        ਅਚਲ std::pair<Ast_ਗੰਢ_ਵਿਅੰਜਕ*, Ast_ਗੰਢ_ਵਿਅੰਜਕ*> ਗਿਣਤੀ_2
            = binp->arraySizep() ? new_ਗਿਣਤੀ_ਦਲੀਲਾਂ(fl, ਅਗੇਤਰ + "_count", binp->arraySizep())
                                 : new_ਗਿਣਤੀ_ਦਲੀਲਾਂ(fl, 1);
        AstCoverWith* ਅਚਲ withp = binWith(binp);
        ਜੇ (withp && !binRangesp(binp)) {  // The coverpoint's name: all of its values
            ਲਈ (ਅਚਲ std::pair<V3_ਸੰਖਿਆ, V3_ਸੰਖਿਆ>& ਲੜੀਵਾਰ : coverpoint_ਮੁੱਲਾਂ(binp, exprp)) {
                m_constructorp->ਜੋੜੋ_Stmtsp(ਵਸਤੂ_Call(fl, cpVarp,
                                                   exprp->isWide()
                                                       ? VCMethod::COVERGROUP_SIZED_RANGE_W
                                                       : VCMethod::COVERGROUP_SIZED_RANGE,
                                                   {new_ਮੁੱਲ_Const(fl, ਲੜੀਵਾਰ.ਪਹਿਲਾ, exprp),
                                                    new_ਮੁੱਲ_Const(fl, ਲੜੀਵਾਰ.ਦੂਜਾ, exprp)})
                                              ->makeStmt());
            }
        }
        uint32_t element = 0;
        ਲਈ (Ast_ਗੰਢ* rangep = binRangesp(binp); rangep; rangep = rangep->nextp()) {
            ਜੇ (VN_IS(rangep, Unbounded)) {  // A parameter of '$'
                binp->v3error("Bins value may not be '$', which may only bound a range "
                              "(IEEE 1800-2023 6.20.7)");
                ਜਾਰੀ;
            }
            generateSizedElement(cpVarp, binp, rangep, exprp, ਅਗੇਤਰ + "_" + cvt_ਮੰਜ਼ਿਲ_Str(element++));
        }
        ਜੇ (withp) generateWithFilter(binp, withp, cpVarp, exprp, ਅਗੇਤਰ);
        std::ਸਦਿਸ਼<Ast_ਗੰਢ_ਵਿਅੰਜਕ*> ਦਲੀਲਾਂ{ctext(fl, binp->binsType().bin_ਸੈੱਟ_Enum()), ਗਿਣਤੀ_2.ਪਹਿਲਾ,
                                       ਗਿਣਤੀ_2.ਦੂਜਾ};
        // A filter's bins have the limit it began with
        ਜੇ (!withp) ਦਲੀਲਾਂ.ਧੱਕੋ_ਪਿੱਠ(cnum(fl, v3Global.opt.coverageMaxBins()));
        ਅਚਲ ਤਾਰਕਿਕ prot = v3Global.opt.protectIds();
        ਦਲੀਲਾਂ.ਧੱਕੋ_ਪਿੱਠ(ctext(fl, quoted(VIdProtect::protect_ਸ਼ਬਦ_If(binp->name(), prot))));
        ਦਲੀਲਾਂ.ਧੱਕੋ_ਪਿੱਠ(ctext(fl, quoted(VIdProtect::protectIf(fl->filename(), prot))));
        ਦਲੀਲਾਂ.ਧੱਕੋ_ਪਿੱਠ(cnum(fl, static_cast<uint32_t>(fl->lineno())));
        ਦਲੀਲਾਂ.ਧੱਕੋ_ਪਿੱਠ(cnum(fl, static_cast<uint32_t>(fl->ਪਹਿਲਾ_ਕਾਲਮ())));
        m_constructorp->ਜੋੜੋ_Stmtsp(
            ਵਸਤੂ_Call(fl, cpVarp,
                     withp ? VCMethod::COVERGROUP_WITH_ਅੰਤ : VCMethod::COVERGROUP_SIZED_ਅੰਤ,
                     ਦਲੀਲਾਂ)
                ->makeStmt());
    }

    // Emit the constructor code evaluating the 'with' filter of 'binp' for each of its candidate
    // values, which sizedRange() added, passing the runs of values it keeps (IEEE 1800-2023
    // 19.5.1.1).  The filter is evaluated in a loop, whose code does not grow with the elements:
    //   withBegin(grouping, limit);
    //   more = 1;
    //   while (withNext()) {
    //       value = withLo(); last = withHi(); run = 0;
    //       while (true) {
    //           item = value;
    //           if (filter) { if (!run) { first = value; run = 1; } }
    //           else if (run) { more = withRun(first, value - 1); run = 0; }
    //           if (!more || value == last) break;
    //           ++value;
    //       }
    //       if (run) more = withRun(first, last);
    //   }
    ਖਾਲੀ generateWithFilter(AstCoverBin* binp, AstCoverWith* withp, AstVar* cpVarp,
                            Ast_ਗੰਢ_ਵਿਅੰਜਕ* exprp, ਅਚਲ string& ਅਗੇਤਰ) {
        File_ਸਤਰ* ਅਚਲ fl = withp->fileline();
        ਅਚਲ ਤਾਰਕਿਕ wide = exprp->isWide();
        ਅਚਲ string grouping = !binp->isArray()     ? "Single"
                                : binp->arraySizep() ? "Fixed"
                                                     : "Values";
        m_constructorp->ਜੋੜੋ_Stmtsp(ਵਸਤੂ_Call(fl, cpVarp, VCMethod::COVERGROUP_WITH_BEGIN,
                                           {ctext(fl, "VlCovBinGrouping::" + grouping),
                                            cnum(fl, v3Global.opt.coverageMaxBins())})
                                      ->makeStmt());
        // The candidates count in the coverpoint's width, and the filter reads each as 'item',
        // of the coverpoint's type, so that a filter changing 'item' cannot change the loop
        Ast_ਗੰਢ_DType* ਅਚਲ ਮੁੱਲ_DTypep = exprp->dtypep();
        AstVar* ਅਚਲ valuep = constructor_ਆਰਜ਼ੀ(fl, ਅਗੇਤਰ + "_value", ਮੁੱਲ_DTypep);
        AstVar* ਅਚਲ lastp = constructor_ਆਰਜ਼ੀ(fl, ਅਗੇਤਰ + "_last", ਮੁੱਲ_DTypep);
        AstVar* ਅਚਲ firstp = constructor_ਆਰਜ਼ੀ(fl, ਅਗੇਤਰ + "_first", ਮੁੱਲ_DTypep);
        AstVar* ਅਚਲ runp = constructor_ਆਰਜ਼ੀ(fl, ਅਗੇਤਰ + "_run", binp->ਲੱਭੋ_ਦੋਆਧਾਰੀ_ਅੰਕ_DType());
        AstVar* ਅਚਲ morep = constructor_ਆਰਜ਼ੀ(fl, ਅਗੇਤਰ + "_more", binp->ਲੱਭੋ_ਦੋਆਧਾਰੀ_ਅੰਕ_DType());
        AstVar* ਅਚਲ itemp = withp->itemp()->unlink_Fr_ਪਿੱਠ();
        itemp->name(ਅਗੇਤਰ + "_item");
        m_constructorp->ਜੋੜੋ_Stmtsp(itemp);
        ਅਚਲ ਸਵੈਚਾਲਿਤ ref = [&](AstVar* varp) { ਵਾਪਸ ਨਵਾਂ AstVarRef{fl, varp, VAccess::READ}; };
        ਅਚਲ ਸਵੈਚਾਲਿਤ ਸੌਂਪੋ_2 = [&](AstVar* varp, Ast_ਗੰਢ_ਵਿਅੰਜਕ* rhsp) -> Ast_ਗੰਢ* {
            ਵਾਪਸ ਨਵਾਂ Ast_ਸੌਂਪੋ{fl, ਨਵਾਂ AstVarRef{fl, varp, VAccess::WRITE}, rhsp};
        };
        ਅਚਲ ਸਵੈਚਾਲਿਤ flag = [&](AstVar* varp, ਤਾਰਕਿਕ ਮੁੱਲ) {
            ਵਾਪਸ ਸੌਂਪੋ_2(varp, ਮੁੱਲ ? ਨਵਾਂ AstConst{fl, AstConst::ਦੋਆਧਾਰੀ_ਅੰਕ_True{}}
                                      : ਨਵਾਂ AstConst{fl, AstConst::ਦੋਆਧਾਰੀ_ਅੰਕ_False{}});
        };
        ਅਚਲ ਸਵੈਚਾਲਿਤ bound = [&](VCMethod narrow, VCMethod wide_ਵਿਧੀ, AstVar* varp) -> Ast_ਗੰਢ* {
            ਜੇ (wide) {
                ਵਾਪਸ ਵਸਤੂ_Call(fl, cpVarp, wide_ਵਿਧੀ, {ਨਵਾਂ AstVarRef{fl, varp, VAccess::WRITE}})
                    ->makeStmt();
            }
            AstCMethodHard* ਅਚਲ callp = ਵਸਤੂ_Call(fl, cpVarp, narrow);
            callp->dtype_ਸੈੱਟ_UInt64();
            ਵਾਪਸ ਸੌਂਪੋ_2(varp, resize_ਮੁੱਲ(callp, ਮੁੱਲ_DTypep));
        };
        ਅਚਲ ਸਵੈਚਾਲਿਤ keep = [&](Ast_ਗੰਢ_ਵਿਅੰਜਕ* lop, Ast_ਗੰਢ_ਵਿਅੰਜਕ* hip) {
            AstCMethodHard* ਅਚਲ callp = ਵਸਤੂ_Call(
                fl, cpVarp, wide ? VCMethod::COVERGROUP_WITH_ਲੜੀਵਾਰ_W : VCMethod::COVERGROUP_WITH_ਲੜੀਵਾਰ,
                {lop, hip});
            callp->dtype_ਸੈੱਟ_ਦੋਆਧਾਰੀ_ਅੰਕ();
            ਵਾਪਸ ਸੌਂਪੋ_2(morep, callp);
        };
        ਅਚਲ ਸਵੈਚਾਲਿਤ ਕਦਮ = [&](ਤਾਰਕਿਕ ਉੱਪਰ) {
            AstConst* ਅਚਲ onep = ਨਵਾਂ AstConst{fl, AstConst::Widthed_ਮੁੱਲ{}, exprp->ਚੌੜਾਈ(), 1};
            Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ stepp
                = ਉੱਪਰ ? static_cast<Ast_ਗੰਢ_ਵਿਅੰਜਕ*>(ਨਵਾਂ Ast_ਜੋੜੋ{fl, ref(valuep), onep})
                     : ਨਵਾਂ AstSub{fl, ref(valuep), onep};
            stepp->dtypep(ਮੁੱਲ_DTypep);
            ਵਾਪਸ stepp;
        };
        AstLoop* ਅਚਲ innerp = ਨਵਾਂ AstLoop{fl};
        innerp->ਜੋੜੋ_Stmtsp(ਸੌਂਪੋ_2(itemp, ref(valuep)));
        innerp->ਜੋੜੋ_Stmtsp(ਨਵਾਂ AstIf{
            fl, withp->filterp()->unlink_Fr_ਪਿੱਠ(),
            ਨਵਾਂ AstIf{fl, ਨਵਾਂ AstNot{fl, ref(runp)},
                      ਸੌਂਪੋ_2(firstp, ref(valuep))->ਜੋੜੋ_ਅਗਲਾ(flag(runp, ਸੱਚ))},
            ਨਵਾਂ AstIf{fl, ref(runp), keep(ref(firstp), ਕਦਮ(ਝੂਠ))->ਜੋੜੋ_ਅਗਲਾ(flag(runp, ਝੂਠ))}});
        innerp->ਜੋੜੋ_Stmtsp(ਨਵਾਂ AstLoopTest{
            fl, innerp, ਨਵਾਂ AstLogAnd{fl, ref(morep), ਨਵਾਂ AstNeq{fl, ref(valuep), ref(lastp)}}});
        innerp->ਜੋੜੋ_Stmtsp(ਸੌਂਪੋ_2(valuep, ਕਦਮ(ਸੱਚ)));
        AstLoop* ਅਚਲ outerp = ਨਵਾਂ AstLoop{fl};
        AstCMethodHard* ਅਚਲ nextp = ਵਸਤੂ_Call(fl, cpVarp, VCMethod::COVERGROUP_WITH_ਅਗਲਾ);
        nextp->dtype_ਸੈੱਟ_ਦੋਆਧਾਰੀ_ਅੰਕ();
        outerp->ਜੋੜੋ_Stmtsp(ਨਵਾਂ AstLoopTest{fl, outerp, nextp});
        outerp->ਜੋੜੋ_Stmtsp(
            bound(VCMethod::COVERGROUP_WITH_LO, VCMethod::COVERGROUP_WITH_LO_W, valuep));
        outerp->ਜੋੜੋ_Stmtsp(
            bound(VCMethod::COVERGROUP_WITH_HI, VCMethod::COVERGROUP_WITH_HI_W, lastp));
        outerp->ਜੋੜੋ_Stmtsp(flag(runp, ਝੂਠ));
        outerp->ਜੋੜੋ_Stmtsp(innerp);
        outerp->ਜੋੜੋ_Stmtsp(ਨਵਾਂ AstIf{fl, ref(runp), keep(ref(firstp), ref(lastp))});
        m_constructorp->ਜੋੜੋ_Stmtsp(flag(morep, ਸੱਚ));
        m_constructorp->ਜੋੜੋ_Stmtsp(outerp);
    }

    // Emit 'sizedRange(lo, hi)' for the coverpoint values of an element of bins 'binp', whose
    // values the constructor computes: resolved now if constant, else when constructed by
    // clipping to the coverpoint's values.  A wildcard pattern's values give a range for each
    // run of them.  'prefix' names its temporaries.
    ਖਾਲੀ generateSizedElement(AstVar* cpVarp, ਅਚਲ AstCoverBin* binp, Ast_ਗੰਢ* rangep,
                              Ast_ਗੰਢ_ਵਿਅੰਜਕ* exprp, ਅਚਲ string& ਅਗੇਤਰ) {
        File_ਸਤਰ* ਅਚਲ fl = rangep->fileline();
        ਅਚਲ VCMethod ਵਿਧੀ = exprp->isWide() ? VCMethod::COVERGROUP_SIZED_RANGE_W
                                                : VCMethod::COVERGROUP_SIZED_RANGE;
        ਅਚਲ AstInsideRange* ਅਚਲ irp = VN_CAST(rangep, InsideRange);
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ lowp = irp ? irp->lhsp() : VN_AS(rangep, ਗੰਢ_ਵਿਅੰਜਕ);
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ highp = irp ? irp->rhsp() : ਖਾਲੀ_ਸੂਚਕ;
        ਅਚਲ ਸਵੈਚਾਲਿਤ unbounded
            = [](ਅਚਲ Ast_ਗੰਢ_ਵਿਅੰਜਕ* boundp) { ਵਾਪਸ !boundp || VN_IS(boundp, Unbounded); };
        ਅਚਲ ਸਵੈਚਾਲਿਤ constant
            = [&](ਅਚਲ Ast_ਗੰਢ_ਵਿਅੰਜਕ* boundp) { ਵਾਪਸ unbounded(boundp) || VN_IS(boundp, Const); };
        ਅਚਲ ਸਵੈਚਾਲਿਤ integral = [&](ਅਚਲ Ast_ਗੰਢ_ਵਿਅੰਜਕ* boundp) {
            ਵਾਪਸ unbounded(boundp) || boundp->dtypep()->skipRefp()->isIntegralOrPacked();
        };
        ਜੇ (constant(lowp) && constant(highp)) {
            ਅਚਲ ਸਵੈਚਾਲਿਤ fourState = [](ਅਚਲ Ast_ਗੰਢ_ਵਿਅੰਜਕ* boundp) {
                ਅਚਲ AstConst* ਅਚਲ constp = VN_CAST(boundp, Const);
                ਵਾਪਸ constp && constp->ਅੰਕ().isFourState();
            };
            ਜੇ (irp && (fourState(lowp) || fourState(highp))) {
                rangep->v3error("Four-state (x/z) value in "
                                << (binp->isArray() ? "array bins" : "bin")
                                << " range bound; range bounds must be two-state constants");
                ਵਾਪਸ;
            }
            Cross_ਮੁੱਲ_Range range{rangep, resolve_ਚੌੜਾਈ(rangep, exprp)};
            ਜੇ (!resolve_ਮੁੱਲ(rangep, exprp, ਸੱਚ, binp->isWildcard(), range)) {
                rangep->v3warn(E_UNSUPPORTED, "Unsupported: non-integral value in a coverage bin "
                                              "of an integral coverpoint.");
            } ਨਹੀਂਤਾਂ ਜੇ (!cross_Range_ਖਾਲੀ_ਥਾਵਾਂ(range)) {
                std::ਸਦਿਸ਼<std::pair<V3_ਸੰਖਿਆ, V3_ਸੰਖਿਆ>> runs{{range.lo, range.hi}};
                ਜੇ (range.wildcard) {  // checkConstructedBins bounded the runs
                    runs.clear();
                    crossRangeRuns(range, v3Global.opt.coverageMaxBins(), runs);
                }
                ਲਈ (ਅਚਲ std::pair<V3_ਸੰਖਿਆ, V3_ਸੰਖਿਆ>& ਲੜੀਵਾਰ : runs) {
                    m_constructorp->ਜੋੜੋ_Stmtsp(ਵਸਤੂ_Call(fl, cpVarp, ਵਿਧੀ,
                                                       {new_ਮੁੱਲ_Const(fl, ਲੜੀਵਾਰ.ਪਹਿਲਾ, exprp),
                                                        new_ਮੁੱਲ_Const(fl, ਲੜੀਵਾਰ.ਦੂਜਾ, exprp)})
                                                  ->makeStmt());
                }
            }
            ਵਾਪਸ;
        }
        ਜੇ (!integral(lowp) || !integral(highp)) {
            rangep->v3warn(E_UNSUPPORTED, "Unsupported: non-integral value in a coverage bin "
                                          "of an integral coverpoint.");
            ਵਾਪਸ;
        }
        // Compare values and the coverpoint's domain signed, in a width holding all of them
        ਅਚਲ ਸਵੈਚਾਲਿਤ bound_ਚੌੜਾਈ
            = [&](ਅਚਲ Ast_ਗੰਢ_ਵਿਅੰਜਕ* boundp) { ਵਾਪਸ unbounded(boundp) ? 0 : boundp->ਚੌੜਾਈ(); };
        ਅਚਲ ਪੂਰਨ ਚੌੜਾਈ = std::max({exprp->ਚੌੜਾਈ(), bound_ਚੌੜਾਈ(lowp), bound_ਚੌੜਾਈ(highp)}) + 1;
        ਅਚਲ Cross_ਮੁੱਲ_Range domain
            = cross_ਮੁੱਲ_Domain(rangep, exprp->ਚੌੜਾਈ(), exprp->isSigned(), ਚੌੜਾਈ);
        AstVar* ਅਚਲ lop = constructor_ਆਰਜ਼ੀ(fl, ਅਗੇਤਰ + "_lo", exprp->dtypep());
        lop->dtype_ਸੈੱਟ_ਦੋਆਧਾਰੀ_ਅੰਕ_Sized(ਚੌੜਾਈ, VSigning::SIGNED);
        AstVar* ਅਚਲ hip = constructor_ਆਰਜ਼ੀ(fl, ਅਗੇਤਰ + "_hi", lop->dtypep());
        ਅਚਲ ਸਵੈਚਾਲਿਤ bound = [&](Ast_ਗੰਢ_ਵਿਅੰਜਕ* boundp, ਅਚਲ V3_ਸੰਖਿਆ& ਹੱਦਬੰਦੀ) -> Ast_ਗੰਢ_ਵਿਅੰਜਕ* {
            ਜੇ (unbounded(boundp)) {
                AstConst* ਅਚਲ limitp = ਨਵਾਂ AstConst{fl, ਹੱਦਬੰਦੀ};
                limitp->dtypeFrom(lop);
                ਵਾਪਸ limitp;
            }
            Ast_ਗੰਢ_ਵਿਅੰਜਕ* valuep = boundp->clone_ਰੁੱਖ(ਝੂਠ);
            // A value no wider than a signed coverpoint has its type (see crossRangeBound)
            ਜੇ (exprp->isSigned() && boundp->ਚੌੜਾਈ() <= exprp->ਚੌੜਾਈ()) {
                valuep = resize_ਮੁੱਲ(valuep, exprp->dtypep());
            }
            ਵਾਪਸ resize_ਮੁੱਲ(valuep, lop->dtypep());
        };
        ਅਚਲ ਸਵੈਚਾਲਿਤ ref = [&](AstVar* varp, VAccess access = VAccess::READ) {
            ਵਾਪਸ ਨਵਾਂ AstVarRef{fl, varp, access};
        };
        m_constructorp->ਜੋੜੋ_Stmtsp(
            ਨਵਾਂ Ast_ਸੌਂਪੋ{fl, ref(lop, VAccess::WRITE), bound(lowp, domain.lo)});
        m_constructorp->ਜੋੜੋ_Stmtsp(
            ਨਵਾਂ Ast_ਸੌਂਪੋ{fl, ref(hip, VAccess::WRITE), irp ? bound(highp, domain.hi) : ref(lop)});
        AstConst* ਅਚਲ minp = ਨਵਾਂ AstConst{fl, domain.lo};
        AstConst* ਅਚਲ maxp = ਨਵਾਂ AstConst{fl, domain.hi};
        minp->dtypeFrom(lop);
        maxp->dtypeFrom(lop);
        AstCond* ਅਚਲ lowerp
            = ਨਵਾਂ AstCond{fl, ਨਵਾਂ AstLtS{fl, ref(lop), minp}, minp->clone_ਰੁੱਖ(ਝੂਠ), ref(lop)};
        AstCond* ਅਚਲ upperp
            = ਨਵਾਂ AstCond{fl, ਨਵਾਂ AstGtS{fl, ref(hip), maxp}, maxp->clone_ਰੁੱਖ(ਝੂਠ), ref(hip)};
        lowerp->dtypeFrom(lop);
        upperp->dtypeFrom(lop);
        m_constructorp->ਜੋੜੋ_Stmtsp(ਨਵਾਂ Ast_ਸੌਂਪੋ{fl, ref(lop, VAccess::WRITE), lowerp});
        m_constructorp->ਜੋੜੋ_Stmtsp(ਨਵਾਂ Ast_ਸੌਂਪੋ{fl, ref(hip, VAccess::WRITE), upperp});
        m_constructorp->ਜੋੜੋ_Stmtsp(ਨਵਾਂ AstIf{fl, ਨਵਾਂ AstLteS{fl, ref(lop), ref(hip)},
                                            ਵਸਤੂ_Call(fl, cpVarp, ਵਿਧੀ,
                                                     {resize_ਮੁੱਲ(ref(lop), exprp->dtypep()),
                                                      resize_ਮੁੱਲ(ref(hip), exprp->dtypep())})
                                                ->makeStmt()});
    }

    // The runtime index of the bin of a run holding the coverpoint value, which is in the run:
    // declared + (value - lo) / stride, capped at the last bin, which holds any remainder.
    ਸਥਿਰ Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਲੜੀਵਾਰ_Bin_ਸੂਚਕਾਂਕ(File_ਸਤਰ* fl, Ast_ਗੰਢ_ਵਿਅੰਜਕ* exprp, ਅਚਲ Bin_ਲੜੀਵਾਰ& ਲੜੀਵਾਰ) {
        ਜੇ (ਲੜੀਵਾਰ.m_ਗਿਣਤੀ == 1) ਵਾਪਸ cnum(fl, ਲੜੀਵਾਰ.m_declared);
        ਅਚਲ ਪੂਰਨ ਚੌੜਾਈ = exprp->ਚੌੜਾਈ();
        // A run spans at most 2^width values, so offsets in it are unsigned width-bit numbers
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* indexp
            = ਨਵਾਂ AstSub{fl, exprp->clone_ਰੁੱਖ(ਝੂਠ), new_ਮੁੱਲ_Const(fl, ਲੜੀਵਾਰ.m_lo, exprp)};
        indexp->dtype_ਸੈੱਟ_ਦੋਆਧਾਰੀ_ਅੰਕ_Sized(ਚੌੜਾਈ, VSigning::UNSIGNED);
        V3_ਸੰਖਿਆ stride{fl, ਚੌੜਾਈ, 0};
        stride.op_ਸੌਂਪੋ(ਲੜੀਵਾਰ.m_stride);
        ਜੇ (stride.ਗਿਣਤੀ_Ones() != 1) {
            indexp = ਨਵਾਂ AstDiv{fl, indexp, ਨਵਾਂ AstConst{fl, stride}};
        } ਨਹੀਂਤਾਂ ਜੇ (!stride.isEqOne()) {
            indexp = ਨਵਾਂ Ast_ਖਿਸਕਾਓ_R{fl, indexp, ਨਵਾਂ AstConst{fl, stride.most_ਸੈੱਟ_ਦੋਆਧਾਰੀ_ਅੰਕ_P1() - 1}};
        }
        // Compare the run's values with those of count bins of stride values, without overflow
        ਅਚਲ ਪੂਰਨ ext_ਚੌੜਾਈ = ਲੜੀਵਾਰ.m_lo.ਚੌੜਾਈ() + 1;
        V3_ਸੰਖਿਆ lo{fl, ext_ਚੌੜਾਈ, 0};
        lo.opExtendS(ਲੜੀਵਾਰ.m_lo, ਲੜੀਵਾਰ.m_lo.ਚੌੜਾਈ());
        V3_ਸੰਖਿਆ ਵਿਸਤਾਰ{fl, ext_ਚੌੜਾਈ, 0};
        ਵਿਸਤਾਰ.opExtendS(ਲੜੀਵਾਰ.m_hi, ਲੜੀਵਾਰ.m_hi.ਚੌੜਾਈ());
        ਵਿਸਤਾਰ.opSub(V3_ਸੰਖਿਆ{ਵਿਸਤਾਰ}, lo);
        V3_ਸੰਖਿਆ covered{fl, ext_ਚੌੜਾਈ, 0};
        covered.op_ਸੌਂਪੋ(ਲੜੀਵਾਰ.m_stride);
        covered.opMul(V3_ਸੰਖਿਆ{covered}, V3_ਸੰਖਿਆ{fl, ext_ਚੌੜਾਈ, ਲੜੀਵਾਰ.m_ਗਿਣਤੀ});
        V3_ਸੰਖਿਆ hasRemainder{fl, 1, 0};
        ਜੇ (!hasRemainder.opGte(ਵਿਸਤਾਰ, covered).isEqZero()) {
            AstConst* ਅਚਲ lastp = ਨਵਾਂ AstConst{fl, V3_ਸੰਖਿਆ{fl, ਚੌੜਾਈ, ਲੜੀਵਾਰ.m_ਗਿਣਤੀ - 1}};
            indexp = ਨਵਾਂ AstCond{fl, ਨਵਾਂ AstGt{fl, indexp->clone_ਰੁੱਖ(ਝੂਠ), lastp},
                                 lastp->clone_ਰੁੱਖ(ਝੂਠ), indexp};
        }
        ਜੇ (ਚੌੜਾਈ < VL_IDATASIZE) {
            indexp = ਨਵਾਂ AstExtend{fl, indexp, VL_IDATASIZE};
        } ਨਹੀਂਤਾਂ ਜੇ (ਚੌੜਾਈ > VL_IDATASIZE) {
            indexp = ਨਵਾਂ AstSel{fl, indexp, 0, VL_IDATASIZE};
        }
        ਵਾਪਸ ਨਵਾਂ Ast_ਜੋੜੋ{fl, cnum(fl, ਲੜੀਵਾਰ.m_declared), indexp};
    }

    // Emit the sample() hit of a run of bins, whose code does not grow with its number of bins:
    //   if (iff && lo <= value && value <= hi) m_cp.incrementBin(<runBinIndex>);
    ਖਾਲੀ emit_ਲੜੀਵਾਰ_ਪ੍ਰਾਪਤੀ(AstCoverpoint* coverpointp, AstCoverBin* binp, AstVar* cpVarp,
                    Ast_ਗੰਢ_ਵਿਅੰਜਕ* exprp, ਅਚਲ Bin_ਲੜੀਵਾਰ& ਲੜੀਵਾਰ) {
        File_ਸਤਰ* ਅਚਲ fl = binp->fileline();
        AstConst* ਅਚਲ lop = new_ਮੁੱਲ_Const(fl, ਲੜੀਵਾਰ.m_lo, exprp);
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* condp = ਖਾਲੀ_ਸੂਚਕ;
        ਜੇ (ਲੜੀਵਾਰ.m_lo.isCaseEq(ਲੜੀਵਾਰ.m_hi)) {
            condp = ਨਵਾਂ AstEq{fl, exprp->clone_ਰੁੱਖ(ਝੂਠ), lop};
        } ਨਹੀਂਤਾਂ {
            AstConst* ਅਚਲ hip = new_ਮੁੱਲ_Const(fl, ਲੜੀਵਾਰ.m_hi, exprp);
            condp = makeRangeCondition(fl, exprp, lop, hip);
            VL_DO_DANGLING(ਧੱਕੋ_Deletep(lop), lop);
            VL_DO_DANGLING(ਧੱਕੋ_Deletep(hip), hip);
        }
        emit_Conv_ਪ੍ਰਾਪਤੀ_If(coverpointp, binp, cpVarp, ਲੜੀਵਾਰ_Bin_ਸੂਚਕਾਂਕ(fl, exprp, ਲੜੀਵਾਰ), condp);
    }

    // Emit a transition bin's hit action into sample():
    //   if (cond) { m_cp.incrementBin/recordHit(idx); [illegal: $error; $stop] }
    // Used by the transition generators so a completed sequence records into the runtime bin.
    ਖਾਲੀ ਜੋੜੋ_Conv_Trans_ਪ੍ਰਾਪਤੀ_If(AstCoverpoint* coverpointp, AstCoverBin* binp, ਅਚਲ Conv_Bin_ਨਿਸ਼ਾਨਾ& tgt,
                           Ast_ਗੰਢ_ਵਿਅੰਜਕ* condp) {
        File_ਸਤਰ* ਅਚਲ fl = binp->fileline();
        Ast_ਗੰਢ* actionp = make_Runtime_Bin_ਪ੍ਰਾਪਤੀ(fl, tgt);
        ਜੇ (binp->binsType() == VCoverBinsType::BINS_ILLEGAL) {
            actionp->ਜੋੜੋ_ਅਗਲਾ(makeIllegalBinAction(
                fl, "Illegal transition bin " + binp->prettyNameQ() + " hit in coverpoint "
                        + coverpointp->prettyNameQ()));
        }
        UASSERT_OBJ(m_sampleFuncp, binp, "sample() CFunc not set for transition bin");
        ਜੋੜੋ_Sample_Stmt(ਨਵਾਂ AstIf{fl, condp, actionp, ਖਾਲੀ_ਸੂਚਕ});
    }

    // Route a coverpoint through a VlCoverpoint member: emit the member, its sample()
    // increments, the constructor configuration (init + namers), and registration.
    ਖਾਲੀ generateCoverpoint(AstCoverpoint* coverpointp, Ast_ਗੰਢ_ਵਿਅੰਜਕ* exprp, ਪੂਰਨ at_ਘੱਟ_ਤੋਂ_ਘੱਟ_ਮੁੱਲ) {
        File_ਸਤਰ* ਅਚਲ fl = coverpointp->fileline();
        ਅਚਲ ਤਾਰਕਿਕ dynamic = m_runtimePoints.ਗਿਣਤੀ_2(coverpointp);
        UINFO(4, "  Generating VlCoverpoint member: " << coverpointp->name());

        // Size the hit list to the gen-time max bin overlap (1 unless cross-fed with
        // overlapping ranges), so no cross hit is ever dropped and storage is minimal.
        ਅਚਲ ਤਾਰਕਿਕ crossFed = m_crossed_Cp_ਨਾਂ.ਗਿਣਤੀ_2(coverpointp->name()) != 0;
        ਅਚਲ ਪੂਰਨ ਪ੍ਰਾਪਤੀ_Bound = compute_ਪ੍ਰਾਪਤੀ_List_Bound(coverpointp, exprp, crossFed);
        UINFO(6, "    Hit-list bound (max bin overlap) = " << ਪ੍ਰਾਪਤੀ_Bound);
        AstVar* ਅਚਲ cpVarp = ਨਵਾਂ AstVar{fl, VVarType::MEMBER, "__Vcp_" + coverpointp->name(),
                                          coverpointDType(fl, static_cast<uint32_t>(ਪ੍ਰਾਪਤੀ_Bound))};
        m_covergroupp->ਜੋੜੋ_Membersp(cpVarp);
        m_cpVarMap[coverpointp->name()] = cpVarp;
        m_cpBins.emplace(cpVarp, CoverpointBins{});
        m_cpBins.at(cpVarp).exprp = exprp;
        m_cpBins.at(cpVarp).crossed = crossFed;
        // Create the runtime in the instance node first; everything below configures it.
        m_constructorp->ਜੋੜੋ_Stmtsp(make_ਵਸਤੂ_Create(fl, cpVarp, VCMethod::COVERGROUP_ਜੋੜੋ_COVERPOINT));
        generate_ਵਸਤੂ_ਭਾਰ(fl, cpVarp, coverpointp->optionsp());

        // A cross reads this coverpoint's hit list, so clear it at the start of the
        // coverpoint's sample() contribution (before any incrementBin appends to it), even
        // when the iff guard disables sampling.
        ਜੇ (crossFed) {
            UASSERT_OBJ(m_sampleFuncp, coverpointp, "sample() CFunc not set for clearHitList");
            m_sampleFuncp->ਜੋੜੋ_Stmtsp(
                ਵਸਤੂ_Call(fl, cpVarp, VCMethod::COVERGROUP_CLEAR_ਪ੍ਰਾਪਤੀ_LIST)->makeStmt());
        }
        ਜੇ (dynamic && coverpointHasStateExclusions(coverpointp)) {
            AstVar* ਅਚਲ excludedp
                = ਨਵਾਂ AstVar{fl, VVarType::BLOCKTEMP,
                             "__VcpExcluded_" + sanitizeGeneratedName(coverpointp->name()),
                             coverpointp->ਲੱਭੋ_ਦੋਆਧਾਰੀ_ਅੰਕ_DType()};
            excludedp->funcLocal(ਸੱਚ);
            m_sampleFuncp->ਜੋੜੋ_Stmtsp(excludedp);
            AstCMethodHard* ਅਚਲ callp
                = ਵਸਤੂ_Call(fl, cpVarp,
                           exprp->isWide() ? VCMethod::COVERGROUP_ਮੁੱਲ_EXCLUDED_W
                                           : VCMethod::COVERGROUP_ਮੁੱਲ_EXCLUDED,
                           {exprp->clone_ਰੁੱਖ(ਝੂਠ)});
            callp->dtype_ਸੈੱਟ_ਦੋਆਧਾਰੀ_ਅੰਕ();
            ਜੋੜੋ_Sample_Stmt(ਨਵਾਂ Ast_ਸੌਂਪੋ{fl, ਨਵਾਂ AstVarRef{fl, excludedp, VAccess::WRITE}, callp});
            m_excludedVars.emplace(cpVarp, excludedp);
        }

        // Walk bins (non-default, then default), assigning sequential indices that match the
        // namer append order; emit sample increments and collect namer statements.  Constructed
        // bins follow them all, placed when the coverpoint is constructed.
        std::ਸਦਿਸ਼<Ast_ਗੰਢ_Stmt*> namerStmts;
        std::ਸਦਿਸ਼<AstCoverBin*> defaultBins;
        std::ਸਦਿਸ਼<AstCoverBin*> sizedBins;
        std::ਸਦਿਸ਼<std::tuple<AstCoverBin*, uint32_t, Ast_ਗੰਢ_ਵਿਅੰਜਕ*>> metadata;
        std::ਸਦਿਸ਼<ਅਚਲ Bin_ਲੜੀਵਾਰ*> ਲੜੀਵਾਰ_Metadata;
        uint64_t idx = 0;  // Runtime index of the next bin; 32-bit once checked below
        ਲਈ (Ast_ਗੰਢ* binp = coverpointp->binsp(); binp; binp = binp->nextp()) {
            AstCoverBin* ਅਚਲ cbinp = VN_AS(binp, CoverBin);
            ਅਚਲ ਪੂਰਨ errorsBefore = dynamic ? V3Error::error_ਗਿਣਤੀ() : 0;
            ਜੇ (cbinp->binsType() == VCoverBinsType::BINS_DEFAULT) {
                defaultBins.ਧੱਕੋ_ਪਿੱਠ(cbinp);
                ਜਾਰੀ;
            }
            ਜੇ (isConstructedBins(cbinp)) {
                UASSERT_OBJ(dynamic, cbinp, "Constructed bins without value metadata");
                Bin_ਵਿਸਤਾਰ ਵਿਸਤਾਰ;
                ਵਿਸਤਾਰ.sized = static_cast<int32_t>(sizedBins.ਮਾਪ());
                m_cpBins.at(cpVarp).spans.emplace(cbinp->name(), ਵਿਸਤਾਰ);
                sizedBins.ਧੱਕੋ_ਪਿੱਠ(cbinp);
                ਜਾਰੀ;
            }
            ਜੇ (cbinp->transp()) {
                // Transition bin (incl. array transition 'bins t[] = (a=>b),(c=>d)' and
                // illegal_bins/ignore_bins transitions).  All sequences of one transition bin
                // share a bin name and merge in the coverage DB to a single point, so model
                // them as one runtime bin incremented by any matching sequence.  The sequence
                // matching is generated as a state machine, with the hit routed to this bin's
                // runtime slot.
                namerStmts.ਧੱਕੋ_ਪਿੱਠ(makeNamer(cpVarp, cbinp, -1, static_cast<uint32_t>(idx)));
                ਅਚਲ Conv_Bin_ਨਿਸ਼ਾਨਾ tgt{cpVarp, static_cast<uint32_t>(idx),
                                        cbinp->binsType().bin_Is_ਸਾਧਾਰਨ()};
                ਲਈ (Ast_ਗੰਢ* sp = cbinp->transp(); sp; sp = sp->nextp())
                    generate_Single_Transition_ਕੋਡ(coverpointp, cbinp, exprp, tgt,
                                                 VN_AS(sp, Cover_Trans_ਸੈੱਟ));
                ਜੇ (dynamic && V3Error::error_ਗਿਣਤੀ() == errorsBefore) {
                    metadata.emplace_ਪਿੱਠ(cbinp, static_cast<uint32_t>(idx), ਖਾਲੀ_ਸੂਚਕ);
                }
                ++idx;
                ਜਾਰੀ;
            }
            BinRuns plan;
            ਜੇ (binRunsFor(cbinp, exprp, plan)) {
                // Array elements and automatic bins generate as runs, so neither sample() nor
                // the constructor grows with their number of bins.
                ਜੇ (plan.unsupported) {  // bin ignored or invalid; reserve no slot
                    m_droppedBins[coverpointp].ਧੱਕੋ_ਪਿੱਠ(cbinp->name());
                    ਜਾਰੀ;
                }
                CoverpointBins& bins = m_cpBins.at(cpVarp);
                ਅਚਲ uint32_t ਪਹਿਲਾ_ਮੁੱਲ = bins.ਕੁੱਲ;
                ਅਚਲ uint32_t ਪਹਿਲਾ_Declared = static_cast<uint32_t>(idx);
                namerStmts.ਧੱਕੋ_ਪਿੱਠ(
                    makeNamer(cpVarp, cbinp, plan.ਗਿਣਤੀ_2, ਪਹਿਲਾ_Declared, {}, plan.ਮੁੱਲਾਂ));
                ਲਈ (Bin_ਲੜੀਵਾਰ& ਲੜੀਵਾਰ : plan.runs) {
                    ਲੜੀਵਾਰ.m_declared = static_cast<uint32_t>(idx);
                    bins.runs.ਧੱਕੋ_ਪਿੱਠ(std::move(ਲੜੀਵਾਰ));
                    ਅਚਲ Bin_ਲੜੀਵਾਰ& stored = bins.runs.ਪਿੱਠ();
                    ਜੇ (bins.crossed && cbinp->binsType().bin_Is_ਸਾਧਾਰਨ()) {
                        ਅਚਲ uint32_t ਪਹਿਲਾ = ਪਹਿਲਾ_ਮੁੱਲ + stored.m_declared - ਪਹਿਲਾ_Declared;
                        ਲਈ (uint32_t element = 0; element < stored.m_ਗਿਣਤੀ; ++element) {
                            bins.ਮੁੱਲਾਂ[ਪਹਿਲਾ + element].runp = &stored;
                            bins.ਮੁੱਲਾਂ[ਪਹਿਲਾ + element].element = element;
                        }
                    }
                    ਜੇ (!stored.m_ਖਾਲੀ_ਥਾਵਾਂ) {
                        emit_ਲੜੀਵਾਰ_ਪ੍ਰਾਪਤੀ(coverpointp, cbinp, cpVarp, exprp, stored);
                        ਜੇ (dynamic && V3Error::error_ਗਿਣਤੀ() == errorsBefore) {
                            ਲੜੀਵਾਰ_Metadata.ਧੱਕੋ_ਪਿੱਠ(&stored);
                        }
                    }
                    idx += stored.m_ਗਿਣਤੀ;
                }
                ਜਾਰੀ;
            }
            ਜੇ (cbinp->isArray()) {  // value array of a real coverpoint: b[0]..b[N-1]
                // Only integral coverpoints have runtime value metadata (m_runtimePoints)
                UASSERT_OBJ(!dynamic, cbinp, "Runtime value metadata for a real coverpoint");
                ਤਾਰਕਿਕ unsupported = ਝੂਠ;
                std::ਸਦਿਸ਼<Ast_ਗੰਢ_ਵਿਅੰਜਕ*> ਮੁੱਲਾਂ = ਕੱਢੋ_Array_ਮੁੱਲਾਂ(cbinp, exprp, unsupported);
                ਜੇ (unsupported) {  // bin ignored (COVERIGN emitted); reserve no slot
                    m_droppedBins[coverpointp].ਧੱਕੋ_ਪਿੱਠ(cbinp->name());
                    ਜਾਰੀ;
                }
                namerStmts.ਧੱਕੋ_ਪਿੱਠ(makeNamer(cpVarp, cbinp, static_cast<int64_t>(ਮੁੱਲਾਂ.ਮਾਪ()),
                                               static_cast<uint32_t>(idx), ਮੁੱਲਾਂ));
                ਲਈ (Ast_ਗੰਢ_ਵਿਅੰਜਕ* valuep : ਮੁੱਲਾਂ) {
                    // The cross selections of this covergroup still read the value.
                    m_detached_ਮੁੱਲਾਂ.ਧੱਕੋ_ਪਿੱਠ(valuep);
                    emit_Conv_ਪ੍ਰਾਪਤੀ_If(coverpointp, cbinp, cpVarp,
                                  cnum(cbinp->fileline(), static_cast<uint32_t>(idx)),
                                  build_ਮੁੱਲ_Condition(cbinp, exprp, valuep));
                    ++idx;
                }
            } ਨਹੀਂਤਾਂ {
                namerStmts.ਧੱਕੋ_ਪਿੱਠ(makeNamer(cpVarp, cbinp, -1, static_cast<uint32_t>(idx)));
                // buildBinCondition is null for 'ignore_bins = default' (no ranges); the bin
                // still gets a reserved slot (recorded, never incremented).
                ਜੇ (Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ condp = buildBinCondition(cbinp, exprp))
                    emit_Conv_ਪ੍ਰਾਪਤੀ_If(coverpointp, cbinp, cpVarp,
                                  cnum(cbinp->fileline(), static_cast<uint32_t>(idx)), condp);
                ਜੇ (dynamic && V3Error::error_ਗਿਣਤੀ() == errorsBefore) {
                    metadata.emplace_ਪਿੱਠ(cbinp, static_cast<uint32_t>(idx), ਖਾਲੀ_ਸੂਚਕ);
                }
                ++idx;
            }
        }
        // A cross selecting an ignored bins declaration selects no bins
        ਲਈ (ਅਚਲ std::string& name : m_droppedBins[coverpointp]) {
            m_cpBins.at(cpVarp).spans.emplace(name, Bin_ਵਿਸਤਾਰ{});
        }
        // A value a constructed bin holds is no default bin's; only sampling tells which do
        AstVar* sizedMatchedp = ਖਾਲੀ_ਸੂਚਕ;
        ਜੇ (!defaultBins.ਖਾਲੀ_ਥਾਵਾਂ()
            && std::any_ਦਾ(sizedBins.begin(), sizedBins.ਸਿਰਾ(), [](ਅਚਲ AstCoverBin* binp) {
                   ਵਾਪਸ binp->binsType().bin_Is_ਸਾਧਾਰਨ();
               })) {
            sizedMatchedp = ਨਵਾਂ AstVar{fl, VVarType::BLOCKTEMP,
                                       "__VcpSized_" + sanitizeGeneratedName(coverpointp->name()),
                                       coverpointp->ਲੱਭੋ_ਦੋਆਧਾਰੀ_ਅੰਕ_DType()};
            sizedMatchedp->funcLocal(ਸੱਚ);
            m_sampleFuncp->ਜੋੜੋ_Stmtsp(sizedMatchedp);
            ਜੋੜੋ_Sample_Stmt(ਨਵਾਂ Ast_ਸੌਂਪੋ{fl, ਨਵਾਂ AstVarRef{fl, sizedMatchedp, VAccess::WRITE},
                                        ਨਵਾਂ AstConst{fl, AstConst::ਦੋਆਧਾਰੀ_ਅੰਕ_False{}}});
        }
        ਲਈ (uint32_t sized = 0; sized < sizedBins.ਮਾਪ(); ++sized) {
            emitSizedSample(coverpointp, sizedBins[sized], cpVarp, exprp, sized, sizedMatchedp);
        }
        ਲਈ (AstCoverBin* ਅਚਲ defBinp : defaultBins) {
            File_ਸਤਰ* ਅਚਲ dfl = defBinp->fileline();
            namerStmts.ਧੱਕੋ_ਪਿੱਠ(makeNamer(cpVarp, defBinp, -1, static_cast<uint32_t>(idx)));
            Ast_ਗੰਢ_ਵਿਅੰਜਕ* condp = buildDefaultCondition(coverpointp, exprp, dfl);
            ਜੇ (sizedMatchedp) {
                condp = ਨਵਾਂ AstLogAnd{
                    dfl, ਨਵਾਂ AstNot{dfl, ਨਵਾਂ AstVarRef{dfl, sizedMatchedp, VAccess::READ}}, condp};
            }
            emit_Conv_ਪ੍ਰਾਪਤੀ_If(coverpointp, defBinp, cpVarp, cnum(dfl, static_cast<uint32_t>(idx)),
                          condp);
            ++idx;
        }
        ਜੇ (idx > std::numeric_limits<uint32_t>::max()) {
            // The runtime indexes bins with 32 bits; stop before generating a model
            coverpointp->v3warn(E_UNSUPPORTED, "Unsupported: coverpoint with more than "
                                                   << std::numeric_limits<uint32_t>::max()
                                                   << " bins");
        }

        // Transition coverpoints track the previous sampled value; update it once at the end of
        // this coverpoint's sample() contribution (the prev var was created on demand by the
        // transition matching above).
        ਜੇ (coverpointHasTransition(coverpointp)) {
            AstVar* ਅਚਲ prevVarp = VN_AS(coverpointp->user1p(), Var);
            ਜੋੜੋ_Sample_Stmt(
                ਨਵਾਂ Ast_ਸੌਂਪੋ{coverpointp->fileline(),
                              ਨਵਾਂ AstVarRef{prevVarp->fileline(), prevVarp, VAccess::WRITE},
                              exprp->clone_ਰੁੱਖ(ਝੂਠ)});
        }

        // Constructor: init (allocates), namers, then registration (under --coverage).
        // Under --protect-ids the hierarchy and page string reach the coverage database
        // verbatim, so obfuscate them like line/toggle points (per-word hierarchy, whole-
        // unit page).  No-ops when --protect-ids is off.
        ਅਚਲ ਤਾਰਕਿਕ prot = v3Global.opt.protectIds();
        ਅਚਲ std::string hier
            = VIdProtect::protect_ਸ਼ਬਦ_If(m_covergroupName + "." + coverpointp->name(), prot);
        m_constructorp->ਜੋੜੋ_Stmtsp(
            ਵਸਤੂ_Call(fl, cpVarp, VCMethod::COVERGROUP_INIT,
                     {ctext(fl, quoted(hier)), cnum(fl, static_cast<uint32_t>(at_ਘੱਟ_ਤੋਂ_ਘੱਟ_ਮੁੱਲ)),
                      cnum(fl, static_cast<uint32_t>(idx))})
                ->makeStmt());
        ਲਈ (Ast_ਗੰਢ_Stmt* ਅਚਲ ns : namerStmts) m_constructorp->ਜੋੜੋ_Stmtsp(ns);
        ਜੇ (dynamic) {
            m_constructorp->ਜੋੜੋ_Stmtsp(
                ਵਸਤੂ_Call(fl, cpVarp, VCMethod::COVERGROUP_ਮੁੱਲ_TYPE,
                         {cnum(fl, exprp->ਚੌੜਾਈ()), cnum(fl, exprp->isSigned())})
                    ->makeStmt());
            ਮੁੱਲ_Lists lists;
            ਲਈ (ਅਚਲ ਸਵੈਚਾਲਿਤ& ਇੰਦਰਾਜ : metadata) {
                collect_ਮੁੱਲ_Metadata(lists, exprp, std::get<0>(ਇੰਦਰਾਜ), std::get<1>(ਇੰਦਰਾਜ),
                                     std::get<2>(ਇੰਦਰਾਜ));
            }
            ਲਈ (ਅਚਲ Bin_ਲੜੀਵਾਰ* ਅਚਲ runp : ਲੜੀਵਾਰ_Metadata) collect_ਲੜੀਵਾਰ_Metadata(lists, exprp, *runp);
            emit_ਮੁੱਲ_List(fl, cpVarp, VCMethod::COVERGROUP_ਮੁੱਲ_RANGES, lists.m_ranges);
            emit_ਮੁੱਲ_List(fl, cpVarp, VCMethod::COVERGROUP_ਮੁੱਲ_RUNS, lists.m_runs);
            emit_ਮੁੱਲ_List(fl, cpVarp, VCMethod::COVERGROUP_ਮੁੱਲ_PATTERNS, lists.m_patterns);
            emit_ਮੁੱਲ_List(fl, cpVarp, VCMethod::COVERGROUP_ਮੁੱਲ_TRANSITIONS, lists.m_transitions);
            ਲਈ (AstCoverBin* ਅਚਲ binp : sizedBins) {
                generateConstructedBins(coverpointp, binp, cpVarp, exprp);
            }
            m_constructorp->ਜੋੜੋ_Stmtsp(
                ਵਸਤੂ_Call(fl, cpVarp, VCMethod::COVERGROUP_ਮੁੱਲ_FINALIZE)->makeStmt());
        }
        ਜੇ (v3Global.opt.coverage()) {
            ਅਚਲ std::string ਪੰਨਾ
                = VIdProtect::protectIf("v_covergroup/" + m_covergroupName, prot);
            m_constructorp->ਜੋੜੋ_Stmtsp(
                ਵਸਤੂ_Call(fl, cpVarp, VCMethod::COVERGROUP_ਰਜਿਸਟਰ_BINS,
                         {ctext(fl, "vlSymsp->_vm_contextp__->coveragep()"),
                          ctext(fl, quoted(ਪੰਨਾ)),
                          cnum(fl, ਵਸਤੂ_Database_ਭਾਰ(coverpointp->optionsp())),
                          cnum(fl, m_cg_Type_ਭਾਰ)})
                    ->makeStmt());
        }
    }

    // Generate state machine code for multi-value transition sequences
    // Handles transitions like (1 => 2 => 3 => 4)
    ਖਾਲੀ generate_Multi_ਮੁੱਲ_Transition_ਕੋਡ(AstCoverpoint* coverpointp, AstCoverBin* binp,
                                          Ast_ਗੰਢ_ਵਿਅੰਜਕ* exprp, ਅਚਲ Conv_Bin_ਨਿਸ਼ਾਨਾ& tgt,
                                          ਅਚਲ std::ਸਦਿਸ਼<Ast_Cover_Trans_ਵਸਤੂ*>& ਚੀਜ਼ਾਂ) {
        UINFO(4, "    Generating multi-value transition state machine for: " << binp->name());
        UINFO(4, "      Sequence length: " << ਚੀਜ਼ਾਂ.ਮਾਪ() << " items");

        // Create state position variable
        AstVar* ਅਚਲ stateVarp = createSequenceStateVar(coverpointp, binp);

        // Build case statement with N cases (one for each state 0 to N-1)
        // State 0: Not started, looking for first item
        // State 1 to N-1: In progress, looking for next item

        AstCase* ਅਚਲ casep
            = ਨਵਾਂ AstCase{binp->fileline(), VCaseType::CT_CASE,
                          ਨਵਾਂ AstVarRef{stateVarp->fileline(), stateVarp, VAccess::READ}, ਖਾਲੀ_ਸੂਚਕ};

        // Generate each case item in the switch statement
        ਲਈ (ਮਾਪ_t state = 0; state < ਚੀਜ਼ਾਂ.ਮਾਪ(); ++state) {
            Ast_Case_ਵਸਤੂ* caseItemp = generateTransitionStateCase(coverpointp, binp, exprp, tgt,
                                                                 stateVarp, ਚੀਜ਼ਾਂ, state);
            casep->ਜੋੜੋ_Itemsp(caseItemp);
        }

        // Add default case (reset to state 0) to prevent CASEINCOMPLETE warnings,
        // since the state variable is wider than the number of valid states.
        Ast_Case_ਵਸਤੂ* ਅਚਲ defaultItemp = ਨਵਾਂ Ast_Case_ਵਸਤੂ{
            binp->fileline(), ਖਾਲੀ_ਸੂਚਕ,
            ਨਵਾਂ Ast_ਸੌਂਪੋ{binp->fileline(),
                          ਨਵਾਂ AstVarRef{binp->fileline(), stateVarp, VAccess::WRITE},
                          ਨਵਾਂ AstConst{binp->fileline(), AstConst::Widthed_ਮੁੱਲ{}, 8, 0}}};
        casep->ਜੋੜੋ_Itemsp(defaultItemp);

        ਜੋੜੋ_Sample_Stmt(casep);
        UINFO(4, "      Successfully added multi-value transition state machine");
    }

    // Generate code for a single state in the transition state machine
    // Returns the case item for this state
    Ast_Case_ਵਸਤੂ* generateTransitionStateCase(AstCoverpoint* coverpointp, AstCoverBin* binp,
                                             Ast_ਗੰਢ_ਵਿਅੰਜਕ* exprp, ਅਚਲ Conv_Bin_ਨਿਸ਼ਾਨਾ& tgt,
                                             AstVar* stateVarp,
                                             ਅਚਲ std::ਸਦਿਸ਼<Ast_Cover_Trans_ਵਸਤੂ*>& ਚੀਜ਼ਾਂ,
                                             ਮਾਪ_t state) {
        File_ਸਤਰ* ਅਚਲ fl = binp->fileline();

        // Build condition for current value matching expected item at this state
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ ਮੇਲ_Condp = build_Transition_ਵਸਤੂ_Condition(ਚੀਜ਼ਾਂ[state], exprp);

        Ast_ਗੰਢ_Stmt* ਮੇਲ_Actionp = ਖਾਲੀ_ਸੂਚਕ;

        ਜੇ (state == ਚੀਜ਼ਾਂ.ਮਾਪ() - 1) {
            // Last state: sequence complete!  Record the hit in the runtime VlCoverpoint.
            ਮੇਲ_Actionp = make_Runtime_Bin_ਪ੍ਰਾਪਤੀ(fl, tgt);

            // For illegal_bins, add error message
            ਜੇ (binp->binsType() == VCoverBinsType::BINS_ILLEGAL) {
                ਅਚਲ string err_ਸੁਨੇਹਾ = "Illegal transition bin " + binp->prettyNameQ()
                                      + " hit in coverpoint " + coverpointp->prettyNameQ();
                ਮੇਲ_Actionp = ਮੇਲ_Actionp->ਜੋੜੋ_ਅਗਲਾ(makeIllegalBinAction(fl, err_ਸੁਨੇਹਾ));
            }

            // Reset state to 0
            ਮੇਲ_Actionp = ਮੇਲ_Actionp->ਜੋੜੋ_ਅਗਲਾ(
                ਨਵਾਂ Ast_ਸੌਂਪੋ{fl, ਨਵਾਂ AstVarRef{fl, stateVarp, VAccess::WRITE},
                              ਨਵਾਂ AstConst{fl, AstConst::Widthed_ਮੁੱਲ{}, 8, 0}});
        } ਨਹੀਂਤਾਂ {
            // Intermediate state: advance to next state
            ਮੇਲ_Actionp = ਨਵਾਂ Ast_ਸੌਂਪੋ{
                fl, ਨਵਾਂ AstVarRef{fl, stateVarp, VAccess::WRITE},
                ਨਵਾਂ AstConst{fl, AstConst::Widthed_ਮੁੱਲ{}, 8, static_cast<uint32_t>(state + 1)}};
        }

        // Build restart logic: check if current value matches first item
        // If so, restart sequence from state 1 (even if we're in middle of sequence)
        Ast_ਗੰਢ_Stmt* no_ਮੇਲ_Actionp = ਖਾਲੀ_ਸੂਚਕ;
        ਜੇ (state > 0) {
            // Check if current value matches first item (restart condition)
            Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ restartCondp = build_Transition_ਵਸਤੂ_Condition(ਚੀਜ਼ਾਂ[0], exprp);

            UASSERT_OBJ(restartCondp, ਚੀਜ਼ਾਂ[0],
                        "buildTransitionItemCondition returned nullptr for restart");

            // Restart to state 1
            Ast_ਗੰਢ_Stmt* restartActionp
                = ਨਵਾਂ Ast_ਸੌਂਪੋ{fl, ਨਵਾਂ AstVarRef{fl, stateVarp, VAccess::WRITE},
                                ਨਵਾਂ AstConst{fl, AstConst::Widthed_ਮੁੱਲ{}, 8, 1}};

            // Reset to state 0 (else branch)
            Ast_ਗੰਢ_Stmt* resetActionp
                = ਨਵਾਂ Ast_ਸੌਂਪੋ{fl, ਨਵਾਂ AstVarRef{fl, stateVarp, VAccess::WRITE},
                                ਨਵਾਂ AstConst{fl, AstConst::Widthed_ਮੁੱਲ{}, 8, 0}};

            no_ਮੇਲ_Actionp = ਨਵਾਂ AstIf{fl, restartCondp, restartActionp, resetActionp};
        }
        // For state 0, no action needed if no match (stay in state 0)

        // Combine into if-else
        Ast_ਗੰਢ_Stmt* ਅਚਲ stmtp = ਨਵਾਂ AstIf{fl, ਮੇਲ_Condp, ਮੇਲ_Actionp, no_ਮੇਲ_Actionp};

        // Create case item for this state value
        Ast_Case_ਵਸਤੂ* ਅਚਲ caseItemp = ਨਵਾਂ Ast_Case_ਵਸਤੂ{
            fl, ਨਵਾਂ AstConst{fl, AstConst::Widthed_ਮੁੱਲ{}, 8, static_cast<uint32_t>(state)},
            stmtp};

        ਵਾਪਸ caseItemp;
    }

    // Create: $error(msg); $stop;  Used when an illegal bin is hit.
    Ast_ਗੰਢ_Stmt* makeIllegalBinAction(File_ਸਤਰ* fl, ਅਚਲ string& err_ਸੁਨੇਹਾ) {
        AstDisplay* ਅਚਲ errorp
            = ਨਵਾਂ AstDisplay{fl, VDisplayType::DT_ERROR, err_ਸੁਨੇਹਾ, ਖਾਲੀ_ਸੂਚਕ, ਖਾਲੀ_ਸੂਚਕ};
        errorp->fmtp()->timeunit(m_covergroupp->timeunit());
        static_cast<Ast_ਗੰਢ*>(errorp)->ਜੋੜੋ_ਅਗਲਾ(ਨਵਾਂ AstStop{fl, ਸੱਚ});
        ਵਾਪਸ errorp;
    }

    // Preserve the coverpoint's width and signedness after V3Width.
    ਸਥਿਰ AstConst* new_ਮੁੱਲ_Const(File_ਸਤਰ* fl, ਅਚਲ V3_ਸੰਖਿਆ& ਮੁੱਲ, ਅਚਲ Ast_ਗੰਢ_ਵਿਅੰਜਕ* exprp) {
        V3_ਸੰਖਿਆ narrowed{fl, exprp->ਚੌੜਾਈ(), 0};
        narrowed.op_ਸੌਂਪੋ(ਮੁੱਲ);
        AstConst* ਅਚਲ constp = ਨਵਾਂ AstConst{fl, narrowed};
        constp->dtypeFrom(exprp);
        ਵਾਪਸ constp;
    }

    // A real copy of a real or integral range bound, for comparing with a real coverpoint.
    ਸਥਿਰ AstConst* newRealConst(AstConst* constp) {
        ਜੇ (constp->ਅੰਕ().isDouble()) ਵਾਪਸ constp->clone_ਰੁੱਖ(ਝੂਠ);
        V3_ਸੰਖਿਆ real{&constp->ਅੰਕ(), 64};
        real.opIToRD(constp->ਅੰਕ(), constp->isSigned());
        ਵਾਪਸ ਨਵਾਂ AstConst{constp->fileline(), real};
    }

    // Clone a constant node, widening to targetWidth if needed (zero-extend).
    // Used to ensure comparisons use matching widths after V3Width has run.
    ਸਥਿਰ AstConst* widenConst(File_ਸਤਰ* fl, AstConst* constp, ਪੂਰਨ ਨਿਸ਼ਾਨਾ_ਚੌੜਾਈ) {
        ਜੇ (constp->ਚੌੜਾਈ() == ਨਿਸ਼ਾਨਾ_ਚੌੜਾਈ) ਵਾਪਸ constp->clone_ਰੁੱਖ(ਝੂਠ);
        V3_ਸੰਖਿਆ ਅੰਕ{fl, ਨਿਸ਼ਾਨਾ_ਚੌੜਾਈ, 0};
        ਅੰਕ.op_ਸੌਂਪੋ(constp->ਅੰਕ());
        ਵਾਪਸ ਨਵਾਂ AstConst{fl, ਅੰਕ};
    }

    // Build a range condition: minp <= exprp <= maxp.
    // Uses signed comparisons if exprp is signed; omits trivially-true domain bounds.
    // All arguments are non-owning; clones exprp/minp/maxp as needed.
    Ast_ਗੰਢ_ਵਿਅੰਜਕ* makeRangeCondition(File_ਸਤਰ* fl, Ast_ਗੰਢ_ਵਿਅੰਜਕ* exprp, Ast_ਗੰਢ_ਵਿਅੰਜਕ* minp,
                                    Ast_ਗੰਢ_ਵਿਅੰਜਕ* maxp) {
        ਅਚਲ ਪੂਰਨ ਵਿਅੰਜਕ_ਚੌੜਾਈ = exprp->ਚੌੜਾਈ_Min();
        AstConst* ਅਚਲ minConstp = VN_AS(minp, Const);
        AstConst* ਅਚਲ maxConstp = VN_AS(maxp, Const);
        ਜੇ (exprp->isDouble()) {
            // A real coverpoint has no finite domain bounds to omit.
            ਵਾਪਸ ਨਵਾਂ AstAnd{fl,
                              ਨਵਾਂ AstGteD{fl, exprp->clone_ਰੁੱਖ(ਝੂਠ), newRealConst(minConstp)},
                              ਨਵਾਂ AstLteD{fl, exprp->clone_ਰੁੱਖ(ਝੂਠ), newRealConst(maxConstp)}};
        }
        // Widen constants to match expression width so post-V3Width nodes use correct macros
        AstConst* ਅਚਲ minWidep = widenConst(fl, minConstp, ਵਿਅੰਜਕ_ਚੌੜਾਈ);
        AstConst* ਅਚਲ maxWidep = widenConst(fl, maxConstp, ਵਿਅੰਜਕ_ਚੌੜਾਈ);
        V3_ਸੰਖਿਆ minimum{fl, ਵਿਅੰਜਕ_ਚੌੜਾਈ, 0};
        V3_ਸੰਖਿਆ ਵੱਧ_ਤੋਂ_ਵੱਧ{fl, ਵਿਅੰਜਕ_ਚੌੜਾਈ, 0};
        ਵੱਧ_ਤੋਂ_ਵੱਧ.ਸੈੱਟ_All_Bits1();
        ਜੇ (exprp->isSigned()) {
            minimum.ਸੈੱਟ_ਦੋਆਧਾਰੀ_ਅੰਕ(ਵਿਅੰਜਕ_ਚੌੜਾਈ - 1, 1);
            ਵੱਧ_ਤੋਂ_ਵੱਧ.ਸੈੱਟ_ਦੋਆਧਾਰੀ_ਅੰਕ(ਵਿਅੰਜਕ_ਚੌੜਾਈ - 1, 0);
        }
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* lowerp = ਖਾਲੀ_ਸੂਚਕ;
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* upperp = ਖਾਲੀ_ਸੂਚਕ;
        ਜੇ (minWidep->ਅੰਕ().isCaseEq(minimum)) {
            VL_DO_DANGLING(ਧੱਕੋ_Deletep(minWidep), minWidep);
        } ਨਹੀਂਤਾਂ {
            lowerp = exprp->isSigned() ? static_cast<Ast_ਗੰਢ_ਵਿਅੰਜਕ*>(
                                             ਨਵਾਂ AstGteS{fl, exprp->clone_ਰੁੱਖ(ਝੂਠ), minWidep})
                                       : static_cast<Ast_ਗੰਢ_ਵਿਅੰਜਕ*>(
                                             ਨਵਾਂ AstGte{fl, exprp->clone_ਰੁੱਖ(ਝੂਠ), minWidep});
        }
        ਜੇ (maxWidep->ਅੰਕ().isCaseEq(ਵੱਧ_ਤੋਂ_ਵੱਧ)) {
            VL_DO_DANGLING(ਧੱਕੋ_Deletep(maxWidep), maxWidep);
        } ਨਹੀਂਤਾਂ {
            upperp = exprp->isSigned() ? static_cast<Ast_ਗੰਢ_ਵਿਅੰਜਕ*>(
                                             ਨਵਾਂ AstLteS{fl, exprp->clone_ਰੁੱਖ(ਝੂਠ), maxWidep})
                                       : static_cast<Ast_ਗੰਢ_ਵਿਅੰਜਕ*>(
                                             ਨਵਾਂ AstLte{fl, exprp->clone_ਰੁੱਖ(ਝੂਠ), maxWidep});
        }
        ਜੇ (lowerp && upperp) ਵਾਪਸ ਨਵਾਂ AstAnd{fl, lowerp, upperp};
        ਜੇ (lowerp) ਵਾਪਸ lowerp;
        ਜੇ (upperp) ਵਾਪਸ upperp;
        ਵਾਪਸ ਨਵਾਂ AstConst{fl, AstConst::ਦੋਆਧਾਰੀ_ਅੰਕ_True{}};
    }

    // Build a one-sided comparison for an open-ended bin range whose other bound is '$'.
    // '$' denotes the coverpoint domain extreme, so {[lo:$]} == (expr >= lo) and
    // {[$:hi]} == (expr <= hi).
    Ast_ਗੰਢ_ਵਿਅੰਜਕ* makeOpenRangeCondition(File_ਸਤਰ* fl, Ast_ਗੰਢ_ਵਿਅੰਜਕ* exprp, AstConst* boundp,
                                        ਤਾਰਕਿਕ is_ਹੇਠਲਾ_Bound) {
        AstConst* ਅਚਲ widep = widenConst(fl, boundp, exprp->ਚੌੜਾਈ_Min());
        ਜੇ (is_ਹੇਠਲਾ_Bound) {
            ਜੇ (exprp->isSigned()) ਵਾਪਸ ਨਵਾਂ AstGteS{fl, exprp->clone_ਰੁੱਖ(ਝੂਠ), widep};
            ਵਾਪਸ ਨਵਾਂ AstGte{fl, exprp->clone_ਰੁੱਖ(ਝੂਠ), widep};
        }
        ਜੇ (exprp->isSigned()) ਵਾਪਸ ਨਵਾਂ AstLteS{fl, exprp->clone_ਰੁੱਖ(ਝੂਠ), widep};
        ਵਾਪਸ ਨਵਾਂ AstLte{fl, exprp->clone_ਰੁੱਖ(ਝੂਠ), widep};
    }

    // Build condition for a single transition item.
    // Returns expression that checks if exprp matches the item's value/range list.
    // Overload for when the expression is a variable read -- creates and manages the VarRef
    // internally, so callers don't need to construct a temporary node.
    Ast_ਗੰਢ_ਵਿਅੰਜਕ* build_Transition_ਵਸਤੂ_Condition(Ast_Cover_Trans_ਵਸਤੂ* itemp, AstVar* varp) {
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* varRefp = ਨਵਾਂ AstVarRef{varp->fileline(), varp, VAccess::READ};
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ condp = build_Transition_ਵਸਤੂ_Condition(itemp, varRefp);
        VL_DO_DANGLING(ਧੱਕੋ_Deletep(varRefp), varRefp);
        ਵਾਪਸ condp;
    }

    // Non-owning: exprp is cloned internally; caller retains ownership of exprp.
    Ast_ਗੰਢ_ਵਿਅੰਜਕ* build_Transition_ਵਸਤੂ_Condition(Ast_Cover_Trans_ਵਸਤੂ* itemp, Ast_ਗੰਢ_ਵਿਅੰਜਕ* exprp) {
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* condp = ਖਾਲੀ_ਸੂਚਕ;

        ਲਈ (Ast_ਗੰਢ* valp = itemp->valuesp(); valp; valp = valp->nextp()) {
            Ast_ਗੰਢ_ਵਿਅੰਜਕ* singleCondp = ਖਾਲੀ_ਸੂਚਕ;
            valp = V3Const::constifyEdit(valp);
            AstConst* ਅਚਲ constp = VN_CAST(valp, Const);
            ਜੇ (!constp) {
                valp->v3error("Non-constant expression in transition bin; "
                              "values must be constants (IEEE 1800-2023 19.5)");
                ਵਾਪਸ ਨਵਾਂ AstConst{valp->fileline(), AstConst::ਦੋਆਧਾਰੀ_ਅੰਕ_False_Erroring{}};
            }
            singleCondp
                = ਨਵਾਂ AstEq{constp->fileline(), exprp->clone_ਰੁੱਖ(ਝੂਠ), constp->clone_ਰੁੱਖ(ਝੂਠ)};
            ਜੇ (condp) {
                condp = ਨਵਾਂ AstOr{itemp->fileline(), condp, singleCondp};
            } ਨਹੀਂਤਾਂ {
                condp = singleCondp;
            }
        }

        ਵਾਪਸ condp;
    }

    // Generate code for a single transition sequence (used by both regular and array bins)
    ਖਾਲੀ generate_Single_Transition_ਕੋਡ(AstCoverpoint* coverpointp, AstCoverBin* binp,
                                      Ast_ਗੰਢ_ਵਿਅੰਜਕ* exprp, ਅਚਲ Conv_Bin_ਨਿਸ਼ਾਨਾ& tgt,
                                      Ast_Cover_Trans_ਸੈੱਟ* transSetp) {
        UINFO(4, "      Generating code for transition sequence");

        // Get or create previous value variable
        AstVar* ਅਚਲ prevVarp = create_Prev_ਮੁੱਲ_Var(coverpointp, exprp);

        UASSERT_OBJ(
            transSetp, binp,
            "Transition bin has no transition set (transp() was checked before calling this)");

        // Get transition items (the sequence: item1 => item2 => item3)
        std::ਸਦਿਸ਼<Ast_Cover_Trans_ਵਸਤੂ*> ਚੀਜ਼ਾਂ;
        ਲਈ (Ast_ਗੰਢ* itemp = transSetp->itemsp(); itemp; itemp = itemp->nextp())
            ਚੀਜ਼ਾਂ.ਧੱਕੋ_ਪਿੱਠ(VN_AS(itemp, Cover_Trans_ਵਸਤੂ));

        ਜੇ (ਚੀਜ਼ਾਂ.ਖਾਲੀ_ਥਾਵਾਂ()) {
            binp->v3error("Transition set without items");
            ਵਾਪਸ;
        }

        ਜੇ (ਚੀਜ਼ਾਂ.ਮਾਪ() == 1) {
            // Single item transition not valid (need at least 2 values for =>)
            binp->v3error("Transition requires at least two values");
            ਵਾਪਸ;
        } ਨਹੀਂਤਾਂ ਜੇ (ਚੀਜ਼ਾਂ.ਮਾਪ() == 2) {
            // Simple two-value transition: (val1 => val2)
            // Use optimized direct comparison (no state machine needed)
            Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ cond1p = build_Transition_ਵਸਤੂ_Condition(ਚੀਜ਼ਾਂ[0], prevVarp);
            Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ cond2p = build_Transition_ਵਸਤੂ_Condition(ਚੀਜ਼ਾਂ[1], exprp);

            // Combine: prev matches val1 AND current matches val2
            Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਭਰਿਆ_Condp = ਨਵਾਂ AstAnd{binp->fileline(), cond1p, cond2p};

            ਜੋੜੋ_Conv_Trans_ਪ੍ਰਾਪਤੀ_If(coverpointp, binp, tgt, ਭਰਿਆ_Condp);

            UINFO(4, "        Successfully added 2-value transition if statement");
        } ਨਹੀਂਤਾਂ {
            // Multi-value sequence (a => b => c => ...)
            // Use state machine to track position in sequence
            generate_Multi_ਮੁੱਲ_Transition_ਕੋਡ(coverpointp, binp, exprp, tgt, ਚੀਜ਼ਾਂ);
        }
    }

    // Append a "{ VlCoverpoint* __Vcx_cps[] = {cp0, cp1, ...}; <call> }" statement.  The brace
    // and the temporary array stay literal text -- a CMethodHard is one call, not a block --
    // but callp itself carries the member, method and '->'.  Construction only: init() copies
    // the array into the cross, so sample() reads it from there and needs no array at all.
    AstCStmt* makeCrossCpsCall(File_ਸਤਰ* fl, ਅਚਲ std::ਸਦਿਸ਼<AstVar*>& cpVars,
                               AstCMethodHard* callp) {
        AstCStmt* ਅਚਲ cs = ਨਵਾਂ AstCStmt{fl};
        cs->ਜੋੜੋ("{ VlCoverpoint* __Vcx_cps[] = {");
        ਲਈ (ਮਾਪ_t d = 0; d < cpVars.ਮਾਪ(); ++d) {
            ਜੇ (d != 0) cs->ਜੋੜੋ(", ");
            cs->ਜੋੜੋ(memberRef(fl, cpVars[d]));
        }
        cs->ਜੋੜੋ("}; ");
        cs->ਜੋੜੋ(callp);
        cs->ਜੋੜੋ("; }");
        ਵਾਪਸ cs;
    }

    // Assign the per-bin flags individually: one-bit SV results have integer C++ storage types,
    // which may narrow in a bool initializer list but convert implicitly in assignments.
    AstCStmt* makeCrossIffsCall(File_ਸਤਰ* fl, ਅਚਲ std::ਸਦਿਸ਼<AstCoverCrossBin*>& bins,
                                AstCMethodHard* callp) {
        AstCStmt* ਅਚਲ cs = ਨਵਾਂ AstCStmt{fl};
        cs->ਜੋੜੋ("{ bool __Vcx_iffs[" + cvt_ਮੰਜ਼ਿਲ_Str(bins.ਮਾਪ()) + "]; ");
        ਲਈ (ਮਾਪ_t i = 0; i < bins.ਮਾਪ(); ++i) {
            ਅਚਲ AstCoverCrossBin* ਅਚਲ binp = bins[i];
            cs->ਜੋੜੋ("__Vcx_iffs[" + cvt_ਮੰਜ਼ਿਲ_Str(i) + "] = ");
            cs->ਜੋੜੋ(binp->iffp() ? binp->iffp()->clone_ਰੁੱਖ(ਝੂਠ)
                                 : ਨਵਾਂ AstConst{fl, AstConst::ਦੋਆਧਾਰੀ_ਅੰਕ_True{}});
            cs->ਜੋੜੋ("; ");
        }
        cs->ਜੋੜੋ(callp);
        cs->ਜੋੜੋ("; }");
        ਵਾਪਸ cs;
    }

    ਵਰਤੋਂ CrossSelection = std::ਸਦਿਸ਼<uint64_t>;
    ਬਣਤਰ ResolvedCrossBin final {
        AstCoverCrossBin* binp;
        CrossSelection selection;
    };
    ਬਣਤਰ CrossLayout final {
        uint32_t tuples = 0;
        uint32_t autoBins = 0;
        uint64_t bin_ਸ਼ਬਦ = 0;
        ਤਾਰਕਿਕ valid = ਸੱਚ;
        std::ਸਦਿਸ਼<ResolvedCrossBin> bins;
    };
    ਬਣਤਰ CrossSelectionContext final {
        AstCoverCross* crossp;  // Cross whose tuple space is being selected
        ਅਚਲ std::ਸਦਿਸ਼<AstVar*>& cpVars;  // Feeding coverpoints in dimension order
        ਅਚਲ std::map<std::string, uint32_t>& dimensions;  // Coverpoint name -> dimension
        uint32_t tuples;  // Size of the Cartesian product
        std::ਸਦਿਸ਼<uint32_t> strides;  // Flat-index stride per dimension
        ਤਾਰਕਿਕ valid = ਸੱਚ;  // False if this explicit bin cannot be implemented
    };
    ਬਣਤਰ Cross_Binsof_ਨਿਸ਼ਾਨਾ final {
        ਅਚਲ CoverpointBins* m_binsp = ਖਾਲੀ_ਸੂਚਕ;  // Declared bins; null after a reported error
        uint32_t m_dimension = 0;  // Coverpoint index within the cross
        uint32_t m_ਪਹਿਲਾ = 0;  // First declared normal bin in the selected span
        uint32_t m_ਗਿਣਤੀ = 0;  // Number of declared normal bins in the selected span
        uint32_t m_declared_ਪਹਿਲਾ = 0;  // First runtime bin index of the selected span
        uint32_t m_declared_ਸਿਰਾ = UINT32_MAX;  // One past the last runtime bin index
        int32_t m_sized = -1;  // Index of the sized array holding the selected bins; or -1
    };
    ਬਣਤਰ Cross_ਮੁੱਲ_Range final {
        V3_ਸੰਖਿਆ lo;  // Inclusive lower bound, sign-extended to the comparison width
        V3_ਸੰਖਿਆ hi;  // Inclusive upper bound
        V3_ਸੰਖਿਆ ਨਮੂਨਾ_2;  // Allowed bit values; all X for an ordinary interval
        ਤਾਰਕਿਕ wildcard = ਝੂਠ;  // A wildcard singleton rather than an exact four-state value

        Cross_ਮੁੱਲ_Range(Ast_ਗੰਢ* nodep, ਪੂਰਨ ਚੌੜਾਈ)
            : lo{nodep, ਚੌੜਾਈ}
            , hi{nodep, ਚੌੜਾਈ}
            , ਨਮੂਨਾ_2{nodep, ਚੌੜਾਈ} {
            ਨਮੂਨਾ_2.ਸੈੱਟ_All_ਬਿੱਟ_X();
        }
    };

    ਸਥਿਰ std::ਸਦਿਸ਼<Ast_ਗੰਢ*> cross_Bin_ਮੁੱਲਾਂ(ਅਚਲ Cross_Bin_ਮੁੱਲਾਂ& bin) {
        ਜੇ (bin.valuep) ਵਾਪਸ {bin.valuep};
        std::ਸਦਿਸ਼<Ast_ਗੰਢ*> ਮੁੱਲਾਂ;
        ਜੇ (bin.binp->transp()) {
            // IEEE 1800-2023 19.6.1: binsof uses the last value of each transition.
            ਲਈ (Ast_ਗੰਢ* setp = bin.binp->transp(); setp; setp = setp->nextp()) {
                Ast_Cover_Trans_ਵਸਤੂ* lastp = VN_AS(setp, Cover_Trans_ਸੈੱਟ)->itemsp();
                ਜਦੋਂ (lastp->nextp()) lastp = VN_AS(lastp->nextp(), Cover_Trans_ਵਸਤੂ);
                ਲਈ (Ast_ਗੰਢ* valuep = lastp->valuesp(); valuep; valuep = valuep->nextp()) {
                    ਮੁੱਲਾਂ.ਧੱਕੋ_ਪਿੱਠ(valuep);
                }
            }
        } ਨਹੀਂਤਾਂ {
            ਲਈ (Ast_ਗੰਢ* valuep = bin.binp->rangesp(); valuep; valuep = valuep->nextp()) {
                ਮੁੱਲਾਂ.ਧੱਕੋ_ਪਿੱਠ(valuep);
            }
        }
        ਵਾਪਸ ਮੁੱਲਾਂ;
    }

    // The values of the element-th bin of a run, sign-extended to 'width'
    ਸਥਿਰ Cross_ਮੁੱਲ_Range ਲੜੀਵਾਰ_Bin_Range(Ast_ਗੰਢ* nodep, ਅਚਲ Bin_ਲੜੀਵਾਰ& ਲੜੀਵਾਰ, uint32_t element,
                                       ਪੂਰਨ ਚੌੜਾਈ) {
        ਅਚਲ ਪੂਰਨ runw = ਲੜੀਵਾਰ.m_lo.ਚੌੜਾਈ();
        V3_ਸੰਖਿਆ ਵਿੱਥ{nodep, runw};
        ਵਿੱਥ.opMul(ਲੜੀਵਾਰ.m_stride, V3_ਸੰਖਿਆ{nodep, runw, element});
        V3_ਸੰਖਿਆ lo{nodep, runw};
        lo.op_ਜੋੜੋ(ਲੜੀਵਾਰ.m_lo, ਵਿੱਥ);
        V3_ਸੰਖਿਆ hi = ਲੜੀਵਾਰ.m_hi;
        ਜੇ (element + 1 < ਲੜੀਵਾਰ.m_ਗਿਣਤੀ) {
            ਵਿੱਥ.opSub(ਲੜੀਵਾਰ.m_stride, V3_ਸੰਖਿਆ{nodep, runw, 1});
            hi.op_ਜੋੜੋ(lo, ਵਿੱਥ);
        }
        Cross_ਮੁੱਲ_Range range{nodep, ਚੌੜਾਈ};
        range.lo.opExtendS(lo, runw);
        range.hi.opExtendS(hi, runw);
        ਵਾਪਸ range;
    }

    ਸਥਿਰ ਪੂਰਨ cross_Range_ਚੌੜਾਈ(Ast_ਗੰਢ* nodep) {
        ਜੇ (ਅਚਲ AstInsideRange* ਅਚਲ rangep = VN_CAST(nodep, InsideRange)) {
            ਵਾਪਸ std::max(rangep->lhsp()->ਚੌੜਾਈ(), rangep->rhsp()->ਚੌੜਾਈ());
        }
        ਵਾਪਸ nodep->ਚੌੜਾਈ();
    }

    ਸਥਿਰ ਤਾਰਕਿਕ cross_ਮੁੱਲ_Less(ਅਚਲ V3_ਸੰਖਿਆ& lhs, ਅਚਲ V3_ਸੰਖਿਆ& rhs) {
        V3_ਸੰਖਿਆ ਨਤੀਜਾ{&lhs};
        ਵਾਪਸ !ਨਤੀਜਾ.opLtS(lhs, rhs).isEqZero();
    }

    // Round a real bound inward to the nearest coverpoint value (IEEE 1800-2023 19.5.7).  Sets
    // 'empty' if the domain has no value on the bound's side.
    ਸਥਿਰ ਖਾਲੀ crossRealBound(ਅਚਲ AstConst* constp, Ast_ਗੰਢ_ਵਿਅੰਜਕ* exprp, ਤਾਰਕਿਕ ਉੱਤਲਾ,
                               ਅਚਲ Cross_ਮੁੱਲ_Range& domain, V3_ਸੰਖਿਆ& ਨਤੀਜਾ, ਤਾਰਕਿਕ& ਖਾਲੀ_ਥਾਵਾਂ) {
        ਅਚਲ ਦੁੱਗਣਾ ਮੁੱਲ = constp->ਅੰਕ().ਮੰਜ਼ਿਲ_Double();
        ਅਚਲ ਦੁੱਗਣਾ bound = ਉੱਤਲਾ ? std::floor(ਮੁੱਲ) : std::ceil(ਮੁੱਲ);
        // Powers of two are exact, so the integral bound compares exactly with the domain edges.
        ਅਚਲ ਦੁੱਗਣਾ ਹੱਦਬੰਦੀ
            = std::ldexp(1.0, exprp->isSigned() ? exprp->ਚੌੜਾਈ() - 1 : exprp->ਚੌੜਾਈ());
        ਅਚਲ ਦੁੱਗਣਾ minimum = exprp->isSigned() ? -ਹੱਦਬੰਦੀ : 0.0;
        ਜੇ (std::isnan(bound) || (ਉੱਤਲਾ ? bound < minimum : bound >= ਹੱਦਬੰਦੀ)) {
            ਖਾਲੀ_ਥਾਵਾਂ = ਸੱਚ;
            ਨਤੀਜਾ = domain.lo;
        } ਨਹੀਂਤਾਂ ਜੇ (bound < minimum) {
            ਨਤੀਜਾ = domain.lo;
        } ਨਹੀਂਤਾਂ ਜੇ (bound >= ਹੱਦਬੰਦੀ) {
            ਨਤੀਜਾ = domain.hi;
        } ਨਹੀਂਤਾਂ {
            V3_ਸੰਖਿਆ real{&ਨਤੀਜਾ, 64};
            real.ਸੈੱਟ_Double(bound);
            ਨਤੀਜਾ.opRToIRoundS(real);
        }
    }

    ਸਥਿਰ ਤਾਰਕਿਕ crossRangeBound(Ast_ਗੰਢ* nodep, Ast_ਗੰਢ_ਵਿਅੰਜਕ* exprp, ਤਾਰਕਿਕ ਉੱਤਲਾ, ਤਾਰਕਿਕ bin_ਮੁੱਲ,
                                ਅਚਲ Cross_ਮੁੱਲ_Range& domain, V3_ਸੰਖਿਆ& ਨਤੀਜਾ, ਤਾਰਕਿਕ& ਖਾਲੀ_ਥਾਵਾਂ) {
        ਜੇ (VN_IS(nodep, Unbounded)) {
            V3_ਸੰਖਿਆ ਹੱਦਬੰਦੀ{nodep, exprp->ਚੌੜਾਈ()};
            ਜੇ (ਉੱਤਲਾ) ਹੱਦਬੰਦੀ.ਸੈੱਟ_All_Bits1();
            ਜੇ (exprp->isSigned()) {
                ਹੱਦਬੰਦੀ.ਸੈੱਟ_ਦੋਆਧਾਰੀ_ਅੰਕ(exprp->ਚੌੜਾਈ() - 1, !ਉੱਤਲਾ);
                ਨਤੀਜਾ.opExtendS(ਹੱਦਬੰਦੀ, ਹੱਦਬੰਦੀ.ਚੌੜਾਈ());
            } ਨਹੀਂਤਾਂ {
                ਨਤੀਜਾ.op_ਸੌਂਪੋ(ਹੱਦਬੰਦੀ);
            }
            ਵਾਪਸ ਸੱਚ;
        }
        ਅਚਲ AstConst* ਅਚਲ constp = VN_CAST(nodep, Const);
        ਜੇ (!constp) ਵਾਪਸ ਝੂਠ;
        ਜੇ (constp->ਅੰਕ().isDouble()) {
            crossRealBound(constp, exprp, ਉੱਤਲਾ, domain, ਨਤੀਜਾ, ਖਾਲੀ_ਥਾਵਾਂ);
            ਵਾਪਸ ਸੱਚ;
        }
        ਜੇ (constp->ਅੰਕ().isString()) ਵਾਪਸ ਝੂਠ;
        ਜੇ (bin_ਮੁੱਲ && exprp->isSigned() && constp->ਚੌੜਾਈ() <= exprp->ਚੌੜਾਈ()) {
            // Bin bit patterns use the coverpoint's effective type (IEEE 1800-2023 19.5.7).
            // Wider values and intersect filters retain their values for domain clipping.
            V3_ਸੰਖਿਆ ਮੁੱਲ{nodep, exprp->ਚੌੜਾਈ()};
            ਜੇ (constp->isSigned()) {
                ਮੁੱਲ.opExtendS(constp->ਅੰਕ(), constp->ਚੌੜਾਈ());
            } ਨਹੀਂਤਾਂ {
                ਮੁੱਲ.op_ਸੌਂਪੋ(constp->ਅੰਕ());
            }
            ਨਤੀਜਾ.opExtendS(ਮੁੱਲ, ਮੁੱਲ.ਚੌੜਾਈ());
        } ਨਹੀਂਤਾਂ ਜੇ (constp->isSigned()) {
            ਨਤੀਜਾ.opExtendS(constp->ਅੰਕ(), constp->ਚੌੜਾਈ());
        } ਨਹੀਂਤਾਂ {
            ਨਤੀਜਾ.op_ਸੌਂਪੋ(constp->ਅੰਕ());
        }
        ਵਾਪਸ ਸੱਚ;
    }

    ਸਥਿਰ Cross_ਮੁੱਲ_Range cross_ਮੁੱਲ_Domain(Ast_ਗੰਢ* nodep, ਪੂਰਨ ਮੁੱਲ_ਚੌੜਾਈ, ਤਾਰਕਿਕ isSigned,
                                            ਪੂਰਨ ਚੌੜਾਈ) {
        Cross_ਮੁੱਲ_Range domain{nodep, ਚੌੜਾਈ};
        V3_ਸੰਖਿਆ lo{nodep, ਮੁੱਲ_ਚੌੜਾਈ};
        V3_ਸੰਖਿਆ hi{nodep, ਮੁੱਲ_ਚੌੜਾਈ};
        hi.ਸੈੱਟ_All_Bits1();
        ਜੇ (isSigned) {
            lo.ਸੈੱਟ_ਦੋਆਧਾਰੀ_ਅੰਕ(ਮੁੱਲ_ਚੌੜਾਈ - 1, 1);
            hi.ਸੈੱਟ_ਦੋਆਧਾਰੀ_ਅੰਕ(ਮੁੱਲ_ਚੌੜਾਈ - 1, 0);
            domain.lo.opExtendS(lo, ਮੁੱਲ_ਚੌੜਾਈ);
            domain.hi.opExtendS(hi, ਮੁੱਲ_ਚੌੜਾਈ);
        } ਨਹੀਂਤਾਂ {
            domain.lo.op_ਸੌਂਪੋ(lo);
            domain.hi.op_ਸੌਂਪੋ(hi);
        }
        ਵਾਪਸ domain;
    }

    ਸਥਿਰ ਖਾਲੀ intersectCrossRange(Cross_ਮੁੱਲ_Range& range, ਅਚਲ Cross_ਮੁੱਲ_Range& other) {
        ਜੇ (cross_ਮੁੱਲ_Less(range.lo, other.lo)) range.lo = other.lo;
        ਜੇ (cross_ਮੁੱਲ_Less(other.hi, range.hi)) range.hi = other.hi;
    }

    ਸਥਿਰ ਤਾਰਕਿਕ cross_ਮੁੱਲ_Range(Ast_ਗੰਢ* nodep, Ast_ਗੰਢ_ਵਿਅੰਜਕ* exprp, ਤਾਰਕਿਕ bin_ਮੁੱਲ, ਤਾਰਕਿਕ wildcard,
                                ਅਚਲ Cross_ਮੁੱਲ_Range& domain, Cross_ਮੁੱਲ_Range& range) {
        ਤਾਰਕਿਕ ਖਾਲੀ_ਥਾਵਾਂ = ਝੂਠ;
        ਅਚਲ AstConst* ਅਚਲ constp = VN_CAST(nodep, Const);
        ਜੇ (ਅਚਲ AstInsideRange* ਅਚਲ rangep = VN_CAST(nodep, InsideRange)) {
            ਜੇ (!crossRangeBound(rangep->lhsp(), exprp, ਝੂਠ, bin_ਮੁੱਲ, domain, range.lo, ਖਾਲੀ_ਥਾਵਾਂ)
                || !crossRangeBound(rangep->rhsp(), exprp, ਸੱਚ, bin_ਮੁੱਲ, domain, range.hi, ਖਾਲੀ_ਥਾਵਾਂ)
                || range.lo.isFourState() || range.hi.isFourState()) {
                ਵਾਪਸ ਝੂਠ;
            }
        } ਨਹੀਂਤਾਂ ਜੇ (constp && constp->ਅੰਕ().isDouble()) {
            // A real value participates only if integral; it has no wildcard bits.
            crossRealBound(constp, exprp, ਝੂਠ, domain, range.lo, ਖਾਲੀ_ਥਾਵਾਂ);
            crossRealBound(constp, exprp, ਸੱਚ, domain, range.hi, ਖਾਲੀ_ਥਾਵਾਂ);
        } ਨਹੀਂਤਾਂ {
            ਜੇ (!crossRangeBound(nodep, exprp, ਝੂਠ, bin_ਮੁੱਲ, domain, range.lo, ਖਾਲੀ_ਥਾਵਾਂ)) {
                ਵਾਪਸ ਝੂਠ;
            }
            ਜੇ (bin_ਮੁੱਲ && exprp->isSigned() && constp && !constp->isSigned()
                && constp->ਚੌੜਾਈ() > exprp->ਚੌੜਾਈ()) {
                ਤਾਰਕਿਕ representable = ਸੱਚ;
                ਲਈ (ਪੂਰਨ ਦੋਆਧਾਰੀ_ਅੰਕ_2 = exprp->ਚੌੜਾਈ(); ਦੋਆਧਾਰੀ_ਅੰਕ_2 < constp->ਚੌੜਾਈ(); ++ਦੋਆਧਾਰੀ_ਅੰਕ_2) {
                    representable &= !constp->ਅੰਕ().ਦੋਆਧਾਰੀ_ਅੰਕ_Is1(ਦੋਆਧਾਰੀ_ਅੰਕ_2);
                }
                ਜੇ (representable) {
                    V3_ਸੰਖਿਆ ਮੁੱਲ{nodep, exprp->ਚੌੜਾਈ()};
                    ਮੁੱਲ.op_ਸੌਂਪੋ(constp->ਅੰਕ());
                    range.lo.opExtendS(ਮੁੱਲ, ਮੁੱਲ.ਚੌੜਾਈ());
                }
            }
            range.hi = range.lo;
            range.wildcard = wildcard;
            ਜੇ (wildcard) {
                range.ਨਮੂਨਾ_2 = range.lo;
                range.lo = domain.lo;
                range.hi = domain.hi;
                ਜੇ (constp && constp->isSigned()) {
                    // Replicated X sign bits are correlated, not independent wildcards.
                    // The source domain preserves expansion-before-casting (19.5.7).
                    intersectCrossRange(
                        range, cross_ਮੁੱਲ_Domain(nodep, constp->ਚੌੜਾਈ(), ਸੱਚ, domain.lo.ਚੌੜਾਈ()));
                }
            }
        }
        ਜੇ (ਖਾਲੀ_ਥਾਵਾਂ) {
            range.lo = domain.hi;
            range.hi = domain.lo;
            ਵਾਪਸ ਸੱਚ;
        }
        ਜੇ (!range.lo.isFourState()) intersectCrossRange(range, domain);
        ਜੇ (range.wildcard && exprp->isSigned()) {
            ਅਚਲ ਪੂਰਨ sign = exprp->ਚੌੜਾਈ() - 1;
            ਜੇ (constp && !constp->isSigned() && constp->ਚੌੜਾਈ() > exprp->ਚੌੜਾਈ()) {
                // Unsigned equality preserves every target bit pattern if discarded bits are zero.
                ਲਈ (ਪੂਰਨ ਦੋਆਧਾਰੀ_ਅੰਕ_2 = exprp->ਚੌੜਾਈ(); ਦੋਆਧਾਰੀ_ਅੰਕ_2 < constp->ਚੌੜਾਈ(); ++ਦੋਆਧਾਰੀ_ਅੰਕ_2) {
                    ਜੇ (constp->ਅੰਕ().ਦੋਆਧਾਰੀ_ਅੰਕ_Is1(ਦੋਆਧਾਰੀ_ਅੰਕ_2)) {
                        range.lo = domain.hi;
                        range.hi = domain.lo;
                        ਵਾਪਸ ਸੱਚ;
                    }
                }
                ਲਈ (ਪੂਰਨ ਦੋਆਧਾਰੀ_ਅੰਕ_2 = exprp->ਚੌੜਾਈ(); ਦੋਆਧਾਰੀ_ਅੰਕ_2 < range.ਨਮੂਨਾ_2.ਚੌੜਾਈ(); ++ਦੋਆਧਾਰੀ_ਅੰਕ_2) {
                    ਜੇ (range.ਨਮੂਨਾ_2.ਦੋਆਧਾਰੀ_ਅੰਕ_Is_XZ(sign))
                        range.ਨਮੂਨਾ_2.ਸੈੱਟ_ਦੋਆਧਾਰੀ_ਅੰਕ(ਦੋਆਧਾਰੀ_ਅੰਕ_2, 'x');
                    ਨਹੀਂਤਾਂ
                        range.ਨਮੂਨਾ_2.ਸੈੱਟ_ਦੋਆਧਾਰੀ_ਅੰਕ(ਦੋਆਧਾਰੀ_ਅੰਕ_2, range.ਨਮੂਨਾ_2.ਦੋਆਧਾਰੀ_ਅੰਕ_Is1(sign));
                }
                ਵਾਪਸ ਸੱਚ;
            }
            // Discarded high bits constrain the target sign, rather than becoming don't-cares.
            ਲਈ (ਪੂਰਨ ਦੋਆਧਾਰੀ_ਅੰਕ_2 = exprp->ਚੌੜਾਈ(); ਦੋਆਧਾਰੀ_ਅੰਕ_2 < range.ਨਮੂਨਾ_2.ਚੌੜਾਈ(); ++ਦੋਆਧਾਰੀ_ਅੰਕ_2) {
                ਜੇ (range.ਨਮੂਨਾ_2.ਦੋਆਧਾਰੀ_ਅੰਕ_Is_XZ(ਦੋਆਧਾਰੀ_ਅੰਕ_2)) ਜਾਰੀ;
                ਅਚਲ ਤਾਰਕਿਕ ਮੁੱਲ = range.ਨਮੂਨਾ_2.ਦੋਆਧਾਰੀ_ਅੰਕ_Is1(ਦੋਆਧਾਰੀ_ਅੰਕ_2);
                ਜੇ (!range.ਨਮੂਨਾ_2.ਦੋਆਧਾਰੀ_ਅੰਕ_Is_XZ(sign) && range.ਨਮੂਨਾ_2.ਦੋਆਧਾਰੀ_ਅੰਕ_Is1(sign) != ਮੁੱਲ) {
                    range.lo = domain.hi;
                    range.hi = domain.lo;
                    ਰੋਕੋ;
                }
                range.ਨਮੂਨਾ_2.ਸੈੱਟ_ਦੋਆਧਾਰੀ_ਅੰਕ(sign, ਮੁੱਲ);
            }
        }
        ਵਾਪਸ ਸੱਚ;
    }

    ਗਿਣਤੀ CrossRangeState : uint8_t {
        CROSS_INSIDE_BOUNDS = 0,  // Prefix is strictly inside the interval
        CROSS_AT_ਹੇਠਲਾ = 1,
        CROSS_AT_ਉੱਤਲਾ = 2,
        CROSS_AT_BOUNDS = CROSS_AT_ਹੇਠਲਾ | CROSS_AT_ਉੱਤਲਾ,
        CROSS_NO_ਮੇਲ = 4  // Prefix cannot match the interval/pattern
    };

    ਸਥਿਰ CrossRangeState cross_Range_ਕਦਮ(ਅਚਲ Cross_ਮੁੱਲ_Range& range, CrossRangeState state,
                                          ਪੂਰਨ ਦੋਆਧਾਰੀ_ਅੰਕ_2, ਪੂਰਨ ਮੁੱਲ) {
        ਜੇ (state == CROSS_NO_ਮੇਲ) ਵਾਪਸ CROSS_NO_ਮੇਲ;
        // Flipping the sign bit makes signed order lexicographic.
        ਅਚਲ ਤਾਰਕਿਕ sign = ਦੋਆਧਾਰੀ_ਅੰਕ_2 == range.ਨਮੂਨਾ_2.ਚੌੜਾਈ() - 1;
        ਜੇ (!range.ਨਮੂਨਾ_2.ਦੋਆਧਾਰੀ_ਅੰਕ_Is_XZ(ਦੋਆਧਾਰੀ_ਅੰਕ_2) && ਮੁੱਲ != (range.ਨਮੂਨਾ_2.ਦੋਆਧਾਰੀ_ਅੰਕ_Is1(ਦੋਆਧਾਰੀ_ਅੰਕ_2) ^ sign)) {
            ਵਾਪਸ CROSS_NO_ਮੇਲ;
        }
        ਅਚਲ ਪੂਰਨ ਨੀਵਾਂ = range.lo.ਦੋਆਧਾਰੀ_ਅੰਕ_Is1(ਦੋਆਧਾਰੀ_ਅੰਕ_2) ^ sign;
        ਅਚਲ ਪੂਰਨ ਉੱਚਾ = range.hi.ਦੋਆਧਾਰੀ_ਅੰਕ_Is1(ਦੋਆਧਾਰੀ_ਅੰਕ_2) ^ sign;
        ਜੇ (((state & CROSS_AT_ਹੇਠਲਾ) && ਮੁੱਲ < ਨੀਵਾਂ)
            || ((state & CROSS_AT_ਉੱਤਲਾ) && ਮੁੱਲ > ਉੱਚਾ)) {
            ਵਾਪਸ CROSS_NO_ਮੇਲ;
        }
        ਵਾਪਸ static_cast<CrossRangeState>(
            ((state & CROSS_AT_ਹੇਠਲਾ) && ਮੁੱਲ == ਨੀਵਾਂ ? CROSS_AT_ਹੇਠਲਾ : CROSS_INSIDE_BOUNDS)
            | ((state & CROSS_AT_ਉੱਤਲਾ) && ਮੁੱਲ == ਉੱਚਾ ? CROSS_AT_ਉੱਤਲਾ : CROSS_INSIDE_BOUNDS));
    }

    ਸਥਿਰ ਤਾਰਕਿਕ crossWildcardIntersects(ਅਚਲ Cross_ਮੁੱਲ_Range& range) {
        ਬਿਨਾਨਿਸ਼ਾਨੀ ਹਾਲਤਾਂ = 1U << CROSS_AT_BOUNDS;
        ਲਈ (ਪੂਰਨ ਦੋਆਧਾਰੀ_ਅੰਕ_2 = range.ਨਮੂਨਾ_2.ਚੌੜਾਈ() - 1; ਦੋਆਧਾਰੀ_ਅੰਕ_2 >= 0 && ਹਾਲਤਾਂ; --ਦੋਆਧਾਰੀ_ਅੰਕ_2) {
            ਬਿਨਾਨਿਸ਼ਾਨੀ ਅਗਲਾ = 0;
            ਲਈ (ਅਚਲ CrossRangeState state :
                 {CROSS_INSIDE_BOUNDS, CROSS_AT_ਹੇਠਲਾ, CROSS_AT_ਉੱਤਲਾ, CROSS_AT_BOUNDS}) {
                ਜੇ (!(ਹਾਲਤਾਂ & (1U << state))) ਜਾਰੀ;
                ਲਈ (ਪੂਰਨ ਮੁੱਲ = 0; ਮੁੱਲ < 2; ++ਮੁੱਲ) {
                    ਅਚਲ CrossRangeState equal = cross_Range_ਕਦਮ(range, state, ਦੋਆਧਾਰੀ_ਅੰਕ_2, ਮੁੱਲ);
                    ਜੇ (equal != CROSS_NO_ਮੇਲ) ਅਗਲਾ |= 1U << equal;
                }
            }
            ਹਾਲਤਾਂ = ਅਗਲਾ;
        }
        ਵਾਪਸ ਹਾਲਤਾਂ != 0;
    }

    // The least value at least 'from' with the non-x bits of 'pattern', in unsigned order; false
    // if none.  A bit scan, which does not enumerate the pattern's x bits.
    ਸਥਿਰ ਤਾਰਕਿਕ ਅਗਲਾ_ਨਮੂਨਾ_ਮੁੱਲ(ਅਚਲ V3_ਸੰਖਿਆ& ਨਮੂਨਾ_2, ਅਚਲ V3_ਸੰਖਿਆ& from, V3_ਸੰਖਿਆ& ਨਤੀਜਾ) {
        ਨਤੀਜਾ.op_ਸੌਂਪੋ(from);
        ਪੂਰਨ ਹਾਸਲ = -1;  // Lowest x bit above the bit scanned where 'from' has a 0
        ਲਈ (ਪੂਰਨ ਦੋਆਧਾਰੀ_ਅੰਕ_2 = from.ਚੌੜਾਈ() - 1; ਦੋਆਧਾਰੀ_ਅੰਕ_2 >= 0; --ਦੋਆਧਾਰੀ_ਅੰਕ_2) {
            ਜੇ (ਨਮੂਨਾ_2.ਦੋਆਧਾਰੀ_ਅੰਕ_Is_XZ(ਦੋਆਧਾਰੀ_ਅੰਕ_2)) {
                ਜੇ (!from.ਦੋਆਧਾਰੀ_ਅੰਕ_Is1(ਦੋਆਧਾਰੀ_ਅੰਕ_2)) ਹਾਸਲ = ਦੋਆਧਾਰੀ_ਅੰਕ_2;
                ਜਾਰੀ;
            }
            ਜੇ (ਨਮੂਨਾ_2.ਦੋਆਧਾਰੀ_ਅੰਕ_Is1(ਦੋਆਧਾਰੀ_ਅੰਕ_2) == from.ਦੋਆਧਾਰੀ_ਅੰਕ_Is1(ਦੋਆਧਾਰੀ_ਅੰਕ_2)) ਜਾਰੀ;
            ਜੇ (from.ਦੋਆਧਾਰੀ_ਅੰਕ_Is1(ਦੋਆਧਾਰੀ_ਅੰਕ_2)) {  // Only a greater prefix, an x bit above raised, can match
                ਜੇ (ਹਾਸਲ < 0) ਵਾਪਸ ਝੂਠ;
                ਦੋਆਧਾਰੀ_ਅੰਕ_2 = ਹਾਸਲ;
            }
            // Then the least such value: the pattern's bits, with x bits of zero
            ਨਤੀਜਾ.ਸੈੱਟ_ਦੋਆਧਾਰੀ_ਅੰਕ(ਦੋਆਧਾਰੀ_ਅੰਕ_2, 1);
            ਜਦੋਂ (--ਦੋਆਧਾਰੀ_ਅੰਕ_2 >= 0) ਨਤੀਜਾ.ਸੈੱਟ_ਦੋਆਧਾਰੀ_ਅੰਕ(ਦੋਆਧਾਰੀ_ਅੰਕ_2, ਨਮੂਨਾ_2.ਦੋਆਧਾਰੀ_ਅੰਕ_Is1(ਦੋਆਧਾਰੀ_ਅੰਕ_2));
            ਵਾਪਸ ਸੱਚ;
        }
        ਵਾਪਸ ਸੱਚ;
    }

    // Append to 'runs' the maximal runs of consecutive values of 'range' that its pattern
    // matches, in value order, until there are more than 'limit'.  A match continues through
    // the pattern's trailing x bits only, which the next value's carry leaves.
    ਸਥਿਰ ਖਾਲੀ crossRangeRuns(ਅਚਲ Cross_ਮੁੱਲ_Range& range, ਮਾਪ_t ਹੱਦਬੰਦੀ,
                               std::ਸਦਿਸ਼<std::pair<V3_ਸੰਖਿਆ, V3_ਸੰਖਿਆ>>& runs) {
        // Values order signed, which is the unsigned order of values with the sign bit flipped
        ਅਚਲ ਪੂਰਨ sign = range.lo.ਚੌੜਾਈ() - 1;
        ਅਚਲ ਸਵੈਚਾਲਿਤ flipped = [sign](V3_ਸੰਖਿਆ ਮੁੱਲ) {
            ਜੇ (!ਮੁੱਲ.ਦੋਆਧਾਰੀ_ਅੰਕ_Is_XZ(sign)) ਮੁੱਲ.ਸੈੱਟ_ਦੋਆਧਾਰੀ_ਅੰਕ(sign, !ਮੁੱਲ.ਦੋਆਧਾਰੀ_ਅੰਕ_Is1(sign));
            ਵਾਪਸ ਮੁੱਲ;
        };
        ਅਚਲ V3_ਸੰਖਿਆ ਨਮੂਨਾ_2 = flipped(range.ਨਮੂਨਾ_2);
        ਅਚਲ V3_ਸੰਖਿਆ hi = flipped(range.hi);
        ਅਚਲ V3_ਸੰਖਿਆ one{&hi, hi.ਚੌੜਾਈ(), 1};
        V3_ਸੰਖਿਆ trailing{&hi, hi.ਚੌੜਾਈ()};
        ਲਈ (ਪੂਰਨ ਦੋਆਧਾਰੀ_ਅੰਕ_2 = 0; ਦੋਆਧਾਰੀ_ਅੰਕ_2 <= sign && ਨਮੂਨਾ_2.ਦੋਆਧਾਰੀ_ਅੰਕ_Is_XZ(ਦੋਆਧਾਰੀ_ਅੰਕ_2); ++ਦੋਆਧਾਰੀ_ਅੰਕ_2) trailing.ਸੈੱਟ_ਦੋਆਧਾਰੀ_ਅੰਕ(ਦੋਆਧਾਰੀ_ਅੰਕ_2, 1);
        V3_ਸੰਖਿਆ from = flipped(range.lo);
        V3_ਸੰਖਿਆ ਪਹਿਲਾ{&hi, hi.ਚੌੜਾਈ()};
        V3_ਸੰਖਿਆ ਆਖਰੀ{&hi, hi.ਚੌੜਾਈ()};
        V3_ਸੰਖਿਆ less{&hi};
        ਜਦੋਂ (runs.ਮਾਪ() <= ਹੱਦਬੰਦੀ && ਅਗਲਾ_ਨਮੂਨਾ_ਮੁੱਲ(ਨਮੂਨਾ_2, from, ਪਹਿਲਾ)
               && less.opLt(hi, ਪਹਿਲਾ).isEqZero()) {
            ਆਖਰੀ.opOr(ਪਹਿਲਾ, trailing);
            ਜੇ (!less.opLt(hi, ਆਖਰੀ).isEqZero()) ਆਖਰੀ = hi;
            runs.emplace_ਪਿੱਠ(flipped(ਪਹਿਲਾ), flipped(ਆਖਰੀ));
            ਜੇ (ਆਖਰੀ.isCaseEq(hi)) ਰੋਕੋ;
            from.op_ਜੋੜੋ(ਆਖਰੀ, one);
        }
    }

    // True if no coverpoint value participates in a resolved value or range.  A value with x
    // or z bits participates only as a wildcard pattern (IEEE 1800-2023 19.5.7).
    ਸਥਿਰ ਤਾਰਕਿਕ cross_Range_ਖਾਲੀ_ਥਾਵਾਂ(ਅਚਲ Cross_ਮੁੱਲ_Range& range) {
        ਜੇ (range.lo.isFourState()) ਵਾਪਸ ਸੱਚ;
        ਵਾਪਸ cross_ਮੁੱਲ_Less(range.hi, range.lo)
               || (range.wildcard && !crossWildcardIntersects(range));
    }

    // Comparison width that holds both the coverpoint's and a bin or intersect value's range.
    ਸਥਿਰ ਪੂਰਨ resolve_ਚੌੜਾਈ(Ast_ਗੰਢ* nodep, ਅਚਲ Ast_ਗੰਢ_ਵਿਅੰਜਕ* exprp) {
        ਵਾਪਸ std::max(exprp->ਚੌੜਾਈ(), cross_Range_ਚੌੜਾਈ(nodep)) + 1;
    }

    // Resolve a bin or intersect value to coverpoint values (IEEE 1800-2023 19.5.7), in the
    // width 'range' was created with.  False if the value is not a constant integral or real.
    ਸਥਿਰ ਤਾਰਕਿਕ resolve_ਮੁੱਲ(Ast_ਗੰਢ* nodep, Ast_ਗੰਢ_ਵਿਅੰਜਕ* exprp, ਤਾਰਕਿਕ bin_ਮੁੱਲ, ਤਾਰਕਿਕ wildcard,
                             Cross_ਮੁੱਲ_Range& range) {
        ਅਚਲ Cross_ਮੁੱਲ_Range domain
            = cross_ਮੁੱਲ_Domain(nodep, exprp->ਚੌੜਾਈ(), exprp->isSigned(), range.lo.ਚੌੜਾਈ());
        ਵਾਪਸ cross_ਮੁੱਲ_Range(nodep, exprp, bin_ਮੁੱਲ, wildcard, domain, range);
    }

    ਸਥਿਰ ਤਾਰਕਿਕ crossRangesIntersect(ਅਚਲ Cross_ਮੁੱਲ_Range& bin, ਅਚਲ Cross_ਮੁੱਲ_Range& filter) {
        // Values with x or z bits do not participate, even in an identical filter.
        ਜੇ (bin.lo.isFourState() || filter.lo.isFourState()) ਵਾਪਸ ਝੂਠ;
        Cross_ਮੁੱਲ_Range ਮੇਲ = bin;
        intersectCrossRange(ਮੇਲ, filter);
        ਵਾਪਸ !cross_ਮੁੱਲ_Less(ਮੇਲ.hi, ਮੇਲ.lo)
               && (!bin.wildcard || crossWildcardIntersects(ਮੇਲ));
    }

    ਸਥਿਰ ਖਾਲੀ unsupportedCrossRange(AstCoverBinsof* selectp, ਤਾਰਕਿਕ& valid) {
        selectp->v3warn(COVERIGN, "Unsupported: non-constant or non-integral 'intersect' value, "
                                  "or four-state range bound.");
        valid = ਝੂਠ;
    }

    ਸਥਿਰ ਤਾਰਕਿਕ cross_ਮੁੱਲ_Matches_Filters(AstCoverBinsof* selectp, Ast_ਗੰਢ* valuep,
                                         Ast_ਗੰਢ_ਵਿਅੰਜਕ* exprp, ਅਚਲ AstCoverBin* binp,
                                         ਅਚਲ Cross_ਮੁੱਲ_Range& domain,
                                         ਅਚਲ std::ਸਦਿਸ਼<Cross_ਮੁੱਲ_Range>& filters,
                                         ਤਾਰਕਿਕ& valid) {
        Cross_ਮੁੱਲ_Range range{valuep, domain.lo.ਚੌੜਾਈ()};
        ਜੇ (!cross_ਮੁੱਲ_Range(valuep, exprp, ਸੱਚ, binp->isWildcard(), domain, range)) {
            unsupportedCrossRange(selectp, valid);
            ਵਾਪਸ ਝੂਠ;
        }
        ਲਈ (ਅਚਲ Cross_ਮੁੱਲ_Range& filter : filters) {
            ਜੇ (crossRangesIntersect(range, filter)) ਵਾਪਸ ਸੱਚ;
        }
        ਵਾਪਸ ਝੂਠ;
    }

    std::ਸਦਿਸ਼<ਤਾਰਕਿਕ> selectCoverpointBins(AstCoverBinsof* selectp, ਅਚਲ CoverpointBins& bins,
                                           uint32_t ਪਹਿਲਾ, uint32_t ਗਿਣਤੀ_2, ਤਾਰਕਿਕ& valid) {
        ਜੇ (selectp->rangesp() && !bins.exprp->dtypep()->skipRefp()->isIntegralOrPacked()) {
            unsupportedCrossRange(selectp, valid);
            ਵਾਪਸ {};
        }
        std::ਸਦਿਸ਼<ਤਾਰਕਿਕ> selected(bins.ਕੁੱਲ, ਝੂਠ);
        std::ਸਦਿਸ਼<std::ਸਦਿਸ਼<Ast_ਗੰਢ*>> ਮੁੱਲਾਂ;
        ਪੂਰਨ ਚੌੜਾਈ = bins.exprp->ਚੌੜਾਈ();
        ਲਈ (Ast_ਗੰਢ* rangep = selectp->rangesp(); rangep; rangep = rangep->nextp()) {
            ਚੌੜਾਈ = std::max(ਚੌੜਾਈ, cross_Range_ਚੌੜਾਈ(rangep));
        }
        ਜੇ (selectp->rangesp()) {
            ਮੁੱਲਾਂ.reserve(ਗਿਣਤੀ_2);
            ਲਈ (uint32_t i = ਪਹਿਲਾ; i < ਪਹਿਲਾ + ਗਿਣਤੀ_2; ++i) {
                // A run's values are in the coverpoint type, whose width 'width' covers
                ਮੁੱਲਾਂ.ਧੱਕੋ_ਪਿੱਠ(bins.ਮੁੱਲਾਂ[i].runp ? std::ਸਦਿਸ਼<Ast_ਗੰਢ*>{}
                                                     : cross_Bin_ਮੁੱਲਾਂ(bins.ਮੁੱਲਾਂ[i]));
                ਲਈ (Ast_ਗੰਢ* ਅਚਲ valuep : ਮੁੱਲਾਂ.ਪਿੱਠ()) {
                    ਚੌੜਾਈ = std::max(ਚੌੜਾਈ, cross_Range_ਚੌੜਾਈ(valuep));
                }
            }
        }
        // One extra bit preserves both unsigned maxima and negative signed bounds.
        ++ਚੌੜਾਈ;
        ਅਚਲ Cross_ਮੁੱਲ_Range domain
            = cross_ਮੁੱਲ_Domain(selectp, bins.exprp->ਚੌੜਾਈ(), bins.exprp->isSigned(), ਚੌੜਾਈ);
        std::ਸਦਿਸ਼<Cross_ਮੁੱਲ_Range> filters;
        ਲਈ (Ast_ਗੰਢ* rangep = selectp->rangesp(); rangep; rangep = rangep->nextp()) {
            Cross_ਮੁੱਲ_Range filter{rangep, ਚੌੜਾਈ};
            ਜੇ (!cross_ਮੁੱਲ_Range(rangep, bins.exprp, ਝੂਠ, ਝੂਠ, domain, filter)) {
                unsupportedCrossRange(selectp, valid);
                ਵਾਪਸ {};
            }
            filters.ਧੱਕੋ_ਪਿੱਠ(std::move(filter));
        }
        ਲਈ (uint32_t i = ਪਹਿਲਾ; i < ਪਹਿਲਾ + ਗਿਣਤੀ_2; ++i) {
            ਜੇ (!selectp->rangesp()) {
                selected[i] = ਸੱਚ;
                ਜਾਰੀ;
            }
            ਜੇ (ਅਚਲ Bin_ਲੜੀਵਾਰ* ਅਚਲ runp = bins.ਮੁੱਲਾਂ[i].runp) {
                ਅਚਲ Cross_ਮੁੱਲ_Range range
                    = ਲੜੀਵਾਰ_Bin_Range(selectp, *runp, bins.ਮੁੱਲਾਂ[i].element, ਚੌੜਾਈ);
                selected[i] = std::any_ਦਾ(filters.begin(), filters.ਸਿਰਾ(),
                                          [&](ਅਚਲ Cross_ਮੁੱਲ_Range& filter) {
                                              ਵਾਪਸ crossRangesIntersect(range, filter);
                                          });
                ਜਾਰੀ;
            }
            ਲਈ (Ast_ਗੰਢ* ਅਚਲ valuep : ਮੁੱਲਾਂ[i - ਪਹਿਲਾ]) {
                selected[i] = cross_ਮੁੱਲ_Matches_Filters(
                    selectp, valuep, bins.exprp, bins.ਮੁੱਲਾਂ[i].binp, domain, filters, valid);
                ਜੇ (!valid) ਵਾਪਸ {};
                ਜੇ (selected[i]) ਰੋਕੋ;
            }
        }
        ਜੇ (selectp->isNegated()) {
            ਲਈ (uint32_t i = 0; i < bins.ਕੁੱਲ; ++i) selected[i] = !selected[i];
        }
        ਵਾਪਸ selected;
    }

    // Constructor-time value metadata of one coverpoint, as C++ list entries
    ਬਣਤਰ ਮੁੱਲ_Lists final {
        std::ਸਦਿਸ਼<std::string> m_ranges;  // Bin, then low and high words
        std::ਸਦਿਸ਼<std::string> m_runs;  // First bin, count, then low, span, and high words
        std::ਸਦਿਸ਼<std::string> m_patterns;  // Bin, then value, mask, low, and high words
        std::ਸਦਿਸ਼<std::string> m_transitions;  // Transition bin
    };

    // Append a value's words, in the coverpoint's width, to a C++ list entry.
    ਸਥਿਰ ਖਾਲੀ append_ਸ਼ਬਦ(std::string& ਪਾਠ, ਅਚਲ V3_ਸੰਖਿਆ& ਮੁੱਲ, ਅਚਲ Ast_ਗੰਢ_ਵਿਅੰਜਕ* exprp) {
        V3_ਸੰਖਿਆ narrowed{&ਮੁੱਲ, exprp->ਚੌੜਾਈ()};
        narrowed.op_ਸੌਂਪੋ(ਮੁੱਲ);
        ਲਈ (ਪੂਰਨ ਸ਼ਬਦ = 0; ਸ਼ਬਦ < exprp->ਚੌੜਾਈ_ਸ਼ਬਦ(); ++ਸ਼ਬਦ) {
            ਪਾਠ += ", " + cvt_ਮੰਜ਼ਿਲ_Str(narrowed.edata_ਸ਼ਬਦ(ਸ਼ਬਦ)) + "U";
        }
    }

    ਖਾਲੀ collect_ਮੁੱਲ_Metadata(ਮੁੱਲ_Lists& lists, Ast_ਗੰਢ_ਵਿਅੰਜਕ* exprp, AstCoverBin* binp,
                              uint32_t ਸੂਚਕਾਂਕ, Ast_ਗੰਢ_ਵਿਅੰਜਕ* valuep) {
        ਅਚਲ std::string bin = cvt_ਮੰਜ਼ਿਲ_Str(ਸੂਚਕਾਂਕ) + "U";
        ਜੇ (binp->transp()) lists.m_transitions.ਧੱਕੋ_ਪਿੱਠ(bin);
        ਲਈ (Ast_ਗੰਢ* ਅਚਲ sourcep : cross_Bin_ਮੁੱਲਾਂ({binp, valuep})) {
            Cross_ਮੁੱਲ_Range range{sourcep, resolve_ਚੌੜਾਈ(sourcep, exprp)};
            ਜੇ (!resolve_ਮੁੱਲ(sourcep, exprp, ਸੱਚ, binp->isWildcard(), range)) {
                // Sampling already resolved every state bin value, so only transitions remain.
                sourcep->v3warn(E_UNSUPPORTED, "Unsupported: non-integral value in a transition "
                                               "bin of a coverpoint with exclusions.");
                ਜਾਰੀ;
            }
            ਜੇ (cross_Range_ਖਾਲੀ_ਥਾਵਾਂ(range)) ਜਾਰੀ;
            std::string ਇੰਦਰਾਜ = bin;
            ਜੇ (range.wildcard) {
                V3_ਸੰਖਿਆ ਮੁੱਲ{sourcep, exprp->ਚੌੜਾਈ()};
                V3_ਸੰਖਿਆ ਨਕਾਬ{sourcep, exprp->ਚੌੜਾਈ()};
                ਨਕਾਬ.op_ਬਿੱਟ_Non_XZ(range.ਨਮੂਨਾ_2);
                ਮੁੱਲ.op_ਬਿੱਟ_One(range.ਨਮੂਨਾ_2);
                append_ਸ਼ਬਦ(ਇੰਦਰਾਜ, ਮੁੱਲ, exprp);
                append_ਸ਼ਬਦ(ਇੰਦਰਾਜ, ਨਕਾਬ, exprp);
            }
            append_ਸ਼ਬਦ(ਇੰਦਰਾਜ, range.lo, exprp);
            append_ਸ਼ਬਦ(ਇੰਦਰਾਜ, range.hi, exprp);
            (range.wildcard ? lists.m_patterns : lists.m_ranges).ਧੱਕੋ_ਪਿੱਠ(ਇੰਦਰਾਜ);
        }
    }

    // Describe a run with one entry, from which the runtime computes the values of its bins.
    ਸਥਿਰ ਖਾਲੀ collect_ਲੜੀਵਾਰ_Metadata(ਮੁੱਲ_Lists& lists, Ast_ਗੰਢ_ਵਿਅੰਜਕ* exprp, ਅਚਲ Bin_ਲੜੀਵਾਰ& ਲੜੀਵਾਰ) {
        V3_ਸੰਖਿਆ ਵਿਸਤਾਰ{exprp, ਲੜੀਵਾਰ.m_stride.ਚੌੜਾਈ(), 0};
        ਵਿਸਤਾਰ.opSub(ਲੜੀਵਾਰ.m_stride, V3_ਸੰਖਿਆ{exprp, ਲੜੀਵਾਰ.m_stride.ਚੌੜਾਈ(), 1});
        std::string ਇੰਦਰਾਜ = cvt_ਮੰਜ਼ਿਲ_Str(ਲੜੀਵਾਰ.m_declared) + "U, " + cvt_ਮੰਜ਼ਿਲ_Str(ਲੜੀਵਾਰ.m_ਗਿਣਤੀ) + "U";
        append_ਸ਼ਬਦ(ਇੰਦਰਾਜ, ਲੜੀਵਾਰ.m_lo, exprp);
        append_ਸ਼ਬਦ(ਇੰਦਰਾਜ, ਵਿਸਤਾਰ, exprp);
        append_ਸ਼ਬਦ(ਇੰਦਰਾਜ, ਲੜੀਵਾਰ.m_hi, exprp);
        lists.m_runs.ਧੱਕੋ_ਪਿੱਠ(ਇੰਦਰਾਜ);
    }

    // Emit one batched metadata list, bounding the size of each call's temporary list.
    ਖਾਲੀ emit_ਮੁੱਲ_List(File_ਸਤਰ* fl, AstVar* cpVarp, VCMethod ਵਿਧੀ,
                       ਅਚਲ std::ਸਦਿਸ਼<std::string>& entries) {
        ਲਈ (ਮਾਪ_t ਪਹਿਲਾ = 0; ਪਹਿਲਾ < entries.ਮਾਪ(); ਪਹਿਲਾ += ਮੁੱਲ_LIST_ENTRIES) {
            ਅਚਲ ਮਾਪ_t ਸਿਰਾ = std::min(entries.ਮਾਪ(), ਪਹਿਲਾ + ਮੁੱਲ_LIST_ENTRIES);
            std::string ਪਾਠ = "{" + entries[ਪਹਿਲਾ];
            ਲਈ (ਮਾਪ_t i = ਪਹਿਲਾ + 1; i < ਸਿਰਾ; ++i) ਪਾਠ += ", " + entries[i];
            m_constructorp->ਜੋੜੋ_Stmtsp(
                ਵਸਤੂ_Call(fl, cpVarp, ਵਿਧੀ, {ctext(fl, ਪਾਠ + "}")})->makeStmt());
        }
    }

    ਸਥਿਰ ਤਾਰਕਿਕ checkCrossRef(ਅਚਲ AstCoverCrossRef* refp, ਅਚਲ AstCoverCross* crossp) {
        ਜੇ (refp->name() == crossp->name()) ਵਾਪਸ ਸੱਚ;
        refp->v3error("Cross selection "
                      << refp->prettyNameQ() << " may only name its enclosing cross "
                      << crossp->prettyNameQ() << " (IEEE 1800-2023 19.6.1.2).");
        ਵਾਪਸ ਝੂਠ;
    }

    ਸਥਿਰ ਤਾਰਕਿਕ checkCrossBinName(ਅਚਲ AstCoverCrossBin* binp, std::ਸੈੱਟ<std::string>& ਨਾਂ) {
        ਜੇ (ਨਾਂ.emplace(binp->name()).ਦੂਜਾ) ਵਾਪਸ ਸੱਚ;
        binp->v3error("Duplicate cross bin " << binp->prettyNameQ()
                                             << " (IEEE 1800-2023 19.6.1).");
        ਵਾਪਸ ਝੂਠ;
    }

    // The span of the implicit automatic bin reported as 'name' ('auto_<i>', see
    // createImplicitAutoBins), found without naming each of its bins
    ਸਥਿਰ ਤਾਰਕਿਕ implicit_Auto_Bin_ਵਿਸਤਾਰ(ਅਚਲ CoverpointBins& bins, ਅਚਲ std::string& name,
                                    Bin_ਵਿਸਤਾਰ& ਵਿਸਤਾਰ) {
        ਅਚਲ std::string ਅਗੇਤਰ = "auto_";
        ਜੇ (!VString::startsWith(name, ਅਗੇਤਰ)) ਵਾਪਸ ਝੂਠ;
        ਅਚਲ std::string digits = name.substr(ਅਗੇਤਰ.ਮਾਪ());
        ਅਚਲ ਬਿਨਾਨਿਸ਼ਾਨੀ ਲੰਮਾ ਸੂਚਕਾਂਕ = std::strtoul(digits.c_str(), ਖਾਲੀ_ਸੂਚਕ, 10);
        // Only the reported spelling names the bin, not e.g. 'auto_01' or 'auto_x'
        ਜੇ (ਸੂਚਕਾਂਕ >= bins.implicitAuto.ਗਿਣਤੀ_2 || digits != cvt_ਮੰਜ਼ਿਲ_Str(ਸੂਚਕਾਂਕ)) ਵਾਪਸ ਝੂਠ;
        ਅਚਲ uint32_t ਵਿੱਥ = static_cast<uint32_t>(ਸੂਚਕਾਂਕ);
        ਵਿਸਤਾਰ = Bin_ਵਿਸਤਾਰ{bins.implicitAuto.ਪਹਿਲਾ + ਵਿੱਥ, 1, bins.implicitAuto.declared + ਵਿੱਥ};
        ਵਾਪਸ ਸੱਚ;
    }

    Cross_Binsof_ਨਿਸ਼ਾਨਾ
    resolve_Binsof_ਨਿਸ਼ਾਨਾ(ਅਚਲ AstCoverBinsof* selectp, ਅਚਲ AstCoverCross* crossp,
                        ਅਚਲ std::ਸਦਿਸ਼<AstVar*>& cpVars,
                        ਅਚਲ std::map<std::string, uint32_t>& dimensions) ਅਚਲ {
        ਅਚਲ ਸਵੈਚਾਲਿਤ dim = dimensions.ਲੱਭੋ(selectp->pointp()->name());
        ਜੇ (dim == dimensions.ਸਿਰਾ()) {
            selectp->v3error("binsof coverpoint "
                             << selectp->pointp()->prettyNameQ() << " is not an item of cross "
                             << crossp->prettyNameQ() << " (IEEE 1800-2023 19.6.1).");
            ਵਾਪਸ {};
        }
        ਅਚਲ CoverpointBins& bins = m_cpBins.at(cpVars[dim->ਦੂਜਾ]);
        Cross_Binsof_ਨਿਸ਼ਾਨਾ ਨਿਸ਼ਾਨਾ{&bins, dim->ਦੂਜਾ, 0, bins.ਕੁੱਲ};
        ਜੇ (!selectp->name().ਖਾਲੀ_ਥਾਵਾਂ()) {
            ਅਚਲ ਸਵੈਚਾਲਿਤ bin = bins.spans.ਲੱਭੋ(selectp->name());
            Bin_ਵਿਸਤਾਰ ਵਿਸਤਾਰ{0, 0, 0};
            ਜੇ (bin != bins.spans.ਸਿਰਾ()) {
                ਵਿਸਤਾਰ = bin->ਦੂਜਾ;
            } ਨਹੀਂਤਾਂ ਜੇ (!implicit_Auto_Bin_ਵਿਸਤਾਰ(bins, selectp->name(), ਵਿਸਤਾਰ)) {
                selectp->v3error("Cannot find bin " << selectp->prettyNameQ() << " in coverpoint "
                                                    << selectp->pointp()->prettyNameQ()
                                                    << " (IEEE 1800-2023 19.6.1).");
                ਵਾਪਸ {};
            }
            ਨਿਸ਼ਾਨਾ.m_ਪਹਿਲਾ = ਵਿਸਤਾਰ.ਪਹਿਲਾ;
            ਨਿਸ਼ਾਨਾ.m_ਗਿਣਤੀ = ਵਿਸਤਾਰ.ਗਿਣਤੀ_2;
            ਨਿਸ਼ਾਨਾ.m_declared_ਪਹਿਲਾ = ਵਿਸਤਾਰ.declared;
            ਨਿਸ਼ਾਨਾ.m_declared_ਸਿਰਾ = ਵਿਸਤਾਰ.declared + ਵਿਸਤਾਰ.ਗਿਣਤੀ_2;
            ਨਿਸ਼ਾਨਾ.m_sized = ਵਿਸਤਾਰ.sized;
        }
        ਵਾਪਸ ਨਿਸ਼ਾਨਾ;
    }

    // The variables a 'with' filter reads, each by its first reference
    ਵਰਤੋਂ CrossFilterReads = std::map<ਅਚਲ AstVar*, AstVarRef*>;

    // Whether a 'with' filter has a count of value tuples, of 'matches' other than '$'
    ਸਥਿਰ ਤਾਰਕਿਕ counts_ਮੁੱਲ_Tuples(ਅਚਲ AstCoverCrossWith* withp) {
        ਵਾਪਸ withp->countp() && !VN_IS(withp->countp(), Unbounded);
    }

    // Whether the runtime knows the values of the cross items a 'with' filter reads, or to count
    // value tuples, of all items, of integral coverpoints; else warns
    ਸਥਿਰ ਤਾਰਕਿਕ crossFilterSupported(ਅਚਲ AstCoverCrossWith* withp,
                                     ਅਚਲ CrossFilterReads& reads) {
        ਲਈ (ਅਚਲ AstVar* varp = withp->varsp(); varp; varp = VN_AS(varp->nextp(), Var)) {
            ਜੇ (varp->dtypep()->skipRefp()->isIntegralOrPacked()) ਜਾਰੀ;
            ਅਚਲ ਸਵੈਚਾਲਿਤ it = reads.ਲੱਭੋ(varp);
            ਜੇ (it != reads.ਸਿਰਾ()) {
                it->ਦੂਜਾ->v3warn(COVERIGN, "Unsupported: cross bin 'with' filter reading the "
                                             "value of non-integral coverpoint "
                                                 << it->ਦੂਜਾ->prettyNameQ());
                ਵਾਪਸ ਝੂਠ;
            }
            ਜੇ (counts_ਮੁੱਲ_Tuples(withp)) {
                withp->countp()->v3warn(COVERIGN, "Unsupported: cross bin 'matches' count of "
                                                  "value tuples of non-integral coverpoint "
                                                      << varp->prettyNameQ());
                ਵਾਪਸ ਝੂਠ;
            }
        }
        ਵਾਪਸ ਸੱਚ;
    }

    // Emit the constructor code selecting, of the bin tuples of the selection, those with enough
    // value tuples for which the 'with' filter is true: one, all for 'matches $', or a count
    // (IEEE 1800-2023 19.6.1.2).  The runtime gives the values of the cross items the filter
    // reads, of each value tuple it may evaluate:
    //   withBegin({dimensions}, count, positive, bin, file, line);
    //   while (withNext()) {
    //       item = withValue(dimension);  // Of each cross item read
    //       withTest(filter);
    //   }
    //   withEnd();
    // The dimensions are those of the items read, or to count value tuples, of all items: the
    // filter's result for a value tuple of the items it reads repeats for each of the others'.
    ਖਾਲੀ generateCrossFilter(ਅਚਲ AstCoverCrossWith* withp, ਅਚਲ CrossFilterReads& reads,
                             ਅਚਲ AstCoverCrossBin* binp, ਅਚਲ AstCoverCross* crossp,
                             AstVar* cxp, ਅਚਲ std::map<string, uint32_t>& dimensions,
                             uint32_t ਚੀਜ਼ਾਂ) {
        File_ਸਤਰ* ਅਚਲ fl = withp->fileline();
        ਅਚਲ string ਅਗੇਤਰ
            = m_sized_ਨਾਂ.get(sanitizeGeneratedName(crossp->name() + "__" + binp->name()));
        ਅਚਲ ਤਾਰਕਿਕ counting = counts_ਮੁੱਲ_Tuples(withp);
        // The count, 0 for all
        ਅਚਲ std::pair<Ast_ਗੰਢ_ਵਿਅੰਜਕ*, Ast_ਗੰਢ_ਵਿਅੰਜਕ*> ਗਿਣਤੀ_2
            = counting ? new_ਗਿਣਤੀ_ਦਲੀਲਾਂ(fl, ਅਗੇਤਰ + "_count", withp->countp())
                       : new_ਗਿਣਤੀ_ਦਲੀਲਾਂ(fl, withp->countp() ? 0 : 1);
        AstLoop* ਅਚਲ loopp = ਨਵਾਂ AstLoop{fl};
        AstCMethodHard* ਅਚਲ nextp = ਵਸਤੂ_Call(fl, cxp, VCMethod::COVERGROUP_WITH_ਅਗਲਾ);
        nextp->dtype_ਸੈੱਟ_ਦੋਆਧਾਰੀ_ਅੰਕ();
        loopp->ਜੋੜੋ_Stmtsp(ਨਵਾਂ AstLoopTest{fl, loopp, nextp});
        string dims;
        ਲਈ (uint32_t dim = 0; counting && dim < ਚੀਜ਼ਾਂ; ++dim) {
            dims += (dims.ਖਾਲੀ_ਥਾਵਾਂ() ? "" : ", ") + cvt_ਮੰਜ਼ਿਲ_Str(dim) + "U";
        }
        ਜਦੋਂ (AstVar* ਅਚਲ varp = withp->varsp()) {
            varp->unlink_Fr_ਪਿੱਠ();
            ਜੇ (!reads.ਗਿਣਤੀ_2(varp)) {
                VL_DO_DANGLING(ਧੱਕੋ_Deletep(varp), varp);
                ਜਾਰੀ;
            }
            ਅਚਲ ਸਵੈਚਾਲਿਤ dimIt = dimensions.ਲੱਭੋ(varp->name());
            UASSERT_OBJ(dimIt != dimensions.ਸਿਰਾ(), varp, "Cross 'with' candidate not of an item");
            ਅਚਲ uint32_t dim = dimIt->ਦੂਜਾ;
            ਜੇ (!counting) dims += (dims.ਖਾਲੀ_ਥਾਵਾਂ() ? "" : ", ") + cvt_ਮੰਜ਼ਿਲ_Str(dim) + "U";
            varp->name(ਅਗੇਤਰ + "_" + sanitizeGeneratedName(varp->name()));
            m_constructorp->ਜੋੜੋ_Stmtsp(varp);
            ਜੇ (varp->isWide()) {
                loopp->ਜੋੜੋ_Stmtsp(ਵਸਤੂ_Call(fl, cxp, VCMethod::COVERGROUP_CROSS_WITH_ਮੁੱਲ_W,
                                          {cnum(fl, dim), ਨਵਾਂ AstVarRef{fl, varp, VAccess::WRITE}})
                                     ->makeStmt());
            } ਨਹੀਂਤਾਂ {
                AstCMethodHard* ਅਚਲ valuep
                    = ਵਸਤੂ_Call(fl, cxp, VCMethod::COVERGROUP_CROSS_WITH_ਮੁੱਲ, {cnum(fl, dim)});
                valuep->dtype_ਸੈੱਟ_UInt64();
                Ast_ਗੰਢ_DType* ਅਚਲ dtypep = varp->ਲੱਭੋ_Logic_DType(
                    varp->ਚੌੜਾਈ(), varp->ਚੌੜਾਈ(),
                    varp->isSigned() ? VSigning::SIGNED : VSigning::UNSIGNED);
                loopp->ਜੋੜੋ_Stmtsp(ਨਵਾਂ Ast_ਸੌਂਪੋ{fl, ਨਵਾਂ AstVarRef{fl, varp, VAccess::WRITE},
                                               resize_ਮੁੱਲ(valuep, dtypep)});
            }
        }
        loopp->ਜੋੜੋ_Stmtsp(ਵਸਤੂ_Call(fl, cxp, VCMethod::COVERGROUP_CROSS_WITH_TEST,
                                  {withp->filterp()->unlink_Fr_ਪਿੱਠ()})
                             ->makeStmt());
        ਅਚਲ ਤਾਰਕਿਕ prot = v3Global.opt.protectIds();
        m_constructorp->ਜੋੜੋ_Stmtsp(
            ਵਸਤੂ_Call(fl, cxp, VCMethod::COVERGROUP_CROSS_WITH_BEGIN,
                     {ctext(fl, "{" + dims + "}"), ਗਿਣਤੀ_2.ਪਹਿਲਾ, ਗਿਣਤੀ_2.ਦੂਜਾ,
                      ctext(fl, quoted(VIdProtect::protect_ਸ਼ਬਦ_If(binp->name(), prot))),
                      ctext(fl, quoted(VIdProtect::protectIf(fl->filename(), prot))),
                      cnum(fl, static_cast<uint32_t>(fl->lineno()))})
                ->makeStmt());
        m_constructorp->ਜੋੜੋ_Stmtsp(loopp);
        m_constructorp->ਜੋੜੋ_Stmtsp(
            ਵਸਤੂ_Call(fl, cxp, VCMethod::COVERGROUP_CROSS_WITH_ਸਿਰਾ)->makeStmt());
    }

    ਤਾਰਕਿਕ generateRuntimeSelection(Ast_ਗੰਢ* nodep, ਅਚਲ AstCoverCrossBin* binp,
                                  AstCoverCross* crossp, AstVar* cxp,
                                  ਅਚਲ std::ਸਦਿਸ਼<AstVar*>& cpVars,
                                  ਅਚਲ std::map<string, uint32_t>& dimensions) {
        File_ਸਤਰ* ਅਚਲ fl = nodep->fileline();
        ਜੇ (ਅਚਲ AstCoverCrossRef* ਅਚਲ refp = VN_CAST(nodep, CoverCrossRef)) {
            ਜੇ (!checkCrossRef(refp, crossp)) ਵਾਪਸ ਝੂਠ;
            m_constructorp->ਜੋੜੋ_Stmtsp(
                ਵਸਤੂ_Call(fl, cxp, VCMethod::COVERGROUP_SELECT_ALL)->makeStmt());
            ਵਾਪਸ ਸੱਚ;
        }
        ਜੇ (ਅਚਲ AstCoverCrossSelect* ਅਚਲ opp = VN_CAST(nodep, CoverCrossSelect)) {
            ਜੇ (!generateRuntimeSelection(opp->lhsp(), binp, crossp, cxp, cpVars, dimensions)
                || !generateRuntimeSelection(opp->rhsp(), binp, crossp, cxp, cpVars, dimensions)) {
                ਵਾਪਸ ਝੂਠ;
            }
            m_constructorp->ਜੋੜੋ_Stmtsp(ਵਸਤੂ_Call(fl, cxp,
                                               opp->isOr() ? VCMethod::COVERGROUP_SELECT_OR
                                                           : VCMethod::COVERGROUP_SELECT_AND)
                                          ->makeStmt());
            ਵਾਪਸ ਸੱਚ;
        }
        ਜੇ (ਅਚਲ AstCoverCrossWith* ਅਚਲ withp = VN_CAST(nodep, CoverCrossWith)) {
            CrossFilterReads reads;
            withp->filterp()->foreach([&](AstVarRef* refp) { reads.emplace(refp->varp(), refp); });
            // The count of 'matches' is a positive integer, or '$' (IEEE 1800-2023 19.6.1.2)
            ਜੇ ((counts_ਮੁੱਲ_Tuples(withp)
                 && !check_ਗਿਣਤੀ(withp->countp(), "Cross bin 'matches' count", "19.6.1.2"))
                || !crossFilterSupported(withp, reads)
                || !generateRuntimeSelection(withp->selectp(), binp, crossp, cxp, cpVars,
                                             dimensions)) {
                ਵਾਪਸ ਝੂਠ;
            }
            generateCrossFilter(withp, reads, binp, crossp, cxp, dimensions,
                                static_cast<uint32_t>(cpVars.ਮਾਪ()));
            ਵਾਪਸ ਸੱਚ;
        }
        AstCoverBinsof* ਅਚਲ selectp = VN_AS(nodep, CoverBinsof);
        ਅਚਲ Cross_Binsof_ਨਿਸ਼ਾਨਾ ਨਿਸ਼ਾਨਾ = resolve_Binsof_ਨਿਸ਼ਾਨਾ(selectp, crossp, cpVars, dimensions);
        ਜੇ (!ਨਿਸ਼ਾਨਾ.m_binsp) ਵਾਪਸ ਝੂਠ;
        ਅਚਲ CoverpointBins& bins = *ਨਿਸ਼ਾਨਾ.m_binsp;
        ਜੇ (selectp->rangesp() && !bins.exprp->dtypep()->skipRefp()->isIntegralOrPacked()) {
            ਤਾਰਕਿਕ valid = ਸੱਚ;
            unsupportedCrossRange(selectp, valid);
            ਵਾਪਸ ਝੂਠ;
        }
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* firstp;
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* endp;
        ਜੇ (ਨਿਸ਼ਾਨਾ.m_sized < 0) {
            firstp = cnum(fl, ਨਿਸ਼ਾਨਾ.m_declared_ਪਹਿਲਾ);
            endp = cnum(fl, ਨਿਸ਼ਾਨਾ.m_declared_ਸਿਰਾ);
        } ਨਹੀਂਤਾਂ {  // A sized array, whose bins the coverpoint's construction placed
            AstVar* ਅਚਲ cpVarp = cpVars[ਨਿਸ਼ਾਨਾ.m_dimension];
            ਅਚਲ uint32_t sized = static_cast<uint32_t>(ਨਿਸ਼ਾਨਾ.m_sized);
            firstp = ਵਸਤੂ_Call(fl, cpVarp, VCMethod::COVERGROUP_SIZED_ਪਹਿਲਾ, {cnum(fl, sized)});
            endp = ਵਸਤੂ_Call(fl, cpVarp, VCMethod::COVERGROUP_SIZED_ਸਿਰਾ, {cnum(fl, sized)});
            firstp->dtype_ਸੈੱਟ_UInt32();
            endp->dtype_ਸੈੱਟ_UInt32();
        }
        m_constructorp->ਜੋੜੋ_Stmtsp(
            ਵਸਤੂ_Call(fl, cxp, VCMethod::COVERGROUP_SELECT_DIM,
                     {cnum(fl, ਨਿਸ਼ਾਨਾ.m_dimension), firstp, endp, cnum(fl, selectp->isNegated()),
                      cnum(fl, selectp->rangesp() != ਖਾਲੀ_ਸੂਚਕ)})
                ->makeStmt());
        ਲਈ (Ast_ਗੰਢ* rangep = selectp->rangesp(); rangep; rangep = rangep->nextp()) {
            Cross_ਮੁੱਲ_Range range{rangep, resolve_ਚੌੜਾਈ(rangep, bins.exprp)};
            ਜੇ (!resolve_ਮੁੱਲ(rangep, bins.exprp, ਝੂਠ, ਝੂਠ, range)) {
                ਤਾਰਕਿਕ valid = ਸੱਚ;
                unsupportedCrossRange(selectp, valid);
                ਵਾਪਸ ਝੂਠ;
            }
            ਜੇ (cross_Range_ਖਾਲੀ_ਥਾਵਾਂ(range)) ਜਾਰੀ;
            m_constructorp->ਜੋੜੋ_Stmtsp(ਵਸਤੂ_Call(fl, cxp,
                                               bins.exprp->isWide()
                                                   ? VCMethod::COVERGROUP_SELECT_RANGE_W
                                                   : VCMethod::COVERGROUP_SELECT_RANGE,
                                               {new_ਮੁੱਲ_Const(fl, range.lo, bins.exprp),
                                                new_ਮੁੱਲ_Const(fl, range.hi, bins.exprp)})
                                          ->makeStmt());
        }
        m_constructorp->ਜੋੜੋ_Stmtsp(
            ਵਸਤੂ_Call(fl, cxp, VCMethod::COVERGROUP_SELECT_DIM_ਸਿਰਾ)->makeStmt());
        ਵਾਪਸ ਸੱਚ;
    }

    std::ਸਦਿਸ਼<AstCoverCrossBin*>
    generateRuntimeCrossBins(AstCoverCross* crossp, AstVar* cxp,
                             ਅਚਲ std::ਸਦਿਸ਼<AstVar*>& cpVars,
                             ਅਚਲ std::map<string, uint32_t>& dimensions) {
        std::ਸਦਿਸ਼<AstCoverCrossBin*> bins;
        std::ਸੈੱਟ<string> ਨਾਂ;
        ਲਈ (Ast_ਗੰਢ* itemp = crossp->binsp(); itemp; itemp = itemp->nextp()) {
            AstCoverCrossBin* ਅਚਲ binp = VN_AS(itemp, CoverCrossBin);
            ਜੇ (!checkCrossBinName(binp, ਨਾਂ)) ਜਾਰੀ;
            File_ਸਤਰ* ਅਚਲ fl = binp->fileline();
            // Generate the selection apart, so that an unsupported one leaves no code, which
            // could leave its state for the next bin
            AstFunc* ਅਚਲ selectionp = ਨਵਾਂ AstFunc{fl, "", ਖਾਲੀ_ਸੂਚਕ, ਖਾਲੀ_ਸੂਚਕ};
            ਤਾਰਕਿਕ supported;
            {
                VL_RESTORER(m_constructorp);
                m_constructorp = selectionp;
                supported = generateRuntimeSelection(binp->selectp(), binp, crossp, cxp, cpVars,
                                                     dimensions);
            }
            // A supported selection has code
            ਜੇ (supported) m_constructorp->ਜੋੜੋ_Stmtsp(selectionp->stmtsp()->unlink_Fr_ਪਿੱਠ_With_ਅਗਲਾ());
            VL_DO_DANGLING(ਧੱਕੋ_Deletep(selectionp), selectionp);
            ਜੇ (!supported) ਜਾਰੀ;
            ਅਚਲ ਤਾਰਕਿਕ protect = v3Global.opt.protectIds();
            m_constructorp->ਜੋੜੋ_Stmtsp(
                ਵਸਤੂ_Call(fl, cxp, VCMethod::COVERGROUP_SELECT_BIN,
                         {ctext(fl, binp->binsType().bin_ਸੈੱਟ_Enum()),
                          ctext(fl, quoted(VIdProtect::protect_ਸ਼ਬਦ_If(binp->name(), protect))),
                          ctext(fl, quoted(VIdProtect::protectIf(fl->filename(), protect))),
                          cnum(fl, fl->lineno()), cnum(fl, fl->ਪਹਿਲਾ_ਕਾਲਮ()),
                          cnum(fl, static_cast<uint32_t>(bins.ਮਾਪ()))})
                    ->makeStmt());
            bins.ਧੱਕੋ_ਪਿੱਠ(binp);
        }
        m_constructorp->ਜੋੜੋ_Stmtsp(
            ਵਸਤੂ_Call(crossp->fileline(), cxp, VCMethod::COVERGROUP_FINALIZE_BINS)->makeStmt());
        ਵਾਪਸ bins;
    }

    ਸਥਿਰ ਖਾਲੀ ਸੈੱਟ_Cross_Selection_Range(CrossSelection& selection, uint64_t ਪਹਿਲਾ, uint64_t ਸਿਰਾ) {
        ਜਦੋਂ (ਪਹਿਲਾ < ਸਿਰਾ) {
            ਅਚਲ ਬਿਨਾਨਿਸ਼ਾਨੀ ਦੋਆਧਾਰੀ_ਅੰਕ_2 = ਪਹਿਲਾ % 64;
            ਅਚਲ ਬਿਨਾਨਿਸ਼ਾਨੀ ਬਿੱਟ = std::min<uint64_t>(64 - ਦੋਆਧਾਰੀ_ਅੰਕ_2, ਸਿਰਾ - ਪਹਿਲਾ);
            selection[VL_BITWORD_Q(ਪਹਿਲਾ)]
                |= (ਬਿੱਟ == 64 ? ~uint64_t{0} : (uint64_t{1} << ਬਿੱਟ) - 1) << ਦੋਆਧਾਰੀ_ਅੰਕ_2;
            ਪਹਿਲਾ += ਬਿੱਟ;
        }
    }

    CrossSelection crossSelection(Ast_ਗੰਢ* nodep, CrossSelectionContext& ctx) {
        ਜੇ (ਅਚਲ AstCoverCrossRef* ਅਚਲ refp = VN_CAST(nodep, CoverCrossRef)) {
            ਜੇ (!checkCrossRef(refp, ctx.crossp)) {
                ctx.valid = ਝੂਠ;
                ਵਾਪਸ {};
            }
            CrossSelection ਨਤੀਜਾ(
                VL_BITWORD_Q(static_cast<uint64_t>(ctx.tuples) + VL_QUADSIZE - 1), 0);
            ਸੈੱਟ_Cross_Selection_Range(ਨਤੀਜਾ, 0, ctx.tuples);
            ਵਾਪਸ ਨਤੀਜਾ;
        }
        ਜੇ (AstCoverCrossSelect* ਅਚਲ opp = VN_CAST(nodep, CoverCrossSelect)) {
            CrossSelection lhs = crossSelection(opp->lhsp(), ctx);
            ਅਚਲ CrossSelection rhs = crossSelection(opp->rhsp(), ctx);
            ਜੇ (!ctx.valid) ਵਾਪਸ {};
            ਲਈ (ਮਾਪ_t i = 0; i < lhs.ਮਾਪ(); ++i) {
                lhs[i] = opp->isOr() ? lhs[i] | rhs[i] : lhs[i] & rhs[i];
            }
            ਵਾਪਸ lhs;
        }
        AstCoverBinsof* ਅਚਲ selectp = VN_AS(nodep, CoverBinsof);
        ਅਚਲ Cross_Binsof_ਨਿਸ਼ਾਨਾ ਨਿਸ਼ਾਨਾ
            = resolve_Binsof_ਨਿਸ਼ਾਨਾ(selectp, ctx.crossp, ctx.cpVars, ctx.dimensions);
        ਜੇ (!ਨਿਸ਼ਾਨਾ.m_binsp) {
            ctx.valid = ਝੂਠ;
            ਵਾਪਸ {};
        }
        ਅਚਲ CoverpointBins& bins = *ਨਿਸ਼ਾਨਾ.m_binsp;
        // Coverpoints with sized arrays are runtime points, so feed only runtime crosses
        UASSERT_OBJ(ਨਿਸ਼ਾਨਾ.m_sized < 0, selectp, "Sized bin array selected by a static cross");
        ਅਚਲ std::ਸਦਿਸ਼<ਤਾਰਕਿਕ> selected
            = selectCoverpointBins(selectp, bins, ਨਿਸ਼ਾਨਾ.m_ਪਹਿਲਾ, ਨਿਸ਼ਾਨਾ.m_ਗਿਣਤੀ, ctx.valid);
        ਜੇ (!ctx.valid) ਵਾਪਸ {};
        CrossSelection ਨਤੀਜਾ(VL_BITWORD_Q(static_cast<uint64_t>(ctx.tuples) + VL_QUADSIZE - 1),
                              0);
        ਅਚਲ uint64_t stride = ctx.strides[ਨਿਸ਼ਾਨਾ.m_dimension];
        ਅਚਲ uint64_t period = stride * bins.ਕੁੱਲ;
        ਲਈ (uint64_t ਅਧਾਰ = 0; ਅਧਾਰ < ctx.tuples; ਅਧਾਰ += period) {
            ਲਈ (uint32_t i = 0; i < bins.ਕੁੱਲ;) {
                ਜੇ (!selected[i]) {
                    ++i;
                    ਜਾਰੀ;
                }
                ਅਚਲ uint32_t begin = i++;
                ਜਦੋਂ (i < bins.ਕੁੱਲ && selected[i]) ++i;
                ਸੈੱਟ_Cross_Selection_Range(ਨਤੀਜਾ, ਅਧਾਰ + begin * stride, ਅਧਾਰ + i * stride);
            }
        }
        ਵਾਪਸ ਨਤੀਜਾ;
    }

    // Size the Cartesian product of the declared Normal bins, with each dimension's flat-index
    // stride.  Live runtime bins only shrink it.  Warns and returns false if it is too large.
    ਤਾਰਕਿਕ crossShape(AstCoverCross* crossp, ਅਚਲ std::ਸਦਿਸ਼<AstVar*>& cpVars,
                    std::ਸਦਿਸ਼<uint32_t>& strides, uint32_t& tuples) ਅਚਲ {
        uint64_t ਗੁਣਨਫਲ = std::any_ਦਾ(cpVars.begin(), cpVars.ਸਿਰਾ(),
                                       [ਇਹ](AstVar* varp) { ਵਾਪਸ !m_cpBins.at(varp).ਕੁੱਲ; })
                               ? 0
                               : 1;
        strides.resize(cpVars.ਮਾਪ());
        ਲਈ (ਮਾਪ_t d = cpVars.ਮਾਪ(); d > 0; --d) {
            strides[d - 1] = static_cast<uint32_t>(ਗੁਣਨਫਲ);
            ਗੁਣਨਫਲ *= m_cpBins.at(cpVars[d - 1]).ਕੁੱਲ;
            ਜੇ (ਗੁਣਨਫਲ > UINT32_MAX) {
                crossp->v3warn(COVERIGN,
                               "Unsupported: cross coverage with more than 2^32-1 tuples.");
                ਵਾਪਸ ਝੂਠ;
            }
        }
        tuples = static_cast<uint32_t>(ਗੁਣਨਫਲ);
        ਵਾਪਸ ਸੱਚ;
    }

    CrossLayout resolveCrossLayout(AstCoverCross* crossp, ਅਚਲ std::ਸਦਿਸ਼<AstVar*>& cpVars,
                                   ਅਚਲ std::map<std::string, uint32_t>& dimensions) {
        CrossLayout layout;
        CrossSelectionContext ctx{crossp, cpVars, dimensions, 0, {}};
        ਜੇ (!crossShape(crossp, cpVars, ctx.strides, ctx.tuples)) {
            layout.valid = ਝੂਠ;
            ਵਾਪਸ layout;
        }
        layout.tuples = ctx.tuples;
        CrossSelection occupied;
        CrossSelection excluded;
        std::ਸੈੱਟ<std::string> ਨਾਂ;
        ਲਈ (Ast_ਗੰਢ* itemp = crossp->binsp(); itemp; itemp = itemp->nextp()) {
            AstCoverCrossBin* ਅਚਲ binp = VN_AS(itemp, CoverCrossBin);
            ਜੇ (!checkCrossBinName(binp, ਨਾਂ)) ਜਾਰੀ;
            ctx.valid = ਸੱਚ;
            CrossSelection selection = crossSelection(binp->selectp(), ctx);
            ਜੇ (!ctx.valid || std::all_ਦਾ(selection.begin(), selection.ਸਿਰਾ(), [](uint64_t ਸ਼ਬਦ) {
                    ਵਾਪਸ ਸ਼ਬਦ == 0;
                })) {
                ਜਾਰੀ;
            }
            ਜੇ (occupied.ਖਾਲੀ_ਥਾਵਾਂ()) {
                occupied.resize(selection.ਮਾਪ(), 0);
                excluded.resize(selection.ਮਾਪ(), 0);
            }
            ਲਈ (ਮਾਪ_t i = 0; i < selection.ਮਾਪ(); ++i) {
                occupied[i] |= selection[i];
                ਜੇ (!binp->binsType().bin_Is_ਸਾਧਾਰਨ()) excluded[i] |= selection[i];
            }
            layout.bins.ਧੱਕੋ_ਪਿੱਠ({binp, std::move(selection)});
        }
        ਜੇ (!layout.bins.ਖਾਲੀ_ਥਾਵਾਂ()) {
            // IEEE 1800-2023 19.6.2/19.6.3: exclusions also remove tuples from named
            // bins, independently of declaration order and sampling guards.
            ਲਈ (ResolvedCrossBin& resolved : layout.bins) {
                ਲਈ (ਮਾਪ_t i = 0; i < resolved.selection.ਮਾਪ(); ++i) {
                    ਜੇ (resolved.binp->binsType().bin_Is_ਸਾਧਾਰਨ()) {
                        resolved.selection[i] &= ~excluded[i];
                    }
                    ਜੇ (resolved.selection[i]) ++layout.bin_ਸ਼ਬਦ;
                }
            }
            layout.bins.erase(std::remove_if(layout.bins.begin(), layout.bins.ਸਿਰਾ(),
                                             [](ਅਚਲ ResolvedCrossBin& resolved) {
                                                 ਵਾਪਸ std::all_ਦਾ(
                                                     resolved.selection.begin(),
                                                     resolved.selection.ਸਿਰਾ(),
                                                     [](uint64_t ਸ਼ਬਦ) { ਵਾਪਸ ਸ਼ਬਦ == 0; });
                                             }),
                              layout.bins.ਸਿਰਾ());
            layout.autoBins = layout.tuples;
            ਲਈ (ਅਚਲ uint64_t ਸ਼ਬਦ : occupied) {
                layout.autoBins -= static_cast<uint32_t>(std::bitset<VL_QUADSIZE>{ਸ਼ਬਦ}.ਗਿਣਤੀ_2());
            }
        }
        ਵਾਪਸ layout;
    }

    AstCoverCrossDType* crossDType(File_ਸਤਰ* fl, uint32_t dimensions, ਅਚਲ CrossLayout& layout,
                                   ਤਾਰਕਿਕ dynamic = ਝੂਠ) {
        ਅਚਲ uint32_t bins = static_cast<uint32_t>(layout.bins.ਮਾਪ());
        ਅਚਲ CrossShape shape{dimensions,      layout.tuples,   bins,
                               layout.autoBins, layout.bin_ਸ਼ਬਦ, dynamic};
        AstCoverCrossDType*& typep = m_cxDTypes[shape];
        ਜੇ (!typep) {
            typep = ਨਵਾਂ AstCoverCrossDType{
                fl, dimensions, layout.tuples, bins, layout.autoBins, layout.bin_ਸ਼ਬਦ, dynamic};
            v3Global.rootp()->typeTablep()->ਜੋੜੋ_Typesp(typep);
        }
        ਵਾਪਸ typep;
    }

    std::ਸਦਿਸ਼<AstCoverCrossBin*> generateCrossBins(AstCoverCross* crossp, AstVar* cxVarp,
                                                     ਅਚਲ CrossLayout& layout) {
        std::ਸਦਿਸ਼<AstCoverCrossBin*> bins;
        ਲਈ (ਅਚਲ ResolvedCrossBin& resolved : layout.bins) {
            AstCoverCrossBin* ਅਚਲ binp = resolved.binp;
            ਅਚਲ CrossSelection& selection = resolved.selection;
            File_ਸਤਰ* ਅਚਲ fl = binp->fileline();
            ਅਚਲ ਤਾਰਕਿਕ prot = v3Global.opt.protectIds();
            std::string ਨਕਾਬ = "{";
            ਲਈ (ਮਾਪ_t i = 0; i < selection.ਮਾਪ(); ++i) {
                ਜੇ (i) ਨਕਾਬ += ", ";
                ਨਕਾਬ += std::ਮੰਜ਼ਿਲ_string(selection[i]) + "ULL";
            }
            ਨਕਾਬ += "}";
            m_constructorp->ਜੋੜੋ_Stmtsp(
                ਵਸਤੂ_Call(fl, cxVarp, VCMethod::COVERGROUP_ਜੋੜੋ_BIN,
                         {ctext(fl, binp->binsType().bin_ਸੈੱਟ_Enum()), ctext(fl, ਨਕਾਬ),
                          ctext(fl, quoted(VIdProtect::protect_ਸ਼ਬਦ_If(binp->name(), prot))),
                          ctext(fl, quoted(VIdProtect::protectIf(fl->filename(), prot))),
                          cnum(fl, static_cast<uint32_t>(fl->lineno())),
                          cnum(fl, static_cast<uint32_t>(fl->ਪਹਿਲਾ_ਕਾਲਮ()))})
                    ->makeStmt());
            bins.ਧੱਕੋ_ਪਿੱਠ(binp);
        }
        ਜੇ (!bins.ਖਾਲੀ_ਥਾਵਾਂ()) {
            m_constructorp->ਜੋੜੋ_Stmtsp(
                ਵਸਤੂ_Call(crossp->fileline(), cxVarp, VCMethod::COVERGROUP_FINALIZE_BINS)
                    ->makeStmt());
        }
        ਵਾਪਸ bins;
    }

    // Route a cross through a VlCoverCross member: emit the member, its constructor init +
    // registration, and the sample() call.  The feeding coverpoints are already generated
    // (their hit lists drive the cross). Each explicit bin adds one configuration call.
    ਖਾਲੀ generateCross(AstCoverCross* crossp) {
        File_ਸਤਰ* ਅਚਲ fl = crossp->fileline();
        ਅਚਲ ਤਾਰਕਿਕ dynamic = m_runtimeCrosses.ਗਿਣਤੀ_2(crossp);
        UINFO(4, "  Generating VlCoverCross member: " << crossp->name());

        ਜੇ (Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ iffp = crossp->iffp()) {
            crossp->iffp(
                capture_Iff_ਮੰਜ਼ਿਲ_ਆਰਜ਼ੀ(iffp, "__VcrossIff_" + sanitizeGeneratedName(crossp->name())));
        }

        // Resolve and unlink the coverpoint refs, in dimension order.  Every ref resolves to a
        // known coverpoint (a cross with an unresolvable item was dropped earlier).
        std::ਸਦਿਸ਼<AstVar*> cpVars;
        std::map<std::string, uint32_t> dimensions;
        ਲਈ (Ast_ਗੰਢ* itemp = crossp->itemsp(); itemp;) {
            Ast_ਗੰਢ* ਅਚਲ nextp = itemp->nextp();
            AstCoverpointRef* ਅਚਲ refp = VN_AS(itemp, CoverpointRef);
            ਅਚਲ ਸਵੈਚਾਲਿਤ it = m_cpVarMap.ਲੱਭੋ(refp->name());
            UASSERT_OBJ(it != m_cpVarMap.ਸਿਰਾ(), crossp, "Cross references an unknown coverpoint");
            dimensions.emplace(refp->name(), static_cast<uint32_t>(cpVars.ਮਾਪ()));
            cpVars.ਧੱਕੋ_ਪਿੱਠ(it->ਦੂਜਾ);
            VL_DO_DANGLING(ਧੱਕੋ_Deletep(refp->unlink_Fr_ਪਿੱਠ()), refp);
            itemp = nextp;
        }
        ਅਚਲ ਪੂਰਨ dims = static_cast<ਪੂਰਨ>(cpVars.ਮਾਪ());
        CrossLayout layout;
        ਜੇ (dynamic) {
            // The runtime sizes the layout from live bins; only check the declared bound here.
            std::ਸਦਿਸ਼<uint32_t> strides;
            uint32_t tuples = 0;
            layout.valid = crossShape(crossp, cpVars, strides, tuples);
        } ਨਹੀਂਤਾਂ {
            layout = resolveCrossLayout(crossp, cpVars, dimensions);
        }
        ਜੇ (!layout.valid) ਵਾਪਸ;

        AstVar* ਅਚਲ cxVarp
            = ਨਵਾਂ AstVar{fl, VVarType::MEMBER, "__Vcx_" + crossp->name(),
                         crossDType(fl, static_cast<uint32_t>(dims), layout, dynamic)};
        m_covergroupp->ਜੋੜੋ_Membersp(cxVarp);
        m_constructorp->ਜੋੜੋ_Stmtsp(make_ਵਸਤੂ_Create(fl, cxVarp,
                                                 dynamic ? VCMethod::COVERGROUP_ਜੋੜੋ_CROSS_DYN
                                                         : VCMethod::COVERGROUP_ਜੋੜੋ_CROSS));
        generate_ਵਸਤੂ_ਭਾਰ(fl, cxVarp, crossp->optionsp());
        ਪੂਰਨ at_ਘੱਟ_ਤੋਂ_ਘੱਟ_ਮੁੱਲ;
        ਪੂਰਨ autoBinMax;  // Not a cross option
        ਕੱਢੋ_ਵਸਤੂ_Options(crossp->optionsp(), at_ਘੱਟ_ਤੋਂ_ਘੱਟ_ਮੁੱਲ, autoBinMax);

        // Constructor: init (after the coverpoints, which generate earlier) then registration.
        // Obfuscate the hierarchy/filename/page under --protect-ids as for coverpoints above.
        ਅਚਲ ਤਾਰਕਿਕ prot = v3Global.opt.protectIds();
        ਅਚਲ std::string hier
            = VIdProtect::protect_ਸ਼ਬਦ_If(m_covergroupName + "." + crossp->name(), prot);
        m_constructorp->ਜੋੜੋ_Stmtsp(makeCrossCpsCall(
            fl, cpVars,
            ਵਸਤੂ_Call(fl, cxVarp, VCMethod::COVERGROUP_INIT,
                     {ctext(fl, quoted(hier)), cnum(fl, static_cast<uint32_t>(at_ਘੱਟ_ਤੋਂ_ਘੱਟ_ਮੁੱਲ)),
                      cnum(fl, static_cast<uint32_t>(dims)), ctext(fl, "__Vcx_cps"),
                      ctext(fl, quoted(VIdProtect::protectIf(fl->filename(), prot))),
                      cnum(fl, static_cast<uint32_t>(fl->lineno())),
                      cnum(fl, static_cast<uint32_t>(fl->ਪਹਿਲਾ_ਕਾਲਮ()))})));
        ਅਚਲ std::ਸਦਿਸ਼<AstCoverCrossBin*> bins
            = dynamic ? generateRuntimeCrossBins(crossp, cxVarp, cpVars, dimensions)
                      : generateCrossBins(crossp, cxVarp, layout);
        ਜੇ (v3Global.opt.coverage()) {
            ਅਚਲ std::string ਪੰਨਾ
                = VIdProtect::protectIf("v_covergroup/" + m_covergroupName, prot);
            m_constructorp->ਜੋੜੋ_Stmtsp(ਵਸਤੂ_Call(fl, cxVarp, VCMethod::COVERGROUP_ਰਜਿਸਟਰ_BINS,
                                               {ctext(fl, "vlSymsp->_vm_contextp__->coveragep()"),
                                                ctext(fl, quoted(ਪੰਨਾ)),
                                                cnum(fl, ਵਸਤੂ_Database_ਭਾਰ(crossp->optionsp())),
                                                cnum(fl, m_cg_Type_ਭਾਰ)})
                                          ->makeStmt());
        }

        // sample(): after all coverpoints have sampled (cross loop runs after coverpoint loop).
        UASSERT_OBJ(m_sampleFuncp, crossp, "sample() CFunc not set for cross");
        // The cross remembers its feeding coverpoints, so sample() needs no cps array;
        // per-bin iff guards still need a temporary array, hence the block form.
        ਅਚਲ ਤਾਰਕਿਕ hasIffs
            = std::any_ਦਾ(bins.begin(), bins.ਸਿਰਾ(),
                          [](ਅਚਲ AstCoverCrossBin* binp) { ਵਾਪਸ binp->iffp() != ਖਾਲੀ_ਸੂਚਕ; });
        Ast_ਗੰਢ_Stmt* ਅਚਲ samplep
            = !hasIffs ? static_cast<Ast_ਗੰਢ_Stmt*>(
                             ਵਸਤੂ_Call(fl, cxVarp, VCMethod::COVERGROUP_SAMPLE)->makeStmt())
                       : static_cast<Ast_ਗੰਢ_Stmt*>(makeCrossIffsCall(
                             fl, bins,
                             ਵਸਤੂ_Call(fl, cxVarp, VCMethod::COVERGROUP_SAMPLE_IFFS,
                                      {ctext(fl, "__Vcx_iffs")})));
        ਜੇ (Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ iffp = crossp->iffp()) {
            m_sampleFuncp->ਜੋੜੋ_Stmtsp(ਨਵਾਂ AstIf{fl, iffp->clone_ਰੁੱਖ(ਝੂਠ), samplep});
        } ਨਹੀਂਤਾਂ {
            m_sampleFuncp->ਜੋੜੋ_Stmtsp(samplep);
        }
    }

    ਖਾਲੀ generate_Cross_ਕੋਡ(AstCoverCross* crossp) {
        UINFO(4, "  Generating code for cross: " << crossp->name());

        // Non-standard hierarchical/dotted cross item (e.g. 'cross a.b'): an implicit coverpoint
        // over the referenced expression (carried in refp->exprp()).  The grammar already warned
        // NONSTD; implicit coverpoints are not yet implemented, so generate no sampling code for
        // this cross.  When support is added the implicit coverpoint should be synthesized
        // upstream (V3LinkParse) as a real AstCoverpoint so it flows through the normal coverpoint
        // path - by here coverpoint lowering has already run.
        ਲਈ (Ast_ਗੰਢ* itemp = crossp->itemsp(); itemp; itemp = itemp->nextp()) {
            ਅਚਲ AstCoverpointRef* ਅਚਲ refp = VN_AS(itemp, CoverpointRef);
            ਜੇ (refp->exprp()) {
                refp->v3warn(COVERIGN,
                             "Unsupported: cross of hierarchical reference (implicit coverpoint)");
                ਵਾਪਸ;
            }
        }

        // A cross naming a bare variable (implicit coverpoint, which Verilator does not
        // synthesize) is dropped entirely with a COVERIGN warning -- it produces no coverage
        // either way -- but only this cross is dropped; its sibling crosses are still generated
        // and the real coverpoints it referenced remain as independent coverpoints.
        ਜੇ (m_droppedCrosses.ਗਿਣਤੀ_2(crossp)) {
            ਲਈ (Ast_ਗੰਢ* itemp = crossp->itemsp(); itemp; itemp = itemp->nextp()) {
                ਅਚਲ AstCoverpointRef* ਅਚਲ refp = VN_AS(itemp, CoverpointRef);
                ਜੇ (m_coverpointMap.ਲੱਭੋ(refp->name()) == m_coverpointMap.ਸਿਰਾ()) {
                    refp->v3warn(COVERIGN, "Unsupported: cross of "
                                               << refp->prettyNameQ()
                                               << " which is not a coverpoint (implicit "
                                                  "coverpoint)");
                    ਰੋਕੋ;
                }
            }
            ਵਾਪਸ;
        }

        // Every cross that isn't dropped routes through a VlCoverCross member.
        generateCross(crossp);
    }

    Ast_ਗੰਢ_ਵਿਅੰਜਕ* buildBinCondition(AstCoverBin* binp, Ast_ਗੰਢ_ਵਿਅੰਜਕ* exprp) {
        // Get the range list from the bin
        Ast_ਗੰਢ* ਅਚਲ rangep = binp->rangesp();
        ਜੇ (!rangep) ਵਾਪਸ ਖਾਲੀ_ਸੂਚਕ;

        // Integral values resolve to the coverpoint's type, as its runtime metadata does
        ਅਚਲ ਤਾਰਕਿਕ integral = exprp->dtypep()->skipRefp()->isIntegralOrPacked();
        // No value form of a wildcard bin is allowed on a real coverpoint (IEEE 1800-2023 19.5.4)
        ਜੇ (binp->isWildcard() && !integral) ਵਾਪਸ wildcardTypeError(binp, exprp);

        // Build condition by OR-ing all ranges together
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਭਰਿਆ_Condp = ਖਾਲੀ_ਸੂਚਕ;

        ਲਈ (Ast_ਗੰਢ* currRangep = rangep; currRangep; currRangep = currRangep->nextp()) {
            Ast_ਗੰਢ_ਵਿਅੰਜਕ* rangeCondp = ਖਾਲੀ_ਸੂਚਕ;
            currRangep = V3Const::constifyEdit(currRangep);

            ਜੇ (AstInsideRange* irp = VN_CAST(currRangep, InsideRange)) {
                Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ minExprp = irp->lhsp();
                Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ maxExprp = irp->rhsp();
                AstConst* ਅਚਲ minConstp = VN_CAST(minExprp, Const);
                AstConst* ਅਚਲ maxConstp = VN_CAST(maxExprp, Const);
                ਅਚਲ ਤਾਰਕਿਕ loUnbounded = VN_IS(minExprp, Unbounded);
                ਅਚਲ ਤਾਰਕਿਕ hiUnbounded = VN_IS(maxExprp, Unbounded);
                ਜੇ (loUnbounded || hiUnbounded) {
                    // Open-ended range: '$' is the coverpoint domain min/max, so the
                    // range reduces to a single inequality (e.g. {[10:$]} -> expr >= 10).
                    AstConst* ਅਚਲ boundp = hiUnbounded ? minConstp : maxConstp;
                    ਜੇ (loUnbounded && hiUnbounded) {
                        rangeCondp = ਨਵਾਂ AstConst{irp->fileline(), AstConst::ਦੋਆਧਾਰੀ_ਅੰਕ_True{}};
                    } ਨਹੀਂਤਾਂ ਜੇ (!boundp) {
                        irp->v3error("Non-constant expression in bin range; "
                                     "range bounds must be constants (IEEE 1800-2023 19.5)");
                        ਜੇ (ਭਰਿਆ_Condp) VL_DO_DANGLING(ਧੱਕੋ_Deletep(ਭਰਿਆ_Condp), ਭਰਿਆ_Condp);
                        ਵਾਪਸ ਖਾਲੀ_ਸੂਚਕ;
                    } ਨਹੀਂਤਾਂ ਜੇ (boundp->ਅੰਕ().isFourState()) {
                        irp->v3error("Four-state (x/z) value in bin range bound; "
                                     "range bounds must be two-state constants");
                        ਜੇ (ਭਰਿਆ_Condp) VL_DO_DANGLING(ਧੱਕੋ_Deletep(ਭਰਿਆ_Condp), ਭਰਿਆ_Condp);
                        ਵਾਪਸ ਖਾਲੀ_ਸੂਚਕ;
                    } ਨਹੀਂਤਾਂ ਜੇ (integral) {
                        rangeCondp = build_ਮੁੱਲ_Condition(binp, exprp, irp);
                    } ਨਹੀਂਤਾਂ {
                        rangeCondp = makeOpenRangeCondition(irp->fileline(), exprp, boundp,
                                                            /*isLowerBound=*/hiUnbounded);
                    }
                } ਨਹੀਂਤਾਂ ਜੇ (!minConstp || !maxConstp) {
                    irp->v3error("Non-constant expression in bin range; "
                                 "range bounds must be constants (IEEE 1800-2023 19.5)");
                    ਜੇ (ਭਰਿਆ_Condp) VL_DO_DANGLING(ਧੱਕੋ_Deletep(ਭਰਿਆ_Condp), ਭਰਿਆ_Condp);
                    ਵਾਪਸ ਖਾਲੀ_ਸੂਚਕ;
                } ਨਹੀਂਤਾਂ ਜੇ (minConstp->ਅੰਕ().isFourState() || maxConstp->ਅੰਕ().isFourState()) {
                    irp->v3error("Four-state (x/z) value in bin range bound; "
                                 "range bounds must be two-state constants");
                    ਜੇ (ਭਰਿਆ_Condp) VL_DO_DANGLING(ਧੱਕੋ_Deletep(ਭਰਿਆ_Condp), ਭਰਿਆ_Condp);
                    ਵਾਪਸ ਖਾਲੀ_ਸੂਚਕ;
                } ਨਹੀਂਤਾਂ ਜੇ (integral) {
                    rangeCondp = build_ਮੁੱਲ_Condition(binp, exprp, irp);
                } ਨਹੀਂਤਾਂ {
                    rangeCondp = makeRangeCondition(irp->fileline(), exprp, minExprp, maxExprp);
                }
            } ਨਹੀਂਤਾਂ ਜੇ (AstConst* constp = VN_CAST(currRangep, Const)) {
                rangeCondp = build_ਮੁੱਲ_Condition(binp, exprp, constp);
            } ਨਹੀਂਤਾਂ {
                currRangep->v3error("Non-constant expression in bin range; values must be "
                                    "constants (IEEE 1800-2023 19.5)");
                ਜੇ (ਭਰਿਆ_Condp) VL_DO_DANGLING(ਧੱਕੋ_Deletep(ਭਰਿਆ_Condp), ਭਰਿਆ_Condp);
                ਵਾਪਸ ਖਾਲੀ_ਸੂਚਕ;
            }

            UASSERT_OBJ(rangeCondp, binp, "rangeCondp is null after building range condition");
            ਭਰਿਆ_Condp
                = ਭਰਿਆ_Condp ? ਨਵਾਂ AstOr{binp->fileline(), ਭਰਿਆ_Condp, rangeCondp} : rangeCondp;
        }

        ਵਾਪਸ ਭਰਿਆ_Condp;
    }

    // Wildcard bits have no meaning for a non-integral coverpoint.
    ਸਥਿਰ Ast_ਗੰਢ_ਵਿਅੰਜਕ* wildcardTypeError(AstCoverBin* binp, Ast_ਗੰਢ_ਵਿਅੰਜਕ* exprp) {
        ਅਚਲ Ast_ਗੰਢ_DType* ਅਚਲ dtypep = exprp->dtypep()->skipRefp();
        exprp->v3error("Cannot use a wildcard bin on a coverpoint of type "
                       << dtypep->prettyDTypeNameQ() << " (IEEE 1800-2023 19.5.4).\n"
                       << exprp->warnContextPrimary() << '\n'
                       << binp->warnOther() << "... Location of wildcard bin\n"
                       << binp->warnContextSecondary());
        ਵਾਪਸ ਨਵਾਂ AstConst{binp->fileline(), AstConst::ਦੋਆਧਾਰੀ_ਅੰਕ_False{}};
    }

    // Match one bin value, range, or wildcard pattern.  Integral coverpoints first resolve the
    // value to their type (IEEE 1800-2023 19.5.7).  Non-owning: clones what it uses.
    Ast_ਗੰਢ_ਵਿਅੰਜਕ* build_ਮੁੱਲ_Condition(AstCoverBin* binp, Ast_ਗੰਢ_ਵਿਅੰਜਕ* exprp, Ast_ਗੰਢ* valuep) {
        File_ਸਤਰ* ਅਚਲ fl = valuep->fileline();
        ਜੇ (!exprp->dtypep()->skipRefp()->isIntegralOrPacked()) {
            ਵਾਪਸ ਨਵਾਂ AstEq{fl, exprp->clone_ਰੁੱਖ(ਝੂਠ),
                             VN_AS(valuep, ਗੰਢ_ਵਿਅੰਜਕ)->clone_ਰੁੱਖ(ਝੂਠ)};
        }
        Cross_ਮੁੱਲ_Range range{valuep, resolve_ਚੌੜਾਈ(valuep, exprp)};
        ਜੇ (!resolve_ਮੁੱਲ(valuep, exprp, ਸੱਚ, binp->isWildcard(), range)) {
            valuep->v3warn(E_UNSUPPORTED,
                           "Unsupported: non-integral value in a coverage bin of an "
                           "integral coverpoint.");
            ਵਾਪਸ ਨਵਾਂ AstConst{fl, AstConst::ਦੋਆਧਾਰੀ_ਅੰਕ_False{}};
        }
        ਜੇ (cross_Range_ਖਾਲੀ_ਥਾਵਾਂ(range)) ਵਾਪਸ ਨਵਾਂ AstConst{fl, AstConst::ਦੋਆਧਾਰੀ_ਅੰਕ_False{}};
        AstConst* ਅਚਲ lop = new_ਮੁੱਲ_Const(fl, range.lo, exprp);
        AstConst* ਅਚਲ hip = new_ਮੁੱਲ_Const(fl, range.hi, exprp);
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* condp = ਖਾਲੀ_ਸੂਚਕ;
        ਜੇ (lop->ਅੰਕ().isCaseEq(hip->ਅੰਕ())) {
            condp = ਨਵਾਂ AstEq{fl, exprp->clone_ਰੁੱਖ(ਝੂਠ), lop};
        } ਨਹੀਂਤਾਂ {
            condp = makeRangeCondition(fl, exprp, lop, hip);
            VL_DO_DANGLING(ਧੱਕੋ_Deletep(lop), lop);
        }
        VL_DO_DANGLING(ਧੱਕੋ_Deletep(hip), hip);
        ਜੇ (!range.wildcard) ਵਾਪਸ condp;
        // Match the pattern's value bits within the source-value bounds.
        V3_ਸੰਖਿਆ ਨਕਾਬ{valuep, exprp->ਚੌੜਾਈ()};
        V3_ਸੰਖਿਆ ਮੁੱਲ{valuep, exprp->ਚੌੜਾਈ()};
        ਨਕਾਬ.op_ਬਿੱਟ_Non_XZ(range.ਨਮੂਨਾ_2);
        ਮੁੱਲ.op_ਬਿੱਟ_One(range.ਨਮੂਨਾ_2);
        AstConst* ਅਚਲ ਨਕਾਬ_Constp = new_ਮੁੱਲ_Const(fl, ਨਕਾਬ, exprp);
        AstConst* ਅਚਲ ਮੁੱਲ_Constp = new_ਮੁੱਲ_Const(fl, ਮੁੱਲ, exprp);
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ ਵਿਅੰਜਕ_Masked = ਨਵਾਂ AstAnd{fl, exprp->clone_ਰੁੱਖ(ਝੂਠ), ਨਕਾਬ_Constp};
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ ਮੁੱਲ_Masked = ਨਵਾਂ AstAnd{fl, ਮੁੱਲ_Constp, ਨਕਾਬ_Constp->clone_ਰੁੱਖ(ਝੂਠ)};
        ਵਾਪਸ ਨਵਾਂ AstLogAnd{fl, condp, ਨਵਾਂ AstEq{fl, ਵਿਅੰਜਕ_Masked, ਮੁੱਲ_Masked}};
    }

    ਖਾਲੀ generate_Coverage_Computation_ਕੋਡ() {
        UINFO(4, "  Generating coverage computation code");

        // Invalidate cache: addMembersp() calls in generateCoverpointCode/generateCrossCode
        // have added new members since the last scan, so clear before re-querying.
        m_memberMap.clear();

        // get_inst_coverage(): the average of the coverpoints and crosses, weighted by their
        // option.weight (IEEE 1800-2023 19.11).  The instance node holds their runtimes.  With
        // the instances merged, get_coverage() instead, unless option.get_inst_coverage
        // (Table 19-1).
        AstFunc* ਅਚਲ getInstCoveragep
            = VN_AS(m_memberMap.ਲੱਭੋ_Member(m_covergroupp, "get_inst_coverage"), Func);
        File_ਸਤਰ* ਅਚਲ instFl = getInstCoveragep->fileline();
        AstCMethodHard* ਅਚਲ instCallp = instanceCall(instFl, VCMethod::COVERGROUP_COVERAGE);
        instCallp->dtype_ਸੈੱਟ_Double();
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* instValuep = instCallp;
        ਜੇ (m_cg_May_ਮਿਲਾਓ) {
            Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ mergedp = ਨਵਾਂ AstLogAnd{
                instFl, newOptionSel(instFl, optionVar(ਸੱਚ), "merge_instances", VAccess::READ),
                ਨਵਾਂ AstLogNot{instFl, newOptionSel(instFl, optionVar(ਝੂਠ), "get_inst_coverage",
                                                   VAccess::READ)}};
            instValuep = ਨਵਾਂ AstCond{instFl, mergedp, typeCoverageCall(instFl), instCallp};
        }
        getInstCoveragep->ਜੋੜੋ_Stmtsp(ਨਵਾਂ Ast_ਸੌਂਪੋ{
            instFl, ਨਵਾਂ AstVarRef{instFl, VN_AS(getInstCoveragep->fvarp(), Var), VAccess::WRITE},
            instValuep});

        // get_coverage(): the average of the covergroup's instances, weighted by their
        // option.weight, or with type_option.merge_instances, the coverage of the union of their
        // bins (IEEE 1800-2023 19.11.3).  Static, so the registry finds the instances.
        AstFunc* ਅਚਲ getCoveragep
            = VN_AS(m_memberMap.ਲੱਭੋ_Member(m_covergroupp, "get_coverage"), Func);
        File_ਸਤਰ* ਅਚਲ typeFl = getCoveragep->fileline();
        getCoveragep->ਜੋੜੋ_Stmtsp(ਨਵਾਂ Ast_ਸੌਂਪੋ{
            typeFl, ਨਵਾਂ AstVarRef{typeFl, VN_AS(getCoveragep->fvarp(), Var), VAccess::WRITE},
            typeCoverageCall(typeFl)});
    }

    // The registry call computing the covergroup's type coverage, per its type options
    AstCMethodHard* typeCoverageCall(File_ਸਤਰ* fl) {
        AstCExpr* ਅਚਲ registryp = ctext(fl, "vlSymsp->_vm_contextp__->covergroupRegistryp()");
        registryp->dtype_ਸੈੱਟ_Void();  // Opaque receiver; only ever the 'fromp' of the call below
        AstCMethodHard* ਅਚਲ callp
            = ਨਵਾਂ AstCMethodHard{fl, registryp, VCMethod::COVERGROUP_TYPE_COVERAGE};
        callp->ਜੋੜੋ_Pinsp(ctext(fl, quoted(covergroupProtectedName())));
        callp->ਜੋੜੋ_Pinsp(newOptionSel(fl, optionVar(ਸੱਚ), "weight", VAccess::READ));
        callp->ਜੋੜੋ_Pinsp(newOptionSel(fl, optionVar(ਸੱਚ), "merge_instances", VAccess::READ));
        callp->ਜੋੜੋ_Pinsp(file_ਸਤਰ_Debug(m_covergroupp->fileline()));
        callp->usePtr(ਸੱਚ);
        callp->dtype_ਸੈੱਟ_Double();
        ਵਾਪਸ callp;
    }

    // VISITORS
    ਸਥਿਰ ਤਾਰਕਿਕ isEnclosingInstanceVar(ਅਚਲ AstVar* varp) {
        ਵਾਪਸ varp->isClassMember() && !varp->lifetime().isStatic() && !varp->isParam();
    }

    ਖਾਲੀ rewriteThisRef(AstThisRef* refp, AstVar* handleVarp) {
        ਅਚਲ AstClassRefDType* ਅਚਲ refDTypep
            = VN_CAST(refp->dtypep()->skipRefp(), ClassRefDType);
        UASSERT_OBJ(refDTypep && refDTypep->classp() == m_covergroupp, refp,
                    "Unexpected this reference in embedded covergroup");
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ newp = ਨਵਾਂ AstVarRef{refp->fileline(), handleVarp, VAccess::READ};
        refp->replaceWith(newp);
        VL_DO_DANGLING(ਧੱਕੋ_Deletep(refp), refp);
    }

    ਖਾਲੀ rewriteVarRef(AstVarRef* refp, AstVar* handleVarp) {
        File_ਸਤਰ* ਅਚਲ fl = refp->fileline();
        // As V3LinkLValue does, the handle of a written member select is marked written
        AstMemberSel* ਅਚਲ selp
            = ਨਵਾਂ AstMemberSel{fl, ਨਵਾਂ AstVarRef{fl, handleVarp, refp->access()}, refp->varp()};
        selp->access(refp->access());
        refp->replaceWith(selp);
        VL_DO_DANGLING(ਧੱਕੋ_Deletep(refp), refp);
    }

    ਖਾਲੀ rewriteFuncRef(AstFuncRef* refp, AstVar* handleVarp) {
        File_ਸਤਰ* ਅਚਲ fl = refp->fileline();
        AstArg* ਅਚਲ argsp = refp->argsp() ? refp->argsp()->unlink_Fr_ਪਿੱਠ_With_ਅਗਲਾ() : ਖਾਲੀ_ਸੂਚਕ;
        Ast_ਵਿਧੀ_Call* ਅਚਲ callp = ਨਵਾਂ Ast_ਵਿਧੀ_Call{
            fl, ਨਵਾਂ AstVarRef{fl, handleVarp, VAccess::READ}, refp->name(), argsp};
        callp->taskp(refp->taskp());
        callp->dtypeFrom(refp);
        refp->replaceWith(callp);
        VL_DO_DANGLING(ਧੱਕੋ_Deletep(refp), refp);
    }

    // True if funcp is an instance method of the enclosing class or one of its bases
    ਤਾਰਕਿਕ isEnclosingInstanceFunc(ਅਚਲ Ast_ਗੰਢ_FTask* funcp) ਅਚਲ {
        ਜੇ (!funcp->class_ਵਿਧੀ() || funcp->isStatic()) ਵਾਪਸ ਝੂਠ;
        ਵਾਪਸ AstClass::isClassExtendedFrom(m_enclosingClassp, VN_AS(funcp->aboveLoopp(), Class));
    }

    ਤਾਰਕਿਕ isEmbeddedCovergroupVar(ਅਚਲ AstVar* varp) ਅਚਲ {
        ਜੇ (!varp || !varp->isClassMember() || varp->isDeclTyped()) ਵਾਪਸ ਝੂਠ;
        ਅਚਲ AstClassRefDType* ਅਚਲ refp = VN_CAST(varp->dtypep()->skipRefp(), ClassRefDType);
        ਵਾਪਸ refp && refp->classp() == m_covergroupp;
    }

    AstVar* ਲੱਭੋ_Embedded_Covergroup_Var() ਅਚਲ {
        ਜੇ (!m_enclosingClassp) ਵਾਪਸ ਖਾਲੀ_ਸੂਚਕ;
        ਲਈ (Ast_ਗੰਢ* itemp = m_enclosingClassp->membersp(); itemp; itemp = itemp->nextp()) {
            ਜੇ (AstVar* ਅਚਲ varp = VN_CAST(itemp, Var)) {
                ਜੇ (isEmbeddedCovergroupVar(varp)) ਵਾਪਸ varp;
            }
        }
        // V3LinkParse always creates an implicit variable for an embedded covergroup.
        ਵਾਪਸ ਖਾਲੀ_ਸੂਚਕ;  // LCOV_EXCL_LINE
    }

    std::ਸਦਿਸ਼<Ast_ਗੰਢ_ਸੌਂਪੋ*> ਲੱਭੋ_Covergroup_Constructions() {
        std::ਸਦਿਸ਼<Ast_ਗੰਢ_ਸੌਂਪੋ*> foundps;
        ਜੇ (!m_embeddedVarp) ਵਾਪਸ foundps;
        AstFunc* ਅਚਲ enclosingNewp
            = VN_CAST(m_memberMap.ਲੱਭੋ_Member(m_enclosingClassp, "new"), Func);
        ਜੇ (!enclosingNewp) ਵਾਪਸ foundps;
        enclosingNewp->foreach([&](Ast_ਗੰਢ_ਸੌਂਪੋ* asgnp) {
            ਅਚਲ AstNew* ਅਚਲ newp = VN_CAST(asgnp->rhsp(), New);
            ਅਚਲ AstVarRef* ਅਚਲ lhsRefp = VN_CAST(asgnp->lhsp(), VarRef);
            ਜੇ (!newp || !lhsRefp || lhsRefp->varp() != m_embeddedVarp) ਵਾਪਸ;
            ਅਚਲ AstClassRefDType* ਅਚਲ refp = VN_CAST(newp->dtypep(), ClassRefDType);
            ਜੇ (refp && refp->classp() == m_covergroupp) foundps.ਧੱਕੋ_ਪਿੱਠ(asgnp);
        });
        ਵਾਪਸ foundps;
    }

    std::ਸੈੱਟ<ਅਚਲ AstVar*> enclosingInstanceVars() ਅਚਲ {
        std::ਸੈੱਟ<ਅਚਲ AstVar*> vars;
        ਜੇ (m_enclosingClassp) {
            m_enclosingClassp->foreachMember([&](AstClass* ਅਚਲ, AstVar* ਅਚਲ varp) {
                ਜੇ (isEnclosingInstanceVar(varp)) vars.ਪਾਓ(varp);
            });
        }
        ਵਾਪਸ vars;
    }

    ਤਾਰਕਿਕ hasEnclosingEventRef(AstCovergroup* cgp) ਅਚਲ {
        ਜੇ (!m_embeddedVarp || !cgp->eventp()) ਵਾਪਸ ਝੂਠ;
        ਅਚਲ std::ਸੈੱਟ<ਅਚਲ AstVar*> enclosingVars = enclosingInstanceVars();
        ਤਾਰਕਿਕ ਮਿਲਿਆ = ਝੂਠ;
        cgp->eventp()->foreach([&](AstVarRef* refp) {
            ਜੇ (enclosingVars.ਗਿਣਤੀ_2(refp->varp())) ਮਿਲਿਆ = ਸੱਚ;
        });
        ਵਾਪਸ ਮਿਲਿਆ;
    }

    ਸਥਿਰ ਤਾਰਕਿਕ parse_Embedded_Event_ਵਿਅੰਜਕ(Ast_ਗੰਢ_ਵਿਅੰਜਕ* exprp, AstVar*& ਅਧਾਰ_Varp,
                                       AstVar*& memberVarp) {
        ਜੇ (AstVarRef* ਅਚਲ refp = VN_CAST(exprp, VarRef)) {
            ਅਧਾਰ_Varp = refp->varp();
            memberVarp = ਖਾਲੀ_ਸੂਚਕ;
            ਵਾਪਸ ਸੱਚ;
        }
        AstMemberSel* ਅਚਲ selp = VN_CAST(exprp, MemberSel);
        ਜੇ (!selp) ਵਾਪਸ ਝੂਠ;
        AstVarRef* ਅਚਲ ਅਧਾਰ_Refp = VN_CAST(selp->fromp(), VarRef);
        ਜੇ (!ਅਧਾਰ_Refp) ਵਾਪਸ ਝੂਠ;
        ਅਧਾਰ_Varp = ਅਧਾਰ_Refp->varp();
        memberVarp = selp->varp();
        ਵਾਪਸ ਸੱਚ;
    }

    ਤਾਰਕਿਕ isEventLvalue(Ast_ਗੰਢ_ਵਿਅੰਜਕ* exprp, ਅਚਲ EmbeddedEventTrigger& trigger) ਅਚਲ {
        ਜੇ (AstSel* ਅਚਲ selp = VN_CAST(exprp, Sel)) exprp = selp->fromp();
        AstVar* ਅਧਾਰ_Varp = ਖਾਲੀ_ਸੂਚਕ;
        AstVar* memberVarp = ਖਾਲੀ_ਸੂਚਕ;
        ਜੇ (!parse_Embedded_Event_ਵਿਅੰਜਕ(exprp, ਅਧਾਰ_Varp, memberVarp)) ਵਾਪਸ ਝੂਠ;
        ਵਾਪਸ ਅਧਾਰ_Varp == trigger.ਅਧਾਰ_Varp && memberVarp == trigger.memberVarp;
    }

    Ast_ਗੰਢ_ਵਿਅੰਜਕ* newEventRead(File_ਸਤਰ* fl, ਅਚਲ EmbeddedEventTrigger& trigger) ਅਚਲ {
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ basep = ਨਵਾਂ AstVarRef{fl, trigger.ਅਧਾਰ_Varp, VAccess::READ};
        ਜੇ (!trigger.memberVarp) ਵਾਪਸ basep;
        AstMemberSel* ਅਚਲ selp = ਨਵਾਂ AstMemberSel{fl, basep, trigger.memberVarp};
        selp->access(VAccess::READ);
        ਵਾਪਸ selp;
    }

    string eventPrevName(ਅਚਲ EmbeddedEventTrigger& trigger, ਮਾਪ_t trigger_ਸੂਚਕਾਂਕ) ਅਚਲ {
        string name = "__Vcg_prev_" + m_embeddedVarp->name() + "_" + std::ਮੰਜ਼ਿਲ_string(trigger_ਸੂਚਕਾਂਕ)
                      + "_" + trigger.ਅਧਾਰ_Varp->name();
        ਜੇ (trigger.memberVarp) name += "_" + trigger.memberVarp->name();
        ਵਾਪਸ name;
    }

    Ast_ਗੰਢ_ਵਿਅੰਜਕ* newEmbeddedVarNonNull(File_ਸਤਰ* fl) ਅਚਲ {
        ਵਾਪਸ ਨਵਾਂ AstNeq{fl, ਨਵਾਂ AstVarRef{fl, m_embeddedVarp, VAccess::READ},
                          ਨਵਾਂ AstConst{fl, AstConst::Null{}}};
    }

    Ast_ਗੰਢ_Stmt* newSampleStmt(File_ਸਤਰ* fl) ਅਚਲ {
        Ast_ਵਿਧੀ_Call* ਅਚਲ callp = ਨਵਾਂ Ast_ਵਿਧੀ_Call{
            fl, ਨਵਾਂ AstVarRef{fl, m_embeddedVarp, VAccess::READ}, "sample", ਖਾਲੀ_ਸੂਚਕ};
        callp->taskp(m_sampleFuncp);
        callp->dtype_ਸੈੱਟ_Void();
        ਵਾਪਸ callp->makeStmt();
    }

    ਖਾਲੀ installEmbeddedEventFork(Ast_Sen_ਰੁੱਖ* eventp,
                                  ਅਚਲ std::ਸਦਿਸ਼<Ast_ਗੰਢ_ਸੌਂਪੋ*>& constructps) {
        // IEEE 1800-2023 19.3 samples coverpoints whenever their clocking event occurs. A
        // per-instance event cannot use V3Active's static sensitivity path, so spawn
        // 'fork forever begin @(event); cg.sample(); end join_none' after each construction.
        ਲਈ (Ast_ਗੰਢ_ਸੌਂਪੋ* ਅਚਲ constructp : constructps) {
            File_ਸਤਰ* ਅਚਲ fl = constructp->fileline();
            AstLoop* ਅਚਲ loopp = ਨਵਾਂ AstLoop{fl};
            loopp->ਜੋੜੋ_Stmtsp(ਨਵਾਂ AstEventControl{fl, eventp->clone_ਰੁੱਖ(ਝੂਠ), ਖਾਲੀ_ਸੂਚਕ});
            loopp->ਜੋੜੋ_Stmtsp(ਨਵਾਂ AstIf{fl, newEmbeddedVarNonNull(fl), newSampleStmt(fl)});
            AstFork* ਅਚਲ forkp = ਨਵਾਂ AstFork{fl, VJoinType::JOIN_NONE};
            forkp->ਤੁਰੰਤ_ਸ਼ੁਰੂ(ਸੱਚ);
            forkp->ਜੋੜੋ_Forksp(ਨਵਾਂ AstBegin{fl, "", loopp, ਸੱਚ});
            constructp->ਜੋੜੋ_ਅਗਲਾ_ਇੱਥੇ(forkp);
        }
        VL_DO_DANGLING(ਧੱਕੋ_Deletep(eventp), eventp);
    }

    Ast_ਗੰਢ_ਵਿਅੰਜਕ* newEventReadyCondition(File_ਸਤਰ* fl, ਅਚਲ EmbeddedEventTrigger& trigger) ਅਚਲ {
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ curp = newEventRead(fl, trigger);
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ prevp = ਨਵਾਂ AstVarRef{fl, trigger.prevVarp, VAccess::READ};
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* edgep = ਖਾਲੀ_ਸੂਚਕ;
        // IEEE 1800-2023 9.4.2 detects edge-qualified events only on the expression's LSB,
        // while an implicit change event observes the complete expression.
        ਜੇ (trigger.ਕਿਨਾਰਾ_Type == VEdgeType::ET_POSEDGE) {
            edgep = ਨਵਾਂ AstSel{fl, ਨਵਾਂ AstAnd{fl, curp, ਨਵਾਂ AstNot{fl, prevp}}, 0, 1};
        } ਨਹੀਂਤਾਂ ਜੇ (trigger.ਕਿਨਾਰਾ_Type == VEdgeType::ET_NEGEDGE) {
            edgep = ਨਵਾਂ AstSel{fl, ਨਵਾਂ AstAnd{fl, ਨਵਾਂ AstNot{fl, curp}, prevp}, 0, 1};
        } ਨਹੀਂਤਾਂ ਜੇ (trigger.ਕਿਨਾਰਾ_Type == VEdgeType::ET_BOTHEDGE) {
            edgep = ਨਵਾਂ AstSel{fl, ਨਵਾਂ AstXor{fl, curp, prevp}, 0, 1};
        } ਨਹੀਂਤਾਂ {
            edgep = ਨਵਾਂ AstNeq{fl, curp, prevp};
        }
        ਵਾਪਸ ਨਵਾਂ AstLogAnd{fl, newEmbeddedVarNonNull(fl), edgep};
    }

    std::ਸਦਿਸ਼<EmbeddedEventTrigger> collectEmbeddedEventTriggers(AstCovergroup* cgp) {
        std::ਸਦਿਸ਼<EmbeddedEventTrigger> triggers;
        ਅਚਲ std::ਸੈੱਟ<ਅਚਲ AstVar*> enclosingVars = enclosingInstanceVars();
        ਲਈ (Ast_ਗੰਢ* senp = cgp->eventp()->sensesp(); senp; senp = senp->nextp()) {
            Ast_Sen_ਵਸਤੂ* ਅਚਲ itemp = VN_AS(senp, Sen_ਵਸਤੂ);
            AstVar* ਅਧਾਰ_Varp = ਖਾਲੀ_ਸੂਚਕ;
            AstVar* memberVarp = ਖਾਲੀ_ਸੂਚਕ;
            ਜੇ (!parse_Embedded_Event_ਵਿਅੰਜਕ(itemp->sensp(), ਅਧਾਰ_Varp, memberVarp)
                || !enclosingVars.ਗਿਣਤੀ_2(ਅਧਾਰ_Varp)) {
                ਵਾਪਸ {};
            }
            triggers.emplace_ਪਿੱਠ(itemp->fileline(), ਅਧਾਰ_Varp, memberVarp, itemp->ਕਿਨਾਰਾ_Type());
        }
        ਵਾਪਸ triggers;
    }

    ਖਾਲੀ installEmbeddedEventTriggers(std::ਸਦਿਸ਼<EmbeddedEventTrigger>& triggers,
                                      ਅਚਲ std::ਸਦਿਸ਼<Ast_ਗੰਢ_ਸੌਂਪੋ*>& constructps) {
        // Without --timing, approximate a per-instance event by sampling after assignments
        // within the enclosing class. External writes and exact scheduling cannot be observed.
        ਜੇ (constructps.ਖਾਲੀ_ਥਾਵਾਂ()) ਵਾਪਸ;
        ਲਈ (ਮਾਪ_t trigger_ਸੂਚਕਾਂਕ = 0; trigger_ਸੂਚਕਾਂਕ < triggers.ਮਾਪ(); ++trigger_ਸੂਚਕਾਂਕ) {
            EmbeddedEventTrigger& trigger = triggers[trigger_ਸੂਚਕਾਂਕ];
            std::ਸਦਿਸ਼<Ast_ਗੰਢ_ਸੌਂਪੋ*> assignps;
            m_enclosingClassp->foreach([&](Ast_ਗੰਢ_ਸੌਂਪੋ* asgnp) {
                ਜੇ (isEventLvalue(asgnp->lhsp(), trigger)) assignps.ਧੱਕੋ_ਪਿੱਠ(asgnp);
            });
            ਜੇ (assignps.ਖਾਲੀ_ਥਾਵਾਂ()) {
                trigger.eventFl->v3warn(
                    COVERIGN, "Unsupported: 'covergroup' clocking event signal has no assignment "
                              "within the enclosing class; no coverage sampled. Use --timing for "
                              "full support.");
                ਜਾਰੀ;
            }
            Ast_ਗੰਢ_DType* ਅਚਲ dtypep
                = trigger.memberVarp ? trigger.memberVarp->dtypep() : trigger.ਅਧਾਰ_Varp->dtypep();
            AstVar* ਅਚਲ prevVarp = ਨਵਾਂ AstVar{trigger.eventFl, VVarType::MEMBER,
                                                eventPrevName(trigger, trigger_ਸੂਚਕਾਂਕ), dtypep};
            m_enclosingClassp->ਜੋੜੋ_Membersp(prevVarp);
            trigger.prevVarp = prevVarp;
            ਲਈ (Ast_ਗੰਢ_ਸੌਂਪੋ* ਅਚਲ asgnp : assignps) {
                File_ਸਤਰ* ਅਚਲ fl = asgnp->fileline();
                AstIf* ਅਚਲ ifp
                    = ਨਵਾਂ AstIf{fl, newEventReadyCondition(fl, trigger), newSampleStmt(fl)};
                ifp->ਜੋੜੋ_ਅਗਲਾ_ਇੱਥੇ(ਨਵਾਂ Ast_ਸੌਂਪੋ{fl,
                                               ਨਵਾਂ AstVarRef{fl, trigger.prevVarp, VAccess::WRITE},
                                               newEventRead(fl, trigger)});
                asgnp->ਜੋੜੋ_ਅਗਲਾ_ਇੱਥੇ(ifp);
            }
        }
    }

    ਖਾਲੀ delete_Coverage_ਚੀਜ਼ਾਂ() {
        ਲਈ (AstCoverpoint* ਅਚਲ cpp : m_coverpoints) {
            VL_DO_DANGLING(ਧੱਕੋ_Deletep(cpp->unlink_Fr_ਪਿੱਠ()), cpp);
        }
        ਲਈ (AstCoverCross* ਅਚਲ crossp : m_coverCrosses) {
            VL_DO_DANGLING(ਧੱਕੋ_Deletep(crossp->unlink_Fr_ਪਿੱਠ()), crossp);
        }
        // Options not lowered: the covergroup was not processed
        ਲਈ (Ast_Cg_Option_ਸੌਂਪੋ* ਅਚਲ optp : m_cgOptions) {
            VL_DO_DANGLING(ਧੱਕੋ_Deletep(optp->unlink_Fr_ਪਿੱਠ()), optp);
        }
        m_cgOptions.clear();
    }

    ਵਰਗ FormalRefVisitor final : ਜਨਤਕ VNVisitor {
        ਅਚਲ std::map<ਅਚਲ AstVar*, AstVar*>& m_replacements;

        ਖਾਲੀ ਦੇਖੋ(AstVarRef* nodep) override {
            ਅਚਲ ਸਵੈਚਾਲਿਤ it = m_replacements.ਲੱਭੋ(nodep->varp());
            ਜੇ (it == m_replacements.ਸਿਰਾ()) ਵਾਪਸ;
            nodep->varp(it->ਦੂਜਾ);
        }
        ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ* nodep) override { iterateChildren(nodep); }

    ਜਨਤਕ:
        ਸਪਸ਼ਟ FormalRefVisitor(ਅਚਲ std::map<ਅਚਲ AstVar*, AstVar*>& replacements)
            : m_replacements{replacements} {}
        ਖਾਲੀ ਜਾਂਚ(Ast_ਗੰਢ* nodep) { iterate(nodep); }
    };

    ਖਾਲੀ validateCovergroupExpressions() {
        std::ਸੈੱਟ<ਅਚਲ AstVar*> sampleMembers;
        std::ਸੈੱਟ<ਅਚਲ AstVar*> constructorRefMembers;
        ਲਈ (Ast_ਗੰਢ* stmtp = m_constructorp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            ਅਚਲ AstVar* ਅਚਲ varp = VN_CAST(stmtp, Var);
            ਜੇ (!varp || !varp->isIO() || (!varp->isRef() && !varp->isConstRef())) ਜਾਰੀ;
            ਅਚਲ AstVar* ਅਚਲ memberp
                = VN_CAST(m_memberMap.ਲੱਭੋ_Member(m_covergroupp, varp->name()), Var);
            UASSERT_OBJ(memberp && memberp->isClassMember(), varp,
                        "Covergroup constructor argument missing persistent member");
            constructorRefMembers.ਪਾਓ(varp);
            constructorRefMembers.ਪਾਓ(memberp);
        }
        ਲਈ (Ast_ਗੰਢ* stmtp = m_sampleFuncp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            ਅਚਲ AstVar* ਅਚਲ varp = VN_CAST(stmtp, Var);
            ਜੇ (!varp || !varp->isIO()) ਜਾਰੀ;
            ਅਚਲ AstVar* ਅਚਲ memberp
                = VN_CAST(m_memberMap.ਲੱਭੋ_Member(m_covergroupp, varp->name()), Var);
            UASSERT_OBJ(memberp && memberp->isClassMember(), varp,
                        "Covergroup sample argument missing persistent member");
            sampleMembers.ਪਾਓ(memberp);
        }
        Covergroup_ਵਿਅੰਜਕ_Valid_Visitor{sampleMembers, constructorRefMembers}.ਜਾਂਚ(m_constructorp);
    }

    ਖਾਲੀ rebindFormalRefs() {
        std::map<ਅਚਲ AstVar*, AstVar*> replacements;
        ਲਈ (Ast_ਗੰਢ* stmtp = m_constructorp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            ਜੇ (ਅਚਲ AstVar* ਅਚਲ varp = VN_CAST(stmtp, Var)) {
                ਜੇ (!varp->isIO()) ਜਾਰੀ;
                AstVar* ਅਚਲ memberp
                    = VN_CAST(m_memberMap.ਲੱਭੋ_Member(m_covergroupp, varp->name()), Var);
                UASSERT_OBJ(memberp && memberp->isClassMember(), varp,
                            "Covergroup constructor argument missing persistent member");
                replacements.emplace(varp, memberp);
            }
        }
        ਮਾਪ_t expectedBindings = 0;
        ਲਈ (ਅਚਲ ਸਵੈਚਾਲਿਤ& pair : replacements) {
            ਜੇ (pair.ਪਹਿਲਾ->isRef() || pair.ਪਹਿਲਾ->isConstRef()) ++expectedBindings;
        }
        ਮਾਪ_t rewrittenBindings = 0;
        ਲਈ (Ast_ਗੰਢ* stmtp = m_constructorp->stmtsp(); stmtp;) {
            Ast_ਗੰਢ* ਅਚਲ nextp = stmtp->nextp();
            ਜੇ (Ast_ਸੌਂਪੋ* ਅਚਲ assignp = VN_CAST(stmtp, ਸੌਂਪੋ)) {
                AstVarRef* ਅਚਲ lhsp = VN_CAST(assignp->lhsp(), VarRef);
                AstVarRef* ਅਚਲ rhsp = VN_CAST(assignp->rhsp(), VarRef);
                ਜੇ (lhsp && rhsp
                    && (lhsp->varp()->declDirection() == VDirection::REF
                        || lhsp->varp()->declDirection() == VDirection::CONSTREF)) {
                    ਅਚਲ ਸਵੈਚਾਲਿਤ it = replacements.ਲੱਭੋ(rhsp->varp());
                    UASSERT_OBJ(it != replacements.ਸਿਰਾ() && it->ਦੂਜਾ == lhsp->varp(), assignp,
                                "Unexpected covergroup reference binding assignment");
                    AstCExpr* ਅਚਲ bindp = ਨਵਾਂ AstCExpr{assignp->fileline(), ""};
                    bindp->ਜੋੜੋ(lhsp->unlink_Fr_ਪਿੱਠ());
                    bindp->ਜੋੜੋ(" = &");
                    bindp->ਜੋੜੋ(rhsp->unlink_Fr_ਪਿੱਠ());
                    assignp->replaceWith(bindp->makeStmt());
                    VL_DO_DANGLING(ਧੱਕੋ_Deletep(assignp), assignp);
                    ++rewrittenBindings;
                }
            }
            stmtp = nextp;
        }
        UASSERT_OBJ(rewrittenBindings == expectedBindings, m_constructorp,
                    "Covergroup reference argument missing binding");
        FormalRefVisitor visitor{replacements};
        ਲਈ (AstCoverpoint* ਅਚਲ cpp : m_coverpoints) visitor.ਜਾਂਚ(cpp);
        ਲਈ (AstCoverCross* ਅਚਲ crossp : m_coverCrosses) visitor.ਜਾਂਚ(crossp);
    }

    AstVarRef* install_Enclosing_ਪਿੱਠ_ਸੂਚਕ(ਅਚਲ std::ਸਦਿਸ਼<Ast_ਗੰਢ_ਸੌਂਪੋ*>& constructps) {
        // Simple-case support for embedded covergroups (IEEE 1800-2023 19.4) whose
        // coverpoints reference members of the enclosing class ("Class members can be used
        // in coverpoint expressions").  The covergroup is lowered into a sibling class with
        // no implicit handle to the enclosing object, so such references would emit
        // uncompilable C++.  Add an explicit back-pointer member to the enclosing instance,
        // route member references through it, and pass it into the constructor so
        // coverage initialization can read enclosing members. Returns an invalid
        // reference if an outer class member cannot be reached; otherwise returns an empty result.
        ਜੇ (!m_enclosingClassp) ਵਾਪਸ ਖਾਲੀ_ਸੂਚਕ;  // Offending refs require an enclosing class

        AstVarRef* invalidp = ਖਾਲੀ_ਸੂਚਕ;
        Ast_ਗੰਢ* offenderp = ਖਾਲੀ_ਸੂਚਕ;
        std::ਸੈੱਟ<ਅਚਲ AstVar*> ownVars;
        ਲਈ (Ast_ਗੰਢ* itemp = m_covergroupp->membersp(); itemp; itemp = itemp->nextp()) {
            ਜੇ (ਅਚਲ AstVar* ਅਚਲ varp = VN_CAST(itemp, Var)) ownVars.ਪਾਓ(varp);
        }
        ਅਚਲ std::ਸੈੱਟ<ਅਚਲ AstVar*> enclosingVars = enclosingInstanceVars();
        std::ਸਦਿਸ਼<AstVarRef*> refs_ਮੰਜ਼ਿਲ_Rewrite;
        std::ਸਦਿਸ਼<AstThisRef*> this_Refs_ਮੰਜ਼ਿਲ_Rewrite;
        std::ਸਦਿਸ਼<AstFuncRef*> func_Refs_ਮੰਜ਼ਿਲ_Rewrite;
        ਅਚਲ ਸਵੈਚਾਲਿਤ ਜਾਂਚ = [&](Ast_ਗੰਢ* rootp) {
            rootp->foreach([&](AstVarRef* refp) {
                ਜੇ (invalidp) ਵਾਪਸ;
                ਅਚਲ AstVar* ਅਚਲ varp = refp->varp();
                ਜੇ (!isEnclosingInstanceVar(varp) || ownVars.ਗਿਣਤੀ_2(varp)) ਵਾਪਸ;
                ਜੇ (!enclosingVars.ਗਿਣਤੀ_2(varp)) {
                    invalidp = refp;
                    ਵਾਪਸ;
                }
                refs_ਮੰਜ਼ਿਲ_Rewrite.ਧੱਕੋ_ਪਿੱਠ(refp);
                ਜੇ (!offenderp) offenderp = refp;
            });
            ਜੇ (invalidp) ਵਾਪਸ;
            rootp->foreach([&](AstThisRef* refp) {
                ਅਚਲ AstClassRefDType* ਅਚਲ refDTypep
                    = VN_CAST(refp->dtypep()->skipRefp(), ClassRefDType);
                ਜੇ (refDTypep && refDTypep->classp() == m_covergroupp) {
                    this_Refs_ਮੰਜ਼ਿਲ_Rewrite.ਧੱਕੋ_ਪਿੱਠ(refp);
                    ਜੇ (!offenderp) offenderp = refp;
                }
            });
            rootp->foreach([&](AstFuncRef* refp) {
                ਜੇ (!isEnclosingInstanceFunc(refp->taskp())) ਵਾਪਸ;
                func_Refs_ਮੰਜ਼ਿਲ_Rewrite.ਧੱਕੋ_ਪਿੱਠ(refp);
                ਜੇ (!offenderp) offenderp = refp;
            });
        };
        ਲਈ (AstCoverpoint* ਅਚਲ cpp : m_coverpoints) ਜਾਂਚ(cpp);
        ਲਈ (AstCoverCross* ਅਚਲ crossp : m_coverCrosses) ਜਾਂਚ(crossp);
        ਲਈ (Ast_Cg_Option_ਸੌਂਪੋ* ਅਚਲ optp : m_cgOptions) ਜਾਂਚ(optp);
        ਜੇ (invalidp || !offenderp) ਵਾਪਸ invalidp;

        UASSERT_OBJ(m_embeddedVarp, m_covergroupp, "Embedded covergroup variable not found");
        // Commit: add the back-pointer member, rewrite the references, initialize the handle.
        File_ਸਤਰ* ਅਚਲ fl = m_covergroupp->fileline();
        AstClassRefDType* ਅਚਲ enclDTypep = ਨਵਾਂ AstClassRefDType{fl, m_enclosingClassp, ਖਾਲੀ_ਸੂਚਕ};
        enclDTypep->raw_ਸੂਚਕ(ਸੱਚ);
        v3Global.rootp()->typeTablep()->ਜੋੜੋ_Typesp(enclDTypep);
        AstVar* ਅਚਲ handleVarp
            = ਨਵਾਂ AstVar{fl, VVarType::MEMBER, "__Vcg_enclosingp", enclDTypep};
        m_covergroupp->ਜੋੜੋ_Membersp(handleVarp);
        AstVar* ਅਚਲ argumentp = ਨਵਾਂ AstVar{fl, VVarType::BLOCKTEMP, "__Vcg_parentp", enclDTypep};
        argumentp->direction(VDirection::ਨਿਵੇਸ਼);
        argumentp->declDirection(VDirection::ਨਿਵੇਸ਼);
        argumentp->funcLocal(ਸੱਚ);
        argumentp->noReset(ਸੱਚ);
        argumentp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
        m_constructorp->ਜੋੜੋ_Stmtsp(argumentp);
        m_constructorp->prependStmtsp(ਨਵਾਂ Ast_ਸੌਂਪੋ{fl, memberRef(fl, handleVarp, VAccess::WRITE),
                                                    ਨਵਾਂ AstVarRef{fl, argumentp, VAccess::READ}});

        // Route each enclosing-member reference through the back-pointer: 'm' -> 'h.m'.
        ਲਈ (AstVarRef* ਅਚਲ refp : refs_ਮੰਜ਼ਿਲ_Rewrite) { rewriteVarRef(refp, handleVarp); }
        ਲਈ (AstThisRef* ਅਚਲ refp : this_Refs_ਮੰਜ਼ਿਲ_Rewrite) { rewriteThisRef(refp, handleVarp); }
        ਲਈ (AstFuncRef* ਅਚਲ refp : func_Refs_ਮੰਜ਼ਿਲ_Rewrite) { rewriteFuncRef(refp, handleVarp); }

        // Append a named hidden argument to preserve positional and defaulted user arguments.
        ਲਈ (Ast_ਗੰਢ_ਸੌਂਪੋ* ਅਚਲ constructp : constructps) {
            File_ਸਤਰ* ਅਚਲ cfl = constructp->fileline();
            AstCExpr* ਅਚਲ thisp = ਨਵਾਂ AstCExpr{cfl, "this"};
            thisp->dtypep(enclDTypep);
            VN_AS(constructp->rhsp(), New)->ਜੋੜੋ_Argsp(ਨਵਾਂ AstArg{cfl, argumentp->name(), thisp});
        }
        ਵਾਪਸ ਖਾਲੀ_ਸੂਚਕ;
    }

    ਖਾਲੀ ਦੇਖੋ(AstClass* nodep) override {
        UINFO(9, "Visiting class: " << nodep->name() << " isCovergroup=" << nodep->isCovergroup());
        ਜੇ (nodep->isCovergroup()) {
            VL_RESTORER(m_covergroupp);
            VL_RESTORER(m_embeddedVarp);
            VL_RESTORER(m_sampleFuncp);
            VL_RESTORER(m_constructorp);
            VL_RESTORER_CLEAR(m_coverpoints);
            VL_RESTORER_CLEAR(m_coverpointMap);
            VL_RESTORER_CLEAR(m_coverCrosses);
            VL_RESTORER_CLEAR(m_cgOptions);
            VL_RESTORER_CLEAR(m_covergroupName);
            m_covergroupp = nodep;
            m_embeddedVarp = ਲੱਭੋ_Embedded_Covergroup_Var();
            m_covergroupName = covergroupTypeName();
            m_sampleFuncp = ਖਾਲੀ_ਸੂਚਕ;
            m_constructorp = ਖਾਲੀ_ਸੂਚਕ;
            std::ਸਦਿਸ਼<EmbeddedEventTrigger> embeddedEventTriggers;
            Ast_Sen_ਰੁੱਖ* embeddedEventForkp = ਖਾਲੀ_ਸੂਚਕ;

            // Extract and store the clocking event from AstCovergroup node
            // The parser creates this node to preserve the event information
            ਤਾਰਕਿਕ hasUnsupportedEvent = ਝੂਠ;
            ਲਈ (Ast_ਗੰਢ* itemp = nodep->membersp(); itemp;) {
                Ast_ਗੰਢ* ਅਚਲ nextp = itemp->nextp();
                ਜੇ (AstCovergroup* ਅਚਲ cgp = VN_CAST(itemp, Covergroup)) {
                    // Store the event in the global map for V3Active to retrieve later
                    // V3LinkParse only creates this sentinel AstCovergroup node when a clocking
                    // event exists, so cgp->eventp() is always non-null here.
                    UASSERT_OBJ(cgp->eventp(), cgp,
                                "Sentinel AstCovergroup in class must have non-null eventp");
                    ਜੇ (hasEnclosingEventRef(cgp)) {
                        UASSERT_OBJ(m_embeddedVarp, cgp,
                                    "Embedded covergroup event has no instance variable");
                        ਜੇ (v3Global.opt.timing().is_ਸੈੱਟ_True()) {
                            embeddedEventForkp = cgp->eventp()->unlink_Fr_ਪਿੱਠ();
                        } ਨਹੀਂਤਾਂ {
                            embeddedEventTriggers = collectEmbeddedEventTriggers(cgp);
                            ਜੇ (embeddedEventTriggers.ਖਾਲੀ_ਥਾਵਾਂ()) {
                                cgp->v3warn(COVERIGN,
                                            "Unsupported: 'covergroup' clocking event on complex "
                                            "member expression; use --timing for full support.");
                                hasUnsupportedEvent = ਸੱਚ;
                            }
                        }
                        VL_DO_DANGLING(ਧੱਕੋ_Deletep(cgp->unlink_Fr_ਪਿੱਠ()), cgp);
                        itemp = nextp;
                        ਜਾਰੀ;
                    }
                    // V3Active handles events that do not depend on an enclosing instance.
                    UINFO(4, "Keeping covergroup event node for V3Active: " << nodep->name());
                    itemp = nextp;
                    ਜਾਰੀ;
                }
                itemp = nextp;
            }

            // Find the sample() method and constructor
            m_sampleFuncp = VN_CAST(m_memberMap.ਲੱਭੋ_Member(nodep, "sample"), Func);
            // V3LinkParse always synthesizes a sample() method for every covergroup, and the
            // sampling-code generation below dereferences m_sampleFuncp unconditionally.
            UASSERT_OBJ(m_sampleFuncp, nodep, "Covergroup missing synthesized sample() method");
            m_sampleFuncp->isCovergroupSample(ਸੱਚ);
            m_constructorp = VN_CAST(m_memberMap.ਲੱਭੋ_Member(nodep, "new"), Func);
            UINFO(9, "Found sample() method: " << (m_sampleFuncp ? "yes" : "no"));
            UINFO(9, "Found constructor: " << (m_constructorp ? "yes" : "no"));

            // If covergroup has unsupported clocking event, skip processing it
            // but still clean up coverpoints so they don't reach downstream passes
            ਜੇ (hasUnsupportedEvent) {
                iterateChildren(nodep);
                validateCovergroupExpressions();
                rebindFormalRefs();
                delete_Coverage_ਚੀਜ਼ਾਂ();
                ਜੇ (embeddedEventForkp) {
                    VL_DO_DANGLING(ਧੱਕੋ_Deletep(embeddedEventForkp), embeddedEventForkp);
                }
                ਵਾਪਸ;
            }

            iterateChildren(nodep);
            validateCovergroupExpressions();
            rebindFormalRefs();
            ਅਚਲ std::ਸਦਿਸ਼<Ast_ਗੰਢ_ਸੌਂਪੋ*> constructps = ਲੱਭੋ_Covergroup_Constructions();

            // Embedded covergroups (IEEE 1800-2023 19.4): coverpoints, iff expressions, and
            // crosses may reference members of the enclosing class. The covergroup is lowered
            // into a sibling class with no implicit handle to the enclosing instance. Install
            // an explicit back-pointer and route the references through it.
            ਜੇ (AstVarRef* ਅਚਲ invalidp = install_Enclosing_ਪਿੱਠ_ਸੂਚਕ(constructps)) {
                invalidp->v3error("Non-static member "
                                  << invalidp->varp()->prettyNameQ()
                                  << " of an outer class requires an explicit "
                                     "object handle (IEEE 1800-2023 8.23).");
                delete_Coverage_ਚੀਜ਼ਾਂ();
                ਜੇ (embeddedEventForkp) {
                    VL_DO_DANGLING(ਧੱਕੋ_Deletep(embeddedEventForkp), embeddedEventForkp);
                }
                ਵਾਪਸ;
            }
            installEmbeddedEventTriggers(embeddedEventTriggers, constructps);
            ਜੇ (embeddedEventForkp) installEmbeddedEventFork(embeddedEventForkp, constructps);
            processCovergroup();
            // Remove lowered coverpoints/crosses from the class - they have been
            // fully translated into C++ code and must not reach downstream passes
            delete_Coverage_ਚੀਜ਼ਾਂ();
        } ਨਹੀਂਤਾਂ {
            // Track the lexically enclosing class so a nested covergroup can resolve
            // references to the enclosing object's members (installEnclosingBackPointer).
            VL_RESTORER(m_enclosingClassp);
            m_enclosingClassp = nodep;
            iterateChildren(nodep);
        }
    }

    ਖਾਲੀ ਦੇਖੋ(AstCoverpoint* nodep) override {
        UINFO(9, "Found coverpoint: " << nodep->name());
        m_coverpoints.ਧੱਕੋ_ਪਿੱਠ(nodep);
        m_coverpointMap.emplace(nodep->name(), nodep);
        iterateChildren(nodep);
    }

    ਖਾਲੀ ਦੇਖੋ(AstCoverCross* nodep) override {
        UINFO(9, "Found cross: " << nodep->name());
        m_coverCrosses.ਧੱਕੋ_ਪਿੱਠ(nodep);
        iterateChildren(nodep);
    }

    // V3Width leaves only the covergroup-level options lowerCovergroupOptions() takes
    ਖਾਲੀ ਦੇਖੋ(Ast_Cg_Option_ਸੌਂਪੋ* nodep) override { m_cgOptions.ਧੱਕੋ_ਪਿੱਠ(nodep); }

    // A package, interface, or module, so the design unit declaring the classes within
    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ_Module* nodep) override {
        VL_RESTORER(m_unitp);
        m_unitp = nodep;
        iterateChildren(nodep);
    }

    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ* nodep) override { iterateChildren(nodep); }

ਜਨਤਕ:
    // CONSTRUCTORS
    FunctionalCoverageVisitor(AstNetlist* nodep,
                              ਅਚਲ std::ਸੈੱਟ<ਅਚਲ AstVar*>& mergeableTypeOptions)
        : m_mergeableTypeOptions{mergeableTypeOptions} {
        iterate(nodep);
    }
    ~FunctionalCoverageVisitor() override = ਮੂਲ;
};

//######################################################################
// Functional coverage class functions

ਖਾਲੀ V3Covergroup::covergroup(AstNetlist* nodep) {
    UINFO(4, __FUNCTION__ << ": ");
    ਅਚਲ Covergroup_ਸੌਂਪੋ_Valid_Visitor validVisitor{nodep};
    ਜੇ (!validVisitor.valid()) V3Error::abortIfErrors();
    {  // Destruct before checking
        FunctionalCoverageVisitor{nodep, validVisitor.mergeableTypeOptions()};
    }
    V3Global::dump_Check_Global_ਰੁੱਖ("coveragefunc", 0, dump_ਰੁੱਖ_Either_Level() >= 3);
}
