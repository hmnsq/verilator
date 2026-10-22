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

#ضمن "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ضمن "V3Covergroup.h"

#ضمن "V3Const.h"
#ضمن "V3Error.h"
#ضمن "V3File.h"
#ضمن "V3MemberMap.h"
#ضمن "V3UniqueNames.h"

#ضمن <bitset>
#ضمن <cmath>
#ضمن <deque>
#ضمن <set>
#ضمن <tuple>
#ضمن <unordered_map>
#ضمن <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Embedded covergroup assignment validation; and in the same pass over the netlist, the
// type_option variables through which SystemVerilog may set type_option.merge_instances

صنف CovergroupAssignValidVisitor final : عام VNVisitorConst {
    VMemberMap m_memberMap;
    std::map<ثابت AstVar*, ثابت AstNodeFTask*>
        m_constructors;  // Implicit instance -> constructor
    std::set<ثابت AstVar*> m_mergeableTypeOptions;  // See mergeableTypeOptions()
    ثابت AstNodeFTask* m_ftaskp = nullptr;
    منطقي m_collecting = true;
    منطقي m_valid = true;

    فراغ visit(AstClass* nodep) override {
        VL_RESTORER(m_ftaskp);
        m_ftaskp = nullptr;
        𐤀𐤌 (m_collecting) {
            ثابت AstNodeFTask* ثابت constructorp
                = VN_CAST(m_memberMap.findMember(nodep, "new"), NodeFTask);
            لكل (ثابت AstNode* itemp = nodep->membersp(); itemp; itemp = itemp->nextp()) {
                ثابت AstVar* ثابت varp = VN_CAST(itemp, Var);
                // Only the implicit instance is restricted, not explicitly typed aliases.
                𐤀𐤌 (!varp || !varp->isClassMember() || varp->isDeclTyped()) استمر;
                ثابت AstClassRefDType* ثابت refp
                    = VN_CAST(varp->dtypep()->skipRefp(), ClassRefDType);
                𐤀𐤌 (refp && refp->classp()->covergroupEnclosingClassp() == nodep) {
                    m_constructors.emplace(varp, constructorp);
                }
            }
        }
        iterateChildrenConst(nodep);
    }
    فراغ visit(AstNodeFTask* nodep) override {
        VL_RESTORER(m_ftaskp);
        m_ftaskp = nodep;
        iterateChildrenConst(nodep);
    }
    فراغ visit(AstNodeAssign* nodep) override {
        𐤀𐤌 (!m_collecting) {
            ثابت AstVar* varp = nullptr;
            𐤀𐤌 (ثابت AstNodeVarRef* ثابت refp = VN_CAST(nodep->lhsp(), NodeVarRef)) {
                varp = refp->varp();
            } وإلا 𐤀𐤌 (ثابت AstMemberSel* ثابت selp = VN_CAST(nodep->lhsp(), MemberSel)) {
                varp = selp->varp();
            }
            ثابت auto it = m_constructors.find(varp);
            𐤀𐤌 (it != m_constructors.end() && (!m_ftaskp || m_ftaskp != it->second)) {
                m_valid = خطأ;
                nodep->v3error("Embedded covergroup variable "
                               << varp->prettyNameQ()
                               << " may only be assigned in the enclosing class's 'new' method "
                                  "(IEEE 1800-2023 19.4).");
            }
        }
        iterateChildrenConst(nodep);
    }
    فراغ visit(AstNodeVarRef* nodep) override {
        // Type options may be set at any time (IEEE 1800-2023 19.7.1), so SystemVerilog may set
        // type_option.merge_instances with any use of a type_option but a read of another
        // member, as not every write is marked yet (std::randomize)
        𐤀𐤌 (m_collecting && nodep->varp()->name() == "type_option") {
            ثابت AstStructSel* ثابت selp = VN_CAST(nodep->backp(), StructSel);
            𐤀𐤌 (!selp || selp->fromp() != nodep || selp->name() == "merge_instances"
                || !nodep->access().isReadOnly()) {
                m_mergeableTypeOptions.insert(nodep->varp());
            }
        }
        iterateChildrenConst(nodep);
    }
    فراغ visit(AstNode* nodep) override { iterateChildrenConst(nodep); }

عام:
    explicit CovergroupAssignValidVisitor(AstNetlist* nodep) {
        // Uses can precede their enclosing class in the tree.
        iterateConst(nodep);
        m_collecting = خطأ;
        𐤀𐤌 (!m_constructors.empty()) iterateConst(nodep);
    }
    منطقي valid() ثابت { أرجع m_valid; }
    // type_option variables through which SystemVerilog may set type_option.merge_instances, so
    // that covergroups are tested with optionVar(true)
    ثابت std::set<ثابت AstVar*>& mergeableTypeOptions() ثابت { أرجع m_mergeableTypeOptions; }
};

//######################################################################
// Covergroup expression validation visitor

صنف CovergroupExprValidVisitor final : عام VNVisitor {
    ثابت std::set<ثابت AstVar*>& m_sampleMembers;
    ثابت std::set<ثابت AstVar*>& m_constructorRefMembers;
    منطقي m_inCoverageExpression = خطأ;
    منطقي m_sampleFormalAllowed = خطأ;

    فراغ scanSampleExpression(AstNode* nodep) {
        𐤀𐤌 (!nodep) أرجع;
        VL_RESTORER(m_sampleFormalAllowed);
        m_sampleFormalAllowed = true;
        iterateAndNextNull(nodep);
    }

    فراغ scanCoverageExpression(AstNode* nodep) {
        𐤀𐤌 (!nodep) أرجع;
        VL_RESTORER(m_inCoverageExpression);
        m_inCoverageExpression = true;
        iterateAndNextNull(nodep);
    }

    فراغ visit(AstCoverpoint* nodep) override {
        scanSampleExpression(nodep->exprp());
        scanSampleExpression(nodep->iffp());
        iterateAndNextNull(nodep->binsp());
        iterateAndNextNull(nodep->optionsp());
    }
    فراغ visit(AstCoverCross* nodep) override {
        iterateAndNextNull(nodep->itemsp());
        scanSampleExpression(nodep->iffp());
        iterateAndNextNull(nodep->optionsp());
        iterateAndNextNull(nodep->binsp());
    }
    فراغ visit(AstCoverCrossBin* nodep) override {
        iterateAndNextNull(nodep->selectp());
        scanSampleExpression(nodep->iffp());
    }
    فراغ visit(AstCoverCrossWith* nodep) override {
        iterateAndNextNull(nodep->selectp());
        scanCoverageExpression(nodep->filterp());
        scanCoverageExpression(nodep->countp());
    }
    فراغ visit(AstCoverBinsof* nodep) override { scanCoverageExpression(nodep->rangesp()); }
    فراغ visit(AstCoverBin* nodep) override {
        scanCoverageExpression(nodep->rangesp());
        scanSampleExpression(nodep->iffp());
        scanCoverageExpression(nodep->arraySizep());
        scanCoverageExpression(nodep->transp());
    }
    فراغ visit(AstVarRef* nodep) override {
        𐤀𐤌 (!m_sampleFormalAllowed && m_sampleMembers.count(nodep->varp())) {
            nodep->v3error("Covergroup sample formal argument "
                           << nodep->varp()->prettyNameQ()
                           << " may only be used in a coverpoint or conditional guard "
                              "expression (IEEE 1800-2023 19.8.1).");
        }
        𐤀𐤌 (m_inCoverageExpression && m_constructorRefMembers.count(nodep->varp())) {
            nodep->v3error("Ref covergroup constructor formal argument "
                           << nodep->varp()->prettyNameQ()
                           << " may not be used in a covergroup expression "
                              "(IEEE 1800-2023 19.5).");
        }
    }
    فراغ visit(AstNodeFTaskRef* nodep) override {
        𐤀𐤌 (m_inCoverageExpression && nodep->taskp()) {
            منطقي invalidDirection = خطأ;
            لكل (AstNode* stmtp = nodep->taskp()->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
                ثابت AstVar* ثابت varp = VN_CAST(stmtp, Var);
                𐤀𐤌 (varp && varp->isIO() && varp->isWritable()) {
                    invalidDirection = true;
                    اكسر;
                }
            }
            𐤀𐤌 (invalidDirection) {
                nodep->v3error("Function " << nodep->taskp()->prettyNameQ()
                                           << " called in a covergroup expression has an "
                                              "output, inout, or non-const ref argument "
                                              "(IEEE 1800-2023 19.5).");
            }
        }
        iterateChildren(nodep);
    }
    فراغ visit(AstNode* nodep) override { iterateChildren(nodep); }

عام:
    CovergroupExprValidVisitor(ثابت std::set<ثابت AstVar*>& sampleMembers,
                               ثابت std::set<ثابت AstVar*>& constructorRefMembers)
        : m_sampleMembers{sampleMembers}
        , m_constructorRefMembers{constructorRefMembers} {}
    فراغ scan(AstNode* nodep) { iterate(nodep); }
};

//######################################################################
// Bins of one declaration whose values are computed rather than listed: bin k covers
// [m_lo + k * m_stride, m_lo + (k + 1) * m_stride - 1], and the last bin extends to m_hi.  An
// array bin element is a run of single-value bins; automatic bins partition the coverpoint
// domain.  Bounds are coverpoint values at FunctionalCoverageVisitor::runWidth(),
// sign-extended like a CrossValueRange's.

صنف BinRun final {
عام:
    // MEMBERS
    uint32_t m_count;  // Number of bins
    V3Number m_lo;  // Lowest value of the first bin
    V3Number m_stride;  // Number of values of each bin but the last
    V3Number m_hi;  // Highest value of the last bin
    منطقي m_empty = خطأ;  // A single bin without a value of the coverpoint type
    uint32_t m_declared = 0;  // Runtime index of the first bin, once generated

    // CONSTRUCTORS
    BinRun(AstNode* nodep, صحيح width, uint32_t count)
        : m_count{count}
        , m_lo{nodep, width}
        , m_stride{nodep, width, 1}
        , m_hi{nodep, width} {}
};

//######################################################################
// Functional coverage visitor

