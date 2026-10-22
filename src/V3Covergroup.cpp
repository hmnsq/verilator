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

#അന്തര്ഭാവ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#അന്തര്ഭാവ "V3Covergroup.h"

#അന്തര്ഭാവ "V3Const.h"
#അന്തര്ഭാവ "V3Error.h"
#അന്തര്ഭാവ "V3File.h"
#അന്തര്ഭാവ "V3MemberMap.h"
#അന്തര്ഭാവ "V3UniqueNames.h"

#അന്തര്ഭാവ <bitset>
#അന്തര്ഭാവ <cmath>
#അന്തര്ഭാവ <deque>
#അന്തര്ഭാവ <set>
#അന്തര്ഭാവ <tuple>
#അന്തര്ഭാവ <unordered_map>
#അന്തര്ഭാവ <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Embedded covergroup assignment validation; and in the same pass over the netlist, the
// type_option variables through which SystemVerilog may set type_option.merge_instances

വർഗ്ഗം CovergroupAssignValidVisitor final : പൊതു VNVisitorConst {
    VMemberMap m_memberMap;
    std::map<സ്ഥിരം AstVar*, സ്ഥിരം Ast_കണ്ണി_FTask*>
        m_constructors;  // Implicit instance -> constructor
    std::set<സ്ഥിരം AstVar*> m_mergeableTypeOptions;  // See mergeableTypeOptions()
    സ്ഥിരം Ast_കണ്ണി_FTask* m_ftaskp = nullptr;
    തർക്കം m_collecting = സത്യം;
    തർക്കം m_valid = സത്യം;

    ശൂന്യം visit(AstClass* nodep) override {
        VL_RESTORER(m_ftaskp);
        m_ftaskp = nullptr;
        എങ്കിൽ (m_collecting) {
            സ്ഥിരം Ast_കണ്ണി_FTask* സ്ഥിരം constructorp
                = VN_CAST(m_memberMap.findMember(nodep, "new"), കണ്ണി_FTask);
            വേണ്ടി (സ്ഥിരം Ast_കണ്ണി* itemp = nodep->membersp(); itemp; itemp = itemp->nextp()) {
                സ്ഥിരം AstVar* സ്ഥിരം varp = VN_CAST(itemp, Var);
                // Only the implicit instance is restricted, not explicitly typed aliases.
                എങ്കിൽ (!varp || !varp->isClassMember() || varp->isDeclTyped()) തുടരുക;
                സ്ഥിരം AstClassRefDType* സ്ഥിരം refp
                    = VN_CAST(varp->dtypep()->skipRefp(), ClassRefDType);
                എങ്കിൽ (refp && refp->classp()->covergroupEnclosingClassp() == nodep) {
                    m_constructors.emplace(varp, constructorp);
                }
            }
        }
        iterateChildrenConst(nodep);
    }
    ശൂന്യം visit(Ast_കണ്ണി_FTask* nodep) override {
        VL_RESTORER(m_ftaskp);
        m_ftaskp = nodep;
        iterateChildrenConst(nodep);
    }
    ശൂന്യം visit(Ast_കണ്ണി_Assign* nodep) override {
        എങ്കിൽ (!m_collecting) {
            സ്ഥിരം AstVar* varp = nullptr;
            എങ്കിൽ (സ്ഥിരം Ast_കണ്ണി_Var_Ref* സ്ഥിരം refp = VN_CAST(nodep->lhsp(), കണ്ണി_Var_Ref)) {
                varp = refp->varp();
            } അല്ലെങ്കിൽ എങ്കിൽ (സ്ഥിരം AstMemberSel* സ്ഥിരം selp = VN_CAST(nodep->lhsp(), MemberSel)) {
                varp = selp->varp();
            }
            സ്ഥിരം auto it = m_constructors.find(varp);
            എങ്കിൽ (it != m_constructors.end() && (!m_ftaskp || m_ftaskp != it->second)) {
                m_valid = അസത്യം;
                nodep->v3error("Embedded covergroup variable "
                               << varp->prettyNameQ()
                               << " may only be assigned in the enclosing class's 'new' method "
                                  "(IEEE 1800-2023 19.4).");
            }
        }
        iterateChildrenConst(nodep);
    }
    ശൂന്യം visit(Ast_കണ്ണി_Var_Ref* nodep) override {
        // Type options may be set at any time (IEEE 1800-2023 19.7.1), so SystemVerilog may set
        // type_option.merge_instances with any use of a type_option but a read of another
        // member, as not every write is marked yet (std::randomize)
        എങ്കിൽ (m_collecting && nodep->varp()->name() == "type_option") {
            സ്ഥിരം AstStructSel* സ്ഥിരം selp = VN_CAST(nodep->backp(), StructSel);
            എങ്കിൽ (!selp || selp->fromp() != nodep || selp->name() == "merge_instances"
                || !nodep->access().isReadOnly()) {
                m_mergeableTypeOptions.തിരുകുക(nodep->varp());
            }
        }
        iterateChildrenConst(nodep);
    }
    ശൂന്യം visit(Ast_കണ്ണി* nodep) override { iterateChildrenConst(nodep); }

പൊതു:
    explicit CovergroupAssignValidVisitor(AstNetlist* nodep) {
        // Uses can precede their enclosing class in the tree.
        iterateConst(nodep);
        m_collecting = അസത്യം;
        എങ്കിൽ (!m_constructors.empty()) iterateConst(nodep);
    }
    തർക്കം valid() സ്ഥിരം { തിരികെ m_valid; }
    // type_option variables through which SystemVerilog may set type_option.merge_instances, so
    // that covergroups are tested with optionVar(true)
    സ്ഥിരം std::set<സ്ഥിരം AstVar*>& mergeableTypeOptions() സ്ഥിരം { തിരികെ m_mergeableTypeOptions; }
};

//######################################################################
// Covergroup expression validation visitor

വർഗ്ഗം Covergroup_വ്യഞ്ജകം_Valid_Visitor final : പൊതു VNVisitor {
    സ്ഥിരം std::set<സ്ഥിരം AstVar*>& m_sampleMembers;
    സ്ഥിരം std::set<സ്ഥിരം AstVar*>& m_constructorRefMembers;
    തർക്കം m_inCoverageExpression = അസത്യം;
    തർക്കം m_sampleFormalAllowed = അസത്യം;

    ശൂന്യം scanSampleExpression(Ast_കണ്ണി* nodep) {
        എങ്കിൽ (!nodep) തിരികെ;
        VL_RESTORER(m_sampleFormalAllowed);
        m_sampleFormalAllowed = സത്യം;
        iterateAndNextNull(nodep);
    }

    ശൂന്യം scanCoverageExpression(Ast_കണ്ണി* nodep) {
        എങ്കിൽ (!nodep) തിരികെ;
        VL_RESTORER(m_inCoverageExpression);
        m_inCoverageExpression = സത്യം;
        iterateAndNextNull(nodep);
    }

    ശൂന്യം visit(AstCoverpoint* nodep) override {
        scanSampleExpression(nodep->exprp());
        scanSampleExpression(nodep->iffp());
        iterateAndNextNull(nodep->binsp());
        iterateAndNextNull(nodep->optionsp());
    }
    ശൂന്യം visit(AstCoverCross* nodep) override {
        iterateAndNextNull(nodep->itemsp());
        scanSampleExpression(nodep->iffp());
        iterateAndNextNull(nodep->optionsp());
        iterateAndNextNull(nodep->binsp());
    }
    ശൂന്യം visit(AstCoverCrossBin* nodep) override {
        iterateAndNextNull(nodep->selectp());
        scanSampleExpression(nodep->iffp());
    }
    ശൂന്യം visit(AstCoverCrossWith* nodep) override {
        iterateAndNextNull(nodep->selectp());
        scanCoverageExpression(nodep->filterp());
        scanCoverageExpression(nodep->countp());
    }
    ശൂന്യം visit(AstCoverBinsof* nodep) override { scanCoverageExpression(nodep->rangesp()); }
    ശൂന്യം visit(AstCoverBin* nodep) override {
        scanCoverageExpression(nodep->rangesp());
        scanSampleExpression(nodep->iffp());
        scanCoverageExpression(nodep->arraySizep());
        scanCoverageExpression(nodep->transp());
    }
    ശൂന്യം visit(AstVarRef* nodep) override {
        എങ്കിൽ (!m_sampleFormalAllowed && m_sampleMembers.എണ്ണം(nodep->varp())) {
            nodep->v3error("Covergroup sample formal argument "
                           << nodep->varp()->prettyNameQ()
                           << " may only be used in a coverpoint or conditional guard "
                              "expression (IEEE 1800-2023 19.8.1).");
        }
        എങ്കിൽ (m_inCoverageExpression && m_constructorRefMembers.എണ്ണം(nodep->varp())) {
            nodep->v3error("Ref covergroup constructor formal argument "
                           << nodep->varp()->prettyNameQ()
                           << " may not be used in a covergroup expression "
                              "(IEEE 1800-2023 19.5).");
        }
    }
    ശൂന്യം visit(Ast_കണ്ണി_FTask_Ref* nodep) override {
        എങ്കിൽ (m_inCoverageExpression && nodep->taskp()) {
            തർക്കം invalidDirection = അസത്യം;
            വേണ്ടി (Ast_കണ്ണി* stmtp = nodep->taskp()->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
                സ്ഥിരം AstVar* സ്ഥിരം varp = VN_CAST(stmtp, Var);
                എങ്കിൽ (varp && varp->isIO() && varp->isWritable()) {
                    invalidDirection = സത്യം;
                    നിർത്തുക;
                }
            }
            എങ്കിൽ (invalidDirection) {
                nodep->v3error("Function " << nodep->taskp()->prettyNameQ()
                                           << " called in a covergroup expression has an "
                                              "output, inout, or non-const ref argument "
                                              "(IEEE 1800-2023 19.5).");
            }
        }
        iterateChildren(nodep);
    }
    ശൂന്യം visit(Ast_കണ്ണി* nodep) override { iterateChildren(nodep); }

പൊതു:
    Covergroup_വ്യഞ്ജകം_Valid_Visitor(സ്ഥിരം std::set<സ്ഥിരം AstVar*>& sampleMembers,
                               സ്ഥിരം std::set<സ്ഥിരം AstVar*>& constructorRefMembers)
        : m_sampleMembers{sampleMembers}
        , m_constructorRefMembers{constructorRefMembers} {}
    ശൂന്യം scan(Ast_കണ്ണി* nodep) { iterate(nodep); }
};

//######################################################################
// Bins of one declaration whose values are computed rather than listed: bin k covers
// [m_lo + k * m_stride, m_lo + (k + 1) * m_stride - 1], and the last bin extends to m_hi.  An
// array bin element is a run of single-value bins; automatic bins partition the coverpoint
// domain.  Bounds are coverpoint values at FunctionalCoverageVisitor::runWidth(),
// sign-extended like a CrossValueRange's.

വർഗ്ഗം BinRun final {
പൊതു:
    // MEMBERS
    uint32_t m_എണ്ണം;  // Number of bins
    V3_സംഖ്യ m_lo;  // Lowest value of the first bin
    V3_സംഖ്യ m_stride;  // Number of values of each bin but the last
    V3_സംഖ്യ m_hi;  // Highest value of the last bin
    തർക്കം m_empty = അസത്യം;  // A single bin without a value of the coverpoint type
    uint32_t m_declared = 0;  // Runtime index of the first bin, once generated

    // CONSTRUCTORS
    BinRun(Ast_കണ്ണി* nodep, പൂർണ്ണസംഖ്യ width, uint32_t എണ്ണം)
        : m_എണ്ണം{എണ്ണം}
        , m_lo{nodep, width}
        , m_stride{nodep, width, 1}
        , m_hi{nodep, width} {}
};

//######################################################################
// Functional coverage visitor

