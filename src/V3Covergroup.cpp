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

#અન્તર્ભાવ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#અન્તર્ભાવ "V3Covergroup.h"

#અન્તર્ભાવ "V3Const.h"
#અન્તર્ભાવ "V3Error.h"
#અન્તર્ભાવ "V3File.h"
#અન્તર્ભાવ "V3MemberMap.h"
#અન્તર્ભાવ "V3UniqueNames.h"

#અન્તર્ભાવ <bitset>
#અન્તર્ભાવ <cmath>
#અન્તર્ભાવ <deque>
#અન્તર્ભાવ <set>
#અન્તર્ભાવ <tuple>
#અન્તર્ભાવ <unordered_map>
#અન્તર્ભાવ <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Embedded covergroup assignment validation; and in the same pass over the netlist, the
// type_option variables through which SystemVerilog may set type_option.merge_instances

વર્ગ CovergroupAssignValidVisitor final : જાહેર VNVisitorConst {
    VMemberMap m_memberMap;
    std::map<અચલ AstVar*, અચલ Ast_ગાંઠ_FTask*>
        m_constructors;  // Implicit instance -> constructor
    std::set<અચલ AstVar*> m_mergeableTypeOptions;  // See mergeableTypeOptions()
    અચલ Ast_ગાંઠ_FTask* m_ftaskp = nullptr;
    તાર્કિક m_collecting = સાચું;
    તાર્કિક m_valid = સાચું;

    શૂન્ય visit(AstClass* nodep) override {
        VL_RESTORER(m_ftaskp);
        m_ftaskp = nullptr;
        જો (m_collecting) {
            અચલ Ast_ગાંઠ_FTask* અચલ constructorp
                = VN_CAST(m_memberMap.findMember(nodep, "new"), ગાંઠ_FTask);
            માટે (અચલ Ast_ગાંઠ* itemp = nodep->membersp(); itemp; itemp = itemp->nextp()) {
                અચલ AstVar* અચલ varp = VN_CAST(itemp, Var);
                // Only the implicit instance is restricted, not explicitly typed aliases.
                જો (!varp || !varp->isClassMember() || varp->isDeclTyped()) ચાલુ;
                અચલ AstClassRefDType* અચલ refp
                    = VN_CAST(varp->dtypep()->skipRefp(), ClassRefDType);
                જો (refp && refp->classp()->covergroupEnclosingClassp() == nodep) {
                    m_constructors.emplace(varp, constructorp);
                }
            }
        }
        iterateChildrenConst(nodep);
    }
    શૂન્ય visit(Ast_ગાંઠ_FTask* nodep) override {
        VL_RESTORER(m_ftaskp);
        m_ftaskp = nodep;
        iterateChildrenConst(nodep);
    }
    શૂન્ય visit(Ast_ગાંઠ_Assign* nodep) override {
        જો (!m_collecting) {
            અચલ AstVar* varp = nullptr;
            જો (અચલ Ast_ગાંઠ_Var_Ref* અચલ refp = VN_CAST(nodep->lhsp(), ગાંઠ_Var_Ref)) {
                varp = refp->varp();
            } નહીંતર જો (અચલ AstMemberSel* અચલ selp = VN_CAST(nodep->lhsp(), MemberSel)) {
                varp = selp->varp();
            }
            અચલ auto it = m_constructors.find(varp);
            જો (it != m_constructors.end() && (!m_ftaskp || m_ftaskp != it->second)) {
                m_valid = ખોટું;
                nodep->v3error("Embedded covergroup variable "
                               << varp->prettyNameQ()
                               << " may only be assigned in the enclosing class's 'new' method "
                                  "(IEEE 1800-2023 19.4).");
            }
        }
        iterateChildrenConst(nodep);
    }
    શૂન્ય visit(Ast_ગાંઠ_Var_Ref* nodep) override {
        // Type options may be set at any time (IEEE 1800-2023 19.7.1), so SystemVerilog may set
        // type_option.merge_instances with any use of a type_option but a read of another
        // member, as not every write is marked yet (std::randomize)
        જો (m_collecting && nodep->varp()->name() == "type_option") {
            અચલ AstStructSel* અચલ selp = VN_CAST(nodep->backp(), StructSel);
            જો (!selp || selp->fromp() != nodep || selp->name() == "merge_instances"
                || !nodep->access().isReadOnly()) {
                m_mergeableTypeOptions.ઉમેરો(nodep->varp());
            }
        }
        iterateChildrenConst(nodep);
    }
    શૂન્ય visit(Ast_ગાંઠ* nodep) override { iterateChildrenConst(nodep); }

જાહેર:
    explicit CovergroupAssignValidVisitor(AstNetlist* nodep) {
        // Uses can precede their enclosing class in the tree.
        iterateConst(nodep);
        m_collecting = ખોટું;
        જો (!m_constructors.empty()) iterateConst(nodep);
    }
    તાર્કિક valid() અચલ { પાછા m_valid; }
    // type_option variables through which SystemVerilog may set type_option.merge_instances, so
    // that covergroups are tested with optionVar(true)
    અચલ std::set<અચલ AstVar*>& mergeableTypeOptions() અચલ { પાછા m_mergeableTypeOptions; }
};

//######################################################################
// Covergroup expression validation visitor

વર્ગ Covergroup_પદાવલિ_Valid_Visitor final : જાહેર VNVisitor {
    અચલ std::set<અચલ AstVar*>& m_sampleMembers;
    અચલ std::set<અચલ AstVar*>& m_constructorRefMembers;
    તાર્કિક m_inCoverageExpression = ખોટું;
    તાર્કિક m_sampleFormalAllowed = ખોટું;

    શૂન્ય scanSampleExpression(Ast_ગાંઠ* nodep) {
        જો (!nodep) પાછા;
        VL_RESTORER(m_sampleFormalAllowed);
        m_sampleFormalAllowed = સાચું;
        iterateAndNextNull(nodep);
    }

    શૂન્ય scanCoverageExpression(Ast_ગાંઠ* nodep) {
        જો (!nodep) પાછા;
        VL_RESTORER(m_inCoverageExpression);
        m_inCoverageExpression = સાચું;
        iterateAndNextNull(nodep);
    }

    શૂન્ય visit(AstCoverpoint* nodep) override {
        scanSampleExpression(nodep->exprp());
        scanSampleExpression(nodep->iffp());
        iterateAndNextNull(nodep->binsp());
        iterateAndNextNull(nodep->optionsp());
    }
    શૂન્ય visit(AstCoverCross* nodep) override {
        iterateAndNextNull(nodep->itemsp());
        scanSampleExpression(nodep->iffp());
        iterateAndNextNull(nodep->optionsp());
        iterateAndNextNull(nodep->binsp());
    }
    શૂન્ય visit(AstCoverCrossBin* nodep) override {
        iterateAndNextNull(nodep->selectp());
        scanSampleExpression(nodep->iffp());
    }
    શૂન્ય visit(AstCoverCrossWith* nodep) override {
        iterateAndNextNull(nodep->selectp());
        scanCoverageExpression(nodep->filterp());
        scanCoverageExpression(nodep->countp());
    }
    શૂન્ય visit(AstCoverBinsof* nodep) override { scanCoverageExpression(nodep->rangesp()); }
    શૂન્ય visit(AstCoverBin* nodep) override {
        scanCoverageExpression(nodep->rangesp());
        scanSampleExpression(nodep->iffp());
        scanCoverageExpression(nodep->arraySizep());
        scanCoverageExpression(nodep->transp());
    }
    શૂન્ય visit(AstVarRef* nodep) override {
        જો (!m_sampleFormalAllowed && m_sampleMembers.ગણતરી(nodep->varp())) {
            nodep->v3error("Covergroup sample formal argument "
                           << nodep->varp()->prettyNameQ()
                           << " may only be used in a coverpoint or conditional guard "
                              "expression (IEEE 1800-2023 19.8.1).");
        }
        જો (m_inCoverageExpression && m_constructorRefMembers.ગણતરી(nodep->varp())) {
            nodep->v3error("Ref covergroup constructor formal argument "
                           << nodep->varp()->prettyNameQ()
                           << " may not be used in a covergroup expression "
                              "(IEEE 1800-2023 19.5).");
        }
    }
    શૂન્ય visit(Ast_ગાંઠ_FTask_Ref* nodep) override {
        જો (m_inCoverageExpression && nodep->taskp()) {
            તાર્કિક invalidDirection = ખોટું;
            માટે (Ast_ગાંઠ* stmtp = nodep->taskp()->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
                અચલ AstVar* અચલ varp = VN_CAST(stmtp, Var);
                જો (varp && varp->isIO() && varp->isWritable()) {
                    invalidDirection = સાચું;
                    તોડો;
                }
            }
            જો (invalidDirection) {
                nodep->v3error("Function " << nodep->taskp()->prettyNameQ()
                                           << " called in a covergroup expression has an "
                                              "output, inout, or non-const ref argument "
                                              "(IEEE 1800-2023 19.5).");
            }
        }
        iterateChildren(nodep);
    }
    શૂન્ય visit(Ast_ગાંઠ* nodep) override { iterateChildren(nodep); }

જાહેર:
    Covergroup_પદાવલિ_Valid_Visitor(અચલ std::set<અચલ AstVar*>& sampleMembers,
                               અચલ std::set<અચલ AstVar*>& constructorRefMembers)
        : m_sampleMembers{sampleMembers}
        , m_constructorRefMembers{constructorRefMembers} {}
    શૂન્ય scan(Ast_ગાંઠ* nodep) { iterate(nodep); }
};

//######################################################################
// Bins of one declaration whose values are computed rather than listed: bin k covers
// [m_lo + k * m_stride, m_lo + (k + 1) * m_stride - 1], and the last bin extends to m_hi.  An
// array bin element is a run of single-value bins; automatic bins partition the coverpoint
// domain.  Bounds are coverpoint values at FunctionalCoverageVisitor::runWidth(),
// sign-extended like a CrossValueRange's.

વર્ગ BinRun final {
જાહેર:
    // MEMBERS
    uint32_t m_ગણતરી;  // Number of bins
    V3_સંખ્યા m_lo;  // Lowest value of the first bin
    V3_સંખ્યા m_stride;  // Number of values of each bin but the last
    V3_સંખ્યા m_hi;  // Highest value of the last bin
    તાર્કિક m_empty = ખોટું;  // A single bin without a value of the coverpoint type
    uint32_t m_declared = 0;  // Runtime index of the first bin, once generated

    // CONSTRUCTORS
    BinRun(Ast_ગાંઠ* nodep, પૂર્ણાંક width, uint32_t ગણતરી)
        : m_ગણતરી{ગણતરી}
        , m_lo{nodep, width}
        , m_stride{nodep, width, 1}
        , m_hi{nodep, width} {}
};

//######################################################################
// Functional coverage visitor

