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

#অন্তর্ভাব "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#অন্তর্ভাব "V3Covergroup.h"

#অন্তর্ভাব "V3Const.h"
#অন্তর্ভাব "V3Error.h"
#অন্তর্ভাব "V3File.h"
#অন্তর্ভাব "V3MemberMap.h"
#অন্তর্ভাব "V3UniqueNames.h"

#অন্তর্ভাব <bitset>
#অন্তর্ভাব <cmath>
#অন্তর্ভাব <deque>
#অন্তর্ভাব <set>
#অন্তর্ভাব <tuple>
#অন্তর্ভাব <unordered_map>
#অন্তর্ভাব <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Embedded covergroup assignment validation; and in the same pass over the netlist, the
// type_option variables through which SystemVerilog may set type_option.merge_instances

বর্গ CovergroupAssignValidVisitor final : সার্বজনিক VNVisitorConst {
    VMemberMap m_memberMap;
    std::map<ধ্রুব AstVar*, ধ্রুব Ast_গ্রন্থি_FTask*>
        m_constructors;  // Implicit instance -> constructor
    std::set<ধ্রুব AstVar*> m_mergeableTypeOptions;  // See mergeableTypeOptions()
    ধ্রুব Ast_গ্রন্থি_FTask* m_ftaskp = nullptr;
    তার্কিক m_collecting = সত্য;
    তার্কিক m_valid = সত্য;

    শূন্য visit(AstClass* nodep) override {
        VL_RESTORER(m_ftaskp);
        m_ftaskp = nullptr;
        যদি (m_collecting) {
            ধ্রুব Ast_গ্রন্থি_FTask* ধ্রুব constructorp
                = VN_CAST(m_memberMap.findMember(nodep, "new"), গ্রন্থি_FTask);
            প্রতি (ধ্রুব Ast_গ্রন্থি* itemp = nodep->membersp(); itemp; itemp = itemp->nextp()) {
                ধ্রুব AstVar* ধ্রুব varp = VN_CAST(itemp, Var);
                // Only the implicit instance is restricted, not explicitly typed aliases.
                যদি (!varp || !varp->isClassMember() || varp->isDeclTyped()) অনুবর্ত;
                ধ্রুব AstClassRefDType* ধ্রুব refp
                    = VN_CAST(varp->dtypep()->skipRefp(), ClassRefDType);
                যদি (refp && refp->classp()->covergroupEnclosingClassp() == nodep) {
                    m_constructors.emplace(varp, constructorp);
                }
            }
        }
        iterateChildrenConst(nodep);
    }
    শূন্য visit(Ast_গ্রন্থি_FTask* nodep) override {
        VL_RESTORER(m_ftaskp);
        m_ftaskp = nodep;
        iterateChildrenConst(nodep);
    }
    শূন্য visit(Ast_গ্রন্থি_Assign* nodep) override {
        যদি (!m_collecting) {
            ধ্রুব AstVar* varp = nullptr;
            যদি (ধ্রুব Ast_গ্রন্থি_Var_Ref* ধ্রুব refp = VN_CAST(nodep->lhsp(), গ্রন্থি_Var_Ref)) {
                varp = refp->varp();
            } অন্যথা যদি (ধ্রুব AstMemberSel* ধ্রুব selp = VN_CAST(nodep->lhsp(), MemberSel)) {
                varp = selp->varp();
            }
            ধ্রুব auto it = m_constructors.find(varp);
            যদি (it != m_constructors.end() && (!m_ftaskp || m_ftaskp != it->second)) {
                m_valid = অসত্য;
                nodep->v3error("Embedded covergroup variable "
                               << varp->prettyNameQ()
                               << " may only be assigned in the enclosing class's 'new' method "
                                  "(IEEE 1800-2023 19.4).");
            }
        }
        iterateChildrenConst(nodep);
    }
    শূন্য visit(Ast_গ্রন্থি_Var_Ref* nodep) override {
        // Type options may be set at any time (IEEE 1800-2023 19.7.1), so SystemVerilog may set
        // type_option.merge_instances with any use of a type_option but a read of another
        // member, as not every write is marked yet (std::randomize)
        যদি (m_collecting && nodep->varp()->name() == "type_option") {
            ধ্রুব AstStructSel* ধ্রুব selp = VN_CAST(nodep->backp(), StructSel);
            যদি (!selp || selp->fromp() != nodep || selp->name() == "merge_instances"
                || !nodep->access().isReadOnly()) {
                m_mergeableTypeOptions.ঢোকাও(nodep->varp());
            }
        }
        iterateChildrenConst(nodep);
    }
    শূন্য visit(Ast_গ্রন্থি* nodep) override { iterateChildrenConst(nodep); }

সার্বজনিক:
    explicit CovergroupAssignValidVisitor(AstNetlist* nodep) {
        // Uses can precede their enclosing class in the tree.
        iterateConst(nodep);
        m_collecting = অসত্য;
        যদি (!m_constructors.empty()) iterateConst(nodep);
    }
    তার্কিক valid() ধ্রুব { প্রত্যাগম m_valid; }
    // type_option variables through which SystemVerilog may set type_option.merge_instances, so
    // that covergroups are tested with optionVar(true)
    ধ্রুব std::set<ধ্রুব AstVar*>& mergeableTypeOptions() ধ্রুব { প্রত্যাগম m_mergeableTypeOptions; }
};

//######################################################################
// Covergroup expression validation visitor

বর্গ Covergroup_রাশিমালা_Valid_Visitor final : সার্বজনিক VNVisitor {
    ধ্রুব std::set<ধ্রুব AstVar*>& m_sampleMembers;
    ধ্রুব std::set<ধ্রুব AstVar*>& m_constructorRefMembers;
    তার্কিক m_inCoverageExpression = অসত্য;
    তার্কিক m_sampleFormalAllowed = অসত্য;

    শূন্য scanSampleExpression(Ast_গ্রন্থি* nodep) {
        যদি (!nodep) প্রত্যাগম;
        VL_RESTORER(m_sampleFormalAllowed);
        m_sampleFormalAllowed = সত্য;
        iterateAndNextNull(nodep);
    }

    শূন্য scanCoverageExpression(Ast_গ্রন্থি* nodep) {
        যদি (!nodep) প্রত্যাগম;
        VL_RESTORER(m_inCoverageExpression);
        m_inCoverageExpression = সত্য;
        iterateAndNextNull(nodep);
    }

    শূন্য visit(AstCoverpoint* nodep) override {
        scanSampleExpression(nodep->exprp());
        scanSampleExpression(nodep->iffp());
        iterateAndNextNull(nodep->binsp());
        iterateAndNextNull(nodep->optionsp());
    }
    শূন্য visit(AstCoverCross* nodep) override {
        iterateAndNextNull(nodep->itemsp());
        scanSampleExpression(nodep->iffp());
        iterateAndNextNull(nodep->optionsp());
        iterateAndNextNull(nodep->binsp());
    }
    শূন্য visit(AstCoverCrossBin* nodep) override {
        iterateAndNextNull(nodep->selectp());
        scanSampleExpression(nodep->iffp());
    }
    শূন্য visit(AstCoverCrossWith* nodep) override {
        iterateAndNextNull(nodep->selectp());
        scanCoverageExpression(nodep->filterp());
        scanCoverageExpression(nodep->countp());
    }
    শূন্য visit(AstCoverBinsof* nodep) override { scanCoverageExpression(nodep->rangesp()); }
    শূন্য visit(AstCoverBin* nodep) override {
        scanCoverageExpression(nodep->rangesp());
        scanSampleExpression(nodep->iffp());
        scanCoverageExpression(nodep->arraySizep());
        scanCoverageExpression(nodep->transp());
    }
    শূন্য visit(AstVarRef* nodep) override {
        যদি (!m_sampleFormalAllowed && m_sampleMembers.গুনতি(nodep->varp())) {
            nodep->v3error("Covergroup sample formal argument "
                           << nodep->varp()->prettyNameQ()
                           << " may only be used in a coverpoint or conditional guard "
                              "expression (IEEE 1800-2023 19.8.1).");
        }
        যদি (m_inCoverageExpression && m_constructorRefMembers.গুনতি(nodep->varp())) {
            nodep->v3error("Ref covergroup constructor formal argument "
                           << nodep->varp()->prettyNameQ()
                           << " may not be used in a covergroup expression "
                              "(IEEE 1800-2023 19.5).");
        }
    }
    শূন্য visit(Ast_গ্রন্থি_FTask_Ref* nodep) override {
        যদি (m_inCoverageExpression && nodep->taskp()) {
            তার্কিক invalidDirection = অসত্য;
            প্রতি (Ast_গ্রন্থি* stmtp = nodep->taskp()->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
                ধ্রুব AstVar* ধ্রুব varp = VN_CAST(stmtp, Var);
                যদি (varp && varp->isIO() && varp->isWritable()) {
                    invalidDirection = সত্য;
                    ভঙ্গ;
                }
            }
            যদি (invalidDirection) {
                nodep->v3error("Function " << nodep->taskp()->prettyNameQ()
                                           << " called in a covergroup expression has an "
                                              "output, inout, or non-const ref argument "
                                              "(IEEE 1800-2023 19.5).");
            }
        }
        iterateChildren(nodep);
    }
    শূন্য visit(Ast_গ্রন্থি* nodep) override { iterateChildren(nodep); }

সার্বজনিক:
    Covergroup_রাশিমালা_Valid_Visitor(ধ্রুব std::set<ধ্রুব AstVar*>& sampleMembers,
                               ধ্রুব std::set<ধ্রুব AstVar*>& constructorRefMembers)
        : m_sampleMembers{sampleMembers}
        , m_constructorRefMembers{constructorRefMembers} {}
    শূন্য scan(Ast_গ্রন্থি* nodep) { iterate(nodep); }
};

//######################################################################
// Bins of one declaration whose values are computed rather than listed: bin k covers
// [m_lo + k * m_stride, m_lo + (k + 1) * m_stride - 1], and the last bin extends to m_hi.  An
// array bin element is a run of single-value bins; automatic bins partition the coverpoint
// domain.  Bounds are coverpoint values at FunctionalCoverageVisitor::runWidth(),
// sign-extended like a CrossValueRange's.

বর্গ BinRun final {
সার্বজনিক:
    // MEMBERS
    uint32_t m_গুনতি;  // Number of bins
    V3_সংখ্যা m_lo;  // Lowest value of the first bin
    V3_সংখ্যা m_stride;  // Number of values of each bin but the last
    V3_সংখ্যা m_hi;  // Highest value of the last bin
    তার্কিক m_empty = অসত্য;  // A single bin without a value of the coverpoint type
    uint32_t m_declared = 0;  // Runtime index of the first bin, once generated

    // CONSTRUCTORS
    BinRun(Ast_গ্রন্থি* nodep, পূর্ণাঙ্ক width, uint32_t গুনতি)
        : m_গুনতি{গুনতি}
        , m_lo{nodep, width}
        , m_stride{nodep, width, 1}
        , m_hi{nodep, width} {}
};

//######################################################################
// Functional coverage visitor

