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

#ଅନ୍ତର୍ଭାଵ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ଅନ୍ତର୍ଭାଵ "V3Covergroup.h"

#ଅନ୍ତର୍ଭାଵ "V3Const.h"
#ଅନ୍ତର୍ଭାଵ "V3Error.h"
#ଅନ୍ତର୍ଭାଵ "V3File.h"
#ଅନ୍ତର୍ଭାଵ "V3MemberMap.h"
#ଅନ୍ତର୍ଭାଵ "V3UniqueNames.h"

#ଅନ୍ତର୍ଭାଵ <bitset>
#ଅନ୍ତର୍ଭାଵ <cmath>
#ଅନ୍ତର୍ଭାଵ <deque>
#ଅନ୍ତର୍ଭାଵ <set>
#ଅନ୍ତର୍ଭାଵ <tuple>
#ଅନ୍ତର୍ଭାଵ <unordered_map>
#ଅନ୍ତର୍ଭାଵ <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Embedded covergroup assignment validation; and in the same pass over the netlist, the
// type_option variables through which SystemVerilog may set type_option.merge_instances

ଶ୍ରେଣୀ CovergroupAssignValidVisitor final : ସର୍ବସାଧାରଣ VNVisitorConst {
    VMemberMap m_memberMap;
    std::map<ଅଚଳ AstVar*, ଅଚଳ Ast_ଗଣ୍ଠି_FTask*>
        m_constructors;  // Implicit instance -> constructor
    std::set<ଅଚଳ AstVar*> m_mergeableTypeOptions;  // See mergeableTypeOptions()
    ଅଚଳ Ast_ଗଣ୍ଠି_FTask* m_ftaskp = nullptr;
    ତାର୍କିକ m_collecting = ସତ୍ୟ;
    ତାର୍କିକ m_valid = ସତ୍ୟ;

    ଶୂନ୍ୟ visit(AstClass* nodep) override {
        VL_RESTORER(m_ftaskp);
        m_ftaskp = nullptr;
        ଯଦି (m_collecting) {
            ଅଚଳ Ast_ଗଣ୍ଠି_FTask* ଅଚଳ constructorp
                = VN_CAST(m_memberMap.findMember(nodep, "new"), ଗଣ୍ଠି_FTask);
            ପାଇଁ (ଅଚଳ Ast_ଗଣ୍ଠି* itemp = nodep->membersp(); itemp; itemp = itemp->nextp()) {
                ଅଚଳ AstVar* ଅଚଳ varp = VN_CAST(itemp, Var);
                // Only the implicit instance is restricted, not explicitly typed aliases.
                ଯଦି (!varp || !varp->isClassMember() || varp->isDeclTyped()) ଜାରି;
                ଅଚଳ AstClassRefDType* ଅଚଳ refp
                    = VN_CAST(varp->dtypep()->skipRefp(), ClassRefDType);
                ଯଦି (refp && refp->classp()->covergroupEnclosingClassp() == nodep) {
                    m_constructors.emplace(varp, constructorp);
                }
            }
        }
        iterateChildrenConst(nodep);
    }
    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି_FTask* nodep) override {
        VL_RESTORER(m_ftaskp);
        m_ftaskp = nodep;
        iterateChildrenConst(nodep);
    }
    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି_Assign* nodep) override {
        ଯଦି (!m_collecting) {
            ଅଚଳ AstVar* varp = nullptr;
            ଯଦି (ଅଚଳ Ast_ଗଣ୍ଠି_Var_Ref* ଅଚଳ refp = VN_CAST(nodep->lhsp(), ଗଣ୍ଠି_Var_Ref)) {
                varp = refp->varp();
            } ଅନ୍ୟଥା ଯଦି (ଅଚଳ AstMemberSel* ଅଚଳ selp = VN_CAST(nodep->lhsp(), MemberSel)) {
                varp = selp->varp();
            }
            ଅଚଳ auto it = m_constructors.find(varp);
            ଯଦି (it != m_constructors.end() && (!m_ftaskp || m_ftaskp != it->second)) {
                m_valid = ମିଥ୍ୟା;
                nodep->v3error("Embedded covergroup variable "
                               << varp->prettyNameQ()
                               << " may only be assigned in the enclosing class's 'new' method "
                                  "(IEEE 1800-2023 19.4).");
            }
        }
        iterateChildrenConst(nodep);
    }
    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି_Var_Ref* nodep) override {
        // Type options may be set at any time (IEEE 1800-2023 19.7.1), so SystemVerilog may set
        // type_option.merge_instances with any use of a type_option but a read of another
        // member, as not every write is marked yet (std::randomize)
        ଯଦି (m_collecting && nodep->varp()->name() == "type_option") {
            ଅଚଳ AstStructSel* ଅଚଳ selp = VN_CAST(nodep->backp(), StructSel);
            ଯଦି (!selp || selp->fromp() != nodep || selp->name() == "merge_instances"
                || !nodep->access().isReadOnly()) {
                m_mergeableTypeOptions.ଭର୍ତ୍ତି(nodep->varp());
            }
        }
        iterateChildrenConst(nodep);
    }
    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି* nodep) override { iterateChildrenConst(nodep); }

ସର୍ବସାଧାରଣ:
    explicit CovergroupAssignValidVisitor(AstNetlist* nodep) {
        // Uses can precede their enclosing class in the tree.
        iterateConst(nodep);
        m_collecting = ମିଥ୍ୟା;
        ଯଦି (!m_constructors.empty()) iterateConst(nodep);
    }
    ତାର୍କିକ valid() ଅଚଳ { ଫେରନ୍ତୁ m_valid; }
    // type_option variables through which SystemVerilog may set type_option.merge_instances, so
    // that covergroups are tested with optionVar(true)
    ଅଚଳ std::set<ଅଚଳ AstVar*>& mergeableTypeOptions() ଅଚଳ { ଫେରନ୍ତୁ m_mergeableTypeOptions; }
};

//######################################################################
// Covergroup expression validation visitor

ଶ୍ରେଣୀ Covergroup_ପଦସମଷ୍ଟି_Valid_Visitor final : ସର୍ବସାଧାରଣ VNVisitor {
    ଅଚଳ std::set<ଅଚଳ AstVar*>& m_sampleMembers;
    ଅଚଳ std::set<ଅଚଳ AstVar*>& m_constructorRefMembers;
    ତାର୍କିକ m_inCoverageExpression = ମିଥ୍ୟା;
    ତାର୍କିକ m_sampleFormalAllowed = ମିଥ୍ୟା;

    ଶୂନ୍ୟ scanSampleExpression(Ast_ଗଣ୍ଠି* nodep) {
        ଯଦି (!nodep) ଫେରନ୍ତୁ;
        VL_RESTORER(m_sampleFormalAllowed);
        m_sampleFormalAllowed = ସତ୍ୟ;
        iterateAndNextNull(nodep);
    }

    ଶୂନ୍ୟ scanCoverageExpression(Ast_ଗଣ୍ଠି* nodep) {
        ଯଦି (!nodep) ଫେରନ୍ତୁ;
        VL_RESTORER(m_inCoverageExpression);
        m_inCoverageExpression = ସତ୍ୟ;
        iterateAndNextNull(nodep);
    }

    ଶୂନ୍ୟ visit(AstCoverpoint* nodep) override {
        scanSampleExpression(nodep->exprp());
        scanSampleExpression(nodep->iffp());
        iterateAndNextNull(nodep->binsp());
        iterateAndNextNull(nodep->optionsp());
    }
    ଶୂନ୍ୟ visit(AstCoverCross* nodep) override {
        iterateAndNextNull(nodep->itemsp());
        scanSampleExpression(nodep->iffp());
        iterateAndNextNull(nodep->optionsp());
        iterateAndNextNull(nodep->binsp());
    }
    ଶୂନ୍ୟ visit(AstCoverCrossBin* nodep) override {
        iterateAndNextNull(nodep->selectp());
        scanSampleExpression(nodep->iffp());
    }
    ଶୂନ୍ୟ visit(AstCoverCrossWith* nodep) override {
        iterateAndNextNull(nodep->selectp());
        scanCoverageExpression(nodep->filterp());
        scanCoverageExpression(nodep->countp());
    }
    ଶୂନ୍ୟ visit(AstCoverBinsof* nodep) override { scanCoverageExpression(nodep->rangesp()); }
    ଶୂନ୍ୟ visit(AstCoverBin* nodep) override {
        scanCoverageExpression(nodep->rangesp());
        scanSampleExpression(nodep->iffp());
        scanCoverageExpression(nodep->arraySizep());
        scanCoverageExpression(nodep->transp());
    }
    ଶୂନ୍ୟ visit(AstVarRef* nodep) override {
        ଯଦି (!m_sampleFormalAllowed && m_sampleMembers.ଗଣନା_2(nodep->varp())) {
            nodep->v3error("Covergroup sample formal argument "
                           << nodep->varp()->prettyNameQ()
                           << " may only be used in a coverpoint or conditional guard "
                              "expression (IEEE 1800-2023 19.8.1).");
        }
        ଯଦି (m_inCoverageExpression && m_constructorRefMembers.ଗଣନା_2(nodep->varp())) {
            nodep->v3error("Ref covergroup constructor formal argument "
                           << nodep->varp()->prettyNameQ()
                           << " may not be used in a covergroup expression "
                              "(IEEE 1800-2023 19.5).");
        }
    }
    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି_FTask_Ref* nodep) override {
        ଯଦି (m_inCoverageExpression && nodep->taskp()) {
            ତାର୍କିକ invalidDirection = ମିଥ୍ୟା;
            ପାଇଁ (Ast_ଗଣ୍ଠି* stmtp = nodep->taskp()->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
                ଅଚଳ AstVar* ଅଚଳ varp = VN_CAST(stmtp, Var);
                ଯଦି (varp && varp->isIO() && varp->isWritable()) {
                    invalidDirection = ସତ୍ୟ;
                    ଭାଙ୍ଗ;
                }
            }
            ଯଦି (invalidDirection) {
                nodep->v3error("Function " << nodep->taskp()->prettyNameQ()
                                           << " called in a covergroup expression has an "
                                              "output, inout, or non-const ref argument "
                                              "(IEEE 1800-2023 19.5).");
            }
        }
        iterateChildren(nodep);
    }
    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି* nodep) override { iterateChildren(nodep); }

ସର୍ବସାଧାରଣ:
    Covergroup_ପଦସମଷ୍ଟି_Valid_Visitor(ଅଚଳ std::set<ଅଚଳ AstVar*>& sampleMembers,
                               ଅଚଳ std::set<ଅଚଳ AstVar*>& constructorRefMembers)
        : m_sampleMembers{sampleMembers}
        , m_constructorRefMembers{constructorRefMembers} {}
    ଶୂନ୍ୟ scan(Ast_ଗଣ୍ଠି* nodep) { iterate(nodep); }
};

//######################################################################
// Bins of one declaration whose values are computed rather than listed: bin k covers
// [m_lo + k * m_stride, m_lo + (k + 1) * m_stride - 1], and the last bin extends to m_hi.  An
// array bin element is a run of single-value bins; automatic bins partition the coverpoint
// domain.  Bounds are coverpoint values at FunctionalCoverageVisitor::runWidth(),
// sign-extended like a CrossValueRange's.

ଶ୍ରେଣୀ BinRun final {
ସର୍ବସାଧାରଣ:
    // MEMBERS
    uint32_t m_ଗଣନା;  // Number of bins
    V3_ସଂଖ୍ୟା m_lo;  // Lowest value of the first bin
    V3_ସଂଖ୍ୟା m_stride;  // Number of values of each bin but the last
    V3_ସଂଖ୍ୟା m_hi;  // Highest value of the last bin
    ତାର୍କିକ m_empty = ମିଥ୍ୟା;  // A single bin without a value of the coverpoint type
    uint32_t m_declared = 0;  // Runtime index of the first bin, once generated

    // CONSTRUCTORS
    BinRun(Ast_ଗଣ୍ଠି* nodep, ପୂର୍ଣ୍ଣସଂଖ୍ୟା width, uint32_t ଗଣନା_2)
        : m_ଗଣନା{ଗଣନା_2}
        , m_lo{nodep, width}
        , m_stride{nodep, width, 1}
        , m_hi{nodep, width} {}
};

//######################################################################
// Functional coverage visitor

