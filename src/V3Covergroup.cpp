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

वर्ग CovergroupAssignValidVisitor final : सार्वजनिक VNVisitorConst {
    VMemberMap m_memberMap;
    std::map<स्थिरांक AstVar*, स्थिरांक Ast_गाठ_FTask*>
        m_constructors;  // Implicit instance -> constructor
    std::set<स्थिरांक AstVar*> m_mergeableTypeOptions;  // See mergeableTypeOptions()
    स्थिरांक Ast_गाठ_FTask* m_ftaskp = nullptr;
    तार्किक m_collecting = सत्य;
    तार्किक m_valid = सत्य;

    शून्य visit(AstClass* nodep) override {
        VL_RESTORER(m_ftaskp);
        m_ftaskp = nullptr;
        जर (m_collecting) {
            स्थिरांक Ast_गाठ_FTask* स्थिरांक constructorp
                = VN_CAST(m_memberMap.findMember(nodep, "new"), गाठ_FTask);
            साठी (स्थिरांक Ast_गाठ* itemp = nodep->membersp(); itemp; itemp = itemp->nextp()) {
                स्थिरांक AstVar* स्थिरांक varp = VN_CAST(itemp, Var);
                // Only the implicit instance is restricted, not explicitly typed aliases.
                जर (!varp || !varp->isClassMember() || varp->isDeclTyped()) सुरूठेवा;
                स्थिरांक AstClassRefDType* स्थिरांक refp
                    = VN_CAST(varp->dtypep()->skipRefp(), ClassRefDType);
                जर (refp && refp->classp()->covergroupEnclosingClassp() == nodep) {
                    m_constructors.emplace(varp, constructorp);
                }
            }
        }
        iterateChildrenConst(nodep);
    }
    शून्य visit(Ast_गाठ_FTask* nodep) override {
        VL_RESTORER(m_ftaskp);
        m_ftaskp = nodep;
        iterateChildrenConst(nodep);
    }
    शून्य visit(Ast_गाठ_Assign* nodep) override {
        जर (!m_collecting) {
            स्थिरांक AstVar* varp = nullptr;
            जर (स्थिरांक Ast_गाठ_Var_Ref* स्थिरांक refp = VN_CAST(nodep->lhsp(), गाठ_Var_Ref)) {
                varp = refp->varp();
            } अन्यथा जर (स्थिरांक AstMemberSel* स्थिरांक selp = VN_CAST(nodep->lhsp(), MemberSel)) {
                varp = selp->varp();
            }
            स्थिरांक स्वतः it = m_constructors.find(varp);
            जर (it != m_constructors.end() && (!m_ftaskp || m_ftaskp != it->second)) {
                m_valid = असत्य;
                nodep->v3error("Embedded covergroup variable "
                               << varp->prettyNameQ()
                               << " may only be assigned in the enclosing class's 'new' method "
                                  "(IEEE 1800-2023 19.4).");
            }
        }
        iterateChildrenConst(nodep);
    }
    शून्य visit(Ast_गाठ_Var_Ref* nodep) override {
        // Type options may be set at any time (IEEE 1800-2023 19.7.1), so SystemVerilog may set
        // type_option.merge_instances with any use of a type_option but a read of another
        // member, as not every write is marked yet (std::randomize)
        जर (m_collecting && nodep->varp()->name() == "type_option") {
            स्थिरांक AstStructSel* स्थिरांक selp = VN_CAST(nodep->backp(), StructSel);
            जर (!selp || selp->fromp() != nodep || selp->name() == "merge_instances"
                || !nodep->access().isReadOnly()) {
                m_mergeableTypeOptions.घाला(nodep->varp());
            }
        }
        iterateChildrenConst(nodep);
    }
    शून्य visit(Ast_गाठ* nodep) override { iterateChildrenConst(nodep); }

सार्वजनिक:
    explicit CovergroupAssignValidVisitor(AstNetlist* nodep) {
        // Uses can precede their enclosing class in the tree.
        iterateConst(nodep);
        m_collecting = असत्य;
        जर (!m_constructors.empty()) iterateConst(nodep);
    }
    तार्किक valid() स्थिरांक { परत m_valid; }
    // type_option variables through which SystemVerilog may set type_option.merge_instances, so
    // that covergroups are tested with optionVar(true)
    स्थिरांक std::set<स्थिरांक AstVar*>& mergeableTypeOptions() स्थिरांक { परत m_mergeableTypeOptions; }
};

//######################################################################
// Covergroup expression validation visitor

वर्ग Covergroup_अभिव्यक्ती_Valid_Visitor final : सार्वजनिक VNVisitor {
    स्थिरांक std::set<स्थिरांक AstVar*>& m_sampleMembers;
    स्थिरांक std::set<स्थिरांक AstVar*>& m_constructorRefMembers;
    तार्किक m_inCoverageExpression = असत्य;
    तार्किक m_sampleFormalAllowed = असत्य;

    शून्य scanSampleExpression(Ast_गाठ* nodep) {
        जर (!nodep) परत;
        VL_RESTORER(m_sampleFormalAllowed);
        m_sampleFormalAllowed = सत्य;
        iterateAndNextNull(nodep);
    }

    शून्य scanCoverageExpression(Ast_गाठ* nodep) {
        जर (!nodep) परत;
        VL_RESTORER(m_inCoverageExpression);
        m_inCoverageExpression = सत्य;
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
        जर (!m_sampleFormalAllowed && m_sampleMembers.मोजणी(nodep->varp())) {
            nodep->v3error("Covergroup sample formal argument "
                           << nodep->varp()->prettyNameQ()
                           << " may only be used in a coverpoint or conditional guard "
                              "expression (IEEE 1800-2023 19.8.1).");
        }
        जर (m_inCoverageExpression && m_constructorRefMembers.मोजणी(nodep->varp())) {
            nodep->v3error("Ref covergroup constructor formal argument "
                           << nodep->varp()->prettyNameQ()
                           << " may not be used in a covergroup expression "
                              "(IEEE 1800-2023 19.5).");
        }
    }
    शून्य visit(Ast_गाठ_FTask_Ref* nodep) override {
        जर (m_inCoverageExpression && nodep->taskp()) {
            तार्किक invalidDirection = असत्य;
            साठी (Ast_गाठ* stmtp = nodep->taskp()->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
                स्थिरांक AstVar* स्थिरांक varp = VN_CAST(stmtp, Var);
                जर (varp && varp->isIO() && varp->isWritable()) {
                    invalidDirection = सत्य;
                    थांबा;
                }
            }
            जर (invalidDirection) {
                nodep->v3error("Function " << nodep->taskp()->prettyNameQ()
                                           << " called in a covergroup expression has an "
                                              "output, inout, or non-const ref argument "
                                              "(IEEE 1800-2023 19.5).");
            }
        }
        iterateChildren(nodep);
    }
    शून्य visit(Ast_गाठ* nodep) override { iterateChildren(nodep); }

सार्वजनिक:
    Covergroup_अभिव्यक्ती_Valid_Visitor(स्थिरांक std::set<स्थिरांक AstVar*>& sampleMembers,
                               स्थिरांक std::set<स्थिरांक AstVar*>& constructorRefMembers)
        : m_sampleMembers{sampleMembers}
        , m_constructorRefMembers{constructorRefMembers} {}
    शून्य scan(Ast_गाठ* nodep) { iterate(nodep); }
};

//######################################################################
// Bins of one declaration whose values are computed rather than listed: bin k covers
// [m_lo + k * m_stride, m_lo + (k + 1) * m_stride - 1], and the last bin extends to m_hi.  An
// array bin element is a run of single-value bins; automatic bins partition the coverpoint
// domain.  Bounds are coverpoint values at FunctionalCoverageVisitor::runWidth(),
// sign-extended like a CrossValueRange's.

वर्ग BinRun final {
सार्वजनिक:
    // MEMBERS
    uint32_t m_मोजणी;  // Number of bins
    V3_संख्या m_lo;  // Lowest value of the first bin
    V3_संख्या m_stride;  // Number of values of each bin but the last
    V3_संख्या m_hi;  // Highest value of the last bin
    तार्किक m_empty = असत्य;  // A single bin without a value of the coverpoint type
    uint32_t m_declared = 0;  // Runtime index of the first bin, once generated

    // CONSTRUCTORS
    BinRun(Ast_गाठ* nodep, पूर्णांक width, uint32_t मोजणी)
        : m_मोजणी{मोजणी}
        , m_lo{nodep, width}
        , m_stride{nodep, width, 1}
        , m_hi{nodep, width} {}
};

//######################################################################
// Functional coverage visitor