বর্গ FunctionalCoverageVisitor final : সার্বজনিক VNVisitor {
    // NODE STATE
    // Entire netlist:
    //  AstCoverpoint::user1p()  -> AstVar*.  Previous-value variable for transition bins
    //  AstCoverpoint::user2()   -> bool.  Had a bins declaration ignored, so no automatic bins
    ধ্রুব VNUser1InUse m_inuser1;
    ধ্রুব VNUser2InUse m_inuser2;

    // STATE
    std::set<AstCoverpoint*>
        m_runtimePoints;  // Points needing value metadata and live-bin mapping
    std::set<AstCoverCross*> m_runtimeCrosses;  // Crosses over finalized live-bin dimensions
    std::map<AstVar*, AstVar*> m_excludedVars;  // Sample-time state-exclusion flags
    AstClass* m_covergroupp = nullptr;  // Current covergroup being processed
    Ast_গ্রন্থি_Module* m_unitp = nullptr;  // Design unit declaring the current class, outside classes
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
    তার্কিক m_cgMergeInstances = অসত্য;  // The covergroup's type_option.merge_instances, a constant
    // The covergroup's type_option.merge_instances is, or SystemVerilog may make it, true
    তার্কিক m_cgMayMerge = অসত্য;
    // type_option members through which SystemVerilog may set type_option.merge_instances,
    // see CovergroupAssignValidVisitor::mergeableTypeOptions()
    ধ্রুব std::set<ধ্রুব AstVar*>& m_mergeableTypeOptions;
    পূর্ণাঙ্ক m_cgAtLeast = 1;  // Covergroup option.at_least

    সংরচনা EmbeddedEventTrigger final {
        File_সারি* eventFl;  // Clocking-event source location
        AstVar* baseVarp;  // Base enclosing-class member in the event expression
        AstVar* memberVarp;  // Selected member in a 'base.member' expression, or nullptr
        VEdgeType edgeType;  // Clocking-event edge qualifier
        AstVar* prevVarp;  // Member containing the previous event value, or nullptr
        EmbeddedEventTrigger(File_সারি* eventFl, AstVar* baseVarp, AstVar* memberVarp,
                             VEdgeType edgeType)
            : eventFl{eventFl}
            , baseVarp{baseVarp}
            , memberVarp{memberVarp}
            , edgeType{edgeType}
            , prevVarp{nullptr} {}
    };

    std::set<std::string> m_crossedCpNames;  // Coverpoints referenced by a cross
    std::map<std::string, AstVar*> m_cpVarMap;  // Coverpoint name -> its VlCoverpoint member
    সংরচনা BinRuns final {
        std::vector<BinRun> runs;  // Runs of an array or automatic bins declaration, in order
        uint32_t গুনতি = 0;  // Bins across all runs
        তার্কিক unsupported = অসত্য;  // Too many bins, or invalid: the declaration is ignored
        // The value of each bin, which names it, of a wildcard array; else the bins are indexed
        std::vector<std::string> মানসমূহ;
        // Too many values of an ignore or illegal wildcard array, which is then one bin
        তার্কিক single = অসত্য;
    };
    সংরচনা Cross_Bin_মানসমূহ final {
        AstCoverBin* binp;  // Declaration owning this Normal bin
        Ast_গ্রন্থি_রাশিমালা* valuep;  // Individual array-bin value, or nullptr for a scalar bin
        ধ্রুব BinRun* runp = nullptr;  // Run computing the bin's values, if any
        uint32_t element = 0;  // Index of the bin within runp
    };
    সংরচনা BinSpan final {
        uint32_t first = 0;  // First Normal index of the bin declaration
        uint32_t গুনতি = 0;  // Number of Normal bins of the declaration
        uint32_t declared = 0;  // First runtime bin index, across all bin kinds
        int32_t sized = -1;  // Index of the sized array, placed at construction; or -1
    };
    সংরচনা CoverpointBins final {
        uint32_t total = 0;  // Number of Normal bins
        Ast_গ্রন্থি_রাশিমালা* exprp = nullptr;  // Sampled expression, for the value domain
        তার্কিক crossed = অসত্য;  // Feeds a cross, which needs 'values'
        std::vector<Cross_Bin_মানসমূহ> মানসমূহ;  // Values in runtime Normal-bin index order
        std::unordered_map<std::string, BinSpan> spans;  // Declared bin name -> index span
        BinSpan implicitAuto{0, 0, 0};  // Implicit automatic bins, each named 'auto_<i>'
        std::deque<BinRun> runs;  // Runs 'values' refers to
    };
    std::map<AstVar*, CoverpointBins> m_cpBins;  // Runtime coverpoint -> binsof index ranges
    // Names of the bins declarations each coverpoint ignored, which binsof selects as no bins
    std::map<ধ্রুব AstCoverpoint*, std::vector<std::string>> m_droppedBins;
    // Prefixes of the constructor temporaries of constructed bins, as coverpoint and bin names
    // alone may repeat: coverpoint 'a_' bins 'b', and coverpoint 'a' bins '_b'
    V3UniqueNames m_sizedNames{"__Vsized"};
    std::vector<Ast_গ্রন্থি_রাশিমালা*> m_detached_মানসমূহ;  // Array-bin values m_cpBins refers to
    std::set<AstCoverCross*>
        m_droppedCrosses;  // Crosses with a bare-variable item: drop (COVERIGN)
    std::map<uint32_t, AstCoverpointDType*> m_cpDTypes;  // Hit-list bound -> interned dtype
    using CrossShape = std::tuple<uint32_t, uint32_t, uint32_t, uint32_t, uint64_t, তার্কিক>;
    std::map<CrossShape, AstCoverCrossDType*> m_cxDTypes;
    AstVar* m_cgInstVarp = nullptr;  // __Vcg_inst handle member of the current covergroup

    VMemberMap m_memberMap;  // Member names cached for fast lookup

    // METHODS
    // The covergroup's 'option' or static 'type_option' member (V3LinkParse creates both)
    AstVar* optionVar(তার্কিক typeOption) {
        AstVar* ধ্রুব varp = VN_AS(
            m_memberMap.findMember(m_covergroupp, typeOption ? "type_option" : "option"), Var);
        UASSERT_OBJ(varp, m_covergroupp, "Covergroup missing option member");
        প্রত্যাগম varp;
    }

    // 'option.<member>' or 'type_option.<member>', per optionVarp
    AstStructSel* newOptionSel(File_সারি* fl, AstVar* optionVarp, ধ্রুব std::string& member,
                               VAccess access) {
        ধ্রুব AstMemberDType* ধ্রুব memberp
            = VN_AS(m_memberMap.findMember(optionVarp->dtypep()->skipRefp(), member), MemberDType);
        UASSERT_OBJ(memberp, optionVarp, "Coverage option structure missing '" << member << "'");
        Ast_গ্রন্থি_রাশিমালা* ধ্রুব fromp = optionVarp->lifetime().isStatic()
                                       ? নূতন AstVarRef{fl, optionVarp, access}
                                       : memberRef(fl, optionVarp, access);
        AstStructSel* ধ্রুব selp = নূতন AstStructSel{fl, fromp, member};
        selp->dtypep(memberp->subDTypep()->skipRefToEnump());
        selp->didWidth(সত্য);
        প্রত্যাগম selp;
    }

    // A constant option.at_least; a negative count of hits has no meaning, so takes the default
    স্থির পূর্ণাঙ্ক constAtLeast(ধ্রুব AstConst* constp) {
        ধ্রুব পূর্ণাঙ্ক মান = constp->toSInt();
        যদি (মান >= 0) প্রত্যাগম মান;
        constp->v3warn(COVERIGN, "Ignoring negative 'option.at_least'; using 1");
        প্রত্যাগম 1;
    }

    // An option's constant value, or nullptr after a warning, as non-constant is unsupported
    স্থির ধ্রুব AstConst* constOption(ধ্রুব Ast_গ্রন্থি* valuep, VCoverOptionType optType) {
        যদি (ধ্রুব AstConst* ধ্রুব constp = VN_CAST(valuep, Const)) প্রত্যাগম constp;
        valuep->v3warn(COVERIGN, "Ignoring unsupported: non-constant 'option."
                                     << optType.ascii() << "'; using default value");
        প্রত্যাগম nullptr;
    }

    // Store the covergroup-level options (IEEE 1800-2023 19.7) where SystemVerilog and the
    // runtime read them.  The instance options, option.weight and option.get_inst_coverage,
    // are evaluated by the constructor, as are the other instance options; the type options,
    // type_option.weight and type_option.merge_instances, are constant, and initialize the
    // static member.  Without its own type_option.merge_instances, a covergroup has the
    // default of --coverage-merge-instances.  A constant option.at_least is the default of
    // the coverpoints and crosses.
    শূন্য lowerCovergroupOptions() {
        তার্কিক mergeSet = অসত্য;  // The covergroup sets type_option.merge_instances
        m_cgAtLeast = 1;
        প্রতি (AstCgOptionAssign* ধ্রুব optp : m_cgOptions) {
            যদি (optp->optType() == VCoverOptionType::AT_LEAST) {
                যদি (ধ্রুব AstConst* ধ্রুব constp = constOption(optp->valuep(), optp->optType())) {
                    m_cgAtLeast = constAtLeast(constp);
                }
                VL_DO_DANGLING(ঠেলো_Deletep(optp->unlinkFrBack()), optp);
                অনুবর্ত;
            }
            File_সারি* ধ্রুব fl = optp->fileline();
            // V3Width left the type options constant, and type_option.weight non-negative
            ধ্রুব AstConst* ধ্রুব constp = VN_CAST(optp->valuep(), Const);
            std::string member = "weight";
            যদি (optp->optType() == VCoverOptionType::MERGE_INSTANCES) {
                member = "merge_instances";
                m_cgMergeInstances = !constp->অঙ্ক().isEqZero();
                mergeSet = সত্য;
            } অন্যথা যদি (optp->optType() == VCoverOptionType::GET_INST_COVERAGE) {
                member = "get_inst_coverage";
            } অন্যথা {
                UASSERT_OBJ(optp->optType() == VCoverOptionType::WEIGHT, optp,
                            "Unexpected covergroup option reaching V3Covergroup");
                যদি (optp->typeOption()) m_cgTypeWeight = constp->toUInt();
            }
            AstAssign* ধ্রুব assignp = নূতন AstAssign{
                fl, newOptionSel(fl, optionVar(optp->typeOption()), member, VAccess::WRITE),
                optp->valuep()->unlinkFrBack()};
            যদি (optp->typeOption()) {
                m_covergroupp->addMembersp(নূতন AstInitialStatic{fl, assignp});
                VL_DO_DANGLING(ঠেলো_Deletep(optp->unlinkFrBack()), optp);
            } অন্যথা {
                optp->replaceWith(assignp);
                VL_DO_DANGLING(ঠেলো_Deletep(optp), optp);
            }
        }
        m_cgOptions.clear();
        যদি (!mergeSet && v3Global.opt.coverageMergeInstances()) {
            File_সারি* ধ্রুব fl = m_covergroupp->fileline();
            m_covergroupp->addMembersp(নূতন AstInitialStatic{
                fl, নূতন AstAssign{
                        fl, newOptionSel(fl, optionVar(সত্য), "merge_instances", VAccess::WRITE),
                        নূতন AstConst{fl, AstConst::BitTrue{}}}});
            m_cgMergeInstances = সত্য;
        }
        m_cgMayMerge = m_cgMergeInstances || m_mergeableTypeOptions.গুনতি(optionVar(সত্য));
    }

    // The weight of an item in the coverage database, which merges the instances: its
    // type_option.weight if the covergroup merges them too (IEEE 1800-2023 19.11.3); else its
    // option.weight if a constant, and so of every instance; else its type_option.weight
    uint32_t itemDatabaseWeight(Ast_গ্রন্থি* optionsp) ধ্রুব {
        ধ্রুব Ast_গ্রন্থি_রাশিমালা* weightp = nullptr;  // The option.weight in effect
        uint32_t typeWeight = 1;
        প্রতি (Ast_গ্রন্থি* nodep = optionsp; nodep; nodep = nodep->nextp()) {
            ধ্রুব AstCoverOption* ধ্রুব optp = VN_AS(nodep, CoverOption);
            যদি (!(optp->optType() == VCoverOptionType::WEIGHT)) অনুবর্ত;
            // V3Width left type_option.weight a non-negative constant
            যদি (optp->typeOption()) {
                typeWeight = VN_AS(optp->valuep(), Const)->toUInt();
            } অন্যথা {
                weightp = optp->valuep();
            }
        }
        যদি (m_cgMergeInstances) প্রত্যাগম typeWeight;
        যদি (!weightp) প্রত্যাগম 1;
        যদি (ধ্রুব AstConst* ধ্রুব constp = VN_CAST(weightp, Const)) প্রত্যাগম constp->toUInt();
        প্রত্যাগম typeWeight;
    }

    // Configure an item's option.weight, its weight in instance coverage (IEEE 1800-2023
    // 19.11), and, if the covergroup may merge its instances, its type_option.weight, its
    // weight in type coverage then (19.11.3).
    শূন্য generateItemWeight(File_সারি* fl, AstVar* itemVarp, Ast_গ্রন্থি* optionsp) {
        প্রতি (Ast_গ্রন্থি* nodep = optionsp; nodep; nodep = nodep->nextp()) {
            ধ্রুব AstCoverOption* ধ্রুব optp = VN_AS(nodep, CoverOption);
            যদি (!(optp->optType() == VCoverOptionType::WEIGHT)) অনুবর্ত;
            যদি (!optp->typeOption()) {
                m_constructorp->addStmtsp(
                    itemCall(fl, itemVarp, VCMethod::COVERGROUP_WEIGHT,
                             {optp->valuep()->clone_বৃক্ষ(অসত্য), file_সারি_Debug(optp->fileline())})
                        ->makeStmt());
            } অন্যথা যদি (m_cgMayMerge) {
                // V3Width left type_option.weight a non-negative constant
                m_constructorp->addStmtsp(itemCall(fl, itemVarp, VCMethod::COVERGROUP_TYPE_WEIGHT,
                                                   {optp->valuep()->clone_বৃক্ষ(অসত্য)})
                                              ->makeStmt());
            }
        }
    }

    শূন্য processCovergroup() {
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
        m_cgMergeInstances = অসত্য;
        m_cgMayMerge = অসত্য;

        lowerCovergroupOptions();

        // Scan every cross item to record the coverpoints it references (the cross dimensions)
        // and to flag any cross naming a bare variable -- a would-be implicit coverpoint, which
        // Verilator does not synthesize.  An unresolvable item drops only that one cross (with a
        // COVERIGN in generateCrossCode), leaving the rest of the covergroup intact.
        প্রতি (AstCoverCross* crossp : m_coverCrosses) {
            প্রতি (Ast_গ্রন্থি* itemp = crossp->itemsp(); itemp; itemp = itemp->nextp()) {
                ধ্রুব AstCoverpointRef* ধ্রুব refp = VN_AS(itemp, CoverpointRef);
                যদি (refp->exprp()) অনুবর্ত;  // hierarchical ref: dropped in generateCrossCode
                যদি (m_coverpointMap.find(refp->name()) == m_coverpointMap.end()) {
                    m_droppedCrosses.ঢোকাও(crossp);  // bare variable: drop this cross only
                } অন্যথা {
                    m_crossedCpNames.ঢোকাও(refp->name());
                }
            }
        }

        std::vector<Ast_গ্রন্থি*> pending;
        std::map<AstCoverpoint*, std::vector<AstCoverCross*>> consumers;
        std::map<AstCoverCross*, std::vector<AstCoverpoint*>> inputs;
        প্রতি (AstCoverpoint* ধ্রুব cpp : m_coverpoints) {
            checkBinNames(cpp);
            checkConstructedBins(cpp);
            যদি (!cpp->exprp()->dtypep()->skipRefp()->isIntegralOrPacked()) অনুবর্ত;
            // Bins without values leave the report (IEEE 1800-2023 19.11.1), exclusions or not.
            // Constructed bins get their values when the covergroup is constructed.
            যদি (!coverpointHasStateExclusions(cpp) && !coverpointHasEmptyBins(cpp)
                && !coverpointHasConstructedBins(cpp)) {
                অনুবর্ত;
            }
            m_runtimePoints.ঢোকাও(cpp);
            pending.ঠেলো_back(cpp);
        }
        প্রতি (AstCoverCross* ধ্রুব crossp : m_coverCrosses) {
            যদি (m_droppedCrosses.গুনতি(crossp)) অনুবর্ত;
            প্রতি (Ast_গ্রন্থি* itemp = crossp->itemsp(); itemp; itemp = itemp->nextp()) {
                ধ্রুব AstCoverpointRef* ধ্রুব refp = VN_AS(itemp, CoverpointRef);
                যদি (refp->exprp()) অনুবর্ত;
                ধ্রুব auto point = m_coverpointMap.find(refp->name());
                যদি (point != m_coverpointMap.end()) {
                    consumers[point->second].ঠেলো_back(crossp);
                    inputs[crossp].ঠেলো_back(point->second);
                }
            }
            // The values of a filter's bin tuples are known when the covergroup is constructed
            যদি (crossp->exists([](ধ্রুব AstCoverCrossWith*) { প্রত্যাগম সত্য; })) {
                m_runtimeCrosses.emplace(crossp);
                pending.ঠেলো_back(crossp);
            }
        }
        প্রতি (size_t next = 0; next < pending.size(); ++next) {
            যদি (AstCoverpoint* ধ্রুব pointp = VN_CAST(pending[next], Coverpoint)) {
                প্রতি (AstCoverCross* ধ্রুব crossp : consumers[pointp]) {
                    যদি (m_runtimeCrosses.emplace(crossp).second) pending.ঠেলো_back(crossp);
                }
            } অন্যথা {
                প্রতি (AstCoverpoint* ধ্রুব pointp : inputs[VN_AS(pending[next], CoverCross)]) {
                    যদি (pointp->exprp()->dtypep()->skipRefp()->isIntegralOrPacked()
                        && m_runtimePoints.emplace(pointp).second) {
                        pending.ঠেলো_back(pointp);
                    }
                }
            }
        }

        // The instance node owns this instance's coverpoint/cross runtimes, so it must exist
        // before any of them is created.  Emitted first, ahead of both generate loops.
        generateInstanceAttach();

        // For each coverpoint, generate sampling code
        প্রতি (AstCoverpoint* cpp : m_coverpoints) generateCoverpointCode(cpp);

        // For each cross, generate sampling code
        প্রতি (AstCoverCross* crossp : m_coverCrosses) generateCrossCode(crossp);

        // Every cross has been built, so runtime points only need their exclusions from here.
        প্রতি (AstCoverpoint* ধ্রুব cpp : m_coverpoints) {
            যদি (!m_runtimePoints.গুনতি(cpp)) অনুবর্ত;
            m_constructorp->addStmtsp(itemCall(cpp->fileline(), m_cpVarMap.at(cpp->name()),
                                               VCMethod::COVERGROUP_মান_RELEASE)
                                          ->makeStmt());
        }
        প্রতি (Ast_গ্রন্থি_রাশিমালা* valuep : m_detached_মানসমূহ) VL_DO_DANGLING(ঠেলো_Deletep(valuep), valuep);
        m_detached_মানসমূহ.clear();

        // Generate coverage computation code (even for empty covergroups).  Bin registration
        // with the coverage database is handled per coverpoint/cross by their runtime
        // registerBins() calls (emitted in generateCoverpoint/generateCross).
        generateCoverageComputationCode();
    }

    স্থির constexpr size_t মান_LIST_ENTRIES = 256;  // Metadata entries per constructor call

    // The value of a constant count, of bins or value tuples: -1 if it is negative, and
    // saturated above the largest limit
    স্থির int64_t const_গুনতি(ধ্রুব AstConst* constp) {
        ধ্রুব V3_সংখ্যা& অঙ্ক = constp->অঙ্ক();
        যদি (constp->isSigned() && অঙ্ক.isNegative()) প্রত্যাগম -1;
        প্রত্যাগম অঙ্ক.mostSetBitP1() > 32 ? INT64_MAX : static_cast<int64_t>(অঙ্ক.toUQuad());
    }

    // Whether a count, 'whatp', is integral and, if constant, positive, as the standard's clause
    // 'clausep' requires; else errors
    স্থির তার্কিক check_গুনতি(ধ্রুব Ast_গ্রন্থি_রাশিমালা* countp, ধ্রুব বর্ণ* whatp, ধ্রুব বর্ণ* clausep) {
        ধ্রুব AstConst* ধ্রুব constp = VN_CAST(countp, Const);
        যদি (!countp->dtypep()->skipRefp()->isIntegralOrPacked()) {
            countp->v3error(whatp << " must be integral (IEEE 1800-2023 " << clausep << ")");
            প্রত্যাগম অসত্য;
        } অন্যথা যদি (constp && (constp->অঙ্ক().isFourState() || const_গুনতি(constp) < 1)) {
            countp->v3error(whatp << " must be >= 1, got "
                                  << (constp->অঙ্ক().isFourState() ? constp->অঙ্ক().ascii(অসত্য)
                                                                  : constp->অঙ্ক().toDecimalS())
                                  << " (IEEE 1800-2023 " << clausep << ")");
            প্রত্যাগম অসত্য;
        }
        প্রত্যাগম সত্য;
    }

    // The number of bins requested by a valid 'bins auto[N]', or 0
    স্থির uint32_t autoBinsRequested(ধ্রুব AstCoverBin* binp) {
        ধ্রুব AstConst* ধ্রুব constp = VN_CAST(binp->arraySizep(), Const);
        যদি (!constp) প্রত্যাগম 0;
        ধ্রুব int64_t গুনতি = const_গুনতি(constp);
        প্রত্যাগম গুনতি < 1 || গুনতি > v3Global.opt.coverageMaxBins() ? 0
                                                                   : static_cast<uint32_t>(গুনতি);
    }

    // True for a 'bins auto[N]' declaration, or the implicit automatic bins of a coverpoint
    স্থির তার্কিক isAutoBins(ধ্রুব AstCoverBin* binp) {
        প্রত্যাগম binp->binsType() == VCoverBinsType::BINS_AUTO
               || binp->binsType() == VCoverBinsType::BINS_AUTO_IMPLICIT;
    }

    // True for a sized array of bins, 'bins b[N] = {...}', whose values an integral coverpoint
    // distributes over N bins when the covergroup is constructed (IEEE 1800-2023 19.5.1)
    স্থির তার্কিক isSizedArray(ধ্রুব AstCoverBin* binp) {
        প্রত্যাগম binp->arraySizep() && !isAutoBins(binp);
    }

    // The 'with' filter of a bin, if any (IEEE 1800-2023 19.5.1.1)
    স্থির AstCoverWith* binWith(ধ্রুব AstCoverBin* binp) {
        প্রত্যাগম VN_CAST(binp->rangesp(), CoverWith);
    }

    // True for bins whose values the covergroup constructor computes: a sized array, or bins of
    // the values a 'with' filter keeps
    স্থির তার্কিক isConstructedBins(ধ্রুব AstCoverBin* binp) {
        প্রত্যাগম isSizedArray(binp) || binWith(binp);
    }

    // The range list of a bin's values, or of the candidates of its 'with' filter; null for all
    // of the coverpoint's values, which a filter of the coverpoint's name has
    স্থির Ast_গ্রন্থি* binRangesp(ধ্রুব AstCoverBin* binp) {
        ধ্রুব AstCoverWith* ধ্রুব withp = binWith(binp);
        যদি (!withp) প্রত্যাগম binp->rangesp();
        প্রত্যাগম VN_IS(withp->subp(), CoverpointRef) ? nullptr : withp->subp();
    }

    // Report and delete a bins declaration of a name that another of its coverpoint has
    শূন্য checkBinNames(AstCoverpoint* coverpointp) {
        std::set<std::string> names;
        প্রতি (Ast_গ্রন্থি* nodep = coverpointp->binsp(); nodep;) {
            AstCoverBin* ধ্রুব binp = VN_AS(nodep, CoverBin);
            nodep = nodep->nextp();
            যদি (names.emplace(binp->name()).second) অনুবর্ত;
            binp->v3error("Duplicate bin " << binp->prettyNameQ() << " in coverpoint "
                                           << coverpointp->prettyNameQ()
                                           << " (IEEE 1800-2023 3.13)");
            VL_DO_DANGLING(ঠেলো_Deletep(binp->unlinkFrBack()), binp);
        }
    }

    // Delete an ignored bins declaration, which binsof then selects as no bins
    শূন্য dropBins(ধ্রুব AstCoverpoint* coverpointp, AstCoverBin* binp) {
        m_droppedBins[coverpointp].ঠেলো_back(binp->name());
        VL_DO_DANGLING(ঠেলো_Deletep(binp->unlinkFrBack()), binp);
    }

    // Check the size of a sized array of bins, which drops an invalid array.  A real coverpoint's
    // are unsupported, and treated as arrays of a bin per value.  False unless it stays sized.
    তার্কিক checkBinsArraySize(ধ্রুব AstCoverpoint* coverpointp, AstCoverBin* binp, তার্কিক integral) {
        Ast_গ্রন্থি_রাশিমালা* ধ্রুব sizep = binp->arraySizep();
        যদি (VN_IS(sizep, Unbounded)) {  // A parameter of '$'; see bins_orBraE
            binp->v3error("Bins array size must be integral, not '$' (IEEE 1800-2023 19.5.1)");
        } অন্যথা যদি (check_গুনতি(sizep, "Bins array size", "19.5.1")) {
            যদি (integral) প্রত্যাগম সত্য;
            binp->v3warn(COVERIGN, "Unsupported: 'bins' explicit array size of a real "
                                   "coverpoint (treated as '[]')");
            VL_DO_DANGLING(ঠেলো_Deletep(sizep->unlinkFrBack()), sizep);
            প্রত্যাগম অসত্য;
        }
        dropBins(coverpointp, binp);
        প্রত্যাগম অসত্য;
    }

    // Check the bins of a coverpoint whose values the constructor computes, dropping invalid
    // ones.  A wildcard array of too many ranges of values is ignored, or if ignore or illegal,
    // treated as one bin; filtering too many ranges of values ignores the bins.
    শূন্য checkConstructedBins(AstCoverpoint* coverpointp) {
        ধ্রুব তার্কিক integral = coverpointp->exprp()->dtypep()->skipRefp()->isIntegralOrPacked();
        প্রতি (Ast_গ্রন্থি* nodep = coverpointp->binsp(); nodep;) {
            AstCoverBin* ধ্রুব binp = VN_AS(nodep, CoverBin);
            nodep = nodep->nextp();
            যদি (!isConstructedBins(binp)) অনুবর্ত;
            যদি (isSizedArray(binp) && !checkBinsArraySize(coverpointp, binp, integral)) অনুবর্ত;
            যদি (!binp->isWildcard()
                || sizedWildcardRuns(binp, coverpointp->exprp())
                       <= v3Global.opt.coverageMaxBins()) {
                যদি (binWith(binp) && withCandidatesOver(binp, coverpointp->exprp())) {
                    binp->v3warn(COVERIGN, "Unsupported: 'with' filter of more than 2**32 "
                                           "candidate values; bin "
                                               << binp->prettyNameQ() << " ignored");
                    যদি (binp->binsType().binIsNormal()) coverpointp->user2(সত্য);
                    dropBins(coverpointp, binp);
                }
                অনুবর্ত;
            }
            // An ignore or illegal array still excludes or checks its values, as one bin
            ধ্রুব তার্কিক single = !binp->binsType().binIsNormal() && !binWith(binp);
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
            যদি (single) {
                Ast_গ্রন্থি_রাশিমালা* ধ্রুব sizep = binp->arraySizep();
                VL_DO_DANGLING(ঠেলো_Deletep(sizep->unlinkFrBack()), sizep);
                binp->isArray(অসত্য);
                অনুবর্ত;
            }
            যদি (binp->binsType().binIsNormal()) coverpointp->user2(সত্য);
            dropBins(coverpointp, binp);
        }
    }

    // True if a 'with' filter would be evaluated for more than 2**32 candidate values, known
    // now for the coverpoint's name, or for a range list of constants: each is evaluated once
    // but for an array 'b[N]', which keeps their order and duplicates (see withBegin())
    স্থির তার্কিক withCandidatesOver(AstCoverBin* binp, Ast_গ্রন্থি_রাশিমালা* exprp) {
        ধ্রুব পূর্ণাঙ্ক width = runWidth(exprp);
        std::vector<std::pair<V3_সংখ্যা, V3_সংখ্যা>> runs;
        যদি (!binRangesp(binp)) runs = coverpoint_মানসমূহ(binp, exprp);
        ধ্রুব auto constant
            = [](ধ্রুব Ast_গ্রন্থি* nodep) { প্রত্যাগম VN_IS(nodep, Const) || VN_IS(nodep, Unbounded); };
        প্রতি (Ast_গ্রন্থি* rangep = binRangesp(binp); rangep; rangep = rangep->nextp()) {
            ধ্রুব AstInsideRange* ধ্রুব irp = VN_CAST(rangep, InsideRange);
            // Else known when constructed
            যদি (irp ? !constant(irp->lhsp()) || !constant(irp->rhsp()) : !VN_IS(rangep, Const)) {
                প্রত্যাগম অসত্য;
            }
            Cross_মান_Range range{rangep, resolveWidth(rangep, exprp)};
            যদি (!resolve_মান(rangep, exprp, সত্য, binp->isWildcard(), range)
                || crossRangeEmpty(range)) {
                অনুবর্ত;
            }
            std::vector<std::pair<V3_সংখ্যা, V3_সংখ্যা>> found{{range.lo, range.hi}};
            যদি (range.wildcard) {  // checkConstructedBins bounded the runs
                found.clear();
                crossRangeRuns(range, v3Global.opt.coverageMaxBins(), found);
            }
            প্রতি (ধ্রুব std::pair<V3_সংখ্যা, V3_সংখ্যা>& run : found) {
                runs.emplace_back(V3_সংখ্যা{rangep, width, run.first},
                                  V3_সংখ্যা{rangep, width, run.second});
            }
        }
        যদি (!isSizedArray(binp)) {  // The union of the values
            std::sort(runs.begin(), runs.end(), [](ধ্রুব auto& lhs, ধ্রুব auto& rhs) {
                প্রত্যাগম cross_মান_Less(lhs.first, rhs.first);
            });
            std::vector<std::pair<V3_সংখ্যা, V3_সংখ্যা>> merged;
            প্রতি (ধ্রুব std::pair<V3_সংখ্যা, V3_সংখ্যা>& run : runs) {
                যদি (merged.empty() || cross_মান_Less(merged.back().second, run.first)) {
                    merged.ঠেলো_back(run);
                } অন্যথা যদি (cross_মান_Less(merged.back().second, run.second)) {
                    merged.back().second = run.second;
                }
            }
            runs = std::move(merged);
        }
        uint64_t গুনতি = 0;
        প্রতি (ধ্রুব std::pair<V3_সংখ্যা, V3_সংখ্যা>& run : runs) {
            V3_সংখ্যা span{exprp, width};
            span.opSub(run.second, run.first);
            যদি (span.mostSetBitP1() > 32) প্রত্যাগম সত্য;  // More than 2**32 values
            গুনতি += span.toUQuad() + 1;
            যদি (গুনতি > (uint64_t{1} << 32)) প্রত্যাগম সত্য;
        }
        প্রত্যাগম অসত্য;
    }

    // The ranges of values the wildcard patterns of a sized wildcard array, or of a 'with'
    // filter's candidates, give, counted up to more than --coverage-max-bins
    স্থির size_t sizedWildcardRuns(ধ্রুব AstCoverBin* binp, Ast_গ্রন্থি_রাশিমালা* exprp) {
        std::vector<std::pair<V3_সংখ্যা, V3_সংখ্যা>> runs;
        প্রতি (Ast_গ্রন্থি* rangep = binRangesp(binp); rangep; rangep = rangep->nextp()) {
            যদি (!VN_IS(rangep, Const)) অনুবর্ত;  // A range, or a value known at construction
            Cross_মান_Range range{rangep, resolveWidth(rangep, exprp)};
            যদি (resolve_মান(rangep, exprp, সত্য, সত্য, range) && !crossRangeEmpty(range)) {
                crossRangeRuns(range, v3Global.opt.coverageMaxBins(), runs);
            }
        }
        প্রত্যাগম runs.size();
    }

    // Check the automatic bins declarations of a coverpoint.  Each stays one declaration, which
    // generates as a partition of the coverpoint domain (see autoBinRuns).
    শূন্য checkAutomaticBins(AstCoverpoint* coverpointp, ধ্রুব Ast_গ্রন্থি_রাশিমালা* exprp) {
        প্রতি (Ast_গ্রন্থি* binp = coverpointp->binsp(); binp; binp = binp->nextp()) {
            AstCoverBin* ধ্রুব cbinp = VN_AS(binp, CoverBin);
            যদি (cbinp->binsType() != VCoverBinsType::BINS_AUTO) অনুবর্ত;
            ধ্রুব AstConst* ধ্রুব constp = VN_CAST(cbinp->arraySizep(), Const);
            যদি (!constp) {
                cbinp->v3error("Automatic bins array size must be a constant");
            } অন্যথা যদি (const_গুনতি(constp) < 1) {
                cbinp->v3error("Automatic bins array size must be >= 1, got "
                               << constp->অঙ্ক().toDecimalS());
            } অন্যথা যদি (const_গুনতি(constp) > v3Global.opt.coverageMaxBins()) {
                cbinp->v3error("Automatic bins array size of "
                               << constp->অঙ্ক().toDecimalU() << " exceeds limit of "
                               << v3Global.opt.coverageMaxBins() << '\n'
                               << cbinp->warnMore() << "... Suggest a larger --coverage-max-bins");
            } অন্যথা যদি (!exprp->dtypep()->skipRefp()->isIntegralOrPacked()) {
                cbinp->v3error("Automatic bins are not allowed on a coverpoint of a non-integral "
                               "expression (IEEE 1800-2023 19.5.3).");
            }
        }
    }

    // Extract all option values of a coverpoint or cross in a single pass.
    // atLeastOut: option.at_least (default the covergroup's)
    // autoBinMaxOut: option.auto_bin_max (coverpoint overrides covergroup, default 64)
    শূন্য extractItemOptions(Ast_গ্রন্থি* optionsp, পূর্ণাঙ্ক& atLeastOut, পূর্ণাঙ্ক& autoBinMaxOut) {
        atLeastOut = m_cgAtLeast;
        autoBinMaxOut = -1;  // -1 = not set at coverpoint level
        প্রতি (Ast_গ্রন্থি* optionp = optionsp; optionp; optionp = optionp->nextp()) {
            AstCoverOption* ধ্রুব optp = VN_AS(optionp, CoverOption);
            // Weights may be non-constant; generateItemWeight() handles them.  A cross keeps
            // its unsupported options, which V3LinkParse warned of.
            যদি (!(optp->optType() == VCoverOptionType::AT_LEAST
                  || optp->optType() == VCoverOptionType::AUTO_BIN_MAX)) {
                অনুবর্ত;
            }
            ধ্রুব AstConst* ধ্রুব constp = constOption(optp->valuep(), optp->optType());
            যদি (!constp) অনুবর্ত;
            যদি (optp->optType() == VCoverOptionType::AT_LEAST) {
                atLeastOut = constAtLeast(constp);
            } অন্যথা {
                autoBinMaxOut = constp->toSInt();
            }
        }
        // Fall back to covergroup-level auto_bin_max if not set at coverpoint level
        যদি (autoBinMaxOut < 0) {
            যদি (m_covergroupp->cgAutoBinMax() >= 0) {
                autoBinMaxOut = m_covergroupp->cgAutoBinMax();
            } অন্যথা {
                autoBinMaxOut = 64;  // Default per IEEE 1800-2023 Table 19-1
            }
        }
    }

    // IEEE 1800-2023 19.5.2: an enum coverpoint has one automatic bin per enumeration value
    শূন্য createEnumAutoBins(AstCoverpoint* coverpointp, Ast_গ্রন্থি_রাশিমালা* exprp,
                            ধ্রুব AstEnumDType* enump) {
        File_সারি* ধ্রুব fl = coverpointp->fileline();
        প্রতি (ধ্রুব AstEnumItem* itemp = enump->itemsp(); itemp;
             itemp = VN_AS(itemp->nextp(), EnumItem)) {
            AstConst* ধ্রুব lop = new_মান_Const(fl, VN_AS(itemp->valuep(), Const)->অঙ্ক(), exprp);
            AstInsideRange* ধ্রুব rangep = নূতন AstInsideRange{fl, lop, lop->clone_বৃক্ষ(অসত্য)};
            rangep->dtypeFrom(exprp);
            coverpointp->addBinsp(
                নূতন AstCoverBin{fl, "auto[" + itemp->name() + "]", rangep, অসত্য, অসত্য});
        }
    }

    // IEEE 1800-2023 19.5.3/19.11.1: partition first, then apply exclusions.  The partition is one
    // automatic bins declaration, generated as a run like 'bins auto[N]' but numbering its bins.
    শূন্য createImplicitAutoBins(AstCoverpoint* coverpointp, Ast_গ্রন্থি_রাশিমালা* exprp, পূর্ণাঙ্ক autoBinMax) {
        যদি (coverpointp->user2()) প্রত্যাগম;  // Declared bins, ignored, leave no bins
        প্রতি (Ast_গ্রন্থি* nodep = coverpointp->binsp(); nodep; nodep = nodep->nextp()) {
            ধ্রুব VCoverBinsType kind = VN_AS(nodep, CoverBin)->binsType();
            যদি (kind != VCoverBinsType::BINS_IGNORE && kind != VCoverBinsType::BINS_ILLEGAL)
                প্রত্যাগম;
        }
        যদি (ধ্রুব AstEnumDType* ধ্রুব enump
            = VN_CAST(exprp->dtypep()->skipRefToEnump(), EnumDType)) {
            createEnumAutoBins(coverpointp, exprp, enump);
            প্রত্যাগম;
        }
        ধ্রুব পূর্ণাঙ্ক width = exprp->width();
        uint32_t গুনতি = width < 31 ? std::min<uint32_t>(uint32_t{1} << width, autoBinMax)
                                    : static_cast<uint32_t>(autoBinMax);
        যদি (!গুনতি) প্রত্যাগম;
        যদি (!exprp->dtypep()->skipRefp()->isIntegralOrPacked()) {
            coverpointp->v3error("Coverpoint of a non-integral expression requires explicit bins "
                                 "(IEEE 1800-2023 19.5.3).");
            প্রত্যাগম;
        }
        যদি (গুনতি > v3Global.opt.coverageMaxBins()) {
            coverpointp->v3warn(COVERIGN, "Unsupported: more than "
                                              << v3Global.opt.coverageMaxBins()
                                              << " automatic bins from 'option.auto_bin_max'; "
                                                 "using "
                                              << v3Global.opt.coverageMaxBins() << ".\n"
                                              << coverpointp->warnMore()
                                              << "... Suggest a larger --coverage-max-bins");
            গুনতি = v3Global.opt.coverageMaxBins();
        }
        File_সারি* ধ্রুব fl = coverpointp->fileline();
        coverpointp->addBinsp(নূতন AstCoverBin{fl, "auto", নূতন AstConst{fl, গুনতি},
                                              VCoverBinsType::BINS_AUTO_IMPLICIT});
    }

    // Sanitize generated names to be valid C++ identifiers
    স্থির string sanitizeGeneratedName(string name) {
        std::replace(name.begin(), name.end(), '[', '_');
        std::replace(name.begin(), name.end(), ']', '_');
        প্রত্যাগম name;
    }

    // Capture an iff guard in a function-local temporary so it is evaluated once per sample()
    AstVarRef* captureIffToTemp(Ast_গ্রন্থি_রাশিমালা* iffp, ধ্রুব string& tempName) {
        File_সারি* ধ্রুব fl = iffp->fileline();
        AstVar* ধ্রুব iffVarp
            = নূতন AstVar{fl, VVarType::BLOCKTEMP, tempName, iffp->findBitDType()};
        iffVarp->funcLocal(সত্য);
        m_sampleFuncp->addStmtsp(iffVarp);
        iffp->unlinkFrBack();
        m_sampleFuncp->addStmtsp(
            নূতন AstAssign{fl, নূতন AstVarRef{fl, iffVarp, VAccess::WRITE}, iffp});
        প্রত্যাগম নূতন AstVarRef{fl, iffVarp, VAccess::READ};
    }

    // Add a statement sampling the current coverpoint to sample(), under its iff guard if any
    শূন্য addSampleStmt(Ast_গ্রন্থি* stmtp) {
        যদি (m_cpGuardp) {
            m_cpGuardp->addThensp(stmtp);
        } অন্যথা {
            m_sampleFuncp->addStmtsp(stmtp);
        }
    }

    // Create previous value variable for transition tracking
    AstVar* create_Prev_মান_Var(AstCoverpoint* coverpointp, Ast_গ্রন্থি_রাশিমালা* exprp) {
        // Check if already created
        যদি (AstVar* ধ্রুব prevVarp = VN_CAST(coverpointp->user1p(), Var)) প্রত্যাগম prevVarp;

        // Create variable to store previous sampled value
        ধ্রুব string varName = "__Vprev_" + coverpointp->name();
        AstVar* prevVarp
            = নূতন AstVar{coverpointp->fileline(), VVarType::MEMBER, varName, exprp->dtypep()};
        m_covergroupp->addMembersp(prevVarp);

        UINFO(4, "    Created previous value variable: " << varName);

        // Initialize to zero in constructor
        Ast_গ্রন্থি_রাশিমালা* ধ্রুব initExprp
            = নূতন AstConst{prevVarp->fileline(), AstConst::Widthed_মান{}, prevVarp->width(), 0};
        Ast_গ্রন্থি_Stmt* ধ্রুব initStmtp = নূতন AstAssign{
            prevVarp->fileline(), নূতন AstVarRef{prevVarp->fileline(), prevVarp, VAccess::WRITE},
            initExprp};
        m_constructorp->addStmtsp(initStmtp);

        coverpointp->user1p(prevVarp);
        প্রত্যাগম prevVarp;
    }

    // Create state position variable for multi-value transition bins
    // Tracks position in sequence: 0=not started, 1=seen first item, etc.
    AstVar* createSequenceStateVar(AstCoverpoint* coverpointp, AstCoverBin* binp) {
        // Create variable to track sequence position
        ধ্রুব string varName = "__Vseqpos_" + coverpointp->name() + "_" + binp->name();
        // Use 8-bit integer for state position (sequences rarely > 255 items)
        AstVar* stateVarp
            = নূতন AstVar{binp->fileline(), VVarType::MEMBER, varName, VFlagLogicPacked{}, 8};
        m_covergroupp->addMembersp(stateVarp);

        UINFO(4, "    Created sequence state variable: " << varName);

        // Initialize to 0 (not started) in constructor
        Ast_গ্রন্থি_Stmt* ধ্রুব initStmtp = নূতন AstAssign{
            stateVarp->fileline(), নূতন AstVarRef{stateVarp->fileline(), stateVarp, VAccess::WRITE},
            নূতন AstConst{stateVarp->fileline(), AstConst::Widthed_মান{}, 8, 0}};
        m_constructorp->addStmtsp(initStmtp);

        প্রত্যাগম stateVarp;
    }

    শূন্য generateCoverpointCode(AstCoverpoint* coverpointp) {
        UINFO(4, "  Generating code for coverpoint: " << coverpointp->name());

        // Get the coverpoint expression
        Ast_গ্রন্থি_রাশিমালা* exprp = coverpointp->exprp();

        // Check automatic bins before processing
        checkAutomaticBins(coverpointp, exprp);

        // Extract all coverpoint options in a single pass
        পূর্ণাঙ্ক at_Least_মান;
        পূর্ণাঙ্ক autoBinMax;
        extractItemOptions(coverpointp->optionsp(), at_Least_মান, autoBinMax);
        UINFO(6, "    Coverpoint at_least = " << at_Least_মান << " auto_bin_max = " << autoBinMax);

        // Create implicit automatic bins if no regular bins exist
        createImplicitAutoBins(coverpointp, exprp, autoBinMax);

        // A false iff guard disables sampling of the coverpoint (IEEE 1800-2023 19.5), so
        // neither is its expression evaluated, nor are its bins or transition state updated
        VL_RESTORER(m_cpGuardp);
        যদি (Ast_গ্রন্থি_রাশিমালা* ধ্রুব iffp = coverpointp->iffp()) {
            m_cpGuardp = নূতন AstIf{iffp->fileline(), iffp->unlinkFrBack()};
        }

        AstVar* ধ্রুব মান_Varp = নূতন AstVar{
            coverpointp->fileline(), VVarType::BLOCKTEMP,
            "__VcpValue_" + sanitizeGeneratedName(coverpointp->name()), exprp->dtypep()};
        মান_Varp->funcLocal(সত্য);
        m_sampleFuncp->addStmtsp(মান_Varp);
        exprp->unlinkFrBack();
        addSampleStmt(নূতন AstAssign{
            coverpointp->fileline(),
            নূতন AstVarRef{coverpointp->fileline(), মান_Varp, VAccess::WRITE}, exprp});
        coverpointp->exprp(নূতন AstVarRef{coverpointp->fileline(), মান_Varp, VAccess::READ});
        exprp = coverpointp->exprp();

        // Every coverpoint routes through the VlCoverpoint runtime.  Transition coverpoints are
        // included: their per-value matching is still generated as a state machine in sample()
        // (see generateCoverpoint), but the bin hit is recorded in the runtime bin
        // rather than a bare counter.
        generateCoverpoint(coverpointp, exprp, at_Least_মান);
        // Sampling follows what generateCoverpoint added unguarded, such as clearing the hit list
        যদি (m_cpGuardp) m_sampleFuncp->addStmtsp(m_cpGuardp);
    }

    // Build the condition under which a default bin matches: NOT(OR of all normal bins).
    Ast_গ্রন্থি_রাশিমালা* buildDefaultCondition(AstCoverpoint* coverpointp, Ast_গ্রন্থি_রাশিমালা* exprp,
                                       File_সারি* fl) {
        Ast_গ্রন্থি_রাশিমালা* anyBinMatchp = nullptr;
        প্রতি (Ast_গ্রন্থি* binp = coverpointp->binsp(); binp; binp = binp->nextp()) {
            AstCoverBin* ধ্রুব cbinp = VN_AS(binp, CoverBin);
            যদি (cbinp->binsType() == VCoverBinsType::BINS_DEFAULT
                || cbinp->binsType() == VCoverBinsType::BINS_IGNORE
                || cbinp->binsType() == VCoverBinsType::BINS_ILLEGAL)
                অনুবর্ত;
            // The values of constructed bins are known at construction; see emitSizedSample
            যদি (isConstructedBins(cbinp)) অনুবর্ত;
            যদি (isAutoBins(cbinp)) {
                // Automatic bins partition the whole domain, leaving no default value
                যদি (anyBinMatchp) VL_DO_DANGLING(ঠেলো_Deletep(anyBinMatchp), anyBinMatchp);
                প্রত্যাগম নূতন AstConst{fl, AstConst::BitFalse{}};
            }
            Ast_গ্রন্থি_রাশিমালা* ধ্রুব binCondp = buildBinCondition(cbinp, exprp);
            UASSERT_OBJ(binCondp, cbinp,
                        "buildBinCondition returned nullptr for non-ignore/non-illegal bin");
            anyBinMatchp = anyBinMatchp ? নূতন AstOr{fl, anyBinMatchp, binCondp} : binCondp;
        }
        প্রত্যাগম anyBinMatchp ? static_cast<Ast_গ্রন্থি_রাশিমালা*>(নূতন AstNot{fl, anyBinMatchp})
                            : static_cast<Ast_গ্রন্থি_রাশিমালা*>(নূতন AstConst{fl, AstConst::BitTrue{}});
    }

    //====================================================================
    // VlCoverpoint conversion

    স্থির তার্কিক coverpointHasStateExclusions(ধ্রুব AstCoverpoint* coverpointp) {
        প্রতি (ধ্রুব Ast_গ্রন্থি* nodep = coverpointp->binsp(); nodep; nodep = nodep->nextp()) {
            ধ্রুব AstCoverBin* ধ্রুব binp = VN_AS(nodep, CoverBin);
            যদি (!binp->transp() && binp->rangesp()
                && (binp->binsType() == VCoverBinsType::BINS_IGNORE
                    || binp->binsType() == VCoverBinsType::BINS_ILLEGAL)) {
                প্রত্যাগম সত্য;
            }
        }
        প্রত্যাগম অসত্য;
    }

    স্থির তার্কিক coverpointHasConstructedBins(ধ্রুব AstCoverpoint* coverpointp) {
        প্রতি (ধ্রুব Ast_গ্রন্থি* nodep = coverpointp->binsp(); nodep; nodep = nodep->nextp()) {
            যদি (isConstructedBins(VN_AS(nodep, CoverBin))) প্রত্যাগম সত্য;
        }
        প্রত্যাগম অসত্য;
    }

    // True if a Normal state bin, or an array-bin element, has no value of the coverpoint's
    // type (IEEE 1800-2023 19.5.7).  Array ranges enumerate in-type values, so cannot vanish.
    স্থির তার্কিক coverpointHasEmptyBins(ধ্রুব AstCoverpoint* coverpointp) {
        Ast_গ্রন্থি_রাশিমালা* ধ্রুব exprp = coverpointp->exprp();
        প্রতি (Ast_গ্রন্থি* nodep = coverpointp->binsp(); nodep; nodep = nodep->nextp()) {
            ধ্রুব AstCoverBin* ধ্রুব binp = VN_AS(nodep, CoverBin);
            যদি (!binp->binsType().binIsNormal() || binp->transp() || !binp->rangesp()) অনুবর্ত;
            // A wildcard array has a bin for each value its elements match, so none empty, and
            // the constructor creates no bin without values of a 'with' filter
            যদি ((binp->isArray() && binp->isWildcard()) || binWith(binp)) অনুবর্ত;
            তার্কিক empty = সত্য;
            প্রতি (Ast_গ্রন্থি* valuep = binp->rangesp(); valuep; valuep = valuep->nextp()) {
                যদি (binp->isArray() && VN_IS(valuep, InsideRange)) অনুবর্ত;
                Cross_মান_Range range{valuep, resolveWidth(valuep, exprp)};
                ধ্রুব তার্কিক none = resolve_মান(valuep, exprp, সত্য, binp->isWildcard(), range)
                                  && crossRangeEmpty(range);
                যদি (binp->isArray() && none) প্রত্যাগম সত্য;
                empty &= none;
            }
            যদি (empty && !binp->isArray()) প্রত্যাগম সত্য;
        }
        প্রত্যাগম অসত্য;
    }

    // True if a coverpoint has any transition bin.  Used to decide whether sample() emits the
    // end-of-sample previous-value update that transition matching needs.
    স্থির তার্কিক coverpointHasTransition(AstCoverpoint* coverpointp) {
        প্রতি (Ast_গ্রন্থি* binp = coverpointp->binsp(); binp; binp = binp->nextp()) {
            যদি (VN_AS(binp, CoverBin)->transp()) প্রত্যাগম সত্য;
        }
        প্রত্যাগম অসত্য;
    }

    // The interned AstBasicDType for one of the covergroup runtime keywords.
    স্থির AstBasicDType* basicDType(File_সারি* fl, VBasicDTypeKwd kwd) {
        প্রত্যাগম v3Global.rootp()->typeTablep()->findBasicDType(fl, kwd);
    }

    // Get (or create) the coverpoint dtype for a hit-list bound, interned so each distinct bound
    // yields one node.
    AstCoverpointDType* coverpointDType(File_সারি* fl, uint32_t hitBound) {
        AstCoverpointDType*& typep = m_cpDTypes[hitBound];
        যদি (!typep) {
            typep = নূতন AstCoverpointDType{fl, hitBound};
            v3Global.rootp()->typeTablep()->addTypesp(typep);
        }
        প্রত্যাগম typep;
    }

    // The key of the covergroup's type in the coverage registry, which get_coverage() queries
    std::string covergroupProtectedName() ধ্রুব {
        প্রত্যাগম VIdProtect::protect_শব্দগুলি_If(m_covergroupName, v3Global.opt.protectIds());
    }

    // The name of the covergroup's type, which keys the coverage registry and database: as
    // $typename names it (IEEE 1800-2023 20.6.1), so apart for covergroups of distinct scopes and
    // specializations, and alike in each Verilator run of a hierarchical design.  As design units
    // of distinct libraries may share a name, one of a library other than the default is prefixed
    // by its library, as '%l' prints it (IEEE 1800-2023 33.4).
    std::string covergroupTypeName() ধ্রুব {
        UASSERT_OBJ(m_unitp, m_covergroupp, "Covergroup declared outside of a design unit");
        ধ্রুব std::string& libname = m_unitp->libname();
        ধ্রুব std::string name = m_covergroupp->dtypeName(সত্য);
        প্রত্যাগম libname == "work" ? name : libname + "." + name;
    }

    // Emit the covergroup's instance handle member and the constructor statement that creates
    // its node in the per-context coverage registry.  Runs before any coverpoint or cross is
    // generated, so their runtimes can be added to the node as they are created.
    শূন্য generateInstanceAttach() {
        File_সারি* ধ্রুব fl = m_covergroupp->fileline();
        // V3LinkParse synthesizes a 'new' for every covergroup; the item generators below already
        // rely on that, and this attach runs even for a covergroup with no coverpoints at all.
        UASSERT_OBJ(m_constructorp, m_covergroupp, "Covergroup missing synthesized constructor");
        m_cgInstVarp = নূতন AstVar{fl, VVarType::MEMBER, "__Vcg_inst",
                                  basicDType(fl, VBasicDTypeKwd::COVERGROUP_INSTHANDLE)};
        m_covergroupp->addMembersp(m_cgInstVarp);

        m_constructorp->addStmtsp(
            itemCall(fl, m_cgInstVarp, VCMethod::COVERGROUP_ATTACH,
                     {ctext(fl, "vlSymsp->_vm_contextp__->covergroupRegistryp()"
                                "->newCovergroupInst("
                                    + quoted(covergroupProtectedName())
                                    + (m_cgMayMerge ? ", true" : ", false") + ")")},
                     /*usePtr=*/অসত্য)
                ->makeStmt());
        // The node reads option.weight in place, so procedural assignments take effect
        AstCExpr* ধ্রুব weightAddrp = নূতন AstCExpr{fl, "&"};
        weightAddrp->add(newOptionSel(fl, optionVar(অসত্য), "weight", VAccess::READ));
        m_constructorp->addStmtsp(itemCall(fl, m_cgInstVarp, VCMethod::COVERGROUP_LEND_WEIGHT,
                                           {weightAddrp, file_সারি_Debug(fl)}, /*usePtr=*/অসত্য)
                                      ->makeStmt());
    }

    // A '__Vcg_inst.p()-><method>()' call on the covergroup's instance node
    AstCMethodHard* instanceCall(File_সারি* fl, VCMethod method) {
        // '__Vcg_inst.p()' -- a value handle, so '.' not '->'
        AstCMethodHard* ধ্রুব instp
            = নূতন AstCMethodHard{fl, memberRef(fl, m_cgInstVarp), VCMethod::COVERGROUP_INST_P};
        instp->usePtr(অসত্য);
        instp->dtypeSetVoid();  // Opaque receiver; only ever the 'fromp' of the call below
        AstCMethodHard* ধ্রুব callp = নূতন AstCMethodHard{fl, instp, method};
        callp->usePtr(সত্য);
        প্রত্যাগম callp;
    }

    // Emit 'this->__Vcp_x = this->__Vcg_inst.p()->addCoverpoint<K>();' (or addCross), which
    // creates the item runtime in the instance node and borrows a pointer to it.
    AstAssign* makeItemCreate(File_সারি* fl, AstVar* itemVarp, VCMethod method) {
        AstCMethodHard* ধ্রুব createp = instanceCall(fl, method);
        createp->dtypep(itemVarp->dtypep());
        প্রত্যাগম নূতন AstAssign{fl, memberRef(fl, itemVarp, VAccess::WRITE), createp};
    }

    // Constant bounds of one rangesp() element (an InsideRange or a single Const).  Each bound is
    // the raw AST node -- an AstConst or an AstUnbounded ('$') -- with the const/unbounded view
    // derived on demand, so there is one source of truth per bound.  After a successful
    // constRangeBounds() neither node is null; a single Const has both bounds aliasing one node.
    সংরচনা RangeBounds final {
        Ast_গ্রন্থি* loNodep = nullptr;  // low bound: AstConst or AstUnbounded
        Ast_গ্রন্থি* hiNodep = nullptr;  // high bound: AstConst or AstUnbounded
        তার্কিক loUnbounded() ধ্রুব { প্রত্যাগম VN_IS(loNodep, Unbounded); }
        তার্কিক hiUnbounded() ধ্রুব { প্রত্যাগম VN_IS(hiNodep, Unbounded); }
        AstConst* loConstp() ধ্রুব { প্রত্যাগম VN_CAST(loNodep, Const); }
        AstConst* hiConstp() ধ্রুব { প্রত্যাগম VN_CAST(hiNodep, Const); }
    };

    // Decode one rangesp() element into its constant bounds.  Returns false if rp is neither an
    // InsideRange nor a single Const, or if a present bound is non-constant or 4-state.  '$'
    // bounds are left as AstUnbounded (not resolved) -- the caller applies its own policy.
    // Centralizes the InsideRange/Const/Unbounded decode shared by the hit-list-bound paths.
    স্থির তার্কিক constRangeBounds(Ast_গ্রন্থি* rp, RangeBounds& rb) {
        যদি (AstInsideRange* ধ্রুব irp = VN_CAST(rp, InsideRange)) {
            rb.loNodep = irp->lhsp();
            rb.hiNodep = irp->rhsp();
        } অন্যথা যদি (AstConst* ধ্রুব cp = VN_CAST(rp, Const)) {
            rb.loNodep = rb.hiNodep = cp;
        } অন্যথা {
            প্রত্যাগম অসত্য;
        }
        // Each bound must be a constant unless it is '$'; reject non-const and 4-state.
        AstConst* ধ্রুব lc = rb.loConstp();
        AstConst* ধ্রুব hc = rb.hiConstp();
        যদি ((!lc && !rb.loUnbounded()) || (!hc && !rb.hiUnbounded())) প্রত্যাগম অসত্য;
        যদি ((lc && lc->অঙ্ক().isFourState()) || (hc && hc->অঙ্ক().isFourState())) প্রত্যাগম অসত্য;
        প্রত্যাগম সত্য;
    }

    // Collect the covered value intervals of a single (non-array) Normal bin.  Returns false
    // if any range isn't a constant/open InsideRange or single Const (e.g. wildcard, non-const).
    স্থির তার্কিক extractRangeIntervals(AstCoverBin* cbinp, uint64_t maxVal,
                                      std::vector<std::pair<uint64_t, uint64_t>>& out) {
        যদি (!cbinp->rangesp()) প্রত্যাগম অসত্য;
        প্রতি (Ast_গ্রন্থি* rp = cbinp->rangesp(); rp; rp = rp->nextp()) {
            RangeBounds rb;
            যদি (!constRangeBounds(rp, rb)) প্রত্যাগম অসত্য;
            যদি ((rb.loConstp() && rb.loConstp()->width() > 64)
                || (rb.hiConstp() && rb.hiConstp()->width() > 64)) {
                প্রত্যাগম অসত্য;  // Use the safe slot-count bound for wide values.
            }
            ধ্রুব uint64_t lo = rb.loUnbounded() ? 0 : rb.loConstp()->toUQuad();
            ধ্রুব uint64_t hi = rb.hiUnbounded() ? maxVal : rb.hiConstp()->toUQuad();
            যদি (lo > hi) প্রত্যাগম অসত্য;
            out.emplace_back(lo, hi);
        }
        প্রত্যাগম সত্য;
    }

    // Append one Normal bin's cross-slot interval-sets to `bins` and bump `slotCount` by an upper
    // bound of the bin's slots holding one value.  Returns false if any part isn't statically
    // enumerable (the caller then falls back to the always-safe slot count).  A non-array bin
    // is one slot covering the union of its intervals; the bins of an array element or of an
    // automatic bins declaration hold disjoint values, so the element or declaration counts once.
    তার্কিক appendBinCrossSlots(AstCoverBin* cbinp, uint64_t maxVal, Ast_গ্রন্থি_রাশিমালা* exprp,
                             std::vector<std::vector<std::pair<uint64_t, uint64_t>>>& bins,
                             পূর্ণাঙ্ক& slot_গুনতি) {
        যদি (isAutoBins(cbinp)) {
            ++slot_গুনতি;
            bins.ঠেলো_back({{0, maxVal}});
            প্রত্যাগম exprp->width() <= 64;
        }
        যদি (binWith(cbinp)) {
            // A value is in one bin of a filter's, but of a sized array, in one for each range
            // list element holding it; the coverpoint's name is one element
            পূর্ণাঙ্ক elements = 0;
            প্রতি (ধ্রুব Ast_গ্রন্থি* rp = binRangesp(cbinp); rp && isSizedArray(cbinp);
                 rp = rp->nextp()) {
                ++elements;
            }
            slot_গুনতি += std::max(1, elements);
            প্রত্যাগম অসত্য;
        }
        যদি (cbinp->isArray() && cbinp->isWildcard() && !cbinp->arraySizep()) {
            // A value is in at most one bin of a wildcard array: one slot covering its values.
            // Signed values are sign-extended, not unsigned intervals (see computeHitListBound)
            ++slot_গুনতি;
            যদি (exprp->isSigned() || exprp->width() > 64) প্রত্যাগম অসত্য;
            ধ্রুব BinRuns runs = wildcardBinRuns(cbinp, exprp, অসত্য);
            যদি (runs.unsupported) প্রত্যাগম অসত্য;
            std::vector<std::pair<uint64_t, uint64_t>> ivs;
            প্রতি (ধ্রুব BinRun& run : runs.runs) {
                ivs.emplace_back(run.m_lo.toUQuad(), run.m_hi.toUQuad());
            }
            bins.ঠেলো_back(std::move(ivs));
            প্রত্যাগম সত্য;
        }
        যদি (cbinp->isArray()) প্রত্যাগম appendArrayBinCrossSlots(cbinp, exprp, bins, slot_গুনতি);
        // Non-array bin: one slot covering the union of its intervals.
        ++slot_গুনতি;
        std::vector<std::pair<uint64_t, uint64_t>> ivs;
        যদি (cbinp->isWildcard() || !extractRangeIntervals(cbinp, maxVal, ivs)) প্রত্যাগম অসত্য;
        bins.ঠেলো_back(std::move(ivs));
        প্রত্যাগম সত্য;
    }

    // Append the cross slots of an array Normal bin: each element is a slot covering its
    // values, as at most one of its single-value bins holds a value.  An element holds the
    // values arrayBinRuns() gives it: those of the coverpoint type (IEEE 1800-2023 19.5.7).
    // Elements of a signed coverpoint, non-constant elements, and elements with values beyond
    // 64 bits can't be enumerated, so they count one slot but lose exactness.  Returns false if
    // any element wasn't enumerable to exact values.
    তার্কিক appendArrayBinCrossSlots(AstCoverBin* cbinp, Ast_গ্রন্থি_রাশিমালা* exprp,
                                  std::vector<std::vector<std::pair<uint64_t, uint64_t>>>& bins,
                                  পূর্ণাঙ্ক& slot_গুনতি) {
        // Signed values resolve sign-extended, not as unsigned intervals, and wildcard patterns
        // are not intervals
        তার্কিক exact = !exprp->isSigned() && !cbinp->isWildcard();
        প্রতি (Ast_গ্রন্থি* rp = cbinp->rangesp(); rp; rp = rp->nextp()) {
            ++slot_গুনতি;
            RangeBounds rb;
            Cross_মান_Range range{rp, resolveWidth(rp, exprp)};
            যদি (!exact || !constRangeBounds(rp, rb)
                || !resolve_মান(rp, exprp, সত্য, অসত্য, range)) {
                exact = অসত্য;
                অনুবর্ত;
            }
            যদি (crossRangeEmpty(range)) অনুবর্ত;  // Its bin, if any, holds no value
            যদি (range.hi.mostSetBitP1() > 64) {
                exact = অসত্য;
                অনুবর্ত;
            }
            bins.ঠেলো_back({{range.lo.toUQuad(), range.hi.toUQuad()}});
        }
        প্রত্যাগম exact;
    }

    // Compute the hit-list bound for a coverpoint: the maximum number of Normal
    // bins one sample value can match.  Non-cross-fed coverpoints don't feed a cross, so
    // their hit list is unused -> 1.  Otherwise compute the exact max bin overlap; fall back
    // to the (always-safe) Normal-slot count when any bin isn't statically analyzable.
    পূর্ণাঙ্ক computeHitListBound(AstCoverpoint* coverpointp, Ast_গ্রন্থি_রাশিমালা* exprp, তার্কিক crossFed) {
        যদি (!crossFed) প্রত্যাগম 1;
        ধ্রুব পূর্ণাঙ্ক width = exprp->width();
        ধ্রুব uint64_t maxVal = (width >= 64) ? UINT64_MAX : ((1ULL << width) - 1);
        // One entry per Normal bin (cross slot): its covered intervals.
        std::vector<std::vector<std::pair<uint64_t, uint64_t>>> bins;
        পূর্ণাঙ্ক slot_গুনতি = 0;  // == runtime m_normal; the safe fallback bound
        // Unsigned intervals cannot establish overlap between differently sized signed values.
        তার্কিক exact = !exprp->isSigned();
        প্রতি (Ast_গ্রন্থি* binp = coverpointp->binsp(); binp; binp = binp->nextp()) {
            AstCoverBin* ধ্রুব cbinp = VN_AS(binp, CoverBin);
            যদি (!cbinp->binsType().binIsNormal())
                অনুবর্ত;  // ignore/illegal/default: not hit-listed
            যদি (!appendBinCrossSlots(cbinp, maxVal, exprp, bins, slot_গুনতি)) exact = অসত্য;
        }
        যদি (!exact) প্রত্যাগম std::max(1, slot_গুনতি);
        যদি (bins.empty()) প্রত্যাগম 1;
        // Max overlap occurs at some interval start; count covering bins at each lo.
        std::vector<uint64_t> pts;
        প্রতি (ধ্রুব auto& b : bins)
            প্রতি (ধ্রুব auto& iv : b) pts.ঠেলো_back(iv.first);
        std::sort(pts.begin(), pts.end());
        pts.erase(std::unique(pts.begin(), pts.end()), pts.end());
        পূর্ণাঙ্ক maxOverlap = 1;
        প্রতি (ধ্রুব uint64_t p : pts) {
            পূর্ণাঙ্ক cnt = 0;
            প্রতি (ধ্রুব auto& b : bins) {
                প্রতি (ধ্রুব auto& iv : b) {
                    যদি (iv.first <= p && p <= iv.second) {
                        ++cnt;
                        ভঙ্গ;  // count each bin at most once
                    }
                }
            }
            যদি (cnt > maxOverlap) maxOverlap = cnt;
        }
        প্রত্যাগম maxOverlap;
    }

    // A 'this->m_member' reference for embedding in an AstCStmt
    AstVarRef* memberRef(File_সারি* fl, AstVar* varp, VAccess access = VAccess::READ) {
        AstVarRef* ধ্রুব refp = নূতন AstVarRef{fl, varp, access};
        refp->selfPointer(VSelfPointerText{VSelfPointerText::This{}});
        প্রত্যাগম refp;
    }

    // A 'this->m_member-><method>(args...)' call on one member.  usePtr is false only for
    // __Vcg_inst, which is a value handle; the item members are borrowed pointers into the
    // instance node.  Numeric arguments are AstConst; the rest are C++ text that has no AST
    // form (see ctext).
    AstCMethodHard* itemCall(File_সারি* fl, AstVar* varp, VCMethod method,
                             ধ্রুব std::vector<Ast_গ্রন্থি_রাশিমালা*>& args = {}, তার্কিক usePtr = সত্য) {
        AstCMethodHard* ধ্রুব callp = নূতন AstCMethodHard{fl, memberRef(fl, varp), method};
        প্রতি (Ast_গ্রন্থি_রাশিমালা* ধ্রুব argp : args) callp->addPinsp(argp);
        callp->usePtr(usePtr);
        callp->dtypeSetVoid();
        প্রত্যাগম callp;
    }

    // An unsigned integer argument.
    স্থির AstConst* cnum(File_সারি* fl, uint32_t মান) { প্রত্যাগম নূতন AstConst{fl, মান}; }

    // A literal C++ argument with no AST equivalent: a 'const char*' string literal (an SV
    // string AstConst emits '"..."s', a std::string temporary the runtime cannot borrow), a
    // VlCovBinKind enum token, a constant selection-word initializer list, a VlFileLineDebug, or
    // a '__V' temporary declared by the enclosing AstCStmt.
    স্থির AstCExpr* ctext(File_সারি* fl, ধ্রুব std::string& text) {
        প্রত্যাগম নূতন AstCExpr{fl, text};
    }

    // A C++ string literal.  Escapes control characters as the emitter does elsewhere -- bin
    // names and filenames reach the generated code verbatim when --protect-ids is off, and an
    // SV escaped identifier may hold a quote or backslash.
    স্থির std::string quoted(ধ্রুব std::string& text) {
        প্রত্যাগম "\"" + V3OutFormatter::quoteNameControls(text) + "\"";
    }

    // A 'VlFileLineDebug' argument: where the runtime reports an error about fl's construct
    স্থির AstCExpr* file_সারি_Debug(File_সারি* fl) {
        ধ্রুব std::string filename
            = VIdProtect::protectIf(fl->filename(), v3Global.opt.protectIds());
        প্রত্যাগম ctext(fl, "VlFileLineDebug{" + quoted(filename) + ", "
                             + std::to_string(fl->lineno()) + "}");
    }

    // Check that an element of an array bin (bins b[] = {values/ranges}) is a two-state
    // constant value or range; false if not, after reporting it if 'report'.
    স্থির তার্কিক checkArrayBinElement(AstCoverBin* arrayBinp, Ast_গ্রন্থি* rangep, তার্কিক report = সত্য) {
        যদি (ধ্রুব AstInsideRange* ধ্রুব irp = VN_CAST(rangep, InsideRange)) {
            ধ্রুব AstConst* ধ্রুব minp = VN_CAST(irp->lhsp(), Const);
            ধ্রুব AstConst* ধ্রুব maxp = VN_CAST(irp->rhsp(), Const);
            যদি ((!minp && !VN_IS(irp->lhsp(), Unbounded))
                || (!maxp && !VN_IS(irp->rhsp(), Unbounded))) {
                যদি (report) {
                    arrayBinp->v3error("Non-constant expression in array bins range; "
                                       "range bounds must be constants (IEEE 1800-2023 19.5)");
                }
                প্রত্যাগম অসত্য;
            }
            যদি ((minp && minp->অঙ্ক().isFourState()) || (maxp && maxp->অঙ্ক().isFourState())) {
                যদি (report) {
                    arrayBinp->v3error("Four-state (x/z) value in array bins range bound; "
                                       "range bounds must be two-state constants");
                }
                প্রত্যাগম অসত্য;
            }
        } অন্যথা যদি (!VN_IS(rangep, Const)) {
            যদি (report) {
                arrayBinp->v3error("Non-constant expression in array bins value list; "
                                   "values must be constants (IEEE 1800-2023 19.5)");
            }
            প্রত্যাগম অসত্য;
        }
        প্রত্যাগম সত্য;
    }

    // Individual equality targets of an array bin (bins b[] = {values/ranges}) of a real
    // coverpoint, in order; integral coverpoints generate array bins as runs (see arrayBinRuns).
    // An open-ended bound ('$', AstUnbounded) resolves to the coverpoint domain: '[lo:$]'
    // covers [lo:maxVal] and '[$:hi]' covers [0:hi].  One target is produced per value; ranges
    // whose resolved size would exceed --coverage-max-real-bins (e.g. an open '[lo:$]') are
    // unsupported -- emits COVERIGN, sets unsupportedOut, yields nothing.
    std::vector<Ast_গ্রন্থি_রাশিমালা*> extract_Array_মানসমূহ(AstCoverBin* arrayBinp, Ast_গ্রন্থি_রাশিমালা* exprp,
                                                 তার্কিক& unsupportedOut) {
        unsupportedOut = অসত্য;
        ধ্রুব পূর্ণাঙ্ক width = exprp->width();
        ধ্রুব uint64_t maxVal = (width >= 64) ? UINT64_MAX : ((1ULL << width) - 1);
        std::vector<Ast_গ্রন্থি_রাশিমালা*> মানসমূহ;
        প্রতি (Ast_গ্রন্থি* rangep = arrayBinp->rangesp(); rangep; rangep = rangep->nextp()) {
            rangep = V3Const::constifyEdit(rangep);
            যদি (!checkArrayBinElement(arrayBinp, rangep)) প্রত্যাগম মানসমূহ;
            যদি (AstInsideRange* ধ্রুব irp = VN_CAST(rangep, InsideRange)) {
                ধ্রুব তার্কিক loUnb = VN_IS(irp->lhsp(), Unbounded);
                ধ্রুব তার্কিক hiUnb = VN_IS(irp->rhsp(), Unbounded);
                ধ্রুব uint64_t lo = loUnb ? 0 : VN_AS(irp->lhsp(), Const)->toUQuad();
                ধ্রুব uint64_t hi = hiUnb ? maxVal : VN_AS(irp->rhsp(), Const)->toUQuad();
                যদি (hi < lo) অনুবর্ত;  // empty range contributes no bins
                // Guard against a '$'-bounded or otherwise huge range exploding the bin count.
                ধ্রুব uint64_t span = hi - lo;  // == valueCount - 1 (no overflow: hi >= lo)
                যদি (span >= v3Global.opt.coverageMaxRealBins()
                    || মানসমূহ.size() + span + 1 > v3Global.opt.coverageMaxRealBins()) {
                    arrayBinp->v3warn(COVERIGN,
                                      "Unsupported: array 'bins' of a real coverpoint "
                                      "covering more than "
                                          << v3Global.opt.coverageMaxRealBins() << " values; bin "
                                          << arrayBinp->prettyNameQ() << " ignored.\n"
                                          << arrayBinp->warnMore()
                                          << "... Suggest a larger --coverage-max-real-bins");
                    unsupportedOut = সত্য;
                    প্রতি (Ast_গ্রন্থি_রাশিমালা* ধ্রুব vp : মানসমূহ) VL_DO_DANGLING(ঠেলো_Deletep(vp), vp);
                    মানসমূহ.clear();
                    প্রত্যাগম মানসমূহ;
                }
                প্রতি (uint64_t v = lo; v <= hi; ++v)
                    মানসমূহ.ঠেলো_back(নূতন AstConst{irp->fileline(), AstConst::Widthed_মান{}, width,
                                                  static_cast<uint32_t>(v)});
            } অন্যথা {
                মানসমূহ.ঠেলো_back(VN_AS(rangep->clone_বৃক্ষ(অসত্য), গ্রন্থি_রাশিমালা));
            }
        }
        প্রত্যাগম মানসমূহ;
    }

    স্থির পূর্ণাঙ্ক runWidth(ধ্রুব Ast_গ্রন্থি_রাশিমালা* exprp) { প্রত্যাগম exprp->width() + 1; }

    // Automatic bins partition the coverpoint domain in value order (IEEE 1800-2023 19.5.3): N
    // bins, capped at the number of values, each hold 2^width / N values, and the last bin also
    // holds the remainder.  False for an invalid declaration, already reported.
    তার্কিক autoBinRuns(AstCoverBin* binp, Ast_গ্রন্থি_রাশিমালা* exprp, BinRuns& out) {
        ধ্রুব uint32_t requested = autoBinsRequested(binp);
        যদি (!requested || !exprp->dtypep()->skipRefp()->isIntegralOrPacked()) প্রত্যাগম অসত্য;
        ধ্রুব পূর্ণাঙ্ক width = exprp->width();
        ধ্রুব পূর্ণাঙ্ক arithmeticWidth = runWidth(exprp);
        ধ্রুব uint32_t গুনতি
            = width < 32
                  ? static_cast<uint32_t>(std::min<uint64_t>(uint64_t{1} << width, requested))
                  : requested;
        BinRun run{binp, arithmeticWidth, গুনতি};
        V3_সংখ্যা total{binp, arithmeticWidth};
        total.setBit(width, 1);
        run.m_stride.opDiv(total, V3_সংখ্যা{binp, arithmeticWidth, গুনতি});
        ধ্রুব Cross_মান_Range domain
            = cross_মান_Domain(binp, width, exprp->isSigned(), arithmeticWidth);
        run.m_lo = domain.lo;
        run.m_hi = domain.hi;
        out.runs.ঠেলো_back(std::move(run));
        out.গুনতি = গুনতি;
        প্রত্যাগম সত্য;
    }

    // The elements of an array bin (bins b[] = {values/ranges}), in order, as runs of
    // single-value bins.  A range holds the values of the coverpoint type it contains (IEEE
    // 1800-2023 19.5.7), while a singleton names a bin even without such a value.  Errors on a
    // non-constant element.  More than --coverage-max-bins bins (e.g. an open '[lo:$]' range over
    // a wide coverpoint) are unsupported -- emits COVERIGN, and sets unsupported.
    BinRuns arrayBinRuns(AstCoverBin* arrayBinp, Ast_গ্রন্থি_রাশিমালা* exprp) {
        BinRuns out;
        ধ্রুব পূর্ণাঙ্ক width = runWidth(exprp);
        প্রতি (Ast_গ্রন্থি* rangep = arrayBinp->rangesp(); rangep; rangep = rangep->nextp()) {
            rangep = V3Const::constifyEdit(rangep);
            যদি (!checkArrayBinElement(arrayBinp, rangep)) প্রত্যাগম out;
            ধ্রুব AstInsideRange* ধ্রুব irp = VN_CAST(rangep, InsideRange);
            Cross_মান_Range range{rangep, resolveWidth(rangep, exprp)};
            তার্কিক empty = সত্য;
            যদি (!resolve_মান(rangep, exprp, সত্য, অসত্য, range)) {
                rangep->v3warn(E_UNSUPPORTED, "Unsupported: non-integral value in a coverage bin "
                                              "of an integral coverpoint.");
            } অন্যথা {
                empty = crossRangeEmpty(range);
            }
            যদি (empty && irp) অনুবর্ত;  // A range without values contributes no bins
            BinRun run{rangep, width, 1};
            run.m_empty = empty;
            uint64_t গুনতি = 1;
            যদি (!empty) {
                run.m_lo.opAssign(range.lo);
                run.m_hi.opAssign(range.hi);
                V3_সংখ্যা span{rangep, width};
                span.opSub(run.m_hi, run.m_lo);
                // Wider spans exceed any limit
                গুনতি = span.mostSetBitP1() > 32 ? UINT64_MAX : span.toUQuad() + 1;
            }
            যদি (গুনতি > v3Global.opt.coverageMaxBins() - out.গুনতি) {
                arrayBinp->v3warn(COVERIGN, "Unsupported: array 'bins' covering more than "
                                                << v3Global.opt.coverageMaxBins()
                                                << " values (e.g. an open '[lo:$]' range over "
                                                   "a wide coverpoint); bin "
                                                << arrayBinp->prettyNameQ() << " ignored\n"
                                                << arrayBinp->warnMore()
                                                << "... Suggest a larger --coverage-max-bins");
                out.runs.clear();
                out.গুনতি = 0;
                out.unsupported = সত্য;
                প্রত্যাগম out;
            }
            run.m_গুনতি = static_cast<uint32_t>(গুনতি);
            out.গুনতি += run.m_গুনতি;
            out.runs.ঠেলো_back(std::move(run));
        }
        প্রত্যাগম out;
    }

    // The bins of a wildcard array (wildcard bins b[] = {...}): one for each coverpoint value
    // an element matches (IEEE 1800-2023 19.5.4, 19.5.7), in value order, and named by the value
    // (19.5.1), as runs of single-value bins.  Errors on a non-constant element.  More than
    // --coverage-max-bins values are unsupported -- emits COVERIGN, and sets unsupported, and for
    // an ignore or illegal array, single.  'report' false omits these diagnostics.
    স্থির BinRuns wildcardBinRuns(AstCoverBin* arrayBinp, Ast_গ্রন্থি_রাশিমালা* exprp, তার্কিক report) {
        BinRuns out;
        ধ্রুব পূর্ণাঙ্ক width = runWidth(exprp);
        ধ্রুব V3_সংখ্যা one{arrayBinp, width, 1};
        // Disjoint runs of the values, in value order, each value once
        std::vector<std::pair<V3_সংখ্যা, V3_সংখ্যা>> spans;
        uint64_t গুনতি = 0;  // Values of 'spans'
        প্রতি (Ast_গ্রন্থি* rangep = arrayBinp->rangesp(); rangep; rangep = rangep->nextp()) {
            rangep = V3Const::constifyEdit(rangep);
            যদি (!checkArrayBinElement(arrayBinp, rangep, report)) {
                out.unsupported = সত্য;
                প্রত্যাগম out;
            }
            Cross_মান_Range range{rangep, resolveWidth(rangep, exprp)};
            যদি (!resolve_মান(rangep, exprp, সত্য, সত্য, range)) {
                যদি (report) {
                    rangep->v3warn(E_UNSUPPORTED, "Unsupported: non-integral value in a "
                                                  "coverage bin of an integral coverpoint.");
                }
                অনুবর্ত;
            }
            যদি (crossRangeEmpty(range)) অনুবর্ত;
            std::vector<std::pair<V3_সংখ্যা, V3_সংখ্যা>> found;
            crossRangeRuns(range, v3Global.opt.coverageMaxBins(), found);
            প্রতি (ধ্রুব std::pair<V3_সংখ্যা, V3_সংখ্যা>& run : found) {
                // Coverpoint values, sign-extended in both widths
                spans.emplace_back(V3_সংখ্যা{rangep, width, run.first},
                                   V3_সংখ্যা{rangep, width, run.second});
            }
            std::sort(spans.begin(), spans.end(), [](ধ্রুব auto& lhs, ধ্রুব auto& rhs) {
                প্রত্যাগম cross_মান_Less(lhs.first, rhs.first);
            });
            std::vector<std::pair<V3_সংখ্যা, V3_সংখ্যা>> merged;
            প্রতি (ধ্রুব std::pair<V3_সংখ্যা, V3_সংখ্যা>& span : spans) {
                // Adjacent or overlapping values join a run; 'first - 1' cannot overflow
                V3_সংখ্যা before{rangep, width};
                before.opSub(span.first, one);
                যদি (merged.empty() || cross_মান_Less(merged.back().second, before)) {
                    merged.ঠেলো_back(span);
                } অন্যথা যদি (cross_মান_Less(merged.back().second, span.second)) {
                    merged.back().second = span.second;
                }
            }
            গুনতি = 0;
            প্রতি (ধ্রুব std::pair<V3_সংখ্যা, V3_সংখ্যা>& span : merged) {
                V3_সংখ্যা size{rangep, width};
                size.opSub(span.second, span.first);
                // Beyond 2^32 values exceed any limit
                গুনতি += size.mostSetBitP1() > 32 ? uint64_t{1} << 33 : size.toUQuad() + 1;
                যদি (গুনতি > v3Global.opt.coverageMaxBins()) ভঙ্গ;
            }
            spans = std::move(merged);
            যদি (গুনতি > v3Global.opt.coverageMaxBins()) {
                // An ignore or illegal array still excludes or checks its values, as one bin
                out.single = !arrayBinp->binsType().binIsNormal();
                যদি (report) {
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
                out.unsupported = সত্য;
                প্রত্যাগম out;
            }
        }
        প্রতি (ধ্রুব std::pair<V3_সংখ্যা, V3_সংখ্যা>& span : spans) {
            out.runs.emplace_back(arrayBinp, width, 0);
            BinRun& run = out.runs.back();
            run.m_lo = span.first;
            run.m_hi = span.second;
            // A bin for each value, named by it in the coverpoint's type
            V3_সংখ্যা মান = span.first;
            যাবত্ (সত্য) {
                ধ্রুব V3_সংখ্যা typed{arrayBinp, exprp->width(), মান};
                out.মানসমূহ.ঠেলো_back(exprp->isSigned() ? typed.toDecimalS() : typed.toDecimalU());
                ++run.m_গুনতি;
                যদি (মান.isCaseEq(span.second)) ভঙ্গ;
                V3_সংখ্যা next{arrayBinp, width};
                মান = next.opAdd(মান, one);
            }
        }
        out.গুনতি = static_cast<uint32_t>(গুনতি);
        প্রত্যাগম out;
    }

    // The runs of an automatic bins declaration, or of an array bin of an integral coverpoint.
    // False for other bins, which do not generate as runs, including a wildcard array then one
    // bin (see BinRuns::single).
    তার্কিক binRunsFor(AstCoverBin* binp, Ast_গ্রন্থি_রাশিমালা* exprp, BinRuns& out) {
        যদি (isAutoBins(binp)) {
            যদি (!autoBinRuns(binp, exprp, out)) out.unsupported = সত্য;
            প্রত্যাগম সত্য;
        }
        যদি (binp->isArray() && binp->isWildcard()) {
            যদি (!exprp->dtypep()->skipRefp()->isIntegralOrPacked()) {
                Ast_গ্রন্থি_রাশিমালা* ধ্রুব falsep = wildcardTypeError(binp, exprp);
                VL_DO_DANGLING(ঠেলো_Deletep(falsep), falsep);
                out.unsupported = সত্য;
            } অন্যথা {
                out = wildcardBinRuns(binp, exprp, সত্য);
                যদি (out.single) {
                    binp->isArray(অসত্য);  // Generates as one bin
                    প্রত্যাগম অসত্য;
                }
            }
            প্রত্যাগম সত্য;
        }
        যদি (!binp->isArray() || binp->transp() || binp->isWildcard()
            || !exprp->dtypep()->skipRefp()->isIntegralOrPacked()) {
            প্রত্যাগম অসত্য;
        }
        out = arrayBinRuns(binp, exprp);
        প্রত্যাগম সত্য;
    }

    // Emit a 'this->m_cp->addSingleNamer/addArrayNamer(...)' statement for one bin whose first
    // runtime bin index is 'declared'; or with 'valueNames', those naming each bin of an array
    Ast_গ্রন্থি_Stmt* makeNamer(AstVar* cpVarp, AstCoverBin* binp, int64_t গুনতি, uint32_t declared,
                           ধ্রুব std::vector<Ast_গ্রন্থি_রাশিমালা*>& মানসমূহ = {},
                           ধ্রুব std::vector<std::string>& মান_Names = {}) {
        File_সারি* ধ্রুব fl = binp->fileline();
        CoverpointBins& bins = m_cpBins.at(cpVarp);
        ধ্রুব uint32_t normal_গুনতি
            = binp->binsType().binIsNormal() ? static_cast<uint32_t>(গুনতি < 0 ? 1 : গুনতি) : 0;
        ধ্রুব BinSpan span{bins.total, normal_গুনতি, declared};
        যদি (binp->binsType() == VCoverBinsType::BINS_AUTO_IMPLICIT) {
            bins.implicitAuto = span;  // Selected by bin, see implicitAutoBinSpan
        } অন্যথা {
            bins.spans.emplace(binp->name(), span);
        }
        bins.total += normal_গুনতি;
        প্রতি (uint32_t i = 0; bins.crossed && i < normal_গুনতি; ++i) {
            bins.মানসমূহ.ঠেলো_back({binp, মানসমূহ.empty() ? nullptr : মানসমূহ[i]});
        }
        // Under --protect-ids the filename and bin name flow into the coverage database
        // verbatim, so obfuscate them exactly as line/toggle coverage points are (whole-
        // unit filename, per-word bin name).  A no-op when --protect-ids is off.
        ধ্রুব তার্কিক prot = v3Global.opt.protectIds();
        ধ্রুব তার্কিক single = গুনতি < 0;
        যদি (!মান_Names.empty()) {
            // A bin of a wildcard array is named by its value (IEEE 1800-2023 19.5.1)
            Ast_গ্রন্থি_Stmt* stmtsp = nullptr;
            প্রতি (ধ্রুব std::string& মান : মান_Names) {
                ধ্রুব std::string name
                    = VIdProtect::protect_শব্দগুলি_If(binp->name(), prot) + "[" + মান + "]";
                stmtsp = Ast_গ্রন্থি::addNext(
                    stmtsp,
                    itemCall(fl, cpVarp, VCMethod::COVERGROUP_ADD_SINGLE_NAMER,
                             {ctext(fl, binp->binsType().binSetEnum()), ctext(fl, quoted(name)),
                              ctext(fl, quoted(VIdProtect::protectIf(fl->filename(), prot))),
                              cnum(fl, static_cast<uint32_t>(fl->lineno())),
                              cnum(fl, static_cast<uint32_t>(fl->firstColumn()))})
                        ->makeStmt());
            }
            প্রত্যাগম stmtsp;
        }
        std::vector<Ast_গ্রন্থি_রাশিমালা*> args{ctext(fl, binp->binsType().binSetEnum())};
        যদি (!single) args.ঠেলো_back(cnum(fl, static_cast<uint32_t>(গুনতি)));  // value array bin
        args.ঠেলো_back(ctext(fl, quoted(VIdProtect::protect_শব্দগুলি_If(binp->name(), prot))));
        args.ঠেলো_back(ctext(fl, quoted(VIdProtect::protectIf(fl->filename(), prot))));
        args.ঠেলো_back(cnum(fl, static_cast<uint32_t>(fl->lineno())));
        args.ঠেলো_back(cnum(fl, static_cast<uint32_t>(fl->firstColumn())));
        প্রত্যাগম itemCall(fl, cpVarp,
                        single ? VCMethod::COVERGROUP_ADD_SINGLE_NAMER
                        : binp->binsType() == VCoverBinsType::BINS_AUTO_IMPLICIT
                            ? VCMethod::COVERGROUP_ADD_NUMBERED_NAMER
                            : VCMethod::COVERGROUP_ADD_ARRAY_NAMER,
                        args)
            ->makeStmt();
    }

    // Emit 'if (iff && cond) m_cp.incrementBin(idx);' (or recordHit, + illegal action) in sample()
    // Where a bin's hit is recorded in the runtime VlCoverpoint member.
    সংরচনা ConvBinTarget final {
        AstVar* cpVarp;  // the __Vcp_<coverpoint> member
        uint32_t idx;  // bin index within that coverpoint
        তার্কিক isNormal;  // Normal -> incrementBin (count + cross hit list); else recordHit (count)
    };

    // Emit 'this->m_cp.incrementBin(idx);' (Normal) or '.recordHit(idx);'
    // (ignore/illegal/default).
    Ast_গ্রন্থি_Stmt* makeRuntimeBinHit(File_সারি* fl, AstVar* cpVarp, Ast_গ্রন্থি_রাশিমালা* idxp,
                                   তার্কিক isNormal) {
        প্রত্যাগম itemCall(fl, cpVarp,
                        isNormal ? VCMethod::COVERGROUP_INCREMENT_BIN
                                 : VCMethod::COVERGROUP_RECORD_HIT,
                        {idxp})
            ->makeStmt();
    }
    Ast_গ্রন্থি_Stmt* makeRuntimeBinHit(File_সারি* fl, ধ্রুব ConvBinTarget& tgt) {
        প্রত্যাগম makeRuntimeBinHit(fl, tgt.cpVarp, cnum(fl, static_cast<uint32_t>(tgt.idx)),
                                 tgt.isNormal);
    }

    // The condition under which a bin counts a sample: condp, and the bin's iff, and for a
    // Normal or default state bin, that the value is not excluded
    Ast_গ্রন্থি_রাশিমালা* binCondition(AstCoverBin* binp, AstVar* cpVarp, Ast_গ্রন্থি_রাশিমালা* condp) {
        File_সারি* ধ্রুব fl = binp->fileline();
        যদি (binp->iffp()) condp = নূতন AstLogAnd{fl, binp->iffp()->clone_বৃক্ষ(অসত্য), condp};
        ধ্রুব auto excluded = m_excludedVars.find(cpVarp);
        যদি (excluded != m_excludedVars.end() && !binp->transp()
            && (binp->binsType().binIsNormal()
                || binp->binsType() == VCoverBinsType::BINS_DEFAULT)) {
            condp = নূতন AstLogAnd{
                fl, নূতন AstNot{fl, নূতন AstVarRef{fl, excluded->second, VAccess::READ}}, condp};
        }
        প্রত্যাগম condp;
    }

    শূন্য emitConvHitIf(AstCoverpoint* coverpointp, AstCoverBin* binp, AstVar* cpVarp,
                       Ast_গ্রন্থি_রাশিমালা* idxp, Ast_গ্রন্থি_রাশিমালা* condp) {
        File_সারি* ধ্রুব fl = binp->fileline();
        Ast_গ্রন্থি* actionp = makeRuntimeBinHit(fl, cpVarp, idxp, binp->binsType().binIsNormal());
        যদি (binp->binsType() == VCoverBinsType::BINS_ILLEGAL) {
            actionp->addNext(makeIllegalBinAction(fl, "Illegal bin " + binp->prettyNameQ()
                                                          + " hit in coverpoint "
                                                          + coverpointp->prettyNameQ()));
        }
        UASSERT_OBJ(m_sampleFuncp, binp, "sample() CFunc not set for coverpoint");
        addSampleStmt(নূতন AstIf{fl, binCondition(binp, cpVarp, condp), actionp, nullptr});
    }

    // Emit the sample() code of sized array 'sized': count the value in its bins holding it when
    // enabled, as other bins are, and for default bins note in matchedp whether any holds it
    শূন্য emitSizedSample(AstCoverpoint* coverpointp, AstCoverBin* binp, AstVar* cpVarp,
                         Ast_গ্রন্থি_রাশিমালা* exprp, uint32_t sized, AstVar* matchedp) {
        File_সারি* ধ্রুব fl = binp->fileline();
        Ast_গ্রন্থি_রাশিমালা* enabledp = binCondition(binp, cpVarp, নূতন AstConst{fl, AstConst::BitTrue{}});
        UASSERT_OBJ(m_sampleFuncp, binp, "sample() CFunc not set for coverpoint");
        ধ্রুব তার্কিক illegal = binp->binsType() == VCoverBinsType::BINS_ILLEGAL;
        যদি (illegal) {
            // The illegal action reads the condition too, which is evaluated once, as the guard
            // may have side effects
            AstVar* ধ্রুব varp
                = নূতন AstVar{fl, VVarType::BLOCKTEMP,
                             "__VcpEnabled_" + sanitizeGeneratedName(coverpointp->name()) + "_"
                                 + cvtToStr(sized),
                             binp->findBitDType()};
            varp->funcLocal(সত্য);
            m_sampleFuncp->addStmtsp(varp);
            addSampleStmt(নূতন AstAssign{fl, নূতন AstVarRef{fl, varp, VAccess::WRITE}, enabledp});
            enabledp = নূতন AstVarRef{fl, varp, VAccess::READ};
        }
        AstCMethodHard* ধ্রুব callp
            = itemCall(fl, cpVarp,
                       exprp->isWide() ? VCMethod::COVERGROUP_SIZED_SAMPLE_W
                                       : VCMethod::COVERGROUP_SIZED_SAMPLE,
                       {cnum(fl, sized), exprp->clone_বৃক্ষ(অসত্য), enabledp});
        callp->dtypeSetBit();
        যদি (illegal) {
            addSampleStmt(নূতন AstIf{fl, নূতন AstLogAnd{fl, callp, enabledp->clone_বৃক্ষ(অসত্য)},
                                    makeIllegalBinAction(fl, "Illegal bin " + binp->prettyNameQ()
                                                                 + " hit in coverpoint "
                                                                 + coverpointp->prettyNameQ())});
        } অন্যথা যদি (matchedp && binp->binsType().binIsNormal()) {
            addSampleStmt(
                নূতন AstAssign{fl, নূতন AstVarRef{fl, matchedp, VAccess::WRITE},
                              নূতন AstOr{fl, নূতন AstVarRef{fl, matchedp, VAccess::READ}, callp}});
        } অন্যথা {
            addSampleStmt(callp->makeStmt());
        }
    }

    // A variable local to the constructor
    AstVar* constructorTemp(File_সারি* fl, ধ্রুব string& name, Ast_গ্রন্থি_DType* dtypep) {
        AstVar* ধ্রুব varp = নূতন AstVar{fl, VVarType::BLOCKTEMP, name, dtypep};
        varp->funcLocal(সত্য);
        m_constructorp->addStmtsp(varp);
        প্রত্যাগম varp;
    }

    // Truncate or extend, as its signedness sets, a value to a type
    স্থির Ast_গ্রন্থি_রাশিমালা* resize_মান(Ast_গ্রন্থি_রাশিমালা* valuep, Ast_গ্রন্থি_DType* dtypep) {
        File_সারি* ধ্রুব fl = valuep->fileline();
        যদি (valuep->width() > dtypep->width()) {
            valuep = নূতন AstSel{fl, valuep, 0, dtypep->width()};
        } অন্যথা যদি (valuep->width() < dtypep->width()) {
            valuep = valuep->isSigned()
                         ? static_cast<Ast_গ্রন্থি_রাশিমালা*>(নূতন AstExtendS{fl, valuep, dtypep->width()})
                         : নূতন AstExtend{fl, valuep, dtypep->width()};
        }
        valuep->dtypep(dtypep);
        প্রত্যাগম valuep;
    }

    // The runtime arguments of a count, which the constructor evaluates once into a temporary:
    // its value, a count wider than 64 bits saturated over any limit, and whether it is positive
    std::pair<Ast_গ্রন্থি_রাশিমালা*, Ast_গ্রন্থি_রাশিমালা*> new_গুনতি_Args(File_সারি* fl, ধ্রুব string& name,
                                                       Ast_গ্রন্থি_রাশিমালা* exprp) {
        AstVar* ধ্রুব countp = constructorTemp(fl, name, exprp->dtypep());
        m_constructorp->addStmtsp(
            নূতন AstAssign{fl, নূতন AstVarRef{fl, countp, VAccess::WRITE}, exprp->clone_বৃক্ষ(অসত্য)});
        ধ্রুব auto গুনতি_Ref = [&]() { প্রত্যাগম নূতন AstVarRef{fl, countp, VAccess::READ}; };
        AstConst* ধ্রুব zerop = নূতন AstConst{fl, AstConst::DTyped{}, countp->dtypep()};
        Ast_গ্রন্থি_রাশিমালা* ধ্রুব positivep
            = countp->isSigned() ? static_cast<Ast_গ্রন্থি_রাশিমালা*>(নূতন AstGtS{fl, গুনতি_Ref(), zerop})
                                 : নূতন AstNeq{fl, গুনতি_Ref(), zerop};
        Ast_গ্রন্থি_রাশিমালা* valuep = resize_মান(গুনতি_Ref(), countp->findUInt64DType());
        যদি (countp->width() > VL_QUADSIZE) {
            valuep = নূতন AstCond{
                fl,
                নূতন AstRedOr{
                    fl, নূতন AstSel{fl, গুনতি_Ref(), VL_QUADSIZE, countp->width() - VL_QUADSIZE}},
                নূতন AstConst{fl, AstConst::Unsized64{}, std::numeric_limits<uint64_t>::max()},
                valuep};
            valuep->dtypeSetUInt64();
        }
        প্রত্যাগম {valuep, positivep};
    }
    // The runtime arguments of a constant count
    স্থির std::pair<Ast_গ্রন্থি_রাশিমালা*, Ast_গ্রন্থি_রাশিমালা*> new_গুনতি_Args(File_সারি* fl, uint64_t গুনতি) {
        প্রত্যাগম {নূতন AstConst{fl, AstConst::Unsized64{}, গুনতি},
                নূতন AstConst{fl, AstConst::BitTrue{}}};
    }

    // The values of a coverpoint, which its name denotes (IEEE 1800-2023 19.5.1.1), as runs in
    // value order: an enumerated type's values (6.19), else all values of its type.  Bounds
    // are at runWidth(), sign-extended like a CrossValueRange's.
    স্থির std::vector<std::pair<V3_সংখ্যা, V3_সংখ্যা>> coverpoint_মানসমূহ(Ast_গ্রন্থি* nodep,
                                                                       Ast_গ্রন্থি_রাশিমালা* exprp) {
        ধ্রুব পূর্ণাঙ্ক width = runWidth(exprp);
        std::vector<std::pair<V3_সংখ্যা, V3_সংখ্যা>> runs;
        ধ্রুব AstEnumDType* ধ্রুব enump = VN_CAST(exprp->dtypep()->skipRefToEnump(), EnumDType);
        যদি (!enump) {
            ধ্রুব Cross_মান_Range domain
                = cross_মান_Domain(nodep, exprp->width(), exprp->isSigned(), width);
            runs.emplace_back(domain.lo, domain.hi);
            প্রত্যাগম runs;
        }
        std::vector<V3_সংখ্যা> মানসমূহ;
        প্রতি (ধ্রুব AstEnumItem* itemp = enump->itemsp(); itemp;
             itemp = VN_AS(itemp->nextp(), EnumItem)) {
            ধ্রুব V3_সংখ্যা& অঙ্ক = VN_AS(itemp->valuep(), Const)->অঙ্ক();
            যদি (অঙ্ক.isFourState()) অনুবর্ত;  // Not a coverpoint value (19.5.7)
            মানসমূহ.emplace_back(nodep, width);
            যদি (exprp->isSigned()) {
                মানসমূহ.back().opExtendS(অঙ্ক, অঙ্ক.width());
            } অন্যথা {
                মানসমূহ.back().opAssign(অঙ্ক);
            }
        }
        std::sort(মানসমূহ.begin(), মানসমূহ.end(), cross_মান_Less);
        ধ্রুব V3_সংখ্যা one{nodep, width, 1};
        প্রতি (ধ্রুব V3_সংখ্যা& মান : মানসমূহ) {
            V3_সংখ্যা next{nodep, width};
            যদি (!runs.empty() && next.opAdd(runs.back().second, one).isCaseEq(মান)) {
                runs.back().second = মান;
            } অন্যথা {
                runs.emplace_back(মান, মান);
            }
        }
        প্রত্যাগম runs;
    }

    // Emit the constructor code building the bins 'binp' whose values it computes: the values of
    // each element that are coverpoint values (IEEE 1800-2023 19.5.7), those a 'with' filter
    // keeps (19.5.1.1), then its bins
    শূন্য generateConstructedBins(AstCoverpoint* coverpointp, AstCoverBin* binp, AstVar* cpVarp,
                                 Ast_গ্রন্থি_রাশিমালা* exprp) {
        File_সারি* ধ্রুব fl = binp->fileline();
        ধ্রুব string prefix
            = m_sizedNames.get(sanitizeGeneratedName(coverpointp->name() + "__" + binp->name()));
        // The count, and whether it is positive; one of a filter's scalar bin, or bin per value
        ধ্রুব std::pair<Ast_গ্রন্থি_রাশিমালা*, Ast_গ্রন্থি_রাশিমালা*> গুনতি
            = binp->arraySizep() ? new_গুনতি_Args(fl, prefix + "_count", binp->arraySizep())
                                 : new_গুনতি_Args(fl, 1);
        AstCoverWith* ধ্রুব withp = binWith(binp);
        যদি (withp && !binRangesp(binp)) {  // The coverpoint's name: all of its values
            প্রতি (ধ্রুব std::pair<V3_সংখ্যা, V3_সংখ্যা>& run : coverpoint_মানসমূহ(binp, exprp)) {
                m_constructorp->addStmtsp(itemCall(fl, cpVarp,
                                                   exprp->isWide()
                                                       ? VCMethod::COVERGROUP_SIZED_RANGE_W
                                                       : VCMethod::COVERGROUP_SIZED_RANGE,
                                                   {new_মান_Const(fl, run.first, exprp),
                                                    new_মান_Const(fl, run.second, exprp)})
                                              ->makeStmt());
            }
        }
        uint32_t element = 0;
        প্রতি (Ast_গ্রন্থি* rangep = binRangesp(binp); rangep; rangep = rangep->nextp()) {
            যদি (VN_IS(rangep, Unbounded)) {  // A parameter of '$'
                binp->v3error("Bins value may not be '$', which may only bound a range "
                              "(IEEE 1800-2023 6.20.7)");
                অনুবর্ত;
            }
            generateSizedElement(cpVarp, binp, rangep, exprp, prefix + "_" + cvtToStr(element++));
        }
        যদি (withp) generateWithFilter(binp, withp, cpVarp, exprp, prefix);
        std::vector<Ast_গ্রন্থি_রাশিমালা*> args{ctext(fl, binp->binsType().binSetEnum()), গুনতি.first,
                                       গুনতি.second};
        // A filter's bins have the limit it began with
        যদি (!withp) args.ঠেলো_back(cnum(fl, v3Global.opt.coverageMaxBins()));
        ধ্রুব তার্কিক prot = v3Global.opt.protectIds();
        args.ঠেলো_back(ctext(fl, quoted(VIdProtect::protect_শব্দগুলি_If(binp->name(), prot))));
        args.ঠেলো_back(ctext(fl, quoted(VIdProtect::protectIf(fl->filename(), prot))));
        args.ঠেলো_back(cnum(fl, static_cast<uint32_t>(fl->lineno())));
        args.ঠেলো_back(cnum(fl, static_cast<uint32_t>(fl->firstColumn())));
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
    শূন্য generateWithFilter(AstCoverBin* binp, AstCoverWith* withp, AstVar* cpVarp,
                            Ast_গ্রন্থি_রাশিমালা* exprp, ধ্রুব string& prefix) {
        File_সারি* ধ্রুব fl = withp->fileline();
        ধ্রুব তার্কিক wide = exprp->isWide();
        ধ্রুব string grouping = !binp->isArray()     ? "Single"
                                : binp->arraySizep() ? "Fixed"
                                                     : "Values";
        m_constructorp->addStmtsp(itemCall(fl, cpVarp, VCMethod::COVERGROUP_WITH_BEGIN,
                                           {ctext(fl, "VlCovBinGrouping::" + grouping),
                                            cnum(fl, v3Global.opt.coverageMaxBins())})
                                      ->makeStmt());
        // The candidates count in the coverpoint's width, and the filter reads each as 'item',
        // of the coverpoint's type, so that a filter changing 'item' cannot change the loop
        Ast_গ্রন্থি_DType* ধ্রুব মান_DTypep = exprp->dtypep();
        AstVar* ধ্রুব valuep = constructorTemp(fl, prefix + "_value", মান_DTypep);
        AstVar* ধ্রুব lastp = constructorTemp(fl, prefix + "_last", মান_DTypep);
        AstVar* ধ্রুব firstp = constructorTemp(fl, prefix + "_first", মান_DTypep);
        AstVar* ধ্রুব runp = constructorTemp(fl, prefix + "_run", binp->findBitDType());
        AstVar* ধ্রুব morep = constructorTemp(fl, prefix + "_more", binp->findBitDType());
        AstVar* ধ্রুব itemp = withp->itemp()->unlinkFrBack();
        itemp->name(prefix + "_item");
        m_constructorp->addStmtsp(itemp);
        ধ্রুব auto ref = [&](AstVar* varp) { প্রত্যাগম নূতন AstVarRef{fl, varp, VAccess::READ}; };
        ধ্রুব auto assign = [&](AstVar* varp, Ast_গ্রন্থি_রাশিমালা* rhsp) -> Ast_গ্রন্থি* {
            প্রত্যাগম নূতন AstAssign{fl, নূতন AstVarRef{fl, varp, VAccess::WRITE}, rhsp};
        };
        ধ্রুব auto flag = [&](AstVar* varp, তার্কিক মান) {
            প্রত্যাগম assign(varp, মান ? নূতন AstConst{fl, AstConst::BitTrue{}}
                                      : নূতন AstConst{fl, AstConst::BitFalse{}});
        };
        ধ্রুব auto bound = [&](VCMethod narrow, VCMethod wideMethod, AstVar* varp) -> Ast_গ্রন্থি* {
            যদি (wide) {
                প্রত্যাগম itemCall(fl, cpVarp, wideMethod, {নূতন AstVarRef{fl, varp, VAccess::WRITE}})
                    ->makeStmt();
            }
            AstCMethodHard* ধ্রুব callp = itemCall(fl, cpVarp, narrow);
            callp->dtypeSetUInt64();
            প্রত্যাগম assign(varp, resize_মান(callp, মান_DTypep));
        };
        ধ্রুব auto keep = [&](Ast_গ্রন্থি_রাশিমালা* lop, Ast_গ্রন্থি_রাশিমালা* hip) {
            AstCMethodHard* ধ্রুব callp = itemCall(
                fl, cpVarp, wide ? VCMethod::COVERGROUP_WITH_RUN_W : VCMethod::COVERGROUP_WITH_RUN,
                {lop, hip});
            callp->dtypeSetBit();
            প্রত্যাগম assign(morep, callp);
        };
        ধ্রুব auto step = [&](তার্কিক up) {
            AstConst* ধ্রুব onep = নূতন AstConst{fl, AstConst::Widthed_মান{}, exprp->width(), 1};
            Ast_গ্রন্থি_রাশিমালা* ধ্রুব stepp
                = up ? static_cast<Ast_গ্রন্থি_রাশিমালা*>(নূতন AstAdd{fl, ref(valuep), onep})
                     : নূতন AstSub{fl, ref(valuep), onep};
            stepp->dtypep(মান_DTypep);
            প্রত্যাগম stepp;
        };
        AstLoop* ধ্রুব innerp = নূতন AstLoop{fl};
        innerp->addStmtsp(assign(itemp, ref(valuep)));
        innerp->addStmtsp(নূতন AstIf{
            fl, withp->filterp()->unlinkFrBack(),
            নূতন AstIf{fl, নূতন AstNot{fl, ref(runp)},
                      assign(firstp, ref(valuep))->addNext(flag(runp, সত্য))},
            নূতন AstIf{fl, ref(runp), keep(ref(firstp), step(অসত্য))->addNext(flag(runp, অসত্য))}});
        innerp->addStmtsp(নূতন AstLoopTest{
            fl, innerp, নূতন AstLogAnd{fl, ref(morep), নূতন AstNeq{fl, ref(valuep), ref(lastp)}}});
        innerp->addStmtsp(assign(valuep, step(সত্য)));
        AstLoop* ধ্রুব outerp = নূতন AstLoop{fl};
        AstCMethodHard* ধ্রুব nextp = itemCall(fl, cpVarp, VCMethod::COVERGROUP_WITH_NEXT);
        nextp->dtypeSetBit();
        outerp->addStmtsp(নূতন AstLoopTest{fl, outerp, nextp});
        outerp->addStmtsp(
            bound(VCMethod::COVERGROUP_WITH_LO, VCMethod::COVERGROUP_WITH_LO_W, valuep));
        outerp->addStmtsp(
            bound(VCMethod::COVERGROUP_WITH_HI, VCMethod::COVERGROUP_WITH_HI_W, lastp));
        outerp->addStmtsp(flag(runp, অসত্য));
        outerp->addStmtsp(innerp);
        outerp->addStmtsp(নূতন AstIf{fl, ref(runp), keep(ref(firstp), ref(lastp))});
        m_constructorp->addStmtsp(flag(morep, সত্য));
        m_constructorp->addStmtsp(outerp);
    }

    // Emit 'sizedRange(lo, hi)' for the coverpoint values of an element of bins 'binp', whose
    // values the constructor computes: resolved now if constant, else when constructed by
    // clipping to the coverpoint's values.  A wildcard pattern's values give a range for each
    // run of them.  'prefix' names its temporaries.
    শূন্য generateSizedElement(AstVar* cpVarp, ধ্রুব AstCoverBin* binp, Ast_গ্রন্থি* rangep,
                              Ast_গ্রন্থি_রাশিমালা* exprp, ধ্রুব string& prefix) {
        File_সারি* ধ্রুব fl = rangep->fileline();
        ধ্রুব VCMethod method = exprp->isWide() ? VCMethod::COVERGROUP_SIZED_RANGE_W
                                                : VCMethod::COVERGROUP_SIZED_RANGE;
        ধ্রুব AstInsideRange* ধ্রুব irp = VN_CAST(rangep, InsideRange);
        Ast_গ্রন্থি_রাশিমালা* ধ্রুব lowp = irp ? irp->lhsp() : VN_AS(rangep, গ্রন্থি_রাশিমালা);
        Ast_গ্রন্থি_রাশিমালা* ধ্রুব highp = irp ? irp->rhsp() : nullptr;
        ধ্রুব auto unbounded
            = [](ধ্রুব Ast_গ্রন্থি_রাশিমালা* boundp) { প্রত্যাগম !boundp || VN_IS(boundp, Unbounded); };
        ধ্রুব auto constant
            = [&](ধ্রুব Ast_গ্রন্থি_রাশিমালা* boundp) { প্রত্যাগম unbounded(boundp) || VN_IS(boundp, Const); };
        ধ্রুব auto integral = [&](ধ্রুব Ast_গ্রন্থি_রাশিমালা* boundp) {
            প্রত্যাগম unbounded(boundp) || boundp->dtypep()->skipRefp()->isIntegralOrPacked();
        };
        যদি (constant(lowp) && constant(highp)) {
            ধ্রুব auto fourState = [](ধ্রুব Ast_গ্রন্থি_রাশিমালা* boundp) {
                ধ্রুব AstConst* ধ্রুব constp = VN_CAST(boundp, Const);
                প্রত্যাগম constp && constp->অঙ্ক().isFourState();
            };
            যদি (irp && (fourState(lowp) || fourState(highp))) {
                rangep->v3error("Four-state (x/z) value in "
                                << (binp->isArray() ? "array bins" : "bin")
                                << " range bound; range bounds must be two-state constants");
                প্রত্যাগম;
            }
            Cross_মান_Range range{rangep, resolveWidth(rangep, exprp)};
            যদি (!resolve_মান(rangep, exprp, সত্য, binp->isWildcard(), range)) {
                rangep->v3warn(E_UNSUPPORTED, "Unsupported: non-integral value in a coverage bin "
                                              "of an integral coverpoint.");
            } অন্যথা যদি (!crossRangeEmpty(range)) {
                std::vector<std::pair<V3_সংখ্যা, V3_সংখ্যা>> runs{{range.lo, range.hi}};
                যদি (range.wildcard) {  // checkConstructedBins bounded the runs
                    runs.clear();
                    crossRangeRuns(range, v3Global.opt.coverageMaxBins(), runs);
                }
                প্রতি (ধ্রুব std::pair<V3_সংখ্যা, V3_সংখ্যা>& run : runs) {
                    m_constructorp->addStmtsp(itemCall(fl, cpVarp, method,
                                                       {new_মান_Const(fl, run.first, exprp),
                                                        new_মান_Const(fl, run.second, exprp)})
                                                  ->makeStmt());
                }
            }
            প্রত্যাগম;
        }
        যদি (!integral(lowp) || !integral(highp)) {
            rangep->v3warn(E_UNSUPPORTED, "Unsupported: non-integral value in a coverage bin "
                                          "of an integral coverpoint.");
            প্রত্যাগম;
        }
        // Compare values and the coverpoint's domain signed, in a width holding all of them
        ধ্রুব auto boundWidth
            = [&](ধ্রুব Ast_গ্রন্থি_রাশিমালা* boundp) { প্রত্যাগম unbounded(boundp) ? 0 : boundp->width(); };
        ধ্রুব পূর্ণাঙ্ক width = std::max({exprp->width(), boundWidth(lowp), boundWidth(highp)}) + 1;
        ধ্রুব Cross_মান_Range domain
            = cross_মান_Domain(rangep, exprp->width(), exprp->isSigned(), width);
        AstVar* ধ্রুব lop = constructorTemp(fl, prefix + "_lo", exprp->dtypep());
        lop->dtypeSetBitSized(width, VSigning::SIGNED);
        AstVar* ধ্রুব hip = constructorTemp(fl, prefix + "_hi", lop->dtypep());
        ধ্রুব auto bound = [&](Ast_গ্রন্থি_রাশিমালা* boundp, ধ্রুব V3_সংখ্যা& limit) -> Ast_গ্রন্থি_রাশিমালা* {
            যদি (unbounded(boundp)) {
                AstConst* ধ্রুব limitp = নূতন AstConst{fl, limit};
                limitp->dtypeFrom(lop);
                প্রত্যাগম limitp;
            }
            Ast_গ্রন্থি_রাশিমালা* valuep = boundp->clone_বৃক্ষ(অসত্য);
            // A value no wider than a signed coverpoint has its type (see crossRangeBound)
            যদি (exprp->isSigned() && boundp->width() <= exprp->width()) {
                valuep = resize_মান(valuep, exprp->dtypep());
            }
            প্রত্যাগম resize_মান(valuep, lop->dtypep());
        };
        ধ্রুব auto ref = [&](AstVar* varp, VAccess access = VAccess::READ) {
            প্রত্যাগম নূতন AstVarRef{fl, varp, access};
        };
        m_constructorp->addStmtsp(
            নূতন AstAssign{fl, ref(lop, VAccess::WRITE), bound(lowp, domain.lo)});
        m_constructorp->addStmtsp(
            নূতন AstAssign{fl, ref(hip, VAccess::WRITE), irp ? bound(highp, domain.hi) : ref(lop)});
        AstConst* ধ্রুব minp = নূতন AstConst{fl, domain.lo};
        AstConst* ধ্রুব maxp = নূতন AstConst{fl, domain.hi};
        minp->dtypeFrom(lop);
        maxp->dtypeFrom(lop);
        AstCond* ধ্রুব lowerp
            = নূতন AstCond{fl, নূতন AstLtS{fl, ref(lop), minp}, minp->clone_বৃক্ষ(অসত্য), ref(lop)};
        AstCond* ধ্রুব upperp
            = নূতন AstCond{fl, নূতন AstGtS{fl, ref(hip), maxp}, maxp->clone_বৃক্ষ(অসত্য), ref(hip)};
        lowerp->dtypeFrom(lop);
        upperp->dtypeFrom(lop);
        m_constructorp->addStmtsp(নূতন AstAssign{fl, ref(lop, VAccess::WRITE), lowerp});
        m_constructorp->addStmtsp(নূতন AstAssign{fl, ref(hip, VAccess::WRITE), upperp});
        m_constructorp->addStmtsp(নূতন AstIf{fl, নূতন AstLteS{fl, ref(lop), ref(hip)},
                                            itemCall(fl, cpVarp, method,
                                                     {resize_মান(ref(lop), exprp->dtypep()),
                                                      resize_মান(ref(hip), exprp->dtypep())})
                                                ->makeStmt()});
    }

    // The runtime index of the bin of a run holding the coverpoint value, which is in the run:
    // declared + (value - lo) / stride, capped at the last bin, which holds any remainder.
    স্থির Ast_গ্রন্থি_রাশিমালা* runBinIndex(File_সারি* fl, Ast_গ্রন্থি_রাশিমালা* exprp, ধ্রুব BinRun& run) {
        যদি (run.m_গুনতি == 1) প্রত্যাগম cnum(fl, run.m_declared);
        ধ্রুব পূর্ণাঙ্ক width = exprp->width();
        // A run spans at most 2^width values, so offsets in it are unsigned width-bit numbers
        Ast_গ্রন্থি_রাশিমালা* indexp
            = নূতন AstSub{fl, exprp->clone_বৃক্ষ(অসত্য), new_মান_Const(fl, run.m_lo, exprp)};
        indexp->dtypeSetBitSized(width, VSigning::UNSIGNED);
        V3_সংখ্যা stride{fl, width, 0};
        stride.opAssign(run.m_stride);
        যদি (stride.গুনতি_Ones() != 1) {
            indexp = নূতন AstDiv{fl, indexp, নূতন AstConst{fl, stride}};
        } অন্যথা যদি (!stride.isEqOne()) {
            indexp = নূতন AstShiftR{fl, indexp, নূতন AstConst{fl, stride.mostSetBitP1() - 1}};
        }
        // Compare the run's values with those of count bins of stride values, without overflow
        ধ্রুব পূর্ণাঙ্ক extWidth = run.m_lo.width() + 1;
        V3_সংখ্যা lo{fl, extWidth, 0};
        lo.opExtendS(run.m_lo, run.m_lo.width());
        V3_সংখ্যা span{fl, extWidth, 0};
        span.opExtendS(run.m_hi, run.m_hi.width());
        span.opSub(V3_সংখ্যা{span}, lo);
        V3_সংখ্যা covered{fl, extWidth, 0};
        covered.opAssign(run.m_stride);
        covered.opMul(V3_সংখ্যা{covered}, V3_সংখ্যা{fl, extWidth, run.m_গুনতি});
        V3_সংখ্যা hasRemainder{fl, 1, 0};
        যদি (!hasRemainder.opGte(span, covered).isEqZero()) {
            AstConst* ধ্রুব lastp = নূতন AstConst{fl, V3_সংখ্যা{fl, width, run.m_গুনতি - 1}};
            indexp = নূতন AstCond{fl, নূতন AstGt{fl, indexp->clone_বৃক্ষ(অসত্য), lastp},
                                 lastp->clone_বৃক্ষ(অসত্য), indexp};
        }
        যদি (width < VL_IDATASIZE) {
            indexp = নূতন AstExtend{fl, indexp, VL_IDATASIZE};
        } অন্যথা যদি (width > VL_IDATASIZE) {
            indexp = নূতন AstSel{fl, indexp, 0, VL_IDATASIZE};
        }
        প্রত্যাগম নূতন AstAdd{fl, cnum(fl, run.m_declared), indexp};
    }

    // Emit the sample() hit of a run of bins, whose code does not grow with its number of bins:
    //   if (iff && lo <= value && value <= hi) m_cp.incrementBin(<runBinIndex>);
    শূন্য emitRunHit(AstCoverpoint* coverpointp, AstCoverBin* binp, AstVar* cpVarp,
                    Ast_গ্রন্থি_রাশিমালা* exprp, ধ্রুব BinRun& run) {
        File_সারি* ধ্রুব fl = binp->fileline();
        AstConst* ধ্রুব lop = new_মান_Const(fl, run.m_lo, exprp);
        Ast_গ্রন্থি_রাশিমালা* condp = nullptr;
        যদি (run.m_lo.isCaseEq(run.m_hi)) {
            condp = নূতন AstEq{fl, exprp->clone_বৃক্ষ(অসত্য), lop};
        } অন্যথা {
            AstConst* ধ্রুব hip = new_মান_Const(fl, run.m_hi, exprp);
            condp = makeRangeCondition(fl, exprp, lop, hip);
            VL_DO_DANGLING(ঠেলো_Deletep(lop), lop);
            VL_DO_DANGLING(ঠেলো_Deletep(hip), hip);
        }
        emitConvHitIf(coverpointp, binp, cpVarp, runBinIndex(fl, exprp, run), condp);
    }

    // Emit a transition bin's hit action into sample():
    //   if (cond) { m_cp.incrementBin/recordHit(idx); [illegal: $error; $stop] }
    // Used by the transition generators so a completed sequence records into the runtime bin.
    শূন্য addConvTransHitIf(AstCoverpoint* coverpointp, AstCoverBin* binp, ধ্রুব ConvBinTarget& tgt,
                           Ast_গ্রন্থি_রাশিমালা* condp) {
        File_সারি* ধ্রুব fl = binp->fileline();
        Ast_গ্রন্থি* actionp = makeRuntimeBinHit(fl, tgt);
        যদি (binp->binsType() == VCoverBinsType::BINS_ILLEGAL) {
            actionp->addNext(makeIllegalBinAction(
                fl, "Illegal transition bin " + binp->prettyNameQ() + " hit in coverpoint "
                        + coverpointp->prettyNameQ()));
        }
        UASSERT_OBJ(m_sampleFuncp, binp, "sample() CFunc not set for transition bin");
        addSampleStmt(নূতন AstIf{fl, condp, actionp, nullptr});
    }

    // Route a coverpoint through a VlCoverpoint member: emit the member, its sample()
    // increments, the constructor configuration (init + namers), and registration.
    শূন্য generateCoverpoint(AstCoverpoint* coverpointp, Ast_গ্রন্থি_রাশিমালা* exprp, পূর্ণাঙ্ক at_Least_মান) {
        File_সারি* ধ্রুব fl = coverpointp->fileline();
        ধ্রুব তার্কিক dynamic = m_runtimePoints.গুনতি(coverpointp);
        UINFO(4, "  Generating VlCoverpoint member: " << coverpointp->name());

        // Size the hit list to the gen-time max bin overlap (1 unless cross-fed with
        // overlapping ranges), so no cross hit is ever dropped and storage is minimal.
        ধ্রুব তার্কিক crossFed = m_crossedCpNames.গুনতি(coverpointp->name()) != 0;
        ধ্রুব পূর্ণাঙ্ক hitBound = computeHitListBound(coverpointp, exprp, crossFed);
        UINFO(6, "    Hit-list bound (max bin overlap) = " << hitBound);
        AstVar* ধ্রুব cpVarp = নূতন AstVar{fl, VVarType::MEMBER, "__Vcp_" + coverpointp->name(),
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
        যদি (crossFed) {
            UASSERT_OBJ(m_sampleFuncp, coverpointp, "sample() CFunc not set for clearHitList");
            m_sampleFuncp->addStmtsp(
                itemCall(fl, cpVarp, VCMethod::COVERGROUP_CLEAR_HIT_LIST)->makeStmt());
        }
        যদি (dynamic && coverpointHasStateExclusions(coverpointp)) {
            AstVar* ধ্রুব excludedp
                = নূতন AstVar{fl, VVarType::BLOCKTEMP,
                             "__VcpExcluded_" + sanitizeGeneratedName(coverpointp->name()),
                             coverpointp->findBitDType()};
            excludedp->funcLocal(সত্য);
            m_sampleFuncp->addStmtsp(excludedp);
            AstCMethodHard* ধ্রুব callp
                = itemCall(fl, cpVarp,
                           exprp->isWide() ? VCMethod::COVERGROUP_মান_EXCLUDED_W
                                           : VCMethod::COVERGROUP_মান_EXCLUDED,
                           {exprp->clone_বৃক্ষ(অসত্য)});
            callp->dtypeSetBit();
            addSampleStmt(নূতন AstAssign{fl, নূতন AstVarRef{fl, excludedp, VAccess::WRITE}, callp});
            m_excludedVars.emplace(cpVarp, excludedp);
        }

        // Walk bins (non-default, then default), assigning sequential indices that match the
        // namer append order; emit sample increments and collect namer statements.  Constructed
        // bins follow them all, placed when the coverpoint is constructed.
        std::vector<Ast_গ্রন্থি_Stmt*> namerStmts;
        std::vector<AstCoverBin*> defaultBins;
        std::vector<AstCoverBin*> sizedBins;
        std::vector<std::tuple<AstCoverBin*, uint32_t, Ast_গ্রন্থি_রাশিমালা*>> metadata;
        std::vector<ধ্রুব BinRun*> runMetadata;
        uint64_t idx = 0;  // Runtime index of the next bin; 32-bit once checked below
        প্রতি (Ast_গ্রন্থি* binp = coverpointp->binsp(); binp; binp = binp->nextp()) {
            AstCoverBin* ধ্রুব cbinp = VN_AS(binp, CoverBin);
            ধ্রুব পূর্ণাঙ্ক errorsBefore = dynamic ? V3Error::error_গুনতি() : 0;
            যদি (cbinp->binsType() == VCoverBinsType::BINS_DEFAULT) {
                defaultBins.ঠেলো_back(cbinp);
                অনুবর্ত;
            }
            যদি (isConstructedBins(cbinp)) {
                UASSERT_OBJ(dynamic, cbinp, "Constructed bins without value metadata");
                BinSpan span;
                span.sized = static_cast<int32_t>(sizedBins.size());
                m_cpBins.at(cpVarp).spans.emplace(cbinp->name(), span);
                sizedBins.ঠেলো_back(cbinp);
                অনুবর্ত;
            }
            যদি (cbinp->transp()) {
                // Transition bin (incl. array transition 'bins t[] = (a=>b),(c=>d)' and
                // illegal_bins/ignore_bins transitions).  All sequences of one transition bin
                // share a bin name and merge in the coverage DB to a single point, so model
                // them as one runtime bin incremented by any matching sequence.  The sequence
                // matching is generated as a state machine, with the hit routed to this bin's
                // runtime slot.
                namerStmts.ঠেলো_back(makeNamer(cpVarp, cbinp, -1, static_cast<uint32_t>(idx)));
                ধ্রুব ConvBinTarget tgt{cpVarp, static_cast<uint32_t>(idx),
                                        cbinp->binsType().binIsNormal()};
                প্রতি (Ast_গ্রন্থি* sp = cbinp->transp(); sp; sp = sp->nextp())
                    generateSingleTransitionCode(coverpointp, cbinp, exprp, tgt,
                                                 VN_AS(sp, CoverTransSet));
                যদি (dynamic && V3Error::error_গুনতি() == errorsBefore) {
                    metadata.emplace_back(cbinp, static_cast<uint32_t>(idx), nullptr);
                }
                ++idx;
                অনুবর্ত;
            }
            BinRuns plan;
            যদি (binRunsFor(cbinp, exprp, plan)) {
                // Array elements and automatic bins generate as runs, so neither sample() nor
                // the constructor grows with their number of bins.
                যদি (plan.unsupported) {  // bin ignored or invalid; reserve no slot
                    m_droppedBins[coverpointp].ঠেলো_back(cbinp->name());
                    অনুবর্ত;
                }
                CoverpointBins& bins = m_cpBins.at(cpVarp);
                ধ্রুব uint32_t first_মান = bins.total;
                ধ্রুব uint32_t firstDeclared = static_cast<uint32_t>(idx);
                namerStmts.ঠেলো_back(
                    makeNamer(cpVarp, cbinp, plan.গুনতি, firstDeclared, {}, plan.মানসমূহ));
                প্রতি (BinRun& run : plan.runs) {
                    run.m_declared = static_cast<uint32_t>(idx);
                    bins.runs.ঠেলো_back(std::move(run));
                    ধ্রুব BinRun& stored = bins.runs.back();
                    যদি (bins.crossed && cbinp->binsType().binIsNormal()) {
                        ধ্রুব uint32_t first = first_মান + stored.m_declared - firstDeclared;
                        প্রতি (uint32_t element = 0; element < stored.m_গুনতি; ++element) {
                            bins.মানসমূহ[first + element].runp = &stored;
                            bins.মানসমূহ[first + element].element = element;
                        }
                    }
                    যদি (!stored.m_empty) {
                        emitRunHit(coverpointp, cbinp, cpVarp, exprp, stored);
                        যদি (dynamic && V3Error::error_গুনতি() == errorsBefore) {
                            runMetadata.ঠেলো_back(&stored);
                        }
                    }
                    idx += stored.m_গুনতি;
                }
                অনুবর্ত;
            }
            যদি (cbinp->isArray()) {  // value array of a real coverpoint: b[0]..b[N-1]
                // Only integral coverpoints have runtime value metadata (m_runtimePoints)
                UASSERT_OBJ(!dynamic, cbinp, "Runtime value metadata for a real coverpoint");
                তার্কিক unsupported = অসত্য;
                std::vector<Ast_গ্রন্থি_রাশিমালা*> মানসমূহ = extract_Array_মানসমূহ(cbinp, exprp, unsupported);
                যদি (unsupported) {  // bin ignored (COVERIGN emitted); reserve no slot
                    m_droppedBins[coverpointp].ঠেলো_back(cbinp->name());
                    অনুবর্ত;
                }
                namerStmts.ঠেলো_back(makeNamer(cpVarp, cbinp, static_cast<int64_t>(মানসমূহ.size()),
                                               static_cast<uint32_t>(idx), মানসমূহ));
                প্রতি (Ast_গ্রন্থি_রাশিমালা* valuep : মানসমূহ) {
                    // The cross selections of this covergroup still read the value.
                    m_detached_মানসমূহ.ঠেলো_back(valuep);
                    emitConvHitIf(coverpointp, cbinp, cpVarp,
                                  cnum(cbinp->fileline(), static_cast<uint32_t>(idx)),
                                  build_মান_Condition(cbinp, exprp, valuep));
                    ++idx;
                }
            } অন্যথা {
                namerStmts.ঠেলো_back(makeNamer(cpVarp, cbinp, -1, static_cast<uint32_t>(idx)));
                // buildBinCondition is null for 'ignore_bins = default' (no ranges); the bin
                // still gets a reserved slot (recorded, never incremented).
                যদি (Ast_গ্রন্থি_রাশিমালা* ধ্রুব condp = buildBinCondition(cbinp, exprp))
                    emitConvHitIf(coverpointp, cbinp, cpVarp,
                                  cnum(cbinp->fileline(), static_cast<uint32_t>(idx)), condp);
                যদি (dynamic && V3Error::error_গুনতি() == errorsBefore) {
                    metadata.emplace_back(cbinp, static_cast<uint32_t>(idx), nullptr);
                }
                ++idx;
            }
        }
        // A cross selecting an ignored bins declaration selects no bins
        প্রতি (ধ্রুব std::string& name : m_droppedBins[coverpointp]) {
            m_cpBins.at(cpVarp).spans.emplace(name, BinSpan{});
        }
        // A value a constructed bin holds is no default bin's; only sampling tells which do
        AstVar* sizedMatchedp = nullptr;
        যদি (!defaultBins.empty()
            && std::any_of(sizedBins.begin(), sizedBins.end(), [](ধ্রুব AstCoverBin* binp) {
                   প্রত্যাগম binp->binsType().binIsNormal();
               })) {
            sizedMatchedp = নূতন AstVar{fl, VVarType::BLOCKTEMP,
                                       "__VcpSized_" + sanitizeGeneratedName(coverpointp->name()),
                                       coverpointp->findBitDType()};
            sizedMatchedp->funcLocal(সত্য);
            m_sampleFuncp->addStmtsp(sizedMatchedp);
            addSampleStmt(নূতন AstAssign{fl, নূতন AstVarRef{fl, sizedMatchedp, VAccess::WRITE},
                                        নূতন AstConst{fl, AstConst::BitFalse{}}});
        }
        প্রতি (uint32_t sized = 0; sized < sizedBins.size(); ++sized) {
            emitSizedSample(coverpointp, sizedBins[sized], cpVarp, exprp, sized, sizedMatchedp);
        }
        প্রতি (AstCoverBin* ধ্রুব defBinp : defaultBins) {
            File_সারি* ধ্রুব dfl = defBinp->fileline();
            namerStmts.ঠেলো_back(makeNamer(cpVarp, defBinp, -1, static_cast<uint32_t>(idx)));
            Ast_গ্রন্থি_রাশিমালা* condp = buildDefaultCondition(coverpointp, exprp, dfl);
            যদি (sizedMatchedp) {
                condp = নূতন AstLogAnd{
                    dfl, নূতন AstNot{dfl, নূতন AstVarRef{dfl, sizedMatchedp, VAccess::READ}}, condp};
            }
            emitConvHitIf(coverpointp, defBinp, cpVarp, cnum(dfl, static_cast<uint32_t>(idx)),
                          condp);
            ++idx;
        }
        যদি (idx > std::numeric_limits<uint32_t>::max()) {
            // The runtime indexes bins with 32 bits; stop before generating a model
            coverpointp->v3warn(E_UNSUPPORTED, "Unsupported: coverpoint with more than "
                                                   << std::numeric_limits<uint32_t>::max()
                                                   << " bins");
        }

        // Transition coverpoints track the previous sampled value; update it once at the end of
        // this coverpoint's sample() contribution (the prev var was created on demand by the
        // transition matching above).
        যদি (coverpointHasTransition(coverpointp)) {
            AstVar* ধ্রুব prevVarp = VN_AS(coverpointp->user1p(), Var);
            addSampleStmt(
                নূতন AstAssign{coverpointp->fileline(),
                              নূতন AstVarRef{prevVarp->fileline(), prevVarp, VAccess::WRITE},
                              exprp->clone_বৃক্ষ(অসত্য)});
        }

        // Constructor: init (allocates), namers, then registration (under --coverage).
        // Under --protect-ids the hierarchy and page string reach the coverage database
        // verbatim, so obfuscate them like line/toggle points (per-word hierarchy, whole-
        // unit page).  No-ops when --protect-ids is off.
        ধ্রুব তার্কিক prot = v3Global.opt.protectIds();
        ধ্রুব std::string hier
            = VIdProtect::protect_শব্দগুলি_If(m_covergroupName + "." + coverpointp->name(), prot);
        m_constructorp->addStmtsp(
            itemCall(fl, cpVarp, VCMethod::COVERGROUP_INIT,
                     {ctext(fl, quoted(hier)), cnum(fl, static_cast<uint32_t>(at_Least_মান)),
                      cnum(fl, static_cast<uint32_t>(idx))})
                ->makeStmt());
        প্রতি (Ast_গ্রন্থি_Stmt* ধ্রুব ns : namerStmts) m_constructorp->addStmtsp(ns);
        যদি (dynamic) {
            m_constructorp->addStmtsp(
                itemCall(fl, cpVarp, VCMethod::COVERGROUP_মান_TYPE,
                         {cnum(fl, exprp->width()), cnum(fl, exprp->isSigned())})
                    ->makeStmt());
            মান_Lists lists;
            প্রতি (ধ্রুব auto& entry : metadata) {
                collect_মান_Metadata(lists, exprp, std::get<0>(entry), std::get<1>(entry),
                                     std::get<2>(entry));
            }
            প্রতি (ধ্রুব BinRun* ধ্রুব runp : runMetadata) collectRunMetadata(lists, exprp, *runp);
            emit_মান_List(fl, cpVarp, VCMethod::COVERGROUP_মান_RANGES, lists.m_ranges);
            emit_মান_List(fl, cpVarp, VCMethod::COVERGROUP_মান_RUNS, lists.m_runs);
            emit_মান_List(fl, cpVarp, VCMethod::COVERGROUP_মান_PATTERNS, lists.m_patterns);
            emit_মান_List(fl, cpVarp, VCMethod::COVERGROUP_মান_TRANSITIONS, lists.m_transitions);
            প্রতি (AstCoverBin* ধ্রুব binp : sizedBins) {
                generateConstructedBins(coverpointp, binp, cpVarp, exprp);
            }
            m_constructorp->addStmtsp(
                itemCall(fl, cpVarp, VCMethod::COVERGROUP_মান_FINALIZE)->makeStmt());
        }
        যদি (v3Global.opt.coverage()) {
            ধ্রুব std::string page
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
    শূন্য generate_Multi_মান_Transition_Code(AstCoverpoint* coverpointp, AstCoverBin* binp,
                                          Ast_গ্রন্থি_রাশিমালা* exprp, ধ্রুব ConvBinTarget& tgt,
                                          ধ্রুব std::vector<AstCoverTransItem*>& বস্তুসমূহ) {
        UINFO(4, "    Generating multi-value transition state machine for: " << binp->name());
        UINFO(4, "      Sequence length: " << বস্তুসমূহ.size() << " items");

        // Create state position variable
        AstVar* ধ্রুব stateVarp = createSequenceStateVar(coverpointp, binp);

        // Build case statement with N cases (one for each state 0 to N-1)
        // State 0: Not started, looking for first item
        // State 1 to N-1: In progress, looking for next item

        AstCase* ধ্রুব casep
            = নূতন AstCase{binp->fileline(), VCaseType::CT_CASE,
                          নূতন AstVarRef{stateVarp->fileline(), stateVarp, VAccess::READ}, nullptr};

        // Generate each case item in the switch statement
        প্রতি (size_t state = 0; state < বস্তুসমূহ.size(); ++state) {
            AstCaseItem* caseItemp = generateTransitionStateCase(coverpointp, binp, exprp, tgt,
                                                                 stateVarp, বস্তুসমূহ, state);
            casep->addItemsp(caseItemp);
        }

        // Add default case (reset to state 0) to prevent CASEINCOMPLETE warnings,
        // since the state variable is wider than the number of valid states.
        AstCaseItem* ধ্রুব defaultItemp = নূতন AstCaseItem{
            binp->fileline(), nullptr,
            নূতন AstAssign{binp->fileline(),
                          নূতন AstVarRef{binp->fileline(), stateVarp, VAccess::WRITE},
                          নূতন AstConst{binp->fileline(), AstConst::Widthed_মান{}, 8, 0}}};
        casep->addItemsp(defaultItemp);

        addSampleStmt(casep);
        UINFO(4, "      Successfully added multi-value transition state machine");
    }

    // Generate code for a single state in the transition state machine
    // Returns the case item for this state
    AstCaseItem* generateTransitionStateCase(AstCoverpoint* coverpointp, AstCoverBin* binp,
                                             Ast_গ্রন্থি_রাশিমালা* exprp, ধ্রুব ConvBinTarget& tgt,
                                             AstVar* stateVarp,
                                             ধ্রুব std::vector<AstCoverTransItem*>& বস্তুসমূহ,
                                             size_t state) {
        File_সারি* ধ্রুব fl = binp->fileline();

        // Build condition for current value matching expected item at this state
        Ast_গ্রন্থি_রাশিমালা* ধ্রুব matchCondp = buildTransitionItemCondition(বস্তুসমূহ[state], exprp);

        Ast_গ্রন্থি_Stmt* matchActionp = nullptr;

        যদি (state == বস্তুসমূহ.size() - 1) {
            // Last state: sequence complete!  Record the hit in the runtime VlCoverpoint.
            matchActionp = makeRuntimeBinHit(fl, tgt);

            // For illegal_bins, add error message
            যদি (binp->binsType() == VCoverBinsType::BINS_ILLEGAL) {
                ধ্রুব string err_বার্তা = "Illegal transition bin " + binp->prettyNameQ()
                                      + " hit in coverpoint " + coverpointp->prettyNameQ();
                matchActionp = matchActionp->addNext(makeIllegalBinAction(fl, err_বার্তা));
            }

            // Reset state to 0
            matchActionp = matchActionp->addNext(
                নূতন AstAssign{fl, নূতন AstVarRef{fl, stateVarp, VAccess::WRITE},
                              নূতন AstConst{fl, AstConst::Widthed_মান{}, 8, 0}});
        } অন্যথা {
            // Intermediate state: advance to next state
            matchActionp = নূতন AstAssign{
                fl, নূতন AstVarRef{fl, stateVarp, VAccess::WRITE},
                নূতন AstConst{fl, AstConst::Widthed_মান{}, 8, static_cast<uint32_t>(state + 1)}};
        }

        // Build restart logic: check if current value matches first item
        // If so, restart sequence from state 1 (even if we're in middle of sequence)
        Ast_গ্রন্থি_Stmt* noMatchActionp = nullptr;
        যদি (state > 0) {
            // Check if current value matches first item (restart condition)
            Ast_গ্রন্থি_রাশিমালা* ধ্রুব restartCondp = buildTransitionItemCondition(বস্তুসমূহ[0], exprp);

            UASSERT_OBJ(restartCondp, বস্তুসমূহ[0],
                        "buildTransitionItemCondition returned nullptr for restart");

            // Restart to state 1
            Ast_গ্রন্থি_Stmt* restartActionp
                = নূতন AstAssign{fl, নূতন AstVarRef{fl, stateVarp, VAccess::WRITE},
                                নূতন AstConst{fl, AstConst::Widthed_মান{}, 8, 1}};

            // Reset to state 0 (else branch)
            Ast_গ্রন্থি_Stmt* resetActionp
                = নূতন AstAssign{fl, নূতন AstVarRef{fl, stateVarp, VAccess::WRITE},
                                নূতন AstConst{fl, AstConst::Widthed_মান{}, 8, 0}};

            noMatchActionp = নূতন AstIf{fl, restartCondp, restartActionp, resetActionp};
        }
        // For state 0, no action needed if no match (stay in state 0)

        // Combine into if-else
        Ast_গ্রন্থি_Stmt* ধ্রুব stmtp = নূতন AstIf{fl, matchCondp, matchActionp, noMatchActionp};

        // Create case item for this state value
        AstCaseItem* ধ্রুব caseItemp = নূতন AstCaseItem{
            fl, নূতন AstConst{fl, AstConst::Widthed_মান{}, 8, static_cast<uint32_t>(state)},
            stmtp};

        প্রত্যাগম caseItemp;
    }

    // Create: $error(msg); $stop;  Used when an illegal bin is hit.
    Ast_গ্রন্থি_Stmt* makeIllegalBinAction(File_সারি* fl, ধ্রুব string& err_বার্তা) {
        AstDisplay* ধ্রুব errorp
            = নূতন AstDisplay{fl, VDisplayType::DT_ERROR, err_বার্তা, nullptr, nullptr};
        errorp->fmtp()->timeunit(m_covergroupp->timeunit());
        static_cast<Ast_গ্রন্থি*>(errorp)->addNext(নূতন AstStop{fl, সত্য});
        প্রত্যাগম errorp;
    }

    // Preserve the coverpoint's width and signedness after V3Width.
    স্থির AstConst* new_মান_Const(File_সারি* fl, ধ্রুব V3_সংখ্যা& মান, ধ্রুব Ast_গ্রন্থি_রাশিমালা* exprp) {
        V3_সংখ্যা narrowed{fl, exprp->width(), 0};
        narrowed.opAssign(মান);
        AstConst* ধ্রুব constp = নূতন AstConst{fl, narrowed};
        constp->dtypeFrom(exprp);
        প্রত্যাগম constp;
    }

    // A real copy of a real or integral range bound, for comparing with a real coverpoint.
    স্থির AstConst* newRealConst(AstConst* constp) {
        যদি (constp->অঙ্ক().isDouble()) প্রত্যাগম constp->clone_বৃক্ষ(অসত্য);
        V3_সংখ্যা real{&constp->অঙ্ক(), 64};
        real.opIToRD(constp->অঙ্ক(), constp->isSigned());
        প্রত্যাগম নূতন AstConst{constp->fileline(), real};
    }

    // Clone a constant node, widening to targetWidth if needed (zero-extend).
    // Used to ensure comparisons use matching widths after V3Width has run.
    স্থির AstConst* widenConst(File_সারি* fl, AstConst* constp, পূর্ণাঙ্ক targetWidth) {
        যদি (constp->width() == targetWidth) প্রত্যাগম constp->clone_বৃক্ষ(অসত্য);
        V3_সংখ্যা অঙ্ক{fl, targetWidth, 0};
        অঙ্ক.opAssign(constp->অঙ্ক());
        প্রত্যাগম নূতন AstConst{fl, অঙ্ক};
    }

    // Build a range condition: minp <= exprp <= maxp.
    // Uses signed comparisons if exprp is signed; omits trivially-true domain bounds.
    // All arguments are non-owning; clones exprp/minp/maxp as needed.
    Ast_গ্রন্থি_রাশিমালা* makeRangeCondition(File_সারি* fl, Ast_গ্রন্থি_রাশিমালা* exprp, Ast_গ্রন্থি_রাশিমালা* minp,
                                    Ast_গ্রন্থি_রাশিমালা* maxp) {
        ধ্রুব পূর্ণাঙ্ক রাশিমালা_Width = exprp->widthMin();
        AstConst* ধ্রুব minConstp = VN_AS(minp, Const);
        AstConst* ধ্রুব maxConstp = VN_AS(maxp, Const);
        যদি (exprp->isDouble()) {
            // A real coverpoint has no finite domain bounds to omit.
            প্রত্যাগম নূতন AstAnd{fl,
                              নূতন AstGteD{fl, exprp->clone_বৃক্ষ(অসত্য), newRealConst(minConstp)},
                              নূতন AstLteD{fl, exprp->clone_বৃক্ষ(অসত্য), newRealConst(maxConstp)}};
        }
        // Widen constants to match expression width so post-V3Width nodes use correct macros
        AstConst* ধ্রুব minWidep = widenConst(fl, minConstp, রাশিমালা_Width);
        AstConst* ধ্রুব maxWidep = widenConst(fl, maxConstp, রাশিমালা_Width);
        V3_সংখ্যা minimum{fl, রাশিমালা_Width, 0};
        V3_সংখ্যা maximum{fl, রাশিমালা_Width, 0};
        maximum.setAllBits1();
        যদি (exprp->isSigned()) {
            minimum.setBit(রাশিমালা_Width - 1, 1);
            maximum.setBit(রাশিমালা_Width - 1, 0);
        }
        Ast_গ্রন্থি_রাশিমালা* lowerp = nullptr;
        Ast_গ্রন্থি_রাশিমালা* upperp = nullptr;
        যদি (minWidep->অঙ্ক().isCaseEq(minimum)) {
            VL_DO_DANGLING(ঠেলো_Deletep(minWidep), minWidep);
        } অন্যথা {
            lowerp = exprp->isSigned() ? static_cast<Ast_গ্রন্থি_রাশিমালা*>(
                                             নূতন AstGteS{fl, exprp->clone_বৃক্ষ(অসত্য), minWidep})
                                       : static_cast<Ast_গ্রন্থি_রাশিমালা*>(
                                             নূতন AstGte{fl, exprp->clone_বৃক্ষ(অসত্য), minWidep});
        }
        যদি (maxWidep->অঙ্ক().isCaseEq(maximum)) {
            VL_DO_DANGLING(ঠেলো_Deletep(maxWidep), maxWidep);
        } অন্যথা {
            upperp = exprp->isSigned() ? static_cast<Ast_গ্রন্থি_রাশিমালা*>(
                                             নূতন AstLteS{fl, exprp->clone_বৃক্ষ(অসত্য), maxWidep})
                                       : static_cast<Ast_গ্রন্থি_রাশিমালা*>(
                                             নূতন AstLte{fl, exprp->clone_বৃক্ষ(অসত্য), maxWidep});
        }
        যদি (lowerp && upperp) প্রত্যাগম নূতন AstAnd{fl, lowerp, upperp};
        যদি (lowerp) প্রত্যাগম lowerp;
        যদি (upperp) প্রত্যাগম upperp;
        প্রত্যাগম নূতন AstConst{fl, AstConst::BitTrue{}};
    }

    // Build a one-sided comparison for an open-ended bin range whose other bound is '$'.
    // '$' denotes the coverpoint domain extreme, so {[lo:$]} == (expr >= lo) and
    // {[$:hi]} == (expr <= hi).
    Ast_গ্রন্থি_রাশিমালা* makeOpenRangeCondition(File_সারি* fl, Ast_গ্রন্থি_রাশিমালা* exprp, AstConst* boundp,
                                        তার্কিক isLowerBound) {
        AstConst* ধ্রুব widep = widenConst(fl, boundp, exprp->widthMin());
        যদি (isLowerBound) {
            যদি (exprp->isSigned()) প্রত্যাগম নূতন AstGteS{fl, exprp->clone_বৃক্ষ(অসত্য), widep};
            প্রত্যাগম নূতন AstGte{fl, exprp->clone_বৃক্ষ(অসত্য), widep};
        }
        যদি (exprp->isSigned()) প্রত্যাগম নূতন AstLteS{fl, exprp->clone_বৃক্ষ(অসত্য), widep};
        প্রত্যাগম নূতন AstLte{fl, exprp->clone_বৃক্ষ(অসত্য), widep};
    }

    // Build condition for a single transition item.
    // Returns expression that checks if exprp matches the item's value/range list.
    // Overload for when the expression is a variable read -- creates and manages the VarRef
    // internally, so callers don't need to construct a temporary node.
    Ast_গ্রন্থি_রাশিমালা* buildTransitionItemCondition(AstCoverTransItem* itemp, AstVar* varp) {
        Ast_গ্রন্থি_রাশিমালা* varRefp = নূতন AstVarRef{varp->fileline(), varp, VAccess::READ};
        Ast_গ্রন্থি_রাশিমালা* ধ্রুব condp = buildTransitionItemCondition(itemp, varRefp);
        VL_DO_DANGLING(ঠেলো_Deletep(varRefp), varRefp);
        প্রত্যাগম condp;
    }

    // Non-owning: exprp is cloned internally; caller retains ownership of exprp.
    Ast_গ্রন্থি_রাশিমালা* buildTransitionItemCondition(AstCoverTransItem* itemp, Ast_গ্রন্থি_রাশিমালা* exprp) {
        Ast_গ্রন্থি_রাশিমালা* condp = nullptr;

        প্রতি (Ast_গ্রন্থি* valp = itemp->valuesp(); valp; valp = valp->nextp()) {
            Ast_গ্রন্থি_রাশিমালা* singleCondp = nullptr;
            valp = V3Const::constifyEdit(valp);
            AstConst* ধ্রুব constp = VN_CAST(valp, Const);
            যদি (!constp) {
                valp->v3error("Non-constant expression in transition bin; "
                              "values must be constants (IEEE 1800-2023 19.5)");
                প্রত্যাগম নূতন AstConst{valp->fileline(), AstConst::BitFalseErroring{}};
            }
            singleCondp
                = নূতন AstEq{constp->fileline(), exprp->clone_বৃক্ষ(অসত্য), constp->clone_বৃক্ষ(অসত্য)};
            যদি (condp) {
                condp = নূতন AstOr{itemp->fileline(), condp, singleCondp};
            } অন্যথা {
                condp = singleCondp;
            }
        }

        প্রত্যাগম condp;
    }

    // Generate code for a single transition sequence (used by both regular and array bins)
    শূন্য generateSingleTransitionCode(AstCoverpoint* coverpointp, AstCoverBin* binp,
                                      Ast_গ্রন্থি_রাশিমালা* exprp, ধ্রুব ConvBinTarget& tgt,
                                      AstCoverTransSet* transSetp) {
        UINFO(4, "      Generating code for transition sequence");

        // Get or create previous value variable
        AstVar* ধ্রুব prevVarp = create_Prev_মান_Var(coverpointp, exprp);

        UASSERT_OBJ(
            transSetp, binp,
            "Transition bin has no transition set (transp() was checked before calling this)");

        // Get transition items (the sequence: item1 => item2 => item3)
        std::vector<AstCoverTransItem*> বস্তুসমূহ;
        প্রতি (Ast_গ্রন্থি* itemp = transSetp->itemsp(); itemp; itemp = itemp->nextp())
            বস্তুসমূহ.ঠেলো_back(VN_AS(itemp, CoverTransItem));

        যদি (বস্তুসমূহ.empty()) {
            binp->v3error("Transition set without items");
            প্রত্যাগম;
        }

        যদি (বস্তুসমূহ.size() == 1) {
            // Single item transition not valid (need at least 2 values for =>)
            binp->v3error("Transition requires at least two values");
            প্রত্যাগম;
        } অন্যথা যদি (বস্তুসমূহ.size() == 2) {
            // Simple two-value transition: (val1 => val2)
            // Use optimized direct comparison (no state machine needed)
            Ast_গ্রন্থি_রাশিমালা* ধ্রুব cond1p = buildTransitionItemCondition(বস্তুসমূহ[0], prevVarp);
            Ast_গ্রন্থি_রাশিমালা* ধ্রুব cond2p = buildTransitionItemCondition(বস্তুসমূহ[1], exprp);

            // Combine: prev matches val1 AND current matches val2
            Ast_গ্রন্থি_রাশিমালা* fullCondp = নূতন AstAnd{binp->fileline(), cond1p, cond2p};

            addConvTransHitIf(coverpointp, binp, tgt, fullCondp);

            UINFO(4, "        Successfully added 2-value transition if statement");
        } অন্যথা {
            // Multi-value sequence (a => b => c => ...)
            // Use state machine to track position in sequence
            generate_Multi_মান_Transition_Code(coverpointp, binp, exprp, tgt, বস্তুসমূহ);
        }
    }

    // Append a "{ VlCoverpoint* __Vcx_cps[] = {cp0, cp1, ...}; <call> }" statement.  The brace
    // and the temporary array stay literal text -- a CMethodHard is one call, not a block --
    // but callp itself carries the member, method and '->'.  Construction only: init() copies
    // the array into the cross, so sample() reads it from there and needs no array at all.
    AstCStmt* makeCrossCpsCall(File_সারি* fl, ধ্রুব std::vector<AstVar*>& cpVars,
                               AstCMethodHard* callp) {
        AstCStmt* ধ্রুব cs = নূতন AstCStmt{fl};
        cs->add("{ VlCoverpoint* __Vcx_cps[] = {");
        প্রতি (size_t d = 0; d < cpVars.size(); ++d) {
            যদি (d != 0) cs->add(", ");
            cs->add(memberRef(fl, cpVars[d]));
        }
        cs->add("}; ");
        cs->add(callp);
        cs->add("; }");
        প্রত্যাগম cs;
    }

    // Assign the per-bin flags individually: one-bit SV results have integer C++ storage types,
    // which may narrow in a bool initializer list but convert implicitly in assignments.
    AstCStmt* makeCrossIffsCall(File_সারি* fl, ধ্রুব std::vector<AstCoverCrossBin*>& bins,
                                AstCMethodHard* callp) {
        AstCStmt* ধ্রুব cs = নূতন AstCStmt{fl};
        cs->add("{ bool __Vcx_iffs[" + cvtToStr(bins.size()) + "]; ");
        প্রতি (size_t i = 0; i < bins.size(); ++i) {
            ধ্রুব AstCoverCrossBin* ধ্রুব binp = bins[i];
            cs->add("__Vcx_iffs[" + cvtToStr(i) + "] = ");
            cs->add(binp->iffp() ? binp->iffp()->clone_বৃক্ষ(অসত্য)
                                 : নূতন AstConst{fl, AstConst::BitTrue{}});
            cs->add("; ");
        }
        cs->add(callp);
        cs->add("; }");
        প্রত্যাগম cs;
    }

    using CrossSelection = std::vector<uint64_t>;
    সংরচনা ResolvedCrossBin final {
        AstCoverCrossBin* binp;
        CrossSelection selection;
    };
    সংরচনা CrossLayout final {
        uint32_t tuples = 0;
        uint32_t autoBins = 0;
        uint64_t bin_শব্দগুলি = 0;
        তার্কিক valid = সত্য;
        std::vector<ResolvedCrossBin> bins;
    };
    সংরচনা CrossSelectionContext final {
        AstCoverCross* crossp;  // Cross whose tuple space is being selected
        ধ্রুব std::vector<AstVar*>& cpVars;  // Feeding coverpoints in dimension order
        ধ্রুব std::map<std::string, uint32_t>& dimensions;  // Coverpoint name -> dimension
        uint32_t tuples;  // Size of the Cartesian product
        std::vector<uint32_t> strides;  // Flat-index stride per dimension
        তার্কিক valid = সত্য;  // False if this explicit bin cannot be implemented
    };
    সংরচনা CrossBinsofTarget final {
        ধ্রুব CoverpointBins* m_binsp = nullptr;  // Declared bins; null after a reported error
        uint32_t m_dimension = 0;  // Coverpoint index within the cross
        uint32_t m_first = 0;  // First declared normal bin in the selected span
        uint32_t m_গুনতি = 0;  // Number of declared normal bins in the selected span
        uint32_t m_declaredFirst = 0;  // First runtime bin index of the selected span
        uint32_t m_declaredEnd = UINT32_MAX;  // One past the last runtime bin index
        int32_t m_sized = -1;  // Index of the sized array holding the selected bins; or -1
    };
    সংরচনা Cross_মান_Range final {
        V3_সংখ্যা lo;  // Inclusive lower bound, sign-extended to the comparison width
        V3_সংখ্যা hi;  // Inclusive upper bound
        V3_সংখ্যা pattern;  // Allowed bit values; all X for an ordinary interval
        তার্কিক wildcard = অসত্য;  // A wildcard singleton rather than an exact four-state value

        Cross_মান_Range(Ast_গ্রন্থি* nodep, পূর্ণাঙ্ক width)
            : lo{nodep, width}
            , hi{nodep, width}
            , pattern{nodep, width} {
            pattern.setAllBitsX();
        }
    };

    স্থির std::vector<Ast_গ্রন্থি*> cross_Bin_মানসমূহ(ধ্রুব Cross_Bin_মানসমূহ& bin) {
        যদি (bin.valuep) প্রত্যাগম {bin.valuep};
        std::vector<Ast_গ্রন্থি*> মানসমূহ;
        যদি (bin.binp->transp()) {
            // IEEE 1800-2023 19.6.1: binsof uses the last value of each transition.
            প্রতি (Ast_গ্রন্থি* setp = bin.binp->transp(); setp; setp = setp->nextp()) {
                AstCoverTransItem* lastp = VN_AS(setp, CoverTransSet)->itemsp();
                যাবত্ (lastp->nextp()) lastp = VN_AS(lastp->nextp(), CoverTransItem);
                প্রতি (Ast_গ্রন্থি* valuep = lastp->valuesp(); valuep; valuep = valuep->nextp()) {
                    মানসমূহ.ঠেলো_back(valuep);
                }
            }
        } অন্যথা {
            প্রতি (Ast_গ্রন্থি* valuep = bin.binp->rangesp(); valuep; valuep = valuep->nextp()) {
                মানসমূহ.ঠেলো_back(valuep);
            }
        }
        প্রত্যাগম মানসমূহ;
    }

    // The values of the element-th bin of a run, sign-extended to 'width'
    স্থির Cross_মান_Range runBinRange(Ast_গ্রন্থি* nodep, ধ্রুব BinRun& run, uint32_t element,
                                       পূর্ণাঙ্ক width) {
        ধ্রুব পূর্ণাঙ্ক runw = run.m_lo.width();
        V3_সংখ্যা offset{nodep, runw};
        offset.opMul(run.m_stride, V3_সংখ্যা{nodep, runw, element});
        V3_সংখ্যা lo{nodep, runw};
        lo.opAdd(run.m_lo, offset);
        V3_সংখ্যা hi = run.m_hi;
        যদি (element + 1 < run.m_গুনতি) {
            offset.opSub(run.m_stride, V3_সংখ্যা{nodep, runw, 1});
            hi.opAdd(lo, offset);
        }
        Cross_মান_Range range{nodep, width};
        range.lo.opExtendS(lo, runw);
        range.hi.opExtendS(hi, runw);
        প্রত্যাগম range;
    }

    স্থির পূর্ণাঙ্ক crossRangeWidth(Ast_গ্রন্থি* nodep) {
        যদি (ধ্রুব AstInsideRange* ধ্রুব rangep = VN_CAST(nodep, InsideRange)) {
            প্রত্যাগম std::max(rangep->lhsp()->width(), rangep->rhsp()->width());
        }
        প্রত্যাগম nodep->width();
    }

    স্থির তার্কিক cross_মান_Less(ধ্রুব V3_সংখ্যা& lhs, ধ্রুব V3_সংখ্যা& rhs) {
        V3_সংখ্যা result{&lhs};
        প্রত্যাগম !result.opLtS(lhs, rhs).isEqZero();
    }

    // Round a real bound inward to the nearest coverpoint value (IEEE 1800-2023 19.5.7).  Sets
    // 'empty' if the domain has no value on the bound's side.
    স্থির শূন্য crossRealBound(ধ্রুব AstConst* constp, Ast_গ্রন্থি_রাশিমালা* exprp, তার্কিক upper,
                               ধ্রুব Cross_মান_Range& domain, V3_সংখ্যা& result, তার্কিক& empty) {
        ধ্রুব দ্বিগুণ মান = constp->অঙ্ক().toDouble();
        ধ্রুব দ্বিগুণ bound = upper ? std::floor(মান) : std::ceil(মান);
        // Powers of two are exact, so the integral bound compares exactly with the domain edges.
        ধ্রুব দ্বিগুণ limit
            = std::ldexp(1.0, exprp->isSigned() ? exprp->width() - 1 : exprp->width());
        ধ্রুব দ্বিগুণ minimum = exprp->isSigned() ? -limit : 0.0;
        যদি (std::isnan(bound) || (upper ? bound < minimum : bound >= limit)) {
            empty = সত্য;
            result = domain.lo;
        } অন্যথা যদি (bound < minimum) {
            result = domain.lo;
        } অন্যথা যদি (bound >= limit) {
            result = domain.hi;
        } অন্যথা {
            V3_সংখ্যা real{&result, 64};
            real.setDouble(bound);
            result.opRToIRoundS(real);
        }
    }

    স্থির তার্কিক crossRangeBound(Ast_গ্রন্থি* nodep, Ast_গ্রন্থি_রাশিমালা* exprp, তার্কিক upper, তার্কিক bin_মান,
                                ধ্রুব Cross_মান_Range& domain, V3_সংখ্যা& result, তার্কিক& empty) {
        যদি (VN_IS(nodep, Unbounded)) {
            V3_সংখ্যা limit{nodep, exprp->width()};
            যদি (upper) limit.setAllBits1();
            যদি (exprp->isSigned()) {
                limit.setBit(exprp->width() - 1, !upper);
                result.opExtendS(limit, limit.width());
            } অন্যথা {
                result.opAssign(limit);
            }
            প্রত্যাগম সত্য;
        }
        ধ্রুব AstConst* ধ্রুব constp = VN_CAST(nodep, Const);
        যদি (!constp) প্রত্যাগম অসত্য;
        যদি (constp->অঙ্ক().isDouble()) {
            crossRealBound(constp, exprp, upper, domain, result, empty);
            প্রত্যাগম সত্য;
        }
        যদি (constp->অঙ্ক().isString()) প্রত্যাগম অসত্য;
        যদি (bin_মান && exprp->isSigned() && constp->width() <= exprp->width()) {
            // Bin bit patterns use the coverpoint's effective type (IEEE 1800-2023 19.5.7).
            // Wider values and intersect filters retain their values for domain clipping.
            V3_সংখ্যা মান{nodep, exprp->width()};
            যদি (constp->isSigned()) {
                মান.opExtendS(constp->অঙ্ক(), constp->width());
            } অন্যথা {
                মান.opAssign(constp->অঙ্ক());
            }
            result.opExtendS(মান, মান.width());
        } অন্যথা যদি (constp->isSigned()) {
            result.opExtendS(constp->অঙ্ক(), constp->width());
        } অন্যথা {
            result.opAssign(constp->অঙ্ক());
        }
        প্রত্যাগম সত্য;
    }

    স্থির Cross_মান_Range cross_মান_Domain(Ast_গ্রন্থি* nodep, পূর্ণাঙ্ক মান_Width, তার্কিক isSigned,
                                            পূর্ণাঙ্ক width) {
        Cross_মান_Range domain{nodep, width};
        V3_সংখ্যা lo{nodep, মান_Width};
        V3_সংখ্যা hi{nodep, মান_Width};
        hi.setAllBits1();
        যদি (isSigned) {
            lo.setBit(মান_Width - 1, 1);
            hi.setBit(মান_Width - 1, 0);
            domain.lo.opExtendS(lo, মান_Width);
            domain.hi.opExtendS(hi, মান_Width);
        } অন্যথা {
            domain.lo.opAssign(lo);
            domain.hi.opAssign(hi);
        }
        প্রত্যাগম domain;
    }

    স্থির শূন্য intersectCrossRange(Cross_মান_Range& range, ধ্রুব Cross_মান_Range& other) {
        যদি (cross_মান_Less(range.lo, other.lo)) range.lo = other.lo;
        যদি (cross_মান_Less(other.hi, range.hi)) range.hi = other.hi;
    }

    স্থির তার্কিক cross_মান_Range(Ast_গ্রন্থি* nodep, Ast_গ্রন্থি_রাশিমালা* exprp, তার্কিক bin_মান, তার্কিক wildcard,
                                ধ্রুব Cross_মান_Range& domain, Cross_মান_Range& range) {
        তার্কিক empty = অসত্য;
        ধ্রুব AstConst* ধ্রুব constp = VN_CAST(nodep, Const);
        যদি (ধ্রুব AstInsideRange* ধ্রুব rangep = VN_CAST(nodep, InsideRange)) {
            যদি (!crossRangeBound(rangep->lhsp(), exprp, অসত্য, bin_মান, domain, range.lo, empty)
                || !crossRangeBound(rangep->rhsp(), exprp, সত্য, bin_মান, domain, range.hi, empty)
                || range.lo.isFourState() || range.hi.isFourState()) {
                প্রত্যাগম অসত্য;
            }
        } অন্যথা যদি (constp && constp->অঙ্ক().isDouble()) {
            // A real value participates only if integral; it has no wildcard bits.
            crossRealBound(constp, exprp, অসত্য, domain, range.lo, empty);
            crossRealBound(constp, exprp, সত্য, domain, range.hi, empty);
        } অন্যথা {
            যদি (!crossRangeBound(nodep, exprp, অসত্য, bin_মান, domain, range.lo, empty)) {
                প্রত্যাগম অসত্য;
            }
            যদি (bin_মান && exprp->isSigned() && constp && !constp->isSigned()
                && constp->width() > exprp->width()) {
                তার্কিক representable = সত্য;
                প্রতি (পূর্ণাঙ্ক bit = exprp->width(); bit < constp->width(); ++bit) {
                    representable &= !constp->অঙ্ক().bitIs1(bit);
                }
                যদি (representable) {
                    V3_সংখ্যা মান{nodep, exprp->width()};
                    মান.opAssign(constp->অঙ্ক());
                    range.lo.opExtendS(মান, মান.width());
                }
            }
            range.hi = range.lo;
            range.wildcard = wildcard;
            যদি (wildcard) {
                range.pattern = range.lo;
                range.lo = domain.lo;
                range.hi = domain.hi;
                যদি (constp && constp->isSigned()) {
                    // Replicated X sign bits are correlated, not independent wildcards.
                    // The source domain preserves expansion-before-casting (19.5.7).
                    intersectCrossRange(
                        range, cross_মান_Domain(nodep, constp->width(), সত্য, domain.lo.width()));
                }
            }
        }
        যদি (empty) {
            range.lo = domain.hi;
            range.hi = domain.lo;
            প্রত্যাগম সত্য;
        }
        যদি (!range.lo.isFourState()) intersectCrossRange(range, domain);
        যদি (range.wildcard && exprp->isSigned()) {
            ধ্রুব পূর্ণাঙ্ক sign = exprp->width() - 1;
            যদি (constp && !constp->isSigned() && constp->width() > exprp->width()) {
                // Unsigned equality preserves every target bit pattern if discarded bits are zero.
                প্রতি (পূর্ণাঙ্ক bit = exprp->width(); bit < constp->width(); ++bit) {
                    যদি (constp->অঙ্ক().bitIs1(bit)) {
                        range.lo = domain.hi;
                        range.hi = domain.lo;
                        প্রত্যাগম সত্য;
                    }
                }
                প্রতি (পূর্ণাঙ্ক bit = exprp->width(); bit < range.pattern.width(); ++bit) {
                    যদি (range.pattern.bitIsXZ(sign))
                        range.pattern.setBit(bit, 'x');
                    অন্যথা
                        range.pattern.setBit(bit, range.pattern.bitIs1(sign));
                }
                প্রত্যাগম সত্য;
            }
            // Discarded high bits constrain the target sign, rather than becoming don't-cares.
            প্রতি (পূর্ণাঙ্ক bit = exprp->width(); bit < range.pattern.width(); ++bit) {
                যদি (range.pattern.bitIsXZ(bit)) অনুবর্ত;
                ধ্রুব তার্কিক মান = range.pattern.bitIs1(bit);
                যদি (!range.pattern.bitIsXZ(sign) && range.pattern.bitIs1(sign) != মান) {
                    range.lo = domain.hi;
                    range.hi = domain.lo;
                    ভঙ্গ;
                }
                range.pattern.setBit(sign, মান);
            }
        }
        প্রত্যাগম সত্য;
    }

    গণনা CrossRangeState : uint8_t {
        CROSS_INSIDE_BOUNDS = 0,  // Prefix is strictly inside the interval
        CROSS_AT_LOWER = 1,
        CROSS_AT_UPPER = 2,
        CROSS_AT_BOUNDS = CROSS_AT_LOWER | CROSS_AT_UPPER,
        CROSS_NO_MATCH = 4  // Prefix cannot match the interval/pattern
    };

    স্থির CrossRangeState crossRangeStep(ধ্রুব Cross_মান_Range& range, CrossRangeState state,
                                          পূর্ণাঙ্ক bit, পূর্ণাঙ্ক মান) {
        যদি (state == CROSS_NO_MATCH) প্রত্যাগম CROSS_NO_MATCH;
        // Flipping the sign bit makes signed order lexicographic.
        ধ্রুব তার্কিক sign = bit == range.pattern.width() - 1;
        যদি (!range.pattern.bitIsXZ(bit) && মান != (range.pattern.bitIs1(bit) ^ sign)) {
            প্রত্যাগম CROSS_NO_MATCH;
        }
        ধ্রুব পূর্ণাঙ্ক নিচের = range.lo.bitIs1(bit) ^ sign;
        ধ্রুব পূর্ণাঙ্ক উপরের = range.hi.bitIs1(bit) ^ sign;
        যদি (((state & CROSS_AT_LOWER) && মান < নিচের)
            || ((state & CROSS_AT_UPPER) && মান > উপরের)) {
            প্রত্যাগম CROSS_NO_MATCH;
        }
        প্রত্যাগম static_cast<CrossRangeState>(
            ((state & CROSS_AT_LOWER) && মান == নিচের ? CROSS_AT_LOWER : CROSS_INSIDE_BOUNDS)
            | ((state & CROSS_AT_UPPER) && মান == উপরের ? CROSS_AT_UPPER : CROSS_INSIDE_BOUNDS));
    }

    স্থির তার্কিক crossWildcardIntersects(ধ্রুব Cross_মান_Range& range) {
        unsigned states = 1U << CROSS_AT_BOUNDS;
        প্রতি (পূর্ণাঙ্ক bit = range.pattern.width() - 1; bit >= 0 && states; --bit) {
            unsigned next = 0;
            প্রতি (ধ্রুব CrossRangeState state :
                 {CROSS_INSIDE_BOUNDS, CROSS_AT_LOWER, CROSS_AT_UPPER, CROSS_AT_BOUNDS}) {
                যদি (!(states & (1U << state))) অনুবর্ত;
                প্রতি (পূর্ণাঙ্ক মান = 0; মান < 2; ++মান) {
                    ধ্রুব CrossRangeState equal = crossRangeStep(range, state, bit, মান);
                    যদি (equal != CROSS_NO_MATCH) next |= 1U << equal;
                }
            }
            states = next;
        }
        প্রত্যাগম states != 0;
    }

    // The least value at least 'from' with the non-x bits of 'pattern', in unsigned order; false
    // if none.  A bit scan, which does not enumerate the pattern's x bits.
    স্থির তার্কিক next_Pattern_মান(ধ্রুব V3_সংখ্যা& pattern, ধ্রুব V3_সংখ্যা& from, V3_সংখ্যা& result) {
        result.opAssign(from);
        পূর্ণাঙ্ক carry = -1;  // Lowest x bit above the bit scanned where 'from' has a 0
        প্রতি (পূর্ণাঙ্ক bit = from.width() - 1; bit >= 0; --bit) {
            যদি (pattern.bitIsXZ(bit)) {
                যদি (!from.bitIs1(bit)) carry = bit;
                অনুবর্ত;
            }
            যদি (pattern.bitIs1(bit) == from.bitIs1(bit)) অনুবর্ত;
            যদি (from.bitIs1(bit)) {  // Only a greater prefix, an x bit above raised, can match
                যদি (carry < 0) প্রত্যাগম অসত্য;
                bit = carry;
            }
            // Then the least such value: the pattern's bits, with x bits of zero
            result.setBit(bit, 1);
            যাবত্ (--bit >= 0) result.setBit(bit, pattern.bitIs1(bit));
            প্রত্যাগম সত্য;
        }
        প্রত্যাগম সত্য;
    }

    // Append to 'runs' the maximal runs of consecutive values of 'range' that its pattern
    // matches, in value order, until there are more than 'limit'.  A match continues through
    // the pattern's trailing x bits only, which the next value's carry leaves.
    স্থির শূন্য crossRangeRuns(ধ্রুব Cross_মান_Range& range, size_t limit,
                               std::vector<std::pair<V3_সংখ্যা, V3_সংখ্যা>>& runs) {
        // Values order signed, which is the unsigned order of values with the sign bit flipped
        ধ্রুব পূর্ণাঙ্ক sign = range.lo.width() - 1;
        ধ্রুব auto flipped = [sign](V3_সংখ্যা মান) {
            যদি (!মান.bitIsXZ(sign)) মান.setBit(sign, !মান.bitIs1(sign));
            প্রত্যাগম মান;
        };
        ধ্রুব V3_সংখ্যা pattern = flipped(range.pattern);
        ধ্রুব V3_সংখ্যা hi = flipped(range.hi);
        ধ্রুব V3_সংখ্যা one{&hi, hi.width(), 1};
        V3_সংখ্যা trailing{&hi, hi.width()};
        প্রতি (পূর্ণাঙ্ক bit = 0; bit <= sign && pattern.bitIsXZ(bit); ++bit) trailing.setBit(bit, 1);
        V3_সংখ্যা from = flipped(range.lo);
        V3_সংখ্যা first{&hi, hi.width()};
        V3_সংখ্যা last{&hi, hi.width()};
        V3_সংখ্যা less{&hi};
        যাবত্ (runs.size() <= limit && next_Pattern_মান(pattern, from, first)
               && less.opLt(hi, first).isEqZero()) {
            last.opOr(first, trailing);
            যদি (!less.opLt(hi, last).isEqZero()) last = hi;
            runs.emplace_back(flipped(first), flipped(last));
            যদি (last.isCaseEq(hi)) ভঙ্গ;
            from.opAdd(last, one);
        }
    }

    // True if no coverpoint value participates in a resolved value or range.  A value with x
    // or z bits participates only as a wildcard pattern (IEEE 1800-2023 19.5.7).
    স্থির তার্কিক crossRangeEmpty(ধ্রুব Cross_মান_Range& range) {
        যদি (range.lo.isFourState()) প্রত্যাগম সত্য;
        প্রত্যাগম cross_মান_Less(range.hi, range.lo)
               || (range.wildcard && !crossWildcardIntersects(range));
    }

    // Comparison width that holds both the coverpoint's and a bin or intersect value's range.
    স্থির পূর্ণাঙ্ক resolveWidth(Ast_গ্রন্থি* nodep, ধ্রুব Ast_গ্রন্থি_রাশিমালা* exprp) {
        প্রত্যাগম std::max(exprp->width(), crossRangeWidth(nodep)) + 1;
    }

    // Resolve a bin or intersect value to coverpoint values (IEEE 1800-2023 19.5.7), in the
    // width 'range' was created with.  False if the value is not a constant integral or real.
    স্থির তার্কিক resolve_মান(Ast_গ্রন্থি* nodep, Ast_গ্রন্থি_রাশিমালা* exprp, তার্কিক bin_মান, তার্কিক wildcard,
                             Cross_মান_Range& range) {
        ধ্রুব Cross_মান_Range domain
            = cross_মান_Domain(nodep, exprp->width(), exprp->isSigned(), range.lo.width());
        প্রত্যাগম cross_মান_Range(nodep, exprp, bin_মান, wildcard, domain, range);
    }

    স্থির তার্কিক crossRangesIntersect(ধ্রুব Cross_মান_Range& bin, ধ্রুব Cross_মান_Range& filter) {
        // Values with x or z bits do not participate, even in an identical filter.
        যদি (bin.lo.isFourState() || filter.lo.isFourState()) প্রত্যাগম অসত্য;
        Cross_মান_Range match = bin;
        intersectCrossRange(match, filter);
        প্রত্যাগম !cross_মান_Less(match.hi, match.lo)
               && (!bin.wildcard || crossWildcardIntersects(match));
    }

    স্থির শূন্য unsupportedCrossRange(AstCoverBinsof* selectp, তার্কিক& valid) {
        selectp->v3warn(COVERIGN, "Unsupported: non-constant or non-integral 'intersect' value, "
                                  "or four-state range bound.");
        valid = অসত্য;
    }

    স্থির তার্কিক cross_মান_Matches_Filters(AstCoverBinsof* selectp, Ast_গ্রন্থি* valuep,
                                         Ast_গ্রন্থি_রাশিমালা* exprp, ধ্রুব AstCoverBin* binp,
                                         ধ্রুব Cross_মান_Range& domain,
                                         ধ্রুব std::vector<Cross_মান_Range>& filters,
                                         তার্কিক& valid) {
        Cross_মান_Range range{valuep, domain.lo.width()};
        যদি (!cross_মান_Range(valuep, exprp, সত্য, binp->isWildcard(), domain, range)) {
            unsupportedCrossRange(selectp, valid);
            প্রত্যাগম অসত্য;
        }
        প্রতি (ধ্রুব Cross_মান_Range& filter : filters) {
            যদি (crossRangesIntersect(range, filter)) প্রত্যাগম সত্য;
        }
        প্রত্যাগম অসত্য;
    }

    std::vector<তার্কিক> selectCoverpointBins(AstCoverBinsof* selectp, ধ্রুব CoverpointBins& bins,
                                           uint32_t first, uint32_t গুনতি, তার্কিক& valid) {
        যদি (selectp->rangesp() && !bins.exprp->dtypep()->skipRefp()->isIntegralOrPacked()) {
            unsupportedCrossRange(selectp, valid);
            প্রত্যাগম {};
        }
        std::vector<তার্কিক> selected(bins.total, অসত্য);
        std::vector<std::vector<Ast_গ্রন্থি*>> মানসমূহ;
        পূর্ণাঙ্ক width = bins.exprp->width();
        প্রতি (Ast_গ্রন্থি* rangep = selectp->rangesp(); rangep; rangep = rangep->nextp()) {
            width = std::max(width, crossRangeWidth(rangep));
        }
        যদি (selectp->rangesp()) {
            মানসমূহ.reserve(গুনতি);
            প্রতি (uint32_t i = first; i < first + গুনতি; ++i) {
                // A run's values are in the coverpoint type, whose width 'width' covers
                মানসমূহ.ঠেলো_back(bins.মানসমূহ[i].runp ? std::vector<Ast_গ্রন্থি*>{}
                                                     : cross_Bin_মানসমূহ(bins.মানসমূহ[i]));
                প্রতি (Ast_গ্রন্থি* ধ্রুব valuep : মানসমূহ.back()) {
                    width = std::max(width, crossRangeWidth(valuep));
                }
            }
        }
        // One extra bit preserves both unsigned maxima and negative signed bounds.
        ++width;
        ধ্রুব Cross_মান_Range domain
            = cross_মান_Domain(selectp, bins.exprp->width(), bins.exprp->isSigned(), width);
        std::vector<Cross_মান_Range> filters;
        প্রতি (Ast_গ্রন্থি* rangep = selectp->rangesp(); rangep; rangep = rangep->nextp()) {
            Cross_মান_Range filter{rangep, width};
            যদি (!cross_মান_Range(rangep, bins.exprp, অসত্য, অসত্য, domain, filter)) {
                unsupportedCrossRange(selectp, valid);
                প্রত্যাগম {};
            }
            filters.ঠেলো_back(std::move(filter));
        }
        প্রতি (uint32_t i = first; i < first + গুনতি; ++i) {
            যদি (!selectp->rangesp()) {
                selected[i] = সত্য;
                অনুবর্ত;
            }
            যদি (ধ্রুব BinRun* ধ্রুব runp = bins.মানসমূহ[i].runp) {
                ধ্রুব Cross_মান_Range range
                    = runBinRange(selectp, *runp, bins.মানসমূহ[i].element, width);
                selected[i] = std::any_of(filters.begin(), filters.end(),
                                          [&](ধ্রুব Cross_মান_Range& filter) {
                                              প্রত্যাগম crossRangesIntersect(range, filter);
                                          });
                অনুবর্ত;
            }
            প্রতি (Ast_গ্রন্থি* ধ্রুব valuep : মানসমূহ[i - first]) {
                selected[i] = cross_মান_Matches_Filters(
                    selectp, valuep, bins.exprp, bins.মানসমূহ[i].binp, domain, filters, valid);
                যদি (!valid) প্রত্যাগম {};
                যদি (selected[i]) ভঙ্গ;
            }
        }
        যদি (selectp->isNegated()) {
            প্রতি (uint32_t i = 0; i < bins.total; ++i) selected[i] = !selected[i];
        }
        প্রত্যাগম selected;
    }

    // Constructor-time value metadata of one coverpoint, as C++ list entries
    সংরচনা মান_Lists final {
        std::vector<std::string> m_ranges;  // Bin, then low and high words
        std::vector<std::string> m_runs;  // First bin, count, then low, span, and high words
        std::vector<std::string> m_patterns;  // Bin, then value, mask, low, and high words
        std::vector<std::string> m_transitions;  // Transition bin
    };

    // Append a value's words, in the coverpoint's width, to a C++ list entry.
    স্থির শূন্য append_শব্দগুলি(std::string& text, ধ্রুব V3_সংখ্যা& মান, ধ্রুব Ast_গ্রন্থি_রাশিমালা* exprp) {
        V3_সংখ্যা narrowed{&মান, exprp->width()};
        narrowed.opAssign(মান);
        প্রতি (পূর্ণাঙ্ক word = 0; word < exprp->width_শব্দগুলি(); ++word) {
            text += ", " + cvtToStr(narrowed.edataWord(word)) + "U";
        }
    }

    শূন্য collect_মান_Metadata(মান_Lists& lists, Ast_গ্রন্থি_রাশিমালা* exprp, AstCoverBin* binp,
                              uint32_t index, Ast_গ্রন্থি_রাশিমালা* valuep) {
        ধ্রুব std::string bin = cvtToStr(index) + "U";
        যদি (binp->transp()) lists.m_transitions.ঠেলো_back(bin);
        প্রতি (Ast_গ্রন্থি* ধ্রুব sourcep : cross_Bin_মানসমূহ({binp, valuep})) {
            Cross_মান_Range range{sourcep, resolveWidth(sourcep, exprp)};
            যদি (!resolve_মান(sourcep, exprp, সত্য, binp->isWildcard(), range)) {
                // Sampling already resolved every state bin value, so only transitions remain.
                sourcep->v3warn(E_UNSUPPORTED, "Unsupported: non-integral value in a transition "
                                               "bin of a coverpoint with exclusions.");
                অনুবর্ত;
            }
            যদি (crossRangeEmpty(range)) অনুবর্ত;
            std::string entry = bin;
            যদি (range.wildcard) {
                V3_সংখ্যা মান{sourcep, exprp->width()};
                V3_সংখ্যা mask{sourcep, exprp->width()};
                mask.opBitsNonXZ(range.pattern);
                মান.opBitsOne(range.pattern);
                append_শব্দগুলি(entry, মান, exprp);
                append_শব্দগুলি(entry, mask, exprp);
            }
            append_শব্দগুলি(entry, range.lo, exprp);
            append_শব্দগুলি(entry, range.hi, exprp);
            (range.wildcard ? lists.m_patterns : lists.m_ranges).ঠেলো_back(entry);
        }
    }

    // Describe a run with one entry, from which the runtime computes the values of its bins.
    স্থির শূন্য collectRunMetadata(মান_Lists& lists, Ast_গ্রন্থি_রাশিমালা* exprp, ধ্রুব BinRun& run) {
        V3_সংখ্যা span{exprp, run.m_stride.width(), 0};
        span.opSub(run.m_stride, V3_সংখ্যা{exprp, run.m_stride.width(), 1});
        std::string entry = cvtToStr(run.m_declared) + "U, " + cvtToStr(run.m_গুনতি) + "U";
        append_শব্দগুলি(entry, run.m_lo, exprp);
        append_শব্দগুলি(entry, span, exprp);
        append_শব্দগুলি(entry, run.m_hi, exprp);
        lists.m_runs.ঠেলো_back(entry);
    }

    // Emit one batched metadata list, bounding the size of each call's temporary list.
    শূন্য emit_মান_List(File_সারি* fl, AstVar* cpVarp, VCMethod method,
                       ধ্রুব std::vector<std::string>& entries) {
        প্রতি (size_t first = 0; first < entries.size(); first += মান_LIST_ENTRIES) {
            ধ্রুব size_t end = std::min(entries.size(), first + মান_LIST_ENTRIES);
            std::string text = "{" + entries[first];
            প্রতি (size_t i = first + 1; i < end; ++i) text += ", " + entries[i];
            m_constructorp->addStmtsp(
                itemCall(fl, cpVarp, method, {ctext(fl, text + "}")})->makeStmt());
        }
    }

    স্থির তার্কিক checkCrossRef(ধ্রুব AstCoverCrossRef* refp, ধ্রুব AstCoverCross* crossp) {
        যদি (refp->name() == crossp->name()) প্রত্যাগম সত্য;
        refp->v3error("Cross selection "
                      << refp->prettyNameQ() << " may only name its enclosing cross "
                      << crossp->prettyNameQ() << " (IEEE 1800-2023 19.6.1.2).");
        প্রত্যাগম অসত্য;
    }

    স্থির তার্কিক checkCrossBinName(ধ্রুব AstCoverCrossBin* binp, std::set<std::string>& names) {
        যদি (names.emplace(binp->name()).second) প্রত্যাগম সত্য;
        binp->v3error("Duplicate cross bin " << binp->prettyNameQ()
                                             << " (IEEE 1800-2023 19.6.1).");
        প্রত্যাগম অসত্য;
    }

    // The span of the implicit automatic bin reported as 'name' ('auto_<i>', see
    // createImplicitAutoBins), found without naming each of its bins
    স্থির তার্কিক implicitAutoBinSpan(ধ্রুব CoverpointBins& bins, ধ্রুব std::string& name,
                                    BinSpan& span) {
        ধ্রুব std::string prefix = "auto_";
        যদি (!VString::startsWith(name, prefix)) প্রত্যাগম অসত্য;
        ধ্রুব std::string digits = name.substr(prefix.size());
        ধ্রুব unsigned long index = std::strtoul(digits.c_str(), nullptr, 10);
        // Only the reported spelling names the bin, not e.g. 'auto_01' or 'auto_x'
        যদি (index >= bins.implicitAuto.গুনতি || digits != cvtToStr(index)) প্রত্যাগম অসত্য;
        ধ্রুব uint32_t offset = static_cast<uint32_t>(index);
        span = BinSpan{bins.implicitAuto.first + offset, 1, bins.implicitAuto.declared + offset};
        প্রত্যাগম সত্য;
    }

    CrossBinsofTarget
    resolveBinsofTarget(ধ্রুব AstCoverBinsof* selectp, ধ্রুব AstCoverCross* crossp,
                        ধ্রুব std::vector<AstVar*>& cpVars,
                        ধ্রুব std::map<std::string, uint32_t>& dimensions) ধ্রুব {
        ধ্রুব auto dim = dimensions.find(selectp->pointp()->name());
        যদি (dim == dimensions.end()) {
            selectp->v3error("binsof coverpoint "
                             << selectp->pointp()->prettyNameQ() << " is not an item of cross "
                             << crossp->prettyNameQ() << " (IEEE 1800-2023 19.6.1).");
            প্রত্যাগম {};
        }
        ধ্রুব CoverpointBins& bins = m_cpBins.at(cpVars[dim->second]);
        CrossBinsofTarget target{&bins, dim->second, 0, bins.total};
        যদি (!selectp->name().empty()) {
            ধ্রুব auto bin = bins.spans.find(selectp->name());
            BinSpan span{0, 0, 0};
            যদি (bin != bins.spans.end()) {
                span = bin->second;
            } অন্যথা যদি (!implicitAutoBinSpan(bins, selectp->name(), span)) {
                selectp->v3error("Cannot find bin " << selectp->prettyNameQ() << " in coverpoint "
                                                    << selectp->pointp()->prettyNameQ()
                                                    << " (IEEE 1800-2023 19.6.1).");
                প্রত্যাগম {};
            }
            target.m_first = span.first;
            target.m_গুনতি = span.গুনতি;
            target.m_declaredFirst = span.declared;
            target.m_declaredEnd = span.declared + span.গুনতি;
            target.m_sized = span.sized;
        }
        প্রত্যাগম target;
    }

    // The variables a 'with' filter reads, each by its first reference
    using CrossFilterReads = std::map<ধ্রুব AstVar*, AstVarRef*>;

    // Whether a 'with' filter has a count of value tuples, of 'matches' other than '$'
    স্থির তার্কিক counts_মান_Tuples(ধ্রুব AstCoverCrossWith* withp) {
        প্রত্যাগম withp->countp() && !VN_IS(withp->countp(), Unbounded);
    }

    // Whether the runtime knows the values of the cross items a 'with' filter reads, or to count
    // value tuples, of all items, of integral coverpoints; else warns
    স্থির তার্কিক crossFilterSupported(ধ্রুব AstCoverCrossWith* withp,
                                     ধ্রুব CrossFilterReads& reads) {
        প্রতি (ধ্রুব AstVar* varp = withp->varsp(); varp; varp = VN_AS(varp->nextp(), Var)) {
            যদি (varp->dtypep()->skipRefp()->isIntegralOrPacked()) অনুবর্ত;
            ধ্রুব auto it = reads.find(varp);
            যদি (it != reads.end()) {
                it->second->v3warn(COVERIGN, "Unsupported: cross bin 'with' filter reading the "
                                             "value of non-integral coverpoint "
                                                 << it->second->prettyNameQ());
                প্রত্যাগম অসত্য;
            }
            যদি (counts_মান_Tuples(withp)) {
                withp->countp()->v3warn(COVERIGN, "Unsupported: cross bin 'matches' count of "
                                                  "value tuples of non-integral coverpoint "
                                                      << varp->prettyNameQ());
                প্রত্যাগম অসত্য;
            }
        }
        প্রত্যাগম সত্য;
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
    শূন্য generateCrossFilter(ধ্রুব AstCoverCrossWith* withp, ধ্রুব CrossFilterReads& reads,
                             ধ্রুব AstCoverCrossBin* binp, ধ্রুব AstCoverCross* crossp,
                             AstVar* cxp, ধ্রুব std::map<string, uint32_t>& dimensions,
                             uint32_t বস্তুসমূহ) {
        File_সারি* ধ্রুব fl = withp->fileline();
        ধ্রুব string prefix
            = m_sizedNames.get(sanitizeGeneratedName(crossp->name() + "__" + binp->name()));
        ধ্রুব তার্কিক counting = counts_মান_Tuples(withp);
        // The count, 0 for all
        ধ্রুব std::pair<Ast_গ্রন্থি_রাশিমালা*, Ast_গ্রন্থি_রাশিমালা*> গুনতি
            = counting ? new_গুনতি_Args(fl, prefix + "_count", withp->countp())
                       : new_গুনতি_Args(fl, withp->countp() ? 0 : 1);
        AstLoop* ধ্রুব loopp = নূতন AstLoop{fl};
        AstCMethodHard* ধ্রুব nextp = itemCall(fl, cxp, VCMethod::COVERGROUP_WITH_NEXT);
        nextp->dtypeSetBit();
        loopp->addStmtsp(নূতন AstLoopTest{fl, loopp, nextp});
        string dims;
        প্রতি (uint32_t dim = 0; counting && dim < বস্তুসমূহ; ++dim) {
            dims += (dims.empty() ? "" : ", ") + cvtToStr(dim) + "U";
        }
        যাবত্ (AstVar* ধ্রুব varp = withp->varsp()) {
            varp->unlinkFrBack();
            যদি (!reads.গুনতি(varp)) {
                VL_DO_DANGLING(ঠেলো_Deletep(varp), varp);
                অনুবর্ত;
            }
            ধ্রুব auto dimIt = dimensions.find(varp->name());
            UASSERT_OBJ(dimIt != dimensions.end(), varp, "Cross 'with' candidate not of an item");
            ধ্রুব uint32_t dim = dimIt->second;
            যদি (!counting) dims += (dims.empty() ? "" : ", ") + cvtToStr(dim) + "U";
            varp->name(prefix + "_" + sanitizeGeneratedName(varp->name()));
            m_constructorp->addStmtsp(varp);
            যদি (varp->isWide()) {
                loopp->addStmtsp(itemCall(fl, cxp, VCMethod::COVERGROUP_CROSS_WITH_মান_W,
                                          {cnum(fl, dim), নূতন AstVarRef{fl, varp, VAccess::WRITE}})
                                     ->makeStmt());
            } অন্যথা {
                AstCMethodHard* ধ্রুব valuep
                    = itemCall(fl, cxp, VCMethod::COVERGROUP_CROSS_WITH_মান, {cnum(fl, dim)});
                valuep->dtypeSetUInt64();
                Ast_গ্রন্থি_DType* ধ্রুব dtypep = varp->findLogicDType(
                    varp->width(), varp->width(),
                    varp->isSigned() ? VSigning::SIGNED : VSigning::UNSIGNED);
                loopp->addStmtsp(নূতন AstAssign{fl, নূতন AstVarRef{fl, varp, VAccess::WRITE},
                                               resize_মান(valuep, dtypep)});
            }
        }
        loopp->addStmtsp(itemCall(fl, cxp, VCMethod::COVERGROUP_CROSS_WITH_TEST,
                                  {withp->filterp()->unlinkFrBack()})
                             ->makeStmt());
        ধ্রুব তার্কিক prot = v3Global.opt.protectIds();
        m_constructorp->addStmtsp(
            itemCall(fl, cxp, VCMethod::COVERGROUP_CROSS_WITH_BEGIN,
                     {ctext(fl, "{" + dims + "}"), গুনতি.first, গুনতি.second,
                      ctext(fl, quoted(VIdProtect::protect_শব্দগুলি_If(binp->name(), prot))),
                      ctext(fl, quoted(VIdProtect::protectIf(fl->filename(), prot))),
                      cnum(fl, static_cast<uint32_t>(fl->lineno()))})
                ->makeStmt());
        m_constructorp->addStmtsp(loopp);
        m_constructorp->addStmtsp(
            itemCall(fl, cxp, VCMethod::COVERGROUP_CROSS_WITH_END)->makeStmt());
    }

    তার্কিক generateRuntimeSelection(Ast_গ্রন্থি* nodep, ধ্রুব AstCoverCrossBin* binp,
                                  AstCoverCross* crossp, AstVar* cxp,
                                  ধ্রুব std::vector<AstVar*>& cpVars,
                                  ধ্রুব std::map<string, uint32_t>& dimensions) {
        File_সারি* ধ্রুব fl = nodep->fileline();
        যদি (ধ্রুব AstCoverCrossRef* ধ্রুব refp = VN_CAST(nodep, CoverCrossRef)) {
            যদি (!checkCrossRef(refp, crossp)) প্রত্যাগম অসত্য;
            m_constructorp->addStmtsp(
                itemCall(fl, cxp, VCMethod::COVERGROUP_SELECT_ALL)->makeStmt());
            প্রত্যাগম সত্য;
        }
        যদি (ধ্রুব AstCoverCrossSelect* ধ্রুব opp = VN_CAST(nodep, CoverCrossSelect)) {
            যদি (!generateRuntimeSelection(opp->lhsp(), binp, crossp, cxp, cpVars, dimensions)
                || !generateRuntimeSelection(opp->rhsp(), binp, crossp, cxp, cpVars, dimensions)) {
                প্রত্যাগম অসত্য;
            }
            m_constructorp->addStmtsp(itemCall(fl, cxp,
                                               opp->isOr() ? VCMethod::COVERGROUP_SELECT_OR
                                                           : VCMethod::COVERGROUP_SELECT_AND)
                                          ->makeStmt());
            প্রত্যাগম সত্য;
        }
        যদি (ধ্রুব AstCoverCrossWith* ধ্রুব withp = VN_CAST(nodep, CoverCrossWith)) {
            CrossFilterReads reads;
            withp->filterp()->foreach([&](AstVarRef* refp) { reads.emplace(refp->varp(), refp); });
            // The count of 'matches' is a positive integer, or '$' (IEEE 1800-2023 19.6.1.2)
            যদি ((counts_মান_Tuples(withp)
                 && !check_গুনতি(withp->countp(), "Cross bin 'matches' count", "19.6.1.2"))
                || !crossFilterSupported(withp, reads)
                || !generateRuntimeSelection(withp->selectp(), binp, crossp, cxp, cpVars,
                                             dimensions)) {
                প্রত্যাগম অসত্য;
            }
            generateCrossFilter(withp, reads, binp, crossp, cxp, dimensions,
                                static_cast<uint32_t>(cpVars.size()));
            প্রত্যাগম সত্য;
        }
        AstCoverBinsof* ধ্রুব selectp = VN_AS(nodep, CoverBinsof);
        ধ্রুব CrossBinsofTarget target = resolveBinsofTarget(selectp, crossp, cpVars, dimensions);
        যদি (!target.m_binsp) প্রত্যাগম অসত্য;
        ধ্রুব CoverpointBins& bins = *target.m_binsp;
        যদি (selectp->rangesp() && !bins.exprp->dtypep()->skipRefp()->isIntegralOrPacked()) {
            তার্কিক valid = সত্য;
            unsupportedCrossRange(selectp, valid);
            প্রত্যাগম অসত্য;
        }
        Ast_গ্রন্থি_রাশিমালা* firstp;
        Ast_গ্রন্থি_রাশিমালা* endp;
        যদি (target.m_sized < 0) {
            firstp = cnum(fl, target.m_declaredFirst);
            endp = cnum(fl, target.m_declaredEnd);
        } অন্যথা {  // A sized array, whose bins the coverpoint's construction placed
            AstVar* ধ্রুব cpVarp = cpVars[target.m_dimension];
            ধ্রুব uint32_t sized = static_cast<uint32_t>(target.m_sized);
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
        প্রতি (Ast_গ্রন্থি* rangep = selectp->rangesp(); rangep; rangep = rangep->nextp()) {
            Cross_মান_Range range{rangep, resolveWidth(rangep, bins.exprp)};
            যদি (!resolve_মান(rangep, bins.exprp, অসত্য, অসত্য, range)) {
                তার্কিক valid = সত্য;
                unsupportedCrossRange(selectp, valid);
                প্রত্যাগম অসত্য;
            }
            যদি (crossRangeEmpty(range)) অনুবর্ত;
            m_constructorp->addStmtsp(itemCall(fl, cxp,
                                               bins.exprp->isWide()
                                                   ? VCMethod::COVERGROUP_SELECT_RANGE_W
                                                   : VCMethod::COVERGROUP_SELECT_RANGE,
                                               {new_মান_Const(fl, range.lo, bins.exprp),
                                                new_মান_Const(fl, range.hi, bins.exprp)})
                                          ->makeStmt());
        }
        m_constructorp->addStmtsp(
            itemCall(fl, cxp, VCMethod::COVERGROUP_SELECT_DIM_END)->makeStmt());
        প্রত্যাগম সত্য;
    }

    std::vector<AstCoverCrossBin*>
    generateRuntimeCrossBins(AstCoverCross* crossp, AstVar* cxp,
                             ধ্রুব std::vector<AstVar*>& cpVars,
                             ধ্রুব std::map<string, uint32_t>& dimensions) {
        std::vector<AstCoverCrossBin*> bins;
        std::set<string> names;
        প্রতি (Ast_গ্রন্থি* itemp = crossp->binsp(); itemp; itemp = itemp->nextp()) {
            AstCoverCrossBin* ধ্রুব binp = VN_AS(itemp, CoverCrossBin);
            যদি (!checkCrossBinName(binp, names)) অনুবর্ত;
            File_সারি* ধ্রুব fl = binp->fileline();
            // Generate the selection apart, so that an unsupported one leaves no code, which
            // could leave its state for the next bin
            AstFunc* ধ্রুব selectionp = নূতন AstFunc{fl, "", nullptr, nullptr};
            তার্কিক supported;
            {
                VL_RESTORER(m_constructorp);
                m_constructorp = selectionp;
                supported = generateRuntimeSelection(binp->selectp(), binp, crossp, cxp, cpVars,
                                                     dimensions);
            }
            // A supported selection has code
            যদি (supported) m_constructorp->addStmtsp(selectionp->stmtsp()->unlinkFrBackWithNext());
            VL_DO_DANGLING(ঠেলো_Deletep(selectionp), selectionp);
            যদি (!supported) অনুবর্ত;
            ধ্রুব তার্কিক protect = v3Global.opt.protectIds();
            m_constructorp->addStmtsp(
                itemCall(fl, cxp, VCMethod::COVERGROUP_SELECT_BIN,
                         {ctext(fl, binp->binsType().binSetEnum()),
                          ctext(fl, quoted(VIdProtect::protect_শব্দগুলি_If(binp->name(), protect))),
                          ctext(fl, quoted(VIdProtect::protectIf(fl->filename(), protect))),
                          cnum(fl, fl->lineno()), cnum(fl, fl->firstColumn()),
                          cnum(fl, static_cast<uint32_t>(bins.size()))})
                    ->makeStmt());
            bins.ঠেলো_back(binp);
        }
        m_constructorp->addStmtsp(
            itemCall(crossp->fileline(), cxp, VCMethod::COVERGROUP_FINALIZE_BINS)->makeStmt());
        প্রত্যাগম bins;
    }

    স্থির শূন্য setCrossSelectionRange(CrossSelection& selection, uint64_t first, uint64_t end) {
        যাবত্ (first < end) {
            ধ্রুব unsigned bit = first % 64;
            ধ্রুব unsigned bits = std::min<uint64_t>(64 - bit, end - first);
            selection[VL_BITWORD_Q(first)]
                |= (bits == 64 ? ~uint64_t{0} : (uint64_t{1} << bits) - 1) << bit;
            first += bits;
        }
    }

    CrossSelection crossSelection(Ast_গ্রন্থি* nodep, CrossSelectionContext& ctx) {
        যদি (ধ্রুব AstCoverCrossRef* ধ্রুব refp = VN_CAST(nodep, CoverCrossRef)) {
            যদি (!checkCrossRef(refp, ctx.crossp)) {
                ctx.valid = অসত্য;
                প্রত্যাগম {};
            }
            CrossSelection result(
                VL_BITWORD_Q(static_cast<uint64_t>(ctx.tuples) + VL_QUADSIZE - 1), 0);
            setCrossSelectionRange(result, 0, ctx.tuples);
            প্রত্যাগম result;
        }
        যদি (AstCoverCrossSelect* ধ্রুব opp = VN_CAST(nodep, CoverCrossSelect)) {
            CrossSelection lhs = crossSelection(opp->lhsp(), ctx);
            ধ্রুব CrossSelection rhs = crossSelection(opp->rhsp(), ctx);
            যদি (!ctx.valid) প্রত্যাগম {};
            প্রতি (size_t i = 0; i < lhs.size(); ++i) {
                lhs[i] = opp->isOr() ? lhs[i] | rhs[i] : lhs[i] & rhs[i];
            }
            প্রত্যাগম lhs;
        }
        AstCoverBinsof* ধ্রুব selectp = VN_AS(nodep, CoverBinsof);
        ধ্রুব CrossBinsofTarget target
            = resolveBinsofTarget(selectp, ctx.crossp, ctx.cpVars, ctx.dimensions);
        যদি (!target.m_binsp) {
            ctx.valid = অসত্য;
            প্রত্যাগম {};
        }
        ধ্রুব CoverpointBins& bins = *target.m_binsp;
        // Coverpoints with sized arrays are runtime points, so feed only runtime crosses
        UASSERT_OBJ(target.m_sized < 0, selectp, "Sized bin array selected by a static cross");
        ধ্রুব std::vector<তার্কিক> selected
            = selectCoverpointBins(selectp, bins, target.m_first, target.m_গুনতি, ctx.valid);
        যদি (!ctx.valid) প্রত্যাগম {};
        CrossSelection result(VL_BITWORD_Q(static_cast<uint64_t>(ctx.tuples) + VL_QUADSIZE - 1),
                              0);
        ধ্রুব uint64_t stride = ctx.strides[target.m_dimension];
        ধ্রুব uint64_t period = stride * bins.total;
        প্রতি (uint64_t base = 0; base < ctx.tuples; base += period) {
            প্রতি (uint32_t i = 0; i < bins.total;) {
                যদি (!selected[i]) {
                    ++i;
                    অনুবর্ত;
                }
                ধ্রুব uint32_t begin = i++;
                যাবত্ (i < bins.total && selected[i]) ++i;
                setCrossSelectionRange(result, base + begin * stride, base + i * stride);
            }
        }
        প্রত্যাগম result;
    }

    // Size the Cartesian product of the declared Normal bins, with each dimension's flat-index
    // stride.  Live runtime bins only shrink it.  Warns and returns false if it is too large.
    তার্কিক crossShape(AstCoverCross* crossp, ধ্রুব std::vector<AstVar*>& cpVars,
                    std::vector<uint32_t>& strides, uint32_t& tuples) ধ্রুব {
        uint64_t product = std::any_of(cpVars.begin(), cpVars.end(),
                                       [এতত্](AstVar* varp) { প্রত্যাগম !m_cpBins.at(varp).total; })
                               ? 0
                               : 1;
        strides.resize(cpVars.size());
        প্রতি (size_t d = cpVars.size(); d > 0; --d) {
            strides[d - 1] = static_cast<uint32_t>(product);
            product *= m_cpBins.at(cpVars[d - 1]).total;
            যদি (product > UINT32_MAX) {
                crossp->v3warn(COVERIGN,
                               "Unsupported: cross coverage with more than 2^32-1 tuples.");
                প্রত্যাগম অসত্য;
            }
        }
        tuples = static_cast<uint32_t>(product);
        প্রত্যাগম সত্য;
    }

    CrossLayout resolveCrossLayout(AstCoverCross* crossp, ধ্রুব std::vector<AstVar*>& cpVars,
                                   ধ্রুব std::map<std::string, uint32_t>& dimensions) {
        CrossLayout layout;
        CrossSelectionContext ctx{crossp, cpVars, dimensions, 0, {}};
        যদি (!crossShape(crossp, cpVars, ctx.strides, ctx.tuples)) {
            layout.valid = অসত্য;
            প্রত্যাগম layout;
        }
        layout.tuples = ctx.tuples;
        CrossSelection occupied;
        CrossSelection excluded;
        std::set<std::string> names;
        প্রতি (Ast_গ্রন্থি* itemp = crossp->binsp(); itemp; itemp = itemp->nextp()) {
            AstCoverCrossBin* ধ্রুব binp = VN_AS(itemp, CoverCrossBin);
            যদি (!checkCrossBinName(binp, names)) অনুবর্ত;
            ctx.valid = সত্য;
            CrossSelection selection = crossSelection(binp->selectp(), ctx);
            যদি (!ctx.valid || std::all_of(selection.begin(), selection.end(), [](uint64_t word) {
                    প্রত্যাগম word == 0;
                })) {
                অনুবর্ত;
            }
            যদি (occupied.empty()) {
                occupied.resize(selection.size(), 0);
                excluded.resize(selection.size(), 0);
            }
            প্রতি (size_t i = 0; i < selection.size(); ++i) {
                occupied[i] |= selection[i];
                যদি (!binp->binsType().binIsNormal()) excluded[i] |= selection[i];
            }
            layout.bins.ঠেলো_back({binp, std::move(selection)});
        }
        যদি (!layout.bins.empty()) {
            // IEEE 1800-2023 19.6.2/19.6.3: exclusions also remove tuples from named
            // bins, independently of declaration order and sampling guards.
            প্রতি (ResolvedCrossBin& resolved : layout.bins) {
                প্রতি (size_t i = 0; i < resolved.selection.size(); ++i) {
                    যদি (resolved.binp->binsType().binIsNormal()) {
                        resolved.selection[i] &= ~excluded[i];
                    }
                    যদি (resolved.selection[i]) ++layout.bin_শব্দগুলি;
                }
            }
            layout.bins.erase(std::remove_if(layout.bins.begin(), layout.bins.end(),
                                             [](ধ্রুব ResolvedCrossBin& resolved) {
                                                 প্রত্যাগম std::all_of(
                                                     resolved.selection.begin(),
                                                     resolved.selection.end(),
                                                     [](uint64_t word) { প্রত্যাগম word == 0; });
                                             }),
                              layout.bins.end());
            layout.autoBins = layout.tuples;
            প্রতি (ধ্রুব uint64_t word : occupied) {
                layout.autoBins -= static_cast<uint32_t>(std::bitset<VL_QUADSIZE>{word}.গুনতি());
            }
        }
        প্রত্যাগম layout;
    }

    AstCoverCrossDType* crossDType(File_সারি* fl, uint32_t dimensions, ধ্রুব CrossLayout& layout,
                                   তার্কিক dynamic = অসত্য) {
        ধ্রুব uint32_t bins = static_cast<uint32_t>(layout.bins.size());
        ধ্রুব CrossShape shape{dimensions,      layout.tuples,   bins,
                               layout.autoBins, layout.bin_শব্দগুলি, dynamic};
        AstCoverCrossDType*& typep = m_cxDTypes[shape];
        যদি (!typep) {
            typep = নূতন AstCoverCrossDType{
                fl, dimensions, layout.tuples, bins, layout.autoBins, layout.bin_শব্দগুলি, dynamic};
            v3Global.rootp()->typeTablep()->addTypesp(typep);
        }
        প্রত্যাগম typep;
    }

    std::vector<AstCoverCrossBin*> generateCrossBins(AstCoverCross* crossp, AstVar* cxVarp,
                                                     ধ্রুব CrossLayout& layout) {
        std::vector<AstCoverCrossBin*> bins;
        প্রতি (ধ্রুব ResolvedCrossBin& resolved : layout.bins) {
            AstCoverCrossBin* ধ্রুব binp = resolved.binp;
            ধ্রুব CrossSelection& selection = resolved.selection;
            File_সারি* ধ্রুব fl = binp->fileline();
            ধ্রুব তার্কিক prot = v3Global.opt.protectIds();
            std::string mask = "{";
            প্রতি (size_t i = 0; i < selection.size(); ++i) {
                যদি (i) mask += ", ";
                mask += std::to_string(selection[i]) + "ULL";
            }
            mask += "}";
            m_constructorp->addStmtsp(
                itemCall(fl, cxVarp, VCMethod::COVERGROUP_ADD_BIN,
                         {ctext(fl, binp->binsType().binSetEnum()), ctext(fl, mask),
                          ctext(fl, quoted(VIdProtect::protect_শব্দগুলি_If(binp->name(), prot))),
                          ctext(fl, quoted(VIdProtect::protectIf(fl->filename(), prot))),
                          cnum(fl, static_cast<uint32_t>(fl->lineno())),
                          cnum(fl, static_cast<uint32_t>(fl->firstColumn()))})
                    ->makeStmt());
            bins.ঠেলো_back(binp);
        }
        যদি (!bins.empty()) {
            m_constructorp->addStmtsp(
                itemCall(crossp->fileline(), cxVarp, VCMethod::COVERGROUP_FINALIZE_BINS)
                    ->makeStmt());
        }
        প্রত্যাগম bins;
    }

    // Route a cross through a VlCoverCross member: emit the member, its constructor init +
    // registration, and the sample() call.  The feeding coverpoints are already generated
    // (their hit lists drive the cross). Each explicit bin adds one configuration call.
    শূন্য generateCross(AstCoverCross* crossp) {
        File_সারি* ধ্রুব fl = crossp->fileline();
        ধ্রুব তার্কিক dynamic = m_runtimeCrosses.গুনতি(crossp);
        UINFO(4, "  Generating VlCoverCross member: " << crossp->name());

        যদি (Ast_গ্রন্থি_রাশিমালা* ধ্রুব iffp = crossp->iffp()) {
            crossp->iffp(
                captureIffToTemp(iffp, "__VcrossIff_" + sanitizeGeneratedName(crossp->name())));
        }

        // Resolve and unlink the coverpoint refs, in dimension order.  Every ref resolves to a
        // known coverpoint (a cross with an unresolvable item was dropped earlier).
        std::vector<AstVar*> cpVars;
        std::map<std::string, uint32_t> dimensions;
        প্রতি (Ast_গ্রন্থি* itemp = crossp->itemsp(); itemp;) {
            Ast_গ্রন্থি* ধ্রুব nextp = itemp->nextp();
            AstCoverpointRef* ধ্রুব refp = VN_AS(itemp, CoverpointRef);
            ধ্রুব auto it = m_cpVarMap.find(refp->name());
            UASSERT_OBJ(it != m_cpVarMap.end(), crossp, "Cross references an unknown coverpoint");
            dimensions.emplace(refp->name(), static_cast<uint32_t>(cpVars.size()));
            cpVars.ঠেলো_back(it->second);
            VL_DO_DANGLING(ঠেলো_Deletep(refp->unlinkFrBack()), refp);
            itemp = nextp;
        }
        ধ্রুব পূর্ণাঙ্ক dims = static_cast<পূর্ণাঙ্ক>(cpVars.size());
        CrossLayout layout;
        যদি (dynamic) {
            // The runtime sizes the layout from live bins; only check the declared bound here.
            std::vector<uint32_t> strides;
            uint32_t tuples = 0;
            layout.valid = crossShape(crossp, cpVars, strides, tuples);
        } অন্যথা {
            layout = resolveCrossLayout(crossp, cpVars, dimensions);
        }
        যদি (!layout.valid) প্রত্যাগম;

        AstVar* ধ্রুব cxVarp
            = নূতন AstVar{fl, VVarType::MEMBER, "__Vcx_" + crossp->name(),
                         crossDType(fl, static_cast<uint32_t>(dims), layout, dynamic)};
        m_covergroupp->addMembersp(cxVarp);
        m_constructorp->addStmtsp(makeItemCreate(fl, cxVarp,
                                                 dynamic ? VCMethod::COVERGROUP_ADD_CROSS_DYN
                                                         : VCMethod::COVERGROUP_ADD_CROSS));
        generateItemWeight(fl, cxVarp, crossp->optionsp());
        পূর্ণাঙ্ক at_Least_মান;
        পূর্ণাঙ্ক autoBinMax;  // Not a cross option
        extractItemOptions(crossp->optionsp(), at_Least_মান, autoBinMax);

        // Constructor: init (after the coverpoints, which generate earlier) then registration.
        // Obfuscate the hierarchy/filename/page under --protect-ids as for coverpoints above.
        ধ্রুব তার্কিক prot = v3Global.opt.protectIds();
        ধ্রুব std::string hier
            = VIdProtect::protect_শব্দগুলি_If(m_covergroupName + "." + crossp->name(), prot);
        m_constructorp->addStmtsp(makeCrossCpsCall(
            fl, cpVars,
            itemCall(fl, cxVarp, VCMethod::COVERGROUP_INIT,
                     {ctext(fl, quoted(hier)), cnum(fl, static_cast<uint32_t>(at_Least_মান)),
                      cnum(fl, static_cast<uint32_t>(dims)), ctext(fl, "__Vcx_cps"),
                      ctext(fl, quoted(VIdProtect::protectIf(fl->filename(), prot))),
                      cnum(fl, static_cast<uint32_t>(fl->lineno())),
                      cnum(fl, static_cast<uint32_t>(fl->firstColumn()))})));
        ধ্রুব std::vector<AstCoverCrossBin*> bins
            = dynamic ? generateRuntimeCrossBins(crossp, cxVarp, cpVars, dimensions)
                      : generateCrossBins(crossp, cxVarp, layout);
        যদি (v3Global.opt.coverage()) {
            ধ্রুব std::string page
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
        ধ্রুব তার্কিক hasIffs
            = std::any_of(bins.begin(), bins.end(),
                          [](ধ্রুব AstCoverCrossBin* binp) { প্রত্যাগম binp->iffp() != nullptr; });
        Ast_গ্রন্থি_Stmt* ধ্রুব samplep
            = !hasIffs ? static_cast<Ast_গ্রন্থি_Stmt*>(
                             itemCall(fl, cxVarp, VCMethod::COVERGROUP_SAMPLE)->makeStmt())
                       : static_cast<Ast_গ্রন্থি_Stmt*>(makeCrossIffsCall(
                             fl, bins,
                             itemCall(fl, cxVarp, VCMethod::COVERGROUP_SAMPLE_IFFS,
                                      {ctext(fl, "__Vcx_iffs")})));
        যদি (Ast_গ্রন্থি_রাশিমালা* ধ্রুব iffp = crossp->iffp()) {
            m_sampleFuncp->addStmtsp(নূতন AstIf{fl, iffp->clone_বৃক্ষ(অসত্য), samplep});
        } অন্যথা {
            m_sampleFuncp->addStmtsp(samplep);
        }
    }

    শূন্য generateCrossCode(AstCoverCross* crossp) {
        UINFO(4, "  Generating code for cross: " << crossp->name());

        // Non-standard hierarchical/dotted cross item (e.g. 'cross a.b'): an implicit coverpoint
        // over the referenced expression (carried in refp->exprp()).  The grammar already warned
        // NONSTD; implicit coverpoints are not yet implemented, so generate no sampling code for
        // this cross.  When support is added the implicit coverpoint should be synthesized
        // upstream (V3LinkParse) as a real AstCoverpoint so it flows through the normal coverpoint
        // path - by here coverpoint lowering has already run.
        প্রতি (Ast_গ্রন্থি* itemp = crossp->itemsp(); itemp; itemp = itemp->nextp()) {
            ধ্রুব AstCoverpointRef* ধ্রুব refp = VN_AS(itemp, CoverpointRef);
            যদি (refp->exprp()) {
                refp->v3warn(COVERIGN,
                             "Unsupported: cross of hierarchical reference (implicit coverpoint)");
                প্রত্যাগম;
            }
        }

        // A cross naming a bare variable (implicit coverpoint, which Verilator does not
        // synthesize) is dropped entirely with a COVERIGN warning -- it produces no coverage
        // either way -- but only this cross is dropped; its sibling crosses are still generated
        // and the real coverpoints it referenced remain as independent coverpoints.
        যদি (m_droppedCrosses.গুনতি(crossp)) {
            প্রতি (Ast_গ্রন্থি* itemp = crossp->itemsp(); itemp; itemp = itemp->nextp()) {
                ধ্রুব AstCoverpointRef* ধ্রুব refp = VN_AS(itemp, CoverpointRef);
                যদি (m_coverpointMap.find(refp->name()) == m_coverpointMap.end()) {
                    refp->v3warn(COVERIGN, "Unsupported: cross of "
                                               << refp->prettyNameQ()
                                               << " which is not a coverpoint (implicit "
                                                  "coverpoint)");
                    ভঙ্গ;
                }
            }
            প্রত্যাগম;
        }

        // Every cross that isn't dropped routes through a VlCoverCross member.
        generateCross(crossp);
    }

    Ast_গ্রন্থি_রাশিমালা* buildBinCondition(AstCoverBin* binp, Ast_গ্রন্থি_রাশিমালা* exprp) {
        // Get the range list from the bin
        Ast_গ্রন্থি* ধ্রুব rangep = binp->rangesp();
        যদি (!rangep) প্রত্যাগম nullptr;

        // Integral values resolve to the coverpoint's type, as its runtime metadata does
        ধ্রুব তার্কিক integral = exprp->dtypep()->skipRefp()->isIntegralOrPacked();
        // No value form of a wildcard bin is allowed on a real coverpoint (IEEE 1800-2023 19.5.4)
        যদি (binp->isWildcard() && !integral) প্রত্যাগম wildcardTypeError(binp, exprp);

        // Build condition by OR-ing all ranges together
        Ast_গ্রন্থি_রাশিমালা* fullCondp = nullptr;

        প্রতি (Ast_গ্রন্থি* currRangep = rangep; currRangep; currRangep = currRangep->nextp()) {
            Ast_গ্রন্থি_রাশিমালা* rangeCondp = nullptr;
            currRangep = V3Const::constifyEdit(currRangep);

            যদি (AstInsideRange* irp = VN_CAST(currRangep, InsideRange)) {
                Ast_গ্রন্থি_রাশিমালা* ধ্রুব minExprp = irp->lhsp();
                Ast_গ্রন্থি_রাশিমালা* ধ্রুব maxExprp = irp->rhsp();
                AstConst* ধ্রুব minConstp = VN_CAST(minExprp, Const);
                AstConst* ধ্রুব maxConstp = VN_CAST(maxExprp, Const);
                ধ্রুব তার্কিক loUnbounded = VN_IS(minExprp, Unbounded);
                ধ্রুব তার্কিক hiUnbounded = VN_IS(maxExprp, Unbounded);
                যদি (loUnbounded || hiUnbounded) {
                    // Open-ended range: '$' is the coverpoint domain min/max, so the
                    // range reduces to a single inequality (e.g. {[10:$]} -> expr >= 10).
                    AstConst* ধ্রুব boundp = hiUnbounded ? minConstp : maxConstp;
                    যদি (loUnbounded && hiUnbounded) {
                        rangeCondp = নূতন AstConst{irp->fileline(), AstConst::BitTrue{}};
                    } অন্যথা যদি (!boundp) {
                        irp->v3error("Non-constant expression in bin range; "
                                     "range bounds must be constants (IEEE 1800-2023 19.5)");
                        যদি (fullCondp) VL_DO_DANGLING(ঠেলো_Deletep(fullCondp), fullCondp);
                        প্রত্যাগম nullptr;
                    } অন্যথা যদি (boundp->অঙ্ক().isFourState()) {
                        irp->v3error("Four-state (x/z) value in bin range bound; "
                                     "range bounds must be two-state constants");
                        যদি (fullCondp) VL_DO_DANGLING(ঠেলো_Deletep(fullCondp), fullCondp);
                        প্রত্যাগম nullptr;
                    } অন্যথা যদি (integral) {
                        rangeCondp = build_মান_Condition(binp, exprp, irp);
                    } অন্যথা {
                        rangeCondp = makeOpenRangeCondition(irp->fileline(), exprp, boundp,
                                                            /*isLowerBound=*/hiUnbounded);
                    }
                } অন্যথা যদি (!minConstp || !maxConstp) {
                    irp->v3error("Non-constant expression in bin range; "
                                 "range bounds must be constants (IEEE 1800-2023 19.5)");
                    যদি (fullCondp) VL_DO_DANGLING(ঠেলো_Deletep(fullCondp), fullCondp);
                    প্রত্যাগম nullptr;
                } অন্যথা যদি (minConstp->অঙ্ক().isFourState() || maxConstp->অঙ্ক().isFourState()) {
                    irp->v3error("Four-state (x/z) value in bin range bound; "
                                 "range bounds must be two-state constants");
                    যদি (fullCondp) VL_DO_DANGLING(ঠেলো_Deletep(fullCondp), fullCondp);
                    প্রত্যাগম nullptr;
                } অন্যথা যদি (integral) {
                    rangeCondp = build_মান_Condition(binp, exprp, irp);
                } অন্যথা {
                    rangeCondp = makeRangeCondition(irp->fileline(), exprp, minExprp, maxExprp);
                }
            } অন্যথা যদি (AstConst* constp = VN_CAST(currRangep, Const)) {
                rangeCondp = build_মান_Condition(binp, exprp, constp);
            } অন্যথা {
                currRangep->v3error("Non-constant expression in bin range; values must be "
                                    "constants (IEEE 1800-2023 19.5)");
                যদি (fullCondp) VL_DO_DANGLING(ঠেলো_Deletep(fullCondp), fullCondp);
                প্রত্যাগম nullptr;
            }

            UASSERT_OBJ(rangeCondp, binp, "rangeCondp is null after building range condition");
            fullCondp
                = fullCondp ? নূতন AstOr{binp->fileline(), fullCondp, rangeCondp} : rangeCondp;
        }

        প্রত্যাগম fullCondp;
    }

    // Wildcard bits have no meaning for a non-integral coverpoint.
    স্থির Ast_গ্রন্থি_রাশিমালা* wildcardTypeError(AstCoverBin* binp, Ast_গ্রন্থি_রাশিমালা* exprp) {
        ধ্রুব Ast_গ্রন্থি_DType* ধ্রুব dtypep = exprp->dtypep()->skipRefp();
        exprp->v3error("Cannot use a wildcard bin on a coverpoint of type "
                       << dtypep->prettyDTypeNameQ() << " (IEEE 1800-2023 19.5.4).\n"
                       << exprp->warnContextPrimary() << '\n'
                       << binp->warnOther() << "... Location of wildcard bin\n"
                       << binp->warnContextSecondary());
        প্রত্যাগম নূতন AstConst{binp->fileline(), AstConst::BitFalse{}};
    }

    // Match one bin value, range, or wildcard pattern.  Integral coverpoints first resolve the
    // value to their type (IEEE 1800-2023 19.5.7).  Non-owning: clones what it uses.
    Ast_গ্রন্থি_রাশিমালা* build_মান_Condition(AstCoverBin* binp, Ast_গ্রন্থি_রাশিমালা* exprp, Ast_গ্রন্থি* valuep) {
        File_সারি* ধ্রুব fl = valuep->fileline();
        যদি (!exprp->dtypep()->skipRefp()->isIntegralOrPacked()) {
            প্রত্যাগম নূতন AstEq{fl, exprp->clone_বৃক্ষ(অসত্য),
                             VN_AS(valuep, গ্রন্থি_রাশিমালা)->clone_বৃক্ষ(অসত্য)};
        }
        Cross_মান_Range range{valuep, resolveWidth(valuep, exprp)};
        যদি (!resolve_মান(valuep, exprp, সত্য, binp->isWildcard(), range)) {
            valuep->v3warn(E_UNSUPPORTED,
                           "Unsupported: non-integral value in a coverage bin of an "
                           "integral coverpoint.");
            প্রত্যাগম নূতন AstConst{fl, AstConst::BitFalse{}};
        }
        যদি (crossRangeEmpty(range)) প্রত্যাগম নূতন AstConst{fl, AstConst::BitFalse{}};
        AstConst* ধ্রুব lop = new_মান_Const(fl, range.lo, exprp);
        AstConst* ধ্রুব hip = new_মান_Const(fl, range.hi, exprp);
        Ast_গ্রন্থি_রাশিমালা* condp = nullptr;
        যদি (lop->অঙ্ক().isCaseEq(hip->অঙ্ক())) {
            condp = নূতন AstEq{fl, exprp->clone_বৃক্ষ(অসত্য), lop};
        } অন্যথা {
            condp = makeRangeCondition(fl, exprp, lop, hip);
            VL_DO_DANGLING(ঠেলো_Deletep(lop), lop);
        }
        VL_DO_DANGLING(ঠেলো_Deletep(hip), hip);
        যদি (!range.wildcard) প্রত্যাগম condp;
        // Match the pattern's value bits within the source-value bounds.
        V3_সংখ্যা mask{valuep, exprp->width()};
        V3_সংখ্যা মান{valuep, exprp->width()};
        mask.opBitsNonXZ(range.pattern);
        মান.opBitsOne(range.pattern);
        AstConst* ধ্রুব maskConstp = new_মান_Const(fl, mask, exprp);
        AstConst* ধ্রুব মান_Constp = new_মান_Const(fl, মান, exprp);
        Ast_গ্রন্থি_রাশিমালা* ধ্রুব রাশিমালা_Masked = নূতন AstAnd{fl, exprp->clone_বৃক্ষ(অসত্য), maskConstp};
        Ast_গ্রন্থি_রাশিমালা* ধ্রুব মান_Masked = নূতন AstAnd{fl, মান_Constp, maskConstp->clone_বৃক্ষ(অসত্য)};
        প্রত্যাগম নূতন AstLogAnd{fl, condp, নূতন AstEq{fl, রাশিমালা_Masked, মান_Masked}};
    }

    শূন্য generateCoverageComputationCode() {
        UINFO(4, "  Generating coverage computation code");

        // Invalidate cache: addMembersp() calls in generateCoverpointCode/generateCrossCode
        // have added new members since the last scan, so clear before re-querying.
        m_memberMap.clear();

        // get_inst_coverage(): the average of the coverpoints and crosses, weighted by their
        // option.weight (IEEE 1800-2023 19.11).  The instance node holds their runtimes.  With
        // the instances merged, get_coverage() instead, unless option.get_inst_coverage
        // (Table 19-1).
        AstFunc* ধ্রুব getInstCoveragep
            = VN_AS(m_memberMap.findMember(m_covergroupp, "get_inst_coverage"), Func);
        File_সারি* ধ্রুব instFl = getInstCoveragep->fileline();
        AstCMethodHard* ধ্রুব instCallp = instanceCall(instFl, VCMethod::COVERGROUP_COVERAGE);
        instCallp->dtypeSetDouble();
        Ast_গ্রন্থি_রাশিমালা* instValuep = instCallp;
        যদি (m_cgMayMerge) {
            Ast_গ্রন্থি_রাশিমালা* ধ্রুব mergedp = নূতন AstLogAnd{
                instFl, newOptionSel(instFl, optionVar(সত্য), "merge_instances", VAccess::READ),
                নূতন AstLogNot{instFl, newOptionSel(instFl, optionVar(অসত্য), "get_inst_coverage",
                                                   VAccess::READ)}};
            instValuep = নূতন AstCond{instFl, mergedp, typeCoverageCall(instFl), instCallp};
        }
        getInstCoveragep->addStmtsp(নূতন AstAssign{
            instFl, নূতন AstVarRef{instFl, VN_AS(getInstCoveragep->fvarp(), Var), VAccess::WRITE},
            instValuep});

        // get_coverage(): the average of the covergroup's instances, weighted by their
        // option.weight, or with type_option.merge_instances, the coverage of the union of their
        // bins (IEEE 1800-2023 19.11.3).  Static, so the registry finds the instances.
        AstFunc* ধ্রুব getCoveragep
            = VN_AS(m_memberMap.findMember(m_covergroupp, "get_coverage"), Func);
        File_সারি* ধ্রুব typeFl = getCoveragep->fileline();
        getCoveragep->addStmtsp(নূতন AstAssign{
            typeFl, নূতন AstVarRef{typeFl, VN_AS(getCoveragep->fvarp(), Var), VAccess::WRITE},
            typeCoverageCall(typeFl)});
    }

    // The registry call computing the covergroup's type coverage, per its type options
    AstCMethodHard* typeCoverageCall(File_সারি* fl) {
        AstCExpr* ধ্রুব registryp = ctext(fl, "vlSymsp->_vm_contextp__->covergroupRegistryp()");
        registryp->dtypeSetVoid();  // Opaque receiver; only ever the 'fromp' of the call below
        AstCMethodHard* ধ্রুব callp
            = নূতন AstCMethodHard{fl, registryp, VCMethod::COVERGROUP_TYPE_COVERAGE};
        callp->addPinsp(ctext(fl, quoted(covergroupProtectedName())));
        callp->addPinsp(newOptionSel(fl, optionVar(সত্য), "weight", VAccess::READ));
        callp->addPinsp(newOptionSel(fl, optionVar(সত্য), "merge_instances", VAccess::READ));
        callp->addPinsp(file_সারি_Debug(m_covergroupp->fileline()));
        callp->usePtr(সত্য);
        callp->dtypeSetDouble();
        প্রত্যাগম callp;
    }

    // VISITORS
    স্থির তার্কিক isEnclosingInstanceVar(ধ্রুব AstVar* varp) {
        প্রত্যাগম varp->isClassMember() && !varp->lifetime().isStatic() && !varp->isParam();
    }

    শূন্য rewriteThisRef(AstThisRef* refp, AstVar* handleVarp) {
        ধ্রুব AstClassRefDType* ধ্রুব refDTypep
            = VN_CAST(refp->dtypep()->skipRefp(), ClassRefDType);
        UASSERT_OBJ(refDTypep && refDTypep->classp() == m_covergroupp, refp,
                    "Unexpected this reference in embedded covergroup");
        Ast_গ্রন্থি_রাশিমালা* ধ্রুব newp = নূতন AstVarRef{refp->fileline(), handleVarp, VAccess::READ};
        refp->replaceWith(newp);
        VL_DO_DANGLING(ঠেলো_Deletep(refp), refp);
    }

    শূন্য rewriteVarRef(AstVarRef* refp, AstVar* handleVarp) {
        File_সারি* ধ্রুব fl = refp->fileline();
        // As V3LinkLValue does, the handle of a written member select is marked written
        AstMemberSel* ধ্রুব selp
            = নূতন AstMemberSel{fl, নূতন AstVarRef{fl, handleVarp, refp->access()}, refp->varp()};
        selp->access(refp->access());
        refp->replaceWith(selp);
        VL_DO_DANGLING(ঠেলো_Deletep(refp), refp);
    }

    শূন্য rewriteFuncRef(AstFuncRef* refp, AstVar* handleVarp) {
        File_সারি* ধ্রুব fl = refp->fileline();
        AstArg* ধ্রুব argsp = refp->argsp() ? refp->argsp()->unlinkFrBackWithNext() : nullptr;
        AstMethodCall* ধ্রুব callp = নূতন AstMethodCall{
            fl, নূতন AstVarRef{fl, handleVarp, VAccess::READ}, refp->name(), argsp};
        callp->taskp(refp->taskp());
        callp->dtypeFrom(refp);
        refp->replaceWith(callp);
        VL_DO_DANGLING(ঠেলো_Deletep(refp), refp);
    }

    // True if funcp is an instance method of the enclosing class or one of its bases
    তার্কিক isEnclosingInstanceFunc(ধ্রুব Ast_গ্রন্থি_FTask* funcp) ধ্রুব {
        যদি (!funcp->classMethod() || funcp->isStatic()) প্রত্যাগম অসত্য;
        প্রত্যাগম AstClass::isClassExtendedFrom(m_enclosingClassp, VN_AS(funcp->aboveLoopp(), Class));
    }

    তার্কিক isEmbeddedCovergroupVar(ধ্রুব AstVar* varp) ধ্রুব {
        যদি (!varp || !varp->isClassMember() || varp->isDeclTyped()) প্রত্যাগম অসত্য;
        ধ্রুব AstClassRefDType* ধ্রুব refp = VN_CAST(varp->dtypep()->skipRefp(), ClassRefDType);
        প্রত্যাগম refp && refp->classp() == m_covergroupp;
    }

    AstVar* findEmbeddedCovergroupVar() ধ্রুব {
        যদি (!m_enclosingClassp) প্রত্যাগম nullptr;
        প্রতি (Ast_গ্রন্থি* itemp = m_enclosingClassp->membersp(); itemp; itemp = itemp->nextp()) {
            যদি (AstVar* ধ্রুব varp = VN_CAST(itemp, Var)) {
                যদি (isEmbeddedCovergroupVar(varp)) প্রত্যাগম varp;
            }
        }
        // V3LinkParse always creates an implicit variable for an embedded covergroup.
        প্রত্যাগম nullptr;  // LCOV_EXCL_LINE
    }

    std::vector<Ast_গ্রন্থি_Assign*> findCovergroupConstructions() {
        std::vector<Ast_গ্রন্থি_Assign*> foundps;
        যদি (!m_embeddedVarp) প্রত্যাগম foundps;
        AstFunc* ধ্রুব enclosingNewp
            = VN_CAST(m_memberMap.findMember(m_enclosingClassp, "new"), Func);
        যদি (!enclosingNewp) প্রত্যাগম foundps;
        enclosingNewp->foreach([&](Ast_গ্রন্থি_Assign* asgnp) {
            ধ্রুব AstNew* ধ্রুব newp = VN_CAST(asgnp->rhsp(), New);
            ধ্রুব AstVarRef* ধ্রুব lhsRefp = VN_CAST(asgnp->lhsp(), VarRef);
            যদি (!newp || !lhsRefp || lhsRefp->varp() != m_embeddedVarp) প্রত্যাগম;
            ধ্রুব AstClassRefDType* ধ্রুব refp = VN_CAST(newp->dtypep(), ClassRefDType);
            যদি (refp && refp->classp() == m_covergroupp) foundps.ঠেলো_back(asgnp);
        });
        প্রত্যাগম foundps;
    }

    std::set<ধ্রুব AstVar*> enclosingInstanceVars() ধ্রুব {
        std::set<ধ্রুব AstVar*> vars;
        যদি (m_enclosingClassp) {
            m_enclosingClassp->foreachMember([&](AstClass* ধ্রুব, AstVar* ধ্রুব varp) {
                যদি (isEnclosingInstanceVar(varp)) vars.ঢোকাও(varp);
            });
        }
        প্রত্যাগম vars;
    }

    তার্কিক hasEnclosingEventRef(AstCovergroup* cgp) ধ্রুব {
        যদি (!m_embeddedVarp || !cgp->eventp()) প্রত্যাগম অসত্য;
        ধ্রুব std::set<ধ্রুব AstVar*> enclosingVars = enclosingInstanceVars();
        তার্কিক found = অসত্য;
        cgp->eventp()->foreach([&](AstVarRef* refp) {
            যদি (enclosingVars.গুনতি(refp->varp())) found = সত্য;
        });
        প্রত্যাগম found;
    }

    স্থির তার্কিক parse_Embedded_Event_রাশিমালা(Ast_গ্রন্থি_রাশিমালা* exprp, AstVar*& baseVarp,
                                       AstVar*& memberVarp) {
        যদি (AstVarRef* ধ্রুব refp = VN_CAST(exprp, VarRef)) {
            baseVarp = refp->varp();
            memberVarp = nullptr;
            প্রত্যাগম সত্য;
        }
        AstMemberSel* ধ্রুব selp = VN_CAST(exprp, MemberSel);
        যদি (!selp) প্রত্যাগম অসত্য;
        AstVarRef* ধ্রুব baseRefp = VN_CAST(selp->fromp(), VarRef);
        যদি (!baseRefp) প্রত্যাগম অসত্য;
        baseVarp = baseRefp->varp();
        memberVarp = selp->varp();
        প্রত্যাগম সত্য;
    }

    তার্কিক isEventLvalue(Ast_গ্রন্থি_রাশিমালা* exprp, ধ্রুব EmbeddedEventTrigger& trigger) ধ্রুব {
        যদি (AstSel* ধ্রুব selp = VN_CAST(exprp, Sel)) exprp = selp->fromp();
        AstVar* baseVarp = nullptr;
        AstVar* memberVarp = nullptr;
        যদি (!parse_Embedded_Event_রাশিমালা(exprp, baseVarp, memberVarp)) প্রত্যাগম অসত্য;
        প্রত্যাগম baseVarp == trigger.baseVarp && memberVarp == trigger.memberVarp;
    }

    Ast_গ্রন্থি_রাশিমালা* newEventRead(File_সারি* fl, ধ্রুব EmbeddedEventTrigger& trigger) ধ্রুব {
        Ast_গ্রন্থি_রাশিমালা* ধ্রুব basep = নূতন AstVarRef{fl, trigger.baseVarp, VAccess::READ};
        যদি (!trigger.memberVarp) প্রত্যাগম basep;
        AstMemberSel* ধ্রুব selp = নূতন AstMemberSel{fl, basep, trigger.memberVarp};
        selp->access(VAccess::READ);
        প্রত্যাগম selp;
    }

    string eventPrevName(ধ্রুব EmbeddedEventTrigger& trigger, size_t triggerIndex) ধ্রুব {
        string name = "__Vcg_prev_" + m_embeddedVarp->name() + "_" + std::to_string(triggerIndex)
                      + "_" + trigger.baseVarp->name();
        যদি (trigger.memberVarp) name += "_" + trigger.memberVarp->name();
        প্রত্যাগম name;
    }

    Ast_গ্রন্থি_রাশিমালা* newEmbeddedVarNonNull(File_সারি* fl) ধ্রুব {
        প্রত্যাগম নূতন AstNeq{fl, নূতন AstVarRef{fl, m_embeddedVarp, VAccess::READ},
                          নূতন AstConst{fl, AstConst::Null{}}};
    }

    Ast_গ্রন্থি_Stmt* newSampleStmt(File_সারি* fl) ধ্রুব {
        AstMethodCall* ধ্রুব callp = নূতন AstMethodCall{
            fl, নূতন AstVarRef{fl, m_embeddedVarp, VAccess::READ}, "sample", nullptr};
        callp->taskp(m_sampleFuncp);
        callp->dtypeSetVoid();
        প্রত্যাগম callp->makeStmt();
    }

    শূন্য installEmbeddedEventFork(Ast_Sen_বৃক্ষ* eventp,
                                  ধ্রুব std::vector<Ast_গ্রন্থি_Assign*>& constructps) {
        // IEEE 1800-2023 19.3 samples coverpoints whenever their clocking event occurs. A
        // per-instance event cannot use V3Active's static sensitivity path, so spawn
        // 'fork forever begin @(event); cg.sample(); end join_none' after each construction.
        প্রতি (Ast_গ্রন্থি_Assign* ধ্রুব constructp : constructps) {
            File_সারি* ধ্রুব fl = constructp->fileline();
            AstLoop* ধ্রুব loopp = নূতন AstLoop{fl};
            loopp->addStmtsp(নূতন AstEventControl{fl, eventp->clone_বৃক্ষ(অসত্য), nullptr});
            loopp->addStmtsp(নূতন AstIf{fl, newEmbeddedVarNonNull(fl), newSampleStmt(fl)});
            AstFork* ধ্রুব forkp = নূতন AstFork{fl, VJoinType::JOIN_NONE};
            forkp->immediateStart(সত্য);
            forkp->addForksp(নূতন AstBegin{fl, "", loopp, সত্য});
            constructp->addNextHere(forkp);
        }
        VL_DO_DANGLING(ঠেলো_Deletep(eventp), eventp);
    }

    Ast_গ্রন্থি_রাশিমালা* newEventReadyCondition(File_সারি* fl, ধ্রুব EmbeddedEventTrigger& trigger) ধ্রুব {
        Ast_গ্রন্থি_রাশিমালা* ধ্রুব curp = newEventRead(fl, trigger);
        Ast_গ্রন্থি_রাশিমালা* ধ্রুব prevp = নূতন AstVarRef{fl, trigger.prevVarp, VAccess::READ};
        Ast_গ্রন্থি_রাশিমালা* edgep = nullptr;
        // IEEE 1800-2023 9.4.2 detects edge-qualified events only on the expression's LSB,
        // while an implicit change event observes the complete expression.
        যদি (trigger.edgeType == VEdgeType::ET_POSEDGE) {
            edgep = নূতন AstSel{fl, নূতন AstAnd{fl, curp, নূতন AstNot{fl, prevp}}, 0, 1};
        } অন্যথা যদি (trigger.edgeType == VEdgeType::ET_NEGEDGE) {
            edgep = নূতন AstSel{fl, নূতন AstAnd{fl, নূতন AstNot{fl, curp}, prevp}, 0, 1};
        } অন্যথা যদি (trigger.edgeType == VEdgeType::ET_BOTHEDGE) {
            edgep = নূতন AstSel{fl, নূতন AstXor{fl, curp, prevp}, 0, 1};
        } অন্যথা {
            edgep = নূতন AstNeq{fl, curp, prevp};
        }
        প্রত্যাগম নূতন AstLogAnd{fl, newEmbeddedVarNonNull(fl), edgep};
    }

    std::vector<EmbeddedEventTrigger> collectEmbeddedEventTriggers(AstCovergroup* cgp) {
        std::vector<EmbeddedEventTrigger> triggers;
        ধ্রুব std::set<ধ্রুব AstVar*> enclosingVars = enclosingInstanceVars();
        প্রতি (Ast_গ্রন্থি* senp = cgp->eventp()->sensesp(); senp; senp = senp->nextp()) {
            AstSenItem* ধ্রুব itemp = VN_AS(senp, SenItem);
            AstVar* baseVarp = nullptr;
            AstVar* memberVarp = nullptr;
            যদি (!parse_Embedded_Event_রাশিমালা(itemp->sensp(), baseVarp, memberVarp)
                || !enclosingVars.গুনতি(baseVarp)) {
                প্রত্যাগম {};
            }
            triggers.emplace_back(itemp->fileline(), baseVarp, memberVarp, itemp->edgeType());
        }
        প্রত্যাগম triggers;
    }

    শূন্য installEmbeddedEventTriggers(std::vector<EmbeddedEventTrigger>& triggers,
                                      ধ্রুব std::vector<Ast_গ্রন্থি_Assign*>& constructps) {
        // Without --timing, approximate a per-instance event by sampling after assignments
        // within the enclosing class. External writes and exact scheduling cannot be observed.
        যদি (constructps.empty()) প্রত্যাগম;
        প্রতি (size_t triggerIndex = 0; triggerIndex < triggers.size(); ++triggerIndex) {
            EmbeddedEventTrigger& trigger = triggers[triggerIndex];
            std::vector<Ast_গ্রন্থি_Assign*> assignps;
            m_enclosingClassp->foreach([&](Ast_গ্রন্থি_Assign* asgnp) {
                যদি (isEventLvalue(asgnp->lhsp(), trigger)) assignps.ঠেলো_back(asgnp);
            });
            যদি (assignps.empty()) {
                trigger.eventFl->v3warn(
                    COVERIGN, "Unsupported: 'covergroup' clocking event signal has no assignment "
                              "within the enclosing class; no coverage sampled. Use --timing for "
                              "full support.");
                অনুবর্ত;
            }
            Ast_গ্রন্থি_DType* ধ্রুব dtypep
                = trigger.memberVarp ? trigger.memberVarp->dtypep() : trigger.baseVarp->dtypep();
            AstVar* ধ্রুব prevVarp = নূতন AstVar{trigger.eventFl, VVarType::MEMBER,
                                                eventPrevName(trigger, triggerIndex), dtypep};
            m_enclosingClassp->addMembersp(prevVarp);
            trigger.prevVarp = prevVarp;
            প্রতি (Ast_গ্রন্থি_Assign* ধ্রুব asgnp : assignps) {
                File_সারি* ধ্রুব fl = asgnp->fileline();
                AstIf* ধ্রুব ifp
                    = নূতন AstIf{fl, newEventReadyCondition(fl, trigger), newSampleStmt(fl)};
                ifp->addNextHere(নূতন AstAssign{fl,
                                               নূতন AstVarRef{fl, trigger.prevVarp, VAccess::WRITE},
                                               newEventRead(fl, trigger)});
                asgnp->addNextHere(ifp);
            }
        }
    }

    শূন্য delete_Coverage_বস্তুসমূহ() {
        প্রতি (AstCoverpoint* ধ্রুব cpp : m_coverpoints) {
            VL_DO_DANGLING(ঠেলো_Deletep(cpp->unlinkFrBack()), cpp);
        }
        প্রতি (AstCoverCross* ধ্রুব crossp : m_coverCrosses) {
            VL_DO_DANGLING(ঠেলো_Deletep(crossp->unlinkFrBack()), crossp);
        }
        // Options not lowered: the covergroup was not processed
        প্রতি (AstCgOptionAssign* ধ্রুব optp : m_cgOptions) {
            VL_DO_DANGLING(ঠেলো_Deletep(optp->unlinkFrBack()), optp);
        }
        m_cgOptions.clear();
    }

    বর্গ FormalRefVisitor final : সার্বজনিক VNVisitor {
        ধ্রুব std::map<ধ্রুব AstVar*, AstVar*>& m_replacements;

        শূন্য visit(AstVarRef* nodep) override {
            ধ্রুব auto it = m_replacements.find(nodep->varp());
            যদি (it == m_replacements.end()) প্রত্যাগম;
            nodep->varp(it->second);
        }
        শূন্য visit(Ast_গ্রন্থি* nodep) override { iterateChildren(nodep); }

    সার্বজনিক:
        explicit FormalRefVisitor(ধ্রুব std::map<ধ্রুব AstVar*, AstVar*>& replacements)
            : m_replacements{replacements} {}
        শূন্য scan(Ast_গ্রন্থি* nodep) { iterate(nodep); }
    };

    শূন্য validateCovergroupExpressions() {
        std::set<ধ্রুব AstVar*> sampleMembers;
        std::set<ধ্রুব AstVar*> constructorRefMembers;
        প্রতি (Ast_গ্রন্থি* stmtp = m_constructorp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            ধ্রুব AstVar* ধ্রুব varp = VN_CAST(stmtp, Var);
            যদি (!varp || !varp->isIO() || (!varp->isRef() && !varp->isConstRef())) অনুবর্ত;
            ধ্রুব AstVar* ধ্রুব memberp
                = VN_CAST(m_memberMap.findMember(m_covergroupp, varp->name()), Var);
            UASSERT_OBJ(memberp && memberp->isClassMember(), varp,
                        "Covergroup constructor argument missing persistent member");
            constructorRefMembers.ঢোকাও(varp);
            constructorRefMembers.ঢোকাও(memberp);
        }
        প্রতি (Ast_গ্রন্থি* stmtp = m_sampleFuncp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            ধ্রুব AstVar* ধ্রুব varp = VN_CAST(stmtp, Var);
            যদি (!varp || !varp->isIO()) অনুবর্ত;
            ধ্রুব AstVar* ধ্রুব memberp
                = VN_CAST(m_memberMap.findMember(m_covergroupp, varp->name()), Var);
            UASSERT_OBJ(memberp && memberp->isClassMember(), varp,
                        "Covergroup sample argument missing persistent member");
            sampleMembers.ঢোকাও(memberp);
        }
        Covergroup_রাশিমালা_Valid_Visitor{sampleMembers, constructorRefMembers}.scan(m_constructorp);
    }

    শূন্য rebindFormalRefs() {
        std::map<ধ্রুব AstVar*, AstVar*> replacements;
        প্রতি (Ast_গ্রন্থি* stmtp = m_constructorp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            যদি (ধ্রুব AstVar* ধ্রুব varp = VN_CAST(stmtp, Var)) {
                যদি (!varp->isIO()) অনুবর্ত;
                AstVar* ধ্রুব memberp
                    = VN_CAST(m_memberMap.findMember(m_covergroupp, varp->name()), Var);
                UASSERT_OBJ(memberp && memberp->isClassMember(), varp,
                            "Covergroup constructor argument missing persistent member");
                replacements.emplace(varp, memberp);
            }
        }
        size_t expectedBindings = 0;
        প্রতি (ধ্রুব auto& pair : replacements) {
            যদি (pair.first->isRef() || pair.first->isConstRef()) ++expectedBindings;
        }
        size_t rewrittenBindings = 0;
        প্রতি (Ast_গ্রন্থি* stmtp = m_constructorp->stmtsp(); stmtp;) {
            Ast_গ্রন্থি* ধ্রুব nextp = stmtp->nextp();
            যদি (AstAssign* ধ্রুব assignp = VN_CAST(stmtp, Assign)) {
                AstVarRef* ধ্রুব lhsp = VN_CAST(assignp->lhsp(), VarRef);
                AstVarRef* ধ্রুব rhsp = VN_CAST(assignp->rhsp(), VarRef);
                যদি (lhsp && rhsp
                    && (lhsp->varp()->declDirection() == VDirection::REF
                        || lhsp->varp()->declDirection() == VDirection::CONSTREF)) {
                    ধ্রুব auto it = replacements.find(rhsp->varp());
                    UASSERT_OBJ(it != replacements.end() && it->second == lhsp->varp(), assignp,
                                "Unexpected covergroup reference binding assignment");
                    AstCExpr* ধ্রুব bindp = নূতন AstCExpr{assignp->fileline(), ""};
                    bindp->add(lhsp->unlinkFrBack());
                    bindp->add(" = &");
                    bindp->add(rhsp->unlinkFrBack());
                    assignp->replaceWith(bindp->makeStmt());
                    VL_DO_DANGLING(ঠেলো_Deletep(assignp), assignp);
                    ++rewrittenBindings;
                }
            }
            stmtp = nextp;
        }
        UASSERT_OBJ(rewrittenBindings == expectedBindings, m_constructorp,
                    "Covergroup reference argument missing binding");
        FormalRefVisitor visitor{replacements};
        প্রতি (AstCoverpoint* ধ্রুব cpp : m_coverpoints) visitor.scan(cpp);
        প্রতি (AstCoverCross* ধ্রুব crossp : m_coverCrosses) visitor.scan(crossp);
    }

    AstVarRef* installEnclosingBackPointer(ধ্রুব std::vector<Ast_গ্রন্থি_Assign*>& constructps) {
        // Simple-case support for embedded covergroups (IEEE 1800-2023 19.4) whose
        // coverpoints reference members of the enclosing class ("Class members can be used
        // in coverpoint expressions").  The covergroup is lowered into a sibling class with
        // no implicit handle to the enclosing object, so such references would emit
        // uncompilable C++.  Add an explicit back-pointer member to the enclosing instance,
        // route member references through it, and pass it into the constructor so
        // coverage initialization can read enclosing members. Returns an invalid
        // reference if an outer class member cannot be reached; otherwise returns an empty result.
        যদি (!m_enclosingClassp) প্রত্যাগম nullptr;  // Offending refs require an enclosing class

        AstVarRef* invalidp = nullptr;
        Ast_গ্রন্থি* offenderp = nullptr;
        std::set<ধ্রুব AstVar*> ownVars;
        প্রতি (Ast_গ্রন্থি* itemp = m_covergroupp->membersp(); itemp; itemp = itemp->nextp()) {
            যদি (ধ্রুব AstVar* ধ্রুব varp = VN_CAST(itemp, Var)) ownVars.ঢোকাও(varp);
        }
        ধ্রুব std::set<ধ্রুব AstVar*> enclosingVars = enclosingInstanceVars();
        std::vector<AstVarRef*> refsToRewrite;
        std::vector<AstThisRef*> thisRefsToRewrite;
        std::vector<AstFuncRef*> funcRefsToRewrite;
        ধ্রুব auto scan = [&](Ast_গ্রন্থি* rootp) {
            rootp->foreach([&](AstVarRef* refp) {
                যদি (invalidp) প্রত্যাগম;
                ধ্রুব AstVar* ধ্রুব varp = refp->varp();
                যদি (!isEnclosingInstanceVar(varp) || ownVars.গুনতি(varp)) প্রত্যাগম;
                যদি (!enclosingVars.গুনতি(varp)) {
                    invalidp = refp;
                    প্রত্যাগম;
                }
                refsToRewrite.ঠেলো_back(refp);
                যদি (!offenderp) offenderp = refp;
            });
            যদি (invalidp) প্রত্যাগম;
            rootp->foreach([&](AstThisRef* refp) {
                ধ্রুব AstClassRefDType* ধ্রুব refDTypep
                    = VN_CAST(refp->dtypep()->skipRefp(), ClassRefDType);
                যদি (refDTypep && refDTypep->classp() == m_covergroupp) {
                    thisRefsToRewrite.ঠেলো_back(refp);
                    যদি (!offenderp) offenderp = refp;
                }
            });
            rootp->foreach([&](AstFuncRef* refp) {
                যদি (!isEnclosingInstanceFunc(refp->taskp())) প্রত্যাগম;
                funcRefsToRewrite.ঠেলো_back(refp);
                যদি (!offenderp) offenderp = refp;
            });
        };
        প্রতি (AstCoverpoint* ধ্রুব cpp : m_coverpoints) scan(cpp);
        প্রতি (AstCoverCross* ধ্রুব crossp : m_coverCrosses) scan(crossp);
        প্রতি (AstCgOptionAssign* ধ্রুব optp : m_cgOptions) scan(optp);
        যদি (invalidp || !offenderp) প্রত্যাগম invalidp;

        UASSERT_OBJ(m_embeddedVarp, m_covergroupp, "Embedded covergroup variable not found");
        // Commit: add the back-pointer member, rewrite the references, initialize the handle.
        File_সারি* ধ্রুব fl = m_covergroupp->fileline();
        AstClassRefDType* ধ্রুব enclDTypep = নূতন AstClassRefDType{fl, m_enclosingClassp, nullptr};
        enclDTypep->rawPointer(সত্য);
        v3Global.rootp()->typeTablep()->addTypesp(enclDTypep);
        AstVar* ধ্রুব handleVarp
            = নূতন AstVar{fl, VVarType::MEMBER, "__Vcg_enclosingp", enclDTypep};
        m_covergroupp->addMembersp(handleVarp);
        AstVar* ধ্রুব argumentp = নূতন AstVar{fl, VVarType::BLOCKTEMP, "__Vcg_parentp", enclDTypep};
        argumentp->direction(VDirection::প্রবেশ);
        argumentp->declDirection(VDirection::প্রবেশ);
        argumentp->funcLocal(সত্য);
        argumentp->noReset(সত্য);
        argumentp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
        m_constructorp->addStmtsp(argumentp);
        m_constructorp->prependStmtsp(নূতন AstAssign{fl, memberRef(fl, handleVarp, VAccess::WRITE),
                                                    নূতন AstVarRef{fl, argumentp, VAccess::READ}});

        // Route each enclosing-member reference through the back-pointer: 'm' -> 'h.m'.
        প্রতি (AstVarRef* ধ্রুব refp : refsToRewrite) { rewriteVarRef(refp, handleVarp); }
        প্রতি (AstThisRef* ধ্রুব refp : thisRefsToRewrite) { rewriteThisRef(refp, handleVarp); }
        প্রতি (AstFuncRef* ধ্রুব refp : funcRefsToRewrite) { rewriteFuncRef(refp, handleVarp); }

        // Append a named hidden argument to preserve positional and defaulted user arguments.
        প্রতি (Ast_গ্রন্থি_Assign* ধ্রুব constructp : constructps) {
            File_সারি* ধ্রুব cfl = constructp->fileline();
            AstCExpr* ধ্রুব thisp = নূতন AstCExpr{cfl, "this"};
            thisp->dtypep(enclDTypep);
            VN_AS(constructp->rhsp(), New)->addArgsp(নূতন AstArg{cfl, argumentp->name(), thisp});
        }
        প্রত্যাগম nullptr;
    }

    শূন্য visit(AstClass* nodep) override {
        UINFO(9, "Visiting class: " << nodep->name() << " isCovergroup=" << nodep->isCovergroup());
        যদি (nodep->isCovergroup()) {
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
            Ast_Sen_বৃক্ষ* embeddedEventForkp = nullptr;

            // Extract and store the clocking event from AstCovergroup node
            // The parser creates this node to preserve the event information
            তার্কিক hasUnsupportedEvent = অসত্য;
            প্রতি (Ast_গ্রন্থি* itemp = nodep->membersp(); itemp;) {
                Ast_গ্রন্থি* ধ্রুব nextp = itemp->nextp();
                যদি (AstCovergroup* ধ্রুব cgp = VN_CAST(itemp, Covergroup)) {
                    // Store the event in the global map for V3Active to retrieve later
                    // V3LinkParse only creates this sentinel AstCovergroup node when a clocking
                    // event exists, so cgp->eventp() is always non-null here.
                    UASSERT_OBJ(cgp->eventp(), cgp,
                                "Sentinel AstCovergroup in class must have non-null eventp");
                    যদি (hasEnclosingEventRef(cgp)) {
                        UASSERT_OBJ(m_embeddedVarp, cgp,
                                    "Embedded covergroup event has no instance variable");
                        যদি (v3Global.opt.timing().isSetTrue()) {
                            embeddedEventForkp = cgp->eventp()->unlinkFrBack();
                        } অন্যথা {
                            embeddedEventTriggers = collectEmbeddedEventTriggers(cgp);
                            যদি (embeddedEventTriggers.empty()) {
                                cgp->v3warn(COVERIGN,
                                            "Unsupported: 'covergroup' clocking event on complex "
                                            "member expression; use --timing for full support.");
                                hasUnsupportedEvent = সত্য;
                            }
                        }
                        VL_DO_DANGLING(ঠেলো_Deletep(cgp->unlinkFrBack()), cgp);
                        itemp = nextp;
                        অনুবর্ত;
                    }
                    // V3Active handles events that do not depend on an enclosing instance.
                    UINFO(4, "Keeping covergroup event node for V3Active: " << nodep->name());
                    itemp = nextp;
                    অনুবর্ত;
                }
                itemp = nextp;
            }

            // Find the sample() method and constructor
            m_sampleFuncp = VN_CAST(m_memberMap.findMember(nodep, "sample"), Func);
            // V3LinkParse always synthesizes a sample() method for every covergroup, and the
            // sampling-code generation below dereferences m_sampleFuncp unconditionally.
            UASSERT_OBJ(m_sampleFuncp, nodep, "Covergroup missing synthesized sample() method");
            m_sampleFuncp->isCovergroupSample(সত্য);
            m_constructorp = VN_CAST(m_memberMap.findMember(nodep, "new"), Func);
            UINFO(9, "Found sample() method: " << (m_sampleFuncp ? "yes" : "no"));
            UINFO(9, "Found constructor: " << (m_constructorp ? "yes" : "no"));

            // If covergroup has unsupported clocking event, skip processing it
            // but still clean up coverpoints so they don't reach downstream passes
            যদি (hasUnsupportedEvent) {
                iterateChildren(nodep);
                validateCovergroupExpressions();
                rebindFormalRefs();
                delete_Coverage_বস্তুসমূহ();
                যদি (embeddedEventForkp) {
                    VL_DO_DANGLING(ঠেলো_Deletep(embeddedEventForkp), embeddedEventForkp);
                }
                প্রত্যাগম;
            }

            iterateChildren(nodep);
            validateCovergroupExpressions();
            rebindFormalRefs();
            ধ্রুব std::vector<Ast_গ্রন্থি_Assign*> constructps = findCovergroupConstructions();

            // Embedded covergroups (IEEE 1800-2023 19.4): coverpoints, iff expressions, and
            // crosses may reference members of the enclosing class. The covergroup is lowered
            // into a sibling class with no implicit handle to the enclosing instance. Install
            // an explicit back-pointer and route the references through it.
            যদি (AstVarRef* ধ্রুব invalidp = installEnclosingBackPointer(constructps)) {
                invalidp->v3error("Non-static member "
                                  << invalidp->varp()->prettyNameQ()
                                  << " of an outer class requires an explicit "
                                     "object handle (IEEE 1800-2023 8.23).");
                delete_Coverage_বস্তুসমূহ();
                যদি (embeddedEventForkp) {
                    VL_DO_DANGLING(ঠেলো_Deletep(embeddedEventForkp), embeddedEventForkp);
                }
                প্রত্যাগম;
            }
            installEmbeddedEventTriggers(embeddedEventTriggers, constructps);
            যদি (embeddedEventForkp) installEmbeddedEventFork(embeddedEventForkp, constructps);
            processCovergroup();
            // Remove lowered coverpoints/crosses from the class - they have been
            // fully translated into C++ code and must not reach downstream passes
            delete_Coverage_বস্তুসমূহ();
        } অন্যথা {
            // Track the lexically enclosing class so a nested covergroup can resolve
            // references to the enclosing object's members (installEnclosingBackPointer).
            VL_RESTORER(m_enclosingClassp);
            m_enclosingClassp = nodep;
            iterateChildren(nodep);
        }
    }

    শূন্য visit(AstCoverpoint* nodep) override {
        UINFO(9, "Found coverpoint: " << nodep->name());
        m_coverpoints.ঠেলো_back(nodep);
        m_coverpointMap.emplace(nodep->name(), nodep);
        iterateChildren(nodep);
    }

    শূন্য visit(AstCoverCross* nodep) override {
        UINFO(9, "Found cross: " << nodep->name());
        m_coverCrosses.ঠেলো_back(nodep);
        iterateChildren(nodep);
    }

    // V3Width leaves only the covergroup-level options lowerCovergroupOptions() takes
    শূন্য visit(AstCgOptionAssign* nodep) override { m_cgOptions.ঠেলো_back(nodep); }

    // A package, interface, or module, so the design unit declaring the classes within
    শূন্য visit(Ast_গ্রন্থি_Module* nodep) override {
        VL_RESTORER(m_unitp);
        m_unitp = nodep;
        iterateChildren(nodep);
    }

    শূন্য visit(Ast_গ্রন্থি* nodep) override { iterateChildren(nodep); }

সার্বজনিক:
    // CONSTRUCTORS
    FunctionalCoverageVisitor(AstNetlist* nodep,
                              ধ্রুব std::set<ধ্রুব AstVar*>& mergeableTypeOptions)
        : m_mergeableTypeOptions{mergeableTypeOptions} {
        iterate(nodep);
    }
    ~FunctionalCoverageVisitor() override = স্বভাব;
};

//######################################################################
// Functional coverage class functions

শূন্য V3Covergroup::covergroup(AstNetlist* nodep) {
    UINFO(4, __FUNCTION__ << ": ");
    ধ্রুব CovergroupAssignValidVisitor validVisitor{nodep};
    যদি (!validVisitor.valid()) V3Error::abortIfErrors();
    {  // Destruct before checking
        FunctionalCoverageVisitor{nodep, validVisitor.mergeableTypeOptions()};
    }
    V3Global::dump_Check_Global_বৃক্ষ("coveragefunc", 0, dump_বৃক্ষ_Either_Level() >= 3);
}
