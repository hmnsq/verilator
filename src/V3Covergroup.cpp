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

#అన్తర్భావ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#అన్తర్భావ "V3Covergroup.h"

#అన్తర్భావ "V3Const.h"
#అన్తర్భావ "V3Error.h"
#అన్తర్భావ "V3File.h"
#అన్తర్భావ "V3MemberMap.h"
#అన్తర్భావ "V3UniqueNames.h"

#అన్తర్భావ <bitset>
#అన్తర్భావ <cmath>
#అన్తర్భావ <deque>
#అన్తర్భావ <set>
#అన్తర్భావ <tuple>
#అన్తర్భావ <unordered_map>
#అన్తర్భావ <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Embedded covergroup assignment validation; and in the same pass over the netlist, the
// type_option variables through which SystemVerilog may set type_option.merge_instances

తరగతి Covergroup_కేటాయించు_Valid_Visitor final : బహిరంగ VNVisitorConst {
    VMemberMap m_memberMap;
    std::map<స్థిరరాశి AstVar*, స్థిరరాశి Ast_కణుపు_FTask*>
        m_constructors;  // Implicit instance -> constructor
    std::సమితి<స్థిరరాశి AstVar*> m_mergeableTypeOptions;  // See mergeableTypeOptions()
    స్థిరరాశి Ast_కణుపు_FTask* m_ftaskp = శూన్యసూచిక;
    తార్కిక m_collecting = నిజం;
    తార్కిక m_valid = నిజం;

    శూన్య సందర్శించు(AstClass* nodep) override {
        VL_RESTORER(m_ftaskp);
        m_ftaskp = శూన్యసూచిక;
        ఒకవేళ (m_collecting) {
            స్థిరరాశి Ast_కణుపు_FTask* స్థిరరాశి constructorp
                = VN_CAST(m_memberMap.కనుగొను_Member(nodep, "new"), కణుపు_FTask);
            కొరకు (స్థిరరాశి Ast_కణుపు* itemp = nodep->membersp(); itemp; itemp = itemp->nextp()) {
                స్థిరరాశి AstVar* స్థిరరాశి varp = VN_CAST(itemp, Var);
                // Only the implicit instance is restricted, not explicitly typed aliases.
                ఒకవేళ (!varp || !varp->isClassMember() || varp->isDeclTyped()) కొనసాగు;
                స్థిరరాశి AstClassRefDType* స్థిరరాశి refp
                    = VN_CAST(varp->dtypep()->skipRefp(), ClassRefDType);
                ఒకవేళ (refp && refp->classp()->covergroupEnclosingClassp() == nodep) {
                    m_constructors.emplace(varp, constructorp);
                }
            }
        }
        iterateChildrenConst(nodep);
    }
    శూన్య సందర్శించు(Ast_కణుపు_FTask* nodep) override {
        VL_RESTORER(m_ftaskp);
        m_ftaskp = nodep;
        iterateChildrenConst(nodep);
    }
    శూన్య సందర్శించు(Ast_కణుపు_కేటాయించు* nodep) override {
        ఒకవేళ (!m_collecting) {
            స్థిరరాశి AstVar* varp = శూన్యసూచిక;
            ఒకవేళ (స్థిరరాశి Ast_కణుపు_Var_Ref* స్థిరరాశి refp = VN_CAST(nodep->lhsp(), కణుపు_Var_Ref)) {
                varp = refp->varp();
            } లేకపోతే ఒకవేళ (స్థిరరాశి AstMemberSel* స్థిరరాశి selp = VN_CAST(nodep->lhsp(), MemberSel)) {
                varp = selp->varp();
            }
            స్థిరరాశి స్వయంచాలక it = m_constructors.కనుగొను(varp);
            ఒకవేళ (it != m_constructors.చివర() && (!m_ftaskp || m_ftaskp != it->రెండవ)) {
                m_valid = అబద్ధం;
                nodep->v3error("Embedded covergroup variable "
                               << varp->prettyNameQ()
                               << " may only be assigned in the enclosing class's 'new' method "
                                  "(IEEE 1800-2023 19.4).");
            }
        }
        iterateChildrenConst(nodep);
    }
    శూన్య సందర్శించు(Ast_కణుపు_Var_Ref* nodep) override {
        // Type options may be set at any time (IEEE 1800-2023 19.7.1), so SystemVerilog may set
        // type_option.merge_instances with any use of a type_option but a read of another
        // member, as not every write is marked yet (std::randomize)
        ఒకవేళ (m_collecting && nodep->varp()->name() == "type_option") {
            స్థిరరాశి AstStructSel* స్థిరరాశి selp = VN_CAST(nodep->backp(), StructSel);
            ఒకవేళ (!selp || selp->fromp() != nodep || selp->name() == "merge_instances"
                || !nodep->access().isReadOnly()) {
                m_mergeableTypeOptions.చొప్పించు(nodep->varp());
            }
        }
        iterateChildrenConst(nodep);
    }
    శూన్య సందర్శించు(Ast_కణుపు* nodep) override { iterateChildrenConst(nodep); }

బహిరంగ:
    స్పష్టమైన Covergroup_కేటాయించు_Valid_Visitor(AstNetlist* nodep) {
        // Uses can precede their enclosing class in the tree.
        iterateConst(nodep);
        m_collecting = అబద్ధం;
        ఒకవేళ (!m_constructors.ఖాళీలు()) iterateConst(nodep);
    }
    తార్కిక valid() స్థిరరాశి { తిరిగి m_valid; }
    // type_option variables through which SystemVerilog may set type_option.merge_instances, so
    // that covergroups are tested with optionVar(true)
    స్థిరరాశి std::సమితి<స్థిరరాశి AstVar*>& mergeableTypeOptions() స్థిరరాశి { తిరిగి m_mergeableTypeOptions; }
};

//######################################################################
// Covergroup expression validation visitor

తరగతి Covergroup_సమాసం_Valid_Visitor final : బహిరంగ VNVisitor {
    స్థిరరాశి std::సమితి<స్థిరరాశి AstVar*>& m_sampleMembers;
    స్థిరరాశి std::సమితి<స్థిరరాశి AstVar*>& m_constructorRefMembers;
    తార్కిక m_లోపల_Coverage_వ్యక్తీకరణ = అబద్ధం;
    తార్కిక m_sampleFormalAllowed = అబద్ధం;

    శూన్య పరిశీలన_Sample_వ్యక్తీకరణ(Ast_కణుపు* nodep) {
        ఒకవేళ (!nodep) తిరిగి;
        VL_RESTORER(m_sampleFormalAllowed);
        m_sampleFormalAllowed = నిజం;
        iterate_And_తదుపరి_Null(nodep);
    }

    శూన్య పరిశీలన_Coverage_వ్యక్తీకరణ(Ast_కణుపు* nodep) {
        ఒకవేళ (!nodep) తిరిగి;
        VL_RESTORER(m_లోపల_Coverage_వ్యక్తీకరణ);
        m_లోపల_Coverage_వ్యక్తీకరణ = నిజం;
        iterate_And_తదుపరి_Null(nodep);
    }

    శూన్య సందర్శించు(AstCoverpoint* nodep) override {
        పరిశీలన_Sample_వ్యక్తీకరణ(nodep->exprp());
        పరిశీలన_Sample_వ్యక్తీకరణ(nodep->iffp());
        iterate_And_తదుపరి_Null(nodep->binsp());
        iterate_And_తదుపరి_Null(nodep->optionsp());
    }
    శూన్య సందర్శించు(AstCoverCross* nodep) override {
        iterate_And_తదుపరి_Null(nodep->itemsp());
        పరిశీలన_Sample_వ్యక్తీకరణ(nodep->iffp());
        iterate_And_తదుపరి_Null(nodep->optionsp());
        iterate_And_తదుపరి_Null(nodep->binsp());
    }
    శూన్య సందర్శించు(AstCoverCrossBin* nodep) override {
        iterate_And_తదుపరి_Null(nodep->selectp());
        పరిశీలన_Sample_వ్యక్తీకరణ(nodep->iffp());
    }
    శూన్య సందర్శించు(AstCoverCrossWith* nodep) override {
        iterate_And_తదుపరి_Null(nodep->selectp());
        పరిశీలన_Coverage_వ్యక్తీకరణ(nodep->filterp());
        పరిశీలన_Coverage_వ్యక్తీకరణ(nodep->countp());
    }
    శూన్య సందర్శించు(AstCoverBinsof* nodep) override { పరిశీలన_Coverage_వ్యక్తీకరణ(nodep->rangesp()); }
    శూన్య సందర్శించు(AstCoverBin* nodep) override {
        పరిశీలన_Coverage_వ్యక్తీకరణ(nodep->rangesp());
        పరిశీలన_Sample_వ్యక్తీకరణ(nodep->iffp());
        పరిశీలన_Coverage_వ్యక్తీకరణ(nodep->arraySizep());
        పరిశీలన_Coverage_వ్యక్తీకరణ(nodep->transp());
    }
    శూన్య సందర్శించు(AstVarRef* nodep) override {
        ఒకవేళ (!m_sampleFormalAllowed && m_sampleMembers.లెక్క(nodep->varp())) {
            nodep->v3error("Covergroup sample formal argument "
                           << nodep->varp()->prettyNameQ()
                           << " may only be used in a coverpoint or conditional guard "
                              "expression (IEEE 1800-2023 19.8.1).");
        }
        ఒకవేళ (m_లోపల_Coverage_వ్యక్తీకరణ && m_constructorRefMembers.లెక్క(nodep->varp())) {
            nodep->v3error("Ref covergroup constructor formal argument "
                           << nodep->varp()->prettyNameQ()
                           << " may not be used in a covergroup expression "
                              "(IEEE 1800-2023 19.5).");
        }
    }
    శూన్య సందర్శించు(Ast_కణుపు_FTask_Ref* nodep) override {
        ఒకవేళ (m_లోపల_Coverage_వ్యక్తీకరణ && nodep->taskp()) {
            తార్కిక invalidDirection = అబద్ధం;
            కొరకు (Ast_కణుపు* stmtp = nodep->taskp()->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
                స్థిరరాశి AstVar* స్థిరరాశి varp = VN_CAST(stmtp, Var);
                ఒకవేళ (varp && varp->isIO() && varp->isWritable()) {
                    invalidDirection = నిజం;
                    ఆపు;
                }
            }
            ఒకవేళ (invalidDirection) {
                nodep->v3error("Function " << nodep->taskp()->prettyNameQ()
                                           << " called in a covergroup expression has an "
                                              "output, inout, or non-const ref argument "
                                              "(IEEE 1800-2023 19.5).");
            }
        }
        iterateChildren(nodep);
    }
    శూన్య సందర్శించు(Ast_కణుపు* nodep) override { iterateChildren(nodep); }

బహిరంగ:
    Covergroup_సమాసం_Valid_Visitor(స్థిరరాశి std::సమితి<స్థిరరాశి AstVar*>& sampleMembers,
                               స్థిరరాశి std::సమితి<స్థిరరాశి AstVar*>& constructorRefMembers)
        : m_sampleMembers{sampleMembers}
        , m_constructorRefMembers{constructorRefMembers} {}
    శూన్య పరిశీలన(Ast_కణుపు* nodep) { iterate(nodep); }
};

//######################################################################
// Bins of one declaration whose values are computed rather than listed: bin k covers
// [m_lo + k * m_stride, m_lo + (k + 1) * m_stride - 1], and the last bin extends to m_hi.  An
// array bin element is a run of single-value bins; automatic bins partition the coverpoint
// domain.  Bounds are coverpoint values at FunctionalCoverageVisitor::runWidth(),
// sign-extended like a CrossValueRange's.

తరగతి Bin_నిరంతరం final {
బహిరంగ:
    // MEMBERS
    uint32_t m_లెక్క;  // Number of bins
    V3_సంఖ్య m_lo;  // Lowest value of the first bin
    V3_సంఖ్య m_stride;  // Number of values of each bin but the last
    V3_సంఖ్య m_hi;  // Highest value of the last bin
    తార్కిక m_ఖాళీలు = అబద్ధం;  // A single bin without a value of the coverpoint type
    uint32_t m_declared = 0;  // Runtime index of the first bin, once generated

    // CONSTRUCTORS
    Bin_నిరంతరం(Ast_కణుపు* nodep, పూర్ణాంకం వెడల్పు, uint32_t లెక్క)
        : m_లెక్క{లెక్క}
        , m_lo{nodep, వెడల్పు}
        , m_stride{nodep, వెడల్పు, 1}
        , m_hi{nodep, వెడల్పు} {}
};

//######################################################################
// Functional coverage visitor

తరగతి FunctionalCoverageVisitor final : బహిరంగ VNVisitor {
    // NODE STATE
    // Entire netlist:
    //  AstCoverpoint::user1p()  -> AstVar*.  Previous-value variable for transition bins
    //  AstCoverpoint::user2()   -> bool.  Had a bins declaration ignored, so no automatic bins
    స్థిరరాశి VNUser1_లోపల_Use m_inuser1;
    స్థిరరాశి VNUser2_లోపల_Use m_inuser2;

    // STATE
    std::సమితి<AstCoverpoint*>
        m_runtimePoints;  // Points needing value metadata and live-bin mapping
    std::సమితి<AstCoverCross*> m_runtimeCrosses;  // Crosses over finalized live-bin dimensions
    std::map<AstVar*, AstVar*> m_excludedVars;  // Sample-time state-exclusion flags
    AstClass* m_covergroupp = శూన్యసూచిక;  // Current covergroup being processed
    Ast_కణుపు_Module* m_unitp = శూన్యసూచిక;  // Design unit declaring the current class, outside classes
    AstClass* m_enclosingClassp = శూన్యసూచిక;  // Class lexically enclosing the covergroup, if any
    AstVar* m_embeddedVarp = శూన్యసూచిక;  // Embedded covergroup member of m_enclosingClassp, if any
    std::string m_covergroupName;  // Current covergroup's type name, see covergroupTypeName()
    AstFunc* m_sampleFuncp = శూన్యసూచిక;  // Current sample() function
    AstIf* m_cpGuardp = శూన్యసూచిక;  // Current coverpoint's iff guard, holding its sampling
    AstFunc* m_constructorp = శూన్యసూచిక;  // Current constructor
    std::సదిశ<AstCoverpoint*> m_coverpoints;  // Coverpoints in current covergroup
    std::map<std::string, AstCoverpoint*> m_coverpointMap;  // Name -> coverpoint for fast lookup
    std::సదిశ<AstCoverCross*> m_coverCrosses;  // Cross coverage items in current covergroup
    std::సదిశ<Ast_Cg_Option_కేటాయించు*> m_cgOptions;  // Covergroup-level options, before lowering
    uint32_t m_cg_Type_బరువు = 1;  // The covergroup's type_option.weight, a constant
    తార్కిక m_cg_విలీనం_Instances = అబద్ధం;  // The covergroup's type_option.merge_instances, a constant
    // The covergroup's type_option.merge_instances is, or SystemVerilog may make it, true
    తార్కిక m_cg_May_విలీనం = అబద్ధం;
    // type_option members through which SystemVerilog may set type_option.merge_instances,
    // see CovergroupAssignValidVisitor::mergeableTypeOptions()
    స్థిరరాశి std::సమితి<స్థిరరాశి AstVar*>& m_mergeableTypeOptions;
    పూర్ణాంకం m_cg_At_కనిష్ఠ = 1;  // Covergroup option.at_least

    నిర్మాణం EmbeddedEventTrigger final {
        File_పంక్తి* eventFl;  // Clocking-event source location
        AstVar* పునాది_Varp;  // Base enclosing-class member in the event expression
        AstVar* memberVarp;  // Selected member in a 'base.member' expression, or nullptr
        VEdgeType అంచు_Type;  // Clocking-event edge qualifier
        AstVar* prevVarp;  // Member containing the previous event value, or nullptr
        EmbeddedEventTrigger(File_పంక్తి* eventFl, AstVar* పునాది_Varp, AstVar* memberVarp,
                             VEdgeType అంచు_Type)
            : eventFl{eventFl}
            , పునాది_Varp{పునాది_Varp}
            , memberVarp{memberVarp}
            , అంచు_Type{అంచు_Type}
            , prevVarp{శూన్యసూచిక} {}
    };

    std::సమితి<std::string> m_crossed_Cp_పేర్లు;  // Coverpoints referenced by a cross
    std::map<std::string, AstVar*> m_cpVarMap;  // Coverpoint name -> its VlCoverpoint member
    నిర్మాణం BinRuns final {
        std::సదిశ<Bin_నిరంతరం> runs;  // Runs of an array or automatic bins declaration, in order
        uint32_t లెక్క = 0;  // Bins across all runs
        తార్కిక unsupported = అబద్ధం;  // Too many bins, or invalid: the declaration is ignored
        // The value of each bin, which names it, of a wildcard array; else the bins are indexed
        std::సదిశ<std::string> విలువలు;
        // Too many values of an ignore or illegal wildcard array, which is then one bin
        తార్కిక single = అబద్ధం;
    };
    నిర్మాణం Cross_Bin_విలువలు final {
        AstCoverBin* binp;  // Declaration owning this Normal bin
        Ast_కణుపు_సమాసం* valuep;  // Individual array-bin value, or nullptr for a scalar bin
        స్థిరరాశి Bin_నిరంతరం* runp = శూన్యసూచిక;  // Run computing the bin's values, if any
        uint32_t element = 0;  // Index of the bin within runp
    };
    నిర్మాణం Bin_పరిధి final {
        uint32_t మొదటి = 0;  // First Normal index of the bin declaration
        uint32_t లెక్క = 0;  // Number of Normal bins of the declaration
        uint32_t declared = 0;  // First runtime bin index, across all bin kinds
        int32_t sized = -1;  // Index of the sized array, placed at construction; or -1
    };
    నిర్మాణం CoverpointBins final {
        uint32_t మొత్తం = 0;  // Number of Normal bins
        Ast_కణుపు_సమాసం* exprp = శూన్యసూచిక;  // Sampled expression, for the value domain
        తార్కిక crossed = అబద్ధం;  // Feeds a cross, which needs 'values'
        std::సదిశ<Cross_Bin_విలువలు> విలువలు;  // Values in runtime Normal-bin index order
        std::unordered_map<std::string, Bin_పరిధి> spans;  // Declared bin name -> index span
        Bin_పరిధి implicitAuto{0, 0, 0};  // Implicit automatic bins, each named 'auto_<i>'
        std::deque<Bin_నిరంతరం> runs;  // Runs 'values' refers to
    };
    std::map<AstVar*, CoverpointBins> m_cpBins;  // Runtime coverpoint -> binsof index ranges
    // Names of the bins declarations each coverpoint ignored, which binsof selects as no bins
    std::map<స్థిరరాశి AstCoverpoint*, std::సదిశ<std::string>> m_droppedBins;
    // Prefixes of the constructor temporaries of constructed bins, as coverpoint and bin names
    // alone may repeat: coverpoint 'a_' bins 'b', and coverpoint 'a' bins '_b'
    V3_Unique_పేర్లు m_sized_పేర్లు{"__Vsized"};
    std::సదిశ<Ast_కణుపు_సమాసం*> m_detached_విలువలు;  // Array-bin values m_cpBins refers to
    std::సమితి<AstCoverCross*>
        m_droppedCrosses;  // Crosses with a bare-variable item: drop (COVERIGN)
    std::map<uint32_t, AstCoverpointDType*> m_cpDTypes;  // Hit-list bound -> interned dtype
    ఉపయోగించి CrossShape = std::tuple<uint32_t, uint32_t, uint32_t, uint32_t, uint64_t, తార్కిక>;
    std::map<CrossShape, AstCoverCrossDType*> m_cxDTypes;
    AstVar* m_cgInstVarp = శూన్యసూచిక;  // __Vcg_inst handle member of the current covergroup

    VMemberMap m_memberMap;  // Member names cached for fast lookup

    // METHODS
    // The covergroup's 'option' or static 'type_option' member (V3LinkParse creates both)
    AstVar* optionVar(తార్కిక typeOption) {
        AstVar* స్థిరరాశి varp = VN_AS(
            m_memberMap.కనుగొను_Member(m_covergroupp, typeOption ? "type_option" : "option"), Var);
        UASSERT_OBJ(varp, m_covergroupp, "Covergroup missing option member");
        తిరిగి varp;
    }

    // 'option.<member>' or 'type_option.<member>', per optionVarp
    AstStructSel* newOptionSel(File_పంక్తి* fl, AstVar* optionVarp, స్థిరరాశి std::string& member,
                               VAccess access) {
        స్థిరరాశి AstMemberDType* స్థిరరాశి memberp
            = VN_AS(m_memberMap.కనుగొను_Member(optionVarp->dtypep()->skipRefp(), member), MemberDType);
        UASSERT_OBJ(memberp, optionVarp, "Coverage option structure missing '" << member << "'");
        Ast_కణుపు_సమాసం* స్థిరరాశి fromp = optionVarp->lifetime().isStatic()
                                       ? కొత్త AstVarRef{fl, optionVarp, access}
                                       : memberRef(fl, optionVarp, access);
        AstStructSel* స్థిరరాశి selp = కొత్త AstStructSel{fl, fromp, member};
        selp->dtypep(memberp->subDTypep()->skip_Ref_గమ్యం_Enump());
        selp->did_వెడల్పు(నిజం);
        తిరిగి selp;
    }

    // A constant option.at_least; a negative count of hits has no meaning, so takes the default
    స్థిర పూర్ణాంకం const_At_కనిష్ఠ(స్థిరరాశి AstConst* constp) {
        స్థిరరాశి పూర్ణాంకం విలువ = constp->గమ్యం_SInt();
        ఒకవేళ (విలువ >= 0) తిరిగి విలువ;
        constp->v3warn(COVERIGN, "Ignoring negative 'option.at_least'; using 1");
        తిరిగి 1;
    }

    // An option's constant value, or nullptr after a warning, as non-constant is unsupported
    స్థిర స్థిరరాశి AstConst* constOption(స్థిరరాశి Ast_కణుపు* valuep, VCoverOptionType optType) {
        ఒకవేళ (స్థిరరాశి AstConst* స్థిరరాశి constp = VN_CAST(valuep, Const)) తిరిగి constp;
        valuep->v3warn(COVERIGN, "Ignoring unsupported: non-constant 'option."
                                     << optType.ascii() << "'; using default value");
        తిరిగి శూన్యసూచిక;
    }

    // Store the covergroup-level options (IEEE 1800-2023 19.7) where SystemVerilog and the
    // runtime read them.  The instance options, option.weight and option.get_inst_coverage,
    // are evaluated by the constructor, as are the other instance options; the type options,
    // type_option.weight and type_option.merge_instances, are constant, and initialize the
    // static member.  Without its own type_option.merge_instances, a covergroup has the
    // default of --coverage-merge-instances.  A constant option.at_least is the default of
    // the coverpoints and crosses.
    శూన్య క్రింది_Covergroup_Options() {
        తార్కిక విలీనం_సమితి = అబద్ధం;  // The covergroup sets type_option.merge_instances
        m_cg_At_కనిష్ఠ = 1;
        కొరకు (Ast_Cg_Option_కేటాయించు* స్థిరరాశి optp : m_cgOptions) {
            ఒకవేళ (optp->optType() == VCoverOptionType::AT_కనిష్ఠ) {
                ఒకవేళ (స్థిరరాశి AstConst* స్థిరరాశి constp = constOption(optp->valuep(), optp->optType())) {
                    m_cg_At_కనిష్ఠ = const_At_కనిష్ఠ(constp);
                }
                VL_DO_DANGLING(నెట్టు_Deletep(optp->unlink_Fr_వెనుక()), optp);
                కొనసాగు;
            }
            File_పంక్తి* స్థిరరాశి fl = optp->fileline();
            // V3Width left the type options constant, and type_option.weight non-negative
            స్థిరరాశి AstConst* స్థిరరాశి constp = VN_CAST(optp->valuep(), Const);
            std::string member = "weight";
            ఒకవేళ (optp->optType() == VCoverOptionType::విలీనం_INSTANCES) {
                member = "merge_instances";
                m_cg_విలీనం_Instances = !constp->అంకె().isEqZero();
                విలీనం_సమితి = నిజం;
            } లేకపోతే ఒకవేళ (optp->optType() == VCoverOptionType::GET_INST_COVERAGE) {
                member = "get_inst_coverage";
            } లేకపోతే {
                UASSERT_OBJ(optp->optType() == VCoverOptionType::బరువు, optp,
                            "Unexpected covergroup option reaching V3Covergroup");
                ఒకవేళ (optp->typeOption()) m_cg_Type_బరువు = constp->గమ్యం_UInt();
            }
            Ast_కేటాయించు* స్థిరరాశి assignp = కొత్త Ast_కేటాయించు{
                fl, newOptionSel(fl, optionVar(optp->typeOption()), member, VAccess::WRITE),
                optp->valuep()->unlink_Fr_వెనుక()};
            ఒకవేళ (optp->typeOption()) {
                m_covergroupp->కూడు_Membersp(కొత్త AstInitialStatic{fl, assignp});
                VL_DO_DANGLING(నెట్టు_Deletep(optp->unlink_Fr_వెనుక()), optp);
            } లేకపోతే {
                optp->replaceWith(assignp);
                VL_DO_DANGLING(నెట్టు_Deletep(optp), optp);
            }
        }
        m_cgOptions.clear();
        ఒకవేళ (!విలీనం_సమితి && v3Global.opt.coverage_విలీనం_Instances()) {
            File_పంక్తి* స్థిరరాశి fl = m_covergroupp->fileline();
            m_covergroupp->కూడు_Membersp(కొత్త AstInitialStatic{
                fl, కొత్త Ast_కేటాయించు{
                        fl, newOptionSel(fl, optionVar(నిజం), "merge_instances", VAccess::WRITE),
                        కొత్త AstConst{fl, AstConst::బిట్టు_True{}}}});
            m_cg_విలీనం_Instances = నిజం;
        }
        m_cg_May_విలీనం = m_cg_విలీనం_Instances || m_mergeableTypeOptions.లెక్క(optionVar(నిజం));
    }

    // The weight of an item in the coverage database, which merges the instances: its
    // type_option.weight if the covergroup merges them too (IEEE 1800-2023 19.11.3); else its
    // option.weight if a constant, and so of every instance; else its type_option.weight
    uint32_t అంశం_Database_బరువు(Ast_కణుపు* optionsp) స్థిరరాశి {
        స్థిరరాశి Ast_కణుపు_సమాసం* weightp = శూన్యసూచిక;  // The option.weight in effect
        uint32_t type_బరువు = 1;
        కొరకు (Ast_కణుపు* nodep = optionsp; nodep; nodep = nodep->nextp()) {
            స్థిరరాశి AstCoverOption* స్థిరరాశి optp = VN_AS(nodep, CoverOption);
            ఒకవేళ (!(optp->optType() == VCoverOptionType::బరువు)) కొనసాగు;
            // V3Width left type_option.weight a non-negative constant
            ఒకవేళ (optp->typeOption()) {
                type_బరువు = VN_AS(optp->valuep(), Const)->గమ్యం_UInt();
            } లేకపోతే {
                weightp = optp->valuep();
            }
        }
        ఒకవేళ (m_cg_విలీనం_Instances) తిరిగి type_బరువు;
        ఒకవేళ (!weightp) తిరిగి 1;
        ఒకవేళ (స్థిరరాశి AstConst* స్థిరరాశి constp = VN_CAST(weightp, Const)) తిరిగి constp->గమ్యం_UInt();
        తిరిగి type_బరువు;
    }

    // Configure an item's option.weight, its weight in instance coverage (IEEE 1800-2023
    // 19.11), and, if the covergroup may merge its instances, its type_option.weight, its
    // weight in type coverage then (19.11.3).
    శూన్య generate_అంశం_బరువు(File_పంక్తి* fl, AstVar* అంశం_Varp, Ast_కణుపు* optionsp) {
        కొరకు (Ast_కణుపు* nodep = optionsp; nodep; nodep = nodep->nextp()) {
            స్థిరరాశి AstCoverOption* స్థిరరాశి optp = VN_AS(nodep, CoverOption);
            ఒకవేళ (!(optp->optType() == VCoverOptionType::బరువు)) కొనసాగు;
            ఒకవేళ (!optp->typeOption()) {
                m_constructorp->కూడు_Stmtsp(
                    అంశం_Call(fl, అంశం_Varp, VCMethod::COVERGROUP_బరువు,
                             {optp->valuep()->clone_వృక్షం(అబద్ధం), file_పంక్తి_Debug(optp->fileline())})
                        ->makeStmt());
            } లేకపోతే ఒకవేళ (m_cg_May_విలీనం) {
                // V3Width left type_option.weight a non-negative constant
                m_constructorp->కూడు_Stmtsp(అంశం_Call(fl, అంశం_Varp, VCMethod::COVERGROUP_TYPE_బరువు,
                                                   {optp->valuep()->clone_వృక్షం(అబద్ధం)})
                                              ->makeStmt());
            }
        }
    }

    శూన్య processCovergroup() {
        UINFO(4, "Processing covergroup: " << m_covergroupp->name() << " with "
                                           << m_coverpoints.కొలత() << " coverpoints and "
                                           << m_coverCrosses.కొలత() << " crosses");

        m_crossed_Cp_పేర్లు.clear();
        m_cpVarMap.clear();
        m_cpBins.clear();
        m_droppedBins.clear();
        m_sized_పేర్లు.reset();
        m_runtimePoints.clear();
        m_runtimeCrosses.clear();
        m_excludedVars.clear();
        m_droppedCrosses.clear();
        m_cgInstVarp = శూన్యసూచిక;
        m_cg_Type_బరువు = 1;
        m_cg_విలీనం_Instances = అబద్ధం;
        m_cg_May_విలీనం = అబద్ధం;

        క్రింది_Covergroup_Options();

        // Scan every cross item to record the coverpoints it references (the cross dimensions)
        // and to flag any cross naming a bare variable -- a would-be implicit coverpoint, which
        // Verilator does not synthesize.  An unresolvable item drops only that one cross (with a
        // COVERIGN in generateCrossCode), leaving the rest of the covergroup intact.
        కొరకు (AstCoverCross* crossp : m_coverCrosses) {
            కొరకు (Ast_కణుపు* itemp = crossp->itemsp(); itemp; itemp = itemp->nextp()) {
                స్థిరరాశి AstCoverpointRef* స్థిరరాశి refp = VN_AS(itemp, CoverpointRef);
                ఒకవేళ (refp->exprp()) కొనసాగు;  // hierarchical ref: dropped in generateCrossCode
                ఒకవేళ (m_coverpointMap.కనుగొను(refp->name()) == m_coverpointMap.చివర()) {
                    m_droppedCrosses.చొప్పించు(crossp);  // bare variable: drop this cross only
                } లేకపోతే {
                    m_crossed_Cp_పేర్లు.చొప్పించు(refp->name());
                }
            }
        }

        std::సదిశ<Ast_కణుపు*> pending;
        std::map<AstCoverpoint*, std::సదిశ<AstCoverCross*>> consumers;
        std::map<AstCoverCross*, std::సదిశ<AstCoverpoint*>> inputs;
        కొరకు (AstCoverpoint* స్థిరరాశి cpp : m_coverpoints) {
            check_Bin_పేర్లు(cpp);
            checkConstructedBins(cpp);
            ఒకవేళ (!cpp->exprp()->dtypep()->skipRefp()->isIntegralOrPacked()) కొనసాగు;
            // Bins without values leave the report (IEEE 1800-2023 19.11.1), exclusions or not.
            // Constructed bins get their values when the covergroup is constructed.
            ఒకవేళ (!coverpointHasStateExclusions(cpp) && !coverpoint_Has_ఖాళీలు_Bins(cpp)
                && !coverpointHasConstructedBins(cpp)) {
                కొనసాగు;
            }
            m_runtimePoints.చొప్పించు(cpp);
            pending.నెట్టు_వెనుక(cpp);
        }
        కొరకు (AstCoverCross* స్థిరరాశి crossp : m_coverCrosses) {
            ఒకవేళ (m_droppedCrosses.లెక్క(crossp)) కొనసాగు;
            కొరకు (Ast_కణుపు* itemp = crossp->itemsp(); itemp; itemp = itemp->nextp()) {
                స్థిరరాశి AstCoverpointRef* స్థిరరాశి refp = VN_AS(itemp, CoverpointRef);
                ఒకవేళ (refp->exprp()) కొనసాగు;
                స్థిరరాశి స్వయంచాలక బిందువు = m_coverpointMap.కనుగొను(refp->name());
                ఒకవేళ (బిందువు != m_coverpointMap.చివర()) {
                    consumers[బిందువు->రెండవ].నెట్టు_వెనుక(crossp);
                    inputs[crossp].నెట్టు_వెనుక(బిందువు->రెండవ);
                }
            }
            // The values of a filter's bin tuples are known when the covergroup is constructed
            ఒకవేళ (crossp->exists([](స్థిరరాశి AstCoverCrossWith*) { తిరిగి నిజం; })) {
                m_runtimeCrosses.emplace(crossp);
                pending.నెట్టు_వెనుక(crossp);
            }
        }
        కొరకు (కొలత_t తదుపరి = 0; తదుపరి < pending.కొలత(); ++తదుపరి) {
            ఒకవేళ (AstCoverpoint* స్థిరరాశి pointp = VN_CAST(pending[తదుపరి], Coverpoint)) {
                కొరకు (AstCoverCross* స్థిరరాశి crossp : consumers[pointp]) {
                    ఒకవేళ (m_runtimeCrosses.emplace(crossp).రెండవ) pending.నెట్టు_వెనుక(crossp);
                }
            } లేకపోతే {
                కొరకు (AstCoverpoint* స్థిరరాశి pointp : inputs[VN_AS(pending[తదుపరి], CoverCross)]) {
                    ఒకవేళ (pointp->exprp()->dtypep()->skipRefp()->isIntegralOrPacked()
                        && m_runtimePoints.emplace(pointp).రెండవ) {
                        pending.నెట్టు_వెనుక(pointp);
                    }
                }
            }
        }

        // The instance node owns this instance's coverpoint/cross runtimes, so it must exist
        // before any of them is created.  Emitted first, ahead of both generate loops.
        generateInstanceAttach();

        // For each coverpoint, generate sampling code
        కొరకు (AstCoverpoint* cpp : m_coverpoints) generate_Coverpoint_సంకేతం(cpp);

        // For each cross, generate sampling code
        కొరకు (AstCoverCross* crossp : m_coverCrosses) generate_Cross_సంకేతం(crossp);

        // Every cross has been built, so runtime points only need their exclusions from here.
        కొరకు (AstCoverpoint* స్థిరరాశి cpp : m_coverpoints) {
            ఒకవేళ (!m_runtimePoints.లెక్క(cpp)) కొనసాగు;
            m_constructorp->కూడు_Stmtsp(అంశం_Call(cpp->fileline(), m_cpVarMap.at(cpp->name()),
                                               VCMethod::COVERGROUP_విలువ_RELEASE)
                                          ->makeStmt());
        }
        కొరకు (Ast_కణుపు_సమాసం* valuep : m_detached_విలువలు) VL_DO_DANGLING(నెట్టు_Deletep(valuep), valuep);
        m_detached_విలువలు.clear();

        // Generate coverage computation code (even for empty covergroups).  Bin registration
        // with the coverage database is handled per coverpoint/cross by their runtime
        // registerBins() calls (emitted in generateCoverpoint/generateCross).
        generate_Coverage_Computation_సంకేతం();
    }

    స్థిర constexpr కొలత_t విలువ_LIST_ENTRIES = 256;  // Metadata entries per constructor call

    // The value of a constant count, of bins or value tuples: -1 if it is negative, and
    // saturated above the largest limit
    స్థిర int64_t const_లెక్క(స్థిరరాశి AstConst* constp) {
        స్థిరరాశి V3_సంఖ్య& అంకె = constp->అంకె();
        ఒకవేళ (constp->isSigned() && అంకె.isNegative()) తిరిగి -1;
        తిరిగి అంకె.most_సమితి_బిట్టు_P1() > 32 ? INT64_MAX : static_cast<int64_t>(అంకె.గమ్యం_UQuad());
    }

    // Whether a count, 'whatp', is integral and, if constant, positive, as the standard's clause
    // 'clausep' requires; else errors
    స్థిర తార్కిక check_లెక్క(స్థిరరాశి Ast_కణుపు_సమాసం* countp, స్థిరరాశి అక్షరం* whatp, స్థిరరాశి అక్షరం* clausep) {
        స్థిరరాశి AstConst* స్థిరరాశి constp = VN_CAST(countp, Const);
        ఒకవేళ (!countp->dtypep()->skipRefp()->isIntegralOrPacked()) {
            countp->v3error(whatp << " must be integral (IEEE 1800-2023 " << clausep << ")");
            తిరిగి అబద్ధం;
        } లేకపోతే ఒకవేళ (constp && (constp->అంకె().isFourState() || const_లెక్క(constp) < 1)) {
            countp->v3error(whatp << " must be >= 1, got "
                                  << (constp->అంకె().isFourState() ? constp->అంకె().ascii(అబద్ధం)
                                                                  : constp->అంకె().గమ్యం_Decimal_S())
                                  << " (IEEE 1800-2023 " << clausep << ")");
            తిరిగి అబద్ధం;
        }
        తిరిగి నిజం;
    }

    // The number of bins requested by a valid 'bins auto[N]', or 0
    స్థిర uint32_t autoBinsRequested(స్థిరరాశి AstCoverBin* binp) {
        స్థిరరాశి AstConst* స్థిరరాశి constp = VN_CAST(binp->arraySizep(), Const);
        ఒకవేళ (!constp) తిరిగి 0;
        స్థిరరాశి int64_t లెక్క = const_లెక్క(constp);
        తిరిగి లెక్క < 1 || లెక్క > v3Global.opt.coverageMaxBins() ? 0
                                                                   : static_cast<uint32_t>(లెక్క);
    }

    // True for a 'bins auto[N]' declaration, or the implicit automatic bins of a coverpoint
    స్థిర తార్కిక isAutoBins(స్థిరరాశి AstCoverBin* binp) {
        తిరిగి binp->binsType() == VCoverBinsType::BINS_AUTO
               || binp->binsType() == VCoverBinsType::BINS_AUTO_IMPLICIT;
    }

    // True for a sized array of bins, 'bins b[N] = {...}', whose values an integral coverpoint
    // distributes over N bins when the covergroup is constructed (IEEE 1800-2023 19.5.1)
    స్థిర తార్కిక isSizedArray(స్థిరరాశి AstCoverBin* binp) {
        తిరిగి binp->arraySizep() && !isAutoBins(binp);
    }

    // The 'with' filter of a bin, if any (IEEE 1800-2023 19.5.1.1)
    స్థిర AstCoverWith* binWith(స్థిరరాశి AstCoverBin* binp) {
        తిరిగి VN_CAST(binp->rangesp(), CoverWith);
    }

    // True for bins whose values the covergroup constructor computes: a sized array, or bins of
    // the values a 'with' filter keeps
    స్థిర తార్కిక isConstructedBins(స్థిరరాశి AstCoverBin* binp) {
        తిరిగి isSizedArray(binp) || binWith(binp);
    }

    // The range list of a bin's values, or of the candidates of its 'with' filter; null for all
    // of the coverpoint's values, which a filter of the coverpoint's name has
    స్థిర Ast_కణుపు* binRangesp(స్థిరరాశి AstCoverBin* binp) {
        స్థిరరాశి AstCoverWith* స్థిరరాశి withp = binWith(binp);
        ఒకవేళ (!withp) తిరిగి binp->rangesp();
        తిరిగి VN_IS(withp->subp(), CoverpointRef) ? శూన్యసూచిక : withp->subp();
    }

    // Report and delete a bins declaration of a name that another of its coverpoint has
    శూన్య check_Bin_పేర్లు(AstCoverpoint* coverpointp) {
        std::సమితి<std::string> పేర్లు;
        కొరకు (Ast_కణుపు* nodep = coverpointp->binsp(); nodep;) {
            AstCoverBin* స్థిరరాశి binp = VN_AS(nodep, CoverBin);
            nodep = nodep->nextp();
            ఒకవేళ (పేర్లు.emplace(binp->name()).రెండవ) కొనసాగు;
            binp->v3error("Duplicate bin " << binp->prettyNameQ() << " in coverpoint "
                                           << coverpointp->prettyNameQ()
                                           << " (IEEE 1800-2023 3.13)");
            VL_DO_DANGLING(నెట్టు_Deletep(binp->unlink_Fr_వెనుక()), binp);
        }
    }

    // Delete an ignored bins declaration, which binsof then selects as no bins
    శూన్య dropBins(స్థిరరాశి AstCoverpoint* coverpointp, AstCoverBin* binp) {
        m_droppedBins[coverpointp].నెట్టు_వెనుక(binp->name());
        VL_DO_DANGLING(నెట్టు_Deletep(binp->unlink_Fr_వెనుక()), binp);
    }

    // Check the size of a sized array of bins, which drops an invalid array.  A real coverpoint's
    // are unsupported, and treated as arrays of a bin per value.  False unless it stays sized.
    తార్కిక check_Bins_Array_కొలత(స్థిరరాశి AstCoverpoint* coverpointp, AstCoverBin* binp, తార్కిక integral) {
        Ast_కణుపు_సమాసం* స్థిరరాశి sizep = binp->arraySizep();
        ఒకవేళ (VN_IS(sizep, Unbounded)) {  // A parameter of '$'; see bins_orBraE
            binp->v3error("Bins array size must be integral, not '$' (IEEE 1800-2023 19.5.1)");
        } లేకపోతే ఒకవేళ (check_లెక్క(sizep, "Bins array size", "19.5.1")) {
            ఒకవేళ (integral) తిరిగి నిజం;
            binp->v3warn(COVERIGN, "Unsupported: 'bins' explicit array size of a real "
                                   "coverpoint (treated as '[]')");
            VL_DO_DANGLING(నెట్టు_Deletep(sizep->unlink_Fr_వెనుక()), sizep);
            తిరిగి అబద్ధం;
        }
        dropBins(coverpointp, binp);
        తిరిగి అబద్ధం;
    }

    // Check the bins of a coverpoint whose values the constructor computes, dropping invalid
    // ones.  A wildcard array of too many ranges of values is ignored, or if ignore or illegal,
    // treated as one bin; filtering too many ranges of values ignores the bins.
    శూన్య checkConstructedBins(AstCoverpoint* coverpointp) {
        స్థిరరాశి తార్కిక integral = coverpointp->exprp()->dtypep()->skipRefp()->isIntegralOrPacked();
        కొరకు (Ast_కణుపు* nodep = coverpointp->binsp(); nodep;) {
            AstCoverBin* స్థిరరాశి binp = VN_AS(nodep, CoverBin);
            nodep = nodep->nextp();
            ఒకవేళ (!isConstructedBins(binp)) కొనసాగు;
            ఒకవేళ (isSizedArray(binp) && !check_Bins_Array_కొలత(coverpointp, binp, integral)) కొనసాగు;
            ఒకవేళ (!binp->isWildcard()
                || sizedWildcardRuns(binp, coverpointp->exprp())
                       <= v3Global.opt.coverageMaxBins()) {
                ఒకవేళ (binWith(binp) && withCandidatesOver(binp, coverpointp->exprp())) {
                    binp->v3warn(COVERIGN, "Unsupported: 'with' filter of more than 2**32 "
                                           "candidate values; bin "
                                               << binp->prettyNameQ() << " ignored");
                    ఒకవేళ (binp->binsType().bin_Is_సాధారణ()) coverpointp->user2(నిజం);
                    dropBins(coverpointp, binp);
                }
                కొనసాగు;
            }
            // An ignore or illegal array still excludes or checks its values, as one bin
            స్థిరరాశి తార్కిక single = !binp->binsType().bin_Is_సాధారణ() && !binWith(binp);
            binp->v3warn(COVERIGN,
                         "Unsupported: " << (binWith(binp) ? "'with' filter of wildcard '"
                                                           : "sized wildcard array '")
                                         << binp->binsType().verilogKwd()
                                         << "' of more than --coverage-max-bins of "
                                         << v3Global.opt.coverageMaxBins()
                                         << " ranges of values; bin " << binp->prettyNameQ()
                                         << (single ? " treated as one bin" : " ignored") << "\n"
                                         << binp->warn_మరిన్ని()
                                         << "... Suggest a larger --coverage-max-bins");
            ఒకవేళ (single) {
                Ast_కణుపు_సమాసం* స్థిరరాశి sizep = binp->arraySizep();
                VL_DO_DANGLING(నెట్టు_Deletep(sizep->unlink_Fr_వెనుక()), sizep);
                binp->isArray(అబద్ధం);
                కొనసాగు;
            }
            ఒకవేళ (binp->binsType().bin_Is_సాధారణ()) coverpointp->user2(నిజం);
            dropBins(coverpointp, binp);
        }
    }

    // True if a 'with' filter would be evaluated for more than 2**32 candidate values, known
    // now for the coverpoint's name, or for a range list of constants: each is evaluated once
    // but for an array 'b[N]', which keeps their order and duplicates (see withBegin())
    స్థిర తార్కిక withCandidatesOver(AstCoverBin* binp, Ast_కణుపు_సమాసం* exprp) {
        స్థిరరాశి పూర్ణాంకం వెడల్పు = నిరంతరం_వెడల్పు(exprp);
        std::సదిశ<std::pair<V3_సంఖ్య, V3_సంఖ్య>> runs;
        ఒకవేళ (!binRangesp(binp)) runs = coverpoint_విలువలు(binp, exprp);
        స్థిరరాశి స్వయంచాలక constant
            = [](స్థిరరాశి Ast_కణుపు* nodep) { తిరిగి VN_IS(nodep, Const) || VN_IS(nodep, Unbounded); };
        కొరకు (Ast_కణుపు* rangep = binRangesp(binp); rangep; rangep = rangep->nextp()) {
            స్థిరరాశి AstInsideRange* స్థిరరాశి irp = VN_CAST(rangep, InsideRange);
            // Else known when constructed
            ఒకవేళ (irp ? !constant(irp->lhsp()) || !constant(irp->rhsp()) : !VN_IS(rangep, Const)) {
                తిరిగి అబద్ధం;
            }
            Cross_విలువ_Range range{rangep, resolve_వెడల్పు(rangep, exprp)};
            ఒకవేళ (!resolve_విలువ(rangep, exprp, నిజం, binp->isWildcard(), range)
                || cross_Range_ఖాళీలు(range)) {
                కొనసాగు;
            }
            std::సదిశ<std::pair<V3_సంఖ్య, V3_సంఖ్య>> దొరికింది{{range.lo, range.hi}};
            ఒకవేళ (range.wildcard) {  // checkConstructedBins bounded the runs
                దొరికింది.clear();
                crossRangeRuns(range, v3Global.opt.coverageMaxBins(), దొరికింది);
            }
            కొరకు (స్థిరరాశి std::pair<V3_సంఖ్య, V3_సంఖ్య>& నిరంతరం : దొరికింది) {
                runs.emplace_వెనుక(V3_సంఖ్య{rangep, వెడల్పు, నిరంతరం.మొదటి},
                                  V3_సంఖ్య{rangep, వెడల్పు, నిరంతరం.రెండవ});
            }
        }
        ఒకవేళ (!isSizedArray(binp)) {  // The union of the values
            std::క్రమీకరించు(runs.begin(), runs.చివర(), [](స్థిరరాశి స్వయంచాలక& lhs, స్థిరరాశి స్వయంచాలక& rhs) {
                తిరిగి cross_విలువ_Less(lhs.మొదటి, rhs.మొదటి);
            });
            std::సదిశ<std::pair<V3_సంఖ్య, V3_సంఖ్య>> merged;
            కొరకు (స్థిరరాశి std::pair<V3_సంఖ్య, V3_సంఖ్య>& నిరంతరం : runs) {
                ఒకవేళ (merged.ఖాళీలు() || cross_విలువ_Less(merged.వెనుక().రెండవ, నిరంతరం.మొదటి)) {
                    merged.నెట్టు_వెనుక(నిరంతరం);
                } లేకపోతే ఒకవేళ (cross_విలువ_Less(merged.వెనుక().రెండవ, నిరంతరం.రెండవ)) {
                    merged.వెనుక().రెండవ = నిరంతరం.రెండవ;
                }
            }
            runs = std::move(merged);
        }
        uint64_t లెక్క = 0;
        కొరకు (స్థిరరాశి std::pair<V3_సంఖ్య, V3_సంఖ్య>& నిరంతరం : runs) {
            V3_సంఖ్య పరిధి{exprp, వెడల్పు};
            పరిధి.opSub(నిరంతరం.రెండవ, నిరంతరం.మొదటి);
            ఒకవేళ (పరిధి.most_సమితి_బిట్టు_P1() > 32) తిరిగి నిజం;  // More than 2**32 values
            లెక్క += పరిధి.గమ్యం_UQuad() + 1;
            ఒకవేళ (లెక్క > (uint64_t{1} << 32)) తిరిగి నిజం;
        }
        తిరిగి అబద్ధం;
    }

    // The ranges of values the wildcard patterns of a sized wildcard array, or of a 'with'
    // filter's candidates, give, counted up to more than --coverage-max-bins
    స్థిర కొలత_t sizedWildcardRuns(స్థిరరాశి AstCoverBin* binp, Ast_కణుపు_సమాసం* exprp) {
        std::సదిశ<std::pair<V3_సంఖ్య, V3_సంఖ్య>> runs;
        కొరకు (Ast_కణుపు* rangep = binRangesp(binp); rangep; rangep = rangep->nextp()) {
            ఒకవేళ (!VN_IS(rangep, Const)) కొనసాగు;  // A range, or a value known at construction
            Cross_విలువ_Range range{rangep, resolve_వెడల్పు(rangep, exprp)};
            ఒకవేళ (resolve_విలువ(rangep, exprp, నిజం, నిజం, range) && !cross_Range_ఖాళీలు(range)) {
                crossRangeRuns(range, v3Global.opt.coverageMaxBins(), runs);
            }
        }
        తిరిగి runs.కొలత();
    }

    // Check the automatic bins declarations of a coverpoint.  Each stays one declaration, which
    // generates as a partition of the coverpoint domain (see autoBinRuns).
    శూన్య checkAutomaticBins(AstCoverpoint* coverpointp, స్థిరరాశి Ast_కణుపు_సమాసం* exprp) {
        కొరకు (Ast_కణుపు* binp = coverpointp->binsp(); binp; binp = binp->nextp()) {
            AstCoverBin* స్థిరరాశి cbinp = VN_AS(binp, CoverBin);
            ఒకవేళ (cbinp->binsType() != VCoverBinsType::BINS_AUTO) కొనసాగు;
            స్థిరరాశి AstConst* స్థిరరాశి constp = VN_CAST(cbinp->arraySizep(), Const);
            ఒకవేళ (!constp) {
                cbinp->v3error("Automatic bins array size must be a constant");
            } లేకపోతే ఒకవేళ (const_లెక్క(constp) < 1) {
                cbinp->v3error("Automatic bins array size must be >= 1, got "
                               << constp->అంకె().గమ్యం_Decimal_S());
            } లేకపోతే ఒకవేళ (const_లెక్క(constp) > v3Global.opt.coverageMaxBins()) {
                cbinp->v3error("Automatic bins array size of "
                               << constp->అంకె().గమ్యం_Decimal_U() << " exceeds limit of "
                               << v3Global.opt.coverageMaxBins() << '\n'
                               << cbinp->warn_మరిన్ని() << "... Suggest a larger --coverage-max-bins");
            } లేకపోతే ఒకవేళ (!exprp->dtypep()->skipRefp()->isIntegralOrPacked()) {
                cbinp->v3error("Automatic bins are not allowed on a coverpoint of a non-integral "
                               "expression (IEEE 1800-2023 19.5.3).");
            }
        }
    }

    // Extract all option values of a coverpoint or cross in a single pass.
    // atLeastOut: option.at_least (default the covergroup's)
    // autoBinMaxOut: option.auto_bin_max (coverpoint overrides covergroup, default 64)
    శూన్య వెలికితీయి_అంశం_Options(Ast_కణుపు* optionsp, పూర్ణాంకం& at_కనిష్ఠ_బహిర్గతం, పూర్ణాంకం& auto_Bin_Max_బహిర్గతం) {
        at_కనిష్ఠ_బహిర్గతం = m_cg_At_కనిష్ఠ;
        auto_Bin_Max_బహిర్గతం = -1;  // -1 = not set at coverpoint level
        కొరకు (Ast_కణుపు* optionp = optionsp; optionp; optionp = optionp->nextp()) {
            AstCoverOption* స్థిరరాశి optp = VN_AS(optionp, CoverOption);
            // Weights may be non-constant; generateItemWeight() handles them.  A cross keeps
            // its unsupported options, which V3LinkParse warned of.
            ఒకవేళ (!(optp->optType() == VCoverOptionType::AT_కనిష్ఠ
                  || optp->optType() == VCoverOptionType::AUTO_BIN_MAX)) {
                కొనసాగు;
            }
            స్థిరరాశి AstConst* స్థిరరాశి constp = constOption(optp->valuep(), optp->optType());
            ఒకవేళ (!constp) కొనసాగు;
            ఒకవేళ (optp->optType() == VCoverOptionType::AT_కనిష్ఠ) {
                at_కనిష్ఠ_బహిర్గతం = const_At_కనిష్ఠ(constp);
            } లేకపోతే {
                auto_Bin_Max_బహిర్గతం = constp->గమ్యం_SInt();
            }
        }
        // Fall back to covergroup-level auto_bin_max if not set at coverpoint level
        ఒకవేళ (auto_Bin_Max_బహిర్గతం < 0) {
            ఒకవేళ (m_covergroupp->cgAutoBinMax() >= 0) {
                auto_Bin_Max_బహిర్గతం = m_covergroupp->cgAutoBinMax();
            } లేకపోతే {
                auto_Bin_Max_బహిర్గతం = 64;  // Default per IEEE 1800-2023 Table 19-1
            }
        }
    }

    // IEEE 1800-2023 19.5.2: an enum coverpoint has one automatic bin per enumeration value
    శూన్య createEnumAutoBins(AstCoverpoint* coverpointp, Ast_కణుపు_సమాసం* exprp,
                            స్థిరరాశి AstEnumDType* enump) {
        File_పంక్తి* స్థిరరాశి fl = coverpointp->fileline();
        కొరకు (స్థిరరాశి Ast_Enum_అంశం* itemp = enump->itemsp(); itemp;
             itemp = VN_AS(itemp->nextp(), Enum_అంశం)) {
            AstConst* స్థిరరాశి lop = new_విలువ_Const(fl, VN_AS(itemp->valuep(), Const)->అంకె(), exprp);
            AstInsideRange* స్థిరరాశి rangep = కొత్త AstInsideRange{fl, lop, lop->clone_వృక్షం(అబద్ధం)};
            rangep->dtypeFrom(exprp);
            coverpointp->కూడు_Binsp(
                కొత్త AstCoverBin{fl, "auto[" + itemp->name() + "]", rangep, అబద్ధం, అబద్ధం});
        }
    }

    // IEEE 1800-2023 19.5.3/19.11.1: partition first, then apply exclusions.  The partition is one
    // automatic bins declaration, generated as a run like 'bins auto[N]' but numbering its bins.
    శూన్య createImplicitAutoBins(AstCoverpoint* coverpointp, Ast_కణుపు_సమాసం* exprp, పూర్ణాంకం autoBinMax) {
        ఒకవేళ (coverpointp->user2()) తిరిగి;  // Declared bins, ignored, leave no bins
        కొరకు (Ast_కణుపు* nodep = coverpointp->binsp(); nodep; nodep = nodep->nextp()) {
            స్థిరరాశి VCoverBinsType రకం = VN_AS(nodep, CoverBin)->binsType();
            ఒకవేళ (రకం != VCoverBinsType::BINS_IGNORE && రకం != VCoverBinsType::BINS_ILLEGAL)
                తిరిగి;
        }
        ఒకవేళ (స్థిరరాశి AstEnumDType* స్థిరరాశి enump
            = VN_CAST(exprp->dtypep()->skip_Ref_గమ్యం_Enump(), EnumDType)) {
            createEnumAutoBins(coverpointp, exprp, enump);
            తిరిగి;
        }
        స్థిరరాశి పూర్ణాంకం వెడల్పు = exprp->వెడల్పు();
        uint32_t లెక్క = వెడల్పు < 31 ? std::min<uint32_t>(uint32_t{1} << వెడల్పు, autoBinMax)
                                    : static_cast<uint32_t>(autoBinMax);
        ఒకవేళ (!లెక్క) తిరిగి;
        ఒకవేళ (!exprp->dtypep()->skipRefp()->isIntegralOrPacked()) {
            coverpointp->v3error("Coverpoint of a non-integral expression requires explicit bins "
                                 "(IEEE 1800-2023 19.5.3).");
            తిరిగి;
        }
        ఒకవేళ (లెక్క > v3Global.opt.coverageMaxBins()) {
            coverpointp->v3warn(COVERIGN, "Unsupported: more than "
                                              << v3Global.opt.coverageMaxBins()
                                              << " automatic bins from 'option.auto_bin_max'; "
                                                 "using "
                                              << v3Global.opt.coverageMaxBins() << ".\n"
                                              << coverpointp->warn_మరిన్ని()
                                              << "... Suggest a larger --coverage-max-bins");
            లెక్క = v3Global.opt.coverageMaxBins();
        }
        File_పంక్తి* స్థిరరాశి fl = coverpointp->fileline();
        coverpointp->కూడు_Binsp(కొత్త AstCoverBin{fl, "auto", కొత్త AstConst{fl, లెక్క},
                                              VCoverBinsType::BINS_AUTO_IMPLICIT});
    }

    // Sanitize generated names to be valid C++ identifiers
    స్థిర string sanitizeGeneratedName(string name) {
        std::replace(name.begin(), name.చివర(), '[', '_');
        std::replace(name.begin(), name.చివర(), ']', '_');
        తిరిగి name;
    }

    // Capture an iff guard in a function-local temporary so it is evaluated once per sample()
    AstVarRef* capture_Iff_గమ్యం_తాత్కాలిక(Ast_కణుపు_సమాసం* iffp, స్థిరరాశి string& తాత్కాలిక_Name) {
        File_పంక్తి* స్థిరరాశి fl = iffp->fileline();
        AstVar* స్థిరరాశి iffVarp
            = కొత్త AstVar{fl, VVarType::BLOCKTEMP, తాత్కాలిక_Name, iffp->కనుగొను_బిట్టు_DType()};
        iffVarp->funcLocal(నిజం);
        m_sampleFuncp->కూడు_Stmtsp(iffVarp);
        iffp->unlink_Fr_వెనుక();
        m_sampleFuncp->కూడు_Stmtsp(
            కొత్త Ast_కేటాయించు{fl, కొత్త AstVarRef{fl, iffVarp, VAccess::WRITE}, iffp});
        తిరిగి కొత్త AstVarRef{fl, iffVarp, VAccess::READ};
    }

    // Add a statement sampling the current coverpoint to sample(), under its iff guard if any
    శూన్య కూడు_Sample_Stmt(Ast_కణుపు* stmtp) {
        ఒకవేళ (m_cpGuardp) {
            m_cpGuardp->కూడు_Thensp(stmtp);
        } లేకపోతే {
            m_sampleFuncp->కూడు_Stmtsp(stmtp);
        }
    }

    // Create previous value variable for transition tracking
    AstVar* create_Prev_విలువ_Var(AstCoverpoint* coverpointp, Ast_కణుపు_సమాసం* exprp) {
        // Check if already created
        ఒకవేళ (AstVar* స్థిరరాశి prevVarp = VN_CAST(coverpointp->user1p(), Var)) తిరిగి prevVarp;

        // Create variable to store previous sampled value
        స్థిరరాశి string varName = "__Vprev_" + coverpointp->name();
        AstVar* prevVarp
            = కొత్త AstVar{coverpointp->fileline(), VVarType::MEMBER, varName, exprp->dtypep()};
        m_covergroupp->కూడు_Membersp(prevVarp);

        UINFO(4, "    Created previous value variable: " << varName);

        // Initialize to zero in constructor
        Ast_కణుపు_సమాసం* స్థిరరాశి initExprp
            = కొత్త AstConst{prevVarp->fileline(), AstConst::Widthed_విలువ{}, prevVarp->వెడల్పు(), 0};
        Ast_కణుపు_Stmt* స్థిరరాశి initStmtp = కొత్త Ast_కేటాయించు{
            prevVarp->fileline(), కొత్త AstVarRef{prevVarp->fileline(), prevVarp, VAccess::WRITE},
            initExprp};
        m_constructorp->కూడు_Stmtsp(initStmtp);

        coverpointp->user1p(prevVarp);
        తిరిగి prevVarp;
    }

    // Create state position variable for multi-value transition bins
    // Tracks position in sequence: 0=not started, 1=seen first item, etc.
    AstVar* createSequenceStateVar(AstCoverpoint* coverpointp, AstCoverBin* binp) {
        // Create variable to track sequence position
        స్థిరరాశి string varName = "__Vseqpos_" + coverpointp->name() + "_" + binp->name();
        // Use 8-bit integer for state position (sequences rarely > 255 items)
        AstVar* stateVarp
            = కొత్త AstVar{binp->fileline(), VVarType::MEMBER, varName, VFlagLogicPacked{}, 8};
        m_covergroupp->కూడు_Membersp(stateVarp);

        UINFO(4, "    Created sequence state variable: " << varName);

        // Initialize to 0 (not started) in constructor
        Ast_కణుపు_Stmt* స్థిరరాశి initStmtp = కొత్త Ast_కేటాయించు{
            stateVarp->fileline(), కొత్త AstVarRef{stateVarp->fileline(), stateVarp, VAccess::WRITE},
            కొత్త AstConst{stateVarp->fileline(), AstConst::Widthed_విలువ{}, 8, 0}};
        m_constructorp->కూడు_Stmtsp(initStmtp);

        తిరిగి stateVarp;
    }

    శూన్య generate_Coverpoint_సంకేతం(AstCoverpoint* coverpointp) {
        UINFO(4, "  Generating code for coverpoint: " << coverpointp->name());

        // Get the coverpoint expression
        Ast_కణుపు_సమాసం* exprp = coverpointp->exprp();

        // Check automatic bins before processing
        checkAutomaticBins(coverpointp, exprp);

        // Extract all coverpoint options in a single pass
        పూర్ణాంకం at_కనిష్ఠ_విలువ;
        పూర్ణాంకం autoBinMax;
        వెలికితీయి_అంశం_Options(coverpointp->optionsp(), at_కనిష్ఠ_విలువ, autoBinMax);
        UINFO(6, "    Coverpoint at_least = " << at_కనిష్ఠ_విలువ << " auto_bin_max = " << autoBinMax);

        // Create implicit automatic bins if no regular bins exist
        createImplicitAutoBins(coverpointp, exprp, autoBinMax);

        // A false iff guard disables sampling of the coverpoint (IEEE 1800-2023 19.5), so
        // neither is its expression evaluated, nor are its bins or transition state updated
        VL_RESTORER(m_cpGuardp);
        ఒకవేళ (Ast_కణుపు_సమాసం* స్థిరరాశి iffp = coverpointp->iffp()) {
            m_cpGuardp = కొత్త AstIf{iffp->fileline(), iffp->unlink_Fr_వెనుక()};
        }

        AstVar* స్థిరరాశి విలువ_Varp = కొత్త AstVar{
            coverpointp->fileline(), VVarType::BLOCKTEMP,
            "__VcpValue_" + sanitizeGeneratedName(coverpointp->name()), exprp->dtypep()};
        విలువ_Varp->funcLocal(నిజం);
        m_sampleFuncp->కూడు_Stmtsp(విలువ_Varp);
        exprp->unlink_Fr_వెనుక();
        కూడు_Sample_Stmt(కొత్త Ast_కేటాయించు{
            coverpointp->fileline(),
            కొత్త AstVarRef{coverpointp->fileline(), విలువ_Varp, VAccess::WRITE}, exprp});
        coverpointp->exprp(కొత్త AstVarRef{coverpointp->fileline(), విలువ_Varp, VAccess::READ});
        exprp = coverpointp->exprp();

        // Every coverpoint routes through the VlCoverpoint runtime.  Transition coverpoints are
        // included: their per-value matching is still generated as a state machine in sample()
        // (see generateCoverpoint), but the bin hit is recorded in the runtime bin
        // rather than a bare counter.
        generateCoverpoint(coverpointp, exprp, at_కనిష్ఠ_విలువ);
        // Sampling follows what generateCoverpoint added unguarded, such as clearing the hit list
        ఒకవేళ (m_cpGuardp) m_sampleFuncp->కూడు_Stmtsp(m_cpGuardp);
    }

    // Build the condition under which a default bin matches: NOT(OR of all normal bins).
    Ast_కణుపు_సమాసం* buildDefaultCondition(AstCoverpoint* coverpointp, Ast_కణుపు_సమాసం* exprp,
                                       File_పంక్తి* fl) {
        Ast_కణుపు_సమాసం* anyBinMatchp = శూన్యసూచిక;
        కొరకు (Ast_కణుపు* binp = coverpointp->binsp(); binp; binp = binp->nextp()) {
            AstCoverBin* స్థిరరాశి cbinp = VN_AS(binp, CoverBin);
            ఒకవేళ (cbinp->binsType() == VCoverBinsType::BINS_DEFAULT
                || cbinp->binsType() == VCoverBinsType::BINS_IGNORE
                || cbinp->binsType() == VCoverBinsType::BINS_ILLEGAL)
                కొనసాగు;
            // The values of constructed bins are known at construction; see emitSizedSample
            ఒకవేళ (isConstructedBins(cbinp)) కొనసాగు;
            ఒకవేళ (isAutoBins(cbinp)) {
                // Automatic bins partition the whole domain, leaving no default value
                ఒకవేళ (anyBinMatchp) VL_DO_DANGLING(నెట్టు_Deletep(anyBinMatchp), anyBinMatchp);
                తిరిగి కొత్త AstConst{fl, AstConst::బిట్టు_False{}};
            }
            Ast_కణుపు_సమాసం* స్థిరరాశి binCondp = buildBinCondition(cbinp, exprp);
            UASSERT_OBJ(binCondp, cbinp,
                        "buildBinCondition returned nullptr for non-ignore/non-illegal bin");
            anyBinMatchp = anyBinMatchp ? కొత్త AstOr{fl, anyBinMatchp, binCondp} : binCondp;
        }
        తిరిగి anyBinMatchp ? static_cast<Ast_కణుపు_సమాసం*>(కొత్త AstNot{fl, anyBinMatchp})
                            : static_cast<Ast_కణుపు_సమాసం*>(కొత్త AstConst{fl, AstConst::బిట్టు_True{}});
    }

    //====================================================================
    // VlCoverpoint conversion

    స్థిర తార్కిక coverpointHasStateExclusions(స్థిరరాశి AstCoverpoint* coverpointp) {
        కొరకు (స్థిరరాశి Ast_కణుపు* nodep = coverpointp->binsp(); nodep; nodep = nodep->nextp()) {
            స్థిరరాశి AstCoverBin* స్థిరరాశి binp = VN_AS(nodep, CoverBin);
            ఒకవేళ (!binp->transp() && binp->rangesp()
                && (binp->binsType() == VCoverBinsType::BINS_IGNORE
                    || binp->binsType() == VCoverBinsType::BINS_ILLEGAL)) {
                తిరిగి నిజం;
            }
        }
        తిరిగి అబద్ధం;
    }

    స్థిర తార్కిక coverpointHasConstructedBins(స్థిరరాశి AstCoverpoint* coverpointp) {
        కొరకు (స్థిరరాశి Ast_కణుపు* nodep = coverpointp->binsp(); nodep; nodep = nodep->nextp()) {
            ఒకవేళ (isConstructedBins(VN_AS(nodep, CoverBin))) తిరిగి నిజం;
        }
        తిరిగి అబద్ధం;
    }

    // True if a Normal state bin, or an array-bin element, has no value of the coverpoint's
    // type (IEEE 1800-2023 19.5.7).  Array ranges enumerate in-type values, so cannot vanish.
    స్థిర తార్కిక coverpoint_Has_ఖాళీలు_Bins(స్థిరరాశి AstCoverpoint* coverpointp) {
        Ast_కణుపు_సమాసం* స్థిరరాశి exprp = coverpointp->exprp();
        కొరకు (Ast_కణుపు* nodep = coverpointp->binsp(); nodep; nodep = nodep->nextp()) {
            స్థిరరాశి AstCoverBin* స్థిరరాశి binp = VN_AS(nodep, CoverBin);
            ఒకవేళ (!binp->binsType().bin_Is_సాధారణ() || binp->transp() || !binp->rangesp()) కొనసాగు;
            // A wildcard array has a bin for each value its elements match, so none empty, and
            // the constructor creates no bin without values of a 'with' filter
            ఒకవేళ ((binp->isArray() && binp->isWildcard()) || binWith(binp)) కొనసాగు;
            తార్కిక ఖాళీలు = నిజం;
            కొరకు (Ast_కణుపు* valuep = binp->rangesp(); valuep; valuep = valuep->nextp()) {
                ఒకవేళ (binp->isArray() && VN_IS(valuep, InsideRange)) కొనసాగు;
                Cross_విలువ_Range range{valuep, resolve_వెడల్పు(valuep, exprp)};
                స్థిరరాశి తార్కిక none = resolve_విలువ(valuep, exprp, నిజం, binp->isWildcard(), range)
                                  && cross_Range_ఖాళీలు(range);
                ఒకవేళ (binp->isArray() && none) తిరిగి నిజం;
                ఖాళీలు &= none;
            }
            ఒకవేళ (ఖాళీలు && !binp->isArray()) తిరిగి నిజం;
        }
        తిరిగి అబద్ధం;
    }

    // True if a coverpoint has any transition bin.  Used to decide whether sample() emits the
    // end-of-sample previous-value update that transition matching needs.
    స్థిర తార్కిక coverpointHasTransition(AstCoverpoint* coverpointp) {
        కొరకు (Ast_కణుపు* binp = coverpointp->binsp(); binp; binp = binp->nextp()) {
            ఒకవేళ (VN_AS(binp, CoverBin)->transp()) తిరిగి నిజం;
        }
        తిరిగి అబద్ధం;
    }

    // The interned AstBasicDType for one of the covergroup runtime keywords.
    స్థిర AstBasicDType* basicDType(File_పంక్తి* fl, VBasicDTypeKwd kwd) {
        తిరిగి v3Global.rootp()->typeTablep()->కనుగొను_Basic_DType(fl, kwd);
    }

    // Get (or create) the coverpoint dtype for a hit-list bound, interned so each distinct bound
    // yields one node.
    AstCoverpointDType* coverpointDType(File_పంక్తి* fl, uint32_t తగిలింది_Bound) {
        AstCoverpointDType*& typep = m_cpDTypes[తగిలింది_Bound];
        ఒకవేళ (!typep) {
            typep = కొత్త AstCoverpointDType{fl, తగిలింది_Bound};
            v3Global.rootp()->typeTablep()->కూడు_Typesp(typep);
        }
        తిరిగి typep;
    }

    // The key of the covergroup's type in the coverage registry, which get_coverage() queries
    std::string covergroupProtectedName() స్థిరరాశి {
        తిరిగి VIdProtect::protect_పదాలు_If(m_covergroupName, v3Global.opt.protectIds());
    }

    // The name of the covergroup's type, which keys the coverage registry and database: as
    // $typename names it (IEEE 1800-2023 20.6.1), so apart for covergroups of distinct scopes and
    // specializations, and alike in each Verilator run of a hierarchical design.  As design units
    // of distinct libraries may share a name, one of a library other than the default is prefixed
    // by its library, as '%l' prints it (IEEE 1800-2023 33.4).
    std::string covergroupTypeName() స్థిరరాశి {
        UASSERT_OBJ(m_unitp, m_covergroupp, "Covergroup declared outside of a design unit");
        స్థిరరాశి std::string& libname = m_unitp->libname();
        స్థిరరాశి std::string name = m_covergroupp->dtypeName(నిజం);
        తిరిగి libname == "work" ? name : libname + "." + name;
    }

    // Emit the covergroup's instance handle member and the constructor statement that creates
    // its node in the per-context coverage registry.  Runs before any coverpoint or cross is
    // generated, so their runtimes can be added to the node as they are created.
    శూన్య generateInstanceAttach() {
        File_పంక్తి* స్థిరరాశి fl = m_covergroupp->fileline();
        // V3LinkParse synthesizes a 'new' for every covergroup; the item generators below already
        // rely on that, and this attach runs even for a covergroup with no coverpoints at all.
        UASSERT_OBJ(m_constructorp, m_covergroupp, "Covergroup missing synthesized constructor");
        m_cgInstVarp = కొత్త AstVar{fl, VVarType::MEMBER, "__Vcg_inst",
                                  basicDType(fl, VBasicDTypeKwd::COVERGROUP_INSTHANDLE)};
        m_covergroupp->కూడు_Membersp(m_cgInstVarp);

        m_constructorp->కూడు_Stmtsp(
            అంశం_Call(fl, m_cgInstVarp, VCMethod::COVERGROUP_ATTACH,
                     {ctext(fl, "vlSymsp->_vm_contextp__->covergroupRegistryp()"
                                "->newCovergroupInst("
                                    + quoted(covergroupProtectedName())
                                    + (m_cg_May_విలీనం ? ", true" : ", false") + ")")},
                     /*usePtr=*/అబద్ధం)
                ->makeStmt());
        // The node reads option.weight in place, so procedural assignments take effect
        AstCExpr* స్థిరరాశి బరువు_Addrp = కొత్త AstCExpr{fl, "&"};
        బరువు_Addrp->కూడు(newOptionSel(fl, optionVar(అబద్ధం), "weight", VAccess::READ));
        m_constructorp->కూడు_Stmtsp(అంశం_Call(fl, m_cgInstVarp, VCMethod::COVERGROUP_LEND_బరువు,
                                           {బరువు_Addrp, file_పంక్తి_Debug(fl)}, /*usePtr=*/అబద్ధం)
                                      ->makeStmt());
    }

    // A '__Vcg_inst.p()-><method>()' call on the covergroup's instance node
    AstCMethodHard* instanceCall(File_పంక్తి* fl, VCMethod పద్ధతి) {
        // '__Vcg_inst.p()' -- a value handle, so '.' not '->'
        AstCMethodHard* స్థిరరాశి instp
            = కొత్త AstCMethodHard{fl, memberRef(fl, m_cgInstVarp), VCMethod::COVERGROUP_INST_P};
        instp->usePtr(అబద్ధం);
        instp->dtype_సమితి_Void();  // Opaque receiver; only ever the 'fromp' of the call below
        AstCMethodHard* స్థిరరాశి callp = కొత్త AstCMethodHard{fl, instp, పద్ధతి};
        callp->usePtr(నిజం);
        తిరిగి callp;
    }

    // Emit 'this->__Vcp_x = this->__Vcg_inst.p()->addCoverpoint<K>();' (or addCross), which
    // creates the item runtime in the instance node and borrows a pointer to it.
    Ast_కేటాయించు* make_అంశం_Create(File_పంక్తి* fl, AstVar* అంశం_Varp, VCMethod పద్ధతి) {
        AstCMethodHard* స్థిరరాశి createp = instanceCall(fl, పద్ధతి);
        createp->dtypep(అంశం_Varp->dtypep());
        తిరిగి కొత్త Ast_కేటాయించు{fl, memberRef(fl, అంశం_Varp, VAccess::WRITE), createp};
    }

    // Constant bounds of one rangesp() element (an InsideRange or a single Const).  Each bound is
    // the raw AST node -- an AstConst or an AstUnbounded ('$') -- with the const/unbounded view
    // derived on demand, so there is one source of truth per bound.  After a successful
    // constRangeBounds() neither node is null; a single Const has both bounds aliasing one node.
    నిర్మాణం RangeBounds final {
        Ast_కణుపు* loNodep = శూన్యసూచిక;  // low bound: AstConst or AstUnbounded
        Ast_కణుపు* hiNodep = శూన్యసూచిక;  // high bound: AstConst or AstUnbounded
        తార్కిక loUnbounded() స్థిరరాశి { తిరిగి VN_IS(loNodep, Unbounded); }
        తార్కిక hiUnbounded() స్థిరరాశి { తిరిగి VN_IS(hiNodep, Unbounded); }
        AstConst* loConstp() స్థిరరాశి { తిరిగి VN_CAST(loNodep, Const); }
        AstConst* hiConstp() స్థిరరాశి { తిరిగి VN_CAST(hiNodep, Const); }
    };

    // Decode one rangesp() element into its constant bounds.  Returns false if rp is neither an
    // InsideRange nor a single Const, or if a present bound is non-constant or 4-state.  '$'
    // bounds are left as AstUnbounded (not resolved) -- the caller applies its own policy.
    // Centralizes the InsideRange/Const/Unbounded decode shared by the hit-list-bound paths.
    స్థిర తార్కిక constRangeBounds(Ast_కణుపు* rp, RangeBounds& rb) {
        ఒకవేళ (AstInsideRange* స్థిరరాశి irp = VN_CAST(rp, InsideRange)) {
            rb.loNodep = irp->lhsp();
            rb.hiNodep = irp->rhsp();
        } లేకపోతే ఒకవేళ (AstConst* స్థిరరాశి cp = VN_CAST(rp, Const)) {
            rb.loNodep = rb.hiNodep = cp;
        } లేకపోతే {
            తిరిగి అబద్ధం;
        }
        // Each bound must be a constant unless it is '$'; reject non-const and 4-state.
        AstConst* స్థిరరాశి lc = rb.loConstp();
        AstConst* స్థిరరాశి hc = rb.hiConstp();
        ఒకవేళ ((!lc && !rb.loUnbounded()) || (!hc && !rb.hiUnbounded())) తిరిగి అబద్ధం;
        ఒకవేళ ((lc && lc->అంకె().isFourState()) || (hc && hc->అంకె().isFourState())) తిరిగి అబద్ధం;
        తిరిగి నిజం;
    }

    // Collect the covered value intervals of a single (non-array) Normal bin.  Returns false
    // if any range isn't a constant/open InsideRange or single Const (e.g. wildcard, non-const).
    స్థిర తార్కిక వెలికితీయి_Range_Intervals(AstCoverBin* cbinp, uint64_t maxVal,
                                      std::సదిశ<std::pair<uint64_t, uint64_t>>& బహిర్గతం) {
        ఒకవేళ (!cbinp->rangesp()) తిరిగి అబద్ధం;
        కొరకు (Ast_కణుపు* rp = cbinp->rangesp(); rp; rp = rp->nextp()) {
            RangeBounds rb;
            ఒకవేళ (!constRangeBounds(rp, rb)) తిరిగి అబద్ధం;
            ఒకవేళ ((rb.loConstp() && rb.loConstp()->వెడల్పు() > 64)
                || (rb.hiConstp() && rb.hiConstp()->వెడల్పు() > 64)) {
                తిరిగి అబద్ధం;  // Use the safe slot-count bound for wide values.
            }
            స్థిరరాశి uint64_t lo = rb.loUnbounded() ? 0 : rb.loConstp()->గమ్యం_UQuad();
            స్థిరరాశి uint64_t hi = rb.hiUnbounded() ? maxVal : rb.hiConstp()->గమ్యం_UQuad();
            ఒకవేళ (lo > hi) తిరిగి అబద్ధం;
            బహిర్గతం.emplace_వెనుక(lo, hi);
        }
        తిరిగి నిజం;
    }

    // Append one Normal bin's cross-slot interval-sets to `bins` and bump `slotCount` by an upper
    // bound of the bin's slots holding one value.  Returns false if any part isn't statically
    // enumerable (the caller then falls back to the always-safe slot count).  A non-array bin
    // is one slot covering the union of its intervals; the bins of an array element or of an
    // automatic bins declaration hold disjoint values, so the element or declaration counts once.
    తార్కిక appendBinCrossSlots(AstCoverBin* cbinp, uint64_t maxVal, Ast_కణుపు_సమాసం* exprp,
                             std::సదిశ<std::సదిశ<std::pair<uint64_t, uint64_t>>>& bins,
                             పూర్ణాంకం& స్థానం_లెక్క) {
        ఒకవేళ (isAutoBins(cbinp)) {
            ++స్థానం_లెక్క;
            bins.నెట్టు_వెనుక({{0, maxVal}});
            తిరిగి exprp->వెడల్పు() <= 64;
        }
        ఒకవేళ (binWith(cbinp)) {
            // A value is in one bin of a filter's, but of a sized array, in one for each range
            // list element holding it; the coverpoint's name is one element
            పూర్ణాంకం elements = 0;
            కొరకు (స్థిరరాశి Ast_కణుపు* rp = binRangesp(cbinp); rp && isSizedArray(cbinp);
                 rp = rp->nextp()) {
                ++elements;
            }
            స్థానం_లెక్క += std::max(1, elements);
            తిరిగి అబద్ధం;
        }
        ఒకవేళ (cbinp->isArray() && cbinp->isWildcard() && !cbinp->arraySizep()) {
            // A value is in at most one bin of a wildcard array: one slot covering its values.
            // Signed values are sign-extended, not unsigned intervals (see computeHitListBound)
            ++స్థానం_లెక్క;
            ఒకవేళ (exprp->isSigned() || exprp->వెడల్పు() > 64) తిరిగి అబద్ధం;
            స్థిరరాశి BinRuns runs = wildcardBinRuns(cbinp, exprp, అబద్ధం);
            ఒకవేళ (runs.unsupported) తిరిగి అబద్ధం;
            std::సదిశ<std::pair<uint64_t, uint64_t>> ivs;
            కొరకు (స్థిరరాశి Bin_నిరంతరం& నిరంతరం : runs.runs) {
                ivs.emplace_వెనుక(నిరంతరం.m_lo.గమ్యం_UQuad(), నిరంతరం.m_hi.గమ్యం_UQuad());
            }
            bins.నెట్టు_వెనుక(std::move(ivs));
            తిరిగి నిజం;
        }
        ఒకవేళ (cbinp->isArray()) తిరిగి appendArrayBinCrossSlots(cbinp, exprp, bins, స్థానం_లెక్క);
        // Non-array bin: one slot covering the union of its intervals.
        ++స్థానం_లెక్క;
        std::సదిశ<std::pair<uint64_t, uint64_t>> ivs;
        ఒకవేళ (cbinp->isWildcard() || !వెలికితీయి_Range_Intervals(cbinp, maxVal, ivs)) తిరిగి అబద్ధం;
        bins.నెట్టు_వెనుక(std::move(ivs));
        తిరిగి నిజం;
    }

    // Append the cross slots of an array Normal bin: each element is a slot covering its
    // values, as at most one of its single-value bins holds a value.  An element holds the
    // values arrayBinRuns() gives it: those of the coverpoint type (IEEE 1800-2023 19.5.7).
    // Elements of a signed coverpoint, non-constant elements, and elements with values beyond
    // 64 bits can't be enumerated, so they count one slot but lose exactness.  Returns false if
    // any element wasn't enumerable to exact values.
    తార్కిక appendArrayBinCrossSlots(AstCoverBin* cbinp, Ast_కణుపు_సమాసం* exprp,
                                  std::సదిశ<std::సదిశ<std::pair<uint64_t, uint64_t>>>& bins,
                                  పూర్ణాంకం& స్థానం_లెక్క) {
        // Signed values resolve sign-extended, not as unsigned intervals, and wildcard patterns
        // are not intervals
        తార్కిక exact = !exprp->isSigned() && !cbinp->isWildcard();
        కొరకు (Ast_కణుపు* rp = cbinp->rangesp(); rp; rp = rp->nextp()) {
            ++స్థానం_లెక్క;
            RangeBounds rb;
            Cross_విలువ_Range range{rp, resolve_వెడల్పు(rp, exprp)};
            ఒకవేళ (!exact || !constRangeBounds(rp, rb)
                || !resolve_విలువ(rp, exprp, నిజం, అబద్ధం, range)) {
                exact = అబద్ధం;
                కొనసాగు;
            }
            ఒకవేళ (cross_Range_ఖాళీలు(range)) కొనసాగు;  // Its bin, if any, holds no value
            ఒకవేళ (range.hi.most_సమితి_బిట్టు_P1() > 64) {
                exact = అబద్ధం;
                కొనసాగు;
            }
            bins.నెట్టు_వెనుక({{range.lo.గమ్యం_UQuad(), range.hi.గమ్యం_UQuad()}});
        }
        తిరిగి exact;
    }

    // Compute the hit-list bound for a coverpoint: the maximum number of Normal
    // bins one sample value can match.  Non-cross-fed coverpoints don't feed a cross, so
    // their hit list is unused -> 1.  Otherwise compute the exact max bin overlap; fall back
    // to the (always-safe) Normal-slot count when any bin isn't statically analyzable.
    పూర్ణాంకం compute_తగిలింది_List_Bound(AstCoverpoint* coverpointp, Ast_కణుపు_సమాసం* exprp, తార్కిక crossFed) {
        ఒకవేళ (!crossFed) తిరిగి 1;
        స్థిరరాశి పూర్ణాంకం వెడల్పు = exprp->వెడల్పు();
        స్థిరరాశి uint64_t maxVal = (వెడల్పు >= 64) ? UINT64_MAX : ((1ULL << వెడల్పు) - 1);
        // One entry per Normal bin (cross slot): its covered intervals.
        std::సదిశ<std::సదిశ<std::pair<uint64_t, uint64_t>>> bins;
        పూర్ణాంకం స్థానం_లెక్క = 0;  // == runtime m_normal; the safe fallback bound
        // Unsigned intervals cannot establish overlap between differently sized signed values.
        తార్కిక exact = !exprp->isSigned();
        కొరకు (Ast_కణుపు* binp = coverpointp->binsp(); binp; binp = binp->nextp()) {
            AstCoverBin* స్థిరరాశి cbinp = VN_AS(binp, CoverBin);
            ఒకవేళ (!cbinp->binsType().bin_Is_సాధారణ())
                కొనసాగు;  // ignore/illegal/default: not hit-listed
            ఒకవేళ (!appendBinCrossSlots(cbinp, maxVal, exprp, bins, స్థానం_లెక్క)) exact = అబద్ధం;
        }
        ఒకవేళ (!exact) తిరిగి std::max(1, స్థానం_లెక్క);
        ఒకవేళ (bins.ఖాళీలు()) తిరిగి 1;
        // Max overlap occurs at some interval start; count covering bins at each lo.
        std::సదిశ<uint64_t> pts;
        కొరకు (స్థిరరాశి స్వయంచాలక& b : bins)
            కొరకు (స్థిరరాశి స్వయంచాలక& iv : b) pts.నెట్టు_వెనుక(iv.మొదటి);
        std::క్రమీకరించు(pts.begin(), pts.చివర());
        pts.erase(std::unique(pts.begin(), pts.చివర()), pts.చివర());
        పూర్ణాంకం maxOverlap = 1;
        కొరకు (స్థిరరాశి uint64_t p : pts) {
            పూర్ణాంకం cnt = 0;
            కొరకు (స్థిరరాశి స్వయంచాలక& b : bins) {
                కొరకు (స్థిరరాశి స్వయంచాలక& iv : b) {
                    ఒకవేళ (iv.మొదటి <= p && p <= iv.రెండవ) {
                        ++cnt;
                        ఆపు;  // count each bin at most once
                    }
                }
            }
            ఒకవేళ (cnt > maxOverlap) maxOverlap = cnt;
        }
        తిరిగి maxOverlap;
    }

    // A 'this->m_member' reference for embedding in an AstCStmt
    AstVarRef* memberRef(File_పంక్తి* fl, AstVar* varp, VAccess access = VAccess::READ) {
        AstVarRef* స్థిరరాశి refp = కొత్త AstVarRef{fl, varp, access};
        refp->self_సూచిక(VSelf_సూచిక_పాఠ్యం{VSelf_సూచిక_పాఠ్యం::This{}});
        తిరిగి refp;
    }

    // A 'this->m_member-><method>(args...)' call on one member.  usePtr is false only for
    // __Vcg_inst, which is a value handle; the item members are borrowed pointers into the
    // instance node.  Numeric arguments are AstConst; the rest are C++ text that has no AST
    // form (see ctext).
    AstCMethodHard* అంశం_Call(File_పంక్తి* fl, AstVar* varp, VCMethod పద్ధతి,
                             స్థిరరాశి std::సదిశ<Ast_కణుపు_సమాసం*>& వాదనలు = {}, తార్కిక usePtr = నిజం) {
        AstCMethodHard* స్థిరరాశి callp = కొత్త AstCMethodHard{fl, memberRef(fl, varp), పద్ధతి};
        కొరకు (Ast_కణుపు_సమాసం* స్థిరరాశి argp : వాదనలు) callp->కూడు_Pinsp(argp);
        callp->usePtr(usePtr);
        callp->dtype_సమితి_Void();
        తిరిగి callp;
    }

    // An unsigned integer argument.
    స్థిర AstConst* cnum(File_పంక్తి* fl, uint32_t విలువ) { తిరిగి కొత్త AstConst{fl, విలువ}; }

    // A literal C++ argument with no AST equivalent: a 'const char*' string literal (an SV
    // string AstConst emits '"..."s', a std::string temporary the runtime cannot borrow), a
    // VlCovBinKind enum token, a constant selection-word initializer list, a VlFileLineDebug, or
    // a '__V' temporary declared by the enclosing AstCStmt.
    స్థిర AstCExpr* ctext(File_పంక్తి* fl, స్థిరరాశి std::string& పాఠ్యం) {
        తిరిగి కొత్త AstCExpr{fl, పాఠ్యం};
    }

    // A C++ string literal.  Escapes control characters as the emitter does elsewhere -- bin
    // names and filenames reach the generated code verbatim when --protect-ids is off, and an
    // SV escaped identifier may hold a quote or backslash.
    స్థిర std::string quoted(స్థిరరాశి std::string& పాఠ్యం) {
        తిరిగి "\"" + V3_బహిర్గతం_Formatter::quoteNameControls(పాఠ్యం) + "\"";
    }

    // A 'VlFileLineDebug' argument: where the runtime reports an error about fl's construct
    స్థిర AstCExpr* file_పంక్తి_Debug(File_పంక్తి* fl) {
        స్థిరరాశి std::string filename
            = VIdProtect::protectIf(fl->filename(), v3Global.opt.protectIds());
        తిరిగి ctext(fl, "VlFileLineDebug{" + quoted(filename) + ", "
                             + std::గమ్యం_string(fl->lineno()) + "}");
    }

    // Check that an element of an array bin (bins b[] = {values/ranges}) is a two-state
    // constant value or range; false if not, after reporting it if 'report'.
    స్థిర తార్కిక checkArrayBinElement(AstCoverBin* arrayBinp, Ast_కణుపు* rangep, తార్కిక report = నిజం) {
        ఒకవేళ (స్థిరరాశి AstInsideRange* స్థిరరాశి irp = VN_CAST(rangep, InsideRange)) {
            స్థిరరాశి AstConst* స్థిరరాశి minp = VN_CAST(irp->lhsp(), Const);
            స్థిరరాశి AstConst* స్థిరరాశి maxp = VN_CAST(irp->rhsp(), Const);
            ఒకవేళ ((!minp && !VN_IS(irp->lhsp(), Unbounded))
                || (!maxp && !VN_IS(irp->rhsp(), Unbounded))) {
                ఒకవేళ (report) {
                    arrayBinp->v3error("Non-constant expression in array bins range; "
                                       "range bounds must be constants (IEEE 1800-2023 19.5)");
                }
                తిరిగి అబద్ధం;
            }
            ఒకవేళ ((minp && minp->అంకె().isFourState()) || (maxp && maxp->అంకె().isFourState())) {
                ఒకవేళ (report) {
                    arrayBinp->v3error("Four-state (x/z) value in array bins range bound; "
                                       "range bounds must be two-state constants");
                }
                తిరిగి అబద్ధం;
            }
        } లేకపోతే ఒకవేళ (!VN_IS(rangep, Const)) {
            ఒకవేళ (report) {
                arrayBinp->v3error("Non-constant expression in array bins value list; "
                                   "values must be constants (IEEE 1800-2023 19.5)");
            }
            తిరిగి అబద్ధం;
        }
        తిరిగి నిజం;
    }

    // Individual equality targets of an array bin (bins b[] = {values/ranges}) of a real
    // coverpoint, in order; integral coverpoints generate array bins as runs (see arrayBinRuns).
    // An open-ended bound ('$', AstUnbounded) resolves to the coverpoint domain: '[lo:$]'
    // covers [lo:maxVal] and '[$:hi]' covers [0:hi].  One target is produced per value; ranges
    // whose resolved size would exceed --coverage-max-real-bins (e.g. an open '[lo:$]') are
    // unsupported -- emits COVERIGN, sets unsupportedOut, yields nothing.
    std::సదిశ<Ast_కణుపు_సమాసం*> వెలికితీయి_Array_విలువలు(AstCoverBin* arrayBinp, Ast_కణుపు_సమాసం* exprp,
                                                 తార్కిక& unsupported_బహిర్గతం) {
        unsupported_బహిర్గతం = అబద్ధం;
        స్థిరరాశి పూర్ణాంకం వెడల్పు = exprp->వెడల్పు();
        స్థిరరాశి uint64_t maxVal = (వెడల్పు >= 64) ? UINT64_MAX : ((1ULL << వెడల్పు) - 1);
        std::సదిశ<Ast_కణుపు_సమాసం*> విలువలు;
        కొరకు (Ast_కణుపు* rangep = arrayBinp->rangesp(); rangep; rangep = rangep->nextp()) {
            rangep = V3Const::constifyEdit(rangep);
            ఒకవేళ (!checkArrayBinElement(arrayBinp, rangep)) తిరిగి విలువలు;
            ఒకవేళ (AstInsideRange* స్థిరరాశి irp = VN_CAST(rangep, InsideRange)) {
                స్థిరరాశి తార్కిక loUnb = VN_IS(irp->lhsp(), Unbounded);
                స్థిరరాశి తార్కిక hiUnb = VN_IS(irp->rhsp(), Unbounded);
                స్థిరరాశి uint64_t lo = loUnb ? 0 : VN_AS(irp->lhsp(), Const)->గమ్యం_UQuad();
                స్థిరరాశి uint64_t hi = hiUnb ? maxVal : VN_AS(irp->rhsp(), Const)->గమ్యం_UQuad();
                ఒకవేళ (hi < lo) కొనసాగు;  // empty range contributes no bins
                // Guard against a '$'-bounded or otherwise huge range exploding the bin count.
                స్థిరరాశి uint64_t పరిధి = hi - lo;  // == valueCount - 1 (no overflow: hi >= lo)
                ఒకవేళ (పరిధి >= v3Global.opt.coverageMaxRealBins()
                    || విలువలు.కొలత() + పరిధి + 1 > v3Global.opt.coverageMaxRealBins()) {
                    arrayBinp->v3warn(COVERIGN,
                                      "Unsupported: array 'bins' of a real coverpoint "
                                      "covering more than "
                                          << v3Global.opt.coverageMaxRealBins() << " values; bin "
                                          << arrayBinp->prettyNameQ() << " ignored.\n"
                                          << arrayBinp->warn_మరిన్ని()
                                          << "... Suggest a larger --coverage-max-real-bins");
                    unsupported_బహిర్గతం = నిజం;
                    కొరకు (Ast_కణుపు_సమాసం* స్థిరరాశి vp : విలువలు) VL_DO_DANGLING(నెట్టు_Deletep(vp), vp);
                    విలువలు.clear();
                    తిరిగి విలువలు;
                }
                కొరకు (uint64_t v = lo; v <= hi; ++v)
                    విలువలు.నెట్టు_వెనుక(కొత్త AstConst{irp->fileline(), AstConst::Widthed_విలువ{}, వెడల్పు,
                                                  static_cast<uint32_t>(v)});
            } లేకపోతే {
                విలువలు.నెట్టు_వెనుక(VN_AS(rangep->clone_వృక్షం(అబద్ధం), కణుపు_సమాసం));
            }
        }
        తిరిగి విలువలు;
    }

    స్థిర పూర్ణాంకం నిరంతరం_వెడల్పు(స్థిరరాశి Ast_కణుపు_సమాసం* exprp) { తిరిగి exprp->వెడల్పు() + 1; }

    // Automatic bins partition the coverpoint domain in value order (IEEE 1800-2023 19.5.3): N
    // bins, capped at the number of values, each hold 2^width / N values, and the last bin also
    // holds the remainder.  False for an invalid declaration, already reported.
    తార్కిక autoBinRuns(AstCoverBin* binp, Ast_కణుపు_సమాసం* exprp, BinRuns& బహిర్గతం) {
        స్థిరరాశి uint32_t requested = autoBinsRequested(binp);
        ఒకవేళ (!requested || !exprp->dtypep()->skipRefp()->isIntegralOrPacked()) తిరిగి అబద్ధం;
        స్థిరరాశి పూర్ణాంకం వెడల్పు = exprp->వెడల్పు();
        స్థిరరాశి పూర్ణాంకం arithmetic_వెడల్పు = నిరంతరం_వెడల్పు(exprp);
        స్థిరరాశి uint32_t లెక్క
            = వెడల్పు < 32
                  ? static_cast<uint32_t>(std::min<uint64_t>(uint64_t{1} << వెడల్పు, requested))
                  : requested;
        Bin_నిరంతరం నిరంతరం{binp, arithmetic_వెడల్పు, లెక్క};
        V3_సంఖ్య మొత్తం{binp, arithmetic_వెడల్పు};
        మొత్తం.సమితి_బిట్టు(వెడల్పు, 1);
        నిరంతరం.m_stride.opDiv(మొత్తం, V3_సంఖ్య{binp, arithmetic_వెడల్పు, లెక్క});
        స్థిరరాశి Cross_విలువ_Range domain
            = cross_విలువ_Domain(binp, వెడల్పు, exprp->isSigned(), arithmetic_వెడల్పు);
        నిరంతరం.m_lo = domain.lo;
        నిరంతరం.m_hi = domain.hi;
        బహిర్గతం.runs.నెట్టు_వెనుక(std::move(నిరంతరం));
        బహిర్గతం.లెక్క = లెక్క;
        తిరిగి నిజం;
    }

    // The elements of an array bin (bins b[] = {values/ranges}), in order, as runs of
    // single-value bins.  A range holds the values of the coverpoint type it contains (IEEE
    // 1800-2023 19.5.7), while a singleton names a bin even without such a value.  Errors on a
    // non-constant element.  More than --coverage-max-bins bins (e.g. an open '[lo:$]' range over
    // a wide coverpoint) are unsupported -- emits COVERIGN, and sets unsupported.
    BinRuns arrayBinRuns(AstCoverBin* arrayBinp, Ast_కణుపు_సమాసం* exprp) {
        BinRuns బహిర్గతం;
        స్థిరరాశి పూర్ణాంకం వెడల్పు = నిరంతరం_వెడల్పు(exprp);
        కొరకు (Ast_కణుపు* rangep = arrayBinp->rangesp(); rangep; rangep = rangep->nextp()) {
            rangep = V3Const::constifyEdit(rangep);
            ఒకవేళ (!checkArrayBinElement(arrayBinp, rangep)) తిరిగి బహిర్గతం;
            స్థిరరాశి AstInsideRange* స్థిరరాశి irp = VN_CAST(rangep, InsideRange);
            Cross_విలువ_Range range{rangep, resolve_వెడల్పు(rangep, exprp)};
            తార్కిక ఖాళీలు = నిజం;
            ఒకవేళ (!resolve_విలువ(rangep, exprp, నిజం, అబద్ధం, range)) {
                rangep->v3warn(E_UNSUPPORTED, "Unsupported: non-integral value in a coverage bin "
                                              "of an integral coverpoint.");
            } లేకపోతే {
                ఖాళీలు = cross_Range_ఖాళీలు(range);
            }
            ఒకవేళ (ఖాళీలు && irp) కొనసాగు;  // A range without values contributes no bins
            Bin_నిరంతరం నిరంతరం{rangep, వెడల్పు, 1};
            నిరంతరం.m_ఖాళీలు = ఖాళీలు;
            uint64_t లెక్క = 1;
            ఒకవేళ (!ఖాళీలు) {
                నిరంతరం.m_lo.op_కేటాయించు(range.lo);
                నిరంతరం.m_hi.op_కేటాయించు(range.hi);
                V3_సంఖ్య పరిధి{rangep, వెడల్పు};
                పరిధి.opSub(నిరంతరం.m_hi, నిరంతరం.m_lo);
                // Wider spans exceed any limit
                లెక్క = పరిధి.most_సమితి_బిట్టు_P1() > 32 ? UINT64_MAX : పరిధి.గమ్యం_UQuad() + 1;
            }
            ఒకవేళ (లెక్క > v3Global.opt.coverageMaxBins() - బహిర్గతం.లెక్క) {
                arrayBinp->v3warn(COVERIGN, "Unsupported: array 'bins' covering more than "
                                                << v3Global.opt.coverageMaxBins()
                                                << " values (e.g. an open '[lo:$]' range over "
                                                   "a wide coverpoint); bin "
                                                << arrayBinp->prettyNameQ() << " ignored\n"
                                                << arrayBinp->warn_మరిన్ని()
                                                << "... Suggest a larger --coverage-max-bins");
                బహిర్గతం.runs.clear();
                బహిర్గతం.లెక్క = 0;
                బహిర్గతం.unsupported = నిజం;
                తిరిగి బహిర్గతం;
            }
            నిరంతరం.m_లెక్క = static_cast<uint32_t>(లెక్క);
            బహిర్గతం.లెక్క += నిరంతరం.m_లెక్క;
            బహిర్గతం.runs.నెట్టు_వెనుక(std::move(నిరంతరం));
        }
        తిరిగి బహిర్గతం;
    }

    // The bins of a wildcard array (wildcard bins b[] = {...}): one for each coverpoint value
    // an element matches (IEEE 1800-2023 19.5.4, 19.5.7), in value order, and named by the value
    // (19.5.1), as runs of single-value bins.  Errors on a non-constant element.  More than
    // --coverage-max-bins values are unsupported -- emits COVERIGN, and sets unsupported, and for
    // an ignore or illegal array, single.  'report' false omits these diagnostics.
    స్థిర BinRuns wildcardBinRuns(AstCoverBin* arrayBinp, Ast_కణుపు_సమాసం* exprp, తార్కిక report) {
        BinRuns బహిర్గతం;
        స్థిరరాశి పూర్ణాంకం వెడల్పు = నిరంతరం_వెడల్పు(exprp);
        స్థిరరాశి V3_సంఖ్య one{arrayBinp, వెడల్పు, 1};
        // Disjoint runs of the values, in value order, each value once
        std::సదిశ<std::pair<V3_సంఖ్య, V3_సంఖ్య>> spans;
        uint64_t లెక్క = 0;  // Values of 'spans'
        కొరకు (Ast_కణుపు* rangep = arrayBinp->rangesp(); rangep; rangep = rangep->nextp()) {
            rangep = V3Const::constifyEdit(rangep);
            ఒకవేళ (!checkArrayBinElement(arrayBinp, rangep, report)) {
                బహిర్గతం.unsupported = నిజం;
                తిరిగి బహిర్గతం;
            }
            Cross_విలువ_Range range{rangep, resolve_వెడల్పు(rangep, exprp)};
            ఒకవేళ (!resolve_విలువ(rangep, exprp, నిజం, నిజం, range)) {
                ఒకవేళ (report) {
                    rangep->v3warn(E_UNSUPPORTED, "Unsupported: non-integral value in a "
                                                  "coverage bin of an integral coverpoint.");
                }
                కొనసాగు;
            }
            ఒకవేళ (cross_Range_ఖాళీలు(range)) కొనసాగు;
            std::సదిశ<std::pair<V3_సంఖ్య, V3_సంఖ్య>> దొరికింది;
            crossRangeRuns(range, v3Global.opt.coverageMaxBins(), దొరికింది);
            కొరకు (స్థిరరాశి std::pair<V3_సంఖ్య, V3_సంఖ్య>& నిరంతరం : దొరికింది) {
                // Coverpoint values, sign-extended in both widths
                spans.emplace_వెనుక(V3_సంఖ్య{rangep, వెడల్పు, నిరంతరం.మొదటి},
                                   V3_సంఖ్య{rangep, వెడల్పు, నిరంతరం.రెండవ});
            }
            std::క్రమీకరించు(spans.begin(), spans.చివర(), [](స్థిరరాశి స్వయంచాలక& lhs, స్థిరరాశి స్వయంచాలక& rhs) {
                తిరిగి cross_విలువ_Less(lhs.మొదటి, rhs.మొదటి);
            });
            std::సదిశ<std::pair<V3_సంఖ్య, V3_సంఖ్య>> merged;
            కొరకు (స్థిరరాశి std::pair<V3_సంఖ్య, V3_సంఖ్య>& పరిధి : spans) {
                // Adjacent or overlapping values join a run; 'first - 1' cannot overflow
                V3_సంఖ్య before{rangep, వెడల్పు};
                before.opSub(పరిధి.మొదటి, one);
                ఒకవేళ (merged.ఖాళీలు() || cross_విలువ_Less(merged.వెనుక().రెండవ, before)) {
                    merged.నెట్టు_వెనుక(పరిధి);
                } లేకపోతే ఒకవేళ (cross_విలువ_Less(merged.వెనుక().రెండవ, పరిధి.రెండవ)) {
                    merged.వెనుక().రెండవ = పరిధి.రెండవ;
                }
            }
            లెక్క = 0;
            కొరకు (స్థిరరాశి std::pair<V3_సంఖ్య, V3_సంఖ్య>& పరిధి : merged) {
                V3_సంఖ్య కొలత{rangep, వెడల్పు};
                కొలత.opSub(పరిధి.రెండవ, పరిధి.మొదటి);
                // Beyond 2^32 values exceed any limit
                లెక్క += కొలత.most_సమితి_బిట్టు_P1() > 32 ? uint64_t{1} << 33 : కొలత.గమ్యం_UQuad() + 1;
                ఒకవేళ (లెక్క > v3Global.opt.coverageMaxBins()) ఆపు;
            }
            spans = std::move(merged);
            ఒకవేళ (లెక్క > v3Global.opt.coverageMaxBins()) {
                // An ignore or illegal array still excludes or checks its values, as one bin
                బహిర్గతం.single = !arrayBinp->binsType().bin_Is_సాధారణ();
                ఒకవేళ (report) {
                    arrayBinp->v3warn(
                        COVERIGN, "Unsupported: wildcard array '"
                                      << arrayBinp->binsType().verilogKwd()
                                      << "' of more than --coverage-max-bins of "
                                      << v3Global.opt.coverageMaxBins() << " values; bin "
                                      << arrayBinp->prettyNameQ()
                                      << (బహిర్గతం.single ? " treated as one bin" : " ignored") << "\n"
                                      << arrayBinp->warn_మరిన్ని()
                                      << "... Suggest a larger --coverage-max-bins");
                }
                బహిర్గతం.unsupported = నిజం;
                తిరిగి బహిర్గతం;
            }
        }
        కొరకు (స్థిరరాశి std::pair<V3_సంఖ్య, V3_సంఖ్య>& పరిధి : spans) {
            బహిర్గతం.runs.emplace_వెనుక(arrayBinp, వెడల్పు, 0);
            Bin_నిరంతరం& నిరంతరం = బహిర్గతం.runs.వెనుక();
            నిరంతరం.m_lo = పరిధి.మొదటి;
            నిరంతరం.m_hi = పరిధి.రెండవ;
            // A bin for each value, named by it in the coverpoint's type
            V3_సంఖ్య విలువ = పరిధి.మొదటి;
            అయితే (నిజం) {
                స్థిరరాశి V3_సంఖ్య typed{arrayBinp, exprp->వెడల్పు(), విలువ};
                బహిర్గతం.విలువలు.నెట్టు_వెనుక(exprp->isSigned() ? typed.గమ్యం_Decimal_S() : typed.గమ్యం_Decimal_U());
                ++నిరంతరం.m_లెక్క;
                ఒకవేళ (విలువ.isCaseEq(పరిధి.రెండవ)) ఆపు;
                V3_సంఖ్య తదుపరి{arrayBinp, వెడల్పు};
                విలువ = తదుపరి.op_కూడు(విలువ, one);
            }
        }
        బహిర్గతం.లెక్క = static_cast<uint32_t>(లెక్క);
        తిరిగి బహిర్గతం;
    }

    // The runs of an automatic bins declaration, or of an array bin of an integral coverpoint.
    // False for other bins, which do not generate as runs, including a wildcard array then one
    // bin (see BinRuns::single).
    తార్కిక binRunsFor(AstCoverBin* binp, Ast_కణుపు_సమాసం* exprp, BinRuns& బహిర్గతం) {
        ఒకవేళ (isAutoBins(binp)) {
            ఒకవేళ (!autoBinRuns(binp, exprp, బహిర్గతం)) బహిర్గతం.unsupported = నిజం;
            తిరిగి నిజం;
        }
        ఒకవేళ (binp->isArray() && binp->isWildcard()) {
            ఒకవేళ (!exprp->dtypep()->skipRefp()->isIntegralOrPacked()) {
                Ast_కణుపు_సమాసం* స్థిరరాశి falsep = wildcardTypeError(binp, exprp);
                VL_DO_DANGLING(నెట్టు_Deletep(falsep), falsep);
                బహిర్గతం.unsupported = నిజం;
            } లేకపోతే {
                బహిర్గతం = wildcardBinRuns(binp, exprp, నిజం);
                ఒకవేళ (బహిర్గతం.single) {
                    binp->isArray(అబద్ధం);  // Generates as one bin
                    తిరిగి అబద్ధం;
                }
            }
            తిరిగి నిజం;
        }
        ఒకవేళ (!binp->isArray() || binp->transp() || binp->isWildcard()
            || !exprp->dtypep()->skipRefp()->isIntegralOrPacked()) {
            తిరిగి అబద్ధం;
        }
        బహిర్గతం = arrayBinRuns(binp, exprp);
        తిరిగి నిజం;
    }

    // Emit a 'this->m_cp->addSingleNamer/addArrayNamer(...)' statement for one bin whose first
    // runtime bin index is 'declared'; or with 'valueNames', those naming each bin of an array
    Ast_కణుపు_Stmt* makeNamer(AstVar* cpVarp, AstCoverBin* binp, int64_t లెక్క, uint32_t declared,
                           స్థిరరాశి std::సదిశ<Ast_కణుపు_సమాసం*>& విలువలు = {},
                           స్థిరరాశి std::సదిశ<std::string>& విలువ_పేర్లు = {}) {
        File_పంక్తి* స్థిరరాశి fl = binp->fileline();
        CoverpointBins& bins = m_cpBins.at(cpVarp);
        స్థిరరాశి uint32_t సాధారణ_లెక్క
            = binp->binsType().bin_Is_సాధారణ() ? static_cast<uint32_t>(లెక్క < 0 ? 1 : లెక్క) : 0;
        స్థిరరాశి Bin_పరిధి పరిధి{bins.మొత్తం, సాధారణ_లెక్క, declared};
        ఒకవేళ (binp->binsType() == VCoverBinsType::BINS_AUTO_IMPLICIT) {
            bins.implicitAuto = పరిధి;  // Selected by bin, see implicitAutoBinSpan
        } లేకపోతే {
            bins.spans.emplace(binp->name(), పరిధి);
        }
        bins.మొత్తం += సాధారణ_లెక్క;
        కొరకు (uint32_t i = 0; bins.crossed && i < సాధారణ_లెక్క; ++i) {
            bins.విలువలు.నెట్టు_వెనుక({binp, విలువలు.ఖాళీలు() ? శూన్యసూచిక : విలువలు[i]});
        }
        // Under --protect-ids the filename and bin name flow into the coverage database
        // verbatim, so obfuscate them exactly as line/toggle coverage points are (whole-
        // unit filename, per-word bin name).  A no-op when --protect-ids is off.
        స్థిరరాశి తార్కిక prot = v3Global.opt.protectIds();
        స్థిరరాశి తార్కిక single = లెక్క < 0;
        ఒకవేళ (!విలువ_పేర్లు.ఖాళీలు()) {
            // A bin of a wildcard array is named by its value (IEEE 1800-2023 19.5.1)
            Ast_కణుపు_Stmt* stmtsp = శూన్యసూచిక;
            కొరకు (స్థిరరాశి std::string& విలువ : విలువ_పేర్లు) {
                స్థిరరాశి std::string name
                    = VIdProtect::protect_పదాలు_If(binp->name(), prot) + "[" + విలువ + "]";
                stmtsp = Ast_కణుపు::కూడు_తదుపరి(
                    stmtsp,
                    అంశం_Call(fl, cpVarp, VCMethod::COVERGROUP_కూడు_SINGLE_NAMER,
                             {ctext(fl, binp->binsType().bin_సమితి_Enum()), ctext(fl, quoted(name)),
                              ctext(fl, quoted(VIdProtect::protectIf(fl->filename(), prot))),
                              cnum(fl, static_cast<uint32_t>(fl->lineno())),
                              cnum(fl, static_cast<uint32_t>(fl->మొదటి_స్తంభం()))})
                        ->makeStmt());
            }
            తిరిగి stmtsp;
        }
        std::సదిశ<Ast_కణుపు_సమాసం*> వాదనలు{ctext(fl, binp->binsType().bin_సమితి_Enum())};
        ఒకవేళ (!single) వాదనలు.నెట్టు_వెనుక(cnum(fl, static_cast<uint32_t>(లెక్క)));  // value array bin
        వాదనలు.నెట్టు_వెనుక(ctext(fl, quoted(VIdProtect::protect_పదాలు_If(binp->name(), prot))));
        వాదనలు.నెట్టు_వెనుక(ctext(fl, quoted(VIdProtect::protectIf(fl->filename(), prot))));
        వాదనలు.నెట్టు_వెనుక(cnum(fl, static_cast<uint32_t>(fl->lineno())));
        వాదనలు.నెట్టు_వెనుక(cnum(fl, static_cast<uint32_t>(fl->మొదటి_స్తంభం())));
        తిరిగి అంశం_Call(fl, cpVarp,
                        single ? VCMethod::COVERGROUP_కూడు_SINGLE_NAMER
                        : binp->binsType() == VCoverBinsType::BINS_AUTO_IMPLICIT
                            ? VCMethod::COVERGROUP_కూడు_NUMBERED_NAMER
                            : VCMethod::COVERGROUP_కూడు_ARRAY_NAMER,
                        వాదనలు)
            ->makeStmt();
    }

    // Emit 'if (iff && cond) m_cp.incrementBin(idx);' (or recordHit, + illegal action) in sample()
    // Where a bin's hit is recorded in the runtime VlCoverpoint member.
    నిర్మాణం Conv_Bin_లక్ష్యం final {
        AstVar* cpVarp;  // the __Vcp_<coverpoint> member
        uint32_t idx;  // bin index within that coverpoint
        తార్కిక is_సాధారణ;  // Normal -> incrementBin (count + cross hit list); else recordHit (count)
    };

    // Emit 'this->m_cp.incrementBin(idx);' (Normal) or '.recordHit(idx);'
    // (ignore/illegal/default).
    Ast_కణుపు_Stmt* make_Runtime_Bin_తగిలింది(File_పంక్తి* fl, AstVar* cpVarp, Ast_కణుపు_సమాసం* idxp,
                                   తార్కిక is_సాధారణ) {
        తిరిగి అంశం_Call(fl, cpVarp,
                        is_సాధారణ ? VCMethod::COVERGROUP_INCREMENT_BIN
                                 : VCMethod::COVERGROUP_రికార్డు_తగిలింది,
                        {idxp})
            ->makeStmt();
    }
    Ast_కణుపు_Stmt* make_Runtime_Bin_తగిలింది(File_పంక్తి* fl, స్థిరరాశి Conv_Bin_లక్ష్యం& tgt) {
        తిరిగి make_Runtime_Bin_తగిలింది(fl, tgt.cpVarp, cnum(fl, static_cast<uint32_t>(tgt.idx)),
                                 tgt.is_సాధారణ);
    }

    // The condition under which a bin counts a sample: condp, and the bin's iff, and for a
    // Normal or default state bin, that the value is not excluded
    Ast_కణుపు_సమాసం* binCondition(AstCoverBin* binp, AstVar* cpVarp, Ast_కణుపు_సమాసం* condp) {
        File_పంక్తి* స్థిరరాశి fl = binp->fileline();
        ఒకవేళ (binp->iffp()) condp = కొత్త AstLogAnd{fl, binp->iffp()->clone_వృక్షం(అబద్ధం), condp};
        స్థిరరాశి స్వయంచాలక excluded = m_excludedVars.కనుగొను(cpVarp);
        ఒకవేళ (excluded != m_excludedVars.చివర() && !binp->transp()
            && (binp->binsType().bin_Is_సాధారణ()
                || binp->binsType() == VCoverBinsType::BINS_DEFAULT)) {
            condp = కొత్త AstLogAnd{
                fl, కొత్త AstNot{fl, కొత్త AstVarRef{fl, excluded->రెండవ, VAccess::READ}}, condp};
        }
        తిరిగి condp;
    }

    శూన్య emit_Conv_తగిలింది_If(AstCoverpoint* coverpointp, AstCoverBin* binp, AstVar* cpVarp,
                       Ast_కణుపు_సమాసం* idxp, Ast_కణుపు_సమాసం* condp) {
        File_పంక్తి* స్థిరరాశి fl = binp->fileline();
        Ast_కణుపు* actionp = make_Runtime_Bin_తగిలింది(fl, cpVarp, idxp, binp->binsType().bin_Is_సాధారణ());
        ఒకవేళ (binp->binsType() == VCoverBinsType::BINS_ILLEGAL) {
            actionp->కూడు_తదుపరి(makeIllegalBinAction(fl, "Illegal bin " + binp->prettyNameQ()
                                                          + " hit in coverpoint "
                                                          + coverpointp->prettyNameQ()));
        }
        UASSERT_OBJ(m_sampleFuncp, binp, "sample() CFunc not set for coverpoint");
        కూడు_Sample_Stmt(కొత్త AstIf{fl, binCondition(binp, cpVarp, condp), actionp, శూన్యసూచిక});
    }

    // Emit the sample() code of sized array 'sized': count the value in its bins holding it when
    // enabled, as other bins are, and for default bins note in matchedp whether any holds it
    శూన్య emitSizedSample(AstCoverpoint* coverpointp, AstCoverBin* binp, AstVar* cpVarp,
                         Ast_కణుపు_సమాసం* exprp, uint32_t sized, AstVar* matchedp) {
        File_పంక్తి* స్థిరరాశి fl = binp->fileline();
        Ast_కణుపు_సమాసం* enabledp = binCondition(binp, cpVarp, కొత్త AstConst{fl, AstConst::బిట్టు_True{}});
        UASSERT_OBJ(m_sampleFuncp, binp, "sample() CFunc not set for coverpoint");
        స్థిరరాశి తార్కిక illegal = binp->binsType() == VCoverBinsType::BINS_ILLEGAL;
        ఒకవేళ (illegal) {
            // The illegal action reads the condition too, which is evaluated once, as the guard
            // may have side effects
            AstVar* స్థిరరాశి varp
                = కొత్త AstVar{fl, VVarType::BLOCKTEMP,
                             "__VcpEnabled_" + sanitizeGeneratedName(coverpointp->name()) + "_"
                                 + cvt_గమ్యం_Str(sized),
                             binp->కనుగొను_బిట్టు_DType()};
            varp->funcLocal(నిజం);
            m_sampleFuncp->కూడు_Stmtsp(varp);
            కూడు_Sample_Stmt(కొత్త Ast_కేటాయించు{fl, కొత్త AstVarRef{fl, varp, VAccess::WRITE}, enabledp});
            enabledp = కొత్త AstVarRef{fl, varp, VAccess::READ};
        }
        AstCMethodHard* స్థిరరాశి callp
            = అంశం_Call(fl, cpVarp,
                       exprp->isWide() ? VCMethod::COVERGROUP_SIZED_SAMPLE_W
                                       : VCMethod::COVERGROUP_SIZED_SAMPLE,
                       {cnum(fl, sized), exprp->clone_వృక్షం(అబద్ధం), enabledp});
        callp->dtype_సమితి_బిట్టు();
        ఒకవేళ (illegal) {
            కూడు_Sample_Stmt(కొత్త AstIf{fl, కొత్త AstLogAnd{fl, callp, enabledp->clone_వృక్షం(అబద్ధం)},
                                    makeIllegalBinAction(fl, "Illegal bin " + binp->prettyNameQ()
                                                                 + " hit in coverpoint "
                                                                 + coverpointp->prettyNameQ())});
        } లేకపోతే ఒకవేళ (matchedp && binp->binsType().bin_Is_సాధారణ()) {
            కూడు_Sample_Stmt(
                కొత్త Ast_కేటాయించు{fl, కొత్త AstVarRef{fl, matchedp, VAccess::WRITE},
                              కొత్త AstOr{fl, కొత్త AstVarRef{fl, matchedp, VAccess::READ}, callp}});
        } లేకపోతే {
            కూడు_Sample_Stmt(callp->makeStmt());
        }
    }

    // A variable local to the constructor
    AstVar* constructor_తాత్కాలిక(File_పంక్తి* fl, స్థిరరాశి string& name, Ast_కణుపు_DType* dtypep) {
        AstVar* స్థిరరాశి varp = కొత్త AstVar{fl, VVarType::BLOCKTEMP, name, dtypep};
        varp->funcLocal(నిజం);
        m_constructorp->కూడు_Stmtsp(varp);
        తిరిగి varp;
    }

    // Truncate or extend, as its signedness sets, a value to a type
    స్థిర Ast_కణుపు_సమాసం* resize_విలువ(Ast_కణుపు_సమాసం* valuep, Ast_కణుపు_DType* dtypep) {
        File_పంక్తి* స్థిరరాశి fl = valuep->fileline();
        ఒకవేళ (valuep->వెడల్పు() > dtypep->వెడల్పు()) {
            valuep = కొత్త AstSel{fl, valuep, 0, dtypep->వెడల్పు()};
        } లేకపోతే ఒకవేళ (valuep->వెడల్పు() < dtypep->వెడల్పు()) {
            valuep = valuep->isSigned()
                         ? static_cast<Ast_కణుపు_సమాసం*>(కొత్త AstExtendS{fl, valuep, dtypep->వెడల్పు()})
                         : కొత్త AstExtend{fl, valuep, dtypep->వెడల్పు()};
        }
        valuep->dtypep(dtypep);
        తిరిగి valuep;
    }

    // The runtime arguments of a count, which the constructor evaluates once into a temporary:
    // its value, a count wider than 64 bits saturated over any limit, and whether it is positive
    std::pair<Ast_కణుపు_సమాసం*, Ast_కణుపు_సమాసం*> new_లెక్క_వాదనలు(File_పంక్తి* fl, స్థిరరాశి string& name,
                                                       Ast_కణుపు_సమాసం* exprp) {
        AstVar* స్థిరరాశి countp = constructor_తాత్కాలిక(fl, name, exprp->dtypep());
        m_constructorp->కూడు_Stmtsp(
            కొత్త Ast_కేటాయించు{fl, కొత్త AstVarRef{fl, countp, VAccess::WRITE}, exprp->clone_వృక్షం(అబద్ధం)});
        స్థిరరాశి స్వయంచాలక లెక్క_Ref = [&]() { తిరిగి కొత్త AstVarRef{fl, countp, VAccess::READ}; };
        AstConst* స్థిరరాశి zerop = కొత్త AstConst{fl, AstConst::DTyped{}, countp->dtypep()};
        Ast_కణుపు_సమాసం* స్థిరరాశి positivep
            = countp->isSigned() ? static_cast<Ast_కణుపు_సమాసం*>(కొత్త AstGtS{fl, లెక్క_Ref(), zerop})
                                 : కొత్త AstNeq{fl, లెక్క_Ref(), zerop};
        Ast_కణుపు_సమాసం* valuep = resize_విలువ(లెక్క_Ref(), countp->కనుగొను_UInt64_DType());
        ఒకవేళ (countp->వెడల్పు() > VL_QUADSIZE) {
            valuep = కొత్త AstCond{
                fl,
                కొత్త AstRedOr{
                    fl, కొత్త AstSel{fl, లెక్క_Ref(), VL_QUADSIZE, countp->వెడల్పు() - VL_QUADSIZE}},
                కొత్త AstConst{fl, AstConst::Unsized64{}, std::numeric_limits<uint64_t>::max()},
                valuep};
            valuep->dtype_సమితి_UInt64();
        }
        తిరిగి {valuep, positivep};
    }
    // The runtime arguments of a constant count
    స్థిర std::pair<Ast_కణుపు_సమాసం*, Ast_కణుపు_సమాసం*> new_లెక్క_వాదనలు(File_పంక్తి* fl, uint64_t లెక్క) {
        తిరిగి {కొత్త AstConst{fl, AstConst::Unsized64{}, లెక్క},
                కొత్త AstConst{fl, AstConst::బిట్టు_True{}}};
    }

    // The values of a coverpoint, which its name denotes (IEEE 1800-2023 19.5.1.1), as runs in
    // value order: an enumerated type's values (6.19), else all values of its type.  Bounds
    // are at runWidth(), sign-extended like a CrossValueRange's.
    స్థిర std::సదిశ<std::pair<V3_సంఖ్య, V3_సంఖ్య>> coverpoint_విలువలు(Ast_కణుపు* nodep,
                                                                       Ast_కణుపు_సమాసం* exprp) {
        స్థిరరాశి పూర్ణాంకం వెడల్పు = నిరంతరం_వెడల్పు(exprp);
        std::సదిశ<std::pair<V3_సంఖ్య, V3_సంఖ్య>> runs;
        స్థిరరాశి AstEnumDType* స్థిరరాశి enump = VN_CAST(exprp->dtypep()->skip_Ref_గమ్యం_Enump(), EnumDType);
        ఒకవేళ (!enump) {
            స్థిరరాశి Cross_విలువ_Range domain
                = cross_విలువ_Domain(nodep, exprp->వెడల్పు(), exprp->isSigned(), వెడల్పు);
            runs.emplace_వెనుక(domain.lo, domain.hi);
            తిరిగి runs;
        }
        std::సదిశ<V3_సంఖ్య> విలువలు;
        కొరకు (స్థిరరాశి Ast_Enum_అంశం* itemp = enump->itemsp(); itemp;
             itemp = VN_AS(itemp->nextp(), Enum_అంశం)) {
            స్థిరరాశి V3_సంఖ్య& అంకె = VN_AS(itemp->valuep(), Const)->అంకె();
            ఒకవేళ (అంకె.isFourState()) కొనసాగు;  // Not a coverpoint value (19.5.7)
            విలువలు.emplace_వెనుక(nodep, వెడల్పు);
            ఒకవేళ (exprp->isSigned()) {
                విలువలు.వెనుక().opExtendS(అంకె, అంకె.వెడల్పు());
            } లేకపోతే {
                విలువలు.వెనుక().op_కేటాయించు(అంకె);
            }
        }
        std::క్రమీకరించు(విలువలు.begin(), విలువలు.చివర(), cross_విలువ_Less);
        స్థిరరాశి V3_సంఖ్య one{nodep, వెడల్పు, 1};
        కొరకు (స్థిరరాశి V3_సంఖ్య& విలువ : విలువలు) {
            V3_సంఖ్య తదుపరి{nodep, వెడల్పు};
            ఒకవేళ (!runs.ఖాళీలు() && తదుపరి.op_కూడు(runs.వెనుక().రెండవ, one).isCaseEq(విలువ)) {
                runs.వెనుక().రెండవ = విలువ;
            } లేకపోతే {
                runs.emplace_వెనుక(విలువ, విలువ);
            }
        }
        తిరిగి runs;
    }

    // Emit the constructor code building the bins 'binp' whose values it computes: the values of
    // each element that are coverpoint values (IEEE 1800-2023 19.5.7), those a 'with' filter
    // keeps (19.5.1.1), then its bins
    శూన్య generateConstructedBins(AstCoverpoint* coverpointp, AstCoverBin* binp, AstVar* cpVarp,
                                 Ast_కణుపు_సమాసం* exprp) {
        File_పంక్తి* స్థిరరాశి fl = binp->fileline();
        స్థిరరాశి string ఉపసర్గ
            = m_sized_పేర్లు.get(sanitizeGeneratedName(coverpointp->name() + "__" + binp->name()));
        // The count, and whether it is positive; one of a filter's scalar bin, or bin per value
        స్థిరరాశి std::pair<Ast_కణుపు_సమాసం*, Ast_కణుపు_సమాసం*> లెక్క
            = binp->arraySizep() ? new_లెక్క_వాదనలు(fl, ఉపసర్గ + "_count", binp->arraySizep())
                                 : new_లెక్క_వాదనలు(fl, 1);
        AstCoverWith* స్థిరరాశి withp = binWith(binp);
        ఒకవేళ (withp && !binRangesp(binp)) {  // The coverpoint's name: all of its values
            కొరకు (స్థిరరాశి std::pair<V3_సంఖ్య, V3_సంఖ్య>& నిరంతరం : coverpoint_విలువలు(binp, exprp)) {
                m_constructorp->కూడు_Stmtsp(అంశం_Call(fl, cpVarp,
                                                   exprp->isWide()
                                                       ? VCMethod::COVERGROUP_SIZED_RANGE_W
                                                       : VCMethod::COVERGROUP_SIZED_RANGE,
                                                   {new_విలువ_Const(fl, నిరంతరం.మొదటి, exprp),
                                                    new_విలువ_Const(fl, నిరంతరం.రెండవ, exprp)})
                                              ->makeStmt());
            }
        }
        uint32_t element = 0;
        కొరకు (Ast_కణుపు* rangep = binRangesp(binp); rangep; rangep = rangep->nextp()) {
            ఒకవేళ (VN_IS(rangep, Unbounded)) {  // A parameter of '$'
                binp->v3error("Bins value may not be '$', which may only bound a range "
                              "(IEEE 1800-2023 6.20.7)");
                కొనసాగు;
            }
            generateSizedElement(cpVarp, binp, rangep, exprp, ఉపసర్గ + "_" + cvt_గమ్యం_Str(element++));
        }
        ఒకవేళ (withp) generateWithFilter(binp, withp, cpVarp, exprp, ఉపసర్గ);
        std::సదిశ<Ast_కణుపు_సమాసం*> వాదనలు{ctext(fl, binp->binsType().bin_సమితి_Enum()), లెక్క.మొదటి,
                                       లెక్క.రెండవ};
        // A filter's bins have the limit it began with
        ఒకవేళ (!withp) వాదనలు.నెట్టు_వెనుక(cnum(fl, v3Global.opt.coverageMaxBins()));
        స్థిరరాశి తార్కిక prot = v3Global.opt.protectIds();
        వాదనలు.నెట్టు_వెనుక(ctext(fl, quoted(VIdProtect::protect_పదాలు_If(binp->name(), prot))));
        వాదనలు.నెట్టు_వెనుక(ctext(fl, quoted(VIdProtect::protectIf(fl->filename(), prot))));
        వాదనలు.నెట్టు_వెనుక(cnum(fl, static_cast<uint32_t>(fl->lineno())));
        వాదనలు.నెట్టు_వెనుక(cnum(fl, static_cast<uint32_t>(fl->మొదటి_స్తంభం())));
        m_constructorp->కూడు_Stmtsp(
            అంశం_Call(fl, cpVarp,
                     withp ? VCMethod::COVERGROUP_WITH_ముగింపు : VCMethod::COVERGROUP_SIZED_ముగింపు,
                     వాదనలు)
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
    శూన్య generateWithFilter(AstCoverBin* binp, AstCoverWith* withp, AstVar* cpVarp,
                            Ast_కణుపు_సమాసం* exprp, స్థిరరాశి string& ఉపసర్గ) {
        File_పంక్తి* స్థిరరాశి fl = withp->fileline();
        స్థిరరాశి తార్కిక wide = exprp->isWide();
        స్థిరరాశి string grouping = !binp->isArray()     ? "Single"
                                : binp->arraySizep() ? "Fixed"
                                                     : "Values";
        m_constructorp->కూడు_Stmtsp(అంశం_Call(fl, cpVarp, VCMethod::COVERGROUP_WITH_BEGIN,
                                           {ctext(fl, "VlCovBinGrouping::" + grouping),
                                            cnum(fl, v3Global.opt.coverageMaxBins())})
                                      ->makeStmt());
        // The candidates count in the coverpoint's width, and the filter reads each as 'item',
        // of the coverpoint's type, so that a filter changing 'item' cannot change the loop
        Ast_కణుపు_DType* స్థిరరాశి విలువ_DTypep = exprp->dtypep();
        AstVar* స్థిరరాశి valuep = constructor_తాత్కాలిక(fl, ఉపసర్గ + "_value", విలువ_DTypep);
        AstVar* స్థిరరాశి lastp = constructor_తాత్కాలిక(fl, ఉపసర్గ + "_last", విలువ_DTypep);
        AstVar* స్థిరరాశి firstp = constructor_తాత్కాలిక(fl, ఉపసర్గ + "_first", విలువ_DTypep);
        AstVar* స్థిరరాశి runp = constructor_తాత్కాలిక(fl, ఉపసర్గ + "_run", binp->కనుగొను_బిట్టు_DType());
        AstVar* స్థిరరాశి morep = constructor_తాత్కాలిక(fl, ఉపసర్గ + "_more", binp->కనుగొను_బిట్టు_DType());
        AstVar* స్థిరరాశి itemp = withp->itemp()->unlink_Fr_వెనుక();
        itemp->name(ఉపసర్గ + "_item");
        m_constructorp->కూడు_Stmtsp(itemp);
        స్థిరరాశి స్వయంచాలక ref = [&](AstVar* varp) { తిరిగి కొత్త AstVarRef{fl, varp, VAccess::READ}; };
        స్థిరరాశి స్వయంచాలక కేటాయించు_2 = [&](AstVar* varp, Ast_కణుపు_సమాసం* rhsp) -> Ast_కణుపు* {
            తిరిగి కొత్త Ast_కేటాయించు{fl, కొత్త AstVarRef{fl, varp, VAccess::WRITE}, rhsp};
        };
        స్థిరరాశి స్వయంచాలక flag = [&](AstVar* varp, తార్కిక విలువ) {
            తిరిగి కేటాయించు_2(varp, విలువ ? కొత్త AstConst{fl, AstConst::బిట్టు_True{}}
                                      : కొత్త AstConst{fl, AstConst::బిట్టు_False{}});
        };
        స్థిరరాశి స్వయంచాలక bound = [&](VCMethod narrow, VCMethod wide_పద్ధతి, AstVar* varp) -> Ast_కణుపు* {
            ఒకవేళ (wide) {
                తిరిగి అంశం_Call(fl, cpVarp, wide_పద్ధతి, {కొత్త AstVarRef{fl, varp, VAccess::WRITE}})
                    ->makeStmt();
            }
            AstCMethodHard* స్థిరరాశి callp = అంశం_Call(fl, cpVarp, narrow);
            callp->dtype_సమితి_UInt64();
            తిరిగి కేటాయించు_2(varp, resize_విలువ(callp, విలువ_DTypep));
        };
        స్థిరరాశి స్వయంచాలక keep = [&](Ast_కణుపు_సమాసం* lop, Ast_కణుపు_సమాసం* hip) {
            AstCMethodHard* స్థిరరాశి callp = అంశం_Call(
                fl, cpVarp, wide ? VCMethod::COVERGROUP_WITH_నిరంతరం_W : VCMethod::COVERGROUP_WITH_నిరంతరం,
                {lop, hip});
            callp->dtype_సమితి_బిట్టు();
            తిరిగి కేటాయించు_2(morep, callp);
        };
        స్థిరరాశి స్వయంచాలక అడుగు = [&](తార్కిక పైకి) {
            AstConst* స్థిరరాశి onep = కొత్త AstConst{fl, AstConst::Widthed_విలువ{}, exprp->వెడల్పు(), 1};
            Ast_కణుపు_సమాసం* స్థిరరాశి stepp
                = పైకి ? static_cast<Ast_కణుపు_సమాసం*>(కొత్త Ast_కూడు{fl, ref(valuep), onep})
                     : కొత్త AstSub{fl, ref(valuep), onep};
            stepp->dtypep(విలువ_DTypep);
            తిరిగి stepp;
        };
        AstLoop* స్థిరరాశి innerp = కొత్త AstLoop{fl};
        innerp->కూడు_Stmtsp(కేటాయించు_2(itemp, ref(valuep)));
        innerp->కూడు_Stmtsp(కొత్త AstIf{
            fl, withp->filterp()->unlink_Fr_వెనుక(),
            కొత్త AstIf{fl, కొత్త AstNot{fl, ref(runp)},
                      కేటాయించు_2(firstp, ref(valuep))->కూడు_తదుపరి(flag(runp, నిజం))},
            కొత్త AstIf{fl, ref(runp), keep(ref(firstp), అడుగు(అబద్ధం))->కూడు_తదుపరి(flag(runp, అబద్ధం))}});
        innerp->కూడు_Stmtsp(కొత్త AstLoopTest{
            fl, innerp, కొత్త AstLogAnd{fl, ref(morep), కొత్త AstNeq{fl, ref(valuep), ref(lastp)}}});
        innerp->కూడు_Stmtsp(కేటాయించు_2(valuep, అడుగు(నిజం)));
        AstLoop* స్థిరరాశి outerp = కొత్త AstLoop{fl};
        AstCMethodHard* స్థిరరాశి nextp = అంశం_Call(fl, cpVarp, VCMethod::COVERGROUP_WITH_తదుపరి);
        nextp->dtype_సమితి_బిట్టు();
        outerp->కూడు_Stmtsp(కొత్త AstLoopTest{fl, outerp, nextp});
        outerp->కూడు_Stmtsp(
            bound(VCMethod::COVERGROUP_WITH_LO, VCMethod::COVERGROUP_WITH_LO_W, valuep));
        outerp->కూడు_Stmtsp(
            bound(VCMethod::COVERGROUP_WITH_HI, VCMethod::COVERGROUP_WITH_HI_W, lastp));
        outerp->కూడు_Stmtsp(flag(runp, అబద్ధం));
        outerp->కూడు_Stmtsp(innerp);
        outerp->కూడు_Stmtsp(కొత్త AstIf{fl, ref(runp), keep(ref(firstp), ref(lastp))});
        m_constructorp->కూడు_Stmtsp(flag(morep, నిజం));
        m_constructorp->కూడు_Stmtsp(outerp);
    }

    // Emit 'sizedRange(lo, hi)' for the coverpoint values of an element of bins 'binp', whose
    // values the constructor computes: resolved now if constant, else when constructed by
    // clipping to the coverpoint's values.  A wildcard pattern's values give a range for each
    // run of them.  'prefix' names its temporaries.
    శూన్య generateSizedElement(AstVar* cpVarp, స్థిరరాశి AstCoverBin* binp, Ast_కణుపు* rangep,
                              Ast_కణుపు_సమాసం* exprp, స్థిరరాశి string& ఉపసర్గ) {
        File_పంక్తి* స్థిరరాశి fl = rangep->fileline();
        స్థిరరాశి VCMethod పద్ధతి = exprp->isWide() ? VCMethod::COVERGROUP_SIZED_RANGE_W
                                                : VCMethod::COVERGROUP_SIZED_RANGE;
        స్థిరరాశి AstInsideRange* స్థిరరాశి irp = VN_CAST(rangep, InsideRange);
        Ast_కణుపు_సమాసం* స్థిరరాశి lowp = irp ? irp->lhsp() : VN_AS(rangep, కణుపు_సమాసం);
        Ast_కణుపు_సమాసం* స్థిరరాశి highp = irp ? irp->rhsp() : శూన్యసూచిక;
        స్థిరరాశి స్వయంచాలక unbounded
            = [](స్థిరరాశి Ast_కణుపు_సమాసం* boundp) { తిరిగి !boundp || VN_IS(boundp, Unbounded); };
        స్థిరరాశి స్వయంచాలక constant
            = [&](స్థిరరాశి Ast_కణుపు_సమాసం* boundp) { తిరిగి unbounded(boundp) || VN_IS(boundp, Const); };
        స్థిరరాశి స్వయంచాలక integral = [&](స్థిరరాశి Ast_కణుపు_సమాసం* boundp) {
            తిరిగి unbounded(boundp) || boundp->dtypep()->skipRefp()->isIntegralOrPacked();
        };
        ఒకవేళ (constant(lowp) && constant(highp)) {
            స్థిరరాశి స్వయంచాలక fourState = [](స్థిరరాశి Ast_కణుపు_సమాసం* boundp) {
                స్థిరరాశి AstConst* స్థిరరాశి constp = VN_CAST(boundp, Const);
                తిరిగి constp && constp->అంకె().isFourState();
            };
            ఒకవేళ (irp && (fourState(lowp) || fourState(highp))) {
                rangep->v3error("Four-state (x/z) value in "
                                << (binp->isArray() ? "array bins" : "bin")
                                << " range bound; range bounds must be two-state constants");
                తిరిగి;
            }
            Cross_విలువ_Range range{rangep, resolve_వెడల్పు(rangep, exprp)};
            ఒకవేళ (!resolve_విలువ(rangep, exprp, నిజం, binp->isWildcard(), range)) {
                rangep->v3warn(E_UNSUPPORTED, "Unsupported: non-integral value in a coverage bin "
                                              "of an integral coverpoint.");
            } లేకపోతే ఒకవేళ (!cross_Range_ఖాళీలు(range)) {
                std::సదిశ<std::pair<V3_సంఖ్య, V3_సంఖ్య>> runs{{range.lo, range.hi}};
                ఒకవేళ (range.wildcard) {  // checkConstructedBins bounded the runs
                    runs.clear();
                    crossRangeRuns(range, v3Global.opt.coverageMaxBins(), runs);
                }
                కొరకు (స్థిరరాశి std::pair<V3_సంఖ్య, V3_సంఖ్య>& నిరంతరం : runs) {
                    m_constructorp->కూడు_Stmtsp(అంశం_Call(fl, cpVarp, పద్ధతి,
                                                       {new_విలువ_Const(fl, నిరంతరం.మొదటి, exprp),
                                                        new_విలువ_Const(fl, నిరంతరం.రెండవ, exprp)})
                                                  ->makeStmt());
                }
            }
            తిరిగి;
        }
        ఒకవేళ (!integral(lowp) || !integral(highp)) {
            rangep->v3warn(E_UNSUPPORTED, "Unsupported: non-integral value in a coverage bin "
                                          "of an integral coverpoint.");
            తిరిగి;
        }
        // Compare values and the coverpoint's domain signed, in a width holding all of them
        స్థిరరాశి స్వయంచాలక bound_వెడల్పు
            = [&](స్థిరరాశి Ast_కణుపు_సమాసం* boundp) { తిరిగి unbounded(boundp) ? 0 : boundp->వెడల్పు(); };
        స్థిరరాశి పూర్ణాంకం వెడల్పు = std::max({exprp->వెడల్పు(), bound_వెడల్పు(lowp), bound_వెడల్పు(highp)}) + 1;
        స్థిరరాశి Cross_విలువ_Range domain
            = cross_విలువ_Domain(rangep, exprp->వెడల్పు(), exprp->isSigned(), వెడల్పు);
        AstVar* స్థిరరాశి lop = constructor_తాత్కాలిక(fl, ఉపసర్గ + "_lo", exprp->dtypep());
        lop->dtype_సమితి_బిట్టు_Sized(వెడల్పు, VSigning::SIGNED);
        AstVar* స్థిరరాశి hip = constructor_తాత్కాలిక(fl, ఉపసర్గ + "_hi", lop->dtypep());
        స్థిరరాశి స్వయంచాలక bound = [&](Ast_కణుపు_సమాసం* boundp, స్థిరరాశి V3_సంఖ్య& హద్దు) -> Ast_కణుపు_సమాసం* {
            ఒకవేళ (unbounded(boundp)) {
                AstConst* స్థిరరాశి limitp = కొత్త AstConst{fl, హద్దు};
                limitp->dtypeFrom(lop);
                తిరిగి limitp;
            }
            Ast_కణుపు_సమాసం* valuep = boundp->clone_వృక్షం(అబద్ధం);
            // A value no wider than a signed coverpoint has its type (see crossRangeBound)
            ఒకవేళ (exprp->isSigned() && boundp->వెడల్పు() <= exprp->వెడల్పు()) {
                valuep = resize_విలువ(valuep, exprp->dtypep());
            }
            తిరిగి resize_విలువ(valuep, lop->dtypep());
        };
        స్థిరరాశి స్వయంచాలక ref = [&](AstVar* varp, VAccess access = VAccess::READ) {
            తిరిగి కొత్త AstVarRef{fl, varp, access};
        };
        m_constructorp->కూడు_Stmtsp(
            కొత్త Ast_కేటాయించు{fl, ref(lop, VAccess::WRITE), bound(lowp, domain.lo)});
        m_constructorp->కూడు_Stmtsp(
            కొత్త Ast_కేటాయించు{fl, ref(hip, VAccess::WRITE), irp ? bound(highp, domain.hi) : ref(lop)});
        AstConst* స్థిరరాశి minp = కొత్త AstConst{fl, domain.lo};
        AstConst* స్థిరరాశి maxp = కొత్త AstConst{fl, domain.hi};
        minp->dtypeFrom(lop);
        maxp->dtypeFrom(lop);
        AstCond* స్థిరరాశి lowerp
            = కొత్త AstCond{fl, కొత్త AstLtS{fl, ref(lop), minp}, minp->clone_వృక్షం(అబద్ధం), ref(lop)};
        AstCond* స్థిరరాశి upperp
            = కొత్త AstCond{fl, కొత్త AstGtS{fl, ref(hip), maxp}, maxp->clone_వృక్షం(అబద్ధం), ref(hip)};
        lowerp->dtypeFrom(lop);
        upperp->dtypeFrom(lop);
        m_constructorp->కూడు_Stmtsp(కొత్త Ast_కేటాయించు{fl, ref(lop, VAccess::WRITE), lowerp});
        m_constructorp->కూడు_Stmtsp(కొత్త Ast_కేటాయించు{fl, ref(hip, VAccess::WRITE), upperp});
        m_constructorp->కూడు_Stmtsp(కొత్త AstIf{fl, కొత్త AstLteS{fl, ref(lop), ref(hip)},
                                            అంశం_Call(fl, cpVarp, పద్ధతి,
                                                     {resize_విలువ(ref(lop), exprp->dtypep()),
                                                      resize_విలువ(ref(hip), exprp->dtypep())})
                                                ->makeStmt()});
    }

    // The runtime index of the bin of a run holding the coverpoint value, which is in the run:
    // declared + (value - lo) / stride, capped at the last bin, which holds any remainder.
    స్థిర Ast_కణుపు_సమాసం* నిరంతరం_Bin_సూచీ(File_పంక్తి* fl, Ast_కణుపు_సమాసం* exprp, స్థిరరాశి Bin_నిరంతరం& నిరంతరం) {
        ఒకవేళ (నిరంతరం.m_లెక్క == 1) తిరిగి cnum(fl, నిరంతరం.m_declared);
        స్థిరరాశి పూర్ణాంకం వెడల్పు = exprp->వెడల్పు();
        // A run spans at most 2^width values, so offsets in it are unsigned width-bit numbers
        Ast_కణుపు_సమాసం* indexp
            = కొత్త AstSub{fl, exprp->clone_వృక్షం(అబద్ధం), new_విలువ_Const(fl, నిరంతరం.m_lo, exprp)};
        indexp->dtype_సమితి_బిట్టు_Sized(వెడల్పు, VSigning::UNSIGNED);
        V3_సంఖ్య stride{fl, వెడల్పు, 0};
        stride.op_కేటాయించు(నిరంతరం.m_stride);
        ఒకవేళ (stride.లెక్క_Ones() != 1) {
            indexp = కొత్త AstDiv{fl, indexp, కొత్త AstConst{fl, stride}};
        } లేకపోతే ఒకవేళ (!stride.isEqOne()) {
            indexp = కొత్త Ast_జరుపు_R{fl, indexp, కొత్త AstConst{fl, stride.most_సమితి_బిట్టు_P1() - 1}};
        }
        // Compare the run's values with those of count bins of stride values, without overflow
        స్థిరరాశి పూర్ణాంకం ext_వెడల్పు = నిరంతరం.m_lo.వెడల్పు() + 1;
        V3_సంఖ్య lo{fl, ext_వెడల్పు, 0};
        lo.opExtendS(నిరంతరం.m_lo, నిరంతరం.m_lo.వెడల్పు());
        V3_సంఖ్య పరిధి{fl, ext_వెడల్పు, 0};
        పరిధి.opExtendS(నిరంతరం.m_hi, నిరంతరం.m_hi.వెడల్పు());
        పరిధి.opSub(V3_సంఖ్య{పరిధి}, lo);
        V3_సంఖ్య covered{fl, ext_వెడల్పు, 0};
        covered.op_కేటాయించు(నిరంతరం.m_stride);
        covered.opMul(V3_సంఖ్య{covered}, V3_సంఖ్య{fl, ext_వెడల్పు, నిరంతరం.m_లెక్క});
        V3_సంఖ్య hasRemainder{fl, 1, 0};
        ఒకవేళ (!hasRemainder.opGte(పరిధి, covered).isEqZero()) {
            AstConst* స్థిరరాశి lastp = కొత్త AstConst{fl, V3_సంఖ్య{fl, వెడల్పు, నిరంతరం.m_లెక్క - 1}};
            indexp = కొత్త AstCond{fl, కొత్త AstGt{fl, indexp->clone_వృక్షం(అబద్ధం), lastp},
                                 lastp->clone_వృక్షం(అబద్ధం), indexp};
        }
        ఒకవేళ (వెడల్పు < VL_IDATASIZE) {
            indexp = కొత్త AstExtend{fl, indexp, VL_IDATASIZE};
        } లేకపోతే ఒకవేళ (వెడల్పు > VL_IDATASIZE) {
            indexp = కొత్త AstSel{fl, indexp, 0, VL_IDATASIZE};
        }
        తిరిగి కొత్త Ast_కూడు{fl, cnum(fl, నిరంతరం.m_declared), indexp};
    }

    // Emit the sample() hit of a run of bins, whose code does not grow with its number of bins:
    //   if (iff && lo <= value && value <= hi) m_cp.incrementBin(<runBinIndex>);
    శూన్య emit_నిరంతరం_తగిలింది(AstCoverpoint* coverpointp, AstCoverBin* binp, AstVar* cpVarp,
                    Ast_కణుపు_సమాసం* exprp, స్థిరరాశి Bin_నిరంతరం& నిరంతరం) {
        File_పంక్తి* స్థిరరాశి fl = binp->fileline();
        AstConst* స్థిరరాశి lop = new_విలువ_Const(fl, నిరంతరం.m_lo, exprp);
        Ast_కణుపు_సమాసం* condp = శూన్యసూచిక;
        ఒకవేళ (నిరంతరం.m_lo.isCaseEq(నిరంతరం.m_hi)) {
            condp = కొత్త AstEq{fl, exprp->clone_వృక్షం(అబద్ధం), lop};
        } లేకపోతే {
            AstConst* స్థిరరాశి hip = new_విలువ_Const(fl, నిరంతరం.m_hi, exprp);
            condp = makeRangeCondition(fl, exprp, lop, hip);
            VL_DO_DANGLING(నెట్టు_Deletep(lop), lop);
            VL_DO_DANGLING(నెట్టు_Deletep(hip), hip);
        }
        emit_Conv_తగిలింది_If(coverpointp, binp, cpVarp, నిరంతరం_Bin_సూచీ(fl, exprp, నిరంతరం), condp);
    }

    // Emit a transition bin's hit action into sample():
    //   if (cond) { m_cp.incrementBin/recordHit(idx); [illegal: $error; $stop] }
    // Used by the transition generators so a completed sequence records into the runtime bin.
    శూన్య కూడు_Conv_Trans_తగిలింది_If(AstCoverpoint* coverpointp, AstCoverBin* binp, స్థిరరాశి Conv_Bin_లక్ష్యం& tgt,
                           Ast_కణుపు_సమాసం* condp) {
        File_పంక్తి* స్థిరరాశి fl = binp->fileline();
        Ast_కణుపు* actionp = make_Runtime_Bin_తగిలింది(fl, tgt);
        ఒకవేళ (binp->binsType() == VCoverBinsType::BINS_ILLEGAL) {
            actionp->కూడు_తదుపరి(makeIllegalBinAction(
                fl, "Illegal transition bin " + binp->prettyNameQ() + " hit in coverpoint "
                        + coverpointp->prettyNameQ()));
        }
        UASSERT_OBJ(m_sampleFuncp, binp, "sample() CFunc not set for transition bin");
        కూడు_Sample_Stmt(కొత్త AstIf{fl, condp, actionp, శూన్యసూచిక});
    }

    // Route a coverpoint through a VlCoverpoint member: emit the member, its sample()
    // increments, the constructor configuration (init + namers), and registration.
    శూన్య generateCoverpoint(AstCoverpoint* coverpointp, Ast_కణుపు_సమాసం* exprp, పూర్ణాంకం at_కనిష్ఠ_విలువ) {
        File_పంక్తి* స్థిరరాశి fl = coverpointp->fileline();
        స్థిరరాశి తార్కిక dynamic = m_runtimePoints.లెక్క(coverpointp);
        UINFO(4, "  Generating VlCoverpoint member: " << coverpointp->name());

        // Size the hit list to the gen-time max bin overlap (1 unless cross-fed with
        // overlapping ranges), so no cross hit is ever dropped and storage is minimal.
        స్థిరరాశి తార్కిక crossFed = m_crossed_Cp_పేర్లు.లెక్క(coverpointp->name()) != 0;
        స్థిరరాశి పూర్ణాంకం తగిలింది_Bound = compute_తగిలింది_List_Bound(coverpointp, exprp, crossFed);
        UINFO(6, "    Hit-list bound (max bin overlap) = " << తగిలింది_Bound);
        AstVar* స్థిరరాశి cpVarp = కొత్త AstVar{fl, VVarType::MEMBER, "__Vcp_" + coverpointp->name(),
                                          coverpointDType(fl, static_cast<uint32_t>(తగిలింది_Bound))};
        m_covergroupp->కూడు_Membersp(cpVarp);
        m_cpVarMap[coverpointp->name()] = cpVarp;
        m_cpBins.emplace(cpVarp, CoverpointBins{});
        m_cpBins.at(cpVarp).exprp = exprp;
        m_cpBins.at(cpVarp).crossed = crossFed;
        // Create the runtime in the instance node first; everything below configures it.
        m_constructorp->కూడు_Stmtsp(make_అంశం_Create(fl, cpVarp, VCMethod::COVERGROUP_కూడు_COVERPOINT));
        generate_అంశం_బరువు(fl, cpVarp, coverpointp->optionsp());

        // A cross reads this coverpoint's hit list, so clear it at the start of the
        // coverpoint's sample() contribution (before any incrementBin appends to it), even
        // when the iff guard disables sampling.
        ఒకవేళ (crossFed) {
            UASSERT_OBJ(m_sampleFuncp, coverpointp, "sample() CFunc not set for clearHitList");
            m_sampleFuncp->కూడు_Stmtsp(
                అంశం_Call(fl, cpVarp, VCMethod::COVERGROUP_CLEAR_తగిలింది_LIST)->makeStmt());
        }
        ఒకవేళ (dynamic && coverpointHasStateExclusions(coverpointp)) {
            AstVar* స్థిరరాశి excludedp
                = కొత్త AstVar{fl, VVarType::BLOCKTEMP,
                             "__VcpExcluded_" + sanitizeGeneratedName(coverpointp->name()),
                             coverpointp->కనుగొను_బిట్టు_DType()};
            excludedp->funcLocal(నిజం);
            m_sampleFuncp->కూడు_Stmtsp(excludedp);
            AstCMethodHard* స్థిరరాశి callp
                = అంశం_Call(fl, cpVarp,
                           exprp->isWide() ? VCMethod::COVERGROUP_విలువ_EXCLUDED_W
                                           : VCMethod::COVERGROUP_విలువ_EXCLUDED,
                           {exprp->clone_వృక్షం(అబద్ధం)});
            callp->dtype_సమితి_బిట్టు();
            కూడు_Sample_Stmt(కొత్త Ast_కేటాయించు{fl, కొత్త AstVarRef{fl, excludedp, VAccess::WRITE}, callp});
            m_excludedVars.emplace(cpVarp, excludedp);
        }

        // Walk bins (non-default, then default), assigning sequential indices that match the
        // namer append order; emit sample increments and collect namer statements.  Constructed
        // bins follow them all, placed when the coverpoint is constructed.
        std::సదిశ<Ast_కణుపు_Stmt*> namerStmts;
        std::సదిశ<AstCoverBin*> defaultBins;
        std::సదిశ<AstCoverBin*> sizedBins;
        std::సదిశ<std::tuple<AstCoverBin*, uint32_t, Ast_కణుపు_సమాసం*>> metadata;
        std::సదిశ<స్థిరరాశి Bin_నిరంతరం*> నిరంతరం_Metadata;
        uint64_t idx = 0;  // Runtime index of the next bin; 32-bit once checked below
        కొరకు (Ast_కణుపు* binp = coverpointp->binsp(); binp; binp = binp->nextp()) {
            AstCoverBin* స్థిరరాశి cbinp = VN_AS(binp, CoverBin);
            స్థిరరాశి పూర్ణాంకం errorsBefore = dynamic ? V3Error::error_లెక్క() : 0;
            ఒకవేళ (cbinp->binsType() == VCoverBinsType::BINS_DEFAULT) {
                defaultBins.నెట్టు_వెనుక(cbinp);
                కొనసాగు;
            }
            ఒకవేళ (isConstructedBins(cbinp)) {
                UASSERT_OBJ(dynamic, cbinp, "Constructed bins without value metadata");
                Bin_పరిధి పరిధి;
                పరిధి.sized = static_cast<int32_t>(sizedBins.కొలత());
                m_cpBins.at(cpVarp).spans.emplace(cbinp->name(), పరిధి);
                sizedBins.నెట్టు_వెనుక(cbinp);
                కొనసాగు;
            }
            ఒకవేళ (cbinp->transp()) {
                // Transition bin (incl. array transition 'bins t[] = (a=>b),(c=>d)' and
                // illegal_bins/ignore_bins transitions).  All sequences of one transition bin
                // share a bin name and merge in the coverage DB to a single point, so model
                // them as one runtime bin incremented by any matching sequence.  The sequence
                // matching is generated as a state machine, with the hit routed to this bin's
                // runtime slot.
                namerStmts.నెట్టు_వెనుక(makeNamer(cpVarp, cbinp, -1, static_cast<uint32_t>(idx)));
                స్థిరరాశి Conv_Bin_లక్ష్యం tgt{cpVarp, static_cast<uint32_t>(idx),
                                        cbinp->binsType().bin_Is_సాధారణ()};
                కొరకు (Ast_కణుపు* sp = cbinp->transp(); sp; sp = sp->nextp())
                    generate_Single_Transition_సంకేతం(coverpointp, cbinp, exprp, tgt,
                                                 VN_AS(sp, Cover_Trans_సమితి));
                ఒకవేళ (dynamic && V3Error::error_లెక్క() == errorsBefore) {
                    metadata.emplace_వెనుక(cbinp, static_cast<uint32_t>(idx), శూన్యసూచిక);
                }
                ++idx;
                కొనసాగు;
            }
            BinRuns plan;
            ఒకవేళ (binRunsFor(cbinp, exprp, plan)) {
                // Array elements and automatic bins generate as runs, so neither sample() nor
                // the constructor grows with their number of bins.
                ఒకవేళ (plan.unsupported) {  // bin ignored or invalid; reserve no slot
                    m_droppedBins[coverpointp].నెట్టు_వెనుక(cbinp->name());
                    కొనసాగు;
                }
                CoverpointBins& bins = m_cpBins.at(cpVarp);
                స్థిరరాశి uint32_t మొదటి_విలువ = bins.మొత్తం;
                స్థిరరాశి uint32_t మొదటి_Declared = static_cast<uint32_t>(idx);
                namerStmts.నెట్టు_వెనుక(
                    makeNamer(cpVarp, cbinp, plan.లెక్క, మొదటి_Declared, {}, plan.విలువలు));
                కొరకు (Bin_నిరంతరం& నిరంతరం : plan.runs) {
                    నిరంతరం.m_declared = static_cast<uint32_t>(idx);
                    bins.runs.నెట్టు_వెనుక(std::move(నిరంతరం));
                    స్థిరరాశి Bin_నిరంతరం& stored = bins.runs.వెనుక();
                    ఒకవేళ (bins.crossed && cbinp->binsType().bin_Is_సాధారణ()) {
                        స్థిరరాశి uint32_t మొదటి = మొదటి_విలువ + stored.m_declared - మొదటి_Declared;
                        కొరకు (uint32_t element = 0; element < stored.m_లెక్క; ++element) {
                            bins.విలువలు[మొదటి + element].runp = &stored;
                            bins.విలువలు[మొదటి + element].element = element;
                        }
                    }
                    ఒకవేళ (!stored.m_ఖాళీలు) {
                        emit_నిరంతరం_తగిలింది(coverpointp, cbinp, cpVarp, exprp, stored);
                        ఒకవేళ (dynamic && V3Error::error_లెక్క() == errorsBefore) {
                            నిరంతరం_Metadata.నెట్టు_వెనుక(&stored);
                        }
                    }
                    idx += stored.m_లెక్క;
                }
                కొనసాగు;
            }
            ఒకవేళ (cbinp->isArray()) {  // value array of a real coverpoint: b[0]..b[N-1]
                // Only integral coverpoints have runtime value metadata (m_runtimePoints)
                UASSERT_OBJ(!dynamic, cbinp, "Runtime value metadata for a real coverpoint");
                తార్కిక unsupported = అబద్ధం;
                std::సదిశ<Ast_కణుపు_సమాసం*> విలువలు = వెలికితీయి_Array_విలువలు(cbinp, exprp, unsupported);
                ఒకవేళ (unsupported) {  // bin ignored (COVERIGN emitted); reserve no slot
                    m_droppedBins[coverpointp].నెట్టు_వెనుక(cbinp->name());
                    కొనసాగు;
                }
                namerStmts.నెట్టు_వెనుక(makeNamer(cpVarp, cbinp, static_cast<int64_t>(విలువలు.కొలత()),
                                               static_cast<uint32_t>(idx), విలువలు));
                కొరకు (Ast_కణుపు_సమాసం* valuep : విలువలు) {
                    // The cross selections of this covergroup still read the value.
                    m_detached_విలువలు.నెట్టు_వెనుక(valuep);
                    emit_Conv_తగిలింది_If(coverpointp, cbinp, cpVarp,
                                  cnum(cbinp->fileline(), static_cast<uint32_t>(idx)),
                                  build_విలువ_Condition(cbinp, exprp, valuep));
                    ++idx;
                }
            } లేకపోతే {
                namerStmts.నెట్టు_వెనుక(makeNamer(cpVarp, cbinp, -1, static_cast<uint32_t>(idx)));
                // buildBinCondition is null for 'ignore_bins = default' (no ranges); the bin
                // still gets a reserved slot (recorded, never incremented).
                ఒకవేళ (Ast_కణుపు_సమాసం* స్థిరరాశి condp = buildBinCondition(cbinp, exprp))
                    emit_Conv_తగిలింది_If(coverpointp, cbinp, cpVarp,
                                  cnum(cbinp->fileline(), static_cast<uint32_t>(idx)), condp);
                ఒకవేళ (dynamic && V3Error::error_లెక్క() == errorsBefore) {
                    metadata.emplace_వెనుక(cbinp, static_cast<uint32_t>(idx), శూన్యసూచిక);
                }
                ++idx;
            }
        }
        // A cross selecting an ignored bins declaration selects no bins
        కొరకు (స్థిరరాశి std::string& name : m_droppedBins[coverpointp]) {
            m_cpBins.at(cpVarp).spans.emplace(name, Bin_పరిధి{});
        }
        // A value a constructed bin holds is no default bin's; only sampling tells which do
        AstVar* sizedMatchedp = శూన్యసూచిక;
        ఒకవేళ (!defaultBins.ఖాళీలు()
            && std::any_యొక్క(sizedBins.begin(), sizedBins.చివర(), [](స్థిరరాశి AstCoverBin* binp) {
                   తిరిగి binp->binsType().bin_Is_సాధారణ();
               })) {
            sizedMatchedp = కొత్త AstVar{fl, VVarType::BLOCKTEMP,
                                       "__VcpSized_" + sanitizeGeneratedName(coverpointp->name()),
                                       coverpointp->కనుగొను_బిట్టు_DType()};
            sizedMatchedp->funcLocal(నిజం);
            m_sampleFuncp->కూడు_Stmtsp(sizedMatchedp);
            కూడు_Sample_Stmt(కొత్త Ast_కేటాయించు{fl, కొత్త AstVarRef{fl, sizedMatchedp, VAccess::WRITE},
                                        కొత్త AstConst{fl, AstConst::బిట్టు_False{}}});
        }
        కొరకు (uint32_t sized = 0; sized < sizedBins.కొలత(); ++sized) {
            emitSizedSample(coverpointp, sizedBins[sized], cpVarp, exprp, sized, sizedMatchedp);
        }
        కొరకు (AstCoverBin* స్థిరరాశి defBinp : defaultBins) {
            File_పంక్తి* స్థిరరాశి dfl = defBinp->fileline();
            namerStmts.నెట్టు_వెనుక(makeNamer(cpVarp, defBinp, -1, static_cast<uint32_t>(idx)));
            Ast_కణుపు_సమాసం* condp = buildDefaultCondition(coverpointp, exprp, dfl);
            ఒకవేళ (sizedMatchedp) {
                condp = కొత్త AstLogAnd{
                    dfl, కొత్త AstNot{dfl, కొత్త AstVarRef{dfl, sizedMatchedp, VAccess::READ}}, condp};
            }
            emit_Conv_తగిలింది_If(coverpointp, defBinp, cpVarp, cnum(dfl, static_cast<uint32_t>(idx)),
                          condp);
            ++idx;
        }
        ఒకవేళ (idx > std::numeric_limits<uint32_t>::max()) {
            // The runtime indexes bins with 32 bits; stop before generating a model
            coverpointp->v3warn(E_UNSUPPORTED, "Unsupported: coverpoint with more than "
                                                   << std::numeric_limits<uint32_t>::max()
                                                   << " bins");
        }

        // Transition coverpoints track the previous sampled value; update it once at the end of
        // this coverpoint's sample() contribution (the prev var was created on demand by the
        // transition matching above).
        ఒకవేళ (coverpointHasTransition(coverpointp)) {
            AstVar* స్థిరరాశి prevVarp = VN_AS(coverpointp->user1p(), Var);
            కూడు_Sample_Stmt(
                కొత్త Ast_కేటాయించు{coverpointp->fileline(),
                              కొత్త AstVarRef{prevVarp->fileline(), prevVarp, VAccess::WRITE},
                              exprp->clone_వృక్షం(అబద్ధం)});
        }

        // Constructor: init (allocates), namers, then registration (under --coverage).
        // Under --protect-ids the hierarchy and page string reach the coverage database
        // verbatim, so obfuscate them like line/toggle points (per-word hierarchy, whole-
        // unit page).  No-ops when --protect-ids is off.
        స్థిరరాశి తార్కిక prot = v3Global.opt.protectIds();
        స్థిరరాశి std::string hier
            = VIdProtect::protect_పదాలు_If(m_covergroupName + "." + coverpointp->name(), prot);
        m_constructorp->కూడు_Stmtsp(
            అంశం_Call(fl, cpVarp, VCMethod::COVERGROUP_INIT,
                     {ctext(fl, quoted(hier)), cnum(fl, static_cast<uint32_t>(at_కనిష్ఠ_విలువ)),
                      cnum(fl, static_cast<uint32_t>(idx))})
                ->makeStmt());
        కొరకు (Ast_కణుపు_Stmt* స్థిరరాశి ns : namerStmts) m_constructorp->కూడు_Stmtsp(ns);
        ఒకవేళ (dynamic) {
            m_constructorp->కూడు_Stmtsp(
                అంశం_Call(fl, cpVarp, VCMethod::COVERGROUP_విలువ_TYPE,
                         {cnum(fl, exprp->వెడల్పు()), cnum(fl, exprp->isSigned())})
                    ->makeStmt());
            విలువ_Lists lists;
            కొరకు (స్థిరరాశి స్వయంచాలక& నమోదు_2 : metadata) {
                collect_విలువ_Metadata(lists, exprp, std::get<0>(నమోదు_2), std::get<1>(నమోదు_2),
                                     std::get<2>(నమోదు_2));
            }
            కొరకు (స్థిరరాశి Bin_నిరంతరం* స్థిరరాశి runp : నిరంతరం_Metadata) collect_నిరంతరం_Metadata(lists, exprp, *runp);
            emit_విలువ_List(fl, cpVarp, VCMethod::COVERGROUP_విలువ_RANGES, lists.m_ranges);
            emit_విలువ_List(fl, cpVarp, VCMethod::COVERGROUP_విలువ_RUNS, lists.m_runs);
            emit_విలువ_List(fl, cpVarp, VCMethod::COVERGROUP_విలువ_PATTERNS, lists.m_patterns);
            emit_విలువ_List(fl, cpVarp, VCMethod::COVERGROUP_విలువ_TRANSITIONS, lists.m_transitions);
            కొరకు (AstCoverBin* స్థిరరాశి binp : sizedBins) {
                generateConstructedBins(coverpointp, binp, cpVarp, exprp);
            }
            m_constructorp->కూడు_Stmtsp(
                అంశం_Call(fl, cpVarp, VCMethod::COVERGROUP_విలువ_FINALIZE)->makeStmt());
        }
        ఒకవేళ (v3Global.opt.coverage()) {
            స్థిరరాశి std::string పుట
                = VIdProtect::protectIf("v_covergroup/" + m_covergroupName, prot);
            m_constructorp->కూడు_Stmtsp(
                అంశం_Call(fl, cpVarp, VCMethod::COVERGROUP_రిజిస్టరు_BINS,
                         {ctext(fl, "vlSymsp->_vm_contextp__->coveragep()"),
                          ctext(fl, quoted(పుట)),
                          cnum(fl, అంశం_Database_బరువు(coverpointp->optionsp())),
                          cnum(fl, m_cg_Type_బరువు)})
                    ->makeStmt());
        }
    }

    // Generate state machine code for multi-value transition sequences
    // Handles transitions like (1 => 2 => 3 => 4)
    శూన్య generate_Multi_విలువ_Transition_సంకేతం(AstCoverpoint* coverpointp, AstCoverBin* binp,
                                          Ast_కణుపు_సమాసం* exprp, స్థిరరాశి Conv_Bin_లక్ష్యం& tgt,
                                          స్థిరరాశి std::సదిశ<Ast_Cover_Trans_అంశం*>& వస్తువులు) {
        UINFO(4, "    Generating multi-value transition state machine for: " << binp->name());
        UINFO(4, "      Sequence length: " << వస్తువులు.కొలత() << " items");

        // Create state position variable
        AstVar* స్థిరరాశి stateVarp = createSequenceStateVar(coverpointp, binp);

        // Build case statement with N cases (one for each state 0 to N-1)
        // State 0: Not started, looking for first item
        // State 1 to N-1: In progress, looking for next item

        AstCase* స్థిరరాశి casep
            = కొత్త AstCase{binp->fileline(), VCaseType::CT_CASE,
                          కొత్త AstVarRef{stateVarp->fileline(), stateVarp, VAccess::READ}, శూన్యసూచిక};

        // Generate each case item in the switch statement
        కొరకు (కొలత_t state = 0; state < వస్తువులు.కొలత(); ++state) {
            Ast_Case_అంశం* caseItemp = generateTransitionStateCase(coverpointp, binp, exprp, tgt,
                                                                 stateVarp, వస్తువులు, state);
            casep->కూడు_Itemsp(caseItemp);
        }

        // Add default case (reset to state 0) to prevent CASEINCOMPLETE warnings,
        // since the state variable is wider than the number of valid states.
        Ast_Case_అంశం* స్థిరరాశి defaultItemp = కొత్త Ast_Case_అంశం{
            binp->fileline(), శూన్యసూచిక,
            కొత్త Ast_కేటాయించు{binp->fileline(),
                          కొత్త AstVarRef{binp->fileline(), stateVarp, VAccess::WRITE},
                          కొత్త AstConst{binp->fileline(), AstConst::Widthed_విలువ{}, 8, 0}}};
        casep->కూడు_Itemsp(defaultItemp);

        కూడు_Sample_Stmt(casep);
        UINFO(4, "      Successfully added multi-value transition state machine");
    }

    // Generate code for a single state in the transition state machine
    // Returns the case item for this state
    Ast_Case_అంశం* generateTransitionStateCase(AstCoverpoint* coverpointp, AstCoverBin* binp,
                                             Ast_కణుపు_సమాసం* exprp, స్థిరరాశి Conv_Bin_లక్ష్యం& tgt,
                                             AstVar* stateVarp,
                                             స్థిరరాశి std::సదిశ<Ast_Cover_Trans_అంశం*>& వస్తువులు,
                                             కొలత_t state) {
        File_పంక్తి* స్థిరరాశి fl = binp->fileline();

        // Build condition for current value matching expected item at this state
        Ast_కణుపు_సమాసం* స్థిరరాశి సరిపోలిక_Condp = build_Transition_అంశం_Condition(వస్తువులు[state], exprp);

        Ast_కణుపు_Stmt* సరిపోలిక_Actionp = శూన్యసూచిక;

        ఒకవేళ (state == వస్తువులు.కొలత() - 1) {
            // Last state: sequence complete!  Record the hit in the runtime VlCoverpoint.
            సరిపోలిక_Actionp = make_Runtime_Bin_తగిలింది(fl, tgt);

            // For illegal_bins, add error message
            ఒకవేళ (binp->binsType() == VCoverBinsType::BINS_ILLEGAL) {
                స్థిరరాశి string err_సందేశం = "Illegal transition bin " + binp->prettyNameQ()
                                      + " hit in coverpoint " + coverpointp->prettyNameQ();
                సరిపోలిక_Actionp = సరిపోలిక_Actionp->కూడు_తదుపరి(makeIllegalBinAction(fl, err_సందేశం));
            }

            // Reset state to 0
            సరిపోలిక_Actionp = సరిపోలిక_Actionp->కూడు_తదుపరి(
                కొత్త Ast_కేటాయించు{fl, కొత్త AstVarRef{fl, stateVarp, VAccess::WRITE},
                              కొత్త AstConst{fl, AstConst::Widthed_విలువ{}, 8, 0}});
        } లేకపోతే {
            // Intermediate state: advance to next state
            సరిపోలిక_Actionp = కొత్త Ast_కేటాయించు{
                fl, కొత్త AstVarRef{fl, stateVarp, VAccess::WRITE},
                కొత్త AstConst{fl, AstConst::Widthed_విలువ{}, 8, static_cast<uint32_t>(state + 1)}};
        }

        // Build restart logic: check if current value matches first item
        // If so, restart sequence from state 1 (even if we're in middle of sequence)
        Ast_కణుపు_Stmt* no_సరిపోలిక_Actionp = శూన్యసూచిక;
        ఒకవేళ (state > 0) {
            // Check if current value matches first item (restart condition)
            Ast_కణుపు_సమాసం* స్థిరరాశి restartCondp = build_Transition_అంశం_Condition(వస్తువులు[0], exprp);

            UASSERT_OBJ(restartCondp, వస్తువులు[0],
                        "buildTransitionItemCondition returned nullptr for restart");

            // Restart to state 1
            Ast_కణుపు_Stmt* restartActionp
                = కొత్త Ast_కేటాయించు{fl, కొత్త AstVarRef{fl, stateVarp, VAccess::WRITE},
                                కొత్త AstConst{fl, AstConst::Widthed_విలువ{}, 8, 1}};

            // Reset to state 0 (else branch)
            Ast_కణుపు_Stmt* resetActionp
                = కొత్త Ast_కేటాయించు{fl, కొత్త AstVarRef{fl, stateVarp, VAccess::WRITE},
                                కొత్త AstConst{fl, AstConst::Widthed_విలువ{}, 8, 0}};

            no_సరిపోలిక_Actionp = కొత్త AstIf{fl, restartCondp, restartActionp, resetActionp};
        }
        // For state 0, no action needed if no match (stay in state 0)

        // Combine into if-else
        Ast_కణుపు_Stmt* స్థిరరాశి stmtp = కొత్త AstIf{fl, సరిపోలిక_Condp, సరిపోలిక_Actionp, no_సరిపోలిక_Actionp};

        // Create case item for this state value
        Ast_Case_అంశం* స్థిరరాశి caseItemp = కొత్త Ast_Case_అంశం{
            fl, కొత్త AstConst{fl, AstConst::Widthed_విలువ{}, 8, static_cast<uint32_t>(state)},
            stmtp};

        తిరిగి caseItemp;
    }

    // Create: $error(msg); $stop;  Used when an illegal bin is hit.
    Ast_కణుపు_Stmt* makeIllegalBinAction(File_పంక్తి* fl, స్థిరరాశి string& err_సందేశం) {
        AstDisplay* స్థిరరాశి errorp
            = కొత్త AstDisplay{fl, VDisplayType::DT_ERROR, err_సందేశం, శూన్యసూచిక, శూన్యసూచిక};
        errorp->fmtp()->timeunit(m_covergroupp->timeunit());
        static_cast<Ast_కణుపు*>(errorp)->కూడు_తదుపరి(కొత్త AstStop{fl, నిజం});
        తిరిగి errorp;
    }

    // Preserve the coverpoint's width and signedness after V3Width.
    స్థిర AstConst* new_విలువ_Const(File_పంక్తి* fl, స్థిరరాశి V3_సంఖ్య& విలువ, స్థిరరాశి Ast_కణుపు_సమాసం* exprp) {
        V3_సంఖ్య narrowed{fl, exprp->వెడల్పు(), 0};
        narrowed.op_కేటాయించు(విలువ);
        AstConst* స్థిరరాశి constp = కొత్త AstConst{fl, narrowed};
        constp->dtypeFrom(exprp);
        తిరిగి constp;
    }

    // A real copy of a real or integral range bound, for comparing with a real coverpoint.
    స్థిర AstConst* newRealConst(AstConst* constp) {
        ఒకవేళ (constp->అంకె().isDouble()) తిరిగి constp->clone_వృక్షం(అబద్ధం);
        V3_సంఖ్య real{&constp->అంకె(), 64};
        real.opIToRD(constp->అంకె(), constp->isSigned());
        తిరిగి కొత్త AstConst{constp->fileline(), real};
    }

    // Clone a constant node, widening to targetWidth if needed (zero-extend).
    // Used to ensure comparisons use matching widths after V3Width has run.
    స్థిర AstConst* widenConst(File_పంక్తి* fl, AstConst* constp, పూర్ణాంకం లక్ష్యం_వెడల్పు) {
        ఒకవేళ (constp->వెడల్పు() == లక్ష్యం_వెడల్పు) తిరిగి constp->clone_వృక్షం(అబద్ధం);
        V3_సంఖ్య అంకె{fl, లక్ష్యం_వెడల్పు, 0};
        అంకె.op_కేటాయించు(constp->అంకె());
        తిరిగి కొత్త AstConst{fl, అంకె};
    }

    // Build a range condition: minp <= exprp <= maxp.
    // Uses signed comparisons if exprp is signed; omits trivially-true domain bounds.
    // All arguments are non-owning; clones exprp/minp/maxp as needed.
    Ast_కణుపు_సమాసం* makeRangeCondition(File_పంక్తి* fl, Ast_కణుపు_సమాసం* exprp, Ast_కణుపు_సమాసం* minp,
                                    Ast_కణుపు_సమాసం* maxp) {
        స్థిరరాశి పూర్ణాంకం సమాసం_వెడల్పు = exprp->వెడల్పు_Min();
        AstConst* స్థిరరాశి minConstp = VN_AS(minp, Const);
        AstConst* స్థిరరాశి maxConstp = VN_AS(maxp, Const);
        ఒకవేళ (exprp->isDouble()) {
            // A real coverpoint has no finite domain bounds to omit.
            తిరిగి కొత్త AstAnd{fl,
                              కొత్త AstGteD{fl, exprp->clone_వృక్షం(అబద్ధం), newRealConst(minConstp)},
                              కొత్త AstLteD{fl, exprp->clone_వృక్షం(అబద్ధం), newRealConst(maxConstp)}};
        }
        // Widen constants to match expression width so post-V3Width nodes use correct macros
        AstConst* స్థిరరాశి minWidep = widenConst(fl, minConstp, సమాసం_వెడల్పు);
        AstConst* స్థిరరాశి maxWidep = widenConst(fl, maxConstp, సమాసం_వెడల్పు);
        V3_సంఖ్య minimum{fl, సమాసం_వెడల్పు, 0};
        V3_సంఖ్య గరిష్ఠ{fl, సమాసం_వెడల్పు, 0};
        గరిష్ఠ.సమితి_All_Bits1();
        ఒకవేళ (exprp->isSigned()) {
            minimum.సమితి_బిట్టు(సమాసం_వెడల్పు - 1, 1);
            గరిష్ఠ.సమితి_బిట్టు(సమాసం_వెడల్పు - 1, 0);
        }
        Ast_కణుపు_సమాసం* lowerp = శూన్యసూచిక;
        Ast_కణుపు_సమాసం* upperp = శూన్యసూచిక;
        ఒకవేళ (minWidep->అంకె().isCaseEq(minimum)) {
            VL_DO_DANGLING(నెట్టు_Deletep(minWidep), minWidep);
        } లేకపోతే {
            lowerp = exprp->isSigned() ? static_cast<Ast_కణుపు_సమాసం*>(
                                             కొత్త AstGteS{fl, exprp->clone_వృక్షం(అబద్ధం), minWidep})
                                       : static_cast<Ast_కణుపు_సమాసం*>(
                                             కొత్త AstGte{fl, exprp->clone_వృక్షం(అబద్ధం), minWidep});
        }
        ఒకవేళ (maxWidep->అంకె().isCaseEq(గరిష్ఠ)) {
            VL_DO_DANGLING(నెట్టు_Deletep(maxWidep), maxWidep);
        } లేకపోతే {
            upperp = exprp->isSigned() ? static_cast<Ast_కణుపు_సమాసం*>(
                                             కొత్త AstLteS{fl, exprp->clone_వృక్షం(అబద్ధం), maxWidep})
                                       : static_cast<Ast_కణుపు_సమాసం*>(
                                             కొత్త AstLte{fl, exprp->clone_వృక్షం(అబద్ధం), maxWidep});
        }
        ఒకవేళ (lowerp && upperp) తిరిగి కొత్త AstAnd{fl, lowerp, upperp};
        ఒకవేళ (lowerp) తిరిగి lowerp;
        ఒకవేళ (upperp) తిరిగి upperp;
        తిరిగి కొత్త AstConst{fl, AstConst::బిట్టు_True{}};
    }

    // Build a one-sided comparison for an open-ended bin range whose other bound is '$'.
    // '$' denotes the coverpoint domain extreme, so {[lo:$]} == (expr >= lo) and
    // {[$:hi]} == (expr <= hi).
    Ast_కణుపు_సమాసం* makeOpenRangeCondition(File_పంక్తి* fl, Ast_కణుపు_సమాసం* exprp, AstConst* boundp,
                                        తార్కిక is_క్రింది_Bound) {
        AstConst* స్థిరరాశి widep = widenConst(fl, boundp, exprp->వెడల్పు_Min());
        ఒకవేళ (is_క్రింది_Bound) {
            ఒకవేళ (exprp->isSigned()) తిరిగి కొత్త AstGteS{fl, exprp->clone_వృక్షం(అబద్ధం), widep};
            తిరిగి కొత్త AstGte{fl, exprp->clone_వృక్షం(అబద్ధం), widep};
        }
        ఒకవేళ (exprp->isSigned()) తిరిగి కొత్త AstLteS{fl, exprp->clone_వృక్షం(అబద్ధం), widep};
        తిరిగి కొత్త AstLte{fl, exprp->clone_వృక్షం(అబద్ధం), widep};
    }

    // Build condition for a single transition item.
    // Returns expression that checks if exprp matches the item's value/range list.
    // Overload for when the expression is a variable read -- creates and manages the VarRef
    // internally, so callers don't need to construct a temporary node.
    Ast_కణుపు_సమాసం* build_Transition_అంశం_Condition(Ast_Cover_Trans_అంశం* itemp, AstVar* varp) {
        Ast_కణుపు_సమాసం* varRefp = కొత్త AstVarRef{varp->fileline(), varp, VAccess::READ};
        Ast_కణుపు_సమాసం* స్థిరరాశి condp = build_Transition_అంశం_Condition(itemp, varRefp);
        VL_DO_DANGLING(నెట్టు_Deletep(varRefp), varRefp);
        తిరిగి condp;
    }

    // Non-owning: exprp is cloned internally; caller retains ownership of exprp.
    Ast_కణుపు_సమాసం* build_Transition_అంశం_Condition(Ast_Cover_Trans_అంశం* itemp, Ast_కణుపు_సమాసం* exprp) {
        Ast_కణుపు_సమాసం* condp = శూన్యసూచిక;

        కొరకు (Ast_కణుపు* valp = itemp->valuesp(); valp; valp = valp->nextp()) {
            Ast_కణుపు_సమాసం* singleCondp = శూన్యసూచిక;
            valp = V3Const::constifyEdit(valp);
            AstConst* స్థిరరాశి constp = VN_CAST(valp, Const);
            ఒకవేళ (!constp) {
                valp->v3error("Non-constant expression in transition bin; "
                              "values must be constants (IEEE 1800-2023 19.5)");
                తిరిగి కొత్త AstConst{valp->fileline(), AstConst::బిట్టు_False_Erroring{}};
            }
            singleCondp
                = కొత్త AstEq{constp->fileline(), exprp->clone_వృక్షం(అబద్ధం), constp->clone_వృక్షం(అబద్ధం)};
            ఒకవేళ (condp) {
                condp = కొత్త AstOr{itemp->fileline(), condp, singleCondp};
            } లేకపోతే {
                condp = singleCondp;
            }
        }

        తిరిగి condp;
    }

    // Generate code for a single transition sequence (used by both regular and array bins)
    శూన్య generate_Single_Transition_సంకేతం(AstCoverpoint* coverpointp, AstCoverBin* binp,
                                      Ast_కణుపు_సమాసం* exprp, స్థిరరాశి Conv_Bin_లక్ష్యం& tgt,
                                      Ast_Cover_Trans_సమితి* transSetp) {
        UINFO(4, "      Generating code for transition sequence");

        // Get or create previous value variable
        AstVar* స్థిరరాశి prevVarp = create_Prev_విలువ_Var(coverpointp, exprp);

        UASSERT_OBJ(
            transSetp, binp,
            "Transition bin has no transition set (transp() was checked before calling this)");

        // Get transition items (the sequence: item1 => item2 => item3)
        std::సదిశ<Ast_Cover_Trans_అంశం*> వస్తువులు;
        కొరకు (Ast_కణుపు* itemp = transSetp->itemsp(); itemp; itemp = itemp->nextp())
            వస్తువులు.నెట్టు_వెనుక(VN_AS(itemp, Cover_Trans_అంశం));

        ఒకవేళ (వస్తువులు.ఖాళీలు()) {
            binp->v3error("Transition set without items");
            తిరిగి;
        }

        ఒకవేళ (వస్తువులు.కొలత() == 1) {
            // Single item transition not valid (need at least 2 values for =>)
            binp->v3error("Transition requires at least two values");
            తిరిగి;
        } లేకపోతే ఒకవేళ (వస్తువులు.కొలత() == 2) {
            // Simple two-value transition: (val1 => val2)
            // Use optimized direct comparison (no state machine needed)
            Ast_కణుపు_సమాసం* స్థిరరాశి cond1p = build_Transition_అంశం_Condition(వస్తువులు[0], prevVarp);
            Ast_కణుపు_సమాసం* స్థిరరాశి cond2p = build_Transition_అంశం_Condition(వస్తువులు[1], exprp);

            // Combine: prev matches val1 AND current matches val2
            Ast_కణుపు_సమాసం* నిండిన_Condp = కొత్త AstAnd{binp->fileline(), cond1p, cond2p};

            కూడు_Conv_Trans_తగిలింది_If(coverpointp, binp, tgt, నిండిన_Condp);

            UINFO(4, "        Successfully added 2-value transition if statement");
        } లేకపోతే {
            // Multi-value sequence (a => b => c => ...)
            // Use state machine to track position in sequence
            generate_Multi_విలువ_Transition_సంకేతం(coverpointp, binp, exprp, tgt, వస్తువులు);
        }
    }

    // Append a "{ VlCoverpoint* __Vcx_cps[] = {cp0, cp1, ...}; <call> }" statement.  The brace
    // and the temporary array stay literal text -- a CMethodHard is one call, not a block --
    // but callp itself carries the member, method and '->'.  Construction only: init() copies
    // the array into the cross, so sample() reads it from there and needs no array at all.
    AstCStmt* makeCrossCpsCall(File_పంక్తి* fl, స్థిరరాశి std::సదిశ<AstVar*>& cpVars,
                               AstCMethodHard* callp) {
        AstCStmt* స్థిరరాశి cs = కొత్త AstCStmt{fl};
        cs->కూడు("{ VlCoverpoint* __Vcx_cps[] = {");
        కొరకు (కొలత_t d = 0; d < cpVars.కొలత(); ++d) {
            ఒకవేళ (d != 0) cs->కూడు(", ");
            cs->కూడు(memberRef(fl, cpVars[d]));
        }
        cs->కూడు("}; ");
        cs->కూడు(callp);
        cs->కూడు("; }");
        తిరిగి cs;
    }

    // Assign the per-bin flags individually: one-bit SV results have integer C++ storage types,
    // which may narrow in a bool initializer list but convert implicitly in assignments.
    AstCStmt* makeCrossIffsCall(File_పంక్తి* fl, స్థిరరాశి std::సదిశ<AstCoverCrossBin*>& bins,
                                AstCMethodHard* callp) {
        AstCStmt* స్థిరరాశి cs = కొత్త AstCStmt{fl};
        cs->కూడు("{ bool __Vcx_iffs[" + cvt_గమ్యం_Str(bins.కొలత()) + "]; ");
        కొరకు (కొలత_t i = 0; i < bins.కొలత(); ++i) {
            స్థిరరాశి AstCoverCrossBin* స్థిరరాశి binp = bins[i];
            cs->కూడు("__Vcx_iffs[" + cvt_గమ్యం_Str(i) + "] = ");
            cs->కూడు(binp->iffp() ? binp->iffp()->clone_వృక్షం(అబద్ధం)
                                 : కొత్త AstConst{fl, AstConst::బిట్టు_True{}});
            cs->కూడు("; ");
        }
        cs->కూడు(callp);
        cs->కూడు("; }");
        తిరిగి cs;
    }

    ఉపయోగించి CrossSelection = std::సదిశ<uint64_t>;
    నిర్మాణం ResolvedCrossBin final {
        AstCoverCrossBin* binp;
        CrossSelection selection;
    };
    నిర్మాణం CrossLayout final {
        uint32_t tuples = 0;
        uint32_t autoBins = 0;
        uint64_t bin_పదాలు = 0;
        తార్కిక valid = నిజం;
        std::సదిశ<ResolvedCrossBin> bins;
    };
    నిర్మాణం CrossSelectionContext final {
        AstCoverCross* crossp;  // Cross whose tuple space is being selected
        స్థిరరాశి std::సదిశ<AstVar*>& cpVars;  // Feeding coverpoints in dimension order
        స్థిరరాశి std::map<std::string, uint32_t>& dimensions;  // Coverpoint name -> dimension
        uint32_t tuples;  // Size of the Cartesian product
        std::సదిశ<uint32_t> strides;  // Flat-index stride per dimension
        తార్కిక valid = నిజం;  // False if this explicit bin cannot be implemented
    };
    నిర్మాణం Cross_Binsof_లక్ష్యం final {
        స్థిరరాశి CoverpointBins* m_binsp = శూన్యసూచిక;  // Declared bins; null after a reported error
        uint32_t m_dimension = 0;  // Coverpoint index within the cross
        uint32_t m_మొదటి = 0;  // First declared normal bin in the selected span
        uint32_t m_లెక్క = 0;  // Number of declared normal bins in the selected span
        uint32_t m_declared_మొదటి = 0;  // First runtime bin index of the selected span
        uint32_t m_declared_చివర = UINT32_MAX;  // One past the last runtime bin index
        int32_t m_sized = -1;  // Index of the sized array holding the selected bins; or -1
    };
    నిర్మాణం Cross_విలువ_Range final {
        V3_సంఖ్య lo;  // Inclusive lower bound, sign-extended to the comparison width
        V3_సంఖ్య hi;  // Inclusive upper bound
        V3_సంఖ్య నమూనా_2;  // Allowed bit values; all X for an ordinary interval
        తార్కిక wildcard = అబద్ధం;  // A wildcard singleton rather than an exact four-state value

        Cross_విలువ_Range(Ast_కణుపు* nodep, పూర్ణాంకం వెడల్పు)
            : lo{nodep, వెడల్పు}
            , hi{nodep, వెడల్పు}
            , నమూనా_2{nodep, వెడల్పు} {
            నమూనా_2.సమితి_All_బిట్లు_X();
        }
    };

    స్థిర std::సదిశ<Ast_కణుపు*> cross_Bin_విలువలు(స్థిరరాశి Cross_Bin_విలువలు& bin) {
        ఒకవేళ (bin.valuep) తిరిగి {bin.valuep};
        std::సదిశ<Ast_కణుపు*> విలువలు;
        ఒకవేళ (bin.binp->transp()) {
            // IEEE 1800-2023 19.6.1: binsof uses the last value of each transition.
            కొరకు (Ast_కణుపు* setp = bin.binp->transp(); setp; setp = setp->nextp()) {
                Ast_Cover_Trans_అంశం* lastp = VN_AS(setp, Cover_Trans_సమితి)->itemsp();
                అయితే (lastp->nextp()) lastp = VN_AS(lastp->nextp(), Cover_Trans_అంశం);
                కొరకు (Ast_కణుపు* valuep = lastp->valuesp(); valuep; valuep = valuep->nextp()) {
                    విలువలు.నెట్టు_వెనుక(valuep);
                }
            }
        } లేకపోతే {
            కొరకు (Ast_కణుపు* valuep = bin.binp->rangesp(); valuep; valuep = valuep->nextp()) {
                విలువలు.నెట్టు_వెనుక(valuep);
            }
        }
        తిరిగి విలువలు;
    }

    // The values of the element-th bin of a run, sign-extended to 'width'
    స్థిర Cross_విలువ_Range నిరంతరం_Bin_Range(Ast_కణుపు* nodep, స్థిరరాశి Bin_నిరంతరం& నిరంతరం, uint32_t element,
                                       పూర్ణాంకం వెడల్పు) {
        స్థిరరాశి పూర్ణాంకం runw = నిరంతరం.m_lo.వెడల్పు();
        V3_సంఖ్య స్థానభ్రంశం{nodep, runw};
        స్థానభ్రంశం.opMul(నిరంతరం.m_stride, V3_సంఖ్య{nodep, runw, element});
        V3_సంఖ్య lo{nodep, runw};
        lo.op_కూడు(నిరంతరం.m_lo, స్థానభ్రంశం);
        V3_సంఖ్య hi = నిరంతరం.m_hi;
        ఒకవేళ (element + 1 < నిరంతరం.m_లెక్క) {
            స్థానభ్రంశం.opSub(నిరంతరం.m_stride, V3_సంఖ్య{nodep, runw, 1});
            hi.op_కూడు(lo, స్థానభ్రంశం);
        }
        Cross_విలువ_Range range{nodep, వెడల్పు};
        range.lo.opExtendS(lo, runw);
        range.hi.opExtendS(hi, runw);
        తిరిగి range;
    }

    స్థిర పూర్ణాంకం cross_Range_వెడల్పు(Ast_కణుపు* nodep) {
        ఒకవేళ (స్థిరరాశి AstInsideRange* స్థిరరాశి rangep = VN_CAST(nodep, InsideRange)) {
            తిరిగి std::max(rangep->lhsp()->వెడల్పు(), rangep->rhsp()->వెడల్పు());
        }
        తిరిగి nodep->వెడల్పు();
    }

    స్థిర తార్కిక cross_విలువ_Less(స్థిరరాశి V3_సంఖ్య& lhs, స్థిరరాశి V3_సంఖ్య& rhs) {
        V3_సంఖ్య ఫలితం{&lhs};
        తిరిగి !ఫలితం.opLtS(lhs, rhs).isEqZero();
    }

    // Round a real bound inward to the nearest coverpoint value (IEEE 1800-2023 19.5.7).  Sets
    // 'empty' if the domain has no value on the bound's side.
    స్థిర శూన్య crossRealBound(స్థిరరాశి AstConst* constp, Ast_కణుపు_సమాసం* exprp, తార్కిక పై,
                               స్థిరరాశి Cross_విలువ_Range& domain, V3_సంఖ్య& ఫలితం, తార్కిక& ఖాళీలు) {
        స్థిరరాశి ద్విగుణ విలువ = constp->అంకె().గమ్యం_Double();
        స్థిరరాశి ద్విగుణ bound = పై ? std::floor(విలువ) : std::ceil(విలువ);
        // Powers of two are exact, so the integral bound compares exactly with the domain edges.
        స్థిరరాశి ద్విగుణ హద్దు
            = std::ldexp(1.0, exprp->isSigned() ? exprp->వెడల్పు() - 1 : exprp->వెడల్పు());
        స్థిరరాశి ద్విగుణ minimum = exprp->isSigned() ? -హద్దు : 0.0;
        ఒకవేళ (std::isnan(bound) || (పై ? bound < minimum : bound >= హద్దు)) {
            ఖాళీలు = నిజం;
            ఫలితం = domain.lo;
        } లేకపోతే ఒకవేళ (bound < minimum) {
            ఫలితం = domain.lo;
        } లేకపోతే ఒకవేళ (bound >= హద్దు) {
            ఫలితం = domain.hi;
        } లేకపోతే {
            V3_సంఖ్య real{&ఫలితం, 64};
            real.సమితి_Double(bound);
            ఫలితం.opRToIRoundS(real);
        }
    }

    స్థిర తార్కిక crossRangeBound(Ast_కణుపు* nodep, Ast_కణుపు_సమాసం* exprp, తార్కిక పై, తార్కిక bin_విలువ,
                                స్థిరరాశి Cross_విలువ_Range& domain, V3_సంఖ్య& ఫలితం, తార్కిక& ఖాళీలు) {
        ఒకవేళ (VN_IS(nodep, Unbounded)) {
            V3_సంఖ్య హద్దు{nodep, exprp->వెడల్పు()};
            ఒకవేళ (పై) హద్దు.సమితి_All_Bits1();
            ఒకవేళ (exprp->isSigned()) {
                హద్దు.సమితి_బిట్టు(exprp->వెడల్పు() - 1, !పై);
                ఫలితం.opExtendS(హద్దు, హద్దు.వెడల్పు());
            } లేకపోతే {
                ఫలితం.op_కేటాయించు(హద్దు);
            }
            తిరిగి నిజం;
        }
        స్థిరరాశి AstConst* స్థిరరాశి constp = VN_CAST(nodep, Const);
        ఒకవేళ (!constp) తిరిగి అబద్ధం;
        ఒకవేళ (constp->అంకె().isDouble()) {
            crossRealBound(constp, exprp, పై, domain, ఫలితం, ఖాళీలు);
            తిరిగి నిజం;
        }
        ఒకవేళ (constp->అంకె().isString()) తిరిగి అబద్ధం;
        ఒకవేళ (bin_విలువ && exprp->isSigned() && constp->వెడల్పు() <= exprp->వెడల్పు()) {
            // Bin bit patterns use the coverpoint's effective type (IEEE 1800-2023 19.5.7).
            // Wider values and intersect filters retain their values for domain clipping.
            V3_సంఖ్య విలువ{nodep, exprp->వెడల్పు()};
            ఒకవేళ (constp->isSigned()) {
                విలువ.opExtendS(constp->అంకె(), constp->వెడల్పు());
            } లేకపోతే {
                విలువ.op_కేటాయించు(constp->అంకె());
            }
            ఫలితం.opExtendS(విలువ, విలువ.వెడల్పు());
        } లేకపోతే ఒకవేళ (constp->isSigned()) {
            ఫలితం.opExtendS(constp->అంకె(), constp->వెడల్పు());
        } లేకపోతే {
            ఫలితం.op_కేటాయించు(constp->అంకె());
        }
        తిరిగి నిజం;
    }

    స్థిర Cross_విలువ_Range cross_విలువ_Domain(Ast_కణుపు* nodep, పూర్ణాంకం విలువ_వెడల్పు, తార్కిక isSigned,
                                            పూర్ణాంకం వెడల్పు) {
        Cross_విలువ_Range domain{nodep, వెడల్పు};
        V3_సంఖ్య lo{nodep, విలువ_వెడల్పు};
        V3_సంఖ్య hi{nodep, విలువ_వెడల్పు};
        hi.సమితి_All_Bits1();
        ఒకవేళ (isSigned) {
            lo.సమితి_బిట్టు(విలువ_వెడల్పు - 1, 1);
            hi.సమితి_బిట్టు(విలువ_వెడల్పు - 1, 0);
            domain.lo.opExtendS(lo, విలువ_వెడల్పు);
            domain.hi.opExtendS(hi, విలువ_వెడల్పు);
        } లేకపోతే {
            domain.lo.op_కేటాయించు(lo);
            domain.hi.op_కేటాయించు(hi);
        }
        తిరిగి domain;
    }

    స్థిర శూన్య intersectCrossRange(Cross_విలువ_Range& range, స్థిరరాశి Cross_విలువ_Range& other) {
        ఒకవేళ (cross_విలువ_Less(range.lo, other.lo)) range.lo = other.lo;
        ఒకవేళ (cross_విలువ_Less(other.hi, range.hi)) range.hi = other.hi;
    }

    స్థిర తార్కిక cross_విలువ_Range(Ast_కణుపు* nodep, Ast_కణుపు_సమాసం* exprp, తార్కిక bin_విలువ, తార్కిక wildcard,
                                స్థిరరాశి Cross_విలువ_Range& domain, Cross_విలువ_Range& range) {
        తార్కిక ఖాళీలు = అబద్ధం;
        స్థిరరాశి AstConst* స్థిరరాశి constp = VN_CAST(nodep, Const);
        ఒకవేళ (స్థిరరాశి AstInsideRange* స్థిరరాశి rangep = VN_CAST(nodep, InsideRange)) {
            ఒకవేళ (!crossRangeBound(rangep->lhsp(), exprp, అబద్ధం, bin_విలువ, domain, range.lo, ఖాళీలు)
                || !crossRangeBound(rangep->rhsp(), exprp, నిజం, bin_విలువ, domain, range.hi, ఖాళీలు)
                || range.lo.isFourState() || range.hi.isFourState()) {
                తిరిగి అబద్ధం;
            }
        } లేకపోతే ఒకవేళ (constp && constp->అంకె().isDouble()) {
            // A real value participates only if integral; it has no wildcard bits.
            crossRealBound(constp, exprp, అబద్ధం, domain, range.lo, ఖాళీలు);
            crossRealBound(constp, exprp, నిజం, domain, range.hi, ఖాళీలు);
        } లేకపోతే {
            ఒకవేళ (!crossRangeBound(nodep, exprp, అబద్ధం, bin_విలువ, domain, range.lo, ఖాళీలు)) {
                తిరిగి అబద్ధం;
            }
            ఒకవేళ (bin_విలువ && exprp->isSigned() && constp && !constp->isSigned()
                && constp->వెడల్పు() > exprp->వెడల్పు()) {
                తార్కిక representable = నిజం;
                కొరకు (పూర్ణాంకం బిట్టు_2 = exprp->వెడల్పు(); బిట్టు_2 < constp->వెడల్పు(); ++బిట్టు_2) {
                    representable &= !constp->అంకె().బిట్టు_Is1(బిట్టు_2);
                }
                ఒకవేళ (representable) {
                    V3_సంఖ్య విలువ{nodep, exprp->వెడల్పు()};
                    విలువ.op_కేటాయించు(constp->అంకె());
                    range.lo.opExtendS(విలువ, విలువ.వెడల్పు());
                }
            }
            range.hi = range.lo;
            range.wildcard = wildcard;
            ఒకవేళ (wildcard) {
                range.నమూనా_2 = range.lo;
                range.lo = domain.lo;
                range.hi = domain.hi;
                ఒకవేళ (constp && constp->isSigned()) {
                    // Replicated X sign bits are correlated, not independent wildcards.
                    // The source domain preserves expansion-before-casting (19.5.7).
                    intersectCrossRange(
                        range, cross_విలువ_Domain(nodep, constp->వెడల్పు(), నిజం, domain.lo.వెడల్పు()));
                }
            }
        }
        ఒకవేళ (ఖాళీలు) {
            range.lo = domain.hi;
            range.hi = domain.lo;
            తిరిగి నిజం;
        }
        ఒకవేళ (!range.lo.isFourState()) intersectCrossRange(range, domain);
        ఒకవేళ (range.wildcard && exprp->isSigned()) {
            స్థిరరాశి పూర్ణాంకం sign = exprp->వెడల్పు() - 1;
            ఒకవేళ (constp && !constp->isSigned() && constp->వెడల్పు() > exprp->వెడల్పు()) {
                // Unsigned equality preserves every target bit pattern if discarded bits are zero.
                కొరకు (పూర్ణాంకం బిట్టు_2 = exprp->వెడల్పు(); బిట్టు_2 < constp->వెడల్పు(); ++బిట్టు_2) {
                    ఒకవేళ (constp->అంకె().బిట్టు_Is1(బిట్టు_2)) {
                        range.lo = domain.hi;
                        range.hi = domain.lo;
                        తిరిగి నిజం;
                    }
                }
                కొరకు (పూర్ణాంకం బిట్టు_2 = exprp->వెడల్పు(); బిట్టు_2 < range.నమూనా_2.వెడల్పు(); ++బిట్టు_2) {
                    ఒకవేళ (range.నమూనా_2.బిట్టు_Is_XZ(sign))
                        range.నమూనా_2.సమితి_బిట్టు(బిట్టు_2, 'x');
                    లేకపోతే
                        range.నమూనా_2.సమితి_బిట్టు(బిట్టు_2, range.నమూనా_2.బిట్టు_Is1(sign));
                }
                తిరిగి నిజం;
            }
            // Discarded high bits constrain the target sign, rather than becoming don't-cares.
            కొరకు (పూర్ణాంకం బిట్టు_2 = exprp->వెడల్పు(); బిట్టు_2 < range.నమూనా_2.వెడల్పు(); ++బిట్టు_2) {
                ఒకవేళ (range.నమూనా_2.బిట్టు_Is_XZ(బిట్టు_2)) కొనసాగు;
                స్థిరరాశి తార్కిక విలువ = range.నమూనా_2.బిట్టు_Is1(బిట్టు_2);
                ఒకవేళ (!range.నమూనా_2.బిట్టు_Is_XZ(sign) && range.నమూనా_2.బిట్టు_Is1(sign) != విలువ) {
                    range.lo = domain.hi;
                    range.hi = domain.lo;
                    ఆపు;
                }
                range.నమూనా_2.సమితి_బిట్టు(sign, విలువ);
            }
        }
        తిరిగి నిజం;
    }

    గణన CrossRangeState : uint8_t {
        CROSS_INSIDE_BOUNDS = 0,  // Prefix is strictly inside the interval
        CROSS_AT_క్రింది = 1,
        CROSS_AT_పై = 2,
        CROSS_AT_BOUNDS = CROSS_AT_క్రింది | CROSS_AT_పై,
        CROSS_NO_సరిపోలిక = 4  // Prefix cannot match the interval/pattern
    };

    స్థిర CrossRangeState cross_Range_అడుగు(స్థిరరాశి Cross_విలువ_Range& range, CrossRangeState state,
                                          పూర్ణాంకం బిట్టు_2, పూర్ణాంకం విలువ) {
        ఒకవేళ (state == CROSS_NO_సరిపోలిక) తిరిగి CROSS_NO_సరిపోలిక;
        // Flipping the sign bit makes signed order lexicographic.
        స్థిరరాశి తార్కిక sign = బిట్టు_2 == range.నమూనా_2.వెడల్పు() - 1;
        ఒకవేళ (!range.నమూనా_2.బిట్టు_Is_XZ(బిట్టు_2) && విలువ != (range.నమూనా_2.బిట్టు_Is1(బిట్టు_2) ^ sign)) {
            తిరిగి CROSS_NO_సరిపోలిక;
        }
        స్థిరరాశి పూర్ణాంకం దిగువ = range.lo.బిట్టు_Is1(బిట్టు_2) ^ sign;
        స్థిరరాశి పూర్ణాంకం ఎగువ = range.hi.బిట్టు_Is1(బిట్టు_2) ^ sign;
        ఒకవేళ (((state & CROSS_AT_క్రింది) && విలువ < దిగువ)
            || ((state & CROSS_AT_పై) && విలువ > ఎగువ)) {
            తిరిగి CROSS_NO_సరిపోలిక;
        }
        తిరిగి static_cast<CrossRangeState>(
            ((state & CROSS_AT_క్రింది) && విలువ == దిగువ ? CROSS_AT_క్రింది : CROSS_INSIDE_BOUNDS)
            | ((state & CROSS_AT_పై) && విలువ == ఎగువ ? CROSS_AT_పై : CROSS_INSIDE_BOUNDS));
    }

    స్థిర తార్కిక crossWildcardIntersects(స్థిరరాశి Cross_విలువ_Range& range) {
        గుర్తులేని స్థితులు = 1U << CROSS_AT_BOUNDS;
        కొరకు (పూర్ణాంకం బిట్టు_2 = range.నమూనా_2.వెడల్పు() - 1; బిట్టు_2 >= 0 && స్థితులు; --బిట్టు_2) {
            గుర్తులేని తదుపరి = 0;
            కొరకు (స్థిరరాశి CrossRangeState state :
                 {CROSS_INSIDE_BOUNDS, CROSS_AT_క్రింది, CROSS_AT_పై, CROSS_AT_BOUNDS}) {
                ఒకవేళ (!(స్థితులు & (1U << state))) కొనసాగు;
                కొరకు (పూర్ణాంకం విలువ = 0; విలువ < 2; ++విలువ) {
                    స్థిరరాశి CrossRangeState equal = cross_Range_అడుగు(range, state, బిట్టు_2, విలువ);
                    ఒకవేళ (equal != CROSS_NO_సరిపోలిక) తదుపరి |= 1U << equal;
                }
            }
            స్థితులు = తదుపరి;
        }
        తిరిగి స్థితులు != 0;
    }

    // The least value at least 'from' with the non-x bits of 'pattern', in unsigned order; false
    // if none.  A bit scan, which does not enumerate the pattern's x bits.
    స్థిర తార్కిక తదుపరి_నమూనా_విలువ(స్థిరరాశి V3_సంఖ్య& నమూనా_2, స్థిరరాశి V3_సంఖ్య& from, V3_సంఖ్య& ఫలితం) {
        ఫలితం.op_కేటాయించు(from);
        పూర్ణాంకం వాహకం = -1;  // Lowest x bit above the bit scanned where 'from' has a 0
        కొరకు (పూర్ణాంకం బిట్టు_2 = from.వెడల్పు() - 1; బిట్టు_2 >= 0; --బిట్టు_2) {
            ఒకవేళ (నమూనా_2.బిట్టు_Is_XZ(బిట్టు_2)) {
                ఒకవేళ (!from.బిట్టు_Is1(బిట్టు_2)) వాహకం = బిట్టు_2;
                కొనసాగు;
            }
            ఒకవేళ (నమూనా_2.బిట్టు_Is1(బిట్టు_2) == from.బిట్టు_Is1(బిట్టు_2)) కొనసాగు;
            ఒకవేళ (from.బిట్టు_Is1(బిట్టు_2)) {  // Only a greater prefix, an x bit above raised, can match
                ఒకవేళ (వాహకం < 0) తిరిగి అబద్ధం;
                బిట్టు_2 = వాహకం;
            }
            // Then the least such value: the pattern's bits, with x bits of zero
            ఫలితం.సమితి_బిట్టు(బిట్టు_2, 1);
            అయితే (--బిట్టు_2 >= 0) ఫలితం.సమితి_బిట్టు(బిట్టు_2, నమూనా_2.బిట్టు_Is1(బిట్టు_2));
            తిరిగి నిజం;
        }
        తిరిగి నిజం;
    }

    // Append to 'runs' the maximal runs of consecutive values of 'range' that its pattern
    // matches, in value order, until there are more than 'limit'.  A match continues through
    // the pattern's trailing x bits only, which the next value's carry leaves.
    స్థిర శూన్య crossRangeRuns(స్థిరరాశి Cross_విలువ_Range& range, కొలత_t హద్దు,
                               std::సదిశ<std::pair<V3_సంఖ్య, V3_సంఖ్య>>& runs) {
        // Values order signed, which is the unsigned order of values with the sign bit flipped
        స్థిరరాశి పూర్ణాంకం sign = range.lo.వెడల్పు() - 1;
        స్థిరరాశి స్వయంచాలక flipped = [sign](V3_సంఖ్య విలువ) {
            ఒకవేళ (!విలువ.బిట్టు_Is_XZ(sign)) విలువ.సమితి_బిట్టు(sign, !విలువ.బిట్టు_Is1(sign));
            తిరిగి విలువ;
        };
        స్థిరరాశి V3_సంఖ్య నమూనా_2 = flipped(range.నమూనా_2);
        స్థిరరాశి V3_సంఖ్య hi = flipped(range.hi);
        స్థిరరాశి V3_సంఖ్య one{&hi, hi.వెడల్పు(), 1};
        V3_సంఖ్య trailing{&hi, hi.వెడల్పు()};
        కొరకు (పూర్ణాంకం బిట్టు_2 = 0; బిట్టు_2 <= sign && నమూనా_2.బిట్టు_Is_XZ(బిట్టు_2); ++బిట్టు_2) trailing.సమితి_బిట్టు(బిట్టు_2, 1);
        V3_సంఖ్య from = flipped(range.lo);
        V3_సంఖ్య మొదటి{&hi, hi.వెడల్పు()};
        V3_సంఖ్య చివరి{&hi, hi.వెడల్పు()};
        V3_సంఖ్య less{&hi};
        అయితే (runs.కొలత() <= హద్దు && తదుపరి_నమూనా_విలువ(నమూనా_2, from, మొదటి)
               && less.opLt(hi, మొదటి).isEqZero()) {
            చివరి.opOr(మొదటి, trailing);
            ఒకవేళ (!less.opLt(hi, చివరి).isEqZero()) చివరి = hi;
            runs.emplace_వెనుక(flipped(మొదటి), flipped(చివరి));
            ఒకవేళ (చివరి.isCaseEq(hi)) ఆపు;
            from.op_కూడు(చివరి, one);
        }
    }

    // True if no coverpoint value participates in a resolved value or range.  A value with x
    // or z bits participates only as a wildcard pattern (IEEE 1800-2023 19.5.7).
    స్థిర తార్కిక cross_Range_ఖాళీలు(స్థిరరాశి Cross_విలువ_Range& range) {
        ఒకవేళ (range.lo.isFourState()) తిరిగి నిజం;
        తిరిగి cross_విలువ_Less(range.hi, range.lo)
               || (range.wildcard && !crossWildcardIntersects(range));
    }

    // Comparison width that holds both the coverpoint's and a bin or intersect value's range.
    స్థిర పూర్ణాంకం resolve_వెడల్పు(Ast_కణుపు* nodep, స్థిరరాశి Ast_కణుపు_సమాసం* exprp) {
        తిరిగి std::max(exprp->వెడల్పు(), cross_Range_వెడల్పు(nodep)) + 1;
    }

    // Resolve a bin or intersect value to coverpoint values (IEEE 1800-2023 19.5.7), in the
    // width 'range' was created with.  False if the value is not a constant integral or real.
    స్థిర తార్కిక resolve_విలువ(Ast_కణుపు* nodep, Ast_కణుపు_సమాసం* exprp, తార్కిక bin_విలువ, తార్కిక wildcard,
                             Cross_విలువ_Range& range) {
        స్థిరరాశి Cross_విలువ_Range domain
            = cross_విలువ_Domain(nodep, exprp->వెడల్పు(), exprp->isSigned(), range.lo.వెడల్పు());
        తిరిగి cross_విలువ_Range(nodep, exprp, bin_విలువ, wildcard, domain, range);
    }

    స్థిర తార్కిక crossRangesIntersect(స్థిరరాశి Cross_విలువ_Range& bin, స్థిరరాశి Cross_విలువ_Range& filter) {
        // Values with x or z bits do not participate, even in an identical filter.
        ఒకవేళ (bin.lo.isFourState() || filter.lo.isFourState()) తిరిగి అబద్ధం;
        Cross_విలువ_Range సరిపోలిక = bin;
        intersectCrossRange(సరిపోలిక, filter);
        తిరిగి !cross_విలువ_Less(సరిపోలిక.hi, సరిపోలిక.lo)
               && (!bin.wildcard || crossWildcardIntersects(సరిపోలిక));
    }

    స్థిర శూన్య unsupportedCrossRange(AstCoverBinsof* selectp, తార్కిక& valid) {
        selectp->v3warn(COVERIGN, "Unsupported: non-constant or non-integral 'intersect' value, "
                                  "or four-state range bound.");
        valid = అబద్ధం;
    }

    స్థిర తార్కిక cross_విలువ_Matches_Filters(AstCoverBinsof* selectp, Ast_కణుపు* valuep,
                                         Ast_కణుపు_సమాసం* exprp, స్థిరరాశి AstCoverBin* binp,
                                         స్థిరరాశి Cross_విలువ_Range& domain,
                                         స్థిరరాశి std::సదిశ<Cross_విలువ_Range>& filters,
                                         తార్కిక& valid) {
        Cross_విలువ_Range range{valuep, domain.lo.వెడల్పు()};
        ఒకవేళ (!cross_విలువ_Range(valuep, exprp, నిజం, binp->isWildcard(), domain, range)) {
            unsupportedCrossRange(selectp, valid);
            తిరిగి అబద్ధం;
        }
        కొరకు (స్థిరరాశి Cross_విలువ_Range& filter : filters) {
            ఒకవేళ (crossRangesIntersect(range, filter)) తిరిగి నిజం;
        }
        తిరిగి అబద్ధం;
    }

    std::సదిశ<తార్కిక> selectCoverpointBins(AstCoverBinsof* selectp, స్థిరరాశి CoverpointBins& bins,
                                           uint32_t మొదటి, uint32_t లెక్క, తార్కిక& valid) {
        ఒకవేళ (selectp->rangesp() && !bins.exprp->dtypep()->skipRefp()->isIntegralOrPacked()) {
            unsupportedCrossRange(selectp, valid);
            తిరిగి {};
        }
        std::సదిశ<తార్కిక> selected(bins.మొత్తం, అబద్ధం);
        std::సదిశ<std::సదిశ<Ast_కణుపు*>> విలువలు;
        పూర్ణాంకం వెడల్పు = bins.exprp->వెడల్పు();
        కొరకు (Ast_కణుపు* rangep = selectp->rangesp(); rangep; rangep = rangep->nextp()) {
            వెడల్పు = std::max(వెడల్పు, cross_Range_వెడల్పు(rangep));
        }
        ఒకవేళ (selectp->rangesp()) {
            విలువలు.reserve(లెక్క);
            కొరకు (uint32_t i = మొదటి; i < మొదటి + లెక్క; ++i) {
                // A run's values are in the coverpoint type, whose width 'width' covers
                విలువలు.నెట్టు_వెనుక(bins.విలువలు[i].runp ? std::సదిశ<Ast_కణుపు*>{}
                                                     : cross_Bin_విలువలు(bins.విలువలు[i]));
                కొరకు (Ast_కణుపు* స్థిరరాశి valuep : విలువలు.వెనుక()) {
                    వెడల్పు = std::max(వెడల్పు, cross_Range_వెడల్పు(valuep));
                }
            }
        }
        // One extra bit preserves both unsigned maxima and negative signed bounds.
        ++వెడల్పు;
        స్థిరరాశి Cross_విలువ_Range domain
            = cross_విలువ_Domain(selectp, bins.exprp->వెడల్పు(), bins.exprp->isSigned(), వెడల్పు);
        std::సదిశ<Cross_విలువ_Range> filters;
        కొరకు (Ast_కణుపు* rangep = selectp->rangesp(); rangep; rangep = rangep->nextp()) {
            Cross_విలువ_Range filter{rangep, వెడల్పు};
            ఒకవేళ (!cross_విలువ_Range(rangep, bins.exprp, అబద్ధం, అబద్ధం, domain, filter)) {
                unsupportedCrossRange(selectp, valid);
                తిరిగి {};
            }
            filters.నెట్టు_వెనుక(std::move(filter));
        }
        కొరకు (uint32_t i = మొదటి; i < మొదటి + లెక్క; ++i) {
            ఒకవేళ (!selectp->rangesp()) {
                selected[i] = నిజం;
                కొనసాగు;
            }
            ఒకవేళ (స్థిరరాశి Bin_నిరంతరం* స్థిరరాశి runp = bins.విలువలు[i].runp) {
                స్థిరరాశి Cross_విలువ_Range range
                    = నిరంతరం_Bin_Range(selectp, *runp, bins.విలువలు[i].element, వెడల్పు);
                selected[i] = std::any_యొక్క(filters.begin(), filters.చివర(),
                                          [&](స్థిరరాశి Cross_విలువ_Range& filter) {
                                              తిరిగి crossRangesIntersect(range, filter);
                                          });
                కొనసాగు;
            }
            కొరకు (Ast_కణుపు* స్థిరరాశి valuep : విలువలు[i - మొదటి]) {
                selected[i] = cross_విలువ_Matches_Filters(
                    selectp, valuep, bins.exprp, bins.విలువలు[i].binp, domain, filters, valid);
                ఒకవేళ (!valid) తిరిగి {};
                ఒకవేళ (selected[i]) ఆపు;
            }
        }
        ఒకవేళ (selectp->isNegated()) {
            కొరకు (uint32_t i = 0; i < bins.మొత్తం; ++i) selected[i] = !selected[i];
        }
        తిరిగి selected;
    }

    // Constructor-time value metadata of one coverpoint, as C++ list entries
    నిర్మాణం విలువ_Lists final {
        std::సదిశ<std::string> m_ranges;  // Bin, then low and high words
        std::సదిశ<std::string> m_runs;  // First bin, count, then low, span, and high words
        std::సదిశ<std::string> m_patterns;  // Bin, then value, mask, low, and high words
        std::సదిశ<std::string> m_transitions;  // Transition bin
    };

    // Append a value's words, in the coverpoint's width, to a C++ list entry.
    స్థిర శూన్య append_పదాలు(std::string& పాఠ్యం, స్థిరరాశి V3_సంఖ్య& విలువ, స్థిరరాశి Ast_కణుపు_సమాసం* exprp) {
        V3_సంఖ్య narrowed{&విలువ, exprp->వెడల్పు()};
        narrowed.op_కేటాయించు(విలువ);
        కొరకు (పూర్ణాంకం మాట = 0; మాట < exprp->వెడల్పు_పదాలు(); ++మాట) {
            పాఠ్యం += ", " + cvt_గమ్యం_Str(narrowed.edata_మాట(మాట)) + "U";
        }
    }

    శూన్య collect_విలువ_Metadata(విలువ_Lists& lists, Ast_కణుపు_సమాసం* exprp, AstCoverBin* binp,
                              uint32_t సూచీ, Ast_కణుపు_సమాసం* valuep) {
        స్థిరరాశి std::string bin = cvt_గమ్యం_Str(సూచీ) + "U";
        ఒకవేళ (binp->transp()) lists.m_transitions.నెట్టు_వెనుక(bin);
        కొరకు (Ast_కణుపు* స్థిరరాశి sourcep : cross_Bin_విలువలు({binp, valuep})) {
            Cross_విలువ_Range range{sourcep, resolve_వెడల్పు(sourcep, exprp)};
            ఒకవేళ (!resolve_విలువ(sourcep, exprp, నిజం, binp->isWildcard(), range)) {
                // Sampling already resolved every state bin value, so only transitions remain.
                sourcep->v3warn(E_UNSUPPORTED, "Unsupported: non-integral value in a transition "
                                               "bin of a coverpoint with exclusions.");
                కొనసాగు;
            }
            ఒకవేళ (cross_Range_ఖాళీలు(range)) కొనసాగు;
            std::string నమోదు_2 = bin;
            ఒకవేళ (range.wildcard) {
                V3_సంఖ్య విలువ{sourcep, exprp->వెడల్పు()};
                V3_సంఖ్య ముసుగు{sourcep, exprp->వెడల్పు()};
                ముసుగు.op_బిట్లు_Non_XZ(range.నమూనా_2);
                విలువ.op_బిట్లు_One(range.నమూనా_2);
                append_పదాలు(నమోదు_2, విలువ, exprp);
                append_పదాలు(నమోదు_2, ముసుగు, exprp);
            }
            append_పదాలు(నమోదు_2, range.lo, exprp);
            append_పదాలు(నమోదు_2, range.hi, exprp);
            (range.wildcard ? lists.m_patterns : lists.m_ranges).నెట్టు_వెనుక(నమోదు_2);
        }
    }

    // Describe a run with one entry, from which the runtime computes the values of its bins.
    స్థిర శూన్య collect_నిరంతరం_Metadata(విలువ_Lists& lists, Ast_కణుపు_సమాసం* exprp, స్థిరరాశి Bin_నిరంతరం& నిరంతరం) {
        V3_సంఖ్య పరిధి{exprp, నిరంతరం.m_stride.వెడల్పు(), 0};
        పరిధి.opSub(నిరంతరం.m_stride, V3_సంఖ్య{exprp, నిరంతరం.m_stride.వెడల్పు(), 1});
        std::string నమోదు_2 = cvt_గమ్యం_Str(నిరంతరం.m_declared) + "U, " + cvt_గమ్యం_Str(నిరంతరం.m_లెక్క) + "U";
        append_పదాలు(నమోదు_2, నిరంతరం.m_lo, exprp);
        append_పదాలు(నమోదు_2, పరిధి, exprp);
        append_పదాలు(నమోదు_2, నిరంతరం.m_hi, exprp);
        lists.m_runs.నెట్టు_వెనుక(నమోదు_2);
    }

    // Emit one batched metadata list, bounding the size of each call's temporary list.
    శూన్య emit_విలువ_List(File_పంక్తి* fl, AstVar* cpVarp, VCMethod పద్ధతి,
                       స్థిరరాశి std::సదిశ<std::string>& entries) {
        కొరకు (కొలత_t మొదటి = 0; మొదటి < entries.కొలత(); మొదటి += విలువ_LIST_ENTRIES) {
            స్థిరరాశి కొలత_t చివర = std::min(entries.కొలత(), మొదటి + విలువ_LIST_ENTRIES);
            std::string పాఠ్యం = "{" + entries[మొదటి];
            కొరకు (కొలత_t i = మొదటి + 1; i < చివర; ++i) పాఠ్యం += ", " + entries[i];
            m_constructorp->కూడు_Stmtsp(
                అంశం_Call(fl, cpVarp, పద్ధతి, {ctext(fl, పాఠ్యం + "}")})->makeStmt());
        }
    }

    స్థిర తార్కిక checkCrossRef(స్థిరరాశి AstCoverCrossRef* refp, స్థిరరాశి AstCoverCross* crossp) {
        ఒకవేళ (refp->name() == crossp->name()) తిరిగి నిజం;
        refp->v3error("Cross selection "
                      << refp->prettyNameQ() << " may only name its enclosing cross "
                      << crossp->prettyNameQ() << " (IEEE 1800-2023 19.6.1.2).");
        తిరిగి అబద్ధం;
    }

    స్థిర తార్కిక checkCrossBinName(స్థిరరాశి AstCoverCrossBin* binp, std::సమితి<std::string>& పేర్లు) {
        ఒకవేళ (పేర్లు.emplace(binp->name()).రెండవ) తిరిగి నిజం;
        binp->v3error("Duplicate cross bin " << binp->prettyNameQ()
                                             << " (IEEE 1800-2023 19.6.1).");
        తిరిగి అబద్ధం;
    }

    // The span of the implicit automatic bin reported as 'name' ('auto_<i>', see
    // createImplicitAutoBins), found without naming each of its bins
    స్థిర తార్కిక implicit_Auto_Bin_పరిధి(స్థిరరాశి CoverpointBins& bins, స్థిరరాశి std::string& name,
                                    Bin_పరిధి& పరిధి) {
        స్థిరరాశి std::string ఉపసర్గ = "auto_";
        ఒకవేళ (!VString::startsWith(name, ఉపసర్గ)) తిరిగి అబద్ధం;
        స్థిరరాశి std::string digits = name.substr(ఉపసర్గ.కొలత());
        స్థిరరాశి గుర్తులేని దీర్ఘ సూచీ = std::strtoul(digits.c_str(), శూన్యసూచిక, 10);
        // Only the reported spelling names the bin, not e.g. 'auto_01' or 'auto_x'
        ఒకవేళ (సూచీ >= bins.implicitAuto.లెక్క || digits != cvt_గమ్యం_Str(సూచీ)) తిరిగి అబద్ధం;
        స్థిరరాశి uint32_t స్థానభ్రంశం = static_cast<uint32_t>(సూచీ);
        పరిధి = Bin_పరిధి{bins.implicitAuto.మొదటి + స్థానభ్రంశం, 1, bins.implicitAuto.declared + స్థానభ్రంశం};
        తిరిగి నిజం;
    }

    Cross_Binsof_లక్ష్యం
    resolve_Binsof_లక్ష్యం(స్థిరరాశి AstCoverBinsof* selectp, స్థిరరాశి AstCoverCross* crossp,
                        స్థిరరాశి std::సదిశ<AstVar*>& cpVars,
                        స్థిరరాశి std::map<std::string, uint32_t>& dimensions) స్థిరరాశి {
        స్థిరరాశి స్వయంచాలక dim = dimensions.కనుగొను(selectp->pointp()->name());
        ఒకవేళ (dim == dimensions.చివర()) {
            selectp->v3error("binsof coverpoint "
                             << selectp->pointp()->prettyNameQ() << " is not an item of cross "
                             << crossp->prettyNameQ() << " (IEEE 1800-2023 19.6.1).");
            తిరిగి {};
        }
        స్థిరరాశి CoverpointBins& bins = m_cpBins.at(cpVars[dim->రెండవ]);
        Cross_Binsof_లక్ష్యం లక్ష్యం{&bins, dim->రెండవ, 0, bins.మొత్తం};
        ఒకవేళ (!selectp->name().ఖాళీలు()) {
            స్థిరరాశి స్వయంచాలక bin = bins.spans.కనుగొను(selectp->name());
            Bin_పరిధి పరిధి{0, 0, 0};
            ఒకవేళ (bin != bins.spans.చివర()) {
                పరిధి = bin->రెండవ;
            } లేకపోతే ఒకవేళ (!implicit_Auto_Bin_పరిధి(bins, selectp->name(), పరిధి)) {
                selectp->v3error("Cannot find bin " << selectp->prettyNameQ() << " in coverpoint "
                                                    << selectp->pointp()->prettyNameQ()
                                                    << " (IEEE 1800-2023 19.6.1).");
                తిరిగి {};
            }
            లక్ష్యం.m_మొదటి = పరిధి.మొదటి;
            లక్ష్యం.m_లెక్క = పరిధి.లెక్క;
            లక్ష్యం.m_declared_మొదటి = పరిధి.declared;
            లక్ష్యం.m_declared_చివర = పరిధి.declared + పరిధి.లెక్క;
            లక్ష్యం.m_sized = పరిధి.sized;
        }
        తిరిగి లక్ష్యం;
    }

    // The variables a 'with' filter reads, each by its first reference
    ఉపయోగించి CrossFilterReads = std::map<స్థిరరాశి AstVar*, AstVarRef*>;

    // Whether a 'with' filter has a count of value tuples, of 'matches' other than '$'
    స్థిర తార్కిక counts_విలువ_Tuples(స్థిరరాశి AstCoverCrossWith* withp) {
        తిరిగి withp->countp() && !VN_IS(withp->countp(), Unbounded);
    }

    // Whether the runtime knows the values of the cross items a 'with' filter reads, or to count
    // value tuples, of all items, of integral coverpoints; else warns
    స్థిర తార్కిక crossFilterSupported(స్థిరరాశి AstCoverCrossWith* withp,
                                     స్థిరరాశి CrossFilterReads& reads) {
        కొరకు (స్థిరరాశి AstVar* varp = withp->varsp(); varp; varp = VN_AS(varp->nextp(), Var)) {
            ఒకవేళ (varp->dtypep()->skipRefp()->isIntegralOrPacked()) కొనసాగు;
            స్థిరరాశి స్వయంచాలక it = reads.కనుగొను(varp);
            ఒకవేళ (it != reads.చివర()) {
                it->రెండవ->v3warn(COVERIGN, "Unsupported: cross bin 'with' filter reading the "
                                             "value of non-integral coverpoint "
                                                 << it->రెండవ->prettyNameQ());
                తిరిగి అబద్ధం;
            }
            ఒకవేళ (counts_విలువ_Tuples(withp)) {
                withp->countp()->v3warn(COVERIGN, "Unsupported: cross bin 'matches' count of "
                                                  "value tuples of non-integral coverpoint "
                                                      << varp->prettyNameQ());
                తిరిగి అబద్ధం;
            }
        }
        తిరిగి నిజం;
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
    శూన్య generateCrossFilter(స్థిరరాశి AstCoverCrossWith* withp, స్థిరరాశి CrossFilterReads& reads,
                             స్థిరరాశి AstCoverCrossBin* binp, స్థిరరాశి AstCoverCross* crossp,
                             AstVar* cxp, స్థిరరాశి std::map<string, uint32_t>& dimensions,
                             uint32_t వస్తువులు) {
        File_పంక్తి* స్థిరరాశి fl = withp->fileline();
        స్థిరరాశి string ఉపసర్గ
            = m_sized_పేర్లు.get(sanitizeGeneratedName(crossp->name() + "__" + binp->name()));
        స్థిరరాశి తార్కిక counting = counts_విలువ_Tuples(withp);
        // The count, 0 for all
        స్థిరరాశి std::pair<Ast_కణుపు_సమాసం*, Ast_కణుపు_సమాసం*> లెక్క
            = counting ? new_లెక్క_వాదనలు(fl, ఉపసర్గ + "_count", withp->countp())
                       : new_లెక్క_వాదనలు(fl, withp->countp() ? 0 : 1);
        AstLoop* స్థిరరాశి loopp = కొత్త AstLoop{fl};
        AstCMethodHard* స్థిరరాశి nextp = అంశం_Call(fl, cxp, VCMethod::COVERGROUP_WITH_తదుపరి);
        nextp->dtype_సమితి_బిట్టు();
        loopp->కూడు_Stmtsp(కొత్త AstLoopTest{fl, loopp, nextp});
        string dims;
        కొరకు (uint32_t dim = 0; counting && dim < వస్తువులు; ++dim) {
            dims += (dims.ఖాళీలు() ? "" : ", ") + cvt_గమ్యం_Str(dim) + "U";
        }
        అయితే (AstVar* స్థిరరాశి varp = withp->varsp()) {
            varp->unlink_Fr_వెనుక();
            ఒకవేళ (!reads.లెక్క(varp)) {
                VL_DO_DANGLING(నెట్టు_Deletep(varp), varp);
                కొనసాగు;
            }
            స్థిరరాశి స్వయంచాలక dimIt = dimensions.కనుగొను(varp->name());
            UASSERT_OBJ(dimIt != dimensions.చివర(), varp, "Cross 'with' candidate not of an item");
            స్థిరరాశి uint32_t dim = dimIt->రెండవ;
            ఒకవేళ (!counting) dims += (dims.ఖాళీలు() ? "" : ", ") + cvt_గమ్యం_Str(dim) + "U";
            varp->name(ఉపసర్గ + "_" + sanitizeGeneratedName(varp->name()));
            m_constructorp->కూడు_Stmtsp(varp);
            ఒకవేళ (varp->isWide()) {
                loopp->కూడు_Stmtsp(అంశం_Call(fl, cxp, VCMethod::COVERGROUP_CROSS_WITH_విలువ_W,
                                          {cnum(fl, dim), కొత్త AstVarRef{fl, varp, VAccess::WRITE}})
                                     ->makeStmt());
            } లేకపోతే {
                AstCMethodHard* స్థిరరాశి valuep
                    = అంశం_Call(fl, cxp, VCMethod::COVERGROUP_CROSS_WITH_విలువ, {cnum(fl, dim)});
                valuep->dtype_సమితి_UInt64();
                Ast_కణుపు_DType* స్థిరరాశి dtypep = varp->కనుగొను_Logic_DType(
                    varp->వెడల్పు(), varp->వెడల్పు(),
                    varp->isSigned() ? VSigning::SIGNED : VSigning::UNSIGNED);
                loopp->కూడు_Stmtsp(కొత్త Ast_కేటాయించు{fl, కొత్త AstVarRef{fl, varp, VAccess::WRITE},
                                               resize_విలువ(valuep, dtypep)});
            }
        }
        loopp->కూడు_Stmtsp(అంశం_Call(fl, cxp, VCMethod::COVERGROUP_CROSS_WITH_TEST,
                                  {withp->filterp()->unlink_Fr_వెనుక()})
                             ->makeStmt());
        స్థిరరాశి తార్కిక prot = v3Global.opt.protectIds();
        m_constructorp->కూడు_Stmtsp(
            అంశం_Call(fl, cxp, VCMethod::COVERGROUP_CROSS_WITH_BEGIN,
                     {ctext(fl, "{" + dims + "}"), లెక్క.మొదటి, లెక్క.రెండవ,
                      ctext(fl, quoted(VIdProtect::protect_పదాలు_If(binp->name(), prot))),
                      ctext(fl, quoted(VIdProtect::protectIf(fl->filename(), prot))),
                      cnum(fl, static_cast<uint32_t>(fl->lineno()))})
                ->makeStmt());
        m_constructorp->కూడు_Stmtsp(loopp);
        m_constructorp->కూడు_Stmtsp(
            అంశం_Call(fl, cxp, VCMethod::COVERGROUP_CROSS_WITH_చివర)->makeStmt());
    }

    తార్కిక generateRuntimeSelection(Ast_కణుపు* nodep, స్థిరరాశి AstCoverCrossBin* binp,
                                  AstCoverCross* crossp, AstVar* cxp,
                                  స్థిరరాశి std::సదిశ<AstVar*>& cpVars,
                                  స్థిరరాశి std::map<string, uint32_t>& dimensions) {
        File_పంక్తి* స్థిరరాశి fl = nodep->fileline();
        ఒకవేళ (స్థిరరాశి AstCoverCrossRef* స్థిరరాశి refp = VN_CAST(nodep, CoverCrossRef)) {
            ఒకవేళ (!checkCrossRef(refp, crossp)) తిరిగి అబద్ధం;
            m_constructorp->కూడు_Stmtsp(
                అంశం_Call(fl, cxp, VCMethod::COVERGROUP_SELECT_ALL)->makeStmt());
            తిరిగి నిజం;
        }
        ఒకవేళ (స్థిరరాశి AstCoverCrossSelect* స్థిరరాశి opp = VN_CAST(nodep, CoverCrossSelect)) {
            ఒకవేళ (!generateRuntimeSelection(opp->lhsp(), binp, crossp, cxp, cpVars, dimensions)
                || !generateRuntimeSelection(opp->rhsp(), binp, crossp, cxp, cpVars, dimensions)) {
                తిరిగి అబద్ధం;
            }
            m_constructorp->కూడు_Stmtsp(అంశం_Call(fl, cxp,
                                               opp->isOr() ? VCMethod::COVERGROUP_SELECT_OR
                                                           : VCMethod::COVERGROUP_SELECT_AND)
                                          ->makeStmt());
            తిరిగి నిజం;
        }
        ఒకవేళ (స్థిరరాశి AstCoverCrossWith* స్థిరరాశి withp = VN_CAST(nodep, CoverCrossWith)) {
            CrossFilterReads reads;
            withp->filterp()->foreach([&](AstVarRef* refp) { reads.emplace(refp->varp(), refp); });
            // The count of 'matches' is a positive integer, or '$' (IEEE 1800-2023 19.6.1.2)
            ఒకవేళ ((counts_విలువ_Tuples(withp)
                 && !check_లెక్క(withp->countp(), "Cross bin 'matches' count", "19.6.1.2"))
                || !crossFilterSupported(withp, reads)
                || !generateRuntimeSelection(withp->selectp(), binp, crossp, cxp, cpVars,
                                             dimensions)) {
                తిరిగి అబద్ధం;
            }
            generateCrossFilter(withp, reads, binp, crossp, cxp, dimensions,
                                static_cast<uint32_t>(cpVars.కొలత()));
            తిరిగి నిజం;
        }
        AstCoverBinsof* స్థిరరాశి selectp = VN_AS(nodep, CoverBinsof);
        స్థిరరాశి Cross_Binsof_లక్ష్యం లక్ష్యం = resolve_Binsof_లక్ష్యం(selectp, crossp, cpVars, dimensions);
        ఒకవేళ (!లక్ష్యం.m_binsp) తిరిగి అబద్ధం;
        స్థిరరాశి CoverpointBins& bins = *లక్ష్యం.m_binsp;
        ఒకవేళ (selectp->rangesp() && !bins.exprp->dtypep()->skipRefp()->isIntegralOrPacked()) {
            తార్కిక valid = నిజం;
            unsupportedCrossRange(selectp, valid);
            తిరిగి అబద్ధం;
        }
        Ast_కణుపు_సమాసం* firstp;
        Ast_కణుపు_సమాసం* endp;
        ఒకవేళ (లక్ష్యం.m_sized < 0) {
            firstp = cnum(fl, లక్ష్యం.m_declared_మొదటి);
            endp = cnum(fl, లక్ష్యం.m_declared_చివర);
        } లేకపోతే {  // A sized array, whose bins the coverpoint's construction placed
            AstVar* స్థిరరాశి cpVarp = cpVars[లక్ష్యం.m_dimension];
            స్థిరరాశి uint32_t sized = static_cast<uint32_t>(లక్ష్యం.m_sized);
            firstp = అంశం_Call(fl, cpVarp, VCMethod::COVERGROUP_SIZED_మొదటి, {cnum(fl, sized)});
            endp = అంశం_Call(fl, cpVarp, VCMethod::COVERGROUP_SIZED_చివర, {cnum(fl, sized)});
            firstp->dtype_సమితి_UInt32();
            endp->dtype_సమితి_UInt32();
        }
        m_constructorp->కూడు_Stmtsp(
            అంశం_Call(fl, cxp, VCMethod::COVERGROUP_SELECT_DIM,
                     {cnum(fl, లక్ష్యం.m_dimension), firstp, endp, cnum(fl, selectp->isNegated()),
                      cnum(fl, selectp->rangesp() != శూన్యసూచిక)})
                ->makeStmt());
        కొరకు (Ast_కణుపు* rangep = selectp->rangesp(); rangep; rangep = rangep->nextp()) {
            Cross_విలువ_Range range{rangep, resolve_వెడల్పు(rangep, bins.exprp)};
            ఒకవేళ (!resolve_విలువ(rangep, bins.exprp, అబద్ధం, అబద్ధం, range)) {
                తార్కిక valid = నిజం;
                unsupportedCrossRange(selectp, valid);
                తిరిగి అబద్ధం;
            }
            ఒకవేళ (cross_Range_ఖాళీలు(range)) కొనసాగు;
            m_constructorp->కూడు_Stmtsp(అంశం_Call(fl, cxp,
                                               bins.exprp->isWide()
                                                   ? VCMethod::COVERGROUP_SELECT_RANGE_W
                                                   : VCMethod::COVERGROUP_SELECT_RANGE,
                                               {new_విలువ_Const(fl, range.lo, bins.exprp),
                                                new_విలువ_Const(fl, range.hi, bins.exprp)})
                                          ->makeStmt());
        }
        m_constructorp->కూడు_Stmtsp(
            అంశం_Call(fl, cxp, VCMethod::COVERGROUP_SELECT_DIM_చివర)->makeStmt());
        తిరిగి నిజం;
    }

    std::సదిశ<AstCoverCrossBin*>
    generateRuntimeCrossBins(AstCoverCross* crossp, AstVar* cxp,
                             స్థిరరాశి std::సదిశ<AstVar*>& cpVars,
                             స్థిరరాశి std::map<string, uint32_t>& dimensions) {
        std::సదిశ<AstCoverCrossBin*> bins;
        std::సమితి<string> పేర్లు;
        కొరకు (Ast_కణుపు* itemp = crossp->binsp(); itemp; itemp = itemp->nextp()) {
            AstCoverCrossBin* స్థిరరాశి binp = VN_AS(itemp, CoverCrossBin);
            ఒకవేళ (!checkCrossBinName(binp, పేర్లు)) కొనసాగు;
            File_పంక్తి* స్థిరరాశి fl = binp->fileline();
            // Generate the selection apart, so that an unsupported one leaves no code, which
            // could leave its state for the next bin
            AstFunc* స్థిరరాశి selectionp = కొత్త AstFunc{fl, "", శూన్యసూచిక, శూన్యసూచిక};
            తార్కిక supported;
            {
                VL_RESTORER(m_constructorp);
                m_constructorp = selectionp;
                supported = generateRuntimeSelection(binp->selectp(), binp, crossp, cxp, cpVars,
                                                     dimensions);
            }
            // A supported selection has code
            ఒకవేళ (supported) m_constructorp->కూడు_Stmtsp(selectionp->stmtsp()->unlink_Fr_వెనుక_With_తదుపరి());
            VL_DO_DANGLING(నెట్టు_Deletep(selectionp), selectionp);
            ఒకవేళ (!supported) కొనసాగు;
            స్థిరరాశి తార్కిక protect = v3Global.opt.protectIds();
            m_constructorp->కూడు_Stmtsp(
                అంశం_Call(fl, cxp, VCMethod::COVERGROUP_SELECT_BIN,
                         {ctext(fl, binp->binsType().bin_సమితి_Enum()),
                          ctext(fl, quoted(VIdProtect::protect_పదాలు_If(binp->name(), protect))),
                          ctext(fl, quoted(VIdProtect::protectIf(fl->filename(), protect))),
                          cnum(fl, fl->lineno()), cnum(fl, fl->మొదటి_స్తంభం()),
                          cnum(fl, static_cast<uint32_t>(bins.కొలత()))})
                    ->makeStmt());
            bins.నెట్టు_వెనుక(binp);
        }
        m_constructorp->కూడు_Stmtsp(
            అంశం_Call(crossp->fileline(), cxp, VCMethod::COVERGROUP_FINALIZE_BINS)->makeStmt());
        తిరిగి bins;
    }

    స్థిర శూన్య సమితి_Cross_Selection_Range(CrossSelection& selection, uint64_t మొదటి, uint64_t చివర) {
        అయితే (మొదటి < చివర) {
            స్థిరరాశి గుర్తులేని బిట్టు_2 = మొదటి % 64;
            స్థిరరాశి గుర్తులేని బిట్లు = std::min<uint64_t>(64 - బిట్టు_2, చివర - మొదటి);
            selection[VL_BITWORD_Q(మొదటి)]
                |= (బిట్లు == 64 ? ~uint64_t{0} : (uint64_t{1} << బిట్లు) - 1) << బిట్టు_2;
            మొదటి += బిట్లు;
        }
    }

    CrossSelection crossSelection(Ast_కణుపు* nodep, CrossSelectionContext& ctx) {
        ఒకవేళ (స్థిరరాశి AstCoverCrossRef* స్థిరరాశి refp = VN_CAST(nodep, CoverCrossRef)) {
            ఒకవేళ (!checkCrossRef(refp, ctx.crossp)) {
                ctx.valid = అబద్ధం;
                తిరిగి {};
            }
            CrossSelection ఫలితం(
                VL_BITWORD_Q(static_cast<uint64_t>(ctx.tuples) + VL_QUADSIZE - 1), 0);
            సమితి_Cross_Selection_Range(ఫలితం, 0, ctx.tuples);
            తిరిగి ఫలితం;
        }
        ఒకవేళ (AstCoverCrossSelect* స్థిరరాశి opp = VN_CAST(nodep, CoverCrossSelect)) {
            CrossSelection lhs = crossSelection(opp->lhsp(), ctx);
            స్థిరరాశి CrossSelection rhs = crossSelection(opp->rhsp(), ctx);
            ఒకవేళ (!ctx.valid) తిరిగి {};
            కొరకు (కొలత_t i = 0; i < lhs.కొలత(); ++i) {
                lhs[i] = opp->isOr() ? lhs[i] | rhs[i] : lhs[i] & rhs[i];
            }
            తిరిగి lhs;
        }
        AstCoverBinsof* స్థిరరాశి selectp = VN_AS(nodep, CoverBinsof);
        స్థిరరాశి Cross_Binsof_లక్ష్యం లక్ష్యం
            = resolve_Binsof_లక్ష్యం(selectp, ctx.crossp, ctx.cpVars, ctx.dimensions);
        ఒకవేళ (!లక్ష్యం.m_binsp) {
            ctx.valid = అబద్ధం;
            తిరిగి {};
        }
        స్థిరరాశి CoverpointBins& bins = *లక్ష్యం.m_binsp;
        // Coverpoints with sized arrays are runtime points, so feed only runtime crosses
        UASSERT_OBJ(లక్ష్యం.m_sized < 0, selectp, "Sized bin array selected by a static cross");
        స్థిరరాశి std::సదిశ<తార్కిక> selected
            = selectCoverpointBins(selectp, bins, లక్ష్యం.m_మొదటి, లక్ష్యం.m_లెక్క, ctx.valid);
        ఒకవేళ (!ctx.valid) తిరిగి {};
        CrossSelection ఫలితం(VL_BITWORD_Q(static_cast<uint64_t>(ctx.tuples) + VL_QUADSIZE - 1),
                              0);
        స్థిరరాశి uint64_t stride = ctx.strides[లక్ష్యం.m_dimension];
        స్థిరరాశి uint64_t period = stride * bins.మొత్తం;
        కొరకు (uint64_t పునాది = 0; పునాది < ctx.tuples; పునాది += period) {
            కొరకు (uint32_t i = 0; i < bins.మొత్తం;) {
                ఒకవేళ (!selected[i]) {
                    ++i;
                    కొనసాగు;
                }
                స్థిరరాశి uint32_t begin = i++;
                అయితే (i < bins.మొత్తం && selected[i]) ++i;
                సమితి_Cross_Selection_Range(ఫలితం, పునాది + begin * stride, పునాది + i * stride);
            }
        }
        తిరిగి ఫలితం;
    }

    // Size the Cartesian product of the declared Normal bins, with each dimension's flat-index
    // stride.  Live runtime bins only shrink it.  Warns and returns false if it is too large.
    తార్కిక crossShape(AstCoverCross* crossp, స్థిరరాశి std::సదిశ<AstVar*>& cpVars,
                    std::సదిశ<uint32_t>& strides, uint32_t& tuples) స్థిరరాశి {
        uint64_t లబ్ధం = std::any_యొక్క(cpVars.begin(), cpVars.చివర(),
                                       [ఇది](AstVar* varp) { తిరిగి !m_cpBins.at(varp).మొత్తం; })
                               ? 0
                               : 1;
        strides.resize(cpVars.కొలత());
        కొరకు (కొలత_t d = cpVars.కొలత(); d > 0; --d) {
            strides[d - 1] = static_cast<uint32_t>(లబ్ధం);
            లబ్ధం *= m_cpBins.at(cpVars[d - 1]).మొత్తం;
            ఒకవేళ (లబ్ధం > UINT32_MAX) {
                crossp->v3warn(COVERIGN,
                               "Unsupported: cross coverage with more than 2^32-1 tuples.");
                తిరిగి అబద్ధం;
            }
        }
        tuples = static_cast<uint32_t>(లబ్ధం);
        తిరిగి నిజం;
    }

    CrossLayout resolveCrossLayout(AstCoverCross* crossp, స్థిరరాశి std::సదిశ<AstVar*>& cpVars,
                                   స్థిరరాశి std::map<std::string, uint32_t>& dimensions) {
        CrossLayout layout;
        CrossSelectionContext ctx{crossp, cpVars, dimensions, 0, {}};
        ఒకవేళ (!crossShape(crossp, cpVars, ctx.strides, ctx.tuples)) {
            layout.valid = అబద్ధం;
            తిరిగి layout;
        }
        layout.tuples = ctx.tuples;
        CrossSelection occupied;
        CrossSelection excluded;
        std::సమితి<std::string> పేర్లు;
        కొరకు (Ast_కణుపు* itemp = crossp->binsp(); itemp; itemp = itemp->nextp()) {
            AstCoverCrossBin* స్థిరరాశి binp = VN_AS(itemp, CoverCrossBin);
            ఒకవేళ (!checkCrossBinName(binp, పేర్లు)) కొనసాగు;
            ctx.valid = నిజం;
            CrossSelection selection = crossSelection(binp->selectp(), ctx);
            ఒకవేళ (!ctx.valid || std::all_యొక్క(selection.begin(), selection.చివర(), [](uint64_t మాట) {
                    తిరిగి మాట == 0;
                })) {
                కొనసాగు;
            }
            ఒకవేళ (occupied.ఖాళీలు()) {
                occupied.resize(selection.కొలత(), 0);
                excluded.resize(selection.కొలత(), 0);
            }
            కొరకు (కొలత_t i = 0; i < selection.కొలత(); ++i) {
                occupied[i] |= selection[i];
                ఒకవేళ (!binp->binsType().bin_Is_సాధారణ()) excluded[i] |= selection[i];
            }
            layout.bins.నెట్టు_వెనుక({binp, std::move(selection)});
        }
        ఒకవేళ (!layout.bins.ఖాళీలు()) {
            // IEEE 1800-2023 19.6.2/19.6.3: exclusions also remove tuples from named
            // bins, independently of declaration order and sampling guards.
            కొరకు (ResolvedCrossBin& resolved : layout.bins) {
                కొరకు (కొలత_t i = 0; i < resolved.selection.కొలత(); ++i) {
                    ఒకవేళ (resolved.binp->binsType().bin_Is_సాధారణ()) {
                        resolved.selection[i] &= ~excluded[i];
                    }
                    ఒకవేళ (resolved.selection[i]) ++layout.bin_పదాలు;
                }
            }
            layout.bins.erase(std::remove_if(layout.bins.begin(), layout.bins.చివర(),
                                             [](స్థిరరాశి ResolvedCrossBin& resolved) {
                                                 తిరిగి std::all_యొక్క(
                                                     resolved.selection.begin(),
                                                     resolved.selection.చివర(),
                                                     [](uint64_t మాట) { తిరిగి మాట == 0; });
                                             }),
                              layout.bins.చివర());
            layout.autoBins = layout.tuples;
            కొరకు (స్థిరరాశి uint64_t మాట : occupied) {
                layout.autoBins -= static_cast<uint32_t>(std::bitset<VL_QUADSIZE>{మాట}.లెక్క());
            }
        }
        తిరిగి layout;
    }

    AstCoverCrossDType* crossDType(File_పంక్తి* fl, uint32_t dimensions, స్థిరరాశి CrossLayout& layout,
                                   తార్కిక dynamic = అబద్ధం) {
        స్థిరరాశి uint32_t bins = static_cast<uint32_t>(layout.bins.కొలత());
        స్థిరరాశి CrossShape shape{dimensions,      layout.tuples,   bins,
                               layout.autoBins, layout.bin_పదాలు, dynamic};
        AstCoverCrossDType*& typep = m_cxDTypes[shape];
        ఒకవేళ (!typep) {
            typep = కొత్త AstCoverCrossDType{
                fl, dimensions, layout.tuples, bins, layout.autoBins, layout.bin_పదాలు, dynamic};
            v3Global.rootp()->typeTablep()->కూడు_Typesp(typep);
        }
        తిరిగి typep;
    }

    std::సదిశ<AstCoverCrossBin*> generateCrossBins(AstCoverCross* crossp, AstVar* cxVarp,
                                                     స్థిరరాశి CrossLayout& layout) {
        std::సదిశ<AstCoverCrossBin*> bins;
        కొరకు (స్థిరరాశి ResolvedCrossBin& resolved : layout.bins) {
            AstCoverCrossBin* స్థిరరాశి binp = resolved.binp;
            స్థిరరాశి CrossSelection& selection = resolved.selection;
            File_పంక్తి* స్థిరరాశి fl = binp->fileline();
            స్థిరరాశి తార్కిక prot = v3Global.opt.protectIds();
            std::string ముసుగు = "{";
            కొరకు (కొలత_t i = 0; i < selection.కొలత(); ++i) {
                ఒకవేళ (i) ముసుగు += ", ";
                ముసుగు += std::గమ్యం_string(selection[i]) + "ULL";
            }
            ముసుగు += "}";
            m_constructorp->కూడు_Stmtsp(
                అంశం_Call(fl, cxVarp, VCMethod::COVERGROUP_కూడు_BIN,
                         {ctext(fl, binp->binsType().bin_సమితి_Enum()), ctext(fl, ముసుగు),
                          ctext(fl, quoted(VIdProtect::protect_పదాలు_If(binp->name(), prot))),
                          ctext(fl, quoted(VIdProtect::protectIf(fl->filename(), prot))),
                          cnum(fl, static_cast<uint32_t>(fl->lineno())),
                          cnum(fl, static_cast<uint32_t>(fl->మొదటి_స్తంభం()))})
                    ->makeStmt());
            bins.నెట్టు_వెనుక(binp);
        }
        ఒకవేళ (!bins.ఖాళీలు()) {
            m_constructorp->కూడు_Stmtsp(
                అంశం_Call(crossp->fileline(), cxVarp, VCMethod::COVERGROUP_FINALIZE_BINS)
                    ->makeStmt());
        }
        తిరిగి bins;
    }

    // Route a cross through a VlCoverCross member: emit the member, its constructor init +
    // registration, and the sample() call.  The feeding coverpoints are already generated
    // (their hit lists drive the cross). Each explicit bin adds one configuration call.
    శూన్య generateCross(AstCoverCross* crossp) {
        File_పంక్తి* స్థిరరాశి fl = crossp->fileline();
        స్థిరరాశి తార్కిక dynamic = m_runtimeCrosses.లెక్క(crossp);
        UINFO(4, "  Generating VlCoverCross member: " << crossp->name());

        ఒకవేళ (Ast_కణుపు_సమాసం* స్థిరరాశి iffp = crossp->iffp()) {
            crossp->iffp(
                capture_Iff_గమ్యం_తాత్కాలిక(iffp, "__VcrossIff_" + sanitizeGeneratedName(crossp->name())));
        }

        // Resolve and unlink the coverpoint refs, in dimension order.  Every ref resolves to a
        // known coverpoint (a cross with an unresolvable item was dropped earlier).
        std::సదిశ<AstVar*> cpVars;
        std::map<std::string, uint32_t> dimensions;
        కొరకు (Ast_కణుపు* itemp = crossp->itemsp(); itemp;) {
            Ast_కణుపు* స్థిరరాశి nextp = itemp->nextp();
            AstCoverpointRef* స్థిరరాశి refp = VN_AS(itemp, CoverpointRef);
            స్థిరరాశి స్వయంచాలక it = m_cpVarMap.కనుగొను(refp->name());
            UASSERT_OBJ(it != m_cpVarMap.చివర(), crossp, "Cross references an unknown coverpoint");
            dimensions.emplace(refp->name(), static_cast<uint32_t>(cpVars.కొలత()));
            cpVars.నెట్టు_వెనుక(it->రెండవ);
            VL_DO_DANGLING(నెట్టు_Deletep(refp->unlink_Fr_వెనుక()), refp);
            itemp = nextp;
        }
        స్థిరరాశి పూర్ణాంకం dims = static_cast<పూర్ణాంకం>(cpVars.కొలత());
        CrossLayout layout;
        ఒకవేళ (dynamic) {
            // The runtime sizes the layout from live bins; only check the declared bound here.
            std::సదిశ<uint32_t> strides;
            uint32_t tuples = 0;
            layout.valid = crossShape(crossp, cpVars, strides, tuples);
        } లేకపోతే {
            layout = resolveCrossLayout(crossp, cpVars, dimensions);
        }
        ఒకవేళ (!layout.valid) తిరిగి;

        AstVar* స్థిరరాశి cxVarp
            = కొత్త AstVar{fl, VVarType::MEMBER, "__Vcx_" + crossp->name(),
                         crossDType(fl, static_cast<uint32_t>(dims), layout, dynamic)};
        m_covergroupp->కూడు_Membersp(cxVarp);
        m_constructorp->కూడు_Stmtsp(make_అంశం_Create(fl, cxVarp,
                                                 dynamic ? VCMethod::COVERGROUP_కూడు_CROSS_DYN
                                                         : VCMethod::COVERGROUP_కూడు_CROSS));
        generate_అంశం_బరువు(fl, cxVarp, crossp->optionsp());
        పూర్ణాంకం at_కనిష్ఠ_విలువ;
        పూర్ణాంకం autoBinMax;  // Not a cross option
        వెలికితీయి_అంశం_Options(crossp->optionsp(), at_కనిష్ఠ_విలువ, autoBinMax);

        // Constructor: init (after the coverpoints, which generate earlier) then registration.
        // Obfuscate the hierarchy/filename/page under --protect-ids as for coverpoints above.
        స్థిరరాశి తార్కిక prot = v3Global.opt.protectIds();
        స్థిరరాశి std::string hier
            = VIdProtect::protect_పదాలు_If(m_covergroupName + "." + crossp->name(), prot);
        m_constructorp->కూడు_Stmtsp(makeCrossCpsCall(
            fl, cpVars,
            అంశం_Call(fl, cxVarp, VCMethod::COVERGROUP_INIT,
                     {ctext(fl, quoted(hier)), cnum(fl, static_cast<uint32_t>(at_కనిష్ఠ_విలువ)),
                      cnum(fl, static_cast<uint32_t>(dims)), ctext(fl, "__Vcx_cps"),
                      ctext(fl, quoted(VIdProtect::protectIf(fl->filename(), prot))),
                      cnum(fl, static_cast<uint32_t>(fl->lineno())),
                      cnum(fl, static_cast<uint32_t>(fl->మొదటి_స్తంభం()))})));
        స్థిరరాశి std::సదిశ<AstCoverCrossBin*> bins
            = dynamic ? generateRuntimeCrossBins(crossp, cxVarp, cpVars, dimensions)
                      : generateCrossBins(crossp, cxVarp, layout);
        ఒకవేళ (v3Global.opt.coverage()) {
            స్థిరరాశి std::string పుట
                = VIdProtect::protectIf("v_covergroup/" + m_covergroupName, prot);
            m_constructorp->కూడు_Stmtsp(అంశం_Call(fl, cxVarp, VCMethod::COVERGROUP_రిజిస్టరు_BINS,
                                               {ctext(fl, "vlSymsp->_vm_contextp__->coveragep()"),
                                                ctext(fl, quoted(పుట)),
                                                cnum(fl, అంశం_Database_బరువు(crossp->optionsp())),
                                                cnum(fl, m_cg_Type_బరువు)})
                                          ->makeStmt());
        }

        // sample(): after all coverpoints have sampled (cross loop runs after coverpoint loop).
        UASSERT_OBJ(m_sampleFuncp, crossp, "sample() CFunc not set for cross");
        // The cross remembers its feeding coverpoints, so sample() needs no cps array;
        // per-bin iff guards still need a temporary array, hence the block form.
        స్థిరరాశి తార్కిక hasIffs
            = std::any_యొక్క(bins.begin(), bins.చివర(),
                          [](స్థిరరాశి AstCoverCrossBin* binp) { తిరిగి binp->iffp() != శూన్యసూచిక; });
        Ast_కణుపు_Stmt* స్థిరరాశి samplep
            = !hasIffs ? static_cast<Ast_కణుపు_Stmt*>(
                             అంశం_Call(fl, cxVarp, VCMethod::COVERGROUP_SAMPLE)->makeStmt())
                       : static_cast<Ast_కణుపు_Stmt*>(makeCrossIffsCall(
                             fl, bins,
                             అంశం_Call(fl, cxVarp, VCMethod::COVERGROUP_SAMPLE_IFFS,
                                      {ctext(fl, "__Vcx_iffs")})));
        ఒకవేళ (Ast_కణుపు_సమాసం* స్థిరరాశి iffp = crossp->iffp()) {
            m_sampleFuncp->కూడు_Stmtsp(కొత్త AstIf{fl, iffp->clone_వృక్షం(అబద్ధం), samplep});
        } లేకపోతే {
            m_sampleFuncp->కూడు_Stmtsp(samplep);
        }
    }

    శూన్య generate_Cross_సంకేతం(AstCoverCross* crossp) {
        UINFO(4, "  Generating code for cross: " << crossp->name());

        // Non-standard hierarchical/dotted cross item (e.g. 'cross a.b'): an implicit coverpoint
        // over the referenced expression (carried in refp->exprp()).  The grammar already warned
        // NONSTD; implicit coverpoints are not yet implemented, so generate no sampling code for
        // this cross.  When support is added the implicit coverpoint should be synthesized
        // upstream (V3LinkParse) as a real AstCoverpoint so it flows through the normal coverpoint
        // path - by here coverpoint lowering has already run.
        కొరకు (Ast_కణుపు* itemp = crossp->itemsp(); itemp; itemp = itemp->nextp()) {
            స్థిరరాశి AstCoverpointRef* స్థిరరాశి refp = VN_AS(itemp, CoverpointRef);
            ఒకవేళ (refp->exprp()) {
                refp->v3warn(COVERIGN,
                             "Unsupported: cross of hierarchical reference (implicit coverpoint)");
                తిరిగి;
            }
        }

        // A cross naming a bare variable (implicit coverpoint, which Verilator does not
        // synthesize) is dropped entirely with a COVERIGN warning -- it produces no coverage
        // either way -- but only this cross is dropped; its sibling crosses are still generated
        // and the real coverpoints it referenced remain as independent coverpoints.
        ఒకవేళ (m_droppedCrosses.లెక్క(crossp)) {
            కొరకు (Ast_కణుపు* itemp = crossp->itemsp(); itemp; itemp = itemp->nextp()) {
                స్థిరరాశి AstCoverpointRef* స్థిరరాశి refp = VN_AS(itemp, CoverpointRef);
                ఒకవేళ (m_coverpointMap.కనుగొను(refp->name()) == m_coverpointMap.చివర()) {
                    refp->v3warn(COVERIGN, "Unsupported: cross of "
                                               << refp->prettyNameQ()
                                               << " which is not a coverpoint (implicit "
                                                  "coverpoint)");
                    ఆపు;
                }
            }
            తిరిగి;
        }

        // Every cross that isn't dropped routes through a VlCoverCross member.
        generateCross(crossp);
    }

    Ast_కణుపు_సమాసం* buildBinCondition(AstCoverBin* binp, Ast_కణుపు_సమాసం* exprp) {
        // Get the range list from the bin
        Ast_కణుపు* స్థిరరాశి rangep = binp->rangesp();
        ఒకవేళ (!rangep) తిరిగి శూన్యసూచిక;

        // Integral values resolve to the coverpoint's type, as its runtime metadata does
        స్థిరరాశి తార్కిక integral = exprp->dtypep()->skipRefp()->isIntegralOrPacked();
        // No value form of a wildcard bin is allowed on a real coverpoint (IEEE 1800-2023 19.5.4)
        ఒకవేళ (binp->isWildcard() && !integral) తిరిగి wildcardTypeError(binp, exprp);

        // Build condition by OR-ing all ranges together
        Ast_కణుపు_సమాసం* నిండిన_Condp = శూన్యసూచిక;

        కొరకు (Ast_కణుపు* currRangep = rangep; currRangep; currRangep = currRangep->nextp()) {
            Ast_కణుపు_సమాసం* rangeCondp = శూన్యసూచిక;
            currRangep = V3Const::constifyEdit(currRangep);

            ఒకవేళ (AstInsideRange* irp = VN_CAST(currRangep, InsideRange)) {
                Ast_కణుపు_సమాసం* స్థిరరాశి minExprp = irp->lhsp();
                Ast_కణుపు_సమాసం* స్థిరరాశి maxExprp = irp->rhsp();
                AstConst* స్థిరరాశి minConstp = VN_CAST(minExprp, Const);
                AstConst* స్థిరరాశి maxConstp = VN_CAST(maxExprp, Const);
                స్థిరరాశి తార్కిక loUnbounded = VN_IS(minExprp, Unbounded);
                స్థిరరాశి తార్కిక hiUnbounded = VN_IS(maxExprp, Unbounded);
                ఒకవేళ (loUnbounded || hiUnbounded) {
                    // Open-ended range: '$' is the coverpoint domain min/max, so the
                    // range reduces to a single inequality (e.g. {[10:$]} -> expr >= 10).
                    AstConst* స్థిరరాశి boundp = hiUnbounded ? minConstp : maxConstp;
                    ఒకవేళ (loUnbounded && hiUnbounded) {
                        rangeCondp = కొత్త AstConst{irp->fileline(), AstConst::బిట్టు_True{}};
                    } లేకపోతే ఒకవేళ (!boundp) {
                        irp->v3error("Non-constant expression in bin range; "
                                     "range bounds must be constants (IEEE 1800-2023 19.5)");
                        ఒకవేళ (నిండిన_Condp) VL_DO_DANGLING(నెట్టు_Deletep(నిండిన_Condp), నిండిన_Condp);
                        తిరిగి శూన్యసూచిక;
                    } లేకపోతే ఒకవేళ (boundp->అంకె().isFourState()) {
                        irp->v3error("Four-state (x/z) value in bin range bound; "
                                     "range bounds must be two-state constants");
                        ఒకవేళ (నిండిన_Condp) VL_DO_DANGLING(నెట్టు_Deletep(నిండిన_Condp), నిండిన_Condp);
                        తిరిగి శూన్యసూచిక;
                    } లేకపోతే ఒకవేళ (integral) {
                        rangeCondp = build_విలువ_Condition(binp, exprp, irp);
                    } లేకపోతే {
                        rangeCondp = makeOpenRangeCondition(irp->fileline(), exprp, boundp,
                                                            /*isLowerBound=*/hiUnbounded);
                    }
                } లేకపోతే ఒకవేళ (!minConstp || !maxConstp) {
                    irp->v3error("Non-constant expression in bin range; "
                                 "range bounds must be constants (IEEE 1800-2023 19.5)");
                    ఒకవేళ (నిండిన_Condp) VL_DO_DANGLING(నెట్టు_Deletep(నిండిన_Condp), నిండిన_Condp);
                    తిరిగి శూన్యసూచిక;
                } లేకపోతే ఒకవేళ (minConstp->అంకె().isFourState() || maxConstp->అంకె().isFourState()) {
                    irp->v3error("Four-state (x/z) value in bin range bound; "
                                 "range bounds must be two-state constants");
                    ఒకవేళ (నిండిన_Condp) VL_DO_DANGLING(నెట్టు_Deletep(నిండిన_Condp), నిండిన_Condp);
                    తిరిగి శూన్యసూచిక;
                } లేకపోతే ఒకవేళ (integral) {
                    rangeCondp = build_విలువ_Condition(binp, exprp, irp);
                } లేకపోతే {
                    rangeCondp = makeRangeCondition(irp->fileline(), exprp, minExprp, maxExprp);
                }
            } లేకపోతే ఒకవేళ (AstConst* constp = VN_CAST(currRangep, Const)) {
                rangeCondp = build_విలువ_Condition(binp, exprp, constp);
            } లేకపోతే {
                currRangep->v3error("Non-constant expression in bin range; values must be "
                                    "constants (IEEE 1800-2023 19.5)");
                ఒకవేళ (నిండిన_Condp) VL_DO_DANGLING(నెట్టు_Deletep(నిండిన_Condp), నిండిన_Condp);
                తిరిగి శూన్యసూచిక;
            }

            UASSERT_OBJ(rangeCondp, binp, "rangeCondp is null after building range condition");
            నిండిన_Condp
                = నిండిన_Condp ? కొత్త AstOr{binp->fileline(), నిండిన_Condp, rangeCondp} : rangeCondp;
        }

        తిరిగి నిండిన_Condp;
    }

    // Wildcard bits have no meaning for a non-integral coverpoint.
    స్థిర Ast_కణుపు_సమాసం* wildcardTypeError(AstCoverBin* binp, Ast_కణుపు_సమాసం* exprp) {
        స్థిరరాశి Ast_కణుపు_DType* స్థిరరాశి dtypep = exprp->dtypep()->skipRefp();
        exprp->v3error("Cannot use a wildcard bin on a coverpoint of type "
                       << dtypep->prettyDTypeNameQ() << " (IEEE 1800-2023 19.5.4).\n"
                       << exprp->warnContextPrimary() << '\n'
                       << binp->warnOther() << "... Location of wildcard bin\n"
                       << binp->warnContextSecondary());
        తిరిగి కొత్త AstConst{binp->fileline(), AstConst::బిట్టు_False{}};
    }

    // Match one bin value, range, or wildcard pattern.  Integral coverpoints first resolve the
    // value to their type (IEEE 1800-2023 19.5.7).  Non-owning: clones what it uses.
    Ast_కణుపు_సమాసం* build_విలువ_Condition(AstCoverBin* binp, Ast_కణుపు_సమాసం* exprp, Ast_కణుపు* valuep) {
        File_పంక్తి* స్థిరరాశి fl = valuep->fileline();
        ఒకవేళ (!exprp->dtypep()->skipRefp()->isIntegralOrPacked()) {
            తిరిగి కొత్త AstEq{fl, exprp->clone_వృక్షం(అబద్ధం),
                             VN_AS(valuep, కణుపు_సమాసం)->clone_వృక్షం(అబద్ధం)};
        }
        Cross_విలువ_Range range{valuep, resolve_వెడల్పు(valuep, exprp)};
        ఒకవేళ (!resolve_విలువ(valuep, exprp, నిజం, binp->isWildcard(), range)) {
            valuep->v3warn(E_UNSUPPORTED,
                           "Unsupported: non-integral value in a coverage bin of an "
                           "integral coverpoint.");
            తిరిగి కొత్త AstConst{fl, AstConst::బిట్టు_False{}};
        }
        ఒకవేళ (cross_Range_ఖాళీలు(range)) తిరిగి కొత్త AstConst{fl, AstConst::బిట్టు_False{}};
        AstConst* స్థిరరాశి lop = new_విలువ_Const(fl, range.lo, exprp);
        AstConst* స్థిరరాశి hip = new_విలువ_Const(fl, range.hi, exprp);
        Ast_కణుపు_సమాసం* condp = శూన్యసూచిక;
        ఒకవేళ (lop->అంకె().isCaseEq(hip->అంకె())) {
            condp = కొత్త AstEq{fl, exprp->clone_వృక్షం(అబద్ధం), lop};
        } లేకపోతే {
            condp = makeRangeCondition(fl, exprp, lop, hip);
            VL_DO_DANGLING(నెట్టు_Deletep(lop), lop);
        }
        VL_DO_DANGLING(నెట్టు_Deletep(hip), hip);
        ఒకవేళ (!range.wildcard) తిరిగి condp;
        // Match the pattern's value bits within the source-value bounds.
        V3_సంఖ్య ముసుగు{valuep, exprp->వెడల్పు()};
        V3_సంఖ్య విలువ{valuep, exprp->వెడల్పు()};
        ముసుగు.op_బిట్లు_Non_XZ(range.నమూనా_2);
        విలువ.op_బిట్లు_One(range.నమూనా_2);
        AstConst* స్థిరరాశి ముసుగు_Constp = new_విలువ_Const(fl, ముసుగు, exprp);
        AstConst* స్థిరరాశి విలువ_Constp = new_విలువ_Const(fl, విలువ, exprp);
        Ast_కణుపు_సమాసం* స్థిరరాశి సమాసం_Masked = కొత్త AstAnd{fl, exprp->clone_వృక్షం(అబద్ధం), ముసుగు_Constp};
        Ast_కణుపు_సమాసం* స్థిరరాశి విలువ_Masked = కొత్త AstAnd{fl, విలువ_Constp, ముసుగు_Constp->clone_వృక్షం(అబద్ధం)};
        తిరిగి కొత్త AstLogAnd{fl, condp, కొత్త AstEq{fl, సమాసం_Masked, విలువ_Masked}};
    }

    శూన్య generate_Coverage_Computation_సంకేతం() {
        UINFO(4, "  Generating coverage computation code");

        // Invalidate cache: addMembersp() calls in generateCoverpointCode/generateCrossCode
        // have added new members since the last scan, so clear before re-querying.
        m_memberMap.clear();

        // get_inst_coverage(): the average of the coverpoints and crosses, weighted by their
        // option.weight (IEEE 1800-2023 19.11).  The instance node holds their runtimes.  With
        // the instances merged, get_coverage() instead, unless option.get_inst_coverage
        // (Table 19-1).
        AstFunc* స్థిరరాశి getInstCoveragep
            = VN_AS(m_memberMap.కనుగొను_Member(m_covergroupp, "get_inst_coverage"), Func);
        File_పంక్తి* స్థిరరాశి instFl = getInstCoveragep->fileline();
        AstCMethodHard* స్థిరరాశి instCallp = instanceCall(instFl, VCMethod::COVERGROUP_COVERAGE);
        instCallp->dtype_సమితి_Double();
        Ast_కణుపు_సమాసం* instValuep = instCallp;
        ఒకవేళ (m_cg_May_విలీనం) {
            Ast_కణుపు_సమాసం* స్థిరరాశి mergedp = కొత్త AstLogAnd{
                instFl, newOptionSel(instFl, optionVar(నిజం), "merge_instances", VAccess::READ),
                కొత్త AstLogNot{instFl, newOptionSel(instFl, optionVar(అబద్ధం), "get_inst_coverage",
                                                   VAccess::READ)}};
            instValuep = కొత్త AstCond{instFl, mergedp, typeCoverageCall(instFl), instCallp};
        }
        getInstCoveragep->కూడు_Stmtsp(కొత్త Ast_కేటాయించు{
            instFl, కొత్త AstVarRef{instFl, VN_AS(getInstCoveragep->fvarp(), Var), VAccess::WRITE},
            instValuep});

        // get_coverage(): the average of the covergroup's instances, weighted by their
        // option.weight, or with type_option.merge_instances, the coverage of the union of their
        // bins (IEEE 1800-2023 19.11.3).  Static, so the registry finds the instances.
        AstFunc* స్థిరరాశి getCoveragep
            = VN_AS(m_memberMap.కనుగొను_Member(m_covergroupp, "get_coverage"), Func);
        File_పంక్తి* స్థిరరాశి typeFl = getCoveragep->fileline();
        getCoveragep->కూడు_Stmtsp(కొత్త Ast_కేటాయించు{
            typeFl, కొత్త AstVarRef{typeFl, VN_AS(getCoveragep->fvarp(), Var), VAccess::WRITE},
            typeCoverageCall(typeFl)});
    }

    // The registry call computing the covergroup's type coverage, per its type options
    AstCMethodHard* typeCoverageCall(File_పంక్తి* fl) {
        AstCExpr* స్థిరరాశి registryp = ctext(fl, "vlSymsp->_vm_contextp__->covergroupRegistryp()");
        registryp->dtype_సమితి_Void();  // Opaque receiver; only ever the 'fromp' of the call below
        AstCMethodHard* స్థిరరాశి callp
            = కొత్త AstCMethodHard{fl, registryp, VCMethod::COVERGROUP_TYPE_COVERAGE};
        callp->కూడు_Pinsp(ctext(fl, quoted(covergroupProtectedName())));
        callp->కూడు_Pinsp(newOptionSel(fl, optionVar(నిజం), "weight", VAccess::READ));
        callp->కూడు_Pinsp(newOptionSel(fl, optionVar(నిజం), "merge_instances", VAccess::READ));
        callp->కూడు_Pinsp(file_పంక్తి_Debug(m_covergroupp->fileline()));
        callp->usePtr(నిజం);
        callp->dtype_సమితి_Double();
        తిరిగి callp;
    }

    // VISITORS
    స్థిర తార్కిక isEnclosingInstanceVar(స్థిరరాశి AstVar* varp) {
        తిరిగి varp->isClassMember() && !varp->lifetime().isStatic() && !varp->isParam();
    }

    శూన్య rewriteThisRef(AstThisRef* refp, AstVar* handleVarp) {
        స్థిరరాశి AstClassRefDType* స్థిరరాశి refDTypep
            = VN_CAST(refp->dtypep()->skipRefp(), ClassRefDType);
        UASSERT_OBJ(refDTypep && refDTypep->classp() == m_covergroupp, refp,
                    "Unexpected this reference in embedded covergroup");
        Ast_కణుపు_సమాసం* స్థిరరాశి newp = కొత్త AstVarRef{refp->fileline(), handleVarp, VAccess::READ};
        refp->replaceWith(newp);
        VL_DO_DANGLING(నెట్టు_Deletep(refp), refp);
    }

    శూన్య rewriteVarRef(AstVarRef* refp, AstVar* handleVarp) {
        File_పంక్తి* స్థిరరాశి fl = refp->fileline();
        // As V3LinkLValue does, the handle of a written member select is marked written
        AstMemberSel* స్థిరరాశి selp
            = కొత్త AstMemberSel{fl, కొత్త AstVarRef{fl, handleVarp, refp->access()}, refp->varp()};
        selp->access(refp->access());
        refp->replaceWith(selp);
        VL_DO_DANGLING(నెట్టు_Deletep(refp), refp);
    }

    శూన్య rewriteFuncRef(AstFuncRef* refp, AstVar* handleVarp) {
        File_పంక్తి* స్థిరరాశి fl = refp->fileline();
        AstArg* స్థిరరాశి argsp = refp->argsp() ? refp->argsp()->unlink_Fr_వెనుక_With_తదుపరి() : శూన్యసూచిక;
        Ast_పద్ధతి_Call* స్థిరరాశి callp = కొత్త Ast_పద్ధతి_Call{
            fl, కొత్త AstVarRef{fl, handleVarp, VAccess::READ}, refp->name(), argsp};
        callp->taskp(refp->taskp());
        callp->dtypeFrom(refp);
        refp->replaceWith(callp);
        VL_DO_DANGLING(నెట్టు_Deletep(refp), refp);
    }

    // True if funcp is an instance method of the enclosing class or one of its bases
    తార్కిక isEnclosingInstanceFunc(స్థిరరాశి Ast_కణుపు_FTask* funcp) స్థిరరాశి {
        ఒకవేళ (!funcp->class_పద్ధతి() || funcp->isStatic()) తిరిగి అబద్ధం;
        తిరిగి AstClass::isClassExtendedFrom(m_enclosingClassp, VN_AS(funcp->aboveLoopp(), Class));
    }

    తార్కిక isEmbeddedCovergroupVar(స్థిరరాశి AstVar* varp) స్థిరరాశి {
        ఒకవేళ (!varp || !varp->isClassMember() || varp->isDeclTyped()) తిరిగి అబద్ధం;
        స్థిరరాశి AstClassRefDType* స్థిరరాశి refp = VN_CAST(varp->dtypep()->skipRefp(), ClassRefDType);
        తిరిగి refp && refp->classp() == m_covergroupp;
    }

    AstVar* కనుగొను_Embedded_Covergroup_Var() స్థిరరాశి {
        ఒకవేళ (!m_enclosingClassp) తిరిగి శూన్యసూచిక;
        కొరకు (Ast_కణుపు* itemp = m_enclosingClassp->membersp(); itemp; itemp = itemp->nextp()) {
            ఒకవేళ (AstVar* స్థిరరాశి varp = VN_CAST(itemp, Var)) {
                ఒకవేళ (isEmbeddedCovergroupVar(varp)) తిరిగి varp;
            }
        }
        // V3LinkParse always creates an implicit variable for an embedded covergroup.
        తిరిగి శూన్యసూచిక;  // LCOV_EXCL_LINE
    }

    std::సదిశ<Ast_కణుపు_కేటాయించు*> కనుగొను_Covergroup_Constructions() {
        std::సదిశ<Ast_కణుపు_కేటాయించు*> foundps;
        ఒకవేళ (!m_embeddedVarp) తిరిగి foundps;
        AstFunc* స్థిరరాశి enclosingNewp
            = VN_CAST(m_memberMap.కనుగొను_Member(m_enclosingClassp, "new"), Func);
        ఒకవేళ (!enclosingNewp) తిరిగి foundps;
        enclosingNewp->foreach([&](Ast_కణుపు_కేటాయించు* asgnp) {
            స్థిరరాశి AstNew* స్థిరరాశి newp = VN_CAST(asgnp->rhsp(), New);
            స్థిరరాశి AstVarRef* స్థిరరాశి lhsRefp = VN_CAST(asgnp->lhsp(), VarRef);
            ఒకవేళ (!newp || !lhsRefp || lhsRefp->varp() != m_embeddedVarp) తిరిగి;
            స్థిరరాశి AstClassRefDType* స్థిరరాశి refp = VN_CAST(newp->dtypep(), ClassRefDType);
            ఒకవేళ (refp && refp->classp() == m_covergroupp) foundps.నెట్టు_వెనుక(asgnp);
        });
        తిరిగి foundps;
    }

    std::సమితి<స్థిరరాశి AstVar*> enclosingInstanceVars() స్థిరరాశి {
        std::సమితి<స్థిరరాశి AstVar*> vars;
        ఒకవేళ (m_enclosingClassp) {
            m_enclosingClassp->foreachMember([&](AstClass* స్థిరరాశి, AstVar* స్థిరరాశి varp) {
                ఒకవేళ (isEnclosingInstanceVar(varp)) vars.చొప్పించు(varp);
            });
        }
        తిరిగి vars;
    }

    తార్కిక hasEnclosingEventRef(AstCovergroup* cgp) స్థిరరాశి {
        ఒకవేళ (!m_embeddedVarp || !cgp->eventp()) తిరిగి అబద్ధం;
        స్థిరరాశి std::సమితి<స్థిరరాశి AstVar*> enclosingVars = enclosingInstanceVars();
        తార్కిక దొరికింది = అబద్ధం;
        cgp->eventp()->foreach([&](AstVarRef* refp) {
            ఒకవేళ (enclosingVars.లెక్క(refp->varp())) దొరికింది = నిజం;
        });
        తిరిగి దొరికింది;
    }

    స్థిర తార్కిక parse_Embedded_Event_సమాసం(Ast_కణుపు_సమాసం* exprp, AstVar*& పునాది_Varp,
                                       AstVar*& memberVarp) {
        ఒకవేళ (AstVarRef* స్థిరరాశి refp = VN_CAST(exprp, VarRef)) {
            పునాది_Varp = refp->varp();
            memberVarp = శూన్యసూచిక;
            తిరిగి నిజం;
        }
        AstMemberSel* స్థిరరాశి selp = VN_CAST(exprp, MemberSel);
        ఒకవేళ (!selp) తిరిగి అబద్ధం;
        AstVarRef* స్థిరరాశి పునాది_Refp = VN_CAST(selp->fromp(), VarRef);
        ఒకవేళ (!పునాది_Refp) తిరిగి అబద్ధం;
        పునాది_Varp = పునాది_Refp->varp();
        memberVarp = selp->varp();
        తిరిగి నిజం;
    }

    తార్కిక isEventLvalue(Ast_కణుపు_సమాసం* exprp, స్థిరరాశి EmbeddedEventTrigger& trigger) స్థిరరాశి {
        ఒకవేళ (AstSel* స్థిరరాశి selp = VN_CAST(exprp, Sel)) exprp = selp->fromp();
        AstVar* పునాది_Varp = శూన్యసూచిక;
        AstVar* memberVarp = శూన్యసూచిక;
        ఒకవేళ (!parse_Embedded_Event_సమాసం(exprp, పునాది_Varp, memberVarp)) తిరిగి అబద్ధం;
        తిరిగి పునాది_Varp == trigger.పునాది_Varp && memberVarp == trigger.memberVarp;
    }

    Ast_కణుపు_సమాసం* newEventRead(File_పంక్తి* fl, స్థిరరాశి EmbeddedEventTrigger& trigger) స్థిరరాశి {
        Ast_కణుపు_సమాసం* స్థిరరాశి basep = కొత్త AstVarRef{fl, trigger.పునాది_Varp, VAccess::READ};
        ఒకవేళ (!trigger.memberVarp) తిరిగి basep;
        AstMemberSel* స్థిరరాశి selp = కొత్త AstMemberSel{fl, basep, trigger.memberVarp};
        selp->access(VAccess::READ);
        తిరిగి selp;
    }

    string eventPrevName(స్థిరరాశి EmbeddedEventTrigger& trigger, కొలత_t trigger_సూచీ) స్థిరరాశి {
        string name = "__Vcg_prev_" + m_embeddedVarp->name() + "_" + std::గమ్యం_string(trigger_సూచీ)
                      + "_" + trigger.పునాది_Varp->name();
        ఒకవేళ (trigger.memberVarp) name += "_" + trigger.memberVarp->name();
        తిరిగి name;
    }

    Ast_కణుపు_సమాసం* newEmbeddedVarNonNull(File_పంక్తి* fl) స్థిరరాశి {
        తిరిగి కొత్త AstNeq{fl, కొత్త AstVarRef{fl, m_embeddedVarp, VAccess::READ},
                          కొత్త AstConst{fl, AstConst::Null{}}};
    }

    Ast_కణుపు_Stmt* newSampleStmt(File_పంక్తి* fl) స్థిరరాశి {
        Ast_పద్ధతి_Call* స్థిరరాశి callp = కొత్త Ast_పద్ధతి_Call{
            fl, కొత్త AstVarRef{fl, m_embeddedVarp, VAccess::READ}, "sample", శూన్యసూచిక};
        callp->taskp(m_sampleFuncp);
        callp->dtype_సమితి_Void();
        తిరిగి callp->makeStmt();
    }

    శూన్య installEmbeddedEventFork(Ast_Sen_వృక్షం* eventp,
                                  స్థిరరాశి std::సదిశ<Ast_కణుపు_కేటాయించు*>& constructps) {
        // IEEE 1800-2023 19.3 samples coverpoints whenever their clocking event occurs. A
        // per-instance event cannot use V3Active's static sensitivity path, so spawn
        // 'fork forever begin @(event); cg.sample(); end join_none' after each construction.
        కొరకు (Ast_కణుపు_కేటాయించు* స్థిరరాశి constructp : constructps) {
            File_పంక్తి* స్థిరరాశి fl = constructp->fileline();
            AstLoop* స్థిరరాశి loopp = కొత్త AstLoop{fl};
            loopp->కూడు_Stmtsp(కొత్త AstEventControl{fl, eventp->clone_వృక్షం(అబద్ధం), శూన్యసూచిక});
            loopp->కూడు_Stmtsp(కొత్త AstIf{fl, newEmbeddedVarNonNull(fl), newSampleStmt(fl)});
            AstFork* స్థిరరాశి forkp = కొత్త AstFork{fl, VJoinType::JOIN_NONE};
            forkp->తక్షణ_ప్రారంభం(నిజం);
            forkp->కూడు_Forksp(కొత్త AstBegin{fl, "", loopp, నిజం});
            constructp->కూడు_తదుపరి_ఇక్కడ(forkp);
        }
        VL_DO_DANGLING(నెట్టు_Deletep(eventp), eventp);
    }

    Ast_కణుపు_సమాసం* newEventReadyCondition(File_పంక్తి* fl, స్థిరరాశి EmbeddedEventTrigger& trigger) స్థిరరాశి {
        Ast_కణుపు_సమాసం* స్థిరరాశి curp = newEventRead(fl, trigger);
        Ast_కణుపు_సమాసం* స్థిరరాశి prevp = కొత్త AstVarRef{fl, trigger.prevVarp, VAccess::READ};
        Ast_కణుపు_సమాసం* edgep = శూన్యసూచిక;
        // IEEE 1800-2023 9.4.2 detects edge-qualified events only on the expression's LSB,
        // while an implicit change event observes the complete expression.
        ఒకవేళ (trigger.అంచు_Type == VEdgeType::ET_POSEDGE) {
            edgep = కొత్త AstSel{fl, కొత్త AstAnd{fl, curp, కొత్త AstNot{fl, prevp}}, 0, 1};
        } లేకపోతే ఒకవేళ (trigger.అంచు_Type == VEdgeType::ET_NEGEDGE) {
            edgep = కొత్త AstSel{fl, కొత్త AstAnd{fl, కొత్త AstNot{fl, curp}, prevp}, 0, 1};
        } లేకపోతే ఒకవేళ (trigger.అంచు_Type == VEdgeType::ET_BOTHEDGE) {
            edgep = కొత్త AstSel{fl, కొత్త AstXor{fl, curp, prevp}, 0, 1};
        } లేకపోతే {
            edgep = కొత్త AstNeq{fl, curp, prevp};
        }
        తిరిగి కొత్త AstLogAnd{fl, newEmbeddedVarNonNull(fl), edgep};
    }

    std::సదిశ<EmbeddedEventTrigger> collectEmbeddedEventTriggers(AstCovergroup* cgp) {
        std::సదిశ<EmbeddedEventTrigger> triggers;
        స్థిరరాశి std::సమితి<స్థిరరాశి AstVar*> enclosingVars = enclosingInstanceVars();
        కొరకు (Ast_కణుపు* senp = cgp->eventp()->sensesp(); senp; senp = senp->nextp()) {
            Ast_Sen_అంశం* స్థిరరాశి itemp = VN_AS(senp, Sen_అంశం);
            AstVar* పునాది_Varp = శూన్యసూచిక;
            AstVar* memberVarp = శూన్యసూచిక;
            ఒకవేళ (!parse_Embedded_Event_సమాసం(itemp->sensp(), పునాది_Varp, memberVarp)
                || !enclosingVars.లెక్క(పునాది_Varp)) {
                తిరిగి {};
            }
            triggers.emplace_వెనుక(itemp->fileline(), పునాది_Varp, memberVarp, itemp->అంచు_Type());
        }
        తిరిగి triggers;
    }

    శూన్య installEmbeddedEventTriggers(std::సదిశ<EmbeddedEventTrigger>& triggers,
                                      స్థిరరాశి std::సదిశ<Ast_కణుపు_కేటాయించు*>& constructps) {
        // Without --timing, approximate a per-instance event by sampling after assignments
        // within the enclosing class. External writes and exact scheduling cannot be observed.
        ఒకవేళ (constructps.ఖాళీలు()) తిరిగి;
        కొరకు (కొలత_t trigger_సూచీ = 0; trigger_సూచీ < triggers.కొలత(); ++trigger_సూచీ) {
            EmbeddedEventTrigger& trigger = triggers[trigger_సూచీ];
            std::సదిశ<Ast_కణుపు_కేటాయించు*> assignps;
            m_enclosingClassp->foreach([&](Ast_కణుపు_కేటాయించు* asgnp) {
                ఒకవేళ (isEventLvalue(asgnp->lhsp(), trigger)) assignps.నెట్టు_వెనుక(asgnp);
            });
            ఒకవేళ (assignps.ఖాళీలు()) {
                trigger.eventFl->v3warn(
                    COVERIGN, "Unsupported: 'covergroup' clocking event signal has no assignment "
                              "within the enclosing class; no coverage sampled. Use --timing for "
                              "full support.");
                కొనసాగు;
            }
            Ast_కణుపు_DType* స్థిరరాశి dtypep
                = trigger.memberVarp ? trigger.memberVarp->dtypep() : trigger.పునాది_Varp->dtypep();
            AstVar* స్థిరరాశి prevVarp = కొత్త AstVar{trigger.eventFl, VVarType::MEMBER,
                                                eventPrevName(trigger, trigger_సూచీ), dtypep};
            m_enclosingClassp->కూడు_Membersp(prevVarp);
            trigger.prevVarp = prevVarp;
            కొరకు (Ast_కణుపు_కేటాయించు* స్థిరరాశి asgnp : assignps) {
                File_పంక్తి* స్థిరరాశి fl = asgnp->fileline();
                AstIf* స్థిరరాశి ifp
                    = కొత్త AstIf{fl, newEventReadyCondition(fl, trigger), newSampleStmt(fl)};
                ifp->కూడు_తదుపరి_ఇక్కడ(కొత్త Ast_కేటాయించు{fl,
                                               కొత్త AstVarRef{fl, trigger.prevVarp, VAccess::WRITE},
                                               newEventRead(fl, trigger)});
                asgnp->కూడు_తదుపరి_ఇక్కడ(ifp);
            }
        }
    }

    శూన్య delete_Coverage_వస్తువులు() {
        కొరకు (AstCoverpoint* స్థిరరాశి cpp : m_coverpoints) {
            VL_DO_DANGLING(నెట్టు_Deletep(cpp->unlink_Fr_వెనుక()), cpp);
        }
        కొరకు (AstCoverCross* స్థిరరాశి crossp : m_coverCrosses) {
            VL_DO_DANGLING(నెట్టు_Deletep(crossp->unlink_Fr_వెనుక()), crossp);
        }
        // Options not lowered: the covergroup was not processed
        కొరకు (Ast_Cg_Option_కేటాయించు* స్థిరరాశి optp : m_cgOptions) {
            VL_DO_DANGLING(నెట్టు_Deletep(optp->unlink_Fr_వెనుక()), optp);
        }
        m_cgOptions.clear();
    }

    తరగతి FormalRefVisitor final : బహిరంగ VNVisitor {
        స్థిరరాశి std::map<స్థిరరాశి AstVar*, AstVar*>& m_replacements;

        శూన్య సందర్శించు(AstVarRef* nodep) override {
            స్థిరరాశి స్వయంచాలక it = m_replacements.కనుగొను(nodep->varp());
            ఒకవేళ (it == m_replacements.చివర()) తిరిగి;
            nodep->varp(it->రెండవ);
        }
        శూన్య సందర్శించు(Ast_కణుపు* nodep) override { iterateChildren(nodep); }

    బహిరంగ:
        స్పష్టమైన FormalRefVisitor(స్థిరరాశి std::map<స్థిరరాశి AstVar*, AstVar*>& replacements)
            : m_replacements{replacements} {}
        శూన్య పరిశీలన(Ast_కణుపు* nodep) { iterate(nodep); }
    };

    శూన్య validateCovergroupExpressions() {
        std::సమితి<స్థిరరాశి AstVar*> sampleMembers;
        std::సమితి<స్థిరరాశి AstVar*> constructorRefMembers;
        కొరకు (Ast_కణుపు* stmtp = m_constructorp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            స్థిరరాశి AstVar* స్థిరరాశి varp = VN_CAST(stmtp, Var);
            ఒకవేళ (!varp || !varp->isIO() || (!varp->isRef() && !varp->isConstRef())) కొనసాగు;
            స్థిరరాశి AstVar* స్థిరరాశి memberp
                = VN_CAST(m_memberMap.కనుగొను_Member(m_covergroupp, varp->name()), Var);
            UASSERT_OBJ(memberp && memberp->isClassMember(), varp,
                        "Covergroup constructor argument missing persistent member");
            constructorRefMembers.చొప్పించు(varp);
            constructorRefMembers.చొప్పించు(memberp);
        }
        కొరకు (Ast_కణుపు* stmtp = m_sampleFuncp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            స్థిరరాశి AstVar* స్థిరరాశి varp = VN_CAST(stmtp, Var);
            ఒకవేళ (!varp || !varp->isIO()) కొనసాగు;
            స్థిరరాశి AstVar* స్థిరరాశి memberp
                = VN_CAST(m_memberMap.కనుగొను_Member(m_covergroupp, varp->name()), Var);
            UASSERT_OBJ(memberp && memberp->isClassMember(), varp,
                        "Covergroup sample argument missing persistent member");
            sampleMembers.చొప్పించు(memberp);
        }
        Covergroup_సమాసం_Valid_Visitor{sampleMembers, constructorRefMembers}.పరిశీలన(m_constructorp);
    }

    శూన్య rebindFormalRefs() {
        std::map<స్థిరరాశి AstVar*, AstVar*> replacements;
        కొరకు (Ast_కణుపు* stmtp = m_constructorp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            ఒకవేళ (స్థిరరాశి AstVar* స్థిరరాశి varp = VN_CAST(stmtp, Var)) {
                ఒకవేళ (!varp->isIO()) కొనసాగు;
                AstVar* స్థిరరాశి memberp
                    = VN_CAST(m_memberMap.కనుగొను_Member(m_covergroupp, varp->name()), Var);
                UASSERT_OBJ(memberp && memberp->isClassMember(), varp,
                            "Covergroup constructor argument missing persistent member");
                replacements.emplace(varp, memberp);
            }
        }
        కొలత_t expectedBindings = 0;
        కొరకు (స్థిరరాశి స్వయంచాలక& pair : replacements) {
            ఒకవేళ (pair.మొదటి->isRef() || pair.మొదటి->isConstRef()) ++expectedBindings;
        }
        కొలత_t rewrittenBindings = 0;
        కొరకు (Ast_కణుపు* stmtp = m_constructorp->stmtsp(); stmtp;) {
            Ast_కణుపు* స్థిరరాశి nextp = stmtp->nextp();
            ఒకవేళ (Ast_కేటాయించు* స్థిరరాశి assignp = VN_CAST(stmtp, కేటాయించు)) {
                AstVarRef* స్థిరరాశి lhsp = VN_CAST(assignp->lhsp(), VarRef);
                AstVarRef* స్థిరరాశి rhsp = VN_CAST(assignp->rhsp(), VarRef);
                ఒకవేళ (lhsp && rhsp
                    && (lhsp->varp()->declDirection() == VDirection::REF
                        || lhsp->varp()->declDirection() == VDirection::CONSTREF)) {
                    స్థిరరాశి స్వయంచాలక it = replacements.కనుగొను(rhsp->varp());
                    UASSERT_OBJ(it != replacements.చివర() && it->రెండవ == lhsp->varp(), assignp,
                                "Unexpected covergroup reference binding assignment");
                    AstCExpr* స్థిరరాశి bindp = కొత్త AstCExpr{assignp->fileline(), ""};
                    bindp->కూడు(lhsp->unlink_Fr_వెనుక());
                    bindp->కూడు(" = &");
                    bindp->కూడు(rhsp->unlink_Fr_వెనుక());
                    assignp->replaceWith(bindp->makeStmt());
                    VL_DO_DANGLING(నెట్టు_Deletep(assignp), assignp);
                    ++rewrittenBindings;
                }
            }
            stmtp = nextp;
        }
        UASSERT_OBJ(rewrittenBindings == expectedBindings, m_constructorp,
                    "Covergroup reference argument missing binding");
        FormalRefVisitor visitor{replacements};
        కొరకు (AstCoverpoint* స్థిరరాశి cpp : m_coverpoints) visitor.పరిశీలన(cpp);
        కొరకు (AstCoverCross* స్థిరరాశి crossp : m_coverCrosses) visitor.పరిశీలన(crossp);
    }

    AstVarRef* install_Enclosing_వెనుక_సూచిక(స్థిరరాశి std::సదిశ<Ast_కణుపు_కేటాయించు*>& constructps) {
        // Simple-case support for embedded covergroups (IEEE 1800-2023 19.4) whose
        // coverpoints reference members of the enclosing class ("Class members can be used
        // in coverpoint expressions").  The covergroup is lowered into a sibling class with
        // no implicit handle to the enclosing object, so such references would emit
        // uncompilable C++.  Add an explicit back-pointer member to the enclosing instance,
        // route member references through it, and pass it into the constructor so
        // coverage initialization can read enclosing members. Returns an invalid
        // reference if an outer class member cannot be reached; otherwise returns an empty result.
        ఒకవేళ (!m_enclosingClassp) తిరిగి శూన్యసూచిక;  // Offending refs require an enclosing class

        AstVarRef* invalidp = శూన్యసూచిక;
        Ast_కణుపు* offenderp = శూన్యసూచిక;
        std::సమితి<స్థిరరాశి AstVar*> ownVars;
        కొరకు (Ast_కణుపు* itemp = m_covergroupp->membersp(); itemp; itemp = itemp->nextp()) {
            ఒకవేళ (స్థిరరాశి AstVar* స్థిరరాశి varp = VN_CAST(itemp, Var)) ownVars.చొప్పించు(varp);
        }
        స్థిరరాశి std::సమితి<స్థిరరాశి AstVar*> enclosingVars = enclosingInstanceVars();
        std::సదిశ<AstVarRef*> refs_గమ్యం_Rewrite;
        std::సదిశ<AstThisRef*> this_Refs_గమ్యం_Rewrite;
        std::సదిశ<AstFuncRef*> func_Refs_గమ్యం_Rewrite;
        స్థిరరాశి స్వయంచాలక పరిశీలన = [&](Ast_కణుపు* rootp) {
            rootp->foreach([&](AstVarRef* refp) {
                ఒకవేళ (invalidp) తిరిగి;
                స్థిరరాశి AstVar* స్థిరరాశి varp = refp->varp();
                ఒకవేళ (!isEnclosingInstanceVar(varp) || ownVars.లెక్క(varp)) తిరిగి;
                ఒకవేళ (!enclosingVars.లెక్క(varp)) {
                    invalidp = refp;
                    తిరిగి;
                }
                refs_గమ్యం_Rewrite.నెట్టు_వెనుక(refp);
                ఒకవేళ (!offenderp) offenderp = refp;
            });
            ఒకవేళ (invalidp) తిరిగి;
            rootp->foreach([&](AstThisRef* refp) {
                స్థిరరాశి AstClassRefDType* స్థిరరాశి refDTypep
                    = VN_CAST(refp->dtypep()->skipRefp(), ClassRefDType);
                ఒకవేళ (refDTypep && refDTypep->classp() == m_covergroupp) {
                    this_Refs_గమ్యం_Rewrite.నెట్టు_వెనుక(refp);
                    ఒకవేళ (!offenderp) offenderp = refp;
                }
            });
            rootp->foreach([&](AstFuncRef* refp) {
                ఒకవేళ (!isEnclosingInstanceFunc(refp->taskp())) తిరిగి;
                func_Refs_గమ్యం_Rewrite.నెట్టు_వెనుక(refp);
                ఒకవేళ (!offenderp) offenderp = refp;
            });
        };
        కొరకు (AstCoverpoint* స్థిరరాశి cpp : m_coverpoints) పరిశీలన(cpp);
        కొరకు (AstCoverCross* స్థిరరాశి crossp : m_coverCrosses) పరిశీలన(crossp);
        కొరకు (Ast_Cg_Option_కేటాయించు* స్థిరరాశి optp : m_cgOptions) పరిశీలన(optp);
        ఒకవేళ (invalidp || !offenderp) తిరిగి invalidp;

        UASSERT_OBJ(m_embeddedVarp, m_covergroupp, "Embedded covergroup variable not found");
        // Commit: add the back-pointer member, rewrite the references, initialize the handle.
        File_పంక్తి* స్థిరరాశి fl = m_covergroupp->fileline();
        AstClassRefDType* స్థిరరాశి enclDTypep = కొత్త AstClassRefDType{fl, m_enclosingClassp, శూన్యసూచిక};
        enclDTypep->raw_సూచిక(నిజం);
        v3Global.rootp()->typeTablep()->కూడు_Typesp(enclDTypep);
        AstVar* స్థిరరాశి handleVarp
            = కొత్త AstVar{fl, VVarType::MEMBER, "__Vcg_enclosingp", enclDTypep};
        m_covergroupp->కూడు_Membersp(handleVarp);
        AstVar* స్థిరరాశి argumentp = కొత్త AstVar{fl, VVarType::BLOCKTEMP, "__Vcg_parentp", enclDTypep};
        argumentp->direction(VDirection::ఆదానం);
        argumentp->declDirection(VDirection::ఆదానం);
        argumentp->funcLocal(నిజం);
        argumentp->noReset(నిజం);
        argumentp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
        m_constructorp->కూడు_Stmtsp(argumentp);
        m_constructorp->prependStmtsp(కొత్త Ast_కేటాయించు{fl, memberRef(fl, handleVarp, VAccess::WRITE),
                                                    కొత్త AstVarRef{fl, argumentp, VAccess::READ}});

        // Route each enclosing-member reference through the back-pointer: 'm' -> 'h.m'.
        కొరకు (AstVarRef* స్థిరరాశి refp : refs_గమ్యం_Rewrite) { rewriteVarRef(refp, handleVarp); }
        కొరకు (AstThisRef* స్థిరరాశి refp : this_Refs_గమ్యం_Rewrite) { rewriteThisRef(refp, handleVarp); }
        కొరకు (AstFuncRef* స్థిరరాశి refp : func_Refs_గమ్యం_Rewrite) { rewriteFuncRef(refp, handleVarp); }

        // Append a named hidden argument to preserve positional and defaulted user arguments.
        కొరకు (Ast_కణుపు_కేటాయించు* స్థిరరాశి constructp : constructps) {
            File_పంక్తి* స్థిరరాశి cfl = constructp->fileline();
            AstCExpr* స్థిరరాశి thisp = కొత్త AstCExpr{cfl, "this"};
            thisp->dtypep(enclDTypep);
            VN_AS(constructp->rhsp(), New)->కూడు_Argsp(కొత్త AstArg{cfl, argumentp->name(), thisp});
        }
        తిరిగి శూన్యసూచిక;
    }

    శూన్య సందర్శించు(AstClass* nodep) override {
        UINFO(9, "Visiting class: " << nodep->name() << " isCovergroup=" << nodep->isCovergroup());
        ఒకవేళ (nodep->isCovergroup()) {
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
            m_embeddedVarp = కనుగొను_Embedded_Covergroup_Var();
            m_covergroupName = covergroupTypeName();
            m_sampleFuncp = శూన్యసూచిక;
            m_constructorp = శూన్యసూచిక;
            std::సదిశ<EmbeddedEventTrigger> embeddedEventTriggers;
            Ast_Sen_వృక్షం* embeddedEventForkp = శూన్యసూచిక;

            // Extract and store the clocking event from AstCovergroup node
            // The parser creates this node to preserve the event information
            తార్కిక hasUnsupportedEvent = అబద్ధం;
            కొరకు (Ast_కణుపు* itemp = nodep->membersp(); itemp;) {
                Ast_కణుపు* స్థిరరాశి nextp = itemp->nextp();
                ఒకవేళ (AstCovergroup* స్థిరరాశి cgp = VN_CAST(itemp, Covergroup)) {
                    // Store the event in the global map for V3Active to retrieve later
                    // V3LinkParse only creates this sentinel AstCovergroup node when a clocking
                    // event exists, so cgp->eventp() is always non-null here.
                    UASSERT_OBJ(cgp->eventp(), cgp,
                                "Sentinel AstCovergroup in class must have non-null eventp");
                    ఒకవేళ (hasEnclosingEventRef(cgp)) {
                        UASSERT_OBJ(m_embeddedVarp, cgp,
                                    "Embedded covergroup event has no instance variable");
                        ఒకవేళ (v3Global.opt.timing().is_సమితి_True()) {
                            embeddedEventForkp = cgp->eventp()->unlink_Fr_వెనుక();
                        } లేకపోతే {
                            embeddedEventTriggers = collectEmbeddedEventTriggers(cgp);
                            ఒకవేళ (embeddedEventTriggers.ఖాళీలు()) {
                                cgp->v3warn(COVERIGN,
                                            "Unsupported: 'covergroup' clocking event on complex "
                                            "member expression; use --timing for full support.");
                                hasUnsupportedEvent = నిజం;
                            }
                        }
                        VL_DO_DANGLING(నెట్టు_Deletep(cgp->unlink_Fr_వెనుక()), cgp);
                        itemp = nextp;
                        కొనసాగు;
                    }
                    // V3Active handles events that do not depend on an enclosing instance.
                    UINFO(4, "Keeping covergroup event node for V3Active: " << nodep->name());
                    itemp = nextp;
                    కొనసాగు;
                }
                itemp = nextp;
            }

            // Find the sample() method and constructor
            m_sampleFuncp = VN_CAST(m_memberMap.కనుగొను_Member(nodep, "sample"), Func);
            // V3LinkParse always synthesizes a sample() method for every covergroup, and the
            // sampling-code generation below dereferences m_sampleFuncp unconditionally.
            UASSERT_OBJ(m_sampleFuncp, nodep, "Covergroup missing synthesized sample() method");
            m_sampleFuncp->isCovergroupSample(నిజం);
            m_constructorp = VN_CAST(m_memberMap.కనుగొను_Member(nodep, "new"), Func);
            UINFO(9, "Found sample() method: " << (m_sampleFuncp ? "yes" : "no"));
            UINFO(9, "Found constructor: " << (m_constructorp ? "yes" : "no"));

            // If covergroup has unsupported clocking event, skip processing it
            // but still clean up coverpoints so they don't reach downstream passes
            ఒకవేళ (hasUnsupportedEvent) {
                iterateChildren(nodep);
                validateCovergroupExpressions();
                rebindFormalRefs();
                delete_Coverage_వస్తువులు();
                ఒకవేళ (embeddedEventForkp) {
                    VL_DO_DANGLING(నెట్టు_Deletep(embeddedEventForkp), embeddedEventForkp);
                }
                తిరిగి;
            }

            iterateChildren(nodep);
            validateCovergroupExpressions();
            rebindFormalRefs();
            స్థిరరాశి std::సదిశ<Ast_కణుపు_కేటాయించు*> constructps = కనుగొను_Covergroup_Constructions();

            // Embedded covergroups (IEEE 1800-2023 19.4): coverpoints, iff expressions, and
            // crosses may reference members of the enclosing class. The covergroup is lowered
            // into a sibling class with no implicit handle to the enclosing instance. Install
            // an explicit back-pointer and route the references through it.
            ఒకవేళ (AstVarRef* స్థిరరాశి invalidp = install_Enclosing_వెనుక_సూచిక(constructps)) {
                invalidp->v3error("Non-static member "
                                  << invalidp->varp()->prettyNameQ()
                                  << " of an outer class requires an explicit "
                                     "object handle (IEEE 1800-2023 8.23).");
                delete_Coverage_వస్తువులు();
                ఒకవేళ (embeddedEventForkp) {
                    VL_DO_DANGLING(నెట్టు_Deletep(embeddedEventForkp), embeddedEventForkp);
                }
                తిరిగి;
            }
            installEmbeddedEventTriggers(embeddedEventTriggers, constructps);
            ఒకవేళ (embeddedEventForkp) installEmbeddedEventFork(embeddedEventForkp, constructps);
            processCovergroup();
            // Remove lowered coverpoints/crosses from the class - they have been
            // fully translated into C++ code and must not reach downstream passes
            delete_Coverage_వస్తువులు();
        } లేకపోతే {
            // Track the lexically enclosing class so a nested covergroup can resolve
            // references to the enclosing object's members (installEnclosingBackPointer).
            VL_RESTORER(m_enclosingClassp);
            m_enclosingClassp = nodep;
            iterateChildren(nodep);
        }
    }

    శూన్య సందర్శించు(AstCoverpoint* nodep) override {
        UINFO(9, "Found coverpoint: " << nodep->name());
        m_coverpoints.నెట్టు_వెనుక(nodep);
        m_coverpointMap.emplace(nodep->name(), nodep);
        iterateChildren(nodep);
    }

    శూన్య సందర్శించు(AstCoverCross* nodep) override {
        UINFO(9, "Found cross: " << nodep->name());
        m_coverCrosses.నెట్టు_వెనుక(nodep);
        iterateChildren(nodep);
    }

    // V3Width leaves only the covergroup-level options lowerCovergroupOptions() takes
    శూన్య సందర్శించు(Ast_Cg_Option_కేటాయించు* nodep) override { m_cgOptions.నెట్టు_వెనుక(nodep); }

    // A package, interface, or module, so the design unit declaring the classes within
    శూన్య సందర్శించు(Ast_కణుపు_Module* nodep) override {
        VL_RESTORER(m_unitp);
        m_unitp = nodep;
        iterateChildren(nodep);
    }

    శూన్య సందర్శించు(Ast_కణుపు* nodep) override { iterateChildren(nodep); }

బహిరంగ:
    // CONSTRUCTORS
    FunctionalCoverageVisitor(AstNetlist* nodep,
                              స్థిరరాశి std::సమితి<స్థిరరాశి AstVar*>& mergeableTypeOptions)
        : m_mergeableTypeOptions{mergeableTypeOptions} {
        iterate(nodep);
    }
    ~FunctionalCoverageVisitor() override = అప్రమేయ;
};

//######################################################################
// Functional coverage class functions

శూన్య V3Covergroup::covergroup(AstNetlist* nodep) {
    UINFO(4, __FUNCTION__ << ": ");
    స్థిరరాశి Covergroup_కేటాయించు_Valid_Visitor validVisitor{nodep};
    ఒకవేళ (!validVisitor.valid()) V3Error::abortIfErrors();
    {  // Destruct before checking
        FunctionalCoverageVisitor{nodep, validVisitor.mergeableTypeOptions()};
    }
    V3Global::dump_Check_Global_వృక్షం("coveragefunc", 0, dump_వృక్షం_Either_Level() >= 3);
}
