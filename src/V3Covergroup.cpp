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

ವರ್ಗ CovergroupAssignValidVisitor final : ಸಾರ್ವಜನಿಕ VNVisitorConst {
    VMemberMap m_memberMap;
    std::map<ಧ್ರುವ AstVar*, ಧ್ರುವ Ast_ಗಂಟು_FTask*>
        m_constructors;  // Implicit instance -> constructor
    std::set<ಧ್ರುವ AstVar*> m_mergeableTypeOptions;  // See mergeableTypeOptions()
    ಧ್ರುವ Ast_ಗಂಟು_FTask* m_ftaskp = nullptr;
    ತಾರ್ಕಿಕ m_collecting = ಸತ್ಯ;
    ತಾರ್ಕಿಕ m_valid = ಸತ್ಯ;

    ಶೂನ್ಯ visit(AstClass* nodep) override {
        VL_RESTORER(m_ftaskp);
        m_ftaskp = nullptr;
        ಯದಿ (m_collecting) {
            ಧ್ರುವ Ast_ಗಂಟು_FTask* ಧ್ರುವ constructorp
                = VN_CAST(m_memberMap.findMember(nodep, "new"), ಗಂಟು_FTask);
            ಪ್ರತಿ (ಧ್ರುವ Ast_ಗಂಟು* itemp = nodep->membersp(); itemp; itemp = itemp->nextp()) {
                ಧ್ರುವ AstVar* ಧ್ರುವ varp = VN_CAST(itemp, Var);
                // Only the implicit instance is restricted, not explicitly typed aliases.
                ಯದಿ (!varp || !varp->isClassMember() || varp->isDeclTyped()) ಅನುವರ್ತ;
                ಧ್ರುವ AstClassRefDType* ಧ್ರುವ refp
                    = VN_CAST(varp->dtypep()->skipRefp(), ClassRefDType);
                ಯದಿ (refp && refp->classp()->covergroupEnclosingClassp() == nodep) {
                    m_constructors.emplace(varp, constructorp);
                }
            }
        }
        iterateChildrenConst(nodep);
    }
    ಶೂನ್ಯ visit(Ast_ಗಂಟು_FTask* nodep) override {
        VL_RESTORER(m_ftaskp);
        m_ftaskp = nodep;
        iterateChildrenConst(nodep);
    }
    ಶೂನ್ಯ visit(Ast_ಗಂಟು_Assign* nodep) override {
        ಯದಿ (!m_collecting) {
            ಧ್ರುವ AstVar* varp = nullptr;
            ಯದಿ (ಧ್ರುವ Ast_ಗಂಟು_Var_Ref* ಧ್ರುವ refp = VN_CAST(nodep->lhsp(), ಗಂಟು_Var_Ref)) {
                varp = refp->varp();
            } ಅನ್ಯಥಾ ಯದಿ (ಧ್ರುವ AstMemberSel* ಧ್ರುವ selp = VN_CAST(nodep->lhsp(), MemberSel)) {
                varp = selp->varp();
            }
            ಧ್ರುವ auto it = m_constructors.find(varp);
            ಯದಿ (it != m_constructors.end() && (!m_ftaskp || m_ftaskp != it->second)) {
                m_valid = ಅಸತ್ಯ;
                nodep->v3error("Embedded covergroup variable "
                               << varp->prettyNameQ()
                               << " may only be assigned in the enclosing class's 'new' method "
                                  "(IEEE 1800-2023 19.4).");
            }
        }
        iterateChildrenConst(nodep);
    }
    ಶೂನ್ಯ visit(Ast_ಗಂಟು_Var_Ref* nodep) override {
        // Type options may be set at any time (IEEE 1800-2023 19.7.1), so SystemVerilog may set
        // type_option.merge_instances with any use of a type_option but a read of another
        // member, as not every write is marked yet (std::randomize)
        ಯದಿ (m_collecting && nodep->varp()->name() == "type_option") {
            ಧ್ರುವ AstStructSel* ಧ್ರುವ selp = VN_CAST(nodep->backp(), StructSel);
            ಯದಿ (!selp || selp->fromp() != nodep || selp->name() == "merge_instances"
                || !nodep->access().isReadOnly()) {
                m_mergeableTypeOptions.ಸೇರಿಸು(nodep->varp());
            }
        }
        iterateChildrenConst(nodep);
    }
    ಶೂನ್ಯ visit(Ast_ಗಂಟು* nodep) override { iterateChildrenConst(nodep); }

ಸಾರ್ವಜನಿಕ:
    explicit CovergroupAssignValidVisitor(AstNetlist* nodep) {
        // Uses can precede their enclosing class in the tree.
        iterateConst(nodep);
        m_collecting = ಅಸತ್ಯ;
        ಯದಿ (!m_constructors.empty()) iterateConst(nodep);
    }
    ತಾರ್ಕಿಕ valid() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_valid; }
    // type_option variables through which SystemVerilog may set type_option.merge_instances, so
    // that covergroups are tested with optionVar(true)
    ಧ್ರುವ std::set<ಧ್ರುವ AstVar*>& mergeableTypeOptions() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_mergeableTypeOptions; }
};

//######################################################################
// Covergroup expression validation visitor

ವರ್ಗ Covergroup_ಅಭಿವ್ಯಕ್ತಿ_Valid_Visitor final : ಸಾರ್ವಜನಿಕ VNVisitor {
    ಧ್ರುವ std::set<ಧ್ರುವ AstVar*>& m_sampleMembers;
    ಧ್ರುವ std::set<ಧ್ರುವ AstVar*>& m_constructorRefMembers;
    ತಾರ್ಕಿಕ m_inCoverageExpression = ಅಸತ್ಯ;
    ತಾರ್ಕಿಕ m_sampleFormalAllowed = ಅಸತ್ಯ;

    ಶೂನ್ಯ scanSampleExpression(Ast_ಗಂಟು* nodep) {
        ಯದಿ (!nodep) ಪ್ರತ್ಯಾಗಮ;
        VL_RESTORER(m_sampleFormalAllowed);
        m_sampleFormalAllowed = ಸತ್ಯ;
        iterateAndNextNull(nodep);
    }

    ಶೂನ್ಯ scanCoverageExpression(Ast_ಗಂಟು* nodep) {
        ಯದಿ (!nodep) ಪ್ರತ್ಯಾಗಮ;
        VL_RESTORER(m_inCoverageExpression);
        m_inCoverageExpression = ಸತ್ಯ;
        iterateAndNextNull(nodep);
    }

    ಶೂನ್ಯ visit(AstCoverpoint* nodep) override {
        scanSampleExpression(nodep->exprp());
        scanSampleExpression(nodep->iffp());
        iterateAndNextNull(nodep->binsp());
        iterateAndNextNull(nodep->optionsp());
    }
    ಶೂನ್ಯ visit(AstCoverCross* nodep) override {
        iterateAndNextNull(nodep->itemsp());
        scanSampleExpression(nodep->iffp());
        iterateAndNextNull(nodep->optionsp());
        iterateAndNextNull(nodep->binsp());
    }
    ಶೂನ್ಯ visit(AstCoverCrossBin* nodep) override {
        iterateAndNextNull(nodep->selectp());
        scanSampleExpression(nodep->iffp());
    }
    ಶೂನ್ಯ visit(AstCoverCrossWith* nodep) override {
        iterateAndNextNull(nodep->selectp());
        scanCoverageExpression(nodep->filterp());
        scanCoverageExpression(nodep->countp());
    }
    ಶೂನ್ಯ visit(AstCoverBinsof* nodep) override { scanCoverageExpression(nodep->rangesp()); }
    ಶೂನ್ಯ visit(AstCoverBin* nodep) override {
        scanCoverageExpression(nodep->rangesp());
        scanSampleExpression(nodep->iffp());
        scanCoverageExpression(nodep->arraySizep());
        scanCoverageExpression(nodep->transp());
    }
    ಶೂನ್ಯ visit(AstVarRef* nodep) override {
        ಯದಿ (!m_sampleFormalAllowed && m_sampleMembers.ಎಣಿಕೆ(nodep->varp())) {
            nodep->v3error("Covergroup sample formal argument "
                           << nodep->varp()->prettyNameQ()
                           << " may only be used in a coverpoint or conditional guard "
                              "expression (IEEE 1800-2023 19.8.1).");
        }
        ಯದಿ (m_inCoverageExpression && m_constructorRefMembers.ಎಣಿಕೆ(nodep->varp())) {
            nodep->v3error("Ref covergroup constructor formal argument "
                           << nodep->varp()->prettyNameQ()
                           << " may not be used in a covergroup expression "
                              "(IEEE 1800-2023 19.5).");
        }
    }
    ಶೂನ್ಯ visit(Ast_ಗಂಟು_FTask_Ref* nodep) override {
        ಯದಿ (m_inCoverageExpression && nodep->taskp()) {
            ತಾರ್ಕಿಕ invalidDirection = ಅಸತ್ಯ;
            ಪ್ರತಿ (Ast_ಗಂಟು* stmtp = nodep->taskp()->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
                ಧ್ರುವ AstVar* ಧ್ರುವ varp = VN_CAST(stmtp, Var);
                ಯದಿ (varp && varp->isIO() && varp->isWritable()) {
                    invalidDirection = ಸತ್ಯ;
                    ಭಙ್ಗ;
                }
            }
            ಯದಿ (invalidDirection) {
                nodep->v3error("Function " << nodep->taskp()->prettyNameQ()
                                           << " called in a covergroup expression has an "
                                              "output, inout, or non-const ref argument "
                                              "(IEEE 1800-2023 19.5).");
            }
        }
        iterateChildren(nodep);
    }
    ಶೂನ್ಯ visit(Ast_ಗಂಟು* nodep) override { iterateChildren(nodep); }

ಸಾರ್ವಜನಿಕ:
    Covergroup_ಅಭಿವ್ಯಕ್ತಿ_Valid_Visitor(ಧ್ರುವ std::set<ಧ್ರುವ AstVar*>& sampleMembers,
                               ಧ್ರುವ std::set<ಧ್ರುವ AstVar*>& constructorRefMembers)
        : m_sampleMembers{sampleMembers}
        , m_constructorRefMembers{constructorRefMembers} {}
    ಶೂನ್ಯ scan(Ast_ಗಂಟು* nodep) { iterate(nodep); }
};

//######################################################################
// Bins of one declaration whose values are computed rather than listed: bin k covers
// [m_lo + k * m_stride, m_lo + (k + 1) * m_stride - 1], and the last bin extends to m_hi.  An
// array bin element is a run of single-value bins; automatic bins partition the coverpoint
// domain.  Bounds are coverpoint values at FunctionalCoverageVisitor::runWidth(),
// sign-extended like a CrossValueRange's.

ವರ್ಗ BinRun final {
ಸಾರ್ವಜನಿಕ:
    // MEMBERS
    uint32_t m_ಎಣಿಕೆ;  // Number of bins
    V3_ಸಂಖ್ಯೆ m_lo;  // Lowest value of the first bin
    V3_ಸಂಖ್ಯೆ m_stride;  // Number of values of each bin but the last
    V3_ಸಂಖ್ಯೆ m_hi;  // Highest value of the last bin
    ತಾರ್ಕಿಕ m_empty = ಅಸತ್ಯ;  // A single bin without a value of the coverpoint type
    uint32_t m_declared = 0;  // Runtime index of the first bin, once generated

    // CONSTRUCTORS
    BinRun(Ast_ಗಂಟು* nodep, ಪೂರ್ಣಾಙ್ಕ width, uint32_t ಎಣಿಕೆ)
        : m_ಎಣಿಕೆ{ಎಣಿಕೆ}
        , m_lo{nodep, width}
        , m_stride{nodep, width, 1}
        , m_hi{nodep, width} {}
};

//######################################################################
// Functional coverage visitor