ଶ୍ରେଣୀ FunctionalCoverageVisitor final : ସର୍ବସାଧାରଣ VNVisitor {
    // NODE STATE
    // Entire netlist:
    //  AstCoverpoint::user1p()  -> AstVar*.  Previous-value variable for transition bins
    //  AstCoverpoint::user2()   -> bool.  Had a bins declaration ignored, so no automatic bins
    ଅଚଳ VNUser1InUse m_inuser1;
    ଅଚଳ VNUser2InUse m_inuser2;

    // STATE
    std::set<AstCoverpoint*>
        m_runtimePoints;  // Points needing value metadata and live-bin mapping
    std::set<AstCoverCross*> m_runtimeCrosses;  // Crosses over finalized live-bin dimensions
    std::map<AstVar*, AstVar*> m_excludedVars;  // Sample-time state-exclusion flags
    AstClass* m_covergroupp = nullptr;  // Current covergroup being processed
    Ast_ଗଣ୍ଠି_Module* m_unitp = nullptr;  // Design unit declaring the current class, outside classes
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
    ତାର୍କିକ m_cgMergeInstances = ମିଥ୍ୟା;  // The covergroup's type_option.merge_instances, a constant
    // The covergroup's type_option.merge_instances is, or SystemVerilog may make it, true
    ତାର୍କିକ m_cgMayMerge = ମିଥ୍ୟା;
    // type_option members through which SystemVerilog may set type_option.merge_instances,
    // see CovergroupAssignValidVisitor::mergeableTypeOptions()
    ଅଚଳ std::set<ଅଚଳ AstVar*>& m_mergeableTypeOptions;
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା m_cgAtLeast = 1;  // Covergroup option.at_least

    ସଂରଚନା EmbeddedEventTrigger final {
        File_ଧାଡ଼ି* eventFl;  // Clocking-event source location
        AstVar* baseVarp;  // Base enclosing-class member in the event expression
        AstVar* memberVarp;  // Selected member in a 'base.member' expression, or nullptr
        VEdgeType edgeType;  // Clocking-event edge qualifier
        AstVar* prevVarp;  // Member containing the previous event value, or nullptr
        EmbeddedEventTrigger(File_ଧାଡ଼ି* eventFl, AstVar* baseVarp, AstVar* memberVarp,
                             VEdgeType edgeType)
            : eventFl{eventFl}
            , baseVarp{baseVarp}
            , memberVarp{memberVarp}
            , edgeType{edgeType}
            , prevVarp{nullptr} {}
    };

    std::set<std::string> m_crossedCpNames;  // Coverpoints referenced by a cross
    std::map<std::string, AstVar*> m_cpVarMap;  // Coverpoint name -> its VlCoverpoint member
    ସଂରଚନା BinRuns final {
        std::vector<BinRun> runs;  // Runs of an array or automatic bins declaration, in order
        uint32_t ଗଣନା_2 = 0;  // Bins across all runs
        ତାର୍କିକ unsupported = ମିଥ୍ୟା;  // Too many bins, or invalid: the declaration is ignored
        // The value of each bin, which names it, of a wildcard array; else the bins are indexed
        std::vector<std::string> ମୂଲ୍ୟଗୁଡ଼ିକ;
        // Too many values of an ignore or illegal wildcard array, which is then one bin
        ତାର୍କିକ single = ମିଥ୍ୟା;
    };
    ସଂରଚନା Cross_Bin_ମୂଲ୍ୟଗୁଡ଼ିକ final {
        AstCoverBin* binp;  // Declaration owning this Normal bin
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* valuep;  // Individual array-bin value, or nullptr for a scalar bin
        ଅଚଳ BinRun* runp = nullptr;  // Run computing the bin's values, if any
        uint32_t element = 0;  // Index of the bin within runp
    };
    ସଂରଚନା BinSpan final {
        uint32_t first = 0;  // First Normal index of the bin declaration
        uint32_t ଗଣନା_2 = 0;  // Number of Normal bins of the declaration
        uint32_t declared = 0;  // First runtime bin index, across all bin kinds
        int32_t sized = -1;  // Index of the sized array, placed at construction; or -1
    };
    ସଂରଚନା CoverpointBins final {
        uint32_t total = 0;  // Number of Normal bins
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* exprp = nullptr;  // Sampled expression, for the value domain
        ତାର୍କିକ crossed = ମିଥ୍ୟା;  // Feeds a cross, which needs 'values'
        std::vector<Cross_Bin_ମୂଲ୍ୟଗୁଡ଼ିକ> ମୂଲ୍ୟଗୁଡ଼ିକ;  // Values in runtime Normal-bin index order
        std::unordered_map<std::string, BinSpan> spans;  // Declared bin name -> index span
        BinSpan implicitAuto{0, 0, 0};  // Implicit automatic bins, each named 'auto_<i>'
        std::deque<BinRun> runs;  // Runs 'values' refers to
    };
    std::map<AstVar*, CoverpointBins> m_cpBins;  // Runtime coverpoint -> binsof index ranges
    // Names of the bins declarations each coverpoint ignored, which binsof selects as no bins
    std::map<ଅଚଳ AstCoverpoint*, std::vector<std::string>> m_droppedBins;
    // Prefixes of the constructor temporaries of constructed bins, as coverpoint and bin names
    // alone may repeat: coverpoint 'a_' bins 'b', and coverpoint 'a' bins '_b'
    V3UniqueNames m_sizedNames{"__Vsized"};
    std::vector<Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି*> m_detached_ମୂଲ୍ୟଗୁଡ଼ିକ;  // Array-bin values m_cpBins refers to
    std::set<AstCoverCross*>
        m_droppedCrosses;  // Crosses with a bare-variable item: drop (COVERIGN)
    std::map<uint32_t, AstCoverpointDType*> m_cpDTypes;  // Hit-list bound -> interned dtype
    using CrossShape = std::tuple<uint32_t, uint32_t, uint32_t, uint32_t, uint64_t, ତାର୍କିକ>;
    std::map<CrossShape, AstCoverCrossDType*> m_cxDTypes;
    AstVar* m_cgInstVarp = nullptr;  // __Vcg_inst handle member of the current covergroup

    VMemberMap m_memberMap;  // Member names cached for fast lookup

    // METHODS
    // The covergroup's 'option' or static 'type_option' member (V3LinkParse creates both)
    AstVar* optionVar(ତାର୍କିକ typeOption) {
        AstVar* ଅଚଳ varp = VN_AS(
            m_memberMap.findMember(m_covergroupp, typeOption ? "type_option" : "option"), Var);
        UASSERT_OBJ(varp, m_covergroupp, "Covergroup missing option member");
        ଫେରନ୍ତୁ varp;
    }

    // 'option.<member>' or 'type_option.<member>', per optionVarp
    AstStructSel* newOptionSel(File_ଧାଡ଼ି* fl, AstVar* optionVarp, ଅଚଳ std::string& member,
                               VAccess access) {
        ଅଚଳ AstMemberDType* ଅଚଳ memberp
            = VN_AS(m_memberMap.findMember(optionVarp->dtypep()->skipRefp(), member), MemberDType);
        UASSERT_OBJ(memberp, optionVarp, "Coverage option structure missing '" << member << "'");
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ fromp = optionVarp->lifetime().isStatic()
                                       ? ନୂତନ AstVarRef{fl, optionVarp, access}
                                       : memberRef(fl, optionVarp, access);
        AstStructSel* ଅଚଳ selp = ନୂତନ AstStructSel{fl, fromp, member};
        selp->dtypep(memberp->subDTypep()->skipRefToEnump());
        selp->didWidth(ସତ୍ୟ);
        ଫେରନ୍ତୁ selp;
    }

    // A constant option.at_least; a negative count of hits has no meaning, so takes the default
    ସ୍ଥିର ପୂର୍ଣ୍ଣସଂଖ୍ୟା constAtLeast(ଅଚଳ AstConst* constp) {
        ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା ମୂଲ୍ୟ = constp->toSInt();
        ଯଦି (ମୂଲ୍ୟ >= 0) ଫେରନ୍ତୁ ମୂଲ୍ୟ;
        constp->v3warn(COVERIGN, "Ignoring negative 'option.at_least'; using 1");
        ଫେରନ୍ତୁ 1;
    }

    // An option's constant value, or nullptr after a warning, as non-constant is unsupported
    ସ୍ଥିର ଅଚଳ AstConst* constOption(ଅଚଳ Ast_ଗଣ୍ଠି* valuep, VCoverOptionType optType) {
        ଯଦି (ଅଚଳ AstConst* ଅଚଳ constp = VN_CAST(valuep, Const)) ଫେରନ୍ତୁ constp;
        valuep->v3warn(COVERIGN, "Ignoring unsupported: non-constant 'option."
                                     << optType.ascii() << "'; using default value");
        ଫେରନ୍ତୁ nullptr;
    }

    // Store the covergroup-level options (IEEE 1800-2023 19.7) where SystemVerilog and the
    // runtime read them.  The instance options, option.weight and option.get_inst_coverage,
    // are evaluated by the constructor, as are the other instance options; the type options,
    // type_option.weight and type_option.merge_instances, are constant, and initialize the
    // static member.  Without its own type_option.merge_instances, a covergroup has the
    // default of --coverage-merge-instances.  A constant option.at_least is the default of
    // the coverpoints and crosses.
    ଶୂନ୍ୟ lowerCovergroupOptions() {
        ତାର୍କିକ mergeSet = ମିଥ୍ୟା;  // The covergroup sets type_option.merge_instances
        m_cgAtLeast = 1;
        ପାଇଁ (AstCgOptionAssign* ଅଚଳ optp : m_cgOptions) {
            ଯଦି (optp->optType() == VCoverOptionType::AT_LEAST) {
                ଯଦି (ଅଚଳ AstConst* ଅଚଳ constp = constOption(optp->valuep(), optp->optType())) {
                    m_cgAtLeast = constAtLeast(constp);
                }
                VL_DO_DANGLING(ଠେଲ_Deletep(optp->unlinkFrBack()), optp);
                ଜାରି;
            }
            File_ଧାଡ଼ି* ଅଚଳ fl = optp->fileline();
            // V3Width left the type options constant, and type_option.weight non-negative
            ଅଚଳ AstConst* ଅଚଳ constp = VN_CAST(optp->valuep(), Const);
            std::string member = "weight";
            ଯଦି (optp->optType() == VCoverOptionType::MERGE_INSTANCES) {
                member = "merge_instances";
                m_cgMergeInstances = !constp->ଅଙ୍କ().isEqZero();
                mergeSet = ସତ୍ୟ;
            } ଅନ୍ୟଥା ଯଦି (optp->optType() == VCoverOptionType::GET_INST_COVERAGE) {
                member = "get_inst_coverage";
            } ଅନ୍ୟଥା {
                UASSERT_OBJ(optp->optType() == VCoverOptionType::WEIGHT, optp,
                            "Unexpected covergroup option reaching V3Covergroup");
                ଯଦି (optp->typeOption()) m_cgTypeWeight = constp->toUInt();
            }
            AstAssign* ଅଚଳ assignp = ନୂତନ AstAssign{
                fl, newOptionSel(fl, optionVar(optp->typeOption()), member, VAccess::WRITE),
                optp->valuep()->unlinkFrBack()};
            ଯଦି (optp->typeOption()) {
                m_covergroupp->addMembersp(ନୂତନ AstInitialStatic{fl, assignp});
                VL_DO_DANGLING(ଠେଲ_Deletep(optp->unlinkFrBack()), optp);
            } ଅନ୍ୟଥା {
                optp->replaceWith(assignp);
                VL_DO_DANGLING(ଠେଲ_Deletep(optp), optp);
            }
        }
        m_cgOptions.clear();
        ଯଦି (!mergeSet && v3Global.opt.coverageMergeInstances()) {
            File_ଧାଡ଼ି* ଅଚଳ fl = m_covergroupp->fileline();
            m_covergroupp->addMembersp(ନୂତନ AstInitialStatic{
                fl, ନୂତନ AstAssign{
                        fl, newOptionSel(fl, optionVar(ସତ୍ୟ), "merge_instances", VAccess::WRITE),
                        ନୂତନ AstConst{fl, AstConst::BitTrue{}}}});
            m_cgMergeInstances = ସତ୍ୟ;
        }
        m_cgMayMerge = m_cgMergeInstances || m_mergeableTypeOptions.ଗଣନା_2(optionVar(ସତ୍ୟ));
    }

    // The weight of an item in the coverage database, which merges the instances: its
    // type_option.weight if the covergroup merges them too (IEEE 1800-2023 19.11.3); else its
    // option.weight if a constant, and so of every instance; else its type_option.weight
    uint32_t itemDatabaseWeight(Ast_ଗଣ୍ଠି* optionsp) ଅଚଳ {
        ଅଚଳ Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* weightp = nullptr;  // The option.weight in effect
        uint32_t typeWeight = 1;
        ପାଇଁ (Ast_ଗଣ୍ଠି* nodep = optionsp; nodep; nodep = nodep->nextp()) {
            ଅଚଳ AstCoverOption* ଅଚଳ optp = VN_AS(nodep, CoverOption);
            ଯଦି (!(optp->optType() == VCoverOptionType::WEIGHT)) ଜାରି;
            // V3Width left type_option.weight a non-negative constant
            ଯଦି (optp->typeOption()) {
                typeWeight = VN_AS(optp->valuep(), Const)->toUInt();
            } ଅନ୍ୟଥା {
                weightp = optp->valuep();
            }
        }
        ଯଦି (m_cgMergeInstances) ଫେରନ୍ତୁ typeWeight;
        ଯଦି (!weightp) ଫେରନ୍ତୁ 1;
        ଯଦି (ଅଚଳ AstConst* ଅଚଳ constp = VN_CAST(weightp, Const)) ଫେରନ୍ତୁ constp->toUInt();
        ଫେରନ୍ତୁ typeWeight;
    }

    // Configure an item's option.weight, its weight in instance coverage (IEEE 1800-2023
    // 19.11), and, if the covergroup may merge its instances, its type_option.weight, its
    // weight in type coverage then (19.11.3).
    ଶୂନ୍ୟ generateItemWeight(File_ଧାଡ଼ି* fl, AstVar* itemVarp, Ast_ଗଣ୍ଠି* optionsp) {
        ପାଇଁ (Ast_ଗଣ୍ଠି* nodep = optionsp; nodep; nodep = nodep->nextp()) {
            ଅଚଳ AstCoverOption* ଅଚଳ optp = VN_AS(nodep, CoverOption);
            ଯଦି (!(optp->optType() == VCoverOptionType::WEIGHT)) ଜାରି;
            ଯଦି (!optp->typeOption()) {
                m_constructorp->addStmtsp(
                    itemCall(fl, itemVarp, VCMethod::COVERGROUP_WEIGHT,
                             {optp->valuep()->clone_ବୃକ୍ଷ(ମିଥ୍ୟା), file_ଧାଡ଼ି_Debug(optp->fileline())})
                        ->makeStmt());
            } ଅନ୍ୟଥା ଯଦି (m_cgMayMerge) {
                // V3Width left type_option.weight a non-negative constant
                m_constructorp->addStmtsp(itemCall(fl, itemVarp, VCMethod::COVERGROUP_TYPE_WEIGHT,
                                                   {optp->valuep()->clone_ବୃକ୍ଷ(ମିଥ୍ୟା)})
                                              ->makeStmt());
            }
        }
    }

    ଶୂନ୍ୟ processCovergroup() {
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
        m_cgMergeInstances = ମିଥ୍ୟା;
        m_cgMayMerge = ମିଥ୍ୟା;

        lowerCovergroupOptions();

        // Scan every cross item to record the coverpoints it references (the cross dimensions)
        // and to flag any cross naming a bare variable -- a would-be implicit coverpoint, which
        // Verilator does not synthesize.  An unresolvable item drops only that one cross (with a
        // COVERIGN in generateCrossCode), leaving the rest of the covergroup intact.
        ପାଇଁ (AstCoverCross* crossp : m_coverCrosses) {
            ପାଇଁ (Ast_ଗଣ୍ଠି* itemp = crossp->itemsp(); itemp; itemp = itemp->nextp()) {
                ଅଚଳ AstCoverpointRef* ଅଚଳ refp = VN_AS(itemp, CoverpointRef);
                ଯଦି (refp->exprp()) ଜାରି;  // hierarchical ref: dropped in generateCrossCode
                ଯଦି (m_coverpointMap.find(refp->name()) == m_coverpointMap.end()) {
                    m_droppedCrosses.ଭର୍ତ୍ତି(crossp);  // bare variable: drop this cross only
                } ଅନ୍ୟଥା {
                    m_crossedCpNames.ଭର୍ତ୍ତି(refp->name());
                }
            }
        }

        std::vector<Ast_ଗଣ୍ଠି*> pending;
        std::map<AstCoverpoint*, std::vector<AstCoverCross*>> consumers;
        std::map<AstCoverCross*, std::vector<AstCoverpoint*>> inputs;
        ପାଇଁ (AstCoverpoint* ଅଚଳ cpp : m_coverpoints) {
            checkBinNames(cpp);
            checkConstructedBins(cpp);
            ଯଦି (!cpp->exprp()->dtypep()->skipRefp()->isIntegralOrPacked()) ଜାରି;
            // Bins without values leave the report (IEEE 1800-2023 19.11.1), exclusions or not.
            // Constructed bins get their values when the covergroup is constructed.
            ଯଦି (!coverpointHasStateExclusions(cpp) && !coverpointHasEmptyBins(cpp)
                && !coverpointHasConstructedBins(cpp)) {
                ଜାରି;
            }
            m_runtimePoints.ଭର୍ତ୍ତି(cpp);
            pending.ଠେଲ_back(cpp);
        }
        ପାଇଁ (AstCoverCross* ଅଚଳ crossp : m_coverCrosses) {
            ଯଦି (m_droppedCrosses.ଗଣନା_2(crossp)) ଜାରି;
            ପାଇଁ (Ast_ଗଣ୍ଠି* itemp = crossp->itemsp(); itemp; itemp = itemp->nextp()) {
                ଅଚଳ AstCoverpointRef* ଅଚଳ refp = VN_AS(itemp, CoverpointRef);
                ଯଦି (refp->exprp()) ଜାରି;
                ଅଚଳ auto point = m_coverpointMap.find(refp->name());
                ଯଦି (point != m_coverpointMap.end()) {
                    consumers[point->second].ଠେଲ_back(crossp);
                    inputs[crossp].ଠେଲ_back(point->second);
                }
            }
            // The values of a filter's bin tuples are known when the covergroup is constructed
            ଯଦି (crossp->exists([](ଅଚଳ AstCoverCrossWith*) { ଫେରନ୍ତୁ ସତ୍ୟ; })) {
                m_runtimeCrosses.emplace(crossp);
                pending.ଠେଲ_back(crossp);
            }
        }
        ପାଇଁ (size_t next = 0; next < pending.size(); ++next) {
            ଯଦି (AstCoverpoint* ଅଚଳ pointp = VN_CAST(pending[next], Coverpoint)) {
                ପାଇଁ (AstCoverCross* ଅଚଳ crossp : consumers[pointp]) {
                    ଯଦି (m_runtimeCrosses.emplace(crossp).second) pending.ଠେଲ_back(crossp);
                }
            } ଅନ୍ୟଥା {
                ପାଇଁ (AstCoverpoint* ଅଚଳ pointp : inputs[VN_AS(pending[next], CoverCross)]) {
                    ଯଦି (pointp->exprp()->dtypep()->skipRefp()->isIntegralOrPacked()
                        && m_runtimePoints.emplace(pointp).second) {
                        pending.ଠେଲ_back(pointp);
                    }
                }
            }
        }

        // The instance node owns this instance's coverpoint/cross runtimes, so it must exist
        // before any of them is created.  Emitted first, ahead of both generate loops.
        generateInstanceAttach();

        // For each coverpoint, generate sampling code
        ପାଇଁ (AstCoverpoint* cpp : m_coverpoints) generateCoverpointCode(cpp);

        // For each cross, generate sampling code
        ପାଇଁ (AstCoverCross* crossp : m_coverCrosses) generateCrossCode(crossp);

        // Every cross has been built, so runtime points only need their exclusions from here.
        ପାଇଁ (AstCoverpoint* ଅଚଳ cpp : m_coverpoints) {
            ଯଦି (!m_runtimePoints.ଗଣନା_2(cpp)) ଜାରି;
            m_constructorp->addStmtsp(itemCall(cpp->fileline(), m_cpVarMap.at(cpp->name()),
                                               VCMethod::COVERGROUP_ମୂଲ୍ୟ_RELEASE)
                                          ->makeStmt());
        }
        ପାଇଁ (Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* valuep : m_detached_ମୂଲ୍ୟଗୁଡ଼ିକ) VL_DO_DANGLING(ଠେଲ_Deletep(valuep), valuep);
        m_detached_ମୂଲ୍ୟଗୁଡ଼ିକ.clear();

        // Generate coverage computation code (even for empty covergroups).  Bin registration
        // with the coverage database is handled per coverpoint/cross by their runtime
        // registerBins() calls (emitted in generateCoverpoint/generateCross).
        generateCoverageComputationCode();
    }

    ସ୍ଥିର constexpr size_t ମୂଲ୍ୟ_LIST_ENTRIES = 256;  // Metadata entries per constructor call

    // The value of a constant count, of bins or value tuples: -1 if it is negative, and
    // saturated above the largest limit
    ସ୍ଥିର int64_t const_ଗଣନା(ଅଚଳ AstConst* constp) {
        ଅଚଳ V3_ସଂଖ୍ୟା& ଅଙ୍କ = constp->ଅଙ୍କ();
        ଯଦି (constp->isSigned() && ଅଙ୍କ.isNegative()) ଫେରନ୍ତୁ -1;
        ଫେରନ୍ତୁ ଅଙ୍କ.mostSetBitP1() > 32 ? INT64_MAX : static_cast<int64_t>(ଅଙ୍କ.toUQuad());
    }

    // Whether a count, 'whatp', is integral and, if constant, positive, as the standard's clause
    // 'clausep' requires; else errors
    ସ୍ଥିର ତାର୍କିକ check_ଗଣନା(ଅଚଳ Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* countp, ଅଚଳ ଅକ୍ଷର* whatp, ଅଚଳ ଅକ୍ଷର* clausep) {
        ଅଚଳ AstConst* ଅଚଳ constp = VN_CAST(countp, Const);
        ଯଦି (!countp->dtypep()->skipRefp()->isIntegralOrPacked()) {
            countp->v3error(whatp << " must be integral (IEEE 1800-2023 " << clausep << ")");
            ଫେରନ୍ତୁ ମିଥ୍ୟା;
        } ଅନ୍ୟଥା ଯଦି (constp && (constp->ଅଙ୍କ().isFourState() || const_ଗଣନା(constp) < 1)) {
            countp->v3error(whatp << " must be >= 1, got "
                                  << (constp->ଅଙ୍କ().isFourState() ? constp->ଅଙ୍କ().ascii(ମିଥ୍ୟା)
                                                                  : constp->ଅଙ୍କ().toDecimalS())
                                  << " (IEEE 1800-2023 " << clausep << ")");
            ଫେରନ୍ତୁ ମିଥ୍ୟା;
        }
        ଫେରନ୍ତୁ ସତ୍ୟ;
    }

    // The number of bins requested by a valid 'bins auto[N]', or 0
    ସ୍ଥିର uint32_t autoBinsRequested(ଅଚଳ AstCoverBin* binp) {
        ଅଚଳ AstConst* ଅଚଳ constp = VN_CAST(binp->arraySizep(), Const);
        ଯଦି (!constp) ଫେରନ୍ତୁ 0;
        ଅଚଳ int64_t ଗଣନା_2 = const_ଗଣନା(constp);
        ଫେରନ୍ତୁ ଗଣନା_2 < 1 || ଗଣନା_2 > v3Global.opt.coverageMaxBins() ? 0
                                                                   : static_cast<uint32_t>(ଗଣନା_2);
    }

    // True for a 'bins auto[N]' declaration, or the implicit automatic bins of a coverpoint
    ସ୍ଥିର ତାର୍କିକ isAutoBins(ଅଚଳ AstCoverBin* binp) {
        ଫେରନ୍ତୁ binp->binsType() == VCoverBinsType::BINS_AUTO
               || binp->binsType() == VCoverBinsType::BINS_AUTO_IMPLICIT;
    }

    // True for a sized array of bins, 'bins b[N] = {...}', whose values an integral coverpoint
    // distributes over N bins when the covergroup is constructed (IEEE 1800-2023 19.5.1)
    ସ୍ଥିର ତାର୍କିକ isSizedArray(ଅଚଳ AstCoverBin* binp) {
        ଫେରନ୍ତୁ binp->arraySizep() && !isAutoBins(binp);
    }

    // The 'with' filter of a bin, if any (IEEE 1800-2023 19.5.1.1)
    ସ୍ଥିର AstCoverWith* binWith(ଅଚଳ AstCoverBin* binp) {
        ଫେରନ୍ତୁ VN_CAST(binp->rangesp(), CoverWith);
    }

    // True for bins whose values the covergroup constructor computes: a sized array, or bins of
    // the values a 'with' filter keeps
    ସ୍ଥିର ତାର୍କିକ isConstructedBins(ଅଚଳ AstCoverBin* binp) {
        ଫେରନ୍ତୁ isSizedArray(binp) || binWith(binp);
    }

    // The range list of a bin's values, or of the candidates of its 'with' filter; null for all
    // of the coverpoint's values, which a filter of the coverpoint's name has
    ସ୍ଥିର Ast_ଗଣ୍ଠି* binRangesp(ଅଚଳ AstCoverBin* binp) {
        ଅଚଳ AstCoverWith* ଅଚଳ withp = binWith(binp);
        ଯଦି (!withp) ଫେରନ୍ତୁ binp->rangesp();
        ଫେରନ୍ତୁ VN_IS(withp->subp(), CoverpointRef) ? nullptr : withp->subp();
    }

    // Report and delete a bins declaration of a name that another of its coverpoint has
    ଶୂନ୍ୟ checkBinNames(AstCoverpoint* coverpointp) {
        std::set<std::string> names;
        ପାଇଁ (Ast_ଗଣ୍ଠି* nodep = coverpointp->binsp(); nodep;) {
            AstCoverBin* ଅଚଳ binp = VN_AS(nodep, CoverBin);
            nodep = nodep->nextp();
            ଯଦି (names.emplace(binp->name()).second) ଜାରି;
            binp->v3error("Duplicate bin " << binp->prettyNameQ() << " in coverpoint "
                                           << coverpointp->prettyNameQ()
                                           << " (IEEE 1800-2023 3.13)");
            VL_DO_DANGLING(ଠେଲ_Deletep(binp->unlinkFrBack()), binp);
        }
    }

    // Delete an ignored bins declaration, which binsof then selects as no bins
    ଶୂନ୍ୟ dropBins(ଅଚଳ AstCoverpoint* coverpointp, AstCoverBin* binp) {
        m_droppedBins[coverpointp].ଠେଲ_back(binp->name());
        VL_DO_DANGLING(ଠେଲ_Deletep(binp->unlinkFrBack()), binp);
    }

    // Check the size of a sized array of bins, which drops an invalid array.  A real coverpoint's
    // are unsupported, and treated as arrays of a bin per value.  False unless it stays sized.
    ତାର୍କିକ checkBinsArraySize(ଅଚଳ AstCoverpoint* coverpointp, AstCoverBin* binp, ତାର୍କିକ integral) {
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ sizep = binp->arraySizep();
        ଯଦି (VN_IS(sizep, Unbounded)) {  // A parameter of '$'; see bins_orBraE
            binp->v3error("Bins array size must be integral, not '$' (IEEE 1800-2023 19.5.1)");
        } ଅନ୍ୟଥା ଯଦି (check_ଗଣନା(sizep, "Bins array size", "19.5.1")) {
            ଯଦି (integral) ଫେରନ୍ତୁ ସତ୍ୟ;
            binp->v3warn(COVERIGN, "Unsupported: 'bins' explicit array size of a real "
                                   "coverpoint (treated as '[]')");
            VL_DO_DANGLING(ଠେଲ_Deletep(sizep->unlinkFrBack()), sizep);
            ଫେରନ୍ତୁ ମିଥ୍ୟା;
        }
        dropBins(coverpointp, binp);
        ଫେରନ୍ତୁ ମିଥ୍ୟା;
    }

    // Check the bins of a coverpoint whose values the constructor computes, dropping invalid
    // ones.  A wildcard array of too many ranges of values is ignored, or if ignore or illegal,
    // treated as one bin; filtering too many ranges of values ignores the bins.
    ଶୂନ୍ୟ checkConstructedBins(AstCoverpoint* coverpointp) {
        ଅଚଳ ତାର୍କିକ integral = coverpointp->exprp()->dtypep()->skipRefp()->isIntegralOrPacked();
        ପାଇଁ (Ast_ଗଣ୍ଠି* nodep = coverpointp->binsp(); nodep;) {
            AstCoverBin* ଅଚଳ binp = VN_AS(nodep, CoverBin);
            nodep = nodep->nextp();
            ଯଦି (!isConstructedBins(binp)) ଜାରି;
            ଯଦି (isSizedArray(binp) && !checkBinsArraySize(coverpointp, binp, integral)) ଜାରି;
            ଯଦି (!binp->isWildcard()
                || sizedWildcardRuns(binp, coverpointp->exprp())
                       <= v3Global.opt.coverageMaxBins()) {
                ଯଦି (binWith(binp) && withCandidatesOver(binp, coverpointp->exprp())) {
                    binp->v3warn(COVERIGN, "Unsupported: 'with' filter of more than 2**32 "
                                           "candidate values; bin "
                                               << binp->prettyNameQ() << " ignored");
                    ଯଦି (binp->binsType().binIsNormal()) coverpointp->user2(ସତ୍ୟ);
                    dropBins(coverpointp, binp);
                }
                ଜାରି;
            }
            // An ignore or illegal array still excludes or checks its values, as one bin
            ଅଚଳ ତାର୍କିକ single = !binp->binsType().binIsNormal() && !binWith(binp);
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
            ଯଦି (single) {
                Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ sizep = binp->arraySizep();
                VL_DO_DANGLING(ଠେଲ_Deletep(sizep->unlinkFrBack()), sizep);
                binp->isArray(ମିଥ୍ୟା);
                ଜାରି;
            }
            ଯଦି (binp->binsType().binIsNormal()) coverpointp->user2(ସତ୍ୟ);
            dropBins(coverpointp, binp);
        }
    }

    // True if a 'with' filter would be evaluated for more than 2**32 candidate values, known
    // now for the coverpoint's name, or for a range list of constants: each is evaluated once
    // but for an array 'b[N]', which keeps their order and duplicates (see withBegin())
    ସ୍ଥିର ତାର୍କିକ withCandidatesOver(AstCoverBin* binp, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* exprp) {
        ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା width = runWidth(exprp);
        std::vector<std::pair<V3_ସଂଖ୍ୟା, V3_ସଂଖ୍ୟା>> runs;
        ଯଦି (!binRangesp(binp)) runs = coverpoint_ମୂଲ୍ୟଗୁଡ଼ିକ(binp, exprp);
        ଅଚଳ auto constant
            = [](ଅଚଳ Ast_ଗଣ୍ଠି* nodep) { ଫେରନ୍ତୁ VN_IS(nodep, Const) || VN_IS(nodep, Unbounded); };
        ପାଇଁ (Ast_ଗଣ୍ଠି* rangep = binRangesp(binp); rangep; rangep = rangep->nextp()) {
            ଅଚଳ AstInsideRange* ଅଚଳ irp = VN_CAST(rangep, InsideRange);
            // Else known when constructed
            ଯଦି (irp ? !constant(irp->lhsp()) || !constant(irp->rhsp()) : !VN_IS(rangep, Const)) {
                ଫେରନ୍ତୁ ମିଥ୍ୟା;
            }
            Cross_ମୂଲ୍ୟ_Range range{rangep, resolveWidth(rangep, exprp)};
            ଯଦି (!resolve_ମୂଲ୍ୟ(rangep, exprp, ସତ୍ୟ, binp->isWildcard(), range)
                || crossRangeEmpty(range)) {
                ଜାରି;
            }
            std::vector<std::pair<V3_ସଂଖ୍ୟା, V3_ସଂଖ୍ୟା>> found{{range.lo, range.hi}};
            ଯଦି (range.wildcard) {  // checkConstructedBins bounded the runs
                found.clear();
                crossRangeRuns(range, v3Global.opt.coverageMaxBins(), found);
            }
            ପାଇଁ (ଅଚଳ std::pair<V3_ସଂଖ୍ୟା, V3_ସଂଖ୍ୟା>& run : found) {
                runs.emplace_back(V3_ସଂଖ୍ୟା{rangep, width, run.first},
                                  V3_ସଂଖ୍ୟା{rangep, width, run.second});
            }
        }
        ଯଦି (!isSizedArray(binp)) {  // The union of the values
            std::sort(runs.begin(), runs.end(), [](ଅଚଳ auto& lhs, ଅଚଳ auto& rhs) {
                ଫେରନ୍ତୁ cross_ମୂଲ୍ୟ_Less(lhs.first, rhs.first);
            });
            std::vector<std::pair<V3_ସଂଖ୍ୟା, V3_ସଂଖ୍ୟା>> merged;
            ପାଇଁ (ଅଚଳ std::pair<V3_ସଂଖ୍ୟା, V3_ସଂଖ୍ୟା>& run : runs) {
                ଯଦି (merged.empty() || cross_ମୂଲ୍ୟ_Less(merged.back().second, run.first)) {
                    merged.ଠେଲ_back(run);
                } ଅନ୍ୟଥା ଯଦି (cross_ମୂଲ୍ୟ_Less(merged.back().second, run.second)) {
                    merged.back().second = run.second;
                }
            }
            runs = std::move(merged);
        }
        uint64_t ଗଣନା_2 = 0;
        ପାଇଁ (ଅଚଳ std::pair<V3_ସଂଖ୍ୟା, V3_ସଂଖ୍ୟା>& run : runs) {
            V3_ସଂଖ୍ୟା span{exprp, width};
            span.opSub(run.second, run.first);
            ଯଦି (span.mostSetBitP1() > 32) ଫେରନ୍ତୁ ସତ୍ୟ;  // More than 2**32 values
            ଗଣନା_2 += span.toUQuad() + 1;
            ଯଦି (ଗଣନା_2 > (uint64_t{1} << 32)) ଫେରନ୍ତୁ ସତ୍ୟ;
        }
        ଫେରନ୍ତୁ ମିଥ୍ୟା;
    }

    // The ranges of values the wildcard patterns of a sized wildcard array, or of a 'with'
    // filter's candidates, give, counted up to more than --coverage-max-bins
    ସ୍ଥିର size_t sizedWildcardRuns(ଅଚଳ AstCoverBin* binp, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* exprp) {
        std::vector<std::pair<V3_ସଂଖ୍ୟା, V3_ସଂଖ୍ୟା>> runs;
        ପାଇଁ (Ast_ଗଣ୍ଠି* rangep = binRangesp(binp); rangep; rangep = rangep->nextp()) {
            ଯଦି (!VN_IS(rangep, Const)) ଜାରି;  // A range, or a value known at construction
            Cross_ମୂଲ୍ୟ_Range range{rangep, resolveWidth(rangep, exprp)};
            ଯଦି (resolve_ମୂଲ୍ୟ(rangep, exprp, ସତ୍ୟ, ସତ୍ୟ, range) && !crossRangeEmpty(range)) {
                crossRangeRuns(range, v3Global.opt.coverageMaxBins(), runs);
            }
        }
        ଫେରନ୍ତୁ runs.size();
    }

    // Check the automatic bins declarations of a coverpoint.  Each stays one declaration, which
    // generates as a partition of the coverpoint domain (see autoBinRuns).
    ଶୂନ୍ୟ checkAutomaticBins(AstCoverpoint* coverpointp, ଅଚଳ Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* exprp) {
        ପାଇଁ (Ast_ଗଣ୍ଠି* binp = coverpointp->binsp(); binp; binp = binp->nextp()) {
            AstCoverBin* ଅଚଳ cbinp = VN_AS(binp, CoverBin);
            ଯଦି (cbinp->binsType() != VCoverBinsType::BINS_AUTO) ଜାରି;
            ଅଚଳ AstConst* ଅଚଳ constp = VN_CAST(cbinp->arraySizep(), Const);
            ଯଦି (!constp) {
                cbinp->v3error("Automatic bins array size must be a constant");
            } ଅନ୍ୟଥା ଯଦି (const_ଗଣନା(constp) < 1) {
                cbinp->v3error("Automatic bins array size must be >= 1, got "
                               << constp->ଅଙ୍କ().toDecimalS());
            } ଅନ୍ୟଥା ଯଦି (const_ଗଣନା(constp) > v3Global.opt.coverageMaxBins()) {
                cbinp->v3error("Automatic bins array size of "
                               << constp->ଅଙ୍କ().toDecimalU() << " exceeds limit of "
                               << v3Global.opt.coverageMaxBins() << '\n'
                               << cbinp->warnMore() << "... Suggest a larger --coverage-max-bins");
            } ଅନ୍ୟଥା ଯଦି (!exprp->dtypep()->skipRefp()->isIntegralOrPacked()) {
                cbinp->v3error("Automatic bins are not allowed on a coverpoint of a non-integral "
                               "expression (IEEE 1800-2023 19.5.3).");
            }
        }
    }

    // Extract all option values of a coverpoint or cross in a single pass.
    // atLeastOut: option.at_least (default the covergroup's)
    // autoBinMaxOut: option.auto_bin_max (coverpoint overrides covergroup, default 64)
    ଶୂନ୍ୟ extractItemOptions(Ast_ଗଣ୍ଠି* optionsp, ପୂର୍ଣ୍ଣସଂଖ୍ୟା& atLeastOut, ପୂର୍ଣ୍ଣସଂଖ୍ୟା& autoBinMaxOut) {
        atLeastOut = m_cgAtLeast;
        autoBinMaxOut = -1;  // -1 = not set at coverpoint level
        ପାଇଁ (Ast_ଗଣ୍ଠି* optionp = optionsp; optionp; optionp = optionp->nextp()) {
            AstCoverOption* ଅଚଳ optp = VN_AS(optionp, CoverOption);
            // Weights may be non-constant; generateItemWeight() handles them.  A cross keeps
            // its unsupported options, which V3LinkParse warned of.
            ଯଦି (!(optp->optType() == VCoverOptionType::AT_LEAST
                  || optp->optType() == VCoverOptionType::AUTO_BIN_MAX)) {
                ଜାରି;
            }
            ଅଚଳ AstConst* ଅଚଳ constp = constOption(optp->valuep(), optp->optType());
            ଯଦି (!constp) ଜାରି;
            ଯଦି (optp->optType() == VCoverOptionType::AT_LEAST) {
                atLeastOut = constAtLeast(constp);
            } ଅନ୍ୟଥା {
                autoBinMaxOut = constp->toSInt();
            }
        }
        // Fall back to covergroup-level auto_bin_max if not set at coverpoint level
        ଯଦି (autoBinMaxOut < 0) {
            ଯଦି (m_covergroupp->cgAutoBinMax() >= 0) {
                autoBinMaxOut = m_covergroupp->cgAutoBinMax();
            } ଅନ୍ୟଥା {
                autoBinMaxOut = 64;  // Default per IEEE 1800-2023 Table 19-1
            }
        }
    }

    // IEEE 1800-2023 19.5.2: an enum coverpoint has one automatic bin per enumeration value
    ଶୂନ୍ୟ createEnumAutoBins(AstCoverpoint* coverpointp, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* exprp,
                            ଅଚଳ AstEnumDType* enump) {
        File_ଧାଡ଼ି* ଅଚଳ fl = coverpointp->fileline();
        ପାଇଁ (ଅଚଳ AstEnumItem* itemp = enump->itemsp(); itemp;
             itemp = VN_AS(itemp->nextp(), EnumItem)) {
            AstConst* ଅଚଳ lop = new_ମୂଲ୍ୟ_Const(fl, VN_AS(itemp->valuep(), Const)->ଅଙ୍କ(), exprp);
            AstInsideRange* ଅଚଳ rangep = ନୂତନ AstInsideRange{fl, lop, lop->clone_ବୃକ୍ଷ(ମିଥ୍ୟା)};
            rangep->dtypeFrom(exprp);
            coverpointp->addBinsp(
                ନୂତନ AstCoverBin{fl, "auto[" + itemp->name() + "]", rangep, ମିଥ୍ୟା, ମିଥ୍ୟା});
        }
    }

    // IEEE 1800-2023 19.5.3/19.11.1: partition first, then apply exclusions.  The partition is one
    // automatic bins declaration, generated as a run like 'bins auto[N]' but numbering its bins.
    ଶୂନ୍ୟ createImplicitAutoBins(AstCoverpoint* coverpointp, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* exprp, ପୂର୍ଣ୍ଣସଂଖ୍ୟା autoBinMax) {
        ଯଦି (coverpointp->user2()) ଫେରନ୍ତୁ;  // Declared bins, ignored, leave no bins
        ପାଇଁ (Ast_ଗଣ୍ଠି* nodep = coverpointp->binsp(); nodep; nodep = nodep->nextp()) {
            ଅଚଳ VCoverBinsType kind = VN_AS(nodep, CoverBin)->binsType();
            ଯଦି (kind != VCoverBinsType::BINS_IGNORE && kind != VCoverBinsType::BINS_ILLEGAL)
                ଫେରନ୍ତୁ;
        }
        ଯଦି (ଅଚଳ AstEnumDType* ଅଚଳ enump
            = VN_CAST(exprp->dtypep()->skipRefToEnump(), EnumDType)) {
            createEnumAutoBins(coverpointp, exprp, enump);
            ଫେରନ୍ତୁ;
        }
        ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା width = exprp->width();
        uint32_t ଗଣନା_2 = width < 31 ? std::min<uint32_t>(uint32_t{1} << width, autoBinMax)
                                    : static_cast<uint32_t>(autoBinMax);
        ଯଦି (!ଗଣନା_2) ଫେରନ୍ତୁ;
        ଯଦି (!exprp->dtypep()->skipRefp()->isIntegralOrPacked()) {
            coverpointp->v3error("Coverpoint of a non-integral expression requires explicit bins "
                                 "(IEEE 1800-2023 19.5.3).");
            ଫେରନ୍ତୁ;
        }
        ଯଦି (ଗଣନା_2 > v3Global.opt.coverageMaxBins()) {
            coverpointp->v3warn(COVERIGN, "Unsupported: more than "
                                              << v3Global.opt.coverageMaxBins()
                                              << " automatic bins from 'option.auto_bin_max'; "
                                                 "using "
                                              << v3Global.opt.coverageMaxBins() << ".\n"
                                              << coverpointp->warnMore()
                                              << "... Suggest a larger --coverage-max-bins");
            ଗଣନା_2 = v3Global.opt.coverageMaxBins();
        }
        File_ଧାଡ଼ି* ଅଚଳ fl = coverpointp->fileline();
        coverpointp->addBinsp(ନୂତନ AstCoverBin{fl, "auto", ନୂତନ AstConst{fl, ଗଣନା_2},
                                              VCoverBinsType::BINS_AUTO_IMPLICIT});
    }

    // Sanitize generated names to be valid C++ identifiers
    ସ୍ଥିର string sanitizeGeneratedName(string name) {
        std::replace(name.begin(), name.end(), '[', '_');
        std::replace(name.begin(), name.end(), ']', '_');
        ଫେରନ୍ତୁ name;
    }

    // Capture an iff guard in a function-local temporary so it is evaluated once per sample()
    AstVarRef* captureIffToTemp(Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* iffp, ଅଚଳ string& tempName) {
        File_ଧାଡ଼ି* ଅଚଳ fl = iffp->fileline();
        AstVar* ଅଚଳ iffVarp
            = ନୂତନ AstVar{fl, VVarType::BLOCKTEMP, tempName, iffp->findBitDType()};
        iffVarp->funcLocal(ସତ୍ୟ);
        m_sampleFuncp->addStmtsp(iffVarp);
        iffp->unlinkFrBack();
        m_sampleFuncp->addStmtsp(
            ନୂତନ AstAssign{fl, ନୂତନ AstVarRef{fl, iffVarp, VAccess::WRITE}, iffp});
        ଫେରନ୍ତୁ ନୂତନ AstVarRef{fl, iffVarp, VAccess::READ};
    }

    // Add a statement sampling the current coverpoint to sample(), under its iff guard if any
    ଶୂନ୍ୟ addSampleStmt(Ast_ଗଣ୍ଠି* stmtp) {
        ଯଦି (m_cpGuardp) {
            m_cpGuardp->addThensp(stmtp);
        } ଅନ୍ୟଥା {
            m_sampleFuncp->addStmtsp(stmtp);
        }
    }

    // Create previous value variable for transition tracking
    AstVar* create_Prev_ମୂଲ୍ୟ_Var(AstCoverpoint* coverpointp, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* exprp) {
        // Check if already created
        ଯଦି (AstVar* ଅଚଳ prevVarp = VN_CAST(coverpointp->user1p(), Var)) ଫେରନ୍ତୁ prevVarp;

        // Create variable to store previous sampled value
        ଅଚଳ string varName = "__Vprev_" + coverpointp->name();
        AstVar* prevVarp
            = ନୂତନ AstVar{coverpointp->fileline(), VVarType::MEMBER, varName, exprp->dtypep()};
        m_covergroupp->addMembersp(prevVarp);

        UINFO(4, "    Created previous value variable: " << varName);

        // Initialize to zero in constructor
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ initExprp
            = ନୂତନ AstConst{prevVarp->fileline(), AstConst::Widthed_ମୂଲ୍ୟ{}, prevVarp->width(), 0};
        Ast_ଗଣ୍ଠି_Stmt* ଅଚଳ initStmtp = ନୂତନ AstAssign{
            prevVarp->fileline(), ନୂତନ AstVarRef{prevVarp->fileline(), prevVarp, VAccess::WRITE},
            initExprp};
        m_constructorp->addStmtsp(initStmtp);

        coverpointp->user1p(prevVarp);
        ଫେରନ୍ତୁ prevVarp;
    }

    // Create state position variable for multi-value transition bins
    // Tracks position in sequence: 0=not started, 1=seen first item, etc.
    AstVar* createSequenceStateVar(AstCoverpoint* coverpointp, AstCoverBin* binp) {
        // Create variable to track sequence position
        ଅଚଳ string varName = "__Vseqpos_" + coverpointp->name() + "_" + binp->name();
        // Use 8-bit integer for state position (sequences rarely > 255 items)
        AstVar* stateVarp
            = ନୂତନ AstVar{binp->fileline(), VVarType::MEMBER, varName, VFlagLogicPacked{}, 8};
        m_covergroupp->addMembersp(stateVarp);

        UINFO(4, "    Created sequence state variable: " << varName);

        // Initialize to 0 (not started) in constructor
        Ast_ଗଣ୍ଠି_Stmt* ଅଚଳ initStmtp = ନୂତନ AstAssign{
            stateVarp->fileline(), ନୂତନ AstVarRef{stateVarp->fileline(), stateVarp, VAccess::WRITE},
            ନୂତନ AstConst{stateVarp->fileline(), AstConst::Widthed_ମୂଲ୍ୟ{}, 8, 0}};
        m_constructorp->addStmtsp(initStmtp);

        ଫେରନ୍ତୁ stateVarp;
    }

    ଶୂନ୍ୟ generateCoverpointCode(AstCoverpoint* coverpointp) {
        UINFO(4, "  Generating code for coverpoint: " << coverpointp->name());

        // Get the coverpoint expression
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* exprp = coverpointp->exprp();

        // Check automatic bins before processing
        checkAutomaticBins(coverpointp, exprp);

        // Extract all coverpoint options in a single pass
        ପୂର୍ଣ୍ଣସଂଖ୍ୟା at_Least_ମୂଲ୍ୟ;
        ପୂର୍ଣ୍ଣସଂଖ୍ୟା autoBinMax;
        extractItemOptions(coverpointp->optionsp(), at_Least_ମୂଲ୍ୟ, autoBinMax);
        UINFO(6, "    Coverpoint at_least = " << at_Least_ମୂଲ୍ୟ << " auto_bin_max = " << autoBinMax);

        // Create implicit automatic bins if no regular bins exist
        createImplicitAutoBins(coverpointp, exprp, autoBinMax);

        // A false iff guard disables sampling of the coverpoint (IEEE 1800-2023 19.5), so
        // neither is its expression evaluated, nor are its bins or transition state updated
        VL_RESTORER(m_cpGuardp);
        ଯଦି (Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ iffp = coverpointp->iffp()) {
            m_cpGuardp = ନୂତନ AstIf{iffp->fileline(), iffp->unlinkFrBack()};
        }

        AstVar* ଅଚଳ ମୂଲ୍ୟ_Varp = ନୂତନ AstVar{
            coverpointp->fileline(), VVarType::BLOCKTEMP,
            "__VcpValue_" + sanitizeGeneratedName(coverpointp->name()), exprp->dtypep()};
        ମୂଲ୍ୟ_Varp->funcLocal(ସତ୍ୟ);
        m_sampleFuncp->addStmtsp(ମୂଲ୍ୟ_Varp);
        exprp->unlinkFrBack();
        addSampleStmt(ନୂତନ AstAssign{
            coverpointp->fileline(),
            ନୂତନ AstVarRef{coverpointp->fileline(), ମୂଲ୍ୟ_Varp, VAccess::WRITE}, exprp});
        coverpointp->exprp(ନୂତନ AstVarRef{coverpointp->fileline(), ମୂଲ୍ୟ_Varp, VAccess::READ});
        exprp = coverpointp->exprp();

        // Every coverpoint routes through the VlCoverpoint runtime.  Transition coverpoints are
        // included: their per-value matching is still generated as a state machine in sample()
        // (see generateCoverpoint), but the bin hit is recorded in the runtime bin
        // rather than a bare counter.
        generateCoverpoint(coverpointp, exprp, at_Least_ମୂଲ୍ୟ);
        // Sampling follows what generateCoverpoint added unguarded, such as clearing the hit list
        ଯଦି (m_cpGuardp) m_sampleFuncp->addStmtsp(m_cpGuardp);
    }

    // Build the condition under which a default bin matches: NOT(OR of all normal bins).
    Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* buildDefaultCondition(AstCoverpoint* coverpointp, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* exprp,
                                       File_ଧାଡ଼ି* fl) {
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* anyBinMatchp = nullptr;
        ପାଇଁ (Ast_ଗଣ୍ଠି* binp = coverpointp->binsp(); binp; binp = binp->nextp()) {
            AstCoverBin* ଅଚଳ cbinp = VN_AS(binp, CoverBin);
            ଯଦି (cbinp->binsType() == VCoverBinsType::BINS_DEFAULT
                || cbinp->binsType() == VCoverBinsType::BINS_IGNORE
                || cbinp->binsType() == VCoverBinsType::BINS_ILLEGAL)
                ଜାରି;
            // The values of constructed bins are known at construction; see emitSizedSample
            ଯଦି (isConstructedBins(cbinp)) ଜାରି;
            ଯଦି (isAutoBins(cbinp)) {
                // Automatic bins partition the whole domain, leaving no default value
                ଯଦି (anyBinMatchp) VL_DO_DANGLING(ଠେଲ_Deletep(anyBinMatchp), anyBinMatchp);
                ଫେରନ୍ତୁ ନୂତନ AstConst{fl, AstConst::BitFalse{}};
            }
            Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ binCondp = buildBinCondition(cbinp, exprp);
            UASSERT_OBJ(binCondp, cbinp,
                        "buildBinCondition returned nullptr for non-ignore/non-illegal bin");
            anyBinMatchp = anyBinMatchp ? ନୂତନ AstOr{fl, anyBinMatchp, binCondp} : binCondp;
        }
        ଫେରନ୍ତୁ anyBinMatchp ? static_cast<Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି*>(ନୂତନ AstNot{fl, anyBinMatchp})
                            : static_cast<Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି*>(ନୂତନ AstConst{fl, AstConst::BitTrue{}});
    }

    //====================================================================
    // VlCoverpoint conversion

    ସ୍ଥିର ତାର୍କିକ coverpointHasStateExclusions(ଅଚଳ AstCoverpoint* coverpointp) {
        ପାଇଁ (ଅଚଳ Ast_ଗଣ୍ଠି* nodep = coverpointp->binsp(); nodep; nodep = nodep->nextp()) {
            ଅଚଳ AstCoverBin* ଅଚଳ binp = VN_AS(nodep, CoverBin);
            ଯଦି (!binp->transp() && binp->rangesp()
                && (binp->binsType() == VCoverBinsType::BINS_IGNORE
                    || binp->binsType() == VCoverBinsType::BINS_ILLEGAL)) {
                ଫେରନ୍ତୁ ସତ୍ୟ;
            }
        }
        ଫେରନ୍ତୁ ମିଥ୍ୟା;
    }

    ସ୍ଥିର ତାର୍କିକ coverpointHasConstructedBins(ଅଚଳ AstCoverpoint* coverpointp) {
        ପାଇଁ (ଅଚଳ Ast_ଗଣ୍ଠି* nodep = coverpointp->binsp(); nodep; nodep = nodep->nextp()) {
            ଯଦି (isConstructedBins(VN_AS(nodep, CoverBin))) ଫେରନ୍ତୁ ସତ୍ୟ;
        }
        ଫେରନ୍ତୁ ମିଥ୍ୟା;
    }

    // True if a Normal state bin, or an array-bin element, has no value of the coverpoint's
    // type (IEEE 1800-2023 19.5.7).  Array ranges enumerate in-type values, so cannot vanish.
    ସ୍ଥିର ତାର୍କିକ coverpointHasEmptyBins(ଅଚଳ AstCoverpoint* coverpointp) {
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ exprp = coverpointp->exprp();
        ପାଇଁ (Ast_ଗଣ୍ଠି* nodep = coverpointp->binsp(); nodep; nodep = nodep->nextp()) {
            ଅଚଳ AstCoverBin* ଅଚଳ binp = VN_AS(nodep, CoverBin);
            ଯଦି (!binp->binsType().binIsNormal() || binp->transp() || !binp->rangesp()) ଜାରି;
            // A wildcard array has a bin for each value its elements match, so none empty, and
            // the constructor creates no bin without values of a 'with' filter
            ଯଦି ((binp->isArray() && binp->isWildcard()) || binWith(binp)) ଜାରି;
            ତାର୍କିକ empty = ସତ୍ୟ;
            ପାଇଁ (Ast_ଗଣ୍ଠି* valuep = binp->rangesp(); valuep; valuep = valuep->nextp()) {
                ଯଦି (binp->isArray() && VN_IS(valuep, InsideRange)) ଜାରି;
                Cross_ମୂଲ୍ୟ_Range range{valuep, resolveWidth(valuep, exprp)};
                ଅଚଳ ତାର୍କିକ none = resolve_ମୂଲ୍ୟ(valuep, exprp, ସତ୍ୟ, binp->isWildcard(), range)
                                  && crossRangeEmpty(range);
                ଯଦି (binp->isArray() && none) ଫେରନ୍ତୁ ସତ୍ୟ;
                empty &= none;
            }
            ଯଦି (empty && !binp->isArray()) ଫେରନ୍ତୁ ସତ୍ୟ;
        }
        ଫେରନ୍ତୁ ମିଥ୍ୟା;
    }

    // True if a coverpoint has any transition bin.  Used to decide whether sample() emits the
    // end-of-sample previous-value update that transition matching needs.
    ସ୍ଥିର ତାର୍କିକ coverpointHasTransition(AstCoverpoint* coverpointp) {
        ପାଇଁ (Ast_ଗଣ୍ଠି* binp = coverpointp->binsp(); binp; binp = binp->nextp()) {
            ଯଦି (VN_AS(binp, CoverBin)->transp()) ଫେରନ୍ତୁ ସତ୍ୟ;
        }
        ଫେରନ୍ତୁ ମିଥ୍ୟା;
    }

    // The interned AstBasicDType for one of the covergroup runtime keywords.
    ସ୍ଥିର AstBasicDType* basicDType(File_ଧାଡ଼ି* fl, VBasicDTypeKwd kwd) {
        ଫେରନ୍ତୁ v3Global.rootp()->typeTablep()->findBasicDType(fl, kwd);
    }

    // Get (or create) the coverpoint dtype for a hit-list bound, interned so each distinct bound
    // yields one node.
    AstCoverpointDType* coverpointDType(File_ଧାଡ଼ି* fl, uint32_t hitBound) {
        AstCoverpointDType*& typep = m_cpDTypes[hitBound];
        ଯଦି (!typep) {
            typep = ନୂତନ AstCoverpointDType{fl, hitBound};
            v3Global.rootp()->typeTablep()->addTypesp(typep);
        }
        ଫେରନ୍ତୁ typep;
    }

    // The key of the covergroup's type in the coverage registry, which get_coverage() queries
    std::string covergroupProtectedName() ଅଚଳ {
        ଫେରନ୍ତୁ VIdProtect::protect_ଶବ୍ଦଗୁଡ଼ିକ_If(m_covergroupName, v3Global.opt.protectIds());
    }

    // The name of the covergroup's type, which keys the coverage registry and database: as
    // $typename names it (IEEE 1800-2023 20.6.1), so apart for covergroups of distinct scopes and
    // specializations, and alike in each Verilator run of a hierarchical design.  As design units
    // of distinct libraries may share a name, one of a library other than the default is prefixed
    // by its library, as '%l' prints it (IEEE 1800-2023 33.4).
    std::string covergroupTypeName() ଅଚଳ {
        UASSERT_OBJ(m_unitp, m_covergroupp, "Covergroup declared outside of a design unit");
        ଅଚଳ std::string& libname = m_unitp->libname();
        ଅଚଳ std::string name = m_covergroupp->dtypeName(ସତ୍ୟ);
        ଫେରନ୍ତୁ libname == "work" ? name : libname + "." + name;
    }

    // Emit the covergroup's instance handle member and the constructor statement that creates
    // its node in the per-context coverage registry.  Runs before any coverpoint or cross is
    // generated, so their runtimes can be added to the node as they are created.
    ଶୂନ୍ୟ generateInstanceAttach() {
        File_ଧାଡ଼ି* ଅଚଳ fl = m_covergroupp->fileline();
        // V3LinkParse synthesizes a 'new' for every covergroup; the item generators below already
        // rely on that, and this attach runs even for a covergroup with no coverpoints at all.
        UASSERT_OBJ(m_constructorp, m_covergroupp, "Covergroup missing synthesized constructor");
        m_cgInstVarp = ନୂତନ AstVar{fl, VVarType::MEMBER, "__Vcg_inst",
                                  basicDType(fl, VBasicDTypeKwd::COVERGROUP_INSTHANDLE)};
        m_covergroupp->addMembersp(m_cgInstVarp);

        m_constructorp->addStmtsp(
            itemCall(fl, m_cgInstVarp, VCMethod::COVERGROUP_ATTACH,
                     {ctext(fl, "vlSymsp->_vm_contextp__->covergroupRegistryp()"
                                "->newCovergroupInst("
                                    + quoted(covergroupProtectedName())
                                    + (m_cgMayMerge ? ", true" : ", false") + ")")},
                     /*usePtr=*/ମିଥ୍ୟା)
                ->makeStmt());
        // The node reads option.weight in place, so procedural assignments take effect
        AstCExpr* ଅଚଳ weightAddrp = ନୂତନ AstCExpr{fl, "&"};
        weightAddrp->add(newOptionSel(fl, optionVar(ମିଥ୍ୟା), "weight", VAccess::READ));
        m_constructorp->addStmtsp(itemCall(fl, m_cgInstVarp, VCMethod::COVERGROUP_LEND_WEIGHT,
                                           {weightAddrp, file_ଧାଡ଼ି_Debug(fl)}, /*usePtr=*/ମିଥ୍ୟା)
                                      ->makeStmt());
    }

    // A '__Vcg_inst.p()-><method>()' call on the covergroup's instance node
    AstCMethodHard* instanceCall(File_ଧାଡ଼ି* fl, VCMethod method) {
        // '__Vcg_inst.p()' -- a value handle, so '.' not '->'
        AstCMethodHard* ଅଚଳ instp
            = ନୂତନ AstCMethodHard{fl, memberRef(fl, m_cgInstVarp), VCMethod::COVERGROUP_INST_P};
        instp->usePtr(ମିଥ୍ୟା);
        instp->dtypeSetVoid();  // Opaque receiver; only ever the 'fromp' of the call below
        AstCMethodHard* ଅଚଳ callp = ନୂତନ AstCMethodHard{fl, instp, method};
        callp->usePtr(ସତ୍ୟ);
        ଫେରନ୍ତୁ callp;
    }

    // Emit 'this->__Vcp_x = this->__Vcg_inst.p()->addCoverpoint<K>();' (or addCross), which
    // creates the item runtime in the instance node and borrows a pointer to it.
    AstAssign* makeItemCreate(File_ଧାଡ଼ି* fl, AstVar* itemVarp, VCMethod method) {
        AstCMethodHard* ଅଚଳ createp = instanceCall(fl, method);
        createp->dtypep(itemVarp->dtypep());
        ଫେରନ୍ତୁ ନୂତନ AstAssign{fl, memberRef(fl, itemVarp, VAccess::WRITE), createp};
    }

    // Constant bounds of one rangesp() element (an InsideRange or a single Const).  Each bound is
    // the raw AST node -- an AstConst or an AstUnbounded ('$') -- with the const/unbounded view
    // derived on demand, so there is one source of truth per bound.  After a successful
    // constRangeBounds() neither node is null; a single Const has both bounds aliasing one node.
    ସଂରଚନା RangeBounds final {
        Ast_ଗଣ୍ଠି* loNodep = nullptr;  // low bound: AstConst or AstUnbounded
        Ast_ଗଣ୍ଠି* hiNodep = nullptr;  // high bound: AstConst or AstUnbounded
        ତାର୍କିକ loUnbounded() ଅଚଳ { ଫେରନ୍ତୁ VN_IS(loNodep, Unbounded); }
        ତାର୍କିକ hiUnbounded() ଅଚଳ { ଫେରନ୍ତୁ VN_IS(hiNodep, Unbounded); }
        AstConst* loConstp() ଅଚଳ { ଫେରନ୍ତୁ VN_CAST(loNodep, Const); }
        AstConst* hiConstp() ଅଚଳ { ଫେରନ୍ତୁ VN_CAST(hiNodep, Const); }
    };

    // Decode one rangesp() element into its constant bounds.  Returns false if rp is neither an
    // InsideRange nor a single Const, or if a present bound is non-constant or 4-state.  '$'
    // bounds are left as AstUnbounded (not resolved) -- the caller applies its own policy.
    // Centralizes the InsideRange/Const/Unbounded decode shared by the hit-list-bound paths.
    ସ୍ଥିର ତାର୍କିକ constRangeBounds(Ast_ଗଣ୍ଠି* rp, RangeBounds& rb) {
        ଯଦି (AstInsideRange* ଅଚଳ irp = VN_CAST(rp, InsideRange)) {
            rb.loNodep = irp->lhsp();
            rb.hiNodep = irp->rhsp();
        } ଅନ୍ୟଥା ଯଦି (AstConst* ଅଚଳ cp = VN_CAST(rp, Const)) {
            rb.loNodep = rb.hiNodep = cp;
        } ଅନ୍ୟଥା {
            ଫେରନ୍ତୁ ମିଥ୍ୟା;
        }
        // Each bound must be a constant unless it is '$'; reject non-const and 4-state.
        AstConst* ଅଚଳ lc = rb.loConstp();
        AstConst* ଅଚଳ hc = rb.hiConstp();
        ଯଦି ((!lc && !rb.loUnbounded()) || (!hc && !rb.hiUnbounded())) ଫେରନ୍ତୁ ମିଥ୍ୟା;
        ଯଦି ((lc && lc->ଅଙ୍କ().isFourState()) || (hc && hc->ଅଙ୍କ().isFourState())) ଫେରନ୍ତୁ ମିଥ୍ୟା;
        ଫେରନ୍ତୁ ସତ୍ୟ;
    }

    // Collect the covered value intervals of a single (non-array) Normal bin.  Returns false
    // if any range isn't a constant/open InsideRange or single Const (e.g. wildcard, non-const).
    ସ୍ଥିର ତାର୍କିକ extractRangeIntervals(AstCoverBin* cbinp, uint64_t maxVal,
                                      std::vector<std::pair<uint64_t, uint64_t>>& out) {
        ଯଦି (!cbinp->rangesp()) ଫେରନ୍ତୁ ମିଥ୍ୟା;
        ପାଇଁ (Ast_ଗଣ୍ଠି* rp = cbinp->rangesp(); rp; rp = rp->nextp()) {
            RangeBounds rb;
            ଯଦି (!constRangeBounds(rp, rb)) ଫେରନ୍ତୁ ମିଥ୍ୟା;
            ଯଦି ((rb.loConstp() && rb.loConstp()->width() > 64)
                || (rb.hiConstp() && rb.hiConstp()->width() > 64)) {
                ଫେରନ୍ତୁ ମିଥ୍ୟା;  // Use the safe slot-count bound for wide values.
            }
            ଅଚଳ uint64_t lo = rb.loUnbounded() ? 0 : rb.loConstp()->toUQuad();
            ଅଚଳ uint64_t hi = rb.hiUnbounded() ? maxVal : rb.hiConstp()->toUQuad();
            ଯଦି (lo > hi) ଫେରନ୍ତୁ ମିଥ୍ୟା;
            out.emplace_back(lo, hi);
        }
        ଫେରନ୍ତୁ ସତ୍ୟ;
    }

    // Append one Normal bin's cross-slot interval-sets to `bins` and bump `slotCount` by an upper
    // bound of the bin's slots holding one value.  Returns false if any part isn't statically
    // enumerable (the caller then falls back to the always-safe slot count).  A non-array bin
    // is one slot covering the union of its intervals; the bins of an array element or of an
    // automatic bins declaration hold disjoint values, so the element or declaration counts once.
    ତାର୍କିକ appendBinCrossSlots(AstCoverBin* cbinp, uint64_t maxVal, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* exprp,
                             std::vector<std::vector<std::pair<uint64_t, uint64_t>>>& bins,
                             ପୂର୍ଣ୍ଣସଂଖ୍ୟା& slot_ଗଣନା) {
        ଯଦି (isAutoBins(cbinp)) {
            ++slot_ଗଣନା;
            bins.ଠେଲ_back({{0, maxVal}});
            ଫେରନ୍ତୁ exprp->width() <= 64;
        }
        ଯଦି (binWith(cbinp)) {
            // A value is in one bin of a filter's, but of a sized array, in one for each range
            // list element holding it; the coverpoint's name is one element
            ପୂର୍ଣ୍ଣସଂଖ୍ୟା elements = 0;
            ପାଇଁ (ଅଚଳ Ast_ଗଣ୍ଠି* rp = binRangesp(cbinp); rp && isSizedArray(cbinp);
                 rp = rp->nextp()) {
                ++elements;
            }
            slot_ଗଣନା += std::max(1, elements);
            ଫେରନ୍ତୁ ମିଥ୍ୟା;
        }
        ଯଦି (cbinp->isArray() && cbinp->isWildcard() && !cbinp->arraySizep()) {
            // A value is in at most one bin of a wildcard array: one slot covering its values.
            // Signed values are sign-extended, not unsigned intervals (see computeHitListBound)
            ++slot_ଗଣନା;
            ଯଦି (exprp->isSigned() || exprp->width() > 64) ଫେରନ୍ତୁ ମିଥ୍ୟା;
            ଅଚଳ BinRuns runs = wildcardBinRuns(cbinp, exprp, ମିଥ୍ୟା);
            ଯଦି (runs.unsupported) ଫେରନ୍ତୁ ମିଥ୍ୟା;
            std::vector<std::pair<uint64_t, uint64_t>> ivs;
            ପାଇଁ (ଅଚଳ BinRun& run : runs.runs) {
                ivs.emplace_back(run.m_lo.toUQuad(), run.m_hi.toUQuad());
            }
            bins.ଠେଲ_back(std::move(ivs));
            ଫେରନ୍ତୁ ସତ୍ୟ;
        }
        ଯଦି (cbinp->isArray()) ଫେରନ୍ତୁ appendArrayBinCrossSlots(cbinp, exprp, bins, slot_ଗଣନା);
        // Non-array bin: one slot covering the union of its intervals.
        ++slot_ଗଣନା;
        std::vector<std::pair<uint64_t, uint64_t>> ivs;
        ଯଦି (cbinp->isWildcard() || !extractRangeIntervals(cbinp, maxVal, ivs)) ଫେରନ୍ତୁ ମିଥ୍ୟା;
        bins.ଠେଲ_back(std::move(ivs));
        ଫେରନ୍ତୁ ସତ୍ୟ;
    }

    // Append the cross slots of an array Normal bin: each element is a slot covering its
    // values, as at most one of its single-value bins holds a value.  An element holds the
    // values arrayBinRuns() gives it: those of the coverpoint type (IEEE 1800-2023 19.5.7).
    // Elements of a signed coverpoint, non-constant elements, and elements with values beyond
    // 64 bits can't be enumerated, so they count one slot but lose exactness.  Returns false if
    // any element wasn't enumerable to exact values.
    ତାର୍କିକ appendArrayBinCrossSlots(AstCoverBin* cbinp, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* exprp,
                                  std::vector<std::vector<std::pair<uint64_t, uint64_t>>>& bins,
                                  ପୂର୍ଣ୍ଣସଂଖ୍ୟା& slot_ଗଣନା) {
        // Signed values resolve sign-extended, not as unsigned intervals, and wildcard patterns
        // are not intervals
        ତାର୍କିକ exact = !exprp->isSigned() && !cbinp->isWildcard();
        ପାଇଁ (Ast_ଗଣ୍ଠି* rp = cbinp->rangesp(); rp; rp = rp->nextp()) {
            ++slot_ଗଣନା;
            RangeBounds rb;
            Cross_ମୂଲ୍ୟ_Range range{rp, resolveWidth(rp, exprp)};
            ଯଦି (!exact || !constRangeBounds(rp, rb)
                || !resolve_ମୂଲ୍ୟ(rp, exprp, ସତ୍ୟ, ମିଥ୍ୟା, range)) {
                exact = ମିଥ୍ୟା;
                ଜାରି;
            }
            ଯଦି (crossRangeEmpty(range)) ଜାରି;  // Its bin, if any, holds no value
            ଯଦି (range.hi.mostSetBitP1() > 64) {
                exact = ମିଥ୍ୟା;
                ଜାରି;
            }
            bins.ଠେଲ_back({{range.lo.toUQuad(), range.hi.toUQuad()}});
        }
        ଫେରନ୍ତୁ exact;
    }

    // Compute the hit-list bound for a coverpoint: the maximum number of Normal
    // bins one sample value can match.  Non-cross-fed coverpoints don't feed a cross, so
    // their hit list is unused -> 1.  Otherwise compute the exact max bin overlap; fall back
    // to the (always-safe) Normal-slot count when any bin isn't statically analyzable.
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା computeHitListBound(AstCoverpoint* coverpointp, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* exprp, ତାର୍କିକ crossFed) {
        ଯଦି (!crossFed) ଫେରନ୍ତୁ 1;
        ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା width = exprp->width();
        ଅଚଳ uint64_t maxVal = (width >= 64) ? UINT64_MAX : ((1ULL << width) - 1);
        // One entry per Normal bin (cross slot): its covered intervals.
        std::vector<std::vector<std::pair<uint64_t, uint64_t>>> bins;
        ପୂର୍ଣ୍ଣସଂଖ୍ୟା slot_ଗଣନା = 0;  // == runtime m_normal; the safe fallback bound
        // Unsigned intervals cannot establish overlap between differently sized signed values.
        ତାର୍କିକ exact = !exprp->isSigned();
        ପାଇଁ (Ast_ଗଣ୍ଠି* binp = coverpointp->binsp(); binp; binp = binp->nextp()) {
            AstCoverBin* ଅଚଳ cbinp = VN_AS(binp, CoverBin);
            ଯଦି (!cbinp->binsType().binIsNormal())
                ଜାରି;  // ignore/illegal/default: not hit-listed
            ଯଦି (!appendBinCrossSlots(cbinp, maxVal, exprp, bins, slot_ଗଣନା)) exact = ମିଥ୍ୟା;
        }
        ଯଦି (!exact) ଫେରନ୍ତୁ std::max(1, slot_ଗଣନା);
        ଯଦି (bins.empty()) ଫେରନ୍ତୁ 1;
        // Max overlap occurs at some interval start; count covering bins at each lo.
        std::vector<uint64_t> pts;
        ପାଇଁ (ଅଚଳ auto& b : bins)
            ପାଇଁ (ଅଚଳ auto& iv : b) pts.ଠେଲ_back(iv.first);
        std::sort(pts.begin(), pts.end());
        pts.erase(std::unique(pts.begin(), pts.end()), pts.end());
        ପୂର୍ଣ୍ଣସଂଖ୍ୟା maxOverlap = 1;
        ପାଇଁ (ଅଚଳ uint64_t p : pts) {
            ପୂର୍ଣ୍ଣସଂଖ୍ୟା cnt = 0;
            ପାଇଁ (ଅଚଳ auto& b : bins) {
                ପାଇଁ (ଅଚଳ auto& iv : b) {
                    ଯଦି (iv.first <= p && p <= iv.second) {
                        ++cnt;
                        ଭାଙ୍ଗ;  // count each bin at most once
                    }
                }
            }
            ଯଦି (cnt > maxOverlap) maxOverlap = cnt;
        }
        ଫେରନ୍ତୁ maxOverlap;
    }

    // A 'this->m_member' reference for embedding in an AstCStmt
    AstVarRef* memberRef(File_ଧାଡ଼ି* fl, AstVar* varp, VAccess access = VAccess::READ) {
        AstVarRef* ଅଚଳ refp = ନୂତନ AstVarRef{fl, varp, access};
        refp->selfPointer(VSelfPointerText{VSelfPointerText::This{}});
        ଫେରନ୍ତୁ refp;
    }

    // A 'this->m_member-><method>(args...)' call on one member.  usePtr is false only for
    // __Vcg_inst, which is a value handle; the item members are borrowed pointers into the
    // instance node.  Numeric arguments are AstConst; the rest are C++ text that has no AST
    // form (see ctext).
    AstCMethodHard* itemCall(File_ଧାଡ଼ି* fl, AstVar* varp, VCMethod method,
                             ଅଚଳ std::vector<Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି*>& args = {}, ତାର୍କିକ usePtr = ସତ୍ୟ) {
        AstCMethodHard* ଅଚଳ callp = ନୂତନ AstCMethodHard{fl, memberRef(fl, varp), method};
        ପାଇଁ (Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ argp : args) callp->addPinsp(argp);
        callp->usePtr(usePtr);
        callp->dtypeSetVoid();
        ଫେରନ୍ତୁ callp;
    }

    // An unsigned integer argument.
    ସ୍ଥିର AstConst* cnum(File_ଧାଡ଼ି* fl, uint32_t ମୂଲ୍ୟ) { ଫେରନ୍ତୁ ନୂତନ AstConst{fl, ମୂଲ୍ୟ}; }

    // A literal C++ argument with no AST equivalent: a 'const char*' string literal (an SV
    // string AstConst emits '"..."s', a std::string temporary the runtime cannot borrow), a
    // VlCovBinKind enum token, a constant selection-word initializer list, a VlFileLineDebug, or
    // a '__V' temporary declared by the enclosing AstCStmt.
    ସ୍ଥିର AstCExpr* ctext(File_ଧାଡ଼ି* fl, ଅଚଳ std::string& text) {
        ଫେରନ୍ତୁ ନୂତନ AstCExpr{fl, text};
    }

    // A C++ string literal.  Escapes control characters as the emitter does elsewhere -- bin
    // names and filenames reach the generated code verbatim when --protect-ids is off, and an
    // SV escaped identifier may hold a quote or backslash.
    ସ୍ଥିର std::string quoted(ଅଚଳ std::string& text) {
        ଫେରନ୍ତୁ "\"" + V3OutFormatter::quoteNameControls(text) + "\"";
    }

    // A 'VlFileLineDebug' argument: where the runtime reports an error about fl's construct
    ସ୍ଥିର AstCExpr* file_ଧାଡ଼ି_Debug(File_ଧାଡ଼ି* fl) {
        ଅଚଳ std::string filename
            = VIdProtect::protectIf(fl->filename(), v3Global.opt.protectIds());
        ଫେରନ୍ତୁ ctext(fl, "VlFileLineDebug{" + quoted(filename) + ", "
                             + std::to_string(fl->lineno()) + "}");
    }

    // Check that an element of an array bin (bins b[] = {values/ranges}) is a two-state
    // constant value or range; false if not, after reporting it if 'report'.
    ସ୍ଥିର ତାର୍କିକ checkArrayBinElement(AstCoverBin* arrayBinp, Ast_ଗଣ୍ଠି* rangep, ତାର୍କିକ report = ସତ୍ୟ) {
        ଯଦି (ଅଚଳ AstInsideRange* ଅଚଳ irp = VN_CAST(rangep, InsideRange)) {
            ଅଚଳ AstConst* ଅଚଳ minp = VN_CAST(irp->lhsp(), Const);
            ଅଚଳ AstConst* ଅଚଳ maxp = VN_CAST(irp->rhsp(), Const);
            ଯଦି ((!minp && !VN_IS(irp->lhsp(), Unbounded))
                || (!maxp && !VN_IS(irp->rhsp(), Unbounded))) {
                ଯଦି (report) {
                    arrayBinp->v3error("Non-constant expression in array bins range; "
                                       "range bounds must be constants (IEEE 1800-2023 19.5)");
                }
                ଫେରନ୍ତୁ ମିଥ୍ୟା;
            }
            ଯଦି ((minp && minp->ଅଙ୍କ().isFourState()) || (maxp && maxp->ଅଙ୍କ().isFourState())) {
                ଯଦି (report) {
                    arrayBinp->v3error("Four-state (x/z) value in array bins range bound; "
                                       "range bounds must be two-state constants");
                }
                ଫେରନ୍ତୁ ମିଥ୍ୟା;
            }
        } ଅନ୍ୟଥା ଯଦି (!VN_IS(rangep, Const)) {
            ଯଦି (report) {
                arrayBinp->v3error("Non-constant expression in array bins value list; "
                                   "values must be constants (IEEE 1800-2023 19.5)");
            }
            ଫେରନ୍ତୁ ମିଥ୍ୟା;
        }
        ଫେରନ୍ତୁ ସତ୍ୟ;
    }

    // Individual equality targets of an array bin (bins b[] = {values/ranges}) of a real
    // coverpoint, in order; integral coverpoints generate array bins as runs (see arrayBinRuns).
    // An open-ended bound ('$', AstUnbounded) resolves to the coverpoint domain: '[lo:$]'
    // covers [lo:maxVal] and '[$:hi]' covers [0:hi].  One target is produced per value; ranges
    // whose resolved size would exceed --coverage-max-real-bins (e.g. an open '[lo:$]') are
    // unsupported -- emits COVERIGN, sets unsupportedOut, yields nothing.
    std::vector<Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି*> extract_Array_ମୂଲ୍ୟଗୁଡ଼ିକ(AstCoverBin* arrayBinp, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* exprp,
                                                 ତାର୍କିକ& unsupportedOut) {
        unsupportedOut = ମିଥ୍ୟା;
        ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା width = exprp->width();
        ଅଚଳ uint64_t maxVal = (width >= 64) ? UINT64_MAX : ((1ULL << width) - 1);
        std::vector<Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି*> ମୂଲ୍ୟଗୁଡ଼ିକ;
        ପାଇଁ (Ast_ଗଣ୍ଠି* rangep = arrayBinp->rangesp(); rangep; rangep = rangep->nextp()) {
            rangep = V3Const::constifyEdit(rangep);
            ଯଦି (!checkArrayBinElement(arrayBinp, rangep)) ଫେରନ୍ତୁ ମୂଲ୍ୟଗୁଡ଼ିକ;
            ଯଦି (AstInsideRange* ଅଚଳ irp = VN_CAST(rangep, InsideRange)) {
                ଅଚଳ ତାର୍କିକ loUnb = VN_IS(irp->lhsp(), Unbounded);
                ଅଚଳ ତାର୍କିକ hiUnb = VN_IS(irp->rhsp(), Unbounded);
                ଅଚଳ uint64_t lo = loUnb ? 0 : VN_AS(irp->lhsp(), Const)->toUQuad();
                ଅଚଳ uint64_t hi = hiUnb ? maxVal : VN_AS(irp->rhsp(), Const)->toUQuad();
                ଯଦି (hi < lo) ଜାରି;  // empty range contributes no bins
                // Guard against a '$'-bounded or otherwise huge range exploding the bin count.
                ଅଚଳ uint64_t span = hi - lo;  // == valueCount - 1 (no overflow: hi >= lo)
                ଯଦି (span >= v3Global.opt.coverageMaxRealBins()
                    || ମୂଲ୍ୟଗୁଡ଼ିକ.size() + span + 1 > v3Global.opt.coverageMaxRealBins()) {
                    arrayBinp->v3warn(COVERIGN,
                                      "Unsupported: array 'bins' of a real coverpoint "
                                      "covering more than "
                                          << v3Global.opt.coverageMaxRealBins() << " values; bin "
                                          << arrayBinp->prettyNameQ() << " ignored.\n"
                                          << arrayBinp->warnMore()
                                          << "... Suggest a larger --coverage-max-real-bins");
                    unsupportedOut = ସତ୍ୟ;
                    ପାଇଁ (Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ vp : ମୂଲ୍ୟଗୁଡ଼ିକ) VL_DO_DANGLING(ଠେଲ_Deletep(vp), vp);
                    ମୂଲ୍ୟଗୁଡ଼ିକ.clear();
                    ଫେରନ୍ତୁ ମୂଲ୍ୟଗୁଡ଼ିକ;
                }
                ପାଇଁ (uint64_t v = lo; v <= hi; ++v)
                    ମୂଲ୍ୟଗୁଡ଼ିକ.ଠେଲ_back(ନୂତନ AstConst{irp->fileline(), AstConst::Widthed_ମୂଲ୍ୟ{}, width,
                                                  static_cast<uint32_t>(v)});
            } ଅନ୍ୟଥା {
                ମୂଲ୍ୟଗୁଡ଼ିକ.ଠେଲ_back(VN_AS(rangep->clone_ବୃକ୍ଷ(ମିଥ୍ୟା), ଗଣ୍ଠି_ପଦସମଷ୍ଟି));
            }
        }
        ଫେରନ୍ତୁ ମୂଲ୍ୟଗୁଡ଼ିକ;
    }

    ସ୍ଥିର ପୂର୍ଣ୍ଣସଂଖ୍ୟା runWidth(ଅଚଳ Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* exprp) { ଫେରନ୍ତୁ exprp->width() + 1; }

    // Automatic bins partition the coverpoint domain in value order (IEEE 1800-2023 19.5.3): N
    // bins, capped at the number of values, each hold 2^width / N values, and the last bin also
    // holds the remainder.  False for an invalid declaration, already reported.
    ତାର୍କିକ autoBinRuns(AstCoverBin* binp, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* exprp, BinRuns& out) {
        ଅଚଳ uint32_t requested = autoBinsRequested(binp);
        ଯଦି (!requested || !exprp->dtypep()->skipRefp()->isIntegralOrPacked()) ଫେରନ୍ତୁ ମିଥ୍ୟା;
        ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା width = exprp->width();
        ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା arithmeticWidth = runWidth(exprp);
        ଅଚଳ uint32_t ଗଣନା_2
            = width < 32
                  ? static_cast<uint32_t>(std::min<uint64_t>(uint64_t{1} << width, requested))
                  : requested;
        BinRun run{binp, arithmeticWidth, ଗଣନା_2};
        V3_ସଂଖ୍ୟା total{binp, arithmeticWidth};
        total.setBit(width, 1);
        run.m_stride.opDiv(total, V3_ସଂଖ୍ୟା{binp, arithmeticWidth, ଗଣନା_2});
        ଅଚଳ Cross_ମୂଲ୍ୟ_Range domain
            = cross_ମୂଲ୍ୟ_Domain(binp, width, exprp->isSigned(), arithmeticWidth);
        run.m_lo = domain.lo;
        run.m_hi = domain.hi;
        out.runs.ଠେଲ_back(std::move(run));
        out.ଗଣନା_2 = ଗଣନା_2;
        ଫେରନ୍ତୁ ସତ୍ୟ;
    }

    // The elements of an array bin (bins b[] = {values/ranges}), in order, as runs of
    // single-value bins.  A range holds the values of the coverpoint type it contains (IEEE
    // 1800-2023 19.5.7), while a singleton names a bin even without such a value.  Errors on a
    // non-constant element.  More than --coverage-max-bins bins (e.g. an open '[lo:$]' range over
    // a wide coverpoint) are unsupported -- emits COVERIGN, and sets unsupported.
    BinRuns arrayBinRuns(AstCoverBin* arrayBinp, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* exprp) {
        BinRuns out;
        ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା width = runWidth(exprp);
        ପାଇଁ (Ast_ଗଣ୍ଠି* rangep = arrayBinp->rangesp(); rangep; rangep = rangep->nextp()) {
            rangep = V3Const::constifyEdit(rangep);
            ଯଦି (!checkArrayBinElement(arrayBinp, rangep)) ଫେରନ୍ତୁ out;
            ଅଚଳ AstInsideRange* ଅଚଳ irp = VN_CAST(rangep, InsideRange);
            Cross_ମୂଲ୍ୟ_Range range{rangep, resolveWidth(rangep, exprp)};
            ତାର୍କିକ empty = ସତ୍ୟ;
            ଯଦି (!resolve_ମୂଲ୍ୟ(rangep, exprp, ସତ୍ୟ, ମିଥ୍ୟା, range)) {
                rangep->v3warn(E_UNSUPPORTED, "Unsupported: non-integral value in a coverage bin "
                                              "of an integral coverpoint.");
            } ଅନ୍ୟଥା {
                empty = crossRangeEmpty(range);
            }
            ଯଦି (empty && irp) ଜାରି;  // A range without values contributes no bins
            BinRun run{rangep, width, 1};
            run.m_empty = empty;
            uint64_t ଗଣନା_2 = 1;
            ଯଦି (!empty) {
                run.m_lo.opAssign(range.lo);
                run.m_hi.opAssign(range.hi);
                V3_ସଂଖ୍ୟା span{rangep, width};
                span.opSub(run.m_hi, run.m_lo);
                // Wider spans exceed any limit
                ଗଣନା_2 = span.mostSetBitP1() > 32 ? UINT64_MAX : span.toUQuad() + 1;
            }
            ଯଦି (ଗଣନା_2 > v3Global.opt.coverageMaxBins() - out.ଗଣନା_2) {
                arrayBinp->v3warn(COVERIGN, "Unsupported: array 'bins' covering more than "
                                                << v3Global.opt.coverageMaxBins()
                                                << " values (e.g. an open '[lo:$]' range over "
                                                   "a wide coverpoint); bin "
                                                << arrayBinp->prettyNameQ() << " ignored\n"
                                                << arrayBinp->warnMore()
                                                << "... Suggest a larger --coverage-max-bins");
                out.runs.clear();
                out.ଗଣନା_2 = 0;
                out.unsupported = ସତ୍ୟ;
                ଫେରନ୍ତୁ out;
            }
            run.m_ଗଣନା = static_cast<uint32_t>(ଗଣନା_2);
            out.ଗଣନା_2 += run.m_ଗଣନା;
            out.runs.ଠେଲ_back(std::move(run));
        }
        ଫେରନ୍ତୁ out;
    }

    // The bins of a wildcard array (wildcard bins b[] = {...}): one for each coverpoint value
    // an element matches (IEEE 1800-2023 19.5.4, 19.5.7), in value order, and named by the value
    // (19.5.1), as runs of single-value bins.  Errors on a non-constant element.  More than
    // --coverage-max-bins values are unsupported -- emits COVERIGN, and sets unsupported, and for
    // an ignore or illegal array, single.  'report' false omits these diagnostics.
    ସ୍ଥିର BinRuns wildcardBinRuns(AstCoverBin* arrayBinp, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* exprp, ତାର୍କିକ report) {
        BinRuns out;
        ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା width = runWidth(exprp);
        ଅଚଳ V3_ସଂଖ୍ୟା one{arrayBinp, width, 1};
        // Disjoint runs of the values, in value order, each value once
        std::vector<std::pair<V3_ସଂଖ୍ୟା, V3_ସଂଖ୍ୟା>> spans;
        uint64_t ଗଣନା_2 = 0;  // Values of 'spans'
        ପାଇଁ (Ast_ଗଣ୍ଠି* rangep = arrayBinp->rangesp(); rangep; rangep = rangep->nextp()) {
            rangep = V3Const::constifyEdit(rangep);
            ଯଦି (!checkArrayBinElement(arrayBinp, rangep, report)) {
                out.unsupported = ସତ୍ୟ;
                ଫେରନ୍ତୁ out;
            }
            Cross_ମୂଲ୍ୟ_Range range{rangep, resolveWidth(rangep, exprp)};
            ଯଦି (!resolve_ମୂଲ୍ୟ(rangep, exprp, ସତ୍ୟ, ସତ୍ୟ, range)) {
                ଯଦି (report) {
                    rangep->v3warn(E_UNSUPPORTED, "Unsupported: non-integral value in a "
                                                  "coverage bin of an integral coverpoint.");
                }
                ଜାରି;
            }
            ଯଦି (crossRangeEmpty(range)) ଜାରି;
            std::vector<std::pair<V3_ସଂଖ୍ୟା, V3_ସଂଖ୍ୟା>> found;
            crossRangeRuns(range, v3Global.opt.coverageMaxBins(), found);
            ପାଇଁ (ଅଚଳ std::pair<V3_ସଂଖ୍ୟା, V3_ସଂଖ୍ୟା>& run : found) {
                // Coverpoint values, sign-extended in both widths
                spans.emplace_back(V3_ସଂଖ୍ୟା{rangep, width, run.first},
                                   V3_ସଂଖ୍ୟା{rangep, width, run.second});
            }
            std::sort(spans.begin(), spans.end(), [](ଅଚଳ auto& lhs, ଅଚଳ auto& rhs) {
                ଫେରନ୍ତୁ cross_ମୂଲ୍ୟ_Less(lhs.first, rhs.first);
            });
            std::vector<std::pair<V3_ସଂଖ୍ୟା, V3_ସଂଖ୍ୟା>> merged;
            ପାଇଁ (ଅଚଳ std::pair<V3_ସଂଖ୍ୟା, V3_ସଂଖ୍ୟା>& span : spans) {
                // Adjacent or overlapping values join a run; 'first - 1' cannot overflow
                V3_ସଂଖ୍ୟା before{rangep, width};
                before.opSub(span.first, one);
                ଯଦି (merged.empty() || cross_ମୂଲ୍ୟ_Less(merged.back().second, before)) {
                    merged.ଠେଲ_back(span);
                } ଅନ୍ୟଥା ଯଦି (cross_ମୂଲ୍ୟ_Less(merged.back().second, span.second)) {
                    merged.back().second = span.second;
                }
            }
            ଗଣନା_2 = 0;
            ପାଇଁ (ଅଚଳ std::pair<V3_ସଂଖ୍ୟା, V3_ସଂଖ୍ୟା>& span : merged) {
                V3_ସଂଖ୍ୟା size{rangep, width};
                size.opSub(span.second, span.first);
                // Beyond 2^32 values exceed any limit
                ଗଣନା_2 += size.mostSetBitP1() > 32 ? uint64_t{1} << 33 : size.toUQuad() + 1;
                ଯଦି (ଗଣନା_2 > v3Global.opt.coverageMaxBins()) ଭାଙ୍ଗ;
            }
            spans = std::move(merged);
            ଯଦି (ଗଣନା_2 > v3Global.opt.coverageMaxBins()) {
                // An ignore or illegal array still excludes or checks its values, as one bin
                out.single = !arrayBinp->binsType().binIsNormal();
                ଯଦି (report) {
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
                out.unsupported = ସତ୍ୟ;
                ଫେରନ୍ତୁ out;
            }
        }
        ପାଇଁ (ଅଚଳ std::pair<V3_ସଂଖ୍ୟା, V3_ସଂଖ୍ୟା>& span : spans) {
            out.runs.emplace_back(arrayBinp, width, 0);
            BinRun& run = out.runs.back();
            run.m_lo = span.first;
            run.m_hi = span.second;
            // A bin for each value, named by it in the coverpoint's type
            V3_ସଂଖ୍ୟା ମୂଲ୍ୟ = span.first;
            ଯେତେବେଳେ (ସତ୍ୟ) {
                ଅଚଳ V3_ସଂଖ୍ୟା typed{arrayBinp, exprp->width(), ମୂଲ୍ୟ};
                out.ମୂଲ୍ୟଗୁଡ଼ିକ.ଠେଲ_back(exprp->isSigned() ? typed.toDecimalS() : typed.toDecimalU());
                ++run.m_ଗଣନା;
                ଯଦି (ମୂଲ୍ୟ.isCaseEq(span.second)) ଭାଙ୍ଗ;
                V3_ସଂଖ୍ୟା next{arrayBinp, width};
                ମୂଲ୍ୟ = next.opAdd(ମୂଲ୍ୟ, one);
            }
        }
        out.ଗଣନା_2 = static_cast<uint32_t>(ଗଣନା_2);
        ଫେରନ୍ତୁ out;
    }

    // The runs of an automatic bins declaration, or of an array bin of an integral coverpoint.
    // False for other bins, which do not generate as runs, including a wildcard array then one
    // bin (see BinRuns::single).
    ତାର୍କିକ binRunsFor(AstCoverBin* binp, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* exprp, BinRuns& out) {
        ଯଦି (isAutoBins(binp)) {
            ଯଦି (!autoBinRuns(binp, exprp, out)) out.unsupported = ସତ୍ୟ;
            ଫେରନ୍ତୁ ସତ୍ୟ;
        }
        ଯଦି (binp->isArray() && binp->isWildcard()) {
            ଯଦି (!exprp->dtypep()->skipRefp()->isIntegralOrPacked()) {
                Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ falsep = wildcardTypeError(binp, exprp);
                VL_DO_DANGLING(ଠେଲ_Deletep(falsep), falsep);
                out.unsupported = ସତ୍ୟ;
            } ଅନ୍ୟଥା {
                out = wildcardBinRuns(binp, exprp, ସତ୍ୟ);
                ଯଦି (out.single) {
                    binp->isArray(ମିଥ୍ୟା);  // Generates as one bin
                    ଫେରନ୍ତୁ ମିଥ୍ୟା;
                }
            }
            ଫେରନ୍ତୁ ସତ୍ୟ;
        }
        ଯଦି (!binp->isArray() || binp->transp() || binp->isWildcard()
            || !exprp->dtypep()->skipRefp()->isIntegralOrPacked()) {
            ଫେରନ୍ତୁ ମିଥ୍ୟା;
        }
        out = arrayBinRuns(binp, exprp);
        ଫେରନ୍ତୁ ସତ୍ୟ;
    }

    // Emit a 'this->m_cp->addSingleNamer/addArrayNamer(...)' statement for one bin whose first
    // runtime bin index is 'declared'; or with 'valueNames', those naming each bin of an array
    Ast_ଗଣ୍ଠି_Stmt* makeNamer(AstVar* cpVarp, AstCoverBin* binp, int64_t ଗଣନା_2, uint32_t declared,
                           ଅଚଳ std::vector<Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି*>& ମୂଲ୍ୟଗୁଡ଼ିକ = {},
                           ଅଚଳ std::vector<std::string>& ମୂଲ୍ୟ_Names = {}) {
        File_ଧାଡ଼ି* ଅଚଳ fl = binp->fileline();
        CoverpointBins& bins = m_cpBins.at(cpVarp);
        ଅଚଳ uint32_t normal_ଗଣନା
            = binp->binsType().binIsNormal() ? static_cast<uint32_t>(ଗଣନା_2 < 0 ? 1 : ଗଣନା_2) : 0;
        ଅଚଳ BinSpan span{bins.total, normal_ଗଣନା, declared};
        ଯଦି (binp->binsType() == VCoverBinsType::BINS_AUTO_IMPLICIT) {
            bins.implicitAuto = span;  // Selected by bin, see implicitAutoBinSpan
        } ଅନ୍ୟଥା {
            bins.spans.emplace(binp->name(), span);
        }
        bins.total += normal_ଗଣନା;
        ପାଇଁ (uint32_t i = 0; bins.crossed && i < normal_ଗଣନା; ++i) {
            bins.ମୂଲ୍ୟଗୁଡ଼ିକ.ଠେଲ_back({binp, ମୂଲ୍ୟଗୁଡ଼ିକ.empty() ? nullptr : ମୂଲ୍ୟଗୁଡ଼ିକ[i]});
        }
        // Under --protect-ids the filename and bin name flow into the coverage database
        // verbatim, so obfuscate them exactly as line/toggle coverage points are (whole-
        // unit filename, per-word bin name).  A no-op when --protect-ids is off.
        ଅଚଳ ତାର୍କିକ prot = v3Global.opt.protectIds();
        ଅଚଳ ତାର୍କିକ single = ଗଣନା_2 < 0;
        ଯଦି (!ମୂଲ୍ୟ_Names.empty()) {
            // A bin of a wildcard array is named by its value (IEEE 1800-2023 19.5.1)
            Ast_ଗଣ୍ଠି_Stmt* stmtsp = nullptr;
            ପାଇଁ (ଅଚଳ std::string& ମୂଲ୍ୟ : ମୂଲ୍ୟ_Names) {
                ଅଚଳ std::string name
                    = VIdProtect::protect_ଶବ୍ଦଗୁଡ଼ିକ_If(binp->name(), prot) + "[" + ମୂଲ୍ୟ + "]";
                stmtsp = Ast_ଗଣ୍ଠି::addNext(
                    stmtsp,
                    itemCall(fl, cpVarp, VCMethod::COVERGROUP_ADD_SINGLE_NAMER,
                             {ctext(fl, binp->binsType().binSetEnum()), ctext(fl, quoted(name)),
                              ctext(fl, quoted(VIdProtect::protectIf(fl->filename(), prot))),
                              cnum(fl, static_cast<uint32_t>(fl->lineno())),
                              cnum(fl, static_cast<uint32_t>(fl->firstColumn()))})
                        ->makeStmt());
            }
            ଫେରନ୍ତୁ stmtsp;
        }
        std::vector<Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି*> args{ctext(fl, binp->binsType().binSetEnum())};
        ଯଦି (!single) args.ଠେଲ_back(cnum(fl, static_cast<uint32_t>(ଗଣନା_2)));  // value array bin
        args.ଠେଲ_back(ctext(fl, quoted(VIdProtect::protect_ଶବ୍ଦଗୁଡ଼ିକ_If(binp->name(), prot))));
        args.ଠେଲ_back(ctext(fl, quoted(VIdProtect::protectIf(fl->filename(), prot))));
        args.ଠେଲ_back(cnum(fl, static_cast<uint32_t>(fl->lineno())));
        args.ଠେଲ_back(cnum(fl, static_cast<uint32_t>(fl->firstColumn())));
        ଫେରନ୍ତୁ itemCall(fl, cpVarp,
                        single ? VCMethod::COVERGROUP_ADD_SINGLE_NAMER
                        : binp->binsType() == VCoverBinsType::BINS_AUTO_IMPLICIT
                            ? VCMethod::COVERGROUP_ADD_NUMBERED_NAMER
                            : VCMethod::COVERGROUP_ADD_ARRAY_NAMER,
                        args)
            ->makeStmt();
    }

    // Emit 'if (iff && cond) m_cp.incrementBin(idx);' (or recordHit, + illegal action) in sample()
    // Where a bin's hit is recorded in the runtime VlCoverpoint member.
    ସଂରଚନା ConvBinTarget final {
        AstVar* cpVarp;  // the __Vcp_<coverpoint> member
        uint32_t idx;  // bin index within that coverpoint
        ତାର୍କିକ isNormal;  // Normal -> incrementBin (count + cross hit list); else recordHit (count)
    };

    // Emit 'this->m_cp.incrementBin(idx);' (Normal) or '.recordHit(idx);'
    // (ignore/illegal/default).
    Ast_ଗଣ୍ଠି_Stmt* makeRuntimeBinHit(File_ଧାଡ଼ି* fl, AstVar* cpVarp, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* idxp,
                                   ତାର୍କିକ isNormal) {
        ଫେରନ୍ତୁ itemCall(fl, cpVarp,
                        isNormal ? VCMethod::COVERGROUP_INCREMENT_BIN
                                 : VCMethod::COVERGROUP_RECORD_HIT,
                        {idxp})
            ->makeStmt();
    }
    Ast_ଗଣ୍ଠି_Stmt* makeRuntimeBinHit(File_ଧାଡ଼ି* fl, ଅଚଳ ConvBinTarget& tgt) {
        ଫେରନ୍ତୁ makeRuntimeBinHit(fl, tgt.cpVarp, cnum(fl, static_cast<uint32_t>(tgt.idx)),
                                 tgt.isNormal);
    }

    // The condition under which a bin counts a sample: condp, and the bin's iff, and for a
    // Normal or default state bin, that the value is not excluded
    Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* binCondition(AstCoverBin* binp, AstVar* cpVarp, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* condp) {
        File_ଧାଡ଼ି* ଅଚଳ fl = binp->fileline();
        ଯଦି (binp->iffp()) condp = ନୂତନ AstLogAnd{fl, binp->iffp()->clone_ବୃକ୍ଷ(ମିଥ୍ୟା), condp};
        ଅଚଳ auto excluded = m_excludedVars.find(cpVarp);
        ଯଦି (excluded != m_excludedVars.end() && !binp->transp()
            && (binp->binsType().binIsNormal()
                || binp->binsType() == VCoverBinsType::BINS_DEFAULT)) {
            condp = ନୂତନ AstLogAnd{
                fl, ନୂତନ AstNot{fl, ନୂତନ AstVarRef{fl, excluded->second, VAccess::READ}}, condp};
        }
        ଫେରନ୍ତୁ condp;
    }

    ଶୂନ୍ୟ emitConvHitIf(AstCoverpoint* coverpointp, AstCoverBin* binp, AstVar* cpVarp,
                       Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* idxp, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* condp) {
        File_ଧାଡ଼ି* ଅଚଳ fl = binp->fileline();
        Ast_ଗଣ୍ଠି* actionp = makeRuntimeBinHit(fl, cpVarp, idxp, binp->binsType().binIsNormal());
        ଯଦି (binp->binsType() == VCoverBinsType::BINS_ILLEGAL) {
            actionp->addNext(makeIllegalBinAction(fl, "Illegal bin " + binp->prettyNameQ()
                                                          + " hit in coverpoint "
                                                          + coverpointp->prettyNameQ()));
        }
        UASSERT_OBJ(m_sampleFuncp, binp, "sample() CFunc not set for coverpoint");
        addSampleStmt(ନୂତନ AstIf{fl, binCondition(binp, cpVarp, condp), actionp, nullptr});
    }

    // Emit the sample() code of sized array 'sized': count the value in its bins holding it when
    // enabled, as other bins are, and for default bins note in matchedp whether any holds it
    ଶୂନ୍ୟ emitSizedSample(AstCoverpoint* coverpointp, AstCoverBin* binp, AstVar* cpVarp,
                         Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* exprp, uint32_t sized, AstVar* matchedp) {
        File_ଧାଡ଼ି* ଅଚଳ fl = binp->fileline();
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* enabledp = binCondition(binp, cpVarp, ନୂତନ AstConst{fl, AstConst::BitTrue{}});
        UASSERT_OBJ(m_sampleFuncp, binp, "sample() CFunc not set for coverpoint");
        ଅଚଳ ତାର୍କିକ illegal = binp->binsType() == VCoverBinsType::BINS_ILLEGAL;
        ଯଦି (illegal) {
            // The illegal action reads the condition too, which is evaluated once, as the guard
            // may have side effects
            AstVar* ଅଚଳ varp
                = ନୂତନ AstVar{fl, VVarType::BLOCKTEMP,
                             "__VcpEnabled_" + sanitizeGeneratedName(coverpointp->name()) + "_"
                                 + cvtToStr(sized),
                             binp->findBitDType()};
            varp->funcLocal(ସତ୍ୟ);
            m_sampleFuncp->addStmtsp(varp);
            addSampleStmt(ନୂତନ AstAssign{fl, ନୂତନ AstVarRef{fl, varp, VAccess::WRITE}, enabledp});
            enabledp = ନୂତନ AstVarRef{fl, varp, VAccess::READ};
        }
        AstCMethodHard* ଅଚଳ callp
            = itemCall(fl, cpVarp,
                       exprp->isWide() ? VCMethod::COVERGROUP_SIZED_SAMPLE_W
                                       : VCMethod::COVERGROUP_SIZED_SAMPLE,
                       {cnum(fl, sized), exprp->clone_ବୃକ୍ଷ(ମିଥ୍ୟା), enabledp});
        callp->dtypeSetBit();
        ଯଦି (illegal) {
            addSampleStmt(ନୂତନ AstIf{fl, ନୂତନ AstLogAnd{fl, callp, enabledp->clone_ବୃକ୍ଷ(ମିଥ୍ୟା)},
                                    makeIllegalBinAction(fl, "Illegal bin " + binp->prettyNameQ()
                                                                 + " hit in coverpoint "
                                                                 + coverpointp->prettyNameQ())});
        } ଅନ୍ୟଥା ଯଦି (matchedp && binp->binsType().binIsNormal()) {
            addSampleStmt(
                ନୂତନ AstAssign{fl, ନୂତନ AstVarRef{fl, matchedp, VAccess::WRITE},
                              ନୂତନ AstOr{fl, ନୂତନ AstVarRef{fl, matchedp, VAccess::READ}, callp}});
        } ଅନ୍ୟଥା {
            addSampleStmt(callp->makeStmt());
        }
    }

    // A variable local to the constructor
    AstVar* constructorTemp(File_ଧାଡ଼ି* fl, ଅଚଳ string& name, Ast_ଗଣ୍ଠି_DType* dtypep) {
        AstVar* ଅଚଳ varp = ନୂତନ AstVar{fl, VVarType::BLOCKTEMP, name, dtypep};
        varp->funcLocal(ସତ୍ୟ);
        m_constructorp->addStmtsp(varp);
        ଫେରନ୍ତୁ varp;
    }

    // Truncate or extend, as its signedness sets, a value to a type
    ସ୍ଥିର Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* resize_ମୂଲ୍ୟ(Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* valuep, Ast_ଗଣ୍ଠି_DType* dtypep) {
        File_ଧାଡ଼ି* ଅଚଳ fl = valuep->fileline();
        ଯଦି (valuep->width() > dtypep->width()) {
            valuep = ନୂତନ AstSel{fl, valuep, 0, dtypep->width()};
        } ଅନ୍ୟଥା ଯଦି (valuep->width() < dtypep->width()) {
            valuep = valuep->isSigned()
                         ? static_cast<Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି*>(ନୂତନ AstExtendS{fl, valuep, dtypep->width()})
                         : ନୂତନ AstExtend{fl, valuep, dtypep->width()};
        }
        valuep->dtypep(dtypep);
        ଫେରନ୍ତୁ valuep;
    }

    // The runtime arguments of a count, which the constructor evaluates once into a temporary:
    // its value, a count wider than 64 bits saturated over any limit, and whether it is positive
    std::pair<Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି*, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି*> new_ଗଣନା_Args(File_ଧାଡ଼ି* fl, ଅଚଳ string& name,
                                                       Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* exprp) {
        AstVar* ଅଚଳ countp = constructorTemp(fl, name, exprp->dtypep());
        m_constructorp->addStmtsp(
            ନୂତନ AstAssign{fl, ନୂତନ AstVarRef{fl, countp, VAccess::WRITE}, exprp->clone_ବୃକ୍ଷ(ମିଥ୍ୟା)});
        ଅଚଳ auto ଗଣନା_Ref = [&]() { ଫେରନ୍ତୁ ନୂତନ AstVarRef{fl, countp, VAccess::READ}; };
        AstConst* ଅଚଳ zerop = ନୂତନ AstConst{fl, AstConst::DTyped{}, countp->dtypep()};
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ positivep
            = countp->isSigned() ? static_cast<Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି*>(ନୂତନ AstGtS{fl, ଗଣନା_Ref(), zerop})
                                 : ନୂତନ AstNeq{fl, ଗଣନା_Ref(), zerop};
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* valuep = resize_ମୂଲ୍ୟ(ଗଣନା_Ref(), countp->findUInt64DType());
        ଯଦି (countp->width() > VL_QUADSIZE) {
            valuep = ନୂତନ AstCond{
                fl,
                ନୂତନ AstRedOr{
                    fl, ନୂତନ AstSel{fl, ଗଣନା_Ref(), VL_QUADSIZE, countp->width() - VL_QUADSIZE}},
                ନୂତନ AstConst{fl, AstConst::Unsized64{}, std::numeric_limits<uint64_t>::max()},
                valuep};
            valuep->dtypeSetUInt64();
        }
        ଫେରନ୍ତୁ {valuep, positivep};
    }
    // The runtime arguments of a constant count
    ସ୍ଥିର std::pair<Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି*, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି*> new_ଗଣନା_Args(File_ଧାଡ଼ି* fl, uint64_t ଗଣନା_2) {
        ଫେରନ୍ତୁ {ନୂତନ AstConst{fl, AstConst::Unsized64{}, ଗଣନା_2},
                ନୂତନ AstConst{fl, AstConst::BitTrue{}}};
    }

    // The values of a coverpoint, which its name denotes (IEEE 1800-2023 19.5.1.1), as runs in
    // value order: an enumerated type's values (6.19), else all values of its type.  Bounds
    // are at runWidth(), sign-extended like a CrossValueRange's.
    ସ୍ଥିର std::vector<std::pair<V3_ସଂଖ୍ୟା, V3_ସଂଖ୍ୟା>> coverpoint_ମୂଲ୍ୟଗୁଡ଼ିକ(Ast_ଗଣ୍ଠି* nodep,
                                                                       Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* exprp) {
        ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା width = runWidth(exprp);
        std::vector<std::pair<V3_ସଂଖ୍ୟା, V3_ସଂଖ୍ୟା>> runs;
        ଅଚଳ AstEnumDType* ଅଚଳ enump = VN_CAST(exprp->dtypep()->skipRefToEnump(), EnumDType);
        ଯଦି (!enump) {
            ଅଚଳ Cross_ମୂଲ୍ୟ_Range domain
                = cross_ମୂଲ୍ୟ_Domain(nodep, exprp->width(), exprp->isSigned(), width);
            runs.emplace_back(domain.lo, domain.hi);
            ଫେରନ୍ତୁ runs;
        }
        std::vector<V3_ସଂଖ୍ୟା> ମୂଲ୍ୟଗୁଡ଼ିକ;
        ପାଇଁ (ଅଚଳ AstEnumItem* itemp = enump->itemsp(); itemp;
             itemp = VN_AS(itemp->nextp(), EnumItem)) {
            ଅଚଳ V3_ସଂଖ୍ୟା& ଅଙ୍କ = VN_AS(itemp->valuep(), Const)->ଅଙ୍କ();
            ଯଦି (ଅଙ୍କ.isFourState()) ଜାରି;  // Not a coverpoint value (19.5.7)
            ମୂଲ୍ୟଗୁଡ଼ିକ.emplace_back(nodep, width);
            ଯଦି (exprp->isSigned()) {
                ମୂଲ୍ୟଗୁଡ଼ିକ.back().opExtendS(ଅଙ୍କ, ଅଙ୍କ.width());
            } ଅନ୍ୟଥା {
                ମୂଲ୍ୟଗୁଡ଼ିକ.back().opAssign(ଅଙ୍କ);
            }
        }
        std::sort(ମୂଲ୍ୟଗୁଡ଼ିକ.begin(), ମୂଲ୍ୟଗୁଡ଼ିକ.end(), cross_ମୂଲ୍ୟ_Less);
        ଅଚଳ V3_ସଂଖ୍ୟା one{nodep, width, 1};
        ପାଇଁ (ଅଚଳ V3_ସଂଖ୍ୟା& ମୂଲ୍ୟ : ମୂଲ୍ୟଗୁଡ଼ିକ) {
            V3_ସଂଖ୍ୟା next{nodep, width};
            ଯଦି (!runs.empty() && next.opAdd(runs.back().second, one).isCaseEq(ମୂଲ୍ୟ)) {
                runs.back().second = ମୂଲ୍ୟ;
            } ଅନ୍ୟଥା {
                runs.emplace_back(ମୂଲ୍ୟ, ମୂଲ୍ୟ);
            }
        }
        ଫେରନ୍ତୁ runs;
    }

    // Emit the constructor code building the bins 'binp' whose values it computes: the values of
    // each element that are coverpoint values (IEEE 1800-2023 19.5.7), those a 'with' filter
    // keeps (19.5.1.1), then its bins
    ଶୂନ୍ୟ generateConstructedBins(AstCoverpoint* coverpointp, AstCoverBin* binp, AstVar* cpVarp,
                                 Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* exprp) {
        File_ଧାଡ଼ି* ଅଚଳ fl = binp->fileline();
        ଅଚଳ string prefix
            = m_sizedNames.get(sanitizeGeneratedName(coverpointp->name() + "__" + binp->name()));
        // The count, and whether it is positive; one of a filter's scalar bin, or bin per value
        ଅଚଳ std::pair<Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି*, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି*> ଗଣନା_2
            = binp->arraySizep() ? new_ଗଣନା_Args(fl, prefix + "_count", binp->arraySizep())
                                 : new_ଗଣନା_Args(fl, 1);
        AstCoverWith* ଅଚଳ withp = binWith(binp);
        ଯଦି (withp && !binRangesp(binp)) {  // The coverpoint's name: all of its values
            ପାଇଁ (ଅଚଳ std::pair<V3_ସଂଖ୍ୟା, V3_ସଂଖ୍ୟା>& run : coverpoint_ମୂଲ୍ୟଗୁଡ଼ିକ(binp, exprp)) {
                m_constructorp->addStmtsp(itemCall(fl, cpVarp,
                                                   exprp->isWide()
                                                       ? VCMethod::COVERGROUP_SIZED_RANGE_W
                                                       : VCMethod::COVERGROUP_SIZED_RANGE,
                                                   {new_ମୂଲ୍ୟ_Const(fl, run.first, exprp),
                                                    new_ମୂଲ୍ୟ_Const(fl, run.second, exprp)})
                                              ->makeStmt());
            }
        }
        uint32_t element = 0;
        ପାଇଁ (Ast_ଗଣ୍ଠି* rangep = binRangesp(binp); rangep; rangep = rangep->nextp()) {
            ଯଦି (VN_IS(rangep, Unbounded)) {  // A parameter of '$'
                binp->v3error("Bins value may not be '$', which may only bound a range "
                              "(IEEE 1800-2023 6.20.7)");
                ଜାରି;
            }
            generateSizedElement(cpVarp, binp, rangep, exprp, prefix + "_" + cvtToStr(element++));
        }
        ଯଦି (withp) generateWithFilter(binp, withp, cpVarp, exprp, prefix);
        std::vector<Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି*> args{ctext(fl, binp->binsType().binSetEnum()), ଗଣନା_2.first,
                                       ଗଣନା_2.second};
        // A filter's bins have the limit it began with
        ଯଦି (!withp) args.ଠେଲ_back(cnum(fl, v3Global.opt.coverageMaxBins()));
        ଅଚଳ ତାର୍କିକ prot = v3Global.opt.protectIds();
        args.ଠେଲ_back(ctext(fl, quoted(VIdProtect::protect_ଶବ୍ଦଗୁଡ଼ିକ_If(binp->name(), prot))));
        args.ଠେଲ_back(ctext(fl, quoted(VIdProtect::protectIf(fl->filename(), prot))));
        args.ଠେଲ_back(cnum(fl, static_cast<uint32_t>(fl->lineno())));
        args.ଠେଲ_back(cnum(fl, static_cast<uint32_t>(fl->firstColumn())));
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
    ଶୂନ୍ୟ generateWithFilter(AstCoverBin* binp, AstCoverWith* withp, AstVar* cpVarp,
                            Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* exprp, ଅଚଳ string& prefix) {
        File_ଧାଡ଼ି* ଅଚଳ fl = withp->fileline();
        ଅଚଳ ତାର୍କିକ wide = exprp->isWide();
        ଅଚଳ string grouping = !binp->isArray()     ? "Single"
                                : binp->arraySizep() ? "Fixed"
                                                     : "Values";
        m_constructorp->addStmtsp(itemCall(fl, cpVarp, VCMethod::COVERGROUP_WITH_BEGIN,
                                           {ctext(fl, "VlCovBinGrouping::" + grouping),
                                            cnum(fl, v3Global.opt.coverageMaxBins())})
                                      ->makeStmt());
        // The candidates count in the coverpoint's width, and the filter reads each as 'item',
        // of the coverpoint's type, so that a filter changing 'item' cannot change the loop
        Ast_ଗଣ୍ଠି_DType* ଅଚଳ ମୂଲ୍ୟ_DTypep = exprp->dtypep();
        AstVar* ଅଚଳ valuep = constructorTemp(fl, prefix + "_value", ମୂଲ୍ୟ_DTypep);
        AstVar* ଅଚଳ lastp = constructorTemp(fl, prefix + "_last", ମୂଲ୍ୟ_DTypep);
        AstVar* ଅଚଳ firstp = constructorTemp(fl, prefix + "_first", ମୂଲ୍ୟ_DTypep);
        AstVar* ଅଚଳ runp = constructorTemp(fl, prefix + "_run", binp->findBitDType());
        AstVar* ଅଚଳ morep = constructorTemp(fl, prefix + "_more", binp->findBitDType());
        AstVar* ଅଚଳ itemp = withp->itemp()->unlinkFrBack();
        itemp->name(prefix + "_item");
        m_constructorp->addStmtsp(itemp);
        ଅଚଳ auto ref = [&](AstVar* varp) { ଫେରନ୍ତୁ ନୂତନ AstVarRef{fl, varp, VAccess::READ}; };
        ଅଚଳ auto assign = [&](AstVar* varp, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* rhsp) -> Ast_ଗଣ୍ଠି* {
            ଫେରନ୍ତୁ ନୂତନ AstAssign{fl, ନୂତନ AstVarRef{fl, varp, VAccess::WRITE}, rhsp};
        };
        ଅଚଳ auto flag = [&](AstVar* varp, ତାର୍କିକ ମୂଲ୍ୟ) {
            ଫେରନ୍ତୁ assign(varp, ମୂଲ୍ୟ ? ନୂତନ AstConst{fl, AstConst::BitTrue{}}
                                      : ନୂତନ AstConst{fl, AstConst::BitFalse{}});
        };
        ଅଚଳ auto bound = [&](VCMethod narrow, VCMethod wideMethod, AstVar* varp) -> Ast_ଗଣ୍ଠି* {
            ଯଦି (wide) {
                ଫେରନ୍ତୁ itemCall(fl, cpVarp, wideMethod, {ନୂତନ AstVarRef{fl, varp, VAccess::WRITE}})
                    ->makeStmt();
            }
            AstCMethodHard* ଅଚଳ callp = itemCall(fl, cpVarp, narrow);
            callp->dtypeSetUInt64();
            ଫେରନ୍ତୁ assign(varp, resize_ମୂଲ୍ୟ(callp, ମୂଲ୍ୟ_DTypep));
        };
        ଅଚଳ auto keep = [&](Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* lop, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* hip) {
            AstCMethodHard* ଅଚଳ callp = itemCall(
                fl, cpVarp, wide ? VCMethod::COVERGROUP_WITH_RUN_W : VCMethod::COVERGROUP_WITH_RUN,
                {lop, hip});
            callp->dtypeSetBit();
            ଫେରନ୍ତୁ assign(morep, callp);
        };
        ଅଚଳ auto step = [&](ତାର୍କିକ up) {
            AstConst* ଅଚଳ onep = ନୂତନ AstConst{fl, AstConst::Widthed_ମୂଲ୍ୟ{}, exprp->width(), 1};
            Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ stepp
                = up ? static_cast<Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି*>(ନୂତନ AstAdd{fl, ref(valuep), onep})
                     : ନୂତନ AstSub{fl, ref(valuep), onep};
            stepp->dtypep(ମୂଲ୍ୟ_DTypep);
            ଫେରନ୍ତୁ stepp;
        };
        AstLoop* ଅଚଳ innerp = ନୂତନ AstLoop{fl};
        innerp->addStmtsp(assign(itemp, ref(valuep)));
        innerp->addStmtsp(ନୂତନ AstIf{
            fl, withp->filterp()->unlinkFrBack(),
            ନୂତନ AstIf{fl, ନୂତନ AstNot{fl, ref(runp)},
                      assign(firstp, ref(valuep))->addNext(flag(runp, ସତ୍ୟ))},
            ନୂତନ AstIf{fl, ref(runp), keep(ref(firstp), step(ମିଥ୍ୟା))->addNext(flag(runp, ମିଥ୍ୟା))}});
        innerp->addStmtsp(ନୂତନ AstLoopTest{
            fl, innerp, ନୂତନ AstLogAnd{fl, ref(morep), ନୂତନ AstNeq{fl, ref(valuep), ref(lastp)}}});
        innerp->addStmtsp(assign(valuep, step(ସତ୍ୟ)));
        AstLoop* ଅଚଳ outerp = ନୂତନ AstLoop{fl};
        AstCMethodHard* ଅଚଳ nextp = itemCall(fl, cpVarp, VCMethod::COVERGROUP_WITH_NEXT);
        nextp->dtypeSetBit();
        outerp->addStmtsp(ନୂତନ AstLoopTest{fl, outerp, nextp});
        outerp->addStmtsp(
            bound(VCMethod::COVERGROUP_WITH_LO, VCMethod::COVERGROUP_WITH_LO_W, valuep));
        outerp->addStmtsp(
            bound(VCMethod::COVERGROUP_WITH_HI, VCMethod::COVERGROUP_WITH_HI_W, lastp));
        outerp->addStmtsp(flag(runp, ମିଥ୍ୟା));
        outerp->addStmtsp(innerp);
        outerp->addStmtsp(ନୂତନ AstIf{fl, ref(runp), keep(ref(firstp), ref(lastp))});
        m_constructorp->addStmtsp(flag(morep, ସତ୍ୟ));
        m_constructorp->addStmtsp(outerp);
    }

    // Emit 'sizedRange(lo, hi)' for the coverpoint values of an element of bins 'binp', whose
    // values the constructor computes: resolved now if constant, else when constructed by
    // clipping to the coverpoint's values.  A wildcard pattern's values give a range for each
    // run of them.  'prefix' names its temporaries.
    ଶୂନ୍ୟ generateSizedElement(AstVar* cpVarp, ଅଚଳ AstCoverBin* binp, Ast_ଗଣ୍ଠି* rangep,
                              Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* exprp, ଅଚଳ string& prefix) {
        File_ଧାଡ଼ି* ଅଚଳ fl = rangep->fileline();
        ଅଚଳ VCMethod method = exprp->isWide() ? VCMethod::COVERGROUP_SIZED_RANGE_W
                                                : VCMethod::COVERGROUP_SIZED_RANGE;
        ଅଚଳ AstInsideRange* ଅଚଳ irp = VN_CAST(rangep, InsideRange);
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ lowp = irp ? irp->lhsp() : VN_AS(rangep, ଗଣ୍ଠି_ପଦସମଷ୍ଟି);
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ highp = irp ? irp->rhsp() : nullptr;
        ଅଚଳ auto unbounded
            = [](ଅଚଳ Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* boundp) { ଫେରନ୍ତୁ !boundp || VN_IS(boundp, Unbounded); };
        ଅଚଳ auto constant
            = [&](ଅଚଳ Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* boundp) { ଫେରନ୍ତୁ unbounded(boundp) || VN_IS(boundp, Const); };
        ଅଚଳ auto integral = [&](ଅଚଳ Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* boundp) {
            ଫେରନ୍ତୁ unbounded(boundp) || boundp->dtypep()->skipRefp()->isIntegralOrPacked();
        };
        ଯଦି (constant(lowp) && constant(highp)) {
            ଅଚଳ auto fourState = [](ଅଚଳ Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* boundp) {
                ଅଚଳ AstConst* ଅଚଳ constp = VN_CAST(boundp, Const);
                ଫେରନ୍ତୁ constp && constp->ଅଙ୍କ().isFourState();
            };
            ଯଦି (irp && (fourState(lowp) || fourState(highp))) {
                rangep->v3error("Four-state (x/z) value in "
                                << (binp->isArray() ? "array bins" : "bin")
                                << " range bound; range bounds must be two-state constants");
                ଫେରନ୍ତୁ;
            }
            Cross_ମୂଲ୍ୟ_Range range{rangep, resolveWidth(rangep, exprp)};
            ଯଦି (!resolve_ମୂଲ୍ୟ(rangep, exprp, ସତ୍ୟ, binp->isWildcard(), range)) {
                rangep->v3warn(E_UNSUPPORTED, "Unsupported: non-integral value in a coverage bin "
                                              "of an integral coverpoint.");
            } ଅନ୍ୟଥା ଯଦି (!crossRangeEmpty(range)) {
                std::vector<std::pair<V3_ସଂଖ୍ୟା, V3_ସଂଖ୍ୟା>> runs{{range.lo, range.hi}};
                ଯଦି (range.wildcard) {  // checkConstructedBins bounded the runs
                    runs.clear();
                    crossRangeRuns(range, v3Global.opt.coverageMaxBins(), runs);
                }
                ପାଇଁ (ଅଚଳ std::pair<V3_ସଂଖ୍ୟା, V3_ସଂଖ୍ୟା>& run : runs) {
                    m_constructorp->addStmtsp(itemCall(fl, cpVarp, method,
                                                       {new_ମୂଲ୍ୟ_Const(fl, run.first, exprp),
                                                        new_ମୂଲ୍ୟ_Const(fl, run.second, exprp)})
                                                  ->makeStmt());
                }
            }
            ଫେରନ୍ତୁ;
        }
        ଯଦି (!integral(lowp) || !integral(highp)) {
            rangep->v3warn(E_UNSUPPORTED, "Unsupported: non-integral value in a coverage bin "
                                          "of an integral coverpoint.");
            ଫେରନ୍ତୁ;
        }
        // Compare values and the coverpoint's domain signed, in a width holding all of them
        ଅଚଳ auto boundWidth
            = [&](ଅଚଳ Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* boundp) { ଫେରନ୍ତୁ unbounded(boundp) ? 0 : boundp->width(); };
        ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା width = std::max({exprp->width(), boundWidth(lowp), boundWidth(highp)}) + 1;
        ଅଚଳ Cross_ମୂଲ୍ୟ_Range domain
            = cross_ମୂଲ୍ୟ_Domain(rangep, exprp->width(), exprp->isSigned(), width);
        AstVar* ଅଚଳ lop = constructorTemp(fl, prefix + "_lo", exprp->dtypep());
        lop->dtypeSetBitSized(width, VSigning::SIGNED);
        AstVar* ଅଚଳ hip = constructorTemp(fl, prefix + "_hi", lop->dtypep());
        ଅଚଳ auto bound = [&](Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* boundp, ଅଚଳ V3_ସଂଖ୍ୟା& limit) -> Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* {
            ଯଦି (unbounded(boundp)) {
                AstConst* ଅଚଳ limitp = ନୂତନ AstConst{fl, limit};
                limitp->dtypeFrom(lop);
                ଫେରନ୍ତୁ limitp;
            }
            Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* valuep = boundp->clone_ବୃକ୍ଷ(ମିଥ୍ୟା);
            // A value no wider than a signed coverpoint has its type (see crossRangeBound)
            ଯଦି (exprp->isSigned() && boundp->width() <= exprp->width()) {
                valuep = resize_ମୂଲ୍ୟ(valuep, exprp->dtypep());
            }
            ଫେରନ୍ତୁ resize_ମୂଲ୍ୟ(valuep, lop->dtypep());
        };
        ଅଚଳ auto ref = [&](AstVar* varp, VAccess access = VAccess::READ) {
            ଫେରନ୍ତୁ ନୂତନ AstVarRef{fl, varp, access};
        };
        m_constructorp->addStmtsp(
            ନୂତନ AstAssign{fl, ref(lop, VAccess::WRITE), bound(lowp, domain.lo)});
        m_constructorp->addStmtsp(
            ନୂତନ AstAssign{fl, ref(hip, VAccess::WRITE), irp ? bound(highp, domain.hi) : ref(lop)});
        AstConst* ଅଚଳ minp = ନୂତନ AstConst{fl, domain.lo};
        AstConst* ଅଚଳ maxp = ନୂତନ AstConst{fl, domain.hi};
        minp->dtypeFrom(lop);
        maxp->dtypeFrom(lop);
        AstCond* ଅଚଳ lowerp
            = ନୂତନ AstCond{fl, ନୂତନ AstLtS{fl, ref(lop), minp}, minp->clone_ବୃକ୍ଷ(ମିଥ୍ୟା), ref(lop)};
        AstCond* ଅଚଳ upperp
            = ନୂତନ AstCond{fl, ନୂତନ AstGtS{fl, ref(hip), maxp}, maxp->clone_ବୃକ୍ଷ(ମିଥ୍ୟା), ref(hip)};
        lowerp->dtypeFrom(lop);
        upperp->dtypeFrom(lop);
        m_constructorp->addStmtsp(ନୂତନ AstAssign{fl, ref(lop, VAccess::WRITE), lowerp});
        m_constructorp->addStmtsp(ନୂତନ AstAssign{fl, ref(hip, VAccess::WRITE), upperp});
        m_constructorp->addStmtsp(ନୂତନ AstIf{fl, ନୂତନ AstLteS{fl, ref(lop), ref(hip)},
                                            itemCall(fl, cpVarp, method,
                                                     {resize_ମୂଲ୍ୟ(ref(lop), exprp->dtypep()),
                                                      resize_ମୂଲ୍ୟ(ref(hip), exprp->dtypep())})
                                                ->makeStmt()});
    }

    // The runtime index of the bin of a run holding the coverpoint value, which is in the run:
    // declared + (value - lo) / stride, capped at the last bin, which holds any remainder.
    ସ୍ଥିର Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* runBinIndex(File_ଧାଡ଼ି* fl, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* exprp, ଅଚଳ BinRun& run) {
        ଯଦି (run.m_ଗଣନା == 1) ଫେରନ୍ତୁ cnum(fl, run.m_declared);
        ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା width = exprp->width();
        // A run spans at most 2^width values, so offsets in it are unsigned width-bit numbers
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* indexp
            = ନୂତନ AstSub{fl, exprp->clone_ବୃକ୍ଷ(ମିଥ୍ୟା), new_ମୂଲ୍ୟ_Const(fl, run.m_lo, exprp)};
        indexp->dtypeSetBitSized(width, VSigning::UNSIGNED);
        V3_ସଂଖ୍ୟା stride{fl, width, 0};
        stride.opAssign(run.m_stride);
        ଯଦି (stride.ଗଣନା_Ones() != 1) {
            indexp = ନୂତନ AstDiv{fl, indexp, ନୂତନ AstConst{fl, stride}};
        } ଅନ୍ୟଥା ଯଦି (!stride.isEqOne()) {
            indexp = ନୂତନ AstShiftR{fl, indexp, ନୂତନ AstConst{fl, stride.mostSetBitP1() - 1}};
        }
        // Compare the run's values with those of count bins of stride values, without overflow
        ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା extWidth = run.m_lo.width() + 1;
        V3_ସଂଖ୍ୟା lo{fl, extWidth, 0};
        lo.opExtendS(run.m_lo, run.m_lo.width());
        V3_ସଂଖ୍ୟା span{fl, extWidth, 0};
        span.opExtendS(run.m_hi, run.m_hi.width());
        span.opSub(V3_ସଂଖ୍ୟା{span}, lo);
        V3_ସଂଖ୍ୟା covered{fl, extWidth, 0};
        covered.opAssign(run.m_stride);
        covered.opMul(V3_ସଂଖ୍ୟା{covered}, V3_ସଂଖ୍ୟା{fl, extWidth, run.m_ଗଣନା});
        V3_ସଂଖ୍ୟା hasRemainder{fl, 1, 0};
        ଯଦି (!hasRemainder.opGte(span, covered).isEqZero()) {
            AstConst* ଅଚଳ lastp = ନୂତନ AstConst{fl, V3_ସଂଖ୍ୟା{fl, width, run.m_ଗଣନା - 1}};
            indexp = ନୂତନ AstCond{fl, ନୂତନ AstGt{fl, indexp->clone_ବୃକ୍ଷ(ମିଥ୍ୟା), lastp},
                                 lastp->clone_ବୃକ୍ଷ(ମିଥ୍ୟା), indexp};
        }
        ଯଦି (width < VL_IDATASIZE) {
            indexp = ନୂତନ AstExtend{fl, indexp, VL_IDATASIZE};
        } ଅନ୍ୟଥା ଯଦି (width > VL_IDATASIZE) {
            indexp = ନୂତନ AstSel{fl, indexp, 0, VL_IDATASIZE};
        }
        ଫେରନ୍ତୁ ନୂତନ AstAdd{fl, cnum(fl, run.m_declared), indexp};
    }

    // Emit the sample() hit of a run of bins, whose code does not grow with its number of bins:
    //   if (iff && lo <= value && value <= hi) m_cp.incrementBin(<runBinIndex>);
    ଶୂନ୍ୟ emitRunHit(AstCoverpoint* coverpointp, AstCoverBin* binp, AstVar* cpVarp,
                    Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* exprp, ଅଚଳ BinRun& run) {
        File_ଧାଡ଼ି* ଅଚଳ fl = binp->fileline();
        AstConst* ଅଚଳ lop = new_ମୂଲ୍ୟ_Const(fl, run.m_lo, exprp);
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* condp = nullptr;
        ଯଦି (run.m_lo.isCaseEq(run.m_hi)) {
            condp = ନୂତନ AstEq{fl, exprp->clone_ବୃକ୍ଷ(ମିଥ୍ୟା), lop};
        } ଅନ୍ୟଥା {
            AstConst* ଅଚଳ hip = new_ମୂଲ୍ୟ_Const(fl, run.m_hi, exprp);
            condp = makeRangeCondition(fl, exprp, lop, hip);
            VL_DO_DANGLING(ଠେଲ_Deletep(lop), lop);
            VL_DO_DANGLING(ଠେଲ_Deletep(hip), hip);
        }
        emitConvHitIf(coverpointp, binp, cpVarp, runBinIndex(fl, exprp, run), condp);
    }

    // Emit a transition bin's hit action into sample():
    //   if (cond) { m_cp.incrementBin/recordHit(idx); [illegal: $error; $stop] }
    // Used by the transition generators so a completed sequence records into the runtime bin.
    ଶୂନ୍ୟ addConvTransHitIf(AstCoverpoint* coverpointp, AstCoverBin* binp, ଅଚଳ ConvBinTarget& tgt,
                           Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* condp) {
        File_ଧାଡ଼ି* ଅଚଳ fl = binp->fileline();
        Ast_ଗଣ୍ଠି* actionp = makeRuntimeBinHit(fl, tgt);
        ଯଦି (binp->binsType() == VCoverBinsType::BINS_ILLEGAL) {
            actionp->addNext(makeIllegalBinAction(
                fl, "Illegal transition bin " + binp->prettyNameQ() + " hit in coverpoint "
                        + coverpointp->prettyNameQ()));
        }
        UASSERT_OBJ(m_sampleFuncp, binp, "sample() CFunc not set for transition bin");
        addSampleStmt(ନୂତନ AstIf{fl, condp, actionp, nullptr});
    }

    // Route a coverpoint through a VlCoverpoint member: emit the member, its sample()
    // increments, the constructor configuration (init + namers), and registration.
    ଶୂନ୍ୟ generateCoverpoint(AstCoverpoint* coverpointp, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* exprp, ପୂର୍ଣ୍ଣସଂଖ୍ୟା at_Least_ମୂଲ୍ୟ) {
        File_ଧାଡ଼ି* ଅଚଳ fl = coverpointp->fileline();
        ଅଚଳ ତାର୍କିକ dynamic = m_runtimePoints.ଗଣନା_2(coverpointp);
        UINFO(4, "  Generating VlCoverpoint member: " << coverpointp->name());

        // Size the hit list to the gen-time max bin overlap (1 unless cross-fed with
        // overlapping ranges), so no cross hit is ever dropped and storage is minimal.
        ଅଚଳ ତାର୍କିକ crossFed = m_crossedCpNames.ଗଣନା_2(coverpointp->name()) != 0;
        ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା hitBound = computeHitListBound(coverpointp, exprp, crossFed);
        UINFO(6, "    Hit-list bound (max bin overlap) = " << hitBound);
        AstVar* ଅଚଳ cpVarp = ନୂତନ AstVar{fl, VVarType::MEMBER, "__Vcp_" + coverpointp->name(),
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
        ଯଦି (crossFed) {
            UASSERT_OBJ(m_sampleFuncp, coverpointp, "sample() CFunc not set for clearHitList");
            m_sampleFuncp->addStmtsp(
                itemCall(fl, cpVarp, VCMethod::COVERGROUP_CLEAR_HIT_LIST)->makeStmt());
        }
        ଯଦି (dynamic && coverpointHasStateExclusions(coverpointp)) {
            AstVar* ଅଚଳ excludedp
                = ନୂତନ AstVar{fl, VVarType::BLOCKTEMP,
                             "__VcpExcluded_" + sanitizeGeneratedName(coverpointp->name()),
                             coverpointp->findBitDType()};
            excludedp->funcLocal(ସତ୍ୟ);
            m_sampleFuncp->addStmtsp(excludedp);
            AstCMethodHard* ଅଚଳ callp
                = itemCall(fl, cpVarp,
                           exprp->isWide() ? VCMethod::COVERGROUP_ମୂଲ୍ୟ_EXCLUDED_W
                                           : VCMethod::COVERGROUP_ମୂଲ୍ୟ_EXCLUDED,
                           {exprp->clone_ବୃକ୍ଷ(ମିଥ୍ୟା)});
            callp->dtypeSetBit();
            addSampleStmt(ନୂତନ AstAssign{fl, ନୂତନ AstVarRef{fl, excludedp, VAccess::WRITE}, callp});
            m_excludedVars.emplace(cpVarp, excludedp);
        }

        // Walk bins (non-default, then default), assigning sequential indices that match the
        // namer append order; emit sample increments and collect namer statements.  Constructed
        // bins follow them all, placed when the coverpoint is constructed.
        std::vector<Ast_ଗଣ୍ଠି_Stmt*> namerStmts;
        std::vector<AstCoverBin*> defaultBins;
        std::vector<AstCoverBin*> sizedBins;
        std::vector<std::tuple<AstCoverBin*, uint32_t, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି*>> metadata;
        std::vector<ଅଚଳ BinRun*> runMetadata;
        uint64_t idx = 0;  // Runtime index of the next bin; 32-bit once checked below
        ପାଇଁ (Ast_ଗଣ୍ଠି* binp = coverpointp->binsp(); binp; binp = binp->nextp()) {
            AstCoverBin* ଅଚଳ cbinp = VN_AS(binp, CoverBin);
            ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା errorsBefore = dynamic ? V3Error::error_ଗଣନା() : 0;
            ଯଦି (cbinp->binsType() == VCoverBinsType::BINS_DEFAULT) {
                defaultBins.ଠେଲ_back(cbinp);
                ଜାରି;
            }
            ଯଦି (isConstructedBins(cbinp)) {
                UASSERT_OBJ(dynamic, cbinp, "Constructed bins without value metadata");
                BinSpan span;
                span.sized = static_cast<int32_t>(sizedBins.size());
                m_cpBins.at(cpVarp).spans.emplace(cbinp->name(), span);
                sizedBins.ଠେଲ_back(cbinp);
                ଜାରି;
            }
            ଯଦି (cbinp->transp()) {
                // Transition bin (incl. array transition 'bins t[] = (a=>b),(c=>d)' and
                // illegal_bins/ignore_bins transitions).  All sequences of one transition bin
                // share a bin name and merge in the coverage DB to a single point, so model
                // them as one runtime bin incremented by any matching sequence.  The sequence
                // matching is generated as a state machine, with the hit routed to this bin's
                // runtime slot.
                namerStmts.ଠେଲ_back(makeNamer(cpVarp, cbinp, -1, static_cast<uint32_t>(idx)));
                ଅଚଳ ConvBinTarget tgt{cpVarp, static_cast<uint32_t>(idx),
                                        cbinp->binsType().binIsNormal()};
                ପାଇଁ (Ast_ଗଣ୍ଠି* sp = cbinp->transp(); sp; sp = sp->nextp())
                    generateSingleTransitionCode(coverpointp, cbinp, exprp, tgt,
                                                 VN_AS(sp, CoverTransSet));
                ଯଦି (dynamic && V3Error::error_ଗଣନା() == errorsBefore) {
                    metadata.emplace_back(cbinp, static_cast<uint32_t>(idx), nullptr);
                }
                ++idx;
                ଜାରି;
            }
            BinRuns plan;
            ଯଦି (binRunsFor(cbinp, exprp, plan)) {
                // Array elements and automatic bins generate as runs, so neither sample() nor
                // the constructor grows with their number of bins.
                ଯଦି (plan.unsupported) {  // bin ignored or invalid; reserve no slot
                    m_droppedBins[coverpointp].ଠେଲ_back(cbinp->name());
                    ଜାରି;
                }
                CoverpointBins& bins = m_cpBins.at(cpVarp);
                ଅଚଳ uint32_t first_ମୂଲ୍ୟ = bins.total;
                ଅଚଳ uint32_t firstDeclared = static_cast<uint32_t>(idx);
                namerStmts.ଠେଲ_back(
                    makeNamer(cpVarp, cbinp, plan.ଗଣନା_2, firstDeclared, {}, plan.ମୂଲ୍ୟଗୁଡ଼ିକ));
                ପାଇଁ (BinRun& run : plan.runs) {
                    run.m_declared = static_cast<uint32_t>(idx);
                    bins.runs.ଠେଲ_back(std::move(run));
                    ଅଚଳ BinRun& stored = bins.runs.back();
                    ଯଦି (bins.crossed && cbinp->binsType().binIsNormal()) {
                        ଅଚଳ uint32_t first = first_ମୂଲ୍ୟ + stored.m_declared - firstDeclared;
                        ପାଇଁ (uint32_t element = 0; element < stored.m_ଗଣନା; ++element) {
                            bins.ମୂଲ୍ୟଗୁଡ଼ିକ[first + element].runp = &stored;
                            bins.ମୂଲ୍ୟଗୁଡ଼ିକ[first + element].element = element;
                        }
                    }
                    ଯଦି (!stored.m_empty) {
                        emitRunHit(coverpointp, cbinp, cpVarp, exprp, stored);
                        ଯଦି (dynamic && V3Error::error_ଗଣନା() == errorsBefore) {
                            runMetadata.ଠେଲ_back(&stored);
                        }
                    }
                    idx += stored.m_ଗଣନା;
                }
                ଜାରି;
            }
            ଯଦି (cbinp->isArray()) {  // value array of a real coverpoint: b[0]..b[N-1]
                // Only integral coverpoints have runtime value metadata (m_runtimePoints)
                UASSERT_OBJ(!dynamic, cbinp, "Runtime value metadata for a real coverpoint");
                ତାର୍କିକ unsupported = ମିଥ୍ୟା;
                std::vector<Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି*> ମୂଲ୍ୟଗୁଡ଼ିକ = extract_Array_ମୂଲ୍ୟଗୁଡ଼ିକ(cbinp, exprp, unsupported);
                ଯଦି (unsupported) {  // bin ignored (COVERIGN emitted); reserve no slot
                    m_droppedBins[coverpointp].ଠେଲ_back(cbinp->name());
                    ଜାରି;
                }
                namerStmts.ଠେଲ_back(makeNamer(cpVarp, cbinp, static_cast<int64_t>(ମୂଲ୍ୟଗୁଡ଼ିକ.size()),
                                               static_cast<uint32_t>(idx), ମୂଲ୍ୟଗୁଡ଼ିକ));
                ପାଇଁ (Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* valuep : ମୂଲ୍ୟଗୁଡ଼ିକ) {
                    // The cross selections of this covergroup still read the value.
                    m_detached_ମୂଲ୍ୟଗୁଡ଼ିକ.ଠେଲ_back(valuep);
                    emitConvHitIf(coverpointp, cbinp, cpVarp,
                                  cnum(cbinp->fileline(), static_cast<uint32_t>(idx)),
                                  build_ମୂଲ୍ୟ_Condition(cbinp, exprp, valuep));
                    ++idx;
                }
            } ଅନ୍ୟଥା {
                namerStmts.ଠେଲ_back(makeNamer(cpVarp, cbinp, -1, static_cast<uint32_t>(idx)));
                // buildBinCondition is null for 'ignore_bins = default' (no ranges); the bin
                // still gets a reserved slot (recorded, never incremented).
                ଯଦି (Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ condp = buildBinCondition(cbinp, exprp))
                    emitConvHitIf(coverpointp, cbinp, cpVarp,
                                  cnum(cbinp->fileline(), static_cast<uint32_t>(idx)), condp);
                ଯଦି (dynamic && V3Error::error_ଗଣନା() == errorsBefore) {
                    metadata.emplace_back(cbinp, static_cast<uint32_t>(idx), nullptr);
                }
                ++idx;
            }
        }
        // A cross selecting an ignored bins declaration selects no bins
        ପାଇଁ (ଅଚଳ std::string& name : m_droppedBins[coverpointp]) {
            m_cpBins.at(cpVarp).spans.emplace(name, BinSpan{});
        }
        // A value a constructed bin holds is no default bin's; only sampling tells which do
        AstVar* sizedMatchedp = nullptr;
        ଯଦି (!defaultBins.empty()
            && std::any_of(sizedBins.begin(), sizedBins.end(), [](ଅଚଳ AstCoverBin* binp) {
                   ଫେରନ୍ତୁ binp->binsType().binIsNormal();
               })) {
            sizedMatchedp = ନୂତନ AstVar{fl, VVarType::BLOCKTEMP,
                                       "__VcpSized_" + sanitizeGeneratedName(coverpointp->name()),
                                       coverpointp->findBitDType()};
            sizedMatchedp->funcLocal(ସତ୍ୟ);
            m_sampleFuncp->addStmtsp(sizedMatchedp);
            addSampleStmt(ନୂତନ AstAssign{fl, ନୂତନ AstVarRef{fl, sizedMatchedp, VAccess::WRITE},
                                        ନୂତନ AstConst{fl, AstConst::BitFalse{}}});
        }
        ପାଇଁ (uint32_t sized = 0; sized < sizedBins.size(); ++sized) {
            emitSizedSample(coverpointp, sizedBins[sized], cpVarp, exprp, sized, sizedMatchedp);
        }
        ପାଇଁ (AstCoverBin* ଅଚଳ defBinp : defaultBins) {
            File_ଧାଡ଼ି* ଅଚଳ dfl = defBinp->fileline();
            namerStmts.ଠେଲ_back(makeNamer(cpVarp, defBinp, -1, static_cast<uint32_t>(idx)));
            Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* condp = buildDefaultCondition(coverpointp, exprp, dfl);
            ଯଦି (sizedMatchedp) {
                condp = ନୂତନ AstLogAnd{
                    dfl, ନୂତନ AstNot{dfl, ନୂତନ AstVarRef{dfl, sizedMatchedp, VAccess::READ}}, condp};
            }
            emitConvHitIf(coverpointp, defBinp, cpVarp, cnum(dfl, static_cast<uint32_t>(idx)),
                          condp);
            ++idx;
        }
        ଯଦି (idx > std::numeric_limits<uint32_t>::max()) {
            // The runtime indexes bins with 32 bits; stop before generating a model
            coverpointp->v3warn(E_UNSUPPORTED, "Unsupported: coverpoint with more than "
                                                   << std::numeric_limits<uint32_t>::max()
                                                   << " bins");
        }

        // Transition coverpoints track the previous sampled value; update it once at the end of
        // this coverpoint's sample() contribution (the prev var was created on demand by the
        // transition matching above).
        ଯଦି (coverpointHasTransition(coverpointp)) {
            AstVar* ଅଚଳ prevVarp = VN_AS(coverpointp->user1p(), Var);
            addSampleStmt(
                ନୂତନ AstAssign{coverpointp->fileline(),
                              ନୂତନ AstVarRef{prevVarp->fileline(), prevVarp, VAccess::WRITE},
                              exprp->clone_ବୃକ୍ଷ(ମିଥ୍ୟା)});
        }

        // Constructor: init (allocates), namers, then registration (under --coverage).
        // Under --protect-ids the hierarchy and page string reach the coverage database
        // verbatim, so obfuscate them like line/toggle points (per-word hierarchy, whole-
        // unit page).  No-ops when --protect-ids is off.
        ଅଚଳ ତାର୍କିକ prot = v3Global.opt.protectIds();
        ଅଚଳ std::string hier
            = VIdProtect::protect_ଶବ୍ଦଗୁଡ଼ିକ_If(m_covergroupName + "." + coverpointp->name(), prot);
        m_constructorp->addStmtsp(
            itemCall(fl, cpVarp, VCMethod::COVERGROUP_INIT,
                     {ctext(fl, quoted(hier)), cnum(fl, static_cast<uint32_t>(at_Least_ମୂଲ୍ୟ)),
                      cnum(fl, static_cast<uint32_t>(idx))})
                ->makeStmt());
        ପାଇଁ (Ast_ଗଣ୍ଠି_Stmt* ଅଚଳ ns : namerStmts) m_constructorp->addStmtsp(ns);
        ଯଦି (dynamic) {
            m_constructorp->addStmtsp(
                itemCall(fl, cpVarp, VCMethod::COVERGROUP_ମୂଲ୍ୟ_TYPE,
                         {cnum(fl, exprp->width()), cnum(fl, exprp->isSigned())})
                    ->makeStmt());
            ମୂଲ୍ୟ_Lists lists;
            ପାଇଁ (ଅଚଳ auto& entry : metadata) {
                collect_ମୂଲ୍ୟ_Metadata(lists, exprp, std::get<0>(entry), std::get<1>(entry),
                                     std::get<2>(entry));
            }
            ପାଇଁ (ଅଚଳ BinRun* ଅଚଳ runp : runMetadata) collectRunMetadata(lists, exprp, *runp);
            emit_ମୂଲ୍ୟ_List(fl, cpVarp, VCMethod::COVERGROUP_ମୂଲ୍ୟ_RANGES, lists.m_ranges);
            emit_ମୂଲ୍ୟ_List(fl, cpVarp, VCMethod::COVERGROUP_ମୂଲ୍ୟ_RUNS, lists.m_runs);
            emit_ମୂଲ୍ୟ_List(fl, cpVarp, VCMethod::COVERGROUP_ମୂଲ୍ୟ_PATTERNS, lists.m_patterns);
            emit_ମୂଲ୍ୟ_List(fl, cpVarp, VCMethod::COVERGROUP_ମୂଲ୍ୟ_TRANSITIONS, lists.m_transitions);
            ପାଇଁ (AstCoverBin* ଅଚଳ binp : sizedBins) {
                generateConstructedBins(coverpointp, binp, cpVarp, exprp);
            }
            m_constructorp->addStmtsp(
                itemCall(fl, cpVarp, VCMethod::COVERGROUP_ମୂଲ୍ୟ_FINALIZE)->makeStmt());
        }
        ଯଦି (v3Global.opt.coverage()) {
            ଅଚଳ std::string page
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
    ଶୂନ୍ୟ generate_Multi_ମୂଲ୍ୟ_Transition_Code(AstCoverpoint* coverpointp, AstCoverBin* binp,
                                          Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* exprp, ଅଚଳ ConvBinTarget& tgt,
                                          ଅଚଳ std::vector<AstCoverTransItem*>& ବସ୍ତୁଗୁଡ଼ିକ) {
        UINFO(4, "    Generating multi-value transition state machine for: " << binp->name());
        UINFO(4, "      Sequence length: " << ବସ୍ତୁଗୁଡ଼ିକ.size() << " items");

        // Create state position variable
        AstVar* ଅଚଳ stateVarp = createSequenceStateVar(coverpointp, binp);

        // Build case statement with N cases (one for each state 0 to N-1)
        // State 0: Not started, looking for first item
        // State 1 to N-1: In progress, looking for next item

        AstCase* ଅଚଳ casep
            = ନୂତନ AstCase{binp->fileline(), VCaseType::CT_CASE,
                          ନୂତନ AstVarRef{stateVarp->fileline(), stateVarp, VAccess::READ}, nullptr};

        // Generate each case item in the switch statement
        ପାଇଁ (size_t state = 0; state < ବସ୍ତୁଗୁଡ଼ିକ.size(); ++state) {
            AstCaseItem* caseItemp = generateTransitionStateCase(coverpointp, binp, exprp, tgt,
                                                                 stateVarp, ବସ୍ତୁଗୁଡ଼ିକ, state);
            casep->addItemsp(caseItemp);
        }

        // Add default case (reset to state 0) to prevent CASEINCOMPLETE warnings,
        // since the state variable is wider than the number of valid states.
        AstCaseItem* ଅଚଳ defaultItemp = ନୂତନ AstCaseItem{
            binp->fileline(), nullptr,
            ନୂତନ AstAssign{binp->fileline(),
                          ନୂତନ AstVarRef{binp->fileline(), stateVarp, VAccess::WRITE},
                          ନୂତନ AstConst{binp->fileline(), AstConst::Widthed_ମୂଲ୍ୟ{}, 8, 0}}};
        casep->addItemsp(defaultItemp);

        addSampleStmt(casep);
        UINFO(4, "      Successfully added multi-value transition state machine");
    }

    // Generate code for a single state in the transition state machine
    // Returns the case item for this state
    AstCaseItem* generateTransitionStateCase(AstCoverpoint* coverpointp, AstCoverBin* binp,
                                             Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* exprp, ଅଚଳ ConvBinTarget& tgt,
                                             AstVar* stateVarp,
                                             ଅଚଳ std::vector<AstCoverTransItem*>& ବସ୍ତୁଗୁଡ଼ିକ,
                                             size_t state) {
        File_ଧାଡ଼ି* ଅଚଳ fl = binp->fileline();

        // Build condition for current value matching expected item at this state
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ matchCondp = buildTransitionItemCondition(ବସ୍ତୁଗୁଡ଼ିକ[state], exprp);

        Ast_ଗଣ୍ଠି_Stmt* matchActionp = nullptr;

        ଯଦି (state == ବସ୍ତୁଗୁଡ଼ିକ.size() - 1) {
            // Last state: sequence complete!  Record the hit in the runtime VlCoverpoint.
            matchActionp = makeRuntimeBinHit(fl, tgt);

            // For illegal_bins, add error message
            ଯଦି (binp->binsType() == VCoverBinsType::BINS_ILLEGAL) {
                ଅଚଳ string err_ସନ୍ଦେଶ = "Illegal transition bin " + binp->prettyNameQ()
                                      + " hit in coverpoint " + coverpointp->prettyNameQ();
                matchActionp = matchActionp->addNext(makeIllegalBinAction(fl, err_ସନ୍ଦେଶ));
            }

            // Reset state to 0
            matchActionp = matchActionp->addNext(
                ନୂତନ AstAssign{fl, ନୂତନ AstVarRef{fl, stateVarp, VAccess::WRITE},
                              ନୂତନ AstConst{fl, AstConst::Widthed_ମୂଲ୍ୟ{}, 8, 0}});
        } ଅନ୍ୟଥା {
            // Intermediate state: advance to next state
            matchActionp = ନୂତନ AstAssign{
                fl, ନୂତନ AstVarRef{fl, stateVarp, VAccess::WRITE},
                ନୂତନ AstConst{fl, AstConst::Widthed_ମୂଲ୍ୟ{}, 8, static_cast<uint32_t>(state + 1)}};
        }

        // Build restart logic: check if current value matches first item
        // If so, restart sequence from state 1 (even if we're in middle of sequence)
        Ast_ଗଣ୍ଠି_Stmt* noMatchActionp = nullptr;
        ଯଦି (state > 0) {
            // Check if current value matches first item (restart condition)
            Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ restartCondp = buildTransitionItemCondition(ବସ୍ତୁଗୁଡ଼ିକ[0], exprp);

            UASSERT_OBJ(restartCondp, ବସ୍ତୁଗୁଡ଼ିକ[0],
                        "buildTransitionItemCondition returned nullptr for restart");

            // Restart to state 1
            Ast_ଗଣ୍ଠି_Stmt* restartActionp
                = ନୂତନ AstAssign{fl, ନୂତନ AstVarRef{fl, stateVarp, VAccess::WRITE},
                                ନୂତନ AstConst{fl, AstConst::Widthed_ମୂଲ୍ୟ{}, 8, 1}};

            // Reset to state 0 (else branch)
            Ast_ଗଣ୍ଠି_Stmt* resetActionp
                = ନୂତନ AstAssign{fl, ନୂତନ AstVarRef{fl, stateVarp, VAccess::WRITE},
                                ନୂତନ AstConst{fl, AstConst::Widthed_ମୂଲ୍ୟ{}, 8, 0}};

            noMatchActionp = ନୂତନ AstIf{fl, restartCondp, restartActionp, resetActionp};
        }
        // For state 0, no action needed if no match (stay in state 0)

        // Combine into if-else
        Ast_ଗଣ୍ଠି_Stmt* ଅଚଳ stmtp = ନୂତନ AstIf{fl, matchCondp, matchActionp, noMatchActionp};

        // Create case item for this state value
        AstCaseItem* ଅଚଳ caseItemp = ନୂତନ AstCaseItem{
            fl, ନୂତନ AstConst{fl, AstConst::Widthed_ମୂଲ୍ୟ{}, 8, static_cast<uint32_t>(state)},
            stmtp};

        ଫେରନ୍ତୁ caseItemp;
    }

    // Create: $error(msg); $stop;  Used when an illegal bin is hit.
    Ast_ଗଣ୍ଠି_Stmt* makeIllegalBinAction(File_ଧାଡ଼ି* fl, ଅଚଳ string& err_ସନ୍ଦେଶ) {
        AstDisplay* ଅଚଳ errorp
            = ନୂତନ AstDisplay{fl, VDisplayType::DT_ERROR, err_ସନ୍ଦେଶ, nullptr, nullptr};
        errorp->fmtp()->timeunit(m_covergroupp->timeunit());
        static_cast<Ast_ଗଣ୍ଠି*>(errorp)->addNext(ନୂତନ AstStop{fl, ସତ୍ୟ});
        ଫେରନ୍ତୁ errorp;
    }

    // Preserve the coverpoint's width and signedness after V3Width.
    ସ୍ଥିର AstConst* new_ମୂଲ୍ୟ_Const(File_ଧାଡ଼ି* fl, ଅଚଳ V3_ସଂଖ୍ୟା& ମୂଲ୍ୟ, ଅଚଳ Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* exprp) {
        V3_ସଂଖ୍ୟା narrowed{fl, exprp->width(), 0};
        narrowed.opAssign(ମୂଲ୍ୟ);
        AstConst* ଅଚଳ constp = ନୂତନ AstConst{fl, narrowed};
        constp->dtypeFrom(exprp);
        ଫେରନ୍ତୁ constp;
    }

    // A real copy of a real or integral range bound, for comparing with a real coverpoint.
    ସ୍ଥିର AstConst* newRealConst(AstConst* constp) {
        ଯଦି (constp->ଅଙ୍କ().isDouble()) ଫେରନ୍ତୁ constp->clone_ବୃକ୍ଷ(ମିଥ୍ୟା);
        V3_ସଂଖ୍ୟା real{&constp->ଅଙ୍କ(), 64};
        real.opIToRD(constp->ଅଙ୍କ(), constp->isSigned());
        ଫେରନ୍ତୁ ନୂତନ AstConst{constp->fileline(), real};
    }

    // Clone a constant node, widening to targetWidth if needed (zero-extend).
    // Used to ensure comparisons use matching widths after V3Width has run.
    ସ୍ଥିର AstConst* widenConst(File_ଧାଡ଼ି* fl, AstConst* constp, ପୂର୍ଣ୍ଣସଂଖ୍ୟା targetWidth) {
        ଯଦି (constp->width() == targetWidth) ଫେରନ୍ତୁ constp->clone_ବୃକ୍ଷ(ମିଥ୍ୟା);
        V3_ସଂଖ୍ୟା ଅଙ୍କ{fl, targetWidth, 0};
        ଅଙ୍କ.opAssign(constp->ଅଙ୍କ());
        ଫେରନ୍ତୁ ନୂତନ AstConst{fl, ଅଙ୍କ};
    }

    // Build a range condition: minp <= exprp <= maxp.
    // Uses signed comparisons if exprp is signed; omits trivially-true domain bounds.
    // All arguments are non-owning; clones exprp/minp/maxp as needed.
    Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* makeRangeCondition(File_ଧାଡ଼ି* fl, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* exprp, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* minp,
                                    Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* maxp) {
        ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା ପଦସମଷ୍ଟି_Width = exprp->widthMin();
        AstConst* ଅଚଳ minConstp = VN_AS(minp, Const);
        AstConst* ଅଚଳ maxConstp = VN_AS(maxp, Const);
        ଯଦି (exprp->isDouble()) {
            // A real coverpoint has no finite domain bounds to omit.
            ଫେରନ୍ତୁ ନୂତନ AstAnd{fl,
                              ନୂତନ AstGteD{fl, exprp->clone_ବୃକ୍ଷ(ମିଥ୍ୟା), newRealConst(minConstp)},
                              ନୂତନ AstLteD{fl, exprp->clone_ବୃକ୍ଷ(ମିଥ୍ୟା), newRealConst(maxConstp)}};
        }
        // Widen constants to match expression width so post-V3Width nodes use correct macros
        AstConst* ଅଚଳ minWidep = widenConst(fl, minConstp, ପଦସମଷ୍ଟି_Width);
        AstConst* ଅଚଳ maxWidep = widenConst(fl, maxConstp, ପଦସମଷ୍ଟି_Width);
        V3_ସଂଖ୍ୟା minimum{fl, ପଦସମଷ୍ଟି_Width, 0};
        V3_ସଂଖ୍ୟା maximum{fl, ପଦସମଷ୍ଟି_Width, 0};
        maximum.setAllBits1();
        ଯଦି (exprp->isSigned()) {
            minimum.setBit(ପଦସମଷ୍ଟି_Width - 1, 1);
            maximum.setBit(ପଦସମଷ୍ଟି_Width - 1, 0);
        }
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* lowerp = nullptr;
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* upperp = nullptr;
        ଯଦି (minWidep->ଅଙ୍କ().isCaseEq(minimum)) {
            VL_DO_DANGLING(ଠେଲ_Deletep(minWidep), minWidep);
        } ଅନ୍ୟଥା {
            lowerp = exprp->isSigned() ? static_cast<Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି*>(
                                             ନୂତନ AstGteS{fl, exprp->clone_ବୃକ୍ଷ(ମିଥ୍ୟା), minWidep})
                                       : static_cast<Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି*>(
                                             ନୂତନ AstGte{fl, exprp->clone_ବୃକ୍ଷ(ମିଥ୍ୟା), minWidep});
        }
        ଯଦି (maxWidep->ଅଙ୍କ().isCaseEq(maximum)) {
            VL_DO_DANGLING(ଠେଲ_Deletep(maxWidep), maxWidep);
        } ଅନ୍ୟଥା {
            upperp = exprp->isSigned() ? static_cast<Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି*>(
                                             ନୂତନ AstLteS{fl, exprp->clone_ବୃକ୍ଷ(ମିଥ୍ୟା), maxWidep})
                                       : static_cast<Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି*>(
                                             ନୂତନ AstLte{fl, exprp->clone_ବୃକ୍ଷ(ମିଥ୍ୟା), maxWidep});
        }
        ଯଦି (lowerp && upperp) ଫେରନ୍ତୁ ନୂତନ AstAnd{fl, lowerp, upperp};
        ଯଦି (lowerp) ଫେରନ୍ତୁ lowerp;
        ଯଦି (upperp) ଫେରନ୍ତୁ upperp;
        ଫେରନ୍ତୁ ନୂତନ AstConst{fl, AstConst::BitTrue{}};
    }

    // Build a one-sided comparison for an open-ended bin range whose other bound is '$'.
    // '$' denotes the coverpoint domain extreme, so {[lo:$]} == (expr >= lo) and
    // {[$:hi]} == (expr <= hi).
    Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* makeOpenRangeCondition(File_ଧାଡ଼ି* fl, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* exprp, AstConst* boundp,
                                        ତାର୍କିକ isLowerBound) {
        AstConst* ଅଚଳ widep = widenConst(fl, boundp, exprp->widthMin());
        ଯଦି (isLowerBound) {
            ଯଦି (exprp->isSigned()) ଫେରନ୍ତୁ ନୂତନ AstGteS{fl, exprp->clone_ବୃକ୍ଷ(ମିଥ୍ୟା), widep};
            ଫେରନ୍ତୁ ନୂତନ AstGte{fl, exprp->clone_ବୃକ୍ଷ(ମିଥ୍ୟା), widep};
        }
        ଯଦି (exprp->isSigned()) ଫେରନ୍ତୁ ନୂତନ AstLteS{fl, exprp->clone_ବୃକ୍ଷ(ମିଥ୍ୟା), widep};
        ଫେରନ୍ତୁ ନୂତନ AstLte{fl, exprp->clone_ବୃକ୍ଷ(ମିଥ୍ୟା), widep};
    }

    // Build condition for a single transition item.
    // Returns expression that checks if exprp matches the item's value/range list.
    // Overload for when the expression is a variable read -- creates and manages the VarRef
    // internally, so callers don't need to construct a temporary node.
    Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* buildTransitionItemCondition(AstCoverTransItem* itemp, AstVar* varp) {
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* varRefp = ନୂତନ AstVarRef{varp->fileline(), varp, VAccess::READ};
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ condp = buildTransitionItemCondition(itemp, varRefp);
        VL_DO_DANGLING(ଠେଲ_Deletep(varRefp), varRefp);
        ଫେରନ୍ତୁ condp;
    }

    // Non-owning: exprp is cloned internally; caller retains ownership of exprp.
    Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* buildTransitionItemCondition(AstCoverTransItem* itemp, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* exprp) {
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* condp = nullptr;

        ପାଇଁ (Ast_ଗଣ୍ଠି* valp = itemp->valuesp(); valp; valp = valp->nextp()) {
            Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* singleCondp = nullptr;
            valp = V3Const::constifyEdit(valp);
            AstConst* ଅଚଳ constp = VN_CAST(valp, Const);
            ଯଦି (!constp) {
                valp->v3error("Non-constant expression in transition bin; "
                              "values must be constants (IEEE 1800-2023 19.5)");
                ଫେରନ୍ତୁ ନୂତନ AstConst{valp->fileline(), AstConst::BitFalseErroring{}};
            }
            singleCondp
                = ନୂତନ AstEq{constp->fileline(), exprp->clone_ବୃକ୍ଷ(ମିଥ୍ୟା), constp->clone_ବୃକ୍ଷ(ମିଥ୍ୟା)};
            ଯଦି (condp) {
                condp = ନୂତନ AstOr{itemp->fileline(), condp, singleCondp};
            } ଅନ୍ୟଥା {
                condp = singleCondp;
            }
        }

        ଫେରନ୍ତୁ condp;
    }

    // Generate code for a single transition sequence (used by both regular and array bins)
    ଶୂନ୍ୟ generateSingleTransitionCode(AstCoverpoint* coverpointp, AstCoverBin* binp,
                                      Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* exprp, ଅଚଳ ConvBinTarget& tgt,
                                      AstCoverTransSet* transSetp) {
        UINFO(4, "      Generating code for transition sequence");

        // Get or create previous value variable
        AstVar* ଅଚଳ prevVarp = create_Prev_ମୂଲ୍ୟ_Var(coverpointp, exprp);

        UASSERT_OBJ(
            transSetp, binp,
            "Transition bin has no transition set (transp() was checked before calling this)");

        // Get transition items (the sequence: item1 => item2 => item3)
        std::vector<AstCoverTransItem*> ବସ୍ତୁଗୁଡ଼ିକ;
        ପାଇଁ (Ast_ଗଣ୍ଠି* itemp = transSetp->itemsp(); itemp; itemp = itemp->nextp())
            ବସ୍ତୁଗୁଡ଼ିକ.ଠେଲ_back(VN_AS(itemp, CoverTransItem));

        ଯଦି (ବସ୍ତୁଗୁଡ଼ିକ.empty()) {
            binp->v3error("Transition set without items");
            ଫେରନ୍ତୁ;
        }

        ଯଦି (ବସ୍ତୁଗୁଡ଼ିକ.size() == 1) {
            // Single item transition not valid (need at least 2 values for =>)
            binp->v3error("Transition requires at least two values");
            ଫେରନ୍ତୁ;
        } ଅନ୍ୟଥା ଯଦି (ବସ୍ତୁଗୁଡ଼ିକ.size() == 2) {
            // Simple two-value transition: (val1 => val2)
            // Use optimized direct comparison (no state machine needed)
            Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ cond1p = buildTransitionItemCondition(ବସ୍ତୁଗୁଡ଼ିକ[0], prevVarp);
            Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ cond2p = buildTransitionItemCondition(ବସ୍ତୁଗୁଡ଼ିକ[1], exprp);

            // Combine: prev matches val1 AND current matches val2
            Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* fullCondp = ନୂତନ AstAnd{binp->fileline(), cond1p, cond2p};

            addConvTransHitIf(coverpointp, binp, tgt, fullCondp);

            UINFO(4, "        Successfully added 2-value transition if statement");
        } ଅନ୍ୟଥା {
            // Multi-value sequence (a => b => c => ...)
            // Use state machine to track position in sequence
            generate_Multi_ମୂଲ୍ୟ_Transition_Code(coverpointp, binp, exprp, tgt, ବସ୍ତୁଗୁଡ଼ିକ);
        }
    }

    // Append a "{ VlCoverpoint* __Vcx_cps[] = {cp0, cp1, ...}; <call> }" statement.  The brace
    // and the temporary array stay literal text -- a CMethodHard is one call, not a block --
    // but callp itself carries the member, method and '->'.  Construction only: init() copies
    // the array into the cross, so sample() reads it from there and needs no array at all.
    AstCStmt* makeCrossCpsCall(File_ଧାଡ଼ି* fl, ଅଚଳ std::vector<AstVar*>& cpVars,
                               AstCMethodHard* callp) {
        AstCStmt* ଅଚଳ cs = ନୂତନ AstCStmt{fl};
        cs->add("{ VlCoverpoint* __Vcx_cps[] = {");
        ପାଇଁ (size_t d = 0; d < cpVars.size(); ++d) {
            ଯଦି (d != 0) cs->add(", ");
            cs->add(memberRef(fl, cpVars[d]));
        }
        cs->add("}; ");
        cs->add(callp);
        cs->add("; }");
        ଫେରନ୍ତୁ cs;
    }

    // Assign the per-bin flags individually: one-bit SV results have integer C++ storage types,
    // which may narrow in a bool initializer list but convert implicitly in assignments.
    AstCStmt* makeCrossIffsCall(File_ଧାଡ଼ି* fl, ଅଚଳ std::vector<AstCoverCrossBin*>& bins,
                                AstCMethodHard* callp) {
        AstCStmt* ଅଚଳ cs = ନୂତନ AstCStmt{fl};
        cs->add("{ bool __Vcx_iffs[" + cvtToStr(bins.size()) + "]; ");
        ପାଇଁ (size_t i = 0; i < bins.size(); ++i) {
            ଅଚଳ AstCoverCrossBin* ଅଚଳ binp = bins[i];
            cs->add("__Vcx_iffs[" + cvtToStr(i) + "] = ");
            cs->add(binp->iffp() ? binp->iffp()->clone_ବୃକ୍ଷ(ମିଥ୍ୟା)
                                 : ନୂତନ AstConst{fl, AstConst::BitTrue{}});
            cs->add("; ");
        }
        cs->add(callp);
        cs->add("; }");
        ଫେରନ୍ତୁ cs;
    }

    using CrossSelection = std::vector<uint64_t>;
    ସଂରଚନା ResolvedCrossBin final {
        AstCoverCrossBin* binp;
        CrossSelection selection;
    };
    ସଂରଚନା CrossLayout final {
        uint32_t tuples = 0;
        uint32_t autoBins = 0;
        uint64_t bin_ଶବ୍ଦଗୁଡ଼ିକ = 0;
        ତାର୍କିକ valid = ସତ୍ୟ;
        std::vector<ResolvedCrossBin> bins;
    };
    ସଂରଚନା CrossSelectionContext final {
        AstCoverCross* crossp;  // Cross whose tuple space is being selected
        ଅଚଳ std::vector<AstVar*>& cpVars;  // Feeding coverpoints in dimension order
        ଅଚଳ std::map<std::string, uint32_t>& dimensions;  // Coverpoint name -> dimension
        uint32_t tuples;  // Size of the Cartesian product
        std::vector<uint32_t> strides;  // Flat-index stride per dimension
        ତାର୍କିକ valid = ସତ୍ୟ;  // False if this explicit bin cannot be implemented
    };
    ସଂରଚନା CrossBinsofTarget final {
        ଅଚଳ CoverpointBins* m_binsp = nullptr;  // Declared bins; null after a reported error
        uint32_t m_dimension = 0;  // Coverpoint index within the cross
        uint32_t m_first = 0;  // First declared normal bin in the selected span
        uint32_t m_ଗଣନା = 0;  // Number of declared normal bins in the selected span
        uint32_t m_declaredFirst = 0;  // First runtime bin index of the selected span
        uint32_t m_declaredEnd = UINT32_MAX;  // One past the last runtime bin index
        int32_t m_sized = -1;  // Index of the sized array holding the selected bins; or -1
    };
    ସଂରଚନା Cross_ମୂଲ୍ୟ_Range final {
        V3_ସଂଖ୍ୟା lo;  // Inclusive lower bound, sign-extended to the comparison width
        V3_ସଂଖ୍ୟା hi;  // Inclusive upper bound
        V3_ସଂଖ୍ୟା pattern;  // Allowed bit values; all X for an ordinary interval
        ତାର୍କିକ wildcard = ମିଥ୍ୟା;  // A wildcard singleton rather than an exact four-state value

        Cross_ମୂଲ୍ୟ_Range(Ast_ଗଣ୍ଠି* nodep, ପୂର୍ଣ୍ଣସଂଖ୍ୟା width)
            : lo{nodep, width}
            , hi{nodep, width}
            , pattern{nodep, width} {
            pattern.setAllBitsX();
        }
    };

    ସ୍ଥିର std::vector<Ast_ଗଣ୍ଠି*> cross_Bin_ମୂଲ୍ୟଗୁଡ଼ିକ(ଅଚଳ Cross_Bin_ମୂଲ୍ୟଗୁଡ଼ିକ& bin) {
        ଯଦି (bin.valuep) ଫେରନ୍ତୁ {bin.valuep};
        std::vector<Ast_ଗଣ୍ଠି*> ମୂଲ୍ୟଗୁଡ଼ିକ;
        ଯଦି (bin.binp->transp()) {
            // IEEE 1800-2023 19.6.1: binsof uses the last value of each transition.
            ପାଇଁ (Ast_ଗଣ୍ଠି* setp = bin.binp->transp(); setp; setp = setp->nextp()) {
                AstCoverTransItem* lastp = VN_AS(setp, CoverTransSet)->itemsp();
                ଯେତେବେଳେ (lastp->nextp()) lastp = VN_AS(lastp->nextp(), CoverTransItem);
                ପାଇଁ (Ast_ଗଣ୍ଠି* valuep = lastp->valuesp(); valuep; valuep = valuep->nextp()) {
                    ମୂଲ୍ୟଗୁଡ଼ିକ.ଠେଲ_back(valuep);
                }
            }
        } ଅନ୍ୟଥା {
            ପାଇଁ (Ast_ଗଣ୍ଠି* valuep = bin.binp->rangesp(); valuep; valuep = valuep->nextp()) {
                ମୂଲ୍ୟଗୁଡ଼ିକ.ଠେଲ_back(valuep);
            }
        }
        ଫେରନ୍ତୁ ମୂଲ୍ୟଗୁଡ଼ିକ;
    }

    // The values of the element-th bin of a run, sign-extended to 'width'
    ସ୍ଥିର Cross_ମୂଲ୍ୟ_Range runBinRange(Ast_ଗଣ୍ଠି* nodep, ଅଚଳ BinRun& run, uint32_t element,
                                       ପୂର୍ଣ୍ଣସଂଖ୍ୟା width) {
        ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା runw = run.m_lo.width();
        V3_ସଂଖ୍ୟା offset{nodep, runw};
        offset.opMul(run.m_stride, V3_ସଂଖ୍ୟା{nodep, runw, element});
        V3_ସଂଖ୍ୟା lo{nodep, runw};
        lo.opAdd(run.m_lo, offset);
        V3_ସଂଖ୍ୟା hi = run.m_hi;
        ଯଦି (element + 1 < run.m_ଗଣନା) {
            offset.opSub(run.m_stride, V3_ସଂଖ୍ୟା{nodep, runw, 1});
            hi.opAdd(lo, offset);
        }
        Cross_ମୂଲ୍ୟ_Range range{nodep, width};
        range.lo.opExtendS(lo, runw);
        range.hi.opExtendS(hi, runw);
        ଫେରନ୍ତୁ range;
    }

    ସ୍ଥିର ପୂର୍ଣ୍ଣସଂଖ୍ୟା crossRangeWidth(Ast_ଗଣ୍ଠି* nodep) {
        ଯଦି (ଅଚଳ AstInsideRange* ଅଚଳ rangep = VN_CAST(nodep, InsideRange)) {
            ଫେରନ୍ତୁ std::max(rangep->lhsp()->width(), rangep->rhsp()->width());
        }
        ଫେରନ୍ତୁ nodep->width();
    }

    ସ୍ଥିର ତାର୍କିକ cross_ମୂଲ୍ୟ_Less(ଅଚଳ V3_ସଂଖ୍ୟା& lhs, ଅଚଳ V3_ସଂଖ୍ୟା& rhs) {
        V3_ସଂଖ୍ୟା result{&lhs};
        ଫେରନ୍ତୁ !result.opLtS(lhs, rhs).isEqZero();
    }

    // Round a real bound inward to the nearest coverpoint value (IEEE 1800-2023 19.5.7).  Sets
    // 'empty' if the domain has no value on the bound's side.
    ସ୍ଥିର ଶୂନ୍ୟ crossRealBound(ଅଚଳ AstConst* constp, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* exprp, ତାର୍କିକ upper,
                               ଅଚଳ Cross_ମୂଲ୍ୟ_Range& domain, V3_ସଂଖ୍ୟା& result, ତାର୍କିକ& empty) {
        ଅଚଳ ଦ୍ୱିଗୁଣ ମୂଲ୍ୟ = constp->ଅଙ୍କ().toDouble();
        ଅଚଳ ଦ୍ୱିଗୁଣ bound = upper ? std::floor(ମୂଲ୍ୟ) : std::ceil(ମୂଲ୍ୟ);
        // Powers of two are exact, so the integral bound compares exactly with the domain edges.
        ଅଚଳ ଦ୍ୱିଗୁଣ limit
            = std::ldexp(1.0, exprp->isSigned() ? exprp->width() - 1 : exprp->width());
        ଅଚଳ ଦ୍ୱିଗୁଣ minimum = exprp->isSigned() ? -limit : 0.0;
        ଯଦି (std::isnan(bound) || (upper ? bound < minimum : bound >= limit)) {
            empty = ସତ୍ୟ;
            result = domain.lo;
        } ଅନ୍ୟଥା ଯଦି (bound < minimum) {
            result = domain.lo;
        } ଅନ୍ୟଥା ଯଦି (bound >= limit) {
            result = domain.hi;
        } ଅନ୍ୟଥା {
            V3_ସଂଖ୍ୟା real{&result, 64};
            real.setDouble(bound);
            result.opRToIRoundS(real);
        }
    }

    ସ୍ଥିର ତାର୍କିକ crossRangeBound(Ast_ଗଣ୍ଠି* nodep, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* exprp, ତାର୍କିକ upper, ତାର୍କିକ bin_ମୂଲ୍ୟ,
                                ଅଚଳ Cross_ମୂଲ୍ୟ_Range& domain, V3_ସଂଖ୍ୟା& result, ତାର୍କିକ& empty) {
        ଯଦି (VN_IS(nodep, Unbounded)) {
            V3_ସଂଖ୍ୟା limit{nodep, exprp->width()};
            ଯଦି (upper) limit.setAllBits1();
            ଯଦି (exprp->isSigned()) {
                limit.setBit(exprp->width() - 1, !upper);
                result.opExtendS(limit, limit.width());
            } ଅନ୍ୟଥା {
                result.opAssign(limit);
            }
            ଫେରନ୍ତୁ ସତ୍ୟ;
        }
        ଅଚଳ AstConst* ଅଚଳ constp = VN_CAST(nodep, Const);
        ଯଦି (!constp) ଫେରନ୍ତୁ ମିଥ୍ୟା;
        ଯଦି (constp->ଅଙ୍କ().isDouble()) {
            crossRealBound(constp, exprp, upper, domain, result, empty);
            ଫେରନ୍ତୁ ସତ୍ୟ;
        }
        ଯଦି (constp->ଅଙ୍କ().isString()) ଫେରନ୍ତୁ ମିଥ୍ୟା;
        ଯଦି (bin_ମୂଲ୍ୟ && exprp->isSigned() && constp->width() <= exprp->width()) {
            // Bin bit patterns use the coverpoint's effective type (IEEE 1800-2023 19.5.7).
            // Wider values and intersect filters retain their values for domain clipping.
            V3_ସଂଖ୍ୟା ମୂଲ୍ୟ{nodep, exprp->width()};
            ଯଦି (constp->isSigned()) {
                ମୂଲ୍ୟ.opExtendS(constp->ଅଙ୍କ(), constp->width());
            } ଅନ୍ୟଥା {
                ମୂଲ୍ୟ.opAssign(constp->ଅଙ୍କ());
            }
            result.opExtendS(ମୂଲ୍ୟ, ମୂଲ୍ୟ.width());
        } ଅନ୍ୟଥା ଯଦି (constp->isSigned()) {
            result.opExtendS(constp->ଅଙ୍କ(), constp->width());
        } ଅନ୍ୟଥା {
            result.opAssign(constp->ଅଙ୍କ());
        }
        ଫେରନ୍ତୁ ସତ୍ୟ;
    }

    ସ୍ଥିର Cross_ମୂଲ୍ୟ_Range cross_ମୂଲ୍ୟ_Domain(Ast_ଗଣ୍ଠି* nodep, ପୂର୍ଣ୍ଣସଂଖ୍ୟା ମୂଲ୍ୟ_Width, ତାର୍କିକ isSigned,
                                            ପୂର୍ଣ୍ଣସଂଖ୍ୟା width) {
        Cross_ମୂଲ୍ୟ_Range domain{nodep, width};
        V3_ସଂଖ୍ୟା lo{nodep, ମୂଲ୍ୟ_Width};
        V3_ସଂଖ୍ୟା hi{nodep, ମୂଲ୍ୟ_Width};
        hi.setAllBits1();
        ଯଦି (isSigned) {
            lo.setBit(ମୂଲ୍ୟ_Width - 1, 1);
            hi.setBit(ମୂଲ୍ୟ_Width - 1, 0);
            domain.lo.opExtendS(lo, ମୂଲ୍ୟ_Width);
            domain.hi.opExtendS(hi, ମୂଲ୍ୟ_Width);
        } ଅନ୍ୟଥା {
            domain.lo.opAssign(lo);
            domain.hi.opAssign(hi);
        }
        ଫେରନ୍ତୁ domain;
    }

    ସ୍ଥିର ଶୂନ୍ୟ intersectCrossRange(Cross_ମୂଲ୍ୟ_Range& range, ଅଚଳ Cross_ମୂଲ୍ୟ_Range& other) {
        ଯଦି (cross_ମୂଲ୍ୟ_Less(range.lo, other.lo)) range.lo = other.lo;
        ଯଦି (cross_ମୂଲ୍ୟ_Less(other.hi, range.hi)) range.hi = other.hi;
    }

    ସ୍ଥିର ତାର୍କିକ cross_ମୂଲ୍ୟ_Range(Ast_ଗଣ୍ଠି* nodep, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* exprp, ତାର୍କିକ bin_ମୂଲ୍ୟ, ତାର୍କିକ wildcard,
                                ଅଚଳ Cross_ମୂଲ୍ୟ_Range& domain, Cross_ମୂଲ୍ୟ_Range& range) {
        ତାର୍କିକ empty = ମିଥ୍ୟା;
        ଅଚଳ AstConst* ଅଚଳ constp = VN_CAST(nodep, Const);
        ଯଦି (ଅଚଳ AstInsideRange* ଅଚଳ rangep = VN_CAST(nodep, InsideRange)) {
            ଯଦି (!crossRangeBound(rangep->lhsp(), exprp, ମିଥ୍ୟା, bin_ମୂଲ୍ୟ, domain, range.lo, empty)
                || !crossRangeBound(rangep->rhsp(), exprp, ସତ୍ୟ, bin_ମୂଲ୍ୟ, domain, range.hi, empty)
                || range.lo.isFourState() || range.hi.isFourState()) {
                ଫେରନ୍ତୁ ମିଥ୍ୟା;
            }
        } ଅନ୍ୟଥା ଯଦି (constp && constp->ଅଙ୍କ().isDouble()) {
            // A real value participates only if integral; it has no wildcard bits.
            crossRealBound(constp, exprp, ମିଥ୍ୟା, domain, range.lo, empty);
            crossRealBound(constp, exprp, ସତ୍ୟ, domain, range.hi, empty);
        } ଅନ୍ୟଥା {
            ଯଦି (!crossRangeBound(nodep, exprp, ମିଥ୍ୟା, bin_ମୂଲ୍ୟ, domain, range.lo, empty)) {
                ଫେରନ୍ତୁ ମିଥ୍ୟା;
            }
            ଯଦି (bin_ମୂଲ୍ୟ && exprp->isSigned() && constp && !constp->isSigned()
                && constp->width() > exprp->width()) {
                ତାର୍କିକ representable = ସତ୍ୟ;
                ପାଇଁ (ପୂର୍ଣ୍ଣସଂଖ୍ୟା bit = exprp->width(); bit < constp->width(); ++bit) {
                    representable &= !constp->ଅଙ୍କ().bitIs1(bit);
                }
                ଯଦି (representable) {
                    V3_ସଂଖ୍ୟା ମୂଲ୍ୟ{nodep, exprp->width()};
                    ମୂଲ୍ୟ.opAssign(constp->ଅଙ୍କ());
                    range.lo.opExtendS(ମୂଲ୍ୟ, ମୂଲ୍ୟ.width());
                }
            }
            range.hi = range.lo;
            range.wildcard = wildcard;
            ଯଦି (wildcard) {
                range.pattern = range.lo;
                range.lo = domain.lo;
                range.hi = domain.hi;
                ଯଦି (constp && constp->isSigned()) {
                    // Replicated X sign bits are correlated, not independent wildcards.
                    // The source domain preserves expansion-before-casting (19.5.7).
                    intersectCrossRange(
                        range, cross_ମୂଲ୍ୟ_Domain(nodep, constp->width(), ସତ୍ୟ, domain.lo.width()));
                }
            }
        }
        ଯଦି (empty) {
            range.lo = domain.hi;
            range.hi = domain.lo;
            ଫେରନ୍ତୁ ସତ୍ୟ;
        }
        ଯଦି (!range.lo.isFourState()) intersectCrossRange(range, domain);
        ଯଦି (range.wildcard && exprp->isSigned()) {
            ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା sign = exprp->width() - 1;
            ଯଦି (constp && !constp->isSigned() && constp->width() > exprp->width()) {
                // Unsigned equality preserves every target bit pattern if discarded bits are zero.
                ପାଇଁ (ପୂର୍ଣ୍ଣସଂଖ୍ୟା bit = exprp->width(); bit < constp->width(); ++bit) {
                    ଯଦି (constp->ଅଙ୍କ().bitIs1(bit)) {
                        range.lo = domain.hi;
                        range.hi = domain.lo;
                        ଫେରନ୍ତୁ ସତ୍ୟ;
                    }
                }
                ପାଇଁ (ପୂର୍ଣ୍ଣସଂଖ୍ୟା bit = exprp->width(); bit < range.pattern.width(); ++bit) {
                    ଯଦି (range.pattern.bitIsXZ(sign))
                        range.pattern.setBit(bit, 'x');
                    ଅନ୍ୟଥା
                        range.pattern.setBit(bit, range.pattern.bitIs1(sign));
                }
                ଫେରନ୍ତୁ ସତ୍ୟ;
            }
            // Discarded high bits constrain the target sign, rather than becoming don't-cares.
            ପାଇଁ (ପୂର୍ଣ୍ଣସଂଖ୍ୟା bit = exprp->width(); bit < range.pattern.width(); ++bit) {
                ଯଦି (range.pattern.bitIsXZ(bit)) ଜାରି;
                ଅଚଳ ତାର୍କିକ ମୂଲ୍ୟ = range.pattern.bitIs1(bit);
                ଯଦି (!range.pattern.bitIsXZ(sign) && range.pattern.bitIs1(sign) != ମୂଲ୍ୟ) {
                    range.lo = domain.hi;
                    range.hi = domain.lo;
                    ଭାଙ୍ଗ;
                }
                range.pattern.setBit(sign, ମୂଲ୍ୟ);
            }
        }
        ଫେରନ୍ତୁ ସତ୍ୟ;
    }

    ଗଣନା CrossRangeState : uint8_t {
        CROSS_INSIDE_BOUNDS = 0,  // Prefix is strictly inside the interval
        CROSS_AT_LOWER = 1,
        CROSS_AT_UPPER = 2,
        CROSS_AT_BOUNDS = CROSS_AT_LOWER | CROSS_AT_UPPER,
        CROSS_NO_MATCH = 4  // Prefix cannot match the interval/pattern
    };

    ସ୍ଥିର CrossRangeState crossRangeStep(ଅଚଳ Cross_ମୂଲ୍ୟ_Range& range, CrossRangeState state,
                                          ପୂର୍ଣ୍ଣସଂଖ୍ୟା bit, ପୂର୍ଣ୍ଣସଂଖ୍ୟା ମୂଲ୍ୟ) {
        ଯଦି (state == CROSS_NO_MATCH) ଫେରନ୍ତୁ CROSS_NO_MATCH;
        // Flipping the sign bit makes signed order lexicographic.
        ଅଚଳ ତାର୍କିକ sign = bit == range.pattern.width() - 1;
        ଯଦି (!range.pattern.bitIsXZ(bit) && ମୂଲ୍ୟ != (range.pattern.bitIs1(bit) ^ sign)) {
            ଫେରନ୍ତୁ CROSS_NO_MATCH;
        }
        ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା ତଳ = range.lo.bitIs1(bit) ^ sign;
        ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା ଉପର = range.hi.bitIs1(bit) ^ sign;
        ଯଦି (((state & CROSS_AT_LOWER) && ମୂଲ୍ୟ < ତଳ)
            || ((state & CROSS_AT_UPPER) && ମୂଲ୍ୟ > ଉପର)) {
            ଫେରନ୍ତୁ CROSS_NO_MATCH;
        }
        ଫେରନ୍ତୁ static_cast<CrossRangeState>(
            ((state & CROSS_AT_LOWER) && ମୂଲ୍ୟ == ତଳ ? CROSS_AT_LOWER : CROSS_INSIDE_BOUNDS)
            | ((state & CROSS_AT_UPPER) && ମୂଲ୍ୟ == ଉପର ? CROSS_AT_UPPER : CROSS_INSIDE_BOUNDS));
    }

    ସ୍ଥିର ତାର୍କିକ crossWildcardIntersects(ଅଚଳ Cross_ମୂଲ୍ୟ_Range& range) {
        unsigned states = 1U << CROSS_AT_BOUNDS;
        ପାଇଁ (ପୂର୍ଣ୍ଣସଂଖ୍ୟା bit = range.pattern.width() - 1; bit >= 0 && states; --bit) {
            unsigned next = 0;
            ପାଇଁ (ଅଚଳ CrossRangeState state :
                 {CROSS_INSIDE_BOUNDS, CROSS_AT_LOWER, CROSS_AT_UPPER, CROSS_AT_BOUNDS}) {
                ଯଦି (!(states & (1U << state))) ଜାରି;
                ପାଇଁ (ପୂର୍ଣ୍ଣସଂଖ୍ୟା ମୂଲ୍ୟ = 0; ମୂଲ୍ୟ < 2; ++ମୂଲ୍ୟ) {
                    ଅଚଳ CrossRangeState equal = crossRangeStep(range, state, bit, ମୂଲ୍ୟ);
                    ଯଦି (equal != CROSS_NO_MATCH) next |= 1U << equal;
                }
            }
            states = next;
        }
        ଫେରନ୍ତୁ states != 0;
    }

    // The least value at least 'from' with the non-x bits of 'pattern', in unsigned order; false
    // if none.  A bit scan, which does not enumerate the pattern's x bits.
    ସ୍ଥିର ତାର୍କିକ next_Pattern_ମୂଲ୍ୟ(ଅଚଳ V3_ସଂଖ୍ୟା& pattern, ଅଚଳ V3_ସଂଖ୍ୟା& from, V3_ସଂଖ୍ୟା& result) {
        result.opAssign(from);
        ପୂର୍ଣ୍ଣସଂଖ୍ୟା carry = -1;  // Lowest x bit above the bit scanned where 'from' has a 0
        ପାଇଁ (ପୂର୍ଣ୍ଣସଂଖ୍ୟା bit = from.width() - 1; bit >= 0; --bit) {
            ଯଦି (pattern.bitIsXZ(bit)) {
                ଯଦି (!from.bitIs1(bit)) carry = bit;
                ଜାରି;
            }
            ଯଦି (pattern.bitIs1(bit) == from.bitIs1(bit)) ଜାରି;
            ଯଦି (from.bitIs1(bit)) {  // Only a greater prefix, an x bit above raised, can match
                ଯଦି (carry < 0) ଫେରନ୍ତୁ ମିଥ୍ୟା;
                bit = carry;
            }
            // Then the least such value: the pattern's bits, with x bits of zero
            result.setBit(bit, 1);
            ଯେତେବେଳେ (--bit >= 0) result.setBit(bit, pattern.bitIs1(bit));
            ଫେରନ୍ତୁ ସତ୍ୟ;
        }
        ଫେରନ୍ତୁ ସତ୍ୟ;
    }

    // Append to 'runs' the maximal runs of consecutive values of 'range' that its pattern
    // matches, in value order, until there are more than 'limit'.  A match continues through
    // the pattern's trailing x bits only, which the next value's carry leaves.
    ସ୍ଥିର ଶୂନ୍ୟ crossRangeRuns(ଅଚଳ Cross_ମୂଲ୍ୟ_Range& range, size_t limit,
                               std::vector<std::pair<V3_ସଂଖ୍ୟା, V3_ସଂଖ୍ୟା>>& runs) {
        // Values order signed, which is the unsigned order of values with the sign bit flipped
        ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା sign = range.lo.width() - 1;
        ଅଚଳ auto flipped = [sign](V3_ସଂଖ୍ୟା ମୂଲ୍ୟ) {
            ଯଦି (!ମୂଲ୍ୟ.bitIsXZ(sign)) ମୂଲ୍ୟ.setBit(sign, !ମୂଲ୍ୟ.bitIs1(sign));
            ଫେରନ୍ତୁ ମୂଲ୍ୟ;
        };
        ଅଚଳ V3_ସଂଖ୍ୟା pattern = flipped(range.pattern);
        ଅଚଳ V3_ସଂଖ୍ୟା hi = flipped(range.hi);
        ଅଚଳ V3_ସଂଖ୍ୟା one{&hi, hi.width(), 1};
        V3_ସଂଖ୍ୟା trailing{&hi, hi.width()};
        ପାଇଁ (ପୂର୍ଣ୍ଣସଂଖ୍ୟା bit = 0; bit <= sign && pattern.bitIsXZ(bit); ++bit) trailing.setBit(bit, 1);
        V3_ସଂଖ୍ୟା from = flipped(range.lo);
        V3_ସଂଖ୍ୟା first{&hi, hi.width()};
        V3_ସଂଖ୍ୟା last{&hi, hi.width()};
        V3_ସଂଖ୍ୟା less{&hi};
        ଯେତେବେଳେ (runs.size() <= limit && next_Pattern_ମୂଲ୍ୟ(pattern, from, first)
               && less.opLt(hi, first).isEqZero()) {
            last.opOr(first, trailing);
            ଯଦି (!less.opLt(hi, last).isEqZero()) last = hi;
            runs.emplace_back(flipped(first), flipped(last));
            ଯଦି (last.isCaseEq(hi)) ଭାଙ୍ଗ;
            from.opAdd(last, one);
        }
    }

    // True if no coverpoint value participates in a resolved value or range.  A value with x
    // or z bits participates only as a wildcard pattern (IEEE 1800-2023 19.5.7).
    ସ୍ଥିର ତାର୍କିକ crossRangeEmpty(ଅଚଳ Cross_ମୂଲ୍ୟ_Range& range) {
        ଯଦି (range.lo.isFourState()) ଫେରନ୍ତୁ ସତ୍ୟ;
        ଫେରନ୍ତୁ cross_ମୂଲ୍ୟ_Less(range.hi, range.lo)
               || (range.wildcard && !crossWildcardIntersects(range));
    }

    // Comparison width that holds both the coverpoint's and a bin or intersect value's range.
    ସ୍ଥିର ପୂର୍ଣ୍ଣସଂଖ୍ୟା resolveWidth(Ast_ଗଣ୍ଠି* nodep, ଅଚଳ Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* exprp) {
        ଫେରନ୍ତୁ std::max(exprp->width(), crossRangeWidth(nodep)) + 1;
    }

    // Resolve a bin or intersect value to coverpoint values (IEEE 1800-2023 19.5.7), in the
    // width 'range' was created with.  False if the value is not a constant integral or real.
    ସ୍ଥିର ତାର୍କିକ resolve_ମୂଲ୍ୟ(Ast_ଗଣ୍ଠି* nodep, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* exprp, ତାର୍କିକ bin_ମୂଲ୍ୟ, ତାର୍କିକ wildcard,
                             Cross_ମୂଲ୍ୟ_Range& range) {
        ଅଚଳ Cross_ମୂଲ୍ୟ_Range domain
            = cross_ମୂଲ୍ୟ_Domain(nodep, exprp->width(), exprp->isSigned(), range.lo.width());
        ଫେରନ୍ତୁ cross_ମୂଲ୍ୟ_Range(nodep, exprp, bin_ମୂଲ୍ୟ, wildcard, domain, range);
    }

    ସ୍ଥିର ତାର୍କିକ crossRangesIntersect(ଅଚଳ Cross_ମୂଲ୍ୟ_Range& bin, ଅଚଳ Cross_ମୂଲ୍ୟ_Range& filter) {
        // Values with x or z bits do not participate, even in an identical filter.
        ଯଦି (bin.lo.isFourState() || filter.lo.isFourState()) ଫେରନ୍ତୁ ମିଥ୍ୟା;
        Cross_ମୂଲ୍ୟ_Range match = bin;
        intersectCrossRange(match, filter);
        ଫେରନ୍ତୁ !cross_ମୂଲ୍ୟ_Less(match.hi, match.lo)
               && (!bin.wildcard || crossWildcardIntersects(match));
    }

    ସ୍ଥିର ଶୂନ୍ୟ unsupportedCrossRange(AstCoverBinsof* selectp, ତାର୍କିକ& valid) {
        selectp->v3warn(COVERIGN, "Unsupported: non-constant or non-integral 'intersect' value, "
                                  "or four-state range bound.");
        valid = ମିଥ୍ୟା;
    }

    ସ୍ଥିର ତାର୍କିକ cross_ମୂଲ୍ୟ_Matches_Filters(AstCoverBinsof* selectp, Ast_ଗଣ୍ଠି* valuep,
                                         Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* exprp, ଅଚଳ AstCoverBin* binp,
                                         ଅଚଳ Cross_ମୂଲ୍ୟ_Range& domain,
                                         ଅଚଳ std::vector<Cross_ମୂଲ୍ୟ_Range>& filters,
                                         ତାର୍କିକ& valid) {
        Cross_ମୂଲ୍ୟ_Range range{valuep, domain.lo.width()};
        ଯଦି (!cross_ମୂଲ୍ୟ_Range(valuep, exprp, ସତ୍ୟ, binp->isWildcard(), domain, range)) {
            unsupportedCrossRange(selectp, valid);
            ଫେରନ୍ତୁ ମିଥ୍ୟା;
        }
        ପାଇଁ (ଅଚଳ Cross_ମୂଲ୍ୟ_Range& filter : filters) {
            ଯଦି (crossRangesIntersect(range, filter)) ଫେରନ୍ତୁ ସତ୍ୟ;
        }
        ଫେରନ୍ତୁ ମିଥ୍ୟା;
    }

    std::vector<ତାର୍କିକ> selectCoverpointBins(AstCoverBinsof* selectp, ଅଚଳ CoverpointBins& bins,
                                           uint32_t first, uint32_t ଗଣନା_2, ତାର୍କିକ& valid) {
        ଯଦି (selectp->rangesp() && !bins.exprp->dtypep()->skipRefp()->isIntegralOrPacked()) {
            unsupportedCrossRange(selectp, valid);
            ଫେରନ୍ତୁ {};
        }
        std::vector<ତାର୍କିକ> selected(bins.total, ମିଥ୍ୟା);
        std::vector<std::vector<Ast_ଗଣ୍ଠି*>> ମୂଲ୍ୟଗୁଡ଼ିକ;
        ପୂର୍ଣ୍ଣସଂଖ୍ୟା width = bins.exprp->width();
        ପାଇଁ (Ast_ଗଣ୍ଠି* rangep = selectp->rangesp(); rangep; rangep = rangep->nextp()) {
            width = std::max(width, crossRangeWidth(rangep));
        }
        ଯଦି (selectp->rangesp()) {
            ମୂଲ୍ୟଗୁଡ଼ିକ.reserve(ଗଣନା_2);
            ପାଇଁ (uint32_t i = first; i < first + ଗଣନା_2; ++i) {
                // A run's values are in the coverpoint type, whose width 'width' covers
                ମୂଲ୍ୟଗୁଡ଼ିକ.ଠେଲ_back(bins.ମୂଲ୍ୟଗୁଡ଼ିକ[i].runp ? std::vector<Ast_ଗଣ୍ଠି*>{}
                                                     : cross_Bin_ମୂଲ୍ୟଗୁଡ଼ିକ(bins.ମୂଲ୍ୟଗୁଡ଼ିକ[i]));
                ପାଇଁ (Ast_ଗଣ୍ଠି* ଅଚଳ valuep : ମୂଲ୍ୟଗୁଡ଼ିକ.back()) {
                    width = std::max(width, crossRangeWidth(valuep));
                }
            }
        }
        // One extra bit preserves both unsigned maxima and negative signed bounds.
        ++width;
        ଅଚଳ Cross_ମୂଲ୍ୟ_Range domain
            = cross_ମୂଲ୍ୟ_Domain(selectp, bins.exprp->width(), bins.exprp->isSigned(), width);
        std::vector<Cross_ମୂଲ୍ୟ_Range> filters;
        ପାଇଁ (Ast_ଗଣ୍ଠି* rangep = selectp->rangesp(); rangep; rangep = rangep->nextp()) {
            Cross_ମୂଲ୍ୟ_Range filter{rangep, width};
            ଯଦି (!cross_ମୂଲ୍ୟ_Range(rangep, bins.exprp, ମିଥ୍ୟା, ମିଥ୍ୟା, domain, filter)) {
                unsupportedCrossRange(selectp, valid);
                ଫେରନ୍ତୁ {};
            }
            filters.ଠେଲ_back(std::move(filter));
        }
        ପାଇଁ (uint32_t i = first; i < first + ଗଣନା_2; ++i) {
            ଯଦି (!selectp->rangesp()) {
                selected[i] = ସତ୍ୟ;
                ଜାରି;
            }
            ଯଦି (ଅଚଳ BinRun* ଅଚଳ runp = bins.ମୂଲ୍ୟଗୁଡ଼ିକ[i].runp) {
                ଅଚଳ Cross_ମୂଲ୍ୟ_Range range
                    = runBinRange(selectp, *runp, bins.ମୂଲ୍ୟଗୁଡ଼ିକ[i].element, width);
                selected[i] = std::any_of(filters.begin(), filters.end(),
                                          [&](ଅଚଳ Cross_ମୂଲ୍ୟ_Range& filter) {
                                              ଫେରନ୍ତୁ crossRangesIntersect(range, filter);
                                          });
                ଜାରି;
            }
            ପାଇଁ (Ast_ଗଣ୍ଠି* ଅଚଳ valuep : ମୂଲ୍ୟଗୁଡ଼ିକ[i - first]) {
                selected[i] = cross_ମୂଲ୍ୟ_Matches_Filters(
                    selectp, valuep, bins.exprp, bins.ମୂଲ୍ୟଗୁଡ଼ିକ[i].binp, domain, filters, valid);
                ଯଦି (!valid) ଫେରନ୍ତୁ {};
                ଯଦି (selected[i]) ଭାଙ୍ଗ;
            }
        }
        ଯଦି (selectp->isNegated()) {
            ପାଇଁ (uint32_t i = 0; i < bins.total; ++i) selected[i] = !selected[i];
        }
        ଫେରନ୍ତୁ selected;
    }

    // Constructor-time value metadata of one coverpoint, as C++ list entries
    ସଂରଚନା ମୂଲ୍ୟ_Lists final {
        std::vector<std::string> m_ranges;  // Bin, then low and high words
        std::vector<std::string> m_runs;  // First bin, count, then low, span, and high words
        std::vector<std::string> m_patterns;  // Bin, then value, mask, low, and high words
        std::vector<std::string> m_transitions;  // Transition bin
    };

    // Append a value's words, in the coverpoint's width, to a C++ list entry.
    ସ୍ଥିର ଶୂନ୍ୟ append_ଶବ୍ଦଗୁଡ଼ିକ(std::string& text, ଅଚଳ V3_ସଂଖ୍ୟା& ମୂଲ୍ୟ, ଅଚଳ Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* exprp) {
        V3_ସଂଖ୍ୟା narrowed{&ମୂଲ୍ୟ, exprp->width()};
        narrowed.opAssign(ମୂଲ୍ୟ);
        ପାଇଁ (ପୂର୍ଣ୍ଣସଂଖ୍ୟା word = 0; word < exprp->width_ଶବ୍ଦଗୁଡ଼ିକ(); ++word) {
            text += ", " + cvtToStr(narrowed.edataWord(word)) + "U";
        }
    }

    ଶୂନ୍ୟ collect_ମୂଲ୍ୟ_Metadata(ମୂଲ୍ୟ_Lists& lists, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* exprp, AstCoverBin* binp,
                              uint32_t index, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* valuep) {
        ଅଚଳ std::string bin = cvtToStr(index) + "U";
        ଯଦି (binp->transp()) lists.m_transitions.ଠେଲ_back(bin);
        ପାଇଁ (Ast_ଗଣ୍ଠି* ଅଚଳ sourcep : cross_Bin_ମୂଲ୍ୟଗୁଡ଼ିକ({binp, valuep})) {
            Cross_ମୂଲ୍ୟ_Range range{sourcep, resolveWidth(sourcep, exprp)};
            ଯଦି (!resolve_ମୂଲ୍ୟ(sourcep, exprp, ସତ୍ୟ, binp->isWildcard(), range)) {
                // Sampling already resolved every state bin value, so only transitions remain.
                sourcep->v3warn(E_UNSUPPORTED, "Unsupported: non-integral value in a transition "
                                               "bin of a coverpoint with exclusions.");
                ଜାରି;
            }
            ଯଦି (crossRangeEmpty(range)) ଜାରି;
            std::string entry = bin;
            ଯଦି (range.wildcard) {
                V3_ସଂଖ୍ୟା ମୂଲ୍ୟ{sourcep, exprp->width()};
                V3_ସଂଖ୍ୟା mask{sourcep, exprp->width()};
                mask.opBitsNonXZ(range.pattern);
                ମୂଲ୍ୟ.opBitsOne(range.pattern);
                append_ଶବ୍ଦଗୁଡ଼ିକ(entry, ମୂଲ୍ୟ, exprp);
                append_ଶବ୍ଦଗୁଡ଼ିକ(entry, mask, exprp);
            }
            append_ଶବ୍ଦଗୁଡ଼ିକ(entry, range.lo, exprp);
            append_ଶବ୍ଦଗୁଡ଼ିକ(entry, range.hi, exprp);
            (range.wildcard ? lists.m_patterns : lists.m_ranges).ଠେଲ_back(entry);
        }
    }

    // Describe a run with one entry, from which the runtime computes the values of its bins.
    ସ୍ଥିର ଶୂନ୍ୟ collectRunMetadata(ମୂଲ୍ୟ_Lists& lists, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* exprp, ଅଚଳ BinRun& run) {
        V3_ସଂଖ୍ୟା span{exprp, run.m_stride.width(), 0};
        span.opSub(run.m_stride, V3_ସଂଖ୍ୟା{exprp, run.m_stride.width(), 1});
        std::string entry = cvtToStr(run.m_declared) + "U, " + cvtToStr(run.m_ଗଣନା) + "U";
        append_ଶବ୍ଦଗୁଡ଼ିକ(entry, run.m_lo, exprp);
        append_ଶବ୍ଦଗୁଡ଼ିକ(entry, span, exprp);
        append_ଶବ୍ଦଗୁଡ଼ିକ(entry, run.m_hi, exprp);
        lists.m_runs.ଠେଲ_back(entry);
    }

    // Emit one batched metadata list, bounding the size of each call's temporary list.
    ଶୂନ୍ୟ emit_ମୂଲ୍ୟ_List(File_ଧାଡ଼ି* fl, AstVar* cpVarp, VCMethod method,
                       ଅଚଳ std::vector<std::string>& entries) {
        ପାଇଁ (size_t first = 0; first < entries.size(); first += ମୂଲ୍ୟ_LIST_ENTRIES) {
            ଅଚଳ size_t end = std::min(entries.size(), first + ମୂଲ୍ୟ_LIST_ENTRIES);
            std::string text = "{" + entries[first];
            ପାଇଁ (size_t i = first + 1; i < end; ++i) text += ", " + entries[i];
            m_constructorp->addStmtsp(
                itemCall(fl, cpVarp, method, {ctext(fl, text + "}")})->makeStmt());
        }
    }

    ସ୍ଥିର ତାର୍କିକ checkCrossRef(ଅଚଳ AstCoverCrossRef* refp, ଅଚଳ AstCoverCross* crossp) {
        ଯଦି (refp->name() == crossp->name()) ଫେରନ୍ତୁ ସତ୍ୟ;
        refp->v3error("Cross selection "
                      << refp->prettyNameQ() << " may only name its enclosing cross "
                      << crossp->prettyNameQ() << " (IEEE 1800-2023 19.6.1.2).");
        ଫେରନ୍ତୁ ମିଥ୍ୟା;
    }

    ସ୍ଥିର ତାର୍କିକ checkCrossBinName(ଅଚଳ AstCoverCrossBin* binp, std::set<std::string>& names) {
        ଯଦି (names.emplace(binp->name()).second) ଫେରନ୍ତୁ ସତ୍ୟ;
        binp->v3error("Duplicate cross bin " << binp->prettyNameQ()
                                             << " (IEEE 1800-2023 19.6.1).");
        ଫେରନ୍ତୁ ମିଥ୍ୟା;
    }

    // The span of the implicit automatic bin reported as 'name' ('auto_<i>', see
    // createImplicitAutoBins), found without naming each of its bins
    ସ୍ଥିର ତାର୍କିକ implicitAutoBinSpan(ଅଚଳ CoverpointBins& bins, ଅଚଳ std::string& name,
                                    BinSpan& span) {
        ଅଚଳ std::string prefix = "auto_";
        ଯଦି (!VString::startsWith(name, prefix)) ଫେରନ୍ତୁ ମିଥ୍ୟା;
        ଅଚଳ std::string digits = name.substr(prefix.size());
        ଅଚଳ unsigned long index = std::strtoul(digits.c_str(), nullptr, 10);
        // Only the reported spelling names the bin, not e.g. 'auto_01' or 'auto_x'
        ଯଦି (index >= bins.implicitAuto.ଗଣନା_2 || digits != cvtToStr(index)) ଫେରନ୍ତୁ ମିଥ୍ୟା;
        ଅଚଳ uint32_t offset = static_cast<uint32_t>(index);
        span = BinSpan{bins.implicitAuto.first + offset, 1, bins.implicitAuto.declared + offset};
        ଫେରନ୍ତୁ ସତ୍ୟ;
    }

    CrossBinsofTarget
    resolveBinsofTarget(ଅଚଳ AstCoverBinsof* selectp, ଅଚଳ AstCoverCross* crossp,
                        ଅଚଳ std::vector<AstVar*>& cpVars,
                        ଅଚଳ std::map<std::string, uint32_t>& dimensions) ଅଚଳ {
        ଅଚଳ auto dim = dimensions.find(selectp->pointp()->name());
        ଯଦି (dim == dimensions.end()) {
            selectp->v3error("binsof coverpoint "
                             << selectp->pointp()->prettyNameQ() << " is not an item of cross "
                             << crossp->prettyNameQ() << " (IEEE 1800-2023 19.6.1).");
            ଫେରନ୍ତୁ {};
        }
        ଅଚଳ CoverpointBins& bins = m_cpBins.at(cpVars[dim->second]);
        CrossBinsofTarget target{&bins, dim->second, 0, bins.total};
        ଯଦି (!selectp->name().empty()) {
            ଅଚଳ auto bin = bins.spans.find(selectp->name());
            BinSpan span{0, 0, 0};
            ଯଦି (bin != bins.spans.end()) {
                span = bin->second;
            } ଅନ୍ୟଥା ଯଦି (!implicitAutoBinSpan(bins, selectp->name(), span)) {
                selectp->v3error("Cannot find bin " << selectp->prettyNameQ() << " in coverpoint "
                                                    << selectp->pointp()->prettyNameQ()
                                                    << " (IEEE 1800-2023 19.6.1).");
                ଫେରନ୍ତୁ {};
            }
            target.m_first = span.first;
            target.m_ଗଣନା = span.ଗଣନା_2;
            target.m_declaredFirst = span.declared;
            target.m_declaredEnd = span.declared + span.ଗଣନା_2;
            target.m_sized = span.sized;
        }
        ଫେରନ୍ତୁ target;
    }

    // The variables a 'with' filter reads, each by its first reference
    using CrossFilterReads = std::map<ଅଚଳ AstVar*, AstVarRef*>;

    // Whether a 'with' filter has a count of value tuples, of 'matches' other than '$'
    ସ୍ଥିର ତାର୍କିକ counts_ମୂଲ୍ୟ_Tuples(ଅଚଳ AstCoverCrossWith* withp) {
        ଫେରନ୍ତୁ withp->countp() && !VN_IS(withp->countp(), Unbounded);
    }

    // Whether the runtime knows the values of the cross items a 'with' filter reads, or to count
    // value tuples, of all items, of integral coverpoints; else warns
    ସ୍ଥିର ତାର୍କିକ crossFilterSupported(ଅଚଳ AstCoverCrossWith* withp,
                                     ଅଚଳ CrossFilterReads& reads) {
        ପାଇଁ (ଅଚଳ AstVar* varp = withp->varsp(); varp; varp = VN_AS(varp->nextp(), Var)) {
            ଯଦି (varp->dtypep()->skipRefp()->isIntegralOrPacked()) ଜାରି;
            ଅଚଳ auto it = reads.find(varp);
            ଯଦି (it != reads.end()) {
                it->second->v3warn(COVERIGN, "Unsupported: cross bin 'with' filter reading the "
                                             "value of non-integral coverpoint "
                                                 << it->second->prettyNameQ());
                ଫେରନ୍ତୁ ମିଥ୍ୟା;
            }
            ଯଦି (counts_ମୂଲ୍ୟ_Tuples(withp)) {
                withp->countp()->v3warn(COVERIGN, "Unsupported: cross bin 'matches' count of "
                                                  "value tuples of non-integral coverpoint "
                                                      << varp->prettyNameQ());
                ଫେରନ୍ତୁ ମିଥ୍ୟା;
            }
        }
        ଫେରନ୍ତୁ ସତ୍ୟ;
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
    ଶୂନ୍ୟ generateCrossFilter(ଅଚଳ AstCoverCrossWith* withp, ଅଚଳ CrossFilterReads& reads,
                             ଅଚଳ AstCoverCrossBin* binp, ଅଚଳ AstCoverCross* crossp,
                             AstVar* cxp, ଅଚଳ std::map<string, uint32_t>& dimensions,
                             uint32_t ବସ୍ତୁଗୁଡ଼ିକ) {
        File_ଧାଡ଼ି* ଅଚଳ fl = withp->fileline();
        ଅଚଳ string prefix
            = m_sizedNames.get(sanitizeGeneratedName(crossp->name() + "__" + binp->name()));
        ଅଚଳ ତାର୍କିକ counting = counts_ମୂଲ୍ୟ_Tuples(withp);
        // The count, 0 for all
        ଅଚଳ std::pair<Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି*, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି*> ଗଣନା_2
            = counting ? new_ଗଣନା_Args(fl, prefix + "_count", withp->countp())
                       : new_ଗଣନା_Args(fl, withp->countp() ? 0 : 1);
        AstLoop* ଅଚଳ loopp = ନୂତନ AstLoop{fl};
        AstCMethodHard* ଅଚଳ nextp = itemCall(fl, cxp, VCMethod::COVERGROUP_WITH_NEXT);
        nextp->dtypeSetBit();
        loopp->addStmtsp(ନୂତନ AstLoopTest{fl, loopp, nextp});
        string dims;
        ପାଇଁ (uint32_t dim = 0; counting && dim < ବସ୍ତୁଗୁଡ଼ିକ; ++dim) {
            dims += (dims.empty() ? "" : ", ") + cvtToStr(dim) + "U";
        }
        ଯେତେବେଳେ (AstVar* ଅଚଳ varp = withp->varsp()) {
            varp->unlinkFrBack();
            ଯଦି (!reads.ଗଣନା_2(varp)) {
                VL_DO_DANGLING(ଠେଲ_Deletep(varp), varp);
                ଜାରି;
            }
            ଅଚଳ auto dimIt = dimensions.find(varp->name());
            UASSERT_OBJ(dimIt != dimensions.end(), varp, "Cross 'with' candidate not of an item");
            ଅଚଳ uint32_t dim = dimIt->second;
            ଯଦି (!counting) dims += (dims.empty() ? "" : ", ") + cvtToStr(dim) + "U";
            varp->name(prefix + "_" + sanitizeGeneratedName(varp->name()));
            m_constructorp->addStmtsp(varp);
            ଯଦି (varp->isWide()) {
                loopp->addStmtsp(itemCall(fl, cxp, VCMethod::COVERGROUP_CROSS_WITH_ମୂଲ୍ୟ_W,
                                          {cnum(fl, dim), ନୂତନ AstVarRef{fl, varp, VAccess::WRITE}})
                                     ->makeStmt());
            } ଅନ୍ୟଥା {
                AstCMethodHard* ଅଚଳ valuep
                    = itemCall(fl, cxp, VCMethod::COVERGROUP_CROSS_WITH_ମୂଲ୍ୟ, {cnum(fl, dim)});
                valuep->dtypeSetUInt64();
                Ast_ଗଣ୍ଠି_DType* ଅଚଳ dtypep = varp->findLogicDType(
                    varp->width(), varp->width(),
                    varp->isSigned() ? VSigning::SIGNED : VSigning::UNSIGNED);
                loopp->addStmtsp(ନୂତନ AstAssign{fl, ନୂତନ AstVarRef{fl, varp, VAccess::WRITE},
                                               resize_ମୂଲ୍ୟ(valuep, dtypep)});
            }
        }
        loopp->addStmtsp(itemCall(fl, cxp, VCMethod::COVERGROUP_CROSS_WITH_TEST,
                                  {withp->filterp()->unlinkFrBack()})
                             ->makeStmt());
        ଅଚଳ ତାର୍କିକ prot = v3Global.opt.protectIds();
        m_constructorp->addStmtsp(
            itemCall(fl, cxp, VCMethod::COVERGROUP_CROSS_WITH_BEGIN,
                     {ctext(fl, "{" + dims + "}"), ଗଣନା_2.first, ଗଣନା_2.second,
                      ctext(fl, quoted(VIdProtect::protect_ଶବ୍ଦଗୁଡ଼ିକ_If(binp->name(), prot))),
                      ctext(fl, quoted(VIdProtect::protectIf(fl->filename(), prot))),
                      cnum(fl, static_cast<uint32_t>(fl->lineno()))})
                ->makeStmt());
        m_constructorp->addStmtsp(loopp);
        m_constructorp->addStmtsp(
            itemCall(fl, cxp, VCMethod::COVERGROUP_CROSS_WITH_END)->makeStmt());
    }

    ତାର୍କିକ generateRuntimeSelection(Ast_ଗଣ୍ଠି* nodep, ଅଚଳ AstCoverCrossBin* binp,
                                  AstCoverCross* crossp, AstVar* cxp,
                                  ଅଚଳ std::vector<AstVar*>& cpVars,
                                  ଅଚଳ std::map<string, uint32_t>& dimensions) {
        File_ଧାଡ଼ି* ଅଚଳ fl = nodep->fileline();
        ଯଦି (ଅଚଳ AstCoverCrossRef* ଅଚଳ refp = VN_CAST(nodep, CoverCrossRef)) {
            ଯଦି (!checkCrossRef(refp, crossp)) ଫେରନ୍ତୁ ମିଥ୍ୟା;
            m_constructorp->addStmtsp(
                itemCall(fl, cxp, VCMethod::COVERGROUP_SELECT_ALL)->makeStmt());
            ଫେରନ୍ତୁ ସତ୍ୟ;
        }
        ଯଦି (ଅଚଳ AstCoverCrossSelect* ଅଚଳ opp = VN_CAST(nodep, CoverCrossSelect)) {
            ଯଦି (!generateRuntimeSelection(opp->lhsp(), binp, crossp, cxp, cpVars, dimensions)
                || !generateRuntimeSelection(opp->rhsp(), binp, crossp, cxp, cpVars, dimensions)) {
                ଫେରନ୍ତୁ ମିଥ୍ୟା;
            }
            m_constructorp->addStmtsp(itemCall(fl, cxp,
                                               opp->isOr() ? VCMethod::COVERGROUP_SELECT_OR
                                                           : VCMethod::COVERGROUP_SELECT_AND)
                                          ->makeStmt());
            ଫେରନ୍ତୁ ସତ୍ୟ;
        }
        ଯଦି (ଅଚଳ AstCoverCrossWith* ଅଚଳ withp = VN_CAST(nodep, CoverCrossWith)) {
            CrossFilterReads reads;
            withp->filterp()->foreach([&](AstVarRef* refp) { reads.emplace(refp->varp(), refp); });
            // The count of 'matches' is a positive integer, or '$' (IEEE 1800-2023 19.6.1.2)
            ଯଦି ((counts_ମୂଲ୍ୟ_Tuples(withp)
                 && !check_ଗଣନା(withp->countp(), "Cross bin 'matches' count", "19.6.1.2"))
                || !crossFilterSupported(withp, reads)
                || !generateRuntimeSelection(withp->selectp(), binp, crossp, cxp, cpVars,
                                             dimensions)) {
                ଫେରନ୍ତୁ ମିଥ୍ୟା;
            }
            generateCrossFilter(withp, reads, binp, crossp, cxp, dimensions,
                                static_cast<uint32_t>(cpVars.size()));
            ଫେରନ୍ତୁ ସତ୍ୟ;
        }
        AstCoverBinsof* ଅଚଳ selectp = VN_AS(nodep, CoverBinsof);
        ଅଚଳ CrossBinsofTarget target = resolveBinsofTarget(selectp, crossp, cpVars, dimensions);
        ଯଦି (!target.m_binsp) ଫେରନ୍ତୁ ମିଥ୍ୟା;
        ଅଚଳ CoverpointBins& bins = *target.m_binsp;
        ଯଦି (selectp->rangesp() && !bins.exprp->dtypep()->skipRefp()->isIntegralOrPacked()) {
            ତାର୍କିକ valid = ସତ୍ୟ;
            unsupportedCrossRange(selectp, valid);
            ଫେରନ୍ତୁ ମିଥ୍ୟା;
        }
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* firstp;
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* endp;
        ଯଦି (target.m_sized < 0) {
            firstp = cnum(fl, target.m_declaredFirst);
            endp = cnum(fl, target.m_declaredEnd);
        } ଅନ୍ୟଥା {  // A sized array, whose bins the coverpoint's construction placed
            AstVar* ଅଚଳ cpVarp = cpVars[target.m_dimension];
            ଅଚଳ uint32_t sized = static_cast<uint32_t>(target.m_sized);
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
        ପାଇଁ (Ast_ଗଣ୍ଠି* rangep = selectp->rangesp(); rangep; rangep = rangep->nextp()) {
            Cross_ମୂଲ୍ୟ_Range range{rangep, resolveWidth(rangep, bins.exprp)};
            ଯଦି (!resolve_ମୂଲ୍ୟ(rangep, bins.exprp, ମିଥ୍ୟା, ମିଥ୍ୟା, range)) {
                ତାର୍କିକ valid = ସତ୍ୟ;
                unsupportedCrossRange(selectp, valid);
                ଫେରନ୍ତୁ ମିଥ୍ୟା;
            }
            ଯଦି (crossRangeEmpty(range)) ଜାରି;
            m_constructorp->addStmtsp(itemCall(fl, cxp,
                                               bins.exprp->isWide()
                                                   ? VCMethod::COVERGROUP_SELECT_RANGE_W
                                                   : VCMethod::COVERGROUP_SELECT_RANGE,
                                               {new_ମୂଲ୍ୟ_Const(fl, range.lo, bins.exprp),
                                                new_ମୂଲ୍ୟ_Const(fl, range.hi, bins.exprp)})
                                          ->makeStmt());
        }
        m_constructorp->addStmtsp(
            itemCall(fl, cxp, VCMethod::COVERGROUP_SELECT_DIM_END)->makeStmt());
        ଫେରନ୍ତୁ ସତ୍ୟ;
    }

    std::vector<AstCoverCrossBin*>
    generateRuntimeCrossBins(AstCoverCross* crossp, AstVar* cxp,
                             ଅଚଳ std::vector<AstVar*>& cpVars,
                             ଅଚଳ std::map<string, uint32_t>& dimensions) {
        std::vector<AstCoverCrossBin*> bins;
        std::set<string> names;
        ପାଇଁ (Ast_ଗଣ୍ଠି* itemp = crossp->binsp(); itemp; itemp = itemp->nextp()) {
            AstCoverCrossBin* ଅଚଳ binp = VN_AS(itemp, CoverCrossBin);
            ଯଦି (!checkCrossBinName(binp, names)) ଜାରି;
            File_ଧାଡ଼ି* ଅଚଳ fl = binp->fileline();
            // Generate the selection apart, so that an unsupported one leaves no code, which
            // could leave its state for the next bin
            AstFunc* ଅଚଳ selectionp = ନୂତନ AstFunc{fl, "", nullptr, nullptr};
            ତାର୍କିକ supported;
            {
                VL_RESTORER(m_constructorp);
                m_constructorp = selectionp;
                supported = generateRuntimeSelection(binp->selectp(), binp, crossp, cxp, cpVars,
                                                     dimensions);
            }
            // A supported selection has code
            ଯଦି (supported) m_constructorp->addStmtsp(selectionp->stmtsp()->unlinkFrBackWithNext());
            VL_DO_DANGLING(ଠେଲ_Deletep(selectionp), selectionp);
            ଯଦି (!supported) ଜାରି;
            ଅଚଳ ତାର୍କିକ protect = v3Global.opt.protectIds();
            m_constructorp->addStmtsp(
                itemCall(fl, cxp, VCMethod::COVERGROUP_SELECT_BIN,
                         {ctext(fl, binp->binsType().binSetEnum()),
                          ctext(fl, quoted(VIdProtect::protect_ଶବ୍ଦଗୁଡ଼ିକ_If(binp->name(), protect))),
                          ctext(fl, quoted(VIdProtect::protectIf(fl->filename(), protect))),
                          cnum(fl, fl->lineno()), cnum(fl, fl->firstColumn()),
                          cnum(fl, static_cast<uint32_t>(bins.size()))})
                    ->makeStmt());
            bins.ଠେଲ_back(binp);
        }
        m_constructorp->addStmtsp(
            itemCall(crossp->fileline(), cxp, VCMethod::COVERGROUP_FINALIZE_BINS)->makeStmt());
        ଫେରନ୍ତୁ bins;
    }

    ସ୍ଥିର ଶୂନ୍ୟ setCrossSelectionRange(CrossSelection& selection, uint64_t first, uint64_t end) {
        ଯେତେବେଳେ (first < end) {
            ଅଚଳ unsigned bit = first % 64;
            ଅଚଳ unsigned bits = std::min<uint64_t>(64 - bit, end - first);
            selection[VL_BITWORD_Q(first)]
                |= (bits == 64 ? ~uint64_t{0} : (uint64_t{1} << bits) - 1) << bit;
            first += bits;
        }
    }

    CrossSelection crossSelection(Ast_ଗଣ୍ଠି* nodep, CrossSelectionContext& ctx) {
        ଯଦି (ଅଚଳ AstCoverCrossRef* ଅଚଳ refp = VN_CAST(nodep, CoverCrossRef)) {
            ଯଦି (!checkCrossRef(refp, ctx.crossp)) {
                ctx.valid = ମିଥ୍ୟା;
                ଫେରନ୍ତୁ {};
            }
            CrossSelection result(
                VL_BITWORD_Q(static_cast<uint64_t>(ctx.tuples) + VL_QUADSIZE - 1), 0);
            setCrossSelectionRange(result, 0, ctx.tuples);
            ଫେରନ୍ତୁ result;
        }
        ଯଦି (AstCoverCrossSelect* ଅଚଳ opp = VN_CAST(nodep, CoverCrossSelect)) {
            CrossSelection lhs = crossSelection(opp->lhsp(), ctx);
            ଅଚଳ CrossSelection rhs = crossSelection(opp->rhsp(), ctx);
            ଯଦି (!ctx.valid) ଫେରନ୍ତୁ {};
            ପାଇଁ (size_t i = 0; i < lhs.size(); ++i) {
                lhs[i] = opp->isOr() ? lhs[i] | rhs[i] : lhs[i] & rhs[i];
            }
            ଫେରନ୍ତୁ lhs;
        }
        AstCoverBinsof* ଅଚଳ selectp = VN_AS(nodep, CoverBinsof);
        ଅଚଳ CrossBinsofTarget target
            = resolveBinsofTarget(selectp, ctx.crossp, ctx.cpVars, ctx.dimensions);
        ଯଦି (!target.m_binsp) {
            ctx.valid = ମିଥ୍ୟା;
            ଫେରନ୍ତୁ {};
        }
        ଅଚଳ CoverpointBins& bins = *target.m_binsp;
        // Coverpoints with sized arrays are runtime points, so feed only runtime crosses
        UASSERT_OBJ(target.m_sized < 0, selectp, "Sized bin array selected by a static cross");
        ଅଚଳ std::vector<ତାର୍କିକ> selected
            = selectCoverpointBins(selectp, bins, target.m_first, target.m_ଗଣନା, ctx.valid);
        ଯଦି (!ctx.valid) ଫେରନ୍ତୁ {};
        CrossSelection result(VL_BITWORD_Q(static_cast<uint64_t>(ctx.tuples) + VL_QUADSIZE - 1),
                              0);
        ଅଚଳ uint64_t stride = ctx.strides[target.m_dimension];
        ଅଚଳ uint64_t period = stride * bins.total;
        ପାଇଁ (uint64_t base = 0; base < ctx.tuples; base += period) {
            ପାଇଁ (uint32_t i = 0; i < bins.total;) {
                ଯଦି (!selected[i]) {
                    ++i;
                    ଜାରି;
                }
                ଅଚଳ uint32_t begin = i++;
                ଯେତେବେଳେ (i < bins.total && selected[i]) ++i;
                setCrossSelectionRange(result, base + begin * stride, base + i * stride);
            }
        }
        ଫେରନ୍ତୁ result;
    }

    // Size the Cartesian product of the declared Normal bins, with each dimension's flat-index
    // stride.  Live runtime bins only shrink it.  Warns and returns false if it is too large.
    ତାର୍କିକ crossShape(AstCoverCross* crossp, ଅଚଳ std::vector<AstVar*>& cpVars,
                    std::vector<uint32_t>& strides, uint32_t& tuples) ଅଚଳ {
        uint64_t product = std::any_of(cpVars.begin(), cpVars.end(),
                                       [ଏହି](AstVar* varp) { ଫେରନ୍ତୁ !m_cpBins.at(varp).total; })
                               ? 0
                               : 1;
        strides.resize(cpVars.size());
        ପାଇଁ (size_t d = cpVars.size(); d > 0; --d) {
            strides[d - 1] = static_cast<uint32_t>(product);
            product *= m_cpBins.at(cpVars[d - 1]).total;
            ଯଦି (product > UINT32_MAX) {
                crossp->v3warn(COVERIGN,
                               "Unsupported: cross coverage with more than 2^32-1 tuples.");
                ଫେରନ୍ତୁ ମିଥ୍ୟା;
            }
        }
        tuples = static_cast<uint32_t>(product);
        ଫେରନ୍ତୁ ସତ୍ୟ;
    }

    CrossLayout resolveCrossLayout(AstCoverCross* crossp, ଅଚଳ std::vector<AstVar*>& cpVars,
                                   ଅଚଳ std::map<std::string, uint32_t>& dimensions) {
        CrossLayout layout;
        CrossSelectionContext ctx{crossp, cpVars, dimensions, 0, {}};
        ଯଦି (!crossShape(crossp, cpVars, ctx.strides, ctx.tuples)) {
            layout.valid = ମିଥ୍ୟା;
            ଫେରନ୍ତୁ layout;
        }
        layout.tuples = ctx.tuples;
        CrossSelection occupied;
        CrossSelection excluded;
        std::set<std::string> names;
        ପାଇଁ (Ast_ଗଣ୍ଠି* itemp = crossp->binsp(); itemp; itemp = itemp->nextp()) {
            AstCoverCrossBin* ଅଚଳ binp = VN_AS(itemp, CoverCrossBin);
            ଯଦି (!checkCrossBinName(binp, names)) ଜାରି;
            ctx.valid = ସତ୍ୟ;
            CrossSelection selection = crossSelection(binp->selectp(), ctx);
            ଯଦି (!ctx.valid || std::all_of(selection.begin(), selection.end(), [](uint64_t word) {
                    ଫେରନ୍ତୁ word == 0;
                })) {
                ଜାରି;
            }
            ଯଦି (occupied.empty()) {
                occupied.resize(selection.size(), 0);
                excluded.resize(selection.size(), 0);
            }
            ପାଇଁ (size_t i = 0; i < selection.size(); ++i) {
                occupied[i] |= selection[i];
                ଯଦି (!binp->binsType().binIsNormal()) excluded[i] |= selection[i];
            }
            layout.bins.ଠେଲ_back({binp, std::move(selection)});
        }
        ଯଦି (!layout.bins.empty()) {
            // IEEE 1800-2023 19.6.2/19.6.3: exclusions also remove tuples from named
            // bins, independently of declaration order and sampling guards.
            ପାଇଁ (ResolvedCrossBin& resolved : layout.bins) {
                ପାଇଁ (size_t i = 0; i < resolved.selection.size(); ++i) {
                    ଯଦି (resolved.binp->binsType().binIsNormal()) {
                        resolved.selection[i] &= ~excluded[i];
                    }
                    ଯଦି (resolved.selection[i]) ++layout.bin_ଶବ୍ଦଗୁଡ଼ିକ;
                }
            }
            layout.bins.erase(std::remove_if(layout.bins.begin(), layout.bins.end(),
                                             [](ଅଚଳ ResolvedCrossBin& resolved) {
                                                 ଫେରନ୍ତୁ std::all_of(
                                                     resolved.selection.begin(),
                                                     resolved.selection.end(),
                                                     [](uint64_t word) { ଫେରନ୍ତୁ word == 0; });
                                             }),
                              layout.bins.end());
            layout.autoBins = layout.tuples;
            ପାଇଁ (ଅଚଳ uint64_t word : occupied) {
                layout.autoBins -= static_cast<uint32_t>(std::bitset<VL_QUADSIZE>{word}.ଗଣନା_2());
            }
        }
        ଫେରନ୍ତୁ layout;
    }

    AstCoverCrossDType* crossDType(File_ଧାଡ଼ି* fl, uint32_t dimensions, ଅଚଳ CrossLayout& layout,
                                   ତାର୍କିକ dynamic = ମିଥ୍ୟା) {
        ଅଚଳ uint32_t bins = static_cast<uint32_t>(layout.bins.size());
        ଅଚଳ CrossShape shape{dimensions,      layout.tuples,   bins,
                               layout.autoBins, layout.bin_ଶବ୍ଦଗୁଡ଼ିକ, dynamic};
        AstCoverCrossDType*& typep = m_cxDTypes[shape];
        ଯଦି (!typep) {
            typep = ନୂତନ AstCoverCrossDType{
                fl, dimensions, layout.tuples, bins, layout.autoBins, layout.bin_ଶବ୍ଦଗୁଡ଼ିକ, dynamic};
            v3Global.rootp()->typeTablep()->addTypesp(typep);
        }
        ଫେରନ୍ତୁ typep;
    }

    std::vector<AstCoverCrossBin*> generateCrossBins(AstCoverCross* crossp, AstVar* cxVarp,
                                                     ଅଚଳ CrossLayout& layout) {
        std::vector<AstCoverCrossBin*> bins;
        ପାଇଁ (ଅଚଳ ResolvedCrossBin& resolved : layout.bins) {
            AstCoverCrossBin* ଅଚଳ binp = resolved.binp;
            ଅଚଳ CrossSelection& selection = resolved.selection;
            File_ଧାଡ଼ି* ଅଚଳ fl = binp->fileline();
            ଅଚଳ ତାର୍କିକ prot = v3Global.opt.protectIds();
            std::string mask = "{";
            ପାଇଁ (size_t i = 0; i < selection.size(); ++i) {
                ଯଦି (i) mask += ", ";
                mask += std::to_string(selection[i]) + "ULL";
            }
            mask += "}";
            m_constructorp->addStmtsp(
                itemCall(fl, cxVarp, VCMethod::COVERGROUP_ADD_BIN,
                         {ctext(fl, binp->binsType().binSetEnum()), ctext(fl, mask),
                          ctext(fl, quoted(VIdProtect::protect_ଶବ୍ଦଗୁଡ଼ିକ_If(binp->name(), prot))),
                          ctext(fl, quoted(VIdProtect::protectIf(fl->filename(), prot))),
                          cnum(fl, static_cast<uint32_t>(fl->lineno())),
                          cnum(fl, static_cast<uint32_t>(fl->firstColumn()))})
                    ->makeStmt());
            bins.ଠେଲ_back(binp);
        }
        ଯଦି (!bins.empty()) {
            m_constructorp->addStmtsp(
                itemCall(crossp->fileline(), cxVarp, VCMethod::COVERGROUP_FINALIZE_BINS)
                    ->makeStmt());
        }
        ଫେରନ୍ତୁ bins;
    }

    // Route a cross through a VlCoverCross member: emit the member, its constructor init +
    // registration, and the sample() call.  The feeding coverpoints are already generated
    // (their hit lists drive the cross). Each explicit bin adds one configuration call.
    ଶୂନ୍ୟ generateCross(AstCoverCross* crossp) {
        File_ଧାଡ଼ି* ଅଚଳ fl = crossp->fileline();
        ଅଚଳ ତାର୍କିକ dynamic = m_runtimeCrosses.ଗଣନା_2(crossp);
        UINFO(4, "  Generating VlCoverCross member: " << crossp->name());

        ଯଦି (Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ iffp = crossp->iffp()) {
            crossp->iffp(
                captureIffToTemp(iffp, "__VcrossIff_" + sanitizeGeneratedName(crossp->name())));
        }

        // Resolve and unlink the coverpoint refs, in dimension order.  Every ref resolves to a
        // known coverpoint (a cross with an unresolvable item was dropped earlier).
        std::vector<AstVar*> cpVars;
        std::map<std::string, uint32_t> dimensions;
        ପାଇଁ (Ast_ଗଣ୍ଠି* itemp = crossp->itemsp(); itemp;) {
            Ast_ଗଣ୍ଠି* ଅଚଳ nextp = itemp->nextp();
            AstCoverpointRef* ଅଚଳ refp = VN_AS(itemp, CoverpointRef);
            ଅଚଳ auto it = m_cpVarMap.find(refp->name());
            UASSERT_OBJ(it != m_cpVarMap.end(), crossp, "Cross references an unknown coverpoint");
            dimensions.emplace(refp->name(), static_cast<uint32_t>(cpVars.size()));
            cpVars.ଠେଲ_back(it->second);
            VL_DO_DANGLING(ଠେଲ_Deletep(refp->unlinkFrBack()), refp);
            itemp = nextp;
        }
        ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା dims = static_cast<ପୂର୍ଣ୍ଣସଂଖ୍ୟା>(cpVars.size());
        CrossLayout layout;
        ଯଦି (dynamic) {
            // The runtime sizes the layout from live bins; only check the declared bound here.
            std::vector<uint32_t> strides;
            uint32_t tuples = 0;
            layout.valid = crossShape(crossp, cpVars, strides, tuples);
        } ଅନ୍ୟଥା {
            layout = resolveCrossLayout(crossp, cpVars, dimensions);
        }
        ଯଦି (!layout.valid) ଫେରନ୍ତୁ;

        AstVar* ଅଚଳ cxVarp
            = ନୂତନ AstVar{fl, VVarType::MEMBER, "__Vcx_" + crossp->name(),
                         crossDType(fl, static_cast<uint32_t>(dims), layout, dynamic)};
        m_covergroupp->addMembersp(cxVarp);
        m_constructorp->addStmtsp(makeItemCreate(fl, cxVarp,
                                                 dynamic ? VCMethod::COVERGROUP_ADD_CROSS_DYN
                                                         : VCMethod::COVERGROUP_ADD_CROSS));
        generateItemWeight(fl, cxVarp, crossp->optionsp());
        ପୂର୍ଣ୍ଣସଂଖ୍ୟା at_Least_ମୂଲ୍ୟ;
        ପୂର୍ଣ୍ଣସଂଖ୍ୟା autoBinMax;  // Not a cross option
        extractItemOptions(crossp->optionsp(), at_Least_ମୂଲ୍ୟ, autoBinMax);

        // Constructor: init (after the coverpoints, which generate earlier) then registration.
        // Obfuscate the hierarchy/filename/page under --protect-ids as for coverpoints above.
        ଅଚଳ ତାର୍କିକ prot = v3Global.opt.protectIds();
        ଅଚଳ std::string hier
            = VIdProtect::protect_ଶବ୍ଦଗୁଡ଼ିକ_If(m_covergroupName + "." + crossp->name(), prot);
        m_constructorp->addStmtsp(makeCrossCpsCall(
            fl, cpVars,
            itemCall(fl, cxVarp, VCMethod::COVERGROUP_INIT,
                     {ctext(fl, quoted(hier)), cnum(fl, static_cast<uint32_t>(at_Least_ମୂଲ୍ୟ)),
                      cnum(fl, static_cast<uint32_t>(dims)), ctext(fl, "__Vcx_cps"),
                      ctext(fl, quoted(VIdProtect::protectIf(fl->filename(), prot))),
                      cnum(fl, static_cast<uint32_t>(fl->lineno())),
                      cnum(fl, static_cast<uint32_t>(fl->firstColumn()))})));
        ଅଚଳ std::vector<AstCoverCrossBin*> bins
            = dynamic ? generateRuntimeCrossBins(crossp, cxVarp, cpVars, dimensions)
                      : generateCrossBins(crossp, cxVarp, layout);
        ଯଦି (v3Global.opt.coverage()) {
            ଅଚଳ std::string page
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
        ଅଚଳ ତାର୍କିକ hasIffs
            = std::any_of(bins.begin(), bins.end(),
                          [](ଅଚଳ AstCoverCrossBin* binp) { ଫେରନ୍ତୁ binp->iffp() != nullptr; });
        Ast_ଗଣ୍ଠି_Stmt* ଅଚଳ samplep
            = !hasIffs ? static_cast<Ast_ଗଣ୍ଠି_Stmt*>(
                             itemCall(fl, cxVarp, VCMethod::COVERGROUP_SAMPLE)->makeStmt())
                       : static_cast<Ast_ଗଣ୍ଠି_Stmt*>(makeCrossIffsCall(
                             fl, bins,
                             itemCall(fl, cxVarp, VCMethod::COVERGROUP_SAMPLE_IFFS,
                                      {ctext(fl, "__Vcx_iffs")})));
        ଯଦି (Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ iffp = crossp->iffp()) {
            m_sampleFuncp->addStmtsp(ନୂତନ AstIf{fl, iffp->clone_ବୃକ୍ଷ(ମିଥ୍ୟା), samplep});
        } ଅନ୍ୟଥା {
            m_sampleFuncp->addStmtsp(samplep);
        }
    }

    ଶୂନ୍ୟ generateCrossCode(AstCoverCross* crossp) {
        UINFO(4, "  Generating code for cross: " << crossp->name());

        // Non-standard hierarchical/dotted cross item (e.g. 'cross a.b'): an implicit coverpoint
        // over the referenced expression (carried in refp->exprp()).  The grammar already warned
        // NONSTD; implicit coverpoints are not yet implemented, so generate no sampling code for
        // this cross.  When support is added the implicit coverpoint should be synthesized
        // upstream (V3LinkParse) as a real AstCoverpoint so it flows through the normal coverpoint
        // path - by here coverpoint lowering has already run.
        ପାଇଁ (Ast_ଗଣ୍ଠି* itemp = crossp->itemsp(); itemp; itemp = itemp->nextp()) {
            ଅଚଳ AstCoverpointRef* ଅଚଳ refp = VN_AS(itemp, CoverpointRef);
            ଯଦି (refp->exprp()) {
                refp->v3warn(COVERIGN,
                             "Unsupported: cross of hierarchical reference (implicit coverpoint)");
                ଫେରନ୍ତୁ;
            }
        }

        // A cross naming a bare variable (implicit coverpoint, which Verilator does not
        // synthesize) is dropped entirely with a COVERIGN warning -- it produces no coverage
        // either way -- but only this cross is dropped; its sibling crosses are still generated
        // and the real coverpoints it referenced remain as independent coverpoints.
        ଯଦି (m_droppedCrosses.ଗଣନା_2(crossp)) {
            ପାଇଁ (Ast_ଗଣ୍ଠି* itemp = crossp->itemsp(); itemp; itemp = itemp->nextp()) {
                ଅଚଳ AstCoverpointRef* ଅଚଳ refp = VN_AS(itemp, CoverpointRef);
                ଯଦି (m_coverpointMap.find(refp->name()) == m_coverpointMap.end()) {
                    refp->v3warn(COVERIGN, "Unsupported: cross of "
                                               << refp->prettyNameQ()
                                               << " which is not a coverpoint (implicit "
                                                  "coverpoint)");
                    ଭାଙ୍ଗ;
                }
            }
            ଫେରନ୍ତୁ;
        }

        // Every cross that isn't dropped routes through a VlCoverCross member.
        generateCross(crossp);
    }

    Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* buildBinCondition(AstCoverBin* binp, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* exprp) {
        // Get the range list from the bin
        Ast_ଗଣ୍ଠି* ଅଚଳ rangep = binp->rangesp();
        ଯଦି (!rangep) ଫେରନ୍ତୁ nullptr;

        // Integral values resolve to the coverpoint's type, as its runtime metadata does
        ଅଚଳ ତାର୍କିକ integral = exprp->dtypep()->skipRefp()->isIntegralOrPacked();
        // No value form of a wildcard bin is allowed on a real coverpoint (IEEE 1800-2023 19.5.4)
        ଯଦି (binp->isWildcard() && !integral) ଫେରନ୍ତୁ wildcardTypeError(binp, exprp);

        // Build condition by OR-ing all ranges together
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* fullCondp = nullptr;

        ପାଇଁ (Ast_ଗଣ୍ଠି* currRangep = rangep; currRangep; currRangep = currRangep->nextp()) {
            Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* rangeCondp = nullptr;
            currRangep = V3Const::constifyEdit(currRangep);

            ଯଦି (AstInsideRange* irp = VN_CAST(currRangep, InsideRange)) {
                Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ minExprp = irp->lhsp();
                Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ maxExprp = irp->rhsp();
                AstConst* ଅଚଳ minConstp = VN_CAST(minExprp, Const);
                AstConst* ଅଚଳ maxConstp = VN_CAST(maxExprp, Const);
                ଅଚଳ ତାର୍କିକ loUnbounded = VN_IS(minExprp, Unbounded);
                ଅଚଳ ତାର୍କିକ hiUnbounded = VN_IS(maxExprp, Unbounded);
                ଯଦି (loUnbounded || hiUnbounded) {
                    // Open-ended range: '$' is the coverpoint domain min/max, so the
                    // range reduces to a single inequality (e.g. {[10:$]} -> expr >= 10).
                    AstConst* ଅଚଳ boundp = hiUnbounded ? minConstp : maxConstp;
                    ଯଦି (loUnbounded && hiUnbounded) {
                        rangeCondp = ନୂତନ AstConst{irp->fileline(), AstConst::BitTrue{}};
                    } ଅନ୍ୟଥା ଯଦି (!boundp) {
                        irp->v3error("Non-constant expression in bin range; "
                                     "range bounds must be constants (IEEE 1800-2023 19.5)");
                        ଯଦି (fullCondp) VL_DO_DANGLING(ଠେଲ_Deletep(fullCondp), fullCondp);
                        ଫେରନ୍ତୁ nullptr;
                    } ଅନ୍ୟଥା ଯଦି (boundp->ଅଙ୍କ().isFourState()) {
                        irp->v3error("Four-state (x/z) value in bin range bound; "
                                     "range bounds must be two-state constants");
                        ଯଦି (fullCondp) VL_DO_DANGLING(ଠେଲ_Deletep(fullCondp), fullCondp);
                        ଫେରନ୍ତୁ nullptr;
                    } ଅନ୍ୟଥା ଯଦି (integral) {
                        rangeCondp = build_ମୂଲ୍ୟ_Condition(binp, exprp, irp);
                    } ଅନ୍ୟଥା {
                        rangeCondp = makeOpenRangeCondition(irp->fileline(), exprp, boundp,
                                                            /*isLowerBound=*/hiUnbounded);
                    }
                } ଅନ୍ୟଥା ଯଦି (!minConstp || !maxConstp) {
                    irp->v3error("Non-constant expression in bin range; "
                                 "range bounds must be constants (IEEE 1800-2023 19.5)");
                    ଯଦି (fullCondp) VL_DO_DANGLING(ଠେଲ_Deletep(fullCondp), fullCondp);
                    ଫେରନ୍ତୁ nullptr;
                } ଅନ୍ୟଥା ଯଦି (minConstp->ଅଙ୍କ().isFourState() || maxConstp->ଅଙ୍କ().isFourState()) {
                    irp->v3error("Four-state (x/z) value in bin range bound; "
                                 "range bounds must be two-state constants");
                    ଯଦି (fullCondp) VL_DO_DANGLING(ଠେଲ_Deletep(fullCondp), fullCondp);
                    ଫେରନ୍ତୁ nullptr;
                } ଅନ୍ୟଥା ଯଦି (integral) {
                    rangeCondp = build_ମୂଲ୍ୟ_Condition(binp, exprp, irp);
                } ଅନ୍ୟଥା {
                    rangeCondp = makeRangeCondition(irp->fileline(), exprp, minExprp, maxExprp);
                }
            } ଅନ୍ୟଥା ଯଦି (AstConst* constp = VN_CAST(currRangep, Const)) {
                rangeCondp = build_ମୂଲ୍ୟ_Condition(binp, exprp, constp);
            } ଅନ୍ୟଥା {
                currRangep->v3error("Non-constant expression in bin range; values must be "
                                    "constants (IEEE 1800-2023 19.5)");
                ଯଦି (fullCondp) VL_DO_DANGLING(ଠେଲ_Deletep(fullCondp), fullCondp);
                ଫେରନ୍ତୁ nullptr;
            }

            UASSERT_OBJ(rangeCondp, binp, "rangeCondp is null after building range condition");
            fullCondp
                = fullCondp ? ନୂତନ AstOr{binp->fileline(), fullCondp, rangeCondp} : rangeCondp;
        }

        ଫେରନ୍ତୁ fullCondp;
    }

    // Wildcard bits have no meaning for a non-integral coverpoint.
    ସ୍ଥିର Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* wildcardTypeError(AstCoverBin* binp, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* exprp) {
        ଅଚଳ Ast_ଗଣ୍ଠି_DType* ଅଚଳ dtypep = exprp->dtypep()->skipRefp();
        exprp->v3error("Cannot use a wildcard bin on a coverpoint of type "
                       << dtypep->prettyDTypeNameQ() << " (IEEE 1800-2023 19.5.4).\n"
                       << exprp->warnContextPrimary() << '\n'
                       << binp->warnOther() << "... Location of wildcard bin\n"
                       << binp->warnContextSecondary());
        ଫେରନ୍ତୁ ନୂତନ AstConst{binp->fileline(), AstConst::BitFalse{}};
    }

    // Match one bin value, range, or wildcard pattern.  Integral coverpoints first resolve the
    // value to their type (IEEE 1800-2023 19.5.7).  Non-owning: clones what it uses.
    Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* build_ମୂଲ୍ୟ_Condition(AstCoverBin* binp, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* exprp, Ast_ଗଣ୍ଠି* valuep) {
        File_ଧାଡ଼ି* ଅଚଳ fl = valuep->fileline();
        ଯଦି (!exprp->dtypep()->skipRefp()->isIntegralOrPacked()) {
            ଫେରନ୍ତୁ ନୂତନ AstEq{fl, exprp->clone_ବୃକ୍ଷ(ମିଥ୍ୟା),
                             VN_AS(valuep, ଗଣ୍ଠି_ପଦସମଷ୍ଟି)->clone_ବୃକ୍ଷ(ମିଥ୍ୟା)};
        }
        Cross_ମୂଲ୍ୟ_Range range{valuep, resolveWidth(valuep, exprp)};
        ଯଦି (!resolve_ମୂଲ୍ୟ(valuep, exprp, ସତ୍ୟ, binp->isWildcard(), range)) {
            valuep->v3warn(E_UNSUPPORTED,
                           "Unsupported: non-integral value in a coverage bin of an "
                           "integral coverpoint.");
            ଫେରନ୍ତୁ ନୂତନ AstConst{fl, AstConst::BitFalse{}};
        }
        ଯଦି (crossRangeEmpty(range)) ଫେରନ୍ତୁ ନୂତନ AstConst{fl, AstConst::BitFalse{}};
        AstConst* ଅଚଳ lop = new_ମୂଲ୍ୟ_Const(fl, range.lo, exprp);
        AstConst* ଅଚଳ hip = new_ମୂଲ୍ୟ_Const(fl, range.hi, exprp);
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* condp = nullptr;
        ଯଦି (lop->ଅଙ୍କ().isCaseEq(hip->ଅଙ୍କ())) {
            condp = ନୂତନ AstEq{fl, exprp->clone_ବୃକ୍ଷ(ମିଥ୍ୟା), lop};
        } ଅନ୍ୟଥା {
            condp = makeRangeCondition(fl, exprp, lop, hip);
            VL_DO_DANGLING(ଠେଲ_Deletep(lop), lop);
        }
        VL_DO_DANGLING(ଠେଲ_Deletep(hip), hip);
        ଯଦି (!range.wildcard) ଫେରନ୍ତୁ condp;
        // Match the pattern's value bits within the source-value bounds.
        V3_ସଂଖ୍ୟା mask{valuep, exprp->width()};
        V3_ସଂଖ୍ୟା ମୂଲ୍ୟ{valuep, exprp->width()};
        mask.opBitsNonXZ(range.pattern);
        ମୂଲ୍ୟ.opBitsOne(range.pattern);
        AstConst* ଅଚଳ maskConstp = new_ମୂଲ୍ୟ_Const(fl, mask, exprp);
        AstConst* ଅଚଳ ମୂଲ୍ୟ_Constp = new_ମୂଲ୍ୟ_Const(fl, ମୂଲ୍ୟ, exprp);
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ ପଦସମଷ୍ଟି_Masked = ନୂତନ AstAnd{fl, exprp->clone_ବୃକ୍ଷ(ମିଥ୍ୟା), maskConstp};
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ ମୂଲ୍ୟ_Masked = ନୂତନ AstAnd{fl, ମୂଲ୍ୟ_Constp, maskConstp->clone_ବୃକ୍ଷ(ମିଥ୍ୟା)};
        ଫେରନ୍ତୁ ନୂତନ AstLogAnd{fl, condp, ନୂତନ AstEq{fl, ପଦସମଷ୍ଟି_Masked, ମୂଲ୍ୟ_Masked}};
    }

    ଶୂନ୍ୟ generateCoverageComputationCode() {
        UINFO(4, "  Generating coverage computation code");

        // Invalidate cache: addMembersp() calls in generateCoverpointCode/generateCrossCode
        // have added new members since the last scan, so clear before re-querying.
        m_memberMap.clear();

        // get_inst_coverage(): the average of the coverpoints and crosses, weighted by their
        // option.weight (IEEE 1800-2023 19.11).  The instance node holds their runtimes.  With
        // the instances merged, get_coverage() instead, unless option.get_inst_coverage
        // (Table 19-1).
        AstFunc* ଅଚଳ getInstCoveragep
            = VN_AS(m_memberMap.findMember(m_covergroupp, "get_inst_coverage"), Func);
        File_ଧାଡ଼ି* ଅଚଳ instFl = getInstCoveragep->fileline();
        AstCMethodHard* ଅଚଳ instCallp = instanceCall(instFl, VCMethod::COVERGROUP_COVERAGE);
        instCallp->dtypeSetDouble();
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* instValuep = instCallp;
        ଯଦି (m_cgMayMerge) {
            Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ mergedp = ନୂତନ AstLogAnd{
                instFl, newOptionSel(instFl, optionVar(ସତ୍ୟ), "merge_instances", VAccess::READ),
                ନୂତନ AstLogNot{instFl, newOptionSel(instFl, optionVar(ମିଥ୍ୟା), "get_inst_coverage",
                                                   VAccess::READ)}};
            instValuep = ନୂତନ AstCond{instFl, mergedp, typeCoverageCall(instFl), instCallp};
        }
        getInstCoveragep->addStmtsp(ନୂତନ AstAssign{
            instFl, ନୂତନ AstVarRef{instFl, VN_AS(getInstCoveragep->fvarp(), Var), VAccess::WRITE},
            instValuep});

        // get_coverage(): the average of the covergroup's instances, weighted by their
        // option.weight, or with type_option.merge_instances, the coverage of the union of their
        // bins (IEEE 1800-2023 19.11.3).  Static, so the registry finds the instances.
        AstFunc* ଅଚଳ getCoveragep
            = VN_AS(m_memberMap.findMember(m_covergroupp, "get_coverage"), Func);
        File_ଧାଡ଼ି* ଅଚଳ typeFl = getCoveragep->fileline();
        getCoveragep->addStmtsp(ନୂତନ AstAssign{
            typeFl, ନୂତନ AstVarRef{typeFl, VN_AS(getCoveragep->fvarp(), Var), VAccess::WRITE},
            typeCoverageCall(typeFl)});
    }

    // The registry call computing the covergroup's type coverage, per its type options
    AstCMethodHard* typeCoverageCall(File_ଧାଡ଼ି* fl) {
        AstCExpr* ଅଚଳ registryp = ctext(fl, "vlSymsp->_vm_contextp__->covergroupRegistryp()");
        registryp->dtypeSetVoid();  // Opaque receiver; only ever the 'fromp' of the call below
        AstCMethodHard* ଅଚଳ callp
            = ନୂତନ AstCMethodHard{fl, registryp, VCMethod::COVERGROUP_TYPE_COVERAGE};
        callp->addPinsp(ctext(fl, quoted(covergroupProtectedName())));
        callp->addPinsp(newOptionSel(fl, optionVar(ସତ୍ୟ), "weight", VAccess::READ));
        callp->addPinsp(newOptionSel(fl, optionVar(ସତ୍ୟ), "merge_instances", VAccess::READ));
        callp->addPinsp(file_ଧାଡ଼ି_Debug(m_covergroupp->fileline()));
        callp->usePtr(ସତ୍ୟ);
        callp->dtypeSetDouble();
        ଫେରନ୍ତୁ callp;
    }

    // VISITORS
    ସ୍ଥିର ତାର୍କିକ isEnclosingInstanceVar(ଅଚଳ AstVar* varp) {
        ଫେରନ୍ତୁ varp->isClassMember() && !varp->lifetime().isStatic() && !varp->isParam();
    }

    ଶୂନ୍ୟ rewriteThisRef(AstThisRef* refp, AstVar* handleVarp) {
        ଅଚଳ AstClassRefDType* ଅଚଳ refDTypep
            = VN_CAST(refp->dtypep()->skipRefp(), ClassRefDType);
        UASSERT_OBJ(refDTypep && refDTypep->classp() == m_covergroupp, refp,
                    "Unexpected this reference in embedded covergroup");
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ newp = ନୂତନ AstVarRef{refp->fileline(), handleVarp, VAccess::READ};
        refp->replaceWith(newp);
        VL_DO_DANGLING(ଠେଲ_Deletep(refp), refp);
    }

    ଶୂନ୍ୟ rewriteVarRef(AstVarRef* refp, AstVar* handleVarp) {
        File_ଧାଡ଼ି* ଅଚଳ fl = refp->fileline();
        // As V3LinkLValue does, the handle of a written member select is marked written
        AstMemberSel* ଅଚଳ selp
            = ନୂତନ AstMemberSel{fl, ନୂତନ AstVarRef{fl, handleVarp, refp->access()}, refp->varp()};
        selp->access(refp->access());
        refp->replaceWith(selp);
        VL_DO_DANGLING(ଠେଲ_Deletep(refp), refp);
    }

    ଶୂନ୍ୟ rewriteFuncRef(AstFuncRef* refp, AstVar* handleVarp) {
        File_ଧାଡ଼ି* ଅଚଳ fl = refp->fileline();
        AstArg* ଅଚଳ argsp = refp->argsp() ? refp->argsp()->unlinkFrBackWithNext() : nullptr;
        AstMethodCall* ଅଚଳ callp = ନୂତନ AstMethodCall{
            fl, ନୂତନ AstVarRef{fl, handleVarp, VAccess::READ}, refp->name(), argsp};
        callp->taskp(refp->taskp());
        callp->dtypeFrom(refp);
        refp->replaceWith(callp);
        VL_DO_DANGLING(ଠେଲ_Deletep(refp), refp);
    }

    // True if funcp is an instance method of the enclosing class or one of its bases
    ତାର୍କିକ isEnclosingInstanceFunc(ଅଚଳ Ast_ଗଣ୍ଠି_FTask* funcp) ଅଚଳ {
        ଯଦି (!funcp->classMethod() || funcp->isStatic()) ଫେରନ୍ତୁ ମିଥ୍ୟା;
        ଫେରନ୍ତୁ AstClass::isClassExtendedFrom(m_enclosingClassp, VN_AS(funcp->aboveLoopp(), Class));
    }

    ତାର୍କିକ isEmbeddedCovergroupVar(ଅଚଳ AstVar* varp) ଅଚଳ {
        ଯଦି (!varp || !varp->isClassMember() || varp->isDeclTyped()) ଫେରନ୍ତୁ ମିଥ୍ୟା;
        ଅଚଳ AstClassRefDType* ଅଚଳ refp = VN_CAST(varp->dtypep()->skipRefp(), ClassRefDType);
        ଫେରନ୍ତୁ refp && refp->classp() == m_covergroupp;
    }

    AstVar* findEmbeddedCovergroupVar() ଅଚଳ {
        ଯଦି (!m_enclosingClassp) ଫେରନ୍ତୁ nullptr;
        ପାଇଁ (Ast_ଗଣ୍ଠି* itemp = m_enclosingClassp->membersp(); itemp; itemp = itemp->nextp()) {
            ଯଦି (AstVar* ଅଚଳ varp = VN_CAST(itemp, Var)) {
                ଯଦି (isEmbeddedCovergroupVar(varp)) ଫେରନ୍ତୁ varp;
            }
        }
        // V3LinkParse always creates an implicit variable for an embedded covergroup.
        ଫେରନ୍ତୁ nullptr;  // LCOV_EXCL_LINE
    }

    std::vector<Ast_ଗଣ୍ଠି_Assign*> findCovergroupConstructions() {
        std::vector<Ast_ଗଣ୍ଠି_Assign*> foundps;
        ଯଦି (!m_embeddedVarp) ଫେରନ୍ତୁ foundps;
        AstFunc* ଅଚଳ enclosingNewp
            = VN_CAST(m_memberMap.findMember(m_enclosingClassp, "new"), Func);
        ଯଦି (!enclosingNewp) ଫେରନ୍ତୁ foundps;
        enclosingNewp->foreach([&](Ast_ଗଣ୍ଠି_Assign* asgnp) {
            ଅଚଳ AstNew* ଅଚଳ newp = VN_CAST(asgnp->rhsp(), New);
            ଅଚଳ AstVarRef* ଅଚଳ lhsRefp = VN_CAST(asgnp->lhsp(), VarRef);
            ଯଦି (!newp || !lhsRefp || lhsRefp->varp() != m_embeddedVarp) ଫେରନ୍ତୁ;
            ଅଚଳ AstClassRefDType* ଅଚଳ refp = VN_CAST(newp->dtypep(), ClassRefDType);
            ଯଦି (refp && refp->classp() == m_covergroupp) foundps.ଠେଲ_back(asgnp);
        });
        ଫେରନ୍ତୁ foundps;
    }

    std::set<ଅଚଳ AstVar*> enclosingInstanceVars() ଅଚଳ {
        std::set<ଅଚଳ AstVar*> vars;
        ଯଦି (m_enclosingClassp) {
            m_enclosingClassp->foreachMember([&](AstClass* ଅଚଳ, AstVar* ଅଚଳ varp) {
                ଯଦି (isEnclosingInstanceVar(varp)) vars.ଭର୍ତ୍ତି(varp);
            });
        }
        ଫେରନ୍ତୁ vars;
    }

    ତାର୍କିକ hasEnclosingEventRef(AstCovergroup* cgp) ଅଚଳ {
        ଯଦି (!m_embeddedVarp || !cgp->eventp()) ଫେରନ୍ତୁ ମିଥ୍ୟା;
        ଅଚଳ std::set<ଅଚଳ AstVar*> enclosingVars = enclosingInstanceVars();
        ତାର୍କିକ found = ମିଥ୍ୟା;
        cgp->eventp()->foreach([&](AstVarRef* refp) {
            ଯଦି (enclosingVars.ଗଣନା_2(refp->varp())) found = ସତ୍ୟ;
        });
        ଫେରନ୍ତୁ found;
    }

    ସ୍ଥିର ତାର୍କିକ parse_Embedded_Event_ପଦସମଷ୍ଟି(Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* exprp, AstVar*& baseVarp,
                                       AstVar*& memberVarp) {
        ଯଦି (AstVarRef* ଅଚଳ refp = VN_CAST(exprp, VarRef)) {
            baseVarp = refp->varp();
            memberVarp = nullptr;
            ଫେରନ୍ତୁ ସତ୍ୟ;
        }
        AstMemberSel* ଅଚଳ selp = VN_CAST(exprp, MemberSel);
        ଯଦି (!selp) ଫେରନ୍ତୁ ମିଥ୍ୟା;
        AstVarRef* ଅଚଳ baseRefp = VN_CAST(selp->fromp(), VarRef);
        ଯଦି (!baseRefp) ଫେରନ୍ତୁ ମିଥ୍ୟା;
        baseVarp = baseRefp->varp();
        memberVarp = selp->varp();
        ଫେରନ୍ତୁ ସତ୍ୟ;
    }

    ତାର୍କିକ isEventLvalue(Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* exprp, ଅଚଳ EmbeddedEventTrigger& trigger) ଅଚଳ {
        ଯଦି (AstSel* ଅଚଳ selp = VN_CAST(exprp, Sel)) exprp = selp->fromp();
        AstVar* baseVarp = nullptr;
        AstVar* memberVarp = nullptr;
        ଯଦି (!parse_Embedded_Event_ପଦସମଷ୍ଟି(exprp, baseVarp, memberVarp)) ଫେରନ୍ତୁ ମିଥ୍ୟା;
        ଫେରନ୍ତୁ baseVarp == trigger.baseVarp && memberVarp == trigger.memberVarp;
    }

    Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* newEventRead(File_ଧାଡ଼ି* fl, ଅଚଳ EmbeddedEventTrigger& trigger) ଅଚଳ {
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ basep = ନୂତନ AstVarRef{fl, trigger.baseVarp, VAccess::READ};
        ଯଦି (!trigger.memberVarp) ଫେରନ୍ତୁ basep;
        AstMemberSel* ଅଚଳ selp = ନୂତନ AstMemberSel{fl, basep, trigger.memberVarp};
        selp->access(VAccess::READ);
        ଫେରନ୍ତୁ selp;
    }

    string eventPrevName(ଅଚଳ EmbeddedEventTrigger& trigger, size_t triggerIndex) ଅଚଳ {
        string name = "__Vcg_prev_" + m_embeddedVarp->name() + "_" + std::to_string(triggerIndex)
                      + "_" + trigger.baseVarp->name();
        ଯଦି (trigger.memberVarp) name += "_" + trigger.memberVarp->name();
        ଫେରନ୍ତୁ name;
    }

    Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* newEmbeddedVarNonNull(File_ଧାଡ଼ି* fl) ଅଚଳ {
        ଫେରନ୍ତୁ ନୂତନ AstNeq{fl, ନୂତନ AstVarRef{fl, m_embeddedVarp, VAccess::READ},
                          ନୂତନ AstConst{fl, AstConst::Null{}}};
    }

    Ast_ଗଣ୍ଠି_Stmt* newSampleStmt(File_ଧାଡ଼ି* fl) ଅଚଳ {
        AstMethodCall* ଅଚଳ callp = ନୂତନ AstMethodCall{
            fl, ନୂତନ AstVarRef{fl, m_embeddedVarp, VAccess::READ}, "sample", nullptr};
        callp->taskp(m_sampleFuncp);
        callp->dtypeSetVoid();
        ଫେରନ୍ତୁ callp->makeStmt();
    }

    ଶୂନ୍ୟ installEmbeddedEventFork(Ast_Sen_ବୃକ୍ଷ* eventp,
                                  ଅଚଳ std::vector<Ast_ଗଣ୍ଠି_Assign*>& constructps) {
        // IEEE 1800-2023 19.3 samples coverpoints whenever their clocking event occurs. A
        // per-instance event cannot use V3Active's static sensitivity path, so spawn
        // 'fork forever begin @(event); cg.sample(); end join_none' after each construction.
        ପାଇଁ (Ast_ଗଣ୍ଠି_Assign* ଅଚଳ constructp : constructps) {
            File_ଧାଡ଼ି* ଅଚଳ fl = constructp->fileline();
            AstLoop* ଅଚଳ loopp = ନୂତନ AstLoop{fl};
            loopp->addStmtsp(ନୂତନ AstEventControl{fl, eventp->clone_ବୃକ୍ଷ(ମିଥ୍ୟା), nullptr});
            loopp->addStmtsp(ନୂତନ AstIf{fl, newEmbeddedVarNonNull(fl), newSampleStmt(fl)});
            AstFork* ଅଚଳ forkp = ନୂତନ AstFork{fl, VJoinType::JOIN_NONE};
            forkp->immediateStart(ସତ୍ୟ);
            forkp->addForksp(ନୂତନ AstBegin{fl, "", loopp, ସତ୍ୟ});
            constructp->addNextHere(forkp);
        }
        VL_DO_DANGLING(ଠେଲ_Deletep(eventp), eventp);
    }

    Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* newEventReadyCondition(File_ଧାଡ଼ି* fl, ଅଚଳ EmbeddedEventTrigger& trigger) ଅଚଳ {
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ curp = newEventRead(fl, trigger);
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ prevp = ନୂତନ AstVarRef{fl, trigger.prevVarp, VAccess::READ};
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* edgep = nullptr;
        // IEEE 1800-2023 9.4.2 detects edge-qualified events only on the expression's LSB,
        // while an implicit change event observes the complete expression.
        ଯଦି (trigger.edgeType == VEdgeType::ET_POSEDGE) {
            edgep = ନୂତନ AstSel{fl, ନୂତନ AstAnd{fl, curp, ନୂତନ AstNot{fl, prevp}}, 0, 1};
        } ଅନ୍ୟଥା ଯଦି (trigger.edgeType == VEdgeType::ET_NEGEDGE) {
            edgep = ନୂତନ AstSel{fl, ନୂତନ AstAnd{fl, ନୂତନ AstNot{fl, curp}, prevp}, 0, 1};
        } ଅନ୍ୟଥା ଯଦି (trigger.edgeType == VEdgeType::ET_BOTHEDGE) {
            edgep = ନୂତନ AstSel{fl, ନୂତନ AstXor{fl, curp, prevp}, 0, 1};
        } ଅନ୍ୟଥା {
            edgep = ନୂତନ AstNeq{fl, curp, prevp};
        }
        ଫେରନ୍ତୁ ନୂତନ AstLogAnd{fl, newEmbeddedVarNonNull(fl), edgep};
    }

    std::vector<EmbeddedEventTrigger> collectEmbeddedEventTriggers(AstCovergroup* cgp) {
        std::vector<EmbeddedEventTrigger> triggers;
        ଅଚଳ std::set<ଅଚଳ AstVar*> enclosingVars = enclosingInstanceVars();
        ପାଇଁ (Ast_ଗଣ୍ଠି* senp = cgp->eventp()->sensesp(); senp; senp = senp->nextp()) {
            AstSenItem* ଅଚଳ itemp = VN_AS(senp, SenItem);
            AstVar* baseVarp = nullptr;
            AstVar* memberVarp = nullptr;
            ଯଦି (!parse_Embedded_Event_ପଦସମଷ୍ଟି(itemp->sensp(), baseVarp, memberVarp)
                || !enclosingVars.ଗଣନା_2(baseVarp)) {
                ଫେରନ୍ତୁ {};
            }
            triggers.emplace_back(itemp->fileline(), baseVarp, memberVarp, itemp->edgeType());
        }
        ଫେରନ୍ତୁ triggers;
    }

    ଶୂନ୍ୟ installEmbeddedEventTriggers(std::vector<EmbeddedEventTrigger>& triggers,
                                      ଅଚଳ std::vector<Ast_ଗଣ୍ଠି_Assign*>& constructps) {
        // Without --timing, approximate a per-instance event by sampling after assignments
        // within the enclosing class. External writes and exact scheduling cannot be observed.
        ଯଦି (constructps.empty()) ଫେରନ୍ତୁ;
        ପାଇଁ (size_t triggerIndex = 0; triggerIndex < triggers.size(); ++triggerIndex) {
            EmbeddedEventTrigger& trigger = triggers[triggerIndex];
            std::vector<Ast_ଗଣ୍ଠି_Assign*> assignps;
            m_enclosingClassp->foreach([&](Ast_ଗଣ୍ଠି_Assign* asgnp) {
                ଯଦି (isEventLvalue(asgnp->lhsp(), trigger)) assignps.ଠେଲ_back(asgnp);
            });
            ଯଦି (assignps.empty()) {
                trigger.eventFl->v3warn(
                    COVERIGN, "Unsupported: 'covergroup' clocking event signal has no assignment "
                              "within the enclosing class; no coverage sampled. Use --timing for "
                              "full support.");
                ଜାରି;
            }
            Ast_ଗଣ୍ଠି_DType* ଅଚଳ dtypep
                = trigger.memberVarp ? trigger.memberVarp->dtypep() : trigger.baseVarp->dtypep();
            AstVar* ଅଚଳ prevVarp = ନୂତନ AstVar{trigger.eventFl, VVarType::MEMBER,
                                                eventPrevName(trigger, triggerIndex), dtypep};
            m_enclosingClassp->addMembersp(prevVarp);
            trigger.prevVarp = prevVarp;
            ପାଇଁ (Ast_ଗଣ୍ଠି_Assign* ଅଚଳ asgnp : assignps) {
                File_ଧାଡ଼ି* ଅଚଳ fl = asgnp->fileline();
                AstIf* ଅଚଳ ifp
                    = ନୂତନ AstIf{fl, newEventReadyCondition(fl, trigger), newSampleStmt(fl)};
                ifp->addNextHere(ନୂତନ AstAssign{fl,
                                               ନୂତନ AstVarRef{fl, trigger.prevVarp, VAccess::WRITE},
                                               newEventRead(fl, trigger)});
                asgnp->addNextHere(ifp);
            }
        }
    }

    ଶୂନ୍ୟ delete_Coverage_ବସ୍ତୁଗୁଡ଼ିକ() {
        ପାଇଁ (AstCoverpoint* ଅଚଳ cpp : m_coverpoints) {
            VL_DO_DANGLING(ଠେଲ_Deletep(cpp->unlinkFrBack()), cpp);
        }
        ପାଇଁ (AstCoverCross* ଅଚଳ crossp : m_coverCrosses) {
            VL_DO_DANGLING(ଠେଲ_Deletep(crossp->unlinkFrBack()), crossp);
        }
        // Options not lowered: the covergroup was not processed
        ପାଇଁ (AstCgOptionAssign* ଅଚଳ optp : m_cgOptions) {
            VL_DO_DANGLING(ଠେଲ_Deletep(optp->unlinkFrBack()), optp);
        }
        m_cgOptions.clear();
    }

    ଶ୍ରେଣୀ FormalRefVisitor final : ସର୍ବସାଧାରଣ VNVisitor {
        ଅଚଳ std::map<ଅଚଳ AstVar*, AstVar*>& m_replacements;

        ଶୂନ୍ୟ visit(AstVarRef* nodep) override {
            ଅଚଳ auto it = m_replacements.find(nodep->varp());
            ଯଦି (it == m_replacements.end()) ଫେରନ୍ତୁ;
            nodep->varp(it->second);
        }
        ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି* nodep) override { iterateChildren(nodep); }

    ସର୍ବସାଧାରଣ:
        explicit FormalRefVisitor(ଅଚଳ std::map<ଅଚଳ AstVar*, AstVar*>& replacements)
            : m_replacements{replacements} {}
        ଶୂନ୍ୟ scan(Ast_ଗଣ୍ଠି* nodep) { iterate(nodep); }
    };

    ଶୂନ୍ୟ validateCovergroupExpressions() {
        std::set<ଅଚଳ AstVar*> sampleMembers;
        std::set<ଅଚଳ AstVar*> constructorRefMembers;
        ପାଇଁ (Ast_ଗଣ୍ଠି* stmtp = m_constructorp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            ଅଚଳ AstVar* ଅଚଳ varp = VN_CAST(stmtp, Var);
            ଯଦି (!varp || !varp->isIO() || (!varp->isRef() && !varp->isConstRef())) ଜାରି;
            ଅଚଳ AstVar* ଅଚଳ memberp
                = VN_CAST(m_memberMap.findMember(m_covergroupp, varp->name()), Var);
            UASSERT_OBJ(memberp && memberp->isClassMember(), varp,
                        "Covergroup constructor argument missing persistent member");
            constructorRefMembers.ଭର୍ତ୍ତି(varp);
            constructorRefMembers.ଭର୍ତ୍ତି(memberp);
        }
        ପାଇଁ (Ast_ଗଣ୍ଠି* stmtp = m_sampleFuncp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            ଅଚଳ AstVar* ଅଚଳ varp = VN_CAST(stmtp, Var);
            ଯଦି (!varp || !varp->isIO()) ଜାରି;
            ଅଚଳ AstVar* ଅଚଳ memberp
                = VN_CAST(m_memberMap.findMember(m_covergroupp, varp->name()), Var);
            UASSERT_OBJ(memberp && memberp->isClassMember(), varp,
                        "Covergroup sample argument missing persistent member");
            sampleMembers.ଭର୍ତ୍ତି(memberp);
        }
        Covergroup_ପଦସମଷ୍ଟି_Valid_Visitor{sampleMembers, constructorRefMembers}.scan(m_constructorp);
    }

    ଶୂନ୍ୟ rebindFormalRefs() {
        std::map<ଅଚଳ AstVar*, AstVar*> replacements;
        ପାଇଁ (Ast_ଗଣ୍ଠି* stmtp = m_constructorp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            ଯଦି (ଅଚଳ AstVar* ଅଚଳ varp = VN_CAST(stmtp, Var)) {
                ଯଦି (!varp->isIO()) ଜାରି;
                AstVar* ଅଚଳ memberp
                    = VN_CAST(m_memberMap.findMember(m_covergroupp, varp->name()), Var);
                UASSERT_OBJ(memberp && memberp->isClassMember(), varp,
                            "Covergroup constructor argument missing persistent member");
                replacements.emplace(varp, memberp);
            }
        }
        size_t expectedBindings = 0;
        ପାଇଁ (ଅଚଳ auto& pair : replacements) {
            ଯଦି (pair.first->isRef() || pair.first->isConstRef()) ++expectedBindings;
        }
        size_t rewrittenBindings = 0;
        ପାଇଁ (Ast_ଗଣ୍ଠି* stmtp = m_constructorp->stmtsp(); stmtp;) {
            Ast_ଗଣ୍ଠି* ଅଚଳ nextp = stmtp->nextp();
            ଯଦି (AstAssign* ଅଚଳ assignp = VN_CAST(stmtp, Assign)) {
                AstVarRef* ଅଚଳ lhsp = VN_CAST(assignp->lhsp(), VarRef);
                AstVarRef* ଅଚଳ rhsp = VN_CAST(assignp->rhsp(), VarRef);
                ଯଦି (lhsp && rhsp
                    && (lhsp->varp()->declDirection() == VDirection::REF
                        || lhsp->varp()->declDirection() == VDirection::CONSTREF)) {
                    ଅଚଳ auto it = replacements.find(rhsp->varp());
                    UASSERT_OBJ(it != replacements.end() && it->second == lhsp->varp(), assignp,
                                "Unexpected covergroup reference binding assignment");
                    AstCExpr* ଅଚଳ bindp = ନୂତନ AstCExpr{assignp->fileline(), ""};
                    bindp->add(lhsp->unlinkFrBack());
                    bindp->add(" = &");
                    bindp->add(rhsp->unlinkFrBack());
                    assignp->replaceWith(bindp->makeStmt());
                    VL_DO_DANGLING(ଠେଲ_Deletep(assignp), assignp);
                    ++rewrittenBindings;
                }
            }
            stmtp = nextp;
        }
        UASSERT_OBJ(rewrittenBindings == expectedBindings, m_constructorp,
                    "Covergroup reference argument missing binding");
        FormalRefVisitor visitor{replacements};
        ପାଇଁ (AstCoverpoint* ଅଚଳ cpp : m_coverpoints) visitor.scan(cpp);
        ପାଇଁ (AstCoverCross* ଅଚଳ crossp : m_coverCrosses) visitor.scan(crossp);
    }

    AstVarRef* installEnclosingBackPointer(ଅଚଳ std::vector<Ast_ଗଣ୍ଠି_Assign*>& constructps) {
        // Simple-case support for embedded covergroups (IEEE 1800-2023 19.4) whose
        // coverpoints reference members of the enclosing class ("Class members can be used
        // in coverpoint expressions").  The covergroup is lowered into a sibling class with
        // no implicit handle to the enclosing object, so such references would emit
        // uncompilable C++.  Add an explicit back-pointer member to the enclosing instance,
        // route member references through it, and pass it into the constructor so
        // coverage initialization can read enclosing members. Returns an invalid
        // reference if an outer class member cannot be reached; otherwise returns an empty result.
        ଯଦି (!m_enclosingClassp) ଫେରନ୍ତୁ nullptr;  // Offending refs require an enclosing class

        AstVarRef* invalidp = nullptr;
        Ast_ଗଣ୍ଠି* offenderp = nullptr;
        std::set<ଅଚଳ AstVar*> ownVars;
        ପାଇଁ (Ast_ଗଣ୍ଠି* itemp = m_covergroupp->membersp(); itemp; itemp = itemp->nextp()) {
            ଯଦି (ଅଚଳ AstVar* ଅଚଳ varp = VN_CAST(itemp, Var)) ownVars.ଭର୍ତ୍ତି(varp);
        }
        ଅଚଳ std::set<ଅଚଳ AstVar*> enclosingVars = enclosingInstanceVars();
        std::vector<AstVarRef*> refsToRewrite;
        std::vector<AstThisRef*> thisRefsToRewrite;
        std::vector<AstFuncRef*> funcRefsToRewrite;
        ଅଚଳ auto scan = [&](Ast_ଗଣ୍ଠି* rootp) {
            rootp->foreach([&](AstVarRef* refp) {
                ଯଦି (invalidp) ଫେରନ୍ତୁ;
                ଅଚଳ AstVar* ଅଚଳ varp = refp->varp();
                ଯଦି (!isEnclosingInstanceVar(varp) || ownVars.ଗଣନା_2(varp)) ଫେରନ୍ତୁ;
                ଯଦି (!enclosingVars.ଗଣନା_2(varp)) {
                    invalidp = refp;
                    ଫେରନ୍ତୁ;
                }
                refsToRewrite.ଠେଲ_back(refp);
                ଯଦି (!offenderp) offenderp = refp;
            });
            ଯଦି (invalidp) ଫେରନ୍ତୁ;
            rootp->foreach([&](AstThisRef* refp) {
                ଅଚଳ AstClassRefDType* ଅଚଳ refDTypep
                    = VN_CAST(refp->dtypep()->skipRefp(), ClassRefDType);
                ଯଦି (refDTypep && refDTypep->classp() == m_covergroupp) {
                    thisRefsToRewrite.ଠେଲ_back(refp);
                    ଯଦି (!offenderp) offenderp = refp;
                }
            });
            rootp->foreach([&](AstFuncRef* refp) {
                ଯଦି (!isEnclosingInstanceFunc(refp->taskp())) ଫେରନ୍ତୁ;
                funcRefsToRewrite.ଠେଲ_back(refp);
                ଯଦି (!offenderp) offenderp = refp;
            });
        };
        ପାଇଁ (AstCoverpoint* ଅଚଳ cpp : m_coverpoints) scan(cpp);
        ପାଇଁ (AstCoverCross* ଅଚଳ crossp : m_coverCrosses) scan(crossp);
        ପାଇଁ (AstCgOptionAssign* ଅଚଳ optp : m_cgOptions) scan(optp);
        ଯଦି (invalidp || !offenderp) ଫେରନ୍ତୁ invalidp;

        UASSERT_OBJ(m_embeddedVarp, m_covergroupp, "Embedded covergroup variable not found");
        // Commit: add the back-pointer member, rewrite the references, initialize the handle.
        File_ଧାଡ଼ି* ଅଚଳ fl = m_covergroupp->fileline();
        AstClassRefDType* ଅଚଳ enclDTypep = ନୂତନ AstClassRefDType{fl, m_enclosingClassp, nullptr};
        enclDTypep->rawPointer(ସତ୍ୟ);
        v3Global.rootp()->typeTablep()->addTypesp(enclDTypep);
        AstVar* ଅଚଳ handleVarp
            = ନୂତନ AstVar{fl, VVarType::MEMBER, "__Vcg_enclosingp", enclDTypep};
        m_covergroupp->addMembersp(handleVarp);
        AstVar* ଅଚଳ argumentp = ନୂତନ AstVar{fl, VVarType::BLOCKTEMP, "__Vcg_parentp", enclDTypep};
        argumentp->direction(VDirection::ନିବେଶ);
        argumentp->declDirection(VDirection::ନିବେଶ);
        argumentp->funcLocal(ସତ୍ୟ);
        argumentp->noReset(ସତ୍ୟ);
        argumentp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
        m_constructorp->addStmtsp(argumentp);
        m_constructorp->prependStmtsp(ନୂତନ AstAssign{fl, memberRef(fl, handleVarp, VAccess::WRITE),
                                                    ନୂତନ AstVarRef{fl, argumentp, VAccess::READ}});

        // Route each enclosing-member reference through the back-pointer: 'm' -> 'h.m'.
        ପାଇଁ (AstVarRef* ଅଚଳ refp : refsToRewrite) { rewriteVarRef(refp, handleVarp); }
        ପାଇଁ (AstThisRef* ଅଚଳ refp : thisRefsToRewrite) { rewriteThisRef(refp, handleVarp); }
        ପାଇଁ (AstFuncRef* ଅଚଳ refp : funcRefsToRewrite) { rewriteFuncRef(refp, handleVarp); }

        // Append a named hidden argument to preserve positional and defaulted user arguments.
        ପାଇଁ (Ast_ଗଣ୍ଠି_Assign* ଅଚଳ constructp : constructps) {
            File_ଧାଡ଼ି* ଅଚଳ cfl = constructp->fileline();
            AstCExpr* ଅଚଳ thisp = ନୂତନ AstCExpr{cfl, "this"};
            thisp->dtypep(enclDTypep);
            VN_AS(constructp->rhsp(), New)->addArgsp(ନୂତନ AstArg{cfl, argumentp->name(), thisp});
        }
        ଫେରନ୍ତୁ nullptr;
    }

    ଶୂନ୍ୟ visit(AstClass* nodep) override {
        UINFO(9, "Visiting class: " << nodep->name() << " isCovergroup=" << nodep->isCovergroup());
        ଯଦି (nodep->isCovergroup()) {
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
            Ast_Sen_ବୃକ୍ଷ* embeddedEventForkp = nullptr;

            // Extract and store the clocking event from AstCovergroup node
            // The parser creates this node to preserve the event information
            ତାର୍କିକ hasUnsupportedEvent = ମିଥ୍ୟା;
            ପାଇଁ (Ast_ଗଣ୍ଠି* itemp = nodep->membersp(); itemp;) {
                Ast_ଗଣ୍ଠି* ଅଚଳ nextp = itemp->nextp();
                ଯଦି (AstCovergroup* ଅଚଳ cgp = VN_CAST(itemp, Covergroup)) {
                    // Store the event in the global map for V3Active to retrieve later
                    // V3LinkParse only creates this sentinel AstCovergroup node when a clocking
                    // event exists, so cgp->eventp() is always non-null here.
                    UASSERT_OBJ(cgp->eventp(), cgp,
                                "Sentinel AstCovergroup in class must have non-null eventp");
                    ଯଦି (hasEnclosingEventRef(cgp)) {
                        UASSERT_OBJ(m_embeddedVarp, cgp,
                                    "Embedded covergroup event has no instance variable");
                        ଯଦି (v3Global.opt.timing().isSetTrue()) {
                            embeddedEventForkp = cgp->eventp()->unlinkFrBack();
                        } ଅନ୍ୟଥା {
                            embeddedEventTriggers = collectEmbeddedEventTriggers(cgp);
                            ଯଦି (embeddedEventTriggers.empty()) {
                                cgp->v3warn(COVERIGN,
                                            "Unsupported: 'covergroup' clocking event on complex "
                                            "member expression; use --timing for full support.");
                                hasUnsupportedEvent = ସତ୍ୟ;
                            }
                        }
                        VL_DO_DANGLING(ଠେଲ_Deletep(cgp->unlinkFrBack()), cgp);
                        itemp = nextp;
                        ଜାରି;
                    }
                    // V3Active handles events that do not depend on an enclosing instance.
                    UINFO(4, "Keeping covergroup event node for V3Active: " << nodep->name());
                    itemp = nextp;
                    ଜାରି;
                }
                itemp = nextp;
            }

            // Find the sample() method and constructor
            m_sampleFuncp = VN_CAST(m_memberMap.findMember(nodep, "sample"), Func);
            // V3LinkParse always synthesizes a sample() method for every covergroup, and the
            // sampling-code generation below dereferences m_sampleFuncp unconditionally.
            UASSERT_OBJ(m_sampleFuncp, nodep, "Covergroup missing synthesized sample() method");
            m_sampleFuncp->isCovergroupSample(ସତ୍ୟ);
            m_constructorp = VN_CAST(m_memberMap.findMember(nodep, "new"), Func);
            UINFO(9, "Found sample() method: " << (m_sampleFuncp ? "yes" : "no"));
            UINFO(9, "Found constructor: " << (m_constructorp ? "yes" : "no"));

            // If covergroup has unsupported clocking event, skip processing it
            // but still clean up coverpoints so they don't reach downstream passes
            ଯଦି (hasUnsupportedEvent) {
                iterateChildren(nodep);
                validateCovergroupExpressions();
                rebindFormalRefs();
                delete_Coverage_ବସ୍ତୁଗୁଡ଼ିକ();
                ଯଦି (embeddedEventForkp) {
                    VL_DO_DANGLING(ଠେଲ_Deletep(embeddedEventForkp), embeddedEventForkp);
                }
                ଫେରନ୍ତୁ;
            }

            iterateChildren(nodep);
            validateCovergroupExpressions();
            rebindFormalRefs();
            ଅଚଳ std::vector<Ast_ଗଣ୍ଠି_Assign*> constructps = findCovergroupConstructions();

            // Embedded covergroups (IEEE 1800-2023 19.4): coverpoints, iff expressions, and
            // crosses may reference members of the enclosing class. The covergroup is lowered
            // into a sibling class with no implicit handle to the enclosing instance. Install
            // an explicit back-pointer and route the references through it.
            ଯଦି (AstVarRef* ଅଚଳ invalidp = installEnclosingBackPointer(constructps)) {
                invalidp->v3error("Non-static member "
                                  << invalidp->varp()->prettyNameQ()
                                  << " of an outer class requires an explicit "
                                     "object handle (IEEE 1800-2023 8.23).");
                delete_Coverage_ବସ୍ତୁଗୁଡ଼ିକ();
                ଯଦି (embeddedEventForkp) {
                    VL_DO_DANGLING(ଠେଲ_Deletep(embeddedEventForkp), embeddedEventForkp);
                }
                ଫେରନ୍ତୁ;
            }
            installEmbeddedEventTriggers(embeddedEventTriggers, constructps);
            ଯଦି (embeddedEventForkp) installEmbeddedEventFork(embeddedEventForkp, constructps);
            processCovergroup();
            // Remove lowered coverpoints/crosses from the class - they have been
            // fully translated into C++ code and must not reach downstream passes
            delete_Coverage_ବସ୍ତୁଗୁଡ଼ିକ();
        } ଅନ୍ୟଥା {
            // Track the lexically enclosing class so a nested covergroup can resolve
            // references to the enclosing object's members (installEnclosingBackPointer).
            VL_RESTORER(m_enclosingClassp);
            m_enclosingClassp = nodep;
            iterateChildren(nodep);
        }
    }

    ଶୂନ୍ୟ visit(AstCoverpoint* nodep) override {
        UINFO(9, "Found coverpoint: " << nodep->name());
        m_coverpoints.ଠେଲ_back(nodep);
        m_coverpointMap.emplace(nodep->name(), nodep);
        iterateChildren(nodep);
    }

    ଶୂନ୍ୟ visit(AstCoverCross* nodep) override {
        UINFO(9, "Found cross: " << nodep->name());
        m_coverCrosses.ଠେଲ_back(nodep);
        iterateChildren(nodep);
    }

    // V3Width leaves only the covergroup-level options lowerCovergroupOptions() takes
    ଶୂନ୍ୟ visit(AstCgOptionAssign* nodep) override { m_cgOptions.ଠେଲ_back(nodep); }

    // A package, interface, or module, so the design unit declaring the classes within
    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି_Module* nodep) override {
        VL_RESTORER(m_unitp);
        m_unitp = nodep;
        iterateChildren(nodep);
    }

    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି* nodep) override { iterateChildren(nodep); }

ସର୍ବସାଧାରଣ:
    // CONSTRUCTORS
    FunctionalCoverageVisitor(AstNetlist* nodep,
                              ଅଚଳ std::set<ଅଚଳ AstVar*>& mergeableTypeOptions)
        : m_mergeableTypeOptions{mergeableTypeOptions} {
        iterate(nodep);
    }
    ~FunctionalCoverageVisitor() override = ପୂର୍ବନିର୍ଦ୍ଧାରିତ;
};

//######################################################################
// Functional coverage class functions

ଶୂନ୍ୟ V3Covergroup::covergroup(AstNetlist* nodep) {
    UINFO(4, __FUNCTION__ << ": ");
    ଅଚଳ CovergroupAssignValidVisitor validVisitor{nodep};
    ଯଦି (!validVisitor.valid()) V3Error::abortIfErrors();
    {  // Destruct before checking
        FunctionalCoverageVisitor{nodep, validVisitor.mergeableTypeOptions()};
    }
    V3Global::dump_Check_Global_ବୃକ୍ଷ("coveragefunc", 0, dump_ବୃକ୍ଷ_Either_Level() >= 3);
}