വർഗ്ഗം FunctionalCoverageVisitor final : പൊതു VNVisitor {
    // NODE STATE
    // Entire netlist:
    //  AstCoverpoint::user1p()  -> AstVar*.  Previous-value variable for transition bins
    //  AstCoverpoint::user2()   -> bool.  Had a bins declaration ignored, so no automatic bins
    സ്ഥിരം VNUser1InUse m_inuser1;
    സ്ഥിരം VNUser2InUse m_inuser2;

    // STATE
    std::set<AstCoverpoint*>
        m_runtimePoints;  // Points needing value metadata and live-bin mapping
    std::set<AstCoverCross*> m_runtimeCrosses;  // Crosses over finalized live-bin dimensions
    std::map<AstVar*, AstVar*> m_excludedVars;  // Sample-time state-exclusion flags
    AstClass* m_covergroupp = nullptr;  // Current covergroup being processed
    Ast_കണ്ണി_Module* m_unitp = nullptr;  // Design unit declaring the current class, outside classes
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
    തർക്കം m_cgMergeInstances = അസത്യം;  // The covergroup's type_option.merge_instances, a constant
    // The covergroup's type_option.merge_instances is, or SystemVerilog may make it, true
    തർക്കം m_cgMayMerge = അസത്യം;
    // type_option members through which SystemVerilog may set type_option.merge_instances,
    // see CovergroupAssignValidVisitor::mergeableTypeOptions()
    സ്ഥിരം std::set<സ്ഥിരം AstVar*>& m_mergeableTypeOptions;
    പൂർണ്ണസംഖ്യ m_cgAtLeast = 1;  // Covergroup option.at_least

    ഘടന EmbeddedEventTrigger final {
        File_വരി* eventFl;  // Clocking-event source location
        AstVar* baseVarp;  // Base enclosing-class member in the event expression
        AstVar* memberVarp;  // Selected member in a 'base.member' expression, or nullptr
        VEdgeType edgeType;  // Clocking-event edge qualifier
        AstVar* prevVarp;  // Member containing the previous event value, or nullptr
        EmbeddedEventTrigger(File_വരി* eventFl, AstVar* baseVarp, AstVar* memberVarp,
                             VEdgeType edgeType)
            : eventFl{eventFl}
            , baseVarp{baseVarp}
            , memberVarp{memberVarp}
            , edgeType{edgeType}
            , prevVarp{nullptr} {}
    };

    std::set<std::string> m_crossedCpNames;  // Coverpoints referenced by a cross
    std::map<std::string, AstVar*> m_cpVarMap;  // Coverpoint name -> its VlCoverpoint member
    ഘടന BinRuns final {
        std::vector<BinRun> runs;  // Runs of an array or automatic bins declaration, in order
        uint32_t എണ്ണം = 0;  // Bins across all runs
        തർക്കം unsupported = അസത്യം;  // Too many bins, or invalid: the declaration is ignored
        // The value of each bin, which names it, of a wildcard array; else the bins are indexed
        std::vector<std::string> മൂല്യങ്ങൾ;
        // Too many values of an ignore or illegal wildcard array, which is then one bin
        തർക്കം single = അസത്യം;
    };
    ഘടന Cross_Bin_മൂല്യങ്ങൾ final {
        AstCoverBin* binp;  // Declaration owning this Normal bin
        Ast_കണ്ണി_വ്യഞ്ജകം* valuep;  // Individual array-bin value, or nullptr for a scalar bin
        സ്ഥിരം BinRun* runp = nullptr;  // Run computing the bin's values, if any
        uint32_t element = 0;  // Index of the bin within runp
    };
    ഘടന BinSpan final {
        uint32_t first = 0;  // First Normal index of the bin declaration
        uint32_t എണ്ണം = 0;  // Number of Normal bins of the declaration
        uint32_t declared = 0;  // First runtime bin index, across all bin kinds
        int32_t sized = -1;  // Index of the sized array, placed at construction; or -1
    };
    ഘടന CoverpointBins final {
        uint32_t total = 0;  // Number of Normal bins
        Ast_കണ്ണി_വ്യഞ്ജകം* exprp = nullptr;  // Sampled expression, for the value domain
        തർക്കം crossed = അസത്യം;  // Feeds a cross, which needs 'values'
        std::vector<Cross_Bin_മൂല്യങ്ങൾ> മൂല്യങ്ങൾ;  // Values in runtime Normal-bin index order
        std::unordered_map<std::string, BinSpan> spans;  // Declared bin name -> index span
        BinSpan implicitAuto{0, 0, 0};  // Implicit automatic bins, each named 'auto_<i>'
        std::deque<BinRun> runs;  // Runs 'values' refers to
    };
    std::map<AstVar*, CoverpointBins> m_cpBins;  // Runtime coverpoint -> binsof index ranges
    // Names of the bins declarations each coverpoint ignored, which binsof selects as no bins
    std::map<സ്ഥിരം AstCoverpoint*, std::vector<std::string>> m_droppedBins;
    // Prefixes of the constructor temporaries of constructed bins, as coverpoint and bin names
    // alone may repeat: coverpoint 'a_' bins 'b', and coverpoint 'a' bins '_b'
    V3UniqueNames m_sizedNames{"__Vsized"};
    std::vector<Ast_കണ്ണി_വ്യഞ്ജകം*> m_detached_മൂല്യങ്ങൾ;  // Array-bin values m_cpBins refers to
    std::set<AstCoverCross*>
        m_droppedCrosses;  // Crosses with a bare-variable item: drop (COVERIGN)
    std::map<uint32_t, AstCoverpointDType*> m_cpDTypes;  // Hit-list bound -> interned dtype
    using CrossShape = std::tuple<uint32_t, uint32_t, uint32_t, uint32_t, uint64_t, തർക്കം>;
    std::map<CrossShape, AstCoverCrossDType*> m_cxDTypes;
    AstVar* m_cgInstVarp = nullptr;  // __Vcg_inst handle member of the current covergroup

    VMemberMap m_memberMap;  // Member names cached for fast lookup

    // METHODS
    // The covergroup's 'option' or static 'type_option' member (V3LinkParse creates both)
    AstVar* optionVar(തർക്കം typeOption) {
        AstVar* സ്ഥിരം varp = VN_AS(
            m_memberMap.findMember(m_covergroupp, typeOption ? "type_option" : "option"), Var);
        UASSERT_OBJ(varp, m_covergroupp, "Covergroup missing option member");
        തിരികെ varp;
    }

    // 'option.<member>' or 'type_option.<member>', per optionVarp
    AstStructSel* newOptionSel(File_വരി* fl, AstVar* optionVarp, സ്ഥിരം std::string& member,
                               VAccess access) {
        സ്ഥിരം AstMemberDType* സ്ഥിരം memberp
            = VN_AS(m_memberMap.findMember(optionVarp->dtypep()->skipRefp(), member), MemberDType);
        UASSERT_OBJ(memberp, optionVarp, "Coverage option structure missing '" << member << "'");
        Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം fromp = optionVarp->lifetime().isStatic()
                                       ? പുതിയ AstVarRef{fl, optionVarp, access}
                                       : memberRef(fl, optionVarp, access);
        AstStructSel* സ്ഥിരം selp = പുതിയ AstStructSel{fl, fromp, member};
        selp->dtypep(memberp->subDTypep()->skipRefToEnump());
        selp->didWidth(സത്യം);
        തിരികെ selp;
    }

    // A constant option.at_least; a negative count of hits has no meaning, so takes the default
    സ്ഥിതം പൂർണ്ണസംഖ്യ constAtLeast(സ്ഥിരം AstConst* constp) {
        സ്ഥിരം പൂർണ്ണസംഖ്യ മൂല്യം = constp->toSInt();
        എങ്കിൽ (മൂല്യം >= 0) തിരികെ മൂല്യം;
        constp->v3warn(COVERIGN, "Ignoring negative 'option.at_least'; using 1");
        തിരികെ 1;
    }

    // An option's constant value, or nullptr after a warning, as non-constant is unsupported
    സ്ഥിതം സ്ഥിരം AstConst* constOption(സ്ഥിരം Ast_കണ്ണി* valuep, VCoverOptionType optType) {
        എങ്കിൽ (സ്ഥിരം AstConst* സ്ഥിരം constp = VN_CAST(valuep, Const)) തിരികെ constp;
        valuep->v3warn(COVERIGN, "Ignoring unsupported: non-constant 'option."
                                     << optType.ascii() << "'; using default value");
        തിരികെ nullptr;
    }

    // Store the covergroup-level options (IEEE 1800-2023 19.7) where SystemVerilog and the
    // runtime read them.  The instance options, option.weight and option.get_inst_coverage,
    // are evaluated by the constructor, as are the other instance options; the type options,
    // type_option.weight and type_option.merge_instances, are constant, and initialize the
    // static member.  Without its own type_option.merge_instances, a covergroup has the
    // default of --coverage-merge-instances.  A constant option.at_least is the default of
    // the coverpoints and crosses.
    ശൂന്യം lowerCovergroupOptions() {
        തർക്കം mergeSet = അസത്യം;  // The covergroup sets type_option.merge_instances
        m_cgAtLeast = 1;
        വേണ്ടി (AstCgOptionAssign* സ്ഥിരം optp : m_cgOptions) {
            എങ്കിൽ (optp->optType() == VCoverOptionType::AT_LEAST) {
                എങ്കിൽ (സ്ഥിരം AstConst* സ്ഥിരം constp = constOption(optp->valuep(), optp->optType())) {
                    m_cgAtLeast = constAtLeast(constp);
                }
                VL_DO_DANGLING(തള്ളുക_Deletep(optp->unlinkFrBack()), optp);
                തുടരുക;
            }
            File_വരി* സ്ഥിരം fl = optp->fileline();
            // V3Width left the type options constant, and type_option.weight non-negative
            സ്ഥിരം AstConst* സ്ഥിരം constp = VN_CAST(optp->valuep(), Const);
            std::string member = "weight";
            എങ്കിൽ (optp->optType() == VCoverOptionType::MERGE_INSTANCES) {
                member = "merge_instances";
                m_cgMergeInstances = !constp->അക്കം().isEqZero();
                mergeSet = സത്യം;
            } അല്ലെങ്കിൽ എങ്കിൽ (optp->optType() == VCoverOptionType::GET_INST_COVERAGE) {
                member = "get_inst_coverage";
            } അല്ലെങ്കിൽ {
                UASSERT_OBJ(optp->optType() == VCoverOptionType::WEIGHT, optp,
                            "Unexpected covergroup option reaching V3Covergroup");
                എങ്കിൽ (optp->typeOption()) m_cgTypeWeight = constp->toUInt();
            }
            AstAssign* സ്ഥിരം assignp = പുതിയ AstAssign{
                fl, newOptionSel(fl, optionVar(optp->typeOption()), member, VAccess::WRITE),
                optp->valuep()->unlinkFrBack()};
            എങ്കിൽ (optp->typeOption()) {
                m_covergroupp->addMembersp(പുതിയ AstInitialStatic{fl, assignp});
                VL_DO_DANGLING(തള്ളുക_Deletep(optp->unlinkFrBack()), optp);
            } അല്ലെങ്കിൽ {
                optp->replaceWith(assignp);
                VL_DO_DANGLING(തള്ളുക_Deletep(optp), optp);
            }
        }
        m_cgOptions.clear();
        എങ്കിൽ (!mergeSet && v3Global.opt.coverageMergeInstances()) {
            File_വരി* സ്ഥിരം fl = m_covergroupp->fileline();
            m_covergroupp->addMembersp(പുതിയ AstInitialStatic{
                fl, പുതിയ AstAssign{
                        fl, newOptionSel(fl, optionVar(സത്യം), "merge_instances", VAccess::WRITE),
                        പുതിയ AstConst{fl, AstConst::BitTrue{}}}});
            m_cgMergeInstances = സത്യം;
        }
        m_cgMayMerge = m_cgMergeInstances || m_mergeableTypeOptions.എണ്ണം(optionVar(സത്യം));
    }

    // The weight of an item in the coverage database, which merges the instances: its
    // type_option.weight if the covergroup merges them too (IEEE 1800-2023 19.11.3); else its
    // option.weight if a constant, and so of every instance; else its type_option.weight
    uint32_t itemDatabaseWeight(Ast_കണ്ണി* optionsp) സ്ഥിരം {
        സ്ഥിരം Ast_കണ്ണി_വ്യഞ്ജകം* weightp = nullptr;  // The option.weight in effect
        uint32_t typeWeight = 1;
        വേണ്ടി (Ast_കണ്ണി* nodep = optionsp; nodep; nodep = nodep->nextp()) {
            സ്ഥിരം AstCoverOption* സ്ഥിരം optp = VN_AS(nodep, CoverOption);
            എങ്കിൽ (!(optp->optType() == VCoverOptionType::WEIGHT)) തുടരുക;
            // V3Width left type_option.weight a non-negative constant
            എങ്കിൽ (optp->typeOption()) {
                typeWeight = VN_AS(optp->valuep(), Const)->toUInt();
            } അല്ലെങ്കിൽ {
                weightp = optp->valuep();
            }
        }
        എങ്കിൽ (m_cgMergeInstances) തിരികെ typeWeight;
        എങ്കിൽ (!weightp) തിരികെ 1;
        എങ്കിൽ (സ്ഥിരം AstConst* സ്ഥിരം constp = VN_CAST(weightp, Const)) തിരികെ constp->toUInt();
        തിരികെ typeWeight;
    }

    // Configure an item's option.weight, its weight in instance coverage (IEEE 1800-2023
    // 19.11), and, if the covergroup may merge its instances, its type_option.weight, its
    // weight in type coverage then (19.11.3).
    ശൂന്യം generateItemWeight(File_വരി* fl, AstVar* itemVarp, Ast_കണ്ണി* optionsp) {
        വേണ്ടി (Ast_കണ്ണി* nodep = optionsp; nodep; nodep = nodep->nextp()) {
            സ്ഥിരം AstCoverOption* സ്ഥിരം optp = VN_AS(nodep, CoverOption);
            എങ്കിൽ (!(optp->optType() == VCoverOptionType::WEIGHT)) തുടരുക;
            എങ്കിൽ (!optp->typeOption()) {
                m_constructorp->addStmtsp(
                    itemCall(fl, itemVarp, VCMethod::COVERGROUP_WEIGHT,
                             {optp->valuep()->clone_വൃക്ഷം(അസത്യം), file_വരി_Debug(optp->fileline())})
                        ->makeStmt());
            } അല്ലെങ്കിൽ എങ്കിൽ (m_cgMayMerge) {
                // V3Width left type_option.weight a non-negative constant
                m_constructorp->addStmtsp(itemCall(fl, itemVarp, VCMethod::COVERGROUP_TYPE_WEIGHT,
                                                   {optp->valuep()->clone_വൃക്ഷം(അസത്യം)})
                                              ->makeStmt());
            }
        }
    }

    ശൂന്യം processCovergroup() {
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
        m_cgMergeInstances = അസത്യം;
        m_cgMayMerge = അസത്യം;

        lowerCovergroupOptions();

        // Scan every cross item to record the coverpoints it references (the cross dimensions)
        // and to flag any cross naming a bare variable -- a would-be implicit coverpoint, which
        // Verilator does not synthesize.  An unresolvable item drops only that one cross (with a
        // COVERIGN in generateCrossCode), leaving the rest of the covergroup intact.
        വേണ്ടി (AstCoverCross* crossp : m_coverCrosses) {
            വേണ്ടി (Ast_കണ്ണി* itemp = crossp->itemsp(); itemp; itemp = itemp->nextp()) {
                സ്ഥിരം AstCoverpointRef* സ്ഥിരം refp = VN_AS(itemp, CoverpointRef);
                എങ്കിൽ (refp->exprp()) തുടരുക;  // hierarchical ref: dropped in generateCrossCode
                എങ്കിൽ (m_coverpointMap.find(refp->name()) == m_coverpointMap.end()) {
                    m_droppedCrosses.തിരുകുക(crossp);  // bare variable: drop this cross only
                } അല്ലെങ്കിൽ {
                    m_crossedCpNames.തിരുകുക(refp->name());
                }
            }
        }

        std::vector<Ast_കണ്ണി*> pending;
        std::map<AstCoverpoint*, std::vector<AstCoverCross*>> consumers;
        std::map<AstCoverCross*, std::vector<AstCoverpoint*>> inputs;
        വേണ്ടി (AstCoverpoint* സ്ഥിരം cpp : m_coverpoints) {
            checkBinNames(cpp);
            checkConstructedBins(cpp);
            എങ്കിൽ (!cpp->exprp()->dtypep()->skipRefp()->isIntegralOrPacked()) തുടരുക;
            // Bins without values leave the report (IEEE 1800-2023 19.11.1), exclusions or not.
            // Constructed bins get their values when the covergroup is constructed.
            എങ്കിൽ (!coverpointHasStateExclusions(cpp) && !coverpointHasEmptyBins(cpp)
                && !coverpointHasConstructedBins(cpp)) {
                തുടരുക;
            }
            m_runtimePoints.തിരുകുക(cpp);
            pending.തള്ളുക_back(cpp);
        }
        വേണ്ടി (AstCoverCross* സ്ഥിരം crossp : m_coverCrosses) {
            എങ്കിൽ (m_droppedCrosses.എണ്ണം(crossp)) തുടരുക;
            വേണ്ടി (Ast_കണ്ണി* itemp = crossp->itemsp(); itemp; itemp = itemp->nextp()) {
                സ്ഥിരം AstCoverpointRef* സ്ഥിരം refp = VN_AS(itemp, CoverpointRef);
                എങ്കിൽ (refp->exprp()) തുടരുക;
                സ്ഥിരം auto point = m_coverpointMap.find(refp->name());
                എങ്കിൽ (point != m_coverpointMap.end()) {
                    consumers[point->second].തള്ളുക_back(crossp);
                    inputs[crossp].തള്ളുക_back(point->second);
                }
            }
            // The values of a filter's bin tuples are known when the covergroup is constructed
            എങ്കിൽ (crossp->exists([](സ്ഥിരം AstCoverCrossWith*) { തിരികെ സത്യം; })) {
                m_runtimeCrosses.emplace(crossp);
                pending.തള്ളുക_back(crossp);
            }
        }
        വേണ്ടി (size_t next = 0; next < pending.size(); ++next) {
            എങ്കിൽ (AstCoverpoint* സ്ഥിരം pointp = VN_CAST(pending[next], Coverpoint)) {
                വേണ്ടി (AstCoverCross* സ്ഥിരം crossp : consumers[pointp]) {
                    എങ്കിൽ (m_runtimeCrosses.emplace(crossp).second) pending.തള്ളുക_back(crossp);
                }
            } അല്ലെങ്കിൽ {
                വേണ്ടി (AstCoverpoint* സ്ഥിരം pointp : inputs[VN_AS(pending[next], CoverCross)]) {
                    എങ്കിൽ (pointp->exprp()->dtypep()->skipRefp()->isIntegralOrPacked()
                        && m_runtimePoints.emplace(pointp).second) {
                        pending.തള്ളുക_back(pointp);
                    }
                }
            }
        }

        // The instance node owns this instance's coverpoint/cross runtimes, so it must exist
        // before any of them is created.  Emitted first, ahead of both generate loops.
        generateInstanceAttach();

        // For each coverpoint, generate sampling code
        വേണ്ടി (AstCoverpoint* cpp : m_coverpoints) generateCoverpointCode(cpp);

        // For each cross, generate sampling code
        വേണ്ടി (AstCoverCross* crossp : m_coverCrosses) generateCrossCode(crossp);

        // Every cross has been built, so runtime points only need their exclusions from here.
        വേണ്ടി (AstCoverpoint* സ്ഥിരം cpp : m_coverpoints) {
            എങ്കിൽ (!m_runtimePoints.എണ്ണം(cpp)) തുടരുക;
            m_constructorp->addStmtsp(itemCall(cpp->fileline(), m_cpVarMap.at(cpp->name()),
                                               VCMethod::COVERGROUP_മൂല്യം_RELEASE)
                                          ->makeStmt());
        }
        വേണ്ടി (Ast_കണ്ണി_വ്യഞ്ജകം* valuep : m_detached_മൂല്യങ്ങൾ) VL_DO_DANGLING(തള്ളുക_Deletep(valuep), valuep);
        m_detached_മൂല്യങ്ങൾ.clear();

        // Generate coverage computation code (even for empty covergroups).  Bin registration
        // with the coverage database is handled per coverpoint/cross by their runtime
        // registerBins() calls (emitted in generateCoverpoint/generateCross).
        generateCoverageComputationCode();
    }

    സ്ഥിതം constexpr size_t മൂല്യം_LIST_ENTRIES = 256;  // Metadata entries per constructor call

    // The value of a constant count, of bins or value tuples: -1 if it is negative, and
    // saturated above the largest limit
    സ്ഥിതം int64_t const_എണ്ണം(സ്ഥിരം AstConst* constp) {
        സ്ഥിരം V3_സംഖ്യ& അക്കം = constp->അക്കം();
        എങ്കിൽ (constp->isSigned() && അക്കം.isNegative()) തിരികെ -1;
        തിരികെ അക്കം.mostSetBitP1() > 32 ? INT64_MAX : static_cast<int64_t>(അക്കം.toUQuad());
    }

    // Whether a count, 'whatp', is integral and, if constant, positive, as the standard's clause
    // 'clausep' requires; else errors
    സ്ഥിതം തർക്കം check_എണ്ണം(സ്ഥിരം Ast_കണ്ണി_വ്യഞ്ജകം* countp, സ്ഥിരം അക്ഷരം* whatp, സ്ഥിരം അക്ഷരം* clausep) {
        സ്ഥിരം AstConst* സ്ഥിരം constp = VN_CAST(countp, Const);
        എങ്കിൽ (!countp->dtypep()->skipRefp()->isIntegralOrPacked()) {
            countp->v3error(whatp << " must be integral (IEEE 1800-2023 " << clausep << ")");
            തിരികെ അസത്യം;
        } അല്ലെങ്കിൽ എങ്കിൽ (constp && (constp->അക്കം().isFourState() || const_എണ്ണം(constp) < 1)) {
            countp->v3error(whatp << " must be >= 1, got "
                                  << (constp->അക്കം().isFourState() ? constp->അക്കം().ascii(അസത്യം)
                                                                  : constp->അക്കം().toDecimalS())
                                  << " (IEEE 1800-2023 " << clausep << ")");
            തിരികെ അസത്യം;
        }
        തിരികെ സത്യം;
    }

    // The number of bins requested by a valid 'bins auto[N]', or 0
    സ്ഥിതം uint32_t autoBinsRequested(സ്ഥിരം AstCoverBin* binp) {
        സ്ഥിരം AstConst* സ്ഥിരം constp = VN_CAST(binp->arraySizep(), Const);
        എങ്കിൽ (!constp) തിരികെ 0;
        സ്ഥിരം int64_t എണ്ണം = const_എണ്ണം(constp);
        തിരികെ എണ്ണം < 1 || എണ്ണം > v3Global.opt.coverageMaxBins() ? 0
                                                                   : static_cast<uint32_t>(എണ്ണം);
    }

    // True for a 'bins auto[N]' declaration, or the implicit automatic bins of a coverpoint
    സ്ഥിതം തർക്കം isAutoBins(സ്ഥിരം AstCoverBin* binp) {
        തിരികെ binp->binsType() == VCoverBinsType::BINS_AUTO
               || binp->binsType() == VCoverBinsType::BINS_AUTO_IMPLICIT;
    }

    // True for a sized array of bins, 'bins b[N] = {...}', whose values an integral coverpoint
    // distributes over N bins when the covergroup is constructed (IEEE 1800-2023 19.5.1)
    സ്ഥിതം തർക്കം isSizedArray(സ്ഥിരം AstCoverBin* binp) {
        തിരികെ binp->arraySizep() && !isAutoBins(binp);
    }

    // The 'with' filter of a bin, if any (IEEE 1800-2023 19.5.1.1)
    സ്ഥിതം AstCoverWith* binWith(സ്ഥിരം AstCoverBin* binp) {
        തിരികെ VN_CAST(binp->rangesp(), CoverWith);
    }

    // True for bins whose values the covergroup constructor computes: a sized array, or bins of
    // the values a 'with' filter keeps
    സ്ഥിതം തർക്കം isConstructedBins(സ്ഥിരം AstCoverBin* binp) {
        തിരികെ isSizedArray(binp) || binWith(binp);
    }

    // The range list of a bin's values, or of the candidates of its 'with' filter; null for all
    // of the coverpoint's values, which a filter of the coverpoint's name has
    സ്ഥിതം Ast_കണ്ണി* binRangesp(സ്ഥിരം AstCoverBin* binp) {
        സ്ഥിരം AstCoverWith* സ്ഥിരം withp = binWith(binp);
        എങ്കിൽ (!withp) തിരികെ binp->rangesp();
        തിരികെ VN_IS(withp->subp(), CoverpointRef) ? nullptr : withp->subp();
    }

    // Report and delete a bins declaration of a name that another of its coverpoint has
    ശൂന്യം checkBinNames(AstCoverpoint* coverpointp) {
        std::set<std::string> names;
        വേണ്ടി (Ast_കണ്ണി* nodep = coverpointp->binsp(); nodep;) {
            AstCoverBin* സ്ഥിരം binp = VN_AS(nodep, CoverBin);
            nodep = nodep->nextp();
            എങ്കിൽ (names.emplace(binp->name()).second) തുടരുക;
            binp->v3error("Duplicate bin " << binp->prettyNameQ() << " in coverpoint "
                                           << coverpointp->prettyNameQ()
                                           << " (IEEE 1800-2023 3.13)");
            VL_DO_DANGLING(തള്ളുക_Deletep(binp->unlinkFrBack()), binp);
        }
    }

    // Delete an ignored bins declaration, which binsof then selects as no bins
    ശൂന്യം dropBins(സ്ഥിരം AstCoverpoint* coverpointp, AstCoverBin* binp) {
        m_droppedBins[coverpointp].തള്ളുക_back(binp->name());
        VL_DO_DANGLING(തള്ളുക_Deletep(binp->unlinkFrBack()), binp);
    }

    // Check the size of a sized array of bins, which drops an invalid array.  A real coverpoint's
    // are unsupported, and treated as arrays of a bin per value.  False unless it stays sized.
    തർക്കം checkBinsArraySize(സ്ഥിരം AstCoverpoint* coverpointp, AstCoverBin* binp, തർക്കം integral) {
        Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം sizep = binp->arraySizep();
        എങ്കിൽ (VN_IS(sizep, Unbounded)) {  // A parameter of '$'; see bins_orBraE
            binp->v3error("Bins array size must be integral, not '$' (IEEE 1800-2023 19.5.1)");
        } അല്ലെങ്കിൽ എങ്കിൽ (check_എണ്ണം(sizep, "Bins array size", "19.5.1")) {
            എങ്കിൽ (integral) തിരികെ സത്യം;
            binp->v3warn(COVERIGN, "Unsupported: 'bins' explicit array size of a real "
                                   "coverpoint (treated as '[]')");
            VL_DO_DANGLING(തള്ളുക_Deletep(sizep->unlinkFrBack()), sizep);
            തിരികെ അസത്യം;
        }
        dropBins(coverpointp, binp);
        തിരികെ അസത്യം;
    }

    // Check the bins of a coverpoint whose values the constructor computes, dropping invalid
    // ones.  A wildcard array of too many ranges of values is ignored, or if ignore or illegal,
    // treated as one bin; filtering too many ranges of values ignores the bins.
    ശൂന്യം checkConstructedBins(AstCoverpoint* coverpointp) {
        സ്ഥിരം തർക്കം integral = coverpointp->exprp()->dtypep()->skipRefp()->isIntegralOrPacked();
        വേണ്ടി (Ast_കണ്ണി* nodep = coverpointp->binsp(); nodep;) {
            AstCoverBin* സ്ഥിരം binp = VN_AS(nodep, CoverBin);
            nodep = nodep->nextp();
            എങ്കിൽ (!isConstructedBins(binp)) തുടരുക;
            എങ്കിൽ (isSizedArray(binp) && !checkBinsArraySize(coverpointp, binp, integral)) തുടരുക;
            എങ്കിൽ (!binp->isWildcard()
                || sizedWildcardRuns(binp, coverpointp->exprp())
                       <= v3Global.opt.coverageMaxBins()) {
                എങ്കിൽ (binWith(binp) && withCandidatesOver(binp, coverpointp->exprp())) {
                    binp->v3warn(COVERIGN, "Unsupported: 'with' filter of more than 2**32 "
                                           "candidate values; bin "
                                               << binp->prettyNameQ() << " ignored");
                    എങ്കിൽ (binp->binsType().binIsNormal()) coverpointp->user2(സത്യം);
                    dropBins(coverpointp, binp);
                }
                തുടരുക;
            }
            // An ignore or illegal array still excludes or checks its values, as one bin
            സ്ഥിരം തർക്കം single = !binp->binsType().binIsNormal() && !binWith(binp);
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
            എങ്കിൽ (single) {
                Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം sizep = binp->arraySizep();
                VL_DO_DANGLING(തള്ളുക_Deletep(sizep->unlinkFrBack()), sizep);
                binp->isArray(അസത്യം);
                തുടരുക;
            }
            എങ്കിൽ (binp->binsType().binIsNormal()) coverpointp->user2(സത്യം);
            dropBins(coverpointp, binp);
        }
    }

    // True if a 'with' filter would be evaluated for more than 2**32 candidate values, known
    // now for the coverpoint's name, or for a range list of constants: each is evaluated once
    // but for an array 'b[N]', which keeps their order and duplicates (see withBegin())
    സ്ഥിതം തർക്കം withCandidatesOver(AstCoverBin* binp, Ast_കണ്ണി_വ്യഞ്ജകം* exprp) {
        സ്ഥിരം പൂർണ്ണസംഖ്യ width = runWidth(exprp);
        std::vector<std::pair<V3_സംഖ്യ, V3_സംഖ്യ>> runs;
        എങ്കിൽ (!binRangesp(binp)) runs = coverpoint_മൂല്യങ്ങൾ(binp, exprp);
        സ്ഥിരം auto constant
            = [](സ്ഥിരം Ast_കണ്ണി* nodep) { തിരികെ VN_IS(nodep, Const) || VN_IS(nodep, Unbounded); };
        വേണ്ടി (Ast_കണ്ണി* rangep = binRangesp(binp); rangep; rangep = rangep->nextp()) {
            സ്ഥിരം AstInsideRange* സ്ഥിരം irp = VN_CAST(rangep, InsideRange);
            // Else known when constructed
            എങ്കിൽ (irp ? !constant(irp->lhsp()) || !constant(irp->rhsp()) : !VN_IS(rangep, Const)) {
                തിരികെ അസത്യം;
            }
            Cross_മൂല്യം_Range range{rangep, resolveWidth(rangep, exprp)};
            എങ്കിൽ (!resolve_മൂല്യം(rangep, exprp, സത്യം, binp->isWildcard(), range)
                || crossRangeEmpty(range)) {
                തുടരുക;
            }
            std::vector<std::pair<V3_സംഖ്യ, V3_സംഖ്യ>> found{{range.lo, range.hi}};
            എങ്കിൽ (range.wildcard) {  // checkConstructedBins bounded the runs
                found.clear();
                crossRangeRuns(range, v3Global.opt.coverageMaxBins(), found);
            }
            വേണ്ടി (സ്ഥിരം std::pair<V3_സംഖ്യ, V3_സംഖ്യ>& run : found) {
                runs.emplace_back(V3_സംഖ്യ{rangep, width, run.first},
                                  V3_സംഖ്യ{rangep, width, run.second});
            }
        }
        എങ്കിൽ (!isSizedArray(binp)) {  // The union of the values
            std::sort(runs.begin(), runs.end(), [](സ്ഥിരം auto& lhs, സ്ഥിരം auto& rhs) {
                തിരികെ cross_മൂല്യം_Less(lhs.first, rhs.first);
            });
            std::vector<std::pair<V3_സംഖ്യ, V3_സംഖ്യ>> merged;
            വേണ്ടി (സ്ഥിരം std::pair<V3_സംഖ്യ, V3_സംഖ്യ>& run : runs) {
                എങ്കിൽ (merged.empty() || cross_മൂല്യം_Less(merged.back().second, run.first)) {
                    merged.തള്ളുക_back(run);
                } അല്ലെങ്കിൽ എങ്കിൽ (cross_മൂല്യം_Less(merged.back().second, run.second)) {
                    merged.back().second = run.second;
                }
            }
            runs = std::move(merged);
        }
        uint64_t എണ്ണം = 0;
        വേണ്ടി (സ്ഥിരം std::pair<V3_സംഖ്യ, V3_സംഖ്യ>& run : runs) {
            V3_സംഖ്യ span{exprp, width};
            span.opSub(run.second, run.first);
            എങ്കിൽ (span.mostSetBitP1() > 32) തിരികെ സത്യം;  // More than 2**32 values
            എണ്ണം += span.toUQuad() + 1;
            എങ്കിൽ (എണ്ണം > (uint64_t{1} << 32)) തിരികെ സത്യം;
        }
        തിരികെ അസത്യം;
    }

    // The ranges of values the wildcard patterns of a sized wildcard array, or of a 'with'
    // filter's candidates, give, counted up to more than --coverage-max-bins
    സ്ഥിതം size_t sizedWildcardRuns(സ്ഥിരം AstCoverBin* binp, Ast_കണ്ണി_വ്യഞ്ജകം* exprp) {
        std::vector<std::pair<V3_സംഖ്യ, V3_സംഖ്യ>> runs;
        വേണ്ടി (Ast_കണ്ണി* rangep = binRangesp(binp); rangep; rangep = rangep->nextp()) {
            എങ്കിൽ (!VN_IS(rangep, Const)) തുടരുക;  // A range, or a value known at construction
            Cross_മൂല്യം_Range range{rangep, resolveWidth(rangep, exprp)};
            എങ്കിൽ (resolve_മൂല്യം(rangep, exprp, സത്യം, സത്യം, range) && !crossRangeEmpty(range)) {
                crossRangeRuns(range, v3Global.opt.coverageMaxBins(), runs);
            }
        }
        തിരികെ runs.size();
    }

    // Check the automatic bins declarations of a coverpoint.  Each stays one declaration, which
    // generates as a partition of the coverpoint domain (see autoBinRuns).
    ശൂന്യം checkAutomaticBins(AstCoverpoint* coverpointp, സ്ഥിരം Ast_കണ്ണി_വ്യഞ്ജകം* exprp) {
        വേണ്ടി (Ast_കണ്ണി* binp = coverpointp->binsp(); binp; binp = binp->nextp()) {
            AstCoverBin* സ്ഥിരം cbinp = VN_AS(binp, CoverBin);
            എങ്കിൽ (cbinp->binsType() != VCoverBinsType::BINS_AUTO) തുടരുക;
            സ്ഥിരം AstConst* സ്ഥിരം constp = VN_CAST(cbinp->arraySizep(), Const);
            എങ്കിൽ (!constp) {
                cbinp->v3error("Automatic bins array size must be a constant");
            } അല്ലെങ്കിൽ എങ്കിൽ (const_എണ്ണം(constp) < 1) {
                cbinp->v3error("Automatic bins array size must be >= 1, got "
                               << constp->അക്കം().toDecimalS());
            } അല്ലെങ്കിൽ എങ്കിൽ (const_എണ്ണം(constp) > v3Global.opt.coverageMaxBins()) {
                cbinp->v3error("Automatic bins array size of "
                               << constp->അക്കം().toDecimalU() << " exceeds limit of "
                               << v3Global.opt.coverageMaxBins() << '\n'
                               << cbinp->warnMore() << "... Suggest a larger --coverage-max-bins");
            } അല്ലെങ്കിൽ എങ്കിൽ (!exprp->dtypep()->skipRefp()->isIntegralOrPacked()) {
                cbinp->v3error("Automatic bins are not allowed on a coverpoint of a non-integral "
                               "expression (IEEE 1800-2023 19.5.3).");
            }
        }
    }

    // Extract all option values of a coverpoint or cross in a single pass.
    // atLeastOut: option.at_least (default the covergroup's)
    // autoBinMaxOut: option.auto_bin_max (coverpoint overrides covergroup, default 64)
    ശൂന്യം extractItemOptions(Ast_കണ്ണി* optionsp, പൂർണ്ണസംഖ്യ& atLeastOut, പൂർണ്ണസംഖ്യ& autoBinMaxOut) {
        atLeastOut = m_cgAtLeast;
        autoBinMaxOut = -1;  // -1 = not set at coverpoint level
        വേണ്ടി (Ast_കണ്ണി* optionp = optionsp; optionp; optionp = optionp->nextp()) {
            AstCoverOption* സ്ഥിരം optp = VN_AS(optionp, CoverOption);
            // Weights may be non-constant; generateItemWeight() handles them.  A cross keeps
            // its unsupported options, which V3LinkParse warned of.
            എങ്കിൽ (!(optp->optType() == VCoverOptionType::AT_LEAST
                  || optp->optType() == VCoverOptionType::AUTO_BIN_MAX)) {
                തുടരുക;
            }
            സ്ഥിരം AstConst* സ്ഥിരം constp = constOption(optp->valuep(), optp->optType());
            എങ്കിൽ (!constp) തുടരുക;
            എങ്കിൽ (optp->optType() == VCoverOptionType::AT_LEAST) {
                atLeastOut = constAtLeast(constp);
            } അല്ലെങ്കിൽ {
                autoBinMaxOut = constp->toSInt();
            }
        }
        // Fall back to covergroup-level auto_bin_max if not set at coverpoint level
        എങ്കിൽ (autoBinMaxOut < 0) {
            എങ്കിൽ (m_covergroupp->cgAutoBinMax() >= 0) {
                autoBinMaxOut = m_covergroupp->cgAutoBinMax();
            } അല്ലെങ്കിൽ {
                autoBinMaxOut = 64;  // Default per IEEE 1800-2023 Table 19-1
            }
        }
    }

    // IEEE 1800-2023 19.5.2: an enum coverpoint has one automatic bin per enumeration value
    ശൂന്യം createEnumAutoBins(AstCoverpoint* coverpointp, Ast_കണ്ണി_വ്യഞ്ജകം* exprp,
                            സ്ഥിരം AstEnumDType* enump) {
        File_വരി* സ്ഥിരം fl = coverpointp->fileline();
        വേണ്ടി (സ്ഥിരം AstEnumItem* itemp = enump->itemsp(); itemp;
             itemp = VN_AS(itemp->nextp(), EnumItem)) {
            AstConst* സ്ഥിരം lop = new_മൂല്യം_Const(fl, VN_AS(itemp->valuep(), Const)->അക്കം(), exprp);
            AstInsideRange* സ്ഥിരം rangep = പുതിയ AstInsideRange{fl, lop, lop->clone_വൃക്ഷം(അസത്യം)};
            rangep->dtypeFrom(exprp);
            coverpointp->addBinsp(
                പുതിയ AstCoverBin{fl, "auto[" + itemp->name() + "]", rangep, അസത്യം, അസത്യം});
        }
    }

    // IEEE 1800-2023 19.5.3/19.11.1: partition first, then apply exclusions.  The partition is one
    // automatic bins declaration, generated as a run like 'bins auto[N]' but numbering its bins.
    ശൂന്യം createImplicitAutoBins(AstCoverpoint* coverpointp, Ast_കണ്ണി_വ്യഞ്ജകം* exprp, പൂർണ്ണസംഖ്യ autoBinMax) {
        എങ്കിൽ (coverpointp->user2()) തിരികെ;  // Declared bins, ignored, leave no bins
        വേണ്ടി (Ast_കണ്ണി* nodep = coverpointp->binsp(); nodep; nodep = nodep->nextp()) {
            സ്ഥിരം VCoverBinsType kind = VN_AS(nodep, CoverBin)->binsType();
            എങ്കിൽ (kind != VCoverBinsType::BINS_IGNORE && kind != VCoverBinsType::BINS_ILLEGAL)
                തിരികെ;
        }
        എങ്കിൽ (സ്ഥിരം AstEnumDType* സ്ഥിരം enump
            = VN_CAST(exprp->dtypep()->skipRefToEnump(), EnumDType)) {
            createEnumAutoBins(coverpointp, exprp, enump);
            തിരികെ;
        }
        സ്ഥിരം പൂർണ്ണസംഖ്യ width = exprp->width();
        uint32_t എണ്ണം = width < 31 ? std::min<uint32_t>(uint32_t{1} << width, autoBinMax)
                                    : static_cast<uint32_t>(autoBinMax);
        എങ്കിൽ (!എണ്ണം) തിരികെ;
        എങ്കിൽ (!exprp->dtypep()->skipRefp()->isIntegralOrPacked()) {
            coverpointp->v3error("Coverpoint of a non-integral expression requires explicit bins "
                                 "(IEEE 1800-2023 19.5.3).");
            തിരികെ;
        }
        എങ്കിൽ (എണ്ണം > v3Global.opt.coverageMaxBins()) {
            coverpointp->v3warn(COVERIGN, "Unsupported: more than "
                                              << v3Global.opt.coverageMaxBins()
                                              << " automatic bins from 'option.auto_bin_max'; "
                                                 "using "
                                              << v3Global.opt.coverageMaxBins() << ".\n"
                                              << coverpointp->warnMore()
                                              << "... Suggest a larger --coverage-max-bins");
            എണ്ണം = v3Global.opt.coverageMaxBins();
        }
        File_വരി* സ്ഥിരം fl = coverpointp->fileline();
        coverpointp->addBinsp(പുതിയ AstCoverBin{fl, "auto", പുതിയ AstConst{fl, എണ്ണം},
                                              VCoverBinsType::BINS_AUTO_IMPLICIT});
    }

    // Sanitize generated names to be valid C++ identifiers
    സ്ഥിതം string sanitizeGeneratedName(string name) {
        std::replace(name.begin(), name.end(), '[', '_');
        std::replace(name.begin(), name.end(), ']', '_');
        തിരികെ name;
    }

    // Capture an iff guard in a function-local temporary so it is evaluated once per sample()
    AstVarRef* captureIffToTemp(Ast_കണ്ണി_വ്യഞ്ജകം* iffp, സ്ഥിരം string& tempName) {
        File_വരി* സ്ഥിരം fl = iffp->fileline();
        AstVar* സ്ഥിരം iffVarp
            = പുതിയ AstVar{fl, VVarType::BLOCKTEMP, tempName, iffp->findBitDType()};
        iffVarp->funcLocal(സത്യം);
        m_sampleFuncp->addStmtsp(iffVarp);
        iffp->unlinkFrBack();
        m_sampleFuncp->addStmtsp(
            പുതിയ AstAssign{fl, പുതിയ AstVarRef{fl, iffVarp, VAccess::WRITE}, iffp});
        തിരികെ പുതിയ AstVarRef{fl, iffVarp, VAccess::READ};
    }

    // Add a statement sampling the current coverpoint to sample(), under its iff guard if any
    ശൂന്യം addSampleStmt(Ast_കണ്ണി* stmtp) {
        എങ്കിൽ (m_cpGuardp) {
            m_cpGuardp->addThensp(stmtp);
        } അല്ലെങ്കിൽ {
            m_sampleFuncp->addStmtsp(stmtp);
        }
    }

    // Create previous value variable for transition tracking
    AstVar* create_Prev_മൂല്യം_Var(AstCoverpoint* coverpointp, Ast_കണ്ണി_വ്യഞ്ജകം* exprp) {
        // Check if already created
        എങ്കിൽ (AstVar* സ്ഥിരം prevVarp = VN_CAST(coverpointp->user1p(), Var)) തിരികെ prevVarp;

        // Create variable to store previous sampled value
        സ്ഥിരം string varName = "__Vprev_" + coverpointp->name();
        AstVar* prevVarp
            = പുതിയ AstVar{coverpointp->fileline(), VVarType::MEMBER, varName, exprp->dtypep()};
        m_covergroupp->addMembersp(prevVarp);

        UINFO(4, "    Created previous value variable: " << varName);

        // Initialize to zero in constructor
        Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം initExprp
            = പുതിയ AstConst{prevVarp->fileline(), AstConst::Widthed_മൂല്യം{}, prevVarp->width(), 0};
        Ast_കണ്ണി_Stmt* സ്ഥിരം initStmtp = പുതിയ AstAssign{
            prevVarp->fileline(), പുതിയ AstVarRef{prevVarp->fileline(), prevVarp, VAccess::WRITE},
            initExprp};
        m_constructorp->addStmtsp(initStmtp);

        coverpointp->user1p(prevVarp);
        തിരികെ prevVarp;
    }

    // Create state position variable for multi-value transition bins
    // Tracks position in sequence: 0=not started, 1=seen first item, etc.
    AstVar* createSequenceStateVar(AstCoverpoint* coverpointp, AstCoverBin* binp) {
        // Create variable to track sequence position
        സ്ഥിരം string varName = "__Vseqpos_" + coverpointp->name() + "_" + binp->name();
        // Use 8-bit integer for state position (sequences rarely > 255 items)
        AstVar* stateVarp
            = പുതിയ AstVar{binp->fileline(), VVarType::MEMBER, varName, VFlagLogicPacked{}, 8};
        m_covergroupp->addMembersp(stateVarp);

        UINFO(4, "    Created sequence state variable: " << varName);

        // Initialize to 0 (not started) in constructor
        Ast_കണ്ണി_Stmt* സ്ഥിരം initStmtp = പുതിയ AstAssign{
            stateVarp->fileline(), പുതിയ AstVarRef{stateVarp->fileline(), stateVarp, VAccess::WRITE},
            പുതിയ AstConst{stateVarp->fileline(), AstConst::Widthed_മൂല്യം{}, 8, 0}};
        m_constructorp->addStmtsp(initStmtp);

        തിരികെ stateVarp;
    }

    ശൂന്യം generateCoverpointCode(AstCoverpoint* coverpointp) {
        UINFO(4, "  Generating code for coverpoint: " << coverpointp->name());

        // Get the coverpoint expression
        Ast_കണ്ണി_വ്യഞ്ജകം* exprp = coverpointp->exprp();

        // Check automatic bins before processing
        checkAutomaticBins(coverpointp, exprp);

        // Extract all coverpoint options in a single pass
        പൂർണ്ണസംഖ്യ at_Least_മൂല്യം;
        പൂർണ്ണസംഖ്യ autoBinMax;
        extractItemOptions(coverpointp->optionsp(), at_Least_മൂല്യം, autoBinMax);
        UINFO(6, "    Coverpoint at_least = " << at_Least_മൂല്യം << " auto_bin_max = " << autoBinMax);

        // Create implicit automatic bins if no regular bins exist
        createImplicitAutoBins(coverpointp, exprp, autoBinMax);

        // A false iff guard disables sampling of the coverpoint (IEEE 1800-2023 19.5), so
        // neither is its expression evaluated, nor are its bins or transition state updated
        VL_RESTORER(m_cpGuardp);
        എങ്കിൽ (Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം iffp = coverpointp->iffp()) {
            m_cpGuardp = പുതിയ AstIf{iffp->fileline(), iffp->unlinkFrBack()};
        }

        AstVar* സ്ഥിരം മൂല്യം_Varp = പുതിയ AstVar{
            coverpointp->fileline(), VVarType::BLOCKTEMP,
            "__VcpValue_" + sanitizeGeneratedName(coverpointp->name()), exprp->dtypep()};
        മൂല്യം_Varp->funcLocal(സത്യം);
        m_sampleFuncp->addStmtsp(മൂല്യം_Varp);
        exprp->unlinkFrBack();
        addSampleStmt(പുതിയ AstAssign{
            coverpointp->fileline(),
            പുതിയ AstVarRef{coverpointp->fileline(), മൂല്യം_Varp, VAccess::WRITE}, exprp});
        coverpointp->exprp(പുതിയ AstVarRef{coverpointp->fileline(), മൂല്യം_Varp, VAccess::READ});
        exprp = coverpointp->exprp();

        // Every coverpoint routes through the VlCoverpoint runtime.  Transition coverpoints are
        // included: their per-value matching is still generated as a state machine in sample()
        // (see generateCoverpoint), but the bin hit is recorded in the runtime bin
        // rather than a bare counter.
        generateCoverpoint(coverpointp, exprp, at_Least_മൂല്യം);
        // Sampling follows what generateCoverpoint added unguarded, such as clearing the hit list
        എങ്കിൽ (m_cpGuardp) m_sampleFuncp->addStmtsp(m_cpGuardp);
    }

    // Build the condition under which a default bin matches: NOT(OR of all normal bins).
    Ast_കണ്ണി_വ്യഞ്ജകം* buildDefaultCondition(AstCoverpoint* coverpointp, Ast_കണ്ണി_വ്യഞ്ജകം* exprp,
                                       File_വരി* fl) {
        Ast_കണ്ണി_വ്യഞ്ജകം* anyBinMatchp = nullptr;
        വേണ്ടി (Ast_കണ്ണി* binp = coverpointp->binsp(); binp; binp = binp->nextp()) {
            AstCoverBin* സ്ഥിരം cbinp = VN_AS(binp, CoverBin);
            എങ്കിൽ (cbinp->binsType() == VCoverBinsType::BINS_DEFAULT
                || cbinp->binsType() == VCoverBinsType::BINS_IGNORE
                || cbinp->binsType() == VCoverBinsType::BINS_ILLEGAL)
                തുടരുക;
            // The values of constructed bins are known at construction; see emitSizedSample
            എങ്കിൽ (isConstructedBins(cbinp)) തുടരുക;
            എങ്കിൽ (isAutoBins(cbinp)) {
                // Automatic bins partition the whole domain, leaving no default value
                എങ്കിൽ (anyBinMatchp) VL_DO_DANGLING(തള്ളുക_Deletep(anyBinMatchp), anyBinMatchp);
                തിരികെ പുതിയ AstConst{fl, AstConst::BitFalse{}};
            }
            Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം binCondp = buildBinCondition(cbinp, exprp);
            UASSERT_OBJ(binCondp, cbinp,
                        "buildBinCondition returned nullptr for non-ignore/non-illegal bin");
            anyBinMatchp = anyBinMatchp ? പുതിയ AstOr{fl, anyBinMatchp, binCondp} : binCondp;
        }
        തിരികെ anyBinMatchp ? static_cast<Ast_കണ്ണി_വ്യഞ്ജകം*>(പുതിയ AstNot{fl, anyBinMatchp})
                            : static_cast<Ast_കണ്ണി_വ്യഞ്ജകം*>(പുതിയ AstConst{fl, AstConst::BitTrue{}});
    }

    //====================================================================
    // VlCoverpoint conversion

    സ്ഥിതം തർക്കം coverpointHasStateExclusions(സ്ഥിരം AstCoverpoint* coverpointp) {
        വേണ്ടി (സ്ഥിരം Ast_കണ്ണി* nodep = coverpointp->binsp(); nodep; nodep = nodep->nextp()) {
            സ്ഥിരം AstCoverBin* സ്ഥിരം binp = VN_AS(nodep, CoverBin);
            എങ്കിൽ (!binp->transp() && binp->rangesp()
                && (binp->binsType() == VCoverBinsType::BINS_IGNORE
                    || binp->binsType() == VCoverBinsType::BINS_ILLEGAL)) {
                തിരികെ സത്യം;
            }
        }
        തിരികെ അസത്യം;
    }

    സ്ഥിതം തർക്കം coverpointHasConstructedBins(സ്ഥിരം AstCoverpoint* coverpointp) {
        വേണ്ടി (സ്ഥിരം Ast_കണ്ണി* nodep = coverpointp->binsp(); nodep; nodep = nodep->nextp()) {
            എങ്കിൽ (isConstructedBins(VN_AS(nodep, CoverBin))) തിരികെ സത്യം;
        }
        തിരികെ അസത്യം;
    }

    // True if a Normal state bin, or an array-bin element, has no value of the coverpoint's
    // type (IEEE 1800-2023 19.5.7).  Array ranges enumerate in-type values, so cannot vanish.
    സ്ഥിതം തർക്കം coverpointHasEmptyBins(സ്ഥിരം AstCoverpoint* coverpointp) {
        Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം exprp = coverpointp->exprp();
        വേണ്ടി (Ast_കണ്ണി* nodep = coverpointp->binsp(); nodep; nodep = nodep->nextp()) {
            സ്ഥിരം AstCoverBin* സ്ഥിരം binp = VN_AS(nodep, CoverBin);
            എങ്കിൽ (!binp->binsType().binIsNormal() || binp->transp() || !binp->rangesp()) തുടരുക;
            // A wildcard array has a bin for each value its elements match, so none empty, and
            // the constructor creates no bin without values of a 'with' filter
            എങ്കിൽ ((binp->isArray() && binp->isWildcard()) || binWith(binp)) തുടരുക;
            തർക്കം empty = സത്യം;
            വേണ്ടി (Ast_കണ്ണി* valuep = binp->rangesp(); valuep; valuep = valuep->nextp()) {
                എങ്കിൽ (binp->isArray() && VN_IS(valuep, InsideRange)) തുടരുക;
                Cross_മൂല്യം_Range range{valuep, resolveWidth(valuep, exprp)};
                സ്ഥിരം തർക്കം none = resolve_മൂല്യം(valuep, exprp, സത്യം, binp->isWildcard(), range)
                                  && crossRangeEmpty(range);
                എങ്കിൽ (binp->isArray() && none) തിരികെ സത്യം;
                empty &= none;
            }
            എങ്കിൽ (empty && !binp->isArray()) തിരികെ സത്യം;
        }
        തിരികെ അസത്യം;
    }

    // True if a coverpoint has any transition bin.  Used to decide whether sample() emits the
    // end-of-sample previous-value update that transition matching needs.
    സ്ഥിതം തർക്കം coverpointHasTransition(AstCoverpoint* coverpointp) {
        വേണ്ടി (Ast_കണ്ണി* binp = coverpointp->binsp(); binp; binp = binp->nextp()) {
            എങ്കിൽ (VN_AS(binp, CoverBin)->transp()) തിരികെ സത്യം;
        }
        തിരികെ അസത്യം;
    }

    // The interned AstBasicDType for one of the covergroup runtime keywords.
    സ്ഥിതം AstBasicDType* basicDType(File_വരി* fl, VBasicDTypeKwd kwd) {
        തിരികെ v3Global.rootp()->typeTablep()->findBasicDType(fl, kwd);
    }

    // Get (or create) the coverpoint dtype for a hit-list bound, interned so each distinct bound
    // yields one node.
    AstCoverpointDType* coverpointDType(File_വരി* fl, uint32_t hitBound) {
        AstCoverpointDType*& typep = m_cpDTypes[hitBound];
        എങ്കിൽ (!typep) {
            typep = പുതിയ AstCoverpointDType{fl, hitBound};
            v3Global.rootp()->typeTablep()->addTypesp(typep);
        }
        തിരികെ typep;
    }

    // The key of the covergroup's type in the coverage registry, which get_coverage() queries
    std::string covergroupProtectedName() സ്ഥിരം {
        തിരികെ VIdProtect::protect_വാക്കുകൾ_If(m_covergroupName, v3Global.opt.protectIds());
    }

    // The name of the covergroup's type, which keys the coverage registry and database: as
    // $typename names it (IEEE 1800-2023 20.6.1), so apart for covergroups of distinct scopes and
    // specializations, and alike in each Verilator run of a hierarchical design.  As design units
    // of distinct libraries may share a name, one of a library other than the default is prefixed
    // by its library, as '%l' prints it (IEEE 1800-2023 33.4).
    std::string covergroupTypeName() സ്ഥിരം {
        UASSERT_OBJ(m_unitp, m_covergroupp, "Covergroup declared outside of a design unit");
        സ്ഥിരം std::string& libname = m_unitp->libname();
        സ്ഥിരം std::string name = m_covergroupp->dtypeName(സത്യം);
        തിരികെ libname == "work" ? name : libname + "." + name;
    }

    // Emit the covergroup's instance handle member and the constructor statement that creates
    // its node in the per-context coverage registry.  Runs before any coverpoint or cross is
    // generated, so their runtimes can be added to the node as they are created.
    ശൂന്യം generateInstanceAttach() {
        File_വരി* സ്ഥിരം fl = m_covergroupp->fileline();
        // V3LinkParse synthesizes a 'new' for every covergroup; the item generators below already
        // rely on that, and this attach runs even for a covergroup with no coverpoints at all.
        UASSERT_OBJ(m_constructorp, m_covergroupp, "Covergroup missing synthesized constructor");
        m_cgInstVarp = പുതിയ AstVar{fl, VVarType::MEMBER, "__Vcg_inst",
                                  basicDType(fl, VBasicDTypeKwd::COVERGROUP_INSTHANDLE)};
        m_covergroupp->addMembersp(m_cgInstVarp);

        m_constructorp->addStmtsp(
            itemCall(fl, m_cgInstVarp, VCMethod::COVERGROUP_ATTACH,
                     {ctext(fl, "vlSymsp->_vm_contextp__->covergroupRegistryp()"
                                "->newCovergroupInst("
                                    + quoted(covergroupProtectedName())
                                    + (m_cgMayMerge ? ", true" : ", false") + ")")},
                     /*usePtr=*/അസത്യം)
                ->makeStmt());
        // The node reads option.weight in place, so procedural assignments take effect
        AstCExpr* സ്ഥിരം weightAddrp = പുതിയ AstCExpr{fl, "&"};
        weightAddrp->add(newOptionSel(fl, optionVar(അസത്യം), "weight", VAccess::READ));
        m_constructorp->addStmtsp(itemCall(fl, m_cgInstVarp, VCMethod::COVERGROUP_LEND_WEIGHT,
                                           {weightAddrp, file_വരി_Debug(fl)}, /*usePtr=*/അസത്യം)
                                      ->makeStmt());
    }

    // A '__Vcg_inst.p()-><method>()' call on the covergroup's instance node
    AstCMethodHard* instanceCall(File_വരി* fl, VCMethod method) {
        // '__Vcg_inst.p()' -- a value handle, so '.' not '->'
        AstCMethodHard* സ്ഥിരം instp
            = പുതിയ AstCMethodHard{fl, memberRef(fl, m_cgInstVarp), VCMethod::COVERGROUP_INST_P};
        instp->usePtr(അസത്യം);
        instp->dtypeSetVoid();  // Opaque receiver; only ever the 'fromp' of the call below
        AstCMethodHard* സ്ഥിരം callp = പുതിയ AstCMethodHard{fl, instp, method};
        callp->usePtr(സത്യം);
        തിരികെ callp;
    }

    // Emit 'this->__Vcp_x = this->__Vcg_inst.p()->addCoverpoint<K>();' (or addCross), which
    // creates the item runtime in the instance node and borrows a pointer to it.
    AstAssign* makeItemCreate(File_വരി* fl, AstVar* itemVarp, VCMethod method) {
        AstCMethodHard* സ്ഥിരം createp = instanceCall(fl, method);
        createp->dtypep(itemVarp->dtypep());
        തിരികെ പുതിയ AstAssign{fl, memberRef(fl, itemVarp, VAccess::WRITE), createp};
    }

    // Constant bounds of one rangesp() element (an InsideRange or a single Const).  Each bound is
    // the raw AST node -- an AstConst or an AstUnbounded ('$') -- with the const/unbounded view
    // derived on demand, so there is one source of truth per bound.  After a successful
    // constRangeBounds() neither node is null; a single Const has both bounds aliasing one node.
    ഘടന RangeBounds final {
        Ast_കണ്ണി* loNodep = nullptr;  // low bound: AstConst or AstUnbounded
        Ast_കണ്ണി* hiNodep = nullptr;  // high bound: AstConst or AstUnbounded
        തർക്കം loUnbounded() സ്ഥിരം { തിരികെ VN_IS(loNodep, Unbounded); }
        തർക്കം hiUnbounded() സ്ഥിരം { തിരികെ VN_IS(hiNodep, Unbounded); }
        AstConst* loConstp() സ്ഥിരം { തിരികെ VN_CAST(loNodep, Const); }
        AstConst* hiConstp() സ്ഥിരം { തിരികെ VN_CAST(hiNodep, Const); }
    };

    // Decode one rangesp() element into its constant bounds.  Returns false if rp is neither an
    // InsideRange nor a single Const, or if a present bound is non-constant or 4-state.  '$'
    // bounds are left as AstUnbounded (not resolved) -- the caller applies its own policy.
    // Centralizes the InsideRange/Const/Unbounded decode shared by the hit-list-bound paths.
    സ്ഥിതം തർക്കം constRangeBounds(Ast_കണ്ണി* rp, RangeBounds& rb) {
        എങ്കിൽ (AstInsideRange* സ്ഥിരം irp = VN_CAST(rp, InsideRange)) {
            rb.loNodep = irp->lhsp();
            rb.hiNodep = irp->rhsp();
        } അല്ലെങ്കിൽ എങ്കിൽ (AstConst* സ്ഥിരം cp = VN_CAST(rp, Const)) {
            rb.loNodep = rb.hiNodep = cp;
        } അല്ലെങ്കിൽ {
            തിരികെ അസത്യം;
        }
        // Each bound must be a constant unless it is '$'; reject non-const and 4-state.
        AstConst* സ്ഥിരം lc = rb.loConstp();
        AstConst* സ്ഥിരം hc = rb.hiConstp();
        എങ്കിൽ ((!lc && !rb.loUnbounded()) || (!hc && !rb.hiUnbounded())) തിരികെ അസത്യം;
        എങ്കിൽ ((lc && lc->അക്കം().isFourState()) || (hc && hc->അക്കം().isFourState())) തിരികെ അസത്യം;
        തിരികെ സത്യം;
    }

    // Collect the covered value intervals of a single (non-array) Normal bin.  Returns false
    // if any range isn't a constant/open InsideRange or single Const (e.g. wildcard, non-const).
    സ്ഥിതം തർക്കം extractRangeIntervals(AstCoverBin* cbinp, uint64_t maxVal,
                                      std::vector<std::pair<uint64_t, uint64_t>>& out) {
        എങ്കിൽ (!cbinp->rangesp()) തിരികെ അസത്യം;
        വേണ്ടി (Ast_കണ്ണി* rp = cbinp->rangesp(); rp; rp = rp->nextp()) {
            RangeBounds rb;
            എങ്കിൽ (!constRangeBounds(rp, rb)) തിരികെ അസത്യം;
            എങ്കിൽ ((rb.loConstp() && rb.loConstp()->width() > 64)
                || (rb.hiConstp() && rb.hiConstp()->width() > 64)) {
                തിരികെ അസത്യം;  // Use the safe slot-count bound for wide values.
            }
            സ്ഥിരം uint64_t lo = rb.loUnbounded() ? 0 : rb.loConstp()->toUQuad();
            സ്ഥിരം uint64_t hi = rb.hiUnbounded() ? maxVal : rb.hiConstp()->toUQuad();
            എങ്കിൽ (lo > hi) തിരികെ അസത്യം;
            out.emplace_back(lo, hi);
        }
        തിരികെ സത്യം;
    }

    // Append one Normal bin's cross-slot interval-sets to `bins` and bump `slotCount` by an upper
    // bound of the bin's slots holding one value.  Returns false if any part isn't statically
    // enumerable (the caller then falls back to the always-safe slot count).  A non-array bin
    // is one slot covering the union of its intervals; the bins of an array element or of an
    // automatic bins declaration hold disjoint values, so the element or declaration counts once.
    തർക്കം appendBinCrossSlots(AstCoverBin* cbinp, uint64_t maxVal, Ast_കണ്ണി_വ്യഞ്ജകം* exprp,
                             std::vector<std::vector<std::pair<uint64_t, uint64_t>>>& bins,
                             പൂർണ്ണസംഖ്യ& slot_എണ്ണം) {
        എങ്കിൽ (isAutoBins(cbinp)) {
            ++slot_എണ്ണം;
            bins.തള്ളുക_back({{0, maxVal}});
            തിരികെ exprp->width() <= 64;
        }
        എങ്കിൽ (binWith(cbinp)) {
            // A value is in one bin of a filter's, but of a sized array, in one for each range
            // list element holding it; the coverpoint's name is one element
            പൂർണ്ണസംഖ്യ elements = 0;
            വേണ്ടി (സ്ഥിരം Ast_കണ്ണി* rp = binRangesp(cbinp); rp && isSizedArray(cbinp);
                 rp = rp->nextp()) {
                ++elements;
            }
            slot_എണ്ണം += std::max(1, elements);
            തിരികെ അസത്യം;
        }
        എങ്കിൽ (cbinp->isArray() && cbinp->isWildcard() && !cbinp->arraySizep()) {
            // A value is in at most one bin of a wildcard array: one slot covering its values.
            // Signed values are sign-extended, not unsigned intervals (see computeHitListBound)
            ++slot_എണ്ണം;
            എങ്കിൽ (exprp->isSigned() || exprp->width() > 64) തിരികെ അസത്യം;
            സ്ഥിരം BinRuns runs = wildcardBinRuns(cbinp, exprp, അസത്യം);
            എങ്കിൽ (runs.unsupported) തിരികെ അസത്യം;
            std::vector<std::pair<uint64_t, uint64_t>> ivs;
            വേണ്ടി (സ്ഥിരം BinRun& run : runs.runs) {
                ivs.emplace_back(run.m_lo.toUQuad(), run.m_hi.toUQuad());
            }
            bins.തള്ളുക_back(std::move(ivs));
            തിരികെ സത്യം;
        }
        എങ്കിൽ (cbinp->isArray()) തിരികെ appendArrayBinCrossSlots(cbinp, exprp, bins, slot_എണ്ണം);
        // Non-array bin: one slot covering the union of its intervals.
        ++slot_എണ്ണം;
        std::vector<std::pair<uint64_t, uint64_t>> ivs;
        എങ്കിൽ (cbinp->isWildcard() || !extractRangeIntervals(cbinp, maxVal, ivs)) തിരികെ അസത്യം;
        bins.തള്ളുക_back(std::move(ivs));
        തിരികെ സത്യം;
    }

    // Append the cross slots of an array Normal bin: each element is a slot covering its
    // values, as at most one of its single-value bins holds a value.  An element holds the
    // values arrayBinRuns() gives it: those of the coverpoint type (IEEE 1800-2023 19.5.7).
    // Elements of a signed coverpoint, non-constant elements, and elements with values beyond
    // 64 bits can't be enumerated, so they count one slot but lose exactness.  Returns false if
    // any element wasn't enumerable to exact values.
    തർക്കം appendArrayBinCrossSlots(AstCoverBin* cbinp, Ast_കണ്ണി_വ്യഞ്ജകം* exprp,
                                  std::vector<std::vector<std::pair<uint64_t, uint64_t>>>& bins,
                                  പൂർണ്ണസംഖ്യ& slot_എണ്ണം) {
        // Signed values resolve sign-extended, not as unsigned intervals, and wildcard patterns
        // are not intervals
        തർക്കം exact = !exprp->isSigned() && !cbinp->isWildcard();
        വേണ്ടി (Ast_കണ്ണി* rp = cbinp->rangesp(); rp; rp = rp->nextp()) {
            ++slot_എണ്ണം;
            RangeBounds rb;
            Cross_മൂല്യം_Range range{rp, resolveWidth(rp, exprp)};
            എങ്കിൽ (!exact || !constRangeBounds(rp, rb)
                || !resolve_മൂല്യം(rp, exprp, സത്യം, അസത്യം, range)) {
                exact = അസത്യം;
                തുടരുക;
            }
            എങ്കിൽ (crossRangeEmpty(range)) തുടരുക;  // Its bin, if any, holds no value
            എങ്കിൽ (range.hi.mostSetBitP1() > 64) {
                exact = അസത്യം;
                തുടരുക;
            }
            bins.തള്ളുക_back({{range.lo.toUQuad(), range.hi.toUQuad()}});
        }
        തിരികെ exact;
    }

    // Compute the hit-list bound for a coverpoint: the maximum number of Normal
    // bins one sample value can match.  Non-cross-fed coverpoints don't feed a cross, so
    // their hit list is unused -> 1.  Otherwise compute the exact max bin overlap; fall back
    // to the (always-safe) Normal-slot count when any bin isn't statically analyzable.
    പൂർണ്ണസംഖ്യ computeHitListBound(AstCoverpoint* coverpointp, Ast_കണ്ണി_വ്യഞ്ജകം* exprp, തർക്കം crossFed) {
        എങ്കിൽ (!crossFed) തിരികെ 1;
        സ്ഥിരം പൂർണ്ണസംഖ്യ width = exprp->width();
        സ്ഥിരം uint64_t maxVal = (width >= 64) ? UINT64_MAX : ((1ULL << width) - 1);
        // One entry per Normal bin (cross slot): its covered intervals.
        std::vector<std::vector<std::pair<uint64_t, uint64_t>>> bins;
        പൂർണ്ണസംഖ്യ slot_എണ്ണം = 0;  // == runtime m_normal; the safe fallback bound
        // Unsigned intervals cannot establish overlap between differently sized signed values.
        തർക്കം exact = !exprp->isSigned();
        വേണ്ടി (Ast_കണ്ണി* binp = coverpointp->binsp(); binp; binp = binp->nextp()) {
            AstCoverBin* സ്ഥിരം cbinp = VN_AS(binp, CoverBin);
            എങ്കിൽ (!cbinp->binsType().binIsNormal())
                തുടരുക;  // ignore/illegal/default: not hit-listed
            എങ്കിൽ (!appendBinCrossSlots(cbinp, maxVal, exprp, bins, slot_എണ്ണം)) exact = അസത്യം;
        }
        എങ്കിൽ (!exact) തിരികെ std::max(1, slot_എണ്ണം);
        എങ്കിൽ (bins.empty()) തിരികെ 1;
        // Max overlap occurs at some interval start; count covering bins at each lo.
        std::vector<uint64_t> pts;
        വേണ്ടി (സ്ഥിരം auto& b : bins)
            വേണ്ടി (സ്ഥിരം auto& iv : b) pts.തള്ളുക_back(iv.first);
        std::sort(pts.begin(), pts.end());
        pts.erase(std::unique(pts.begin(), pts.end()), pts.end());
        പൂർണ്ണസംഖ്യ maxOverlap = 1;
        വേണ്ടി (സ്ഥിരം uint64_t p : pts) {
            പൂർണ്ണസംഖ്യ cnt = 0;
            വേണ്ടി (സ്ഥിരം auto& b : bins) {
                വേണ്ടി (സ്ഥിരം auto& iv : b) {
                    എങ്കിൽ (iv.first <= p && p <= iv.second) {
                        ++cnt;
                        നിർത്തുക;  // count each bin at most once
                    }
                }
            }
            എങ്കിൽ (cnt > maxOverlap) maxOverlap = cnt;
        }
        തിരികെ maxOverlap;
    }

    // A 'this->m_member' reference for embedding in an AstCStmt
    AstVarRef* memberRef(File_വരി* fl, AstVar* varp, VAccess access = VAccess::READ) {
        AstVarRef* സ്ഥിരം refp = പുതിയ AstVarRef{fl, varp, access};
        refp->selfPointer(VSelfPointerText{VSelfPointerText::This{}});
        തിരികെ refp;
    }

    // A 'this->m_member-><method>(args...)' call on one member.  usePtr is false only for
    // __Vcg_inst, which is a value handle; the item members are borrowed pointers into the
    // instance node.  Numeric arguments are AstConst; the rest are C++ text that has no AST
    // form (see ctext).
    AstCMethodHard* itemCall(File_വരി* fl, AstVar* varp, VCMethod method,
                             സ്ഥിരം std::vector<Ast_കണ്ണി_വ്യഞ്ജകം*>& args = {}, തർക്കം usePtr = സത്യം) {
        AstCMethodHard* സ്ഥിരം callp = പുതിയ AstCMethodHard{fl, memberRef(fl, varp), method};
        വേണ്ടി (Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം argp : args) callp->addPinsp(argp);
        callp->usePtr(usePtr);
        callp->dtypeSetVoid();
        തിരികെ callp;
    }

    // An unsigned integer argument.
    സ്ഥിതം AstConst* cnum(File_വരി* fl, uint32_t മൂല്യം) { തിരികെ പുതിയ AstConst{fl, മൂല്യം}; }

    // A literal C++ argument with no AST equivalent: a 'const char*' string literal (an SV
    // string AstConst emits '"..."s', a std::string temporary the runtime cannot borrow), a
    // VlCovBinKind enum token, a constant selection-word initializer list, a VlFileLineDebug, or
    // a '__V' temporary declared by the enclosing AstCStmt.
    സ്ഥിതം AstCExpr* ctext(File_വരി* fl, സ്ഥിരം std::string& text) {
        തിരികെ പുതിയ AstCExpr{fl, text};
    }

    // A C++ string literal.  Escapes control characters as the emitter does elsewhere -- bin
    // names and filenames reach the generated code verbatim when --protect-ids is off, and an
    // SV escaped identifier may hold a quote or backslash.
    സ്ഥിതം std::string quoted(സ്ഥിരം std::string& text) {
        തിരികെ "\"" + V3OutFormatter::quoteNameControls(text) + "\"";
    }

    // A 'VlFileLineDebug' argument: where the runtime reports an error about fl's construct
    സ്ഥിതം AstCExpr* file_വരി_Debug(File_വരി* fl) {
        സ്ഥിരം std::string filename
            = VIdProtect::protectIf(fl->filename(), v3Global.opt.protectIds());
        തിരികെ ctext(fl, "VlFileLineDebug{" + quoted(filename) + ", "
                             + std::to_string(fl->lineno()) + "}");
    }

    // Check that an element of an array bin (bins b[] = {values/ranges}) is a two-state
    // constant value or range; false if not, after reporting it if 'report'.
    സ്ഥിതം തർക്കം checkArrayBinElement(AstCoverBin* arrayBinp, Ast_കണ്ണി* rangep, തർക്കം report = സത്യം) {
        എങ്കിൽ (സ്ഥിരം AstInsideRange* സ്ഥിരം irp = VN_CAST(rangep, InsideRange)) {
            സ്ഥിരം AstConst* സ്ഥിരം minp = VN_CAST(irp->lhsp(), Const);
            സ്ഥിരം AstConst* സ്ഥിരം maxp = VN_CAST(irp->rhsp(), Const);
            എങ്കിൽ ((!minp && !VN_IS(irp->lhsp(), Unbounded))
                || (!maxp && !VN_IS(irp->rhsp(), Unbounded))) {
                എങ്കിൽ (report) {
                    arrayBinp->v3error("Non-constant expression in array bins range; "
                                       "range bounds must be constants (IEEE 1800-2023 19.5)");
                }
                തിരികെ അസത്യം;
            }
            എങ്കിൽ ((minp && minp->അക്കം().isFourState()) || (maxp && maxp->അക്കം().isFourState())) {
                എങ്കിൽ (report) {
                    arrayBinp->v3error("Four-state (x/z) value in array bins range bound; "
                                       "range bounds must be two-state constants");
                }
                തിരികെ അസത്യം;
            }
        } അല്ലെങ്കിൽ എങ്കിൽ (!VN_IS(rangep, Const)) {
            എങ്കിൽ (report) {
                arrayBinp->v3error("Non-constant expression in array bins value list; "
                                   "values must be constants (IEEE 1800-2023 19.5)");
            }
            തിരികെ അസത്യം;
        }
        തിരികെ സത്യം;
    }

    // Individual equality targets of an array bin (bins b[] = {values/ranges}) of a real
    // coverpoint, in order; integral coverpoints generate array bins as runs (see arrayBinRuns).
    // An open-ended bound ('$', AstUnbounded) resolves to the coverpoint domain: '[lo:$]'
    // covers [lo:maxVal] and '[$:hi]' covers [0:hi].  One target is produced per value; ranges
    // whose resolved size would exceed --coverage-max-real-bins (e.g. an open '[lo:$]') are
    // unsupported -- emits COVERIGN, sets unsupportedOut, yields nothing.
    std::vector<Ast_കണ്ണി_വ്യഞ്ജകം*> extract_Array_മൂല്യങ്ങൾ(AstCoverBin* arrayBinp, Ast_കണ്ണി_വ്യഞ്ജകം* exprp,
                                                 തർക്കം& unsupportedOut) {
        unsupportedOut = അസത്യം;
        സ്ഥിരം പൂർണ്ണസംഖ്യ width = exprp->width();
        സ്ഥിരം uint64_t maxVal = (width >= 64) ? UINT64_MAX : ((1ULL << width) - 1);
        std::vector<Ast_കണ്ണി_വ്യഞ്ജകം*> മൂല്യങ്ങൾ;
        വേണ്ടി (Ast_കണ്ണി* rangep = arrayBinp->rangesp(); rangep; rangep = rangep->nextp()) {
            rangep = V3Const::constifyEdit(rangep);
            എങ്കിൽ (!checkArrayBinElement(arrayBinp, rangep)) തിരികെ മൂല്യങ്ങൾ;
            എങ്കിൽ (AstInsideRange* സ്ഥിരം irp = VN_CAST(rangep, InsideRange)) {
                സ്ഥിരം തർക്കം loUnb = VN_IS(irp->lhsp(), Unbounded);
                സ്ഥിരം തർക്കം hiUnb = VN_IS(irp->rhsp(), Unbounded);
                സ്ഥിരം uint64_t lo = loUnb ? 0 : VN_AS(irp->lhsp(), Const)->toUQuad();
                സ്ഥിരം uint64_t hi = hiUnb ? maxVal : VN_AS(irp->rhsp(), Const)->toUQuad();
                എങ്കിൽ (hi < lo) തുടരുക;  // empty range contributes no bins
                // Guard against a '$'-bounded or otherwise huge range exploding the bin count.
                സ്ഥിരം uint64_t span = hi - lo;  // == valueCount - 1 (no overflow: hi >= lo)
                എങ്കിൽ (span >= v3Global.opt.coverageMaxRealBins()
                    || മൂല്യങ്ങൾ.size() + span + 1 > v3Global.opt.coverageMaxRealBins()) {
                    arrayBinp->v3warn(COVERIGN,
                                      "Unsupported: array 'bins' of a real coverpoint "
                                      "covering more than "
                                          << v3Global.opt.coverageMaxRealBins() << " values; bin "
                                          << arrayBinp->prettyNameQ() << " ignored.\n"
                                          << arrayBinp->warnMore()
                                          << "... Suggest a larger --coverage-max-real-bins");
                    unsupportedOut = സത്യം;
                    വേണ്ടി (Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം vp : മൂല്യങ്ങൾ) VL_DO_DANGLING(തള്ളുക_Deletep(vp), vp);
                    മൂല്യങ്ങൾ.clear();
                    തിരികെ മൂല്യങ്ങൾ;
                }
                വേണ്ടി (uint64_t v = lo; v <= hi; ++v)
                    മൂല്യങ്ങൾ.തള്ളുക_back(പുതിയ AstConst{irp->fileline(), AstConst::Widthed_മൂല്യം{}, width,
                                                  static_cast<uint32_t>(v)});
            } അല്ലെങ്കിൽ {
                മൂല്യങ്ങൾ.തള്ളുക_back(VN_AS(rangep->clone_വൃക്ഷം(അസത്യം), കണ്ണി_വ്യഞ്ജകം));
            }
        }
        തിരികെ മൂല്യങ്ങൾ;
    }

    സ്ഥിതം പൂർണ്ണസംഖ്യ runWidth(സ്ഥിരം Ast_കണ്ണി_വ്യഞ്ജകം* exprp) { തിരികെ exprp->width() + 1; }

    // Automatic bins partition the coverpoint domain in value order (IEEE 1800-2023 19.5.3): N
    // bins, capped at the number of values, each hold 2^width / N values, and the last bin also
    // holds the remainder.  False for an invalid declaration, already reported.
    തർക്കം autoBinRuns(AstCoverBin* binp, Ast_കണ്ണി_വ്യഞ്ജകം* exprp, BinRuns& out) {
        സ്ഥിരം uint32_t requested = autoBinsRequested(binp);
        എങ്കിൽ (!requested || !exprp->dtypep()->skipRefp()->isIntegralOrPacked()) തിരികെ അസത്യം;
        സ്ഥിരം പൂർണ്ണസംഖ്യ width = exprp->width();
        സ്ഥിരം പൂർണ്ണസംഖ്യ arithmeticWidth = runWidth(exprp);
        സ്ഥിരം uint32_t എണ്ണം
            = width < 32
                  ? static_cast<uint32_t>(std::min<uint64_t>(uint64_t{1} << width, requested))
                  : requested;
        BinRun run{binp, arithmeticWidth, എണ്ണം};
        V3_സംഖ്യ total{binp, arithmeticWidth};
        total.setBit(width, 1);
        run.m_stride.opDiv(total, V3_സംഖ്യ{binp, arithmeticWidth, എണ്ണം});
        സ്ഥിരം Cross_മൂല്യം_Range domain
            = cross_മൂല്യം_Domain(binp, width, exprp->isSigned(), arithmeticWidth);
        run.m_lo = domain.lo;
        run.m_hi = domain.hi;
        out.runs.തള്ളുക_back(std::move(run));
        out.എണ്ണം = എണ്ണം;
        തിരികെ സത്യം;
    }

    // The elements of an array bin (bins b[] = {values/ranges}), in order, as runs of
    // single-value bins.  A range holds the values of the coverpoint type it contains (IEEE
    // 1800-2023 19.5.7), while a singleton names a bin even without such a value.  Errors on a
    // non-constant element.  More than --coverage-max-bins bins (e.g. an open '[lo:$]' range over
    // a wide coverpoint) are unsupported -- emits COVERIGN, and sets unsupported.
    BinRuns arrayBinRuns(AstCoverBin* arrayBinp, Ast_കണ്ണി_വ്യഞ്ജകം* exprp) {
        BinRuns out;
        സ്ഥിരം പൂർണ്ണസംഖ്യ width = runWidth(exprp);
        വേണ്ടി (Ast_കണ്ണി* rangep = arrayBinp->rangesp(); rangep; rangep = rangep->nextp()) {
            rangep = V3Const::constifyEdit(rangep);
            എങ്കിൽ (!checkArrayBinElement(arrayBinp, rangep)) തിരികെ out;
            സ്ഥിരം AstInsideRange* സ്ഥിരം irp = VN_CAST(rangep, InsideRange);
            Cross_മൂല്യം_Range range{rangep, resolveWidth(rangep, exprp)};
            തർക്കം empty = സത്യം;
            എങ്കിൽ (!resolve_മൂല്യം(rangep, exprp, സത്യം, അസത്യം, range)) {
                rangep->v3warn(E_UNSUPPORTED, "Unsupported: non-integral value in a coverage bin "
                                              "of an integral coverpoint.");
            } അല്ലെങ്കിൽ {
                empty = crossRangeEmpty(range);
            }
            എങ്കിൽ (empty && irp) തുടരുക;  // A range without values contributes no bins
            BinRun run{rangep, width, 1};
            run.m_empty = empty;
            uint64_t എണ്ണം = 1;
            എങ്കിൽ (!empty) {
                run.m_lo.opAssign(range.lo);
                run.m_hi.opAssign(range.hi);
                V3_സംഖ്യ span{rangep, width};
                span.opSub(run.m_hi, run.m_lo);
                // Wider spans exceed any limit
                എണ്ണം = span.mostSetBitP1() > 32 ? UINT64_MAX : span.toUQuad() + 1;
            }
            എങ്കിൽ (എണ്ണം > v3Global.opt.coverageMaxBins() - out.എണ്ണം) {
                arrayBinp->v3warn(COVERIGN, "Unsupported: array 'bins' covering more than "
                                                << v3Global.opt.coverageMaxBins()
                                                << " values (e.g. an open '[lo:$]' range over "
                                                   "a wide coverpoint); bin "
                                                << arrayBinp->prettyNameQ() << " ignored\n"
                                                << arrayBinp->warnMore()
                                                << "... Suggest a larger --coverage-max-bins");
                out.runs.clear();
                out.എണ്ണം = 0;
                out.unsupported = സത്യം;
                തിരികെ out;
            }
            run.m_എണ്ണം = static_cast<uint32_t>(എണ്ണം);
            out.എണ്ണം += run.m_എണ്ണം;
            out.runs.തള്ളുക_back(std::move(run));
        }
        തിരികെ out;
    }

    // The bins of a wildcard array (wildcard bins b[] = {...}): one for each coverpoint value
    // an element matches (IEEE 1800-2023 19.5.4, 19.5.7), in value order, and named by the value
    // (19.5.1), as runs of single-value bins.  Errors on a non-constant element.  More than
    // --coverage-max-bins values are unsupported -- emits COVERIGN, and sets unsupported, and for
    // an ignore or illegal array, single.  'report' false omits these diagnostics.
    സ്ഥിതം BinRuns wildcardBinRuns(AstCoverBin* arrayBinp, Ast_കണ്ണി_വ്യഞ്ജകം* exprp, തർക്കം report) {
        BinRuns out;
        സ്ഥിരം പൂർണ്ണസംഖ്യ width = runWidth(exprp);
        സ്ഥിരം V3_സംഖ്യ one{arrayBinp, width, 1};
        // Disjoint runs of the values, in value order, each value once
        std::vector<std::pair<V3_സംഖ്യ, V3_സംഖ്യ>> spans;
        uint64_t എണ്ണം = 0;  // Values of 'spans'
        വേണ്ടി (Ast_കണ്ണി* rangep = arrayBinp->rangesp(); rangep; rangep = rangep->nextp()) {
            rangep = V3Const::constifyEdit(rangep);
            എങ്കിൽ (!checkArrayBinElement(arrayBinp, rangep, report)) {
                out.unsupported = സത്യം;
                തിരികെ out;
            }
            Cross_മൂല്യം_Range range{rangep, resolveWidth(rangep, exprp)};
            എങ്കിൽ (!resolve_മൂല്യം(rangep, exprp, സത്യം, സത്യം, range)) {
                എങ്കിൽ (report) {
                    rangep->v3warn(E_UNSUPPORTED, "Unsupported: non-integral value in a "
                                                  "coverage bin of an integral coverpoint.");
                }
                തുടരുക;
            }
            എങ്കിൽ (crossRangeEmpty(range)) തുടരുക;
            std::vector<std::pair<V3_സംഖ്യ, V3_സംഖ്യ>> found;
            crossRangeRuns(range, v3Global.opt.coverageMaxBins(), found);
            വേണ്ടി (സ്ഥിരം std::pair<V3_സംഖ്യ, V3_സംഖ്യ>& run : found) {
                // Coverpoint values, sign-extended in both widths
                spans.emplace_back(V3_സംഖ്യ{rangep, width, run.first},
                                   V3_സംഖ്യ{rangep, width, run.second});
            }
            std::sort(spans.begin(), spans.end(), [](സ്ഥിരം auto& lhs, സ്ഥിരം auto& rhs) {
                തിരികെ cross_മൂല്യം_Less(lhs.first, rhs.first);
            });
            std::vector<std::pair<V3_സംഖ്യ, V3_സംഖ്യ>> merged;
            വേണ്ടി (സ്ഥിരം std::pair<V3_സംഖ്യ, V3_സംഖ്യ>& span : spans) {
                // Adjacent or overlapping values join a run; 'first - 1' cannot overflow
                V3_സംഖ്യ before{rangep, width};
                before.opSub(span.first, one);
                എങ്കിൽ (merged.empty() || cross_മൂല്യം_Less(merged.back().second, before)) {
                    merged.തള്ളുക_back(span);
                } അല്ലെങ്കിൽ എങ്കിൽ (cross_മൂല്യം_Less(merged.back().second, span.second)) {
                    merged.back().second = span.second;
                }
            }
            എണ്ണം = 0;
            വേണ്ടി (സ്ഥിരം std::pair<V3_സംഖ്യ, V3_സംഖ്യ>& span : merged) {
                V3_സംഖ്യ size{rangep, width};
                size.opSub(span.second, span.first);
                // Beyond 2^32 values exceed any limit
                എണ്ണം += size.mostSetBitP1() > 32 ? uint64_t{1} << 33 : size.toUQuad() + 1;
                എങ്കിൽ (എണ്ണം > v3Global.opt.coverageMaxBins()) നിർത്തുക;
            }
            spans = std::move(merged);
            എങ്കിൽ (എണ്ണം > v3Global.opt.coverageMaxBins()) {
                // An ignore or illegal array still excludes or checks its values, as one bin
                out.single = !arrayBinp->binsType().binIsNormal();
                എങ്കിൽ (report) {
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
                out.unsupported = സത്യം;
                തിരികെ out;
            }
        }
        വേണ്ടി (സ്ഥിരം std::pair<V3_സംഖ്യ, V3_സംഖ്യ>& span : spans) {
            out.runs.emplace_back(arrayBinp, width, 0);
            BinRun& run = out.runs.back();
            run.m_lo = span.first;
            run.m_hi = span.second;
            // A bin for each value, named by it in the coverpoint's type
            V3_സംഖ്യ മൂല്യം = span.first;
            ആകുമ്പോൾ (സത്യം) {
                സ്ഥിരം V3_സംഖ്യ typed{arrayBinp, exprp->width(), മൂല്യം};
                out.മൂല്യങ്ങൾ.തള്ളുക_back(exprp->isSigned() ? typed.toDecimalS() : typed.toDecimalU());
                ++run.m_എണ്ണം;
                എങ്കിൽ (മൂല്യം.isCaseEq(span.second)) നിർത്തുക;
                V3_സംഖ്യ next{arrayBinp, width};
                മൂല്യം = next.opAdd(മൂല്യം, one);
            }
        }
        out.എണ്ണം = static_cast<uint32_t>(എണ്ണം);
        തിരികെ out;
    }

    // The runs of an automatic bins declaration, or of an array bin of an integral coverpoint.
    // False for other bins, which do not generate as runs, including a wildcard array then one
    // bin (see BinRuns::single).
    തർക്കം binRunsFor(AstCoverBin* binp, Ast_കണ്ണി_വ്യഞ്ജകം* exprp, BinRuns& out) {
        എങ്കിൽ (isAutoBins(binp)) {
            എങ്കിൽ (!autoBinRuns(binp, exprp, out)) out.unsupported = സത്യം;
            തിരികെ സത്യം;
        }
        എങ്കിൽ (binp->isArray() && binp->isWildcard()) {
            എങ്കിൽ (!exprp->dtypep()->skipRefp()->isIntegralOrPacked()) {
                Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം falsep = wildcardTypeError(binp, exprp);
                VL_DO_DANGLING(തള്ളുക_Deletep(falsep), falsep);
                out.unsupported = സത്യം;
            } അല്ലെങ്കിൽ {
                out = wildcardBinRuns(binp, exprp, സത്യം);
                എങ്കിൽ (out.single) {
                    binp->isArray(അസത്യം);  // Generates as one bin
                    തിരികെ അസത്യം;
                }
            }
            തിരികെ സത്യം;
        }
        എങ്കിൽ (!binp->isArray() || binp->transp() || binp->isWildcard()
            || !exprp->dtypep()->skipRefp()->isIntegralOrPacked()) {
            തിരികെ അസത്യം;
        }
        out = arrayBinRuns(binp, exprp);
        തിരികെ സത്യം;
    }

    // Emit a 'this->m_cp->addSingleNamer/addArrayNamer(...)' statement for one bin whose first
    // runtime bin index is 'declared'; or with 'valueNames', those naming each bin of an array
    Ast_കണ്ണി_Stmt* makeNamer(AstVar* cpVarp, AstCoverBin* binp, int64_t എണ്ണം, uint32_t declared,
                           സ്ഥിരം std::vector<Ast_കണ്ണി_വ്യഞ്ജകം*>& മൂല്യങ്ങൾ = {},
                           സ്ഥിരം std::vector<std::string>& മൂല്യം_Names = {}) {
        File_വരി* സ്ഥിരം fl = binp->fileline();
        CoverpointBins& bins = m_cpBins.at(cpVarp);
        സ്ഥിരം uint32_t normal_എണ്ണം
            = binp->binsType().binIsNormal() ? static_cast<uint32_t>(എണ്ണം < 0 ? 1 : എണ്ണം) : 0;
        സ്ഥിരം BinSpan span{bins.total, normal_എണ്ണം, declared};
        എങ്കിൽ (binp->binsType() == VCoverBinsType::BINS_AUTO_IMPLICIT) {
            bins.implicitAuto = span;  // Selected by bin, see implicitAutoBinSpan
        } അല്ലെങ്കിൽ {
            bins.spans.emplace(binp->name(), span);
        }
        bins.total += normal_എണ്ണം;
        വേണ്ടി (uint32_t i = 0; bins.crossed && i < normal_എണ്ണം; ++i) {
            bins.മൂല്യങ്ങൾ.തള്ളുക_back({binp, മൂല്യങ്ങൾ.empty() ? nullptr : മൂല്യങ്ങൾ[i]});
        }
        // Under --protect-ids the filename and bin name flow into the coverage database
        // verbatim, so obfuscate them exactly as line/toggle coverage points are (whole-
        // unit filename, per-word bin name).  A no-op when --protect-ids is off.
        സ്ഥിരം തർക്കം prot = v3Global.opt.protectIds();
        സ്ഥിരം തർക്കം single = എണ്ണം < 0;
        എങ്കിൽ (!മൂല്യം_Names.empty()) {
            // A bin of a wildcard array is named by its value (IEEE 1800-2023 19.5.1)
            Ast_കണ്ണി_Stmt* stmtsp = nullptr;
            വേണ്ടി (സ്ഥിരം std::string& മൂല്യം : മൂല്യം_Names) {
                സ്ഥിരം std::string name
                    = VIdProtect::protect_വാക്കുകൾ_If(binp->name(), prot) + "[" + മൂല്യം + "]";
                stmtsp = Ast_കണ്ണി::addNext(
                    stmtsp,
                    itemCall(fl, cpVarp, VCMethod::COVERGROUP_ADD_SINGLE_NAMER,
                             {ctext(fl, binp->binsType().binSetEnum()), ctext(fl, quoted(name)),
                              ctext(fl, quoted(VIdProtect::protectIf(fl->filename(), prot))),
                              cnum(fl, static_cast<uint32_t>(fl->lineno())),
                              cnum(fl, static_cast<uint32_t>(fl->firstColumn()))})
                        ->makeStmt());
            }
            തിരികെ stmtsp;
        }
        std::vector<Ast_കണ്ണി_വ്യഞ്ജകം*> args{ctext(fl, binp->binsType().binSetEnum())};
        എങ്കിൽ (!single) args.തള്ളുക_back(cnum(fl, static_cast<uint32_t>(എണ്ണം)));  // value array bin
        args.തള്ളുക_back(ctext(fl, quoted(VIdProtect::protect_വാക്കുകൾ_If(binp->name(), prot))));
        args.തള്ളുക_back(ctext(fl, quoted(VIdProtect::protectIf(fl->filename(), prot))));
        args.തള്ളുക_back(cnum(fl, static_cast<uint32_t>(fl->lineno())));
        args.തള്ളുക_back(cnum(fl, static_cast<uint32_t>(fl->firstColumn())));
        തിരികെ itemCall(fl, cpVarp,
                        single ? VCMethod::COVERGROUP_ADD_SINGLE_NAMER
                        : binp->binsType() == VCoverBinsType::BINS_AUTO_IMPLICIT
                            ? VCMethod::COVERGROUP_ADD_NUMBERED_NAMER
                            : VCMethod::COVERGROUP_ADD_ARRAY_NAMER,
                        args)
            ->makeStmt();
    }

    // Emit 'if (iff && cond) m_cp.incrementBin(idx);' (or recordHit, + illegal action) in sample()
    // Where a bin's hit is recorded in the runtime VlCoverpoint member.
    ഘടന ConvBinTarget final {
        AstVar* cpVarp;  // the __Vcp_<coverpoint> member
        uint32_t idx;  // bin index within that coverpoint
        തർക്കം isNormal;  // Normal -> incrementBin (count + cross hit list); else recordHit (count)
    };

    // Emit 'this->m_cp.incrementBin(idx);' (Normal) or '.recordHit(idx);'
    // (ignore/illegal/default).
    Ast_കണ്ണി_Stmt* makeRuntimeBinHit(File_വരി* fl, AstVar* cpVarp, Ast_കണ്ണി_വ്യഞ്ജകം* idxp,
                                   തർക്കം isNormal) {
        തിരികെ itemCall(fl, cpVarp,
                        isNormal ? VCMethod::COVERGROUP_INCREMENT_BIN
                                 : VCMethod::COVERGROUP_RECORD_HIT,
                        {idxp})
            ->makeStmt();
    }
    Ast_കണ്ണി_Stmt* makeRuntimeBinHit(File_വരി* fl, സ്ഥിരം ConvBinTarget& tgt) {
        തിരികെ makeRuntimeBinHit(fl, tgt.cpVarp, cnum(fl, static_cast<uint32_t>(tgt.idx)),
                                 tgt.isNormal);
    }

    // The condition under which a bin counts a sample: condp, and the bin's iff, and for a
    // Normal or default state bin, that the value is not excluded
    Ast_കണ്ണി_വ്യഞ്ജകം* binCondition(AstCoverBin* binp, AstVar* cpVarp, Ast_കണ്ണി_വ്യഞ്ജകം* condp) {
        File_വരി* സ്ഥിരം fl = binp->fileline();
        എങ്കിൽ (binp->iffp()) condp = പുതിയ AstLogAnd{fl, binp->iffp()->clone_വൃക്ഷം(അസത്യം), condp};
        സ്ഥിരം auto excluded = m_excludedVars.find(cpVarp);
        എങ്കിൽ (excluded != m_excludedVars.end() && !binp->transp()
            && (binp->binsType().binIsNormal()
                || binp->binsType() == VCoverBinsType::BINS_DEFAULT)) {
            condp = പുതിയ AstLogAnd{
                fl, പുതിയ AstNot{fl, പുതിയ AstVarRef{fl, excluded->second, VAccess::READ}}, condp};
        }
        തിരികെ condp;
    }

    ശൂന്യം emitConvHitIf(AstCoverpoint* coverpointp, AstCoverBin* binp, AstVar* cpVarp,
                       Ast_കണ്ണി_വ്യഞ്ജകം* idxp, Ast_കണ്ണി_വ്യഞ്ജകം* condp) {
        File_വരി* സ്ഥിരം fl = binp->fileline();
        Ast_കണ്ണി* actionp = makeRuntimeBinHit(fl, cpVarp, idxp, binp->binsType().binIsNormal());
        എങ്കിൽ (binp->binsType() == VCoverBinsType::BINS_ILLEGAL) {
            actionp->addNext(makeIllegalBinAction(fl, "Illegal bin " + binp->prettyNameQ()
                                                          + " hit in coverpoint "
                                                          + coverpointp->prettyNameQ()));
        }
        UASSERT_OBJ(m_sampleFuncp, binp, "sample() CFunc not set for coverpoint");
        addSampleStmt(പുതിയ AstIf{fl, binCondition(binp, cpVarp, condp), actionp, nullptr});
    }

    // Emit the sample() code of sized array 'sized': count the value in its bins holding it when
    // enabled, as other bins are, and for default bins note in matchedp whether any holds it
    ശൂന്യം emitSizedSample(AstCoverpoint* coverpointp, AstCoverBin* binp, AstVar* cpVarp,
                         Ast_കണ്ണി_വ്യഞ്ജകം* exprp, uint32_t sized, AstVar* matchedp) {
        File_വരി* സ്ഥിരം fl = binp->fileline();
        Ast_കണ്ണി_വ്യഞ്ജകം* enabledp = binCondition(binp, cpVarp, പുതിയ AstConst{fl, AstConst::BitTrue{}});
        UASSERT_OBJ(m_sampleFuncp, binp, "sample() CFunc not set for coverpoint");
        സ്ഥിരം തർക്കം illegal = binp->binsType() == VCoverBinsType::BINS_ILLEGAL;
        എങ്കിൽ (illegal) {
            // The illegal action reads the condition too, which is evaluated once, as the guard
            // may have side effects
            AstVar* സ്ഥിരം varp
                = പുതിയ AstVar{fl, VVarType::BLOCKTEMP,
                             "__VcpEnabled_" + sanitizeGeneratedName(coverpointp->name()) + "_"
                                 + cvtToStr(sized),
                             binp->findBitDType()};
            varp->funcLocal(സത്യം);
            m_sampleFuncp->addStmtsp(varp);
            addSampleStmt(പുതിയ AstAssign{fl, പുതിയ AstVarRef{fl, varp, VAccess::WRITE}, enabledp});
            enabledp = പുതിയ AstVarRef{fl, varp, VAccess::READ};
        }
        AstCMethodHard* സ്ഥിരം callp
            = itemCall(fl, cpVarp,
                       exprp->isWide() ? VCMethod::COVERGROUP_SIZED_SAMPLE_W
                                       : VCMethod::COVERGROUP_SIZED_SAMPLE,
                       {cnum(fl, sized), exprp->clone_വൃക്ഷം(അസത്യം), enabledp});
        callp->dtypeSetBit();
        എങ്കിൽ (illegal) {
            addSampleStmt(പുതിയ AstIf{fl, പുതിയ AstLogAnd{fl, callp, enabledp->clone_വൃക്ഷം(അസത്യം)},
                                    makeIllegalBinAction(fl, "Illegal bin " + binp->prettyNameQ()
                                                                 + " hit in coverpoint "
                                                                 + coverpointp->prettyNameQ())});
        } അല്ലെങ്കിൽ എങ്കിൽ (matchedp && binp->binsType().binIsNormal()) {
            addSampleStmt(
                പുതിയ AstAssign{fl, പുതിയ AstVarRef{fl, matchedp, VAccess::WRITE},
                              പുതിയ AstOr{fl, പുതിയ AstVarRef{fl, matchedp, VAccess::READ}, callp}});
        } അല്ലെങ്കിൽ {
            addSampleStmt(callp->makeStmt());
        }
    }

    // A variable local to the constructor
    AstVar* constructorTemp(File_വരി* fl, സ്ഥിരം string& name, Ast_കണ്ണി_DType* dtypep) {
        AstVar* സ്ഥിരം varp = പുതിയ AstVar{fl, VVarType::BLOCKTEMP, name, dtypep};
        varp->funcLocal(സത്യം);
        m_constructorp->addStmtsp(varp);
        തിരികെ varp;
    }

    // Truncate or extend, as its signedness sets, a value to a type
    സ്ഥിതം Ast_കണ്ണി_വ്യഞ്ജകം* resize_മൂല്യം(Ast_കണ്ണി_വ്യഞ്ജകം* valuep, Ast_കണ്ണി_DType* dtypep) {
        File_വരി* സ്ഥിരം fl = valuep->fileline();
        എങ്കിൽ (valuep->width() > dtypep->width()) {
            valuep = പുതിയ AstSel{fl, valuep, 0, dtypep->width()};
        } അല്ലെങ്കിൽ എങ്കിൽ (valuep->width() < dtypep->width()) {
            valuep = valuep->isSigned()
                         ? static_cast<Ast_കണ്ണി_വ്യഞ്ജകം*>(പുതിയ AstExtendS{fl, valuep, dtypep->width()})
                         : പുതിയ AstExtend{fl, valuep, dtypep->width()};
        }
        valuep->dtypep(dtypep);
        തിരികെ valuep;
    }

    // The runtime arguments of a count, which the constructor evaluates once into a temporary:
    // its value, a count wider than 64 bits saturated over any limit, and whether it is positive
    std::pair<Ast_കണ്ണി_വ്യഞ്ജകം*, Ast_കണ്ണി_വ്യഞ്ജകം*> new_എണ്ണം_Args(File_വരി* fl, സ്ഥിരം string& name,
                                                       Ast_കണ്ണി_വ്യഞ്ജകം* exprp) {
        AstVar* സ്ഥിരം countp = constructorTemp(fl, name, exprp->dtypep());
        m_constructorp->addStmtsp(
            പുതിയ AstAssign{fl, പുതിയ AstVarRef{fl, countp, VAccess::WRITE}, exprp->clone_വൃക്ഷം(അസത്യം)});
        സ്ഥിരം auto എണ്ണം_Ref = [&]() { തിരികെ പുതിയ AstVarRef{fl, countp, VAccess::READ}; };
        AstConst* സ്ഥിരം zerop = പുതിയ AstConst{fl, AstConst::DTyped{}, countp->dtypep()};
        Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം positivep
            = countp->isSigned() ? static_cast<Ast_കണ്ണി_വ്യഞ്ജകം*>(പുതിയ AstGtS{fl, എണ്ണം_Ref(), zerop})
                                 : പുതിയ AstNeq{fl, എണ്ണം_Ref(), zerop};
        Ast_കണ്ണി_വ്യഞ്ജകം* valuep = resize_മൂല്യം(എണ്ണം_Ref(), countp->findUInt64DType());
        എങ്കിൽ (countp->width() > VL_QUADSIZE) {
            valuep = പുതിയ AstCond{
                fl,
                പുതിയ AstRedOr{
                    fl, പുതിയ AstSel{fl, എണ്ണം_Ref(), VL_QUADSIZE, countp->width() - VL_QUADSIZE}},
                പുതിയ AstConst{fl, AstConst::Unsized64{}, std::numeric_limits<uint64_t>::max()},
                valuep};
            valuep->dtypeSetUInt64();
        }
        തിരികെ {valuep, positivep};
    }
    // The runtime arguments of a constant count
    സ്ഥിതം std::pair<Ast_കണ്ണി_വ്യഞ്ജകം*, Ast_കണ്ണി_വ്യഞ്ജകം*> new_എണ്ണം_Args(File_വരി* fl, uint64_t എണ്ണം) {
        തിരികെ {പുതിയ AstConst{fl, AstConst::Unsized64{}, എണ്ണം},
                പുതിയ AstConst{fl, AstConst::BitTrue{}}};
    }

    // The values of a coverpoint, which its name denotes (IEEE 1800-2023 19.5.1.1), as runs in
    // value order: an enumerated type's values (6.19), else all values of its type.  Bounds
    // are at runWidth(), sign-extended like a CrossValueRange's.
    സ്ഥിതം std::vector<std::pair<V3_സംഖ്യ, V3_സംഖ്യ>> coverpoint_മൂല്യങ്ങൾ(Ast_കണ്ണി* nodep,
                                                                       Ast_കണ്ണി_വ്യഞ്ജകം* exprp) {
        സ്ഥിരം പൂർണ്ണസംഖ്യ width = runWidth(exprp);
        std::vector<std::pair<V3_സംഖ്യ, V3_സംഖ്യ>> runs;
        സ്ഥിരം AstEnumDType* സ്ഥിരം enump = VN_CAST(exprp->dtypep()->skipRefToEnump(), EnumDType);
        എങ്കിൽ (!enump) {
            സ്ഥിരം Cross_മൂല്യം_Range domain
                = cross_മൂല്യം_Domain(nodep, exprp->width(), exprp->isSigned(), width);
            runs.emplace_back(domain.lo, domain.hi);
            തിരികെ runs;
        }
        std::vector<V3_സംഖ്യ> മൂല്യങ്ങൾ;
        വേണ്ടി (സ്ഥിരം AstEnumItem* itemp = enump->itemsp(); itemp;
             itemp = VN_AS(itemp->nextp(), EnumItem)) {
            സ്ഥിരം V3_സംഖ്യ& അക്കം = VN_AS(itemp->valuep(), Const)->അക്കം();
            എങ്കിൽ (അക്കം.isFourState()) തുടരുക;  // Not a coverpoint value (19.5.7)
            മൂല്യങ്ങൾ.emplace_back(nodep, width);
            എങ്കിൽ (exprp->isSigned()) {
                മൂല്യങ്ങൾ.back().opExtendS(അക്കം, അക്കം.width());
            } അല്ലെങ്കിൽ {
                മൂല്യങ്ങൾ.back().opAssign(അക്കം);
            }
        }
        std::sort(മൂല്യങ്ങൾ.begin(), മൂല്യങ്ങൾ.end(), cross_മൂല്യം_Less);
        സ്ഥിരം V3_സംഖ്യ one{nodep, width, 1};
        വേണ്ടി (സ്ഥിരം V3_സംഖ്യ& മൂല്യം : മൂല്യങ്ങൾ) {
            V3_സംഖ്യ next{nodep, width};
            എങ്കിൽ (!runs.empty() && next.opAdd(runs.back().second, one).isCaseEq(മൂല്യം)) {
                runs.back().second = മൂല്യം;
            } അല്ലെങ്കിൽ {
                runs.emplace_back(മൂല്യം, മൂല്യം);
            }
        }
        തിരികെ runs;
    }

    // Emit the constructor code building the bins 'binp' whose values it computes: the values of
    // each element that are coverpoint values (IEEE 1800-2023 19.5.7), those a 'with' filter
    // keeps (19.5.1.1), then its bins
    ശൂന്യം generateConstructedBins(AstCoverpoint* coverpointp, AstCoverBin* binp, AstVar* cpVarp,
                                 Ast_കണ്ണി_വ്യഞ്ജകം* exprp) {
        File_വരി* സ്ഥിരം fl = binp->fileline();
        സ്ഥിരം string prefix
            = m_sizedNames.get(sanitizeGeneratedName(coverpointp->name() + "__" + binp->name()));
        // The count, and whether it is positive; one of a filter's scalar bin, or bin per value
        സ്ഥിരം std::pair<Ast_കണ്ണി_വ്യഞ്ജകം*, Ast_കണ്ണി_വ്യഞ്ജകം*> എണ്ണം
            = binp->arraySizep() ? new_എണ്ണം_Args(fl, prefix + "_count", binp->arraySizep())
                                 : new_എണ്ണം_Args(fl, 1);
        AstCoverWith* സ്ഥിരം withp = binWith(binp);
        എങ്കിൽ (withp && !binRangesp(binp)) {  // The coverpoint's name: all of its values
            വേണ്ടി (സ്ഥിരം std::pair<V3_സംഖ്യ, V3_സംഖ്യ>& run : coverpoint_മൂല്യങ്ങൾ(binp, exprp)) {
                m_constructorp->addStmtsp(itemCall(fl, cpVarp,
                                                   exprp->isWide()
                                                       ? VCMethod::COVERGROUP_SIZED_RANGE_W
                                                       : VCMethod::COVERGROUP_SIZED_RANGE,
                                                   {new_മൂല്യം_Const(fl, run.first, exprp),
                                                    new_മൂല്യം_Const(fl, run.second, exprp)})
                                              ->makeStmt());
            }
        }
        uint32_t element = 0;
        വേണ്ടി (Ast_കണ്ണി* rangep = binRangesp(binp); rangep; rangep = rangep->nextp()) {
            എങ്കിൽ (VN_IS(rangep, Unbounded)) {  // A parameter of '$'
                binp->v3error("Bins value may not be '$', which may only bound a range "
                              "(IEEE 1800-2023 6.20.7)");
                തുടരുക;
            }
            generateSizedElement(cpVarp, binp, rangep, exprp, prefix + "_" + cvtToStr(element++));
        }
        എങ്കിൽ (withp) generateWithFilter(binp, withp, cpVarp, exprp, prefix);
        std::vector<Ast_കണ്ണി_വ്യഞ്ജകം*> args{ctext(fl, binp->binsType().binSetEnum()), എണ്ണം.first,
                                       എണ്ണം.second};
        // A filter's bins have the limit it began with
        എങ്കിൽ (!withp) args.തള്ളുക_back(cnum(fl, v3Global.opt.coverageMaxBins()));
        സ്ഥിരം തർക്കം prot = v3Global.opt.protectIds();
        args.തള്ളുക_back(ctext(fl, quoted(VIdProtect::protect_വാക്കുകൾ_If(binp->name(), prot))));
        args.തള്ളുക_back(ctext(fl, quoted(VIdProtect::protectIf(fl->filename(), prot))));
        args.തള്ളുക_back(cnum(fl, static_cast<uint32_t>(fl->lineno())));
        args.തള്ളുക_back(cnum(fl, static_cast<uint32_t>(fl->firstColumn())));
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
    ശൂന്യം generateWithFilter(AstCoverBin* binp, AstCoverWith* withp, AstVar* cpVarp,
                            Ast_കണ്ണി_വ്യഞ്ജകം* exprp, സ്ഥിരം string& prefix) {
        File_വരി* സ്ഥിരം fl = withp->fileline();
        സ്ഥിരം തർക്കം wide = exprp->isWide();
        സ്ഥിരം string grouping = !binp->isArray()     ? "Single"
                                : binp->arraySizep() ? "Fixed"
                                                     : "Values";
        m_constructorp->addStmtsp(itemCall(fl, cpVarp, VCMethod::COVERGROUP_WITH_BEGIN,
                                           {ctext(fl, "VlCovBinGrouping::" + grouping),
                                            cnum(fl, v3Global.opt.coverageMaxBins())})
                                      ->makeStmt());
        // The candidates count in the coverpoint's width, and the filter reads each as 'item',
        // of the coverpoint's type, so that a filter changing 'item' cannot change the loop
        Ast_കണ്ണി_DType* സ്ഥിരം മൂല്യം_DTypep = exprp->dtypep();
        AstVar* സ്ഥിരം valuep = constructorTemp(fl, prefix + "_value", മൂല്യം_DTypep);
        AstVar* സ്ഥിരം lastp = constructorTemp(fl, prefix + "_last", മൂല്യം_DTypep);
        AstVar* സ്ഥിരം firstp = constructorTemp(fl, prefix + "_first", മൂല്യം_DTypep);
        AstVar* സ്ഥിരം runp = constructorTemp(fl, prefix + "_run", binp->findBitDType());
        AstVar* സ്ഥിരം morep = constructorTemp(fl, prefix + "_more", binp->findBitDType());
        AstVar* സ്ഥിരം itemp = withp->itemp()->unlinkFrBack();
        itemp->name(prefix + "_item");
        m_constructorp->addStmtsp(itemp);
        സ്ഥിരം auto ref = [&](AstVar* varp) { തിരികെ പുതിയ AstVarRef{fl, varp, VAccess::READ}; };
        സ്ഥിരം auto assign = [&](AstVar* varp, Ast_കണ്ണി_വ്യഞ്ജകം* rhsp) -> Ast_കണ്ണി* {
            തിരികെ പുതിയ AstAssign{fl, പുതിയ AstVarRef{fl, varp, VAccess::WRITE}, rhsp};
        };
        സ്ഥിരം auto flag = [&](AstVar* varp, തർക്കം മൂല്യം) {
            തിരികെ assign(varp, മൂല്യം ? പുതിയ AstConst{fl, AstConst::BitTrue{}}
                                      : പുതിയ AstConst{fl, AstConst::BitFalse{}});
        };
        സ്ഥിരം auto bound = [&](VCMethod narrow, VCMethod wideMethod, AstVar* varp) -> Ast_കണ്ണി* {
            എങ്കിൽ (wide) {
                തിരികെ itemCall(fl, cpVarp, wideMethod, {പുതിയ AstVarRef{fl, varp, VAccess::WRITE}})
                    ->makeStmt();
            }
            AstCMethodHard* സ്ഥിരം callp = itemCall(fl, cpVarp, narrow);
            callp->dtypeSetUInt64();
            തിരികെ assign(varp, resize_മൂല്യം(callp, മൂല്യം_DTypep));
        };
        സ്ഥിരം auto keep = [&](Ast_കണ്ണി_വ്യഞ്ജകം* lop, Ast_കണ്ണി_വ്യഞ്ജകം* hip) {
            AstCMethodHard* സ്ഥിരം callp = itemCall(
                fl, cpVarp, wide ? VCMethod::COVERGROUP_WITH_RUN_W : VCMethod::COVERGROUP_WITH_RUN,
                {lop, hip});
            callp->dtypeSetBit();
            തിരികെ assign(morep, callp);
        };
        സ്ഥിരം auto step = [&](തർക്കം up) {
            AstConst* സ്ഥിരം onep = പുതിയ AstConst{fl, AstConst::Widthed_മൂല്യം{}, exprp->width(), 1};
            Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം stepp
                = up ? static_cast<Ast_കണ്ണി_വ്യഞ്ജകം*>(പുതിയ AstAdd{fl, ref(valuep), onep})
                     : പുതിയ AstSub{fl, ref(valuep), onep};
            stepp->dtypep(മൂല്യം_DTypep);
            തിരികെ stepp;
        };
        AstLoop* സ്ഥിരം innerp = പുതിയ AstLoop{fl};
        innerp->addStmtsp(assign(itemp, ref(valuep)));
        innerp->addStmtsp(പുതിയ AstIf{
            fl, withp->filterp()->unlinkFrBack(),
            പുതിയ AstIf{fl, പുതിയ AstNot{fl, ref(runp)},
                      assign(firstp, ref(valuep))->addNext(flag(runp, സത്യം))},
            പുതിയ AstIf{fl, ref(runp), keep(ref(firstp), step(അസത്യം))->addNext(flag(runp, അസത്യം))}});
        innerp->addStmtsp(പുതിയ AstLoopTest{
            fl, innerp, പുതിയ AstLogAnd{fl, ref(morep), പുതിയ AstNeq{fl, ref(valuep), ref(lastp)}}});
        innerp->addStmtsp(assign(valuep, step(സത്യം)));
        AstLoop* സ്ഥിരം outerp = പുതിയ AstLoop{fl};
        AstCMethodHard* സ്ഥിരം nextp = itemCall(fl, cpVarp, VCMethod::COVERGROUP_WITH_NEXT);
        nextp->dtypeSetBit();
        outerp->addStmtsp(പുതിയ AstLoopTest{fl, outerp, nextp});
        outerp->addStmtsp(
            bound(VCMethod::COVERGROUP_WITH_LO, VCMethod::COVERGROUP_WITH_LO_W, valuep));
        outerp->addStmtsp(
            bound(VCMethod::COVERGROUP_WITH_HI, VCMethod::COVERGROUP_WITH_HI_W, lastp));
        outerp->addStmtsp(flag(runp, അസത്യം));
        outerp->addStmtsp(innerp);
        outerp->addStmtsp(പുതിയ AstIf{fl, ref(runp), keep(ref(firstp), ref(lastp))});
        m_constructorp->addStmtsp(flag(morep, സത്യം));
        m_constructorp->addStmtsp(outerp);
    }

    // Emit 'sizedRange(lo, hi)' for the coverpoint values of an element of bins 'binp', whose
    // values the constructor computes: resolved now if constant, else when constructed by
    // clipping to the coverpoint's values.  A wildcard pattern's values give a range for each
    // run of them.  'prefix' names its temporaries.
    ശൂന്യം generateSizedElement(AstVar* cpVarp, സ്ഥിരം AstCoverBin* binp, Ast_കണ്ണി* rangep,
                              Ast_കണ്ണി_വ്യഞ്ജകം* exprp, സ്ഥിരം string& prefix) {
        File_വരി* സ്ഥിരം fl = rangep->fileline();
        സ്ഥിരം VCMethod method = exprp->isWide() ? VCMethod::COVERGROUP_SIZED_RANGE_W
                                                : VCMethod::COVERGROUP_SIZED_RANGE;
        സ്ഥിരം AstInsideRange* സ്ഥിരം irp = VN_CAST(rangep, InsideRange);
        Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം lowp = irp ? irp->lhsp() : VN_AS(rangep, കണ്ണി_വ്യഞ്ജകം);
        Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം highp = irp ? irp->rhsp() : nullptr;
        സ്ഥിരം auto unbounded
            = [](സ്ഥിരം Ast_കണ്ണി_വ്യഞ്ജകം* boundp) { തിരികെ !boundp || VN_IS(boundp, Unbounded); };
        സ്ഥിരം auto constant
            = [&](സ്ഥിരം Ast_കണ്ണി_വ്യഞ്ജകം* boundp) { തിരികെ unbounded(boundp) || VN_IS(boundp, Const); };
        സ്ഥിരം auto integral = [&](സ്ഥിരം Ast_കണ്ണി_വ്യഞ്ജകം* boundp) {
            തിരികെ unbounded(boundp) || boundp->dtypep()->skipRefp()->isIntegralOrPacked();
        };
        എങ്കിൽ (constant(lowp) && constant(highp)) {
            സ്ഥിരം auto fourState = [](സ്ഥിരം Ast_കണ്ണി_വ്യഞ്ജകം* boundp) {
                സ്ഥിരം AstConst* സ്ഥിരം constp = VN_CAST(boundp, Const);
                തിരികെ constp && constp->അക്കം().isFourState();
            };
            എങ്കിൽ (irp && (fourState(lowp) || fourState(highp))) {
                rangep->v3error("Four-state (x/z) value in "
                                << (binp->isArray() ? "array bins" : "bin")
                                << " range bound; range bounds must be two-state constants");
                തിരികെ;
            }
            Cross_മൂല്യം_Range range{rangep, resolveWidth(rangep, exprp)};
            എങ്കിൽ (!resolve_മൂല്യം(rangep, exprp, സത്യം, binp->isWildcard(), range)) {
                rangep->v3warn(E_UNSUPPORTED, "Unsupported: non-integral value in a coverage bin "
                                              "of an integral coverpoint.");
            } അല്ലെങ്കിൽ എങ്കിൽ (!crossRangeEmpty(range)) {
                std::vector<std::pair<V3_സംഖ്യ, V3_സംഖ്യ>> runs{{range.lo, range.hi}};
                എങ്കിൽ (range.wildcard) {  // checkConstructedBins bounded the runs
                    runs.clear();
                    crossRangeRuns(range, v3Global.opt.coverageMaxBins(), runs);
                }
                വേണ്ടി (സ്ഥിരം std::pair<V3_സംഖ്യ, V3_സംഖ്യ>& run : runs) {
                    m_constructorp->addStmtsp(itemCall(fl, cpVarp, method,
                                                       {new_മൂല്യം_Const(fl, run.first, exprp),
                                                        new_മൂല്യം_Const(fl, run.second, exprp)})
                                                  ->makeStmt());
                }
            }
            തിരികെ;
        }
        എങ്കിൽ (!integral(lowp) || !integral(highp)) {
            rangep->v3warn(E_UNSUPPORTED, "Unsupported: non-integral value in a coverage bin "
                                          "of an integral coverpoint.");
            തിരികെ;
        }
        // Compare values and the coverpoint's domain signed, in a width holding all of them
        സ്ഥിരം auto boundWidth
            = [&](സ്ഥിരം Ast_കണ്ണി_വ്യഞ്ജകം* boundp) { തിരികെ unbounded(boundp) ? 0 : boundp->width(); };
        സ്ഥിരം പൂർണ്ണസംഖ്യ width = std::max({exprp->width(), boundWidth(lowp), boundWidth(highp)}) + 1;
        സ്ഥിരം Cross_മൂല്യം_Range domain
            = cross_മൂല്യം_Domain(rangep, exprp->width(), exprp->isSigned(), width);
        AstVar* സ്ഥിരം lop = constructorTemp(fl, prefix + "_lo", exprp->dtypep());
        lop->dtypeSetBitSized(width, VSigning::SIGNED);
        AstVar* സ്ഥിരം hip = constructorTemp(fl, prefix + "_hi", lop->dtypep());
        സ്ഥിരം auto bound = [&](Ast_കണ്ണി_വ്യഞ്ജകം* boundp, സ്ഥിരം V3_സംഖ്യ& limit) -> Ast_കണ്ണി_വ്യഞ്ജകം* {
            എങ്കിൽ (unbounded(boundp)) {
                AstConst* സ്ഥിരം limitp = പുതിയ AstConst{fl, limit};
                limitp->dtypeFrom(lop);
                തിരികെ limitp;
            }
            Ast_കണ്ണി_വ്യഞ്ജകം* valuep = boundp->clone_വൃക്ഷം(അസത്യം);
            // A value no wider than a signed coverpoint has its type (see crossRangeBound)
            എങ്കിൽ (exprp->isSigned() && boundp->width() <= exprp->width()) {
                valuep = resize_മൂല്യം(valuep, exprp->dtypep());
            }
            തിരികെ resize_മൂല്യം(valuep, lop->dtypep());
        };
        സ്ഥിരം auto ref = [&](AstVar* varp, VAccess access = VAccess::READ) {
            തിരികെ പുതിയ AstVarRef{fl, varp, access};
        };
        m_constructorp->addStmtsp(
            പുതിയ AstAssign{fl, ref(lop, VAccess::WRITE), bound(lowp, domain.lo)});
        m_constructorp->addStmtsp(
            പുതിയ AstAssign{fl, ref(hip, VAccess::WRITE), irp ? bound(highp, domain.hi) : ref(lop)});
        AstConst* സ്ഥിരം minp = പുതിയ AstConst{fl, domain.lo};
        AstConst* സ്ഥിരം maxp = പുതിയ AstConst{fl, domain.hi};
        minp->dtypeFrom(lop);
        maxp->dtypeFrom(lop);
        AstCond* സ്ഥിരം lowerp
            = പുതിയ AstCond{fl, പുതിയ AstLtS{fl, ref(lop), minp}, minp->clone_വൃക്ഷം(അസത്യം), ref(lop)};
        AstCond* സ്ഥിരം upperp
            = പുതിയ AstCond{fl, പുതിയ AstGtS{fl, ref(hip), maxp}, maxp->clone_വൃക്ഷം(അസത്യം), ref(hip)};
        lowerp->dtypeFrom(lop);
        upperp->dtypeFrom(lop);
        m_constructorp->addStmtsp(പുതിയ AstAssign{fl, ref(lop, VAccess::WRITE), lowerp});
        m_constructorp->addStmtsp(പുതിയ AstAssign{fl, ref(hip, VAccess::WRITE), upperp});
        m_constructorp->addStmtsp(പുതിയ AstIf{fl, പുതിയ AstLteS{fl, ref(lop), ref(hip)},
                                            itemCall(fl, cpVarp, method,
                                                     {resize_മൂല്യം(ref(lop), exprp->dtypep()),
                                                      resize_മൂല്യം(ref(hip), exprp->dtypep())})
                                                ->makeStmt()});
    }

    // The runtime index of the bin of a run holding the coverpoint value, which is in the run:
    // declared + (value - lo) / stride, capped at the last bin, which holds any remainder.
    സ്ഥിതം Ast_കണ്ണി_വ്യഞ്ജകം* runBinIndex(File_വരി* fl, Ast_കണ്ണി_വ്യഞ്ജകം* exprp, സ്ഥിരം BinRun& run) {
        എങ്കിൽ (run.m_എണ്ണം == 1) തിരികെ cnum(fl, run.m_declared);
        സ്ഥിരം പൂർണ്ണസംഖ്യ width = exprp->width();
        // A run spans at most 2^width values, so offsets in it are unsigned width-bit numbers
        Ast_കണ്ണി_വ്യഞ്ജകം* indexp
            = പുതിയ AstSub{fl, exprp->clone_വൃക്ഷം(അസത്യം), new_മൂല്യം_Const(fl, run.m_lo, exprp)};
        indexp->dtypeSetBitSized(width, VSigning::UNSIGNED);
        V3_സംഖ്യ stride{fl, width, 0};
        stride.opAssign(run.m_stride);
        എങ്കിൽ (stride.എണ്ണം_Ones() != 1) {
            indexp = പുതിയ AstDiv{fl, indexp, പുതിയ AstConst{fl, stride}};
        } അല്ലെങ്കിൽ എങ്കിൽ (!stride.isEqOne()) {
            indexp = പുതിയ AstShiftR{fl, indexp, പുതിയ AstConst{fl, stride.mostSetBitP1() - 1}};
        }
        // Compare the run's values with those of count bins of stride values, without overflow
        സ്ഥിരം പൂർണ്ണസംഖ്യ extWidth = run.m_lo.width() + 1;
        V3_സംഖ്യ lo{fl, extWidth, 0};
        lo.opExtendS(run.m_lo, run.m_lo.width());
        V3_സംഖ്യ span{fl, extWidth, 0};
        span.opExtendS(run.m_hi, run.m_hi.width());
        span.opSub(V3_സംഖ്യ{span}, lo);
        V3_സംഖ്യ covered{fl, extWidth, 0};
        covered.opAssign(run.m_stride);
        covered.opMul(V3_സംഖ്യ{covered}, V3_സംഖ്യ{fl, extWidth, run.m_എണ്ണം});
        V3_സംഖ്യ hasRemainder{fl, 1, 0};
        എങ്കിൽ (!hasRemainder.opGte(span, covered).isEqZero()) {
            AstConst* സ്ഥിരം lastp = പുതിയ AstConst{fl, V3_സംഖ്യ{fl, width, run.m_എണ്ണം - 1}};
            indexp = പുതിയ AstCond{fl, പുതിയ AstGt{fl, indexp->clone_വൃക്ഷം(അസത്യം), lastp},
                                 lastp->clone_വൃക്ഷം(അസത്യം), indexp};
        }
        എങ്കിൽ (width < VL_IDATASIZE) {
            indexp = പുതിയ AstExtend{fl, indexp, VL_IDATASIZE};
        } അല്ലെങ്കിൽ എങ്കിൽ (width > VL_IDATASIZE) {
            indexp = പുതിയ AstSel{fl, indexp, 0, VL_IDATASIZE};
        }
        തിരികെ പുതിയ AstAdd{fl, cnum(fl, run.m_declared), indexp};
    }

    // Emit the sample() hit of a run of bins, whose code does not grow with its number of bins:
    //   if (iff && lo <= value && value <= hi) m_cp.incrementBin(<runBinIndex>);
    ശൂന്യം emitRunHit(AstCoverpoint* coverpointp, AstCoverBin* binp, AstVar* cpVarp,
                    Ast_കണ്ണി_വ്യഞ്ജകം* exprp, സ്ഥിരം BinRun& run) {
        File_വരി* സ്ഥിരം fl = binp->fileline();
        AstConst* സ്ഥിരം lop = new_മൂല്യം_Const(fl, run.m_lo, exprp);
        Ast_കണ്ണി_വ്യഞ്ജകം* condp = nullptr;
        എങ്കിൽ (run.m_lo.isCaseEq(run.m_hi)) {
            condp = പുതിയ AstEq{fl, exprp->clone_വൃക്ഷം(അസത്യം), lop};
        } അല്ലെങ്കിൽ {
            AstConst* സ്ഥിരം hip = new_മൂല്യം_Const(fl, run.m_hi, exprp);
            condp = makeRangeCondition(fl, exprp, lop, hip);
            VL_DO_DANGLING(തള്ളുക_Deletep(lop), lop);
            VL_DO_DANGLING(തള്ളുക_Deletep(hip), hip);
        }
        emitConvHitIf(coverpointp, binp, cpVarp, runBinIndex(fl, exprp, run), condp);
    }

    // Emit a transition bin's hit action into sample():
    //   if (cond) { m_cp.incrementBin/recordHit(idx); [illegal: $error; $stop] }
    // Used by the transition generators so a completed sequence records into the runtime bin.
    ശൂന്യം addConvTransHitIf(AstCoverpoint* coverpointp, AstCoverBin* binp, സ്ഥിരം ConvBinTarget& tgt,
                           Ast_കണ്ണി_വ്യഞ്ജകം* condp) {
        File_വരി* സ്ഥിരം fl = binp->fileline();
        Ast_കണ്ണി* actionp = makeRuntimeBinHit(fl, tgt);
        എങ്കിൽ (binp->binsType() == VCoverBinsType::BINS_ILLEGAL) {
            actionp->addNext(makeIllegalBinAction(
                fl, "Illegal transition bin " + binp->prettyNameQ() + " hit in coverpoint "
                        + coverpointp->prettyNameQ()));
        }
        UASSERT_OBJ(m_sampleFuncp, binp, "sample() CFunc not set for transition bin");
        addSampleStmt(പുതിയ AstIf{fl, condp, actionp, nullptr});
    }

    // Route a coverpoint through a VlCoverpoint member: emit the member, its sample()
    // increments, the constructor configuration (init + namers), and registration.
    ശൂന്യം generateCoverpoint(AstCoverpoint* coverpointp, Ast_കണ്ണി_വ്യഞ്ജകം* exprp, പൂർണ്ണസംഖ്യ at_Least_മൂല്യം) {
        File_വരി* സ്ഥിരം fl = coverpointp->fileline();
        സ്ഥിരം തർക്കം dynamic = m_runtimePoints.എണ്ണം(coverpointp);
        UINFO(4, "  Generating VlCoverpoint member: " << coverpointp->name());

        // Size the hit list to the gen-time max bin overlap (1 unless cross-fed with
        // overlapping ranges), so no cross hit is ever dropped and storage is minimal.
        സ്ഥിരം തർക്കം crossFed = m_crossedCpNames.എണ്ണം(coverpointp->name()) != 0;
        സ്ഥിരം പൂർണ്ണസംഖ്യ hitBound = computeHitListBound(coverpointp, exprp, crossFed);
        UINFO(6, "    Hit-list bound (max bin overlap) = " << hitBound);
        AstVar* സ്ഥിരം cpVarp = പുതിയ AstVar{fl, VVarType::MEMBER, "__Vcp_" + coverpointp->name(),
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
        എങ്കിൽ (crossFed) {
            UASSERT_OBJ(m_sampleFuncp, coverpointp, "sample() CFunc not set for clearHitList");
            m_sampleFuncp->addStmtsp(
                itemCall(fl, cpVarp, VCMethod::COVERGROUP_CLEAR_HIT_LIST)->makeStmt());
        }
        എങ്കിൽ (dynamic && coverpointHasStateExclusions(coverpointp)) {
            AstVar* സ്ഥിരം excludedp
                = പുതിയ AstVar{fl, VVarType::BLOCKTEMP,
                             "__VcpExcluded_" + sanitizeGeneratedName(coverpointp->name()),
                             coverpointp->findBitDType()};
            excludedp->funcLocal(സത്യം);
            m_sampleFuncp->addStmtsp(excludedp);
            AstCMethodHard* സ്ഥിരം callp
                = itemCall(fl, cpVarp,
                           exprp->isWide() ? VCMethod::COVERGROUP_മൂല്യം_EXCLUDED_W
                                           : VCMethod::COVERGROUP_മൂല്യം_EXCLUDED,
                           {exprp->clone_വൃക്ഷം(അസത്യം)});
            callp->dtypeSetBit();
            addSampleStmt(പുതിയ AstAssign{fl, പുതിയ AstVarRef{fl, excludedp, VAccess::WRITE}, callp});
            m_excludedVars.emplace(cpVarp, excludedp);
        }

        // Walk bins (non-default, then default), assigning sequential indices that match the
        // namer append order; emit sample increments and collect namer statements.  Constructed
        // bins follow them all, placed when the coverpoint is constructed.
        std::vector<Ast_കണ്ണി_Stmt*> namerStmts;
        std::vector<AstCoverBin*> defaultBins;
        std::vector<AstCoverBin*> sizedBins;
        std::vector<std::tuple<AstCoverBin*, uint32_t, Ast_കണ്ണി_വ്യഞ്ജകം*>> metadata;
        std::vector<സ്ഥിരം BinRun*> runMetadata;
        uint64_t idx = 0;  // Runtime index of the next bin; 32-bit once checked below
        വേണ്ടി (Ast_കണ്ണി* binp = coverpointp->binsp(); binp; binp = binp->nextp()) {
            AstCoverBin* സ്ഥിരം cbinp = VN_AS(binp, CoverBin);
            സ്ഥിരം പൂർണ്ണസംഖ്യ errorsBefore = dynamic ? V3Error::error_എണ്ണം() : 0;
            എങ്കിൽ (cbinp->binsType() == VCoverBinsType::BINS_DEFAULT) {
                defaultBins.തള്ളുക_back(cbinp);
                തുടരുക;
            }
            എങ്കിൽ (isConstructedBins(cbinp)) {
                UASSERT_OBJ(dynamic, cbinp, "Constructed bins without value metadata");
                BinSpan span;
                span.sized = static_cast<int32_t>(sizedBins.size());
                m_cpBins.at(cpVarp).spans.emplace(cbinp->name(), span);
                sizedBins.തള്ളുക_back(cbinp);
                തുടരുക;
            }
            എങ്കിൽ (cbinp->transp()) {
                // Transition bin (incl. array transition 'bins t[] = (a=>b),(c=>d)' and
                // illegal_bins/ignore_bins transitions).  All sequences of one transition bin
                // share a bin name and merge in the coverage DB to a single point, so model
                // them as one runtime bin incremented by any matching sequence.  The sequence
                // matching is generated as a state machine, with the hit routed to this bin's
                // runtime slot.
                namerStmts.തള്ളുക_back(makeNamer(cpVarp, cbinp, -1, static_cast<uint32_t>(idx)));
                സ്ഥിരം ConvBinTarget tgt{cpVarp, static_cast<uint32_t>(idx),
                                        cbinp->binsType().binIsNormal()};
                വേണ്ടി (Ast_കണ്ണി* sp = cbinp->transp(); sp; sp = sp->nextp())
                    generateSingleTransitionCode(coverpointp, cbinp, exprp, tgt,
                                                 VN_AS(sp, CoverTransSet));
                എങ്കിൽ (dynamic && V3Error::error_എണ്ണം() == errorsBefore) {
                    metadata.emplace_back(cbinp, static_cast<uint32_t>(idx), nullptr);
                }
                ++idx;
                തുടരുക;
            }
            BinRuns plan;
            എങ്കിൽ (binRunsFor(cbinp, exprp, plan)) {
                // Array elements and automatic bins generate as runs, so neither sample() nor
                // the constructor grows with their number of bins.
                എങ്കിൽ (plan.unsupported) {  // bin ignored or invalid; reserve no slot
                    m_droppedBins[coverpointp].തള്ളുക_back(cbinp->name());
                    തുടരുക;
                }
                CoverpointBins& bins = m_cpBins.at(cpVarp);
                സ്ഥിരം uint32_t first_മൂല്യം = bins.total;
                സ്ഥിരം uint32_t firstDeclared = static_cast<uint32_t>(idx);
                namerStmts.തള്ളുക_back(
                    makeNamer(cpVarp, cbinp, plan.എണ്ണം, firstDeclared, {}, plan.മൂല്യങ്ങൾ));
                വേണ്ടി (BinRun& run : plan.runs) {
                    run.m_declared = static_cast<uint32_t>(idx);
                    bins.runs.തള്ളുക_back(std::move(run));
                    സ്ഥിരം BinRun& stored = bins.runs.back();
                    എങ്കിൽ (bins.crossed && cbinp->binsType().binIsNormal()) {
                        സ്ഥിരം uint32_t first = first_മൂല്യം + stored.m_declared - firstDeclared;
                        വേണ്ടി (uint32_t element = 0; element < stored.m_എണ്ണം; ++element) {
                            bins.മൂല്യങ്ങൾ[first + element].runp = &stored;
                            bins.മൂല്യങ്ങൾ[first + element].element = element;
                        }
                    }
                    എങ്കിൽ (!stored.m_empty) {
                        emitRunHit(coverpointp, cbinp, cpVarp, exprp, stored);
                        എങ്കിൽ (dynamic && V3Error::error_എണ്ണം() == errorsBefore) {
                            runMetadata.തള്ളുക_back(&stored);
                        }
                    }
                    idx += stored.m_എണ്ണം;
                }
                തുടരുക;
            }
            എങ്കിൽ (cbinp->isArray()) {  // value array of a real coverpoint: b[0]..b[N-1]
                // Only integral coverpoints have runtime value metadata (m_runtimePoints)
                UASSERT_OBJ(!dynamic, cbinp, "Runtime value metadata for a real coverpoint");
                തർക്കം unsupported = അസത്യം;
                std::vector<Ast_കണ്ണി_വ്യഞ്ജകം*> മൂല്യങ്ങൾ = extract_Array_മൂല്യങ്ങൾ(cbinp, exprp, unsupported);
                എങ്കിൽ (unsupported) {  // bin ignored (COVERIGN emitted); reserve no slot
                    m_droppedBins[coverpointp].തള്ളുക_back(cbinp->name());
                    തുടരുക;
                }
                namerStmts.തള്ളുക_back(makeNamer(cpVarp, cbinp, static_cast<int64_t>(മൂല്യങ്ങൾ.size()),
                                               static_cast<uint32_t>(idx), മൂല്യങ്ങൾ));
                വേണ്ടി (Ast_കണ്ണി_വ്യഞ്ജകം* valuep : മൂല്യങ്ങൾ) {
                    // The cross selections of this covergroup still read the value.
                    m_detached_മൂല്യങ്ങൾ.തള്ളുക_back(valuep);
                    emitConvHitIf(coverpointp, cbinp, cpVarp,
                                  cnum(cbinp->fileline(), static_cast<uint32_t>(idx)),
                                  build_മൂല്യം_Condition(cbinp, exprp, valuep));
                    ++idx;
                }
            } അല്ലെങ്കിൽ {
                namerStmts.തള്ളുക_back(makeNamer(cpVarp, cbinp, -1, static_cast<uint32_t>(idx)));
                // buildBinCondition is null for 'ignore_bins = default' (no ranges); the bin
                // still gets a reserved slot (recorded, never incremented).
                എങ്കിൽ (Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം condp = buildBinCondition(cbinp, exprp))
                    emitConvHitIf(coverpointp, cbinp, cpVarp,
                                  cnum(cbinp->fileline(), static_cast<uint32_t>(idx)), condp);
                എങ്കിൽ (dynamic && V3Error::error_എണ്ണം() == errorsBefore) {
                    metadata.emplace_back(cbinp, static_cast<uint32_t>(idx), nullptr);
                }
                ++idx;
            }
        }
        // A cross selecting an ignored bins declaration selects no bins
        വേണ്ടി (സ്ഥിരം std::string& name : m_droppedBins[coverpointp]) {
            m_cpBins.at(cpVarp).spans.emplace(name, BinSpan{});
        }
        // A value a constructed bin holds is no default bin's; only sampling tells which do
        AstVar* sizedMatchedp = nullptr;
        എങ്കിൽ (!defaultBins.empty()
            && std::any_of(sizedBins.begin(), sizedBins.end(), [](സ്ഥിരം AstCoverBin* binp) {
                   തിരികെ binp->binsType().binIsNormal();
               })) {
            sizedMatchedp = പുതിയ AstVar{fl, VVarType::BLOCKTEMP,
                                       "__VcpSized_" + sanitizeGeneratedName(coverpointp->name()),
                                       coverpointp->findBitDType()};
            sizedMatchedp->funcLocal(സത്യം);
            m_sampleFuncp->addStmtsp(sizedMatchedp);
            addSampleStmt(പുതിയ AstAssign{fl, പുതിയ AstVarRef{fl, sizedMatchedp, VAccess::WRITE},
                                        പുതിയ AstConst{fl, AstConst::BitFalse{}}});
        }
        വേണ്ടി (uint32_t sized = 0; sized < sizedBins.size(); ++sized) {
            emitSizedSample(coverpointp, sizedBins[sized], cpVarp, exprp, sized, sizedMatchedp);
        }
        വേണ്ടി (AstCoverBin* സ്ഥിരം defBinp : defaultBins) {
            File_വരി* സ്ഥിരം dfl = defBinp->fileline();
            namerStmts.തള്ളുക_back(makeNamer(cpVarp, defBinp, -1, static_cast<uint32_t>(idx)));
            Ast_കണ്ണി_വ്യഞ്ജകം* condp = buildDefaultCondition(coverpointp, exprp, dfl);
            എങ്കിൽ (sizedMatchedp) {
                condp = പുതിയ AstLogAnd{
                    dfl, പുതിയ AstNot{dfl, പുതിയ AstVarRef{dfl, sizedMatchedp, VAccess::READ}}, condp};
            }
            emitConvHitIf(coverpointp, defBinp, cpVarp, cnum(dfl, static_cast<uint32_t>(idx)),
                          condp);
            ++idx;
        }
        എങ്കിൽ (idx > std::numeric_limits<uint32_t>::max()) {
            // The runtime indexes bins with 32 bits; stop before generating a model
            coverpointp->v3warn(E_UNSUPPORTED, "Unsupported: coverpoint with more than "
                                                   << std::numeric_limits<uint32_t>::max()
                                                   << " bins");
        }

        // Transition coverpoints track the previous sampled value; update it once at the end of
        // this coverpoint's sample() contribution (the prev var was created on demand by the
        // transition matching above).
        എങ്കിൽ (coverpointHasTransition(coverpointp)) {
            AstVar* സ്ഥിരം prevVarp = VN_AS(coverpointp->user1p(), Var);
            addSampleStmt(
                പുതിയ AstAssign{coverpointp->fileline(),
                              പുതിയ AstVarRef{prevVarp->fileline(), prevVarp, VAccess::WRITE},
                              exprp->clone_വൃക്ഷം(അസത്യം)});
        }

        // Constructor: init (allocates), namers, then registration (under --coverage).
        // Under --protect-ids the hierarchy and page string reach the coverage database
        // verbatim, so obfuscate them like line/toggle points (per-word hierarchy, whole-
        // unit page).  No-ops when --protect-ids is off.
        സ്ഥിരം തർക്കം prot = v3Global.opt.protectIds();
        സ്ഥിരം std::string hier
            = VIdProtect::protect_വാക്കുകൾ_If(m_covergroupName + "." + coverpointp->name(), prot);
        m_constructorp->addStmtsp(
            itemCall(fl, cpVarp, VCMethod::COVERGROUP_INIT,
                     {ctext(fl, quoted(hier)), cnum(fl, static_cast<uint32_t>(at_Least_മൂല്യം)),
                      cnum(fl, static_cast<uint32_t>(idx))})
                ->makeStmt());
        വേണ്ടി (Ast_കണ്ണി_Stmt* സ്ഥിരം ns : namerStmts) m_constructorp->addStmtsp(ns);
        എങ്കിൽ (dynamic) {
            m_constructorp->addStmtsp(
                itemCall(fl, cpVarp, VCMethod::COVERGROUP_മൂല്യം_TYPE,
                         {cnum(fl, exprp->width()), cnum(fl, exprp->isSigned())})
                    ->makeStmt());
            മൂല്യം_Lists lists;
            വേണ്ടി (സ്ഥിരം auto& entry : metadata) {
                collect_മൂല്യം_Metadata(lists, exprp, std::get<0>(entry), std::get<1>(entry),
                                     std::get<2>(entry));
            }
            വേണ്ടി (സ്ഥിരം BinRun* സ്ഥിരം runp : runMetadata) collectRunMetadata(lists, exprp, *runp);
            emit_മൂല്യം_List(fl, cpVarp, VCMethod::COVERGROUP_മൂല്യം_RANGES, lists.m_ranges);
            emit_മൂല്യം_List(fl, cpVarp, VCMethod::COVERGROUP_മൂല്യം_RUNS, lists.m_runs);
            emit_മൂല്യം_List(fl, cpVarp, VCMethod::COVERGROUP_മൂല്യം_PATTERNS, lists.m_patterns);
            emit_മൂല്യം_List(fl, cpVarp, VCMethod::COVERGROUP_മൂല്യം_TRANSITIONS, lists.m_transitions);
            വേണ്ടി (AstCoverBin* സ്ഥിരം binp : sizedBins) {
                generateConstructedBins(coverpointp, binp, cpVarp, exprp);
            }
            m_constructorp->addStmtsp(
                itemCall(fl, cpVarp, VCMethod::COVERGROUP_മൂല്യം_FINALIZE)->makeStmt());
        }
        എങ്കിൽ (v3Global.opt.coverage()) {
            സ്ഥിരം std::string page
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
    ശൂന്യം generate_Multi_മൂല്യം_Transition_Code(AstCoverpoint* coverpointp, AstCoverBin* binp,
                                          Ast_കണ്ണി_വ്യഞ്ജകം* exprp, സ്ഥിരം ConvBinTarget& tgt,
                                          സ്ഥിരം std::vector<AstCoverTransItem*>& ഇനങ്ങൾ) {
        UINFO(4, "    Generating multi-value transition state machine for: " << binp->name());
        UINFO(4, "      Sequence length: " << ഇനങ്ങൾ.size() << " items");

        // Create state position variable
        AstVar* സ്ഥിരം stateVarp = createSequenceStateVar(coverpointp, binp);

        // Build case statement with N cases (one for each state 0 to N-1)
        // State 0: Not started, looking for first item
        // State 1 to N-1: In progress, looking for next item

        AstCase* സ്ഥിരം casep
            = പുതിയ AstCase{binp->fileline(), VCaseType::CT_CASE,
                          പുതിയ AstVarRef{stateVarp->fileline(), stateVarp, VAccess::READ}, nullptr};

        // Generate each case item in the switch statement
        വേണ്ടി (size_t state = 0; state < ഇനങ്ങൾ.size(); ++state) {
            AstCaseItem* caseItemp = generateTransitionStateCase(coverpointp, binp, exprp, tgt,
                                                                 stateVarp, ഇനങ്ങൾ, state);
            casep->addItemsp(caseItemp);
        }

        // Add default case (reset to state 0) to prevent CASEINCOMPLETE warnings,
        // since the state variable is wider than the number of valid states.
        AstCaseItem* സ്ഥിരം defaultItemp = പുതിയ AstCaseItem{
            binp->fileline(), nullptr,
            പുതിയ AstAssign{binp->fileline(),
                          പുതിയ AstVarRef{binp->fileline(), stateVarp, VAccess::WRITE},
                          പുതിയ AstConst{binp->fileline(), AstConst::Widthed_മൂല്യം{}, 8, 0}}};
        casep->addItemsp(defaultItemp);

        addSampleStmt(casep);
        UINFO(4, "      Successfully added multi-value transition state machine");
    }

    // Generate code for a single state in the transition state machine
    // Returns the case item for this state
    AstCaseItem* generateTransitionStateCase(AstCoverpoint* coverpointp, AstCoverBin* binp,
                                             Ast_കണ്ണി_വ്യഞ്ജകം* exprp, സ്ഥിരം ConvBinTarget& tgt,
                                             AstVar* stateVarp,
                                             സ്ഥിരം std::vector<AstCoverTransItem*>& ഇനങ്ങൾ,
                                             size_t state) {
        File_വരി* സ്ഥിരം fl = binp->fileline();

        // Build condition for current value matching expected item at this state
        Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം matchCondp = buildTransitionItemCondition(ഇനങ്ങൾ[state], exprp);

        Ast_കണ്ണി_Stmt* matchActionp = nullptr;

        എങ്കിൽ (state == ഇനങ്ങൾ.size() - 1) {
            // Last state: sequence complete!  Record the hit in the runtime VlCoverpoint.
            matchActionp = makeRuntimeBinHit(fl, tgt);

            // For illegal_bins, add error message
            എങ്കിൽ (binp->binsType() == VCoverBinsType::BINS_ILLEGAL) {
                സ്ഥിരം string err_സന്ദേശം = "Illegal transition bin " + binp->prettyNameQ()
                                      + " hit in coverpoint " + coverpointp->prettyNameQ();
                matchActionp = matchActionp->addNext(makeIllegalBinAction(fl, err_സന്ദേശം));
            }

            // Reset state to 0
            matchActionp = matchActionp->addNext(
                പുതിയ AstAssign{fl, പുതിയ AstVarRef{fl, stateVarp, VAccess::WRITE},
                              പുതിയ AstConst{fl, AstConst::Widthed_മൂല്യം{}, 8, 0}});
        } അല്ലെങ്കിൽ {
            // Intermediate state: advance to next state
            matchActionp = പുതിയ AstAssign{
                fl, പുതിയ AstVarRef{fl, stateVarp, VAccess::WRITE},
                പുതിയ AstConst{fl, AstConst::Widthed_മൂല്യം{}, 8, static_cast<uint32_t>(state + 1)}};
        }

        // Build restart logic: check if current value matches first item
        // If so, restart sequence from state 1 (even if we're in middle of sequence)
        Ast_കണ്ണി_Stmt* noMatchActionp = nullptr;
        എങ്കിൽ (state > 0) {
            // Check if current value matches first item (restart condition)
            Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം restartCondp = buildTransitionItemCondition(ഇനങ്ങൾ[0], exprp);

            UASSERT_OBJ(restartCondp, ഇനങ്ങൾ[0],
                        "buildTransitionItemCondition returned nullptr for restart");

            // Restart to state 1
            Ast_കണ്ണി_Stmt* restartActionp
                = പുതിയ AstAssign{fl, പുതിയ AstVarRef{fl, stateVarp, VAccess::WRITE},
                                പുതിയ AstConst{fl, AstConst::Widthed_മൂല്യം{}, 8, 1}};

            // Reset to state 0 (else branch)
            Ast_കണ്ണി_Stmt* resetActionp
                = പുതിയ AstAssign{fl, പുതിയ AstVarRef{fl, stateVarp, VAccess::WRITE},
                                പുതിയ AstConst{fl, AstConst::Widthed_മൂല്യം{}, 8, 0}};

            noMatchActionp = പുതിയ AstIf{fl, restartCondp, restartActionp, resetActionp};
        }
        // For state 0, no action needed if no match (stay in state 0)

        // Combine into if-else
        Ast_കണ്ണി_Stmt* സ്ഥിരം stmtp = പുതിയ AstIf{fl, matchCondp, matchActionp, noMatchActionp};

        // Create case item for this state value
        AstCaseItem* സ്ഥിരം caseItemp = പുതിയ AstCaseItem{
            fl, പുതിയ AstConst{fl, AstConst::Widthed_മൂല്യം{}, 8, static_cast<uint32_t>(state)},
            stmtp};

        തിരികെ caseItemp;
    }

    // Create: $error(msg); $stop;  Used when an illegal bin is hit.
    Ast_കണ്ണി_Stmt* makeIllegalBinAction(File_വരി* fl, സ്ഥിരം string& err_സന്ദേശം) {
        AstDisplay* സ്ഥിരം errorp
            = പുതിയ AstDisplay{fl, VDisplayType::DT_ERROR, err_സന്ദേശം, nullptr, nullptr};
        errorp->fmtp()->timeunit(m_covergroupp->timeunit());
        static_cast<Ast_കണ്ണി*>(errorp)->addNext(പുതിയ AstStop{fl, സത്യം});
        തിരികെ errorp;
    }

    // Preserve the coverpoint's width and signedness after V3Width.
    സ്ഥിതം AstConst* new_മൂല്യം_Const(File_വരി* fl, സ്ഥിരം V3_സംഖ്യ& മൂല്യം, സ്ഥിരം Ast_കണ്ണി_വ്യഞ്ജകം* exprp) {
        V3_സംഖ്യ narrowed{fl, exprp->width(), 0};
        narrowed.opAssign(മൂല്യം);
        AstConst* സ്ഥിരം constp = പുതിയ AstConst{fl, narrowed};
        constp->dtypeFrom(exprp);
        തിരികെ constp;
    }

    // A real copy of a real or integral range bound, for comparing with a real coverpoint.
    സ്ഥിതം AstConst* newRealConst(AstConst* constp) {
        എങ്കിൽ (constp->അക്കം().isDouble()) തിരികെ constp->clone_വൃക്ഷം(അസത്യം);
        V3_സംഖ്യ real{&constp->അക്കം(), 64};
        real.opIToRD(constp->അക്കം(), constp->isSigned());
        തിരികെ പുതിയ AstConst{constp->fileline(), real};
    }

    // Clone a constant node, widening to targetWidth if needed (zero-extend).
    // Used to ensure comparisons use matching widths after V3Width has run.
    സ്ഥിതം AstConst* widenConst(File_വരി* fl, AstConst* constp, പൂർണ്ണസംഖ്യ targetWidth) {
        എങ്കിൽ (constp->width() == targetWidth) തിരികെ constp->clone_വൃക്ഷം(അസത്യം);
        V3_സംഖ്യ അക്കം{fl, targetWidth, 0};
        അക്കം.opAssign(constp->അക്കം());
        തിരികെ പുതിയ AstConst{fl, അക്കം};
    }

    // Build a range condition: minp <= exprp <= maxp.
    // Uses signed comparisons if exprp is signed; omits trivially-true domain bounds.
    // All arguments are non-owning; clones exprp/minp/maxp as needed.
    Ast_കണ്ണി_വ്യഞ്ജകം* makeRangeCondition(File_വരി* fl, Ast_കണ്ണി_വ്യഞ്ജകം* exprp, Ast_കണ്ണി_വ്യഞ്ജകം* minp,
                                    Ast_കണ്ണി_വ്യഞ്ജകം* maxp) {
        സ്ഥിരം പൂർണ്ണസംഖ്യ വ്യഞ്ജകം_Width = exprp->widthMin();
        AstConst* സ്ഥിരം minConstp = VN_AS(minp, Const);
        AstConst* സ്ഥിരം maxConstp = VN_AS(maxp, Const);
        എങ്കിൽ (exprp->isDouble()) {
            // A real coverpoint has no finite domain bounds to omit.
            തിരികെ പുതിയ AstAnd{fl,
                              പുതിയ AstGteD{fl, exprp->clone_വൃക്ഷം(അസത്യം), newRealConst(minConstp)},
                              പുതിയ AstLteD{fl, exprp->clone_വൃക്ഷം(അസത്യം), newRealConst(maxConstp)}};
        }
        // Widen constants to match expression width so post-V3Width nodes use correct macros
        AstConst* സ്ഥിരം minWidep = widenConst(fl, minConstp, വ്യഞ്ജകം_Width);
        AstConst* സ്ഥിരം maxWidep = widenConst(fl, maxConstp, വ്യഞ്ജകം_Width);
        V3_സംഖ്യ minimum{fl, വ്യഞ്ജകം_Width, 0};
        V3_സംഖ്യ maximum{fl, വ്യഞ്ജകം_Width, 0};
        maximum.setAllBits1();
        എങ്കിൽ (exprp->isSigned()) {
            minimum.setBit(വ്യഞ്ജകം_Width - 1, 1);
            maximum.setBit(വ്യഞ്ജകം_Width - 1, 0);
        }
        Ast_കണ്ണി_വ്യഞ്ജകം* lowerp = nullptr;
        Ast_കണ്ണി_വ്യഞ്ജകം* upperp = nullptr;
        എങ്കിൽ (minWidep->അക്കം().isCaseEq(minimum)) {
            VL_DO_DANGLING(തള്ളുക_Deletep(minWidep), minWidep);
        } അല്ലെങ്കിൽ {
            lowerp = exprp->isSigned() ? static_cast<Ast_കണ്ണി_വ്യഞ്ജകം*>(
                                             പുതിയ AstGteS{fl, exprp->clone_വൃക്ഷം(അസത്യം), minWidep})
                                       : static_cast<Ast_കണ്ണി_വ്യഞ്ജകം*>(
                                             പുതിയ AstGte{fl, exprp->clone_വൃക്ഷം(അസത്യം), minWidep});
        }
        എങ്കിൽ (maxWidep->അക്കം().isCaseEq(maximum)) {
            VL_DO_DANGLING(തള്ളുക_Deletep(maxWidep), maxWidep);
        } അല്ലെങ്കിൽ {
            upperp = exprp->isSigned() ? static_cast<Ast_കണ്ണി_വ്യഞ്ജകം*>(
                                             പുതിയ AstLteS{fl, exprp->clone_വൃക്ഷം(അസത്യം), maxWidep})
                                       : static_cast<Ast_കണ്ണി_വ്യഞ്ജകം*>(
                                             പുതിയ AstLte{fl, exprp->clone_വൃക്ഷം(അസത്യം), maxWidep});
        }
        എങ്കിൽ (lowerp && upperp) തിരികെ പുതിയ AstAnd{fl, lowerp, upperp};
        എങ്കിൽ (lowerp) തിരികെ lowerp;
        എങ്കിൽ (upperp) തിരികെ upperp;
        തിരികെ പുതിയ AstConst{fl, AstConst::BitTrue{}};
    }

    // Build a one-sided comparison for an open-ended bin range whose other bound is '$'.
    // '$' denotes the coverpoint domain extreme, so {[lo:$]} == (expr >= lo) and
    // {[$:hi]} == (expr <= hi).
    Ast_കണ്ണി_വ്യഞ്ജകം* makeOpenRangeCondition(File_വരി* fl, Ast_കണ്ണി_വ്യഞ്ജകം* exprp, AstConst* boundp,
                                        തർക്കം isLowerBound) {
        AstConst* സ്ഥിരം widep = widenConst(fl, boundp, exprp->widthMin());
        എങ്കിൽ (isLowerBound) {
            എങ്കിൽ (exprp->isSigned()) തിരികെ പുതിയ AstGteS{fl, exprp->clone_വൃക്ഷം(അസത്യം), widep};
            തിരികെ പുതിയ AstGte{fl, exprp->clone_വൃക്ഷം(അസത്യം), widep};
        }
        എങ്കിൽ (exprp->isSigned()) തിരികെ പുതിയ AstLteS{fl, exprp->clone_വൃക്ഷം(അസത്യം), widep};
        തിരികെ പുതിയ AstLte{fl, exprp->clone_വൃക്ഷം(അസത്യം), widep};
    }

    // Build condition for a single transition item.
    // Returns expression that checks if exprp matches the item's value/range list.
    // Overload for when the expression is a variable read -- creates and manages the VarRef
    // internally, so callers don't need to construct a temporary node.
    Ast_കണ്ണി_വ്യഞ്ജകം* buildTransitionItemCondition(AstCoverTransItem* itemp, AstVar* varp) {
        Ast_കണ്ണി_വ്യഞ്ജകം* varRefp = പുതിയ AstVarRef{varp->fileline(), varp, VAccess::READ};
        Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം condp = buildTransitionItemCondition(itemp, varRefp);
        VL_DO_DANGLING(തള്ളുക_Deletep(varRefp), varRefp);
        തിരികെ condp;
    }

    // Non-owning: exprp is cloned internally; caller retains ownership of exprp.
    Ast_കണ്ണി_വ്യഞ്ജകം* buildTransitionItemCondition(AstCoverTransItem* itemp, Ast_കണ്ണി_വ്യഞ്ജകം* exprp) {
        Ast_കണ്ണി_വ്യഞ്ജകം* condp = nullptr;

        വേണ്ടി (Ast_കണ്ണി* valp = itemp->valuesp(); valp; valp = valp->nextp()) {
            Ast_കണ്ണി_വ്യഞ്ജകം* singleCondp = nullptr;
            valp = V3Const::constifyEdit(valp);
            AstConst* സ്ഥിരം constp = VN_CAST(valp, Const);
            എങ്കിൽ (!constp) {
                valp->v3error("Non-constant expression in transition bin; "
                              "values must be constants (IEEE 1800-2023 19.5)");
                തിരികെ പുതിയ AstConst{valp->fileline(), AstConst::BitFalseErroring{}};
            }
            singleCondp
                = പുതിയ AstEq{constp->fileline(), exprp->clone_വൃക്ഷം(അസത്യം), constp->clone_വൃക്ഷം(അസത്യം)};
            എങ്കിൽ (condp) {
                condp = പുതിയ AstOr{itemp->fileline(), condp, singleCondp};
            } അല്ലെങ്കിൽ {
                condp = singleCondp;
            }
        }

        തിരികെ condp;
    }

    // Generate code for a single transition sequence (used by both regular and array bins)
    ശൂന്യം generateSingleTransitionCode(AstCoverpoint* coverpointp, AstCoverBin* binp,
                                      Ast_കണ്ണി_വ്യഞ്ജകം* exprp, സ്ഥിരം ConvBinTarget& tgt,
                                      AstCoverTransSet* transSetp) {
        UINFO(4, "      Generating code for transition sequence");

        // Get or create previous value variable
        AstVar* സ്ഥിരം prevVarp = create_Prev_മൂല്യം_Var(coverpointp, exprp);

        UASSERT_OBJ(
            transSetp, binp,
            "Transition bin has no transition set (transp() was checked before calling this)");

        // Get transition items (the sequence: item1 => item2 => item3)
        std::vector<AstCoverTransItem*> ഇനങ്ങൾ;
        വേണ്ടി (Ast_കണ്ണി* itemp = transSetp->itemsp(); itemp; itemp = itemp->nextp())
            ഇനങ്ങൾ.തള്ളുക_back(VN_AS(itemp, CoverTransItem));

        എങ്കിൽ (ഇനങ്ങൾ.empty()) {
            binp->v3error("Transition set without items");
            തിരികെ;
        }

        എങ്കിൽ (ഇനങ്ങൾ.size() == 1) {
            // Single item transition not valid (need at least 2 values for =>)
            binp->v3error("Transition requires at least two values");
            തിരികെ;
        } അല്ലെങ്കിൽ എങ്കിൽ (ഇനങ്ങൾ.size() == 2) {
            // Simple two-value transition: (val1 => val2)
            // Use optimized direct comparison (no state machine needed)
            Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം cond1p = buildTransitionItemCondition(ഇനങ്ങൾ[0], prevVarp);
            Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം cond2p = buildTransitionItemCondition(ഇനങ്ങൾ[1], exprp);

            // Combine: prev matches val1 AND current matches val2
            Ast_കണ്ണി_വ്യഞ്ജകം* fullCondp = പുതിയ AstAnd{binp->fileline(), cond1p, cond2p};

            addConvTransHitIf(coverpointp, binp, tgt, fullCondp);

            UINFO(4, "        Successfully added 2-value transition if statement");
        } അല്ലെങ്കിൽ {
            // Multi-value sequence (a => b => c => ...)
            // Use state machine to track position in sequence
            generate_Multi_മൂല്യം_Transition_Code(coverpointp, binp, exprp, tgt, ഇനങ്ങൾ);
        }
    }

    // Append a "{ VlCoverpoint* __Vcx_cps[] = {cp0, cp1, ...}; <call> }" statement.  The brace
    // and the temporary array stay literal text -- a CMethodHard is one call, not a block --
    // but callp itself carries the member, method and '->'.  Construction only: init() copies
    // the array into the cross, so sample() reads it from there and needs no array at all.
    AstCStmt* makeCrossCpsCall(File_വരി* fl, സ്ഥിരം std::vector<AstVar*>& cpVars,
                               AstCMethodHard* callp) {
        AstCStmt* സ്ഥിരം cs = പുതിയ AstCStmt{fl};
        cs->add("{ VlCoverpoint* __Vcx_cps[] = {");
        വേണ്ടി (size_t d = 0; d < cpVars.size(); ++d) {
            എങ്കിൽ (d != 0) cs->add(", ");
            cs->add(memberRef(fl, cpVars[d]));
        }
        cs->add("}; ");
        cs->add(callp);
        cs->add("; }");
        തിരികെ cs;
    }

    // Assign the per-bin flags individually: one-bit SV results have integer C++ storage types,
    // which may narrow in a bool initializer list but convert implicitly in assignments.
    AstCStmt* makeCrossIffsCall(File_വരി* fl, സ്ഥിരം std::vector<AstCoverCrossBin*>& bins,
                                AstCMethodHard* callp) {
        AstCStmt* സ്ഥിരം cs = പുതിയ AstCStmt{fl};
        cs->add("{ bool __Vcx_iffs[" + cvtToStr(bins.size()) + "]; ");
        വേണ്ടി (size_t i = 0; i < bins.size(); ++i) {
            സ്ഥിരം AstCoverCrossBin* സ്ഥിരം binp = bins[i];
            cs->add("__Vcx_iffs[" + cvtToStr(i) + "] = ");
            cs->add(binp->iffp() ? binp->iffp()->clone_വൃക്ഷം(അസത്യം)
                                 : പുതിയ AstConst{fl, AstConst::BitTrue{}});
            cs->add("; ");
        }
        cs->add(callp);
        cs->add("; }");
        തിരികെ cs;
    }

    using CrossSelection = std::vector<uint64_t>;
    ഘടന ResolvedCrossBin final {
        AstCoverCrossBin* binp;
        CrossSelection selection;
    };
    ഘടന CrossLayout final {
        uint32_t tuples = 0;
        uint32_t autoBins = 0;
        uint64_t bin_വാക്കുകൾ = 0;
        തർക്കം valid = സത്യം;
        std::vector<ResolvedCrossBin> bins;
    };
    ഘടന CrossSelectionContext final {
        AstCoverCross* crossp;  // Cross whose tuple space is being selected
        സ്ഥിരം std::vector<AstVar*>& cpVars;  // Feeding coverpoints in dimension order
        സ്ഥിരം std::map<std::string, uint32_t>& dimensions;  // Coverpoint name -> dimension
        uint32_t tuples;  // Size of the Cartesian product
        std::vector<uint32_t> strides;  // Flat-index stride per dimension
        തർക്കം valid = സത്യം;  // False if this explicit bin cannot be implemented
    };
    ഘടന CrossBinsofTarget final {
        സ്ഥിരം CoverpointBins* m_binsp = nullptr;  // Declared bins; null after a reported error
        uint32_t m_dimension = 0;  // Coverpoint index within the cross
        uint32_t m_first = 0;  // First declared normal bin in the selected span
        uint32_t m_എണ്ണം = 0;  // Number of declared normal bins in the selected span
        uint32_t m_declaredFirst = 0;  // First runtime bin index of the selected span
        uint32_t m_declaredEnd = UINT32_MAX;  // One past the last runtime bin index
        int32_t m_sized = -1;  // Index of the sized array holding the selected bins; or -1
    };
    ഘടന Cross_മൂല്യം_Range final {
        V3_സംഖ്യ lo;  // Inclusive lower bound, sign-extended to the comparison width
        V3_സംഖ്യ hi;  // Inclusive upper bound
        V3_സംഖ്യ pattern;  // Allowed bit values; all X for an ordinary interval
        തർക്കം wildcard = അസത്യം;  // A wildcard singleton rather than an exact four-state value

        Cross_മൂല്യം_Range(Ast_കണ്ണി* nodep, പൂർണ്ണസംഖ്യ width)
            : lo{nodep, width}
            , hi{nodep, width}
            , pattern{nodep, width} {
            pattern.setAllBitsX();
        }
    };

    സ്ഥിതം std::vector<Ast_കണ്ണി*> cross_Bin_മൂല്യങ്ങൾ(സ്ഥിരം Cross_Bin_മൂല്യങ്ങൾ& bin) {
        എങ്കിൽ (bin.valuep) തിരികെ {bin.valuep};
        std::vector<Ast_കണ്ണി*> മൂല്യങ്ങൾ;
        എങ്കിൽ (bin.binp->transp()) {
            // IEEE 1800-2023 19.6.1: binsof uses the last value of each transition.
            വേണ്ടി (Ast_കണ്ണി* setp = bin.binp->transp(); setp; setp = setp->nextp()) {
                AstCoverTransItem* lastp = VN_AS(setp, CoverTransSet)->itemsp();
                ആകുമ്പോൾ (lastp->nextp()) lastp = VN_AS(lastp->nextp(), CoverTransItem);
                വേണ്ടി (Ast_കണ്ണി* valuep = lastp->valuesp(); valuep; valuep = valuep->nextp()) {
                    മൂല്യങ്ങൾ.തള്ളുക_back(valuep);
                }
            }
        } അല്ലെങ്കിൽ {
            വേണ്ടി (Ast_കണ്ണി* valuep = bin.binp->rangesp(); valuep; valuep = valuep->nextp()) {
                മൂല്യങ്ങൾ.തള്ളുക_back(valuep);
            }
        }
        തിരികെ മൂല്യങ്ങൾ;
    }

    // The values of the element-th bin of a run, sign-extended to 'width'
    സ്ഥിതം Cross_മൂല്യം_Range runBinRange(Ast_കണ്ണി* nodep, സ്ഥിരം BinRun& run, uint32_t element,
                                       പൂർണ്ണസംഖ്യ width) {
        സ്ഥിരം പൂർണ്ണസംഖ്യ runw = run.m_lo.width();
        V3_സംഖ്യ offset{nodep, runw};
        offset.opMul(run.m_stride, V3_സംഖ്യ{nodep, runw, element});
        V3_സംഖ്യ lo{nodep, runw};
        lo.opAdd(run.m_lo, offset);
        V3_സംഖ്യ hi = run.m_hi;
        എങ്കിൽ (element + 1 < run.m_എണ്ണം) {
            offset.opSub(run.m_stride, V3_സംഖ്യ{nodep, runw, 1});
            hi.opAdd(lo, offset);
        }
        Cross_മൂല്യം_Range range{nodep, width};
        range.lo.opExtendS(lo, runw);
        range.hi.opExtendS(hi, runw);
        തിരികെ range;
    }

    സ്ഥിതം പൂർണ്ണസംഖ്യ crossRangeWidth(Ast_കണ്ണി* nodep) {
        എങ്കിൽ (സ്ഥിരം AstInsideRange* സ്ഥിരം rangep = VN_CAST(nodep, InsideRange)) {
            തിരികെ std::max(rangep->lhsp()->width(), rangep->rhsp()->width());
        }
        തിരികെ nodep->width();
    }

    സ്ഥിതം തർക്കം cross_മൂല്യം_Less(സ്ഥിരം V3_സംഖ്യ& lhs, സ്ഥിരം V3_സംഖ്യ& rhs) {
        V3_സംഖ്യ result{&lhs};
        തിരികെ !result.opLtS(lhs, rhs).isEqZero();
    }

    // Round a real bound inward to the nearest coverpoint value (IEEE 1800-2023 19.5.7).  Sets
    // 'empty' if the domain has no value on the bound's side.
    സ്ഥിതം ശൂന്യം crossRealBound(സ്ഥിരം AstConst* constp, Ast_കണ്ണി_വ്യഞ്ജകം* exprp, തർക്കം upper,
                               സ്ഥിരം Cross_മൂല്യം_Range& domain, V3_സംഖ്യ& result, തർക്കം& empty) {
        സ്ഥിരം ഇരട്ട മൂല്യം = constp->അക്കം().toDouble();
        സ്ഥിരം ഇരട്ട bound = upper ? std::floor(മൂല്യം) : std::ceil(മൂല്യം);
        // Powers of two are exact, so the integral bound compares exactly with the domain edges.
        സ്ഥിരം ഇരട്ട limit
            = std::ldexp(1.0, exprp->isSigned() ? exprp->width() - 1 : exprp->width());
        സ്ഥിരം ഇരട്ട minimum = exprp->isSigned() ? -limit : 0.0;
        എങ്കിൽ (std::isnan(bound) || (upper ? bound < minimum : bound >= limit)) {
            empty = സത്യം;
            result = domain.lo;
        } അല്ലെങ്കിൽ എങ്കിൽ (bound < minimum) {
            result = domain.lo;
        } അല്ലെങ്കിൽ എങ്കിൽ (bound >= limit) {
            result = domain.hi;
        } അല്ലെങ്കിൽ {
            V3_സംഖ്യ real{&result, 64};
            real.setDouble(bound);
            result.opRToIRoundS(real);
        }
    }

    സ്ഥിതം തർക്കം crossRangeBound(Ast_കണ്ണി* nodep, Ast_കണ്ണി_വ്യഞ്ജകം* exprp, തർക്കം upper, തർക്കം bin_മൂല്യം,
                                സ്ഥിരം Cross_മൂല്യം_Range& domain, V3_സംഖ്യ& result, തർക്കം& empty) {
        എങ്കിൽ (VN_IS(nodep, Unbounded)) {
            V3_സംഖ്യ limit{nodep, exprp->width()};
            എങ്കിൽ (upper) limit.setAllBits1();
            എങ്കിൽ (exprp->isSigned()) {
                limit.setBit(exprp->width() - 1, !upper);
                result.opExtendS(limit, limit.width());
            } അല്ലെങ്കിൽ {
                result.opAssign(limit);
            }
            തിരികെ സത്യം;
        }
        സ്ഥിരം AstConst* സ്ഥിരം constp = VN_CAST(nodep, Const);
        എങ്കിൽ (!constp) തിരികെ അസത്യം;
        എങ്കിൽ (constp->അക്കം().isDouble()) {
            crossRealBound(constp, exprp, upper, domain, result, empty);
            തിരികെ സത്യം;
        }
        എങ്കിൽ (constp->അക്കം().isString()) തിരികെ അസത്യം;
        എങ്കിൽ (bin_മൂല്യം && exprp->isSigned() && constp->width() <= exprp->width()) {
            // Bin bit patterns use the coverpoint's effective type (IEEE 1800-2023 19.5.7).
            // Wider values and intersect filters retain their values for domain clipping.
            V3_സംഖ്യ മൂല്യം{nodep, exprp->width()};
            എങ്കിൽ (constp->isSigned()) {
                മൂല്യം.opExtendS(constp->അക്കം(), constp->width());
            } അല്ലെങ്കിൽ {
                മൂല്യം.opAssign(constp->അക്കം());
            }
            result.opExtendS(മൂല്യം, മൂല്യം.width());
        } അല്ലെങ്കിൽ എങ്കിൽ (constp->isSigned()) {
            result.opExtendS(constp->അക്കം(), constp->width());
        } അല്ലെങ്കിൽ {
            result.opAssign(constp->അക്കം());
        }
        തിരികെ സത്യം;
    }

    സ്ഥിതം Cross_മൂല്യം_Range cross_മൂല്യം_Domain(Ast_കണ്ണി* nodep, പൂർണ്ണസംഖ്യ മൂല്യം_Width, തർക്കം isSigned,
                                            പൂർണ്ണസംഖ്യ width) {
        Cross_മൂല്യം_Range domain{nodep, width};
        V3_സംഖ്യ lo{nodep, മൂല്യം_Width};
        V3_സംഖ്യ hi{nodep, മൂല്യം_Width};
        hi.setAllBits1();
        എങ്കിൽ (isSigned) {
            lo.setBit(മൂല്യം_Width - 1, 1);
            hi.setBit(മൂല്യം_Width - 1, 0);
            domain.lo.opExtendS(lo, മൂല്യം_Width);
            domain.hi.opExtendS(hi, മൂല്യം_Width);
        } അല്ലെങ്കിൽ {
            domain.lo.opAssign(lo);
            domain.hi.opAssign(hi);
        }
        തിരികെ domain;
    }

    സ്ഥിതം ശൂന്യം intersectCrossRange(Cross_മൂല്യം_Range& range, സ്ഥിരം Cross_മൂല്യം_Range& other) {
        എങ്കിൽ (cross_മൂല്യം_Less(range.lo, other.lo)) range.lo = other.lo;
        എങ്കിൽ (cross_മൂല്യം_Less(other.hi, range.hi)) range.hi = other.hi;
    }

    സ്ഥിതം തർക്കം cross_മൂല്യം_Range(Ast_കണ്ണി* nodep, Ast_കണ്ണി_വ്യഞ്ജകം* exprp, തർക്കം bin_മൂല്യം, തർക്കം wildcard,
                                സ്ഥിരം Cross_മൂല്യം_Range& domain, Cross_മൂല്യം_Range& range) {
        തർക്കം empty = അസത്യം;
        സ്ഥിരം AstConst* സ്ഥിരം constp = VN_CAST(nodep, Const);
        എങ്കിൽ (സ്ഥിരം AstInsideRange* സ്ഥിരം rangep = VN_CAST(nodep, InsideRange)) {
            എങ്കിൽ (!crossRangeBound(rangep->lhsp(), exprp, അസത്യം, bin_മൂല്യം, domain, range.lo, empty)
                || !crossRangeBound(rangep->rhsp(), exprp, സത്യം, bin_മൂല്യം, domain, range.hi, empty)
                || range.lo.isFourState() || range.hi.isFourState()) {
                തിരികെ അസത്യം;
            }
        } അല്ലെങ്കിൽ എങ്കിൽ (constp && constp->അക്കം().isDouble()) {
            // A real value participates only if integral; it has no wildcard bits.
            crossRealBound(constp, exprp, അസത്യം, domain, range.lo, empty);
            crossRealBound(constp, exprp, സത്യം, domain, range.hi, empty);
        } അല്ലെങ്കിൽ {
            എങ്കിൽ (!crossRangeBound(nodep, exprp, അസത്യം, bin_മൂല്യം, domain, range.lo, empty)) {
                തിരികെ അസത്യം;
            }
            എങ്കിൽ (bin_മൂല്യം && exprp->isSigned() && constp && !constp->isSigned()
                && constp->width() > exprp->width()) {
                തർക്കം representable = സത്യം;
                വേണ്ടി (പൂർണ്ണസംഖ്യ bit = exprp->width(); bit < constp->width(); ++bit) {
                    representable &= !constp->അക്കം().bitIs1(bit);
                }
                എങ്കിൽ (representable) {
                    V3_സംഖ്യ മൂല്യം{nodep, exprp->width()};
                    മൂല്യം.opAssign(constp->അക്കം());
                    range.lo.opExtendS(മൂല്യം, മൂല്യം.width());
                }
            }
            range.hi = range.lo;
            range.wildcard = wildcard;
            എങ്കിൽ (wildcard) {
                range.pattern = range.lo;
                range.lo = domain.lo;
                range.hi = domain.hi;
                എങ്കിൽ (constp && constp->isSigned()) {
                    // Replicated X sign bits are correlated, not independent wildcards.
                    // The source domain preserves expansion-before-casting (19.5.7).
                    intersectCrossRange(
                        range, cross_മൂല്യം_Domain(nodep, constp->width(), സത്യം, domain.lo.width()));
                }
            }
        }
        എങ്കിൽ (empty) {
            range.lo = domain.hi;
            range.hi = domain.lo;
            തിരികെ സത്യം;
        }
        എങ്കിൽ (!range.lo.isFourState()) intersectCrossRange(range, domain);
        എങ്കിൽ (range.wildcard && exprp->isSigned()) {
            സ്ഥിരം പൂർണ്ണസംഖ്യ sign = exprp->width() - 1;
            എങ്കിൽ (constp && !constp->isSigned() && constp->width() > exprp->width()) {
                // Unsigned equality preserves every target bit pattern if discarded bits are zero.
                വേണ്ടി (പൂർണ്ണസംഖ്യ bit = exprp->width(); bit < constp->width(); ++bit) {
                    എങ്കിൽ (constp->അക്കം().bitIs1(bit)) {
                        range.lo = domain.hi;
                        range.hi = domain.lo;
                        തിരികെ സത്യം;
                    }
                }
                വേണ്ടി (പൂർണ്ണസംഖ്യ bit = exprp->width(); bit < range.pattern.width(); ++bit) {
                    എങ്കിൽ (range.pattern.bitIsXZ(sign))
                        range.pattern.setBit(bit, 'x');
                    അല്ലെങ്കിൽ
                        range.pattern.setBit(bit, range.pattern.bitIs1(sign));
                }
                തിരികെ സത്യം;
            }
            // Discarded high bits constrain the target sign, rather than becoming don't-cares.
            വേണ്ടി (പൂർണ്ണസംഖ്യ bit = exprp->width(); bit < range.pattern.width(); ++bit) {
                എങ്കിൽ (range.pattern.bitIsXZ(bit)) തുടരുക;
                സ്ഥിരം തർക്കം മൂല്യം = range.pattern.bitIs1(bit);
                എങ്കിൽ (!range.pattern.bitIsXZ(sign) && range.pattern.bitIs1(sign) != മൂല്യം) {
                    range.lo = domain.hi;
                    range.hi = domain.lo;
                    നിർത്തുക;
                }
                range.pattern.setBit(sign, മൂല്യം);
            }
        }
        തിരികെ സത്യം;
    }

    എണ്ണൽ CrossRangeState : uint8_t {
        CROSS_INSIDE_BOUNDS = 0,  // Prefix is strictly inside the interval
        CROSS_AT_LOWER = 1,
        CROSS_AT_UPPER = 2,
        CROSS_AT_BOUNDS = CROSS_AT_LOWER | CROSS_AT_UPPER,
        CROSS_NO_MATCH = 4  // Prefix cannot match the interval/pattern
    };

    സ്ഥിതം CrossRangeState crossRangeStep(സ്ഥിരം Cross_മൂല്യം_Range& range, CrossRangeState state,
                                          പൂർണ്ണസംഖ്യ bit, പൂർണ്ണസംഖ്യ മൂല്യം) {
        എങ്കിൽ (state == CROSS_NO_MATCH) തിരികെ CROSS_NO_MATCH;
        // Flipping the sign bit makes signed order lexicographic.
        സ്ഥിരം തർക്കം sign = bit == range.pattern.width() - 1;
        എങ്കിൽ (!range.pattern.bitIsXZ(bit) && മൂല്യം != (range.pattern.bitIs1(bit) ^ sign)) {
            തിരികെ CROSS_NO_MATCH;
        }
        സ്ഥിരം പൂർണ്ണസംഖ്യ താഴെയുള്ള = range.lo.bitIs1(bit) ^ sign;
        സ്ഥിരം പൂർണ്ണസംഖ്യ മുകളിലെ = range.hi.bitIs1(bit) ^ sign;
        എങ്കിൽ (((state & CROSS_AT_LOWER) && മൂല്യം < താഴെയുള്ള)
            || ((state & CROSS_AT_UPPER) && മൂല്യം > മുകളിലെ)) {
            തിരികെ CROSS_NO_MATCH;
        }
        തിരികെ static_cast<CrossRangeState>(
            ((state & CROSS_AT_LOWER) && മൂല്യം == താഴെയുള്ള ? CROSS_AT_LOWER : CROSS_INSIDE_BOUNDS)
            | ((state & CROSS_AT_UPPER) && മൂല്യം == മുകളിലെ ? CROSS_AT_UPPER : CROSS_INSIDE_BOUNDS));
    }

    സ്ഥിതം തർക്കം crossWildcardIntersects(സ്ഥിരം Cross_മൂല്യം_Range& range) {
        unsigned states = 1U << CROSS_AT_BOUNDS;
        വേണ്ടി (പൂർണ്ണസംഖ്യ bit = range.pattern.width() - 1; bit >= 0 && states; --bit) {
            unsigned next = 0;
            വേണ്ടി (സ്ഥിരം CrossRangeState state :
                 {CROSS_INSIDE_BOUNDS, CROSS_AT_LOWER, CROSS_AT_UPPER, CROSS_AT_BOUNDS}) {
                എങ്കിൽ (!(states & (1U << state))) തുടരുക;
                വേണ്ടി (പൂർണ്ണസംഖ്യ മൂല്യം = 0; മൂല്യം < 2; ++മൂല്യം) {
                    സ്ഥിരം CrossRangeState equal = crossRangeStep(range, state, bit, മൂല്യം);
                    എങ്കിൽ (equal != CROSS_NO_MATCH) next |= 1U << equal;
                }
            }
            states = next;
        }
        തിരികെ states != 0;
    }

    // The least value at least 'from' with the non-x bits of 'pattern', in unsigned order; false
    // if none.  A bit scan, which does not enumerate the pattern's x bits.
    സ്ഥിതം തർക്കം next_Pattern_മൂല്യം(സ്ഥിരം V3_സംഖ്യ& pattern, സ്ഥിരം V3_സംഖ്യ& from, V3_സംഖ്യ& result) {
        result.opAssign(from);
        പൂർണ്ണസംഖ്യ carry = -1;  // Lowest x bit above the bit scanned where 'from' has a 0
        വേണ്ടി (പൂർണ്ണസംഖ്യ bit = from.width() - 1; bit >= 0; --bit) {
            എങ്കിൽ (pattern.bitIsXZ(bit)) {
                എങ്കിൽ (!from.bitIs1(bit)) carry = bit;
                തുടരുക;
            }
            എങ്കിൽ (pattern.bitIs1(bit) == from.bitIs1(bit)) തുടരുക;
            എങ്കിൽ (from.bitIs1(bit)) {  // Only a greater prefix, an x bit above raised, can match
                എങ്കിൽ (carry < 0) തിരികെ അസത്യം;
                bit = carry;
            }
            // Then the least such value: the pattern's bits, with x bits of zero
            result.setBit(bit, 1);
            ആകുമ്പോൾ (--bit >= 0) result.setBit(bit, pattern.bitIs1(bit));
            തിരികെ സത്യം;
        }
        തിരികെ സത്യം;
    }

    // Append to 'runs' the maximal runs of consecutive values of 'range' that its pattern
    // matches, in value order, until there are more than 'limit'.  A match continues through
    // the pattern's trailing x bits only, which the next value's carry leaves.
    സ്ഥിതം ശൂന്യം crossRangeRuns(സ്ഥിരം Cross_മൂല്യം_Range& range, size_t limit,
                               std::vector<std::pair<V3_സംഖ്യ, V3_സംഖ്യ>>& runs) {
        // Values order signed, which is the unsigned order of values with the sign bit flipped
        സ്ഥിരം പൂർണ്ണസംഖ്യ sign = range.lo.width() - 1;
        സ്ഥിരം auto flipped = [sign](V3_സംഖ്യ മൂല്യം) {
            എങ്കിൽ (!മൂല്യം.bitIsXZ(sign)) മൂല്യം.setBit(sign, !മൂല്യം.bitIs1(sign));
            തിരികെ മൂല്യം;
        };
        സ്ഥിരം V3_സംഖ്യ pattern = flipped(range.pattern);
        സ്ഥിരം V3_സംഖ്യ hi = flipped(range.hi);
        സ്ഥിരം V3_സംഖ്യ one{&hi, hi.width(), 1};
        V3_സംഖ്യ trailing{&hi, hi.width()};
        വേണ്ടി (പൂർണ്ണസംഖ്യ bit = 0; bit <= sign && pattern.bitIsXZ(bit); ++bit) trailing.setBit(bit, 1);
        V3_സംഖ്യ from = flipped(range.lo);
        V3_സംഖ്യ first{&hi, hi.width()};
        V3_സംഖ്യ last{&hi, hi.width()};
        V3_സംഖ്യ less{&hi};
        ആകുമ്പോൾ (runs.size() <= limit && next_Pattern_മൂല്യം(pattern, from, first)
               && less.opLt(hi, first).isEqZero()) {
            last.opOr(first, trailing);
            എങ്കിൽ (!less.opLt(hi, last).isEqZero()) last = hi;
            runs.emplace_back(flipped(first), flipped(last));
            എങ്കിൽ (last.isCaseEq(hi)) നിർത്തുക;
            from.opAdd(last, one);
        }
    }

    // True if no coverpoint value participates in a resolved value or range.  A value with x
    // or z bits participates only as a wildcard pattern (IEEE 1800-2023 19.5.7).
    സ്ഥിതം തർക്കം crossRangeEmpty(സ്ഥിരം Cross_മൂല്യം_Range& range) {
        എങ്കിൽ (range.lo.isFourState()) തിരികെ സത്യം;
        തിരികെ cross_മൂല്യം_Less(range.hi, range.lo)
               || (range.wildcard && !crossWildcardIntersects(range));
    }

    // Comparison width that holds both the coverpoint's and a bin or intersect value's range.
    സ്ഥിതം പൂർണ്ണസംഖ്യ resolveWidth(Ast_കണ്ണി* nodep, സ്ഥിരം Ast_കണ്ണി_വ്യഞ്ജകം* exprp) {
        തിരികെ std::max(exprp->width(), crossRangeWidth(nodep)) + 1;
    }

    // Resolve a bin or intersect value to coverpoint values (IEEE 1800-2023 19.5.7), in the
    // width 'range' was created with.  False if the value is not a constant integral or real.
    സ്ഥിതം തർക്കം resolve_മൂല്യം(Ast_കണ്ണി* nodep, Ast_കണ്ണി_വ്യഞ്ജകം* exprp, തർക്കം bin_മൂല്യം, തർക്കം wildcard,
                             Cross_മൂല്യം_Range& range) {
        സ്ഥിരം Cross_മൂല്യം_Range domain
            = cross_മൂല്യം_Domain(nodep, exprp->width(), exprp->isSigned(), range.lo.width());
        തിരികെ cross_മൂല്യം_Range(nodep, exprp, bin_മൂല്യം, wildcard, domain, range);
    }

    സ്ഥിതം തർക്കം crossRangesIntersect(സ്ഥിരം Cross_മൂല്യം_Range& bin, സ്ഥിരം Cross_മൂല്യം_Range& filter) {
        // Values with x or z bits do not participate, even in an identical filter.
        എങ്കിൽ (bin.lo.isFourState() || filter.lo.isFourState()) തിരികെ അസത്യം;
        Cross_മൂല്യം_Range match = bin;
        intersectCrossRange(match, filter);
        തിരികെ !cross_മൂല്യം_Less(match.hi, match.lo)
               && (!bin.wildcard || crossWildcardIntersects(match));
    }

    സ്ഥിതം ശൂന്യം unsupportedCrossRange(AstCoverBinsof* selectp, തർക്കം& valid) {
        selectp->v3warn(COVERIGN, "Unsupported: non-constant or non-integral 'intersect' value, "
                                  "or four-state range bound.");
        valid = അസത്യം;
    }

    സ്ഥിതം തർക്കം cross_മൂല്യം_Matches_Filters(AstCoverBinsof* selectp, Ast_കണ്ണി* valuep,
                                         Ast_കണ്ണി_വ്യഞ്ജകം* exprp, സ്ഥിരം AstCoverBin* binp,
                                         സ്ഥിരം Cross_മൂല്യം_Range& domain,
                                         സ്ഥിരം std::vector<Cross_മൂല്യം_Range>& filters,
                                         തർക്കം& valid) {
        Cross_മൂല്യം_Range range{valuep, domain.lo.width()};
        എങ്കിൽ (!cross_മൂല്യം_Range(valuep, exprp, സത്യം, binp->isWildcard(), domain, range)) {
            unsupportedCrossRange(selectp, valid);
            തിരികെ അസത്യം;
        }
        വേണ്ടി (സ്ഥിരം Cross_മൂല്യം_Range& filter : filters) {
            എങ്കിൽ (crossRangesIntersect(range, filter)) തിരികെ സത്യം;
        }
        തിരികെ അസത്യം;
    }

    std::vector<തർക്കം> selectCoverpointBins(AstCoverBinsof* selectp, സ്ഥിരം CoverpointBins& bins,
                                           uint32_t first, uint32_t എണ്ണം, തർക്കം& valid) {
        എങ്കിൽ (selectp->rangesp() && !bins.exprp->dtypep()->skipRefp()->isIntegralOrPacked()) {
            unsupportedCrossRange(selectp, valid);
            തിരികെ {};
        }
        std::vector<തർക്കം> selected(bins.total, അസത്യം);
        std::vector<std::vector<Ast_കണ്ണി*>> മൂല്യങ്ങൾ;
        പൂർണ്ണസംഖ്യ width = bins.exprp->width();
        വേണ്ടി (Ast_കണ്ണി* rangep = selectp->rangesp(); rangep; rangep = rangep->nextp()) {
            width = std::max(width, crossRangeWidth(rangep));
        }
        എങ്കിൽ (selectp->rangesp()) {
            മൂല്യങ്ങൾ.reserve(എണ്ണം);
            വേണ്ടി (uint32_t i = first; i < first + എണ്ണം; ++i) {
                // A run's values are in the coverpoint type, whose width 'width' covers
                മൂല്യങ്ങൾ.തള്ളുക_back(bins.മൂല്യങ്ങൾ[i].runp ? std::vector<Ast_കണ്ണി*>{}
                                                     : cross_Bin_മൂല്യങ്ങൾ(bins.മൂല്യങ്ങൾ[i]));
                വേണ്ടി (Ast_കണ്ണി* സ്ഥിരം valuep : മൂല്യങ്ങൾ.back()) {
                    width = std::max(width, crossRangeWidth(valuep));
                }
            }
        }
        // One extra bit preserves both unsigned maxima and negative signed bounds.
        ++width;
        സ്ഥിരം Cross_മൂല്യം_Range domain
            = cross_മൂല്യം_Domain(selectp, bins.exprp->width(), bins.exprp->isSigned(), width);
        std::vector<Cross_മൂല്യം_Range> filters;
        വേണ്ടി (Ast_കണ്ണി* rangep = selectp->rangesp(); rangep; rangep = rangep->nextp()) {
            Cross_മൂല്യം_Range filter{rangep, width};
            എങ്കിൽ (!cross_മൂല്യം_Range(rangep, bins.exprp, അസത്യം, അസത്യം, domain, filter)) {
                unsupportedCrossRange(selectp, valid);
                തിരികെ {};
            }
            filters.തള്ളുക_back(std::move(filter));
        }
        വേണ്ടി (uint32_t i = first; i < first + എണ്ണം; ++i) {
            എങ്കിൽ (!selectp->rangesp()) {
                selected[i] = സത്യം;
                തുടരുക;
            }
            എങ്കിൽ (സ്ഥിരം BinRun* സ്ഥിരം runp = bins.മൂല്യങ്ങൾ[i].runp) {
                സ്ഥിരം Cross_മൂല്യം_Range range
                    = runBinRange(selectp, *runp, bins.മൂല്യങ്ങൾ[i].element, width);
                selected[i] = std::any_of(filters.begin(), filters.end(),
                                          [&](സ്ഥിരം Cross_മൂല്യം_Range& filter) {
                                              തിരികെ crossRangesIntersect(range, filter);
                                          });
                തുടരുക;
            }
            വേണ്ടി (Ast_കണ്ണി* സ്ഥിരം valuep : മൂല്യങ്ങൾ[i - first]) {
                selected[i] = cross_മൂല്യം_Matches_Filters(
                    selectp, valuep, bins.exprp, bins.മൂല്യങ്ങൾ[i].binp, domain, filters, valid);
                എങ്കിൽ (!valid) തിരികെ {};
                എങ്കിൽ (selected[i]) നിർത്തുക;
            }
        }
        എങ്കിൽ (selectp->isNegated()) {
            വേണ്ടി (uint32_t i = 0; i < bins.total; ++i) selected[i] = !selected[i];
        }
        തിരികെ selected;
    }

    // Constructor-time value metadata of one coverpoint, as C++ list entries
    ഘടന മൂല്യം_Lists final {
        std::vector<std::string> m_ranges;  // Bin, then low and high words
        std::vector<std::string> m_runs;  // First bin, count, then low, span, and high words
        std::vector<std::string> m_patterns;  // Bin, then value, mask, low, and high words
        std::vector<std::string> m_transitions;  // Transition bin
    };

    // Append a value's words, in the coverpoint's width, to a C++ list entry.
    സ്ഥിതം ശൂന്യം append_വാക്കുകൾ(std::string& text, സ്ഥിരം V3_സംഖ്യ& മൂല്യം, സ്ഥിരം Ast_കണ്ണി_വ്യഞ്ജകം* exprp) {
        V3_സംഖ്യ narrowed{&മൂല്യം, exprp->width()};
        narrowed.opAssign(മൂല്യം);
        വേണ്ടി (പൂർണ്ണസംഖ്യ word = 0; word < exprp->width_വാക്കുകൾ(); ++word) {
            text += ", " + cvtToStr(narrowed.edataWord(word)) + "U";
        }
    }

    ശൂന്യം collect_മൂല്യം_Metadata(മൂല്യം_Lists& lists, Ast_കണ്ണി_വ്യഞ്ജകം* exprp, AstCoverBin* binp,
                              uint32_t index, Ast_കണ്ണി_വ്യഞ്ജകം* valuep) {
        സ്ഥിരം std::string bin = cvtToStr(index) + "U";
        എങ്കിൽ (binp->transp()) lists.m_transitions.തള്ളുക_back(bin);
        വേണ്ടി (Ast_കണ്ണി* സ്ഥിരം sourcep : cross_Bin_മൂല്യങ്ങൾ({binp, valuep})) {
            Cross_മൂല്യം_Range range{sourcep, resolveWidth(sourcep, exprp)};
            എങ്കിൽ (!resolve_മൂല്യം(sourcep, exprp, സത്യം, binp->isWildcard(), range)) {
                // Sampling already resolved every state bin value, so only transitions remain.
                sourcep->v3warn(E_UNSUPPORTED, "Unsupported: non-integral value in a transition "
                                               "bin of a coverpoint with exclusions.");
                തുടരുക;
            }
            എങ്കിൽ (crossRangeEmpty(range)) തുടരുക;
            std::string entry = bin;
            എങ്കിൽ (range.wildcard) {
                V3_സംഖ്യ മൂല്യം{sourcep, exprp->width()};
                V3_സംഖ്യ mask{sourcep, exprp->width()};
                mask.opBitsNonXZ(range.pattern);
                മൂല്യം.opBitsOne(range.pattern);
                append_വാക്കുകൾ(entry, മൂല്യം, exprp);
                append_വാക്കുകൾ(entry, mask, exprp);
            }
            append_വാക്കുകൾ(entry, range.lo, exprp);
            append_വാക്കുകൾ(entry, range.hi, exprp);
            (range.wildcard ? lists.m_patterns : lists.m_ranges).തള്ളുക_back(entry);
        }
    }

    // Describe a run with one entry, from which the runtime computes the values of its bins.
    സ്ഥിതം ശൂന്യം collectRunMetadata(മൂല്യം_Lists& lists, Ast_കണ്ണി_വ്യഞ്ജകം* exprp, സ്ഥിരം BinRun& run) {
        V3_സംഖ്യ span{exprp, run.m_stride.width(), 0};
        span.opSub(run.m_stride, V3_സംഖ്യ{exprp, run.m_stride.width(), 1});
        std::string entry = cvtToStr(run.m_declared) + "U, " + cvtToStr(run.m_എണ്ണം) + "U";
        append_വാക്കുകൾ(entry, run.m_lo, exprp);
        append_വാക്കുകൾ(entry, span, exprp);
        append_വാക്കുകൾ(entry, run.m_hi, exprp);
        lists.m_runs.തള്ളുക_back(entry);
    }

    // Emit one batched metadata list, bounding the size of each call's temporary list.
    ശൂന്യം emit_മൂല്യം_List(File_വരി* fl, AstVar* cpVarp, VCMethod method,
                       സ്ഥിരം std::vector<std::string>& entries) {
        വേണ്ടി (size_t first = 0; first < entries.size(); first += മൂല്യം_LIST_ENTRIES) {
            സ്ഥിരം size_t end = std::min(entries.size(), first + മൂല്യം_LIST_ENTRIES);
            std::string text = "{" + entries[first];
            വേണ്ടി (size_t i = first + 1; i < end; ++i) text += ", " + entries[i];
            m_constructorp->addStmtsp(
                itemCall(fl, cpVarp, method, {ctext(fl, text + "}")})->makeStmt());
        }
    }

    സ്ഥിതം തർക്കം checkCrossRef(സ്ഥിരം AstCoverCrossRef* refp, സ്ഥിരം AstCoverCross* crossp) {
        എങ്കിൽ (refp->name() == crossp->name()) തിരികെ സത്യം;
        refp->v3error("Cross selection "
                      << refp->prettyNameQ() << " may only name its enclosing cross "
                      << crossp->prettyNameQ() << " (IEEE 1800-2023 19.6.1.2).");
        തിരികെ അസത്യം;
    }

    സ്ഥിതം തർക്കം checkCrossBinName(സ്ഥിരം AstCoverCrossBin* binp, std::set<std::string>& names) {
        എങ്കിൽ (names.emplace(binp->name()).second) തിരികെ സത്യം;
        binp->v3error("Duplicate cross bin " << binp->prettyNameQ()
                                             << " (IEEE 1800-2023 19.6.1).");
        തിരികെ അസത്യം;
    }

    // The span of the implicit automatic bin reported as 'name' ('auto_<i>', see
    // createImplicitAutoBins), found without naming each of its bins
    സ്ഥിതം തർക്കം implicitAutoBinSpan(സ്ഥിരം CoverpointBins& bins, സ്ഥിരം std::string& name,
                                    BinSpan& span) {
        സ്ഥിരം std::string prefix = "auto_";
        എങ്കിൽ (!VString::startsWith(name, prefix)) തിരികെ അസത്യം;
        സ്ഥിരം std::string digits = name.substr(prefix.size());
        സ്ഥിരം unsigned long index = std::strtoul(digits.c_str(), nullptr, 10);
        // Only the reported spelling names the bin, not e.g. 'auto_01' or 'auto_x'
        എങ്കിൽ (index >= bins.implicitAuto.എണ്ണം || digits != cvtToStr(index)) തിരികെ അസത്യം;
        സ്ഥിരം uint32_t offset = static_cast<uint32_t>(index);
        span = BinSpan{bins.implicitAuto.first + offset, 1, bins.implicitAuto.declared + offset};
        തിരികെ സത്യം;
    }

    CrossBinsofTarget
    resolveBinsofTarget(സ്ഥിരം AstCoverBinsof* selectp, സ്ഥിരം AstCoverCross* crossp,
                        സ്ഥിരം std::vector<AstVar*>& cpVars,
                        സ്ഥിരം std::map<std::string, uint32_t>& dimensions) സ്ഥിരം {
        സ്ഥിരം auto dim = dimensions.find(selectp->pointp()->name());
        എങ്കിൽ (dim == dimensions.end()) {
            selectp->v3error("binsof coverpoint "
                             << selectp->pointp()->prettyNameQ() << " is not an item of cross "
                             << crossp->prettyNameQ() << " (IEEE 1800-2023 19.6.1).");
            തിരികെ {};
        }
        സ്ഥിരം CoverpointBins& bins = m_cpBins.at(cpVars[dim->second]);
        CrossBinsofTarget target{&bins, dim->second, 0, bins.total};
        എങ്കിൽ (!selectp->name().empty()) {
            സ്ഥിരം auto bin = bins.spans.find(selectp->name());
            BinSpan span{0, 0, 0};
            എങ്കിൽ (bin != bins.spans.end()) {
                span = bin->second;
            } അല്ലെങ്കിൽ എങ്കിൽ (!implicitAutoBinSpan(bins, selectp->name(), span)) {
                selectp->v3error("Cannot find bin " << selectp->prettyNameQ() << " in coverpoint "
                                                    << selectp->pointp()->prettyNameQ()
                                                    << " (IEEE 1800-2023 19.6.1).");
                തിരികെ {};
            }
            target.m_first = span.first;
            target.m_എണ്ണം = span.എണ്ണം;
            target.m_declaredFirst = span.declared;
            target.m_declaredEnd = span.declared + span.എണ്ണം;
            target.m_sized = span.sized;
        }
        തിരികെ target;
    }

    // The variables a 'with' filter reads, each by its first reference
    using CrossFilterReads = std::map<സ്ഥിരം AstVar*, AstVarRef*>;

    // Whether a 'with' filter has a count of value tuples, of 'matches' other than '$'
    സ്ഥിതം തർക്കം counts_മൂല്യം_Tuples(സ്ഥിരം AstCoverCrossWith* withp) {
        തിരികെ withp->countp() && !VN_IS(withp->countp(), Unbounded);
    }

    // Whether the runtime knows the values of the cross items a 'with' filter reads, or to count
    // value tuples, of all items, of integral coverpoints; else warns
    സ്ഥിതം തർക്കം crossFilterSupported(സ്ഥിരം AstCoverCrossWith* withp,
                                     സ്ഥിരം CrossFilterReads& reads) {
        വേണ്ടി (സ്ഥിരം AstVar* varp = withp->varsp(); varp; varp = VN_AS(varp->nextp(), Var)) {
            എങ്കിൽ (varp->dtypep()->skipRefp()->isIntegralOrPacked()) തുടരുക;
            സ്ഥിരം auto it = reads.find(varp);
            എങ്കിൽ (it != reads.end()) {
                it->second->v3warn(COVERIGN, "Unsupported: cross bin 'with' filter reading the "
                                             "value of non-integral coverpoint "
                                                 << it->second->prettyNameQ());
                തിരികെ അസത്യം;
            }
            എങ്കിൽ (counts_മൂല്യം_Tuples(withp)) {
                withp->countp()->v3warn(COVERIGN, "Unsupported: cross bin 'matches' count of "
                                                  "value tuples of non-integral coverpoint "
                                                      << varp->prettyNameQ());
                തിരികെ അസത്യം;
            }
        }
        തിരികെ സത്യം;
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
    ശൂന്യം generateCrossFilter(സ്ഥിരം AstCoverCrossWith* withp, സ്ഥിരം CrossFilterReads& reads,
                             സ്ഥിരം AstCoverCrossBin* binp, സ്ഥിരം AstCoverCross* crossp,
                             AstVar* cxp, സ്ഥിരം std::map<string, uint32_t>& dimensions,
                             uint32_t ഇനങ്ങൾ) {
        File_വരി* സ്ഥിരം fl = withp->fileline();
        സ്ഥിരം string prefix
            = m_sizedNames.get(sanitizeGeneratedName(crossp->name() + "__" + binp->name()));
        സ്ഥിരം തർക്കം counting = counts_മൂല്യം_Tuples(withp);
        // The count, 0 for all
        സ്ഥിരം std::pair<Ast_കണ്ണി_വ്യഞ്ജകം*, Ast_കണ്ണി_വ്യഞ്ജകം*> എണ്ണം
            = counting ? new_എണ്ണം_Args(fl, prefix + "_count", withp->countp())
                       : new_എണ്ണം_Args(fl, withp->countp() ? 0 : 1);
        AstLoop* സ്ഥിരം loopp = പുതിയ AstLoop{fl};
        AstCMethodHard* സ്ഥിരം nextp = itemCall(fl, cxp, VCMethod::COVERGROUP_WITH_NEXT);
        nextp->dtypeSetBit();
        loopp->addStmtsp(പുതിയ AstLoopTest{fl, loopp, nextp});
        string dims;
        വേണ്ടി (uint32_t dim = 0; counting && dim < ഇനങ്ങൾ; ++dim) {
            dims += (dims.empty() ? "" : ", ") + cvtToStr(dim) + "U";
        }
        ആകുമ്പോൾ (AstVar* സ്ഥിരം varp = withp->varsp()) {
            varp->unlinkFrBack();
            എങ്കിൽ (!reads.എണ്ണം(varp)) {
                VL_DO_DANGLING(തള്ളുക_Deletep(varp), varp);
                തുടരുക;
            }
            സ്ഥിരം auto dimIt = dimensions.find(varp->name());
            UASSERT_OBJ(dimIt != dimensions.end(), varp, "Cross 'with' candidate not of an item");
            സ്ഥിരം uint32_t dim = dimIt->second;
            എങ്കിൽ (!counting) dims += (dims.empty() ? "" : ", ") + cvtToStr(dim) + "U";
            varp->name(prefix + "_" + sanitizeGeneratedName(varp->name()));
            m_constructorp->addStmtsp(varp);
            എങ്കിൽ (varp->isWide()) {
                loopp->addStmtsp(itemCall(fl, cxp, VCMethod::COVERGROUP_CROSS_WITH_മൂല്യം_W,
                                          {cnum(fl, dim), പുതിയ AstVarRef{fl, varp, VAccess::WRITE}})
                                     ->makeStmt());
            } അല്ലെങ്കിൽ {
                AstCMethodHard* സ്ഥിരം valuep
                    = itemCall(fl, cxp, VCMethod::COVERGROUP_CROSS_WITH_മൂല്യം, {cnum(fl, dim)});
                valuep->dtypeSetUInt64();
                Ast_കണ്ണി_DType* സ്ഥിരം dtypep = varp->findLogicDType(
                    varp->width(), varp->width(),
                    varp->isSigned() ? VSigning::SIGNED : VSigning::UNSIGNED);
                loopp->addStmtsp(പുതിയ AstAssign{fl, പുതിയ AstVarRef{fl, varp, VAccess::WRITE},
                                               resize_മൂല്യം(valuep, dtypep)});
            }
        }
        loopp->addStmtsp(itemCall(fl, cxp, VCMethod::COVERGROUP_CROSS_WITH_TEST,
                                  {withp->filterp()->unlinkFrBack()})
                             ->makeStmt());
        സ്ഥിരം തർക്കം prot = v3Global.opt.protectIds();
        m_constructorp->addStmtsp(
            itemCall(fl, cxp, VCMethod::COVERGROUP_CROSS_WITH_BEGIN,
                     {ctext(fl, "{" + dims + "}"), എണ്ണം.first, എണ്ണം.second,
                      ctext(fl, quoted(VIdProtect::protect_വാക്കുകൾ_If(binp->name(), prot))),
                      ctext(fl, quoted(VIdProtect::protectIf(fl->filename(), prot))),
                      cnum(fl, static_cast<uint32_t>(fl->lineno()))})
                ->makeStmt());
        m_constructorp->addStmtsp(loopp);
        m_constructorp->addStmtsp(
            itemCall(fl, cxp, VCMethod::COVERGROUP_CROSS_WITH_END)->makeStmt());
    }

    തർക്കം generateRuntimeSelection(Ast_കണ്ണി* nodep, സ്ഥിരം AstCoverCrossBin* binp,
                                  AstCoverCross* crossp, AstVar* cxp,
                                  സ്ഥിരം std::vector<AstVar*>& cpVars,
                                  സ്ഥിരം std::map<string, uint32_t>& dimensions) {
        File_വരി* സ്ഥിരം fl = nodep->fileline();
        എങ്കിൽ (സ്ഥിരം AstCoverCrossRef* സ്ഥിരം refp = VN_CAST(nodep, CoverCrossRef)) {
            എങ്കിൽ (!checkCrossRef(refp, crossp)) തിരികെ അസത്യം;
            m_constructorp->addStmtsp(
                itemCall(fl, cxp, VCMethod::COVERGROUP_SELECT_ALL)->makeStmt());
            തിരികെ സത്യം;
        }
        എങ്കിൽ (സ്ഥിരം AstCoverCrossSelect* സ്ഥിരം opp = VN_CAST(nodep, CoverCrossSelect)) {
            എങ്കിൽ (!generateRuntimeSelection(opp->lhsp(), binp, crossp, cxp, cpVars, dimensions)
                || !generateRuntimeSelection(opp->rhsp(), binp, crossp, cxp, cpVars, dimensions)) {
                തിരികെ അസത്യം;
            }
            m_constructorp->addStmtsp(itemCall(fl, cxp,
                                               opp->isOr() ? VCMethod::COVERGROUP_SELECT_OR
                                                           : VCMethod::COVERGROUP_SELECT_AND)
                                          ->makeStmt());
            തിരികെ സത്യം;
        }
        എങ്കിൽ (സ്ഥിരം AstCoverCrossWith* സ്ഥിരം withp = VN_CAST(nodep, CoverCrossWith)) {
            CrossFilterReads reads;
            withp->filterp()->foreach([&](AstVarRef* refp) { reads.emplace(refp->varp(), refp); });
            // The count of 'matches' is a positive integer, or '$' (IEEE 1800-2023 19.6.1.2)
            എങ്കിൽ ((counts_മൂല്യം_Tuples(withp)
                 && !check_എണ്ണം(withp->countp(), "Cross bin 'matches' count", "19.6.1.2"))
                || !crossFilterSupported(withp, reads)
                || !generateRuntimeSelection(withp->selectp(), binp, crossp, cxp, cpVars,
                                             dimensions)) {
                തിരികെ അസത്യം;
            }
            generateCrossFilter(withp, reads, binp, crossp, cxp, dimensions,
                                static_cast<uint32_t>(cpVars.size()));
            തിരികെ സത്യം;
        }
        AstCoverBinsof* സ്ഥിരം selectp = VN_AS(nodep, CoverBinsof);
        സ്ഥിരം CrossBinsofTarget target = resolveBinsofTarget(selectp, crossp, cpVars, dimensions);
        എങ്കിൽ (!target.m_binsp) തിരികെ അസത്യം;
        സ്ഥിരം CoverpointBins& bins = *target.m_binsp;
        എങ്കിൽ (selectp->rangesp() && !bins.exprp->dtypep()->skipRefp()->isIntegralOrPacked()) {
            തർക്കം valid = സത്യം;
            unsupportedCrossRange(selectp, valid);
            തിരികെ അസത്യം;
        }
        Ast_കണ്ണി_വ്യഞ്ജകം* firstp;
        Ast_കണ്ണി_വ്യഞ്ജകം* endp;
        എങ്കിൽ (target.m_sized < 0) {
            firstp = cnum(fl, target.m_declaredFirst);
            endp = cnum(fl, target.m_declaredEnd);
        } അല്ലെങ്കിൽ {  // A sized array, whose bins the coverpoint's construction placed
            AstVar* സ്ഥിരം cpVarp = cpVars[target.m_dimension];
            സ്ഥിരം uint32_t sized = static_cast<uint32_t>(target.m_sized);
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
        വേണ്ടി (Ast_കണ്ണി* rangep = selectp->rangesp(); rangep; rangep = rangep->nextp()) {
            Cross_മൂല്യം_Range range{rangep, resolveWidth(rangep, bins.exprp)};
            എങ്കിൽ (!resolve_മൂല്യം(rangep, bins.exprp, അസത്യം, അസത്യം, range)) {
                തർക്കം valid = സത്യം;
                unsupportedCrossRange(selectp, valid);
                തിരികെ അസത്യം;
            }
            എങ്കിൽ (crossRangeEmpty(range)) തുടരുക;
            m_constructorp->addStmtsp(itemCall(fl, cxp,
                                               bins.exprp->isWide()
                                                   ? VCMethod::COVERGROUP_SELECT_RANGE_W
                                                   : VCMethod::COVERGROUP_SELECT_RANGE,
                                               {new_മൂല്യം_Const(fl, range.lo, bins.exprp),
                                                new_മൂല്യം_Const(fl, range.hi, bins.exprp)})
                                          ->makeStmt());
        }
        m_constructorp->addStmtsp(
            itemCall(fl, cxp, VCMethod::COVERGROUP_SELECT_DIM_END)->makeStmt());
        തിരികെ സത്യം;
    }

    std::vector<AstCoverCrossBin*>
    generateRuntimeCrossBins(AstCoverCross* crossp, AstVar* cxp,
                             സ്ഥിരം std::vector<AstVar*>& cpVars,
                             സ്ഥിരം std::map<string, uint32_t>& dimensions) {
        std::vector<AstCoverCrossBin*> bins;
        std::set<string> names;
        വേണ്ടി (Ast_കണ്ണി* itemp = crossp->binsp(); itemp; itemp = itemp->nextp()) {
            AstCoverCrossBin* സ്ഥിരം binp = VN_AS(itemp, CoverCrossBin);
            എങ്കിൽ (!checkCrossBinName(binp, names)) തുടരുക;
            File_വരി* സ്ഥിരം fl = binp->fileline();
            // Generate the selection apart, so that an unsupported one leaves no code, which
            // could leave its state for the next bin
            AstFunc* സ്ഥിരം selectionp = പുതിയ AstFunc{fl, "", nullptr, nullptr};
            തർക്കം supported;
            {
                VL_RESTORER(m_constructorp);
                m_constructorp = selectionp;
                supported = generateRuntimeSelection(binp->selectp(), binp, crossp, cxp, cpVars,
                                                     dimensions);
            }
            // A supported selection has code
            എങ്കിൽ (supported) m_constructorp->addStmtsp(selectionp->stmtsp()->unlinkFrBackWithNext());
            VL_DO_DANGLING(തള്ളുക_Deletep(selectionp), selectionp);
            എങ്കിൽ (!supported) തുടരുക;
            സ്ഥിരം തർക്കം protect = v3Global.opt.protectIds();
            m_constructorp->addStmtsp(
                itemCall(fl, cxp, VCMethod::COVERGROUP_SELECT_BIN,
                         {ctext(fl, binp->binsType().binSetEnum()),
                          ctext(fl, quoted(VIdProtect::protect_വാക്കുകൾ_If(binp->name(), protect))),
                          ctext(fl, quoted(VIdProtect::protectIf(fl->filename(), protect))),
                          cnum(fl, fl->lineno()), cnum(fl, fl->firstColumn()),
                          cnum(fl, static_cast<uint32_t>(bins.size()))})
                    ->makeStmt());
            bins.തള്ളുക_back(binp);
        }
        m_constructorp->addStmtsp(
            itemCall(crossp->fileline(), cxp, VCMethod::COVERGROUP_FINALIZE_BINS)->makeStmt());
        തിരികെ bins;
    }

    സ്ഥിതം ശൂന്യം setCrossSelectionRange(CrossSelection& selection, uint64_t first, uint64_t end) {
        ആകുമ്പോൾ (first < end) {
            സ്ഥിരം unsigned bit = first % 64;
            സ്ഥിരം unsigned bits = std::min<uint64_t>(64 - bit, end - first);
            selection[VL_BITWORD_Q(first)]
                |= (bits == 64 ? ~uint64_t{0} : (uint64_t{1} << bits) - 1) << bit;
            first += bits;
        }
    }

    CrossSelection crossSelection(Ast_കണ്ണി* nodep, CrossSelectionContext& ctx) {
        എങ്കിൽ (സ്ഥിരം AstCoverCrossRef* സ്ഥിരം refp = VN_CAST(nodep, CoverCrossRef)) {
            എങ്കിൽ (!checkCrossRef(refp, ctx.crossp)) {
                ctx.valid = അസത്യം;
                തിരികെ {};
            }
            CrossSelection result(
                VL_BITWORD_Q(static_cast<uint64_t>(ctx.tuples) + VL_QUADSIZE - 1), 0);
            setCrossSelectionRange(result, 0, ctx.tuples);
            തിരികെ result;
        }
        എങ്കിൽ (AstCoverCrossSelect* സ്ഥിരം opp = VN_CAST(nodep, CoverCrossSelect)) {
            CrossSelection lhs = crossSelection(opp->lhsp(), ctx);
            സ്ഥിരം CrossSelection rhs = crossSelection(opp->rhsp(), ctx);
            എങ്കിൽ (!ctx.valid) തിരികെ {};
            വേണ്ടി (size_t i = 0; i < lhs.size(); ++i) {
                lhs[i] = opp->isOr() ? lhs[i] | rhs[i] : lhs[i] & rhs[i];
            }
            തിരികെ lhs;
        }
        AstCoverBinsof* സ്ഥിരം selectp = VN_AS(nodep, CoverBinsof);
        സ്ഥിരം CrossBinsofTarget target
            = resolveBinsofTarget(selectp, ctx.crossp, ctx.cpVars, ctx.dimensions);
        എങ്കിൽ (!target.m_binsp) {
            ctx.valid = അസത്യം;
            തിരികെ {};
        }
        സ്ഥിരം CoverpointBins& bins = *target.m_binsp;
        // Coverpoints with sized arrays are runtime points, so feed only runtime crosses
        UASSERT_OBJ(target.m_sized < 0, selectp, "Sized bin array selected by a static cross");
        സ്ഥിരം std::vector<തർക്കം> selected
            = selectCoverpointBins(selectp, bins, target.m_first, target.m_എണ്ണം, ctx.valid);
        എങ്കിൽ (!ctx.valid) തിരികെ {};
        CrossSelection result(VL_BITWORD_Q(static_cast<uint64_t>(ctx.tuples) + VL_QUADSIZE - 1),
                              0);
        സ്ഥിരം uint64_t stride = ctx.strides[target.m_dimension];
        സ്ഥിരം uint64_t period = stride * bins.total;
        വേണ്ടി (uint64_t base = 0; base < ctx.tuples; base += period) {
            വേണ്ടി (uint32_t i = 0; i < bins.total;) {
                എങ്കിൽ (!selected[i]) {
                    ++i;
                    തുടരുക;
                }
                സ്ഥിരം uint32_t begin = i++;
                ആകുമ്പോൾ (i < bins.total && selected[i]) ++i;
                setCrossSelectionRange(result, base + begin * stride, base + i * stride);
            }
        }
        തിരികെ result;
    }

    // Size the Cartesian product of the declared Normal bins, with each dimension's flat-index
    // stride.  Live runtime bins only shrink it.  Warns and returns false if it is too large.
    തർക്കം crossShape(AstCoverCross* crossp, സ്ഥിരം std::vector<AstVar*>& cpVars,
                    std::vector<uint32_t>& strides, uint32_t& tuples) സ്ഥിരം {
        uint64_t product = std::any_of(cpVars.begin(), cpVars.end(),
                                       [ഇത്](AstVar* varp) { തിരികെ !m_cpBins.at(varp).total; })
                               ? 0
                               : 1;
        strides.resize(cpVars.size());
        വേണ്ടി (size_t d = cpVars.size(); d > 0; --d) {
            strides[d - 1] = static_cast<uint32_t>(product);
            product *= m_cpBins.at(cpVars[d - 1]).total;
            എങ്കിൽ (product > UINT32_MAX) {
                crossp->v3warn(COVERIGN,
                               "Unsupported: cross coverage with more than 2^32-1 tuples.");
                തിരികെ അസത്യം;
            }
        }
        tuples = static_cast<uint32_t>(product);
        തിരികെ സത്യം;
    }

    CrossLayout resolveCrossLayout(AstCoverCross* crossp, സ്ഥിരം std::vector<AstVar*>& cpVars,
                                   സ്ഥിരം std::map<std::string, uint32_t>& dimensions) {
        CrossLayout layout;
        CrossSelectionContext ctx{crossp, cpVars, dimensions, 0, {}};
        എങ്കിൽ (!crossShape(crossp, cpVars, ctx.strides, ctx.tuples)) {
            layout.valid = അസത്യം;
            തിരികെ layout;
        }
        layout.tuples = ctx.tuples;
        CrossSelection occupied;
        CrossSelection excluded;
        std::set<std::string> names;
        വേണ്ടി (Ast_കണ്ണി* itemp = crossp->binsp(); itemp; itemp = itemp->nextp()) {
            AstCoverCrossBin* സ്ഥിരം binp = VN_AS(itemp, CoverCrossBin);
            എങ്കിൽ (!checkCrossBinName(binp, names)) തുടരുക;
            ctx.valid = സത്യം;
            CrossSelection selection = crossSelection(binp->selectp(), ctx);
            എങ്കിൽ (!ctx.valid || std::all_of(selection.begin(), selection.end(), [](uint64_t word) {
                    തിരികെ word == 0;
                })) {
                തുടരുക;
            }
            എങ്കിൽ (occupied.empty()) {
                occupied.resize(selection.size(), 0);
                excluded.resize(selection.size(), 0);
            }
            വേണ്ടി (size_t i = 0; i < selection.size(); ++i) {
                occupied[i] |= selection[i];
                എങ്കിൽ (!binp->binsType().binIsNormal()) excluded[i] |= selection[i];
            }
            layout.bins.തള്ളുക_back({binp, std::move(selection)});
        }
        എങ്കിൽ (!layout.bins.empty()) {
            // IEEE 1800-2023 19.6.2/19.6.3: exclusions also remove tuples from named
            // bins, independently of declaration order and sampling guards.
            വേണ്ടി (ResolvedCrossBin& resolved : layout.bins) {
                വേണ്ടി (size_t i = 0; i < resolved.selection.size(); ++i) {
                    എങ്കിൽ (resolved.binp->binsType().binIsNormal()) {
                        resolved.selection[i] &= ~excluded[i];
                    }
                    എങ്കിൽ (resolved.selection[i]) ++layout.bin_വാക്കുകൾ;
                }
            }
            layout.bins.erase(std::remove_if(layout.bins.begin(), layout.bins.end(),
                                             [](സ്ഥിരം ResolvedCrossBin& resolved) {
                                                 തിരികെ std::all_of(
                                                     resolved.selection.begin(),
                                                     resolved.selection.end(),
                                                     [](uint64_t word) { തിരികെ word == 0; });
                                             }),
                              layout.bins.end());
            layout.autoBins = layout.tuples;
            വേണ്ടി (സ്ഥിരം uint64_t word : occupied) {
                layout.autoBins -= static_cast<uint32_t>(std::bitset<VL_QUADSIZE>{word}.എണ്ണം());
            }
        }
        തിരികെ layout;
    }

    AstCoverCrossDType* crossDType(File_വരി* fl, uint32_t dimensions, സ്ഥിരം CrossLayout& layout,
                                   തർക്കം dynamic = അസത്യം) {
        സ്ഥിരം uint32_t bins = static_cast<uint32_t>(layout.bins.size());
        സ്ഥിരം CrossShape shape{dimensions,      layout.tuples,   bins,
                               layout.autoBins, layout.bin_വാക്കുകൾ, dynamic};
        AstCoverCrossDType*& typep = m_cxDTypes[shape];
        എങ്കിൽ (!typep) {
            typep = പുതിയ AstCoverCrossDType{
                fl, dimensions, layout.tuples, bins, layout.autoBins, layout.bin_വാക്കുകൾ, dynamic};
            v3Global.rootp()->typeTablep()->addTypesp(typep);
        }
        തിരികെ typep;
    }

    std::vector<AstCoverCrossBin*> generateCrossBins(AstCoverCross* crossp, AstVar* cxVarp,
                                                     സ്ഥിരം CrossLayout& layout) {
        std::vector<AstCoverCrossBin*> bins;
        വേണ്ടി (സ്ഥിരം ResolvedCrossBin& resolved : layout.bins) {
            AstCoverCrossBin* സ്ഥിരം binp = resolved.binp;
            സ്ഥിരം CrossSelection& selection = resolved.selection;
            File_വരി* സ്ഥിരം fl = binp->fileline();
            സ്ഥിരം തർക്കം prot = v3Global.opt.protectIds();
            std::string mask = "{";
            വേണ്ടി (size_t i = 0; i < selection.size(); ++i) {
                എങ്കിൽ (i) mask += ", ";
                mask += std::to_string(selection[i]) + "ULL";
            }
            mask += "}";
            m_constructorp->addStmtsp(
                itemCall(fl, cxVarp, VCMethod::COVERGROUP_ADD_BIN,
                         {ctext(fl, binp->binsType().binSetEnum()), ctext(fl, mask),
                          ctext(fl, quoted(VIdProtect::protect_വാക്കുകൾ_If(binp->name(), prot))),
                          ctext(fl, quoted(VIdProtect::protectIf(fl->filename(), prot))),
                          cnum(fl, static_cast<uint32_t>(fl->lineno())),
                          cnum(fl, static_cast<uint32_t>(fl->firstColumn()))})
                    ->makeStmt());
            bins.തള്ളുക_back(binp);
        }
        എങ്കിൽ (!bins.empty()) {
            m_constructorp->addStmtsp(
                itemCall(crossp->fileline(), cxVarp, VCMethod::COVERGROUP_FINALIZE_BINS)
                    ->makeStmt());
        }
        തിരികെ bins;
    }

    // Route a cross through a VlCoverCross member: emit the member, its constructor init +
    // registration, and the sample() call.  The feeding coverpoints are already generated
    // (their hit lists drive the cross). Each explicit bin adds one configuration call.
    ശൂന്യം generateCross(AstCoverCross* crossp) {
        File_വരി* സ്ഥിരം fl = crossp->fileline();
        സ്ഥിരം തർക്കം dynamic = m_runtimeCrosses.എണ്ണം(crossp);
        UINFO(4, "  Generating VlCoverCross member: " << crossp->name());

        എങ്കിൽ (Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം iffp = crossp->iffp()) {
            crossp->iffp(
                captureIffToTemp(iffp, "__VcrossIff_" + sanitizeGeneratedName(crossp->name())));
        }

        // Resolve and unlink the coverpoint refs, in dimension order.  Every ref resolves to a
        // known coverpoint (a cross with an unresolvable item was dropped earlier).
        std::vector<AstVar*> cpVars;
        std::map<std::string, uint32_t> dimensions;
        വേണ്ടി (Ast_കണ്ണി* itemp = crossp->itemsp(); itemp;) {
            Ast_കണ്ണി* സ്ഥിരം nextp = itemp->nextp();
            AstCoverpointRef* സ്ഥിരം refp = VN_AS(itemp, CoverpointRef);
            സ്ഥിരം auto it = m_cpVarMap.find(refp->name());
            UASSERT_OBJ(it != m_cpVarMap.end(), crossp, "Cross references an unknown coverpoint");
            dimensions.emplace(refp->name(), static_cast<uint32_t>(cpVars.size()));
            cpVars.തള്ളുക_back(it->second);
            VL_DO_DANGLING(തള്ളുക_Deletep(refp->unlinkFrBack()), refp);
            itemp = nextp;
        }
        സ്ഥിരം പൂർണ്ണസംഖ്യ dims = static_cast<പൂർണ്ണസംഖ്യ>(cpVars.size());
        CrossLayout layout;
        എങ്കിൽ (dynamic) {
            // The runtime sizes the layout from live bins; only check the declared bound here.
            std::vector<uint32_t> strides;
            uint32_t tuples = 0;
            layout.valid = crossShape(crossp, cpVars, strides, tuples);
        } അല്ലെങ്കിൽ {
            layout = resolveCrossLayout(crossp, cpVars, dimensions);
        }
        എങ്കിൽ (!layout.valid) തിരികെ;

        AstVar* സ്ഥിരം cxVarp
            = പുതിയ AstVar{fl, VVarType::MEMBER, "__Vcx_" + crossp->name(),
                         crossDType(fl, static_cast<uint32_t>(dims), layout, dynamic)};
        m_covergroupp->addMembersp(cxVarp);
        m_constructorp->addStmtsp(makeItemCreate(fl, cxVarp,
                                                 dynamic ? VCMethod::COVERGROUP_ADD_CROSS_DYN
                                                         : VCMethod::COVERGROUP_ADD_CROSS));
        generateItemWeight(fl, cxVarp, crossp->optionsp());
        പൂർണ്ണസംഖ്യ at_Least_മൂല്യം;
        പൂർണ്ണസംഖ്യ autoBinMax;  // Not a cross option
        extractItemOptions(crossp->optionsp(), at_Least_മൂല്യം, autoBinMax);

        // Constructor: init (after the coverpoints, which generate earlier) then registration.
        // Obfuscate the hierarchy/filename/page under --protect-ids as for coverpoints above.
        സ്ഥിരം തർക്കം prot = v3Global.opt.protectIds();
        സ്ഥിരം std::string hier
            = VIdProtect::protect_വാക്കുകൾ_If(m_covergroupName + "." + crossp->name(), prot);
        m_constructorp->addStmtsp(makeCrossCpsCall(
            fl, cpVars,
            itemCall(fl, cxVarp, VCMethod::COVERGROUP_INIT,
                     {ctext(fl, quoted(hier)), cnum(fl, static_cast<uint32_t>(at_Least_മൂല്യം)),
                      cnum(fl, static_cast<uint32_t>(dims)), ctext(fl, "__Vcx_cps"),
                      ctext(fl, quoted(VIdProtect::protectIf(fl->filename(), prot))),
                      cnum(fl, static_cast<uint32_t>(fl->lineno())),
                      cnum(fl, static_cast<uint32_t>(fl->firstColumn()))})));
        സ്ഥിരം std::vector<AstCoverCrossBin*> bins
            = dynamic ? generateRuntimeCrossBins(crossp, cxVarp, cpVars, dimensions)
                      : generateCrossBins(crossp, cxVarp, layout);
        എങ്കിൽ (v3Global.opt.coverage()) {
            സ്ഥിരം std::string page
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
        സ്ഥിരം തർക്കം hasIffs
            = std::any_of(bins.begin(), bins.end(),
                          [](സ്ഥിരം AstCoverCrossBin* binp) { തിരികെ binp->iffp() != nullptr; });
        Ast_കണ്ണി_Stmt* സ്ഥിരം samplep
            = !hasIffs ? static_cast<Ast_കണ്ണി_Stmt*>(
                             itemCall(fl, cxVarp, VCMethod::COVERGROUP_SAMPLE)->makeStmt())
                       : static_cast<Ast_കണ്ണി_Stmt*>(makeCrossIffsCall(
                             fl, bins,
                             itemCall(fl, cxVarp, VCMethod::COVERGROUP_SAMPLE_IFFS,
                                      {ctext(fl, "__Vcx_iffs")})));
        എങ്കിൽ (Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം iffp = crossp->iffp()) {
            m_sampleFuncp->addStmtsp(പുതിയ AstIf{fl, iffp->clone_വൃക്ഷം(അസത്യം), samplep});
        } അല്ലെങ്കിൽ {
            m_sampleFuncp->addStmtsp(samplep);
        }
    }

    ശൂന്യം generateCrossCode(AstCoverCross* crossp) {
        UINFO(4, "  Generating code for cross: " << crossp->name());

        // Non-standard hierarchical/dotted cross item (e.g. 'cross a.b'): an implicit coverpoint
        // over the referenced expression (carried in refp->exprp()).  The grammar already warned
        // NONSTD; implicit coverpoints are not yet implemented, so generate no sampling code for
        // this cross.  When support is added the implicit coverpoint should be synthesized
        // upstream (V3LinkParse) as a real AstCoverpoint so it flows through the normal coverpoint
        // path - by here coverpoint lowering has already run.
        വേണ്ടി (Ast_കണ്ണി* itemp = crossp->itemsp(); itemp; itemp = itemp->nextp()) {
            സ്ഥിരം AstCoverpointRef* സ്ഥിരം refp = VN_AS(itemp, CoverpointRef);
            എങ്കിൽ (refp->exprp()) {
                refp->v3warn(COVERIGN,
                             "Unsupported: cross of hierarchical reference (implicit coverpoint)");
                തിരികെ;
            }
        }

        // A cross naming a bare variable (implicit coverpoint, which Verilator does not
        // synthesize) is dropped entirely with a COVERIGN warning -- it produces no coverage
        // either way -- but only this cross is dropped; its sibling crosses are still generated
        // and the real coverpoints it referenced remain as independent coverpoints.
        എങ്കിൽ (m_droppedCrosses.എണ്ണം(crossp)) {
            വേണ്ടി (Ast_കണ്ണി* itemp = crossp->itemsp(); itemp; itemp = itemp->nextp()) {
                സ്ഥിരം AstCoverpointRef* സ്ഥിരം refp = VN_AS(itemp, CoverpointRef);
                എങ്കിൽ (m_coverpointMap.find(refp->name()) == m_coverpointMap.end()) {
                    refp->v3warn(COVERIGN, "Unsupported: cross of "
                                               << refp->prettyNameQ()
                                               << " which is not a coverpoint (implicit "
                                                  "coverpoint)");
                    നിർത്തുക;
                }
            }
            തിരികെ;
        }

        // Every cross that isn't dropped routes through a VlCoverCross member.
        generateCross(crossp);
    }

    Ast_കണ്ണി_വ്യഞ്ജകം* buildBinCondition(AstCoverBin* binp, Ast_കണ്ണി_വ്യഞ്ജകം* exprp) {
        // Get the range list from the bin
        Ast_കണ്ണി* സ്ഥിരം rangep = binp->rangesp();
        എങ്കിൽ (!rangep) തിരികെ nullptr;

        // Integral values resolve to the coverpoint's type, as its runtime metadata does
        സ്ഥിരം തർക്കം integral = exprp->dtypep()->skipRefp()->isIntegralOrPacked();
        // No value form of a wildcard bin is allowed on a real coverpoint (IEEE 1800-2023 19.5.4)
        എങ്കിൽ (binp->isWildcard() && !integral) തിരികെ wildcardTypeError(binp, exprp);

        // Build condition by OR-ing all ranges together
        Ast_കണ്ണി_വ്യഞ്ജകം* fullCondp = nullptr;

        വേണ്ടി (Ast_കണ്ണി* currRangep = rangep; currRangep; currRangep = currRangep->nextp()) {
            Ast_കണ്ണി_വ്യഞ്ജകം* rangeCondp = nullptr;
            currRangep = V3Const::constifyEdit(currRangep);

            എങ്കിൽ (AstInsideRange* irp = VN_CAST(currRangep, InsideRange)) {
                Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം minExprp = irp->lhsp();
                Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം maxExprp = irp->rhsp();
                AstConst* സ്ഥിരം minConstp = VN_CAST(minExprp, Const);
                AstConst* സ്ഥിരം maxConstp = VN_CAST(maxExprp, Const);
                സ്ഥിരം തർക്കം loUnbounded = VN_IS(minExprp, Unbounded);
                സ്ഥിരം തർക്കം hiUnbounded = VN_IS(maxExprp, Unbounded);
                എങ്കിൽ (loUnbounded || hiUnbounded) {
                    // Open-ended range: '$' is the coverpoint domain min/max, so the
                    // range reduces to a single inequality (e.g. {[10:$]} -> expr >= 10).
                    AstConst* സ്ഥിരം boundp = hiUnbounded ? minConstp : maxConstp;
                    എങ്കിൽ (loUnbounded && hiUnbounded) {
                        rangeCondp = പുതിയ AstConst{irp->fileline(), AstConst::BitTrue{}};
                    } അല്ലെങ്കിൽ എങ്കിൽ (!boundp) {
                        irp->v3error("Non-constant expression in bin range; "
                                     "range bounds must be constants (IEEE 1800-2023 19.5)");
                        എങ്കിൽ (fullCondp) VL_DO_DANGLING(തള്ളുക_Deletep(fullCondp), fullCondp);
                        തിരികെ nullptr;
                    } അല്ലെങ്കിൽ എങ്കിൽ (boundp->അക്കം().isFourState()) {
                        irp->v3error("Four-state (x/z) value in bin range bound; "
                                     "range bounds must be two-state constants");
                        എങ്കിൽ (fullCondp) VL_DO_DANGLING(തള്ളുക_Deletep(fullCondp), fullCondp);
                        തിരികെ nullptr;
                    } അല്ലെങ്കിൽ എങ്കിൽ (integral) {
                        rangeCondp = build_മൂല്യം_Condition(binp, exprp, irp);
                    } അല്ലെങ്കിൽ {
                        rangeCondp = makeOpenRangeCondition(irp->fileline(), exprp, boundp,
                                                            /*isLowerBound=*/hiUnbounded);
                    }
                } അല്ലെങ്കിൽ എങ്കിൽ (!minConstp || !maxConstp) {
                    irp->v3error("Non-constant expression in bin range; "
                                 "range bounds must be constants (IEEE 1800-2023 19.5)");
                    എങ്കിൽ (fullCondp) VL_DO_DANGLING(തള്ളുക_Deletep(fullCondp), fullCondp);
                    തിരികെ nullptr;
                } അല്ലെങ്കിൽ എങ്കിൽ (minConstp->അക്കം().isFourState() || maxConstp->അക്കം().isFourState()) {
                    irp->v3error("Four-state (x/z) value in bin range bound; "
                                 "range bounds must be two-state constants");
                    എങ്കിൽ (fullCondp) VL_DO_DANGLING(തള്ളുക_Deletep(fullCondp), fullCondp);
                    തിരികെ nullptr;
                } അല്ലെങ്കിൽ എങ്കിൽ (integral) {
                    rangeCondp = build_മൂല്യം_Condition(binp, exprp, irp);
                } അല്ലെങ്കിൽ {
                    rangeCondp = makeRangeCondition(irp->fileline(), exprp, minExprp, maxExprp);
                }
            } അല്ലെങ്കിൽ എങ്കിൽ (AstConst* constp = VN_CAST(currRangep, Const)) {
                rangeCondp = build_മൂല്യം_Condition(binp, exprp, constp);
            } അല്ലെങ്കിൽ {
                currRangep->v3error("Non-constant expression in bin range; values must be "
                                    "constants (IEEE 1800-2023 19.5)");
                എങ്കിൽ (fullCondp) VL_DO_DANGLING(തള്ളുക_Deletep(fullCondp), fullCondp);
                തിരികെ nullptr;
            }

            UASSERT_OBJ(rangeCondp, binp, "rangeCondp is null after building range condition");
            fullCondp
                = fullCondp ? പുതിയ AstOr{binp->fileline(), fullCondp, rangeCondp} : rangeCondp;
        }

        തിരികെ fullCondp;
    }

    // Wildcard bits have no meaning for a non-integral coverpoint.
    സ്ഥിതം Ast_കണ്ണി_വ്യഞ്ജകം* wildcardTypeError(AstCoverBin* binp, Ast_കണ്ണി_വ്യഞ്ജകം* exprp) {
        സ്ഥിരം Ast_കണ്ണി_DType* സ്ഥിരം dtypep = exprp->dtypep()->skipRefp();
        exprp->v3error("Cannot use a wildcard bin on a coverpoint of type "
                       << dtypep->prettyDTypeNameQ() << " (IEEE 1800-2023 19.5.4).\n"
                       << exprp->warnContextPrimary() << '\n'
                       << binp->warnOther() << "... Location of wildcard bin\n"
                       << binp->warnContextSecondary());
        തിരികെ പുതിയ AstConst{binp->fileline(), AstConst::BitFalse{}};
    }

    // Match one bin value, range, or wildcard pattern.  Integral coverpoints first resolve the
    // value to their type (IEEE 1800-2023 19.5.7).  Non-owning: clones what it uses.
    Ast_കണ്ണി_വ്യഞ്ജകം* build_മൂല്യം_Condition(AstCoverBin* binp, Ast_കണ്ണി_വ്യഞ്ജകം* exprp, Ast_കണ്ണി* valuep) {
        File_വരി* സ്ഥിരം fl = valuep->fileline();
        എങ്കിൽ (!exprp->dtypep()->skipRefp()->isIntegralOrPacked()) {
            തിരികെ പുതിയ AstEq{fl, exprp->clone_വൃക്ഷം(അസത്യം),
                             VN_AS(valuep, കണ്ണി_വ്യഞ്ജകം)->clone_വൃക്ഷം(അസത്യം)};
        }
        Cross_മൂല്യം_Range range{valuep, resolveWidth(valuep, exprp)};
        എങ്കിൽ (!resolve_മൂല്യം(valuep, exprp, സത്യം, binp->isWildcard(), range)) {
            valuep->v3warn(E_UNSUPPORTED,
                           "Unsupported: non-integral value in a coverage bin of an "
                           "integral coverpoint.");
            തിരികെ പുതിയ AstConst{fl, AstConst::BitFalse{}};
        }
        എങ്കിൽ (crossRangeEmpty(range)) തിരികെ പുതിയ AstConst{fl, AstConst::BitFalse{}};
        AstConst* സ്ഥിരം lop = new_മൂല്യം_Const(fl, range.lo, exprp);
        AstConst* സ്ഥിരം hip = new_മൂല്യം_Const(fl, range.hi, exprp);
        Ast_കണ്ണി_വ്യഞ്ജകം* condp = nullptr;
        എങ്കിൽ (lop->അക്കം().isCaseEq(hip->അക്കം())) {
            condp = പുതിയ AstEq{fl, exprp->clone_വൃക്ഷം(അസത്യം), lop};
        } അല്ലെങ്കിൽ {
            condp = makeRangeCondition(fl, exprp, lop, hip);
            VL_DO_DANGLING(തള്ളുക_Deletep(lop), lop);
        }
        VL_DO_DANGLING(തള്ളുക_Deletep(hip), hip);
        എങ്കിൽ (!range.wildcard) തിരികെ condp;
        // Match the pattern's value bits within the source-value bounds.
        V3_സംഖ്യ mask{valuep, exprp->width()};
        V3_സംഖ്യ മൂല്യം{valuep, exprp->width()};
        mask.opBitsNonXZ(range.pattern);
        മൂല്യം.opBitsOne(range.pattern);
        AstConst* സ്ഥിരം maskConstp = new_മൂല്യം_Const(fl, mask, exprp);
        AstConst* സ്ഥിരം മൂല്യം_Constp = new_മൂല്യം_Const(fl, മൂല്യം, exprp);
        Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം വ്യഞ്ജകം_Masked = പുതിയ AstAnd{fl, exprp->clone_വൃക്ഷം(അസത്യം), maskConstp};
        Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം മൂല്യം_Masked = പുതിയ AstAnd{fl, മൂല്യം_Constp, maskConstp->clone_വൃക്ഷം(അസത്യം)};
        തിരികെ പുതിയ AstLogAnd{fl, condp, പുതിയ AstEq{fl, വ്യഞ്ജകം_Masked, മൂല്യം_Masked}};
    }

    ശൂന്യം generateCoverageComputationCode() {
        UINFO(4, "  Generating coverage computation code");

        // Invalidate cache: addMembersp() calls in generateCoverpointCode/generateCrossCode
        // have added new members since the last scan, so clear before re-querying.
        m_memberMap.clear();

        // get_inst_coverage(): the average of the coverpoints and crosses, weighted by their
        // option.weight (IEEE 1800-2023 19.11).  The instance node holds their runtimes.  With
        // the instances merged, get_coverage() instead, unless option.get_inst_coverage
        // (Table 19-1).
        AstFunc* സ്ഥിരം getInstCoveragep
            = VN_AS(m_memberMap.findMember(m_covergroupp, "get_inst_coverage"), Func);
        File_വരി* സ്ഥിരം instFl = getInstCoveragep->fileline();
        AstCMethodHard* സ്ഥിരം instCallp = instanceCall(instFl, VCMethod::COVERGROUP_COVERAGE);
        instCallp->dtypeSetDouble();
        Ast_കണ്ണി_വ്യഞ്ജകം* instValuep = instCallp;
        എങ്കിൽ (m_cgMayMerge) {
            Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം mergedp = പുതിയ AstLogAnd{
                instFl, newOptionSel(instFl, optionVar(സത്യം), "merge_instances", VAccess::READ),
                പുതിയ AstLogNot{instFl, newOptionSel(instFl, optionVar(അസത്യം), "get_inst_coverage",
                                                   VAccess::READ)}};
            instValuep = പുതിയ AstCond{instFl, mergedp, typeCoverageCall(instFl), instCallp};
        }
        getInstCoveragep->addStmtsp(പുതിയ AstAssign{
            instFl, പുതിയ AstVarRef{instFl, VN_AS(getInstCoveragep->fvarp(), Var), VAccess::WRITE},
            instValuep});

        // get_coverage(): the average of the covergroup's instances, weighted by their
        // option.weight, or with type_option.merge_instances, the coverage of the union of their
        // bins (IEEE 1800-2023 19.11.3).  Static, so the registry finds the instances.
        AstFunc* സ്ഥിരം getCoveragep
            = VN_AS(m_memberMap.findMember(m_covergroupp, "get_coverage"), Func);
        File_വരി* സ്ഥിരം typeFl = getCoveragep->fileline();
        getCoveragep->addStmtsp(പുതിയ AstAssign{
            typeFl, പുതിയ AstVarRef{typeFl, VN_AS(getCoveragep->fvarp(), Var), VAccess::WRITE},
            typeCoverageCall(typeFl)});
    }

    // The registry call computing the covergroup's type coverage, per its type options
    AstCMethodHard* typeCoverageCall(File_വരി* fl) {
        AstCExpr* സ്ഥിരം registryp = ctext(fl, "vlSymsp->_vm_contextp__->covergroupRegistryp()");
        registryp->dtypeSetVoid();  // Opaque receiver; only ever the 'fromp' of the call below
        AstCMethodHard* സ്ഥിരം callp
            = പുതിയ AstCMethodHard{fl, registryp, VCMethod::COVERGROUP_TYPE_COVERAGE};
        callp->addPinsp(ctext(fl, quoted(covergroupProtectedName())));
        callp->addPinsp(newOptionSel(fl, optionVar(സത്യം), "weight", VAccess::READ));
        callp->addPinsp(newOptionSel(fl, optionVar(സത്യം), "merge_instances", VAccess::READ));
        callp->addPinsp(file_വരി_Debug(m_covergroupp->fileline()));
        callp->usePtr(സത്യം);
        callp->dtypeSetDouble();
        തിരികെ callp;
    }

    // VISITORS
    സ്ഥിതം തർക്കം isEnclosingInstanceVar(സ്ഥിരം AstVar* varp) {
        തിരികെ varp->isClassMember() && !varp->lifetime().isStatic() && !varp->isParam();
    }

    ശൂന്യം rewriteThisRef(AstThisRef* refp, AstVar* handleVarp) {
        സ്ഥിരം AstClassRefDType* സ്ഥിരം refDTypep
            = VN_CAST(refp->dtypep()->skipRefp(), ClassRefDType);
        UASSERT_OBJ(refDTypep && refDTypep->classp() == m_covergroupp, refp,
                    "Unexpected this reference in embedded covergroup");
        Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം newp = പുതിയ AstVarRef{refp->fileline(), handleVarp, VAccess::READ};
        refp->replaceWith(newp);
        VL_DO_DANGLING(തള്ളുക_Deletep(refp), refp);
    }

    ശൂന്യം rewriteVarRef(AstVarRef* refp, AstVar* handleVarp) {
        File_വരി* സ്ഥിരം fl = refp->fileline();
        // As V3LinkLValue does, the handle of a written member select is marked written
        AstMemberSel* സ്ഥിരം selp
            = പുതിയ AstMemberSel{fl, പുതിയ AstVarRef{fl, handleVarp, refp->access()}, refp->varp()};
        selp->access(refp->access());
        refp->replaceWith(selp);
        VL_DO_DANGLING(തള്ളുക_Deletep(refp), refp);
    }

    ശൂന്യം rewriteFuncRef(AstFuncRef* refp, AstVar* handleVarp) {
        File_വരി* സ്ഥിരം fl = refp->fileline();
        AstArg* സ്ഥിരം argsp = refp->argsp() ? refp->argsp()->unlinkFrBackWithNext() : nullptr;
        AstMethodCall* സ്ഥിരം callp = പുതിയ AstMethodCall{
            fl, പുതിയ AstVarRef{fl, handleVarp, VAccess::READ}, refp->name(), argsp};
        callp->taskp(refp->taskp());
        callp->dtypeFrom(refp);
        refp->replaceWith(callp);
        VL_DO_DANGLING(തള്ളുക_Deletep(refp), refp);
    }

    // True if funcp is an instance method of the enclosing class or one of its bases
    തർക്കം isEnclosingInstanceFunc(സ്ഥിരം Ast_കണ്ണി_FTask* funcp) സ്ഥിരം {
        എങ്കിൽ (!funcp->classMethod() || funcp->isStatic()) തിരികെ അസത്യം;
        തിരികെ AstClass::isClassExtendedFrom(m_enclosingClassp, VN_AS(funcp->aboveLoopp(), Class));
    }

    തർക്കം isEmbeddedCovergroupVar(സ്ഥിരം AstVar* varp) സ്ഥിരം {
        എങ്കിൽ (!varp || !varp->isClassMember() || varp->isDeclTyped()) തിരികെ അസത്യം;
        സ്ഥിരം AstClassRefDType* സ്ഥിരം refp = VN_CAST(varp->dtypep()->skipRefp(), ClassRefDType);
        തിരികെ refp && refp->classp() == m_covergroupp;
    }

    AstVar* findEmbeddedCovergroupVar() സ്ഥിരം {
        എങ്കിൽ (!m_enclosingClassp) തിരികെ nullptr;
        വേണ്ടി (Ast_കണ്ണി* itemp = m_enclosingClassp->membersp(); itemp; itemp = itemp->nextp()) {
            എങ്കിൽ (AstVar* സ്ഥിരം varp = VN_CAST(itemp, Var)) {
                എങ്കിൽ (isEmbeddedCovergroupVar(varp)) തിരികെ varp;
            }
        }
        // V3LinkParse always creates an implicit variable for an embedded covergroup.
        തിരികെ nullptr;  // LCOV_EXCL_LINE
    }

    std::vector<Ast_കണ്ണി_Assign*> findCovergroupConstructions() {
        std::vector<Ast_കണ്ണി_Assign*> foundps;
        എങ്കിൽ (!m_embeddedVarp) തിരികെ foundps;
        AstFunc* സ്ഥിരം enclosingNewp
            = VN_CAST(m_memberMap.findMember(m_enclosingClassp, "new"), Func);
        എങ്കിൽ (!enclosingNewp) തിരികെ foundps;
        enclosingNewp->foreach([&](Ast_കണ്ണി_Assign* asgnp) {
            സ്ഥിരം AstNew* സ്ഥിരം newp = VN_CAST(asgnp->rhsp(), New);
            സ്ഥിരം AstVarRef* സ്ഥിരം lhsRefp = VN_CAST(asgnp->lhsp(), VarRef);
            എങ്കിൽ (!newp || !lhsRefp || lhsRefp->varp() != m_embeddedVarp) തിരികെ;
            സ്ഥിരം AstClassRefDType* സ്ഥിരം refp = VN_CAST(newp->dtypep(), ClassRefDType);
            എങ്കിൽ (refp && refp->classp() == m_covergroupp) foundps.തള്ളുക_back(asgnp);
        });
        തിരികെ foundps;
    }

    std::set<സ്ഥിരം AstVar*> enclosingInstanceVars() സ്ഥിരം {
        std::set<സ്ഥിരം AstVar*> vars;
        എങ്കിൽ (m_enclosingClassp) {
            m_enclosingClassp->foreachMember([&](AstClass* സ്ഥിരം, AstVar* സ്ഥിരം varp) {
                എങ്കിൽ (isEnclosingInstanceVar(varp)) vars.തിരുകുക(varp);
            });
        }
        തിരികെ vars;
    }

    തർക്കം hasEnclosingEventRef(AstCovergroup* cgp) സ്ഥിരം {
        എങ്കിൽ (!m_embeddedVarp || !cgp->eventp()) തിരികെ അസത്യം;
        സ്ഥിരം std::set<സ്ഥിരം AstVar*> enclosingVars = enclosingInstanceVars();
        തർക്കം found = അസത്യം;
        cgp->eventp()->foreach([&](AstVarRef* refp) {
            എങ്കിൽ (enclosingVars.എണ്ണം(refp->varp())) found = സത്യം;
        });
        തിരികെ found;
    }

    സ്ഥിതം തർക്കം parse_Embedded_Event_വ്യഞ്ജകം(Ast_കണ്ണി_വ്യഞ്ജകം* exprp, AstVar*& baseVarp,
                                       AstVar*& memberVarp) {
        എങ്കിൽ (AstVarRef* സ്ഥിരം refp = VN_CAST(exprp, VarRef)) {
            baseVarp = refp->varp();
            memberVarp = nullptr;
            തിരികെ സത്യം;
        }
        AstMemberSel* സ്ഥിരം selp = VN_CAST(exprp, MemberSel);
        എങ്കിൽ (!selp) തിരികെ അസത്യം;
        AstVarRef* സ്ഥിരം baseRefp = VN_CAST(selp->fromp(), VarRef);
        എങ്കിൽ (!baseRefp) തിരികെ അസത്യം;
        baseVarp = baseRefp->varp();
        memberVarp = selp->varp();
        തിരികെ സത്യം;
    }

    തർക്കം isEventLvalue(Ast_കണ്ണി_വ്യഞ്ജകം* exprp, സ്ഥിരം EmbeddedEventTrigger& trigger) സ്ഥിരം {
        എങ്കിൽ (AstSel* സ്ഥിരം selp = VN_CAST(exprp, Sel)) exprp = selp->fromp();
        AstVar* baseVarp = nullptr;
        AstVar* memberVarp = nullptr;
        എങ്കിൽ (!parse_Embedded_Event_വ്യഞ്ജകം(exprp, baseVarp, memberVarp)) തിരികെ അസത്യം;
        തിരികെ baseVarp == trigger.baseVarp && memberVarp == trigger.memberVarp;
    }

    Ast_കണ്ണി_വ്യഞ്ജകം* newEventRead(File_വരി* fl, സ്ഥിരം EmbeddedEventTrigger& trigger) സ്ഥിരം {
        Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം basep = പുതിയ AstVarRef{fl, trigger.baseVarp, VAccess::READ};
        എങ്കിൽ (!trigger.memberVarp) തിരികെ basep;
        AstMemberSel* സ്ഥിരം selp = പുതിയ AstMemberSel{fl, basep, trigger.memberVarp};
        selp->access(VAccess::READ);
        തിരികെ selp;
    }

    string eventPrevName(സ്ഥിരം EmbeddedEventTrigger& trigger, size_t triggerIndex) സ്ഥിരം {
        string name = "__Vcg_prev_" + m_embeddedVarp->name() + "_" + std::to_string(triggerIndex)
                      + "_" + trigger.baseVarp->name();
        എങ്കിൽ (trigger.memberVarp) name += "_" + trigger.memberVarp->name();
        തിരികെ name;
    }

    Ast_കണ്ണി_വ്യഞ്ജകം* newEmbeddedVarNonNull(File_വരി* fl) സ്ഥിരം {
        തിരികെ പുതിയ AstNeq{fl, പുതിയ AstVarRef{fl, m_embeddedVarp, VAccess::READ},
                          പുതിയ AstConst{fl, AstConst::Null{}}};
    }

    Ast_കണ്ണി_Stmt* newSampleStmt(File_വരി* fl) സ്ഥിരം {
        AstMethodCall* സ്ഥിരം callp = പുതിയ AstMethodCall{
            fl, പുതിയ AstVarRef{fl, m_embeddedVarp, VAccess::READ}, "sample", nullptr};
        callp->taskp(m_sampleFuncp);
        callp->dtypeSetVoid();
        തിരികെ callp->makeStmt();
    }

    ശൂന്യം installEmbeddedEventFork(Ast_Sen_വൃക്ഷം* eventp,
                                  സ്ഥിരം std::vector<Ast_കണ്ണി_Assign*>& constructps) {
        // IEEE 1800-2023 19.3 samples coverpoints whenever their clocking event occurs. A
        // per-instance event cannot use V3Active's static sensitivity path, so spawn
        // 'fork forever begin @(event); cg.sample(); end join_none' after each construction.
        വേണ്ടി (Ast_കണ്ണി_Assign* സ്ഥിരം constructp : constructps) {
            File_വരി* സ്ഥിരം fl = constructp->fileline();
            AstLoop* സ്ഥിരം loopp = പുതിയ AstLoop{fl};
            loopp->addStmtsp(പുതിയ AstEventControl{fl, eventp->clone_വൃക്ഷം(അസത്യം), nullptr});
            loopp->addStmtsp(പുതിയ AstIf{fl, newEmbeddedVarNonNull(fl), newSampleStmt(fl)});
            AstFork* സ്ഥിരം forkp = പുതിയ AstFork{fl, VJoinType::JOIN_NONE};
            forkp->immediateStart(സത്യം);
            forkp->addForksp(പുതിയ AstBegin{fl, "", loopp, സത്യം});
            constructp->addNextHere(forkp);
        }
        VL_DO_DANGLING(തള്ളുക_Deletep(eventp), eventp);
    }

    Ast_കണ്ണി_വ്യഞ്ജകം* newEventReadyCondition(File_വരി* fl, സ്ഥിരം EmbeddedEventTrigger& trigger) സ്ഥിരം {
        Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം curp = newEventRead(fl, trigger);
        Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം prevp = പുതിയ AstVarRef{fl, trigger.prevVarp, VAccess::READ};
        Ast_കണ്ണി_വ്യഞ്ജകം* edgep = nullptr;
        // IEEE 1800-2023 9.4.2 detects edge-qualified events only on the expression's LSB,
        // while an implicit change event observes the complete expression.
        എങ്കിൽ (trigger.edgeType == VEdgeType::ET_POSEDGE) {
            edgep = പുതിയ AstSel{fl, പുതിയ AstAnd{fl, curp, പുതിയ AstNot{fl, prevp}}, 0, 1};
        } അല്ലെങ്കിൽ എങ്കിൽ (trigger.edgeType == VEdgeType::ET_NEGEDGE) {
            edgep = പുതിയ AstSel{fl, പുതിയ AstAnd{fl, പുതിയ AstNot{fl, curp}, prevp}, 0, 1};
        } അല്ലെങ്കിൽ എങ്കിൽ (trigger.edgeType == VEdgeType::ET_BOTHEDGE) {
            edgep = പുതിയ AstSel{fl, പുതിയ AstXor{fl, curp, prevp}, 0, 1};
        } അല്ലെങ്കിൽ {
            edgep = പുതിയ AstNeq{fl, curp, prevp};
        }
        തിരികെ പുതിയ AstLogAnd{fl, newEmbeddedVarNonNull(fl), edgep};
    }

    std::vector<EmbeddedEventTrigger> collectEmbeddedEventTriggers(AstCovergroup* cgp) {
        std::vector<EmbeddedEventTrigger> triggers;
        സ്ഥിരം std::set<സ്ഥിരം AstVar*> enclosingVars = enclosingInstanceVars();
        വേണ്ടി (Ast_കണ്ണി* senp = cgp->eventp()->sensesp(); senp; senp = senp->nextp()) {
            AstSenItem* സ്ഥിരം itemp = VN_AS(senp, SenItem);
            AstVar* baseVarp = nullptr;
            AstVar* memberVarp = nullptr;
            എങ്കിൽ (!parse_Embedded_Event_വ്യഞ്ജകം(itemp->sensp(), baseVarp, memberVarp)
                || !enclosingVars.എണ്ണം(baseVarp)) {
                തിരികെ {};
            }
            triggers.emplace_back(itemp->fileline(), baseVarp, memberVarp, itemp->edgeType());
        }
        തിരികെ triggers;
    }

    ശൂന്യം installEmbeddedEventTriggers(std::vector<EmbeddedEventTrigger>& triggers,
                                      സ്ഥിരം std::vector<Ast_കണ്ണി_Assign*>& constructps) {
        // Without --timing, approximate a per-instance event by sampling after assignments
        // within the enclosing class. External writes and exact scheduling cannot be observed.
        എങ്കിൽ (constructps.empty()) തിരികെ;
        വേണ്ടി (size_t triggerIndex = 0; triggerIndex < triggers.size(); ++triggerIndex) {
            EmbeddedEventTrigger& trigger = triggers[triggerIndex];
            std::vector<Ast_കണ്ണി_Assign*> assignps;
            m_enclosingClassp->foreach([&](Ast_കണ്ണി_Assign* asgnp) {
                എങ്കിൽ (isEventLvalue(asgnp->lhsp(), trigger)) assignps.തള്ളുക_back(asgnp);
            });
            എങ്കിൽ (assignps.empty()) {
                trigger.eventFl->v3warn(
                    COVERIGN, "Unsupported: 'covergroup' clocking event signal has no assignment "
                              "within the enclosing class; no coverage sampled. Use --timing for "
                              "full support.");
                തുടരുക;
            }
            Ast_കണ്ണി_DType* സ്ഥിരം dtypep
                = trigger.memberVarp ? trigger.memberVarp->dtypep() : trigger.baseVarp->dtypep();
            AstVar* സ്ഥിരം prevVarp = പുതിയ AstVar{trigger.eventFl, VVarType::MEMBER,
                                                eventPrevName(trigger, triggerIndex), dtypep};
            m_enclosingClassp->addMembersp(prevVarp);
            trigger.prevVarp = prevVarp;
            വേണ്ടി (Ast_കണ്ണി_Assign* സ്ഥിരം asgnp : assignps) {
                File_വരി* സ്ഥിരം fl = asgnp->fileline();
                AstIf* സ്ഥിരം ifp
                    = പുതിയ AstIf{fl, newEventReadyCondition(fl, trigger), newSampleStmt(fl)};
                ifp->addNextHere(പുതിയ AstAssign{fl,
                                               പുതിയ AstVarRef{fl, trigger.prevVarp, VAccess::WRITE},
                                               newEventRead(fl, trigger)});
                asgnp->addNextHere(ifp);
            }
        }
    }

    ശൂന്യം delete_Coverage_ഇനങ്ങൾ() {
        വേണ്ടി (AstCoverpoint* സ്ഥിരം cpp : m_coverpoints) {
            VL_DO_DANGLING(തള്ളുക_Deletep(cpp->unlinkFrBack()), cpp);
        }
        വേണ്ടി (AstCoverCross* സ്ഥിരം crossp : m_coverCrosses) {
            VL_DO_DANGLING(തള്ളുക_Deletep(crossp->unlinkFrBack()), crossp);
        }
        // Options not lowered: the covergroup was not processed
        വേണ്ടി (AstCgOptionAssign* സ്ഥിരം optp : m_cgOptions) {
            VL_DO_DANGLING(തള്ളുക_Deletep(optp->unlinkFrBack()), optp);
        }
        m_cgOptions.clear();
    }

    വർഗ്ഗം FormalRefVisitor final : പൊതു VNVisitor {
        സ്ഥിരം std::map<സ്ഥിരം AstVar*, AstVar*>& m_replacements;

        ശൂന്യം visit(AstVarRef* nodep) override {
            സ്ഥിരം auto it = m_replacements.find(nodep->varp());
            എങ്കിൽ (it == m_replacements.end()) തിരികെ;
            nodep->varp(it->second);
        }
        ശൂന്യം visit(Ast_കണ്ണി* nodep) override { iterateChildren(nodep); }

    പൊതു:
        explicit FormalRefVisitor(സ്ഥിരം std::map<സ്ഥിരം AstVar*, AstVar*>& replacements)
            : m_replacements{replacements} {}
        ശൂന്യം scan(Ast_കണ്ണി* nodep) { iterate(nodep); }
    };

    ശൂന്യം validateCovergroupExpressions() {
        std::set<സ്ഥിരം AstVar*> sampleMembers;
        std::set<സ്ഥിരം AstVar*> constructorRefMembers;
        വേണ്ടി (Ast_കണ്ണി* stmtp = m_constructorp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            സ്ഥിരം AstVar* സ്ഥിരം varp = VN_CAST(stmtp, Var);
            എങ്കിൽ (!varp || !varp->isIO() || (!varp->isRef() && !varp->isConstRef())) തുടരുക;
            സ്ഥിരം AstVar* സ്ഥിരം memberp
                = VN_CAST(m_memberMap.findMember(m_covergroupp, varp->name()), Var);
            UASSERT_OBJ(memberp && memberp->isClassMember(), varp,
                        "Covergroup constructor argument missing persistent member");
            constructorRefMembers.തിരുകുക(varp);
            constructorRefMembers.തിരുകുക(memberp);
        }
        വേണ്ടി (Ast_കണ്ണി* stmtp = m_sampleFuncp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            സ്ഥിരം AstVar* സ്ഥിരം varp = VN_CAST(stmtp, Var);
            എങ്കിൽ (!varp || !varp->isIO()) തുടരുക;
            സ്ഥിരം AstVar* സ്ഥിരം memberp
                = VN_CAST(m_memberMap.findMember(m_covergroupp, varp->name()), Var);
            UASSERT_OBJ(memberp && memberp->isClassMember(), varp,
                        "Covergroup sample argument missing persistent member");
            sampleMembers.തിരുകുക(memberp);
        }
        Covergroup_വ്യഞ്ജകം_Valid_Visitor{sampleMembers, constructorRefMembers}.scan(m_constructorp);
    }

    ശൂന്യം rebindFormalRefs() {
        std::map<സ്ഥിരം AstVar*, AstVar*> replacements;
        വേണ്ടി (Ast_കണ്ണി* stmtp = m_constructorp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            എങ്കിൽ (സ്ഥിരം AstVar* സ്ഥിരം varp = VN_CAST(stmtp, Var)) {
                എങ്കിൽ (!varp->isIO()) തുടരുക;
                AstVar* സ്ഥിരം memberp
                    = VN_CAST(m_memberMap.findMember(m_covergroupp, varp->name()), Var);
                UASSERT_OBJ(memberp && memberp->isClassMember(), varp,
                            "Covergroup constructor argument missing persistent member");
                replacements.emplace(varp, memberp);
            }
        }
        size_t expectedBindings = 0;
        വേണ്ടി (സ്ഥിരം auto& pair : replacements) {
            എങ്കിൽ (pair.first->isRef() || pair.first->isConstRef()) ++expectedBindings;
        }
        size_t rewrittenBindings = 0;
        വേണ്ടി (Ast_കണ്ണി* stmtp = m_constructorp->stmtsp(); stmtp;) {
            Ast_കണ്ണി* സ്ഥിരം nextp = stmtp->nextp();
            എങ്കിൽ (AstAssign* സ്ഥിരം assignp = VN_CAST(stmtp, Assign)) {
                AstVarRef* സ്ഥിരം lhsp = VN_CAST(assignp->lhsp(), VarRef);
                AstVarRef* സ്ഥിരം rhsp = VN_CAST(assignp->rhsp(), VarRef);
                എങ്കിൽ (lhsp && rhsp
                    && (lhsp->varp()->declDirection() == VDirection::REF
                        || lhsp->varp()->declDirection() == VDirection::CONSTREF)) {
                    സ്ഥിരം auto it = replacements.find(rhsp->varp());
                    UASSERT_OBJ(it != replacements.end() && it->second == lhsp->varp(), assignp,
                                "Unexpected covergroup reference binding assignment");
                    AstCExpr* സ്ഥിരം bindp = പുതിയ AstCExpr{assignp->fileline(), ""};
                    bindp->add(lhsp->unlinkFrBack());
                    bindp->add(" = &");
                    bindp->add(rhsp->unlinkFrBack());
                    assignp->replaceWith(bindp->makeStmt());
                    VL_DO_DANGLING(തള്ളുക_Deletep(assignp), assignp);
                    ++rewrittenBindings;
                }
            }
            stmtp = nextp;
        }
        UASSERT_OBJ(rewrittenBindings == expectedBindings, m_constructorp,
                    "Covergroup reference argument missing binding");
        FormalRefVisitor visitor{replacements};
        വേണ്ടി (AstCoverpoint* സ്ഥിരം cpp : m_coverpoints) visitor.scan(cpp);
        വേണ്ടി (AstCoverCross* സ്ഥിരം crossp : m_coverCrosses) visitor.scan(crossp);
    }

    AstVarRef* installEnclosingBackPointer(സ്ഥിരം std::vector<Ast_കണ്ണി_Assign*>& constructps) {
        // Simple-case support for embedded covergroups (IEEE 1800-2023 19.4) whose
        // coverpoints reference members of the enclosing class ("Class members can be used
        // in coverpoint expressions").  The covergroup is lowered into a sibling class with
        // no implicit handle to the enclosing object, so such references would emit
        // uncompilable C++.  Add an explicit back-pointer member to the enclosing instance,
        // route member references through it, and pass it into the constructor so
        // coverage initialization can read enclosing members. Returns an invalid
        // reference if an outer class member cannot be reached; otherwise returns an empty result.
        എങ്കിൽ (!m_enclosingClassp) തിരികെ nullptr;  // Offending refs require an enclosing class

        AstVarRef* invalidp = nullptr;
        Ast_കണ്ണി* offenderp = nullptr;
        std::set<സ്ഥിരം AstVar*> ownVars;
        വേണ്ടി (Ast_കണ്ണി* itemp = m_covergroupp->membersp(); itemp; itemp = itemp->nextp()) {
            എങ്കിൽ (സ്ഥിരം AstVar* സ്ഥിരം varp = VN_CAST(itemp, Var)) ownVars.തിരുകുക(varp);
        }
        സ്ഥിരം std::set<സ്ഥിരം AstVar*> enclosingVars = enclosingInstanceVars();
        std::vector<AstVarRef*> refsToRewrite;
        std::vector<AstThisRef*> thisRefsToRewrite;
        std::vector<AstFuncRef*> funcRefsToRewrite;
        സ്ഥിരം auto scan = [&](Ast_കണ്ണി* rootp) {
            rootp->foreach([&](AstVarRef* refp) {
                എങ്കിൽ (invalidp) തിരികെ;
                സ്ഥിരം AstVar* സ്ഥിരം varp = refp->varp();
                എങ്കിൽ (!isEnclosingInstanceVar(varp) || ownVars.എണ്ണം(varp)) തിരികെ;
                എങ്കിൽ (!enclosingVars.എണ്ണം(varp)) {
                    invalidp = refp;
                    തിരികെ;
                }
                refsToRewrite.തള്ളുക_back(refp);
                എങ്കിൽ (!offenderp) offenderp = refp;
            });
            എങ്കിൽ (invalidp) തിരികെ;
            rootp->foreach([&](AstThisRef* refp) {
                സ്ഥിരം AstClassRefDType* സ്ഥിരം refDTypep
                    = VN_CAST(refp->dtypep()->skipRefp(), ClassRefDType);
                എങ്കിൽ (refDTypep && refDTypep->classp() == m_covergroupp) {
                    thisRefsToRewrite.തള്ളുക_back(refp);
                    എങ്കിൽ (!offenderp) offenderp = refp;
                }
            });
            rootp->foreach([&](AstFuncRef* refp) {
                എങ്കിൽ (!isEnclosingInstanceFunc(refp->taskp())) തിരികെ;
                funcRefsToRewrite.തള്ളുക_back(refp);
                എങ്കിൽ (!offenderp) offenderp = refp;
            });
        };
        വേണ്ടി (AstCoverpoint* സ്ഥിരം cpp : m_coverpoints) scan(cpp);
        വേണ്ടി (AstCoverCross* സ്ഥിരം crossp : m_coverCrosses) scan(crossp);
        വേണ്ടി (AstCgOptionAssign* സ്ഥിരം optp : m_cgOptions) scan(optp);
        എങ്കിൽ (invalidp || !offenderp) തിരികെ invalidp;

        UASSERT_OBJ(m_embeddedVarp, m_covergroupp, "Embedded covergroup variable not found");
        // Commit: add the back-pointer member, rewrite the references, initialize the handle.
        File_വരി* സ്ഥിരം fl = m_covergroupp->fileline();
        AstClassRefDType* സ്ഥിരം enclDTypep = പുതിയ AstClassRefDType{fl, m_enclosingClassp, nullptr};
        enclDTypep->rawPointer(സത്യം);
        v3Global.rootp()->typeTablep()->addTypesp(enclDTypep);
        AstVar* സ്ഥിരം handleVarp
            = പുതിയ AstVar{fl, VVarType::MEMBER, "__Vcg_enclosingp", enclDTypep};
        m_covergroupp->addMembersp(handleVarp);
        AstVar* സ്ഥിരം argumentp = പുതിയ AstVar{fl, VVarType::BLOCKTEMP, "__Vcg_parentp", enclDTypep};
        argumentp->direction(VDirection::നിവേശം);
        argumentp->declDirection(VDirection::നിവേശം);
        argumentp->funcLocal(സത്യം);
        argumentp->noReset(സത്യം);
        argumentp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
        m_constructorp->addStmtsp(argumentp);
        m_constructorp->prependStmtsp(പുതിയ AstAssign{fl, memberRef(fl, handleVarp, VAccess::WRITE),
                                                    പുതിയ AstVarRef{fl, argumentp, VAccess::READ}});

        // Route each enclosing-member reference through the back-pointer: 'm' -> 'h.m'.
        വേണ്ടി (AstVarRef* സ്ഥിരം refp : refsToRewrite) { rewriteVarRef(refp, handleVarp); }
        വേണ്ടി (AstThisRef* സ്ഥിരം refp : thisRefsToRewrite) { rewriteThisRef(refp, handleVarp); }
        വേണ്ടി (AstFuncRef* സ്ഥിരം refp : funcRefsToRewrite) { rewriteFuncRef(refp, handleVarp); }

        // Append a named hidden argument to preserve positional and defaulted user arguments.
        വേണ്ടി (Ast_കണ്ണി_Assign* സ്ഥിരം constructp : constructps) {
            File_വരി* സ്ഥിരം cfl = constructp->fileline();
            AstCExpr* സ്ഥിരം thisp = പുതിയ AstCExpr{cfl, "this"};
            thisp->dtypep(enclDTypep);
            VN_AS(constructp->rhsp(), New)->addArgsp(പുതിയ AstArg{cfl, argumentp->name(), thisp});
        }
        തിരികെ nullptr;
    }

    ശൂന്യം visit(AstClass* nodep) override {
        UINFO(9, "Visiting class: " << nodep->name() << " isCovergroup=" << nodep->isCovergroup());
        എങ്കിൽ (nodep->isCovergroup()) {
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
            Ast_Sen_വൃക്ഷം* embeddedEventForkp = nullptr;

            // Extract and store the clocking event from AstCovergroup node
            // The parser creates this node to preserve the event information
            തർക്കം hasUnsupportedEvent = അസത്യം;
            വേണ്ടി (Ast_കണ്ണി* itemp = nodep->membersp(); itemp;) {
                Ast_കണ്ണി* സ്ഥിരം nextp = itemp->nextp();
                എങ്കിൽ (AstCovergroup* സ്ഥിരം cgp = VN_CAST(itemp, Covergroup)) {
                    // Store the event in the global map for V3Active to retrieve later
                    // V3LinkParse only creates this sentinel AstCovergroup node when a clocking
                    // event exists, so cgp->eventp() is always non-null here.
                    UASSERT_OBJ(cgp->eventp(), cgp,
                                "Sentinel AstCovergroup in class must have non-null eventp");
                    എങ്കിൽ (hasEnclosingEventRef(cgp)) {
                        UASSERT_OBJ(m_embeddedVarp, cgp,
                                    "Embedded covergroup event has no instance variable");
                        എങ്കിൽ (v3Global.opt.timing().isSetTrue()) {
                            embeddedEventForkp = cgp->eventp()->unlinkFrBack();
                        } അല്ലെങ്കിൽ {
                            embeddedEventTriggers = collectEmbeddedEventTriggers(cgp);
                            എങ്കിൽ (embeddedEventTriggers.empty()) {
                                cgp->v3warn(COVERIGN,
                                            "Unsupported: 'covergroup' clocking event on complex "
                                            "member expression; use --timing for full support.");
                                hasUnsupportedEvent = സത്യം;
                            }
                        }
                        VL_DO_DANGLING(തള്ളുക_Deletep(cgp->unlinkFrBack()), cgp);
                        itemp = nextp;
                        തുടരുക;
                    }
                    // V3Active handles events that do not depend on an enclosing instance.
                    UINFO(4, "Keeping covergroup event node for V3Active: " << nodep->name());
                    itemp = nextp;
                    തുടരുക;
                }
                itemp = nextp;
            }

            // Find the sample() method and constructor
            m_sampleFuncp = VN_CAST(m_memberMap.findMember(nodep, "sample"), Func);
            // V3LinkParse always synthesizes a sample() method for every covergroup, and the
            // sampling-code generation below dereferences m_sampleFuncp unconditionally.
            UASSERT_OBJ(m_sampleFuncp, nodep, "Covergroup missing synthesized sample() method");
            m_sampleFuncp->isCovergroupSample(സത്യം);
            m_constructorp = VN_CAST(m_memberMap.findMember(nodep, "new"), Func);
            UINFO(9, "Found sample() method: " << (m_sampleFuncp ? "yes" : "no"));
            UINFO(9, "Found constructor: " << (m_constructorp ? "yes" : "no"));

            // If covergroup has unsupported clocking event, skip processing it
            // but still clean up coverpoints so they don't reach downstream passes
            എങ്കിൽ (hasUnsupportedEvent) {
                iterateChildren(nodep);
                validateCovergroupExpressions();
                rebindFormalRefs();
                delete_Coverage_ഇനങ്ങൾ();
                എങ്കിൽ (embeddedEventForkp) {
                    VL_DO_DANGLING(തള്ളുക_Deletep(embeddedEventForkp), embeddedEventForkp);
                }
                തിരികെ;
            }

            iterateChildren(nodep);
            validateCovergroupExpressions();
            rebindFormalRefs();
            സ്ഥിരം std::vector<Ast_കണ്ണി_Assign*> constructps = findCovergroupConstructions();

            // Embedded covergroups (IEEE 1800-2023 19.4): coverpoints, iff expressions, and
            // crosses may reference members of the enclosing class. The covergroup is lowered
            // into a sibling class with no implicit handle to the enclosing instance. Install
            // an explicit back-pointer and route the references through it.
            എങ്കിൽ (AstVarRef* സ്ഥിരം invalidp = installEnclosingBackPointer(constructps)) {
                invalidp->v3error("Non-static member "
                                  << invalidp->varp()->prettyNameQ()
                                  << " of an outer class requires an explicit "
                                     "object handle (IEEE 1800-2023 8.23).");
                delete_Coverage_ഇനങ്ങൾ();
                എങ്കിൽ (embeddedEventForkp) {
                    VL_DO_DANGLING(തള്ളുക_Deletep(embeddedEventForkp), embeddedEventForkp);
                }
                തിരികെ;
            }
            installEmbeddedEventTriggers(embeddedEventTriggers, constructps);
            എങ്കിൽ (embeddedEventForkp) installEmbeddedEventFork(embeddedEventForkp, constructps);
            processCovergroup();
            // Remove lowered coverpoints/crosses from the class - they have been
            // fully translated into C++ code and must not reach downstream passes
            delete_Coverage_ഇനങ്ങൾ();
        } അല്ലെങ്കിൽ {
            // Track the lexically enclosing class so a nested covergroup can resolve
            // references to the enclosing object's members (installEnclosingBackPointer).
            VL_RESTORER(m_enclosingClassp);
            m_enclosingClassp = nodep;
            iterateChildren(nodep);
        }
    }

    ശൂന്യം visit(AstCoverpoint* nodep) override {
        UINFO(9, "Found coverpoint: " << nodep->name());
        m_coverpoints.തള്ളുക_back(nodep);
        m_coverpointMap.emplace(nodep->name(), nodep);
        iterateChildren(nodep);
    }

    ശൂന്യം visit(AstCoverCross* nodep) override {
        UINFO(9, "Found cross: " << nodep->name());
        m_coverCrosses.തള്ളുക_back(nodep);
        iterateChildren(nodep);
    }

    // V3Width leaves only the covergroup-level options lowerCovergroupOptions() takes
    ശൂന്യം visit(AstCgOptionAssign* nodep) override { m_cgOptions.തള്ളുക_back(nodep); }

    // A package, interface, or module, so the design unit declaring the classes within
    ശൂന്യം visit(Ast_കണ്ണി_Module* nodep) override {
        VL_RESTORER(m_unitp);
        m_unitp = nodep;
        iterateChildren(nodep);
    }

    ശൂന്യം visit(Ast_കണ്ണി* nodep) override { iterateChildren(nodep); }

പൊതു:
    // CONSTRUCTORS
    FunctionalCoverageVisitor(AstNetlist* nodep,
                              സ്ഥിരം std::set<സ്ഥിരം AstVar*>& mergeableTypeOptions)
        : m_mergeableTypeOptions{mergeableTypeOptions} {
        iterate(nodep);
    }
    ~FunctionalCoverageVisitor() override = സ്ഥിരസ്ഥിതി;
};

//######################################################################
// Functional coverage class functions

ശൂന്യം V3Covergroup::covergroup(AstNetlist* nodep) {
    UINFO(4, __FUNCTION__ << ": ");
    സ്ഥിരം CovergroupAssignValidVisitor validVisitor{nodep};
    എങ്കിൽ (!validVisitor.valid()) V3Error::abortIfErrors();
    {  // Destruct before checking
        FunctionalCoverageVisitor{nodep, validVisitor.mergeableTypeOptions()};
    }
    V3Global::dump_Check_Global_വൃക്ഷം("coveragefunc", 0, dump_വൃക്ഷം_Either_Level() >= 3);
}
