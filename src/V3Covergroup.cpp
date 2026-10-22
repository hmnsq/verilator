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

#ಅನ್ತರ್ಭಾವ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ಅನ್ತರ್ಭಾವ "V3Covergroup.h"

#ಅನ್ತರ್ಭಾವ "V3Const.h"
#ಅನ್ತರ್ಭಾವ "V3Error.h"
#ಅನ್ತರ್ಭಾವ "V3File.h"
#ಅನ್ತರ್ಭಾವ "V3MemberMap.h"
#ಅನ್ತರ್ಭಾವ "V3UniqueNames.h"

#ಅನ್ತರ್ಭಾವ <bitset>
#ಅನ್ತರ್ಭಾವ <cmath>
#ಅನ್ತರ್ಭಾವ <deque>
#ಅನ್ತರ್ಭಾವ <set>
#ಅನ್ತರ್ಭಾವ <tuple>
#ಅನ್ತರ್ಭಾವ <unordered_map>
#ಅನ್ತರ್ಭಾವ <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Embedded covergroup assignment validation; and in the same pass over the netlist, the
// type_option variables through which SystemVerilog may set type_option.merge_instances

ವರ್ಗ Covergroup_ನಿಯೋಜಿಸು_Valid_Visitor final : ಸಾರ್ವಜನಿಕ VNVisitorConst {
    VMemberMap m_memberMap;
    std::map<ಸ್ಥಿರಾಂಕ AstVar*, ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_FTask*>
        m_constructors;  // Implicit instance -> constructor
    std::ಗಣ<ಸ್ಥಿರಾಂಕ AstVar*> m_mergeableTypeOptions;  // See mergeableTypeOptions()
    ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_FTask* m_ftaskp = ಶೂನ್ಯಸೂಚಕ;
    ತಾರ್ಕಿಕ m_collecting = ನಿಜ;
    ತಾರ್ಕಿಕ m_valid = ನಿಜ;

    ಶೂನ್ಯ ಭೇಟಿ(AstClass* nodep) override {
        VL_RESTORER(m_ftaskp);
        m_ftaskp = ಶೂನ್ಯಸೂಚಕ;
        ಆದರೆ (m_collecting) {
            ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_FTask* ಸ್ಥಿರಾಂಕ constructorp
                = VN_CAST(m_memberMap.ಕಂಡುಹಿಡಿ_Member(nodep, "new"), ಗಂಟು_FTask);
            ಗಾಗಿ (ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* itemp = nodep->membersp(); itemp; itemp = itemp->nextp()) {
                ಸ್ಥಿರಾಂಕ AstVar* ಸ್ಥಿರಾಂಕ varp = VN_CAST(itemp, Var);
                // Only the implicit instance is restricted, not explicitly typed aliases.
                ಆದರೆ (!varp || !varp->isClassMember() || varp->isDeclTyped()) ಮುಂದುವರಿ;
                ಸ್ಥಿರಾಂಕ AstClassRefDType* ಸ್ಥಿರಾಂಕ refp
                    = VN_CAST(varp->dtypep()->skipRefp(), ClassRefDType);
                ಆದರೆ (refp && refp->classp()->covergroupEnclosingClassp() == nodep) {
                    m_constructors.emplace(varp, constructorp);
                }
            }
        }
        iterateChildrenConst(nodep);
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು_FTask* nodep) override {
        VL_RESTORER(m_ftaskp);
        m_ftaskp = nodep;
        iterateChildrenConst(nodep);
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು_ನಿಯೋಜಿಸು* nodep) override {
        ಆದರೆ (!m_collecting) {
            ಸ್ಥಿರಾಂಕ AstVar* varp = ಶೂನ್ಯಸೂಚಕ;
            ಆದರೆ (ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_Var_Ref* ಸ್ಥಿರಾಂಕ refp = VN_CAST(nodep->lhsp(), ಗಂಟು_Var_Ref)) {
                varp = refp->varp();
            } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (ಸ್ಥಿರಾಂಕ AstMemberSel* ಸ್ಥಿರಾಂಕ selp = VN_CAST(nodep->lhsp(), MemberSel)) {
                varp = selp->varp();
            }
            ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ it = m_constructors.ಕಂಡುಹಿಡಿ(varp);
            ಆದರೆ (it != m_constructors.ಕೊನೆ() && (!m_ftaskp || m_ftaskp != it->ಎರಡನೇ)) {
                m_valid = ಸುಳ್ಳು;
                nodep->v3error("Embedded covergroup variable "
                               << varp->prettyNameQ()
                               << " may only be assigned in the enclosing class's 'new' method "
                                  "(IEEE 1800-2023 19.4).");
            }
        }
        iterateChildrenConst(nodep);
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು_Var_Ref* nodep) override {
        // Type options may be set at any time (IEEE 1800-2023 19.7.1), so SystemVerilog may set
        // type_option.merge_instances with any use of a type_option but a read of another
        // member, as not every write is marked yet (std::randomize)
        ಆದರೆ (m_collecting && nodep->varp()->name() == "type_option") {
            ಸ್ಥಿರಾಂಕ AstStructSel* ಸ್ಥಿರಾಂಕ selp = VN_CAST(nodep->backp(), StructSel);
            ಆದರೆ (!selp || selp->fromp() != nodep || selp->name() == "merge_instances"
                || !nodep->access().isReadOnly()) {
                m_mergeableTypeOptions.ಸೇರಿಸು(nodep->varp());
            }
        }
        iterateChildrenConst(nodep);
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು* nodep) override { iterateChildrenConst(nodep); }

ಸಾರ್ವಜನಿಕ:
    ಸ್ಪಷ್ಟ Covergroup_ನಿಯೋಜಿಸು_Valid_Visitor(AstNetlist* nodep) {
        // Uses can precede their enclosing class in the tree.
        iterateConst(nodep);
        m_collecting = ಸುಳ್ಳು;
        ಆದರೆ (!m_constructors.ಖಾಲಿಗಳು()) iterateConst(nodep);
    }
    ತಾರ್ಕಿಕ valid() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_valid; }
    // type_option variables through which SystemVerilog may set type_option.merge_instances, so
    // that covergroups are tested with optionVar(true)
    ಸ್ಥಿರಾಂಕ std::ಗಣ<ಸ್ಥಿರಾಂಕ AstVar*>& mergeableTypeOptions() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_mergeableTypeOptions; }
};

//######################################################################
// Covergroup expression validation visitor

ವರ್ಗ Covergroup_ಅಭಿವ್ಯಕ್ತಿ_Valid_Visitor final : ಸಾರ್ವಜನಿಕ VNVisitor {
    ಸ್ಥಿರಾಂಕ std::ಗಣ<ಸ್ಥಿರಾಂಕ AstVar*>& m_sampleMembers;
    ಸ್ಥಿರಾಂಕ std::ಗಣ<ಸ್ಥಿರಾಂಕ AstVar*>& m_constructorRefMembers;
    ತಾರ್ಕಿಕ m_ಒಳ_Coverage_ಅಭಿವ್ಯಕ್ತಿ = ಸುಳ್ಳು;
    ತಾರ್ಕಿಕ m_sampleFormalAllowed = ಸುಳ್ಳು;

    ಶೂನ್ಯ ಪರಿಶೀಲನೆ_Sample_ಅಭಿವ್ಯಕ್ತಿ(Ast_ಗಂಟು* nodep) {
        ಆದರೆ (!nodep) ಹಿಂತಿರುಗಿ;
        VL_RESTORER(m_sampleFormalAllowed);
        m_sampleFormalAllowed = ನಿಜ;
        iterate_And_ಮುಂದಿನ_Null(nodep);
    }

    ಶೂನ್ಯ ಪರಿಶೀಲನೆ_Coverage_ಅಭಿವ್ಯಕ್ತಿ(Ast_ಗಂಟು* nodep) {
        ಆದರೆ (!nodep) ಹಿಂತಿರುಗಿ;
        VL_RESTORER(m_ಒಳ_Coverage_ಅಭಿವ್ಯಕ್ತಿ);
        m_ಒಳ_Coverage_ಅಭಿವ್ಯಕ್ತಿ = ನಿಜ;
        iterate_And_ಮುಂದಿನ_Null(nodep);
    }

    ಶೂನ್ಯ ಭೇಟಿ(AstCoverpoint* nodep) override {
        ಪರಿಶೀಲನೆ_Sample_ಅಭಿವ್ಯಕ್ತಿ(nodep->exprp());
        ಪರಿಶೀಲನೆ_Sample_ಅಭಿವ್ಯಕ್ತಿ(nodep->iffp());
        iterate_And_ಮುಂದಿನ_Null(nodep->binsp());
        iterate_And_ಮುಂದಿನ_Null(nodep->optionsp());
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstCoverCross* nodep) override {
        iterate_And_ಮುಂದಿನ_Null(nodep->itemsp());
        ಪರಿಶೀಲನೆ_Sample_ಅಭಿವ್ಯಕ್ತಿ(nodep->iffp());
        iterate_And_ಮುಂದಿನ_Null(nodep->optionsp());
        iterate_And_ಮುಂದಿನ_Null(nodep->binsp());
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstCoverCrossBin* nodep) override {
        iterate_And_ಮುಂದಿನ_Null(nodep->selectp());
        ಪರಿಶೀಲನೆ_Sample_ಅಭಿವ್ಯಕ್ತಿ(nodep->iffp());
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstCoverCrossWith* nodep) override {
        iterate_And_ಮುಂದಿನ_Null(nodep->selectp());
        ಪರಿಶೀಲನೆ_Coverage_ಅಭಿವ್ಯಕ್ತಿ(nodep->filterp());
        ಪರಿಶೀಲನೆ_Coverage_ಅಭಿವ್ಯಕ್ತಿ(nodep->countp());
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstCoverBinsof* nodep) override { ಪರಿಶೀಲನೆ_Coverage_ಅಭಿವ್ಯಕ್ತಿ(nodep->rangesp()); }
    ಶೂನ್ಯ ಭೇಟಿ(AstCoverBin* nodep) override {
        ಪರಿಶೀಲನೆ_Coverage_ಅಭಿವ್ಯಕ್ತಿ(nodep->rangesp());
        ಪರಿಶೀಲನೆ_Sample_ಅಭಿವ್ಯಕ್ತಿ(nodep->iffp());
        ಪರಿಶೀಲನೆ_Coverage_ಅಭಿವ್ಯಕ್ತಿ(nodep->arraySizep());
        ಪರಿಶೀಲನೆ_Coverage_ಅಭಿವ್ಯಕ್ತಿ(nodep->transp());
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstVarRef* nodep) override {
        ಆದರೆ (!m_sampleFormalAllowed && m_sampleMembers.ಎಣಿಕೆ_2(nodep->varp())) {
            nodep->v3error("Covergroup sample formal argument "
                           << nodep->varp()->prettyNameQ()
                           << " may only be used in a coverpoint or conditional guard "
                              "expression (IEEE 1800-2023 19.8.1).");
        }
        ಆದರೆ (m_ಒಳ_Coverage_ಅಭಿವ್ಯಕ್ತಿ && m_constructorRefMembers.ಎಣಿಕೆ_2(nodep->varp())) {
            nodep->v3error("Ref covergroup constructor formal argument "
                           << nodep->varp()->prettyNameQ()
                           << " may not be used in a covergroup expression "
                              "(IEEE 1800-2023 19.5).");
        }
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು_FTask_Ref* nodep) override {
        ಆದರೆ (m_ಒಳ_Coverage_ಅಭಿವ್ಯಕ್ತಿ && nodep->taskp()) {
            ತಾರ್ಕಿಕ invalidDirection = ಸುಳ್ಳು;
            ಗಾಗಿ (Ast_ಗಂಟು* stmtp = nodep->taskp()->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
                ಸ್ಥಿರಾಂಕ AstVar* ಸ್ಥಿರಾಂಕ varp = VN_CAST(stmtp, Var);
                ಆದರೆ (varp && varp->isIO() && varp->isWritable()) {
                    invalidDirection = ನಿಜ;
                    ನಿಲ್ಲಿಸು;
                }
            }
            ಆದರೆ (invalidDirection) {
                nodep->v3error("Function " << nodep->taskp()->prettyNameQ()
                                           << " called in a covergroup expression has an "
                                              "output, inout, or non-const ref argument "
                                              "(IEEE 1800-2023 19.5).");
            }
        }
        iterateChildren(nodep);
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು* nodep) override { iterateChildren(nodep); }

ಸಾರ್ವಜನಿಕ:
    Covergroup_ಅಭಿವ್ಯಕ್ತಿ_Valid_Visitor(ಸ್ಥಿರಾಂಕ std::ಗಣ<ಸ್ಥಿರಾಂಕ AstVar*>& sampleMembers,
                               ಸ್ಥಿರಾಂಕ std::ಗಣ<ಸ್ಥಿರಾಂಕ AstVar*>& constructorRefMembers)
        : m_sampleMembers{sampleMembers}
        , m_constructorRefMembers{constructorRefMembers} {}
    ಶೂನ್ಯ ಪರಿಶೀಲನೆ(Ast_ಗಂಟು* nodep) { iterate(nodep); }
};

//######################################################################
// Bins of one declaration whose values are computed rather than listed: bin k covers
// [m_lo + k * m_stride, m_lo + (k + 1) * m_stride - 1], and the last bin extends to m_hi.  An
// array bin element is a run of single-value bins; automatic bins partition the coverpoint
// domain.  Bounds are coverpoint values at FunctionalCoverageVisitor::runWidth(),
// sign-extended like a CrossValueRange's.

ವರ್ಗ Bin_ಸತತ final {
ಸಾರ್ವಜನಿಕ:
    // MEMBERS
    uint32_t m_ಎಣಿಕೆ;  // Number of bins
    V3_ಸಂಖ್ಯೆ m_lo;  // Lowest value of the first bin
    V3_ಸಂಖ್ಯೆ m_stride;  // Number of values of each bin but the last
    V3_ಸಂಖ್ಯೆ m_hi;  // Highest value of the last bin
    ತಾರ್ಕಿಕ m_ಖಾಲಿಗಳು = ಸುಳ್ಳು;  // A single bin without a value of the coverpoint type
    uint32_t m_declared = 0;  // Runtime index of the first bin, once generated

    // CONSTRUCTORS
    Bin_ಸತತ(Ast_ಗಂಟು* nodep, ಪೂರ್ಣಾಂಕ ಅಗಲ, uint32_t ಎಣಿಕೆ_2)
        : m_ಎಣಿಕೆ{ಎಣಿಕೆ_2}
        , m_lo{nodep, ಅಗಲ}
        , m_stride{nodep, ಅಗಲ, 1}
        , m_hi{nodep, ಅಗಲ} {}
};

//######################################################################
// Functional coverage visitor

ವರ್ಗ FunctionalCoverageVisitor final : ಸಾರ್ವಜನಿಕ VNVisitor {
    // NODE STATE
    // Entire netlist:
    //  AstCoverpoint::user1p()  -> AstVar*.  Previous-value variable for transition bins
    //  AstCoverpoint::user2()   -> bool.  Had a bins declaration ignored, so no automatic bins
    ಸ್ಥಿರಾಂಕ VNUser1_ಒಳ_Use m_inuser1;
    ಸ್ಥಿರಾಂಕ VNUser2_ಒಳ_Use m_inuser2;

    // STATE
    std::ಗಣ<AstCoverpoint*>
        m_runtimePoints;  // Points needing value metadata and live-bin mapping
    std::ಗಣ<AstCoverCross*> m_runtimeCrosses;  // Crosses over finalized live-bin dimensions
    std::map<AstVar*, AstVar*> m_excludedVars;  // Sample-time state-exclusion flags
    AstClass* m_covergroupp = ಶೂನ್ಯಸೂಚಕ;  // Current covergroup being processed
    Ast_ಗಂಟು_Module* m_unitp = ಶೂನ್ಯಸೂಚಕ;  // Design unit declaring the current class, outside classes
    AstClass* m_enclosingClassp = ಶೂನ್ಯಸೂಚಕ;  // Class lexically enclosing the covergroup, if any
    AstVar* m_embeddedVarp = ಶೂನ್ಯಸೂಚಕ;  // Embedded covergroup member of m_enclosingClassp, if any
    std::string m_covergroupName;  // Current covergroup's type name, see covergroupTypeName()
    AstFunc* m_sampleFuncp = ಶೂನ್ಯಸೂಚಕ;  // Current sample() function
    AstIf* m_cpGuardp = ಶೂನ್ಯಸೂಚಕ;  // Current coverpoint's iff guard, holding its sampling
    AstFunc* m_constructorp = ಶೂನ್ಯಸೂಚಕ;  // Current constructor
    std::ಸದಿಶ<AstCoverpoint*> m_coverpoints;  // Coverpoints in current covergroup
    std::map<std::string, AstCoverpoint*> m_coverpointMap;  // Name -> coverpoint for fast lookup
    std::ಸದಿಶ<AstCoverCross*> m_coverCrosses;  // Cross coverage items in current covergroup
    std::ಸದಿಶ<Ast_Cg_Option_ನಿಯೋಜಿಸು*> m_cgOptions;  // Covergroup-level options, before lowering
    uint32_t m_cg_Type_ತೂಕ = 1;  // The covergroup's type_option.weight, a constant
    ತಾರ್ಕಿಕ m_cg_ವಿಲೀನ_Instances = ಸುಳ್ಳು;  // The covergroup's type_option.merge_instances, a constant
    // The covergroup's type_option.merge_instances is, or SystemVerilog may make it, true
    ತಾರ್ಕಿಕ m_cg_May_ವಿಲೀನ = ಸುಳ್ಳು;
    // type_option members through which SystemVerilog may set type_option.merge_instances,
    // see CovergroupAssignValidVisitor::mergeableTypeOptions()
    ಸ್ಥಿರಾಂಕ std::ಗಣ<ಸ್ಥಿರಾಂಕ AstVar*>& m_mergeableTypeOptions;
    ಪೂರ್ಣಾಂಕ m_cg_At_ಕನಿಷ್ಠ = 1;  // Covergroup option.at_least

    ರಚನೆ EmbeddedEventTrigger final {
        File_ಸಾಲು* eventFl;  // Clocking-event source location
        AstVar* ತಳ_Varp;  // Base enclosing-class member in the event expression
        AstVar* memberVarp;  // Selected member in a 'base.member' expression, or nullptr
        VEdgeType ಅಂಚು_Type;  // Clocking-event edge qualifier
        AstVar* prevVarp;  // Member containing the previous event value, or nullptr
        EmbeddedEventTrigger(File_ಸಾಲು* eventFl, AstVar* ತಳ_Varp, AstVar* memberVarp,
                             VEdgeType ಅಂಚು_Type)
            : eventFl{eventFl}
            , ತಳ_Varp{ತಳ_Varp}
            , memberVarp{memberVarp}
            , ಅಂಚು_Type{ಅಂಚು_Type}
            , prevVarp{ಶೂನ್ಯಸೂಚಕ} {}
    };

    std::ಗಣ<std::string> m_crossed_Cp_ಹೆಸರುಗಳು;  // Coverpoints referenced by a cross
    std::map<std::string, AstVar*> m_cpVarMap;  // Coverpoint name -> its VlCoverpoint member
    ರಚನೆ BinRuns final {
        std::ಸದಿಶ<Bin_ಸತತ> runs;  // Runs of an array or automatic bins declaration, in order
        uint32_t ಎಣಿಕೆ_2 = 0;  // Bins across all runs
        ತಾರ್ಕಿಕ unsupported = ಸುಳ್ಳು;  // Too many bins, or invalid: the declaration is ignored
        // The value of each bin, which names it, of a wildcard array; else the bins are indexed
        std::ಸದಿಶ<std::string> ಮೌಲ್ಯಗಳು;
        // Too many values of an ignore or illegal wildcard array, which is then one bin
        ತಾರ್ಕಿಕ single = ಸುಳ್ಳು;
    };
    ರಚನೆ Cross_Bin_ಮೌಲ್ಯಗಳು final {
        AstCoverBin* binp;  // Declaration owning this Normal bin
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* valuep;  // Individual array-bin value, or nullptr for a scalar bin
        ಸ್ಥಿರಾಂಕ Bin_ಸತತ* runp = ಶೂನ್ಯಸೂಚಕ;  // Run computing the bin's values, if any
        uint32_t element = 0;  // Index of the bin within runp
    };
    ರಚನೆ Bin_ಹರವು final {
        uint32_t ಮೊದಲ = 0;  // First Normal index of the bin declaration
        uint32_t ಎಣಿಕೆ_2 = 0;  // Number of Normal bins of the declaration
        uint32_t declared = 0;  // First runtime bin index, across all bin kinds
        int32_t sized = -1;  // Index of the sized array, placed at construction; or -1
    };
    ರಚನೆ CoverpointBins final {
        uint32_t ಒಟ್ಟು = 0;  // Number of Normal bins
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* exprp = ಶೂನ್ಯಸೂಚಕ;  // Sampled expression, for the value domain
        ತಾರ್ಕಿಕ crossed = ಸುಳ್ಳು;  // Feeds a cross, which needs 'values'
        std::ಸದಿಶ<Cross_Bin_ಮೌಲ್ಯಗಳು> ಮೌಲ್ಯಗಳು;  // Values in runtime Normal-bin index order
        std::unordered_map<std::string, Bin_ಹರವು> spans;  // Declared bin name -> index span
        Bin_ಹರವು implicitAuto{0, 0, 0};  // Implicit automatic bins, each named 'auto_<i>'
        std::deque<Bin_ಸತತ> runs;  // Runs 'values' refers to
    };
    std::map<AstVar*, CoverpointBins> m_cpBins;  // Runtime coverpoint -> binsof index ranges
    // Names of the bins declarations each coverpoint ignored, which binsof selects as no bins
    std::map<ಸ್ಥಿರಾಂಕ AstCoverpoint*, std::ಸದಿಶ<std::string>> m_droppedBins;
    // Prefixes of the constructor temporaries of constructed bins, as coverpoint and bin names
    // alone may repeat: coverpoint 'a_' bins 'b', and coverpoint 'a' bins '_b'
    V3_Unique_ಹೆಸರುಗಳು m_sized_ಹೆಸರುಗಳು{"__Vsized"};
    std::ಸದಿಶ<Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ*> m_detached_ಮೌಲ್ಯಗಳು;  // Array-bin values m_cpBins refers to
    std::ಗಣ<AstCoverCross*>
        m_droppedCrosses;  // Crosses with a bare-variable item: drop (COVERIGN)
    std::map<uint32_t, AstCoverpointDType*> m_cpDTypes;  // Hit-list bound -> interned dtype
    ಬಳಸಿ CrossShape = std::tuple<uint32_t, uint32_t, uint32_t, uint32_t, uint64_t, ತಾರ್ಕಿಕ>;
    std::map<CrossShape, AstCoverCrossDType*> m_cxDTypes;
    AstVar* m_cgInstVarp = ಶೂನ್ಯಸೂಚಕ;  // __Vcg_inst handle member of the current covergroup

    VMemberMap m_memberMap;  // Member names cached for fast lookup

    // METHODS
    // The covergroup's 'option' or static 'type_option' member (V3LinkParse creates both)
    AstVar* optionVar(ತಾರ್ಕಿಕ typeOption) {
        AstVar* ಸ್ಥಿರಾಂಕ varp = VN_AS(
            m_memberMap.ಕಂಡುಹಿಡಿ_Member(m_covergroupp, typeOption ? "type_option" : "option"), Var);
        UASSERT_OBJ(varp, m_covergroupp, "Covergroup missing option member");
        ಹಿಂತಿರುಗಿ varp;
    }

    // 'option.<member>' or 'type_option.<member>', per optionVarp
    AstStructSel* newOptionSel(File_ಸಾಲು* fl, AstVar* optionVarp, ಸ್ಥಿರಾಂಕ std::string& member,
                               VAccess access) {
        ಸ್ಥಿರಾಂಕ AstMemberDType* ಸ್ಥಿರಾಂಕ memberp
            = VN_AS(m_memberMap.ಕಂಡುಹಿಡಿ_Member(optionVarp->dtypep()->skipRefp(), member), MemberDType);
        UASSERT_OBJ(memberp, optionVarp, "Coverage option structure missing '" << member << "'");
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ fromp = optionVarp->lifetime().isStatic()
                                       ? ಹೊಸ AstVarRef{fl, optionVarp, access}
                                       : memberRef(fl, optionVarp, access);
        AstStructSel* ಸ್ಥಿರಾಂಕ selp = ಹೊಸ AstStructSel{fl, fromp, member};
        selp->dtypep(memberp->subDTypep()->skip_Ref_ಗಮ್ಯ_Enump());
        selp->did_ಅಗಲ(ನಿಜ);
        ಹಿಂತಿರುಗಿ selp;
    }

    // A constant option.at_least; a negative count of hits has no meaning, so takes the default
    ಸ್ಥಿರ ಪೂರ್ಣಾಂಕ const_At_ಕನಿಷ್ಠ(ಸ್ಥಿರಾಂಕ AstConst* constp) {
        ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ ಮೌಲ್ಯ = constp->ಗಮ್ಯ_SInt();
        ಆದರೆ (ಮೌಲ್ಯ >= 0) ಹಿಂತಿರುಗಿ ಮೌಲ್ಯ;
        constp->v3warn(COVERIGN, "Ignoring negative 'option.at_least'; using 1");
        ಹಿಂತಿರುಗಿ 1;
    }

    // An option's constant value, or nullptr after a warning, as non-constant is unsupported
    ಸ್ಥಿರ ಸ್ಥಿರಾಂಕ AstConst* constOption(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* valuep, VCoverOptionType optType) {
        ಆದರೆ (ಸ್ಥಿರಾಂಕ AstConst* ಸ್ಥಿರಾಂಕ constp = VN_CAST(valuep, Const)) ಹಿಂತಿರುಗಿ constp;
        valuep->v3warn(COVERIGN, "Ignoring unsupported: non-constant 'option."
                                     << optType.ascii() << "'; using default value");
        ಹಿಂತಿರುಗಿ ಶೂನ್ಯಸೂಚಕ;
    }

    // Store the covergroup-level options (IEEE 1800-2023 19.7) where SystemVerilog and the
    // runtime read them.  The instance options, option.weight and option.get_inst_coverage,
    // are evaluated by the constructor, as are the other instance options; the type options,
    // type_option.weight and type_option.merge_instances, are constant, and initialize the
    // static member.  Without its own type_option.merge_instances, a covergroup has the
    // default of --coverage-merge-instances.  A constant option.at_least is the default of
    // the coverpoints and crosses.
    ಶೂನ್ಯ ಅಧೋ_Covergroup_Options() {
        ತಾರ್ಕಿಕ ವಿಲೀನ_ಗಣ = ಸುಳ್ಳು;  // The covergroup sets type_option.merge_instances
        m_cg_At_ಕನಿಷ್ಠ = 1;
        ಗಾಗಿ (Ast_Cg_Option_ನಿಯೋಜಿಸು* ಸ್ಥಿರಾಂಕ optp : m_cgOptions) {
            ಆದರೆ (optp->optType() == VCoverOptionType::AT_ಕನಿಷ್ಠ) {
                ಆದರೆ (ಸ್ಥಿರಾಂಕ AstConst* ಸ್ಥಿರಾಂಕ constp = constOption(optp->valuep(), optp->optType())) {
                    m_cg_At_ಕನಿಷ್ಠ = const_At_ಕನಿಷ್ಠ(constp);
                }
                VL_DO_DANGLING(ತಳ್ಳು_Deletep(optp->unlink_Fr_ಹಿಂಭಾಗ()), optp);
                ಮುಂದುವರಿ;
            }
            File_ಸಾಲು* ಸ್ಥಿರಾಂಕ fl = optp->fileline();
            // V3Width left the type options constant, and type_option.weight non-negative
            ಸ್ಥಿರಾಂಕ AstConst* ಸ್ಥಿರಾಂಕ constp = VN_CAST(optp->valuep(), Const);
            std::string member = "weight";
            ಆದರೆ (optp->optType() == VCoverOptionType::ವಿಲೀನ_INSTANCES) {
                member = "merge_instances";
                m_cg_ವಿಲೀನ_Instances = !constp->ಅಂಕೆ().isEqZero();
                ವಿಲೀನ_ಗಣ = ನಿಜ;
            } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (optp->optType() == VCoverOptionType::GET_INST_COVERAGE) {
                member = "get_inst_coverage";
            } ಇಲ್ಲದಿದ್ದರೆ {
                UASSERT_OBJ(optp->optType() == VCoverOptionType::ತೂಕ, optp,
                            "Unexpected covergroup option reaching V3Covergroup");
                ಆದರೆ (optp->typeOption()) m_cg_Type_ತೂಕ = constp->ಗಮ್ಯ_UInt();
            }
            Ast_ನಿಯೋಜಿಸು* ಸ್ಥಿರಾಂಕ assignp = ಹೊಸ Ast_ನಿಯೋಜಿಸು{
                fl, newOptionSel(fl, optionVar(optp->typeOption()), member, VAccess::WRITE),
                optp->valuep()->unlink_Fr_ಹಿಂಭಾಗ()};
            ಆದರೆ (optp->typeOption()) {
                m_covergroupp->ಕೂಡಿಸು_Membersp(ಹೊಸ AstInitialStatic{fl, assignp});
                VL_DO_DANGLING(ತಳ್ಳು_Deletep(optp->unlink_Fr_ಹಿಂಭಾಗ()), optp);
            } ಇಲ್ಲದಿದ್ದರೆ {
                optp->replaceWith(assignp);
                VL_DO_DANGLING(ತಳ್ಳು_Deletep(optp), optp);
            }
        }
        m_cgOptions.clear();
        ಆದರೆ (!ವಿಲೀನ_ಗಣ && v3Global.opt.coverage_ವಿಲೀನ_Instances()) {
            File_ಸಾಲು* ಸ್ಥಿರಾಂಕ fl = m_covergroupp->fileline();
            m_covergroupp->ಕೂಡಿಸು_Membersp(ಹೊಸ AstInitialStatic{
                fl, ಹೊಸ Ast_ನಿಯೋಜಿಸು{
                        fl, newOptionSel(fl, optionVar(ನಿಜ), "merge_instances", VAccess::WRITE),
                        ಹೊಸ AstConst{fl, AstConst::ಬಿಟ್_True{}}}});
            m_cg_ವಿಲೀನ_Instances = ನಿಜ;
        }
        m_cg_May_ವಿಲೀನ = m_cg_ವಿಲೀನ_Instances || m_mergeableTypeOptions.ಎಣಿಕೆ_2(optionVar(ನಿಜ));
    }

    // The weight of an item in the coverage database, which merges the instances: its
    // type_option.weight if the covergroup merges them too (IEEE 1800-2023 19.11.3); else its
    // option.weight if a constant, and so of every instance; else its type_option.weight
    uint32_t ಅಂಶ_Database_ತೂಕ(Ast_ಗಂಟು* optionsp) ಸ್ಥಿರಾಂಕ {
        ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* weightp = ಶೂನ್ಯಸೂಚಕ;  // The option.weight in effect
        uint32_t type_ತೂಕ = 1;
        ಗಾಗಿ (Ast_ಗಂಟು* nodep = optionsp; nodep; nodep = nodep->nextp()) {
            ಸ್ಥಿರಾಂಕ AstCoverOption* ಸ್ಥಿರಾಂಕ optp = VN_AS(nodep, CoverOption);
            ಆದರೆ (!(optp->optType() == VCoverOptionType::ತೂಕ)) ಮುಂದುವರಿ;
            // V3Width left type_option.weight a non-negative constant
            ಆದರೆ (optp->typeOption()) {
                type_ತೂಕ = VN_AS(optp->valuep(), Const)->ಗಮ್ಯ_UInt();
            } ಇಲ್ಲದಿದ್ದರೆ {
                weightp = optp->valuep();
            }
        }
        ಆದರೆ (m_cg_ವಿಲೀನ_Instances) ಹಿಂತಿರುಗಿ type_ತೂಕ;
        ಆದರೆ (!weightp) ಹಿಂತಿರುಗಿ 1;
        ಆದರೆ (ಸ್ಥಿರಾಂಕ AstConst* ಸ್ಥಿರಾಂಕ constp = VN_CAST(weightp, Const)) ಹಿಂತಿರುಗಿ constp->ಗಮ್ಯ_UInt();
        ಹಿಂತಿರುಗಿ type_ತೂಕ;
    }

    // Configure an item's option.weight, its weight in instance coverage (IEEE 1800-2023
    // 19.11), and, if the covergroup may merge its instances, its type_option.weight, its
    // weight in type coverage then (19.11.3).
    ಶೂನ್ಯ generate_ಅಂಶ_ತೂಕ(File_ಸಾಲು* fl, AstVar* ಅಂಶ_Varp, Ast_ಗಂಟು* optionsp) {
        ಗಾಗಿ (Ast_ಗಂಟು* nodep = optionsp; nodep; nodep = nodep->nextp()) {
            ಸ್ಥಿರಾಂಕ AstCoverOption* ಸ್ಥಿರಾಂಕ optp = VN_AS(nodep, CoverOption);
            ಆದರೆ (!(optp->optType() == VCoverOptionType::ತೂಕ)) ಮುಂದುವರಿ;
            ಆದರೆ (!optp->typeOption()) {
                m_constructorp->ಕೂಡಿಸು_Stmtsp(
                    ಅಂಶ_Call(fl, ಅಂಶ_Varp, VCMethod::COVERGROUP_ತೂಕ,
                             {optp->valuep()->clone_ವೃಕ್ಷ(ಸುಳ್ಳು), file_ಸಾಲು_Debug(optp->fileline())})
                        ->makeStmt());
            } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (m_cg_May_ವಿಲೀನ) {
                // V3Width left type_option.weight a non-negative constant
                m_constructorp->ಕೂಡಿಸು_Stmtsp(ಅಂಶ_Call(fl, ಅಂಶ_Varp, VCMethod::COVERGROUP_TYPE_ತೂಕ,
                                                   {optp->valuep()->clone_ವೃಕ್ಷ(ಸುಳ್ಳು)})
                                              ->makeStmt());
            }
        }
    }

    ಶೂನ್ಯ processCovergroup() {
        UINFO(4, "Processing covergroup: " << m_covergroupp->name() << " with "
                                           << m_coverpoints.ಗಾತ್ರ_2() << " coverpoints and "
                                           << m_coverCrosses.ಗಾತ್ರ_2() << " crosses");

        m_crossed_Cp_ಹೆಸರುಗಳು.clear();
        m_cpVarMap.clear();
        m_cpBins.clear();
        m_droppedBins.clear();
        m_sized_ಹೆಸರುಗಳು.reset();
        m_runtimePoints.clear();
        m_runtimeCrosses.clear();
        m_excludedVars.clear();
        m_droppedCrosses.clear();
        m_cgInstVarp = ಶೂನ್ಯಸೂಚಕ;
        m_cg_Type_ತೂಕ = 1;
        m_cg_ವಿಲೀನ_Instances = ಸುಳ್ಳು;
        m_cg_May_ವಿಲೀನ = ಸುಳ್ಳು;

        ಅಧೋ_Covergroup_Options();

        // Scan every cross item to record the coverpoints it references (the cross dimensions)
        // and to flag any cross naming a bare variable -- a would-be implicit coverpoint, which
        // Verilator does not synthesize.  An unresolvable item drops only that one cross (with a
        // COVERIGN in generateCrossCode), leaving the rest of the covergroup intact.
        ಗಾಗಿ (AstCoverCross* crossp : m_coverCrosses) {
            ಗಾಗಿ (Ast_ಗಂಟು* itemp = crossp->itemsp(); itemp; itemp = itemp->nextp()) {
                ಸ್ಥಿರಾಂಕ AstCoverpointRef* ಸ್ಥಿರಾಂಕ refp = VN_AS(itemp, CoverpointRef);
                ಆದರೆ (refp->exprp()) ಮುಂದುವರಿ;  // hierarchical ref: dropped in generateCrossCode
                ಆದರೆ (m_coverpointMap.ಕಂಡುಹಿಡಿ(refp->name()) == m_coverpointMap.ಕೊನೆ()) {
                    m_droppedCrosses.ಸೇರಿಸು(crossp);  // bare variable: drop this cross only
                } ಇಲ್ಲದಿದ್ದರೆ {
                    m_crossed_Cp_ಹೆಸರುಗಳು.ಸೇರಿಸು(refp->name());
                }
            }
        }

        std::ಸದಿಶ<Ast_ಗಂಟು*> pending;
        std::map<AstCoverpoint*, std::ಸದಿಶ<AstCoverCross*>> consumers;
        std::map<AstCoverCross*, std::ಸದಿಶ<AstCoverpoint*>> inputs;
        ಗಾಗಿ (AstCoverpoint* ಸ್ಥಿರಾಂಕ cpp : m_coverpoints) {
            check_Bin_ಹೆಸರುಗಳು(cpp);
            checkConstructedBins(cpp);
            ಆದರೆ (!cpp->exprp()->dtypep()->skipRefp()->isIntegralOrPacked()) ಮುಂದುವರಿ;
            // Bins without values leave the report (IEEE 1800-2023 19.11.1), exclusions or not.
            // Constructed bins get their values when the covergroup is constructed.
            ಆದರೆ (!coverpointHasStateExclusions(cpp) && !coverpoint_Has_ಖಾಲಿಗಳು_Bins(cpp)
                && !coverpointHasConstructedBins(cpp)) {
                ಮುಂದುವರಿ;
            }
            m_runtimePoints.ಸೇರಿಸು(cpp);
            pending.ತಳ್ಳು_ಹಿಂಭಾಗ(cpp);
        }
        ಗಾಗಿ (AstCoverCross* ಸ್ಥಿರಾಂಕ crossp : m_coverCrosses) {
            ಆದರೆ (m_droppedCrosses.ಎಣಿಕೆ_2(crossp)) ಮುಂದುವರಿ;
            ಗಾಗಿ (Ast_ಗಂಟು* itemp = crossp->itemsp(); itemp; itemp = itemp->nextp()) {
                ಸ್ಥಿರಾಂಕ AstCoverpointRef* ಸ್ಥಿರಾಂಕ refp = VN_AS(itemp, CoverpointRef);
                ಆದರೆ (refp->exprp()) ಮುಂದುವರಿ;
                ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ ಬಿಂದು = m_coverpointMap.ಕಂಡುಹಿಡಿ(refp->name());
                ಆದರೆ (ಬಿಂದು != m_coverpointMap.ಕೊನೆ()) {
                    consumers[ಬಿಂದು->ಎರಡನೇ].ತಳ್ಳು_ಹಿಂಭಾಗ(crossp);
                    inputs[crossp].ತಳ್ಳು_ಹಿಂಭಾಗ(ಬಿಂದು->ಎರಡನೇ);
                }
            }
            // The values of a filter's bin tuples are known when the covergroup is constructed
            ಆದರೆ (crossp->exists([](ಸ್ಥಿರಾಂಕ AstCoverCrossWith*) { ಹಿಂತಿರುಗಿ ನಿಜ; })) {
                m_runtimeCrosses.emplace(crossp);
                pending.ತಳ್ಳು_ಹಿಂಭಾಗ(crossp);
            }
        }
        ಗಾಗಿ (ಗಾತ್ರ_t ಮುಂದಿನ = 0; ಮುಂದಿನ < pending.ಗಾತ್ರ_2(); ++ಮುಂದಿನ) {
            ಆದರೆ (AstCoverpoint* ಸ್ಥಿರಾಂಕ pointp = VN_CAST(pending[ಮುಂದಿನ], Coverpoint)) {
                ಗಾಗಿ (AstCoverCross* ಸ್ಥಿರಾಂಕ crossp : consumers[pointp]) {
                    ಆದರೆ (m_runtimeCrosses.emplace(crossp).ಎರಡನೇ) pending.ತಳ್ಳು_ಹಿಂಭಾಗ(crossp);
                }
            } ಇಲ್ಲದಿದ್ದರೆ {
                ಗಾಗಿ (AstCoverpoint* ಸ್ಥಿರಾಂಕ pointp : inputs[VN_AS(pending[ಮುಂದಿನ], CoverCross)]) {
                    ಆದರೆ (pointp->exprp()->dtypep()->skipRefp()->isIntegralOrPacked()
                        && m_runtimePoints.emplace(pointp).ಎರಡನೇ) {
                        pending.ತಳ್ಳು_ಹಿಂಭಾಗ(pointp);
                    }
                }
            }
        }

        // The instance node owns this instance's coverpoint/cross runtimes, so it must exist
        // before any of them is created.  Emitted first, ahead of both generate loops.
        generateInstanceAttach();

        // For each coverpoint, generate sampling code
        ಗಾಗಿ (AstCoverpoint* cpp : m_coverpoints) generate_Coverpoint_ಸಂಕೇತ(cpp);

        // For each cross, generate sampling code
        ಗಾಗಿ (AstCoverCross* crossp : m_coverCrosses) generate_Cross_ಸಂಕೇತ(crossp);

        // Every cross has been built, so runtime points only need their exclusions from here.
        ಗಾಗಿ (AstCoverpoint* ಸ್ಥಿರಾಂಕ cpp : m_coverpoints) {
            ಆದರೆ (!m_runtimePoints.ಎಣಿಕೆ_2(cpp)) ಮುಂದುವರಿ;
            m_constructorp->ಕೂಡಿಸು_Stmtsp(ಅಂಶ_Call(cpp->fileline(), m_cpVarMap.at(cpp->name()),
                                               VCMethod::COVERGROUP_ಮೌಲ್ಯ_RELEASE)
                                          ->makeStmt());
        }
        ಗಾಗಿ (Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* valuep : m_detached_ಮೌಲ್ಯಗಳು) VL_DO_DANGLING(ತಳ್ಳು_Deletep(valuep), valuep);
        m_detached_ಮೌಲ್ಯಗಳು.clear();

        // Generate coverage computation code (even for empty covergroups).  Bin registration
        // with the coverage database is handled per coverpoint/cross by their runtime
        // registerBins() calls (emitted in generateCoverpoint/generateCross).
        generate_Coverage_Computation_ಸಂಕೇತ();
    }

    ಸ್ಥಿರ constexpr ಗಾತ್ರ_t ಮೌಲ್ಯ_LIST_ENTRIES = 256;  // Metadata entries per constructor call

    // The value of a constant count, of bins or value tuples: -1 if it is negative, and
    // saturated above the largest limit
    ಸ್ಥಿರ int64_t const_ಎಣಿಕೆ(ಸ್ಥಿರಾಂಕ AstConst* constp) {
        ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& ಅಂಕೆ = constp->ಅಂಕೆ();
        ಆದರೆ (constp->isSigned() && ಅಂಕೆ.isNegative()) ಹಿಂತಿರುಗಿ -1;
        ಹಿಂತಿರುಗಿ ಅಂಕೆ.most_ಗಣ_ಬಿಟ್_P1() > 32 ? INT64_MAX : static_cast<int64_t>(ಅಂಕೆ.ಗಮ್ಯ_UQuad());
    }

    // Whether a count, 'whatp', is integral and, if constant, positive, as the standard's clause
    // 'clausep' requires; else errors
    ಸ್ಥಿರ ತಾರ್ಕಿಕ check_ಎಣಿಕೆ(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* countp, ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* whatp, ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* clausep) {
        ಸ್ಥಿರಾಂಕ AstConst* ಸ್ಥಿರಾಂಕ constp = VN_CAST(countp, Const);
        ಆದರೆ (!countp->dtypep()->skipRefp()->isIntegralOrPacked()) {
            countp->v3error(whatp << " must be integral (IEEE 1800-2023 " << clausep << ")");
            ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (constp && (constp->ಅಂಕೆ().isFourState() || const_ಎಣಿಕೆ(constp) < 1)) {
            countp->v3error(whatp << " must be >= 1, got "
                                  << (constp->ಅಂಕೆ().isFourState() ? constp->ಅಂಕೆ().ascii(ಸುಳ್ಳು)
                                                                  : constp->ಅಂಕೆ().ಗಮ್ಯ_Decimal_S())
                                  << " (IEEE 1800-2023 " << clausep << ")");
            ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
        }
        ಹಿಂತಿರುಗಿ ನಿಜ;
    }

    // The number of bins requested by a valid 'bins auto[N]', or 0
    ಸ್ಥಿರ uint32_t autoBinsRequested(ಸ್ಥಿರಾಂಕ AstCoverBin* binp) {
        ಸ್ಥಿರಾಂಕ AstConst* ಸ್ಥಿರಾಂಕ constp = VN_CAST(binp->arraySizep(), Const);
        ಆದರೆ (!constp) ಹಿಂತಿರುಗಿ 0;
        ಸ್ಥಿರಾಂಕ int64_t ಎಣಿಕೆ_2 = const_ಎಣಿಕೆ(constp);
        ಹಿಂತಿರುಗಿ ಎಣಿಕೆ_2 < 1 || ಎಣಿಕೆ_2 > v3Global.opt.coverageMaxBins() ? 0
                                                                   : static_cast<uint32_t>(ಎಣಿಕೆ_2);
    }

    // True for a 'bins auto[N]' declaration, or the implicit automatic bins of a coverpoint
    ಸ್ಥಿರ ತಾರ್ಕಿಕ isAutoBins(ಸ್ಥಿರಾಂಕ AstCoverBin* binp) {
        ಹಿಂತಿರುಗಿ binp->binsType() == VCoverBinsType::BINS_AUTO
               || binp->binsType() == VCoverBinsType::BINS_AUTO_IMPLICIT;
    }

    // True for a sized array of bins, 'bins b[N] = {...}', whose values an integral coverpoint
    // distributes over N bins when the covergroup is constructed (IEEE 1800-2023 19.5.1)
    ಸ್ಥಿರ ತಾರ್ಕಿಕ isSizedArray(ಸ್ಥಿರಾಂಕ AstCoverBin* binp) {
        ಹಿಂತಿರುಗಿ binp->arraySizep() && !isAutoBins(binp);
    }

    // The 'with' filter of a bin, if any (IEEE 1800-2023 19.5.1.1)
    ಸ್ಥಿರ AstCoverWith* binWith(ಸ್ಥಿರಾಂಕ AstCoverBin* binp) {
        ಹಿಂತಿರುಗಿ VN_CAST(binp->rangesp(), CoverWith);
    }

    // True for bins whose values the covergroup constructor computes: a sized array, or bins of
    // the values a 'with' filter keeps
    ಸ್ಥಿರ ತಾರ್ಕಿಕ isConstructedBins(ಸ್ಥಿರಾಂಕ AstCoverBin* binp) {
        ಹಿಂತಿರುಗಿ isSizedArray(binp) || binWith(binp);
    }

    // The range list of a bin's values, or of the candidates of its 'with' filter; null for all
    // of the coverpoint's values, which a filter of the coverpoint's name has
    ಸ್ಥಿರ Ast_ಗಂಟು* binRangesp(ಸ್ಥಿರಾಂಕ AstCoverBin* binp) {
        ಸ್ಥಿರಾಂಕ AstCoverWith* ಸ್ಥಿರಾಂಕ withp = binWith(binp);
        ಆದರೆ (!withp) ಹಿಂತಿರುಗಿ binp->rangesp();
        ಹಿಂತಿರುಗಿ VN_IS(withp->subp(), CoverpointRef) ? ಶೂನ್ಯಸೂಚಕ : withp->subp();
    }

    // Report and delete a bins declaration of a name that another of its coverpoint has
    ಶೂನ್ಯ check_Bin_ಹೆಸರುಗಳು(AstCoverpoint* coverpointp) {
        std::ಗಣ<std::string> ಹೆಸರುಗಳು;
        ಗಾಗಿ (Ast_ಗಂಟು* nodep = coverpointp->binsp(); nodep;) {
            AstCoverBin* ಸ್ಥಿರಾಂಕ binp = VN_AS(nodep, CoverBin);
            nodep = nodep->nextp();
            ಆದರೆ (ಹೆಸರುಗಳು.emplace(binp->name()).ಎರಡನೇ) ಮುಂದುವರಿ;
            binp->v3error("Duplicate bin " << binp->prettyNameQ() << " in coverpoint "
                                           << coverpointp->prettyNameQ()
                                           << " (IEEE 1800-2023 3.13)");
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(binp->unlink_Fr_ಹಿಂಭಾಗ()), binp);
        }
    }

    // Delete an ignored bins declaration, which binsof then selects as no bins
    ಶೂನ್ಯ dropBins(ಸ್ಥಿರಾಂಕ AstCoverpoint* coverpointp, AstCoverBin* binp) {
        m_droppedBins[coverpointp].ತಳ್ಳು_ಹಿಂಭಾಗ(binp->name());
        VL_DO_DANGLING(ತಳ್ಳು_Deletep(binp->unlink_Fr_ಹಿಂಭಾಗ()), binp);
    }

    // Check the size of a sized array of bins, which drops an invalid array.  A real coverpoint's
    // are unsupported, and treated as arrays of a bin per value.  False unless it stays sized.
    ತಾರ್ಕಿಕ check_Bins_Array_ಗಾತ್ರ(ಸ್ಥಿರಾಂಕ AstCoverpoint* coverpointp, AstCoverBin* binp, ತಾರ್ಕಿಕ integral) {
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ sizep = binp->arraySizep();
        ಆದರೆ (VN_IS(sizep, Unbounded)) {  // A parameter of '$'; see bins_orBraE
            binp->v3error("Bins array size must be integral, not '$' (IEEE 1800-2023 19.5.1)");
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (check_ಎಣಿಕೆ(sizep, "Bins array size", "19.5.1")) {
            ಆದರೆ (integral) ಹಿಂತಿರುಗಿ ನಿಜ;
            binp->v3warn(COVERIGN, "Unsupported: 'bins' explicit array size of a real "
                                   "coverpoint (treated as '[]')");
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(sizep->unlink_Fr_ಹಿಂಭಾಗ()), sizep);
            ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
        }
        dropBins(coverpointp, binp);
        ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
    }

    // Check the bins of a coverpoint whose values the constructor computes, dropping invalid
    // ones.  A wildcard array of too many ranges of values is ignored, or if ignore or illegal,
    // treated as one bin; filtering too many ranges of values ignores the bins.
    ಶೂನ್ಯ checkConstructedBins(AstCoverpoint* coverpointp) {
        ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ integral = coverpointp->exprp()->dtypep()->skipRefp()->isIntegralOrPacked();
        ಗಾಗಿ (Ast_ಗಂಟು* nodep = coverpointp->binsp(); nodep;) {
            AstCoverBin* ಸ್ಥಿರಾಂಕ binp = VN_AS(nodep, CoverBin);
            nodep = nodep->nextp();
            ಆದರೆ (!isConstructedBins(binp)) ಮುಂದುವರಿ;
            ಆದರೆ (isSizedArray(binp) && !check_Bins_Array_ಗಾತ್ರ(coverpointp, binp, integral)) ಮುಂದುವರಿ;
            ಆದರೆ (!binp->isWildcard()
                || sizedWildcardRuns(binp, coverpointp->exprp())
                       <= v3Global.opt.coverageMaxBins()) {
                ಆದರೆ (binWith(binp) && withCandidatesOver(binp, coverpointp->exprp())) {
                    binp->v3warn(COVERIGN, "Unsupported: 'with' filter of more than 2**32 "
                                           "candidate values; bin "
                                               << binp->prettyNameQ() << " ignored");
                    ಆದರೆ (binp->binsType().bin_Is_ಸಾಮಾನ್ಯ()) coverpointp->user2(ನಿಜ);
                    dropBins(coverpointp, binp);
                }
                ಮುಂದುವರಿ;
            }
            // An ignore or illegal array still excludes or checks its values, as one bin
            ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ single = !binp->binsType().bin_Is_ಸಾಮಾನ್ಯ() && !binWith(binp);
            binp->v3warn(COVERIGN,
                         "Unsupported: " << (binWith(binp) ? "'with' filter of wildcard '"
                                                           : "sized wildcard array '")
                                         << binp->binsType().verilogKwd()
                                         << "' of more than --coverage-max-bins of "
                                         << v3Global.opt.coverageMaxBins()
                                         << " ranges of values; bin " << binp->prettyNameQ()
                                         << (single ? " treated as one bin" : " ignored") << "\n"
                                         << binp->warn_ಇನ್ನಷ್ಟು()
                                         << "... Suggest a larger --coverage-max-bins");
            ಆದರೆ (single) {
                Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ sizep = binp->arraySizep();
                VL_DO_DANGLING(ತಳ್ಳು_Deletep(sizep->unlink_Fr_ಹಿಂಭಾಗ()), sizep);
                binp->isArray(ಸುಳ್ಳು);
                ಮುಂದುವರಿ;
            }
            ಆದರೆ (binp->binsType().bin_Is_ಸಾಮಾನ್ಯ()) coverpointp->user2(ನಿಜ);
            dropBins(coverpointp, binp);
        }
    }

    // True if a 'with' filter would be evaluated for more than 2**32 candidate values, known
    // now for the coverpoint's name, or for a range list of constants: each is evaluated once
    // but for an array 'b[N]', which keeps their order and duplicates (see withBegin())
    ಸ್ಥಿರ ತಾರ್ಕಿಕ withCandidatesOver(AstCoverBin* binp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* exprp) {
        ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ ಅಗಲ = ಸತತ_ಅಗಲ(exprp);
        std::ಸದಿಶ<std::pair<V3_ಸಂಖ್ಯೆ, V3_ಸಂಖ್ಯೆ>> runs;
        ಆದರೆ (!binRangesp(binp)) runs = coverpoint_ಮೌಲ್ಯಗಳು(binp, exprp);
        ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ constant
            = [](ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* nodep) { ಹಿಂತಿರುಗಿ VN_IS(nodep, Const) || VN_IS(nodep, Unbounded); };
        ಗಾಗಿ (Ast_ಗಂಟು* rangep = binRangesp(binp); rangep; rangep = rangep->nextp()) {
            ಸ್ಥಿರಾಂಕ AstInsideRange* ಸ್ಥಿರಾಂಕ irp = VN_CAST(rangep, InsideRange);
            // Else known when constructed
            ಆದರೆ (irp ? !constant(irp->lhsp()) || !constant(irp->rhsp()) : !VN_IS(rangep, Const)) {
                ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
            }
            Cross_ಮೌಲ್ಯ_Range range{rangep, resolve_ಅಗಲ(rangep, exprp)};
            ಆದರೆ (!resolve_ಮೌಲ್ಯ(rangep, exprp, ನಿಜ, binp->isWildcard(), range)
                || cross_Range_ಖಾಲಿಗಳು(range)) {
                ಮುಂದುವರಿ;
            }
            std::ಸದಿಶ<std::pair<V3_ಸಂಖ್ಯೆ, V3_ಸಂಖ್ಯೆ>> ಸಿಕ್ಕಿತು{{range.lo, range.hi}};
            ಆದರೆ (range.wildcard) {  // checkConstructedBins bounded the runs
                ಸಿಕ್ಕಿತು.clear();
                crossRangeRuns(range, v3Global.opt.coverageMaxBins(), ಸಿಕ್ಕಿತು);
            }
            ಗಾಗಿ (ಸ್ಥಿರಾಂಕ std::pair<V3_ಸಂಖ್ಯೆ, V3_ಸಂಖ್ಯೆ>& ಸತತ : ಸಿಕ್ಕಿತು) {
                runs.emplace_ಹಿಂಭಾಗ(V3_ಸಂಖ್ಯೆ{rangep, ಅಗಲ, ಸತತ.ಮೊದಲ},
                                  V3_ಸಂಖ್ಯೆ{rangep, ಅಗಲ, ಸತತ.ಎರಡನೇ});
            }
        }
        ಆದರೆ (!isSizedArray(binp)) {  // The union of the values
            std::ವಿಂಗಡಿಸು(runs.begin(), runs.ಕೊನೆ(), [](ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ& lhs, ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ& rhs) {
                ಹಿಂತಿರುಗಿ cross_ಮೌಲ್ಯ_Less(lhs.ಮೊದಲ, rhs.ಮೊದಲ);
            });
            std::ಸದಿಶ<std::pair<V3_ಸಂಖ್ಯೆ, V3_ಸಂಖ್ಯೆ>> merged;
            ಗಾಗಿ (ಸ್ಥಿರಾಂಕ std::pair<V3_ಸಂಖ್ಯೆ, V3_ಸಂಖ್ಯೆ>& ಸತತ : runs) {
                ಆದರೆ (merged.ಖಾಲಿಗಳು() || cross_ಮೌಲ್ಯ_Less(merged.ಹಿಂಭಾಗ().ಎರಡನೇ, ಸತತ.ಮೊದಲ)) {
                    merged.ತಳ್ಳು_ಹಿಂಭಾಗ(ಸತತ);
                } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (cross_ಮೌಲ್ಯ_Less(merged.ಹಿಂಭಾಗ().ಎರಡನೇ, ಸತತ.ಎರಡನೇ)) {
                    merged.ಹಿಂಭಾಗ().ಎರಡನೇ = ಸತತ.ಎರಡನೇ;
                }
            }
            runs = std::move(merged);
        }
        uint64_t ಎಣಿಕೆ_2 = 0;
        ಗಾಗಿ (ಸ್ಥಿರಾಂಕ std::pair<V3_ಸಂಖ್ಯೆ, V3_ಸಂಖ್ಯೆ>& ಸತತ : runs) {
            V3_ಸಂಖ್ಯೆ ಹರವು{exprp, ಅಗಲ};
            ಹರವು.opSub(ಸತತ.ಎರಡನೇ, ಸತತ.ಮೊದಲ);
            ಆದರೆ (ಹರವು.most_ಗಣ_ಬಿಟ್_P1() > 32) ಹಿಂತಿರುಗಿ ನಿಜ;  // More than 2**32 values
            ಎಣಿಕೆ_2 += ಹರವು.ಗಮ್ಯ_UQuad() + 1;
            ಆದರೆ (ಎಣಿಕೆ_2 > (uint64_t{1} << 32)) ಹಿಂತಿರುಗಿ ನಿಜ;
        }
        ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
    }

    // The ranges of values the wildcard patterns of a sized wildcard array, or of a 'with'
    // filter's candidates, give, counted up to more than --coverage-max-bins
    ಸ್ಥಿರ ಗಾತ್ರ_t sizedWildcardRuns(ಸ್ಥಿರಾಂಕ AstCoverBin* binp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* exprp) {
        std::ಸದಿಶ<std::pair<V3_ಸಂಖ್ಯೆ, V3_ಸಂಖ್ಯೆ>> runs;
        ಗಾಗಿ (Ast_ಗಂಟು* rangep = binRangesp(binp); rangep; rangep = rangep->nextp()) {
            ಆದರೆ (!VN_IS(rangep, Const)) ಮುಂದುವರಿ;  // A range, or a value known at construction
            Cross_ಮೌಲ್ಯ_Range range{rangep, resolve_ಅಗಲ(rangep, exprp)};
            ಆದರೆ (resolve_ಮೌಲ್ಯ(rangep, exprp, ನಿಜ, ನಿಜ, range) && !cross_Range_ಖಾಲಿಗಳು(range)) {
                crossRangeRuns(range, v3Global.opt.coverageMaxBins(), runs);
            }
        }
        ಹಿಂತಿರುಗಿ runs.ಗಾತ್ರ_2();
    }

    // Check the automatic bins declarations of a coverpoint.  Each stays one declaration, which
    // generates as a partition of the coverpoint domain (see autoBinRuns).
    ಶೂನ್ಯ checkAutomaticBins(AstCoverpoint* coverpointp, ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* exprp) {
        ಗಾಗಿ (Ast_ಗಂಟು* binp = coverpointp->binsp(); binp; binp = binp->nextp()) {
            AstCoverBin* ಸ್ಥಿರಾಂಕ cbinp = VN_AS(binp, CoverBin);
            ಆದರೆ (cbinp->binsType() != VCoverBinsType::BINS_AUTO) ಮುಂದುವರಿ;
            ಸ್ಥಿರಾಂಕ AstConst* ಸ್ಥಿರಾಂಕ constp = VN_CAST(cbinp->arraySizep(), Const);
            ಆದರೆ (!constp) {
                cbinp->v3error("Automatic bins array size must be a constant");
            } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (const_ಎಣಿಕೆ(constp) < 1) {
                cbinp->v3error("Automatic bins array size must be >= 1, got "
                               << constp->ಅಂಕೆ().ಗಮ್ಯ_Decimal_S());
            } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (const_ಎಣಿಕೆ(constp) > v3Global.opt.coverageMaxBins()) {
                cbinp->v3error("Automatic bins array size of "
                               << constp->ಅಂಕೆ().ಗಮ್ಯ_Decimal_U() << " exceeds limit of "
                               << v3Global.opt.coverageMaxBins() << '\n'
                               << cbinp->warn_ಇನ್ನಷ್ಟು() << "... Suggest a larger --coverage-max-bins");
            } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (!exprp->dtypep()->skipRefp()->isIntegralOrPacked()) {
                cbinp->v3error("Automatic bins are not allowed on a coverpoint of a non-integral "
                               "expression (IEEE 1800-2023 19.5.3).");
            }
        }
    }

    // Extract all option values of a coverpoint or cross in a single pass.
    // atLeastOut: option.at_least (default the covergroup's)
    // autoBinMaxOut: option.auto_bin_max (coverpoint overrides covergroup, default 64)
    ಶೂನ್ಯ ಹೊರತೆಗೆ_ಅಂಶ_Options(Ast_ಗಂಟು* optionsp, ಪೂರ್ಣಾಂಕ& at_ಕನಿಷ್ಠ_ಹೊರಹೋಗು, ಪೂರ್ಣಾಂಕ& auto_Bin_Max_ಹೊರಹೋಗು) {
        at_ಕನಿಷ್ಠ_ಹೊರಹೋಗು = m_cg_At_ಕನಿಷ್ಠ;
        auto_Bin_Max_ಹೊರಹೋಗು = -1;  // -1 = not set at coverpoint level
        ಗಾಗಿ (Ast_ಗಂಟು* optionp = optionsp; optionp; optionp = optionp->nextp()) {
            AstCoverOption* ಸ್ಥಿರಾಂಕ optp = VN_AS(optionp, CoverOption);
            // Weights may be non-constant; generateItemWeight() handles them.  A cross keeps
            // its unsupported options, which V3LinkParse warned of.
            ಆದರೆ (!(optp->optType() == VCoverOptionType::AT_ಕನಿಷ್ಠ
                  || optp->optType() == VCoverOptionType::AUTO_BIN_MAX)) {
                ಮುಂದುವರಿ;
            }
            ಸ್ಥಿರಾಂಕ AstConst* ಸ್ಥಿರಾಂಕ constp = constOption(optp->valuep(), optp->optType());
            ಆದರೆ (!constp) ಮುಂದುವರಿ;
            ಆದರೆ (optp->optType() == VCoverOptionType::AT_ಕನಿಷ್ಠ) {
                at_ಕನಿಷ್ಠ_ಹೊರಹೋಗು = const_At_ಕನಿಷ್ಠ(constp);
            } ಇಲ್ಲದಿದ್ದರೆ {
                auto_Bin_Max_ಹೊರಹೋಗು = constp->ಗಮ್ಯ_SInt();
            }
        }
        // Fall back to covergroup-level auto_bin_max if not set at coverpoint level
        ಆದರೆ (auto_Bin_Max_ಹೊರಹೋಗು < 0) {
            ಆದರೆ (m_covergroupp->cgAutoBinMax() >= 0) {
                auto_Bin_Max_ಹೊರಹೋಗು = m_covergroupp->cgAutoBinMax();
            } ಇಲ್ಲದಿದ್ದರೆ {
                auto_Bin_Max_ಹೊರಹೋಗು = 64;  // Default per IEEE 1800-2023 Table 19-1
            }
        }
    }

    // IEEE 1800-2023 19.5.2: an enum coverpoint has one automatic bin per enumeration value
    ಶೂನ್ಯ createEnumAutoBins(AstCoverpoint* coverpointp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* exprp,
                            ಸ್ಥಿರಾಂಕ AstEnumDType* enump) {
        File_ಸಾಲು* ಸ್ಥಿರಾಂಕ fl = coverpointp->fileline();
        ಗಾಗಿ (ಸ್ಥಿರಾಂಕ Ast_Enum_ಅಂಶ* itemp = enump->itemsp(); itemp;
             itemp = VN_AS(itemp->nextp(), Enum_ಅಂಶ)) {
            AstConst* ಸ್ಥಿರಾಂಕ lop = new_ಮೌಲ್ಯ_Const(fl, VN_AS(itemp->valuep(), Const)->ಅಂಕೆ(), exprp);
            AstInsideRange* ಸ್ಥಿರಾಂಕ rangep = ಹೊಸ AstInsideRange{fl, lop, lop->clone_ವೃಕ್ಷ(ಸುಳ್ಳು)};
            rangep->dtypeFrom(exprp);
            coverpointp->ಕೂಡಿಸು_Binsp(
                ಹೊಸ AstCoverBin{fl, "auto[" + itemp->name() + "]", rangep, ಸುಳ್ಳು, ಸುಳ್ಳು});
        }
    }

    // IEEE 1800-2023 19.5.3/19.11.1: partition first, then apply exclusions.  The partition is one
    // automatic bins declaration, generated as a run like 'bins auto[N]' but numbering its bins.
    ಶೂನ್ಯ createImplicitAutoBins(AstCoverpoint* coverpointp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* exprp, ಪೂರ್ಣಾಂಕ autoBinMax) {
        ಆದರೆ (coverpointp->user2()) ಹಿಂತಿರುಗಿ;  // Declared bins, ignored, leave no bins
        ಗಾಗಿ (Ast_ಗಂಟು* nodep = coverpointp->binsp(); nodep; nodep = nodep->nextp()) {
            ಸ್ಥಿರಾಂಕ VCoverBinsType ಬಗೆ = VN_AS(nodep, CoverBin)->binsType();
            ಆದರೆ (ಬಗೆ != VCoverBinsType::BINS_IGNORE && ಬಗೆ != VCoverBinsType::BINS_ILLEGAL)
                ಹಿಂತಿರುಗಿ;
        }
        ಆದರೆ (ಸ್ಥಿರಾಂಕ AstEnumDType* ಸ್ಥಿರಾಂಕ enump
            = VN_CAST(exprp->dtypep()->skip_Ref_ಗಮ್ಯ_Enump(), EnumDType)) {
            createEnumAutoBins(coverpointp, exprp, enump);
            ಹಿಂತಿರುಗಿ;
        }
        ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ ಅಗಲ = exprp->ಅಗಲ();
        uint32_t ಎಣಿಕೆ_2 = ಅಗಲ < 31 ? std::min<uint32_t>(uint32_t{1} << ಅಗಲ, autoBinMax)
                                    : static_cast<uint32_t>(autoBinMax);
        ಆದರೆ (!ಎಣಿಕೆ_2) ಹಿಂತಿರುಗಿ;
        ಆದರೆ (!exprp->dtypep()->skipRefp()->isIntegralOrPacked()) {
            coverpointp->v3error("Coverpoint of a non-integral expression requires explicit bins "
                                 "(IEEE 1800-2023 19.5.3).");
            ಹಿಂತಿರುಗಿ;
        }
        ಆದರೆ (ಎಣಿಕೆ_2 > v3Global.opt.coverageMaxBins()) {
            coverpointp->v3warn(COVERIGN, "Unsupported: more than "
                                              << v3Global.opt.coverageMaxBins()
                                              << " automatic bins from 'option.auto_bin_max'; "
                                                 "using "
                                              << v3Global.opt.coverageMaxBins() << ".\n"
                                              << coverpointp->warn_ಇನ್ನಷ್ಟು()
                                              << "... Suggest a larger --coverage-max-bins");
            ಎಣಿಕೆ_2 = v3Global.opt.coverageMaxBins();
        }
        File_ಸಾಲು* ಸ್ಥಿರಾಂಕ fl = coverpointp->fileline();
        coverpointp->ಕೂಡಿಸು_Binsp(ಹೊಸ AstCoverBin{fl, "auto", ಹೊಸ AstConst{fl, ಎಣಿಕೆ_2},
                                              VCoverBinsType::BINS_AUTO_IMPLICIT});
    }

    // Sanitize generated names to be valid C++ identifiers
    ಸ್ಥಿರ string sanitizeGeneratedName(string name) {
        std::replace(name.begin(), name.ಕೊನೆ(), '[', '_');
        std::replace(name.begin(), name.ಕೊನೆ(), ']', '_');
        ಹಿಂತಿರುಗಿ name;
    }

    // Capture an iff guard in a function-local temporary so it is evaluated once per sample()
    AstVarRef* capture_Iff_ಗಮ್ಯ_ತಾತ್ಕಾಲಿಕ(Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* iffp, ಸ್ಥಿರಾಂಕ string& ತಾತ್ಕಾಲಿಕ_Name) {
        File_ಸಾಲು* ಸ್ಥಿರಾಂಕ fl = iffp->fileline();
        AstVar* ಸ್ಥಿರಾಂಕ iffVarp
            = ಹೊಸ AstVar{fl, VVarType::BLOCKTEMP, ತಾತ್ಕಾಲಿಕ_Name, iffp->ಕಂಡುಹಿಡಿ_ಬಿಟ್_DType()};
        iffVarp->funcLocal(ನಿಜ);
        m_sampleFuncp->ಕೂಡಿಸು_Stmtsp(iffVarp);
        iffp->unlink_Fr_ಹಿಂಭಾಗ();
        m_sampleFuncp->ಕೂಡಿಸು_Stmtsp(
            ಹೊಸ Ast_ನಿಯೋಜಿಸು{fl, ಹೊಸ AstVarRef{fl, iffVarp, VAccess::WRITE}, iffp});
        ಹಿಂತಿರುಗಿ ಹೊಸ AstVarRef{fl, iffVarp, VAccess::READ};
    }

    // Add a statement sampling the current coverpoint to sample(), under its iff guard if any
    ಶೂನ್ಯ ಕೂಡಿಸು_Sample_Stmt(Ast_ಗಂಟು* stmtp) {
        ಆದರೆ (m_cpGuardp) {
            m_cpGuardp->ಕೂಡಿಸು_Thensp(stmtp);
        } ಇಲ್ಲದಿದ್ದರೆ {
            m_sampleFuncp->ಕೂಡಿಸು_Stmtsp(stmtp);
        }
    }

    // Create previous value variable for transition tracking
    AstVar* create_Prev_ಮೌಲ್ಯ_Var(AstCoverpoint* coverpointp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* exprp) {
        // Check if already created
        ಆದರೆ (AstVar* ಸ್ಥಿರಾಂಕ prevVarp = VN_CAST(coverpointp->user1p(), Var)) ಹಿಂತಿರುಗಿ prevVarp;

        // Create variable to store previous sampled value
        ಸ್ಥಿರಾಂಕ string varName = "__Vprev_" + coverpointp->name();
        AstVar* prevVarp
            = ಹೊಸ AstVar{coverpointp->fileline(), VVarType::MEMBER, varName, exprp->dtypep()};
        m_covergroupp->ಕೂಡಿಸು_Membersp(prevVarp);

        UINFO(4, "    Created previous value variable: " << varName);

        // Initialize to zero in constructor
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ initExprp
            = ಹೊಸ AstConst{prevVarp->fileline(), AstConst::Widthed_ಮೌಲ್ಯ{}, prevVarp->ಅಗಲ(), 0};
        Ast_ಗಂಟು_Stmt* ಸ್ಥಿರಾಂಕ initStmtp = ಹೊಸ Ast_ನಿಯೋಜಿಸು{
            prevVarp->fileline(), ಹೊಸ AstVarRef{prevVarp->fileline(), prevVarp, VAccess::WRITE},
            initExprp};
        m_constructorp->ಕೂಡಿಸು_Stmtsp(initStmtp);

        coverpointp->user1p(prevVarp);
        ಹಿಂತಿರುಗಿ prevVarp;
    }

    // Create state position variable for multi-value transition bins
    // Tracks position in sequence: 0=not started, 1=seen first item, etc.
    AstVar* createSequenceStateVar(AstCoverpoint* coverpointp, AstCoverBin* binp) {
        // Create variable to track sequence position
        ಸ್ಥಿರಾಂಕ string varName = "__Vseqpos_" + coverpointp->name() + "_" + binp->name();
        // Use 8-bit integer for state position (sequences rarely > 255 items)
        AstVar* stateVarp
            = ಹೊಸ AstVar{binp->fileline(), VVarType::MEMBER, varName, VFlagLogicPacked{}, 8};
        m_covergroupp->ಕೂಡಿಸು_Membersp(stateVarp);

        UINFO(4, "    Created sequence state variable: " << varName);

        // Initialize to 0 (not started) in constructor
        Ast_ಗಂಟು_Stmt* ಸ್ಥಿರಾಂಕ initStmtp = ಹೊಸ Ast_ನಿಯೋಜಿಸು{
            stateVarp->fileline(), ಹೊಸ AstVarRef{stateVarp->fileline(), stateVarp, VAccess::WRITE},
            ಹೊಸ AstConst{stateVarp->fileline(), AstConst::Widthed_ಮೌಲ್ಯ{}, 8, 0}};
        m_constructorp->ಕೂಡಿಸು_Stmtsp(initStmtp);

        ಹಿಂತಿರುಗಿ stateVarp;
    }

    ಶೂನ್ಯ generate_Coverpoint_ಸಂಕೇತ(AstCoverpoint* coverpointp) {
        UINFO(4, "  Generating code for coverpoint: " << coverpointp->name());

        // Get the coverpoint expression
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* exprp = coverpointp->exprp();

        // Check automatic bins before processing
        checkAutomaticBins(coverpointp, exprp);

        // Extract all coverpoint options in a single pass
        ಪೂರ್ಣಾಂಕ at_ಕನಿಷ್ಠ_ಮೌಲ್ಯ;
        ಪೂರ್ಣಾಂಕ autoBinMax;
        ಹೊರತೆಗೆ_ಅಂಶ_Options(coverpointp->optionsp(), at_ಕನಿಷ್ಠ_ಮೌಲ್ಯ, autoBinMax);
        UINFO(6, "    Coverpoint at_least = " << at_ಕನಿಷ್ಠ_ಮೌಲ್ಯ << " auto_bin_max = " << autoBinMax);

        // Create implicit automatic bins if no regular bins exist
        createImplicitAutoBins(coverpointp, exprp, autoBinMax);

        // A false iff guard disables sampling of the coverpoint (IEEE 1800-2023 19.5), so
        // neither is its expression evaluated, nor are its bins or transition state updated
        VL_RESTORER(m_cpGuardp);
        ಆದರೆ (Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ iffp = coverpointp->iffp()) {
            m_cpGuardp = ಹೊಸ AstIf{iffp->fileline(), iffp->unlink_Fr_ಹಿಂಭಾಗ()};
        }

        AstVar* ಸ್ಥಿರಾಂಕ ಮೌಲ್ಯ_Varp = ಹೊಸ AstVar{
            coverpointp->fileline(), VVarType::BLOCKTEMP,
            "__VcpValue_" + sanitizeGeneratedName(coverpointp->name()), exprp->dtypep()};
        ಮೌಲ್ಯ_Varp->funcLocal(ನಿಜ);
        m_sampleFuncp->ಕೂಡಿಸು_Stmtsp(ಮೌಲ್ಯ_Varp);
        exprp->unlink_Fr_ಹಿಂಭಾಗ();
        ಕೂಡಿಸು_Sample_Stmt(ಹೊಸ Ast_ನಿಯೋಜಿಸು{
            coverpointp->fileline(),
            ಹೊಸ AstVarRef{coverpointp->fileline(), ಮೌಲ್ಯ_Varp, VAccess::WRITE}, exprp});
        coverpointp->exprp(ಹೊಸ AstVarRef{coverpointp->fileline(), ಮೌಲ್ಯ_Varp, VAccess::READ});
        exprp = coverpointp->exprp();

        // Every coverpoint routes through the VlCoverpoint runtime.  Transition coverpoints are
        // included: their per-value matching is still generated as a state machine in sample()
        // (see generateCoverpoint), but the bin hit is recorded in the runtime bin
        // rather than a bare counter.
        generateCoverpoint(coverpointp, exprp, at_ಕನಿಷ್ಠ_ಮೌಲ್ಯ);
        // Sampling follows what generateCoverpoint added unguarded, such as clearing the hit list
        ಆದರೆ (m_cpGuardp) m_sampleFuncp->ಕೂಡಿಸು_Stmtsp(m_cpGuardp);
    }

    // Build the condition under which a default bin matches: NOT(OR of all normal bins).
    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* buildDefaultCondition(AstCoverpoint* coverpointp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* exprp,
                                       File_ಸಾಲು* fl) {
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* anyBinMatchp = ಶೂನ್ಯಸೂಚಕ;
        ಗಾಗಿ (Ast_ಗಂಟು* binp = coverpointp->binsp(); binp; binp = binp->nextp()) {
            AstCoverBin* ಸ್ಥಿರಾಂಕ cbinp = VN_AS(binp, CoverBin);
            ಆದರೆ (cbinp->binsType() == VCoverBinsType::BINS_DEFAULT
                || cbinp->binsType() == VCoverBinsType::BINS_IGNORE
                || cbinp->binsType() == VCoverBinsType::BINS_ILLEGAL)
                ಮುಂದುವರಿ;
            // The values of constructed bins are known at construction; see emitSizedSample
            ಆದರೆ (isConstructedBins(cbinp)) ಮುಂದುವರಿ;
            ಆದರೆ (isAutoBins(cbinp)) {
                // Automatic bins partition the whole domain, leaving no default value
                ಆದರೆ (anyBinMatchp) VL_DO_DANGLING(ತಳ್ಳು_Deletep(anyBinMatchp), anyBinMatchp);
                ಹಿಂತಿರುಗಿ ಹೊಸ AstConst{fl, AstConst::ಬಿಟ್_False{}};
            }
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ binCondp = buildBinCondition(cbinp, exprp);
            UASSERT_OBJ(binCondp, cbinp,
                        "buildBinCondition returned nullptr for non-ignore/non-illegal bin");
            anyBinMatchp = anyBinMatchp ? ಹೊಸ AstOr{fl, anyBinMatchp, binCondp} : binCondp;
        }
        ಹಿಂತಿರುಗಿ anyBinMatchp ? static_cast<Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ*>(ಹೊಸ AstNot{fl, anyBinMatchp})
                            : static_cast<Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ*>(ಹೊಸ AstConst{fl, AstConst::ಬಿಟ್_True{}});
    }

    //====================================================================
    // VlCoverpoint conversion

    ಸ್ಥಿರ ತಾರ್ಕಿಕ coverpointHasStateExclusions(ಸ್ಥಿರಾಂಕ AstCoverpoint* coverpointp) {
        ಗಾಗಿ (ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* nodep = coverpointp->binsp(); nodep; nodep = nodep->nextp()) {
            ಸ್ಥಿರಾಂಕ AstCoverBin* ಸ್ಥಿರಾಂಕ binp = VN_AS(nodep, CoverBin);
            ಆದರೆ (!binp->transp() && binp->rangesp()
                && (binp->binsType() == VCoverBinsType::BINS_IGNORE
                    || binp->binsType() == VCoverBinsType::BINS_ILLEGAL)) {
                ಹಿಂತಿರುಗಿ ನಿಜ;
            }
        }
        ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
    }

    ಸ್ಥಿರ ತಾರ್ಕಿಕ coverpointHasConstructedBins(ಸ್ಥಿರಾಂಕ AstCoverpoint* coverpointp) {
        ಗಾಗಿ (ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* nodep = coverpointp->binsp(); nodep; nodep = nodep->nextp()) {
            ಆದರೆ (isConstructedBins(VN_AS(nodep, CoverBin))) ಹಿಂತಿರುಗಿ ನಿಜ;
        }
        ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
    }

    // True if a Normal state bin, or an array-bin element, has no value of the coverpoint's
    // type (IEEE 1800-2023 19.5.7).  Array ranges enumerate in-type values, so cannot vanish.
    ಸ್ಥಿರ ತಾರ್ಕಿಕ coverpoint_Has_ಖಾಲಿಗಳು_Bins(ಸ್ಥಿರಾಂಕ AstCoverpoint* coverpointp) {
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ exprp = coverpointp->exprp();
        ಗಾಗಿ (Ast_ಗಂಟು* nodep = coverpointp->binsp(); nodep; nodep = nodep->nextp()) {
            ಸ್ಥಿರಾಂಕ AstCoverBin* ಸ್ಥಿರಾಂಕ binp = VN_AS(nodep, CoverBin);
            ಆದರೆ (!binp->binsType().bin_Is_ಸಾಮಾನ್ಯ() || binp->transp() || !binp->rangesp()) ಮುಂದುವರಿ;
            // A wildcard array has a bin for each value its elements match, so none empty, and
            // the constructor creates no bin without values of a 'with' filter
            ಆದರೆ ((binp->isArray() && binp->isWildcard()) || binWith(binp)) ಮುಂದುವರಿ;
            ತಾರ್ಕಿಕ ಖಾಲಿಗಳು = ನಿಜ;
            ಗಾಗಿ (Ast_ಗಂಟು* valuep = binp->rangesp(); valuep; valuep = valuep->nextp()) {
                ಆದರೆ (binp->isArray() && VN_IS(valuep, InsideRange)) ಮುಂದುವರಿ;
                Cross_ಮೌಲ್ಯ_Range range{valuep, resolve_ಅಗಲ(valuep, exprp)};
                ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ none = resolve_ಮೌಲ್ಯ(valuep, exprp, ನಿಜ, binp->isWildcard(), range)
                                  && cross_Range_ಖಾಲಿಗಳು(range);
                ಆದರೆ (binp->isArray() && none) ಹಿಂತಿರುಗಿ ನಿಜ;
                ಖಾಲಿಗಳು &= none;
            }
            ಆದರೆ (ಖಾಲಿಗಳು && !binp->isArray()) ಹಿಂತಿರುಗಿ ನಿಜ;
        }
        ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
    }

    // True if a coverpoint has any transition bin.  Used to decide whether sample() emits the
    // end-of-sample previous-value update that transition matching needs.
    ಸ್ಥಿರ ತಾರ್ಕಿಕ coverpointHasTransition(AstCoverpoint* coverpointp) {
        ಗಾಗಿ (Ast_ಗಂಟು* binp = coverpointp->binsp(); binp; binp = binp->nextp()) {
            ಆದರೆ (VN_AS(binp, CoverBin)->transp()) ಹಿಂತಿರುಗಿ ನಿಜ;
        }
        ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
    }

    // The interned AstBasicDType for one of the covergroup runtime keywords.
    ಸ್ಥಿರ AstBasicDType* basicDType(File_ಸಾಲು* fl, VBasicDTypeKwd kwd) {
        ಹಿಂತಿರುಗಿ v3Global.rootp()->typeTablep()->ಕಂಡುಹಿಡಿ_Basic_DType(fl, kwd);
    }

    // Get (or create) the coverpoint dtype for a hit-list bound, interned so each distinct bound
    // yields one node.
    AstCoverpointDType* coverpointDType(File_ಸಾಲು* fl, uint32_t ದೊರಕಿತು_Bound) {
        AstCoverpointDType*& typep = m_cpDTypes[ದೊರಕಿತು_Bound];
        ಆದರೆ (!typep) {
            typep = ಹೊಸ AstCoverpointDType{fl, ದೊರಕಿತು_Bound};
            v3Global.rootp()->typeTablep()->ಕೂಡಿಸು_Typesp(typep);
        }
        ಹಿಂತಿರುಗಿ typep;
    }

    // The key of the covergroup's type in the coverage registry, which get_coverage() queries
    std::string covergroupProtectedName() ಸ್ಥಿರಾಂಕ {
        ಹಿಂತಿರುಗಿ VIdProtect::protect_ಪದಗಳು_If(m_covergroupName, v3Global.opt.protectIds());
    }

    // The name of the covergroup's type, which keys the coverage registry and database: as
    // $typename names it (IEEE 1800-2023 20.6.1), so apart for covergroups of distinct scopes and
    // specializations, and alike in each Verilator run of a hierarchical design.  As design units
    // of distinct libraries may share a name, one of a library other than the default is prefixed
    // by its library, as '%l' prints it (IEEE 1800-2023 33.4).
    std::string covergroupTypeName() ಸ್ಥಿರಾಂಕ {
        UASSERT_OBJ(m_unitp, m_covergroupp, "Covergroup declared outside of a design unit");
        ಸ್ಥಿರಾಂಕ std::string& libname = m_unitp->libname();
        ಸ್ಥಿರಾಂಕ std::string name = m_covergroupp->dtypeName(ನಿಜ);
        ಹಿಂತಿರುಗಿ libname == "work" ? name : libname + "." + name;
    }

    // Emit the covergroup's instance handle member and the constructor statement that creates
    // its node in the per-context coverage registry.  Runs before any coverpoint or cross is
    // generated, so their runtimes can be added to the node as they are created.
    ಶೂನ್ಯ generateInstanceAttach() {
        File_ಸಾಲು* ಸ್ಥಿರಾಂಕ fl = m_covergroupp->fileline();
        // V3LinkParse synthesizes a 'new' for every covergroup; the item generators below already
        // rely on that, and this attach runs even for a covergroup with no coverpoints at all.
        UASSERT_OBJ(m_constructorp, m_covergroupp, "Covergroup missing synthesized constructor");
        m_cgInstVarp = ಹೊಸ AstVar{fl, VVarType::MEMBER, "__Vcg_inst",
                                  basicDType(fl, VBasicDTypeKwd::COVERGROUP_INSTHANDLE)};
        m_covergroupp->ಕೂಡಿಸು_Membersp(m_cgInstVarp);

        m_constructorp->ಕೂಡಿಸು_Stmtsp(
            ಅಂಶ_Call(fl, m_cgInstVarp, VCMethod::COVERGROUP_ATTACH,
                     {ctext(fl, "vlSymsp->_vm_contextp__->covergroupRegistryp()"
                                "->newCovergroupInst("
                                    + quoted(covergroupProtectedName())
                                    + (m_cg_May_ವಿಲೀನ ? ", true" : ", false") + ")")},
                     /*usePtr=*/ಸುಳ್ಳು)
                ->makeStmt());
        // The node reads option.weight in place, so procedural assignments take effect
        AstCExpr* ಸ್ಥಿರಾಂಕ ತೂಕ_Addrp = ಹೊಸ AstCExpr{fl, "&"};
        ತೂಕ_Addrp->ಕೂಡಿಸು(newOptionSel(fl, optionVar(ಸುಳ್ಳು), "weight", VAccess::READ));
        m_constructorp->ಕೂಡಿಸು_Stmtsp(ಅಂಶ_Call(fl, m_cgInstVarp, VCMethod::COVERGROUP_LEND_ತೂಕ,
                                           {ತೂಕ_Addrp, file_ಸಾಲು_Debug(fl)}, /*usePtr=*/ಸುಳ್ಳು)
                                      ->makeStmt());
    }

    // A '__Vcg_inst.p()-><method>()' call on the covergroup's instance node
    AstCMethodHard* instanceCall(File_ಸಾಲು* fl, VCMethod ವಿಧಾನ) {
        // '__Vcg_inst.p()' -- a value handle, so '.' not '->'
        AstCMethodHard* ಸ್ಥಿರಾಂಕ instp
            = ಹೊಸ AstCMethodHard{fl, memberRef(fl, m_cgInstVarp), VCMethod::COVERGROUP_INST_P};
        instp->usePtr(ಸುಳ್ಳು);
        instp->dtype_ಗಣ_Void();  // Opaque receiver; only ever the 'fromp' of the call below
        AstCMethodHard* ಸ್ಥಿರಾಂಕ callp = ಹೊಸ AstCMethodHard{fl, instp, ವಿಧಾನ};
        callp->usePtr(ನಿಜ);
        ಹಿಂತಿರುಗಿ callp;
    }

    // Emit 'this->__Vcp_x = this->__Vcg_inst.p()->addCoverpoint<K>();' (or addCross), which
    // creates the item runtime in the instance node and borrows a pointer to it.
    Ast_ನಿಯೋಜಿಸು* make_ಅಂಶ_Create(File_ಸಾಲು* fl, AstVar* ಅಂಶ_Varp, VCMethod ವಿಧಾನ) {
        AstCMethodHard* ಸ್ಥಿರಾಂಕ createp = instanceCall(fl, ವಿಧಾನ);
        createp->dtypep(ಅಂಶ_Varp->dtypep());
        ಹಿಂತಿರುಗಿ ಹೊಸ Ast_ನಿಯೋಜಿಸು{fl, memberRef(fl, ಅಂಶ_Varp, VAccess::WRITE), createp};
    }

    // Constant bounds of one rangesp() element (an InsideRange or a single Const).  Each bound is
    // the raw AST node -- an AstConst or an AstUnbounded ('$') -- with the const/unbounded view
    // derived on demand, so there is one source of truth per bound.  After a successful
    // constRangeBounds() neither node is null; a single Const has both bounds aliasing one node.
    ರಚನೆ RangeBounds final {
        Ast_ಗಂಟು* loNodep = ಶೂನ್ಯಸೂಚಕ;  // low bound: AstConst or AstUnbounded
        Ast_ಗಂಟು* hiNodep = ಶೂನ್ಯಸೂಚಕ;  // high bound: AstConst or AstUnbounded
        ತಾರ್ಕಿಕ loUnbounded() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ VN_IS(loNodep, Unbounded); }
        ತಾರ್ಕಿಕ hiUnbounded() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ VN_IS(hiNodep, Unbounded); }
        AstConst* loConstp() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ VN_CAST(loNodep, Const); }
        AstConst* hiConstp() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ VN_CAST(hiNodep, Const); }
    };

    // Decode one rangesp() element into its constant bounds.  Returns false if rp is neither an
    // InsideRange nor a single Const, or if a present bound is non-constant or 4-state.  '$'
    // bounds are left as AstUnbounded (not resolved) -- the caller applies its own policy.
    // Centralizes the InsideRange/Const/Unbounded decode shared by the hit-list-bound paths.
    ಸ್ಥಿರ ತಾರ್ಕಿಕ constRangeBounds(Ast_ಗಂಟು* rp, RangeBounds& rb) {
        ಆದರೆ (AstInsideRange* ಸ್ಥಿರಾಂಕ irp = VN_CAST(rp, InsideRange)) {
            rb.loNodep = irp->lhsp();
            rb.hiNodep = irp->rhsp();
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (AstConst* ಸ್ಥಿರಾಂಕ cp = VN_CAST(rp, Const)) {
            rb.loNodep = rb.hiNodep = cp;
        } ಇಲ್ಲದಿದ್ದರೆ {
            ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
        }
        // Each bound must be a constant unless it is '$'; reject non-const and 4-state.
        AstConst* ಸ್ಥಿರಾಂಕ lc = rb.loConstp();
        AstConst* ಸ್ಥಿರಾಂಕ hc = rb.hiConstp();
        ಆದರೆ ((!lc && !rb.loUnbounded()) || (!hc && !rb.hiUnbounded())) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
        ಆದರೆ ((lc && lc->ಅಂಕೆ().isFourState()) || (hc && hc->ಅಂಕೆ().isFourState())) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
        ಹಿಂತಿರುಗಿ ನಿಜ;
    }

    // Collect the covered value intervals of a single (non-array) Normal bin.  Returns false
    // if any range isn't a constant/open InsideRange or single Const (e.g. wildcard, non-const).
    ಸ್ಥಿರ ತಾರ್ಕಿಕ ಹೊರತೆಗೆ_Range_Intervals(AstCoverBin* cbinp, uint64_t maxVal,
                                      std::ಸದಿಶ<std::pair<uint64_t, uint64_t>>& ಹೊರಹೋಗು) {
        ಆದರೆ (!cbinp->rangesp()) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
        ಗಾಗಿ (Ast_ಗಂಟು* rp = cbinp->rangesp(); rp; rp = rp->nextp()) {
            RangeBounds rb;
            ಆದರೆ (!constRangeBounds(rp, rb)) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
            ಆದರೆ ((rb.loConstp() && rb.loConstp()->ಅಗಲ() > 64)
                || (rb.hiConstp() && rb.hiConstp()->ಅಗಲ() > 64)) {
                ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;  // Use the safe slot-count bound for wide values.
            }
            ಸ್ಥಿರಾಂಕ uint64_t lo = rb.loUnbounded() ? 0 : rb.loConstp()->ಗಮ್ಯ_UQuad();
            ಸ್ಥಿರಾಂಕ uint64_t hi = rb.hiUnbounded() ? maxVal : rb.hiConstp()->ಗಮ್ಯ_UQuad();
            ಆದರೆ (lo > hi) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
            ಹೊರಹೋಗು.emplace_ಹಿಂಭಾಗ(lo, hi);
        }
        ಹಿಂತಿರುಗಿ ನಿಜ;
    }

    // Append one Normal bin's cross-slot interval-sets to `bins` and bump `slotCount` by an upper
    // bound of the bin's slots holding one value.  Returns false if any part isn't statically
    // enumerable (the caller then falls back to the always-safe slot count).  A non-array bin
    // is one slot covering the union of its intervals; the bins of an array element or of an
    // automatic bins declaration hold disjoint values, so the element or declaration counts once.
    ತಾರ್ಕಿಕ appendBinCrossSlots(AstCoverBin* cbinp, uint64_t maxVal, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* exprp,
                             std::ಸದಿಶ<std::ಸದಿಶ<std::pair<uint64_t, uint64_t>>>& bins,
                             ಪೂರ್ಣಾಂಕ& ಸ್ಥಾನ_ಎಣಿಕೆ) {
        ಆದರೆ (isAutoBins(cbinp)) {
            ++ಸ್ಥಾನ_ಎಣಿಕೆ;
            bins.ತಳ್ಳು_ಹಿಂಭಾಗ({{0, maxVal}});
            ಹಿಂತಿರುಗಿ exprp->ಅಗಲ() <= 64;
        }
        ಆದರೆ (binWith(cbinp)) {
            // A value is in one bin of a filter's, but of a sized array, in one for each range
            // list element holding it; the coverpoint's name is one element
            ಪೂರ್ಣಾಂಕ elements = 0;
            ಗಾಗಿ (ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* rp = binRangesp(cbinp); rp && isSizedArray(cbinp);
                 rp = rp->nextp()) {
                ++elements;
            }
            ಸ್ಥಾನ_ಎಣಿಕೆ += std::max(1, elements);
            ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
        }
        ಆದರೆ (cbinp->isArray() && cbinp->isWildcard() && !cbinp->arraySizep()) {
            // A value is in at most one bin of a wildcard array: one slot covering its values.
            // Signed values are sign-extended, not unsigned intervals (see computeHitListBound)
            ++ಸ್ಥಾನ_ಎಣಿಕೆ;
            ಆದರೆ (exprp->isSigned() || exprp->ಅಗಲ() > 64) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
            ಸ್ಥಿರಾಂಕ BinRuns runs = wildcardBinRuns(cbinp, exprp, ಸುಳ್ಳು);
            ಆದರೆ (runs.unsupported) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
            std::ಸದಿಶ<std::pair<uint64_t, uint64_t>> ivs;
            ಗಾಗಿ (ಸ್ಥಿರಾಂಕ Bin_ಸತತ& ಸತತ : runs.runs) {
                ivs.emplace_ಹಿಂಭಾಗ(ಸತತ.m_lo.ಗಮ್ಯ_UQuad(), ಸತತ.m_hi.ಗಮ್ಯ_UQuad());
            }
            bins.ತಳ್ಳು_ಹಿಂಭಾಗ(std::move(ivs));
            ಹಿಂತಿರುಗಿ ನಿಜ;
        }
        ಆದರೆ (cbinp->isArray()) ಹಿಂತಿರುಗಿ appendArrayBinCrossSlots(cbinp, exprp, bins, ಸ್ಥಾನ_ಎಣಿಕೆ);
        // Non-array bin: one slot covering the union of its intervals.
        ++ಸ್ಥಾನ_ಎಣಿಕೆ;
        std::ಸದಿಶ<std::pair<uint64_t, uint64_t>> ivs;
        ಆದರೆ (cbinp->isWildcard() || !ಹೊರತೆಗೆ_Range_Intervals(cbinp, maxVal, ivs)) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
        bins.ತಳ್ಳು_ಹಿಂಭಾಗ(std::move(ivs));
        ಹಿಂತಿರುಗಿ ನಿಜ;
    }

    // Append the cross slots of an array Normal bin: each element is a slot covering its
    // values, as at most one of its single-value bins holds a value.  An element holds the
    // values arrayBinRuns() gives it: those of the coverpoint type (IEEE 1800-2023 19.5.7).
    // Elements of a signed coverpoint, non-constant elements, and elements with values beyond
    // 64 bits can't be enumerated, so they count one slot but lose exactness.  Returns false if
    // any element wasn't enumerable to exact values.
    ತಾರ್ಕಿಕ appendArrayBinCrossSlots(AstCoverBin* cbinp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* exprp,
                                  std::ಸದಿಶ<std::ಸದಿಶ<std::pair<uint64_t, uint64_t>>>& bins,
                                  ಪೂರ್ಣಾಂಕ& ಸ್ಥಾನ_ಎಣಿಕೆ) {
        // Signed values resolve sign-extended, not as unsigned intervals, and wildcard patterns
        // are not intervals
        ತಾರ್ಕಿಕ exact = !exprp->isSigned() && !cbinp->isWildcard();
        ಗಾಗಿ (Ast_ಗಂಟು* rp = cbinp->rangesp(); rp; rp = rp->nextp()) {
            ++ಸ್ಥಾನ_ಎಣಿಕೆ;
            RangeBounds rb;
            Cross_ಮೌಲ್ಯ_Range range{rp, resolve_ಅಗಲ(rp, exprp)};
            ಆದರೆ (!exact || !constRangeBounds(rp, rb)
                || !resolve_ಮೌಲ್ಯ(rp, exprp, ನಿಜ, ಸುಳ್ಳು, range)) {
                exact = ಸುಳ್ಳು;
                ಮುಂದುವರಿ;
            }
            ಆದರೆ (cross_Range_ಖಾಲಿಗಳು(range)) ಮುಂದುವರಿ;  // Its bin, if any, holds no value
            ಆದರೆ (range.hi.most_ಗಣ_ಬಿಟ್_P1() > 64) {
                exact = ಸುಳ್ಳು;
                ಮುಂದುವರಿ;
            }
            bins.ತಳ್ಳು_ಹಿಂಭಾಗ({{range.lo.ಗಮ್ಯ_UQuad(), range.hi.ಗಮ್ಯ_UQuad()}});
        }
        ಹಿಂತಿರುಗಿ exact;
    }

    // Compute the hit-list bound for a coverpoint: the maximum number of Normal
    // bins one sample value can match.  Non-cross-fed coverpoints don't feed a cross, so
    // their hit list is unused -> 1.  Otherwise compute the exact max bin overlap; fall back
    // to the (always-safe) Normal-slot count when any bin isn't statically analyzable.
    ಪೂರ್ಣಾಂಕ compute_ದೊರಕಿತು_List_Bound(AstCoverpoint* coverpointp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* exprp, ತಾರ್ಕಿಕ crossFed) {
        ಆದರೆ (!crossFed) ಹಿಂತಿರುಗಿ 1;
        ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ ಅಗಲ = exprp->ಅಗಲ();
        ಸ್ಥಿರಾಂಕ uint64_t maxVal = (ಅಗಲ >= 64) ? UINT64_MAX : ((1ULL << ಅಗಲ) - 1);
        // One entry per Normal bin (cross slot): its covered intervals.
        std::ಸದಿಶ<std::ಸದಿಶ<std::pair<uint64_t, uint64_t>>> bins;
        ಪೂರ್ಣಾಂಕ ಸ್ಥಾನ_ಎಣಿಕೆ = 0;  // == runtime m_normal; the safe fallback bound
        // Unsigned intervals cannot establish overlap between differently sized signed values.
        ತಾರ್ಕಿಕ exact = !exprp->isSigned();
        ಗಾಗಿ (Ast_ಗಂಟು* binp = coverpointp->binsp(); binp; binp = binp->nextp()) {
            AstCoverBin* ಸ್ಥಿರಾಂಕ cbinp = VN_AS(binp, CoverBin);
            ಆದರೆ (!cbinp->binsType().bin_Is_ಸಾಮಾನ್ಯ())
                ಮುಂದುವರಿ;  // ignore/illegal/default: not hit-listed
            ಆದರೆ (!appendBinCrossSlots(cbinp, maxVal, exprp, bins, ಸ್ಥಾನ_ಎಣಿಕೆ)) exact = ಸುಳ್ಳು;
        }
        ಆದರೆ (!exact) ಹಿಂತಿರುಗಿ std::max(1, ಸ್ಥಾನ_ಎಣಿಕೆ);
        ಆದರೆ (bins.ಖಾಲಿಗಳು()) ಹಿಂತಿರುಗಿ 1;
        // Max overlap occurs at some interval start; count covering bins at each lo.
        std::ಸದಿಶ<uint64_t> pts;
        ಗಾಗಿ (ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ& b : bins)
            ಗಾಗಿ (ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ& iv : b) pts.ತಳ್ಳು_ಹಿಂಭಾಗ(iv.ಮೊದಲ);
        std::ವಿಂಗಡಿಸು(pts.begin(), pts.ಕೊನೆ());
        pts.erase(std::unique(pts.begin(), pts.ಕೊನೆ()), pts.ಕೊನೆ());
        ಪೂರ್ಣಾಂಕ maxOverlap = 1;
        ಗಾಗಿ (ಸ್ಥಿರಾಂಕ uint64_t p : pts) {
            ಪೂರ್ಣಾಂಕ cnt = 0;
            ಗಾಗಿ (ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ& b : bins) {
                ಗಾಗಿ (ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ& iv : b) {
                    ಆದರೆ (iv.ಮೊದಲ <= p && p <= iv.ಎರಡನೇ) {
                        ++cnt;
                        ನಿಲ್ಲಿಸು;  // count each bin at most once
                    }
                }
            }
            ಆದರೆ (cnt > maxOverlap) maxOverlap = cnt;
        }
        ಹಿಂತಿರುಗಿ maxOverlap;
    }

    // A 'this->m_member' reference for embedding in an AstCStmt
    AstVarRef* memberRef(File_ಸಾಲು* fl, AstVar* varp, VAccess access = VAccess::READ) {
        AstVarRef* ಸ್ಥಿರಾಂಕ refp = ಹೊಸ AstVarRef{fl, varp, access};
        refp->self_ಸೂಚಕ(VSelf_ಸೂಚಕ_ಪಠ್ಯ{VSelf_ಸೂಚಕ_ಪಠ್ಯ::This{}});
        ಹಿಂತಿರುಗಿ refp;
    }

    // A 'this->m_member-><method>(args...)' call on one member.  usePtr is false only for
    // __Vcg_inst, which is a value handle; the item members are borrowed pointers into the
    // instance node.  Numeric arguments are AstConst; the rest are C++ text that has no AST
    // form (see ctext).
    AstCMethodHard* ಅಂಶ_Call(File_ಸಾಲು* fl, AstVar* varp, VCMethod ವಿಧಾನ,
                             ಸ್ಥಿರಾಂಕ std::ಸದಿಶ<Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ*>& ವಾದಗಳು = {}, ತಾರ್ಕಿಕ usePtr = ನಿಜ) {
        AstCMethodHard* ಸ್ಥಿರಾಂಕ callp = ಹೊಸ AstCMethodHard{fl, memberRef(fl, varp), ವಿಧಾನ};
        ಗಾಗಿ (Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ argp : ವಾದಗಳು) callp->ಕೂಡಿಸು_Pinsp(argp);
        callp->usePtr(usePtr);
        callp->dtype_ಗಣ_Void();
        ಹಿಂತಿರುಗಿ callp;
    }

    // An unsigned integer argument.
    ಸ್ಥಿರ AstConst* cnum(File_ಸಾಲು* fl, uint32_t ಮೌಲ್ಯ) { ಹಿಂತಿರುಗಿ ಹೊಸ AstConst{fl, ಮೌಲ್ಯ}; }

    // A literal C++ argument with no AST equivalent: a 'const char*' string literal (an SV
    // string AstConst emits '"..."s', a std::string temporary the runtime cannot borrow), a
    // VlCovBinKind enum token, a constant selection-word initializer list, a VlFileLineDebug, or
    // a '__V' temporary declared by the enclosing AstCStmt.
    ಸ್ಥಿರ AstCExpr* ctext(File_ಸಾಲು* fl, ಸ್ಥಿರಾಂಕ std::string& ಪಠ್ಯ) {
        ಹಿಂತಿರುಗಿ ಹೊಸ AstCExpr{fl, ಪಠ್ಯ};
    }

    // A C++ string literal.  Escapes control characters as the emitter does elsewhere -- bin
    // names and filenames reach the generated code verbatim when --protect-ids is off, and an
    // SV escaped identifier may hold a quote or backslash.
    ಸ್ಥಿರ std::string quoted(ಸ್ಥಿರಾಂಕ std::string& ಪಠ್ಯ) {
        ಹಿಂತಿರುಗಿ "\"" + V3_ಹೊರಹೋಗು_Formatter::quoteNameControls(ಪಠ್ಯ) + "\"";
    }

    // A 'VlFileLineDebug' argument: where the runtime reports an error about fl's construct
    ಸ್ಥಿರ AstCExpr* file_ಸಾಲು_Debug(File_ಸಾಲು* fl) {
        ಸ್ಥಿರಾಂಕ std::string filename
            = VIdProtect::protectIf(fl->filename(), v3Global.opt.protectIds());
        ಹಿಂತಿರುಗಿ ctext(fl, "VlFileLineDebug{" + quoted(filename) + ", "
                             + std::ಗಮ್ಯ_string(fl->lineno()) + "}");
    }

    // Check that an element of an array bin (bins b[] = {values/ranges}) is a two-state
    // constant value or range; false if not, after reporting it if 'report'.
    ಸ್ಥಿರ ತಾರ್ಕಿಕ checkArrayBinElement(AstCoverBin* arrayBinp, Ast_ಗಂಟು* rangep, ತಾರ್ಕಿಕ report = ನಿಜ) {
        ಆದರೆ (ಸ್ಥಿರಾಂಕ AstInsideRange* ಸ್ಥಿರಾಂಕ irp = VN_CAST(rangep, InsideRange)) {
            ಸ್ಥಿರಾಂಕ AstConst* ಸ್ಥಿರಾಂಕ minp = VN_CAST(irp->lhsp(), Const);
            ಸ್ಥಿರಾಂಕ AstConst* ಸ್ಥಿರಾಂಕ maxp = VN_CAST(irp->rhsp(), Const);
            ಆದರೆ ((!minp && !VN_IS(irp->lhsp(), Unbounded))
                || (!maxp && !VN_IS(irp->rhsp(), Unbounded))) {
                ಆದರೆ (report) {
                    arrayBinp->v3error("Non-constant expression in array bins range; "
                                       "range bounds must be constants (IEEE 1800-2023 19.5)");
                }
                ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
            }
            ಆದರೆ ((minp && minp->ಅಂಕೆ().isFourState()) || (maxp && maxp->ಅಂಕೆ().isFourState())) {
                ಆದರೆ (report) {
                    arrayBinp->v3error("Four-state (x/z) value in array bins range bound; "
                                       "range bounds must be two-state constants");
                }
                ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
            }
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (!VN_IS(rangep, Const)) {
            ಆದರೆ (report) {
                arrayBinp->v3error("Non-constant expression in array bins value list; "
                                   "values must be constants (IEEE 1800-2023 19.5)");
            }
            ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
        }
        ಹಿಂತಿರುಗಿ ನಿಜ;
    }

    // Individual equality targets of an array bin (bins b[] = {values/ranges}) of a real
    // coverpoint, in order; integral coverpoints generate array bins as runs (see arrayBinRuns).
    // An open-ended bound ('$', AstUnbounded) resolves to the coverpoint domain: '[lo:$]'
    // covers [lo:maxVal] and '[$:hi]' covers [0:hi].  One target is produced per value; ranges
    // whose resolved size would exceed --coverage-max-real-bins (e.g. an open '[lo:$]') are
    // unsupported -- emits COVERIGN, sets unsupportedOut, yields nothing.
    std::ಸದಿಶ<Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ*> ಹೊರತೆಗೆ_Array_ಮೌಲ್ಯಗಳು(AstCoverBin* arrayBinp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* exprp,
                                                 ತಾರ್ಕಿಕ& unsupported_ಹೊರಹೋಗು) {
        unsupported_ಹೊರಹೋಗು = ಸುಳ್ಳು;
        ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ ಅಗಲ = exprp->ಅಗಲ();
        ಸ್ಥಿರಾಂಕ uint64_t maxVal = (ಅಗಲ >= 64) ? UINT64_MAX : ((1ULL << ಅಗಲ) - 1);
        std::ಸದಿಶ<Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ*> ಮೌಲ್ಯಗಳು;
        ಗಾಗಿ (Ast_ಗಂಟು* rangep = arrayBinp->rangesp(); rangep; rangep = rangep->nextp()) {
            rangep = V3Const::constifyEdit(rangep);
            ಆದರೆ (!checkArrayBinElement(arrayBinp, rangep)) ಹಿಂತಿರುಗಿ ಮೌಲ್ಯಗಳು;
            ಆದರೆ (AstInsideRange* ಸ್ಥಿರಾಂಕ irp = VN_CAST(rangep, InsideRange)) {
                ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ loUnb = VN_IS(irp->lhsp(), Unbounded);
                ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ hiUnb = VN_IS(irp->rhsp(), Unbounded);
                ಸ್ಥಿರಾಂಕ uint64_t lo = loUnb ? 0 : VN_AS(irp->lhsp(), Const)->ಗಮ್ಯ_UQuad();
                ಸ್ಥಿರಾಂಕ uint64_t hi = hiUnb ? maxVal : VN_AS(irp->rhsp(), Const)->ಗಮ್ಯ_UQuad();
                ಆದರೆ (hi < lo) ಮುಂದುವರಿ;  // empty range contributes no bins
                // Guard against a '$'-bounded or otherwise huge range exploding the bin count.
                ಸ್ಥಿರಾಂಕ uint64_t ಹರವು = hi - lo;  // == valueCount - 1 (no overflow: hi >= lo)
                ಆದರೆ (ಹರವು >= v3Global.opt.coverageMaxRealBins()
                    || ಮೌಲ್ಯಗಳು.ಗಾತ್ರ_2() + ಹರವು + 1 > v3Global.opt.coverageMaxRealBins()) {
                    arrayBinp->v3warn(COVERIGN,
                                      "Unsupported: array 'bins' of a real coverpoint "
                                      "covering more than "
                                          << v3Global.opt.coverageMaxRealBins() << " values; bin "
                                          << arrayBinp->prettyNameQ() << " ignored.\n"
                                          << arrayBinp->warn_ಇನ್ನಷ್ಟು()
                                          << "... Suggest a larger --coverage-max-real-bins");
                    unsupported_ಹೊರಹೋಗು = ನಿಜ;
                    ಗಾಗಿ (Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ vp : ಮೌಲ್ಯಗಳು) VL_DO_DANGLING(ತಳ್ಳು_Deletep(vp), vp);
                    ಮೌಲ್ಯಗಳು.clear();
                    ಹಿಂತಿರುಗಿ ಮೌಲ್ಯಗಳು;
                }
                ಗಾಗಿ (uint64_t v = lo; v <= hi; ++v)
                    ಮೌಲ್ಯಗಳು.ತಳ್ಳು_ಹಿಂಭಾಗ(ಹೊಸ AstConst{irp->fileline(), AstConst::Widthed_ಮೌಲ್ಯ{}, ಅಗಲ,
                                                  static_cast<uint32_t>(v)});
            } ಇಲ್ಲದಿದ್ದರೆ {
                ಮೌಲ್ಯಗಳು.ತಳ್ಳು_ಹಿಂಭಾಗ(VN_AS(rangep->clone_ವೃಕ್ಷ(ಸುಳ್ಳು), ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ));
            }
        }
        ಹಿಂತಿರುಗಿ ಮೌಲ್ಯಗಳು;
    }

    ಸ್ಥಿರ ಪೂರ್ಣಾಂಕ ಸತತ_ಅಗಲ(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* exprp) { ಹಿಂತಿರುಗಿ exprp->ಅಗಲ() + 1; }

    // Automatic bins partition the coverpoint domain in value order (IEEE 1800-2023 19.5.3): N
    // bins, capped at the number of values, each hold 2^width / N values, and the last bin also
    // holds the remainder.  False for an invalid declaration, already reported.
    ತಾರ್ಕಿಕ autoBinRuns(AstCoverBin* binp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* exprp, BinRuns& ಹೊರಹೋಗು) {
        ಸ್ಥಿರಾಂಕ uint32_t requested = autoBinsRequested(binp);
        ಆದರೆ (!requested || !exprp->dtypep()->skipRefp()->isIntegralOrPacked()) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
        ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ ಅಗಲ = exprp->ಅಗಲ();
        ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ arithmetic_ಅಗಲ = ಸತತ_ಅಗಲ(exprp);
        ಸ್ಥಿರಾಂಕ uint32_t ಎಣಿಕೆ_2
            = ಅಗಲ < 32
                  ? static_cast<uint32_t>(std::min<uint64_t>(uint64_t{1} << ಅಗಲ, requested))
                  : requested;
        Bin_ಸತತ ಸತತ{binp, arithmetic_ಅಗಲ, ಎಣಿಕೆ_2};
        V3_ಸಂಖ್ಯೆ ಒಟ್ಟು{binp, arithmetic_ಅಗಲ};
        ಒಟ್ಟು.ಗಣ_ಬಿಟ್(ಅಗಲ, 1);
        ಸತತ.m_stride.opDiv(ಒಟ್ಟು, V3_ಸಂಖ್ಯೆ{binp, arithmetic_ಅಗಲ, ಎಣಿಕೆ_2});
        ಸ್ಥಿರಾಂಕ Cross_ಮೌಲ್ಯ_Range domain
            = cross_ಮೌಲ್ಯ_Domain(binp, ಅಗಲ, exprp->isSigned(), arithmetic_ಅಗಲ);
        ಸತತ.m_lo = domain.lo;
        ಸತತ.m_hi = domain.hi;
        ಹೊರಹೋಗು.runs.ತಳ್ಳು_ಹಿಂಭಾಗ(std::move(ಸತತ));
        ಹೊರಹೋಗು.ಎಣಿಕೆ_2 = ಎಣಿಕೆ_2;
        ಹಿಂತಿರುಗಿ ನಿಜ;
    }

    // The elements of an array bin (bins b[] = {values/ranges}), in order, as runs of
    // single-value bins.  A range holds the values of the coverpoint type it contains (IEEE
    // 1800-2023 19.5.7), while a singleton names a bin even without such a value.  Errors on a
    // non-constant element.  More than --coverage-max-bins bins (e.g. an open '[lo:$]' range over
    // a wide coverpoint) are unsupported -- emits COVERIGN, and sets unsupported.
    BinRuns arrayBinRuns(AstCoverBin* arrayBinp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* exprp) {
        BinRuns ಹೊರಹೋಗು;
        ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ ಅಗಲ = ಸತತ_ಅಗಲ(exprp);
        ಗಾಗಿ (Ast_ಗಂಟು* rangep = arrayBinp->rangesp(); rangep; rangep = rangep->nextp()) {
            rangep = V3Const::constifyEdit(rangep);
            ಆದರೆ (!checkArrayBinElement(arrayBinp, rangep)) ಹಿಂತಿರುಗಿ ಹೊರಹೋಗು;
            ಸ್ಥಿರಾಂಕ AstInsideRange* ಸ್ಥಿರಾಂಕ irp = VN_CAST(rangep, InsideRange);
            Cross_ಮೌಲ್ಯ_Range range{rangep, resolve_ಅಗಲ(rangep, exprp)};
            ತಾರ್ಕಿಕ ಖಾಲಿಗಳು = ನಿಜ;
            ಆದರೆ (!resolve_ಮೌಲ್ಯ(rangep, exprp, ನಿಜ, ಸುಳ್ಳು, range)) {
                rangep->v3warn(E_UNSUPPORTED, "Unsupported: non-integral value in a coverage bin "
                                              "of an integral coverpoint.");
            } ಇಲ್ಲದಿದ್ದರೆ {
                ಖಾಲಿಗಳು = cross_Range_ಖಾಲಿಗಳು(range);
            }
            ಆದರೆ (ಖಾಲಿಗಳು && irp) ಮುಂದುವರಿ;  // A range without values contributes no bins
            Bin_ಸತತ ಸತತ{rangep, ಅಗಲ, 1};
            ಸತತ.m_ಖಾಲಿಗಳು = ಖಾಲಿಗಳು;
            uint64_t ಎಣಿಕೆ_2 = 1;
            ಆದರೆ (!ಖಾಲಿಗಳು) {
                ಸತತ.m_lo.op_ನಿಯೋಜಿಸು(range.lo);
                ಸತತ.m_hi.op_ನಿಯೋಜಿಸು(range.hi);
                V3_ಸಂಖ್ಯೆ ಹರವು{rangep, ಅಗಲ};
                ಹರವು.opSub(ಸತತ.m_hi, ಸತತ.m_lo);
                // Wider spans exceed any limit
                ಎಣಿಕೆ_2 = ಹರವು.most_ಗಣ_ಬಿಟ್_P1() > 32 ? UINT64_MAX : ಹರವು.ಗಮ್ಯ_UQuad() + 1;
            }
            ಆದರೆ (ಎಣಿಕೆ_2 > v3Global.opt.coverageMaxBins() - ಹೊರಹೋಗು.ಎಣಿಕೆ_2) {
                arrayBinp->v3warn(COVERIGN, "Unsupported: array 'bins' covering more than "
                                                << v3Global.opt.coverageMaxBins()
                                                << " values (e.g. an open '[lo:$]' range over "
                                                   "a wide coverpoint); bin "
                                                << arrayBinp->prettyNameQ() << " ignored\n"
                                                << arrayBinp->warn_ಇನ್ನಷ್ಟು()
                                                << "... Suggest a larger --coverage-max-bins");
                ಹೊರಹೋಗು.runs.clear();
                ಹೊರಹೋಗು.ಎಣಿಕೆ_2 = 0;
                ಹೊರಹೋಗು.unsupported = ನಿಜ;
                ಹಿಂತಿರುಗಿ ಹೊರಹೋಗು;
            }
            ಸತತ.m_ಎಣಿಕೆ = static_cast<uint32_t>(ಎಣಿಕೆ_2);
            ಹೊರಹೋಗು.ಎಣಿಕೆ_2 += ಸತತ.m_ಎಣಿಕೆ;
            ಹೊರಹೋಗು.runs.ತಳ್ಳು_ಹಿಂಭಾಗ(std::move(ಸತತ));
        }
        ಹಿಂತಿರುಗಿ ಹೊರಹೋಗು;
    }

    // The bins of a wildcard array (wildcard bins b[] = {...}): one for each coverpoint value
    // an element matches (IEEE 1800-2023 19.5.4, 19.5.7), in value order, and named by the value
    // (19.5.1), as runs of single-value bins.  Errors on a non-constant element.  More than
    // --coverage-max-bins values are unsupported -- emits COVERIGN, and sets unsupported, and for
    // an ignore or illegal array, single.  'report' false omits these diagnostics.
    ಸ್ಥಿರ BinRuns wildcardBinRuns(AstCoverBin* arrayBinp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* exprp, ತಾರ್ಕಿಕ report) {
        BinRuns ಹೊರಹೋಗು;
        ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ ಅಗಲ = ಸತತ_ಅಗಲ(exprp);
        ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ one{arrayBinp, ಅಗಲ, 1};
        // Disjoint runs of the values, in value order, each value once
        std::ಸದಿಶ<std::pair<V3_ಸಂಖ್ಯೆ, V3_ಸಂಖ್ಯೆ>> spans;
        uint64_t ಎಣಿಕೆ_2 = 0;  // Values of 'spans'
        ಗಾಗಿ (Ast_ಗಂಟು* rangep = arrayBinp->rangesp(); rangep; rangep = rangep->nextp()) {
            rangep = V3Const::constifyEdit(rangep);
            ಆದರೆ (!checkArrayBinElement(arrayBinp, rangep, report)) {
                ಹೊರಹೋಗು.unsupported = ನಿಜ;
                ಹಿಂತಿರುಗಿ ಹೊರಹೋಗು;
            }
            Cross_ಮೌಲ್ಯ_Range range{rangep, resolve_ಅಗಲ(rangep, exprp)};
            ಆದರೆ (!resolve_ಮೌಲ್ಯ(rangep, exprp, ನಿಜ, ನಿಜ, range)) {
                ಆದರೆ (report) {
                    rangep->v3warn(E_UNSUPPORTED, "Unsupported: non-integral value in a "
                                                  "coverage bin of an integral coverpoint.");
                }
                ಮುಂದುವರಿ;
            }
            ಆದರೆ (cross_Range_ಖಾಲಿಗಳು(range)) ಮುಂದುವರಿ;
            std::ಸದಿಶ<std::pair<V3_ಸಂಖ್ಯೆ, V3_ಸಂಖ್ಯೆ>> ಸಿಕ್ಕಿತು;
            crossRangeRuns(range, v3Global.opt.coverageMaxBins(), ಸಿಕ್ಕಿತು);
            ಗಾಗಿ (ಸ್ಥಿರಾಂಕ std::pair<V3_ಸಂಖ್ಯೆ, V3_ಸಂಖ್ಯೆ>& ಸತತ : ಸಿಕ್ಕಿತು) {
                // Coverpoint values, sign-extended in both widths
                spans.emplace_ಹಿಂಭಾಗ(V3_ಸಂಖ್ಯೆ{rangep, ಅಗಲ, ಸತತ.ಮೊದಲ},
                                   V3_ಸಂಖ್ಯೆ{rangep, ಅಗಲ, ಸತತ.ಎರಡನೇ});
            }
            std::ವಿಂಗಡಿಸು(spans.begin(), spans.ಕೊನೆ(), [](ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ& lhs, ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ& rhs) {
                ಹಿಂತಿರುಗಿ cross_ಮೌಲ್ಯ_Less(lhs.ಮೊದಲ, rhs.ಮೊದಲ);
            });
            std::ಸದಿಶ<std::pair<V3_ಸಂಖ್ಯೆ, V3_ಸಂಖ್ಯೆ>> merged;
            ಗಾಗಿ (ಸ್ಥಿರಾಂಕ std::pair<V3_ಸಂಖ್ಯೆ, V3_ಸಂಖ್ಯೆ>& ಹರವು : spans) {
                // Adjacent or overlapping values join a run; 'first - 1' cannot overflow
                V3_ಸಂಖ್ಯೆ before{rangep, ಅಗಲ};
                before.opSub(ಹರವು.ಮೊದಲ, one);
                ಆದರೆ (merged.ಖಾಲಿಗಳು() || cross_ಮೌಲ್ಯ_Less(merged.ಹಿಂಭಾಗ().ಎರಡನೇ, before)) {
                    merged.ತಳ್ಳು_ಹಿಂಭಾಗ(ಹರವು);
                } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (cross_ಮೌಲ್ಯ_Less(merged.ಹಿಂಭಾಗ().ಎರಡನೇ, ಹರವು.ಎರಡನೇ)) {
                    merged.ಹಿಂಭಾಗ().ಎರಡನೇ = ಹರವು.ಎರಡನೇ;
                }
            }
            ಎಣಿಕೆ_2 = 0;
            ಗಾಗಿ (ಸ್ಥಿರಾಂಕ std::pair<V3_ಸಂಖ್ಯೆ, V3_ಸಂಖ್ಯೆ>& ಹರವು : merged) {
                V3_ಸಂಖ್ಯೆ ಗಾತ್ರ_2{rangep, ಅಗಲ};
                ಗಾತ್ರ_2.opSub(ಹರವು.ಎರಡನೇ, ಹರವು.ಮೊದಲ);
                // Beyond 2^32 values exceed any limit
                ಎಣಿಕೆ_2 += ಗಾತ್ರ_2.most_ಗಣ_ಬಿಟ್_P1() > 32 ? uint64_t{1} << 33 : ಗಾತ್ರ_2.ಗಮ್ಯ_UQuad() + 1;
                ಆದರೆ (ಎಣಿಕೆ_2 > v3Global.opt.coverageMaxBins()) ನಿಲ್ಲಿಸು;
            }
            spans = std::move(merged);
            ಆದರೆ (ಎಣಿಕೆ_2 > v3Global.opt.coverageMaxBins()) {
                // An ignore or illegal array still excludes or checks its values, as one bin
                ಹೊರಹೋಗು.single = !arrayBinp->binsType().bin_Is_ಸಾಮಾನ್ಯ();
                ಆದರೆ (report) {
                    arrayBinp->v3warn(
                        COVERIGN, "Unsupported: wildcard array '"
                                      << arrayBinp->binsType().verilogKwd()
                                      << "' of more than --coverage-max-bins of "
                                      << v3Global.opt.coverageMaxBins() << " values; bin "
                                      << arrayBinp->prettyNameQ()
                                      << (ಹೊರಹೋಗು.single ? " treated as one bin" : " ignored") << "\n"
                                      << arrayBinp->warn_ಇನ್ನಷ್ಟು()
                                      << "... Suggest a larger --coverage-max-bins");
                }
                ಹೊರಹೋಗು.unsupported = ನಿಜ;
                ಹಿಂತಿರುಗಿ ಹೊರಹೋಗು;
            }
        }
        ಗಾಗಿ (ಸ್ಥಿರಾಂಕ std::pair<V3_ಸಂಖ್ಯೆ, V3_ಸಂಖ್ಯೆ>& ಹರವು : spans) {
            ಹೊರಹೋಗು.runs.emplace_ಹಿಂಭಾಗ(arrayBinp, ಅಗಲ, 0);
            Bin_ಸತತ& ಸತತ = ಹೊರಹೋಗು.runs.ಹಿಂಭಾಗ();
            ಸತತ.m_lo = ಹರವು.ಮೊದಲ;
            ಸತತ.m_hi = ಹರವು.ಎರಡನೇ;
            // A bin for each value, named by it in the coverpoint's type
            V3_ಸಂಖ್ಯೆ ಮೌಲ್ಯ = ಹರವು.ಮೊದಲ;
            ಆಗ (ನಿಜ) {
                ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ typed{arrayBinp, exprp->ಅಗಲ(), ಮೌಲ್ಯ};
                ಹೊರಹೋಗು.ಮೌಲ್ಯಗಳು.ತಳ್ಳು_ಹಿಂಭಾಗ(exprp->isSigned() ? typed.ಗಮ್ಯ_Decimal_S() : typed.ಗಮ್ಯ_Decimal_U());
                ++ಸತತ.m_ಎಣಿಕೆ;
                ಆದರೆ (ಮೌಲ್ಯ.isCaseEq(ಹರವು.ಎರಡನೇ)) ನಿಲ್ಲಿಸು;
                V3_ಸಂಖ್ಯೆ ಮುಂದಿನ{arrayBinp, ಅಗಲ};
                ಮೌಲ್ಯ = ಮುಂದಿನ.op_ಕೂಡಿಸು(ಮೌಲ್ಯ, one);
            }
        }
        ಹೊರಹೋಗು.ಎಣಿಕೆ_2 = static_cast<uint32_t>(ಎಣಿಕೆ_2);
        ಹಿಂತಿರುಗಿ ಹೊರಹೋಗು;
    }

    // The runs of an automatic bins declaration, or of an array bin of an integral coverpoint.
    // False for other bins, which do not generate as runs, including a wildcard array then one
    // bin (see BinRuns::single).
    ತಾರ್ಕಿಕ binRunsFor(AstCoverBin* binp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* exprp, BinRuns& ಹೊರಹೋಗು) {
        ಆದರೆ (isAutoBins(binp)) {
            ಆದರೆ (!autoBinRuns(binp, exprp, ಹೊರಹೋಗು)) ಹೊರಹೋಗು.unsupported = ನಿಜ;
            ಹಿಂತಿರುಗಿ ನಿಜ;
        }
        ಆದರೆ (binp->isArray() && binp->isWildcard()) {
            ಆದರೆ (!exprp->dtypep()->skipRefp()->isIntegralOrPacked()) {
                Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ falsep = wildcardTypeError(binp, exprp);
                VL_DO_DANGLING(ತಳ್ಳು_Deletep(falsep), falsep);
                ಹೊರಹೋಗು.unsupported = ನಿಜ;
            } ಇಲ್ಲದಿದ್ದರೆ {
                ಹೊರಹೋಗು = wildcardBinRuns(binp, exprp, ನಿಜ);
                ಆದರೆ (ಹೊರಹೋಗು.single) {
                    binp->isArray(ಸುಳ್ಳು);  // Generates as one bin
                    ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
                }
            }
            ಹಿಂತಿರುಗಿ ನಿಜ;
        }
        ಆದರೆ (!binp->isArray() || binp->transp() || binp->isWildcard()
            || !exprp->dtypep()->skipRefp()->isIntegralOrPacked()) {
            ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
        }
        ಹೊರಹೋಗು = arrayBinRuns(binp, exprp);
        ಹಿಂತಿರುಗಿ ನಿಜ;
    }

    // Emit a 'this->m_cp->addSingleNamer/addArrayNamer(...)' statement for one bin whose first
    // runtime bin index is 'declared'; or with 'valueNames', those naming each bin of an array
    Ast_ಗಂಟು_Stmt* makeNamer(AstVar* cpVarp, AstCoverBin* binp, int64_t ಎಣಿಕೆ_2, uint32_t declared,
                           ಸ್ಥಿರಾಂಕ std::ಸದಿಶ<Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ*>& ಮೌಲ್ಯಗಳು = {},
                           ಸ್ಥಿರಾಂಕ std::ಸದಿಶ<std::string>& ಮೌಲ್ಯ_ಹೆಸರುಗಳು = {}) {
        File_ಸಾಲು* ಸ್ಥಿರಾಂಕ fl = binp->fileline();
        CoverpointBins& bins = m_cpBins.at(cpVarp);
        ಸ್ಥಿರಾಂಕ uint32_t ಸಾಮಾನ್ಯ_ಎಣಿಕೆ
            = binp->binsType().bin_Is_ಸಾಮಾನ್ಯ() ? static_cast<uint32_t>(ಎಣಿಕೆ_2 < 0 ? 1 : ಎಣಿಕೆ_2) : 0;
        ಸ್ಥಿರಾಂಕ Bin_ಹರವು ಹರವು{bins.ಒಟ್ಟು, ಸಾಮಾನ್ಯ_ಎಣಿಕೆ, declared};
        ಆದರೆ (binp->binsType() == VCoverBinsType::BINS_AUTO_IMPLICIT) {
            bins.implicitAuto = ಹರವು;  // Selected by bin, see implicitAutoBinSpan
        } ಇಲ್ಲದಿದ್ದರೆ {
            bins.spans.emplace(binp->name(), ಹರವು);
        }
        bins.ಒಟ್ಟು += ಸಾಮಾನ್ಯ_ಎಣಿಕೆ;
        ಗಾಗಿ (uint32_t i = 0; bins.crossed && i < ಸಾಮಾನ್ಯ_ಎಣಿಕೆ; ++i) {
            bins.ಮೌಲ್ಯಗಳು.ತಳ್ಳು_ಹಿಂಭಾಗ({binp, ಮೌಲ್ಯಗಳು.ಖಾಲಿಗಳು() ? ಶೂನ್ಯಸೂಚಕ : ಮೌಲ್ಯಗಳು[i]});
        }
        // Under --protect-ids the filename and bin name flow into the coverage database
        // verbatim, so obfuscate them exactly as line/toggle coverage points are (whole-
        // unit filename, per-word bin name).  A no-op when --protect-ids is off.
        ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ prot = v3Global.opt.protectIds();
        ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ single = ಎಣಿಕೆ_2 < 0;
        ಆದರೆ (!ಮೌಲ್ಯ_ಹೆಸರುಗಳು.ಖಾಲಿಗಳು()) {
            // A bin of a wildcard array is named by its value (IEEE 1800-2023 19.5.1)
            Ast_ಗಂಟು_Stmt* stmtsp = ಶೂನ್ಯಸೂಚಕ;
            ಗಾಗಿ (ಸ್ಥಿರಾಂಕ std::string& ಮೌಲ್ಯ : ಮೌಲ್ಯ_ಹೆಸರುಗಳು) {
                ಸ್ಥಿರಾಂಕ std::string name
                    = VIdProtect::protect_ಪದಗಳು_If(binp->name(), prot) + "[" + ಮೌಲ್ಯ + "]";
                stmtsp = Ast_ಗಂಟು::ಕೂಡಿಸು_ಮುಂದಿನ(
                    stmtsp,
                    ಅಂಶ_Call(fl, cpVarp, VCMethod::COVERGROUP_ಕೂಡಿಸು_SINGLE_NAMER,
                             {ctext(fl, binp->binsType().bin_ಗಣ_Enum()), ctext(fl, quoted(name)),
                              ctext(fl, quoted(VIdProtect::protectIf(fl->filename(), prot))),
                              cnum(fl, static_cast<uint32_t>(fl->lineno())),
                              cnum(fl, static_cast<uint32_t>(fl->ಮೊದಲ_ಸ್ತಂಭ()))})
                        ->makeStmt());
            }
            ಹಿಂತಿರುಗಿ stmtsp;
        }
        std::ಸದಿಶ<Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ*> ವಾದಗಳು{ctext(fl, binp->binsType().bin_ಗಣ_Enum())};
        ಆದರೆ (!single) ವಾದಗಳು.ತಳ್ಳು_ಹಿಂಭಾಗ(cnum(fl, static_cast<uint32_t>(ಎಣಿಕೆ_2)));  // value array bin
        ವಾದಗಳು.ತಳ್ಳು_ಹಿಂಭಾಗ(ctext(fl, quoted(VIdProtect::protect_ಪದಗಳು_If(binp->name(), prot))));
        ವಾದಗಳು.ತಳ್ಳು_ಹಿಂಭಾಗ(ctext(fl, quoted(VIdProtect::protectIf(fl->filename(), prot))));
        ವಾದಗಳು.ತಳ್ಳು_ಹಿಂಭಾಗ(cnum(fl, static_cast<uint32_t>(fl->lineno())));
        ವಾದಗಳು.ತಳ್ಳು_ಹಿಂಭಾಗ(cnum(fl, static_cast<uint32_t>(fl->ಮೊದಲ_ಸ್ತಂಭ())));
        ಹಿಂತಿರುಗಿ ಅಂಶ_Call(fl, cpVarp,
                        single ? VCMethod::COVERGROUP_ಕೂಡಿಸು_SINGLE_NAMER
                        : binp->binsType() == VCoverBinsType::BINS_AUTO_IMPLICIT
                            ? VCMethod::COVERGROUP_ಕೂಡಿಸು_NUMBERED_NAMER
                            : VCMethod::COVERGROUP_ಕೂಡಿಸು_ARRAY_NAMER,
                        ವಾದಗಳು)
            ->makeStmt();
    }

    // Emit 'if (iff && cond) m_cp.incrementBin(idx);' (or recordHit, + illegal action) in sample()
    // Where a bin's hit is recorded in the runtime VlCoverpoint member.
    ರಚನೆ Conv_Bin_ಗುರಿ final {
        AstVar* cpVarp;  // the __Vcp_<coverpoint> member
        uint32_t idx;  // bin index within that coverpoint
        ತಾರ್ಕಿಕ is_ಸಾಮಾನ್ಯ;  // Normal -> incrementBin (count + cross hit list); else recordHit (count)
    };

    // Emit 'this->m_cp.incrementBin(idx);' (Normal) or '.recordHit(idx);'
    // (ignore/illegal/default).
    Ast_ಗಂಟು_Stmt* make_Runtime_Bin_ದೊರಕಿತು(File_ಸಾಲು* fl, AstVar* cpVarp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* idxp,
                                   ತಾರ್ಕಿಕ is_ಸಾಮಾನ್ಯ) {
        ಹಿಂತಿರುಗಿ ಅಂಶ_Call(fl, cpVarp,
                        is_ಸಾಮಾನ್ಯ ? VCMethod::COVERGROUP_INCREMENT_BIN
                                 : VCMethod::COVERGROUP_ದಾಖಲೆ_ದೊರಕಿತು,
                        {idxp})
            ->makeStmt();
    }
    Ast_ಗಂಟು_Stmt* make_Runtime_Bin_ದೊರಕಿತು(File_ಸಾಲು* fl, ಸ್ಥಿರಾಂಕ Conv_Bin_ಗುರಿ& tgt) {
        ಹಿಂತಿರುಗಿ make_Runtime_Bin_ದೊರಕಿತು(fl, tgt.cpVarp, cnum(fl, static_cast<uint32_t>(tgt.idx)),
                                 tgt.is_ಸಾಮಾನ್ಯ);
    }

    // The condition under which a bin counts a sample: condp, and the bin's iff, and for a
    // Normal or default state bin, that the value is not excluded
    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* binCondition(AstCoverBin* binp, AstVar* cpVarp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* condp) {
        File_ಸಾಲು* ಸ್ಥಿರಾಂಕ fl = binp->fileline();
        ಆದರೆ (binp->iffp()) condp = ಹೊಸ AstLogAnd{fl, binp->iffp()->clone_ವೃಕ್ಷ(ಸುಳ್ಳು), condp};
        ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ excluded = m_excludedVars.ಕಂಡುಹಿಡಿ(cpVarp);
        ಆದರೆ (excluded != m_excludedVars.ಕೊನೆ() && !binp->transp()
            && (binp->binsType().bin_Is_ಸಾಮಾನ್ಯ()
                || binp->binsType() == VCoverBinsType::BINS_DEFAULT)) {
            condp = ಹೊಸ AstLogAnd{
                fl, ಹೊಸ AstNot{fl, ಹೊಸ AstVarRef{fl, excluded->ಎರಡನೇ, VAccess::READ}}, condp};
        }
        ಹಿಂತಿರುಗಿ condp;
    }

    ಶೂನ್ಯ emit_Conv_ದೊರಕಿತು_If(AstCoverpoint* coverpointp, AstCoverBin* binp, AstVar* cpVarp,
                       Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* idxp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* condp) {
        File_ಸಾಲು* ಸ್ಥಿರಾಂಕ fl = binp->fileline();
        Ast_ಗಂಟು* actionp = make_Runtime_Bin_ದೊರಕಿತು(fl, cpVarp, idxp, binp->binsType().bin_Is_ಸಾಮಾನ್ಯ());
        ಆದರೆ (binp->binsType() == VCoverBinsType::BINS_ILLEGAL) {
            actionp->ಕೂಡಿಸು_ಮುಂದಿನ(makeIllegalBinAction(fl, "Illegal bin " + binp->prettyNameQ()
                                                          + " hit in coverpoint "
                                                          + coverpointp->prettyNameQ()));
        }
        UASSERT_OBJ(m_sampleFuncp, binp, "sample() CFunc not set for coverpoint");
        ಕೂಡಿಸು_Sample_Stmt(ಹೊಸ AstIf{fl, binCondition(binp, cpVarp, condp), actionp, ಶೂನ್ಯಸೂಚಕ});
    }

    // Emit the sample() code of sized array 'sized': count the value in its bins holding it when
    // enabled, as other bins are, and for default bins note in matchedp whether any holds it
    ಶೂನ್ಯ emitSizedSample(AstCoverpoint* coverpointp, AstCoverBin* binp, AstVar* cpVarp,
                         Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* exprp, uint32_t sized, AstVar* matchedp) {
        File_ಸಾಲು* ಸ್ಥಿರಾಂಕ fl = binp->fileline();
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* enabledp = binCondition(binp, cpVarp, ಹೊಸ AstConst{fl, AstConst::ಬಿಟ್_True{}});
        UASSERT_OBJ(m_sampleFuncp, binp, "sample() CFunc not set for coverpoint");
        ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ illegal = binp->binsType() == VCoverBinsType::BINS_ILLEGAL;
        ಆದರೆ (illegal) {
            // The illegal action reads the condition too, which is evaluated once, as the guard
            // may have side effects
            AstVar* ಸ್ಥಿರಾಂಕ varp
                = ಹೊಸ AstVar{fl, VVarType::BLOCKTEMP,
                             "__VcpEnabled_" + sanitizeGeneratedName(coverpointp->name()) + "_"
                                 + cvt_ಗಮ್ಯ_Str(sized),
                             binp->ಕಂಡುಹಿಡಿ_ಬಿಟ್_DType()};
            varp->funcLocal(ನಿಜ);
            m_sampleFuncp->ಕೂಡಿಸು_Stmtsp(varp);
            ಕೂಡಿಸು_Sample_Stmt(ಹೊಸ Ast_ನಿಯೋಜಿಸು{fl, ಹೊಸ AstVarRef{fl, varp, VAccess::WRITE}, enabledp});
            enabledp = ಹೊಸ AstVarRef{fl, varp, VAccess::READ};
        }
        AstCMethodHard* ಸ್ಥಿರಾಂಕ callp
            = ಅಂಶ_Call(fl, cpVarp,
                       exprp->isWide() ? VCMethod::COVERGROUP_SIZED_SAMPLE_W
                                       : VCMethod::COVERGROUP_SIZED_SAMPLE,
                       {cnum(fl, sized), exprp->clone_ವೃಕ್ಷ(ಸುಳ್ಳು), enabledp});
        callp->dtype_ಗಣ_ಬಿಟ್();
        ಆದರೆ (illegal) {
            ಕೂಡಿಸು_Sample_Stmt(ಹೊಸ AstIf{fl, ಹೊಸ AstLogAnd{fl, callp, enabledp->clone_ವೃಕ್ಷ(ಸುಳ್ಳು)},
                                    makeIllegalBinAction(fl, "Illegal bin " + binp->prettyNameQ()
                                                                 + " hit in coverpoint "
                                                                 + coverpointp->prettyNameQ())});
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (matchedp && binp->binsType().bin_Is_ಸಾಮಾನ್ಯ()) {
            ಕೂಡಿಸು_Sample_Stmt(
                ಹೊಸ Ast_ನಿಯೋಜಿಸು{fl, ಹೊಸ AstVarRef{fl, matchedp, VAccess::WRITE},
                              ಹೊಸ AstOr{fl, ಹೊಸ AstVarRef{fl, matchedp, VAccess::READ}, callp}});
        } ಇಲ್ಲದಿದ್ದರೆ {
            ಕೂಡಿಸು_Sample_Stmt(callp->makeStmt());
        }
    }

    // A variable local to the constructor
    AstVar* constructor_ತಾತ್ಕಾಲಿಕ(File_ಸಾಲು* fl, ಸ್ಥಿರಾಂಕ string& name, Ast_ಗಂಟು_DType* dtypep) {
        AstVar* ಸ್ಥಿರಾಂಕ varp = ಹೊಸ AstVar{fl, VVarType::BLOCKTEMP, name, dtypep};
        varp->funcLocal(ನಿಜ);
        m_constructorp->ಕೂಡಿಸು_Stmtsp(varp);
        ಹಿಂತಿರುಗಿ varp;
    }

    // Truncate or extend, as its signedness sets, a value to a type
    ಸ್ಥಿರ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* resize_ಮೌಲ್ಯ(Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* valuep, Ast_ಗಂಟು_DType* dtypep) {
        File_ಸಾಲು* ಸ್ಥಿರಾಂಕ fl = valuep->fileline();
        ಆದರೆ (valuep->ಅಗಲ() > dtypep->ಅಗಲ()) {
            valuep = ಹೊಸ AstSel{fl, valuep, 0, dtypep->ಅಗಲ()};
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (valuep->ಅಗಲ() < dtypep->ಅಗಲ()) {
            valuep = valuep->isSigned()
                         ? static_cast<Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ*>(ಹೊಸ AstExtendS{fl, valuep, dtypep->ಅಗಲ()})
                         : ಹೊಸ AstExtend{fl, valuep, dtypep->ಅಗಲ()};
        }
        valuep->dtypep(dtypep);
        ಹಿಂತಿರುಗಿ valuep;
    }

    // The runtime arguments of a count, which the constructor evaluates once into a temporary:
    // its value, a count wider than 64 bits saturated over any limit, and whether it is positive
    std::pair<Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ*, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ*> new_ಎಣಿಕೆ_ವಾದಗಳು(File_ಸಾಲು* fl, ಸ್ಥಿರಾಂಕ string& name,
                                                       Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* exprp) {
        AstVar* ಸ್ಥಿರಾಂಕ countp = constructor_ತಾತ್ಕಾಲಿಕ(fl, name, exprp->dtypep());
        m_constructorp->ಕೂಡಿಸು_Stmtsp(
            ಹೊಸ Ast_ನಿಯೋಜಿಸು{fl, ಹೊಸ AstVarRef{fl, countp, VAccess::WRITE}, exprp->clone_ವೃಕ್ಷ(ಸುಳ್ಳು)});
        ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ ಎಣಿಕೆ_Ref = [&]() { ಹಿಂತಿರುಗಿ ಹೊಸ AstVarRef{fl, countp, VAccess::READ}; };
        AstConst* ಸ್ಥಿರಾಂಕ zerop = ಹೊಸ AstConst{fl, AstConst::DTyped{}, countp->dtypep()};
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ positivep
            = countp->isSigned() ? static_cast<Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ*>(ಹೊಸ AstGtS{fl, ಎಣಿಕೆ_Ref(), zerop})
                                 : ಹೊಸ AstNeq{fl, ಎಣಿಕೆ_Ref(), zerop};
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* valuep = resize_ಮೌಲ್ಯ(ಎಣಿಕೆ_Ref(), countp->ಕಂಡುಹಿಡಿ_UInt64_DType());
        ಆದರೆ (countp->ಅಗಲ() > VL_QUADSIZE) {
            valuep = ಹೊಸ AstCond{
                fl,
                ಹೊಸ AstRedOr{
                    fl, ಹೊಸ AstSel{fl, ಎಣಿಕೆ_Ref(), VL_QUADSIZE, countp->ಅಗಲ() - VL_QUADSIZE}},
                ಹೊಸ AstConst{fl, AstConst::Unsized64{}, std::numeric_limits<uint64_t>::max()},
                valuep};
            valuep->dtype_ಗಣ_UInt64();
        }
        ಹಿಂತಿರುಗಿ {valuep, positivep};
    }
    // The runtime arguments of a constant count
    ಸ್ಥಿರ std::pair<Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ*, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ*> new_ಎಣಿಕೆ_ವಾದಗಳು(File_ಸಾಲು* fl, uint64_t ಎಣಿಕೆ_2) {
        ಹಿಂತಿರುಗಿ {ಹೊಸ AstConst{fl, AstConst::Unsized64{}, ಎಣಿಕೆ_2},
                ಹೊಸ AstConst{fl, AstConst::ಬಿಟ್_True{}}};
    }

    // The values of a coverpoint, which its name denotes (IEEE 1800-2023 19.5.1.1), as runs in
    // value order: an enumerated type's values (6.19), else all values of its type.  Bounds
    // are at runWidth(), sign-extended like a CrossValueRange's.
    ಸ್ಥಿರ std::ಸದಿಶ<std::pair<V3_ಸಂಖ್ಯೆ, V3_ಸಂಖ್ಯೆ>> coverpoint_ಮೌಲ್ಯಗಳು(Ast_ಗಂಟು* nodep,
                                                                       Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* exprp) {
        ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ ಅಗಲ = ಸತತ_ಅಗಲ(exprp);
        std::ಸದಿಶ<std::pair<V3_ಸಂಖ್ಯೆ, V3_ಸಂಖ್ಯೆ>> runs;
        ಸ್ಥಿರಾಂಕ AstEnumDType* ಸ್ಥಿರಾಂಕ enump = VN_CAST(exprp->dtypep()->skip_Ref_ಗಮ್ಯ_Enump(), EnumDType);
        ಆದರೆ (!enump) {
            ಸ್ಥಿರಾಂಕ Cross_ಮೌಲ್ಯ_Range domain
                = cross_ಮೌಲ್ಯ_Domain(nodep, exprp->ಅಗಲ(), exprp->isSigned(), ಅಗಲ);
            runs.emplace_ಹಿಂಭಾಗ(domain.lo, domain.hi);
            ಹಿಂತಿರುಗಿ runs;
        }
        std::ಸದಿಶ<V3_ಸಂಖ್ಯೆ> ಮೌಲ್ಯಗಳು;
        ಗಾಗಿ (ಸ್ಥಿರಾಂಕ Ast_Enum_ಅಂಶ* itemp = enump->itemsp(); itemp;
             itemp = VN_AS(itemp->nextp(), Enum_ಅಂಶ)) {
            ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& ಅಂಕೆ = VN_AS(itemp->valuep(), Const)->ಅಂಕೆ();
            ಆದರೆ (ಅಂಕೆ.isFourState()) ಮುಂದುವರಿ;  // Not a coverpoint value (19.5.7)
            ಮೌಲ್ಯಗಳು.emplace_ಹಿಂಭಾಗ(nodep, ಅಗಲ);
            ಆದರೆ (exprp->isSigned()) {
                ಮೌಲ್ಯಗಳು.ಹಿಂಭಾಗ().opExtendS(ಅಂಕೆ, ಅಂಕೆ.ಅಗಲ());
            } ಇಲ್ಲದಿದ್ದರೆ {
                ಮೌಲ್ಯಗಳು.ಹಿಂಭಾಗ().op_ನಿಯೋಜಿಸು(ಅಂಕೆ);
            }
        }
        std::ವಿಂಗಡಿಸು(ಮೌಲ್ಯಗಳು.begin(), ಮೌಲ್ಯಗಳು.ಕೊನೆ(), cross_ಮೌಲ್ಯ_Less);
        ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ one{nodep, ಅಗಲ, 1};
        ಗಾಗಿ (ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& ಮೌಲ್ಯ : ಮೌಲ್ಯಗಳು) {
            V3_ಸಂಖ್ಯೆ ಮುಂದಿನ{nodep, ಅಗಲ};
            ಆದರೆ (!runs.ಖಾಲಿಗಳು() && ಮುಂದಿನ.op_ಕೂಡಿಸು(runs.ಹಿಂಭಾಗ().ಎರಡನೇ, one).isCaseEq(ಮೌಲ್ಯ)) {
                runs.ಹಿಂಭಾಗ().ಎರಡನೇ = ಮೌಲ್ಯ;
            } ಇಲ್ಲದಿದ್ದರೆ {
                runs.emplace_ಹಿಂಭಾಗ(ಮೌಲ್ಯ, ಮೌಲ್ಯ);
            }
        }
        ಹಿಂತಿರುಗಿ runs;
    }

    // Emit the constructor code building the bins 'binp' whose values it computes: the values of
    // each element that are coverpoint values (IEEE 1800-2023 19.5.7), those a 'with' filter
    // keeps (19.5.1.1), then its bins
    ಶೂನ್ಯ generateConstructedBins(AstCoverpoint* coverpointp, AstCoverBin* binp, AstVar* cpVarp,
                                 Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* exprp) {
        File_ಸಾಲು* ಸ್ಥಿರಾಂಕ fl = binp->fileline();
        ಸ್ಥಿರಾಂಕ string ಪೂರ್ವಪ್ರತ್ಯಯ
            = m_sized_ಹೆಸರುಗಳು.get(sanitizeGeneratedName(coverpointp->name() + "__" + binp->name()));
        // The count, and whether it is positive; one of a filter's scalar bin, or bin per value
        ಸ್ಥಿರಾಂಕ std::pair<Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ*, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ*> ಎಣಿಕೆ_2
            = binp->arraySizep() ? new_ಎಣಿಕೆ_ವಾದಗಳು(fl, ಪೂರ್ವಪ್ರತ್ಯಯ + "_count", binp->arraySizep())
                                 : new_ಎಣಿಕೆ_ವಾದಗಳು(fl, 1);
        AstCoverWith* ಸ್ಥಿರಾಂಕ withp = binWith(binp);
        ಆದರೆ (withp && !binRangesp(binp)) {  // The coverpoint's name: all of its values
            ಗಾಗಿ (ಸ್ಥಿರಾಂಕ std::pair<V3_ಸಂಖ್ಯೆ, V3_ಸಂಖ್ಯೆ>& ಸತತ : coverpoint_ಮೌಲ್ಯಗಳು(binp, exprp)) {
                m_constructorp->ಕೂಡಿಸು_Stmtsp(ಅಂಶ_Call(fl, cpVarp,
                                                   exprp->isWide()
                                                       ? VCMethod::COVERGROUP_SIZED_RANGE_W
                                                       : VCMethod::COVERGROUP_SIZED_RANGE,
                                                   {new_ಮೌಲ್ಯ_Const(fl, ಸತತ.ಮೊದಲ, exprp),
                                                    new_ಮೌಲ್ಯ_Const(fl, ಸತತ.ಎರಡನೇ, exprp)})
                                              ->makeStmt());
            }
        }
        uint32_t element = 0;
        ಗಾಗಿ (Ast_ಗಂಟು* rangep = binRangesp(binp); rangep; rangep = rangep->nextp()) {
            ಆದರೆ (VN_IS(rangep, Unbounded)) {  // A parameter of '$'
                binp->v3error("Bins value may not be '$', which may only bound a range "
                              "(IEEE 1800-2023 6.20.7)");
                ಮುಂದುವರಿ;
            }
            generateSizedElement(cpVarp, binp, rangep, exprp, ಪೂರ್ವಪ್ರತ್ಯಯ + "_" + cvt_ಗಮ್ಯ_Str(element++));
        }
        ಆದರೆ (withp) generateWithFilter(binp, withp, cpVarp, exprp, ಪೂರ್ವಪ್ರತ್ಯಯ);
        std::ಸದಿಶ<Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ*> ವಾದಗಳು{ctext(fl, binp->binsType().bin_ಗಣ_Enum()), ಎಣಿಕೆ_2.ಮೊದಲ,
                                       ಎಣಿಕೆ_2.ಎರಡನೇ};
        // A filter's bins have the limit it began with
        ಆದರೆ (!withp) ವಾದಗಳು.ತಳ್ಳು_ಹಿಂಭಾಗ(cnum(fl, v3Global.opt.coverageMaxBins()));
        ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ prot = v3Global.opt.protectIds();
        ವಾದಗಳು.ತಳ್ಳು_ಹಿಂಭಾಗ(ctext(fl, quoted(VIdProtect::protect_ಪದಗಳು_If(binp->name(), prot))));
        ವಾದಗಳು.ತಳ್ಳು_ಹಿಂಭಾಗ(ctext(fl, quoted(VIdProtect::protectIf(fl->filename(), prot))));
        ವಾದಗಳು.ತಳ್ಳು_ಹಿಂಭಾಗ(cnum(fl, static_cast<uint32_t>(fl->lineno())));
        ವಾದಗಳು.ತಳ್ಳು_ಹಿಂಭಾಗ(cnum(fl, static_cast<uint32_t>(fl->ಮೊದಲ_ಸ್ತಂಭ())));
        m_constructorp->ಕೂಡಿಸು_Stmtsp(
            ಅಂಶ_Call(fl, cpVarp,
                     withp ? VCMethod::COVERGROUP_WITH_ಮುಕ್ತಾಯ : VCMethod::COVERGROUP_SIZED_ಮುಕ್ತಾಯ,
                     ವಾದಗಳು)
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
    ಶೂನ್ಯ generateWithFilter(AstCoverBin* binp, AstCoverWith* withp, AstVar* cpVarp,
                            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* exprp, ಸ್ಥಿರಾಂಕ string& ಪೂರ್ವಪ್ರತ್ಯಯ) {
        File_ಸಾಲು* ಸ್ಥಿರಾಂಕ fl = withp->fileline();
        ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ wide = exprp->isWide();
        ಸ್ಥಿರಾಂಕ string grouping = !binp->isArray()     ? "Single"
                                : binp->arraySizep() ? "Fixed"
                                                     : "Values";
        m_constructorp->ಕೂಡಿಸು_Stmtsp(ಅಂಶ_Call(fl, cpVarp, VCMethod::COVERGROUP_WITH_BEGIN,
                                           {ctext(fl, "VlCovBinGrouping::" + grouping),
                                            cnum(fl, v3Global.opt.coverageMaxBins())})
                                      ->makeStmt());
        // The candidates count in the coverpoint's width, and the filter reads each as 'item',
        // of the coverpoint's type, so that a filter changing 'item' cannot change the loop
        Ast_ಗಂಟು_DType* ಸ್ಥಿರಾಂಕ ಮೌಲ್ಯ_DTypep = exprp->dtypep();
        AstVar* ಸ್ಥಿರಾಂಕ valuep = constructor_ತಾತ್ಕಾಲಿಕ(fl, ಪೂರ್ವಪ್ರತ್ಯಯ + "_value", ಮೌಲ್ಯ_DTypep);
        AstVar* ಸ್ಥಿರಾಂಕ lastp = constructor_ತಾತ್ಕಾಲಿಕ(fl, ಪೂರ್ವಪ್ರತ್ಯಯ + "_last", ಮೌಲ್ಯ_DTypep);
        AstVar* ಸ್ಥಿರಾಂಕ firstp = constructor_ತಾತ್ಕಾಲಿಕ(fl, ಪೂರ್ವಪ್ರತ್ಯಯ + "_first", ಮೌಲ್ಯ_DTypep);
        AstVar* ಸ್ಥಿರಾಂಕ runp = constructor_ತಾತ್ಕಾಲಿಕ(fl, ಪೂರ್ವಪ್ರತ್ಯಯ + "_run", binp->ಕಂಡುಹಿಡಿ_ಬಿಟ್_DType());
        AstVar* ಸ್ಥಿರಾಂಕ morep = constructor_ತಾತ್ಕಾಲಿಕ(fl, ಪೂರ್ವಪ್ರತ್ಯಯ + "_more", binp->ಕಂಡುಹಿಡಿ_ಬಿಟ್_DType());
        AstVar* ಸ್ಥಿರಾಂಕ itemp = withp->itemp()->unlink_Fr_ಹಿಂಭಾಗ();
        itemp->name(ಪೂರ್ವಪ್ರತ್ಯಯ + "_item");
        m_constructorp->ಕೂಡಿಸು_Stmtsp(itemp);
        ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ ref = [&](AstVar* varp) { ಹಿಂತಿರುಗಿ ಹೊಸ AstVarRef{fl, varp, VAccess::READ}; };
        ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ ನಿಯೋಜಿಸು_2 = [&](AstVar* varp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* rhsp) -> Ast_ಗಂಟು* {
            ಹಿಂತಿರುಗಿ ಹೊಸ Ast_ನಿಯೋಜಿಸು{fl, ಹೊಸ AstVarRef{fl, varp, VAccess::WRITE}, rhsp};
        };
        ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ flag = [&](AstVar* varp, ತಾರ್ಕಿಕ ಮೌಲ್ಯ) {
            ಹಿಂತಿರುಗಿ ನಿಯೋಜಿಸು_2(varp, ಮೌಲ್ಯ ? ಹೊಸ AstConst{fl, AstConst::ಬಿಟ್_True{}}
                                      : ಹೊಸ AstConst{fl, AstConst::ಬಿಟ್_False{}});
        };
        ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ bound = [&](VCMethod narrow, VCMethod wide_ವಿಧಾನ, AstVar* varp) -> Ast_ಗಂಟು* {
            ಆದರೆ (wide) {
                ಹಿಂತಿರುಗಿ ಅಂಶ_Call(fl, cpVarp, wide_ವಿಧಾನ, {ಹೊಸ AstVarRef{fl, varp, VAccess::WRITE}})
                    ->makeStmt();
            }
            AstCMethodHard* ಸ್ಥಿರಾಂಕ callp = ಅಂಶ_Call(fl, cpVarp, narrow);
            callp->dtype_ಗಣ_UInt64();
            ಹಿಂತಿರುಗಿ ನಿಯೋಜಿಸು_2(varp, resize_ಮೌಲ್ಯ(callp, ಮೌಲ್ಯ_DTypep));
        };
        ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ keep = [&](Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lop, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* hip) {
            AstCMethodHard* ಸ್ಥಿರಾಂಕ callp = ಅಂಶ_Call(
                fl, cpVarp, wide ? VCMethod::COVERGROUP_WITH_ಸತತ_W : VCMethod::COVERGROUP_WITH_ಸತತ,
                {lop, hip});
            callp->dtype_ಗಣ_ಬಿಟ್();
            ಹಿಂತಿರುಗಿ ನಿಯೋಜಿಸು_2(morep, callp);
        };
        ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ ಹೆಜ್ಜೆ = [&](ತಾರ್ಕಿಕ ಮೇಲೆ) {
            AstConst* ಸ್ಥಿರಾಂಕ onep = ಹೊಸ AstConst{fl, AstConst::Widthed_ಮೌಲ್ಯ{}, exprp->ಅಗಲ(), 1};
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ stepp
                = ಮೇಲೆ ? static_cast<Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ*>(ಹೊಸ Ast_ಕೂಡಿಸು{fl, ref(valuep), onep})
                     : ಹೊಸ AstSub{fl, ref(valuep), onep};
            stepp->dtypep(ಮೌಲ್ಯ_DTypep);
            ಹಿಂತಿರುಗಿ stepp;
        };
        AstLoop* ಸ್ಥಿರಾಂಕ innerp = ಹೊಸ AstLoop{fl};
        innerp->ಕೂಡಿಸು_Stmtsp(ನಿಯೋಜಿಸು_2(itemp, ref(valuep)));
        innerp->ಕೂಡಿಸು_Stmtsp(ಹೊಸ AstIf{
            fl, withp->filterp()->unlink_Fr_ಹಿಂಭಾಗ(),
            ಹೊಸ AstIf{fl, ಹೊಸ AstNot{fl, ref(runp)},
                      ನಿಯೋಜಿಸು_2(firstp, ref(valuep))->ಕೂಡಿಸು_ಮುಂದಿನ(flag(runp, ನಿಜ))},
            ಹೊಸ AstIf{fl, ref(runp), keep(ref(firstp), ಹೆಜ್ಜೆ(ಸುಳ್ಳು))->ಕೂಡಿಸು_ಮುಂದಿನ(flag(runp, ಸುಳ್ಳು))}});
        innerp->ಕೂಡಿಸು_Stmtsp(ಹೊಸ AstLoopTest{
            fl, innerp, ಹೊಸ AstLogAnd{fl, ref(morep), ಹೊಸ AstNeq{fl, ref(valuep), ref(lastp)}}});
        innerp->ಕೂಡಿಸು_Stmtsp(ನಿಯೋಜಿಸು_2(valuep, ಹೆಜ್ಜೆ(ನಿಜ)));
        AstLoop* ಸ್ಥಿರಾಂಕ outerp = ಹೊಸ AstLoop{fl};
        AstCMethodHard* ಸ್ಥಿರಾಂಕ nextp = ಅಂಶ_Call(fl, cpVarp, VCMethod::COVERGROUP_WITH_ಮುಂದಿನ);
        nextp->dtype_ಗಣ_ಬಿಟ್();
        outerp->ಕೂಡಿಸು_Stmtsp(ಹೊಸ AstLoopTest{fl, outerp, nextp});
        outerp->ಕೂಡಿಸು_Stmtsp(
            bound(VCMethod::COVERGROUP_WITH_LO, VCMethod::COVERGROUP_WITH_LO_W, valuep));
        outerp->ಕೂಡಿಸು_Stmtsp(
            bound(VCMethod::COVERGROUP_WITH_HI, VCMethod::COVERGROUP_WITH_HI_W, lastp));
        outerp->ಕೂಡಿಸು_Stmtsp(flag(runp, ಸುಳ್ಳು));
        outerp->ಕೂಡಿಸು_Stmtsp(innerp);
        outerp->ಕೂಡಿಸು_Stmtsp(ಹೊಸ AstIf{fl, ref(runp), keep(ref(firstp), ref(lastp))});
        m_constructorp->ಕೂಡಿಸು_Stmtsp(flag(morep, ನಿಜ));
        m_constructorp->ಕೂಡಿಸು_Stmtsp(outerp);
    }

    // Emit 'sizedRange(lo, hi)' for the coverpoint values of an element of bins 'binp', whose
    // values the constructor computes: resolved now if constant, else when constructed by
    // clipping to the coverpoint's values.  A wildcard pattern's values give a range for each
    // run of them.  'prefix' names its temporaries.
    ಶೂನ್ಯ generateSizedElement(AstVar* cpVarp, ಸ್ಥಿರಾಂಕ AstCoverBin* binp, Ast_ಗಂಟು* rangep,
                              Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* exprp, ಸ್ಥಿರಾಂಕ string& ಪೂರ್ವಪ್ರತ್ಯಯ) {
        File_ಸಾಲು* ಸ್ಥಿರಾಂಕ fl = rangep->fileline();
        ಸ್ಥಿರಾಂಕ VCMethod ವಿಧಾನ = exprp->isWide() ? VCMethod::COVERGROUP_SIZED_RANGE_W
                                                : VCMethod::COVERGROUP_SIZED_RANGE;
        ಸ್ಥಿರಾಂಕ AstInsideRange* ಸ್ಥಿರಾಂಕ irp = VN_CAST(rangep, InsideRange);
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ lowp = irp ? irp->lhsp() : VN_AS(rangep, ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ);
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ highp = irp ? irp->rhsp() : ಶೂನ್ಯಸೂಚಕ;
        ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ unbounded
            = [](ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* boundp) { ಹಿಂತಿರುಗಿ !boundp || VN_IS(boundp, Unbounded); };
        ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ constant
            = [&](ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* boundp) { ಹಿಂತಿರುಗಿ unbounded(boundp) || VN_IS(boundp, Const); };
        ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ integral = [&](ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* boundp) {
            ಹಿಂತಿರುಗಿ unbounded(boundp) || boundp->dtypep()->skipRefp()->isIntegralOrPacked();
        };
        ಆದರೆ (constant(lowp) && constant(highp)) {
            ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ fourState = [](ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* boundp) {
                ಸ್ಥಿರಾಂಕ AstConst* ಸ್ಥಿರಾಂಕ constp = VN_CAST(boundp, Const);
                ಹಿಂತಿರುಗಿ constp && constp->ಅಂಕೆ().isFourState();
            };
            ಆದರೆ (irp && (fourState(lowp) || fourState(highp))) {
                rangep->v3error("Four-state (x/z) value in "
                                << (binp->isArray() ? "array bins" : "bin")
                                << " range bound; range bounds must be two-state constants");
                ಹಿಂತಿರುಗಿ;
            }
            Cross_ಮೌಲ್ಯ_Range range{rangep, resolve_ಅಗಲ(rangep, exprp)};
            ಆದರೆ (!resolve_ಮೌಲ್ಯ(rangep, exprp, ನಿಜ, binp->isWildcard(), range)) {
                rangep->v3warn(E_UNSUPPORTED, "Unsupported: non-integral value in a coverage bin "
                                              "of an integral coverpoint.");
            } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (!cross_Range_ಖಾಲಿಗಳು(range)) {
                std::ಸದಿಶ<std::pair<V3_ಸಂಖ್ಯೆ, V3_ಸಂಖ್ಯೆ>> runs{{range.lo, range.hi}};
                ಆದರೆ (range.wildcard) {  // checkConstructedBins bounded the runs
                    runs.clear();
                    crossRangeRuns(range, v3Global.opt.coverageMaxBins(), runs);
                }
                ಗಾಗಿ (ಸ್ಥಿರಾಂಕ std::pair<V3_ಸಂಖ್ಯೆ, V3_ಸಂಖ್ಯೆ>& ಸತತ : runs) {
                    m_constructorp->ಕೂಡಿಸು_Stmtsp(ಅಂಶ_Call(fl, cpVarp, ವಿಧಾನ,
                                                       {new_ಮೌಲ್ಯ_Const(fl, ಸತತ.ಮೊದಲ, exprp),
                                                        new_ಮೌಲ್ಯ_Const(fl, ಸತತ.ಎರಡನೇ, exprp)})
                                                  ->makeStmt());
                }
            }
            ಹಿಂತಿರುಗಿ;
        }
        ಆದರೆ (!integral(lowp) || !integral(highp)) {
            rangep->v3warn(E_UNSUPPORTED, "Unsupported: non-integral value in a coverage bin "
                                          "of an integral coverpoint.");
            ಹಿಂತಿರುಗಿ;
        }
        // Compare values and the coverpoint's domain signed, in a width holding all of them
        ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ bound_ಅಗಲ
            = [&](ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* boundp) { ಹಿಂತಿರುಗಿ unbounded(boundp) ? 0 : boundp->ಅಗಲ(); };
        ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ ಅಗಲ = std::max({exprp->ಅಗಲ(), bound_ಅಗಲ(lowp), bound_ಅಗಲ(highp)}) + 1;
        ಸ್ಥಿರಾಂಕ Cross_ಮೌಲ್ಯ_Range domain
            = cross_ಮೌಲ್ಯ_Domain(rangep, exprp->ಅಗಲ(), exprp->isSigned(), ಅಗಲ);
        AstVar* ಸ್ಥಿರಾಂಕ lop = constructor_ತಾತ್ಕಾಲಿಕ(fl, ಪೂರ್ವಪ್ರತ್ಯಯ + "_lo", exprp->dtypep());
        lop->dtype_ಗಣ_ಬಿಟ್_Sized(ಅಗಲ, VSigning::SIGNED);
        AstVar* ಸ್ಥಿರಾಂಕ hip = constructor_ತಾತ್ಕಾಲಿಕ(fl, ಪೂರ್ವಪ್ರತ್ಯಯ + "_hi", lop->dtypep());
        ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ bound = [&](Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* boundp, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& ಎಲ್ಲೆ) -> Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* {
            ಆದರೆ (unbounded(boundp)) {
                AstConst* ಸ್ಥಿರಾಂಕ limitp = ಹೊಸ AstConst{fl, ಎಲ್ಲೆ};
                limitp->dtypeFrom(lop);
                ಹಿಂತಿರುಗಿ limitp;
            }
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* valuep = boundp->clone_ವೃಕ್ಷ(ಸುಳ್ಳು);
            // A value no wider than a signed coverpoint has its type (see crossRangeBound)
            ಆದರೆ (exprp->isSigned() && boundp->ಅಗಲ() <= exprp->ಅಗಲ()) {
                valuep = resize_ಮೌಲ್ಯ(valuep, exprp->dtypep());
            }
            ಹಿಂತಿರುಗಿ resize_ಮೌಲ್ಯ(valuep, lop->dtypep());
        };
        ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ ref = [&](AstVar* varp, VAccess access = VAccess::READ) {
            ಹಿಂತಿರುಗಿ ಹೊಸ AstVarRef{fl, varp, access};
        };
        m_constructorp->ಕೂಡಿಸು_Stmtsp(
            ಹೊಸ Ast_ನಿಯೋಜಿಸು{fl, ref(lop, VAccess::WRITE), bound(lowp, domain.lo)});
        m_constructorp->ಕೂಡಿಸು_Stmtsp(
            ಹೊಸ Ast_ನಿಯೋಜಿಸು{fl, ref(hip, VAccess::WRITE), irp ? bound(highp, domain.hi) : ref(lop)});
        AstConst* ಸ್ಥಿರಾಂಕ minp = ಹೊಸ AstConst{fl, domain.lo};
        AstConst* ಸ್ಥಿರಾಂಕ maxp = ಹೊಸ AstConst{fl, domain.hi};
        minp->dtypeFrom(lop);
        maxp->dtypeFrom(lop);
        AstCond* ಸ್ಥಿರಾಂಕ lowerp
            = ಹೊಸ AstCond{fl, ಹೊಸ AstLtS{fl, ref(lop), minp}, minp->clone_ವೃಕ್ಷ(ಸುಳ್ಳು), ref(lop)};
        AstCond* ಸ್ಥಿರಾಂಕ upperp
            = ಹೊಸ AstCond{fl, ಹೊಸ AstGtS{fl, ref(hip), maxp}, maxp->clone_ವೃಕ್ಷ(ಸುಳ್ಳು), ref(hip)};
        lowerp->dtypeFrom(lop);
        upperp->dtypeFrom(lop);
        m_constructorp->ಕೂಡಿಸು_Stmtsp(ಹೊಸ Ast_ನಿಯೋಜಿಸು{fl, ref(lop, VAccess::WRITE), lowerp});
        m_constructorp->ಕೂಡಿಸು_Stmtsp(ಹೊಸ Ast_ನಿಯೋಜಿಸು{fl, ref(hip, VAccess::WRITE), upperp});
        m_constructorp->ಕೂಡಿಸು_Stmtsp(ಹೊಸ AstIf{fl, ಹೊಸ AstLteS{fl, ref(lop), ref(hip)},
                                            ಅಂಶ_Call(fl, cpVarp, ವಿಧಾನ,
                                                     {resize_ಮೌಲ್ಯ(ref(lop), exprp->dtypep()),
                                                      resize_ಮೌಲ್ಯ(ref(hip), exprp->dtypep())})
                                                ->makeStmt()});
    }

    // The runtime index of the bin of a run holding the coverpoint value, which is in the run:
    // declared + (value - lo) / stride, capped at the last bin, which holds any remainder.
    ಸ್ಥಿರ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸತತ_Bin_ಸೂಚ್ಯಂಕ(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* exprp, ಸ್ಥಿರಾಂಕ Bin_ಸತತ& ಸತತ) {
        ಆದರೆ (ಸತತ.m_ಎಣಿಕೆ == 1) ಹಿಂತಿರುಗಿ cnum(fl, ಸತತ.m_declared);
        ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ ಅಗಲ = exprp->ಅಗಲ();
        // A run spans at most 2^width values, so offsets in it are unsigned width-bit numbers
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* indexp
            = ಹೊಸ AstSub{fl, exprp->clone_ವೃಕ್ಷ(ಸುಳ್ಳು), new_ಮೌಲ್ಯ_Const(fl, ಸತತ.m_lo, exprp)};
        indexp->dtype_ಗಣ_ಬಿಟ್_Sized(ಅಗಲ, VSigning::UNSIGNED);
        V3_ಸಂಖ್ಯೆ stride{fl, ಅಗಲ, 0};
        stride.op_ನಿಯೋಜಿಸು(ಸತತ.m_stride);
        ಆದರೆ (stride.ಎಣಿಕೆ_Ones() != 1) {
            indexp = ಹೊಸ AstDiv{fl, indexp, ಹೊಸ AstConst{fl, stride}};
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (!stride.isEqOne()) {
            indexp = ಹೊಸ Ast_ಸರಿಸು_R{fl, indexp, ಹೊಸ AstConst{fl, stride.most_ಗಣ_ಬಿಟ್_P1() - 1}};
        }
        // Compare the run's values with those of count bins of stride values, without overflow
        ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ ext_ಅಗಲ = ಸತತ.m_lo.ಅಗಲ() + 1;
        V3_ಸಂಖ್ಯೆ lo{fl, ext_ಅಗಲ, 0};
        lo.opExtendS(ಸತತ.m_lo, ಸತತ.m_lo.ಅಗಲ());
        V3_ಸಂಖ್ಯೆ ಹರವು{fl, ext_ಅಗಲ, 0};
        ಹರವು.opExtendS(ಸತತ.m_hi, ಸತತ.m_hi.ಅಗಲ());
        ಹರವು.opSub(V3_ಸಂಖ್ಯೆ{ಹರವು}, lo);
        V3_ಸಂಖ್ಯೆ covered{fl, ext_ಅಗಲ, 0};
        covered.op_ನಿಯೋಜಿಸು(ಸತತ.m_stride);
        covered.opMul(V3_ಸಂಖ್ಯೆ{covered}, V3_ಸಂಖ್ಯೆ{fl, ext_ಅಗಲ, ಸತತ.m_ಎಣಿಕೆ});
        V3_ಸಂಖ್ಯೆ hasRemainder{fl, 1, 0};
        ಆದರೆ (!hasRemainder.opGte(ಹರವು, covered).isEqZero()) {
            AstConst* ಸ್ಥಿರಾಂಕ lastp = ಹೊಸ AstConst{fl, V3_ಸಂಖ್ಯೆ{fl, ಅಗಲ, ಸತತ.m_ಎಣಿಕೆ - 1}};
            indexp = ಹೊಸ AstCond{fl, ಹೊಸ AstGt{fl, indexp->clone_ವೃಕ್ಷ(ಸುಳ್ಳು), lastp},
                                 lastp->clone_ವೃಕ್ಷ(ಸುಳ್ಳು), indexp};
        }
        ಆದರೆ (ಅಗಲ < VL_IDATASIZE) {
            indexp = ಹೊಸ AstExtend{fl, indexp, VL_IDATASIZE};
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (ಅಗಲ > VL_IDATASIZE) {
            indexp = ಹೊಸ AstSel{fl, indexp, 0, VL_IDATASIZE};
        }
        ಹಿಂತಿರುಗಿ ಹೊಸ Ast_ಕೂಡಿಸು{fl, cnum(fl, ಸತತ.m_declared), indexp};
    }

    // Emit the sample() hit of a run of bins, whose code does not grow with its number of bins:
    //   if (iff && lo <= value && value <= hi) m_cp.incrementBin(<runBinIndex>);
    ಶೂನ್ಯ emit_ಸತತ_ದೊರಕಿತು(AstCoverpoint* coverpointp, AstCoverBin* binp, AstVar* cpVarp,
                    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* exprp, ಸ್ಥಿರಾಂಕ Bin_ಸತತ& ಸತತ) {
        File_ಸಾಲು* ಸ್ಥಿರಾಂಕ fl = binp->fileline();
        AstConst* ಸ್ಥಿರಾಂಕ lop = new_ಮೌಲ್ಯ_Const(fl, ಸತತ.m_lo, exprp);
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* condp = ಶೂನ್ಯಸೂಚಕ;
        ಆದರೆ (ಸತತ.m_lo.isCaseEq(ಸತತ.m_hi)) {
            condp = ಹೊಸ AstEq{fl, exprp->clone_ವೃಕ್ಷ(ಸುಳ್ಳು), lop};
        } ಇಲ್ಲದಿದ್ದರೆ {
            AstConst* ಸ್ಥಿರಾಂಕ hip = new_ಮೌಲ್ಯ_Const(fl, ಸತತ.m_hi, exprp);
            condp = makeRangeCondition(fl, exprp, lop, hip);
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(lop), lop);
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(hip), hip);
        }
        emit_Conv_ದೊರಕಿತು_If(coverpointp, binp, cpVarp, ಸತತ_Bin_ಸೂಚ್ಯಂಕ(fl, exprp, ಸತತ), condp);
    }

    // Emit a transition bin's hit action into sample():
    //   if (cond) { m_cp.incrementBin/recordHit(idx); [illegal: $error; $stop] }
    // Used by the transition generators so a completed sequence records into the runtime bin.
    ಶೂನ್ಯ ಕೂಡಿಸು_Conv_Trans_ದೊರಕಿತು_If(AstCoverpoint* coverpointp, AstCoverBin* binp, ಸ್ಥಿರಾಂಕ Conv_Bin_ಗುರಿ& tgt,
                           Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* condp) {
        File_ಸಾಲು* ಸ್ಥಿರಾಂಕ fl = binp->fileline();
        Ast_ಗಂಟು* actionp = make_Runtime_Bin_ದೊರಕಿತು(fl, tgt);
        ಆದರೆ (binp->binsType() == VCoverBinsType::BINS_ILLEGAL) {
            actionp->ಕೂಡಿಸು_ಮುಂದಿನ(makeIllegalBinAction(
                fl, "Illegal transition bin " + binp->prettyNameQ() + " hit in coverpoint "
                        + coverpointp->prettyNameQ()));
        }
        UASSERT_OBJ(m_sampleFuncp, binp, "sample() CFunc not set for transition bin");
        ಕೂಡಿಸು_Sample_Stmt(ಹೊಸ AstIf{fl, condp, actionp, ಶೂನ್ಯಸೂಚಕ});
    }

    // Route a coverpoint through a VlCoverpoint member: emit the member, its sample()
    // increments, the constructor configuration (init + namers), and registration.
    ಶೂನ್ಯ generateCoverpoint(AstCoverpoint* coverpointp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* exprp, ಪೂರ್ಣಾಂಕ at_ಕನಿಷ್ಠ_ಮೌಲ್ಯ) {
        File_ಸಾಲು* ಸ್ಥಿರಾಂಕ fl = coverpointp->fileline();
        ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ dynamic = m_runtimePoints.ಎಣಿಕೆ_2(coverpointp);
        UINFO(4, "  Generating VlCoverpoint member: " << coverpointp->name());

        // Size the hit list to the gen-time max bin overlap (1 unless cross-fed with
        // overlapping ranges), so no cross hit is ever dropped and storage is minimal.
        ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ crossFed = m_crossed_Cp_ಹೆಸರುಗಳು.ಎಣಿಕೆ_2(coverpointp->name()) != 0;
        ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ ದೊರಕಿತು_Bound = compute_ದೊರಕಿತು_List_Bound(coverpointp, exprp, crossFed);
        UINFO(6, "    Hit-list bound (max bin overlap) = " << ದೊರಕಿತು_Bound);
        AstVar* ಸ್ಥಿರಾಂಕ cpVarp = ಹೊಸ AstVar{fl, VVarType::MEMBER, "__Vcp_" + coverpointp->name(),
                                          coverpointDType(fl, static_cast<uint32_t>(ದೊರಕಿತು_Bound))};
        m_covergroupp->ಕೂಡಿಸು_Membersp(cpVarp);
        m_cpVarMap[coverpointp->name()] = cpVarp;
        m_cpBins.emplace(cpVarp, CoverpointBins{});
        m_cpBins.at(cpVarp).exprp = exprp;
        m_cpBins.at(cpVarp).crossed = crossFed;
        // Create the runtime in the instance node first; everything below configures it.
        m_constructorp->ಕೂಡಿಸು_Stmtsp(make_ಅಂಶ_Create(fl, cpVarp, VCMethod::COVERGROUP_ಕೂಡಿಸು_COVERPOINT));
        generate_ಅಂಶ_ತೂಕ(fl, cpVarp, coverpointp->optionsp());

        // A cross reads this coverpoint's hit list, so clear it at the start of the
        // coverpoint's sample() contribution (before any incrementBin appends to it), even
        // when the iff guard disables sampling.
        ಆದರೆ (crossFed) {
            UASSERT_OBJ(m_sampleFuncp, coverpointp, "sample() CFunc not set for clearHitList");
            m_sampleFuncp->ಕೂಡಿಸು_Stmtsp(
                ಅಂಶ_Call(fl, cpVarp, VCMethod::COVERGROUP_CLEAR_ದೊರಕಿತು_LIST)->makeStmt());
        }
        ಆದರೆ (dynamic && coverpointHasStateExclusions(coverpointp)) {
            AstVar* ಸ್ಥಿರಾಂಕ excludedp
                = ಹೊಸ AstVar{fl, VVarType::BLOCKTEMP,
                             "__VcpExcluded_" + sanitizeGeneratedName(coverpointp->name()),
                             coverpointp->ಕಂಡುಹಿಡಿ_ಬಿಟ್_DType()};
            excludedp->funcLocal(ನಿಜ);
            m_sampleFuncp->ಕೂಡಿಸು_Stmtsp(excludedp);
            AstCMethodHard* ಸ್ಥಿರಾಂಕ callp
                = ಅಂಶ_Call(fl, cpVarp,
                           exprp->isWide() ? VCMethod::COVERGROUP_ಮೌಲ್ಯ_EXCLUDED_W
                                           : VCMethod::COVERGROUP_ಮೌಲ್ಯ_EXCLUDED,
                           {exprp->clone_ವೃಕ್ಷ(ಸುಳ್ಳು)});
            callp->dtype_ಗಣ_ಬಿಟ್();
            ಕೂಡಿಸು_Sample_Stmt(ಹೊಸ Ast_ನಿಯೋಜಿಸು{fl, ಹೊಸ AstVarRef{fl, excludedp, VAccess::WRITE}, callp});
            m_excludedVars.emplace(cpVarp, excludedp);
        }

        // Walk bins (non-default, then default), assigning sequential indices that match the
        // namer append order; emit sample increments and collect namer statements.  Constructed
        // bins follow them all, placed when the coverpoint is constructed.
        std::ಸದಿಶ<Ast_ಗಂಟು_Stmt*> namerStmts;
        std::ಸದಿಶ<AstCoverBin*> defaultBins;
        std::ಸದಿಶ<AstCoverBin*> sizedBins;
        std::ಸದಿಶ<std::tuple<AstCoverBin*, uint32_t, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ*>> metadata;
        std::ಸದಿಶ<ಸ್ಥಿರಾಂಕ Bin_ಸತತ*> ಸತತ_Metadata;
        uint64_t idx = 0;  // Runtime index of the next bin; 32-bit once checked below
        ಗಾಗಿ (Ast_ಗಂಟು* binp = coverpointp->binsp(); binp; binp = binp->nextp()) {
            AstCoverBin* ಸ್ಥಿರಾಂಕ cbinp = VN_AS(binp, CoverBin);
            ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ errorsBefore = dynamic ? V3Error::error_ಎಣಿಕೆ() : 0;
            ಆದರೆ (cbinp->binsType() == VCoverBinsType::BINS_DEFAULT) {
                defaultBins.ತಳ್ಳು_ಹಿಂಭಾಗ(cbinp);
                ಮುಂದುವರಿ;
            }
            ಆದರೆ (isConstructedBins(cbinp)) {
                UASSERT_OBJ(dynamic, cbinp, "Constructed bins without value metadata");
                Bin_ಹರವು ಹರವು;
                ಹರವು.sized = static_cast<int32_t>(sizedBins.ಗಾತ್ರ_2());
                m_cpBins.at(cpVarp).spans.emplace(cbinp->name(), ಹರವು);
                sizedBins.ತಳ್ಳು_ಹಿಂಭಾಗ(cbinp);
                ಮುಂದುವರಿ;
            }
            ಆದರೆ (cbinp->transp()) {
                // Transition bin (incl. array transition 'bins t[] = (a=>b),(c=>d)' and
                // illegal_bins/ignore_bins transitions).  All sequences of one transition bin
                // share a bin name and merge in the coverage DB to a single point, so model
                // them as one runtime bin incremented by any matching sequence.  The sequence
                // matching is generated as a state machine, with the hit routed to this bin's
                // runtime slot.
                namerStmts.ತಳ್ಳು_ಹಿಂಭಾಗ(makeNamer(cpVarp, cbinp, -1, static_cast<uint32_t>(idx)));
                ಸ್ಥಿರಾಂಕ Conv_Bin_ಗುರಿ tgt{cpVarp, static_cast<uint32_t>(idx),
                                        cbinp->binsType().bin_Is_ಸಾಮಾನ್ಯ()};
                ಗಾಗಿ (Ast_ಗಂಟು* sp = cbinp->transp(); sp; sp = sp->nextp())
                    generate_Single_Transition_ಸಂಕೇತ(coverpointp, cbinp, exprp, tgt,
                                                 VN_AS(sp, Cover_Trans_ಗಣ));
                ಆದರೆ (dynamic && V3Error::error_ಎಣಿಕೆ() == errorsBefore) {
                    metadata.emplace_ಹಿಂಭಾಗ(cbinp, static_cast<uint32_t>(idx), ಶೂನ್ಯಸೂಚಕ);
                }
                ++idx;
                ಮುಂದುವರಿ;
            }
            BinRuns plan;
            ಆದರೆ (binRunsFor(cbinp, exprp, plan)) {
                // Array elements and automatic bins generate as runs, so neither sample() nor
                // the constructor grows with their number of bins.
                ಆದರೆ (plan.unsupported) {  // bin ignored or invalid; reserve no slot
                    m_droppedBins[coverpointp].ತಳ್ಳು_ಹಿಂಭಾಗ(cbinp->name());
                    ಮುಂದುವರಿ;
                }
                CoverpointBins& bins = m_cpBins.at(cpVarp);
                ಸ್ಥಿರಾಂಕ uint32_t ಮೊದಲ_ಮೌಲ್ಯ = bins.ಒಟ್ಟು;
                ಸ್ಥಿರಾಂಕ uint32_t ಮೊದಲ_Declared = static_cast<uint32_t>(idx);
                namerStmts.ತಳ್ಳು_ಹಿಂಭಾಗ(
                    makeNamer(cpVarp, cbinp, plan.ಎಣಿಕೆ_2, ಮೊದಲ_Declared, {}, plan.ಮೌಲ್ಯಗಳು));
                ಗಾಗಿ (Bin_ಸತತ& ಸತತ : plan.runs) {
                    ಸತತ.m_declared = static_cast<uint32_t>(idx);
                    bins.runs.ತಳ್ಳು_ಹಿಂಭಾಗ(std::move(ಸತತ));
                    ಸ್ಥಿರಾಂಕ Bin_ಸತತ& stored = bins.runs.ಹಿಂಭಾಗ();
                    ಆದರೆ (bins.crossed && cbinp->binsType().bin_Is_ಸಾಮಾನ್ಯ()) {
                        ಸ್ಥಿರಾಂಕ uint32_t ಮೊದಲ = ಮೊದಲ_ಮೌಲ್ಯ + stored.m_declared - ಮೊದಲ_Declared;
                        ಗಾಗಿ (uint32_t element = 0; element < stored.m_ಎಣಿಕೆ; ++element) {
                            bins.ಮೌಲ್ಯಗಳು[ಮೊದಲ + element].runp = &stored;
                            bins.ಮೌಲ್ಯಗಳು[ಮೊದಲ + element].element = element;
                        }
                    }
                    ಆದರೆ (!stored.m_ಖಾಲಿಗಳು) {
                        emit_ಸತತ_ದೊರಕಿತು(coverpointp, cbinp, cpVarp, exprp, stored);
                        ಆದರೆ (dynamic && V3Error::error_ಎಣಿಕೆ() == errorsBefore) {
                            ಸತತ_Metadata.ತಳ್ಳು_ಹಿಂಭಾಗ(&stored);
                        }
                    }
                    idx += stored.m_ಎಣಿಕೆ;
                }
                ಮುಂದುವರಿ;
            }
            ಆದರೆ (cbinp->isArray()) {  // value array of a real coverpoint: b[0]..b[N-1]
                // Only integral coverpoints have runtime value metadata (m_runtimePoints)
                UASSERT_OBJ(!dynamic, cbinp, "Runtime value metadata for a real coverpoint");
                ತಾರ್ಕಿಕ unsupported = ಸುಳ್ಳು;
                std::ಸದಿಶ<Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ*> ಮೌಲ್ಯಗಳು = ಹೊರತೆಗೆ_Array_ಮೌಲ್ಯಗಳು(cbinp, exprp, unsupported);
                ಆದರೆ (unsupported) {  // bin ignored (COVERIGN emitted); reserve no slot
                    m_droppedBins[coverpointp].ತಳ್ಳು_ಹಿಂಭಾಗ(cbinp->name());
                    ಮುಂದುವರಿ;
                }
                namerStmts.ತಳ್ಳು_ಹಿಂಭಾಗ(makeNamer(cpVarp, cbinp, static_cast<int64_t>(ಮೌಲ್ಯಗಳು.ಗಾತ್ರ_2()),
                                               static_cast<uint32_t>(idx), ಮೌಲ್ಯಗಳು));
                ಗಾಗಿ (Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* valuep : ಮೌಲ್ಯಗಳು) {
                    // The cross selections of this covergroup still read the value.
                    m_detached_ಮೌಲ್ಯಗಳು.ತಳ್ಳು_ಹಿಂಭಾಗ(valuep);
                    emit_Conv_ದೊರಕಿತು_If(coverpointp, cbinp, cpVarp,
                                  cnum(cbinp->fileline(), static_cast<uint32_t>(idx)),
                                  build_ಮೌಲ್ಯ_Condition(cbinp, exprp, valuep));
                    ++idx;
                }
            } ಇಲ್ಲದಿದ್ದರೆ {
                namerStmts.ತಳ್ಳು_ಹಿಂಭಾಗ(makeNamer(cpVarp, cbinp, -1, static_cast<uint32_t>(idx)));
                // buildBinCondition is null for 'ignore_bins = default' (no ranges); the bin
                // still gets a reserved slot (recorded, never incremented).
                ಆದರೆ (Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ condp = buildBinCondition(cbinp, exprp))
                    emit_Conv_ದೊರಕಿತು_If(coverpointp, cbinp, cpVarp,
                                  cnum(cbinp->fileline(), static_cast<uint32_t>(idx)), condp);
                ಆದರೆ (dynamic && V3Error::error_ಎಣಿಕೆ() == errorsBefore) {
                    metadata.emplace_ಹಿಂಭಾಗ(cbinp, static_cast<uint32_t>(idx), ಶೂನ್ಯಸೂಚಕ);
                }
                ++idx;
            }
        }
        // A cross selecting an ignored bins declaration selects no bins
        ಗಾಗಿ (ಸ್ಥಿರಾಂಕ std::string& name : m_droppedBins[coverpointp]) {
            m_cpBins.at(cpVarp).spans.emplace(name, Bin_ಹರವು{});
        }
        // A value a constructed bin holds is no default bin's; only sampling tells which do
        AstVar* sizedMatchedp = ಶೂನ್ಯಸೂಚಕ;
        ಆದರೆ (!defaultBins.ಖಾಲಿಗಳು()
            && std::any_ದ(sizedBins.begin(), sizedBins.ಕೊನೆ(), [](ಸ್ಥಿರಾಂಕ AstCoverBin* binp) {
                   ಹಿಂತಿರುಗಿ binp->binsType().bin_Is_ಸಾಮಾನ್ಯ();
               })) {
            sizedMatchedp = ಹೊಸ AstVar{fl, VVarType::BLOCKTEMP,
                                       "__VcpSized_" + sanitizeGeneratedName(coverpointp->name()),
                                       coverpointp->ಕಂಡುಹಿಡಿ_ಬಿಟ್_DType()};
            sizedMatchedp->funcLocal(ನಿಜ);
            m_sampleFuncp->ಕೂಡಿಸು_Stmtsp(sizedMatchedp);
            ಕೂಡಿಸು_Sample_Stmt(ಹೊಸ Ast_ನಿಯೋಜಿಸು{fl, ಹೊಸ AstVarRef{fl, sizedMatchedp, VAccess::WRITE},
                                        ಹೊಸ AstConst{fl, AstConst::ಬಿಟ್_False{}}});
        }
        ಗಾಗಿ (uint32_t sized = 0; sized < sizedBins.ಗಾತ್ರ_2(); ++sized) {
            emitSizedSample(coverpointp, sizedBins[sized], cpVarp, exprp, sized, sizedMatchedp);
        }
        ಗಾಗಿ (AstCoverBin* ಸ್ಥಿರಾಂಕ defBinp : defaultBins) {
            File_ಸಾಲು* ಸ್ಥಿರಾಂಕ dfl = defBinp->fileline();
            namerStmts.ತಳ್ಳು_ಹಿಂಭಾಗ(makeNamer(cpVarp, defBinp, -1, static_cast<uint32_t>(idx)));
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* condp = buildDefaultCondition(coverpointp, exprp, dfl);
            ಆದರೆ (sizedMatchedp) {
                condp = ಹೊಸ AstLogAnd{
                    dfl, ಹೊಸ AstNot{dfl, ಹೊಸ AstVarRef{dfl, sizedMatchedp, VAccess::READ}}, condp};
            }
            emit_Conv_ದೊರಕಿತು_If(coverpointp, defBinp, cpVarp, cnum(dfl, static_cast<uint32_t>(idx)),
                          condp);
            ++idx;
        }
        ಆದರೆ (idx > std::numeric_limits<uint32_t>::max()) {
            // The runtime indexes bins with 32 bits; stop before generating a model
            coverpointp->v3warn(E_UNSUPPORTED, "Unsupported: coverpoint with more than "
                                                   << std::numeric_limits<uint32_t>::max()
                                                   << " bins");
        }

        // Transition coverpoints track the previous sampled value; update it once at the end of
        // this coverpoint's sample() contribution (the prev var was created on demand by the
        // transition matching above).
        ಆದರೆ (coverpointHasTransition(coverpointp)) {
            AstVar* ಸ್ಥಿರಾಂಕ prevVarp = VN_AS(coverpointp->user1p(), Var);
            ಕೂಡಿಸು_Sample_Stmt(
                ಹೊಸ Ast_ನಿಯೋಜಿಸು{coverpointp->fileline(),
                              ಹೊಸ AstVarRef{prevVarp->fileline(), prevVarp, VAccess::WRITE},
                              exprp->clone_ವೃಕ್ಷ(ಸುಳ್ಳು)});
        }

        // Constructor: init (allocates), namers, then registration (under --coverage).
        // Under --protect-ids the hierarchy and page string reach the coverage database
        // verbatim, so obfuscate them like line/toggle points (per-word hierarchy, whole-
        // unit page).  No-ops when --protect-ids is off.
        ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ prot = v3Global.opt.protectIds();
        ಸ್ಥಿರಾಂಕ std::string hier
            = VIdProtect::protect_ಪದಗಳು_If(m_covergroupName + "." + coverpointp->name(), prot);
        m_constructorp->ಕೂಡಿಸು_Stmtsp(
            ಅಂಶ_Call(fl, cpVarp, VCMethod::COVERGROUP_INIT,
                     {ctext(fl, quoted(hier)), cnum(fl, static_cast<uint32_t>(at_ಕನಿಷ್ಠ_ಮೌಲ್ಯ)),
                      cnum(fl, static_cast<uint32_t>(idx))})
                ->makeStmt());
        ಗಾಗಿ (Ast_ಗಂಟು_Stmt* ಸ್ಥಿರಾಂಕ ns : namerStmts) m_constructorp->ಕೂಡಿಸು_Stmtsp(ns);
        ಆದರೆ (dynamic) {
            m_constructorp->ಕೂಡಿಸು_Stmtsp(
                ಅಂಶ_Call(fl, cpVarp, VCMethod::COVERGROUP_ಮೌಲ್ಯ_TYPE,
                         {cnum(fl, exprp->ಅಗಲ()), cnum(fl, exprp->isSigned())})
                    ->makeStmt());
            ಮೌಲ್ಯ_Lists lists;
            ಗಾಗಿ (ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ& ನಮೂದು : metadata) {
                collect_ಮೌಲ್ಯ_Metadata(lists, exprp, std::get<0>(ನಮೂದು), std::get<1>(ನಮೂದು),
                                     std::get<2>(ನಮೂದು));
            }
            ಗಾಗಿ (ಸ್ಥಿರಾಂಕ Bin_ಸತತ* ಸ್ಥಿರಾಂಕ runp : ಸತತ_Metadata) collect_ಸತತ_Metadata(lists, exprp, *runp);
            emit_ಮೌಲ್ಯ_List(fl, cpVarp, VCMethod::COVERGROUP_ಮೌಲ್ಯ_RANGES, lists.m_ranges);
            emit_ಮೌಲ್ಯ_List(fl, cpVarp, VCMethod::COVERGROUP_ಮೌಲ್ಯ_RUNS, lists.m_runs);
            emit_ಮೌಲ್ಯ_List(fl, cpVarp, VCMethod::COVERGROUP_ಮೌಲ್ಯ_PATTERNS, lists.m_patterns);
            emit_ಮೌಲ್ಯ_List(fl, cpVarp, VCMethod::COVERGROUP_ಮೌಲ್ಯ_TRANSITIONS, lists.m_transitions);
            ಗಾಗಿ (AstCoverBin* ಸ್ಥಿರಾಂಕ binp : sizedBins) {
                generateConstructedBins(coverpointp, binp, cpVarp, exprp);
            }
            m_constructorp->ಕೂಡಿಸು_Stmtsp(
                ಅಂಶ_Call(fl, cpVarp, VCMethod::COVERGROUP_ಮೌಲ್ಯ_FINALIZE)->makeStmt());
        }
        ಆದರೆ (v3Global.opt.coverage()) {
            ಸ್ಥಿರಾಂಕ std::string ಪುಟ
                = VIdProtect::protectIf("v_covergroup/" + m_covergroupName, prot);
            m_constructorp->ಕೂಡಿಸು_Stmtsp(
                ಅಂಶ_Call(fl, cpVarp, VCMethod::COVERGROUP_ರಿಜಿಸ್ಟರ್_BINS,
                         {ctext(fl, "vlSymsp->_vm_contextp__->coveragep()"),
                          ctext(fl, quoted(ಪುಟ)),
                          cnum(fl, ಅಂಶ_Database_ತೂಕ(coverpointp->optionsp())),
                          cnum(fl, m_cg_Type_ತೂಕ)})
                    ->makeStmt());
        }
    }

    // Generate state machine code for multi-value transition sequences
    // Handles transitions like (1 => 2 => 3 => 4)
    ಶೂನ್ಯ generate_Multi_ಮೌಲ್ಯ_Transition_ಸಂಕೇತ(AstCoverpoint* coverpointp, AstCoverBin* binp,
                                          Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* exprp, ಸ್ಥಿರಾಂಕ Conv_Bin_ಗುರಿ& tgt,
                                          ಸ್ಥಿರಾಂಕ std::ಸದಿಶ<Ast_Cover_Trans_ಅಂಶ*>& ವಸ್ತುಗಳು) {
        UINFO(4, "    Generating multi-value transition state machine for: " << binp->name());
        UINFO(4, "      Sequence length: " << ವಸ್ತುಗಳು.ಗಾತ್ರ_2() << " items");

        // Create state position variable
        AstVar* ಸ್ಥಿರಾಂಕ stateVarp = createSequenceStateVar(coverpointp, binp);

        // Build case statement with N cases (one for each state 0 to N-1)
        // State 0: Not started, looking for first item
        // State 1 to N-1: In progress, looking for next item

        AstCase* ಸ್ಥಿರಾಂಕ casep
            = ಹೊಸ AstCase{binp->fileline(), VCaseType::CT_CASE,
                          ಹೊಸ AstVarRef{stateVarp->fileline(), stateVarp, VAccess::READ}, ಶೂನ್ಯಸೂಚಕ};

        // Generate each case item in the switch statement
        ಗಾಗಿ (ಗಾತ್ರ_t state = 0; state < ವಸ್ತುಗಳು.ಗಾತ್ರ_2(); ++state) {
            Ast_Case_ಅಂಶ* caseItemp = generateTransitionStateCase(coverpointp, binp, exprp, tgt,
                                                                 stateVarp, ವಸ್ತುಗಳು, state);
            casep->ಕೂಡಿಸು_Itemsp(caseItemp);
        }

        // Add default case (reset to state 0) to prevent CASEINCOMPLETE warnings,
        // since the state variable is wider than the number of valid states.
        Ast_Case_ಅಂಶ* ಸ್ಥಿರಾಂಕ defaultItemp = ಹೊಸ Ast_Case_ಅಂಶ{
            binp->fileline(), ಶೂನ್ಯಸೂಚಕ,
            ಹೊಸ Ast_ನಿಯೋಜಿಸು{binp->fileline(),
                          ಹೊಸ AstVarRef{binp->fileline(), stateVarp, VAccess::WRITE},
                          ಹೊಸ AstConst{binp->fileline(), AstConst::Widthed_ಮೌಲ್ಯ{}, 8, 0}}};
        casep->ಕೂಡಿಸು_Itemsp(defaultItemp);

        ಕೂಡಿಸು_Sample_Stmt(casep);
        UINFO(4, "      Successfully added multi-value transition state machine");
    }

    // Generate code for a single state in the transition state machine
    // Returns the case item for this state
    Ast_Case_ಅಂಶ* generateTransitionStateCase(AstCoverpoint* coverpointp, AstCoverBin* binp,
                                             Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* exprp, ಸ್ಥಿರಾಂಕ Conv_Bin_ಗುರಿ& tgt,
                                             AstVar* stateVarp,
                                             ಸ್ಥಿರಾಂಕ std::ಸದಿಶ<Ast_Cover_Trans_ಅಂಶ*>& ವಸ್ತುಗಳು,
                                             ಗಾತ್ರ_t state) {
        File_ಸಾಲು* ಸ್ಥಿರಾಂಕ fl = binp->fileline();

        // Build condition for current value matching expected item at this state
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ ಹೊಂದಿಕೆ_Condp = build_Transition_ಅಂಶ_Condition(ವಸ್ತುಗಳು[state], exprp);

        Ast_ಗಂಟು_Stmt* ಹೊಂದಿಕೆ_Actionp = ಶೂನ್ಯಸೂಚಕ;

        ಆದರೆ (state == ವಸ್ತುಗಳು.ಗಾತ್ರ_2() - 1) {
            // Last state: sequence complete!  Record the hit in the runtime VlCoverpoint.
            ಹೊಂದಿಕೆ_Actionp = make_Runtime_Bin_ದೊರಕಿತು(fl, tgt);

            // For illegal_bins, add error message
            ಆದರೆ (binp->binsType() == VCoverBinsType::BINS_ILLEGAL) {
                ಸ್ಥಿರಾಂಕ string err_ಸಂದೇಶ = "Illegal transition bin " + binp->prettyNameQ()
                                      + " hit in coverpoint " + coverpointp->prettyNameQ();
                ಹೊಂದಿಕೆ_Actionp = ಹೊಂದಿಕೆ_Actionp->ಕೂಡಿಸು_ಮುಂದಿನ(makeIllegalBinAction(fl, err_ಸಂದೇಶ));
            }

            // Reset state to 0
            ಹೊಂದಿಕೆ_Actionp = ಹೊಂದಿಕೆ_Actionp->ಕೂಡಿಸು_ಮುಂದಿನ(
                ಹೊಸ Ast_ನಿಯೋಜಿಸು{fl, ಹೊಸ AstVarRef{fl, stateVarp, VAccess::WRITE},
                              ಹೊಸ AstConst{fl, AstConst::Widthed_ಮೌಲ್ಯ{}, 8, 0}});
        } ಇಲ್ಲದಿದ್ದರೆ {
            // Intermediate state: advance to next state
            ಹೊಂದಿಕೆ_Actionp = ಹೊಸ Ast_ನಿಯೋಜಿಸು{
                fl, ಹೊಸ AstVarRef{fl, stateVarp, VAccess::WRITE},
                ಹೊಸ AstConst{fl, AstConst::Widthed_ಮೌಲ್ಯ{}, 8, static_cast<uint32_t>(state + 1)}};
        }

        // Build restart logic: check if current value matches first item
        // If so, restart sequence from state 1 (even if we're in middle of sequence)
        Ast_ಗಂಟು_Stmt* no_ಹೊಂದಿಕೆ_Actionp = ಶೂನ್ಯಸೂಚಕ;
        ಆದರೆ (state > 0) {
            // Check if current value matches first item (restart condition)
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ restartCondp = build_Transition_ಅಂಶ_Condition(ವಸ್ತುಗಳು[0], exprp);

            UASSERT_OBJ(restartCondp, ವಸ್ತುಗಳು[0],
                        "buildTransitionItemCondition returned nullptr for restart");

            // Restart to state 1
            Ast_ಗಂಟು_Stmt* restartActionp
                = ಹೊಸ Ast_ನಿಯೋಜಿಸು{fl, ಹೊಸ AstVarRef{fl, stateVarp, VAccess::WRITE},
                                ಹೊಸ AstConst{fl, AstConst::Widthed_ಮೌಲ್ಯ{}, 8, 1}};

            // Reset to state 0 (else branch)
            Ast_ಗಂಟು_Stmt* resetActionp
                = ಹೊಸ Ast_ನಿಯೋಜಿಸು{fl, ಹೊಸ AstVarRef{fl, stateVarp, VAccess::WRITE},
                                ಹೊಸ AstConst{fl, AstConst::Widthed_ಮೌಲ್ಯ{}, 8, 0}};

            no_ಹೊಂದಿಕೆ_Actionp = ಹೊಸ AstIf{fl, restartCondp, restartActionp, resetActionp};
        }
        // For state 0, no action needed if no match (stay in state 0)

        // Combine into if-else
        Ast_ಗಂಟು_Stmt* ಸ್ಥಿರಾಂಕ stmtp = ಹೊಸ AstIf{fl, ಹೊಂದಿಕೆ_Condp, ಹೊಂದಿಕೆ_Actionp, no_ಹೊಂದಿಕೆ_Actionp};

        // Create case item for this state value
        Ast_Case_ಅಂಶ* ಸ್ಥಿರಾಂಕ caseItemp = ಹೊಸ Ast_Case_ಅಂಶ{
            fl, ಹೊಸ AstConst{fl, AstConst::Widthed_ಮೌಲ್ಯ{}, 8, static_cast<uint32_t>(state)},
            stmtp};

        ಹಿಂತಿರುಗಿ caseItemp;
    }

    // Create: $error(msg); $stop;  Used when an illegal bin is hit.
    Ast_ಗಂಟು_Stmt* makeIllegalBinAction(File_ಸಾಲು* fl, ಸ್ಥಿರಾಂಕ string& err_ಸಂದೇಶ) {
        AstDisplay* ಸ್ಥಿರಾಂಕ errorp
            = ಹೊಸ AstDisplay{fl, VDisplayType::DT_ERROR, err_ಸಂದೇಶ, ಶೂನ್ಯಸೂಚಕ, ಶೂನ್ಯಸೂಚಕ};
        errorp->fmtp()->timeunit(m_covergroupp->timeunit());
        static_cast<Ast_ಗಂಟು*>(errorp)->ಕೂಡಿಸು_ಮುಂದಿನ(ಹೊಸ AstStop{fl, ನಿಜ});
        ಹಿಂತಿರುಗಿ errorp;
    }

    // Preserve the coverpoint's width and signedness after V3Width.
    ಸ್ಥಿರ AstConst* new_ಮೌಲ್ಯ_Const(File_ಸಾಲು* fl, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& ಮೌಲ್ಯ, ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* exprp) {
        V3_ಸಂಖ್ಯೆ narrowed{fl, exprp->ಅಗಲ(), 0};
        narrowed.op_ನಿಯೋಜಿಸು(ಮೌಲ್ಯ);
        AstConst* ಸ್ಥಿರಾಂಕ constp = ಹೊಸ AstConst{fl, narrowed};
        constp->dtypeFrom(exprp);
        ಹಿಂತಿರುಗಿ constp;
    }

    // A real copy of a real or integral range bound, for comparing with a real coverpoint.
    ಸ್ಥಿರ AstConst* newRealConst(AstConst* constp) {
        ಆದರೆ (constp->ಅಂಕೆ().isDouble()) ಹಿಂತಿರುಗಿ constp->clone_ವೃಕ್ಷ(ಸುಳ್ಳು);
        V3_ಸಂಖ್ಯೆ real{&constp->ಅಂಕೆ(), 64};
        real.opIToRD(constp->ಅಂಕೆ(), constp->isSigned());
        ಹಿಂತಿರುಗಿ ಹೊಸ AstConst{constp->fileline(), real};
    }

    // Clone a constant node, widening to targetWidth if needed (zero-extend).
    // Used to ensure comparisons use matching widths after V3Width has run.
    ಸ್ಥಿರ AstConst* widenConst(File_ಸಾಲು* fl, AstConst* constp, ಪೂರ್ಣಾಂಕ ಗುರಿ_ಅಗಲ) {
        ಆದರೆ (constp->ಅಗಲ() == ಗುರಿ_ಅಗಲ) ಹಿಂತಿರುಗಿ constp->clone_ವೃಕ್ಷ(ಸುಳ್ಳು);
        V3_ಸಂಖ್ಯೆ ಅಂಕೆ{fl, ಗುರಿ_ಅಗಲ, 0};
        ಅಂಕೆ.op_ನಿಯೋಜಿಸು(constp->ಅಂಕೆ());
        ಹಿಂತಿರುಗಿ ಹೊಸ AstConst{fl, ಅಂಕೆ};
    }

    // Build a range condition: minp <= exprp <= maxp.
    // Uses signed comparisons if exprp is signed; omits trivially-true domain bounds.
    // All arguments are non-owning; clones exprp/minp/maxp as needed.
    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* makeRangeCondition(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* exprp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* minp,
                                    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* maxp) {
        ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ ಅಭಿವ್ಯಕ್ತಿ_ಅಗಲ = exprp->ಅಗಲ_Min();
        AstConst* ಸ್ಥಿರಾಂಕ minConstp = VN_AS(minp, Const);
        AstConst* ಸ್ಥಿರಾಂಕ maxConstp = VN_AS(maxp, Const);
        ಆದರೆ (exprp->isDouble()) {
            // A real coverpoint has no finite domain bounds to omit.
            ಹಿಂತಿರುಗಿ ಹೊಸ AstAnd{fl,
                              ಹೊಸ AstGteD{fl, exprp->clone_ವೃಕ್ಷ(ಸುಳ್ಳು), newRealConst(minConstp)},
                              ಹೊಸ AstLteD{fl, exprp->clone_ವೃಕ್ಷ(ಸುಳ್ಳು), newRealConst(maxConstp)}};
        }
        // Widen constants to match expression width so post-V3Width nodes use correct macros
        AstConst* ಸ್ಥಿರಾಂಕ minWidep = widenConst(fl, minConstp, ಅಭಿವ್ಯಕ್ತಿ_ಅಗಲ);
        AstConst* ಸ್ಥಿರಾಂಕ maxWidep = widenConst(fl, maxConstp, ಅಭಿವ್ಯಕ್ತಿ_ಅಗಲ);
        V3_ಸಂಖ್ಯೆ minimum{fl, ಅಭಿವ್ಯಕ್ತಿ_ಅಗಲ, 0};
        V3_ಸಂಖ್ಯೆ ಗರಿಷ್ಠ{fl, ಅಭಿವ್ಯಕ್ತಿ_ಅಗಲ, 0};
        ಗರಿಷ್ಠ.ಗಣ_All_Bits1();
        ಆದರೆ (exprp->isSigned()) {
            minimum.ಗಣ_ಬಿಟ್(ಅಭಿವ್ಯಕ್ತಿ_ಅಗಲ - 1, 1);
            ಗರಿಷ್ಠ.ಗಣ_ಬಿಟ್(ಅಭಿವ್ಯಕ್ತಿ_ಅಗಲ - 1, 0);
        }
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lowerp = ಶೂನ್ಯಸೂಚಕ;
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* upperp = ಶೂನ್ಯಸೂಚಕ;
        ಆದರೆ (minWidep->ಅಂಕೆ().isCaseEq(minimum)) {
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(minWidep), minWidep);
        } ಇಲ್ಲದಿದ್ದರೆ {
            lowerp = exprp->isSigned() ? static_cast<Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ*>(
                                             ಹೊಸ AstGteS{fl, exprp->clone_ವೃಕ್ಷ(ಸುಳ್ಳು), minWidep})
                                       : static_cast<Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ*>(
                                             ಹೊಸ AstGte{fl, exprp->clone_ವೃಕ್ಷ(ಸುಳ್ಳು), minWidep});
        }
        ಆದರೆ (maxWidep->ಅಂಕೆ().isCaseEq(ಗರಿಷ್ಠ)) {
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(maxWidep), maxWidep);
        } ಇಲ್ಲದಿದ್ದರೆ {
            upperp = exprp->isSigned() ? static_cast<Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ*>(
                                             ಹೊಸ AstLteS{fl, exprp->clone_ವೃಕ್ಷ(ಸುಳ್ಳು), maxWidep})
                                       : static_cast<Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ*>(
                                             ಹೊಸ AstLte{fl, exprp->clone_ವೃಕ್ಷ(ಸುಳ್ಳು), maxWidep});
        }
        ಆದರೆ (lowerp && upperp) ಹಿಂತಿರುಗಿ ಹೊಸ AstAnd{fl, lowerp, upperp};
        ಆದರೆ (lowerp) ಹಿಂತಿರುಗಿ lowerp;
        ಆದರೆ (upperp) ಹಿಂತಿರುಗಿ upperp;
        ಹಿಂತಿರುಗಿ ಹೊಸ AstConst{fl, AstConst::ಬಿಟ್_True{}};
    }

    // Build a one-sided comparison for an open-ended bin range whose other bound is '$'.
    // '$' denotes the coverpoint domain extreme, so {[lo:$]} == (expr >= lo) and
    // {[$:hi]} == (expr <= hi).
    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* makeOpenRangeCondition(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* exprp, AstConst* boundp,
                                        ತಾರ್ಕಿಕ is_ಅಧೋ_Bound) {
        AstConst* ಸ್ಥಿರಾಂಕ widep = widenConst(fl, boundp, exprp->ಅಗಲ_Min());
        ಆದರೆ (is_ಅಧೋ_Bound) {
            ಆದರೆ (exprp->isSigned()) ಹಿಂತಿರುಗಿ ಹೊಸ AstGteS{fl, exprp->clone_ವೃಕ್ಷ(ಸುಳ್ಳು), widep};
            ಹಿಂತಿರುಗಿ ಹೊಸ AstGte{fl, exprp->clone_ವೃಕ್ಷ(ಸುಳ್ಳು), widep};
        }
        ಆದರೆ (exprp->isSigned()) ಹಿಂತಿರುಗಿ ಹೊಸ AstLteS{fl, exprp->clone_ವೃಕ್ಷ(ಸುಳ್ಳು), widep};
        ಹಿಂತಿರುಗಿ ಹೊಸ AstLte{fl, exprp->clone_ವೃಕ್ಷ(ಸುಳ್ಳು), widep};
    }

    // Build condition for a single transition item.
    // Returns expression that checks if exprp matches the item's value/range list.
    // Overload for when the expression is a variable read -- creates and manages the VarRef
    // internally, so callers don't need to construct a temporary node.
    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* build_Transition_ಅಂಶ_Condition(Ast_Cover_Trans_ಅಂಶ* itemp, AstVar* varp) {
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* varRefp = ಹೊಸ AstVarRef{varp->fileline(), varp, VAccess::READ};
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ condp = build_Transition_ಅಂಶ_Condition(itemp, varRefp);
        VL_DO_DANGLING(ತಳ್ಳು_Deletep(varRefp), varRefp);
        ಹಿಂತಿರುಗಿ condp;
    }

    // Non-owning: exprp is cloned internally; caller retains ownership of exprp.
    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* build_Transition_ಅಂಶ_Condition(Ast_Cover_Trans_ಅಂಶ* itemp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* exprp) {
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* condp = ಶೂನ್ಯಸೂಚಕ;

        ಗಾಗಿ (Ast_ಗಂಟು* valp = itemp->valuesp(); valp; valp = valp->nextp()) {
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* singleCondp = ಶೂನ್ಯಸೂಚಕ;
            valp = V3Const::constifyEdit(valp);
            AstConst* ಸ್ಥಿರಾಂಕ constp = VN_CAST(valp, Const);
            ಆದರೆ (!constp) {
                valp->v3error("Non-constant expression in transition bin; "
                              "values must be constants (IEEE 1800-2023 19.5)");
                ಹಿಂತಿರುಗಿ ಹೊಸ AstConst{valp->fileline(), AstConst::ಬಿಟ್_False_Erroring{}};
            }
            singleCondp
                = ಹೊಸ AstEq{constp->fileline(), exprp->clone_ವೃಕ್ಷ(ಸುಳ್ಳು), constp->clone_ವೃಕ್ಷ(ಸುಳ್ಳು)};
            ಆದರೆ (condp) {
                condp = ಹೊಸ AstOr{itemp->fileline(), condp, singleCondp};
            } ಇಲ್ಲದಿದ್ದರೆ {
                condp = singleCondp;
            }
        }

        ಹಿಂತಿರುಗಿ condp;
    }

    // Generate code for a single transition sequence (used by both regular and array bins)
    ಶೂನ್ಯ generate_Single_Transition_ಸಂಕೇತ(AstCoverpoint* coverpointp, AstCoverBin* binp,
                                      Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* exprp, ಸ್ಥಿರಾಂಕ Conv_Bin_ಗುರಿ& tgt,
                                      Ast_Cover_Trans_ಗಣ* transSetp) {
        UINFO(4, "      Generating code for transition sequence");

        // Get or create previous value variable
        AstVar* ಸ್ಥಿರಾಂಕ prevVarp = create_Prev_ಮೌಲ್ಯ_Var(coverpointp, exprp);

        UASSERT_OBJ(
            transSetp, binp,
            "Transition bin has no transition set (transp() was checked before calling this)");

        // Get transition items (the sequence: item1 => item2 => item3)
        std::ಸದಿಶ<Ast_Cover_Trans_ಅಂಶ*> ವಸ್ತುಗಳು;
        ಗಾಗಿ (Ast_ಗಂಟು* itemp = transSetp->itemsp(); itemp; itemp = itemp->nextp())
            ವಸ್ತುಗಳು.ತಳ್ಳು_ಹಿಂಭಾಗ(VN_AS(itemp, Cover_Trans_ಅಂಶ));

        ಆದರೆ (ವಸ್ತುಗಳು.ಖಾಲಿಗಳು()) {
            binp->v3error("Transition set without items");
            ಹಿಂತಿರುಗಿ;
        }

        ಆದರೆ (ವಸ್ತುಗಳು.ಗಾತ್ರ_2() == 1) {
            // Single item transition not valid (need at least 2 values for =>)
            binp->v3error("Transition requires at least two values");
            ಹಿಂತಿರುಗಿ;
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (ವಸ್ತುಗಳು.ಗಾತ್ರ_2() == 2) {
            // Simple two-value transition: (val1 => val2)
            // Use optimized direct comparison (no state machine needed)
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ cond1p = build_Transition_ಅಂಶ_Condition(ವಸ್ತುಗಳು[0], prevVarp);
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ cond2p = build_Transition_ಅಂಶ_Condition(ವಸ್ತುಗಳು[1], exprp);

            // Combine: prev matches val1 AND current matches val2
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ತುಂಬಿದ_Condp = ಹೊಸ AstAnd{binp->fileline(), cond1p, cond2p};

            ಕೂಡಿಸು_Conv_Trans_ದೊರಕಿತು_If(coverpointp, binp, tgt, ತುಂಬಿದ_Condp);

            UINFO(4, "        Successfully added 2-value transition if statement");
        } ಇಲ್ಲದಿದ್ದರೆ {
            // Multi-value sequence (a => b => c => ...)
            // Use state machine to track position in sequence
            generate_Multi_ಮೌಲ್ಯ_Transition_ಸಂಕೇತ(coverpointp, binp, exprp, tgt, ವಸ್ತುಗಳು);
        }
    }

    // Append a "{ VlCoverpoint* __Vcx_cps[] = {cp0, cp1, ...}; <call> }" statement.  The brace
    // and the temporary array stay literal text -- a CMethodHard is one call, not a block --
    // but callp itself carries the member, method and '->'.  Construction only: init() copies
    // the array into the cross, so sample() reads it from there and needs no array at all.
    AstCStmt* makeCrossCpsCall(File_ಸಾಲು* fl, ಸ್ಥಿರಾಂಕ std::ಸದಿಶ<AstVar*>& cpVars,
                               AstCMethodHard* callp) {
        AstCStmt* ಸ್ಥಿರಾಂಕ cs = ಹೊಸ AstCStmt{fl};
        cs->ಕೂಡಿಸು("{ VlCoverpoint* __Vcx_cps[] = {");
        ಗಾಗಿ (ಗಾತ್ರ_t d = 0; d < cpVars.ಗಾತ್ರ_2(); ++d) {
            ಆದರೆ (d != 0) cs->ಕೂಡಿಸು(", ");
            cs->ಕೂಡಿಸು(memberRef(fl, cpVars[d]));
        }
        cs->ಕೂಡಿಸು("}; ");
        cs->ಕೂಡಿಸು(callp);
        cs->ಕೂಡಿಸು("; }");
        ಹಿಂತಿರುಗಿ cs;
    }

    // Assign the per-bin flags individually: one-bit SV results have integer C++ storage types,
    // which may narrow in a bool initializer list but convert implicitly in assignments.
    AstCStmt* makeCrossIffsCall(File_ಸಾಲು* fl, ಸ್ಥಿರಾಂಕ std::ಸದಿಶ<AstCoverCrossBin*>& bins,
                                AstCMethodHard* callp) {
        AstCStmt* ಸ್ಥಿರಾಂಕ cs = ಹೊಸ AstCStmt{fl};
        cs->ಕೂಡಿಸು("{ bool __Vcx_iffs[" + cvt_ಗಮ್ಯ_Str(bins.ಗಾತ್ರ_2()) + "]; ");
        ಗಾಗಿ (ಗಾತ್ರ_t i = 0; i < bins.ಗಾತ್ರ_2(); ++i) {
            ಸ್ಥಿರಾಂಕ AstCoverCrossBin* ಸ್ಥಿರಾಂಕ binp = bins[i];
            cs->ಕೂಡಿಸು("__Vcx_iffs[" + cvt_ಗಮ್ಯ_Str(i) + "] = ");
            cs->ಕೂಡಿಸು(binp->iffp() ? binp->iffp()->clone_ವೃಕ್ಷ(ಸುಳ್ಳು)
                                 : ಹೊಸ AstConst{fl, AstConst::ಬಿಟ್_True{}});
            cs->ಕೂಡಿಸು("; ");
        }
        cs->ಕೂಡಿಸು(callp);
        cs->ಕೂಡಿಸು("; }");
        ಹಿಂತಿರುಗಿ cs;
    }

    ಬಳಸಿ CrossSelection = std::ಸದಿಶ<uint64_t>;
    ರಚನೆ ResolvedCrossBin final {
        AstCoverCrossBin* binp;
        CrossSelection selection;
    };
    ರಚನೆ CrossLayout final {
        uint32_t tuples = 0;
        uint32_t autoBins = 0;
        uint64_t bin_ಪದಗಳು = 0;
        ತಾರ್ಕಿಕ valid = ನಿಜ;
        std::ಸದಿಶ<ResolvedCrossBin> bins;
    };
    ರಚನೆ CrossSelectionContext final {
        AstCoverCross* crossp;  // Cross whose tuple space is being selected
        ಸ್ಥಿರಾಂಕ std::ಸದಿಶ<AstVar*>& cpVars;  // Feeding coverpoints in dimension order
        ಸ್ಥಿರಾಂಕ std::map<std::string, uint32_t>& dimensions;  // Coverpoint name -> dimension
        uint32_t tuples;  // Size of the Cartesian product
        std::ಸದಿಶ<uint32_t> strides;  // Flat-index stride per dimension
        ತಾರ್ಕಿಕ valid = ನಿಜ;  // False if this explicit bin cannot be implemented
    };
    ರಚನೆ Cross_Binsof_ಗುರಿ final {
        ಸ್ಥಿರಾಂಕ CoverpointBins* m_binsp = ಶೂನ್ಯಸೂಚಕ;  // Declared bins; null after a reported error
        uint32_t m_dimension = 0;  // Coverpoint index within the cross
        uint32_t m_ಮೊದಲ = 0;  // First declared normal bin in the selected span
        uint32_t m_ಎಣಿಕೆ = 0;  // Number of declared normal bins in the selected span
        uint32_t m_declared_ಮೊದಲ = 0;  // First runtime bin index of the selected span
        uint32_t m_declared_ಕೊನೆ = UINT32_MAX;  // One past the last runtime bin index
        int32_t m_sized = -1;  // Index of the sized array holding the selected bins; or -1
    };
    ರಚನೆ Cross_ಮೌಲ್ಯ_Range final {
        V3_ಸಂಖ್ಯೆ lo;  // Inclusive lower bound, sign-extended to the comparison width
        V3_ಸಂಖ್ಯೆ hi;  // Inclusive upper bound
        V3_ಸಂಖ್ಯೆ ನಮೂನೆ_2;  // Allowed bit values; all X for an ordinary interval
        ತಾರ್ಕಿಕ wildcard = ಸುಳ್ಳು;  // A wildcard singleton rather than an exact four-state value

        Cross_ಮೌಲ್ಯ_Range(Ast_ಗಂಟು* nodep, ಪೂರ್ಣಾಂಕ ಅಗಲ)
            : lo{nodep, ಅಗಲ}
            , hi{nodep, ಅಗಲ}
            , ನಮೂನೆ_2{nodep, ಅಗಲ} {
            ನಮೂನೆ_2.ಗಣ_All_ಬಿಟ್ಟುಗಳು_X();
        }
    };

    ಸ್ಥಿರ std::ಸದಿಶ<Ast_ಗಂಟು*> cross_Bin_ಮೌಲ್ಯಗಳು(ಸ್ಥಿರಾಂಕ Cross_Bin_ಮೌಲ್ಯಗಳು& bin) {
        ಆದರೆ (bin.valuep) ಹಿಂತಿರುಗಿ {bin.valuep};
        std::ಸದಿಶ<Ast_ಗಂಟು*> ಮೌಲ್ಯಗಳು;
        ಆದರೆ (bin.binp->transp()) {
            // IEEE 1800-2023 19.6.1: binsof uses the last value of each transition.
            ಗಾಗಿ (Ast_ಗಂಟು* setp = bin.binp->transp(); setp; setp = setp->nextp()) {
                Ast_Cover_Trans_ಅಂಶ* lastp = VN_AS(setp, Cover_Trans_ಗಣ)->itemsp();
                ಆಗ (lastp->nextp()) lastp = VN_AS(lastp->nextp(), Cover_Trans_ಅಂಶ);
                ಗಾಗಿ (Ast_ಗಂಟು* valuep = lastp->valuesp(); valuep; valuep = valuep->nextp()) {
                    ಮೌಲ್ಯಗಳು.ತಳ್ಳು_ಹಿಂಭಾಗ(valuep);
                }
            }
        } ಇಲ್ಲದಿದ್ದರೆ {
            ಗಾಗಿ (Ast_ಗಂಟು* valuep = bin.binp->rangesp(); valuep; valuep = valuep->nextp()) {
                ಮೌಲ್ಯಗಳು.ತಳ್ಳು_ಹಿಂಭಾಗ(valuep);
            }
        }
        ಹಿಂತಿರುಗಿ ಮೌಲ್ಯಗಳು;
    }

    // The values of the element-th bin of a run, sign-extended to 'width'
    ಸ್ಥಿರ Cross_ಮೌಲ್ಯ_Range ಸತತ_Bin_Range(Ast_ಗಂಟು* nodep, ಸ್ಥಿರಾಂಕ Bin_ಸತತ& ಸತತ, uint32_t element,
                                       ಪೂರ್ಣಾಂಕ ಅಗಲ) {
        ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ runw = ಸತತ.m_lo.ಅಗಲ();
        V3_ಸಂಖ್ಯೆ ಸ್ಥಾನಾಂತರ{nodep, runw};
        ಸ್ಥಾನಾಂತರ.opMul(ಸತತ.m_stride, V3_ಸಂಖ್ಯೆ{nodep, runw, element});
        V3_ಸಂಖ್ಯೆ lo{nodep, runw};
        lo.op_ಕೂಡಿಸು(ಸತತ.m_lo, ಸ್ಥಾನಾಂತರ);
        V3_ಸಂಖ್ಯೆ hi = ಸತತ.m_hi;
        ಆದರೆ (element + 1 < ಸತತ.m_ಎಣಿಕೆ) {
            ಸ್ಥಾನಾಂತರ.opSub(ಸತತ.m_stride, V3_ಸಂಖ್ಯೆ{nodep, runw, 1});
            hi.op_ಕೂಡಿಸು(lo, ಸ್ಥಾನಾಂತರ);
        }
        Cross_ಮೌಲ್ಯ_Range range{nodep, ಅಗಲ};
        range.lo.opExtendS(lo, runw);
        range.hi.opExtendS(hi, runw);
        ಹಿಂತಿರುಗಿ range;
    }

    ಸ್ಥಿರ ಪೂರ್ಣಾಂಕ cross_Range_ಅಗಲ(Ast_ಗಂಟು* nodep) {
        ಆದರೆ (ಸ್ಥಿರಾಂಕ AstInsideRange* ಸ್ಥಿರಾಂಕ rangep = VN_CAST(nodep, InsideRange)) {
            ಹಿಂತಿರುಗಿ std::max(rangep->lhsp()->ಅಗಲ(), rangep->rhsp()->ಅಗಲ());
        }
        ಹಿಂತಿರುಗಿ nodep->ಅಗಲ();
    }

    ಸ್ಥಿರ ತಾರ್ಕಿಕ cross_ಮೌಲ್ಯ_Less(ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& lhs, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& rhs) {
        V3_ಸಂಖ್ಯೆ ಫಲಿತಾಂಶ{&lhs};
        ಹಿಂತಿರುಗಿ !ಫಲಿತಾಂಶ.opLtS(lhs, rhs).isEqZero();
    }

    // Round a real bound inward to the nearest coverpoint value (IEEE 1800-2023 19.5.7).  Sets
    // 'empty' if the domain has no value on the bound's side.
    ಸ್ಥಿರ ಶೂನ್ಯ crossRealBound(ಸ್ಥಿರಾಂಕ AstConst* constp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* exprp, ತಾರ್ಕಿಕ ಊರ್ಧ್ವ,
                               ಸ್ಥಿರಾಂಕ Cross_ಮೌಲ್ಯ_Range& domain, V3_ಸಂಖ್ಯೆ& ಫಲಿತಾಂಶ, ತಾರ್ಕಿಕ& ಖಾಲಿಗಳು) {
        ಸ್ಥಿರಾಂಕ ದ್ವಿಗುಣ ಮೌಲ್ಯ = constp->ಅಂಕೆ().ಗಮ್ಯ_Double();
        ಸ್ಥಿರಾಂಕ ದ್ವಿಗುಣ bound = ಊರ್ಧ್ವ ? std::floor(ಮೌಲ್ಯ) : std::ceil(ಮೌಲ್ಯ);
        // Powers of two are exact, so the integral bound compares exactly with the domain edges.
        ಸ್ಥಿರಾಂಕ ದ್ವಿಗುಣ ಎಲ್ಲೆ
            = std::ldexp(1.0, exprp->isSigned() ? exprp->ಅಗಲ() - 1 : exprp->ಅಗಲ());
        ಸ್ಥಿರಾಂಕ ದ್ವಿಗುಣ minimum = exprp->isSigned() ? -ಎಲ್ಲೆ : 0.0;
        ಆದರೆ (std::isnan(bound) || (ಊರ್ಧ್ವ ? bound < minimum : bound >= ಎಲ್ಲೆ)) {
            ಖಾಲಿಗಳು = ನಿಜ;
            ಫಲಿತಾಂಶ = domain.lo;
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (bound < minimum) {
            ಫಲಿತಾಂಶ = domain.lo;
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (bound >= ಎಲ್ಲೆ) {
            ಫಲಿತಾಂಶ = domain.hi;
        } ಇಲ್ಲದಿದ್ದರೆ {
            V3_ಸಂಖ್ಯೆ real{&ಫಲಿತಾಂಶ, 64};
            real.ಗಣ_Double(bound);
            ಫಲಿತಾಂಶ.opRToIRoundS(real);
        }
    }

    ಸ್ಥಿರ ತಾರ್ಕಿಕ crossRangeBound(Ast_ಗಂಟು* nodep, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* exprp, ತಾರ್ಕಿಕ ಊರ್ಧ್ವ, ತಾರ್ಕಿಕ bin_ಮೌಲ್ಯ,
                                ಸ್ಥಿರಾಂಕ Cross_ಮೌಲ್ಯ_Range& domain, V3_ಸಂಖ್ಯೆ& ಫಲಿತಾಂಶ, ತಾರ್ಕಿಕ& ಖಾಲಿಗಳು) {
        ಆದರೆ (VN_IS(nodep, Unbounded)) {
            V3_ಸಂಖ್ಯೆ ಎಲ್ಲೆ{nodep, exprp->ಅಗಲ()};
            ಆದರೆ (ಊರ್ಧ್ವ) ಎಲ್ಲೆ.ಗಣ_All_Bits1();
            ಆದರೆ (exprp->isSigned()) {
                ಎಲ್ಲೆ.ಗಣ_ಬಿಟ್(exprp->ಅಗಲ() - 1, !ಊರ್ಧ್ವ);
                ಫಲಿತಾಂಶ.opExtendS(ಎಲ್ಲೆ, ಎಲ್ಲೆ.ಅಗಲ());
            } ಇಲ್ಲದಿದ್ದರೆ {
                ಫಲಿತಾಂಶ.op_ನಿಯೋಜಿಸು(ಎಲ್ಲೆ);
            }
            ಹಿಂತಿರುಗಿ ನಿಜ;
        }
        ಸ್ಥಿರಾಂಕ AstConst* ಸ್ಥಿರಾಂಕ constp = VN_CAST(nodep, Const);
        ಆದರೆ (!constp) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
        ಆದರೆ (constp->ಅಂಕೆ().isDouble()) {
            crossRealBound(constp, exprp, ಊರ್ಧ್ವ, domain, ಫಲಿತಾಂಶ, ಖಾಲಿಗಳು);
            ಹಿಂತಿರುಗಿ ನಿಜ;
        }
        ಆದರೆ (constp->ಅಂಕೆ().isString()) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
        ಆದರೆ (bin_ಮೌಲ್ಯ && exprp->isSigned() && constp->ಅಗಲ() <= exprp->ಅಗಲ()) {
            // Bin bit patterns use the coverpoint's effective type (IEEE 1800-2023 19.5.7).
            // Wider values and intersect filters retain their values for domain clipping.
            V3_ಸಂಖ್ಯೆ ಮೌಲ್ಯ{nodep, exprp->ಅಗಲ()};
            ಆದರೆ (constp->isSigned()) {
                ಮೌಲ್ಯ.opExtendS(constp->ಅಂಕೆ(), constp->ಅಗಲ());
            } ಇಲ್ಲದಿದ್ದರೆ {
                ಮೌಲ್ಯ.op_ನಿಯೋಜಿಸು(constp->ಅಂಕೆ());
            }
            ಫಲಿತಾಂಶ.opExtendS(ಮೌಲ್ಯ, ಮೌಲ್ಯ.ಅಗಲ());
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (constp->isSigned()) {
            ಫಲಿತಾಂಶ.opExtendS(constp->ಅಂಕೆ(), constp->ಅಗಲ());
        } ಇಲ್ಲದಿದ್ದರೆ {
            ಫಲಿತಾಂಶ.op_ನಿಯೋಜಿಸು(constp->ಅಂಕೆ());
        }
        ಹಿಂತಿರುಗಿ ನಿಜ;
    }

    ಸ್ಥಿರ Cross_ಮೌಲ್ಯ_Range cross_ಮೌಲ್ಯ_Domain(Ast_ಗಂಟು* nodep, ಪೂರ್ಣಾಂಕ ಮೌಲ್ಯ_ಅಗಲ, ತಾರ್ಕಿಕ isSigned,
                                            ಪೂರ್ಣಾಂಕ ಅಗಲ) {
        Cross_ಮೌಲ್ಯ_Range domain{nodep, ಅಗಲ};
        V3_ಸಂಖ್ಯೆ lo{nodep, ಮೌಲ್ಯ_ಅಗಲ};
        V3_ಸಂಖ್ಯೆ hi{nodep, ಮೌಲ್ಯ_ಅಗಲ};
        hi.ಗಣ_All_Bits1();
        ಆದರೆ (isSigned) {
            lo.ಗಣ_ಬಿಟ್(ಮೌಲ್ಯ_ಅಗಲ - 1, 1);
            hi.ಗಣ_ಬಿಟ್(ಮೌಲ್ಯ_ಅಗಲ - 1, 0);
            domain.lo.opExtendS(lo, ಮೌಲ್ಯ_ಅಗಲ);
            domain.hi.opExtendS(hi, ಮೌಲ್ಯ_ಅಗಲ);
        } ಇಲ್ಲದಿದ್ದರೆ {
            domain.lo.op_ನಿಯೋಜಿಸು(lo);
            domain.hi.op_ನಿಯೋಜಿಸು(hi);
        }
        ಹಿಂತಿರುಗಿ domain;
    }

    ಸ್ಥಿರ ಶೂನ್ಯ intersectCrossRange(Cross_ಮೌಲ್ಯ_Range& range, ಸ್ಥಿರಾಂಕ Cross_ಮೌಲ್ಯ_Range& other) {
        ಆದರೆ (cross_ಮೌಲ್ಯ_Less(range.lo, other.lo)) range.lo = other.lo;
        ಆದರೆ (cross_ಮೌಲ್ಯ_Less(other.hi, range.hi)) range.hi = other.hi;
    }

    ಸ್ಥಿರ ತಾರ್ಕಿಕ cross_ಮೌಲ್ಯ_Range(Ast_ಗಂಟು* nodep, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* exprp, ತಾರ್ಕಿಕ bin_ಮೌಲ್ಯ, ತಾರ್ಕಿಕ wildcard,
                                ಸ್ಥಿರಾಂಕ Cross_ಮೌಲ್ಯ_Range& domain, Cross_ಮೌಲ್ಯ_Range& range) {
        ತಾರ್ಕಿಕ ಖಾಲಿಗಳು = ಸುಳ್ಳು;
        ಸ್ಥಿರಾಂಕ AstConst* ಸ್ಥಿರಾಂಕ constp = VN_CAST(nodep, Const);
        ಆದರೆ (ಸ್ಥಿರಾಂಕ AstInsideRange* ಸ್ಥಿರಾಂಕ rangep = VN_CAST(nodep, InsideRange)) {
            ಆದರೆ (!crossRangeBound(rangep->lhsp(), exprp, ಸುಳ್ಳು, bin_ಮೌಲ್ಯ, domain, range.lo, ಖಾಲಿಗಳು)
                || !crossRangeBound(rangep->rhsp(), exprp, ನಿಜ, bin_ಮೌಲ್ಯ, domain, range.hi, ಖಾಲಿಗಳು)
                || range.lo.isFourState() || range.hi.isFourState()) {
                ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
            }
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (constp && constp->ಅಂಕೆ().isDouble()) {
            // A real value participates only if integral; it has no wildcard bits.
            crossRealBound(constp, exprp, ಸುಳ್ಳು, domain, range.lo, ಖಾಲಿಗಳು);
            crossRealBound(constp, exprp, ನಿಜ, domain, range.hi, ಖಾಲಿಗಳು);
        } ಇಲ್ಲದಿದ್ದರೆ {
            ಆದರೆ (!crossRangeBound(nodep, exprp, ಸುಳ್ಳು, bin_ಮೌಲ್ಯ, domain, range.lo, ಖಾಲಿಗಳು)) {
                ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
            }
            ಆದರೆ (bin_ಮೌಲ್ಯ && exprp->isSigned() && constp && !constp->isSigned()
                && constp->ಅಗಲ() > exprp->ಅಗಲ()) {
                ತಾರ್ಕಿಕ representable = ನಿಜ;
                ಗಾಗಿ (ಪೂರ್ಣಾಂಕ ಬಿಟ್_2 = exprp->ಅಗಲ(); ಬಿಟ್_2 < constp->ಅಗಲ(); ++ಬಿಟ್_2) {
                    representable &= !constp->ಅಂಕೆ().ಬಿಟ್_Is1(ಬಿಟ್_2);
                }
                ಆದರೆ (representable) {
                    V3_ಸಂಖ್ಯೆ ಮೌಲ್ಯ{nodep, exprp->ಅಗಲ()};
                    ಮೌಲ್ಯ.op_ನಿಯೋಜಿಸು(constp->ಅಂಕೆ());
                    range.lo.opExtendS(ಮೌಲ್ಯ, ಮೌಲ್ಯ.ಅಗಲ());
                }
            }
            range.hi = range.lo;
            range.wildcard = wildcard;
            ಆದರೆ (wildcard) {
                range.ನಮೂನೆ_2 = range.lo;
                range.lo = domain.lo;
                range.hi = domain.hi;
                ಆದರೆ (constp && constp->isSigned()) {
                    // Replicated X sign bits are correlated, not independent wildcards.
                    // The source domain preserves expansion-before-casting (19.5.7).
                    intersectCrossRange(
                        range, cross_ಮೌಲ್ಯ_Domain(nodep, constp->ಅಗಲ(), ನಿಜ, domain.lo.ಅಗಲ()));
                }
            }
        }
        ಆದರೆ (ಖಾಲಿಗಳು) {
            range.lo = domain.hi;
            range.hi = domain.lo;
            ಹಿಂತಿರುಗಿ ನಿಜ;
        }
        ಆದರೆ (!range.lo.isFourState()) intersectCrossRange(range, domain);
        ಆದರೆ (range.wildcard && exprp->isSigned()) {
            ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ sign = exprp->ಅಗಲ() - 1;
            ಆದರೆ (constp && !constp->isSigned() && constp->ಅಗಲ() > exprp->ಅಗಲ()) {
                // Unsigned equality preserves every target bit pattern if discarded bits are zero.
                ಗಾಗಿ (ಪೂರ್ಣಾಂಕ ಬಿಟ್_2 = exprp->ಅಗಲ(); ಬಿಟ್_2 < constp->ಅಗಲ(); ++ಬಿಟ್_2) {
                    ಆದರೆ (constp->ಅಂಕೆ().ಬಿಟ್_Is1(ಬಿಟ್_2)) {
                        range.lo = domain.hi;
                        range.hi = domain.lo;
                        ಹಿಂತಿರುಗಿ ನಿಜ;
                    }
                }
                ಗಾಗಿ (ಪೂರ್ಣಾಂಕ ಬಿಟ್_2 = exprp->ಅಗಲ(); ಬಿಟ್_2 < range.ನಮೂನೆ_2.ಅಗಲ(); ++ಬಿಟ್_2) {
                    ಆದರೆ (range.ನಮೂನೆ_2.ಬಿಟ್_Is_XZ(sign))
                        range.ನಮೂನೆ_2.ಗಣ_ಬಿಟ್(ಬಿಟ್_2, 'x');
                    ಇಲ್ಲದಿದ್ದರೆ
                        range.ನಮೂನೆ_2.ಗಣ_ಬಿಟ್(ಬಿಟ್_2, range.ನಮೂನೆ_2.ಬಿಟ್_Is1(sign));
                }
                ಹಿಂತಿರುಗಿ ನಿಜ;
            }
            // Discarded high bits constrain the target sign, rather than becoming don't-cares.
            ಗಾಗಿ (ಪೂರ್ಣಾಂಕ ಬಿಟ್_2 = exprp->ಅಗಲ(); ಬಿಟ್_2 < range.ನಮೂನೆ_2.ಅಗಲ(); ++ಬಿಟ್_2) {
                ಆದರೆ (range.ನಮೂನೆ_2.ಬಿಟ್_Is_XZ(ಬಿಟ್_2)) ಮುಂದುವರಿ;
                ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ ಮೌಲ್ಯ = range.ನಮೂನೆ_2.ಬಿಟ್_Is1(ಬಿಟ್_2);
                ಆದರೆ (!range.ನಮೂನೆ_2.ಬಿಟ್_Is_XZ(sign) && range.ನಮೂನೆ_2.ಬಿಟ್_Is1(sign) != ಮೌಲ್ಯ) {
                    range.lo = domain.hi;
                    range.hi = domain.lo;
                    ನಿಲ್ಲಿಸು;
                }
                range.ನಮೂನೆ_2.ಗಣ_ಬಿಟ್(sign, ಮೌಲ್ಯ);
            }
        }
        ಹಿಂತಿರುಗಿ ನಿಜ;
    }

    ಎಣಿಕೆ CrossRangeState : uint8_t {
        CROSS_INSIDE_BOUNDS = 0,  // Prefix is strictly inside the interval
        CROSS_AT_ಅಧೋ = 1,
        CROSS_AT_ಊರ್ಧ್ವ = 2,
        CROSS_AT_BOUNDS = CROSS_AT_ಅಧೋ | CROSS_AT_ಊರ್ಧ್ವ,
        CROSS_NO_ಹೊಂದಿಕೆ = 4  // Prefix cannot match the interval/pattern
    };

    ಸ್ಥಿರ CrossRangeState cross_Range_ಹೆಜ್ಜೆ(ಸ್ಥಿರಾಂಕ Cross_ಮೌಲ್ಯ_Range& range, CrossRangeState state,
                                          ಪೂರ್ಣಾಂಕ ಬಿಟ್_2, ಪೂರ್ಣಾಂಕ ಮೌಲ್ಯ) {
        ಆದರೆ (state == CROSS_NO_ಹೊಂದಿಕೆ) ಹಿಂತಿರುಗಿ CROSS_NO_ಹೊಂದಿಕೆ;
        // Flipping the sign bit makes signed order lexicographic.
        ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ sign = ಬಿಟ್_2 == range.ನಮೂನೆ_2.ಅಗಲ() - 1;
        ಆದರೆ (!range.ನಮೂನೆ_2.ಬಿಟ್_Is_XZ(ಬಿಟ್_2) && ಮೌಲ್ಯ != (range.ನಮೂನೆ_2.ಬಿಟ್_Is1(ಬಿಟ್_2) ^ sign)) {
            ಹಿಂತಿರುಗಿ CROSS_NO_ಹೊಂದಿಕೆ;
        }
        ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ ಕೆಳಗಿನ = range.lo.ಬಿಟ್_Is1(ಬಿಟ್_2) ^ sign;
        ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ ಮೇಲಿನ = range.hi.ಬಿಟ್_Is1(ಬಿಟ್_2) ^ sign;
        ಆದರೆ (((state & CROSS_AT_ಅಧೋ) && ಮೌಲ್ಯ < ಕೆಳಗಿನ)
            || ((state & CROSS_AT_ಊರ್ಧ್ವ) && ಮೌಲ್ಯ > ಮೇಲಿನ)) {
            ಹಿಂತಿರುಗಿ CROSS_NO_ಹೊಂದಿಕೆ;
        }
        ಹಿಂತಿರುಗಿ static_cast<CrossRangeState>(
            ((state & CROSS_AT_ಅಧೋ) && ಮೌಲ್ಯ == ಕೆಳಗಿನ ? CROSS_AT_ಅಧೋ : CROSS_INSIDE_BOUNDS)
            | ((state & CROSS_AT_ಊರ್ಧ್ವ) && ಮೌಲ್ಯ == ಮೇಲಿನ ? CROSS_AT_ಊರ್ಧ್ವ : CROSS_INSIDE_BOUNDS));
    }

    ಸ್ಥಿರ ತಾರ್ಕಿಕ crossWildcardIntersects(ಸ್ಥಿರಾಂಕ Cross_ಮೌಲ್ಯ_Range& range) {
        ಚಿಹ್ನೆರಹಿತ ಸ್ಥಿತಿಗಳು = 1U << CROSS_AT_BOUNDS;
        ಗಾಗಿ (ಪೂರ್ಣಾಂಕ ಬಿಟ್_2 = range.ನಮೂನೆ_2.ಅಗಲ() - 1; ಬಿಟ್_2 >= 0 && ಸ್ಥಿತಿಗಳು; --ಬಿಟ್_2) {
            ಚಿಹ್ನೆರಹಿತ ಮುಂದಿನ = 0;
            ಗಾಗಿ (ಸ್ಥಿರಾಂಕ CrossRangeState state :
                 {CROSS_INSIDE_BOUNDS, CROSS_AT_ಅಧೋ, CROSS_AT_ಊರ್ಧ್ವ, CROSS_AT_BOUNDS}) {
                ಆದರೆ (!(ಸ್ಥಿತಿಗಳು & (1U << state))) ಮುಂದುವರಿ;
                ಗಾಗಿ (ಪೂರ್ಣಾಂಕ ಮೌಲ್ಯ = 0; ಮೌಲ್ಯ < 2; ++ಮೌಲ್ಯ) {
                    ಸ್ಥಿರಾಂಕ CrossRangeState equal = cross_Range_ಹೆಜ್ಜೆ(range, state, ಬಿಟ್_2, ಮೌಲ್ಯ);
                    ಆದರೆ (equal != CROSS_NO_ಹೊಂದಿಕೆ) ಮುಂದಿನ |= 1U << equal;
                }
            }
            ಸ್ಥಿತಿಗಳು = ಮುಂದಿನ;
        }
        ಹಿಂತಿರುಗಿ ಸ್ಥಿತಿಗಳು != 0;
    }

    // The least value at least 'from' with the non-x bits of 'pattern', in unsigned order; false
    // if none.  A bit scan, which does not enumerate the pattern's x bits.
    ಸ್ಥಿರ ತಾರ್ಕಿಕ ಮುಂದಿನ_ನಮೂನೆ_ಮೌಲ್ಯ(ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& ನಮೂನೆ_2, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& from, V3_ಸಂಖ್ಯೆ& ಫಲಿತಾಂಶ) {
        ಫಲಿತಾಂಶ.op_ನಿಯೋಜಿಸು(from);
        ಪೂರ್ಣಾಂಕ ವಾಹಕ = -1;  // Lowest x bit above the bit scanned where 'from' has a 0
        ಗಾಗಿ (ಪೂರ್ಣಾಂಕ ಬಿಟ್_2 = from.ಅಗಲ() - 1; ಬಿಟ್_2 >= 0; --ಬಿಟ್_2) {
            ಆದರೆ (ನಮೂನೆ_2.ಬಿಟ್_Is_XZ(ಬಿಟ್_2)) {
                ಆದರೆ (!from.ಬಿಟ್_Is1(ಬಿಟ್_2)) ವಾಹಕ = ಬಿಟ್_2;
                ಮುಂದುವರಿ;
            }
            ಆದರೆ (ನಮೂನೆ_2.ಬಿಟ್_Is1(ಬಿಟ್_2) == from.ಬಿಟ್_Is1(ಬಿಟ್_2)) ಮುಂದುವರಿ;
            ಆದರೆ (from.ಬಿಟ್_Is1(ಬಿಟ್_2)) {  // Only a greater prefix, an x bit above raised, can match
                ಆದರೆ (ವಾಹಕ < 0) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
                ಬಿಟ್_2 = ವಾಹಕ;
            }
            // Then the least such value: the pattern's bits, with x bits of zero
            ಫಲಿತಾಂಶ.ಗಣ_ಬಿಟ್(ಬಿಟ್_2, 1);
            ಆಗ (--ಬಿಟ್_2 >= 0) ಫಲಿತಾಂಶ.ಗಣ_ಬಿಟ್(ಬಿಟ್_2, ನಮೂನೆ_2.ಬಿಟ್_Is1(ಬಿಟ್_2));
            ಹಿಂತಿರುಗಿ ನಿಜ;
        }
        ಹಿಂತಿರುಗಿ ನಿಜ;
    }

    // Append to 'runs' the maximal runs of consecutive values of 'range' that its pattern
    // matches, in value order, until there are more than 'limit'.  A match continues through
    // the pattern's trailing x bits only, which the next value's carry leaves.
    ಸ್ಥಿರ ಶೂನ್ಯ crossRangeRuns(ಸ್ಥಿರಾಂಕ Cross_ಮೌಲ್ಯ_Range& range, ಗಾತ್ರ_t ಎಲ್ಲೆ,
                               std::ಸದಿಶ<std::pair<V3_ಸಂಖ್ಯೆ, V3_ಸಂಖ್ಯೆ>>& runs) {
        // Values order signed, which is the unsigned order of values with the sign bit flipped
        ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ sign = range.lo.ಅಗಲ() - 1;
        ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ flipped = [sign](V3_ಸಂಖ್ಯೆ ಮೌಲ್ಯ) {
            ಆದರೆ (!ಮೌಲ್ಯ.ಬಿಟ್_Is_XZ(sign)) ಮೌಲ್ಯ.ಗಣ_ಬಿಟ್(sign, !ಮೌಲ್ಯ.ಬಿಟ್_Is1(sign));
            ಹಿಂತಿರುಗಿ ಮೌಲ್ಯ;
        };
        ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ ನಮೂನೆ_2 = flipped(range.ನಮೂನೆ_2);
        ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ hi = flipped(range.hi);
        ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ one{&hi, hi.ಅಗಲ(), 1};
        V3_ಸಂಖ್ಯೆ trailing{&hi, hi.ಅಗಲ()};
        ಗಾಗಿ (ಪೂರ್ಣಾಂಕ ಬಿಟ್_2 = 0; ಬಿಟ್_2 <= sign && ನಮೂನೆ_2.ಬಿಟ್_Is_XZ(ಬಿಟ್_2); ++ಬಿಟ್_2) trailing.ಗಣ_ಬಿಟ್(ಬಿಟ್_2, 1);
        V3_ಸಂಖ್ಯೆ from = flipped(range.lo);
        V3_ಸಂಖ್ಯೆ ಮೊದಲ{&hi, hi.ಅಗಲ()};
        V3_ಸಂಖ್ಯೆ ಕೊನೆಯ{&hi, hi.ಅಗಲ()};
        V3_ಸಂಖ್ಯೆ less{&hi};
        ಆಗ (runs.ಗಾತ್ರ_2() <= ಎಲ್ಲೆ && ಮುಂದಿನ_ನಮೂನೆ_ಮೌಲ್ಯ(ನಮೂನೆ_2, from, ಮೊದಲ)
               && less.opLt(hi, ಮೊದಲ).isEqZero()) {
            ಕೊನೆಯ.opOr(ಮೊದಲ, trailing);
            ಆದರೆ (!less.opLt(hi, ಕೊನೆಯ).isEqZero()) ಕೊನೆಯ = hi;
            runs.emplace_ಹಿಂಭಾಗ(flipped(ಮೊದಲ), flipped(ಕೊನೆಯ));
            ಆದರೆ (ಕೊನೆಯ.isCaseEq(hi)) ನಿಲ್ಲಿಸು;
            from.op_ಕೂಡಿಸು(ಕೊನೆಯ, one);
        }
    }

    // True if no coverpoint value participates in a resolved value or range.  A value with x
    // or z bits participates only as a wildcard pattern (IEEE 1800-2023 19.5.7).
    ಸ್ಥಿರ ತಾರ್ಕಿಕ cross_Range_ಖಾಲಿಗಳು(ಸ್ಥಿರಾಂಕ Cross_ಮೌಲ್ಯ_Range& range) {
        ಆದರೆ (range.lo.isFourState()) ಹಿಂತಿರುಗಿ ನಿಜ;
        ಹಿಂತಿರುಗಿ cross_ಮೌಲ್ಯ_Less(range.hi, range.lo)
               || (range.wildcard && !crossWildcardIntersects(range));
    }

    // Comparison width that holds both the coverpoint's and a bin or intersect value's range.
    ಸ್ಥಿರ ಪೂರ್ಣಾಂಕ resolve_ಅಗಲ(Ast_ಗಂಟು* nodep, ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* exprp) {
        ಹಿಂತಿರುಗಿ std::max(exprp->ಅಗಲ(), cross_Range_ಅಗಲ(nodep)) + 1;
    }

    // Resolve a bin or intersect value to coverpoint values (IEEE 1800-2023 19.5.7), in the
    // width 'range' was created with.  False if the value is not a constant integral or real.
    ಸ್ಥಿರ ತಾರ್ಕಿಕ resolve_ಮೌಲ್ಯ(Ast_ಗಂಟು* nodep, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* exprp, ತಾರ್ಕಿಕ bin_ಮೌಲ್ಯ, ತಾರ್ಕಿಕ wildcard,
                             Cross_ಮೌಲ್ಯ_Range& range) {
        ಸ್ಥಿರಾಂಕ Cross_ಮೌಲ್ಯ_Range domain
            = cross_ಮೌಲ್ಯ_Domain(nodep, exprp->ಅಗಲ(), exprp->isSigned(), range.lo.ಅಗಲ());
        ಹಿಂತಿರುಗಿ cross_ಮೌಲ್ಯ_Range(nodep, exprp, bin_ಮೌಲ್ಯ, wildcard, domain, range);
    }

    ಸ್ಥಿರ ತಾರ್ಕಿಕ crossRangesIntersect(ಸ್ಥಿರಾಂಕ Cross_ಮೌಲ್ಯ_Range& bin, ಸ್ಥಿರಾಂಕ Cross_ಮೌಲ್ಯ_Range& filter) {
        // Values with x or z bits do not participate, even in an identical filter.
        ಆದರೆ (bin.lo.isFourState() || filter.lo.isFourState()) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
        Cross_ಮೌಲ್ಯ_Range ಹೊಂದಿಕೆ = bin;
        intersectCrossRange(ಹೊಂದಿಕೆ, filter);
        ಹಿಂತಿರುಗಿ !cross_ಮೌಲ್ಯ_Less(ಹೊಂದಿಕೆ.hi, ಹೊಂದಿಕೆ.lo)
               && (!bin.wildcard || crossWildcardIntersects(ಹೊಂದಿಕೆ));
    }

    ಸ್ಥಿರ ಶೂನ್ಯ unsupportedCrossRange(AstCoverBinsof* selectp, ತಾರ್ಕಿಕ& valid) {
        selectp->v3warn(COVERIGN, "Unsupported: non-constant or non-integral 'intersect' value, "
                                  "or four-state range bound.");
        valid = ಸುಳ್ಳು;
    }

    ಸ್ಥಿರ ತಾರ್ಕಿಕ cross_ಮೌಲ್ಯ_Matches_Filters(AstCoverBinsof* selectp, Ast_ಗಂಟು* valuep,
                                         Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* exprp, ಸ್ಥಿರಾಂಕ AstCoverBin* binp,
                                         ಸ್ಥಿರಾಂಕ Cross_ಮೌಲ್ಯ_Range& domain,
                                         ಸ್ಥಿರಾಂಕ std::ಸದಿಶ<Cross_ಮೌಲ್ಯ_Range>& filters,
                                         ತಾರ್ಕಿಕ& valid) {
        Cross_ಮೌಲ್ಯ_Range range{valuep, domain.lo.ಅಗಲ()};
        ಆದರೆ (!cross_ಮೌಲ್ಯ_Range(valuep, exprp, ನಿಜ, binp->isWildcard(), domain, range)) {
            unsupportedCrossRange(selectp, valid);
            ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
        }
        ಗಾಗಿ (ಸ್ಥಿರಾಂಕ Cross_ಮೌಲ್ಯ_Range& filter : filters) {
            ಆದರೆ (crossRangesIntersect(range, filter)) ಹಿಂತಿರುಗಿ ನಿಜ;
        }
        ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
    }

    std::ಸದಿಶ<ತಾರ್ಕಿಕ> selectCoverpointBins(AstCoverBinsof* selectp, ಸ್ಥಿರಾಂಕ CoverpointBins& bins,
                                           uint32_t ಮೊದಲ, uint32_t ಎಣಿಕೆ_2, ತಾರ್ಕಿಕ& valid) {
        ಆದರೆ (selectp->rangesp() && !bins.exprp->dtypep()->skipRefp()->isIntegralOrPacked()) {
            unsupportedCrossRange(selectp, valid);
            ಹಿಂತಿರುಗಿ {};
        }
        std::ಸದಿಶ<ತಾರ್ಕಿಕ> selected(bins.ಒಟ್ಟು, ಸುಳ್ಳು);
        std::ಸದಿಶ<std::ಸದಿಶ<Ast_ಗಂಟು*>> ಮೌಲ್ಯಗಳು;
        ಪೂರ್ಣಾಂಕ ಅಗಲ = bins.exprp->ಅಗಲ();
        ಗಾಗಿ (Ast_ಗಂಟು* rangep = selectp->rangesp(); rangep; rangep = rangep->nextp()) {
            ಅಗಲ = std::max(ಅಗಲ, cross_Range_ಅಗಲ(rangep));
        }
        ಆದರೆ (selectp->rangesp()) {
            ಮೌಲ್ಯಗಳು.reserve(ಎಣಿಕೆ_2);
            ಗಾಗಿ (uint32_t i = ಮೊದಲ; i < ಮೊದಲ + ಎಣಿಕೆ_2; ++i) {
                // A run's values are in the coverpoint type, whose width 'width' covers
                ಮೌಲ್ಯಗಳು.ತಳ್ಳು_ಹಿಂಭಾಗ(bins.ಮೌಲ್ಯಗಳು[i].runp ? std::ಸದಿಶ<Ast_ಗಂಟು*>{}
                                                     : cross_Bin_ಮೌಲ್ಯಗಳು(bins.ಮೌಲ್ಯಗಳು[i]));
                ಗಾಗಿ (Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ valuep : ಮೌಲ್ಯಗಳು.ಹಿಂಭಾಗ()) {
                    ಅಗಲ = std::max(ಅಗಲ, cross_Range_ಅಗಲ(valuep));
                }
            }
        }
        // One extra bit preserves both unsigned maxima and negative signed bounds.
        ++ಅಗಲ;
        ಸ್ಥಿರಾಂಕ Cross_ಮೌಲ್ಯ_Range domain
            = cross_ಮೌಲ್ಯ_Domain(selectp, bins.exprp->ಅಗಲ(), bins.exprp->isSigned(), ಅಗಲ);
        std::ಸದಿಶ<Cross_ಮೌಲ್ಯ_Range> filters;
        ಗಾಗಿ (Ast_ಗಂಟು* rangep = selectp->rangesp(); rangep; rangep = rangep->nextp()) {
            Cross_ಮೌಲ್ಯ_Range filter{rangep, ಅಗಲ};
            ಆದರೆ (!cross_ಮೌಲ್ಯ_Range(rangep, bins.exprp, ಸುಳ್ಳು, ಸುಳ್ಳು, domain, filter)) {
                unsupportedCrossRange(selectp, valid);
                ಹಿಂತಿರುಗಿ {};
            }
            filters.ತಳ್ಳು_ಹಿಂಭಾಗ(std::move(filter));
        }
        ಗಾಗಿ (uint32_t i = ಮೊದಲ; i < ಮೊದಲ + ಎಣಿಕೆ_2; ++i) {
            ಆದರೆ (!selectp->rangesp()) {
                selected[i] = ನಿಜ;
                ಮುಂದುವರಿ;
            }
            ಆದರೆ (ಸ್ಥಿರಾಂಕ Bin_ಸತತ* ಸ್ಥಿರಾಂಕ runp = bins.ಮೌಲ್ಯಗಳು[i].runp) {
                ಸ್ಥಿರಾಂಕ Cross_ಮೌಲ್ಯ_Range range
                    = ಸತತ_Bin_Range(selectp, *runp, bins.ಮೌಲ್ಯಗಳು[i].element, ಅಗಲ);
                selected[i] = std::any_ದ(filters.begin(), filters.ಕೊನೆ(),
                                          [&](ಸ್ಥಿರಾಂಕ Cross_ಮೌಲ್ಯ_Range& filter) {
                                              ಹಿಂತಿರುಗಿ crossRangesIntersect(range, filter);
                                          });
                ಮುಂದುವರಿ;
            }
            ಗಾಗಿ (Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ valuep : ಮೌಲ್ಯಗಳು[i - ಮೊದಲ]) {
                selected[i] = cross_ಮೌಲ್ಯ_Matches_Filters(
                    selectp, valuep, bins.exprp, bins.ಮೌಲ್ಯಗಳು[i].binp, domain, filters, valid);
                ಆದರೆ (!valid) ಹಿಂತಿರುಗಿ {};
                ಆದರೆ (selected[i]) ನಿಲ್ಲಿಸು;
            }
        }
        ಆದರೆ (selectp->isNegated()) {
            ಗಾಗಿ (uint32_t i = 0; i < bins.ಒಟ್ಟು; ++i) selected[i] = !selected[i];
        }
        ಹಿಂತಿರುಗಿ selected;
    }

    // Constructor-time value metadata of one coverpoint, as C++ list entries
    ರಚನೆ ಮೌಲ್ಯ_Lists final {
        std::ಸದಿಶ<std::string> m_ranges;  // Bin, then low and high words
        std::ಸದಿಶ<std::string> m_runs;  // First bin, count, then low, span, and high words
        std::ಸದಿಶ<std::string> m_patterns;  // Bin, then value, mask, low, and high words
        std::ಸದಿಶ<std::string> m_transitions;  // Transition bin
    };

    // Append a value's words, in the coverpoint's width, to a C++ list entry.
    ಸ್ಥಿರ ಶೂನ್ಯ append_ಪದಗಳು(std::string& ಪಠ್ಯ, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& ಮೌಲ್ಯ, ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* exprp) {
        V3_ಸಂಖ್ಯೆ narrowed{&ಮೌಲ್ಯ, exprp->ಅಗಲ()};
        narrowed.op_ನಿಯೋಜಿಸು(ಮೌಲ್ಯ);
        ಗಾಗಿ (ಪೂರ್ಣಾಂಕ ಶಬ್ದ = 0; ಶಬ್ದ < exprp->ಅಗಲ_ಪದಗಳು(); ++ಶಬ್ದ) {
            ಪಠ್ಯ += ", " + cvt_ಗಮ್ಯ_Str(narrowed.edata_ಶಬ್ದ(ಶಬ್ದ)) + "U";
        }
    }

    ಶೂನ್ಯ collect_ಮೌಲ್ಯ_Metadata(ಮೌಲ್ಯ_Lists& lists, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* exprp, AstCoverBin* binp,
                              uint32_t ಸೂಚ್ಯಂಕ, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* valuep) {
        ಸ್ಥಿರಾಂಕ std::string bin = cvt_ಗಮ್ಯ_Str(ಸೂಚ್ಯಂಕ) + "U";
        ಆದರೆ (binp->transp()) lists.m_transitions.ತಳ್ಳು_ಹಿಂಭಾಗ(bin);
        ಗಾಗಿ (Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ sourcep : cross_Bin_ಮೌಲ್ಯಗಳು({binp, valuep})) {
            Cross_ಮೌಲ್ಯ_Range range{sourcep, resolve_ಅಗಲ(sourcep, exprp)};
            ಆದರೆ (!resolve_ಮೌಲ್ಯ(sourcep, exprp, ನಿಜ, binp->isWildcard(), range)) {
                // Sampling already resolved every state bin value, so only transitions remain.
                sourcep->v3warn(E_UNSUPPORTED, "Unsupported: non-integral value in a transition "
                                               "bin of a coverpoint with exclusions.");
                ಮುಂದುವರಿ;
            }
            ಆದರೆ (cross_Range_ಖಾಲಿಗಳು(range)) ಮುಂದುವರಿ;
            std::string ನಮೂದು = bin;
            ಆದರೆ (range.wildcard) {
                V3_ಸಂಖ್ಯೆ ಮೌಲ್ಯ{sourcep, exprp->ಅಗಲ()};
                V3_ಸಂಖ್ಯೆ ಮುಖವಾಡ{sourcep, exprp->ಅಗಲ()};
                ಮುಖವಾಡ.op_ಬಿಟ್ಟುಗಳು_Non_XZ(range.ನಮೂನೆ_2);
                ಮೌಲ್ಯ.op_ಬಿಟ್ಟುಗಳು_One(range.ನಮೂನೆ_2);
                append_ಪದಗಳು(ನಮೂದು, ಮೌಲ್ಯ, exprp);
                append_ಪದಗಳು(ನಮೂದು, ಮುಖವಾಡ, exprp);
            }
            append_ಪದಗಳು(ನಮೂದು, range.lo, exprp);
            append_ಪದಗಳು(ನಮೂದು, range.hi, exprp);
            (range.wildcard ? lists.m_patterns : lists.m_ranges).ತಳ್ಳು_ಹಿಂಭಾಗ(ನಮೂದು);
        }
    }

    // Describe a run with one entry, from which the runtime computes the values of its bins.
    ಸ್ಥಿರ ಶೂನ್ಯ collect_ಸತತ_Metadata(ಮೌಲ್ಯ_Lists& lists, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* exprp, ಸ್ಥಿರಾಂಕ Bin_ಸತತ& ಸತತ) {
        V3_ಸಂಖ್ಯೆ ಹರವು{exprp, ಸತತ.m_stride.ಅಗಲ(), 0};
        ಹರವು.opSub(ಸತತ.m_stride, V3_ಸಂಖ್ಯೆ{exprp, ಸತತ.m_stride.ಅಗಲ(), 1});
        std::string ನಮೂದು = cvt_ಗಮ್ಯ_Str(ಸತತ.m_declared) + "U, " + cvt_ಗಮ್ಯ_Str(ಸತತ.m_ಎಣಿಕೆ) + "U";
        append_ಪದಗಳು(ನಮೂದು, ಸತತ.m_lo, exprp);
        append_ಪದಗಳು(ನಮೂದು, ಹರವು, exprp);
        append_ಪದಗಳು(ನಮೂದು, ಸತತ.m_hi, exprp);
        lists.m_runs.ತಳ್ಳು_ಹಿಂಭಾಗ(ನಮೂದು);
    }

    // Emit one batched metadata list, bounding the size of each call's temporary list.
    ಶೂನ್ಯ emit_ಮೌಲ್ಯ_List(File_ಸಾಲು* fl, AstVar* cpVarp, VCMethod ವಿಧಾನ,
                       ಸ್ಥಿರಾಂಕ std::ಸದಿಶ<std::string>& entries) {
        ಗಾಗಿ (ಗಾತ್ರ_t ಮೊದಲ = 0; ಮೊದಲ < entries.ಗಾತ್ರ_2(); ಮೊದಲ += ಮೌಲ್ಯ_LIST_ENTRIES) {
            ಸ್ಥಿರಾಂಕ ಗಾತ್ರ_t ಕೊನೆ = std::min(entries.ಗಾತ್ರ_2(), ಮೊದಲ + ಮೌಲ್ಯ_LIST_ENTRIES);
            std::string ಪಠ್ಯ = "{" + entries[ಮೊದಲ];
            ಗಾಗಿ (ಗಾತ್ರ_t i = ಮೊದಲ + 1; i < ಕೊನೆ; ++i) ಪಠ್ಯ += ", " + entries[i];
            m_constructorp->ಕೂಡಿಸು_Stmtsp(
                ಅಂಶ_Call(fl, cpVarp, ವಿಧಾನ, {ctext(fl, ಪಠ್ಯ + "}")})->makeStmt());
        }
    }

    ಸ್ಥಿರ ತಾರ್ಕಿಕ checkCrossRef(ಸ್ಥಿರಾಂಕ AstCoverCrossRef* refp, ಸ್ಥಿರಾಂಕ AstCoverCross* crossp) {
        ಆದರೆ (refp->name() == crossp->name()) ಹಿಂತಿರುಗಿ ನಿಜ;
        refp->v3error("Cross selection "
                      << refp->prettyNameQ() << " may only name its enclosing cross "
                      << crossp->prettyNameQ() << " (IEEE 1800-2023 19.6.1.2).");
        ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
    }

    ಸ್ಥಿರ ತಾರ್ಕಿಕ checkCrossBinName(ಸ್ಥಿರಾಂಕ AstCoverCrossBin* binp, std::ಗಣ<std::string>& ಹೆಸರುಗಳು) {
        ಆದರೆ (ಹೆಸರುಗಳು.emplace(binp->name()).ಎರಡನೇ) ಹಿಂತಿರುಗಿ ನಿಜ;
        binp->v3error("Duplicate cross bin " << binp->prettyNameQ()
                                             << " (IEEE 1800-2023 19.6.1).");
        ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
    }

    // The span of the implicit automatic bin reported as 'name' ('auto_<i>', see
    // createImplicitAutoBins), found without naming each of its bins
    ಸ್ಥಿರ ತಾರ್ಕಿಕ implicit_Auto_Bin_ಹರವು(ಸ್ಥಿರಾಂಕ CoverpointBins& bins, ಸ್ಥಿರಾಂಕ std::string& name,
                                    Bin_ಹರವು& ಹರವು) {
        ಸ್ಥಿರಾಂಕ std::string ಪೂರ್ವಪ್ರತ್ಯಯ = "auto_";
        ಆದರೆ (!VString::startsWith(name, ಪೂರ್ವಪ್ರತ್ಯಯ)) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
        ಸ್ಥಿರಾಂಕ std::string digits = name.substr(ಪೂರ್ವಪ್ರತ್ಯಯ.ಗಾತ್ರ_2());
        ಸ್ಥಿರಾಂಕ ಚಿಹ್ನೆರಹಿತ ದೀರ್ಘ ಸೂಚ್ಯಂಕ = std::strtoul(digits.c_str(), ಶೂನ್ಯಸೂಚಕ, 10);
        // Only the reported spelling names the bin, not e.g. 'auto_01' or 'auto_x'
        ಆದರೆ (ಸೂಚ್ಯಂಕ >= bins.implicitAuto.ಎಣಿಕೆ_2 || digits != cvt_ಗಮ್ಯ_Str(ಸೂಚ್ಯಂಕ)) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
        ಸ್ಥಿರಾಂಕ uint32_t ಸ್ಥಾನಾಂತರ = static_cast<uint32_t>(ಸೂಚ್ಯಂಕ);
        ಹರವು = Bin_ಹರವು{bins.implicitAuto.ಮೊದಲ + ಸ್ಥಾನಾಂತರ, 1, bins.implicitAuto.declared + ಸ್ಥಾನಾಂತರ};
        ಹಿಂತಿರುಗಿ ನಿಜ;
    }

    Cross_Binsof_ಗುರಿ
    resolve_Binsof_ಗುರಿ(ಸ್ಥಿರಾಂಕ AstCoverBinsof* selectp, ಸ್ಥಿರಾಂಕ AstCoverCross* crossp,
                        ಸ್ಥಿರಾಂಕ std::ಸದಿಶ<AstVar*>& cpVars,
                        ಸ್ಥಿರಾಂಕ std::map<std::string, uint32_t>& dimensions) ಸ್ಥಿರಾಂಕ {
        ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ dim = dimensions.ಕಂಡುಹಿಡಿ(selectp->pointp()->name());
        ಆದರೆ (dim == dimensions.ಕೊನೆ()) {
            selectp->v3error("binsof coverpoint "
                             << selectp->pointp()->prettyNameQ() << " is not an item of cross "
                             << crossp->prettyNameQ() << " (IEEE 1800-2023 19.6.1).");
            ಹಿಂತಿರುಗಿ {};
        }
        ಸ್ಥಿರಾಂಕ CoverpointBins& bins = m_cpBins.at(cpVars[dim->ಎರಡನೇ]);
        Cross_Binsof_ಗುರಿ ಗುರಿ{&bins, dim->ಎರಡನೇ, 0, bins.ಒಟ್ಟು};
        ಆದರೆ (!selectp->name().ಖಾಲಿಗಳು()) {
            ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ bin = bins.spans.ಕಂಡುಹಿಡಿ(selectp->name());
            Bin_ಹರವು ಹರವು{0, 0, 0};
            ಆದರೆ (bin != bins.spans.ಕೊನೆ()) {
                ಹರವು = bin->ಎರಡನೇ;
            } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (!implicit_Auto_Bin_ಹರವು(bins, selectp->name(), ಹರವು)) {
                selectp->v3error("Cannot find bin " << selectp->prettyNameQ() << " in coverpoint "
                                                    << selectp->pointp()->prettyNameQ()
                                                    << " (IEEE 1800-2023 19.6.1).");
                ಹಿಂತಿರುಗಿ {};
            }
            ಗುರಿ.m_ಮೊದಲ = ಹರವು.ಮೊದಲ;
            ಗುರಿ.m_ಎಣಿಕೆ = ಹರವು.ಎಣಿಕೆ_2;
            ಗುರಿ.m_declared_ಮೊದಲ = ಹರವು.declared;
            ಗುರಿ.m_declared_ಕೊನೆ = ಹರವು.declared + ಹರವು.ಎಣಿಕೆ_2;
            ಗುರಿ.m_sized = ಹರವು.sized;
        }
        ಹಿಂತಿರುಗಿ ಗುರಿ;
    }

    // The variables a 'with' filter reads, each by its first reference
    ಬಳಸಿ CrossFilterReads = std::map<ಸ್ಥಿರಾಂಕ AstVar*, AstVarRef*>;

    // Whether a 'with' filter has a count of value tuples, of 'matches' other than '$'
    ಸ್ಥಿರ ತಾರ್ಕಿಕ counts_ಮೌಲ್ಯ_Tuples(ಸ್ಥಿರಾಂಕ AstCoverCrossWith* withp) {
        ಹಿಂತಿರುಗಿ withp->countp() && !VN_IS(withp->countp(), Unbounded);
    }

    // Whether the runtime knows the values of the cross items a 'with' filter reads, or to count
    // value tuples, of all items, of integral coverpoints; else warns
    ಸ್ಥಿರ ತಾರ್ಕಿಕ crossFilterSupported(ಸ್ಥಿರಾಂಕ AstCoverCrossWith* withp,
                                     ಸ್ಥಿರಾಂಕ CrossFilterReads& reads) {
        ಗಾಗಿ (ಸ್ಥಿರಾಂಕ AstVar* varp = withp->varsp(); varp; varp = VN_AS(varp->nextp(), Var)) {
            ಆದರೆ (varp->dtypep()->skipRefp()->isIntegralOrPacked()) ಮುಂದುವರಿ;
            ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ it = reads.ಕಂಡುಹಿಡಿ(varp);
            ಆದರೆ (it != reads.ಕೊನೆ()) {
                it->ಎರಡನೇ->v3warn(COVERIGN, "Unsupported: cross bin 'with' filter reading the "
                                             "value of non-integral coverpoint "
                                                 << it->ಎರಡನೇ->prettyNameQ());
                ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
            }
            ಆದರೆ (counts_ಮೌಲ್ಯ_Tuples(withp)) {
                withp->countp()->v3warn(COVERIGN, "Unsupported: cross bin 'matches' count of "
                                                  "value tuples of non-integral coverpoint "
                                                      << varp->prettyNameQ());
                ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
            }
        }
        ಹಿಂತಿರುಗಿ ನಿಜ;
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
    ಶೂನ್ಯ generateCrossFilter(ಸ್ಥಿರಾಂಕ AstCoverCrossWith* withp, ಸ್ಥಿರಾಂಕ CrossFilterReads& reads,
                             ಸ್ಥಿರಾಂಕ AstCoverCrossBin* binp, ಸ್ಥಿರಾಂಕ AstCoverCross* crossp,
                             AstVar* cxp, ಸ್ಥಿರಾಂಕ std::map<string, uint32_t>& dimensions,
                             uint32_t ವಸ್ತುಗಳು) {
        File_ಸಾಲು* ಸ್ಥಿರಾಂಕ fl = withp->fileline();
        ಸ್ಥಿರಾಂಕ string ಪೂರ್ವಪ್ರತ್ಯಯ
            = m_sized_ಹೆಸರುಗಳು.get(sanitizeGeneratedName(crossp->name() + "__" + binp->name()));
        ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ counting = counts_ಮೌಲ್ಯ_Tuples(withp);
        // The count, 0 for all
        ಸ್ಥಿರಾಂಕ std::pair<Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ*, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ*> ಎಣಿಕೆ_2
            = counting ? new_ಎಣಿಕೆ_ವಾದಗಳು(fl, ಪೂರ್ವಪ್ರತ್ಯಯ + "_count", withp->countp())
                       : new_ಎಣಿಕೆ_ವಾದಗಳು(fl, withp->countp() ? 0 : 1);
        AstLoop* ಸ್ಥಿರಾಂಕ loopp = ಹೊಸ AstLoop{fl};
        AstCMethodHard* ಸ್ಥಿರಾಂಕ nextp = ಅಂಶ_Call(fl, cxp, VCMethod::COVERGROUP_WITH_ಮುಂದಿನ);
        nextp->dtype_ಗಣ_ಬಿಟ್();
        loopp->ಕೂಡಿಸು_Stmtsp(ಹೊಸ AstLoopTest{fl, loopp, nextp});
        string dims;
        ಗಾಗಿ (uint32_t dim = 0; counting && dim < ವಸ್ತುಗಳು; ++dim) {
            dims += (dims.ಖಾಲಿಗಳು() ? "" : ", ") + cvt_ಗಮ್ಯ_Str(dim) + "U";
        }
        ಆಗ (AstVar* ಸ್ಥಿರಾಂಕ varp = withp->varsp()) {
            varp->unlink_Fr_ಹಿಂಭಾಗ();
            ಆದರೆ (!reads.ಎಣಿಕೆ_2(varp)) {
                VL_DO_DANGLING(ತಳ್ಳು_Deletep(varp), varp);
                ಮುಂದುವರಿ;
            }
            ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ dimIt = dimensions.ಕಂಡುಹಿಡಿ(varp->name());
            UASSERT_OBJ(dimIt != dimensions.ಕೊನೆ(), varp, "Cross 'with' candidate not of an item");
            ಸ್ಥಿರಾಂಕ uint32_t dim = dimIt->ಎರಡನೇ;
            ಆದರೆ (!counting) dims += (dims.ಖಾಲಿಗಳು() ? "" : ", ") + cvt_ಗಮ್ಯ_Str(dim) + "U";
            varp->name(ಪೂರ್ವಪ್ರತ್ಯಯ + "_" + sanitizeGeneratedName(varp->name()));
            m_constructorp->ಕೂಡಿಸು_Stmtsp(varp);
            ಆದರೆ (varp->isWide()) {
                loopp->ಕೂಡಿಸು_Stmtsp(ಅಂಶ_Call(fl, cxp, VCMethod::COVERGROUP_CROSS_WITH_ಮೌಲ್ಯ_W,
                                          {cnum(fl, dim), ಹೊಸ AstVarRef{fl, varp, VAccess::WRITE}})
                                     ->makeStmt());
            } ಇಲ್ಲದಿದ್ದರೆ {
                AstCMethodHard* ಸ್ಥಿರಾಂಕ valuep
                    = ಅಂಶ_Call(fl, cxp, VCMethod::COVERGROUP_CROSS_WITH_ಮೌಲ್ಯ, {cnum(fl, dim)});
                valuep->dtype_ಗಣ_UInt64();
                Ast_ಗಂಟು_DType* ಸ್ಥಿರಾಂಕ dtypep = varp->ಕಂಡುಹಿಡಿ_Logic_DType(
                    varp->ಅಗಲ(), varp->ಅಗಲ(),
                    varp->isSigned() ? VSigning::SIGNED : VSigning::UNSIGNED);
                loopp->ಕೂಡಿಸು_Stmtsp(ಹೊಸ Ast_ನಿಯೋಜಿಸು{fl, ಹೊಸ AstVarRef{fl, varp, VAccess::WRITE},
                                               resize_ಮೌಲ್ಯ(valuep, dtypep)});
            }
        }
        loopp->ಕೂಡಿಸು_Stmtsp(ಅಂಶ_Call(fl, cxp, VCMethod::COVERGROUP_CROSS_WITH_TEST,
                                  {withp->filterp()->unlink_Fr_ಹಿಂಭಾಗ()})
                             ->makeStmt());
        ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ prot = v3Global.opt.protectIds();
        m_constructorp->ಕೂಡಿಸು_Stmtsp(
            ಅಂಶ_Call(fl, cxp, VCMethod::COVERGROUP_CROSS_WITH_BEGIN,
                     {ctext(fl, "{" + dims + "}"), ಎಣಿಕೆ_2.ಮೊದಲ, ಎಣಿಕೆ_2.ಎರಡನೇ,
                      ctext(fl, quoted(VIdProtect::protect_ಪದಗಳು_If(binp->name(), prot))),
                      ctext(fl, quoted(VIdProtect::protectIf(fl->filename(), prot))),
                      cnum(fl, static_cast<uint32_t>(fl->lineno()))})
                ->makeStmt());
        m_constructorp->ಕೂಡಿಸು_Stmtsp(loopp);
        m_constructorp->ಕೂಡಿಸು_Stmtsp(
            ಅಂಶ_Call(fl, cxp, VCMethod::COVERGROUP_CROSS_WITH_ಕೊನೆ)->makeStmt());
    }

    ತಾರ್ಕಿಕ generateRuntimeSelection(Ast_ಗಂಟು* nodep, ಸ್ಥಿರಾಂಕ AstCoverCrossBin* binp,
                                  AstCoverCross* crossp, AstVar* cxp,
                                  ಸ್ಥಿರಾಂಕ std::ಸದಿಶ<AstVar*>& cpVars,
                                  ಸ್ಥಿರಾಂಕ std::map<string, uint32_t>& dimensions) {
        File_ಸಾಲು* ಸ್ಥಿರಾಂಕ fl = nodep->fileline();
        ಆದರೆ (ಸ್ಥಿರಾಂಕ AstCoverCrossRef* ಸ್ಥಿರಾಂಕ refp = VN_CAST(nodep, CoverCrossRef)) {
            ಆದರೆ (!checkCrossRef(refp, crossp)) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
            m_constructorp->ಕೂಡಿಸು_Stmtsp(
                ಅಂಶ_Call(fl, cxp, VCMethod::COVERGROUP_SELECT_ALL)->makeStmt());
            ಹಿಂತಿರುಗಿ ನಿಜ;
        }
        ಆದರೆ (ಸ್ಥಿರಾಂಕ AstCoverCrossSelect* ಸ್ಥಿರಾಂಕ opp = VN_CAST(nodep, CoverCrossSelect)) {
            ಆದರೆ (!generateRuntimeSelection(opp->lhsp(), binp, crossp, cxp, cpVars, dimensions)
                || !generateRuntimeSelection(opp->rhsp(), binp, crossp, cxp, cpVars, dimensions)) {
                ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
            }
            m_constructorp->ಕೂಡಿಸು_Stmtsp(ಅಂಶ_Call(fl, cxp,
                                               opp->isOr() ? VCMethod::COVERGROUP_SELECT_OR
                                                           : VCMethod::COVERGROUP_SELECT_AND)
                                          ->makeStmt());
            ಹಿಂತಿರುಗಿ ನಿಜ;
        }
        ಆದರೆ (ಸ್ಥಿರಾಂಕ AstCoverCrossWith* ಸ್ಥಿರಾಂಕ withp = VN_CAST(nodep, CoverCrossWith)) {
            CrossFilterReads reads;
            withp->filterp()->foreach([&](AstVarRef* refp) { reads.emplace(refp->varp(), refp); });
            // The count of 'matches' is a positive integer, or '$' (IEEE 1800-2023 19.6.1.2)
            ಆದರೆ ((counts_ಮೌಲ್ಯ_Tuples(withp)
                 && !check_ಎಣಿಕೆ(withp->countp(), "Cross bin 'matches' count", "19.6.1.2"))
                || !crossFilterSupported(withp, reads)
                || !generateRuntimeSelection(withp->selectp(), binp, crossp, cxp, cpVars,
                                             dimensions)) {
                ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
            }
            generateCrossFilter(withp, reads, binp, crossp, cxp, dimensions,
                                static_cast<uint32_t>(cpVars.ಗಾತ್ರ_2()));
            ಹಿಂತಿರುಗಿ ನಿಜ;
        }
        AstCoverBinsof* ಸ್ಥಿರಾಂಕ selectp = VN_AS(nodep, CoverBinsof);
        ಸ್ಥಿರಾಂಕ Cross_Binsof_ಗುರಿ ಗುರಿ = resolve_Binsof_ಗುರಿ(selectp, crossp, cpVars, dimensions);
        ಆದರೆ (!ಗುರಿ.m_binsp) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
        ಸ್ಥಿರಾಂಕ CoverpointBins& bins = *ಗುರಿ.m_binsp;
        ಆದರೆ (selectp->rangesp() && !bins.exprp->dtypep()->skipRefp()->isIntegralOrPacked()) {
            ತಾರ್ಕಿಕ valid = ನಿಜ;
            unsupportedCrossRange(selectp, valid);
            ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
        }
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* firstp;
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* endp;
        ಆದರೆ (ಗುರಿ.m_sized < 0) {
            firstp = cnum(fl, ಗುರಿ.m_declared_ಮೊದಲ);
            endp = cnum(fl, ಗುರಿ.m_declared_ಕೊನೆ);
        } ಇಲ್ಲದಿದ್ದರೆ {  // A sized array, whose bins the coverpoint's construction placed
            AstVar* ಸ್ಥಿರಾಂಕ cpVarp = cpVars[ಗುರಿ.m_dimension];
            ಸ್ಥಿರಾಂಕ uint32_t sized = static_cast<uint32_t>(ಗುರಿ.m_sized);
            firstp = ಅಂಶ_Call(fl, cpVarp, VCMethod::COVERGROUP_SIZED_ಮೊದಲ, {cnum(fl, sized)});
            endp = ಅಂಶ_Call(fl, cpVarp, VCMethod::COVERGROUP_SIZED_ಕೊನೆ, {cnum(fl, sized)});
            firstp->dtype_ಗಣ_UInt32();
            endp->dtype_ಗಣ_UInt32();
        }
        m_constructorp->ಕೂಡಿಸು_Stmtsp(
            ಅಂಶ_Call(fl, cxp, VCMethod::COVERGROUP_SELECT_DIM,
                     {cnum(fl, ಗುರಿ.m_dimension), firstp, endp, cnum(fl, selectp->isNegated()),
                      cnum(fl, selectp->rangesp() != ಶೂನ್ಯಸೂಚಕ)})
                ->makeStmt());
        ಗಾಗಿ (Ast_ಗಂಟು* rangep = selectp->rangesp(); rangep; rangep = rangep->nextp()) {
            Cross_ಮೌಲ್ಯ_Range range{rangep, resolve_ಅಗಲ(rangep, bins.exprp)};
            ಆದರೆ (!resolve_ಮೌಲ್ಯ(rangep, bins.exprp, ಸುಳ್ಳು, ಸುಳ್ಳು, range)) {
                ತಾರ್ಕಿಕ valid = ನಿಜ;
                unsupportedCrossRange(selectp, valid);
                ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
            }
            ಆದರೆ (cross_Range_ಖಾಲಿಗಳು(range)) ಮುಂದುವರಿ;
            m_constructorp->ಕೂಡಿಸು_Stmtsp(ಅಂಶ_Call(fl, cxp,
                                               bins.exprp->isWide()
                                                   ? VCMethod::COVERGROUP_SELECT_RANGE_W
                                                   : VCMethod::COVERGROUP_SELECT_RANGE,
                                               {new_ಮೌಲ್ಯ_Const(fl, range.lo, bins.exprp),
                                                new_ಮೌಲ್ಯ_Const(fl, range.hi, bins.exprp)})
                                          ->makeStmt());
        }
        m_constructorp->ಕೂಡಿಸು_Stmtsp(
            ಅಂಶ_Call(fl, cxp, VCMethod::COVERGROUP_SELECT_DIM_ಕೊನೆ)->makeStmt());
        ಹಿಂತಿರುಗಿ ನಿಜ;
    }

    std::ಸದಿಶ<AstCoverCrossBin*>
    generateRuntimeCrossBins(AstCoverCross* crossp, AstVar* cxp,
                             ಸ್ಥಿರಾಂಕ std::ಸದಿಶ<AstVar*>& cpVars,
                             ಸ್ಥಿರಾಂಕ std::map<string, uint32_t>& dimensions) {
        std::ಸದಿಶ<AstCoverCrossBin*> bins;
        std::ಗಣ<string> ಹೆಸರುಗಳು;
        ಗಾಗಿ (Ast_ಗಂಟು* itemp = crossp->binsp(); itemp; itemp = itemp->nextp()) {
            AstCoverCrossBin* ಸ್ಥಿರಾಂಕ binp = VN_AS(itemp, CoverCrossBin);
            ಆದರೆ (!checkCrossBinName(binp, ಹೆಸರುಗಳು)) ಮುಂದುವರಿ;
            File_ಸಾಲು* ಸ್ಥಿರಾಂಕ fl = binp->fileline();
            // Generate the selection apart, so that an unsupported one leaves no code, which
            // could leave its state for the next bin
            AstFunc* ಸ್ಥಿರಾಂಕ selectionp = ಹೊಸ AstFunc{fl, "", ಶೂನ್ಯಸೂಚಕ, ಶೂನ್ಯಸೂಚಕ};
            ತಾರ್ಕಿಕ supported;
            {
                VL_RESTORER(m_constructorp);
                m_constructorp = selectionp;
                supported = generateRuntimeSelection(binp->selectp(), binp, crossp, cxp, cpVars,
                                                     dimensions);
            }
            // A supported selection has code
            ಆದರೆ (supported) m_constructorp->ಕೂಡಿಸು_Stmtsp(selectionp->stmtsp()->unlink_Fr_ಹಿಂಭಾಗ_With_ಮುಂದಿನ());
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(selectionp), selectionp);
            ಆದರೆ (!supported) ಮುಂದುವರಿ;
            ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ protect = v3Global.opt.protectIds();
            m_constructorp->ಕೂಡಿಸು_Stmtsp(
                ಅಂಶ_Call(fl, cxp, VCMethod::COVERGROUP_SELECT_BIN,
                         {ctext(fl, binp->binsType().bin_ಗಣ_Enum()),
                          ctext(fl, quoted(VIdProtect::protect_ಪದಗಳು_If(binp->name(), protect))),
                          ctext(fl, quoted(VIdProtect::protectIf(fl->filename(), protect))),
                          cnum(fl, fl->lineno()), cnum(fl, fl->ಮೊದಲ_ಸ್ತಂಭ()),
                          cnum(fl, static_cast<uint32_t>(bins.ಗಾತ್ರ_2()))})
                    ->makeStmt());
            bins.ತಳ್ಳು_ಹಿಂಭಾಗ(binp);
        }
        m_constructorp->ಕೂಡಿಸು_Stmtsp(
            ಅಂಶ_Call(crossp->fileline(), cxp, VCMethod::COVERGROUP_FINALIZE_BINS)->makeStmt());
        ಹಿಂತಿರುಗಿ bins;
    }

    ಸ್ಥಿರ ಶೂನ್ಯ ಗಣ_Cross_Selection_Range(CrossSelection& selection, uint64_t ಮೊದಲ, uint64_t ಕೊನೆ) {
        ಆಗ (ಮೊದಲ < ಕೊನೆ) {
            ಸ್ಥಿರಾಂಕ ಚಿಹ್ನೆರಹಿತ ಬಿಟ್_2 = ಮೊದಲ % 64;
            ಸ್ಥಿರಾಂಕ ಚಿಹ್ನೆರಹಿತ ಬಿಟ್ಟುಗಳು = std::min<uint64_t>(64 - ಬಿಟ್_2, ಕೊನೆ - ಮೊದಲ);
            selection[VL_BITWORD_Q(ಮೊದಲ)]
                |= (ಬಿಟ್ಟುಗಳು == 64 ? ~uint64_t{0} : (uint64_t{1} << ಬಿಟ್ಟುಗಳು) - 1) << ಬಿಟ್_2;
            ಮೊದಲ += ಬಿಟ್ಟುಗಳು;
        }
    }

    CrossSelection crossSelection(Ast_ಗಂಟು* nodep, CrossSelectionContext& ctx) {
        ಆದರೆ (ಸ್ಥಿರಾಂಕ AstCoverCrossRef* ಸ್ಥಿರಾಂಕ refp = VN_CAST(nodep, CoverCrossRef)) {
            ಆದರೆ (!checkCrossRef(refp, ctx.crossp)) {
                ctx.valid = ಸುಳ್ಳು;
                ಹಿಂತಿರುಗಿ {};
            }
            CrossSelection ಫಲಿತಾಂಶ(
                VL_BITWORD_Q(static_cast<uint64_t>(ctx.tuples) + VL_QUADSIZE - 1), 0);
            ಗಣ_Cross_Selection_Range(ಫಲಿತಾಂಶ, 0, ctx.tuples);
            ಹಿಂತಿರುಗಿ ಫಲಿತಾಂಶ;
        }
        ಆದರೆ (AstCoverCrossSelect* ಸ್ಥಿರಾಂಕ opp = VN_CAST(nodep, CoverCrossSelect)) {
            CrossSelection lhs = crossSelection(opp->lhsp(), ctx);
            ಸ್ಥಿರಾಂಕ CrossSelection rhs = crossSelection(opp->rhsp(), ctx);
            ಆದರೆ (!ctx.valid) ಹಿಂತಿರುಗಿ {};
            ಗಾಗಿ (ಗಾತ್ರ_t i = 0; i < lhs.ಗಾತ್ರ_2(); ++i) {
                lhs[i] = opp->isOr() ? lhs[i] | rhs[i] : lhs[i] & rhs[i];
            }
            ಹಿಂತಿರುಗಿ lhs;
        }
        AstCoverBinsof* ಸ್ಥಿರಾಂಕ selectp = VN_AS(nodep, CoverBinsof);
        ಸ್ಥಿರಾಂಕ Cross_Binsof_ಗುರಿ ಗುರಿ
            = resolve_Binsof_ಗುರಿ(selectp, ctx.crossp, ctx.cpVars, ctx.dimensions);
        ಆದರೆ (!ಗುರಿ.m_binsp) {
            ctx.valid = ಸುಳ್ಳು;
            ಹಿಂತಿರುಗಿ {};
        }
        ಸ್ಥಿರಾಂಕ CoverpointBins& bins = *ಗುರಿ.m_binsp;
        // Coverpoints with sized arrays are runtime points, so feed only runtime crosses
        UASSERT_OBJ(ಗುರಿ.m_sized < 0, selectp, "Sized bin array selected by a static cross");
        ಸ್ಥಿರಾಂಕ std::ಸದಿಶ<ತಾರ್ಕಿಕ> selected
            = selectCoverpointBins(selectp, bins, ಗುರಿ.m_ಮೊದಲ, ಗುರಿ.m_ಎಣಿಕೆ, ctx.valid);
        ಆದರೆ (!ctx.valid) ಹಿಂತಿರುಗಿ {};
        CrossSelection ಫಲಿತಾಂಶ(VL_BITWORD_Q(static_cast<uint64_t>(ctx.tuples) + VL_QUADSIZE - 1),
                              0);
        ಸ್ಥಿರಾಂಕ uint64_t stride = ctx.strides[ಗುರಿ.m_dimension];
        ಸ್ಥಿರಾಂಕ uint64_t period = stride * bins.ಒಟ್ಟು;
        ಗಾಗಿ (uint64_t ತಳ = 0; ತಳ < ctx.tuples; ತಳ += period) {
            ಗಾಗಿ (uint32_t i = 0; i < bins.ಒಟ್ಟು;) {
                ಆದರೆ (!selected[i]) {
                    ++i;
                    ಮುಂದುವರಿ;
                }
                ಸ್ಥಿರಾಂಕ uint32_t begin = i++;
                ಆಗ (i < bins.ಒಟ್ಟು && selected[i]) ++i;
                ಗಣ_Cross_Selection_Range(ಫಲಿತಾಂಶ, ತಳ + begin * stride, ತಳ + i * stride);
            }
        }
        ಹಿಂತಿರುಗಿ ಫಲಿತಾಂಶ;
    }

    // Size the Cartesian product of the declared Normal bins, with each dimension's flat-index
    // stride.  Live runtime bins only shrink it.  Warns and returns false if it is too large.
    ತಾರ್ಕಿಕ crossShape(AstCoverCross* crossp, ಸ್ಥಿರಾಂಕ std::ಸದಿಶ<AstVar*>& cpVars,
                    std::ಸದಿಶ<uint32_t>& strides, uint32_t& tuples) ಸ್ಥಿರಾಂಕ {
        uint64_t ಗುಣಲಬ್ಧ = std::any_ದ(cpVars.begin(), cpVars.ಕೊನೆ(),
                                       [ಇದು](AstVar* varp) { ಹಿಂತಿರುಗಿ !m_cpBins.at(varp).ಒಟ್ಟು; })
                               ? 0
                               : 1;
        strides.resize(cpVars.ಗಾತ್ರ_2());
        ಗಾಗಿ (ಗಾತ್ರ_t d = cpVars.ಗಾತ್ರ_2(); d > 0; --d) {
            strides[d - 1] = static_cast<uint32_t>(ಗುಣಲಬ್ಧ);
            ಗುಣಲಬ್ಧ *= m_cpBins.at(cpVars[d - 1]).ಒಟ್ಟು;
            ಆದರೆ (ಗುಣಲಬ್ಧ > UINT32_MAX) {
                crossp->v3warn(COVERIGN,
                               "Unsupported: cross coverage with more than 2^32-1 tuples.");
                ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
            }
        }
        tuples = static_cast<uint32_t>(ಗುಣಲಬ್ಧ);
        ಹಿಂತಿರುಗಿ ನಿಜ;
    }

    CrossLayout resolveCrossLayout(AstCoverCross* crossp, ಸ್ಥಿರಾಂಕ std::ಸದಿಶ<AstVar*>& cpVars,
                                   ಸ್ಥಿರಾಂಕ std::map<std::string, uint32_t>& dimensions) {
        CrossLayout layout;
        CrossSelectionContext ctx{crossp, cpVars, dimensions, 0, {}};
        ಆದರೆ (!crossShape(crossp, cpVars, ctx.strides, ctx.tuples)) {
            layout.valid = ಸುಳ್ಳು;
            ಹಿಂತಿರುಗಿ layout;
        }
        layout.tuples = ctx.tuples;
        CrossSelection occupied;
        CrossSelection excluded;
        std::ಗಣ<std::string> ಹೆಸರುಗಳು;
        ಗಾಗಿ (Ast_ಗಂಟು* itemp = crossp->binsp(); itemp; itemp = itemp->nextp()) {
            AstCoverCrossBin* ಸ್ಥಿರಾಂಕ binp = VN_AS(itemp, CoverCrossBin);
            ಆದರೆ (!checkCrossBinName(binp, ಹೆಸರುಗಳು)) ಮುಂದುವರಿ;
            ctx.valid = ನಿಜ;
            CrossSelection selection = crossSelection(binp->selectp(), ctx);
            ಆದರೆ (!ctx.valid || std::all_ದ(selection.begin(), selection.ಕೊನೆ(), [](uint64_t ಶಬ್ದ) {
                    ಹಿಂತಿರುಗಿ ಶಬ್ದ == 0;
                })) {
                ಮುಂದುವರಿ;
            }
            ಆದರೆ (occupied.ಖಾಲಿಗಳು()) {
                occupied.resize(selection.ಗಾತ್ರ_2(), 0);
                excluded.resize(selection.ಗಾತ್ರ_2(), 0);
            }
            ಗಾಗಿ (ಗಾತ್ರ_t i = 0; i < selection.ಗಾತ್ರ_2(); ++i) {
                occupied[i] |= selection[i];
                ಆದರೆ (!binp->binsType().bin_Is_ಸಾಮಾನ್ಯ()) excluded[i] |= selection[i];
            }
            layout.bins.ತಳ್ಳು_ಹಿಂಭಾಗ({binp, std::move(selection)});
        }
        ಆದರೆ (!layout.bins.ಖಾಲಿಗಳು()) {
            // IEEE 1800-2023 19.6.2/19.6.3: exclusions also remove tuples from named
            // bins, independently of declaration order and sampling guards.
            ಗಾಗಿ (ResolvedCrossBin& resolved : layout.bins) {
                ಗಾಗಿ (ಗಾತ್ರ_t i = 0; i < resolved.selection.ಗಾತ್ರ_2(); ++i) {
                    ಆದರೆ (resolved.binp->binsType().bin_Is_ಸಾಮಾನ್ಯ()) {
                        resolved.selection[i] &= ~excluded[i];
                    }
                    ಆದರೆ (resolved.selection[i]) ++layout.bin_ಪದಗಳು;
                }
            }
            layout.bins.erase(std::remove_if(layout.bins.begin(), layout.bins.ಕೊನೆ(),
                                             [](ಸ್ಥಿರಾಂಕ ResolvedCrossBin& resolved) {
                                                 ಹಿಂತಿರುಗಿ std::all_ದ(
                                                     resolved.selection.begin(),
                                                     resolved.selection.ಕೊನೆ(),
                                                     [](uint64_t ಶಬ್ದ) { ಹಿಂತಿರುಗಿ ಶಬ್ದ == 0; });
                                             }),
                              layout.bins.ಕೊನೆ());
            layout.autoBins = layout.tuples;
            ಗಾಗಿ (ಸ್ಥಿರಾಂಕ uint64_t ಶಬ್ದ : occupied) {
                layout.autoBins -= static_cast<uint32_t>(std::bitset<VL_QUADSIZE>{ಶಬ್ದ}.ಎಣಿಕೆ_2());
            }
        }
        ಹಿಂತಿರುಗಿ layout;
    }

    AstCoverCrossDType* crossDType(File_ಸಾಲು* fl, uint32_t dimensions, ಸ್ಥಿರಾಂಕ CrossLayout& layout,
                                   ತಾರ್ಕಿಕ dynamic = ಸುಳ್ಳು) {
        ಸ್ಥಿರಾಂಕ uint32_t bins = static_cast<uint32_t>(layout.bins.ಗಾತ್ರ_2());
        ಸ್ಥಿರಾಂಕ CrossShape shape{dimensions,      layout.tuples,   bins,
                               layout.autoBins, layout.bin_ಪದಗಳು, dynamic};
        AstCoverCrossDType*& typep = m_cxDTypes[shape];
        ಆದರೆ (!typep) {
            typep = ಹೊಸ AstCoverCrossDType{
                fl, dimensions, layout.tuples, bins, layout.autoBins, layout.bin_ಪದಗಳು, dynamic};
            v3Global.rootp()->typeTablep()->ಕೂಡಿಸು_Typesp(typep);
        }
        ಹಿಂತಿರುಗಿ typep;
    }

    std::ಸದಿಶ<AstCoverCrossBin*> generateCrossBins(AstCoverCross* crossp, AstVar* cxVarp,
                                                     ಸ್ಥಿರಾಂಕ CrossLayout& layout) {
        std::ಸದಿಶ<AstCoverCrossBin*> bins;
        ಗಾಗಿ (ಸ್ಥಿರಾಂಕ ResolvedCrossBin& resolved : layout.bins) {
            AstCoverCrossBin* ಸ್ಥಿರಾಂಕ binp = resolved.binp;
            ಸ್ಥಿರಾಂಕ CrossSelection& selection = resolved.selection;
            File_ಸಾಲು* ಸ್ಥಿರಾಂಕ fl = binp->fileline();
            ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ prot = v3Global.opt.protectIds();
            std::string ಮುಖವಾಡ = "{";
            ಗಾಗಿ (ಗಾತ್ರ_t i = 0; i < selection.ಗಾತ್ರ_2(); ++i) {
                ಆದರೆ (i) ಮುಖವಾಡ += ", ";
                ಮುಖವಾಡ += std::ಗಮ್ಯ_string(selection[i]) + "ULL";
            }
            ಮುಖವಾಡ += "}";
            m_constructorp->ಕೂಡಿಸು_Stmtsp(
                ಅಂಶ_Call(fl, cxVarp, VCMethod::COVERGROUP_ಕೂಡಿಸು_BIN,
                         {ctext(fl, binp->binsType().bin_ಗಣ_Enum()), ctext(fl, ಮುಖವಾಡ),
                          ctext(fl, quoted(VIdProtect::protect_ಪದಗಳು_If(binp->name(), prot))),
                          ctext(fl, quoted(VIdProtect::protectIf(fl->filename(), prot))),
                          cnum(fl, static_cast<uint32_t>(fl->lineno())),
                          cnum(fl, static_cast<uint32_t>(fl->ಮೊದಲ_ಸ್ತಂಭ()))})
                    ->makeStmt());
            bins.ತಳ್ಳು_ಹಿಂಭಾಗ(binp);
        }
        ಆದರೆ (!bins.ಖಾಲಿಗಳು()) {
            m_constructorp->ಕೂಡಿಸು_Stmtsp(
                ಅಂಶ_Call(crossp->fileline(), cxVarp, VCMethod::COVERGROUP_FINALIZE_BINS)
                    ->makeStmt());
        }
        ಹಿಂತಿರುಗಿ bins;
    }

    // Route a cross through a VlCoverCross member: emit the member, its constructor init +
    // registration, and the sample() call.  The feeding coverpoints are already generated
    // (their hit lists drive the cross). Each explicit bin adds one configuration call.
    ಶೂನ್ಯ generateCross(AstCoverCross* crossp) {
        File_ಸಾಲು* ಸ್ಥಿರಾಂಕ fl = crossp->fileline();
        ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ dynamic = m_runtimeCrosses.ಎಣಿಕೆ_2(crossp);
        UINFO(4, "  Generating VlCoverCross member: " << crossp->name());

        ಆದರೆ (Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ iffp = crossp->iffp()) {
            crossp->iffp(
                capture_Iff_ಗಮ್ಯ_ತಾತ್ಕಾಲಿಕ(iffp, "__VcrossIff_" + sanitizeGeneratedName(crossp->name())));
        }

        // Resolve and unlink the coverpoint refs, in dimension order.  Every ref resolves to a
        // known coverpoint (a cross with an unresolvable item was dropped earlier).
        std::ಸದಿಶ<AstVar*> cpVars;
        std::map<std::string, uint32_t> dimensions;
        ಗಾಗಿ (Ast_ಗಂಟು* itemp = crossp->itemsp(); itemp;) {
            Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ nextp = itemp->nextp();
            AstCoverpointRef* ಸ್ಥಿರಾಂಕ refp = VN_AS(itemp, CoverpointRef);
            ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ it = m_cpVarMap.ಕಂಡುಹಿಡಿ(refp->name());
            UASSERT_OBJ(it != m_cpVarMap.ಕೊನೆ(), crossp, "Cross references an unknown coverpoint");
            dimensions.emplace(refp->name(), static_cast<uint32_t>(cpVars.ಗಾತ್ರ_2()));
            cpVars.ತಳ್ಳು_ಹಿಂಭಾಗ(it->ಎರಡನೇ);
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(refp->unlink_Fr_ಹಿಂಭಾಗ()), refp);
            itemp = nextp;
        }
        ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ dims = static_cast<ಪೂರ್ಣಾಂಕ>(cpVars.ಗಾತ್ರ_2());
        CrossLayout layout;
        ಆದರೆ (dynamic) {
            // The runtime sizes the layout from live bins; only check the declared bound here.
            std::ಸದಿಶ<uint32_t> strides;
            uint32_t tuples = 0;
            layout.valid = crossShape(crossp, cpVars, strides, tuples);
        } ಇಲ್ಲದಿದ್ದರೆ {
            layout = resolveCrossLayout(crossp, cpVars, dimensions);
        }
        ಆದರೆ (!layout.valid) ಹಿಂತಿರುಗಿ;

        AstVar* ಸ್ಥಿರಾಂಕ cxVarp
            = ಹೊಸ AstVar{fl, VVarType::MEMBER, "__Vcx_" + crossp->name(),
                         crossDType(fl, static_cast<uint32_t>(dims), layout, dynamic)};
        m_covergroupp->ಕೂಡಿಸು_Membersp(cxVarp);
        m_constructorp->ಕೂಡಿಸು_Stmtsp(make_ಅಂಶ_Create(fl, cxVarp,
                                                 dynamic ? VCMethod::COVERGROUP_ಕೂಡಿಸು_CROSS_DYN
                                                         : VCMethod::COVERGROUP_ಕೂಡಿಸು_CROSS));
        generate_ಅಂಶ_ತೂಕ(fl, cxVarp, crossp->optionsp());
        ಪೂರ್ಣಾಂಕ at_ಕನಿಷ್ಠ_ಮೌಲ್ಯ;
        ಪೂರ್ಣಾಂಕ autoBinMax;  // Not a cross option
        ಹೊರತೆಗೆ_ಅಂಶ_Options(crossp->optionsp(), at_ಕನಿಷ್ಠ_ಮೌಲ್ಯ, autoBinMax);

        // Constructor: init (after the coverpoints, which generate earlier) then registration.
        // Obfuscate the hierarchy/filename/page under --protect-ids as for coverpoints above.
        ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ prot = v3Global.opt.protectIds();
        ಸ್ಥಿರಾಂಕ std::string hier
            = VIdProtect::protect_ಪದಗಳು_If(m_covergroupName + "." + crossp->name(), prot);
        m_constructorp->ಕೂಡಿಸು_Stmtsp(makeCrossCpsCall(
            fl, cpVars,
            ಅಂಶ_Call(fl, cxVarp, VCMethod::COVERGROUP_INIT,
                     {ctext(fl, quoted(hier)), cnum(fl, static_cast<uint32_t>(at_ಕನಿಷ್ಠ_ಮೌಲ್ಯ)),
                      cnum(fl, static_cast<uint32_t>(dims)), ctext(fl, "__Vcx_cps"),
                      ctext(fl, quoted(VIdProtect::protectIf(fl->filename(), prot))),
                      cnum(fl, static_cast<uint32_t>(fl->lineno())),
                      cnum(fl, static_cast<uint32_t>(fl->ಮೊದಲ_ಸ್ತಂಭ()))})));
        ಸ್ಥಿರಾಂಕ std::ಸದಿಶ<AstCoverCrossBin*> bins
            = dynamic ? generateRuntimeCrossBins(crossp, cxVarp, cpVars, dimensions)
                      : generateCrossBins(crossp, cxVarp, layout);
        ಆದರೆ (v3Global.opt.coverage()) {
            ಸ್ಥಿರಾಂಕ std::string ಪುಟ
                = VIdProtect::protectIf("v_covergroup/" + m_covergroupName, prot);
            m_constructorp->ಕೂಡಿಸು_Stmtsp(ಅಂಶ_Call(fl, cxVarp, VCMethod::COVERGROUP_ರಿಜಿಸ್ಟರ್_BINS,
                                               {ctext(fl, "vlSymsp->_vm_contextp__->coveragep()"),
                                                ctext(fl, quoted(ಪುಟ)),
                                                cnum(fl, ಅಂಶ_Database_ತೂಕ(crossp->optionsp())),
                                                cnum(fl, m_cg_Type_ತೂಕ)})
                                          ->makeStmt());
        }

        // sample(): after all coverpoints have sampled (cross loop runs after coverpoint loop).
        UASSERT_OBJ(m_sampleFuncp, crossp, "sample() CFunc not set for cross");
        // The cross remembers its feeding coverpoints, so sample() needs no cps array;
        // per-bin iff guards still need a temporary array, hence the block form.
        ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ hasIffs
            = std::any_ದ(bins.begin(), bins.ಕೊನೆ(),
                          [](ಸ್ಥಿರಾಂಕ AstCoverCrossBin* binp) { ಹಿಂತಿರುಗಿ binp->iffp() != ಶೂನ್ಯಸೂಚಕ; });
        Ast_ಗಂಟು_Stmt* ಸ್ಥಿರಾಂಕ samplep
            = !hasIffs ? static_cast<Ast_ಗಂಟು_Stmt*>(
                             ಅಂಶ_Call(fl, cxVarp, VCMethod::COVERGROUP_SAMPLE)->makeStmt())
                       : static_cast<Ast_ಗಂಟು_Stmt*>(makeCrossIffsCall(
                             fl, bins,
                             ಅಂಶ_Call(fl, cxVarp, VCMethod::COVERGROUP_SAMPLE_IFFS,
                                      {ctext(fl, "__Vcx_iffs")})));
        ಆದರೆ (Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ iffp = crossp->iffp()) {
            m_sampleFuncp->ಕೂಡಿಸು_Stmtsp(ಹೊಸ AstIf{fl, iffp->clone_ವೃಕ್ಷ(ಸುಳ್ಳು), samplep});
        } ಇಲ್ಲದಿದ್ದರೆ {
            m_sampleFuncp->ಕೂಡಿಸು_Stmtsp(samplep);
        }
    }

    ಶೂನ್ಯ generate_Cross_ಸಂಕೇತ(AstCoverCross* crossp) {
        UINFO(4, "  Generating code for cross: " << crossp->name());

        // Non-standard hierarchical/dotted cross item (e.g. 'cross a.b'): an implicit coverpoint
        // over the referenced expression (carried in refp->exprp()).  The grammar already warned
        // NONSTD; implicit coverpoints are not yet implemented, so generate no sampling code for
        // this cross.  When support is added the implicit coverpoint should be synthesized
        // upstream (V3LinkParse) as a real AstCoverpoint so it flows through the normal coverpoint
        // path - by here coverpoint lowering has already run.
        ಗಾಗಿ (Ast_ಗಂಟು* itemp = crossp->itemsp(); itemp; itemp = itemp->nextp()) {
            ಸ್ಥಿರಾಂಕ AstCoverpointRef* ಸ್ಥಿರಾಂಕ refp = VN_AS(itemp, CoverpointRef);
            ಆದರೆ (refp->exprp()) {
                refp->v3warn(COVERIGN,
                             "Unsupported: cross of hierarchical reference (implicit coverpoint)");
                ಹಿಂತಿರುಗಿ;
            }
        }

        // A cross naming a bare variable (implicit coverpoint, which Verilator does not
        // synthesize) is dropped entirely with a COVERIGN warning -- it produces no coverage
        // either way -- but only this cross is dropped; its sibling crosses are still generated
        // and the real coverpoints it referenced remain as independent coverpoints.
        ಆದರೆ (m_droppedCrosses.ಎಣಿಕೆ_2(crossp)) {
            ಗಾಗಿ (Ast_ಗಂಟು* itemp = crossp->itemsp(); itemp; itemp = itemp->nextp()) {
                ಸ್ಥಿರಾಂಕ AstCoverpointRef* ಸ್ಥಿರಾಂಕ refp = VN_AS(itemp, CoverpointRef);
                ಆದರೆ (m_coverpointMap.ಕಂಡುಹಿಡಿ(refp->name()) == m_coverpointMap.ಕೊನೆ()) {
                    refp->v3warn(COVERIGN, "Unsupported: cross of "
                                               << refp->prettyNameQ()
                                               << " which is not a coverpoint (implicit "
                                                  "coverpoint)");
                    ನಿಲ್ಲಿಸು;
                }
            }
            ಹಿಂತಿರುಗಿ;
        }

        // Every cross that isn't dropped routes through a VlCoverCross member.
        generateCross(crossp);
    }

    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* buildBinCondition(AstCoverBin* binp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* exprp) {
        // Get the range list from the bin
        Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ rangep = binp->rangesp();
        ಆದರೆ (!rangep) ಹಿಂತಿರುಗಿ ಶೂನ್ಯಸೂಚಕ;

        // Integral values resolve to the coverpoint's type, as its runtime metadata does
        ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ integral = exprp->dtypep()->skipRefp()->isIntegralOrPacked();
        // No value form of a wildcard bin is allowed on a real coverpoint (IEEE 1800-2023 19.5.4)
        ಆದರೆ (binp->isWildcard() && !integral) ಹಿಂತಿರುಗಿ wildcardTypeError(binp, exprp);

        // Build condition by OR-ing all ranges together
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ತುಂಬಿದ_Condp = ಶೂನ್ಯಸೂಚಕ;

        ಗಾಗಿ (Ast_ಗಂಟು* currRangep = rangep; currRangep; currRangep = currRangep->nextp()) {
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* rangeCondp = ಶೂನ್ಯಸೂಚಕ;
            currRangep = V3Const::constifyEdit(currRangep);

            ಆದರೆ (AstInsideRange* irp = VN_CAST(currRangep, InsideRange)) {
                Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ minExprp = irp->lhsp();
                Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ maxExprp = irp->rhsp();
                AstConst* ಸ್ಥಿರಾಂಕ minConstp = VN_CAST(minExprp, Const);
                AstConst* ಸ್ಥಿರಾಂಕ maxConstp = VN_CAST(maxExprp, Const);
                ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ loUnbounded = VN_IS(minExprp, Unbounded);
                ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ hiUnbounded = VN_IS(maxExprp, Unbounded);
                ಆದರೆ (loUnbounded || hiUnbounded) {
                    // Open-ended range: '$' is the coverpoint domain min/max, so the
                    // range reduces to a single inequality (e.g. {[10:$]} -> expr >= 10).
                    AstConst* ಸ್ಥಿರಾಂಕ boundp = hiUnbounded ? minConstp : maxConstp;
                    ಆದರೆ (loUnbounded && hiUnbounded) {
                        rangeCondp = ಹೊಸ AstConst{irp->fileline(), AstConst::ಬಿಟ್_True{}};
                    } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (!boundp) {
                        irp->v3error("Non-constant expression in bin range; "
                                     "range bounds must be constants (IEEE 1800-2023 19.5)");
                        ಆದರೆ (ತುಂಬಿದ_Condp) VL_DO_DANGLING(ತಳ್ಳು_Deletep(ತುಂಬಿದ_Condp), ತುಂಬಿದ_Condp);
                        ಹಿಂತಿರುಗಿ ಶೂನ್ಯಸೂಚಕ;
                    } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (boundp->ಅಂಕೆ().isFourState()) {
                        irp->v3error("Four-state (x/z) value in bin range bound; "
                                     "range bounds must be two-state constants");
                        ಆದರೆ (ತುಂಬಿದ_Condp) VL_DO_DANGLING(ತಳ್ಳು_Deletep(ತುಂಬಿದ_Condp), ತುಂಬಿದ_Condp);
                        ಹಿಂತಿರುಗಿ ಶೂನ್ಯಸೂಚಕ;
                    } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (integral) {
                        rangeCondp = build_ಮೌಲ್ಯ_Condition(binp, exprp, irp);
                    } ಇಲ್ಲದಿದ್ದರೆ {
                        rangeCondp = makeOpenRangeCondition(irp->fileline(), exprp, boundp,
                                                            /*isLowerBound=*/hiUnbounded);
                    }
                } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (!minConstp || !maxConstp) {
                    irp->v3error("Non-constant expression in bin range; "
                                 "range bounds must be constants (IEEE 1800-2023 19.5)");
                    ಆದರೆ (ತುಂಬಿದ_Condp) VL_DO_DANGLING(ತಳ್ಳು_Deletep(ತುಂಬಿದ_Condp), ತುಂಬಿದ_Condp);
                    ಹಿಂತಿರುಗಿ ಶೂನ್ಯಸೂಚಕ;
                } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (minConstp->ಅಂಕೆ().isFourState() || maxConstp->ಅಂಕೆ().isFourState()) {
                    irp->v3error("Four-state (x/z) value in bin range bound; "
                                 "range bounds must be two-state constants");
                    ಆದರೆ (ತುಂಬಿದ_Condp) VL_DO_DANGLING(ತಳ್ಳು_Deletep(ತುಂಬಿದ_Condp), ತುಂಬಿದ_Condp);
                    ಹಿಂತಿರುಗಿ ಶೂನ್ಯಸೂಚಕ;
                } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (integral) {
                    rangeCondp = build_ಮೌಲ್ಯ_Condition(binp, exprp, irp);
                } ಇಲ್ಲದಿದ್ದರೆ {
                    rangeCondp = makeRangeCondition(irp->fileline(), exprp, minExprp, maxExprp);
                }
            } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (AstConst* constp = VN_CAST(currRangep, Const)) {
                rangeCondp = build_ಮೌಲ್ಯ_Condition(binp, exprp, constp);
            } ಇಲ್ಲದಿದ್ದರೆ {
                currRangep->v3error("Non-constant expression in bin range; values must be "
                                    "constants (IEEE 1800-2023 19.5)");
                ಆದರೆ (ತುಂಬಿದ_Condp) VL_DO_DANGLING(ತಳ್ಳು_Deletep(ತುಂಬಿದ_Condp), ತುಂಬಿದ_Condp);
                ಹಿಂತಿರುಗಿ ಶೂನ್ಯಸೂಚಕ;
            }

            UASSERT_OBJ(rangeCondp, binp, "rangeCondp is null after building range condition");
            ತುಂಬಿದ_Condp
                = ತುಂಬಿದ_Condp ? ಹೊಸ AstOr{binp->fileline(), ತುಂಬಿದ_Condp, rangeCondp} : rangeCondp;
        }

        ಹಿಂತಿರುಗಿ ತುಂಬಿದ_Condp;
    }

    // Wildcard bits have no meaning for a non-integral coverpoint.
    ಸ್ಥಿರ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* wildcardTypeError(AstCoverBin* binp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* exprp) {
        ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_DType* ಸ್ಥಿರಾಂಕ dtypep = exprp->dtypep()->skipRefp();
        exprp->v3error("Cannot use a wildcard bin on a coverpoint of type "
                       << dtypep->prettyDTypeNameQ() << " (IEEE 1800-2023 19.5.4).\n"
                       << exprp->warnContextPrimary() << '\n'
                       << binp->warnOther() << "... Location of wildcard bin\n"
                       << binp->warnContextSecondary());
        ಹಿಂತಿರುಗಿ ಹೊಸ AstConst{binp->fileline(), AstConst::ಬಿಟ್_False{}};
    }

    // Match one bin value, range, or wildcard pattern.  Integral coverpoints first resolve the
    // value to their type (IEEE 1800-2023 19.5.7).  Non-owning: clones what it uses.
    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* build_ಮೌಲ್ಯ_Condition(AstCoverBin* binp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* exprp, Ast_ಗಂಟು* valuep) {
        File_ಸಾಲು* ಸ್ಥಿರಾಂಕ fl = valuep->fileline();
        ಆದರೆ (!exprp->dtypep()->skipRefp()->isIntegralOrPacked()) {
            ಹಿಂತಿರುಗಿ ಹೊಸ AstEq{fl, exprp->clone_ವೃಕ್ಷ(ಸುಳ್ಳು),
                             VN_AS(valuep, ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ)->clone_ವೃಕ್ಷ(ಸುಳ್ಳು)};
        }
        Cross_ಮೌಲ್ಯ_Range range{valuep, resolve_ಅಗಲ(valuep, exprp)};
        ಆದರೆ (!resolve_ಮೌಲ್ಯ(valuep, exprp, ನಿಜ, binp->isWildcard(), range)) {
            valuep->v3warn(E_UNSUPPORTED,
                           "Unsupported: non-integral value in a coverage bin of an "
                           "integral coverpoint.");
            ಹಿಂತಿರುಗಿ ಹೊಸ AstConst{fl, AstConst::ಬಿಟ್_False{}};
        }
        ಆದರೆ (cross_Range_ಖಾಲಿಗಳು(range)) ಹಿಂತಿರುಗಿ ಹೊಸ AstConst{fl, AstConst::ಬಿಟ್_False{}};
        AstConst* ಸ್ಥಿರಾಂಕ lop = new_ಮೌಲ್ಯ_Const(fl, range.lo, exprp);
        AstConst* ಸ್ಥಿರಾಂಕ hip = new_ಮೌಲ್ಯ_Const(fl, range.hi, exprp);
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* condp = ಶೂನ್ಯಸೂಚಕ;
        ಆದರೆ (lop->ಅಂಕೆ().isCaseEq(hip->ಅಂಕೆ())) {
            condp = ಹೊಸ AstEq{fl, exprp->clone_ವೃಕ್ಷ(ಸುಳ್ಳು), lop};
        } ಇಲ್ಲದಿದ್ದರೆ {
            condp = makeRangeCondition(fl, exprp, lop, hip);
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(lop), lop);
        }
        VL_DO_DANGLING(ತಳ್ಳು_Deletep(hip), hip);
        ಆದರೆ (!range.wildcard) ಹಿಂತಿರುಗಿ condp;
        // Match the pattern's value bits within the source-value bounds.
        V3_ಸಂಖ್ಯೆ ಮುಖವಾಡ{valuep, exprp->ಅಗಲ()};
        V3_ಸಂಖ್ಯೆ ಮೌಲ್ಯ{valuep, exprp->ಅಗಲ()};
        ಮುಖವಾಡ.op_ಬಿಟ್ಟುಗಳು_Non_XZ(range.ನಮೂನೆ_2);
        ಮೌಲ್ಯ.op_ಬಿಟ್ಟುಗಳು_One(range.ನಮೂನೆ_2);
        AstConst* ಸ್ಥಿರಾಂಕ ಮುಖವಾಡ_Constp = new_ಮೌಲ್ಯ_Const(fl, ಮುಖವಾಡ, exprp);
        AstConst* ಸ್ಥಿರಾಂಕ ಮೌಲ್ಯ_Constp = new_ಮೌಲ್ಯ_Const(fl, ಮೌಲ್ಯ, exprp);
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ ಅಭಿವ್ಯಕ್ತಿ_Masked = ಹೊಸ AstAnd{fl, exprp->clone_ವೃಕ್ಷ(ಸುಳ್ಳು), ಮುಖವಾಡ_Constp};
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ ಮೌಲ್ಯ_Masked = ಹೊಸ AstAnd{fl, ಮೌಲ್ಯ_Constp, ಮುಖವಾಡ_Constp->clone_ವೃಕ್ಷ(ಸುಳ್ಳು)};
        ಹಿಂತಿರುಗಿ ಹೊಸ AstLogAnd{fl, condp, ಹೊಸ AstEq{fl, ಅಭಿವ್ಯಕ್ತಿ_Masked, ಮೌಲ್ಯ_Masked}};
    }

    ಶೂನ್ಯ generate_Coverage_Computation_ಸಂಕೇತ() {
        UINFO(4, "  Generating coverage computation code");

        // Invalidate cache: addMembersp() calls in generateCoverpointCode/generateCrossCode
        // have added new members since the last scan, so clear before re-querying.
        m_memberMap.clear();

        // get_inst_coverage(): the average of the coverpoints and crosses, weighted by their
        // option.weight (IEEE 1800-2023 19.11).  The instance node holds their runtimes.  With
        // the instances merged, get_coverage() instead, unless option.get_inst_coverage
        // (Table 19-1).
        AstFunc* ಸ್ಥಿರಾಂಕ getInstCoveragep
            = VN_AS(m_memberMap.ಕಂಡುಹಿಡಿ_Member(m_covergroupp, "get_inst_coverage"), Func);
        File_ಸಾಲು* ಸ್ಥಿರಾಂಕ instFl = getInstCoveragep->fileline();
        AstCMethodHard* ಸ್ಥಿರಾಂಕ instCallp = instanceCall(instFl, VCMethod::COVERGROUP_COVERAGE);
        instCallp->dtype_ಗಣ_Double();
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* instValuep = instCallp;
        ಆದರೆ (m_cg_May_ವಿಲೀನ) {
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ mergedp = ಹೊಸ AstLogAnd{
                instFl, newOptionSel(instFl, optionVar(ನಿಜ), "merge_instances", VAccess::READ),
                ಹೊಸ AstLogNot{instFl, newOptionSel(instFl, optionVar(ಸುಳ್ಳು), "get_inst_coverage",
                                                   VAccess::READ)}};
            instValuep = ಹೊಸ AstCond{instFl, mergedp, typeCoverageCall(instFl), instCallp};
        }
        getInstCoveragep->ಕೂಡಿಸು_Stmtsp(ಹೊಸ Ast_ನಿಯೋಜಿಸು{
            instFl, ಹೊಸ AstVarRef{instFl, VN_AS(getInstCoveragep->fvarp(), Var), VAccess::WRITE},
            instValuep});

        // get_coverage(): the average of the covergroup's instances, weighted by their
        // option.weight, or with type_option.merge_instances, the coverage of the union of their
        // bins (IEEE 1800-2023 19.11.3).  Static, so the registry finds the instances.
        AstFunc* ಸ್ಥಿರಾಂಕ getCoveragep
            = VN_AS(m_memberMap.ಕಂಡುಹಿಡಿ_Member(m_covergroupp, "get_coverage"), Func);
        File_ಸಾಲು* ಸ್ಥಿರಾಂಕ typeFl = getCoveragep->fileline();
        getCoveragep->ಕೂಡಿಸು_Stmtsp(ಹೊಸ Ast_ನಿಯೋಜಿಸು{
            typeFl, ಹೊಸ AstVarRef{typeFl, VN_AS(getCoveragep->fvarp(), Var), VAccess::WRITE},
            typeCoverageCall(typeFl)});
    }

    // The registry call computing the covergroup's type coverage, per its type options
    AstCMethodHard* typeCoverageCall(File_ಸಾಲು* fl) {
        AstCExpr* ಸ್ಥಿರಾಂಕ registryp = ctext(fl, "vlSymsp->_vm_contextp__->covergroupRegistryp()");
        registryp->dtype_ಗಣ_Void();  // Opaque receiver; only ever the 'fromp' of the call below
        AstCMethodHard* ಸ್ಥಿರಾಂಕ callp
            = ಹೊಸ AstCMethodHard{fl, registryp, VCMethod::COVERGROUP_TYPE_COVERAGE};
        callp->ಕೂಡಿಸು_Pinsp(ctext(fl, quoted(covergroupProtectedName())));
        callp->ಕೂಡಿಸು_Pinsp(newOptionSel(fl, optionVar(ನಿಜ), "weight", VAccess::READ));
        callp->ಕೂಡಿಸು_Pinsp(newOptionSel(fl, optionVar(ನಿಜ), "merge_instances", VAccess::READ));
        callp->ಕೂಡಿಸು_Pinsp(file_ಸಾಲು_Debug(m_covergroupp->fileline()));
        callp->usePtr(ನಿಜ);
        callp->dtype_ಗಣ_Double();
        ಹಿಂತಿರುಗಿ callp;
    }

    // VISITORS
    ಸ್ಥಿರ ತಾರ್ಕಿಕ isEnclosingInstanceVar(ಸ್ಥಿರಾಂಕ AstVar* varp) {
        ಹಿಂತಿರುಗಿ varp->isClassMember() && !varp->lifetime().isStatic() && !varp->isParam();
    }

    ಶೂನ್ಯ rewriteThisRef(AstThisRef* refp, AstVar* handleVarp) {
        ಸ್ಥಿರಾಂಕ AstClassRefDType* ಸ್ಥಿರಾಂಕ refDTypep
            = VN_CAST(refp->dtypep()->skipRefp(), ClassRefDType);
        UASSERT_OBJ(refDTypep && refDTypep->classp() == m_covergroupp, refp,
                    "Unexpected this reference in embedded covergroup");
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ newp = ಹೊಸ AstVarRef{refp->fileline(), handleVarp, VAccess::READ};
        refp->replaceWith(newp);
        VL_DO_DANGLING(ತಳ್ಳು_Deletep(refp), refp);
    }

    ಶೂನ್ಯ rewriteVarRef(AstVarRef* refp, AstVar* handleVarp) {
        File_ಸಾಲು* ಸ್ಥಿರಾಂಕ fl = refp->fileline();
        // As V3LinkLValue does, the handle of a written member select is marked written
        AstMemberSel* ಸ್ಥಿರಾಂಕ selp
            = ಹೊಸ AstMemberSel{fl, ಹೊಸ AstVarRef{fl, handleVarp, refp->access()}, refp->varp()};
        selp->access(refp->access());
        refp->replaceWith(selp);
        VL_DO_DANGLING(ತಳ್ಳು_Deletep(refp), refp);
    }

    ಶೂನ್ಯ rewriteFuncRef(AstFuncRef* refp, AstVar* handleVarp) {
        File_ಸಾಲು* ಸ್ಥಿರಾಂಕ fl = refp->fileline();
        AstArg* ಸ್ಥಿರಾಂಕ argsp = refp->argsp() ? refp->argsp()->unlink_Fr_ಹಿಂಭಾಗ_With_ಮುಂದಿನ() : ಶೂನ್ಯಸೂಚಕ;
        Ast_ವಿಧಾನ_Call* ಸ್ಥಿರಾಂಕ callp = ಹೊಸ Ast_ವಿಧಾನ_Call{
            fl, ಹೊಸ AstVarRef{fl, handleVarp, VAccess::READ}, refp->name(), argsp};
        callp->taskp(refp->taskp());
        callp->dtypeFrom(refp);
        refp->replaceWith(callp);
        VL_DO_DANGLING(ತಳ್ಳು_Deletep(refp), refp);
    }

    // True if funcp is an instance method of the enclosing class or one of its bases
    ತಾರ್ಕಿಕ isEnclosingInstanceFunc(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_FTask* funcp) ಸ್ಥಿರಾಂಕ {
        ಆದರೆ (!funcp->class_ವಿಧಾನ() || funcp->isStatic()) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
        ಹಿಂತಿರುಗಿ AstClass::isClassExtendedFrom(m_enclosingClassp, VN_AS(funcp->aboveLoopp(), Class));
    }

    ತಾರ್ಕಿಕ isEmbeddedCovergroupVar(ಸ್ಥಿರಾಂಕ AstVar* varp) ಸ್ಥಿರಾಂಕ {
        ಆದರೆ (!varp || !varp->isClassMember() || varp->isDeclTyped()) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
        ಸ್ಥಿರಾಂಕ AstClassRefDType* ಸ್ಥಿರಾಂಕ refp = VN_CAST(varp->dtypep()->skipRefp(), ClassRefDType);
        ಹಿಂತಿರುಗಿ refp && refp->classp() == m_covergroupp;
    }

    AstVar* ಕಂಡುಹಿಡಿ_Embedded_Covergroup_Var() ಸ್ಥಿರಾಂಕ {
        ಆದರೆ (!m_enclosingClassp) ಹಿಂತಿರುಗಿ ಶೂನ್ಯಸೂಚಕ;
        ಗಾಗಿ (Ast_ಗಂಟು* itemp = m_enclosingClassp->membersp(); itemp; itemp = itemp->nextp()) {
            ಆದರೆ (AstVar* ಸ್ಥಿರಾಂಕ varp = VN_CAST(itemp, Var)) {
                ಆದರೆ (isEmbeddedCovergroupVar(varp)) ಹಿಂತಿರುಗಿ varp;
            }
        }
        // V3LinkParse always creates an implicit variable for an embedded covergroup.
        ಹಿಂತಿರುಗಿ ಶೂನ್ಯಸೂಚಕ;  // LCOV_EXCL_LINE
    }

    std::ಸದಿಶ<Ast_ಗಂಟು_ನಿಯೋಜಿಸು*> ಕಂಡುಹಿಡಿ_Covergroup_Constructions() {
        std::ಸದಿಶ<Ast_ಗಂಟು_ನಿಯೋಜಿಸು*> foundps;
        ಆದರೆ (!m_embeddedVarp) ಹಿಂತಿರುಗಿ foundps;
        AstFunc* ಸ್ಥಿರಾಂಕ enclosingNewp
            = VN_CAST(m_memberMap.ಕಂಡುಹಿಡಿ_Member(m_enclosingClassp, "new"), Func);
        ಆದರೆ (!enclosingNewp) ಹಿಂತಿರುಗಿ foundps;
        enclosingNewp->foreach([&](Ast_ಗಂಟು_ನಿಯೋಜಿಸು* asgnp) {
            ಸ್ಥಿರಾಂಕ AstNew* ಸ್ಥಿರಾಂಕ newp = VN_CAST(asgnp->rhsp(), New);
            ಸ್ಥಿರಾಂಕ AstVarRef* ಸ್ಥಿರಾಂಕ lhsRefp = VN_CAST(asgnp->lhsp(), VarRef);
            ಆದರೆ (!newp || !lhsRefp || lhsRefp->varp() != m_embeddedVarp) ಹಿಂತಿರುಗಿ;
            ಸ್ಥಿರಾಂಕ AstClassRefDType* ಸ್ಥಿರಾಂಕ refp = VN_CAST(newp->dtypep(), ClassRefDType);
            ಆದರೆ (refp && refp->classp() == m_covergroupp) foundps.ತಳ್ಳು_ಹಿಂಭಾಗ(asgnp);
        });
        ಹಿಂತಿರುಗಿ foundps;
    }

    std::ಗಣ<ಸ್ಥಿರಾಂಕ AstVar*> enclosingInstanceVars() ಸ್ಥಿರಾಂಕ {
        std::ಗಣ<ಸ್ಥಿರಾಂಕ AstVar*> vars;
        ಆದರೆ (m_enclosingClassp) {
            m_enclosingClassp->foreachMember([&](AstClass* ಸ್ಥಿರಾಂಕ, AstVar* ಸ್ಥಿರಾಂಕ varp) {
                ಆದರೆ (isEnclosingInstanceVar(varp)) vars.ಸೇರಿಸು(varp);
            });
        }
        ಹಿಂತಿರುಗಿ vars;
    }

    ತಾರ್ಕಿಕ hasEnclosingEventRef(AstCovergroup* cgp) ಸ್ಥಿರಾಂಕ {
        ಆದರೆ (!m_embeddedVarp || !cgp->eventp()) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
        ಸ್ಥಿರಾಂಕ std::ಗಣ<ಸ್ಥಿರಾಂಕ AstVar*> enclosingVars = enclosingInstanceVars();
        ತಾರ್ಕಿಕ ಸಿಕ್ಕಿತು = ಸುಳ್ಳು;
        cgp->eventp()->foreach([&](AstVarRef* refp) {
            ಆದರೆ (enclosingVars.ಎಣಿಕೆ_2(refp->varp())) ಸಿಕ್ಕಿತು = ನಿಜ;
        });
        ಹಿಂತಿರುಗಿ ಸಿಕ್ಕಿತು;
    }

    ಸ್ಥಿರ ತಾರ್ಕಿಕ parse_Embedded_Event_ಅಭಿವ್ಯಕ್ತಿ(Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* exprp, AstVar*& ತಳ_Varp,
                                       AstVar*& memberVarp) {
        ಆದರೆ (AstVarRef* ಸ್ಥಿರಾಂಕ refp = VN_CAST(exprp, VarRef)) {
            ತಳ_Varp = refp->varp();
            memberVarp = ಶೂನ್ಯಸೂಚಕ;
            ಹಿಂತಿರುಗಿ ನಿಜ;
        }
        AstMemberSel* ಸ್ಥಿರಾಂಕ selp = VN_CAST(exprp, MemberSel);
        ಆದರೆ (!selp) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
        AstVarRef* ಸ್ಥಿರಾಂಕ ತಳ_Refp = VN_CAST(selp->fromp(), VarRef);
        ಆದರೆ (!ತಳ_Refp) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
        ತಳ_Varp = ತಳ_Refp->varp();
        memberVarp = selp->varp();
        ಹಿಂತಿರುಗಿ ನಿಜ;
    }

    ತಾರ್ಕಿಕ isEventLvalue(Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* exprp, ಸ್ಥಿರಾಂಕ EmbeddedEventTrigger& trigger) ಸ್ಥಿರಾಂಕ {
        ಆದರೆ (AstSel* ಸ್ಥಿರಾಂಕ selp = VN_CAST(exprp, Sel)) exprp = selp->fromp();
        AstVar* ತಳ_Varp = ಶೂನ್ಯಸೂಚಕ;
        AstVar* memberVarp = ಶೂನ್ಯಸೂಚಕ;
        ಆದರೆ (!parse_Embedded_Event_ಅಭಿವ್ಯಕ್ತಿ(exprp, ತಳ_Varp, memberVarp)) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
        ಹಿಂತಿರುಗಿ ತಳ_Varp == trigger.ತಳ_Varp && memberVarp == trigger.memberVarp;
    }

    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* newEventRead(File_ಸಾಲು* fl, ಸ್ಥಿರಾಂಕ EmbeddedEventTrigger& trigger) ಸ್ಥಿರಾಂಕ {
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ basep = ಹೊಸ AstVarRef{fl, trigger.ತಳ_Varp, VAccess::READ};
        ಆದರೆ (!trigger.memberVarp) ಹಿಂತಿರುಗಿ basep;
        AstMemberSel* ಸ್ಥಿರಾಂಕ selp = ಹೊಸ AstMemberSel{fl, basep, trigger.memberVarp};
        selp->access(VAccess::READ);
        ಹಿಂತಿರುಗಿ selp;
    }

    string eventPrevName(ಸ್ಥಿರಾಂಕ EmbeddedEventTrigger& trigger, ಗಾತ್ರ_t trigger_ಸೂಚ್ಯಂಕ) ಸ್ಥಿರಾಂಕ {
        string name = "__Vcg_prev_" + m_embeddedVarp->name() + "_" + std::ಗಮ್ಯ_string(trigger_ಸೂಚ್ಯಂಕ)
                      + "_" + trigger.ತಳ_Varp->name();
        ಆದರೆ (trigger.memberVarp) name += "_" + trigger.memberVarp->name();
        ಹಿಂತಿರುಗಿ name;
    }

    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* newEmbeddedVarNonNull(File_ಸಾಲು* fl) ಸ್ಥಿರಾಂಕ {
        ಹಿಂತಿರುಗಿ ಹೊಸ AstNeq{fl, ಹೊಸ AstVarRef{fl, m_embeddedVarp, VAccess::READ},
                          ಹೊಸ AstConst{fl, AstConst::Null{}}};
    }

    Ast_ಗಂಟು_Stmt* newSampleStmt(File_ಸಾಲು* fl) ಸ್ಥಿರಾಂಕ {
        Ast_ವಿಧಾನ_Call* ಸ್ಥಿರಾಂಕ callp = ಹೊಸ Ast_ವಿಧಾನ_Call{
            fl, ಹೊಸ AstVarRef{fl, m_embeddedVarp, VAccess::READ}, "sample", ಶೂನ್ಯಸೂಚಕ};
        callp->taskp(m_sampleFuncp);
        callp->dtype_ಗಣ_Void();
        ಹಿಂತಿರುಗಿ callp->makeStmt();
    }

    ಶೂನ್ಯ installEmbeddedEventFork(Ast_Sen_ವೃಕ್ಷ* eventp,
                                  ಸ್ಥಿರಾಂಕ std::ಸದಿಶ<Ast_ಗಂಟು_ನಿಯೋಜಿಸು*>& constructps) {
        // IEEE 1800-2023 19.3 samples coverpoints whenever their clocking event occurs. A
        // per-instance event cannot use V3Active's static sensitivity path, so spawn
        // 'fork forever begin @(event); cg.sample(); end join_none' after each construction.
        ಗಾಗಿ (Ast_ಗಂಟು_ನಿಯೋಜಿಸು* ಸ್ಥಿರಾಂಕ constructp : constructps) {
            File_ಸಾಲು* ಸ್ಥಿರಾಂಕ fl = constructp->fileline();
            AstLoop* ಸ್ಥಿರಾಂಕ loopp = ಹೊಸ AstLoop{fl};
            loopp->ಕೂಡಿಸು_Stmtsp(ಹೊಸ AstEventControl{fl, eventp->clone_ವೃಕ್ಷ(ಸುಳ್ಳು), ಶೂನ್ಯಸೂಚಕ});
            loopp->ಕೂಡಿಸು_Stmtsp(ಹೊಸ AstIf{fl, newEmbeddedVarNonNull(fl), newSampleStmt(fl)});
            AstFork* ಸ್ಥಿರಾಂಕ forkp = ಹೊಸ AstFork{fl, VJoinType::JOIN_NONE};
            forkp->ತಕ್ಷಣ_ಆರಂಭ(ನಿಜ);
            forkp->ಕೂಡಿಸು_Forksp(ಹೊಸ AstBegin{fl, "", loopp, ನಿಜ});
            constructp->ಕೂಡಿಸು_ಮುಂದಿನ_ಇಲ್ಲಿ(forkp);
        }
        VL_DO_DANGLING(ತಳ್ಳು_Deletep(eventp), eventp);
    }

    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* newEventReadyCondition(File_ಸಾಲು* fl, ಸ್ಥಿರಾಂಕ EmbeddedEventTrigger& trigger) ಸ್ಥಿರಾಂಕ {
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ curp = newEventRead(fl, trigger);
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ prevp = ಹೊಸ AstVarRef{fl, trigger.prevVarp, VAccess::READ};
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* edgep = ಶೂನ್ಯಸೂಚಕ;
        // IEEE 1800-2023 9.4.2 detects edge-qualified events only on the expression's LSB,
        // while an implicit change event observes the complete expression.
        ಆದರೆ (trigger.ಅಂಚು_Type == VEdgeType::ET_POSEDGE) {
            edgep = ಹೊಸ AstSel{fl, ಹೊಸ AstAnd{fl, curp, ಹೊಸ AstNot{fl, prevp}}, 0, 1};
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (trigger.ಅಂಚು_Type == VEdgeType::ET_NEGEDGE) {
            edgep = ಹೊಸ AstSel{fl, ಹೊಸ AstAnd{fl, ಹೊಸ AstNot{fl, curp}, prevp}, 0, 1};
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (trigger.ಅಂಚು_Type == VEdgeType::ET_BOTHEDGE) {
            edgep = ಹೊಸ AstSel{fl, ಹೊಸ AstXor{fl, curp, prevp}, 0, 1};
        } ಇಲ್ಲದಿದ್ದರೆ {
            edgep = ಹೊಸ AstNeq{fl, curp, prevp};
        }
        ಹಿಂತಿರುಗಿ ಹೊಸ AstLogAnd{fl, newEmbeddedVarNonNull(fl), edgep};
    }

    std::ಸದಿಶ<EmbeddedEventTrigger> collectEmbeddedEventTriggers(AstCovergroup* cgp) {
        std::ಸದಿಶ<EmbeddedEventTrigger> triggers;
        ಸ್ಥಿರಾಂಕ std::ಗಣ<ಸ್ಥಿರಾಂಕ AstVar*> enclosingVars = enclosingInstanceVars();
        ಗಾಗಿ (Ast_ಗಂಟು* senp = cgp->eventp()->sensesp(); senp; senp = senp->nextp()) {
            Ast_Sen_ಅಂಶ* ಸ್ಥಿರಾಂಕ itemp = VN_AS(senp, Sen_ಅಂಶ);
            AstVar* ತಳ_Varp = ಶೂನ್ಯಸೂಚಕ;
            AstVar* memberVarp = ಶೂನ್ಯಸೂಚಕ;
            ಆದರೆ (!parse_Embedded_Event_ಅಭಿವ್ಯಕ್ತಿ(itemp->sensp(), ತಳ_Varp, memberVarp)
                || !enclosingVars.ಎಣಿಕೆ_2(ತಳ_Varp)) {
                ಹಿಂತಿರುಗಿ {};
            }
            triggers.emplace_ಹಿಂಭಾಗ(itemp->fileline(), ತಳ_Varp, memberVarp, itemp->ಅಂಚು_Type());
        }
        ಹಿಂತಿರುಗಿ triggers;
    }

    ಶೂನ್ಯ installEmbeddedEventTriggers(std::ಸದಿಶ<EmbeddedEventTrigger>& triggers,
                                      ಸ್ಥಿರಾಂಕ std::ಸದಿಶ<Ast_ಗಂಟು_ನಿಯೋಜಿಸು*>& constructps) {
        // Without --timing, approximate a per-instance event by sampling after assignments
        // within the enclosing class. External writes and exact scheduling cannot be observed.
        ಆದರೆ (constructps.ಖಾಲಿಗಳು()) ಹಿಂತಿರುಗಿ;
        ಗಾಗಿ (ಗಾತ್ರ_t trigger_ಸೂಚ್ಯಂಕ = 0; trigger_ಸೂಚ್ಯಂಕ < triggers.ಗಾತ್ರ_2(); ++trigger_ಸೂಚ್ಯಂಕ) {
            EmbeddedEventTrigger& trigger = triggers[trigger_ಸೂಚ್ಯಂಕ];
            std::ಸದಿಶ<Ast_ಗಂಟು_ನಿಯೋಜಿಸು*> assignps;
            m_enclosingClassp->foreach([&](Ast_ಗಂಟು_ನಿಯೋಜಿಸು* asgnp) {
                ಆದರೆ (isEventLvalue(asgnp->lhsp(), trigger)) assignps.ತಳ್ಳು_ಹಿಂಭಾಗ(asgnp);
            });
            ಆದರೆ (assignps.ಖಾಲಿಗಳು()) {
                trigger.eventFl->v3warn(
                    COVERIGN, "Unsupported: 'covergroup' clocking event signal has no assignment "
                              "within the enclosing class; no coverage sampled. Use --timing for "
                              "full support.");
                ಮುಂದುವರಿ;
            }
            Ast_ಗಂಟು_DType* ಸ್ಥಿರಾಂಕ dtypep
                = trigger.memberVarp ? trigger.memberVarp->dtypep() : trigger.ತಳ_Varp->dtypep();
            AstVar* ಸ್ಥಿರಾಂಕ prevVarp = ಹೊಸ AstVar{trigger.eventFl, VVarType::MEMBER,
                                                eventPrevName(trigger, trigger_ಸೂಚ್ಯಂಕ), dtypep};
            m_enclosingClassp->ಕೂಡಿಸು_Membersp(prevVarp);
            trigger.prevVarp = prevVarp;
            ಗಾಗಿ (Ast_ಗಂಟು_ನಿಯೋಜಿಸು* ಸ್ಥಿರಾಂಕ asgnp : assignps) {
                File_ಸಾಲು* ಸ್ಥಿರಾಂಕ fl = asgnp->fileline();
                AstIf* ಸ್ಥಿರಾಂಕ ifp
                    = ಹೊಸ AstIf{fl, newEventReadyCondition(fl, trigger), newSampleStmt(fl)};
                ifp->ಕೂಡಿಸು_ಮುಂದಿನ_ಇಲ್ಲಿ(ಹೊಸ Ast_ನಿಯೋಜಿಸು{fl,
                                               ಹೊಸ AstVarRef{fl, trigger.prevVarp, VAccess::WRITE},
                                               newEventRead(fl, trigger)});
                asgnp->ಕೂಡಿಸು_ಮುಂದಿನ_ಇಲ್ಲಿ(ifp);
            }
        }
    }

    ಶೂನ್ಯ delete_Coverage_ವಸ್ತುಗಳು() {
        ಗಾಗಿ (AstCoverpoint* ಸ್ಥಿರಾಂಕ cpp : m_coverpoints) {
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(cpp->unlink_Fr_ಹಿಂಭಾಗ()), cpp);
        }
        ಗಾಗಿ (AstCoverCross* ಸ್ಥಿರಾಂಕ crossp : m_coverCrosses) {
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(crossp->unlink_Fr_ಹಿಂಭಾಗ()), crossp);
        }
        // Options not lowered: the covergroup was not processed
        ಗಾಗಿ (Ast_Cg_Option_ನಿಯೋಜಿಸು* ಸ್ಥಿರಾಂಕ optp : m_cgOptions) {
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(optp->unlink_Fr_ಹಿಂಭಾಗ()), optp);
        }
        m_cgOptions.clear();
    }

    ವರ್ಗ FormalRefVisitor final : ಸಾರ್ವಜನಿಕ VNVisitor {
        ಸ್ಥಿರಾಂಕ std::map<ಸ್ಥಿರಾಂಕ AstVar*, AstVar*>& m_replacements;

        ಶೂನ್ಯ ಭೇಟಿ(AstVarRef* nodep) override {
            ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ it = m_replacements.ಕಂಡುಹಿಡಿ(nodep->varp());
            ಆದರೆ (it == m_replacements.ಕೊನೆ()) ಹಿಂತಿರುಗಿ;
            nodep->varp(it->ಎರಡನೇ);
        }
        ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು* nodep) override { iterateChildren(nodep); }

    ಸಾರ್ವಜನಿಕ:
        ಸ್ಪಷ್ಟ FormalRefVisitor(ಸ್ಥಿರಾಂಕ std::map<ಸ್ಥಿರಾಂಕ AstVar*, AstVar*>& replacements)
            : m_replacements{replacements} {}
        ಶೂನ್ಯ ಪರಿಶೀಲನೆ(Ast_ಗಂಟು* nodep) { iterate(nodep); }
    };

    ಶೂನ್ಯ validateCovergroupExpressions() {
        std::ಗಣ<ಸ್ಥಿರಾಂಕ AstVar*> sampleMembers;
        std::ಗಣ<ಸ್ಥಿರಾಂಕ AstVar*> constructorRefMembers;
        ಗಾಗಿ (Ast_ಗಂಟು* stmtp = m_constructorp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            ಸ್ಥಿರಾಂಕ AstVar* ಸ್ಥಿರಾಂಕ varp = VN_CAST(stmtp, Var);
            ಆದರೆ (!varp || !varp->isIO() || (!varp->isRef() && !varp->isConstRef())) ಮುಂದುವರಿ;
            ಸ್ಥಿರಾಂಕ AstVar* ಸ್ಥಿರಾಂಕ memberp
                = VN_CAST(m_memberMap.ಕಂಡುಹಿಡಿ_Member(m_covergroupp, varp->name()), Var);
            UASSERT_OBJ(memberp && memberp->isClassMember(), varp,
                        "Covergroup constructor argument missing persistent member");
            constructorRefMembers.ಸೇರಿಸು(varp);
            constructorRefMembers.ಸೇರಿಸು(memberp);
        }
        ಗಾಗಿ (Ast_ಗಂಟು* stmtp = m_sampleFuncp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            ಸ್ಥಿರಾಂಕ AstVar* ಸ್ಥಿರಾಂಕ varp = VN_CAST(stmtp, Var);
            ಆದರೆ (!varp || !varp->isIO()) ಮುಂದುವರಿ;
            ಸ್ಥಿರಾಂಕ AstVar* ಸ್ಥಿರಾಂಕ memberp
                = VN_CAST(m_memberMap.ಕಂಡುಹಿಡಿ_Member(m_covergroupp, varp->name()), Var);
            UASSERT_OBJ(memberp && memberp->isClassMember(), varp,
                        "Covergroup sample argument missing persistent member");
            sampleMembers.ಸೇರಿಸು(memberp);
        }
        Covergroup_ಅಭಿವ್ಯಕ್ತಿ_Valid_Visitor{sampleMembers, constructorRefMembers}.ಪರಿಶೀಲನೆ(m_constructorp);
    }

    ಶೂನ್ಯ rebindFormalRefs() {
        std::map<ಸ್ಥಿರಾಂಕ AstVar*, AstVar*> replacements;
        ಗಾಗಿ (Ast_ಗಂಟು* stmtp = m_constructorp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            ಆದರೆ (ಸ್ಥಿರಾಂಕ AstVar* ಸ್ಥಿರಾಂಕ varp = VN_CAST(stmtp, Var)) {
                ಆದರೆ (!varp->isIO()) ಮುಂದುವರಿ;
                AstVar* ಸ್ಥಿರಾಂಕ memberp
                    = VN_CAST(m_memberMap.ಕಂಡುಹಿಡಿ_Member(m_covergroupp, varp->name()), Var);
                UASSERT_OBJ(memberp && memberp->isClassMember(), varp,
                            "Covergroup constructor argument missing persistent member");
                replacements.emplace(varp, memberp);
            }
        }
        ಗಾತ್ರ_t expectedBindings = 0;
        ಗಾಗಿ (ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ& pair : replacements) {
            ಆದರೆ (pair.ಮೊದಲ->isRef() || pair.ಮೊದಲ->isConstRef()) ++expectedBindings;
        }
        ಗಾತ್ರ_t rewrittenBindings = 0;
        ಗಾಗಿ (Ast_ಗಂಟು* stmtp = m_constructorp->stmtsp(); stmtp;) {
            Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ nextp = stmtp->nextp();
            ಆದರೆ (Ast_ನಿಯೋಜಿಸು* ಸ್ಥಿರಾಂಕ assignp = VN_CAST(stmtp, ನಿಯೋಜಿಸು)) {
                AstVarRef* ಸ್ಥಿರಾಂಕ lhsp = VN_CAST(assignp->lhsp(), VarRef);
                AstVarRef* ಸ್ಥಿರಾಂಕ rhsp = VN_CAST(assignp->rhsp(), VarRef);
                ಆದರೆ (lhsp && rhsp
                    && (lhsp->varp()->declDirection() == VDirection::REF
                        || lhsp->varp()->declDirection() == VDirection::CONSTREF)) {
                    ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ it = replacements.ಕಂಡುಹಿಡಿ(rhsp->varp());
                    UASSERT_OBJ(it != replacements.ಕೊನೆ() && it->ಎರಡನೇ == lhsp->varp(), assignp,
                                "Unexpected covergroup reference binding assignment");
                    AstCExpr* ಸ್ಥಿರಾಂಕ bindp = ಹೊಸ AstCExpr{assignp->fileline(), ""};
                    bindp->ಕೂಡಿಸು(lhsp->unlink_Fr_ಹಿಂಭಾಗ());
                    bindp->ಕೂಡಿಸು(" = &");
                    bindp->ಕೂಡಿಸು(rhsp->unlink_Fr_ಹಿಂಭಾಗ());
                    assignp->replaceWith(bindp->makeStmt());
                    VL_DO_DANGLING(ತಳ್ಳು_Deletep(assignp), assignp);
                    ++rewrittenBindings;
                }
            }
            stmtp = nextp;
        }
        UASSERT_OBJ(rewrittenBindings == expectedBindings, m_constructorp,
                    "Covergroup reference argument missing binding");
        FormalRefVisitor visitor{replacements};
        ಗಾಗಿ (AstCoverpoint* ಸ್ಥಿರಾಂಕ cpp : m_coverpoints) visitor.ಪರಿಶೀಲನೆ(cpp);
        ಗಾಗಿ (AstCoverCross* ಸ್ಥಿರಾಂಕ crossp : m_coverCrosses) visitor.ಪರಿಶೀಲನೆ(crossp);
    }

    AstVarRef* install_Enclosing_ಹಿಂಭಾಗ_ಸೂಚಕ(ಸ್ಥಿರಾಂಕ std::ಸದಿಶ<Ast_ಗಂಟು_ನಿಯೋಜಿಸು*>& constructps) {
        // Simple-case support for embedded covergroups (IEEE 1800-2023 19.4) whose
        // coverpoints reference members of the enclosing class ("Class members can be used
        // in coverpoint expressions").  The covergroup is lowered into a sibling class with
        // no implicit handle to the enclosing object, so such references would emit
        // uncompilable C++.  Add an explicit back-pointer member to the enclosing instance,
        // route member references through it, and pass it into the constructor so
        // coverage initialization can read enclosing members. Returns an invalid
        // reference if an outer class member cannot be reached; otherwise returns an empty result.
        ಆದರೆ (!m_enclosingClassp) ಹಿಂತಿರುಗಿ ಶೂನ್ಯಸೂಚಕ;  // Offending refs require an enclosing class

        AstVarRef* invalidp = ಶೂನ್ಯಸೂಚಕ;
        Ast_ಗಂಟು* offenderp = ಶೂನ್ಯಸೂಚಕ;
        std::ಗಣ<ಸ್ಥಿರಾಂಕ AstVar*> ownVars;
        ಗಾಗಿ (Ast_ಗಂಟು* itemp = m_covergroupp->membersp(); itemp; itemp = itemp->nextp()) {
            ಆದರೆ (ಸ್ಥಿರಾಂಕ AstVar* ಸ್ಥಿರಾಂಕ varp = VN_CAST(itemp, Var)) ownVars.ಸೇರಿಸು(varp);
        }
        ಸ್ಥಿರಾಂಕ std::ಗಣ<ಸ್ಥಿರಾಂಕ AstVar*> enclosingVars = enclosingInstanceVars();
        std::ಸದಿಶ<AstVarRef*> refs_ಗಮ್ಯ_Rewrite;
        std::ಸದಿಶ<AstThisRef*> this_Refs_ಗಮ್ಯ_Rewrite;
        std::ಸದಿಶ<AstFuncRef*> func_Refs_ಗಮ್ಯ_Rewrite;
        ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ ಪರಿಶೀಲನೆ = [&](Ast_ಗಂಟು* rootp) {
            rootp->foreach([&](AstVarRef* refp) {
                ಆದರೆ (invalidp) ಹಿಂತಿರುಗಿ;
                ಸ್ಥಿರಾಂಕ AstVar* ಸ್ಥಿರಾಂಕ varp = refp->varp();
                ಆದರೆ (!isEnclosingInstanceVar(varp) || ownVars.ಎಣಿಕೆ_2(varp)) ಹಿಂತಿರುಗಿ;
                ಆದರೆ (!enclosingVars.ಎಣಿಕೆ_2(varp)) {
                    invalidp = refp;
                    ಹಿಂತಿರುಗಿ;
                }
                refs_ಗಮ್ಯ_Rewrite.ತಳ್ಳು_ಹಿಂಭಾಗ(refp);
                ಆದರೆ (!offenderp) offenderp = refp;
            });
            ಆದರೆ (invalidp) ಹಿಂತಿರುಗಿ;
            rootp->foreach([&](AstThisRef* refp) {
                ಸ್ಥಿರಾಂಕ AstClassRefDType* ಸ್ಥಿರಾಂಕ refDTypep
                    = VN_CAST(refp->dtypep()->skipRefp(), ClassRefDType);
                ಆದರೆ (refDTypep && refDTypep->classp() == m_covergroupp) {
                    this_Refs_ಗಮ್ಯ_Rewrite.ತಳ್ಳು_ಹಿಂಭಾಗ(refp);
                    ಆದರೆ (!offenderp) offenderp = refp;
                }
            });
            rootp->foreach([&](AstFuncRef* refp) {
                ಆದರೆ (!isEnclosingInstanceFunc(refp->taskp())) ಹಿಂತಿರುಗಿ;
                func_Refs_ಗಮ್ಯ_Rewrite.ತಳ್ಳು_ಹಿಂಭಾಗ(refp);
                ಆದರೆ (!offenderp) offenderp = refp;
            });
        };
        ಗಾಗಿ (AstCoverpoint* ಸ್ಥಿರಾಂಕ cpp : m_coverpoints) ಪರಿಶೀಲನೆ(cpp);
        ಗಾಗಿ (AstCoverCross* ಸ್ಥಿರಾಂಕ crossp : m_coverCrosses) ಪರಿಶೀಲನೆ(crossp);
        ಗಾಗಿ (Ast_Cg_Option_ನಿಯೋಜಿಸು* ಸ್ಥಿರಾಂಕ optp : m_cgOptions) ಪರಿಶೀಲನೆ(optp);
        ಆದರೆ (invalidp || !offenderp) ಹಿಂತಿರುಗಿ invalidp;

        UASSERT_OBJ(m_embeddedVarp, m_covergroupp, "Embedded covergroup variable not found");
        // Commit: add the back-pointer member, rewrite the references, initialize the handle.
        File_ಸಾಲು* ಸ್ಥಿರಾಂಕ fl = m_covergroupp->fileline();
        AstClassRefDType* ಸ್ಥಿರಾಂಕ enclDTypep = ಹೊಸ AstClassRefDType{fl, m_enclosingClassp, ಶೂನ್ಯಸೂಚಕ};
        enclDTypep->raw_ಸೂಚಕ(ನಿಜ);
        v3Global.rootp()->typeTablep()->ಕೂಡಿಸು_Typesp(enclDTypep);
        AstVar* ಸ್ಥಿರಾಂಕ handleVarp
            = ಹೊಸ AstVar{fl, VVarType::MEMBER, "__Vcg_enclosingp", enclDTypep};
        m_covergroupp->ಕೂಡಿಸು_Membersp(handleVarp);
        AstVar* ಸ್ಥಿರಾಂಕ argumentp = ಹೊಸ AstVar{fl, VVarType::BLOCKTEMP, "__Vcg_parentp", enclDTypep};
        argumentp->direction(VDirection::ಒಳಹರಿವು);
        argumentp->declDirection(VDirection::ಒಳಹರಿವು);
        argumentp->funcLocal(ನಿಜ);
        argumentp->noReset(ನಿಜ);
        argumentp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
        m_constructorp->ಕೂಡಿಸು_Stmtsp(argumentp);
        m_constructorp->prependStmtsp(ಹೊಸ Ast_ನಿಯೋಜಿಸು{fl, memberRef(fl, handleVarp, VAccess::WRITE),
                                                    ಹೊಸ AstVarRef{fl, argumentp, VAccess::READ}});

        // Route each enclosing-member reference through the back-pointer: 'm' -> 'h.m'.
        ಗಾಗಿ (AstVarRef* ಸ್ಥಿರಾಂಕ refp : refs_ಗಮ್ಯ_Rewrite) { rewriteVarRef(refp, handleVarp); }
        ಗಾಗಿ (AstThisRef* ಸ್ಥಿರಾಂಕ refp : this_Refs_ಗಮ್ಯ_Rewrite) { rewriteThisRef(refp, handleVarp); }
        ಗಾಗಿ (AstFuncRef* ಸ್ಥಿರಾಂಕ refp : func_Refs_ಗಮ್ಯ_Rewrite) { rewriteFuncRef(refp, handleVarp); }

        // Append a named hidden argument to preserve positional and defaulted user arguments.
        ಗಾಗಿ (Ast_ಗಂಟು_ನಿಯೋಜಿಸು* ಸ್ಥಿರಾಂಕ constructp : constructps) {
            File_ಸಾಲು* ಸ್ಥಿರಾಂಕ cfl = constructp->fileline();
            AstCExpr* ಸ್ಥಿರಾಂಕ thisp = ಹೊಸ AstCExpr{cfl, "this"};
            thisp->dtypep(enclDTypep);
            VN_AS(constructp->rhsp(), New)->ಕೂಡಿಸು_Argsp(ಹೊಸ AstArg{cfl, argumentp->name(), thisp});
        }
        ಹಿಂತಿರುಗಿ ಶೂನ್ಯಸೂಚಕ;
    }

    ಶೂನ್ಯ ಭೇಟಿ(AstClass* nodep) override {
        UINFO(9, "Visiting class: " << nodep->name() << " isCovergroup=" << nodep->isCovergroup());
        ಆದರೆ (nodep->isCovergroup()) {
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
            m_embeddedVarp = ಕಂಡುಹಿಡಿ_Embedded_Covergroup_Var();
            m_covergroupName = covergroupTypeName();
            m_sampleFuncp = ಶೂನ್ಯಸೂಚಕ;
            m_constructorp = ಶೂನ್ಯಸೂಚಕ;
            std::ಸದಿಶ<EmbeddedEventTrigger> embeddedEventTriggers;
            Ast_Sen_ವೃಕ್ಷ* embeddedEventForkp = ಶೂನ್ಯಸೂಚಕ;

            // Extract and store the clocking event from AstCovergroup node
            // The parser creates this node to preserve the event information
            ತಾರ್ಕಿಕ hasUnsupportedEvent = ಸುಳ್ಳು;
            ಗಾಗಿ (Ast_ಗಂಟು* itemp = nodep->membersp(); itemp;) {
                Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ nextp = itemp->nextp();
                ಆದರೆ (AstCovergroup* ಸ್ಥಿರಾಂಕ cgp = VN_CAST(itemp, Covergroup)) {
                    // Store the event in the global map for V3Active to retrieve later
                    // V3LinkParse only creates this sentinel AstCovergroup node when a clocking
                    // event exists, so cgp->eventp() is always non-null here.
                    UASSERT_OBJ(cgp->eventp(), cgp,
                                "Sentinel AstCovergroup in class must have non-null eventp");
                    ಆದರೆ (hasEnclosingEventRef(cgp)) {
                        UASSERT_OBJ(m_embeddedVarp, cgp,
                                    "Embedded covergroup event has no instance variable");
                        ಆದರೆ (v3Global.opt.timing().is_ಗಣ_True()) {
                            embeddedEventForkp = cgp->eventp()->unlink_Fr_ಹಿಂಭಾಗ();
                        } ಇಲ್ಲದಿದ್ದರೆ {
                            embeddedEventTriggers = collectEmbeddedEventTriggers(cgp);
                            ಆದರೆ (embeddedEventTriggers.ಖಾಲಿಗಳು()) {
                                cgp->v3warn(COVERIGN,
                                            "Unsupported: 'covergroup' clocking event on complex "
                                            "member expression; use --timing for full support.");
                                hasUnsupportedEvent = ನಿಜ;
                            }
                        }
                        VL_DO_DANGLING(ತಳ್ಳು_Deletep(cgp->unlink_Fr_ಹಿಂಭಾಗ()), cgp);
                        itemp = nextp;
                        ಮುಂದುವರಿ;
                    }
                    // V3Active handles events that do not depend on an enclosing instance.
                    UINFO(4, "Keeping covergroup event node for V3Active: " << nodep->name());
                    itemp = nextp;
                    ಮುಂದುವರಿ;
                }
                itemp = nextp;
            }

            // Find the sample() method and constructor
            m_sampleFuncp = VN_CAST(m_memberMap.ಕಂಡುಹಿಡಿ_Member(nodep, "sample"), Func);
            // V3LinkParse always synthesizes a sample() method for every covergroup, and the
            // sampling-code generation below dereferences m_sampleFuncp unconditionally.
            UASSERT_OBJ(m_sampleFuncp, nodep, "Covergroup missing synthesized sample() method");
            m_sampleFuncp->isCovergroupSample(ನಿಜ);
            m_constructorp = VN_CAST(m_memberMap.ಕಂಡುಹಿಡಿ_Member(nodep, "new"), Func);
            UINFO(9, "Found sample() method: " << (m_sampleFuncp ? "yes" : "no"));
            UINFO(9, "Found constructor: " << (m_constructorp ? "yes" : "no"));

            // If covergroup has unsupported clocking event, skip processing it
            // but still clean up coverpoints so they don't reach downstream passes
            ಆದರೆ (hasUnsupportedEvent) {
                iterateChildren(nodep);
                validateCovergroupExpressions();
                rebindFormalRefs();
                delete_Coverage_ವಸ್ತುಗಳು();
                ಆದರೆ (embeddedEventForkp) {
                    VL_DO_DANGLING(ತಳ್ಳು_Deletep(embeddedEventForkp), embeddedEventForkp);
                }
                ಹಿಂತಿರುಗಿ;
            }

            iterateChildren(nodep);
            validateCovergroupExpressions();
            rebindFormalRefs();
            ಸ್ಥಿರಾಂಕ std::ಸದಿಶ<Ast_ಗಂಟು_ನಿಯೋಜಿಸು*> constructps = ಕಂಡುಹಿಡಿ_Covergroup_Constructions();

            // Embedded covergroups (IEEE 1800-2023 19.4): coverpoints, iff expressions, and
            // crosses may reference members of the enclosing class. The covergroup is lowered
            // into a sibling class with no implicit handle to the enclosing instance. Install
            // an explicit back-pointer and route the references through it.
            ಆದರೆ (AstVarRef* ಸ್ಥಿರಾಂಕ invalidp = install_Enclosing_ಹಿಂಭಾಗ_ಸೂಚಕ(constructps)) {
                invalidp->v3error("Non-static member "
                                  << invalidp->varp()->prettyNameQ()
                                  << " of an outer class requires an explicit "
                                     "object handle (IEEE 1800-2023 8.23).");
                delete_Coverage_ವಸ್ತುಗಳು();
                ಆದರೆ (embeddedEventForkp) {
                    VL_DO_DANGLING(ತಳ್ಳು_Deletep(embeddedEventForkp), embeddedEventForkp);
                }
                ಹಿಂತಿರುಗಿ;
            }
            installEmbeddedEventTriggers(embeddedEventTriggers, constructps);
            ಆದರೆ (embeddedEventForkp) installEmbeddedEventFork(embeddedEventForkp, constructps);
            processCovergroup();
            // Remove lowered coverpoints/crosses from the class - they have been
            // fully translated into C++ code and must not reach downstream passes
            delete_Coverage_ವಸ್ತುಗಳು();
        } ಇಲ್ಲದಿದ್ದರೆ {
            // Track the lexically enclosing class so a nested covergroup can resolve
            // references to the enclosing object's members (installEnclosingBackPointer).
            VL_RESTORER(m_enclosingClassp);
            m_enclosingClassp = nodep;
            iterateChildren(nodep);
        }
    }

    ಶೂನ್ಯ ಭೇಟಿ(AstCoverpoint* nodep) override {
        UINFO(9, "Found coverpoint: " << nodep->name());
        m_coverpoints.ತಳ್ಳು_ಹಿಂಭಾಗ(nodep);
        m_coverpointMap.emplace(nodep->name(), nodep);
        iterateChildren(nodep);
    }

    ಶೂನ್ಯ ಭೇಟಿ(AstCoverCross* nodep) override {
        UINFO(9, "Found cross: " << nodep->name());
        m_coverCrosses.ತಳ್ಳು_ಹಿಂಭಾಗ(nodep);
        iterateChildren(nodep);
    }

    // V3Width leaves only the covergroup-level options lowerCovergroupOptions() takes
    ಶೂನ್ಯ ಭೇಟಿ(Ast_Cg_Option_ನಿಯೋಜಿಸು* nodep) override { m_cgOptions.ತಳ್ಳು_ಹಿಂಭಾಗ(nodep); }

    // A package, interface, or module, so the design unit declaring the classes within
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು_Module* nodep) override {
        VL_RESTORER(m_unitp);
        m_unitp = nodep;
        iterateChildren(nodep);
    }

    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು* nodep) override { iterateChildren(nodep); }

ಸಾರ್ವಜನಿಕ:
    // CONSTRUCTORS
    FunctionalCoverageVisitor(AstNetlist* nodep,
                              ಸ್ಥಿರಾಂಕ std::ಗಣ<ಸ್ಥಿರಾಂಕ AstVar*>& mergeableTypeOptions)
        : m_mergeableTypeOptions{mergeableTypeOptions} {
        iterate(nodep);
    }
    ~FunctionalCoverageVisitor() override = ಪೂರ್ವನಿಯೋಜಿತ;
};

//######################################################################
// Functional coverage class functions

ಶೂನ್ಯ V3Covergroup::covergroup(AstNetlist* nodep) {
    UINFO(4, __FUNCTION__ << ": ");
    ಸ್ಥಿರಾಂಕ Covergroup_ನಿಯೋಜಿಸು_Valid_Visitor validVisitor{nodep};
    ಆದರೆ (!validVisitor.valid()) V3Error::abortIfErrors();
    {  // Destruct before checking
        FunctionalCoverageVisitor{nodep, validVisitor.mergeableTypeOptions()};
    }
    V3Global::dump_Check_Global_ವೃಕ್ಷ("coveragefunc", 0, dump_ವೃಕ್ಷ_Either_Level() >= 3);
}