صنف FunctionalCoverageVisitor final : عام VNVisitor {
    // NODE STATE
    // Entire netlist:
    //  AstCoverpoint::user1p()  -> AstVar*.  Previous-value variable for transition bins
    //  AstCoverpoint::user2()   -> bool.  Had a bins declaration ignored, so no automatic bins
    ثابت VNUser1InUse m_inuser1;
    ثابت VNUser2InUse m_inuser2;

    // STATE
    std::set<AstCoverpoint*>
        m_runtimePoints;  // Points needing value metadata and live-bin mapping
    std::set<AstCoverCross*> m_runtimeCrosses;  // Crosses over finalized live-bin dimensions
    std::map<AstVar*, AstVar*> m_excludedVars;  // Sample-time state-exclusion flags
    AstClass* m_covergroupp = nullptr;  // Current covergroup being processed
    AstNodeModule* m_unitp = nullptr;  // Design unit declaring the current class, outside classes
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
    منطقي m_cgMergeInstances = خطأ;  // The covergroup's type_option.merge_instances, a constant
    // The covergroup's type_option.merge_instances is, or SystemVerilog may make it, true
    منطقي m_cgMayMerge = خطأ;
    // type_option members through which SystemVerilog may set type_option.merge_instances,
    // see CovergroupAssignValidVisitor::mergeableTypeOptions()
    ثابت std::set<ثابت AstVar*>& m_mergeableTypeOptions;
    صحيح m_cgAtLeast = 1;  // Covergroup option.at_least

    بنية EmbeddedEventTrigger final {
        FileLine* eventFl;  // Clocking-event source location
        AstVar* baseVarp;  // Base enclosing-class member in the event expression
        AstVar* memberVarp;  // Selected member in a 'base.member' expression, or nullptr
        VEdgeType edgeType;  // Clocking-event edge qualifier
        AstVar* prevVarp;  // Member containing the previous event value, or nullptr
        EmbeddedEventTrigger(FileLine* eventFl, AstVar* baseVarp, AstVar* memberVarp,
                             VEdgeType edgeType)
            : eventFl{eventFl}
            , baseVarp{baseVarp}
            , memberVarp{memberVarp}
            , edgeType{edgeType}
            , prevVarp{nullptr} {}
    };

    std::set<std::string> m_crossedCpNames;  // Coverpoints referenced by a cross
    std::map<std::string, AstVar*> m_cpVarMap;  // Coverpoint name -> its VlCoverpoint member
    بنية BinRuns final {
        std::vector<BinRun> runs;  // Runs of an array or automatic bins declaration, in order
        uint32_t count = 0;  // Bins across all runs
        منطقي unsupported = خطأ;  // Too many bins, or invalid: the declaration is ignored
        // The value of each bin, which names it, of a wildcard array; else the bins are indexed
        std::vector<std::string> values;
        // Too many values of an ignore or illegal wildcard array, which is then one bin
        منطقي single = خطأ;
    };
    بنية CrossBinValues final {
        AstCoverBin* binp;  // Declaration owning this Normal bin
        AstNodeExpr* valuep;  // Individual array-bin value, or nullptr for a scalar bin
        ثابت BinRun* runp = nullptr;  // Run computing the bin's values, if any
        uint32_t element = 0;  // Index of the bin within runp
    };
    بنية BinSpan final {
        uint32_t first = 0;  // First Normal index of the bin declaration
        uint32_t count = 0;  // Number of Normal bins of the declaration
        uint32_t declared = 0;  // First runtime bin index, across all bin kinds
        int32_t sized = -1;  // Index of the sized array, placed at construction; or -1
    };
    بنية CoverpointBins final {
        uint32_t total = 0;  // Number of Normal bins
        AstNodeExpr* exprp = nullptr;  // Sampled expression, for the value domain
        منطقي crossed = خطأ;  // Feeds a cross, which needs 'values'
        std::vector<CrossBinValues> values;  // Values in runtime Normal-bin index order
        std::unordered_map<std::string, BinSpan> spans;  // Declared bin name -> index span
        BinSpan implicitAuto{0, 0, 0};  // Implicit automatic bins, each named 'auto_<i>'
        std::deque<BinRun> runs;  // Runs 'values' refers to
    };
    std::map<AstVar*, CoverpointBins> m_cpBins;  // Runtime coverpoint -> binsof index ranges
    // Names of the bins declarations each coverpoint ignored, which binsof selects as no bins
    std::map<ثابت AstCoverpoint*, std::vector<std::string>> m_droppedBins;
    // Prefixes of the constructor temporaries of constructed bins, as coverpoint and bin names
    // alone may repeat: coverpoint 'a_' bins 'b', and coverpoint 'a' bins '_b'
    V3UniqueNames m_sizedNames{"__Vsized"};
    std::vector<AstNodeExpr*> m_detachedValues;  // Array-bin values m_cpBins refers to
    std::set<AstCoverCross*>
        m_droppedCrosses;  // Crosses with a bare-variable item: drop (COVERIGN)
    std::map<uint32_t, AstCoverpointDType*> m_cpDTypes;  // Hit-list bound -> interned dtype
    using CrossShape = std::tuple<uint32_t, uint32_t, uint32_t, uint32_t, uint64_t, منطقي>;
    std::map<CrossShape, AstCoverCrossDType*> m_cxDTypes;
    AstVar* m_cgInstVarp = nullptr;  // __Vcg_inst handle member of the current covergroup

    VMemberMap m_memberMap;  // Member names cached for fast lookup

    // METHODS
    // The covergroup's 'option' or static 'type_option' member (V3LinkParse creates both)
    AstVar* optionVar(منطقي typeOption) {
        AstVar* ثابت varp = VN_AS(
            m_memberMap.findMember(m_covergroupp, typeOption ? "type_option" : "option"), Var);
        UASSERT_OBJ(varp, m_covergroupp, "Covergroup missing option member");
        أرجع varp;
    }

    // 'option.<member>' or 'type_option.<member>', per optionVarp
    AstStructSel* newOptionSel(FileLine* fl, AstVar* optionVarp, ثابت std::string& member,
                               VAccess access) {
        ثابت AstMemberDType* ثابت memberp
            = VN_AS(m_memberMap.findMember(optionVarp->dtypep()->skipRefp(), member), MemberDType);
        UASSERT_OBJ(memberp, optionVarp, "Coverage option structure missing '" << member << "'");
        AstNodeExpr* ثابت fromp = optionVarp->lifetime().isStatic()
                                       ? جديد AstVarRef{fl, optionVarp, access}
                                       : memberRef(fl, optionVarp, access);
        AstStructSel* ثابت selp = جديد AstStructSel{fl, fromp, member};
        selp->dtypep(memberp->subDTypep()->skipRefToEnump());
        selp->didWidth(true);
        أرجع selp;
    }

    // A constant option.at_least; a negative count of hits has no meaning, so takes the default
    ساكن صحيح constAtLeast(ثابت AstConst* constp) {
        ثابت صحيح value = constp->toSInt();
        𐤀𐤌 (value >= 0) أرجع value;
        constp->v3warn(COVERIGN, "Ignoring negative 'option.at_least'; using 1");
        أرجع 1;
    }

    // An option's constant value, or nullptr after a warning, as non-constant is unsupported
    ساكن ثابت AstConst* constOption(ثابت AstNode* valuep, VCoverOptionType optType) {
        𐤀𐤌 (ثابت AstConst* ثابت constp = VN_CAST(valuep, Const)) أرجع constp;
        valuep->v3warn(COVERIGN, "Ignoring unsupported: non-constant 'option."
                                     << optType.ascii() << "'; using default value");
        أرجع nullptr;
    }

    // Store the covergroup-level options (IEEE 1800-2023 19.7) where SystemVerilog and the
    // runtime read them.  The instance options, option.weight and option.get_inst_coverage,
    // are evaluated by the constructor, as are the other instance options; the type options,
    // type_option.weight and type_option.merge_instances, are constant, and initialize the
    // static member.  Without its own type_option.merge_instances, a covergroup has the
    // default of --coverage-merge-instances.  A constant option.at_least is the default of
    // the coverpoints and crosses.
    فراغ lowerCovergroupOptions() {
        منطقي mergeSet = خطأ;  // The covergroup sets type_option.merge_instances
        m_cgAtLeast = 1;
        لكل (AstCgOptionAssign* ثابت optp : m_cgOptions) {
            𐤀𐤌 (optp->optType() == VCoverOptionType::AT_LEAST) {
                𐤀𐤌 (ثابت AstConst* ثابت constp = constOption(optp->valuep(), optp->optType())) {
                    m_cgAtLeast = constAtLeast(constp);
                }
                VL_DO_DANGLING(pushDeletep(optp->unlinkFrBack()), optp);
                استمر;
            }
            FileLine* ثابت fl = optp->fileline();
            // V3Width left the type options constant, and type_option.weight non-negative
            ثابت AstConst* ثابت constp = VN_CAST(optp->valuep(), Const);
            std::string member = "weight";
            𐤀𐤌 (optp->optType() == VCoverOptionType::MERGE_INSTANCES) {
                member = "merge_instances";
                m_cgMergeInstances = !constp->num().isEqZero();
                mergeSet = true;
            } وإلا 𐤀𐤌 (optp->optType() == VCoverOptionType::GET_INST_COVERAGE) {
                member = "get_inst_coverage";
            } وإلا {
                UASSERT_OBJ(optp->optType() == VCoverOptionType::WEIGHT, optp,
                            "Unexpected covergroup option reaching V3Covergroup");
                𐤀𐤌 (optp->typeOption()) m_cgTypeWeight = constp->toUInt();
            }
            AstAssign* ثابت assignp = جديد AstAssign{
                fl, newOptionSel(fl, optionVar(optp->typeOption()), member, VAccess::WRITE),
                optp->valuep()->unlinkFrBack()};
            𐤀𐤌 (optp->typeOption()) {
                m_covergroupp->addMembersp(جديد AstInitialStatic{fl, assignp});
                VL_DO_DANGLING(pushDeletep(optp->unlinkFrBack()), optp);
            } وإلا {
                optp->replaceWith(assignp);
                VL_DO_DANGLING(pushDeletep(optp), optp);
            }
        }
        m_cgOptions.clear();
        𐤀𐤌 (!mergeSet && v3Global.opt.coverageMergeInstances()) {
            FileLine* ثابت fl = m_covergroupp->fileline();
            m_covergroupp->addMembersp(جديد AstInitialStatic{
                fl, جديد AstAssign{
                        fl, newOptionSel(fl, optionVar(true), "merge_instances", VAccess::WRITE),
                        جديد AstConst{fl, AstConst::BitTrue{}}}});
            m_cgMergeInstances = true;
        }
        m_cgMayMerge = m_cgMergeInstances || m_mergeableTypeOptions.count(optionVar(true));
    }

    // The weight of an item in the coverage database, which merges the instances: its
    // type_option.weight if the covergroup merges them too (IEEE 1800-2023 19.11.3); else its
    // option.weight if a constant, and so of every instance; else its type_option.weight
    uint32_t itemDatabaseWeight(AstNode* optionsp) ثابت {
        ثابت AstNodeExpr* weightp = nullptr;  // The option.weight in effect
        uint32_t typeWeight = 1;
        لكل (AstNode* nodep = optionsp; nodep; nodep = nodep->nextp()) {
            ثابت AstCoverOption* ثابت optp = VN_AS(nodep, CoverOption);
            𐤀𐤌 (!(optp->optType() == VCoverOptionType::WEIGHT)) استمر;
            // V3Width left type_option.weight a non-negative constant
            𐤀𐤌 (optp->typeOption()) {
                typeWeight = VN_AS(optp->valuep(), Const)->toUInt();
            } وإلا {
                weightp = optp->valuep();
            }
        }
        𐤀𐤌 (m_cgMergeInstances) أرجع typeWeight;
        𐤀𐤌 (!weightp) أرجع 1;
        𐤀𐤌 (ثابت AstConst* ثابت constp = VN_CAST(weightp, Const)) أرجع constp->toUInt();
        أرجع typeWeight;
    }

    // Configure an item's option.weight, its weight in instance coverage (IEEE 1800-2023
    // 19.11), and, if the covergroup may merge its instances, its type_option.weight, its
    // weight in type coverage then (19.11.3).
    فراغ generateItemWeight(FileLine* fl, AstVar* itemVarp, AstNode* optionsp) {
        لكل (AstNode* nodep = optionsp; nodep; nodep = nodep->nextp()) {
            ثابت AstCoverOption* ثابت optp = VN_AS(nodep, CoverOption);
            𐤀𐤌 (!(optp->optType() == VCoverOptionType::WEIGHT)) استمر;
            𐤀𐤌 (!optp->typeOption()) {
                m_constructorp->addStmtsp(
                    itemCall(fl, itemVarp, VCMethod::COVERGROUP_WEIGHT,
                             {optp->valuep()->cloneTree(خطأ), fileLineDebug(optp->fileline())})
                        ->makeStmt());
            } وإلا 𐤀𐤌 (m_cgMayMerge) {
                // V3Width left type_option.weight a non-negative constant
                m_constructorp->addStmtsp(itemCall(fl, itemVarp, VCMethod::COVERGROUP_TYPE_WEIGHT,
                                                   {optp->valuep()->cloneTree(خطأ)})
                                              ->makeStmt());
            }
        }
    }

    فراغ processCovergroup() {
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
        m_cgMergeInstances = خطأ;
        m_cgMayMerge = خطأ;

        lowerCovergroupOptions();

        // Scan every cross item to record the coverpoints it references (the cross dimensions)
        // and to flag any cross naming a bare variable -- a would-be implicit coverpoint, which
        // Verilator does not synthesize.  An unresolvable item drops only that one cross (with a
        // COVERIGN in generateCrossCode), leaving the rest of the covergroup intact.
        لكل (AstCoverCross* crossp : m_coverCrosses) {
            لكل (AstNode* itemp = crossp->itemsp(); itemp; itemp = itemp->nextp()) {
                ثابت AstCoverpointRef* ثابت refp = VN_AS(itemp, CoverpointRef);
                𐤀𐤌 (refp->exprp()) استمر;  // hierarchical ref: dropped in generateCrossCode
                𐤀𐤌 (m_coverpointMap.find(refp->name()) == m_coverpointMap.end()) {
                    m_droppedCrosses.insert(crossp);  // bare variable: drop this cross only
                } وإلا {
                    m_crossedCpNames.insert(refp->name());
                }
            }
        }

        std::vector<AstNode*> pending;
        std::map<AstCoverpoint*, std::vector<AstCoverCross*>> consumers;
        std::map<AstCoverCross*, std::vector<AstCoverpoint*>> inputs;
        لكل (AstCoverpoint* ثابت cpp : m_coverpoints) {
            checkBinNames(cpp);
            checkConstructedBins(cpp);
            𐤀𐤌 (!cpp->exprp()->dtypep()->skipRefp()->isIntegralOrPacked()) استمر;
            // Bins without values leave the report (IEEE 1800-2023 19.11.1), exclusions or not.
            // Constructed bins get their values when the covergroup is constructed.
            𐤀𐤌 (!coverpointHasStateExclusions(cpp) && !coverpointHasEmptyBins(cpp)
                && !coverpointHasConstructedBins(cpp)) {
                استمر;
            }
            m_runtimePoints.insert(cpp);
            pending.push_back(cpp);
        }
        لكل (AstCoverCross* ثابت crossp : m_coverCrosses) {
            𐤀𐤌 (m_droppedCrosses.count(crossp)) استمر;
            لكل (AstNode* itemp = crossp->itemsp(); itemp; itemp = itemp->nextp()) {
                ثابت AstCoverpointRef* ثابت refp = VN_AS(itemp, CoverpointRef);
                𐤀𐤌 (refp->exprp()) استمر;
                ثابت auto point = m_coverpointMap.find(refp->name());
                𐤀𐤌 (point != m_coverpointMap.end()) {
                    consumers[point->second].push_back(crossp);
                    inputs[crossp].push_back(point->second);
                }
            }
            // The values of a filter's bin tuples are known when the covergroup is constructed
            𐤀𐤌 (crossp->exists([](ثابت AstCoverCrossWith*) { أرجع true; })) {
                m_runtimeCrosses.emplace(crossp);
                pending.push_back(crossp);
            }
        }
        لكل (size_t next = 0; next < pending.size(); ++next) {
            𐤀𐤌 (AstCoverpoint* ثابت pointp = VN_CAST(pending[next], Coverpoint)) {
                لكل (AstCoverCross* ثابت crossp : consumers[pointp]) {
                    𐤀𐤌 (m_runtimeCrosses.emplace(crossp).second) pending.push_back(crossp);
                }
            } وإلا {
                لكل (AstCoverpoint* ثابت pointp : inputs[VN_AS(pending[next], CoverCross)]) {
                    𐤀𐤌 (pointp->exprp()->dtypep()->skipRefp()->isIntegralOrPacked()
                        && m_runtimePoints.emplace(pointp).second) {
                        pending.push_back(pointp);
                    }
                }
            }
        }

        // The instance node owns this instance's coverpoint/cross runtimes, so it must exist
        // before any of them is created.  Emitted first, ahead of both generate loops.
        generateInstanceAttach();

        // For each coverpoint, generate sampling code
        لكل (AstCoverpoint* cpp : m_coverpoints) generateCoverpointCode(cpp);

        // For each cross, generate sampling code
        لكل (AstCoverCross* crossp : m_coverCrosses) generateCrossCode(crossp);

        // Every cross has been built, so runtime points only need their exclusions from here.
        لكل (AstCoverpoint* ثابت cpp : m_coverpoints) {
            𐤀𐤌 (!m_runtimePoints.count(cpp)) استمر;
            m_constructorp->addStmtsp(itemCall(cpp->fileline(), m_cpVarMap.at(cpp->name()),
                                               VCMethod::COVERGROUP_VALUE_RELEASE)
                                          ->makeStmt());
        }
        لكل (AstNodeExpr* valuep : m_detachedValues) VL_DO_DANGLING(pushDeletep(valuep), valuep);
        m_detachedValues.clear();

        // Generate coverage computation code (even for empty covergroups).  Bin registration
        // with the coverage database is handled per coverpoint/cross by their runtime
        // registerBins() calls (emitted in generateCoverpoint/generateCross).
        generateCoverageComputationCode();
    }

    ساكن constexpr size_t VALUE_LIST_ENTRIES = 256;  // Metadata entries per constructor call

    // The value of a constant count, of bins or value tuples: -1 if it is negative, and
    // saturated above the largest limit
    ساكن int64_t constCount(ثابت AstConst* constp) {
        ثابت V3Number& num = constp->num();
        𐤀𐤌 (constp->isSigned() && num.isNegative()) أرجع -1;
        أرجع num.mostSetBitP1() > 32 ? INT64_MAX : static_cast<int64_t>(num.toUQuad());
    }

    // Whether a count, 'whatp', is integral and, if constant, positive, as the standard's clause
    // 'clausep' requires; else errors
    ساكن منطقي checkCount(ثابت AstNodeExpr* countp, ثابت حرف* whatp, ثابت حرف* clausep) {
        ثابت AstConst* ثابت constp = VN_CAST(countp, Const);
        𐤀𐤌 (!countp->dtypep()->skipRefp()->isIntegralOrPacked()) {
            countp->v3error(whatp << " must be integral (IEEE 1800-2023 " << clausep << ")");
            أرجع خطأ;
        } وإلا 𐤀𐤌 (constp && (constp->num().isFourState() || constCount(constp) < 1)) {
            countp->v3error(whatp << " must be >= 1, got "
                                  << (constp->num().isFourState() ? constp->num().ascii(خطأ)
                                                                  : constp->num().toDecimalS())
                                  << " (IEEE 1800-2023 " << clausep << ")");
            أرجع خطأ;
        }
        أرجع true;
    }

    // The number of bins requested by a valid 'bins auto[N]', or 0
    ساكن uint32_t autoBinsRequested(ثابت AstCoverBin* binp) {
        ثابت AstConst* ثابت constp = VN_CAST(binp->arraySizep(), Const);
        𐤀𐤌 (!constp) أرجع 0;
        ثابت int64_t count = constCount(constp);
        أرجع count < 1 || count > v3Global.opt.coverageMaxBins() ? 0
                                                                   : static_cast<uint32_t>(count);
    }

    // True for a 'bins auto[N]' declaration, or the implicit automatic bins of a coverpoint
    ساكن منطقي isAutoBins(ثابت AstCoverBin* binp) {
        أرجع binp->binsType() == VCoverBinsType::BINS_AUTO
               || binp->binsType() == VCoverBinsType::BINS_AUTO_IMPLICIT;
    }

    // True for a sized array of bins, 'bins b[N] = {...}', whose values an integral coverpoint
    // distributes over N bins when the covergroup is constructed (IEEE 1800-2023 19.5.1)
    ساكن منطقي isSizedArray(ثابت AstCoverBin* binp) {
        أرجع binp->arraySizep() && !isAutoBins(binp);
    }

    // The 'with' filter of a bin, if any (IEEE 1800-2023 19.5.1.1)
    ساكن AstCoverWith* binWith(ثابت AstCoverBin* binp) {
        أرجع VN_CAST(binp->rangesp(), CoverWith);
    }

    // True for bins whose values the covergroup constructor computes: a sized array, or bins of
    // the values a 'with' filter keeps
    ساكن منطقي isConstructedBins(ثابت AstCoverBin* binp) {
        أرجع isSizedArray(binp) || binWith(binp);
    }

    // The range list of a bin's values, or of the candidates of its 'with' filter; null for all
    // of the coverpoint's values, which a filter of the coverpoint's name has
    ساكن AstNode* binRangesp(ثابت AstCoverBin* binp) {
        ثابت AstCoverWith* ثابت withp = binWith(binp);
        𐤀𐤌 (!withp) أرجع binp->rangesp();
        أرجع VN_IS(withp->subp(), CoverpointRef) ? nullptr : withp->subp();
    }

    // Report and delete a bins declaration of a name that another of its coverpoint has
    فراغ checkBinNames(AstCoverpoint* coverpointp) {
        std::set<std::string> names;
        لكل (AstNode* nodep = coverpointp->binsp(); nodep;) {
            AstCoverBin* ثابت binp = VN_AS(nodep, CoverBin);
            nodep = nodep->nextp();
            𐤀𐤌 (names.emplace(binp->name()).second) استمر;
            binp->v3error("Duplicate bin " << binp->prettyNameQ() << " in coverpoint "
                                           << coverpointp->prettyNameQ()
                                           << " (IEEE 1800-2023 3.13)");
            VL_DO_DANGLING(pushDeletep(binp->unlinkFrBack()), binp);
        }
    }

    // Delete an ignored bins declaration, which binsof then selects as no bins
    فراغ dropBins(ثابت AstCoverpoint* coverpointp, AstCoverBin* binp) {
        m_droppedBins[coverpointp].push_back(binp->name());
        VL_DO_DANGLING(pushDeletep(binp->unlinkFrBack()), binp);
    }

    // Check the size of a sized array of bins, which drops an invalid array.  A real coverpoint's
    // are unsupported, and treated as arrays of a bin per value.  False unless it stays sized.
    منطقي checkBinsArraySize(ثابت AstCoverpoint* coverpointp, AstCoverBin* binp, منطقي integral) {
        AstNodeExpr* ثابت sizep = binp->arraySizep();
        𐤀𐤌 (VN_IS(sizep, Unbounded)) {  // A parameter of '$'; see bins_orBraE
            binp->v3error("Bins array size must be integral, not '$' (IEEE 1800-2023 19.5.1)");
        } وإلا 𐤀𐤌 (checkCount(sizep, "Bins array size", "19.5.1")) {
            𐤀𐤌 (integral) أرجع true;
            binp->v3warn(COVERIGN, "Unsupported: 'bins' explicit array size of a real "
                                   "coverpoint (treated as '[]')");
            VL_DO_DANGLING(pushDeletep(sizep->unlinkFrBack()), sizep);
            أرجع خطأ;
        }
        dropBins(coverpointp, binp);
        أرجع خطأ;
    }

    // Check the bins of a coverpoint whose values the constructor computes, dropping invalid
    // ones.  A wildcard array of too many ranges of values is ignored, or if ignore or illegal,
    // treated as one bin; filtering too many ranges of values ignores the bins.
    فراغ checkConstructedBins(AstCoverpoint* coverpointp) {
        ثابت منطقي integral = coverpointp->exprp()->dtypep()->skipRefp()->isIntegralOrPacked();
        لكل (AstNode* nodep = coverpointp->binsp(); nodep;) {
            AstCoverBin* ثابت binp = VN_AS(nodep, CoverBin);
            nodep = nodep->nextp();
            𐤀𐤌 (!isConstructedBins(binp)) استمر;
            𐤀𐤌 (isSizedArray(binp) && !checkBinsArraySize(coverpointp, binp, integral)) استمر;
            𐤀𐤌 (!binp->isWildcard()
                || sizedWildcardRuns(binp, coverpointp->exprp())
                       <= v3Global.opt.coverageMaxBins()) {
                𐤀𐤌 (binWith(binp) && withCandidatesOver(binp, coverpointp->exprp())) {
                    binp->v3warn(COVERIGN, "Unsupported: 'with' filter of more than 2**32 "
                                           "candidate values; bin "
                                               << binp->prettyNameQ() << " ignored");
                    𐤀𐤌 (binp->binsType().binIsNormal()) coverpointp->user2(true);
                    dropBins(coverpointp, binp);
                }
                استمر;
            }
            // An ignore or illegal array still excludes or checks its values, as one bin
            ثابت منطقي single = !binp->binsType().binIsNormal() && !binWith(binp);
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
            𐤀𐤌 (single) {
                AstNodeExpr* ثابت sizep = binp->arraySizep();
                VL_DO_DANGLING(pushDeletep(sizep->unlinkFrBack()), sizep);
                binp->isArray(خطأ);
                استمر;
            }
            𐤀𐤌 (binp->binsType().binIsNormal()) coverpointp->user2(true);
            dropBins(coverpointp, binp);
        }
    }

    // True if a 'with' filter would be evaluated for more than 2**32 candidate values, known
    // now for the coverpoint's name, or for a range list of constants: each is evaluated once
    // but for an array 'b[N]', which keeps their order and duplicates (see withBegin())
    ساكن منطقي withCandidatesOver(AstCoverBin* binp, AstNodeExpr* exprp) {
        ثابت صحيح width = runWidth(exprp);
        std::vector<std::pair<V3Number, V3Number>> runs;
        𐤀𐤌 (!binRangesp(binp)) runs = coverpointValues(binp, exprp);
        ثابت auto constant
            = [](ثابت AstNode* nodep) { أرجع VN_IS(nodep, Const) || VN_IS(nodep, Unbounded); };
        لكل (AstNode* rangep = binRangesp(binp); rangep; rangep = rangep->nextp()) {
            ثابت AstInsideRange* ثابت irp = VN_CAST(rangep, InsideRange);
            // Else known when constructed
            𐤀𐤌 (irp ? !constant(irp->lhsp()) || !constant(irp->rhsp()) : !VN_IS(rangep, Const)) {
                أرجع خطأ;
            }
            CrossValueRange range{rangep, resolveWidth(rangep, exprp)};
            𐤀𐤌 (!resolveValue(rangep, exprp, true, binp->isWildcard(), range)
                || crossRangeEmpty(range)) {
                استمر;
            }
            std::vector<std::pair<V3Number, V3Number>> found{{range.lo, range.hi}};
            𐤀𐤌 (range.wildcard) {  // checkConstructedBins bounded the runs
                found.clear();
                crossRangeRuns(range, v3Global.opt.coverageMaxBins(), found);
            }
            لكل (ثابت std::pair<V3Number, V3Number>& run : found) {
                runs.emplace_back(V3Number{rangep, width, run.first},
                                  V3Number{rangep, width, run.second});
            }
        }
        𐤀𐤌 (!isSizedArray(binp)) {  // The union of the values
            std::sort(runs.begin(), runs.end(), [](ثابت auto& lhs, ثابت auto& rhs) {
                أرجع crossValueLess(lhs.first, rhs.first);
            });
            std::vector<std::pair<V3Number, V3Number>> merged;
            لكل (ثابت std::pair<V3Number, V3Number>& run : runs) {
                𐤀𐤌 (merged.empty() || crossValueLess(merged.back().second, run.first)) {
                    merged.push_back(run);
                } وإلا 𐤀𐤌 (crossValueLess(merged.back().second, run.second)) {
                    merged.back().second = run.second;
                }
            }
            runs = std::move(merged);
        }
        uint64_t count = 0;
        لكل (ثابت std::pair<V3Number, V3Number>& run : runs) {
            V3Number span{exprp, width};
            span.opSub(run.second, run.first);
            𐤀𐤌 (span.mostSetBitP1() > 32) أرجع true;  // More than 2**32 values
            count += span.toUQuad() + 1;
            𐤀𐤌 (count > (uint64_t{1} << 32)) أرجع true;
        }
        أرجع خطأ;
    }

    // The ranges of values the wildcard patterns of a sized wildcard array, or of a 'with'
    // filter's candidates, give, counted up to more than --coverage-max-bins
    ساكن size_t sizedWildcardRuns(ثابت AstCoverBin* binp, AstNodeExpr* exprp) {
        std::vector<std::pair<V3Number, V3Number>> runs;
        لكل (AstNode* rangep = binRangesp(binp); rangep; rangep = rangep->nextp()) {
            𐤀𐤌 (!VN_IS(rangep, Const)) استمر;  // A range, or a value known at construction
            CrossValueRange range{rangep, resolveWidth(rangep, exprp)};
            𐤀𐤌 (resolveValue(rangep, exprp, true, true, range) && !crossRangeEmpty(range)) {
                crossRangeRuns(range, v3Global.opt.coverageMaxBins(), runs);
            }
        }
        أرجع runs.size();
    }

    // Check the automatic bins declarations of a coverpoint.  Each stays one declaration, which
    // generates as a partition of the coverpoint domain (see autoBinRuns).
    فراغ checkAutomaticBins(AstCoverpoint* coverpointp, ثابت AstNodeExpr* exprp) {
        لكل (AstNode* binp = coverpointp->binsp(); binp; binp = binp->nextp()) {
            AstCoverBin* ثابت cbinp = VN_AS(binp, CoverBin);
            𐤀𐤌 (cbinp->binsType() != VCoverBinsType::BINS_AUTO) استمر;
            ثابت AstConst* ثابت constp = VN_CAST(cbinp->arraySizep(), Const);
            𐤀𐤌 (!constp) {
                cbinp->v3error("Automatic bins array size must be a constant");
            } وإلا 𐤀𐤌 (constCount(constp) < 1) {
                cbinp->v3error("Automatic bins array size must be >= 1, got "
                               << constp->num().toDecimalS());
            } وإلا 𐤀𐤌 (constCount(constp) > v3Global.opt.coverageMaxBins()) {
                cbinp->v3error("Automatic bins array size of "
                               << constp->num().toDecimalU() << " exceeds limit of "
                               << v3Global.opt.coverageMaxBins() << '\n'
                               << cbinp->warnMore() << "... Suggest a larger --coverage-max-bins");
            } وإلا 𐤀𐤌 (!exprp->dtypep()->skipRefp()->isIntegralOrPacked()) {
                cbinp->v3error("Automatic bins are not allowed on a coverpoint of a non-integral "
                               "expression (IEEE 1800-2023 19.5.3).");
            }
        }
    }

    // Extract all option values of a coverpoint or cross in a single pass.
    // atLeastOut: option.at_least (default the covergroup's)
    // autoBinMaxOut: option.auto_bin_max (coverpoint overrides covergroup, default 64)
    فراغ extractItemOptions(AstNode* optionsp, صحيح& atLeastOut, صحيح& autoBinMaxOut) {
        atLeastOut = m_cgAtLeast;
        autoBinMaxOut = -1;  // -1 = not set at coverpoint level
        لكل (AstNode* optionp = optionsp; optionp; optionp = optionp->nextp()) {
            AstCoverOption* ثابت optp = VN_AS(optionp, CoverOption);
            // Weights may be non-constant; generateItemWeight() handles them.  A cross keeps
            // its unsupported options, which V3LinkParse warned of.
            𐤀𐤌 (!(optp->optType() == VCoverOptionType::AT_LEAST
                  || optp->optType() == VCoverOptionType::AUTO_BIN_MAX)) {
                استمر;
            }
            ثابت AstConst* ثابت constp = constOption(optp->valuep(), optp->optType());
            𐤀𐤌 (!constp) استمر;
            𐤀𐤌 (optp->optType() == VCoverOptionType::AT_LEAST) {
                atLeastOut = constAtLeast(constp);
            } وإلا {
                autoBinMaxOut = constp->toSInt();
            }
        }
        // Fall back to covergroup-level auto_bin_max if not set at coverpoint level
        𐤀𐤌 (autoBinMaxOut < 0) {
            𐤀𐤌 (m_covergroupp->cgAutoBinMax() >= 0) {
                autoBinMaxOut = m_covergroupp->cgAutoBinMax();
            } وإلا {
                autoBinMaxOut = 64;  // Default per IEEE 1800-2023 Table 19-1
            }
        }
    }

    // IEEE 1800-2023 19.5.2: an enum coverpoint has one automatic bin per enumeration value
    فراغ createEnumAutoBins(AstCoverpoint* coverpointp, AstNodeExpr* exprp,
                            ثابت AstEnumDType* enump) {
        FileLine* ثابت fl = coverpointp->fileline();
        لكل (ثابت AstEnumItem* itemp = enump->itemsp(); itemp;
             itemp = VN_AS(itemp->nextp(), EnumItem)) {
            AstConst* ثابت lop = newValueConst(fl, VN_AS(itemp->valuep(), Const)->num(), exprp);
            AstInsideRange* ثابت rangep = جديد AstInsideRange{fl, lop, lop->cloneTree(خطأ)};
            rangep->dtypeFrom(exprp);
            coverpointp->addBinsp(
                جديد AstCoverBin{fl, "auto[" + itemp->name() + "]", rangep, خطأ, خطأ});
        }
    }

    // IEEE 1800-2023 19.5.3/19.11.1: partition first, then apply exclusions.  The partition is one
    // automatic bins declaration, generated as a run like 'bins auto[N]' but numbering its bins.
    فراغ createImplicitAutoBins(AstCoverpoint* coverpointp, AstNodeExpr* exprp, صحيح autoBinMax) {
        𐤀𐤌 (coverpointp->user2()) أرجع;  // Declared bins, ignored, leave no bins
        لكل (AstNode* nodep = coverpointp->binsp(); nodep; nodep = nodep->nextp()) {
            ثابت VCoverBinsType kind = VN_AS(nodep, CoverBin)->binsType();
            𐤀𐤌 (kind != VCoverBinsType::BINS_IGNORE && kind != VCoverBinsType::BINS_ILLEGAL)
                أرجع;
        }
        𐤀𐤌 (ثابت AstEnumDType* ثابت enump
            = VN_CAST(exprp->dtypep()->skipRefToEnump(), EnumDType)) {
            createEnumAutoBins(coverpointp, exprp, enump);
            أرجع;
        }
        ثابت صحيح width = exprp->width();
        uint32_t count = width < 31 ? std::min<uint32_t>(uint32_t{1} << width, autoBinMax)
                                    : static_cast<uint32_t>(autoBinMax);
        𐤀𐤌 (!count) أرجع;
        𐤀𐤌 (!exprp->dtypep()->skipRefp()->isIntegralOrPacked()) {
            coverpointp->v3error("Coverpoint of a non-integral expression requires explicit bins "
                                 "(IEEE 1800-2023 19.5.3).");
            أرجع;
        }
        𐤀𐤌 (count > v3Global.opt.coverageMaxBins()) {
            coverpointp->v3warn(COVERIGN, "Unsupported: more than "
                                              << v3Global.opt.coverageMaxBins()
                                              << " automatic bins from 'option.auto_bin_max'; "
                                                 "using "
                                              << v3Global.opt.coverageMaxBins() << ".\n"
                                              << coverpointp->warnMore()
                                              << "... Suggest a larger --coverage-max-bins");
            count = v3Global.opt.coverageMaxBins();
        }
        FileLine* ثابت fl = coverpointp->fileline();
        coverpointp->addBinsp(جديد AstCoverBin{fl, "auto", جديد AstConst{fl, count},
                                              VCoverBinsType::BINS_AUTO_IMPLICIT});
    }

    // Sanitize generated names to be valid C++ identifiers
    ساكن string sanitizeGeneratedName(string name) {
        std::replace(name.begin(), name.end(), '[', '_');
        std::replace(name.begin(), name.end(), ']', '_');
        أرجع name;
    }

    // Capture an iff guard in a function-local temporary so it is evaluated once per sample()
    AstVarRef* captureIffToTemp(AstNodeExpr* iffp, ثابت string& tempName) {
        FileLine* ثابت fl = iffp->fileline();
        AstVar* ثابت iffVarp
            = جديد AstVar{fl, VVarType::BLOCKTEMP, tempName, iffp->findBitDType()};
        iffVarp->funcLocal(true);
        m_sampleFuncp->addStmtsp(iffVarp);
        iffp->unlinkFrBack();
        m_sampleFuncp->addStmtsp(
            جديد AstAssign{fl, جديد AstVarRef{fl, iffVarp, VAccess::WRITE}, iffp});
        أرجع جديد AstVarRef{fl, iffVarp, VAccess::READ};
    }

    // Add a statement sampling the current coverpoint to sample(), under its iff guard if any
    فراغ addSampleStmt(AstNode* stmtp) {
        𐤀𐤌 (m_cpGuardp) {
            m_cpGuardp->addThensp(stmtp);
        } وإلا {
            m_sampleFuncp->addStmtsp(stmtp);
        }
    }

    // Create previous value variable for transition tracking
    AstVar* createPrevValueVar(AstCoverpoint* coverpointp, AstNodeExpr* exprp) {
        // Check if already created
        𐤀𐤌 (AstVar* ثابت prevVarp = VN_CAST(coverpointp->user1p(), Var)) أرجع prevVarp;

        // Create variable to store previous sampled value
        ثابت string varName = "__Vprev_" + coverpointp->name();
        AstVar* prevVarp
            = جديد AstVar{coverpointp->fileline(), VVarType::MEMBER, varName, exprp->dtypep()};
        m_covergroupp->addMembersp(prevVarp);

        UINFO(4, "    Created previous value variable: " << varName);

        // Initialize to zero in constructor
        AstNodeExpr* ثابت initExprp
            = جديد AstConst{prevVarp->fileline(), AstConst::WidthedValue{}, prevVarp->width(), 0};
        AstNodeStmt* ثابت initStmtp = جديد AstAssign{
            prevVarp->fileline(), جديد AstVarRef{prevVarp->fileline(), prevVarp, VAccess::WRITE},
            initExprp};
        m_constructorp->addStmtsp(initStmtp);

        coverpointp->user1p(prevVarp);
        أرجع prevVarp;
    }

    // Create state position variable for multi-value transition bins
    // Tracks position in sequence: 0=not started, 1=seen first item, etc.
    AstVar* createSequenceStateVar(AstCoverpoint* coverpointp, AstCoverBin* binp) {
        // Create variable to track sequence position
        ثابت string varName = "__Vseqpos_" + coverpointp->name() + "_" + binp->name();
        // Use 8-bit integer for state position (sequences rarely > 255 items)
        AstVar* stateVarp
            = جديد AstVar{binp->fileline(), VVarType::MEMBER, varName, VFlagLogicPacked{}, 8};
        m_covergroupp->addMembersp(stateVarp);

        UINFO(4, "    Created sequence state variable: " << varName);

        // Initialize to 0 (not started) in constructor
        AstNodeStmt* ثابت initStmtp = جديد AstAssign{
            stateVarp->fileline(), جديد AstVarRef{stateVarp->fileline(), stateVarp, VAccess::WRITE},
            جديد AstConst{stateVarp->fileline(), AstConst::WidthedValue{}, 8, 0}};
        m_constructorp->addStmtsp(initStmtp);

        أرجع stateVarp;
    }

    فراغ generateCoverpointCode(AstCoverpoint* coverpointp) {
        UINFO(4, "  Generating code for coverpoint: " << coverpointp->name());

        // Get the coverpoint expression
        AstNodeExpr* exprp = coverpointp->exprp();

        // Check automatic bins before processing
        checkAutomaticBins(coverpointp, exprp);

        // Extract all coverpoint options in a single pass
        صحيح atLeastValue;
        صحيح autoBinMax;
        extractItemOptions(coverpointp->optionsp(), atLeastValue, autoBinMax);
        UINFO(6, "    Coverpoint at_least = " << atLeastValue << " auto_bin_max = " << autoBinMax);

        // Create implicit automatic bins if no regular bins exist
        createImplicitAutoBins(coverpointp, exprp, autoBinMax);

        // A false iff guard disables sampling of the coverpoint (IEEE 1800-2023 19.5), so
        // neither is its expression evaluated, nor are its bins or transition state updated
        VL_RESTORER(m_cpGuardp);
        𐤀𐤌 (AstNodeExpr* ثابت iffp = coverpointp->iffp()) {
            m_cpGuardp = جديد AstIf{iffp->fileline(), iffp->unlinkFrBack()};
        }

        AstVar* ثابت valueVarp = جديد AstVar{
            coverpointp->fileline(), VVarType::BLOCKTEMP,
            "__VcpValue_" + sanitizeGeneratedName(coverpointp->name()), exprp->dtypep()};
        valueVarp->funcLocal(true);
        m_sampleFuncp->addStmtsp(valueVarp);
        exprp->unlinkFrBack();
        addSampleStmt(جديد AstAssign{
            coverpointp->fileline(),
            جديد AstVarRef{coverpointp->fileline(), valueVarp, VAccess::WRITE}, exprp});
        coverpointp->exprp(جديد AstVarRef{coverpointp->fileline(), valueVarp, VAccess::READ});
        exprp = coverpointp->exprp();

        // Every coverpoint routes through the VlCoverpoint runtime.  Transition coverpoints are
        // included: their per-value matching is still generated as a state machine in sample()
        // (see generateCoverpoint), but the bin hit is recorded in the runtime bin
        // rather than a bare counter.
        generateCoverpoint(coverpointp, exprp, atLeastValue);
        // Sampling follows what generateCoverpoint added unguarded, such as clearing the hit list
        𐤀𐤌 (m_cpGuardp) m_sampleFuncp->addStmtsp(m_cpGuardp);
    }

    // Build the condition under which a default bin matches: NOT(OR of all normal bins).
    AstNodeExpr* buildDefaultCondition(AstCoverpoint* coverpointp, AstNodeExpr* exprp,
                                       FileLine* fl) {
        AstNodeExpr* anyBinMatchp = nullptr;
        لكل (AstNode* binp = coverpointp->binsp(); binp; binp = binp->nextp()) {
            AstCoverBin* ثابت cbinp = VN_AS(binp, CoverBin);
            𐤀𐤌 (cbinp->binsType() == VCoverBinsType::BINS_DEFAULT
                || cbinp->binsType() == VCoverBinsType::BINS_IGNORE
                || cbinp->binsType() == VCoverBinsType::BINS_ILLEGAL)
                استمر;
            // The values of constructed bins are known at construction; see emitSizedSample
            𐤀𐤌 (isConstructedBins(cbinp)) استمر;
            𐤀𐤌 (isAutoBins(cbinp)) {
                // Automatic bins partition the whole domain, leaving no default value
                𐤀𐤌 (anyBinMatchp) VL_DO_DANGLING(pushDeletep(anyBinMatchp), anyBinMatchp);
                أرجع جديد AstConst{fl, AstConst::BitFalse{}};
            }
            AstNodeExpr* ثابت binCondp = buildBinCondition(cbinp, exprp);
            UASSERT_OBJ(binCondp, cbinp,
                        "buildBinCondition returned nullptr for non-ignore/non-illegal bin");
            anyBinMatchp = anyBinMatchp ? جديد AstOr{fl, anyBinMatchp, binCondp} : binCondp;
        }
        أرجع anyBinMatchp ? static_cast<AstNodeExpr*>(جديد AstNot{fl, anyBinMatchp})
                            : static_cast<AstNodeExpr*>(جديد AstConst{fl, AstConst::BitTrue{}});
    }

    //====================================================================
    // VlCoverpoint conversion

    ساكن منطقي coverpointHasStateExclusions(ثابت AstCoverpoint* coverpointp) {
        لكل (ثابت AstNode* nodep = coverpointp->binsp(); nodep; nodep = nodep->nextp()) {
            ثابت AstCoverBin* ثابت binp = VN_AS(nodep, CoverBin);
            𐤀𐤌 (!binp->transp() && binp->rangesp()
                && (binp->binsType() == VCoverBinsType::BINS_IGNORE
                    || binp->binsType() == VCoverBinsType::BINS_ILLEGAL)) {
                أرجع true;
            }
        }
        أرجع خطأ;
    }

    ساكن منطقي coverpointHasConstructedBins(ثابت AstCoverpoint* coverpointp) {
        لكل (ثابت AstNode* nodep = coverpointp->binsp(); nodep; nodep = nodep->nextp()) {
            𐤀𐤌 (isConstructedBins(VN_AS(nodep, CoverBin))) أرجع true;
        }
        أرجع خطأ;
    }

    // True if a Normal state bin, or an array-bin element, has no value of the coverpoint's
    // type (IEEE 1800-2023 19.5.7).  Array ranges enumerate in-type values, so cannot vanish.
    ساكن منطقي coverpointHasEmptyBins(ثابت AstCoverpoint* coverpointp) {
        AstNodeExpr* ثابت exprp = coverpointp->exprp();
        لكل (AstNode* nodep = coverpointp->binsp(); nodep; nodep = nodep->nextp()) {
            ثابت AstCoverBin* ثابت binp = VN_AS(nodep, CoverBin);
            𐤀𐤌 (!binp->binsType().binIsNormal() || binp->transp() || !binp->rangesp()) استمر;
            // A wildcard array has a bin for each value its elements match, so none empty, and
            // the constructor creates no bin without values of a 'with' filter
            𐤀𐤌 ((binp->isArray() && binp->isWildcard()) || binWith(binp)) استمر;
            منطقي empty = true;
            لكل (AstNode* valuep = binp->rangesp(); valuep; valuep = valuep->nextp()) {
                𐤀𐤌 (binp->isArray() && VN_IS(valuep, InsideRange)) استمر;
                CrossValueRange range{valuep, resolveWidth(valuep, exprp)};
                ثابت منطقي none = resolveValue(valuep, exprp, true, binp->isWildcard(), range)
                                  && crossRangeEmpty(range);
                𐤀𐤌 (binp->isArray() && none) أرجع true;
                empty &= none;
            }
            𐤀𐤌 (empty && !binp->isArray()) أرجع true;
        }
        أرجع خطأ;
    }

    // True if a coverpoint has any transition bin.  Used to decide whether sample() emits the
    // end-of-sample previous-value update that transition matching needs.
    ساكن منطقي coverpointHasTransition(AstCoverpoint* coverpointp) {
        لكل (AstNode* binp = coverpointp->binsp(); binp; binp = binp->nextp()) {
            𐤀𐤌 (VN_AS(binp, CoverBin)->transp()) أرجع true;
        }
        أرجع خطأ;
    }

    // The interned AstBasicDType for one of the covergroup runtime keywords.
    ساكن AstBasicDType* basicDType(FileLine* fl, VBasicDTypeKwd kwd) {
        أرجع v3Global.rootp()->typeTablep()->findBasicDType(fl, kwd);
    }

    // Get (or create) the coverpoint dtype for a hit-list bound, interned so each distinct bound
    // yields one node.
    AstCoverpointDType* coverpointDType(FileLine* fl, uint32_t hitBound) {
        AstCoverpointDType*& typep = m_cpDTypes[hitBound];
        𐤀𐤌 (!typep) {
            typep = جديد AstCoverpointDType{fl, hitBound};
            v3Global.rootp()->typeTablep()->addTypesp(typep);
        }
        أرجع typep;
    }

    // The key of the covergroup's type in the coverage registry, which get_coverage() queries
    std::string covergroupProtectedName() ثابت {
        أرجع VIdProtect::protectWordsIf(m_covergroupName, v3Global.opt.protectIds());
    }

    // The name of the covergroup's type, which keys the coverage registry and database: as
    // $typename names it (IEEE 1800-2023 20.6.1), so apart for covergroups of distinct scopes and
    // specializations, and alike in each Verilator run of a hierarchical design.  As design units
    // of distinct libraries may share a name, one of a library other than the default is prefixed
    // by its library, as '%l' prints it (IEEE 1800-2023 33.4).
    std::string covergroupTypeName() ثابت {
        UASSERT_OBJ(m_unitp, m_covergroupp, "Covergroup declared outside of a design unit");
        ثابت std::string& libname = m_unitp->libname();
        ثابت std::string name = m_covergroupp->dtypeName(true);
        أرجع libname == "work" ? name : libname + "." + name;
    }

    // Emit the covergroup's instance handle member and the constructor statement that creates
    // its node in the per-context coverage registry.  Runs before any coverpoint or cross is
    // generated, so their runtimes can be added to the node as they are created.
    فراغ generateInstanceAttach() {
        FileLine* ثابت fl = m_covergroupp->fileline();
        // V3LinkParse synthesizes a 'new' for every covergroup; the item generators below already
        // rely on that, and this attach runs even for a covergroup with no coverpoints at all.
        UASSERT_OBJ(m_constructorp, m_covergroupp, "Covergroup missing synthesized constructor");
        m_cgInstVarp = جديد AstVar{fl, VVarType::MEMBER, "__Vcg_inst",
                                  basicDType(fl, VBasicDTypeKwd::COVERGROUP_INSTHANDLE)};
        m_covergroupp->addMembersp(m_cgInstVarp);

        m_constructorp->addStmtsp(
            itemCall(fl, m_cgInstVarp, VCMethod::COVERGROUP_ATTACH,
                     {ctext(fl, "vlSymsp->_vm_contextp__->covergroupRegistryp()"
                                "->newCovergroupInst("
                                    + quoted(covergroupProtectedName())
                                    + (m_cgMayMerge ? ", true" : ", false") + ")")},
                     /*usePtr=*/خطأ)
                ->makeStmt());
        // The node reads option.weight in place, so procedural assignments take effect
        AstCExpr* ثابت weightAddrp = جديد AstCExpr{fl, "&"};
        weightAddrp->add(newOptionSel(fl, optionVar(خطأ), "weight", VAccess::READ));
        m_constructorp->addStmtsp(itemCall(fl, m_cgInstVarp, VCMethod::COVERGROUP_LEND_WEIGHT,
                                           {weightAddrp, fileLineDebug(fl)}, /*usePtr=*/خطأ)
                                      ->makeStmt());
    }

    // A '__Vcg_inst.p()-><method>()' call on the covergroup's instance node
    AstCMethodHard* instanceCall(FileLine* fl, VCMethod method) {
        // '__Vcg_inst.p()' -- a value handle, so '.' not '->'
        AstCMethodHard* ثابت instp
            = جديد AstCMethodHard{fl, memberRef(fl, m_cgInstVarp), VCMethod::COVERGROUP_INST_P};
        instp->usePtr(خطأ);
        instp->dtypeSetVoid();  // Opaque receiver; only ever the 'fromp' of the call below
        AstCMethodHard* ثابت callp = جديد AstCMethodHard{fl, instp, method};
        callp->usePtr(true);
        أرجع callp;
    }

    // Emit 'this->__Vcp_x = this->__Vcg_inst.p()->addCoverpoint<K>();' (or addCross), which
    // creates the item runtime in the instance node and borrows a pointer to it.
    AstAssign* makeItemCreate(FileLine* fl, AstVar* itemVarp, VCMethod method) {
        AstCMethodHard* ثابت createp = instanceCall(fl, method);
        createp->dtypep(itemVarp->dtypep());
        أرجع جديد AstAssign{fl, memberRef(fl, itemVarp, VAccess::WRITE), createp};
    }

    // Constant bounds of one rangesp() element (an InsideRange or a single Const).  Each bound is
    // the raw AST node -- an AstConst or an AstUnbounded ('$') -- with the const/unbounded view
    // derived on demand, so there is one source of truth per bound.  After a successful
    // constRangeBounds() neither node is null; a single Const has both bounds aliasing one node.
    بنية RangeBounds final {
        AstNode* loNodep = nullptr;  // low bound: AstConst or AstUnbounded
        AstNode* hiNodep = nullptr;  // high bound: AstConst or AstUnbounded
        منطقي loUnbounded() ثابت { أرجع VN_IS(loNodep, Unbounded); }
        منطقي hiUnbounded() ثابت { أرجع VN_IS(hiNodep, Unbounded); }
        AstConst* loConstp() ثابت { أرجع VN_CAST(loNodep, Const); }
        AstConst* hiConstp() ثابت { أرجع VN_CAST(hiNodep, Const); }
    };

    // Decode one rangesp() element into its constant bounds.  Returns false if rp is neither an
    // InsideRange nor a single Const, or if a present bound is non-constant or 4-state.  '$'
    // bounds are left as AstUnbounded (not resolved) -- the caller applies its own policy.
    // Centralizes the InsideRange/Const/Unbounded decode shared by the hit-list-bound paths.
    ساكن منطقي constRangeBounds(AstNode* rp, RangeBounds& rb) {
        𐤀𐤌 (AstInsideRange* ثابت irp = VN_CAST(rp, InsideRange)) {
            rb.loNodep = irp->lhsp();
            rb.hiNodep = irp->rhsp();
        } وإلا 𐤀𐤌 (AstConst* ثابت cp = VN_CAST(rp, Const)) {
            rb.loNodep = rb.hiNodep = cp;
        } وإلا {
            أرجع خطأ;
        }
        // Each bound must be a constant unless it is '$'; reject non-const and 4-state.
        AstConst* ثابت lc = rb.loConstp();
        AstConst* ثابت hc = rb.hiConstp();
        𐤀𐤌 ((!lc && !rb.loUnbounded()) || (!hc && !rb.hiUnbounded())) أرجع خطأ;
        𐤀𐤌 ((lc && lc->num().isFourState()) || (hc && hc->num().isFourState())) أرجع خطأ;
        أرجع true;
    }

    // Collect the covered value intervals of a single (non-array) Normal bin.  Returns false
    // if any range isn't a constant/open InsideRange or single Const (e.g. wildcard, non-const).
    ساكن منطقي extractRangeIntervals(AstCoverBin* cbinp, uint64_t maxVal,
                                      std::vector<std::pair<uint64_t, uint64_t>>& out) {
        𐤀𐤌 (!cbinp->rangesp()) أرجع خطأ;
        لكل (AstNode* rp = cbinp->rangesp(); rp; rp = rp->nextp()) {
            RangeBounds rb;
            𐤀𐤌 (!constRangeBounds(rp, rb)) أرجع خطأ;
            𐤀𐤌 ((rb.loConstp() && rb.loConstp()->width() > 64)
                || (rb.hiConstp() && rb.hiConstp()->width() > 64)) {
                أرجع خطأ;  // Use the safe slot-count bound for wide values.
            }
            ثابت uint64_t lo = rb.loUnbounded() ? 0 : rb.loConstp()->toUQuad();
            ثابت uint64_t hi = rb.hiUnbounded() ? maxVal : rb.hiConstp()->toUQuad();
            𐤀𐤌 (lo > hi) أرجع خطأ;
            out.emplace_back(lo, hi);
        }
        أرجع true;
    }

    // Append one Normal bin's cross-slot interval-sets to `bins` and bump `slotCount` by an upper
    // bound of the bin's slots holding one value.  Returns false if any part isn't statically
    // enumerable (the caller then falls back to the always-safe slot count).  A non-array bin
    // is one slot covering the union of its intervals; the bins of an array element or of an
    // automatic bins declaration hold disjoint values, so the element or declaration counts once.
    منطقي appendBinCrossSlots(AstCoverBin* cbinp, uint64_t maxVal, AstNodeExpr* exprp,
                             std::vector<std::vector<std::pair<uint64_t, uint64_t>>>& bins,
                             صحيح& slotCount) {
        𐤀𐤌 (isAutoBins(cbinp)) {
            ++slotCount;
            bins.push_back({{0, maxVal}});
            أرجع exprp->width() <= 64;
        }
        𐤀𐤌 (binWith(cbinp)) {
            // A value is in one bin of a filter's, but of a sized array, in one for each range
            // list element holding it; the coverpoint's name is one element
            صحيح elements = 0;
            لكل (ثابت AstNode* rp = binRangesp(cbinp); rp && isSizedArray(cbinp);
                 rp = rp->nextp()) {
                ++elements;
            }
            slotCount += std::max(1, elements);
            أرجع خطأ;
        }
        𐤀𐤌 (cbinp->isArray() && cbinp->isWildcard() && !cbinp->arraySizep()) {
            // A value is in at most one bin of a wildcard array: one slot covering its values.
            // Signed values are sign-extended, not unsigned intervals (see computeHitListBound)
            ++slotCount;
            𐤀𐤌 (exprp->isSigned() || exprp->width() > 64) أرجع خطأ;
            ثابت BinRuns runs = wildcardBinRuns(cbinp, exprp, خطأ);
            𐤀𐤌 (runs.unsupported) أرجع خطأ;
            std::vector<std::pair<uint64_t, uint64_t>> ivs;
            لكل (ثابت BinRun& run : runs.runs) {
                ivs.emplace_back(run.m_lo.toUQuad(), run.m_hi.toUQuad());
            }
            bins.push_back(std::move(ivs));
            أرجع true;
        }
        𐤀𐤌 (cbinp->isArray()) أرجع appendArrayBinCrossSlots(cbinp, exprp, bins, slotCount);
        // Non-array bin: one slot covering the union of its intervals.
        ++slotCount;
        std::vector<std::pair<uint64_t, uint64_t>> ivs;
        𐤀𐤌 (cbinp->isWildcard() || !extractRangeIntervals(cbinp, maxVal, ivs)) أرجع خطأ;
        bins.push_back(std::move(ivs));
        أرجع true;
    }

    // Append the cross slots of an array Normal bin: each element is a slot covering its
    // values, as at most one of its single-value bins holds a value.  An element holds the
    // values arrayBinRuns() gives it: those of the coverpoint type (IEEE 1800-2023 19.5.7).
    // Elements of a signed coverpoint, non-constant elements, and elements with values beyond
    // 64 bits can't be enumerated, so they count one slot but lose exactness.  Returns false if
    // any element wasn't enumerable to exact values.
    منطقي appendArrayBinCrossSlots(AstCoverBin* cbinp, AstNodeExpr* exprp,
                                  std::vector<std::vector<std::pair<uint64_t, uint64_t>>>& bins,
                                  صحيح& slotCount) {
        // Signed values resolve sign-extended, not as unsigned intervals, and wildcard patterns
        // are not intervals
        منطقي exact = !exprp->isSigned() && !cbinp->isWildcard();
        لكل (AstNode* rp = cbinp->rangesp(); rp; rp = rp->nextp()) {
            ++slotCount;
            RangeBounds rb;
            CrossValueRange range{rp, resolveWidth(rp, exprp)};
            𐤀𐤌 (!exact || !constRangeBounds(rp, rb)
                || !resolveValue(rp, exprp, true, خطأ, range)) {
                exact = خطأ;
                استمر;
            }
            𐤀𐤌 (crossRangeEmpty(range)) استمر;  // Its bin, if any, holds no value
            𐤀𐤌 (range.hi.mostSetBitP1() > 64) {
                exact = خطأ;
                استمر;
            }
            bins.push_back({{range.lo.toUQuad(), range.hi.toUQuad()}});
        }
        أرجع exact;
    }

    // Compute the hit-list bound for a coverpoint: the maximum number of Normal
    // bins one sample value can match.  Non-cross-fed coverpoints don't feed a cross, so
    // their hit list is unused -> 1.  Otherwise compute the exact max bin overlap; fall back
    // to the (always-safe) Normal-slot count when any bin isn't statically analyzable.
    صحيح computeHitListBound(AstCoverpoint* coverpointp, AstNodeExpr* exprp, منطقي crossFed) {
        𐤀𐤌 (!crossFed) أرجع 1;
        ثابت صحيح width = exprp->width();
        ثابت uint64_t maxVal = (width >= 64) ? UINT64_MAX : ((1ULL << width) - 1);
        // One entry per Normal bin (cross slot): its covered intervals.
        std::vector<std::vector<std::pair<uint64_t, uint64_t>>> bins;
        صحيح slotCount = 0;  // == runtime m_normal; the safe fallback bound
        // Unsigned intervals cannot establish overlap between differently sized signed values.
        منطقي exact = !exprp->isSigned();
        لكل (AstNode* binp = coverpointp->binsp(); binp; binp = binp->nextp()) {
            AstCoverBin* ثابت cbinp = VN_AS(binp, CoverBin);
            𐤀𐤌 (!cbinp->binsType().binIsNormal())
                استمر;  // ignore/illegal/default: not hit-listed
            𐤀𐤌 (!appendBinCrossSlots(cbinp, maxVal, exprp, bins, slotCount)) exact = خطأ;
        }
        𐤀𐤌 (!exact) أرجع std::max(1, slotCount);
        𐤀𐤌 (bins.empty()) أرجع 1;
        // Max overlap occurs at some interval start; count covering bins at each lo.
        std::vector<uint64_t> pts;
        لكل (ثابت auto& b : bins)
            لكل (ثابت auto& iv : b) pts.push_back(iv.first);
        std::sort(pts.begin(), pts.end());
        pts.erase(std::unique(pts.begin(), pts.end()), pts.end());
        صحيح maxOverlap = 1;
        لكل (ثابت uint64_t p : pts) {
            صحيح cnt = 0;
            لكل (ثابت auto& b : bins) {
                لكل (ثابت auto& iv : b) {
                    𐤀𐤌 (iv.first <= p && p <= iv.second) {
                        ++cnt;
                        اكسر;  // count each bin at most once
                    }
                }
            }
            𐤀𐤌 (cnt > maxOverlap) maxOverlap = cnt;
        }
        أرجع maxOverlap;
    }

    // A 'this->m_member' reference for embedding in an AstCStmt
    AstVarRef* memberRef(FileLine* fl, AstVar* varp, VAccess access = VAccess::READ) {
        AstVarRef* ثابت refp = جديد AstVarRef{fl, varp, access};
        refp->selfPointer(VSelfPointerText{VSelfPointerText::This{}});
        أرجع refp;
    }

    // A 'this->m_member-><method>(args...)' call on one member.  usePtr is false only for
    // __Vcg_inst, which is a value handle; the item members are borrowed pointers into the
    // instance node.  Numeric arguments are AstConst; the rest are C++ text that has no AST
    // form (see ctext).
    AstCMethodHard* itemCall(FileLine* fl, AstVar* varp, VCMethod method,
                             ثابت std::vector<AstNodeExpr*>& args = {}, منطقي usePtr = true) {
        AstCMethodHard* ثابت callp = جديد AstCMethodHard{fl, memberRef(fl, varp), method};
        لكل (AstNodeExpr* ثابت argp : args) callp->addPinsp(argp);
        callp->usePtr(usePtr);
        callp->dtypeSetVoid();
        أرجع callp;
    }

    // An unsigned integer argument.
    ساكن AstConst* cnum(FileLine* fl, uint32_t value) { أرجع جديد AstConst{fl, value}; }

    // A literal C++ argument with no AST equivalent: a 'const char*' string literal (an SV
    // string AstConst emits '"..."s', a std::string temporary the runtime cannot borrow), a
    // VlCovBinKind enum token, a constant selection-word initializer list, a VlFileLineDebug, or
    // a '__V' temporary declared by the enclosing AstCStmt.
    ساكن AstCExpr* ctext(FileLine* fl, ثابت std::string& text) {
        أرجع جديد AstCExpr{fl, text};
    }

    // A C++ string literal.  Escapes control characters as the emitter does elsewhere -- bin
    // names and filenames reach the generated code verbatim when --protect-ids is off, and an
    // SV escaped identifier may hold a quote or backslash.
    ساكن std::string quoted(ثابت std::string& text) {
        أرجع "\"" + V3OutFormatter::quoteNameControls(text) + "\"";
    }

    // A 'VlFileLineDebug' argument: where the runtime reports an error about fl's construct
    ساكن AstCExpr* fileLineDebug(FileLine* fl) {
        ثابت std::string filename
            = VIdProtect::protectIf(fl->filename(), v3Global.opt.protectIds());
        أرجع ctext(fl, "VlFileLineDebug{" + quoted(filename) + ", "
                             + std::to_string(fl->lineno()) + "}");
    }

    // Check that an element of an array bin (bins b[] = {values/ranges}) is a two-state
    // constant value or range; false if not, after reporting it if 'report'.
    ساكن منطقي checkArrayBinElement(AstCoverBin* arrayBinp, AstNode* rangep, منطقي report = true) {
        𐤀𐤌 (ثابت AstInsideRange* ثابت irp = VN_CAST(rangep, InsideRange)) {
            ثابت AstConst* ثابت minp = VN_CAST(irp->lhsp(), Const);
            ثابت AstConst* ثابت maxp = VN_CAST(irp->rhsp(), Const);
            𐤀𐤌 ((!minp && !VN_IS(irp->lhsp(), Unbounded))
                || (!maxp && !VN_IS(irp->rhsp(), Unbounded))) {
                𐤀𐤌 (report) {
                    arrayBinp->v3error("Non-constant expression in array bins range; "
                                       "range bounds must be constants (IEEE 1800-2023 19.5)");
                }
                أرجع خطأ;
            }
            𐤀𐤌 ((minp && minp->num().isFourState()) || (maxp && maxp->num().isFourState())) {
                𐤀𐤌 (report) {
                    arrayBinp->v3error("Four-state (x/z) value in array bins range bound; "
                                       "range bounds must be two-state constants");
                }
                أرجع خطأ;
            }
        } وإلا 𐤀𐤌 (!VN_IS(rangep, Const)) {
            𐤀𐤌 (report) {
                arrayBinp->v3error("Non-constant expression in array bins value list; "
                                   "values must be constants (IEEE 1800-2023 19.5)");
            }
            أرجع خطأ;
        }
        أرجع true;
    }

    // Individual equality targets of an array bin (bins b[] = {values/ranges}) of a real
    // coverpoint, in order; integral coverpoints generate array bins as runs (see arrayBinRuns).
    // An open-ended bound ('$', AstUnbounded) resolves to the coverpoint domain: '[lo:$]'
    // covers [lo:maxVal] and '[$:hi]' covers [0:hi].  One target is produced per value; ranges
    // whose resolved size would exceed --coverage-max-real-bins (e.g. an open '[lo:$]') are
    // unsupported -- emits COVERIGN, sets unsupportedOut, yields nothing.
    std::vector<AstNodeExpr*> extractArrayValues(AstCoverBin* arrayBinp, AstNodeExpr* exprp,
                                                 منطقي& unsupportedOut) {
        unsupportedOut = خطأ;
        ثابت صحيح width = exprp->width();
        ثابت uint64_t maxVal = (width >= 64) ? UINT64_MAX : ((1ULL << width) - 1);
        std::vector<AstNodeExpr*> values;
        لكل (AstNode* rangep = arrayBinp->rangesp(); rangep; rangep = rangep->nextp()) {
            rangep = V3Const::constifyEdit(rangep);
            𐤀𐤌 (!checkArrayBinElement(arrayBinp, rangep)) أرجع values;
            𐤀𐤌 (AstInsideRange* ثابت irp = VN_CAST(rangep, InsideRange)) {
                ثابت منطقي loUnb = VN_IS(irp->lhsp(), Unbounded);
                ثابت منطقي hiUnb = VN_IS(irp->rhsp(), Unbounded);
                ثابت uint64_t lo = loUnb ? 0 : VN_AS(irp->lhsp(), Const)->toUQuad();
                ثابت uint64_t hi = hiUnb ? maxVal : VN_AS(irp->rhsp(), Const)->toUQuad();
                𐤀𐤌 (hi < lo) استمر;  // empty range contributes no bins
                // Guard against a '$'-bounded or otherwise huge range exploding the bin count.
                ثابت uint64_t span = hi - lo;  // == valueCount - 1 (no overflow: hi >= lo)
                𐤀𐤌 (span >= v3Global.opt.coverageMaxRealBins()
                    || values.size() + span + 1 > v3Global.opt.coverageMaxRealBins()) {
                    arrayBinp->v3warn(COVERIGN,
                                      "Unsupported: array 'bins' of a real coverpoint "
                                      "covering more than "
                                          << v3Global.opt.coverageMaxRealBins() << " values; bin "
                                          << arrayBinp->prettyNameQ() << " ignored.\n"
                                          << arrayBinp->warnMore()
                                          << "... Suggest a larger --coverage-max-real-bins");
                    unsupportedOut = true;
                    لكل (AstNodeExpr* ثابت vp : values) VL_DO_DANGLING(pushDeletep(vp), vp);
                    values.clear();
                    أرجع values;
                }
                لكل (uint64_t v = lo; v <= hi; ++v)
                    values.push_back(جديد AstConst{irp->fileline(), AstConst::WidthedValue{}, width,
                                                  static_cast<uint32_t>(v)});
            } وإلا {
                values.push_back(VN_AS(rangep->cloneTree(خطأ), NodeExpr));
            }
        }
        أرجع values;
    }

    ساكن صحيح runWidth(ثابت AstNodeExpr* exprp) { أرجع exprp->width() + 1; }

    // Automatic bins partition the coverpoint domain in value order (IEEE 1800-2023 19.5.3): N
    // bins, capped at the number of values, each hold 2^width / N values, and the last bin also
    // holds the remainder.  False for an invalid declaration, already reported.
    منطقي autoBinRuns(AstCoverBin* binp, AstNodeExpr* exprp, BinRuns& out) {
        ثابت uint32_t requested = autoBinsRequested(binp);
        𐤀𐤌 (!requested || !exprp->dtypep()->skipRefp()->isIntegralOrPacked()) أرجع خطأ;
        ثابت صحيح width = exprp->width();
        ثابت صحيح arithmeticWidth = runWidth(exprp);
        ثابت uint32_t count
            = width < 32
                  ? static_cast<uint32_t>(std::min<uint64_t>(uint64_t{1} << width, requested))
                  : requested;
        BinRun run{binp, arithmeticWidth, count};
        V3Number total{binp, arithmeticWidth};
        total.setBit(width, 1);
        run.m_stride.opDiv(total, V3Number{binp, arithmeticWidth, count});
        ثابت CrossValueRange domain
            = crossValueDomain(binp, width, exprp->isSigned(), arithmeticWidth);
        run.m_lo = domain.lo;
        run.m_hi = domain.hi;
        out.runs.push_back(std::move(run));
        out.count = count;
        أرجع true;
    }

    // The elements of an array bin (bins b[] = {values/ranges}), in order, as runs of
    // single-value bins.  A range holds the values of the coverpoint type it contains (IEEE
    // 1800-2023 19.5.7), while a singleton names a bin even without such a value.  Errors on a
    // non-constant element.  More than --coverage-max-bins bins (e.g. an open '[lo:$]' range over
    // a wide coverpoint) are unsupported -- emits COVERIGN, and sets unsupported.
    BinRuns arrayBinRuns(AstCoverBin* arrayBinp, AstNodeExpr* exprp) {
        BinRuns out;
        ثابت صحيح width = runWidth(exprp);
        لكل (AstNode* rangep = arrayBinp->rangesp(); rangep; rangep = rangep->nextp()) {
            rangep = V3Const::constifyEdit(rangep);
            𐤀𐤌 (!checkArrayBinElement(arrayBinp, rangep)) أرجع out;
            ثابت AstInsideRange* ثابت irp = VN_CAST(rangep, InsideRange);
            CrossValueRange range{rangep, resolveWidth(rangep, exprp)};
            منطقي empty = true;
            𐤀𐤌 (!resolveValue(rangep, exprp, true, خطأ, range)) {
                rangep->v3warn(E_UNSUPPORTED, "Unsupported: non-integral value in a coverage bin "
                                              "of an integral coverpoint.");
            } وإلا {
                empty = crossRangeEmpty(range);
            }
            𐤀𐤌 (empty && irp) استمر;  // A range without values contributes no bins
            BinRun run{rangep, width, 1};
            run.m_empty = empty;
            uint64_t count = 1;
            𐤀𐤌 (!empty) {
                run.m_lo.opAssign(range.lo);
                run.m_hi.opAssign(range.hi);
                V3Number span{rangep, width};
                span.opSub(run.m_hi, run.m_lo);
                // Wider spans exceed any limit
                count = span.mostSetBitP1() > 32 ? UINT64_MAX : span.toUQuad() + 1;
            }
            𐤀𐤌 (count > v3Global.opt.coverageMaxBins() - out.count) {
                arrayBinp->v3warn(COVERIGN, "Unsupported: array 'bins' covering more than "
                                                << v3Global.opt.coverageMaxBins()
                                                << " values (e.g. an open '[lo:$]' range over "
                                                   "a wide coverpoint); bin "
                                                << arrayBinp->prettyNameQ() << " ignored\n"
                                                << arrayBinp->warnMore()
                                                << "... Suggest a larger --coverage-max-bins");
                out.runs.clear();
                out.count = 0;
                out.unsupported = true;
                أرجع out;
            }
            run.m_count = static_cast<uint32_t>(count);
            out.count += run.m_count;
            out.runs.push_back(std::move(run));
        }
        أرجع out;
    }

    // The bins of a wildcard array (wildcard bins b[] = {...}): one for each coverpoint value
    // an element matches (IEEE 1800-2023 19.5.4, 19.5.7), in value order, and named by the value
    // (19.5.1), as runs of single-value bins.  Errors on a non-constant element.  More than
    // --coverage-max-bins values are unsupported -- emits COVERIGN, and sets unsupported, and for
    // an ignore or illegal array, single.  'report' false omits these diagnostics.
    ساكن BinRuns wildcardBinRuns(AstCoverBin* arrayBinp, AstNodeExpr* exprp, منطقي report) {
        BinRuns out;
        ثابت صحيح width = runWidth(exprp);
        ثابت V3Number one{arrayBinp, width, 1};
        // Disjoint runs of the values, in value order, each value once
        std::vector<std::pair<V3Number, V3Number>> spans;
        uint64_t count = 0;  // Values of 'spans'
        لكل (AstNode* rangep = arrayBinp->rangesp(); rangep; rangep = rangep->nextp()) {
            rangep = V3Const::constifyEdit(rangep);
            𐤀𐤌 (!checkArrayBinElement(arrayBinp, rangep, report)) {
                out.unsupported = true;
                أرجع out;
            }
            CrossValueRange range{rangep, resolveWidth(rangep, exprp)};
            𐤀𐤌 (!resolveValue(rangep, exprp, true, true, range)) {
                𐤀𐤌 (report) {
                    rangep->v3warn(E_UNSUPPORTED, "Unsupported: non-integral value in a "
                                                  "coverage bin of an integral coverpoint.");
                }
                استمر;
            }
            𐤀𐤌 (crossRangeEmpty(range)) استمر;
            std::vector<std::pair<V3Number, V3Number>> found;
            crossRangeRuns(range, v3Global.opt.coverageMaxBins(), found);
            لكل (ثابت std::pair<V3Number, V3Number>& run : found) {
                // Coverpoint values, sign-extended in both widths
                spans.emplace_back(V3Number{rangep, width, run.first},
                                   V3Number{rangep, width, run.second});
            }
            std::sort(spans.begin(), spans.end(), [](ثابت auto& lhs, ثابت auto& rhs) {
                أرجع crossValueLess(lhs.first, rhs.first);
            });
            std::vector<std::pair<V3Number, V3Number>> merged;
            لكل (ثابت std::pair<V3Number, V3Number>& span : spans) {
                // Adjacent or overlapping values join a run; 'first - 1' cannot overflow
                V3Number before{rangep, width};
                before.opSub(span.first, one);
                𐤀𐤌 (merged.empty() || crossValueLess(merged.back().second, before)) {
                    merged.push_back(span);
                } وإلا 𐤀𐤌 (crossValueLess(merged.back().second, span.second)) {
                    merged.back().second = span.second;
                }
            }
            count = 0;
            لكل (ثابت std::pair<V3Number, V3Number>& span : merged) {
                V3Number size{rangep, width};
                size.opSub(span.second, span.first);
                // Beyond 2^32 values exceed any limit
                count += size.mostSetBitP1() > 32 ? uint64_t{1} << 33 : size.toUQuad() + 1;
                𐤀𐤌 (count > v3Global.opt.coverageMaxBins()) اكسر;
            }
            spans = std::move(merged);
            𐤀𐤌 (count > v3Global.opt.coverageMaxBins()) {
                // An ignore or illegal array still excludes or checks its values, as one bin
                out.single = !arrayBinp->binsType().binIsNormal();
                𐤀𐤌 (report) {
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
                out.unsupported = true;
                أرجع out;
            }
        }
        لكل (ثابت std::pair<V3Number, V3Number>& span : spans) {
            out.runs.emplace_back(arrayBinp, width, 0);
            BinRun& run = out.runs.back();
            run.m_lo = span.first;
            run.m_hi = span.second;
            // A bin for each value, named by it in the coverpoint's type
            V3Number value = span.first;
            طالما (true) {
                ثابت V3Number typed{arrayBinp, exprp->width(), value};
                out.values.push_back(exprp->isSigned() ? typed.toDecimalS() : typed.toDecimalU());
                ++run.m_count;
                𐤀𐤌 (value.isCaseEq(span.second)) اكسر;
                V3Number next{arrayBinp, width};
                value = next.opAdd(value, one);
            }
        }
        out.count = static_cast<uint32_t>(count);
        أرجع out;
    }

    // The runs of an automatic bins declaration, or of an array bin of an integral coverpoint.
    // False for other bins, which do not generate as runs, including a wildcard array then one
    // bin (see BinRuns::single).
    منطقي binRunsFor(AstCoverBin* binp, AstNodeExpr* exprp, BinRuns& out) {
        𐤀𐤌 (isAutoBins(binp)) {
            𐤀𐤌 (!autoBinRuns(binp, exprp, out)) out.unsupported = true;
            أرجع true;
        }
        𐤀𐤌 (binp->isArray() && binp->isWildcard()) {
            𐤀𐤌 (!exprp->dtypep()->skipRefp()->isIntegralOrPacked()) {
                AstNodeExpr* ثابت falsep = wildcardTypeError(binp, exprp);
                VL_DO_DANGLING(pushDeletep(falsep), falsep);
                out.unsupported = true;
            } وإلا {
                out = wildcardBinRuns(binp, exprp, true);
                𐤀𐤌 (out.single) {
                    binp->isArray(خطأ);  // Generates as one bin
                    أرجع خطأ;
                }
            }
            أرجع true;
        }
        𐤀𐤌 (!binp->isArray() || binp->transp() || binp->isWildcard()
            || !exprp->dtypep()->skipRefp()->isIntegralOrPacked()) {
            أرجع خطأ;
        }
        out = arrayBinRuns(binp, exprp);
        أرجع true;
    }

    // Emit a 'this->m_cp->addSingleNamer/addArrayNamer(...)' statement for one bin whose first
    // runtime bin index is 'declared'; or with 'valueNames', those naming each bin of an array
    AstNodeStmt* makeNamer(AstVar* cpVarp, AstCoverBin* binp, int64_t count, uint32_t declared,
                           ثابت std::vector<AstNodeExpr*>& values = {},
                           ثابت std::vector<std::string>& valueNames = {}) {
        FileLine* ثابت fl = binp->fileline();
        CoverpointBins& bins = m_cpBins.at(cpVarp);
        ثابت uint32_t normalCount
            = binp->binsType().binIsNormal() ? static_cast<uint32_t>(count < 0 ? 1 : count) : 0;
        ثابت BinSpan span{bins.total, normalCount, declared};
        𐤀𐤌 (binp->binsType() == VCoverBinsType::BINS_AUTO_IMPLICIT) {
            bins.implicitAuto = span;  // Selected by bin, see implicitAutoBinSpan
        } وإلا {
            bins.spans.emplace(binp->name(), span);
        }
        bins.total += normalCount;
        لكل (uint32_t i = 0; bins.crossed && i < normalCount; ++i) {
            bins.values.push_back({binp, values.empty() ? nullptr : values[i]});
        }
        // Under --protect-ids the filename and bin name flow into the coverage database
        // verbatim, so obfuscate them exactly as line/toggle coverage points are (whole-
        // unit filename, per-word bin name).  A no-op when --protect-ids is off.
        ثابت منطقي prot = v3Global.opt.protectIds();
        ثابت منطقي single = count < 0;
        𐤀𐤌 (!valueNames.empty()) {
            // A bin of a wildcard array is named by its value (IEEE 1800-2023 19.5.1)
            AstNodeStmt* stmtsp = nullptr;
            لكل (ثابت std::string& value : valueNames) {
                ثابت std::string name
                    = VIdProtect::protectWordsIf(binp->name(), prot) + "[" + value + "]";
                stmtsp = AstNode::addNext(
                    stmtsp,
                    itemCall(fl, cpVarp, VCMethod::COVERGROUP_ADD_SINGLE_NAMER,
                             {ctext(fl, binp->binsType().binSetEnum()), ctext(fl, quoted(name)),
                              ctext(fl, quoted(VIdProtect::protectIf(fl->filename(), prot))),
                              cnum(fl, static_cast<uint32_t>(fl->lineno())),
                              cnum(fl, static_cast<uint32_t>(fl->firstColumn()))})
                        ->makeStmt());
            }
            أرجع stmtsp;
        }
        std::vector<AstNodeExpr*> args{ctext(fl, binp->binsType().binSetEnum())};
        𐤀𐤌 (!single) args.push_back(cnum(fl, static_cast<uint32_t>(count)));  // value array bin
        args.push_back(ctext(fl, quoted(VIdProtect::protectWordsIf(binp->name(), prot))));
        args.push_back(ctext(fl, quoted(VIdProtect::protectIf(fl->filename(), prot))));
        args.push_back(cnum(fl, static_cast<uint32_t>(fl->lineno())));
        args.push_back(cnum(fl, static_cast<uint32_t>(fl->firstColumn())));
        أرجع itemCall(fl, cpVarp,
                        single ? VCMethod::COVERGROUP_ADD_SINGLE_NAMER
                        : binp->binsType() == VCoverBinsType::BINS_AUTO_IMPLICIT
                            ? VCMethod::COVERGROUP_ADD_NUMBERED_NAMER
                            : VCMethod::COVERGROUP_ADD_ARRAY_NAMER,
                        args)
            ->makeStmt();
    }

    // Emit 'if (iff && cond) m_cp.incrementBin(idx);' (or recordHit, + illegal action) in sample()
    // Where a bin's hit is recorded in the runtime VlCoverpoint member.
    بنية ConvBinTarget final {
        AstVar* cpVarp;  // the __Vcp_<coverpoint> member
        uint32_t idx;  // bin index within that coverpoint
        منطقي isNormal;  // Normal -> incrementBin (count + cross hit list); else recordHit (count)
    };

    // Emit 'this->m_cp.incrementBin(idx);' (Normal) or '.recordHit(idx);'
    // (ignore/illegal/default).
    AstNodeStmt* makeRuntimeBinHit(FileLine* fl, AstVar* cpVarp, AstNodeExpr* idxp,
                                   منطقي isNormal) {
        أرجع itemCall(fl, cpVarp,
                        isNormal ? VCMethod::COVERGROUP_INCREMENT_BIN
                                 : VCMethod::COVERGROUP_RECORD_HIT,
                        {idxp})
            ->makeStmt();
    }
    AstNodeStmt* makeRuntimeBinHit(FileLine* fl, ثابت ConvBinTarget& tgt) {
        أرجع makeRuntimeBinHit(fl, tgt.cpVarp, cnum(fl, static_cast<uint32_t>(tgt.idx)),
                                 tgt.isNormal);
    }

    // The condition under which a bin counts a sample: condp, and the bin's iff, and for a
    // Normal or default state bin, that the value is not excluded
    AstNodeExpr* binCondition(AstCoverBin* binp, AstVar* cpVarp, AstNodeExpr* condp) {
        FileLine* ثابت fl = binp->fileline();
        𐤀𐤌 (binp->iffp()) condp = جديد AstLogAnd{fl, binp->iffp()->cloneTree(خطأ), condp};
        ثابت auto excluded = m_excludedVars.find(cpVarp);
        𐤀𐤌 (excluded != m_excludedVars.end() && !binp->transp()
            && (binp->binsType().binIsNormal()
                || binp->binsType() == VCoverBinsType::BINS_DEFAULT)) {
            condp = جديد AstLogAnd{
                fl, جديد AstNot{fl, جديد AstVarRef{fl, excluded->second, VAccess::READ}}, condp};
        }
        أرجع condp;
    }

    فراغ emitConvHitIf(AstCoverpoint* coverpointp, AstCoverBin* binp, AstVar* cpVarp,
                       AstNodeExpr* idxp, AstNodeExpr* condp) {
        FileLine* ثابت fl = binp->fileline();
        AstNode* actionp = makeRuntimeBinHit(fl, cpVarp, idxp, binp->binsType().binIsNormal());
        𐤀𐤌 (binp->binsType() == VCoverBinsType::BINS_ILLEGAL) {
            actionp->addNext(makeIllegalBinAction(fl, "Illegal bin " + binp->prettyNameQ()
                                                          + " hit in coverpoint "
                                                          + coverpointp->prettyNameQ()));
        }
        UASSERT_OBJ(m_sampleFuncp, binp, "sample() CFunc not set for coverpoint");
        addSampleStmt(جديد AstIf{fl, binCondition(binp, cpVarp, condp), actionp, nullptr});
    }

    // Emit the sample() code of sized array 'sized': count the value in its bins holding it when
    // enabled, as other bins are, and for default bins note in matchedp whether any holds it
    فراغ emitSizedSample(AstCoverpoint* coverpointp, AstCoverBin* binp, AstVar* cpVarp,
                         AstNodeExpr* exprp, uint32_t sized, AstVar* matchedp) {
        FileLine* ثابت fl = binp->fileline();
        AstNodeExpr* enabledp = binCondition(binp, cpVarp, جديد AstConst{fl, AstConst::BitTrue{}});
        UASSERT_OBJ(m_sampleFuncp, binp, "sample() CFunc not set for coverpoint");
        ثابت منطقي illegal = binp->binsType() == VCoverBinsType::BINS_ILLEGAL;
        𐤀𐤌 (illegal) {
            // The illegal action reads the condition too, which is evaluated once, as the guard
            // may have side effects
            AstVar* ثابت varp
                = جديد AstVar{fl, VVarType::BLOCKTEMP,
                             "__VcpEnabled_" + sanitizeGeneratedName(coverpointp->name()) + "_"
                                 + cvtToStr(sized),
                             binp->findBitDType()};
            varp->funcLocal(true);
            m_sampleFuncp->addStmtsp(varp);
            addSampleStmt(جديد AstAssign{fl, جديد AstVarRef{fl, varp, VAccess::WRITE}, enabledp});
            enabledp = جديد AstVarRef{fl, varp, VAccess::READ};
        }
        AstCMethodHard* ثابت callp
            = itemCall(fl, cpVarp,
                       exprp->isWide() ? VCMethod::COVERGROUP_SIZED_SAMPLE_W
                                       : VCMethod::COVERGROUP_SIZED_SAMPLE,
                       {cnum(fl, sized), exprp->cloneTree(خطأ), enabledp});
        callp->dtypeSetBit();
        𐤀𐤌 (illegal) {
            addSampleStmt(جديد AstIf{fl, جديد AstLogAnd{fl, callp, enabledp->cloneTree(خطأ)},
                                    makeIllegalBinAction(fl, "Illegal bin " + binp->prettyNameQ()
                                                                 + " hit in coverpoint "
                                                                 + coverpointp->prettyNameQ())});
        } وإلا 𐤀𐤌 (matchedp && binp->binsType().binIsNormal()) {
            addSampleStmt(
                جديد AstAssign{fl, جديد AstVarRef{fl, matchedp, VAccess::WRITE},
                              جديد AstOr{fl, جديد AstVarRef{fl, matchedp, VAccess::READ}, callp}});
        } وإلا {
            addSampleStmt(callp->makeStmt());
        }
    }

    // A variable local to the constructor
    AstVar* constructorTemp(FileLine* fl, ثابت string& name, AstNodeDType* dtypep) {
        AstVar* ثابت varp = جديد AstVar{fl, VVarType::BLOCKTEMP, name, dtypep};
        varp->funcLocal(true);
        m_constructorp->addStmtsp(varp);
        أرجع varp;
    }

    // Truncate or extend, as its signedness sets, a value to a type
    ساكن AstNodeExpr* resizeValue(AstNodeExpr* valuep, AstNodeDType* dtypep) {
        FileLine* ثابت fl = valuep->fileline();
        𐤀𐤌 (valuep->width() > dtypep->width()) {
            valuep = جديد AstSel{fl, valuep, 0, dtypep->width()};
        } وإلا 𐤀𐤌 (valuep->width() < dtypep->width()) {
            valuep = valuep->isSigned()
                         ? static_cast<AstNodeExpr*>(جديد AstExtendS{fl, valuep, dtypep->width()})
                         : جديد AstExtend{fl, valuep, dtypep->width()};
        }
        valuep->dtypep(dtypep);
        أرجع valuep;
    }

    // The runtime arguments of a count, which the constructor evaluates once into a temporary:
    // its value, a count wider than 64 bits saturated over any limit, and whether it is positive
    std::pair<AstNodeExpr*, AstNodeExpr*> newCountArgs(FileLine* fl, ثابت string& name,
                                                       AstNodeExpr* exprp) {
        AstVar* ثابت countp = constructorTemp(fl, name, exprp->dtypep());
        m_constructorp->addStmtsp(
            جديد AstAssign{fl, جديد AstVarRef{fl, countp, VAccess::WRITE}, exprp->cloneTree(خطأ)});
        ثابت auto countRef = [&]() { أرجع جديد AstVarRef{fl, countp, VAccess::READ}; };
        AstConst* ثابت zerop = جديد AstConst{fl, AstConst::DTyped{}, countp->dtypep()};
        AstNodeExpr* ثابت positivep
            = countp->isSigned() ? static_cast<AstNodeExpr*>(جديد AstGtS{fl, countRef(), zerop})
                                 : جديد AstNeq{fl, countRef(), zerop};
        AstNodeExpr* valuep = resizeValue(countRef(), countp->findUInt64DType());
        𐤀𐤌 (countp->width() > VL_QUADSIZE) {
            valuep = جديد AstCond{
                fl,
                جديد AstRedOr{
                    fl, جديد AstSel{fl, countRef(), VL_QUADSIZE, countp->width() - VL_QUADSIZE}},
                جديد AstConst{fl, AstConst::Unsized64{}, std::numeric_limits<uint64_t>::max()},
                valuep};
            valuep->dtypeSetUInt64();
        }
        أرجع {valuep, positivep};
    }
    // The runtime arguments of a constant count
    ساكن std::pair<AstNodeExpr*, AstNodeExpr*> newCountArgs(FileLine* fl, uint64_t count) {
        أرجع {جديد AstConst{fl, AstConst::Unsized64{}, count},
                جديد AstConst{fl, AstConst::BitTrue{}}};
    }

    // The values of a coverpoint, which its name denotes (IEEE 1800-2023 19.5.1.1), as runs in
    // value order: an enumerated type's values (6.19), else all values of its type.  Bounds
    // are at runWidth(), sign-extended like a CrossValueRange's.
    ساكن std::vector<std::pair<V3Number, V3Number>> coverpointValues(AstNode* nodep,
                                                                       AstNodeExpr* exprp) {
        ثابت صحيح width = runWidth(exprp);
        std::vector<std::pair<V3Number, V3Number>> runs;
        ثابت AstEnumDType* ثابت enump = VN_CAST(exprp->dtypep()->skipRefToEnump(), EnumDType);
        𐤀𐤌 (!enump) {
            ثابت CrossValueRange domain
                = crossValueDomain(nodep, exprp->width(), exprp->isSigned(), width);
            runs.emplace_back(domain.lo, domain.hi);
            أرجع runs;
        }
        std::vector<V3Number> values;
        لكل (ثابت AstEnumItem* itemp = enump->itemsp(); itemp;
             itemp = VN_AS(itemp->nextp(), EnumItem)) {
            ثابت V3Number& num = VN_AS(itemp->valuep(), Const)->num();
            𐤀𐤌 (num.isFourState()) استمر;  // Not a coverpoint value (19.5.7)
            values.emplace_back(nodep, width);
            𐤀𐤌 (exprp->isSigned()) {
                values.back().opExtendS(num, num.width());
            } وإلا {
                values.back().opAssign(num);
            }
        }
        std::sort(values.begin(), values.end(), crossValueLess);
        ثابت V3Number one{nodep, width, 1};
        لكل (ثابت V3Number& value : values) {
            V3Number next{nodep, width};
            𐤀𐤌 (!runs.empty() && next.opAdd(runs.back().second, one).isCaseEq(value)) {
                runs.back().second = value;
            } وإلا {
                runs.emplace_back(value, value);
            }
        }
        أرجع runs;
    }

    // Emit the constructor code building the bins 'binp' whose values it computes: the values of
    // each element that are coverpoint values (IEEE 1800-2023 19.5.7), those a 'with' filter
    // keeps (19.5.1.1), then its bins
    فراغ generateConstructedBins(AstCoverpoint* coverpointp, AstCoverBin* binp, AstVar* cpVarp,
                                 AstNodeExpr* exprp) {
        FileLine* ثابت fl = binp->fileline();
        ثابت string prefix
            = m_sizedNames.get(sanitizeGeneratedName(coverpointp->name() + "__" + binp->name()));
        // The count, and whether it is positive; one of a filter's scalar bin, or bin per value
        ثابت std::pair<AstNodeExpr*, AstNodeExpr*> count
            = binp->arraySizep() ? newCountArgs(fl, prefix + "_count", binp->arraySizep())
                                 : newCountArgs(fl, 1);
        AstCoverWith* ثابت withp = binWith(binp);
        𐤀𐤌 (withp && !binRangesp(binp)) {  // The coverpoint's name: all of its values
            لكل (ثابت std::pair<V3Number, V3Number>& run : coverpointValues(binp, exprp)) {
                m_constructorp->addStmtsp(itemCall(fl, cpVarp,
                                                   exprp->isWide()
                                                       ? VCMethod::COVERGROUP_SIZED_RANGE_W
                                                       : VCMethod::COVERGROUP_SIZED_RANGE,
                                                   {newValueConst(fl, run.first, exprp),
                                                    newValueConst(fl, run.second, exprp)})
                                              ->makeStmt());
            }
        }
        uint32_t element = 0;
        لكل (AstNode* rangep = binRangesp(binp); rangep; rangep = rangep->nextp()) {
            𐤀𐤌 (VN_IS(rangep, Unbounded)) {  // A parameter of '$'
                binp->v3error("Bins value may not be '$', which may only bound a range "
                              "(IEEE 1800-2023 6.20.7)");
                استمر;
            }
            generateSizedElement(cpVarp, binp, rangep, exprp, prefix + "_" + cvtToStr(element++));
        }
        𐤀𐤌 (withp) generateWithFilter(binp, withp, cpVarp, exprp, prefix);
        std::vector<AstNodeExpr*> args{ctext(fl, binp->binsType().binSetEnum()), count.first,
                                       count.second};
        // A filter's bins have the limit it began with
        𐤀𐤌 (!withp) args.push_back(cnum(fl, v3Global.opt.coverageMaxBins()));
        ثابت منطقي prot = v3Global.opt.protectIds();
        args.push_back(ctext(fl, quoted(VIdProtect::protectWordsIf(binp->name(), prot))));
        args.push_back(ctext(fl, quoted(VIdProtect::protectIf(fl->filename(), prot))));
        args.push_back(cnum(fl, static_cast<uint32_t>(fl->lineno())));
        args.push_back(cnum(fl, static_cast<uint32_t>(fl->firstColumn())));
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
    فراغ generateWithFilter(AstCoverBin* binp, AstCoverWith* withp, AstVar* cpVarp,
                            AstNodeExpr* exprp, ثابت string& prefix) {
        FileLine* ثابت fl = withp->fileline();
        ثابت منطقي wide = exprp->isWide();
        ثابت string grouping = !binp->isArray()     ? "Single"
                                : binp->arraySizep() ? "Fixed"
                                                     : "Values";
        m_constructorp->addStmtsp(itemCall(fl, cpVarp, VCMethod::COVERGROUP_WITH_BEGIN,
                                           {ctext(fl, "VlCovBinGrouping::" + grouping),
                                            cnum(fl, v3Global.opt.coverageMaxBins())})
                                      ->makeStmt());
        // The candidates count in the coverpoint's width, and the filter reads each as 'item',
        // of the coverpoint's type, so that a filter changing 'item' cannot change the loop
        AstNodeDType* ثابت valueDTypep = exprp->dtypep();
        AstVar* ثابت valuep = constructorTemp(fl, prefix + "_value", valueDTypep);
        AstVar* ثابت lastp = constructorTemp(fl, prefix + "_last", valueDTypep);
        AstVar* ثابت firstp = constructorTemp(fl, prefix + "_first", valueDTypep);
        AstVar* ثابت runp = constructorTemp(fl, prefix + "_run", binp->findBitDType());
        AstVar* ثابت morep = constructorTemp(fl, prefix + "_more", binp->findBitDType());
        AstVar* ثابت itemp = withp->itemp()->unlinkFrBack();
        itemp->name(prefix + "_item");
        m_constructorp->addStmtsp(itemp);
        ثابت auto ref = [&](AstVar* varp) { أرجع جديد AstVarRef{fl, varp, VAccess::READ}; };
        ثابت auto assign = [&](AstVar* varp, AstNodeExpr* rhsp) -> AstNode* {
            أرجع جديد AstAssign{fl, جديد AstVarRef{fl, varp, VAccess::WRITE}, rhsp};
        };
        ثابت auto flag = [&](AstVar* varp, منطقي value) {
            أرجع assign(varp, value ? جديد AstConst{fl, AstConst::BitTrue{}}
                                      : جديد AstConst{fl, AstConst::BitFalse{}});
        };
        ثابت auto bound = [&](VCMethod narrow, VCMethod wideMethod, AstVar* varp) -> AstNode* {
            𐤀𐤌 (wide) {
                أرجع itemCall(fl, cpVarp, wideMethod, {جديد AstVarRef{fl, varp, VAccess::WRITE}})
                    ->makeStmt();
            }
            AstCMethodHard* ثابت callp = itemCall(fl, cpVarp, narrow);
            callp->dtypeSetUInt64();
            أرجع assign(varp, resizeValue(callp, valueDTypep));
        };
        ثابت auto keep = [&](AstNodeExpr* lop, AstNodeExpr* hip) {
            AstCMethodHard* ثابت callp = itemCall(
                fl, cpVarp, wide ? VCMethod::COVERGROUP_WITH_RUN_W : VCMethod::COVERGROUP_WITH_RUN,
                {lop, hip});
            callp->dtypeSetBit();
            أرجع assign(morep, callp);
        };
        ثابت auto step = [&](منطقي up) {
            AstConst* ثابت onep = جديد AstConst{fl, AstConst::WidthedValue{}, exprp->width(), 1};
            AstNodeExpr* ثابت stepp
                = up ? static_cast<AstNodeExpr*>(جديد AstAdd{fl, ref(valuep), onep})
                     : جديد AstSub{fl, ref(valuep), onep};
            stepp->dtypep(valueDTypep);
            أرجع stepp;
        };
        AstLoop* ثابت innerp = جديد AstLoop{fl};
        innerp->addStmtsp(assign(itemp, ref(valuep)));
        innerp->addStmtsp(جديد AstIf{
            fl, withp->filterp()->unlinkFrBack(),
            جديد AstIf{fl, جديد AstNot{fl, ref(runp)},
                      assign(firstp, ref(valuep))->addNext(flag(runp, true))},
            جديد AstIf{fl, ref(runp), keep(ref(firstp), step(خطأ))->addNext(flag(runp, خطأ))}});
        innerp->addStmtsp(جديد AstLoopTest{
            fl, innerp, جديد AstLogAnd{fl, ref(morep), جديد AstNeq{fl, ref(valuep), ref(lastp)}}});
        innerp->addStmtsp(assign(valuep, step(true)));
        AstLoop* ثابت outerp = جديد AstLoop{fl};
        AstCMethodHard* ثابت nextp = itemCall(fl, cpVarp, VCMethod::COVERGROUP_WITH_NEXT);
        nextp->dtypeSetBit();
        outerp->addStmtsp(جديد AstLoopTest{fl, outerp, nextp});
        outerp->addStmtsp(
            bound(VCMethod::COVERGROUP_WITH_LO, VCMethod::COVERGROUP_WITH_LO_W, valuep));
        outerp->addStmtsp(
            bound(VCMethod::COVERGROUP_WITH_HI, VCMethod::COVERGROUP_WITH_HI_W, lastp));
        outerp->addStmtsp(flag(runp, خطأ));
        outerp->addStmtsp(innerp);
        outerp->addStmtsp(جديد AstIf{fl, ref(runp), keep(ref(firstp), ref(lastp))});
        m_constructorp->addStmtsp(flag(morep, true));
        m_constructorp->addStmtsp(outerp);
    }

    // Emit 'sizedRange(lo, hi)' for the coverpoint values of an element of bins 'binp', whose
    // values the constructor computes: resolved now if constant, else when constructed by
    // clipping to the coverpoint's values.  A wildcard pattern's values give a range for each
    // run of them.  'prefix' names its temporaries.
    فراغ generateSizedElement(AstVar* cpVarp, ثابت AstCoverBin* binp, AstNode* rangep,
                              AstNodeExpr* exprp, ثابت string& prefix) {
        FileLine* ثابت fl = rangep->fileline();
        ثابت VCMethod method = exprp->isWide() ? VCMethod::COVERGROUP_SIZED_RANGE_W
                                                : VCMethod::COVERGROUP_SIZED_RANGE;
        ثابت AstInsideRange* ثابت irp = VN_CAST(rangep, InsideRange);
        AstNodeExpr* ثابت lowp = irp ? irp->lhsp() : VN_AS(rangep, NodeExpr);
        AstNodeExpr* ثابت highp = irp ? irp->rhsp() : nullptr;
        ثابت auto unbounded
            = [](ثابت AstNodeExpr* boundp) { أرجع !boundp || VN_IS(boundp, Unbounded); };
        ثابت auto constant
            = [&](ثابت AstNodeExpr* boundp) { أرجع unbounded(boundp) || VN_IS(boundp, Const); };
        ثابت auto integral = [&](ثابت AstNodeExpr* boundp) {
            أرجع unbounded(boundp) || boundp->dtypep()->skipRefp()->isIntegralOrPacked();
        };
        𐤀𐤌 (constant(lowp) && constant(highp)) {
            ثابت auto fourState = [](ثابت AstNodeExpr* boundp) {
                ثابت AstConst* ثابت constp = VN_CAST(boundp, Const);
                أرجع constp && constp->num().isFourState();
            };
            𐤀𐤌 (irp && (fourState(lowp) || fourState(highp))) {
                rangep->v3error("Four-state (x/z) value in "
                                << (binp->isArray() ? "array bins" : "bin")
                                << " range bound; range bounds must be two-state constants");
                أرجع;
            }
            CrossValueRange range{rangep, resolveWidth(rangep, exprp)};
            𐤀𐤌 (!resolveValue(rangep, exprp, true, binp->isWildcard(), range)) {
                rangep->v3warn(E_UNSUPPORTED, "Unsupported: non-integral value in a coverage bin "
                                              "of an integral coverpoint.");
            } وإلا 𐤀𐤌 (!crossRangeEmpty(range)) {
                std::vector<std::pair<V3Number, V3Number>> runs{{range.lo, range.hi}};
                𐤀𐤌 (range.wildcard) {  // checkConstructedBins bounded the runs
                    runs.clear();
                    crossRangeRuns(range, v3Global.opt.coverageMaxBins(), runs);
                }
                لكل (ثابت std::pair<V3Number, V3Number>& run : runs) {
                    m_constructorp->addStmtsp(itemCall(fl, cpVarp, method,
                                                       {newValueConst(fl, run.first, exprp),
                                                        newValueConst(fl, run.second, exprp)})
                                                  ->makeStmt());
                }
            }
            أرجع;
        }
        𐤀𐤌 (!integral(lowp) || !integral(highp)) {
            rangep->v3warn(E_UNSUPPORTED, "Unsupported: non-integral value in a coverage bin "
                                          "of an integral coverpoint.");
            أرجع;
        }
        // Compare values and the coverpoint's domain signed, in a width holding all of them
        ثابت auto boundWidth
            = [&](ثابت AstNodeExpr* boundp) { أرجع unbounded(boundp) ? 0 : boundp->width(); };
        ثابت صحيح width = std::max({exprp->width(), boundWidth(lowp), boundWidth(highp)}) + 1;
        ثابت CrossValueRange domain
            = crossValueDomain(rangep, exprp->width(), exprp->isSigned(), width);
        AstVar* ثابت lop = constructorTemp(fl, prefix + "_lo", exprp->dtypep());
        lop->dtypeSetBitSized(width, VSigning::SIGNED);
        AstVar* ثابت hip = constructorTemp(fl, prefix + "_hi", lop->dtypep());
        ثابت auto bound = [&](AstNodeExpr* boundp, ثابت V3Number& limit) -> AstNodeExpr* {
            𐤀𐤌 (unbounded(boundp)) {
                AstConst* ثابت limitp = جديد AstConst{fl, limit};
                limitp->dtypeFrom(lop);
                أرجع limitp;
            }
            AstNodeExpr* valuep = boundp->cloneTree(خطأ);
            // A value no wider than a signed coverpoint has its type (see crossRangeBound)
            𐤀𐤌 (exprp->isSigned() && boundp->width() <= exprp->width()) {
                valuep = resizeValue(valuep, exprp->dtypep());
            }
            أرجع resizeValue(valuep, lop->dtypep());
        };
        ثابت auto ref = [&](AstVar* varp, VAccess access = VAccess::READ) {
            أرجع جديد AstVarRef{fl, varp, access};
        };
        m_constructorp->addStmtsp(
            جديد AstAssign{fl, ref(lop, VAccess::WRITE), bound(lowp, domain.lo)});
        m_constructorp->addStmtsp(
            جديد AstAssign{fl, ref(hip, VAccess::WRITE), irp ? bound(highp, domain.hi) : ref(lop)});
        AstConst* ثابت minp = جديد AstConst{fl, domain.lo};
        AstConst* ثابت maxp = جديد AstConst{fl, domain.hi};
        minp->dtypeFrom(lop);
        maxp->dtypeFrom(lop);
        AstCond* ثابت lowerp
            = جديد AstCond{fl, جديد AstLtS{fl, ref(lop), minp}, minp->cloneTree(خطأ), ref(lop)};
        AstCond* ثابت upperp
            = جديد AstCond{fl, جديد AstGtS{fl, ref(hip), maxp}, maxp->cloneTree(خطأ), ref(hip)};
        lowerp->dtypeFrom(lop);
        upperp->dtypeFrom(lop);
        m_constructorp->addStmtsp(جديد AstAssign{fl, ref(lop, VAccess::WRITE), lowerp});
        m_constructorp->addStmtsp(جديد AstAssign{fl, ref(hip, VAccess::WRITE), upperp});
        m_constructorp->addStmtsp(جديد AstIf{fl, جديد AstLteS{fl, ref(lop), ref(hip)},
                                            itemCall(fl, cpVarp, method,
                                                     {resizeValue(ref(lop), exprp->dtypep()),
                                                      resizeValue(ref(hip), exprp->dtypep())})
                                                ->makeStmt()});
    }

    // The runtime index of the bin of a run holding the coverpoint value, which is in the run:
    // declared + (value - lo) / stride, capped at the last bin, which holds any remainder.
    ساكن AstNodeExpr* runBinIndex(FileLine* fl, AstNodeExpr* exprp, ثابت BinRun& run) {
        𐤀𐤌 (run.m_count == 1) أرجع cnum(fl, run.m_declared);
        ثابت صحيح width = exprp->width();
        // A run spans at most 2^width values, so offsets in it are unsigned width-bit numbers
        AstNodeExpr* indexp
            = جديد AstSub{fl, exprp->cloneTree(خطأ), newValueConst(fl, run.m_lo, exprp)};
        indexp->dtypeSetBitSized(width, VSigning::UNSIGNED);
        V3Number stride{fl, width, 0};
        stride.opAssign(run.m_stride);
        𐤀𐤌 (stride.countOnes() != 1) {
            indexp = جديد AstDiv{fl, indexp, جديد AstConst{fl, stride}};
        } وإلا 𐤀𐤌 (!stride.isEqOne()) {
            indexp = جديد AstShiftR{fl, indexp, جديد AstConst{fl, stride.mostSetBitP1() - 1}};
        }
        // Compare the run's values with those of count bins of stride values, without overflow
        ثابت صحيح extWidth = run.m_lo.width() + 1;
        V3Number lo{fl, extWidth, 0};
        lo.opExtendS(run.m_lo, run.m_lo.width());
        V3Number span{fl, extWidth, 0};
        span.opExtendS(run.m_hi, run.m_hi.width());
        span.opSub(V3Number{span}, lo);
        V3Number covered{fl, extWidth, 0};
        covered.opAssign(run.m_stride);
        covered.opMul(V3Number{covered}, V3Number{fl, extWidth, run.m_count});
        V3Number hasRemainder{fl, 1, 0};
        𐤀𐤌 (!hasRemainder.opGte(span, covered).isEqZero()) {
            AstConst* ثابت lastp = جديد AstConst{fl, V3Number{fl, width, run.m_count - 1}};
            indexp = جديد AstCond{fl, جديد AstGt{fl, indexp->cloneTree(خطأ), lastp},
                                 lastp->cloneTree(خطأ), indexp};
        }
        𐤀𐤌 (width < VL_IDATASIZE) {
            indexp = جديد AstExtend{fl, indexp, VL_IDATASIZE};
        } وإلا 𐤀𐤌 (width > VL_IDATASIZE) {
            indexp = جديد AstSel{fl, indexp, 0, VL_IDATASIZE};
        }
        أرجع جديد AstAdd{fl, cnum(fl, run.m_declared), indexp};
    }

    // Emit the sample() hit of a run of bins, whose code does not grow with its number of bins:
    //   if (iff && lo <= value && value <= hi) m_cp.incrementBin(<runBinIndex>);
    فراغ emitRunHit(AstCoverpoint* coverpointp, AstCoverBin* binp, AstVar* cpVarp,
                    AstNodeExpr* exprp, ثابت BinRun& run) {
        FileLine* ثابت fl = binp->fileline();
        AstConst* ثابت lop = newValueConst(fl, run.m_lo, exprp);
        AstNodeExpr* condp = nullptr;
        𐤀𐤌 (run.m_lo.isCaseEq(run.m_hi)) {
            condp = جديد AstEq{fl, exprp->cloneTree(خطأ), lop};
        } وإلا {
            AstConst* ثابت hip = newValueConst(fl, run.m_hi, exprp);
            condp = makeRangeCondition(fl, exprp, lop, hip);
            VL_DO_DANGLING(pushDeletep(lop), lop);
            VL_DO_DANGLING(pushDeletep(hip), hip);
        }
        emitConvHitIf(coverpointp, binp, cpVarp, runBinIndex(fl, exprp, run), condp);
    }

    // Emit a transition bin's hit action into sample():
    //   if (cond) { m_cp.incrementBin/recordHit(idx); [illegal: $error; $stop] }
    // Used by the transition generators so a completed sequence records into the runtime bin.
    فراغ addConvTransHitIf(AstCoverpoint* coverpointp, AstCoverBin* binp, ثابت ConvBinTarget& tgt,
                           AstNodeExpr* condp) {
        FileLine* ثابت fl = binp->fileline();
        AstNode* actionp = makeRuntimeBinHit(fl, tgt);
        𐤀𐤌 (binp->binsType() == VCoverBinsType::BINS_ILLEGAL) {
            actionp->addNext(makeIllegalBinAction(
                fl, "Illegal transition bin " + binp->prettyNameQ() + " hit in coverpoint "
                        + coverpointp->prettyNameQ()));
        }
        UASSERT_OBJ(m_sampleFuncp, binp, "sample() CFunc not set for transition bin");
        addSampleStmt(جديد AstIf{fl, condp, actionp, nullptr});
    }

    // Route a coverpoint through a VlCoverpoint member: emit the member, its sample()
    // increments, the constructor configuration (init + namers), and registration.
    فراغ generateCoverpoint(AstCoverpoint* coverpointp, AstNodeExpr* exprp, صحيح atLeastValue) {
        FileLine* ثابت fl = coverpointp->fileline();
        ثابت منطقي dynamic = m_runtimePoints.count(coverpointp);
        UINFO(4, "  Generating VlCoverpoint member: " << coverpointp->name());

        // Size the hit list to the gen-time max bin overlap (1 unless cross-fed with
        // overlapping ranges), so no cross hit is ever dropped and storage is minimal.
        ثابت منطقي crossFed = m_crossedCpNames.count(coverpointp->name()) != 0;
        ثابت صحيح hitBound = computeHitListBound(coverpointp, exprp, crossFed);
        UINFO(6, "    Hit-list bound (max bin overlap) = " << hitBound);
        AstVar* ثابت cpVarp = جديد AstVar{fl, VVarType::MEMBER, "__Vcp_" + coverpointp->name(),
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
        𐤀𐤌 (crossFed) {
            UASSERT_OBJ(m_sampleFuncp, coverpointp, "sample() CFunc not set for clearHitList");
            m_sampleFuncp->addStmtsp(
                itemCall(fl, cpVarp, VCMethod::COVERGROUP_CLEAR_HIT_LIST)->makeStmt());
        }
        𐤀𐤌 (dynamic && coverpointHasStateExclusions(coverpointp)) {
            AstVar* ثابت excludedp
                = جديد AstVar{fl, VVarType::BLOCKTEMP,
                             "__VcpExcluded_" + sanitizeGeneratedName(coverpointp->name()),
                             coverpointp->findBitDType()};
            excludedp->funcLocal(true);
            m_sampleFuncp->addStmtsp(excludedp);
            AstCMethodHard* ثابت callp
                = itemCall(fl, cpVarp,
                           exprp->isWide() ? VCMethod::COVERGROUP_VALUE_EXCLUDED_W
                                           : VCMethod::COVERGROUP_VALUE_EXCLUDED,
                           {exprp->cloneTree(خطأ)});
            callp->dtypeSetBit();
            addSampleStmt(جديد AstAssign{fl, جديد AstVarRef{fl, excludedp, VAccess::WRITE}, callp});
            m_excludedVars.emplace(cpVarp, excludedp);
        }

        // Walk bins (non-default, then default), assigning sequential indices that match the
        // namer append order; emit sample increments and collect namer statements.  Constructed
        // bins follow them all, placed when the coverpoint is constructed.
        std::vector<AstNodeStmt*> namerStmts;
        std::vector<AstCoverBin*> defaultBins;
        std::vector<AstCoverBin*> sizedBins;
        std::vector<std::tuple<AstCoverBin*, uint32_t, AstNodeExpr*>> metadata;
        std::vector<ثابت BinRun*> runMetadata;
        uint64_t idx = 0;  // Runtime index of the next bin; 32-bit once checked below
        لكل (AstNode* binp = coverpointp->binsp(); binp; binp = binp->nextp()) {
            AstCoverBin* ثابت cbinp = VN_AS(binp, CoverBin);
            ثابت صحيح errorsBefore = dynamic ? V3Error::errorCount() : 0;
            𐤀𐤌 (cbinp->binsType() == VCoverBinsType::BINS_DEFAULT) {
                defaultBins.push_back(cbinp);
                استمر;
            }
            𐤀𐤌 (isConstructedBins(cbinp)) {
                UASSERT_OBJ(dynamic, cbinp, "Constructed bins without value metadata");
                BinSpan span;
                span.sized = static_cast<int32_t>(sizedBins.size());
                m_cpBins.at(cpVarp).spans.emplace(cbinp->name(), span);
                sizedBins.push_back(cbinp);
                استمر;
            }
            𐤀𐤌 (cbinp->transp()) {
                // Transition bin (incl. array transition 'bins t[] = (a=>b),(c=>d)' and
                // illegal_bins/ignore_bins transitions).  All sequences of one transition bin
                // share a bin name and merge in the coverage DB to a single point, so model
                // them as one runtime bin incremented by any matching sequence.  The sequence
                // matching is generated as a state machine, with the hit routed to this bin's
                // runtime slot.
                namerStmts.push_back(makeNamer(cpVarp, cbinp, -1, static_cast<uint32_t>(idx)));
                ثابت ConvBinTarget tgt{cpVarp, static_cast<uint32_t>(idx),
                                        cbinp->binsType().binIsNormal()};
                لكل (AstNode* sp = cbinp->transp(); sp; sp = sp->nextp())
                    generateSingleTransitionCode(coverpointp, cbinp, exprp, tgt,
                                                 VN_AS(sp, CoverTransSet));
                𐤀𐤌 (dynamic && V3Error::errorCount() == errorsBefore) {
                    metadata.emplace_back(cbinp, static_cast<uint32_t>(idx), nullptr);
                }
                ++idx;
                استمر;
            }
            BinRuns plan;
            𐤀𐤌 (binRunsFor(cbinp, exprp, plan)) {
                // Array elements and automatic bins generate as runs, so neither sample() nor
                // the constructor grows with their number of bins.
                𐤀𐤌 (plan.unsupported) {  // bin ignored or invalid; reserve no slot
                    m_droppedBins[coverpointp].push_back(cbinp->name());
                    استمر;
                }
                CoverpointBins& bins = m_cpBins.at(cpVarp);
                ثابت uint32_t firstValue = bins.total;
                ثابت uint32_t firstDeclared = static_cast<uint32_t>(idx);
                namerStmts.push_back(
                    makeNamer(cpVarp, cbinp, plan.count, firstDeclared, {}, plan.values));
                لكل (BinRun& run : plan.runs) {
                    run.m_declared = static_cast<uint32_t>(idx);
                    bins.runs.push_back(std::move(run));
                    ثابت BinRun& stored = bins.runs.back();
                    𐤀𐤌 (bins.crossed && cbinp->binsType().binIsNormal()) {
                        ثابت uint32_t first = firstValue + stored.m_declared - firstDeclared;
                        لكل (uint32_t element = 0; element < stored.m_count; ++element) {
                            bins.values[first + element].runp = &stored;
                            bins.values[first + element].element = element;
                        }
                    }
                    𐤀𐤌 (!stored.m_empty) {
                        emitRunHit(coverpointp, cbinp, cpVarp, exprp, stored);
                        𐤀𐤌 (dynamic && V3Error::errorCount() == errorsBefore) {
                            runMetadata.push_back(&stored);
                        }
                    }
                    idx += stored.m_count;
                }
                استمر;
            }
            𐤀𐤌 (cbinp->isArray()) {  // value array of a real coverpoint: b[0]..b[N-1]
                // Only integral coverpoints have runtime value metadata (m_runtimePoints)
                UASSERT_OBJ(!dynamic, cbinp, "Runtime value metadata for a real coverpoint");
                منطقي unsupported = خطأ;
                std::vector<AstNodeExpr*> values = extractArrayValues(cbinp, exprp, unsupported);
                𐤀𐤌 (unsupported) {  // bin ignored (COVERIGN emitted); reserve no slot
                    m_droppedBins[coverpointp].push_back(cbinp->name());
                    استمر;
                }
                namerStmts.push_back(makeNamer(cpVarp, cbinp, static_cast<int64_t>(values.size()),
                                               static_cast<uint32_t>(idx), values));
                لكل (AstNodeExpr* valuep : values) {
                    // The cross selections of this covergroup still read the value.
                    m_detachedValues.push_back(valuep);
                    emitConvHitIf(coverpointp, cbinp, cpVarp,
                                  cnum(cbinp->fileline(), static_cast<uint32_t>(idx)),
                                  buildValueCondition(cbinp, exprp, valuep));
                    ++idx;
                }
            } وإلا {
                namerStmts.push_back(makeNamer(cpVarp, cbinp, -1, static_cast<uint32_t>(idx)));
                // buildBinCondition is null for 'ignore_bins = default' (no ranges); the bin
                // still gets a reserved slot (recorded, never incremented).
                𐤀𐤌 (AstNodeExpr* ثابت condp = buildBinCondition(cbinp, exprp))
                    emitConvHitIf(coverpointp, cbinp, cpVarp,
                                  cnum(cbinp->fileline(), static_cast<uint32_t>(idx)), condp);
                𐤀𐤌 (dynamic && V3Error::errorCount() == errorsBefore) {
                    metadata.emplace_back(cbinp, static_cast<uint32_t>(idx), nullptr);
                }
                ++idx;
            }
        }
        // A cross selecting an ignored bins declaration selects no bins
        لكل (ثابت std::string& name : m_droppedBins[coverpointp]) {
            m_cpBins.at(cpVarp).spans.emplace(name, BinSpan{});
        }
        // A value a constructed bin holds is no default bin's; only sampling tells which do
        AstVar* sizedMatchedp = nullptr;
        𐤀𐤌 (!defaultBins.empty()
            && std::any_of(sizedBins.begin(), sizedBins.end(), [](ثابت AstCoverBin* binp) {
                   أرجع binp->binsType().binIsNormal();
               })) {
            sizedMatchedp = جديد AstVar{fl, VVarType::BLOCKTEMP,
                                       "__VcpSized_" + sanitizeGeneratedName(coverpointp->name()),
                                       coverpointp->findBitDType()};
            sizedMatchedp->funcLocal(true);
            m_sampleFuncp->addStmtsp(sizedMatchedp);
            addSampleStmt(جديد AstAssign{fl, جديد AstVarRef{fl, sizedMatchedp, VAccess::WRITE},
                                        جديد AstConst{fl, AstConst::BitFalse{}}});
        }
        لكل (uint32_t sized = 0; sized < sizedBins.size(); ++sized) {
            emitSizedSample(coverpointp, sizedBins[sized], cpVarp, exprp, sized, sizedMatchedp);
        }
        لكل (AstCoverBin* ثابت defBinp : defaultBins) {
            FileLine* ثابت dfl = defBinp->fileline();
            namerStmts.push_back(makeNamer(cpVarp, defBinp, -1, static_cast<uint32_t>(idx)));
            AstNodeExpr* condp = buildDefaultCondition(coverpointp, exprp, dfl);
            𐤀𐤌 (sizedMatchedp) {
                condp = جديد AstLogAnd{
                    dfl, جديد AstNot{dfl, جديد AstVarRef{dfl, sizedMatchedp, VAccess::READ}}, condp};
            }
            emitConvHitIf(coverpointp, defBinp, cpVarp, cnum(dfl, static_cast<uint32_t>(idx)),
                          condp);
            ++idx;
        }
        𐤀𐤌 (idx > std::numeric_limits<uint32_t>::max()) {
            // The runtime indexes bins with 32 bits; stop before generating a model
            coverpointp->v3warn(E_UNSUPPORTED, "Unsupported: coverpoint with more than "
                                                   << std::numeric_limits<uint32_t>::max()
                                                   << " bins");
        }

        // Transition coverpoints track the previous sampled value; update it once at the end of
        // this coverpoint's sample() contribution (the prev var was created on demand by the
        // transition matching above).
        𐤀𐤌 (coverpointHasTransition(coverpointp)) {
            AstVar* ثابت prevVarp = VN_AS(coverpointp->user1p(), Var);
            addSampleStmt(
                جديد AstAssign{coverpointp->fileline(),
                              جديد AstVarRef{prevVarp->fileline(), prevVarp, VAccess::WRITE},
                              exprp->cloneTree(خطأ)});
        }

        // Constructor: init (allocates), namers, then registration (under --coverage).
        // Under --protect-ids the hierarchy and page string reach the coverage database
        // verbatim, so obfuscate them like line/toggle points (per-word hierarchy, whole-
        // unit page).  No-ops when --protect-ids is off.
        ثابت منطقي prot = v3Global.opt.protectIds();
        ثابت std::string hier
            = VIdProtect::protectWordsIf(m_covergroupName + "." + coverpointp->name(), prot);
        m_constructorp->addStmtsp(
            itemCall(fl, cpVarp, VCMethod::COVERGROUP_INIT,
                     {ctext(fl, quoted(hier)), cnum(fl, static_cast<uint32_t>(atLeastValue)),
                      cnum(fl, static_cast<uint32_t>(idx))})
                ->makeStmt());
        لكل (AstNodeStmt* ثابت ns : namerStmts) m_constructorp->addStmtsp(ns);
        𐤀𐤌 (dynamic) {
            m_constructorp->addStmtsp(
                itemCall(fl, cpVarp, VCMethod::COVERGROUP_VALUE_TYPE,
                         {cnum(fl, exprp->width()), cnum(fl, exprp->isSigned())})
                    ->makeStmt());
            ValueLists lists;
            لكل (ثابت auto& entry : metadata) {
                collectValueMetadata(lists, exprp, std::get<0>(entry), std::get<1>(entry),
                                     std::get<2>(entry));
            }
            لكل (ثابت BinRun* ثابت runp : runMetadata) collectRunMetadata(lists, exprp, *runp);
            emitValueList(fl, cpVarp, VCMethod::COVERGROUP_VALUE_RANGES, lists.m_ranges);
            emitValueList(fl, cpVarp, VCMethod::COVERGROUP_VALUE_RUNS, lists.m_runs);
            emitValueList(fl, cpVarp, VCMethod::COVERGROUP_VALUE_PATTERNS, lists.m_patterns);
            emitValueList(fl, cpVarp, VCMethod::COVERGROUP_VALUE_TRANSITIONS, lists.m_transitions);
            لكل (AstCoverBin* ثابت binp : sizedBins) {
                generateConstructedBins(coverpointp, binp, cpVarp, exprp);
            }
            m_constructorp->addStmtsp(
                itemCall(fl, cpVarp, VCMethod::COVERGROUP_VALUE_FINALIZE)->makeStmt());
        }
        𐤀𐤌 (v3Global.opt.coverage()) {
            ثابت std::string page
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
    فراغ generateMultiValueTransitionCode(AstCoverpoint* coverpointp, AstCoverBin* binp,
                                          AstNodeExpr* exprp, ثابت ConvBinTarget& tgt,
                                          ثابت std::vector<AstCoverTransItem*>& items) {
        UINFO(4, "    Generating multi-value transition state machine for: " << binp->name());
        UINFO(4, "      Sequence length: " << items.size() << " items");

        // Create state position variable
        AstVar* ثابت stateVarp = createSequenceStateVar(coverpointp, binp);

        // Build case statement with N cases (one for each state 0 to N-1)
        // State 0: Not started, looking for first item
        // State 1 to N-1: In progress, looking for next item

        AstCase* ثابت casep
            = جديد AstCase{binp->fileline(), VCaseType::CT_CASE,
                          جديد AstVarRef{stateVarp->fileline(), stateVarp, VAccess::READ}, nullptr};

        // Generate each case item in the switch statement
        لكل (size_t state = 0; state < items.size(); ++state) {
            AstCaseItem* caseItemp = generateTransitionStateCase(coverpointp, binp, exprp, tgt,
                                                                 stateVarp, items, state);
            casep->addItemsp(caseItemp);
        }

        // Add default case (reset to state 0) to prevent CASEINCOMPLETE warnings,
        // since the state variable is wider than the number of valid states.
        AstCaseItem* ثابت defaultItemp = جديد AstCaseItem{
            binp->fileline(), nullptr,
            جديد AstAssign{binp->fileline(),
                          جديد AstVarRef{binp->fileline(), stateVarp, VAccess::WRITE},
                          جديد AstConst{binp->fileline(), AstConst::WidthedValue{}, 8, 0}}};
        casep->addItemsp(defaultItemp);

        addSampleStmt(casep);
        UINFO(4, "      Successfully added multi-value transition state machine");
    }

    // Generate code for a single state in the transition state machine
    // Returns the case item for this state
    AstCaseItem* generateTransitionStateCase(AstCoverpoint* coverpointp, AstCoverBin* binp,
                                             AstNodeExpr* exprp, ثابت ConvBinTarget& tgt,
                                             AstVar* stateVarp,
                                             ثابت std::vector<AstCoverTransItem*>& items,
                                             size_t state) {
        FileLine* ثابت fl = binp->fileline();

        // Build condition for current value matching expected item at this state
        AstNodeExpr* ثابت matchCondp = buildTransitionItemCondition(items[state], exprp);

        AstNodeStmt* matchActionp = nullptr;

        𐤀𐤌 (state == items.size() - 1) {
            // Last state: sequence complete!  Record the hit in the runtime VlCoverpoint.
            matchActionp = makeRuntimeBinHit(fl, tgt);

            // For illegal_bins, add error message
            𐤀𐤌 (binp->binsType() == VCoverBinsType::BINS_ILLEGAL) {
                ثابت string errMsg = "Illegal transition bin " + binp->prettyNameQ()
                                      + " hit in coverpoint " + coverpointp->prettyNameQ();
                matchActionp = matchActionp->addNext(makeIllegalBinAction(fl, errMsg));
            }

            // Reset state to 0
            matchActionp = matchActionp->addNext(
                جديد AstAssign{fl, جديد AstVarRef{fl, stateVarp, VAccess::WRITE},
                              جديد AstConst{fl, AstConst::WidthedValue{}, 8, 0}});
        } وإلا {
            // Intermediate state: advance to next state
            matchActionp = جديد AstAssign{
                fl, جديد AstVarRef{fl, stateVarp, VAccess::WRITE},
                جديد AstConst{fl, AstConst::WidthedValue{}, 8, static_cast<uint32_t>(state + 1)}};
        }

        // Build restart logic: check if current value matches first item
        // If so, restart sequence from state 1 (even if we're in middle of sequence)
        AstNodeStmt* noMatchActionp = nullptr;
        𐤀𐤌 (state > 0) {
            // Check if current value matches first item (restart condition)
            AstNodeExpr* ثابت restartCondp = buildTransitionItemCondition(items[0], exprp);

            UASSERT_OBJ(restartCondp, items[0],
                        "buildTransitionItemCondition returned nullptr for restart");

            // Restart to state 1
            AstNodeStmt* restartActionp
                = جديد AstAssign{fl, جديد AstVarRef{fl, stateVarp, VAccess::WRITE},
                                جديد AstConst{fl, AstConst::WidthedValue{}, 8, 1}};

            // Reset to state 0 (else branch)
            AstNodeStmt* resetActionp
                = جديد AstAssign{fl, جديد AstVarRef{fl, stateVarp, VAccess::WRITE},
                                جديد AstConst{fl, AstConst::WidthedValue{}, 8, 0}};

            noMatchActionp = جديد AstIf{fl, restartCondp, restartActionp, resetActionp};
        }
        // For state 0, no action needed if no match (stay in state 0)

        // Combine into if-else
        AstNodeStmt* ثابت stmtp = جديد AstIf{fl, matchCondp, matchActionp, noMatchActionp};

        // Create case item for this state value
        AstCaseItem* ثابت caseItemp = جديد AstCaseItem{
            fl, جديد AstConst{fl, AstConst::WidthedValue{}, 8, static_cast<uint32_t>(state)},
            stmtp};

        أرجع caseItemp;
    }

    // Create: $error(msg); $stop;  Used when an illegal bin is hit.
    AstNodeStmt* makeIllegalBinAction(FileLine* fl, ثابت string& errMsg) {
        AstDisplay* ثابت errorp
            = جديد AstDisplay{fl, VDisplayType::DT_ERROR, errMsg, nullptr, nullptr};
        errorp->fmtp()->timeunit(m_covergroupp->timeunit());
        static_cast<AstNode*>(errorp)->addNext(جديد AstStop{fl, true});
        أرجع errorp;
    }

    // Preserve the coverpoint's width and signedness after V3Width.
    ساكن AstConst* newValueConst(FileLine* fl, ثابت V3Number& value, ثابت AstNodeExpr* exprp) {
        V3Number narrowed{fl, exprp->width(), 0};
        narrowed.opAssign(value);
        AstConst* ثابت constp = جديد AstConst{fl, narrowed};
        constp->dtypeFrom(exprp);
        أرجع constp;
    }

    // A real copy of a real or integral range bound, for comparing with a real coverpoint.
    ساكن AstConst* newRealConst(AstConst* constp) {
        𐤀𐤌 (constp->num().isDouble()) أرجع constp->cloneTree(خطأ);
        V3Number real{&constp->num(), 64};
        real.opIToRD(constp->num(), constp->isSigned());
        أرجع جديد AstConst{constp->fileline(), real};
    }

    // Clone a constant node, widening to targetWidth if needed (zero-extend).
    // Used to ensure comparisons use matching widths after V3Width has run.
    ساكن AstConst* widenConst(FileLine* fl, AstConst* constp, صحيح targetWidth) {
        𐤀𐤌 (constp->width() == targetWidth) أرجع constp->cloneTree(خطأ);
        V3Number num{fl, targetWidth, 0};
        num.opAssign(constp->num());
        أرجع جديد AstConst{fl, num};
    }

    // Build a range condition: minp <= exprp <= maxp.
    // Uses signed comparisons if exprp is signed; omits trivially-true domain bounds.
    // All arguments are non-owning; clones exprp/minp/maxp as needed.
    AstNodeExpr* makeRangeCondition(FileLine* fl, AstNodeExpr* exprp, AstNodeExpr* minp,
                                    AstNodeExpr* maxp) {
        ثابت صحيح exprWidth = exprp->widthMin();
        AstConst* ثابت minConstp = VN_AS(minp, Const);
        AstConst* ثابت maxConstp = VN_AS(maxp, Const);
        𐤀𐤌 (exprp->isDouble()) {
            // A real coverpoint has no finite domain bounds to omit.
            أرجع جديد AstAnd{fl,
                              جديد AstGteD{fl, exprp->cloneTree(خطأ), newRealConst(minConstp)},
                              جديد AstLteD{fl, exprp->cloneTree(خطأ), newRealConst(maxConstp)}};
        }
        // Widen constants to match expression width so post-V3Width nodes use correct macros
        AstConst* ثابت minWidep = widenConst(fl, minConstp, exprWidth);
        AstConst* ثابت maxWidep = widenConst(fl, maxConstp, exprWidth);
        V3Number minimum{fl, exprWidth, 0};
        V3Number maximum{fl, exprWidth, 0};
        maximum.setAllBits1();
        𐤀𐤌 (exprp->isSigned()) {
            minimum.setBit(exprWidth - 1, 1);
            maximum.setBit(exprWidth - 1, 0);
        }
        AstNodeExpr* lowerp = nullptr;
        AstNodeExpr* upperp = nullptr;
        𐤀𐤌 (minWidep->num().isCaseEq(minimum)) {
            VL_DO_DANGLING(pushDeletep(minWidep), minWidep);
        } وإلا {
            lowerp = exprp->isSigned() ? static_cast<AstNodeExpr*>(
                                             جديد AstGteS{fl, exprp->cloneTree(خطأ), minWidep})
                                       : static_cast<AstNodeExpr*>(
                                             جديد AstGte{fl, exprp->cloneTree(خطأ), minWidep});
        }
        𐤀𐤌 (maxWidep->num().isCaseEq(maximum)) {
            VL_DO_DANGLING(pushDeletep(maxWidep), maxWidep);
        } وإلا {
            upperp = exprp->isSigned() ? static_cast<AstNodeExpr*>(
                                             جديد AstLteS{fl, exprp->cloneTree(خطأ), maxWidep})
                                       : static_cast<AstNodeExpr*>(
                                             جديد AstLte{fl, exprp->cloneTree(خطأ), maxWidep});
        }
        𐤀𐤌 (lowerp && upperp) أرجع جديد AstAnd{fl, lowerp, upperp};
        𐤀𐤌 (lowerp) أرجع lowerp;
        𐤀𐤌 (upperp) أرجع upperp;
        أرجع جديد AstConst{fl, AstConst::BitTrue{}};
    }

    // Build a one-sided comparison for an open-ended bin range whose other bound is '$'.
    // '$' denotes the coverpoint domain extreme, so {[lo:$]} == (expr >= lo) and
    // {[$:hi]} == (expr <= hi).
    AstNodeExpr* makeOpenRangeCondition(FileLine* fl, AstNodeExpr* exprp, AstConst* boundp,
                                        منطقي isLowerBound) {
        AstConst* ثابت widep = widenConst(fl, boundp, exprp->widthMin());
        𐤀𐤌 (isLowerBound) {
            𐤀𐤌 (exprp->isSigned()) أرجع جديد AstGteS{fl, exprp->cloneTree(خطأ), widep};
            أرجع جديد AstGte{fl, exprp->cloneTree(خطأ), widep};
        }
        𐤀𐤌 (exprp->isSigned()) أرجع جديد AstLteS{fl, exprp->cloneTree(خطأ), widep};
        أرجع جديد AstLte{fl, exprp->cloneTree(خطأ), widep};
    }

    // Build condition for a single transition item.
    // Returns expression that checks if exprp matches the item's value/range list.
    // Overload for when the expression is a variable read -- creates and manages the VarRef
    // internally, so callers don't need to construct a temporary node.
    AstNodeExpr* buildTransitionItemCondition(AstCoverTransItem* itemp, AstVar* varp) {
        AstNodeExpr* varRefp = جديد AstVarRef{varp->fileline(), varp, VAccess::READ};
        AstNodeExpr* ثابت condp = buildTransitionItemCondition(itemp, varRefp);
        VL_DO_DANGLING(pushDeletep(varRefp), varRefp);
        أرجع condp;
    }

    // Non-owning: exprp is cloned internally; caller retains ownership of exprp.
    AstNodeExpr* buildTransitionItemCondition(AstCoverTransItem* itemp, AstNodeExpr* exprp) {
        AstNodeExpr* condp = nullptr;

        لكل (AstNode* valp = itemp->valuesp(); valp; valp = valp->nextp()) {
            AstNodeExpr* singleCondp = nullptr;
            valp = V3Const::constifyEdit(valp);
            AstConst* ثابت constp = VN_CAST(valp, Const);
            𐤀𐤌 (!constp) {
                valp->v3error("Non-constant expression in transition bin; "
                              "values must be constants (IEEE 1800-2023 19.5)");
                أرجع جديد AstConst{valp->fileline(), AstConst::BitFalseErroring{}};
            }
            singleCondp
                = جديد AstEq{constp->fileline(), exprp->cloneTree(خطأ), constp->cloneTree(خطأ)};
            𐤀𐤌 (condp) {
                condp = جديد AstOr{itemp->fileline(), condp, singleCondp};
            } وإلا {
                condp = singleCondp;
            }
        }

        أرجع condp;
    }

    // Generate code for a single transition sequence (used by both regular and array bins)
    فراغ generateSingleTransitionCode(AstCoverpoint* coverpointp, AstCoverBin* binp,
                                      AstNodeExpr* exprp, ثابت ConvBinTarget& tgt,
                                      AstCoverTransSet* transSetp) {
        UINFO(4, "      Generating code for transition sequence");

        // Get or create previous value variable
        AstVar* ثابت prevVarp = createPrevValueVar(coverpointp, exprp);

        UASSERT_OBJ(
            transSetp, binp,
            "Transition bin has no transition set (transp() was checked before calling this)");

        // Get transition items (the sequence: item1 => item2 => item3)
        std::vector<AstCoverTransItem*> items;
        لكل (AstNode* itemp = transSetp->itemsp(); itemp; itemp = itemp->nextp())
            items.push_back(VN_AS(itemp, CoverTransItem));

        𐤀𐤌 (items.empty()) {
            binp->v3error("Transition set without items");
            أرجع;
        }

        𐤀𐤌 (items.size() == 1) {
            // Single item transition not valid (need at least 2 values for =>)
            binp->v3error("Transition requires at least two values");
            أرجع;
        } وإلا 𐤀𐤌 (items.size() == 2) {
            // Simple two-value transition: (val1 => val2)
            // Use optimized direct comparison (no state machine needed)
            AstNodeExpr* ثابت cond1p = buildTransitionItemCondition(items[0], prevVarp);
            AstNodeExpr* ثابت cond2p = buildTransitionItemCondition(items[1], exprp);

            // Combine: prev matches val1 AND current matches val2
            AstNodeExpr* fullCondp = جديد AstAnd{binp->fileline(), cond1p, cond2p};

            addConvTransHitIf(coverpointp, binp, tgt, fullCondp);

            UINFO(4, "        Successfully added 2-value transition if statement");
        } وإلا {
            // Multi-value sequence (a => b => c => ...)
            // Use state machine to track position in sequence
            generateMultiValueTransitionCode(coverpointp, binp, exprp, tgt, items);
        }
    }

    // Append a "{ VlCoverpoint* __Vcx_cps[] = {cp0, cp1, ...}; <call> }" statement.  The brace
    // and the temporary array stay literal text -- a CMethodHard is one call, not a block --
    // but callp itself carries the member, method and '->'.  Construction only: init() copies
    // the array into the cross, so sample() reads it from there and needs no array at all.
    AstCStmt* makeCrossCpsCall(FileLine* fl, ثابت std::vector<AstVar*>& cpVars,
                               AstCMethodHard* callp) {
        AstCStmt* ثابت cs = جديد AstCStmt{fl};
        cs->add("{ VlCoverpoint* __Vcx_cps[] = {");
        لكل (size_t d = 0; d < cpVars.size(); ++d) {
            𐤀𐤌 (d != 0) cs->add(", ");
            cs->add(memberRef(fl, cpVars[d]));
        }
        cs->add("}; ");
        cs->add(callp);
        cs->add("; }");
        أرجع cs;
    }

    // Assign the per-bin flags individually: one-bit SV results have integer C++ storage types,
    // which may narrow in a bool initializer list but convert implicitly in assignments.
    AstCStmt* makeCrossIffsCall(FileLine* fl, ثابت std::vector<AstCoverCrossBin*>& bins,
                                AstCMethodHard* callp) {
        AstCStmt* ثابت cs = جديد AstCStmt{fl};
        cs->add("{ bool __Vcx_iffs[" + cvtToStr(bins.size()) + "]; ");
        لكل (size_t i = 0; i < bins.size(); ++i) {
            ثابت AstCoverCrossBin* ثابت binp = bins[i];
            cs->add("__Vcx_iffs[" + cvtToStr(i) + "] = ");
            cs->add(binp->iffp() ? binp->iffp()->cloneTree(خطأ)
                                 : جديد AstConst{fl, AstConst::BitTrue{}});
            cs->add("; ");
        }
        cs->add(callp);
        cs->add("; }");
        أرجع cs;
    }

    using CrossSelection = std::vector<uint64_t>;
    بنية ResolvedCrossBin final {
        AstCoverCrossBin* binp;
        CrossSelection selection;
    };
    بنية CrossLayout final {
        uint32_t tuples = 0;
        uint32_t autoBins = 0;
        uint64_t binWords = 0;
        منطقي valid = true;
        std::vector<ResolvedCrossBin> bins;
    };
    بنية CrossSelectionContext final {
        AstCoverCross* crossp;  // Cross whose tuple space is being selected
        ثابت std::vector<AstVar*>& cpVars;  // Feeding coverpoints in dimension order
        ثابت std::map<std::string, uint32_t>& dimensions;  // Coverpoint name -> dimension
        uint32_t tuples;  // Size of the Cartesian product
        std::vector<uint32_t> strides;  // Flat-index stride per dimension
        منطقي valid = true;  // False if this explicit bin cannot be implemented
    };
    بنية CrossBinsofTarget final {
        ثابت CoverpointBins* m_binsp = nullptr;  // Declared bins; null after a reported error
        uint32_t m_dimension = 0;  // Coverpoint index within the cross
        uint32_t m_first = 0;  // First declared normal bin in the selected span
        uint32_t m_count = 0;  // Number of declared normal bins in the selected span
        uint32_t m_declaredFirst = 0;  // First runtime bin index of the selected span
        uint32_t m_declaredEnd = UINT32_MAX;  // One past the last runtime bin index
        int32_t m_sized = -1;  // Index of the sized array holding the selected bins; or -1
    };
    بنية CrossValueRange final {
        V3Number lo;  // Inclusive lower bound, sign-extended to the comparison width
        V3Number hi;  // Inclusive upper bound
        V3Number pattern;  // Allowed bit values; all X for an ordinary interval
        منطقي wildcard = خطأ;  // A wildcard singleton rather than an exact four-state value

        CrossValueRange(AstNode* nodep, صحيح width)
            : lo{nodep, width}
            , hi{nodep, width}
            , pattern{nodep, width} {
            pattern.setAllBitsX();
        }
    };

    ساكن std::vector<AstNode*> crossBinValues(ثابت CrossBinValues& bin) {
        𐤀𐤌 (bin.valuep) أرجع {bin.valuep};
        std::vector<AstNode*> values;
        𐤀𐤌 (bin.binp->transp()) {
            // IEEE 1800-2023 19.6.1: binsof uses the last value of each transition.
            لكل (AstNode* setp = bin.binp->transp(); setp; setp = setp->nextp()) {
                AstCoverTransItem* lastp = VN_AS(setp, CoverTransSet)->itemsp();
                طالما (lastp->nextp()) lastp = VN_AS(lastp->nextp(), CoverTransItem);
                لكل (AstNode* valuep = lastp->valuesp(); valuep; valuep = valuep->nextp()) {
                    values.push_back(valuep);
                }
            }
        } وإلا {
            لكل (AstNode* valuep = bin.binp->rangesp(); valuep; valuep = valuep->nextp()) {
                values.push_back(valuep);
            }
        }
        أرجع values;
    }

    // The values of the element-th bin of a run, sign-extended to 'width'
    ساكن CrossValueRange runBinRange(AstNode* nodep, ثابت BinRun& run, uint32_t element,
                                       صحيح width) {
        ثابت صحيح runw = run.m_lo.width();
        V3Number offset{nodep, runw};
        offset.opMul(run.m_stride, V3Number{nodep, runw, element});
        V3Number lo{nodep, runw};
        lo.opAdd(run.m_lo, offset);
        V3Number hi = run.m_hi;
        𐤀𐤌 (element + 1 < run.m_count) {
            offset.opSub(run.m_stride, V3Number{nodep, runw, 1});
            hi.opAdd(lo, offset);
        }
        CrossValueRange range{nodep, width};
        range.lo.opExtendS(lo, runw);
        range.hi.opExtendS(hi, runw);
        أرجع range;
    }

    ساكن صحيح crossRangeWidth(AstNode* nodep) {
        𐤀𐤌 (ثابت AstInsideRange* ثابت rangep = VN_CAST(nodep, InsideRange)) {
            أرجع std::max(rangep->lhsp()->width(), rangep->rhsp()->width());
        }
        أرجع nodep->width();
    }

    ساكن منطقي crossValueLess(ثابت V3Number& lhs, ثابت V3Number& rhs) {
        V3Number result{&lhs};
        أرجع !result.opLtS(lhs, rhs).isEqZero();
    }

    // Round a real bound inward to the nearest coverpoint value (IEEE 1800-2023 19.5.7).  Sets
    // 'empty' if the domain has no value on the bound's side.
    ساكن فراغ crossRealBound(ثابت AstConst* constp, AstNodeExpr* exprp, منطقي upper,
                               ثابت CrossValueRange& domain, V3Number& result, منطقي& empty) {
        ثابت مزدوج value = constp->num().toDouble();
        ثابت مزدوج bound = upper ? std::floor(value) : std::ceil(value);
        // Powers of two are exact, so the integral bound compares exactly with the domain edges.
        ثابت مزدوج limit
            = std::ldexp(1.0, exprp->isSigned() ? exprp->width() - 1 : exprp->width());
        ثابت مزدوج minimum = exprp->isSigned() ? -limit : 0.0;
        𐤀𐤌 (std::isnan(bound) || (upper ? bound < minimum : bound >= limit)) {
            empty = true;
            result = domain.lo;
        } وإلا 𐤀𐤌 (bound < minimum) {
            result = domain.lo;
        } وإلا 𐤀𐤌 (bound >= limit) {
            result = domain.hi;
        } وإلا {
            V3Number real{&result, 64};
            real.setDouble(bound);
            result.opRToIRoundS(real);
        }
    }

    ساكن منطقي crossRangeBound(AstNode* nodep, AstNodeExpr* exprp, منطقي upper, منطقي binValue,
                                ثابت CrossValueRange& domain, V3Number& result, منطقي& empty) {
        𐤀𐤌 (VN_IS(nodep, Unbounded)) {
            V3Number limit{nodep, exprp->width()};
            𐤀𐤌 (upper) limit.setAllBits1();
            𐤀𐤌 (exprp->isSigned()) {
                limit.setBit(exprp->width() - 1, !upper);
                result.opExtendS(limit, limit.width());
            } وإلا {
                result.opAssign(limit);
            }
            أرجع true;
        }
        ثابت AstConst* ثابت constp = VN_CAST(nodep, Const);
        𐤀𐤌 (!constp) أرجع خطأ;
        𐤀𐤌 (constp->num().isDouble()) {
            crossRealBound(constp, exprp, upper, domain, result, empty);
            أرجع true;
        }
        𐤀𐤌 (constp->num().isString()) أرجع خطأ;
        𐤀𐤌 (binValue && exprp->isSigned() && constp->width() <= exprp->width()) {
            // Bin bit patterns use the coverpoint's effective type (IEEE 1800-2023 19.5.7).
            // Wider values and intersect filters retain their values for domain clipping.
            V3Number value{nodep, exprp->width()};
            𐤀𐤌 (constp->isSigned()) {
                value.opExtendS(constp->num(), constp->width());
            } وإلا {
                value.opAssign(constp->num());
            }
            result.opExtendS(value, value.width());
        } وإلا 𐤀𐤌 (constp->isSigned()) {
            result.opExtendS(constp->num(), constp->width());
        } وإلا {
            result.opAssign(constp->num());
        }
        أرجع true;
    }

    ساكن CrossValueRange crossValueDomain(AstNode* nodep, صحيح valueWidth, منطقي isSigned,
                                            صحيح width) {
        CrossValueRange domain{nodep, width};
        V3Number lo{nodep, valueWidth};
        V3Number hi{nodep, valueWidth};
        hi.setAllBits1();
        𐤀𐤌 (isSigned) {
            lo.setBit(valueWidth - 1, 1);
            hi.setBit(valueWidth - 1, 0);
            domain.lo.opExtendS(lo, valueWidth);
            domain.hi.opExtendS(hi, valueWidth);
        } وإلا {
            domain.lo.opAssign(lo);
            domain.hi.opAssign(hi);
        }
        أرجع domain;
    }

    ساكن فراغ intersectCrossRange(CrossValueRange& range, ثابت CrossValueRange& other) {
        𐤀𐤌 (crossValueLess(range.lo, other.lo)) range.lo = other.lo;
        𐤀𐤌 (crossValueLess(other.hi, range.hi)) range.hi = other.hi;
    }

    ساكن منطقي crossValueRange(AstNode* nodep, AstNodeExpr* exprp, منطقي binValue, منطقي wildcard,
                                ثابت CrossValueRange& domain, CrossValueRange& range) {
        منطقي empty = خطأ;
        ثابت AstConst* ثابت constp = VN_CAST(nodep, Const);
        𐤀𐤌 (ثابت AstInsideRange* ثابت rangep = VN_CAST(nodep, InsideRange)) {
            𐤀𐤌 (!crossRangeBound(rangep->lhsp(), exprp, خطأ, binValue, domain, range.lo, empty)
                || !crossRangeBound(rangep->rhsp(), exprp, true, binValue, domain, range.hi, empty)
                || range.lo.isFourState() || range.hi.isFourState()) {
                أرجع خطأ;
            }
        } وإلا 𐤀𐤌 (constp && constp->num().isDouble()) {
            // A real value participates only if integral; it has no wildcard bits.
            crossRealBound(constp, exprp, خطأ, domain, range.lo, empty);
            crossRealBound(constp, exprp, true, domain, range.hi, empty);
        } وإلا {
            𐤀𐤌 (!crossRangeBound(nodep, exprp, خطأ, binValue, domain, range.lo, empty)) {
                أرجع خطأ;
            }
            𐤀𐤌 (binValue && exprp->isSigned() && constp && !constp->isSigned()
                && constp->width() > exprp->width()) {
                منطقي representable = true;
                لكل (صحيح bit = exprp->width(); bit < constp->width(); ++bit) {
                    representable &= !constp->num().bitIs1(bit);
                }
                𐤀𐤌 (representable) {
                    V3Number value{nodep, exprp->width()};
                    value.opAssign(constp->num());
                    range.lo.opExtendS(value, value.width());
                }
            }
            range.hi = range.lo;
            range.wildcard = wildcard;
            𐤀𐤌 (wildcard) {
                range.pattern = range.lo;
                range.lo = domain.lo;
                range.hi = domain.hi;
                𐤀𐤌 (constp && constp->isSigned()) {
                    // Replicated X sign bits are correlated, not independent wildcards.
                    // The source domain preserves expansion-before-casting (19.5.7).
                    intersectCrossRange(
                        range, crossValueDomain(nodep, constp->width(), true, domain.lo.width()));
                }
            }
        }
        𐤀𐤌 (empty) {
            range.lo = domain.hi;
            range.hi = domain.lo;
            أرجع true;
        }
        𐤀𐤌 (!range.lo.isFourState()) intersectCrossRange(range, domain);
        𐤀𐤌 (range.wildcard && exprp->isSigned()) {
            ثابت صحيح sign = exprp->width() - 1;
            𐤀𐤌 (constp && !constp->isSigned() && constp->width() > exprp->width()) {
                // Unsigned equality preserves every target bit pattern if discarded bits are zero.
                لكل (صحيح bit = exprp->width(); bit < constp->width(); ++bit) {
                    𐤀𐤌 (constp->num().bitIs1(bit)) {
                        range.lo = domain.hi;
                        range.hi = domain.lo;
                        أرجع true;
                    }
                }
                لكل (صحيح bit = exprp->width(); bit < range.pattern.width(); ++bit) {
                    𐤀𐤌 (range.pattern.bitIsXZ(sign))
                        range.pattern.setBit(bit, 'x');
                    وإلا
                        range.pattern.setBit(bit, range.pattern.bitIs1(sign));
                }
                أرجع true;
            }
            // Discarded high bits constrain the target sign, rather than becoming don't-cares.
            لكل (صحيح bit = exprp->width(); bit < range.pattern.width(); ++bit) {
                𐤀𐤌 (range.pattern.bitIsXZ(bit)) استمر;
                ثابت منطقي value = range.pattern.bitIs1(bit);
                𐤀𐤌 (!range.pattern.bitIsXZ(sign) && range.pattern.bitIs1(sign) != value) {
                    range.lo = domain.hi;
                    range.hi = domain.lo;
                    اكسر;
                }
                range.pattern.setBit(sign, value);
            }
        }
        أرجع true;
    }

    تعداد CrossRangeState : uint8_t {
        CROSS_INSIDE_BOUNDS = 0,  // Prefix is strictly inside the interval
        CROSS_AT_LOWER = 1,
        CROSS_AT_UPPER = 2,
        CROSS_AT_BOUNDS = CROSS_AT_LOWER | CROSS_AT_UPPER,
        CROSS_NO_MATCH = 4  // Prefix cannot match the interval/pattern
    };

    ساكن CrossRangeState crossRangeStep(ثابت CrossValueRange& range, CrossRangeState state,
                                          صحيح bit, صحيح value) {
        𐤀𐤌 (state == CROSS_NO_MATCH) أرجع CROSS_NO_MATCH;
        // Flipping the sign bit makes signed order lexicographic.
        ثابت منطقي sign = bit == range.pattern.width() - 1;
        𐤀𐤌 (!range.pattern.bitIsXZ(bit) && value != (range.pattern.bitIs1(bit) ^ sign)) {
            أرجع CROSS_NO_MATCH;
        }
        ثابت صحيح low = range.lo.bitIs1(bit) ^ sign;
        ثابت صحيح high = range.hi.bitIs1(bit) ^ sign;
        𐤀𐤌 (((state & CROSS_AT_LOWER) && value < low)
            || ((state & CROSS_AT_UPPER) && value > high)) {
            أرجع CROSS_NO_MATCH;
        }
        أرجع static_cast<CrossRangeState>(
            ((state & CROSS_AT_LOWER) && value == low ? CROSS_AT_LOWER : CROSS_INSIDE_BOUNDS)
            | ((state & CROSS_AT_UPPER) && value == high ? CROSS_AT_UPPER : CROSS_INSIDE_BOUNDS));
    }

    ساكن منطقي crossWildcardIntersects(ثابت CrossValueRange& range) {
        unsigned states = 1U << CROSS_AT_BOUNDS;
        لكل (صحيح bit = range.pattern.width() - 1; bit >= 0 && states; --bit) {
            unsigned next = 0;
            لكل (ثابت CrossRangeState state :
                 {CROSS_INSIDE_BOUNDS, CROSS_AT_LOWER, CROSS_AT_UPPER, CROSS_AT_BOUNDS}) {
                𐤀𐤌 (!(states & (1U << state))) استمر;
                لكل (صحيح value = 0; value < 2; ++value) {
                    ثابت CrossRangeState equal = crossRangeStep(range, state, bit, value);
                    𐤀𐤌 (equal != CROSS_NO_MATCH) next |= 1U << equal;
                }
            }
            states = next;
        }
        أرجع states != 0;
    }

    // The least value at least 'from' with the non-x bits of 'pattern', in unsigned order; false
    // if none.  A bit scan, which does not enumerate the pattern's x bits.
    ساكن منطقي nextPatternValue(ثابت V3Number& pattern, ثابت V3Number& from, V3Number& result) {
        result.opAssign(from);
        صحيح carry = -1;  // Lowest x bit above the bit scanned where 'from' has a 0
        لكل (صحيح bit = from.width() - 1; bit >= 0; --bit) {
            𐤀𐤌 (pattern.bitIsXZ(bit)) {
                𐤀𐤌 (!from.bitIs1(bit)) carry = bit;
                استمر;
            }
            𐤀𐤌 (pattern.bitIs1(bit) == from.bitIs1(bit)) استمر;
            𐤀𐤌 (from.bitIs1(bit)) {  // Only a greater prefix, an x bit above raised, can match
                𐤀𐤌 (carry < 0) أرجع خطأ;
                bit = carry;
            }
            // Then the least such value: the pattern's bits, with x bits of zero
            result.setBit(bit, 1);
            طالما (--bit >= 0) result.setBit(bit, pattern.bitIs1(bit));
            أرجع true;
        }
        أرجع true;
    }

    // Append to 'runs' the maximal runs of consecutive values of 'range' that its pattern
    // matches, in value order, until there are more than 'limit'.  A match continues through
    // the pattern's trailing x bits only, which the next value's carry leaves.
    ساكن فراغ crossRangeRuns(ثابت CrossValueRange& range, size_t limit,
                               std::vector<std::pair<V3Number, V3Number>>& runs) {
        // Values order signed, which is the unsigned order of values with the sign bit flipped
        ثابت صحيح sign = range.lo.width() - 1;
        ثابت auto flipped = [sign](V3Number value) {
            𐤀𐤌 (!value.bitIsXZ(sign)) value.setBit(sign, !value.bitIs1(sign));
            أرجع value;
        };
        ثابت V3Number pattern = flipped(range.pattern);
        ثابت V3Number hi = flipped(range.hi);
        ثابت V3Number one{&hi, hi.width(), 1};
        V3Number trailing{&hi, hi.width()};
        لكل (صحيح bit = 0; bit <= sign && pattern.bitIsXZ(bit); ++bit) trailing.setBit(bit, 1);
        V3Number from = flipped(range.lo);
        V3Number first{&hi, hi.width()};
        V3Number last{&hi, hi.width()};
        V3Number less{&hi};
        طالما (runs.size() <= limit && nextPatternValue(pattern, from, first)
               && less.opLt(hi, first).isEqZero()) {
            last.opOr(first, trailing);
            𐤀𐤌 (!less.opLt(hi, last).isEqZero()) last = hi;
            runs.emplace_back(flipped(first), flipped(last));
            𐤀𐤌 (last.isCaseEq(hi)) اكسر;
            from.opAdd(last, one);
        }
    }

    // True if no coverpoint value participates in a resolved value or range.  A value with x
    // or z bits participates only as a wildcard pattern (IEEE 1800-2023 19.5.7).
    ساكن منطقي crossRangeEmpty(ثابت CrossValueRange& range) {
        𐤀𐤌 (range.lo.isFourState()) أرجع true;
        أرجع crossValueLess(range.hi, range.lo)
               || (range.wildcard && !crossWildcardIntersects(range));
    }

    // Comparison width that holds both the coverpoint's and a bin or intersect value's range.
    ساكن صحيح resolveWidth(AstNode* nodep, ثابت AstNodeExpr* exprp) {
        أرجع std::max(exprp->width(), crossRangeWidth(nodep)) + 1;
    }

    // Resolve a bin or intersect value to coverpoint values (IEEE 1800-2023 19.5.7), in the
    // width 'range' was created with.  False if the value is not a constant integral or real.
    ساكن منطقي resolveValue(AstNode* nodep, AstNodeExpr* exprp, منطقي binValue, منطقي wildcard,
                             CrossValueRange& range) {
        ثابت CrossValueRange domain
            = crossValueDomain(nodep, exprp->width(), exprp->isSigned(), range.lo.width());
        أرجع crossValueRange(nodep, exprp, binValue, wildcard, domain, range);
    }

    ساكن منطقي crossRangesIntersect(ثابت CrossValueRange& bin, ثابت CrossValueRange& filter) {
        // Values with x or z bits do not participate, even in an identical filter.
        𐤀𐤌 (bin.lo.isFourState() || filter.lo.isFourState()) أرجع خطأ;
        CrossValueRange match = bin;
        intersectCrossRange(match, filter);
        أرجع !crossValueLess(match.hi, match.lo)
               && (!bin.wildcard || crossWildcardIntersects(match));
    }

    ساكن فراغ unsupportedCrossRange(AstCoverBinsof* selectp, منطقي& valid) {
        selectp->v3warn(COVERIGN, "Unsupported: non-constant or non-integral 'intersect' value, "
                                  "or four-state range bound.");
        valid = خطأ;
    }

    ساكن منطقي crossValueMatchesFilters(AstCoverBinsof* selectp, AstNode* valuep,
                                         AstNodeExpr* exprp, ثابت AstCoverBin* binp,
                                         ثابت CrossValueRange& domain,
                                         ثابت std::vector<CrossValueRange>& filters,
                                         منطقي& valid) {
        CrossValueRange range{valuep, domain.lo.width()};
        𐤀𐤌 (!crossValueRange(valuep, exprp, true, binp->isWildcard(), domain, range)) {
            unsupportedCrossRange(selectp, valid);
            أرجع خطأ;
        }
        لكل (ثابت CrossValueRange& filter : filters) {
            𐤀𐤌 (crossRangesIntersect(range, filter)) أرجع true;
        }
        أرجع خطأ;
    }

    std::vector<منطقي> selectCoverpointBins(AstCoverBinsof* selectp, ثابت CoverpointBins& bins,
                                           uint32_t first, uint32_t count, منطقي& valid) {
        𐤀𐤌 (selectp->rangesp() && !bins.exprp->dtypep()->skipRefp()->isIntegralOrPacked()) {
            unsupportedCrossRange(selectp, valid);
            أرجع {};
        }
        std::vector<منطقي> selected(bins.total, خطأ);
        std::vector<std::vector<AstNode*>> values;
        صحيح width = bins.exprp->width();
        لكل (AstNode* rangep = selectp->rangesp(); rangep; rangep = rangep->nextp()) {
            width = std::max(width, crossRangeWidth(rangep));
        }
        𐤀𐤌 (selectp->rangesp()) {
            values.reserve(count);
            لكل (uint32_t i = first; i < first + count; ++i) {
                // A run's values are in the coverpoint type, whose width 'width' covers
                values.push_back(bins.values[i].runp ? std::vector<AstNode*>{}
                                                     : crossBinValues(bins.values[i]));
                لكل (AstNode* ثابت valuep : values.back()) {
                    width = std::max(width, crossRangeWidth(valuep));
                }
            }
        }
        // One extra bit preserves both unsigned maxima and negative signed bounds.
        ++width;
        ثابت CrossValueRange domain
            = crossValueDomain(selectp, bins.exprp->width(), bins.exprp->isSigned(), width);
        std::vector<CrossValueRange> filters;
        لكل (AstNode* rangep = selectp->rangesp(); rangep; rangep = rangep->nextp()) {
            CrossValueRange filter{rangep, width};
            𐤀𐤌 (!crossValueRange(rangep, bins.exprp, خطأ, خطأ, domain, filter)) {
                unsupportedCrossRange(selectp, valid);
                أرجع {};
            }
            filters.push_back(std::move(filter));
        }
        لكل (uint32_t i = first; i < first + count; ++i) {
            𐤀𐤌 (!selectp->rangesp()) {
                selected[i] = true;
                استمر;
            }
            𐤀𐤌 (ثابت BinRun* ثابت runp = bins.values[i].runp) {
                ثابت CrossValueRange range
                    = runBinRange(selectp, *runp, bins.values[i].element, width);
                selected[i] = std::any_of(filters.begin(), filters.end(),
                                          [&](ثابت CrossValueRange& filter) {
                                              أرجع crossRangesIntersect(range, filter);
                                          });
                استمر;
            }
            لكل (AstNode* ثابت valuep : values[i - first]) {
                selected[i] = crossValueMatchesFilters(
                    selectp, valuep, bins.exprp, bins.values[i].binp, domain, filters, valid);
                𐤀𐤌 (!valid) أرجع {};
                𐤀𐤌 (selected[i]) اكسر;
            }
        }
        𐤀𐤌 (selectp->isNegated()) {
            لكل (uint32_t i = 0; i < bins.total; ++i) selected[i] = !selected[i];
        }
        أرجع selected;
    }

    // Constructor-time value metadata of one coverpoint, as C++ list entries
    بنية ValueLists final {
        std::vector<std::string> m_ranges;  // Bin, then low and high words
        std::vector<std::string> m_runs;  // First bin, count, then low, span, and high words
        std::vector<std::string> m_patterns;  // Bin, then value, mask, low, and high words
        std::vector<std::string> m_transitions;  // Transition bin
    };

    // Append a value's words, in the coverpoint's width, to a C++ list entry.
    ساكن فراغ appendWords(std::string& text, ثابت V3Number& value, ثابت AstNodeExpr* exprp) {
        V3Number narrowed{&value, exprp->width()};
        narrowed.opAssign(value);
        لكل (صحيح word = 0; word < exprp->widthWords(); ++word) {
            text += ", " + cvtToStr(narrowed.edataWord(word)) + "U";
        }
    }

    فراغ collectValueMetadata(ValueLists& lists, AstNodeExpr* exprp, AstCoverBin* binp,
                              uint32_t index, AstNodeExpr* valuep) {
        ثابت std::string bin = cvtToStr(index) + "U";
        𐤀𐤌 (binp->transp()) lists.m_transitions.push_back(bin);
        لكل (AstNode* ثابت sourcep : crossBinValues({binp, valuep})) {
            CrossValueRange range{sourcep, resolveWidth(sourcep, exprp)};
            𐤀𐤌 (!resolveValue(sourcep, exprp, true, binp->isWildcard(), range)) {
                // Sampling already resolved every state bin value, so only transitions remain.
                sourcep->v3warn(E_UNSUPPORTED, "Unsupported: non-integral value in a transition "
                                               "bin of a coverpoint with exclusions.");
                استمر;
            }
            𐤀𐤌 (crossRangeEmpty(range)) استمر;
            std::string entry = bin;
            𐤀𐤌 (range.wildcard) {
                V3Number value{sourcep, exprp->width()};
                V3Number mask{sourcep, exprp->width()};
                mask.opBitsNonXZ(range.pattern);
                value.opBitsOne(range.pattern);
                appendWords(entry, value, exprp);
                appendWords(entry, mask, exprp);
            }
            appendWords(entry, range.lo, exprp);
            appendWords(entry, range.hi, exprp);
            (range.wildcard ? lists.m_patterns : lists.m_ranges).push_back(entry);
        }
    }

    // Describe a run with one entry, from which the runtime computes the values of its bins.
    ساكن فراغ collectRunMetadata(ValueLists& lists, AstNodeExpr* exprp, ثابت BinRun& run) {
        V3Number span{exprp, run.m_stride.width(), 0};
        span.opSub(run.m_stride, V3Number{exprp, run.m_stride.width(), 1});
        std::string entry = cvtToStr(run.m_declared) + "U, " + cvtToStr(run.m_count) + "U";
        appendWords(entry, run.m_lo, exprp);
        appendWords(entry, span, exprp);
        appendWords(entry, run.m_hi, exprp);
        lists.m_runs.push_back(entry);
    }

    // Emit one batched metadata list, bounding the size of each call's temporary list.
    فراغ emitValueList(FileLine* fl, AstVar* cpVarp, VCMethod method,
                       ثابت std::vector<std::string>& entries) {
        لكل (size_t first = 0; first < entries.size(); first += VALUE_LIST_ENTRIES) {
            ثابت size_t end = std::min(entries.size(), first + VALUE_LIST_ENTRIES);
            std::string text = "{" + entries[first];
            لكل (size_t i = first + 1; i < end; ++i) text += ", " + entries[i];
            m_constructorp->addStmtsp(
                itemCall(fl, cpVarp, method, {ctext(fl, text + "}")})->makeStmt());
        }
    }

    ساكن منطقي checkCrossRef(ثابت AstCoverCrossRef* refp, ثابت AstCoverCross* crossp) {
        𐤀𐤌 (refp->name() == crossp->name()) أرجع true;
        refp->v3error("Cross selection "
                      << refp->prettyNameQ() << " may only name its enclosing cross "
                      << crossp->prettyNameQ() << " (IEEE 1800-2023 19.6.1.2).");
        أرجع خطأ;
    }

    ساكن منطقي checkCrossBinName(ثابت AstCoverCrossBin* binp, std::set<std::string>& names) {
        𐤀𐤌 (names.emplace(binp->name()).second) أرجع true;
        binp->v3error("Duplicate cross bin " << binp->prettyNameQ()
                                             << " (IEEE 1800-2023 19.6.1).");
        أرجع خطأ;
    }

    // The span of the implicit automatic bin reported as 'name' ('auto_<i>', see
    // createImplicitAutoBins), found without naming each of its bins
    ساكن منطقي implicitAutoBinSpan(ثابت CoverpointBins& bins, ثابت std::string& name,
                                    BinSpan& span) {
        ثابت std::string prefix = "auto_";
        𐤀𐤌 (!VString::startsWith(name, prefix)) أرجع خطأ;
        ثابت std::string digits = name.substr(prefix.size());
        ثابت unsigned long index = std::strtoul(digits.c_str(), nullptr, 10);
        // Only the reported spelling names the bin, not e.g. 'auto_01' or 'auto_x'
        𐤀𐤌 (index >= bins.implicitAuto.count || digits != cvtToStr(index)) أرجع خطأ;
        ثابت uint32_t offset = static_cast<uint32_t>(index);
        span = BinSpan{bins.implicitAuto.first + offset, 1, bins.implicitAuto.declared + offset};
        أرجع true;
    }

    CrossBinsofTarget
    resolveBinsofTarget(ثابت AstCoverBinsof* selectp, ثابت AstCoverCross* crossp,
                        ثابت std::vector<AstVar*>& cpVars,
                        ثابت std::map<std::string, uint32_t>& dimensions) ثابت {
        ثابت auto dim = dimensions.find(selectp->pointp()->name());
        𐤀𐤌 (dim == dimensions.end()) {
            selectp->v3error("binsof coverpoint "
                             << selectp->pointp()->prettyNameQ() << " is not an item of cross "
                             << crossp->prettyNameQ() << " (IEEE 1800-2023 19.6.1).");
            أرجع {};
        }
        ثابت CoverpointBins& bins = m_cpBins.at(cpVars[dim->second]);
        CrossBinsofTarget target{&bins, dim->second, 0, bins.total};
        𐤀𐤌 (!selectp->name().empty()) {
            ثابت auto bin = bins.spans.find(selectp->name());
            BinSpan span{0, 0, 0};
            𐤀𐤌 (bin != bins.spans.end()) {
                span = bin->second;
            } وإلا 𐤀𐤌 (!implicitAutoBinSpan(bins, selectp->name(), span)) {
                selectp->v3error("Cannot find bin " << selectp->prettyNameQ() << " in coverpoint "
                                                    << selectp->pointp()->prettyNameQ()
                                                    << " (IEEE 1800-2023 19.6.1).");
                أرجع {};
            }
            target.m_first = span.first;
            target.m_count = span.count;
            target.m_declaredFirst = span.declared;
            target.m_declaredEnd = span.declared + span.count;
            target.m_sized = span.sized;
        }
        أرجع target;
    }

    // The variables a 'with' filter reads, each by its first reference
    using CrossFilterReads = std::map<ثابت AstVar*, AstVarRef*>;

    // Whether a 'with' filter has a count of value tuples, of 'matches' other than '$'
    ساكن منطقي countsValueTuples(ثابت AstCoverCrossWith* withp) {
        أرجع withp->countp() && !VN_IS(withp->countp(), Unbounded);
    }

    // Whether the runtime knows the values of the cross items a 'with' filter reads, or to count
    // value tuples, of all items, of integral coverpoints; else warns
    ساكن منطقي crossFilterSupported(ثابت AstCoverCrossWith* withp,
                                     ثابت CrossFilterReads& reads) {
        لكل (ثابت AstVar* varp = withp->varsp(); varp; varp = VN_AS(varp->nextp(), Var)) {
            𐤀𐤌 (varp->dtypep()->skipRefp()->isIntegralOrPacked()) استمر;
            ثابت auto it = reads.find(varp);
            𐤀𐤌 (it != reads.end()) {
                it->second->v3warn(COVERIGN, "Unsupported: cross bin 'with' filter reading the "
                                             "value of non-integral coverpoint "
                                                 << it->second->prettyNameQ());
                أرجع خطأ;
            }
            𐤀𐤌 (countsValueTuples(withp)) {
                withp->countp()->v3warn(COVERIGN, "Unsupported: cross bin 'matches' count of "
                                                  "value tuples of non-integral coverpoint "
                                                      << varp->prettyNameQ());
                أرجع خطأ;
            }
        }
        أرجع true;
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
    فراغ generateCrossFilter(ثابت AstCoverCrossWith* withp, ثابت CrossFilterReads& reads,
                             ثابت AstCoverCrossBin* binp, ثابت AstCoverCross* crossp,
                             AstVar* cxp, ثابت std::map<string, uint32_t>& dimensions,
                             uint32_t items) {
        FileLine* ثابت fl = withp->fileline();
        ثابت string prefix
            = m_sizedNames.get(sanitizeGeneratedName(crossp->name() + "__" + binp->name()));
        ثابت منطقي counting = countsValueTuples(withp);
        // The count, 0 for all
        ثابت std::pair<AstNodeExpr*, AstNodeExpr*> count
            = counting ? newCountArgs(fl, prefix + "_count", withp->countp())
                       : newCountArgs(fl, withp->countp() ? 0 : 1);
        AstLoop* ثابت loopp = جديد AstLoop{fl};
        AstCMethodHard* ثابت nextp = itemCall(fl, cxp, VCMethod::COVERGROUP_WITH_NEXT);
        nextp->dtypeSetBit();
        loopp->addStmtsp(جديد AstLoopTest{fl, loopp, nextp});
        string dims;
        لكل (uint32_t dim = 0; counting && dim < items; ++dim) {
            dims += (dims.empty() ? "" : ", ") + cvtToStr(dim) + "U";
        }
        طالما (AstVar* ثابت varp = withp->varsp()) {
            varp->unlinkFrBack();
            𐤀𐤌 (!reads.count(varp)) {
                VL_DO_DANGLING(pushDeletep(varp), varp);
                استمر;
            }
            ثابت auto dimIt = dimensions.find(varp->name());
            UASSERT_OBJ(dimIt != dimensions.end(), varp, "Cross 'with' candidate not of an item");
            ثابت uint32_t dim = dimIt->second;
            𐤀𐤌 (!counting) dims += (dims.empty() ? "" : ", ") + cvtToStr(dim) + "U";
            varp->name(prefix + "_" + sanitizeGeneratedName(varp->name()));
            m_constructorp->addStmtsp(varp);
            𐤀𐤌 (varp->isWide()) {
                loopp->addStmtsp(itemCall(fl, cxp, VCMethod::COVERGROUP_CROSS_WITH_VALUE_W,
                                          {cnum(fl, dim), جديد AstVarRef{fl, varp, VAccess::WRITE}})
                                     ->makeStmt());
            } وإلا {
                AstCMethodHard* ثابت valuep
                    = itemCall(fl, cxp, VCMethod::COVERGROUP_CROSS_WITH_VALUE, {cnum(fl, dim)});
                valuep->dtypeSetUInt64();
                AstNodeDType* ثابت dtypep = varp->findLogicDType(
                    varp->width(), varp->width(),
                    varp->isSigned() ? VSigning::SIGNED : VSigning::UNSIGNED);
                loopp->addStmtsp(جديد AstAssign{fl, جديد AstVarRef{fl, varp, VAccess::WRITE},
                                               resizeValue(valuep, dtypep)});
            }
        }
        loopp->addStmtsp(itemCall(fl, cxp, VCMethod::COVERGROUP_CROSS_WITH_TEST,
                                  {withp->filterp()->unlinkFrBack()})
                             ->makeStmt());
        ثابت منطقي prot = v3Global.opt.protectIds();
        m_constructorp->addStmtsp(
            itemCall(fl, cxp, VCMethod::COVERGROUP_CROSS_WITH_BEGIN,
                     {ctext(fl, "{" + dims + "}"), count.first, count.second,
                      ctext(fl, quoted(VIdProtect::protectWordsIf(binp->name(), prot))),
                      ctext(fl, quoted(VIdProtect::protectIf(fl->filename(), prot))),
                      cnum(fl, static_cast<uint32_t>(fl->lineno()))})
                ->makeStmt());
        m_constructorp->addStmtsp(loopp);
        m_constructorp->addStmtsp(
            itemCall(fl, cxp, VCMethod::COVERGROUP_CROSS_WITH_END)->makeStmt());
    }

    منطقي generateRuntimeSelection(AstNode* nodep, ثابت AstCoverCrossBin* binp,
                                  AstCoverCross* crossp, AstVar* cxp,
                                  ثابت std::vector<AstVar*>& cpVars,
                                  ثابت std::map<string, uint32_t>& dimensions) {
        FileLine* ثابت fl = nodep->fileline();
        𐤀𐤌 (ثابت AstCoverCrossRef* ثابت refp = VN_CAST(nodep, CoverCrossRef)) {
            𐤀𐤌 (!checkCrossRef(refp, crossp)) أرجع خطأ;
            m_constructorp->addStmtsp(
                itemCall(fl, cxp, VCMethod::COVERGROUP_SELECT_ALL)->makeStmt());
            أرجع true;
        }
        𐤀𐤌 (ثابت AstCoverCrossSelect* ثابت opp = VN_CAST(nodep, CoverCrossSelect)) {
            𐤀𐤌 (!generateRuntimeSelection(opp->lhsp(), binp, crossp, cxp, cpVars, dimensions)
                || !generateRuntimeSelection(opp->rhsp(), binp, crossp, cxp, cpVars, dimensions)) {
                أرجع خطأ;
            }
            m_constructorp->addStmtsp(itemCall(fl, cxp,
                                               opp->isOr() ? VCMethod::COVERGROUP_SELECT_OR
                                                           : VCMethod::COVERGROUP_SELECT_AND)
                                          ->makeStmt());
            أرجع true;
        }
        𐤀𐤌 (ثابت AstCoverCrossWith* ثابت withp = VN_CAST(nodep, CoverCrossWith)) {
            CrossFilterReads reads;
            withp->filterp()->foreach([&](AstVarRef* refp) { reads.emplace(refp->varp(), refp); });
            // The count of 'matches' is a positive integer, or '$' (IEEE 1800-2023 19.6.1.2)
            𐤀𐤌 ((countsValueTuples(withp)
                 && !checkCount(withp->countp(), "Cross bin 'matches' count", "19.6.1.2"))
                || !crossFilterSupported(withp, reads)
                || !generateRuntimeSelection(withp->selectp(), binp, crossp, cxp, cpVars,
                                             dimensions)) {
                أرجع خطأ;
            }
            generateCrossFilter(withp, reads, binp, crossp, cxp, dimensions,
                                static_cast<uint32_t>(cpVars.size()));
            أرجع true;
        }
        AstCoverBinsof* ثابت selectp = VN_AS(nodep, CoverBinsof);
        ثابت CrossBinsofTarget target = resolveBinsofTarget(selectp, crossp, cpVars, dimensions);
        𐤀𐤌 (!target.m_binsp) أرجع خطأ;
        ثابت CoverpointBins& bins = *target.m_binsp;
        𐤀𐤌 (selectp->rangesp() && !bins.exprp->dtypep()->skipRefp()->isIntegralOrPacked()) {
            منطقي valid = true;
            unsupportedCrossRange(selectp, valid);
            أرجع خطأ;
        }
        AstNodeExpr* firstp;
        AstNodeExpr* endp;
        𐤀𐤌 (target.m_sized < 0) {
            firstp = cnum(fl, target.m_declaredFirst);
            endp = cnum(fl, target.m_declaredEnd);
        } وإلا {  // A sized array, whose bins the coverpoint's construction placed
            AstVar* ثابت cpVarp = cpVars[target.m_dimension];
            ثابت uint32_t sized = static_cast<uint32_t>(target.m_sized);
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
        لكل (AstNode* rangep = selectp->rangesp(); rangep; rangep = rangep->nextp()) {
            CrossValueRange range{rangep, resolveWidth(rangep, bins.exprp)};
            𐤀𐤌 (!resolveValue(rangep, bins.exprp, خطأ, خطأ, range)) {
                منطقي valid = true;
                unsupportedCrossRange(selectp, valid);
                أرجع خطأ;
            }
            𐤀𐤌 (crossRangeEmpty(range)) استمر;
            m_constructorp->addStmtsp(itemCall(fl, cxp,
                                               bins.exprp->isWide()
                                                   ? VCMethod::COVERGROUP_SELECT_RANGE_W
                                                   : VCMethod::COVERGROUP_SELECT_RANGE,
                                               {newValueConst(fl, range.lo, bins.exprp),
                                                newValueConst(fl, range.hi, bins.exprp)})
                                          ->makeStmt());
        }
        m_constructorp->addStmtsp(
            itemCall(fl, cxp, VCMethod::COVERGROUP_SELECT_DIM_END)->makeStmt());
        أرجع true;
    }

    std::vector<AstCoverCrossBin*>
    generateRuntimeCrossBins(AstCoverCross* crossp, AstVar* cxp,
                             ثابت std::vector<AstVar*>& cpVars,
                             ثابت std::map<string, uint32_t>& dimensions) {
        std::vector<AstCoverCrossBin*> bins;
        std::set<string> names;
        لكل (AstNode* itemp = crossp->binsp(); itemp; itemp = itemp->nextp()) {
            AstCoverCrossBin* ثابت binp = VN_AS(itemp, CoverCrossBin);
            𐤀𐤌 (!checkCrossBinName(binp, names)) استمر;
            FileLine* ثابت fl = binp->fileline();
            // Generate the selection apart, so that an unsupported one leaves no code, which
            // could leave its state for the next bin
            AstFunc* ثابت selectionp = جديد AstFunc{fl, "", nullptr, nullptr};
            منطقي supported;
            {
                VL_RESTORER(m_constructorp);
                m_constructorp = selectionp;
                supported = generateRuntimeSelection(binp->selectp(), binp, crossp, cxp, cpVars,
                                                     dimensions);
            }
            // A supported selection has code
            𐤀𐤌 (supported) m_constructorp->addStmtsp(selectionp->stmtsp()->unlinkFrBackWithNext());
            VL_DO_DANGLING(pushDeletep(selectionp), selectionp);
            𐤀𐤌 (!supported) استمر;
            ثابت منطقي protect = v3Global.opt.protectIds();
            m_constructorp->addStmtsp(
                itemCall(fl, cxp, VCMethod::COVERGROUP_SELECT_BIN,
                         {ctext(fl, binp->binsType().binSetEnum()),
                          ctext(fl, quoted(VIdProtect::protectWordsIf(binp->name(), protect))),
                          ctext(fl, quoted(VIdProtect::protectIf(fl->filename(), protect))),
                          cnum(fl, fl->lineno()), cnum(fl, fl->firstColumn()),
                          cnum(fl, static_cast<uint32_t>(bins.size()))})
                    ->makeStmt());
            bins.push_back(binp);
        }
        m_constructorp->addStmtsp(
            itemCall(crossp->fileline(), cxp, VCMethod::COVERGROUP_FINALIZE_BINS)->makeStmt());
        أرجع bins;
    }

    ساكن فراغ setCrossSelectionRange(CrossSelection& selection, uint64_t first, uint64_t end) {
        طالما (first < end) {
            ثابت unsigned bit = first % 64;
            ثابت unsigned bits = std::min<uint64_t>(64 - bit, end - first);
            selection[VL_BITWORD_Q(first)]
                |= (bits == 64 ? ~uint64_t{0} : (uint64_t{1} << bits) - 1) << bit;
            first += bits;
        }
    }

    CrossSelection crossSelection(AstNode* nodep, CrossSelectionContext& ctx) {
        𐤀𐤌 (ثابت AstCoverCrossRef* ثابت refp = VN_CAST(nodep, CoverCrossRef)) {
            𐤀𐤌 (!checkCrossRef(refp, ctx.crossp)) {
                ctx.valid = خطأ;
                أرجع {};
            }
            CrossSelection result(
                VL_BITWORD_Q(static_cast<uint64_t>(ctx.tuples) + VL_QUADSIZE - 1), 0);
            setCrossSelectionRange(result, 0, ctx.tuples);
            أرجع result;
        }
        𐤀𐤌 (AstCoverCrossSelect* ثابت opp = VN_CAST(nodep, CoverCrossSelect)) {
            CrossSelection lhs = crossSelection(opp->lhsp(), ctx);
            ثابت CrossSelection rhs = crossSelection(opp->rhsp(), ctx);
            𐤀𐤌 (!ctx.valid) أرجع {};
            لكل (size_t i = 0; i < lhs.size(); ++i) {
                lhs[i] = opp->isOr() ? lhs[i] | rhs[i] : lhs[i] & rhs[i];
            }
            أرجع lhs;
        }
        AstCoverBinsof* ثابت selectp = VN_AS(nodep, CoverBinsof);
        ثابت CrossBinsofTarget target
            = resolveBinsofTarget(selectp, ctx.crossp, ctx.cpVars, ctx.dimensions);
        𐤀𐤌 (!target.m_binsp) {
            ctx.valid = خطأ;
            أرجع {};
        }
        ثابت CoverpointBins& bins = *target.m_binsp;
        // Coverpoints with sized arrays are runtime points, so feed only runtime crosses
        UASSERT_OBJ(target.m_sized < 0, selectp, "Sized bin array selected by a static cross");
        ثابت std::vector<منطقي> selected
            = selectCoverpointBins(selectp, bins, target.m_first, target.m_count, ctx.valid);
        𐤀𐤌 (!ctx.valid) أرجع {};
        CrossSelection result(VL_BITWORD_Q(static_cast<uint64_t>(ctx.tuples) + VL_QUADSIZE - 1),
                              0);
        ثابت uint64_t stride = ctx.strides[target.m_dimension];
        ثابت uint64_t period = stride * bins.total;
        لكل (uint64_t base = 0; base < ctx.tuples; base += period) {
            لكل (uint32_t i = 0; i < bins.total;) {
                𐤀𐤌 (!selected[i]) {
                    ++i;
                    استمر;
                }
                ثابت uint32_t begin = i++;
                طالما (i < bins.total && selected[i]) ++i;
                setCrossSelectionRange(result, base + begin * stride, base + i * stride);
            }
        }
        أرجع result;
    }

    // Size the Cartesian product of the declared Normal bins, with each dimension's flat-index
    // stride.  Live runtime bins only shrink it.  Warns and returns false if it is too large.
    منطقي crossShape(AstCoverCross* crossp, ثابت std::vector<AstVar*>& cpVars,
                    std::vector<uint32_t>& strides, uint32_t& tuples) ثابت {
        uint64_t product = std::any_of(cpVars.begin(), cpVars.end(),
                                       [هذا](AstVar* varp) { أرجع !m_cpBins.at(varp).total; })
                               ? 0
                               : 1;
        strides.resize(cpVars.size());
        لكل (size_t d = cpVars.size(); d > 0; --d) {
            strides[d - 1] = static_cast<uint32_t>(product);
            product *= m_cpBins.at(cpVars[d - 1]).total;
            𐤀𐤌 (product > UINT32_MAX) {
                crossp->v3warn(COVERIGN,
                               "Unsupported: cross coverage with more than 2^32-1 tuples.");
                أرجع خطأ;
            }
        }
        tuples = static_cast<uint32_t>(product);
        أرجع true;
    }

    CrossLayout resolveCrossLayout(AstCoverCross* crossp, ثابت std::vector<AstVar*>& cpVars,
                                   ثابت std::map<std::string, uint32_t>& dimensions) {
        CrossLayout layout;
        CrossSelectionContext ctx{crossp, cpVars, dimensions, 0, {}};
        𐤀𐤌 (!crossShape(crossp, cpVars, ctx.strides, ctx.tuples)) {
            layout.valid = خطأ;
            أرجع layout;
        }
        layout.tuples = ctx.tuples;
        CrossSelection occupied;
        CrossSelection excluded;
        std::set<std::string> names;
        لكل (AstNode* itemp = crossp->binsp(); itemp; itemp = itemp->nextp()) {
            AstCoverCrossBin* ثابت binp = VN_AS(itemp, CoverCrossBin);
            𐤀𐤌 (!checkCrossBinName(binp, names)) استمر;
            ctx.valid = true;
            CrossSelection selection = crossSelection(binp->selectp(), ctx);
            𐤀𐤌 (!ctx.valid || std::all_of(selection.begin(), selection.end(), [](uint64_t word) {
                    أرجع word == 0;
                })) {
                استمر;
            }
            𐤀𐤌 (occupied.empty()) {
                occupied.resize(selection.size(), 0);
                excluded.resize(selection.size(), 0);
            }
            لكل (size_t i = 0; i < selection.size(); ++i) {
                occupied[i] |= selection[i];
                𐤀𐤌 (!binp->binsType().binIsNormal()) excluded[i] |= selection[i];
            }
            layout.bins.push_back({binp, std::move(selection)});
        }
        𐤀𐤌 (!layout.bins.empty()) {
            // IEEE 1800-2023 19.6.2/19.6.3: exclusions also remove tuples from named
            // bins, independently of declaration order and sampling guards.
            لكل (ResolvedCrossBin& resolved : layout.bins) {
                لكل (size_t i = 0; i < resolved.selection.size(); ++i) {
                    𐤀𐤌 (resolved.binp->binsType().binIsNormal()) {
                        resolved.selection[i] &= ~excluded[i];
                    }
                    𐤀𐤌 (resolved.selection[i]) ++layout.binWords;
                }
            }
            layout.bins.erase(std::remove_if(layout.bins.begin(), layout.bins.end(),
                                             [](ثابت ResolvedCrossBin& resolved) {
                                                 أرجع std::all_of(
                                                     resolved.selection.begin(),
                                                     resolved.selection.end(),
                                                     [](uint64_t word) { أرجع word == 0; });
                                             }),
                              layout.bins.end());
            layout.autoBins = layout.tuples;
            لكل (ثابت uint64_t word : occupied) {
                layout.autoBins -= static_cast<uint32_t>(std::bitset<VL_QUADSIZE>{word}.count());
            }
        }
        أرجع layout;
    }

    AstCoverCrossDType* crossDType(FileLine* fl, uint32_t dimensions, ثابت CrossLayout& layout,
                                   منطقي dynamic = خطأ) {
        ثابت uint32_t bins = static_cast<uint32_t>(layout.bins.size());
        ثابت CrossShape shape{dimensions,      layout.tuples,   bins,
                               layout.autoBins, layout.binWords, dynamic};
        AstCoverCrossDType*& typep = m_cxDTypes[shape];
        𐤀𐤌 (!typep) {
            typep = جديد AstCoverCrossDType{
                fl, dimensions, layout.tuples, bins, layout.autoBins, layout.binWords, dynamic};
            v3Global.rootp()->typeTablep()->addTypesp(typep);
        }
        أرجع typep;
    }

    std::vector<AstCoverCrossBin*> generateCrossBins(AstCoverCross* crossp, AstVar* cxVarp,
                                                     ثابت CrossLayout& layout) {
        std::vector<AstCoverCrossBin*> bins;
        لكل (ثابت ResolvedCrossBin& resolved : layout.bins) {
            AstCoverCrossBin* ثابت binp = resolved.binp;
            ثابت CrossSelection& selection = resolved.selection;
            FileLine* ثابت fl = binp->fileline();
            ثابت منطقي prot = v3Global.opt.protectIds();
            std::string mask = "{";
            لكل (size_t i = 0; i < selection.size(); ++i) {
                𐤀𐤌 (i) mask += ", ";
                mask += std::to_string(selection[i]) + "ULL";
            }
            mask += "}";
            m_constructorp->addStmtsp(
                itemCall(fl, cxVarp, VCMethod::COVERGROUP_ADD_BIN,
                         {ctext(fl, binp->binsType().binSetEnum()), ctext(fl, mask),
                          ctext(fl, quoted(VIdProtect::protectWordsIf(binp->name(), prot))),
                          ctext(fl, quoted(VIdProtect::protectIf(fl->filename(), prot))),
                          cnum(fl, static_cast<uint32_t>(fl->lineno())),
                          cnum(fl, static_cast<uint32_t>(fl->firstColumn()))})
                    ->makeStmt());
            bins.push_back(binp);
        }
        𐤀𐤌 (!bins.empty()) {
            m_constructorp->addStmtsp(
                itemCall(crossp->fileline(), cxVarp, VCMethod::COVERGROUP_FINALIZE_BINS)
                    ->makeStmt());
        }
        أرجع bins;
    }

    // Route a cross through a VlCoverCross member: emit the member, its constructor init +
    // registration, and the sample() call.  The feeding coverpoints are already generated
    // (their hit lists drive the cross). Each explicit bin adds one configuration call.
    فراغ generateCross(AstCoverCross* crossp) {
        FileLine* ثابت fl = crossp->fileline();
        ثابت منطقي dynamic = m_runtimeCrosses.count(crossp);
        UINFO(4, "  Generating VlCoverCross member: " << crossp->name());

        𐤀𐤌 (AstNodeExpr* ثابت iffp = crossp->iffp()) {
            crossp->iffp(
                captureIffToTemp(iffp, "__VcrossIff_" + sanitizeGeneratedName(crossp->name())));
        }

        // Resolve and unlink the coverpoint refs, in dimension order.  Every ref resolves to a
        // known coverpoint (a cross with an unresolvable item was dropped earlier).
        std::vector<AstVar*> cpVars;
        std::map<std::string, uint32_t> dimensions;
        لكل (AstNode* itemp = crossp->itemsp(); itemp;) {
            AstNode* ثابت nextp = itemp->nextp();
            AstCoverpointRef* ثابت refp = VN_AS(itemp, CoverpointRef);
            ثابت auto it = m_cpVarMap.find(refp->name());
            UASSERT_OBJ(it != m_cpVarMap.end(), crossp, "Cross references an unknown coverpoint");
            dimensions.emplace(refp->name(), static_cast<uint32_t>(cpVars.size()));
            cpVars.push_back(it->second);
            VL_DO_DANGLING(pushDeletep(refp->unlinkFrBack()), refp);
            itemp = nextp;
        }
        ثابت صحيح dims = static_cast<صحيح>(cpVars.size());
        CrossLayout layout;
        𐤀𐤌 (dynamic) {
            // The runtime sizes the layout from live bins; only check the declared bound here.
            std::vector<uint32_t> strides;
            uint32_t tuples = 0;
            layout.valid = crossShape(crossp, cpVars, strides, tuples);
        } وإلا {
            layout = resolveCrossLayout(crossp, cpVars, dimensions);
        }
        𐤀𐤌 (!layout.valid) أرجع;

        AstVar* ثابت cxVarp
            = جديد AstVar{fl, VVarType::MEMBER, "__Vcx_" + crossp->name(),
                         crossDType(fl, static_cast<uint32_t>(dims), layout, dynamic)};
        m_covergroupp->addMembersp(cxVarp);
        m_constructorp->addStmtsp(makeItemCreate(fl, cxVarp,
                                                 dynamic ? VCMethod::COVERGROUP_ADD_CROSS_DYN
                                                         : VCMethod::COVERGROUP_ADD_CROSS));
        generateItemWeight(fl, cxVarp, crossp->optionsp());
        صحيح atLeastValue;
        صحيح autoBinMax;  // Not a cross option
        extractItemOptions(crossp->optionsp(), atLeastValue, autoBinMax);

        // Constructor: init (after the coverpoints, which generate earlier) then registration.
        // Obfuscate the hierarchy/filename/page under --protect-ids as for coverpoints above.
        ثابت منطقي prot = v3Global.opt.protectIds();
        ثابت std::string hier
            = VIdProtect::protectWordsIf(m_covergroupName + "." + crossp->name(), prot);
        m_constructorp->addStmtsp(makeCrossCpsCall(
            fl, cpVars,
            itemCall(fl, cxVarp, VCMethod::COVERGROUP_INIT,
                     {ctext(fl, quoted(hier)), cnum(fl, static_cast<uint32_t>(atLeastValue)),
                      cnum(fl, static_cast<uint32_t>(dims)), ctext(fl, "__Vcx_cps"),
                      ctext(fl, quoted(VIdProtect::protectIf(fl->filename(), prot))),
                      cnum(fl, static_cast<uint32_t>(fl->lineno())),
                      cnum(fl, static_cast<uint32_t>(fl->firstColumn()))})));
        ثابت std::vector<AstCoverCrossBin*> bins
            = dynamic ? generateRuntimeCrossBins(crossp, cxVarp, cpVars, dimensions)
                      : generateCrossBins(crossp, cxVarp, layout);
        𐤀𐤌 (v3Global.opt.coverage()) {
            ثابت std::string page
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
        ثابت منطقي hasIffs
            = std::any_of(bins.begin(), bins.end(),
                          [](ثابت AstCoverCrossBin* binp) { أرجع binp->iffp() != nullptr; });
        AstNodeStmt* ثابت samplep
            = !hasIffs ? static_cast<AstNodeStmt*>(
                             itemCall(fl, cxVarp, VCMethod::COVERGROUP_SAMPLE)->makeStmt())
                       : static_cast<AstNodeStmt*>(makeCrossIffsCall(
                             fl, bins,
                             itemCall(fl, cxVarp, VCMethod::COVERGROUP_SAMPLE_IFFS,
                                      {ctext(fl, "__Vcx_iffs")})));
        𐤀𐤌 (AstNodeExpr* ثابت iffp = crossp->iffp()) {
            m_sampleFuncp->addStmtsp(جديد AstIf{fl, iffp->cloneTree(خطأ), samplep});
        } وإلا {
            m_sampleFuncp->addStmtsp(samplep);
        }
    }

    فراغ generateCrossCode(AstCoverCross* crossp) {
        UINFO(4, "  Generating code for cross: " << crossp->name());

        // Non-standard hierarchical/dotted cross item (e.g. 'cross a.b'): an implicit coverpoint
        // over the referenced expression (carried in refp->exprp()).  The grammar already warned
        // NONSTD; implicit coverpoints are not yet implemented, so generate no sampling code for
        // this cross.  When support is added the implicit coverpoint should be synthesized
        // upstream (V3LinkParse) as a real AstCoverpoint so it flows through the normal coverpoint
        // path - by here coverpoint lowering has already run.
        لكل (AstNode* itemp = crossp->itemsp(); itemp; itemp = itemp->nextp()) {
            ثابت AstCoverpointRef* ثابت refp = VN_AS(itemp, CoverpointRef);
            𐤀𐤌 (refp->exprp()) {
                refp->v3warn(COVERIGN,
                             "Unsupported: cross of hierarchical reference (implicit coverpoint)");
                أرجع;
            }
        }

        // A cross naming a bare variable (implicit coverpoint, which Verilator does not
        // synthesize) is dropped entirely with a COVERIGN warning -- it produces no coverage
        // either way -- but only this cross is dropped; its sibling crosses are still generated
        // and the real coverpoints it referenced remain as independent coverpoints.
        𐤀𐤌 (m_droppedCrosses.count(crossp)) {
            لكل (AstNode* itemp = crossp->itemsp(); itemp; itemp = itemp->nextp()) {
                ثابت AstCoverpointRef* ثابت refp = VN_AS(itemp, CoverpointRef);
                𐤀𐤌 (m_coverpointMap.find(refp->name()) == m_coverpointMap.end()) {
                    refp->v3warn(COVERIGN, "Unsupported: cross of "
                                               << refp->prettyNameQ()
                                               << " which is not a coverpoint (implicit "
                                                  "coverpoint)");
                    اكسر;
                }
            }
            أرجع;
        }

        // Every cross that isn't dropped routes through a VlCoverCross member.
        generateCross(crossp);
    }

    AstNodeExpr* buildBinCondition(AstCoverBin* binp, AstNodeExpr* exprp) {
        // Get the range list from the bin
        AstNode* ثابت rangep = binp->rangesp();
        𐤀𐤌 (!rangep) أرجع nullptr;

        // Integral values resolve to the coverpoint's type, as its runtime metadata does
        ثابت منطقي integral = exprp->dtypep()->skipRefp()->isIntegralOrPacked();
        // No value form of a wildcard bin is allowed on a real coverpoint (IEEE 1800-2023 19.5.4)
        𐤀𐤌 (binp->isWildcard() && !integral) أرجع wildcardTypeError(binp, exprp);

        // Build condition by OR-ing all ranges together
        AstNodeExpr* fullCondp = nullptr;

        لكل (AstNode* currRangep = rangep; currRangep; currRangep = currRangep->nextp()) {
            AstNodeExpr* rangeCondp = nullptr;
            currRangep = V3Const::constifyEdit(currRangep);

            𐤀𐤌 (AstInsideRange* irp = VN_CAST(currRangep, InsideRange)) {
                AstNodeExpr* ثابت minExprp = irp->lhsp();
                AstNodeExpr* ثابت maxExprp = irp->rhsp();
                AstConst* ثابت minConstp = VN_CAST(minExprp, Const);
                AstConst* ثابت maxConstp = VN_CAST(maxExprp, Const);
                ثابت منطقي loUnbounded = VN_IS(minExprp, Unbounded);
                ثابت منطقي hiUnbounded = VN_IS(maxExprp, Unbounded);
                𐤀𐤌 (loUnbounded || hiUnbounded) {
                    // Open-ended range: '$' is the coverpoint domain min/max, so the
                    // range reduces to a single inequality (e.g. {[10:$]} -> expr >= 10).
                    AstConst* ثابت boundp = hiUnbounded ? minConstp : maxConstp;
                    𐤀𐤌 (loUnbounded && hiUnbounded) {
                        rangeCondp = جديد AstConst{irp->fileline(), AstConst::BitTrue{}};
                    } وإلا 𐤀𐤌 (!boundp) {
                        irp->v3error("Non-constant expression in bin range; "
                                     "range bounds must be constants (IEEE 1800-2023 19.5)");
                        𐤀𐤌 (fullCondp) VL_DO_DANGLING(pushDeletep(fullCondp), fullCondp);
                        أرجع nullptr;
                    } وإلا 𐤀𐤌 (boundp->num().isFourState()) {
                        irp->v3error("Four-state (x/z) value in bin range bound; "
                                     "range bounds must be two-state constants");
                        𐤀𐤌 (fullCondp) VL_DO_DANGLING(pushDeletep(fullCondp), fullCondp);
                        أرجع nullptr;
                    } وإلا 𐤀𐤌 (integral) {
                        rangeCondp = buildValueCondition(binp, exprp, irp);
                    } وإلا {
                        rangeCondp = makeOpenRangeCondition(irp->fileline(), exprp, boundp,
                                                            /*isLowerBound=*/hiUnbounded);
                    }
                } وإلا 𐤀𐤌 (!minConstp || !maxConstp) {
                    irp->v3error("Non-constant expression in bin range; "
                                 "range bounds must be constants (IEEE 1800-2023 19.5)");
                    𐤀𐤌 (fullCondp) VL_DO_DANGLING(pushDeletep(fullCondp), fullCondp);
                    أرجع nullptr;
                } وإلا 𐤀𐤌 (minConstp->num().isFourState() || maxConstp->num().isFourState()) {
                    irp->v3error("Four-state (x/z) value in bin range bound; "
                                 "range bounds must be two-state constants");
                    𐤀𐤌 (fullCondp) VL_DO_DANGLING(pushDeletep(fullCondp), fullCondp);
                    أرجع nullptr;
                } وإلا 𐤀𐤌 (integral) {
                    rangeCondp = buildValueCondition(binp, exprp, irp);
                } وإلا {
                    rangeCondp = makeRangeCondition(irp->fileline(), exprp, minExprp, maxExprp);
                }
            } وإلا 𐤀𐤌 (AstConst* constp = VN_CAST(currRangep, Const)) {
                rangeCondp = buildValueCondition(binp, exprp, constp);
            } وإلا {
                currRangep->v3error("Non-constant expression in bin range; values must be "
                                    "constants (IEEE 1800-2023 19.5)");
                𐤀𐤌 (fullCondp) VL_DO_DANGLING(pushDeletep(fullCondp), fullCondp);
                أرجع nullptr;
            }

            UASSERT_OBJ(rangeCondp, binp, "rangeCondp is null after building range condition");
            fullCondp
                = fullCondp ? جديد AstOr{binp->fileline(), fullCondp, rangeCondp} : rangeCondp;
        }

        أرجع fullCondp;
    }

    // Wildcard bits have no meaning for a non-integral coverpoint.
    ساكن AstNodeExpr* wildcardTypeError(AstCoverBin* binp, AstNodeExpr* exprp) {
        ثابت AstNodeDType* ثابت dtypep = exprp->dtypep()->skipRefp();
        exprp->v3error("Cannot use a wildcard bin on a coverpoint of type "
                       << dtypep->prettyDTypeNameQ() << " (IEEE 1800-2023 19.5.4).\n"
                       << exprp->warnContextPrimary() << '\n'
                       << binp->warnOther() << "... Location of wildcard bin\n"
                       << binp->warnContextSecondary());
        أرجع جديد AstConst{binp->fileline(), AstConst::BitFalse{}};
    }

    // Match one bin value, range, or wildcard pattern.  Integral coverpoints first resolve the
    // value to their type (IEEE 1800-2023 19.5.7).  Non-owning: clones what it uses.
    AstNodeExpr* buildValueCondition(AstCoverBin* binp, AstNodeExpr* exprp, AstNode* valuep) {
        FileLine* ثابت fl = valuep->fileline();
        𐤀𐤌 (!exprp->dtypep()->skipRefp()->isIntegralOrPacked()) {
            أرجع جديد AstEq{fl, exprp->cloneTree(خطأ),
                             VN_AS(valuep, NodeExpr)->cloneTree(خطأ)};
        }
        CrossValueRange range{valuep, resolveWidth(valuep, exprp)};
        𐤀𐤌 (!resolveValue(valuep, exprp, true, binp->isWildcard(), range)) {
            valuep->v3warn(E_UNSUPPORTED,
                           "Unsupported: non-integral value in a coverage bin of an "
                           "integral coverpoint.");
            أرجع جديد AstConst{fl, AstConst::BitFalse{}};
        }
        𐤀𐤌 (crossRangeEmpty(range)) أرجع جديد AstConst{fl, AstConst::BitFalse{}};
        AstConst* ثابت lop = newValueConst(fl, range.lo, exprp);
        AstConst* ثابت hip = newValueConst(fl, range.hi, exprp);
        AstNodeExpr* condp = nullptr;
        𐤀𐤌 (lop->num().isCaseEq(hip->num())) {
            condp = جديد AstEq{fl, exprp->cloneTree(خطأ), lop};
        } وإلا {
            condp = makeRangeCondition(fl, exprp, lop, hip);
            VL_DO_DANGLING(pushDeletep(lop), lop);
        }
        VL_DO_DANGLING(pushDeletep(hip), hip);
        𐤀𐤌 (!range.wildcard) أرجع condp;
        // Match the pattern's value bits within the source-value bounds.
        V3Number mask{valuep, exprp->width()};
        V3Number value{valuep, exprp->width()};
        mask.opBitsNonXZ(range.pattern);
        value.opBitsOne(range.pattern);
        AstConst* ثابت maskConstp = newValueConst(fl, mask, exprp);
        AstConst* ثابت valueConstp = newValueConst(fl, value, exprp);
        AstNodeExpr* ثابت exprMasked = جديد AstAnd{fl, exprp->cloneTree(خطأ), maskConstp};
        AstNodeExpr* ثابت valueMasked = جديد AstAnd{fl, valueConstp, maskConstp->cloneTree(خطأ)};
        أرجع جديد AstLogAnd{fl, condp, جديد AstEq{fl, exprMasked, valueMasked}};
    }

    فراغ generateCoverageComputationCode() {
        UINFO(4, "  Generating coverage computation code");

        // Invalidate cache: addMembersp() calls in generateCoverpointCode/generateCrossCode
        // have added new members since the last scan, so clear before re-querying.
        m_memberMap.clear();

        // get_inst_coverage(): the average of the coverpoints and crosses, weighted by their
        // option.weight (IEEE 1800-2023 19.11).  The instance node holds their runtimes.  With
        // the instances merged, get_coverage() instead, unless option.get_inst_coverage
        // (Table 19-1).
        AstFunc* ثابت getInstCoveragep
            = VN_AS(m_memberMap.findMember(m_covergroupp, "get_inst_coverage"), Func);
        FileLine* ثابت instFl = getInstCoveragep->fileline();
        AstCMethodHard* ثابت instCallp = instanceCall(instFl, VCMethod::COVERGROUP_COVERAGE);
        instCallp->dtypeSetDouble();
        AstNodeExpr* instValuep = instCallp;
        𐤀𐤌 (m_cgMayMerge) {
            AstNodeExpr* ثابت mergedp = جديد AstLogAnd{
                instFl, newOptionSel(instFl, optionVar(true), "merge_instances", VAccess::READ),
                جديد AstLogNot{instFl, newOptionSel(instFl, optionVar(خطأ), "get_inst_coverage",
                                                   VAccess::READ)}};
            instValuep = جديد AstCond{instFl, mergedp, typeCoverageCall(instFl), instCallp};
        }
        getInstCoveragep->addStmtsp(جديد AstAssign{
            instFl, جديد AstVarRef{instFl, VN_AS(getInstCoveragep->fvarp(), Var), VAccess::WRITE},
            instValuep});

        // get_coverage(): the average of the covergroup's instances, weighted by their
        // option.weight, or with type_option.merge_instances, the coverage of the union of their
        // bins (IEEE 1800-2023 19.11.3).  Static, so the registry finds the instances.
        AstFunc* ثابت getCoveragep
            = VN_AS(m_memberMap.findMember(m_covergroupp, "get_coverage"), Func);
        FileLine* ثابت typeFl = getCoveragep->fileline();
        getCoveragep->addStmtsp(جديد AstAssign{
            typeFl, جديد AstVarRef{typeFl, VN_AS(getCoveragep->fvarp(), Var), VAccess::WRITE},
            typeCoverageCall(typeFl)});
    }

    // The registry call computing the covergroup's type coverage, per its type options
    AstCMethodHard* typeCoverageCall(FileLine* fl) {
        AstCExpr* ثابت registryp = ctext(fl, "vlSymsp->_vm_contextp__->covergroupRegistryp()");
        registryp->dtypeSetVoid();  // Opaque receiver; only ever the 'fromp' of the call below
        AstCMethodHard* ثابت callp
            = جديد AstCMethodHard{fl, registryp, VCMethod::COVERGROUP_TYPE_COVERAGE};
        callp->addPinsp(ctext(fl, quoted(covergroupProtectedName())));
        callp->addPinsp(newOptionSel(fl, optionVar(true), "weight", VAccess::READ));
        callp->addPinsp(newOptionSel(fl, optionVar(true), "merge_instances", VAccess::READ));
        callp->addPinsp(fileLineDebug(m_covergroupp->fileline()));
        callp->usePtr(true);
        callp->dtypeSetDouble();
        أرجع callp;
    }

    // VISITORS
    ساكن منطقي isEnclosingInstanceVar(ثابت AstVar* varp) {
        أرجع varp->isClassMember() && !varp->lifetime().isStatic() && !varp->isParam();
    }

    فراغ rewriteThisRef(AstThisRef* refp, AstVar* handleVarp) {
        ثابت AstClassRefDType* ثابت refDTypep
            = VN_CAST(refp->dtypep()->skipRefp(), ClassRefDType);
        UASSERT_OBJ(refDTypep && refDTypep->classp() == m_covergroupp, refp,
                    "Unexpected this reference in embedded covergroup");
        AstNodeExpr* ثابت newp = جديد AstVarRef{refp->fileline(), handleVarp, VAccess::READ};
        refp->replaceWith(newp);
        VL_DO_DANGLING(pushDeletep(refp), refp);
    }

    فراغ rewriteVarRef(AstVarRef* refp, AstVar* handleVarp) {
        FileLine* ثابت fl = refp->fileline();
        // As V3LinkLValue does, the handle of a written member select is marked written
        AstMemberSel* ثابت selp
            = جديد AstMemberSel{fl, جديد AstVarRef{fl, handleVarp, refp->access()}, refp->varp()};
        selp->access(refp->access());
        refp->replaceWith(selp);
        VL_DO_DANGLING(pushDeletep(refp), refp);
    }

    فراغ rewriteFuncRef(AstFuncRef* refp, AstVar* handleVarp) {
        FileLine* ثابت fl = refp->fileline();
        AstArg* ثابت argsp = refp->argsp() ? refp->argsp()->unlinkFrBackWithNext() : nullptr;
        AstMethodCall* ثابت callp = جديد AstMethodCall{
            fl, جديد AstVarRef{fl, handleVarp, VAccess::READ}, refp->name(), argsp};
        callp->taskp(refp->taskp());
        callp->dtypeFrom(refp);
        refp->replaceWith(callp);
        VL_DO_DANGLING(pushDeletep(refp), refp);
    }

    // True if funcp is an instance method of the enclosing class or one of its bases
    منطقي isEnclosingInstanceFunc(ثابت AstNodeFTask* funcp) ثابت {
        𐤀𐤌 (!funcp->classMethod() || funcp->isStatic()) أرجع خطأ;
        أرجع AstClass::isClassExtendedFrom(m_enclosingClassp, VN_AS(funcp->aboveLoopp(), Class));
    }

    منطقي isEmbeddedCovergroupVar(ثابت AstVar* varp) ثابت {
        𐤀𐤌 (!varp || !varp->isClassMember() || varp->isDeclTyped()) أرجع خطأ;
        ثابت AstClassRefDType* ثابت refp = VN_CAST(varp->dtypep()->skipRefp(), ClassRefDType);
        أرجع refp && refp->classp() == m_covergroupp;
    }

    AstVar* findEmbeddedCovergroupVar() ثابت {
        𐤀𐤌 (!m_enclosingClassp) أرجع nullptr;
        لكل (AstNode* itemp = m_enclosingClassp->membersp(); itemp; itemp = itemp->nextp()) {
            𐤀𐤌 (AstVar* ثابت varp = VN_CAST(itemp, Var)) {
                𐤀𐤌 (isEmbeddedCovergroupVar(varp)) أرجع varp;
            }
        }
        // V3LinkParse always creates an implicit variable for an embedded covergroup.
        أرجع nullptr;  // LCOV_EXCL_LINE
    }

    std::vector<AstNodeAssign*> findCovergroupConstructions() {
        std::vector<AstNodeAssign*> foundps;
        𐤀𐤌 (!m_embeddedVarp) أرجع foundps;
        AstFunc* ثابت enclosingNewp
            = VN_CAST(m_memberMap.findMember(m_enclosingClassp, "new"), Func);
        𐤀𐤌 (!enclosingNewp) أرجع foundps;
        enclosingNewp->foreach([&](AstNodeAssign* asgnp) {
            ثابت AstNew* ثابت newp = VN_CAST(asgnp->rhsp(), New);
            ثابت AstVarRef* ثابت lhsRefp = VN_CAST(asgnp->lhsp(), VarRef);
            𐤀𐤌 (!newp || !lhsRefp || lhsRefp->varp() != m_embeddedVarp) أرجع;
            ثابت AstClassRefDType* ثابت refp = VN_CAST(newp->dtypep(), ClassRefDType);
            𐤀𐤌 (refp && refp->classp() == m_covergroupp) foundps.push_back(asgnp);
        });
        أرجع foundps;
    }

    std::set<ثابت AstVar*> enclosingInstanceVars() ثابت {
        std::set<ثابت AstVar*> vars;
        𐤀𐤌 (m_enclosingClassp) {
            m_enclosingClassp->foreachMember([&](AstClass* ثابت, AstVar* ثابت varp) {
                𐤀𐤌 (isEnclosingInstanceVar(varp)) vars.insert(varp);
            });
        }
        أرجع vars;
    }

    منطقي hasEnclosingEventRef(AstCovergroup* cgp) ثابت {
        𐤀𐤌 (!m_embeddedVarp || !cgp->eventp()) أرجع خطأ;
        ثابت std::set<ثابت AstVar*> enclosingVars = enclosingInstanceVars();
        منطقي found = خطأ;
        cgp->eventp()->foreach([&](AstVarRef* refp) {
            𐤀𐤌 (enclosingVars.count(refp->varp())) found = true;
        });
        أرجع found;
    }

    ساكن منطقي parseEmbeddedEventExpr(AstNodeExpr* exprp, AstVar*& baseVarp,
                                       AstVar*& memberVarp) {
        𐤀𐤌 (AstVarRef* ثابت refp = VN_CAST(exprp, VarRef)) {
            baseVarp = refp->varp();
            memberVarp = nullptr;
            أرجع true;
        }
        AstMemberSel* ثابت selp = VN_CAST(exprp, MemberSel);
        𐤀𐤌 (!selp) أرجع خطأ;
        AstVarRef* ثابت baseRefp = VN_CAST(selp->fromp(), VarRef);
        𐤀𐤌 (!baseRefp) أرجع خطأ;
        baseVarp = baseRefp->varp();
        memberVarp = selp->varp();
        أرجع true;
    }

    منطقي isEventLvalue(AstNodeExpr* exprp, ثابت EmbeddedEventTrigger& trigger) ثابت {
        𐤀𐤌 (AstSel* ثابت selp = VN_CAST(exprp, Sel)) exprp = selp->fromp();
        AstVar* baseVarp = nullptr;
        AstVar* memberVarp = nullptr;
        𐤀𐤌 (!parseEmbeddedEventExpr(exprp, baseVarp, memberVarp)) أرجع خطأ;
        أرجع baseVarp == trigger.baseVarp && memberVarp == trigger.memberVarp;
    }

    AstNodeExpr* newEventRead(FileLine* fl, ثابت EmbeddedEventTrigger& trigger) ثابت {
        AstNodeExpr* ثابت basep = جديد AstVarRef{fl, trigger.baseVarp, VAccess::READ};
        𐤀𐤌 (!trigger.memberVarp) أرجع basep;
        AstMemberSel* ثابت selp = جديد AstMemberSel{fl, basep, trigger.memberVarp};
        selp->access(VAccess::READ);
        أرجع selp;
    }

    string eventPrevName(ثابت EmbeddedEventTrigger& trigger, size_t triggerIndex) ثابت {
        string name = "__Vcg_prev_" + m_embeddedVarp->name() + "_" + std::to_string(triggerIndex)
                      + "_" + trigger.baseVarp->name();
        𐤀𐤌 (trigger.memberVarp) name += "_" + trigger.memberVarp->name();
        أرجع name;
    }

    AstNodeExpr* newEmbeddedVarNonNull(FileLine* fl) ثابت {
        أرجع جديد AstNeq{fl, جديد AstVarRef{fl, m_embeddedVarp, VAccess::READ},
                          جديد AstConst{fl, AstConst::Null{}}};
    }

    AstNodeStmt* newSampleStmt(FileLine* fl) ثابت {
        AstMethodCall* ثابت callp = جديد AstMethodCall{
            fl, جديد AstVarRef{fl, m_embeddedVarp, VAccess::READ}, "sample", nullptr};
        callp->taskp(m_sampleFuncp);
        callp->dtypeSetVoid();
        أرجع callp->makeStmt();
    }

    فراغ installEmbeddedEventFork(AstSenTree* eventp,
                                  ثابت std::vector<AstNodeAssign*>& constructps) {
        // IEEE 1800-2023 19.3 samples coverpoints whenever their clocking event occurs. A
        // per-instance event cannot use V3Active's static sensitivity path, so spawn
        // 'fork forever begin @(event); cg.sample(); end join_none' after each construction.
        لكل (AstNodeAssign* ثابت constructp : constructps) {
            FileLine* ثابت fl = constructp->fileline();
            AstLoop* ثابت loopp = جديد AstLoop{fl};
            loopp->addStmtsp(جديد AstEventControl{fl, eventp->cloneTree(خطأ), nullptr});
            loopp->addStmtsp(جديد AstIf{fl, newEmbeddedVarNonNull(fl), newSampleStmt(fl)});
            AstFork* ثابت forkp = جديد AstFork{fl, VJoinType::JOIN_NONE};
            forkp->immediateStart(true);
            forkp->addForksp(جديد AstBegin{fl, "", loopp, true});
            constructp->addNextHere(forkp);
        }
        VL_DO_DANGLING(pushDeletep(eventp), eventp);
    }

    AstNodeExpr* newEventReadyCondition(FileLine* fl, ثابت EmbeddedEventTrigger& trigger) ثابت {
        AstNodeExpr* ثابت curp = newEventRead(fl, trigger);
        AstNodeExpr* ثابت prevp = جديد AstVarRef{fl, trigger.prevVarp, VAccess::READ};
        AstNodeExpr* edgep = nullptr;
        // IEEE 1800-2023 9.4.2 detects edge-qualified events only on the expression's LSB,
        // while an implicit change event observes the complete expression.
        𐤀𐤌 (trigger.edgeType == VEdgeType::ET_POSEDGE) {
            edgep = جديد AstSel{fl, جديد AstAnd{fl, curp, جديد AstNot{fl, prevp}}, 0, 1};
        } وإلا 𐤀𐤌 (trigger.edgeType == VEdgeType::ET_NEGEDGE) {
            edgep = جديد AstSel{fl, جديد AstAnd{fl, جديد AstNot{fl, curp}, prevp}, 0, 1};
        } وإلا 𐤀𐤌 (trigger.edgeType == VEdgeType::ET_BOTHEDGE) {
            edgep = جديد AstSel{fl, جديد AstXor{fl, curp, prevp}, 0, 1};
        } وإلا {
            edgep = جديد AstNeq{fl, curp, prevp};
        }
        أرجع جديد AstLogAnd{fl, newEmbeddedVarNonNull(fl), edgep};
    }

    std::vector<EmbeddedEventTrigger> collectEmbeddedEventTriggers(AstCovergroup* cgp) {
        std::vector<EmbeddedEventTrigger> triggers;
        ثابت std::set<ثابت AstVar*> enclosingVars = enclosingInstanceVars();
        لكل (AstNode* senp = cgp->eventp()->sensesp(); senp; senp = senp->nextp()) {
            AstSenItem* ثابت itemp = VN_AS(senp, SenItem);
            AstVar* baseVarp = nullptr;
            AstVar* memberVarp = nullptr;
            𐤀𐤌 (!parseEmbeddedEventExpr(itemp->sensp(), baseVarp, memberVarp)
                || !enclosingVars.count(baseVarp)) {
                أرجع {};
            }
            triggers.emplace_back(itemp->fileline(), baseVarp, memberVarp, itemp->edgeType());
        }
        أرجع triggers;
    }

    فراغ installEmbeddedEventTriggers(std::vector<EmbeddedEventTrigger>& triggers,
                                      ثابت std::vector<AstNodeAssign*>& constructps) {
        // Without --timing, approximate a per-instance event by sampling after assignments
        // within the enclosing class. External writes and exact scheduling cannot be observed.
        𐤀𐤌 (constructps.empty()) أرجع;
        لكل (size_t triggerIndex = 0; triggerIndex < triggers.size(); ++triggerIndex) {
            EmbeddedEventTrigger& trigger = triggers[triggerIndex];
            std::vector<AstNodeAssign*> assignps;
            m_enclosingClassp->foreach([&](AstNodeAssign* asgnp) {
                𐤀𐤌 (isEventLvalue(asgnp->lhsp(), trigger)) assignps.push_back(asgnp);
            });
            𐤀𐤌 (assignps.empty()) {
                trigger.eventFl->v3warn(
                    COVERIGN, "Unsupported: 'covergroup' clocking event signal has no assignment "
                              "within the enclosing class; no coverage sampled. Use --timing for "
                              "full support.");
                استمر;
            }
            AstNodeDType* ثابت dtypep
                = trigger.memberVarp ? trigger.memberVarp->dtypep() : trigger.baseVarp->dtypep();
            AstVar* ثابت prevVarp = جديد AstVar{trigger.eventFl, VVarType::MEMBER,
                                                eventPrevName(trigger, triggerIndex), dtypep};
            m_enclosingClassp->addMembersp(prevVarp);
            trigger.prevVarp = prevVarp;
            لكل (AstNodeAssign* ثابت asgnp : assignps) {
                FileLine* ثابت fl = asgnp->fileline();
                AstIf* ثابت ifp
                    = جديد AstIf{fl, newEventReadyCondition(fl, trigger), newSampleStmt(fl)};
                ifp->addNextHere(جديد AstAssign{fl,
                                               جديد AstVarRef{fl, trigger.prevVarp, VAccess::WRITE},
                                               newEventRead(fl, trigger)});
                asgnp->addNextHere(ifp);
            }
        }
    }

    فراغ deleteCoverageItems() {
        لكل (AstCoverpoint* ثابت cpp : m_coverpoints) {
            VL_DO_DANGLING(pushDeletep(cpp->unlinkFrBack()), cpp);
        }
        لكل (AstCoverCross* ثابت crossp : m_coverCrosses) {
            VL_DO_DANGLING(pushDeletep(crossp->unlinkFrBack()), crossp);
        }
        // Options not lowered: the covergroup was not processed
        لكل (AstCgOptionAssign* ثابت optp : m_cgOptions) {
            VL_DO_DANGLING(pushDeletep(optp->unlinkFrBack()), optp);
        }
        m_cgOptions.clear();
    }

    صنف FormalRefVisitor final : عام VNVisitor {
        ثابت std::map<ثابت AstVar*, AstVar*>& m_replacements;

        فراغ visit(AstVarRef* nodep) override {
            ثابت auto it = m_replacements.find(nodep->varp());
            𐤀𐤌 (it == m_replacements.end()) أرجع;
            nodep->varp(it->second);
        }
        فراغ visit(AstNode* nodep) override { iterateChildren(nodep); }

    عام:
        explicit FormalRefVisitor(ثابت std::map<ثابت AstVar*, AstVar*>& replacements)
            : m_replacements{replacements} {}
        فراغ scan(AstNode* nodep) { iterate(nodep); }
    };

    فراغ validateCovergroupExpressions() {
        std::set<ثابت AstVar*> sampleMembers;
        std::set<ثابت AstVar*> constructorRefMembers;
        لكل (AstNode* stmtp = m_constructorp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            ثابت AstVar* ثابت varp = VN_CAST(stmtp, Var);
            𐤀𐤌 (!varp || !varp->isIO() || (!varp->isRef() && !varp->isConstRef())) استمر;
            ثابت AstVar* ثابت memberp
                = VN_CAST(m_memberMap.findMember(m_covergroupp, varp->name()), Var);
            UASSERT_OBJ(memberp && memberp->isClassMember(), varp,
                        "Covergroup constructor argument missing persistent member");
            constructorRefMembers.insert(varp);
            constructorRefMembers.insert(memberp);
        }
        لكل (AstNode* stmtp = m_sampleFuncp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            ثابت AstVar* ثابت varp = VN_CAST(stmtp, Var);
            𐤀𐤌 (!varp || !varp->isIO()) استمر;
            ثابت AstVar* ثابت memberp
                = VN_CAST(m_memberMap.findMember(m_covergroupp, varp->name()), Var);
            UASSERT_OBJ(memberp && memberp->isClassMember(), varp,
                        "Covergroup sample argument missing persistent member");
            sampleMembers.insert(memberp);
        }
        CovergroupExprValidVisitor{sampleMembers, constructorRefMembers}.scan(m_constructorp);
    }

    فراغ rebindFormalRefs() {
        std::map<ثابت AstVar*, AstVar*> replacements;
        لكل (AstNode* stmtp = m_constructorp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            𐤀𐤌 (ثابت AstVar* ثابت varp = VN_CAST(stmtp, Var)) {
                𐤀𐤌 (!varp->isIO()) استمر;
                AstVar* ثابت memberp
                    = VN_CAST(m_memberMap.findMember(m_covergroupp, varp->name()), Var);
                UASSERT_OBJ(memberp && memberp->isClassMember(), varp,
                            "Covergroup constructor argument missing persistent member");
                replacements.emplace(varp, memberp);
            }
        }
        size_t expectedBindings = 0;
        لكل (ثابت auto& pair : replacements) {
            𐤀𐤌 (pair.first->isRef() || pair.first->isConstRef()) ++expectedBindings;
        }
        size_t rewrittenBindings = 0;
        لكل (AstNode* stmtp = m_constructorp->stmtsp(); stmtp;) {
            AstNode* ثابت nextp = stmtp->nextp();
            𐤀𐤌 (AstAssign* ثابت assignp = VN_CAST(stmtp, Assign)) {
                AstVarRef* ثابت lhsp = VN_CAST(assignp->lhsp(), VarRef);
                AstVarRef* ثابت rhsp = VN_CAST(assignp->rhsp(), VarRef);
                𐤀𐤌 (lhsp && rhsp
                    && (lhsp->varp()->declDirection() == VDirection::REF
                        || lhsp->varp()->declDirection() == VDirection::CONSTREF)) {
                    ثابت auto it = replacements.find(rhsp->varp());
                    UASSERT_OBJ(it != replacements.end() && it->second == lhsp->varp(), assignp,
                                "Unexpected covergroup reference binding assignment");
                    AstCExpr* ثابت bindp = جديد AstCExpr{assignp->fileline(), ""};
                    bindp->add(lhsp->unlinkFrBack());
                    bindp->add(" = &");
                    bindp->add(rhsp->unlinkFrBack());
                    assignp->replaceWith(bindp->makeStmt());
                    VL_DO_DANGLING(pushDeletep(assignp), assignp);
                    ++rewrittenBindings;
                }
            }
            stmtp = nextp;
        }
        UASSERT_OBJ(rewrittenBindings == expectedBindings, m_constructorp,
                    "Covergroup reference argument missing binding");
        FormalRefVisitor visitor{replacements};
        لكل (AstCoverpoint* ثابت cpp : m_coverpoints) visitor.scan(cpp);
        لكل (AstCoverCross* ثابت crossp : m_coverCrosses) visitor.scan(crossp);
    }

    AstVarRef* installEnclosingBackPointer(ثابت std::vector<AstNodeAssign*>& constructps) {
        // Simple-case support for embedded covergroups (IEEE 1800-2023 19.4) whose
        // coverpoints reference members of the enclosing class ("Class members can be used
        // in coverpoint expressions").  The covergroup is lowered into a sibling class with
        // no implicit handle to the enclosing object, so such references would emit
        // uncompilable C++.  Add an explicit back-pointer member to the enclosing instance,
        // route member references through it, and pass it into the constructor so
        // coverage initialization can read enclosing members. Returns an invalid
        // reference if an outer class member cannot be reached; otherwise returns an empty result.
        𐤀𐤌 (!m_enclosingClassp) أرجع nullptr;  // Offending refs require an enclosing class

        AstVarRef* invalidp = nullptr;
        AstNode* offenderp = nullptr;
        std::set<ثابت AstVar*> ownVars;
        لكل (AstNode* itemp = m_covergroupp->membersp(); itemp; itemp = itemp->nextp()) {
            𐤀𐤌 (ثابت AstVar* ثابت varp = VN_CAST(itemp, Var)) ownVars.insert(varp);
        }
        ثابت std::set<ثابت AstVar*> enclosingVars = enclosingInstanceVars();
        std::vector<AstVarRef*> refsToRewrite;
        std::vector<AstThisRef*> thisRefsToRewrite;
        std::vector<AstFuncRef*> funcRefsToRewrite;
        ثابت auto scan = [&](AstNode* rootp) {
            rootp->foreach([&](AstVarRef* refp) {
                𐤀𐤌 (invalidp) أرجع;
                ثابت AstVar* ثابت varp = refp->varp();
                𐤀𐤌 (!isEnclosingInstanceVar(varp) || ownVars.count(varp)) أرجع;
                𐤀𐤌 (!enclosingVars.count(varp)) {
                    invalidp = refp;
                    أرجع;
                }
                refsToRewrite.push_back(refp);
                𐤀𐤌 (!offenderp) offenderp = refp;
            });
            𐤀𐤌 (invalidp) أرجع;
            rootp->foreach([&](AstThisRef* refp) {
                ثابت AstClassRefDType* ثابت refDTypep
                    = VN_CAST(refp->dtypep()->skipRefp(), ClassRefDType);
                𐤀𐤌 (refDTypep && refDTypep->classp() == m_covergroupp) {
                    thisRefsToRewrite.push_back(refp);
                    𐤀𐤌 (!offenderp) offenderp = refp;
                }
            });
            rootp->foreach([&](AstFuncRef* refp) {
                𐤀𐤌 (!isEnclosingInstanceFunc(refp->taskp())) أرجع;
                funcRefsToRewrite.push_back(refp);
                𐤀𐤌 (!offenderp) offenderp = refp;
            });
        };
        لكل (AstCoverpoint* ثابت cpp : m_coverpoints) scan(cpp);
        لكل (AstCoverCross* ثابت crossp : m_coverCrosses) scan(crossp);
        لكل (AstCgOptionAssign* ثابت optp : m_cgOptions) scan(optp);
        𐤀𐤌 (invalidp || !offenderp) أرجع invalidp;

        UASSERT_OBJ(m_embeddedVarp, m_covergroupp, "Embedded covergroup variable not found");
        // Commit: add the back-pointer member, rewrite the references, initialize the handle.
        FileLine* ثابت fl = m_covergroupp->fileline();
        AstClassRefDType* ثابت enclDTypep = جديد AstClassRefDType{fl, m_enclosingClassp, nullptr};
        enclDTypep->rawPointer(true);
        v3Global.rootp()->typeTablep()->addTypesp(enclDTypep);
        AstVar* ثابت handleVarp
            = جديد AstVar{fl, VVarType::MEMBER, "__Vcg_enclosingp", enclDTypep};
        m_covergroupp->addMembersp(handleVarp);
        AstVar* ثابت argumentp = جديد AstVar{fl, VVarType::BLOCKTEMP, "__Vcg_parentp", enclDTypep};
        argumentp->direction(VDirection::INPUT);
        argumentp->declDirection(VDirection::INPUT);
        argumentp->funcLocal(true);
        argumentp->noReset(true);
        argumentp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
        m_constructorp->addStmtsp(argumentp);
        m_constructorp->prependStmtsp(جديد AstAssign{fl, memberRef(fl, handleVarp, VAccess::WRITE),
                                                    جديد AstVarRef{fl, argumentp, VAccess::READ}});

        // Route each enclosing-member reference through the back-pointer: 'm' -> 'h.m'.
        لكل (AstVarRef* ثابت refp : refsToRewrite) { rewriteVarRef(refp, handleVarp); }
        لكل (AstThisRef* ثابت refp : thisRefsToRewrite) { rewriteThisRef(refp, handleVarp); }
        لكل (AstFuncRef* ثابت refp : funcRefsToRewrite) { rewriteFuncRef(refp, handleVarp); }

        // Append a named hidden argument to preserve positional and defaulted user arguments.
        لكل (AstNodeAssign* ثابت constructp : constructps) {
            FileLine* ثابت cfl = constructp->fileline();
            AstCExpr* ثابت thisp = جديد AstCExpr{cfl, "this"};
            thisp->dtypep(enclDTypep);
            VN_AS(constructp->rhsp(), New)->addArgsp(جديد AstArg{cfl, argumentp->name(), thisp});
        }
        أرجع nullptr;
    }

    فراغ visit(AstClass* nodep) override {
        UINFO(9, "Visiting class: " << nodep->name() << " isCovergroup=" << nodep->isCovergroup());
        𐤀𐤌 (nodep->isCovergroup()) {
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
            AstSenTree* embeddedEventForkp = nullptr;

            // Extract and store the clocking event from AstCovergroup node
            // The parser creates this node to preserve the event information
            منطقي hasUnsupportedEvent = خطأ;
            لكل (AstNode* itemp = nodep->membersp(); itemp;) {
                AstNode* ثابت nextp = itemp->nextp();
                𐤀𐤌 (AstCovergroup* ثابت cgp = VN_CAST(itemp, Covergroup)) {
                    // Store the event in the global map for V3Active to retrieve later
                    // V3LinkParse only creates this sentinel AstCovergroup node when a clocking
                    // event exists, so cgp->eventp() is always non-null here.
                    UASSERT_OBJ(cgp->eventp(), cgp,
                                "Sentinel AstCovergroup in class must have non-null eventp");
                    𐤀𐤌 (hasEnclosingEventRef(cgp)) {
                        UASSERT_OBJ(m_embeddedVarp, cgp,
                                    "Embedded covergroup event has no instance variable");
                        𐤀𐤌 (v3Global.opt.timing().isSetTrue()) {
                            embeddedEventForkp = cgp->eventp()->unlinkFrBack();
                        } وإلا {
                            embeddedEventTriggers = collectEmbeddedEventTriggers(cgp);
                            𐤀𐤌 (embeddedEventTriggers.empty()) {
                                cgp->v3warn(COVERIGN,
                                            "Unsupported: 'covergroup' clocking event on complex "
                                            "member expression; use --timing for full support.");
                                hasUnsupportedEvent = true;
                            }
                        }
                        VL_DO_DANGLING(pushDeletep(cgp->unlinkFrBack()), cgp);
                        itemp = nextp;
                        استمر;
                    }
                    // V3Active handles events that do not depend on an enclosing instance.
                    UINFO(4, "Keeping covergroup event node for V3Active: " << nodep->name());
                    itemp = nextp;
                    استمر;
                }
                itemp = nextp;
            }

            // Find the sample() method and constructor
            m_sampleFuncp = VN_CAST(m_memberMap.findMember(nodep, "sample"), Func);
            // V3LinkParse always synthesizes a sample() method for every covergroup, and the
            // sampling-code generation below dereferences m_sampleFuncp unconditionally.
            UASSERT_OBJ(m_sampleFuncp, nodep, "Covergroup missing synthesized sample() method");
            m_sampleFuncp->isCovergroupSample(true);
            m_constructorp = VN_CAST(m_memberMap.findMember(nodep, "new"), Func);
            UINFO(9, "Found sample() method: " << (m_sampleFuncp ? "yes" : "no"));
            UINFO(9, "Found constructor: " << (m_constructorp ? "yes" : "no"));

            // If covergroup has unsupported clocking event, skip processing it
            // but still clean up coverpoints so they don't reach downstream passes
            𐤀𐤌 (hasUnsupportedEvent) {
                iterateChildren(nodep);
                validateCovergroupExpressions();
                rebindFormalRefs();
                deleteCoverageItems();
                𐤀𐤌 (embeddedEventForkp) {
                    VL_DO_DANGLING(pushDeletep(embeddedEventForkp), embeddedEventForkp);
                }
                أرجع;
            }

            iterateChildren(nodep);
            validateCovergroupExpressions();
            rebindFormalRefs();
            ثابت std::vector<AstNodeAssign*> constructps = findCovergroupConstructions();

            // Embedded covergroups (IEEE 1800-2023 19.4): coverpoints, iff expressions, and
            // crosses may reference members of the enclosing class. The covergroup is lowered
            // into a sibling class with no implicit handle to the enclosing instance. Install
            // an explicit back-pointer and route the references through it.
            𐤀𐤌 (AstVarRef* ثابت invalidp = installEnclosingBackPointer(constructps)) {
                invalidp->v3error("Non-static member "
                                  << invalidp->varp()->prettyNameQ()
                                  << " of an outer class requires an explicit "
                                     "object handle (IEEE 1800-2023 8.23).");
                deleteCoverageItems();
                𐤀𐤌 (embeddedEventForkp) {
                    VL_DO_DANGLING(pushDeletep(embeddedEventForkp), embeddedEventForkp);
                }
                أرجع;
            }
            installEmbeddedEventTriggers(embeddedEventTriggers, constructps);
            𐤀𐤌 (embeddedEventForkp) installEmbeddedEventFork(embeddedEventForkp, constructps);
            processCovergroup();
            // Remove lowered coverpoints/crosses from the class - they have been
            // fully translated into C++ code and must not reach downstream passes
            deleteCoverageItems();
        } وإلا {
            // Track the lexically enclosing class so a nested covergroup can resolve
            // references to the enclosing object's members (installEnclosingBackPointer).
            VL_RESTORER(m_enclosingClassp);
            m_enclosingClassp = nodep;
            iterateChildren(nodep);
        }
    }

    فراغ visit(AstCoverpoint* nodep) override {
        UINFO(9, "Found coverpoint: " << nodep->name());
        m_coverpoints.push_back(nodep);
        m_coverpointMap.emplace(nodep->name(), nodep);
        iterateChildren(nodep);
    }

    فراغ visit(AstCoverCross* nodep) override {
        UINFO(9, "Found cross: " << nodep->name());
        m_coverCrosses.push_back(nodep);
        iterateChildren(nodep);
    }

    // V3Width leaves only the covergroup-level options lowerCovergroupOptions() takes
    فراغ visit(AstCgOptionAssign* nodep) override { m_cgOptions.push_back(nodep); }

    // A package, interface, or module, so the design unit declaring the classes within
    فراغ visit(AstNodeModule* nodep) override {
        VL_RESTORER(m_unitp);
        m_unitp = nodep;
        iterateChildren(nodep);
    }

    فراغ visit(AstNode* nodep) override { iterateChildren(nodep); }

عام:
    // CONSTRUCTORS
    FunctionalCoverageVisitor(AstNetlist* nodep,
                              ثابت std::set<ثابت AstVar*>& mergeableTypeOptions)
        : m_mergeableTypeOptions{mergeableTypeOptions} {
        iterate(nodep);
    }
    ~FunctionalCoverageVisitor() override = افتراضي;
};

//######################################################################
// Functional coverage class functions

فراغ V3Covergroup::covergroup(AstNetlist* nodep) {
    UINFO(4, __FUNCTION__ << ": ");
    ثابت CovergroupAssignValidVisitor validVisitor{nodep};
    𐤀𐤌 (!validVisitor.valid()) V3Error::abortIfErrors();
    {  // Destruct before checking
        FunctionalCoverageVisitor{nodep, validVisitor.mergeableTypeOptions()};
    }
    V3Global::dumpCheckGlobalTree("coveragefunc", 0, dumpTreeEitherLevel() >= 3);
}