ವರ್ಗ FunctionalCoverageVisitor final : ಸಾರ್ವಜನಿಕ VNVisitor {
    // NODE STATE
    // Entire netlist:
    //  AstCoverpoint::user1p()  -> AstVar*.  Previous-value variable for transition bins
    //  AstCoverpoint::user2()   -> bool.  Had a bins declaration ignored, so no automatic bins
    ಧ್ರುವ VNUser1InUse m_inuser1;
    ಧ್ರುವ VNUser2InUse m_inuser2;

    // STATE
    std::set<AstCoverpoint*>
        m_runtimePoints;  // Points needing value metadata and live-bin mapping
    std::set<AstCoverCross*> m_runtimeCrosses;  // Crosses over finalized live-bin dimensions
    std::map<AstVar*, AstVar*> m_excludedVars;  // Sample-time state-exclusion flags
    AstClass* m_covergroupp = nullptr;  // Current covergroup being processed
    Ast_ಗಂಟು_Module* m_unitp = nullptr;  // Design unit declaring the current class, outside classes
    AstClass* m_enclosingClassp = nullptr;  // Class lexically enclosing the covergroup, if any
    AstVar* m_embeddedVarp = nullptr;  // Embedded covergroup member of m_enclosingClassp, if any
    std::string m_covergroupName;  // Current covergroup's type name, see covergroupTypeName()
    AstFunc* m_sampleFuncp = nullptr;  // Current sample() function
    AstIf* m_cpGuardp = nullptr;  // Current coverpoint's iff guard, holding its sampling
    AstFunc* m_constructorp = nullptr;  // Current constructor
    std::vector<AstCoverpoint*> m_coverpoints;  // Coverpoints in current covergroup
    std::map<std::string, AstCoverpoint*> m_coverpointMap;  // Name -> coverpoint for fast lookup
    std::vector<AstCoverCross*> m_coverCrosses;  // Cross coverage items in current covergroup
    std::vector<AstCgOptionAssign*> m_cgOptions;  // Covergroup-level options, before lowering
    uint32_t m_cgTypeWeight = 1;  // The covergroup's type_option.weight, a constant
    ತಾರ್ಕಿಕ m_cgMergeInstances = ಅಸತ್ಯ;  // The covergroup's type_option.merge_instances, a constant
    // The covergroup's type_option.merge_instances is, or SystemVerilog may make it, true
    ತಾರ್ಕಿಕ m_cgMayMerge = ಅಸತ್ಯ;
    // type_option members through which SystemVerilog may set type_option.merge_instances,
    // see CovergroupAssignValidVisitor::mergeableTypeOptions()
    ಧ್ರುವ std::set<ಧ್ರುವ AstVar*>& m_mergeableTypeOptions;
    ಪೂರ್ಣಾಙ್ಕ m_cgAtLeast = 1;  // Covergroup option.at_least

    ಸಂರಚನಾ EmbeddedEventTrigger final {
        File_ಸಾಲು* eventFl;  // Clocking-event source location
        AstVar* baseVarp;  // Base enclosing-class member in the event expression
        AstVar* memberVarp;  // Selected member in a 'base.member' expression, or nullptr
        VEdgeType edgeType;  // Clocking-event edge qualifier
        AstVar* prevVarp;  // Member containing the previous event value, or nullptr
        EmbeddedEventTrigger(File_ಸಾಲು* eventFl, AstVar* baseVarp, AstVar* memberVarp,
                             VEdgeType edgeType)
            : eventFl{eventFl}
            , baseVarp{baseVarp}
            , memberVarp{memberVarp}
            , edgeType{edgeType}
            , prevVarp{nullptr} {}
    };

    std::set<std::string> m_crossedCpNames;  // Coverpoints referenced by a cross
    std::map<std::string, AstVar*> m_cpVarMap;  // Coverpoint name -> its VlCoverpoint member
    ಸಂರಚನಾ BinRuns final {
        std::vector<BinRun> runs;  // Runs of an array or automatic bins declaration, in order
        uint32_t ಎಣಿಕೆ = 0;  // Bins across all runs
        ತಾರ್ಕಿಕ unsupported = ಅಸತ್ಯ;  // Too many bins, or invalid: the declaration is ignored
        // The value of each bin, which names it, of a wildcard array; else the bins are indexed
        std::vector<std::string> ಮೌಲ್ಯಗಳು;
        // Too many values of an ignore or illegal wildcard array, which is then one bin
        ತಾರ್ಕಿಕ single = ಅಸತ್ಯ;
    };
    ಸಂರಚನಾ Cross_Bin_ಮೌಲ್ಯಗಳು final {
        AstCoverBin* binp;  // Declaration owning this Normal bin
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* valuep;  // Individual array-bin value, or nullptr for a scalar bin
        ಧ್ರುವ BinRun* runp = nullptr;  // Run computing the bin's values, if any
        uint32_t element = 0;  // Index of the bin within runp
    };
    ಸಂರಚನಾ BinSpan final {
        uint32_t first = 0;  // First Normal index of the bin declaration
        uint32_t ಎಣಿಕೆ = 0;  // Number of Normal bins of the declaration
        uint32_t declared = 0;  // First runtime bin index, across all bin kinds
        int32_t sized = -1;  // Index of the sized array, placed at construction; or -1
    };
    ಸಂರಚನಾ CoverpointBins final {
        uint32_t total = 0;  // Number of Normal bins
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* exprp = nullptr;  // Sampled expression, for the value domain
        ತಾರ್ಕಿಕ crossed = ಅಸತ್ಯ;  // Feeds a cross, which needs 'values'
        std::vector<Cross_Bin_ಮೌಲ್ಯಗಳು> ಮೌಲ್ಯಗಳು;  // Values in runtime Normal-bin index order
        std::unordered_map<std::string, BinSpan> spans;  // Declared bin name -> index span
        BinSpan implicitAuto{0, 0, 0};  // Implicit automatic bins, each named 'auto_<i>'
        std::deque<BinRun> runs;  // Runs 'values' refers to
    };
    std::map<AstVar*, CoverpointBins> m_cpBins;  // Runtime coverpoint -> binsof index ranges
    // Names of the bins declarations each coverpoint ignored, which binsof selects as no bins
    std::map<ಧ್ರುವ AstCoverpoint*, std::vector<std::string>> m_droppedBins;
    // Prefixes of the constructor temporaries of constructed bins, as coverpoint and bin names
    // alone may repeat: coverpoint 'a_' bins 'b', and coverpoint 'a' bins '_b'
    V3UniqueNames m_sizedNames{"__Vsized"};
    std::vector<Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ*> m_detached_ಮೌಲ್ಯಗಳು;  // Array-bin values m_cpBins refers to
    std::set<AstCoverCross*>
        m_droppedCrosses;  // Crosses with a bare-variable item: drop (COVERIGN)
    std::map<uint32_t, AstCoverpointDType*> m_cpDTypes;  // Hit-list bound -> interned dtype
    using CrossShape = std::tuple<uint32_t, uint32_t, uint32_t, uint32_t, uint64_t, ತಾರ್ಕಿಕ>;
    std::map<CrossShape, AstCoverCrossDType*> m_cxDTypes;
    AstVar* m_cgInstVarp = nullptr;  // __Vcg_inst handle member of the current covergroup

    VMemberMap m_memberMap;  // Member names cached for fast lookup

    // METHODS
    // The covergroup's 'option' or static 'type_option' member (V3LinkParse creates both)
    AstVar* optionVar(ತಾರ್ಕಿಕ typeOption) {
        AstVar* ಧ್ರುವ varp = VN_AS(
            m_memberMap.findMember(m_covergroupp, typeOption ? "type_option" : "option"), Var);
        UASSERT_OBJ(varp, m_covergroupp, "Covergroup missing option member");
        ಪ್ರತ್ಯಾಗಮ varp;
    }

    // 'option.<member>' or 'type_option.<member>', per optionVarp
    AstStructSel* newOptionSel(File_ಸಾಲು* fl, AstVar* optionVarp, ಧ್ರುವ std::string& member,
                               VAccess access) {
        ಧ್ರುವ AstMemberDType* ಧ್ರುವ memberp
            = VN_AS(m_memberMap.findMember(optionVarp->dtypep()->skipRefp(), member), MemberDType);
        UASSERT_OBJ(memberp, optionVarp, "Coverage option structure missing '" << member << "'");
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ fromp = optionVarp->lifetime().isStatic()
                                       ? ನೂತನ AstVarRef{fl, optionVarp, access}
                                       : memberRef(fl, optionVarp, access);
        AstStructSel* ಧ್ರುವ selp = ನೂತನ AstStructSel{fl, fromp, member};
        selp->dtypep(memberp->subDTypep()->skipRefToEnump());
        selp->didWidth(ಸತ್ಯ);
        ಪ್ರತ್ಯಾಗಮ selp;
    }

    // A constant option.at_least; a negative count of hits has no meaning, so takes the default
    ಸ್ಥಿರ ಪೂರ್ಣಾಙ್ಕ constAtLeast(ಧ್ರುವ AstConst* constp) {
        ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ ಮೌಲ್ಯ = constp->toSInt();
        ಯದಿ (ಮೌಲ್ಯ >= 0) ಪ್ರತ್ಯಾಗಮ ಮೌಲ್ಯ;
        constp->v3warn(COVERIGN, "Ignoring negative 'option.at_least'; using 1");
        ಪ್ರತ್ಯಾಗಮ 1;
    }

    // An option's constant value, or nullptr after a warning, as non-constant is unsupported
    ಸ್ಥಿರ ಧ್ರುವ AstConst* constOption(ಧ್ರುವ Ast_ಗಂಟು* valuep, VCoverOptionType optType) {
        ಯದಿ (ಧ್ರುವ AstConst* ಧ್ರುವ constp = VN_CAST(valuep, Const)) ಪ್ರತ್ಯಾಗಮ constp;
        valuep->v3warn(COVERIGN, "Ignoring unsupported: non-constant 'option."
                                     << optType.ascii() << "'; using default value");
        ಪ್ರತ್ಯಾಗಮ nullptr;
    }

    // Store the covergroup-level options (IEEE 1800-2023 19.7) where SystemVerilog and the
    // runtime read them.  The instance options, option.weight and option.get_inst_coverage,
    // are evaluated by the constructor, as are the other instance options; the type options,
    // type_option.weight and type_option.merge_instances, are constant, and initialize the
    // static member.  Without its own type_option.merge_instances, a covergroup has the
    // default of --coverage-merge-instances.  A constant option.at_least is the default of
    // the coverpoints and crosses.
    ಶೂನ್ಯ lowerCovergroupOptions() {
        ತಾರ್ಕಿಕ mergeSet = ಅಸತ್ಯ;  // The covergroup sets type_option.merge_instances
        m_cgAtLeast = 1;
        ಪ್ರತಿ (AstCgOptionAssign* ಧ್ರುವ optp : m_cgOptions) {
            ಯದಿ (optp->optType() == VCoverOptionType::AT_LEAST) {
                ಯದಿ (ಧ್ರುವ AstConst* ಧ್ರುವ constp = constOption(optp->valuep(), optp->optType())) {
                    m_cgAtLeast = constAtLeast(constp);
                }
                VL_DO_DANGLING(ತಳ್ಳು_Deletep(optp->unlinkFrBack()), optp);
                ಅನುವರ್ತ;
            }
            File_ಸಾಲು* ಧ್ರುವ fl = optp->fileline();
            // V3Width left the type options constant, and type_option.weight non-negative
            ಧ್ರುವ AstConst* ಧ್ರುವ constp = VN_CAST(optp->valuep(), Const);
            std::string member = "weight";
            ಯದಿ (optp->optType() == VCoverOptionType::MERGE_INSTANCES) {
                member = "merge_instances";
                m_cgMergeInstances = !constp->ಅಂಕೆ().isEqZero();
                mergeSet = ಸತ್ಯ;
            } ಅನ್ಯಥಾ ಯದಿ (optp->optType() == VCoverOptionType::GET_INST_COVERAGE) {
                member = "get_inst_coverage";
            } ಅನ್ಯಥಾ {
                UASSERT_OBJ(optp->optType() == VCoverOptionType::WEIGHT, optp,
                            "Unexpected covergroup option reaching V3Covergroup");
                ಯದಿ (optp->typeOption()) m_cgTypeWeight = constp->toUInt();
            }
            AstAssign* ಧ್ರುವ assignp = ನೂತನ AstAssign{
                fl, newOptionSel(fl, optionVar(optp->typeOption()), member, VAccess::WRITE),
                optp->valuep()->unlinkFrBack()};
            ಯದಿ (optp->typeOption()) {
                m_covergroupp->addMembersp(ನೂತನ AstInitialStatic{fl, assignp});
                VL_DO_DANGLING(ತಳ್ಳು_Deletep(optp->unlinkFrBack()), optp);
            } ಅನ್ಯಥಾ {
                optp->replaceWith(assignp);
                VL_DO_DANGLING(ತಳ್ಳು_Deletep(optp), optp);
            }
        }
        m_cgOptions.clear();
        ಯದಿ (!mergeSet && v3Global.opt.coverageMergeInstances()) {
            File_ಸಾಲು* ಧ್ರುವ fl = m_covergroupp->fileline();
            m_covergroupp->addMembersp(ನೂತನ AstInitialStatic{
                fl, ನೂತನ AstAssign{
                        fl, newOptionSel(fl, optionVar(ಸತ್ಯ), "merge_instances", VAccess::WRITE),
                        ನೂತನ AstConst{fl, AstConst::BitTrue{}}}});
            m_cgMergeInstances = ಸತ್ಯ;
        }
        m_cgMayMerge = m_cgMergeInstances || m_mergeableTypeOptions.ಎಣಿಕೆ(optionVar(ಸತ್ಯ));
    }

    // The weight of an item in the coverage database, which merges the instances: its
    // type_option.weight if the covergroup merges them too (IEEE 1800-2023 19.11.3); else its
    // option.weight if a constant, and so of every instance; else its type_option.weight
    uint32_t itemDatabaseWeight(Ast_ಗಂಟು* optionsp) ಧ್ರುವ {
        ಧ್ರುವ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* weightp = nullptr;  // The option.weight in effect
        uint32_t typeWeight = 1;
        ಪ್ರತಿ (Ast_ಗಂಟು* nodep = optionsp; nodep; nodep = nodep->nextp()) {
            ಧ್ರುವ AstCoverOption* ಧ್ರುವ optp = VN_AS(nodep, CoverOption);
            ಯದಿ (!(optp->optType() == VCoverOptionType::WEIGHT)) ಅನುವರ್ತ;
            // V3Width left type_option.weight a non-negative constant
            ಯದಿ (optp->typeOption()) {
                typeWeight = VN_AS(optp->valuep(), Const)->toUInt();
            } ಅನ್ಯಥಾ {
                weightp = optp->valuep();
            }
        }
        ಯದಿ (m_cgMergeInstances) ಪ್ರತ್ಯಾಗಮ typeWeight;
        ಯದಿ (!weightp) ಪ್ರತ್ಯಾಗಮ 1;
        ಯದಿ (ಧ್ರುವ AstConst* ಧ್ರುವ constp = VN_CAST(weightp, Const)) ಪ್ರತ್ಯಾಗಮ constp->toUInt();
        ಪ್ರತ್ಯಾಗಮ typeWeight;
    }

    // Configure an item's option.weight, its weight in instance coverage (IEEE 1800-2023
    // 19.11), and, if the covergroup may merge its instances, its type_option.weight, its
    // weight in type coverage then (19.11.3).
    ಶೂನ್ಯ generateItemWeight(File_ಸಾಲು* fl, AstVar* itemVarp, Ast_ಗಂಟು* optionsp) {
        ಪ್ರತಿ (Ast_ಗಂಟು* nodep = optionsp; nodep; nodep = nodep->nextp()) {
            ಧ್ರುವ AstCoverOption* ಧ್ರುವ optp = VN_AS(nodep, CoverOption);
            ಯದಿ (!(optp->optType() == VCoverOptionType::WEIGHT)) ಅನುವರ್ತ;
            ಯದಿ (!optp->typeOption()) {
                m_constructorp->addStmtsp(
                    itemCall(fl, itemVarp, VCMethod::COVERGROUP_WEIGHT,
                             {optp->valuep()->clone_ವೃಕ್ಷ(ಅಸತ್ಯ), file_ಸಾಲು_Debug(optp->fileline())})
                        ->makeStmt());
            } ಅನ್ಯಥಾ ಯದಿ (m_cgMayMerge) {
                // V3Width left type_option.weight a non-negative constant
                m_constructorp->addStmtsp(itemCall(fl, itemVarp, VCMethod::COVERGROUP_TYPE_WEIGHT,
                                                   {optp->valuep()->clone_ವೃಕ್ಷ(ಅಸತ್ಯ)})
                                              ->makeStmt());
            }
        }
    }

    ಶೂನ್ಯ processCovergroup() {
        UINFO(4, "Processing covergroup: " << m_covergroupp->name() << " with "
                                           << m_coverpoints.size() << " coverpoints and "
                                           << m_coverCrosses.size() << " crosses");

        m_crossedCpNames.clear();
        m_cpVarMap.clear();
        m_cpBins.clear();
        m_droppedBins.clear();
        m_sizedNames.reset();
        m_runtimePoints.clear();
        m_runtimeCrosses.clear();
        m_excludedVars.clear();
        m_droppedCrosses.clear();
        m_cgInstVarp = nullptr;
        m_cgTypeWeight = 1;
        m_cgMergeInstances = ಅಸತ್ಯ;
        m_cgMayMerge = ಅಸತ್ಯ;

        lowerCovergroupOptions();

        // Scan every cross item to record the coverpoints it references (the cross dimensions)
        // and to flag any cross naming a bare variable -- a would-be implicit coverpoint, which
        // Verilator does not synthesize.  An unresolvable item drops only that one cross (with a
        // COVERIGN in generateCrossCode), leaving the rest of the covergroup intact.
        ಪ್ರತಿ (AstCoverCross* crossp : m_coverCrosses) {
            ಪ್ರತಿ (Ast_ಗಂಟು* itemp = crossp->itemsp(); itemp; itemp = itemp->nextp()) {
                ಧ್ರುವ AstCoverpointRef* ಧ್ರುವ refp = VN_AS(itemp, CoverpointRef);
                ಯದಿ (refp->exprp()) ಅನುವರ್ತ;  // hierarchical ref: dropped in generateCrossCode
                ಯದಿ (m_coverpointMap.find(refp->name()) == m_coverpointMap.end()) {
                    m_droppedCrosses.ಸೇರಿಸು(crossp);  // bare variable: drop this cross only
                } ಅನ್ಯಥಾ {
                    m_crossedCpNames.ಸೇರಿಸು(refp->name());
                }
            }
        }

        std::vector<Ast_ಗಂಟು*> pending;
        std::map<AstCoverpoint*, std::vector<AstCoverCross*>> consumers;
        std::map<AstCoverCross*, std::vector<AstCoverpoint*>> inputs;
        ಪ್ರತಿ (AstCoverpoint* ಧ್ರುವ cpp : m_coverpoints) {
            checkBinNames(cpp);
            checkConstructedBins(cpp);
            ಯದಿ (!cpp->exprp()->dtypep()->skipRefp()->isIntegralOrPacked()) ಅನುವರ್ತ;
            // Bins without values leave the report (IEEE 1800-2023 19.11.1), exclusions or not.
            // Constructed bins get their values when the covergroup is constructed.
            ಯದಿ (!coverpointHasStateExclusions(cpp) && !coverpointHasEmptyBins(cpp)
                && !coverpointHasConstructedBins(cpp)) {
                ಅನುವರ್ತ;
            }
            m_runtimePoints.ಸೇರಿಸು(cpp);
            pending.ತಳ್ಳು_back(cpp);
        }
        ಪ್ರತಿ (AstCoverCross* ಧ್ರುವ crossp : m_coverCrosses) {
            ಯದಿ (m_droppedCrosses.ಎಣಿಕೆ(crossp)) ಅನುವರ್ತ;
            ಪ್ರತಿ (Ast_ಗಂಟು* itemp = crossp->itemsp(); itemp; itemp = itemp->nextp()) {
                ಧ್ರುವ AstCoverpointRef* ಧ್ರುವ refp = VN_AS(itemp, CoverpointRef);
                ಯದಿ (refp->exprp()) ಅನುವರ್ತ;
                ಧ್ರುವ auto point = m_coverpointMap.find(refp->name());
                ಯದಿ (point != m_coverpointMap.end()) {
                    consumers[point->second].ತಳ್ಳು_back(crossp);
                    inputs[crossp].ತಳ್ಳು_back(point->second);
                }
            }
            // The values of a filter's bin tuples are known when the covergroup is constructed
            ಯದಿ (crossp->exists([](ಧ್ರುವ AstCoverCrossWith*) { ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ; })) {
                m_runtimeCrosses.emplace(crossp);
                pending.ತಳ್ಳು_back(crossp);
            }
        }
        ಪ್ರತಿ (size_t next = 0; next < pending.size(); ++next) {
            ಯದಿ (AstCoverpoint* ಧ್ರುವ pointp = VN_CAST(pending[next], Coverpoint)) {
                ಪ್ರತಿ (AstCoverCross* ಧ್ರುವ crossp : consumers[pointp]) {
                    ಯದಿ (m_runtimeCrosses.emplace(crossp).second) pending.ತಳ್ಳು_back(crossp);
                }
            } ಅನ್ಯಥಾ {
                ಪ್ರತಿ (AstCoverpoint* ಧ್ರುವ pointp : inputs[VN_AS(pending[next], CoverCross)]) {
                    ಯದಿ (pointp->exprp()->dtypep()->skipRefp()->isIntegralOrPacked()
                        && m_runtimePoints.emplace(pointp).second) {
                        pending.ತಳ್ಳು_back(pointp);
                    }
                }
            }
        }

        // The instance node owns this instance's coverpoint/cross runtimes, so it must exist
        // before any of them is created.  Emitted first, ahead of both generate loops.
        generateInstanceAttach();

        // For each coverpoint, generate sampling code
        ಪ್ರತಿ (AstCoverpoint* cpp : m_coverpoints) generateCoverpointCode(cpp);

        // For each cross, generate sampling code
        ಪ್ರತಿ (AstCoverCross* crossp : m_coverCrosses) generateCrossCode(crossp);

        // Every cross has been built, so runtime points only need their exclusions from here.
        ಪ್ರತಿ (AstCoverpoint* ಧ್ರುವ cpp : m_coverpoints) {
            ಯದಿ (!m_runtimePoints.ಎಣಿಕೆ(cpp)) ಅನುವರ್ತ;
            m_constructorp->addStmtsp(itemCall(cpp->fileline(), m_cpVarMap.at(cpp->name()),
                                               VCMethod::COVERGROUP_ಮೌಲ್ಯ_RELEASE)
                                          ->makeStmt());
        }
        ಪ್ರತಿ (Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* valuep : m_detached_ಮೌಲ್ಯಗಳು) VL_DO_DANGLING(ತಳ್ಳು_Deletep(valuep), valuep);
        m_detached_ಮೌಲ್ಯಗಳು.clear();

        // Generate coverage computation code (even for empty covergroups).  Bin registration
        // with the coverage database is handled per coverpoint/cross by their runtime
        // registerBins() calls (emitted in generateCoverpoint/generateCross).
        generateCoverageComputationCode();
    }

    ಸ್ಥಿರ constexpr size_t ಮೌಲ್ಯ_LIST_ENTRIES = 256;  // Metadata entries per constructor call

    // The value of a constant count, of bins or value tuples: -1 if it is negative, and
    // saturated above the largest limit
    ಸ್ಥಿರ int64_t const_ಎಣಿಕೆ(ಧ್ರುವ AstConst* constp) {
        ಧ್ರುವ V3_ಸಂಖ್ಯೆ& ಅಂಕೆ = constp->ಅಂಕೆ();
        ಯದಿ (constp->isSigned() && ಅಂಕೆ.isNegative()) ಪ್ರತ್ಯಾಗಮ -1;
        ಪ್ರತ್ಯಾಗಮ ಅಂಕೆ.mostSetBitP1() > 32 ? INT64_MAX : static_cast<int64_t>(ಅಂಕೆ.toUQuad());
    }

    // Whether a count, 'whatp', is integral and, if constant, positive, as the standard's clause
    // 'clausep' requires; else errors
    ಸ್ಥಿರ ತಾರ್ಕಿಕ check_ಎಣಿಕೆ(ಧ್ರುವ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* countp, ಧ್ರುವ ವರ್ಣ* whatp, ಧ್ರುವ ವರ್ಣ* clausep) {
        ಧ್ರುವ AstConst* ಧ್ರುವ constp = VN_CAST(countp, Const);
        ಯದಿ (!countp->dtypep()->skipRefp()->isIntegralOrPacked()) {
            countp->v3error(whatp << " must be integral (IEEE 1800-2023 " << clausep << ")");
            ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
        } ಅನ್ಯಥಾ ಯದಿ (constp && (constp->ಅಂಕೆ().isFourState() || const_ಎಣಿಕೆ(constp) < 1)) {
            countp->v3error(whatp << " must be >= 1, got "
                                  << (constp->ಅಂಕೆ().isFourState() ? constp->ಅಂಕೆ().ascii(ಅಸತ್ಯ)
                                                                  : constp->ಅಂಕೆ().toDecimalS())
                                  << " (IEEE 1800-2023 " << clausep << ")");
            ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
        }
        ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;
    }

    // The number of bins requested by a valid 'bins auto[N]', or 0
    ಸ್ಥಿರ uint32_t autoBinsRequested(ಧ್ರುವ AstCoverBin* binp) {
        ಧ್ರುವ AstConst* ಧ್ರುವ constp = VN_CAST(binp->arraySizep(), Const);
        ಯದಿ (!constp) ಪ್ರತ್ಯಾಗಮ 0;
        ಧ್ರುವ int64_t ಎಣಿಕೆ = const_ಎಣಿಕೆ(constp);
        ಪ್ರತ್ಯಾಗಮ ಎಣಿಕೆ < 1 || ಎಣಿಕೆ > v3Global.opt.coverageMaxBins() ? 0
                                                                   : static_cast<uint32_t>(ಎಣಿಕೆ);
    }

    // True for a 'bins auto[N]' declaration, or the implicit automatic bins of a coverpoint
    ಸ್ಥಿರ ತಾರ್ಕಿಕ isAutoBins(ಧ್ರುವ AstCoverBin* binp) {
        ಪ್ರತ್ಯಾಗಮ binp->binsType() == VCoverBinsType::BINS_AUTO
               || binp->binsType() == VCoverBinsType::BINS_AUTO_IMPLICIT;
    }

    // True for a sized array of bins, 'bins b[N] = {...}', whose values an integral coverpoint
    // distributes over N bins when the covergroup is constructed (IEEE 1800-2023 19.5.1)
    ಸ್ಥಿರ ತಾರ್ಕಿಕ isSizedArray(ಧ್ರುವ AstCoverBin* binp) {
        ಪ್ರತ್ಯಾಗಮ binp->arraySizep() && !isAutoBins(binp);
    }

    // The 'with' filter of a bin, if any (IEEE 1800-2023 19.5.1.1)
    ಸ್ಥಿರ AstCoverWith* binWith(ಧ್ರುವ AstCoverBin* binp) {
        ಪ್ರತ್ಯಾಗಮ VN_CAST(binp->rangesp(), CoverWith);
    }

    // True for bins whose values the covergroup constructor computes: a sized array, or bins of
    // the values a 'with' filter keeps
    ಸ್ಥಿರ ತಾರ್ಕಿಕ isConstructedBins(ಧ್ರುವ AstCoverBin* binp) {
        ಪ್ರತ್ಯಾಗಮ isSizedArray(binp) || binWith(binp);
    }

    // The range list of a bin's values, or of the candidates of its 'with' filter; null for all
    // of the coverpoint's values, which a filter of the coverpoint's name has
    ಸ್ಥಿರ Ast_ಗಂಟು* binRangesp(ಧ್ರುವ AstCoverBin* binp) {
        ಧ್ರುವ AstCoverWith* ಧ್ರುವ withp = binWith(binp);
        ಯದಿ (!withp) ಪ್ರತ್ಯಾಗಮ binp->rangesp();
        ಪ್ರತ್ಯಾಗಮ VN_IS(withp->subp(), CoverpointRef) ? nullptr : withp->subp();
    }

    // Report and delete a bins declaration of a name that another of its coverpoint has
    ಶೂನ್ಯ checkBinNames(AstCoverpoint* coverpointp) {
        std::set<std::string> names;
        ಪ್ರತಿ (Ast_ಗಂಟು* nodep = coverpointp->binsp(); nodep;) {
            AstCoverBin* ಧ್ರುವ binp = VN_AS(nodep, CoverBin);
            nodep = nodep->nextp();
            ಯದಿ (names.emplace(binp->name()).second) ಅನುವರ್ತ;
            binp->v3error("Duplicate bin " << binp->prettyNameQ() << " in coverpoint "
                                           << coverpointp->prettyNameQ()
                                           << " (IEEE 1800-2023 3.13)");
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(binp->unlinkFrBack()), binp);
        }
    }

    // Delete an ignored bins declaration, which binsof then selects as no bins
    ಶೂನ್ಯ dropBins(ಧ್ರುವ AstCoverpoint* coverpointp, AstCoverBin* binp) {
        m_droppedBins[coverpointp].ತಳ್ಳು_back(binp->name());
        VL_DO_DANGLING(ತಳ್ಳು_Deletep(binp->unlinkFrBack()), binp);
    }

    // Check the size of a sized array of bins, which drops an invalid array.  A real coverpoint's
    // are unsupported, and treated as arrays of a bin per value.  False unless it stays sized.
    ತಾರ್ಕಿಕ checkBinsArraySize(ಧ್ರುವ AstCoverpoint* coverpointp, AstCoverBin* binp, ತಾರ್ಕಿಕ integral) {
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ sizep = binp->arraySizep();
        ಯದಿ (VN_IS(sizep, Unbounded)) {  // A parameter of '$'; see bins_orBraE
            binp->v3error("Bins array size must be integral, not '$' (IEEE 1800-2023 19.5.1)");
        } ಅನ್ಯಥಾ ಯದಿ (check_ಎಣಿಕೆ(sizep, "Bins array size", "19.5.1")) {
            ಯದಿ (integral) ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;
            binp->v3warn(COVERIGN, "Unsupported: 'bins' explicit array size of a real "
                                   "coverpoint (treated as '[]')");
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(sizep->unlinkFrBack()), sizep);
            ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
        }
        dropBins(coverpointp, binp);
        ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
    }

    // Check the bins of a coverpoint whose values the constructor computes, dropping invalid
    // ones.  A wildcard array of too many ranges of values is ignored, or if ignore or illegal,
    // treated as one bin; filtering too many ranges of values ignores the bins.
    ಶೂನ್ಯ checkConstructedBins(AstCoverpoint* coverpointp) {
        ಧ್ರುವ ತಾರ್ಕಿಕ integral = coverpointp->exprp()->dtypep()->skipRefp()->isIntegralOrPacked();
        ಪ್ರತಿ (Ast_ಗಂಟು* nodep = coverpointp->binsp(); nodep;) {
            AstCoverBin* ಧ್ರುವ binp = VN_AS(nodep, CoverBin);
            nodep = nodep->nextp();
            ಯದಿ (!isConstructedBins(binp)) ಅನುವರ್ತ;
            ಯದಿ (isSizedArray(binp) && !checkBinsArraySize(coverpointp, binp, integral)) ಅನುವರ್ತ;
            ಯದಿ (!binp->isWildcard()
                || sizedWildcardRuns(binp, coverpointp->exprp())
                       <= v3Global.opt.coverageMaxBins()) {
                ಯದಿ (binWith(binp) && withCandidatesOver(binp, coverpointp->exprp())) {
                    binp->v3warn(COVERIGN, "Unsupported: 'with' filter of more than 2**32 "
                                           "candidate values; bin "
                                               << binp->prettyNameQ() << " ignored");
                    ಯದಿ (binp->binsType().binIsNormal()) coverpointp->user2(ಸತ್ಯ);
                    dropBins(coverpointp, binp);
                }
                ಅನುವರ್ತ;
            }
            // An ignore or illegal array still excludes or checks its values, as one bin
            ಧ್ರುವ ತಾರ್ಕಿಕ single = !binp->binsType().binIsNormal() && !binWith(binp);
            binp->v3warn(COVERIGN,
                         "Unsupported: " << (binWith(binp) ? "'with' filter of wildcard '"
                                                           : "sized wildcard array '")
                                         << binp->binsType().verilogKwd()
                                         << "' of more than --coverage-max-bins of "
                                         << v3Global.opt.coverageMaxBins()
                                         << " ranges of values; bin " << binp->prettyNameQ()
                                         << (single ? " treated as one bin" : " ignored") << "\n"
                                         << binp->warnMore()
                                         << "... Suggest a larger --coverage-max-bins");
            ಯದಿ (single) {
                Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ sizep = binp->arraySizep();
                VL_DO_DANGLING(ತಳ್ಳು_Deletep(sizep->unlinkFrBack()), sizep);
                binp->isArray(ಅಸತ್ಯ);
                ಅನುವರ್ತ;
            }
            ಯದಿ (binp->binsType().binIsNormal()) coverpointp->user2(ಸತ್ಯ);
            dropBins(coverpointp, binp);
        }
    }

    // True if a 'with' filter would be evaluated for more than 2**32 candidate values, known
    // now for the coverpoint's name, or for a range list of constants: each is evaluated once
    // but for an array 'b[N]', which keeps their order and duplicates (see withBegin())
    ಸ್ಥಿರ ತಾರ್ಕಿಕ withCandidatesOver(AstCoverBin* binp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* exprp) {
        ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ width = runWidth(exprp);
        std::vector<std::pair<V3_ಸಂಖ್ಯೆ, V3_ಸಂಖ್ಯೆ>> runs;
        ಯದಿ (!binRangesp(binp)) runs = coverpoint_ಮೌಲ್ಯಗಳು(binp, exprp);
        ಧ್ರುವ auto constant
            = [](ಧ್ರುವ Ast_ಗಂಟು* nodep) { ಪ್ರತ್ಯಾಗಮ VN_IS(nodep, Const) || VN_IS(nodep, Unbounded); };
        ಪ್ರತಿ (Ast_ಗಂಟು* rangep = binRangesp(binp); rangep; rangep = rangep->nextp()) {
            ಧ್ರುವ AstInsideRange* ಧ್ರುವ irp = VN_CAST(rangep, InsideRange);
            // Else known when constructed
            ಯದಿ (irp ? !constant(irp->lhsp()) || !constant(irp->rhsp()) : !VN_IS(rangep, Const)) {
                ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
            }
            Cross_ಮೌಲ್ಯ_Range range{rangep, resolveWidth(rangep, exprp)};
            ಯದಿ (!resolve_ಮೌಲ್ಯ(rangep, exprp, ಸತ್ಯ, binp->isWildcard(), range)
                || crossRangeEmpty(range)) {
                ಅನುವರ್ತ;
            }
            std::vector<std::pair<V3_ಸಂಖ್ಯೆ, V3_ಸಂಖ್ಯೆ>> found{{range.lo, range.hi}};
            ಯದಿ (range.wildcard) {  // checkConstructedBins bounded the runs
                found.clear();
                crossRangeRuns(range, v3Global.opt.coverageMaxBins(), found);
            }
            ಪ್ರತಿ (ಧ್ರುವ std::pair<V3_ಸಂಖ್ಯೆ, V3_ಸಂಖ್ಯೆ>& run : found) {
                runs.emplace_back(V3_ಸಂಖ್ಯೆ{rangep, width, run.first},
                                  V3_ಸಂಖ್ಯೆ{rangep, width, run.second});
            }
        }
        ಯದಿ (!isSizedArray(binp)) {  // The union of the values
            std::sort(runs.begin(), runs.end(), [](ಧ್ರುವ auto& lhs, ಧ್ರುವ auto& rhs) {
                ಪ್ರತ್ಯಾಗಮ cross_ಮೌಲ್ಯ_Less(lhs.first, rhs.first);
            });
            std::vector<std::pair<V3_ಸಂಖ್ಯೆ, V3_ಸಂಖ್ಯೆ>> merged;
            ಪ್ರತಿ (ಧ್ರುವ std::pair<V3_ಸಂಖ್ಯೆ, V3_ಸಂಖ್ಯೆ>& run : runs) {
                ಯದಿ (merged.empty() || cross_ಮೌಲ್ಯ_Less(merged.back().second, run.first)) {
                    merged.ತಳ್ಳು_back(run);
                } ಅನ್ಯಥಾ ಯದಿ (cross_ಮೌಲ್ಯ_Less(merged.back().second, run.second)) {
                    merged.back().second = run.second;
                }
            }
            runs = std::move(merged);
        }
        uint64_t ಎಣಿಕೆ = 0;
        ಪ್ರತಿ (ಧ್ರುವ std::pair<V3_ಸಂಖ್ಯೆ, V3_ಸಂಖ್ಯೆ>& run : runs) {
            V3_ಸಂಖ್ಯೆ span{exprp, width};
            span.opSub(run.second, run.first);
            ಯದಿ (span.mostSetBitP1() > 32) ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;  // More than 2**32 values
            ಎಣಿಕೆ += span.toUQuad() + 1;
            ಯದಿ (ಎಣಿಕೆ > (uint64_t{1} << 32)) ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;
        }
        ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
    }

    // The ranges of values the wildcard patterns of a sized wildcard array, or of a 'with'
    // filter's candidates, give, counted up to more than --coverage-max-bins
    ಸ್ಥಿರ size_t sizedWildcardRuns(ಧ್ರುವ AstCoverBin* binp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* exprp) {
        std::vector<std::pair<V3_ಸಂಖ್ಯೆ, V3_ಸಂಖ್ಯೆ>> runs;
        ಪ್ರತಿ (Ast_ಗಂಟು* rangep = binRangesp(binp); rangep; rangep = rangep->nextp()) {
            ಯದಿ (!VN_IS(rangep, Const)) ಅನುವರ್ತ;  // A range, or a value known at construction
            Cross_ಮೌಲ್ಯ_Range range{rangep, resolveWidth(rangep, exprp)};
            ಯದಿ (resolve_ಮೌಲ್ಯ(rangep, exprp, ಸತ್ಯ, ಸತ್ಯ, range) && !crossRangeEmpty(range)) {
                crossRangeRuns(range, v3Global.opt.coverageMaxBins(), runs);
            }
        }
        ಪ್ರತ್ಯಾಗಮ runs.size();
    }

    // Check the automatic bins declarations of a coverpoint.  Each stays one declaration, which
    // generates as a partition of the coverpoint domain (see autoBinRuns).
    ಶೂನ್ಯ checkAutomaticBins(AstCoverpoint* coverpointp, ಧ್ರುವ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* exprp) {
        ಪ್ರತಿ (Ast_ಗಂಟು* binp = coverpointp->binsp(); binp; binp = binp->nextp()) {
            AstCoverBin* ಧ್ರುವ cbinp = VN_AS(binp, CoverBin);
            ಯದಿ (cbinp->binsType() != VCoverBinsType::BINS_AUTO) ಅನುವರ್ತ;
            ಧ್ರುವ AstConst* ಧ್ರುವ constp = VN_CAST(cbinp->arraySizep(), Const);
            ಯದಿ (!constp) {
                cbinp->v3error("Automatic bins array size must be a constant");
            } ಅನ್ಯಥಾ ಯದಿ (const_ಎಣಿಕೆ(constp) < 1) {
                cbinp->v3error("Automatic bins array size must be >= 1, got "
                               << constp->ಅಂಕೆ().toDecimalS());
            } ಅನ್ಯಥಾ ಯದಿ (const_ಎಣಿಕೆ(constp) > v3Global.opt.coverageMaxBins()) {
                cbinp->v3error("Automatic bins array size of "
                               << constp->ಅಂಕೆ().toDecimalU() << " exceeds limit of "
                               << v3Global.opt.coverageMaxBins() << '\n'
                               << cbinp->warnMore() << "... Suggest a larger --coverage-max-bins");
            } ಅನ್ಯಥಾ ಯದಿ (!exprp->dtypep()->skipRefp()->isIntegralOrPacked()) {
                cbinp->v3error("Automatic bins are not allowed on a coverpoint of a non-integral "
                               "expression (IEEE 1800-2023 19.5.3).");
            }
        }
    }

    // Extract all option values of a coverpoint or cross in a single pass.
    // atLeastOut: option.at_least (default the covergroup's)
    // autoBinMaxOut: option.auto_bin_max (coverpoint overrides covergroup, default 64)
    ಶೂನ್ಯ extractItemOptions(Ast_ಗಂಟು* optionsp, ಪೂರ್ಣಾಙ್ಕ& atLeastOut, ಪೂರ್ಣಾಙ್ಕ& autoBinMaxOut) {
        atLeastOut = m_cgAtLeast;
        autoBinMaxOut = -1;  // -1 = not set at coverpoint level
        ಪ್ರತಿ (Ast_ಗಂಟು* optionp = optionsp; optionp; optionp = optionp->nextp()) {
            AstCoverOption* ಧ್ರುವ optp = VN_AS(optionp, CoverOption);
            // Weights may be non-constant; generateItemWeight() handles them.  A cross keeps
            // its unsupported options, which V3LinkParse warned of.
            ಯದಿ (!(optp->optType() == VCoverOptionType::AT_LEAST
                  || optp->optType() == VCoverOptionType::AUTO_BIN_MAX)) {
                ಅನುವರ್ತ;
            }
            ಧ್ರುವ AstConst* ಧ್ರುವ constp = constOption(optp->valuep(), optp->optType());
            ಯದಿ (!constp) ಅನುವರ್ತ;
            ಯದಿ (optp->optType() == VCoverOptionType::AT_LEAST) {
                atLeastOut = constAtLeast(constp);
            } ಅನ್ಯಥಾ {
                autoBinMaxOut = constp->toSInt();
            }
        }
        // Fall back to covergroup-level auto_bin_max if not set at coverpoint level
        ಯದಿ (autoBinMaxOut < 0) {
            ಯದಿ (m_covergroupp->cgAutoBinMax() >= 0) {
                autoBinMaxOut = m_covergroupp->cgAutoBinMax();
            } ಅನ್ಯಥಾ {
                autoBinMaxOut = 64;  // Default per IEEE 1800-2023 Table 19-1
            }
        }
    }

    // IEEE 1800-2023 19.5.2: an enum coverpoint has one automatic bin per enumeration value
    ಶೂನ್ಯ createEnumAutoBins(AstCoverpoint* coverpointp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* exprp,
                            ಧ್ರುವ AstEnumDType* enump) {
        File_ಸಾಲು* ಧ್ರುವ fl = coverpointp->fileline();
        ಪ್ರತಿ (ಧ್ರುವ AstEnumItem* itemp = enump->itemsp(); itemp;
             itemp = VN_AS(itemp->nextp(), EnumItem)) {
            AstConst* ಧ್ರುವ lop = new_ಮೌಲ್ಯ_Const(fl, VN_AS(itemp->valuep(), Const)->ಅಂಕೆ(), exprp);
            AstInsideRange* ಧ್ರುವ rangep = ನೂತನ AstInsideRange{fl, lop, lop->clone_ವೃಕ್ಷ(ಅಸತ್ಯ)};
            rangep->dtypeFrom(exprp);
            coverpointp->addBinsp(
                ನೂತನ AstCoverBin{fl, "auto[" + itemp->name() + "]", rangep, ಅಸತ್ಯ, ಅಸತ್ಯ});
        }
    }

    // IEEE 1800-2023 19.5.3/19.11.1: partition first, then apply exclusions.  The partition is one
    // automatic bins declaration, generated as a run like 'bins auto[N]' but numbering its bins.
    ಶೂನ್ಯ createImplicitAutoBins(AstCoverpoint* coverpointp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* exprp, ಪೂರ್ಣಾಙ್ಕ autoBinMax) {
        ಯದಿ (coverpointp->user2()) ಪ್ರತ್ಯಾಗಮ;  // Declared bins, ignored, leave no bins
        ಪ್ರತಿ (Ast_ಗಂಟು* nodep = coverpointp->binsp(); nodep; nodep = nodep->nextp()) {
            ಧ್ರುವ VCoverBinsType kind = VN_AS(nodep, CoverBin)->binsType();
            ಯದಿ (kind != VCoverBinsType::BINS_IGNORE && kind != VCoverBinsType::BINS_ILLEGAL)
                ಪ್ರತ್ಯಾಗಮ;
        }
        ಯದಿ (ಧ್ರುವ AstEnumDType* ಧ್ರುವ enump
            = VN_CAST(exprp->dtypep()->skipRefToEnump(), EnumDType)) {
            createEnumAutoBins(coverpointp, exprp, enump);
            ಪ್ರತ್ಯಾಗಮ;
        }
        ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ width = exprp->width();
        uint32_t ಎಣಿಕೆ = width < 31 ? std::min<uint32_t>(uint32_t{1} << width, autoBinMax)
                                    : static_cast<uint32_t>(autoBinMax);
        ಯದಿ (!ಎಣಿಕೆ) ಪ್ರತ್ಯಾಗಮ;
        ಯದಿ (!exprp->dtypep()->skipRefp()->isIntegralOrPacked()) {
            coverpointp->v3error("Coverpoint of a non-integral expression requires explicit bins "
                                 "(IEEE 1800-2023 19.5.3).");
            ಪ್ರತ್ಯಾಗಮ;
        }
        ಯದಿ (ಎಣಿಕೆ > v3Global.opt.coverageMaxBins()) {
            coverpointp->v3warn(COVERIGN, "Unsupported: more than "
                                              << v3Global.opt.coverageMaxBins()
                                              << " automatic bins from 'option.auto_bin_max'; "
                                                 "using "
                                              << v3Global.opt.coverageMaxBins() << ".\n"
                                              << coverpointp->warnMore()
                                              << "... Suggest a larger --coverage-max-bins");
            ಎಣಿಕೆ = v3Global.opt.coverageMaxBins();
        }
        File_ಸಾಲು* ಧ್ರುವ fl = coverpointp->fileline();
        coverpointp->addBinsp(ನೂತನ AstCoverBin{fl, "auto", ನೂತನ AstConst{fl, ಎಣಿಕೆ},
                                              VCoverBinsType::BINS_AUTO_IMPLICIT});
    }

    // Sanitize generated names to be valid C++ identifiers
    ಸ್ಥಿರ string sanitizeGeneratedName(string name) {
        std::replace(name.begin(), name.end(), '[', '_');
        std::replace(name.begin(), name.end(), ']', '_');
        ಪ್ರತ್ಯಾಗಮ name;
    }

    // Capture an iff guard in a function-local temporary so it is evaluated once per sample()
    AstVarRef* captureIffToTemp(Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* iffp, ಧ್ರುವ string& tempName) {
        File_ಸಾಲು* ಧ್ರುವ fl = iffp->fileline();
        AstVar* ಧ್ರುವ iffVarp
            = ನೂತನ AstVar{fl, VVarType::BLOCKTEMP, tempName, iffp->findBitDType()};
        iffVarp->funcLocal(ಸತ್ಯ);
        m_sampleFuncp->addStmtsp(iffVarp);
        iffp->unlinkFrBack();
        m_sampleFuncp->addStmtsp(
            ನೂತನ AstAssign{fl, ನೂತನ AstVarRef{fl, iffVarp, VAccess::WRITE}, iffp});
        ಪ್ರತ್ಯಾಗಮ ನೂತನ AstVarRef{fl, iffVarp, VAccess::READ};
    }

    // Add a statement sampling the current coverpoint to sample(), under its iff guard if any
    ಶೂನ್ಯ addSampleStmt(Ast_ಗಂಟು* stmtp) {
        ಯದಿ (m_cpGuardp) {
            m_cpGuardp->addThensp(stmtp);
        } ಅನ್ಯಥಾ {
            m_sampleFuncp->addStmtsp(stmtp);
        }
    }

    // Create previous value variable for transition tracking
    AstVar* create_Prev_ಮೌಲ್ಯ_Var(AstCoverpoint* coverpointp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* exprp) {
        // Check if already created
        ಯದಿ (AstVar* ಧ್ರುವ prevVarp = VN_CAST(coverpointp->user1p(), Var)) ಪ್ರತ್ಯಾಗಮ prevVarp;

        // Create variable to store previous sampled value
        ಧ್ರುವ string varName = "__Vprev_" + coverpointp->name();
        AstVar* prevVarp
            = ನೂತನ AstVar{coverpointp->fileline(), VVarType::MEMBER, varName, exprp->dtypep()};
        m_covergroupp->addMembersp(prevVarp);

        UINFO(4, "    Created previous value variable: " << varName);

        // Initialize to zero in constructor
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ initExprp
            = ನೂತನ AstConst{prevVarp->fileline(), AstConst::Widthed_ಮೌಲ್ಯ{}, prevVarp->width(), 0};
        Ast_ಗಂಟು_Stmt* ಧ್ರುವ initStmtp = ನೂತನ AstAssign{
            prevVarp->fileline(), ನೂತನ AstVarRef{prevVarp->fileline(), prevVarp, VAccess::WRITE},
            initExprp};
        m_constructorp->addStmtsp(initStmtp);

        coverpointp->user1p(prevVarp);
        ಪ್ರತ್ಯಾಗಮ prevVarp;
    }

    // Create state position variable for multi-value transition bins
    // Tracks position in sequence: 0=not started, 1=seen first item, etc.
    AstVar* createSequenceStateVar(AstCoverpoint* coverpointp, AstCoverBin* binp) {
        // Create variable to track sequence position
        ಧ್ರುವ string varName = "__Vseqpos_" + coverpointp->name() + "_" + binp->name();
        // Use 8-bit integer for state position (sequences rarely > 255 items)
        AstVar* stateVarp
            = ನೂತನ AstVar{binp->fileline(), VVarType::MEMBER, varName, VFlagLogicPacked{}, 8};
        m_covergroupp->addMembersp(stateVarp);

        UINFO(4, "    Created sequence state variable: " << varName);

        // Initialize to 0 (not started) in constructor
        Ast_ಗಂಟು_Stmt* ಧ್ರುವ initStmtp = ನೂತನ AstAssign{
            stateVarp->fileline(), ನೂತನ AstVarRef{stateVarp->fileline(), stateVarp, VAccess::WRITE},
            ನೂತನ AstConst{stateVarp->fileline(), AstConst::Widthed_ಮೌಲ್ಯ{}, 8, 0}};
        m_constructorp->addStmtsp(initStmtp);

        ಪ್ರತ್ಯಾಗಮ stateVarp;
    }

    ಶೂನ್ಯ generateCoverpointCode(AstCoverpoint* coverpointp) {
        UINFO(4, "  Generating code for coverpoint: " << coverpointp->name());

        // Get the coverpoint expression
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* exprp = coverpointp->exprp();

        // Check automatic bins before processing
        checkAutomaticBins(coverpointp, exprp);

        // Extract all coverpoint options in a single pass
        ಪೂರ್ಣಾಙ್ಕ at_Least_ಮೌಲ್ಯ;
        ಪೂರ್ಣಾಙ್ಕ autoBinMax;
        extractItemOptions(coverpointp->optionsp(), at_Least_ಮೌಲ್ಯ, autoBinMax);
        UINFO(6, "    Coverpoint at_least = " << at_Least_ಮೌಲ್ಯ << " auto_bin_max = " << autoBinMax);

        // Create implicit automatic bins if no regular bins exist
        createImplicitAutoBins(coverpointp, exprp, autoBinMax);

        // A false iff guard disables sampling of the coverpoint (IEEE 1800-2023 19.5), so
        // neither is its expression evaluated, nor are its bins or transition state updated
        VL_RESTORER(m_cpGuardp);
        ಯದಿ (Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ iffp = coverpointp->iffp()) {
            m_cpGuardp = ನೂತನ AstIf{iffp->fileline(), iffp->unlinkFrBack()};
        }

        AstVar* ಧ್ರುವ ಮೌಲ್ಯ_Varp = ನೂತನ AstVar{
            coverpointp->fileline(), VVarType::BLOCKTEMP,
            "__VcpValue_" + sanitizeGeneratedName(coverpointp->name()), exprp->dtypep()};
        ಮೌಲ್ಯ_Varp->funcLocal(ಸತ್ಯ);
        m_sampleFuncp->addStmtsp(ಮೌಲ್ಯ_Varp);
        exprp->unlinkFrBack();
        addSampleStmt(ನೂತನ AstAssign{
            coverpointp->fileline(),
            ನೂತನ AstVarRef{coverpointp->fileline(), ಮೌಲ್ಯ_Varp, VAccess::WRITE}, exprp});
        coverpointp->exprp(ನೂತನ AstVarRef{coverpointp->fileline(), ಮೌಲ್ಯ_Varp, VAccess::READ});
        exprp = coverpointp->exprp();

        // Every coverpoint routes through the VlCoverpoint runtime.  Transition coverpoints are
        // included: their per-value matching is still generated as a state machine in sample()
        // (see generateCoverpoint), but the bin hit is recorded in the runtime bin
        // rather than a bare counter.
        generateCoverpoint(coverpointp, exprp, at_Least_ಮೌಲ್ಯ);
        // Sampling follows what generateCoverpoint added unguarded, such as clearing the hit list
        ಯದಿ (m_cpGuardp) m_sampleFuncp->addStmtsp(m_cpGuardp);
    }

    // Build the condition under which a default bin matches: NOT(OR of all normal bins).
    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* buildDefaultCondition(AstCoverpoint* coverpointp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* exprp,
                                       File_ಸಾಲು* fl) {
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* anyBinMatchp = nullptr;
        ಪ್ರತಿ (Ast_ಗಂಟು* binp = coverpointp->binsp(); binp; binp = binp->nextp()) {
            AstCoverBin* ಧ್ರುವ cbinp = VN_AS(binp, CoverBin);
            ಯದಿ (cbinp->binsType() == VCoverBinsType::BINS_DEFAULT
                || cbinp->binsType() == VCoverBinsType::BINS_IGNORE
                || cbinp->binsType() == VCoverBinsType::BINS_ILLEGAL)
                ಅನುವರ್ತ;
            // The values of constructed bins are known at construction; see emitSizedSample
            ಯದಿ (isConstructedBins(cbinp)) ಅನುವರ್ತ;
            ಯದಿ (isAutoBins(cbinp)) {
                // Automatic bins partition the whole domain, leaving no default value
                ಯದಿ (anyBinMatchp) VL_DO_DANGLING(ತಳ್ಳು_Deletep(anyBinMatchp), anyBinMatchp);
                ಪ್ರತ್ಯಾಗಮ ನೂತನ AstConst{fl, AstConst::BitFalse{}};
            }
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ binCondp = buildBinCondition(cbinp, exprp);
            UASSERT_OBJ(binCondp, cbinp,
                        "buildBinCondition returned nullptr for non-ignore/non-illegal bin");
            anyBinMatchp = anyBinMatchp ? ನೂತನ AstOr{fl, anyBinMatchp, binCondp} : binCondp;
        }
        ಪ್ರತ್ಯಾಗಮ anyBinMatchp ? static_cast<Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ*>(ನೂತನ AstNot{fl, anyBinMatchp})
                            : static_cast<Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ*>(ನೂತನ AstConst{fl, AstConst::BitTrue{}});
    }

    //====================================================================
    // VlCoverpoint conversion

    ಸ್ಥಿರ ತಾರ್ಕಿಕ coverpointHasStateExclusions(ಧ್ರುವ AstCoverpoint* coverpointp) {
        ಪ್ರತಿ (ಧ್ರುವ Ast_ಗಂಟು* nodep = coverpointp->binsp(); nodep; nodep = nodep->nextp()) {
            ಧ್ರುವ AstCoverBin* ಧ್ರುವ binp = VN_AS(nodep, CoverBin);
            ಯದಿ (!binp->transp() && binp->rangesp()
                && (binp->binsType() == VCoverBinsType::BINS_IGNORE
                    || binp->binsType() == VCoverBinsType::BINS_ILLEGAL)) {
                ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;
            }
        }
        ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
    }

    ಸ್ಥಿರ ತಾರ್ಕಿಕ coverpointHasConstructedBins(ಧ್ರುವ AstCoverpoint* coverpointp) {
        ಪ್ರತಿ (ಧ್ರುವ Ast_ಗಂಟು* nodep = coverpointp->binsp(); nodep; nodep = nodep->nextp()) {
            ಯದಿ (isConstructedBins(VN_AS(nodep, CoverBin))) ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;
        }
        ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
    }

    // True if a Normal state bin, or an array-bin element, has no value of the coverpoint's
    // type (IEEE 1800-2023 19.5.7).  Array ranges enumerate in-type values, so cannot vanish.
    ಸ್ಥಿರ ತಾರ್ಕಿಕ coverpointHasEmptyBins(ಧ್ರುವ AstCoverpoint* coverpointp) {
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ exprp = coverpointp->exprp();
        ಪ್ರತಿ (Ast_ಗಂಟು* nodep = coverpointp->binsp(); nodep; nodep = nodep->nextp()) {
            ಧ್ರುವ AstCoverBin* ಧ್ರುವ binp = VN_AS(nodep, CoverBin);
            ಯದಿ (!binp->binsType().binIsNormal() || binp->transp() || !binp->rangesp()) ಅನುವರ್ತ;
            // A wildcard array has a bin for each value its elements match, so none empty, and
            // the constructor creates no bin without values of a 'with' filter
            ಯದಿ ((binp->isArray() && binp->isWildcard()) || binWith(binp)) ಅನುವರ್ತ;
            ತಾರ್ಕಿಕ empty = ಸತ್ಯ;
            ಪ್ರತಿ (Ast_ಗಂಟು* valuep = binp->rangesp(); valuep; valuep = valuep->nextp()) {
                ಯದಿ (binp->isArray() && VN_IS(valuep, InsideRange)) ಅನುವರ್ತ;
                Cross_ಮೌಲ್ಯ_Range range{valuep, resolveWidth(valuep, exprp)};
                ಧ್ರುವ ತಾರ್ಕಿಕ none = resolve_ಮೌಲ್ಯ(valuep, exprp, ಸತ್ಯ, binp->isWildcard(), range)
                                  && crossRangeEmpty(range);
                ಯದಿ (binp->isArray() && none) ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;
                empty &= none;
            }
            ಯದಿ (empty && !binp->isArray()) ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;
        }
        ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
    }

    // True if a coverpoint has any transition bin.  Used to decide whether sample() emits the
    // end-of-sample previous-value update that transition matching needs.
    ಸ್ಥಿರ ತಾರ್ಕಿಕ coverpointHasTransition(AstCoverpoint* coverpointp) {
        ಪ್ರತಿ (Ast_ಗಂಟು* binp = coverpointp->binsp(); binp; binp = binp->nextp()) {
            ಯದಿ (VN_AS(binp, CoverBin)->transp()) ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;
        }
        ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
    }

    // The interned AstBasicDType for one of the covergroup runtime keywords.
    ಸ್ಥಿರ AstBasicDType* basicDType(File_ಸಾಲು* fl, VBasicDTypeKwd kwd) {
        ಪ್ರತ್ಯಾಗಮ v3Global.rootp()->typeTablep()->findBasicDType(fl, kwd);
    }

    // Get (or create) the coverpoint dtype for a hit-list bound, interned so each distinct bound
    // yields one node.
    AstCoverpointDType* coverpointDType(File_ಸಾಲು* fl, uint32_t hitBound) {
        AstCoverpointDType*& typep = m_cpDTypes[hitBound];
        ಯದಿ (!typep) {
            typep = ನೂತನ AstCoverpointDType{fl, hitBound};
            v3Global.rootp()->typeTablep()->addTypesp(typep);
        }
        ಪ್ರತ್ಯಾಗಮ typep;
    }

    // The key of the covergroup's type in the coverage registry, which get_coverage() queries
    std::string covergroupProtectedName() ಧ್ರುವ {
        ಪ್ರತ್ಯಾಗಮ VIdProtect::protect_ಪದಗಳು_If(m_covergroupName, v3Global.opt.protectIds());
    }

    // The name of the covergroup's type, which keys the coverage registry and database: as
    // $typename names it (IEEE 1800-2023 20.6.1), so apart for covergroups of distinct scopes and
    // specializations, and alike in each Verilator run of a hierarchical design.  As design units
    // of distinct libraries may share a name, one of a library other than the default is prefixed
    // by its library, as '%l' prints it (IEEE 1800-2023 33.4).
    std::string covergroupTypeName() ಧ್ರುವ {
        UASSERT_OBJ(m_unitp, m_covergroupp, "Covergroup declared outside of a design unit");
        ಧ್ರುವ std::string& libname = m_unitp->libname();
        ಧ್ರುವ std::string name = m_covergroupp->dtypeName(ಸತ್ಯ);
        ಪ್ರತ್ಯಾಗಮ libname == "work" ? name : libname + "." + name;
    }

    // Emit the covergroup's instance handle member and the constructor statement that creates
    // its node in the per-context coverage registry.  Runs before any coverpoint or cross is
    // generated, so their runtimes can be added to the node as they are created.
    ಶೂನ್ಯ generateInstanceAttach() {
        File_ಸಾಲು* ಧ್ರುವ fl = m_covergroupp->fileline();
        // V3LinkParse synthesizes a 'new' for every covergroup; the item generators below already
        // rely on that, and this attach runs even for a covergroup with no coverpoints at all.
        UASSERT_OBJ(m_constructorp, m_covergroupp, "Covergroup missing synthesized constructor");
        m_cgInstVarp = ನೂತನ AstVar{fl, VVarType::MEMBER, "__Vcg_inst",
                                  basicDType(fl, VBasicDTypeKwd::COVERGROUP_INSTHANDLE)};
        m_covergroupp->addMembersp(m_cgInstVarp);

        m_constructorp->addStmtsp(
            itemCall(fl, m_cgInstVarp, VCMethod::COVERGROUP_ATTACH,
                     {ctext(fl, "vlSymsp->_vm_contextp__->covergroupRegistryp()"
                                "->newCovergroupInst("
                                    + quoted(covergroupProtectedName())
                                    + (m_cgMayMerge ? ", true" : ", false") + ")")},
                     /*usePtr=*/ಅಸತ್ಯ)
                ->makeStmt());
        // The node reads option.weight in place, so procedural assignments take effect
        AstCExpr* ಧ್ರುವ weightAddrp = ನೂತನ AstCExpr{fl, "&"};
        weightAddrp->add(newOptionSel(fl, optionVar(ಅಸತ್ಯ), "weight", VAccess::READ));
        m_constructorp->addStmtsp(itemCall(fl, m_cgInstVarp, VCMethod::COVERGROUP_LEND_WEIGHT,
                                           {weightAddrp, file_ಸಾಲು_Debug(fl)}, /*usePtr=*/ಅಸತ್ಯ)
                                      ->makeStmt());
    }

    // A '__Vcg_inst.p()-><method>()' call on the covergroup's instance node
    AstCMethodHard* instanceCall(File_ಸಾಲು* fl, VCMethod method) {
        // '__Vcg_inst.p()' -- a value handle, so '.' not '->'
        AstCMethodHard* ಧ್ರುವ instp
            = ನೂತನ AstCMethodHard{fl, memberRef(fl, m_cgInstVarp), VCMethod::COVERGROUP_INST_P};
        instp->usePtr(ಅಸತ್ಯ);
        instp->dtypeSetVoid();  // Opaque receiver; only ever the 'fromp' of the call below
        AstCMethodHard* ಧ್ರುವ callp = ನೂತನ AstCMethodHard{fl, instp, method};
        callp->usePtr(ಸತ್ಯ);
        ಪ್ರತ್ಯಾಗಮ callp;
    }

    // Emit 'this->__Vcp_x = this->__Vcg_inst.p()->addCoverpoint<K>();' (or addCross), which
    // creates the item runtime in the instance node and borrows a pointer to it.
    AstAssign* makeItemCreate(File_ಸಾಲು* fl, AstVar* itemVarp, VCMethod method) {
        AstCMethodHard* ಧ್ರುವ createp = instanceCall(fl, method);
        createp->dtypep(itemVarp->dtypep());
        ಪ್ರತ್ಯಾಗಮ ನೂತನ AstAssign{fl, memberRef(fl, itemVarp, VAccess::WRITE), createp};
    }

    // Constant bounds of one rangesp() element (an InsideRange or a single Const).  Each bound is
    // the raw AST node -- an AstConst or an AstUnbounded ('$') -- with the const/unbounded view
    // derived on demand, so there is one source of truth per bound.  After a successful
    // constRangeBounds() neither node is null; a single Const has both bounds aliasing one node.
    ಸಂರಚನಾ RangeBounds final {
        Ast_ಗಂಟು* loNodep = nullptr;  // low bound: AstConst or AstUnbounded
        Ast_ಗಂಟು* hiNodep = nullptr;  // high bound: AstConst or AstUnbounded
        ತಾರ್ಕಿಕ loUnbounded() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ VN_IS(loNodep, Unbounded); }
        ತಾರ್ಕಿಕ hiUnbounded() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ VN_IS(hiNodep, Unbounded); }
        AstConst* loConstp() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ VN_CAST(loNodep, Const); }
        AstConst* hiConstp() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ VN_CAST(hiNodep, Const); }
    };

    // Decode one rangesp() element into its constant bounds.  Returns false if rp is neither an
    // InsideRange nor a single Const, or if a present bound is non-constant or 4-state.  '$'
    // bounds are left as AstUnbounded (not resolved) -- the caller applies its own policy.
    // Centralizes the InsideRange/Const/Unbounded decode shared by the hit-list-bound paths.
    ಸ್ಥಿರ ತಾರ್ಕಿಕ constRangeBounds(Ast_ಗಂಟು* rp, RangeBounds& rb) {
        ಯದಿ (AstInsideRange* ಧ್ರುವ irp = VN_CAST(rp, InsideRange)) {
            rb.loNodep = irp->lhsp();
            rb.hiNodep = irp->rhsp();
        } ಅನ್ಯಥಾ ಯದಿ (AstConst* ಧ್ರುವ cp = VN_CAST(rp, Const)) {
            rb.loNodep = rb.hiNodep = cp;
        } ಅನ್ಯಥಾ {
            ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
        }
        // Each bound must be a constant unless it is '$'; reject non-const and 4-state.
        AstConst* ಧ್ರುವ lc = rb.loConstp();
        AstConst* ಧ್ರುವ hc = rb.hiConstp();
        ಯದಿ ((!lc && !rb.loUnbounded()) || (!hc && !rb.hiUnbounded())) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
        ಯದಿ ((lc && lc->ಅಂಕೆ().isFourState()) || (hc && hc->ಅಂಕೆ().isFourState())) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
        ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;
    }

    // Collect the covered value intervals of a single (non-array) Normal bin.  Returns false
    // if any range isn't a constant/open InsideRange or single Const (e.g. wildcard, non-const).
    ಸ್ಥಿರ ತಾರ್ಕಿಕ extractRangeIntervals(AstCoverBin* cbinp, uint64_t maxVal,
                                      std::vector<std::pair<uint64_t, uint64_t>>& out) {
        ಯದಿ (!cbinp->rangesp()) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
        ಪ್ರತಿ (Ast_ಗಂಟು* rp = cbinp->rangesp(); rp; rp = rp->nextp()) {
            RangeBounds rb;
            ಯದಿ (!constRangeBounds(rp, rb)) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
            ಯದಿ ((rb.loConstp() && rb.loConstp()->width() > 64)
                || (rb.hiConstp() && rb.hiConstp()->width() > 64)) {
                ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;  // Use the safe slot-count bound for wide values.
            }
            ಧ್ರುವ uint64_t lo = rb.loUnbounded() ? 0 : rb.loConstp()->toUQuad();
            ಧ್ರುವ uint64_t hi = rb.hiUnbounded() ? maxVal : rb.hiConstp()->toUQuad();
            ಯದಿ (lo > hi) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
            out.emplace_back(lo, hi);
        }
        ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;
    }

    // Append one Normal bin's cross-slot interval-sets to `bins` and bump `slotCount` by an upper
    // bound of the bin's slots holding one value.  Returns false if any part isn't statically
    // enumerable (the caller then falls back to the always-safe slot count).  A non-array bin
    // is one slot covering the union of its intervals; the bins of an array element or of an
    // automatic bins declaration hold disjoint values, so the element or declaration counts once.
    ತಾರ್ಕಿಕ appendBinCrossSlots(AstCoverBin* cbinp, uint64_t maxVal, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* exprp,
                             std::vector<std::vector<std::pair<uint64_t, uint64_t>>>& bins,
                             ಪೂರ್ಣಾಙ್ಕ& slot_ಎಣಿಕೆ) {
        ಯದಿ (isAutoBins(cbinp)) {
            ++slot_ಎಣಿಕೆ;
            bins.ತಳ್ಳು_back({{0, maxVal}});
            ಪ್ರತ್ಯಾಗಮ exprp->width() <= 64;
        }
        ಯದಿ (binWith(cbinp)) {
            // A value is in one bin of a filter's, but of a sized array, in one for each range
            // list element holding it; the coverpoint's name is one element
            ಪೂರ್ಣಾಙ್ಕ elements = 0;
            ಪ್ರತಿ (ಧ್ರುವ Ast_ಗಂಟು* rp = binRangesp(cbinp); rp && isSizedArray(cbinp);
                 rp = rp->nextp()) {
                ++elements;
            }
            slot_ಎಣಿಕೆ += std::max(1, elements);
            ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
        }
        ಯದಿ (cbinp->isArray() && cbinp->isWildcard() && !cbinp->arraySizep()) {
            // A value is in at most one bin of a wildcard array: one slot covering its values.
            // Signed values are sign-extended, not unsigned intervals (see computeHitListBound)
            ++slot_ಎಣಿಕೆ;
            ಯದಿ (exprp->isSigned() || exprp->width() > 64) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
            ಧ್ರುವ BinRuns runs = wildcardBinRuns(cbinp, exprp, ಅಸತ್ಯ);
            ಯದಿ (runs.unsupported) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
            std::vector<std::pair<uint64_t, uint64_t>> ivs;
            ಪ್ರತಿ (ಧ್ರುವ BinRun& run : runs.runs) {
                ivs.emplace_back(run.m_lo.toUQuad(), run.m_hi.toUQuad());
            }
            bins.ತಳ್ಳು_back(std::move(ivs));
            ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;
        }
        ಯದಿ (cbinp->isArray()) ಪ್ರತ್ಯಾಗಮ appendArrayBinCrossSlots(cbinp, exprp, bins, slot_ಎಣಿಕೆ);
        // Non-array bin: one slot covering the union of its intervals.
        ++slot_ಎಣಿಕೆ;
        std::vector<std::pair<uint64_t, uint64_t>> ivs;
        ಯದಿ (cbinp->isWildcard() || !extractRangeIntervals(cbinp, maxVal, ivs)) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
        bins.ತಳ್ಳು_back(std::move(ivs));
        ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;
    }

    // Append the cross slots of an array Normal bin: each element is a slot covering its
    // values, as at most one of its single-value bins holds a value.  An element holds the
    // values arrayBinRuns() gives it: those of the coverpoint type (IEEE 1800-2023 19.5.7).
    // Elements of a signed coverpoint, non-constant elements, and elements with values beyond
    // 64 bits can't be enumerated, so they count one slot but lose exactness.  Returns false if
    // any element wasn't enumerable to exact values.
    ತಾರ್ಕಿಕ appendArrayBinCrossSlots(AstCoverBin* cbinp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* exprp,
                                  std::vector<std::vector<std::pair<uint64_t, uint64_t>>>& bins,
                                  ಪೂರ್ಣಾಙ್ಕ& slot_ಎಣಿಕೆ) {
        // Signed values resolve sign-extended, not as unsigned intervals, and wildcard patterns
        // are not intervals
        ತಾರ್ಕಿಕ exact = !exprp->isSigned() && !cbinp->isWildcard();
        ಪ್ರತಿ (Ast_ಗಂಟು* rp = cbinp->rangesp(); rp; rp = rp->nextp()) {
            ++slot_ಎಣಿಕೆ;
            RangeBounds rb;
            Cross_ಮೌಲ್ಯ_Range range{rp, resolveWidth(rp, exprp)};
            ಯದಿ (!exact || !constRangeBounds(rp, rb)
                || !resolve_ಮೌಲ್ಯ(rp, exprp, ಸತ್ಯ, ಅಸತ್ಯ, range)) {
                exact = ಅಸತ್ಯ;
                ಅನುವರ್ತ;
            }
            ಯದಿ (crossRangeEmpty(range)) ಅನುವರ್ತ;  // Its bin, if any, holds no value
            ಯದಿ (range.hi.mostSetBitP1() > 64) {
                exact = ಅಸತ್ಯ;
                ಅನುವರ್ತ;
            }
            bins.ತಳ್ಳು_back({{range.lo.toUQuad(), range.hi.toUQuad()}});
        }
        ಪ್ರತ್ಯಾಗಮ exact;
    }

    // Compute the hit-list bound for a coverpoint: the maximum number of Normal
    // bins one sample value can match.  Non-cross-fed coverpoints don't feed a cross, so
    // their hit list is unused -> 1.  Otherwise compute the exact max bin overlap; fall back
    // to the (always-safe) Normal-slot count when any bin isn't statically analyzable.
    ಪೂರ್ಣಾಙ್ಕ computeHitListBound(AstCoverpoint* coverpointp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* exprp, ತಾರ್ಕಿಕ crossFed) {
        ಯದಿ (!crossFed) ಪ್ರತ್ಯಾಗಮ 1;
        ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ width = exprp->width();
        ಧ್ರುವ uint64_t maxVal = (width >= 64) ? UINT64_MAX : ((1ULL << width) - 1);
        // One entry per Normal bin (cross slot): its covered intervals.
        std::vector<std::vector<std::pair<uint64_t, uint64_t>>> bins;
        ಪೂರ್ಣಾಙ್ಕ slot_ಎಣಿಕೆ = 0;  // == runtime m_normal; the safe fallback bound
        // Unsigned intervals cannot establish overlap between differently sized signed values.
        ತಾರ್ಕಿಕ exact = !exprp->isSigned();
        ಪ್ರತಿ (Ast_ಗಂಟು* binp = coverpointp->binsp(); binp; binp = binp->nextp()) {
            AstCoverBin* ಧ್ರುವ cbinp = VN_AS(binp, CoverBin);
            ಯದಿ (!cbinp->binsType().binIsNormal())
                ಅನುವರ್ತ;  // ignore/illegal/default: not hit-listed
            ಯದಿ (!appendBinCrossSlots(cbinp, maxVal, exprp, bins, slot_ಎಣಿಕೆ)) exact = ಅಸತ್ಯ;
        }
        ಯದಿ (!exact) ಪ್ರತ್ಯಾಗಮ std::max(1, slot_ಎಣಿಕೆ);
        ಯದಿ (bins.empty()) ಪ್ರತ್ಯಾಗಮ 1;
        // Max overlap occurs at some interval start; count covering bins at each lo.
        std::vector<uint64_t> pts;
        ಪ್ರತಿ (ಧ್ರುವ auto& b : bins)
            ಪ್ರತಿ (ಧ್ರುವ auto& iv : b) pts.ತಳ್ಳು_back(iv.first);
        std::sort(pts.begin(), pts.end());
        pts.erase(std::unique(pts.begin(), pts.end()), pts.end());
        ಪೂರ್ಣಾಙ್ಕ maxOverlap = 1;
        ಪ್ರತಿ (ಧ್ರುವ uint64_t p : pts) {
            ಪೂರ್ಣಾಙ್ಕ cnt = 0;
            ಪ್ರತಿ (ಧ್ರುವ auto& b : bins) {
                ಪ್ರತಿ (ಧ್ರುವ auto& iv : b) {
                    ಯದಿ (iv.first <= p && p <= iv.second) {
                        ++cnt;
                        ಭಙ್ಗ;  // count each bin at most once
                    }
                }
            }
            ಯದಿ (cnt > maxOverlap) maxOverlap = cnt;
        }
        ಪ್ರತ್ಯಾಗಮ maxOverlap;
    }

    // A 'this->m_member' reference for embedding in an AstCStmt
    AstVarRef* memberRef(File_ಸಾಲು* fl, AstVar* varp, VAccess access = VAccess::READ) {
        AstVarRef* ಧ್ರುವ refp = ನೂತನ AstVarRef{fl, varp, access};
        refp->selfPointer(VSelfPointerText{VSelfPointerText::This{}});
        ಪ್ರತ್ಯಾಗಮ refp;
    }

    // A 'this->m_member-><method>(args...)' call on one member.  usePtr is false only for
    // __Vcg_inst, which is a value handle; the item members are borrowed pointers into the
    // instance node.  Numeric arguments are AstConst; the rest are C++ text that has no AST
    // form (see ctext).
    AstCMethodHard* itemCall(File_ಸಾಲು* fl, AstVar* varp, VCMethod method,
                             ಧ್ರುವ std::vector<Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ*>& args = {}, ತಾರ್ಕಿಕ usePtr = ಸತ್ಯ) {
        AstCMethodHard* ಧ್ರುವ callp = ನೂತನ AstCMethodHard{fl, memberRef(fl, varp), method};
        ಪ್ರತಿ (Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ argp : args) callp->addPinsp(argp);
        callp->usePtr(usePtr);
        callp->dtypeSetVoid();
        ಪ್ರತ್ಯಾಗಮ callp;
    }

    // An unsigned integer argument.
    ಸ್ಥಿರ AstConst* cnum(File_ಸಾಲು* fl, uint32_t ಮೌಲ್ಯ) { ಪ್ರತ್ಯಾಗಮ ನೂತನ AstConst{fl, ಮೌಲ್ಯ}; }

    // A literal C++ argument with no AST equivalent: a 'const char*' string literal (an SV
    // string AstConst emits '"..."s', a std::string temporary the runtime cannot borrow), a
    // VlCovBinKind enum token, a constant selection-word initializer list, a VlFileLineDebug, or
    // a '__V' temporary declared by the enclosing AstCStmt.
    ಸ್ಥಿರ AstCExpr* ctext(File_ಸಾಲು* fl, ಧ್ರುವ std::string& text) {
        ಪ್ರತ್ಯಾಗಮ ನೂತನ AstCExpr{fl, text};
    }

    // A C++ string literal.  Escapes control characters as the emitter does elsewhere -- bin
    // names and filenames reach the generated code verbatim when --protect-ids is off, and an
    // SV escaped identifier may hold a quote or backslash.
    ಸ್ಥಿರ std::string quoted(ಧ್ರುವ std::string& text) {
        ಪ್ರತ್ಯಾಗಮ "\"" + V3OutFormatter::quoteNameControls(text) + "\"";
    }

    // A 'VlFileLineDebug' argument: where the runtime reports an error about fl's construct
    ಸ್ಥಿರ AstCExpr* file_ಸಾಲು_Debug(File_ಸಾಲು* fl) {
        ಧ್ರುವ std::string filename
            = VIdProtect::protectIf(fl->filename(), v3Global.opt.protectIds());
        ಪ್ರತ್ಯಾಗಮ ctext(fl, "VlFileLineDebug{" + quoted(filename) + ", "
                             + std::to_string(fl->lineno()) + "}");
    }

    // Check that an element of an array bin (bins b[] = {values/ranges}) is a two-state
    // constant value or range; false if not, after reporting it if 'report'.
    ಸ್ಥಿರ ತಾರ್ಕಿಕ checkArrayBinElement(AstCoverBin* arrayBinp, Ast_ಗಂಟು* rangep, ತಾರ್ಕಿಕ report = ಸತ್ಯ) {
        ಯದಿ (ಧ್ರುವ AstInsideRange* ಧ್ರುವ irp = VN_CAST(rangep, InsideRange)) {
            ಧ್ರುವ AstConst* ಧ್ರುವ minp = VN_CAST(irp->lhsp(), Const);
            ಧ್ರುವ AstConst* ಧ್ರುವ maxp = VN_CAST(irp->rhsp(), Const);
            ಯದಿ ((!minp && !VN_IS(irp->lhsp(), Unbounded))
                || (!maxp && !VN_IS(irp->rhsp(), Unbounded))) {
                ಯದಿ (report) {
                    arrayBinp->v3error("Non-constant expression in array bins range; "
                                       "range bounds must be constants (IEEE 1800-2023 19.5)");
                }
                ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
            }
            ಯದಿ ((minp && minp->ಅಂಕೆ().isFourState()) || (maxp && maxp->ಅಂಕೆ().isFourState())) {
                ಯದಿ (report) {
                    arrayBinp->v3error("Four-state (x/z) value in array bins range bound; "
                                       "range bounds must be two-state constants");
                }
                ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
            }
        } ಅನ್ಯಥಾ ಯದಿ (!VN_IS(rangep, Const)) {
            ಯದಿ (report) {
                arrayBinp->v3error("Non-constant expression in array bins value list; "
                                   "values must be constants (IEEE 1800-2023 19.5)");
            }
            ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
        }
        ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;
    }

    // Individual equality targets of an array bin (bins b[] = {values/ranges}) of a real
    // coverpoint, in order; integral coverpoints generate array bins as runs (see arrayBinRuns).
    // An open-ended bound ('$', AstUnbounded) resolves to the coverpoint domain: '[lo:$]'
    // covers [lo:maxVal] and '[$:hi]' covers [0:hi].  One target is produced per value; ranges
    // whose resolved size would exceed --coverage-max-real-bins (e.g. an open '[lo:$]') are
    // unsupported -- emits COVERIGN, sets unsupportedOut, yields nothing.
    std::vector<Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ*> extract_Array_ಮೌಲ್ಯಗಳು(AstCoverBin* arrayBinp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* exprp,
                                                 ತಾರ್ಕಿಕ& unsupportedOut) {
        unsupportedOut = ಅಸತ್ಯ;
        ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ width = exprp->width();
        ಧ್ರುವ uint64_t maxVal = (width >= 64) ? UINT64_MAX : ((1ULL << width) - 1);
        std::vector<Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ*> ಮೌಲ್ಯಗಳು;
        ಪ್ರತಿ (Ast_ಗಂಟು* rangep = arrayBinp->rangesp(); rangep; rangep = rangep->nextp()) {
            rangep = V3Const::constifyEdit(rangep);
            ಯದಿ (!checkArrayBinElement(arrayBinp, rangep)) ಪ್ರತ್ಯಾಗಮ ಮೌಲ್ಯಗಳು;
            ಯದಿ (AstInsideRange* ಧ್ರುವ irp = VN_CAST(rangep, InsideRange)) {
                ಧ್ರುವ ತಾರ್ಕಿಕ loUnb = VN_IS(irp->lhsp(), Unbounded);
                ಧ್ರುವ ತಾರ್ಕಿಕ hiUnb = VN_IS(irp->rhsp(), Unbounded);
                ಧ್ರುವ uint64_t lo = loUnb ? 0 : VN_AS(irp->lhsp(), Const)->toUQuad();
                ಧ್ರುವ uint64_t hi = hiUnb ? maxVal : VN_AS(irp->rhsp(), Const)->toUQuad();
                ಯದಿ (hi < lo) ಅನುವರ್ತ;  // empty range contributes no bins
                // Guard against a '$'-bounded or otherwise huge range exploding the bin count.
                ಧ್ರುವ uint64_t span = hi - lo;  // == valueCount - 1 (no overflow: hi >= lo)
                ಯದಿ (span >= v3Global.opt.coverageMaxRealBins()
                    || ಮೌಲ್ಯಗಳು.size() + span + 1 > v3Global.opt.coverageMaxRealBins()) {
                    arrayBinp->v3warn(COVERIGN,
                                      "Unsupported: array 'bins' of a real coverpoint "
                                      "covering more than "
                                          << v3Global.opt.coverageMaxRealBins() << " values; bin "
                                          << arrayBinp->prettyNameQ() << " ignored.\n"
                                          << arrayBinp->warnMore()
                                          << "... Suggest a larger --coverage-max-real-bins");
                    unsupportedOut = ಸತ್ಯ;
                    ಪ್ರತಿ (Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ vp : ಮೌಲ್ಯಗಳು) VL_DO_DANGLING(ತಳ್ಳು_Deletep(vp), vp);
                    ಮೌಲ್ಯಗಳು.clear();
                    ಪ್ರತ್ಯಾಗಮ ಮೌಲ್ಯಗಳು;
                }
                ಪ್ರತಿ (uint64_t v = lo; v <= hi; ++v)
                    ಮೌಲ್ಯಗಳು.ತಳ್ಳು_back(ನೂತನ AstConst{irp->fileline(), AstConst::Widthed_ಮೌಲ್ಯ{}, width,
                                                  static_cast<uint32_t>(v)});
            } ಅನ್ಯಥಾ {
                ಮೌಲ್ಯಗಳು.ತಳ್ಳು_back(VN_AS(rangep->clone_ವೃಕ್ಷ(ಅಸತ್ಯ), ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ));
            }
        }
        ಪ್ರತ್ಯಾಗಮ ಮೌಲ್ಯಗಳು;
    }

    ಸ್ಥಿರ ಪೂರ್ಣಾಙ್ಕ runWidth(ಧ್ರುವ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* exprp) { ಪ್ರತ್ಯಾಗಮ exprp->width() + 1; }

    // Automatic bins partition the coverpoint domain in value order (IEEE 1800-2023 19.5.3): N
    // bins, capped at the number of values, each hold 2^width / N values, and the last bin also
    // holds the remainder.  False for an invalid declaration, already reported.
    ತಾರ್ಕಿಕ autoBinRuns(AstCoverBin* binp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* exprp, BinRuns& out) {
        ಧ್ರುವ uint32_t requested = autoBinsRequested(binp);
        ಯದಿ (!requested || !exprp->dtypep()->skipRefp()->isIntegralOrPacked()) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
        ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ width = exprp->width();
        ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ arithmeticWidth = runWidth(exprp);
        ಧ್ರುವ uint32_t ಎಣಿಕೆ
            = width < 32
                  ? static_cast<uint32_t>(std::min<uint64_t>(uint64_t{1} << width, requested))
                  : requested;
        BinRun run{binp, arithmeticWidth, ಎಣಿಕೆ};
        V3_ಸಂಖ್ಯೆ total{binp, arithmeticWidth};
        total.setBit(width, 1);
        run.m_stride.opDiv(total, V3_ಸಂಖ್ಯೆ{binp, arithmeticWidth, ಎಣಿಕೆ});
        ಧ್ರುವ Cross_ಮೌಲ್ಯ_Range domain
            = cross_ಮೌಲ್ಯ_Domain(binp, width, exprp->isSigned(), arithmeticWidth);
        run.m_lo = domain.lo;
        run.m_hi = domain.hi;
        out.runs.ತಳ್ಳು_back(std::move(run));
        out.ಎಣಿಕೆ = ಎಣಿಕೆ;
        ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;
    }

    // The elements of an array bin (bins b[] = {values/ranges}), in order, as runs of
    // single-value bins.  A range holds the values of the coverpoint type it contains (IEEE
    // 1800-2023 19.5.7), while a singleton names a bin even without such a value.  Errors on a
    // non-constant element.  More than --coverage-max-bins bins (e.g. an open '[lo:$]' range over
    // a wide coverpoint) are unsupported -- emits COVERIGN, and sets unsupported.
    BinRuns arrayBinRuns(AstCoverBin* arrayBinp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* exprp) {
        BinRuns out;
        ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ width = runWidth(exprp);
        ಪ್ರತಿ (Ast_ಗಂಟು* rangep = arrayBinp->rangesp(); rangep; rangep = rangep->nextp()) {
            rangep = V3Const::constifyEdit(rangep);
            ಯದಿ (!checkArrayBinElement(arrayBinp, rangep)) ಪ್ರತ್ಯಾಗಮ out;
            ಧ್ರುವ AstInsideRange* ಧ್ರುವ irp = VN_CAST(rangep, InsideRange);
            Cross_ಮೌಲ್ಯ_Range range{rangep, resolveWidth(rangep, exprp)};
            ತಾರ್ಕಿಕ empty = ಸತ್ಯ;
            ಯದಿ (!resolve_ಮೌಲ್ಯ(rangep, exprp, ಸತ್ಯ, ಅಸತ್ಯ, range)) {
                rangep->v3warn(E_UNSUPPORTED, "Unsupported: non-integral value in a coverage bin "
                                              "of an integral coverpoint.");
            } ಅನ್ಯಥಾ {
                empty = crossRangeEmpty(range);
            }
            ಯದಿ (empty && irp) ಅನುವರ್ತ;  // A range without values contributes no bins
            BinRun run{rangep, width, 1};
            run.m_empty = empty;
            uint64_t ಎಣಿಕೆ = 1;
            ಯದಿ (!empty) {
                run.m_lo.opAssign(range.lo);
                run.m_hi.opAssign(range.hi);
                V3_ಸಂಖ್ಯೆ span{rangep, width};
                span.opSub(run.m_hi, run.m_lo);
                // Wider spans exceed any limit
                ಎಣಿಕೆ = span.mostSetBitP1() > 32 ? UINT64_MAX : span.toUQuad() + 1;
            }
            ಯದಿ (ಎಣಿಕೆ > v3Global.opt.coverageMaxBins() - out.ಎಣಿಕೆ) {
                arrayBinp->v3warn(COVERIGN, "Unsupported: array 'bins' covering more than "
                                                << v3Global.opt.coverageMaxBins()
                                                << " values (e.g. an open '[lo:$]' range over "
                                                   "a wide coverpoint); bin "
                                                << arrayBinp->prettyNameQ() << " ignored\n"
                                                << arrayBinp->warnMore()
                                                << "... Suggest a larger --coverage-max-bins");
                out.runs.clear();
                out.ಎಣಿಕೆ = 0;
                out.unsupported = ಸತ್ಯ;
                ಪ್ರತ್ಯಾಗಮ out;
            }
            run.m_ಎಣಿಕೆ = static_cast<uint32_t>(ಎಣಿಕೆ);
            out.ಎಣಿಕೆ += run.m_ಎಣಿಕೆ;
            out.runs.ತಳ್ಳು_back(std::move(run));
        }
        ಪ್ರತ್ಯಾಗಮ out;
    }

    // The bins of a wildcard array (wildcard bins b[] = {...}): one for each coverpoint value
    // an element matches (IEEE 1800-2023 19.5.4, 19.5.7), in value order, and named by the value
    // (19.5.1), as runs of single-value bins.  Errors on a non-constant element.  More than
    // --coverage-max-bins values are unsupported -- emits COVERIGN, and sets unsupported, and for
    // an ignore or illegal array, single.  'report' false omits these diagnostics.
    ಸ್ಥಿರ BinRuns wildcardBinRuns(AstCoverBin* arrayBinp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* exprp, ತಾರ್ಕಿಕ report) {
        BinRuns out;
        ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ width = runWidth(exprp);
        ಧ್ರುವ V3_ಸಂಖ್ಯೆ one{arrayBinp, width, 1};
        // Disjoint runs of the values, in value order, each value once
        std::vector<std::pair<V3_ಸಂಖ್ಯೆ, V3_ಸಂಖ್ಯೆ>> spans;
        uint64_t ಎಣಿಕೆ = 0;  // Values of 'spans'
        ಪ್ರತಿ (Ast_ಗಂಟು* rangep = arrayBinp->rangesp(); rangep; rangep = rangep->nextp()) {
            rangep = V3Const::constifyEdit(rangep);
            ಯದಿ (!checkArrayBinElement(arrayBinp, rangep, report)) {
                out.unsupported = ಸತ್ಯ;
                ಪ್ರತ್ಯಾಗಮ out;
            }
            Cross_ಮೌಲ್ಯ_Range range{rangep, resolveWidth(rangep, exprp)};
            ಯದಿ (!resolve_ಮೌಲ್ಯ(rangep, exprp, ಸತ್ಯ, ಸತ್ಯ, range)) {
                ಯದಿ (report) {
                    rangep->v3warn(E_UNSUPPORTED, "Unsupported: non-integral value in a "
                                                  "coverage bin of an integral coverpoint.");
                }
                ಅನುವರ್ತ;
            }
            ಯದಿ (crossRangeEmpty(range)) ಅನುವರ್ತ;
            std::vector<std::pair<V3_ಸಂಖ್ಯೆ, V3_ಸಂಖ್ಯೆ>> found;
            crossRangeRuns(range, v3Global.opt.coverageMaxBins(), found);
            ಪ್ರತಿ (ಧ್ರುವ std::pair<V3_ಸಂಖ್ಯೆ, V3_ಸಂಖ್ಯೆ>& run : found) {
                // Coverpoint values, sign-extended in both widths
                spans.emplace_back(V3_ಸಂಖ್ಯೆ{rangep, width, run.first},
                                   V3_ಸಂಖ್ಯೆ{rangep, width, run.second});
            }
            std::sort(spans.begin(), spans.end(), [](ಧ್ರುವ auto& lhs, ಧ್ರುವ auto& rhs) {
                ಪ್ರತ್ಯಾಗಮ cross_ಮೌಲ್ಯ_Less(lhs.first, rhs.first);
            });
            std::vector<std::pair<V3_ಸಂಖ್ಯೆ, V3_ಸಂಖ್ಯೆ>> merged;
            ಪ್ರತಿ (ಧ್ರುವ std::pair<V3_ಸಂಖ್ಯೆ, V3_ಸಂಖ್ಯೆ>& span : spans) {
                // Adjacent or overlapping values join a run; 'first - 1' cannot overflow
                V3_ಸಂಖ್ಯೆ before{rangep, width};
                before.opSub(span.first, one);
                ಯದಿ (merged.empty() || cross_ಮೌಲ್ಯ_Less(merged.back().second, before)) {
                    merged.ತಳ್ಳು_back(span);
                } ಅನ್ಯಥಾ ಯದಿ (cross_ಮೌಲ್ಯ_Less(merged.back().second, span.second)) {
                    merged.back().second = span.second;
                }
            }
            ಎಣಿಕೆ = 0;
            ಪ್ರತಿ (ಧ್ರುವ std::pair<V3_ಸಂಖ್ಯೆ, V3_ಸಂಖ್ಯೆ>& span : merged) {
                V3_ಸಂಖ್ಯೆ size{rangep, width};
                size.opSub(span.second, span.first);
                // Beyond 2^32 values exceed any limit
                ಎಣಿಕೆ += size.mostSetBitP1() > 32 ? uint64_t{1} << 33 : size.toUQuad() + 1;
                ಯದಿ (ಎಣಿಕೆ > v3Global.opt.coverageMaxBins()) ಭಙ್ಗ;
            }
            spans = std::move(merged);
            ಯದಿ (ಎಣಿಕೆ > v3Global.opt.coverageMaxBins()) {
                // An ignore or illegal array still excludes or checks its values, as one bin
                out.single = !arrayBinp->binsType().binIsNormal();
                ಯದಿ (report) {
                    arrayBinp->v3warn(
                        COVERIGN, "Unsupported: wildcard array '"
                                      << arrayBinp->binsType().verilogKwd()
                                      << "' of more than --coverage-max-bins of "
                                      << v3Global.opt.coverageMaxBins() << " values; bin "
                                      << arrayBinp->prettyNameQ()
                                      << (out.single ? " treated as one bin" : " ignored") << "\n"
                                      << arrayBinp->warnMore()
                                      << "... Suggest a larger --coverage-max-bins");
                }
                out.unsupported = ಸತ್ಯ;
                ಪ್ರತ್ಯಾಗಮ out;
            }
        }
        ಪ್ರತಿ (ಧ್ರುವ std::pair<V3_ಸಂಖ್ಯೆ, V3_ಸಂಖ್ಯೆ>& span : spans) {
            out.runs.emplace_back(arrayBinp, width, 0);
            BinRun& run = out.runs.back();
            run.m_lo = span.first;
            run.m_hi = span.second;
            // A bin for each value, named by it in the coverpoint's type
            V3_ಸಂಖ್ಯೆ ಮೌಲ್ಯ = span.first;
            ಯಾವತ್ (ಸತ್ಯ) {
                ಧ್ರುವ V3_ಸಂಖ್ಯೆ typed{arrayBinp, exprp->width(), ಮೌಲ್ಯ};
                out.ಮೌಲ್ಯಗಳು.ತಳ್ಳು_back(exprp->isSigned() ? typed.toDecimalS() : typed.toDecimalU());
                ++run.m_ಎಣಿಕೆ;
                ಯದಿ (ಮೌಲ್ಯ.isCaseEq(span.second)) ಭಙ್ಗ;
                V3_ಸಂಖ್ಯೆ next{arrayBinp, width};
                ಮೌಲ್ಯ = next.opAdd(ಮೌಲ್ಯ, one);
            }
        }
        out.ಎಣಿಕೆ = static_cast<uint32_t>(ಎಣಿಕೆ);
        ಪ್ರತ್ಯಾಗಮ out;
    }

    // The runs of an automatic bins declaration, or of an array bin of an integral coverpoint.
    // False for other bins, which do not generate as runs, including a wildcard array then one
    // bin (see BinRuns::single).
    ತಾರ್ಕಿಕ binRunsFor(AstCoverBin* binp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* exprp, BinRuns& out) {
        ಯದಿ (isAutoBins(binp)) {
            ಯದಿ (!autoBinRuns(binp, exprp, out)) out.unsupported = ಸತ್ಯ;
            ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;
        }
        ಯದಿ (binp->isArray() && binp->isWildcard()) {
            ಯದಿ (!exprp->dtypep()->skipRefp()->isIntegralOrPacked()) {
                Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ falsep = wildcardTypeError(binp, exprp);
                VL_DO_DANGLING(ತಳ್ಳು_Deletep(falsep), falsep);
                out.unsupported = ಸತ್ಯ;
            } ಅನ್ಯಥಾ {
                out = wildcardBinRuns(binp, exprp, ಸತ್ಯ);
                ಯದಿ (out.single) {
                    binp->isArray(ಅಸತ್ಯ);  // Generates as one bin
                    ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
                }
            }
            ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;
        }
        ಯದಿ (!binp->isArray() || binp->transp() || binp->isWildcard()
            || !exprp->dtypep()->skipRefp()->isIntegralOrPacked()) {
            ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
        }
        out = arrayBinRuns(binp, exprp);
        ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;
    }

    // Emit a 'this->m_cp->addSingleNamer/addArrayNamer(...)' statement for one bin whose first
    // runtime bin index is 'declared'; or with 'valueNames', those naming each bin of an array
    Ast_ಗಂಟು_Stmt* makeNamer(AstVar* cpVarp, AstCoverBin* binp, int64_t ಎಣಿಕೆ, uint32_t declared,
                           ಧ್ರುವ std::vector<Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ*>& ಮೌಲ್ಯಗಳು = {},
                           ಧ್ರುವ std::vector<std::string>& ಮೌಲ್ಯ_Names = {}) {
        File_ಸಾಲು* ಧ್ರುವ fl = binp->fileline();
        CoverpointBins& bins = m_cpBins.at(cpVarp);
        ಧ್ರುವ uint32_t normal_ಎಣಿಕೆ
            = binp->binsType().binIsNormal() ? static_cast<uint32_t>(ಎಣಿಕೆ < 0 ? 1 : ಎಣಿಕೆ) : 0;
        ಧ್ರುವ BinSpan span{bins.total, normal_ಎಣಿಕೆ, declared};
        ಯದಿ (binp->binsType() == VCoverBinsType::BINS_AUTO_IMPLICIT) {
            bins.implicitAuto = span;  // Selected by bin, see implicitAutoBinSpan
        } ಅನ್ಯಥಾ {
            bins.spans.emplace(binp->name(), span);
        }
        bins.total += normal_ಎಣಿಕೆ;
        ಪ್ರತಿ (uint32_t i = 0; bins.crossed && i < normal_ಎಣಿಕೆ; ++i) {
            bins.ಮೌಲ್ಯಗಳು.ತಳ್ಳು_back({binp, ಮೌಲ್ಯಗಳು.empty() ? nullptr : ಮೌಲ್ಯಗಳು[i]});
        }
        // Under --protect-ids the filename and bin name flow into the coverage database
        // verbatim, so obfuscate them exactly as line/toggle coverage points are (whole-
        // unit filename, per-word bin name).  A no-op when --protect-ids is off.
        ಧ್ರುವ ತಾರ್ಕಿಕ prot = v3Global.opt.protectIds();
        ಧ್ರುವ ತಾರ್ಕಿಕ single = ಎಣಿಕೆ < 0;
        ಯದಿ (!ಮೌಲ್ಯ_Names.empty()) {
            // A bin of a wildcard array is named by its value (IEEE 1800-2023 19.5.1)
            Ast_ಗಂಟು_Stmt* stmtsp = nullptr;
            ಪ್ರತಿ (ಧ್ರುವ std::string& ಮೌಲ್ಯ : ಮೌಲ್ಯ_Names) {
                ಧ್ರುವ std::string name
                    = VIdProtect::protect_ಪದಗಳು_If(binp->name(), prot) + "[" + ಮೌಲ್ಯ + "]";
                stmtsp = Ast_ಗಂಟು::addNext(
                    stmtsp,
                    itemCall(fl, cpVarp, VCMethod::COVERGROUP_ADD_SINGLE_NAMER,
                             {ctext(fl, binp->binsType().binSetEnum()), ctext(fl, quoted(name)),
                              ctext(fl, quoted(VIdProtect::protectIf(fl->filename(), prot))),
                              cnum(fl, static_cast<uint32_t>(fl->lineno())),
                              cnum(fl, static_cast<uint32_t>(fl->firstColumn()))})
                        ->makeStmt());
            }
            ಪ್ರತ್ಯಾಗಮ stmtsp;
        }
        std::vector<Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ*> args{ctext(fl, binp->binsType().binSetEnum())};
        ಯದಿ (!single) args.ತಳ್ಳು_back(cnum(fl, static_cast<uint32_t>(ಎಣಿಕೆ)));  // value array bin
        args.ತಳ್ಳು_back(ctext(fl, quoted(VIdProtect::protect_ಪದಗಳು_If(binp->name(), prot))));
        args.ತಳ್ಳು_back(ctext(fl, quoted(VIdProtect::protectIf(fl->filename(), prot))));
        args.ತಳ್ಳು_back(cnum(fl, static_cast<uint32_t>(fl->lineno())));
        args.ತಳ್ಳು_back(cnum(fl, static_cast<uint32_t>(fl->firstColumn())));
        ಪ್ರತ್ಯಾಗಮ itemCall(fl, cpVarp,
                        single ? VCMethod::COVERGROUP_ADD_SINGLE_NAMER
                        : binp->binsType() == VCoverBinsType::BINS_AUTO_IMPLICIT
                            ? VCMethod::COVERGROUP_ADD_NUMBERED_NAMER
                            : VCMethod::COVERGROUP_ADD_ARRAY_NAMER,
                        args)
            ->makeStmt();
    }

    // Emit 'if (iff && cond) m_cp.incrementBin(idx);' (or recordHit, + illegal action) in sample()
    // Where a bin's hit is recorded in the runtime VlCoverpoint member.
    ಸಂರಚನಾ ConvBinTarget final {
        AstVar* cpVarp;  // the __Vcp_<coverpoint> member
        uint32_t idx;  // bin index within that coverpoint
        ತಾರ್ಕಿಕ isNormal;  // Normal -> incrementBin (count + cross hit list); else recordHit (count)
    };

    // Emit 'this->m_cp.incrementBin(idx);' (Normal) or '.recordHit(idx);'
    // (ignore/illegal/default).
    Ast_ಗಂಟು_Stmt* makeRuntimeBinHit(File_ಸಾಲು* fl, AstVar* cpVarp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* idxp,
                                   ತಾರ್ಕಿಕ isNormal) {
        ಪ್ರತ್ಯಾಗಮ itemCall(fl, cpVarp,
                        isNormal ? VCMethod::COVERGROUP_INCREMENT_BIN
                                 : VCMethod::COVERGROUP_RECORD_HIT,
                        {idxp})
            ->makeStmt();
    }
    Ast_ಗಂಟು_Stmt* makeRuntimeBinHit(File_ಸಾಲು* fl, ಧ್ರುವ ConvBinTarget& tgt) {
        ಪ್ರತ್ಯಾಗಮ makeRuntimeBinHit(fl, tgt.cpVarp, cnum(fl, static_cast<uint32_t>(tgt.idx)),
                                 tgt.isNormal);
    }

    // The condition under which a bin counts a sample: condp, and the bin's iff, and for a
    // Normal or default state bin, that the value is not excluded
    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* binCondition(AstCoverBin* binp, AstVar* cpVarp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* condp) {
        File_ಸಾಲು* ಧ್ರುವ fl = binp->fileline();
        ಯದಿ (binp->iffp()) condp = ನೂತನ AstLogAnd{fl, binp->iffp()->clone_ವೃಕ್ಷ(ಅಸತ್ಯ), condp};
        ಧ್ರುವ auto excluded = m_excludedVars.find(cpVarp);
        ಯದಿ (excluded != m_excludedVars.end() && !binp->transp()
            && (binp->binsType().binIsNormal()
                || binp->binsType() == VCoverBinsType::BINS_DEFAULT)) {
            condp = ನೂತನ AstLogAnd{
                fl, ನೂತನ AstNot{fl, ನೂತನ AstVarRef{fl, excluded->second, VAccess::READ}}, condp};
        }
        ಪ್ರತ್ಯಾಗಮ condp;
    }

    ಶೂನ್ಯ emitConvHitIf(AstCoverpoint* coverpointp, AstCoverBin* binp, AstVar* cpVarp,
                       Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* idxp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* condp) {
        File_ಸಾಲು* ಧ್ರುವ fl = binp->fileline();
        Ast_ಗಂಟು* actionp = makeRuntimeBinHit(fl, cpVarp, idxp, binp->binsType().binIsNormal());
        ಯದಿ (binp->binsType() == VCoverBinsType::BINS_ILLEGAL) {
            actionp->addNext(makeIllegalBinAction(fl, "Illegal bin " + binp->prettyNameQ()
                                                          + " hit in coverpoint "
                                                          + coverpointp->prettyNameQ()));
        }
        UASSERT_OBJ(m_sampleFuncp, binp, "sample() CFunc not set for coverpoint");
        addSampleStmt(ನೂತನ AstIf{fl, binCondition(binp, cpVarp, condp), actionp, nullptr});
    }

    // Emit the sample() code of sized array 'sized': count the value in its bins holding it when
    // enabled, as other bins are, and for default bins note in matchedp whether any holds it
    ಶೂನ್ಯ emitSizedSample(AstCoverpoint* coverpointp, AstCoverBin* binp, AstVar* cpVarp,
                         Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* exprp, uint32_t sized, AstVar* matchedp) {
        File_ಸಾಲು* ಧ್ರುವ fl = binp->fileline();
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* enabledp = binCondition(binp, cpVarp, ನೂತನ AstConst{fl, AstConst::BitTrue{}});
        UASSERT_OBJ(m_sampleFuncp, binp, "sample() CFunc not set for coverpoint");
        ಧ್ರುವ ತಾರ್ಕಿಕ illegal = binp->binsType() == VCoverBinsType::BINS_ILLEGAL;
        ಯದಿ (illegal) {
            // The illegal action reads the condition too, which is evaluated once, as the guard
            // may have side effects
            AstVar* ಧ್ರುವ varp
                = ನೂತನ AstVar{fl, VVarType::BLOCKTEMP,
                             "__VcpEnabled_" + sanitizeGeneratedName(coverpointp->name()) + "_"
                                 + cvtToStr(sized),
                             binp->findBitDType()};
            varp->funcLocal(ಸತ್ಯ);
            m_sampleFuncp->addStmtsp(varp);
            addSampleStmt(ನೂತನ AstAssign{fl, ನೂತನ AstVarRef{fl, varp, VAccess::WRITE}, enabledp});
            enabledp = ನೂತನ AstVarRef{fl, varp, VAccess::READ};
        }
        AstCMethodHard* ಧ್ರುವ callp
            = itemCall(fl, cpVarp,
                       exprp->isWide() ? VCMethod::COVERGROUP_SIZED_SAMPLE_W
                                       : VCMethod::COVERGROUP_SIZED_SAMPLE,
                       {cnum(fl, sized), exprp->clone_ವೃಕ್ಷ(ಅಸತ್ಯ), enabledp});
        callp->dtypeSetBit();
        ಯದಿ (illegal) {
            addSampleStmt(ನೂತನ AstIf{fl, ನೂತನ AstLogAnd{fl, callp, enabledp->clone_ವೃಕ್ಷ(ಅಸತ್ಯ)},
                                    makeIllegalBinAction(fl, "Illegal bin " + binp->prettyNameQ()
                                                                 + " hit in coverpoint "
                                                                 + coverpointp->prettyNameQ())});
        } ಅನ್ಯಥಾ ಯದಿ (matchedp && binp->binsType().binIsNormal()) {
            addSampleStmt(
                ನೂತನ AstAssign{fl, ನೂತನ AstVarRef{fl, matchedp, VAccess::WRITE},
                              ನೂತನ AstOr{fl, ನೂತನ AstVarRef{fl, matchedp, VAccess::READ}, callp}});
        } ಅನ್ಯಥಾ {
            addSampleStmt(callp->makeStmt());
        }
    }

    // A variable local to the constructor
    AstVar* constructorTemp(File_ಸಾಲು* fl, ಧ್ರುವ string& name, Ast_ಗಂಟು_DType* dtypep) {
        AstVar* ಧ್ರುವ varp = ನೂತನ AstVar{fl, VVarType::BLOCKTEMP, name, dtypep};
        varp->funcLocal(ಸತ್ಯ);
        m_constructorp->addStmtsp(varp);
        ಪ್ರತ್ಯಾಗಮ varp;
    }

    // Truncate or extend, as its signedness sets, a value to a type
    ಸ್ಥಿರ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* resize_ಮೌಲ್ಯ(Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* valuep, Ast_ಗಂಟು_DType* dtypep) {
        File_ಸಾಲು* ಧ್ರುವ fl = valuep->fileline();
        ಯದಿ (valuep->width() > dtypep->width()) {
            valuep = ನೂತನ AstSel{fl, valuep, 0, dtypep->width()};
        } ಅನ್ಯಥಾ ಯದಿ (valuep->width() < dtypep->width()) {
            valuep = valuep->isSigned()
                         ? static_cast<Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ*>(ನೂತನ AstExtendS{fl, valuep, dtypep->width()})
                         : ನೂತನ AstExtend{fl, valuep, dtypep->width()};
        }
        valuep->dtypep(dtypep);
        ಪ್ರತ್ಯಾಗಮ valuep;
    }

    // The runtime arguments of a count, which the constructor evaluates once into a temporary:
    // its value, a count wider than 64 bits saturated over any limit, and whether it is positive
    std::pair<Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ*, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ*> new_ಎಣಿಕೆ_Args(File_ಸಾಲು* fl, ಧ್ರುವ string& name,
                                                       Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* exprp) {
        AstVar* ಧ್ರುವ countp = constructorTemp(fl, name, exprp->dtypep());
        m_constructorp->addStmtsp(
            ನೂತನ AstAssign{fl, ನೂತನ AstVarRef{fl, countp, VAccess::WRITE}, exprp->clone_ವೃಕ್ಷ(ಅಸತ್ಯ)});
        ಧ್ರುವ auto ಎಣಿಕೆ_Ref = [&]() { ಪ್ರತ್ಯಾಗಮ ನೂತನ AstVarRef{fl, countp, VAccess::READ}; };
        AstConst* ಧ್ರುವ zerop = ನೂತನ AstConst{fl, AstConst::DTyped{}, countp->dtypep()};
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ positivep
            = countp->isSigned() ? static_cast<Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ*>(ನೂತನ AstGtS{fl, ಎಣಿಕೆ_Ref(), zerop})
                                 : ನೂತನ AstNeq{fl, ಎಣಿಕೆ_Ref(), zerop};
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* valuep = resize_ಮೌಲ್ಯ(ಎಣಿಕೆ_Ref(), countp->findUInt64DType());
        ಯದಿ (countp->width() > VL_QUADSIZE) {
            valuep = ನೂತನ AstCond{
                fl,
                ನೂತನ AstRedOr{
                    fl, ನೂತನ AstSel{fl, ಎಣಿಕೆ_Ref(), VL_QUADSIZE, countp->width() - VL_QUADSIZE}},
                ನೂತನ AstConst{fl, AstConst::Unsized64{}, std::numeric_limits<uint64_t>::max()},
                valuep};
            valuep->dtypeSetUInt64();
        }
        ಪ್ರತ್ಯಾಗಮ {valuep, positivep};
    }
    // The runtime arguments of a constant count
    ಸ್ಥಿರ std::pair<Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ*, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ*> new_ಎಣಿಕೆ_Args(File_ಸಾಲು* fl, uint64_t ಎಣಿಕೆ) {
        ಪ್ರತ್ಯಾಗಮ {ನೂತನ AstConst{fl, AstConst::Unsized64{}, ಎಣಿಕೆ},
                ನೂತನ AstConst{fl, AstConst::BitTrue{}}};
    }

    // The values of a coverpoint, which its name denotes (IEEE 1800-2023 19.5.1.1), as runs in
    // value order: an enumerated type's values (6.19), else all values of its type.  Bounds
    // are at runWidth(), sign-extended like a CrossValueRange's.
    ಸ್ಥಿರ std::vector<std::pair<V3_ಸಂಖ್ಯೆ, V3_ಸಂಖ್ಯೆ>> coverpoint_ಮೌಲ್ಯಗಳು(Ast_ಗಂಟು* nodep,
                                                                       Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* exprp) {
        ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ width = runWidth(exprp);
        std::vector<std::pair<V3_ಸಂಖ್ಯೆ, V3_ಸಂಖ್ಯೆ>> runs;
        ಧ್ರುವ AstEnumDType* ಧ್ರುವ enump = VN_CAST(exprp->dtypep()->skipRefToEnump(), EnumDType);
        ಯದಿ (!enump) {
            ಧ್ರುವ Cross_ಮೌಲ್ಯ_Range domain
                = cross_ಮೌಲ್ಯ_Domain(nodep, exprp->width(), exprp->isSigned(), width);
            runs.emplace_back(domain.lo, domain.hi);
            ಪ್ರತ್ಯಾಗಮ runs;
        }
        std::vector<V3_ಸಂಖ್ಯೆ> ಮೌಲ್ಯಗಳು;
        ಪ್ರತಿ (ಧ್ರುವ AstEnumItem* itemp = enump->itemsp(); itemp;
             itemp = VN_AS(itemp->nextp(), EnumItem)) {
            ಧ್ರುವ V3_ಸಂಖ್ಯೆ& ಅಂಕೆ = VN_AS(itemp->valuep(), Const)->ಅಂಕೆ();
            ಯದಿ (ಅಂಕೆ.isFourState()) ಅನುವರ್ತ;  // Not a coverpoint value (19.5.7)
            ಮೌಲ್ಯಗಳು.emplace_back(nodep, width);
            ಯದಿ (exprp->isSigned()) {
                ಮೌಲ್ಯಗಳು.back().opExtendS(ಅಂಕೆ, ಅಂಕೆ.width());
            } ಅನ್ಯಥಾ {
                ಮೌಲ್ಯಗಳು.back().opAssign(ಅಂಕೆ);
            }
        }
        std::sort(ಮೌಲ್ಯಗಳು.begin(), ಮೌಲ್ಯಗಳು.end(), cross_ಮೌಲ್ಯ_Less);
        ಧ್ರುವ V3_ಸಂಖ್ಯೆ one{nodep, width, 1};
        ಪ್ರತಿ (ಧ್ರುವ V3_ಸಂಖ್ಯೆ& ಮೌಲ್ಯ : ಮೌಲ್ಯಗಳು) {
            V3_ಸಂಖ್ಯೆ next{nodep, width};
            ಯದಿ (!runs.empty() && next.opAdd(runs.back().second, one).isCaseEq(ಮೌಲ್ಯ)) {
                runs.back().second = ಮೌಲ್ಯ;
            } ಅನ್ಯಥಾ {
                runs.emplace_back(ಮೌಲ್ಯ, ಮೌಲ್ಯ);
            }
        }
        ಪ್ರತ್ಯಾಗಮ runs;
    }

    // Emit the constructor code building the bins 'binp' whose values it computes: the values of
    // each element that are coverpoint values (IEEE 1800-2023 19.5.7), those a 'with' filter
    // keeps (19.5.1.1), then its bins
    ಶೂನ್ಯ generateConstructedBins(AstCoverpoint* coverpointp, AstCoverBin* binp, AstVar* cpVarp,
                                 Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* exprp) {
        File_ಸಾಲು* ಧ್ರುವ fl = binp->fileline();
        ಧ್ರುವ string prefix
            = m_sizedNames.get(sanitizeGeneratedName(coverpointp->name() + "__" + binp->name()));
        // The count, and whether it is positive; one of a filter's scalar bin, or bin per value
        ಧ್ರುವ std::pair<Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ*, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ*> ಎಣಿಕೆ
            = binp->arraySizep() ? new_ಎಣಿಕೆ_Args(fl, prefix + "_count", binp->arraySizep())
                                 : new_ಎಣಿಕೆ_Args(fl, 1);
        AstCoverWith* ಧ್ರುವ withp = binWith(binp);
        ಯದಿ (withp && !binRangesp(binp)) {  // The coverpoint's name: all of its values
            ಪ್ರತಿ (ಧ್ರುವ std::pair<V3_ಸಂಖ್ಯೆ, V3_ಸಂಖ್ಯೆ>& run : coverpoint_ಮೌಲ್ಯಗಳು(binp, exprp)) {
                m_constructorp->addStmtsp(itemCall(fl, cpVarp,
                                                   exprp->isWide()
                                                       ? VCMethod::COVERGROUP_SIZED_RANGE_W
                                                       : VCMethod::COVERGROUP_SIZED_RANGE,
                                                   {new_ಮೌಲ್ಯ_Const(fl, run.first, exprp),
                                                    new_ಮೌಲ್ಯ_Const(fl, run.second, exprp)})
                                              ->makeStmt());
            }
        }
        uint32_t element = 0;
        ಪ್ರತಿ (Ast_ಗಂಟು* rangep = binRangesp(binp); rangep; rangep = rangep->nextp()) {
            ಯದಿ (VN_IS(rangep, Unbounded)) {  // A parameter of '$'
                binp->v3error("Bins value may not be '$', which may only bound a range "
                              "(IEEE 1800-2023 6.20.7)");
                ಅನುವರ್ತ;
            }
            generateSizedElement(cpVarp, binp, rangep, exprp, prefix + "_" + cvtToStr(element++));
        }
        ಯದಿ (withp) generateWithFilter(binp, withp, cpVarp, exprp, prefix);
        std::vector<Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ*> args{ctext(fl, binp->binsType().binSetEnum()), ಎಣಿಕೆ.first,
                                       ಎಣಿಕೆ.second};
        // A filter's bins have the limit it began with
        ಯದಿ (!withp) args.ತಳ್ಳು_back(cnum(fl, v3Global.opt.coverageMaxBins()));
        ಧ್ರುವ ತಾರ್ಕಿಕ prot = v3Global.opt.protectIds();
        args.ತಳ್ಳು_back(ctext(fl, quoted(VIdProtect::protect_ಪದಗಳು_If(binp->name(), prot))));
        args.ತಳ್ಳು_back(ctext(fl, quoted(VIdProtect::protectIf(fl->filename(), prot))));
        args.ತಳ್ಳು_back(cnum(fl, static_cast<uint32_t>(fl->lineno())));
        args.ತಳ್ಳು_back(cnum(fl, static_cast<uint32_t>(fl->firstColumn())));
        m_constructorp->addStmtsp(
            itemCall(fl, cpVarp,
                     withp ? VCMethod::COVERGROUP_WITH_FINISH : VCMethod::COVERGROUP_SIZED_FINISH,
                     args)
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
                            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* exprp, ಧ್ರುವ string& prefix) {
        File_ಸಾಲು* ಧ್ರುವ fl = withp->fileline();
        ಧ್ರುವ ತಾರ್ಕಿಕ wide = exprp->isWide();
        ಧ್ರುವ string grouping = !binp->isArray()     ? "Single"
                                : binp->arraySizep() ? "Fixed"
                                                     : "Values";
        m_constructorp->addStmtsp(itemCall(fl, cpVarp, VCMethod::COVERGROUP_WITH_BEGIN,
                                           {ctext(fl, "VlCovBinGrouping::" + grouping),
                                            cnum(fl, v3Global.opt.coverageMaxBins())})
                                      ->makeStmt());
        // The candidates count in the coverpoint's width, and the filter reads each as 'item',
        // of the coverpoint's type, so that a filter changing 'item' cannot change the loop
        Ast_ಗಂಟು_DType* ಧ್ರುವ ಮೌಲ್ಯ_DTypep = exprp->dtypep();
        AstVar* ಧ್ರುವ valuep = constructorTemp(fl, prefix + "_value", ಮೌಲ್ಯ_DTypep);
        AstVar* ಧ್ರುವ lastp = constructorTemp(fl, prefix + "_last", ಮೌಲ್ಯ_DTypep);
        AstVar* ಧ್ರುವ firstp = constructorTemp(fl, prefix + "_first", ಮೌಲ್ಯ_DTypep);
        AstVar* ಧ್ರುವ runp = constructorTemp(fl, prefix + "_run", binp->findBitDType());
        AstVar* ಧ್ರುವ morep = constructorTemp(fl, prefix + "_more", binp->findBitDType());
        AstVar* ಧ್ರುವ itemp = withp->itemp()->unlinkFrBack();
        itemp->name(prefix + "_item");
        m_constructorp->addStmtsp(itemp);
        ಧ್ರುವ auto ref = [&](AstVar* varp) { ಪ್ರತ್ಯಾಗಮ ನೂತನ AstVarRef{fl, varp, VAccess::READ}; };
        ಧ್ರುವ auto assign = [&](AstVar* varp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* rhsp) -> Ast_ಗಂಟು* {
            ಪ್ರತ್ಯಾಗಮ ನೂತನ AstAssign{fl, ನೂತನ AstVarRef{fl, varp, VAccess::WRITE}, rhsp};
        };
        ಧ್ರುವ auto flag = [&](AstVar* varp, ತಾರ್ಕಿಕ ಮೌಲ್ಯ) {
            ಪ್ರತ್ಯಾಗಮ assign(varp, ಮೌಲ್ಯ ? ನೂತನ AstConst{fl, AstConst::BitTrue{}}
                                      : ನೂತನ AstConst{fl, AstConst::BitFalse{}});
        };
        ಧ್ರುವ auto bound = [&](VCMethod narrow, VCMethod wideMethod, AstVar* varp) -> Ast_ಗಂಟು* {
            ಯದಿ (wide) {
                ಪ್ರತ್ಯಾಗಮ itemCall(fl, cpVarp, wideMethod, {ನೂತನ AstVarRef{fl, varp, VAccess::WRITE}})
                    ->makeStmt();
            }
            AstCMethodHard* ಧ್ರುವ callp = itemCall(fl, cpVarp, narrow);
            callp->dtypeSetUInt64();
            ಪ್ರತ್ಯಾಗಮ assign(varp, resize_ಮೌಲ್ಯ(callp, ಮೌಲ್ಯ_DTypep));
        };
        ಧ್ರುವ auto keep = [&](Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lop, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* hip) {
            AstCMethodHard* ಧ್ರುವ callp = itemCall(
                fl, cpVarp, wide ? VCMethod::COVERGROUP_WITH_RUN_W : VCMethod::COVERGROUP_WITH_RUN,
                {lop, hip});
            callp->dtypeSetBit();
            ಪ್ರತ್ಯಾಗಮ assign(morep, callp);
        };
        ಧ್ರುವ auto step = [&](ತಾರ್ಕಿಕ up) {
            AstConst* ಧ್ರುವ onep = ನೂತನ AstConst{fl, AstConst::Widthed_ಮೌಲ್ಯ{}, exprp->width(), 1};
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ stepp
                = up ? static_cast<Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ*>(ನೂತನ AstAdd{fl, ref(valuep), onep})
                     : ನೂತನ AstSub{fl, ref(valuep), onep};
            stepp->dtypep(ಮೌಲ್ಯ_DTypep);
            ಪ್ರತ್ಯಾಗಮ stepp;
        };
        AstLoop* ಧ್ರುವ innerp = ನೂತನ AstLoop{fl};
        innerp->addStmtsp(assign(itemp, ref(valuep)));
        innerp->addStmtsp(ನೂತನ AstIf{
            fl, withp->filterp()->unlinkFrBack(),
            ನೂತನ AstIf{fl, ನೂತನ AstNot{fl, ref(runp)},
                      assign(firstp, ref(valuep))->addNext(flag(runp, ಸತ್ಯ))},
            ನೂತನ AstIf{fl, ref(runp), keep(ref(firstp), step(ಅಸತ್ಯ))->addNext(flag(runp, ಅಸತ್ಯ))}});
        innerp->addStmtsp(ನೂತನ AstLoopTest{
            fl, innerp, ನೂತನ AstLogAnd{fl, ref(morep), ನೂತನ AstNeq{fl, ref(valuep), ref(lastp)}}});
        innerp->addStmtsp(assign(valuep, step(ಸತ್ಯ)));
        AstLoop* ಧ್ರುವ outerp = ನೂತನ AstLoop{fl};
        AstCMethodHard* ಧ್ರುವ nextp = itemCall(fl, cpVarp, VCMethod::COVERGROUP_WITH_NEXT);
        nextp->dtypeSetBit();
        outerp->addStmtsp(ನೂತನ AstLoopTest{fl, outerp, nextp});
        outerp->addStmtsp(
            bound(VCMethod::COVERGROUP_WITH_LO, VCMethod::COVERGROUP_WITH_LO_W, valuep));
        outerp->addStmtsp(
            bound(VCMethod::COVERGROUP_WITH_HI, VCMethod::COVERGROUP_WITH_HI_W, lastp));
        outerp->addStmtsp(flag(runp, ಅಸತ್ಯ));
        outerp->addStmtsp(innerp);
        outerp->addStmtsp(ನೂತನ AstIf{fl, ref(runp), keep(ref(firstp), ref(lastp))});
        m_constructorp->addStmtsp(flag(morep, ಸತ್ಯ));
        m_constructorp->addStmtsp(outerp);
    }

    // Emit 'sizedRange(lo, hi)' for the coverpoint values of an element of bins 'binp', whose
    // values the constructor computes: resolved now if constant, else when constructed by
    // clipping to the coverpoint's values.  A wildcard pattern's values give a range for each
    // run of them.  'prefix' names its temporaries.
    ಶೂನ್ಯ generateSizedElement(AstVar* cpVarp, ಧ್ರುವ AstCoverBin* binp, Ast_ಗಂಟು* rangep,
                              Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* exprp, ಧ್ರುವ string& prefix) {
        File_ಸಾಲು* ಧ್ರುವ fl = rangep->fileline();
        ಧ್ರುವ VCMethod method = exprp->isWide() ? VCMethod::COVERGROUP_SIZED_RANGE_W
                                                : VCMethod::COVERGROUP_SIZED_RANGE;
        ಧ್ರುವ AstInsideRange* ಧ್ರುವ irp = VN_CAST(rangep, InsideRange);
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ lowp = irp ? irp->lhsp() : VN_AS(rangep, ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ);
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ highp = irp ? irp->rhsp() : nullptr;
        ಧ್ರುವ auto unbounded
            = [](ಧ್ರುವ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* boundp) { ಪ್ರತ್ಯಾಗಮ !boundp || VN_IS(boundp, Unbounded); };
        ಧ್ರುವ auto constant
            = [&](ಧ್ರುವ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* boundp) { ಪ್ರತ್ಯಾಗಮ unbounded(boundp) || VN_IS(boundp, Const); };
        ಧ್ರುವ auto integral = [&](ಧ್ರುವ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* boundp) {
            ಪ್ರತ್ಯಾಗಮ unbounded(boundp) || boundp->dtypep()->skipRefp()->isIntegralOrPacked();
        };
        ಯದಿ (constant(lowp) && constant(highp)) {
            ಧ್ರುವ auto fourState = [](ಧ್ರುವ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* boundp) {
                ಧ್ರುವ AstConst* ಧ್ರುವ constp = VN_CAST(boundp, Const);
                ಪ್ರತ್ಯಾಗಮ constp && constp->ಅಂಕೆ().isFourState();
            };
            ಯದಿ (irp && (fourState(lowp) || fourState(highp))) {
                rangep->v3error("Four-state (x/z) value in "
                                << (binp->isArray() ? "array bins" : "bin")
                                << " range bound; range bounds must be two-state constants");
                ಪ್ರತ್ಯಾಗಮ;
            }
            Cross_ಮೌಲ್ಯ_Range range{rangep, resolveWidth(rangep, exprp)};
            ಯದಿ (!resolve_ಮೌಲ್ಯ(rangep, exprp, ಸತ್ಯ, binp->isWildcard(), range)) {
                rangep->v3warn(E_UNSUPPORTED, "Unsupported: non-integral value in a coverage bin "
                                              "of an integral coverpoint.");
            } ಅನ್ಯಥಾ ಯದಿ (!crossRangeEmpty(range)) {
                std::vector<std::pair<V3_ಸಂಖ್ಯೆ, V3_ಸಂಖ್ಯೆ>> runs{{range.lo, range.hi}};
                ಯದಿ (range.wildcard) {  // checkConstructedBins bounded the runs
                    runs.clear();
                    crossRangeRuns(range, v3Global.opt.coverageMaxBins(), runs);
                }
                ಪ್ರತಿ (ಧ್ರುವ std::pair<V3_ಸಂಖ್ಯೆ, V3_ಸಂಖ್ಯೆ>& run : runs) {
                    m_constructorp->addStmtsp(itemCall(fl, cpVarp, method,
                                                       {new_ಮೌಲ್ಯ_Const(fl, run.first, exprp),
                                                        new_ಮೌಲ್ಯ_Const(fl, run.second, exprp)})
                                                  ->makeStmt());
                }
            }
            ಪ್ರತ್ಯಾಗಮ;
        }
        ಯದಿ (!integral(lowp) || !integral(highp)) {
            rangep->v3warn(E_UNSUPPORTED, "Unsupported: non-integral value in a coverage bin "
                                          "of an integral coverpoint.");
            ಪ್ರತ್ಯಾಗಮ;
        }
        // Compare values and the coverpoint's domain signed, in a width holding all of them
        ಧ್ರುವ auto boundWidth
            = [&](ಧ್ರುವ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* boundp) { ಪ್ರತ್ಯಾಗಮ unbounded(boundp) ? 0 : boundp->width(); };
        ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ width = std::max({exprp->width(), boundWidth(lowp), boundWidth(highp)}) + 1;
        ಧ್ರುವ Cross_ಮೌಲ್ಯ_Range domain
            = cross_ಮೌಲ್ಯ_Domain(rangep, exprp->width(), exprp->isSigned(), width);
        AstVar* ಧ್ರುವ lop = constructorTemp(fl, prefix + "_lo", exprp->dtypep());
        lop->dtypeSetBitSized(width, VSigning::SIGNED);
        AstVar* ಧ್ರುವ hip = constructorTemp(fl, prefix + "_hi", lop->dtypep());
        ಧ್ರುವ auto bound = [&](Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* boundp, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& limit) -> Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* {
            ಯದಿ (unbounded(boundp)) {
                AstConst* ಧ್ರುವ limitp = ನೂತನ AstConst{fl, limit};
                limitp->dtypeFrom(lop);
                ಪ್ರತ್ಯಾಗಮ limitp;
            }
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* valuep = boundp->clone_ವೃಕ್ಷ(ಅಸತ್ಯ);
            // A value no wider than a signed coverpoint has its type (see crossRangeBound)
            ಯದಿ (exprp->isSigned() && boundp->width() <= exprp->width()) {
                valuep = resize_ಮೌಲ್ಯ(valuep, exprp->dtypep());
            }
            ಪ್ರತ್ಯಾಗಮ resize_ಮೌಲ್ಯ(valuep, lop->dtypep());
        };
        ಧ್ರುವ auto ref = [&](AstVar* varp, VAccess access = VAccess::READ) {
            ಪ್ರತ್ಯಾಗಮ ನೂತನ AstVarRef{fl, varp, access};
        };
        m_constructorp->addStmtsp(
            ನೂತನ AstAssign{fl, ref(lop, VAccess::WRITE), bound(lowp, domain.lo)});
        m_constructorp->addStmtsp(
            ನೂತನ AstAssign{fl, ref(hip, VAccess::WRITE), irp ? bound(highp, domain.hi) : ref(lop)});
        AstConst* ಧ್ರುವ minp = ನೂತನ AstConst{fl, domain.lo};
        AstConst* ಧ್ರುವ maxp = ನೂತನ AstConst{fl, domain.hi};
        minp->dtypeFrom(lop);
        maxp->dtypeFrom(lop);
        AstCond* ಧ್ರುವ lowerp
            = ನೂತನ AstCond{fl, ನೂತನ AstLtS{fl, ref(lop), minp}, minp->clone_ವೃಕ್ಷ(ಅಸತ್ಯ), ref(lop)};
        AstCond* ಧ್ರುವ upperp
            = ನೂತನ AstCond{fl, ನೂತನ AstGtS{fl, ref(hip), maxp}, maxp->clone_ವೃಕ್ಷ(ಅಸತ್ಯ), ref(hip)};
        lowerp->dtypeFrom(lop);
        upperp->dtypeFrom(lop);
        m_constructorp->addStmtsp(ನೂತನ AstAssign{fl, ref(lop, VAccess::WRITE), lowerp});
        m_constructorp->addStmtsp(ನೂತನ AstAssign{fl, ref(hip, VAccess::WRITE), upperp});
        m_constructorp->addStmtsp(ನೂತನ AstIf{fl, ನೂತನ AstLteS{fl, ref(lop), ref(hip)},
                                            itemCall(fl, cpVarp, method,
                                                     {resize_ಮೌಲ್ಯ(ref(lop), exprp->dtypep()),
                                                      resize_ಮೌಲ್ಯ(ref(hip), exprp->dtypep())})
                                                ->makeStmt()});
    }

    // The runtime index of the bin of a run holding the coverpoint value, which is in the run:
    // declared + (value - lo) / stride, capped at the last bin, which holds any remainder.
    ಸ್ಥಿರ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* runBinIndex(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* exprp, ಧ್ರುವ BinRun& run) {
        ಯದಿ (run.m_ಎಣಿಕೆ == 1) ಪ್ರತ್ಯಾಗಮ cnum(fl, run.m_declared);
        ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ width = exprp->width();
        // A run spans at most 2^width values, so offsets in it are unsigned width-bit numbers
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* indexp
            = ನೂತನ AstSub{fl, exprp->clone_ವೃಕ್ಷ(ಅಸತ್ಯ), new_ಮೌಲ್ಯ_Const(fl, run.m_lo, exprp)};
        indexp->dtypeSetBitSized(width, VSigning::UNSIGNED);
        V3_ಸಂಖ್ಯೆ stride{fl, width, 0};
        stride.opAssign(run.m_stride);
        ಯದಿ (stride.ಎಣಿಕೆ_Ones() != 1) {
            indexp = ನೂತನ AstDiv{fl, indexp, ನೂತನ AstConst{fl, stride}};
        } ಅನ್ಯಥಾ ಯದಿ (!stride.isEqOne()) {
            indexp = ನೂತನ AstShiftR{fl, indexp, ನೂತನ AstConst{fl, stride.mostSetBitP1() - 1}};
        }
        // Compare the run's values with those of count bins of stride values, without overflow
        ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ extWidth = run.m_lo.width() + 1;
        V3_ಸಂಖ್ಯೆ lo{fl, extWidth, 0};
        lo.opExtendS(run.m_lo, run.m_lo.width());
        V3_ಸಂಖ್ಯೆ span{fl, extWidth, 0};
        span.opExtendS(run.m_hi, run.m_hi.width());
        span.opSub(V3_ಸಂಖ್ಯೆ{span}, lo);
        V3_ಸಂಖ್ಯೆ covered{fl, extWidth, 0};
        covered.opAssign(run.m_stride);
        covered.opMul(V3_ಸಂಖ್ಯೆ{covered}, V3_ಸಂಖ್ಯೆ{fl, extWidth, run.m_ಎಣಿಕೆ});
        V3_ಸಂಖ್ಯೆ hasRemainder{fl, 1, 0};
        ಯದಿ (!hasRemainder.opGte(span, covered).isEqZero()) {
            AstConst* ಧ್ರುವ lastp = ನೂತನ AstConst{fl, V3_ಸಂಖ್ಯೆ{fl, width, run.m_ಎಣಿಕೆ - 1}};
            indexp = ನೂತನ AstCond{fl, ನೂತನ AstGt{fl, indexp->clone_ವೃಕ್ಷ(ಅಸತ್ಯ), lastp},
                                 lastp->clone_ವೃಕ್ಷ(ಅಸತ್ಯ), indexp};
        }
        ಯದಿ (width < VL_IDATASIZE) {
            indexp = ನೂತನ AstExtend{fl, indexp, VL_IDATASIZE};
        } ಅನ್ಯಥಾ ಯದಿ (width > VL_IDATASIZE) {
            indexp = ನೂತನ AstSel{fl, indexp, 0, VL_IDATASIZE};
        }
        ಪ್ರತ್ಯಾಗಮ ನೂತನ AstAdd{fl, cnum(fl, run.m_declared), indexp};
    }

    // Emit the sample() hit of a run of bins, whose code does not grow with its number of bins:
    //   if (iff && lo <= value && value <= hi) m_cp.incrementBin(<runBinIndex>);
    ಶೂನ್ಯ emitRunHit(AstCoverpoint* coverpointp, AstCoverBin* binp, AstVar* cpVarp,
                    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* exprp, ಧ್ರುವ BinRun& run) {
        File_ಸಾಲು* ಧ್ರುವ fl = binp->fileline();
        AstConst* ಧ್ರುವ lop = new_ಮೌಲ್ಯ_Const(fl, run.m_lo, exprp);
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* condp = nullptr;
        ಯದಿ (run.m_lo.isCaseEq(run.m_hi)) {
            condp = ನೂತನ AstEq{fl, exprp->clone_ವೃಕ್ಷ(ಅಸತ್ಯ), lop};
        } ಅನ್ಯಥಾ {
            AstConst* ಧ್ರುವ hip = new_ಮೌಲ್ಯ_Const(fl, run.m_hi, exprp);
            condp = makeRangeCondition(fl, exprp, lop, hip);
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(lop), lop);
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(hip), hip);
        }
        emitConvHitIf(coverpointp, binp, cpVarp, runBinIndex(fl, exprp, run), condp);
    }

    // Emit a transition bin's hit action into sample():
    //   if (cond) { m_cp.incrementBin/recordHit(idx); [illegal: $error; $stop] }
    // Used by the transition generators so a completed sequence records into the runtime bin.
    ಶೂನ್ಯ addConvTransHitIf(AstCoverpoint* coverpointp, AstCoverBin* binp, ಧ್ರುವ ConvBinTarget& tgt,
                           Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* condp) {
        File_ಸಾಲು* ಧ್ರುವ fl = binp->fileline();
        Ast_ಗಂಟು* actionp = makeRuntimeBinHit(fl, tgt);
        ಯದಿ (binp->binsType() == VCoverBinsType::BINS_ILLEGAL) {
            actionp->addNext(makeIllegalBinAction(
                fl, "Illegal transition bin " + binp->prettyNameQ() + " hit in coverpoint "
                        + coverpointp->prettyNameQ()));
        }
        UASSERT_OBJ(m_sampleFuncp, binp, "sample() CFunc not set for transition bin");
        addSampleStmt(ನೂತನ AstIf{fl, condp, actionp, nullptr});
    }

    // Route a coverpoint through a VlCoverpoint member: emit the member, its sample()
    // increments, the constructor configuration (init + namers), and registration.
    ಶೂನ್ಯ generateCoverpoint(AstCoverpoint* coverpointp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* exprp, ಪೂರ್ಣಾಙ್ಕ at_Least_ಮೌಲ್ಯ) {
        File_ಸಾಲು* ಧ್ರುವ fl = coverpointp->fileline();
        ಧ್ರುವ ತಾರ್ಕಿಕ dynamic = m_runtimePoints.ಎಣಿಕೆ(coverpointp);
        UINFO(4, "  Generating VlCoverpoint member: " << coverpointp->name());

        // Size the hit list to the gen-time max bin overlap (1 unless cross-fed with
        // overlapping ranges), so no cross hit is ever dropped and storage is minimal.
        ಧ್ರುವ ತಾರ್ಕಿಕ crossFed = m_crossedCpNames.ಎಣಿಕೆ(coverpointp->name()) != 0;
        ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ hitBound = computeHitListBound(coverpointp, exprp, crossFed);
        UINFO(6, "    Hit-list bound (max bin overlap) = " << hitBound);
        AstVar* ಧ್ರುವ cpVarp = ನೂತನ AstVar{fl, VVarType::MEMBER, "__Vcp_" + coverpointp->name(),
                                          coverpointDType(fl, static_cast<uint32_t>(hitBound))};
        m_covergroupp->addMembersp(cpVarp);
        m_cpVarMap[coverpointp->name()] = cpVarp;
        m_cpBins.emplace(cpVarp, CoverpointBins{});
        m_cpBins.at(cpVarp).exprp = exprp;
        m_cpBins.at(cpVarp).crossed = crossFed;
        // Create the runtime in the instance node first; everything below configures it.
        m_constructorp->addStmtsp(makeItemCreate(fl, cpVarp, VCMethod::COVERGROUP_ADD_COVERPOINT));
        generateItemWeight(fl, cpVarp, coverpointp->optionsp());

        // A cross reads this coverpoint's hit list, so clear it at the start of the
        // coverpoint's sample() contribution (before any incrementBin appends to it), even
        // when the iff guard disables sampling.
        ಯದಿ (crossFed) {
            UASSERT_OBJ(m_sampleFuncp, coverpointp, "sample() CFunc not set for clearHitList");
            m_sampleFuncp->addStmtsp(
                itemCall(fl, cpVarp, VCMethod::COVERGROUP_CLEAR_HIT_LIST)->makeStmt());
        }
        ಯದಿ (dynamic && coverpointHasStateExclusions(coverpointp)) {
            AstVar* ಧ್ರುವ excludedp
                = ನೂತನ AstVar{fl, VVarType::BLOCKTEMP,
                             "__VcpExcluded_" + sanitizeGeneratedName(coverpointp->name()),
                             coverpointp->findBitDType()};
            excludedp->funcLocal(ಸತ್ಯ);
            m_sampleFuncp->addStmtsp(excludedp);
            AstCMethodHard* ಧ್ರುವ callp
                = itemCall(fl, cpVarp,
                           exprp->isWide() ? VCMethod::COVERGROUP_ಮೌಲ್ಯ_EXCLUDED_W
                                           : VCMethod::COVERGROUP_ಮೌಲ್ಯ_EXCLUDED,
                           {exprp->clone_ವೃಕ್ಷ(ಅಸತ್ಯ)});
            callp->dtypeSetBit();
            addSampleStmt(ನೂತನ AstAssign{fl, ನೂತನ AstVarRef{fl, excludedp, VAccess::WRITE}, callp});
            m_excludedVars.emplace(cpVarp, excludedp);
        }

        // Walk bins (non-default, then default), assigning sequential indices that match the
        // namer append order; emit sample increments and collect namer statements.  Constructed
        // bins follow them all, placed when the coverpoint is constructed.
        std::vector<Ast_ಗಂಟು_Stmt*> namerStmts;
        std::vector<AstCoverBin*> defaultBins;
        std::vector<AstCoverBin*> sizedBins;
        std::vector<std::tuple<AstCoverBin*, uint32_t, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ*>> metadata;
        std::vector<ಧ್ರುವ BinRun*> runMetadata;
        uint64_t idx = 0;  // Runtime index of the next bin; 32-bit once checked below
        ಪ್ರತಿ (Ast_ಗಂಟು* binp = coverpointp->binsp(); binp; binp = binp->nextp()) {
            AstCoverBin* ಧ್ರುವ cbinp = VN_AS(binp, CoverBin);
            ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ errorsBefore = dynamic ? V3Error::error_ಎಣಿಕೆ() : 0;
            ಯದಿ (cbinp->binsType() == VCoverBinsType::BINS_DEFAULT) {
                defaultBins.ತಳ್ಳು_back(cbinp);
                ಅನುವರ್ತ;
            }
            ಯದಿ (isConstructedBins(cbinp)) {
                UASSERT_OBJ(dynamic, cbinp, "Constructed bins without value metadata");
                BinSpan span;
                span.sized = static_cast<int32_t>(sizedBins.size());
                m_cpBins.at(cpVarp).spans.emplace(cbinp->name(), span);
                sizedBins.ತಳ್ಳು_back(cbinp);
                ಅನುವರ್ತ;
            }
            ಯದಿ (cbinp->transp()) {
                // Transition bin (incl. array transition 'bins t[] = (a=>b),(c=>d)' and
                // illegal_bins/ignore_bins transitions).  All sequences of one transition bin
                // share a bin name and merge in the coverage DB to a single point, so model
                // them as one runtime bin incremented by any matching sequence.  The sequence
                // matching is generated as a state machine, with the hit routed to this bin's
                // runtime slot.
                namerStmts.ತಳ್ಳು_back(makeNamer(cpVarp, cbinp, -1, static_cast<uint32_t>(idx)));
                ಧ್ರುವ ConvBinTarget tgt{cpVarp, static_cast<uint32_t>(idx),
                                        cbinp->binsType().binIsNormal()};
                ಪ್ರತಿ (Ast_ಗಂಟು* sp = cbinp->transp(); sp; sp = sp->nextp())
                    generateSingleTransitionCode(coverpointp, cbinp, exprp, tgt,
                                                 VN_AS(sp, CoverTransSet));
                ಯದಿ (dynamic && V3Error::error_ಎಣಿಕೆ() == errorsBefore) {
                    metadata.emplace_back(cbinp, static_cast<uint32_t>(idx), nullptr);
                }
                ++idx;
                ಅನುವರ್ತ;
            }
            BinRuns plan;
            ಯದಿ (binRunsFor(cbinp, exprp, plan)) {
                // Array elements and automatic bins generate as runs, so neither sample() nor
                // the constructor grows with their number of bins.
                ಯದಿ (plan.unsupported) {  // bin ignored or invalid; reserve no slot
                    m_droppedBins[coverpointp].ತಳ್ಳು_back(cbinp->name());
                    ಅನುವರ್ತ;
                }
                CoverpointBins& bins = m_cpBins.at(cpVarp);
                ಧ್ರುವ uint32_t first_ಮೌಲ್ಯ = bins.total;
                ಧ್ರುವ uint32_t firstDeclared = static_cast<uint32_t>(idx);
                namerStmts.ತಳ್ಳು_back(
                    makeNamer(cpVarp, cbinp, plan.ಎಣಿಕೆ, firstDeclared, {}, plan.ಮೌಲ್ಯಗಳು));
                ಪ್ರತಿ (BinRun& run : plan.runs) {
                    run.m_declared = static_cast<uint32_t>(idx);
                    bins.runs.ತಳ್ಳು_back(std::move(run));
                    ಧ್ರುವ BinRun& stored = bins.runs.back();
                    ಯದಿ (bins.crossed && cbinp->binsType().binIsNormal()) {
                        ಧ್ರುವ uint32_t first = first_ಮೌಲ್ಯ + stored.m_declared - firstDeclared;
                        ಪ್ರತಿ (uint32_t element = 0; element < stored.m_ಎಣಿಕೆ; ++element) {
                            bins.ಮೌಲ್ಯಗಳು[first + element].runp = &stored;
                            bins.ಮೌಲ್ಯಗಳು[first + element].element = element;
                        }
                    }
                    ಯದಿ (!stored.m_empty) {
                        emitRunHit(coverpointp, cbinp, cpVarp, exprp, stored);
                        ಯದಿ (dynamic && V3Error::error_ಎಣಿಕೆ() == errorsBefore) {
                            runMetadata.ತಳ್ಳು_back(&stored);
                        }
                    }
                    idx += stored.m_ಎಣಿಕೆ;
                }
                ಅನುವರ್ತ;
            }
            ಯದಿ (cbinp->isArray()) {  // value array of a real coverpoint: b[0]..b[N-1]
                // Only integral coverpoints have runtime value metadata (m_runtimePoints)
                UASSERT_OBJ(!dynamic, cbinp, "Runtime value metadata for a real coverpoint");
                ತಾರ್ಕಿಕ unsupported = ಅಸತ್ಯ;
                std::vector<Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ*> ಮೌಲ್ಯಗಳು = extract_Array_ಮೌಲ್ಯಗಳು(cbinp, exprp, unsupported);
                ಯದಿ (unsupported) {  // bin ignored (COVERIGN emitted); reserve no slot
                    m_droppedBins[coverpointp].ತಳ್ಳು_back(cbinp->name());
                    ಅನುವರ್ತ;
                }
                namerStmts.ತಳ್ಳು_back(makeNamer(cpVarp, cbinp, static_cast<int64_t>(ಮೌಲ್ಯಗಳು.size()),
                                               static_cast<uint32_t>(idx), ಮೌಲ್ಯಗಳು));
                ಪ್ರತಿ (Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* valuep : ಮೌಲ್ಯಗಳು) {
                    // The cross selections of this covergroup still read the value.
                    m_detached_ಮೌಲ್ಯಗಳು.ತಳ್ಳು_back(valuep);
                    emitConvHitIf(coverpointp, cbinp, cpVarp,
                                  cnum(cbinp->fileline(), static_cast<uint32_t>(idx)),
                                  build_ಮೌಲ್ಯ_Condition(cbinp, exprp, valuep));
                    ++idx;
                }
            } ಅನ್ಯಥಾ {
                namerStmts.ತಳ್ಳು_back(makeNamer(cpVarp, cbinp, -1, static_cast<uint32_t>(idx)));
                // buildBinCondition is null for 'ignore_bins = default' (no ranges); the bin
                // still gets a reserved slot (recorded, never incremented).
                ಯದಿ (Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ condp = buildBinCondition(cbinp, exprp))
                    emitConvHitIf(coverpointp, cbinp, cpVarp,
                                  cnum(cbinp->fileline(), static_cast<uint32_t>(idx)), condp);
                ಯದಿ (dynamic && V3Error::error_ಎಣಿಕೆ() == errorsBefore) {
                    metadata.emplace_back(cbinp, static_cast<uint32_t>(idx), nullptr);
                }
                ++idx;
            }
        }
        // A cross selecting an ignored bins declaration selects no bins
        ಪ್ರತಿ (ಧ್ರುವ std::string& name : m_droppedBins[coverpointp]) {
            m_cpBins.at(cpVarp).spans.emplace(name, BinSpan{});
        }
        // A value a constructed bin holds is no default bin's; only sampling tells which do
        AstVar* sizedMatchedp = nullptr;
        ಯದಿ (!defaultBins.empty()
            && std::any_of(sizedBins.begin(), sizedBins.end(), [](ಧ್ರುವ AstCoverBin* binp) {
                   ಪ್ರತ್ಯಾಗಮ binp->binsType().binIsNormal();
               })) {
            sizedMatchedp = ನೂತನ AstVar{fl, VVarType::BLOCKTEMP,
                                       "__VcpSized_" + sanitizeGeneratedName(coverpointp->name()),
                                       coverpointp->findBitDType()};
            sizedMatchedp->funcLocal(ಸತ್ಯ);
            m_sampleFuncp->addStmtsp(sizedMatchedp);
            addSampleStmt(ನೂತನ AstAssign{fl, ನೂತನ AstVarRef{fl, sizedMatchedp, VAccess::WRITE},
                                        ನೂತನ AstConst{fl, AstConst::BitFalse{}}});
        }
        ಪ್ರತಿ (uint32_t sized = 0; sized < sizedBins.size(); ++sized) {
            emitSizedSample(coverpointp, sizedBins[sized], cpVarp, exprp, sized, sizedMatchedp);
        }
        ಪ್ರತಿ (AstCoverBin* ಧ್ರುವ defBinp : defaultBins) {
            File_ಸಾಲು* ಧ್ರುವ dfl = defBinp->fileline();
            namerStmts.ತಳ್ಳು_back(makeNamer(cpVarp, defBinp, -1, static_cast<uint32_t>(idx)));
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* condp = buildDefaultCondition(coverpointp, exprp, dfl);
            ಯದಿ (sizedMatchedp) {
                condp = ನೂತನ AstLogAnd{
                    dfl, ನೂತನ AstNot{dfl, ನೂತನ AstVarRef{dfl, sizedMatchedp, VAccess::READ}}, condp};
            }
            emitConvHitIf(coverpointp, defBinp, cpVarp, cnum(dfl, static_cast<uint32_t>(idx)),
                          condp);
            ++idx;
        }
        ಯದಿ (idx > std::numeric_limits<uint32_t>::max()) {
            // The runtime indexes bins with 32 bits; stop before generating a model
            coverpointp->v3warn(E_UNSUPPORTED, "Unsupported: coverpoint with more than "
                                                   << std::numeric_limits<uint32_t>::max()
                                                   << " bins");
        }

        // Transition coverpoints track the previous sampled value; update it once at the end of
        // this coverpoint's sample() contribution (the prev var was created on demand by the
        // transition matching above).
        ಯದಿ (coverpointHasTransition(coverpointp)) {
            AstVar* ಧ್ರುವ prevVarp = VN_AS(coverpointp->user1p(), Var);
            addSampleStmt(
                ನೂತನ AstAssign{coverpointp->fileline(),
                              ನೂತನ AstVarRef{prevVarp->fileline(), prevVarp, VAccess::WRITE},
                              exprp->clone_ವೃಕ್ಷ(ಅಸತ್ಯ)});
        }

        // Constructor: init (allocates), namers, then registration (under --coverage).
        // Under --protect-ids the hierarchy and page string reach the coverage database
        // verbatim, so obfuscate them like line/toggle points (per-word hierarchy, whole-
        // unit page).  No-ops when --protect-ids is off.
        ಧ್ರುವ ತಾರ್ಕಿಕ prot = v3Global.opt.protectIds();
        ಧ್ರುವ std::string hier
            = VIdProtect::protect_ಪದಗಳು_If(m_covergroupName + "." + coverpointp->name(), prot);
        m_constructorp->addStmtsp(
            itemCall(fl, cpVarp, VCMethod::COVERGROUP_INIT,
                     {ctext(fl, quoted(hier)), cnum(fl, static_cast<uint32_t>(at_Least_ಮೌಲ್ಯ)),
                      cnum(fl, static_cast<uint32_t>(idx))})
                ->makeStmt());
        ಪ್ರತಿ (Ast_ಗಂಟು_Stmt* ಧ್ರುವ ns : namerStmts) m_constructorp->addStmtsp(ns);
        ಯದಿ (dynamic) {
            m_constructorp->addStmtsp(
                itemCall(fl, cpVarp, VCMethod::COVERGROUP_ಮೌಲ್ಯ_TYPE,
                         {cnum(fl, exprp->width()), cnum(fl, exprp->isSigned())})
                    ->makeStmt());
            ಮೌಲ್ಯ_Lists lists;
            ಪ್ರತಿ (ಧ್ರುವ auto& entry : metadata) {
                collect_ಮೌಲ್ಯ_Metadata(lists, exprp, std::get<0>(entry), std::get<1>(entry),
                                     std::get<2>(entry));
            }
            ಪ್ರತಿ (ಧ್ರುವ BinRun* ಧ್ರುವ runp : runMetadata) collectRunMetadata(lists, exprp, *runp);
            emit_ಮೌಲ್ಯ_List(fl, cpVarp, VCMethod::COVERGROUP_ಮೌಲ್ಯ_RANGES, lists.m_ranges);
            emit_ಮೌಲ್ಯ_List(fl, cpVarp, VCMethod::COVERGROUP_ಮೌಲ್ಯ_RUNS, lists.m_runs);
            emit_ಮೌಲ್ಯ_List(fl, cpVarp, VCMethod::COVERGROUP_ಮೌಲ್ಯ_PATTERNS, lists.m_patterns);
            emit_ಮೌಲ್ಯ_List(fl, cpVarp, VCMethod::COVERGROUP_ಮೌಲ್ಯ_TRANSITIONS, lists.m_transitions);
            ಪ್ರತಿ (AstCoverBin* ಧ್ರುವ binp : sizedBins) {
                generateConstructedBins(coverpointp, binp, cpVarp, exprp);
            }
            m_constructorp->addStmtsp(
                itemCall(fl, cpVarp, VCMethod::COVERGROUP_ಮೌಲ್ಯ_FINALIZE)->makeStmt());
        }
        ಯದಿ (v3Global.opt.coverage()) {
            ಧ್ರುವ std::string page
                = VIdProtect::protectIf("v_covergroup/" + m_covergroupName, prot);
            m_constructorp->addStmtsp(
                itemCall(fl, cpVarp, VCMethod::COVERGROUP_REGISTER_BINS,
                         {ctext(fl, "vlSymsp->_vm_contextp__->coveragep()"),
                          ctext(fl, quoted(page)),
                          cnum(fl, itemDatabaseWeight(coverpointp->optionsp())),
                          cnum(fl, m_cgTypeWeight)})
                    ->makeStmt());
        }
    }

    // Generate state machine code for multi-value transition sequences
    // Handles transitions like (1 => 2 => 3 => 4)
    ಶೂನ್ಯ generate_Multi_ಮೌಲ್ಯ_Transition_Code(AstCoverpoint* coverpointp, AstCoverBin* binp,
                                          Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* exprp, ಧ್ರುವ ConvBinTarget& tgt,
                                          ಧ್ರುವ std::vector<AstCoverTransItem*>& ವಸ್ತುಗಳು) {
        UINFO(4, "    Generating multi-value transition state machine for: " << binp->name());
        UINFO(4, "      Sequence length: " << ವಸ್ತುಗಳು.size() << " items");

        // Create state position variable
        AstVar* ಧ್ರುವ stateVarp = createSequenceStateVar(coverpointp, binp);

        // Build case statement with N cases (one for each state 0 to N-1)
        // State 0: Not started, looking for first item
        // State 1 to N-1: In progress, looking for next item

        AstCase* ಧ್ರುವ casep
            = ನೂತನ AstCase{binp->fileline(), VCaseType::CT_CASE,
                          ನೂತನ AstVarRef{stateVarp->fileline(), stateVarp, VAccess::READ}, nullptr};

        // Generate each case item in the switch statement
        ಪ್ರತಿ (size_t state = 0; state < ವಸ್ತುಗಳು.size(); ++state) {
            AstCaseItem* caseItemp = generateTransitionStateCase(coverpointp, binp, exprp, tgt,
                                                                 stateVarp, ವಸ್ತುಗಳು, state);
            casep->addItemsp(caseItemp);
        }

        // Add default case (reset to state 0) to prevent CASEINCOMPLETE warnings,
        // since the state variable is wider than the number of valid states.
        AstCaseItem* ಧ್ರುವ defaultItemp = ನೂತನ AstCaseItem{
            binp->fileline(), nullptr,
            ನೂತನ AstAssign{binp->fileline(),
                          ನೂತನ AstVarRef{binp->fileline(), stateVarp, VAccess::WRITE},
                          ನೂತನ AstConst{binp->fileline(), AstConst::Widthed_ಮೌಲ್ಯ{}, 8, 0}}};
        casep->addItemsp(defaultItemp);

        addSampleStmt(casep);
        UINFO(4, "      Successfully added multi-value transition state machine");
    }

    // Generate code for a single state in the transition state machine
    // Returns the case item for this state
    AstCaseItem* generateTransitionStateCase(AstCoverpoint* coverpointp, AstCoverBin* binp,
                                             Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* exprp, ಧ್ರುವ ConvBinTarget& tgt,
                                             AstVar* stateVarp,
                                             ಧ್ರುವ std::vector<AstCoverTransItem*>& ವಸ್ತುಗಳು,
                                             size_t state) {
        File_ಸಾಲು* ಧ್ರುವ fl = binp->fileline();

        // Build condition for current value matching expected item at this state
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ matchCondp = buildTransitionItemCondition(ವಸ್ತುಗಳು[state], exprp);

        Ast_ಗಂಟು_Stmt* matchActionp = nullptr;

        ಯದಿ (state == ವಸ್ತುಗಳು.size() - 1) {
            // Last state: sequence complete!  Record the hit in the runtime VlCoverpoint.
            matchActionp = makeRuntimeBinHit(fl, tgt);

            // For illegal_bins, add error message
            ಯದಿ (binp->binsType() == VCoverBinsType::BINS_ILLEGAL) {
                ಧ್ರುವ string err_ಸಂದೇಶ = "Illegal transition bin " + binp->prettyNameQ()
                                      + " hit in coverpoint " + coverpointp->prettyNameQ();
                matchActionp = matchActionp->addNext(makeIllegalBinAction(fl, err_ಸಂದೇಶ));
            }

            // Reset state to 0
            matchActionp = matchActionp->addNext(
                ನೂತನ AstAssign{fl, ನೂತನ AstVarRef{fl, stateVarp, VAccess::WRITE},
                              ನೂತನ AstConst{fl, AstConst::Widthed_ಮೌಲ್ಯ{}, 8, 0}});
        } ಅನ್ಯಥಾ {
            // Intermediate state: advance to next state
            matchActionp = ನೂತನ AstAssign{
                fl, ನೂತನ AstVarRef{fl, stateVarp, VAccess::WRITE},
                ನೂತನ AstConst{fl, AstConst::Widthed_ಮೌಲ್ಯ{}, 8, static_cast<uint32_t>(state + 1)}};
        }

        // Build restart logic: check if current value matches first item
        // If so, restart sequence from state 1 (even if we're in middle of sequence)
        Ast_ಗಂಟು_Stmt* noMatchActionp = nullptr;
        ಯದಿ (state > 0) {
            // Check if current value matches first item (restart condition)
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ restartCondp = buildTransitionItemCondition(ವಸ್ತುಗಳು[0], exprp);

            UASSERT_OBJ(restartCondp, ವಸ್ತುಗಳು[0],
                        "buildTransitionItemCondition returned nullptr for restart");

            // Restart to state 1
            Ast_ಗಂಟು_Stmt* restartActionp
                = ನೂತನ AstAssign{fl, ನೂತನ AstVarRef{fl, stateVarp, VAccess::WRITE},
                                ನೂತನ AstConst{fl, AstConst::Widthed_ಮೌಲ್ಯ{}, 8, 1}};

            // Reset to state 0 (else branch)
            Ast_ಗಂಟು_Stmt* resetActionp
                = ನೂತನ AstAssign{fl, ನೂತನ AstVarRef{fl, stateVarp, VAccess::WRITE},
                                ನೂತನ AstConst{fl, AstConst::Widthed_ಮೌಲ್ಯ{}, 8, 0}};

            noMatchActionp = ನೂತನ AstIf{fl, restartCondp, restartActionp, resetActionp};
        }
        // For state 0, no action needed if no match (stay in state 0)

        // Combine into if-else
        Ast_ಗಂಟು_Stmt* ಧ್ರುವ stmtp = ನೂತನ AstIf{fl, matchCondp, matchActionp, noMatchActionp};

        // Create case item for this state value
        AstCaseItem* ಧ್ರುವ caseItemp = ನೂತನ AstCaseItem{
            fl, ನೂತನ AstConst{fl, AstConst::Widthed_ಮೌಲ್ಯ{}, 8, static_cast<uint32_t>(state)},
            stmtp};

        ಪ್ರತ್ಯಾಗಮ caseItemp;
    }

    // Create: $error(msg); $stop;  Used when an illegal bin is hit.
    Ast_ಗಂಟು_Stmt* makeIllegalBinAction(File_ಸಾಲು* fl, ಧ್ರುವ string& err_ಸಂದೇಶ) {
        AstDisplay* ಧ್ರುವ errorp
            = ನೂತನ AstDisplay{fl, VDisplayType::DT_ERROR, err_ಸಂದೇಶ, nullptr, nullptr};
        errorp->fmtp()->timeunit(m_covergroupp->timeunit());
        static_cast<Ast_ಗಂಟು*>(errorp)->addNext(ನೂತನ AstStop{fl, ಸತ್ಯ});
        ಪ್ರತ್ಯಾಗಮ errorp;
    }

    // Preserve the coverpoint's width and signedness after V3Width.
    ಸ್ಥಿರ AstConst* new_ಮೌಲ್ಯ_Const(File_ಸಾಲು* fl, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& ಮೌಲ್ಯ, ಧ್ರುವ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* exprp) {
        V3_ಸಂಖ್ಯೆ narrowed{fl, exprp->width(), 0};
        narrowed.opAssign(ಮೌಲ್ಯ);
        AstConst* ಧ್ರುವ constp = ನೂತನ AstConst{fl, narrowed};
        constp->dtypeFrom(exprp);
        ಪ್ರತ್ಯಾಗಮ constp;
    }

    // A real copy of a real or integral range bound, for comparing with a real coverpoint.
    ಸ್ಥಿರ AstConst* newRealConst(AstConst* constp) {
        ಯದಿ (constp->ಅಂಕೆ().isDouble()) ಪ್ರತ್ಯಾಗಮ constp->clone_ವೃಕ್ಷ(ಅಸತ್ಯ);
        V3_ಸಂಖ್ಯೆ real{&constp->ಅಂಕೆ(), 64};
        real.opIToRD(constp->ಅಂಕೆ(), constp->isSigned());
        ಪ್ರತ್ಯಾಗಮ ನೂತನ AstConst{constp->fileline(), real};
    }

    // Clone a constant node, widening to targetWidth if needed (zero-extend).
    // Used to ensure comparisons use matching widths after V3Width has run.
    ಸ್ಥಿರ AstConst* widenConst(File_ಸಾಲು* fl, AstConst* constp, ಪೂರ್ಣಾಙ್ಕ targetWidth) {
        ಯದಿ (constp->width() == targetWidth) ಪ್ರತ್ಯಾಗಮ constp->clone_ವೃಕ್ಷ(ಅಸತ್ಯ);
        V3_ಸಂಖ್ಯೆ ಅಂಕೆ{fl, targetWidth, 0};
        ಅಂಕೆ.opAssign(constp->ಅಂಕೆ());
        ಪ್ರತ್ಯಾಗಮ ನೂತನ AstConst{fl, ಅಂಕೆ};
    }

    // Build a range condition: minp <= exprp <= maxp.
    // Uses signed comparisons if exprp is signed; omits trivially-true domain bounds.
    // All arguments are non-owning; clones exprp/minp/maxp as needed.
    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* makeRangeCondition(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* exprp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* minp,
                                    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* maxp) {
        ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ ಅಭಿವ್ಯಕ್ತಿ_Width = exprp->widthMin();
        AstConst* ಧ್ರುವ minConstp = VN_AS(minp, Const);
        AstConst* ಧ್ರುವ maxConstp = VN_AS(maxp, Const);
        ಯದಿ (exprp->isDouble()) {
            // A real coverpoint has no finite domain bounds to omit.
            ಪ್ರತ್ಯಾಗಮ ನೂತನ AstAnd{fl,
                              ನೂತನ AstGteD{fl, exprp->clone_ವೃಕ್ಷ(ಅಸತ್ಯ), newRealConst(minConstp)},
                              ನೂತನ AstLteD{fl, exprp->clone_ವೃಕ್ಷ(ಅಸತ್ಯ), newRealConst(maxConstp)}};
        }
        // Widen constants to match expression width so post-V3Width nodes use correct macros
        AstConst* ಧ್ರುವ minWidep = widenConst(fl, minConstp, ಅಭಿವ್ಯಕ್ತಿ_Width);
        AstConst* ಧ್ರುವ maxWidep = widenConst(fl, maxConstp, ಅಭಿವ್ಯಕ್ತಿ_Width);
        V3_ಸಂಖ್ಯೆ minimum{fl, ಅಭಿವ್ಯಕ್ತಿ_Width, 0};
        V3_ಸಂಖ್ಯೆ maximum{fl, ಅಭಿವ್ಯಕ್ತಿ_Width, 0};
        maximum.setAllBits1();
        ಯದಿ (exprp->isSigned()) {
            minimum.setBit(ಅಭಿವ್ಯಕ್ತಿ_Width - 1, 1);
            maximum.setBit(ಅಭಿವ್ಯಕ್ತಿ_Width - 1, 0);
        }
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lowerp = nullptr;
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* upperp = nullptr;
        ಯದಿ (minWidep->ಅಂಕೆ().isCaseEq(minimum)) {
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(minWidep), minWidep);
        } ಅನ್ಯಥಾ {
            lowerp = exprp->isSigned() ? static_cast<Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ*>(
                                             ನೂತನ AstGteS{fl, exprp->clone_ವೃಕ್ಷ(ಅಸತ್ಯ), minWidep})
                                       : static_cast<Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ*>(
                                             ನೂತನ AstGte{fl, exprp->clone_ವೃಕ್ಷ(ಅಸತ್ಯ), minWidep});
        }
        ಯದಿ (maxWidep->ಅಂಕೆ().isCaseEq(maximum)) {
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(maxWidep), maxWidep);
        } ಅನ್ಯಥಾ {
            upperp = exprp->isSigned() ? static_cast<Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ*>(
                                             ನೂತನ AstLteS{fl, exprp->clone_ವೃಕ್ಷ(ಅಸತ್ಯ), maxWidep})
                                       : static_cast<Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ*>(
                                             ನೂತನ AstLte{fl, exprp->clone_ವೃಕ್ಷ(ಅಸತ್ಯ), maxWidep});
        }
        ಯದಿ (lowerp && upperp) ಪ್ರತ್ಯಾಗಮ ನೂತನ AstAnd{fl, lowerp, upperp};
        ಯದಿ (lowerp) ಪ್ರತ್ಯಾಗಮ lowerp;
        ಯದಿ (upperp) ಪ್ರತ್ಯಾಗಮ upperp;
        ಪ್ರತ್ಯಾಗಮ ನೂತನ AstConst{fl, AstConst::BitTrue{}};
    }

    // Build a one-sided comparison for an open-ended bin range whose other bound is '$'.
    // '$' denotes the coverpoint domain extreme, so {[lo:$]} == (expr >= lo) and
    // {[$:hi]} == (expr <= hi).
    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* makeOpenRangeCondition(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* exprp, AstConst* boundp,
                                        ತಾರ್ಕಿಕ isLowerBound) {
        AstConst* ಧ್ರುವ widep = widenConst(fl, boundp, exprp->widthMin());
        ಯದಿ (isLowerBound) {
            ಯದಿ (exprp->isSigned()) ಪ್ರತ್ಯಾಗಮ ನೂತನ AstGteS{fl, exprp->clone_ವೃಕ್ಷ(ಅಸತ್ಯ), widep};
            ಪ್ರತ್ಯಾಗಮ ನೂತನ AstGte{fl, exprp->clone_ವೃಕ್ಷ(ಅಸತ್ಯ), widep};
        }
        ಯದಿ (exprp->isSigned()) ಪ್ರತ್ಯಾಗಮ ನೂತನ AstLteS{fl, exprp->clone_ವೃಕ್ಷ(ಅಸತ್ಯ), widep};
        ಪ್ರತ್ಯಾಗಮ ನೂತನ AstLte{fl, exprp->clone_ವೃಕ್ಷ(ಅಸತ್ಯ), widep};
    }

    // Build condition for a single transition item.
    // Returns expression that checks if exprp matches the item's value/range list.
    // Overload for when the expression is a variable read -- creates and manages the VarRef
    // internally, so callers don't need to construct a temporary node.
    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* buildTransitionItemCondition(AstCoverTransItem* itemp, AstVar* varp) {
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* varRefp = ನೂತನ AstVarRef{varp->fileline(), varp, VAccess::READ};
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ condp = buildTransitionItemCondition(itemp, varRefp);
        VL_DO_DANGLING(ತಳ್ಳು_Deletep(varRefp), varRefp);
        ಪ್ರತ್ಯಾಗಮ condp;
    }

    // Non-owning: exprp is cloned internally; caller retains ownership of exprp.
    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* buildTransitionItemCondition(AstCoverTransItem* itemp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* exprp) {
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* condp = nullptr;

        ಪ್ರತಿ (Ast_ಗಂಟು* valp = itemp->valuesp(); valp; valp = valp->nextp()) {
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* singleCondp = nullptr;
            valp = V3Const::constifyEdit(valp);
            AstConst* ಧ್ರುವ constp = VN_CAST(valp, Const);
            ಯದಿ (!constp) {
                valp->v3error("Non-constant expression in transition bin; "
                              "values must be constants (IEEE 1800-2023 19.5)");
                ಪ್ರತ್ಯಾಗಮ ನೂತನ AstConst{valp->fileline(), AstConst::BitFalseErroring{}};
            }
            singleCondp
                = ನೂತನ AstEq{constp->fileline(), exprp->clone_ವೃಕ್ಷ(ಅಸತ್ಯ), constp->clone_ವೃಕ್ಷ(ಅಸತ್ಯ)};
            ಯದಿ (condp) {
                condp = ನೂತನ AstOr{itemp->fileline(), condp, singleCondp};
            } ಅನ್ಯಥಾ {
                condp = singleCondp;
            }
        }

        ಪ್ರತ್ಯಾಗಮ condp;
    }

    // Generate code for a single transition sequence (used by both regular and array bins)
    ಶೂನ್ಯ generateSingleTransitionCode(AstCoverpoint* coverpointp, AstCoverBin* binp,
                                      Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* exprp, ಧ್ರುವ ConvBinTarget& tgt,
                                      AstCoverTransSet* transSetp) {
        UINFO(4, "      Generating code for transition sequence");

        // Get or create previous value variable
        AstVar* ಧ್ರುವ prevVarp = create_Prev_ಮೌಲ್ಯ_Var(coverpointp, exprp);

        UASSERT_OBJ(
            transSetp, binp,
            "Transition bin has no transition set (transp() was checked before calling this)");

        // Get transition items (the sequence: item1 => item2 => item3)
        std::vector<AstCoverTransItem*> ವಸ್ತುಗಳು;
        ಪ್ರತಿ (Ast_ಗಂಟು* itemp = transSetp->itemsp(); itemp; itemp = itemp->nextp())
            ವಸ್ತುಗಳು.ತಳ್ಳು_back(VN_AS(itemp, CoverTransItem));

        ಯದಿ (ವಸ್ತುಗಳು.empty()) {
            binp->v3error("Transition set without items");
            ಪ್ರತ್ಯಾಗಮ;
        }

        ಯದಿ (ವಸ್ತುಗಳು.size() == 1) {
            // Single item transition not valid (need at least 2 values for =>)
            binp->v3error("Transition requires at least two values");
            ಪ್ರತ್ಯಾಗಮ;
        } ಅನ್ಯಥಾ ಯದಿ (ವಸ್ತುಗಳು.size() == 2) {
            // Simple two-value transition: (val1 => val2)
            // Use optimized direct comparison (no state machine needed)
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ cond1p = buildTransitionItemCondition(ವಸ್ತುಗಳು[0], prevVarp);
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ cond2p = buildTransitionItemCondition(ವಸ್ತುಗಳು[1], exprp);

            // Combine: prev matches val1 AND current matches val2
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* fullCondp = ನೂತನ AstAnd{binp->fileline(), cond1p, cond2p};

            addConvTransHitIf(coverpointp, binp, tgt, fullCondp);

            UINFO(4, "        Successfully added 2-value transition if statement");
        } ಅನ್ಯಥಾ {
            // Multi-value sequence (a => b => c => ...)
            // Use state machine to track position in sequence
            generate_Multi_ಮೌಲ್ಯ_Transition_Code(coverpointp, binp, exprp, tgt, ವಸ್ತುಗಳು);
        }
    }

    // Append a "{ VlCoverpoint* __Vcx_cps[] = {cp0, cp1, ...}; <call> }" statement.  The brace
    // and the temporary array stay literal text -- a CMethodHard is one call, not a block --
    // but callp itself carries the member, method and '->'.  Construction only: init() copies
    // the array into the cross, so sample() reads it from there and needs no array at all.
    AstCStmt* makeCrossCpsCall(File_ಸಾಲು* fl, ಧ್ರುವ std::vector<AstVar*>& cpVars,
                               AstCMethodHard* callp) {
        AstCStmt* ಧ್ರುವ cs = ನೂತನ AstCStmt{fl};
        cs->add("{ VlCoverpoint* __Vcx_cps[] = {");
        ಪ್ರತಿ (size_t d = 0; d < cpVars.size(); ++d) {
            ಯದಿ (d != 0) cs->add(", ");
            cs->add(memberRef(fl, cpVars[d]));
        }
        cs->add("}; ");
        cs->add(callp);
        cs->add("; }");
        ಪ್ರತ್ಯಾಗಮ cs;
    }

    // Assign the per-bin flags individually: one-bit SV results have integer C++ storage types,
    // which may narrow in a bool initializer list but convert implicitly in assignments.
    AstCStmt* makeCrossIffsCall(File_ಸಾಲು* fl, ಧ್ರುವ std::vector<AstCoverCrossBin*>& bins,
                                AstCMethodHard* callp) {
        AstCStmt* ಧ್ರುವ cs = ನೂತನ AstCStmt{fl};
        cs->add("{ bool __Vcx_iffs[" + cvtToStr(bins.size()) + "]; ");
        ಪ್ರತಿ (size_t i = 0; i < bins.size(); ++i) {
            ಧ್ರುವ AstCoverCrossBin* ಧ್ರುವ binp = bins[i];
            cs->add("__Vcx_iffs[" + cvtToStr(i) + "] = ");
            cs->add(binp->iffp() ? binp->iffp()->clone_ವೃಕ್ಷ(ಅಸತ್ಯ)
                                 : ನೂತನ AstConst{fl, AstConst::BitTrue{}});
            cs->add("; ");
        }
        cs->add(callp);
        cs->add("; }");
        ಪ್ರತ್ಯಾಗಮ cs;
    }

    using CrossSelection = std::vector<uint64_t>;
    ಸಂರಚನಾ ResolvedCrossBin final {
        AstCoverCrossBin* binp;
        CrossSelection selection;
    };
    ಸಂರಚನಾ CrossLayout final {
        uint32_t tuples = 0;
        uint32_t autoBins = 0;
        uint64_t bin_ಪದಗಳು = 0;
        ತಾರ್ಕಿಕ valid = ಸತ್ಯ;
        std::vector<ResolvedCrossBin> bins;
    };
    ಸಂರಚನಾ CrossSelectionContext final {
        AstCoverCross* crossp;  // Cross whose tuple space is being selected
        ಧ್ರುವ std::vector<AstVar*>& cpVars;  // Feeding coverpoints in dimension order
        ಧ್ರುವ std::map<std::string, uint32_t>& dimensions;  // Coverpoint name -> dimension
        uint32_t tuples;  // Size of the Cartesian product
        std::vector<uint32_t> strides;  // Flat-index stride per dimension
        ತಾರ್ಕಿಕ valid = ಸತ್ಯ;  // False if this explicit bin cannot be implemented
    };
    ಸಂರಚನಾ CrossBinsofTarget final {
        ಧ್ರುವ CoverpointBins* m_binsp = nullptr;  // Declared bins; null after a reported error
        uint32_t m_dimension = 0;  // Coverpoint index within the cross
        uint32_t m_first = 0;  // First declared normal bin in the selected span
        uint32_t m_ಎಣಿಕೆ = 0;  // Number of declared normal bins in the selected span
        uint32_t m_declaredFirst = 0;  // First runtime bin index of the selected span
        uint32_t m_declaredEnd = UINT32_MAX;  // One past the last runtime bin index
        int32_t m_sized = -1;  // Index of the sized array holding the selected bins; or -1
    };
    ಸಂರಚನಾ Cross_ಮೌಲ್ಯ_Range final {
        V3_ಸಂಖ್ಯೆ lo;  // Inclusive lower bound, sign-extended to the comparison width
        V3_ಸಂಖ್ಯೆ hi;  // Inclusive upper bound
        V3_ಸಂಖ್ಯೆ pattern;  // Allowed bit values; all X for an ordinary interval
        ತಾರ್ಕಿಕ wildcard = ಅಸತ್ಯ;  // A wildcard singleton rather than an exact four-state value

        Cross_ಮೌಲ್ಯ_Range(Ast_ಗಂಟು* nodep, ಪೂರ್ಣಾಙ್ಕ width)
            : lo{nodep, width}
            , hi{nodep, width}
            , pattern{nodep, width} {
            pattern.setAllBitsX();
        }
    };

    ಸ್ಥಿರ std::vector<Ast_ಗಂಟು*> cross_Bin_ಮೌಲ್ಯಗಳು(ಧ್ರುವ Cross_Bin_ಮೌಲ್ಯಗಳು& bin) {
        ಯದಿ (bin.valuep) ಪ್ರತ್ಯಾಗಮ {bin.valuep};
        std::vector<Ast_ಗಂಟು*> ಮೌಲ್ಯಗಳು;
        ಯದಿ (bin.binp->transp()) {
            // IEEE 1800-2023 19.6.1: binsof uses the last value of each transition.
            ಪ್ರತಿ (Ast_ಗಂಟು* setp = bin.binp->transp(); setp; setp = setp->nextp()) {
                AstCoverTransItem* lastp = VN_AS(setp, CoverTransSet)->itemsp();
                ಯಾವತ್ (lastp->nextp()) lastp = VN_AS(lastp->nextp(), CoverTransItem);
                ಪ್ರತಿ (Ast_ಗಂಟು* valuep = lastp->valuesp(); valuep; valuep = valuep->nextp()) {
                    ಮೌಲ್ಯಗಳು.ತಳ್ಳು_back(valuep);
                }
            }
        } ಅನ್ಯಥಾ {
            ಪ್ರತಿ (Ast_ಗಂಟು* valuep = bin.binp->rangesp(); valuep; valuep = valuep->nextp()) {
                ಮೌಲ್ಯಗಳು.ತಳ್ಳು_back(valuep);
            }
        }
        ಪ್ರತ್ಯಾಗಮ ಮೌಲ್ಯಗಳು;
    }

    // The values of the element-th bin of a run, sign-extended to 'width'
    ಸ್ಥಿರ Cross_ಮೌಲ್ಯ_Range runBinRange(Ast_ಗಂಟು* nodep, ಧ್ರುವ BinRun& run, uint32_t element,
                                       ಪೂರ್ಣಾಙ್ಕ width) {
        ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ runw = run.m_lo.width();
        V3_ಸಂಖ್ಯೆ offset{nodep, runw};
        offset.opMul(run.m_stride, V3_ಸಂಖ್ಯೆ{nodep, runw, element});
        V3_ಸಂಖ್ಯೆ lo{nodep, runw};
        lo.opAdd(run.m_lo, offset);
        V3_ಸಂಖ್ಯೆ hi = run.m_hi;
        ಯದಿ (element + 1 < run.m_ಎಣಿಕೆ) {
            offset.opSub(run.m_stride, V3_ಸಂಖ್ಯೆ{nodep, runw, 1});
            hi.opAdd(lo, offset);
        }
        Cross_ಮೌಲ್ಯ_Range range{nodep, width};
        range.lo.opExtendS(lo, runw);
        range.hi.opExtendS(hi, runw);
        ಪ್ರತ್ಯಾಗಮ range;
    }

    ಸ್ಥಿರ ಪೂರ್ಣಾಙ್ಕ crossRangeWidth(Ast_ಗಂಟು* nodep) {
        ಯದಿ (ಧ್ರುವ AstInsideRange* ಧ್ರುವ rangep = VN_CAST(nodep, InsideRange)) {
            ಪ್ರತ್ಯಾಗಮ std::max(rangep->lhsp()->width(), rangep->rhsp()->width());
        }
        ಪ್ರತ್ಯಾಗಮ nodep->width();
    }

    ಸ್ಥಿರ ತಾರ್ಕಿಕ cross_ಮೌಲ್ಯ_Less(ಧ್ರುವ V3_ಸಂಖ್ಯೆ& lhs, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& rhs) {
        V3_ಸಂಖ್ಯೆ result{&lhs};
        ಪ್ರತ್ಯಾಗಮ !result.opLtS(lhs, rhs).isEqZero();
    }

    // Round a real bound inward to the nearest coverpoint value (IEEE 1800-2023 19.5.7).  Sets
    // 'empty' if the domain has no value on the bound's side.
    ಸ್ಥಿರ ಶೂನ್ಯ crossRealBound(ಧ್ರುವ AstConst* constp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* exprp, ತಾರ್ಕಿಕ upper,
                               ಧ್ರುವ Cross_ಮೌಲ್ಯ_Range& domain, V3_ಸಂಖ್ಯೆ& result, ತಾರ್ಕಿಕ& empty) {
        ಧ್ರುವ ದ್ವಿಗುಣ ಮೌಲ್ಯ = constp->ಅಂಕೆ().toDouble();
        ಧ್ರುವ ದ್ವಿಗುಣ bound = upper ? std::floor(ಮೌಲ್ಯ) : std::ceil(ಮೌಲ್ಯ);
        // Powers of two are exact, so the integral bound compares exactly with the domain edges.
        ಧ್ರುವ ದ್ವಿಗುಣ limit
            = std::ldexp(1.0, exprp->isSigned() ? exprp->width() - 1 : exprp->width());
        ಧ್ರುವ ದ್ವಿಗುಣ minimum = exprp->isSigned() ? -limit : 0.0;
        ಯದಿ (std::isnan(bound) || (upper ? bound < minimum : bound >= limit)) {
            empty = ಸತ್ಯ;
            result = domain.lo;
        } ಅನ್ಯಥಾ ಯದಿ (bound < minimum) {
            result = domain.lo;
        } ಅನ್ಯಥಾ ಯದಿ (bound >= limit) {
            result = domain.hi;
        } ಅನ್ಯಥಾ {
            V3_ಸಂಖ್ಯೆ real{&result, 64};
            real.setDouble(bound);
            result.opRToIRoundS(real);
        }
    }

    ಸ್ಥಿರ ತಾರ್ಕಿಕ crossRangeBound(Ast_ಗಂಟು* nodep, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* exprp, ತಾರ್ಕಿಕ upper, ತಾರ್ಕಿಕ bin_ಮೌಲ್ಯ,
                                ಧ್ರುವ Cross_ಮೌಲ್ಯ_Range& domain, V3_ಸಂಖ್ಯೆ& result, ತಾರ್ಕಿಕ& empty) {
        ಯದಿ (VN_IS(nodep, Unbounded)) {
            V3_ಸಂಖ್ಯೆ limit{nodep, exprp->width()};
            ಯದಿ (upper) limit.setAllBits1();
            ಯದಿ (exprp->isSigned()) {
                limit.setBit(exprp->width() - 1, !upper);
                result.opExtendS(limit, limit.width());
            } ಅನ್ಯಥಾ {
                result.opAssign(limit);
            }
            ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;
        }
        ಧ್ರುವ AstConst* ಧ್ರುವ constp = VN_CAST(nodep, Const);
        ಯದಿ (!constp) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
        ಯದಿ (constp->ಅಂಕೆ().isDouble()) {
            crossRealBound(constp, exprp, upper, domain, result, empty);
            ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;
        }
        ಯದಿ (constp->ಅಂಕೆ().isString()) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
        ಯದಿ (bin_ಮೌಲ್ಯ && exprp->isSigned() && constp->width() <= exprp->width()) {
            // Bin bit patterns use the coverpoint's effective type (IEEE 1800-2023 19.5.7).
            // Wider values and intersect filters retain their values for domain clipping.
            V3_ಸಂಖ್ಯೆ ಮೌಲ್ಯ{nodep, exprp->width()};
            ಯದಿ (constp->isSigned()) {
                ಮೌಲ್ಯ.opExtendS(constp->ಅಂಕೆ(), constp->width());
            } ಅನ್ಯಥಾ {
                ಮೌಲ್ಯ.opAssign(constp->ಅಂಕೆ());
            }
            result.opExtendS(ಮೌಲ್ಯ, ಮೌಲ್ಯ.width());
        } ಅನ್ಯಥಾ ಯದಿ (constp->isSigned()) {
            result.opExtendS(constp->ಅಂಕೆ(), constp->width());
        } ಅನ್ಯಥಾ {
            result.opAssign(constp->ಅಂಕೆ());
        }
        ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;
    }

    ಸ್ಥಿರ Cross_ಮೌಲ್ಯ_Range cross_ಮೌಲ್ಯ_Domain(Ast_ಗಂಟು* nodep, ಪೂರ್ಣಾಙ್ಕ ಮೌಲ್ಯ_Width, ತಾರ್ಕಿಕ isSigned,
                                            ಪೂರ್ಣಾಙ್ಕ width) {
        Cross_ಮೌಲ್ಯ_Range domain{nodep, width};
        V3_ಸಂಖ್ಯೆ lo{nodep, ಮೌಲ್ಯ_Width};
        V3_ಸಂಖ್ಯೆ hi{nodep, ಮೌಲ್ಯ_Width};
        hi.setAllBits1();
        ಯದಿ (isSigned) {
            lo.setBit(ಮೌಲ್ಯ_Width - 1, 1);
            hi.setBit(ಮೌಲ್ಯ_Width - 1, 0);
            domain.lo.opExtendS(lo, ಮೌಲ್ಯ_Width);
            domain.hi.opExtendS(hi, ಮೌಲ್ಯ_Width);
        } ಅನ್ಯಥಾ {
            domain.lo.opAssign(lo);
            domain.hi.opAssign(hi);
        }
        ಪ್ರತ್ಯಾಗಮ domain;
    }

    ಸ್ಥಿರ ಶೂನ್ಯ intersectCrossRange(Cross_ಮೌಲ್ಯ_Range& range, ಧ್ರುವ Cross_ಮೌಲ್ಯ_Range& other) {
        ಯದಿ (cross_ಮೌಲ್ಯ_Less(range.lo, other.lo)) range.lo = other.lo;
        ಯದಿ (cross_ಮೌಲ್ಯ_Less(other.hi, range.hi)) range.hi = other.hi;
    }

    ಸ್ಥಿರ ತಾರ್ಕಿಕ cross_ಮೌಲ್ಯ_Range(Ast_ಗಂಟು* nodep, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* exprp, ತಾರ್ಕಿಕ bin_ಮೌಲ್ಯ, ತಾರ್ಕಿಕ wildcard,
                                ಧ್ರುವ Cross_ಮೌಲ್ಯ_Range& domain, Cross_ಮೌಲ್ಯ_Range& range) {
        ತಾರ್ಕಿಕ empty = ಅಸತ್ಯ;
        ಧ್ರುವ AstConst* ಧ್ರುವ constp = VN_CAST(nodep, Const);
        ಯದಿ (ಧ್ರುವ AstInsideRange* ಧ್ರುವ rangep = VN_CAST(nodep, InsideRange)) {
            ಯದಿ (!crossRangeBound(rangep->lhsp(), exprp, ಅಸತ್ಯ, bin_ಮೌಲ್ಯ, domain, range.lo, empty)
                || !crossRangeBound(rangep->rhsp(), exprp, ಸತ್ಯ, bin_ಮೌಲ್ಯ, domain, range.hi, empty)
                || range.lo.isFourState() || range.hi.isFourState()) {
                ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
            }
        } ಅನ್ಯಥಾ ಯದಿ (constp && constp->ಅಂಕೆ().isDouble()) {
            // A real value participates only if integral; it has no wildcard bits.
            crossRealBound(constp, exprp, ಅಸತ್ಯ, domain, range.lo, empty);
            crossRealBound(constp, exprp, ಸತ್ಯ, domain, range.hi, empty);
        } ಅನ್ಯಥಾ {
            ಯದಿ (!crossRangeBound(nodep, exprp, ಅಸತ್ಯ, bin_ಮೌಲ್ಯ, domain, range.lo, empty)) {
                ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
            }
            ಯದಿ (bin_ಮೌಲ್ಯ && exprp->isSigned() && constp && !constp->isSigned()
                && constp->width() > exprp->width()) {
                ತಾರ್ಕಿಕ representable = ಸತ್ಯ;
                ಪ್ರತಿ (ಪೂರ್ಣಾಙ್ಕ bit = exprp->width(); bit < constp->width(); ++bit) {
                    representable &= !constp->ಅಂಕೆ().bitIs1(bit);
                }
                ಯದಿ (representable) {
                    V3_ಸಂಖ್ಯೆ ಮೌಲ್ಯ{nodep, exprp->width()};
                    ಮೌಲ್ಯ.opAssign(constp->ಅಂಕೆ());
                    range.lo.opExtendS(ಮೌಲ್ಯ, ಮೌಲ್ಯ.width());
                }
            }
            range.hi = range.lo;
            range.wildcard = wildcard;
            ಯದಿ (wildcard) {
                range.pattern = range.lo;
                range.lo = domain.lo;
                range.hi = domain.hi;
                ಯದಿ (constp && constp->isSigned()) {
                    // Replicated X sign bits are correlated, not independent wildcards.
                    // The source domain preserves expansion-before-casting (19.5.7).
                    intersectCrossRange(
                        range, cross_ಮೌಲ್ಯ_Domain(nodep, constp->width(), ಸತ್ಯ, domain.lo.width()));
                }
            }
        }
        ಯದಿ (empty) {
            range.lo = domain.hi;
            range.hi = domain.lo;
            ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;
        }
        ಯದಿ (!range.lo.isFourState()) intersectCrossRange(range, domain);
        ಯದಿ (range.wildcard && exprp->isSigned()) {
            ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ sign = exprp->width() - 1;
            ಯದಿ (constp && !constp->isSigned() && constp->width() > exprp->width()) {
                // Unsigned equality preserves every target bit pattern if discarded bits are zero.
                ಪ್ರತಿ (ಪೂರ್ಣಾಙ್ಕ bit = exprp->width(); bit < constp->width(); ++bit) {
                    ಯದಿ (constp->ಅಂಕೆ().bitIs1(bit)) {
                        range.lo = domain.hi;
                        range.hi = domain.lo;
                        ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;
                    }
                }
                ಪ್ರತಿ (ಪೂರ್ಣಾಙ್ಕ bit = exprp->width(); bit < range.pattern.width(); ++bit) {
                    ಯದಿ (range.pattern.bitIsXZ(sign))
                        range.pattern.setBit(bit, 'x');
                    ಅನ್ಯಥಾ
                        range.pattern.setBit(bit, range.pattern.bitIs1(sign));
                }
                ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;
            }
            // Discarded high bits constrain the target sign, rather than becoming don't-cares.
            ಪ್ರತಿ (ಪೂರ್ಣಾಙ್ಕ bit = exprp->width(); bit < range.pattern.width(); ++bit) {
                ಯದಿ (range.pattern.bitIsXZ(bit)) ಅನುವರ್ತ;
                ಧ್ರುವ ತಾರ್ಕಿಕ ಮೌಲ್ಯ = range.pattern.bitIs1(bit);
                ಯದಿ (!range.pattern.bitIsXZ(sign) && range.pattern.bitIs1(sign) != ಮೌಲ್ಯ) {
                    range.lo = domain.hi;
                    range.hi = domain.lo;
                    ಭಙ್ಗ;
                }
                range.pattern.setBit(sign, ಮೌಲ್ಯ);
            }
        }
        ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;
    }

    ಗಣನಾ CrossRangeState : uint8_t {
        CROSS_INSIDE_BOUNDS = 0,  // Prefix is strictly inside the interval
        CROSS_AT_LOWER = 1,
        CROSS_AT_UPPER = 2,
        CROSS_AT_BOUNDS = CROSS_AT_LOWER | CROSS_AT_UPPER,
        CROSS_NO_MATCH = 4  // Prefix cannot match the interval/pattern
    };

    ಸ್ಥಿರ CrossRangeState crossRangeStep(ಧ್ರುವ Cross_ಮೌಲ್ಯ_Range& range, CrossRangeState state,
                                          ಪೂರ್ಣಾಙ್ಕ bit, ಪೂರ್ಣಾಙ್ಕ ಮೌಲ್ಯ) {
        ಯದಿ (state == CROSS_NO_MATCH) ಪ್ರತ್ಯಾಗಮ CROSS_NO_MATCH;
        // Flipping the sign bit makes signed order lexicographic.
        ಧ್ರುವ ತಾರ್ಕಿಕ sign = bit == range.pattern.width() - 1;
        ಯದಿ (!range.pattern.bitIsXZ(bit) && ಮೌಲ್ಯ != (range.pattern.bitIs1(bit) ^ sign)) {
            ಪ್ರತ್ಯಾಗಮ CROSS_NO_MATCH;
        }
        ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ ಕೆಳಗಿನ = range.lo.bitIs1(bit) ^ sign;
        ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ ಮೇಲಿನ = range.hi.bitIs1(bit) ^ sign;
        ಯದಿ (((state & CROSS_AT_LOWER) && ಮೌಲ್ಯ < ಕೆಳಗಿನ)
            || ((state & CROSS_AT_UPPER) && ಮೌಲ್ಯ > ಮೇಲಿನ)) {
            ಪ್ರತ್ಯಾಗಮ CROSS_NO_MATCH;
        }
        ಪ್ರತ್ಯಾಗಮ static_cast<CrossRangeState>(
            ((state & CROSS_AT_LOWER) && ಮೌಲ್ಯ == ಕೆಳಗಿನ ? CROSS_AT_LOWER : CROSS_INSIDE_BOUNDS)
            | ((state & CROSS_AT_UPPER) && ಮೌಲ್ಯ == ಮೇಲಿನ ? CROSS_AT_UPPER : CROSS_INSIDE_BOUNDS));
    }

    ಸ್ಥಿರ ತಾರ್ಕಿಕ crossWildcardIntersects(ಧ್ರುವ Cross_ಮೌಲ್ಯ_Range& range) {
        unsigned states = 1U << CROSS_AT_BOUNDS;
        ಪ್ರತಿ (ಪೂರ್ಣಾಙ್ಕ bit = range.pattern.width() - 1; bit >= 0 && states; --bit) {
            unsigned next = 0;
            ಪ್ರತಿ (ಧ್ರುವ CrossRangeState state :
                 {CROSS_INSIDE_BOUNDS, CROSS_AT_LOWER, CROSS_AT_UPPER, CROSS_AT_BOUNDS}) {
                ಯದಿ (!(states & (1U << state))) ಅನುವರ್ತ;
                ಪ್ರತಿ (ಪೂರ್ಣಾಙ್ಕ ಮೌಲ್ಯ = 0; ಮೌಲ್ಯ < 2; ++ಮೌಲ್ಯ) {
                    ಧ್ರುವ CrossRangeState equal = crossRangeStep(range, state, bit, ಮೌಲ್ಯ);
                    ಯದಿ (equal != CROSS_NO_MATCH) next |= 1U << equal;
                }
            }
            states = next;
        }
        ಪ್ರತ್ಯಾಗಮ states != 0;
    }

    // The least value at least 'from' with the non-x bits of 'pattern', in unsigned order; false
    // if none.  A bit scan, which does not enumerate the pattern's x bits.
    ಸ್ಥಿರ ತಾರ್ಕಿಕ next_Pattern_ಮೌಲ್ಯ(ಧ್ರುವ V3_ಸಂಖ್ಯೆ& pattern, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& from, V3_ಸಂಖ್ಯೆ& result) {
        result.opAssign(from);
        ಪೂರ್ಣಾಙ್ಕ carry = -1;  // Lowest x bit above the bit scanned where 'from' has a 0
        ಪ್ರತಿ (ಪೂರ್ಣಾಙ್ಕ bit = from.width() - 1; bit >= 0; --bit) {
            ಯದಿ (pattern.bitIsXZ(bit)) {
                ಯದಿ (!from.bitIs1(bit)) carry = bit;
                ಅನುವರ್ತ;
            }
            ಯದಿ (pattern.bitIs1(bit) == from.bitIs1(bit)) ಅನುವರ್ತ;
            ಯದಿ (from.bitIs1(bit)) {  // Only a greater prefix, an x bit above raised, can match
                ಯದಿ (carry < 0) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
                bit = carry;
            }
            // Then the least such value: the pattern's bits, with x bits of zero
            result.setBit(bit, 1);
            ಯಾವತ್ (--bit >= 0) result.setBit(bit, pattern.bitIs1(bit));
            ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;
        }
        ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;
    }

    // Append to 'runs' the maximal runs of consecutive values of 'range' that its pattern
    // matches, in value order, until there are more than 'limit'.  A match continues through
    // the pattern's trailing x bits only, which the next value's carry leaves.
    ಸ್ಥಿರ ಶೂನ್ಯ crossRangeRuns(ಧ್ರುವ Cross_ಮೌಲ್ಯ_Range& range, size_t limit,
                               std::vector<std::pair<V3_ಸಂಖ್ಯೆ, V3_ಸಂಖ್ಯೆ>>& runs) {
        // Values order signed, which is the unsigned order of values with the sign bit flipped
        ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ sign = range.lo.width() - 1;
        ಧ್ರುವ auto flipped = [sign](V3_ಸಂಖ್ಯೆ ಮೌಲ್ಯ) {
            ಯದಿ (!ಮೌಲ್ಯ.bitIsXZ(sign)) ಮೌಲ್ಯ.setBit(sign, !ಮೌಲ್ಯ.bitIs1(sign));
            ಪ್ರತ್ಯಾಗಮ ಮೌಲ್ಯ;
        };
        ಧ್ರುವ V3_ಸಂಖ್ಯೆ pattern = flipped(range.pattern);
        ಧ್ರುವ V3_ಸಂಖ್ಯೆ hi = flipped(range.hi);
        ಧ್ರುವ V3_ಸಂಖ್ಯೆ one{&hi, hi.width(), 1};
        V3_ಸಂಖ್ಯೆ trailing{&hi, hi.width()};
        ಪ್ರತಿ (ಪೂರ್ಣಾಙ್ಕ bit = 0; bit <= sign && pattern.bitIsXZ(bit); ++bit) trailing.setBit(bit, 1);
        V3_ಸಂಖ್ಯೆ from = flipped(range.lo);
        V3_ಸಂಖ್ಯೆ first{&hi, hi.width()};
        V3_ಸಂಖ್ಯೆ last{&hi, hi.width()};
        V3_ಸಂಖ್ಯೆ less{&hi};
        ಯಾವತ್ (runs.size() <= limit && next_Pattern_ಮೌಲ್ಯ(pattern, from, first)
               && less.opLt(hi, first).isEqZero()) {
            last.opOr(first, trailing);
            ಯದಿ (!less.opLt(hi, last).isEqZero()) last = hi;
            runs.emplace_back(flipped(first), flipped(last));
            ಯದಿ (last.isCaseEq(hi)) ಭಙ್ಗ;
            from.opAdd(last, one);
        }
    }

    // True if no coverpoint value participates in a resolved value or range.  A value with x
    // or z bits participates only as a wildcard pattern (IEEE 1800-2023 19.5.7).
    ಸ್ಥಿರ ತಾರ್ಕಿಕ crossRangeEmpty(ಧ್ರುವ Cross_ಮೌಲ್ಯ_Range& range) {
        ಯದಿ (range.lo.isFourState()) ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;
        ಪ್ರತ್ಯಾಗಮ cross_ಮೌಲ್ಯ_Less(range.hi, range.lo)
               || (range.wildcard && !crossWildcardIntersects(range));
    }

    // Comparison width that holds both the coverpoint's and a bin or intersect value's range.
    ಸ್ಥಿರ ಪೂರ್ಣಾಙ್ಕ resolveWidth(Ast_ಗಂಟು* nodep, ಧ್ರುವ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* exprp) {
        ಪ್ರತ್ಯಾಗಮ std::max(exprp->width(), crossRangeWidth(nodep)) + 1;
    }

    // Resolve a bin or intersect value to coverpoint values (IEEE 1800-2023 19.5.7), in the
    // width 'range' was created with.  False if the value is not a constant integral or real.
    ಸ್ಥಿರ ತಾರ್ಕಿಕ resolve_ಮೌಲ್ಯ(Ast_ಗಂಟು* nodep, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* exprp, ತಾರ್ಕಿಕ bin_ಮೌಲ್ಯ, ತಾರ್ಕಿಕ wildcard,
                             Cross_ಮೌಲ್ಯ_Range& range) {
        ಧ್ರುವ Cross_ಮೌಲ್ಯ_Range domain
            = cross_ಮೌಲ್ಯ_Domain(nodep, exprp->width(), exprp->isSigned(), range.lo.width());
        ಪ್ರತ್ಯಾಗಮ cross_ಮೌಲ್ಯ_Range(nodep, exprp, bin_ಮೌಲ್ಯ, wildcard, domain, range);
    }

    ಸ್ಥಿರ ತಾರ್ಕಿಕ crossRangesIntersect(ಧ್ರುವ Cross_ಮೌಲ್ಯ_Range& bin, ಧ್ರುವ Cross_ಮೌಲ್ಯ_Range& filter) {
        // Values with x or z bits do not participate, even in an identical filter.
        ಯದಿ (bin.lo.isFourState() || filter.lo.isFourState()) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
        Cross_ಮೌಲ್ಯ_Range match = bin;
        intersectCrossRange(match, filter);
        ಪ್ರತ್ಯಾಗಮ !cross_ಮೌಲ್ಯ_Less(match.hi, match.lo)
               && (!bin.wildcard || crossWildcardIntersects(match));
    }

    ಸ್ಥಿರ ಶೂನ್ಯ unsupportedCrossRange(AstCoverBinsof* selectp, ತಾರ್ಕಿಕ& valid) {
        selectp->v3warn(COVERIGN, "Unsupported: non-constant or non-integral 'intersect' value, "
                                  "or four-state range bound.");
        valid = ಅಸತ್ಯ;
    }

    ಸ್ಥಿರ ತಾರ್ಕಿಕ cross_ಮೌಲ್ಯ_Matches_Filters(AstCoverBinsof* selectp, Ast_ಗಂಟು* valuep,
                                         Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* exprp, ಧ್ರುವ AstCoverBin* binp,
                                         ಧ್ರುವ Cross_ಮೌಲ್ಯ_Range& domain,
                                         ಧ್ರುವ std::vector<Cross_ಮೌಲ್ಯ_Range>& filters,
                                         ತಾರ್ಕಿಕ& valid) {
        Cross_ಮೌಲ್ಯ_Range range{valuep, domain.lo.width()};
        ಯದಿ (!cross_ಮೌಲ್ಯ_Range(valuep, exprp, ಸತ್ಯ, binp->isWildcard(), domain, range)) {
            unsupportedCrossRange(selectp, valid);
            ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
        }
        ಪ್ರತಿ (ಧ್ರುವ Cross_ಮೌಲ್ಯ_Range& filter : filters) {
            ಯದಿ (crossRangesIntersect(range, filter)) ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;
        }
        ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
    }

    std::vector<ತಾರ್ಕಿಕ> selectCoverpointBins(AstCoverBinsof* selectp, ಧ್ರುವ CoverpointBins& bins,
                                           uint32_t first, uint32_t ಎಣಿಕೆ, ತಾರ್ಕಿಕ& valid) {
        ಯದಿ (selectp->rangesp() && !bins.exprp->dtypep()->skipRefp()->isIntegralOrPacked()) {
            unsupportedCrossRange(selectp, valid);
            ಪ್ರತ್ಯಾಗಮ {};
        }
        std::vector<ತಾರ್ಕಿಕ> selected(bins.total, ಅಸತ್ಯ);
        std::vector<std::vector<Ast_ಗಂಟು*>> ಮೌಲ್ಯಗಳು;
        ಪೂರ್ಣಾಙ್ಕ width = bins.exprp->width();
        ಪ್ರತಿ (Ast_ಗಂಟು* rangep = selectp->rangesp(); rangep; rangep = rangep->nextp()) {
            width = std::max(width, crossRangeWidth(rangep));
        }
        ಯದಿ (selectp->rangesp()) {
            ಮೌಲ್ಯಗಳು.reserve(ಎಣಿಕೆ);
            ಪ್ರತಿ (uint32_t i = first; i < first + ಎಣಿಕೆ; ++i) {
                // A run's values are in the coverpoint type, whose width 'width' covers
                ಮೌಲ್ಯಗಳು.ತಳ್ಳು_back(bins.ಮೌಲ್ಯಗಳು[i].runp ? std::vector<Ast_ಗಂಟು*>{}
                                                     : cross_Bin_ಮೌಲ್ಯಗಳು(bins.ಮೌಲ್ಯಗಳು[i]));
                ಪ್ರತಿ (Ast_ಗಂಟು* ಧ್ರುವ valuep : ಮೌಲ್ಯಗಳು.back()) {
                    width = std::max(width, crossRangeWidth(valuep));
                }
            }
        }
        // One extra bit preserves both unsigned maxima and negative signed bounds.
        ++width;
        ಧ್ರುವ Cross_ಮೌಲ್ಯ_Range domain
            = cross_ಮೌಲ್ಯ_Domain(selectp, bins.exprp->width(), bins.exprp->isSigned(), width);
        std::vector<Cross_ಮೌಲ್ಯ_Range> filters;
        ಪ್ರತಿ (Ast_ಗಂಟು* rangep = selectp->rangesp(); rangep; rangep = rangep->nextp()) {
            Cross_ಮೌಲ್ಯ_Range filter{rangep, width};
            ಯದಿ (!cross_ಮೌಲ್ಯ_Range(rangep, bins.exprp, ಅಸತ್ಯ, ಅಸತ್ಯ, domain, filter)) {
                unsupportedCrossRange(selectp, valid);
                ಪ್ರತ್ಯಾಗಮ {};
            }
            filters.ತಳ್ಳು_back(std::move(filter));
        }
        ಪ್ರತಿ (uint32_t i = first; i < first + ಎಣಿಕೆ; ++i) {
            ಯದಿ (!selectp->rangesp()) {
                selected[i] = ಸತ್ಯ;
                ಅನುವರ್ತ;
            }
            ಯದಿ (ಧ್ರುವ BinRun* ಧ್ರುವ runp = bins.ಮೌಲ್ಯಗಳು[i].runp) {
                ಧ್ರುವ Cross_ಮೌಲ್ಯ_Range range
                    = runBinRange(selectp, *runp, bins.ಮೌಲ್ಯಗಳು[i].element, width);
                selected[i] = std::any_of(filters.begin(), filters.end(),
                                          [&](ಧ್ರುವ Cross_ಮೌಲ್ಯ_Range& filter) {
                                              ಪ್ರತ್ಯಾಗಮ crossRangesIntersect(range, filter);
                                          });
                ಅನುವರ್ತ;
            }
            ಪ್ರತಿ (Ast_ಗಂಟು* ಧ್ರುವ valuep : ಮೌಲ್ಯಗಳು[i - first]) {
                selected[i] = cross_ಮೌಲ್ಯ_Matches_Filters(
                    selectp, valuep, bins.exprp, bins.ಮೌಲ್ಯಗಳು[i].binp, domain, filters, valid);
                ಯದಿ (!valid) ಪ್ರತ್ಯಾಗಮ {};
                ಯದಿ (selected[i]) ಭಙ್ಗ;
            }
        }
        ಯದಿ (selectp->isNegated()) {
            ಪ್ರತಿ (uint32_t i = 0; i < bins.total; ++i) selected[i] = !selected[i];
        }
        ಪ್ರತ್ಯಾಗಮ selected;
    }

    // Constructor-time value metadata of one coverpoint, as C++ list entries
    ಸಂರಚನಾ ಮೌಲ್ಯ_Lists final {
        std::vector<std::string> m_ranges;  // Bin, then low and high words
        std::vector<std::string> m_runs;  // First bin, count, then low, span, and high words
        std::vector<std::string> m_patterns;  // Bin, then value, mask, low, and high words
        std::vector<std::string> m_transitions;  // Transition bin
    };

    // Append a value's words, in the coverpoint's width, to a C++ list entry.
    ಸ್ಥಿರ ಶೂನ್ಯ append_ಪದಗಳು(std::string& text, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& ಮೌಲ್ಯ, ಧ್ರುವ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* exprp) {
        V3_ಸಂಖ್ಯೆ narrowed{&ಮೌಲ್ಯ, exprp->width()};
        narrowed.opAssign(ಮೌಲ್ಯ);
        ಪ್ರತಿ (ಪೂರ್ಣಾಙ್ಕ word = 0; word < exprp->width_ಪದಗಳು(); ++word) {
            text += ", " + cvtToStr(narrowed.edataWord(word)) + "U";
        }
    }

    ಶೂನ್ಯ collect_ಮೌಲ್ಯ_Metadata(ಮೌಲ್ಯ_Lists& lists, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* exprp, AstCoverBin* binp,
                              uint32_t index, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* valuep) {
        ಧ್ರುವ std::string bin = cvtToStr(index) + "U";
        ಯದಿ (binp->transp()) lists.m_transitions.ತಳ್ಳು_back(bin);
        ಪ್ರತಿ (Ast_ಗಂಟು* ಧ್ರುವ sourcep : cross_Bin_ಮೌಲ್ಯಗಳು({binp, valuep})) {
            Cross_ಮೌಲ್ಯ_Range range{sourcep, resolveWidth(sourcep, exprp)};
            ಯದಿ (!resolve_ಮೌಲ್ಯ(sourcep, exprp, ಸತ್ಯ, binp->isWildcard(), range)) {
                // Sampling already resolved every state bin value, so only transitions remain.
                sourcep->v3warn(E_UNSUPPORTED, "Unsupported: non-integral value in a transition "
                                               "bin of a coverpoint with exclusions.");
                ಅನುವರ್ತ;
            }
            ಯದಿ (crossRangeEmpty(range)) ಅನುವರ್ತ;
            std::string entry = bin;
            ಯದಿ (range.wildcard) {
                V3_ಸಂಖ್ಯೆ ಮೌಲ್ಯ{sourcep, exprp->width()};
                V3_ಸಂಖ್ಯೆ mask{sourcep, exprp->width()};
                mask.opBitsNonXZ(range.pattern);
                ಮೌಲ್ಯ.opBitsOne(range.pattern);
                append_ಪದಗಳು(entry, ಮೌಲ್ಯ, exprp);
                append_ಪದಗಳು(entry, mask, exprp);
            }
            append_ಪದಗಳು(entry, range.lo, exprp);
            append_ಪದಗಳು(entry, range.hi, exprp);
            (range.wildcard ? lists.m_patterns : lists.m_ranges).ತಳ್ಳು_back(entry);
        }
    }

    // Describe a run with one entry, from which the runtime computes the values of its bins.
    ಸ್ಥಿರ ಶೂನ್ಯ collectRunMetadata(ಮೌಲ್ಯ_Lists& lists, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* exprp, ಧ್ರುವ BinRun& run) {
        V3_ಸಂಖ್ಯೆ span{exprp, run.m_stride.width(), 0};
        span.opSub(run.m_stride, V3_ಸಂಖ್ಯೆ{exprp, run.m_stride.width(), 1});
        std::string entry = cvtToStr(run.m_declared) + "U, " + cvtToStr(run.m_ಎಣಿಕೆ) + "U";
        append_ಪದಗಳು(entry, run.m_lo, exprp);
        append_ಪದಗಳು(entry, span, exprp);
        append_ಪದಗಳು(entry, run.m_hi, exprp);
        lists.m_runs.ತಳ್ಳು_back(entry);
    }

    // Emit one batched metadata list, bounding the size of each call's temporary list.
    ಶೂನ್ಯ emit_ಮೌಲ್ಯ_List(File_ಸಾಲು* fl, AstVar* cpVarp, VCMethod method,
                       ಧ್ರುವ std::vector<std::string>& entries) {
        ಪ್ರತಿ (size_t first = 0; first < entries.size(); first += ಮೌಲ್ಯ_LIST_ENTRIES) {
            ಧ್ರುವ size_t end = std::min(entries.size(), first + ಮೌಲ್ಯ_LIST_ENTRIES);
            std::string text = "{" + entries[first];
            ಪ್ರತಿ (size_t i = first + 1; i < end; ++i) text += ", " + entries[i];
            m_constructorp->addStmtsp(
                itemCall(fl, cpVarp, method, {ctext(fl, text + "}")})->makeStmt());
        }
    }

    ಸ್ಥಿರ ತಾರ್ಕಿಕ checkCrossRef(ಧ್ರುವ AstCoverCrossRef* refp, ಧ್ರುವ AstCoverCross* crossp) {
        ಯದಿ (refp->name() == crossp->name()) ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;
        refp->v3error("Cross selection "
                      << refp->prettyNameQ() << " may only name its enclosing cross "
                      << crossp->prettyNameQ() << " (IEEE 1800-2023 19.6.1.2).");
        ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
    }

    ಸ್ಥಿರ ತಾರ್ಕಿಕ checkCrossBinName(ಧ್ರುವ AstCoverCrossBin* binp, std::set<std::string>& names) {
        ಯದಿ (names.emplace(binp->name()).second) ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;
        binp->v3error("Duplicate cross bin " << binp->prettyNameQ()
                                             << " (IEEE 1800-2023 19.6.1).");
        ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
    }

    // The span of the implicit automatic bin reported as 'name' ('auto_<i>', see
    // createImplicitAutoBins), found without naming each of its bins
    ಸ್ಥಿರ ತಾರ್ಕಿಕ implicitAutoBinSpan(ಧ್ರುವ CoverpointBins& bins, ಧ್ರುವ std::string& name,
                                    BinSpan& span) {
        ಧ್ರುವ std::string prefix = "auto_";
        ಯದಿ (!VString::startsWith(name, prefix)) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
        ಧ್ರುವ std::string digits = name.substr(prefix.size());
        ಧ್ರುವ unsigned long index = std::strtoul(digits.c_str(), nullptr, 10);
        // Only the reported spelling names the bin, not e.g. 'auto_01' or 'auto_x'
        ಯದಿ (index >= bins.implicitAuto.ಎಣಿಕೆ || digits != cvtToStr(index)) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
        ಧ್ರುವ uint32_t offset = static_cast<uint32_t>(index);
        span = BinSpan{bins.implicitAuto.first + offset, 1, bins.implicitAuto.declared + offset};
        ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;
    }

    CrossBinsofTarget
    resolveBinsofTarget(ಧ್ರುವ AstCoverBinsof* selectp, ಧ್ರುವ AstCoverCross* crossp,
                        ಧ್ರುವ std::vector<AstVar*>& cpVars,
                        ಧ್ರುವ std::map<std::string, uint32_t>& dimensions) ಧ್ರುವ {
        ಧ್ರುವ auto dim = dimensions.find(selectp->pointp()->name());
        ಯದಿ (dim == dimensions.end()) {
            selectp->v3error("binsof coverpoint "
                             << selectp->pointp()->prettyNameQ() << " is not an item of cross "
                             << crossp->prettyNameQ() << " (IEEE 1800-2023 19.6.1).");
            ಪ್ರತ್ಯಾಗಮ {};
        }
        ಧ್ರುವ CoverpointBins& bins = m_cpBins.at(cpVars[dim->second]);
        CrossBinsofTarget target{&bins, dim->second, 0, bins.total};
        ಯದಿ (!selectp->name().empty()) {
            ಧ್ರುವ auto bin = bins.spans.find(selectp->name());
            BinSpan span{0, 0, 0};
            ಯದಿ (bin != bins.spans.end()) {
                span = bin->second;
            } ಅನ್ಯಥಾ ಯದಿ (!implicitAutoBinSpan(bins, selectp->name(), span)) {
                selectp->v3error("Cannot find bin " << selectp->prettyNameQ() << " in coverpoint "
                                                    << selectp->pointp()->prettyNameQ()
                                                    << " (IEEE 1800-2023 19.6.1).");
                ಪ್ರತ್ಯಾಗಮ {};
            }
            target.m_first = span.first;
            target.m_ಎಣಿಕೆ = span.ಎಣಿಕೆ;
            target.m_declaredFirst = span.declared;
            target.m_declaredEnd = span.declared + span.ಎಣಿಕೆ;
            target.m_sized = span.sized;
        }
        ಪ್ರತ್ಯಾಗಮ target;
    }

    // The variables a 'with' filter reads, each by its first reference
    using CrossFilterReads = std::map<ಧ್ರುವ AstVar*, AstVarRef*>;

    // Whether a 'with' filter has a count of value tuples, of 'matches' other than '$'
    ಸ್ಥಿರ ತಾರ್ಕಿಕ counts_ಮೌಲ್ಯ_Tuples(ಧ್ರುವ AstCoverCrossWith* withp) {
        ಪ್ರತ್ಯಾಗಮ withp->countp() && !VN_IS(withp->countp(), Unbounded);
    }

    // Whether the runtime knows the values of the cross items a 'with' filter reads, or to count
    // value tuples, of all items, of integral coverpoints; else warns
    ಸ್ಥಿರ ತಾರ್ಕಿಕ crossFilterSupported(ಧ್ರುವ AstCoverCrossWith* withp,
                                     ಧ್ರುವ CrossFilterReads& reads) {
        ಪ್ರತಿ (ಧ್ರುವ AstVar* varp = withp->varsp(); varp; varp = VN_AS(varp->nextp(), Var)) {
            ಯದಿ (varp->dtypep()->skipRefp()->isIntegralOrPacked()) ಅನುವರ್ತ;
            ಧ್ರುವ auto it = reads.find(varp);
            ಯದಿ (it != reads.end()) {
                it->second->v3warn(COVERIGN, "Unsupported: cross bin 'with' filter reading the "
                                             "value of non-integral coverpoint "
                                                 << it->second->prettyNameQ());
                ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
            }
            ಯದಿ (counts_ಮೌಲ್ಯ_Tuples(withp)) {
                withp->countp()->v3warn(COVERIGN, "Unsupported: cross bin 'matches' count of "
                                                  "value tuples of non-integral coverpoint "
                                                      << varp->prettyNameQ());
                ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
            }
        }
        ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;
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
    ಶೂನ್ಯ generateCrossFilter(ಧ್ರುವ AstCoverCrossWith* withp, ಧ್ರುವ CrossFilterReads& reads,
                             ಧ್ರುವ AstCoverCrossBin* binp, ಧ್ರುವ AstCoverCross* crossp,
                             AstVar* cxp, ಧ್ರುವ std::map<string, uint32_t>& dimensions,
                             uint32_t ವಸ್ತುಗಳು) {
        File_ಸಾಲು* ಧ್ರುವ fl = withp->fileline();
        ಧ್ರುವ string prefix
            = m_sizedNames.get(sanitizeGeneratedName(crossp->name() + "__" + binp->name()));
        ಧ್ರುವ ತಾರ್ಕಿಕ counting = counts_ಮೌಲ್ಯ_Tuples(withp);
        // The count, 0 for all
        ಧ್ರುವ std::pair<Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ*, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ*> ಎಣಿಕೆ
            = counting ? new_ಎಣಿಕೆ_Args(fl, prefix + "_count", withp->countp())
                       : new_ಎಣಿಕೆ_Args(fl, withp->countp() ? 0 : 1);
        AstLoop* ಧ್ರುವ loopp = ನೂತನ AstLoop{fl};
        AstCMethodHard* ಧ್ರುವ nextp = itemCall(fl, cxp, VCMethod::COVERGROUP_WITH_NEXT);
        nextp->dtypeSetBit();
        loopp->addStmtsp(ನೂತನ AstLoopTest{fl, loopp, nextp});
        string dims;
        ಪ್ರತಿ (uint32_t dim = 0; counting && dim < ವಸ್ತುಗಳು; ++dim) {
            dims += (dims.empty() ? "" : ", ") + cvtToStr(dim) + "U";
        }
        ಯಾವತ್ (AstVar* ಧ್ರುವ varp = withp->varsp()) {
            varp->unlinkFrBack();
            ಯದಿ (!reads.ಎಣಿಕೆ(varp)) {
                VL_DO_DANGLING(ತಳ್ಳು_Deletep(varp), varp);
                ಅನುವರ್ತ;
            }
            ಧ್ರುವ auto dimIt = dimensions.find(varp->name());
            UASSERT_OBJ(dimIt != dimensions.end(), varp, "Cross 'with' candidate not of an item");
            ಧ್ರುವ uint32_t dim = dimIt->second;
            ಯದಿ (!counting) dims += (dims.empty() ? "" : ", ") + cvtToStr(dim) + "U";
            varp->name(prefix + "_" + sanitizeGeneratedName(varp->name()));
            m_constructorp->addStmtsp(varp);
            ಯದಿ (varp->isWide()) {
                loopp->addStmtsp(itemCall(fl, cxp, VCMethod::COVERGROUP_CROSS_WITH_ಮೌಲ್ಯ_W,
                                          {cnum(fl, dim), ನೂತನ AstVarRef{fl, varp, VAccess::WRITE}})
                                     ->makeStmt());
            } ಅನ್ಯಥಾ {
                AstCMethodHard* ಧ್ರುವ valuep
                    = itemCall(fl, cxp, VCMethod::COVERGROUP_CROSS_WITH_ಮೌಲ್ಯ, {cnum(fl, dim)});
                valuep->dtypeSetUInt64();
                Ast_ಗಂಟು_DType* ಧ್ರುವ dtypep = varp->findLogicDType(
                    varp->width(), varp->width(),
                    varp->isSigned() ? VSigning::SIGNED : VSigning::UNSIGNED);
                loopp->addStmtsp(ನೂತನ AstAssign{fl, ನೂತನ AstVarRef{fl, varp, VAccess::WRITE},
                                               resize_ಮೌಲ್ಯ(valuep, dtypep)});
            }
        }
        loopp->addStmtsp(itemCall(fl, cxp, VCMethod::COVERGROUP_CROSS_WITH_TEST,
                                  {withp->filterp()->unlinkFrBack()})
                             ->makeStmt());
        ಧ್ರುವ ತಾರ್ಕಿಕ prot = v3Global.opt.protectIds();
        m_constructorp->addStmtsp(
            itemCall(fl, cxp, VCMethod::COVERGROUP_CROSS_WITH_BEGIN,
                     {ctext(fl, "{" + dims + "}"), ಎಣಿಕೆ.first, ಎಣಿಕೆ.second,
                      ctext(fl, quoted(VIdProtect::protect_ಪದಗಳು_If(binp->name(), prot))),
                      ctext(fl, quoted(VIdProtect::protectIf(fl->filename(), prot))),
                      cnum(fl, static_cast<uint32_t>(fl->lineno()))})
                ->makeStmt());
        m_constructorp->addStmtsp(loopp);
        m_constructorp->addStmtsp(
            itemCall(fl, cxp, VCMethod::COVERGROUP_CROSS_WITH_END)->makeStmt());
    }

    ತಾರ್ಕಿಕ generateRuntimeSelection(Ast_ಗಂಟು* nodep, ಧ್ರುವ AstCoverCrossBin* binp,
                                  AstCoverCross* crossp, AstVar* cxp,
                                  ಧ್ರುವ std::vector<AstVar*>& cpVars,
                                  ಧ್ರುವ std::map<string, uint32_t>& dimensions) {
        File_ಸಾಲು* ಧ್ರುವ fl = nodep->fileline();
        ಯದಿ (ಧ್ರುವ AstCoverCrossRef* ಧ್ರುವ refp = VN_CAST(nodep, CoverCrossRef)) {
            ಯದಿ (!checkCrossRef(refp, crossp)) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
            m_constructorp->addStmtsp(
                itemCall(fl, cxp, VCMethod::COVERGROUP_SELECT_ALL)->makeStmt());
            ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;
        }
        ಯದಿ (ಧ್ರುವ AstCoverCrossSelect* ಧ್ರುವ opp = VN_CAST(nodep, CoverCrossSelect)) {
            ಯದಿ (!generateRuntimeSelection(opp->lhsp(), binp, crossp, cxp, cpVars, dimensions)
                || !generateRuntimeSelection(opp->rhsp(), binp, crossp, cxp, cpVars, dimensions)) {
                ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
            }
            m_constructorp->addStmtsp(itemCall(fl, cxp,
                                               opp->isOr() ? VCMethod::COVERGROUP_SELECT_OR
                                                           : VCMethod::COVERGROUP_SELECT_AND)
                                          ->makeStmt());
            ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;
        }
        ಯದಿ (ಧ್ರುವ AstCoverCrossWith* ಧ್ರುವ withp = VN_CAST(nodep, CoverCrossWith)) {
            CrossFilterReads reads;
            withp->filterp()->foreach([&](AstVarRef* refp) { reads.emplace(refp->varp(), refp); });
            // The count of 'matches' is a positive integer, or '$' (IEEE 1800-2023 19.6.1.2)
            ಯದಿ ((counts_ಮೌಲ್ಯ_Tuples(withp)
                 && !check_ಎಣಿಕೆ(withp->countp(), "Cross bin 'matches' count", "19.6.1.2"))
                || !crossFilterSupported(withp, reads)
                || !generateRuntimeSelection(withp->selectp(), binp, crossp, cxp, cpVars,
                                             dimensions)) {
                ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
            }
            generateCrossFilter(withp, reads, binp, crossp, cxp, dimensions,
                                static_cast<uint32_t>(cpVars.size()));
            ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;
        }
        AstCoverBinsof* ಧ್ರುವ selectp = VN_AS(nodep, CoverBinsof);
        ಧ್ರುವ CrossBinsofTarget target = resolveBinsofTarget(selectp, crossp, cpVars, dimensions);
        ಯದಿ (!target.m_binsp) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
        ಧ್ರುವ CoverpointBins& bins = *target.m_binsp;
        ಯದಿ (selectp->rangesp() && !bins.exprp->dtypep()->skipRefp()->isIntegralOrPacked()) {
            ತಾರ್ಕಿಕ valid = ಸತ್ಯ;
            unsupportedCrossRange(selectp, valid);
            ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
        }
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* firstp;
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* endp;
        ಯದಿ (target.m_sized < 0) {
            firstp = cnum(fl, target.m_declaredFirst);
            endp = cnum(fl, target.m_declaredEnd);
        } ಅನ್ಯಥಾ {  // A sized array, whose bins the coverpoint's construction placed
            AstVar* ಧ್ರುವ cpVarp = cpVars[target.m_dimension];
            ಧ್ರುವ uint32_t sized = static_cast<uint32_t>(target.m_sized);
            firstp = itemCall(fl, cpVarp, VCMethod::COVERGROUP_SIZED_FIRST, {cnum(fl, sized)});
            endp = itemCall(fl, cpVarp, VCMethod::COVERGROUP_SIZED_END, {cnum(fl, sized)});
            firstp->dtypeSetUInt32();
            endp->dtypeSetUInt32();
        }
        m_constructorp->addStmtsp(
            itemCall(fl, cxp, VCMethod::COVERGROUP_SELECT_DIM,
                     {cnum(fl, target.m_dimension), firstp, endp, cnum(fl, selectp->isNegated()),
                      cnum(fl, selectp->rangesp() != nullptr)})
                ->makeStmt());
        ಪ್ರತಿ (Ast_ಗಂಟು* rangep = selectp->rangesp(); rangep; rangep = rangep->nextp()) {
            Cross_ಮೌಲ್ಯ_Range range{rangep, resolveWidth(rangep, bins.exprp)};
            ಯದಿ (!resolve_ಮೌಲ್ಯ(rangep, bins.exprp, ಅಸತ್ಯ, ಅಸತ್ಯ, range)) {
                ತಾರ್ಕಿಕ valid = ಸತ್ಯ;
                unsupportedCrossRange(selectp, valid);
                ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
            }
            ಯದಿ (crossRangeEmpty(range)) ಅನುವರ್ತ;
            m_constructorp->addStmtsp(itemCall(fl, cxp,
                                               bins.exprp->isWide()
                                                   ? VCMethod::COVERGROUP_SELECT_RANGE_W
                                                   : VCMethod::COVERGROUP_SELECT_RANGE,
                                               {new_ಮೌಲ್ಯ_Const(fl, range.lo, bins.exprp),
                                                new_ಮೌಲ್ಯ_Const(fl, range.hi, bins.exprp)})
                                          ->makeStmt());
        }
        m_constructorp->addStmtsp(
            itemCall(fl, cxp, VCMethod::COVERGROUP_SELECT_DIM_END)->makeStmt());
        ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;
    }

    std::vector<AstCoverCrossBin*>
    generateRuntimeCrossBins(AstCoverCross* crossp, AstVar* cxp,
                             ಧ್ರುವ std::vector<AstVar*>& cpVars,
                             ಧ್ರುವ std::map<string, uint32_t>& dimensions) {
        std::vector<AstCoverCrossBin*> bins;
        std::set<string> names;
        ಪ್ರತಿ (Ast_ಗಂಟು* itemp = crossp->binsp(); itemp; itemp = itemp->nextp()) {
            AstCoverCrossBin* ಧ್ರುವ binp = VN_AS(itemp, CoverCrossBin);
            ಯದಿ (!checkCrossBinName(binp, names)) ಅನುವರ್ತ;
            File_ಸಾಲು* ಧ್ರುವ fl = binp->fileline();
            // Generate the selection apart, so that an unsupported one leaves no code, which
            // could leave its state for the next bin
            AstFunc* ಧ್ರುವ selectionp = ನೂತನ AstFunc{fl, "", nullptr, nullptr};
            ತಾರ್ಕಿಕ supported;
            {
                VL_RESTORER(m_constructorp);
                m_constructorp = selectionp;
                supported = generateRuntimeSelection(binp->selectp(), binp, crossp, cxp, cpVars,
                                                     dimensions);
            }
            // A supported selection has code
            ಯದಿ (supported) m_constructorp->addStmtsp(selectionp->stmtsp()->unlinkFrBackWithNext());
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(selectionp), selectionp);
            ಯದಿ (!supported) ಅನುವರ್ತ;
            ಧ್ರುವ ತಾರ್ಕಿಕ protect = v3Global.opt.protectIds();
            m_constructorp->addStmtsp(
                itemCall(fl, cxp, VCMethod::COVERGROUP_SELECT_BIN,
                         {ctext(fl, binp->binsType().binSetEnum()),
                          ctext(fl, quoted(VIdProtect::protect_ಪದಗಳು_If(binp->name(), protect))),
                          ctext(fl, quoted(VIdProtect::protectIf(fl->filename(), protect))),
                          cnum(fl, fl->lineno()), cnum(fl, fl->firstColumn()),
                          cnum(fl, static_cast<uint32_t>(bins.size()))})
                    ->makeStmt());
            bins.ತಳ್ಳು_back(binp);
        }
        m_constructorp->addStmtsp(
            itemCall(crossp->fileline(), cxp, VCMethod::COVERGROUP_FINALIZE_BINS)->makeStmt());
        ಪ್ರತ್ಯಾಗಮ bins;
    }

    ಸ್ಥಿರ ಶೂನ್ಯ setCrossSelectionRange(CrossSelection& selection, uint64_t first, uint64_t end) {
        ಯಾವತ್ (first < end) {
            ಧ್ರುವ unsigned bit = first % 64;
            ಧ್ರುವ unsigned bits = std::min<uint64_t>(64 - bit, end - first);
            selection[VL_BITWORD_Q(first)]
                |= (bits == 64 ? ~uint64_t{0} : (uint64_t{1} << bits) - 1) << bit;
            first += bits;
        }
    }

    CrossSelection crossSelection(Ast_ಗಂಟು* nodep, CrossSelectionContext& ctx) {
        ಯದಿ (ಧ್ರುವ AstCoverCrossRef* ಧ್ರುವ refp = VN_CAST(nodep, CoverCrossRef)) {
            ಯದಿ (!checkCrossRef(refp, ctx.crossp)) {
                ctx.valid = ಅಸತ್ಯ;
                ಪ್ರತ್ಯಾಗಮ {};
            }
            CrossSelection result(
                VL_BITWORD_Q(static_cast<uint64_t>(ctx.tuples) + VL_QUADSIZE - 1), 0);
            setCrossSelectionRange(result, 0, ctx.tuples);
            ಪ್ರತ್ಯಾಗಮ result;
        }
        ಯದಿ (AstCoverCrossSelect* ಧ್ರುವ opp = VN_CAST(nodep, CoverCrossSelect)) {
            CrossSelection lhs = crossSelection(opp->lhsp(), ctx);
            ಧ್ರುವ CrossSelection rhs = crossSelection(opp->rhsp(), ctx);
            ಯದಿ (!ctx.valid) ಪ್ರತ್ಯಾಗಮ {};
            ಪ್ರತಿ (size_t i = 0; i < lhs.size(); ++i) {
                lhs[i] = opp->isOr() ? lhs[i] | rhs[i] : lhs[i] & rhs[i];
            }
            ಪ್ರತ್ಯಾಗಮ lhs;
        }
        AstCoverBinsof* ಧ್ರುವ selectp = VN_AS(nodep, CoverBinsof);
        ಧ್ರುವ CrossBinsofTarget target
            = resolveBinsofTarget(selectp, ctx.crossp, ctx.cpVars, ctx.dimensions);
        ಯದಿ (!target.m_binsp) {
            ctx.valid = ಅಸತ್ಯ;
            ಪ್ರತ್ಯಾಗಮ {};
        }
        ಧ್ರುವ CoverpointBins& bins = *target.m_binsp;
        // Coverpoints with sized arrays are runtime points, so feed only runtime crosses
        UASSERT_OBJ(target.m_sized < 0, selectp, "Sized bin array selected by a static cross");
        ಧ್ರುವ std::vector<ತಾರ್ಕಿಕ> selected
            = selectCoverpointBins(selectp, bins, target.m_first, target.m_ಎಣಿಕೆ, ctx.valid);
        ಯದಿ (!ctx.valid) ಪ್ರತ್ಯಾಗಮ {};
        CrossSelection result(VL_BITWORD_Q(static_cast<uint64_t>(ctx.tuples) + VL_QUADSIZE - 1),
                              0);
        ಧ್ರುವ uint64_t stride = ctx.strides[target.m_dimension];
        ಧ್ರುವ uint64_t period = stride * bins.total;
        ಪ್ರತಿ (uint64_t base = 0; base < ctx.tuples; base += period) {
            ಪ್ರತಿ (uint32_t i = 0; i < bins.total;) {
                ಯದಿ (!selected[i]) {
                    ++i;
                    ಅನುವರ್ತ;
                }
                ಧ್ರುವ uint32_t begin = i++;
                ಯಾವತ್ (i < bins.total && selected[i]) ++i;
                setCrossSelectionRange(result, base + begin * stride, base + i * stride);
            }
        }
        ಪ್ರತ್ಯಾಗಮ result;
    }

    // Size the Cartesian product of the declared Normal bins, with each dimension's flat-index
    // stride.  Live runtime bins only shrink it.  Warns and returns false if it is too large.
    ತಾರ್ಕಿಕ crossShape(AstCoverCross* crossp, ಧ್ರುವ std::vector<AstVar*>& cpVars,
                    std::vector<uint32_t>& strides, uint32_t& tuples) ಧ್ರುವ {
        uint64_t product = std::any_of(cpVars.begin(), cpVars.end(),
                                       [ಏತತ್](AstVar* varp) { ಪ್ರತ್ಯಾಗಮ !m_cpBins.at(varp).total; })
                               ? 0
                               : 1;
        strides.resize(cpVars.size());
        ಪ್ರತಿ (size_t d = cpVars.size(); d > 0; --d) {
            strides[d - 1] = static_cast<uint32_t>(product);
            product *= m_cpBins.at(cpVars[d - 1]).total;
            ಯದಿ (product > UINT32_MAX) {
                crossp->v3warn(COVERIGN,
                               "Unsupported: cross coverage with more than 2^32-1 tuples.");
                ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
            }
        }
        tuples = static_cast<uint32_t>(product);
        ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;
    }

    CrossLayout resolveCrossLayout(AstCoverCross* crossp, ಧ್ರುವ std::vector<AstVar*>& cpVars,
                                   ಧ್ರುವ std::map<std::string, uint32_t>& dimensions) {
        CrossLayout layout;
        CrossSelectionContext ctx{crossp, cpVars, dimensions, 0, {}};
        ಯದಿ (!crossShape(crossp, cpVars, ctx.strides, ctx.tuples)) {
            layout.valid = ಅಸತ್ಯ;
            ಪ್ರತ್ಯಾಗಮ layout;
        }
        layout.tuples = ctx.tuples;
        CrossSelection occupied;
        CrossSelection excluded;
        std::set<std::string> names;
        ಪ್ರತಿ (Ast_ಗಂಟು* itemp = crossp->binsp(); itemp; itemp = itemp->nextp()) {
            AstCoverCrossBin* ಧ್ರುವ binp = VN_AS(itemp, CoverCrossBin);
            ಯದಿ (!checkCrossBinName(binp, names)) ಅನುವರ್ತ;
            ctx.valid = ಸತ್ಯ;
            CrossSelection selection = crossSelection(binp->selectp(), ctx);
            ಯದಿ (!ctx.valid || std::all_of(selection.begin(), selection.end(), [](uint64_t word) {
                    ಪ್ರತ್ಯಾಗಮ word == 0;
                })) {
                ಅನುವರ್ತ;
            }
            ಯದಿ (occupied.empty()) {
                occupied.resize(selection.size(), 0);
                excluded.resize(selection.size(), 0);
            }
            ಪ್ರತಿ (size_t i = 0; i < selection.size(); ++i) {
                occupied[i] |= selection[i];
                ಯದಿ (!binp->binsType().binIsNormal()) excluded[i] |= selection[i];
            }
            layout.bins.ತಳ್ಳು_back({binp, std::move(selection)});
        }
        ಯದಿ (!layout.bins.empty()) {
            // IEEE 1800-2023 19.6.2/19.6.3: exclusions also remove tuples from named
            // bins, independently of declaration order and sampling guards.
            ಪ್ರತಿ (ResolvedCrossBin& resolved : layout.bins) {
                ಪ್ರತಿ (size_t i = 0; i < resolved.selection.size(); ++i) {
                    ಯದಿ (resolved.binp->binsType().binIsNormal()) {
                        resolved.selection[i] &= ~excluded[i];
                    }
                    ಯದಿ (resolved.selection[i]) ++layout.bin_ಪದಗಳು;
                }
            }
            layout.bins.erase(std::remove_if(layout.bins.begin(), layout.bins.end(),
                                             [](ಧ್ರುವ ResolvedCrossBin& resolved) {
                                                 ಪ್ರತ್ಯಾಗಮ std::all_of(
                                                     resolved.selection.begin(),
                                                     resolved.selection.end(),
                                                     [](uint64_t word) { ಪ್ರತ್ಯಾಗಮ word == 0; });
                                             }),
                              layout.bins.end());
            layout.autoBins = layout.tuples;
            ಪ್ರತಿ (ಧ್ರುವ uint64_t word : occupied) {
                layout.autoBins -= static_cast<uint32_t>(std::bitset<VL_QUADSIZE>{word}.ಎಣಿಕೆ());
            }
        }
        ಪ್ರತ್ಯಾಗಮ layout;
    }

    AstCoverCrossDType* crossDType(File_ಸಾಲು* fl, uint32_t dimensions, ಧ್ರುವ CrossLayout& layout,
                                   ತಾರ್ಕಿಕ dynamic = ಅಸತ್ಯ) {
        ಧ್ರುವ uint32_t bins = static_cast<uint32_t>(layout.bins.size());
        ಧ್ರುವ CrossShape shape{dimensions,      layout.tuples,   bins,
                               layout.autoBins, layout.bin_ಪದಗಳು, dynamic};
        AstCoverCrossDType*& typep = m_cxDTypes[shape];
        ಯದಿ (!typep) {
            typep = ನೂತನ AstCoverCrossDType{
                fl, dimensions, layout.tuples, bins, layout.autoBins, layout.bin_ಪದಗಳು, dynamic};
            v3Global.rootp()->typeTablep()->addTypesp(typep);
        }
        ಪ್ರತ್ಯಾಗಮ typep;
    }

    std::vector<AstCoverCrossBin*> generateCrossBins(AstCoverCross* crossp, AstVar* cxVarp,
                                                     ಧ್ರುವ CrossLayout& layout) {
        std::vector<AstCoverCrossBin*> bins;
        ಪ್ರತಿ (ಧ್ರುವ ResolvedCrossBin& resolved : layout.bins) {
            AstCoverCrossBin* ಧ್ರುವ binp = resolved.binp;
            ಧ್ರುವ CrossSelection& selection = resolved.selection;
            File_ಸಾಲು* ಧ್ರುವ fl = binp->fileline();
            ಧ್ರುವ ತಾರ್ಕಿಕ prot = v3Global.opt.protectIds();
            std::string mask = "{";
            ಪ್ರತಿ (size_t i = 0; i < selection.size(); ++i) {
                ಯದಿ (i) mask += ", ";
                mask += std::to_string(selection[i]) + "ULL";
            }
            mask += "}";
            m_constructorp->addStmtsp(
                itemCall(fl, cxVarp, VCMethod::COVERGROUP_ADD_BIN,
                         {ctext(fl, binp->binsType().binSetEnum()), ctext(fl, mask),
                          ctext(fl, quoted(VIdProtect::protect_ಪದಗಳು_If(binp->name(), prot))),
                          ctext(fl, quoted(VIdProtect::protectIf(fl->filename(), prot))),
                          cnum(fl, static_cast<uint32_t>(fl->lineno())),
                          cnum(fl, static_cast<uint32_t>(fl->firstColumn()))})
                    ->makeStmt());
            bins.ತಳ್ಳು_back(binp);
        }
        ಯದಿ (!bins.empty()) {
            m_constructorp->addStmtsp(
                itemCall(crossp->fileline(), cxVarp, VCMethod::COVERGROUP_FINALIZE_BINS)
                    ->makeStmt());
        }
        ಪ್ರತ್ಯಾಗಮ bins;
    }

    // Route a cross through a VlCoverCross member: emit the member, its constructor init +
    // registration, and the sample() call.  The feeding coverpoints are already generated
    // (their hit lists drive the cross). Each explicit bin adds one configuration call.
    ಶೂನ್ಯ generateCross(AstCoverCross* crossp) {
        File_ಸಾಲು* ಧ್ರುವ fl = crossp->fileline();
        ಧ್ರುವ ತಾರ್ಕಿಕ dynamic = m_runtimeCrosses.ಎಣಿಕೆ(crossp);
        UINFO(4, "  Generating VlCoverCross member: " << crossp->name());

        ಯದಿ (Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ iffp = crossp->iffp()) {
            crossp->iffp(
                captureIffToTemp(iffp, "__VcrossIff_" + sanitizeGeneratedName(crossp->name())));
        }

        // Resolve and unlink the coverpoint refs, in dimension order.  Every ref resolves to a
        // known coverpoint (a cross with an unresolvable item was dropped earlier).
        std::vector<AstVar*> cpVars;
        std::map<std::string, uint32_t> dimensions;
        ಪ್ರತಿ (Ast_ಗಂಟು* itemp = crossp->itemsp(); itemp;) {
            Ast_ಗಂಟು* ಧ್ರುವ nextp = itemp->nextp();
            AstCoverpointRef* ಧ್ರುವ refp = VN_AS(itemp, CoverpointRef);
            ಧ್ರುವ auto it = m_cpVarMap.find(refp->name());
            UASSERT_OBJ(it != m_cpVarMap.end(), crossp, "Cross references an unknown coverpoint");
            dimensions.emplace(refp->name(), static_cast<uint32_t>(cpVars.size()));
            cpVars.ತಳ್ಳು_back(it->second);
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(refp->unlinkFrBack()), refp);
            itemp = nextp;
        }
        ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ dims = static_cast<ಪೂರ್ಣಾಙ್ಕ>(cpVars.size());
        CrossLayout layout;
        ಯದಿ (dynamic) {
            // The runtime sizes the layout from live bins; only check the declared bound here.
            std::vector<uint32_t> strides;
            uint32_t tuples = 0;
            layout.valid = crossShape(crossp, cpVars, strides, tuples);
        } ಅನ್ಯಥಾ {
            layout = resolveCrossLayout(crossp, cpVars, dimensions);
        }
        ಯದಿ (!layout.valid) ಪ್ರತ್ಯಾಗಮ;

        AstVar* ಧ್ರುವ cxVarp
            = ನೂತನ AstVar{fl, VVarType::MEMBER, "__Vcx_" + crossp->name(),
                         crossDType(fl, static_cast<uint32_t>(dims), layout, dynamic)};
        m_covergroupp->addMembersp(cxVarp);
        m_constructorp->addStmtsp(makeItemCreate(fl, cxVarp,
                                                 dynamic ? VCMethod::COVERGROUP_ADD_CROSS_DYN
                                                         : VCMethod::COVERGROUP_ADD_CROSS));
        generateItemWeight(fl, cxVarp, crossp->optionsp());
        ಪೂರ್ಣಾಙ್ಕ at_Least_ಮೌಲ್ಯ;
        ಪೂರ್ಣಾಙ್ಕ autoBinMax;  // Not a cross option
        extractItemOptions(crossp->optionsp(), at_Least_ಮೌಲ್ಯ, autoBinMax);

        // Constructor: init (after the coverpoints, which generate earlier) then registration.
        // Obfuscate the hierarchy/filename/page under --protect-ids as for coverpoints above.
        ಧ್ರುವ ತಾರ್ಕಿಕ prot = v3Global.opt.protectIds();
        ಧ್ರುವ std::string hier
            = VIdProtect::protect_ಪದಗಳು_If(m_covergroupName + "." + crossp->name(), prot);
        m_constructorp->addStmtsp(makeCrossCpsCall(
            fl, cpVars,
            itemCall(fl, cxVarp, VCMethod::COVERGROUP_INIT,
                     {ctext(fl, quoted(hier)), cnum(fl, static_cast<uint32_t>(at_Least_ಮೌಲ್ಯ)),
                      cnum(fl, static_cast<uint32_t>(dims)), ctext(fl, "__Vcx_cps"),
                      ctext(fl, quoted(VIdProtect::protectIf(fl->filename(), prot))),
                      cnum(fl, static_cast<uint32_t>(fl->lineno())),
                      cnum(fl, static_cast<uint32_t>(fl->firstColumn()))})));
        ಧ್ರುವ std::vector<AstCoverCrossBin*> bins
            = dynamic ? generateRuntimeCrossBins(crossp, cxVarp, cpVars, dimensions)
                      : generateCrossBins(crossp, cxVarp, layout);
        ಯದಿ (v3Global.opt.coverage()) {
            ಧ್ರುವ std::string page
                = VIdProtect::protectIf("v_covergroup/" + m_covergroupName, prot);
            m_constructorp->addStmtsp(itemCall(fl, cxVarp, VCMethod::COVERGROUP_REGISTER_BINS,
                                               {ctext(fl, "vlSymsp->_vm_contextp__->coveragep()"),
                                                ctext(fl, quoted(page)),
                                                cnum(fl, itemDatabaseWeight(crossp->optionsp())),
                                                cnum(fl, m_cgTypeWeight)})
                                          ->makeStmt());
        }

        // sample(): after all coverpoints have sampled (cross loop runs after coverpoint loop).
        UASSERT_OBJ(m_sampleFuncp, crossp, "sample() CFunc not set for cross");
        // The cross remembers its feeding coverpoints, so sample() needs no cps array;
        // per-bin iff guards still need a temporary array, hence the block form.
        ಧ್ರುವ ತಾರ್ಕಿಕ hasIffs
            = std::any_of(bins.begin(), bins.end(),
                          [](ಧ್ರುವ AstCoverCrossBin* binp) { ಪ್ರತ್ಯಾಗಮ binp->iffp() != nullptr; });
        Ast_ಗಂಟು_Stmt* ಧ್ರುವ samplep
            = !hasIffs ? static_cast<Ast_ಗಂಟು_Stmt*>(
                             itemCall(fl, cxVarp, VCMethod::COVERGROUP_SAMPLE)->makeStmt())
                       : static_cast<Ast_ಗಂಟು_Stmt*>(makeCrossIffsCall(
                             fl, bins,
                             itemCall(fl, cxVarp, VCMethod::COVERGROUP_SAMPLE_IFFS,
                                      {ctext(fl, "__Vcx_iffs")})));
        ಯದಿ (Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ iffp = crossp->iffp()) {
            m_sampleFuncp->addStmtsp(ನೂತನ AstIf{fl, iffp->clone_ವೃಕ್ಷ(ಅಸತ್ಯ), samplep});
        } ಅನ್ಯಥಾ {
            m_sampleFuncp->addStmtsp(samplep);
        }
    }

    ಶೂನ್ಯ generateCrossCode(AstCoverCross* crossp) {
        UINFO(4, "  Generating code for cross: " << crossp->name());

        // Non-standard hierarchical/dotted cross item (e.g. 'cross a.b'): an implicit coverpoint
        // over the referenced expression (carried in refp->exprp()).  The grammar already warned
        // NONSTD; implicit coverpoints are not yet implemented, so generate no sampling code for
        // this cross.  When support is added the implicit coverpoint should be synthesized
        // upstream (V3LinkParse) as a real AstCoverpoint so it flows through the normal coverpoint
        // path - by here coverpoint lowering has already run.
        ಪ್ರತಿ (Ast_ಗಂಟು* itemp = crossp->itemsp(); itemp; itemp = itemp->nextp()) {
            ಧ್ರುವ AstCoverpointRef* ಧ್ರುವ refp = VN_AS(itemp, CoverpointRef);
            ಯದಿ (refp->exprp()) {
                refp->v3warn(COVERIGN,
                             "Unsupported: cross of hierarchical reference (implicit coverpoint)");
                ಪ್ರತ್ಯಾಗಮ;
            }
        }

        // A cross naming a bare variable (implicit coverpoint, which Verilator does not
        // synthesize) is dropped entirely with a COVERIGN warning -- it produces no coverage
        // either way -- but only this cross is dropped; its sibling crosses are still generated
        // and the real coverpoints it referenced remain as independent coverpoints.
        ಯದಿ (m_droppedCrosses.ಎಣಿಕೆ(crossp)) {
            ಪ್ರತಿ (Ast_ಗಂಟು* itemp = crossp->itemsp(); itemp; itemp = itemp->nextp()) {
                ಧ್ರುವ AstCoverpointRef* ಧ್ರುವ refp = VN_AS(itemp, CoverpointRef);
                ಯದಿ (m_coverpointMap.find(refp->name()) == m_coverpointMap.end()) {
                    refp->v3warn(COVERIGN, "Unsupported: cross of "
                                               << refp->prettyNameQ()
                                               << " which is not a coverpoint (implicit "
                                                  "coverpoint)");
                    ಭಙ್ಗ;
                }
            }
            ಪ್ರತ್ಯಾಗಮ;
        }

        // Every cross that isn't dropped routes through a VlCoverCross member.
        generateCross(crossp);
    }

    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* buildBinCondition(AstCoverBin* binp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* exprp) {
        // Get the range list from the bin
        Ast_ಗಂಟು* ಧ್ರುವ rangep = binp->rangesp();
        ಯದಿ (!rangep) ಪ್ರತ್ಯಾಗಮ nullptr;

        // Integral values resolve to the coverpoint's type, as its runtime metadata does
        ಧ್ರುವ ತಾರ್ಕಿಕ integral = exprp->dtypep()->skipRefp()->isIntegralOrPacked();
        // No value form of a wildcard bin is allowed on a real coverpoint (IEEE 1800-2023 19.5.4)
        ಯದಿ (binp->isWildcard() && !integral) ಪ್ರತ್ಯಾಗಮ wildcardTypeError(binp, exprp);

        // Build condition by OR-ing all ranges together
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* fullCondp = nullptr;

        ಪ್ರತಿ (Ast_ಗಂಟು* currRangep = rangep; currRangep; currRangep = currRangep->nextp()) {
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* rangeCondp = nullptr;
            currRangep = V3Const::constifyEdit(currRangep);

            ಯದಿ (AstInsideRange* irp = VN_CAST(currRangep, InsideRange)) {
                Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ minExprp = irp->lhsp();
                Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ maxExprp = irp->rhsp();
                AstConst* ಧ್ರುವ minConstp = VN_CAST(minExprp, Const);
                AstConst* ಧ್ರುವ maxConstp = VN_CAST(maxExprp, Const);
                ಧ್ರುವ ತಾರ್ಕಿಕ loUnbounded = VN_IS(minExprp, Unbounded);
                ಧ್ರುವ ತಾರ್ಕಿಕ hiUnbounded = VN_IS(maxExprp, Unbounded);
                ಯದಿ (loUnbounded || hiUnbounded) {
                    // Open-ended range: '$' is the coverpoint domain min/max, so the
                    // range reduces to a single inequality (e.g. {[10:$]} -> expr >= 10).
                    AstConst* ಧ್ರುವ boundp = hiUnbounded ? minConstp : maxConstp;
                    ಯದಿ (loUnbounded && hiUnbounded) {
                        rangeCondp = ನೂತನ AstConst{irp->fileline(), AstConst::BitTrue{}};
                    } ಅನ್ಯಥಾ ಯದಿ (!boundp) {
                        irp->v3error("Non-constant expression in bin range; "
                                     "range bounds must be constants (IEEE 1800-2023 19.5)");
                        ಯದಿ (fullCondp) VL_DO_DANGLING(ತಳ್ಳು_Deletep(fullCondp), fullCondp);
                        ಪ್ರತ್ಯಾಗಮ nullptr;
                    } ಅನ್ಯಥಾ ಯದಿ (boundp->ಅಂಕೆ().isFourState()) {
                        irp->v3error("Four-state (x/z) value in bin range bound; "
                                     "range bounds must be two-state constants");
                        ಯದಿ (fullCondp) VL_DO_DANGLING(ತಳ್ಳು_Deletep(fullCondp), fullCondp);
                        ಪ್ರತ್ಯಾಗಮ nullptr;
                    } ಅನ್ಯಥಾ ಯದಿ (integral) {
                        rangeCondp = build_ಮೌಲ್ಯ_Condition(binp, exprp, irp);
                    } ಅನ್ಯಥಾ {
                        rangeCondp = makeOpenRangeCondition(irp->fileline(), exprp, boundp,
                                                            /*isLowerBound=*/hiUnbounded);
                    }
                } ಅನ್ಯಥಾ ಯದಿ (!minConstp || !maxConstp) {
                    irp->v3error("Non-constant expression in bin range; "
                                 "range bounds must be constants (IEEE 1800-2023 19.5)");
                    ಯದಿ (fullCondp) VL_DO_DANGLING(ತಳ್ಳು_Deletep(fullCondp), fullCondp);
                    ಪ್ರತ್ಯಾಗಮ nullptr;
                } ಅನ್ಯಥಾ ಯದಿ (minConstp->ಅಂಕೆ().isFourState() || maxConstp->ಅಂಕೆ().isFourState()) {
                    irp->v3error("Four-state (x/z) value in bin range bound; "
                                 "range bounds must be two-state constants");
                    ಯದಿ (fullCondp) VL_DO_DANGLING(ತಳ್ಳು_Deletep(fullCondp), fullCondp);
                    ಪ್ರತ್ಯಾಗಮ nullptr;
                } ಅನ್ಯಥಾ ಯದಿ (integral) {
                    rangeCondp = build_ಮೌಲ್ಯ_Condition(binp, exprp, irp);
                } ಅನ್ಯಥಾ {
                    rangeCondp = makeRangeCondition(irp->fileline(), exprp, minExprp, maxExprp);
                }
            } ಅನ್ಯಥಾ ಯದಿ (AstConst* constp = VN_CAST(currRangep, Const)) {
                rangeCondp = build_ಮೌಲ್ಯ_Condition(binp, exprp, constp);
            } ಅನ್ಯಥಾ {
                currRangep->v3error("Non-constant expression in bin range; values must be "
                                    "constants (IEEE 1800-2023 19.5)");
                ಯದಿ (fullCondp) VL_DO_DANGLING(ತಳ್ಳು_Deletep(fullCondp), fullCondp);
                ಪ್ರತ್ಯಾಗಮ nullptr;
            }

            UASSERT_OBJ(rangeCondp, binp, "rangeCondp is null after building range condition");
            fullCondp
                = fullCondp ? ನೂತನ AstOr{binp->fileline(), fullCondp, rangeCondp} : rangeCondp;
        }

        ಪ್ರತ್ಯಾಗಮ fullCondp;
    }

    // Wildcard bits have no meaning for a non-integral coverpoint.
    ಸ್ಥಿರ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* wildcardTypeError(AstCoverBin* binp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* exprp) {
        ಧ್ರುವ Ast_ಗಂಟು_DType* ಧ್ರುವ dtypep = exprp->dtypep()->skipRefp();
        exprp->v3error("Cannot use a wildcard bin on a coverpoint of type "
                       << dtypep->prettyDTypeNameQ() << " (IEEE 1800-2023 19.5.4).\n"
                       << exprp->warnContextPrimary() << '\n'
                       << binp->warnOther() << "... Location of wildcard bin\n"
                       << binp->warnContextSecondary());
        ಪ್ರತ್ಯಾಗಮ ನೂತನ AstConst{binp->fileline(), AstConst::BitFalse{}};
    }

    // Match one bin value, range, or wildcard pattern.  Integral coverpoints first resolve the
    // value to their type (IEEE 1800-2023 19.5.7).  Non-owning: clones what it uses.
    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* build_ಮೌಲ್ಯ_Condition(AstCoverBin* binp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* exprp, Ast_ಗಂಟು* valuep) {
        File_ಸಾಲು* ಧ್ರುವ fl = valuep->fileline();
        ಯದಿ (!exprp->dtypep()->skipRefp()->isIntegralOrPacked()) {
            ಪ್ರತ್ಯಾಗಮ ನೂತನ AstEq{fl, exprp->clone_ವೃಕ್ಷ(ಅಸತ್ಯ),
                             VN_AS(valuep, ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ)->clone_ವೃಕ್ಷ(ಅಸತ್ಯ)};
        }
        Cross_ಮೌಲ್ಯ_Range range{valuep, resolveWidth(valuep, exprp)};
        ಯದಿ (!resolve_ಮೌಲ್ಯ(valuep, exprp, ಸತ್ಯ, binp->isWildcard(), range)) {
            valuep->v3warn(E_UNSUPPORTED,
                           "Unsupported: non-integral value in a coverage bin of an "
                           "integral coverpoint.");
            ಪ್ರತ್ಯಾಗಮ ನೂತನ AstConst{fl, AstConst::BitFalse{}};
        }
        ಯದಿ (crossRangeEmpty(range)) ಪ್ರತ್ಯಾಗಮ ನೂತನ AstConst{fl, AstConst::BitFalse{}};
        AstConst* ಧ್ರುವ lop = new_ಮೌಲ್ಯ_Const(fl, range.lo, exprp);
        AstConst* ಧ್ರುವ hip = new_ಮೌಲ್ಯ_Const(fl, range.hi, exprp);
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* condp = nullptr;
        ಯದಿ (lop->ಅಂಕೆ().isCaseEq(hip->ಅಂಕೆ())) {
            condp = ನೂತನ AstEq{fl, exprp->clone_ವೃಕ್ಷ(ಅಸತ್ಯ), lop};
        } ಅನ್ಯಥಾ {
            condp = makeRangeCondition(fl, exprp, lop, hip);
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(lop), lop);
        }
        VL_DO_DANGLING(ತಳ್ಳು_Deletep(hip), hip);
        ಯದಿ (!range.wildcard) ಪ್ರತ್ಯಾಗಮ condp;
        // Match the pattern's value bits within the source-value bounds.
        V3_ಸಂಖ್ಯೆ mask{valuep, exprp->width()};
        V3_ಸಂಖ್ಯೆ ಮೌಲ್ಯ{valuep, exprp->width()};
        mask.opBitsNonXZ(range.pattern);
        ಮೌಲ್ಯ.opBitsOne(range.pattern);
        AstConst* ಧ್ರುವ maskConstp = new_ಮೌಲ್ಯ_Const(fl, mask, exprp);
        AstConst* ಧ್ರುವ ಮೌಲ್ಯ_Constp = new_ಮೌಲ್ಯ_Const(fl, ಮೌಲ್ಯ, exprp);
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ ಅಭಿವ್ಯಕ್ತಿ_Masked = ನೂತನ AstAnd{fl, exprp->clone_ವೃಕ್ಷ(ಅಸತ್ಯ), maskConstp};
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ ಮೌಲ್ಯ_Masked = ನೂತನ AstAnd{fl, ಮೌಲ್ಯ_Constp, maskConstp->clone_ವೃಕ್ಷ(ಅಸತ್ಯ)};
        ಪ್ರತ್ಯಾಗಮ ನೂತನ AstLogAnd{fl, condp, ನೂತನ AstEq{fl, ಅಭಿವ್ಯಕ್ತಿ_Masked, ಮೌಲ್ಯ_Masked}};
    }

    ಶೂನ್ಯ generateCoverageComputationCode() {
        UINFO(4, "  Generating coverage computation code");

        // Invalidate cache: addMembersp() calls in generateCoverpointCode/generateCrossCode
        // have added new members since the last scan, so clear before re-querying.
        m_memberMap.clear();

        // get_inst_coverage(): the average of the coverpoints and crosses, weighted by their
        // option.weight (IEEE 1800-2023 19.11).  The instance node holds their runtimes.  With
        // the instances merged, get_coverage() instead, unless option.get_inst_coverage
        // (Table 19-1).
        AstFunc* ಧ್ರುವ getInstCoveragep
            = VN_AS(m_memberMap.findMember(m_covergroupp, "get_inst_coverage"), Func);
        File_ಸಾಲು* ಧ್ರುವ instFl = getInstCoveragep->fileline();
        AstCMethodHard* ಧ್ರುವ instCallp = instanceCall(instFl, VCMethod::COVERGROUP_COVERAGE);
        instCallp->dtypeSetDouble();
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* instValuep = instCallp;
        ಯದಿ (m_cgMayMerge) {
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ mergedp = ನೂತನ AstLogAnd{
                instFl, newOptionSel(instFl, optionVar(ಸತ್ಯ), "merge_instances", VAccess::READ),
                ನೂತನ AstLogNot{instFl, newOptionSel(instFl, optionVar(ಅಸತ್ಯ), "get_inst_coverage",
                                                   VAccess::READ)}};
            instValuep = ನೂತನ AstCond{instFl, mergedp, typeCoverageCall(instFl), instCallp};
        }
        getInstCoveragep->addStmtsp(ನೂತನ AstAssign{
            instFl, ನೂತನ AstVarRef{instFl, VN_AS(getInstCoveragep->fvarp(), Var), VAccess::WRITE},
            instValuep});

        // get_coverage(): the average of the covergroup's instances, weighted by their
        // option.weight, or with type_option.merge_instances, the coverage of the union of their
        // bins (IEEE 1800-2023 19.11.3).  Static, so the registry finds the instances.
        AstFunc* ಧ್ರುವ getCoveragep
            = VN_AS(m_memberMap.findMember(m_covergroupp, "get_coverage"), Func);
        File_ಸಾಲು* ಧ್ರುವ typeFl = getCoveragep->fileline();
        getCoveragep->addStmtsp(ನೂತನ AstAssign{
            typeFl, ನೂತನ AstVarRef{typeFl, VN_AS(getCoveragep->fvarp(), Var), VAccess::WRITE},
            typeCoverageCall(typeFl)});
    }

    // The registry call computing the covergroup's type coverage, per its type options
    AstCMethodHard* typeCoverageCall(File_ಸಾಲು* fl) {
        AstCExpr* ಧ್ರುವ registryp = ctext(fl, "vlSymsp->_vm_contextp__->covergroupRegistryp()");
        registryp->dtypeSetVoid();  // Opaque receiver; only ever the 'fromp' of the call below
        AstCMethodHard* ಧ್ರುವ callp
            = ನೂತನ AstCMethodHard{fl, registryp, VCMethod::COVERGROUP_TYPE_COVERAGE};
        callp->addPinsp(ctext(fl, quoted(covergroupProtectedName())));
        callp->addPinsp(newOptionSel(fl, optionVar(ಸತ್ಯ), "weight", VAccess::READ));
        callp->addPinsp(newOptionSel(fl, optionVar(ಸತ್ಯ), "merge_instances", VAccess::READ));
        callp->addPinsp(file_ಸಾಲು_Debug(m_covergroupp->fileline()));
        callp->usePtr(ಸತ್ಯ);
        callp->dtypeSetDouble();
        ಪ್ರತ್ಯಾಗಮ callp;
    }

    // VISITORS
    ಸ್ಥಿರ ತಾರ್ಕಿಕ isEnclosingInstanceVar(ಧ್ರುವ AstVar* varp) {
        ಪ್ರತ್ಯಾಗಮ varp->isClassMember() && !varp->lifetime().isStatic() && !varp->isParam();
    }

    ಶೂನ್ಯ rewriteThisRef(AstThisRef* refp, AstVar* handleVarp) {
        ಧ್ರುವ AstClassRefDType* ಧ್ರುವ refDTypep
            = VN_CAST(refp->dtypep()->skipRefp(), ClassRefDType);
        UASSERT_OBJ(refDTypep && refDTypep->classp() == m_covergroupp, refp,
                    "Unexpected this reference in embedded covergroup");
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ newp = ನೂತನ AstVarRef{refp->fileline(), handleVarp, VAccess::READ};
        refp->replaceWith(newp);
        VL_DO_DANGLING(ತಳ್ಳು_Deletep(refp), refp);
    }

    ಶೂನ್ಯ rewriteVarRef(AstVarRef* refp, AstVar* handleVarp) {
        File_ಸಾಲು* ಧ್ರುವ fl = refp->fileline();
        // As V3LinkLValue does, the handle of a written member select is marked written
        AstMemberSel* ಧ್ರುವ selp
            = ನೂತನ AstMemberSel{fl, ನೂತನ AstVarRef{fl, handleVarp, refp->access()}, refp->varp()};
        selp->access(refp->access());
        refp->replaceWith(selp);
        VL_DO_DANGLING(ತಳ್ಳು_Deletep(refp), refp);
    }

    ಶೂನ್ಯ rewriteFuncRef(AstFuncRef* refp, AstVar* handleVarp) {
        File_ಸಾಲು* ಧ್ರುವ fl = refp->fileline();
        AstArg* ಧ್ರುವ argsp = refp->argsp() ? refp->argsp()->unlinkFrBackWithNext() : nullptr;
        AstMethodCall* ಧ್ರುವ callp = ನೂತನ AstMethodCall{
            fl, ನೂತನ AstVarRef{fl, handleVarp, VAccess::READ}, refp->name(), argsp};
        callp->taskp(refp->taskp());
        callp->dtypeFrom(refp);
        refp->replaceWith(callp);
        VL_DO_DANGLING(ತಳ್ಳು_Deletep(refp), refp);
    }

    // True if funcp is an instance method of the enclosing class or one of its bases
    ತಾರ್ಕಿಕ isEnclosingInstanceFunc(ಧ್ರುವ Ast_ಗಂಟು_FTask* funcp) ಧ್ರುವ {
        ಯದಿ (!funcp->classMethod() || funcp->isStatic()) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
        ಪ್ರತ್ಯಾಗಮ AstClass::isClassExtendedFrom(m_enclosingClassp, VN_AS(funcp->aboveLoopp(), Class));
    }

    ತಾರ್ಕಿಕ isEmbeddedCovergroupVar(ಧ್ರುವ AstVar* varp) ಧ್ರುವ {
        ಯದಿ (!varp || !varp->isClassMember() || varp->isDeclTyped()) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
        ಧ್ರುವ AstClassRefDType* ಧ್ರುವ refp = VN_CAST(varp->dtypep()->skipRefp(), ClassRefDType);
        ಪ್ರತ್ಯಾಗಮ refp && refp->classp() == m_covergroupp;
    }

    AstVar* findEmbeddedCovergroupVar() ಧ್ರುವ {
        ಯದಿ (!m_enclosingClassp) ಪ್ರತ್ಯಾಗಮ nullptr;
        ಪ್ರತಿ (Ast_ಗಂಟು* itemp = m_enclosingClassp->membersp(); itemp; itemp = itemp->nextp()) {
            ಯದಿ (AstVar* ಧ್ರುವ varp = VN_CAST(itemp, Var)) {
                ಯದಿ (isEmbeddedCovergroupVar(varp)) ಪ್ರತ್ಯಾಗಮ varp;
            }
        }
        // V3LinkParse always creates an implicit variable for an embedded covergroup.
        ಪ್ರತ್ಯಾಗಮ nullptr;  // LCOV_EXCL_LINE
    }

    std::vector<Ast_ಗಂಟು_Assign*> findCovergroupConstructions() {
        std::vector<Ast_ಗಂಟು_Assign*> foundps;
        ಯದಿ (!m_embeddedVarp) ಪ್ರತ್ಯಾಗಮ foundps;
        AstFunc* ಧ್ರುವ enclosingNewp
            = VN_CAST(m_memberMap.findMember(m_enclosingClassp, "new"), Func);
        ಯದಿ (!enclosingNewp) ಪ್ರತ್ಯಾಗಮ foundps;
        enclosingNewp->foreach([&](Ast_ಗಂಟು_Assign* asgnp) {
            ಧ್ರುವ AstNew* ಧ್ರುವ newp = VN_CAST(asgnp->rhsp(), New);
            ಧ್ರುವ AstVarRef* ಧ್ರುವ lhsRefp = VN_CAST(asgnp->lhsp(), VarRef);
            ಯದಿ (!newp || !lhsRefp || lhsRefp->varp() != m_embeddedVarp) ಪ್ರತ್ಯಾಗಮ;
            ಧ್ರುವ AstClassRefDType* ಧ್ರುವ refp = VN_CAST(newp->dtypep(), ClassRefDType);
            ಯದಿ (refp && refp->classp() == m_covergroupp) foundps.ತಳ್ಳು_back(asgnp);
        });
        ಪ್ರತ್ಯಾಗಮ foundps;
    }

    std::set<ಧ್ರುವ AstVar*> enclosingInstanceVars() ಧ್ರುವ {
        std::set<ಧ್ರುವ AstVar*> vars;
        ಯದಿ (m_enclosingClassp) {
            m_enclosingClassp->foreachMember([&](AstClass* ಧ್ರುವ, AstVar* ಧ್ರುವ varp) {
                ಯದಿ (isEnclosingInstanceVar(varp)) vars.ಸೇರಿಸು(varp);
            });
        }
        ಪ್ರತ್ಯಾಗಮ vars;
    }

    ತಾರ್ಕಿಕ hasEnclosingEventRef(AstCovergroup* cgp) ಧ್ರುವ {
        ಯದಿ (!m_embeddedVarp || !cgp->eventp()) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
        ಧ್ರುವ std::set<ಧ್ರುವ AstVar*> enclosingVars = enclosingInstanceVars();
        ತಾರ್ಕಿಕ found = ಅಸತ್ಯ;
        cgp->eventp()->foreach([&](AstVarRef* refp) {
            ಯದಿ (enclosingVars.ಎಣಿಕೆ(refp->varp())) found = ಸತ್ಯ;
        });
        ಪ್ರತ್ಯಾಗಮ found;
    }

    ಸ್ಥಿರ ತಾರ್ಕಿಕ parse_Embedded_Event_ಅಭಿವ್ಯಕ್ತಿ(Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* exprp, AstVar*& baseVarp,
                                       AstVar*& memberVarp) {
        ಯದಿ (AstVarRef* ಧ್ರುವ refp = VN_CAST(exprp, VarRef)) {
            baseVarp = refp->varp();
            memberVarp = nullptr;
            ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;
        }
        AstMemberSel* ಧ್ರುವ selp = VN_CAST(exprp, MemberSel);
        ಯದಿ (!selp) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
        AstVarRef* ಧ್ರುವ baseRefp = VN_CAST(selp->fromp(), VarRef);
        ಯದಿ (!baseRefp) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
        baseVarp = baseRefp->varp();
        memberVarp = selp->varp();
        ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;
    }

    ತಾರ್ಕಿಕ isEventLvalue(Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* exprp, ಧ್ರುವ EmbeddedEventTrigger& trigger) ಧ್ರುವ {
        ಯದಿ (AstSel* ಧ್ರುವ selp = VN_CAST(exprp, Sel)) exprp = selp->fromp();
        AstVar* baseVarp = nullptr;
        AstVar* memberVarp = nullptr;
        ಯದಿ (!parse_Embedded_Event_ಅಭಿವ್ಯಕ್ತಿ(exprp, baseVarp, memberVarp)) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
        ಪ್ರತ್ಯಾಗಮ baseVarp == trigger.baseVarp && memberVarp == trigger.memberVarp;
    }

    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* newEventRead(File_ಸಾಲು* fl, ಧ್ರುವ EmbeddedEventTrigger& trigger) ಧ್ರುವ {
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ basep = ನೂತನ AstVarRef{fl, trigger.baseVarp, VAccess::READ};
        ಯದಿ (!trigger.memberVarp) ಪ್ರತ್ಯಾಗಮ basep;
        AstMemberSel* ಧ್ರುವ selp = ನೂತನ AstMemberSel{fl, basep, trigger.memberVarp};
        selp->access(VAccess::READ);
        ಪ್ರತ್ಯಾಗಮ selp;
    }

    string eventPrevName(ಧ್ರುವ EmbeddedEventTrigger& trigger, size_t triggerIndex) ಧ್ರುವ {
        string name = "__Vcg_prev_" + m_embeddedVarp->name() + "_" + std::to_string(triggerIndex)
                      + "_" + trigger.baseVarp->name();
        ಯದಿ (trigger.memberVarp) name += "_" + trigger.memberVarp->name();
        ಪ್ರತ್ಯಾಗಮ name;
    }

    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* newEmbeddedVarNonNull(File_ಸಾಲು* fl) ಧ್ರುವ {
        ಪ್ರತ್ಯಾಗಮ ನೂತನ AstNeq{fl, ನೂತನ AstVarRef{fl, m_embeddedVarp, VAccess::READ},
                          ನೂತನ AstConst{fl, AstConst::Null{}}};
    }

    Ast_ಗಂಟು_Stmt* newSampleStmt(File_ಸಾಲು* fl) ಧ್ರುವ {
        AstMethodCall* ಧ್ರುವ callp = ನೂತನ AstMethodCall{
            fl, ನೂತನ AstVarRef{fl, m_embeddedVarp, VAccess::READ}, "sample", nullptr};
        callp->taskp(m_sampleFuncp);
        callp->dtypeSetVoid();
        ಪ್ರತ್ಯಾಗಮ callp->makeStmt();
    }

    ಶೂನ್ಯ installEmbeddedEventFork(Ast_Sen_ವೃಕ್ಷ* eventp,
                                  ಧ್ರುವ std::vector<Ast_ಗಂಟು_Assign*>& constructps) {
        // IEEE 1800-2023 19.3 samples coverpoints whenever their clocking event occurs. A
        // per-instance event cannot use V3Active's static sensitivity path, so spawn
        // 'fork forever begin @(event); cg.sample(); end join_none' after each construction.
        ಪ್ರತಿ (Ast_ಗಂಟು_Assign* ಧ್ರುವ constructp : constructps) {
            File_ಸಾಲು* ಧ್ರುವ fl = constructp->fileline();
            AstLoop* ಧ್ರುವ loopp = ನೂತನ AstLoop{fl};
            loopp->addStmtsp(ನೂತನ AstEventControl{fl, eventp->clone_ವೃಕ್ಷ(ಅಸತ್ಯ), nullptr});
            loopp->addStmtsp(ನೂತನ AstIf{fl, newEmbeddedVarNonNull(fl), newSampleStmt(fl)});
            AstFork* ಧ್ರುವ forkp = ನೂತನ AstFork{fl, VJoinType::JOIN_NONE};
            forkp->immediateStart(ಸತ್ಯ);
            forkp->addForksp(ನೂತನ AstBegin{fl, "", loopp, ಸತ್ಯ});
            constructp->addNextHere(forkp);
        }
        VL_DO_DANGLING(ತಳ್ಳು_Deletep(eventp), eventp);
    }

    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* newEventReadyCondition(File_ಸಾಲು* fl, ಧ್ರುವ EmbeddedEventTrigger& trigger) ಧ್ರುವ {
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ curp = newEventRead(fl, trigger);
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ prevp = ನೂತನ AstVarRef{fl, trigger.prevVarp, VAccess::READ};
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* edgep = nullptr;
        // IEEE 1800-2023 9.4.2 detects edge-qualified events only on the expression's LSB,
        // while an implicit change event observes the complete expression.
        ಯದಿ (trigger.edgeType == VEdgeType::ET_POSEDGE) {
            edgep = ನೂತನ AstSel{fl, ನೂತನ AstAnd{fl, curp, ನೂತನ AstNot{fl, prevp}}, 0, 1};
        } ಅನ್ಯಥಾ ಯದಿ (trigger.edgeType == VEdgeType::ET_NEGEDGE) {
            edgep = ನೂತನ AstSel{fl, ನೂತನ AstAnd{fl, ನೂತನ AstNot{fl, curp}, prevp}, 0, 1};
        } ಅನ್ಯಥಾ ಯದಿ (trigger.edgeType == VEdgeType::ET_BOTHEDGE) {
            edgep = ನೂತನ AstSel{fl, ನೂತನ AstXor{fl, curp, prevp}, 0, 1};
        } ಅನ್ಯಥಾ {
            edgep = ನೂತನ AstNeq{fl, curp, prevp};
        }
        ಪ್ರತ್ಯಾಗಮ ನೂತನ AstLogAnd{fl, newEmbeddedVarNonNull(fl), edgep};
    }

    std::vector<EmbeddedEventTrigger> collectEmbeddedEventTriggers(AstCovergroup* cgp) {
        std::vector<EmbeddedEventTrigger> triggers;
        ಧ್ರುವ std::set<ಧ್ರುವ AstVar*> enclosingVars = enclosingInstanceVars();
        ಪ್ರತಿ (Ast_ಗಂಟು* senp = cgp->eventp()->sensesp(); senp; senp = senp->nextp()) {
            AstSenItem* ಧ್ರುವ itemp = VN_AS(senp, SenItem);
            AstVar* baseVarp = nullptr;
            AstVar* memberVarp = nullptr;
            ಯದಿ (!parse_Embedded_Event_ಅಭಿವ್ಯಕ್ತಿ(itemp->sensp(), baseVarp, memberVarp)
                || !enclosingVars.ಎಣಿಕೆ(baseVarp)) {
                ಪ್ರತ್ಯಾಗಮ {};
            }
            triggers.emplace_back(itemp->fileline(), baseVarp, memberVarp, itemp->edgeType());
        }
        ಪ್ರತ್ಯಾಗಮ triggers;
    }

    ಶೂನ್ಯ installEmbeddedEventTriggers(std::vector<EmbeddedEventTrigger>& triggers,
                                      ಧ್ರುವ std::vector<Ast_ಗಂಟು_Assign*>& constructps) {
        // Without --timing, approximate a per-instance event by sampling after assignments
        // within the enclosing class. External writes and exact scheduling cannot be observed.
        ಯದಿ (constructps.empty()) ಪ್ರತ್ಯಾಗಮ;
        ಪ್ರತಿ (size_t triggerIndex = 0; triggerIndex < triggers.size(); ++triggerIndex) {
            EmbeddedEventTrigger& trigger = triggers[triggerIndex];
            std::vector<Ast_ಗಂಟು_Assign*> assignps;
            m_enclosingClassp->foreach([&](Ast_ಗಂಟು_Assign* asgnp) {
                ಯದಿ (isEventLvalue(asgnp->lhsp(), trigger)) assignps.ತಳ್ಳು_back(asgnp);
            });
            ಯದಿ (assignps.empty()) {
                trigger.eventFl->v3warn(
                    COVERIGN, "Unsupported: 'covergroup' clocking event signal has no assignment "
                              "within the enclosing class; no coverage sampled. Use --timing for "
                              "full support.");
                ಅನುವರ್ತ;
            }
            Ast_ಗಂಟು_DType* ಧ್ರುವ dtypep
                = trigger.memberVarp ? trigger.memberVarp->dtypep() : trigger.baseVarp->dtypep();
            AstVar* ಧ್ರುವ prevVarp = ನೂತನ AstVar{trigger.eventFl, VVarType::MEMBER,
                                                eventPrevName(trigger, triggerIndex), dtypep};
            m_enclosingClassp->addMembersp(prevVarp);
            trigger.prevVarp = prevVarp;
            ಪ್ರತಿ (Ast_ಗಂಟು_Assign* ಧ್ರುವ asgnp : assignps) {
                File_ಸಾಲು* ಧ್ರುವ fl = asgnp->fileline();
                AstIf* ಧ್ರುವ ifp
                    = ನೂತನ AstIf{fl, newEventReadyCondition(fl, trigger), newSampleStmt(fl)};
                ifp->addNextHere(ನೂತನ AstAssign{fl,
                                               ನೂತನ AstVarRef{fl, trigger.prevVarp, VAccess::WRITE},
                                               newEventRead(fl, trigger)});
                asgnp->addNextHere(ifp);
            }
        }
    }

    ಶೂನ್ಯ delete_Coverage_ವಸ್ತುಗಳು() {
        ಪ್ರತಿ (AstCoverpoint* ಧ್ರುವ cpp : m_coverpoints) {
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(cpp->unlinkFrBack()), cpp);
        }
        ಪ್ರತಿ (AstCoverCross* ಧ್ರುವ crossp : m_coverCrosses) {
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(crossp->unlinkFrBack()), crossp);
        }
        // Options not lowered: the covergroup was not processed
        ಪ್ರತಿ (AstCgOptionAssign* ಧ್ರುವ optp : m_cgOptions) {
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(optp->unlinkFrBack()), optp);
        }
        m_cgOptions.clear();
    }

    ವರ್ಗ FormalRefVisitor final : ಸಾರ್ವಜನಿಕ VNVisitor {
        ಧ್ರುವ std::map<ಧ್ರುವ AstVar*, AstVar*>& m_replacements;

        ಶೂನ್ಯ visit(AstVarRef* nodep) override {
            ಧ್ರುವ auto it = m_replacements.find(nodep->varp());
            ಯದಿ (it == m_replacements.end()) ಪ್ರತ್ಯಾಗಮ;
            nodep->varp(it->second);
        }
        ಶೂನ್ಯ visit(Ast_ಗಂಟು* nodep) override { iterateChildren(nodep); }

    ಸಾರ್ವಜನಿಕ:
        explicit FormalRefVisitor(ಧ್ರುವ std::map<ಧ್ರುವ AstVar*, AstVar*>& replacements)
            : m_replacements{replacements} {}
        ಶೂನ್ಯ scan(Ast_ಗಂಟು* nodep) { iterate(nodep); }
    };

    ಶೂನ್ಯ validateCovergroupExpressions() {
        std::set<ಧ್ರುವ AstVar*> sampleMembers;
        std::set<ಧ್ರುವ AstVar*> constructorRefMembers;
        ಪ್ರತಿ (Ast_ಗಂಟು* stmtp = m_constructorp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            ಧ್ರುವ AstVar* ಧ್ರುವ varp = VN_CAST(stmtp, Var);
            ಯದಿ (!varp || !varp->isIO() || (!varp->isRef() && !varp->isConstRef())) ಅನುವರ್ತ;
            ಧ್ರುವ AstVar* ಧ್ರುವ memberp
                = VN_CAST(m_memberMap.findMember(m_covergroupp, varp->name()), Var);
            UASSERT_OBJ(memberp && memberp->isClassMember(), varp,
                        "Covergroup constructor argument missing persistent member");
            constructorRefMembers.ಸೇರಿಸು(varp);
            constructorRefMembers.ಸೇರಿಸು(memberp);
        }
        ಪ್ರತಿ (Ast_ಗಂಟು* stmtp = m_sampleFuncp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            ಧ್ರುವ AstVar* ಧ್ರುವ varp = VN_CAST(stmtp, Var);
            ಯದಿ (!varp || !varp->isIO()) ಅನುವರ್ತ;
            ಧ್ರುವ AstVar* ಧ್ರುವ memberp
                = VN_CAST(m_memberMap.findMember(m_covergroupp, varp->name()), Var);
            UASSERT_OBJ(memberp && memberp->isClassMember(), varp,
                        "Covergroup sample argument missing persistent member");
            sampleMembers.ಸೇರಿಸು(memberp);
        }
        Covergroup_ಅಭಿವ್ಯಕ್ತಿ_Valid_Visitor{sampleMembers, constructorRefMembers}.scan(m_constructorp);
    }

    ಶೂನ್ಯ rebindFormalRefs() {
        std::map<ಧ್ರುವ AstVar*, AstVar*> replacements;
        ಪ್ರತಿ (Ast_ಗಂಟು* stmtp = m_constructorp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            ಯದಿ (ಧ್ರುವ AstVar* ಧ್ರುವ varp = VN_CAST(stmtp, Var)) {
                ಯದಿ (!varp->isIO()) ಅನುವರ್ತ;
                AstVar* ಧ್ರುವ memberp
                    = VN_CAST(m_memberMap.findMember(m_covergroupp, varp->name()), Var);
                UASSERT_OBJ(memberp && memberp->isClassMember(), varp,
                            "Covergroup constructor argument missing persistent member");
                replacements.emplace(varp, memberp);
            }
        }
        size_t expectedBindings = 0;
        ಪ್ರತಿ (ಧ್ರುವ auto& pair : replacements) {
            ಯದಿ (pair.first->isRef() || pair.first->isConstRef()) ++expectedBindings;
        }
        size_t rewrittenBindings = 0;
        ಪ್ರತಿ (Ast_ಗಂಟು* stmtp = m_constructorp->stmtsp(); stmtp;) {
            Ast_ಗಂಟು* ಧ್ರುವ nextp = stmtp->nextp();
            ಯದಿ (AstAssign* ಧ್ರುವ assignp = VN_CAST(stmtp, Assign)) {
                AstVarRef* ಧ್ರುವ lhsp = VN_CAST(assignp->lhsp(), VarRef);
                AstVarRef* ಧ್ರುವ rhsp = VN_CAST(assignp->rhsp(), VarRef);
                ಯದಿ (lhsp && rhsp
                    && (lhsp->varp()->declDirection() == VDirection::REF
                        || lhsp->varp()->declDirection() == VDirection::CONSTREF)) {
                    ಧ್ರುವ auto it = replacements.find(rhsp->varp());
                    UASSERT_OBJ(it != replacements.end() && it->second == lhsp->varp(), assignp,
                                "Unexpected covergroup reference binding assignment");
                    AstCExpr* ಧ್ರುವ bindp = ನೂತನ AstCExpr{assignp->fileline(), ""};
                    bindp->add(lhsp->unlinkFrBack());
                    bindp->add(" = &");
                    bindp->add(rhsp->unlinkFrBack());
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
        ಪ್ರತಿ (AstCoverpoint* ಧ್ರುವ cpp : m_coverpoints) visitor.scan(cpp);
        ಪ್ರತಿ (AstCoverCross* ಧ್ರುವ crossp : m_coverCrosses) visitor.scan(crossp);
    }

    AstVarRef* installEnclosingBackPointer(ಧ್ರುವ std::vector<Ast_ಗಂಟು_Assign*>& constructps) {
        // Simple-case support for embedded covergroups (IEEE 1800-2023 19.4) whose
        // coverpoints reference members of the enclosing class ("Class members can be used
        // in coverpoint expressions").  The covergroup is lowered into a sibling class with
        // no implicit handle to the enclosing object, so such references would emit
        // uncompilable C++.  Add an explicit back-pointer member to the enclosing instance,
        // route member references through it, and pass it into the constructor so
        // coverage initialization can read enclosing members. Returns an invalid
        // reference if an outer class member cannot be reached; otherwise returns an empty result.
        ಯದಿ (!m_enclosingClassp) ಪ್ರತ್ಯಾಗಮ nullptr;  // Offending refs require an enclosing class

        AstVarRef* invalidp = nullptr;
        Ast_ಗಂಟು* offenderp = nullptr;
        std::set<ಧ್ರುವ AstVar*> ownVars;
        ಪ್ರತಿ (Ast_ಗಂಟು* itemp = m_covergroupp->membersp(); itemp; itemp = itemp->nextp()) {
            ಯದಿ (ಧ್ರುವ AstVar* ಧ್ರುವ varp = VN_CAST(itemp, Var)) ownVars.ಸೇರಿಸು(varp);
        }
        ಧ್ರುವ std::set<ಧ್ರುವ AstVar*> enclosingVars = enclosingInstanceVars();
        std::vector<AstVarRef*> refsToRewrite;
        std::vector<AstThisRef*> thisRefsToRewrite;
        std::vector<AstFuncRef*> funcRefsToRewrite;
        ಧ್ರುವ auto scan = [&](Ast_ಗಂಟು* rootp) {
            rootp->foreach([&](AstVarRef* refp) {
                ಯದಿ (invalidp) ಪ್ರತ್ಯಾಗಮ;
                ಧ್ರುವ AstVar* ಧ್ರುವ varp = refp->varp();
                ಯದಿ (!isEnclosingInstanceVar(varp) || ownVars.ಎಣಿಕೆ(varp)) ಪ್ರತ್ಯಾಗಮ;
                ಯದಿ (!enclosingVars.ಎಣಿಕೆ(varp)) {
                    invalidp = refp;
                    ಪ್ರತ್ಯಾಗಮ;
                }
                refsToRewrite.ತಳ್ಳು_back(refp);
                ಯದಿ (!offenderp) offenderp = refp;
            });
            ಯದಿ (invalidp) ಪ್ರತ್ಯಾಗಮ;
            rootp->foreach([&](AstThisRef* refp) {
                ಧ್ರುವ AstClassRefDType* ಧ್ರುವ refDTypep
                    = VN_CAST(refp->dtypep()->skipRefp(), ClassRefDType);
                ಯದಿ (refDTypep && refDTypep->classp() == m_covergroupp) {
                    thisRefsToRewrite.ತಳ್ಳು_back(refp);
                    ಯದಿ (!offenderp) offenderp = refp;
                }
            });
            rootp->foreach([&](AstFuncRef* refp) {
                ಯದಿ (!isEnclosingInstanceFunc(refp->taskp())) ಪ್ರತ್ಯಾಗಮ;
                funcRefsToRewrite.ತಳ್ಳು_back(refp);
                ಯದಿ (!offenderp) offenderp = refp;
            });
        };
        ಪ್ರತಿ (AstCoverpoint* ಧ್ರುವ cpp : m_coverpoints) scan(cpp);
        ಪ್ರತಿ (AstCoverCross* ಧ್ರುವ crossp : m_coverCrosses) scan(crossp);
        ಪ್ರತಿ (AstCgOptionAssign* ಧ್ರುವ optp : m_cgOptions) scan(optp);
        ಯದಿ (invalidp || !offenderp) ಪ್ರತ್ಯಾಗಮ invalidp;

        UASSERT_OBJ(m_embeddedVarp, m_covergroupp, "Embedded covergroup variable not found");
        // Commit: add the back-pointer member, rewrite the references, initialize the handle.
        File_ಸಾಲು* ಧ್ರುವ fl = m_covergroupp->fileline();
        AstClassRefDType* ಧ್ರುವ enclDTypep = ನೂತನ AstClassRefDType{fl, m_enclosingClassp, nullptr};
        enclDTypep->rawPointer(ಸತ್ಯ);
        v3Global.rootp()->typeTablep()->addTypesp(enclDTypep);
        AstVar* ಧ್ರುವ handleVarp
            = ನೂತನ AstVar{fl, VVarType::MEMBER, "__Vcg_enclosingp", enclDTypep};
        m_covergroupp->addMembersp(handleVarp);
        AstVar* ಧ್ರುವ argumentp = ನೂತನ AstVar{fl, VVarType::BLOCKTEMP, "__Vcg_parentp", enclDTypep};
        argumentp->direction(VDirection::ಒಳಹರಿವು);
        argumentp->declDirection(VDirection::ಒಳಹರಿವು);
        argumentp->funcLocal(ಸತ್ಯ);
        argumentp->noReset(ಸತ್ಯ);
        argumentp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
        m_constructorp->addStmtsp(argumentp);
        m_constructorp->prependStmtsp(ನೂತನ AstAssign{fl, memberRef(fl, handleVarp, VAccess::WRITE),
                                                    ನೂತನ AstVarRef{fl, argumentp, VAccess::READ}});

        // Route each enclosing-member reference through the back-pointer: 'm' -> 'h.m'.
        ಪ್ರತಿ (AstVarRef* ಧ್ರುವ refp : refsToRewrite) { rewriteVarRef(refp, handleVarp); }
        ಪ್ರತಿ (AstThisRef* ಧ್ರುವ refp : thisRefsToRewrite) { rewriteThisRef(refp, handleVarp); }
        ಪ್ರತಿ (AstFuncRef* ಧ್ರುವ refp : funcRefsToRewrite) { rewriteFuncRef(refp, handleVarp); }

        // Append a named hidden argument to preserve positional and defaulted user arguments.
        ಪ್ರತಿ (Ast_ಗಂಟು_Assign* ಧ್ರುವ constructp : constructps) {
            File_ಸಾಲು* ಧ್ರುವ cfl = constructp->fileline();
            AstCExpr* ಧ್ರುವ thisp = ನೂತನ AstCExpr{cfl, "this"};
            thisp->dtypep(enclDTypep);
            VN_AS(constructp->rhsp(), New)->addArgsp(ನೂತನ AstArg{cfl, argumentp->name(), thisp});
        }
        ಪ್ರತ್ಯಾಗಮ nullptr;
    }

    ಶೂನ್ಯ visit(AstClass* nodep) override {
        UINFO(9, "Visiting class: " << nodep->name() << " isCovergroup=" << nodep->isCovergroup());
        ಯದಿ (nodep->isCovergroup()) {
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
            m_embeddedVarp = findEmbeddedCovergroupVar();
            m_covergroupName = covergroupTypeName();
            m_sampleFuncp = nullptr;
            m_constructorp = nullptr;
            std::vector<EmbeddedEventTrigger> embeddedEventTriggers;
            Ast_Sen_ವೃಕ್ಷ* embeddedEventForkp = nullptr;

            // Extract and store the clocking event from AstCovergroup node
            // The parser creates this node to preserve the event information
            ತಾರ್ಕಿಕ hasUnsupportedEvent = ಅಸತ್ಯ;
            ಪ್ರತಿ (Ast_ಗಂಟು* itemp = nodep->membersp(); itemp;) {
                Ast_ಗಂಟು* ಧ್ರುವ nextp = itemp->nextp();
                ಯದಿ (AstCovergroup* ಧ್ರುವ cgp = VN_CAST(itemp, Covergroup)) {
                    // Store the event in the global map for V3Active to retrieve later
                    // V3LinkParse only creates this sentinel AstCovergroup node when a clocking
                    // event exists, so cgp->eventp() is always non-null here.
                    UASSERT_OBJ(cgp->eventp(), cgp,
                                "Sentinel AstCovergroup in class must have non-null eventp");
                    ಯದಿ (hasEnclosingEventRef(cgp)) {
                        UASSERT_OBJ(m_embeddedVarp, cgp,
                                    "Embedded covergroup event has no instance variable");
                        ಯದಿ (v3Global.opt.timing().isSetTrue()) {
                            embeddedEventForkp = cgp->eventp()->unlinkFrBack();
                        } ಅನ್ಯಥಾ {
                            embeddedEventTriggers = collectEmbeddedEventTriggers(cgp);
                            ಯದಿ (embeddedEventTriggers.empty()) {
                                cgp->v3warn(COVERIGN,
                                            "Unsupported: 'covergroup' clocking event on complex "
                                            "member expression; use --timing for full support.");
                                hasUnsupportedEvent = ಸತ್ಯ;
                            }
                        }
                        VL_DO_DANGLING(ತಳ್ಳು_Deletep(cgp->unlinkFrBack()), cgp);
                        itemp = nextp;
                        ಅನುವರ್ತ;
                    }
                    // V3Active handles events that do not depend on an enclosing instance.
                    UINFO(4, "Keeping covergroup event node for V3Active: " << nodep->name());
                    itemp = nextp;
                    ಅನುವರ್ತ;
                }
                itemp = nextp;
            }

            // Find the sample() method and constructor
            m_sampleFuncp = VN_CAST(m_memberMap.findMember(nodep, "sample"), Func);
            // V3LinkParse always synthesizes a sample() method for every covergroup, and the
            // sampling-code generation below dereferences m_sampleFuncp unconditionally.
            UASSERT_OBJ(m_sampleFuncp, nodep, "Covergroup missing synthesized sample() method");
            m_sampleFuncp->isCovergroupSample(ಸತ್ಯ);
            m_constructorp = VN_CAST(m_memberMap.findMember(nodep, "new"), Func);
            UINFO(9, "Found sample() method: " << (m_sampleFuncp ? "yes" : "no"));
            UINFO(9, "Found constructor: " << (m_constructorp ? "yes" : "no"));

            // If covergroup has unsupported clocking event, skip processing it
            // but still clean up coverpoints so they don't reach downstream passes
            ಯದಿ (hasUnsupportedEvent) {
                iterateChildren(nodep);
                validateCovergroupExpressions();
                rebindFormalRefs();
                delete_Coverage_ವಸ್ತುಗಳು();
                ಯದಿ (embeddedEventForkp) {
                    VL_DO_DANGLING(ತಳ್ಳು_Deletep(embeddedEventForkp), embeddedEventForkp);
                }
                ಪ್ರತ್ಯಾಗಮ;
            }

            iterateChildren(nodep);
            validateCovergroupExpressions();
            rebindFormalRefs();
            ಧ್ರುವ std::vector<Ast_ಗಂಟು_Assign*> constructps = findCovergroupConstructions();

            // Embedded covergroups (IEEE 1800-2023 19.4): coverpoints, iff expressions, and
            // crosses may reference members of the enclosing class. The covergroup is lowered
            // into a sibling class with no implicit handle to the enclosing instance. Install
            // an explicit back-pointer and route the references through it.
            ಯದಿ (AstVarRef* ಧ್ರುವ invalidp = installEnclosingBackPointer(constructps)) {
                invalidp->v3error("Non-static member "
                                  << invalidp->varp()->prettyNameQ()
                                  << " of an outer class requires an explicit "
                                     "object handle (IEEE 1800-2023 8.23).");
                delete_Coverage_ವಸ್ತುಗಳು();
                ಯದಿ (embeddedEventForkp) {
                    VL_DO_DANGLING(ತಳ್ಳು_Deletep(embeddedEventForkp), embeddedEventForkp);
                }
                ಪ್ರತ್ಯಾಗಮ;
            }
            installEmbeddedEventTriggers(embeddedEventTriggers, constructps);
            ಯದಿ (embeddedEventForkp) installEmbeddedEventFork(embeddedEventForkp, constructps);
            processCovergroup();
            // Remove lowered coverpoints/crosses from the class - they have been
            // fully translated into C++ code and must not reach downstream passes
            delete_Coverage_ವಸ್ತುಗಳು();
        } ಅನ್ಯಥಾ {
            // Track the lexically enclosing class so a nested covergroup can resolve
            // references to the enclosing object's members (installEnclosingBackPointer).
            VL_RESTORER(m_enclosingClassp);
            m_enclosingClassp = nodep;
            iterateChildren(nodep);
        }
    }

    ಶೂನ್ಯ visit(AstCoverpoint* nodep) override {
        UINFO(9, "Found coverpoint: " << nodep->name());
        m_coverpoints.ತಳ್ಳು_back(nodep);
        m_coverpointMap.emplace(nodep->name(), nodep);
        iterateChildren(nodep);
    }

    ಶೂನ್ಯ visit(AstCoverCross* nodep) override {
        UINFO(9, "Found cross: " << nodep->name());
        m_coverCrosses.ತಳ್ಳು_back(nodep);
        iterateChildren(nodep);
    }

    // V3Width leaves only the covergroup-level options lowerCovergroupOptions() takes
    ಶೂನ್ಯ visit(AstCgOptionAssign* nodep) override { m_cgOptions.ತಳ್ಳು_back(nodep); }

    // A package, interface, or module, so the design unit declaring the classes within
    ಶೂನ್ಯ visit(Ast_ಗಂಟು_Module* nodep) override {
        VL_RESTORER(m_unitp);
        m_unitp = nodep;
        iterateChildren(nodep);
    }

    ಶೂನ್ಯ visit(Ast_ಗಂಟು* nodep) override { iterateChildren(nodep); }

ಸಾರ್ವಜನಿಕ:
    // CONSTRUCTORS
    FunctionalCoverageVisitor(AstNetlist* nodep,
                              ಧ್ರುವ std::set<ಧ್ರುವ AstVar*>& mergeableTypeOptions)
        : m_mergeableTypeOptions{mergeableTypeOptions} {
        iterate(nodep);
    }
    ~FunctionalCoverageVisitor() override = ಸ್ವಭಾವ;
};

//######################################################################
// Functional coverage class functions

ಶೂನ್ಯ V3Covergroup::covergroup(AstNetlist* nodep) {
    UINFO(4, __FUNCTION__ << ": ");
    ಧ್ರುವ CovergroupAssignValidVisitor validVisitor{nodep};
    ಯದಿ (!validVisitor.valid()) V3Error::abortIfErrors();
    {  // Destruct before checking
        FunctionalCoverageVisitor{nodep, validVisitor.mergeableTypeOptions()};
    }
    V3Global::dump_Check_Global_ವೃಕ್ಷ("coveragefunc", 0, dump_ವೃಕ್ಷ_Either_Level() >= 3);
}