वर्ग FunctionalCoverageVisitor final : सार्वजनिक VNVisitor {
    // NODE STATE
    // Entire netlist:
    //  AstCoverpoint::user1p()  -> AstVar*.  Previous-value variable for transition bins
    //  AstCoverpoint::user2()   -> bool.  Had a bins declaration ignored, so no automatic bins
    स्थिरांक VNUser1InUse m_inuser1;
    स्थिरांक VNUser2InUse m_inuser2;

    // STATE
    std::set<AstCoverpoint*>
        m_runtimePoints;  // Points needing value metadata and live-bin mapping
    std::set<AstCoverCross*> m_runtimeCrosses;  // Crosses over finalized live-bin dimensions
    std::map<AstVar*, AstVar*> m_excludedVars;  // Sample-time state-exclusion flags
    AstClass* m_covergroupp = nullptr;  // Current covergroup being processed
    Ast_गाठ_Module* m_unitp = nullptr;  // Design unit declaring the current class, outside classes
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
    तार्किक m_cgMergeInstances = असत्य;  // The covergroup's type_option.merge_instances, a constant
    // The covergroup's type_option.merge_instances is, or SystemVerilog may make it, true
    तार्किक m_cgMayMerge = असत्य;
    // type_option members through which SystemVerilog may set type_option.merge_instances,
    // see CovergroupAssignValidVisitor::mergeableTypeOptions()
    स्थिरांक std::set<स्थिरांक AstVar*>& m_mergeableTypeOptions;
    पूर्णांक m_cgAtLeast = 1;  // Covergroup option.at_least

    संरचना EmbeddedEventTrigger final {
        File_ओळ* eventFl;  // Clocking-event source location
        AstVar* baseVarp;  // Base enclosing-class member in the event expression
        AstVar* memberVarp;  // Selected member in a 'base.member' expression, or nullptr
        VEdgeType edgeType;  // Clocking-event edge qualifier
        AstVar* prevVarp;  // Member containing the previous event value, or nullptr
        EmbeddedEventTrigger(File_ओळ* eventFl, AstVar* baseVarp, AstVar* memberVarp,
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
        uint32_t मोजणी = 0;  // Bins across all runs
        तार्किक unsupported = असत्य;  // Too many bins, or invalid: the declaration is ignored
        // The value of each bin, which names it, of a wildcard array; else the bins are indexed
        std::vector<std::string> मूल्ये;
        // Too many values of an ignore or illegal wildcard array, which is then one bin
        तार्किक single = असत्य;
    };
    संरचना Cross_Bin_मूल्ये final {
        AstCoverBin* binp;  // Declaration owning this Normal bin
        Ast_गाठ_अभिव्यक्ती* valuep;  // Individual array-bin value, or nullptr for a scalar bin
        स्थिरांक BinRun* runp = nullptr;  // Run computing the bin's values, if any
        uint32_t element = 0;  // Index of the bin within runp
    };
    संरचना BinSpan final {
        uint32_t first = 0;  // First Normal index of the bin declaration
        uint32_t मोजणी = 0;  // Number of Normal bins of the declaration
        uint32_t declared = 0;  // First runtime bin index, across all bin kinds
        int32_t sized = -1;  // Index of the sized array, placed at construction; or -1
    };
    संरचना CoverpointBins final {
        uint32_t total = 0;  // Number of Normal bins
        Ast_गाठ_अभिव्यक्ती* exprp = nullptr;  // Sampled expression, for the value domain
        तार्किक crossed = असत्य;  // Feeds a cross, which needs 'values'
        std::vector<Cross_Bin_मूल्ये> मूल्ये;  // Values in runtime Normal-bin index order
        std::unordered_map<std::string, BinSpan> spans;  // Declared bin name -> index span
        BinSpan implicitAuto{0, 0, 0};  // Implicit automatic bins, each named 'auto_<i>'
        std::deque<BinRun> runs;  // Runs 'values' refers to
    };
    std::map<AstVar*, CoverpointBins> m_cpBins;  // Runtime coverpoint -> binsof index ranges
    // Names of the bins declarations each coverpoint ignored, which binsof selects as no bins
    std::map<स्थिरांक AstCoverpoint*, std::vector<std::string>> m_droppedBins;
    // Prefixes of the constructor temporaries of constructed bins, as coverpoint and bin names
    // alone may repeat: coverpoint 'a_' bins 'b', and coverpoint 'a' bins '_b'
    V3UniqueNames m_sizedNames{"__Vsized"};
    std::vector<Ast_गाठ_अभिव्यक्ती*> m_detached_मूल्ये;  // Array-bin values m_cpBins refers to
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
        AstVar* स्थिरांक varp = VN_AS(
            m_memberMap.findMember(m_covergroupp, typeOption ? "type_option" : "option"), Var);
        UASSERT_OBJ(varp, m_covergroupp, "Covergroup missing option member");
        परत varp;
    }

    // 'option.<member>' or 'type_option.<member>', per optionVarp
    AstStructSel* newOptionSel(File_ओळ* fl, AstVar* optionVarp, स्थिरांक std::string& member,
                               VAccess access) {
        स्थिरांक AstMemberDType* स्थिरांक memberp
            = VN_AS(m_memberMap.findMember(optionVarp->dtypep()->skipRefp(), member), MemberDType);
        UASSERT_OBJ(memberp, optionVarp, "Coverage option structure missing '" << member << "'");
        Ast_गाठ_अभिव्यक्ती* स्थिरांक fromp = optionVarp->lifetime().isStatic()
                                       ? नवीन AstVarRef{fl, optionVarp, access}
                                       : memberRef(fl, optionVarp, access);
        AstStructSel* स्थिरांक selp = नवीन AstStructSel{fl, fromp, member};
        selp->dtypep(memberp->subDTypep()->skipRefToEnump());
        selp->didWidth(सत्य);
        परत selp;
    }

    // A constant option.at_least; a negative count of hits has no meaning, so takes the default
    स्थिर पूर्णांक constAtLeast(स्थिरांक AstConst* constp) {
        स्थिरांक पूर्णांक मूल्य = constp->toSInt();
        जर (मूल्य >= 0) परत मूल्य;
        constp->v3warn(COVERIGN, "Ignoring negative 'option.at_least'; using 1");
        परत 1;
    }

    // An option's constant value, or nullptr after a warning, as non-constant is unsupported
    स्थिर स्थिरांक AstConst* constOption(स्थिरांक Ast_गाठ* valuep, VCoverOptionType optType) {
        जर (स्थिरांक AstConst* स्थिरांक constp = VN_CAST(valuep, Const)) परत constp;
        valuep->v3warn(COVERIGN, "Ignoring unsupported: non-constant 'option."
                                     << optType.ascii() << "'; using default value");
        परत nullptr;
    }

    // Store the covergroup-level options (IEEE 1800-2023 19.7) where SystemVerilog and the
    // runtime read them.  The instance options, option.weight and option.get_inst_coverage,
    // are evaluated by the constructor, as are the other instance options; the type options,
    // type_option.weight and type_option.merge_instances, are constant, and initialize the
    // static member.  Without its own type_option.merge_instances, a covergroup has the
    // default of --coverage-merge-instances.  A constant option.at_least is the default of
    // the coverpoints and crosses.
    शून्य lowerCovergroupOptions() {
        तार्किक mergeSet = असत्य;  // The covergroup sets type_option.merge_instances
        m_cgAtLeast = 1;
        साठी (AstCgOptionAssign* स्थिरांक optp : m_cgOptions) {
            जर (optp->optType() == VCoverOptionType::AT_LEAST) {
                जर (स्थिरांक AstConst* स्थिरांक constp = constOption(optp->valuep(), optp->optType())) {
                    m_cgAtLeast = constAtLeast(constp);
                }
                VL_DO_DANGLING(ढकला_Deletep(optp->unlinkFrBack()), optp);
                सुरूठेवा;
            }
            File_ओळ* स्थिरांक fl = optp->fileline();
            // V3Width left the type options constant, and type_option.weight non-negative
            स्थिरांक AstConst* स्थिरांक constp = VN_CAST(optp->valuep(), Const);
            std::string member = "weight";
            जर (optp->optType() == VCoverOptionType::MERGE_INSTANCES) {
                member = "merge_instances";
                m_cgMergeInstances = !constp->अंक().isEqZero();
                mergeSet = सत्य;
            } अन्यथा जर (optp->optType() == VCoverOptionType::GET_INST_COVERAGE) {
                member = "get_inst_coverage";
            } अन्यथा {
                UASSERT_OBJ(optp->optType() == VCoverOptionType::WEIGHT, optp,
                            "Unexpected covergroup option reaching V3Covergroup");
                जर (optp->typeOption()) m_cgTypeWeight = constp->toUInt();
            }
            AstAssign* स्थिरांक assignp = नवीन AstAssign{
                fl, newOptionSel(fl, optionVar(optp->typeOption()), member, VAccess::WRITE),
                optp->valuep()->unlinkFrBack()};
            जर (optp->typeOption()) {
                m_covergroupp->addMembersp(नवीन AstInitialStatic{fl, assignp});
                VL_DO_DANGLING(ढकला_Deletep(optp->unlinkFrBack()), optp);
            } अन्यथा {
                optp->replaceWith(assignp);
                VL_DO_DANGLING(ढकला_Deletep(optp), optp);
            }
        }
        m_cgOptions.clear();
        जर (!mergeSet && v3Global.opt.coverageMergeInstances()) {
            File_ओळ* स्थिरांक fl = m_covergroupp->fileline();
            m_covergroupp->addMembersp(नवीन AstInitialStatic{
                fl, नवीन AstAssign{
                        fl, newOptionSel(fl, optionVar(सत्य), "merge_instances", VAccess::WRITE),
                        नवीन AstConst{fl, AstConst::BitTrue{}}}});
            m_cgMergeInstances = सत्य;
        }
        m_cgMayMerge = m_cgMergeInstances || m_mergeableTypeOptions.मोजणी(optionVar(सत्य));
    }

    // The weight of an item in the coverage database, which merges the instances: its
    // type_option.weight if the covergroup merges them too (IEEE 1800-2023 19.11.3); else its
    // option.weight if a constant, and so of every instance; else its type_option.weight
    uint32_t itemDatabaseWeight(Ast_गाठ* optionsp) स्थिरांक {
        स्थिरांक Ast_गाठ_अभिव्यक्ती* weightp = nullptr;  // The option.weight in effect
        uint32_t typeWeight = 1;
        साठी (Ast_गाठ* nodep = optionsp; nodep; nodep = nodep->nextp()) {
            स्थिरांक AstCoverOption* स्थिरांक optp = VN_AS(nodep, CoverOption);
            जर (!(optp->optType() == VCoverOptionType::WEIGHT)) सुरूठेवा;
            // V3Width left type_option.weight a non-negative constant
            जर (optp->typeOption()) {
                typeWeight = VN_AS(optp->valuep(), Const)->toUInt();
            } अन्यथा {
                weightp = optp->valuep();
            }
        }
        जर (m_cgMergeInstances) परत typeWeight;
        जर (!weightp) परत 1;
        जर (स्थिरांक AstConst* स्थिरांक constp = VN_CAST(weightp, Const)) परत constp->toUInt();
        परत typeWeight;
    }

    // Configure an item's option.weight, its weight in instance coverage (IEEE 1800-2023
    // 19.11), and, if the covergroup may merge its instances, its type_option.weight, its
    // weight in type coverage then (19.11.3).
    शून्य generateItemWeight(File_ओळ* fl, AstVar* itemVarp, Ast_गाठ* optionsp) {
        साठी (Ast_गाठ* nodep = optionsp; nodep; nodep = nodep->nextp()) {
            स्थिरांक AstCoverOption* स्थिरांक optp = VN_AS(nodep, CoverOption);
            जर (!(optp->optType() == VCoverOptionType::WEIGHT)) सुरूठेवा;
            जर (!optp->typeOption()) {
                m_constructorp->addStmtsp(
                    itemCall(fl, itemVarp, VCMethod::COVERGROUP_WEIGHT,
                             {optp->valuep()->clone_वृक्ष(असत्य), file_ओळ_Debug(optp->fileline())})
                        ->makeStmt());
            } अन्यथा जर (m_cgMayMerge) {
                // V3Width left type_option.weight a non-negative constant
                m_constructorp->addStmtsp(itemCall(fl, itemVarp, VCMethod::COVERGROUP_TYPE_WEIGHT,
                                                   {optp->valuep()->clone_वृक्ष(असत्य)})
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
        m_cgMergeInstances = असत्य;
        m_cgMayMerge = असत्य;

        lowerCovergroupOptions();

        // Scan every cross item to record the coverpoints it references (the cross dimensions)
        // and to flag any cross naming a bare variable -- a would-be implicit coverpoint, which
        // Verilator does not synthesize.  An unresolvable item drops only that one cross (with a
        // COVERIGN in generateCrossCode), leaving the rest of the covergroup intact.
        साठी (AstCoverCross* crossp : m_coverCrosses) {
            साठी (Ast_गाठ* itemp = crossp->itemsp(); itemp; itemp = itemp->nextp()) {
                स्थिरांक AstCoverpointRef* स्थिरांक refp = VN_AS(itemp, CoverpointRef);
                जर (refp->exprp()) सुरूठेवा;  // hierarchical ref: dropped in generateCrossCode
                जर (m_coverpointMap.find(refp->name()) == m_coverpointMap.end()) {
                    m_droppedCrosses.घाला(crossp);  // bare variable: drop this cross only
                } अन्यथा {
                    m_crossedCpNames.घाला(refp->name());
                }
            }
        }

        std::vector<Ast_गाठ*> pending;
        std::map<AstCoverpoint*, std::vector<AstCoverCross*>> consumers;
        std::map<AstCoverCross*, std::vector<AstCoverpoint*>> inputs;
        साठी (AstCoverpoint* स्थिरांक cpp : m_coverpoints) {
            checkBinNames(cpp);
            checkConstructedBins(cpp);
            जर (!cpp->exprp()->dtypep()->skipRefp()->isIntegralOrPacked()) सुरूठेवा;
            // Bins without values leave the report (IEEE 1800-2023 19.11.1), exclusions or not.
            // Constructed bins get their values when the covergroup is constructed.
            जर (!coverpointHasStateExclusions(cpp) && !coverpointHasEmptyBins(cpp)
                && !coverpointHasConstructedBins(cpp)) {
                सुरूठेवा;
            }
            m_runtimePoints.घाला(cpp);
            pending.ढकला_back(cpp);
        }
        साठी (AstCoverCross* स्थिरांक crossp : m_coverCrosses) {
            जर (m_droppedCrosses.मोजणी(crossp)) सुरूठेवा;
            साठी (Ast_गाठ* itemp = crossp->itemsp(); itemp; itemp = itemp->nextp()) {
                स्थिरांक AstCoverpointRef* स्थिरांक refp = VN_AS(itemp, CoverpointRef);
                जर (refp->exprp()) सुरूठेवा;
                स्थिरांक स्वतः point = m_coverpointMap.find(refp->name());
                जर (point != m_coverpointMap.end()) {
                    consumers[point->second].ढकला_back(crossp);
                    inputs[crossp].ढकला_back(point->second);
                }
            }
            // The values of a filter's bin tuples are known when the covergroup is constructed
            जर (crossp->exists([](स्थिरांक AstCoverCrossWith*) { परत सत्य; })) {
                m_runtimeCrosses.emplace(crossp);
                pending.ढकला_back(crossp);
            }
        }
        साठी (माप_प्रकार next = 0; next < pending.size(); ++next) {
            जर (AstCoverpoint* स्थिरांक pointp = VN_CAST(pending[next], Coverpoint)) {
                साठी (AstCoverCross* स्थिरांक crossp : consumers[pointp]) {
                    जर (m_runtimeCrosses.emplace(crossp).second) pending.ढकला_back(crossp);
                }
            } अन्यथा {
                साठी (AstCoverpoint* स्थिरांक pointp : inputs[VN_AS(pending[next], CoverCross)]) {
                    जर (pointp->exprp()->dtypep()->skipRefp()->isIntegralOrPacked()
                        && m_runtimePoints.emplace(pointp).second) {
                        pending.ढकला_back(pointp);
                    }
                }
            }
        }

        // The instance node owns this instance's coverpoint/cross runtimes, so it must exist
        // before any of them is created.  Emitted first, ahead of both generate loops.
        generateInstanceAttach();

        // For each coverpoint, generate sampling code
        साठी (AstCoverpoint* cpp : m_coverpoints) generateCoverpointCode(cpp);

        // For each cross, generate sampling code
        साठी (AstCoverCross* crossp : m_coverCrosses) generateCrossCode(crossp);

        // Every cross has been built, so runtime points only need their exclusions from here.
        साठी (AstCoverpoint* स्थिरांक cpp : m_coverpoints) {
            जर (!m_runtimePoints.मोजणी(cpp)) सुरूठेवा;
            m_constructorp->addStmtsp(itemCall(cpp->fileline(), m_cpVarMap.at(cpp->name()),
                                               VCMethod::COVERGROUP_मूल्य_RELEASE)
                                          ->makeStmt());
        }
        साठी (Ast_गाठ_अभिव्यक्ती* valuep : m_detached_मूल्ये) VL_DO_DANGLING(ढकला_Deletep(valuep), valuep);
        m_detached_मूल्ये.clear();

        // Generate coverage computation code (even for empty covergroups).  Bin registration
        // with the coverage database is handled per coverpoint/cross by their runtime
        // registerBins() calls (emitted in generateCoverpoint/generateCross).
        generateCoverageComputationCode();
    }

    स्थिर constexpr माप_प्रकार मूल्य_LIST_ENTRIES = 256;  // Metadata entries per constructor call

    // The value of a constant count, of bins or value tuples: -1 if it is negative, and
    // saturated above the largest limit
    स्थिर int64_t const_मोजणी(स्थिरांक AstConst* constp) {
        स्थिरांक V3_संख्या& अंक = constp->अंक();
        जर (constp->isSigned() && अंक.isNegative()) परत -1;
        परत अंक.mostSetBitP1() > 32 ? INT64_MAX : static_cast<int64_t>(अंक.toUQuad());
    }

    // Whether a count, 'whatp', is integral and, if constant, positive, as the standard's clause
    // 'clausep' requires; else errors
    स्थिर तार्किक check_मोजणी(स्थिरांक Ast_गाठ_अभिव्यक्ती* countp, स्थिरांक अक्षर* whatp, स्थिरांक अक्षर* clausep) {
        स्थिरांक AstConst* स्थिरांक constp = VN_CAST(countp, Const);
        जर (!countp->dtypep()->skipRefp()->isIntegralOrPacked()) {
            countp->v3error(whatp << " must be integral (IEEE 1800-2023 " << clausep << ")");
            परत असत्य;
        } अन्यथा जर (constp && (constp->अंक().isFourState() || const_मोजणी(constp) < 1)) {
            countp->v3error(whatp << " must be >= 1, got "
                                  << (constp->अंक().isFourState() ? constp->अंक().ascii(असत्य)
                                                                  : constp->अंक().toDecimalS())
                                  << " (IEEE 1800-2023 " << clausep << ")");
            परत असत्य;
        }
        परत सत्य;
    }

    // The number of bins requested by a valid 'bins auto[N]', or 0
    स्थिर uint32_t autoBinsRequested(स्थिरांक AstCoverBin* binp) {
        स्थिरांक AstConst* स्थिरांक constp = VN_CAST(binp->arraySizep(), Const);
        जर (!constp) परत 0;
        स्थिरांक int64_t मोजणी = const_मोजणी(constp);
        परत मोजणी < 1 || मोजणी > v3Global.opt.coverageMaxBins() ? 0
                                                                   : static_cast<uint32_t>(मोजणी);
    }

    // True for a 'bins auto[N]' declaration, or the implicit automatic bins of a coverpoint
    स्थिर तार्किक isAutoBins(स्थिरांक AstCoverBin* binp) {
        परत binp->binsType() == VCoverBinsType::BINS_AUTO
               || binp->binsType() == VCoverBinsType::BINS_AUTO_IMPLICIT;
    }

    // True for a sized array of bins, 'bins b[N] = {...}', whose values an integral coverpoint
    // distributes over N bins when the covergroup is constructed (IEEE 1800-2023 19.5.1)
    स्थिर तार्किक isSizedArray(स्थिरांक AstCoverBin* binp) {
        परत binp->arraySizep() && !isAutoBins(binp);
    }

    // The 'with' filter of a bin, if any (IEEE 1800-2023 19.5.1.1)
    स्थिर AstCoverWith* binWith(स्थिरांक AstCoverBin* binp) {
        परत VN_CAST(binp->rangesp(), CoverWith);
    }

    // True for bins whose values the covergroup constructor computes: a sized array, or bins of
    // the values a 'with' filter keeps
    स्थिर तार्किक isConstructedBins(स्थिरांक AstCoverBin* binp) {
        परत isSizedArray(binp) || binWith(binp);
    }

    // The range list of a bin's values, or of the candidates of its 'with' filter; null for all
    // of the coverpoint's values, which a filter of the coverpoint's name has
    स्थिर Ast_गाठ* binRangesp(स्थिरांक AstCoverBin* binp) {
        स्थिरांक AstCoverWith* स्थिरांक withp = binWith(binp);
        जर (!withp) परत binp->rangesp();
        परत VN_IS(withp->subp(), CoverpointRef) ? nullptr : withp->subp();
    }

    // Report and delete a bins declaration of a name that another of its coverpoint has
    शून्य checkBinNames(AstCoverpoint* coverpointp) {
        std::set<std::string> names;
        साठी (Ast_गाठ* nodep = coverpointp->binsp(); nodep;) {
            AstCoverBin* स्थिरांक binp = VN_AS(nodep, CoverBin);
            nodep = nodep->nextp();
            जर (names.emplace(binp->name()).second) सुरूठेवा;
            binp->v3error("Duplicate bin " << binp->prettyNameQ() << " in coverpoint "
                                           << coverpointp->prettyNameQ()
                                           << " (IEEE 1800-2023 3.13)");
            VL_DO_DANGLING(ढकला_Deletep(binp->unlinkFrBack()), binp);
        }
    }

    // Delete an ignored bins declaration, which binsof then selects as no bins
    शून्य dropBins(स्थिरांक AstCoverpoint* coverpointp, AstCoverBin* binp) {
        m_droppedBins[coverpointp].ढकला_back(binp->name());
        VL_DO_DANGLING(ढकला_Deletep(binp->unlinkFrBack()), binp);
    }

    // Check the size of a sized array of bins, which drops an invalid array.  A real coverpoint's
    // are unsupported, and treated as arrays of a bin per value.  False unless it stays sized.
    तार्किक checkBinsArraySize(स्थिरांक AstCoverpoint* coverpointp, AstCoverBin* binp, तार्किक integral) {
        Ast_गाठ_अभिव्यक्ती* स्थिरांक sizep = binp->arraySizep();
        जर (VN_IS(sizep, Unbounded)) {  // A parameter of '$'; see bins_orBraE
            binp->v3error("Bins array size must be integral, not '$' (IEEE 1800-2023 19.5.1)");
        } अन्यथा जर (check_मोजणी(sizep, "Bins array size", "19.5.1")) {
            जर (integral) परत सत्य;
            binp->v3warn(COVERIGN, "Unsupported: 'bins' explicit array size of a real "
                                   "coverpoint (treated as '[]')");
            VL_DO_DANGLING(ढकला_Deletep(sizep->unlinkFrBack()), sizep);
            परत असत्य;
        }
        dropBins(coverpointp, binp);
        परत असत्य;
    }

    // Check the bins of a coverpoint whose values the constructor computes, dropping invalid
    // ones.  A wildcard array of too many ranges of values is ignored, or if ignore or illegal,
    // treated as one bin; filtering too many ranges of values ignores the bins.
    शून्य checkConstructedBins(AstCoverpoint* coverpointp) {
        स्थिरांक तार्किक integral = coverpointp->exprp()->dtypep()->skipRefp()->isIntegralOrPacked();
        साठी (Ast_गाठ* nodep = coverpointp->binsp(); nodep;) {
            AstCoverBin* स्थिरांक binp = VN_AS(nodep, CoverBin);
            nodep = nodep->nextp();
            जर (!isConstructedBins(binp)) सुरूठेवा;
            जर (isSizedArray(binp) && !checkBinsArraySize(coverpointp, binp, integral)) सुरूठेवा;
            जर (!binp->isWildcard()
                || sizedWildcardRuns(binp, coverpointp->exprp())
                       <= v3Global.opt.coverageMaxBins()) {
                जर (binWith(binp) && withCandidatesOver(binp, coverpointp->exprp())) {
                    binp->v3warn(COVERIGN, "Unsupported: 'with' filter of more than 2**32 "
                                           "candidate values; bin "
                                               << binp->prettyNameQ() << " ignored");
                    जर (binp->binsType().binIsNormal()) coverpointp->user2(सत्य);
                    dropBins(coverpointp, binp);
                }
                सुरूठेवा;
            }
            // An ignore or illegal array still excludes or checks its values, as one bin
            स्थिरांक तार्किक single = !binp->binsType().binIsNormal() && !binWith(binp);
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
            जर (single) {
                Ast_गाठ_अभिव्यक्ती* स्थिरांक sizep = binp->arraySizep();
                VL_DO_DANGLING(ढकला_Deletep(sizep->unlinkFrBack()), sizep);
                binp->isArray(असत्य);
                सुरूठेवा;
            }
            जर (binp->binsType().binIsNormal()) coverpointp->user2(सत्य);
            dropBins(coverpointp, binp);
        }
    }

    // True if a 'with' filter would be evaluated for more than 2**32 candidate values, known
    // now for the coverpoint's name, or for a range list of constants: each is evaluated once
    // but for an array 'b[N]', which keeps their order and duplicates (see withBegin())
    स्थिर तार्किक withCandidatesOver(AstCoverBin* binp, Ast_गाठ_अभिव्यक्ती* exprp) {
        स्थिरांक पूर्णांक width = runWidth(exprp);
        std::vector<std::pair<V3_संख्या, V3_संख्या>> runs;
        जर (!binRangesp(binp)) runs = coverpoint_मूल्ये(binp, exprp);
        स्थिरांक स्वतः constant
            = [](स्थिरांक Ast_गाठ* nodep) { परत VN_IS(nodep, Const) || VN_IS(nodep, Unbounded); };
        साठी (Ast_गाठ* rangep = binRangesp(binp); rangep; rangep = rangep->nextp()) {
            स्थिरांक AstInsideRange* स्थिरांक irp = VN_CAST(rangep, InsideRange);
            // Else known when constructed
            जर (irp ? !constant(irp->lhsp()) || !constant(irp->rhsp()) : !VN_IS(rangep, Const)) {
                परत असत्य;
            }
            Cross_मूल्य_Range range{rangep, resolveWidth(rangep, exprp)};
            जर (!resolve_मूल्य(rangep, exprp, सत्य, binp->isWildcard(), range)
                || crossRangeEmpty(range)) {
                सुरूठेवा;
            }
            std::vector<std::pair<V3_संख्या, V3_संख्या>> found{{range.lo, range.hi}};
            जर (range.wildcard) {  // checkConstructedBins bounded the runs
                found.clear();
                crossRangeRuns(range, v3Global.opt.coverageMaxBins(), found);
            }
            साठी (स्थिरांक std::pair<V3_संख्या, V3_संख्या>& run : found) {
                runs.emplace_back(V3_संख्या{rangep, width, run.first},
                                  V3_संख्या{rangep, width, run.second});
            }
        }
        जर (!isSizedArray(binp)) {  // The union of the values
            std::sort(runs.begin(), runs.end(), [](स्थिरांक स्वतः& lhs, स्थिरांक स्वतः& rhs) {
                परत cross_मूल्य_Less(lhs.first, rhs.first);
            });
            std::vector<std::pair<V3_संख्या, V3_संख्या>> merged;
            साठी (स्थिरांक std::pair<V3_संख्या, V3_संख्या>& run : runs) {
                जर (merged.empty() || cross_मूल्य_Less(merged.back().second, run.first)) {
                    merged.ढकला_back(run);
                } अन्यथा जर (cross_मूल्य_Less(merged.back().second, run.second)) {
                    merged.back().second = run.second;
                }
            }
            runs = std::move(merged);
        }
        uint64_t मोजणी = 0;
        साठी (स्थिरांक std::pair<V3_संख्या, V3_संख्या>& run : runs) {
            V3_संख्या span{exprp, width};
            span.opSub(run.second, run.first);
            जर (span.mostSetBitP1() > 32) परत सत्य;  // More than 2**32 values
            मोजणी += span.toUQuad() + 1;
            जर (मोजणी > (uint64_t{1} << 32)) परत सत्य;
        }
        परत असत्य;
    }

    // The ranges of values the wildcard patterns of a sized wildcard array, or of a 'with'
    // filter's candidates, give, counted up to more than --coverage-max-bins
    स्थिर माप_प्रकार sizedWildcardRuns(स्थिरांक AstCoverBin* binp, Ast_गाठ_अभिव्यक्ती* exprp) {
        std::vector<std::pair<V3_संख्या, V3_संख्या>> runs;
        साठी (Ast_गाठ* rangep = binRangesp(binp); rangep; rangep = rangep->nextp()) {
            जर (!VN_IS(rangep, Const)) सुरूठेवा;  // A range, or a value known at construction
            Cross_मूल्य_Range range{rangep, resolveWidth(rangep, exprp)};
            जर (resolve_मूल्य(rangep, exprp, सत्य, सत्य, range) && !crossRangeEmpty(range)) {
                crossRangeRuns(range, v3Global.opt.coverageMaxBins(), runs);
            }
        }
        परत runs.size();
    }

    // Check the automatic bins declarations of a coverpoint.  Each stays one declaration, which
    // generates as a partition of the coverpoint domain (see autoBinRuns).
    शून्य checkAutomaticBins(AstCoverpoint* coverpointp, स्थिरांक Ast_गाठ_अभिव्यक्ती* exprp) {
        साठी (Ast_गाठ* binp = coverpointp->binsp(); binp; binp = binp->nextp()) {
            AstCoverBin* स्थिरांक cbinp = VN_AS(binp, CoverBin);
            जर (cbinp->binsType() != VCoverBinsType::BINS_AUTO) सुरूठेवा;
            स्थिरांक AstConst* स्थिरांक constp = VN_CAST(cbinp->arraySizep(), Const);
            जर (!constp) {
                cbinp->v3error("Automatic bins array size must be a constant");
            } अन्यथा जर (const_मोजणी(constp) < 1) {
                cbinp->v3error("Automatic bins array size must be >= 1, got "
                               << constp->अंक().toDecimalS());
            } अन्यथा जर (const_मोजणी(constp) > v3Global.opt.coverageMaxBins()) {
                cbinp->v3error("Automatic bins array size of "
                               << constp->अंक().toDecimalU() << " exceeds limit of "
                               << v3Global.opt.coverageMaxBins() << '\n'
                               << cbinp->warnMore() << "... Suggest a larger --coverage-max-bins");
            } अन्यथा जर (!exprp->dtypep()->skipRefp()->isIntegralOrPacked()) {
                cbinp->v3error("Automatic bins are not allowed on a coverpoint of a non-integral "
                               "expression (IEEE 1800-2023 19.5.3).");
            }
        }
    }

    // Extract all option values of a coverpoint or cross in a single pass.
    // atLeastOut: option.at_least (default the covergroup's)
    // autoBinMaxOut: option.auto_bin_max (coverpoint overrides covergroup, default 64)
    शून्य extractItemOptions(Ast_गाठ* optionsp, पूर्णांक& atLeastOut, पूर्णांक& autoBinMaxOut) {
        atLeastOut = m_cgAtLeast;
        autoBinMaxOut = -1;  // -1 = not set at coverpoint level
        साठी (Ast_गाठ* optionp = optionsp; optionp; optionp = optionp->nextp()) {
            AstCoverOption* स्थिरांक optp = VN_AS(optionp, CoverOption);
            // Weights may be non-constant; generateItemWeight() handles them.  A cross keeps
            // its unsupported options, which V3LinkParse warned of.
            जर (!(optp->optType() == VCoverOptionType::AT_LEAST
                  || optp->optType() == VCoverOptionType::AUTO_BIN_MAX)) {
                सुरूठेवा;
            }
            स्थिरांक AstConst* स्थिरांक constp = constOption(optp->valuep(), optp->optType());
            जर (!constp) सुरूठेवा;
            जर (optp->optType() == VCoverOptionType::AT_LEAST) {
                atLeastOut = constAtLeast(constp);
            } अन्यथा {
                autoBinMaxOut = constp->toSInt();
            }
        }
        // Fall back to covergroup-level auto_bin_max if not set at coverpoint level
        जर (autoBinMaxOut < 0) {
            जर (m_covergroupp->cgAutoBinMax() >= 0) {
                autoBinMaxOut = m_covergroupp->cgAutoBinMax();
            } अन्यथा {
                autoBinMaxOut = 64;  // Default per IEEE 1800-2023 Table 19-1
            }
        }
    }

    // IEEE 1800-2023 19.5.2: an enum coverpoint has one automatic bin per enumeration value
    शून्य createEnumAutoBins(AstCoverpoint* coverpointp, Ast_गाठ_अभिव्यक्ती* exprp,
                            स्थिरांक AstEnumDType* enump) {
        File_ओळ* स्थिरांक fl = coverpointp->fileline();
        साठी (स्थिरांक AstEnumItem* itemp = enump->itemsp(); itemp;
             itemp = VN_AS(itemp->nextp(), EnumItem)) {
            AstConst* स्थिरांक lop = new_मूल्य_Const(fl, VN_AS(itemp->valuep(), Const)->अंक(), exprp);
            AstInsideRange* स्थिरांक rangep = नवीन AstInsideRange{fl, lop, lop->clone_वृक्ष(असत्य)};
            rangep->dtypeFrom(exprp);
            coverpointp->addBinsp(
                नवीन AstCoverBin{fl, "auto[" + itemp->name() + "]", rangep, असत्य, असत्य});
        }
    }

    // IEEE 1800-2023 19.5.3/19.11.1: partition first, then apply exclusions.  The partition is one
    // automatic bins declaration, generated as a run like 'bins auto[N]' but numbering its bins.
    शून्य createImplicitAutoBins(AstCoverpoint* coverpointp, Ast_गाठ_अभिव्यक्ती* exprp, पूर्णांक autoBinMax) {
        जर (coverpointp->user2()) परत;  // Declared bins, ignored, leave no bins
        साठी (Ast_गाठ* nodep = coverpointp->binsp(); nodep; nodep = nodep->nextp()) {
            स्थिरांक VCoverBinsType kind = VN_AS(nodep, CoverBin)->binsType();
            जर (kind != VCoverBinsType::BINS_IGNORE && kind != VCoverBinsType::BINS_ILLEGAL)
                परत;
        }
        जर (स्थिरांक AstEnumDType* स्थिरांक enump
            = VN_CAST(exprp->dtypep()->skipRefToEnump(), EnumDType)) {
            createEnumAutoBins(coverpointp, exprp, enump);
            परत;
        }
        स्थिरांक पूर्णांक width = exprp->width();
        uint32_t मोजणी = width < 31 ? std::min<uint32_t>(uint32_t{1} << width, autoBinMax)
                                    : static_cast<uint32_t>(autoBinMax);
        जर (!मोजणी) परत;
        जर (!exprp->dtypep()->skipRefp()->isIntegralOrPacked()) {
            coverpointp->v3error("Coverpoint of a non-integral expression requires explicit bins "
                                 "(IEEE 1800-2023 19.5.3).");
            परत;
        }
        जर (मोजणी > v3Global.opt.coverageMaxBins()) {
            coverpointp->v3warn(COVERIGN, "Unsupported: more than "
                                              << v3Global.opt.coverageMaxBins()
                                              << " automatic bins from 'option.auto_bin_max'; "
                                                 "using "
                                              << v3Global.opt.coverageMaxBins() << ".\n"
                                              << coverpointp->warnMore()
                                              << "... Suggest a larger --coverage-max-bins");
            मोजणी = v3Global.opt.coverageMaxBins();
        }
        File_ओळ* स्थिरांक fl = coverpointp->fileline();
        coverpointp->addBinsp(नवीन AstCoverBin{fl, "auto", नवीन AstConst{fl, मोजणी},
                                              VCoverBinsType::BINS_AUTO_IMPLICIT});
    }

    // Sanitize generated names to be valid C++ identifiers
    स्थिर string sanitizeGeneratedName(string name) {
        std::replace(name.begin(), name.end(), '[', '_');
        std::replace(name.begin(), name.end(), ']', '_');
        परत name;
    }

    // Capture an iff guard in a function-local temporary so it is evaluated once per sample()
    AstVarRef* captureIffToTemp(Ast_गाठ_अभिव्यक्ती* iffp, स्थिरांक string& tempName) {
        File_ओळ* स्थिरांक fl = iffp->fileline();
        AstVar* स्थिरांक iffVarp
            = नवीन AstVar{fl, VVarType::BLOCKTEMP, tempName, iffp->findBitDType()};
        iffVarp->funcLocal(सत्य);
        m_sampleFuncp->addStmtsp(iffVarp);
        iffp->unlinkFrBack();
        m_sampleFuncp->addStmtsp(
            नवीन AstAssign{fl, नवीन AstVarRef{fl, iffVarp, VAccess::WRITE}, iffp});
        परत नवीन AstVarRef{fl, iffVarp, VAccess::READ};
    }

    // Add a statement sampling the current coverpoint to sample(), under its iff guard if any
    शून्य addSampleStmt(Ast_गाठ* stmtp) {
        जर (m_cpGuardp) {
            m_cpGuardp->addThensp(stmtp);
        } अन्यथा {
            m_sampleFuncp->addStmtsp(stmtp);
        }
    }

    // Create previous value variable for transition tracking
    AstVar* create_Prev_मूल्य_Var(AstCoverpoint* coverpointp, Ast_गाठ_अभिव्यक्ती* exprp) {
        // Check if already created
        जर (AstVar* स्थिरांक prevVarp = VN_CAST(coverpointp->user1p(), Var)) परत prevVarp;

        // Create variable to store previous sampled value
        स्थिरांक string varName = "__Vprev_" + coverpointp->name();
        AstVar* prevVarp
            = नवीन AstVar{coverpointp->fileline(), VVarType::MEMBER, varName, exprp->dtypep()};
        m_covergroupp->addMembersp(prevVarp);

        UINFO(4, "    Created previous value variable: " << varName);

        // Initialize to zero in constructor
        Ast_गाठ_अभिव्यक्ती* स्थिरांक initExprp
            = नवीन AstConst{prevVarp->fileline(), AstConst::Widthed_मूल्य{}, prevVarp->width(), 0};
        Ast_गाठ_Stmt* स्थिरांक initStmtp = नवीन AstAssign{
            prevVarp->fileline(), नवीन AstVarRef{prevVarp->fileline(), prevVarp, VAccess::WRITE},
            initExprp};
        m_constructorp->addStmtsp(initStmtp);

        coverpointp->user1p(prevVarp);
        परत prevVarp;
    }

    // Create state position variable for multi-value transition bins
    // Tracks position in sequence: 0=not started, 1=seen first item, etc.
    AstVar* createSequenceStateVar(AstCoverpoint* coverpointp, AstCoverBin* binp) {
        // Create variable to track sequence position
        स्थिरांक string varName = "__Vseqpos_" + coverpointp->name() + "_" + binp->name();
        // Use 8-bit integer for state position (sequences rarely > 255 items)
        AstVar* stateVarp
            = नवीन AstVar{binp->fileline(), VVarType::MEMBER, varName, VFlagLogicPacked{}, 8};
        m_covergroupp->addMembersp(stateVarp);

        UINFO(4, "    Created sequence state variable: " << varName);

        // Initialize to 0 (not started) in constructor
        Ast_गाठ_Stmt* स्थिरांक initStmtp = नवीन AstAssign{
            stateVarp->fileline(), नवीन AstVarRef{stateVarp->fileline(), stateVarp, VAccess::WRITE},
            नवीन AstConst{stateVarp->fileline(), AstConst::Widthed_मूल्य{}, 8, 0}};
        m_constructorp->addStmtsp(initStmtp);

        परत stateVarp;
    }

    शून्य generateCoverpointCode(AstCoverpoint* coverpointp) {
        UINFO(4, "  Generating code for coverpoint: " << coverpointp->name());

        // Get the coverpoint expression
        Ast_गाठ_अभिव्यक्ती* exprp = coverpointp->exprp();

        // Check automatic bins before processing
        checkAutomaticBins(coverpointp, exprp);

        // Extract all coverpoint options in a single pass
        पूर्णांक at_Least_मूल्य;
        पूर्णांक autoBinMax;
        extractItemOptions(coverpointp->optionsp(), at_Least_मूल्य, autoBinMax);
        UINFO(6, "    Coverpoint at_least = " << at_Least_मूल्य << " auto_bin_max = " << autoBinMax);

        // Create implicit automatic bins if no regular bins exist
        createImplicitAutoBins(coverpointp, exprp, autoBinMax);

        // A false iff guard disables sampling of the coverpoint (IEEE 1800-2023 19.5), so
        // neither is its expression evaluated, nor are its bins or transition state updated
        VL_RESTORER(m_cpGuardp);
        जर (Ast_गाठ_अभिव्यक्ती* स्थिरांक iffp = coverpointp->iffp()) {
            m_cpGuardp = नवीन AstIf{iffp->fileline(), iffp->unlinkFrBack()};
        }

        AstVar* स्थिरांक मूल्य_Varp = नवीन AstVar{
            coverpointp->fileline(), VVarType::BLOCKTEMP,
            "__VcpValue_" + sanitizeGeneratedName(coverpointp->name()), exprp->dtypep()};
        मूल्य_Varp->funcLocal(सत्य);
        m_sampleFuncp->addStmtsp(मूल्य_Varp);
        exprp->unlinkFrBack();
        addSampleStmt(नवीन AstAssign{
            coverpointp->fileline(),
            नवीन AstVarRef{coverpointp->fileline(), मूल्य_Varp, VAccess::WRITE}, exprp});
        coverpointp->exprp(नवीन AstVarRef{coverpointp->fileline(), मूल्य_Varp, VAccess::READ});
        exprp = coverpointp->exprp();

        // Every coverpoint routes through the VlCoverpoint runtime.  Transition coverpoints are
        // included: their per-value matching is still generated as a state machine in sample()
        // (see generateCoverpoint), but the bin hit is recorded in the runtime bin
        // rather than a bare counter.
        generateCoverpoint(coverpointp, exprp, at_Least_मूल्य);
        // Sampling follows what generateCoverpoint added unguarded, such as clearing the hit list
        जर (m_cpGuardp) m_sampleFuncp->addStmtsp(m_cpGuardp);
    }

    // Build the condition under which a default bin matches: NOT(OR of all normal bins).
    Ast_गाठ_अभिव्यक्ती* buildDefaultCondition(AstCoverpoint* coverpointp, Ast_गाठ_अभिव्यक्ती* exprp,
                                       File_ओळ* fl) {
        Ast_गाठ_अभिव्यक्ती* anyBinMatchp = nullptr;
        साठी (Ast_गाठ* binp = coverpointp->binsp(); binp; binp = binp->nextp()) {
            AstCoverBin* स्थिरांक cbinp = VN_AS(binp, CoverBin);
            जर (cbinp->binsType() == VCoverBinsType::BINS_DEFAULT
                || cbinp->binsType() == VCoverBinsType::BINS_IGNORE
                || cbinp->binsType() == VCoverBinsType::BINS_ILLEGAL)
                सुरूठेवा;
            // The values of constructed bins are known at construction; see emitSizedSample
            जर (isConstructedBins(cbinp)) सुरूठेवा;
            जर (isAutoBins(cbinp)) {
                // Automatic bins partition the whole domain, leaving no default value
                जर (anyBinMatchp) VL_DO_DANGLING(ढकला_Deletep(anyBinMatchp), anyBinMatchp);
                परत नवीन AstConst{fl, AstConst::BitFalse{}};
            }
            Ast_गाठ_अभिव्यक्ती* स्थिरांक binCondp = buildBinCondition(cbinp, exprp);
            UASSERT_OBJ(binCondp, cbinp,
                        "buildBinCondition returned nullptr for non-ignore/non-illegal bin");
            anyBinMatchp = anyBinMatchp ? नवीन AstOr{fl, anyBinMatchp, binCondp} : binCondp;
        }
        परत anyBinMatchp ? static_cast<Ast_गाठ_अभिव्यक्ती*>(नवीन AstNot{fl, anyBinMatchp})
                            : static_cast<Ast_गाठ_अभिव्यक्ती*>(नवीन AstConst{fl, AstConst::BitTrue{}});
    }

    //====================================================================
    // VlCoverpoint conversion

    स्थिर तार्किक coverpointHasStateExclusions(स्थिरांक AstCoverpoint* coverpointp) {
        साठी (स्थिरांक Ast_गाठ* nodep = coverpointp->binsp(); nodep; nodep = nodep->nextp()) {
            स्थिरांक AstCoverBin* स्थिरांक binp = VN_AS(nodep, CoverBin);
            जर (!binp->transp() && binp->rangesp()
                && (binp->binsType() == VCoverBinsType::BINS_IGNORE
                    || binp->binsType() == VCoverBinsType::BINS_ILLEGAL)) {
                परत सत्य;
            }
        }
        परत असत्य;
    }

    स्थिर तार्किक coverpointHasConstructedBins(स्थिरांक AstCoverpoint* coverpointp) {
        साठी (स्थिरांक Ast_गाठ* nodep = coverpointp->binsp(); nodep; nodep = nodep->nextp()) {
            जर (isConstructedBins(VN_AS(nodep, CoverBin))) परत सत्य;
        }
        परत असत्य;
    }

    // True if a Normal state bin, or an array-bin element, has no value of the coverpoint's
    // type (IEEE 1800-2023 19.5.7).  Array ranges enumerate in-type values, so cannot vanish.
    स्थिर तार्किक coverpointHasEmptyBins(स्थिरांक AstCoverpoint* coverpointp) {
        Ast_गाठ_अभिव्यक्ती* स्थिरांक exprp = coverpointp->exprp();
        साठी (Ast_गाठ* nodep = coverpointp->binsp(); nodep; nodep = nodep->nextp()) {
            स्थिरांक AstCoverBin* स्थिरांक binp = VN_AS(nodep, CoverBin);
            जर (!binp->binsType().binIsNormal() || binp->transp() || !binp->rangesp()) सुरूठेवा;
            // A wildcard array has a bin for each value its elements match, so none empty, and
            // the constructor creates no bin without values of a 'with' filter
            जर ((binp->isArray() && binp->isWildcard()) || binWith(binp)) सुरूठेवा;
            तार्किक empty = सत्य;
            साठी (Ast_गाठ* valuep = binp->rangesp(); valuep; valuep = valuep->nextp()) {
                जर (binp->isArray() && VN_IS(valuep, InsideRange)) सुरूठेवा;
                Cross_मूल्य_Range range{valuep, resolveWidth(valuep, exprp)};
                स्थिरांक तार्किक none = resolve_मूल्य(valuep, exprp, सत्य, binp->isWildcard(), range)
                                  && crossRangeEmpty(range);
                जर (binp->isArray() && none) परत सत्य;
                empty &= none;
            }
            जर (empty && !binp->isArray()) परत सत्य;
        }
        परत असत्य;
    }

    // True if a coverpoint has any transition bin.  Used to decide whether sample() emits the
    // end-of-sample previous-value update that transition matching needs.
    स्थिर तार्किक coverpointHasTransition(AstCoverpoint* coverpointp) {
        साठी (Ast_गाठ* binp = coverpointp->binsp(); binp; binp = binp->nextp()) {
            जर (VN_AS(binp, CoverBin)->transp()) परत सत्य;
        }
        परत असत्य;
    }

    // The interned AstBasicDType for one of the covergroup runtime keywords.
    स्थिर AstBasicDType* basicDType(File_ओळ* fl, VBasicDTypeKwd kwd) {
        परत v3Global.rootp()->typeTablep()->findBasicDType(fl, kwd);
    }

    // Get (or create) the coverpoint dtype for a hit-list bound, interned so each distinct bound
    // yields one node.
    AstCoverpointDType* coverpointDType(File_ओळ* fl, uint32_t hitBound) {
        AstCoverpointDType*& typep = m_cpDTypes[hitBound];
        जर (!typep) {
            typep = नवीन AstCoverpointDType{fl, hitBound};
            v3Global.rootp()->typeTablep()->addTypesp(typep);
        }
        परत typep;
    }

    // The key of the covergroup's type in the coverage registry, which get_coverage() queries
    std::string covergroupProtectedName() स्थिरांक {
        परत VIdProtect::protect_शब्द_If(m_covergroupName, v3Global.opt.protectIds());
    }

    // The name of the covergroup's type, which keys the coverage registry and database: as
    // $typename names it (IEEE 1800-2023 20.6.1), so apart for covergroups of distinct scopes and
    // specializations, and alike in each Verilator run of a hierarchical design.  As design units
    // of distinct libraries may share a name, one of a library other than the default is prefixed
    // by its library, as '%l' prints it (IEEE 1800-2023 33.4).
    std::string covergroupTypeName() स्थिरांक {
        UASSERT_OBJ(m_unitp, m_covergroupp, "Covergroup declared outside of a design unit");
        स्थिरांक std::string& libname = m_unitp->libname();
        स्थिरांक std::string name = m_covergroupp->dtypeName(सत्य);
        परत libname == "work" ? name : libname + "." + name;
    }

    // Emit the covergroup's instance handle member and the constructor statement that creates
    // its node in the per-context coverage registry.  Runs before any coverpoint or cross is
    // generated, so their runtimes can be added to the node as they are created.
    शून्य generateInstanceAttach() {
        File_ओळ* स्थिरांक fl = m_covergroupp->fileline();
        // V3LinkParse synthesizes a 'new' for every covergroup; the item generators below already
        // rely on that, and this attach runs even for a covergroup with no coverpoints at all.
        UASSERT_OBJ(m_constructorp, m_covergroupp, "Covergroup missing synthesized constructor");
        m_cgInstVarp = नवीन AstVar{fl, VVarType::MEMBER, "__Vcg_inst",
                                  basicDType(fl, VBasicDTypeKwd::COVERGROUP_INSTHANDLE)};
        m_covergroupp->addMembersp(m_cgInstVarp);

        m_constructorp->addStmtsp(
            itemCall(fl, m_cgInstVarp, VCMethod::COVERGROUP_ATTACH,
                     {ctext(fl, "vlSymsp->_vm_contextp__->covergroupRegistryp()"
                                "->newCovergroupInst("
                                    + quoted(covergroupProtectedName())
                                    + (m_cgMayMerge ? ", true" : ", false") + ")")},
                     /*usePtr=*/असत्य)
                ->makeStmt());
        // The node reads option.weight in place, so procedural assignments take effect
        AstCExpr* स्थिरांक weightAddrp = नवीन AstCExpr{fl, "&"};
        weightAddrp->add(newOptionSel(fl, optionVar(असत्य), "weight", VAccess::READ));
        m_constructorp->addStmtsp(itemCall(fl, m_cgInstVarp, VCMethod::COVERGROUP_LEND_WEIGHT,
                                           {weightAddrp, file_ओळ_Debug(fl)}, /*usePtr=*/असत्य)
                                      ->makeStmt());
    }

    // A '__Vcg_inst.p()-><method>()' call on the covergroup's instance node
    AstCMethodHard* instanceCall(File_ओळ* fl, VCMethod method) {
        // '__Vcg_inst.p()' -- a value handle, so '.' not '->'
        AstCMethodHard* स्थिरांक instp
            = नवीन AstCMethodHard{fl, memberRef(fl, m_cgInstVarp), VCMethod::COVERGROUP_INST_P};
        instp->usePtr(असत्य);
        instp->dtypeSetVoid();  // Opaque receiver; only ever the 'fromp' of the call below
        AstCMethodHard* स्थिरांक callp = नवीन AstCMethodHard{fl, instp, method};
        callp->usePtr(सत्य);
        परत callp;
    }

    // Emit 'this->__Vcp_x = this->__Vcg_inst.p()->addCoverpoint<K>();' (or addCross), which
    // creates the item runtime in the instance node and borrows a pointer to it.
    AstAssign* makeItemCreate(File_ओळ* fl, AstVar* itemVarp, VCMethod method) {
        AstCMethodHard* स्थिरांक createp = instanceCall(fl, method);
        createp->dtypep(itemVarp->dtypep());
        परत नवीन AstAssign{fl, memberRef(fl, itemVarp, VAccess::WRITE), createp};
    }

    // Constant bounds of one rangesp() element (an InsideRange or a single Const).  Each bound is
    // the raw AST node -- an AstConst or an AstUnbounded ('$') -- with the const/unbounded view
    // derived on demand, so there is one source of truth per bound.  After a successful
    // constRangeBounds() neither node is null; a single Const has both bounds aliasing one node.
    संरचना RangeBounds final {
        Ast_गाठ* loNodep = nullptr;  // low bound: AstConst or AstUnbounded
        Ast_गाठ* hiNodep = nullptr;  // high bound: AstConst or AstUnbounded
        तार्किक loUnbounded() स्थिरांक { परत VN_IS(loNodep, Unbounded); }
        तार्किक hiUnbounded() स्थिरांक { परत VN_IS(hiNodep, Unbounded); }
        AstConst* loConstp() स्थिरांक { परत VN_CAST(loNodep, Const); }
        AstConst* hiConstp() स्थिरांक { परत VN_CAST(hiNodep, Const); }
    };

    // Decode one rangesp() element into its constant bounds.  Returns false if rp is neither an
    // InsideRange nor a single Const, or if a present bound is non-constant or 4-state.  '$'
    // bounds are left as AstUnbounded (not resolved) -- the caller applies its own policy.
    // Centralizes the InsideRange/Const/Unbounded decode shared by the hit-list-bound paths.
    स्थिर तार्किक constRangeBounds(Ast_गाठ* rp, RangeBounds& rb) {
        जर (AstInsideRange* स्थिरांक irp = VN_CAST(rp, InsideRange)) {
            rb.loNodep = irp->lhsp();
            rb.hiNodep = irp->rhsp();
        } अन्यथा जर (AstConst* स्थिरांक cp = VN_CAST(rp, Const)) {
            rb.loNodep = rb.hiNodep = cp;
        } अन्यथा {
            परत असत्य;
        }
        // Each bound must be a constant unless it is '$'; reject non-const and 4-state.
        AstConst* स्थिरांक lc = rb.loConstp();
        AstConst* स्थिरांक hc = rb.hiConstp();
        जर ((!lc && !rb.loUnbounded()) || (!hc && !rb.hiUnbounded())) परत असत्य;
        जर ((lc && lc->अंक().isFourState()) || (hc && hc->अंक().isFourState())) परत असत्य;
        परत सत्य;
    }

    // Collect the covered value intervals of a single (non-array) Normal bin.  Returns false
    // if any range isn't a constant/open InsideRange or single Const (e.g. wildcard, non-const).
    स्थिर तार्किक extractRangeIntervals(AstCoverBin* cbinp, uint64_t maxVal,
                                      std::vector<std::pair<uint64_t, uint64_t>>& out) {
        जर (!cbinp->rangesp()) परत असत्य;
        साठी (Ast_गाठ* rp = cbinp->rangesp(); rp; rp = rp->nextp()) {
            RangeBounds rb;
            जर (!constRangeBounds(rp, rb)) परत असत्य;
            जर ((rb.loConstp() && rb.loConstp()->width() > 64)
                || (rb.hiConstp() && rb.hiConstp()->width() > 64)) {
                परत असत्य;  // Use the safe slot-count bound for wide values.
            }
            स्थिरांक uint64_t lo = rb.loUnbounded() ? 0 : rb.loConstp()->toUQuad();
            स्थिरांक uint64_t hi = rb.hiUnbounded() ? maxVal : rb.hiConstp()->toUQuad();
            जर (lo > hi) परत असत्य;
            out.emplace_back(lo, hi);
        }
        परत सत्य;
    }

    // Append one Normal bin's cross-slot interval-sets to `bins` and bump `slotCount` by an upper
    // bound of the bin's slots holding one value.  Returns false if any part isn't statically
    // enumerable (the caller then falls back to the always-safe slot count).  A non-array bin
    // is one slot covering the union of its intervals; the bins of an array element or of an
    // automatic bins declaration hold disjoint values, so the element or declaration counts once.
    तार्किक appendBinCrossSlots(AstCoverBin* cbinp, uint64_t maxVal, Ast_गाठ_अभिव्यक्ती* exprp,
                             std::vector<std::vector<std::pair<uint64_t, uint64_t>>>& bins,
                             पूर्णांक& slot_मोजणी) {
        जर (isAutoBins(cbinp)) {
            ++slot_मोजणी;
            bins.ढकला_back({{0, maxVal}});
            परत exprp->width() <= 64;
        }
        जर (binWith(cbinp)) {
            // A value is in one bin of a filter's, but of a sized array, in one for each range
            // list element holding it; the coverpoint's name is one element
            पूर्णांक elements = 0;
            साठी (स्थिरांक Ast_गाठ* rp = binRangesp(cbinp); rp && isSizedArray(cbinp);
                 rp = rp->nextp()) {
                ++elements;
            }
            slot_मोजणी += std::max(1, elements);
            परत असत्य;
        }
        जर (cbinp->isArray() && cbinp->isWildcard() && !cbinp->arraySizep()) {
            // A value is in at most one bin of a wildcard array: one slot covering its values.
            // Signed values are sign-extended, not unsigned intervals (see computeHitListBound)
            ++slot_मोजणी;
            जर (exprp->isSigned() || exprp->width() > 64) परत असत्य;
            स्थिरांक BinRuns runs = wildcardBinRuns(cbinp, exprp, असत्य);
            जर (runs.unsupported) परत असत्य;
            std::vector<std::pair<uint64_t, uint64_t>> ivs;
            साठी (स्थिरांक BinRun& run : runs.runs) {
                ivs.emplace_back(run.m_lo.toUQuad(), run.m_hi.toUQuad());
            }
            bins.ढकला_back(std::move(ivs));
            परत सत्य;
        }
        जर (cbinp->isArray()) परत appendArrayBinCrossSlots(cbinp, exprp, bins, slot_मोजणी);
        // Non-array bin: one slot covering the union of its intervals.
        ++slot_मोजणी;
        std::vector<std::pair<uint64_t, uint64_t>> ivs;
        जर (cbinp->isWildcard() || !extractRangeIntervals(cbinp, maxVal, ivs)) परत असत्य;
        bins.ढकला_back(std::move(ivs));
        परत सत्य;
    }

    // Append the cross slots of an array Normal bin: each element is a slot covering its
    // values, as at most one of its single-value bins holds a value.  An element holds the
    // values arrayBinRuns() gives it: those of the coverpoint type (IEEE 1800-2023 19.5.7).
    // Elements of a signed coverpoint, non-constant elements, and elements with values beyond
    // 64 bits can't be enumerated, so they count one slot but lose exactness.  Returns false if
    // any element wasn't enumerable to exact values.
    तार्किक appendArrayBinCrossSlots(AstCoverBin* cbinp, Ast_गाठ_अभिव्यक्ती* exprp,
                                  std::vector<std::vector<std::pair<uint64_t, uint64_t>>>& bins,
                                  पूर्णांक& slot_मोजणी) {
        // Signed values resolve sign-extended, not as unsigned intervals, and wildcard patterns
        // are not intervals
        तार्किक exact = !exprp->isSigned() && !cbinp->isWildcard();
        साठी (Ast_गाठ* rp = cbinp->rangesp(); rp; rp = rp->nextp()) {
            ++slot_मोजणी;
            RangeBounds rb;
            Cross_मूल्य_Range range{rp, resolveWidth(rp, exprp)};
            जर (!exact || !constRangeBounds(rp, rb)
                || !resolve_मूल्य(rp, exprp, सत्य, असत्य, range)) {
                exact = असत्य;
                सुरूठेवा;
            }
            जर (crossRangeEmpty(range)) सुरूठेवा;  // Its bin, if any, holds no value
            जर (range.hi.mostSetBitP1() > 64) {
                exact = असत्य;
                सुरूठेवा;
            }
            bins.ढकला_back({{range.lo.toUQuad(), range.hi.toUQuad()}});
        }
        परत exact;
    }

    // Compute the hit-list bound for a coverpoint: the maximum number of Normal
    // bins one sample value can match.  Non-cross-fed coverpoints don't feed a cross, so
    // their hit list is unused -> 1.  Otherwise compute the exact max bin overlap; fall back
    // to the (always-safe) Normal-slot count when any bin isn't statically analyzable.
    पूर्णांक computeHitListBound(AstCoverpoint* coverpointp, Ast_गाठ_अभिव्यक्ती* exprp, तार्किक crossFed) {
        जर (!crossFed) परत 1;
        स्थिरांक पूर्णांक width = exprp->width();
        स्थिरांक uint64_t maxVal = (width >= 64) ? UINT64_MAX : ((1ULL << width) - 1);
        // One entry per Normal bin (cross slot): its covered intervals.
        std::vector<std::vector<std::pair<uint64_t, uint64_t>>> bins;
        पूर्णांक slot_मोजणी = 0;  // == runtime m_normal; the safe fallback bound
        // Unsigned intervals cannot establish overlap between differently sized signed values.
        तार्किक exact = !exprp->isSigned();
        साठी (Ast_गाठ* binp = coverpointp->binsp(); binp; binp = binp->nextp()) {
            AstCoverBin* स्थिरांक cbinp = VN_AS(binp, CoverBin);
            जर (!cbinp->binsType().binIsNormal())
                सुरूठेवा;  // ignore/illegal/default: not hit-listed
            जर (!appendBinCrossSlots(cbinp, maxVal, exprp, bins, slot_मोजणी)) exact = असत्य;
        }
        जर (!exact) परत std::max(1, slot_मोजणी);
        जर (bins.empty()) परत 1;
        // Max overlap occurs at some interval start; count covering bins at each lo.
        std::vector<uint64_t> pts;
        साठी (स्थिरांक स्वतः& b : bins)
            साठी (स्थिरांक स्वतः& iv : b) pts.ढकला_back(iv.first);
        std::sort(pts.begin(), pts.end());
        pts.erase(std::unique(pts.begin(), pts.end()), pts.end());
        पूर्णांक maxOverlap = 1;
        साठी (स्थिरांक uint64_t p : pts) {
            पूर्णांक cnt = 0;
            साठी (स्थिरांक स्वतः& b : bins) {
                साठी (स्थिरांक स्वतः& iv : b) {
                    जर (iv.first <= p && p <= iv.second) {
                        ++cnt;
                        थांबा;  // count each bin at most once
                    }
                }
            }
            जर (cnt > maxOverlap) maxOverlap = cnt;
        }
        परत maxOverlap;
    }

    // A 'this->m_member' reference for embedding in an AstCStmt
    AstVarRef* memberRef(File_ओळ* fl, AstVar* varp, VAccess access = VAccess::READ) {
        AstVarRef* स्थिरांक refp = नवीन AstVarRef{fl, varp, access};
        refp->selfPointer(VSelfPointerText{VSelfPointerText::This{}});
        परत refp;
    }

    // A 'this->m_member-><method>(args...)' call on one member.  usePtr is false only for
    // __Vcg_inst, which is a value handle; the item members are borrowed pointers into the
    // instance node.  Numeric arguments are AstConst; the rest are C++ text that has no AST
    // form (see ctext).
    AstCMethodHard* itemCall(File_ओळ* fl, AstVar* varp, VCMethod method,
                             स्थिरांक std::vector<Ast_गाठ_अभिव्यक्ती*>& args = {}, तार्किक usePtr = सत्य) {
        AstCMethodHard* स्थिरांक callp = नवीन AstCMethodHard{fl, memberRef(fl, varp), method};
        साठी (Ast_गाठ_अभिव्यक्ती* स्थिरांक argp : args) callp->addPinsp(argp);
        callp->usePtr(usePtr);
        callp->dtypeSetVoid();
        परत callp;
    }

    // An unsigned integer argument.
    स्थिर AstConst* cnum(File_ओळ* fl, uint32_t मूल्य) { परत नवीन AstConst{fl, मूल्य}; }

    // A literal C++ argument with no AST equivalent: a 'const char*' string literal (an SV
    // string AstConst emits '"..."s', a std::string temporary the runtime cannot borrow), a
    // VlCovBinKind enum token, a constant selection-word initializer list, a VlFileLineDebug, or
    // a '__V' temporary declared by the enclosing AstCStmt.
    स्थिर AstCExpr* ctext(File_ओळ* fl, स्थिरांक std::string& text) {
        परत नवीन AstCExpr{fl, text};
    }

    // A C++ string literal.  Escapes control characters as the emitter does elsewhere -- bin
    // names and filenames reach the generated code verbatim when --protect-ids is off, and an
    // SV escaped identifier may hold a quote or backslash.
    स्थिर std::string quoted(स्थिरांक std::string& text) {
        परत "\"" + V3OutFormatter::quoteNameControls(text) + "\"";
    }

    // A 'VlFileLineDebug' argument: where the runtime reports an error about fl's construct
    स्थिर AstCExpr* file_ओळ_Debug(File_ओळ* fl) {
        स्थिरांक std::string filename
            = VIdProtect::protectIf(fl->filename(), v3Global.opt.protectIds());
        परत ctext(fl, "VlFileLineDebug{" + quoted(filename) + ", "
                             + std::to_string(fl->lineno()) + "}");
    }

    // Check that an element of an array bin (bins b[] = {values/ranges}) is a two-state
    // constant value or range; false if not, after reporting it if 'report'.
    स्थिर तार्किक checkArrayBinElement(AstCoverBin* arrayBinp, Ast_गाठ* rangep, तार्किक report = सत्य) {
        जर (स्थिरांक AstInsideRange* स्थिरांक irp = VN_CAST(rangep, InsideRange)) {
            स्थिरांक AstConst* स्थिरांक minp = VN_CAST(irp->lhsp(), Const);
            स्थिरांक AstConst* स्थिरांक maxp = VN_CAST(irp->rhsp(), Const);
            जर ((!minp && !VN_IS(irp->lhsp(), Unbounded))
                || (!maxp && !VN_IS(irp->rhsp(), Unbounded))) {
                जर (report) {
                    arrayBinp->v3error("Non-constant expression in array bins range; "
                                       "range bounds must be constants (IEEE 1800-2023 19.5)");
                }
                परत असत्य;
            }
            जर ((minp && minp->अंक().isFourState()) || (maxp && maxp->अंक().isFourState())) {
                जर (report) {
                    arrayBinp->v3error("Four-state (x/z) value in array bins range bound; "
                                       "range bounds must be two-state constants");
                }
                परत असत्य;
            }
        } अन्यथा जर (!VN_IS(rangep, Const)) {
            जर (report) {
                arrayBinp->v3error("Non-constant expression in array bins value list; "
                                   "values must be constants (IEEE 1800-2023 19.5)");
            }
            परत असत्य;
        }
        परत सत्य;
    }

    // Individual equality targets of an array bin (bins b[] = {values/ranges}) of a real
    // coverpoint, in order; integral coverpoints generate array bins as runs (see arrayBinRuns).
    // An open-ended bound ('$', AstUnbounded) resolves to the coverpoint domain: '[lo:$]'
    // covers [lo:maxVal] and '[$:hi]' covers [0:hi].  One target is produced per value; ranges
    // whose resolved size would exceed --coverage-max-real-bins (e.g. an open '[lo:$]') are
    // unsupported -- emits COVERIGN, sets unsupportedOut, yields nothing.
    std::vector<Ast_गाठ_अभिव्यक्ती*> extract_Array_मूल्ये(AstCoverBin* arrayBinp, Ast_गाठ_अभिव्यक्ती* exprp,
                                                 तार्किक& unsupportedOut) {
        unsupportedOut = असत्य;
        स्थिरांक पूर्णांक width = exprp->width();
        स्थिरांक uint64_t maxVal = (width >= 64) ? UINT64_MAX : ((1ULL << width) - 1);
        std::vector<Ast_गाठ_अभिव्यक्ती*> मूल्ये;
        साठी (Ast_गाठ* rangep = arrayBinp->rangesp(); rangep; rangep = rangep->nextp()) {
            rangep = V3Const::constifyEdit(rangep);
            जर (!checkArrayBinElement(arrayBinp, rangep)) परत मूल्ये;
            जर (AstInsideRange* स्थिरांक irp = VN_CAST(rangep, InsideRange)) {
                स्थिरांक तार्किक loUnb = VN_IS(irp->lhsp(), Unbounded);
                स्थिरांक तार्किक hiUnb = VN_IS(irp->rhsp(), Unbounded);
                स्थिरांक uint64_t lo = loUnb ? 0 : VN_AS(irp->lhsp(), Const)->toUQuad();
                स्थिरांक uint64_t hi = hiUnb ? maxVal : VN_AS(irp->rhsp(), Const)->toUQuad();
                जर (hi < lo) सुरूठेवा;  // empty range contributes no bins
                // Guard against a '$'-bounded or otherwise huge range exploding the bin count.
                स्थिरांक uint64_t span = hi - lo;  // == valueCount - 1 (no overflow: hi >= lo)
                जर (span >= v3Global.opt.coverageMaxRealBins()
                    || मूल्ये.size() + span + 1 > v3Global.opt.coverageMaxRealBins()) {
                    arrayBinp->v3warn(COVERIGN,
                                      "Unsupported: array 'bins' of a real coverpoint "
                                      "covering more than "
                                          << v3Global.opt.coverageMaxRealBins() << " values; bin "
                                          << arrayBinp->prettyNameQ() << " ignored.\n"
                                          << arrayBinp->warnMore()
                                          << "... Suggest a larger --coverage-max-real-bins");
                    unsupportedOut = सत्य;
                    साठी (Ast_गाठ_अभिव्यक्ती* स्थिरांक vp : मूल्ये) VL_DO_DANGLING(ढकला_Deletep(vp), vp);
                    मूल्ये.clear();
                    परत मूल्ये;
                }
                साठी (uint64_t v = lo; v <= hi; ++v)
                    मूल्ये.ढकला_back(नवीन AstConst{irp->fileline(), AstConst::Widthed_मूल्य{}, width,
                                                  static_cast<uint32_t>(v)});
            } अन्यथा {
                मूल्ये.ढकला_back(VN_AS(rangep->clone_वृक्ष(असत्य), गाठ_अभिव्यक्ती));
            }
        }
        परत मूल्ये;
    }

    स्थिर पूर्णांक runWidth(स्थिरांक Ast_गाठ_अभिव्यक्ती* exprp) { परत exprp->width() + 1; }

    // Automatic bins partition the coverpoint domain in value order (IEEE 1800-2023 19.5.3): N
    // bins, capped at the number of values, each hold 2^width / N values, and the last bin also
    // holds the remainder.  False for an invalid declaration, already reported.
    तार्किक autoBinRuns(AstCoverBin* binp, Ast_गाठ_अभिव्यक्ती* exprp, BinRuns& out) {
        स्थिरांक uint32_t requested = autoBinsRequested(binp);
        जर (!requested || !exprp->dtypep()->skipRefp()->isIntegralOrPacked()) परत असत्य;
        स्थिरांक पूर्णांक width = exprp->width();
        स्थिरांक पूर्णांक arithmeticWidth = runWidth(exprp);
        स्थिरांक uint32_t मोजणी
            = width < 32
                  ? static_cast<uint32_t>(std::min<uint64_t>(uint64_t{1} << width, requested))
                  : requested;
        BinRun run{binp, arithmeticWidth, मोजणी};
        V3_संख्या total{binp, arithmeticWidth};
        total.setBit(width, 1);
        run.m_stride.opDiv(total, V3_संख्या{binp, arithmeticWidth, मोजणी});
        स्थिरांक Cross_मूल्य_Range domain
            = cross_मूल्य_Domain(binp, width, exprp->isSigned(), arithmeticWidth);
        run.m_lo = domain.lo;
        run.m_hi = domain.hi;
        out.runs.ढकला_back(std::move(run));
        out.मोजणी = मोजणी;
        परत सत्य;
    }

    // The elements of an array bin (bins b[] = {values/ranges}), in order, as runs of
    // single-value bins.  A range holds the values of the coverpoint type it contains (IEEE
    // 1800-2023 19.5.7), while a singleton names a bin even without such a value.  Errors on a
    // non-constant element.  More than --coverage-max-bins bins (e.g. an open '[lo:$]' range over
    // a wide coverpoint) are unsupported -- emits COVERIGN, and sets unsupported.
    BinRuns arrayBinRuns(AstCoverBin* arrayBinp, Ast_गाठ_अभिव्यक्ती* exprp) {
        BinRuns out;
        स्थिरांक पूर्णांक width = runWidth(exprp);
        साठी (Ast_गाठ* rangep = arrayBinp->rangesp(); rangep; rangep = rangep->nextp()) {
            rangep = V3Const::constifyEdit(rangep);
            जर (!checkArrayBinElement(arrayBinp, rangep)) परत out;
            स्थिरांक AstInsideRange* स्थिरांक irp = VN_CAST(rangep, InsideRange);
            Cross_मूल्य_Range range{rangep, resolveWidth(rangep, exprp)};
            तार्किक empty = सत्य;
            जर (!resolve_मूल्य(rangep, exprp, सत्य, असत्य, range)) {
                rangep->v3warn(E_UNSUPPORTED, "Unsupported: non-integral value in a coverage bin "
                                              "of an integral coverpoint.");
            } अन्यथा {
                empty = crossRangeEmpty(range);
            }
            जर (empty && irp) सुरूठेवा;  // A range without values contributes no bins
            BinRun run{rangep, width, 1};
            run.m_empty = empty;
            uint64_t मोजणी = 1;
            जर (!empty) {
                run.m_lo.opAssign(range.lo);
                run.m_hi.opAssign(range.hi);
                V3_संख्या span{rangep, width};
                span.opSub(run.m_hi, run.m_lo);
                // Wider spans exceed any limit
                मोजणी = span.mostSetBitP1() > 32 ? UINT64_MAX : span.toUQuad() + 1;
            }
            जर (मोजणी > v3Global.opt.coverageMaxBins() - out.मोजणी) {
                arrayBinp->v3warn(COVERIGN, "Unsupported: array 'bins' covering more than "
                                                << v3Global.opt.coverageMaxBins()
                                                << " values (e.g. an open '[lo:$]' range over "
                                                   "a wide coverpoint); bin "
                                                << arrayBinp->prettyNameQ() << " ignored\n"
                                                << arrayBinp->warnMore()
                                                << "... Suggest a larger --coverage-max-bins");
                out.runs.clear();
                out.मोजणी = 0;
                out.unsupported = सत्य;
                परत out;
            }
            run.m_मोजणी = static_cast<uint32_t>(मोजणी);
            out.मोजणी += run.m_मोजणी;
            out.runs.ढकला_back(std::move(run));
        }
        परत out;
    }

    // The bins of a wildcard array (wildcard bins b[] = {...}): one for each coverpoint value
    // an element matches (IEEE 1800-2023 19.5.4, 19.5.7), in value order, and named by the value
    // (19.5.1), as runs of single-value bins.  Errors on a non-constant element.  More than
    // --coverage-max-bins values are unsupported -- emits COVERIGN, and sets unsupported, and for
    // an ignore or illegal array, single.  'report' false omits these diagnostics.
    स्थिर BinRuns wildcardBinRuns(AstCoverBin* arrayBinp, Ast_गाठ_अभिव्यक्ती* exprp, तार्किक report) {
        BinRuns out;
        स्थिरांक पूर्णांक width = runWidth(exprp);
        स्थिरांक V3_संख्या one{arrayBinp, width, 1};
        // Disjoint runs of the values, in value order, each value once
        std::vector<std::pair<V3_संख्या, V3_संख्या>> spans;
        uint64_t मोजणी = 0;  // Values of 'spans'
        साठी (Ast_गाठ* rangep = arrayBinp->rangesp(); rangep; rangep = rangep->nextp()) {
            rangep = V3Const::constifyEdit(rangep);
            जर (!checkArrayBinElement(arrayBinp, rangep, report)) {
                out.unsupported = सत्य;
                परत out;
            }
            Cross_मूल्य_Range range{rangep, resolveWidth(rangep, exprp)};
            जर (!resolve_मूल्य(rangep, exprp, सत्य, सत्य, range)) {
                जर (report) {
                    rangep->v3warn(E_UNSUPPORTED, "Unsupported: non-integral value in a "
                                                  "coverage bin of an integral coverpoint.");
                }
                सुरूठेवा;
            }
            जर (crossRangeEmpty(range)) सुरूठेवा;
            std::vector<std::pair<V3_संख्या, V3_संख्या>> found;
            crossRangeRuns(range, v3Global.opt.coverageMaxBins(), found);
            साठी (स्थिरांक std::pair<V3_संख्या, V3_संख्या>& run : found) {
                // Coverpoint values, sign-extended in both widths
                spans.emplace_back(V3_संख्या{rangep, width, run.first},
                                   V3_संख्या{rangep, width, run.second});
            }
            std::sort(spans.begin(), spans.end(), [](स्थिरांक स्वतः& lhs, स्थिरांक स्वतः& rhs) {
                परत cross_मूल्य_Less(lhs.first, rhs.first);
            });
            std::vector<std::pair<V3_संख्या, V3_संख्या>> merged;
            साठी (स्थिरांक std::pair<V3_संख्या, V3_संख्या>& span : spans) {
                // Adjacent or overlapping values join a run; 'first - 1' cannot overflow
                V3_संख्या before{rangep, width};
                before.opSub(span.first, one);
                जर (merged.empty() || cross_मूल्य_Less(merged.back().second, before)) {
                    merged.ढकला_back(span);
                } अन्यथा जर (cross_मूल्य_Less(merged.back().second, span.second)) {
                    merged.back().second = span.second;
                }
            }
            मोजणी = 0;
            साठी (स्थिरांक std::pair<V3_संख्या, V3_संख्या>& span : merged) {
                V3_संख्या size{rangep, width};
                size.opSub(span.second, span.first);
                // Beyond 2^32 values exceed any limit
                मोजणी += size.mostSetBitP1() > 32 ? uint64_t{1} << 33 : size.toUQuad() + 1;
                जर (मोजणी > v3Global.opt.coverageMaxBins()) थांबा;
            }
            spans = std::move(merged);
            जर (मोजणी > v3Global.opt.coverageMaxBins()) {
                // An ignore or illegal array still excludes or checks its values, as one bin
                out.single = !arrayBinp->binsType().binIsNormal();
                जर (report) {
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
                out.unsupported = सत्य;
                परत out;
            }
        }
        साठी (स्थिरांक std::pair<V3_संख्या, V3_संख्या>& span : spans) {
            out.runs.emplace_back(arrayBinp, width, 0);
            BinRun& run = out.runs.back();
            run.m_lo = span.first;
            run.m_hi = span.second;
            // A bin for each value, named by it in the coverpoint's type
            V3_संख्या मूल्य = span.first;
            पर्यंत (सत्य) {
                स्थिरांक V3_संख्या typed{arrayBinp, exprp->width(), मूल्य};
                out.मूल्ये.ढकला_back(exprp->isSigned() ? typed.toDecimalS() : typed.toDecimalU());
                ++run.m_मोजणी;
                जर (मूल्य.isCaseEq(span.second)) थांबा;
                V3_संख्या next{arrayBinp, width};
                मूल्य = next.opAdd(मूल्य, one);
            }
        }
        out.मोजणी = static_cast<uint32_t>(मोजणी);
        परत out;
    }

    // The runs of an automatic bins declaration, or of an array bin of an integral coverpoint.
    // False for other bins, which do not generate as runs, including a wildcard array then one
    // bin (see BinRuns::single).
    तार्किक binRunsFor(AstCoverBin* binp, Ast_गाठ_अभिव्यक्ती* exprp, BinRuns& out) {
        जर (isAutoBins(binp)) {
            जर (!autoBinRuns(binp, exprp, out)) out.unsupported = सत्य;
            परत सत्य;
        }
        जर (binp->isArray() && binp->isWildcard()) {
            जर (!exprp->dtypep()->skipRefp()->isIntegralOrPacked()) {
                Ast_गाठ_अभिव्यक्ती* स्थिरांक falsep = wildcardTypeError(binp, exprp);
                VL_DO_DANGLING(ढकला_Deletep(falsep), falsep);
                out.unsupported = सत्य;
            } अन्यथा {
                out = wildcardBinRuns(binp, exprp, सत्य);
                जर (out.single) {
                    binp->isArray(असत्य);  // Generates as one bin
                    परत असत्य;
                }
            }
            परत सत्य;
        }
        जर (!binp->isArray() || binp->transp() || binp->isWildcard()
            || !exprp->dtypep()->skipRefp()->isIntegralOrPacked()) {
            परत असत्य;
        }
        out = arrayBinRuns(binp, exprp);
        परत सत्य;
    }

    // Emit a 'this->m_cp->addSingleNamer/addArrayNamer(...)' statement for one bin whose first
    // runtime bin index is 'declared'; or with 'valueNames', those naming each bin of an array
    Ast_गाठ_Stmt* makeNamer(AstVar* cpVarp, AstCoverBin* binp, int64_t मोजणी, uint32_t declared,
                           स्थिरांक std::vector<Ast_गाठ_अभिव्यक्ती*>& मूल्ये = {},
                           स्थिरांक std::vector<std::string>& मूल्य_Names = {}) {
        File_ओळ* स्थिरांक fl = binp->fileline();
        CoverpointBins& bins = m_cpBins.at(cpVarp);
        स्थिरांक uint32_t normal_मोजणी
            = binp->binsType().binIsNormal() ? static_cast<uint32_t>(मोजणी < 0 ? 1 : मोजणी) : 0;
        स्थिरांक BinSpan span{bins.total, normal_मोजणी, declared};
        जर (binp->binsType() == VCoverBinsType::BINS_AUTO_IMPLICIT) {
            bins.implicitAuto = span;  // Selected by bin, see implicitAutoBinSpan
        } अन्यथा {
            bins.spans.emplace(binp->name(), span);
        }
        bins.total += normal_मोजणी;
        साठी (uint32_t i = 0; bins.crossed && i < normal_मोजणी; ++i) {
            bins.मूल्ये.ढकला_back({binp, मूल्ये.empty() ? nullptr : मूल्ये[i]});
        }
        // Under --protect-ids the filename and bin name flow into the coverage database
        // verbatim, so obfuscate them exactly as line/toggle coverage points are (whole-
        // unit filename, per-word bin name).  A no-op when --protect-ids is off.
        स्थिरांक तार्किक prot = v3Global.opt.protectIds();
        स्थिरांक तार्किक single = मोजणी < 0;
        जर (!मूल्य_Names.empty()) {
            // A bin of a wildcard array is named by its value (IEEE 1800-2023 19.5.1)
            Ast_गाठ_Stmt* stmtsp = nullptr;
            साठी (स्थिरांक std::string& मूल्य : मूल्य_Names) {
                स्थिरांक std::string name
                    = VIdProtect::protect_शब्द_If(binp->name(), prot) + "[" + मूल्य + "]";
                stmtsp = Ast_गाठ::addNext(
                    stmtsp,
                    itemCall(fl, cpVarp, VCMethod::COVERGROUP_ADD_SINGLE_NAMER,
                             {ctext(fl, binp->binsType().binSetEnum()), ctext(fl, quoted(name)),
                              ctext(fl, quoted(VIdProtect::protectIf(fl->filename(), prot))),
                              cnum(fl, static_cast<uint32_t>(fl->lineno())),
                              cnum(fl, static_cast<uint32_t>(fl->firstColumn()))})
                        ->makeStmt());
            }
            परत stmtsp;
        }
        std::vector<Ast_गाठ_अभिव्यक्ती*> args{ctext(fl, binp->binsType().binSetEnum())};
        जर (!single) args.ढकला_back(cnum(fl, static_cast<uint32_t>(मोजणी)));  // value array bin
        args.ढकला_back(ctext(fl, quoted(VIdProtect::protect_शब्द_If(binp->name(), prot))));
        args.ढकला_back(ctext(fl, quoted(VIdProtect::protectIf(fl->filename(), prot))));
        args.ढकला_back(cnum(fl, static_cast<uint32_t>(fl->lineno())));
        args.ढकला_back(cnum(fl, static_cast<uint32_t>(fl->firstColumn())));
        परत itemCall(fl, cpVarp,
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
    Ast_गाठ_Stmt* makeRuntimeBinHit(File_ओळ* fl, AstVar* cpVarp, Ast_गाठ_अभिव्यक्ती* idxp,
                                   तार्किक isNormal) {
        परत itemCall(fl, cpVarp,
                        isNormal ? VCMethod::COVERGROUP_INCREMENT_BIN
                                 : VCMethod::COVERGROUP_RECORD_HIT,
                        {idxp})
            ->makeStmt();
    }
    Ast_गाठ_Stmt* makeRuntimeBinHit(File_ओळ* fl, स्थिरांक ConvBinTarget& tgt) {
        परत makeRuntimeBinHit(fl, tgt.cpVarp, cnum(fl, static_cast<uint32_t>(tgt.idx)),
                                 tgt.isNormal);
    }

    // The condition under which a bin counts a sample: condp, and the bin's iff, and for a
    // Normal or default state bin, that the value is not excluded
    Ast_गाठ_अभिव्यक्ती* binCondition(AstCoverBin* binp, AstVar* cpVarp, Ast_गाठ_अभिव्यक्ती* condp) {
        File_ओळ* स्थिरांक fl = binp->fileline();
        जर (binp->iffp()) condp = नवीन AstLogAnd{fl, binp->iffp()->clone_वृक्ष(असत्य), condp};
        स्थिरांक स्वतः excluded = m_excludedVars.find(cpVarp);
        जर (excluded != m_excludedVars.end() && !binp->transp()
            && (binp->binsType().binIsNormal()
                || binp->binsType() == VCoverBinsType::BINS_DEFAULT)) {
            condp = नवीन AstLogAnd{
                fl, नवीन AstNot{fl, नवीन AstVarRef{fl, excluded->second, VAccess::READ}}, condp};
        }
        परत condp;
    }

    शून्य emitConvHitIf(AstCoverpoint* coverpointp, AstCoverBin* binp, AstVar* cpVarp,
                       Ast_गाठ_अभिव्यक्ती* idxp, Ast_गाठ_अभिव्यक्ती* condp) {
        File_ओळ* स्थिरांक fl = binp->fileline();
        Ast_गाठ* actionp = makeRuntimeBinHit(fl, cpVarp, idxp, binp->binsType().binIsNormal());
        जर (binp->binsType() == VCoverBinsType::BINS_ILLEGAL) {
            actionp->addNext(makeIllegalBinAction(fl, "Illegal bin " + binp->prettyNameQ()
                                                          + " hit in coverpoint "
                                                          + coverpointp->prettyNameQ()));
        }
        UASSERT_OBJ(m_sampleFuncp, binp, "sample() CFunc not set for coverpoint");
        addSampleStmt(नवीन AstIf{fl, binCondition(binp, cpVarp, condp), actionp, nullptr});
    }

    // Emit the sample() code of sized array 'sized': count the value in its bins holding it when
    // enabled, as other bins are, and for default bins note in matchedp whether any holds it
    शून्य emitSizedSample(AstCoverpoint* coverpointp, AstCoverBin* binp, AstVar* cpVarp,
                         Ast_गाठ_अभिव्यक्ती* exprp, uint32_t sized, AstVar* matchedp) {
        File_ओळ* स्थिरांक fl = binp->fileline();
        Ast_गाठ_अभिव्यक्ती* enabledp = binCondition(binp, cpVarp, नवीन AstConst{fl, AstConst::BitTrue{}});
        UASSERT_OBJ(m_sampleFuncp, binp, "sample() CFunc not set for coverpoint");
        स्थिरांक तार्किक illegal = binp->binsType() == VCoverBinsType::BINS_ILLEGAL;
        जर (illegal) {
            // The illegal action reads the condition too, which is evaluated once, as the guard
            // may have side effects
            AstVar* स्थिरांक varp
                = नवीन AstVar{fl, VVarType::BLOCKTEMP,
                             "__VcpEnabled_" + sanitizeGeneratedName(coverpointp->name()) + "_"
                                 + cvtToStr(sized),
                             binp->findBitDType()};
            varp->funcLocal(सत्य);
            m_sampleFuncp->addStmtsp(varp);
            addSampleStmt(नवीन AstAssign{fl, नवीन AstVarRef{fl, varp, VAccess::WRITE}, enabledp});
            enabledp = नवीन AstVarRef{fl, varp, VAccess::READ};
        }
        AstCMethodHard* स्थिरांक callp
            = itemCall(fl, cpVarp,
                       exprp->isWide() ? VCMethod::COVERGROUP_SIZED_SAMPLE_W
                                       : VCMethod::COVERGROUP_SIZED_SAMPLE,
                       {cnum(fl, sized), exprp->clone_वृक्ष(असत्य), enabledp});
        callp->dtypeSetBit();
        जर (illegal) {
            addSampleStmt(नवीन AstIf{fl, नवीन AstLogAnd{fl, callp, enabledp->clone_वृक्ष(असत्य)},
                                    makeIllegalBinAction(fl, "Illegal bin " + binp->prettyNameQ()
                                                                 + " hit in coverpoint "
                                                                 + coverpointp->prettyNameQ())});
        } अन्यथा जर (matchedp && binp->binsType().binIsNormal()) {
            addSampleStmt(
                नवीन AstAssign{fl, नवीन AstVarRef{fl, matchedp, VAccess::WRITE},
                              नवीन AstOr{fl, नवीन AstVarRef{fl, matchedp, VAccess::READ}, callp}});
        } अन्यथा {
            addSampleStmt(callp->makeStmt());
        }
    }

    // A variable local to the constructor
    AstVar* constructorTemp(File_ओळ* fl, स्थिरांक string& name, Ast_गाठ_DType* dtypep) {
        AstVar* स्थिरांक varp = नवीन AstVar{fl, VVarType::BLOCKTEMP, name, dtypep};
        varp->funcLocal(सत्य);
        m_constructorp->addStmtsp(varp);
        परत varp;
    }

    // Truncate or extend, as its signedness sets, a value to a type
    स्थिर Ast_गाठ_अभिव्यक्ती* resize_मूल्य(Ast_गाठ_अभिव्यक्ती* valuep, Ast_गाठ_DType* dtypep) {
        File_ओळ* स्थिरांक fl = valuep->fileline();
        जर (valuep->width() > dtypep->width()) {
            valuep = नवीन AstSel{fl, valuep, 0, dtypep->width()};
        } अन्यथा जर (valuep->width() < dtypep->width()) {
            valuep = valuep->isSigned()
                         ? static_cast<Ast_गाठ_अभिव्यक्ती*>(नवीन AstExtendS{fl, valuep, dtypep->width()})
                         : नवीन AstExtend{fl, valuep, dtypep->width()};
        }
        valuep->dtypep(dtypep);
        परत valuep;
    }

    // The runtime arguments of a count, which the constructor evaluates once into a temporary:
    // its value, a count wider than 64 bits saturated over any limit, and whether it is positive
    std::pair<Ast_गाठ_अभिव्यक्ती*, Ast_गाठ_अभिव्यक्ती*> new_मोजणी_Args(File_ओळ* fl, स्थिरांक string& name,
                                                       Ast_गाठ_अभिव्यक्ती* exprp) {
        AstVar* स्थिरांक countp = constructorTemp(fl, name, exprp->dtypep());
        m_constructorp->addStmtsp(
            नवीन AstAssign{fl, नवीन AstVarRef{fl, countp, VAccess::WRITE}, exprp->clone_वृक्ष(असत्य)});
        स्थिरांक स्वतः मोजणी_Ref = [&]() { परत नवीन AstVarRef{fl, countp, VAccess::READ}; };
        AstConst* स्थिरांक zerop = नवीन AstConst{fl, AstConst::DTyped{}, countp->dtypep()};
        Ast_गाठ_अभिव्यक्ती* स्थिरांक positivep
            = countp->isSigned() ? static_cast<Ast_गाठ_अभिव्यक्ती*>(नवीन AstGtS{fl, मोजणी_Ref(), zerop})
                                 : नवीन AstNeq{fl, मोजणी_Ref(), zerop};
        Ast_गाठ_अभिव्यक्ती* valuep = resize_मूल्य(मोजणी_Ref(), countp->findUInt64DType());
        जर (countp->width() > VL_QUADSIZE) {
            valuep = नवीन AstCond{
                fl,
                नवीन AstRedOr{
                    fl, नवीन AstSel{fl, मोजणी_Ref(), VL_QUADSIZE, countp->width() - VL_QUADSIZE}},
                नवीन AstConst{fl, AstConst::Unsized64{}, std::numeric_limits<uint64_t>::max()},
                valuep};
            valuep->dtypeSetUInt64();
        }
        परत {valuep, positivep};
    }
    // The runtime arguments of a constant count
    स्थिर std::pair<Ast_गाठ_अभिव्यक्ती*, Ast_गाठ_अभिव्यक्ती*> new_मोजणी_Args(File_ओळ* fl, uint64_t मोजणी) {
        परत {नवीन AstConst{fl, AstConst::Unsized64{}, मोजणी},
                नवीन AstConst{fl, AstConst::BitTrue{}}};
    }

    // The values of a coverpoint, which its name denotes (IEEE 1800-2023 19.5.1.1), as runs in
    // value order: an enumerated type's values (6.19), else all values of its type.  Bounds
    // are at runWidth(), sign-extended like a CrossValueRange's.
    स्थिर std::vector<std::pair<V3_संख्या, V3_संख्या>> coverpoint_मूल्ये(Ast_गाठ* nodep,
                                                                       Ast_गाठ_अभिव्यक्ती* exprp) {
        स्थिरांक पूर्णांक width = runWidth(exprp);
        std::vector<std::pair<V3_संख्या, V3_संख्या>> runs;
        स्थिरांक AstEnumDType* स्थिरांक enump = VN_CAST(exprp->dtypep()->skipRefToEnump(), EnumDType);
        जर (!enump) {
            स्थिरांक Cross_मूल्य_Range domain
                = cross_मूल्य_Domain(nodep, exprp->width(), exprp->isSigned(), width);
            runs.emplace_back(domain.lo, domain.hi);
            परत runs;
        }
        std::vector<V3_संख्या> मूल्ये;
        साठी (स्थिरांक AstEnumItem* itemp = enump->itemsp(); itemp;
             itemp = VN_AS(itemp->nextp(), EnumItem)) {
            स्थिरांक V3_संख्या& अंक = VN_AS(itemp->valuep(), Const)->अंक();
            जर (अंक.isFourState()) सुरूठेवा;  // Not a coverpoint value (19.5.7)
            मूल्ये.emplace_back(nodep, width);
            जर (exprp->isSigned()) {
                मूल्ये.back().opExtendS(अंक, अंक.width());
            } अन्यथा {
                मूल्ये.back().opAssign(अंक);
            }
        }
        std::sort(मूल्ये.begin(), मूल्ये.end(), cross_मूल्य_Less);
        स्थिरांक V3_संख्या one{nodep, width, 1};
        साठी (स्थिरांक V3_संख्या& मूल्य : मूल्ये) {
            V3_संख्या next{nodep, width};
            जर (!runs.empty() && next.opAdd(runs.back().second, one).isCaseEq(मूल्य)) {
                runs.back().second = मूल्य;
            } अन्यथा {
                runs.emplace_back(मूल्य, मूल्य);
            }
        }
        परत runs;
    }

    // Emit the constructor code building the bins 'binp' whose values it computes: the values of
    // each element that are coverpoint values (IEEE 1800-2023 19.5.7), those a 'with' filter
    // keeps (19.5.1.1), then its bins
    शून्य generateConstructedBins(AstCoverpoint* coverpointp, AstCoverBin* binp, AstVar* cpVarp,
                                 Ast_गाठ_अभिव्यक्ती* exprp) {
        File_ओळ* स्थिरांक fl = binp->fileline();
        स्थिरांक string prefix
            = m_sizedNames.get(sanitizeGeneratedName(coverpointp->name() + "__" + binp->name()));
        // The count, and whether it is positive; one of a filter's scalar bin, or bin per value
        स्थिरांक std::pair<Ast_गाठ_अभिव्यक्ती*, Ast_गाठ_अभिव्यक्ती*> मोजणी
            = binp->arraySizep() ? new_मोजणी_Args(fl, prefix + "_count", binp->arraySizep())
                                 : new_मोजणी_Args(fl, 1);
        AstCoverWith* स्थिरांक withp = binWith(binp);
        जर (withp && !binRangesp(binp)) {  // The coverpoint's name: all of its values
            साठी (स्थिरांक std::pair<V3_संख्या, V3_संख्या>& run : coverpoint_मूल्ये(binp, exprp)) {
                m_constructorp->addStmtsp(itemCall(fl, cpVarp,
                                                   exprp->isWide()
                                                       ? VCMethod::COVERGROUP_SIZED_RANGE_W
                                                       : VCMethod::COVERGROUP_SIZED_RANGE,
                                                   {new_मूल्य_Const(fl, run.first, exprp),
                                                    new_मूल्य_Const(fl, run.second, exprp)})
                                              ->makeStmt());
            }
        }
        uint32_t element = 0;
        साठी (Ast_गाठ* rangep = binRangesp(binp); rangep; rangep = rangep->nextp()) {
            जर (VN_IS(rangep, Unbounded)) {  // A parameter of '$'
                binp->v3error("Bins value may not be '$', which may only bound a range "
                              "(IEEE 1800-2023 6.20.7)");
                सुरूठेवा;
            }
            generateSizedElement(cpVarp, binp, rangep, exprp, prefix + "_" + cvtToStr(element++));
        }
        जर (withp) generateWithFilter(binp, withp, cpVarp, exprp, prefix);
        std::vector<Ast_गाठ_अभिव्यक्ती*> args{ctext(fl, binp->binsType().binSetEnum()), मोजणी.first,
                                       मोजणी.second};
        // A filter's bins have the limit it began with
        जर (!withp) args.ढकला_back(cnum(fl, v3Global.opt.coverageMaxBins()));
        स्थिरांक तार्किक prot = v3Global.opt.protectIds();
        args.ढकला_back(ctext(fl, quoted(VIdProtect::protect_शब्द_If(binp->name(), prot))));
        args.ढकला_back(ctext(fl, quoted(VIdProtect::protectIf(fl->filename(), prot))));
        args.ढकला_back(cnum(fl, static_cast<uint32_t>(fl->lineno())));
        args.ढकला_back(cnum(fl, static_cast<uint32_t>(fl->firstColumn())));
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
                            Ast_गाठ_अभिव्यक्ती* exprp, स्थिरांक string& prefix) {
        File_ओळ* स्थिरांक fl = withp->fileline();
        स्थिरांक तार्किक wide = exprp->isWide();
        स्थिरांक string grouping = !binp->isArray()     ? "Single"
                                : binp->arraySizep() ? "Fixed"
                                                     : "Values";
        m_constructorp->addStmtsp(itemCall(fl, cpVarp, VCMethod::COVERGROUP_WITH_BEGIN,
                                           {ctext(fl, "VlCovBinGrouping::" + grouping),
                                            cnum(fl, v3Global.opt.coverageMaxBins())})
                                      ->makeStmt());
        // The candidates count in the coverpoint's width, and the filter reads each as 'item',
        // of the coverpoint's type, so that a filter changing 'item' cannot change the loop
        Ast_गाठ_DType* स्थिरांक मूल्य_DTypep = exprp->dtypep();
        AstVar* स्थिरांक valuep = constructorTemp(fl, prefix + "_value", मूल्य_DTypep);
        AstVar* स्थिरांक lastp = constructorTemp(fl, prefix + "_last", मूल्य_DTypep);
        AstVar* स्थिरांक firstp = constructorTemp(fl, prefix + "_first", मूल्य_DTypep);
        AstVar* स्थिरांक runp = constructorTemp(fl, prefix + "_run", binp->findBitDType());
        AstVar* स्थिरांक morep = constructorTemp(fl, prefix + "_more", binp->findBitDType());
        AstVar* स्थिरांक itemp = withp->itemp()->unlinkFrBack();
        itemp->name(prefix + "_item");
        m_constructorp->addStmtsp(itemp);
        स्थिरांक स्वतः ref = [&](AstVar* varp) { परत नवीन AstVarRef{fl, varp, VAccess::READ}; };
        स्थिरांक स्वतः assign = [&](AstVar* varp, Ast_गाठ_अभिव्यक्ती* rhsp) -> Ast_गाठ* {
            परत नवीन AstAssign{fl, नवीन AstVarRef{fl, varp, VAccess::WRITE}, rhsp};
        };
        स्थिरांक स्वतः flag = [&](AstVar* varp, तार्किक मूल्य) {
            परत assign(varp, मूल्य ? नवीन AstConst{fl, AstConst::BitTrue{}}
                                      : नवीन AstConst{fl, AstConst::BitFalse{}});
        };
        स्थिरांक स्वतः bound = [&](VCMethod narrow, VCMethod wideMethod, AstVar* varp) -> Ast_गाठ* {
            जर (wide) {
                परत itemCall(fl, cpVarp, wideMethod, {नवीन AstVarRef{fl, varp, VAccess::WRITE}})
                    ->makeStmt();
            }
            AstCMethodHard* स्थिरांक callp = itemCall(fl, cpVarp, narrow);
            callp->dtypeSetUInt64();
            परत assign(varp, resize_मूल्य(callp, मूल्य_DTypep));
        };
        स्थिरांक स्वतः keep = [&](Ast_गाठ_अभिव्यक्ती* lop, Ast_गाठ_अभिव्यक्ती* hip) {
            AstCMethodHard* स्थिरांक callp = itemCall(
                fl, cpVarp, wide ? VCMethod::COVERGROUP_WITH_RUN_W : VCMethod::COVERGROUP_WITH_RUN,
                {lop, hip});
            callp->dtypeSetBit();
            परत assign(morep, callp);
        };
        स्थिरांक स्वतः step = [&](तार्किक up) {
            AstConst* स्थिरांक onep = नवीन AstConst{fl, AstConst::Widthed_मूल्य{}, exprp->width(), 1};
            Ast_गाठ_अभिव्यक्ती* स्थिरांक stepp
                = up ? static_cast<Ast_गाठ_अभिव्यक्ती*>(नवीन AstAdd{fl, ref(valuep), onep})
                     : नवीन AstSub{fl, ref(valuep), onep};
            stepp->dtypep(मूल्य_DTypep);
            परत stepp;
        };
        AstLoop* स्थिरांक innerp = नवीन AstLoop{fl};
        innerp->addStmtsp(assign(itemp, ref(valuep)));
        innerp->addStmtsp(नवीन AstIf{
            fl, withp->filterp()->unlinkFrBack(),
            नवीन AstIf{fl, नवीन AstNot{fl, ref(runp)},
                      assign(firstp, ref(valuep))->addNext(flag(runp, सत्य))},
            नवीन AstIf{fl, ref(runp), keep(ref(firstp), step(असत्य))->addNext(flag(runp, असत्य))}});
        innerp->addStmtsp(नवीन AstLoopTest{
            fl, innerp, नवीन AstLogAnd{fl, ref(morep), नवीन AstNeq{fl, ref(valuep), ref(lastp)}}});
        innerp->addStmtsp(assign(valuep, step(सत्य)));
        AstLoop* स्थिरांक outerp = नवीन AstLoop{fl};
        AstCMethodHard* स्थिरांक nextp = itemCall(fl, cpVarp, VCMethod::COVERGROUP_WITH_NEXT);
        nextp->dtypeSetBit();
        outerp->addStmtsp(नवीन AstLoopTest{fl, outerp, nextp});
        outerp->addStmtsp(
            bound(VCMethod::COVERGROUP_WITH_LO, VCMethod::COVERGROUP_WITH_LO_W, valuep));
        outerp->addStmtsp(
            bound(VCMethod::COVERGROUP_WITH_HI, VCMethod::COVERGROUP_WITH_HI_W, lastp));
        outerp->addStmtsp(flag(runp, असत्य));
        outerp->addStmtsp(innerp);
        outerp->addStmtsp(नवीन AstIf{fl, ref(runp), keep(ref(firstp), ref(lastp))});
        m_constructorp->addStmtsp(flag(morep, सत्य));
        m_constructorp->addStmtsp(outerp);
    }

    // Emit 'sizedRange(lo, hi)' for the coverpoint values of an element of bins 'binp', whose
    // values the constructor computes: resolved now if constant, else when constructed by
    // clipping to the coverpoint's values.  A wildcard pattern's values give a range for each
    // run of them.  'prefix' names its temporaries.
    शून्य generateSizedElement(AstVar* cpVarp, स्थिरांक AstCoverBin* binp, Ast_गाठ* rangep,
                              Ast_गाठ_अभिव्यक्ती* exprp, स्थिरांक string& prefix) {
        File_ओळ* स्थिरांक fl = rangep->fileline();
        स्थिरांक VCMethod method = exprp->isWide() ? VCMethod::COVERGROUP_SIZED_RANGE_W
                                                : VCMethod::COVERGROUP_SIZED_RANGE;
        स्थिरांक AstInsideRange* स्थिरांक irp = VN_CAST(rangep, InsideRange);
        Ast_गाठ_अभिव्यक्ती* स्थिरांक lowp = irp ? irp->lhsp() : VN_AS(rangep, गाठ_अभिव्यक्ती);
        Ast_गाठ_अभिव्यक्ती* स्थिरांक highp = irp ? irp->rhsp() : nullptr;
        स्थिरांक स्वतः unbounded
            = [](स्थिरांक Ast_गाठ_अभिव्यक्ती* boundp) { परत !boundp || VN_IS(boundp, Unbounded); };
        स्थिरांक स्वतः constant
            = [&](स्थिरांक Ast_गाठ_अभिव्यक्ती* boundp) { परत unbounded(boundp) || VN_IS(boundp, Const); };
        स्थिरांक स्वतः integral = [&](स्थिरांक Ast_गाठ_अभिव्यक्ती* boundp) {
            परत unbounded(boundp) || boundp->dtypep()->skipRefp()->isIntegralOrPacked();
        };
        जर (constant(lowp) && constant(highp)) {
            स्थिरांक स्वतः fourState = [](स्थिरांक Ast_गाठ_अभिव्यक्ती* boundp) {
                स्थिरांक AstConst* स्थिरांक constp = VN_CAST(boundp, Const);
                परत constp && constp->अंक().isFourState();
            };
            जर (irp && (fourState(lowp) || fourState(highp))) {
                rangep->v3error("Four-state (x/z) value in "
                                << (binp->isArray() ? "array bins" : "bin")
                                << " range bound; range bounds must be two-state constants");
                परत;
            }
            Cross_मूल्य_Range range{rangep, resolveWidth(rangep, exprp)};
            जर (!resolve_मूल्य(rangep, exprp, सत्य, binp->isWildcard(), range)) {
                rangep->v3warn(E_UNSUPPORTED, "Unsupported: non-integral value in a coverage bin "
                                              "of an integral coverpoint.");
            } अन्यथा जर (!crossRangeEmpty(range)) {
                std::vector<std::pair<V3_संख्या, V3_संख्या>> runs{{range.lo, range.hi}};
                जर (range.wildcard) {  // checkConstructedBins bounded the runs
                    runs.clear();
                    crossRangeRuns(range, v3Global.opt.coverageMaxBins(), runs);
                }
                साठी (स्थिरांक std::pair<V3_संख्या, V3_संख्या>& run : runs) {
                    m_constructorp->addStmtsp(itemCall(fl, cpVarp, method,
                                                       {new_मूल्य_Const(fl, run.first, exprp),
                                                        new_मूल्य_Const(fl, run.second, exprp)})
                                                  ->makeStmt());
                }
            }
            परत;
        }
        जर (!integral(lowp) || !integral(highp)) {
            rangep->v3warn(E_UNSUPPORTED, "Unsupported: non-integral value in a coverage bin "
                                          "of an integral coverpoint.");
            परत;
        }
        // Compare values and the coverpoint's domain signed, in a width holding all of them
        स्थिरांक स्वतः boundWidth
            = [&](स्थिरांक Ast_गाठ_अभिव्यक्ती* boundp) { परत unbounded(boundp) ? 0 : boundp->width(); };
        स्थिरांक पूर्णांक width = std::max({exprp->width(), boundWidth(lowp), boundWidth(highp)}) + 1;
        स्थिरांक Cross_मूल्य_Range domain
            = cross_मूल्य_Domain(rangep, exprp->width(), exprp->isSigned(), width);
        AstVar* स्थिरांक lop = constructorTemp(fl, prefix + "_lo", exprp->dtypep());
        lop->dtypeSetBitSized(width, VSigning::SIGNED);
        AstVar* स्थिरांक hip = constructorTemp(fl, prefix + "_hi", lop->dtypep());
        स्थिरांक स्वतः bound = [&](Ast_गाठ_अभिव्यक्ती* boundp, स्थिरांक V3_संख्या& limit) -> Ast_गाठ_अभिव्यक्ती* {
            जर (unbounded(boundp)) {
                AstConst* स्थिरांक limitp = नवीन AstConst{fl, limit};
                limitp->dtypeFrom(lop);
                परत limitp;
            }
            Ast_गाठ_अभिव्यक्ती* valuep = boundp->clone_वृक्ष(असत्य);
            // A value no wider than a signed coverpoint has its type (see crossRangeBound)
            जर (exprp->isSigned() && boundp->width() <= exprp->width()) {
                valuep = resize_मूल्य(valuep, exprp->dtypep());
            }
            परत resize_मूल्य(valuep, lop->dtypep());
        };
        स्थिरांक स्वतः ref = [&](AstVar* varp, VAccess access = VAccess::READ) {
            परत नवीन AstVarRef{fl, varp, access};
        };
        m_constructorp->addStmtsp(
            नवीन AstAssign{fl, ref(lop, VAccess::WRITE), bound(lowp, domain.lo)});
        m_constructorp->addStmtsp(
            नवीन AstAssign{fl, ref(hip, VAccess::WRITE), irp ? bound(highp, domain.hi) : ref(lop)});
        AstConst* स्थिरांक minp = नवीन AstConst{fl, domain.lo};
        AstConst* स्थिरांक maxp = नवीन AstConst{fl, domain.hi};
        minp->dtypeFrom(lop);
        maxp->dtypeFrom(lop);
        AstCond* स्थिरांक lowerp
            = नवीन AstCond{fl, नवीन AstLtS{fl, ref(lop), minp}, minp->clone_वृक्ष(असत्य), ref(lop)};
        AstCond* स्थिरांक upperp
            = नवीन AstCond{fl, नवीन AstGtS{fl, ref(hip), maxp}, maxp->clone_वृक्ष(असत्य), ref(hip)};
        lowerp->dtypeFrom(lop);
        upperp->dtypeFrom(lop);
        m_constructorp->addStmtsp(नवीन AstAssign{fl, ref(lop, VAccess::WRITE), lowerp});
        m_constructorp->addStmtsp(नवीन AstAssign{fl, ref(hip, VAccess::WRITE), upperp});
        m_constructorp->addStmtsp(नवीन AstIf{fl, नवीन AstLteS{fl, ref(lop), ref(hip)},
                                            itemCall(fl, cpVarp, method,
                                                     {resize_मूल्य(ref(lop), exprp->dtypep()),
                                                      resize_मूल्य(ref(hip), exprp->dtypep())})
                                                ->makeStmt()});
    }

    // The runtime index of the bin of a run holding the coverpoint value, which is in the run:
    // declared + (value - lo) / stride, capped at the last bin, which holds any remainder.
    स्थिर Ast_गाठ_अभिव्यक्ती* runBinIndex(File_ओळ* fl, Ast_गाठ_अभिव्यक्ती* exprp, स्थिरांक BinRun& run) {
        जर (run.m_मोजणी == 1) परत cnum(fl, run.m_declared);
        स्थिरांक पूर्णांक width = exprp->width();
        // A run spans at most 2^width values, so offsets in it are unsigned width-bit numbers
        Ast_गाठ_अभिव्यक्ती* indexp
            = नवीन AstSub{fl, exprp->clone_वृक्ष(असत्य), new_मूल्य_Const(fl, run.m_lo, exprp)};
        indexp->dtypeSetBitSized(width, VSigning::UNSIGNED);
        V3_संख्या stride{fl, width, 0};
        stride.opAssign(run.m_stride);
        जर (stride.मोजणी_Ones() != 1) {
            indexp = नवीन AstDiv{fl, indexp, नवीन AstConst{fl, stride}};
        } अन्यथा जर (!stride.isEqOne()) {
            indexp = नवीन AstShiftR{fl, indexp, नवीन AstConst{fl, stride.mostSetBitP1() - 1}};
        }
        // Compare the run's values with those of count bins of stride values, without overflow
        स्थिरांक पूर्णांक extWidth = run.m_lo.width() + 1;
        V3_संख्या lo{fl, extWidth, 0};
        lo.opExtendS(run.m_lo, run.m_lo.width());
        V3_संख्या span{fl, extWidth, 0};
        span.opExtendS(run.m_hi, run.m_hi.width());
        span.opSub(V3_संख्या{span}, lo);
        V3_संख्या covered{fl, extWidth, 0};
        covered.opAssign(run.m_stride);
        covered.opMul(V3_संख्या{covered}, V3_संख्या{fl, extWidth, run.m_मोजणी});
        V3_संख्या hasRemainder{fl, 1, 0};
        जर (!hasRemainder.opGte(span, covered).isEqZero()) {
            AstConst* स्थिरांक lastp = नवीन AstConst{fl, V3_संख्या{fl, width, run.m_मोजणी - 1}};
            indexp = नवीन AstCond{fl, नवीन AstGt{fl, indexp->clone_वृक्ष(असत्य), lastp},
                                 lastp->clone_वृक्ष(असत्य), indexp};
        }
        जर (width < VL_IDATASIZE) {
            indexp = नवीन AstExtend{fl, indexp, VL_IDATASIZE};
        } अन्यथा जर (width > VL_IDATASIZE) {
            indexp = नवीन AstSel{fl, indexp, 0, VL_IDATASIZE};
        }
        परत नवीन AstAdd{fl, cnum(fl, run.m_declared), indexp};
    }

    // Emit the sample() hit of a run of bins, whose code does not grow with its number of bins:
    //   if (iff && lo <= value && value <= hi) m_cp.incrementBin(<runBinIndex>);
    शून्य emitRunHit(AstCoverpoint* coverpointp, AstCoverBin* binp, AstVar* cpVarp,
                    Ast_गाठ_अभिव्यक्ती* exprp, स्थिरांक BinRun& run) {
        File_ओळ* स्थिरांक fl = binp->fileline();
        AstConst* स्थिरांक lop = new_मूल्य_Const(fl, run.m_lo, exprp);
        Ast_गाठ_अभिव्यक्ती* condp = nullptr;
        जर (run.m_lo.isCaseEq(run.m_hi)) {
            condp = नवीन AstEq{fl, exprp->clone_वृक्ष(असत्य), lop};
        } अन्यथा {
            AstConst* स्थिरांक hip = new_मूल्य_Const(fl, run.m_hi, exprp);
            condp = makeRangeCondition(fl, exprp, lop, hip);
            VL_DO_DANGLING(ढकला_Deletep(lop), lop);
            VL_DO_DANGLING(ढकला_Deletep(hip), hip);
        }
        emitConvHitIf(coverpointp, binp, cpVarp, runBinIndex(fl, exprp, run), condp);
    }

    // Emit a transition bin's hit action into sample():
    //   if (cond) { m_cp.incrementBin/recordHit(idx); [illegal: $error; $stop] }
    // Used by the transition generators so a completed sequence records into the runtime bin.
    शून्य addConvTransHitIf(AstCoverpoint* coverpointp, AstCoverBin* binp, स्थिरांक ConvBinTarget& tgt,
                           Ast_गाठ_अभिव्यक्ती* condp) {
        File_ओळ* स्थिरांक fl = binp->fileline();
        Ast_गाठ* actionp = makeRuntimeBinHit(fl, tgt);
        जर (binp->binsType() == VCoverBinsType::BINS_ILLEGAL) {
            actionp->addNext(makeIllegalBinAction(
                fl, "Illegal transition bin " + binp->prettyNameQ() + " hit in coverpoint "
                        + coverpointp->prettyNameQ()));
        }
        UASSERT_OBJ(m_sampleFuncp, binp, "sample() CFunc not set for transition bin");
        addSampleStmt(नवीन AstIf{fl, condp, actionp, nullptr});
    }

    // Route a coverpoint through a VlCoverpoint member: emit the member, its sample()
    // increments, the constructor configuration (init + namers), and registration.
    शून्य generateCoverpoint(AstCoverpoint* coverpointp, Ast_गाठ_अभिव्यक्ती* exprp, पूर्णांक at_Least_मूल्य) {
        File_ओळ* स्थिरांक fl = coverpointp->fileline();
        स्थिरांक तार्किक dynamic = m_runtimePoints.मोजणी(coverpointp);
        UINFO(4, "  Generating VlCoverpoint member: " << coverpointp->name());

        // Size the hit list to the gen-time max bin overlap (1 unless cross-fed with
        // overlapping ranges), so no cross hit is ever dropped and storage is minimal.
        स्थिरांक तार्किक crossFed = m_crossedCpNames.मोजणी(coverpointp->name()) != 0;
        स्थिरांक पूर्णांक hitBound = computeHitListBound(coverpointp, exprp, crossFed);
        UINFO(6, "    Hit-list bound (max bin overlap) = " << hitBound);
        AstVar* स्थिरांक cpVarp = नवीन AstVar{fl, VVarType::MEMBER, "__Vcp_" + coverpointp->name(),
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
        जर (crossFed) {
            UASSERT_OBJ(m_sampleFuncp, coverpointp, "sample() CFunc not set for clearHitList");
            m_sampleFuncp->addStmtsp(
                itemCall(fl, cpVarp, VCMethod::COVERGROUP_CLEAR_HIT_LIST)->makeStmt());
        }
        जर (dynamic && coverpointHasStateExclusions(coverpointp)) {
            AstVar* स्थिरांक excludedp
                = नवीन AstVar{fl, VVarType::BLOCKTEMP,
                             "__VcpExcluded_" + sanitizeGeneratedName(coverpointp->name()),
                             coverpointp->findBitDType()};
            excludedp->funcLocal(सत्य);
            m_sampleFuncp->addStmtsp(excludedp);
            AstCMethodHard* स्थिरांक callp
                = itemCall(fl, cpVarp,
                           exprp->isWide() ? VCMethod::COVERGROUP_मूल्य_EXCLUDED_W
                                           : VCMethod::COVERGROUP_मूल्य_EXCLUDED,
                           {exprp->clone_वृक्ष(असत्य)});
            callp->dtypeSetBit();
            addSampleStmt(नवीन AstAssign{fl, नवीन AstVarRef{fl, excludedp, VAccess::WRITE}, callp});
            m_excludedVars.emplace(cpVarp, excludedp);
        }

        // Walk bins (non-default, then default), assigning sequential indices that match the
        // namer append order; emit sample increments and collect namer statements.  Constructed
        // bins follow them all, placed when the coverpoint is constructed.
        std::vector<Ast_गाठ_Stmt*> namerStmts;
        std::vector<AstCoverBin*> defaultBins;
        std::vector<AstCoverBin*> sizedBins;
        std::vector<std::tuple<AstCoverBin*, uint32_t, Ast_गाठ_अभिव्यक्ती*>> metadata;
        std::vector<स्थिरांक BinRun*> runMetadata;
        uint64_t idx = 0;  // Runtime index of the next bin; 32-bit once checked below
        साठी (Ast_गाठ* binp = coverpointp->binsp(); binp; binp = binp->nextp()) {
            AstCoverBin* स्थिरांक cbinp = VN_AS(binp, CoverBin);
            स्थिरांक पूर्णांक errorsBefore = dynamic ? V3Error::error_मोजणी() : 0;
            जर (cbinp->binsType() == VCoverBinsType::BINS_DEFAULT) {
                defaultBins.ढकला_back(cbinp);
                सुरूठेवा;
            }
            जर (isConstructedBins(cbinp)) {
                UASSERT_OBJ(dynamic, cbinp, "Constructed bins without value metadata");
                BinSpan span;
                span.sized = static_cast<int32_t>(sizedBins.size());
                m_cpBins.at(cpVarp).spans.emplace(cbinp->name(), span);
                sizedBins.ढकला_back(cbinp);
                सुरूठेवा;
            }
            जर (cbinp->transp()) {
                // Transition bin (incl. array transition 'bins t[] = (a=>b),(c=>d)' and
                // illegal_bins/ignore_bins transitions).  All sequences of one transition bin
                // share a bin name and merge in the coverage DB to a single point, so model
                // them as one runtime bin incremented by any matching sequence.  The sequence
                // matching is generated as a state machine, with the hit routed to this bin's
                // runtime slot.
                namerStmts.ढकला_back(makeNamer(cpVarp, cbinp, -1, static_cast<uint32_t>(idx)));
                स्थिरांक ConvBinTarget tgt{cpVarp, static_cast<uint32_t>(idx),
                                        cbinp->binsType().binIsNormal()};
                साठी (Ast_गाठ* sp = cbinp->transp(); sp; sp = sp->nextp())
                    generateSingleTransitionCode(coverpointp, cbinp, exprp, tgt,
                                                 VN_AS(sp, CoverTransSet));
                जर (dynamic && V3Error::error_मोजणी() == errorsBefore) {
                    metadata.emplace_back(cbinp, static_cast<uint32_t>(idx), nullptr);
                }
                ++idx;
                सुरूठेवा;
            }
            BinRuns plan;
            जर (binRunsFor(cbinp, exprp, plan)) {
                // Array elements and automatic bins generate as runs, so neither sample() nor
                // the constructor grows with their number of bins.
                जर (plan.unsupported) {  // bin ignored or invalid; reserve no slot
                    m_droppedBins[coverpointp].ढकला_back(cbinp->name());
                    सुरूठेवा;
                }
                CoverpointBins& bins = m_cpBins.at(cpVarp);
                स्थिरांक uint32_t first_मूल्य = bins.total;
                स्थिरांक uint32_t firstDeclared = static_cast<uint32_t>(idx);
                namerStmts.ढकला_back(
                    makeNamer(cpVarp, cbinp, plan.मोजणी, firstDeclared, {}, plan.मूल्ये));
                साठी (BinRun& run : plan.runs) {
                    run.m_declared = static_cast<uint32_t>(idx);
                    bins.runs.ढकला_back(std::move(run));
                    स्थिरांक BinRun& stored = bins.runs.back();
                    जर (bins.crossed && cbinp->binsType().binIsNormal()) {
                        स्थिरांक uint32_t first = first_मूल्य + stored.m_declared - firstDeclared;
                        साठी (uint32_t element = 0; element < stored.m_मोजणी; ++element) {
                            bins.मूल्ये[first + element].runp = &stored;
                            bins.मूल्ये[first + element].element = element;
                        }
                    }
                    जर (!stored.m_empty) {
                        emitRunHit(coverpointp, cbinp, cpVarp, exprp, stored);
                        जर (dynamic && V3Error::error_मोजणी() == errorsBefore) {
                            runMetadata.ढकला_back(&stored);
                        }
                    }
                    idx += stored.m_मोजणी;
                }
                सुरूठेवा;
            }
            जर (cbinp->isArray()) {  // value array of a real coverpoint: b[0]..b[N-1]
                // Only integral coverpoints have runtime value metadata (m_runtimePoints)
                UASSERT_OBJ(!dynamic, cbinp, "Runtime value metadata for a real coverpoint");
                तार्किक unsupported = असत्य;
                std::vector<Ast_गाठ_अभिव्यक्ती*> मूल्ये = extract_Array_मूल्ये(cbinp, exprp, unsupported);
                जर (unsupported) {  // bin ignored (COVERIGN emitted); reserve no slot
                    m_droppedBins[coverpointp].ढकला_back(cbinp->name());
                    सुरूठेवा;
                }
                namerStmts.ढकला_back(makeNamer(cpVarp, cbinp, static_cast<int64_t>(मूल्ये.size()),
                                               static_cast<uint32_t>(idx), मूल्ये));
                साठी (Ast_गाठ_अभिव्यक्ती* valuep : मूल्ये) {
                    // The cross selections of this covergroup still read the value.
                    m_detached_मूल्ये.ढकला_back(valuep);
                    emitConvHitIf(coverpointp, cbinp, cpVarp,
                                  cnum(cbinp->fileline(), static_cast<uint32_t>(idx)),
                                  build_मूल्य_Condition(cbinp, exprp, valuep));
                    ++idx;
                }
            } अन्यथा {
                namerStmts.ढकला_back(makeNamer(cpVarp, cbinp, -1, static_cast<uint32_t>(idx)));
                // buildBinCondition is null for 'ignore_bins = default' (no ranges); the bin
                // still gets a reserved slot (recorded, never incremented).
                जर (Ast_गाठ_अभिव्यक्ती* स्थिरांक condp = buildBinCondition(cbinp, exprp))
                    emitConvHitIf(coverpointp, cbinp, cpVarp,
                                  cnum(cbinp->fileline(), static_cast<uint32_t>(idx)), condp);
                जर (dynamic && V3Error::error_मोजणी() == errorsBefore) {
                    metadata.emplace_back(cbinp, static_cast<uint32_t>(idx), nullptr);
                }
                ++idx;
            }
        }
        // A cross selecting an ignored bins declaration selects no bins
        साठी (स्थिरांक std::string& name : m_droppedBins[coverpointp]) {
            m_cpBins.at(cpVarp).spans.emplace(name, BinSpan{});
        }
        // A value a constructed bin holds is no default bin's; only sampling tells which do
        AstVar* sizedMatchedp = nullptr;
        जर (!defaultBins.empty()
            && std::any_of(sizedBins.begin(), sizedBins.end(), [](स्थिरांक AstCoverBin* binp) {
                   परत binp->binsType().binIsNormal();
               })) {
            sizedMatchedp = नवीन AstVar{fl, VVarType::BLOCKTEMP,
                                       "__VcpSized_" + sanitizeGeneratedName(coverpointp->name()),
                                       coverpointp->findBitDType()};
            sizedMatchedp->funcLocal(सत्य);
            m_sampleFuncp->addStmtsp(sizedMatchedp);
            addSampleStmt(नवीन AstAssign{fl, नवीन AstVarRef{fl, sizedMatchedp, VAccess::WRITE},
                                        नवीन AstConst{fl, AstConst::BitFalse{}}});
        }
        साठी (uint32_t sized = 0; sized < sizedBins.size(); ++sized) {
            emitSizedSample(coverpointp, sizedBins[sized], cpVarp, exprp, sized, sizedMatchedp);
        }
        साठी (AstCoverBin* स्थिरांक defBinp : defaultBins) {
            File_ओळ* स्थिरांक dfl = defBinp->fileline();
            namerStmts.ढकला_back(makeNamer(cpVarp, defBinp, -1, static_cast<uint32_t>(idx)));
            Ast_गाठ_अभिव्यक्ती* condp = buildDefaultCondition(coverpointp, exprp, dfl);
            जर (sizedMatchedp) {
                condp = नवीन AstLogAnd{
                    dfl, नवीन AstNot{dfl, नवीन AstVarRef{dfl, sizedMatchedp, VAccess::READ}}, condp};
            }
            emitConvHitIf(coverpointp, defBinp, cpVarp, cnum(dfl, static_cast<uint32_t>(idx)),
                          condp);
            ++idx;
        }
        जर (idx > std::numeric_limits<uint32_t>::max()) {
            // The runtime indexes bins with 32 bits; stop before generating a model
            coverpointp->v3warn(E_UNSUPPORTED, "Unsupported: coverpoint with more than "
                                                   << std::numeric_limits<uint32_t>::max()
                                                   << " bins");
        }

        // Transition coverpoints track the previous sampled value; update it once at the end of
        // this coverpoint's sample() contribution (the prev var was created on demand by the
        // transition matching above).
        जर (coverpointHasTransition(coverpointp)) {
            AstVar* स्थिरांक prevVarp = VN_AS(coverpointp->user1p(), Var);
            addSampleStmt(
                नवीन AstAssign{coverpointp->fileline(),
                              नवीन AstVarRef{prevVarp->fileline(), prevVarp, VAccess::WRITE},
                              exprp->clone_वृक्ष(असत्य)});
        }

        // Constructor: init (allocates), namers, then registration (under --coverage).
        // Under --protect-ids the hierarchy and page string reach the coverage database
        // verbatim, so obfuscate them like line/toggle points (per-word hierarchy, whole-
        // unit page).  No-ops when --protect-ids is off.
        स्थिरांक तार्किक prot = v3Global.opt.protectIds();
        स्थिरांक std::string hier
            = VIdProtect::protect_शब्द_If(m_covergroupName + "." + coverpointp->name(), prot);
        m_constructorp->addStmtsp(
            itemCall(fl, cpVarp, VCMethod::COVERGROUP_INIT,
                     {ctext(fl, quoted(hier)), cnum(fl, static_cast<uint32_t>(at_Least_मूल्य)),
                      cnum(fl, static_cast<uint32_t>(idx))})
                ->makeStmt());
        साठी (Ast_गाठ_Stmt* स्थिरांक ns : namerStmts) m_constructorp->addStmtsp(ns);
        जर (dynamic) {
            m_constructorp->addStmtsp(
                itemCall(fl, cpVarp, VCMethod::COVERGROUP_मूल्य_TYPE,
                         {cnum(fl, exprp->width()), cnum(fl, exprp->isSigned())})
                    ->makeStmt());
            मूल्य_Lists lists;
            साठी (स्थिरांक स्वतः& entry : metadata) {
                collect_मूल्य_Metadata(lists, exprp, std::get<0>(entry), std::get<1>(entry),
                                     std::get<2>(entry));
            }
            साठी (स्थिरांक BinRun* स्थिरांक runp : runMetadata) collectRunMetadata(lists, exprp, *runp);
            emit_मूल्य_List(fl, cpVarp, VCMethod::COVERGROUP_मूल्य_RANGES, lists.m_ranges);
            emit_मूल्य_List(fl, cpVarp, VCMethod::COVERGROUP_मूल्य_RUNS, lists.m_runs);
            emit_मूल्य_List(fl, cpVarp, VCMethod::COVERGROUP_मूल्य_PATTERNS, lists.m_patterns);
            emit_मूल्य_List(fl, cpVarp, VCMethod::COVERGROUP_मूल्य_TRANSITIONS, lists.m_transitions);
            साठी (AstCoverBin* स्थिरांक binp : sizedBins) {
                generateConstructedBins(coverpointp, binp, cpVarp, exprp);
            }
            m_constructorp->addStmtsp(
                itemCall(fl, cpVarp, VCMethod::COVERGROUP_मूल्य_FINALIZE)->makeStmt());
        }
        जर (v3Global.opt.coverage()) {
            स्थिरांक std::string page
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
    शून्य generate_Multi_मूल्य_Transition_Code(AstCoverpoint* coverpointp, AstCoverBin* binp,
                                          Ast_गाठ_अभिव्यक्ती* exprp, स्थिरांक ConvBinTarget& tgt,
                                          स्थिरांक std::vector<AstCoverTransItem*>& वस्तुसंच) {
        UINFO(4, "    Generating multi-value transition state machine for: " << binp->name());
        UINFO(4, "      Sequence length: " << वस्तुसंच.size() << " items");

        // Create state position variable
        AstVar* स्थिरांक stateVarp = createSequenceStateVar(coverpointp, binp);

        // Build case statement with N cases (one for each state 0 to N-1)
        // State 0: Not started, looking for first item
        // State 1 to N-1: In progress, looking for next item

        AstCase* स्थिरांक casep
            = नवीन AstCase{binp->fileline(), VCaseType::CT_CASE,
                          नवीन AstVarRef{stateVarp->fileline(), stateVarp, VAccess::READ}, nullptr};

        // Generate each case item in the switch statement
        साठी (माप_प्रकार state = 0; state < वस्तुसंच.size(); ++state) {
            AstCaseItem* caseItemp = generateTransitionStateCase(coverpointp, binp, exprp, tgt,
                                                                 stateVarp, वस्तुसंच, state);
            casep->addItemsp(caseItemp);
        }

        // Add default case (reset to state 0) to prevent CASEINCOMPLETE warnings,
        // since the state variable is wider than the number of valid states.
        AstCaseItem* स्थिरांक defaultItemp = नवीन AstCaseItem{
            binp->fileline(), nullptr,
            नवीन AstAssign{binp->fileline(),
                          नवीन AstVarRef{binp->fileline(), stateVarp, VAccess::WRITE},
                          नवीन AstConst{binp->fileline(), AstConst::Widthed_मूल्य{}, 8, 0}}};
        casep->addItemsp(defaultItemp);

        addSampleStmt(casep);
        UINFO(4, "      Successfully added multi-value transition state machine");
    }

    // Generate code for a single state in the transition state machine
    // Returns the case item for this state
    AstCaseItem* generateTransitionStateCase(AstCoverpoint* coverpointp, AstCoverBin* binp,
                                             Ast_गाठ_अभिव्यक्ती* exprp, स्थिरांक ConvBinTarget& tgt,
                                             AstVar* stateVarp,
                                             स्थिरांक std::vector<AstCoverTransItem*>& वस्तुसंच,
                                             माप_प्रकार state) {
        File_ओळ* स्थिरांक fl = binp->fileline();

        // Build condition for current value matching expected item at this state
        Ast_गाठ_अभिव्यक्ती* स्थिरांक matchCondp = buildTransitionItemCondition(वस्तुसंच[state], exprp);

        Ast_गाठ_Stmt* matchActionp = nullptr;

        जर (state == वस्तुसंच.size() - 1) {
            // Last state: sequence complete!  Record the hit in the runtime VlCoverpoint.
            matchActionp = makeRuntimeBinHit(fl, tgt);

            // For illegal_bins, add error message
            जर (binp->binsType() == VCoverBinsType::BINS_ILLEGAL) {
                स्थिरांक string err_संदेश = "Illegal transition bin " + binp->prettyNameQ()
                                      + " hit in coverpoint " + coverpointp->prettyNameQ();
                matchActionp = matchActionp->addNext(makeIllegalBinAction(fl, err_संदेश));
            }

            // Reset state to 0
            matchActionp = matchActionp->addNext(
                नवीन AstAssign{fl, नवीन AstVarRef{fl, stateVarp, VAccess::WRITE},
                              नवीन AstConst{fl, AstConst::Widthed_मूल्य{}, 8, 0}});
        } अन्यथा {
            // Intermediate state: advance to next state
            matchActionp = नवीन AstAssign{
                fl, नवीन AstVarRef{fl, stateVarp, VAccess::WRITE},
                नवीन AstConst{fl, AstConst::Widthed_मूल्य{}, 8, static_cast<uint32_t>(state + 1)}};
        }

        // Build restart logic: check if current value matches first item
        // If so, restart sequence from state 1 (even if we're in middle of sequence)
        Ast_गाठ_Stmt* noMatchActionp = nullptr;
        जर (state > 0) {
            // Check if current value matches first item (restart condition)
            Ast_गाठ_अभिव्यक्ती* स्थिरांक restartCondp = buildTransitionItemCondition(वस्तुसंच[0], exprp);

            UASSERT_OBJ(restartCondp, वस्तुसंच[0],
                        "buildTransitionItemCondition returned nullptr for restart");

            // Restart to state 1
            Ast_गाठ_Stmt* restartActionp
                = नवीन AstAssign{fl, नवीन AstVarRef{fl, stateVarp, VAccess::WRITE},
                                नवीन AstConst{fl, AstConst::Widthed_मूल्य{}, 8, 1}};

            // Reset to state 0 (else branch)
            Ast_गाठ_Stmt* resetActionp
                = नवीन AstAssign{fl, नवीन AstVarRef{fl, stateVarp, VAccess::WRITE},
                                नवीन AstConst{fl, AstConst::Widthed_मूल्य{}, 8, 0}};

            noMatchActionp = नवीन AstIf{fl, restartCondp, restartActionp, resetActionp};
        }
        // For state 0, no action needed if no match (stay in state 0)

        // Combine into if-else
        Ast_गाठ_Stmt* स्थिरांक stmtp = नवीन AstIf{fl, matchCondp, matchActionp, noMatchActionp};

        // Create case item for this state value
        AstCaseItem* स्थिरांक caseItemp = नवीन AstCaseItem{
            fl, नवीन AstConst{fl, AstConst::Widthed_मूल्य{}, 8, static_cast<uint32_t>(state)},
            stmtp};

        परत caseItemp;
    }

    // Create: $error(msg); $stop;  Used when an illegal bin is hit.
    Ast_गाठ_Stmt* makeIllegalBinAction(File_ओळ* fl, स्थिरांक string& err_संदेश) {
        AstDisplay* स्थिरांक errorp
            = नवीन AstDisplay{fl, VDisplayType::DT_ERROR, err_संदेश, nullptr, nullptr};
        errorp->fmtp()->timeunit(m_covergroupp->timeunit());
        static_cast<Ast_गाठ*>(errorp)->addNext(नवीन AstStop{fl, सत्य});
        परत errorp;
    }

    // Preserve the coverpoint's width and signedness after V3Width.
    स्थिर AstConst* new_मूल्य_Const(File_ओळ* fl, स्थिरांक V3_संख्या& मूल्य, स्थिरांक Ast_गाठ_अभिव्यक्ती* exprp) {
        V3_संख्या narrowed{fl, exprp->width(), 0};
        narrowed.opAssign(मूल्य);
        AstConst* स्थिरांक constp = नवीन AstConst{fl, narrowed};
        constp->dtypeFrom(exprp);
        परत constp;
    }

    // A real copy of a real or integral range bound, for comparing with a real coverpoint.
    स्थिर AstConst* newRealConst(AstConst* constp) {
        जर (constp->अंक().isDouble()) परत constp->clone_वृक्ष(असत्य);
        V3_संख्या real{&constp->अंक(), 64};
        real.opIToRD(constp->अंक(), constp->isSigned());
        परत नवीन AstConst{constp->fileline(), real};
    }

    // Clone a constant node, widening to targetWidth if needed (zero-extend).
    // Used to ensure comparisons use matching widths after V3Width has run.
    स्थिर AstConst* widenConst(File_ओळ* fl, AstConst* constp, पूर्णांक targetWidth) {
        जर (constp->width() == targetWidth) परत constp->clone_वृक्ष(असत्य);
        V3_संख्या अंक{fl, targetWidth, 0};
        अंक.opAssign(constp->अंक());
        परत नवीन AstConst{fl, अंक};
    }

    // Build a range condition: minp <= exprp <= maxp.
    // Uses signed comparisons if exprp is signed; omits trivially-true domain bounds.
    // All arguments are non-owning; clones exprp/minp/maxp as needed.
    Ast_गाठ_अभिव्यक्ती* makeRangeCondition(File_ओळ* fl, Ast_गाठ_अभिव्यक्ती* exprp, Ast_गाठ_अभिव्यक्ती* minp,
                                    Ast_गाठ_अभिव्यक्ती* maxp) {
        स्थिरांक पूर्णांक अभिव्यक्ती_Width = exprp->widthMin();
        AstConst* स्थिरांक minConstp = VN_AS(minp, Const);
        AstConst* स्थिरांक maxConstp = VN_AS(maxp, Const);
        जर (exprp->isDouble()) {
            // A real coverpoint has no finite domain bounds to omit.
            परत नवीन AstAnd{fl,
                              नवीन AstGteD{fl, exprp->clone_वृक्ष(असत्य), newRealConst(minConstp)},
                              नवीन AstLteD{fl, exprp->clone_वृक्ष(असत्य), newRealConst(maxConstp)}};
        }
        // Widen constants to match expression width so post-V3Width nodes use correct macros
        AstConst* स्थिरांक minWidep = widenConst(fl, minConstp, अभिव्यक्ती_Width);
        AstConst* स्थिरांक maxWidep = widenConst(fl, maxConstp, अभिव्यक्ती_Width);
        V3_संख्या minimum{fl, अभिव्यक्ती_Width, 0};
        V3_संख्या maximum{fl, अभिव्यक्ती_Width, 0};
        maximum.setAllBits1();
        जर (exprp->isSigned()) {
            minimum.setBit(अभिव्यक्ती_Width - 1, 1);
            maximum.setBit(अभिव्यक्ती_Width - 1, 0);
        }
        Ast_गाठ_अभिव्यक्ती* lowerp = nullptr;
        Ast_गाठ_अभिव्यक्ती* upperp = nullptr;
        जर (minWidep->अंक().isCaseEq(minimum)) {
            VL_DO_DANGLING(ढकला_Deletep(minWidep), minWidep);
        } अन्यथा {
            lowerp = exprp->isSigned() ? static_cast<Ast_गाठ_अभिव्यक्ती*>(
                                             नवीन AstGteS{fl, exprp->clone_वृक्ष(असत्य), minWidep})
                                       : static_cast<Ast_गाठ_अभिव्यक्ती*>(
                                             नवीन AstGte{fl, exprp->clone_वृक्ष(असत्य), minWidep});
        }
        जर (maxWidep->अंक().isCaseEq(maximum)) {
            VL_DO_DANGLING(ढकला_Deletep(maxWidep), maxWidep);
        } अन्यथा {
            upperp = exprp->isSigned() ? static_cast<Ast_गाठ_अभिव्यक्ती*>(
                                             नवीन AstLteS{fl, exprp->clone_वृक्ष(असत्य), maxWidep})
                                       : static_cast<Ast_गाठ_अभिव्यक्ती*>(
                                             नवीन AstLte{fl, exprp->clone_वृक्ष(असत्य), maxWidep});
        }
        जर (lowerp && upperp) परत नवीन AstAnd{fl, lowerp, upperp};
        जर (lowerp) परत lowerp;
        जर (upperp) परत upperp;
        परत नवीन AstConst{fl, AstConst::BitTrue{}};
    }

    // Build a one-sided comparison for an open-ended bin range whose other bound is '$'.
    // '$' denotes the coverpoint domain extreme, so {[lo:$]} == (expr >= lo) and
    // {[$:hi]} == (expr <= hi).
    Ast_गाठ_अभिव्यक्ती* makeOpenRangeCondition(File_ओळ* fl, Ast_गाठ_अभिव्यक्ती* exprp, AstConst* boundp,
                                        तार्किक isLowerBound) {
        AstConst* स्थिरांक widep = widenConst(fl, boundp, exprp->widthMin());
        जर (isLowerBound) {
            जर (exprp->isSigned()) परत नवीन AstGteS{fl, exprp->clone_वृक्ष(असत्य), widep};
            परत नवीन AstGte{fl, exprp->clone_वृक्ष(असत्य), widep};
        }
        जर (exprp->isSigned()) परत नवीन AstLteS{fl, exprp->clone_वृक्ष(असत्य), widep};
        परत नवीन AstLte{fl, exprp->clone_वृक्ष(असत्य), widep};
    }

    // Build condition for a single transition item.
    // Returns expression that checks if exprp matches the item's value/range list.
    // Overload for when the expression is a variable read -- creates and manages the VarRef
    // internally, so callers don't need to construct a temporary node.
    Ast_गाठ_अभिव्यक्ती* buildTransitionItemCondition(AstCoverTransItem* itemp, AstVar* varp) {
        Ast_गाठ_अभिव्यक्ती* varRefp = नवीन AstVarRef{varp->fileline(), varp, VAccess::READ};
        Ast_गाठ_अभिव्यक्ती* स्थिरांक condp = buildTransitionItemCondition(itemp, varRefp);
        VL_DO_DANGLING(ढकला_Deletep(varRefp), varRefp);
        परत condp;
    }

    // Non-owning: exprp is cloned internally; caller retains ownership of exprp.
    Ast_गाठ_अभिव्यक्ती* buildTransitionItemCondition(AstCoverTransItem* itemp, Ast_गाठ_अभिव्यक्ती* exprp) {
        Ast_गाठ_अभिव्यक्ती* condp = nullptr;

        साठी (Ast_गाठ* valp = itemp->valuesp(); valp; valp = valp->nextp()) {
            Ast_गाठ_अभिव्यक्ती* singleCondp = nullptr;
            valp = V3Const::constifyEdit(valp);
            AstConst* स्थिरांक constp = VN_CAST(valp, Const);
            जर (!constp) {
                valp->v3error("Non-constant expression in transition bin; "
                              "values must be constants (IEEE 1800-2023 19.5)");
                परत नवीन AstConst{valp->fileline(), AstConst::BitFalseErroring{}};
            }
            singleCondp
                = नवीन AstEq{constp->fileline(), exprp->clone_वृक्ष(असत्य), constp->clone_वृक्ष(असत्य)};
            जर (condp) {
                condp = नवीन AstOr{itemp->fileline(), condp, singleCondp};
            } अन्यथा {
                condp = singleCondp;
            }
        }

        परत condp;
    }

    // Generate code for a single transition sequence (used by both regular and array bins)
    शून्य generateSingleTransitionCode(AstCoverpoint* coverpointp, AstCoverBin* binp,
                                      Ast_गाठ_अभिव्यक्ती* exprp, स्थिरांक ConvBinTarget& tgt,
                                      AstCoverTransSet* transSetp) {
        UINFO(4, "      Generating code for transition sequence");

        // Get or create previous value variable
        AstVar* स्थिरांक prevVarp = create_Prev_मूल्य_Var(coverpointp, exprp);

        UASSERT_OBJ(
            transSetp, binp,
            "Transition bin has no transition set (transp() was checked before calling this)");

        // Get transition items (the sequence: item1 => item2 => item3)
        std::vector<AstCoverTransItem*> वस्तुसंच;
        साठी (Ast_गाठ* itemp = transSetp->itemsp(); itemp; itemp = itemp->nextp())
            वस्तुसंच.ढकला_back(VN_AS(itemp, CoverTransItem));

        जर (वस्तुसंच.empty()) {
            binp->v3error("Transition set without items");
            परत;
        }

        जर (वस्तुसंच.size() == 1) {
            // Single item transition not valid (need at least 2 values for =>)
            binp->v3error("Transition requires at least two values");
            परत;
        } अन्यथा जर (वस्तुसंच.size() == 2) {
            // Simple two-value transition: (val1 => val2)
            // Use optimized direct comparison (no state machine needed)
            Ast_गाठ_अभिव्यक्ती* स्थिरांक cond1p = buildTransitionItemCondition(वस्तुसंच[0], prevVarp);
            Ast_गाठ_अभिव्यक्ती* स्थिरांक cond2p = buildTransitionItemCondition(वस्तुसंच[1], exprp);

            // Combine: prev matches val1 AND current matches val2
            Ast_गाठ_अभिव्यक्ती* fullCondp = नवीन AstAnd{binp->fileline(), cond1p, cond2p};

            addConvTransHitIf(coverpointp, binp, tgt, fullCondp);

            UINFO(4, "        Successfully added 2-value transition if statement");
        } अन्यथा {
            // Multi-value sequence (a => b => c => ...)
            // Use state machine to track position in sequence
            generate_Multi_मूल्य_Transition_Code(coverpointp, binp, exprp, tgt, वस्तुसंच);
        }
    }

    // Append a "{ VlCoverpoint* __Vcx_cps[] = {cp0, cp1, ...}; <call> }" statement.  The brace
    // and the temporary array stay literal text -- a CMethodHard is one call, not a block --
    // but callp itself carries the member, method and '->'.  Construction only: init() copies
    // the array into the cross, so sample() reads it from there and needs no array at all.
    AstCStmt* makeCrossCpsCall(File_ओळ* fl, स्थिरांक std::vector<AstVar*>& cpVars,
                               AstCMethodHard* callp) {
        AstCStmt* स्थिरांक cs = नवीन AstCStmt{fl};
        cs->add("{ VlCoverpoint* __Vcx_cps[] = {");
        साठी (माप_प्रकार d = 0; d < cpVars.size(); ++d) {
            जर (d != 0) cs->add(", ");
            cs->add(memberRef(fl, cpVars[d]));
        }
        cs->add("}; ");
        cs->add(callp);
        cs->add("; }");
        परत cs;
    }

    // Assign the per-bin flags individually: one-bit SV results have integer C++ storage types,
    // which may narrow in a bool initializer list but convert implicitly in assignments.
    AstCStmt* makeCrossIffsCall(File_ओळ* fl, स्थिरांक std::vector<AstCoverCrossBin*>& bins,
                                AstCMethodHard* callp) {
        AstCStmt* स्थिरांक cs = नवीन AstCStmt{fl};
        cs->add("{ bool __Vcx_iffs[" + cvtToStr(bins.size()) + "]; ");
        साठी (माप_प्रकार i = 0; i < bins.size(); ++i) {
            स्थिरांक AstCoverCrossBin* स्थिरांक binp = bins[i];
            cs->add("__Vcx_iffs[" + cvtToStr(i) + "] = ");
            cs->add(binp->iffp() ? binp->iffp()->clone_वृक्ष(असत्य)
                                 : नवीन AstConst{fl, AstConst::BitTrue{}});
            cs->add("; ");
        }
        cs->add(callp);
        cs->add("; }");
        परत cs;
    }

    using CrossSelection = std::vector<uint64_t>;
    संरचना ResolvedCrossBin final {
        AstCoverCrossBin* binp;
        CrossSelection selection;
    };
    संरचना CrossLayout final {
        uint32_t tuples = 0;
        uint32_t autoBins = 0;
        uint64_t bin_शब्द = 0;
        तार्किक valid = सत्य;
        std::vector<ResolvedCrossBin> bins;
    };
    संरचना CrossSelectionContext final {
        AstCoverCross* crossp;  // Cross whose tuple space is being selected
        स्थिरांक std::vector<AstVar*>& cpVars;  // Feeding coverpoints in dimension order
        स्थिरांक std::map<std::string, uint32_t>& dimensions;  // Coverpoint name -> dimension
        uint32_t tuples;  // Size of the Cartesian product
        std::vector<uint32_t> strides;  // Flat-index stride per dimension
        तार्किक valid = सत्य;  // False if this explicit bin cannot be implemented
    };
    संरचना CrossBinsofTarget final {
        स्थिरांक CoverpointBins* m_binsp = nullptr;  // Declared bins; null after a reported error
        uint32_t m_dimension = 0;  // Coverpoint index within the cross
        uint32_t m_first = 0;  // First declared normal bin in the selected span
        uint32_t m_मोजणी = 0;  // Number of declared normal bins in the selected span
        uint32_t m_declaredFirst = 0;  // First runtime bin index of the selected span
        uint32_t m_declaredEnd = UINT32_MAX;  // One past the last runtime bin index
        int32_t m_sized = -1;  // Index of the sized array holding the selected bins; or -1
    };
    संरचना Cross_मूल्य_Range final {
        V3_संख्या lo;  // Inclusive lower bound, sign-extended to the comparison width
        V3_संख्या hi;  // Inclusive upper bound
        V3_संख्या pattern;  // Allowed bit values; all X for an ordinary interval
        तार्किक wildcard = असत्य;  // A wildcard singleton rather than an exact four-state value

        Cross_मूल्य_Range(Ast_गाठ* nodep, पूर्णांक width)
            : lo{nodep, width}
            , hi{nodep, width}
            , pattern{nodep, width} {
            pattern.setAllBitsX();
        }
    };

    स्थिर std::vector<Ast_गाठ*> cross_Bin_मूल्ये(स्थिरांक Cross_Bin_मूल्ये& bin) {
        जर (bin.valuep) परत {bin.valuep};
        std::vector<Ast_गाठ*> मूल्ये;
        जर (bin.binp->transp()) {
            // IEEE 1800-2023 19.6.1: binsof uses the last value of each transition.
            साठी (Ast_गाठ* setp = bin.binp->transp(); setp; setp = setp->nextp()) {
                AstCoverTransItem* lastp = VN_AS(setp, CoverTransSet)->itemsp();
                पर्यंत (lastp->nextp()) lastp = VN_AS(lastp->nextp(), CoverTransItem);
                साठी (Ast_गाठ* valuep = lastp->valuesp(); valuep; valuep = valuep->nextp()) {
                    मूल्ये.ढकला_back(valuep);
                }
            }
        } अन्यथा {
            साठी (Ast_गाठ* valuep = bin.binp->rangesp(); valuep; valuep = valuep->nextp()) {
                मूल्ये.ढकला_back(valuep);
            }
        }
        परत मूल्ये;
    }

    // The values of the element-th bin of a run, sign-extended to 'width'
    स्थिर Cross_मूल्य_Range runBinRange(Ast_गाठ* nodep, स्थिरांक BinRun& run, uint32_t element,
                                       पूर्णांक width) {
        स्थिरांक पूर्णांक runw = run.m_lo.width();
        V3_संख्या offset{nodep, runw};
        offset.opMul(run.m_stride, V3_संख्या{nodep, runw, element});
        V3_संख्या lo{nodep, runw};
        lo.opAdd(run.m_lo, offset);
        V3_संख्या hi = run.m_hi;
        जर (element + 1 < run.m_मोजणी) {
            offset.opSub(run.m_stride, V3_संख्या{nodep, runw, 1});
            hi.opAdd(lo, offset);
        }
        Cross_मूल्य_Range range{nodep, width};
        range.lo.opExtendS(lo, runw);
        range.hi.opExtendS(hi, runw);
        परत range;
    }

    स्थिर पूर्णांक crossRangeWidth(Ast_गाठ* nodep) {
        जर (स्थिरांक AstInsideRange* स्थिरांक rangep = VN_CAST(nodep, InsideRange)) {
            परत std::max(rangep->lhsp()->width(), rangep->rhsp()->width());
        }
        परत nodep->width();
    }

    स्थिर तार्किक cross_मूल्य_Less(स्थिरांक V3_संख्या& lhs, स्थिरांक V3_संख्या& rhs) {
        V3_संख्या result{&lhs};
        परत !result.opLtS(lhs, rhs).isEqZero();
    }

    // Round a real bound inward to the nearest coverpoint value (IEEE 1800-2023 19.5.7).  Sets
    // 'empty' if the domain has no value on the bound's side.
    स्थिर शून्य crossRealBound(स्थिरांक AstConst* constp, Ast_गाठ_अभिव्यक्ती* exprp, तार्किक upper,
                               स्थिरांक Cross_मूल्य_Range& domain, V3_संख्या& result, तार्किक& empty) {
        स्थिरांक द्विगुण मूल्य = constp->अंक().toDouble();
        स्थिरांक द्विगुण bound = upper ? std::न्यूनमान(मूल्य) : std::उच्चमान(मूल्य);
        // Powers of two are exact, so the integral bound compares exactly with the domain edges.
        स्थिरांक द्विगुण limit
            = std::ldexp(1.0, exprp->isSigned() ? exprp->width() - 1 : exprp->width());
        स्थिरांक द्विगुण minimum = exprp->isSigned() ? -limit : 0.0;
        जर (std::isnan(bound) || (upper ? bound < minimum : bound >= limit)) {
            empty = सत्य;
            result = domain.lo;
        } अन्यथा जर (bound < minimum) {
            result = domain.lo;
        } अन्यथा जर (bound >= limit) {
            result = domain.hi;
        } अन्यथा {
            V3_संख्या real{&result, 64};
            real.setDouble(bound);
            result.opRToIRoundS(real);
        }
    }

    स्थिर तार्किक crossRangeBound(Ast_गाठ* nodep, Ast_गाठ_अभिव्यक्ती* exprp, तार्किक upper, तार्किक bin_मूल्य,
                                स्थिरांक Cross_मूल्य_Range& domain, V3_संख्या& result, तार्किक& empty) {
        जर (VN_IS(nodep, Unbounded)) {
            V3_संख्या limit{nodep, exprp->width()};
            जर (upper) limit.setAllBits1();
            जर (exprp->isSigned()) {
                limit.setBit(exprp->width() - 1, !upper);
                result.opExtendS(limit, limit.width());
            } अन्यथा {
                result.opAssign(limit);
            }
            परत सत्य;
        }
        स्थिरांक AstConst* स्थिरांक constp = VN_CAST(nodep, Const);
        जर (!constp) परत असत्य;
        जर (constp->अंक().isDouble()) {
            crossRealBound(constp, exprp, upper, domain, result, empty);
            परत सत्य;
        }
        जर (constp->अंक().isString()) परत असत्य;
        जर (bin_मूल्य && exprp->isSigned() && constp->width() <= exprp->width()) {
            // Bin bit patterns use the coverpoint's effective type (IEEE 1800-2023 19.5.7).
            // Wider values and intersect filters retain their values for domain clipping.
            V3_संख्या मूल्य{nodep, exprp->width()};
            जर (constp->isSigned()) {
                मूल्य.opExtendS(constp->अंक(), constp->width());
            } अन्यथा {
                मूल्य.opAssign(constp->अंक());
            }
            result.opExtendS(मूल्य, मूल्य.width());
        } अन्यथा जर (constp->isSigned()) {
            result.opExtendS(constp->अंक(), constp->width());
        } अन्यथा {
            result.opAssign(constp->अंक());
        }
        परत सत्य;
    }

    स्थिर Cross_मूल्य_Range cross_मूल्य_Domain(Ast_गाठ* nodep, पूर्णांक मूल्य_Width, तार्किक isSigned,
                                            पूर्णांक width) {
        Cross_मूल्य_Range domain{nodep, width};
        V3_संख्या lo{nodep, मूल्य_Width};
        V3_संख्या hi{nodep, मूल्य_Width};
        hi.setAllBits1();
        जर (isSigned) {
            lo.setBit(मूल्य_Width - 1, 1);
            hi.setBit(मूल्य_Width - 1, 0);
            domain.lo.opExtendS(lo, मूल्य_Width);
            domain.hi.opExtendS(hi, मूल्य_Width);
        } अन्यथा {
            domain.lo.opAssign(lo);
            domain.hi.opAssign(hi);
        }
        परत domain;
    }

    स्थिर शून्य intersectCrossRange(Cross_मूल्य_Range& range, स्थिरांक Cross_मूल्य_Range& other) {
        जर (cross_मूल्य_Less(range.lo, other.lo)) range.lo = other.lo;
        जर (cross_मूल्य_Less(other.hi, range.hi)) range.hi = other.hi;
    }

    स्थिर तार्किक cross_मूल्य_Range(Ast_गाठ* nodep, Ast_गाठ_अभिव्यक्ती* exprp, तार्किक bin_मूल्य, तार्किक wildcard,
                                स्थिरांक Cross_मूल्य_Range& domain, Cross_मूल्य_Range& range) {
        तार्किक empty = असत्य;
        स्थिरांक AstConst* स्थिरांक constp = VN_CAST(nodep, Const);
        जर (स्थिरांक AstInsideRange* स्थिरांक rangep = VN_CAST(nodep, InsideRange)) {
            जर (!crossRangeBound(rangep->lhsp(), exprp, असत्य, bin_मूल्य, domain, range.lo, empty)
                || !crossRangeBound(rangep->rhsp(), exprp, सत्य, bin_मूल्य, domain, range.hi, empty)
                || range.lo.isFourState() || range.hi.isFourState()) {
                परत असत्य;
            }
        } अन्यथा जर (constp && constp->अंक().isDouble()) {
            // A real value participates only if integral; it has no wildcard bits.
            crossRealBound(constp, exprp, असत्य, domain, range.lo, empty);
            crossRealBound(constp, exprp, सत्य, domain, range.hi, empty);
        } अन्यथा {
            जर (!crossRangeBound(nodep, exprp, असत्य, bin_मूल्य, domain, range.lo, empty)) {
                परत असत्य;
            }
            जर (bin_मूल्य && exprp->isSigned() && constp && !constp->isSigned()
                && constp->width() > exprp->width()) {
                तार्किक representable = सत्य;
                साठी (पूर्णांक bit = exprp->width(); bit < constp->width(); ++bit) {
                    representable &= !constp->अंक().bitIs1(bit);
                }
                जर (representable) {
                    V3_संख्या मूल्य{nodep, exprp->width()};
                    मूल्य.opAssign(constp->अंक());
                    range.lo.opExtendS(मूल्य, मूल्य.width());
                }
            }
            range.hi = range.lo;
            range.wildcard = wildcard;
            जर (wildcard) {
                range.pattern = range.lo;
                range.lo = domain.lo;
                range.hi = domain.hi;
                जर (constp && constp->isSigned()) {
                    // Replicated X sign bits are correlated, not independent wildcards.
                    // The source domain preserves expansion-before-casting (19.5.7).
                    intersectCrossRange(
                        range, cross_मूल्य_Domain(nodep, constp->width(), सत्य, domain.lo.width()));
                }
            }
        }
        जर (empty) {
            range.lo = domain.hi;
            range.hi = domain.lo;
            परत सत्य;
        }
        जर (!range.lo.isFourState()) intersectCrossRange(range, domain);
        जर (range.wildcard && exprp->isSigned()) {
            स्थिरांक पूर्णांक sign = exprp->width() - 1;
            जर (constp && !constp->isSigned() && constp->width() > exprp->width()) {
                // Unsigned equality preserves every target bit pattern if discarded bits are zero.
                साठी (पूर्णांक bit = exprp->width(); bit < constp->width(); ++bit) {
                    जर (constp->अंक().bitIs1(bit)) {
                        range.lo = domain.hi;
                        range.hi = domain.lo;
                        परत सत्य;
                    }
                }
                साठी (पूर्णांक bit = exprp->width(); bit < range.pattern.width(); ++bit) {
                    जर (range.pattern.bitIsXZ(sign))
                        range.pattern.setBit(bit, 'x');
                    अन्यथा
                        range.pattern.setBit(bit, range.pattern.bitIs1(sign));
                }
                परत सत्य;
            }
            // Discarded high bits constrain the target sign, rather than becoming don't-cares.
            साठी (पूर्णांक bit = exprp->width(); bit < range.pattern.width(); ++bit) {
                जर (range.pattern.bitIsXZ(bit)) सुरूठेवा;
                स्थिरांक तार्किक मूल्य = range.pattern.bitIs1(bit);
                जर (!range.pattern.bitIsXZ(sign) && range.pattern.bitIs1(sign) != मूल्य) {
                    range.lo = domain.hi;
                    range.hi = domain.lo;
                    थांबा;
                }
                range.pattern.setBit(sign, मूल्य);
            }
        }
        परत सत्य;
    }

    गणना CrossRangeState : uint8_t {
        CROSS_INSIDE_BOUNDS = 0,  // Prefix is strictly inside the interval
        CROSS_AT_LOWER = 1,
        CROSS_AT_UPPER = 2,
        CROSS_AT_BOUNDS = CROSS_AT_LOWER | CROSS_AT_UPPER,
        CROSS_NO_MATCH = 4  // Prefix cannot match the interval/pattern
    };

    स्थिर CrossRangeState crossRangeStep(स्थिरांक Cross_मूल्य_Range& range, CrossRangeState state,
                                          पूर्णांक bit, पूर्णांक मूल्य) {
        जर (state == CROSS_NO_MATCH) परत CROSS_NO_MATCH;
        // Flipping the sign bit makes signed order lexicographic.
        स्थिरांक तार्किक sign = bit == range.pattern.width() - 1;
        जर (!range.pattern.bitIsXZ(bit) && मूल्य != (range.pattern.bitIs1(bit) ^ sign)) {
            परत CROSS_NO_MATCH;
        }
        स्थिरांक पूर्णांक खालचा = range.lo.bitIs1(bit) ^ sign;
        स्थिरांक पूर्णांक वरचा = range.hi.bitIs1(bit) ^ sign;
        जर (((state & CROSS_AT_LOWER) && मूल्य < खालचा)
            || ((state & CROSS_AT_UPPER) && मूल्य > वरचा)) {
            परत CROSS_NO_MATCH;
        }
        परत static_cast<CrossRangeState>(
            ((state & CROSS_AT_LOWER) && मूल्य == खालचा ? CROSS_AT_LOWER : CROSS_INSIDE_BOUNDS)
            | ((state & CROSS_AT_UPPER) && मूल्य == वरचा ? CROSS_AT_UPPER : CROSS_INSIDE_BOUNDS));
    }

    स्थिर तार्किक crossWildcardIntersects(स्थिरांक Cross_मूल्य_Range& range) {
        अचिन्हित states = 1U << CROSS_AT_BOUNDS;
        साठी (पूर्णांक bit = range.pattern.width() - 1; bit >= 0 && states; --bit) {
            अचिन्हित next = 0;
            साठी (स्थिरांक CrossRangeState state :
                 {CROSS_INSIDE_BOUNDS, CROSS_AT_LOWER, CROSS_AT_UPPER, CROSS_AT_BOUNDS}) {
                जर (!(states & (1U << state))) सुरूठेवा;
                साठी (पूर्णांक मूल्य = 0; मूल्य < 2; ++मूल्य) {
                    स्थिरांक CrossRangeState equal = crossRangeStep(range, state, bit, मूल्य);
                    जर (equal != CROSS_NO_MATCH) next |= 1U << equal;
                }
            }
            states = next;
        }
        परत states != 0;
    }

    // The least value at least 'from' with the non-x bits of 'pattern', in unsigned order; false
    // if none.  A bit scan, which does not enumerate the pattern's x bits.
    स्थिर तार्किक next_Pattern_मूल्य(स्थिरांक V3_संख्या& pattern, स्थिरांक V3_संख्या& from, V3_संख्या& result) {
        result.opAssign(from);
        पूर्णांक carry = -1;  // Lowest x bit above the bit scanned where 'from' has a 0
        साठी (पूर्णांक bit = from.width() - 1; bit >= 0; --bit) {
            जर (pattern.bitIsXZ(bit)) {
                जर (!from.bitIs1(bit)) carry = bit;
                सुरूठेवा;
            }
            जर (pattern.bitIs1(bit) == from.bitIs1(bit)) सुरूठेवा;
            जर (from.bitIs1(bit)) {  // Only a greater prefix, an x bit above raised, can match
                जर (carry < 0) परत असत्य;
                bit = carry;
            }
            // Then the least such value: the pattern's bits, with x bits of zero
            result.setBit(bit, 1);
            पर्यंत (--bit >= 0) result.setBit(bit, pattern.bitIs1(bit));
            परत सत्य;
        }
        परत सत्य;
    }

    // Append to 'runs' the maximal runs of consecutive values of 'range' that its pattern
    // matches, in value order, until there are more than 'limit'.  A match continues through
    // the pattern's trailing x bits only, which the next value's carry leaves.
    स्थिर शून्य crossRangeRuns(स्थिरांक Cross_मूल्य_Range& range, माप_प्रकार limit,
                               std::vector<std::pair<V3_संख्या, V3_संख्या>>& runs) {
        // Values order signed, which is the unsigned order of values with the sign bit flipped
        स्थिरांक पूर्णांक sign = range.lo.width() - 1;
        स्थिरांक स्वतः flipped = [sign](V3_संख्या मूल्य) {
            जर (!मूल्य.bitIsXZ(sign)) मूल्य.setBit(sign, !मूल्य.bitIs1(sign));
            परत मूल्य;
        };
        स्थिरांक V3_संख्या pattern = flipped(range.pattern);
        स्थिरांक V3_संख्या hi = flipped(range.hi);
        स्थिरांक V3_संख्या one{&hi, hi.width(), 1};
        V3_संख्या trailing{&hi, hi.width()};
        साठी (पूर्णांक bit = 0; bit <= sign && pattern.bitIsXZ(bit); ++bit) trailing.setBit(bit, 1);
        V3_संख्या from = flipped(range.lo);
        V3_संख्या first{&hi, hi.width()};
        V3_संख्या last{&hi, hi.width()};
        V3_संख्या less{&hi};
        पर्यंत (runs.size() <= limit && next_Pattern_मूल्य(pattern, from, first)
               && less.opLt(hi, first).isEqZero()) {
            last.opOr(first, trailing);
            जर (!less.opLt(hi, last).isEqZero()) last = hi;
            runs.emplace_back(flipped(first), flipped(last));
            जर (last.isCaseEq(hi)) थांबा;
            from.opAdd(last, one);
        }
    }

    // True if no coverpoint value participates in a resolved value or range.  A value with x
    // or z bits participates only as a wildcard pattern (IEEE 1800-2023 19.5.7).
    स्थिर तार्किक crossRangeEmpty(स्थिरांक Cross_मूल्य_Range& range) {
        जर (range.lo.isFourState()) परत सत्य;
        परत cross_मूल्य_Less(range.hi, range.lo)
               || (range.wildcard && !crossWildcardIntersects(range));
    }

    // Comparison width that holds both the coverpoint's and a bin or intersect value's range.
    स्थिर पूर्णांक resolveWidth(Ast_गाठ* nodep, स्थिरांक Ast_गाठ_अभिव्यक्ती* exprp) {
        परत std::max(exprp->width(), crossRangeWidth(nodep)) + 1;
    }

    // Resolve a bin or intersect value to coverpoint values (IEEE 1800-2023 19.5.7), in the
    // width 'range' was created with.  False if the value is not a constant integral or real.
    स्थिर तार्किक resolve_मूल्य(Ast_गाठ* nodep, Ast_गाठ_अभिव्यक्ती* exprp, तार्किक bin_मूल्य, तार्किक wildcard,
                             Cross_मूल्य_Range& range) {
        स्थिरांक Cross_मूल्य_Range domain
            = cross_मूल्य_Domain(nodep, exprp->width(), exprp->isSigned(), range.lo.width());
        परत cross_मूल्य_Range(nodep, exprp, bin_मूल्य, wildcard, domain, range);
    }

    स्थिर तार्किक crossRangesIntersect(स्थिरांक Cross_मूल्य_Range& bin, स्थिरांक Cross_मूल्य_Range& filter) {
        // Values with x or z bits do not participate, even in an identical filter.
        जर (bin.lo.isFourState() || filter.lo.isFourState()) परत असत्य;
        Cross_मूल्य_Range match = bin;
        intersectCrossRange(match, filter);
        परत !cross_मूल्य_Less(match.hi, match.lo)
               && (!bin.wildcard || crossWildcardIntersects(match));
    }

    स्थिर शून्य unsupportedCrossRange(AstCoverBinsof* selectp, तार्किक& valid) {
        selectp->v3warn(COVERIGN, "Unsupported: non-constant or non-integral 'intersect' value, "
                                  "or four-state range bound.");
        valid = असत्य;
    }

    स्थिर तार्किक cross_मूल्य_Matches_Filters(AstCoverBinsof* selectp, Ast_गाठ* valuep,
                                         Ast_गाठ_अभिव्यक्ती* exprp, स्थिरांक AstCoverBin* binp,
                                         स्थिरांक Cross_मूल्य_Range& domain,
                                         स्थिरांक std::vector<Cross_मूल्य_Range>& filters,
                                         तार्किक& valid) {
        Cross_मूल्य_Range range{valuep, domain.lo.width()};
        जर (!cross_मूल्य_Range(valuep, exprp, सत्य, binp->isWildcard(), domain, range)) {
            unsupportedCrossRange(selectp, valid);
            परत असत्य;
        }
        साठी (स्थिरांक Cross_मूल्य_Range& filter : filters) {
            जर (crossRangesIntersect(range, filter)) परत सत्य;
        }
        परत असत्य;
    }

    std::vector<तार्किक> selectCoverpointBins(AstCoverBinsof* selectp, स्थिरांक CoverpointBins& bins,
                                           uint32_t first, uint32_t मोजणी, तार्किक& valid) {
        जर (selectp->rangesp() && !bins.exprp->dtypep()->skipRefp()->isIntegralOrPacked()) {
            unsupportedCrossRange(selectp, valid);
            परत {};
        }
        std::vector<तार्किक> selected(bins.total, असत्य);
        std::vector<std::vector<Ast_गाठ*>> मूल्ये;
        पूर्णांक width = bins.exprp->width();
        साठी (Ast_गाठ* rangep = selectp->rangesp(); rangep; rangep = rangep->nextp()) {
            width = std::max(width, crossRangeWidth(rangep));
        }
        जर (selectp->rangesp()) {
            मूल्ये.reserve(मोजणी);
            साठी (uint32_t i = first; i < first + मोजणी; ++i) {
                // A run's values are in the coverpoint type, whose width 'width' covers
                मूल्ये.ढकला_back(bins.मूल्ये[i].runp ? std::vector<Ast_गाठ*>{}
                                                     : cross_Bin_मूल्ये(bins.मूल्ये[i]));
                साठी (Ast_गाठ* स्थिरांक valuep : मूल्ये.back()) {
                    width = std::max(width, crossRangeWidth(valuep));
                }
            }
        }
        // One extra bit preserves both unsigned maxima and negative signed bounds.
        ++width;
        स्थिरांक Cross_मूल्य_Range domain
            = cross_मूल्य_Domain(selectp, bins.exprp->width(), bins.exprp->isSigned(), width);
        std::vector<Cross_मूल्य_Range> filters;
        साठी (Ast_गाठ* rangep = selectp->rangesp(); rangep; rangep = rangep->nextp()) {
            Cross_मूल्य_Range filter{rangep, width};
            जर (!cross_मूल्य_Range(rangep, bins.exprp, असत्य, असत्य, domain, filter)) {
                unsupportedCrossRange(selectp, valid);
                परत {};
            }
            filters.ढकला_back(std::move(filter));
        }
        साठी (uint32_t i = first; i < first + मोजणी; ++i) {
            जर (!selectp->rangesp()) {
                selected[i] = सत्य;
                सुरूठेवा;
            }
            जर (स्थिरांक BinRun* स्थिरांक runp = bins.मूल्ये[i].runp) {
                स्थिरांक Cross_मूल्य_Range range
                    = runBinRange(selectp, *runp, bins.मूल्ये[i].element, width);
                selected[i] = std::any_of(filters.begin(), filters.end(),
                                          [&](स्थिरांक Cross_मूल्य_Range& filter) {
                                              परत crossRangesIntersect(range, filter);
                                          });
                सुरूठेवा;
            }
            साठी (Ast_गाठ* स्थिरांक valuep : मूल्ये[i - first]) {
                selected[i] = cross_मूल्य_Matches_Filters(
                    selectp, valuep, bins.exprp, bins.मूल्ये[i].binp, domain, filters, valid);
                जर (!valid) परत {};
                जर (selected[i]) थांबा;
            }
        }
        जर (selectp->isNegated()) {
            साठी (uint32_t i = 0; i < bins.total; ++i) selected[i] = !selected[i];
        }
        परत selected;
    }

    // Constructor-time value metadata of one coverpoint, as C++ list entries
    संरचना मूल्य_Lists final {
        std::vector<std::string> m_ranges;  // Bin, then low and high words
        std::vector<std::string> m_runs;  // First bin, count, then low, span, and high words
        std::vector<std::string> m_patterns;  // Bin, then value, mask, low, and high words
        std::vector<std::string> m_transitions;  // Transition bin
    };

    // Append a value's words, in the coverpoint's width, to a C++ list entry.
    स्थिर शून्य append_शब्द(std::string& text, स्थिरांक V3_संख्या& मूल्य, स्थिरांक Ast_गाठ_अभिव्यक्ती* exprp) {
        V3_संख्या narrowed{&मूल्य, exprp->width()};
        narrowed.opAssign(मूल्य);
        साठी (पूर्णांक word = 0; word < exprp->width_शब्द(); ++word) {
            text += ", " + cvtToStr(narrowed.edataWord(word)) + "U";
        }
    }

    शून्य collect_मूल्य_Metadata(मूल्य_Lists& lists, Ast_गाठ_अभिव्यक्ती* exprp, AstCoverBin* binp,
                              uint32_t index, Ast_गाठ_अभिव्यक्ती* valuep) {
        स्थिरांक std::string bin = cvtToStr(index) + "U";
        जर (binp->transp()) lists.m_transitions.ढकला_back(bin);
        साठी (Ast_गाठ* स्थिरांक sourcep : cross_Bin_मूल्ये({binp, valuep})) {
            Cross_मूल्य_Range range{sourcep, resolveWidth(sourcep, exprp)};
            जर (!resolve_मूल्य(sourcep, exprp, सत्य, binp->isWildcard(), range)) {
                // Sampling already resolved every state bin value, so only transitions remain.
                sourcep->v3warn(E_UNSUPPORTED, "Unsupported: non-integral value in a transition "
                                               "bin of a coverpoint with exclusions.");
                सुरूठेवा;
            }
            जर (crossRangeEmpty(range)) सुरूठेवा;
            std::string entry = bin;
            जर (range.wildcard) {
                V3_संख्या मूल्य{sourcep, exprp->width()};
                V3_संख्या mask{sourcep, exprp->width()};
                mask.opBitsNonXZ(range.pattern);
                मूल्य.opBitsOne(range.pattern);
                append_शब्द(entry, मूल्य, exprp);
                append_शब्द(entry, mask, exprp);
            }
            append_शब्द(entry, range.lo, exprp);
            append_शब्द(entry, range.hi, exprp);
            (range.wildcard ? lists.m_patterns : lists.m_ranges).ढकला_back(entry);
        }
    }

    // Describe a run with one entry, from which the runtime computes the values of its bins.
    स्थिर शून्य collectRunMetadata(मूल्य_Lists& lists, Ast_गाठ_अभिव्यक्ती* exprp, स्थिरांक BinRun& run) {
        V3_संख्या span{exprp, run.m_stride.width(), 0};
        span.opSub(run.m_stride, V3_संख्या{exprp, run.m_stride.width(), 1});
        std::string entry = cvtToStr(run.m_declared) + "U, " + cvtToStr(run.m_मोजणी) + "U";
        append_शब्द(entry, run.m_lo, exprp);
        append_शब्द(entry, span, exprp);
        append_शब्द(entry, run.m_hi, exprp);
        lists.m_runs.ढकला_back(entry);
    }

    // Emit one batched metadata list, bounding the size of each call's temporary list.
    शून्य emit_मूल्य_List(File_ओळ* fl, AstVar* cpVarp, VCMethod method,
                       स्थिरांक std::vector<std::string>& entries) {
        साठी (माप_प्रकार first = 0; first < entries.size(); first += मूल्य_LIST_ENTRIES) {
            स्थिरांक माप_प्रकार end = std::min(entries.size(), first + मूल्य_LIST_ENTRIES);
            std::string text = "{" + entries[first];
            साठी (माप_प्रकार i = first + 1; i < end; ++i) text += ", " + entries[i];
            m_constructorp->addStmtsp(
                itemCall(fl, cpVarp, method, {ctext(fl, text + "}")})->makeStmt());
        }
    }

    स्थिर तार्किक checkCrossRef(स्थिरांक AstCoverCrossRef* refp, स्थिरांक AstCoverCross* crossp) {
        जर (refp->name() == crossp->name()) परत सत्य;
        refp->v3error("Cross selection "
                      << refp->prettyNameQ() << " may only name its enclosing cross "
                      << crossp->prettyNameQ() << " (IEEE 1800-2023 19.6.1.2).");
        परत असत्य;
    }

    स्थिर तार्किक checkCrossBinName(स्थिरांक AstCoverCrossBin* binp, std::set<std::string>& names) {
        जर (names.emplace(binp->name()).second) परत सत्य;
        binp->v3error("Duplicate cross bin " << binp->prettyNameQ()
                                             << " (IEEE 1800-2023 19.6.1).");
        परत असत्य;
    }

    // The span of the implicit automatic bin reported as 'name' ('auto_<i>', see
    // createImplicitAutoBins), found without naming each of its bins
    स्थिर तार्किक implicitAutoBinSpan(स्थिरांक CoverpointBins& bins, स्थिरांक std::string& name,
                                    BinSpan& span) {
        स्थिरांक std::string prefix = "auto_";
        जर (!VString::startsWith(name, prefix)) परत असत्य;
        स्थिरांक std::string digits = name.substr(prefix.size());
        स्थिरांक अचिन्हित दीर्घ index = std::म_से_अदीर्घ(digits.c_str(), nullptr, 10);
        // Only the reported spelling names the bin, not e.g. 'auto_01' or 'auto_x'
        जर (index >= bins.implicitAuto.मोजणी || digits != cvtToStr(index)) परत असत्य;
        स्थिरांक uint32_t offset = static_cast<uint32_t>(index);
        span = BinSpan{bins.implicitAuto.first + offset, 1, bins.implicitAuto.declared + offset};
        परत सत्य;
    }

    CrossBinsofTarget
    resolveBinsofTarget(स्थिरांक AstCoverBinsof* selectp, स्थिरांक AstCoverCross* crossp,
                        स्थिरांक std::vector<AstVar*>& cpVars,
                        स्थिरांक std::map<std::string, uint32_t>& dimensions) स्थिरांक {
        स्थिरांक स्वतः dim = dimensions.find(selectp->pointp()->name());
        जर (dim == dimensions.end()) {
            selectp->v3error("binsof coverpoint "
                             << selectp->pointp()->prettyNameQ() << " is not an item of cross "
                             << crossp->prettyNameQ() << " (IEEE 1800-2023 19.6.1).");
            परत {};
        }
        स्थिरांक CoverpointBins& bins = m_cpBins.at(cpVars[dim->second]);
        CrossBinsofTarget target{&bins, dim->second, 0, bins.total};
        जर (!selectp->name().empty()) {
            स्थिरांक स्वतः bin = bins.spans.find(selectp->name());
            BinSpan span{0, 0, 0};
            जर (bin != bins.spans.end()) {
                span = bin->second;
            } अन्यथा जर (!implicitAutoBinSpan(bins, selectp->name(), span)) {
                selectp->v3error("Cannot find bin " << selectp->prettyNameQ() << " in coverpoint "
                                                    << selectp->pointp()->prettyNameQ()
                                                    << " (IEEE 1800-2023 19.6.1).");
                परत {};
            }
            target.m_first = span.first;
            target.m_मोजणी = span.मोजणी;
            target.m_declaredFirst = span.declared;
            target.m_declaredEnd = span.declared + span.मोजणी;
            target.m_sized = span.sized;
        }
        परत target;
    }

    // The variables a 'with' filter reads, each by its first reference
    using CrossFilterReads = std::map<स्थिरांक AstVar*, AstVarRef*>;

    // Whether a 'with' filter has a count of value tuples, of 'matches' other than '$'
    स्थिर तार्किक counts_मूल्य_Tuples(स्थिरांक AstCoverCrossWith* withp) {
        परत withp->countp() && !VN_IS(withp->countp(), Unbounded);
    }

    // Whether the runtime knows the values of the cross items a 'with' filter reads, or to count
    // value tuples, of all items, of integral coverpoints; else warns
    स्थिर तार्किक crossFilterSupported(स्थिरांक AstCoverCrossWith* withp,
                                     स्थिरांक CrossFilterReads& reads) {
        साठी (स्थिरांक AstVar* varp = withp->varsp(); varp; varp = VN_AS(varp->nextp(), Var)) {
            जर (varp->dtypep()->skipRefp()->isIntegralOrPacked()) सुरूठेवा;
            स्थिरांक स्वतः it = reads.find(varp);
            जर (it != reads.end()) {
                it->second->v3warn(COVERIGN, "Unsupported: cross bin 'with' filter reading the "
                                             "value of non-integral coverpoint "
                                                 << it->second->prettyNameQ());
                परत असत्य;
            }
            जर (counts_मूल्य_Tuples(withp)) {
                withp->countp()->v3warn(COVERIGN, "Unsupported: cross bin 'matches' count of "
                                                  "value tuples of non-integral coverpoint "
                                                      << varp->prettyNameQ());
                परत असत्य;
            }
        }
        परत सत्य;
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
    शून्य generateCrossFilter(स्थिरांक AstCoverCrossWith* withp, स्थिरांक CrossFilterReads& reads,
                             स्थिरांक AstCoverCrossBin* binp, स्थिरांक AstCoverCross* crossp,
                             AstVar* cxp, स्थिरांक std::map<string, uint32_t>& dimensions,
                             uint32_t वस्तुसंच) {
        File_ओळ* स्थिरांक fl = withp->fileline();
        स्थिरांक string prefix
            = m_sizedNames.get(sanitizeGeneratedName(crossp->name() + "__" + binp->name()));
        स्थिरांक तार्किक counting = counts_मूल्य_Tuples(withp);
        // The count, 0 for all
        स्थिरांक std::pair<Ast_गाठ_अभिव्यक्ती*, Ast_गाठ_अभिव्यक्ती*> मोजणी
            = counting ? new_मोजणी_Args(fl, prefix + "_count", withp->countp())
                       : new_मोजणी_Args(fl, withp->countp() ? 0 : 1);
        AstLoop* स्थिरांक loopp = नवीन AstLoop{fl};
        AstCMethodHard* स्थिरांक nextp = itemCall(fl, cxp, VCMethod::COVERGROUP_WITH_NEXT);
        nextp->dtypeSetBit();
        loopp->addStmtsp(नवीन AstLoopTest{fl, loopp, nextp});
        string dims;
        साठी (uint32_t dim = 0; counting && dim < वस्तुसंच; ++dim) {
            dims += (dims.empty() ? "" : ", ") + cvtToStr(dim) + "U";
        }
        पर्यंत (AstVar* स्थिरांक varp = withp->varsp()) {
            varp->unlinkFrBack();
            जर (!reads.मोजणी(varp)) {
                VL_DO_DANGLING(ढकला_Deletep(varp), varp);
                सुरूठेवा;
            }
            स्थिरांक स्वतः dimIt = dimensions.find(varp->name());
            UASSERT_OBJ(dimIt != dimensions.end(), varp, "Cross 'with' candidate not of an item");
            स्थिरांक uint32_t dim = dimIt->second;
            जर (!counting) dims += (dims.empty() ? "" : ", ") + cvtToStr(dim) + "U";
            varp->name(prefix + "_" + sanitizeGeneratedName(varp->name()));
            m_constructorp->addStmtsp(varp);
            जर (varp->isWide()) {
                loopp->addStmtsp(itemCall(fl, cxp, VCMethod::COVERGROUP_CROSS_WITH_मूल्य_W,
                                          {cnum(fl, dim), नवीन AstVarRef{fl, varp, VAccess::WRITE}})
                                     ->makeStmt());
            } अन्यथा {
                AstCMethodHard* स्थिरांक valuep
                    = itemCall(fl, cxp, VCMethod::COVERGROUP_CROSS_WITH_मूल्य, {cnum(fl, dim)});
                valuep->dtypeSetUInt64();
                Ast_गाठ_DType* स्थिरांक dtypep = varp->findLogicDType(
                    varp->width(), varp->width(),
                    varp->isSigned() ? VSigning::SIGNED : VSigning::UNSIGNED);
                loopp->addStmtsp(नवीन AstAssign{fl, नवीन AstVarRef{fl, varp, VAccess::WRITE},
                                               resize_मूल्य(valuep, dtypep)});
            }
        }
        loopp->addStmtsp(itemCall(fl, cxp, VCMethod::COVERGROUP_CROSS_WITH_TEST,
                                  {withp->filterp()->unlinkFrBack()})
                             ->makeStmt());
        स्थिरांक तार्किक prot = v3Global.opt.protectIds();
        m_constructorp->addStmtsp(
            itemCall(fl, cxp, VCMethod::COVERGROUP_CROSS_WITH_BEGIN,
                     {ctext(fl, "{" + dims + "}"), मोजणी.first, मोजणी.second,
                      ctext(fl, quoted(VIdProtect::protect_शब्द_If(binp->name(), prot))),
                      ctext(fl, quoted(VIdProtect::protectIf(fl->filename(), prot))),
                      cnum(fl, static_cast<uint32_t>(fl->lineno()))})
                ->makeStmt());
        m_constructorp->addStmtsp(loopp);
        m_constructorp->addStmtsp(
            itemCall(fl, cxp, VCMethod::COVERGROUP_CROSS_WITH_END)->makeStmt());
    }

    तार्किक generateRuntimeSelection(Ast_गाठ* nodep, स्थिरांक AstCoverCrossBin* binp,
                                  AstCoverCross* crossp, AstVar* cxp,
                                  स्थिरांक std::vector<AstVar*>& cpVars,
                                  स्थिरांक std::map<string, uint32_t>& dimensions) {
        File_ओळ* स्थिरांक fl = nodep->fileline();
        जर (स्थिरांक AstCoverCrossRef* स्थिरांक refp = VN_CAST(nodep, CoverCrossRef)) {
            जर (!checkCrossRef(refp, crossp)) परत असत्य;
            m_constructorp->addStmtsp(
                itemCall(fl, cxp, VCMethod::COVERGROUP_SELECT_ALL)->makeStmt());
            परत सत्य;
        }
        जर (स्थिरांक AstCoverCrossSelect* स्थिरांक opp = VN_CAST(nodep, CoverCrossSelect)) {
            जर (!generateRuntimeSelection(opp->lhsp(), binp, crossp, cxp, cpVars, dimensions)
                || !generateRuntimeSelection(opp->rhsp(), binp, crossp, cxp, cpVars, dimensions)) {
                परत असत्य;
            }
            m_constructorp->addStmtsp(itemCall(fl, cxp,
                                               opp->isOr() ? VCMethod::COVERGROUP_SELECT_OR
                                                           : VCMethod::COVERGROUP_SELECT_AND)
                                          ->makeStmt());
            परत सत्य;
        }
        जर (स्थिरांक AstCoverCrossWith* स्थिरांक withp = VN_CAST(nodep, CoverCrossWith)) {
            CrossFilterReads reads;
            withp->filterp()->foreach([&](AstVarRef* refp) { reads.emplace(refp->varp(), refp); });
            // The count of 'matches' is a positive integer, or '$' (IEEE 1800-2023 19.6.1.2)
            जर ((counts_मूल्य_Tuples(withp)
                 && !check_मोजणी(withp->countp(), "Cross bin 'matches' count", "19.6.1.2"))
                || !crossFilterSupported(withp, reads)
                || !generateRuntimeSelection(withp->selectp(), binp, crossp, cxp, cpVars,
                                             dimensions)) {
                परत असत्य;
            }
            generateCrossFilter(withp, reads, binp, crossp, cxp, dimensions,
                                static_cast<uint32_t>(cpVars.size()));
            परत सत्य;
        }
        AstCoverBinsof* स्थिरांक selectp = VN_AS(nodep, CoverBinsof);
        स्थिरांक CrossBinsofTarget target = resolveBinsofTarget(selectp, crossp, cpVars, dimensions);
        जर (!target.m_binsp) परत असत्य;
        स्थिरांक CoverpointBins& bins = *target.m_binsp;
        जर (selectp->rangesp() && !bins.exprp->dtypep()->skipRefp()->isIntegralOrPacked()) {
            तार्किक valid = सत्य;
            unsupportedCrossRange(selectp, valid);
            परत असत्य;
        }
        Ast_गाठ_अभिव्यक्ती* firstp;
        Ast_गाठ_अभिव्यक्ती* endp;
        जर (target.m_sized < 0) {
            firstp = cnum(fl, target.m_declaredFirst);
            endp = cnum(fl, target.m_declaredEnd);
        } अन्यथा {  // A sized array, whose bins the coverpoint's construction placed
            AstVar* स्थिरांक cpVarp = cpVars[target.m_dimension];
            स्थिरांक uint32_t sized = static_cast<uint32_t>(target.m_sized);
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
        साठी (Ast_गाठ* rangep = selectp->rangesp(); rangep; rangep = rangep->nextp()) {
            Cross_मूल्य_Range range{rangep, resolveWidth(rangep, bins.exprp)};
            जर (!resolve_मूल्य(rangep, bins.exprp, असत्य, असत्य, range)) {
                तार्किक valid = सत्य;
                unsupportedCrossRange(selectp, valid);
                परत असत्य;
            }
            जर (crossRangeEmpty(range)) सुरूठेवा;
            m_constructorp->addStmtsp(itemCall(fl, cxp,
                                               bins.exprp->isWide()
                                                   ? VCMethod::COVERGROUP_SELECT_RANGE_W
                                                   : VCMethod::COVERGROUP_SELECT_RANGE,
                                               {new_मूल्य_Const(fl, range.lo, bins.exprp),
                                                new_मूल्य_Const(fl, range.hi, bins.exprp)})
                                          ->makeStmt());
        }
        m_constructorp->addStmtsp(
            itemCall(fl, cxp, VCMethod::COVERGROUP_SELECT_DIM_END)->makeStmt());
        परत सत्य;
    }

    std::vector<AstCoverCrossBin*>
    generateRuntimeCrossBins(AstCoverCross* crossp, AstVar* cxp,
                             स्थिरांक std::vector<AstVar*>& cpVars,
                             स्थिरांक std::map<string, uint32_t>& dimensions) {
        std::vector<AstCoverCrossBin*> bins;
        std::set<string> names;
        साठी (Ast_गाठ* itemp = crossp->binsp(); itemp; itemp = itemp->nextp()) {
            AstCoverCrossBin* स्थिरांक binp = VN_AS(itemp, CoverCrossBin);
            जर (!checkCrossBinName(binp, names)) सुरूठेवा;
            File_ओळ* स्थिरांक fl = binp->fileline();
            // Generate the selection apart, so that an unsupported one leaves no code, which
            // could leave its state for the next bin
            AstFunc* स्थिरांक selectionp = नवीन AstFunc{fl, "", nullptr, nullptr};
            तार्किक supported;
            {
                VL_RESTORER(m_constructorp);
                m_constructorp = selectionp;
                supported = generateRuntimeSelection(binp->selectp(), binp, crossp, cxp, cpVars,
                                                     dimensions);
            }
            // A supported selection has code
            जर (supported) m_constructorp->addStmtsp(selectionp->stmtsp()->unlinkFrBackWithNext());
            VL_DO_DANGLING(ढकला_Deletep(selectionp), selectionp);
            जर (!supported) सुरूठेवा;
            स्थिरांक तार्किक protect = v3Global.opt.protectIds();
            m_constructorp->addStmtsp(
                itemCall(fl, cxp, VCMethod::COVERGROUP_SELECT_BIN,
                         {ctext(fl, binp->binsType().binSetEnum()),
                          ctext(fl, quoted(VIdProtect::protect_शब्द_If(binp->name(), protect))),
                          ctext(fl, quoted(VIdProtect::protectIf(fl->filename(), protect))),
                          cnum(fl, fl->lineno()), cnum(fl, fl->firstColumn()),
                          cnum(fl, static_cast<uint32_t>(bins.size()))})
                    ->makeStmt());
            bins.ढकला_back(binp);
        }
        m_constructorp->addStmtsp(
            itemCall(crossp->fileline(), cxp, VCMethod::COVERGROUP_FINALIZE_BINS)->makeStmt());
        परत bins;
    }

    स्थिर शून्य setCrossSelectionRange(CrossSelection& selection, uint64_t first, uint64_t end) {
        पर्यंत (first < end) {
            स्थिरांक अचिन्हित bit = first % 64;
            स्थिरांक अचिन्हित bits = std::min<uint64_t>(64 - bit, end - first);
            selection[VL_BITWORD_Q(first)]
                |= (bits == 64 ? ~uint64_t{0} : (uint64_t{1} << bits) - 1) << bit;
            first += bits;
        }
    }

    CrossSelection crossSelection(Ast_गाठ* nodep, CrossSelectionContext& ctx) {
        जर (स्थिरांक AstCoverCrossRef* स्थिरांक refp = VN_CAST(nodep, CoverCrossRef)) {
            जर (!checkCrossRef(refp, ctx.crossp)) {
                ctx.valid = असत्य;
                परत {};
            }
            CrossSelection result(
                VL_BITWORD_Q(static_cast<uint64_t>(ctx.tuples) + VL_QUADSIZE - 1), 0);
            setCrossSelectionRange(result, 0, ctx.tuples);
            परत result;
        }
        जर (AstCoverCrossSelect* स्थिरांक opp = VN_CAST(nodep, CoverCrossSelect)) {
            CrossSelection lhs = crossSelection(opp->lhsp(), ctx);
            स्थिरांक CrossSelection rhs = crossSelection(opp->rhsp(), ctx);
            जर (!ctx.valid) परत {};
            साठी (माप_प्रकार i = 0; i < lhs.size(); ++i) {
                lhs[i] = opp->isOr() ? lhs[i] | rhs[i] : lhs[i] & rhs[i];
            }
            परत lhs;
        }
        AstCoverBinsof* स्थिरांक selectp = VN_AS(nodep, CoverBinsof);
        स्थिरांक CrossBinsofTarget target
            = resolveBinsofTarget(selectp, ctx.crossp, ctx.cpVars, ctx.dimensions);
        जर (!target.m_binsp) {
            ctx.valid = असत्य;
            परत {};
        }
        स्थिरांक CoverpointBins& bins = *target.m_binsp;
        // Coverpoints with sized arrays are runtime points, so feed only runtime crosses
        UASSERT_OBJ(target.m_sized < 0, selectp, "Sized bin array selected by a static cross");
        स्थिरांक std::vector<तार्किक> selected
            = selectCoverpointBins(selectp, bins, target.m_first, target.m_मोजणी, ctx.valid);
        जर (!ctx.valid) परत {};
        CrossSelection result(VL_BITWORD_Q(static_cast<uint64_t>(ctx.tuples) + VL_QUADSIZE - 1),
                              0);
        स्थिरांक uint64_t stride = ctx.strides[target.m_dimension];
        स्थिरांक uint64_t period = stride * bins.total;
        साठी (uint64_t base = 0; base < ctx.tuples; base += period) {
            साठी (uint32_t i = 0; i < bins.total;) {
                जर (!selected[i]) {
                    ++i;
                    सुरूठेवा;
                }
                स्थिरांक uint32_t begin = i++;
                पर्यंत (i < bins.total && selected[i]) ++i;
                setCrossSelectionRange(result, base + begin * stride, base + i * stride);
            }
        }
        परत result;
    }

    // Size the Cartesian product of the declared Normal bins, with each dimension's flat-index
    // stride.  Live runtime bins only shrink it.  Warns and returns false if it is too large.
    तार्किक crossShape(AstCoverCross* crossp, स्थिरांक std::vector<AstVar*>& cpVars,
                    std::vector<uint32_t>& strides, uint32_t& tuples) स्थिरांक {
        uint64_t product = std::any_of(cpVars.begin(), cpVars.end(),
                                       [हे](AstVar* varp) { परत !m_cpBins.at(varp).total; })
                               ? 0
                               : 1;
        strides.resize(cpVars.size());
        साठी (माप_प्रकार d = cpVars.size(); d > 0; --d) {
            strides[d - 1] = static_cast<uint32_t>(product);
            product *= m_cpBins.at(cpVars[d - 1]).total;
            जर (product > UINT32_MAX) {
                crossp->v3warn(COVERIGN,
                               "Unsupported: cross coverage with more than 2^32-1 tuples.");
                परत असत्य;
            }
        }
        tuples = static_cast<uint32_t>(product);
        परत सत्य;
    }

    CrossLayout resolveCrossLayout(AstCoverCross* crossp, स्थिरांक std::vector<AstVar*>& cpVars,
                                   स्थिरांक std::map<std::string, uint32_t>& dimensions) {
        CrossLayout layout;
        CrossSelectionContext ctx{crossp, cpVars, dimensions, 0, {}};
        जर (!crossShape(crossp, cpVars, ctx.strides, ctx.tuples)) {
            layout.valid = असत्य;
            परत layout;
        }
        layout.tuples = ctx.tuples;
        CrossSelection occupied;
        CrossSelection excluded;
        std::set<std::string> names;
        साठी (Ast_गाठ* itemp = crossp->binsp(); itemp; itemp = itemp->nextp()) {
            AstCoverCrossBin* स्थिरांक binp = VN_AS(itemp, CoverCrossBin);
            जर (!checkCrossBinName(binp, names)) सुरूठेवा;
            ctx.valid = सत्य;
            CrossSelection selection = crossSelection(binp->selectp(), ctx);
            जर (!ctx.valid || std::all_of(selection.begin(), selection.end(), [](uint64_t word) {
                    परत word == 0;
                })) {
                सुरूठेवा;
            }
            जर (occupied.empty()) {
                occupied.resize(selection.size(), 0);
                excluded.resize(selection.size(), 0);
            }
            साठी (माप_प्रकार i = 0; i < selection.size(); ++i) {
                occupied[i] |= selection[i];
                जर (!binp->binsType().binIsNormal()) excluded[i] |= selection[i];
            }
            layout.bins.ढकला_back({binp, std::move(selection)});
        }
        जर (!layout.bins.empty()) {
            // IEEE 1800-2023 19.6.2/19.6.3: exclusions also remove tuples from named
            // bins, independently of declaration order and sampling guards.
            साठी (ResolvedCrossBin& resolved : layout.bins) {
                साठी (माप_प्रकार i = 0; i < resolved.selection.size(); ++i) {
                    जर (resolved.binp->binsType().binIsNormal()) {
                        resolved.selection[i] &= ~excluded[i];
                    }
                    जर (resolved.selection[i]) ++layout.bin_शब्द;
                }
            }
            layout.bins.erase(std::remove_if(layout.bins.begin(), layout.bins.end(),
                                             [](स्थिरांक ResolvedCrossBin& resolved) {
                                                 परत std::all_of(
                                                     resolved.selection.begin(),
                                                     resolved.selection.end(),
                                                     [](uint64_t word) { परत word == 0; });
                                             }),
                              layout.bins.end());
            layout.autoBins = layout.tuples;
            साठी (स्थिरांक uint64_t word : occupied) {
                layout.autoBins -= static_cast<uint32_t>(std::bitset<VL_QUADSIZE>{word}.मोजणी());
            }
        }
        परत layout;
    }

    AstCoverCrossDType* crossDType(File_ओळ* fl, uint32_t dimensions, स्थिरांक CrossLayout& layout,
                                   तार्किक dynamic = असत्य) {
        स्थिरांक uint32_t bins = static_cast<uint32_t>(layout.bins.size());
        स्थिरांक CrossShape shape{dimensions,      layout.tuples,   bins,
                               layout.autoBins, layout.bin_शब्द, dynamic};
        AstCoverCrossDType*& typep = m_cxDTypes[shape];
        जर (!typep) {
            typep = नवीन AstCoverCrossDType{
                fl, dimensions, layout.tuples, bins, layout.autoBins, layout.bin_शब्द, dynamic};
            v3Global.rootp()->typeTablep()->addTypesp(typep);
        }
        परत typep;
    }

    std::vector<AstCoverCrossBin*> generateCrossBins(AstCoverCross* crossp, AstVar* cxVarp,
                                                     स्थिरांक CrossLayout& layout) {
        std::vector<AstCoverCrossBin*> bins;
        साठी (स्थिरांक ResolvedCrossBin& resolved : layout.bins) {
            AstCoverCrossBin* स्थिरांक binp = resolved.binp;
            स्थिरांक CrossSelection& selection = resolved.selection;
            File_ओळ* स्थिरांक fl = binp->fileline();
            स्थिरांक तार्किक prot = v3Global.opt.protectIds();
            std::string mask = "{";
            साठी (माप_प्रकार i = 0; i < selection.size(); ++i) {
                जर (i) mask += ", ";
                mask += std::to_string(selection[i]) + "ULL";
            }
            mask += "}";
            m_constructorp->addStmtsp(
                itemCall(fl, cxVarp, VCMethod::COVERGROUP_ADD_BIN,
                         {ctext(fl, binp->binsType().binSetEnum()), ctext(fl, mask),
                          ctext(fl, quoted(VIdProtect::protect_शब्द_If(binp->name(), prot))),
                          ctext(fl, quoted(VIdProtect::protectIf(fl->filename(), prot))),
                          cnum(fl, static_cast<uint32_t>(fl->lineno())),
                          cnum(fl, static_cast<uint32_t>(fl->firstColumn()))})
                    ->makeStmt());
            bins.ढकला_back(binp);
        }
        जर (!bins.empty()) {
            m_constructorp->addStmtsp(
                itemCall(crossp->fileline(), cxVarp, VCMethod::COVERGROUP_FINALIZE_BINS)
                    ->makeStmt());
        }
        परत bins;
    }

    // Route a cross through a VlCoverCross member: emit the member, its constructor init +
    // registration, and the sample() call.  The feeding coverpoints are already generated
    // (their hit lists drive the cross). Each explicit bin adds one configuration call.
    शून्य generateCross(AstCoverCross* crossp) {
        File_ओळ* स्थिरांक fl = crossp->fileline();
        स्थिरांक तार्किक dynamic = m_runtimeCrosses.मोजणी(crossp);
        UINFO(4, "  Generating VlCoverCross member: " << crossp->name());

        जर (Ast_गाठ_अभिव्यक्ती* स्थिरांक iffp = crossp->iffp()) {
            crossp->iffp(
                captureIffToTemp(iffp, "__VcrossIff_" + sanitizeGeneratedName(crossp->name())));
        }

        // Resolve and unlink the coverpoint refs, in dimension order.  Every ref resolves to a
        // known coverpoint (a cross with an unresolvable item was dropped earlier).
        std::vector<AstVar*> cpVars;
        std::map<std::string, uint32_t> dimensions;
        साठी (Ast_गाठ* itemp = crossp->itemsp(); itemp;) {
            Ast_गाठ* स्थिरांक nextp = itemp->nextp();
            AstCoverpointRef* स्थिरांक refp = VN_AS(itemp, CoverpointRef);
            स्थिरांक स्वतः it = m_cpVarMap.find(refp->name());
            UASSERT_OBJ(it != m_cpVarMap.end(), crossp, "Cross references an unknown coverpoint");
            dimensions.emplace(refp->name(), static_cast<uint32_t>(cpVars.size()));
            cpVars.ढकला_back(it->second);
            VL_DO_DANGLING(ढकला_Deletep(refp->unlinkFrBack()), refp);
            itemp = nextp;
        }
        स्थिरांक पूर्णांक dims = static_cast<पूर्णांक>(cpVars.size());
        CrossLayout layout;
        जर (dynamic) {
            // The runtime sizes the layout from live bins; only check the declared bound here.
            std::vector<uint32_t> strides;
            uint32_t tuples = 0;
            layout.valid = crossShape(crossp, cpVars, strides, tuples);
        } अन्यथा {
            layout = resolveCrossLayout(crossp, cpVars, dimensions);
        }
        जर (!layout.valid) परत;

        AstVar* स्थिरांक cxVarp
            = नवीन AstVar{fl, VVarType::MEMBER, "__Vcx_" + crossp->name(),
                         crossDType(fl, static_cast<uint32_t>(dims), layout, dynamic)};
        m_covergroupp->addMembersp(cxVarp);
        m_constructorp->addStmtsp(makeItemCreate(fl, cxVarp,
                                                 dynamic ? VCMethod::COVERGROUP_ADD_CROSS_DYN
                                                         : VCMethod::COVERGROUP_ADD_CROSS));
        generateItemWeight(fl, cxVarp, crossp->optionsp());
        पूर्णांक at_Least_मूल्य;
        पूर्णांक autoBinMax;  // Not a cross option
        extractItemOptions(crossp->optionsp(), at_Least_मूल्य, autoBinMax);

        // Constructor: init (after the coverpoints, which generate earlier) then registration.
        // Obfuscate the hierarchy/filename/page under --protect-ids as for coverpoints above.
        स्थिरांक तार्किक prot = v3Global.opt.protectIds();
        स्थिरांक std::string hier
            = VIdProtect::protect_शब्द_If(m_covergroupName + "." + crossp->name(), prot);
        m_constructorp->addStmtsp(makeCrossCpsCall(
            fl, cpVars,
            itemCall(fl, cxVarp, VCMethod::COVERGROUP_INIT,
                     {ctext(fl, quoted(hier)), cnum(fl, static_cast<uint32_t>(at_Least_मूल्य)),
                      cnum(fl, static_cast<uint32_t>(dims)), ctext(fl, "__Vcx_cps"),
                      ctext(fl, quoted(VIdProtect::protectIf(fl->filename(), prot))),
                      cnum(fl, static_cast<uint32_t>(fl->lineno())),
                      cnum(fl, static_cast<uint32_t>(fl->firstColumn()))})));
        स्थिरांक std::vector<AstCoverCrossBin*> bins
            = dynamic ? generateRuntimeCrossBins(crossp, cxVarp, cpVars, dimensions)
                      : generateCrossBins(crossp, cxVarp, layout);
        जर (v3Global.opt.coverage()) {
            स्थिरांक std::string page
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
        स्थिरांक तार्किक hasIffs
            = std::any_of(bins.begin(), bins.end(),
                          [](स्थिरांक AstCoverCrossBin* binp) { परत binp->iffp() != nullptr; });
        Ast_गाठ_Stmt* स्थिरांक samplep
            = !hasIffs ? static_cast<Ast_गाठ_Stmt*>(
                             itemCall(fl, cxVarp, VCMethod::COVERGROUP_SAMPLE)->makeStmt())
                       : static_cast<Ast_गाठ_Stmt*>(makeCrossIffsCall(
                             fl, bins,
                             itemCall(fl, cxVarp, VCMethod::COVERGROUP_SAMPLE_IFFS,
                                      {ctext(fl, "__Vcx_iffs")})));
        जर (Ast_गाठ_अभिव्यक्ती* स्थिरांक iffp = crossp->iffp()) {
            m_sampleFuncp->addStmtsp(नवीन AstIf{fl, iffp->clone_वृक्ष(असत्य), samplep});
        } अन्यथा {
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
        साठी (Ast_गाठ* itemp = crossp->itemsp(); itemp; itemp = itemp->nextp()) {
            स्थिरांक AstCoverpointRef* स्थिरांक refp = VN_AS(itemp, CoverpointRef);
            जर (refp->exprp()) {
                refp->v3warn(COVERIGN,
                             "Unsupported: cross of hierarchical reference (implicit coverpoint)");
                परत;
            }
        }

        // A cross naming a bare variable (implicit coverpoint, which Verilator does not
        // synthesize) is dropped entirely with a COVERIGN warning -- it produces no coverage
        // either way -- but only this cross is dropped; its sibling crosses are still generated
        // and the real coverpoints it referenced remain as independent coverpoints.
        जर (m_droppedCrosses.मोजणी(crossp)) {
            साठी (Ast_गाठ* itemp = crossp->itemsp(); itemp; itemp = itemp->nextp()) {
                स्थिरांक AstCoverpointRef* स्थिरांक refp = VN_AS(itemp, CoverpointRef);
                जर (m_coverpointMap.find(refp->name()) == m_coverpointMap.end()) {
                    refp->v3warn(COVERIGN, "Unsupported: cross of "
                                               << refp->prettyNameQ()
                                               << " which is not a coverpoint (implicit "
                                                  "coverpoint)");
                    थांबा;
                }
            }
            परत;
        }

        // Every cross that isn't dropped routes through a VlCoverCross member.
        generateCross(crossp);
    }

    Ast_गाठ_अभिव्यक्ती* buildBinCondition(AstCoverBin* binp, Ast_गाठ_अभिव्यक्ती* exprp) {
        // Get the range list from the bin
        Ast_गाठ* स्थिरांक rangep = binp->rangesp();
        जर (!rangep) परत nullptr;

        // Integral values resolve to the coverpoint's type, as its runtime metadata does
        स्थिरांक तार्किक integral = exprp->dtypep()->skipRefp()->isIntegralOrPacked();
        // No value form of a wildcard bin is allowed on a real coverpoint (IEEE 1800-2023 19.5.4)
        जर (binp->isWildcard() && !integral) परत wildcardTypeError(binp, exprp);

        // Build condition by OR-ing all ranges together
        Ast_गाठ_अभिव्यक्ती* fullCondp = nullptr;

        साठी (Ast_गाठ* currRangep = rangep; currRangep; currRangep = currRangep->nextp()) {
            Ast_गाठ_अभिव्यक्ती* rangeCondp = nullptr;
            currRangep = V3Const::constifyEdit(currRangep);

            जर (AstInsideRange* irp = VN_CAST(currRangep, InsideRange)) {
                Ast_गाठ_अभिव्यक्ती* स्थिरांक minExprp = irp->lhsp();
                Ast_गाठ_अभिव्यक्ती* स्थिरांक maxExprp = irp->rhsp();
                AstConst* स्थिरांक minConstp = VN_CAST(minExprp, Const);
                AstConst* स्थिरांक maxConstp = VN_CAST(maxExprp, Const);
                स्थिरांक तार्किक loUnbounded = VN_IS(minExprp, Unbounded);
                स्थिरांक तार्किक hiUnbounded = VN_IS(maxExprp, Unbounded);
                जर (loUnbounded || hiUnbounded) {
                    // Open-ended range: '$' is the coverpoint domain min/max, so the
                    // range reduces to a single inequality (e.g. {[10:$]} -> expr >= 10).
                    AstConst* स्थिरांक boundp = hiUnbounded ? minConstp : maxConstp;
                    जर (loUnbounded && hiUnbounded) {
                        rangeCondp = नवीन AstConst{irp->fileline(), AstConst::BitTrue{}};
                    } अन्यथा जर (!boundp) {
                        irp->v3error("Non-constant expression in bin range; "
                                     "range bounds must be constants (IEEE 1800-2023 19.5)");
                        जर (fullCondp) VL_DO_DANGLING(ढकला_Deletep(fullCondp), fullCondp);
                        परत nullptr;
                    } अन्यथा जर (boundp->अंक().isFourState()) {
                        irp->v3error("Four-state (x/z) value in bin range bound; "
                                     "range bounds must be two-state constants");
                        जर (fullCondp) VL_DO_DANGLING(ढकला_Deletep(fullCondp), fullCondp);
                        परत nullptr;
                    } अन्यथा जर (integral) {
                        rangeCondp = build_मूल्य_Condition(binp, exprp, irp);
                    } अन्यथा {
                        rangeCondp = makeOpenRangeCondition(irp->fileline(), exprp, boundp,
                                                            /*isLowerBound=*/hiUnbounded);
                    }
                } अन्यथा जर (!minConstp || !maxConstp) {
                    irp->v3error("Non-constant expression in bin range; "
                                 "range bounds must be constants (IEEE 1800-2023 19.5)");
                    जर (fullCondp) VL_DO_DANGLING(ढकला_Deletep(fullCondp), fullCondp);
                    परत nullptr;
                } अन्यथा जर (minConstp->अंक().isFourState() || maxConstp->अंक().isFourState()) {
                    irp->v3error("Four-state (x/z) value in bin range bound; "
                                 "range bounds must be two-state constants");
                    जर (fullCondp) VL_DO_DANGLING(ढकला_Deletep(fullCondp), fullCondp);
                    परत nullptr;
                } अन्यथा जर (integral) {
                    rangeCondp = build_मूल्य_Condition(binp, exprp, irp);
                } अन्यथा {
                    rangeCondp = makeRangeCondition(irp->fileline(), exprp, minExprp, maxExprp);
                }
            } अन्यथा जर (AstConst* constp = VN_CAST(currRangep, Const)) {
                rangeCondp = build_मूल्य_Condition(binp, exprp, constp);
            } अन्यथा {
                currRangep->v3error("Non-constant expression in bin range; values must be "
                                    "constants (IEEE 1800-2023 19.5)");
                जर (fullCondp) VL_DO_DANGLING(ढकला_Deletep(fullCondp), fullCondp);
                परत nullptr;
            }

            UASSERT_OBJ(rangeCondp, binp, "rangeCondp is null after building range condition");
            fullCondp
                = fullCondp ? नवीन AstOr{binp->fileline(), fullCondp, rangeCondp} : rangeCondp;
        }

        परत fullCondp;
    }

    // Wildcard bits have no meaning for a non-integral coverpoint.
    स्थिर Ast_गाठ_अभिव्यक्ती* wildcardTypeError(AstCoverBin* binp, Ast_गाठ_अभिव्यक्ती* exprp) {
        स्थिरांक Ast_गाठ_DType* स्थिरांक dtypep = exprp->dtypep()->skipRefp();
        exprp->v3error("Cannot use a wildcard bin on a coverpoint of type "
                       << dtypep->prettyDTypeNameQ() << " (IEEE 1800-2023 19.5.4).\n"
                       << exprp->warnContextPrimary() << '\n'
                       << binp->warnOther() << "... Location of wildcard bin\n"
                       << binp->warnContextSecondary());
        परत नवीन AstConst{binp->fileline(), AstConst::BitFalse{}};
    }

    // Match one bin value, range, or wildcard pattern.  Integral coverpoints first resolve the
    // value to their type (IEEE 1800-2023 19.5.7).  Non-owning: clones what it uses.
    Ast_गाठ_अभिव्यक्ती* build_मूल्य_Condition(AstCoverBin* binp, Ast_गाठ_अभिव्यक्ती* exprp, Ast_गाठ* valuep) {
        File_ओळ* स्थिरांक fl = valuep->fileline();
        जर (!exprp->dtypep()->skipRefp()->isIntegralOrPacked()) {
            परत नवीन AstEq{fl, exprp->clone_वृक्ष(असत्य),
                             VN_AS(valuep, गाठ_अभिव्यक्ती)->clone_वृक्ष(असत्य)};
        }
        Cross_मूल्य_Range range{valuep, resolveWidth(valuep, exprp)};
        जर (!resolve_मूल्य(valuep, exprp, सत्य, binp->isWildcard(), range)) {
            valuep->v3warn(E_UNSUPPORTED,
                           "Unsupported: non-integral value in a coverage bin of an "
                           "integral coverpoint.");
            परत नवीन AstConst{fl, AstConst::BitFalse{}};
        }
        जर (crossRangeEmpty(range)) परत नवीन AstConst{fl, AstConst::BitFalse{}};
        AstConst* स्थिरांक lop = new_मूल्य_Const(fl, range.lo, exprp);
        AstConst* स्थिरांक hip = new_मूल्य_Const(fl, range.hi, exprp);
        Ast_गाठ_अभिव्यक्ती* condp = nullptr;
        जर (lop->अंक().isCaseEq(hip->अंक())) {
            condp = नवीन AstEq{fl, exprp->clone_वृक्ष(असत्य), lop};
        } अन्यथा {
            condp = makeRangeCondition(fl, exprp, lop, hip);
            VL_DO_DANGLING(ढकला_Deletep(lop), lop);
        }
        VL_DO_DANGLING(ढकला_Deletep(hip), hip);
        जर (!range.wildcard) परत condp;
        // Match the pattern's value bits within the source-value bounds.
        V3_संख्या mask{valuep, exprp->width()};
        V3_संख्या मूल्य{valuep, exprp->width()};
        mask.opBitsNonXZ(range.pattern);
        मूल्य.opBitsOne(range.pattern);
        AstConst* स्थिरांक maskConstp = new_मूल्य_Const(fl, mask, exprp);
        AstConst* स्थिरांक मूल्य_Constp = new_मूल्य_Const(fl, मूल्य, exprp);
        Ast_गाठ_अभिव्यक्ती* स्थिरांक अभिव्यक्ती_Masked = नवीन AstAnd{fl, exprp->clone_वृक्ष(असत्य), maskConstp};
        Ast_गाठ_अभिव्यक्ती* स्थिरांक मूल्य_Masked = नवीन AstAnd{fl, मूल्य_Constp, maskConstp->clone_वृक्ष(असत्य)};
        परत नवीन AstLogAnd{fl, condp, नवीन AstEq{fl, अभिव्यक्ती_Masked, मूल्य_Masked}};
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
        AstFunc* स्थिरांक getInstCoveragep
            = VN_AS(m_memberMap.findMember(m_covergroupp, "get_inst_coverage"), Func);
        File_ओळ* स्थिरांक instFl = getInstCoveragep->fileline();
        AstCMethodHard* स्थिरांक instCallp = instanceCall(instFl, VCMethod::COVERGROUP_COVERAGE);
        instCallp->dtypeSetDouble();
        Ast_गाठ_अभिव्यक्ती* instValuep = instCallp;
        जर (m_cgMayMerge) {
            Ast_गाठ_अभिव्यक्ती* स्थिरांक mergedp = नवीन AstLogAnd{
                instFl, newOptionSel(instFl, optionVar(सत्य), "merge_instances", VAccess::READ),
                नवीन AstLogNot{instFl, newOptionSel(instFl, optionVar(असत्य), "get_inst_coverage",
                                                   VAccess::READ)}};
            instValuep = नवीन AstCond{instFl, mergedp, typeCoverageCall(instFl), instCallp};
        }
        getInstCoveragep->addStmtsp(नवीन AstAssign{
            instFl, नवीन AstVarRef{instFl, VN_AS(getInstCoveragep->fvarp(), Var), VAccess::WRITE},
            instValuep});

        // get_coverage(): the average of the covergroup's instances, weighted by their
        // option.weight, or with type_option.merge_instances, the coverage of the union of their
        // bins (IEEE 1800-2023 19.11.3).  Static, so the registry finds the instances.
        AstFunc* स्थिरांक getCoveragep
            = VN_AS(m_memberMap.findMember(m_covergroupp, "get_coverage"), Func);
        File_ओळ* स्थिरांक typeFl = getCoveragep->fileline();
        getCoveragep->addStmtsp(नवीन AstAssign{
            typeFl, नवीन AstVarRef{typeFl, VN_AS(getCoveragep->fvarp(), Var), VAccess::WRITE},
            typeCoverageCall(typeFl)});
    }

    // The registry call computing the covergroup's type coverage, per its type options
    AstCMethodHard* typeCoverageCall(File_ओळ* fl) {
        AstCExpr* स्थिरांक registryp = ctext(fl, "vlSymsp->_vm_contextp__->covergroupRegistryp()");
        registryp->dtypeSetVoid();  // Opaque receiver; only ever the 'fromp' of the call below
        AstCMethodHard* स्थिरांक callp
            = नवीन AstCMethodHard{fl, registryp, VCMethod::COVERGROUP_TYPE_COVERAGE};
        callp->addPinsp(ctext(fl, quoted(covergroupProtectedName())));
        callp->addPinsp(newOptionSel(fl, optionVar(सत्य), "weight", VAccess::READ));
        callp->addPinsp(newOptionSel(fl, optionVar(सत्य), "merge_instances", VAccess::READ));
        callp->addPinsp(file_ओळ_Debug(m_covergroupp->fileline()));
        callp->usePtr(सत्य);
        callp->dtypeSetDouble();
        परत callp;
    }

    // VISITORS
    स्थिर तार्किक isEnclosingInstanceVar(स्थिरांक AstVar* varp) {
        परत varp->isClassMember() && !varp->lifetime().isStatic() && !varp->isParam();
    }

    शून्य rewriteThisRef(AstThisRef* refp, AstVar* handleVarp) {
        स्थिरांक AstClassRefDType* स्थिरांक refDTypep
            = VN_CAST(refp->dtypep()->skipRefp(), ClassRefDType);
        UASSERT_OBJ(refDTypep && refDTypep->classp() == m_covergroupp, refp,
                    "Unexpected this reference in embedded covergroup");
        Ast_गाठ_अभिव्यक्ती* स्थिरांक newp = नवीन AstVarRef{refp->fileline(), handleVarp, VAccess::READ};
        refp->replaceWith(newp);
        VL_DO_DANGLING(ढकला_Deletep(refp), refp);
    }

    शून्य rewriteVarRef(AstVarRef* refp, AstVar* handleVarp) {
        File_ओळ* स्थिरांक fl = refp->fileline();
        // As V3LinkLValue does, the handle of a written member select is marked written
        AstMemberSel* स्थिरांक selp
            = नवीन AstMemberSel{fl, नवीन AstVarRef{fl, handleVarp, refp->access()}, refp->varp()};
        selp->access(refp->access());
        refp->replaceWith(selp);
        VL_DO_DANGLING(ढकला_Deletep(refp), refp);
    }

    शून्य rewriteFuncRef(AstFuncRef* refp, AstVar* handleVarp) {
        File_ओळ* स्थिरांक fl = refp->fileline();
        AstArg* स्थिरांक argsp = refp->argsp() ? refp->argsp()->unlinkFrBackWithNext() : nullptr;
        AstMethodCall* स्थिरांक callp = नवीन AstMethodCall{
            fl, नवीन AstVarRef{fl, handleVarp, VAccess::READ}, refp->name(), argsp};
        callp->taskp(refp->taskp());
        callp->dtypeFrom(refp);
        refp->replaceWith(callp);
        VL_DO_DANGLING(ढकला_Deletep(refp), refp);
    }

    // True if funcp is an instance method of the enclosing class or one of its bases
    तार्किक isEnclosingInstanceFunc(स्थिरांक Ast_गाठ_FTask* funcp) स्थिरांक {
        जर (!funcp->classMethod() || funcp->isStatic()) परत असत्य;
        परत AstClass::isClassExtendedFrom(m_enclosingClassp, VN_AS(funcp->aboveLoopp(), Class));
    }

    तार्किक isEmbeddedCovergroupVar(स्थिरांक AstVar* varp) स्थिरांक {
        जर (!varp || !varp->isClassMember() || varp->isDeclTyped()) परत असत्य;
        स्थिरांक AstClassRefDType* स्थिरांक refp = VN_CAST(varp->dtypep()->skipRefp(), ClassRefDType);
        परत refp && refp->classp() == m_covergroupp;
    }

    AstVar* findEmbeddedCovergroupVar() स्थिरांक {
        जर (!m_enclosingClassp) परत nullptr;
        साठी (Ast_गाठ* itemp = m_enclosingClassp->membersp(); itemp; itemp = itemp->nextp()) {
            जर (AstVar* स्थिरांक varp = VN_CAST(itemp, Var)) {
                जर (isEmbeddedCovergroupVar(varp)) परत varp;
            }
        }
        // V3LinkParse always creates an implicit variable for an embedded covergroup.
        परत nullptr;  // LCOV_EXCL_LINE
    }

    std::vector<Ast_गाठ_Assign*> findCovergroupConstructions() {
        std::vector<Ast_गाठ_Assign*> foundps;
        जर (!m_embeddedVarp) परत foundps;
        AstFunc* स्थिरांक enclosingNewp
            = VN_CAST(m_memberMap.findMember(m_enclosingClassp, "new"), Func);
        जर (!enclosingNewp) परत foundps;
        enclosingNewp->foreach([&](Ast_गाठ_Assign* asgnp) {
            स्थिरांक AstNew* स्थिरांक newp = VN_CAST(asgnp->rhsp(), New);
            स्थिरांक AstVarRef* स्थिरांक lhsRefp = VN_CAST(asgnp->lhsp(), VarRef);
            जर (!newp || !lhsRefp || lhsRefp->varp() != m_embeddedVarp) परत;
            स्थिरांक AstClassRefDType* स्थिरांक refp = VN_CAST(newp->dtypep(), ClassRefDType);
            जर (refp && refp->classp() == m_covergroupp) foundps.ढकला_back(asgnp);
        });
        परत foundps;
    }

    std::set<स्थिरांक AstVar*> enclosingInstanceVars() स्थिरांक {
        std::set<स्थिरांक AstVar*> vars;
        जर (m_enclosingClassp) {
            m_enclosingClassp->foreachMember([&](AstClass* स्थिरांक, AstVar* स्थिरांक varp) {
                जर (isEnclosingInstanceVar(varp)) vars.घाला(varp);
            });
        }
        परत vars;
    }

    तार्किक hasEnclosingEventRef(AstCovergroup* cgp) स्थिरांक {
        जर (!m_embeddedVarp || !cgp->eventp()) परत असत्य;
        स्थिरांक std::set<स्थिरांक AstVar*> enclosingVars = enclosingInstanceVars();
        तार्किक found = असत्य;
        cgp->eventp()->foreach([&](AstVarRef* refp) {
            जर (enclosingVars.मोजणी(refp->varp())) found = सत्य;
        });
        परत found;
    }

    स्थिर तार्किक parse_Embedded_Event_अभिव्यक्ती(Ast_गाठ_अभिव्यक्ती* exprp, AstVar*& baseVarp,
                                       AstVar*& memberVarp) {
        जर (AstVarRef* स्थिरांक refp = VN_CAST(exprp, VarRef)) {
            baseVarp = refp->varp();
            memberVarp = nullptr;
            परत सत्य;
        }
        AstMemberSel* स्थिरांक selp = VN_CAST(exprp, MemberSel);
        जर (!selp) परत असत्य;
        AstVarRef* स्थिरांक baseRefp = VN_CAST(selp->fromp(), VarRef);
        जर (!baseRefp) परत असत्य;
        baseVarp = baseRefp->varp();
        memberVarp = selp->varp();
        परत सत्य;
    }

    तार्किक isEventLvalue(Ast_गाठ_अभिव्यक्ती* exprp, स्थिरांक EmbeddedEventTrigger& trigger) स्थिरांक {
        जर (AstSel* स्थिरांक selp = VN_CAST(exprp, Sel)) exprp = selp->fromp();
        AstVar* baseVarp = nullptr;
        AstVar* memberVarp = nullptr;
        जर (!parse_Embedded_Event_अभिव्यक्ती(exprp, baseVarp, memberVarp)) परत असत्य;
        परत baseVarp == trigger.baseVarp && memberVarp == trigger.memberVarp;
    }

    Ast_गाठ_अभिव्यक्ती* newEventRead(File_ओळ* fl, स्थिरांक EmbeddedEventTrigger& trigger) स्थिरांक {
        Ast_गाठ_अभिव्यक्ती* स्थिरांक basep = नवीन AstVarRef{fl, trigger.baseVarp, VAccess::READ};
        जर (!trigger.memberVarp) परत basep;
        AstMemberSel* स्थिरांक selp = नवीन AstMemberSel{fl, basep, trigger.memberVarp};
        selp->access(VAccess::READ);
        परत selp;
    }

    string eventPrevName(स्थिरांक EmbeddedEventTrigger& trigger, माप_प्रकार triggerIndex) स्थिरांक {
        string name = "__Vcg_prev_" + m_embeddedVarp->name() + "_" + std::to_string(triggerIndex)
                      + "_" + trigger.baseVarp->name();
        जर (trigger.memberVarp) name += "_" + trigger.memberVarp->name();
        परत name;
    }

    Ast_गाठ_अभिव्यक्ती* newEmbeddedVarNonNull(File_ओळ* fl) स्थिरांक {
        परत नवीन AstNeq{fl, नवीन AstVarRef{fl, m_embeddedVarp, VAccess::READ},
                          नवीन AstConst{fl, AstConst::Null{}}};
    }

    Ast_गाठ_Stmt* newSampleStmt(File_ओळ* fl) स्थिरांक {
        AstMethodCall* स्थिरांक callp = नवीन AstMethodCall{
            fl, नवीन AstVarRef{fl, m_embeddedVarp, VAccess::READ}, "sample", nullptr};
        callp->taskp(m_sampleFuncp);
        callp->dtypeSetVoid();
        परत callp->makeStmt();
    }

    शून्य installEmbeddedEventFork(Ast_Sen_वृक्ष* eventp,
                                  स्थिरांक std::vector<Ast_गाठ_Assign*>& constructps) {
        // IEEE 1800-2023 19.3 samples coverpoints whenever their clocking event occurs. A
        // per-instance event cannot use V3Active's static sensitivity path, so spawn
        // 'fork forever begin @(event); cg.sample(); end join_none' after each construction.
        साठी (Ast_गाठ_Assign* स्थिरांक constructp : constructps) {
            File_ओळ* स्थिरांक fl = constructp->fileline();
            AstLoop* स्थिरांक loopp = नवीन AstLoop{fl};
            loopp->addStmtsp(नवीन AstEventControl{fl, eventp->clone_वृक्ष(असत्य), nullptr});
            loopp->addStmtsp(नवीन AstIf{fl, newEmbeddedVarNonNull(fl), newSampleStmt(fl)});
            AstFork* स्थिरांक forkp = नवीन AstFork{fl, VJoinType::JOIN_NONE};
            forkp->immediateStart(सत्य);
            forkp->addForksp(नवीन AstBegin{fl, "", loopp, सत्य});
            constructp->addNextHere(forkp);
        }
        VL_DO_DANGLING(ढकला_Deletep(eventp), eventp);
    }

    Ast_गाठ_अभिव्यक्ती* newEventReadyCondition(File_ओळ* fl, स्थिरांक EmbeddedEventTrigger& trigger) स्थिरांक {
        Ast_गाठ_अभिव्यक्ती* स्थिरांक curp = newEventRead(fl, trigger);
        Ast_गाठ_अभिव्यक्ती* स्थिरांक prevp = नवीन AstVarRef{fl, trigger.prevVarp, VAccess::READ};
        Ast_गाठ_अभिव्यक्ती* edgep = nullptr;
        // IEEE 1800-2023 9.4.2 detects edge-qualified events only on the expression's LSB,
        // while an implicit change event observes the complete expression.
        जर (trigger.edgeType == VEdgeType::ET_POSEDGE) {
            edgep = नवीन AstSel{fl, नवीन AstAnd{fl, curp, नवीन AstNot{fl, prevp}}, 0, 1};
        } अन्यथा जर (trigger.edgeType == VEdgeType::ET_NEGEDGE) {
            edgep = नवीन AstSel{fl, नवीन AstAnd{fl, नवीन AstNot{fl, curp}, prevp}, 0, 1};
        } अन्यथा जर (trigger.edgeType == VEdgeType::ET_BOTHEDGE) {
            edgep = नवीन AstSel{fl, नवीन AstXor{fl, curp, prevp}, 0, 1};
        } अन्यथा {
            edgep = नवीन AstNeq{fl, curp, prevp};
        }
        परत नवीन AstLogAnd{fl, newEmbeddedVarNonNull(fl), edgep};
    }

    std::vector<EmbeddedEventTrigger> collectEmbeddedEventTriggers(AstCovergroup* cgp) {
        std::vector<EmbeddedEventTrigger> triggers;
        स्थिरांक std::set<स्थिरांक AstVar*> enclosingVars = enclosingInstanceVars();
        साठी (Ast_गाठ* senp = cgp->eventp()->sensesp(); senp; senp = senp->nextp()) {
            AstSenItem* स्थिरांक itemp = VN_AS(senp, SenItem);
            AstVar* baseVarp = nullptr;
            AstVar* memberVarp = nullptr;
            जर (!parse_Embedded_Event_अभिव्यक्ती(itemp->sensp(), baseVarp, memberVarp)
                || !enclosingVars.मोजणी(baseVarp)) {
                परत {};
            }
            triggers.emplace_back(itemp->fileline(), baseVarp, memberVarp, itemp->edgeType());
        }
        परत triggers;
    }

    शून्य installEmbeddedEventTriggers(std::vector<EmbeddedEventTrigger>& triggers,
                                      स्थिरांक std::vector<Ast_गाठ_Assign*>& constructps) {
        // Without --timing, approximate a per-instance event by sampling after assignments
        // within the enclosing class. External writes and exact scheduling cannot be observed.
        जर (constructps.empty()) परत;
        साठी (माप_प्रकार triggerIndex = 0; triggerIndex < triggers.size(); ++triggerIndex) {
            EmbeddedEventTrigger& trigger = triggers[triggerIndex];
            std::vector<Ast_गाठ_Assign*> assignps;
            m_enclosingClassp->foreach([&](Ast_गाठ_Assign* asgnp) {
                जर (isEventLvalue(asgnp->lhsp(), trigger)) assignps.ढकला_back(asgnp);
            });
            जर (assignps.empty()) {
                trigger.eventFl->v3warn(
                    COVERIGN, "Unsupported: 'covergroup' clocking event signal has no assignment "
                              "within the enclosing class; no coverage sampled. Use --timing for "
                              "full support.");
                सुरूठेवा;
            }
            Ast_गाठ_DType* स्थिरांक dtypep
                = trigger.memberVarp ? trigger.memberVarp->dtypep() : trigger.baseVarp->dtypep();
            AstVar* स्थिरांक prevVarp = नवीन AstVar{trigger.eventFl, VVarType::MEMBER,
                                                eventPrevName(trigger, triggerIndex), dtypep};
            m_enclosingClassp->addMembersp(prevVarp);
            trigger.prevVarp = prevVarp;
            साठी (Ast_गाठ_Assign* स्थिरांक asgnp : assignps) {
                File_ओळ* स्थिरांक fl = asgnp->fileline();
                AstIf* स्थिरांक ifp
                    = नवीन AstIf{fl, newEventReadyCondition(fl, trigger), newSampleStmt(fl)};
                ifp->addNextHere(नवीन AstAssign{fl,
                                               नवीन AstVarRef{fl, trigger.prevVarp, VAccess::WRITE},
                                               newEventRead(fl, trigger)});
                asgnp->addNextHere(ifp);
            }
        }
    }

    शून्य delete_Coverage_वस्तुसंच() {
        साठी (AstCoverpoint* स्थिरांक cpp : m_coverpoints) {
            VL_DO_DANGLING(ढकला_Deletep(cpp->unlinkFrBack()), cpp);
        }
        साठी (AstCoverCross* स्थिरांक crossp : m_coverCrosses) {
            VL_DO_DANGLING(ढकला_Deletep(crossp->unlinkFrBack()), crossp);
        }
        // Options not lowered: the covergroup was not processed
        साठी (AstCgOptionAssign* स्थिरांक optp : m_cgOptions) {
            VL_DO_DANGLING(ढकला_Deletep(optp->unlinkFrBack()), optp);
        }
        m_cgOptions.clear();
    }

    वर्ग FormalRefVisitor final : सार्वजनिक VNVisitor {
        स्थिरांक std::map<स्थिरांक AstVar*, AstVar*>& m_replacements;

        शून्य visit(AstVarRef* nodep) override {
            स्थिरांक स्वतः it = m_replacements.find(nodep->varp());
            जर (it == m_replacements.end()) परत;
            nodep->varp(it->second);
        }
        शून्य visit(Ast_गाठ* nodep) override { iterateChildren(nodep); }

    सार्वजनिक:
        explicit FormalRefVisitor(स्थिरांक std::map<स्थिरांक AstVar*, AstVar*>& replacements)
            : m_replacements{replacements} {}
        शून्य scan(Ast_गाठ* nodep) { iterate(nodep); }
    };

    शून्य validateCovergroupExpressions() {
        std::set<स्थिरांक AstVar*> sampleMembers;
        std::set<स्थिरांक AstVar*> constructorRefMembers;
        साठी (Ast_गाठ* stmtp = m_constructorp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            स्थिरांक AstVar* स्थिरांक varp = VN_CAST(stmtp, Var);
            जर (!varp || !varp->isIO() || (!varp->isRef() && !varp->isConstRef())) सुरूठेवा;
            स्थिरांक AstVar* स्थिरांक memberp
                = VN_CAST(m_memberMap.findMember(m_covergroupp, varp->name()), Var);
            UASSERT_OBJ(memberp && memberp->isClassMember(), varp,
                        "Covergroup constructor argument missing persistent member");
            constructorRefMembers.घाला(varp);
            constructorRefMembers.घाला(memberp);
        }
        साठी (Ast_गाठ* stmtp = m_sampleFuncp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            स्थिरांक AstVar* स्थिरांक varp = VN_CAST(stmtp, Var);
            जर (!varp || !varp->isIO()) सुरूठेवा;
            स्थिरांक AstVar* स्थिरांक memberp
                = VN_CAST(m_memberMap.findMember(m_covergroupp, varp->name()), Var);
            UASSERT_OBJ(memberp && memberp->isClassMember(), varp,
                        "Covergroup sample argument missing persistent member");
            sampleMembers.घाला(memberp);
        }
        Covergroup_अभिव्यक्ती_Valid_Visitor{sampleMembers, constructorRefMembers}.scan(m_constructorp);
    }

    शून्य rebindFormalRefs() {
        std::map<स्थिरांक AstVar*, AstVar*> replacements;
        साठी (Ast_गाठ* stmtp = m_constructorp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            जर (स्थिरांक AstVar* स्थिरांक varp = VN_CAST(stmtp, Var)) {
                जर (!varp->isIO()) सुरूठेवा;
                AstVar* स्थिरांक memberp
                    = VN_CAST(m_memberMap.findMember(m_covergroupp, varp->name()), Var);
                UASSERT_OBJ(memberp && memberp->isClassMember(), varp,
                            "Covergroup constructor argument missing persistent member");
                replacements.emplace(varp, memberp);
            }
        }
        माप_प्रकार expectedBindings = 0;
        साठी (स्थिरांक स्वतः& pair : replacements) {
            जर (pair.first->isRef() || pair.first->isConstRef()) ++expectedBindings;
        }
        माप_प्रकार rewrittenBindings = 0;
        साठी (Ast_गाठ* stmtp = m_constructorp->stmtsp(); stmtp;) {
            Ast_गाठ* स्थिरांक nextp = stmtp->nextp();
            जर (AstAssign* स्थिरांक assignp = VN_CAST(stmtp, Assign)) {
                AstVarRef* स्थिरांक lhsp = VN_CAST(assignp->lhsp(), VarRef);
                AstVarRef* स्थिरांक rhsp = VN_CAST(assignp->rhsp(), VarRef);
                जर (lhsp && rhsp
                    && (lhsp->varp()->declDirection() == VDirection::REF
                        || lhsp->varp()->declDirection() == VDirection::CONSTREF)) {
                    स्थिरांक स्वतः it = replacements.find(rhsp->varp());
                    UASSERT_OBJ(it != replacements.end() && it->second == lhsp->varp(), assignp,
                                "Unexpected covergroup reference binding assignment");
                    AstCExpr* स्थिरांक bindp = नवीन AstCExpr{assignp->fileline(), ""};
                    bindp->add(lhsp->unlinkFrBack());
                    bindp->add(" = &");
                    bindp->add(rhsp->unlinkFrBack());
                    assignp->replaceWith(bindp->makeStmt());
                    VL_DO_DANGLING(ढकला_Deletep(assignp), assignp);
                    ++rewrittenBindings;
                }
            }
            stmtp = nextp;
        }
        UASSERT_OBJ(rewrittenBindings == expectedBindings, m_constructorp,
                    "Covergroup reference argument missing binding");
        FormalRefVisitor visitor{replacements};
        साठी (AstCoverpoint* स्थिरांक cpp : m_coverpoints) visitor.scan(cpp);
        साठी (AstCoverCross* स्थिरांक crossp : m_coverCrosses) visitor.scan(crossp);
    }

    AstVarRef* installEnclosingBackPointer(स्थिरांक std::vector<Ast_गाठ_Assign*>& constructps) {
        // Simple-case support for embedded covergroups (IEEE 1800-2023 19.4) whose
        // coverpoints reference members of the enclosing class ("Class members can be used
        // in coverpoint expressions").  The covergroup is lowered into a sibling class with
        // no implicit handle to the enclosing object, so such references would emit
        // uncompilable C++.  Add an explicit back-pointer member to the enclosing instance,
        // route member references through it, and pass it into the constructor so
        // coverage initialization can read enclosing members. Returns an invalid
        // reference if an outer class member cannot be reached; otherwise returns an empty result.
        जर (!m_enclosingClassp) परत nullptr;  // Offending refs require an enclosing class

        AstVarRef* invalidp = nullptr;
        Ast_गाठ* offenderp = nullptr;
        std::set<स्थिरांक AstVar*> ownVars;
        साठी (Ast_गाठ* itemp = m_covergroupp->membersp(); itemp; itemp = itemp->nextp()) {
            जर (स्थिरांक AstVar* स्थिरांक varp = VN_CAST(itemp, Var)) ownVars.घाला(varp);
        }
        स्थिरांक std::set<स्थिरांक AstVar*> enclosingVars = enclosingInstanceVars();
        std::vector<AstVarRef*> refsToRewrite;
        std::vector<AstThisRef*> thisRefsToRewrite;
        std::vector<AstFuncRef*> funcRefsToRewrite;
        स्थिरांक स्वतः scan = [&](Ast_गाठ* rootp) {
            rootp->foreach([&](AstVarRef* refp) {
                जर (invalidp) परत;
                स्थिरांक AstVar* स्थिरांक varp = refp->varp();
                जर (!isEnclosingInstanceVar(varp) || ownVars.मोजणी(varp)) परत;
                जर (!enclosingVars.मोजणी(varp)) {
                    invalidp = refp;
                    परत;
                }
                refsToRewrite.ढकला_back(refp);
                जर (!offenderp) offenderp = refp;
            });
            जर (invalidp) परत;
            rootp->foreach([&](AstThisRef* refp) {
                स्थिरांक AstClassRefDType* स्थिरांक refDTypep
                    = VN_CAST(refp->dtypep()->skipRefp(), ClassRefDType);
                जर (refDTypep && refDTypep->classp() == m_covergroupp) {
                    thisRefsToRewrite.ढकला_back(refp);
                    जर (!offenderp) offenderp = refp;
                }
            });
            rootp->foreach([&](AstFuncRef* refp) {
                जर (!isEnclosingInstanceFunc(refp->taskp())) परत;
                funcRefsToRewrite.ढकला_back(refp);
                जर (!offenderp) offenderp = refp;
            });
        };
        साठी (AstCoverpoint* स्थिरांक cpp : m_coverpoints) scan(cpp);
        साठी (AstCoverCross* स्थिरांक crossp : m_coverCrosses) scan(crossp);
        साठी (AstCgOptionAssign* स्थिरांक optp : m_cgOptions) scan(optp);
        जर (invalidp || !offenderp) परत invalidp;

        UASSERT_OBJ(m_embeddedVarp, m_covergroupp, "Embedded covergroup variable not found");
        // Commit: add the back-pointer member, rewrite the references, initialize the handle.
        File_ओळ* स्थिरांक fl = m_covergroupp->fileline();
        AstClassRefDType* स्थिरांक enclDTypep = नवीन AstClassRefDType{fl, m_enclosingClassp, nullptr};
        enclDTypep->rawPointer(सत्य);
        v3Global.rootp()->typeTablep()->addTypesp(enclDTypep);
        AstVar* स्थिरांक handleVarp
            = नवीन AstVar{fl, VVarType::MEMBER, "__Vcg_enclosingp", enclDTypep};
        m_covergroupp->addMembersp(handleVarp);
        AstVar* स्थिरांक argumentp = नवीन AstVar{fl, VVarType::BLOCKTEMP, "__Vcg_parentp", enclDTypep};
        argumentp->direction(VDirection::आदान);
        argumentp->declDirection(VDirection::आदान);
        argumentp->funcLocal(सत्य);
        argumentp->noReset(सत्य);
        argumentp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
        m_constructorp->addStmtsp(argumentp);
        m_constructorp->prependStmtsp(नवीन AstAssign{fl, memberRef(fl, handleVarp, VAccess::WRITE),
                                                    नवीन AstVarRef{fl, argumentp, VAccess::READ}});

        // Route each enclosing-member reference through the back-pointer: 'm' -> 'h.m'.
        साठी (AstVarRef* स्थिरांक refp : refsToRewrite) { rewriteVarRef(refp, handleVarp); }
        साठी (AstThisRef* स्थिरांक refp : thisRefsToRewrite) { rewriteThisRef(refp, handleVarp); }
        साठी (AstFuncRef* स्थिरांक refp : funcRefsToRewrite) { rewriteFuncRef(refp, handleVarp); }

        // Append a named hidden argument to preserve positional and defaulted user arguments.
        साठी (Ast_गाठ_Assign* स्थिरांक constructp : constructps) {
            File_ओळ* स्थिरांक cfl = constructp->fileline();
            AstCExpr* स्थिरांक thisp = नवीन AstCExpr{cfl, "this"};
            thisp->dtypep(enclDTypep);
            VN_AS(constructp->rhsp(), New)->addArgsp(नवीन AstArg{cfl, argumentp->name(), thisp});
        }
        परत nullptr;
    }

    शून्य visit(AstClass* nodep) override {
        UINFO(9, "Visiting class: " << nodep->name() << " isCovergroup=" << nodep->isCovergroup());
        जर (nodep->isCovergroup()) {
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
            Ast_Sen_वृक्ष* embeddedEventForkp = nullptr;

            // Extract and store the clocking event from AstCovergroup node
            // The parser creates this node to preserve the event information
            तार्किक hasUnsupportedEvent = असत्य;
            साठी (Ast_गाठ* itemp = nodep->membersp(); itemp;) {
                Ast_गाठ* स्थिरांक nextp = itemp->nextp();
                जर (AstCovergroup* स्थिरांक cgp = VN_CAST(itemp, Covergroup)) {
                    // Store the event in the global map for V3Active to retrieve later
                    // V3LinkParse only creates this sentinel AstCovergroup node when a clocking
                    // event exists, so cgp->eventp() is always non-null here.
                    UASSERT_OBJ(cgp->eventp(), cgp,
                                "Sentinel AstCovergroup in class must have non-null eventp");
                    जर (hasEnclosingEventRef(cgp)) {
                        UASSERT_OBJ(m_embeddedVarp, cgp,
                                    "Embedded covergroup event has no instance variable");
                        जर (v3Global.opt.timing().isSetTrue()) {
                            embeddedEventForkp = cgp->eventp()->unlinkFrBack();
                        } अन्यथा {
                            embeddedEventTriggers = collectEmbeddedEventTriggers(cgp);
                            जर (embeddedEventTriggers.empty()) {
                                cgp->v3warn(COVERIGN,
                                            "Unsupported: 'covergroup' clocking event on complex "
                                            "member expression; use --timing for full support.");
                                hasUnsupportedEvent = सत्य;
                            }
                        }
                        VL_DO_DANGLING(ढकला_Deletep(cgp->unlinkFrBack()), cgp);
                        itemp = nextp;
                        सुरूठेवा;
                    }
                    // V3Active handles events that do not depend on an enclosing instance.
                    UINFO(4, "Keeping covergroup event node for V3Active: " << nodep->name());
                    itemp = nextp;
                    सुरूठेवा;
                }
                itemp = nextp;
            }

            // Find the sample() method and constructor
            m_sampleFuncp = VN_CAST(m_memberMap.findMember(nodep, "sample"), Func);
            // V3LinkParse always synthesizes a sample() method for every covergroup, and the
            // sampling-code generation below dereferences m_sampleFuncp unconditionally.
            UASSERT_OBJ(m_sampleFuncp, nodep, "Covergroup missing synthesized sample() method");
            m_sampleFuncp->isCovergroupSample(सत्य);
            m_constructorp = VN_CAST(m_memberMap.findMember(nodep, "new"), Func);
            UINFO(9, "Found sample() method: " << (m_sampleFuncp ? "yes" : "no"));
            UINFO(9, "Found constructor: " << (m_constructorp ? "yes" : "no"));

            // If covergroup has unsupported clocking event, skip processing it
            // but still clean up coverpoints so they don't reach downstream passes
            जर (hasUnsupportedEvent) {
                iterateChildren(nodep);
                validateCovergroupExpressions();
                rebindFormalRefs();
                delete_Coverage_वस्तुसंच();
                जर (embeddedEventForkp) {
                    VL_DO_DANGLING(ढकला_Deletep(embeddedEventForkp), embeddedEventForkp);
                }
                परत;
            }

            iterateChildren(nodep);
            validateCovergroupExpressions();
            rebindFormalRefs();
            स्थिरांक std::vector<Ast_गाठ_Assign*> constructps = findCovergroupConstructions();

            // Embedded covergroups (IEEE 1800-2023 19.4): coverpoints, iff expressions, and
            // crosses may reference members of the enclosing class. The covergroup is lowered
            // into a sibling class with no implicit handle to the enclosing instance. Install
            // an explicit back-pointer and route the references through it.
            जर (AstVarRef* स्थिरांक invalidp = installEnclosingBackPointer(constructps)) {
                invalidp->v3error("Non-static member "
                                  << invalidp->varp()->prettyNameQ()
                                  << " of an outer class requires an explicit "
                                     "object handle (IEEE 1800-2023 8.23).");
                delete_Coverage_वस्तुसंच();
                जर (embeddedEventForkp) {
                    VL_DO_DANGLING(ढकला_Deletep(embeddedEventForkp), embeddedEventForkp);
                }
                परत;
            }
            installEmbeddedEventTriggers(embeddedEventTriggers, constructps);
            जर (embeddedEventForkp) installEmbeddedEventFork(embeddedEventForkp, constructps);
            processCovergroup();
            // Remove lowered coverpoints/crosses from the class - they have been
            // fully translated into C++ code and must not reach downstream passes
            delete_Coverage_वस्तुसंच();
        } अन्यथा {
            // Track the lexically enclosing class so a nested covergroup can resolve
            // references to the enclosing object's members (installEnclosingBackPointer).
            VL_RESTORER(m_enclosingClassp);
            m_enclosingClassp = nodep;
            iterateChildren(nodep);
        }
    }

    शून्य visit(AstCoverpoint* nodep) override {
        UINFO(9, "Found coverpoint: " << nodep->name());
        m_coverpoints.ढकला_back(nodep);
        m_coverpointMap.emplace(nodep->name(), nodep);
        iterateChildren(nodep);
    }

    शून्य visit(AstCoverCross* nodep) override {
        UINFO(9, "Found cross: " << nodep->name());
        m_coverCrosses.ढकला_back(nodep);
        iterateChildren(nodep);
    }

    // V3Width leaves only the covergroup-level options lowerCovergroupOptions() takes
    शून्य visit(AstCgOptionAssign* nodep) override { m_cgOptions.ढकला_back(nodep); }

    // A package, interface, or module, so the design unit declaring the classes within
    शून्य visit(Ast_गाठ_Module* nodep) override {
        VL_RESTORER(m_unitp);
        m_unitp = nodep;
        iterateChildren(nodep);
    }

    शून्य visit(Ast_गाठ* nodep) override { iterateChildren(nodep); }

सार्वजनिक:
    // CONSTRUCTORS
    FunctionalCoverageVisitor(AstNetlist* nodep,
                              स्थिरांक std::set<स्थिरांक AstVar*>& mergeableTypeOptions)
        : m_mergeableTypeOptions{mergeableTypeOptions} {
        iterate(nodep);
    }
    ~FunctionalCoverageVisitor() override = पूर्वनिर्धारित;
};

//######################################################################
// Functional coverage class functions

शून्य V3Covergroup::covergroup(AstNetlist* nodep) {
    UINFO(4, __FUNCTION__ << ": ");
    स्थिरांक CovergroupAssignValidVisitor validVisitor{nodep};
    जर (!validVisitor.valid()) V3Error::abortIfErrors();
    {  // Destruct before checking
        FunctionalCoverageVisitor{nodep, validVisitor.mergeableTypeOptions()};
    }
    V3Global::dump_Check_Global_वृक्ष("coveragefunc", 0, dump_वृक्ष_Either_Level() >= 3);
}