વર્ગ FunctionalCoverageVisitor final : જાહેર VNVisitor {
    // NODE STATE
    // Entire netlist:
    //  AstCoverpoint::user1p()  -> AstVar*.  Previous-value variable for transition bins
    //  AstCoverpoint::user2()   -> bool.  Had a bins declaration ignored, so no automatic bins
    અચલ VNUser1InUse m_inuser1;
    અચલ VNUser2InUse m_inuser2;

    // STATE
    std::set<AstCoverpoint*>
        m_runtimePoints;  // Points needing value metadata and live-bin mapping
    std::set<AstCoverCross*> m_runtimeCrosses;  // Crosses over finalized live-bin dimensions
    std::map<AstVar*, AstVar*> m_excludedVars;  // Sample-time state-exclusion flags
    AstClass* m_covergroupp = nullptr;  // Current covergroup being processed
    Ast_ગાંઠ_Module* m_unitp = nullptr;  // Design unit declaring the current class, outside classes
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
    તાર્કિક m_cgMergeInstances = ખોટું;  // The covergroup's type_option.merge_instances, a constant
    // The covergroup's type_option.merge_instances is, or SystemVerilog may make it, true
    તાર્કિક m_cgMayMerge = ખોટું;
    // type_option members through which SystemVerilog may set type_option.merge_instances,
    // see CovergroupAssignValidVisitor::mergeableTypeOptions()
    અચલ std::set<અચલ AstVar*>& m_mergeableTypeOptions;
    પૂર્ણાંક m_cgAtLeast = 1;  // Covergroup option.at_least

    સંરચના EmbeddedEventTrigger final {
        File_લીટી* eventFl;  // Clocking-event source location
        AstVar* baseVarp;  // Base enclosing-class member in the event expression
        AstVar* memberVarp;  // Selected member in a 'base.member' expression, or nullptr
        VEdgeType edgeType;  // Clocking-event edge qualifier
        AstVar* prevVarp;  // Member containing the previous event value, or nullptr
        EmbeddedEventTrigger(File_લીટી* eventFl, AstVar* baseVarp, AstVar* memberVarp,
                             VEdgeType edgeType)
            : eventFl{eventFl}
            , baseVarp{baseVarp}
            , memberVarp{memberVarp}
            , edgeType{edgeType}
            , prevVarp{nullptr} {}
    };

    std::set<std::string> m_crossedCpNames;  // Coverpoints referenced by a cross
    std::map<std::string, AstVar*> m_cpVarMap;  // Coverpoint name -> its VlCoverpoint member
    સંરચના BinRuns final {
        std::vector<BinRun> runs;  // Runs of an array or automatic bins declaration, in order
        uint32_t ગણતરી = 0;  // Bins across all runs
        તાર્કિક unsupported = ખોટું;  // Too many bins, or invalid: the declaration is ignored
        // The value of each bin, which names it, of a wildcard array; else the bins are indexed
        std::vector<std::string> કિંમતો;
        // Too many values of an ignore or illegal wildcard array, which is then one bin
        તાર્કિક single = ખોટું;
    };
    સંરચના Cross_Bin_કિંમતો final {
        AstCoverBin* binp;  // Declaration owning this Normal bin
        Ast_ગાંઠ_પદાવલિ* valuep;  // Individual array-bin value, or nullptr for a scalar bin
        અચલ BinRun* runp = nullptr;  // Run computing the bin's values, if any
        uint32_t element = 0;  // Index of the bin within runp
    };
    સંરચના BinSpan final {
        uint32_t first = 0;  // First Normal index of the bin declaration
        uint32_t ગણતરી = 0;  // Number of Normal bins of the declaration
        uint32_t declared = 0;  // First runtime bin index, across all bin kinds
        int32_t sized = -1;  // Index of the sized array, placed at construction; or -1
    };
    સંરચના CoverpointBins final {
        uint32_t total = 0;  // Number of Normal bins
        Ast_ગાંઠ_પદાવલિ* exprp = nullptr;  // Sampled expression, for the value domain
        તાર્કિક crossed = ખોટું;  // Feeds a cross, which needs 'values'
        std::vector<Cross_Bin_કિંમતો> કિંમતો;  // Values in runtime Normal-bin index order
        std::unordered_map<std::string, BinSpan> spans;  // Declared bin name -> index span
        BinSpan implicitAuto{0, 0, 0};  // Implicit automatic bins, each named 'auto_<i>'
        std::deque<BinRun> runs;  // Runs 'values' refers to
    };
    std::map<AstVar*, CoverpointBins> m_cpBins;  // Runtime coverpoint -> binsof index ranges
    // Names of the bins declarations each coverpoint ignored, which binsof selects as no bins
    std::map<અચલ AstCoverpoint*, std::vector<std::string>> m_droppedBins;
    // Prefixes of the constructor temporaries of constructed bins, as coverpoint and bin names
    // alone may repeat: coverpoint 'a_' bins 'b', and coverpoint 'a' bins '_b'
    V3UniqueNames m_sizedNames{"__Vsized"};
    std::vector<Ast_ગાંઠ_પદાવલિ*> m_detached_કિંમતો;  // Array-bin values m_cpBins refers to
    std::set<AstCoverCross*>
        m_droppedCrosses;  // Crosses with a bare-variable item: drop (COVERIGN)
    std::map<uint32_t, AstCoverpointDType*> m_cpDTypes;  // Hit-list bound -> interned dtype
    using CrossShape = std::tuple<uint32_t, uint32_t, uint32_t, uint32_t, uint64_t, તાર્કિક>;
    std::map<CrossShape, AstCoverCrossDType*> m_cxDTypes;
    AstVar* m_cgInstVarp = nullptr;  // __Vcg_inst handle member of the current covergroup

    VMemberMap m_memberMap;  // Member names cached for fast lookup

    // METHODS
    // The covergroup's 'option' or static 'type_option' member (V3LinkParse creates both)
    AstVar* optionVar(તાર્કિક typeOption) {
        AstVar* અચલ varp = VN_AS(
            m_memberMap.findMember(m_covergroupp, typeOption ? "type_option" : "option"), Var);
        UASSERT_OBJ(varp, m_covergroupp, "Covergroup missing option member");
        પાછા varp;
    }

    // 'option.<member>' or 'type_option.<member>', per optionVarp
    AstStructSel* newOptionSel(File_લીટી* fl, AstVar* optionVarp, અચલ std::string& member,
                               VAccess access) {
        અચલ AstMemberDType* અચલ memberp
            = VN_AS(m_memberMap.findMember(optionVarp->dtypep()->skipRefp(), member), MemberDType);
        UASSERT_OBJ(memberp, optionVarp, "Coverage option structure missing '" << member << "'");
        Ast_ગાંઠ_પદાવલિ* અચલ fromp = optionVarp->lifetime().isStatic()
                                       ? નવું AstVarRef{fl, optionVarp, access}
                                       : memberRef(fl, optionVarp, access);
        AstStructSel* અચલ selp = નવું AstStructSel{fl, fromp, member};
        selp->dtypep(memberp->subDTypep()->skipRefToEnump());
        selp->didWidth(સાચું);
        પાછા selp;
    }

    // A constant option.at_least; a negative count of hits has no meaning, so takes the default
    સ્થિર પૂર્ણાંક constAtLeast(અચલ AstConst* constp) {
        અચલ પૂર્ણાંક કિંમત = constp->toSInt();
        જો (કિંમત >= 0) પાછા કિંમત;
        constp->v3warn(COVERIGN, "Ignoring negative 'option.at_least'; using 1");
        પાછા 1;
    }

    // An option's constant value, or nullptr after a warning, as non-constant is unsupported
    સ્થિર અચલ AstConst* constOption(અચલ Ast_ગાંઠ* valuep, VCoverOptionType optType) {
        જો (અચલ AstConst* અચલ constp = VN_CAST(valuep, Const)) પાછા constp;
        valuep->v3warn(COVERIGN, "Ignoring unsupported: non-constant 'option."
                                     << optType.ascii() << "'; using default value");
        પાછા nullptr;
    }

    // Store the covergroup-level options (IEEE 1800-2023 19.7) where SystemVerilog and the
    // runtime read them.  The instance options, option.weight and option.get_inst_coverage,
    // are evaluated by the constructor, as are the other instance options; the type options,
    // type_option.weight and type_option.merge_instances, are constant, and initialize the
    // static member.  Without its own type_option.merge_instances, a covergroup has the
    // default of --coverage-merge-instances.  A constant option.at_least is the default of
    // the coverpoints and crosses.
    શૂન્ય lowerCovergroupOptions() {
        તાર્કિક mergeSet = ખોટું;  // The covergroup sets type_option.merge_instances
        m_cgAtLeast = 1;
        માટે (AstCgOptionAssign* અચલ optp : m_cgOptions) {
            જો (optp->optType() == VCoverOptionType::AT_LEAST) {
                જો (અચલ AstConst* અચલ constp = constOption(optp->valuep(), optp->optType())) {
                    m_cgAtLeast = constAtLeast(constp);
                }
                VL_DO_DANGLING(ધકેલો_Deletep(optp->unlinkFrBack()), optp);
                ચાલુ;
            }
            File_લીટી* અચલ fl = optp->fileline();
            // V3Width left the type options constant, and type_option.weight non-negative
            અચલ AstConst* અચલ constp = VN_CAST(optp->valuep(), Const);
            std::string member = "weight";
            જો (optp->optType() == VCoverOptionType::MERGE_INSTANCES) {
                member = "merge_instances";
                m_cgMergeInstances = !constp->અંક().isEqZero();
                mergeSet = સાચું;
            } નહીંતર જો (optp->optType() == VCoverOptionType::GET_INST_COVERAGE) {
                member = "get_inst_coverage";
            } નહીંતર {
                UASSERT_OBJ(optp->optType() == VCoverOptionType::WEIGHT, optp,
                            "Unexpected covergroup option reaching V3Covergroup");
                જો (optp->typeOption()) m_cgTypeWeight = constp->toUInt();
            }
            AstAssign* અચલ assignp = નવું AstAssign{
                fl, newOptionSel(fl, optionVar(optp->typeOption()), member, VAccess::WRITE),
                optp->valuep()->unlinkFrBack()};
            જો (optp->typeOption()) {
                m_covergroupp->addMembersp(નવું AstInitialStatic{fl, assignp});
                VL_DO_DANGLING(ધકેલો_Deletep(optp->unlinkFrBack()), optp);
            } નહીંતર {
                optp->replaceWith(assignp);
                VL_DO_DANGLING(ધકેલો_Deletep(optp), optp);
            }
        }
        m_cgOptions.clear();
        જો (!mergeSet && v3Global.opt.coverageMergeInstances()) {
            File_લીટી* અચલ fl = m_covergroupp->fileline();
            m_covergroupp->addMembersp(નવું AstInitialStatic{
                fl, નવું AstAssign{
                        fl, newOptionSel(fl, optionVar(સાચું), "merge_instances", VAccess::WRITE),
                        નવું AstConst{fl, AstConst::BitTrue{}}}});
            m_cgMergeInstances = સાચું;
        }
        m_cgMayMerge = m_cgMergeInstances || m_mergeableTypeOptions.ગણતરી(optionVar(સાચું));
    }

    // The weight of an item in the coverage database, which merges the instances: its
    // type_option.weight if the covergroup merges them too (IEEE 1800-2023 19.11.3); else its
    // option.weight if a constant, and so of every instance; else its type_option.weight
    uint32_t itemDatabaseWeight(Ast_ગાંઠ* optionsp) અચલ {
        અચલ Ast_ગાંઠ_પદાવલિ* weightp = nullptr;  // The option.weight in effect
        uint32_t typeWeight = 1;
        માટે (Ast_ગાંઠ* nodep = optionsp; nodep; nodep = nodep->nextp()) {
            અચલ AstCoverOption* અચલ optp = VN_AS(nodep, CoverOption);
            જો (!(optp->optType() == VCoverOptionType::WEIGHT)) ચાલુ;
            // V3Width left type_option.weight a non-negative constant
            જો (optp->typeOption()) {
                typeWeight = VN_AS(optp->valuep(), Const)->toUInt();
            } નહીંતર {
                weightp = optp->valuep();
            }
        }
        જો (m_cgMergeInstances) પાછા typeWeight;
        જો (!weightp) પાછા 1;
        જો (અચલ AstConst* અચલ constp = VN_CAST(weightp, Const)) પાછા constp->toUInt();
        પાછા typeWeight;
    }

    // Configure an item's option.weight, its weight in instance coverage (IEEE 1800-2023
    // 19.11), and, if the covergroup may merge its instances, its type_option.weight, its
    // weight in type coverage then (19.11.3).
    શૂન્ય generateItemWeight(File_લીટી* fl, AstVar* itemVarp, Ast_ગાંઠ* optionsp) {
        માટે (Ast_ગાંઠ* nodep = optionsp; nodep; nodep = nodep->nextp()) {
            અચલ AstCoverOption* અચલ optp = VN_AS(nodep, CoverOption);
            જો (!(optp->optType() == VCoverOptionType::WEIGHT)) ચાલુ;
            જો (!optp->typeOption()) {
                m_constructorp->addStmtsp(
                    itemCall(fl, itemVarp, VCMethod::COVERGROUP_WEIGHT,
                             {optp->valuep()->clone_વૃક્ષ(ખોટું), file_લીટી_Debug(optp->fileline())})
                        ->makeStmt());
            } નહીંતર જો (m_cgMayMerge) {
                // V3Width left type_option.weight a non-negative constant
                m_constructorp->addStmtsp(itemCall(fl, itemVarp, VCMethod::COVERGROUP_TYPE_WEIGHT,
                                                   {optp->valuep()->clone_વૃક્ષ(ખોટું)})
                                              ->makeStmt());
            }
        }
    }

    શૂન્ય processCovergroup() {
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
        m_cgMergeInstances = ખોટું;
        m_cgMayMerge = ખોટું;

        lowerCovergroupOptions();

        // Scan every cross item to record the coverpoints it references (the cross dimensions)
        // and to flag any cross naming a bare variable -- a would-be implicit coverpoint, which
        // Verilator does not synthesize.  An unresolvable item drops only that one cross (with a
        // COVERIGN in generateCrossCode), leaving the rest of the covergroup intact.
        માટે (AstCoverCross* crossp : m_coverCrosses) {
            માટે (Ast_ગાંઠ* itemp = crossp->itemsp(); itemp; itemp = itemp->nextp()) {
                અચલ AstCoverpointRef* અચલ refp = VN_AS(itemp, CoverpointRef);
                જો (refp->exprp()) ચાલુ;  // hierarchical ref: dropped in generateCrossCode
                જો (m_coverpointMap.find(refp->name()) == m_coverpointMap.end()) {
                    m_droppedCrosses.ઉમેરો(crossp);  // bare variable: drop this cross only
                } નહીંતર {
                    m_crossedCpNames.ઉમેરો(refp->name());
                }
            }
        }

        std::vector<Ast_ગાંઠ*> pending;
        std::map<AstCoverpoint*, std::vector<AstCoverCross*>> consumers;
        std::map<AstCoverCross*, std::vector<AstCoverpoint*>> inputs;
        માટે (AstCoverpoint* અચલ cpp : m_coverpoints) {
            checkBinNames(cpp);
            checkConstructedBins(cpp);
            જો (!cpp->exprp()->dtypep()->skipRefp()->isIntegralOrPacked()) ચાલુ;
            // Bins without values leave the report (IEEE 1800-2023 19.11.1), exclusions or not.
            // Constructed bins get their values when the covergroup is constructed.
            જો (!coverpointHasStateExclusions(cpp) && !coverpointHasEmptyBins(cpp)
                && !coverpointHasConstructedBins(cpp)) {
                ચાલુ;
            }
            m_runtimePoints.ઉમેરો(cpp);
            pending.ધકેલો_back(cpp);
        }
        માટે (AstCoverCross* અચલ crossp : m_coverCrosses) {
            જો (m_droppedCrosses.ગણતરી(crossp)) ચાલુ;
            માટે (Ast_ગાંઠ* itemp = crossp->itemsp(); itemp; itemp = itemp->nextp()) {
                અચલ AstCoverpointRef* અચલ refp = VN_AS(itemp, CoverpointRef);
                જો (refp->exprp()) ચાલુ;
                અચલ auto point = m_coverpointMap.find(refp->name());
                જો (point != m_coverpointMap.end()) {
                    consumers[point->second].ધકેલો_back(crossp);
                    inputs[crossp].ધકેલો_back(point->second);
                }
            }
            // The values of a filter's bin tuples are known when the covergroup is constructed
            જો (crossp->exists([](અચલ AstCoverCrossWith*) { પાછા સાચું; })) {
                m_runtimeCrosses.emplace(crossp);
                pending.ધકેલો_back(crossp);
            }
        }
        માટે (size_t next = 0; next < pending.size(); ++next) {
            જો (AstCoverpoint* અચલ pointp = VN_CAST(pending[next], Coverpoint)) {
                માટે (AstCoverCross* અચલ crossp : consumers[pointp]) {
                    જો (m_runtimeCrosses.emplace(crossp).second) pending.ધકેલો_back(crossp);
                }
            } નહીંતર {
                માટે (AstCoverpoint* અચલ pointp : inputs[VN_AS(pending[next], CoverCross)]) {
                    જો (pointp->exprp()->dtypep()->skipRefp()->isIntegralOrPacked()
                        && m_runtimePoints.emplace(pointp).second) {
                        pending.ધકેલો_back(pointp);
                    }
                }
            }
        }

        // The instance node owns this instance's coverpoint/cross runtimes, so it must exist
        // before any of them is created.  Emitted first, ahead of both generate loops.
        generateInstanceAttach();

        // For each coverpoint, generate sampling code
        માટે (AstCoverpoint* cpp : m_coverpoints) generateCoverpointCode(cpp);

        // For each cross, generate sampling code
        માટે (AstCoverCross* crossp : m_coverCrosses) generateCrossCode(crossp);

        // Every cross has been built, so runtime points only need their exclusions from here.
        માટે (AstCoverpoint* અચલ cpp : m_coverpoints) {
            જો (!m_runtimePoints.ગણતરી(cpp)) ચાલુ;
            m_constructorp->addStmtsp(itemCall(cpp->fileline(), m_cpVarMap.at(cpp->name()),
                                               VCMethod::COVERGROUP_કિંમત_RELEASE)
                                          ->makeStmt());
        }
        માટે (Ast_ગાંઠ_પદાવલિ* valuep : m_detached_કિંમતો) VL_DO_DANGLING(ધકેલો_Deletep(valuep), valuep);
        m_detached_કિંમતો.clear();

        // Generate coverage computation code (even for empty covergroups).  Bin registration
        // with the coverage database is handled per coverpoint/cross by their runtime
        // registerBins() calls (emitted in generateCoverpoint/generateCross).
        generateCoverageComputationCode();
    }

    સ્થિર constexpr size_t કિંમત_LIST_ENTRIES = 256;  // Metadata entries per constructor call

    // The value of a constant count, of bins or value tuples: -1 if it is negative, and
    // saturated above the largest limit
    સ્થિર int64_t const_ગણતરી(અચલ AstConst* constp) {
        અચલ V3_સંખ્યા& અંક = constp->અંક();
        જો (constp->isSigned() && અંક.isNegative()) પાછા -1;
        પાછા અંક.mostSetBitP1() > 32 ? INT64_MAX : static_cast<int64_t>(અંક.toUQuad());
    }

    // Whether a count, 'whatp', is integral and, if constant, positive, as the standard's clause
    // 'clausep' requires; else errors
    સ્થિર તાર્કિક check_ગણતરી(અચલ Ast_ગાંઠ_પદાવલિ* countp, અચલ અક્ષર* whatp, અચલ અક્ષર* clausep) {
        અચલ AstConst* અચલ constp = VN_CAST(countp, Const);
        જો (!countp->dtypep()->skipRefp()->isIntegralOrPacked()) {
            countp->v3error(whatp << " must be integral (IEEE 1800-2023 " << clausep << ")");
            પાછા ખોટું;
        } નહીંતર જો (constp && (constp->અંક().isFourState() || const_ગણતરી(constp) < 1)) {
            countp->v3error(whatp << " must be >= 1, got "
                                  << (constp->અંક().isFourState() ? constp->અંક().ascii(ખોટું)
                                                                  : constp->અંક().toDecimalS())
                                  << " (IEEE 1800-2023 " << clausep << ")");
            પાછા ખોટું;
        }
        પાછા સાચું;
    }

    // The number of bins requested by a valid 'bins auto[N]', or 0
    સ્થિર uint32_t autoBinsRequested(અચલ AstCoverBin* binp) {
        અચલ AstConst* અચલ constp = VN_CAST(binp->arraySizep(), Const);
        જો (!constp) પાછા 0;
        અચલ int64_t ગણતરી = const_ગણતરી(constp);
        પાછા ગણતરી < 1 || ગણતરી > v3Global.opt.coverageMaxBins() ? 0
                                                                   : static_cast<uint32_t>(ગણતરી);
    }

    // True for a 'bins auto[N]' declaration, or the implicit automatic bins of a coverpoint
    સ્થિર તાર્કિક isAutoBins(અચલ AstCoverBin* binp) {
        પાછા binp->binsType() == VCoverBinsType::BINS_AUTO
               || binp->binsType() == VCoverBinsType::BINS_AUTO_IMPLICIT;
    }

    // True for a sized array of bins, 'bins b[N] = {...}', whose values an integral coverpoint
    // distributes over N bins when the covergroup is constructed (IEEE 1800-2023 19.5.1)
    સ્થિર તાર્કિક isSizedArray(અચલ AstCoverBin* binp) {
        પાછા binp->arraySizep() && !isAutoBins(binp);
    }

    // The 'with' filter of a bin, if any (IEEE 1800-2023 19.5.1.1)
    સ્થિર AstCoverWith* binWith(અચલ AstCoverBin* binp) {
        પાછા VN_CAST(binp->rangesp(), CoverWith);
    }

    // True for bins whose values the covergroup constructor computes: a sized array, or bins of
    // the values a 'with' filter keeps
    સ્થિર તાર્કિક isConstructedBins(અચલ AstCoverBin* binp) {
        પાછા isSizedArray(binp) || binWith(binp);
    }

    // The range list of a bin's values, or of the candidates of its 'with' filter; null for all
    // of the coverpoint's values, which a filter of the coverpoint's name has
    સ્થિર Ast_ગાંઠ* binRangesp(અચલ AstCoverBin* binp) {
        અચલ AstCoverWith* અચલ withp = binWith(binp);
        જો (!withp) પાછા binp->rangesp();
        પાછા VN_IS(withp->subp(), CoverpointRef) ? nullptr : withp->subp();
    }

    // Report and delete a bins declaration of a name that another of its coverpoint has
    શૂન્ય checkBinNames(AstCoverpoint* coverpointp) {
        std::set<std::string> names;
        માટે (Ast_ગાંઠ* nodep = coverpointp->binsp(); nodep;) {
            AstCoverBin* અચલ binp = VN_AS(nodep, CoverBin);
            nodep = nodep->nextp();
            જો (names.emplace(binp->name()).second) ચાલુ;
            binp->v3error("Duplicate bin " << binp->prettyNameQ() << " in coverpoint "
                                           << coverpointp->prettyNameQ()
                                           << " (IEEE 1800-2023 3.13)");
            VL_DO_DANGLING(ધકેલો_Deletep(binp->unlinkFrBack()), binp);
        }
    }

    // Delete an ignored bins declaration, which binsof then selects as no bins
    શૂન્ય dropBins(અચલ AstCoverpoint* coverpointp, AstCoverBin* binp) {
        m_droppedBins[coverpointp].ધકેલો_back(binp->name());
        VL_DO_DANGLING(ધકેલો_Deletep(binp->unlinkFrBack()), binp);
    }

    // Check the size of a sized array of bins, which drops an invalid array.  A real coverpoint's
    // are unsupported, and treated as arrays of a bin per value.  False unless it stays sized.
    તાર્કિક checkBinsArraySize(અચલ AstCoverpoint* coverpointp, AstCoverBin* binp, તાર્કિક integral) {
        Ast_ગાંઠ_પદાવલિ* અચલ sizep = binp->arraySizep();
        જો (VN_IS(sizep, Unbounded)) {  // A parameter of '$'; see bins_orBraE
            binp->v3error("Bins array size must be integral, not '$' (IEEE 1800-2023 19.5.1)");
        } નહીંતર જો (check_ગણતરી(sizep, "Bins array size", "19.5.1")) {
            જો (integral) પાછા સાચું;
            binp->v3warn(COVERIGN, "Unsupported: 'bins' explicit array size of a real "
                                   "coverpoint (treated as '[]')");
            VL_DO_DANGLING(ધકેલો_Deletep(sizep->unlinkFrBack()), sizep);
            પાછા ખોટું;
        }
        dropBins(coverpointp, binp);
        પાછા ખોટું;
    }

    // Check the bins of a coverpoint whose values the constructor computes, dropping invalid
    // ones.  A wildcard array of too many ranges of values is ignored, or if ignore or illegal,
    // treated as one bin; filtering too many ranges of values ignores the bins.
    શૂન્ય checkConstructedBins(AstCoverpoint* coverpointp) {
        અચલ તાર્કિક integral = coverpointp->exprp()->dtypep()->skipRefp()->isIntegralOrPacked();
        માટે (Ast_ગાંઠ* nodep = coverpointp->binsp(); nodep;) {
            AstCoverBin* અચલ binp = VN_AS(nodep, CoverBin);
            nodep = nodep->nextp();
            જો (!isConstructedBins(binp)) ચાલુ;
            જો (isSizedArray(binp) && !checkBinsArraySize(coverpointp, binp, integral)) ચાલુ;
            જો (!binp->isWildcard()
                || sizedWildcardRuns(binp, coverpointp->exprp())
                       <= v3Global.opt.coverageMaxBins()) {
                જો (binWith(binp) && withCandidatesOver(binp, coverpointp->exprp())) {
                    binp->v3warn(COVERIGN, "Unsupported: 'with' filter of more than 2**32 "
                                           "candidate values; bin "
                                               << binp->prettyNameQ() << " ignored");
                    જો (binp->binsType().binIsNormal()) coverpointp->user2(સાચું);
                    dropBins(coverpointp, binp);
                }
                ચાલુ;
            }
            // An ignore or illegal array still excludes or checks its values, as one bin
            અચલ તાર્કિક single = !binp->binsType().binIsNormal() && !binWith(binp);
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
            જો (single) {
                Ast_ગાંઠ_પદાવલિ* અચલ sizep = binp->arraySizep();
                VL_DO_DANGLING(ધકેલો_Deletep(sizep->unlinkFrBack()), sizep);
                binp->isArray(ખોટું);
                ચાલુ;
            }
            જો (binp->binsType().binIsNormal()) coverpointp->user2(સાચું);
            dropBins(coverpointp, binp);
        }
    }

    // True if a 'with' filter would be evaluated for more than 2**32 candidate values, known
    // now for the coverpoint's name, or for a range list of constants: each is evaluated once
    // but for an array 'b[N]', which keeps their order and duplicates (see withBegin())
    સ્થિર તાર્કિક withCandidatesOver(AstCoverBin* binp, Ast_ગાંઠ_પદાવલિ* exprp) {
        અચલ પૂર્ણાંક width = runWidth(exprp);
        std::vector<std::pair<V3_સંખ્યા, V3_સંખ્યા>> runs;
        જો (!binRangesp(binp)) runs = coverpoint_કિંમતો(binp, exprp);
        અચલ auto constant
            = [](અચલ Ast_ગાંઠ* nodep) { પાછા VN_IS(nodep, Const) || VN_IS(nodep, Unbounded); };
        માટે (Ast_ગાંઠ* rangep = binRangesp(binp); rangep; rangep = rangep->nextp()) {
            અચલ AstInsideRange* અચલ irp = VN_CAST(rangep, InsideRange);
            // Else known when constructed
            જો (irp ? !constant(irp->lhsp()) || !constant(irp->rhsp()) : !VN_IS(rangep, Const)) {
                પાછા ખોટું;
            }
            Cross_કિંમત_Range range{rangep, resolveWidth(rangep, exprp)};
            જો (!resolve_કિંમત(rangep, exprp, સાચું, binp->isWildcard(), range)
                || crossRangeEmpty(range)) {
                ચાલુ;
            }
            std::vector<std::pair<V3_સંખ્યા, V3_સંખ્યા>> found{{range.lo, range.hi}};
            જો (range.wildcard) {  // checkConstructedBins bounded the runs
                found.clear();
                crossRangeRuns(range, v3Global.opt.coverageMaxBins(), found);
            }
            માટે (અચલ std::pair<V3_સંખ્યા, V3_સંખ્યા>& run : found) {
                runs.emplace_back(V3_સંખ્યા{rangep, width, run.first},
                                  V3_સંખ્યા{rangep, width, run.second});
            }
        }
        જો (!isSizedArray(binp)) {  // The union of the values
            std::sort(runs.begin(), runs.end(), [](અચલ auto& lhs, અચલ auto& rhs) {
                પાછા cross_કિંમત_Less(lhs.first, rhs.first);
            });
            std::vector<std::pair<V3_સંખ્યા, V3_સંખ્યા>> merged;
            માટે (અચલ std::pair<V3_સંખ્યા, V3_સંખ્યા>& run : runs) {
                જો (merged.empty() || cross_કિંમત_Less(merged.back().second, run.first)) {
                    merged.ધકેલો_back(run);
                } નહીંતર જો (cross_કિંમત_Less(merged.back().second, run.second)) {
                    merged.back().second = run.second;
                }
            }
            runs = std::move(merged);
        }
        uint64_t ગણતરી = 0;
        માટે (અચલ std::pair<V3_સંખ્યા, V3_સંખ્યા>& run : runs) {
            V3_સંખ્યા span{exprp, width};
            span.opSub(run.second, run.first);
            જો (span.mostSetBitP1() > 32) પાછા સાચું;  // More than 2**32 values
            ગણતરી += span.toUQuad() + 1;
            જો (ગણતરી > (uint64_t{1} << 32)) પાછા સાચું;
        }
        પાછા ખોટું;
    }

    // The ranges of values the wildcard patterns of a sized wildcard array, or of a 'with'
    // filter's candidates, give, counted up to more than --coverage-max-bins
    સ્થિર size_t sizedWildcardRuns(અચલ AstCoverBin* binp, Ast_ગાંઠ_પદાવલિ* exprp) {
        std::vector<std::pair<V3_સંખ્યા, V3_સંખ્યા>> runs;
        માટે (Ast_ગાંઠ* rangep = binRangesp(binp); rangep; rangep = rangep->nextp()) {
            જો (!VN_IS(rangep, Const)) ચાલુ;  // A range, or a value known at construction
            Cross_કિંમત_Range range{rangep, resolveWidth(rangep, exprp)};
            જો (resolve_કિંમત(rangep, exprp, સાચું, સાચું, range) && !crossRangeEmpty(range)) {
                crossRangeRuns(range, v3Global.opt.coverageMaxBins(), runs);
            }
        }
        પાછા runs.size();
    }

    // Check the automatic bins declarations of a coverpoint.  Each stays one declaration, which
    // generates as a partition of the coverpoint domain (see autoBinRuns).
    શૂન્ય checkAutomaticBins(AstCoverpoint* coverpointp, અચલ Ast_ગાંઠ_પદાવલિ* exprp) {
        માટે (Ast_ગાંઠ* binp = coverpointp->binsp(); binp; binp = binp->nextp()) {
            AstCoverBin* અચલ cbinp = VN_AS(binp, CoverBin);
            જો (cbinp->binsType() != VCoverBinsType::BINS_AUTO) ચાલુ;
            અચલ AstConst* અચલ constp = VN_CAST(cbinp->arraySizep(), Const);
            જો (!constp) {
                cbinp->v3error("Automatic bins array size must be a constant");
            } નહીંતર જો (const_ગણતરી(constp) < 1) {
                cbinp->v3error("Automatic bins array size must be >= 1, got "
                               << constp->અંક().toDecimalS());
            } નહીંતર જો (const_ગણતરી(constp) > v3Global.opt.coverageMaxBins()) {
                cbinp->v3error("Automatic bins array size of "
                               << constp->અંક().toDecimalU() << " exceeds limit of "
                               << v3Global.opt.coverageMaxBins() << '\n'
                               << cbinp->warnMore() << "... Suggest a larger --coverage-max-bins");
            } નહીંતર જો (!exprp->dtypep()->skipRefp()->isIntegralOrPacked()) {
                cbinp->v3error("Automatic bins are not allowed on a coverpoint of a non-integral "
                               "expression (IEEE 1800-2023 19.5.3).");
            }
        }
    }

    // Extract all option values of a coverpoint or cross in a single pass.
    // atLeastOut: option.at_least (default the covergroup's)
    // autoBinMaxOut: option.auto_bin_max (coverpoint overrides covergroup, default 64)
    શૂન્ય extractItemOptions(Ast_ગાંઠ* optionsp, પૂર્ણાંક& atLeastOut, પૂર્ણાંક& autoBinMaxOut) {
        atLeastOut = m_cgAtLeast;
        autoBinMaxOut = -1;  // -1 = not set at coverpoint level
        માટે (Ast_ગાંઠ* optionp = optionsp; optionp; optionp = optionp->nextp()) {
            AstCoverOption* અચલ optp = VN_AS(optionp, CoverOption);
            // Weights may be non-constant; generateItemWeight() handles them.  A cross keeps
            // its unsupported options, which V3LinkParse warned of.
            જો (!(optp->optType() == VCoverOptionType::AT_LEAST
                  || optp->optType() == VCoverOptionType::AUTO_BIN_MAX)) {
                ચાલુ;
            }
            અચલ AstConst* અચલ constp = constOption(optp->valuep(), optp->optType());
            જો (!constp) ચાલુ;
            જો (optp->optType() == VCoverOptionType::AT_LEAST) {
                atLeastOut = constAtLeast(constp);
            } નહીંતર {
                autoBinMaxOut = constp->toSInt();
            }
        }
        // Fall back to covergroup-level auto_bin_max if not set at coverpoint level
        જો (autoBinMaxOut < 0) {
            જો (m_covergroupp->cgAutoBinMax() >= 0) {
                autoBinMaxOut = m_covergroupp->cgAutoBinMax();
            } નહીંતર {
                autoBinMaxOut = 64;  // Default per IEEE 1800-2023 Table 19-1
            }
        }
    }

    // IEEE 1800-2023 19.5.2: an enum coverpoint has one automatic bin per enumeration value
    શૂન્ય createEnumAutoBins(AstCoverpoint* coverpointp, Ast_ગાંઠ_પદાવલિ* exprp,
                            અચલ AstEnumDType* enump) {
        File_લીટી* અચલ fl = coverpointp->fileline();
        માટે (અચલ AstEnumItem* itemp = enump->itemsp(); itemp;
             itemp = VN_AS(itemp->nextp(), EnumItem)) {
            AstConst* અચલ lop = new_કિંમત_Const(fl, VN_AS(itemp->valuep(), Const)->અંક(), exprp);
            AstInsideRange* અચલ rangep = નવું AstInsideRange{fl, lop, lop->clone_વૃક્ષ(ખોટું)};
            rangep->dtypeFrom(exprp);
            coverpointp->addBinsp(
                નવું AstCoverBin{fl, "auto[" + itemp->name() + "]", rangep, ખોટું, ખોટું});
        }
    }

    // IEEE 1800-2023 19.5.3/19.11.1: partition first, then apply exclusions.  The partition is one
    // automatic bins declaration, generated as a run like 'bins auto[N]' but numbering its bins.
    શૂન્ય createImplicitAutoBins(AstCoverpoint* coverpointp, Ast_ગાંઠ_પદાવલિ* exprp, પૂર્ણાંક autoBinMax) {
        જો (coverpointp->user2()) પાછા;  // Declared bins, ignored, leave no bins
        માટે (Ast_ગાંઠ* nodep = coverpointp->binsp(); nodep; nodep = nodep->nextp()) {
            અચલ VCoverBinsType kind = VN_AS(nodep, CoverBin)->binsType();
            જો (kind != VCoverBinsType::BINS_IGNORE && kind != VCoverBinsType::BINS_ILLEGAL)
                પાછા;
        }
        જો (અચલ AstEnumDType* અચલ enump
            = VN_CAST(exprp->dtypep()->skipRefToEnump(), EnumDType)) {
            createEnumAutoBins(coverpointp, exprp, enump);
            પાછા;
        }
        અચલ પૂર્ણાંક width = exprp->width();
        uint32_t ગણતરી = width < 31 ? std::min<uint32_t>(uint32_t{1} << width, autoBinMax)
                                    : static_cast<uint32_t>(autoBinMax);
        જો (!ગણતરી) પાછા;
        જો (!exprp->dtypep()->skipRefp()->isIntegralOrPacked()) {
            coverpointp->v3error("Coverpoint of a non-integral expression requires explicit bins "
                                 "(IEEE 1800-2023 19.5.3).");
            પાછા;
        }
        જો (ગણતરી > v3Global.opt.coverageMaxBins()) {
            coverpointp->v3warn(COVERIGN, "Unsupported: more than "
                                              << v3Global.opt.coverageMaxBins()
                                              << " automatic bins from 'option.auto_bin_max'; "
                                                 "using "
                                              << v3Global.opt.coverageMaxBins() << ".\n"
                                              << coverpointp->warnMore()
                                              << "... Suggest a larger --coverage-max-bins");
            ગણતરી = v3Global.opt.coverageMaxBins();
        }
        File_લીટી* અચલ fl = coverpointp->fileline();
        coverpointp->addBinsp(નવું AstCoverBin{fl, "auto", નવું AstConst{fl, ગણતરી},
                                              VCoverBinsType::BINS_AUTO_IMPLICIT});
    }

    // Sanitize generated names to be valid C++ identifiers
    સ્થિર string sanitizeGeneratedName(string name) {
        std::replace(name.begin(), name.end(), '[', '_');
        std::replace(name.begin(), name.end(), ']', '_');
        પાછા name;
    }

    // Capture an iff guard in a function-local temporary so it is evaluated once per sample()
    AstVarRef* captureIffToTemp(Ast_ગાંઠ_પદાવલિ* iffp, અચલ string& tempName) {
        File_લીટી* અચલ fl = iffp->fileline();
        AstVar* અચલ iffVarp
            = નવું AstVar{fl, VVarType::BLOCKTEMP, tempName, iffp->findBitDType()};
        iffVarp->funcLocal(સાચું);
        m_sampleFuncp->addStmtsp(iffVarp);
        iffp->unlinkFrBack();
        m_sampleFuncp->addStmtsp(
            નવું AstAssign{fl, નવું AstVarRef{fl, iffVarp, VAccess::WRITE}, iffp});
        પાછા નવું AstVarRef{fl, iffVarp, VAccess::READ};
    }

    // Add a statement sampling the current coverpoint to sample(), under its iff guard if any
    શૂન્ય addSampleStmt(Ast_ગાંઠ* stmtp) {
        જો (m_cpGuardp) {
            m_cpGuardp->addThensp(stmtp);
        } નહીંતર {
            m_sampleFuncp->addStmtsp(stmtp);
        }
    }

    // Create previous value variable for transition tracking
    AstVar* create_Prev_કિંમત_Var(AstCoverpoint* coverpointp, Ast_ગાંઠ_પદાવલિ* exprp) {
        // Check if already created
        જો (AstVar* અચલ prevVarp = VN_CAST(coverpointp->user1p(), Var)) પાછા prevVarp;

        // Create variable to store previous sampled value
        અચલ string varName = "__Vprev_" + coverpointp->name();
        AstVar* prevVarp
            = નવું AstVar{coverpointp->fileline(), VVarType::MEMBER, varName, exprp->dtypep()};
        m_covergroupp->addMembersp(prevVarp);

        UINFO(4, "    Created previous value variable: " << varName);

        // Initialize to zero in constructor
        Ast_ગાંઠ_પદાવલિ* અચલ initExprp
            = નવું AstConst{prevVarp->fileline(), AstConst::Widthed_કિંમત{}, prevVarp->width(), 0};
        Ast_ગાંઠ_Stmt* અચલ initStmtp = નવું AstAssign{
            prevVarp->fileline(), નવું AstVarRef{prevVarp->fileline(), prevVarp, VAccess::WRITE},
            initExprp};
        m_constructorp->addStmtsp(initStmtp);

        coverpointp->user1p(prevVarp);
        પાછા prevVarp;
    }

    // Create state position variable for multi-value transition bins
    // Tracks position in sequence: 0=not started, 1=seen first item, etc.
    AstVar* createSequenceStateVar(AstCoverpoint* coverpointp, AstCoverBin* binp) {
        // Create variable to track sequence position
        અચલ string varName = "__Vseqpos_" + coverpointp->name() + "_" + binp->name();
        // Use 8-bit integer for state position (sequences rarely > 255 items)
        AstVar* stateVarp
            = નવું AstVar{binp->fileline(), VVarType::MEMBER, varName, VFlagLogicPacked{}, 8};
        m_covergroupp->addMembersp(stateVarp);

        UINFO(4, "    Created sequence state variable: " << varName);

        // Initialize to 0 (not started) in constructor
        Ast_ગાંઠ_Stmt* અચલ initStmtp = નવું AstAssign{
            stateVarp->fileline(), નવું AstVarRef{stateVarp->fileline(), stateVarp, VAccess::WRITE},
            નવું AstConst{stateVarp->fileline(), AstConst::Widthed_કિંમત{}, 8, 0}};
        m_constructorp->addStmtsp(initStmtp);

        પાછા stateVarp;
    }

    શૂન્ય generateCoverpointCode(AstCoverpoint* coverpointp) {
        UINFO(4, "  Generating code for coverpoint: " << coverpointp->name());

        // Get the coverpoint expression
        Ast_ગાંઠ_પદાવલિ* exprp = coverpointp->exprp();

        // Check automatic bins before processing
        checkAutomaticBins(coverpointp, exprp);

        // Extract all coverpoint options in a single pass
        પૂર્ણાંક at_Least_કિંમત;
        પૂર્ણાંક autoBinMax;
        extractItemOptions(coverpointp->optionsp(), at_Least_કિંમત, autoBinMax);
        UINFO(6, "    Coverpoint at_least = " << at_Least_કિંમત << " auto_bin_max = " << autoBinMax);

        // Create implicit automatic bins if no regular bins exist
        createImplicitAutoBins(coverpointp, exprp, autoBinMax);

        // A false iff guard disables sampling of the coverpoint (IEEE 1800-2023 19.5), so
        // neither is its expression evaluated, nor are its bins or transition state updated
        VL_RESTORER(m_cpGuardp);
        જો (Ast_ગાંઠ_પદાવલિ* અચલ iffp = coverpointp->iffp()) {
            m_cpGuardp = નવું AstIf{iffp->fileline(), iffp->unlinkFrBack()};
        }

        AstVar* અચલ કિંમત_Varp = નવું AstVar{
            coverpointp->fileline(), VVarType::BLOCKTEMP,
            "__VcpValue_" + sanitizeGeneratedName(coverpointp->name()), exprp->dtypep()};
        કિંમત_Varp->funcLocal(સાચું);
        m_sampleFuncp->addStmtsp(કિંમત_Varp);
        exprp->unlinkFrBack();
        addSampleStmt(નવું AstAssign{
            coverpointp->fileline(),
            નવું AstVarRef{coverpointp->fileline(), કિંમત_Varp, VAccess::WRITE}, exprp});
        coverpointp->exprp(નવું AstVarRef{coverpointp->fileline(), કિંમત_Varp, VAccess::READ});
        exprp = coverpointp->exprp();

        // Every coverpoint routes through the VlCoverpoint runtime.  Transition coverpoints are
        // included: their per-value matching is still generated as a state machine in sample()
        // (see generateCoverpoint), but the bin hit is recorded in the runtime bin
        // rather than a bare counter.
        generateCoverpoint(coverpointp, exprp, at_Least_કિંમત);
        // Sampling follows what generateCoverpoint added unguarded, such as clearing the hit list
        જો (m_cpGuardp) m_sampleFuncp->addStmtsp(m_cpGuardp);
    }

    // Build the condition under which a default bin matches: NOT(OR of all normal bins).
    Ast_ગાંઠ_પદાવલિ* buildDefaultCondition(AstCoverpoint* coverpointp, Ast_ગાંઠ_પદાવલિ* exprp,
                                       File_લીટી* fl) {
        Ast_ગાંઠ_પદાવલિ* anyBinMatchp = nullptr;
        માટે (Ast_ગાંઠ* binp = coverpointp->binsp(); binp; binp = binp->nextp()) {
            AstCoverBin* અચલ cbinp = VN_AS(binp, CoverBin);
            જો (cbinp->binsType() == VCoverBinsType::BINS_DEFAULT
                || cbinp->binsType() == VCoverBinsType::BINS_IGNORE
                || cbinp->binsType() == VCoverBinsType::BINS_ILLEGAL)
                ચાલુ;
            // The values of constructed bins are known at construction; see emitSizedSample
            જો (isConstructedBins(cbinp)) ચાલુ;
            જો (isAutoBins(cbinp)) {
                // Automatic bins partition the whole domain, leaving no default value
                જો (anyBinMatchp) VL_DO_DANGLING(ધકેલો_Deletep(anyBinMatchp), anyBinMatchp);
                પાછા નવું AstConst{fl, AstConst::BitFalse{}};
            }
            Ast_ગાંઠ_પદાવલિ* અચલ binCondp = buildBinCondition(cbinp, exprp);
            UASSERT_OBJ(binCondp, cbinp,
                        "buildBinCondition returned nullptr for non-ignore/non-illegal bin");
            anyBinMatchp = anyBinMatchp ? નવું AstOr{fl, anyBinMatchp, binCondp} : binCondp;
        }
        પાછા anyBinMatchp ? static_cast<Ast_ગાંઠ_પદાવલિ*>(નવું AstNot{fl, anyBinMatchp})
                            : static_cast<Ast_ગાંઠ_પદાવલિ*>(નવું AstConst{fl, AstConst::BitTrue{}});
    }

    //====================================================================
    // VlCoverpoint conversion

    સ્થિર તાર્કિક coverpointHasStateExclusions(અચલ AstCoverpoint* coverpointp) {
        માટે (અચલ Ast_ગાંઠ* nodep = coverpointp->binsp(); nodep; nodep = nodep->nextp()) {
            અચલ AstCoverBin* અચલ binp = VN_AS(nodep, CoverBin);
            જો (!binp->transp() && binp->rangesp()
                && (binp->binsType() == VCoverBinsType::BINS_IGNORE
                    || binp->binsType() == VCoverBinsType::BINS_ILLEGAL)) {
                પાછા સાચું;
            }
        }
        પાછા ખોટું;
    }

    સ્થિર તાર્કિક coverpointHasConstructedBins(અચલ AstCoverpoint* coverpointp) {
        માટે (અચલ Ast_ગાંઠ* nodep = coverpointp->binsp(); nodep; nodep = nodep->nextp()) {
            જો (isConstructedBins(VN_AS(nodep, CoverBin))) પાછા સાચું;
        }
        પાછા ખોટું;
    }

    // True if a Normal state bin, or an array-bin element, has no value of the coverpoint's
    // type (IEEE 1800-2023 19.5.7).  Array ranges enumerate in-type values, so cannot vanish.
    સ્થિર તાર્કિક coverpointHasEmptyBins(અચલ AstCoverpoint* coverpointp) {
        Ast_ગાંઠ_પદાવલિ* અચલ exprp = coverpointp->exprp();
        માટે (Ast_ગાંઠ* nodep = coverpointp->binsp(); nodep; nodep = nodep->nextp()) {
            અચલ AstCoverBin* અચલ binp = VN_AS(nodep, CoverBin);
            જો (!binp->binsType().binIsNormal() || binp->transp() || !binp->rangesp()) ચાલુ;
            // A wildcard array has a bin for each value its elements match, so none empty, and
            // the constructor creates no bin without values of a 'with' filter
            જો ((binp->isArray() && binp->isWildcard()) || binWith(binp)) ચાલુ;
            તાર્કિક empty = સાચું;
            માટે (Ast_ગાંઠ* valuep = binp->rangesp(); valuep; valuep = valuep->nextp()) {
                જો (binp->isArray() && VN_IS(valuep, InsideRange)) ચાલુ;
                Cross_કિંમત_Range range{valuep, resolveWidth(valuep, exprp)};
                અચલ તાર્કિક none = resolve_કિંમત(valuep, exprp, સાચું, binp->isWildcard(), range)
                                  && crossRangeEmpty(range);
                જો (binp->isArray() && none) પાછા સાચું;
                empty &= none;
            }
            જો (empty && !binp->isArray()) પાછા સાચું;
        }
        પાછા ખોટું;
    }

    // True if a coverpoint has any transition bin.  Used to decide whether sample() emits the
    // end-of-sample previous-value update that transition matching needs.
    સ્થિર તાર્કિક coverpointHasTransition(AstCoverpoint* coverpointp) {
        માટે (Ast_ગાંઠ* binp = coverpointp->binsp(); binp; binp = binp->nextp()) {
            જો (VN_AS(binp, CoverBin)->transp()) પાછા સાચું;
        }
        પાછા ખોટું;
    }

    // The interned AstBasicDType for one of the covergroup runtime keywords.
    સ્થિર AstBasicDType* basicDType(File_લીટી* fl, VBasicDTypeKwd kwd) {
        પાછા v3Global.rootp()->typeTablep()->findBasicDType(fl, kwd);
    }

    // Get (or create) the coverpoint dtype for a hit-list bound, interned so each distinct bound
    // yields one node.
    AstCoverpointDType* coverpointDType(File_લીટી* fl, uint32_t hitBound) {
        AstCoverpointDType*& typep = m_cpDTypes[hitBound];
        જો (!typep) {
            typep = નવું AstCoverpointDType{fl, hitBound};
            v3Global.rootp()->typeTablep()->addTypesp(typep);
        }
        પાછા typep;
    }

    // The key of the covergroup's type in the coverage registry, which get_coverage() queries
    std::string covergroupProtectedName() અચલ {
        પાછા VIdProtect::protect_શબ્દો_If(m_covergroupName, v3Global.opt.protectIds());
    }

    // The name of the covergroup's type, which keys the coverage registry and database: as
    // $typename names it (IEEE 1800-2023 20.6.1), so apart for covergroups of distinct scopes and
    // specializations, and alike in each Verilator run of a hierarchical design.  As design units
    // of distinct libraries may share a name, one of a library other than the default is prefixed
    // by its library, as '%l' prints it (IEEE 1800-2023 33.4).
    std::string covergroupTypeName() અચલ {
        UASSERT_OBJ(m_unitp, m_covergroupp, "Covergroup declared outside of a design unit");
        અચલ std::string& libname = m_unitp->libname();
        અચલ std::string name = m_covergroupp->dtypeName(સાચું);
        પાછા libname == "work" ? name : libname + "." + name;
    }

    // Emit the covergroup's instance handle member and the constructor statement that creates
    // its node in the per-context coverage registry.  Runs before any coverpoint or cross is
    // generated, so their runtimes can be added to the node as they are created.
    શૂન્ય generateInstanceAttach() {
        File_લીટી* અચલ fl = m_covergroupp->fileline();
        // V3LinkParse synthesizes a 'new' for every covergroup; the item generators below already
        // rely on that, and this attach runs even for a covergroup with no coverpoints at all.
        UASSERT_OBJ(m_constructorp, m_covergroupp, "Covergroup missing synthesized constructor");
        m_cgInstVarp = નવું AstVar{fl, VVarType::MEMBER, "__Vcg_inst",
                                  basicDType(fl, VBasicDTypeKwd::COVERGROUP_INSTHANDLE)};
        m_covergroupp->addMembersp(m_cgInstVarp);

        m_constructorp->addStmtsp(
            itemCall(fl, m_cgInstVarp, VCMethod::COVERGROUP_ATTACH,
                     {ctext(fl, "vlSymsp->_vm_contextp__->covergroupRegistryp()"
                                "->newCovergroupInst("
                                    + quoted(covergroupProtectedName())
                                    + (m_cgMayMerge ? ", true" : ", false") + ")")},
                     /*usePtr=*/ખોટું)
                ->makeStmt());
        // The node reads option.weight in place, so procedural assignments take effect
        AstCExpr* અચલ weightAddrp = નવું AstCExpr{fl, "&"};
        weightAddrp->add(newOptionSel(fl, optionVar(ખોટું), "weight", VAccess::READ));
        m_constructorp->addStmtsp(itemCall(fl, m_cgInstVarp, VCMethod::COVERGROUP_LEND_WEIGHT,
                                           {weightAddrp, file_લીટી_Debug(fl)}, /*usePtr=*/ખોટું)
                                      ->makeStmt());
    }

    // A '__Vcg_inst.p()-><method>()' call on the covergroup's instance node
    AstCMethodHard* instanceCall(File_લીટી* fl, VCMethod method) {
        // '__Vcg_inst.p()' -- a value handle, so '.' not '->'
        AstCMethodHard* અચલ instp
            = નવું AstCMethodHard{fl, memberRef(fl, m_cgInstVarp), VCMethod::COVERGROUP_INST_P};
        instp->usePtr(ખોટું);
        instp->dtypeSetVoid();  // Opaque receiver; only ever the 'fromp' of the call below
        AstCMethodHard* અચલ callp = નવું AstCMethodHard{fl, instp, method};
        callp->usePtr(સાચું);
        પાછા callp;
    }

    // Emit 'this->__Vcp_x = this->__Vcg_inst.p()->addCoverpoint<K>();' (or addCross), which
    // creates the item runtime in the instance node and borrows a pointer to it.
    AstAssign* makeItemCreate(File_લીટી* fl, AstVar* itemVarp, VCMethod method) {
        AstCMethodHard* અચલ createp = instanceCall(fl, method);
        createp->dtypep(itemVarp->dtypep());
        પાછા નવું AstAssign{fl, memberRef(fl, itemVarp, VAccess::WRITE), createp};
    }

    // Constant bounds of one rangesp() element (an InsideRange or a single Const).  Each bound is
    // the raw AST node -- an AstConst or an AstUnbounded ('$') -- with the const/unbounded view
    // derived on demand, so there is one source of truth per bound.  After a successful
    // constRangeBounds() neither node is null; a single Const has both bounds aliasing one node.
    સંરચના RangeBounds final {
        Ast_ગાંઠ* loNodep = nullptr;  // low bound: AstConst or AstUnbounded
        Ast_ગાંઠ* hiNodep = nullptr;  // high bound: AstConst or AstUnbounded
        તાર્કિક loUnbounded() અચલ { પાછા VN_IS(loNodep, Unbounded); }
        તાર્કિક hiUnbounded() અચલ { પાછા VN_IS(hiNodep, Unbounded); }
        AstConst* loConstp() અચલ { પાછા VN_CAST(loNodep, Const); }
        AstConst* hiConstp() અચલ { પાછા VN_CAST(hiNodep, Const); }
    };

    // Decode one rangesp() element into its constant bounds.  Returns false if rp is neither an
    // InsideRange nor a single Const, or if a present bound is non-constant or 4-state.  '$'
    // bounds are left as AstUnbounded (not resolved) -- the caller applies its own policy.
    // Centralizes the InsideRange/Const/Unbounded decode shared by the hit-list-bound paths.
    સ્થિર તાર્કિક constRangeBounds(Ast_ગાંઠ* rp, RangeBounds& rb) {
        જો (AstInsideRange* અચલ irp = VN_CAST(rp, InsideRange)) {
            rb.loNodep = irp->lhsp();
            rb.hiNodep = irp->rhsp();
        } નહીંતર જો (AstConst* અચલ cp = VN_CAST(rp, Const)) {
            rb.loNodep = rb.hiNodep = cp;
        } નહીંતર {
            પાછા ખોટું;
        }
        // Each bound must be a constant unless it is '$'; reject non-const and 4-state.
        AstConst* અચલ lc = rb.loConstp();
        AstConst* અચલ hc = rb.hiConstp();
        જો ((!lc && !rb.loUnbounded()) || (!hc && !rb.hiUnbounded())) પાછા ખોટું;
        જો ((lc && lc->અંક().isFourState()) || (hc && hc->અંક().isFourState())) પાછા ખોટું;
        પાછા સાચું;
    }

    // Collect the covered value intervals of a single (non-array) Normal bin.  Returns false
    // if any range isn't a constant/open InsideRange or single Const (e.g. wildcard, non-const).
    સ્થિર તાર્કિક extractRangeIntervals(AstCoverBin* cbinp, uint64_t maxVal,
                                      std::vector<std::pair<uint64_t, uint64_t>>& out) {
        જો (!cbinp->rangesp()) પાછા ખોટું;
        માટે (Ast_ગાંઠ* rp = cbinp->rangesp(); rp; rp = rp->nextp()) {
            RangeBounds rb;
            જો (!constRangeBounds(rp, rb)) પાછા ખોટું;
            જો ((rb.loConstp() && rb.loConstp()->width() > 64)
                || (rb.hiConstp() && rb.hiConstp()->width() > 64)) {
                પાછા ખોટું;  // Use the safe slot-count bound for wide values.
            }
            અચલ uint64_t lo = rb.loUnbounded() ? 0 : rb.loConstp()->toUQuad();
            અચલ uint64_t hi = rb.hiUnbounded() ? maxVal : rb.hiConstp()->toUQuad();
            જો (lo > hi) પાછા ખોટું;
            out.emplace_back(lo, hi);
        }
        પાછા સાચું;
    }

    // Append one Normal bin's cross-slot interval-sets to `bins` and bump `slotCount` by an upper
    // bound of the bin's slots holding one value.  Returns false if any part isn't statically
    // enumerable (the caller then falls back to the always-safe slot count).  A non-array bin
    // is one slot covering the union of its intervals; the bins of an array element or of an
    // automatic bins declaration hold disjoint values, so the element or declaration counts once.
    તાર્કિક appendBinCrossSlots(AstCoverBin* cbinp, uint64_t maxVal, Ast_ગાંઠ_પદાવલિ* exprp,
                             std::vector<std::vector<std::pair<uint64_t, uint64_t>>>& bins,
                             પૂર્ણાંક& slot_ગણતરી) {
        જો (isAutoBins(cbinp)) {
            ++slot_ગણતરી;
            bins.ધકેલો_back({{0, maxVal}});
            પાછા exprp->width() <= 64;
        }
        જો (binWith(cbinp)) {
            // A value is in one bin of a filter's, but of a sized array, in one for each range
            // list element holding it; the coverpoint's name is one element
            પૂર્ણાંક elements = 0;
            માટે (અચલ Ast_ગાંઠ* rp = binRangesp(cbinp); rp && isSizedArray(cbinp);
                 rp = rp->nextp()) {
                ++elements;
            }
            slot_ગણતરી += std::max(1, elements);
            પાછા ખોટું;
        }
        જો (cbinp->isArray() && cbinp->isWildcard() && !cbinp->arraySizep()) {
            // A value is in at most one bin of a wildcard array: one slot covering its values.
            // Signed values are sign-extended, not unsigned intervals (see computeHitListBound)
            ++slot_ગણતરી;
            જો (exprp->isSigned() || exprp->width() > 64) પાછા ખોટું;
            અચલ BinRuns runs = wildcardBinRuns(cbinp, exprp, ખોટું);
            જો (runs.unsupported) પાછા ખોટું;
            std::vector<std::pair<uint64_t, uint64_t>> ivs;
            માટે (અચલ BinRun& run : runs.runs) {
                ivs.emplace_back(run.m_lo.toUQuad(), run.m_hi.toUQuad());
            }
            bins.ધકેલો_back(std::move(ivs));
            પાછા સાચું;
        }
        જો (cbinp->isArray()) પાછા appendArrayBinCrossSlots(cbinp, exprp, bins, slot_ગણતરી);
        // Non-array bin: one slot covering the union of its intervals.
        ++slot_ગણતરી;
        std::vector<std::pair<uint64_t, uint64_t>> ivs;
        જો (cbinp->isWildcard() || !extractRangeIntervals(cbinp, maxVal, ivs)) પાછા ખોટું;
        bins.ધકેલો_back(std::move(ivs));
        પાછા સાચું;
    }

    // Append the cross slots of an array Normal bin: each element is a slot covering its
    // values, as at most one of its single-value bins holds a value.  An element holds the
    // values arrayBinRuns() gives it: those of the coverpoint type (IEEE 1800-2023 19.5.7).
    // Elements of a signed coverpoint, non-constant elements, and elements with values beyond
    // 64 bits can't be enumerated, so they count one slot but lose exactness.  Returns false if
    // any element wasn't enumerable to exact values.
    તાર્કિક appendArrayBinCrossSlots(AstCoverBin* cbinp, Ast_ગાંઠ_પદાવલિ* exprp,
                                  std::vector<std::vector<std::pair<uint64_t, uint64_t>>>& bins,
                                  પૂર્ણાંક& slot_ગણતરી) {
        // Signed values resolve sign-extended, not as unsigned intervals, and wildcard patterns
        // are not intervals
        તાર્કિક exact = !exprp->isSigned() && !cbinp->isWildcard();
        માટે (Ast_ગાંઠ* rp = cbinp->rangesp(); rp; rp = rp->nextp()) {
            ++slot_ગણતરી;
            RangeBounds rb;
            Cross_કિંમત_Range range{rp, resolveWidth(rp, exprp)};
            જો (!exact || !constRangeBounds(rp, rb)
                || !resolve_કિંમત(rp, exprp, સાચું, ખોટું, range)) {
                exact = ખોટું;
                ચાલુ;
            }
            જો (crossRangeEmpty(range)) ચાલુ;  // Its bin, if any, holds no value
            જો (range.hi.mostSetBitP1() > 64) {
                exact = ખોટું;
                ચાલુ;
            }
            bins.ધકેલો_back({{range.lo.toUQuad(), range.hi.toUQuad()}});
        }
        પાછા exact;
    }

    // Compute the hit-list bound for a coverpoint: the maximum number of Normal
    // bins one sample value can match.  Non-cross-fed coverpoints don't feed a cross, so
    // their hit list is unused -> 1.  Otherwise compute the exact max bin overlap; fall back
    // to the (always-safe) Normal-slot count when any bin isn't statically analyzable.
    પૂર્ણાંક computeHitListBound(AstCoverpoint* coverpointp, Ast_ગાંઠ_પદાવલિ* exprp, તાર્કિક crossFed) {
        જો (!crossFed) પાછા 1;
        અચલ પૂર્ણાંક width = exprp->width();
        અચલ uint64_t maxVal = (width >= 64) ? UINT64_MAX : ((1ULL << width) - 1);
        // One entry per Normal bin (cross slot): its covered intervals.
        std::vector<std::vector<std::pair<uint64_t, uint64_t>>> bins;
        પૂર્ણાંક slot_ગણતરી = 0;  // == runtime m_normal; the safe fallback bound
        // Unsigned intervals cannot establish overlap between differently sized signed values.
        તાર્કિક exact = !exprp->isSigned();
        માટે (Ast_ગાંઠ* binp = coverpointp->binsp(); binp; binp = binp->nextp()) {
            AstCoverBin* અચલ cbinp = VN_AS(binp, CoverBin);
            જો (!cbinp->binsType().binIsNormal())
                ચાલુ;  // ignore/illegal/default: not hit-listed
            જો (!appendBinCrossSlots(cbinp, maxVal, exprp, bins, slot_ગણતરી)) exact = ખોટું;
        }
        જો (!exact) પાછા std::max(1, slot_ગણતરી);
        જો (bins.empty()) પાછા 1;
        // Max overlap occurs at some interval start; count covering bins at each lo.
        std::vector<uint64_t> pts;
        માટે (અચલ auto& b : bins)
            માટે (અચલ auto& iv : b) pts.ધકેલો_back(iv.first);
        std::sort(pts.begin(), pts.end());
        pts.erase(std::unique(pts.begin(), pts.end()), pts.end());
        પૂર્ણાંક maxOverlap = 1;
        માટે (અચલ uint64_t p : pts) {
            પૂર્ણાંક cnt = 0;
            માટે (અચલ auto& b : bins) {
                માટે (અચલ auto& iv : b) {
                    જો (iv.first <= p && p <= iv.second) {
                        ++cnt;
                        તોડો;  // count each bin at most once
                    }
                }
            }
            જો (cnt > maxOverlap) maxOverlap = cnt;
        }
        પાછા maxOverlap;
    }

    // A 'this->m_member' reference for embedding in an AstCStmt
    AstVarRef* memberRef(File_લીટી* fl, AstVar* varp, VAccess access = VAccess::READ) {
        AstVarRef* અચલ refp = નવું AstVarRef{fl, varp, access};
        refp->selfPointer(VSelfPointerText{VSelfPointerText::This{}});
        પાછા refp;
    }

    // A 'this->m_member-><method>(args...)' call on one member.  usePtr is false only for
    // __Vcg_inst, which is a value handle; the item members are borrowed pointers into the
    // instance node.  Numeric arguments are AstConst; the rest are C++ text that has no AST
    // form (see ctext).
    AstCMethodHard* itemCall(File_લીટી* fl, AstVar* varp, VCMethod method,
                             અચલ std::vector<Ast_ગાંઠ_પદાવલિ*>& args = {}, તાર્કિક usePtr = સાચું) {
        AstCMethodHard* અચલ callp = નવું AstCMethodHard{fl, memberRef(fl, varp), method};
        માટે (Ast_ગાંઠ_પદાવલિ* અચલ argp : args) callp->addPinsp(argp);
        callp->usePtr(usePtr);
        callp->dtypeSetVoid();
        પાછા callp;
    }

    // An unsigned integer argument.
    સ્થિર AstConst* cnum(File_લીટી* fl, uint32_t કિંમત) { પાછા નવું AstConst{fl, કિંમત}; }

    // A literal C++ argument with no AST equivalent: a 'const char*' string literal (an SV
    // string AstConst emits '"..."s', a std::string temporary the runtime cannot borrow), a
    // VlCovBinKind enum token, a constant selection-word initializer list, a VlFileLineDebug, or
    // a '__V' temporary declared by the enclosing AstCStmt.
    સ્થિર AstCExpr* ctext(File_લીટી* fl, અચલ std::string& text) {
        પાછા નવું AstCExpr{fl, text};
    }

    // A C++ string literal.  Escapes control characters as the emitter does elsewhere -- bin
    // names and filenames reach the generated code verbatim when --protect-ids is off, and an
    // SV escaped identifier may hold a quote or backslash.
    સ્થિર std::string quoted(અચલ std::string& text) {
        પાછા "\"" + V3OutFormatter::quoteNameControls(text) + "\"";
    }

    // A 'VlFileLineDebug' argument: where the runtime reports an error about fl's construct
    સ્થિર AstCExpr* file_લીટી_Debug(File_લીટી* fl) {
        અચલ std::string filename
            = VIdProtect::protectIf(fl->filename(), v3Global.opt.protectIds());
        પાછા ctext(fl, "VlFileLineDebug{" + quoted(filename) + ", "
                             + std::to_string(fl->lineno()) + "}");
    }

    // Check that an element of an array bin (bins b[] = {values/ranges}) is a two-state
    // constant value or range; false if not, after reporting it if 'report'.
    સ્થિર તાર્કિક checkArrayBinElement(AstCoverBin* arrayBinp, Ast_ગાંઠ* rangep, તાર્કિક report = સાચું) {
        જો (અચલ AstInsideRange* અચલ irp = VN_CAST(rangep, InsideRange)) {
            અચલ AstConst* અચલ minp = VN_CAST(irp->lhsp(), Const);
            અચલ AstConst* અચલ maxp = VN_CAST(irp->rhsp(), Const);
            જો ((!minp && !VN_IS(irp->lhsp(), Unbounded))
                || (!maxp && !VN_IS(irp->rhsp(), Unbounded))) {
                જો (report) {
                    arrayBinp->v3error("Non-constant expression in array bins range; "
                                       "range bounds must be constants (IEEE 1800-2023 19.5)");
                }
                પાછા ખોટું;
            }
            જો ((minp && minp->અંક().isFourState()) || (maxp && maxp->અંક().isFourState())) {
                જો (report) {
                    arrayBinp->v3error("Four-state (x/z) value in array bins range bound; "
                                       "range bounds must be two-state constants");
                }
                પાછા ખોટું;
            }
        } નહીંતર જો (!VN_IS(rangep, Const)) {
            જો (report) {
                arrayBinp->v3error("Non-constant expression in array bins value list; "
                                   "values must be constants (IEEE 1800-2023 19.5)");
            }
            પાછા ખોટું;
        }
        પાછા સાચું;
    }

    // Individual equality targets of an array bin (bins b[] = {values/ranges}) of a real
    // coverpoint, in order; integral coverpoints generate array bins as runs (see arrayBinRuns).
    // An open-ended bound ('$', AstUnbounded) resolves to the coverpoint domain: '[lo:$]'
    // covers [lo:maxVal] and '[$:hi]' covers [0:hi].  One target is produced per value; ranges
    // whose resolved size would exceed --coverage-max-real-bins (e.g. an open '[lo:$]') are
    // unsupported -- emits COVERIGN, sets unsupportedOut, yields nothing.
    std::vector<Ast_ગાંઠ_પદાવલિ*> extract_Array_કિંમતો(AstCoverBin* arrayBinp, Ast_ગાંઠ_પદાવલિ* exprp,
                                                 તાર્કિક& unsupportedOut) {
        unsupportedOut = ખોટું;
        અચલ પૂર્ણાંક width = exprp->width();
        અચલ uint64_t maxVal = (width >= 64) ? UINT64_MAX : ((1ULL << width) - 1);
        std::vector<Ast_ગાંઠ_પદાવલિ*> કિંમતો;
        માટે (Ast_ગાંઠ* rangep = arrayBinp->rangesp(); rangep; rangep = rangep->nextp()) {
            rangep = V3Const::constifyEdit(rangep);
            જો (!checkArrayBinElement(arrayBinp, rangep)) પાછા કિંમતો;
            જો (AstInsideRange* અચલ irp = VN_CAST(rangep, InsideRange)) {
                અચલ તાર્કિક loUnb = VN_IS(irp->lhsp(), Unbounded);
                અચલ તાર્કિક hiUnb = VN_IS(irp->rhsp(), Unbounded);
                અચલ uint64_t lo = loUnb ? 0 : VN_AS(irp->lhsp(), Const)->toUQuad();
                અચલ uint64_t hi = hiUnb ? maxVal : VN_AS(irp->rhsp(), Const)->toUQuad();
                જો (hi < lo) ચાલુ;  // empty range contributes no bins
                // Guard against a '$'-bounded or otherwise huge range exploding the bin count.
                અચલ uint64_t span = hi - lo;  // == valueCount - 1 (no overflow: hi >= lo)
                જો (span >= v3Global.opt.coverageMaxRealBins()
                    || કિંમતો.size() + span + 1 > v3Global.opt.coverageMaxRealBins()) {
                    arrayBinp->v3warn(COVERIGN,
                                      "Unsupported: array 'bins' of a real coverpoint "
                                      "covering more than "
                                          << v3Global.opt.coverageMaxRealBins() << " values; bin "
                                          << arrayBinp->prettyNameQ() << " ignored.\n"
                                          << arrayBinp->warnMore()
                                          << "... Suggest a larger --coverage-max-real-bins");
                    unsupportedOut = સાચું;
                    માટે (Ast_ગાંઠ_પદાવલિ* અચલ vp : કિંમતો) VL_DO_DANGLING(ધકેલો_Deletep(vp), vp);
                    કિંમતો.clear();
                    પાછા કિંમતો;
                }
                માટે (uint64_t v = lo; v <= hi; ++v)
                    કિંમતો.ધકેલો_back(નવું AstConst{irp->fileline(), AstConst::Widthed_કિંમત{}, width,
                                                  static_cast<uint32_t>(v)});
            } નહીંતર {
                કિંમતો.ધકેલો_back(VN_AS(rangep->clone_વૃક્ષ(ખોટું), ગાંઠ_પદાવલિ));
            }
        }
        પાછા કિંમતો;
    }

    સ્થિર પૂર્ણાંક runWidth(અચલ Ast_ગાંઠ_પદાવલિ* exprp) { પાછા exprp->width() + 1; }

    // Automatic bins partition the coverpoint domain in value order (IEEE 1800-2023 19.5.3): N
    // bins, capped at the number of values, each hold 2^width / N values, and the last bin also
    // holds the remainder.  False for an invalid declaration, already reported.
    તાર્કિક autoBinRuns(AstCoverBin* binp, Ast_ગાંઠ_પદાવલિ* exprp, BinRuns& out) {
        અચલ uint32_t requested = autoBinsRequested(binp);
        જો (!requested || !exprp->dtypep()->skipRefp()->isIntegralOrPacked()) પાછા ખોટું;
        અચલ પૂર્ણાંક width = exprp->width();
        અચલ પૂર્ણાંક arithmeticWidth = runWidth(exprp);
        અચલ uint32_t ગણતરી
            = width < 32
                  ? static_cast<uint32_t>(std::min<uint64_t>(uint64_t{1} << width, requested))
                  : requested;
        BinRun run{binp, arithmeticWidth, ગણતરી};
        V3_સંખ્યા total{binp, arithmeticWidth};
        total.setBit(width, 1);
        run.m_stride.opDiv(total, V3_સંખ્યા{binp, arithmeticWidth, ગણતરી});
        અચલ Cross_કિંમત_Range domain
            = cross_કિંમત_Domain(binp, width, exprp->isSigned(), arithmeticWidth);
        run.m_lo = domain.lo;
        run.m_hi = domain.hi;
        out.runs.ધકેલો_back(std::move(run));
        out.ગણતરી = ગણતરી;
        પાછા સાચું;
    }

    // The elements of an array bin (bins b[] = {values/ranges}), in order, as runs of
    // single-value bins.  A range holds the values of the coverpoint type it contains (IEEE
    // 1800-2023 19.5.7), while a singleton names a bin even without such a value.  Errors on a
    // non-constant element.  More than --coverage-max-bins bins (e.g. an open '[lo:$]' range over
    // a wide coverpoint) are unsupported -- emits COVERIGN, and sets unsupported.
    BinRuns arrayBinRuns(AstCoverBin* arrayBinp, Ast_ગાંઠ_પદાવલિ* exprp) {
        BinRuns out;
        અચલ પૂર્ણાંક width = runWidth(exprp);
        માટે (Ast_ગાંઠ* rangep = arrayBinp->rangesp(); rangep; rangep = rangep->nextp()) {
            rangep = V3Const::constifyEdit(rangep);
            જો (!checkArrayBinElement(arrayBinp, rangep)) પાછા out;
            અચલ AstInsideRange* અચલ irp = VN_CAST(rangep, InsideRange);
            Cross_કિંમત_Range range{rangep, resolveWidth(rangep, exprp)};
            તાર્કિક empty = સાચું;
            જો (!resolve_કિંમત(rangep, exprp, સાચું, ખોટું, range)) {
                rangep->v3warn(E_UNSUPPORTED, "Unsupported: non-integral value in a coverage bin "
                                              "of an integral coverpoint.");
            } નહીંતર {
                empty = crossRangeEmpty(range);
            }
            જો (empty && irp) ચાલુ;  // A range without values contributes no bins
            BinRun run{rangep, width, 1};
            run.m_empty = empty;
            uint64_t ગણતરી = 1;
            જો (!empty) {
                run.m_lo.opAssign(range.lo);
                run.m_hi.opAssign(range.hi);
                V3_સંખ્યા span{rangep, width};
                span.opSub(run.m_hi, run.m_lo);
                // Wider spans exceed any limit
                ગણતરી = span.mostSetBitP1() > 32 ? UINT64_MAX : span.toUQuad() + 1;
            }
            જો (ગણતરી > v3Global.opt.coverageMaxBins() - out.ગણતરી) {
                arrayBinp->v3warn(COVERIGN, "Unsupported: array 'bins' covering more than "
                                                << v3Global.opt.coverageMaxBins()
                                                << " values (e.g. an open '[lo:$]' range over "
                                                   "a wide coverpoint); bin "
                                                << arrayBinp->prettyNameQ() << " ignored\n"
                                                << arrayBinp->warnMore()
                                                << "... Suggest a larger --coverage-max-bins");
                out.runs.clear();
                out.ગણતરી = 0;
                out.unsupported = સાચું;
                પાછા out;
            }
            run.m_ગણતરી = static_cast<uint32_t>(ગણતરી);
            out.ગણતરી += run.m_ગણતરી;
            out.runs.ધકેલો_back(std::move(run));
        }
        પાછા out;
    }

    // The bins of a wildcard array (wildcard bins b[] = {...}): one for each coverpoint value
    // an element matches (IEEE 1800-2023 19.5.4, 19.5.7), in value order, and named by the value
    // (19.5.1), as runs of single-value bins.  Errors on a non-constant element.  More than
    // --coverage-max-bins values are unsupported -- emits COVERIGN, and sets unsupported, and for
    // an ignore or illegal array, single.  'report' false omits these diagnostics.
    સ્થિર BinRuns wildcardBinRuns(AstCoverBin* arrayBinp, Ast_ગાંઠ_પદાવલિ* exprp, તાર્કિક report) {
        BinRuns out;
        અચલ પૂર્ણાંક width = runWidth(exprp);
        અચલ V3_સંખ્યા one{arrayBinp, width, 1};
        // Disjoint runs of the values, in value order, each value once
        std::vector<std::pair<V3_સંખ્યા, V3_સંખ્યા>> spans;
        uint64_t ગણતરી = 0;  // Values of 'spans'
        માટે (Ast_ગાંઠ* rangep = arrayBinp->rangesp(); rangep; rangep = rangep->nextp()) {
            rangep = V3Const::constifyEdit(rangep);
            જો (!checkArrayBinElement(arrayBinp, rangep, report)) {
                out.unsupported = સાચું;
                પાછા out;
            }
            Cross_કિંમત_Range range{rangep, resolveWidth(rangep, exprp)};
            જો (!resolve_કિંમત(rangep, exprp, સાચું, સાચું, range)) {
                જો (report) {
                    rangep->v3warn(E_UNSUPPORTED, "Unsupported: non-integral value in a "
                                                  "coverage bin of an integral coverpoint.");
                }
                ચાલુ;
            }
            જો (crossRangeEmpty(range)) ચાલુ;
            std::vector<std::pair<V3_સંખ્યા, V3_સંખ્યા>> found;
            crossRangeRuns(range, v3Global.opt.coverageMaxBins(), found);
            માટે (અચલ std::pair<V3_સંખ્યા, V3_સંખ્યા>& run : found) {
                // Coverpoint values, sign-extended in both widths
                spans.emplace_back(V3_સંખ્યા{rangep, width, run.first},
                                   V3_સંખ્યા{rangep, width, run.second});
            }
            std::sort(spans.begin(), spans.end(), [](અચલ auto& lhs, અચલ auto& rhs) {
                પાછા cross_કિંમત_Less(lhs.first, rhs.first);
            });
            std::vector<std::pair<V3_સંખ્યા, V3_સંખ્યા>> merged;
            માટે (અચલ std::pair<V3_સંખ્યા, V3_સંખ્યા>& span : spans) {
                // Adjacent or overlapping values join a run; 'first - 1' cannot overflow
                V3_સંખ્યા before{rangep, width};
                before.opSub(span.first, one);
                જો (merged.empty() || cross_કિંમત_Less(merged.back().second, before)) {
                    merged.ધકેલો_back(span);
                } નહીંતર જો (cross_કિંમત_Less(merged.back().second, span.second)) {
                    merged.back().second = span.second;
                }
            }
            ગણતરી = 0;
            માટે (અચલ std::pair<V3_સંખ્યા, V3_સંખ્યા>& span : merged) {
                V3_સંખ્યા size{rangep, width};
                size.opSub(span.second, span.first);
                // Beyond 2^32 values exceed any limit
                ગણતરી += size.mostSetBitP1() > 32 ? uint64_t{1} << 33 : size.toUQuad() + 1;
                જો (ગણતરી > v3Global.opt.coverageMaxBins()) તોડો;
            }
            spans = std::move(merged);
            જો (ગણતરી > v3Global.opt.coverageMaxBins()) {
                // An ignore or illegal array still excludes or checks its values, as one bin
                out.single = !arrayBinp->binsType().binIsNormal();
                જો (report) {
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
                out.unsupported = સાચું;
                પાછા out;
            }
        }
        માટે (અચલ std::pair<V3_સંખ્યા, V3_સંખ્યા>& span : spans) {
            out.runs.emplace_back(arrayBinp, width, 0);
            BinRun& run = out.runs.back();
            run.m_lo = span.first;
            run.m_hi = span.second;
            // A bin for each value, named by it in the coverpoint's type
            V3_સંખ્યા કિંમત = span.first;
            જ્યારે (સાચું) {
                અચલ V3_સંખ્યા typed{arrayBinp, exprp->width(), કિંમત};
                out.કિંમતો.ધકેલો_back(exprp->isSigned() ? typed.toDecimalS() : typed.toDecimalU());
                ++run.m_ગણતરી;
                જો (કિંમત.isCaseEq(span.second)) તોડો;
                V3_સંખ્યા next{arrayBinp, width};
                કિંમત = next.opAdd(કિંમત, one);
            }
        }
        out.ગણતરી = static_cast<uint32_t>(ગણતરી);
        પાછા out;
    }

    // The runs of an automatic bins declaration, or of an array bin of an integral coverpoint.
    // False for other bins, which do not generate as runs, including a wildcard array then one
    // bin (see BinRuns::single).
    તાર્કિક binRunsFor(AstCoverBin* binp, Ast_ગાંઠ_પદાવલિ* exprp, BinRuns& out) {
        જો (isAutoBins(binp)) {
            જો (!autoBinRuns(binp, exprp, out)) out.unsupported = સાચું;
            પાછા સાચું;
        }
        જો (binp->isArray() && binp->isWildcard()) {
            જો (!exprp->dtypep()->skipRefp()->isIntegralOrPacked()) {
                Ast_ગાંઠ_પદાવલિ* અચલ falsep = wildcardTypeError(binp, exprp);
                VL_DO_DANGLING(ધકેલો_Deletep(falsep), falsep);
                out.unsupported = સાચું;
            } નહીંતર {
                out = wildcardBinRuns(binp, exprp, સાચું);
                જો (out.single) {
                    binp->isArray(ખોટું);  // Generates as one bin
                    પાછા ખોટું;
                }
            }
            પાછા સાચું;
        }
        જો (!binp->isArray() || binp->transp() || binp->isWildcard()
            || !exprp->dtypep()->skipRefp()->isIntegralOrPacked()) {
            પાછા ખોટું;
        }
        out = arrayBinRuns(binp, exprp);
        પાછા સાચું;
    }

    // Emit a 'this->m_cp->addSingleNamer/addArrayNamer(...)' statement for one bin whose first
    // runtime bin index is 'declared'; or with 'valueNames', those naming each bin of an array
    Ast_ગાંઠ_Stmt* makeNamer(AstVar* cpVarp, AstCoverBin* binp, int64_t ગણતરી, uint32_t declared,
                           અચલ std::vector<Ast_ગાંઠ_પદાવલિ*>& કિંમતો = {},
                           અચલ std::vector<std::string>& કિંમત_Names = {}) {
        File_લીટી* અચલ fl = binp->fileline();
        CoverpointBins& bins = m_cpBins.at(cpVarp);
        અચલ uint32_t normal_ગણતરી
            = binp->binsType().binIsNormal() ? static_cast<uint32_t>(ગણતરી < 0 ? 1 : ગણતરી) : 0;
        અચલ BinSpan span{bins.total, normal_ગણતરી, declared};
        જો (binp->binsType() == VCoverBinsType::BINS_AUTO_IMPLICIT) {
            bins.implicitAuto = span;  // Selected by bin, see implicitAutoBinSpan
        } નહીંતર {
            bins.spans.emplace(binp->name(), span);
        }
        bins.total += normal_ગણતરી;
        માટે (uint32_t i = 0; bins.crossed && i < normal_ગણતરી; ++i) {
            bins.કિંમતો.ધકેલો_back({binp, કિંમતો.empty() ? nullptr : કિંમતો[i]});
        }
        // Under --protect-ids the filename and bin name flow into the coverage database
        // verbatim, so obfuscate them exactly as line/toggle coverage points are (whole-
        // unit filename, per-word bin name).  A no-op when --protect-ids is off.
        અચલ તાર્કિક prot = v3Global.opt.protectIds();
        અચલ તાર્કિક single = ગણતરી < 0;
        જો (!કિંમત_Names.empty()) {
            // A bin of a wildcard array is named by its value (IEEE 1800-2023 19.5.1)
            Ast_ગાંઠ_Stmt* stmtsp = nullptr;
            માટે (અચલ std::string& કિંમત : કિંમત_Names) {
                અચલ std::string name
                    = VIdProtect::protect_શબ્દો_If(binp->name(), prot) + "[" + કિંમત + "]";
                stmtsp = Ast_ગાંઠ::addNext(
                    stmtsp,
                    itemCall(fl, cpVarp, VCMethod::COVERGROUP_ADD_SINGLE_NAMER,
                             {ctext(fl, binp->binsType().binSetEnum()), ctext(fl, quoted(name)),
                              ctext(fl, quoted(VIdProtect::protectIf(fl->filename(), prot))),
                              cnum(fl, static_cast<uint32_t>(fl->lineno())),
                              cnum(fl, static_cast<uint32_t>(fl->firstColumn()))})
                        ->makeStmt());
            }
            પાછા stmtsp;
        }
        std::vector<Ast_ગાંઠ_પદાવલિ*> args{ctext(fl, binp->binsType().binSetEnum())};
        જો (!single) args.ધકેલો_back(cnum(fl, static_cast<uint32_t>(ગણતરી)));  // value array bin
        args.ધકેલો_back(ctext(fl, quoted(VIdProtect::protect_શબ્દો_If(binp->name(), prot))));
        args.ધકેલો_back(ctext(fl, quoted(VIdProtect::protectIf(fl->filename(), prot))));
        args.ધકેલો_back(cnum(fl, static_cast<uint32_t>(fl->lineno())));
        args.ધકેલો_back(cnum(fl, static_cast<uint32_t>(fl->firstColumn())));
        પાછા itemCall(fl, cpVarp,
                        single ? VCMethod::COVERGROUP_ADD_SINGLE_NAMER
                        : binp->binsType() == VCoverBinsType::BINS_AUTO_IMPLICIT
                            ? VCMethod::COVERGROUP_ADD_NUMBERED_NAMER
                            : VCMethod::COVERGROUP_ADD_ARRAY_NAMER,
                        args)
            ->makeStmt();
    }

    // Emit 'if (iff && cond) m_cp.incrementBin(idx);' (or recordHit, + illegal action) in sample()
    // Where a bin's hit is recorded in the runtime VlCoverpoint member.
    સંરચના ConvBinTarget final {
        AstVar* cpVarp;  // the __Vcp_<coverpoint> member
        uint32_t idx;  // bin index within that coverpoint
        તાર્કિક isNormal;  // Normal -> incrementBin (count + cross hit list); else recordHit (count)
    };

    // Emit 'this->m_cp.incrementBin(idx);' (Normal) or '.recordHit(idx);'
    // (ignore/illegal/default).
    Ast_ગાંઠ_Stmt* makeRuntimeBinHit(File_લીટી* fl, AstVar* cpVarp, Ast_ગાંઠ_પદાવલિ* idxp,
                                   તાર્કિક isNormal) {
        પાછા itemCall(fl, cpVarp,
                        isNormal ? VCMethod::COVERGROUP_INCREMENT_BIN
                                 : VCMethod::COVERGROUP_RECORD_HIT,
                        {idxp})
            ->makeStmt();
    }
    Ast_ગાંઠ_Stmt* makeRuntimeBinHit(File_લીટી* fl, અચલ ConvBinTarget& tgt) {
        પાછા makeRuntimeBinHit(fl, tgt.cpVarp, cnum(fl, static_cast<uint32_t>(tgt.idx)),
                                 tgt.isNormal);
    }

    // The condition under which a bin counts a sample: condp, and the bin's iff, and for a
    // Normal or default state bin, that the value is not excluded
    Ast_ગાંઠ_પદાવલિ* binCondition(AstCoverBin* binp, AstVar* cpVarp, Ast_ગાંઠ_પદાવલિ* condp) {
        File_લીટી* અચલ fl = binp->fileline();
        જો (binp->iffp()) condp = નવું AstLogAnd{fl, binp->iffp()->clone_વૃક્ષ(ખોટું), condp};
        અચલ auto excluded = m_excludedVars.find(cpVarp);
        જો (excluded != m_excludedVars.end() && !binp->transp()
            && (binp->binsType().binIsNormal()
                || binp->binsType() == VCoverBinsType::BINS_DEFAULT)) {
            condp = નવું AstLogAnd{
                fl, નવું AstNot{fl, નવું AstVarRef{fl, excluded->second, VAccess::READ}}, condp};
        }
        પાછા condp;
    }

    શૂન્ય emitConvHitIf(AstCoverpoint* coverpointp, AstCoverBin* binp, AstVar* cpVarp,
                       Ast_ગાંઠ_પદાવલિ* idxp, Ast_ગાંઠ_પદાવલિ* condp) {
        File_લીટી* અચલ fl = binp->fileline();
        Ast_ગાંઠ* actionp = makeRuntimeBinHit(fl, cpVarp, idxp, binp->binsType().binIsNormal());
        જો (binp->binsType() == VCoverBinsType::BINS_ILLEGAL) {
            actionp->addNext(makeIllegalBinAction(fl, "Illegal bin " + binp->prettyNameQ()
                                                          + " hit in coverpoint "
                                                          + coverpointp->prettyNameQ()));
        }
        UASSERT_OBJ(m_sampleFuncp, binp, "sample() CFunc not set for coverpoint");
        addSampleStmt(નવું AstIf{fl, binCondition(binp, cpVarp, condp), actionp, nullptr});
    }

    // Emit the sample() code of sized array 'sized': count the value in its bins holding it when
    // enabled, as other bins are, and for default bins note in matchedp whether any holds it
    શૂન્ય emitSizedSample(AstCoverpoint* coverpointp, AstCoverBin* binp, AstVar* cpVarp,
                         Ast_ગાંઠ_પદાવલિ* exprp, uint32_t sized, AstVar* matchedp) {
        File_લીટી* અચલ fl = binp->fileline();
        Ast_ગાંઠ_પદાવલિ* enabledp = binCondition(binp, cpVarp, નવું AstConst{fl, AstConst::BitTrue{}});
        UASSERT_OBJ(m_sampleFuncp, binp, "sample() CFunc not set for coverpoint");
        અચલ તાર્કિક illegal = binp->binsType() == VCoverBinsType::BINS_ILLEGAL;
        જો (illegal) {
            // The illegal action reads the condition too, which is evaluated once, as the guard
            // may have side effects
            AstVar* અચલ varp
                = નવું AstVar{fl, VVarType::BLOCKTEMP,
                             "__VcpEnabled_" + sanitizeGeneratedName(coverpointp->name()) + "_"
                                 + cvtToStr(sized),
                             binp->findBitDType()};
            varp->funcLocal(સાચું);
            m_sampleFuncp->addStmtsp(varp);
            addSampleStmt(નવું AstAssign{fl, નવું AstVarRef{fl, varp, VAccess::WRITE}, enabledp});
            enabledp = નવું AstVarRef{fl, varp, VAccess::READ};
        }
        AstCMethodHard* અચલ callp
            = itemCall(fl, cpVarp,
                       exprp->isWide() ? VCMethod::COVERGROUP_SIZED_SAMPLE_W
                                       : VCMethod::COVERGROUP_SIZED_SAMPLE,
                       {cnum(fl, sized), exprp->clone_વૃક્ષ(ખોટું), enabledp});
        callp->dtypeSetBit();
        જો (illegal) {
            addSampleStmt(નવું AstIf{fl, નવું AstLogAnd{fl, callp, enabledp->clone_વૃક્ષ(ખોટું)},
                                    makeIllegalBinAction(fl, "Illegal bin " + binp->prettyNameQ()
                                                                 + " hit in coverpoint "
                                                                 + coverpointp->prettyNameQ())});
        } નહીંતર જો (matchedp && binp->binsType().binIsNormal()) {
            addSampleStmt(
                નવું AstAssign{fl, નવું AstVarRef{fl, matchedp, VAccess::WRITE},
                              નવું AstOr{fl, નવું AstVarRef{fl, matchedp, VAccess::READ}, callp}});
        } નહીંતર {
            addSampleStmt(callp->makeStmt());
        }
    }

    // A variable local to the constructor
    AstVar* constructorTemp(File_લીટી* fl, અચલ string& name, Ast_ગાંઠ_DType* dtypep) {
        AstVar* અચલ varp = નવું AstVar{fl, VVarType::BLOCKTEMP, name, dtypep};
        varp->funcLocal(સાચું);
        m_constructorp->addStmtsp(varp);
        પાછા varp;
    }

    // Truncate or extend, as its signedness sets, a value to a type
    સ્થિર Ast_ગાંઠ_પદાવલિ* resize_કિંમત(Ast_ગાંઠ_પદાવલિ* valuep, Ast_ગાંઠ_DType* dtypep) {
        File_લીટી* અચલ fl = valuep->fileline();
        જો (valuep->width() > dtypep->width()) {
            valuep = નવું AstSel{fl, valuep, 0, dtypep->width()};
        } નહીંતર જો (valuep->width() < dtypep->width()) {
            valuep = valuep->isSigned()
                         ? static_cast<Ast_ગાંઠ_પદાવલિ*>(નવું AstExtendS{fl, valuep, dtypep->width()})
                         : નવું AstExtend{fl, valuep, dtypep->width()};
        }
        valuep->dtypep(dtypep);
        પાછા valuep;
    }

    // The runtime arguments of a count, which the constructor evaluates once into a temporary:
    // its value, a count wider than 64 bits saturated over any limit, and whether it is positive
    std::pair<Ast_ગાંઠ_પદાવલિ*, Ast_ગાંઠ_પદાવલિ*> new_ગણતરી_Args(File_લીટી* fl, અચલ string& name,
                                                       Ast_ગાંઠ_પદાવલિ* exprp) {
        AstVar* અચલ countp = constructorTemp(fl, name, exprp->dtypep());
        m_constructorp->addStmtsp(
            નવું AstAssign{fl, નવું AstVarRef{fl, countp, VAccess::WRITE}, exprp->clone_વૃક્ષ(ખોટું)});
        અચલ auto ગણતરી_Ref = [&]() { પાછા નવું AstVarRef{fl, countp, VAccess::READ}; };
        AstConst* અચલ zerop = નવું AstConst{fl, AstConst::DTyped{}, countp->dtypep()};
        Ast_ગાંઠ_પદાવલિ* અચલ positivep
            = countp->isSigned() ? static_cast<Ast_ગાંઠ_પદાવલિ*>(નવું AstGtS{fl, ગણતરી_Ref(), zerop})
                                 : નવું AstNeq{fl, ગણતરી_Ref(), zerop};
        Ast_ગાંઠ_પદાવલિ* valuep = resize_કિંમત(ગણતરી_Ref(), countp->findUInt64DType());
        જો (countp->width() > VL_QUADSIZE) {
            valuep = નવું AstCond{
                fl,
                નવું AstRedOr{
                    fl, નવું AstSel{fl, ગણતરી_Ref(), VL_QUADSIZE, countp->width() - VL_QUADSIZE}},
                નવું AstConst{fl, AstConst::Unsized64{}, std::numeric_limits<uint64_t>::max()},
                valuep};
            valuep->dtypeSetUInt64();
        }
        પાછા {valuep, positivep};
    }
    // The runtime arguments of a constant count
    સ્થિર std::pair<Ast_ગાંઠ_પદાવલિ*, Ast_ગાંઠ_પદાવલિ*> new_ગણતરી_Args(File_લીટી* fl, uint64_t ગણતરી) {
        પાછા {નવું AstConst{fl, AstConst::Unsized64{}, ગણતરી},
                નવું AstConst{fl, AstConst::BitTrue{}}};
    }

    // The values of a coverpoint, which its name denotes (IEEE 1800-2023 19.5.1.1), as runs in
    // value order: an enumerated type's values (6.19), else all values of its type.  Bounds
    // are at runWidth(), sign-extended like a CrossValueRange's.
    સ્થિર std::vector<std::pair<V3_સંખ્યા, V3_સંખ્યા>> coverpoint_કિંમતો(Ast_ગાંઠ* nodep,
                                                                       Ast_ગાંઠ_પદાવલિ* exprp) {
        અચલ પૂર્ણાંક width = runWidth(exprp);
        std::vector<std::pair<V3_સંખ્યા, V3_સંખ્યા>> runs;
        અચલ AstEnumDType* અચલ enump = VN_CAST(exprp->dtypep()->skipRefToEnump(), EnumDType);
        જો (!enump) {
            અચલ Cross_કિંમત_Range domain
                = cross_કિંમત_Domain(nodep, exprp->width(), exprp->isSigned(), width);
            runs.emplace_back(domain.lo, domain.hi);
            પાછા runs;
        }
        std::vector<V3_સંખ્યા> કિંમતો;
        માટે (અચલ AstEnumItem* itemp = enump->itemsp(); itemp;
             itemp = VN_AS(itemp->nextp(), EnumItem)) {
            અચલ V3_સંખ્યા& અંક = VN_AS(itemp->valuep(), Const)->અંક();
            જો (અંક.isFourState()) ચાલુ;  // Not a coverpoint value (19.5.7)
            કિંમતો.emplace_back(nodep, width);
            જો (exprp->isSigned()) {
                કિંમતો.back().opExtendS(અંક, અંક.width());
            } નહીંતર {
                કિંમતો.back().opAssign(અંક);
            }
        }
        std::sort(કિંમતો.begin(), કિંમતો.end(), cross_કિંમત_Less);
        અચલ V3_સંખ્યા one{nodep, width, 1};
        માટે (અચલ V3_સંખ્યા& કિંમત : કિંમતો) {
            V3_સંખ્યા next{nodep, width};
            જો (!runs.empty() && next.opAdd(runs.back().second, one).isCaseEq(કિંમત)) {
                runs.back().second = કિંમત;
            } નહીંતર {
                runs.emplace_back(કિંમત, કિંમત);
            }
        }
        પાછા runs;
    }

    // Emit the constructor code building the bins 'binp' whose values it computes: the values of
    // each element that are coverpoint values (IEEE 1800-2023 19.5.7), those a 'with' filter
    // keeps (19.5.1.1), then its bins
    શૂન્ય generateConstructedBins(AstCoverpoint* coverpointp, AstCoverBin* binp, AstVar* cpVarp,
                                 Ast_ગાંઠ_પદાવલિ* exprp) {
        File_લીટી* અચલ fl = binp->fileline();
        અચલ string prefix
            = m_sizedNames.get(sanitizeGeneratedName(coverpointp->name() + "__" + binp->name()));
        // The count, and whether it is positive; one of a filter's scalar bin, or bin per value
        અચલ std::pair<Ast_ગાંઠ_પદાવલિ*, Ast_ગાંઠ_પદાવલિ*> ગણતરી
            = binp->arraySizep() ? new_ગણતરી_Args(fl, prefix + "_count", binp->arraySizep())
                                 : new_ગણતરી_Args(fl, 1);
        AstCoverWith* અચલ withp = binWith(binp);
        જો (withp && !binRangesp(binp)) {  // The coverpoint's name: all of its values
            માટે (અચલ std::pair<V3_સંખ્યા, V3_સંખ્યા>& run : coverpoint_કિંમતો(binp, exprp)) {
                m_constructorp->addStmtsp(itemCall(fl, cpVarp,
                                                   exprp->isWide()
                                                       ? VCMethod::COVERGROUP_SIZED_RANGE_W
                                                       : VCMethod::COVERGROUP_SIZED_RANGE,
                                                   {new_કિંમત_Const(fl, run.first, exprp),
                                                    new_કિંમત_Const(fl, run.second, exprp)})
                                              ->makeStmt());
            }
        }
        uint32_t element = 0;
        માટે (Ast_ગાંઠ* rangep = binRangesp(binp); rangep; rangep = rangep->nextp()) {
            જો (VN_IS(rangep, Unbounded)) {  // A parameter of '$'
                binp->v3error("Bins value may not be '$', which may only bound a range "
                              "(IEEE 1800-2023 6.20.7)");
                ચાલુ;
            }
            generateSizedElement(cpVarp, binp, rangep, exprp, prefix + "_" + cvtToStr(element++));
        }
        જો (withp) generateWithFilter(binp, withp, cpVarp, exprp, prefix);
        std::vector<Ast_ગાંઠ_પદાવલિ*> args{ctext(fl, binp->binsType().binSetEnum()), ગણતરી.first,
                                       ગણતરી.second};
        // A filter's bins have the limit it began with
        જો (!withp) args.ધકેલો_back(cnum(fl, v3Global.opt.coverageMaxBins()));
        અચલ તાર્કિક prot = v3Global.opt.protectIds();
        args.ધકેલો_back(ctext(fl, quoted(VIdProtect::protect_શબ્દો_If(binp->name(), prot))));
        args.ધકેલો_back(ctext(fl, quoted(VIdProtect::protectIf(fl->filename(), prot))));
        args.ધકેલો_back(cnum(fl, static_cast<uint32_t>(fl->lineno())));
        args.ધકેલો_back(cnum(fl, static_cast<uint32_t>(fl->firstColumn())));
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
    શૂન્ય generateWithFilter(AstCoverBin* binp, AstCoverWith* withp, AstVar* cpVarp,
                            Ast_ગાંઠ_પદાવલિ* exprp, અચલ string& prefix) {
        File_લીટી* અચલ fl = withp->fileline();
        અચલ તાર્કિક wide = exprp->isWide();
        અચલ string grouping = !binp->isArray()     ? "Single"
                                : binp->arraySizep() ? "Fixed"
                                                     : "Values";
        m_constructorp->addStmtsp(itemCall(fl, cpVarp, VCMethod::COVERGROUP_WITH_BEGIN,
                                           {ctext(fl, "VlCovBinGrouping::" + grouping),
                                            cnum(fl, v3Global.opt.coverageMaxBins())})
                                      ->makeStmt());
        // The candidates count in the coverpoint's width, and the filter reads each as 'item',
        // of the coverpoint's type, so that a filter changing 'item' cannot change the loop
        Ast_ગાંઠ_DType* અચલ કિંમત_DTypep = exprp->dtypep();
        AstVar* અચલ valuep = constructorTemp(fl, prefix + "_value", કિંમત_DTypep);
        AstVar* અચલ lastp = constructorTemp(fl, prefix + "_last", કિંમત_DTypep);
        AstVar* અચલ firstp = constructorTemp(fl, prefix + "_first", કિંમત_DTypep);
        AstVar* અચલ runp = constructorTemp(fl, prefix + "_run", binp->findBitDType());
        AstVar* અચલ morep = constructorTemp(fl, prefix + "_more", binp->findBitDType());
        AstVar* અચલ itemp = withp->itemp()->unlinkFrBack();
        itemp->name(prefix + "_item");
        m_constructorp->addStmtsp(itemp);
        અચલ auto ref = [&](AstVar* varp) { પાછા નવું AstVarRef{fl, varp, VAccess::READ}; };
        અચલ auto assign = [&](AstVar* varp, Ast_ગાંઠ_પદાવલિ* rhsp) -> Ast_ગાંઠ* {
            પાછા નવું AstAssign{fl, નવું AstVarRef{fl, varp, VAccess::WRITE}, rhsp};
        };
        અચલ auto flag = [&](AstVar* varp, તાર્કિક કિંમત) {
            પાછા assign(varp, કિંમત ? નવું AstConst{fl, AstConst::BitTrue{}}
                                      : નવું AstConst{fl, AstConst::BitFalse{}});
        };
        અચલ auto bound = [&](VCMethod narrow, VCMethod wideMethod, AstVar* varp) -> Ast_ગાંઠ* {
            જો (wide) {
                પાછા itemCall(fl, cpVarp, wideMethod, {નવું AstVarRef{fl, varp, VAccess::WRITE}})
                    ->makeStmt();
            }
            AstCMethodHard* અચલ callp = itemCall(fl, cpVarp, narrow);
            callp->dtypeSetUInt64();
            પાછા assign(varp, resize_કિંમત(callp, કિંમત_DTypep));
        };
        અચલ auto keep = [&](Ast_ગાંઠ_પદાવલિ* lop, Ast_ગાંઠ_પદાવલિ* hip) {
            AstCMethodHard* અચલ callp = itemCall(
                fl, cpVarp, wide ? VCMethod::COVERGROUP_WITH_RUN_W : VCMethod::COVERGROUP_WITH_RUN,
                {lop, hip});
            callp->dtypeSetBit();
            પાછા assign(morep, callp);
        };
        અચલ auto step = [&](તાર્કિક up) {
            AstConst* અચલ onep = નવું AstConst{fl, AstConst::Widthed_કિંમત{}, exprp->width(), 1};
            Ast_ગાંઠ_પદાવલિ* અચલ stepp
                = up ? static_cast<Ast_ગાંઠ_પદાવલિ*>(નવું AstAdd{fl, ref(valuep), onep})
                     : નવું AstSub{fl, ref(valuep), onep};
            stepp->dtypep(કિંમત_DTypep);
            પાછા stepp;
        };
        AstLoop* અચલ innerp = નવું AstLoop{fl};
        innerp->addStmtsp(assign(itemp, ref(valuep)));
        innerp->addStmtsp(નવું AstIf{
            fl, withp->filterp()->unlinkFrBack(),
            નવું AstIf{fl, નવું AstNot{fl, ref(runp)},
                      assign(firstp, ref(valuep))->addNext(flag(runp, સાચું))},
            નવું AstIf{fl, ref(runp), keep(ref(firstp), step(ખોટું))->addNext(flag(runp, ખોટું))}});
        innerp->addStmtsp(નવું AstLoopTest{
            fl, innerp, નવું AstLogAnd{fl, ref(morep), નવું AstNeq{fl, ref(valuep), ref(lastp)}}});
        innerp->addStmtsp(assign(valuep, step(સાચું)));
        AstLoop* અચલ outerp = નવું AstLoop{fl};
        AstCMethodHard* અચલ nextp = itemCall(fl, cpVarp, VCMethod::COVERGROUP_WITH_NEXT);
        nextp->dtypeSetBit();
        outerp->addStmtsp(નવું AstLoopTest{fl, outerp, nextp});
        outerp->addStmtsp(
            bound(VCMethod::COVERGROUP_WITH_LO, VCMethod::COVERGROUP_WITH_LO_W, valuep));
        outerp->addStmtsp(
            bound(VCMethod::COVERGROUP_WITH_HI, VCMethod::COVERGROUP_WITH_HI_W, lastp));
        outerp->addStmtsp(flag(runp, ખોટું));
        outerp->addStmtsp(innerp);
        outerp->addStmtsp(નવું AstIf{fl, ref(runp), keep(ref(firstp), ref(lastp))});
        m_constructorp->addStmtsp(flag(morep, સાચું));
        m_constructorp->addStmtsp(outerp);
    }

    // Emit 'sizedRange(lo, hi)' for the coverpoint values of an element of bins 'binp', whose
    // values the constructor computes: resolved now if constant, else when constructed by
    // clipping to the coverpoint's values.  A wildcard pattern's values give a range for each
    // run of them.  'prefix' names its temporaries.
    શૂન્ય generateSizedElement(AstVar* cpVarp, અચલ AstCoverBin* binp, Ast_ગાંઠ* rangep,
                              Ast_ગાંઠ_પદાવલિ* exprp, અચલ string& prefix) {
        File_લીટી* અચલ fl = rangep->fileline();
        અચલ VCMethod method = exprp->isWide() ? VCMethod::COVERGROUP_SIZED_RANGE_W
                                                : VCMethod::COVERGROUP_SIZED_RANGE;
        અચલ AstInsideRange* અચલ irp = VN_CAST(rangep, InsideRange);
        Ast_ગાંઠ_પદાવલિ* અચલ lowp = irp ? irp->lhsp() : VN_AS(rangep, ગાંઠ_પદાવલિ);
        Ast_ગાંઠ_પદાવલિ* અચલ highp = irp ? irp->rhsp() : nullptr;
        અચલ auto unbounded
            = [](અચલ Ast_ગાંઠ_પદાવલિ* boundp) { પાછા !boundp || VN_IS(boundp, Unbounded); };
        અચલ auto constant
            = [&](અચલ Ast_ગાંઠ_પદાવલિ* boundp) { પાછા unbounded(boundp) || VN_IS(boundp, Const); };
        અચલ auto integral = [&](અચલ Ast_ગાંઠ_પદાવલિ* boundp) {
            પાછા unbounded(boundp) || boundp->dtypep()->skipRefp()->isIntegralOrPacked();
        };
        જો (constant(lowp) && constant(highp)) {
            અચલ auto fourState = [](અચલ Ast_ગાંઠ_પદાવલિ* boundp) {
                અચલ AstConst* અચલ constp = VN_CAST(boundp, Const);
                પાછા constp && constp->અંક().isFourState();
            };
            જો (irp && (fourState(lowp) || fourState(highp))) {
                rangep->v3error("Four-state (x/z) value in "
                                << (binp->isArray() ? "array bins" : "bin")
                                << " range bound; range bounds must be two-state constants");
                પાછા;
            }
            Cross_કિંમત_Range range{rangep, resolveWidth(rangep, exprp)};
            જો (!resolve_કિંમત(rangep, exprp, સાચું, binp->isWildcard(), range)) {
                rangep->v3warn(E_UNSUPPORTED, "Unsupported: non-integral value in a coverage bin "
                                              "of an integral coverpoint.");
            } નહીંતર જો (!crossRangeEmpty(range)) {
                std::vector<std::pair<V3_સંખ્યા, V3_સંખ્યા>> runs{{range.lo, range.hi}};
                જો (range.wildcard) {  // checkConstructedBins bounded the runs
                    runs.clear();
                    crossRangeRuns(range, v3Global.opt.coverageMaxBins(), runs);
                }
                માટે (અચલ std::pair<V3_સંખ્યા, V3_સંખ્યા>& run : runs) {
                    m_constructorp->addStmtsp(itemCall(fl, cpVarp, method,
                                                       {new_કિંમત_Const(fl, run.first, exprp),
                                                        new_કિંમત_Const(fl, run.second, exprp)})
                                                  ->makeStmt());
                }
            }
            પાછા;
        }
        જો (!integral(lowp) || !integral(highp)) {
            rangep->v3warn(E_UNSUPPORTED, "Unsupported: non-integral value in a coverage bin "
                                          "of an integral coverpoint.");
            પાછા;
        }
        // Compare values and the coverpoint's domain signed, in a width holding all of them
        અચલ auto boundWidth
            = [&](અચલ Ast_ગાંઠ_પદાવલિ* boundp) { પાછા unbounded(boundp) ? 0 : boundp->width(); };
        અચલ પૂર્ણાંક width = std::max({exprp->width(), boundWidth(lowp), boundWidth(highp)}) + 1;
        અચલ Cross_કિંમત_Range domain
            = cross_કિંમત_Domain(rangep, exprp->width(), exprp->isSigned(), width);
        AstVar* અચલ lop = constructorTemp(fl, prefix + "_lo", exprp->dtypep());
        lop->dtypeSetBitSized(width, VSigning::SIGNED);
        AstVar* અચલ hip = constructorTemp(fl, prefix + "_hi", lop->dtypep());
        અચલ auto bound = [&](Ast_ગાંઠ_પદાવલિ* boundp, અચલ V3_સંખ્યા& limit) -> Ast_ગાંઠ_પદાવલિ* {
            જો (unbounded(boundp)) {
                AstConst* અચલ limitp = નવું AstConst{fl, limit};
                limitp->dtypeFrom(lop);
                પાછા limitp;
            }
            Ast_ગાંઠ_પદાવલિ* valuep = boundp->clone_વૃક્ષ(ખોટું);
            // A value no wider than a signed coverpoint has its type (see crossRangeBound)
            જો (exprp->isSigned() && boundp->width() <= exprp->width()) {
                valuep = resize_કિંમત(valuep, exprp->dtypep());
            }
            પાછા resize_કિંમત(valuep, lop->dtypep());
        };
        અચલ auto ref = [&](AstVar* varp, VAccess access = VAccess::READ) {
            પાછા નવું AstVarRef{fl, varp, access};
        };
        m_constructorp->addStmtsp(
            નવું AstAssign{fl, ref(lop, VAccess::WRITE), bound(lowp, domain.lo)});
        m_constructorp->addStmtsp(
            નવું AstAssign{fl, ref(hip, VAccess::WRITE), irp ? bound(highp, domain.hi) : ref(lop)});
        AstConst* અચલ minp = નવું AstConst{fl, domain.lo};
        AstConst* અચલ maxp = નવું AstConst{fl, domain.hi};
        minp->dtypeFrom(lop);
        maxp->dtypeFrom(lop);
        AstCond* અચલ lowerp
            = નવું AstCond{fl, નવું AstLtS{fl, ref(lop), minp}, minp->clone_વૃક્ષ(ખોટું), ref(lop)};
        AstCond* અચલ upperp
            = નવું AstCond{fl, નવું AstGtS{fl, ref(hip), maxp}, maxp->clone_વૃક્ષ(ખોટું), ref(hip)};
        lowerp->dtypeFrom(lop);
        upperp->dtypeFrom(lop);
        m_constructorp->addStmtsp(નવું AstAssign{fl, ref(lop, VAccess::WRITE), lowerp});
        m_constructorp->addStmtsp(નવું AstAssign{fl, ref(hip, VAccess::WRITE), upperp});
        m_constructorp->addStmtsp(નવું AstIf{fl, નવું AstLteS{fl, ref(lop), ref(hip)},
                                            itemCall(fl, cpVarp, method,
                                                     {resize_કિંમત(ref(lop), exprp->dtypep()),
                                                      resize_કિંમત(ref(hip), exprp->dtypep())})
                                                ->makeStmt()});
    }

    // The runtime index of the bin of a run holding the coverpoint value, which is in the run:
    // declared + (value - lo) / stride, capped at the last bin, which holds any remainder.
    સ્થિર Ast_ગાંઠ_પદાવલિ* runBinIndex(File_લીટી* fl, Ast_ગાંઠ_પદાવલિ* exprp, અચલ BinRun& run) {
        જો (run.m_ગણતરી == 1) પાછા cnum(fl, run.m_declared);
        અચલ પૂર્ણાંક width = exprp->width();
        // A run spans at most 2^width values, so offsets in it are unsigned width-bit numbers
        Ast_ગાંઠ_પદાવલિ* indexp
            = નવું AstSub{fl, exprp->clone_વૃક્ષ(ખોટું), new_કિંમત_Const(fl, run.m_lo, exprp)};
        indexp->dtypeSetBitSized(width, VSigning::UNSIGNED);
        V3_સંખ્યા stride{fl, width, 0};
        stride.opAssign(run.m_stride);
        જો (stride.ગણતરી_Ones() != 1) {
            indexp = નવું AstDiv{fl, indexp, નવું AstConst{fl, stride}};
        } નહીંતર જો (!stride.isEqOne()) {
            indexp = નવું AstShiftR{fl, indexp, નવું AstConst{fl, stride.mostSetBitP1() - 1}};
        }
        // Compare the run's values with those of count bins of stride values, without overflow
        અચલ પૂર્ણાંક extWidth = run.m_lo.width() + 1;
        V3_સંખ્યા lo{fl, extWidth, 0};
        lo.opExtendS(run.m_lo, run.m_lo.width());
        V3_સંખ્યા span{fl, extWidth, 0};
        span.opExtendS(run.m_hi, run.m_hi.width());
        span.opSub(V3_સંખ્યા{span}, lo);
        V3_સંખ્યા covered{fl, extWidth, 0};
        covered.opAssign(run.m_stride);
        covered.opMul(V3_સંખ્યા{covered}, V3_સંખ્યા{fl, extWidth, run.m_ગણતરી});
        V3_સંખ્યા hasRemainder{fl, 1, 0};
        જો (!hasRemainder.opGte(span, covered).isEqZero()) {
            AstConst* અચલ lastp = નવું AstConst{fl, V3_સંખ્યા{fl, width, run.m_ગણતરી - 1}};
            indexp = નવું AstCond{fl, નવું AstGt{fl, indexp->clone_વૃક્ષ(ખોટું), lastp},
                                 lastp->clone_વૃક્ષ(ખોટું), indexp};
        }
        જો (width < VL_IDATASIZE) {
            indexp = નવું AstExtend{fl, indexp, VL_IDATASIZE};
        } નહીંતર જો (width > VL_IDATASIZE) {
            indexp = નવું AstSel{fl, indexp, 0, VL_IDATASIZE};
        }
        પાછા નવું AstAdd{fl, cnum(fl, run.m_declared), indexp};
    }

    // Emit the sample() hit of a run of bins, whose code does not grow with its number of bins:
    //   if (iff && lo <= value && value <= hi) m_cp.incrementBin(<runBinIndex>);
    શૂન્ય emitRunHit(AstCoverpoint* coverpointp, AstCoverBin* binp, AstVar* cpVarp,
                    Ast_ગાંઠ_પદાવલિ* exprp, અચલ BinRun& run) {
        File_લીટી* અચલ fl = binp->fileline();
        AstConst* અચલ lop = new_કિંમત_Const(fl, run.m_lo, exprp);
        Ast_ગાંઠ_પદાવલિ* condp = nullptr;
        જો (run.m_lo.isCaseEq(run.m_hi)) {
            condp = નવું AstEq{fl, exprp->clone_વૃક્ષ(ખોટું), lop};
        } નહીંતર {
            AstConst* અચલ hip = new_કિંમત_Const(fl, run.m_hi, exprp);
            condp = makeRangeCondition(fl, exprp, lop, hip);
            VL_DO_DANGLING(ધકેલો_Deletep(lop), lop);
            VL_DO_DANGLING(ધકેલો_Deletep(hip), hip);
        }
        emitConvHitIf(coverpointp, binp, cpVarp, runBinIndex(fl, exprp, run), condp);
    }

    // Emit a transition bin's hit action into sample():
    //   if (cond) { m_cp.incrementBin/recordHit(idx); [illegal: $error; $stop] }
    // Used by the transition generators so a completed sequence records into the runtime bin.
    શૂન્ય addConvTransHitIf(AstCoverpoint* coverpointp, AstCoverBin* binp, અચલ ConvBinTarget& tgt,
                           Ast_ગાંઠ_પદાવલિ* condp) {
        File_લીટી* અચલ fl = binp->fileline();
        Ast_ગાંઠ* actionp = makeRuntimeBinHit(fl, tgt);
        જો (binp->binsType() == VCoverBinsType::BINS_ILLEGAL) {
            actionp->addNext(makeIllegalBinAction(
                fl, "Illegal transition bin " + binp->prettyNameQ() + " hit in coverpoint "
                        + coverpointp->prettyNameQ()));
        }
        UASSERT_OBJ(m_sampleFuncp, binp, "sample() CFunc not set for transition bin");
        addSampleStmt(નવું AstIf{fl, condp, actionp, nullptr});
    }

    // Route a coverpoint through a VlCoverpoint member: emit the member, its sample()
    // increments, the constructor configuration (init + namers), and registration.
    શૂન્ય generateCoverpoint(AstCoverpoint* coverpointp, Ast_ગાંઠ_પદાવલિ* exprp, પૂર્ણાંક at_Least_કિંમત) {
        File_લીટી* અચલ fl = coverpointp->fileline();
        અચલ તાર્કિક dynamic = m_runtimePoints.ગણતરી(coverpointp);
        UINFO(4, "  Generating VlCoverpoint member: " << coverpointp->name());

        // Size the hit list to the gen-time max bin overlap (1 unless cross-fed with
        // overlapping ranges), so no cross hit is ever dropped and storage is minimal.
        અચલ તાર્કિક crossFed = m_crossedCpNames.ગણતરી(coverpointp->name()) != 0;
        અચલ પૂર્ણાંક hitBound = computeHitListBound(coverpointp, exprp, crossFed);
        UINFO(6, "    Hit-list bound (max bin overlap) = " << hitBound);
        AstVar* અચલ cpVarp = નવું AstVar{fl, VVarType::MEMBER, "__Vcp_" + coverpointp->name(),
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
        જો (crossFed) {
            UASSERT_OBJ(m_sampleFuncp, coverpointp, "sample() CFunc not set for clearHitList");
            m_sampleFuncp->addStmtsp(
                itemCall(fl, cpVarp, VCMethod::COVERGROUP_CLEAR_HIT_LIST)->makeStmt());
        }
        જો (dynamic && coverpointHasStateExclusions(coverpointp)) {
            AstVar* અચલ excludedp
                = નવું AstVar{fl, VVarType::BLOCKTEMP,
                             "__VcpExcluded_" + sanitizeGeneratedName(coverpointp->name()),
                             coverpointp->findBitDType()};
            excludedp->funcLocal(સાચું);
            m_sampleFuncp->addStmtsp(excludedp);
            AstCMethodHard* અચલ callp
                = itemCall(fl, cpVarp,
                           exprp->isWide() ? VCMethod::COVERGROUP_કિંમત_EXCLUDED_W
                                           : VCMethod::COVERGROUP_કિંમત_EXCLUDED,
                           {exprp->clone_વૃક્ષ(ખોટું)});
            callp->dtypeSetBit();
            addSampleStmt(નવું AstAssign{fl, નવું AstVarRef{fl, excludedp, VAccess::WRITE}, callp});
            m_excludedVars.emplace(cpVarp, excludedp);
        }

        // Walk bins (non-default, then default), assigning sequential indices that match the
        // namer append order; emit sample increments and collect namer statements.  Constructed
        // bins follow them all, placed when the coverpoint is constructed.
        std::vector<Ast_ગાંઠ_Stmt*> namerStmts;
        std::vector<AstCoverBin*> defaultBins;
        std::vector<AstCoverBin*> sizedBins;
        std::vector<std::tuple<AstCoverBin*, uint32_t, Ast_ગાંઠ_પદાવલિ*>> metadata;
        std::vector<અચલ BinRun*> runMetadata;
        uint64_t idx = 0;  // Runtime index of the next bin; 32-bit once checked below
        માટે (Ast_ગાંઠ* binp = coverpointp->binsp(); binp; binp = binp->nextp()) {
            AstCoverBin* અચલ cbinp = VN_AS(binp, CoverBin);
            અચલ પૂર્ણાંક errorsBefore = dynamic ? V3Error::error_ગણતરી() : 0;
            જો (cbinp->binsType() == VCoverBinsType::BINS_DEFAULT) {
                defaultBins.ધકેલો_back(cbinp);
                ચાલુ;
            }
            જો (isConstructedBins(cbinp)) {
                UASSERT_OBJ(dynamic, cbinp, "Constructed bins without value metadata");
                BinSpan span;
                span.sized = static_cast<int32_t>(sizedBins.size());
                m_cpBins.at(cpVarp).spans.emplace(cbinp->name(), span);
                sizedBins.ધકેલો_back(cbinp);
                ચાલુ;
            }
            જો (cbinp->transp()) {
                // Transition bin (incl. array transition 'bins t[] = (a=>b),(c=>d)' and
                // illegal_bins/ignore_bins transitions).  All sequences of one transition bin
                // share a bin name and merge in the coverage DB to a single point, so model
                // them as one runtime bin incremented by any matching sequence.  The sequence
                // matching is generated as a state machine, with the hit routed to this bin's
                // runtime slot.
                namerStmts.ધકેલો_back(makeNamer(cpVarp, cbinp, -1, static_cast<uint32_t>(idx)));
                અચલ ConvBinTarget tgt{cpVarp, static_cast<uint32_t>(idx),
                                        cbinp->binsType().binIsNormal()};
                માટે (Ast_ગાંઠ* sp = cbinp->transp(); sp; sp = sp->nextp())
                    generateSingleTransitionCode(coverpointp, cbinp, exprp, tgt,
                                                 VN_AS(sp, CoverTransSet));
                જો (dynamic && V3Error::error_ગણતરી() == errorsBefore) {
                    metadata.emplace_back(cbinp, static_cast<uint32_t>(idx), nullptr);
                }
                ++idx;
                ચાલુ;
            }
            BinRuns plan;
            જો (binRunsFor(cbinp, exprp, plan)) {
                // Array elements and automatic bins generate as runs, so neither sample() nor
                // the constructor grows with their number of bins.
                જો (plan.unsupported) {  // bin ignored or invalid; reserve no slot
                    m_droppedBins[coverpointp].ધકેલો_back(cbinp->name());
                    ચાલુ;
                }
                CoverpointBins& bins = m_cpBins.at(cpVarp);
                અચલ uint32_t first_કિંમત = bins.total;
                અચલ uint32_t firstDeclared = static_cast<uint32_t>(idx);
                namerStmts.ધકેલો_back(
                    makeNamer(cpVarp, cbinp, plan.ગણતરી, firstDeclared, {}, plan.કિંમતો));
                માટે (BinRun& run : plan.runs) {
                    run.m_declared = static_cast<uint32_t>(idx);
                    bins.runs.ધકેલો_back(std::move(run));
                    અચલ BinRun& stored = bins.runs.back();
                    જો (bins.crossed && cbinp->binsType().binIsNormal()) {
                        અચલ uint32_t first = first_કિંમત + stored.m_declared - firstDeclared;
                        માટે (uint32_t element = 0; element < stored.m_ગણતરી; ++element) {
                            bins.કિંમતો[first + element].runp = &stored;
                            bins.કિંમતો[first + element].element = element;
                        }
                    }
                    જો (!stored.m_empty) {
                        emitRunHit(coverpointp, cbinp, cpVarp, exprp, stored);
                        જો (dynamic && V3Error::error_ગણતરી() == errorsBefore) {
                            runMetadata.ધકેલો_back(&stored);
                        }
                    }
                    idx += stored.m_ગણતરી;
                }
                ચાલુ;
            }
            જો (cbinp->isArray()) {  // value array of a real coverpoint: b[0]..b[N-1]
                // Only integral coverpoints have runtime value metadata (m_runtimePoints)
                UASSERT_OBJ(!dynamic, cbinp, "Runtime value metadata for a real coverpoint");
                તાર્કિક unsupported = ખોટું;
                std::vector<Ast_ગાંઠ_પદાવલિ*> કિંમતો = extract_Array_કિંમતો(cbinp, exprp, unsupported);
                જો (unsupported) {  // bin ignored (COVERIGN emitted); reserve no slot
                    m_droppedBins[coverpointp].ધકેલો_back(cbinp->name());
                    ચાલુ;
                }
                namerStmts.ધકેલો_back(makeNamer(cpVarp, cbinp, static_cast<int64_t>(કિંમતો.size()),
                                               static_cast<uint32_t>(idx), કિંમતો));
                માટે (Ast_ગાંઠ_પદાવલિ* valuep : કિંમતો) {
                    // The cross selections of this covergroup still read the value.
                    m_detached_કિંમતો.ધકેલો_back(valuep);
                    emitConvHitIf(coverpointp, cbinp, cpVarp,
                                  cnum(cbinp->fileline(), static_cast<uint32_t>(idx)),
                                  build_કિંમત_Condition(cbinp, exprp, valuep));
                    ++idx;
                }
            } નહીંતર {
                namerStmts.ધકેલો_back(makeNamer(cpVarp, cbinp, -1, static_cast<uint32_t>(idx)));
                // buildBinCondition is null for 'ignore_bins = default' (no ranges); the bin
                // still gets a reserved slot (recorded, never incremented).
                જો (Ast_ગાંઠ_પદાવલિ* અચલ condp = buildBinCondition(cbinp, exprp))
                    emitConvHitIf(coverpointp, cbinp, cpVarp,
                                  cnum(cbinp->fileline(), static_cast<uint32_t>(idx)), condp);
                જો (dynamic && V3Error::error_ગણતરી() == errorsBefore) {
                    metadata.emplace_back(cbinp, static_cast<uint32_t>(idx), nullptr);
                }
                ++idx;
            }
        }
        // A cross selecting an ignored bins declaration selects no bins
        માટે (અચલ std::string& name : m_droppedBins[coverpointp]) {
            m_cpBins.at(cpVarp).spans.emplace(name, BinSpan{});
        }
        // A value a constructed bin holds is no default bin's; only sampling tells which do
        AstVar* sizedMatchedp = nullptr;
        જો (!defaultBins.empty()
            && std::any_of(sizedBins.begin(), sizedBins.end(), [](અચલ AstCoverBin* binp) {
                   પાછા binp->binsType().binIsNormal();
               })) {
            sizedMatchedp = નવું AstVar{fl, VVarType::BLOCKTEMP,
                                       "__VcpSized_" + sanitizeGeneratedName(coverpointp->name()),
                                       coverpointp->findBitDType()};
            sizedMatchedp->funcLocal(સાચું);
            m_sampleFuncp->addStmtsp(sizedMatchedp);
            addSampleStmt(નવું AstAssign{fl, નવું AstVarRef{fl, sizedMatchedp, VAccess::WRITE},
                                        નવું AstConst{fl, AstConst::BitFalse{}}});
        }
        માટે (uint32_t sized = 0; sized < sizedBins.size(); ++sized) {
            emitSizedSample(coverpointp, sizedBins[sized], cpVarp, exprp, sized, sizedMatchedp);
        }
        માટે (AstCoverBin* અચલ defBinp : defaultBins) {
            File_લીટી* અચલ dfl = defBinp->fileline();
            namerStmts.ધકેલો_back(makeNamer(cpVarp, defBinp, -1, static_cast<uint32_t>(idx)));
            Ast_ગાંઠ_પદાવલિ* condp = buildDefaultCondition(coverpointp, exprp, dfl);
            જો (sizedMatchedp) {
                condp = નવું AstLogAnd{
                    dfl, નવું AstNot{dfl, નવું AstVarRef{dfl, sizedMatchedp, VAccess::READ}}, condp};
            }
            emitConvHitIf(coverpointp, defBinp, cpVarp, cnum(dfl, static_cast<uint32_t>(idx)),
                          condp);
            ++idx;
        }
        જો (idx > std::numeric_limits<uint32_t>::max()) {
            // The runtime indexes bins with 32 bits; stop before generating a model
            coverpointp->v3warn(E_UNSUPPORTED, "Unsupported: coverpoint with more than "
                                                   << std::numeric_limits<uint32_t>::max()
                                                   << " bins");
        }

        // Transition coverpoints track the previous sampled value; update it once at the end of
        // this coverpoint's sample() contribution (the prev var was created on demand by the
        // transition matching above).
        જો (coverpointHasTransition(coverpointp)) {
            AstVar* અચલ prevVarp = VN_AS(coverpointp->user1p(), Var);
            addSampleStmt(
                નવું AstAssign{coverpointp->fileline(),
                              નવું AstVarRef{prevVarp->fileline(), prevVarp, VAccess::WRITE},
                              exprp->clone_વૃક્ષ(ખોટું)});
        }

        // Constructor: init (allocates), namers, then registration (under --coverage).
        // Under --protect-ids the hierarchy and page string reach the coverage database
        // verbatim, so obfuscate them like line/toggle points (per-word hierarchy, whole-
        // unit page).  No-ops when --protect-ids is off.
        અચલ તાર્કિક prot = v3Global.opt.protectIds();
        અચલ std::string hier
            = VIdProtect::protect_શબ્દો_If(m_covergroupName + "." + coverpointp->name(), prot);
        m_constructorp->addStmtsp(
            itemCall(fl, cpVarp, VCMethod::COVERGROUP_INIT,
                     {ctext(fl, quoted(hier)), cnum(fl, static_cast<uint32_t>(at_Least_કિંમત)),
                      cnum(fl, static_cast<uint32_t>(idx))})
                ->makeStmt());
        માટે (Ast_ગાંઠ_Stmt* અચલ ns : namerStmts) m_constructorp->addStmtsp(ns);
        જો (dynamic) {
            m_constructorp->addStmtsp(
                itemCall(fl, cpVarp, VCMethod::COVERGROUP_કિંમત_TYPE,
                         {cnum(fl, exprp->width()), cnum(fl, exprp->isSigned())})
                    ->makeStmt());
            કિંમત_Lists lists;
            માટે (અચલ auto& entry : metadata) {
                collect_કિંમત_Metadata(lists, exprp, std::get<0>(entry), std::get<1>(entry),
                                     std::get<2>(entry));
            }
            માટે (અચલ BinRun* અચલ runp : runMetadata) collectRunMetadata(lists, exprp, *runp);
            emit_કિંમત_List(fl, cpVarp, VCMethod::COVERGROUP_કિંમત_RANGES, lists.m_ranges);
            emit_કિંમત_List(fl, cpVarp, VCMethod::COVERGROUP_કિંમત_RUNS, lists.m_runs);
            emit_કિંમત_List(fl, cpVarp, VCMethod::COVERGROUP_કિંમત_PATTERNS, lists.m_patterns);
            emit_કિંમત_List(fl, cpVarp, VCMethod::COVERGROUP_કિંમત_TRANSITIONS, lists.m_transitions);
            માટે (AstCoverBin* અચલ binp : sizedBins) {
                generateConstructedBins(coverpointp, binp, cpVarp, exprp);
            }
            m_constructorp->addStmtsp(
                itemCall(fl, cpVarp, VCMethod::COVERGROUP_કિંમત_FINALIZE)->makeStmt());
        }
        જો (v3Global.opt.coverage()) {
            અચલ std::string page
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
    શૂન્ય generate_Multi_કિંમત_Transition_Code(AstCoverpoint* coverpointp, AstCoverBin* binp,
                                          Ast_ગાંઠ_પદાવલિ* exprp, અચલ ConvBinTarget& tgt,
                                          અચલ std::vector<AstCoverTransItem*>& વસ્તુઓ) {
        UINFO(4, "    Generating multi-value transition state machine for: " << binp->name());
        UINFO(4, "      Sequence length: " << વસ્તુઓ.size() << " items");

        // Create state position variable
        AstVar* અચલ stateVarp = createSequenceStateVar(coverpointp, binp);

        // Build case statement with N cases (one for each state 0 to N-1)
        // State 0: Not started, looking for first item
        // State 1 to N-1: In progress, looking for next item

        AstCase* અચલ casep
            = નવું AstCase{binp->fileline(), VCaseType::CT_CASE,
                          નવું AstVarRef{stateVarp->fileline(), stateVarp, VAccess::READ}, nullptr};

        // Generate each case item in the switch statement
        માટે (size_t state = 0; state < વસ્તુઓ.size(); ++state) {
            AstCaseItem* caseItemp = generateTransitionStateCase(coverpointp, binp, exprp, tgt,
                                                                 stateVarp, વસ્તુઓ, state);
            casep->addItemsp(caseItemp);
        }

        // Add default case (reset to state 0) to prevent CASEINCOMPLETE warnings,
        // since the state variable is wider than the number of valid states.
        AstCaseItem* અચલ defaultItemp = નવું AstCaseItem{
            binp->fileline(), nullptr,
            નવું AstAssign{binp->fileline(),
                          નવું AstVarRef{binp->fileline(), stateVarp, VAccess::WRITE},
                          નવું AstConst{binp->fileline(), AstConst::Widthed_કિંમત{}, 8, 0}}};
        casep->addItemsp(defaultItemp);

        addSampleStmt(casep);
        UINFO(4, "      Successfully added multi-value transition state machine");
    }

    // Generate code for a single state in the transition state machine
    // Returns the case item for this state
    AstCaseItem* generateTransitionStateCase(AstCoverpoint* coverpointp, AstCoverBin* binp,
                                             Ast_ગાંઠ_પદાવલિ* exprp, અચલ ConvBinTarget& tgt,
                                             AstVar* stateVarp,
                                             અચલ std::vector<AstCoverTransItem*>& વસ્તુઓ,
                                             size_t state) {
        File_લીટી* અચલ fl = binp->fileline();

        // Build condition for current value matching expected item at this state
        Ast_ગાંઠ_પદાવલિ* અચલ matchCondp = buildTransitionItemCondition(વસ્તુઓ[state], exprp);

        Ast_ગાંઠ_Stmt* matchActionp = nullptr;

        જો (state == વસ્તુઓ.size() - 1) {
            // Last state: sequence complete!  Record the hit in the runtime VlCoverpoint.
            matchActionp = makeRuntimeBinHit(fl, tgt);

            // For illegal_bins, add error message
            જો (binp->binsType() == VCoverBinsType::BINS_ILLEGAL) {
                અચલ string err_સંદેશ = "Illegal transition bin " + binp->prettyNameQ()
                                      + " hit in coverpoint " + coverpointp->prettyNameQ();
                matchActionp = matchActionp->addNext(makeIllegalBinAction(fl, err_સંદેશ));
            }

            // Reset state to 0
            matchActionp = matchActionp->addNext(
                નવું AstAssign{fl, નવું AstVarRef{fl, stateVarp, VAccess::WRITE},
                              નવું AstConst{fl, AstConst::Widthed_કિંમત{}, 8, 0}});
        } નહીંતર {
            // Intermediate state: advance to next state
            matchActionp = નવું AstAssign{
                fl, નવું AstVarRef{fl, stateVarp, VAccess::WRITE},
                નવું AstConst{fl, AstConst::Widthed_કિંમત{}, 8, static_cast<uint32_t>(state + 1)}};
        }

        // Build restart logic: check if current value matches first item
        // If so, restart sequence from state 1 (even if we're in middle of sequence)
        Ast_ગાંઠ_Stmt* noMatchActionp = nullptr;
        જો (state > 0) {
            // Check if current value matches first item (restart condition)
            Ast_ગાંઠ_પદાવલિ* અચલ restartCondp = buildTransitionItemCondition(વસ્તુઓ[0], exprp);

            UASSERT_OBJ(restartCondp, વસ્તુઓ[0],
                        "buildTransitionItemCondition returned nullptr for restart");

            // Restart to state 1
            Ast_ગાંઠ_Stmt* restartActionp
                = નવું AstAssign{fl, નવું AstVarRef{fl, stateVarp, VAccess::WRITE},
                                નવું AstConst{fl, AstConst::Widthed_કિંમત{}, 8, 1}};

            // Reset to state 0 (else branch)
            Ast_ગાંઠ_Stmt* resetActionp
                = નવું AstAssign{fl, નવું AstVarRef{fl, stateVarp, VAccess::WRITE},
                                નવું AstConst{fl, AstConst::Widthed_કિંમત{}, 8, 0}};

            noMatchActionp = નવું AstIf{fl, restartCondp, restartActionp, resetActionp};
        }
        // For state 0, no action needed if no match (stay in state 0)

        // Combine into if-else
        Ast_ગાંઠ_Stmt* અચલ stmtp = નવું AstIf{fl, matchCondp, matchActionp, noMatchActionp};

        // Create case item for this state value
        AstCaseItem* અચલ caseItemp = નવું AstCaseItem{
            fl, નવું AstConst{fl, AstConst::Widthed_કિંમત{}, 8, static_cast<uint32_t>(state)},
            stmtp};

        પાછા caseItemp;
    }

    // Create: $error(msg); $stop;  Used when an illegal bin is hit.
    Ast_ગાંઠ_Stmt* makeIllegalBinAction(File_લીટી* fl, અચલ string& err_સંદેશ) {
        AstDisplay* અચલ errorp
            = નવું AstDisplay{fl, VDisplayType::DT_ERROR, err_સંદેશ, nullptr, nullptr};
        errorp->fmtp()->timeunit(m_covergroupp->timeunit());
        static_cast<Ast_ગાંઠ*>(errorp)->addNext(નવું AstStop{fl, સાચું});
        પાછા errorp;
    }

    // Preserve the coverpoint's width and signedness after V3Width.
    સ્થિર AstConst* new_કિંમત_Const(File_લીટી* fl, અચલ V3_સંખ્યા& કિંમત, અચલ Ast_ગાંઠ_પદાવલિ* exprp) {
        V3_સંખ્યા narrowed{fl, exprp->width(), 0};
        narrowed.opAssign(કિંમત);
        AstConst* અચલ constp = નવું AstConst{fl, narrowed};
        constp->dtypeFrom(exprp);
        પાછા constp;
    }

    // A real copy of a real or integral range bound, for comparing with a real coverpoint.
    સ્થિર AstConst* newRealConst(AstConst* constp) {
        જો (constp->અંક().isDouble()) પાછા constp->clone_વૃક્ષ(ખોટું);
        V3_સંખ્યા real{&constp->અંક(), 64};
        real.opIToRD(constp->અંક(), constp->isSigned());
        પાછા નવું AstConst{constp->fileline(), real};
    }

    // Clone a constant node, widening to targetWidth if needed (zero-extend).
    // Used to ensure comparisons use matching widths after V3Width has run.
    સ્થિર AstConst* widenConst(File_લીટી* fl, AstConst* constp, પૂર્ણાંક targetWidth) {
        જો (constp->width() == targetWidth) પાછા constp->clone_વૃક્ષ(ખોટું);
        V3_સંખ્યા અંક{fl, targetWidth, 0};
        અંક.opAssign(constp->અંક());
        પાછા નવું AstConst{fl, અંક};
    }

    // Build a range condition: minp <= exprp <= maxp.
    // Uses signed comparisons if exprp is signed; omits trivially-true domain bounds.
    // All arguments are non-owning; clones exprp/minp/maxp as needed.
    Ast_ગાંઠ_પદાવલિ* makeRangeCondition(File_લીટી* fl, Ast_ગાંઠ_પદાવલિ* exprp, Ast_ગાંઠ_પદાવલિ* minp,
                                    Ast_ગાંઠ_પદાવલિ* maxp) {
        અચલ પૂર્ણાંક પદાવલિ_Width = exprp->widthMin();
        AstConst* અચલ minConstp = VN_AS(minp, Const);
        AstConst* અચલ maxConstp = VN_AS(maxp, Const);
        જો (exprp->isDouble()) {
            // A real coverpoint has no finite domain bounds to omit.
            પાછા નવું AstAnd{fl,
                              નવું AstGteD{fl, exprp->clone_વૃક્ષ(ખોટું), newRealConst(minConstp)},
                              નવું AstLteD{fl, exprp->clone_વૃક્ષ(ખોટું), newRealConst(maxConstp)}};
        }
        // Widen constants to match expression width so post-V3Width nodes use correct macros
        AstConst* અચલ minWidep = widenConst(fl, minConstp, પદાવલિ_Width);
        AstConst* અચલ maxWidep = widenConst(fl, maxConstp, પદાવલિ_Width);
        V3_સંખ્યા minimum{fl, પદાવલિ_Width, 0};
        V3_સંખ્યા maximum{fl, પદાવલિ_Width, 0};
        maximum.setAllBits1();
        જો (exprp->isSigned()) {
            minimum.setBit(પદાવલિ_Width - 1, 1);
            maximum.setBit(પદાવલિ_Width - 1, 0);
        }
        Ast_ગાંઠ_પદાવલિ* lowerp = nullptr;
        Ast_ગાંઠ_પદાવલિ* upperp = nullptr;
        જો (minWidep->અંક().isCaseEq(minimum)) {
            VL_DO_DANGLING(ધકેલો_Deletep(minWidep), minWidep);
        } નહીંતર {
            lowerp = exprp->isSigned() ? static_cast<Ast_ગાંઠ_પદાવલિ*>(
                                             નવું AstGteS{fl, exprp->clone_વૃક્ષ(ખોટું), minWidep})
                                       : static_cast<Ast_ગાંઠ_પદાવલિ*>(
                                             નવું AstGte{fl, exprp->clone_વૃક્ષ(ખોટું), minWidep});
        }
        જો (maxWidep->અંક().isCaseEq(maximum)) {
            VL_DO_DANGLING(ધકેલો_Deletep(maxWidep), maxWidep);
        } નહીંતર {
            upperp = exprp->isSigned() ? static_cast<Ast_ગાંઠ_પદાવલિ*>(
                                             નવું AstLteS{fl, exprp->clone_વૃક્ષ(ખોટું), maxWidep})
                                       : static_cast<Ast_ગાંઠ_પદાવલિ*>(
                                             નવું AstLte{fl, exprp->clone_વૃક્ષ(ખોટું), maxWidep});
        }
        જો (lowerp && upperp) પાછા નવું AstAnd{fl, lowerp, upperp};
        જો (lowerp) પાછા lowerp;
        જો (upperp) પાછા upperp;
        પાછા નવું AstConst{fl, AstConst::BitTrue{}};
    }

    // Build a one-sided comparison for an open-ended bin range whose other bound is '$'.
    // '$' denotes the coverpoint domain extreme, so {[lo:$]} == (expr >= lo) and
    // {[$:hi]} == (expr <= hi).
    Ast_ગાંઠ_પદાવલિ* makeOpenRangeCondition(File_લીટી* fl, Ast_ગાંઠ_પદાવલિ* exprp, AstConst* boundp,
                                        તાર્કિક isLowerBound) {
        AstConst* અચલ widep = widenConst(fl, boundp, exprp->widthMin());
        જો (isLowerBound) {
            જો (exprp->isSigned()) પાછા નવું AstGteS{fl, exprp->clone_વૃક્ષ(ખોટું), widep};
            પાછા નવું AstGte{fl, exprp->clone_વૃક્ષ(ખોટું), widep};
        }
        જો (exprp->isSigned()) પાછા નવું AstLteS{fl, exprp->clone_વૃક્ષ(ખોટું), widep};
        પાછા નવું AstLte{fl, exprp->clone_વૃક્ષ(ખોટું), widep};
    }

    // Build condition for a single transition item.
    // Returns expression that checks if exprp matches the item's value/range list.
    // Overload for when the expression is a variable read -- creates and manages the VarRef
    // internally, so callers don't need to construct a temporary node.
    Ast_ગાંઠ_પદાવલિ* buildTransitionItemCondition(AstCoverTransItem* itemp, AstVar* varp) {
        Ast_ગાંઠ_પદાવલિ* varRefp = નવું AstVarRef{varp->fileline(), varp, VAccess::READ};
        Ast_ગાંઠ_પદાવલિ* અચલ condp = buildTransitionItemCondition(itemp, varRefp);
        VL_DO_DANGLING(ધકેલો_Deletep(varRefp), varRefp);
        પાછા condp;
    }

    // Non-owning: exprp is cloned internally; caller retains ownership of exprp.
    Ast_ગાંઠ_પદાવલિ* buildTransitionItemCondition(AstCoverTransItem* itemp, Ast_ગાંઠ_પદાવલિ* exprp) {
        Ast_ગાંઠ_પદાવલિ* condp = nullptr;

        માટે (Ast_ગાંઠ* valp = itemp->valuesp(); valp; valp = valp->nextp()) {
            Ast_ગાંઠ_પદાવલિ* singleCondp = nullptr;
            valp = V3Const::constifyEdit(valp);
            AstConst* અચલ constp = VN_CAST(valp, Const);
            જો (!constp) {
                valp->v3error("Non-constant expression in transition bin; "
                              "values must be constants (IEEE 1800-2023 19.5)");
                પાછા નવું AstConst{valp->fileline(), AstConst::BitFalseErroring{}};
            }
            singleCondp
                = નવું AstEq{constp->fileline(), exprp->clone_વૃક્ષ(ખોટું), constp->clone_વૃક્ષ(ખોટું)};
            જો (condp) {
                condp = નવું AstOr{itemp->fileline(), condp, singleCondp};
            } નહીંતર {
                condp = singleCondp;
            }
        }

        પાછા condp;
    }

    // Generate code for a single transition sequence (used by both regular and array bins)
    શૂન્ય generateSingleTransitionCode(AstCoverpoint* coverpointp, AstCoverBin* binp,
                                      Ast_ગાંઠ_પદાવલિ* exprp, અચલ ConvBinTarget& tgt,
                                      AstCoverTransSet* transSetp) {
        UINFO(4, "      Generating code for transition sequence");

        // Get or create previous value variable
        AstVar* અચલ prevVarp = create_Prev_કિંમત_Var(coverpointp, exprp);

        UASSERT_OBJ(
            transSetp, binp,
            "Transition bin has no transition set (transp() was checked before calling this)");

        // Get transition items (the sequence: item1 => item2 => item3)
        std::vector<AstCoverTransItem*> વસ્તુઓ;
        માટે (Ast_ગાંઠ* itemp = transSetp->itemsp(); itemp; itemp = itemp->nextp())
            વસ્તુઓ.ધકેલો_back(VN_AS(itemp, CoverTransItem));

        જો (વસ્તુઓ.empty()) {
            binp->v3error("Transition set without items");
            પાછા;
        }

        જો (વસ્તુઓ.size() == 1) {
            // Single item transition not valid (need at least 2 values for =>)
            binp->v3error("Transition requires at least two values");
            પાછા;
        } નહીંતર જો (વસ્તુઓ.size() == 2) {
            // Simple two-value transition: (val1 => val2)
            // Use optimized direct comparison (no state machine needed)
            Ast_ગાંઠ_પદાવલિ* અચલ cond1p = buildTransitionItemCondition(વસ્તુઓ[0], prevVarp);
            Ast_ગાંઠ_પદાવલિ* અચલ cond2p = buildTransitionItemCondition(વસ્તુઓ[1], exprp);

            // Combine: prev matches val1 AND current matches val2
            Ast_ગાંઠ_પદાવલિ* fullCondp = નવું AstAnd{binp->fileline(), cond1p, cond2p};

            addConvTransHitIf(coverpointp, binp, tgt, fullCondp);

            UINFO(4, "        Successfully added 2-value transition if statement");
        } નહીંતર {
            // Multi-value sequence (a => b => c => ...)
            // Use state machine to track position in sequence
            generate_Multi_કિંમત_Transition_Code(coverpointp, binp, exprp, tgt, વસ્તુઓ);
        }
    }

    // Append a "{ VlCoverpoint* __Vcx_cps[] = {cp0, cp1, ...}; <call> }" statement.  The brace
    // and the temporary array stay literal text -- a CMethodHard is one call, not a block --
    // but callp itself carries the member, method and '->'.  Construction only: init() copies
    // the array into the cross, so sample() reads it from there and needs no array at all.
    AstCStmt* makeCrossCpsCall(File_લીટી* fl, અચલ std::vector<AstVar*>& cpVars,
                               AstCMethodHard* callp) {
        AstCStmt* અચલ cs = નવું AstCStmt{fl};
        cs->add("{ VlCoverpoint* __Vcx_cps[] = {");
        માટે (size_t d = 0; d < cpVars.size(); ++d) {
            જો (d != 0) cs->add(", ");
            cs->add(memberRef(fl, cpVars[d]));
        }
        cs->add("}; ");
        cs->add(callp);
        cs->add("; }");
        પાછા cs;
    }

    // Assign the per-bin flags individually: one-bit SV results have integer C++ storage types,
    // which may narrow in a bool initializer list but convert implicitly in assignments.
    AstCStmt* makeCrossIffsCall(File_લીટી* fl, અચલ std::vector<AstCoverCrossBin*>& bins,
                                AstCMethodHard* callp) {
        AstCStmt* અચલ cs = નવું AstCStmt{fl};
        cs->add("{ bool __Vcx_iffs[" + cvtToStr(bins.size()) + "]; ");
        માટે (size_t i = 0; i < bins.size(); ++i) {
            અચલ AstCoverCrossBin* અચલ binp = bins[i];
            cs->add("__Vcx_iffs[" + cvtToStr(i) + "] = ");
            cs->add(binp->iffp() ? binp->iffp()->clone_વૃક્ષ(ખોટું)
                                 : નવું AstConst{fl, AstConst::BitTrue{}});
            cs->add("; ");
        }
        cs->add(callp);
        cs->add("; }");
        પાછા cs;
    }

    using CrossSelection = std::vector<uint64_t>;
    સંરચના ResolvedCrossBin final {
        AstCoverCrossBin* binp;
        CrossSelection selection;
    };
    સંરચના CrossLayout final {
        uint32_t tuples = 0;
        uint32_t autoBins = 0;
        uint64_t bin_શબ્દો = 0;
        તાર્કિક valid = સાચું;
        std::vector<ResolvedCrossBin> bins;
    };
    સંરચના CrossSelectionContext final {
        AstCoverCross* crossp;  // Cross whose tuple space is being selected
        અચલ std::vector<AstVar*>& cpVars;  // Feeding coverpoints in dimension order
        અચલ std::map<std::string, uint32_t>& dimensions;  // Coverpoint name -> dimension
        uint32_t tuples;  // Size of the Cartesian product
        std::vector<uint32_t> strides;  // Flat-index stride per dimension
        તાર્કિક valid = સાચું;  // False if this explicit bin cannot be implemented
    };
    સંરચના CrossBinsofTarget final {
        અચલ CoverpointBins* m_binsp = nullptr;  // Declared bins; null after a reported error
        uint32_t m_dimension = 0;  // Coverpoint index within the cross
        uint32_t m_first = 0;  // First declared normal bin in the selected span
        uint32_t m_ગણતરી = 0;  // Number of declared normal bins in the selected span
        uint32_t m_declaredFirst = 0;  // First runtime bin index of the selected span
        uint32_t m_declaredEnd = UINT32_MAX;  // One past the last runtime bin index
        int32_t m_sized = -1;  // Index of the sized array holding the selected bins; or -1
    };
    સંરચના Cross_કિંમત_Range final {
        V3_સંખ્યા lo;  // Inclusive lower bound, sign-extended to the comparison width
        V3_સંખ્યા hi;  // Inclusive upper bound
        V3_સંખ્યા pattern;  // Allowed bit values; all X for an ordinary interval
        તાર્કિક wildcard = ખોટું;  // A wildcard singleton rather than an exact four-state value

        Cross_કિંમત_Range(Ast_ગાંઠ* nodep, પૂર્ણાંક width)
            : lo{nodep, width}
            , hi{nodep, width}
            , pattern{nodep, width} {
            pattern.setAllBitsX();
        }
    };

    સ્થિર std::vector<Ast_ગાંઠ*> cross_Bin_કિંમતો(અચલ Cross_Bin_કિંમતો& bin) {
        જો (bin.valuep) પાછા {bin.valuep};
        std::vector<Ast_ગાંઠ*> કિંમતો;
        જો (bin.binp->transp()) {
            // IEEE 1800-2023 19.6.1: binsof uses the last value of each transition.
            માટે (Ast_ગાંઠ* setp = bin.binp->transp(); setp; setp = setp->nextp()) {
                AstCoverTransItem* lastp = VN_AS(setp, CoverTransSet)->itemsp();
                જ્યારે (lastp->nextp()) lastp = VN_AS(lastp->nextp(), CoverTransItem);
                માટે (Ast_ગાંઠ* valuep = lastp->valuesp(); valuep; valuep = valuep->nextp()) {
                    કિંમતો.ધકેલો_back(valuep);
                }
            }
        } નહીંતર {
            માટે (Ast_ગાંઠ* valuep = bin.binp->rangesp(); valuep; valuep = valuep->nextp()) {
                કિંમતો.ધકેલો_back(valuep);
            }
        }
        પાછા કિંમતો;
    }

    // The values of the element-th bin of a run, sign-extended to 'width'
    સ્થિર Cross_કિંમત_Range runBinRange(Ast_ગાંઠ* nodep, અચલ BinRun& run, uint32_t element,
                                       પૂર્ણાંક width) {
        અચલ પૂર્ણાંક runw = run.m_lo.width();
        V3_સંખ્યા offset{nodep, runw};
        offset.opMul(run.m_stride, V3_સંખ્યા{nodep, runw, element});
        V3_સંખ્યા lo{nodep, runw};
        lo.opAdd(run.m_lo, offset);
        V3_સંખ્યા hi = run.m_hi;
        જો (element + 1 < run.m_ગણતરી) {
            offset.opSub(run.m_stride, V3_સંખ્યા{nodep, runw, 1});
            hi.opAdd(lo, offset);
        }
        Cross_કિંમત_Range range{nodep, width};
        range.lo.opExtendS(lo, runw);
        range.hi.opExtendS(hi, runw);
        પાછા range;
    }

    સ્થિર પૂર્ણાંક crossRangeWidth(Ast_ગાંઠ* nodep) {
        જો (અચલ AstInsideRange* અચલ rangep = VN_CAST(nodep, InsideRange)) {
            પાછા std::max(rangep->lhsp()->width(), rangep->rhsp()->width());
        }
        પાછા nodep->width();
    }

    સ્થિર તાર્કિક cross_કિંમત_Less(અચલ V3_સંખ્યા& lhs, અચલ V3_સંખ્યા& rhs) {
        V3_સંખ્યા result{&lhs};
        પાછા !result.opLtS(lhs, rhs).isEqZero();
    }

    // Round a real bound inward to the nearest coverpoint value (IEEE 1800-2023 19.5.7).  Sets
    // 'empty' if the domain has no value on the bound's side.
    સ્થિર શૂન્ય crossRealBound(અચલ AstConst* constp, Ast_ગાંઠ_પદાવલિ* exprp, તાર્કિક upper,
                               અચલ Cross_કિંમત_Range& domain, V3_સંખ્યા& result, તાર્કિક& empty) {
        અચલ દ્વિગુણ કિંમત = constp->અંક().toDouble();
        અચલ દ્વિગુણ bound = upper ? std::floor(કિંમત) : std::ceil(કિંમત);
        // Powers of two are exact, so the integral bound compares exactly with the domain edges.
        અચલ દ્વિગુણ limit
            = std::ldexp(1.0, exprp->isSigned() ? exprp->width() - 1 : exprp->width());
        અચલ દ્વિગુણ minimum = exprp->isSigned() ? -limit : 0.0;
        જો (std::isnan(bound) || (upper ? bound < minimum : bound >= limit)) {
            empty = સાચું;
            result = domain.lo;
        } નહીંતર જો (bound < minimum) {
            result = domain.lo;
        } નહીંતર જો (bound >= limit) {
            result = domain.hi;
        } નહીંતર {
            V3_સંખ્યા real{&result, 64};
            real.setDouble(bound);
            result.opRToIRoundS(real);
        }
    }

    સ્થિર તાર્કિક crossRangeBound(Ast_ગાંઠ* nodep, Ast_ગાંઠ_પદાવલિ* exprp, તાર્કિક upper, તાર્કિક bin_કિંમત,
                                અચલ Cross_કિંમત_Range& domain, V3_સંખ્યા& result, તાર્કિક& empty) {
        જો (VN_IS(nodep, Unbounded)) {
            V3_સંખ્યા limit{nodep, exprp->width()};
            જો (upper) limit.setAllBits1();
            જો (exprp->isSigned()) {
                limit.setBit(exprp->width() - 1, !upper);
                result.opExtendS(limit, limit.width());
            } નહીંતર {
                result.opAssign(limit);
            }
            પાછા સાચું;
        }
        અચલ AstConst* અચલ constp = VN_CAST(nodep, Const);
        જો (!constp) પાછા ખોટું;
        જો (constp->અંક().isDouble()) {
            crossRealBound(constp, exprp, upper, domain, result, empty);
            પાછા સાચું;
        }
        જો (constp->અંક().isString()) પાછા ખોટું;
        જો (bin_કિંમત && exprp->isSigned() && constp->width() <= exprp->width()) {
            // Bin bit patterns use the coverpoint's effective type (IEEE 1800-2023 19.5.7).
            // Wider values and intersect filters retain their values for domain clipping.
            V3_સંખ્યા કિંમત{nodep, exprp->width()};
            જો (constp->isSigned()) {
                કિંમત.opExtendS(constp->અંક(), constp->width());
            } નહીંતર {
                કિંમત.opAssign(constp->અંક());
            }
            result.opExtendS(કિંમત, કિંમત.width());
        } નહીંતર જો (constp->isSigned()) {
            result.opExtendS(constp->અંક(), constp->width());
        } નહીંતર {
            result.opAssign(constp->અંક());
        }
        પાછા સાચું;
    }

    સ્થિર Cross_કિંમત_Range cross_કિંમત_Domain(Ast_ગાંઠ* nodep, પૂર્ણાંક કિંમત_Width, તાર્કિક isSigned,
                                            પૂર્ણાંક width) {
        Cross_કિંમત_Range domain{nodep, width};
        V3_સંખ્યા lo{nodep, કિંમત_Width};
        V3_સંખ્યા hi{nodep, કિંમત_Width};
        hi.setAllBits1();
        જો (isSigned) {
            lo.setBit(કિંમત_Width - 1, 1);
            hi.setBit(કિંમત_Width - 1, 0);
            domain.lo.opExtendS(lo, કિંમત_Width);
            domain.hi.opExtendS(hi, કિંમત_Width);
        } નહીંતર {
            domain.lo.opAssign(lo);
            domain.hi.opAssign(hi);
        }
        પાછા domain;
    }

    સ્થિર શૂન્ય intersectCrossRange(Cross_કિંમત_Range& range, અચલ Cross_કિંમત_Range& other) {
        જો (cross_કિંમત_Less(range.lo, other.lo)) range.lo = other.lo;
        જો (cross_કિંમત_Less(other.hi, range.hi)) range.hi = other.hi;
    }

    સ્થિર તાર્કિક cross_કિંમત_Range(Ast_ગાંઠ* nodep, Ast_ગાંઠ_પદાવલિ* exprp, તાર્કિક bin_કિંમત, તાર્કિક wildcard,
                                અચલ Cross_કિંમત_Range& domain, Cross_કિંમત_Range& range) {
        તાર્કિક empty = ખોટું;
        અચલ AstConst* અચલ constp = VN_CAST(nodep, Const);
        જો (અચલ AstInsideRange* અચલ rangep = VN_CAST(nodep, InsideRange)) {
            જો (!crossRangeBound(rangep->lhsp(), exprp, ખોટું, bin_કિંમત, domain, range.lo, empty)
                || !crossRangeBound(rangep->rhsp(), exprp, સાચું, bin_કિંમત, domain, range.hi, empty)
                || range.lo.isFourState() || range.hi.isFourState()) {
                પાછા ખોટું;
            }
        } નહીંતર જો (constp && constp->અંક().isDouble()) {
            // A real value participates only if integral; it has no wildcard bits.
            crossRealBound(constp, exprp, ખોટું, domain, range.lo, empty);
            crossRealBound(constp, exprp, સાચું, domain, range.hi, empty);
        } નહીંતર {
            જો (!crossRangeBound(nodep, exprp, ખોટું, bin_કિંમત, domain, range.lo, empty)) {
                પાછા ખોટું;
            }
            જો (bin_કિંમત && exprp->isSigned() && constp && !constp->isSigned()
                && constp->width() > exprp->width()) {
                તાર્કિક representable = સાચું;
                માટે (પૂર્ણાંક bit = exprp->width(); bit < constp->width(); ++bit) {
                    representable &= !constp->અંક().bitIs1(bit);
                }
                જો (representable) {
                    V3_સંખ્યા કિંમત{nodep, exprp->width()};
                    કિંમત.opAssign(constp->અંક());
                    range.lo.opExtendS(કિંમત, કિંમત.width());
                }
            }
            range.hi = range.lo;
            range.wildcard = wildcard;
            જો (wildcard) {
                range.pattern = range.lo;
                range.lo = domain.lo;
                range.hi = domain.hi;
                જો (constp && constp->isSigned()) {
                    // Replicated X sign bits are correlated, not independent wildcards.
                    // The source domain preserves expansion-before-casting (19.5.7).
                    intersectCrossRange(
                        range, cross_કિંમત_Domain(nodep, constp->width(), સાચું, domain.lo.width()));
                }
            }
        }
        જો (empty) {
            range.lo = domain.hi;
            range.hi = domain.lo;
            પાછા સાચું;
        }
        જો (!range.lo.isFourState()) intersectCrossRange(range, domain);
        જો (range.wildcard && exprp->isSigned()) {
            અચલ પૂર્ણાંક sign = exprp->width() - 1;
            જો (constp && !constp->isSigned() && constp->width() > exprp->width()) {
                // Unsigned equality preserves every target bit pattern if discarded bits are zero.
                માટે (પૂર્ણાંક bit = exprp->width(); bit < constp->width(); ++bit) {
                    જો (constp->અંક().bitIs1(bit)) {
                        range.lo = domain.hi;
                        range.hi = domain.lo;
                        પાછા સાચું;
                    }
                }
                માટે (પૂર્ણાંક bit = exprp->width(); bit < range.pattern.width(); ++bit) {
                    જો (range.pattern.bitIsXZ(sign))
                        range.pattern.setBit(bit, 'x');
                    નહીંતર
                        range.pattern.setBit(bit, range.pattern.bitIs1(sign));
                }
                પાછા સાચું;
            }
            // Discarded high bits constrain the target sign, rather than becoming don't-cares.
            માટે (પૂર્ણાંક bit = exprp->width(); bit < range.pattern.width(); ++bit) {
                જો (range.pattern.bitIsXZ(bit)) ચાલુ;
                અચલ તાર્કિક કિંમત = range.pattern.bitIs1(bit);
                જો (!range.pattern.bitIsXZ(sign) && range.pattern.bitIs1(sign) != કિંમત) {
                    range.lo = domain.hi;
                    range.hi = domain.lo;
                    તોડો;
                }
                range.pattern.setBit(sign, કિંમત);
            }
        }
        પાછા સાચું;
    }

    ગણના CrossRangeState : uint8_t {
        CROSS_INSIDE_BOUNDS = 0,  // Prefix is strictly inside the interval
        CROSS_AT_LOWER = 1,
        CROSS_AT_UPPER = 2,
        CROSS_AT_BOUNDS = CROSS_AT_LOWER | CROSS_AT_UPPER,
        CROSS_NO_MATCH = 4  // Prefix cannot match the interval/pattern
    };

    સ્થિર CrossRangeState crossRangeStep(અચલ Cross_કિંમત_Range& range, CrossRangeState state,
                                          પૂર્ણાંક bit, પૂર્ણાંક કિંમત) {
        જો (state == CROSS_NO_MATCH) પાછા CROSS_NO_MATCH;
        // Flipping the sign bit makes signed order lexicographic.
        અચલ તાર્કિક sign = bit == range.pattern.width() - 1;
        જો (!range.pattern.bitIsXZ(bit) && કિંમત != (range.pattern.bitIs1(bit) ^ sign)) {
            પાછા CROSS_NO_MATCH;
        }
        અચલ પૂર્ણાંક નીચલું = range.lo.bitIs1(bit) ^ sign;
        અચલ પૂર્ણાંક ઉપલું = range.hi.bitIs1(bit) ^ sign;
        જો (((state & CROSS_AT_LOWER) && કિંમત < નીચલું)
            || ((state & CROSS_AT_UPPER) && કિંમત > ઉપલું)) {
            પાછા CROSS_NO_MATCH;
        }
        પાછા static_cast<CrossRangeState>(
            ((state & CROSS_AT_LOWER) && કિંમત == નીચલું ? CROSS_AT_LOWER : CROSS_INSIDE_BOUNDS)
            | ((state & CROSS_AT_UPPER) && કિંમત == ઉપલું ? CROSS_AT_UPPER : CROSS_INSIDE_BOUNDS));
    }

    સ્થિર તાર્કિક crossWildcardIntersects(અચલ Cross_કિંમત_Range& range) {
        unsigned states = 1U << CROSS_AT_BOUNDS;
        માટે (પૂર્ણાંક bit = range.pattern.width() - 1; bit >= 0 && states; --bit) {
            unsigned next = 0;
            માટે (અચલ CrossRangeState state :
                 {CROSS_INSIDE_BOUNDS, CROSS_AT_LOWER, CROSS_AT_UPPER, CROSS_AT_BOUNDS}) {
                જો (!(states & (1U << state))) ચાલુ;
                માટે (પૂર્ણાંક કિંમત = 0; કિંમત < 2; ++કિંમત) {
                    અચલ CrossRangeState equal = crossRangeStep(range, state, bit, કિંમત);
                    જો (equal != CROSS_NO_MATCH) next |= 1U << equal;
                }
            }
            states = next;
        }
        પાછા states != 0;
    }

    // The least value at least 'from' with the non-x bits of 'pattern', in unsigned order; false
    // if none.  A bit scan, which does not enumerate the pattern's x bits.
    સ્થિર તાર્કિક next_Pattern_કિંમત(અચલ V3_સંખ્યા& pattern, અચલ V3_સંખ્યા& from, V3_સંખ્યા& result) {
        result.opAssign(from);
        પૂર્ણાંક carry = -1;  // Lowest x bit above the bit scanned where 'from' has a 0
        માટે (પૂર્ણાંક bit = from.width() - 1; bit >= 0; --bit) {
            જો (pattern.bitIsXZ(bit)) {
                જો (!from.bitIs1(bit)) carry = bit;
                ચાલુ;
            }
            જો (pattern.bitIs1(bit) == from.bitIs1(bit)) ચાલુ;
            જો (from.bitIs1(bit)) {  // Only a greater prefix, an x bit above raised, can match
                જો (carry < 0) પાછા ખોટું;
                bit = carry;
            }
            // Then the least such value: the pattern's bits, with x bits of zero
            result.setBit(bit, 1);
            જ્યારે (--bit >= 0) result.setBit(bit, pattern.bitIs1(bit));
            પાછા સાચું;
        }
        પાછા સાચું;
    }

    // Append to 'runs' the maximal runs of consecutive values of 'range' that its pattern
    // matches, in value order, until there are more than 'limit'.  A match continues through
    // the pattern's trailing x bits only, which the next value's carry leaves.
    સ્થિર શૂન્ય crossRangeRuns(અચલ Cross_કિંમત_Range& range, size_t limit,
                               std::vector<std::pair<V3_સંખ્યા, V3_સંખ્યા>>& runs) {
        // Values order signed, which is the unsigned order of values with the sign bit flipped
        અચલ પૂર્ણાંક sign = range.lo.width() - 1;
        અચલ auto flipped = [sign](V3_સંખ્યા કિંમત) {
            જો (!કિંમત.bitIsXZ(sign)) કિંમત.setBit(sign, !કિંમત.bitIs1(sign));
            પાછા કિંમત;
        };
        અચલ V3_સંખ્યા pattern = flipped(range.pattern);
        અચલ V3_સંખ્યા hi = flipped(range.hi);
        અચલ V3_સંખ્યા one{&hi, hi.width(), 1};
        V3_સંખ્યા trailing{&hi, hi.width()};
        માટે (પૂર્ણાંક bit = 0; bit <= sign && pattern.bitIsXZ(bit); ++bit) trailing.setBit(bit, 1);
        V3_સંખ્યા from = flipped(range.lo);
        V3_સંખ્યા first{&hi, hi.width()};
        V3_સંખ્યા last{&hi, hi.width()};
        V3_સંખ્યા less{&hi};
        જ્યારે (runs.size() <= limit && next_Pattern_કિંમત(pattern, from, first)
               && less.opLt(hi, first).isEqZero()) {
            last.opOr(first, trailing);
            જો (!less.opLt(hi, last).isEqZero()) last = hi;
            runs.emplace_back(flipped(first), flipped(last));
            જો (last.isCaseEq(hi)) તોડો;
            from.opAdd(last, one);
        }
    }

    // True if no coverpoint value participates in a resolved value or range.  A value with x
    // or z bits participates only as a wildcard pattern (IEEE 1800-2023 19.5.7).
    સ્થિર તાર્કિક crossRangeEmpty(અચલ Cross_કિંમત_Range& range) {
        જો (range.lo.isFourState()) પાછા સાચું;
        પાછા cross_કિંમત_Less(range.hi, range.lo)
               || (range.wildcard && !crossWildcardIntersects(range));
    }

    // Comparison width that holds both the coverpoint's and a bin or intersect value's range.
    સ્થિર પૂર્ણાંક resolveWidth(Ast_ગાંઠ* nodep, અચલ Ast_ગાંઠ_પદાવલિ* exprp) {
        પાછા std::max(exprp->width(), crossRangeWidth(nodep)) + 1;
    }

    // Resolve a bin or intersect value to coverpoint values (IEEE 1800-2023 19.5.7), in the
    // width 'range' was created with.  False if the value is not a constant integral or real.
    સ્થિર તાર્કિક resolve_કિંમત(Ast_ગાંઠ* nodep, Ast_ગાંઠ_પદાવલિ* exprp, તાર્કિક bin_કિંમત, તાર્કિક wildcard,
                             Cross_કિંમત_Range& range) {
        અચલ Cross_કિંમત_Range domain
            = cross_કિંમત_Domain(nodep, exprp->width(), exprp->isSigned(), range.lo.width());
        પાછા cross_કિંમત_Range(nodep, exprp, bin_કિંમત, wildcard, domain, range);
    }

    સ્થિર તાર્કિક crossRangesIntersect(અચલ Cross_કિંમત_Range& bin, અચલ Cross_કિંમત_Range& filter) {
        // Values with x or z bits do not participate, even in an identical filter.
        જો (bin.lo.isFourState() || filter.lo.isFourState()) પાછા ખોટું;
        Cross_કિંમત_Range match = bin;
        intersectCrossRange(match, filter);
        પાછા !cross_કિંમત_Less(match.hi, match.lo)
               && (!bin.wildcard || crossWildcardIntersects(match));
    }

    સ્થિર શૂન્ય unsupportedCrossRange(AstCoverBinsof* selectp, તાર્કિક& valid) {
        selectp->v3warn(COVERIGN, "Unsupported: non-constant or non-integral 'intersect' value, "
                                  "or four-state range bound.");
        valid = ખોટું;
    }

    સ્થિર તાર્કિક cross_કિંમત_Matches_Filters(AstCoverBinsof* selectp, Ast_ગાંઠ* valuep,
                                         Ast_ગાંઠ_પદાવલિ* exprp, અચલ AstCoverBin* binp,
                                         અચલ Cross_કિંમત_Range& domain,
                                         અચલ std::vector<Cross_કિંમત_Range>& filters,
                                         તાર્કિક& valid) {
        Cross_કિંમત_Range range{valuep, domain.lo.width()};
        જો (!cross_કિંમત_Range(valuep, exprp, સાચું, binp->isWildcard(), domain, range)) {
            unsupportedCrossRange(selectp, valid);
            પાછા ખોટું;
        }
        માટે (અચલ Cross_કિંમત_Range& filter : filters) {
            જો (crossRangesIntersect(range, filter)) પાછા સાચું;
        }
        પાછા ખોટું;
    }

    std::vector<તાર્કિક> selectCoverpointBins(AstCoverBinsof* selectp, અચલ CoverpointBins& bins,
                                           uint32_t first, uint32_t ગણતરી, તાર્કિક& valid) {
        જો (selectp->rangesp() && !bins.exprp->dtypep()->skipRefp()->isIntegralOrPacked()) {
            unsupportedCrossRange(selectp, valid);
            પાછા {};
        }
        std::vector<તાર્કિક> selected(bins.total, ખોટું);
        std::vector<std::vector<Ast_ગાંઠ*>> કિંમતો;
        પૂર્ણાંક width = bins.exprp->width();
        માટે (Ast_ગાંઠ* rangep = selectp->rangesp(); rangep; rangep = rangep->nextp()) {
            width = std::max(width, crossRangeWidth(rangep));
        }
        જો (selectp->rangesp()) {
            કિંમતો.reserve(ગણતરી);
            માટે (uint32_t i = first; i < first + ગણતરી; ++i) {
                // A run's values are in the coverpoint type, whose width 'width' covers
                કિંમતો.ધકેલો_back(bins.કિંમતો[i].runp ? std::vector<Ast_ગાંઠ*>{}
                                                     : cross_Bin_કિંમતો(bins.કિંમતો[i]));
                માટે (Ast_ગાંઠ* અચલ valuep : કિંમતો.back()) {
                    width = std::max(width, crossRangeWidth(valuep));
                }
            }
        }
        // One extra bit preserves both unsigned maxima and negative signed bounds.
        ++width;
        અચલ Cross_કિંમત_Range domain
            = cross_કિંમત_Domain(selectp, bins.exprp->width(), bins.exprp->isSigned(), width);
        std::vector<Cross_કિંમત_Range> filters;
        માટે (Ast_ગાંઠ* rangep = selectp->rangesp(); rangep; rangep = rangep->nextp()) {
            Cross_કિંમત_Range filter{rangep, width};
            જો (!cross_કિંમત_Range(rangep, bins.exprp, ખોટું, ખોટું, domain, filter)) {
                unsupportedCrossRange(selectp, valid);
                પાછા {};
            }
            filters.ધકેલો_back(std::move(filter));
        }
        માટે (uint32_t i = first; i < first + ગણતરી; ++i) {
            જો (!selectp->rangesp()) {
                selected[i] = સાચું;
                ચાલુ;
            }
            જો (અચલ BinRun* અચલ runp = bins.કિંમતો[i].runp) {
                અચલ Cross_કિંમત_Range range
                    = runBinRange(selectp, *runp, bins.કિંમતો[i].element, width);
                selected[i] = std::any_of(filters.begin(), filters.end(),
                                          [&](અચલ Cross_કિંમત_Range& filter) {
                                              પાછા crossRangesIntersect(range, filter);
                                          });
                ચાલુ;
            }
            માટે (Ast_ગાંઠ* અચલ valuep : કિંમતો[i - first]) {
                selected[i] = cross_કિંમત_Matches_Filters(
                    selectp, valuep, bins.exprp, bins.કિંમતો[i].binp, domain, filters, valid);
                જો (!valid) પાછા {};
                જો (selected[i]) તોડો;
            }
        }
        જો (selectp->isNegated()) {
            માટે (uint32_t i = 0; i < bins.total; ++i) selected[i] = !selected[i];
        }
        પાછા selected;
    }

    // Constructor-time value metadata of one coverpoint, as C++ list entries
    સંરચના કિંમત_Lists final {
        std::vector<std::string> m_ranges;  // Bin, then low and high words
        std::vector<std::string> m_runs;  // First bin, count, then low, span, and high words
        std::vector<std::string> m_patterns;  // Bin, then value, mask, low, and high words
        std::vector<std::string> m_transitions;  // Transition bin
    };

    // Append a value's words, in the coverpoint's width, to a C++ list entry.
    સ્થિર શૂન્ય append_શબ્દો(std::string& text, અચલ V3_સંખ્યા& કિંમત, અચલ Ast_ગાંઠ_પદાવલિ* exprp) {
        V3_સંખ્યા narrowed{&કિંમત, exprp->width()};
        narrowed.opAssign(કિંમત);
        માટે (પૂર્ણાંક word = 0; word < exprp->width_શબ્દો(); ++word) {
            text += ", " + cvtToStr(narrowed.edataWord(word)) + "U";
        }
    }

    શૂન્ય collect_કિંમત_Metadata(કિંમત_Lists& lists, Ast_ગાંઠ_પદાવલિ* exprp, AstCoverBin* binp,
                              uint32_t index, Ast_ગાંઠ_પદાવલિ* valuep) {
        અચલ std::string bin = cvtToStr(index) + "U";
        જો (binp->transp()) lists.m_transitions.ધકેલો_back(bin);
        માટે (Ast_ગાંઠ* અચલ sourcep : cross_Bin_કિંમતો({binp, valuep})) {
            Cross_કિંમત_Range range{sourcep, resolveWidth(sourcep, exprp)};
            જો (!resolve_કિંમત(sourcep, exprp, સાચું, binp->isWildcard(), range)) {
                // Sampling already resolved every state bin value, so only transitions remain.
                sourcep->v3warn(E_UNSUPPORTED, "Unsupported: non-integral value in a transition "
                                               "bin of a coverpoint with exclusions.");
                ચાલુ;
            }
            જો (crossRangeEmpty(range)) ચાલુ;
            std::string entry = bin;
            જો (range.wildcard) {
                V3_સંખ્યા કિંમત{sourcep, exprp->width()};
                V3_સંખ્યા mask{sourcep, exprp->width()};
                mask.opBitsNonXZ(range.pattern);
                કિંમત.opBitsOne(range.pattern);
                append_શબ્દો(entry, કિંમત, exprp);
                append_શબ્દો(entry, mask, exprp);
            }
            append_શબ્દો(entry, range.lo, exprp);
            append_શબ્દો(entry, range.hi, exprp);
            (range.wildcard ? lists.m_patterns : lists.m_ranges).ધકેલો_back(entry);
        }
    }

    // Describe a run with one entry, from which the runtime computes the values of its bins.
    સ્થિર શૂન્ય collectRunMetadata(કિંમત_Lists& lists, Ast_ગાંઠ_પદાવલિ* exprp, અચલ BinRun& run) {
        V3_સંખ્યા span{exprp, run.m_stride.width(), 0};
        span.opSub(run.m_stride, V3_સંખ્યા{exprp, run.m_stride.width(), 1});
        std::string entry = cvtToStr(run.m_declared) + "U, " + cvtToStr(run.m_ગણતરી) + "U";
        append_શબ્દો(entry, run.m_lo, exprp);
        append_શબ્દો(entry, span, exprp);
        append_શબ્દો(entry, run.m_hi, exprp);
        lists.m_runs.ધકેલો_back(entry);
    }

    // Emit one batched metadata list, bounding the size of each call's temporary list.
    શૂન્ય emit_કિંમત_List(File_લીટી* fl, AstVar* cpVarp, VCMethod method,
                       અચલ std::vector<std::string>& entries) {
        માટે (size_t first = 0; first < entries.size(); first += કિંમત_LIST_ENTRIES) {
            અચલ size_t end = std::min(entries.size(), first + કિંમત_LIST_ENTRIES);
            std::string text = "{" + entries[first];
            માટે (size_t i = first + 1; i < end; ++i) text += ", " + entries[i];
            m_constructorp->addStmtsp(
                itemCall(fl, cpVarp, method, {ctext(fl, text + "}")})->makeStmt());
        }
    }

    સ્થિર તાર્કિક checkCrossRef(અચલ AstCoverCrossRef* refp, અચલ AstCoverCross* crossp) {
        જો (refp->name() == crossp->name()) પાછા સાચું;
        refp->v3error("Cross selection "
                      << refp->prettyNameQ() << " may only name its enclosing cross "
                      << crossp->prettyNameQ() << " (IEEE 1800-2023 19.6.1.2).");
        પાછા ખોટું;
    }

    સ્થિર તાર્કિક checkCrossBinName(અચલ AstCoverCrossBin* binp, std::set<std::string>& names) {
        જો (names.emplace(binp->name()).second) પાછા સાચું;
        binp->v3error("Duplicate cross bin " << binp->prettyNameQ()
                                             << " (IEEE 1800-2023 19.6.1).");
        પાછા ખોટું;
    }

    // The span of the implicit automatic bin reported as 'name' ('auto_<i>', see
    // createImplicitAutoBins), found without naming each of its bins
    સ્થિર તાર્કિક implicitAutoBinSpan(અચલ CoverpointBins& bins, અચલ std::string& name,
                                    BinSpan& span) {
        અચલ std::string prefix = "auto_";
        જો (!VString::startsWith(name, prefix)) પાછા ખોટું;
        અચલ std::string digits = name.substr(prefix.size());
        અચલ unsigned long index = std::strtoul(digits.c_str(), nullptr, 10);
        // Only the reported spelling names the bin, not e.g. 'auto_01' or 'auto_x'
        જો (index >= bins.implicitAuto.ગણતરી || digits != cvtToStr(index)) પાછા ખોટું;
        અચલ uint32_t offset = static_cast<uint32_t>(index);
        span = BinSpan{bins.implicitAuto.first + offset, 1, bins.implicitAuto.declared + offset};
        પાછા સાચું;
    }

    CrossBinsofTarget
    resolveBinsofTarget(અચલ AstCoverBinsof* selectp, અચલ AstCoverCross* crossp,
                        અચલ std::vector<AstVar*>& cpVars,
                        અચલ std::map<std::string, uint32_t>& dimensions) અચલ {
        અચલ auto dim = dimensions.find(selectp->pointp()->name());
        જો (dim == dimensions.end()) {
            selectp->v3error("binsof coverpoint "
                             << selectp->pointp()->prettyNameQ() << " is not an item of cross "
                             << crossp->prettyNameQ() << " (IEEE 1800-2023 19.6.1).");
            પાછા {};
        }
        અચલ CoverpointBins& bins = m_cpBins.at(cpVars[dim->second]);
        CrossBinsofTarget target{&bins, dim->second, 0, bins.total};
        જો (!selectp->name().empty()) {
            અચલ auto bin = bins.spans.find(selectp->name());
            BinSpan span{0, 0, 0};
            જો (bin != bins.spans.end()) {
                span = bin->second;
            } નહીંતર જો (!implicitAutoBinSpan(bins, selectp->name(), span)) {
                selectp->v3error("Cannot find bin " << selectp->prettyNameQ() << " in coverpoint "
                                                    << selectp->pointp()->prettyNameQ()
                                                    << " (IEEE 1800-2023 19.6.1).");
                પાછા {};
            }
            target.m_first = span.first;
            target.m_ગણતરી = span.ગણતરી;
            target.m_declaredFirst = span.declared;
            target.m_declaredEnd = span.declared + span.ગણતરી;
            target.m_sized = span.sized;
        }
        પાછા target;
    }

    // The variables a 'with' filter reads, each by its first reference
    using CrossFilterReads = std::map<અચલ AstVar*, AstVarRef*>;

    // Whether a 'with' filter has a count of value tuples, of 'matches' other than '$'
    સ્થિર તાર્કિક counts_કિંમત_Tuples(અચલ AstCoverCrossWith* withp) {
        પાછા withp->countp() && !VN_IS(withp->countp(), Unbounded);
    }

    // Whether the runtime knows the values of the cross items a 'with' filter reads, or to count
    // value tuples, of all items, of integral coverpoints; else warns
    સ્થિર તાર્કિક crossFilterSupported(અચલ AstCoverCrossWith* withp,
                                     અચલ CrossFilterReads& reads) {
        માટે (અચલ AstVar* varp = withp->varsp(); varp; varp = VN_AS(varp->nextp(), Var)) {
            જો (varp->dtypep()->skipRefp()->isIntegralOrPacked()) ચાલુ;
            અચલ auto it = reads.find(varp);
            જો (it != reads.end()) {
                it->second->v3warn(COVERIGN, "Unsupported: cross bin 'with' filter reading the "
                                             "value of non-integral coverpoint "
                                                 << it->second->prettyNameQ());
                પાછા ખોટું;
            }
            જો (counts_કિંમત_Tuples(withp)) {
                withp->countp()->v3warn(COVERIGN, "Unsupported: cross bin 'matches' count of "
                                                  "value tuples of non-integral coverpoint "
                                                      << varp->prettyNameQ());
                પાછા ખોટું;
            }
        }
        પાછા સાચું;
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
    શૂન્ય generateCrossFilter(અચલ AstCoverCrossWith* withp, અચલ CrossFilterReads& reads,
                             અચલ AstCoverCrossBin* binp, અચલ AstCoverCross* crossp,
                             AstVar* cxp, અચલ std::map<string, uint32_t>& dimensions,
                             uint32_t વસ્તુઓ) {
        File_લીટી* અચલ fl = withp->fileline();
        અચલ string prefix
            = m_sizedNames.get(sanitizeGeneratedName(crossp->name() + "__" + binp->name()));
        અચલ તાર્કિક counting = counts_કિંમત_Tuples(withp);
        // The count, 0 for all
        અચલ std::pair<Ast_ગાંઠ_પદાવલિ*, Ast_ગાંઠ_પદાવલિ*> ગણતરી
            = counting ? new_ગણતરી_Args(fl, prefix + "_count", withp->countp())
                       : new_ગણતરી_Args(fl, withp->countp() ? 0 : 1);
        AstLoop* અચલ loopp = નવું AstLoop{fl};
        AstCMethodHard* અચલ nextp = itemCall(fl, cxp, VCMethod::COVERGROUP_WITH_NEXT);
        nextp->dtypeSetBit();
        loopp->addStmtsp(નવું AstLoopTest{fl, loopp, nextp});
        string dims;
        માટે (uint32_t dim = 0; counting && dim < વસ્તુઓ; ++dim) {
            dims += (dims.empty() ? "" : ", ") + cvtToStr(dim) + "U";
        }
        જ્યારે (AstVar* અચલ varp = withp->varsp()) {
            varp->unlinkFrBack();
            જો (!reads.ગણતરી(varp)) {
                VL_DO_DANGLING(ધકેલો_Deletep(varp), varp);
                ચાલુ;
            }
            અચલ auto dimIt = dimensions.find(varp->name());
            UASSERT_OBJ(dimIt != dimensions.end(), varp, "Cross 'with' candidate not of an item");
            અચલ uint32_t dim = dimIt->second;
            જો (!counting) dims += (dims.empty() ? "" : ", ") + cvtToStr(dim) + "U";
            varp->name(prefix + "_" + sanitizeGeneratedName(varp->name()));
            m_constructorp->addStmtsp(varp);
            જો (varp->isWide()) {
                loopp->addStmtsp(itemCall(fl, cxp, VCMethod::COVERGROUP_CROSS_WITH_કિંમત_W,
                                          {cnum(fl, dim), નવું AstVarRef{fl, varp, VAccess::WRITE}})
                                     ->makeStmt());
            } નહીંતર {
                AstCMethodHard* અચલ valuep
                    = itemCall(fl, cxp, VCMethod::COVERGROUP_CROSS_WITH_કિંમત, {cnum(fl, dim)});
                valuep->dtypeSetUInt64();
                Ast_ગાંઠ_DType* અચલ dtypep = varp->findLogicDType(
                    varp->width(), varp->width(),
                    varp->isSigned() ? VSigning::SIGNED : VSigning::UNSIGNED);
                loopp->addStmtsp(નવું AstAssign{fl, નવું AstVarRef{fl, varp, VAccess::WRITE},
                                               resize_કિંમત(valuep, dtypep)});
            }
        }
        loopp->addStmtsp(itemCall(fl, cxp, VCMethod::COVERGROUP_CROSS_WITH_TEST,
                                  {withp->filterp()->unlinkFrBack()})
                             ->makeStmt());
        અચલ તાર્કિક prot = v3Global.opt.protectIds();
        m_constructorp->addStmtsp(
            itemCall(fl, cxp, VCMethod::COVERGROUP_CROSS_WITH_BEGIN,
                     {ctext(fl, "{" + dims + "}"), ગણતરી.first, ગણતરી.second,
                      ctext(fl, quoted(VIdProtect::protect_શબ્દો_If(binp->name(), prot))),
                      ctext(fl, quoted(VIdProtect::protectIf(fl->filename(), prot))),
                      cnum(fl, static_cast<uint32_t>(fl->lineno()))})
                ->makeStmt());
        m_constructorp->addStmtsp(loopp);
        m_constructorp->addStmtsp(
            itemCall(fl, cxp, VCMethod::COVERGROUP_CROSS_WITH_END)->makeStmt());
    }

    તાર્કિક generateRuntimeSelection(Ast_ગાંઠ* nodep, અચલ AstCoverCrossBin* binp,
                                  AstCoverCross* crossp, AstVar* cxp,
                                  અચલ std::vector<AstVar*>& cpVars,
                                  અચલ std::map<string, uint32_t>& dimensions) {
        File_લીટી* અચલ fl = nodep->fileline();
        જો (અચલ AstCoverCrossRef* અચલ refp = VN_CAST(nodep, CoverCrossRef)) {
            જો (!checkCrossRef(refp, crossp)) પાછા ખોટું;
            m_constructorp->addStmtsp(
                itemCall(fl, cxp, VCMethod::COVERGROUP_SELECT_ALL)->makeStmt());
            પાછા સાચું;
        }
        જો (અચલ AstCoverCrossSelect* અચલ opp = VN_CAST(nodep, CoverCrossSelect)) {
            જો (!generateRuntimeSelection(opp->lhsp(), binp, crossp, cxp, cpVars, dimensions)
                || !generateRuntimeSelection(opp->rhsp(), binp, crossp, cxp, cpVars, dimensions)) {
                પાછા ખોટું;
            }
            m_constructorp->addStmtsp(itemCall(fl, cxp,
                                               opp->isOr() ? VCMethod::COVERGROUP_SELECT_OR
                                                           : VCMethod::COVERGROUP_SELECT_AND)
                                          ->makeStmt());
            પાછા સાચું;
        }
        જો (અચલ AstCoverCrossWith* અચલ withp = VN_CAST(nodep, CoverCrossWith)) {
            CrossFilterReads reads;
            withp->filterp()->foreach([&](AstVarRef* refp) { reads.emplace(refp->varp(), refp); });
            // The count of 'matches' is a positive integer, or '$' (IEEE 1800-2023 19.6.1.2)
            જો ((counts_કિંમત_Tuples(withp)
                 && !check_ગણતરી(withp->countp(), "Cross bin 'matches' count", "19.6.1.2"))
                || !crossFilterSupported(withp, reads)
                || !generateRuntimeSelection(withp->selectp(), binp, crossp, cxp, cpVars,
                                             dimensions)) {
                પાછા ખોટું;
            }
            generateCrossFilter(withp, reads, binp, crossp, cxp, dimensions,
                                static_cast<uint32_t>(cpVars.size()));
            પાછા સાચું;
        }
        AstCoverBinsof* અચલ selectp = VN_AS(nodep, CoverBinsof);
        અચલ CrossBinsofTarget target = resolveBinsofTarget(selectp, crossp, cpVars, dimensions);
        જો (!target.m_binsp) પાછા ખોટું;
        અચલ CoverpointBins& bins = *target.m_binsp;
        જો (selectp->rangesp() && !bins.exprp->dtypep()->skipRefp()->isIntegralOrPacked()) {
            તાર્કિક valid = સાચું;
            unsupportedCrossRange(selectp, valid);
            પાછા ખોટું;
        }
        Ast_ગાંઠ_પદાવલિ* firstp;
        Ast_ગાંઠ_પદાવલિ* endp;
        જો (target.m_sized < 0) {
            firstp = cnum(fl, target.m_declaredFirst);
            endp = cnum(fl, target.m_declaredEnd);
        } નહીંતર {  // A sized array, whose bins the coverpoint's construction placed
            AstVar* અચલ cpVarp = cpVars[target.m_dimension];
            અચલ uint32_t sized = static_cast<uint32_t>(target.m_sized);
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
        માટે (Ast_ગાંઠ* rangep = selectp->rangesp(); rangep; rangep = rangep->nextp()) {
            Cross_કિંમત_Range range{rangep, resolveWidth(rangep, bins.exprp)};
            જો (!resolve_કિંમત(rangep, bins.exprp, ખોટું, ખોટું, range)) {
                તાર્કિક valid = સાચું;
                unsupportedCrossRange(selectp, valid);
                પાછા ખોટું;
            }
            જો (crossRangeEmpty(range)) ચાલુ;
            m_constructorp->addStmtsp(itemCall(fl, cxp,
                                               bins.exprp->isWide()
                                                   ? VCMethod::COVERGROUP_SELECT_RANGE_W
                                                   : VCMethod::COVERGROUP_SELECT_RANGE,
                                               {new_કિંમત_Const(fl, range.lo, bins.exprp),
                                                new_કિંમત_Const(fl, range.hi, bins.exprp)})
                                          ->makeStmt());
        }
        m_constructorp->addStmtsp(
            itemCall(fl, cxp, VCMethod::COVERGROUP_SELECT_DIM_END)->makeStmt());
        પાછા સાચું;
    }

    std::vector<AstCoverCrossBin*>
    generateRuntimeCrossBins(AstCoverCross* crossp, AstVar* cxp,
                             અચલ std::vector<AstVar*>& cpVars,
                             અચલ std::map<string, uint32_t>& dimensions) {
        std::vector<AstCoverCrossBin*> bins;
        std::set<string> names;
        માટે (Ast_ગાંઠ* itemp = crossp->binsp(); itemp; itemp = itemp->nextp()) {
            AstCoverCrossBin* અચલ binp = VN_AS(itemp, CoverCrossBin);
            જો (!checkCrossBinName(binp, names)) ચાલુ;
            File_લીટી* અચલ fl = binp->fileline();
            // Generate the selection apart, so that an unsupported one leaves no code, which
            // could leave its state for the next bin
            AstFunc* અચલ selectionp = નવું AstFunc{fl, "", nullptr, nullptr};
            તાર્કિક supported;
            {
                VL_RESTORER(m_constructorp);
                m_constructorp = selectionp;
                supported = generateRuntimeSelection(binp->selectp(), binp, crossp, cxp, cpVars,
                                                     dimensions);
            }
            // A supported selection has code
            જો (supported) m_constructorp->addStmtsp(selectionp->stmtsp()->unlinkFrBackWithNext());
            VL_DO_DANGLING(ધકેલો_Deletep(selectionp), selectionp);
            જો (!supported) ચાલુ;
            અચલ તાર્કિક protect = v3Global.opt.protectIds();
            m_constructorp->addStmtsp(
                itemCall(fl, cxp, VCMethod::COVERGROUP_SELECT_BIN,
                         {ctext(fl, binp->binsType().binSetEnum()),
                          ctext(fl, quoted(VIdProtect::protect_શબ્દો_If(binp->name(), protect))),
                          ctext(fl, quoted(VIdProtect::protectIf(fl->filename(), protect))),
                          cnum(fl, fl->lineno()), cnum(fl, fl->firstColumn()),
                          cnum(fl, static_cast<uint32_t>(bins.size()))})
                    ->makeStmt());
            bins.ધકેલો_back(binp);
        }
        m_constructorp->addStmtsp(
            itemCall(crossp->fileline(), cxp, VCMethod::COVERGROUP_FINALIZE_BINS)->makeStmt());
        પાછા bins;
    }

    સ્થિર શૂન્ય setCrossSelectionRange(CrossSelection& selection, uint64_t first, uint64_t end) {
        જ્યારે (first < end) {
            અચલ unsigned bit = first % 64;
            અચલ unsigned bits = std::min<uint64_t>(64 - bit, end - first);
            selection[VL_BITWORD_Q(first)]
                |= (bits == 64 ? ~uint64_t{0} : (uint64_t{1} << bits) - 1) << bit;
            first += bits;
        }
    }

    CrossSelection crossSelection(Ast_ગાંઠ* nodep, CrossSelectionContext& ctx) {
        જો (અચલ AstCoverCrossRef* અચલ refp = VN_CAST(nodep, CoverCrossRef)) {
            જો (!checkCrossRef(refp, ctx.crossp)) {
                ctx.valid = ખોટું;
                પાછા {};
            }
            CrossSelection result(
                VL_BITWORD_Q(static_cast<uint64_t>(ctx.tuples) + VL_QUADSIZE - 1), 0);
            setCrossSelectionRange(result, 0, ctx.tuples);
            પાછા result;
        }
        જો (AstCoverCrossSelect* અચલ opp = VN_CAST(nodep, CoverCrossSelect)) {
            CrossSelection lhs = crossSelection(opp->lhsp(), ctx);
            અચલ CrossSelection rhs = crossSelection(opp->rhsp(), ctx);
            જો (!ctx.valid) પાછા {};
            માટે (size_t i = 0; i < lhs.size(); ++i) {
                lhs[i] = opp->isOr() ? lhs[i] | rhs[i] : lhs[i] & rhs[i];
            }
            પાછા lhs;
        }
        AstCoverBinsof* અચલ selectp = VN_AS(nodep, CoverBinsof);
        અચલ CrossBinsofTarget target
            = resolveBinsofTarget(selectp, ctx.crossp, ctx.cpVars, ctx.dimensions);
        જો (!target.m_binsp) {
            ctx.valid = ખોટું;
            પાછા {};
        }
        અચલ CoverpointBins& bins = *target.m_binsp;
        // Coverpoints with sized arrays are runtime points, so feed only runtime crosses
        UASSERT_OBJ(target.m_sized < 0, selectp, "Sized bin array selected by a static cross");
        અચલ std::vector<તાર્કિક> selected
            = selectCoverpointBins(selectp, bins, target.m_first, target.m_ગણતરી, ctx.valid);
        જો (!ctx.valid) પાછા {};
        CrossSelection result(VL_BITWORD_Q(static_cast<uint64_t>(ctx.tuples) + VL_QUADSIZE - 1),
                              0);
        અચલ uint64_t stride = ctx.strides[target.m_dimension];
        અચલ uint64_t period = stride * bins.total;
        માટે (uint64_t base = 0; base < ctx.tuples; base += period) {
            માટે (uint32_t i = 0; i < bins.total;) {
                જો (!selected[i]) {
                    ++i;
                    ચાલુ;
                }
                અચલ uint32_t begin = i++;
                જ્યારે (i < bins.total && selected[i]) ++i;
                setCrossSelectionRange(result, base + begin * stride, base + i * stride);
            }
        }
        પાછા result;
    }

    // Size the Cartesian product of the declared Normal bins, with each dimension's flat-index
    // stride.  Live runtime bins only shrink it.  Warns and returns false if it is too large.
    તાર્કિક crossShape(AstCoverCross* crossp, અચલ std::vector<AstVar*>& cpVars,
                    std::vector<uint32_t>& strides, uint32_t& tuples) અચલ {
        uint64_t product = std::any_of(cpVars.begin(), cpVars.end(),
                                       [આ](AstVar* varp) { પાછા !m_cpBins.at(varp).total; })
                               ? 0
                               : 1;
        strides.resize(cpVars.size());
        માટે (size_t d = cpVars.size(); d > 0; --d) {
            strides[d - 1] = static_cast<uint32_t>(product);
            product *= m_cpBins.at(cpVars[d - 1]).total;
            જો (product > UINT32_MAX) {
                crossp->v3warn(COVERIGN,
                               "Unsupported: cross coverage with more than 2^32-1 tuples.");
                પાછા ખોટું;
            }
        }
        tuples = static_cast<uint32_t>(product);
        પાછા સાચું;
    }

    CrossLayout resolveCrossLayout(AstCoverCross* crossp, અચલ std::vector<AstVar*>& cpVars,
                                   અચલ std::map<std::string, uint32_t>& dimensions) {
        CrossLayout layout;
        CrossSelectionContext ctx{crossp, cpVars, dimensions, 0, {}};
        જો (!crossShape(crossp, cpVars, ctx.strides, ctx.tuples)) {
            layout.valid = ખોટું;
            પાછા layout;
        }
        layout.tuples = ctx.tuples;
        CrossSelection occupied;
        CrossSelection excluded;
        std::set<std::string> names;
        માટે (Ast_ગાંઠ* itemp = crossp->binsp(); itemp; itemp = itemp->nextp()) {
            AstCoverCrossBin* અચલ binp = VN_AS(itemp, CoverCrossBin);
            જો (!checkCrossBinName(binp, names)) ચાલુ;
            ctx.valid = સાચું;
            CrossSelection selection = crossSelection(binp->selectp(), ctx);
            જો (!ctx.valid || std::all_of(selection.begin(), selection.end(), [](uint64_t word) {
                    પાછા word == 0;
                })) {
                ચાલુ;
            }
            જો (occupied.empty()) {
                occupied.resize(selection.size(), 0);
                excluded.resize(selection.size(), 0);
            }
            માટે (size_t i = 0; i < selection.size(); ++i) {
                occupied[i] |= selection[i];
                જો (!binp->binsType().binIsNormal()) excluded[i] |= selection[i];
            }
            layout.bins.ધકેલો_back({binp, std::move(selection)});
        }
        જો (!layout.bins.empty()) {
            // IEEE 1800-2023 19.6.2/19.6.3: exclusions also remove tuples from named
            // bins, independently of declaration order and sampling guards.
            માટે (ResolvedCrossBin& resolved : layout.bins) {
                માટે (size_t i = 0; i < resolved.selection.size(); ++i) {
                    જો (resolved.binp->binsType().binIsNormal()) {
                        resolved.selection[i] &= ~excluded[i];
                    }
                    જો (resolved.selection[i]) ++layout.bin_શબ્દો;
                }
            }
            layout.bins.erase(std::remove_if(layout.bins.begin(), layout.bins.end(),
                                             [](અચલ ResolvedCrossBin& resolved) {
                                                 પાછા std::all_of(
                                                     resolved.selection.begin(),
                                                     resolved.selection.end(),
                                                     [](uint64_t word) { પાછા word == 0; });
                                             }),
                              layout.bins.end());
            layout.autoBins = layout.tuples;
            માટે (અચલ uint64_t word : occupied) {
                layout.autoBins -= static_cast<uint32_t>(std::bitset<VL_QUADSIZE>{word}.ગણતરી());
            }
        }
        પાછા layout;
    }

    AstCoverCrossDType* crossDType(File_લીટી* fl, uint32_t dimensions, અચલ CrossLayout& layout,
                                   તાર્કિક dynamic = ખોટું) {
        અચલ uint32_t bins = static_cast<uint32_t>(layout.bins.size());
        અચલ CrossShape shape{dimensions,      layout.tuples,   bins,
                               layout.autoBins, layout.bin_શબ્દો, dynamic};
        AstCoverCrossDType*& typep = m_cxDTypes[shape];
        જો (!typep) {
            typep = નવું AstCoverCrossDType{
                fl, dimensions, layout.tuples, bins, layout.autoBins, layout.bin_શબ્દો, dynamic};
            v3Global.rootp()->typeTablep()->addTypesp(typep);
        }
        પાછા typep;
    }

    std::vector<AstCoverCrossBin*> generateCrossBins(AstCoverCross* crossp, AstVar* cxVarp,
                                                     અચલ CrossLayout& layout) {
        std::vector<AstCoverCrossBin*> bins;
        માટે (અચલ ResolvedCrossBin& resolved : layout.bins) {
            AstCoverCrossBin* અચલ binp = resolved.binp;
            અચલ CrossSelection& selection = resolved.selection;
            File_લીટી* અચલ fl = binp->fileline();
            અચલ તાર્કિક prot = v3Global.opt.protectIds();
            std::string mask = "{";
            માટે (size_t i = 0; i < selection.size(); ++i) {
                જો (i) mask += ", ";
                mask += std::to_string(selection[i]) + "ULL";
            }
            mask += "}";
            m_constructorp->addStmtsp(
                itemCall(fl, cxVarp, VCMethod::COVERGROUP_ADD_BIN,
                         {ctext(fl, binp->binsType().binSetEnum()), ctext(fl, mask),
                          ctext(fl, quoted(VIdProtect::protect_શબ્દો_If(binp->name(), prot))),
                          ctext(fl, quoted(VIdProtect::protectIf(fl->filename(), prot))),
                          cnum(fl, static_cast<uint32_t>(fl->lineno())),
                          cnum(fl, static_cast<uint32_t>(fl->firstColumn()))})
                    ->makeStmt());
            bins.ધકેલો_back(binp);
        }
        જો (!bins.empty()) {
            m_constructorp->addStmtsp(
                itemCall(crossp->fileline(), cxVarp, VCMethod::COVERGROUP_FINALIZE_BINS)
                    ->makeStmt());
        }
        પાછા bins;
    }

    // Route a cross through a VlCoverCross member: emit the member, its constructor init +
    // registration, and the sample() call.  The feeding coverpoints are already generated
    // (their hit lists drive the cross). Each explicit bin adds one configuration call.
    શૂન્ય generateCross(AstCoverCross* crossp) {
        File_લીટી* અચલ fl = crossp->fileline();
        અચલ તાર્કિક dynamic = m_runtimeCrosses.ગણતરી(crossp);
        UINFO(4, "  Generating VlCoverCross member: " << crossp->name());

        જો (Ast_ગાંઠ_પદાવલિ* અચલ iffp = crossp->iffp()) {
            crossp->iffp(
                captureIffToTemp(iffp, "__VcrossIff_" + sanitizeGeneratedName(crossp->name())));
        }

        // Resolve and unlink the coverpoint refs, in dimension order.  Every ref resolves to a
        // known coverpoint (a cross with an unresolvable item was dropped earlier).
        std::vector<AstVar*> cpVars;
        std::map<std::string, uint32_t> dimensions;
        માટે (Ast_ગાંઠ* itemp = crossp->itemsp(); itemp;) {
            Ast_ગાંઠ* અચલ nextp = itemp->nextp();
            AstCoverpointRef* અચલ refp = VN_AS(itemp, CoverpointRef);
            અચલ auto it = m_cpVarMap.find(refp->name());
            UASSERT_OBJ(it != m_cpVarMap.end(), crossp, "Cross references an unknown coverpoint");
            dimensions.emplace(refp->name(), static_cast<uint32_t>(cpVars.size()));
            cpVars.ધકેલો_back(it->second);
            VL_DO_DANGLING(ધકેલો_Deletep(refp->unlinkFrBack()), refp);
            itemp = nextp;
        }
        અચલ પૂર્ણાંક dims = static_cast<પૂર્ણાંક>(cpVars.size());
        CrossLayout layout;
        જો (dynamic) {
            // The runtime sizes the layout from live bins; only check the declared bound here.
            std::vector<uint32_t> strides;
            uint32_t tuples = 0;
            layout.valid = crossShape(crossp, cpVars, strides, tuples);
        } નહીંતર {
            layout = resolveCrossLayout(crossp, cpVars, dimensions);
        }
        જો (!layout.valid) પાછા;

        AstVar* અચલ cxVarp
            = નવું AstVar{fl, VVarType::MEMBER, "__Vcx_" + crossp->name(),
                         crossDType(fl, static_cast<uint32_t>(dims), layout, dynamic)};
        m_covergroupp->addMembersp(cxVarp);
        m_constructorp->addStmtsp(makeItemCreate(fl, cxVarp,
                                                 dynamic ? VCMethod::COVERGROUP_ADD_CROSS_DYN
                                                         : VCMethod::COVERGROUP_ADD_CROSS));
        generateItemWeight(fl, cxVarp, crossp->optionsp());
        પૂર્ણાંક at_Least_કિંમત;
        પૂર્ણાંક autoBinMax;  // Not a cross option
        extractItemOptions(crossp->optionsp(), at_Least_કિંમત, autoBinMax);

        // Constructor: init (after the coverpoints, which generate earlier) then registration.
        // Obfuscate the hierarchy/filename/page under --protect-ids as for coverpoints above.
        અચલ તાર્કિક prot = v3Global.opt.protectIds();
        અચલ std::string hier
            = VIdProtect::protect_શબ્દો_If(m_covergroupName + "." + crossp->name(), prot);
        m_constructorp->addStmtsp(makeCrossCpsCall(
            fl, cpVars,
            itemCall(fl, cxVarp, VCMethod::COVERGROUP_INIT,
                     {ctext(fl, quoted(hier)), cnum(fl, static_cast<uint32_t>(at_Least_કિંમત)),
                      cnum(fl, static_cast<uint32_t>(dims)), ctext(fl, "__Vcx_cps"),
                      ctext(fl, quoted(VIdProtect::protectIf(fl->filename(), prot))),
                      cnum(fl, static_cast<uint32_t>(fl->lineno())),
                      cnum(fl, static_cast<uint32_t>(fl->firstColumn()))})));
        અચલ std::vector<AstCoverCrossBin*> bins
            = dynamic ? generateRuntimeCrossBins(crossp, cxVarp, cpVars, dimensions)
                      : generateCrossBins(crossp, cxVarp, layout);
        જો (v3Global.opt.coverage()) {
            અચલ std::string page
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
        અચલ તાર્કિક hasIffs
            = std::any_of(bins.begin(), bins.end(),
                          [](અચલ AstCoverCrossBin* binp) { પાછા binp->iffp() != nullptr; });
        Ast_ગાંઠ_Stmt* અચલ samplep
            = !hasIffs ? static_cast<Ast_ગાંઠ_Stmt*>(
                             itemCall(fl, cxVarp, VCMethod::COVERGROUP_SAMPLE)->makeStmt())
                       : static_cast<Ast_ગાંઠ_Stmt*>(makeCrossIffsCall(
                             fl, bins,
                             itemCall(fl, cxVarp, VCMethod::COVERGROUP_SAMPLE_IFFS,
                                      {ctext(fl, "__Vcx_iffs")})));
        જો (Ast_ગાંઠ_પદાવલિ* અચલ iffp = crossp->iffp()) {
            m_sampleFuncp->addStmtsp(નવું AstIf{fl, iffp->clone_વૃક્ષ(ખોટું), samplep});
        } નહીંતર {
            m_sampleFuncp->addStmtsp(samplep);
        }
    }

    શૂન્ય generateCrossCode(AstCoverCross* crossp) {
        UINFO(4, "  Generating code for cross: " << crossp->name());

        // Non-standard hierarchical/dotted cross item (e.g. 'cross a.b'): an implicit coverpoint
        // over the referenced expression (carried in refp->exprp()).  The grammar already warned
        // NONSTD; implicit coverpoints are not yet implemented, so generate no sampling code for
        // this cross.  When support is added the implicit coverpoint should be synthesized
        // upstream (V3LinkParse) as a real AstCoverpoint so it flows through the normal coverpoint
        // path - by here coverpoint lowering has already run.
        માટે (Ast_ગાંઠ* itemp = crossp->itemsp(); itemp; itemp = itemp->nextp()) {
            અચલ AstCoverpointRef* અચલ refp = VN_AS(itemp, CoverpointRef);
            જો (refp->exprp()) {
                refp->v3warn(COVERIGN,
                             "Unsupported: cross of hierarchical reference (implicit coverpoint)");
                પાછા;
            }
        }

        // A cross naming a bare variable (implicit coverpoint, which Verilator does not
        // synthesize) is dropped entirely with a COVERIGN warning -- it produces no coverage
        // either way -- but only this cross is dropped; its sibling crosses are still generated
        // and the real coverpoints it referenced remain as independent coverpoints.
        જો (m_droppedCrosses.ગણતરી(crossp)) {
            માટે (Ast_ગાંઠ* itemp = crossp->itemsp(); itemp; itemp = itemp->nextp()) {
                અચલ AstCoverpointRef* અચલ refp = VN_AS(itemp, CoverpointRef);
                જો (m_coverpointMap.find(refp->name()) == m_coverpointMap.end()) {
                    refp->v3warn(COVERIGN, "Unsupported: cross of "
                                               << refp->prettyNameQ()
                                               << " which is not a coverpoint (implicit "
                                                  "coverpoint)");
                    તોડો;
                }
            }
            પાછા;
        }

        // Every cross that isn't dropped routes through a VlCoverCross member.
        generateCross(crossp);
    }

    Ast_ગાંઠ_પદાવલિ* buildBinCondition(AstCoverBin* binp, Ast_ગાંઠ_પદાવલિ* exprp) {
        // Get the range list from the bin
        Ast_ગાંઠ* અચલ rangep = binp->rangesp();
        જો (!rangep) પાછા nullptr;

        // Integral values resolve to the coverpoint's type, as its runtime metadata does
        અચલ તાર્કિક integral = exprp->dtypep()->skipRefp()->isIntegralOrPacked();
        // No value form of a wildcard bin is allowed on a real coverpoint (IEEE 1800-2023 19.5.4)
        જો (binp->isWildcard() && !integral) પાછા wildcardTypeError(binp, exprp);

        // Build condition by OR-ing all ranges together
        Ast_ગાંઠ_પદાવલિ* fullCondp = nullptr;

        માટે (Ast_ગાંઠ* currRangep = rangep; currRangep; currRangep = currRangep->nextp()) {
            Ast_ગાંઠ_પદાવલિ* rangeCondp = nullptr;
            currRangep = V3Const::constifyEdit(currRangep);

            જો (AstInsideRange* irp = VN_CAST(currRangep, InsideRange)) {
                Ast_ગાંઠ_પદાવલિ* અચલ minExprp = irp->lhsp();
                Ast_ગાંઠ_પદાવલિ* અચલ maxExprp = irp->rhsp();
                AstConst* અચલ minConstp = VN_CAST(minExprp, Const);
                AstConst* અચલ maxConstp = VN_CAST(maxExprp, Const);
                અચલ તાર્કિક loUnbounded = VN_IS(minExprp, Unbounded);
                અચલ તાર્કિક hiUnbounded = VN_IS(maxExprp, Unbounded);
                જો (loUnbounded || hiUnbounded) {
                    // Open-ended range: '$' is the coverpoint domain min/max, so the
                    // range reduces to a single inequality (e.g. {[10:$]} -> expr >= 10).
                    AstConst* અચલ boundp = hiUnbounded ? minConstp : maxConstp;
                    જો (loUnbounded && hiUnbounded) {
                        rangeCondp = નવું AstConst{irp->fileline(), AstConst::BitTrue{}};
                    } નહીંતર જો (!boundp) {
                        irp->v3error("Non-constant expression in bin range; "
                                     "range bounds must be constants (IEEE 1800-2023 19.5)");
                        જો (fullCondp) VL_DO_DANGLING(ધકેલો_Deletep(fullCondp), fullCondp);
                        પાછા nullptr;
                    } નહીંતર જો (boundp->અંક().isFourState()) {
                        irp->v3error("Four-state (x/z) value in bin range bound; "
                                     "range bounds must be two-state constants");
                        જો (fullCondp) VL_DO_DANGLING(ધકેલો_Deletep(fullCondp), fullCondp);
                        પાછા nullptr;
                    } નહીંતર જો (integral) {
                        rangeCondp = build_કિંમત_Condition(binp, exprp, irp);
                    } નહીંતર {
                        rangeCondp = makeOpenRangeCondition(irp->fileline(), exprp, boundp,
                                                            /*isLowerBound=*/hiUnbounded);
                    }
                } નહીંતર જો (!minConstp || !maxConstp) {
                    irp->v3error("Non-constant expression in bin range; "
                                 "range bounds must be constants (IEEE 1800-2023 19.5)");
                    જો (fullCondp) VL_DO_DANGLING(ધકેલો_Deletep(fullCondp), fullCondp);
                    પાછા nullptr;
                } નહીંતર જો (minConstp->અંક().isFourState() || maxConstp->અંક().isFourState()) {
                    irp->v3error("Four-state (x/z) value in bin range bound; "
                                 "range bounds must be two-state constants");
                    જો (fullCondp) VL_DO_DANGLING(ધકેલો_Deletep(fullCondp), fullCondp);
                    પાછા nullptr;
                } નહીંતર જો (integral) {
                    rangeCondp = build_કિંમત_Condition(binp, exprp, irp);
                } નહીંતર {
                    rangeCondp = makeRangeCondition(irp->fileline(), exprp, minExprp, maxExprp);
                }
            } નહીંતર જો (AstConst* constp = VN_CAST(currRangep, Const)) {
                rangeCondp = build_કિંમત_Condition(binp, exprp, constp);
            } નહીંતર {
                currRangep->v3error("Non-constant expression in bin range; values must be "
                                    "constants (IEEE 1800-2023 19.5)");
                જો (fullCondp) VL_DO_DANGLING(ધકેલો_Deletep(fullCondp), fullCondp);
                પાછા nullptr;
            }

            UASSERT_OBJ(rangeCondp, binp, "rangeCondp is null after building range condition");
            fullCondp
                = fullCondp ? નવું AstOr{binp->fileline(), fullCondp, rangeCondp} : rangeCondp;
        }

        પાછા fullCondp;
    }

    // Wildcard bits have no meaning for a non-integral coverpoint.
    સ્થિર Ast_ગાંઠ_પદાવલિ* wildcardTypeError(AstCoverBin* binp, Ast_ગાંઠ_પદાવલિ* exprp) {
        અચલ Ast_ગાંઠ_DType* અચલ dtypep = exprp->dtypep()->skipRefp();
        exprp->v3error("Cannot use a wildcard bin on a coverpoint of type "
                       << dtypep->prettyDTypeNameQ() << " (IEEE 1800-2023 19.5.4).\n"
                       << exprp->warnContextPrimary() << '\n'
                       << binp->warnOther() << "... Location of wildcard bin\n"
                       << binp->warnContextSecondary());
        પાછા નવું AstConst{binp->fileline(), AstConst::BitFalse{}};
    }

    // Match one bin value, range, or wildcard pattern.  Integral coverpoints first resolve the
    // value to their type (IEEE 1800-2023 19.5.7).  Non-owning: clones what it uses.
    Ast_ગાંઠ_પદાવલિ* build_કિંમત_Condition(AstCoverBin* binp, Ast_ગાંઠ_પદાવલિ* exprp, Ast_ગાંઠ* valuep) {
        File_લીટી* અચલ fl = valuep->fileline();
        જો (!exprp->dtypep()->skipRefp()->isIntegralOrPacked()) {
            પાછા નવું AstEq{fl, exprp->clone_વૃક્ષ(ખોટું),
                             VN_AS(valuep, ગાંઠ_પદાવલિ)->clone_વૃક્ષ(ખોટું)};
        }
        Cross_કિંમત_Range range{valuep, resolveWidth(valuep, exprp)};
        જો (!resolve_કિંમત(valuep, exprp, સાચું, binp->isWildcard(), range)) {
            valuep->v3warn(E_UNSUPPORTED,
                           "Unsupported: non-integral value in a coverage bin of an "
                           "integral coverpoint.");
            પાછા નવું AstConst{fl, AstConst::BitFalse{}};
        }
        જો (crossRangeEmpty(range)) પાછા નવું AstConst{fl, AstConst::BitFalse{}};
        AstConst* અચલ lop = new_કિંમત_Const(fl, range.lo, exprp);
        AstConst* અચલ hip = new_કિંમત_Const(fl, range.hi, exprp);
        Ast_ગાંઠ_પદાવલિ* condp = nullptr;
        જો (lop->અંક().isCaseEq(hip->અંક())) {
            condp = નવું AstEq{fl, exprp->clone_વૃક્ષ(ખોટું), lop};
        } નહીંતર {
            condp = makeRangeCondition(fl, exprp, lop, hip);
            VL_DO_DANGLING(ધકેલો_Deletep(lop), lop);
        }
        VL_DO_DANGLING(ધકેલો_Deletep(hip), hip);
        જો (!range.wildcard) પાછા condp;
        // Match the pattern's value bits within the source-value bounds.
        V3_સંખ્યા mask{valuep, exprp->width()};
        V3_સંખ્યા કિંમત{valuep, exprp->width()};
        mask.opBitsNonXZ(range.pattern);
        કિંમત.opBitsOne(range.pattern);
        AstConst* અચલ maskConstp = new_કિંમત_Const(fl, mask, exprp);
        AstConst* અચલ કિંમત_Constp = new_કિંમત_Const(fl, કિંમત, exprp);
        Ast_ગાંઠ_પદાવલિ* અચલ પદાવલિ_Masked = નવું AstAnd{fl, exprp->clone_વૃક્ષ(ખોટું), maskConstp};
        Ast_ગાંઠ_પદાવલિ* અચલ કિંમત_Masked = નવું AstAnd{fl, કિંમત_Constp, maskConstp->clone_વૃક્ષ(ખોટું)};
        પાછા નવું AstLogAnd{fl, condp, નવું AstEq{fl, પદાવલિ_Masked, કિંમત_Masked}};
    }

    શૂન્ય generateCoverageComputationCode() {
        UINFO(4, "  Generating coverage computation code");

        // Invalidate cache: addMembersp() calls in generateCoverpointCode/generateCrossCode
        // have added new members since the last scan, so clear before re-querying.
        m_memberMap.clear();

        // get_inst_coverage(): the average of the coverpoints and crosses, weighted by their
        // option.weight (IEEE 1800-2023 19.11).  The instance node holds their runtimes.  With
        // the instances merged, get_coverage() instead, unless option.get_inst_coverage
        // (Table 19-1).
        AstFunc* અચલ getInstCoveragep
            = VN_AS(m_memberMap.findMember(m_covergroupp, "get_inst_coverage"), Func);
        File_લીટી* અચલ instFl = getInstCoveragep->fileline();
        AstCMethodHard* અચલ instCallp = instanceCall(instFl, VCMethod::COVERGROUP_COVERAGE);
        instCallp->dtypeSetDouble();
        Ast_ગાંઠ_પદાવલિ* instValuep = instCallp;
        જો (m_cgMayMerge) {
            Ast_ગાંઠ_પદાવલિ* અચલ mergedp = નવું AstLogAnd{
                instFl, newOptionSel(instFl, optionVar(સાચું), "merge_instances", VAccess::READ),
                નવું AstLogNot{instFl, newOptionSel(instFl, optionVar(ખોટું), "get_inst_coverage",
                                                   VAccess::READ)}};
            instValuep = નવું AstCond{instFl, mergedp, typeCoverageCall(instFl), instCallp};
        }
        getInstCoveragep->addStmtsp(નવું AstAssign{
            instFl, નવું AstVarRef{instFl, VN_AS(getInstCoveragep->fvarp(), Var), VAccess::WRITE},
            instValuep});

        // get_coverage(): the average of the covergroup's instances, weighted by their
        // option.weight, or with type_option.merge_instances, the coverage of the union of their
        // bins (IEEE 1800-2023 19.11.3).  Static, so the registry finds the instances.
        AstFunc* અચલ getCoveragep
            = VN_AS(m_memberMap.findMember(m_covergroupp, "get_coverage"), Func);
        File_લીટી* અચલ typeFl = getCoveragep->fileline();
        getCoveragep->addStmtsp(નવું AstAssign{
            typeFl, નવું AstVarRef{typeFl, VN_AS(getCoveragep->fvarp(), Var), VAccess::WRITE},
            typeCoverageCall(typeFl)});
    }

    // The registry call computing the covergroup's type coverage, per its type options
    AstCMethodHard* typeCoverageCall(File_લીટી* fl) {
        AstCExpr* અચલ registryp = ctext(fl, "vlSymsp->_vm_contextp__->covergroupRegistryp()");
        registryp->dtypeSetVoid();  // Opaque receiver; only ever the 'fromp' of the call below
        AstCMethodHard* અચલ callp
            = નવું AstCMethodHard{fl, registryp, VCMethod::COVERGROUP_TYPE_COVERAGE};
        callp->addPinsp(ctext(fl, quoted(covergroupProtectedName())));
        callp->addPinsp(newOptionSel(fl, optionVar(સાચું), "weight", VAccess::READ));
        callp->addPinsp(newOptionSel(fl, optionVar(સાચું), "merge_instances", VAccess::READ));
        callp->addPinsp(file_લીટી_Debug(m_covergroupp->fileline()));
        callp->usePtr(સાચું);
        callp->dtypeSetDouble();
        પાછા callp;
    }

    // VISITORS
    સ્થિર તાર્કિક isEnclosingInstanceVar(અચલ AstVar* varp) {
        પાછા varp->isClassMember() && !varp->lifetime().isStatic() && !varp->isParam();
    }

    શૂન્ય rewriteThisRef(AstThisRef* refp, AstVar* handleVarp) {
        અચલ AstClassRefDType* અચલ refDTypep
            = VN_CAST(refp->dtypep()->skipRefp(), ClassRefDType);
        UASSERT_OBJ(refDTypep && refDTypep->classp() == m_covergroupp, refp,
                    "Unexpected this reference in embedded covergroup");
        Ast_ગાંઠ_પદાવલિ* અચલ newp = નવું AstVarRef{refp->fileline(), handleVarp, VAccess::READ};
        refp->replaceWith(newp);
        VL_DO_DANGLING(ધકેલો_Deletep(refp), refp);
    }

    શૂન્ય rewriteVarRef(AstVarRef* refp, AstVar* handleVarp) {
        File_લીટી* અચલ fl = refp->fileline();
        // As V3LinkLValue does, the handle of a written member select is marked written
        AstMemberSel* અચલ selp
            = નવું AstMemberSel{fl, નવું AstVarRef{fl, handleVarp, refp->access()}, refp->varp()};
        selp->access(refp->access());
        refp->replaceWith(selp);
        VL_DO_DANGLING(ધકેલો_Deletep(refp), refp);
    }

    શૂન્ય rewriteFuncRef(AstFuncRef* refp, AstVar* handleVarp) {
        File_લીટી* અચલ fl = refp->fileline();
        AstArg* અચલ argsp = refp->argsp() ? refp->argsp()->unlinkFrBackWithNext() : nullptr;
        AstMethodCall* અચલ callp = નવું AstMethodCall{
            fl, નવું AstVarRef{fl, handleVarp, VAccess::READ}, refp->name(), argsp};
        callp->taskp(refp->taskp());
        callp->dtypeFrom(refp);
        refp->replaceWith(callp);
        VL_DO_DANGLING(ધકેલો_Deletep(refp), refp);
    }

    // True if funcp is an instance method of the enclosing class or one of its bases
    તાર્કિક isEnclosingInstanceFunc(અચલ Ast_ગાંઠ_FTask* funcp) અચલ {
        જો (!funcp->classMethod() || funcp->isStatic()) પાછા ખોટું;
        પાછા AstClass::isClassExtendedFrom(m_enclosingClassp, VN_AS(funcp->aboveLoopp(), Class));
    }

    તાર્કિક isEmbeddedCovergroupVar(અચલ AstVar* varp) અચલ {
        જો (!varp || !varp->isClassMember() || varp->isDeclTyped()) પાછા ખોટું;
        અચલ AstClassRefDType* અચલ refp = VN_CAST(varp->dtypep()->skipRefp(), ClassRefDType);
        પાછા refp && refp->classp() == m_covergroupp;
    }

    AstVar* findEmbeddedCovergroupVar() અચલ {
        જો (!m_enclosingClassp) પાછા nullptr;
        માટે (Ast_ગાંઠ* itemp = m_enclosingClassp->membersp(); itemp; itemp = itemp->nextp()) {
            જો (AstVar* અચલ varp = VN_CAST(itemp, Var)) {
                જો (isEmbeddedCovergroupVar(varp)) પાછા varp;
            }
        }
        // V3LinkParse always creates an implicit variable for an embedded covergroup.
        પાછા nullptr;  // LCOV_EXCL_LINE
    }

    std::vector<Ast_ગાંઠ_Assign*> findCovergroupConstructions() {
        std::vector<Ast_ગાંઠ_Assign*> foundps;
        જો (!m_embeddedVarp) પાછા foundps;
        AstFunc* અચલ enclosingNewp
            = VN_CAST(m_memberMap.findMember(m_enclosingClassp, "new"), Func);
        જો (!enclosingNewp) પાછા foundps;
        enclosingNewp->foreach([&](Ast_ગાંઠ_Assign* asgnp) {
            અચલ AstNew* અચલ newp = VN_CAST(asgnp->rhsp(), New);
            અચલ AstVarRef* અચલ lhsRefp = VN_CAST(asgnp->lhsp(), VarRef);
            જો (!newp || !lhsRefp || lhsRefp->varp() != m_embeddedVarp) પાછા;
            અચલ AstClassRefDType* અચલ refp = VN_CAST(newp->dtypep(), ClassRefDType);
            જો (refp && refp->classp() == m_covergroupp) foundps.ધકેલો_back(asgnp);
        });
        પાછા foundps;
    }

    std::set<અચલ AstVar*> enclosingInstanceVars() અચલ {
        std::set<અચલ AstVar*> vars;
        જો (m_enclosingClassp) {
            m_enclosingClassp->foreachMember([&](AstClass* અચલ, AstVar* અચલ varp) {
                જો (isEnclosingInstanceVar(varp)) vars.ઉમેરો(varp);
            });
        }
        પાછા vars;
    }

    તાર્કિક hasEnclosingEventRef(AstCovergroup* cgp) અચલ {
        જો (!m_embeddedVarp || !cgp->eventp()) પાછા ખોટું;
        અચલ std::set<અચલ AstVar*> enclosingVars = enclosingInstanceVars();
        તાર્કિક found = ખોટું;
        cgp->eventp()->foreach([&](AstVarRef* refp) {
            જો (enclosingVars.ગણતરી(refp->varp())) found = સાચું;
        });
        પાછા found;
    }

    સ્થિર તાર્કિક parse_Embedded_Event_પદાવલિ(Ast_ગાંઠ_પદાવલિ* exprp, AstVar*& baseVarp,
                                       AstVar*& memberVarp) {
        જો (AstVarRef* અચલ refp = VN_CAST(exprp, VarRef)) {
            baseVarp = refp->varp();
            memberVarp = nullptr;
            પાછા સાચું;
        }
        AstMemberSel* અચલ selp = VN_CAST(exprp, MemberSel);
        જો (!selp) પાછા ખોટું;
        AstVarRef* અચલ baseRefp = VN_CAST(selp->fromp(), VarRef);
        જો (!baseRefp) પાછા ખોટું;
        baseVarp = baseRefp->varp();
        memberVarp = selp->varp();
        પાછા સાચું;
    }

    તાર્કિક isEventLvalue(Ast_ગાંઠ_પદાવલિ* exprp, અચલ EmbeddedEventTrigger& trigger) અચલ {
        જો (AstSel* અચલ selp = VN_CAST(exprp, Sel)) exprp = selp->fromp();
        AstVar* baseVarp = nullptr;
        AstVar* memberVarp = nullptr;
        જો (!parse_Embedded_Event_પદાવલિ(exprp, baseVarp, memberVarp)) પાછા ખોટું;
        પાછા baseVarp == trigger.baseVarp && memberVarp == trigger.memberVarp;
    }

    Ast_ગાંઠ_પદાવલિ* newEventRead(File_લીટી* fl, અચલ EmbeddedEventTrigger& trigger) અચલ {
        Ast_ગાંઠ_પદાવલિ* અચલ basep = નવું AstVarRef{fl, trigger.baseVarp, VAccess::READ};
        જો (!trigger.memberVarp) પાછા basep;
        AstMemberSel* અચલ selp = નવું AstMemberSel{fl, basep, trigger.memberVarp};
        selp->access(VAccess::READ);
        પાછા selp;
    }

    string eventPrevName(અચલ EmbeddedEventTrigger& trigger, size_t triggerIndex) અચલ {
        string name = "__Vcg_prev_" + m_embeddedVarp->name() + "_" + std::to_string(triggerIndex)
                      + "_" + trigger.baseVarp->name();
        જો (trigger.memberVarp) name += "_" + trigger.memberVarp->name();
        પાછા name;
    }

    Ast_ગાંઠ_પદાવલિ* newEmbeddedVarNonNull(File_લીટી* fl) અચલ {
        પાછા નવું AstNeq{fl, નવું AstVarRef{fl, m_embeddedVarp, VAccess::READ},
                          નવું AstConst{fl, AstConst::Null{}}};
    }

    Ast_ગાંઠ_Stmt* newSampleStmt(File_લીટી* fl) અચલ {
        AstMethodCall* અચલ callp = નવું AstMethodCall{
            fl, નવું AstVarRef{fl, m_embeddedVarp, VAccess::READ}, "sample", nullptr};
        callp->taskp(m_sampleFuncp);
        callp->dtypeSetVoid();
        પાછા callp->makeStmt();
    }

    શૂન્ય installEmbeddedEventFork(Ast_Sen_વૃક્ષ* eventp,
                                  અચલ std::vector<Ast_ગાંઠ_Assign*>& constructps) {
        // IEEE 1800-2023 19.3 samples coverpoints whenever their clocking event occurs. A
        // per-instance event cannot use V3Active's static sensitivity path, so spawn
        // 'fork forever begin @(event); cg.sample(); end join_none' after each construction.
        માટે (Ast_ગાંઠ_Assign* અચલ constructp : constructps) {
            File_લીટી* અચલ fl = constructp->fileline();
            AstLoop* અચલ loopp = નવું AstLoop{fl};
            loopp->addStmtsp(નવું AstEventControl{fl, eventp->clone_વૃક્ષ(ખોટું), nullptr});
            loopp->addStmtsp(નવું AstIf{fl, newEmbeddedVarNonNull(fl), newSampleStmt(fl)});
            AstFork* અચલ forkp = નવું AstFork{fl, VJoinType::JOIN_NONE};
            forkp->immediateStart(સાચું);
            forkp->addForksp(નવું AstBegin{fl, "", loopp, સાચું});
            constructp->addNextHere(forkp);
        }
        VL_DO_DANGLING(ધકેલો_Deletep(eventp), eventp);
    }

    Ast_ગાંઠ_પદાવલિ* newEventReadyCondition(File_લીટી* fl, અચલ EmbeddedEventTrigger& trigger) અચલ {
        Ast_ગાંઠ_પદાવલિ* અચલ curp = newEventRead(fl, trigger);
        Ast_ગાંઠ_પદાવલિ* અચલ prevp = નવું AstVarRef{fl, trigger.prevVarp, VAccess::READ};
        Ast_ગાંઠ_પદાવલિ* edgep = nullptr;
        // IEEE 1800-2023 9.4.2 detects edge-qualified events only on the expression's LSB,
        // while an implicit change event observes the complete expression.
        જો (trigger.edgeType == VEdgeType::ET_POSEDGE) {
            edgep = નવું AstSel{fl, નવું AstAnd{fl, curp, નવું AstNot{fl, prevp}}, 0, 1};
        } નહીંતર જો (trigger.edgeType == VEdgeType::ET_NEGEDGE) {
            edgep = નવું AstSel{fl, નવું AstAnd{fl, નવું AstNot{fl, curp}, prevp}, 0, 1};
        } નહીંતર જો (trigger.edgeType == VEdgeType::ET_BOTHEDGE) {
            edgep = નવું AstSel{fl, નવું AstXor{fl, curp, prevp}, 0, 1};
        } નહીંતર {
            edgep = નવું AstNeq{fl, curp, prevp};
        }
        પાછા નવું AstLogAnd{fl, newEmbeddedVarNonNull(fl), edgep};
    }

    std::vector<EmbeddedEventTrigger> collectEmbeddedEventTriggers(AstCovergroup* cgp) {
        std::vector<EmbeddedEventTrigger> triggers;
        અચલ std::set<અચલ AstVar*> enclosingVars = enclosingInstanceVars();
        માટે (Ast_ગાંઠ* senp = cgp->eventp()->sensesp(); senp; senp = senp->nextp()) {
            AstSenItem* અચલ itemp = VN_AS(senp, SenItem);
            AstVar* baseVarp = nullptr;
            AstVar* memberVarp = nullptr;
            જો (!parse_Embedded_Event_પદાવલિ(itemp->sensp(), baseVarp, memberVarp)
                || !enclosingVars.ગણતરી(baseVarp)) {
                પાછા {};
            }
            triggers.emplace_back(itemp->fileline(), baseVarp, memberVarp, itemp->edgeType());
        }
        પાછા triggers;
    }

    શૂન્ય installEmbeddedEventTriggers(std::vector<EmbeddedEventTrigger>& triggers,
                                      અચલ std::vector<Ast_ગાંઠ_Assign*>& constructps) {
        // Without --timing, approximate a per-instance event by sampling after assignments
        // within the enclosing class. External writes and exact scheduling cannot be observed.
        જો (constructps.empty()) પાછા;
        માટે (size_t triggerIndex = 0; triggerIndex < triggers.size(); ++triggerIndex) {
            EmbeddedEventTrigger& trigger = triggers[triggerIndex];
            std::vector<Ast_ગાંઠ_Assign*> assignps;
            m_enclosingClassp->foreach([&](Ast_ગાંઠ_Assign* asgnp) {
                જો (isEventLvalue(asgnp->lhsp(), trigger)) assignps.ધકેલો_back(asgnp);
            });
            જો (assignps.empty()) {
                trigger.eventFl->v3warn(
                    COVERIGN, "Unsupported: 'covergroup' clocking event signal has no assignment "
                              "within the enclosing class; no coverage sampled. Use --timing for "
                              "full support.");
                ચાલુ;
            }
            Ast_ગાંઠ_DType* અચલ dtypep
                = trigger.memberVarp ? trigger.memberVarp->dtypep() : trigger.baseVarp->dtypep();
            AstVar* અચલ prevVarp = નવું AstVar{trigger.eventFl, VVarType::MEMBER,
                                                eventPrevName(trigger, triggerIndex), dtypep};
            m_enclosingClassp->addMembersp(prevVarp);
            trigger.prevVarp = prevVarp;
            માટે (Ast_ગાંઠ_Assign* અચલ asgnp : assignps) {
                File_લીટી* અચલ fl = asgnp->fileline();
                AstIf* અચલ ifp
                    = નવું AstIf{fl, newEventReadyCondition(fl, trigger), newSampleStmt(fl)};
                ifp->addNextHere(નવું AstAssign{fl,
                                               નવું AstVarRef{fl, trigger.prevVarp, VAccess::WRITE},
                                               newEventRead(fl, trigger)});
                asgnp->addNextHere(ifp);
            }
        }
    }

    શૂન્ય delete_Coverage_વસ્તુઓ() {
        માટે (AstCoverpoint* અચલ cpp : m_coverpoints) {
            VL_DO_DANGLING(ધકેલો_Deletep(cpp->unlinkFrBack()), cpp);
        }
        માટે (AstCoverCross* અચલ crossp : m_coverCrosses) {
            VL_DO_DANGLING(ધકેલો_Deletep(crossp->unlinkFrBack()), crossp);
        }
        // Options not lowered: the covergroup was not processed
        માટે (AstCgOptionAssign* અચલ optp : m_cgOptions) {
            VL_DO_DANGLING(ધકેલો_Deletep(optp->unlinkFrBack()), optp);
        }
        m_cgOptions.clear();
    }

    વર્ગ FormalRefVisitor final : જાહેર VNVisitor {
        અચલ std::map<અચલ AstVar*, AstVar*>& m_replacements;

        શૂન્ય visit(AstVarRef* nodep) override {
            અચલ auto it = m_replacements.find(nodep->varp());
            જો (it == m_replacements.end()) પાછા;
            nodep->varp(it->second);
        }
        શૂન્ય visit(Ast_ગાંઠ* nodep) override { iterateChildren(nodep); }

    જાહેર:
        explicit FormalRefVisitor(અચલ std::map<અચલ AstVar*, AstVar*>& replacements)
            : m_replacements{replacements} {}
        શૂન્ય scan(Ast_ગાંઠ* nodep) { iterate(nodep); }
    };

    શૂન્ય validateCovergroupExpressions() {
        std::set<અચલ AstVar*> sampleMembers;
        std::set<અચલ AstVar*> constructorRefMembers;
        માટે (Ast_ગાંઠ* stmtp = m_constructorp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            અચલ AstVar* અચલ varp = VN_CAST(stmtp, Var);
            જો (!varp || !varp->isIO() || (!varp->isRef() && !varp->isConstRef())) ચાલુ;
            અચલ AstVar* અચલ memberp
                = VN_CAST(m_memberMap.findMember(m_covergroupp, varp->name()), Var);
            UASSERT_OBJ(memberp && memberp->isClassMember(), varp,
                        "Covergroup constructor argument missing persistent member");
            constructorRefMembers.ઉમેરો(varp);
            constructorRefMembers.ઉમેરો(memberp);
        }
        માટે (Ast_ગાંઠ* stmtp = m_sampleFuncp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            અચલ AstVar* અચલ varp = VN_CAST(stmtp, Var);
            જો (!varp || !varp->isIO()) ચાલુ;
            અચલ AstVar* અચલ memberp
                = VN_CAST(m_memberMap.findMember(m_covergroupp, varp->name()), Var);
            UASSERT_OBJ(memberp && memberp->isClassMember(), varp,
                        "Covergroup sample argument missing persistent member");
            sampleMembers.ઉમેરો(memberp);
        }
        Covergroup_પદાવલિ_Valid_Visitor{sampleMembers, constructorRefMembers}.scan(m_constructorp);
    }

    શૂન્ય rebindFormalRefs() {
        std::map<અચલ AstVar*, AstVar*> replacements;
        માટે (Ast_ગાંઠ* stmtp = m_constructorp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            જો (અચલ AstVar* અચલ varp = VN_CAST(stmtp, Var)) {
                જો (!varp->isIO()) ચાલુ;
                AstVar* અચલ memberp
                    = VN_CAST(m_memberMap.findMember(m_covergroupp, varp->name()), Var);
                UASSERT_OBJ(memberp && memberp->isClassMember(), varp,
                            "Covergroup constructor argument missing persistent member");
                replacements.emplace(varp, memberp);
            }
        }
        size_t expectedBindings = 0;
        માટે (અચલ auto& pair : replacements) {
            જો (pair.first->isRef() || pair.first->isConstRef()) ++expectedBindings;
        }
        size_t rewrittenBindings = 0;
        માટે (Ast_ગાંઠ* stmtp = m_constructorp->stmtsp(); stmtp;) {
            Ast_ગાંઠ* અચલ nextp = stmtp->nextp();
            જો (AstAssign* અચલ assignp = VN_CAST(stmtp, Assign)) {
                AstVarRef* અચલ lhsp = VN_CAST(assignp->lhsp(), VarRef);
                AstVarRef* અચલ rhsp = VN_CAST(assignp->rhsp(), VarRef);
                જો (lhsp && rhsp
                    && (lhsp->varp()->declDirection() == VDirection::REF
                        || lhsp->varp()->declDirection() == VDirection::CONSTREF)) {
                    અચલ auto it = replacements.find(rhsp->varp());
                    UASSERT_OBJ(it != replacements.end() && it->second == lhsp->varp(), assignp,
                                "Unexpected covergroup reference binding assignment");
                    AstCExpr* અચલ bindp = નવું AstCExpr{assignp->fileline(), ""};
                    bindp->add(lhsp->unlinkFrBack());
                    bindp->add(" = &");
                    bindp->add(rhsp->unlinkFrBack());
                    assignp->replaceWith(bindp->makeStmt());
                    VL_DO_DANGLING(ધકેલો_Deletep(assignp), assignp);
                    ++rewrittenBindings;
                }
            }
            stmtp = nextp;
        }
        UASSERT_OBJ(rewrittenBindings == expectedBindings, m_constructorp,
                    "Covergroup reference argument missing binding");
        FormalRefVisitor visitor{replacements};
        માટે (AstCoverpoint* અચલ cpp : m_coverpoints) visitor.scan(cpp);
        માટે (AstCoverCross* અચલ crossp : m_coverCrosses) visitor.scan(crossp);
    }

    AstVarRef* installEnclosingBackPointer(અચલ std::vector<Ast_ગાંઠ_Assign*>& constructps) {
        // Simple-case support for embedded covergroups (IEEE 1800-2023 19.4) whose
        // coverpoints reference members of the enclosing class ("Class members can be used
        // in coverpoint expressions").  The covergroup is lowered into a sibling class with
        // no implicit handle to the enclosing object, so such references would emit
        // uncompilable C++.  Add an explicit back-pointer member to the enclosing instance,
        // route member references through it, and pass it into the constructor so
        // coverage initialization can read enclosing members. Returns an invalid
        // reference if an outer class member cannot be reached; otherwise returns an empty result.
        જો (!m_enclosingClassp) પાછા nullptr;  // Offending refs require an enclosing class

        AstVarRef* invalidp = nullptr;
        Ast_ગાંઠ* offenderp = nullptr;
        std::set<અચલ AstVar*> ownVars;
        માટે (Ast_ગાંઠ* itemp = m_covergroupp->membersp(); itemp; itemp = itemp->nextp()) {
            જો (અચલ AstVar* અચલ varp = VN_CAST(itemp, Var)) ownVars.ઉમેરો(varp);
        }
        અચલ std::set<અચલ AstVar*> enclosingVars = enclosingInstanceVars();
        std::vector<AstVarRef*> refsToRewrite;
        std::vector<AstThisRef*> thisRefsToRewrite;
        std::vector<AstFuncRef*> funcRefsToRewrite;
        અચલ auto scan = [&](Ast_ગાંઠ* rootp) {
            rootp->foreach([&](AstVarRef* refp) {
                જો (invalidp) પાછા;
                અચલ AstVar* અચલ varp = refp->varp();
                જો (!isEnclosingInstanceVar(varp) || ownVars.ગણતરી(varp)) પાછા;
                જો (!enclosingVars.ગણતરી(varp)) {
                    invalidp = refp;
                    પાછા;
                }
                refsToRewrite.ધકેલો_back(refp);
                જો (!offenderp) offenderp = refp;
            });
            જો (invalidp) પાછા;
            rootp->foreach([&](AstThisRef* refp) {
                અચલ AstClassRefDType* અચલ refDTypep
                    = VN_CAST(refp->dtypep()->skipRefp(), ClassRefDType);
                જો (refDTypep && refDTypep->classp() == m_covergroupp) {
                    thisRefsToRewrite.ધકેલો_back(refp);
                    જો (!offenderp) offenderp = refp;
                }
            });
            rootp->foreach([&](AstFuncRef* refp) {
                જો (!isEnclosingInstanceFunc(refp->taskp())) પાછા;
                funcRefsToRewrite.ધકેલો_back(refp);
                જો (!offenderp) offenderp = refp;
            });
        };
        માટે (AstCoverpoint* અચલ cpp : m_coverpoints) scan(cpp);
        માટે (AstCoverCross* અચલ crossp : m_coverCrosses) scan(crossp);
        માટે (AstCgOptionAssign* અચલ optp : m_cgOptions) scan(optp);
        જો (invalidp || !offenderp) પાછા invalidp;

        UASSERT_OBJ(m_embeddedVarp, m_covergroupp, "Embedded covergroup variable not found");
        // Commit: add the back-pointer member, rewrite the references, initialize the handle.
        File_લીટી* અચલ fl = m_covergroupp->fileline();
        AstClassRefDType* અચલ enclDTypep = નવું AstClassRefDType{fl, m_enclosingClassp, nullptr};
        enclDTypep->rawPointer(સાચું);
        v3Global.rootp()->typeTablep()->addTypesp(enclDTypep);
        AstVar* અચલ handleVarp
            = નવું AstVar{fl, VVarType::MEMBER, "__Vcg_enclosingp", enclDTypep};
        m_covergroupp->addMembersp(handleVarp);
        AstVar* અચલ argumentp = નવું AstVar{fl, VVarType::BLOCKTEMP, "__Vcg_parentp", enclDTypep};
        argumentp->direction(VDirection::આગત);
        argumentp->declDirection(VDirection::આગત);
        argumentp->funcLocal(સાચું);
        argumentp->noReset(સાચું);
        argumentp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
        m_constructorp->addStmtsp(argumentp);
        m_constructorp->prependStmtsp(નવું AstAssign{fl, memberRef(fl, handleVarp, VAccess::WRITE),
                                                    નવું AstVarRef{fl, argumentp, VAccess::READ}});

        // Route each enclosing-member reference through the back-pointer: 'm' -> 'h.m'.
        માટે (AstVarRef* અચલ refp : refsToRewrite) { rewriteVarRef(refp, handleVarp); }
        માટે (AstThisRef* અચલ refp : thisRefsToRewrite) { rewriteThisRef(refp, handleVarp); }
        માટે (AstFuncRef* અચલ refp : funcRefsToRewrite) { rewriteFuncRef(refp, handleVarp); }

        // Append a named hidden argument to preserve positional and defaulted user arguments.
        માટે (Ast_ગાંઠ_Assign* અચલ constructp : constructps) {
            File_લીટી* અચલ cfl = constructp->fileline();
            AstCExpr* અચલ thisp = નવું AstCExpr{cfl, "this"};
            thisp->dtypep(enclDTypep);
            VN_AS(constructp->rhsp(), New)->addArgsp(નવું AstArg{cfl, argumentp->name(), thisp});
        }
        પાછા nullptr;
    }

    શૂન્ય visit(AstClass* nodep) override {
        UINFO(9, "Visiting class: " << nodep->name() << " isCovergroup=" << nodep->isCovergroup());
        જો (nodep->isCovergroup()) {
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
            Ast_Sen_વૃક્ષ* embeddedEventForkp = nullptr;

            // Extract and store the clocking event from AstCovergroup node
            // The parser creates this node to preserve the event information
            તાર્કિક hasUnsupportedEvent = ખોટું;
            માટે (Ast_ગાંઠ* itemp = nodep->membersp(); itemp;) {
                Ast_ગાંઠ* અચલ nextp = itemp->nextp();
                જો (AstCovergroup* અચલ cgp = VN_CAST(itemp, Covergroup)) {
                    // Store the event in the global map for V3Active to retrieve later
                    // V3LinkParse only creates this sentinel AstCovergroup node when a clocking
                    // event exists, so cgp->eventp() is always non-null here.
                    UASSERT_OBJ(cgp->eventp(), cgp,
                                "Sentinel AstCovergroup in class must have non-null eventp");
                    જો (hasEnclosingEventRef(cgp)) {
                        UASSERT_OBJ(m_embeddedVarp, cgp,
                                    "Embedded covergroup event has no instance variable");
                        જો (v3Global.opt.timing().isSetTrue()) {
                            embeddedEventForkp = cgp->eventp()->unlinkFrBack();
                        } નહીંતર {
                            embeddedEventTriggers = collectEmbeddedEventTriggers(cgp);
                            જો (embeddedEventTriggers.empty()) {
                                cgp->v3warn(COVERIGN,
                                            "Unsupported: 'covergroup' clocking event on complex "
                                            "member expression; use --timing for full support.");
                                hasUnsupportedEvent = સાચું;
                            }
                        }
                        VL_DO_DANGLING(ધકેલો_Deletep(cgp->unlinkFrBack()), cgp);
                        itemp = nextp;
                        ચાલુ;
                    }
                    // V3Active handles events that do not depend on an enclosing instance.
                    UINFO(4, "Keeping covergroup event node for V3Active: " << nodep->name());
                    itemp = nextp;
                    ચાલુ;
                }
                itemp = nextp;
            }

            // Find the sample() method and constructor
            m_sampleFuncp = VN_CAST(m_memberMap.findMember(nodep, "sample"), Func);
            // V3LinkParse always synthesizes a sample() method for every covergroup, and the
            // sampling-code generation below dereferences m_sampleFuncp unconditionally.
            UASSERT_OBJ(m_sampleFuncp, nodep, "Covergroup missing synthesized sample() method");
            m_sampleFuncp->isCovergroupSample(સાચું);
            m_constructorp = VN_CAST(m_memberMap.findMember(nodep, "new"), Func);
            UINFO(9, "Found sample() method: " << (m_sampleFuncp ? "yes" : "no"));
            UINFO(9, "Found constructor: " << (m_constructorp ? "yes" : "no"));

            // If covergroup has unsupported clocking event, skip processing it
            // but still clean up coverpoints so they don't reach downstream passes
            જો (hasUnsupportedEvent) {
                iterateChildren(nodep);
                validateCovergroupExpressions();
                rebindFormalRefs();
                delete_Coverage_વસ્તુઓ();
                જો (embeddedEventForkp) {
                    VL_DO_DANGLING(ધકેલો_Deletep(embeddedEventForkp), embeddedEventForkp);
                }
                પાછા;
            }

            iterateChildren(nodep);
            validateCovergroupExpressions();
            rebindFormalRefs();
            અચલ std::vector<Ast_ગાંઠ_Assign*> constructps = findCovergroupConstructions();

            // Embedded covergroups (IEEE 1800-2023 19.4): coverpoints, iff expressions, and
            // crosses may reference members of the enclosing class. The covergroup is lowered
            // into a sibling class with no implicit handle to the enclosing instance. Install
            // an explicit back-pointer and route the references through it.
            જો (AstVarRef* અચલ invalidp = installEnclosingBackPointer(constructps)) {
                invalidp->v3error("Non-static member "
                                  << invalidp->varp()->prettyNameQ()
                                  << " of an outer class requires an explicit "
                                     "object handle (IEEE 1800-2023 8.23).");
                delete_Coverage_વસ્તુઓ();
                જો (embeddedEventForkp) {
                    VL_DO_DANGLING(ધકેલો_Deletep(embeddedEventForkp), embeddedEventForkp);
                }
                પાછા;
            }
            installEmbeddedEventTriggers(embeddedEventTriggers, constructps);
            જો (embeddedEventForkp) installEmbeddedEventFork(embeddedEventForkp, constructps);
            processCovergroup();
            // Remove lowered coverpoints/crosses from the class - they have been
            // fully translated into C++ code and must not reach downstream passes
            delete_Coverage_વસ્તુઓ();
        } નહીંતર {
            // Track the lexically enclosing class so a nested covergroup can resolve
            // references to the enclosing object's members (installEnclosingBackPointer).
            VL_RESTORER(m_enclosingClassp);
            m_enclosingClassp = nodep;
            iterateChildren(nodep);
        }
    }

    શૂન્ય visit(AstCoverpoint* nodep) override {
        UINFO(9, "Found coverpoint: " << nodep->name());
        m_coverpoints.ધકેલો_back(nodep);
        m_coverpointMap.emplace(nodep->name(), nodep);
        iterateChildren(nodep);
    }

    શૂન્ય visit(AstCoverCross* nodep) override {
        UINFO(9, "Found cross: " << nodep->name());
        m_coverCrosses.ધકેલો_back(nodep);
        iterateChildren(nodep);
    }

    // V3Width leaves only the covergroup-level options lowerCovergroupOptions() takes
    શૂન્ય visit(AstCgOptionAssign* nodep) override { m_cgOptions.ધકેલો_back(nodep); }

    // A package, interface, or module, so the design unit declaring the classes within
    શૂન્ય visit(Ast_ગાંઠ_Module* nodep) override {
        VL_RESTORER(m_unitp);
        m_unitp = nodep;
        iterateChildren(nodep);
    }

    શૂન્ય visit(Ast_ગાંઠ* nodep) override { iterateChildren(nodep); }

જાહેર:
    // CONSTRUCTORS
    FunctionalCoverageVisitor(AstNetlist* nodep,
                              અચલ std::set<અચલ AstVar*>& mergeableTypeOptions)
        : m_mergeableTypeOptions{mergeableTypeOptions} {
        iterate(nodep);
    }
    ~FunctionalCoverageVisitor() override = મૂળભૂત;
};

//######################################################################
// Functional coverage class functions

શૂન્ય V3Covergroup::covergroup(AstNetlist* nodep) {
    UINFO(4, __FUNCTION__ << ": ");
    અચલ CovergroupAssignValidVisitor validVisitor{nodep};
    જો (!validVisitor.valid()) V3Error::abortIfErrors();
    {  // Destruct before checking
        FunctionalCoverageVisitor{nodep, validVisitor.mergeableTypeOptions()};
    }
    V3Global::dump_Check_Global_વૃક્ષ("coveragefunc", 0, dump_વૃક્ષ_Either_Level() >= 3);
}
