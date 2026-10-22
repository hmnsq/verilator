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

#अन्तर्भाव "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#अन्तर्भाव "V3Covergroup.h"

#अन्तर्भाव "V3Const.h"
#अन्तर्भाव "V3Error.h"
#अन्तर्भाव "V3File.h"
#अन्तर्भाव "V3MemberMap.h"
#अन्तर्भाव "V3UniqueNames.h"

#अन्तर्भाव <bitset>
#अन्तर्भाव <cmath>
#अन्तर्भाव <deque>
#अन्तर्भाव <set>
#अन्तर्भाव <tuple>
#अन्तर्भाव <unordered_map>
#अन्तर्भाव <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Embedded covergroup assignment validation; and in the same pass over the netlist, the
// type_option variables through which SystemVerilog may set type_option.merge_instances

वग्ग CovergroupAssignValidVisitor final : सार्वजनिक VNVisitorConst {
    VMemberMap m_memberMap;
    std::map<ध्रुव AstVar*, ध्रुव Ast_ग्रन्थिः_FTask*>
        m_constructors;  // Implicit instance -> constructor
    std::set<ध्रुव AstVar*> m_mergeableTypeOptions;  // See mergeableTypeOptions()
    ध्रुव Ast_ग्रन्थिः_FTask* m_ftaskp = nullptr;
    तार्किक m_collecting = सच्च;
    तार्किक m_valid = सच्च;

    शून्य visit(AstClass* nodep) override {
        VL_RESTORER(m_ftaskp);
        m_ftaskp = nullptr;
        सचे (m_collecting) {
            ध्रुव Ast_ग्रन्थिः_FTask* ध्रुव constructorp
                = VN_CAST(m_memberMap.findMember(nodep, "new"), ग्रन्थिः_FTask);
            अत्थाय (ध्रुव Ast_ग्रन्थिः* itemp = nodep->membersp(); itemp; itemp = itemp->nextp()) {
                ध्रुव AstVar* ध्रुव varp = VN_CAST(itemp, Var);
                // Only the implicit instance is restricted, not explicitly typed aliases.
                सचे (!varp || !varp->isClassMember() || varp->isDeclTyped()) अनुवर्त;
                ध्रुव AstClassRefDType* ध्रुव refp
                    = VN_CAST(varp->dtypep()->skipRefp(), ClassRefDType);
                सचे (refp && refp->classp()->covergroupEnclosingClassp() == nodep) {
                    m_constructors.emplace(varp, constructorp);
                }
            }
        }
        iterateChildrenConst(nodep);
    }
    शून्य visit(Ast_ग्रन्थिः_FTask* nodep) override {
        VL_RESTORER(m_ftaskp);
        m_ftaskp = nodep;
        iterateChildrenConst(nodep);
    }
    शून्य visit(Ast_ग्रन्थिः_Assign* nodep) override {
        सचे (!m_collecting) {
            ध्रुव AstVar* varp = nullptr;
            सचे (ध्रुव Ast_ग्रन्थिः_Var_Ref* ध्रुव refp = VN_CAST(nodep->lhsp(), ग्रन्थिः_Var_Ref)) {
                varp = refp->varp();
            } अञ्ञथा सचे (ध्रुव AstMemberSel* ध्रुव selp = VN_CAST(nodep->lhsp(), MemberSel)) {
                varp = selp->varp();
            }
            ध्रुव auto it = m_constructors.find(varp);
            सचे (it != m_constructors.end() && (!m_ftaskp || m_ftaskp != it->second)) {
                m_valid = मिच्छा;
                nodep->v3error("Embedded covergroup variable "
                               << varp->prettyNameQ()
                               << " may only be assigned in the enclosing class's 'new' method "
                                  "(IEEE 1800-2023 19.4).");
            }
        }
        iterateChildrenConst(nodep);
    }
    शून्य visit(Ast_ग्रन्थिः_Var_Ref* nodep) override {
        // Type options may be set at any time (IEEE 1800-2023 19.7.1), so SystemVerilog may set
        // type_option.merge_instances with any use of a type_option but a read of another
        // member, as not every write is marked yet (std::randomize)
        सचे (m_collecting && nodep->varp()->name() == "type_option") {
            ध्रुव AstStructSel* ध्रुव selp = VN_CAST(nodep->backp(), StructSel);
            सचे (!selp || selp->fromp() != nodep || selp->name() == "merge_instances"
                || !nodep->access().isReadOnly()) {
                m_mergeableTypeOptions.निवेशय(nodep->varp());
            }
        }
        iterateChildrenConst(nodep);
    }
    शून्य visit(Ast_ग्रन्थिः* nodep) override { iterateChildrenConst(nodep); }

सार्वजनिक:
    explicit CovergroupAssignValidVisitor(AstNetlist* nodep) {
        // Uses can precede their enclosing class in the tree.
        iterateConst(nodep);
        m_collecting = मिच्छा;
        सचे (!m_constructors.empty()) iterateConst(nodep);
    }
    तार्किक valid() ध्रुव { पटिनिवत्त m_valid; }
    // type_option variables through which SystemVerilog may set type_option.merge_instances, so
    // that covergroups are tested with optionVar(true)
    ध्रुव std::set<ध्रुव AstVar*>& mergeableTypeOptions() ध्रुव { पटिनिवत्त m_mergeableTypeOptions; }
};

//######################################################################
// Covergroup expression validation visitor

वग्ग Covergroup_व्यञ्जकम्_Valid_Visitor final : सार्वजनिक VNVisitor {
    ध्रुव std::set<ध्रुव AstVar*>& m_sampleMembers;
    ध्रुव std::set<ध्रुव AstVar*>& m_constructorRefMembers;
    तार्किक m_inCoverageExpression = मिच्छा;
    तार्किक m_sampleFormalAllowed = मिच्छा;

    शून्य scanSampleExpression(Ast_ग्रन्थिः* nodep) {
        सचे (!nodep) पटिनिवत्त;
        VL_RESTORER(m_sampleFormalAllowed);
        m_sampleFormalAllowed = सच्च;
        iterateAndNextNull(nodep);
    }

    शून्य scanCoverageExpression(Ast_ग्रन्थिः* nodep) {
        सचे (!nodep) पटिनिवत्त;
        VL_RESTORER(m_inCoverageExpression);
        m_inCoverageExpression = सच्च;
        iterateAndNextNull(nodep);
    }

    शून्य visit(AstCoverpoint* nodep) override {
        scanSampleExpression(nodep->exprp());
        scanSampleExpression(nodep->iffp());
        iterateAndNextNull(nodep->binsp());
        iterateAndNextNull(nodep->optionsp());
    }
    शून्य visit(AstCoverCross* nodep) override {
        iterateAndNextNull(nodep->itemsp());
        scanSampleExpression(nodep->iffp());
        iterateAndNextNull(nodep->optionsp());
        iterateAndNextNull(nodep->binsp());
    }
    शून्य visit(AstCoverCrossBin* nodep) override {
        iterateAndNextNull(nodep->selectp());
        scanSampleExpression(nodep->iffp());
    }
    शून्य visit(AstCoverCrossWith* nodep) override {
        iterateAndNextNull(nodep->selectp());
        scanCoverageExpression(nodep->filterp());
        scanCoverageExpression(nodep->countp());
    }
    शून्य visit(AstCoverBinsof* nodep) override { scanCoverageExpression(nodep->rangesp()); }
    शून्य visit(AstCoverBin* nodep) override {
        scanCoverageExpression(nodep->rangesp());
        scanSampleExpression(nodep->iffp());
        scanCoverageExpression(nodep->arraySizep());
        scanCoverageExpression(nodep->transp());
    }
    शून्य visit(AstVarRef* nodep) override {
        सचे (!m_sampleFormalAllowed && m_sampleMembers.गणनम्(nodep->varp())) {
            nodep->v3error("Covergroup sample formal argument "
                           << nodep->varp()->prettyNameQ()
                           << " may only be used in a coverpoint or conditional guard "
                              "expression (IEEE 1800-2023 19.8.1).");
        }
        सचे (m_inCoverageExpression && m_constructorRefMembers.गणनम्(nodep->varp())) {
            nodep->v3error("Ref covergroup constructor formal argument "
                           << nodep->varp()->prettyNameQ()
                           << " may not be used in a covergroup expression "
                              "(IEEE 1800-2023 19.5).");
        }
    }
    शून्य visit(Ast_ग्रन्थिः_FTask_Ref* nodep) override {
        सचे (m_inCoverageExpression && nodep->taskp()) {
            तार्किक invalidDirection = मिच्छा;
            अत्थाय (Ast_ग्रन्थिः* stmtp = nodep->taskp()->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
                ध्रुव AstVar* ध्रुव varp = VN_CAST(stmtp, Var);
                सचे (varp && varp->isIO() && varp->isWritable()) {
                    invalidDirection = सच्च;
                    भङ्ग;
                }
            }
            सचे (invalidDirection) {
                nodep->v3error("Function " << nodep->taskp()->prettyNameQ()
                                           << " called in a covergroup expression has an "
                                              "output, inout, or non-const ref argument "
                                              "(IEEE 1800-2023 19.5).");
            }
        }
        iterateChildren(nodep);
    }
    शून्य visit(Ast_ग्रन्थिः* nodep) override { iterateChildren(nodep); }

सार्वजनिक:
    Covergroup_व्यञ्जकम्_Valid_Visitor(ध्रुव std::set<ध्रुव AstVar*>& sampleMembers,
                               ध्रुव std::set<ध्रुव AstVar*>& constructorRefMembers)
        : m_sampleMembers{sampleMembers}
        , m_constructorRefMembers{constructorRefMembers} {}
    शून्य scan(Ast_ग्रन्थिः* nodep) { iterate(nodep); }
};

//######################################################################
// Bins of one declaration whose values are computed rather than listed: bin k covers
// [m_lo + k * m_stride, m_lo + (k + 1) * m_stride - 1], and the last bin extends to m_hi.  An
// array bin element is a run of single-value bins; automatic bins partition the coverpoint
// domain.  Bounds are coverpoint values at FunctionalCoverageVisitor::runWidth(),
// sign-extended like a CrossValueRange's.

वग्ग BinRun final {
सार्वजनिक:
    // MEMBERS
    uint32_t m_गणनम्;  // Number of bins
    V3_सङ्ख्या m_lo;  // Lowest value of the first bin
    V3_सङ्ख्या m_stride;  // Number of values of each bin but the last
    V3_सङ्ख्या m_hi;  // Highest value of the last bin
    तार्किक m_empty = मिच्छा;  // A single bin without a value of the coverpoint type
    uint32_t m_declared = 0;  // Runtime index of the first bin, once generated

    // CONSTRUCTORS
    BinRun(Ast_ग्रन्थिः* nodep, पूर्णाङ्क width, uint32_t गणनम्)
        : m_गणनम्{गणनम्}
        , m_lo{nodep, width}
        , m_stride{nodep, width, 1}
        , m_hi{nodep, width} {}
};

//######################################################################
// Functional coverage visitor

वग्ग FunctionalCoverageVisitor final : सार्वजनिक VNVisitor {
    // NODE STATE
    // Entire netlist:
    //  AstCoverpoint::user1p()  -> AstVar*.  Previous-value variable for transition bins
    //  AstCoverpoint::user2()   -> bool.  Had a bins declaration ignored, so no automatic bins
    ध्रुव VNUser1InUse m_inuser1;
    ध्रुव VNUser2InUse m_inuser2;

    // STATE
    std::set<AstCoverpoint*>
        m_runtimePoints;  // Points needing value metadata and live-bin mapping
    std::set<AstCoverCross*> m_runtimeCrosses;  // Crosses over finalized live-bin dimensions
    std::map<AstVar*, AstVar*> m_excludedVars;  // Sample-time state-exclusion flags
    AstClass* m_covergroupp = nullptr;  // Current covergroup being processed
    Ast_ग्रन्थिः_Module* m_unitp = nullptr;  // Design unit declaring the current class, outside classes
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
    तार्किक m_cgMergeInstances = मिच्छा;  // The covergroup's type_option.merge_instances, a constant
    // The covergroup's type_option.merge_instances is, or SystemVerilog may make it, true
    तार्किक m_cgMayMerge = मिच्छा;
    // type_option members through which SystemVerilog may set type_option.merge_instances,
    // see CovergroupAssignValidVisitor::mergeableTypeOptions()
    ध्रुव std::set<ध्रुव AstVar*>& m_mergeableTypeOptions;
    पूर्णाङ्क m_cgAtLeast = 1;  // Covergroup option.at_least

    संरचना EmbeddedEventTrigger final {
        File_पङ्क्तिः* eventFl;  // Clocking-event source location
        AstVar* baseVarp;  // Base enclosing-class member in the event expression
        AstVar* memberVarp;  // Selected member in a 'base.member' expression, or nullptr
        VEdgeType edgeType;  // Clocking-event edge qualifier
        AstVar* prevVarp;  // Member containing the previous event value, or nullptr
        EmbeddedEventTrigger(File_पङ्क्तिः* eventFl, AstVar* baseVarp, AstVar* memberVarp,
                             VEdgeType edgeType)
            : eventFl{eventFl}
            , baseVarp{baseVarp}
            , memberVarp{memberVarp}
            , edgeType{edgeType}
            , prevVarp{nullptr} {}
    };

    std::set<std::string> m_crossedCpNames;  // Coverpoints referenced by a cross
    std::map<std::string, AstVar*> m_cpVarMap;  // Coverpoint name -> its VlCoverpoint member
    संरचना BinRuns final {
        std::vector<BinRun> runs;  // Runs of an array or automatic bins declaration, in order
        uint32_t गणनम् = 0;  // Bins across all runs
        तार्किक unsupported = मिच्छा;  // Too many bins, or invalid: the declaration is ignored
        // The value of each bin, which names it, of a wildcard array; else the bins are indexed
        std::vector<std::string> मानानि;
        // Too many values of an ignore or illegal wildcard array, which is then one bin
        तार्किक single = मिच्छा;
    };
    संरचना Cross_Bin_मानानि final {
        AstCoverBin* binp;  // Declaration owning this Normal bin
        Ast_ग्रन्थिः_व्यञ्जकम्* valuep;  // Individual array-bin value, or nullptr for a scalar bin
        ध्रुव BinRun* runp = nullptr;  // Run computing the bin's values, if any
        uint32_t element = 0;  // Index of the bin within runp
    };
    संरचना BinSpan final {
        uint32_t first = 0;  // First Normal index of the bin declaration
        uint32_t गणनम् = 0;  // Number of Normal bins of the declaration
        uint32_t declared = 0;  // First runtime bin index, across all bin kinds
        int32_t sized = -1;  // Index of the sized array, placed at construction; or -1
    };
    संरचना CoverpointBins final {
        uint32_t total = 0;  // Number of Normal bins
        Ast_ग्रन्थिः_व्यञ्जकम्* exprp = nullptr;  // Sampled expression, for the value domain
        तार्किक crossed = मिच्छा;  // Feeds a cross, which needs 'values'
        std::vector<Cross_Bin_मानानि> मानानि;  // Values in runtime Normal-bin index order
        std::unordered_map<std::string, BinSpan> spans;  // Declared bin name -> index span
        BinSpan implicitAuto{0, 0, 0};  // Implicit automatic bins, each named 'auto_<i>'
        std::deque<BinRun> runs;  // Runs 'values' refers to
    };
    std::map<AstVar*, CoverpointBins> m_cpBins;  // Runtime coverpoint -> binsof index ranges
    // Names of the bins declarations each coverpoint ignored, which binsof selects as no bins
    std::map<ध्रुव AstCoverpoint*, std::vector<std::string>> m_droppedBins;
    // Prefixes of the constructor temporaries of constructed bins, as coverpoint and bin names
    // alone may repeat: coverpoint 'a_' bins 'b', and coverpoint 'a' bins '_b'
    V3UniqueNames m_sizedNames{"__Vsized"};
    std::vector<Ast_ग्रन्थिः_व्यञ्जकम्*> m_detached_मानानि;  // Array-bin values m_cpBins refers to
    std::set<AstCoverCross*>
        m_droppedCrosses;  // Crosses with a bare-variable item: drop (COVERIGN)
    std::map<uint32_t, AstCoverpointDType*> m_cpDTypes;  // Hit-list bound -> interned dtype
    using CrossShape = std::tuple<uint32_t, uint32_t, uint32_t, uint32_t, uint64_t, तार्किक>;
    std::map<CrossShape, AstCoverCrossDType*> m_cxDTypes;
    AstVar* m_cgInstVarp = nullptr;  // __Vcg_inst handle member of the current covergroup

    VMemberMap m_memberMap;  // Member names cached for fast lookup

    // METHODS
    // The covergroup's 'option' or static 'type_option' member (V3LinkParse creates both)
    AstVar* optionVar(तार्किक typeOption) {
        AstVar* ध्रुव varp = VN_AS(
            m_memberMap.findMember(m_covergroupp, typeOption ? "type_option" : "option"), Var);
        UASSERT_OBJ(varp, m_covergroupp, "Covergroup missing option member");
        पटिनिवत्त varp;
    }

    // 'option.<member>' or 'type_option.<member>', per optionVarp
    AstStructSel* newOptionSel(File_पङ्क्तिः* fl, AstVar* optionVarp, ध्रुव std::string& member,
                               VAccess access) {
        ध्रुव AstMemberDType* ध्रुव memberp
            = VN_AS(m_memberMap.findMember(optionVarp->dtypep()->skipRefp(), member), MemberDType);
        UASSERT_OBJ(memberp, optionVarp, "Coverage option structure missing '" << member << "'");
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव fromp = optionVarp->lifetime().isStatic()
                                       ? नूतन AstVarRef{fl, optionVarp, access}
                                       : memberRef(fl, optionVarp, access);
        AstStructSel* ध्रुव selp = नूतन AstStructSel{fl, fromp, member};
        selp->dtypep(memberp->subDTypep()->skipRefToEnump());
        selp->didWidth(सच्च);
        पटिनिवत्त selp;
    }

    // A constant option.at_least; a negative count of hits has no meaning, so takes the default
    स्थिर पूर्णाङ्क constAtLeast(ध्रुव AstConst* constp) {
        ध्रुव पूर्णाङ्क मानम् = constp->toSInt();
        सचे (मानम् >= 0) पटिनिवत्त मानम्;
        constp->v3warn(COVERIGN, "Ignoring negative 'option.at_least'; using 1");
        पटिनिवत्त 1;
    }

    // An option's constant value, or nullptr after a warning, as non-constant is unsupported
    स्थिर ध्रुव AstConst* constOption(ध्रुव Ast_ग्रन्थिः* valuep, VCoverOptionType optType) {
        सचे (ध्रुव AstConst* ध्रुव constp = VN_CAST(valuep, Const)) पटिनिवत्त constp;
        valuep->v3warn(COVERIGN, "Ignoring unsupported: non-constant 'option."
                                     << optType.ascii() << "'; using default value");
        पटिनिवत्त nullptr;
    }

    // Store the covergroup-level options (IEEE 1800-2023 19.7) where SystemVerilog and the
    // runtime read them.  The instance options, option.weight and option.get_inst_coverage,
    // are evaluated by the constructor, as are the other instance options; the type options,
    // type_option.weight and type_option.merge_instances, are constant, and initialize the
    // static member.  Without its own type_option.merge_instances, a covergroup has the
    // default of --coverage-merge-instances.  A constant option.at_least is the default of
    // the coverpoints and crosses.
    शून्य lowerCovergroupOptions() {
        तार्किक mergeSet = मिच्छा;  // The covergroup sets type_option.merge_instances
        m_cgAtLeast = 1;
        अत्थाय (AstCgOptionAssign* ध्रुव optp : m_cgOptions) {
            सचे (optp->optType() == VCoverOptionType::AT_LEAST) {
                सचे (ध्रुव AstConst* ध्रुव constp = constOption(optp->valuep(), optp->optType())) {
                    m_cgAtLeast = constAtLeast(constp);
                }
                VL_DO_DANGLING(प्रक्षिप_Deletep(optp->unlinkFrBack()), optp);
                अनुवर्त;
            }
            File_पङ्क्तिः* ध्रुव fl = optp->fileline();
            // V3Width left the type options constant, and type_option.weight non-negative
            ध्रुव AstConst* ध्रुव constp = VN_CAST(optp->valuep(), Const);
            std::string member = "weight";
            सचे (optp->optType() == VCoverOptionType::MERGE_INSTANCES) {
                member = "merge_instances";
                m_cgMergeInstances = !constp->अङ्कः().isEqZero();
                mergeSet = सच्च;
            } अञ्ञथा सचे (optp->optType() == VCoverOptionType::GET_INST_COVERAGE) {
                member = "get_inst_coverage";
            } अञ्ञथा {
                UASSERT_OBJ(optp->optType() == VCoverOptionType::WEIGHT, optp,
                            "Unexpected covergroup option reaching V3Covergroup");
                सचे (optp->typeOption()) m_cgTypeWeight = constp->toUInt();
            }
            AstAssign* ध्रुव assignp = नूतन AstAssign{
                fl, newOptionSel(fl, optionVar(optp->typeOption()), member, VAccess::WRITE),
                optp->valuep()->unlinkFrBack()};
            सचे (optp->typeOption()) {
                m_covergroupp->addMembersp(नूतन AstInitialStatic{fl, assignp});
                VL_DO_DANGLING(प्रक्षिप_Deletep(optp->unlinkFrBack()), optp);
            } अञ्ञथा {
                optp->replaceWith(assignp);
                VL_DO_DANGLING(प्रक्षिप_Deletep(optp), optp);
            }
        }
        m_cgOptions.clear();
        सचे (!mergeSet && v3Global.opt.coverageMergeInstances()) {
            File_पङ्क्तिः* ध्रुव fl = m_covergroupp->fileline();
            m_covergroupp->addMembersp(नूतन AstInitialStatic{
                fl, नूतन AstAssign{
                        fl, newOptionSel(fl, optionVar(सच्च), "merge_instances", VAccess::WRITE),
                        नूतन AstConst{fl, AstConst::BitTrue{}}}});
            m_cgMergeInstances = सच्च;
        }
        m_cgMayMerge = m_cgMergeInstances || m_mergeableTypeOptions.गणनम्(optionVar(सच्च));
    }

    // The weight of an item in the coverage database, which merges the instances: its
    // type_option.weight if the covergroup merges them too (IEEE 1800-2023 19.11.3); else its
    // option.weight if a constant, and so of every instance; else its type_option.weight
    uint32_t itemDatabaseWeight(Ast_ग्रन्थिः* optionsp) ध्रुव {
        ध्रुव Ast_ग्रन्थिः_व्यञ्जकम्* weightp = nullptr;  // The option.weight in effect
        uint32_t typeWeight = 1;
        अत्थाय (Ast_ग्रन्थिः* nodep = optionsp; nodep; nodep = nodep->nextp()) {
            ध्रुव AstCoverOption* ध्रुव optp = VN_AS(nodep, CoverOption);
            सचे (!(optp->optType() == VCoverOptionType::WEIGHT)) अनुवर्त;
            // V3Width left type_option.weight a non-negative constant
            सचे (optp->typeOption()) {
                typeWeight = VN_AS(optp->valuep(), Const)->toUInt();
            } अञ्ञथा {
                weightp = optp->valuep();
            }
        }
        सचे (m_cgMergeInstances) पटिनिवत्त typeWeight;
        सचे (!weightp) पटिनिवत्त 1;
        सचे (ध्रुव AstConst* ध्रुव constp = VN_CAST(weightp, Const)) पटिनिवत्त constp->toUInt();
        पटिनिवत्त typeWeight;
    }

    // Configure an item's option.weight, its weight in instance coverage (IEEE 1800-2023
    // 19.11), and, if the covergroup may merge its instances, its type_option.weight, its
    // weight in type coverage then (19.11.3).
    शून्य generateItemWeight(File_पङ्क्तिः* fl, AstVar* itemVarp, Ast_ग्रन्थिः* optionsp) {
        अत्थाय (Ast_ग्रन्थिः* nodep = optionsp; nodep; nodep = nodep->nextp()) {
            ध्रुव AstCoverOption* ध्रुव optp = VN_AS(nodep, CoverOption);
            सचे (!(optp->optType() == VCoverOptionType::WEIGHT)) अनुवर्त;
            सचे (!optp->typeOption()) {
                m_constructorp->addStmtsp(
                    itemCall(fl, itemVarp, VCMethod::COVERGROUP_WEIGHT,
                             {optp->valuep()->clone_वृक्षः(मिच्छा), file_पङ्क्तिः_Debug(optp->fileline())})
                        ->makeStmt());
            } अञ्ञथा सचे (m_cgMayMerge) {
                // V3Width left type_option.weight a non-negative constant
                m_constructorp->addStmtsp(itemCall(fl, itemVarp, VCMethod::COVERGROUP_TYPE_WEIGHT,
                                                   {optp->valuep()->clone_वृक्षः(मिच्छा)})
                                              ->makeStmt());
            }
        }
    }

    शून्य processCovergroup() {
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
        m_cgMergeInstances = मिच्छा;
        m_cgMayMerge = मिच्छा;

        lowerCovergroupOptions();

        // Scan every cross item to record the coverpoints it references (the cross dimensions)
        // and to flag any cross naming a bare variable -- a would-be implicit coverpoint, which
        // Verilator does not synthesize.  An unresolvable item drops only that one cross (with a
        // COVERIGN in generateCrossCode), leaving the rest of the covergroup intact.
        अत्थाय (AstCoverCross* crossp : m_coverCrosses) {
            अत्थाय (Ast_ग्रन्थिः* itemp = crossp->itemsp(); itemp; itemp = itemp->nextp()) {
                ध्रुव AstCoverpointRef* ध्रुव refp = VN_AS(itemp, CoverpointRef);
                सचे (refp->exprp()) अनुवर्त;  // hierarchical ref: dropped in generateCrossCode
                सचे (m_coverpointMap.find(refp->name()) == m_coverpointMap.end()) {
                    m_droppedCrosses.निवेशय(crossp);  // bare variable: drop this cross only
                } अञ्ञथा {
                    m_crossedCpNames.निवेशय(refp->name());
                }
            }
        }

        std::vector<Ast_ग्रन्थिः*> pending;
        std::map<AstCoverpoint*, std::vector<AstCoverCross*>> consumers;
        std::map<AstCoverCross*, std::vector<AstCoverpoint*>> inputs;
        अत्थाय (AstCoverpoint* ध्रुव cpp : m_coverpoints) {
            checkBinNames(cpp);
            checkConstructedBins(cpp);
            सचे (!cpp->exprp()->dtypep()->skipRefp()->isIntegralOrPacked()) अनुवर्त;
            // Bins without values leave the report (IEEE 1800-2023 19.11.1), exclusions or not.
            // Constructed bins get their values when the covergroup is constructed.
            सचे (!coverpointHasStateExclusions(cpp) && !coverpointHasEmptyBins(cpp)
                && !coverpointHasConstructedBins(cpp)) {
                अनुवर्त;
            }
            m_runtimePoints.निवेशय(cpp);
            pending.प्रक्षिप_back(cpp);
        }
        अत्थाय (AstCoverCross* ध्रुव crossp : m_coverCrosses) {
            सचे (m_droppedCrosses.गणनम्(crossp)) अनुवर्त;
            अत्थाय (Ast_ग्रन्थिः* itemp = crossp->itemsp(); itemp; itemp = itemp->nextp()) {
                ध्रुव AstCoverpointRef* ध्रुव refp = VN_AS(itemp, CoverpointRef);
                सचे (refp->exprp()) अनुवर्त;
                ध्रुव auto point = m_coverpointMap.find(refp->name());
                सचे (point != m_coverpointMap.end()) {
                    consumers[point->second].प्रक्षिप_back(crossp);
                    inputs[crossp].प्रक्षिप_back(point->second);
                }
            }
            // The values of a filter's bin tuples are known when the covergroup is constructed
            सचे (crossp->exists([](ध्रुव AstCoverCrossWith*) { पटिनिवत्त सच्च; })) {
                m_runtimeCrosses.emplace(crossp);
                pending.प्रक्षिप_back(crossp);
            }
        }
        अत्थाय (size_t next = 0; next < pending.size(); ++next) {
            सचे (AstCoverpoint* ध्रुव pointp = VN_CAST(pending[next], Coverpoint)) {
                अत्थाय (AstCoverCross* ध्रुव crossp : consumers[pointp]) {
                    सचे (m_runtimeCrosses.emplace(crossp).second) pending.प्रक्षिप_back(crossp);
                }
            } अञ्ञथा {
                अत्थाय (AstCoverpoint* ध्रुव pointp : inputs[VN_AS(pending[next], CoverCross)]) {
                    सचे (pointp->exprp()->dtypep()->skipRefp()->isIntegralOrPacked()
                        && m_runtimePoints.emplace(pointp).second) {
                        pending.प्रक्षिप_back(pointp);
                    }
                }
            }
        }

        // The instance node owns this instance's coverpoint/cross runtimes, so it must exist
        // before any of them is created.  Emitted first, ahead of both generate loops.
        generateInstanceAttach();

        // For each coverpoint, generate sampling code
        अत्थाय (AstCoverpoint* cpp : m_coverpoints) generateCoverpointCode(cpp);

        // For each cross, generate sampling code
        अत्थाय (AstCoverCross* crossp : m_coverCrosses) generateCrossCode(crossp);

        // Every cross has been built, so runtime points only need their exclusions from here.
        अत्थाय (AstCoverpoint* ध्रुव cpp : m_coverpoints) {
            सचे (!m_runtimePoints.गणनम्(cpp)) अनुवर्त;
            m_constructorp->addStmtsp(itemCall(cpp->fileline(), m_cpVarMap.at(cpp->name()),
                                               VCMethod::COVERGROUP_मानम्_RELEASE)
                                          ->makeStmt());
        }
        अत्थाय (Ast_ग्रन्थिः_व्यञ्जकम्* valuep : m_detached_मानानि) VL_DO_DANGLING(प्रक्षिप_Deletep(valuep), valuep);
        m_detached_मानानि.clear();

        // Generate coverage computation code (even for empty covergroups).  Bin registration
        // with the coverage database is handled per coverpoint/cross by their runtime
        // registerBins() calls (emitted in generateCoverpoint/generateCross).
        generateCoverageComputationCode();
    }

    स्थिर constexpr size_t मानम्_LIST_ENTRIES = 256;  // Metadata entries per constructor call

    // The value of a constant count, of bins or value tuples: -1 if it is negative, and
    // saturated above the largest limit
    स्थिर int64_t const_गणनम्(ध्रुव AstConst* constp) {
        ध्रुव V3_सङ्ख्या& अङ्कः = constp->अङ्कः();
        सचे (constp->isSigned() && अङ्कः.isNegative()) पटिनिवत्त -1;
        पटिनिवत्त अङ्कः.mostSetBitP1() > 32 ? INT64_MAX : static_cast<int64_t>(अङ्कः.toUQuad());
    }

    // Whether a count, 'whatp', is integral and, if constant, positive, as the standard's clause
    // 'clausep' requires; else errors
    स्थिर तार्किक check_गणनम्(ध्रुव Ast_ग्रन्थिः_व्यञ्जकम्* countp, ध्रुव वर्ण* whatp, ध्रुव वर्ण* clausep) {
        ध्रुव AstConst* ध्रुव constp = VN_CAST(countp, Const);
        सचे (!countp->dtypep()->skipRefp()->isIntegralOrPacked()) {
            countp->v3error(whatp << " must be integral (IEEE 1800-2023 " << clausep << ")");
            पटिनिवत्त मिच्छा;
        } अञ्ञथा सचे (constp && (constp->अङ्कः().isFourState() || const_गणनम्(constp) < 1)) {
            countp->v3error(whatp << " must be >= 1, got "
                                  << (constp->अङ्कः().isFourState() ? constp->अङ्कः().ascii(मिच्छा)
                                                                  : constp->अङ्कः().toDecimalS())
                                  << " (IEEE 1800-2023 " << clausep << ")");
            पटिनिवत्त मिच्छा;
        }
        पटिनिवत्त सच्च;
    }

    // The number of bins requested by a valid 'bins auto[N]', or 0
    स्थिर uint32_t autoBinsRequested(ध्रुव AstCoverBin* binp) {
        ध्रुव AstConst* ध्रुव constp = VN_CAST(binp->arraySizep(), Const);
        सचे (!constp) पटिनिवत्त 0;
        ध्रुव int64_t गणनम् = const_गणनम्(constp);
        पटिनिवत्त गणनम् < 1 || गणनम् > v3Global.opt.coverageMaxBins() ? 0
                                                                   : static_cast<uint32_t>(गणनम्);
    }

    // True for a 'bins auto[N]' declaration, or the implicit automatic bins of a coverpoint
    स्थिर तार्किक isAutoBins(ध्रुव AstCoverBin* binp) {
        पटिनिवत्त binp->binsType() == VCoverBinsType::BINS_AUTO
               || binp->binsType() == VCoverBinsType::BINS_AUTO_IMPLICIT;
    }

    // True for a sized array of bins, 'bins b[N] = {...}', whose values an integral coverpoint
    // distributes over N bins when the covergroup is constructed (IEEE 1800-2023 19.5.1)
    स्थिर तार्किक isSizedArray(ध्रुव AstCoverBin* binp) {
        पटिनिवत्त binp->arraySizep() && !isAutoBins(binp);
    }

    // The 'with' filter of a bin, if any (IEEE 1800-2023 19.5.1.1)
    स्थिर AstCoverWith* binWith(ध्रुव AstCoverBin* binp) {
        पटिनिवत्त VN_CAST(binp->rangesp(), CoverWith);
    }

    // True for bins whose values the covergroup constructor computes: a sized array, or bins of
    // the values a 'with' filter keeps
    स्थिर तार्किक isConstructedBins(ध्रुव AstCoverBin* binp) {
        पटिनिवत्त isSizedArray(binp) || binWith(binp);
    }

    // The range list of a bin's values, or of the candidates of its 'with' filter; null for all
    // of the coverpoint's values, which a filter of the coverpoint's name has
    स्थिर Ast_ग्रन्थिः* binRangesp(ध्रुव AstCoverBin* binp) {
        ध्रुव AstCoverWith* ध्रुव withp = binWith(binp);
        सचे (!withp) पटिनिवत्त binp->rangesp();
        पटिनिवत्त VN_IS(withp->subp(), CoverpointRef) ? nullptr : withp->subp();
    }

    // Report and delete a bins declaration of a name that another of its coverpoint has
    शून्य checkBinNames(AstCoverpoint* coverpointp) {
        std::set<std::string> names;
        अत्थाय (Ast_ग्रन्थिः* nodep = coverpointp->binsp(); nodep;) {
            AstCoverBin* ध्रुव binp = VN_AS(nodep, CoverBin);
            nodep = nodep->nextp();
            सचे (names.emplace(binp->name()).second) अनुवर्त;
            binp->v3error("Duplicate bin " << binp->prettyNameQ() << " in coverpoint "
                                           << coverpointp->prettyNameQ()
                                           << " (IEEE 1800-2023 3.13)");
            VL_DO_DANGLING(प्रक्षिप_Deletep(binp->unlinkFrBack()), binp);
        }
    }

    // Delete an ignored bins declaration, which binsof then selects as no bins
    शून्य dropBins(ध्रुव AstCoverpoint* coverpointp, AstCoverBin* binp) {
        m_droppedBins[coverpointp].प्रक्षिप_back(binp->name());
        VL_DO_DANGLING(प्रक्षिप_Deletep(binp->unlinkFrBack()), binp);
    }

    // Check the size of a sized array of bins, which drops an invalid array.  A real coverpoint's
    // are unsupported, and treated as arrays of a bin per value.  False unless it stays sized.
    तार्किक checkBinsArraySize(ध्रुव AstCoverpoint* coverpointp, AstCoverBin* binp, तार्किक integral) {
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव sizep = binp->arraySizep();
        सचे (VN_IS(sizep, Unbounded)) {  // A parameter of '$'; see bins_orBraE
            binp->v3error("Bins array size must be integral, not '$' (IEEE 1800-2023 19.5.1)");
        } अञ्ञथा सचे (check_गणनम्(sizep, "Bins array size", "19.5.1")) {
            सचे (integral) पटिनिवत्त सच्च;
            binp->v3warn(COVERIGN, "Unsupported: 'bins' explicit array size of a real "
                                   "coverpoint (treated as '[]')");
            VL_DO_DANGLING(प्रक्षिप_Deletep(sizep->unlinkFrBack()), sizep);
            पटिनिवत्त मिच्छा;
        }
        dropBins(coverpointp, binp);
        पटिनिवत्त मिच्छा;
    }

    // Check the bins of a coverpoint whose values the constructor computes, dropping invalid
    // ones.  A wildcard array of too many ranges of values is ignored, or if ignore or illegal,
    // treated as one bin; filtering too many ranges of values ignores the bins.
    शून्य checkConstructedBins(AstCoverpoint* coverpointp) {
        ध्रुव तार्किक integral = coverpointp->exprp()->dtypep()->skipRefp()->isIntegralOrPacked();
        अत्थाय (Ast_ग्रन्थिः* nodep = coverpointp->binsp(); nodep;) {
            AstCoverBin* ध्रुव binp = VN_AS(nodep, CoverBin);
            nodep = nodep->nextp();
            सचे (!isConstructedBins(binp)) अनुवर्त;
            सचे (isSizedArray(binp) && !checkBinsArraySize(coverpointp, binp, integral)) अनुवर्त;
            सचे (!binp->isWildcard()
                || sizedWildcardRuns(binp, coverpointp->exprp())
                       <= v3Global.opt.coverageMaxBins()) {
                सचे (binWith(binp) && withCandidatesOver(binp, coverpointp->exprp())) {
                    binp->v3warn(COVERIGN, "Unsupported: 'with' filter of more than 2**32 "
                                           "candidate values; bin "
                                               << binp->prettyNameQ() << " ignored");
                    सचे (binp->binsType().binIsNormal()) coverpointp->user2(सच्च);
                    dropBins(coverpointp, binp);
                }
                अनुवर्त;
            }
            // An ignore or illegal array still excludes or checks its values, as one bin
            ध्रुव तार्किक single = !binp->binsType().binIsNormal() && !binWith(binp);
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
            सचे (single) {
                Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव sizep = binp->arraySizep();
                VL_DO_DANGLING(प्रक्षिप_Deletep(sizep->unlinkFrBack()), sizep);
                binp->isArray(मिच्छा);
                अनुवर्त;
            }
            सचे (binp->binsType().binIsNormal()) coverpointp->user2(सच्च);
            dropBins(coverpointp, binp);
        }
    }

    // True if a 'with' filter would be evaluated for more than 2**32 candidate values, known
    // now for the coverpoint's name, or for a range list of constants: each is evaluated once
    // but for an array 'b[N]', which keeps their order and duplicates (see withBegin())
    स्थिर तार्किक withCandidatesOver(AstCoverBin* binp, Ast_ग्रन्थिः_व्यञ्जकम्* exprp) {
        ध्रुव पूर्णाङ्क width = runWidth(exprp);
        std::vector<std::pair<V3_सङ्ख्या, V3_सङ्ख्या>> runs;
        सचे (!binRangesp(binp)) runs = coverpoint_मानानि(binp, exprp);
        ध्रुव auto constant
            = [](ध्रुव Ast_ग्रन्थिः* nodep) { पटिनिवत्त VN_IS(nodep, Const) || VN_IS(nodep, Unbounded); };
        अत्थाय (Ast_ग्रन्थिः* rangep = binRangesp(binp); rangep; rangep = rangep->nextp()) {
            ध्रुव AstInsideRange* ध्रुव irp = VN_CAST(rangep, InsideRange);
            // Else known when constructed
            सचे (irp ? !constant(irp->lhsp()) || !constant(irp->rhsp()) : !VN_IS(rangep, Const)) {
                पटिनिवत्त मिच्छा;
            }
            Cross_मानम्_Range range{rangep, resolveWidth(rangep, exprp)};
            सचे (!resolve_मानम्(rangep, exprp, सच्च, binp->isWildcard(), range)
                || crossRangeEmpty(range)) {
                अनुवर्त;
            }
            std::vector<std::pair<V3_सङ्ख्या, V3_सङ्ख्या>> found{{range.lo, range.hi}};
            सचे (range.wildcard) {  // checkConstructedBins bounded the runs
                found.clear();
                crossRangeRuns(range, v3Global.opt.coverageMaxBins(), found);
            }
            अत्थाय (ध्रुव std::pair<V3_सङ्ख्या, V3_सङ्ख्या>& run : found) {
                runs.emplace_back(V3_सङ्ख्या{rangep, width, run.first},
                                  V3_सङ्ख्या{rangep, width, run.second});
            }
        }
        सचे (!isSizedArray(binp)) {  // The union of the values
            std::sort(runs.begin(), runs.end(), [](ध्रुव auto& lhs, ध्रुव auto& rhs) {
                पटिनिवत्त cross_मानम्_Less(lhs.first, rhs.first);
            });
            std::vector<std::pair<V3_सङ्ख्या, V3_सङ्ख्या>> merged;
            अत्थाय (ध्रुव std::pair<V3_सङ्ख्या, V3_सङ्ख्या>& run : runs) {
                सचे (merged.empty() || cross_मानम्_Less(merged.back().second, run.first)) {
                    merged.प्रक्षिप_back(run);
                } अञ्ञथा सचे (cross_मानम्_Less(merged.back().second, run.second)) {
                    merged.back().second = run.second;
                }
            }
            runs = std::move(merged);
        }
        uint64_t गणनम् = 0;
        अत्थाय (ध्रुव std::pair<V3_सङ्ख्या, V3_सङ्ख्या>& run : runs) {
            V3_सङ्ख्या span{exprp, width};
            span.opSub(run.second, run.first);
            सचे (span.mostSetBitP1() > 32) पटिनिवत्त सच्च;  // More than 2**32 values
            गणनम् += span.toUQuad() + 1;
            सचे (गणनम् > (uint64_t{1} << 32)) पटिनिवत्त सच्च;
        }
        पटिनिवत्त मिच्छा;
    }

    // The ranges of values the wildcard patterns of a sized wildcard array, or of a 'with'
    // filter's candidates, give, counted up to more than --coverage-max-bins
    स्थिर size_t sizedWildcardRuns(ध्रुव AstCoverBin* binp, Ast_ग्रन्थिः_व्यञ्जकम्* exprp) {
        std::vector<std::pair<V3_सङ्ख्या, V3_सङ्ख्या>> runs;
        अत्थाय (Ast_ग्रन्थिः* rangep = binRangesp(binp); rangep; rangep = rangep->nextp()) {
            सचे (!VN_IS(rangep, Const)) अनुवर्त;  // A range, or a value known at construction
            Cross_मानम्_Range range{rangep, resolveWidth(rangep, exprp)};
            सचे (resolve_मानम्(rangep, exprp, सच्च, सच्च, range) && !crossRangeEmpty(range)) {
                crossRangeRuns(range, v3Global.opt.coverageMaxBins(), runs);
            }
        }
        पटिनिवत्त runs.size();
    }

    // Check the automatic bins declarations of a coverpoint.  Each stays one declaration, which
    // generates as a partition of the coverpoint domain (see autoBinRuns).
    शून्य checkAutomaticBins(AstCoverpoint* coverpointp, ध्रुव Ast_ग्रन्थिः_व्यञ्जकम्* exprp) {
        अत्थाय (Ast_ग्रन्थिः* binp = coverpointp->binsp(); binp; binp = binp->nextp()) {
            AstCoverBin* ध्रुव cbinp = VN_AS(binp, CoverBin);
            सचे (cbinp->binsType() != VCoverBinsType::BINS_AUTO) अनुवर्त;
            ध्रुव AstConst* ध्रुव constp = VN_CAST(cbinp->arraySizep(), Const);
            सचे (!constp) {
                cbinp->v3error("Automatic bins array size must be a constant");
            } अञ्ञथा सचे (const_गणनम्(constp) < 1) {
                cbinp->v3error("Automatic bins array size must be >= 1, got "
                               << constp->अङ्कः().toDecimalS());
            } अञ्ञथा सचे (const_गणनम्(constp) > v3Global.opt.coverageMaxBins()) {
                cbinp->v3error("Automatic bins array size of "
                               << constp->अङ्कः().toDecimalU() << " exceeds limit of "
                               << v3Global.opt.coverageMaxBins() << '\n'
                               << cbinp->warnMore() << "... Suggest a larger --coverage-max-bins");
            } अञ्ञथा सचे (!exprp->dtypep()->skipRefp()->isIntegralOrPacked()) {
                cbinp->v3error("Automatic bins are not allowed on a coverpoint of a non-integral "
                               "expression (IEEE 1800-2023 19.5.3).");
            }
        }
    }

    // Extract all option values of a coverpoint or cross in a single pass.
    // atLeastOut: option.at_least (default the covergroup's)
    // autoBinMaxOut: option.auto_bin_max (coverpoint overrides covergroup, default 64)
    शून्य extractItemOptions(Ast_ग्रन्थिः* optionsp, पूर्णाङ्क& atLeastOut, पूर्णाङ्क& autoBinMaxOut) {
        atLeastOut = m_cgAtLeast;
        autoBinMaxOut = -1;  // -1 = not set at coverpoint level
        अत्थाय (Ast_ग्रन्थिः* optionp = optionsp; optionp; optionp = optionp->nextp()) {
            AstCoverOption* ध्रुव optp = VN_AS(optionp, CoverOption);
            // Weights may be non-constant; generateItemWeight() handles them.  A cross keeps
            // its unsupported options, which V3LinkParse warned of.
            सचे (!(optp->optType() == VCoverOptionType::AT_LEAST
                  || optp->optType() == VCoverOptionType::AUTO_BIN_MAX)) {
                अनुवर्त;
            }
            ध्रुव AstConst* ध्रुव constp = constOption(optp->valuep(), optp->optType());
            सचे (!constp) अनुवर्त;
            सचे (optp->optType() == VCoverOptionType::AT_LEAST) {
                atLeastOut = constAtLeast(constp);
            } अञ्ञथा {
                autoBinMaxOut = constp->toSInt();
            }
        }
        // Fall back to covergroup-level auto_bin_max if not set at coverpoint level
        सचे (autoBinMaxOut < 0) {
            सचे (m_covergroupp->cgAutoBinMax() >= 0) {
                autoBinMaxOut = m_covergroupp->cgAutoBinMax();
            } अञ्ञथा {
                autoBinMaxOut = 64;  // Default per IEEE 1800-2023 Table 19-1
            }
        }
    }

    // IEEE 1800-2023 19.5.2: an enum coverpoint has one automatic bin per enumeration value
    शून्य createEnumAutoBins(AstCoverpoint* coverpointp, Ast_ग्रन्थिः_व्यञ्जकम्* exprp,
                            ध्रुव AstEnumDType* enump) {
        File_पङ्क्तिः* ध्रुव fl = coverpointp->fileline();
        अत्थाय (ध्रुव AstEnumItem* itemp = enump->itemsp(); itemp;
             itemp = VN_AS(itemp->nextp(), EnumItem)) {
            AstConst* ध्रुव lop = new_मानम्_Const(fl, VN_AS(itemp->valuep(), Const)->अङ्कः(), exprp);
            AstInsideRange* ध्रुव rangep = नूतन AstInsideRange{fl, lop, lop->clone_वृक्षः(मिच्छा)};
            rangep->dtypeFrom(exprp);
            coverpointp->addBinsp(
                नूतन AstCoverBin{fl, "auto[" + itemp->name() + "]", rangep, मिच्छा, मिच्छा});
        }
    }

    // IEEE 1800-2023 19.5.3/19.11.1: partition first, then apply exclusions.  The partition is one
    // automatic bins declaration, generated as a run like 'bins auto[N]' but numbering its bins.
    शून्य createImplicitAutoBins(AstCoverpoint* coverpointp, Ast_ग्रन्थिः_व्यञ्जकम्* exprp, पूर्णाङ्क autoBinMax) {
        सचे (coverpointp->user2()) पटिनिवत्त;  // Declared bins, ignored, leave no bins
        अत्थाय (Ast_ग्रन्थिः* nodep = coverpointp->binsp(); nodep; nodep = nodep->nextp()) {
            ध्रुव VCoverBinsType kind = VN_AS(nodep, CoverBin)->binsType();
            सचे (kind != VCoverBinsType::BINS_IGNORE && kind != VCoverBinsType::BINS_ILLEGAL)
                पटिनिवत्त;
        }
        सचे (ध्रुव AstEnumDType* ध्रुव enump
            = VN_CAST(exprp->dtypep()->skipRefToEnump(), EnumDType)) {
            createEnumAutoBins(coverpointp, exprp, enump);
            पटिनिवत्त;
        }
        ध्रुव पूर्णाङ्क width = exprp->width();
        uint32_t गणनम् = width < 31 ? std::min<uint32_t>(uint32_t{1} << width, autoBinMax)
                                    : static_cast<uint32_t>(autoBinMax);
        सचे (!गणनम्) पटिनिवत्त;
        सचे (!exprp->dtypep()->skipRefp()->isIntegralOrPacked()) {
            coverpointp->v3error("Coverpoint of a non-integral expression requires explicit bins "
                                 "(IEEE 1800-2023 19.5.3).");
            पटिनिवत्त;
        }
        सचे (गणनम् > v3Global.opt.coverageMaxBins()) {
            coverpointp->v3warn(COVERIGN, "Unsupported: more than "
                                              << v3Global.opt.coverageMaxBins()
                                              << " automatic bins from 'option.auto_bin_max'; "
                                                 "using "
                                              << v3Global.opt.coverageMaxBins() << ".\n"
                                              << coverpointp->warnMore()
                                              << "... Suggest a larger --coverage-max-bins");
            गणनम् = v3Global.opt.coverageMaxBins();
        }
        File_पङ्क्तिः* ध्रुव fl = coverpointp->fileline();
        coverpointp->addBinsp(नूतन AstCoverBin{fl, "auto", नूतन AstConst{fl, गणनम्},
                                              VCoverBinsType::BINS_AUTO_IMPLICIT});
    }

    // Sanitize generated names to be valid C++ identifiers
    स्थिर string sanitizeGeneratedName(string name) {
        std::replace(name.begin(), name.end(), '[', '_');
        std::replace(name.begin(), name.end(), ']', '_');
        पटिनिवत्त name;
    }

    // Capture an iff guard in a function-local temporary so it is evaluated once per sample()
    AstVarRef* captureIffToTemp(Ast_ग्रन्थिः_व्यञ्जकम्* iffp, ध्रुव string& tempName) {
        File_पङ्क्तिः* ध्रुव fl = iffp->fileline();
        AstVar* ध्रुव iffVarp
            = नूतन AstVar{fl, VVarType::BLOCKTEMP, tempName, iffp->findBitDType()};
        iffVarp->funcLocal(सच्च);
        m_sampleFuncp->addStmtsp(iffVarp);
        iffp->unlinkFrBack();
        m_sampleFuncp->addStmtsp(
            नूतन AstAssign{fl, नूतन AstVarRef{fl, iffVarp, VAccess::WRITE}, iffp});
        पटिनिवत्त नूतन AstVarRef{fl, iffVarp, VAccess::READ};
    }

    // Add a statement sampling the current coverpoint to sample(), under its iff guard if any
    शून्य addSampleStmt(Ast_ग्रन्थिः* stmtp) {
        सचे (m_cpGuardp) {
            m_cpGuardp->addThensp(stmtp);
        } अञ्ञथा {
            m_sampleFuncp->addStmtsp(stmtp);
        }
    }

    // Create previous value variable for transition tracking
    AstVar* create_Prev_मानम्_Var(AstCoverpoint* coverpointp, Ast_ग्रन्थिः_व्यञ्जकम्* exprp) {
        // Check if already created
        सचे (AstVar* ध्रुव prevVarp = VN_CAST(coverpointp->user1p(), Var)) पटिनिवत्त prevVarp;

        // Create variable to store previous sampled value
        ध्रुव string varName = "__Vprev_" + coverpointp->name();
        AstVar* prevVarp
            = नूतन AstVar{coverpointp->fileline(), VVarType::MEMBER, varName, exprp->dtypep()};
        m_covergroupp->addMembersp(prevVarp);

        UINFO(4, "    Created previous value variable: " << varName);

        // Initialize to zero in constructor
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव initExprp
            = नूतन AstConst{prevVarp->fileline(), AstConst::Widthed_मानम्{}, prevVarp->width(), 0};
        Ast_ग्रन्थिः_Stmt* ध्रुव initStmtp = नूतन AstAssign{
            prevVarp->fileline(), नूतन AstVarRef{prevVarp->fileline(), prevVarp, VAccess::WRITE},
            initExprp};
        m_constructorp->addStmtsp(initStmtp);

        coverpointp->user1p(prevVarp);
        पटिनिवत्त prevVarp;
    }

    // Create state position variable for multi-value transition bins
    // Tracks position in sequence: 0=not started, 1=seen first item, etc.
    AstVar* createSequenceStateVar(AstCoverpoint* coverpointp, AstCoverBin* binp) {
        // Create variable to track sequence position
        ध्रुव string varName = "__Vseqpos_" + coverpointp->name() + "_" + binp->name();
        // Use 8-bit integer for state position (sequences rarely > 255 items)
        AstVar* stateVarp
            = नूतन AstVar{binp->fileline(), VVarType::MEMBER, varName, VFlagLogicPacked{}, 8};
        m_covergroupp->addMembersp(stateVarp);

        UINFO(4, "    Created sequence state variable: " << varName);

        // Initialize to 0 (not started) in constructor
        Ast_ग्रन्थिः_Stmt* ध्रुव initStmtp = नूतन AstAssign{
            stateVarp->fileline(), नूतन AstVarRef{stateVarp->fileline(), stateVarp, VAccess::WRITE},
            नूतन AstConst{stateVarp->fileline(), AstConst::Widthed_मानम्{}, 8, 0}};
        m_constructorp->addStmtsp(initStmtp);

        पटिनिवत्त stateVarp;
    }

    शून्य generateCoverpointCode(AstCoverpoint* coverpointp) {
        UINFO(4, "  Generating code for coverpoint: " << coverpointp->name());

        // Get the coverpoint expression
        Ast_ग्रन्थिः_व्यञ्जकम्* exprp = coverpointp->exprp();

        // Check automatic bins before processing
        checkAutomaticBins(coverpointp, exprp);

        // Extract all coverpoint options in a single pass
        पूर्णाङ्क at_Least_मानम्;
        पूर्णाङ्क autoBinMax;
        extractItemOptions(coverpointp->optionsp(), at_Least_मानम्, autoBinMax);
        UINFO(6, "    Coverpoint at_least = " << at_Least_मानम् << " auto_bin_max = " << autoBinMax);

        // Create implicit automatic bins if no regular bins exist
        createImplicitAutoBins(coverpointp, exprp, autoBinMax);

        // A false iff guard disables sampling of the coverpoint (IEEE 1800-2023 19.5), so
        // neither is its expression evaluated, nor are its bins or transition state updated
        VL_RESTORER(m_cpGuardp);
        सचे (Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव iffp = coverpointp->iffp()) {
            m_cpGuardp = नूतन AstIf{iffp->fileline(), iffp->unlinkFrBack()};
        }

        AstVar* ध्रुव मानम्_Varp = नूतन AstVar{
            coverpointp->fileline(), VVarType::BLOCKTEMP,
            "__VcpValue_" + sanitizeGeneratedName(coverpointp->name()), exprp->dtypep()};
        मानम्_Varp->funcLocal(सच्च);
        m_sampleFuncp->addStmtsp(मानम्_Varp);
        exprp->unlinkFrBack();
        addSampleStmt(नूतन AstAssign{
            coverpointp->fileline(),
            नूतन AstVarRef{coverpointp->fileline(), मानम्_Varp, VAccess::WRITE}, exprp});
        coverpointp->exprp(नूतन AstVarRef{coverpointp->fileline(), मानम्_Varp, VAccess::READ});
        exprp = coverpointp->exprp();

        // Every coverpoint routes through the VlCoverpoint runtime.  Transition coverpoints are
        // included: their per-value matching is still generated as a state machine in sample()
        // (see generateCoverpoint), but the bin hit is recorded in the runtime bin
        // rather than a bare counter.
        generateCoverpoint(coverpointp, exprp, at_Least_मानम्);
        // Sampling follows what generateCoverpoint added unguarded, such as clearing the hit list
        सचे (m_cpGuardp) m_sampleFuncp->addStmtsp(m_cpGuardp);
    }

    // Build the condition under which a default bin matches: NOT(OR of all normal bins).
    Ast_ग्रन्थिः_व्यञ्जकम्* buildDefaultCondition(AstCoverpoint* coverpointp, Ast_ग्रन्थिः_व्यञ्जकम्* exprp,
                                       File_पङ्क्तिः* fl) {
        Ast_ग्रन्थिः_व्यञ्जकम्* anyBinMatchp = nullptr;
        अत्थाय (Ast_ग्रन्थिः* binp = coverpointp->binsp(); binp; binp = binp->nextp()) {
            AstCoverBin* ध्रुव cbinp = VN_AS(binp, CoverBin);
            सचे (cbinp->binsType() == VCoverBinsType::BINS_DEFAULT
                || cbinp->binsType() == VCoverBinsType::BINS_IGNORE
                || cbinp->binsType() == VCoverBinsType::BINS_ILLEGAL)
                अनुवर्त;
            // The values of constructed bins are known at construction; see emitSizedSample
            सचे (isConstructedBins(cbinp)) अनुवर्त;
            सचे (isAutoBins(cbinp)) {
                // Automatic bins partition the whole domain, leaving no default value
                सचे (anyBinMatchp) VL_DO_DANGLING(प्रक्षिप_Deletep(anyBinMatchp), anyBinMatchp);
                पटिनिवत्त नूतन AstConst{fl, AstConst::BitFalse{}};
            }
            Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव binCondp = buildBinCondition(cbinp, exprp);
            UASSERT_OBJ(binCondp, cbinp,
                        "buildBinCondition returned nullptr for non-ignore/non-illegal bin");
            anyBinMatchp = anyBinMatchp ? नूतन AstOr{fl, anyBinMatchp, binCondp} : binCondp;
        }
        पटिनिवत्त anyBinMatchp ? static_cast<Ast_ग्रन्थिः_व्यञ्जकम्*>(नूतन AstNot{fl, anyBinMatchp})
                            : static_cast<Ast_ग्रन्थिः_व्यञ्जकम्*>(नूतन AstConst{fl, AstConst::BitTrue{}});
    }

    //====================================================================
    // VlCoverpoint conversion

    स्थिर तार्किक coverpointHasStateExclusions(ध्रुव AstCoverpoint* coverpointp) {
        अत्थाय (ध्रुव Ast_ग्रन्थिः* nodep = coverpointp->binsp(); nodep; nodep = nodep->nextp()) {
            ध्रुव AstCoverBin* ध्रुव binp = VN_AS(nodep, CoverBin);
            सचे (!binp->transp() && binp->rangesp()
                && (binp->binsType() == VCoverBinsType::BINS_IGNORE
                    || binp->binsType() == VCoverBinsType::BINS_ILLEGAL)) {
                पटिनिवत्त सच्च;
            }
        }
        पटिनिवत्त मिच्छा;
    }

    स्थिर तार्किक coverpointHasConstructedBins(ध्रुव AstCoverpoint* coverpointp) {
        अत्थाय (ध्रुव Ast_ग्रन्थिः* nodep = coverpointp->binsp(); nodep; nodep = nodep->nextp()) {
            सचे (isConstructedBins(VN_AS(nodep, CoverBin))) पटिनिवत्त सच्च;
        }
        पटिनिवत्त मिच्छा;
    }

    // True if a Normal state bin, or an array-bin element, has no value of the coverpoint's
    // type (IEEE 1800-2023 19.5.7).  Array ranges enumerate in-type values, so cannot vanish.
    स्थिर तार्किक coverpointHasEmptyBins(ध्रुव AstCoverpoint* coverpointp) {
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव exprp = coverpointp->exprp();
        अत्थाय (Ast_ग्रन्थिः* nodep = coverpointp->binsp(); nodep; nodep = nodep->nextp()) {
            ध्रुव AstCoverBin* ध्रुव binp = VN_AS(nodep, CoverBin);
            सचे (!binp->binsType().binIsNormal() || binp->transp() || !binp->rangesp()) अनुवर्त;
            // A wildcard array has a bin for each value its elements match, so none empty, and
            // the constructor creates no bin without values of a 'with' filter
            सचे ((binp->isArray() && binp->isWildcard()) || binWith(binp)) अनुवर्त;
            तार्किक empty = सच्च;
            अत्थाय (Ast_ग्रन्थिः* valuep = binp->rangesp(); valuep; valuep = valuep->nextp()) {
                सचे (binp->isArray() && VN_IS(valuep, InsideRange)) अनुवर्त;
                Cross_मानम्_Range range{valuep, resolveWidth(valuep, exprp)};
                ध्रुव तार्किक none = resolve_मानम्(valuep, exprp, सच्च, binp->isWildcard(), range)
                                  && crossRangeEmpty(range);
                सचे (binp->isArray() && none) पटिनिवत्त सच्च;
                empty &= none;
            }
            सचे (empty && !binp->isArray()) पटिनिवत्त सच्च;
        }
        पटिनिवत्त मिच्छा;
    }

    // True if a coverpoint has any transition bin.  Used to decide whether sample() emits the
    // end-of-sample previous-value update that transition matching needs.
    स्थिर तार्किक coverpointHasTransition(AstCoverpoint* coverpointp) {
        अत्थाय (Ast_ग्रन्थिः* binp = coverpointp->binsp(); binp; binp = binp->nextp()) {
            सचे (VN_AS(binp, CoverBin)->transp()) पटिनिवत्त सच्च;
        }
        पटिनिवत्त मिच्छा;
    }

    // The interned AstBasicDType for one of the covergroup runtime keywords.
    स्थिर AstBasicDType* basicDType(File_पङ्क्तिः* fl, VBasicDTypeKwd kwd) {
        पटिनिवत्त v3Global.rootp()->typeTablep()->findBasicDType(fl, kwd);
    }

    // Get (or create) the coverpoint dtype for a hit-list bound, interned so each distinct bound
    // yields one node.
    AstCoverpointDType* coverpointDType(File_पङ्क्तिः* fl, uint32_t hitBound) {
        AstCoverpointDType*& typep = m_cpDTypes[hitBound];
        सचे (!typep) {
            typep = नूतन AstCoverpointDType{fl, hitBound};
            v3Global.rootp()->typeTablep()->addTypesp(typep);
        }
        पटिनिवत्त typep;
    }

    // The key of the covergroup's type in the coverage registry, which get_coverage() queries
    std::string covergroupProtectedName() ध्रुव {
        पटिनिवत्त VIdProtect::protect_शब्दाः_If(m_covergroupName, v3Global.opt.protectIds());
    }

    // The name of the covergroup's type, which keys the coverage registry and database: as
    // $typename names it (IEEE 1800-2023 20.6.1), so apart for covergroups of distinct scopes and
    // specializations, and alike in each Verilator run of a hierarchical design.  As design units
    // of distinct libraries may share a name, one of a library other than the default is prefixed
    // by its library, as '%l' prints it (IEEE 1800-2023 33.4).
    std::string covergroupTypeName() ध्रुव {
        UASSERT_OBJ(m_unitp, m_covergroupp, "Covergroup declared outside of a design unit");
        ध्रुव std::string& libname = m_unitp->libname();
        ध्रुव std::string name = m_covergroupp->dtypeName(सच्च);
        पटिनिवत्त libname == "work" ? name : libname + "." + name;
    }

    // Emit the covergroup's instance handle member and the constructor statement that creates
    // its node in the per-context coverage registry.  Runs before any coverpoint or cross is
    // generated, so their runtimes can be added to the node as they are created.
    शून्य generateInstanceAttach() {
        File_पङ्क्तिः* ध्रुव fl = m_covergroupp->fileline();
        // V3LinkParse synthesizes a 'new' for every covergroup; the item generators below already
        // rely on that, and this attach runs even for a covergroup with no coverpoints at all.
        UASSERT_OBJ(m_constructorp, m_covergroupp, "Covergroup missing synthesized constructor");
        m_cgInstVarp = नूतन AstVar{fl, VVarType::MEMBER, "__Vcg_inst",
                                  basicDType(fl, VBasicDTypeKwd::COVERGROUP_INSTHANDLE)};
        m_covergroupp->addMembersp(m_cgInstVarp);

        m_constructorp->addStmtsp(
            itemCall(fl, m_cgInstVarp, VCMethod::COVERGROUP_ATTACH,
                     {ctext(fl, "vlSymsp->_vm_contextp__->covergroupRegistryp()"
                                "->newCovergroupInst("
                                    + quoted(covergroupProtectedName())
                                    + (m_cgMayMerge ? ", true" : ", false") + ")")},
                     /*usePtr=*/मिच्छा)
                ->makeStmt());
        // The node reads option.weight in place, so procedural assignments take effect
        AstCExpr* ध्रुव weightAddrp = नूतन AstCExpr{fl, "&"};
        weightAddrp->add(newOptionSel(fl, optionVar(मिच्छा), "weight", VAccess::READ));
        m_constructorp->addStmtsp(itemCall(fl, m_cgInstVarp, VCMethod::COVERGROUP_LEND_WEIGHT,
                                           {weightAddrp, file_पङ्क्तिः_Debug(fl)}, /*usePtr=*/मिच्छा)
                                      ->makeStmt());
    }

    // A '__Vcg_inst.p()-><method>()' call on the covergroup's instance node
    AstCMethodHard* instanceCall(File_पङ्क्तिः* fl, VCMethod method) {
        // '__Vcg_inst.p()' -- a value handle, so '.' not '->'
        AstCMethodHard* ध्रुव instp
            = नूतन AstCMethodHard{fl, memberRef(fl, m_cgInstVarp), VCMethod::COVERGROUP_INST_P};
        instp->usePtr(मिच्छा);
        instp->dtypeSetVoid();  // Opaque receiver; only ever the 'fromp' of the call below
        AstCMethodHard* ध्रुव callp = नूतन AstCMethodHard{fl, instp, method};
        callp->usePtr(सच्च);
        पटिनिवत्त callp;
    }

    // Emit 'this->__Vcp_x = this->__Vcg_inst.p()->addCoverpoint<K>();' (or addCross), which
    // creates the item runtime in the instance node and borrows a pointer to it.
    AstAssign* makeItemCreate(File_पङ्क्तिः* fl, AstVar* itemVarp, VCMethod method) {
        AstCMethodHard* ध्रुव createp = instanceCall(fl, method);
        createp->dtypep(itemVarp->dtypep());
        पटिनिवत्त नूतन AstAssign{fl, memberRef(fl, itemVarp, VAccess::WRITE), createp};
    }

    // Constant bounds of one rangesp() element (an InsideRange or a single Const).  Each bound is
    // the raw AST node -- an AstConst or an AstUnbounded ('$') -- with the const/unbounded view
    // derived on demand, so there is one source of truth per bound.  After a successful
    // constRangeBounds() neither node is null; a single Const has both bounds aliasing one node.
    संरचना RangeBounds final {
        Ast_ग्रन्थिः* loNodep = nullptr;  // low bound: AstConst or AstUnbounded
        Ast_ग्रन्थिः* hiNodep = nullptr;  // high bound: AstConst or AstUnbounded
        तार्किक loUnbounded() ध्रुव { पटिनिवत्त VN_IS(loNodep, Unbounded); }
        तार्किक hiUnbounded() ध्रुव { पटिनिवत्त VN_IS(hiNodep, Unbounded); }
        AstConst* loConstp() ध्रुव { पटिनिवत्त VN_CAST(loNodep, Const); }
        AstConst* hiConstp() ध्रुव { पटिनिवत्त VN_CAST(hiNodep, Const); }
    };

    // Decode one rangesp() element into its constant bounds.  Returns false if rp is neither an
    // InsideRange nor a single Const, or if a present bound is non-constant or 4-state.  '$'
    // bounds are left as AstUnbounded (not resolved) -- the caller applies its own policy.
    // Centralizes the InsideRange/Const/Unbounded decode shared by the hit-list-bound paths.
    स्थिर तार्किक constRangeBounds(Ast_ग्रन्थिः* rp, RangeBounds& rb) {
        सचे (AstInsideRange* ध्रुव irp = VN_CAST(rp, InsideRange)) {
            rb.loNodep = irp->lhsp();
            rb.hiNodep = irp->rhsp();
        } अञ्ञथा सचे (AstConst* ध्रुव cp = VN_CAST(rp, Const)) {
            rb.loNodep = rb.hiNodep = cp;
        } अञ्ञथा {
            पटिनिवत्त मिच्छा;
        }
        // Each bound must be a constant unless it is '$'; reject non-const and 4-state.
        AstConst* ध्रुव lc = rb.loConstp();
        AstConst* ध्रुव hc = rb.hiConstp();
        सचे ((!lc && !rb.loUnbounded()) || (!hc && !rb.hiUnbounded())) पटिनिवत्त मिच्छा;
        सचे ((lc && lc->अङ्कः().isFourState()) || (hc && hc->अङ्कः().isFourState())) पटिनिवत्त मिच्छा;
        पटिनिवत्त सच्च;
    }

    // Collect the covered value intervals of a single (non-array) Normal bin.  Returns false
    // if any range isn't a constant/open InsideRange or single Const (e.g. wildcard, non-const).
    स्थिर तार्किक extractRangeIntervals(AstCoverBin* cbinp, uint64_t maxVal,
                                      std::vector<std::pair<uint64_t, uint64_t>>& out) {
        सचे (!cbinp->rangesp()) पटिनिवत्त मिच्छा;
        अत्थाय (Ast_ग्रन्थिः* rp = cbinp->rangesp(); rp; rp = rp->nextp()) {
            RangeBounds rb;
            सचे (!constRangeBounds(rp, rb)) पटिनिवत्त मिच्छा;
            सचे ((rb.loConstp() && rb.loConstp()->width() > 64)
                || (rb.hiConstp() && rb.hiConstp()->width() > 64)) {
                पटिनिवत्त मिच्छा;  // Use the safe slot-count bound for wide values.
            }
            ध्रुव uint64_t lo = rb.loUnbounded() ? 0 : rb.loConstp()->toUQuad();
            ध्रुव uint64_t hi = rb.hiUnbounded() ? maxVal : rb.hiConstp()->toUQuad();
            सचे (lo > hi) पटिनिवत्त मिच्छा;
            out.emplace_back(lo, hi);
        }
        पटिनिवत्त सच्च;
    }

    // Append one Normal bin's cross-slot interval-sets to `bins` and bump `slotCount` by an upper
    // bound of the bin's slots holding one value.  Returns false if any part isn't statically
    // enumerable (the caller then falls back to the always-safe slot count).  A non-array bin
    // is one slot covering the union of its intervals; the bins of an array element or of an
    // automatic bins declaration hold disjoint values, so the element or declaration counts once.
    तार्किक appendBinCrossSlots(AstCoverBin* cbinp, uint64_t maxVal, Ast_ग्रन्थिः_व्यञ्जकम्* exprp,
                             std::vector<std::vector<std::pair<uint64_t, uint64_t>>>& bins,
                             पूर्णाङ्क& slot_गणनम्) {
        सचे (isAutoBins(cbinp)) {
            ++slot_गणनम्;
            bins.प्रक्षिप_back({{0, maxVal}});
            पटिनिवत्त exprp->width() <= 64;
        }
        सचे (binWith(cbinp)) {
            // A value is in one bin of a filter's, but of a sized array, in one for each range
            // list element holding it; the coverpoint's name is one element
            पूर्णाङ्क elements = 0;
            अत्थाय (ध्रुव Ast_ग्रन्थिः* rp = binRangesp(cbinp); rp && isSizedArray(cbinp);
                 rp = rp->nextp()) {
                ++elements;
            }
            slot_गणनम् += std::max(1, elements);
            पटिनिवत्त मिच्छा;
        }
        सचे (cbinp->isArray() && cbinp->isWildcard() && !cbinp->arraySizep()) {
            // A value is in at most one bin of a wildcard array: one slot covering its values.
            // Signed values are sign-extended, not unsigned intervals (see computeHitListBound)
            ++slot_गणनम्;
            सचे (exprp->isSigned() || exprp->width() > 64) पटिनिवत्त मिच्छा;
            ध्रुव BinRuns runs = wildcardBinRuns(cbinp, exprp, मिच्छा);
            सचे (runs.unsupported) पटिनिवत्त मिच्छा;
            std::vector<std::pair<uint64_t, uint64_t>> ivs;
            अत्थाय (ध्रुव BinRun& run : runs.runs) {
                ivs.emplace_back(run.m_lo.toUQuad(), run.m_hi.toUQuad());
            }
            bins.प्रक्षिप_back(std::move(ivs));
            पटिनिवत्त सच्च;
        }
        सचे (cbinp->isArray()) पटिनिवत्त appendArrayBinCrossSlots(cbinp, exprp, bins, slot_गणनम्);
        // Non-array bin: one slot covering the union of its intervals.
        ++slot_गणनम्;
        std::vector<std::pair<uint64_t, uint64_t>> ivs;
        सचे (cbinp->isWildcard() || !extractRangeIntervals(cbinp, maxVal, ivs)) पटिनिवत्त मिच्छा;
        bins.प्रक्षिप_back(std::move(ivs));
        पटिनिवत्त सच्च;
    }

    // Append the cross slots of an array Normal bin: each element is a slot covering its
    // values, as at most one of its single-value bins holds a value.  An element holds the
    // values arrayBinRuns() gives it: those of the coverpoint type (IEEE 1800-2023 19.5.7).
    // Elements of a signed coverpoint, non-constant elements, and elements with values beyond
    // 64 bits can't be enumerated, so they count one slot but lose exactness.  Returns false if
    // any element wasn't enumerable to exact values.
    तार्किक appendArrayBinCrossSlots(AstCoverBin* cbinp, Ast_ग्रन्थिः_व्यञ्जकम्* exprp,
                                  std::vector<std::vector<std::pair<uint64_t, uint64_t>>>& bins,
                                  पूर्णाङ्क& slot_गणनम्) {
        // Signed values resolve sign-extended, not as unsigned intervals, and wildcard patterns
        // are not intervals
        तार्किक exact = !exprp->isSigned() && !cbinp->isWildcard();
        अत्थाय (Ast_ग्रन्थिः* rp = cbinp->rangesp(); rp; rp = rp->nextp()) {
            ++slot_गणनम्;
            RangeBounds rb;
            Cross_मानम्_Range range{rp, resolveWidth(rp, exprp)};
            सचे (!exact || !constRangeBounds(rp, rb)
                || !resolve_मानम्(rp, exprp, सच्च, मिच्छा, range)) {
                exact = मिच्छा;
                अनुवर्त;
            }
            सचे (crossRangeEmpty(range)) अनुवर्त;  // Its bin, if any, holds no value
            सचे (range.hi.mostSetBitP1() > 64) {
                exact = मिच्छा;
                अनुवर्त;
            }
            bins.प्रक्षिप_back({{range.lo.toUQuad(), range.hi.toUQuad()}});
        }
        पटिनिवत्त exact;
    }

    // Compute the hit-list bound for a coverpoint: the maximum number of Normal
    // bins one sample value can match.  Non-cross-fed coverpoints don't feed a cross, so
    // their hit list is unused -> 1.  Otherwise compute the exact max bin overlap; fall back
    // to the (always-safe) Normal-slot count when any bin isn't statically analyzable.
    पूर्णाङ्क computeHitListBound(AstCoverpoint* coverpointp, Ast_ग्रन्थिः_व्यञ्जकम्* exprp, तार्किक crossFed) {
        सचे (!crossFed) पटिनिवत्त 1;
        ध्रुव पूर्णाङ्क width = exprp->width();
        ध्रुव uint64_t maxVal = (width >= 64) ? UINT64_MAX : ((1ULL << width) - 1);
        // One entry per Normal bin (cross slot): its covered intervals.
        std::vector<std::vector<std::pair<uint64_t, uint64_t>>> bins;
        पूर्णाङ्क slot_गणनम् = 0;  // == runtime m_normal; the safe fallback bound
        // Unsigned intervals cannot establish overlap between differently sized signed values.
        तार्किक exact = !exprp->isSigned();
        अत्थाय (Ast_ग्रन्थिः* binp = coverpointp->binsp(); binp; binp = binp->nextp()) {
            AstCoverBin* ध्रुव cbinp = VN_AS(binp, CoverBin);
            सचे (!cbinp->binsType().binIsNormal())
                अनुवर्त;  // ignore/illegal/default: not hit-listed
            सचे (!appendBinCrossSlots(cbinp, maxVal, exprp, bins, slot_गणनम्)) exact = मिच्छा;
        }
        सचे (!exact) पटिनिवत्त std::max(1, slot_गणनम्);
        सचे (bins.empty()) पटिनिवत्त 1;
        // Max overlap occurs at some interval start; count covering bins at each lo.
        std::vector<uint64_t> pts;
        अत्थाय (ध्रुव auto& b : bins)
            अत्थाय (ध्रुव auto& iv : b) pts.प्रक्षिप_back(iv.first);
        std::sort(pts.begin(), pts.end());
        pts.erase(std::unique(pts.begin(), pts.end()), pts.end());
        पूर्णाङ्क maxOverlap = 1;
        अत्थाय (ध्रुव uint64_t p : pts) {
            पूर्णाङ्क cnt = 0;
            अत्थाय (ध्रुव auto& b : bins) {
                अत्थाय (ध्रुव auto& iv : b) {
                    सचे (iv.first <= p && p <= iv.second) {
                        ++cnt;
                        भङ्ग;  // count each bin at most once
                    }
                }
            }
            सचे (cnt > maxOverlap) maxOverlap = cnt;
        }
        पटिनिवत्त maxOverlap;
    }

    // A 'this->m_member' reference for embedding in an AstCStmt
    AstVarRef* memberRef(File_पङ्क्तिः* fl, AstVar* varp, VAccess access = VAccess::READ) {
        AstVarRef* ध्रुव refp = नूतन AstVarRef{fl, varp, access};
        refp->selfPointer(VSelfPointerText{VSelfPointerText::This{}});
        पटिनिवत्त refp;
    }

    // A 'this->m_member-><method>(args...)' call on one member.  usePtr is false only for
    // __Vcg_inst, which is a value handle; the item members are borrowed pointers into the
    // instance node.  Numeric arguments are AstConst; the rest are C++ text that has no AST
    // form (see ctext).
    AstCMethodHard* itemCall(File_पङ्क्तिः* fl, AstVar* varp, VCMethod method,
                             ध्रुव std::vector<Ast_ग्रन्थिः_व्यञ्जकम्*>& args = {}, तार्किक usePtr = सच्च) {
        AstCMethodHard* ध्रुव callp = नूतन AstCMethodHard{fl, memberRef(fl, varp), method};
        अत्थाय (Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव argp : args) callp->addPinsp(argp);
        callp->usePtr(usePtr);
        callp->dtypeSetVoid();
        पटिनिवत्त callp;
    }

    // An unsigned integer argument.
    स्थिर AstConst* cnum(File_पङ्क्तिः* fl, uint32_t मानम्) { पटिनिवत्त नूतन AstConst{fl, मानम्}; }

    // A literal C++ argument with no AST equivalent: a 'const char*' string literal (an SV
    // string AstConst emits '"..."s', a std::string temporary the runtime cannot borrow), a
    // VlCovBinKind enum token, a constant selection-word initializer list, a VlFileLineDebug, or
    // a '__V' temporary declared by the enclosing AstCStmt.
    स्थिर AstCExpr* ctext(File_पङ्क्तिः* fl, ध्रुव std::string& text) {
        पटिनिवत्त नूतन AstCExpr{fl, text};
    }

    // A C++ string literal.  Escapes control characters as the emitter does elsewhere -- bin
    // names and filenames reach the generated code verbatim when --protect-ids is off, and an
    // SV escaped identifier may hold a quote or backslash.
    स्थिर std::string quoted(ध्रुव std::string& text) {
        पटिनिवत्त "\"" + V3OutFormatter::quoteNameControls(text) + "\"";
    }

    // A 'VlFileLineDebug' argument: where the runtime reports an error about fl's construct
    स्थिर AstCExpr* file_पङ्क्तिः_Debug(File_पङ्क्तिः* fl) {
        ध्रुव std::string filename
            = VIdProtect::protectIf(fl->filename(), v3Global.opt.protectIds());
        पटिनिवत्त ctext(fl, "VlFileLineDebug{" + quoted(filename) + ", "
                             + std::to_string(fl->lineno()) + "}");
    }

    // Check that an element of an array bin (bins b[] = {values/ranges}) is a two-state
    // constant value or range; false if not, after reporting it if 'report'.
    स्थिर तार्किक checkArrayBinElement(AstCoverBin* arrayBinp, Ast_ग्रन्थिः* rangep, तार्किक report = सच्च) {
        सचे (ध्रुव AstInsideRange* ध्रुव irp = VN_CAST(rangep, InsideRange)) {
            ध्रुव AstConst* ध्रुव minp = VN_CAST(irp->lhsp(), Const);
            ध्रुव AstConst* ध्रुव maxp = VN_CAST(irp->rhsp(), Const);
            सचे ((!minp && !VN_IS(irp->lhsp(), Unbounded))
                || (!maxp && !VN_IS(irp->rhsp(), Unbounded))) {
                सचे (report) {
                    arrayBinp->v3error("Non-constant expression in array bins range; "
                                       "range bounds must be constants (IEEE 1800-2023 19.5)");
                }
                पटिनिवत्त मिच्छा;
            }
            सचे ((minp && minp->अङ्कः().isFourState()) || (maxp && maxp->अङ्कः().isFourState())) {
                सचे (report) {
                    arrayBinp->v3error("Four-state (x/z) value in array bins range bound; "
                                       "range bounds must be two-state constants");
                }
                पटिनिवत्त मिच्छा;
            }
        } अञ्ञथा सचे (!VN_IS(rangep, Const)) {
            सचे (report) {
                arrayBinp->v3error("Non-constant expression in array bins value list; "
                                   "values must be constants (IEEE 1800-2023 19.5)");
            }
            पटिनिवत्त मिच्छा;
        }
        पटिनिवत्त सच्च;
    }

    // Individual equality targets of an array bin (bins b[] = {values/ranges}) of a real
    // coverpoint, in order; integral coverpoints generate array bins as runs (see arrayBinRuns).
    // An open-ended bound ('$', AstUnbounded) resolves to the coverpoint domain: '[lo:$]'
    // covers [lo:maxVal] and '[$:hi]' covers [0:hi].  One target is produced per value; ranges
    // whose resolved size would exceed --coverage-max-real-bins (e.g. an open '[lo:$]') are
    // unsupported -- emits COVERIGN, sets unsupportedOut, yields nothing.
    std::vector<Ast_ग्रन्थिः_व्यञ्जकम्*> extract_Array_मानानि(AstCoverBin* arrayBinp, Ast_ग्रन्थिः_व्यञ्जकम्* exprp,
                                                 तार्किक& unsupportedOut) {
        unsupportedOut = मिच्छा;
        ध्रुव पूर्णाङ्क width = exprp->width();
        ध्रुव uint64_t maxVal = (width >= 64) ? UINT64_MAX : ((1ULL << width) - 1);
        std::vector<Ast_ग्रन्थिः_व्यञ्जकम्*> मानानि;
        अत्थाय (Ast_ग्रन्थिः* rangep = arrayBinp->rangesp(); rangep; rangep = rangep->nextp()) {
            rangep = V3Const::constifyEdit(rangep);
            सचे (!checkArrayBinElement(arrayBinp, rangep)) पटिनिवत्त मानानि;
            सचे (AstInsideRange* ध्रुव irp = VN_CAST(rangep, InsideRange)) {
                ध्रुव तार्किक loUnb = VN_IS(irp->lhsp(), Unbounded);
                ध्रुव तार्किक hiUnb = VN_IS(irp->rhsp(), Unbounded);
                ध्रुव uint64_t lo = loUnb ? 0 : VN_AS(irp->lhsp(), Const)->toUQuad();
                ध्रुव uint64_t hi = hiUnb ? maxVal : VN_AS(irp->rhsp(), Const)->toUQuad();
                सचे (hi < lo) अनुवर्त;  // empty range contributes no bins
                // Guard against a '$'-bounded or otherwise huge range exploding the bin count.
                ध्रुव uint64_t span = hi - lo;  // == valueCount - 1 (no overflow: hi >= lo)
                सचे (span >= v3Global.opt.coverageMaxRealBins()
                    || मानानि.size() + span + 1 > v3Global.opt.coverageMaxRealBins()) {
                    arrayBinp->v3warn(COVERIGN,
                                      "Unsupported: array 'bins' of a real coverpoint "
                                      "covering more than "
                                          << v3Global.opt.coverageMaxRealBins() << " values; bin "
                                          << arrayBinp->prettyNameQ() << " ignored.\n"
                                          << arrayBinp->warnMore()
                                          << "... Suggest a larger --coverage-max-real-bins");
                    unsupportedOut = सच्च;
                    अत्थाय (Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव vp : मानानि) VL_DO_DANGLING(प्रक्षिप_Deletep(vp), vp);
                    मानानि.clear();
                    पटिनिवत्त मानानि;
                }
                अत्थाय (uint64_t v = lo; v <= hi; ++v)
                    मानानि.प्रक्षिप_back(नूतन AstConst{irp->fileline(), AstConst::Widthed_मानम्{}, width,
                                                  static_cast<uint32_t>(v)});
            } अञ्ञथा {
                मानानि.प्रक्षिप_back(VN_AS(rangep->clone_वृक्षः(मिच्छा), ग्रन्थिः_व्यञ्जकम्));
            }
        }
        पटिनिवत्त मानानि;
    }

    स्थिर पूर्णाङ्क runWidth(ध्रुव Ast_ग्रन्थिः_व्यञ्जकम्* exprp) { पटिनिवत्त exprp->width() + 1; }

    // Automatic bins partition the coverpoint domain in value order (IEEE 1800-2023 19.5.3): N
    // bins, capped at the number of values, each hold 2^width / N values, and the last bin also
    // holds the remainder.  False for an invalid declaration, already reported.
    तार्किक autoBinRuns(AstCoverBin* binp, Ast_ग्रन्थिः_व्यञ्जकम्* exprp, BinRuns& out) {
        ध्रुव uint32_t requested = autoBinsRequested(binp);
        सचे (!requested || !exprp->dtypep()->skipRefp()->isIntegralOrPacked()) पटिनिवत्त मिच्छा;
        ध्रुव पूर्णाङ्क width = exprp->width();
        ध्रुव पूर्णाङ्क arithmeticWidth = runWidth(exprp);
        ध्रुव uint32_t गणनम्
            = width < 32
                  ? static_cast<uint32_t>(std::min<uint64_t>(uint64_t{1} << width, requested))
                  : requested;
        BinRun run{binp, arithmeticWidth, गणनम्};
        V3_सङ्ख्या total{binp, arithmeticWidth};
        total.setBit(width, 1);
        run.m_stride.opDiv(total, V3_सङ्ख्या{binp, arithmeticWidth, गणनम्});
        ध्रुव Cross_मानम्_Range domain
            = cross_मानम्_Domain(binp, width, exprp->isSigned(), arithmeticWidth);
        run.m_lo = domain.lo;
        run.m_hi = domain.hi;
        out.runs.प्रक्षिप_back(std::move(run));
        out.गणनम् = गणनम्;
        पटिनिवत्त सच्च;
    }

    // The elements of an array bin (bins b[] = {values/ranges}), in order, as runs of
    // single-value bins.  A range holds the values of the coverpoint type it contains (IEEE
    // 1800-2023 19.5.7), while a singleton names a bin even without such a value.  Errors on a
    // non-constant element.  More than --coverage-max-bins bins (e.g. an open '[lo:$]' range over
    // a wide coverpoint) are unsupported -- emits COVERIGN, and sets unsupported.
    BinRuns arrayBinRuns(AstCoverBin* arrayBinp, Ast_ग्रन्थिः_व्यञ्जकम्* exprp) {
        BinRuns out;
        ध्रुव पूर्णाङ्क width = runWidth(exprp);
        अत्थाय (Ast_ग्रन्थिः* rangep = arrayBinp->rangesp(); rangep; rangep = rangep->nextp()) {
            rangep = V3Const::constifyEdit(rangep);
            सचे (!checkArrayBinElement(arrayBinp, rangep)) पटिनिवत्त out;
            ध्रुव AstInsideRange* ध्रुव irp = VN_CAST(rangep, InsideRange);
            Cross_मानम्_Range range{rangep, resolveWidth(rangep, exprp)};
            तार्किक empty = सच्च;
            सचे (!resolve_मानम्(rangep, exprp, सच्च, मिच्छा, range)) {
                rangep->v3warn(E_UNSUPPORTED, "Unsupported: non-integral value in a coverage bin "
                                              "of an integral coverpoint.");
            } अञ्ञथा {
                empty = crossRangeEmpty(range);
            }
            सचे (empty && irp) अनुवर्त;  // A range without values contributes no bins
            BinRun run{rangep, width, 1};
            run.m_empty = empty;
            uint64_t गणनम् = 1;
            सचे (!empty) {
                run.m_lo.opAssign(range.lo);
                run.m_hi.opAssign(range.hi);
                V3_सङ्ख्या span{rangep, width};
                span.opSub(run.m_hi, run.m_lo);
                // Wider spans exceed any limit
                गणनम् = span.mostSetBitP1() > 32 ? UINT64_MAX : span.toUQuad() + 1;
            }
            सचे (गणनम् > v3Global.opt.coverageMaxBins() - out.गणनम्) {
                arrayBinp->v3warn(COVERIGN, "Unsupported: array 'bins' covering more than "
                                                << v3Global.opt.coverageMaxBins()
                                                << " values (e.g. an open '[lo:$]' range over "
                                                   "a wide coverpoint); bin "
                                                << arrayBinp->prettyNameQ() << " ignored\n"
                                                << arrayBinp->warnMore()
                                                << "... Suggest a larger --coverage-max-bins");
                out.runs.clear();
                out.गणनम् = 0;
                out.unsupported = सच्च;
                पटिनिवत्त out;
            }
            run.m_गणनम् = static_cast<uint32_t>(गणनम्);
            out.गणनम् += run.m_गणनम्;
            out.runs.प्रक्षिप_back(std::move(run));
        }
        पटिनिवत्त out;
    }

    // The bins of a wildcard array (wildcard bins b[] = {...}): one for each coverpoint value
    // an element matches (IEEE 1800-2023 19.5.4, 19.5.7), in value order, and named by the value
    // (19.5.1), as runs of single-value bins.  Errors on a non-constant element.  More than
    // --coverage-max-bins values are unsupported -- emits COVERIGN, and sets unsupported, and for
    // an ignore or illegal array, single.  'report' false omits these diagnostics.
    स्थिर BinRuns wildcardBinRuns(AstCoverBin* arrayBinp, Ast_ग्रन्थिः_व्यञ्जकम्* exprp, तार्किक report) {
        BinRuns out;
        ध्रुव पूर्णाङ्क width = runWidth(exprp);
        ध्रुव V3_सङ्ख्या one{arrayBinp, width, 1};
        // Disjoint runs of the values, in value order, each value once
        std::vector<std::pair<V3_सङ्ख्या, V3_सङ्ख्या>> spans;
        uint64_t गणनम् = 0;  // Values of 'spans'
        अत्थाय (Ast_ग्रन्थिः* rangep = arrayBinp->rangesp(); rangep; rangep = rangep->nextp()) {
            rangep = V3Const::constifyEdit(rangep);
            सचे (!checkArrayBinElement(arrayBinp, rangep, report)) {
                out.unsupported = सच्च;
                पटिनिवत्त out;
            }
            Cross_मानम्_Range range{rangep, resolveWidth(rangep, exprp)};
            सचे (!resolve_मानम्(rangep, exprp, सच्च, सच्च, range)) {
                सचे (report) {
                    rangep->v3warn(E_UNSUPPORTED, "Unsupported: non-integral value in a "
                                                  "coverage bin of an integral coverpoint.");
                }
                अनुवर्त;
            }
            सचे (crossRangeEmpty(range)) अनुवर्त;
            std::vector<std::pair<V3_सङ्ख्या, V3_सङ्ख्या>> found;
            crossRangeRuns(range, v3Global.opt.coverageMaxBins(), found);
            अत्थाय (ध्रुव std::pair<V3_सङ्ख्या, V3_सङ्ख्या>& run : found) {
                // Coverpoint values, sign-extended in both widths
                spans.emplace_back(V3_सङ्ख्या{rangep, width, run.first},
                                   V3_सङ्ख्या{rangep, width, run.second});
            }
            std::sort(spans.begin(), spans.end(), [](ध्रुव auto& lhs, ध्रुव auto& rhs) {
                पटिनिवत्त cross_मानम्_Less(lhs.first, rhs.first);
            });
            std::vector<std::pair<V3_सङ्ख्या, V3_सङ्ख्या>> merged;
            अत्थाय (ध्रुव std::pair<V3_सङ्ख्या, V3_सङ्ख्या>& span : spans) {
                // Adjacent or overlapping values join a run; 'first - 1' cannot overflow
                V3_सङ्ख्या before{rangep, width};
                before.opSub(span.first, one);
                सचे (merged.empty() || cross_मानम्_Less(merged.back().second, before)) {
                    merged.प्रक्षिप_back(span);
                } अञ्ञथा सचे (cross_मानम्_Less(merged.back().second, span.second)) {
                    merged.back().second = span.second;
                }
            }
            गणनम् = 0;
            अत्थाय (ध्रुव std::pair<V3_सङ्ख्या, V3_सङ्ख्या>& span : merged) {
                V3_सङ्ख्या size{rangep, width};
                size.opSub(span.second, span.first);
                // Beyond 2^32 values exceed any limit
                गणनम् += size.mostSetBitP1() > 32 ? uint64_t{1} << 33 : size.toUQuad() + 1;
                सचे (गणनम् > v3Global.opt.coverageMaxBins()) भङ्ग;
            }
            spans = std::move(merged);
            सचे (गणनम् > v3Global.opt.coverageMaxBins()) {
                // An ignore or illegal array still excludes or checks its values, as one bin
                out.single = !arrayBinp->binsType().binIsNormal();
                सचे (report) {
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
                out.unsupported = सच्च;
                पटिनिवत्त out;
            }
        }
        अत्थाय (ध्रुव std::pair<V3_सङ्ख्या, V3_सङ्ख्या>& span : spans) {
            out.runs.emplace_back(arrayBinp, width, 0);
            BinRun& run = out.runs.back();
            run.m_lo = span.first;
            run.m_hi = span.second;
            // A bin for each value, named by it in the coverpoint's type
            V3_सङ्ख्या मानम् = span.first;
            याव (सच्च) {
                ध्रुव V3_सङ्ख्या typed{arrayBinp, exprp->width(), मानम्};
                out.मानानि.प्रक्षिप_back(exprp->isSigned() ? typed.toDecimalS() : typed.toDecimalU());
                ++run.m_गणनम्;
                सचे (मानम्.isCaseEq(span.second)) भङ्ग;
                V3_सङ्ख्या next{arrayBinp, width};
                मानम् = next.opAdd(मानम्, one);
            }
        }
        out.गणनम् = static_cast<uint32_t>(गणनम्);
        पटिनिवत्त out;
    }

    // The runs of an automatic bins declaration, or of an array bin of an integral coverpoint.
    // False for other bins, which do not generate as runs, including a wildcard array then one
    // bin (see BinRuns::single).
    तार्किक binRunsFor(AstCoverBin* binp, Ast_ग्रन्थिः_व्यञ्जकम्* exprp, BinRuns& out) {
        सचे (isAutoBins(binp)) {
            सचे (!autoBinRuns(binp, exprp, out)) out.unsupported = सच्च;
            पटिनिवत्त सच्च;
        }
        सचे (binp->isArray() && binp->isWildcard()) {
            सचे (!exprp->dtypep()->skipRefp()->isIntegralOrPacked()) {
                Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव falsep = wildcardTypeError(binp, exprp);
                VL_DO_DANGLING(प्रक्षिप_Deletep(falsep), falsep);
                out.unsupported = सच्च;
            } अञ्ञथा {
                out = wildcardBinRuns(binp, exprp, सच्च);
                सचे (out.single) {
                    binp->isArray(मिच्छा);  // Generates as one bin
                    पटिनिवत्त मिच्छा;
                }
            }
            पटिनिवत्त सच्च;
        }
        सचे (!binp->isArray() || binp->transp() || binp->isWildcard()
            || !exprp->dtypep()->skipRefp()->isIntegralOrPacked()) {
            पटिनिवत्त मिच्छा;
        }
        out = arrayBinRuns(binp, exprp);
        पटिनिवत्त सच्च;
    }

    // Emit a 'this->m_cp->addSingleNamer/addArrayNamer(...)' statement for one bin whose first
    // runtime bin index is 'declared'; or with 'valueNames', those naming each bin of an array
    Ast_ग्रन्थिः_Stmt* makeNamer(AstVar* cpVarp, AstCoverBin* binp, int64_t गणनम्, uint32_t declared,
                           ध्रुव std::vector<Ast_ग्रन्थिः_व्यञ्जकम्*>& मानानि = {},
                           ध्रुव std::vector<std::string>& मानम्_Names = {}) {
        File_पङ्क्तिः* ध्रुव fl = binp->fileline();
        CoverpointBins& bins = m_cpBins.at(cpVarp);
        ध्रुव uint32_t normal_गणनम्
            = binp->binsType().binIsNormal() ? static_cast<uint32_t>(गणनम् < 0 ? 1 : गणनम्) : 0;
        ध्रुव BinSpan span{bins.total, normal_गणनम्, declared};
        सचे (binp->binsType() == VCoverBinsType::BINS_AUTO_IMPLICIT) {
            bins.implicitAuto = span;  // Selected by bin, see implicitAutoBinSpan
        } अञ्ञथा {
            bins.spans.emplace(binp->name(), span);
        }
        bins.total += normal_गणनम्;
        अत्थाय (uint32_t i = 0; bins.crossed && i < normal_गणनम्; ++i) {
            bins.मानानि.प्रक्षिप_back({binp, मानानि.empty() ? nullptr : मानानि[i]});
        }
        // Under --protect-ids the filename and bin name flow into the coverage database
        // verbatim, so obfuscate them exactly as line/toggle coverage points are (whole-
        // unit filename, per-word bin name).  A no-op when --protect-ids is off.
        ध्रुव तार्किक prot = v3Global.opt.protectIds();
        ध्रुव तार्किक single = गणनम् < 0;
        सचे (!मानम्_Names.empty()) {
            // A bin of a wildcard array is named by its value (IEEE 1800-2023 19.5.1)
            Ast_ग्रन्थिः_Stmt* stmtsp = nullptr;
            अत्थाय (ध्रुव std::string& मानम् : मानम्_Names) {
                ध्रुव std::string name
                    = VIdProtect::protect_शब्दाः_If(binp->name(), prot) + "[" + मानम् + "]";
                stmtsp = Ast_ग्रन्थिः::addNext(
                    stmtsp,
                    itemCall(fl, cpVarp, VCMethod::COVERGROUP_ADD_SINGLE_NAMER,
                             {ctext(fl, binp->binsType().binSetEnum()), ctext(fl, quoted(name)),
                              ctext(fl, quoted(VIdProtect::protectIf(fl->filename(), prot))),
                              cnum(fl, static_cast<uint32_t>(fl->lineno())),
                              cnum(fl, static_cast<uint32_t>(fl->firstColumn()))})
                        ->makeStmt());
            }
            पटिनिवत्त stmtsp;
        }
        std::vector<Ast_ग्रन्थिः_व्यञ्जकम्*> args{ctext(fl, binp->binsType().binSetEnum())};
        सचे (!single) args.प्रक्षिप_back(cnum(fl, static_cast<uint32_t>(गणनम्)));  // value array bin
        args.प्रक्षिप_back(ctext(fl, quoted(VIdProtect::protect_शब्दाः_If(binp->name(), prot))));
        args.प्रक्षिप_back(ctext(fl, quoted(VIdProtect::protectIf(fl->filename(), prot))));
        args.प्रक्षिप_back(cnum(fl, static_cast<uint32_t>(fl->lineno())));
        args.प्रक्षिप_back(cnum(fl, static_cast<uint32_t>(fl->firstColumn())));
        पटिनिवत्त itemCall(fl, cpVarp,
                        single ? VCMethod::COVERGROUP_ADD_SINGLE_NAMER
                        : binp->binsType() == VCoverBinsType::BINS_AUTO_IMPLICIT
                            ? VCMethod::COVERGROUP_ADD_NUMBERED_NAMER
                            : VCMethod::COVERGROUP_ADD_ARRAY_NAMER,
                        args)
            ->makeStmt();
    }

    // Emit 'if (iff && cond) m_cp.incrementBin(idx);' (or recordHit, + illegal action) in sample()
    // Where a bin's hit is recorded in the runtime VlCoverpoint member.
    संरचना ConvBinTarget final {
        AstVar* cpVarp;  // the __Vcp_<coverpoint> member
        uint32_t idx;  // bin index within that coverpoint
        तार्किक isNormal;  // Normal -> incrementBin (count + cross hit list); else recordHit (count)
    };

    // Emit 'this->m_cp.incrementBin(idx);' (Normal) or '.recordHit(idx);'
    // (ignore/illegal/default).
    Ast_ग्रन्थिः_Stmt* makeRuntimeBinHit(File_पङ्क्तिः* fl, AstVar* cpVarp, Ast_ग्रन्थिः_व्यञ्जकम्* idxp,
                                   तार्किक isNormal) {
        पटिनिवत्त itemCall(fl, cpVarp,
                        isNormal ? VCMethod::COVERGROUP_INCREMENT_BIN
                                 : VCMethod::COVERGROUP_RECORD_HIT,
                        {idxp})
            ->makeStmt();
    }
    Ast_ग्रन्थिः_Stmt* makeRuntimeBinHit(File_पङ्क्तिः* fl, ध्रुव ConvBinTarget& tgt) {
        पटिनिवत्त makeRuntimeBinHit(fl, tgt.cpVarp, cnum(fl, static_cast<uint32_t>(tgt.idx)),
                                 tgt.isNormal);
    }

    // The condition under which a bin counts a sample: condp, and the bin's iff, and for a
    // Normal or default state bin, that the value is not excluded
    Ast_ग्रन्थिः_व्यञ्जकम्* binCondition(AstCoverBin* binp, AstVar* cpVarp, Ast_ग्रन्थिः_व्यञ्जकम्* condp) {
        File_पङ्क्तिः* ध्रुव fl = binp->fileline();
        सचे (binp->iffp()) condp = नूतन AstLogAnd{fl, binp->iffp()->clone_वृक्षः(मिच्छा), condp};
        ध्रुव auto excluded = m_excludedVars.find(cpVarp);
        सचे (excluded != m_excludedVars.end() && !binp->transp()
            && (binp->binsType().binIsNormal()
                || binp->binsType() == VCoverBinsType::BINS_DEFAULT)) {
            condp = नूतन AstLogAnd{
                fl, नूतन AstNot{fl, नूतन AstVarRef{fl, excluded->second, VAccess::READ}}, condp};
        }
        पटिनिवत्त condp;
    }

    शून्य emitConvHitIf(AstCoverpoint* coverpointp, AstCoverBin* binp, AstVar* cpVarp,
                       Ast_ग्रन्थिः_व्यञ्जकम्* idxp, Ast_ग्रन्थिः_व्यञ्जकम्* condp) {
        File_पङ्क्तिः* ध्रुव fl = binp->fileline();
        Ast_ग्रन्थिः* actionp = makeRuntimeBinHit(fl, cpVarp, idxp, binp->binsType().binIsNormal());
        सचे (binp->binsType() == VCoverBinsType::BINS_ILLEGAL) {
            actionp->addNext(makeIllegalBinAction(fl, "Illegal bin " + binp->prettyNameQ()
                                                          + " hit in coverpoint "
                                                          + coverpointp->prettyNameQ()));
        }
        UASSERT_OBJ(m_sampleFuncp, binp, "sample() CFunc not set for coverpoint");
        addSampleStmt(नूतन AstIf{fl, binCondition(binp, cpVarp, condp), actionp, nullptr});
    }

    // Emit the sample() code of sized array 'sized': count the value in its bins holding it when
    // enabled, as other bins are, and for default bins note in matchedp whether any holds it
    शून्य emitSizedSample(AstCoverpoint* coverpointp, AstCoverBin* binp, AstVar* cpVarp,
                         Ast_ग्रन्थिः_व्यञ्जकम्* exprp, uint32_t sized, AstVar* matchedp) {
        File_पङ्क्तिः* ध्रुव fl = binp->fileline();
        Ast_ग्रन्थिः_व्यञ्जकम्* enabledp = binCondition(binp, cpVarp, नूतन AstConst{fl, AstConst::BitTrue{}});
        UASSERT_OBJ(m_sampleFuncp, binp, "sample() CFunc not set for coverpoint");
        ध्रुव तार्किक illegal = binp->binsType() == VCoverBinsType::BINS_ILLEGAL;
        सचे (illegal) {
            // The illegal action reads the condition too, which is evaluated once, as the guard
            // may have side effects
            AstVar* ध्रुव varp
                = नूतन AstVar{fl, VVarType::BLOCKTEMP,
                             "__VcpEnabled_" + sanitizeGeneratedName(coverpointp->name()) + "_"
                                 + cvtToStr(sized),
                             binp->findBitDType()};
            varp->funcLocal(सच्च);
            m_sampleFuncp->addStmtsp(varp);
            addSampleStmt(नूतन AstAssign{fl, नूतन AstVarRef{fl, varp, VAccess::WRITE}, enabledp});
            enabledp = नूतन AstVarRef{fl, varp, VAccess::READ};
        }
        AstCMethodHard* ध्रुव callp
            = itemCall(fl, cpVarp,
                       exprp->isWide() ? VCMethod::COVERGROUP_SIZED_SAMPLE_W
                                       : VCMethod::COVERGROUP_SIZED_SAMPLE,
                       {cnum(fl, sized), exprp->clone_वृक्षः(मिच्छा), enabledp});
        callp->dtypeSetBit();
        सचे (illegal) {
            addSampleStmt(नूतन AstIf{fl, नूतन AstLogAnd{fl, callp, enabledp->clone_वृक्षः(मिच्छा)},
                                    makeIllegalBinAction(fl, "Illegal bin " + binp->prettyNameQ()
                                                                 + " hit in coverpoint "
                                                                 + coverpointp->prettyNameQ())});
        } अञ्ञथा सचे (matchedp && binp->binsType().binIsNormal()) {
            addSampleStmt(
                नूतन AstAssign{fl, नूतन AstVarRef{fl, matchedp, VAccess::WRITE},
                              नूतन AstOr{fl, नूतन AstVarRef{fl, matchedp, VAccess::READ}, callp}});
        } अञ्ञथा {
            addSampleStmt(callp->makeStmt());
        }
    }

    // A variable local to the constructor
    AstVar* constructorTemp(File_पङ्क्तिः* fl, ध्रुव string& name, Ast_ग्रन्थिः_DType* dtypep) {
        AstVar* ध्रुव varp = नूतन AstVar{fl, VVarType::BLOCKTEMP, name, dtypep};
        varp->funcLocal(सच्च);
        m_constructorp->addStmtsp(varp);
        पटिनिवत्त varp;
    }

    // Truncate or extend, as its signedness sets, a value to a type
    स्थिर Ast_ग्रन्थिः_व्यञ्जकम्* resize_मानम्(Ast_ग्रन्थिः_व्यञ्जकम्* valuep, Ast_ग्रन्थिः_DType* dtypep) {
        File_पङ्क्तिः* ध्रुव fl = valuep->fileline();
        सचे (valuep->width() > dtypep->width()) {
            valuep = नूतन AstSel{fl, valuep, 0, dtypep->width()};
        } अञ्ञथा सचे (valuep->width() < dtypep->width()) {
            valuep = valuep->isSigned()
                         ? static_cast<Ast_ग्रन्थिः_व्यञ्जकम्*>(नूतन AstExtendS{fl, valuep, dtypep->width()})
                         : नूतन AstExtend{fl, valuep, dtypep->width()};
        }
        valuep->dtypep(dtypep);
        पटिनिवत्त valuep;
    }

    // The runtime arguments of a count, which the constructor evaluates once into a temporary:
    // its value, a count wider than 64 bits saturated over any limit, and whether it is positive
    std::pair<Ast_ग्रन्थिः_व्यञ्जकम्*, Ast_ग्रन्थिः_व्यञ्जकम्*> new_गणनम्_Args(File_पङ्क्तिः* fl, ध्रुव string& name,
                                                       Ast_ग्रन्थिः_व्यञ्जकम्* exprp) {
        AstVar* ध्रुव countp = constructorTemp(fl, name, exprp->dtypep());
        m_constructorp->addStmtsp(
            नूतन AstAssign{fl, नूतन AstVarRef{fl, countp, VAccess::WRITE}, exprp->clone_वृक्षः(मिच्छा)});
        ध्रुव auto गणनम्_Ref = [&]() { पटिनिवत्त नूतन AstVarRef{fl, countp, VAccess::READ}; };
        AstConst* ध्रुव zerop = नूतन AstConst{fl, AstConst::DTyped{}, countp->dtypep()};
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव positivep
            = countp->isSigned() ? static_cast<Ast_ग्रन्थिः_व्यञ्जकम्*>(नूतन AstGtS{fl, गणनम्_Ref(), zerop})
                                 : नूतन AstNeq{fl, गणनम्_Ref(), zerop};
        Ast_ग्रन्थिः_व्यञ्जकम्* valuep = resize_मानम्(गणनम्_Ref(), countp->findUInt64DType());
        सचे (countp->width() > VL_QUADSIZE) {
            valuep = नूतन AstCond{
                fl,
                नूतन AstRedOr{
                    fl, नूतन AstSel{fl, गणनम्_Ref(), VL_QUADSIZE, countp->width() - VL_QUADSIZE}},
                नूतन AstConst{fl, AstConst::Unsized64{}, std::numeric_limits<uint64_t>::max()},
                valuep};
            valuep->dtypeSetUInt64();
        }
        पटिनिवत्त {valuep, positivep};
    }
    // The runtime arguments of a constant count
    स्थिर std::pair<Ast_ग्रन्थिः_व्यञ्जकम्*, Ast_ग्रन्थिः_व्यञ्जकम्*> new_गणनम्_Args(File_पङ्क्तिः* fl, uint64_t गणनम्) {
        पटिनिवत्त {नूतन AstConst{fl, AstConst::Unsized64{}, गणनम्},
                नूतन AstConst{fl, AstConst::BitTrue{}}};
    }

    // The values of a coverpoint, which its name denotes (IEEE 1800-2023 19.5.1.1), as runs in
    // value order: an enumerated type's values (6.19), else all values of its type.  Bounds
    // are at runWidth(), sign-extended like a CrossValueRange's.
    स्थिर std::vector<std::pair<V3_सङ्ख्या, V3_सङ्ख्या>> coverpoint_मानानि(Ast_ग्रन्थिः* nodep,
                                                                       Ast_ग्रन्थिः_व्यञ्जकम्* exprp) {
        ध्रुव पूर्णाङ्क width = runWidth(exprp);
        std::vector<std::pair<V3_सङ्ख्या, V3_सङ्ख्या>> runs;
        ध्रुव AstEnumDType* ध्रुव enump = VN_CAST(exprp->dtypep()->skipRefToEnump(), EnumDType);
        सचे (!enump) {
            ध्रुव Cross_मानम्_Range domain
                = cross_मानम्_Domain(nodep, exprp->width(), exprp->isSigned(), width);
            runs.emplace_back(domain.lo, domain.hi);
            पटिनिवत्त runs;
        }
        std::vector<V3_सङ्ख्या> मानानि;
        अत्थाय (ध्रुव AstEnumItem* itemp = enump->itemsp(); itemp;
             itemp = VN_AS(itemp->nextp(), EnumItem)) {
            ध्रुव V3_सङ्ख्या& अङ्कः = VN_AS(itemp->valuep(), Const)->अङ्कः();
            सचे (अङ्कः.isFourState()) अनुवर्त;  // Not a coverpoint value (19.5.7)
            मानानि.emplace_back(nodep, width);
            सचे (exprp->isSigned()) {
                मानानि.back().opExtendS(अङ्कः, अङ्कः.width());
            } अञ्ञथा {
                मानानि.back().opAssign(अङ्कः);
            }
        }
        std::sort(मानानि.begin(), मानानि.end(), cross_मानम्_Less);
        ध्रुव V3_सङ्ख्या one{nodep, width, 1};
        अत्थाय (ध्रुव V3_सङ्ख्या& मानम् : मानानि) {
            V3_सङ्ख्या next{nodep, width};
            सचे (!runs.empty() && next.opAdd(runs.back().second, one).isCaseEq(मानम्)) {
                runs.back().second = मानम्;
            } अञ्ञथा {
                runs.emplace_back(मानम्, मानम्);
            }
        }
        पटिनिवत्त runs;
    }

    // Emit the constructor code building the bins 'binp' whose values it computes: the values of
    // each element that are coverpoint values (IEEE 1800-2023 19.5.7), those a 'with' filter
    // keeps (19.5.1.1), then its bins
    शून्य generateConstructedBins(AstCoverpoint* coverpointp, AstCoverBin* binp, AstVar* cpVarp,
                                 Ast_ग्रन्थिः_व्यञ्जकम्* exprp) {
        File_पङ्क्तिः* ध्रुव fl = binp->fileline();
        ध्रुव string prefix
            = m_sizedNames.get(sanitizeGeneratedName(coverpointp->name() + "__" + binp->name()));
        // The count, and whether it is positive; one of a filter's scalar bin, or bin per value
        ध्रुव std::pair<Ast_ग्रन्थिः_व्यञ्जकम्*, Ast_ग्रन्थिः_व्यञ्जकम्*> गणनम्
            = binp->arraySizep() ? new_गणनम्_Args(fl, prefix + "_count", binp->arraySizep())
                                 : new_गणनम्_Args(fl, 1);
        AstCoverWith* ध्रुव withp = binWith(binp);
        सचे (withp && !binRangesp(binp)) {  // The coverpoint's name: all of its values
            अत्थाय (ध्रुव std::pair<V3_सङ्ख्या, V3_सङ्ख्या>& run : coverpoint_मानानि(binp, exprp)) {
                m_constructorp->addStmtsp(itemCall(fl, cpVarp,
                                                   exprp->isWide()
                                                       ? VCMethod::COVERGROUP_SIZED_RANGE_W
                                                       : VCMethod::COVERGROUP_SIZED_RANGE,
                                                   {new_मानम्_Const(fl, run.first, exprp),
                                                    new_मानम्_Const(fl, run.second, exprp)})
                                              ->makeStmt());
            }
        }
        uint32_t element = 0;
        अत्थाय (Ast_ग्रन्थिः* rangep = binRangesp(binp); rangep; rangep = rangep->nextp()) {
            सचे (VN_IS(rangep, Unbounded)) {  // A parameter of '$'
                binp->v3error("Bins value may not be '$', which may only bound a range "
                              "(IEEE 1800-2023 6.20.7)");
                अनुवर्त;
            }
            generateSizedElement(cpVarp, binp, rangep, exprp, prefix + "_" + cvtToStr(element++));
        }
        सचे (withp) generateWithFilter(binp, withp, cpVarp, exprp, prefix);
        std::vector<Ast_ग्रन्थिः_व्यञ्जकम्*> args{ctext(fl, binp->binsType().binSetEnum()), गणनम्.first,
                                       गणनम्.second};
        // A filter's bins have the limit it began with
        सचे (!withp) args.प्रक्षिप_back(cnum(fl, v3Global.opt.coverageMaxBins()));
        ध्रुव तार्किक prot = v3Global.opt.protectIds();
        args.प्रक्षिप_back(ctext(fl, quoted(VIdProtect::protect_शब्दाः_If(binp->name(), prot))));
        args.प्रक्षिप_back(ctext(fl, quoted(VIdProtect::protectIf(fl->filename(), prot))));
        args.प्रक्षिप_back(cnum(fl, static_cast<uint32_t>(fl->lineno())));
        args.प्रक्षिप_back(cnum(fl, static_cast<uint32_t>(fl->firstColumn())));
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
    शून्य generateWithFilter(AstCoverBin* binp, AstCoverWith* withp, AstVar* cpVarp,
                            Ast_ग्रन्थिः_व्यञ्जकम्* exprp, ध्रुव string& prefix) {
        File_पङ्क्तिः* ध्रुव fl = withp->fileline();
        ध्रुव तार्किक wide = exprp->isWide();
        ध्रुव string grouping = !binp->isArray()     ? "Single"
                                : binp->arraySizep() ? "Fixed"
                                                     : "Values";
        m_constructorp->addStmtsp(itemCall(fl, cpVarp, VCMethod::COVERGROUP_WITH_BEGIN,
                                           {ctext(fl, "VlCovBinGrouping::" + grouping),
                                            cnum(fl, v3Global.opt.coverageMaxBins())})
                                      ->makeStmt());
        // The candidates count in the coverpoint's width, and the filter reads each as 'item',
        // of the coverpoint's type, so that a filter changing 'item' cannot change the loop
        Ast_ग्रन्थिः_DType* ध्रुव मानम्_DTypep = exprp->dtypep();
        AstVar* ध्रुव valuep = constructorTemp(fl, prefix + "_value", मानम्_DTypep);
        AstVar* ध्रुव lastp = constructorTemp(fl, prefix + "_last", मानम्_DTypep);
        AstVar* ध्रुव firstp = constructorTemp(fl, prefix + "_first", मानम्_DTypep);
        AstVar* ध्रुव runp = constructorTemp(fl, prefix + "_run", binp->findBitDType());
        AstVar* ध्रुव morep = constructorTemp(fl, prefix + "_more", binp->findBitDType());
        AstVar* ध्रुव itemp = withp->itemp()->unlinkFrBack();
        itemp->name(prefix + "_item");
        m_constructorp->addStmtsp(itemp);
        ध्रुव auto ref = [&](AstVar* varp) { पटिनिवत्त नूतन AstVarRef{fl, varp, VAccess::READ}; };
        ध्रुव auto assign = [&](AstVar* varp, Ast_ग्रन्थिः_व्यञ्जकम्* rhsp) -> Ast_ग्रन्थिः* {
            पटिनिवत्त नूतन AstAssign{fl, नूतन AstVarRef{fl, varp, VAccess::WRITE}, rhsp};
        };
        ध्रुव auto flag = [&](AstVar* varp, तार्किक मानम्) {
            पटिनिवत्त assign(varp, मानम् ? नूतन AstConst{fl, AstConst::BitTrue{}}
                                      : नूतन AstConst{fl, AstConst::BitFalse{}});
        };
        ध्रुव auto bound = [&](VCMethod narrow, VCMethod wideMethod, AstVar* varp) -> Ast_ग्रन्थिः* {
            सचे (wide) {
                पटिनिवत्त itemCall(fl, cpVarp, wideMethod, {नूतन AstVarRef{fl, varp, VAccess::WRITE}})
                    ->makeStmt();
            }
            AstCMethodHard* ध्रुव callp = itemCall(fl, cpVarp, narrow);
            callp->dtypeSetUInt64();
            पटिनिवत्त assign(varp, resize_मानम्(callp, मानम्_DTypep));
        };
        ध्रुव auto keep = [&](Ast_ग्रन्थिः_व्यञ्जकम्* lop, Ast_ग्रन्थिः_व्यञ्जकम्* hip) {
            AstCMethodHard* ध्रुव callp = itemCall(
                fl, cpVarp, wide ? VCMethod::COVERGROUP_WITH_RUN_W : VCMethod::COVERGROUP_WITH_RUN,
                {lop, hip});
            callp->dtypeSetBit();
            पटिनिवत्त assign(morep, callp);
        };
        ध्रुव auto step = [&](तार्किक up) {
            AstConst* ध्रुव onep = नूतन AstConst{fl, AstConst::Widthed_मानम्{}, exprp->width(), 1};
            Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव stepp
                = up ? static_cast<Ast_ग्रन्थिः_व्यञ्जकम्*>(नूतन AstAdd{fl, ref(valuep), onep})
                     : नूतन AstSub{fl, ref(valuep), onep};
            stepp->dtypep(मानम्_DTypep);
            पटिनिवत्त stepp;
        };
        AstLoop* ध्रुव innerp = नूतन AstLoop{fl};
        innerp->addStmtsp(assign(itemp, ref(valuep)));
        innerp->addStmtsp(नूतन AstIf{
            fl, withp->filterp()->unlinkFrBack(),
            नूतन AstIf{fl, नूतन AstNot{fl, ref(runp)},
                      assign(firstp, ref(valuep))->addNext(flag(runp, सच्च))},
            नूतन AstIf{fl, ref(runp), keep(ref(firstp), step(मिच्छा))->addNext(flag(runp, मिच्छा))}});
        innerp->addStmtsp(नूतन AstLoopTest{
            fl, innerp, नूतन AstLogAnd{fl, ref(morep), नूतन AstNeq{fl, ref(valuep), ref(lastp)}}});
        innerp->addStmtsp(assign(valuep, step(सच्च)));
        AstLoop* ध्रुव outerp = नूतन AstLoop{fl};
        AstCMethodHard* ध्रुव nextp = itemCall(fl, cpVarp, VCMethod::COVERGROUP_WITH_NEXT);
        nextp->dtypeSetBit();
        outerp->addStmtsp(नूतन AstLoopTest{fl, outerp, nextp});
        outerp->addStmtsp(
            bound(VCMethod::COVERGROUP_WITH_LO, VCMethod::COVERGROUP_WITH_LO_W, valuep));
        outerp->addStmtsp(
            bound(VCMethod::COVERGROUP_WITH_HI, VCMethod::COVERGROUP_WITH_HI_W, lastp));
        outerp->addStmtsp(flag(runp, मिच्छा));
        outerp->addStmtsp(innerp);
        outerp->addStmtsp(नूतन AstIf{fl, ref(runp), keep(ref(firstp), ref(lastp))});
        m_constructorp->addStmtsp(flag(morep, सच्च));
        m_constructorp->addStmtsp(outerp);
    }

    // Emit 'sizedRange(lo, hi)' for the coverpoint values of an element of bins 'binp', whose
    // values the constructor computes: resolved now if constant, else when constructed by
    // clipping to the coverpoint's values.  A wildcard pattern's values give a range for each
    // run of them.  'prefix' names its temporaries.
    शून्य generateSizedElement(AstVar* cpVarp, ध्रुव AstCoverBin* binp, Ast_ग्रन्थिः* rangep,
                              Ast_ग्रन्थिः_व्यञ्जकम्* exprp, ध्रुव string& prefix) {
        File_पङ्क्तिः* ध्रुव fl = rangep->fileline();
        ध्रुव VCMethod method = exprp->isWide() ? VCMethod::COVERGROUP_SIZED_RANGE_W
                                                : VCMethod::COVERGROUP_SIZED_RANGE;
        ध्रुव AstInsideRange* ध्रुव irp = VN_CAST(rangep, InsideRange);
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव lowp = irp ? irp->lhsp() : VN_AS(rangep, ग्रन्थिः_व्यञ्जकम्);
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव highp = irp ? irp->rhsp() : nullptr;
        ध्रुव auto unbounded
            = [](ध्रुव Ast_ग्रन्थिः_व्यञ्जकम्* boundp) { पटिनिवत्त !boundp || VN_IS(boundp, Unbounded); };
        ध्रुव auto constant
            = [&](ध्रुव Ast_ग्रन्थिः_व्यञ्जकम्* boundp) { पटिनिवत्त unbounded(boundp) || VN_IS(boundp, Const); };
        ध्रुव auto integral = [&](ध्रुव Ast_ग्रन्थिः_व्यञ्जकम्* boundp) {
            पटिनिवत्त unbounded(boundp) || boundp->dtypep()->skipRefp()->isIntegralOrPacked();
        };
        सचे (constant(lowp) && constant(highp)) {
            ध्रुव auto fourState = [](ध्रुव Ast_ग्रन्थिः_व्यञ्जकम्* boundp) {
                ध्रुव AstConst* ध्रुव constp = VN_CAST(boundp, Const);
                पटिनिवत्त constp && constp->अङ्कः().isFourState();
            };
            सचे (irp && (fourState(lowp) || fourState(highp))) {
                rangep->v3error("Four-state (x/z) value in "
                                << (binp->isArray() ? "array bins" : "bin")
                                << " range bound; range bounds must be two-state constants");
                पटिनिवत्त;
            }
            Cross_मानम्_Range range{rangep, resolveWidth(rangep, exprp)};
            सचे (!resolve_मानम्(rangep, exprp, सच्च, binp->isWildcard(), range)) {
                rangep->v3warn(E_UNSUPPORTED, "Unsupported: non-integral value in a coverage bin "
                                              "of an integral coverpoint.");
            } अञ्ञथा सचे (!crossRangeEmpty(range)) {
                std::vector<std::pair<V3_सङ्ख्या, V3_सङ्ख्या>> runs{{range.lo, range.hi}};
                सचे (range.wildcard) {  // checkConstructedBins bounded the runs
                    runs.clear();
                    crossRangeRuns(range, v3Global.opt.coverageMaxBins(), runs);
                }
                अत्थाय (ध्रुव std::pair<V3_सङ्ख्या, V3_सङ्ख्या>& run : runs) {
                    m_constructorp->addStmtsp(itemCall(fl, cpVarp, method,
                                                       {new_मानम्_Const(fl, run.first, exprp),
                                                        new_मानम्_Const(fl, run.second, exprp)})
                                                  ->makeStmt());
                }
            }
            पटिनिवत्त;
        }
        सचे (!integral(lowp) || !integral(highp)) {
            rangep->v3warn(E_UNSUPPORTED, "Unsupported: non-integral value in a coverage bin "
                                          "of an integral coverpoint.");
            पटिनिवत्त;
        }
        // Compare values and the coverpoint's domain signed, in a width holding all of them
        ध्रुव auto boundWidth
            = [&](ध्रुव Ast_ग्रन्थिः_व्यञ्जकम्* boundp) { पटिनिवत्त unbounded(boundp) ? 0 : boundp->width(); };
        ध्रुव पूर्णाङ्क width = std::max({exprp->width(), boundWidth(lowp), boundWidth(highp)}) + 1;
        ध्रुव Cross_मानम्_Range domain
            = cross_मानम्_Domain(rangep, exprp->width(), exprp->isSigned(), width);
        AstVar* ध्रुव lop = constructorTemp(fl, prefix + "_lo", exprp->dtypep());
        lop->dtypeSetBitSized(width, VSigning::SIGNED);
        AstVar* ध्रुव hip = constructorTemp(fl, prefix + "_hi", lop->dtypep());
        ध्रुव auto bound = [&](Ast_ग्रन्थिः_व्यञ्जकम्* boundp, ध्रुव V3_सङ्ख्या& limit) -> Ast_ग्रन्थिः_व्यञ्जकम्* {
            सचे (unbounded(boundp)) {
                AstConst* ध्रुव limitp = नूतन AstConst{fl, limit};
                limitp->dtypeFrom(lop);
                पटिनिवत्त limitp;
            }
            Ast_ग्रन्थिः_व्यञ्जकम्* valuep = boundp->clone_वृक्षः(मिच्छा);
            // A value no wider than a signed coverpoint has its type (see crossRangeBound)
            सचे (exprp->isSigned() && boundp->width() <= exprp->width()) {
                valuep = resize_मानम्(valuep, exprp->dtypep());
            }
            पटिनिवत्त resize_मानम्(valuep, lop->dtypep());
        };
        ध्रुव auto ref = [&](AstVar* varp, VAccess access = VAccess::READ) {
            पटिनिवत्त नूतन AstVarRef{fl, varp, access};
        };
        m_constructorp->addStmtsp(
            नूतन AstAssign{fl, ref(lop, VAccess::WRITE), bound(lowp, domain.lo)});
        m_constructorp->addStmtsp(
            नूतन AstAssign{fl, ref(hip, VAccess::WRITE), irp ? bound(highp, domain.hi) : ref(lop)});
        AstConst* ध्रुव minp = नूतन AstConst{fl, domain.lo};
        AstConst* ध्रुव maxp = नूतन AstConst{fl, domain.hi};
        minp->dtypeFrom(lop);
        maxp->dtypeFrom(lop);
        AstCond* ध्रुव lowerp
            = नूतन AstCond{fl, नूतन AstLtS{fl, ref(lop), minp}, minp->clone_वृक्षः(मिच्छा), ref(lop)};
        AstCond* ध्रुव upperp
            = नूतन AstCond{fl, नूतन AstGtS{fl, ref(hip), maxp}, maxp->clone_वृक्षः(मिच्छा), ref(hip)};
        lowerp->dtypeFrom(lop);
        upperp->dtypeFrom(lop);
        m_constructorp->addStmtsp(नूतन AstAssign{fl, ref(lop, VAccess::WRITE), lowerp});
        m_constructorp->addStmtsp(नूतन AstAssign{fl, ref(hip, VAccess::WRITE), upperp});
        m_constructorp->addStmtsp(नूतन AstIf{fl, नूतन AstLteS{fl, ref(lop), ref(hip)},
                                            itemCall(fl, cpVarp, method,
                                                     {resize_मानम्(ref(lop), exprp->dtypep()),
                                                      resize_मानम्(ref(hip), exprp->dtypep())})
                                                ->makeStmt()});
    }

    // The runtime index of the bin of a run holding the coverpoint value, which is in the run:
    // declared + (value - lo) / stride, capped at the last bin, which holds any remainder.
    स्थिर Ast_ग्रन्थिः_व्यञ्जकम्* runBinIndex(File_पङ्क्तिः* fl, Ast_ग्रन्थिः_व्यञ्जकम्* exprp, ध्रुव BinRun& run) {
        सचे (run.m_गणनम् == 1) पटिनिवत्त cnum(fl, run.m_declared);
        ध्रुव पूर्णाङ्क width = exprp->width();
        // A run spans at most 2^width values, so offsets in it are unsigned width-bit numbers
        Ast_ग्रन्थिः_व्यञ्जकम्* indexp
            = नूतन AstSub{fl, exprp->clone_वृक्षः(मिच्छा), new_मानम्_Const(fl, run.m_lo, exprp)};
        indexp->dtypeSetBitSized(width, VSigning::UNSIGNED);
        V3_सङ्ख्या stride{fl, width, 0};
        stride.opAssign(run.m_stride);
        सचे (stride.गणनम्_Ones() != 1) {
            indexp = नूतन AstDiv{fl, indexp, नूतन AstConst{fl, stride}};
        } अञ्ञथा सचे (!stride.isEqOne()) {
            indexp = नूतन AstShiftR{fl, indexp, नूतन AstConst{fl, stride.mostSetBitP1() - 1}};
        }
        // Compare the run's values with those of count bins of stride values, without overflow
        ध्रुव पूर्णाङ्क extWidth = run.m_lo.width() + 1;
        V3_सङ्ख्या lo{fl, extWidth, 0};
        lo.opExtendS(run.m_lo, run.m_lo.width());
        V3_सङ्ख्या span{fl, extWidth, 0};
        span.opExtendS(run.m_hi, run.m_hi.width());
        span.opSub(V3_सङ्ख्या{span}, lo);
        V3_सङ्ख्या covered{fl, extWidth, 0};
        covered.opAssign(run.m_stride);
        covered.opMul(V3_सङ्ख्या{covered}, V3_सङ्ख्या{fl, extWidth, run.m_गणनम्});
        V3_सङ्ख्या hasRemainder{fl, 1, 0};
        सचे (!hasRemainder.opGte(span, covered).isEqZero()) {
            AstConst* ध्रुव lastp = नूतन AstConst{fl, V3_सङ्ख्या{fl, width, run.m_गणनम् - 1}};
            indexp = नूतन AstCond{fl, नूतन AstGt{fl, indexp->clone_वृक्षः(मिच्छा), lastp},
                                 lastp->clone_वृक्षः(मिच्छा), indexp};
        }
        सचे (width < VL_IDATASIZE) {
            indexp = नूतन AstExtend{fl, indexp, VL_IDATASIZE};
        } अञ्ञथा सचे (width > VL_IDATASIZE) {
            indexp = नूतन AstSel{fl, indexp, 0, VL_IDATASIZE};
        }
        पटिनिवत्त नूतन AstAdd{fl, cnum(fl, run.m_declared), indexp};
    }

    // Emit the sample() hit of a run of bins, whose code does not grow with its number of bins:
    //   if (iff && lo <= value && value <= hi) m_cp.incrementBin(<runBinIndex>);
    शून्य emitRunHit(AstCoverpoint* coverpointp, AstCoverBin* binp, AstVar* cpVarp,
                    Ast_ग्रन्थिः_व्यञ्जकम्* exprp, ध्रुव BinRun& run) {
        File_पङ्क्तिः* ध्रुव fl = binp->fileline();
        AstConst* ध्रुव lop = new_मानम्_Const(fl, run.m_lo, exprp);
        Ast_ग्रन्थिः_व्यञ्जकम्* condp = nullptr;
        सचे (run.m_lo.isCaseEq(run.m_hi)) {
            condp = नूतन AstEq{fl, exprp->clone_वृक्षः(मिच्छा), lop};
        } अञ्ञथा {
            AstConst* ध्रुव hip = new_मानम्_Const(fl, run.m_hi, exprp);
            condp = makeRangeCondition(fl, exprp, lop, hip);
            VL_DO_DANGLING(प्रक्षिप_Deletep(lop), lop);
            VL_DO_DANGLING(प्रक्षिप_Deletep(hip), hip);
        }
        emitConvHitIf(coverpointp, binp, cpVarp, runBinIndex(fl, exprp, run), condp);
    }

    // Emit a transition bin's hit action into sample():
    //   if (cond) { m_cp.incrementBin/recordHit(idx); [illegal: $error; $stop] }
    // Used by the transition generators so a completed sequence records into the runtime bin.
    शून्य addConvTransHitIf(AstCoverpoint* coverpointp, AstCoverBin* binp, ध्रुव ConvBinTarget& tgt,
                           Ast_ग्रन्थिः_व्यञ्जकम्* condp) {
        File_पङ्क्तिः* ध्रुव fl = binp->fileline();
        Ast_ग्रन्थिः* actionp = makeRuntimeBinHit(fl, tgt);
        सचे (binp->binsType() == VCoverBinsType::BINS_ILLEGAL) {
            actionp->addNext(makeIllegalBinAction(
                fl, "Illegal transition bin " + binp->prettyNameQ() + " hit in coverpoint "
                        + coverpointp->prettyNameQ()));
        }
        UASSERT_OBJ(m_sampleFuncp, binp, "sample() CFunc not set for transition bin");
        addSampleStmt(नूतन AstIf{fl, condp, actionp, nullptr});
    }

    // Route a coverpoint through a VlCoverpoint member: emit the member, its sample()
    // increments, the constructor configuration (init + namers), and registration.
    शून्य generateCoverpoint(AstCoverpoint* coverpointp, Ast_ग्रन्थिः_व्यञ्जकम्* exprp, पूर्णाङ्क at_Least_मानम्) {
        File_पङ्क्तिः* ध्रुव fl = coverpointp->fileline();
        ध्रुव तार्किक dynamic = m_runtimePoints.गणनम्(coverpointp);
        UINFO(4, "  Generating VlCoverpoint member: " << coverpointp->name());

        // Size the hit list to the gen-time max bin overlap (1 unless cross-fed with
        // overlapping ranges), so no cross hit is ever dropped and storage is minimal.
        ध्रुव तार्किक crossFed = m_crossedCpNames.गणनम्(coverpointp->name()) != 0;
        ध्रुव पूर्णाङ्क hitBound = computeHitListBound(coverpointp, exprp, crossFed);
        UINFO(6, "    Hit-list bound (max bin overlap) = " << hitBound);
        AstVar* ध्रुव cpVarp = नूतन AstVar{fl, VVarType::MEMBER, "__Vcp_" + coverpointp->name(),
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
        सचे (crossFed) {
            UASSERT_OBJ(m_sampleFuncp, coverpointp, "sample() CFunc not set for clearHitList");
            m_sampleFuncp->addStmtsp(
                itemCall(fl, cpVarp, VCMethod::COVERGROUP_CLEAR_HIT_LIST)->makeStmt());
        }
        सचे (dynamic && coverpointHasStateExclusions(coverpointp)) {
            AstVar* ध्रुव excludedp
                = नूतन AstVar{fl, VVarType::BLOCKTEMP,
                             "__VcpExcluded_" + sanitizeGeneratedName(coverpointp->name()),
                             coverpointp->findBitDType()};
            excludedp->funcLocal(सच्च);
            m_sampleFuncp->addStmtsp(excludedp);
            AstCMethodHard* ध्रुव callp
                = itemCall(fl, cpVarp,
                           exprp->isWide() ? VCMethod::COVERGROUP_मानम्_EXCLUDED_W
                                           : VCMethod::COVERGROUP_मानम्_EXCLUDED,
                           {exprp->clone_वृक्षः(मिच्छा)});
            callp->dtypeSetBit();
            addSampleStmt(नूतन AstAssign{fl, नूतन AstVarRef{fl, excludedp, VAccess::WRITE}, callp});
            m_excludedVars.emplace(cpVarp, excludedp);
        }

        // Walk bins (non-default, then default), assigning sequential indices that match the
        // namer append order; emit sample increments and collect namer statements.  Constructed
        // bins follow them all, placed when the coverpoint is constructed.
        std::vector<Ast_ग्रन्थिः_Stmt*> namerStmts;
        std::vector<AstCoverBin*> defaultBins;
        std::vector<AstCoverBin*> sizedBins;
        std::vector<std::tuple<AstCoverBin*, uint32_t, Ast_ग्रन्थिः_व्यञ्जकम्*>> metadata;
        std::vector<ध्रुव BinRun*> runMetadata;
        uint64_t idx = 0;  // Runtime index of the next bin; 32-bit once checked below
        अत्थाय (Ast_ग्रन्थिः* binp = coverpointp->binsp(); binp; binp = binp->nextp()) {
            AstCoverBin* ध्रुव cbinp = VN_AS(binp, CoverBin);
            ध्रुव पूर्णाङ्क errorsBefore = dynamic ? V3Error::error_गणनम्() : 0;
            सचे (cbinp->binsType() == VCoverBinsType::BINS_DEFAULT) {
                defaultBins.प्रक्षिप_back(cbinp);
                अनुवर्त;
            }
            सचे (isConstructedBins(cbinp)) {
                UASSERT_OBJ(dynamic, cbinp, "Constructed bins without value metadata");
                BinSpan span;
                span.sized = static_cast<int32_t>(sizedBins.size());
                m_cpBins.at(cpVarp).spans.emplace(cbinp->name(), span);
                sizedBins.प्रक्षिप_back(cbinp);
                अनुवर्त;
            }
            सचे (cbinp->transp()) {
                // Transition bin (incl. array transition 'bins t[] = (a=>b),(c=>d)' and
                // illegal_bins/ignore_bins transitions).  All sequences of one transition bin
                // share a bin name and merge in the coverage DB to a single point, so model
                // them as one runtime bin incremented by any matching sequence.  The sequence
                // matching is generated as a state machine, with the hit routed to this bin's
                // runtime slot.
                namerStmts.प्रक्षिप_back(makeNamer(cpVarp, cbinp, -1, static_cast<uint32_t>(idx)));
                ध्रुव ConvBinTarget tgt{cpVarp, static_cast<uint32_t>(idx),
                                        cbinp->binsType().binIsNormal()};
                अत्थाय (Ast_ग्रन्थिः* sp = cbinp->transp(); sp; sp = sp->nextp())
                    generateSingleTransitionCode(coverpointp, cbinp, exprp, tgt,
                                                 VN_AS(sp, CoverTransSet));
                सचे (dynamic && V3Error::error_गणनम्() == errorsBefore) {
                    metadata.emplace_back(cbinp, static_cast<uint32_t>(idx), nullptr);
                }
                ++idx;
                अनुवर्त;
            }
            BinRuns plan;
            सचे (binRunsFor(cbinp, exprp, plan)) {
                // Array elements and automatic bins generate as runs, so neither sample() nor
                // the constructor grows with their number of bins.
                सचे (plan.unsupported) {  // bin ignored or invalid; reserve no slot
                    m_droppedBins[coverpointp].प्रक्षिप_back(cbinp->name());
                    अनुवर्त;
                }
                CoverpointBins& bins = m_cpBins.at(cpVarp);
                ध्रुव uint32_t first_मानम् = bins.total;
                ध्रुव uint32_t firstDeclared = static_cast<uint32_t>(idx);
                namerStmts.प्रक्षिप_back(
                    makeNamer(cpVarp, cbinp, plan.गणनम्, firstDeclared, {}, plan.मानानि));
                अत्थाय (BinRun& run : plan.runs) {
                    run.m_declared = static_cast<uint32_t>(idx);
                    bins.runs.प्रक्षिप_back(std::move(run));
                    ध्रुव BinRun& stored = bins.runs.back();
                    सचे (bins.crossed && cbinp->binsType().binIsNormal()) {
                        ध्रुव uint32_t first = first_मानम् + stored.m_declared - firstDeclared;
                        अत्थाय (uint32_t element = 0; element < stored.m_गणनम्; ++element) {
                            bins.मानानि[first + element].runp = &stored;
                            bins.मानानि[first + element].element = element;
                        }
                    }
                    सचे (!stored.m_empty) {
                        emitRunHit(coverpointp, cbinp, cpVarp, exprp, stored);
                        सचे (dynamic && V3Error::error_गणनम्() == errorsBefore) {
                            runMetadata.प्रक्षिप_back(&stored);
                        }
                    }
                    idx += stored.m_गणनम्;
                }
                अनुवर्त;
            }
            सचे (cbinp->isArray()) {  // value array of a real coverpoint: b[0]..b[N-1]
                // Only integral coverpoints have runtime value metadata (m_runtimePoints)
                UASSERT_OBJ(!dynamic, cbinp, "Runtime value metadata for a real coverpoint");
                तार्किक unsupported = मिच्छा;
                std::vector<Ast_ग्रन्थिः_व्यञ्जकम्*> मानानि = extract_Array_मानानि(cbinp, exprp, unsupported);
                सचे (unsupported) {  // bin ignored (COVERIGN emitted); reserve no slot
                    m_droppedBins[coverpointp].प्रक्षिप_back(cbinp->name());
                    अनुवर्त;
                }
                namerStmts.प्रक्षिप_back(makeNamer(cpVarp, cbinp, static_cast<int64_t>(मानानि.size()),
                                               static_cast<uint32_t>(idx), मानानि));
                अत्थाय (Ast_ग्रन्थिः_व्यञ्जकम्* valuep : मानानि) {
                    // The cross selections of this covergroup still read the value.
                    m_detached_मानानि.प्रक्षिप_back(valuep);
                    emitConvHitIf(coverpointp, cbinp, cpVarp,
                                  cnum(cbinp->fileline(), static_cast<uint32_t>(idx)),
                                  build_मानम्_Condition(cbinp, exprp, valuep));
                    ++idx;
                }
            } अञ्ञथा {
                namerStmts.प्रक्षिप_back(makeNamer(cpVarp, cbinp, -1, static_cast<uint32_t>(idx)));
                // buildBinCondition is null for 'ignore_bins = default' (no ranges); the bin
                // still gets a reserved slot (recorded, never incremented).
                सचे (Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव condp = buildBinCondition(cbinp, exprp))
                    emitConvHitIf(coverpointp, cbinp, cpVarp,
                                  cnum(cbinp->fileline(), static_cast<uint32_t>(idx)), condp);
                सचे (dynamic && V3Error::error_गणनम्() == errorsBefore) {
                    metadata.emplace_back(cbinp, static_cast<uint32_t>(idx), nullptr);
                }
                ++idx;
            }
        }
        // A cross selecting an ignored bins declaration selects no bins
        अत्थाय (ध्रुव std::string& name : m_droppedBins[coverpointp]) {
            m_cpBins.at(cpVarp).spans.emplace(name, BinSpan{});
        }
        // A value a constructed bin holds is no default bin's; only sampling tells which do
        AstVar* sizedMatchedp = nullptr;
        सचे (!defaultBins.empty()
            && std::any_of(sizedBins.begin(), sizedBins.end(), [](ध्रुव AstCoverBin* binp) {
                   पटिनिवत्त binp->binsType().binIsNormal();
               })) {
            sizedMatchedp = नूतन AstVar{fl, VVarType::BLOCKTEMP,
                                       "__VcpSized_" + sanitizeGeneratedName(coverpointp->name()),
                                       coverpointp->findBitDType()};
            sizedMatchedp->funcLocal(सच्च);
            m_sampleFuncp->addStmtsp(sizedMatchedp);
            addSampleStmt(नूतन AstAssign{fl, नूतन AstVarRef{fl, sizedMatchedp, VAccess::WRITE},
                                        नूतन AstConst{fl, AstConst::BitFalse{}}});
        }
        अत्थाय (uint32_t sized = 0; sized < sizedBins.size(); ++sized) {
            emitSizedSample(coverpointp, sizedBins[sized], cpVarp, exprp, sized, sizedMatchedp);
        }
        अत्थाय (AstCoverBin* ध्रुव defBinp : defaultBins) {
            File_पङ्क्तिः* ध्रुव dfl = defBinp->fileline();
            namerStmts.प्रक्षिप_back(makeNamer(cpVarp, defBinp, -1, static_cast<uint32_t>(idx)));
            Ast_ग्रन्थिः_व्यञ्जकम्* condp = buildDefaultCondition(coverpointp, exprp, dfl);
            सचे (sizedMatchedp) {
                condp = नूतन AstLogAnd{
                    dfl, नूतन AstNot{dfl, नूतन AstVarRef{dfl, sizedMatchedp, VAccess::READ}}, condp};
            }
            emitConvHitIf(coverpointp, defBinp, cpVarp, cnum(dfl, static_cast<uint32_t>(idx)),
                          condp);
            ++idx;
        }
        सचे (idx > std::numeric_limits<uint32_t>::max()) {
            // The runtime indexes bins with 32 bits; stop before generating a model
            coverpointp->v3warn(E_UNSUPPORTED, "Unsupported: coverpoint with more than "
                                                   << std::numeric_limits<uint32_t>::max()
                                                   << " bins");
        }

        // Transition coverpoints track the previous sampled value; update it once at the end of
        // this coverpoint's sample() contribution (the prev var was created on demand by the
        // transition matching above).
        सचे (coverpointHasTransition(coverpointp)) {
            AstVar* ध्रुव prevVarp = VN_AS(coverpointp->user1p(), Var);
            addSampleStmt(
                नूतन AstAssign{coverpointp->fileline(),
                              नूतन AstVarRef{prevVarp->fileline(), prevVarp, VAccess::WRITE},
                              exprp->clone_वृक्षः(मिच्छा)});
        }

        // Constructor: init (allocates), namers, then registration (under --coverage).
        // Under --protect-ids the hierarchy and page string reach the coverage database
        // verbatim, so obfuscate them like line/toggle points (per-word hierarchy, whole-
        // unit page).  No-ops when --protect-ids is off.
        ध्रुव तार्किक prot = v3Global.opt.protectIds();
        ध्रुव std::string hier
            = VIdProtect::protect_शब्दाः_If(m_covergroupName + "." + coverpointp->name(), prot);
        m_constructorp->addStmtsp(
            itemCall(fl, cpVarp, VCMethod::COVERGROUP_INIT,
                     {ctext(fl, quoted(hier)), cnum(fl, static_cast<uint32_t>(at_Least_मानम्)),
                      cnum(fl, static_cast<uint32_t>(idx))})
                ->makeStmt());
        अत्थाय (Ast_ग्रन्थिः_Stmt* ध्रुव ns : namerStmts) m_constructorp->addStmtsp(ns);
        सचे (dynamic) {
            m_constructorp->addStmtsp(
                itemCall(fl, cpVarp, VCMethod::COVERGROUP_मानम्_TYPE,
                         {cnum(fl, exprp->width()), cnum(fl, exprp->isSigned())})
                    ->makeStmt());
            मानम्_Lists lists;
            अत्थाय (ध्रुव auto& entry : metadata) {
                collect_मानम्_Metadata(lists, exprp, std::get<0>(entry), std::get<1>(entry),
                                     std::get<2>(entry));
            }
            अत्थाय (ध्रुव BinRun* ध्रुव runp : runMetadata) collectRunMetadata(lists, exprp, *runp);
            emit_मानम्_List(fl, cpVarp, VCMethod::COVERGROUP_मानम्_RANGES, lists.m_ranges);
            emit_मानम्_List(fl, cpVarp, VCMethod::COVERGROUP_मानम्_RUNS, lists.m_runs);
            emit_मानम्_List(fl, cpVarp, VCMethod::COVERGROUP_मानम्_PATTERNS, lists.m_patterns);
            emit_मानम्_List(fl, cpVarp, VCMethod::COVERGROUP_मानम्_TRANSITIONS, lists.m_transitions);
            अत्थाय (AstCoverBin* ध्रुव binp : sizedBins) {
                generateConstructedBins(coverpointp, binp, cpVarp, exprp);
            }
            m_constructorp->addStmtsp(
                itemCall(fl, cpVarp, VCMethod::COVERGROUP_मानम्_FINALIZE)->makeStmt());
        }
        सचे (v3Global.opt.coverage()) {
            ध्रुव std::string page
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
    शून्य generate_Multi_मानम्_Transition_Code(AstCoverpoint* coverpointp, AstCoverBin* binp,
                                          Ast_ग्रन्थिः_व्यञ्जकम्* exprp, ध्रुव ConvBinTarget& tgt,
                                          ध्रुव std::vector<AstCoverTransItem*>& वस्तूनि) {
        UINFO(4, "    Generating multi-value transition state machine for: " << binp->name());
        UINFO(4, "      Sequence length: " << वस्तूनि.size() << " items");

        // Create state position variable
        AstVar* ध्रुव stateVarp = createSequenceStateVar(coverpointp, binp);

        // Build case statement with N cases (one for each state 0 to N-1)
        // State 0: Not started, looking for first item
        // State 1 to N-1: In progress, looking for next item

        AstCase* ध्रुव casep
            = नूतन AstCase{binp->fileline(), VCaseType::CT_CASE,
                          नूतन AstVarRef{stateVarp->fileline(), stateVarp, VAccess::READ}, nullptr};

        // Generate each case item in the switch statement
        अत्थाय (size_t state = 0; state < वस्तूनि.size(); ++state) {
            AstCaseItem* caseItemp = generateTransitionStateCase(coverpointp, binp, exprp, tgt,
                                                                 stateVarp, वस्तूनि, state);
            casep->addItemsp(caseItemp);
        }

        // Add default case (reset to state 0) to prevent CASEINCOMPLETE warnings,
        // since the state variable is wider than the number of valid states.
        AstCaseItem* ध्रुव defaultItemp = नूतन AstCaseItem{
            binp->fileline(), nullptr,
            नूतन AstAssign{binp->fileline(),
                          नूतन AstVarRef{binp->fileline(), stateVarp, VAccess::WRITE},
                          नूतन AstConst{binp->fileline(), AstConst::Widthed_मानम्{}, 8, 0}}};
        casep->addItemsp(defaultItemp);

        addSampleStmt(casep);
        UINFO(4, "      Successfully added multi-value transition state machine");
    }

    // Generate code for a single state in the transition state machine
    // Returns the case item for this state
    AstCaseItem* generateTransitionStateCase(AstCoverpoint* coverpointp, AstCoverBin* binp,
                                             Ast_ग्रन्थिः_व्यञ्जकम्* exprp, ध्रुव ConvBinTarget& tgt,
                                             AstVar* stateVarp,
                                             ध्रुव std::vector<AstCoverTransItem*>& वस्तूनि,
                                             size_t state) {
        File_पङ्क्तिः* ध्रुव fl = binp->fileline();

        // Build condition for current value matching expected item at this state
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव matchCondp = buildTransitionItemCondition(वस्तूनि[state], exprp);

        Ast_ग्रन्थिः_Stmt* matchActionp = nullptr;

        सचे (state == वस्तूनि.size() - 1) {
            // Last state: sequence complete!  Record the hit in the runtime VlCoverpoint.
            matchActionp = makeRuntimeBinHit(fl, tgt);

            // For illegal_bins, add error message
            सचे (binp->binsType() == VCoverBinsType::BINS_ILLEGAL) {
                ध्रुव string err_सन्देशः = "Illegal transition bin " + binp->prettyNameQ()
                                      + " hit in coverpoint " + coverpointp->prettyNameQ();
                matchActionp = matchActionp->addNext(makeIllegalBinAction(fl, err_सन्देशः));
            }

            // Reset state to 0
            matchActionp = matchActionp->addNext(
                नूतन AstAssign{fl, नूतन AstVarRef{fl, stateVarp, VAccess::WRITE},
                              नूतन AstConst{fl, AstConst::Widthed_मानम्{}, 8, 0}});
        } अञ्ञथा {
            // Intermediate state: advance to next state
            matchActionp = नूतन AstAssign{
                fl, नूतन AstVarRef{fl, stateVarp, VAccess::WRITE},
                नूतन AstConst{fl, AstConst::Widthed_मानम्{}, 8, static_cast<uint32_t>(state + 1)}};
        }

        // Build restart logic: check if current value matches first item
        // If so, restart sequence from state 1 (even if we're in middle of sequence)
        Ast_ग्रन्थिः_Stmt* noMatchActionp = nullptr;
        सचे (state > 0) {
            // Check if current value matches first item (restart condition)
            Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव restartCondp = buildTransitionItemCondition(वस्तूनि[0], exprp);

            UASSERT_OBJ(restartCondp, वस्तूनि[0],
                        "buildTransitionItemCondition returned nullptr for restart");

            // Restart to state 1
            Ast_ग्रन्थिः_Stmt* restartActionp
                = नूतन AstAssign{fl, नूतन AstVarRef{fl, stateVarp, VAccess::WRITE},
                                नूतन AstConst{fl, AstConst::Widthed_मानम्{}, 8, 1}};

            // Reset to state 0 (else branch)
            Ast_ग्रन्थिः_Stmt* resetActionp
                = नूतन AstAssign{fl, नूतन AstVarRef{fl, stateVarp, VAccess::WRITE},
                                नूतन AstConst{fl, AstConst::Widthed_मानम्{}, 8, 0}};

            noMatchActionp = नूतन AstIf{fl, restartCondp, restartActionp, resetActionp};
        }
        // For state 0, no action needed if no match (stay in state 0)

        // Combine into if-else
        Ast_ग्रन्थिः_Stmt* ध्रुव stmtp = नूतन AstIf{fl, matchCondp, matchActionp, noMatchActionp};

        // Create case item for this state value
        AstCaseItem* ध्रुव caseItemp = नूतन AstCaseItem{
            fl, नूतन AstConst{fl, AstConst::Widthed_मानम्{}, 8, static_cast<uint32_t>(state)},
            stmtp};

        पटिनिवत्त caseItemp;
    }

    // Create: $error(msg); $stop;  Used when an illegal bin is hit.
    Ast_ग्रन्थिः_Stmt* makeIllegalBinAction(File_पङ्क्तिः* fl, ध्रुव string& err_सन्देशः) {
        AstDisplay* ध्रुव errorp
            = नूतन AstDisplay{fl, VDisplayType::DT_ERROR, err_सन्देशः, nullptr, nullptr};
        errorp->fmtp()->timeunit(m_covergroupp->timeunit());
        static_cast<Ast_ग्रन्थिः*>(errorp)->addNext(नूतन AstStop{fl, सच्च});
        पटिनिवत्त errorp;
    }

    // Preserve the coverpoint's width and signedness after V3Width.
    स्थिर AstConst* new_मानम्_Const(File_पङ्क्तिः* fl, ध्रुव V3_सङ्ख्या& मानम्, ध्रुव Ast_ग्रन्थिः_व्यञ्जकम्* exprp) {
        V3_सङ्ख्या narrowed{fl, exprp->width(), 0};
        narrowed.opAssign(मानम्);
        AstConst* ध्रुव constp = नूतन AstConst{fl, narrowed};
        constp->dtypeFrom(exprp);
        पटिनिवत्त constp;
    }

    // A real copy of a real or integral range bound, for comparing with a real coverpoint.
    स्थिर AstConst* newRealConst(AstConst* constp) {
        सचे (constp->अङ्कः().isDouble()) पटिनिवत्त constp->clone_वृक्षः(मिच्छा);
        V3_सङ्ख्या real{&constp->अङ्कः(), 64};
        real.opIToRD(constp->अङ्कः(), constp->isSigned());
        पटिनिवत्त नूतन AstConst{constp->fileline(), real};
    }

    // Clone a constant node, widening to targetWidth if needed (zero-extend).
    // Used to ensure comparisons use matching widths after V3Width has run.
    स्थिर AstConst* widenConst(File_पङ्क्तिः* fl, AstConst* constp, पूर्णाङ्क targetWidth) {
        सचे (constp->width() == targetWidth) पटिनिवत्त constp->clone_वृक्षः(मिच्छा);
        V3_सङ्ख्या अङ्कः{fl, targetWidth, 0};
        अङ्कः.opAssign(constp->अङ्कः());
        पटिनिवत्त नूतन AstConst{fl, अङ्कः};
    }

    // Build a range condition: minp <= exprp <= maxp.
    // Uses signed comparisons if exprp is signed; omits trivially-true domain bounds.
    // All arguments are non-owning; clones exprp/minp/maxp as needed.
    Ast_ग्रन्थिः_व्यञ्जकम्* makeRangeCondition(File_पङ्क्तिः* fl, Ast_ग्रन्थिः_व्यञ्जकम्* exprp, Ast_ग्रन्थिः_व्यञ्जकम्* minp,
                                    Ast_ग्रन्थिः_व्यञ्जकम्* maxp) {
        ध्रुव पूर्णाङ्क व्यञ्जकम्_Width = exprp->widthMin();
        AstConst* ध्रुव minConstp = VN_AS(minp, Const);
        AstConst* ध्रुव maxConstp = VN_AS(maxp, Const);
        सचे (exprp->isDouble()) {
            // A real coverpoint has no finite domain bounds to omit.
            पटिनिवत्त नूतन AstAnd{fl,
                              नूतन AstGteD{fl, exprp->clone_वृक्षः(मिच्छा), newRealConst(minConstp)},
                              नूतन AstLteD{fl, exprp->clone_वृक्षः(मिच्छा), newRealConst(maxConstp)}};
        }
        // Widen constants to match expression width so post-V3Width nodes use correct macros
        AstConst* ध्रुव minWidep = widenConst(fl, minConstp, व्यञ्जकम्_Width);
        AstConst* ध्रुव maxWidep = widenConst(fl, maxConstp, व्यञ्जकम्_Width);
        V3_सङ्ख्या minimum{fl, व्यञ्जकम्_Width, 0};
        V3_सङ्ख्या maximum{fl, व्यञ्जकम्_Width, 0};
        maximum.setAllBits1();
        सचे (exprp->isSigned()) {
            minimum.setBit(व्यञ्जकम्_Width - 1, 1);
            maximum.setBit(व्यञ्जकम्_Width - 1, 0);
        }
        Ast_ग्रन्थिः_व्यञ्जकम्* lowerp = nullptr;
        Ast_ग्रन्थिः_व्यञ्जकम्* upperp = nullptr;
        सचे (minWidep->अङ्कः().isCaseEq(minimum)) {
            VL_DO_DANGLING(प्रक्षिप_Deletep(minWidep), minWidep);
        } अञ्ञथा {
            lowerp = exprp->isSigned() ? static_cast<Ast_ग्रन्थिः_व्यञ्जकम्*>(
                                             नूतन AstGteS{fl, exprp->clone_वृक्षः(मिच्छा), minWidep})
                                       : static_cast<Ast_ग्रन्थिः_व्यञ्जकम्*>(
                                             नूतन AstGte{fl, exprp->clone_वृक्षः(मिच्छा), minWidep});
        }
        सचे (maxWidep->अङ्कः().isCaseEq(maximum)) {
            VL_DO_DANGLING(प्रक्षिप_Deletep(maxWidep), maxWidep);
        } अञ्ञथा {
            upperp = exprp->isSigned() ? static_cast<Ast_ग्रन्थिः_व्यञ्जकम्*>(
                                             नूतन AstLteS{fl, exprp->clone_वृक्षः(मिच्छा), maxWidep})
                                       : static_cast<Ast_ग्रन्थिः_व्यञ्जकम्*>(
                                             नूतन AstLte{fl, exprp->clone_वृक्षः(मिच्छा), maxWidep});
        }
        सचे (lowerp && upperp) पटिनिवत्त नूतन AstAnd{fl, lowerp, upperp};
        सचे (lowerp) पटिनिवत्त lowerp;
        सचे (upperp) पटिनिवत्त upperp;
        पटिनिवत्त नूतन AstConst{fl, AstConst::BitTrue{}};
    }

    // Build a one-sided comparison for an open-ended bin range whose other bound is '$'.
    // '$' denotes the coverpoint domain extreme, so {[lo:$]} == (expr >= lo) and
    // {[$:hi]} == (expr <= hi).
    Ast_ग्रन्थिः_व्यञ्जकम्* makeOpenRangeCondition(File_पङ्क्तिः* fl, Ast_ग्रन्थिः_व्यञ्जकम्* exprp, AstConst* boundp,
                                        तार्किक isLowerBound) {
        AstConst* ध्रुव widep = widenConst(fl, boundp, exprp->widthMin());
        सचे (isLowerBound) {
            सचे (exprp->isSigned()) पटिनिवत्त नूतन AstGteS{fl, exprp->clone_वृक्षः(मिच्छा), widep};
            पटिनिवत्त नूतन AstGte{fl, exprp->clone_वृक्षः(मिच्छा), widep};
        }
        सचे (exprp->isSigned()) पटिनिवत्त नूतन AstLteS{fl, exprp->clone_वृक्षः(मिच्छा), widep};
        पटिनिवत्त नूतन AstLte{fl, exprp->clone_वृक्षः(मिच्छा), widep};
    }

    // Build condition for a single transition item.
    // Returns expression that checks if exprp matches the item's value/range list.
    // Overload for when the expression is a variable read -- creates and manages the VarRef
    // internally, so callers don't need to construct a temporary node.
    Ast_ग्रन्थिः_व्यञ्जकम्* buildTransitionItemCondition(AstCoverTransItem* itemp, AstVar* varp) {
        Ast_ग्रन्थिः_व्यञ्जकम्* varRefp = नूतन AstVarRef{varp->fileline(), varp, VAccess::READ};
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव condp = buildTransitionItemCondition(itemp, varRefp);
        VL_DO_DANGLING(प्रक्षिप_Deletep(varRefp), varRefp);
        पटिनिवत्त condp;
    }

    // Non-owning: exprp is cloned internally; caller retains ownership of exprp.
    Ast_ग्रन्थिः_व्यञ्जकम्* buildTransitionItemCondition(AstCoverTransItem* itemp, Ast_ग्रन्थिः_व्यञ्जकम्* exprp) {
        Ast_ग्रन्थिः_व्यञ्जकम्* condp = nullptr;

        अत्थाय (Ast_ग्रन्थिः* valp = itemp->valuesp(); valp; valp = valp->nextp()) {
            Ast_ग्रन्थिः_व्यञ्जकम्* singleCondp = nullptr;
            valp = V3Const::constifyEdit(valp);
            AstConst* ध्रुव constp = VN_CAST(valp, Const);
            सचे (!constp) {
                valp->v3error("Non-constant expression in transition bin; "
                              "values must be constants (IEEE 1800-2023 19.5)");
                पटिनिवत्त नूतन AstConst{valp->fileline(), AstConst::BitFalseErroring{}};
            }
            singleCondp
                = नूतन AstEq{constp->fileline(), exprp->clone_वृक्षः(मिच्छा), constp->clone_वृक्षः(मिच्छा)};
            सचे (condp) {
                condp = नूतन AstOr{itemp->fileline(), condp, singleCondp};
            } अञ्ञथा {
                condp = singleCondp;
            }
        }

        पटिनिवत्त condp;
    }

    // Generate code for a single transition sequence (used by both regular and array bins)
    शून्य generateSingleTransitionCode(AstCoverpoint* coverpointp, AstCoverBin* binp,
                                      Ast_ग्रन्थिः_व्यञ्जकम्* exprp, ध्रुव ConvBinTarget& tgt,
                                      AstCoverTransSet* transSetp) {
        UINFO(4, "      Generating code for transition sequence");

        // Get or create previous value variable
        AstVar* ध्रुव prevVarp = create_Prev_मानम्_Var(coverpointp, exprp);

        UASSERT_OBJ(
            transSetp, binp,
            "Transition bin has no transition set (transp() was checked before calling this)");

        // Get transition items (the sequence: item1 => item2 => item3)
        std::vector<AstCoverTransItem*> वस्तूनि;
        अत्थाय (Ast_ग्रन्थिः* itemp = transSetp->itemsp(); itemp; itemp = itemp->nextp())
            वस्तूनि.प्रक्षिप_back(VN_AS(itemp, CoverTransItem));

        सचे (वस्तूनि.empty()) {
            binp->v3error("Transition set without items");
            पटिनिवत्त;
        }

        सचे (वस्तूनि.size() == 1) {
            // Single item transition not valid (need at least 2 values for =>)
            binp->v3error("Transition requires at least two values");
            पटिनिवत्त;
        } अञ्ञथा सचे (वस्तूनि.size() == 2) {
            // Simple two-value transition: (val1 => val2)
            // Use optimized direct comparison (no state machine needed)
            Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव cond1p = buildTransitionItemCondition(वस्तूनि[0], prevVarp);
            Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव cond2p = buildTransitionItemCondition(वस्तूनि[1], exprp);

            // Combine: prev matches val1 AND current matches val2
            Ast_ग्रन्थिः_व्यञ्जकम्* fullCondp = नूतन AstAnd{binp->fileline(), cond1p, cond2p};

            addConvTransHitIf(coverpointp, binp, tgt, fullCondp);

            UINFO(4, "        Successfully added 2-value transition if statement");
        } अञ्ञथा {
            // Multi-value sequence (a => b => c => ...)
            // Use state machine to track position in sequence
            generate_Multi_मानम्_Transition_Code(coverpointp, binp, exprp, tgt, वस्तूनि);
        }
    }

    // Append a "{ VlCoverpoint* __Vcx_cps[] = {cp0, cp1, ...}; <call> }" statement.  The brace
    // and the temporary array stay literal text -- a CMethodHard is one call, not a block --
    // but callp itself carries the member, method and '->'.  Construction only: init() copies
    // the array into the cross, so sample() reads it from there and needs no array at all.
    AstCStmt* makeCrossCpsCall(File_पङ्क्तिः* fl, ध्रुव std::vector<AstVar*>& cpVars,
                               AstCMethodHard* callp) {
        AstCStmt* ध्रुव cs = नूतन AstCStmt{fl};
        cs->add("{ VlCoverpoint* __Vcx_cps[] = {");
        अत्थाय (size_t d = 0; d < cpVars.size(); ++d) {
            सचे (d != 0) cs->add(", ");
            cs->add(memberRef(fl, cpVars[d]));
        }
        cs->add("}; ");
        cs->add(callp);
        cs->add("; }");
        पटिनिवत्त cs;
    }

    // Assign the per-bin flags individually: one-bit SV results have integer C++ storage types,
    // which may narrow in a bool initializer list but convert implicitly in assignments.
    AstCStmt* makeCrossIffsCall(File_पङ्क्तिः* fl, ध्रुव std::vector<AstCoverCrossBin*>& bins,
                                AstCMethodHard* callp) {
        AstCStmt* ध्रुव cs = नूतन AstCStmt{fl};
        cs->add("{ bool __Vcx_iffs[" + cvtToStr(bins.size()) + "]; ");
        अत्थाय (size_t i = 0; i < bins.size(); ++i) {
            ध्रुव AstCoverCrossBin* ध्रुव binp = bins[i];
            cs->add("__Vcx_iffs[" + cvtToStr(i) + "] = ");
            cs->add(binp->iffp() ? binp->iffp()->clone_वृक्षः(मिच्छा)
                                 : नूतन AstConst{fl, AstConst::BitTrue{}});
            cs->add("; ");
        }
        cs->add(callp);
        cs->add("; }");
        पटिनिवत्त cs;
    }

    using CrossSelection = std::vector<uint64_t>;
    संरचना ResolvedCrossBin final {
        AstCoverCrossBin* binp;
        CrossSelection selection;
    };
    संरचना CrossLayout final {
        uint32_t tuples = 0;
        uint32_t autoBins = 0;
        uint64_t bin_शब्दाः = 0;
        तार्किक valid = सच्च;
        std::vector<ResolvedCrossBin> bins;
    };
    संरचना CrossSelectionContext final {
        AstCoverCross* crossp;  // Cross whose tuple space is being selected
        ध्रुव std::vector<AstVar*>& cpVars;  // Feeding coverpoints in dimension order
        ध्रुव std::map<std::string, uint32_t>& dimensions;  // Coverpoint name -> dimension
        uint32_t tuples;  // Size of the Cartesian product
        std::vector<uint32_t> strides;  // Flat-index stride per dimension
        तार्किक valid = सच्च;  // False if this explicit bin cannot be implemented
    };
    संरचना CrossBinsofTarget final {
        ध्रुव CoverpointBins* m_binsp = nullptr;  // Declared bins; null after a reported error
        uint32_t m_dimension = 0;  // Coverpoint index within the cross
        uint32_t m_first = 0;  // First declared normal bin in the selected span
        uint32_t m_गणनम् = 0;  // Number of declared normal bins in the selected span
        uint32_t m_declaredFirst = 0;  // First runtime bin index of the selected span
        uint32_t m_declaredEnd = UINT32_MAX;  // One past the last runtime bin index
        int32_t m_sized = -1;  // Index of the sized array holding the selected bins; or -1
    };
    संरचना Cross_मानम्_Range final {
        V3_सङ्ख्या lo;  // Inclusive lower bound, sign-extended to the comparison width
        V3_सङ्ख्या hi;  // Inclusive upper bound
        V3_सङ्ख्या pattern;  // Allowed bit values; all X for an ordinary interval
        तार्किक wildcard = मिच्छा;  // A wildcard singleton rather than an exact four-state value

        Cross_मानम्_Range(Ast_ग्रन्थिः* nodep, पूर्णाङ्क width)
            : lo{nodep, width}
            , hi{nodep, width}
            , pattern{nodep, width} {
            pattern.setAllBitsX();
        }
    };

    स्थिर std::vector<Ast_ग्रन्थिः*> cross_Bin_मानानि(ध्रुव Cross_Bin_मानानि& bin) {
        सचे (bin.valuep) पटिनिवत्त {bin.valuep};
        std::vector<Ast_ग्रन्थिः*> मानानि;
        सचे (bin.binp->transp()) {
            // IEEE 1800-2023 19.6.1: binsof uses the last value of each transition.
            अत्थाय (Ast_ग्रन्थिः* setp = bin.binp->transp(); setp; setp = setp->nextp()) {
                AstCoverTransItem* lastp = VN_AS(setp, CoverTransSet)->itemsp();
                याव (lastp->nextp()) lastp = VN_AS(lastp->nextp(), CoverTransItem);
                अत्थाय (Ast_ग्रन्थिः* valuep = lastp->valuesp(); valuep; valuep = valuep->nextp()) {
                    मानानि.प्रक्षिप_back(valuep);
                }
            }
        } अञ्ञथा {
            अत्थाय (Ast_ग्रन्थिः* valuep = bin.binp->rangesp(); valuep; valuep = valuep->nextp()) {
                मानानि.प्रक्षिप_back(valuep);
            }
        }
        पटिनिवत्त मानानि;
    }

    // The values of the element-th bin of a run, sign-extended to 'width'
    स्थिर Cross_मानम्_Range runBinRange(Ast_ग्रन्थिः* nodep, ध्रुव BinRun& run, uint32_t element,
                                       पूर्णाङ्क width) {
        ध्रुव पूर्णाङ्क runw = run.m_lo.width();
        V3_सङ्ख्या offset{nodep, runw};
        offset.opMul(run.m_stride, V3_सङ्ख्या{nodep, runw, element});
        V3_सङ्ख्या lo{nodep, runw};
        lo.opAdd(run.m_lo, offset);
        V3_सङ्ख्या hi = run.m_hi;
        सचे (element + 1 < run.m_गणनम्) {
            offset.opSub(run.m_stride, V3_सङ्ख्या{nodep, runw, 1});
            hi.opAdd(lo, offset);
        }
        Cross_मानम्_Range range{nodep, width};
        range.lo.opExtendS(lo, runw);
        range.hi.opExtendS(hi, runw);
        पटिनिवत्त range;
    }

    स्थिर पूर्णाङ्क crossRangeWidth(Ast_ग्रन्थिः* nodep) {
        सचे (ध्रुव AstInsideRange* ध्रुव rangep = VN_CAST(nodep, InsideRange)) {
            पटिनिवत्त std::max(rangep->lhsp()->width(), rangep->rhsp()->width());
        }
        पटिनिवत्त nodep->width();
    }

    स्थिर तार्किक cross_मानम्_Less(ध्रुव V3_सङ्ख्या& lhs, ध्रुव V3_सङ्ख्या& rhs) {
        V3_सङ्ख्या result{&lhs};
        पटिनिवत्त !result.opLtS(lhs, rhs).isEqZero();
    }

    // Round a real bound inward to the nearest coverpoint value (IEEE 1800-2023 19.5.7).  Sets
    // 'empty' if the domain has no value on the bound's side.
    स्थिर शून्य crossRealBound(ध्रुव AstConst* constp, Ast_ग्रन्थिः_व्यञ्जकम्* exprp, तार्किक upper,
                               ध्रुव Cross_मानम्_Range& domain, V3_सङ्ख्या& result, तार्किक& empty) {
        ध्रुव द्विगुण मानम् = constp->अङ्कः().toDouble();
        ध्रुव द्विगुण bound = upper ? std::floor(मानम्) : std::ceil(मानम्);
        // Powers of two are exact, so the integral bound compares exactly with the domain edges.
        ध्रुव द्विगुण limit
            = std::ldexp(1.0, exprp->isSigned() ? exprp->width() - 1 : exprp->width());
        ध्रुव द्विगुण minimum = exprp->isSigned() ? -limit : 0.0;
        सचे (std::isnan(bound) || (upper ? bound < minimum : bound >= limit)) {
            empty = सच्च;
            result = domain.lo;
        } अञ्ञथा सचे (bound < minimum) {
            result = domain.lo;
        } अञ्ञथा सचे (bound >= limit) {
            result = domain.hi;
        } अञ्ञथा {
            V3_सङ्ख्या real{&result, 64};
            real.setDouble(bound);
            result.opRToIRoundS(real);
        }
    }

    स्थिर तार्किक crossRangeBound(Ast_ग्रन्थिः* nodep, Ast_ग्रन्थिः_व्यञ्जकम्* exprp, तार्किक upper, तार्किक bin_मानम्,
                                ध्रुव Cross_मानम्_Range& domain, V3_सङ्ख्या& result, तार्किक& empty) {
        सचे (VN_IS(nodep, Unbounded)) {
            V3_सङ्ख्या limit{nodep, exprp->width()};
            सचे (upper) limit.setAllBits1();
            सचे (exprp->isSigned()) {
                limit.setBit(exprp->width() - 1, !upper);
                result.opExtendS(limit, limit.width());
            } अञ्ञथा {
                result.opAssign(limit);
            }
            पटिनिवत्त सच्च;
        }
        ध्रुव AstConst* ध्रुव constp = VN_CAST(nodep, Const);
        सचे (!constp) पटिनिवत्त मिच्छा;
        सचे (constp->अङ्कः().isDouble()) {
            crossRealBound(constp, exprp, upper, domain, result, empty);
            पटिनिवत्त सच्च;
        }
        सचे (constp->अङ्कः().isString()) पटिनिवत्त मिच्छा;
        सचे (bin_मानम् && exprp->isSigned() && constp->width() <= exprp->width()) {
            // Bin bit patterns use the coverpoint's effective type (IEEE 1800-2023 19.5.7).
            // Wider values and intersect filters retain their values for domain clipping.
            V3_सङ्ख्या मानम्{nodep, exprp->width()};
            सचे (constp->isSigned()) {
                मानम्.opExtendS(constp->अङ्कः(), constp->width());
            } अञ्ञथा {
                मानम्.opAssign(constp->अङ्कः());
            }
            result.opExtendS(मानम्, मानम्.width());
        } अञ्ञथा सचे (constp->isSigned()) {
            result.opExtendS(constp->अङ्कः(), constp->width());
        } अञ्ञथा {
            result.opAssign(constp->अङ्कः());
        }
        पटिनिवत्त सच्च;
    }

    स्थिर Cross_मानम्_Range cross_मानम्_Domain(Ast_ग्रन्थिः* nodep, पूर्णाङ्क मानम्_Width, तार्किक isSigned,
                                            पूर्णाङ्क width) {
        Cross_मानम्_Range domain{nodep, width};
        V3_सङ्ख्या lo{nodep, मानम्_Width};
        V3_सङ्ख्या hi{nodep, मानम्_Width};
        hi.setAllBits1();
        सचे (isSigned) {
            lo.setBit(मानम्_Width - 1, 1);
            hi.setBit(मानम्_Width - 1, 0);
            domain.lo.opExtendS(lo, मानम्_Width);
            domain.hi.opExtendS(hi, मानम्_Width);
        } अञ्ञथा {
            domain.lo.opAssign(lo);
            domain.hi.opAssign(hi);
        }
        पटिनिवत्त domain;
    }

    स्थिर शून्य intersectCrossRange(Cross_मानम्_Range& range, ध्रुव Cross_मानम्_Range& other) {
        सचे (cross_मानम्_Less(range.lo, other.lo)) range.lo = other.lo;
        सचे (cross_मानम्_Less(other.hi, range.hi)) range.hi = other.hi;
    }

    स्थिर तार्किक cross_मानम्_Range(Ast_ग्रन्थिः* nodep, Ast_ग्रन्थिः_व्यञ्जकम्* exprp, तार्किक bin_मानम्, तार्किक wildcard,
                                ध्रुव Cross_मानम्_Range& domain, Cross_मानम्_Range& range) {
        तार्किक empty = मिच्छा;
        ध्रुव AstConst* ध्रुव constp = VN_CAST(nodep, Const);
        सचे (ध्रुव AstInsideRange* ध्रुव rangep = VN_CAST(nodep, InsideRange)) {
            सचे (!crossRangeBound(rangep->lhsp(), exprp, मिच्छा, bin_मानम्, domain, range.lo, empty)
                || !crossRangeBound(rangep->rhsp(), exprp, सच्च, bin_मानम्, domain, range.hi, empty)
                || range.lo.isFourState() || range.hi.isFourState()) {
                पटिनिवत्त मिच्छा;
            }
        } अञ्ञथा सचे (constp && constp->अङ्कः().isDouble()) {
            // A real value participates only if integral; it has no wildcard bits.
            crossRealBound(constp, exprp, मिच्छा, domain, range.lo, empty);
            crossRealBound(constp, exprp, सच्च, domain, range.hi, empty);
        } अञ्ञथा {
            सचे (!crossRangeBound(nodep, exprp, मिच्छा, bin_मानम्, domain, range.lo, empty)) {
                पटिनिवत्त मिच्छा;
            }
            सचे (bin_मानम् && exprp->isSigned() && constp && !constp->isSigned()
                && constp->width() > exprp->width()) {
                तार्किक representable = सच्च;
                अत्थाय (पूर्णाङ्क bit = exprp->width(); bit < constp->width(); ++bit) {
                    representable &= !constp->अङ्कः().bitIs1(bit);
                }
                सचे (representable) {
                    V3_सङ्ख्या मानम्{nodep, exprp->width()};
                    मानम्.opAssign(constp->अङ्कः());
                    range.lo.opExtendS(मानम्, मानम्.width());
                }
            }
            range.hi = range.lo;
            range.wildcard = wildcard;
            सचे (wildcard) {
                range.pattern = range.lo;
                range.lo = domain.lo;
                range.hi = domain.hi;
                सचे (constp && constp->isSigned()) {
                    // Replicated X sign bits are correlated, not independent wildcards.
                    // The source domain preserves expansion-before-casting (19.5.7).
                    intersectCrossRange(
                        range, cross_मानम्_Domain(nodep, constp->width(), सच्च, domain.lo.width()));
                }
            }
        }
        सचे (empty) {
            range.lo = domain.hi;
            range.hi = domain.lo;
            पटिनिवत्त सच्च;
        }
        सचे (!range.lo.isFourState()) intersectCrossRange(range, domain);
        सचे (range.wildcard && exprp->isSigned()) {
            ध्रुव पूर्णाङ्क sign = exprp->width() - 1;
            सचे (constp && !constp->isSigned() && constp->width() > exprp->width()) {
                // Unsigned equality preserves every target bit pattern if discarded bits are zero.
                अत्थाय (पूर्णाङ्क bit = exprp->width(); bit < constp->width(); ++bit) {
                    सचे (constp->अङ्कः().bitIs1(bit)) {
                        range.lo = domain.hi;
                        range.hi = domain.lo;
                        पटिनिवत्त सच्च;
                    }
                }
                अत्थाय (पूर्णाङ्क bit = exprp->width(); bit < range.pattern.width(); ++bit) {
                    सचे (range.pattern.bitIsXZ(sign))
                        range.pattern.setBit(bit, 'x');
                    अञ्ञथा
                        range.pattern.setBit(bit, range.pattern.bitIs1(sign));
                }
                पटिनिवत्त सच्च;
            }
            // Discarded high bits constrain the target sign, rather than becoming don't-cares.
            अत्थाय (पूर्णाङ्क bit = exprp->width(); bit < range.pattern.width(); ++bit) {
                सचे (range.pattern.bitIsXZ(bit)) अनुवर्त;
                ध्रुव तार्किक मानम् = range.pattern.bitIs1(bit);
                सचे (!range.pattern.bitIsXZ(sign) && range.pattern.bitIs1(sign) != मानम्) {
                    range.lo = domain.hi;
                    range.hi = domain.lo;
                    भङ्ग;
                }
                range.pattern.setBit(sign, मानम्);
            }
        }
        पटिनिवत्त सच्च;
    }

    गणना CrossRangeState : uint8_t {
        CROSS_INSIDE_BOUNDS = 0,  // Prefix is strictly inside the interval
        CROSS_AT_LOWER = 1,
        CROSS_AT_UPPER = 2,
        CROSS_AT_BOUNDS = CROSS_AT_LOWER | CROSS_AT_UPPER,
        CROSS_NO_MATCH = 4  // Prefix cannot match the interval/pattern
    };

    स्थिर CrossRangeState crossRangeStep(ध्रुव Cross_मानम्_Range& range, CrossRangeState state,
                                          पूर्णाङ्क bit, पूर्णाङ्क मानम्) {
        सचे (state == CROSS_NO_MATCH) पटिनिवत्त CROSS_NO_MATCH;
        // Flipping the sign bit makes signed order lexicographic.
        ध्रुव तार्किक sign = bit == range.pattern.width() - 1;
        सचे (!range.pattern.bitIsXZ(bit) && मानम् != (range.pattern.bitIs1(bit) ^ sign)) {
            पटिनिवत्त CROSS_NO_MATCH;
        }
        ध्रुव पूर्णाङ्क नीचः = range.lo.bitIs1(bit) ^ sign;
        ध्रुव पूर्णाङ्क उच्चः = range.hi.bitIs1(bit) ^ sign;
        सचे (((state & CROSS_AT_LOWER) && मानम् < नीचः)
            || ((state & CROSS_AT_UPPER) && मानम् > उच्चः)) {
            पटिनिवत्त CROSS_NO_MATCH;
        }
        पटिनिवत्त static_cast<CrossRangeState>(
            ((state & CROSS_AT_LOWER) && मानम् == नीचः ? CROSS_AT_LOWER : CROSS_INSIDE_BOUNDS)
            | ((state & CROSS_AT_UPPER) && मानम् == उच्चः ? CROSS_AT_UPPER : CROSS_INSIDE_BOUNDS));
    }

    स्थिर तार्किक crossWildcardIntersects(ध्रुव Cross_मानम्_Range& range) {
        unsigned states = 1U << CROSS_AT_BOUNDS;
        अत्थाय (पूर्णाङ्क bit = range.pattern.width() - 1; bit >= 0 && states; --bit) {
            unsigned next = 0;
            अत्थाय (ध्रुव CrossRangeState state :
                 {CROSS_INSIDE_BOUNDS, CROSS_AT_LOWER, CROSS_AT_UPPER, CROSS_AT_BOUNDS}) {
                सचे (!(states & (1U << state))) अनुवर्त;
                अत्थाय (पूर्णाङ्क मानम् = 0; मानम् < 2; ++मानम्) {
                    ध्रुव CrossRangeState equal = crossRangeStep(range, state, bit, मानम्);
                    सचे (equal != CROSS_NO_MATCH) next |= 1U << equal;
                }
            }
            states = next;
        }
        पटिनिवत्त states != 0;
    }

    // The least value at least 'from' with the non-x bits of 'pattern', in unsigned order; false
    // if none.  A bit scan, which does not enumerate the pattern's x bits.
    स्थिर तार्किक next_Pattern_मानम्(ध्रुव V3_सङ्ख्या& pattern, ध्रुव V3_सङ्ख्या& from, V3_सङ्ख्या& result) {
        result.opAssign(from);
        पूर्णाङ्क carry = -1;  // Lowest x bit above the bit scanned where 'from' has a 0
        अत्थाय (पूर्णाङ्क bit = from.width() - 1; bit >= 0; --bit) {
            सचे (pattern.bitIsXZ(bit)) {
                सचे (!from.bitIs1(bit)) carry = bit;
                अनुवर्त;
            }
            सचे (pattern.bitIs1(bit) == from.bitIs1(bit)) अनुवर्त;
            सचे (from.bitIs1(bit)) {  // Only a greater prefix, an x bit above raised, can match
                सचे (carry < 0) पटिनिवत्त मिच्छा;
                bit = carry;
            }
            // Then the least such value: the pattern's bits, with x bits of zero
            result.setBit(bit, 1);
            याव (--bit >= 0) result.setBit(bit, pattern.bitIs1(bit));
            पटिनिवत्त सच्च;
        }
        पटिनिवत्त सच्च;
    }

    // Append to 'runs' the maximal runs of consecutive values of 'range' that its pattern
    // matches, in value order, until there are more than 'limit'.  A match continues through
    // the pattern's trailing x bits only, which the next value's carry leaves.
    स्थिर शून्य crossRangeRuns(ध्रुव Cross_मानम्_Range& range, size_t limit,
                               std::vector<std::pair<V3_सङ्ख्या, V3_सङ्ख्या>>& runs) {
        // Values order signed, which is the unsigned order of values with the sign bit flipped
        ध्रुव पूर्णाङ्क sign = range.lo.width() - 1;
        ध्रुव auto flipped = [sign](V3_सङ्ख्या मानम्) {
            सचे (!मानम्.bitIsXZ(sign)) मानम्.setBit(sign, !मानम्.bitIs1(sign));
            पटिनिवत्त मानम्;
        };
        ध्रुव V3_सङ्ख्या pattern = flipped(range.pattern);
        ध्रुव V3_सङ्ख्या hi = flipped(range.hi);
        ध्रुव V3_सङ्ख्या one{&hi, hi.width(), 1};
        V3_सङ्ख्या trailing{&hi, hi.width()};
        अत्थाय (पूर्णाङ्क bit = 0; bit <= sign && pattern.bitIsXZ(bit); ++bit) trailing.setBit(bit, 1);
        V3_सङ्ख्या from = flipped(range.lo);
        V3_सङ्ख्या first{&hi, hi.width()};
        V3_सङ्ख्या last{&hi, hi.width()};
        V3_सङ्ख्या less{&hi};
        याव (runs.size() <= limit && next_Pattern_मानम्(pattern, from, first)
               && less.opLt(hi, first).isEqZero()) {
            last.opOr(first, trailing);
            सचे (!less.opLt(hi, last).isEqZero()) last = hi;
            runs.emplace_back(flipped(first), flipped(last));
            सचे (last.isCaseEq(hi)) भङ्ग;
            from.opAdd(last, one);
        }
    }

    // True if no coverpoint value participates in a resolved value or range.  A value with x
    // or z bits participates only as a wildcard pattern (IEEE 1800-2023 19.5.7).
    स्थिर तार्किक crossRangeEmpty(ध्रुव Cross_मानम्_Range& range) {
        सचे (range.lo.isFourState()) पटिनिवत्त सच्च;
        पटिनिवत्त cross_मानम्_Less(range.hi, range.lo)
               || (range.wildcard && !crossWildcardIntersects(range));
    }

    // Comparison width that holds both the coverpoint's and a bin or intersect value's range.
    स्थिर पूर्णाङ्क resolveWidth(Ast_ग्रन्थिः* nodep, ध्रुव Ast_ग्रन्थिः_व्यञ्जकम्* exprp) {
        पटिनिवत्त std::max(exprp->width(), crossRangeWidth(nodep)) + 1;
    }

    // Resolve a bin or intersect value to coverpoint values (IEEE 1800-2023 19.5.7), in the
    // width 'range' was created with.  False if the value is not a constant integral or real.
    स्थिर तार्किक resolve_मानम्(Ast_ग्रन्थिः* nodep, Ast_ग्रन्थिः_व्यञ्जकम्* exprp, तार्किक bin_मानम्, तार्किक wildcard,
                             Cross_मानम्_Range& range) {
        ध्रुव Cross_मानम्_Range domain
            = cross_मानम्_Domain(nodep, exprp->width(), exprp->isSigned(), range.lo.width());
        पटिनिवत्त cross_मानम्_Range(nodep, exprp, bin_मानम्, wildcard, domain, range);
    }

    स्थिर तार्किक crossRangesIntersect(ध्रुव Cross_मानम्_Range& bin, ध्रुव Cross_मानम्_Range& filter) {
        // Values with x or z bits do not participate, even in an identical filter.
        सचे (bin.lo.isFourState() || filter.lo.isFourState()) पटिनिवत्त मिच्छा;
        Cross_मानम्_Range match = bin;
        intersectCrossRange(match, filter);
        पटिनिवत्त !cross_मानम्_Less(match.hi, match.lo)
               && (!bin.wildcard || crossWildcardIntersects(match));
    }

    स्थिर शून्य unsupportedCrossRange(AstCoverBinsof* selectp, तार्किक& valid) {
        selectp->v3warn(COVERIGN, "Unsupported: non-constant or non-integral 'intersect' value, "
                                  "or four-state range bound.");
        valid = मिच्छा;
    }

    स्थिर तार्किक cross_मानम्_Matches_Filters(AstCoverBinsof* selectp, Ast_ग्रन्थिः* valuep,
                                         Ast_ग्रन्थिः_व्यञ्जकम्* exprp, ध्रुव AstCoverBin* binp,
                                         ध्रुव Cross_मानम्_Range& domain,
                                         ध्रुव std::vector<Cross_मानम्_Range>& filters,
                                         तार्किक& valid) {
        Cross_मानम्_Range range{valuep, domain.lo.width()};
        सचे (!cross_मानम्_Range(valuep, exprp, सच्च, binp->isWildcard(), domain, range)) {
            unsupportedCrossRange(selectp, valid);
            पटिनिवत्त मिच्छा;
        }
        अत्थाय (ध्रुव Cross_मानम्_Range& filter : filters) {
            सचे (crossRangesIntersect(range, filter)) पटिनिवत्त सच्च;
        }
        पटिनिवत्त मिच्छा;
    }

    std::vector<तार्किक> selectCoverpointBins(AstCoverBinsof* selectp, ध्रुव CoverpointBins& bins,
                                           uint32_t first, uint32_t गणनम्, तार्किक& valid) {
        सचे (selectp->rangesp() && !bins.exprp->dtypep()->skipRefp()->isIntegralOrPacked()) {
            unsupportedCrossRange(selectp, valid);
            पटिनिवत्त {};
        }
        std::vector<तार्किक> selected(bins.total, मिच्छा);
        std::vector<std::vector<Ast_ग्रन्थिः*>> मानानि;
        पूर्णाङ्क width = bins.exprp->width();
        अत्थाय (Ast_ग्रन्थिः* rangep = selectp->rangesp(); rangep; rangep = rangep->nextp()) {
            width = std::max(width, crossRangeWidth(rangep));
        }
        सचे (selectp->rangesp()) {
            मानानि.reserve(गणनम्);
            अत्थाय (uint32_t i = first; i < first + गणनम्; ++i) {
                // A run's values are in the coverpoint type, whose width 'width' covers
                मानानि.प्रक्षिप_back(bins.मानानि[i].runp ? std::vector<Ast_ग्रन्थिः*>{}
                                                     : cross_Bin_मानानि(bins.मानानि[i]));
                अत्थाय (Ast_ग्रन्थिः* ध्रुव valuep : मानानि.back()) {
                    width = std::max(width, crossRangeWidth(valuep));
                }
            }
        }
        // One extra bit preserves both unsigned maxima and negative signed bounds.
        ++width;
        ध्रुव Cross_मानम्_Range domain
            = cross_मानम्_Domain(selectp, bins.exprp->width(), bins.exprp->isSigned(), width);
        std::vector<Cross_मानम्_Range> filters;
        अत्थाय (Ast_ग्रन्थिः* rangep = selectp->rangesp(); rangep; rangep = rangep->nextp()) {
            Cross_मानम्_Range filter{rangep, width};
            सचे (!cross_मानम्_Range(rangep, bins.exprp, मिच्छा, मिच्छा, domain, filter)) {
                unsupportedCrossRange(selectp, valid);
                पटिनिवत्त {};
            }
            filters.प्रक्षिप_back(std::move(filter));
        }
        अत्थाय (uint32_t i = first; i < first + गणनम्; ++i) {
            सचे (!selectp->rangesp()) {
                selected[i] = सच्च;
                अनुवर्त;
            }
            सचे (ध्रुव BinRun* ध्रुव runp = bins.मानानि[i].runp) {
                ध्रुव Cross_मानम्_Range range
                    = runBinRange(selectp, *runp, bins.मानानि[i].element, width);
                selected[i] = std::any_of(filters.begin(), filters.end(),
                                          [&](ध्रुव Cross_मानम्_Range& filter) {
                                              पटिनिवत्त crossRangesIntersect(range, filter);
                                          });
                अनुवर्त;
            }
            अत्थाय (Ast_ग्रन्थिः* ध्रुव valuep : मानानि[i - first]) {
                selected[i] = cross_मानम्_Matches_Filters(
                    selectp, valuep, bins.exprp, bins.मानानि[i].binp, domain, filters, valid);
                सचे (!valid) पटिनिवत्त {};
                सचे (selected[i]) भङ्ग;
            }
        }
        सचे (selectp->isNegated()) {
            अत्थाय (uint32_t i = 0; i < bins.total; ++i) selected[i] = !selected[i];
        }
        पटिनिवत्त selected;
    }

    // Constructor-time value metadata of one coverpoint, as C++ list entries
    संरचना मानम्_Lists final {
        std::vector<std::string> m_ranges;  // Bin, then low and high words
        std::vector<std::string> m_runs;  // First bin, count, then low, span, and high words
        std::vector<std::string> m_patterns;  // Bin, then value, mask, low, and high words
        std::vector<std::string> m_transitions;  // Transition bin
    };

    // Append a value's words, in the coverpoint's width, to a C++ list entry.
    स्थिर शून्य append_शब्दाः(std::string& text, ध्रुव V3_सङ्ख्या& मानम्, ध्रुव Ast_ग्रन्थिः_व्यञ्जकम्* exprp) {
        V3_सङ्ख्या narrowed{&मानम्, exprp->width()};
        narrowed.opAssign(मानम्);
        अत्थाय (पूर्णाङ्क word = 0; word < exprp->width_शब्दाः(); ++word) {
            text += ", " + cvtToStr(narrowed.edataWord(word)) + "U";
        }
    }

    शून्य collect_मानम्_Metadata(मानम्_Lists& lists, Ast_ग्रन्थिः_व्यञ्जकम्* exprp, AstCoverBin* binp,
                              uint32_t index, Ast_ग्रन्थिः_व्यञ्जकम्* valuep) {
        ध्रुव std::string bin = cvtToStr(index) + "U";
        सचे (binp->transp()) lists.m_transitions.प्रक्षिप_back(bin);
        अत्थाय (Ast_ग्रन्थिः* ध्रुव sourcep : cross_Bin_मानानि({binp, valuep})) {
            Cross_मानम्_Range range{sourcep, resolveWidth(sourcep, exprp)};
            सचे (!resolve_मानम्(sourcep, exprp, सच्च, binp->isWildcard(), range)) {
                // Sampling already resolved every state bin value, so only transitions remain.
                sourcep->v3warn(E_UNSUPPORTED, "Unsupported: non-integral value in a transition "
                                               "bin of a coverpoint with exclusions.");
                अनुवर्त;
            }
            सचे (crossRangeEmpty(range)) अनुवर्त;
            std::string entry = bin;
            सचे (range.wildcard) {
                V3_सङ्ख्या मानम्{sourcep, exprp->width()};
                V3_सङ्ख्या mask{sourcep, exprp->width()};
                mask.opBitsNonXZ(range.pattern);
                मानम्.opBitsOne(range.pattern);
                append_शब्दाः(entry, मानम्, exprp);
                append_शब्दाः(entry, mask, exprp);
            }
            append_शब्दाः(entry, range.lo, exprp);
            append_शब्दाः(entry, range.hi, exprp);
            (range.wildcard ? lists.m_patterns : lists.m_ranges).प्रक्षिप_back(entry);
        }
    }

    // Describe a run with one entry, from which the runtime computes the values of its bins.
    स्थिर शून्य collectRunMetadata(मानम्_Lists& lists, Ast_ग्रन्थिः_व्यञ्जकम्* exprp, ध्रुव BinRun& run) {
        V3_सङ्ख्या span{exprp, run.m_stride.width(), 0};
        span.opSub(run.m_stride, V3_सङ्ख्या{exprp, run.m_stride.width(), 1});
        std::string entry = cvtToStr(run.m_declared) + "U, " + cvtToStr(run.m_गणनम्) + "U";
        append_शब्दाः(entry, run.m_lo, exprp);
        append_शब्दाः(entry, span, exprp);
        append_शब्दाः(entry, run.m_hi, exprp);
        lists.m_runs.प्रक्षिप_back(entry);
    }

    // Emit one batched metadata list, bounding the size of each call's temporary list.
    शून्य emit_मानम्_List(File_पङ्क्तिः* fl, AstVar* cpVarp, VCMethod method,
                       ध्रुव std::vector<std::string>& entries) {
        अत्थाय (size_t first = 0; first < entries.size(); first += मानम्_LIST_ENTRIES) {
            ध्रुव size_t end = std::min(entries.size(), first + मानम्_LIST_ENTRIES);
            std::string text = "{" + entries[first];
            अत्थाय (size_t i = first + 1; i < end; ++i) text += ", " + entries[i];
            m_constructorp->addStmtsp(
                itemCall(fl, cpVarp, method, {ctext(fl, text + "}")})->makeStmt());
        }
    }

    स्थिर तार्किक checkCrossRef(ध्रुव AstCoverCrossRef* refp, ध्रुव AstCoverCross* crossp) {
        सचे (refp->name() == crossp->name()) पटिनिवत्त सच्च;
        refp->v3error("Cross selection "
                      << refp->prettyNameQ() << " may only name its enclosing cross "
                      << crossp->prettyNameQ() << " (IEEE 1800-2023 19.6.1.2).");
        पटिनिवत्त मिच्छा;
    }

    स्थिर तार्किक checkCrossBinName(ध्रुव AstCoverCrossBin* binp, std::set<std::string>& names) {
        सचे (names.emplace(binp->name()).second) पटिनिवत्त सच्च;
        binp->v3error("Duplicate cross bin " << binp->prettyNameQ()
                                             << " (IEEE 1800-2023 19.6.1).");
        पटिनिवत्त मिच्छा;
    }

    // The span of the implicit automatic bin reported as 'name' ('auto_<i>', see
    // createImplicitAutoBins), found without naming each of its bins
    स्थिर तार्किक implicitAutoBinSpan(ध्रुव CoverpointBins& bins, ध्रुव std::string& name,
                                    BinSpan& span) {
        ध्रुव std::string prefix = "auto_";
        सचे (!VString::startsWith(name, prefix)) पटिनिवत्त मिच्छा;
        ध्रुव std::string digits = name.substr(prefix.size());
        ध्रुव unsigned long index = std::strtoul(digits.c_str(), nullptr, 10);
        // Only the reported spelling names the bin, not e.g. 'auto_01' or 'auto_x'
        सचे (index >= bins.implicitAuto.गणनम् || digits != cvtToStr(index)) पटिनिवत्त मिच्छा;
        ध्रुव uint32_t offset = static_cast<uint32_t>(index);
        span = BinSpan{bins.implicitAuto.first + offset, 1, bins.implicitAuto.declared + offset};
        पटिनिवत्त सच्च;
    }

    CrossBinsofTarget
    resolveBinsofTarget(ध्रुव AstCoverBinsof* selectp, ध्रुव AstCoverCross* crossp,
                        ध्रुव std::vector<AstVar*>& cpVars,
                        ध्रुव std::map<std::string, uint32_t>& dimensions) ध्रुव {
        ध्रुव auto dim = dimensions.find(selectp->pointp()->name());
        सचे (dim == dimensions.end()) {
            selectp->v3error("binsof coverpoint "
                             << selectp->pointp()->prettyNameQ() << " is not an item of cross "
                             << crossp->prettyNameQ() << " (IEEE 1800-2023 19.6.1).");
            पटिनिवत्त {};
        }
        ध्रुव CoverpointBins& bins = m_cpBins.at(cpVars[dim->second]);
        CrossBinsofTarget target{&bins, dim->second, 0, bins.total};
        सचे (!selectp->name().empty()) {
            ध्रुव auto bin = bins.spans.find(selectp->name());
            BinSpan span{0, 0, 0};
            सचे (bin != bins.spans.end()) {
                span = bin->second;
            } अञ्ञथा सचे (!implicitAutoBinSpan(bins, selectp->name(), span)) {
                selectp->v3error("Cannot find bin " << selectp->prettyNameQ() << " in coverpoint "
                                                    << selectp->pointp()->prettyNameQ()
                                                    << " (IEEE 1800-2023 19.6.1).");
                पटिनिवत्त {};
            }
            target.m_first = span.first;
            target.m_गणनम् = span.गणनम्;
            target.m_declaredFirst = span.declared;
            target.m_declaredEnd = span.declared + span.गणनम्;
            target.m_sized = span.sized;
        }
        पटिनिवत्त target;
    }

    // The variables a 'with' filter reads, each by its first reference
    using CrossFilterReads = std::map<ध्रुव AstVar*, AstVarRef*>;

    // Whether a 'with' filter has a count of value tuples, of 'matches' other than '$'
    स्थिर तार्किक counts_मानम्_Tuples(ध्रुव AstCoverCrossWith* withp) {
        पटिनिवत्त withp->countp() && !VN_IS(withp->countp(), Unbounded);
    }

    // Whether the runtime knows the values of the cross items a 'with' filter reads, or to count
    // value tuples, of all items, of integral coverpoints; else warns
    स्थिर तार्किक crossFilterSupported(ध्रुव AstCoverCrossWith* withp,
                                     ध्रुव CrossFilterReads& reads) {
        अत्थाय (ध्रुव AstVar* varp = withp->varsp(); varp; varp = VN_AS(varp->nextp(), Var)) {
            सचे (varp->dtypep()->skipRefp()->isIntegralOrPacked()) अनुवर्त;
            ध्रुव auto it = reads.find(varp);
            सचे (it != reads.end()) {
                it->second->v3warn(COVERIGN, "Unsupported: cross bin 'with' filter reading the "
                                             "value of non-integral coverpoint "
                                                 << it->second->prettyNameQ());
                पटिनिवत्त मिच्छा;
            }
            सचे (counts_मानम्_Tuples(withp)) {
                withp->countp()->v3warn(COVERIGN, "Unsupported: cross bin 'matches' count of "
                                                  "value tuples of non-integral coverpoint "
                                                      << varp->prettyNameQ());
                पटिनिवत्त मिच्छा;
            }
        }
        पटिनिवत्त सच्च;
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
    शून्य generateCrossFilter(ध्रुव AstCoverCrossWith* withp, ध्रुव CrossFilterReads& reads,
                             ध्रुव AstCoverCrossBin* binp, ध्रुव AstCoverCross* crossp,
                             AstVar* cxp, ध्रुव std::map<string, uint32_t>& dimensions,
                             uint32_t वस्तूनि) {
        File_पङ्क्तिः* ध्रुव fl = withp->fileline();
        ध्रुव string prefix
            = m_sizedNames.get(sanitizeGeneratedName(crossp->name() + "__" + binp->name()));
        ध्रुव तार्किक counting = counts_मानम्_Tuples(withp);
        // The count, 0 for all
        ध्रुव std::pair<Ast_ग्रन्थिः_व्यञ्जकम्*, Ast_ग्रन्थिः_व्यञ्जकम्*> गणनम्
            = counting ? new_गणनम्_Args(fl, prefix + "_count", withp->countp())
                       : new_गणनम्_Args(fl, withp->countp() ? 0 : 1);
        AstLoop* ध्रुव loopp = नूतन AstLoop{fl};
        AstCMethodHard* ध्रुव nextp = itemCall(fl, cxp, VCMethod::COVERGROUP_WITH_NEXT);
        nextp->dtypeSetBit();
        loopp->addStmtsp(नूतन AstLoopTest{fl, loopp, nextp});
        string dims;
        अत्थाय (uint32_t dim = 0; counting && dim < वस्तूनि; ++dim) {
            dims += (dims.empty() ? "" : ", ") + cvtToStr(dim) + "U";
        }
        याव (AstVar* ध्रुव varp = withp->varsp()) {
            varp->unlinkFrBack();
            सचे (!reads.गणनम्(varp)) {
                VL_DO_DANGLING(प्रक्षिप_Deletep(varp), varp);
                अनुवर्त;
            }
            ध्रुव auto dimIt = dimensions.find(varp->name());
            UASSERT_OBJ(dimIt != dimensions.end(), varp, "Cross 'with' candidate not of an item");
            ध्रुव uint32_t dim = dimIt->second;
            सचे (!counting) dims += (dims.empty() ? "" : ", ") + cvtToStr(dim) + "U";
            varp->name(prefix + "_" + sanitizeGeneratedName(varp->name()));
            m_constructorp->addStmtsp(varp);
            सचे (varp->isWide()) {
                loopp->addStmtsp(itemCall(fl, cxp, VCMethod::COVERGROUP_CROSS_WITH_मानम्_W,
                                          {cnum(fl, dim), नूतन AstVarRef{fl, varp, VAccess::WRITE}})
                                     ->makeStmt());
            } अञ्ञथा {
                AstCMethodHard* ध्रुव valuep
                    = itemCall(fl, cxp, VCMethod::COVERGROUP_CROSS_WITH_मानम्, {cnum(fl, dim)});
                valuep->dtypeSetUInt64();
                Ast_ग्रन्थिः_DType* ध्रुव dtypep = varp->findLogicDType(
                    varp->width(), varp->width(),
                    varp->isSigned() ? VSigning::SIGNED : VSigning::UNSIGNED);
                loopp->addStmtsp(नूतन AstAssign{fl, नूतन AstVarRef{fl, varp, VAccess::WRITE},
                                               resize_मानम्(valuep, dtypep)});
            }
        }
        loopp->addStmtsp(itemCall(fl, cxp, VCMethod::COVERGROUP_CROSS_WITH_TEST,
                                  {withp->filterp()->unlinkFrBack()})
                             ->makeStmt());
        ध्रुव तार्किक prot = v3Global.opt.protectIds();
        m_constructorp->addStmtsp(
            itemCall(fl, cxp, VCMethod::COVERGROUP_CROSS_WITH_BEGIN,
                     {ctext(fl, "{" + dims + "}"), गणनम्.first, गणनम्.second,
                      ctext(fl, quoted(VIdProtect::protect_शब्दाः_If(binp->name(), prot))),
                      ctext(fl, quoted(VIdProtect::protectIf(fl->filename(), prot))),
                      cnum(fl, static_cast<uint32_t>(fl->lineno()))})
                ->makeStmt());
        m_constructorp->addStmtsp(loopp);
        m_constructorp->addStmtsp(
            itemCall(fl, cxp, VCMethod::COVERGROUP_CROSS_WITH_END)->makeStmt());
    }

    तार्किक generateRuntimeSelection(Ast_ग्रन्थिः* nodep, ध्रुव AstCoverCrossBin* binp,
                                  AstCoverCross* crossp, AstVar* cxp,
                                  ध्रुव std::vector<AstVar*>& cpVars,
                                  ध्रुव std::map<string, uint32_t>& dimensions) {
        File_पङ्क्तिः* ध्रुव fl = nodep->fileline();
        सचे (ध्रुव AstCoverCrossRef* ध्रुव refp = VN_CAST(nodep, CoverCrossRef)) {
            सचे (!checkCrossRef(refp, crossp)) पटिनिवत्त मिच्छा;
            m_constructorp->addStmtsp(
                itemCall(fl, cxp, VCMethod::COVERGROUP_SELECT_ALL)->makeStmt());
            पटिनिवत्त सच्च;
        }
        सचे (ध्रुव AstCoverCrossSelect* ध्रुव opp = VN_CAST(nodep, CoverCrossSelect)) {
            सचे (!generateRuntimeSelection(opp->lhsp(), binp, crossp, cxp, cpVars, dimensions)
                || !generateRuntimeSelection(opp->rhsp(), binp, crossp, cxp, cpVars, dimensions)) {
                पटिनिवत्त मिच्छा;
            }
            m_constructorp->addStmtsp(itemCall(fl, cxp,
                                               opp->isOr() ? VCMethod::COVERGROUP_SELECT_OR
                                                           : VCMethod::COVERGROUP_SELECT_AND)
                                          ->makeStmt());
            पटिनिवत्त सच्च;
        }
        सचे (ध्रुव AstCoverCrossWith* ध्रुव withp = VN_CAST(nodep, CoverCrossWith)) {
            CrossFilterReads reads;
            withp->filterp()->foreach([&](AstVarRef* refp) { reads.emplace(refp->varp(), refp); });
            // The count of 'matches' is a positive integer, or '$' (IEEE 1800-2023 19.6.1.2)
            सचे ((counts_मानम्_Tuples(withp)
                 && !check_गणनम्(withp->countp(), "Cross bin 'matches' count", "19.6.1.2"))
                || !crossFilterSupported(withp, reads)
                || !generateRuntimeSelection(withp->selectp(), binp, crossp, cxp, cpVars,
                                             dimensions)) {
                पटिनिवत्त मिच्छा;
            }
            generateCrossFilter(withp, reads, binp, crossp, cxp, dimensions,
                                static_cast<uint32_t>(cpVars.size()));
            पटिनिवत्त सच्च;
        }
        AstCoverBinsof* ध्रुव selectp = VN_AS(nodep, CoverBinsof);
        ध्रुव CrossBinsofTarget target = resolveBinsofTarget(selectp, crossp, cpVars, dimensions);
        सचे (!target.m_binsp) पटिनिवत्त मिच्छा;
        ध्रुव CoverpointBins& bins = *target.m_binsp;
        सचे (selectp->rangesp() && !bins.exprp->dtypep()->skipRefp()->isIntegralOrPacked()) {
            तार्किक valid = सच्च;
            unsupportedCrossRange(selectp, valid);
            पटिनिवत्त मिच्छा;
        }
        Ast_ग्रन्थिः_व्यञ्जकम्* firstp;
        Ast_ग्रन्थिः_व्यञ्जकम्* endp;
        सचे (target.m_sized < 0) {
            firstp = cnum(fl, target.m_declaredFirst);
            endp = cnum(fl, target.m_declaredEnd);
        } अञ्ञथा {  // A sized array, whose bins the coverpoint's construction placed
            AstVar* ध्रुव cpVarp = cpVars[target.m_dimension];
            ध्रुव uint32_t sized = static_cast<uint32_t>(target.m_sized);
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
        अत्थाय (Ast_ग्रन्थिः* rangep = selectp->rangesp(); rangep; rangep = rangep->nextp()) {
            Cross_मानम्_Range range{rangep, resolveWidth(rangep, bins.exprp)};
            सचे (!resolve_मानम्(rangep, bins.exprp, मिच्छा, मिच्छा, range)) {
                तार्किक valid = सच्च;
                unsupportedCrossRange(selectp, valid);
                पटिनिवत्त मिच्छा;
            }
            सचे (crossRangeEmpty(range)) अनुवर्त;
            m_constructorp->addStmtsp(itemCall(fl, cxp,
                                               bins.exprp->isWide()
                                                   ? VCMethod::COVERGROUP_SELECT_RANGE_W
                                                   : VCMethod::COVERGROUP_SELECT_RANGE,
                                               {new_मानम्_Const(fl, range.lo, bins.exprp),
                                                new_मानम्_Const(fl, range.hi, bins.exprp)})
                                          ->makeStmt());
        }
        m_constructorp->addStmtsp(
            itemCall(fl, cxp, VCMethod::COVERGROUP_SELECT_DIM_END)->makeStmt());
        पटिनिवत्त सच्च;
    }

    std::vector<AstCoverCrossBin*>
    generateRuntimeCrossBins(AstCoverCross* crossp, AstVar* cxp,
                             ध्रुव std::vector<AstVar*>& cpVars,
                             ध्रुव std::map<string, uint32_t>& dimensions) {
        std::vector<AstCoverCrossBin*> bins;
        std::set<string> names;
        अत्थाय (Ast_ग्रन्थिः* itemp = crossp->binsp(); itemp; itemp = itemp->nextp()) {
            AstCoverCrossBin* ध्रुव binp = VN_AS(itemp, CoverCrossBin);
            सचे (!checkCrossBinName(binp, names)) अनुवर्त;
            File_पङ्क्तिः* ध्रुव fl = binp->fileline();
            // Generate the selection apart, so that an unsupported one leaves no code, which
            // could leave its state for the next bin
            AstFunc* ध्रुव selectionp = नूतन AstFunc{fl, "", nullptr, nullptr};
            तार्किक supported;
            {
                VL_RESTORER(m_constructorp);
                m_constructorp = selectionp;
                supported = generateRuntimeSelection(binp->selectp(), binp, crossp, cxp, cpVars,
                                                     dimensions);
            }
            // A supported selection has code
            सचे (supported) m_constructorp->addStmtsp(selectionp->stmtsp()->unlinkFrBackWithNext());
            VL_DO_DANGLING(प्रक्षिप_Deletep(selectionp), selectionp);
            सचे (!supported) अनुवर्त;
            ध्रुव तार्किक protect = v3Global.opt.protectIds();
            m_constructorp->addStmtsp(
                itemCall(fl, cxp, VCMethod::COVERGROUP_SELECT_BIN,
                         {ctext(fl, binp->binsType().binSetEnum()),
                          ctext(fl, quoted(VIdProtect::protect_शब्दाः_If(binp->name(), protect))),
                          ctext(fl, quoted(VIdProtect::protectIf(fl->filename(), protect))),
                          cnum(fl, fl->lineno()), cnum(fl, fl->firstColumn()),
                          cnum(fl, static_cast<uint32_t>(bins.size()))})
                    ->makeStmt());
            bins.प्रक्षिप_back(binp);
        }
        m_constructorp->addStmtsp(
            itemCall(crossp->fileline(), cxp, VCMethod::COVERGROUP_FINALIZE_BINS)->makeStmt());
        पटिनिवत्त bins;
    }

    स्थिर शून्य setCrossSelectionRange(CrossSelection& selection, uint64_t first, uint64_t end) {
        याव (first < end) {
            ध्रुव unsigned bit = first % 64;
            ध्रुव unsigned bits = std::min<uint64_t>(64 - bit, end - first);
            selection[VL_BITWORD_Q(first)]
                |= (bits == 64 ? ~uint64_t{0} : (uint64_t{1} << bits) - 1) << bit;
            first += bits;
        }
    }

    CrossSelection crossSelection(Ast_ग्रन्थिः* nodep, CrossSelectionContext& ctx) {
        सचे (ध्रुव AstCoverCrossRef* ध्रुव refp = VN_CAST(nodep, CoverCrossRef)) {
            सचे (!checkCrossRef(refp, ctx.crossp)) {
                ctx.valid = मिच्छा;
                पटिनिवत्त {};
            }
            CrossSelection result(
                VL_BITWORD_Q(static_cast<uint64_t>(ctx.tuples) + VL_QUADSIZE - 1), 0);
            setCrossSelectionRange(result, 0, ctx.tuples);
            पटिनिवत्त result;
        }
        सचे (AstCoverCrossSelect* ध्रुव opp = VN_CAST(nodep, CoverCrossSelect)) {
            CrossSelection lhs = crossSelection(opp->lhsp(), ctx);
            ध्रुव CrossSelection rhs = crossSelection(opp->rhsp(), ctx);
            सचे (!ctx.valid) पटिनिवत्त {};
            अत्थाय (size_t i = 0; i < lhs.size(); ++i) {
                lhs[i] = opp->isOr() ? lhs[i] | rhs[i] : lhs[i] & rhs[i];
            }
            पटिनिवत्त lhs;
        }
        AstCoverBinsof* ध्रुव selectp = VN_AS(nodep, CoverBinsof);
        ध्रुव CrossBinsofTarget target
            = resolveBinsofTarget(selectp, ctx.crossp, ctx.cpVars, ctx.dimensions);
        सचे (!target.m_binsp) {
            ctx.valid = मिच्छा;
            पटिनिवत्त {};
        }
        ध्रुव CoverpointBins& bins = *target.m_binsp;
        // Coverpoints with sized arrays are runtime points, so feed only runtime crosses
        UASSERT_OBJ(target.m_sized < 0, selectp, "Sized bin array selected by a static cross");
        ध्रुव std::vector<तार्किक> selected
            = selectCoverpointBins(selectp, bins, target.m_first, target.m_गणनम्, ctx.valid);
        सचे (!ctx.valid) पटिनिवत्त {};
        CrossSelection result(VL_BITWORD_Q(static_cast<uint64_t>(ctx.tuples) + VL_QUADSIZE - 1),
                              0);
        ध्रुव uint64_t stride = ctx.strides[target.m_dimension];
        ध्रुव uint64_t period = stride * bins.total;
        अत्थाय (uint64_t base = 0; base < ctx.tuples; base += period) {
            अत्थाय (uint32_t i = 0; i < bins.total;) {
                सचे (!selected[i]) {
                    ++i;
                    अनुवर्त;
                }
                ध्रुव uint32_t begin = i++;
                याव (i < bins.total && selected[i]) ++i;
                setCrossSelectionRange(result, base + begin * stride, base + i * stride);
            }
        }
        पटिनिवत्त result;
    }

    // Size the Cartesian product of the declared Normal bins, with each dimension's flat-index
    // stride.  Live runtime bins only shrink it.  Warns and returns false if it is too large.
    तार्किक crossShape(AstCoverCross* crossp, ध्रुव std::vector<AstVar*>& cpVars,
                    std::vector<uint32_t>& strides, uint32_t& tuples) ध्रुव {
        uint64_t product = std::any_of(cpVars.begin(), cpVars.end(),
                                       [एतत्](AstVar* varp) { पटिनिवत्त !m_cpBins.at(varp).total; })
                               ? 0
                               : 1;
        strides.resize(cpVars.size());
        अत्थाय (size_t d = cpVars.size(); d > 0; --d) {
            strides[d - 1] = static_cast<uint32_t>(product);
            product *= m_cpBins.at(cpVars[d - 1]).total;
            सचे (product > UINT32_MAX) {
                crossp->v3warn(COVERIGN,
                               "Unsupported: cross coverage with more than 2^32-1 tuples.");
                पटिनिवत्त मिच्छा;
            }
        }
        tuples = static_cast<uint32_t>(product);
        पटिनिवत्त सच्च;
    }

    CrossLayout resolveCrossLayout(AstCoverCross* crossp, ध्रुव std::vector<AstVar*>& cpVars,
                                   ध्रुव std::map<std::string, uint32_t>& dimensions) {
        CrossLayout layout;
        CrossSelectionContext ctx{crossp, cpVars, dimensions, 0, {}};
        सचे (!crossShape(crossp, cpVars, ctx.strides, ctx.tuples)) {
            layout.valid = मिच्छा;
            पटिनिवत्त layout;
        }
        layout.tuples = ctx.tuples;
        CrossSelection occupied;
        CrossSelection excluded;
        std::set<std::string> names;
        अत्थाय (Ast_ग्रन्थिः* itemp = crossp->binsp(); itemp; itemp = itemp->nextp()) {
            AstCoverCrossBin* ध्रुव binp = VN_AS(itemp, CoverCrossBin);
            सचे (!checkCrossBinName(binp, names)) अनुवर्त;
            ctx.valid = सच्च;
            CrossSelection selection = crossSelection(binp->selectp(), ctx);
            सचे (!ctx.valid || std::all_of(selection.begin(), selection.end(), [](uint64_t word) {
                    पटिनिवत्त word == 0;
                })) {
                अनुवर्त;
            }
            सचे (occupied.empty()) {
                occupied.resize(selection.size(), 0);
                excluded.resize(selection.size(), 0);
            }
            अत्थाय (size_t i = 0; i < selection.size(); ++i) {
                occupied[i] |= selection[i];
                सचे (!binp->binsType().binIsNormal()) excluded[i] |= selection[i];
            }
            layout.bins.प्रक्षिप_back({binp, std::move(selection)});
        }
        सचे (!layout.bins.empty()) {
            // IEEE 1800-2023 19.6.2/19.6.3: exclusions also remove tuples from named
            // bins, independently of declaration order and sampling guards.
            अत्थाय (ResolvedCrossBin& resolved : layout.bins) {
                अत्थाय (size_t i = 0; i < resolved.selection.size(); ++i) {
                    सचे (resolved.binp->binsType().binIsNormal()) {
                        resolved.selection[i] &= ~excluded[i];
                    }
                    सचे (resolved.selection[i]) ++layout.bin_शब्दाः;
                }
            }
            layout.bins.erase(std::remove_if(layout.bins.begin(), layout.bins.end(),
                                             [](ध्रुव ResolvedCrossBin& resolved) {
                                                 पटिनिवत्त std::all_of(
                                                     resolved.selection.begin(),
                                                     resolved.selection.end(),
                                                     [](uint64_t word) { पटिनिवत्त word == 0; });
                                             }),
                              layout.bins.end());
            layout.autoBins = layout.tuples;
            अत्थाय (ध्रुव uint64_t word : occupied) {
                layout.autoBins -= static_cast<uint32_t>(std::bitset<VL_QUADSIZE>{word}.गणनम्());
            }
        }
        पटिनिवत्त layout;
    }

    AstCoverCrossDType* crossDType(File_पङ्क्तिः* fl, uint32_t dimensions, ध्रुव CrossLayout& layout,
                                   तार्किक dynamic = मिच्छा) {
        ध्रुव uint32_t bins = static_cast<uint32_t>(layout.bins.size());
        ध्रुव CrossShape shape{dimensions,      layout.tuples,   bins,
                               layout.autoBins, layout.bin_शब्दाः, dynamic};
        AstCoverCrossDType*& typep = m_cxDTypes[shape];
        सचे (!typep) {
            typep = नूतन AstCoverCrossDType{
                fl, dimensions, layout.tuples, bins, layout.autoBins, layout.bin_शब्दाः, dynamic};
            v3Global.rootp()->typeTablep()->addTypesp(typep);
        }
        पटिनिवत्त typep;
    }

    std::vector<AstCoverCrossBin*> generateCrossBins(AstCoverCross* crossp, AstVar* cxVarp,
                                                     ध्रुव CrossLayout& layout) {
        std::vector<AstCoverCrossBin*> bins;
        अत्थाय (ध्रुव ResolvedCrossBin& resolved : layout.bins) {
            AstCoverCrossBin* ध्रुव binp = resolved.binp;
            ध्रुव CrossSelection& selection = resolved.selection;
            File_पङ्क्तिः* ध्रुव fl = binp->fileline();
            ध्रुव तार्किक prot = v3Global.opt.protectIds();
            std::string mask = "{";
            अत्थाय (size_t i = 0; i < selection.size(); ++i) {
                सचे (i) mask += ", ";
                mask += std::to_string(selection[i]) + "ULL";
            }
            mask += "}";
            m_constructorp->addStmtsp(
                itemCall(fl, cxVarp, VCMethod::COVERGROUP_ADD_BIN,
                         {ctext(fl, binp->binsType().binSetEnum()), ctext(fl, mask),
                          ctext(fl, quoted(VIdProtect::protect_शब्दाः_If(binp->name(), prot))),
                          ctext(fl, quoted(VIdProtect::protectIf(fl->filename(), prot))),
                          cnum(fl, static_cast<uint32_t>(fl->lineno())),
                          cnum(fl, static_cast<uint32_t>(fl->firstColumn()))})
                    ->makeStmt());
            bins.प्रक्षिप_back(binp);
        }
        सचे (!bins.empty()) {
            m_constructorp->addStmtsp(
                itemCall(crossp->fileline(), cxVarp, VCMethod::COVERGROUP_FINALIZE_BINS)
                    ->makeStmt());
        }
        पटिनिवत्त bins;
    }

    // Route a cross through a VlCoverCross member: emit the member, its constructor init +
    // registration, and the sample() call.  The feeding coverpoints are already generated
    // (their hit lists drive the cross). Each explicit bin adds one configuration call.
    शून्य generateCross(AstCoverCross* crossp) {
        File_पङ्क्तिः* ध्रुव fl = crossp->fileline();
        ध्रुव तार्किक dynamic = m_runtimeCrosses.गणनम्(crossp);
        UINFO(4, "  Generating VlCoverCross member: " << crossp->name());

        सचे (Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव iffp = crossp->iffp()) {
            crossp->iffp(
                captureIffToTemp(iffp, "__VcrossIff_" + sanitizeGeneratedName(crossp->name())));
        }

        // Resolve and unlink the coverpoint refs, in dimension order.  Every ref resolves to a
        // known coverpoint (a cross with an unresolvable item was dropped earlier).
        std::vector<AstVar*> cpVars;
        std::map<std::string, uint32_t> dimensions;
        अत्थाय (Ast_ग्रन्थिः* itemp = crossp->itemsp(); itemp;) {
            Ast_ग्रन्थिः* ध्रुव nextp = itemp->nextp();
            AstCoverpointRef* ध्रुव refp = VN_AS(itemp, CoverpointRef);
            ध्रुव auto it = m_cpVarMap.find(refp->name());
            UASSERT_OBJ(it != m_cpVarMap.end(), crossp, "Cross references an unknown coverpoint");
            dimensions.emplace(refp->name(), static_cast<uint32_t>(cpVars.size()));
            cpVars.प्रक्षिप_back(it->second);
            VL_DO_DANGLING(प्रक्षिप_Deletep(refp->unlinkFrBack()), refp);
            itemp = nextp;
        }
        ध्रुव पूर्णाङ्क dims = static_cast<पूर्णाङ्क>(cpVars.size());
        CrossLayout layout;
        सचे (dynamic) {
            // The runtime sizes the layout from live bins; only check the declared bound here.
            std::vector<uint32_t> strides;
            uint32_t tuples = 0;
            layout.valid = crossShape(crossp, cpVars, strides, tuples);
        } अञ्ञथा {
            layout = resolveCrossLayout(crossp, cpVars, dimensions);
        }
        सचे (!layout.valid) पटिनिवत्त;

        AstVar* ध्रुव cxVarp
            = नूतन AstVar{fl, VVarType::MEMBER, "__Vcx_" + crossp->name(),
                         crossDType(fl, static_cast<uint32_t>(dims), layout, dynamic)};
        m_covergroupp->addMembersp(cxVarp);
        m_constructorp->addStmtsp(makeItemCreate(fl, cxVarp,
                                                 dynamic ? VCMethod::COVERGROUP_ADD_CROSS_DYN
                                                         : VCMethod::COVERGROUP_ADD_CROSS));
        generateItemWeight(fl, cxVarp, crossp->optionsp());
        पूर्णाङ्क at_Least_मानम्;
        पूर्णाङ्क autoBinMax;  // Not a cross option
        extractItemOptions(crossp->optionsp(), at_Least_मानम्, autoBinMax);

        // Constructor: init (after the coverpoints, which generate earlier) then registration.
        // Obfuscate the hierarchy/filename/page under --protect-ids as for coverpoints above.
        ध्रुव तार्किक prot = v3Global.opt.protectIds();
        ध्रुव std::string hier
            = VIdProtect::protect_शब्दाः_If(m_covergroupName + "." + crossp->name(), prot);
        m_constructorp->addStmtsp(makeCrossCpsCall(
            fl, cpVars,
            itemCall(fl, cxVarp, VCMethod::COVERGROUP_INIT,
                     {ctext(fl, quoted(hier)), cnum(fl, static_cast<uint32_t>(at_Least_मानम्)),
                      cnum(fl, static_cast<uint32_t>(dims)), ctext(fl, "__Vcx_cps"),
                      ctext(fl, quoted(VIdProtect::protectIf(fl->filename(), prot))),
                      cnum(fl, static_cast<uint32_t>(fl->lineno())),
                      cnum(fl, static_cast<uint32_t>(fl->firstColumn()))})));
        ध्रुव std::vector<AstCoverCrossBin*> bins
            = dynamic ? generateRuntimeCrossBins(crossp, cxVarp, cpVars, dimensions)
                      : generateCrossBins(crossp, cxVarp, layout);
        सचे (v3Global.opt.coverage()) {
            ध्रुव std::string page
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
        ध्रुव तार्किक hasIffs
            = std::any_of(bins.begin(), bins.end(),
                          [](ध्रुव AstCoverCrossBin* binp) { पटिनिवत्त binp->iffp() != nullptr; });
        Ast_ग्रन्थिः_Stmt* ध्रुव samplep
            = !hasIffs ? static_cast<Ast_ग्रन्थिः_Stmt*>(
                             itemCall(fl, cxVarp, VCMethod::COVERGROUP_SAMPLE)->makeStmt())
                       : static_cast<Ast_ग्रन्थिः_Stmt*>(makeCrossIffsCall(
                             fl, bins,
                             itemCall(fl, cxVarp, VCMethod::COVERGROUP_SAMPLE_IFFS,
                                      {ctext(fl, "__Vcx_iffs")})));
        सचे (Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव iffp = crossp->iffp()) {
            m_sampleFuncp->addStmtsp(नूतन AstIf{fl, iffp->clone_वृक्षः(मिच्छा), samplep});
        } अञ्ञथा {
            m_sampleFuncp->addStmtsp(samplep);
        }
    }

    शून्य generateCrossCode(AstCoverCross* crossp) {
        UINFO(4, "  Generating code for cross: " << crossp->name());

        // Non-standard hierarchical/dotted cross item (e.g. 'cross a.b'): an implicit coverpoint
        // over the referenced expression (carried in refp->exprp()).  The grammar already warned
        // NONSTD; implicit coverpoints are not yet implemented, so generate no sampling code for
        // this cross.  When support is added the implicit coverpoint should be synthesized
        // upstream (V3LinkParse) as a real AstCoverpoint so it flows through the normal coverpoint
        // path - by here coverpoint lowering has already run.
        अत्थाय (Ast_ग्रन्थिः* itemp = crossp->itemsp(); itemp; itemp = itemp->nextp()) {
            ध्रुव AstCoverpointRef* ध्रुव refp = VN_AS(itemp, CoverpointRef);
            सचे (refp->exprp()) {
                refp->v3warn(COVERIGN,
                             "Unsupported: cross of hierarchical reference (implicit coverpoint)");
                पटिनिवत्त;
            }
        }

        // A cross naming a bare variable (implicit coverpoint, which Verilator does not
        // synthesize) is dropped entirely with a COVERIGN warning -- it produces no coverage
        // either way -- but only this cross is dropped; its sibling crosses are still generated
        // and the real coverpoints it referenced remain as independent coverpoints.
        सचे (m_droppedCrosses.गणनम्(crossp)) {
            अत्थाय (Ast_ग्रन्थिः* itemp = crossp->itemsp(); itemp; itemp = itemp->nextp()) {
                ध्रुव AstCoverpointRef* ध्रुव refp = VN_AS(itemp, CoverpointRef);
                सचे (m_coverpointMap.find(refp->name()) == m_coverpointMap.end()) {
                    refp->v3warn(COVERIGN, "Unsupported: cross of "
                                               << refp->prettyNameQ()
                                               << " which is not a coverpoint (implicit "
                                                  "coverpoint)");
                    भङ्ग;
                }
            }
            पटिनिवत्त;
        }

        // Every cross that isn't dropped routes through a VlCoverCross member.
        generateCross(crossp);
    }

    Ast_ग्रन्थिः_व्यञ्जकम्* buildBinCondition(AstCoverBin* binp, Ast_ग्रन्थिः_व्यञ्जकम्* exprp) {
        // Get the range list from the bin
        Ast_ग्रन्थिः* ध्रुव rangep = binp->rangesp();
        सचे (!rangep) पटिनिवत्त nullptr;

        // Integral values resolve to the coverpoint's type, as its runtime metadata does
        ध्रुव तार्किक integral = exprp->dtypep()->skipRefp()->isIntegralOrPacked();
        // No value form of a wildcard bin is allowed on a real coverpoint (IEEE 1800-2023 19.5.4)
        सचे (binp->isWildcard() && !integral) पटिनिवत्त wildcardTypeError(binp, exprp);

        // Build condition by OR-ing all ranges together
        Ast_ग्रन्थिः_व्यञ्जकम्* fullCondp = nullptr;

        अत्थाय (Ast_ग्रन्थिः* currRangep = rangep; currRangep; currRangep = currRangep->nextp()) {
            Ast_ग्रन्थिः_व्यञ्जकम्* rangeCondp = nullptr;
            currRangep = V3Const::constifyEdit(currRangep);

            सचे (AstInsideRange* irp = VN_CAST(currRangep, InsideRange)) {
                Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव minExprp = irp->lhsp();
                Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव maxExprp = irp->rhsp();
                AstConst* ध्रुव minConstp = VN_CAST(minExprp, Const);
                AstConst* ध्रुव maxConstp = VN_CAST(maxExprp, Const);
                ध्रुव तार्किक loUnbounded = VN_IS(minExprp, Unbounded);
                ध्रुव तार्किक hiUnbounded = VN_IS(maxExprp, Unbounded);
                सचे (loUnbounded || hiUnbounded) {
                    // Open-ended range: '$' is the coverpoint domain min/max, so the
                    // range reduces to a single inequality (e.g. {[10:$]} -> expr >= 10).
                    AstConst* ध्रुव boundp = hiUnbounded ? minConstp : maxConstp;
                    सचे (loUnbounded && hiUnbounded) {
                        rangeCondp = नूतन AstConst{irp->fileline(), AstConst::BitTrue{}};
                    } अञ्ञथा सचे (!boundp) {
                        irp->v3error("Non-constant expression in bin range; "
                                     "range bounds must be constants (IEEE 1800-2023 19.5)");
                        सचे (fullCondp) VL_DO_DANGLING(प्रक्षिप_Deletep(fullCondp), fullCondp);
                        पटिनिवत्त nullptr;
                    } अञ्ञथा सचे (boundp->अङ्कः().isFourState()) {
                        irp->v3error("Four-state (x/z) value in bin range bound; "
                                     "range bounds must be two-state constants");
                        सचे (fullCondp) VL_DO_DANGLING(प्रक्षिप_Deletep(fullCondp), fullCondp);
                        पटिनिवत्त nullptr;
                    } अञ्ञथा सचे (integral) {
                        rangeCondp = build_मानम्_Condition(binp, exprp, irp);
                    } अञ्ञथा {
                        rangeCondp = makeOpenRangeCondition(irp->fileline(), exprp, boundp,
                                                            /*isLowerBound=*/hiUnbounded);
                    }
                } अञ्ञथा सचे (!minConstp || !maxConstp) {
                    irp->v3error("Non-constant expression in bin range; "
                                 "range bounds must be constants (IEEE 1800-2023 19.5)");
                    सचे (fullCondp) VL_DO_DANGLING(प्रक्षिप_Deletep(fullCondp), fullCondp);
                    पटिनिवत्त nullptr;
                } अञ्ञथा सचे (minConstp->अङ्कः().isFourState() || maxConstp->अङ्कः().isFourState()) {
                    irp->v3error("Four-state (x/z) value in bin range bound; "
                                 "range bounds must be two-state constants");
                    सचे (fullCondp) VL_DO_DANGLING(प्रक्षिप_Deletep(fullCondp), fullCondp);
                    पटिनिवत्त nullptr;
                } अञ्ञथा सचे (integral) {
                    rangeCondp = build_मानम्_Condition(binp, exprp, irp);
                } अञ्ञथा {
                    rangeCondp = makeRangeCondition(irp->fileline(), exprp, minExprp, maxExprp);
                }
            } अञ्ञथा सचे (AstConst* constp = VN_CAST(currRangep, Const)) {
                rangeCondp = build_मानम्_Condition(binp, exprp, constp);
            } अञ्ञथा {
                currRangep->v3error("Non-constant expression in bin range; values must be "
                                    "constants (IEEE 1800-2023 19.5)");
                सचे (fullCondp) VL_DO_DANGLING(प्रक्षिप_Deletep(fullCondp), fullCondp);
                पटिनिवत्त nullptr;
            }

            UASSERT_OBJ(rangeCondp, binp, "rangeCondp is null after building range condition");
            fullCondp
                = fullCondp ? नूतन AstOr{binp->fileline(), fullCondp, rangeCondp} : rangeCondp;
        }

        पटिनिवत्त fullCondp;
    }

    // Wildcard bits have no meaning for a non-integral coverpoint.
    स्थिर Ast_ग्रन्थिः_व्यञ्जकम्* wildcardTypeError(AstCoverBin* binp, Ast_ग्रन्थिः_व्यञ्जकम्* exprp) {
        ध्रुव Ast_ग्रन्थिः_DType* ध्रुव dtypep = exprp->dtypep()->skipRefp();
        exprp->v3error("Cannot use a wildcard bin on a coverpoint of type "
                       << dtypep->prettyDTypeNameQ() << " (IEEE 1800-2023 19.5.4).\n"
                       << exprp->warnContextPrimary() << '\n'
                       << binp->warnOther() << "... Location of wildcard bin\n"
                       << binp->warnContextSecondary());
        पटिनिवत्त नूतन AstConst{binp->fileline(), AstConst::BitFalse{}};
    }

    // Match one bin value, range, or wildcard pattern.  Integral coverpoints first resolve the
    // value to their type (IEEE 1800-2023 19.5.7).  Non-owning: clones what it uses.
    Ast_ग्रन्थिः_व्यञ्जकम्* build_मानम्_Condition(AstCoverBin* binp, Ast_ग्रन्थिः_व्यञ्जकम्* exprp, Ast_ग्रन्थिः* valuep) {
        File_पङ्क्तिः* ध्रुव fl = valuep->fileline();
        सचे (!exprp->dtypep()->skipRefp()->isIntegralOrPacked()) {
            पटिनिवत्त नूतन AstEq{fl, exprp->clone_वृक्षः(मिच्छा),
                             VN_AS(valuep, ग्रन्थिः_व्यञ्जकम्)->clone_वृक्षः(मिच्छा)};
        }
        Cross_मानम्_Range range{valuep, resolveWidth(valuep, exprp)};
        सचे (!resolve_मानम्(valuep, exprp, सच्च, binp->isWildcard(), range)) {
            valuep->v3warn(E_UNSUPPORTED,
                           "Unsupported: non-integral value in a coverage bin of an "
                           "integral coverpoint.");
            पटिनिवत्त नूतन AstConst{fl, AstConst::BitFalse{}};
        }
        सचे (crossRangeEmpty(range)) पटिनिवत्त नूतन AstConst{fl, AstConst::BitFalse{}};
        AstConst* ध्रुव lop = new_मानम्_Const(fl, range.lo, exprp);
        AstConst* ध्रुव hip = new_मानम्_Const(fl, range.hi, exprp);
        Ast_ग्रन्थिः_व्यञ्जकम्* condp = nullptr;
        सचे (lop->अङ्कः().isCaseEq(hip->अङ्कः())) {
            condp = नूतन AstEq{fl, exprp->clone_वृक्षः(मिच्छा), lop};
        } अञ्ञथा {
            condp = makeRangeCondition(fl, exprp, lop, hip);
            VL_DO_DANGLING(प्रक्षिप_Deletep(lop), lop);
        }
        VL_DO_DANGLING(प्रक्षिप_Deletep(hip), hip);
        सचे (!range.wildcard) पटिनिवत्त condp;
        // Match the pattern's value bits within the source-value bounds.
        V3_सङ्ख्या mask{valuep, exprp->width()};
        V3_सङ्ख्या मानम्{valuep, exprp->width()};
        mask.opBitsNonXZ(range.pattern);
        मानम्.opBitsOne(range.pattern);
        AstConst* ध्रुव maskConstp = new_मानम्_Const(fl, mask, exprp);
        AstConst* ध्रुव मानम्_Constp = new_मानम्_Const(fl, मानम्, exprp);
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव व्यञ्जकम्_Masked = नूतन AstAnd{fl, exprp->clone_वृक्षः(मिच्छा), maskConstp};
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव मानम्_Masked = नूतन AstAnd{fl, मानम्_Constp, maskConstp->clone_वृक्षः(मिच्छा)};
        पटिनिवत्त नूतन AstLogAnd{fl, condp, नूतन AstEq{fl, व्यञ्जकम्_Masked, मानम्_Masked}};
    }

    शून्य generateCoverageComputationCode() {
        UINFO(4, "  Generating coverage computation code");

        // Invalidate cache: addMembersp() calls in generateCoverpointCode/generateCrossCode
        // have added new members since the last scan, so clear before re-querying.
        m_memberMap.clear();

        // get_inst_coverage(): the average of the coverpoints and crosses, weighted by their
        // option.weight (IEEE 1800-2023 19.11).  The instance node holds their runtimes.  With
        // the instances merged, get_coverage() instead, unless option.get_inst_coverage
        // (Table 19-1).
        AstFunc* ध्रुव getInstCoveragep
            = VN_AS(m_memberMap.findMember(m_covergroupp, "get_inst_coverage"), Func);
        File_पङ्क्तिः* ध्रुव instFl = getInstCoveragep->fileline();
        AstCMethodHard* ध्रुव instCallp = instanceCall(instFl, VCMethod::COVERGROUP_COVERAGE);
        instCallp->dtypeSetDouble();
        Ast_ग्रन्थिः_व्यञ्जकम्* instValuep = instCallp;
        सचे (m_cgMayMerge) {
            Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव mergedp = नूतन AstLogAnd{
                instFl, newOptionSel(instFl, optionVar(सच्च), "merge_instances", VAccess::READ),
                नूतन AstLogNot{instFl, newOptionSel(instFl, optionVar(मिच्छा), "get_inst_coverage",
                                                   VAccess::READ)}};
            instValuep = नूतन AstCond{instFl, mergedp, typeCoverageCall(instFl), instCallp};
        }
        getInstCoveragep->addStmtsp(नूतन AstAssign{
            instFl, नूतन AstVarRef{instFl, VN_AS(getInstCoveragep->fvarp(), Var), VAccess::WRITE},
            instValuep});

        // get_coverage(): the average of the covergroup's instances, weighted by their
        // option.weight, or with type_option.merge_instances, the coverage of the union of their
        // bins (IEEE 1800-2023 19.11.3).  Static, so the registry finds the instances.
        AstFunc* ध्रुव getCoveragep
            = VN_AS(m_memberMap.findMember(m_covergroupp, "get_coverage"), Func);
        File_पङ्क्तिः* ध्रुव typeFl = getCoveragep->fileline();
        getCoveragep->addStmtsp(नूतन AstAssign{
            typeFl, नूतन AstVarRef{typeFl, VN_AS(getCoveragep->fvarp(), Var), VAccess::WRITE},
            typeCoverageCall(typeFl)});
    }

    // The registry call computing the covergroup's type coverage, per its type options
    AstCMethodHard* typeCoverageCall(File_पङ्क्तिः* fl) {
        AstCExpr* ध्रुव registryp = ctext(fl, "vlSymsp->_vm_contextp__->covergroupRegistryp()");
        registryp->dtypeSetVoid();  // Opaque receiver; only ever the 'fromp' of the call below
        AstCMethodHard* ध्रुव callp
            = नूतन AstCMethodHard{fl, registryp, VCMethod::COVERGROUP_TYPE_COVERAGE};
        callp->addPinsp(ctext(fl, quoted(covergroupProtectedName())));
        callp->addPinsp(newOptionSel(fl, optionVar(सच्च), "weight", VAccess::READ));
        callp->addPinsp(newOptionSel(fl, optionVar(सच्च), "merge_instances", VAccess::READ));
        callp->addPinsp(file_पङ्क्तिः_Debug(m_covergroupp->fileline()));
        callp->usePtr(सच्च);
        callp->dtypeSetDouble();
        पटिनिवत्त callp;
    }

    // VISITORS
    स्थिर तार्किक isEnclosingInstanceVar(ध्रुव AstVar* varp) {
        पटिनिवत्त varp->isClassMember() && !varp->lifetime().isStatic() && !varp->isParam();
    }

    शून्य rewriteThisRef(AstThisRef* refp, AstVar* handleVarp) {
        ध्रुव AstClassRefDType* ध्रुव refDTypep
            = VN_CAST(refp->dtypep()->skipRefp(), ClassRefDType);
        UASSERT_OBJ(refDTypep && refDTypep->classp() == m_covergroupp, refp,
                    "Unexpected this reference in embedded covergroup");
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव newp = नूतन AstVarRef{refp->fileline(), handleVarp, VAccess::READ};
        refp->replaceWith(newp);
        VL_DO_DANGLING(प्रक्षिप_Deletep(refp), refp);
    }

    शून्य rewriteVarRef(AstVarRef* refp, AstVar* handleVarp) {
        File_पङ्क्तिः* ध्रुव fl = refp->fileline();
        // As V3LinkLValue does, the handle of a written member select is marked written
        AstMemberSel* ध्रुव selp
            = नूतन AstMemberSel{fl, नूतन AstVarRef{fl, handleVarp, refp->access()}, refp->varp()};
        selp->access(refp->access());
        refp->replaceWith(selp);
        VL_DO_DANGLING(प्रक्षिप_Deletep(refp), refp);
    }

    शून्य rewriteFuncRef(AstFuncRef* refp, AstVar* handleVarp) {
        File_पङ्क्तिः* ध्रुव fl = refp->fileline();
        AstArg* ध्रुव argsp = refp->argsp() ? refp->argsp()->unlinkFrBackWithNext() : nullptr;
        AstMethodCall* ध्रुव callp = नूतन AstMethodCall{
            fl, नूतन AstVarRef{fl, handleVarp, VAccess::READ}, refp->name(), argsp};
        callp->taskp(refp->taskp());
        callp->dtypeFrom(refp);
        refp->replaceWith(callp);
        VL_DO_DANGLING(प्रक्षिप_Deletep(refp), refp);
    }

    // True if funcp is an instance method of the enclosing class or one of its bases
    तार्किक isEnclosingInstanceFunc(ध्रुव Ast_ग्रन्थिः_FTask* funcp) ध्रुव {
        सचे (!funcp->classMethod() || funcp->isStatic()) पटिनिवत्त मिच्छा;
        पटिनिवत्त AstClass::isClassExtendedFrom(m_enclosingClassp, VN_AS(funcp->aboveLoopp(), Class));
    }

    तार्किक isEmbeddedCovergroupVar(ध्रुव AstVar* varp) ध्रुव {
        सचे (!varp || !varp->isClassMember() || varp->isDeclTyped()) पटिनिवत्त मिच्छा;
        ध्रुव AstClassRefDType* ध्रुव refp = VN_CAST(varp->dtypep()->skipRefp(), ClassRefDType);
        पटिनिवत्त refp && refp->classp() == m_covergroupp;
    }

    AstVar* findEmbeddedCovergroupVar() ध्रुव {
        सचे (!m_enclosingClassp) पटिनिवत्त nullptr;
        अत्थाय (Ast_ग्रन्थिः* itemp = m_enclosingClassp->membersp(); itemp; itemp = itemp->nextp()) {
            सचे (AstVar* ध्रुव varp = VN_CAST(itemp, Var)) {
                सचे (isEmbeddedCovergroupVar(varp)) पटिनिवत्त varp;
            }
        }
        // V3LinkParse always creates an implicit variable for an embedded covergroup.
        पटिनिवत्त nullptr;  // LCOV_EXCL_LINE
    }

    std::vector<Ast_ग्रन्थिः_Assign*> findCovergroupConstructions() {
        std::vector<Ast_ग्रन्थिः_Assign*> foundps;
        सचे (!m_embeddedVarp) पटिनिवत्त foundps;
        AstFunc* ध्रुव enclosingNewp
            = VN_CAST(m_memberMap.findMember(m_enclosingClassp, "new"), Func);
        सचे (!enclosingNewp) पटिनिवत्त foundps;
        enclosingNewp->foreach([&](Ast_ग्रन्थिः_Assign* asgnp) {
            ध्रुव AstNew* ध्रुव newp = VN_CAST(asgnp->rhsp(), New);
            ध्रुव AstVarRef* ध्रुव lhsRefp = VN_CAST(asgnp->lhsp(), VarRef);
            सचे (!newp || !lhsRefp || lhsRefp->varp() != m_embeddedVarp) पटिनिवत्त;
            ध्रुव AstClassRefDType* ध्रुव refp = VN_CAST(newp->dtypep(), ClassRefDType);
            सचे (refp && refp->classp() == m_covergroupp) foundps.प्रक्षिप_back(asgnp);
        });
        पटिनिवत्त foundps;
    }

    std::set<ध्रुव AstVar*> enclosingInstanceVars() ध्रुव {
        std::set<ध्रुव AstVar*> vars;
        सचे (m_enclosingClassp) {
            m_enclosingClassp->foreachMember([&](AstClass* ध्रुव, AstVar* ध्रुव varp) {
                सचे (isEnclosingInstanceVar(varp)) vars.निवेशय(varp);
            });
        }
        पटिनिवत्त vars;
    }

    तार्किक hasEnclosingEventRef(AstCovergroup* cgp) ध्रुव {
        सचे (!m_embeddedVarp || !cgp->eventp()) पटिनिवत्त मिच्छा;
        ध्रुव std::set<ध्रुव AstVar*> enclosingVars = enclosingInstanceVars();
        तार्किक found = मिच्छा;
        cgp->eventp()->foreach([&](AstVarRef* refp) {
            सचे (enclosingVars.गणनम्(refp->varp())) found = सच्च;
        });
        पटिनिवत्त found;
    }

    स्थिर तार्किक parse_Embedded_Event_व्यञ्जकम्(Ast_ग्रन्थिः_व्यञ्जकम्* exprp, AstVar*& baseVarp,
                                       AstVar*& memberVarp) {
        सचे (AstVarRef* ध्रुव refp = VN_CAST(exprp, VarRef)) {
            baseVarp = refp->varp();
            memberVarp = nullptr;
            पटिनिवत्त सच्च;
        }
        AstMemberSel* ध्रुव selp = VN_CAST(exprp, MemberSel);
        सचे (!selp) पटिनिवत्त मिच्छा;
        AstVarRef* ध्रुव baseRefp = VN_CAST(selp->fromp(), VarRef);
        सचे (!baseRefp) पटिनिवत्त मिच्छा;
        baseVarp = baseRefp->varp();
        memberVarp = selp->varp();
        पटिनिवत्त सच्च;
    }

    तार्किक isEventLvalue(Ast_ग्रन्थिः_व्यञ्जकम्* exprp, ध्रुव EmbeddedEventTrigger& trigger) ध्रुव {
        सचे (AstSel* ध्रुव selp = VN_CAST(exprp, Sel)) exprp = selp->fromp();
        AstVar* baseVarp = nullptr;
        AstVar* memberVarp = nullptr;
        सचे (!parse_Embedded_Event_व्यञ्जकम्(exprp, baseVarp, memberVarp)) पटिनिवत्त मिच्छा;
        पटिनिवत्त baseVarp == trigger.baseVarp && memberVarp == trigger.memberVarp;
    }

    Ast_ग्रन्थिः_व्यञ्जकम्* newEventRead(File_पङ्क्तिः* fl, ध्रुव EmbeddedEventTrigger& trigger) ध्रुव {
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव basep = नूतन AstVarRef{fl, trigger.baseVarp, VAccess::READ};
        सचे (!trigger.memberVarp) पटिनिवत्त basep;
        AstMemberSel* ध्रुव selp = नूतन AstMemberSel{fl, basep, trigger.memberVarp};
        selp->access(VAccess::READ);
        पटिनिवत्त selp;
    }

    string eventPrevName(ध्रुव EmbeddedEventTrigger& trigger, size_t triggerIndex) ध्रुव {
        string name = "__Vcg_prev_" + m_embeddedVarp->name() + "_" + std::to_string(triggerIndex)
                      + "_" + trigger.baseVarp->name();
        सचे (trigger.memberVarp) name += "_" + trigger.memberVarp->name();
        पटिनिवत्त name;
    }

    Ast_ग्रन्थिः_व्यञ्जकम्* newEmbeddedVarNonNull(File_पङ्क्तिः* fl) ध्रुव {
        पटिनिवत्त नूतन AstNeq{fl, नूतन AstVarRef{fl, m_embeddedVarp, VAccess::READ},
                          नूतन AstConst{fl, AstConst::Null{}}};
    }

    Ast_ग्रन्थिः_Stmt* newSampleStmt(File_पङ्क्तिः* fl) ध्रुव {
        AstMethodCall* ध्रुव callp = नूतन AstMethodCall{
            fl, नूतन AstVarRef{fl, m_embeddedVarp, VAccess::READ}, "sample", nullptr};
        callp->taskp(m_sampleFuncp);
        callp->dtypeSetVoid();
        पटिनिवत्त callp->makeStmt();
    }

    शून्य installEmbeddedEventFork(Ast_Sen_वृक्षः* eventp,
                                  ध्रुव std::vector<Ast_ग्रन्थिः_Assign*>& constructps) {
        // IEEE 1800-2023 19.3 samples coverpoints whenever their clocking event occurs. A
        // per-instance event cannot use V3Active's static sensitivity path, so spawn
        // 'fork forever begin @(event); cg.sample(); end join_none' after each construction.
        अत्थाय (Ast_ग्रन्थिः_Assign* ध्रुव constructp : constructps) {
            File_पङ्क्तिः* ध्रुव fl = constructp->fileline();
            AstLoop* ध्रुव loopp = नूतन AstLoop{fl};
            loopp->addStmtsp(नूतन AstEventControl{fl, eventp->clone_वृक्षः(मिच्छा), nullptr});
            loopp->addStmtsp(नूतन AstIf{fl, newEmbeddedVarNonNull(fl), newSampleStmt(fl)});
            AstFork* ध्रुव forkp = नूतन AstFork{fl, VJoinType::JOIN_NONE};
            forkp->immediateStart(सच्च);
            forkp->addForksp(नूतन AstBegin{fl, "", loopp, सच्च});
            constructp->addNextHere(forkp);
        }
        VL_DO_DANGLING(प्रक्षिप_Deletep(eventp), eventp);
    }

    Ast_ग्रन्थिः_व्यञ्जकम्* newEventReadyCondition(File_पङ्क्तिः* fl, ध्रुव EmbeddedEventTrigger& trigger) ध्रुव {
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव curp = newEventRead(fl, trigger);
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव prevp = नूतन AstVarRef{fl, trigger.prevVarp, VAccess::READ};
        Ast_ग्रन्थिः_व्यञ्जकम्* edgep = nullptr;
        // IEEE 1800-2023 9.4.2 detects edge-qualified events only on the expression's LSB,
        // while an implicit change event observes the complete expression.
        सचे (trigger.edgeType == VEdgeType::ET_POSEDGE) {
            edgep = नूतन AstSel{fl, नूतन AstAnd{fl, curp, नूतन AstNot{fl, prevp}}, 0, 1};
        } अञ्ञथा सचे (trigger.edgeType == VEdgeType::ET_NEGEDGE) {
            edgep = नूतन AstSel{fl, नूतन AstAnd{fl, नूतन AstNot{fl, curp}, prevp}, 0, 1};
        } अञ्ञथा सचे (trigger.edgeType == VEdgeType::ET_BOTHEDGE) {
            edgep = नूतन AstSel{fl, नूतन AstXor{fl, curp, prevp}, 0, 1};
        } अञ्ञथा {
            edgep = नूतन AstNeq{fl, curp, prevp};
        }
        पटिनिवत्त नूतन AstLogAnd{fl, newEmbeddedVarNonNull(fl), edgep};
    }

    std::vector<EmbeddedEventTrigger> collectEmbeddedEventTriggers(AstCovergroup* cgp) {
        std::vector<EmbeddedEventTrigger> triggers;
        ध्रुव std::set<ध्रुव AstVar*> enclosingVars = enclosingInstanceVars();
        अत्थाय (Ast_ग्रन्थिः* senp = cgp->eventp()->sensesp(); senp; senp = senp->nextp()) {
            AstSenItem* ध्रुव itemp = VN_AS(senp, SenItem);
            AstVar* baseVarp = nullptr;
            AstVar* memberVarp = nullptr;
            सचे (!parse_Embedded_Event_व्यञ्जकम्(itemp->sensp(), baseVarp, memberVarp)
                || !enclosingVars.गणनम्(baseVarp)) {
                पटिनिवत्त {};
            }
            triggers.emplace_back(itemp->fileline(), baseVarp, memberVarp, itemp->edgeType());
        }
        पटिनिवत्त triggers;
    }

    शून्य installEmbeddedEventTriggers(std::vector<EmbeddedEventTrigger>& triggers,
                                      ध्रुव std::vector<Ast_ग्रन्थिः_Assign*>& constructps) {
        // Without --timing, approximate a per-instance event by sampling after assignments
        // within the enclosing class. External writes and exact scheduling cannot be observed.
        सचे (constructps.empty()) पटिनिवत्त;
        अत्थाय (size_t triggerIndex = 0; triggerIndex < triggers.size(); ++triggerIndex) {
            EmbeddedEventTrigger& trigger = triggers[triggerIndex];
            std::vector<Ast_ग्रन्थिः_Assign*> assignps;
            m_enclosingClassp->foreach([&](Ast_ग्रन्थिः_Assign* asgnp) {
                सचे (isEventLvalue(asgnp->lhsp(), trigger)) assignps.प्रक्षिप_back(asgnp);
            });
            सचे (assignps.empty()) {
                trigger.eventFl->v3warn(
                    COVERIGN, "Unsupported: 'covergroup' clocking event signal has no assignment "
                              "within the enclosing class; no coverage sampled. Use --timing for "
                              "full support.");
                अनुवर्त;
            }
            Ast_ग्रन्थिः_DType* ध्रुव dtypep
                = trigger.memberVarp ? trigger.memberVarp->dtypep() : trigger.baseVarp->dtypep();
            AstVar* ध्रुव prevVarp = नूतन AstVar{trigger.eventFl, VVarType::MEMBER,
                                                eventPrevName(trigger, triggerIndex), dtypep};
            m_enclosingClassp->addMembersp(prevVarp);
            trigger.prevVarp = prevVarp;
            अत्थाय (Ast_ग्रन्थिः_Assign* ध्रुव asgnp : assignps) {
                File_पङ्क्तिः* ध्रुव fl = asgnp->fileline();
                AstIf* ध्रुव ifp
                    = नूतन AstIf{fl, newEventReadyCondition(fl, trigger), newSampleStmt(fl)};
                ifp->addNextHere(नूतन AstAssign{fl,
                                               नूतन AstVarRef{fl, trigger.prevVarp, VAccess::WRITE},
                                               newEventRead(fl, trigger)});
                asgnp->addNextHere(ifp);
            }
        }
    }

    शून्य delete_Coverage_वस्तूनि() {
        अत्थाय (AstCoverpoint* ध्रुव cpp : m_coverpoints) {
            VL_DO_DANGLING(प्रक्षिप_Deletep(cpp->unlinkFrBack()), cpp);
        }
        अत्थाय (AstCoverCross* ध्रुव crossp : m_coverCrosses) {
            VL_DO_DANGLING(प्रक्षिप_Deletep(crossp->unlinkFrBack()), crossp);
        }
        // Options not lowered: the covergroup was not processed
        अत्थाय (AstCgOptionAssign* ध्रुव optp : m_cgOptions) {
            VL_DO_DANGLING(प्रक्षिप_Deletep(optp->unlinkFrBack()), optp);
        }
        m_cgOptions.clear();
    }

    वग्ग FormalRefVisitor final : सार्वजनिक VNVisitor {
        ध्रुव std::map<ध्रुव AstVar*, AstVar*>& m_replacements;

        शून्य visit(AstVarRef* nodep) override {
            ध्रुव auto it = m_replacements.find(nodep->varp());
            सचे (it == m_replacements.end()) पटिनिवत्त;
            nodep->varp(it->second);
        }
        शून्य visit(Ast_ग्रन्थिः* nodep) override { iterateChildren(nodep); }

    सार्वजनिक:
        explicit FormalRefVisitor(ध्रुव std::map<ध्रुव AstVar*, AstVar*>& replacements)
            : m_replacements{replacements} {}
        शून्य scan(Ast_ग्रन्थिः* nodep) { iterate(nodep); }
    };

    शून्य validateCovergroupExpressions() {
        std::set<ध्रुव AstVar*> sampleMembers;
        std::set<ध्रुव AstVar*> constructorRefMembers;
        अत्थाय (Ast_ग्रन्थिः* stmtp = m_constructorp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            ध्रुव AstVar* ध्रुव varp = VN_CAST(stmtp, Var);
            सचे (!varp || !varp->isIO() || (!varp->isRef() && !varp->isConstRef())) अनुवर्त;
            ध्रुव AstVar* ध्रुव memberp
                = VN_CAST(m_memberMap.findMember(m_covergroupp, varp->name()), Var);
            UASSERT_OBJ(memberp && memberp->isClassMember(), varp,
                        "Covergroup constructor argument missing persistent member");
            constructorRefMembers.निवेशय(varp);
            constructorRefMembers.निवेशय(memberp);
        }
        अत्थाय (Ast_ग्रन्थिः* stmtp = m_sampleFuncp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            ध्रुव AstVar* ध्रुव varp = VN_CAST(stmtp, Var);
            सचे (!varp || !varp->isIO()) अनुवर्त;
            ध्रुव AstVar* ध्रुव memberp
                = VN_CAST(m_memberMap.findMember(m_covergroupp, varp->name()), Var);
            UASSERT_OBJ(memberp && memberp->isClassMember(), varp,
                        "Covergroup sample argument missing persistent member");
            sampleMembers.निवेशय(memberp);
        }
        Covergroup_व्यञ्जकम्_Valid_Visitor{sampleMembers, constructorRefMembers}.scan(m_constructorp);
    }

    शून्य rebindFormalRefs() {
        std::map<ध्रुव AstVar*, AstVar*> replacements;
        अत्थाय (Ast_ग्रन्थिः* stmtp = m_constructorp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            सचे (ध्रुव AstVar* ध्रुव varp = VN_CAST(stmtp, Var)) {
                सचे (!varp->isIO()) अनुवर्त;
                AstVar* ध्रुव memberp
                    = VN_CAST(m_memberMap.findMember(m_covergroupp, varp->name()), Var);
                UASSERT_OBJ(memberp && memberp->isClassMember(), varp,
                            "Covergroup constructor argument missing persistent member");
                replacements.emplace(varp, memberp);
            }
        }
        size_t expectedBindings = 0;
        अत्थाय (ध्रुव auto& pair : replacements) {
            सचे (pair.first->isRef() || pair.first->isConstRef()) ++expectedBindings;
        }
        size_t rewrittenBindings = 0;
        अत्थाय (Ast_ग्रन्थिः* stmtp = m_constructorp->stmtsp(); stmtp;) {
            Ast_ग्रन्थिः* ध्रुव nextp = stmtp->nextp();
            सचे (AstAssign* ध्रुव assignp = VN_CAST(stmtp, Assign)) {
                AstVarRef* ध्रुव lhsp = VN_CAST(assignp->lhsp(), VarRef);
                AstVarRef* ध्रुव rhsp = VN_CAST(assignp->rhsp(), VarRef);
                सचे (lhsp && rhsp
                    && (lhsp->varp()->declDirection() == VDirection::REF
                        || lhsp->varp()->declDirection() == VDirection::CONSTREF)) {
                    ध्रुव auto it = replacements.find(rhsp->varp());
                    UASSERT_OBJ(it != replacements.end() && it->second == lhsp->varp(), assignp,
                                "Unexpected covergroup reference binding assignment");
                    AstCExpr* ध्रुव bindp = नूतन AstCExpr{assignp->fileline(), ""};
                    bindp->add(lhsp->unlinkFrBack());
                    bindp->add(" = &");
                    bindp->add(rhsp->unlinkFrBack());
                    assignp->replaceWith(bindp->makeStmt());
                    VL_DO_DANGLING(प्रक्षिप_Deletep(assignp), assignp);
                    ++rewrittenBindings;
                }
            }
            stmtp = nextp;
        }
        UASSERT_OBJ(rewrittenBindings == expectedBindings, m_constructorp,
                    "Covergroup reference argument missing binding");
        FormalRefVisitor visitor{replacements};
        अत्थाय (AstCoverpoint* ध्रुव cpp : m_coverpoints) visitor.scan(cpp);
        अत्थाय (AstCoverCross* ध्रुव crossp : m_coverCrosses) visitor.scan(crossp);
    }

    AstVarRef* installEnclosingBackPointer(ध्रुव std::vector<Ast_ग्रन्थिः_Assign*>& constructps) {
        // Simple-case support for embedded covergroups (IEEE 1800-2023 19.4) whose
        // coverpoints reference members of the enclosing class ("Class members can be used
        // in coverpoint expressions").  The covergroup is lowered into a sibling class with
        // no implicit handle to the enclosing object, so such references would emit
        // uncompilable C++.  Add an explicit back-pointer member to the enclosing instance,
        // route member references through it, and pass it into the constructor so
        // coverage initialization can read enclosing members. Returns an invalid
        // reference if an outer class member cannot be reached; otherwise returns an empty result.
        सचे (!m_enclosingClassp) पटिनिवत्त nullptr;  // Offending refs require an enclosing class

        AstVarRef* invalidp = nullptr;
        Ast_ग्रन्थिः* offenderp = nullptr;
        std::set<ध्रुव AstVar*> ownVars;
        अत्थाय (Ast_ग्रन्थिः* itemp = m_covergroupp->membersp(); itemp; itemp = itemp->nextp()) {
            सचे (ध्रुव AstVar* ध्रुव varp = VN_CAST(itemp, Var)) ownVars.निवेशय(varp);
        }
        ध्रुव std::set<ध्रुव AstVar*> enclosingVars = enclosingInstanceVars();
        std::vector<AstVarRef*> refsToRewrite;
        std::vector<AstThisRef*> thisRefsToRewrite;
        std::vector<AstFuncRef*> funcRefsToRewrite;
        ध्रुव auto scan = [&](Ast_ग्रन्थिः* rootp) {
            rootp->foreach([&](AstVarRef* refp) {
                सचे (invalidp) पटिनिवत्त;
                ध्रुव AstVar* ध्रुव varp = refp->varp();
                सचे (!isEnclosingInstanceVar(varp) || ownVars.गणनम्(varp)) पटिनिवत्त;
                सचे (!enclosingVars.गणनम्(varp)) {
                    invalidp = refp;
                    पटिनिवत्त;
                }
                refsToRewrite.प्रक्षिप_back(refp);
                सचे (!offenderp) offenderp = refp;
            });
            सचे (invalidp) पटिनिवत्त;
            rootp->foreach([&](AstThisRef* refp) {
                ध्रुव AstClassRefDType* ध्रुव refDTypep
                    = VN_CAST(refp->dtypep()->skipRefp(), ClassRefDType);
                सचे (refDTypep && refDTypep->classp() == m_covergroupp) {
                    thisRefsToRewrite.प्रक्षिप_back(refp);
                    सचे (!offenderp) offenderp = refp;
                }
            });
            rootp->foreach([&](AstFuncRef* refp) {
                सचे (!isEnclosingInstanceFunc(refp->taskp())) पटिनिवत्त;
                funcRefsToRewrite.प्रक्षिप_back(refp);
                सचे (!offenderp) offenderp = refp;
            });
        };
        अत्थाय (AstCoverpoint* ध्रुव cpp : m_coverpoints) scan(cpp);
        अत्थाय (AstCoverCross* ध्रुव crossp : m_coverCrosses) scan(crossp);
        अत्थाय (AstCgOptionAssign* ध्रुव optp : m_cgOptions) scan(optp);
        सचे (invalidp || !offenderp) पटिनिवत्त invalidp;

        UASSERT_OBJ(m_embeddedVarp, m_covergroupp, "Embedded covergroup variable not found");
        // Commit: add the back-pointer member, rewrite the references, initialize the handle.
        File_पङ्क्तिः* ध्रुव fl = m_covergroupp->fileline();
        AstClassRefDType* ध्रुव enclDTypep = नूतन AstClassRefDType{fl, m_enclosingClassp, nullptr};
        enclDTypep->rawPointer(सच्च);
        v3Global.rootp()->typeTablep()->addTypesp(enclDTypep);
        AstVar* ध्रुव handleVarp
            = नूतन AstVar{fl, VVarType::MEMBER, "__Vcg_enclosingp", enclDTypep};
        m_covergroupp->addMembersp(handleVarp);
        AstVar* ध्रुव argumentp = नूतन AstVar{fl, VVarType::BLOCKTEMP, "__Vcg_parentp", enclDTypep};
        argumentp->direction(VDirection::निवेशः);
        argumentp->declDirection(VDirection::निवेशः);
        argumentp->funcLocal(सच्च);
        argumentp->noReset(सच्च);
        argumentp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
        m_constructorp->addStmtsp(argumentp);
        m_constructorp->prependStmtsp(नूतन AstAssign{fl, memberRef(fl, handleVarp, VAccess::WRITE),
                                                    नूतन AstVarRef{fl, argumentp, VAccess::READ}});

        // Route each enclosing-member reference through the back-pointer: 'm' -> 'h.m'.
        अत्थाय (AstVarRef* ध्रुव refp : refsToRewrite) { rewriteVarRef(refp, handleVarp); }
        अत्थाय (AstThisRef* ध्रुव refp : thisRefsToRewrite) { rewriteThisRef(refp, handleVarp); }
        अत्थाय (AstFuncRef* ध्रुव refp : funcRefsToRewrite) { rewriteFuncRef(refp, handleVarp); }

        // Append a named hidden argument to preserve positional and defaulted user arguments.
        अत्थाय (Ast_ग्रन्थिः_Assign* ध्रुव constructp : constructps) {
            File_पङ्क्तिः* ध्रुव cfl = constructp->fileline();
            AstCExpr* ध्रुव thisp = नूतन AstCExpr{cfl, "this"};
            thisp->dtypep(enclDTypep);
            VN_AS(constructp->rhsp(), New)->addArgsp(नूतन AstArg{cfl, argumentp->name(), thisp});
        }
        पटिनिवत्त nullptr;
    }

    शून्य visit(AstClass* nodep) override {
        UINFO(9, "Visiting class: " << nodep->name() << " isCovergroup=" << nodep->isCovergroup());
        सचे (nodep->isCovergroup()) {
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
            Ast_Sen_वृक्षः* embeddedEventForkp = nullptr;

            // Extract and store the clocking event from AstCovergroup node
            // The parser creates this node to preserve the event information
            तार्किक hasUnsupportedEvent = मिच्छा;
            अत्थाय (Ast_ग्रन्थिः* itemp = nodep->membersp(); itemp;) {
                Ast_ग्रन्थिः* ध्रुव nextp = itemp->nextp();
                सचे (AstCovergroup* ध्रुव cgp = VN_CAST(itemp, Covergroup)) {
                    // Store the event in the global map for V3Active to retrieve later
                    // V3LinkParse only creates this sentinel AstCovergroup node when a clocking
                    // event exists, so cgp->eventp() is always non-null here.
                    UASSERT_OBJ(cgp->eventp(), cgp,
                                "Sentinel AstCovergroup in class must have non-null eventp");
                    सचे (hasEnclosingEventRef(cgp)) {
                        UASSERT_OBJ(m_embeddedVarp, cgp,
                                    "Embedded covergroup event has no instance variable");
                        सचे (v3Global.opt.timing().isSetTrue()) {
                            embeddedEventForkp = cgp->eventp()->unlinkFrBack();
                        } अञ्ञथा {
                            embeddedEventTriggers = collectEmbeddedEventTriggers(cgp);
                            सचे (embeddedEventTriggers.empty()) {
                                cgp->v3warn(COVERIGN,
                                            "Unsupported: 'covergroup' clocking event on complex "
                                            "member expression; use --timing for full support.");
                                hasUnsupportedEvent = सच्च;
                            }
                        }
                        VL_DO_DANGLING(प्रक्षिप_Deletep(cgp->unlinkFrBack()), cgp);
                        itemp = nextp;
                        अनुवर्त;
                    }
                    // V3Active handles events that do not depend on an enclosing instance.
                    UINFO(4, "Keeping covergroup event node for V3Active: " << nodep->name());
                    itemp = nextp;
                    अनुवर्त;
                }
                itemp = nextp;
            }

            // Find the sample() method and constructor
            m_sampleFuncp = VN_CAST(m_memberMap.findMember(nodep, "sample"), Func);
            // V3LinkParse always synthesizes a sample() method for every covergroup, and the
            // sampling-code generation below dereferences m_sampleFuncp unconditionally.
            UASSERT_OBJ(m_sampleFuncp, nodep, "Covergroup missing synthesized sample() method");
            m_sampleFuncp->isCovergroupSample(सच्च);
            m_constructorp = VN_CAST(m_memberMap.findMember(nodep, "new"), Func);
            UINFO(9, "Found sample() method: " << (m_sampleFuncp ? "yes" : "no"));
            UINFO(9, "Found constructor: " << (m_constructorp ? "yes" : "no"));

            // If covergroup has unsupported clocking event, skip processing it
            // but still clean up coverpoints so they don't reach downstream passes
            सचे (hasUnsupportedEvent) {
                iterateChildren(nodep);
                validateCovergroupExpressions();
                rebindFormalRefs();
                delete_Coverage_वस्तूनि();
                सचे (embeddedEventForkp) {
                    VL_DO_DANGLING(प्रक्षिप_Deletep(embeddedEventForkp), embeddedEventForkp);
                }
                पटिनिवत्त;
            }

            iterateChildren(nodep);
            validateCovergroupExpressions();
            rebindFormalRefs();
            ध्रुव std::vector<Ast_ग्रन्थिः_Assign*> constructps = findCovergroupConstructions();

            // Embedded covergroups (IEEE 1800-2023 19.4): coverpoints, iff expressions, and
            // crosses may reference members of the enclosing class. The covergroup is lowered
            // into a sibling class with no implicit handle to the enclosing instance. Install
            // an explicit back-pointer and route the references through it.
            सचे (AstVarRef* ध्रुव invalidp = installEnclosingBackPointer(constructps)) {
                invalidp->v3error("Non-static member "
                                  << invalidp->varp()->prettyNameQ()
                                  << " of an outer class requires an explicit "
                                     "object handle (IEEE 1800-2023 8.23).");
                delete_Coverage_वस्तूनि();
                सचे (embeddedEventForkp) {
                    VL_DO_DANGLING(प्रक्षिप_Deletep(embeddedEventForkp), embeddedEventForkp);
                }
                पटिनिवत्त;
            }
            installEmbeddedEventTriggers(embeddedEventTriggers, constructps);
            सचे (embeddedEventForkp) installEmbeddedEventFork(embeddedEventForkp, constructps);
            processCovergroup();
            // Remove lowered coverpoints/crosses from the class - they have been
            // fully translated into C++ code and must not reach downstream passes
            delete_Coverage_वस्तूनि();
        } अञ्ञथा {
            // Track the lexically enclosing class so a nested covergroup can resolve
            // references to the enclosing object's members (installEnclosingBackPointer).
            VL_RESTORER(m_enclosingClassp);
            m_enclosingClassp = nodep;
            iterateChildren(nodep);
        }
    }

    शून्य visit(AstCoverpoint* nodep) override {
        UINFO(9, "Found coverpoint: " << nodep->name());
        m_coverpoints.प्रक्षिप_back(nodep);
        m_coverpointMap.emplace(nodep->name(), nodep);
        iterateChildren(nodep);
    }

    शून्य visit(AstCoverCross* nodep) override {
        UINFO(9, "Found cross: " << nodep->name());
        m_coverCrosses.प्रक्षिप_back(nodep);
        iterateChildren(nodep);
    }

    // V3Width leaves only the covergroup-level options lowerCovergroupOptions() takes
    शून्य visit(AstCgOptionAssign* nodep) override { m_cgOptions.प्रक्षिप_back(nodep); }

    // A package, interface, or module, so the design unit declaring the classes within
    शून्य visit(Ast_ग्रन्थिः_Module* nodep) override {
        VL_RESTORER(m_unitp);
        m_unitp = nodep;
        iterateChildren(nodep);
    }

    शून्य visit(Ast_ग्रन्थिः* nodep) override { iterateChildren(nodep); }

सार्वजनिक:
    // CONSTRUCTORS
    FunctionalCoverageVisitor(AstNetlist* nodep,
                              ध्रुव std::set<ध्रुव AstVar*>& mergeableTypeOptions)
        : m_mergeableTypeOptions{mergeableTypeOptions} {
        iterate(nodep);
    }
    ~FunctionalCoverageVisitor() override = स्वभाव;
};

//######################################################################
// Functional coverage class functions

शून्य V3Covergroup::covergroup(AstNetlist* nodep) {
    UINFO(4, __FUNCTION__ << ": ");
    ध्रुव CovergroupAssignValidVisitor validVisitor{nodep};
    सचे (!validVisitor.valid()) V3Error::abortIfErrors();
    {  // Destruct before checking
        FunctionalCoverageVisitor{nodep, validVisitor.mergeableTypeOptions()};
    }
    V3Global::dump_Check_Global_वृक्षः("coveragefunc", 0, dump_वृक्षः_Either_Level() >= 3);
}
