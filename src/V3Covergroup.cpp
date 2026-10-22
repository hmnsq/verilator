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
    std::map<अचर AstVar*, अचर Ast_गाँठो_FTask*>
        m_constructors;  // Implicit instance -> constructor
    std::set<अचर AstVar*> m_mergeableTypeOptions;  // See mergeableTypeOptions()
    अचर Ast_गाँठो_FTask* m_ftaskp = nullptr;
    तार्किक m_collecting = सत्य;
    तार्किक m_valid = सत्य;

    शून्य visit(AstClass* nodep) override {
        VL_RESTORER(m_ftaskp);
        m_ftaskp = nullptr;
        यदि (m_collecting) {
            अचर Ast_गाँठो_FTask* अचर constructorp
                = VN_CAST(m_memberMap.findMember(nodep, "new"), गाँठो_FTask);
            लागि (अचर Ast_गाँठो* itemp = nodep->membersp(); itemp; itemp = itemp->nextp()) {
                अचर AstVar* अचर varp = VN_CAST(itemp, Var);
                // Only the implicit instance is restricted, not explicitly typed aliases.
                यदि (!varp || !varp->isClassMember() || varp->isDeclTyped()) जारी;
                अचर AstClassRefDType* अचर refp
                    = VN_CAST(varp->dtypep()->skipRefp(), ClassRefDType);
                यदि (refp && refp->classp()->covergroupEnclosingClassp() == nodep) {
                    m_constructors.emplace(varp, constructorp);
                }
            }
        }
        iterateChildrenConst(nodep);
    }
    शून्य visit(Ast_गाँठो_FTask* nodep) override {
        VL_RESTORER(m_ftaskp);
        m_ftaskp = nodep;
        iterateChildrenConst(nodep);
    }
    शून्य visit(Ast_गाँठो_Assign* nodep) override {
        यदि (!m_collecting) {
            अचर AstVar* varp = nullptr;
            यदि (अचर Ast_गाँठो_Var_Ref* अचर refp = VN_CAST(nodep->lhsp(), गाँठो_Var_Ref)) {
                varp = refp->varp();
            } अन्यथा यदि (अचर AstMemberSel* अचर selp = VN_CAST(nodep->lhsp(), MemberSel)) {
                varp = selp->varp();
            }
            अचर स्वतः it = m_constructors.find(varp);
            यदि (it != m_constructors.end() && (!m_ftaskp || m_ftaskp != it->second)) {
                m_valid = असत्य;
                nodep->v3error("Embedded covergroup variable "
                               << varp->prettyNameQ()
                               << " may only be assigned in the enclosing class's 'new' method "
                                  "(IEEE 1800-2023 19.4).");
            }
        }
        iterateChildrenConst(nodep);
    }
    शून्य visit(Ast_गाँठो_Var_Ref* nodep) override {
        // Type options may be set at any time (IEEE 1800-2023 19.7.1), so SystemVerilog may set
        // type_option.merge_instances with any use of a type_option but a read of another
        // member, as not every write is marked yet (std::randomize)
        यदि (m_collecting && nodep->varp()->name() == "type_option") {
            अचर AstStructSel* अचर selp = VN_CAST(nodep->backp(), StructSel);
            यदि (!selp || selp->fromp() != nodep || selp->name() == "merge_instances"
                || !nodep->access().isReadOnly()) {
                m_mergeableTypeOptions.घुसाउ(nodep->varp());
            }
        }
        iterateChildrenConst(nodep);
    }
    शून्य visit(Ast_गाँठो* nodep) override { iterateChildrenConst(nodep); }

सार्वजनिक:
    explicit CovergroupAssignValidVisitor(AstNetlist* nodep) {
        // Uses can precede their enclosing class in the tree.
        iterateConst(nodep);
        m_collecting = असत्य;
        यदि (!m_constructors.empty()) iterateConst(nodep);
    }
    तार्किक valid() अचर { फर्काऊ m_valid; }
    // type_option variables through which SystemVerilog may set type_option.merge_instances, so
    // that covergroups are tested with optionVar(true)
    अचर std::set<अचर AstVar*>& mergeableTypeOptions() अचर { फर्काऊ m_mergeableTypeOptions; }
};

//######################################################################
// Covergroup expression validation visitor

वर्ग Covergroup_अभिव्यक्ति_Valid_Visitor final : सार्वजनिक VNVisitor {
    अचर std::set<अचर AstVar*>& m_sampleMembers;
    अचर std::set<अचर AstVar*>& m_constructorRefMembers;
    तार्किक m_inCoverageExpression = असत्य;
    तार्किक m_sampleFormalAllowed = असत्य;

    शून्य scanSampleExpression(Ast_गाँठो* nodep) {
        यदि (!nodep) फर्काऊ;
        VL_RESTORER(m_sampleFormalAllowed);
        m_sampleFormalAllowed = सत्य;
        iterateAndNextNull(nodep);
    }

    शून्य scanCoverageExpression(Ast_गाँठो* nodep) {
        यदि (!nodep) फर्काऊ;
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
        यदि (!m_sampleFormalAllowed && m_sampleMembers.गन्ती(nodep->varp())) {
            nodep->v3error("Covergroup sample formal argument "
                           << nodep->varp()->prettyNameQ()
                           << " may only be used in a coverpoint or conditional guard "
                              "expression (IEEE 1800-2023 19.8.1).");
        }
        यदि (m_inCoverageExpression && m_constructorRefMembers.गन्ती(nodep->varp())) {
            nodep->v3error("Ref covergroup constructor formal argument "
                           << nodep->varp()->prettyNameQ()
                           << " may not be used in a covergroup expression "
                              "(IEEE 1800-2023 19.5).");
        }
    }
    शून्य visit(Ast_गाँठो_FTask_Ref* nodep) override {
        यदि (m_inCoverageExpression && nodep->taskp()) {
            तार्किक invalidDirection = असत्य;
            लागि (Ast_गाँठो* stmtp = nodep->taskp()->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
                अचर AstVar* अचर varp = VN_CAST(stmtp, Var);
                यदि (varp && varp->isIO() && varp->isWritable()) {
                    invalidDirection = सत्य;
                    रोक;
                }
            }
            यदि (invalidDirection) {
                nodep->v3error("Function " << nodep->taskp()->prettyNameQ()
                                           << " called in a covergroup expression has an "
                                              "output, inout, or non-const ref argument "
                                              "(IEEE 1800-2023 19.5).");
            }
        }
        iterateChildren(nodep);
    }
    शून्य visit(Ast_गाँठो* nodep) override { iterateChildren(nodep); }

सार्वजनिक:
    Covergroup_अभिव्यक्ति_Valid_Visitor(अचर std::set<अचर AstVar*>& sampleMembers,
                               अचर std::set<अचर AstVar*>& constructorRefMembers)
        : m_sampleMembers{sampleMembers}
        , m_constructorRefMembers{constructorRefMembers} {}
    शून्य scan(Ast_गाँठो* nodep) { iterate(nodep); }
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
    uint32_t m_गन्ती;  // Number of bins
    V3_सङ्ख्या m_lo;  // Lowest value of the first bin
    V3_सङ्ख्या m_stride;  // Number of values of each bin but the last
    V3_सङ्ख्या m_hi;  // Highest value of the last bin
    तार्किक m_empty = असत्य;  // A single bin without a value of the coverpoint type
    uint32_t m_declared = 0;  // Runtime index of the first bin, once generated

    // CONSTRUCTORS
    BinRun(Ast_गाँठो* nodep, पूर्णाङ्क width, uint32_t गन्ती)
        : m_गन्ती{गन्ती}
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
    अचर VNUser1InUse m_inuser1;
    अचर VNUser2InUse m_inuser2;

    // STATE
    std::set<AstCoverpoint*>
        m_runtimePoints;  // Points needing value metadata and live-bin mapping
    std::set<AstCoverCross*> m_runtimeCrosses;  // Crosses over finalized live-bin dimensions
    std::map<AstVar*, AstVar*> m_excludedVars;  // Sample-time state-exclusion flags
    AstClass* m_covergroupp = nullptr;  // Current covergroup being processed
    Ast_गाँठो_Module* m_unitp = nullptr;  // Design unit declaring the current class, outside classes
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
    अचर std::set<अचर AstVar*>& m_mergeableTypeOptions;
    पूर्णाङ्क m_cgAtLeast = 1;  // Covergroup option.at_least

    संरचना EmbeddedEventTrigger final {
        File_पङ्क्ति* eventFl;  // Clocking-event source location
        AstVar* baseVarp;  // Base enclosing-class member in the event expression
        AstVar* memberVarp;  // Selected member in a 'base.member' expression, or nullptr
        VEdgeType edgeType;  // Clocking-event edge qualifier
        AstVar* prevVarp;  // Member containing the previous event value, or nullptr
        EmbeddedEventTrigger(File_पङ्क्ति* eventFl, AstVar* baseVarp, AstVar* memberVarp,
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
        uint32_t गन्ती = 0;  // Bins across all runs
        तार्किक unsupported = असत्य;  // Too many bins, or invalid: the declaration is ignored
        // The value of each bin, which names it, of a wildcard array; else the bins are indexed
        std::vector<std::string> मानहरू;
        // Too many values of an ignore or illegal wildcard array, which is then one bin
        तार्किक single = असत्य;
    };
    संरचना Cross_Bin_मानहरू final {
        AstCoverBin* binp;  // Declaration owning this Normal bin
        Ast_गाँठो_अभिव्यक्ति* valuep;  // Individual array-bin value, or nullptr for a scalar bin
        अचर BinRun* runp = nullptr;  // Run computing the bin's values, if any
        uint32_t element = 0;  // Index of the bin within runp
    };
    संरचना BinSpan final {
        uint32_t first = 0;  // First Normal index of the bin declaration
        uint32_t गन्ती = 0;  // Number of Normal bins of the declaration
        uint32_t declared = 0;  // First runtime bin index, across all bin kinds
        int32_t sized = -1;  // Index of the sized array, placed at construction; or -1
    };
    संरचना CoverpointBins final {
        uint32_t total = 0;  // Number of Normal bins
        Ast_गाँठो_अभिव्यक्ति* exprp = nullptr;  // Sampled expression, for the value domain
        तार्किक crossed = असत्य;  // Feeds a cross, which needs 'values'
        std::vector<Cross_Bin_मानहरू> मानहरू;  // Values in runtime Normal-bin index order
        std::unordered_map<std::string, BinSpan> spans;  // Declared bin name -> index span
        BinSpan implicitAuto{0, 0, 0};  // Implicit automatic bins, each named 'auto_<i>'
        std::deque<BinRun> runs;  // Runs 'values' refers to
    };
    std::map<AstVar*, CoverpointBins> m_cpBins;  // Runtime coverpoint -> binsof index ranges
    // Names of the bins declarations each coverpoint ignored, which binsof selects as no bins
    std::map<अचर AstCoverpoint*, std::vector<std::string>> m_droppedBins;
    // Prefixes of the constructor temporaries of constructed bins, as coverpoint and bin names
    // alone may repeat: coverpoint 'a_' bins 'b', and coverpoint 'a' bins '_b'
    V3UniqueNames m_sizedNames{"__Vsized"};
    std::vector<Ast_गाँठो_अभिव्यक्ति*> m_detached_मानहरू;  // Array-bin values m_cpBins refers to
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
        AstVar* अचर varp = VN_AS(
            m_memberMap.findMember(m_covergroupp, typeOption ? "type_option" : "option"), Var);
        UASSERT_OBJ(varp, m_covergroupp, "Covergroup missing option member");
        फर्काऊ varp;
    }

    // 'option.<member>' or 'type_option.<member>', per optionVarp
    AstStructSel* newOptionSel(File_पङ्क्ति* fl, AstVar* optionVarp, अचर std::string& member,
                               VAccess access) {
        अचर AstMemberDType* अचर memberp
            = VN_AS(m_memberMap.findMember(optionVarp->dtypep()->skipRefp(), member), MemberDType);
        UASSERT_OBJ(memberp, optionVarp, "Coverage option structure missing '" << member << "'");
        Ast_गाँठो_अभिव्यक्ति* अचर fromp = optionVarp->lifetime().isStatic()
                                       ? नूतन AstVarRef{fl, optionVarp, access}
                                       : memberRef(fl, optionVarp, access);
        AstStructSel* अचर selp = नूतन AstStructSel{fl, fromp, member};
        selp->dtypep(memberp->subDTypep()->skipRefToEnump());
        selp->didWidth(सत्य);
        फर्काऊ selp;
    }

    // A constant option.at_least; a negative count of hits has no meaning, so takes the default
    स्थिर पूर्णाङ्क constAtLeast(अचर AstConst* constp) {
        अचर पूर्णाङ्क मान = constp->toSInt();
        यदि (मान >= 0) फर्काऊ मान;
        constp->v3warn(COVERIGN, "Ignoring negative 'option.at_least'; using 1");
        फर्काऊ 1;
    }

    // An option's constant value, or nullptr after a warning, as non-constant is unsupported
    स्थिर अचर AstConst* constOption(अचर Ast_गाँठो* valuep, VCoverOptionType optType) {
        यदि (अचर AstConst* अचर constp = VN_CAST(valuep, Const)) फर्काऊ constp;
        valuep->v3warn(COVERIGN, "Ignoring unsupported: non-constant 'option."
                                     << optType.ascii() << "'; using default value");
        फर्काऊ nullptr;
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
        लागि (AstCgOptionAssign* अचर optp : m_cgOptions) {
            यदि (optp->optType() == VCoverOptionType::AT_LEAST) {
                यदि (अचर AstConst* अचर constp = constOption(optp->valuep(), optp->optType())) {
                    m_cgAtLeast = constAtLeast(constp);
                }
                VL_DO_DANGLING(थप_Deletep(optp->unlinkFrBack()), optp);
                जारी;
            }
            File_पङ्क्ति* अचर fl = optp->fileline();
            // V3Width left the type options constant, and type_option.weight non-negative
            अचर AstConst* अचर constp = VN_CAST(optp->valuep(), Const);
            std::string member = "weight";
            यदि (optp->optType() == VCoverOptionType::MERGE_INSTANCES) {
                member = "merge_instances";
                m_cgMergeInstances = !constp->अङ्क().isEqZero();
                mergeSet = सत्य;
            } अन्यथा यदि (optp->optType() == VCoverOptionType::GET_INST_COVERAGE) {
                member = "get_inst_coverage";
            } अन्यथा {
                UASSERT_OBJ(optp->optType() == VCoverOptionType::WEIGHT, optp,
                            "Unexpected covergroup option reaching V3Covergroup");
                यदि (optp->typeOption()) m_cgTypeWeight = constp->toUInt();
            }
            AstAssign* अचर assignp = नूतन AstAssign{
                fl, newOptionSel(fl, optionVar(optp->typeOption()), member, VAccess::WRITE),
                optp->valuep()->unlinkFrBack()};
            यदि (optp->typeOption()) {
                m_covergroupp->addMembersp(नूतन AstInitialStatic{fl, assignp});
                VL_DO_DANGLING(थप_Deletep(optp->unlinkFrBack()), optp);
            } अन्यथा {
                optp->replaceWith(assignp);
                VL_DO_DANGLING(थप_Deletep(optp), optp);
            }
        }
        m_cgOptions.clear();
        यदि (!mergeSet && v3Global.opt.coverageMergeInstances()) {
            File_पङ्क्ति* अचर fl = m_covergroupp->fileline();
            m_covergroupp->addMembersp(नूतन AstInitialStatic{
                fl, नूतन AstAssign{
                        fl, newOptionSel(fl, optionVar(सत्य), "merge_instances", VAccess::WRITE),
                        नूतन AstConst{fl, AstConst::BitTrue{}}}});
            m_cgMergeInstances = सत्य;
        }
        m_cgMayMerge = m_cgMergeInstances || m_mergeableTypeOptions.गन्ती(optionVar(सत्य));
    }

    // The weight of an item in the coverage database, which merges the instances: its
    // type_option.weight if the covergroup merges them too (IEEE 1800-2023 19.11.3); else its
    // option.weight if a constant, and so of every instance; else its type_option.weight
    uint32_t itemDatabaseWeight(Ast_गाँठो* optionsp) अचर {
        अचर Ast_गाँठो_अभिव्यक्ति* weightp = nullptr;  // The option.weight in effect
        uint32_t typeWeight = 1;
        लागि (Ast_गाँठो* nodep = optionsp; nodep; nodep = nodep->nextp()) {
            अचर AstCoverOption* अचर optp = VN_AS(nodep, CoverOption);
            यदि (!(optp->optType() == VCoverOptionType::WEIGHT)) जारी;
            // V3Width left type_option.weight a non-negative constant
            यदि (optp->typeOption()) {
                typeWeight = VN_AS(optp->valuep(), Const)->toUInt();
            } अन्यथा {
                weightp = optp->valuep();
            }
        }
        यदि (m_cgMergeInstances) फर्काऊ typeWeight;
        यदि (!weightp) फर्काऊ 1;
        यदि (अचर AstConst* अचर constp = VN_CAST(weightp, Const)) फर्काऊ constp->toUInt();
        फर्काऊ typeWeight;
    }

    // Configure an item's option.weight, its weight in instance coverage (IEEE 1800-2023
    // 19.11), and, if the covergroup may merge its instances, its type_option.weight, its
    // weight in type coverage then (19.11.3).
    शून्य generateItemWeight(File_पङ्क्ति* fl, AstVar* itemVarp, Ast_गाँठो* optionsp) {
        लागि (Ast_गाँठो* nodep = optionsp; nodep; nodep = nodep->nextp()) {
            अचर AstCoverOption* अचर optp = VN_AS(nodep, CoverOption);
            यदि (!(optp->optType() == VCoverOptionType::WEIGHT)) जारी;
            यदि (!optp->typeOption()) {
                m_constructorp->addStmtsp(
                    itemCall(fl, itemVarp, VCMethod::COVERGROUP_WEIGHT,
                             {optp->valuep()->clone_रूख(असत्य), file_पङ्क्ति_Debug(optp->fileline())})
                        ->makeStmt());
            } अन्यथा यदि (m_cgMayMerge) {
                // V3Width left type_option.weight a non-negative constant
                m_constructorp->addStmtsp(itemCall(fl, itemVarp, VCMethod::COVERGROUP_TYPE_WEIGHT,
                                                   {optp->valuep()->clone_रूख(असत्य)})
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
        लागि (AstCoverCross* crossp : m_coverCrosses) {
            लागि (Ast_गाँठो* itemp = crossp->itemsp(); itemp; itemp = itemp->nextp()) {
                अचर AstCoverpointRef* अचर refp = VN_AS(itemp, CoverpointRef);
                यदि (refp->exprp()) जारी;  // hierarchical ref: dropped in generateCrossCode
                यदि (m_coverpointMap.find(refp->name()) == m_coverpointMap.end()) {
                    m_droppedCrosses.घुसाउ(crossp);  // bare variable: drop this cross only
                } अन्यथा {
                    m_crossedCpNames.घुसाउ(refp->name());
                }
            }
        }

        std::vector<Ast_गाँठो*> pending;
        std::map<AstCoverpoint*, std::vector<AstCoverCross*>> consumers;
        std::map<AstCoverCross*, std::vector<AstCoverpoint*>> inputs;
        लागि (AstCoverpoint* अचर cpp : m_coverpoints) {
            checkBinNames(cpp);
            checkConstructedBins(cpp);
            यदि (!cpp->exprp()->dtypep()->skipRefp()->isIntegralOrPacked()) जारी;
            // Bins without values leave the report (IEEE 1800-2023 19.11.1), exclusions or not.
            // Constructed bins get their values when the covergroup is constructed.
            यदि (!coverpointHasStateExclusions(cpp) && !coverpointHasEmptyBins(cpp)
                && !coverpointHasConstructedBins(cpp)) {
                जारी;
            }
            m_runtimePoints.घुसाउ(cpp);
            pending.थप_back(cpp);
        }
        लागि (AstCoverCross* अचर crossp : m_coverCrosses) {
            यदि (m_droppedCrosses.गन्ती(crossp)) जारी;
            लागि (Ast_गाँठो* itemp = crossp->itemsp(); itemp; itemp = itemp->nextp()) {
                अचर AstCoverpointRef* अचर refp = VN_AS(itemp, CoverpointRef);
                यदि (refp->exprp()) जारी;
                अचर स्वतः point = m_coverpointMap.find(refp->name());
                यदि (point != m_coverpointMap.end()) {
                    consumers[point->second].थप_back(crossp);
                    inputs[crossp].थप_back(point->second);
                }
            }
            // The values of a filter's bin tuples are known when the covergroup is constructed
            यदि (crossp->exists([](अचर AstCoverCrossWith*) { फर्काऊ सत्य; })) {
                m_runtimeCrosses.emplace(crossp);
                pending.थप_back(crossp);
            }
        }
        लागि (माप_प्रकार next = 0; next < pending.size(); ++next) {
            यदि (AstCoverpoint* अचर pointp = VN_CAST(pending[next], Coverpoint)) {
                लागि (AstCoverCross* अचर crossp : consumers[pointp]) {
                    यदि (m_runtimeCrosses.emplace(crossp).second) pending.थप_back(crossp);
                }
            } अन्यथा {
                लागि (AstCoverpoint* अचर pointp : inputs[VN_AS(pending[next], CoverCross)]) {
                    यदि (pointp->exprp()->dtypep()->skipRefp()->isIntegralOrPacked()
                        && m_runtimePoints.emplace(pointp).second) {
                        pending.थप_back(pointp);
                    }
                }
            }
        }

        // The instance node owns this instance's coverpoint/cross runtimes, so it must exist
        // before any of them is created.  Emitted first, ahead of both generate loops.
        generateInstanceAttach();

        // For each coverpoint, generate sampling code
        लागि (AstCoverpoint* cpp : m_coverpoints) generateCoverpointCode(cpp);

        // For each cross, generate sampling code
        लागि (AstCoverCross* crossp : m_coverCrosses) generateCrossCode(crossp);

        // Every cross has been built, so runtime points only need their exclusions from here.
        लागि (AstCoverpoint* अचर cpp : m_coverpoints) {
            यदि (!m_runtimePoints.गन्ती(cpp)) जारी;
            m_constructorp->addStmtsp(itemCall(cpp->fileline(), m_cpVarMap.at(cpp->name()),
                                               VCMethod::COVERGROUP_मान_RELEASE)
                                          ->makeStmt());
        }
        लागि (Ast_गाँठो_अभिव्यक्ति* valuep : m_detached_मानहरू) VL_DO_DANGLING(थप_Deletep(valuep), valuep);
        m_detached_मानहरू.clear();

        // Generate coverage computation code (even for empty covergroups).  Bin registration
        // with the coverage database is handled per coverpoint/cross by their runtime
        // registerBins() calls (emitted in generateCoverpoint/generateCross).
        generateCoverageComputationCode();
    }

    स्थिर constexpr माप_प्रकार मान_LIST_ENTRIES = 256;  // Metadata entries per constructor call

    // The value of a constant count, of bins or value tuples: -1 if it is negative, and
    // saturated above the largest limit
    स्थिर int64_t const_गन्ती(अचर AstConst* constp) {
        अचर V3_सङ्ख्या& अङ्क = constp->अङ्क();
        यदि (constp->isSigned() && अङ्क.isNegative()) फर्काऊ -1;
        फर्काऊ अङ्क.mostSetBitP1() > 32 ? INT64_MAX : static_cast<int64_t>(अङ्क.toUQuad());
    }

    // Whether a count, 'whatp', is integral and, if constant, positive, as the standard's clause
    // 'clausep' requires; else errors
    स्थिर तार्किक check_गन्ती(अचर Ast_गाँठो_अभिव्यक्ति* countp, अचर वर्ण* whatp, अचर वर्ण* clausep) {
        अचर AstConst* अचर constp = VN_CAST(countp, Const);
        यदि (!countp->dtypep()->skipRefp()->isIntegralOrPacked()) {
            countp->v3error(whatp << " must be integral (IEEE 1800-2023 " << clausep << ")");
            फर्काऊ असत्य;
        } अन्यथा यदि (constp && (constp->अङ्क().isFourState() || const_गन्ती(constp) < 1)) {
            countp->v3error(whatp << " must be >= 1, got "
                                  << (constp->अङ्क().isFourState() ? constp->अङ्क().ascii(असत्य)
                                                                  : constp->अङ्क().toDecimalS())
                                  << " (IEEE 1800-2023 " << clausep << ")");
            फर्काऊ असत्य;
        }
        फर्काऊ सत्य;
    }

    // The number of bins requested by a valid 'bins auto[N]', or 0
    स्थिर uint32_t autoBinsRequested(अचर AstCoverBin* binp) {
        अचर AstConst* अचर constp = VN_CAST(binp->arraySizep(), Const);
        यदि (!constp) फर्काऊ 0;
        अचर int64_t गन्ती = const_गन्ती(constp);
        फर्काऊ गन्ती < 1 || गन्ती > v3Global.opt.coverageMaxBins() ? 0
                                                                   : static_cast<uint32_t>(गन्ती);
    }

    // True for a 'bins auto[N]' declaration, or the implicit automatic bins of a coverpoint
    स्थिर तार्किक isAutoBins(अचर AstCoverBin* binp) {
        फर्काऊ binp->binsType() == VCoverBinsType::BINS_AUTO
               || binp->binsType() == VCoverBinsType::BINS_AUTO_IMPLICIT;
    }

    // True for a sized array of bins, 'bins b[N] = {...}', whose values an integral coverpoint
    // distributes over N bins when the covergroup is constructed (IEEE 1800-2023 19.5.1)
    स्थिर तार्किक isSizedArray(अचर AstCoverBin* binp) {
        फर्काऊ binp->arraySizep() && !isAutoBins(binp);
    }

    // The 'with' filter of a bin, if any (IEEE 1800-2023 19.5.1.1)
    स्थिर AstCoverWith* binWith(अचर AstCoverBin* binp) {
        फर्काऊ VN_CAST(binp->rangesp(), CoverWith);
    }

    // True for bins whose values the covergroup constructor computes: a sized array, or bins of
    // the values a 'with' filter keeps
    स्थिर तार्किक isConstructedBins(अचर AstCoverBin* binp) {
        फर्काऊ isSizedArray(binp) || binWith(binp);
    }

    // The range list of a bin's values, or of the candidates of its 'with' filter; null for all
    // of the coverpoint's values, which a filter of the coverpoint's name has
    स्थिर Ast_गाँठो* binRangesp(अचर AstCoverBin* binp) {
        अचर AstCoverWith* अचर withp = binWith(binp);
        यदि (!withp) फर्काऊ binp->rangesp();
        फर्काऊ VN_IS(withp->subp(), CoverpointRef) ? nullptr : withp->subp();
    }

    // Report and delete a bins declaration of a name that another of its coverpoint has
    शून्य checkBinNames(AstCoverpoint* coverpointp) {
        std::set<std::string> names;
        लागि (Ast_गाँठो* nodep = coverpointp->binsp(); nodep;) {
            AstCoverBin* अचर binp = VN_AS(nodep, CoverBin);
            nodep = nodep->nextp();
            यदि (names.emplace(binp->name()).second) जारी;
            binp->v3error("Duplicate bin " << binp->prettyNameQ() << " in coverpoint "
                                           << coverpointp->prettyNameQ()
                                           << " (IEEE 1800-2023 3.13)");
            VL_DO_DANGLING(थप_Deletep(binp->unlinkFrBack()), binp);
        }
    }

    // Delete an ignored bins declaration, which binsof then selects as no bins
    शून्य dropBins(अचर AstCoverpoint* coverpointp, AstCoverBin* binp) {
        m_droppedBins[coverpointp].थप_back(binp->name());
        VL_DO_DANGLING(थप_Deletep(binp->unlinkFrBack()), binp);
    }

    // Check the size of a sized array of bins, which drops an invalid array.  A real coverpoint's
    // are unsupported, and treated as arrays of a bin per value.  False unless it stays sized.
    तार्किक checkBinsArraySize(अचर AstCoverpoint* coverpointp, AstCoverBin* binp, तार्किक integral) {
        Ast_गाँठो_अभिव्यक्ति* अचर sizep = binp->arraySizep();
        यदि (VN_IS(sizep, Unbounded)) {  // A parameter of '$'; see bins_orBraE
            binp->v3error("Bins array size must be integral, not '$' (IEEE 1800-2023 19.5.1)");
        } अन्यथा यदि (check_गन्ती(sizep, "Bins array size", "19.5.1")) {
            यदि (integral) फर्काऊ सत्य;
            binp->v3warn(COVERIGN, "Unsupported: 'bins' explicit array size of a real "
                                   "coverpoint (treated as '[]')");
            VL_DO_DANGLING(थप_Deletep(sizep->unlinkFrBack()), sizep);
            फर्काऊ असत्य;
        }
        dropBins(coverpointp, binp);
        फर्काऊ असत्य;
    }

    // Check the bins of a coverpoint whose values the constructor computes, dropping invalid
    // ones.  A wildcard array of too many ranges of values is ignored, or if ignore or illegal,
    // treated as one bin; filtering too many ranges of values ignores the bins.
    शून्य checkConstructedBins(AstCoverpoint* coverpointp) {
        अचर तार्किक integral = coverpointp->exprp()->dtypep()->skipRefp()->isIntegralOrPacked();
        लागि (Ast_गाँठो* nodep = coverpointp->binsp(); nodep;) {
            AstCoverBin* अचर binp = VN_AS(nodep, CoverBin);
            nodep = nodep->nextp();
            यदि (!isConstructedBins(binp)) जारी;
            यदि (isSizedArray(binp) && !checkBinsArraySize(coverpointp, binp, integral)) जारी;
            यदि (!binp->isWildcard()
                || sizedWildcardRuns(binp, coverpointp->exprp())
                       <= v3Global.opt.coverageMaxBins()) {
                यदि (binWith(binp) && withCandidatesOver(binp, coverpointp->exprp())) {
                    binp->v3warn(COVERIGN, "Unsupported: 'with' filter of more than 2**32 "
                                           "candidate values; bin "
                                               << binp->prettyNameQ() << " ignored");
                    यदि (binp->binsType().binIsNormal()) coverpointp->user2(सत्य);
                    dropBins(coverpointp, binp);
                }
                जारी;
            }
            // An ignore or illegal array still excludes or checks its values, as one bin
            अचर तार्किक single = !binp->binsType().binIsNormal() && !binWith(binp);
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
            यदि (single) {
                Ast_गाँठो_अभिव्यक्ति* अचर sizep = binp->arraySizep();
                VL_DO_DANGLING(थप_Deletep(sizep->unlinkFrBack()), sizep);
                binp->isArray(असत्य);
                जारी;
            }
            यदि (binp->binsType().binIsNormal()) coverpointp->user2(सत्य);
            dropBins(coverpointp, binp);
        }
    }

    // True if a 'with' filter would be evaluated for more than 2**32 candidate values, known
    // now for the coverpoint's name, or for a range list of constants: each is evaluated once
    // but for an array 'b[N]', which keeps their order and duplicates (see withBegin())
    स्थिर तार्किक withCandidatesOver(AstCoverBin* binp, Ast_गाँठो_अभिव्यक्ति* exprp) {
        अचर पूर्णाङ्क width = runWidth(exprp);
        std::vector<std::pair<V3_सङ्ख्या, V3_सङ्ख्या>> runs;
        यदि (!binRangesp(binp)) runs = coverpoint_मानहरू(binp, exprp);
        अचर स्वतः constant
            = [](अचर Ast_गाँठो* nodep) { फर्काऊ VN_IS(nodep, Const) || VN_IS(nodep, Unbounded); };
        लागि (Ast_गाँठो* rangep = binRangesp(binp); rangep; rangep = rangep->nextp()) {
            अचर AstInsideRange* अचर irp = VN_CAST(rangep, InsideRange);
            // Else known when constructed
            यदि (irp ? !constant(irp->lhsp()) || !constant(irp->rhsp()) : !VN_IS(rangep, Const)) {
                फर्काऊ असत्य;
            }
            Cross_मान_Range range{rangep, resolveWidth(rangep, exprp)};
            यदि (!resolve_मान(rangep, exprp, सत्य, binp->isWildcard(), range)
                || crossRangeEmpty(range)) {
                जारी;
            }
            std::vector<std::pair<V3_सङ्ख्या, V3_सङ्ख्या>> found{{range.lo, range.hi}};
            यदि (range.wildcard) {  // checkConstructedBins bounded the runs
                found.clear();
                crossRangeRuns(range, v3Global.opt.coverageMaxBins(), found);
            }
            लागि (अचर std::pair<V3_सङ्ख्या, V3_सङ्ख्या>& run : found) {
                runs.emplace_back(V3_सङ्ख्या{rangep, width, run.first},
                                  V3_सङ्ख्या{rangep, width, run.second});
            }
        }
        यदि (!isSizedArray(binp)) {  // The union of the values
            std::sort(runs.begin(), runs.end(), [](अचर स्वतः& lhs, अचर स्वतः& rhs) {
                फर्काऊ cross_मान_Less(lhs.first, rhs.first);
            });
            std::vector<std::pair<V3_सङ्ख्या, V3_सङ्ख्या>> merged;
            लागि (अचर std::pair<V3_सङ्ख्या, V3_सङ्ख्या>& run : runs) {
                यदि (merged.empty() || cross_मान_Less(merged.back().second, run.first)) {
                    merged.थप_back(run);
                } अन्यथा यदि (cross_मान_Less(merged.back().second, run.second)) {
                    merged.back().second = run.second;
                }
            }
            runs = std::move(merged);
        }
        uint64_t गन्ती = 0;
        लागि (अचर std::pair<V3_सङ्ख्या, V3_सङ्ख्या>& run : runs) {
            V3_सङ्ख्या span{exprp, width};
            span.opSub(run.second, run.first);
            यदि (span.mostSetBitP1() > 32) फर्काऊ सत्य;  // More than 2**32 values
            गन्ती += span.toUQuad() + 1;
            यदि (गन्ती > (uint64_t{1} << 32)) फर्काऊ सत्य;
        }
        फर्काऊ असत्य;
    }

    // The ranges of values the wildcard patterns of a sized wildcard array, or of a 'with'
    // filter's candidates, give, counted up to more than --coverage-max-bins
    स्थिर माप_प्रकार sizedWildcardRuns(अचर AstCoverBin* binp, Ast_गाँठो_अभिव्यक्ति* exprp) {
        std::vector<std::pair<V3_सङ्ख्या, V3_सङ्ख्या>> runs;
        लागि (Ast_गाँठो* rangep = binRangesp(binp); rangep; rangep = rangep->nextp()) {
            यदि (!VN_IS(rangep, Const)) जारी;  // A range, or a value known at construction
            Cross_मान_Range range{rangep, resolveWidth(rangep, exprp)};
            यदि (resolve_मान(rangep, exprp, सत्य, सत्य, range) && !crossRangeEmpty(range)) {
                crossRangeRuns(range, v3Global.opt.coverageMaxBins(), runs);
            }
        }
        फर्काऊ runs.size();
    }

    // Check the automatic bins declarations of a coverpoint.  Each stays one declaration, which
    // generates as a partition of the coverpoint domain (see autoBinRuns).
    शून्य checkAutomaticBins(AstCoverpoint* coverpointp, अचर Ast_गाँठो_अभिव्यक्ति* exprp) {
        लागि (Ast_गाँठो* binp = coverpointp->binsp(); binp; binp = binp->nextp()) {
            AstCoverBin* अचर cbinp = VN_AS(binp, CoverBin);
            यदि (cbinp->binsType() != VCoverBinsType::BINS_AUTO) जारी;
            अचर AstConst* अचर constp = VN_CAST(cbinp->arraySizep(), Const);
            यदि (!constp) {
                cbinp->v3error("Automatic bins array size must be a constant");
            } अन्यथा यदि (const_गन्ती(constp) < 1) {
                cbinp->v3error("Automatic bins array size must be >= 1, got "
                               << constp->अङ्क().toDecimalS());
            } अन्यथा यदि (const_गन्ती(constp) > v3Global.opt.coverageMaxBins()) {
                cbinp->v3error("Automatic bins array size of "
                               << constp->अङ्क().toDecimalU() << " exceeds limit of "
                               << v3Global.opt.coverageMaxBins() << '\n'
                               << cbinp->warnMore() << "... Suggest a larger --coverage-max-bins");
            } अन्यथा यदि (!exprp->dtypep()->skipRefp()->isIntegralOrPacked()) {
                cbinp->v3error("Automatic bins are not allowed on a coverpoint of a non-integral "
                               "expression (IEEE 1800-2023 19.5.3).");
            }
        }
    }

    // Extract all option values of a coverpoint or cross in a single pass.
    // atLeastOut: option.at_least (default the covergroup's)
    // autoBinMaxOut: option.auto_bin_max (coverpoint overrides covergroup, default 64)
    शून्य extractItemOptions(Ast_गाँठो* optionsp, पूर्णाङ्क& atLeastOut, पूर्णाङ्क& autoBinMaxOut) {
        atLeastOut = m_cgAtLeast;
        autoBinMaxOut = -1;  // -1 = not set at coverpoint level
        लागि (Ast_गाँठो* optionp = optionsp; optionp; optionp = optionp->nextp()) {
            AstCoverOption* अचर optp = VN_AS(optionp, CoverOption);
            // Weights may be non-constant; generateItemWeight() handles them.  A cross keeps
            // its unsupported options, which V3LinkParse warned of.
            यदि (!(optp->optType() == VCoverOptionType::AT_LEAST
                  || optp->optType() == VCoverOptionType::AUTO_BIN_MAX)) {
                जारी;
            }
            अचर AstConst* अचर constp = constOption(optp->valuep(), optp->optType());
            यदि (!constp) जारी;
            यदि (optp->optType() == VCoverOptionType::AT_LEAST) {
                atLeastOut = constAtLeast(constp);
            } अन्यथा {
                autoBinMaxOut = constp->toSInt();
            }
        }
        // Fall back to covergroup-level auto_bin_max if not set at coverpoint level
        यदि (autoBinMaxOut < 0) {
            यदि (m_covergroupp->cgAutoBinMax() >= 0) {
                autoBinMaxOut = m_covergroupp->cgAutoBinMax();
            } अन्यथा {
                autoBinMaxOut = 64;  // Default per IEEE 1800-2023 Table 19-1
            }
        }
    }

    // IEEE 1800-2023 19.5.2: an enum coverpoint has one automatic bin per enumeration value
    शून्य createEnumAutoBins(AstCoverpoint* coverpointp, Ast_गाँठो_अभिव्यक्ति* exprp,
                            अचर AstEnumDType* enump) {
        File_पङ्क्ति* अचर fl = coverpointp->fileline();
        लागि (अचर AstEnumItem* itemp = enump->itemsp(); itemp;
             itemp = VN_AS(itemp->nextp(), EnumItem)) {
            AstConst* अचर lop = new_मान_Const(fl, VN_AS(itemp->valuep(), Const)->अङ्क(), exprp);
            AstInsideRange* अचर rangep = नूतन AstInsideRange{fl, lop, lop->clone_रूख(असत्य)};
            rangep->dtypeFrom(exprp);
            coverpointp->addBinsp(
                नूतन AstCoverBin{fl, "auto[" + itemp->name() + "]", rangep, असत्य, असत्य});
        }
    }

    // IEEE 1800-2023 19.5.3/19.11.1: partition first, then apply exclusions.  The partition is one
    // automatic bins declaration, generated as a run like 'bins auto[N]' but numbering its bins.
    शून्य createImplicitAutoBins(AstCoverpoint* coverpointp, Ast_गाँठो_अभिव्यक्ति* exprp, पूर्णाङ्क autoBinMax) {
        यदि (coverpointp->user2()) फर्काऊ;  // Declared bins, ignored, leave no bins
        लागि (Ast_गाँठो* nodep = coverpointp->binsp(); nodep; nodep = nodep->nextp()) {
            अचर VCoverBinsType kind = VN_AS(nodep, CoverBin)->binsType();
            यदि (kind != VCoverBinsType::BINS_IGNORE && kind != VCoverBinsType::BINS_ILLEGAL)
                फर्काऊ;
        }
        यदि (अचर AstEnumDType* अचर enump
            = VN_CAST(exprp->dtypep()->skipRefToEnump(), EnumDType)) {
            createEnumAutoBins(coverpointp, exprp, enump);
            फर्काऊ;
        }
        अचर पूर्णाङ्क width = exprp->width();
        uint32_t गन्ती = width < 31 ? std::min<uint32_t>(uint32_t{1} << width, autoBinMax)
                                    : static_cast<uint32_t>(autoBinMax);
        यदि (!गन्ती) फर्काऊ;
        यदि (!exprp->dtypep()->skipRefp()->isIntegralOrPacked()) {
            coverpointp->v3error("Coverpoint of a non-integral expression requires explicit bins "
                                 "(IEEE 1800-2023 19.5.3).");
            फर्काऊ;
        }
        यदि (गन्ती > v3Global.opt.coverageMaxBins()) {
            coverpointp->v3warn(COVERIGN, "Unsupported: more than "
                                              << v3Global.opt.coverageMaxBins()
                                              << " automatic bins from 'option.auto_bin_max'; "
                                                 "using "
                                              << v3Global.opt.coverageMaxBins() << ".\n"
                                              << coverpointp->warnMore()
                                              << "... Suggest a larger --coverage-max-bins");
            गन्ती = v3Global.opt.coverageMaxBins();
        }
        File_पङ्क्ति* अचर fl = coverpointp->fileline();
        coverpointp->addBinsp(नूतन AstCoverBin{fl, "auto", नूतन AstConst{fl, गन्ती},
                                              VCoverBinsType::BINS_AUTO_IMPLICIT});
    }

    // Sanitize generated names to be valid C++ identifiers
    स्थिर string sanitizeGeneratedName(string name) {
        std::replace(name.begin(), name.end(), '[', '_');
        std::replace(name.begin(), name.end(), ']', '_');
        फर्काऊ name;
    }

    // Capture an iff guard in a function-local temporary so it is evaluated once per sample()
    AstVarRef* captureIffToTemp(Ast_गाँठो_अभिव्यक्ति* iffp, अचर string& tempName) {
        File_पङ्क्ति* अचर fl = iffp->fileline();
        AstVar* अचर iffVarp
            = नूतन AstVar{fl, VVarType::BLOCKTEMP, tempName, iffp->findBitDType()};
        iffVarp->funcLocal(सत्य);
        m_sampleFuncp->addStmtsp(iffVarp);
        iffp->unlinkFrBack();
        m_sampleFuncp->addStmtsp(
            नूतन AstAssign{fl, नूतन AstVarRef{fl, iffVarp, VAccess::WRITE}, iffp});
        फर्काऊ नूतन AstVarRef{fl, iffVarp, VAccess::READ};
    }

    // Add a statement sampling the current coverpoint to sample(), under its iff guard if any
    शून्य addSampleStmt(Ast_गाँठो* stmtp) {
        यदि (m_cpGuardp) {
            m_cpGuardp->addThensp(stmtp);
        } अन्यथा {
            m_sampleFuncp->addStmtsp(stmtp);
        }
    }

    // Create previous value variable for transition tracking
    AstVar* create_Prev_मान_Var(AstCoverpoint* coverpointp, Ast_गाँठो_अभिव्यक्ति* exprp) {
        // Check if already created
        यदि (AstVar* अचर prevVarp = VN_CAST(coverpointp->user1p(), Var)) फर्काऊ prevVarp;

        // Create variable to store previous sampled value
        अचर string varName = "__Vprev_" + coverpointp->name();
        AstVar* prevVarp
            = नूतन AstVar{coverpointp->fileline(), VVarType::MEMBER, varName, exprp->dtypep()};
        m_covergroupp->addMembersp(prevVarp);

        UINFO(4, "    Created previous value variable: " << varName);

        // Initialize to zero in constructor
        Ast_गाँठो_अभिव्यक्ति* अचर initExprp
            = नूतन AstConst{prevVarp->fileline(), AstConst::Widthed_मान{}, prevVarp->width(), 0};
        Ast_गाँठो_Stmt* अचर initStmtp = नूतन AstAssign{
            prevVarp->fileline(), नूतन AstVarRef{prevVarp->fileline(), prevVarp, VAccess::WRITE},
            initExprp};
        m_constructorp->addStmtsp(initStmtp);

        coverpointp->user1p(prevVarp);
        फर्काऊ prevVarp;
    }

    // Create state position variable for multi-value transition bins
    // Tracks position in sequence: 0=not started, 1=seen first item, etc.
    AstVar* createSequenceStateVar(AstCoverpoint* coverpointp, AstCoverBin* binp) {
        // Create variable to track sequence position
        अचर string varName = "__Vseqpos_" + coverpointp->name() + "_" + binp->name();
        // Use 8-bit integer for state position (sequences rarely > 255 items)
        AstVar* stateVarp
            = नूतन AstVar{binp->fileline(), VVarType::MEMBER, varName, VFlagLogicPacked{}, 8};
        m_covergroupp->addMembersp(stateVarp);

        UINFO(4, "    Created sequence state variable: " << varName);

        // Initialize to 0 (not started) in constructor
        Ast_गाँठो_Stmt* अचर initStmtp = नूतन AstAssign{
            stateVarp->fileline(), नूतन AstVarRef{stateVarp->fileline(), stateVarp, VAccess::WRITE},
            नूतन AstConst{stateVarp->fileline(), AstConst::Widthed_मान{}, 8, 0}};
        m_constructorp->addStmtsp(initStmtp);

        फर्काऊ stateVarp;
    }

    शून्य generateCoverpointCode(AstCoverpoint* coverpointp) {
        UINFO(4, "  Generating code for coverpoint: " << coverpointp->name());

        // Get the coverpoint expression
        Ast_गाँठो_अभिव्यक्ति* exprp = coverpointp->exprp();

        // Check automatic bins before processing
        checkAutomaticBins(coverpointp, exprp);

        // Extract all coverpoint options in a single pass
        पूर्णाङ्क at_Least_मान;
        पूर्णाङ्क autoBinMax;
        extractItemOptions(coverpointp->optionsp(), at_Least_मान, autoBinMax);
        UINFO(6, "    Coverpoint at_least = " << at_Least_मान << " auto_bin_max = " << autoBinMax);

        // Create implicit automatic bins if no regular bins exist
        createImplicitAutoBins(coverpointp, exprp, autoBinMax);

        // A false iff guard disables sampling of the coverpoint (IEEE 1800-2023 19.5), so
        // neither is its expression evaluated, nor are its bins or transition state updated
        VL_RESTORER(m_cpGuardp);
        यदि (Ast_गाँठो_अभिव्यक्ति* अचर iffp = coverpointp->iffp()) {
            m_cpGuardp = नूतन AstIf{iffp->fileline(), iffp->unlinkFrBack()};
        }

        AstVar* अचर मान_Varp = नूतन AstVar{
            coverpointp->fileline(), VVarType::BLOCKTEMP,
            "__VcpValue_" + sanitizeGeneratedName(coverpointp->name()), exprp->dtypep()};
        मान_Varp->funcLocal(सत्य);
        m_sampleFuncp->addStmtsp(मान_Varp);
        exprp->unlinkFrBack();
        addSampleStmt(नूतन AstAssign{
            coverpointp->fileline(),
            नूतन AstVarRef{coverpointp->fileline(), मान_Varp, VAccess::WRITE}, exprp});
        coverpointp->exprp(नूतन AstVarRef{coverpointp->fileline(), मान_Varp, VAccess::READ});
        exprp = coverpointp->exprp();

        // Every coverpoint routes through the VlCoverpoint runtime.  Transition coverpoints are
        // included: their per-value matching is still generated as a state machine in sample()
        // (see generateCoverpoint), but the bin hit is recorded in the runtime bin
        // rather than a bare counter.
        generateCoverpoint(coverpointp, exprp, at_Least_मान);
        // Sampling follows what generateCoverpoint added unguarded, such as clearing the hit list
        यदि (m_cpGuardp) m_sampleFuncp->addStmtsp(m_cpGuardp);
    }

    // Build the condition under which a default bin matches: NOT(OR of all normal bins).
    Ast_गाँठो_अभिव्यक्ति* buildDefaultCondition(AstCoverpoint* coverpointp, Ast_गाँठो_अभिव्यक्ति* exprp,
                                       File_पङ्क्ति* fl) {
        Ast_गाँठो_अभिव्यक्ति* anyBinMatchp = nullptr;
        लागि (Ast_गाँठो* binp = coverpointp->binsp(); binp; binp = binp->nextp()) {
            AstCoverBin* अचर cbinp = VN_AS(binp, CoverBin);
            यदि (cbinp->binsType() == VCoverBinsType::BINS_DEFAULT
                || cbinp->binsType() == VCoverBinsType::BINS_IGNORE
                || cbinp->binsType() == VCoverBinsType::BINS_ILLEGAL)
                जारी;
            // The values of constructed bins are known at construction; see emitSizedSample
            यदि (isConstructedBins(cbinp)) जारी;
            यदि (isAutoBins(cbinp)) {
                // Automatic bins partition the whole domain, leaving no default value
                यदि (anyBinMatchp) VL_DO_DANGLING(थप_Deletep(anyBinMatchp), anyBinMatchp);
                फर्काऊ नूतन AstConst{fl, AstConst::BitFalse{}};
            }
            Ast_गाँठो_अभिव्यक्ति* अचर binCondp = buildBinCondition(cbinp, exprp);
            UASSERT_OBJ(binCondp, cbinp,
                        "buildBinCondition returned nullptr for non-ignore/non-illegal bin");
            anyBinMatchp = anyBinMatchp ? नूतन AstOr{fl, anyBinMatchp, binCondp} : binCondp;
        }
        फर्काऊ anyBinMatchp ? static_cast<Ast_गाँठो_अभिव्यक्ति*>(नूतन AstNot{fl, anyBinMatchp})
                            : static_cast<Ast_गाँठो_अभिव्यक्ति*>(नूतन AstConst{fl, AstConst::BitTrue{}});
    }

    //====================================================================
    // VlCoverpoint conversion

    स्थिर तार्किक coverpointHasStateExclusions(अचर AstCoverpoint* coverpointp) {
        लागि (अचर Ast_गाँठो* nodep = coverpointp->binsp(); nodep; nodep = nodep->nextp()) {
            अचर AstCoverBin* अचर binp = VN_AS(nodep, CoverBin);
            यदि (!binp->transp() && binp->rangesp()
                && (binp->binsType() == VCoverBinsType::BINS_IGNORE
                    || binp->binsType() == VCoverBinsType::BINS_ILLEGAL)) {
                फर्काऊ सत्य;
            }
        }
        फर्काऊ असत्य;
    }

    स्थिर तार्किक coverpointHasConstructedBins(अचर AstCoverpoint* coverpointp) {
        लागि (अचर Ast_गाँठो* nodep = coverpointp->binsp(); nodep; nodep = nodep->nextp()) {
            यदि (isConstructedBins(VN_AS(nodep, CoverBin))) फर्काऊ सत्य;
        }
        फर्काऊ असत्य;
    }

    // True if a Normal state bin, or an array-bin element, has no value of the coverpoint's
    // type (IEEE 1800-2023 19.5.7).  Array ranges enumerate in-type values, so cannot vanish.
    स्थिर तार्किक coverpointHasEmptyBins(अचर AstCoverpoint* coverpointp) {
        Ast_गाँठो_अभिव्यक्ति* अचर exprp = coverpointp->exprp();
        लागि (Ast_गाँठो* nodep = coverpointp->binsp(); nodep; nodep = nodep->nextp()) {
            अचर AstCoverBin* अचर binp = VN_AS(nodep, CoverBin);
            यदि (!binp->binsType().binIsNormal() || binp->transp() || !binp->rangesp()) जारी;
            // A wildcard array has a bin for each value its elements match, so none empty, and
            // the constructor creates no bin without values of a 'with' filter
            यदि ((binp->isArray() && binp->isWildcard()) || binWith(binp)) जारी;
            तार्किक empty = सत्य;
            लागि (Ast_गाँठो* valuep = binp->rangesp(); valuep; valuep = valuep->nextp()) {
                यदि (binp->isArray() && VN_IS(valuep, InsideRange)) जारी;
                Cross_मान_Range range{valuep, resolveWidth(valuep, exprp)};
                अचर तार्किक none = resolve_मान(valuep, exprp, सत्य, binp->isWildcard(), range)
                                  && crossRangeEmpty(range);
                यदि (binp->isArray() && none) फर्काऊ सत्य;
                empty &= none;
            }
            यदि (empty && !binp->isArray()) फर्काऊ सत्य;
        }
        फर्काऊ असत्य;
    }

    // True if a coverpoint has any transition bin.  Used to decide whether sample() emits the
    // end-of-sample previous-value update that transition matching needs.
    स्थिर तार्किक coverpointHasTransition(AstCoverpoint* coverpointp) {
        लागि (Ast_गाँठो* binp = coverpointp->binsp(); binp; binp = binp->nextp()) {
            यदि (VN_AS(binp, CoverBin)->transp()) फर्काऊ सत्य;
        }
        फर्काऊ असत्य;
    }

    // The interned AstBasicDType for one of the covergroup runtime keywords.
    स्थिर AstBasicDType* basicDType(File_पङ्क्ति* fl, VBasicDTypeKwd kwd) {
        फर्काऊ v3Global.rootp()->typeTablep()->findBasicDType(fl, kwd);
    }

    // Get (or create) the coverpoint dtype for a hit-list bound, interned so each distinct bound
    // yields one node.
    AstCoverpointDType* coverpointDType(File_पङ्क्ति* fl, uint32_t hitBound) {
        AstCoverpointDType*& typep = m_cpDTypes[hitBound];
        यदि (!typep) {
            typep = नूतन AstCoverpointDType{fl, hitBound};
            v3Global.rootp()->typeTablep()->addTypesp(typep);
        }
        फर्काऊ typep;
    }

    // The key of the covergroup's type in the coverage registry, which get_coverage() queries
    std::string covergroupProtectedName() अचर {
        फर्काऊ VIdProtect::protect_शब्दहरू_If(m_covergroupName, v3Global.opt.protectIds());
    }

    // The name of the covergroup's type, which keys the coverage registry and database: as
    // $typename names it (IEEE 1800-2023 20.6.1), so apart for covergroups of distinct scopes and
    // specializations, and alike in each Verilator run of a hierarchical design.  As design units
    // of distinct libraries may share a name, one of a library other than the default is prefixed
    // by its library, as '%l' prints it (IEEE 1800-2023 33.4).
    std::string covergroupTypeName() अचर {
        UASSERT_OBJ(m_unitp, m_covergroupp, "Covergroup declared outside of a design unit");
        अचर std::string& libname = m_unitp->libname();
        अचर std::string name = m_covergroupp->dtypeName(सत्य);
        फर्काऊ libname == "work" ? name : libname + "." + name;
    }

    // Emit the covergroup's instance handle member and the constructor statement that creates
    // its node in the per-context coverage registry.  Runs before any coverpoint or cross is
    // generated, so their runtimes can be added to the node as they are created.
    शून्य generateInstanceAttach() {
        File_पङ्क्ति* अचर fl = m_covergroupp->fileline();
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
                     /*usePtr=*/असत्य)
                ->makeStmt());
        // The node reads option.weight in place, so procedural assignments take effect
        AstCExpr* अचर weightAddrp = नूतन AstCExpr{fl, "&"};
        weightAddrp->add(newOptionSel(fl, optionVar(असत्य), "weight", VAccess::READ));
        m_constructorp->addStmtsp(itemCall(fl, m_cgInstVarp, VCMethod::COVERGROUP_LEND_WEIGHT,
                                           {weightAddrp, file_पङ्क्ति_Debug(fl)}, /*usePtr=*/असत्य)
                                      ->makeStmt());
    }

    // A '__Vcg_inst.p()-><method>()' call on the covergroup's instance node
    AstCMethodHard* instanceCall(File_पङ्क्ति* fl, VCMethod method) {
        // '__Vcg_inst.p()' -- a value handle, so '.' not '->'
        AstCMethodHard* अचर instp
            = नूतन AstCMethodHard{fl, memberRef(fl, m_cgInstVarp), VCMethod::COVERGROUP_INST_P};
        instp->usePtr(असत्य);
        instp->dtypeSetVoid();  // Opaque receiver; only ever the 'fromp' of the call below
        AstCMethodHard* अचर callp = नूतन AstCMethodHard{fl, instp, method};
        callp->usePtr(सत्य);
        फर्काऊ callp;
    }

    // Emit 'this->__Vcp_x = this->__Vcg_inst.p()->addCoverpoint<K>();' (or addCross), which
    // creates the item runtime in the instance node and borrows a pointer to it.
    AstAssign* makeItemCreate(File_पङ्क्ति* fl, AstVar* itemVarp, VCMethod method) {
        AstCMethodHard* अचर createp = instanceCall(fl, method);
        createp->dtypep(itemVarp->dtypep());
        फर्काऊ नूतन AstAssign{fl, memberRef(fl, itemVarp, VAccess::WRITE), createp};
    }

    // Constant bounds of one rangesp() element (an InsideRange or a single Const).  Each bound is
    // the raw AST node -- an AstConst or an AstUnbounded ('$') -- with the const/unbounded view
    // derived on demand, so there is one source of truth per bound.  After a successful
    // constRangeBounds() neither node is null; a single Const has both bounds aliasing one node.
    संरचना RangeBounds final {
        Ast_गाँठो* loNodep = nullptr;  // low bound: AstConst or AstUnbounded
        Ast_गाँठो* hiNodep = nullptr;  // high bound: AstConst or AstUnbounded
        तार्किक loUnbounded() अचर { फर्काऊ VN_IS(loNodep, Unbounded); }
        तार्किक hiUnbounded() अचर { फर्काऊ VN_IS(hiNodep, Unbounded); }
        AstConst* loConstp() अचर { फर्काऊ VN_CAST(loNodep, Const); }
        AstConst* hiConstp() अचर { फर्काऊ VN_CAST(hiNodep, Const); }
    };

    // Decode one rangesp() element into its constant bounds.  Returns false if rp is neither an
    // InsideRange nor a single Const, or if a present bound is non-constant or 4-state.  '$'
    // bounds are left as AstUnbounded (not resolved) -- the caller applies its own policy.
    // Centralizes the InsideRange/Const/Unbounded decode shared by the hit-list-bound paths.
    स्थिर तार्किक constRangeBounds(Ast_गाँठो* rp, RangeBounds& rb) {
        यदि (AstInsideRange* अचर irp = VN_CAST(rp, InsideRange)) {
            rb.loNodep = irp->lhsp();
            rb.hiNodep = irp->rhsp();
        } अन्यथा यदि (AstConst* अचर cp = VN_CAST(rp, Const)) {
            rb.loNodep = rb.hiNodep = cp;
        } अन्यथा {
            फर्काऊ असत्य;
        }
        // Each bound must be a constant unless it is '$'; reject non-const and 4-state.
        AstConst* अचर lc = rb.loConstp();
        AstConst* अचर hc = rb.hiConstp();
        यदि ((!lc && !rb.loUnbounded()) || (!hc && !rb.hiUnbounded())) फर्काऊ असत्य;
        यदि ((lc && lc->अङ्क().isFourState()) || (hc && hc->अङ्क().isFourState())) फर्काऊ असत्य;
        फर्काऊ सत्य;
    }

    // Collect the covered value intervals of a single (non-array) Normal bin.  Returns false
    // if any range isn't a constant/open InsideRange or single Const (e.g. wildcard, non-const).
    स्थिर तार्किक extractRangeIntervals(AstCoverBin* cbinp, uint64_t maxVal,
                                      std::vector<std::pair<uint64_t, uint64_t>>& out) {
        यदि (!cbinp->rangesp()) फर्काऊ असत्य;
        लागि (Ast_गाँठो* rp = cbinp->rangesp(); rp; rp = rp->nextp()) {
            RangeBounds rb;
            यदि (!constRangeBounds(rp, rb)) फर्काऊ असत्य;
            यदि ((rb.loConstp() && rb.loConstp()->width() > 64)
                || (rb.hiConstp() && rb.hiConstp()->width() > 64)) {
                फर्काऊ असत्य;  // Use the safe slot-count bound for wide values.
            }
            अचर uint64_t lo = rb.loUnbounded() ? 0 : rb.loConstp()->toUQuad();
            अचर uint64_t hi = rb.hiUnbounded() ? maxVal : rb.hiConstp()->toUQuad();
            यदि (lo > hi) फर्काऊ असत्य;
            out.emplace_back(lo, hi);
        }
        फर्काऊ सत्य;
    }

    // Append one Normal bin's cross-slot interval-sets to `bins` and bump `slotCount` by an upper
    // bound of the bin's slots holding one value.  Returns false if any part isn't statically
    // enumerable (the caller then falls back to the always-safe slot count).  A non-array bin
    // is one slot covering the union of its intervals; the bins of an array element or of an
    // automatic bins declaration hold disjoint values, so the element or declaration counts once.
    तार्किक appendBinCrossSlots(AstCoverBin* cbinp, uint64_t maxVal, Ast_गाँठो_अभिव्यक्ति* exprp,
                             std::vector<std::vector<std::pair<uint64_t, uint64_t>>>& bins,
                             पूर्णाङ्क& slot_गन्ती) {
        यदि (isAutoBins(cbinp)) {
            ++slot_गन्ती;
            bins.थप_back({{0, maxVal}});
            फर्काऊ exprp->width() <= 64;
        }
        यदि (binWith(cbinp)) {
            // A value is in one bin of a filter's, but of a sized array, in one for each range
            // list element holding it; the coverpoint's name is one element
            पूर्णाङ्क elements = 0;
            लागि (अचर Ast_गाँठो* rp = binRangesp(cbinp); rp && isSizedArray(cbinp);
                 rp = rp->nextp()) {
                ++elements;
            }
            slot_गन्ती += std::max(1, elements);
            फर्काऊ असत्य;
        }
        यदि (cbinp->isArray() && cbinp->isWildcard() && !cbinp->arraySizep()) {
            // A value is in at most one bin of a wildcard array: one slot covering its values.
            // Signed values are sign-extended, not unsigned intervals (see computeHitListBound)
            ++slot_गन्ती;
            यदि (exprp->isSigned() || exprp->width() > 64) फर्काऊ असत्य;
            अचर BinRuns runs = wildcardBinRuns(cbinp, exprp, असत्य);
            यदि (runs.unsupported) फर्काऊ असत्य;
            std::vector<std::pair<uint64_t, uint64_t>> ivs;
            लागि (अचर BinRun& run : runs.runs) {
                ivs.emplace_back(run.m_lo.toUQuad(), run.m_hi.toUQuad());
            }
            bins.थप_back(std::move(ivs));
            फर्काऊ सत्य;
        }
        यदि (cbinp->isArray()) फर्काऊ appendArrayBinCrossSlots(cbinp, exprp, bins, slot_गन्ती);
        // Non-array bin: one slot covering the union of its intervals.
        ++slot_गन्ती;
        std::vector<std::pair<uint64_t, uint64_t>> ivs;
        यदि (cbinp->isWildcard() || !extractRangeIntervals(cbinp, maxVal, ivs)) फर्काऊ असत्य;
        bins.थप_back(std::move(ivs));
        फर्काऊ सत्य;
    }

    // Append the cross slots of an array Normal bin: each element is a slot covering its
    // values, as at most one of its single-value bins holds a value.  An element holds the
    // values arrayBinRuns() gives it: those of the coverpoint type (IEEE 1800-2023 19.5.7).
    // Elements of a signed coverpoint, non-constant elements, and elements with values beyond
    // 64 bits can't be enumerated, so they count one slot but lose exactness.  Returns false if
    // any element wasn't enumerable to exact values.
    तार्किक appendArrayBinCrossSlots(AstCoverBin* cbinp, Ast_गाँठो_अभिव्यक्ति* exprp,
                                  std::vector<std::vector<std::pair<uint64_t, uint64_t>>>& bins,
                                  पूर्णाङ्क& slot_गन्ती) {
        // Signed values resolve sign-extended, not as unsigned intervals, and wildcard patterns
        // are not intervals
        तार्किक exact = !exprp->isSigned() && !cbinp->isWildcard();
        लागि (Ast_गाँठो* rp = cbinp->rangesp(); rp; rp = rp->nextp()) {
            ++slot_गन्ती;
            RangeBounds rb;
            Cross_मान_Range range{rp, resolveWidth(rp, exprp)};
            यदि (!exact || !constRangeBounds(rp, rb)
                || !resolve_मान(rp, exprp, सत्य, असत्य, range)) {
                exact = असत्य;
                जारी;
            }
            यदि (crossRangeEmpty(range)) जारी;  // Its bin, if any, holds no value
            यदि (range.hi.mostSetBitP1() > 64) {
                exact = असत्य;
                जारी;
            }
            bins.थप_back({{range.lo.toUQuad(), range.hi.toUQuad()}});
        }
        फर्काऊ exact;
    }

    // Compute the hit-list bound for a coverpoint: the maximum number of Normal
    // bins one sample value can match.  Non-cross-fed coverpoints don't feed a cross, so
    // their hit list is unused -> 1.  Otherwise compute the exact max bin overlap; fall back
    // to the (always-safe) Normal-slot count when any bin isn't statically analyzable.
    पूर्णाङ्क computeHitListBound(AstCoverpoint* coverpointp, Ast_गाँठो_अभिव्यक्ति* exprp, तार्किक crossFed) {
        यदि (!crossFed) फर्काऊ 1;
        अचर पूर्णाङ्क width = exprp->width();
        अचर uint64_t maxVal = (width >= 64) ? UINT64_MAX : ((1ULL << width) - 1);
        // One entry per Normal bin (cross slot): its covered intervals.
        std::vector<std::vector<std::pair<uint64_t, uint64_t>>> bins;
        पूर्णाङ्क slot_गन्ती = 0;  // == runtime m_normal; the safe fallback bound
        // Unsigned intervals cannot establish overlap between differently sized signed values.
        तार्किक exact = !exprp->isSigned();
        लागि (Ast_गाँठो* binp = coverpointp->binsp(); binp; binp = binp->nextp()) {
            AstCoverBin* अचर cbinp = VN_AS(binp, CoverBin);
            यदि (!cbinp->binsType().binIsNormal())
                जारी;  // ignore/illegal/default: not hit-listed
            यदि (!appendBinCrossSlots(cbinp, maxVal, exprp, bins, slot_गन्ती)) exact = असत्य;
        }
        यदि (!exact) फर्काऊ std::max(1, slot_गन्ती);
        यदि (bins.empty()) फर्काऊ 1;
        // Max overlap occurs at some interval start; count covering bins at each lo.
        std::vector<uint64_t> pts;
        लागि (अचर स्वतः& b : bins)
            लागि (अचर स्वतः& iv : b) pts.थप_back(iv.first);
        std::sort(pts.begin(), pts.end());
        pts.erase(std::unique(pts.begin(), pts.end()), pts.end());
        पूर्णाङ्क maxOverlap = 1;
        लागि (अचर uint64_t p : pts) {
            पूर्णाङ्क cnt = 0;
            लागि (अचर स्वतः& b : bins) {
                लागि (अचर स्वतः& iv : b) {
                    यदि (iv.first <= p && p <= iv.second) {
                        ++cnt;
                        रोक;  // count each bin at most once
                    }
                }
            }
            यदि (cnt > maxOverlap) maxOverlap = cnt;
        }
        फर्काऊ maxOverlap;
    }

    // A 'this->m_member' reference for embedding in an AstCStmt
    AstVarRef* memberRef(File_पङ्क्ति* fl, AstVar* varp, VAccess access = VAccess::READ) {
        AstVarRef* अचर refp = नूतन AstVarRef{fl, varp, access};
        refp->selfPointer(VSelfPointerText{VSelfPointerText::This{}});
        फर्काऊ refp;
    }

    // A 'this->m_member-><method>(args...)' call on one member.  usePtr is false only for
    // __Vcg_inst, which is a value handle; the item members are borrowed pointers into the
    // instance node.  Numeric arguments are AstConst; the rest are C++ text that has no AST
    // form (see ctext).
    AstCMethodHard* itemCall(File_पङ्क्ति* fl, AstVar* varp, VCMethod method,
                             अचर std::vector<Ast_गाँठो_अभिव्यक्ति*>& args = {}, तार्किक usePtr = सत्य) {
        AstCMethodHard* अचर callp = नूतन AstCMethodHard{fl, memberRef(fl, varp), method};
        लागि (Ast_गाँठो_अभिव्यक्ति* अचर argp : args) callp->addPinsp(argp);
        callp->usePtr(usePtr);
        callp->dtypeSetVoid();
        फर्काऊ callp;
    }

    // An unsigned integer argument.
    स्थिर AstConst* cnum(File_पङ्क्ति* fl, uint32_t मान) { फर्काऊ नूतन AstConst{fl, मान}; }

    // A literal C++ argument with no AST equivalent: a 'const char*' string literal (an SV
    // string AstConst emits '"..."s', a std::string temporary the runtime cannot borrow), a
    // VlCovBinKind enum token, a constant selection-word initializer list, a VlFileLineDebug, or
    // a '__V' temporary declared by the enclosing AstCStmt.
    स्थिर AstCExpr* ctext(File_पङ्क्ति* fl, अचर std::string& text) {
        फर्काऊ नूतन AstCExpr{fl, text};
    }

    // A C++ string literal.  Escapes control characters as the emitter does elsewhere -- bin
    // names and filenames reach the generated code verbatim when --protect-ids is off, and an
    // SV escaped identifier may hold a quote or backslash.
    स्थिर std::string quoted(अचर std::string& text) {
        फर्काऊ "\"" + V3OutFormatter::quoteNameControls(text) + "\"";
    }

    // A 'VlFileLineDebug' argument: where the runtime reports an error about fl's construct
    स्थिर AstCExpr* file_पङ्क्ति_Debug(File_पङ्क्ति* fl) {
        अचर std::string filename
            = VIdProtect::protectIf(fl->filename(), v3Global.opt.protectIds());
        फर्काऊ ctext(fl, "VlFileLineDebug{" + quoted(filename) + ", "
                             + std::to_string(fl->lineno()) + "}");
    }

    // Check that an element of an array bin (bins b[] = {values/ranges}) is a two-state
    // constant value or range; false if not, after reporting it if 'report'.
    स्थिर तार्किक checkArrayBinElement(AstCoverBin* arrayBinp, Ast_गाँठो* rangep, तार्किक report = सत्य) {
        यदि (अचर AstInsideRange* अचर irp = VN_CAST(rangep, InsideRange)) {
            अचर AstConst* अचर minp = VN_CAST(irp->lhsp(), Const);
            अचर AstConst* अचर maxp = VN_CAST(irp->rhsp(), Const);
            यदि ((!minp && !VN_IS(irp->lhsp(), Unbounded))
                || (!maxp && !VN_IS(irp->rhsp(), Unbounded))) {
                यदि (report) {
                    arrayBinp->v3error("Non-constant expression in array bins range; "
                                       "range bounds must be constants (IEEE 1800-2023 19.5)");
                }
                फर्काऊ असत्य;
            }
            यदि ((minp && minp->अङ्क().isFourState()) || (maxp && maxp->अङ्क().isFourState())) {
                यदि (report) {
                    arrayBinp->v3error("Four-state (x/z) value in array bins range bound; "
                                       "range bounds must be two-state constants");
                }
                फर्काऊ असत्य;
            }
        } अन्यथा यदि (!VN_IS(rangep, Const)) {
            यदि (report) {
                arrayBinp->v3error("Non-constant expression in array bins value list; "
                                   "values must be constants (IEEE 1800-2023 19.5)");
            }
            फर्काऊ असत्य;
        }
        फर्काऊ सत्य;
    }

    // Individual equality targets of an array bin (bins b[] = {values/ranges}) of a real
    // coverpoint, in order; integral coverpoints generate array bins as runs (see arrayBinRuns).
    // An open-ended bound ('$', AstUnbounded) resolves to the coverpoint domain: '[lo:$]'
    // covers [lo:maxVal] and '[$:hi]' covers [0:hi].  One target is produced per value; ranges
    // whose resolved size would exceed --coverage-max-real-bins (e.g. an open '[lo:$]') are
    // unsupported -- emits COVERIGN, sets unsupportedOut, yields nothing.
    std::vector<Ast_गाँठो_अभिव्यक्ति*> extract_Array_मानहरू(AstCoverBin* arrayBinp, Ast_गाँठो_अभिव्यक्ति* exprp,
                                                 तार्किक& unsupportedOut) {
        unsupportedOut = असत्य;
        अचर पूर्णाङ्क width = exprp->width();
        अचर uint64_t maxVal = (width >= 64) ? UINT64_MAX : ((1ULL << width) - 1);
        std::vector<Ast_गाँठो_अभिव्यक्ति*> मानहरू;
        लागि (Ast_गाँठो* rangep = arrayBinp->rangesp(); rangep; rangep = rangep->nextp()) {
            rangep = V3Const::constifyEdit(rangep);
            यदि (!checkArrayBinElement(arrayBinp, rangep)) फर्काऊ मानहरू;
            यदि (AstInsideRange* अचर irp = VN_CAST(rangep, InsideRange)) {
                अचर तार्किक loUnb = VN_IS(irp->lhsp(), Unbounded);
                अचर तार्किक hiUnb = VN_IS(irp->rhsp(), Unbounded);
                अचर uint64_t lo = loUnb ? 0 : VN_AS(irp->lhsp(), Const)->toUQuad();
                अचर uint64_t hi = hiUnb ? maxVal : VN_AS(irp->rhsp(), Const)->toUQuad();
                यदि (hi < lo) जारी;  // empty range contributes no bins
                // Guard against a '$'-bounded or otherwise huge range exploding the bin count.
                अचर uint64_t span = hi - lo;  // == valueCount - 1 (no overflow: hi >= lo)
                यदि (span >= v3Global.opt.coverageMaxRealBins()
                    || मानहरू.size() + span + 1 > v3Global.opt.coverageMaxRealBins()) {
                    arrayBinp->v3warn(COVERIGN,
                                      "Unsupported: array 'bins' of a real coverpoint "
                                      "covering more than "
                                          << v3Global.opt.coverageMaxRealBins() << " values; bin "
                                          << arrayBinp->prettyNameQ() << " ignored.\n"
                                          << arrayBinp->warnMore()
                                          << "... Suggest a larger --coverage-max-real-bins");
                    unsupportedOut = सत्य;
                    लागि (Ast_गाँठो_अभिव्यक्ति* अचर vp : मानहरू) VL_DO_DANGLING(थप_Deletep(vp), vp);
                    मानहरू.clear();
                    फर्काऊ मानहरू;
                }
                लागि (uint64_t v = lo; v <= hi; ++v)
                    मानहरू.थप_back(नूतन AstConst{irp->fileline(), AstConst::Widthed_मान{}, width,
                                                  static_cast<uint32_t>(v)});
            } अन्यथा {
                मानहरू.थप_back(VN_AS(rangep->clone_रूख(असत्य), गाँठो_अभिव्यक्ति));
            }
        }
        फर्काऊ मानहरू;
    }

    स्थिर पूर्णाङ्क runWidth(अचर Ast_गाँठो_अभिव्यक्ति* exprp) { फर्काऊ exprp->width() + 1; }

    // Automatic bins partition the coverpoint domain in value order (IEEE 1800-2023 19.5.3): N
    // bins, capped at the number of values, each hold 2^width / N values, and the last bin also
    // holds the remainder.  False for an invalid declaration, already reported.
    तार्किक autoBinRuns(AstCoverBin* binp, Ast_गाँठो_अभिव्यक्ति* exprp, BinRuns& out) {
        अचर uint32_t requested = autoBinsRequested(binp);
        यदि (!requested || !exprp->dtypep()->skipRefp()->isIntegralOrPacked()) फर्काऊ असत्य;
        अचर पूर्णाङ्क width = exprp->width();
        अचर पूर्णाङ्क arithmeticWidth = runWidth(exprp);
        अचर uint32_t गन्ती
            = width < 32
                  ? static_cast<uint32_t>(std::min<uint64_t>(uint64_t{1} << width, requested))
                  : requested;
        BinRun run{binp, arithmeticWidth, गन्ती};
        V3_सङ्ख्या total{binp, arithmeticWidth};
        total.setBit(width, 1);
        run.m_stride.opDiv(total, V3_सङ्ख्या{binp, arithmeticWidth, गन्ती});
        अचर Cross_मान_Range domain
            = cross_मान_Domain(binp, width, exprp->isSigned(), arithmeticWidth);
        run.m_lo = domain.lo;
        run.m_hi = domain.hi;
        out.runs.थप_back(std::move(run));
        out.गन्ती = गन्ती;
        फर्काऊ सत्य;
    }

    // The elements of an array bin (bins b[] = {values/ranges}), in order, as runs of
    // single-value bins.  A range holds the values of the coverpoint type it contains (IEEE
    // 1800-2023 19.5.7), while a singleton names a bin even without such a value.  Errors on a
    // non-constant element.  More than --coverage-max-bins bins (e.g. an open '[lo:$]' range over
    // a wide coverpoint) are unsupported -- emits COVERIGN, and sets unsupported.
    BinRuns arrayBinRuns(AstCoverBin* arrayBinp, Ast_गाँठो_अभिव्यक्ति* exprp) {
        BinRuns out;
        अचर पूर्णाङ्क width = runWidth(exprp);
        लागि (Ast_गाँठो* rangep = arrayBinp->rangesp(); rangep; rangep = rangep->nextp()) {
            rangep = V3Const::constifyEdit(rangep);
            यदि (!checkArrayBinElement(arrayBinp, rangep)) फर्काऊ out;
            अचर AstInsideRange* अचर irp = VN_CAST(rangep, InsideRange);
            Cross_मान_Range range{rangep, resolveWidth(rangep, exprp)};
            तार्किक empty = सत्य;
            यदि (!resolve_मान(rangep, exprp, सत्य, असत्य, range)) {
                rangep->v3warn(E_UNSUPPORTED, "Unsupported: non-integral value in a coverage bin "
                                              "of an integral coverpoint.");
            } अन्यथा {
                empty = crossRangeEmpty(range);
            }
            यदि (empty && irp) जारी;  // A range without values contributes no bins
            BinRun run{rangep, width, 1};
            run.m_empty = empty;
            uint64_t गन्ती = 1;
            यदि (!empty) {
                run.m_lo.opAssign(range.lo);
                run.m_hi.opAssign(range.hi);
                V3_सङ्ख्या span{rangep, width};
                span.opSub(run.m_hi, run.m_lo);
                // Wider spans exceed any limit
                गन्ती = span.mostSetBitP1() > 32 ? UINT64_MAX : span.toUQuad() + 1;
            }
            यदि (गन्ती > v3Global.opt.coverageMaxBins() - out.गन्ती) {
                arrayBinp->v3warn(COVERIGN, "Unsupported: array 'bins' covering more than "
                                                << v3Global.opt.coverageMaxBins()
                                                << " values (e.g. an open '[lo:$]' range over "
                                                   "a wide coverpoint); bin "
                                                << arrayBinp->prettyNameQ() << " ignored\n"
                                                << arrayBinp->warnMore()
                                                << "... Suggest a larger --coverage-max-bins");
                out.runs.clear();
                out.गन्ती = 0;
                out.unsupported = सत्य;
                फर्काऊ out;
            }
            run.m_गन्ती = static_cast<uint32_t>(गन्ती);
            out.गन्ती += run.m_गन्ती;
            out.runs.थप_back(std::move(run));
        }
        फर्काऊ out;
    }

    // The bins of a wildcard array (wildcard bins b[] = {...}): one for each coverpoint value
    // an element matches (IEEE 1800-2023 19.5.4, 19.5.7), in value order, and named by the value
    // (19.5.1), as runs of single-value bins.  Errors on a non-constant element.  More than
    // --coverage-max-bins values are unsupported -- emits COVERIGN, and sets unsupported, and for
    // an ignore or illegal array, single.  'report' false omits these diagnostics.
    स्थिर BinRuns wildcardBinRuns(AstCoverBin* arrayBinp, Ast_गाँठो_अभिव्यक्ति* exprp, तार्किक report) {
        BinRuns out;
        अचर पूर्णाङ्क width = runWidth(exprp);
        अचर V3_सङ्ख्या one{arrayBinp, width, 1};
        // Disjoint runs of the values, in value order, each value once
        std::vector<std::pair<V3_सङ्ख्या, V3_सङ्ख्या>> spans;
        uint64_t गन्ती = 0;  // Values of 'spans'
        लागि (Ast_गाँठो* rangep = arrayBinp->rangesp(); rangep; rangep = rangep->nextp()) {
            rangep = V3Const::constifyEdit(rangep);
            यदि (!checkArrayBinElement(arrayBinp, rangep, report)) {
                out.unsupported = सत्य;
                फर्काऊ out;
            }
            Cross_मान_Range range{rangep, resolveWidth(rangep, exprp)};
            यदि (!resolve_मान(rangep, exprp, सत्य, सत्य, range)) {
                यदि (report) {
                    rangep->v3warn(E_UNSUPPORTED, "Unsupported: non-integral value in a "
                                                  "coverage bin of an integral coverpoint.");
                }
                जारी;
            }
            यदि (crossRangeEmpty(range)) जारी;
            std::vector<std::pair<V3_सङ्ख्या, V3_सङ्ख्या>> found;
            crossRangeRuns(range, v3Global.opt.coverageMaxBins(), found);
            लागि (अचर std::pair<V3_सङ्ख्या, V3_सङ्ख्या>& run : found) {
                // Coverpoint values, sign-extended in both widths
                spans.emplace_back(V3_सङ्ख्या{rangep, width, run.first},
                                   V3_सङ्ख्या{rangep, width, run.second});
            }
            std::sort(spans.begin(), spans.end(), [](अचर स्वतः& lhs, अचर स्वतः& rhs) {
                फर्काऊ cross_मान_Less(lhs.first, rhs.first);
            });
            std::vector<std::pair<V3_सङ्ख्या, V3_सङ्ख्या>> merged;
            लागि (अचर std::pair<V3_सङ्ख्या, V3_सङ्ख्या>& span : spans) {
                // Adjacent or overlapping values join a run; 'first - 1' cannot overflow
                V3_सङ्ख्या before{rangep, width};
                before.opSub(span.first, one);
                यदि (merged.empty() || cross_मान_Less(merged.back().second, before)) {
                    merged.थप_back(span);
                } अन्यथा यदि (cross_मान_Less(merged.back().second, span.second)) {
                    merged.back().second = span.second;
                }
            }
            गन्ती = 0;
            लागि (अचर std::pair<V3_सङ्ख्या, V3_सङ्ख्या>& span : merged) {
                V3_सङ्ख्या size{rangep, width};
                size.opSub(span.second, span.first);
                // Beyond 2^32 values exceed any limit
                गन्ती += size.mostSetBitP1() > 32 ? uint64_t{1} << 33 : size.toUQuad() + 1;
                यदि (गन्ती > v3Global.opt.coverageMaxBins()) रोक;
            }
            spans = std::move(merged);
            यदि (गन्ती > v3Global.opt.coverageMaxBins()) {
                // An ignore or illegal array still excludes or checks its values, as one bin
                out.single = !arrayBinp->binsType().binIsNormal();
                यदि (report) {
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
                फर्काऊ out;
            }
        }
        लागि (अचर std::pair<V3_सङ्ख्या, V3_सङ्ख्या>& span : spans) {
            out.runs.emplace_back(arrayBinp, width, 0);
            BinRun& run = out.runs.back();
            run.m_lo = span.first;
            run.m_hi = span.second;
            // A bin for each value, named by it in the coverpoint's type
            V3_सङ्ख्या मान = span.first;
            जबसम्म (सत्य) {
                अचर V3_सङ्ख्या typed{arrayBinp, exprp->width(), मान};
                out.मानहरू.थप_back(exprp->isSigned() ? typed.toDecimalS() : typed.toDecimalU());
                ++run.m_गन्ती;
                यदि (मान.isCaseEq(span.second)) रोक;
                V3_सङ्ख्या next{arrayBinp, width};
                मान = next.opAdd(मान, one);
            }
        }
        out.गन्ती = static_cast<uint32_t>(गन्ती);
        फर्काऊ out;
    }

    // The runs of an automatic bins declaration, or of an array bin of an integral coverpoint.
    // False for other bins, which do not generate as runs, including a wildcard array then one
    // bin (see BinRuns::single).
    तार्किक binRunsFor(AstCoverBin* binp, Ast_गाँठो_अभिव्यक्ति* exprp, BinRuns& out) {
        यदि (isAutoBins(binp)) {
            यदि (!autoBinRuns(binp, exprp, out)) out.unsupported = सत्य;
            फर्काऊ सत्य;
        }
        यदि (binp->isArray() && binp->isWildcard()) {
            यदि (!exprp->dtypep()->skipRefp()->isIntegralOrPacked()) {
                Ast_गाँठो_अभिव्यक्ति* अचर falsep = wildcardTypeError(binp, exprp);
                VL_DO_DANGLING(थप_Deletep(falsep), falsep);
                out.unsupported = सत्य;
            } अन्यथा {
                out = wildcardBinRuns(binp, exprp, सत्य);
                यदि (out.single) {
                    binp->isArray(असत्य);  // Generates as one bin
                    फर्काऊ असत्य;
                }
            }
            फर्काऊ सत्य;
        }
        यदि (!binp->isArray() || binp->transp() || binp->isWildcard()
            || !exprp->dtypep()->skipRefp()->isIntegralOrPacked()) {
            फर्काऊ असत्य;
        }
        out = arrayBinRuns(binp, exprp);
        फर्काऊ सत्य;
    }

    // Emit a 'this->m_cp->addSingleNamer/addArrayNamer(...)' statement for one bin whose first
    // runtime bin index is 'declared'; or with 'valueNames', those naming each bin of an array
    Ast_गाँठो_Stmt* makeNamer(AstVar* cpVarp, AstCoverBin* binp, int64_t गन्ती, uint32_t declared,
                           अचर std::vector<Ast_गाँठो_अभिव्यक्ति*>& मानहरू = {},
                           अचर std::vector<std::string>& मान_Names = {}) {
        File_पङ्क्ति* अचर fl = binp->fileline();
        CoverpointBins& bins = m_cpBins.at(cpVarp);
        अचर uint32_t normal_गन्ती
            = binp->binsType().binIsNormal() ? static_cast<uint32_t>(गन्ती < 0 ? 1 : गन्ती) : 0;
        अचर BinSpan span{bins.total, normal_गन्ती, declared};
        यदि (binp->binsType() == VCoverBinsType::BINS_AUTO_IMPLICIT) {
            bins.implicitAuto = span;  // Selected by bin, see implicitAutoBinSpan
        } अन्यथा {
            bins.spans.emplace(binp->name(), span);
        }
        bins.total += normal_गन्ती;
        लागि (uint32_t i = 0; bins.crossed && i < normal_गन्ती; ++i) {
            bins.मानहरू.थप_back({binp, मानहरू.empty() ? nullptr : मानहरू[i]});
        }
        // Under --protect-ids the filename and bin name flow into the coverage database
        // verbatim, so obfuscate them exactly as line/toggle coverage points are (whole-
        // unit filename, per-word bin name).  A no-op when --protect-ids is off.
        अचर तार्किक prot = v3Global.opt.protectIds();
        अचर तार्किक single = गन्ती < 0;
        यदि (!मान_Names.empty()) {
            // A bin of a wildcard array is named by its value (IEEE 1800-2023 19.5.1)
            Ast_गाँठो_Stmt* stmtsp = nullptr;
            लागि (अचर std::string& मान : मान_Names) {
                अचर std::string name
                    = VIdProtect::protect_शब्दहरू_If(binp->name(), prot) + "[" + मान + "]";
                stmtsp = Ast_गाँठो::addNext(
                    stmtsp,
                    itemCall(fl, cpVarp, VCMethod::COVERGROUP_ADD_SINGLE_NAMER,
                             {ctext(fl, binp->binsType().binSetEnum()), ctext(fl, quoted(name)),
                              ctext(fl, quoted(VIdProtect::protectIf(fl->filename(), prot))),
                              cnum(fl, static_cast<uint32_t>(fl->lineno())),
                              cnum(fl, static_cast<uint32_t>(fl->firstColumn()))})
                        ->makeStmt());
            }
            फर्काऊ stmtsp;
        }
        std::vector<Ast_गाँठो_अभिव्यक्ति*> args{ctext(fl, binp->binsType().binSetEnum())};
        यदि (!single) args.थप_back(cnum(fl, static_cast<uint32_t>(गन्ती)));  // value array bin
        args.थप_back(ctext(fl, quoted(VIdProtect::protect_शब्दहरू_If(binp->name(), prot))));
        args.थप_back(ctext(fl, quoted(VIdProtect::protectIf(fl->filename(), prot))));
        args.थप_back(cnum(fl, static_cast<uint32_t>(fl->lineno())));
        args.थप_back(cnum(fl, static_cast<uint32_t>(fl->firstColumn())));
        फर्काऊ itemCall(fl, cpVarp,
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
    Ast_गाँठो_Stmt* makeRuntimeBinHit(File_पङ्क्ति* fl, AstVar* cpVarp, Ast_गाँठो_अभिव्यक्ति* idxp,
                                   तार्किक isNormal) {
        फर्काऊ itemCall(fl, cpVarp,
                        isNormal ? VCMethod::COVERGROUP_INCREMENT_BIN
                                 : VCMethod::COVERGROUP_RECORD_HIT,
                        {idxp})
            ->makeStmt();
    }
    Ast_गाँठो_Stmt* makeRuntimeBinHit(File_पङ्क्ति* fl, अचर ConvBinTarget& tgt) {
        फर्काऊ makeRuntimeBinHit(fl, tgt.cpVarp, cnum(fl, static_cast<uint32_t>(tgt.idx)),
                                 tgt.isNormal);
    }

    // The condition under which a bin counts a sample: condp, and the bin's iff, and for a
    // Normal or default state bin, that the value is not excluded
    Ast_गाँठो_अभिव्यक्ति* binCondition(AstCoverBin* binp, AstVar* cpVarp, Ast_गाँठो_अभिव्यक्ति* condp) {
        File_पङ्क्ति* अचर fl = binp->fileline();
        यदि (binp->iffp()) condp = नूतन AstLogAnd{fl, binp->iffp()->clone_रूख(असत्य), condp};
        अचर स्वतः excluded = m_excludedVars.find(cpVarp);
        यदि (excluded != m_excludedVars.end() && !binp->transp()
            && (binp->binsType().binIsNormal()
                || binp->binsType() == VCoverBinsType::BINS_DEFAULT)) {
            condp = नूतन AstLogAnd{
                fl, नूतन AstNot{fl, नूतन AstVarRef{fl, excluded->second, VAccess::READ}}, condp};
        }
        फर्काऊ condp;
    }

    शून्य emitConvHitIf(AstCoverpoint* coverpointp, AstCoverBin* binp, AstVar* cpVarp,
                       Ast_गाँठो_अभिव्यक्ति* idxp, Ast_गाँठो_अभिव्यक्ति* condp) {
        File_पङ्क्ति* अचर fl = binp->fileline();
        Ast_गाँठो* actionp = makeRuntimeBinHit(fl, cpVarp, idxp, binp->binsType().binIsNormal());
        यदि (binp->binsType() == VCoverBinsType::BINS_ILLEGAL) {
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
                         Ast_गाँठो_अभिव्यक्ति* exprp, uint32_t sized, AstVar* matchedp) {
        File_पङ्क्ति* अचर fl = binp->fileline();
        Ast_गाँठो_अभिव्यक्ति* enabledp = binCondition(binp, cpVarp, नूतन AstConst{fl, AstConst::BitTrue{}});
        UASSERT_OBJ(m_sampleFuncp, binp, "sample() CFunc not set for coverpoint");
        अचर तार्किक illegal = binp->binsType() == VCoverBinsType::BINS_ILLEGAL;
        यदि (illegal) {
            // The illegal action reads the condition too, which is evaluated once, as the guard
            // may have side effects
            AstVar* अचर varp
                = नूतन AstVar{fl, VVarType::BLOCKTEMP,
                             "__VcpEnabled_" + sanitizeGeneratedName(coverpointp->name()) + "_"
                                 + cvtToStr(sized),
                             binp->findBitDType()};
            varp->funcLocal(सत्य);
            m_sampleFuncp->addStmtsp(varp);
            addSampleStmt(नूतन AstAssign{fl, नूतन AstVarRef{fl, varp, VAccess::WRITE}, enabledp});
            enabledp = नूतन AstVarRef{fl, varp, VAccess::READ};
        }
        AstCMethodHard* अचर callp
            = itemCall(fl, cpVarp,
                       exprp->isWide() ? VCMethod::COVERGROUP_SIZED_SAMPLE_W
                                       : VCMethod::COVERGROUP_SIZED_SAMPLE,
                       {cnum(fl, sized), exprp->clone_रूख(असत्य), enabledp});
        callp->dtypeSetBit();
        यदि (illegal) {
            addSampleStmt(नूतन AstIf{fl, नूतन AstLogAnd{fl, callp, enabledp->clone_रूख(असत्य)},
                                    makeIllegalBinAction(fl, "Illegal bin " + binp->prettyNameQ()
                                                                 + " hit in coverpoint "
                                                                 + coverpointp->prettyNameQ())});
        } अन्यथा यदि (matchedp && binp->binsType().binIsNormal()) {
            addSampleStmt(
                नूतन AstAssign{fl, नूतन AstVarRef{fl, matchedp, VAccess::WRITE},
                              नूतन AstOr{fl, नूतन AstVarRef{fl, matchedp, VAccess::READ}, callp}});
        } अन्यथा {
            addSampleStmt(callp->makeStmt());
        }
    }

    // A variable local to the constructor
    AstVar* constructorTemp(File_पङ्क्ति* fl, अचर string& name, Ast_गाँठो_DType* dtypep) {
        AstVar* अचर varp = नूतन AstVar{fl, VVarType::BLOCKTEMP, name, dtypep};
        varp->funcLocal(सत्य);
        m_constructorp->addStmtsp(varp);
        फर्काऊ varp;
    }

    // Truncate or extend, as its signedness sets, a value to a type
    स्थिर Ast_गाँठो_अभिव्यक्ति* resize_मान(Ast_गाँठो_अभिव्यक्ति* valuep, Ast_गाँठो_DType* dtypep) {
        File_पङ्क्ति* अचर fl = valuep->fileline();
        यदि (valuep->width() > dtypep->width()) {
            valuep = नूतन AstSel{fl, valuep, 0, dtypep->width()};
        } अन्यथा यदि (valuep->width() < dtypep->width()) {
            valuep = valuep->isSigned()
                         ? static_cast<Ast_गाँठो_अभिव्यक्ति*>(नूतन AstExtendS{fl, valuep, dtypep->width()})
                         : नूतन AstExtend{fl, valuep, dtypep->width()};
        }
        valuep->dtypep(dtypep);
        फर्काऊ valuep;
    }

    // The runtime arguments of a count, which the constructor evaluates once into a temporary:
    // its value, a count wider than 64 bits saturated over any limit, and whether it is positive
    std::pair<Ast_गाँठो_अभिव्यक्ति*, Ast_गाँठो_अभिव्यक्ति*> new_गन्ती_Args(File_पङ्क्ति* fl, अचर string& name,
                                                       Ast_गाँठो_अभिव्यक्ति* exprp) {
        AstVar* अचर countp = constructorTemp(fl, name, exprp->dtypep());
        m_constructorp->addStmtsp(
            नूतन AstAssign{fl, नूतन AstVarRef{fl, countp, VAccess::WRITE}, exprp->clone_रूख(असत्य)});
        अचर स्वतः गन्ती_Ref = [&]() { फर्काऊ नूतन AstVarRef{fl, countp, VAccess::READ}; };
        AstConst* अचर zerop = नूतन AstConst{fl, AstConst::DTyped{}, countp->dtypep()};
        Ast_गाँठो_अभिव्यक्ति* अचर positivep
            = countp->isSigned() ? static_cast<Ast_गाँठो_अभिव्यक्ति*>(नूतन AstGtS{fl, गन्ती_Ref(), zerop})
                                 : नूतन AstNeq{fl, गन्ती_Ref(), zerop};
        Ast_गाँठो_अभिव्यक्ति* valuep = resize_मान(गन्ती_Ref(), countp->findUInt64DType());
        यदि (countp->width() > VL_QUADSIZE) {
            valuep = नूतन AstCond{
                fl,
                नूतन AstRedOr{
                    fl, नूतन AstSel{fl, गन्ती_Ref(), VL_QUADSIZE, countp->width() - VL_QUADSIZE}},
                नूतन AstConst{fl, AstConst::Unsized64{}, std::numeric_limits<uint64_t>::max()},
                valuep};
            valuep->dtypeSetUInt64();
        }
        फर्काऊ {valuep, positivep};
    }
    // The runtime arguments of a constant count
    स्थिर std::pair<Ast_गाँठो_अभिव्यक्ति*, Ast_गाँठो_अभिव्यक्ति*> new_गन्ती_Args(File_पङ्क्ति* fl, uint64_t गन्ती) {
        फर्काऊ {नूतन AstConst{fl, AstConst::Unsized64{}, गन्ती},
                नूतन AstConst{fl, AstConst::BitTrue{}}};
    }

    // The values of a coverpoint, which its name denotes (IEEE 1800-2023 19.5.1.1), as runs in
    // value order: an enumerated type's values (6.19), else all values of its type.  Bounds
    // are at runWidth(), sign-extended like a CrossValueRange's.
    स्थिर std::vector<std::pair<V3_सङ्ख्या, V3_सङ्ख्या>> coverpoint_मानहरू(Ast_गाँठो* nodep,
                                                                       Ast_गाँठो_अभिव्यक्ति* exprp) {
        अचर पूर्णाङ्क width = runWidth(exprp);
        std::vector<std::pair<V3_सङ्ख्या, V3_सङ्ख्या>> runs;
        अचर AstEnumDType* अचर enump = VN_CAST(exprp->dtypep()->skipRefToEnump(), EnumDType);
        यदि (!enump) {
            अचर Cross_मान_Range domain
                = cross_मान_Domain(nodep, exprp->width(), exprp->isSigned(), width);
            runs.emplace_back(domain.lo, domain.hi);
            फर्काऊ runs;
        }
        std::vector<V3_सङ्ख्या> मानहरू;
        लागि (अचर AstEnumItem* itemp = enump->itemsp(); itemp;
             itemp = VN_AS(itemp->nextp(), EnumItem)) {
            अचर V3_सङ्ख्या& अङ्क = VN_AS(itemp->valuep(), Const)->अङ्क();
            यदि (अङ्क.isFourState()) जारी;  // Not a coverpoint value (19.5.7)
            मानहरू.emplace_back(nodep, width);
            यदि (exprp->isSigned()) {
                मानहरू.back().opExtendS(अङ्क, अङ्क.width());
            } अन्यथा {
                मानहरू.back().opAssign(अङ्क);
            }
        }
        std::sort(मानहरू.begin(), मानहरू.end(), cross_मान_Less);
        अचर V3_सङ्ख्या one{nodep, width, 1};
        लागि (अचर V3_सङ्ख्या& मान : मानहरू) {
            V3_सङ्ख्या next{nodep, width};
            यदि (!runs.empty() && next.opAdd(runs.back().second, one).isCaseEq(मान)) {
                runs.back().second = मान;
            } अन्यथा {
                runs.emplace_back(मान, मान);
            }
        }
        फर्काऊ runs;
    }

    // Emit the constructor code building the bins 'binp' whose values it computes: the values of
    // each element that are coverpoint values (IEEE 1800-2023 19.5.7), those a 'with' filter
    // keeps (19.5.1.1), then its bins
    शून्य generateConstructedBins(AstCoverpoint* coverpointp, AstCoverBin* binp, AstVar* cpVarp,
                                 Ast_गाँठो_अभिव्यक्ति* exprp) {
        File_पङ्क्ति* अचर fl = binp->fileline();
        अचर string prefix
            = m_sizedNames.get(sanitizeGeneratedName(coverpointp->name() + "__" + binp->name()));
        // The count, and whether it is positive; one of a filter's scalar bin, or bin per value
        अचर std::pair<Ast_गाँठो_अभिव्यक्ति*, Ast_गाँठो_अभिव्यक्ति*> गन्ती
            = binp->arraySizep() ? new_गन्ती_Args(fl, prefix + "_count", binp->arraySizep())
                                 : new_गन्ती_Args(fl, 1);
        AstCoverWith* अचर withp = binWith(binp);
        यदि (withp && !binRangesp(binp)) {  // The coverpoint's name: all of its values
            लागि (अचर std::pair<V3_सङ्ख्या, V3_सङ्ख्या>& run : coverpoint_मानहरू(binp, exprp)) {
                m_constructorp->addStmtsp(itemCall(fl, cpVarp,
                                                   exprp->isWide()
                                                       ? VCMethod::COVERGROUP_SIZED_RANGE_W
                                                       : VCMethod::COVERGROUP_SIZED_RANGE,
                                                   {new_मान_Const(fl, run.first, exprp),
                                                    new_मान_Const(fl, run.second, exprp)})
                                              ->makeStmt());
            }
        }
        uint32_t element = 0;
        लागि (Ast_गाँठो* rangep = binRangesp(binp); rangep; rangep = rangep->nextp()) {
            यदि (VN_IS(rangep, Unbounded)) {  // A parameter of '$'
                binp->v3error("Bins value may not be '$', which may only bound a range "
                              "(IEEE 1800-2023 6.20.7)");
                जारी;
            }
            generateSizedElement(cpVarp, binp, rangep, exprp, prefix + "_" + cvtToStr(element++));
        }
        यदि (withp) generateWithFilter(binp, withp, cpVarp, exprp, prefix);
        std::vector<Ast_गाँठो_अभिव्यक्ति*> args{ctext(fl, binp->binsType().binSetEnum()), गन्ती.first,
                                       गन्ती.second};
        // A filter's bins have the limit it began with
        यदि (!withp) args.थप_back(cnum(fl, v3Global.opt.coverageMaxBins()));
        अचर तार्किक prot = v3Global.opt.protectIds();
        args.थप_back(ctext(fl, quoted(VIdProtect::protect_शब्दहरू_If(binp->name(), prot))));
        args.थप_back(ctext(fl, quoted(VIdProtect::protectIf(fl->filename(), prot))));
        args.थप_back(cnum(fl, static_cast<uint32_t>(fl->lineno())));
        args.थप_back(cnum(fl, static_cast<uint32_t>(fl->firstColumn())));
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
                            Ast_गाँठो_अभिव्यक्ति* exprp, अचर string& prefix) {
        File_पङ्क्ति* अचर fl = withp->fileline();
        अचर तार्किक wide = exprp->isWide();
        अचर string grouping = !binp->isArray()     ? "Single"
                                : binp->arraySizep() ? "Fixed"
                                                     : "Values";
        m_constructorp->addStmtsp(itemCall(fl, cpVarp, VCMethod::COVERGROUP_WITH_BEGIN,
                                           {ctext(fl, "VlCovBinGrouping::" + grouping),
                                            cnum(fl, v3Global.opt.coverageMaxBins())})
                                      ->makeStmt());
        // The candidates count in the coverpoint's width, and the filter reads each as 'item',
        // of the coverpoint's type, so that a filter changing 'item' cannot change the loop
        Ast_गाँठो_DType* अचर मान_DTypep = exprp->dtypep();
        AstVar* अचर valuep = constructorTemp(fl, prefix + "_value", मान_DTypep);
        AstVar* अचर lastp = constructorTemp(fl, prefix + "_last", मान_DTypep);
        AstVar* अचर firstp = constructorTemp(fl, prefix + "_first", मान_DTypep);
        AstVar* अचर runp = constructorTemp(fl, prefix + "_run", binp->findBitDType());
        AstVar* अचर morep = constructorTemp(fl, prefix + "_more", binp->findBitDType());
        AstVar* अचर itemp = withp->itemp()->unlinkFrBack();
        itemp->name(prefix + "_item");
        m_constructorp->addStmtsp(itemp);
        अचर स्वतः ref = [&](AstVar* varp) { फर्काऊ नूतन AstVarRef{fl, varp, VAccess::READ}; };
        अचर स्वतः assign = [&](AstVar* varp, Ast_गाँठो_अभिव्यक्ति* rhsp) -> Ast_गाँठो* {
            फर्काऊ नूतन AstAssign{fl, नूतन AstVarRef{fl, varp, VAccess::WRITE}, rhsp};
        };
        अचर स्वतः flag = [&](AstVar* varp, तार्किक मान) {
            फर्काऊ assign(varp, मान ? नूतन AstConst{fl, AstConst::BitTrue{}}
                                      : नूतन AstConst{fl, AstConst::BitFalse{}});
        };
        अचर स्वतः bound = [&](VCMethod narrow, VCMethod wideMethod, AstVar* varp) -> Ast_गाँठो* {
            यदि (wide) {
                फर्काऊ itemCall(fl, cpVarp, wideMethod, {नूतन AstVarRef{fl, varp, VAccess::WRITE}})
                    ->makeStmt();
            }
            AstCMethodHard* अचर callp = itemCall(fl, cpVarp, narrow);
            callp->dtypeSetUInt64();
            फर्काऊ assign(varp, resize_मान(callp, मान_DTypep));
        };
        अचर स्वतः keep = [&](Ast_गाँठो_अभिव्यक्ति* lop, Ast_गाँठो_अभिव्यक्ति* hip) {
            AstCMethodHard* अचर callp = itemCall(
                fl, cpVarp, wide ? VCMethod::COVERGROUP_WITH_RUN_W : VCMethod::COVERGROUP_WITH_RUN,
                {lop, hip});
            callp->dtypeSetBit();
            फर्काऊ assign(morep, callp);
        };
        अचर स्वतः step = [&](तार्किक up) {
            AstConst* अचर onep = नूतन AstConst{fl, AstConst::Widthed_मान{}, exprp->width(), 1};
            Ast_गाँठो_अभिव्यक्ति* अचर stepp
                = up ? static_cast<Ast_गाँठो_अभिव्यक्ति*>(नूतन AstAdd{fl, ref(valuep), onep})
                     : नूतन AstSub{fl, ref(valuep), onep};
            stepp->dtypep(मान_DTypep);
            फर्काऊ stepp;
        };
        AstLoop* अचर innerp = नूतन AstLoop{fl};
        innerp->addStmtsp(assign(itemp, ref(valuep)));
        innerp->addStmtsp(नूतन AstIf{
            fl, withp->filterp()->unlinkFrBack(),
            नूतन AstIf{fl, नूतन AstNot{fl, ref(runp)},
                      assign(firstp, ref(valuep))->addNext(flag(runp, सत्य))},
            नूतन AstIf{fl, ref(runp), keep(ref(firstp), step(असत्य))->addNext(flag(runp, असत्य))}});
        innerp->addStmtsp(नूतन AstLoopTest{
            fl, innerp, नूतन AstLogAnd{fl, ref(morep), नूतन AstNeq{fl, ref(valuep), ref(lastp)}}});
        innerp->addStmtsp(assign(valuep, step(सत्य)));
        AstLoop* अचर outerp = नूतन AstLoop{fl};
        AstCMethodHard* अचर nextp = itemCall(fl, cpVarp, VCMethod::COVERGROUP_WITH_NEXT);
        nextp->dtypeSetBit();
        outerp->addStmtsp(नूतन AstLoopTest{fl, outerp, nextp});
        outerp->addStmtsp(
            bound(VCMethod::COVERGROUP_WITH_LO, VCMethod::COVERGROUP_WITH_LO_W, valuep));
        outerp->addStmtsp(
            bound(VCMethod::COVERGROUP_WITH_HI, VCMethod::COVERGROUP_WITH_HI_W, lastp));
        outerp->addStmtsp(flag(runp, असत्य));
        outerp->addStmtsp(innerp);
        outerp->addStmtsp(नूतन AstIf{fl, ref(runp), keep(ref(firstp), ref(lastp))});
        m_constructorp->addStmtsp(flag(morep, सत्य));
        m_constructorp->addStmtsp(outerp);
    }

    // Emit 'sizedRange(lo, hi)' for the coverpoint values of an element of bins 'binp', whose
    // values the constructor computes: resolved now if constant, else when constructed by
    // clipping to the coverpoint's values.  A wildcard pattern's values give a range for each
    // run of them.  'prefix' names its temporaries.
    शून्य generateSizedElement(AstVar* cpVarp, अचर AstCoverBin* binp, Ast_गाँठो* rangep,
                              Ast_गाँठो_अभिव्यक्ति* exprp, अचर string& prefix) {
        File_पङ्क्ति* अचर fl = rangep->fileline();
        अचर VCMethod method = exprp->isWide() ? VCMethod::COVERGROUP_SIZED_RANGE_W
                                                : VCMethod::COVERGROUP_SIZED_RANGE;
        अचर AstInsideRange* अचर irp = VN_CAST(rangep, InsideRange);
        Ast_गाँठो_अभिव्यक्ति* अचर lowp = irp ? irp->lhsp() : VN_AS(rangep, गाँठो_अभिव्यक्ति);
        Ast_गाँठो_अभिव्यक्ति* अचर highp = irp ? irp->rhsp() : nullptr;
        अचर स्वतः unbounded
            = [](अचर Ast_गाँठो_अभिव्यक्ति* boundp) { फर्काऊ !boundp || VN_IS(boundp, Unbounded); };
        अचर स्वतः constant
            = [&](अचर Ast_गाँठो_अभिव्यक्ति* boundp) { फर्काऊ unbounded(boundp) || VN_IS(boundp, Const); };
        अचर स्वतः integral = [&](अचर Ast_गाँठो_अभिव्यक्ति* boundp) {
            फर्काऊ unbounded(boundp) || boundp->dtypep()->skipRefp()->isIntegralOrPacked();
        };
        यदि (constant(lowp) && constant(highp)) {
            अचर स्वतः fourState = [](अचर Ast_गाँठो_अभिव्यक्ति* boundp) {
                अचर AstConst* अचर constp = VN_CAST(boundp, Const);
                फर्काऊ constp && constp->अङ्क().isFourState();
            };
            यदि (irp && (fourState(lowp) || fourState(highp))) {
                rangep->v3error("Four-state (x/z) value in "
                                << (binp->isArray() ? "array bins" : "bin")
                                << " range bound; range bounds must be two-state constants");
                फर्काऊ;
            }
            Cross_मान_Range range{rangep, resolveWidth(rangep, exprp)};
            यदि (!resolve_मान(rangep, exprp, सत्य, binp->isWildcard(), range)) {
                rangep->v3warn(E_UNSUPPORTED, "Unsupported: non-integral value in a coverage bin "
                                              "of an integral coverpoint.");
            } अन्यथा यदि (!crossRangeEmpty(range)) {
                std::vector<std::pair<V3_सङ्ख्या, V3_सङ्ख्या>> runs{{range.lo, range.hi}};
                यदि (range.wildcard) {  // checkConstructedBins bounded the runs
                    runs.clear();
                    crossRangeRuns(range, v3Global.opt.coverageMaxBins(), runs);
                }
                लागि (अचर std::pair<V3_सङ्ख्या, V3_सङ्ख्या>& run : runs) {
                    m_constructorp->addStmtsp(itemCall(fl, cpVarp, method,
                                                       {new_मान_Const(fl, run.first, exprp),
                                                        new_मान_Const(fl, run.second, exprp)})
                                                  ->makeStmt());
                }
            }
            फर्काऊ;
        }
        यदि (!integral(lowp) || !integral(highp)) {
            rangep->v3warn(E_UNSUPPORTED, "Unsupported: non-integral value in a coverage bin "
                                          "of an integral coverpoint.");
            फर्काऊ;
        }
        // Compare values and the coverpoint's domain signed, in a width holding all of them
        अचर स्वतः boundWidth
            = [&](अचर Ast_गाँठो_अभिव्यक्ति* boundp) { फर्काऊ unbounded(boundp) ? 0 : boundp->width(); };
        अचर पूर्णाङ्क width = std::max({exprp->width(), boundWidth(lowp), boundWidth(highp)}) + 1;
        अचर Cross_मान_Range domain
            = cross_मान_Domain(rangep, exprp->width(), exprp->isSigned(), width);
        AstVar* अचर lop = constructorTemp(fl, prefix + "_lo", exprp->dtypep());
        lop->dtypeSetBitSized(width, VSigning::SIGNED);
        AstVar* अचर hip = constructorTemp(fl, prefix + "_hi", lop->dtypep());
        अचर स्वतः bound = [&](Ast_गाँठो_अभिव्यक्ति* boundp, अचर V3_सङ्ख्या& limit) -> Ast_गाँठो_अभिव्यक्ति* {
            यदि (unbounded(boundp)) {
                AstConst* अचर limitp = नूतन AstConst{fl, limit};
                limitp->dtypeFrom(lop);
                फर्काऊ limitp;
            }
            Ast_गाँठो_अभिव्यक्ति* valuep = boundp->clone_रूख(असत्य);
            // A value no wider than a signed coverpoint has its type (see crossRangeBound)
            यदि (exprp->isSigned() && boundp->width() <= exprp->width()) {
                valuep = resize_मान(valuep, exprp->dtypep());
            }
            फर्काऊ resize_मान(valuep, lop->dtypep());
        };
        अचर स्वतः ref = [&](AstVar* varp, VAccess access = VAccess::READ) {
            फर्काऊ नूतन AstVarRef{fl, varp, access};
        };
        m_constructorp->addStmtsp(
            नूतन AstAssign{fl, ref(lop, VAccess::WRITE), bound(lowp, domain.lo)});
        m_constructorp->addStmtsp(
            नूतन AstAssign{fl, ref(hip, VAccess::WRITE), irp ? bound(highp, domain.hi) : ref(lop)});
        AstConst* अचर minp = नूतन AstConst{fl, domain.lo};
        AstConst* अचर maxp = नूतन AstConst{fl, domain.hi};
        minp->dtypeFrom(lop);
        maxp->dtypeFrom(lop);
        AstCond* अचर lowerp
            = नूतन AstCond{fl, नूतन AstLtS{fl, ref(lop), minp}, minp->clone_रूख(असत्य), ref(lop)};
        AstCond* अचर upperp
            = नूतन AstCond{fl, नूतन AstGtS{fl, ref(hip), maxp}, maxp->clone_रूख(असत्य), ref(hip)};
        lowerp->dtypeFrom(lop);
        upperp->dtypeFrom(lop);
        m_constructorp->addStmtsp(नूतन AstAssign{fl, ref(lop, VAccess::WRITE), lowerp});
        m_constructorp->addStmtsp(नूतन AstAssign{fl, ref(hip, VAccess::WRITE), upperp});
        m_constructorp->addStmtsp(नूतन AstIf{fl, नूतन AstLteS{fl, ref(lop), ref(hip)},
                                            itemCall(fl, cpVarp, method,
                                                     {resize_मान(ref(lop), exprp->dtypep()),
                                                      resize_मान(ref(hip), exprp->dtypep())})
                                                ->makeStmt()});
    }

    // The runtime index of the bin of a run holding the coverpoint value, which is in the run:
    // declared + (value - lo) / stride, capped at the last bin, which holds any remainder.
    स्थिर Ast_गाँठो_अभिव्यक्ति* runBinIndex(File_पङ्क्ति* fl, Ast_गाँठो_अभिव्यक्ति* exprp, अचर BinRun& run) {
        यदि (run.m_गन्ती == 1) फर्काऊ cnum(fl, run.m_declared);
        अचर पूर्णाङ्क width = exprp->width();
        // A run spans at most 2^width values, so offsets in it are unsigned width-bit numbers
        Ast_गाँठो_अभिव्यक्ति* indexp
            = नूतन AstSub{fl, exprp->clone_रूख(असत्य), new_मान_Const(fl, run.m_lo, exprp)};
        indexp->dtypeSetBitSized(width, VSigning::UNSIGNED);
        V3_सङ्ख्या stride{fl, width, 0};
        stride.opAssign(run.m_stride);
        यदि (stride.गन्ती_Ones() != 1) {
            indexp = नूतन AstDiv{fl, indexp, नूतन AstConst{fl, stride}};
        } अन्यथा यदि (!stride.isEqOne()) {
            indexp = नूतन AstShiftR{fl, indexp, नूतन AstConst{fl, stride.mostSetBitP1() - 1}};
        }
        // Compare the run's values with those of count bins of stride values, without overflow
        अचर पूर्णाङ्क extWidth = run.m_lo.width() + 1;
        V3_सङ्ख्या lo{fl, extWidth, 0};
        lo.opExtendS(run.m_lo, run.m_lo.width());
        V3_सङ्ख्या span{fl, extWidth, 0};
        span.opExtendS(run.m_hi, run.m_hi.width());
        span.opSub(V3_सङ्ख्या{span}, lo);
        V3_सङ्ख्या covered{fl, extWidth, 0};
        covered.opAssign(run.m_stride);
        covered.opMul(V3_सङ्ख्या{covered}, V3_सङ्ख्या{fl, extWidth, run.m_गन्ती});
        V3_सङ्ख्या hasRemainder{fl, 1, 0};
        यदि (!hasRemainder.opGte(span, covered).isEqZero()) {
            AstConst* अचर lastp = नूतन AstConst{fl, V3_सङ्ख्या{fl, width, run.m_गन्ती - 1}};
            indexp = नूतन AstCond{fl, नूतन AstGt{fl, indexp->clone_रूख(असत्य), lastp},
                                 lastp->clone_रूख(असत्य), indexp};
        }
        यदि (width < VL_IDATASIZE) {
            indexp = नूतन AstExtend{fl, indexp, VL_IDATASIZE};
        } अन्यथा यदि (width > VL_IDATASIZE) {
            indexp = नूतन AstSel{fl, indexp, 0, VL_IDATASIZE};
        }
        फर्काऊ नूतन AstAdd{fl, cnum(fl, run.m_declared), indexp};
    }

    // Emit the sample() hit of a run of bins, whose code does not grow with its number of bins:
    //   if (iff && lo <= value && value <= hi) m_cp.incrementBin(<runBinIndex>);
    शून्य emitRunHit(AstCoverpoint* coverpointp, AstCoverBin* binp, AstVar* cpVarp,
                    Ast_गाँठो_अभिव्यक्ति* exprp, अचर BinRun& run) {
        File_पङ्क्ति* अचर fl = binp->fileline();
        AstConst* अचर lop = new_मान_Const(fl, run.m_lo, exprp);
        Ast_गाँठो_अभिव्यक्ति* condp = nullptr;
        यदि (run.m_lo.isCaseEq(run.m_hi)) {
            condp = नूतन AstEq{fl, exprp->clone_रूख(असत्य), lop};
        } अन्यथा {
            AstConst* अचर hip = new_मान_Const(fl, run.m_hi, exprp);
            condp = makeRangeCondition(fl, exprp, lop, hip);
            VL_DO_DANGLING(थप_Deletep(lop), lop);
            VL_DO_DANGLING(थप_Deletep(hip), hip);
        }
        emitConvHitIf(coverpointp, binp, cpVarp, runBinIndex(fl, exprp, run), condp);
    }

    // Emit a transition bin's hit action into sample():
    //   if (cond) { m_cp.incrementBin/recordHit(idx); [illegal: $error; $stop] }
    // Used by the transition generators so a completed sequence records into the runtime bin.
    शून्य addConvTransHitIf(AstCoverpoint* coverpointp, AstCoverBin* binp, अचर ConvBinTarget& tgt,
                           Ast_गाँठो_अभिव्यक्ति* condp) {
        File_पङ्क्ति* अचर fl = binp->fileline();
        Ast_गाँठो* actionp = makeRuntimeBinHit(fl, tgt);
        यदि (binp->binsType() == VCoverBinsType::BINS_ILLEGAL) {
            actionp->addNext(makeIllegalBinAction(
                fl, "Illegal transition bin " + binp->prettyNameQ() + " hit in coverpoint "
                        + coverpointp->prettyNameQ()));
        }
        UASSERT_OBJ(m_sampleFuncp, binp, "sample() CFunc not set for transition bin");
        addSampleStmt(नूतन AstIf{fl, condp, actionp, nullptr});
    }

    // Route a coverpoint through a VlCoverpoint member: emit the member, its sample()
    // increments, the constructor configuration (init + namers), and registration.
    शून्य generateCoverpoint(AstCoverpoint* coverpointp, Ast_गाँठो_अभिव्यक्ति* exprp, पूर्णाङ्क at_Least_मान) {
        File_पङ्क्ति* अचर fl = coverpointp->fileline();
        अचर तार्किक dynamic = m_runtimePoints.गन्ती(coverpointp);
        UINFO(4, "  Generating VlCoverpoint member: " << coverpointp->name());

        // Size the hit list to the gen-time max bin overlap (1 unless cross-fed with
        // overlapping ranges), so no cross hit is ever dropped and storage is minimal.
        अचर तार्किक crossFed = m_crossedCpNames.गन्ती(coverpointp->name()) != 0;
        अचर पूर्णाङ्क hitBound = computeHitListBound(coverpointp, exprp, crossFed);
        UINFO(6, "    Hit-list bound (max bin overlap) = " << hitBound);
        AstVar* अचर cpVarp = नूतन AstVar{fl, VVarType::MEMBER, "__Vcp_" + coverpointp->name(),
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
        यदि (crossFed) {
            UASSERT_OBJ(m_sampleFuncp, coverpointp, "sample() CFunc not set for clearHitList");
            m_sampleFuncp->addStmtsp(
                itemCall(fl, cpVarp, VCMethod::COVERGROUP_CLEAR_HIT_LIST)->makeStmt());
        }
        यदि (dynamic && coverpointHasStateExclusions(coverpointp)) {
            AstVar* अचर excludedp
                = नूतन AstVar{fl, VVarType::BLOCKTEMP,
                             "__VcpExcluded_" + sanitizeGeneratedName(coverpointp->name()),
                             coverpointp->findBitDType()};
            excludedp->funcLocal(सत्य);
            m_sampleFuncp->addStmtsp(excludedp);
            AstCMethodHard* अचर callp
                = itemCall(fl, cpVarp,
                           exprp->isWide() ? VCMethod::COVERGROUP_मान_EXCLUDED_W
                                           : VCMethod::COVERGROUP_मान_EXCLUDED,
                           {exprp->clone_रूख(असत्य)});
            callp->dtypeSetBit();
            addSampleStmt(नूतन AstAssign{fl, नूतन AstVarRef{fl, excludedp, VAccess::WRITE}, callp});
            m_excludedVars.emplace(cpVarp, excludedp);
        }

        // Walk bins (non-default, then default), assigning sequential indices that match the
        // namer append order; emit sample increments and collect namer statements.  Constructed
        // bins follow them all, placed when the coverpoint is constructed.
        std::vector<Ast_गाँठो_Stmt*> namerStmts;
        std::vector<AstCoverBin*> defaultBins;
        std::vector<AstCoverBin*> sizedBins;
        std::vector<std::tuple<AstCoverBin*, uint32_t, Ast_गाँठो_अभिव्यक्ति*>> metadata;
        std::vector<अचर BinRun*> runMetadata;
        uint64_t idx = 0;  // Runtime index of the next bin; 32-bit once checked below
        लागि (Ast_गाँठो* binp = coverpointp->binsp(); binp; binp = binp->nextp()) {
            AstCoverBin* अचर cbinp = VN_AS(binp, CoverBin);
            अचर पूर्णाङ्क errorsBefore = dynamic ? V3Error::error_गन्ती() : 0;
            यदि (cbinp->binsType() == VCoverBinsType::BINS_DEFAULT) {
                defaultBins.थप_back(cbinp);
                जारी;
            }
            यदि (isConstructedBins(cbinp)) {
                UASSERT_OBJ(dynamic, cbinp, "Constructed bins without value metadata");
                BinSpan span;
                span.sized = static_cast<int32_t>(sizedBins.size());
                m_cpBins.at(cpVarp).spans.emplace(cbinp->name(), span);
                sizedBins.थप_back(cbinp);
                जारी;
            }
            यदि (cbinp->transp()) {
                // Transition bin (incl. array transition 'bins t[] = (a=>b),(c=>d)' and
                // illegal_bins/ignore_bins transitions).  All sequences of one transition bin
                // share a bin name and merge in the coverage DB to a single point, so model
                // them as one runtime bin incremented by any matching sequence.  The sequence
                // matching is generated as a state machine, with the hit routed to this bin's
                // runtime slot.
                namerStmts.थप_back(makeNamer(cpVarp, cbinp, -1, static_cast<uint32_t>(idx)));
                अचर ConvBinTarget tgt{cpVarp, static_cast<uint32_t>(idx),
                                        cbinp->binsType().binIsNormal()};
                लागि (Ast_गाँठो* sp = cbinp->transp(); sp; sp = sp->nextp())
                    generateSingleTransitionCode(coverpointp, cbinp, exprp, tgt,
                                                 VN_AS(sp, CoverTransSet));
                यदि (dynamic && V3Error::error_गन्ती() == errorsBefore) {
                    metadata.emplace_back(cbinp, static_cast<uint32_t>(idx), nullptr);
                }
                ++idx;
                जारी;
            }
            BinRuns plan;
            यदि (binRunsFor(cbinp, exprp, plan)) {
                // Array elements and automatic bins generate as runs, so neither sample() nor
                // the constructor grows with their number of bins.
                यदि (plan.unsupported) {  // bin ignored or invalid; reserve no slot
                    m_droppedBins[coverpointp].थप_back(cbinp->name());
                    जारी;
                }
                CoverpointBins& bins = m_cpBins.at(cpVarp);
                अचर uint32_t first_मान = bins.total;
                अचर uint32_t firstDeclared = static_cast<uint32_t>(idx);
                namerStmts.थप_back(
                    makeNamer(cpVarp, cbinp, plan.गन्ती, firstDeclared, {}, plan.मानहरू));
                लागि (BinRun& run : plan.runs) {
                    run.m_declared = static_cast<uint32_t>(idx);
                    bins.runs.थप_back(std::move(run));
                    अचर BinRun& stored = bins.runs.back();
                    यदि (bins.crossed && cbinp->binsType().binIsNormal()) {
                        अचर uint32_t first = first_मान + stored.m_declared - firstDeclared;
                        लागि (uint32_t element = 0; element < stored.m_गन्ती; ++element) {
                            bins.मानहरू[first + element].runp = &stored;
                            bins.मानहरू[first + element].element = element;
                        }
                    }
                    यदि (!stored.m_empty) {
                        emitRunHit(coverpointp, cbinp, cpVarp, exprp, stored);
                        यदि (dynamic && V3Error::error_गन्ती() == errorsBefore) {
                            runMetadata.थप_back(&stored);
                        }
                    }
                    idx += stored.m_गन्ती;
                }
                जारी;
            }
            यदि (cbinp->isArray()) {  // value array of a real coverpoint: b[0]..b[N-1]
                // Only integral coverpoints have runtime value metadata (m_runtimePoints)
                UASSERT_OBJ(!dynamic, cbinp, "Runtime value metadata for a real coverpoint");
                तार्किक unsupported = असत्य;
                std::vector<Ast_गाँठो_अभिव्यक्ति*> मानहरू = extract_Array_मानहरू(cbinp, exprp, unsupported);
                यदि (unsupported) {  // bin ignored (COVERIGN emitted); reserve no slot
                    m_droppedBins[coverpointp].थप_back(cbinp->name());
                    जारी;
                }
                namerStmts.थप_back(makeNamer(cpVarp, cbinp, static_cast<int64_t>(मानहरू.size()),
                                               static_cast<uint32_t>(idx), मानहरू));
                लागि (Ast_गाँठो_अभिव्यक्ति* valuep : मानहरू) {
                    // The cross selections of this covergroup still read the value.
                    m_detached_मानहरू.थप_back(valuep);
                    emitConvHitIf(coverpointp, cbinp, cpVarp,
                                  cnum(cbinp->fileline(), static_cast<uint32_t>(idx)),
                                  build_मान_Condition(cbinp, exprp, valuep));
                    ++idx;
                }
            } अन्यथा {
                namerStmts.थप_back(makeNamer(cpVarp, cbinp, -1, static_cast<uint32_t>(idx)));
                // buildBinCondition is null for 'ignore_bins = default' (no ranges); the bin
                // still gets a reserved slot (recorded, never incremented).
                यदि (Ast_गाँठो_अभिव्यक्ति* अचर condp = buildBinCondition(cbinp, exprp))
                    emitConvHitIf(coverpointp, cbinp, cpVarp,
                                  cnum(cbinp->fileline(), static_cast<uint32_t>(idx)), condp);
                यदि (dynamic && V3Error::error_गन्ती() == errorsBefore) {
                    metadata.emplace_back(cbinp, static_cast<uint32_t>(idx), nullptr);
                }
                ++idx;
            }
        }
        // A cross selecting an ignored bins declaration selects no bins
        लागि (अचर std::string& name : m_droppedBins[coverpointp]) {
            m_cpBins.at(cpVarp).spans.emplace(name, BinSpan{});
        }
        // A value a constructed bin holds is no default bin's; only sampling tells which do
        AstVar* sizedMatchedp = nullptr;
        यदि (!defaultBins.empty()
            && std::any_of(sizedBins.begin(), sizedBins.end(), [](अचर AstCoverBin* binp) {
                   फर्काऊ binp->binsType().binIsNormal();
               })) {
            sizedMatchedp = नूतन AstVar{fl, VVarType::BLOCKTEMP,
                                       "__VcpSized_" + sanitizeGeneratedName(coverpointp->name()),
                                       coverpointp->findBitDType()};
            sizedMatchedp->funcLocal(सत्य);
            m_sampleFuncp->addStmtsp(sizedMatchedp);
            addSampleStmt(नूतन AstAssign{fl, नूतन AstVarRef{fl, sizedMatchedp, VAccess::WRITE},
                                        नूतन AstConst{fl, AstConst::BitFalse{}}});
        }
        लागि (uint32_t sized = 0; sized < sizedBins.size(); ++sized) {
            emitSizedSample(coverpointp, sizedBins[sized], cpVarp, exprp, sized, sizedMatchedp);
        }
        लागि (AstCoverBin* अचर defBinp : defaultBins) {
            File_पङ्क्ति* अचर dfl = defBinp->fileline();
            namerStmts.थप_back(makeNamer(cpVarp, defBinp, -1, static_cast<uint32_t>(idx)));
            Ast_गाँठो_अभिव्यक्ति* condp = buildDefaultCondition(coverpointp, exprp, dfl);
            यदि (sizedMatchedp) {
                condp = नूतन AstLogAnd{
                    dfl, नूतन AstNot{dfl, नूतन AstVarRef{dfl, sizedMatchedp, VAccess::READ}}, condp};
            }
            emitConvHitIf(coverpointp, defBinp, cpVarp, cnum(dfl, static_cast<uint32_t>(idx)),
                          condp);
            ++idx;
        }
        यदि (idx > std::numeric_limits<uint32_t>::max()) {
            // The runtime indexes bins with 32 bits; stop before generating a model
            coverpointp->v3warn(E_UNSUPPORTED, "Unsupported: coverpoint with more than "
                                                   << std::numeric_limits<uint32_t>::max()
                                                   << " bins");
        }

        // Transition coverpoints track the previous sampled value; update it once at the end of
        // this coverpoint's sample() contribution (the prev var was created on demand by the
        // transition matching above).
        यदि (coverpointHasTransition(coverpointp)) {
            AstVar* अचर prevVarp = VN_AS(coverpointp->user1p(), Var);
            addSampleStmt(
                नूतन AstAssign{coverpointp->fileline(),
                              नूतन AstVarRef{prevVarp->fileline(), prevVarp, VAccess::WRITE},
                              exprp->clone_रूख(असत्य)});
        }

        // Constructor: init (allocates), namers, then registration (under --coverage).
        // Under --protect-ids the hierarchy and page string reach the coverage database
        // verbatim, so obfuscate them like line/toggle points (per-word hierarchy, whole-
        // unit page).  No-ops when --protect-ids is off.
        अचर तार्किक prot = v3Global.opt.protectIds();
        अचर std::string hier
            = VIdProtect::protect_शब्दहरू_If(m_covergroupName + "." + coverpointp->name(), prot);
        m_constructorp->addStmtsp(
            itemCall(fl, cpVarp, VCMethod::COVERGROUP_INIT,
                     {ctext(fl, quoted(hier)), cnum(fl, static_cast<uint32_t>(at_Least_मान)),
                      cnum(fl, static_cast<uint32_t>(idx))})
                ->makeStmt());
        लागि (Ast_गाँठो_Stmt* अचर ns : namerStmts) m_constructorp->addStmtsp(ns);
        यदि (dynamic) {
            m_constructorp->addStmtsp(
                itemCall(fl, cpVarp, VCMethod::COVERGROUP_मान_TYPE,
                         {cnum(fl, exprp->width()), cnum(fl, exprp->isSigned())})
                    ->makeStmt());
            मान_Lists lists;
            लागि (अचर स्वतः& entry : metadata) {
                collect_मान_Metadata(lists, exprp, std::get<0>(entry), std::get<1>(entry),
                                     std::get<2>(entry));
            }
            लागि (अचर BinRun* अचर runp : runMetadata) collectRunMetadata(lists, exprp, *runp);
            emit_मान_List(fl, cpVarp, VCMethod::COVERGROUP_मान_RANGES, lists.m_ranges);
            emit_मान_List(fl, cpVarp, VCMethod::COVERGROUP_मान_RUNS, lists.m_runs);
            emit_मान_List(fl, cpVarp, VCMethod::COVERGROUP_मान_PATTERNS, lists.m_patterns);
            emit_मान_List(fl, cpVarp, VCMethod::COVERGROUP_मान_TRANSITIONS, lists.m_transitions);
            लागि (AstCoverBin* अचर binp : sizedBins) {
                generateConstructedBins(coverpointp, binp, cpVarp, exprp);
            }
            m_constructorp->addStmtsp(
                itemCall(fl, cpVarp, VCMethod::COVERGROUP_मान_FINALIZE)->makeStmt());
        }
        यदि (v3Global.opt.coverage()) {
            अचर std::string page
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
    शून्य generate_Multi_मान_Transition_Code(AstCoverpoint* coverpointp, AstCoverBin* binp,
                                          Ast_गाँठो_अभिव्यक्ति* exprp, अचर ConvBinTarget& tgt,
                                          अचर std::vector<AstCoverTransItem*>& वस्तुहरू) {
        UINFO(4, "    Generating multi-value transition state machine for: " << binp->name());
        UINFO(4, "      Sequence length: " << वस्तुहरू.size() << " items");

        // Create state position variable
        AstVar* अचर stateVarp = createSequenceStateVar(coverpointp, binp);

        // Build case statement with N cases (one for each state 0 to N-1)
        // State 0: Not started, looking for first item
        // State 1 to N-1: In progress, looking for next item

        AstCase* अचर casep
            = नूतन AstCase{binp->fileline(), VCaseType::CT_CASE,
                          नूतन AstVarRef{stateVarp->fileline(), stateVarp, VAccess::READ}, nullptr};

        // Generate each case item in the switch statement
        लागि (माप_प्रकार state = 0; state < वस्तुहरू.size(); ++state) {
            AstCaseItem* caseItemp = generateTransitionStateCase(coverpointp, binp, exprp, tgt,
                                                                 stateVarp, वस्तुहरू, state);
            casep->addItemsp(caseItemp);
        }

        // Add default case (reset to state 0) to prevent CASEINCOMPLETE warnings,
        // since the state variable is wider than the number of valid states.
        AstCaseItem* अचर defaultItemp = नूतन AstCaseItem{
            binp->fileline(), nullptr,
            नूतन AstAssign{binp->fileline(),
                          नूतन AstVarRef{binp->fileline(), stateVarp, VAccess::WRITE},
                          नूतन AstConst{binp->fileline(), AstConst::Widthed_मान{}, 8, 0}}};
        casep->addItemsp(defaultItemp);

        addSampleStmt(casep);
        UINFO(4, "      Successfully added multi-value transition state machine");
    }

    // Generate code for a single state in the transition state machine
    // Returns the case item for this state
    AstCaseItem* generateTransitionStateCase(AstCoverpoint* coverpointp, AstCoverBin* binp,
                                             Ast_गाँठो_अभिव्यक्ति* exprp, अचर ConvBinTarget& tgt,
                                             AstVar* stateVarp,
                                             अचर std::vector<AstCoverTransItem*>& वस्तुहरू,
                                             माप_प्रकार state) {
        File_पङ्क्ति* अचर fl = binp->fileline();

        // Build condition for current value matching expected item at this state
        Ast_गाँठो_अभिव्यक्ति* अचर matchCondp = buildTransitionItemCondition(वस्तुहरू[state], exprp);

        Ast_गाँठो_Stmt* matchActionp = nullptr;

        यदि (state == वस्तुहरू.size() - 1) {
            // Last state: sequence complete!  Record the hit in the runtime VlCoverpoint.
            matchActionp = makeRuntimeBinHit(fl, tgt);

            // For illegal_bins, add error message
            यदि (binp->binsType() == VCoverBinsType::BINS_ILLEGAL) {
                अचर string err_सन्देश = "Illegal transition bin " + binp->prettyNameQ()
                                      + " hit in coverpoint " + coverpointp->prettyNameQ();
                matchActionp = matchActionp->addNext(makeIllegalBinAction(fl, err_सन्देश));
            }

            // Reset state to 0
            matchActionp = matchActionp->addNext(
                नूतन AstAssign{fl, नूतन AstVarRef{fl, stateVarp, VAccess::WRITE},
                              नूतन AstConst{fl, AstConst::Widthed_मान{}, 8, 0}});
        } अन्यथा {
            // Intermediate state: advance to next state
            matchActionp = नूतन AstAssign{
                fl, नूतन AstVarRef{fl, stateVarp, VAccess::WRITE},
                नूतन AstConst{fl, AstConst::Widthed_मान{}, 8, static_cast<uint32_t>(state + 1)}};
        }

        // Build restart logic: check if current value matches first item
        // If so, restart sequence from state 1 (even if we're in middle of sequence)
        Ast_गाँठो_Stmt* noMatchActionp = nullptr;
        यदि (state > 0) {
            // Check if current value matches first item (restart condition)
            Ast_गाँठो_अभिव्यक्ति* अचर restartCondp = buildTransitionItemCondition(वस्तुहरू[0], exprp);

            UASSERT_OBJ(restartCondp, वस्तुहरू[0],
                        "buildTransitionItemCondition returned nullptr for restart");

            // Restart to state 1
            Ast_गाँठो_Stmt* restartActionp
                = नूतन AstAssign{fl, नूतन AstVarRef{fl, stateVarp, VAccess::WRITE},
                                नूतन AstConst{fl, AstConst::Widthed_मान{}, 8, 1}};

            // Reset to state 0 (else branch)
            Ast_गाँठो_Stmt* resetActionp
                = नूतन AstAssign{fl, नूतन AstVarRef{fl, stateVarp, VAccess::WRITE},
                                नूतन AstConst{fl, AstConst::Widthed_मान{}, 8, 0}};

            noMatchActionp = नूतन AstIf{fl, restartCondp, restartActionp, resetActionp};
        }
        // For state 0, no action needed if no match (stay in state 0)

        // Combine into if-else
        Ast_गाँठो_Stmt* अचर stmtp = नूतन AstIf{fl, matchCondp, matchActionp, noMatchActionp};

        // Create case item for this state value
        AstCaseItem* अचर caseItemp = नूतन AstCaseItem{
            fl, नूतन AstConst{fl, AstConst::Widthed_मान{}, 8, static_cast<uint32_t>(state)},
            stmtp};

        फर्काऊ caseItemp;
    }

    // Create: $error(msg); $stop;  Used when an illegal bin is hit.
    Ast_गाँठो_Stmt* makeIllegalBinAction(File_पङ्क्ति* fl, अचर string& err_सन्देश) {
        AstDisplay* अचर errorp
            = नूतन AstDisplay{fl, VDisplayType::DT_ERROR, err_सन्देश, nullptr, nullptr};
        errorp->fmtp()->timeunit(m_covergroupp->timeunit());
        static_cast<Ast_गाँठो*>(errorp)->addNext(नूतन AstStop{fl, सत्य});
        फर्काऊ errorp;
    }

    // Preserve the coverpoint's width and signedness after V3Width.
    स्थिर AstConst* new_मान_Const(File_पङ्क्ति* fl, अचर V3_सङ्ख्या& मान, अचर Ast_गाँठो_अभिव्यक्ति* exprp) {
        V3_सङ्ख्या narrowed{fl, exprp->width(), 0};
        narrowed.opAssign(मान);
        AstConst* अचर constp = नूतन AstConst{fl, narrowed};
        constp->dtypeFrom(exprp);
        फर्काऊ constp;
    }

    // A real copy of a real or integral range bound, for comparing with a real coverpoint.
    स्थिर AstConst* newRealConst(AstConst* constp) {
        यदि (constp->अङ्क().isDouble()) फर्काऊ constp->clone_रूख(असत्य);
        V3_सङ्ख्या real{&constp->अङ्क(), 64};
        real.opIToRD(constp->अङ्क(), constp->isSigned());
        फर्काऊ नूतन AstConst{constp->fileline(), real};
    }

    // Clone a constant node, widening to targetWidth if needed (zero-extend).
    // Used to ensure comparisons use matching widths after V3Width has run.
    स्थिर AstConst* widenConst(File_पङ्क्ति* fl, AstConst* constp, पूर्णाङ्क targetWidth) {
        यदि (constp->width() == targetWidth) फर्काऊ constp->clone_रूख(असत्य);
        V3_सङ्ख्या अङ्क{fl, targetWidth, 0};
        अङ्क.opAssign(constp->अङ्क());
        फर्काऊ नूतन AstConst{fl, अङ्क};
    }

    // Build a range condition: minp <= exprp <= maxp.
    // Uses signed comparisons if exprp is signed; omits trivially-true domain bounds.
    // All arguments are non-owning; clones exprp/minp/maxp as needed.
    Ast_गाँठो_अभिव्यक्ति* makeRangeCondition(File_पङ्क्ति* fl, Ast_गाँठो_अभिव्यक्ति* exprp, Ast_गाँठो_अभिव्यक्ति* minp,
                                    Ast_गाँठो_अभिव्यक्ति* maxp) {
        अचर पूर्णाङ्क अभिव्यक्ति_Width = exprp->widthMin();
        AstConst* अचर minConstp = VN_AS(minp, Const);
        AstConst* अचर maxConstp = VN_AS(maxp, Const);
        यदि (exprp->isDouble()) {
            // A real coverpoint has no finite domain bounds to omit.
            फर्काऊ नूतन AstAnd{fl,
                              नूतन AstGteD{fl, exprp->clone_रूख(असत्य), newRealConst(minConstp)},
                              नूतन AstLteD{fl, exprp->clone_रूख(असत्य), newRealConst(maxConstp)}};
        }
        // Widen constants to match expression width so post-V3Width nodes use correct macros
        AstConst* अचर minWidep = widenConst(fl, minConstp, अभिव्यक्ति_Width);
        AstConst* अचर maxWidep = widenConst(fl, maxConstp, अभिव्यक्ति_Width);
        V3_सङ्ख्या minimum{fl, अभिव्यक्ति_Width, 0};
        V3_सङ्ख्या maximum{fl, अभिव्यक्ति_Width, 0};
        maximum.setAllBits1();
        यदि (exprp->isSigned()) {
            minimum.setBit(अभिव्यक्ति_Width - 1, 1);
            maximum.setBit(अभिव्यक्ति_Width - 1, 0);
        }
        Ast_गाँठो_अभिव्यक्ति* lowerp = nullptr;
        Ast_गाँठो_अभिव्यक्ति* upperp = nullptr;
        यदि (minWidep->अङ्क().isCaseEq(minimum)) {
            VL_DO_DANGLING(थप_Deletep(minWidep), minWidep);
        } अन्यथा {
            lowerp = exprp->isSigned() ? static_cast<Ast_गाँठो_अभिव्यक्ति*>(
                                             नूतन AstGteS{fl, exprp->clone_रूख(असत्य), minWidep})
                                       : static_cast<Ast_गाँठो_अभिव्यक्ति*>(
                                             नूतन AstGte{fl, exprp->clone_रूख(असत्य), minWidep});
        }
        यदि (maxWidep->अङ्क().isCaseEq(maximum)) {
            VL_DO_DANGLING(थप_Deletep(maxWidep), maxWidep);
        } अन्यथा {
            upperp = exprp->isSigned() ? static_cast<Ast_गाँठो_अभिव्यक्ति*>(
                                             नूतन AstLteS{fl, exprp->clone_रूख(असत्य), maxWidep})
                                       : static_cast<Ast_गाँठो_अभिव्यक्ति*>(
                                             नूतन AstLte{fl, exprp->clone_रूख(असत्य), maxWidep});
        }
        यदि (lowerp && upperp) फर्काऊ नूतन AstAnd{fl, lowerp, upperp};
        यदि (lowerp) फर्काऊ lowerp;
        यदि (upperp) फर्काऊ upperp;
        फर्काऊ नूतन AstConst{fl, AstConst::BitTrue{}};
    }

    // Build a one-sided comparison for an open-ended bin range whose other bound is '$'.
    // '$' denotes the coverpoint domain extreme, so {[lo:$]} == (expr >= lo) and
    // {[$:hi]} == (expr <= hi).
    Ast_गाँठो_अभिव्यक्ति* makeOpenRangeCondition(File_पङ्क्ति* fl, Ast_गाँठो_अभिव्यक्ति* exprp, AstConst* boundp,
                                        तार्किक isLowerBound) {
        AstConst* अचर widep = widenConst(fl, boundp, exprp->widthMin());
        यदि (isLowerBound) {
            यदि (exprp->isSigned()) फर्काऊ नूतन AstGteS{fl, exprp->clone_रूख(असत्य), widep};
            फर्काऊ नूतन AstGte{fl, exprp->clone_रूख(असत्य), widep};
        }
        यदि (exprp->isSigned()) फर्काऊ नूतन AstLteS{fl, exprp->clone_रूख(असत्य), widep};
        फर्काऊ नूतन AstLte{fl, exprp->clone_रूख(असत्य), widep};
    }

    // Build condition for a single transition item.
    // Returns expression that checks if exprp matches the item's value/range list.
    // Overload for when the expression is a variable read -- creates and manages the VarRef
    // internally, so callers don't need to construct a temporary node.
    Ast_गाँठो_अभिव्यक्ति* buildTransitionItemCondition(AstCoverTransItem* itemp, AstVar* varp) {
        Ast_गाँठो_अभिव्यक्ति* varRefp = नूतन AstVarRef{varp->fileline(), varp, VAccess::READ};
        Ast_गाँठो_अभिव्यक्ति* अचर condp = buildTransitionItemCondition(itemp, varRefp);
        VL_DO_DANGLING(थप_Deletep(varRefp), varRefp);
        फर्काऊ condp;
    }

    // Non-owning: exprp is cloned internally; caller retains ownership of exprp.
    Ast_गाँठो_अभिव्यक्ति* buildTransitionItemCondition(AstCoverTransItem* itemp, Ast_गाँठो_अभिव्यक्ति* exprp) {
        Ast_गाँठो_अभिव्यक्ति* condp = nullptr;

        लागि (Ast_गाँठो* valp = itemp->valuesp(); valp; valp = valp->nextp()) {
            Ast_गाँठो_अभिव्यक्ति* singleCondp = nullptr;
            valp = V3Const::constifyEdit(valp);
            AstConst* अचर constp = VN_CAST(valp, Const);
            यदि (!constp) {
                valp->v3error("Non-constant expression in transition bin; "
                              "values must be constants (IEEE 1800-2023 19.5)");
                फर्काऊ नूतन AstConst{valp->fileline(), AstConst::BitFalseErroring{}};
            }
            singleCondp
                = नूतन AstEq{constp->fileline(), exprp->clone_रूख(असत्य), constp->clone_रूख(असत्य)};
            यदि (condp) {
                condp = नूतन AstOr{itemp->fileline(), condp, singleCondp};
            } अन्यथा {
                condp = singleCondp;
            }
        }

        फर्काऊ condp;
    }

    // Generate code for a single transition sequence (used by both regular and array bins)
    शून्य generateSingleTransitionCode(AstCoverpoint* coverpointp, AstCoverBin* binp,
                                      Ast_गाँठो_अभिव्यक्ति* exprp, अचर ConvBinTarget& tgt,
                                      AstCoverTransSet* transSetp) {
        UINFO(4, "      Generating code for transition sequence");

        // Get or create previous value variable
        AstVar* अचर prevVarp = create_Prev_मान_Var(coverpointp, exprp);

        UASSERT_OBJ(
            transSetp, binp,
            "Transition bin has no transition set (transp() was checked before calling this)");

        // Get transition items (the sequence: item1 => item2 => item3)
        std::vector<AstCoverTransItem*> वस्तुहरू;
        लागि (Ast_गाँठो* itemp = transSetp->itemsp(); itemp; itemp = itemp->nextp())
            वस्तुहरू.थप_back(VN_AS(itemp, CoverTransItem));

        यदि (वस्तुहरू.empty()) {
            binp->v3error("Transition set without items");
            फर्काऊ;
        }

        यदि (वस्तुहरू.size() == 1) {
            // Single item transition not valid (need at least 2 values for =>)
            binp->v3error("Transition requires at least two values");
            फर्काऊ;
        } अन्यथा यदि (वस्तुहरू.size() == 2) {
            // Simple two-value transition: (val1 => val2)
            // Use optimized direct comparison (no state machine needed)
            Ast_गाँठो_अभिव्यक्ति* अचर cond1p = buildTransitionItemCondition(वस्तुहरू[0], prevVarp);
            Ast_गाँठो_अभिव्यक्ति* अचर cond2p = buildTransitionItemCondition(वस्तुहरू[1], exprp);

            // Combine: prev matches val1 AND current matches val2
            Ast_गाँठो_अभिव्यक्ति* fullCondp = नूतन AstAnd{binp->fileline(), cond1p, cond2p};

            addConvTransHitIf(coverpointp, binp, tgt, fullCondp);

            UINFO(4, "        Successfully added 2-value transition if statement");
        } अन्यथा {
            // Multi-value sequence (a => b => c => ...)
            // Use state machine to track position in sequence
            generate_Multi_मान_Transition_Code(coverpointp, binp, exprp, tgt, वस्तुहरू);
        }
    }

    // Append a "{ VlCoverpoint* __Vcx_cps[] = {cp0, cp1, ...}; <call> }" statement.  The brace
    // and the temporary array stay literal text -- a CMethodHard is one call, not a block --
    // but callp itself carries the member, method and '->'.  Construction only: init() copies
    // the array into the cross, so sample() reads it from there and needs no array at all.
    AstCStmt* makeCrossCpsCall(File_पङ्क्ति* fl, अचर std::vector<AstVar*>& cpVars,
                               AstCMethodHard* callp) {
        AstCStmt* अचर cs = नूतन AstCStmt{fl};
        cs->add("{ VlCoverpoint* __Vcx_cps[] = {");
        लागि (माप_प्रकार d = 0; d < cpVars.size(); ++d) {
            यदि (d != 0) cs->add(", ");
            cs->add(memberRef(fl, cpVars[d]));
        }
        cs->add("}; ");
        cs->add(callp);
        cs->add("; }");
        फर्काऊ cs;
    }

    // Assign the per-bin flags individually: one-bit SV results have integer C++ storage types,
    // which may narrow in a bool initializer list but convert implicitly in assignments.
    AstCStmt* makeCrossIffsCall(File_पङ्क्ति* fl, अचर std::vector<AstCoverCrossBin*>& bins,
                                AstCMethodHard* callp) {
        AstCStmt* अचर cs = नूतन AstCStmt{fl};
        cs->add("{ bool __Vcx_iffs[" + cvtToStr(bins.size()) + "]; ");
        लागि (माप_प्रकार i = 0; i < bins.size(); ++i) {
            अचर AstCoverCrossBin* अचर binp = bins[i];
            cs->add("__Vcx_iffs[" + cvtToStr(i) + "] = ");
            cs->add(binp->iffp() ? binp->iffp()->clone_रूख(असत्य)
                                 : नूतन AstConst{fl, AstConst::BitTrue{}});
            cs->add("; ");
        }
        cs->add(callp);
        cs->add("; }");
        फर्काऊ cs;
    }

    using CrossSelection = std::vector<uint64_t>;
    संरचना ResolvedCrossBin final {
        AstCoverCrossBin* binp;
        CrossSelection selection;
    };
    संरचना CrossLayout final {
        uint32_t tuples = 0;
        uint32_t autoBins = 0;
        uint64_t bin_शब्दहरू = 0;
        तार्किक valid = सत्य;
        std::vector<ResolvedCrossBin> bins;
    };
    संरचना CrossSelectionContext final {
        AstCoverCross* crossp;  // Cross whose tuple space is being selected
        अचर std::vector<AstVar*>& cpVars;  // Feeding coverpoints in dimension order
        अचर std::map<std::string, uint32_t>& dimensions;  // Coverpoint name -> dimension
        uint32_t tuples;  // Size of the Cartesian product
        std::vector<uint32_t> strides;  // Flat-index stride per dimension
        तार्किक valid = सत्य;  // False if this explicit bin cannot be implemented
    };
    संरचना CrossBinsofTarget final {
        अचर CoverpointBins* m_binsp = nullptr;  // Declared bins; null after a reported error
        uint32_t m_dimension = 0;  // Coverpoint index within the cross
        uint32_t m_first = 0;  // First declared normal bin in the selected span
        uint32_t m_गन्ती = 0;  // Number of declared normal bins in the selected span
        uint32_t m_declaredFirst = 0;  // First runtime bin index of the selected span
        uint32_t m_declaredEnd = UINT32_MAX;  // One past the last runtime bin index
        int32_t m_sized = -1;  // Index of the sized array holding the selected bins; or -1
    };
    संरचना Cross_मान_Range final {
        V3_सङ्ख्या lo;  // Inclusive lower bound, sign-extended to the comparison width
        V3_सङ्ख्या hi;  // Inclusive upper bound
        V3_सङ्ख्या pattern;  // Allowed bit values; all X for an ordinary interval
        तार्किक wildcard = असत्य;  // A wildcard singleton rather than an exact four-state value

        Cross_मान_Range(Ast_गाँठो* nodep, पूर्णाङ्क width)
            : lo{nodep, width}
            , hi{nodep, width}
            , pattern{nodep, width} {
            pattern.setAllBitsX();
        }
    };

    स्थिर std::vector<Ast_गाँठो*> cross_Bin_मानहरू(अचर Cross_Bin_मानहरू& bin) {
        यदि (bin.valuep) फर्काऊ {bin.valuep};
        std::vector<Ast_गाँठो*> मानहरू;
        यदि (bin.binp->transp()) {
            // IEEE 1800-2023 19.6.1: binsof uses the last value of each transition.
            लागि (Ast_गाँठो* setp = bin.binp->transp(); setp; setp = setp->nextp()) {
                AstCoverTransItem* lastp = VN_AS(setp, CoverTransSet)->itemsp();
                जबसम्म (lastp->nextp()) lastp = VN_AS(lastp->nextp(), CoverTransItem);
                लागि (Ast_गाँठो* valuep = lastp->valuesp(); valuep; valuep = valuep->nextp()) {
                    मानहरू.थप_back(valuep);
                }
            }
        } अन्यथा {
            लागि (Ast_गाँठो* valuep = bin.binp->rangesp(); valuep; valuep = valuep->nextp()) {
                मानहरू.थप_back(valuep);
            }
        }
        फर्काऊ मानहरू;
    }

    // The values of the element-th bin of a run, sign-extended to 'width'
    स्थिर Cross_मान_Range runBinRange(Ast_गाँठो* nodep, अचर BinRun& run, uint32_t element,
                                       पूर्णाङ्क width) {
        अचर पूर्णाङ्क runw = run.m_lo.width();
        V3_सङ्ख्या offset{nodep, runw};
        offset.opMul(run.m_stride, V3_सङ्ख्या{nodep, runw, element});
        V3_सङ्ख्या lo{nodep, runw};
        lo.opAdd(run.m_lo, offset);
        V3_सङ्ख्या hi = run.m_hi;
        यदि (element + 1 < run.m_गन्ती) {
            offset.opSub(run.m_stride, V3_सङ्ख्या{nodep, runw, 1});
            hi.opAdd(lo, offset);
        }
        Cross_मान_Range range{nodep, width};
        range.lo.opExtendS(lo, runw);
        range.hi.opExtendS(hi, runw);
        फर्काऊ range;
    }

    स्थिर पूर्णाङ्क crossRangeWidth(Ast_गाँठो* nodep) {
        यदि (अचर AstInsideRange* अचर rangep = VN_CAST(nodep, InsideRange)) {
            फर्काऊ std::max(rangep->lhsp()->width(), rangep->rhsp()->width());
        }
        फर्काऊ nodep->width();
    }

    स्थिर तार्किक cross_मान_Less(अचर V3_सङ्ख्या& lhs, अचर V3_सङ्ख्या& rhs) {
        V3_सङ्ख्या result{&lhs};
        फर्काऊ !result.opLtS(lhs, rhs).isEqZero();
    }

    // Round a real bound inward to the nearest coverpoint value (IEEE 1800-2023 19.5.7).  Sets
    // 'empty' if the domain has no value on the bound's side.
    स्थिर शून्य crossRealBound(अचर AstConst* constp, Ast_गाँठो_अभिव्यक्ति* exprp, तार्किक upper,
                               अचर Cross_मान_Range& domain, V3_सङ्ख्या& result, तार्किक& empty) {
        अचर द्विगुण मान = constp->अङ्क().toDouble();
        अचर द्विगुण bound = upper ? std::न्यूनमान(मान) : std::उच्चमान(मान);
        // Powers of two are exact, so the integral bound compares exactly with the domain edges.
        अचर द्विगुण limit
            = std::ldexp(1.0, exprp->isSigned() ? exprp->width() - 1 : exprp->width());
        अचर द्विगुण minimum = exprp->isSigned() ? -limit : 0.0;
        यदि (std::isnan(bound) || (upper ? bound < minimum : bound >= limit)) {
            empty = सत्य;
            result = domain.lo;
        } अन्यथा यदि (bound < minimum) {
            result = domain.lo;
        } अन्यथा यदि (bound >= limit) {
            result = domain.hi;
        } अन्यथा {
            V3_सङ्ख्या real{&result, 64};
            real.setDouble(bound);
            result.opRToIRoundS(real);
        }
    }

    स्थिर तार्किक crossRangeBound(Ast_गाँठो* nodep, Ast_गाँठो_अभिव्यक्ति* exprp, तार्किक upper, तार्किक bin_मान,
                                अचर Cross_मान_Range& domain, V3_सङ्ख्या& result, तार्किक& empty) {
        यदि (VN_IS(nodep, Unbounded)) {
            V3_सङ्ख्या limit{nodep, exprp->width()};
            यदि (upper) limit.setAllBits1();
            यदि (exprp->isSigned()) {
                limit.setBit(exprp->width() - 1, !upper);
                result.opExtendS(limit, limit.width());
            } अन्यथा {
                result.opAssign(limit);
            }
            फर्काऊ सत्य;
        }
        अचर AstConst* अचर constp = VN_CAST(nodep, Const);
        यदि (!constp) फर्काऊ असत्य;
        यदि (constp->अङ्क().isDouble()) {
            crossRealBound(constp, exprp, upper, domain, result, empty);
            फर्काऊ सत्य;
        }
        यदि (constp->अङ्क().isString()) फर्काऊ असत्य;
        यदि (bin_मान && exprp->isSigned() && constp->width() <= exprp->width()) {
            // Bin bit patterns use the coverpoint's effective type (IEEE 1800-2023 19.5.7).
            // Wider values and intersect filters retain their values for domain clipping.
            V3_सङ्ख्या मान{nodep, exprp->width()};
            यदि (constp->isSigned()) {
                मान.opExtendS(constp->अङ्क(), constp->width());
            } अन्यथा {
                मान.opAssign(constp->अङ्क());
            }
            result.opExtendS(मान, मान.width());
        } अन्यथा यदि (constp->isSigned()) {
            result.opExtendS(constp->अङ्क(), constp->width());
        } अन्यथा {
            result.opAssign(constp->अङ्क());
        }
        फर्काऊ सत्य;
    }

    स्थिर Cross_मान_Range cross_मान_Domain(Ast_गाँठो* nodep, पूर्णाङ्क मान_Width, तार्किक isSigned,
                                            पूर्णाङ्क width) {
        Cross_मान_Range domain{nodep, width};
        V3_सङ्ख्या lo{nodep, मान_Width};
        V3_सङ्ख्या hi{nodep, मान_Width};
        hi.setAllBits1();
        यदि (isSigned) {
            lo.setBit(मान_Width - 1, 1);
            hi.setBit(मान_Width - 1, 0);
            domain.lo.opExtendS(lo, मान_Width);
            domain.hi.opExtendS(hi, मान_Width);
        } अन्यथा {
            domain.lo.opAssign(lo);
            domain.hi.opAssign(hi);
        }
        फर्काऊ domain;
    }

    स्थिर शून्य intersectCrossRange(Cross_मान_Range& range, अचर Cross_मान_Range& other) {
        यदि (cross_मान_Less(range.lo, other.lo)) range.lo = other.lo;
        यदि (cross_मान_Less(other.hi, range.hi)) range.hi = other.hi;
    }

    स्थिर तार्किक cross_मान_Range(Ast_गाँठो* nodep, Ast_गाँठो_अभिव्यक्ति* exprp, तार्किक bin_मान, तार्किक wildcard,
                                अचर Cross_मान_Range& domain, Cross_मान_Range& range) {
        तार्किक empty = असत्य;
        अचर AstConst* अचर constp = VN_CAST(nodep, Const);
        यदि (अचर AstInsideRange* अचर rangep = VN_CAST(nodep, InsideRange)) {
            यदि (!crossRangeBound(rangep->lhsp(), exprp, असत्य, bin_मान, domain, range.lo, empty)
                || !crossRangeBound(rangep->rhsp(), exprp, सत्य, bin_मान, domain, range.hi, empty)
                || range.lo.isFourState() || range.hi.isFourState()) {
                फर्काऊ असत्य;
            }
        } अन्यथा यदि (constp && constp->अङ्क().isDouble()) {
            // A real value participates only if integral; it has no wildcard bits.
            crossRealBound(constp, exprp, असत्य, domain, range.lo, empty);
            crossRealBound(constp, exprp, सत्य, domain, range.hi, empty);
        } अन्यथा {
            यदि (!crossRangeBound(nodep, exprp, असत्य, bin_मान, domain, range.lo, empty)) {
                फर्काऊ असत्य;
            }
            यदि (bin_मान && exprp->isSigned() && constp && !constp->isSigned()
                && constp->width() > exprp->width()) {
                तार्किक representable = सत्य;
                लागि (पूर्णाङ्क bit = exprp->width(); bit < constp->width(); ++bit) {
                    representable &= !constp->अङ्क().bitIs1(bit);
                }
                यदि (representable) {
                    V3_सङ्ख्या मान{nodep, exprp->width()};
                    मान.opAssign(constp->अङ्क());
                    range.lo.opExtendS(मान, मान.width());
                }
            }
            range.hi = range.lo;
            range.wildcard = wildcard;
            यदि (wildcard) {
                range.pattern = range.lo;
                range.lo = domain.lo;
                range.hi = domain.hi;
                यदि (constp && constp->isSigned()) {
                    // Replicated X sign bits are correlated, not independent wildcards.
                    // The source domain preserves expansion-before-casting (19.5.7).
                    intersectCrossRange(
                        range, cross_मान_Domain(nodep, constp->width(), सत्य, domain.lo.width()));
                }
            }
        }
        यदि (empty) {
            range.lo = domain.hi;
            range.hi = domain.lo;
            फर्काऊ सत्य;
        }
        यदि (!range.lo.isFourState()) intersectCrossRange(range, domain);
        यदि (range.wildcard && exprp->isSigned()) {
            अचर पूर्णाङ्क sign = exprp->width() - 1;
            यदि (constp && !constp->isSigned() && constp->width() > exprp->width()) {
                // Unsigned equality preserves every target bit pattern if discarded bits are zero.
                लागि (पूर्णाङ्क bit = exprp->width(); bit < constp->width(); ++bit) {
                    यदि (constp->अङ्क().bitIs1(bit)) {
                        range.lo = domain.hi;
                        range.hi = domain.lo;
                        फर्काऊ सत्य;
                    }
                }
                लागि (पूर्णाङ्क bit = exprp->width(); bit < range.pattern.width(); ++bit) {
                    यदि (range.pattern.bitIsXZ(sign))
                        range.pattern.setBit(bit, 'x');
                    अन्यथा
                        range.pattern.setBit(bit, range.pattern.bitIs1(sign));
                }
                फर्काऊ सत्य;
            }
            // Discarded high bits constrain the target sign, rather than becoming don't-cares.
            लागि (पूर्णाङ्क bit = exprp->width(); bit < range.pattern.width(); ++bit) {
                यदि (range.pattern.bitIsXZ(bit)) जारी;
                अचर तार्किक मान = range.pattern.bitIs1(bit);
                यदि (!range.pattern.bitIsXZ(sign) && range.pattern.bitIs1(sign) != मान) {
                    range.lo = domain.hi;
                    range.hi = domain.lo;
                    रोक;
                }
                range.pattern.setBit(sign, मान);
            }
        }
        फर्काऊ सत्य;
    }

    गणना CrossRangeState : uint8_t {
        CROSS_INSIDE_BOUNDS = 0,  // Prefix is strictly inside the interval
        CROSS_AT_LOWER = 1,
        CROSS_AT_UPPER = 2,
        CROSS_AT_BOUNDS = CROSS_AT_LOWER | CROSS_AT_UPPER,
        CROSS_NO_MATCH = 4  // Prefix cannot match the interval/pattern
    };

    स्थिर CrossRangeState crossRangeStep(अचर Cross_मान_Range& range, CrossRangeState state,
                                          पूर्णाङ्क bit, पूर्णाङ्क मान) {
        यदि (state == CROSS_NO_MATCH) फर्काऊ CROSS_NO_MATCH;
        // Flipping the sign bit makes signed order lexicographic.
        अचर तार्किक sign = bit == range.pattern.width() - 1;
        यदि (!range.pattern.bitIsXZ(bit) && मान != (range.pattern.bitIs1(bit) ^ sign)) {
            फर्काऊ CROSS_NO_MATCH;
        }
        अचर पूर्णाङ्क तल्लो = range.lo.bitIs1(bit) ^ sign;
        अचर पूर्णाङ्क माथिल्लो = range.hi.bitIs1(bit) ^ sign;
        यदि (((state & CROSS_AT_LOWER) && मान < तल्लो)
            || ((state & CROSS_AT_UPPER) && मान > माथिल्लो)) {
            फर्काऊ CROSS_NO_MATCH;
        }
        फर्काऊ static_cast<CrossRangeState>(
            ((state & CROSS_AT_LOWER) && मान == तल्लो ? CROSS_AT_LOWER : CROSS_INSIDE_BOUNDS)
            | ((state & CROSS_AT_UPPER) && मान == माथिल्लो ? CROSS_AT_UPPER : CROSS_INSIDE_BOUNDS));
    }

    स्थिर तार्किक crossWildcardIntersects(अचर Cross_मान_Range& range) {
        अचिन्हित states = 1U << CROSS_AT_BOUNDS;
        लागि (पूर्णाङ्क bit = range.pattern.width() - 1; bit >= 0 && states; --bit) {
            अचिन्हित next = 0;
            लागि (अचर CrossRangeState state :
                 {CROSS_INSIDE_BOUNDS, CROSS_AT_LOWER, CROSS_AT_UPPER, CROSS_AT_BOUNDS}) {
                यदि (!(states & (1U << state))) जारी;
                लागि (पूर्णाङ्क मान = 0; मान < 2; ++मान) {
                    अचर CrossRangeState equal = crossRangeStep(range, state, bit, मान);
                    यदि (equal != CROSS_NO_MATCH) next |= 1U << equal;
                }
            }
            states = next;
        }
        फर्काऊ states != 0;
    }

    // The least value at least 'from' with the non-x bits of 'pattern', in unsigned order; false
    // if none.  A bit scan, which does not enumerate the pattern's x bits.
    स्थिर तार्किक next_Pattern_मान(अचर V3_सङ्ख्या& pattern, अचर V3_सङ्ख्या& from, V3_सङ्ख्या& result) {
        result.opAssign(from);
        पूर्णाङ्क carry = -1;  // Lowest x bit above the bit scanned where 'from' has a 0
        लागि (पूर्णाङ्क bit = from.width() - 1; bit >= 0; --bit) {
            यदि (pattern.bitIsXZ(bit)) {
                यदि (!from.bitIs1(bit)) carry = bit;
                जारी;
            }
            यदि (pattern.bitIs1(bit) == from.bitIs1(bit)) जारी;
            यदि (from.bitIs1(bit)) {  // Only a greater prefix, an x bit above raised, can match
                यदि (carry < 0) फर्काऊ असत्य;
                bit = carry;
            }
            // Then the least such value: the pattern's bits, with x bits of zero
            result.setBit(bit, 1);
            जबसम्म (--bit >= 0) result.setBit(bit, pattern.bitIs1(bit));
            फर्काऊ सत्य;
        }
        फर्काऊ सत्य;
    }

    // Append to 'runs' the maximal runs of consecutive values of 'range' that its pattern
    // matches, in value order, until there are more than 'limit'.  A match continues through
    // the pattern's trailing x bits only, which the next value's carry leaves.
    स्थिर शून्य crossRangeRuns(अचर Cross_मान_Range& range, माप_प्रकार limit,
                               std::vector<std::pair<V3_सङ्ख्या, V3_सङ्ख्या>>& runs) {
        // Values order signed, which is the unsigned order of values with the sign bit flipped
        अचर पूर्णाङ्क sign = range.lo.width() - 1;
        अचर स्वतः flipped = [sign](V3_सङ्ख्या मान) {
            यदि (!मान.bitIsXZ(sign)) मान.setBit(sign, !मान.bitIs1(sign));
            फर्काऊ मान;
        };
        अचर V3_सङ्ख्या pattern = flipped(range.pattern);
        अचर V3_सङ्ख्या hi = flipped(range.hi);
        अचर V3_सङ्ख्या one{&hi, hi.width(), 1};
        V3_सङ्ख्या trailing{&hi, hi.width()};
        लागि (पूर्णाङ्क bit = 0; bit <= sign && pattern.bitIsXZ(bit); ++bit) trailing.setBit(bit, 1);
        V3_सङ्ख्या from = flipped(range.lo);
        V3_सङ्ख्या first{&hi, hi.width()};
        V3_सङ्ख्या last{&hi, hi.width()};
        V3_सङ्ख्या less{&hi};
        जबसम्म (runs.size() <= limit && next_Pattern_मान(pattern, from, first)
               && less.opLt(hi, first).isEqZero()) {
            last.opOr(first, trailing);
            यदि (!less.opLt(hi, last).isEqZero()) last = hi;
            runs.emplace_back(flipped(first), flipped(last));
            यदि (last.isCaseEq(hi)) रोक;
            from.opAdd(last, one);
        }
    }

    // True if no coverpoint value participates in a resolved value or range.  A value with x
    // or z bits participates only as a wildcard pattern (IEEE 1800-2023 19.5.7).
    स्थिर तार्किक crossRangeEmpty(अचर Cross_मान_Range& range) {
        यदि (range.lo.isFourState()) फर्काऊ सत्य;
        फर्काऊ cross_मान_Less(range.hi, range.lo)
               || (range.wildcard && !crossWildcardIntersects(range));
    }

    // Comparison width that holds both the coverpoint's and a bin or intersect value's range.
    स्थिर पूर्णाङ्क resolveWidth(Ast_गाँठो* nodep, अचर Ast_गाँठो_अभिव्यक्ति* exprp) {
        फर्काऊ std::max(exprp->width(), crossRangeWidth(nodep)) + 1;
    }

    // Resolve a bin or intersect value to coverpoint values (IEEE 1800-2023 19.5.7), in the
    // width 'range' was created with.  False if the value is not a constant integral or real.
    स्थिर तार्किक resolve_मान(Ast_गाँठो* nodep, Ast_गाँठो_अभिव्यक्ति* exprp, तार्किक bin_मान, तार्किक wildcard,
                             Cross_मान_Range& range) {
        अचर Cross_मान_Range domain
            = cross_मान_Domain(nodep, exprp->width(), exprp->isSigned(), range.lo.width());
        फर्काऊ cross_मान_Range(nodep, exprp, bin_मान, wildcard, domain, range);
    }

    स्थिर तार्किक crossRangesIntersect(अचर Cross_मान_Range& bin, अचर Cross_मान_Range& filter) {
        // Values with x or z bits do not participate, even in an identical filter.
        यदि (bin.lo.isFourState() || filter.lo.isFourState()) फर्काऊ असत्य;
        Cross_मान_Range match = bin;
        intersectCrossRange(match, filter);
        फर्काऊ !cross_मान_Less(match.hi, match.lo)
               && (!bin.wildcard || crossWildcardIntersects(match));
    }

    स्थिर शून्य unsupportedCrossRange(AstCoverBinsof* selectp, तार्किक& valid) {
        selectp->v3warn(COVERIGN, "Unsupported: non-constant or non-integral 'intersect' value, "
                                  "or four-state range bound.");
        valid = असत्य;
    }

    स्थिर तार्किक cross_मान_Matches_Filters(AstCoverBinsof* selectp, Ast_गाँठो* valuep,
                                         Ast_गाँठो_अभिव्यक्ति* exprp, अचर AstCoverBin* binp,
                                         अचर Cross_मान_Range& domain,
                                         अचर std::vector<Cross_मान_Range>& filters,
                                         तार्किक& valid) {
        Cross_मान_Range range{valuep, domain.lo.width()};
        यदि (!cross_मान_Range(valuep, exprp, सत्य, binp->isWildcard(), domain, range)) {
            unsupportedCrossRange(selectp, valid);
            फर्काऊ असत्य;
        }
        लागि (अचर Cross_मान_Range& filter : filters) {
            यदि (crossRangesIntersect(range, filter)) फर्काऊ सत्य;
        }
        फर्काऊ असत्य;
    }

    std::vector<तार्किक> selectCoverpointBins(AstCoverBinsof* selectp, अचर CoverpointBins& bins,
                                           uint32_t first, uint32_t गन्ती, तार्किक& valid) {
        यदि (selectp->rangesp() && !bins.exprp->dtypep()->skipRefp()->isIntegralOrPacked()) {
            unsupportedCrossRange(selectp, valid);
            फर्काऊ {};
        }
        std::vector<तार्किक> selected(bins.total, असत्य);
        std::vector<std::vector<Ast_गाँठो*>> मानहरू;
        पूर्णाङ्क width = bins.exprp->width();
        लागि (Ast_गाँठो* rangep = selectp->rangesp(); rangep; rangep = rangep->nextp()) {
            width = std::max(width, crossRangeWidth(rangep));
        }
        यदि (selectp->rangesp()) {
            मानहरू.reserve(गन्ती);
            लागि (uint32_t i = first; i < first + गन्ती; ++i) {
                // A run's values are in the coverpoint type, whose width 'width' covers
                मानहरू.थप_back(bins.मानहरू[i].runp ? std::vector<Ast_गाँठो*>{}
                                                     : cross_Bin_मानहरू(bins.मानहरू[i]));
                लागि (Ast_गाँठो* अचर valuep : मानहरू.back()) {
                    width = std::max(width, crossRangeWidth(valuep));
                }
            }
        }
        // One extra bit preserves both unsigned maxima and negative signed bounds.
        ++width;
        अचर Cross_मान_Range domain
            = cross_मान_Domain(selectp, bins.exprp->width(), bins.exprp->isSigned(), width);
        std::vector<Cross_मान_Range> filters;
        लागि (Ast_गाँठो* rangep = selectp->rangesp(); rangep; rangep = rangep->nextp()) {
            Cross_मान_Range filter{rangep, width};
            यदि (!cross_मान_Range(rangep, bins.exprp, असत्य, असत्य, domain, filter)) {
                unsupportedCrossRange(selectp, valid);
                फर्काऊ {};
            }
            filters.थप_back(std::move(filter));
        }
        लागि (uint32_t i = first; i < first + गन्ती; ++i) {
            यदि (!selectp->rangesp()) {
                selected[i] = सत्य;
                जारी;
            }
            यदि (अचर BinRun* अचर runp = bins.मानहरू[i].runp) {
                अचर Cross_मान_Range range
                    = runBinRange(selectp, *runp, bins.मानहरू[i].element, width);
                selected[i] = std::any_of(filters.begin(), filters.end(),
                                          [&](अचर Cross_मान_Range& filter) {
                                              फर्काऊ crossRangesIntersect(range, filter);
                                          });
                जारी;
            }
            लागि (Ast_गाँठो* अचर valuep : मानहरू[i - first]) {
                selected[i] = cross_मान_Matches_Filters(
                    selectp, valuep, bins.exprp, bins.मानहरू[i].binp, domain, filters, valid);
                यदि (!valid) फर्काऊ {};
                यदि (selected[i]) रोक;
            }
        }
        यदि (selectp->isNegated()) {
            लागि (uint32_t i = 0; i < bins.total; ++i) selected[i] = !selected[i];
        }
        फर्काऊ selected;
    }

    // Constructor-time value metadata of one coverpoint, as C++ list entries
    संरचना मान_Lists final {
        std::vector<std::string> m_ranges;  // Bin, then low and high words
        std::vector<std::string> m_runs;  // First bin, count, then low, span, and high words
        std::vector<std::string> m_patterns;  // Bin, then value, mask, low, and high words
        std::vector<std::string> m_transitions;  // Transition bin
    };

    // Append a value's words, in the coverpoint's width, to a C++ list entry.
    स्थिर शून्य append_शब्दहरू(std::string& text, अचर V3_सङ्ख्या& मान, अचर Ast_गाँठो_अभिव्यक्ति* exprp) {
        V3_सङ्ख्या narrowed{&मान, exprp->width()};
        narrowed.opAssign(मान);
        लागि (पूर्णाङ्क word = 0; word < exprp->width_शब्दहरू(); ++word) {
            text += ", " + cvtToStr(narrowed.edataWord(word)) + "U";
        }
    }

    शून्य collect_मान_Metadata(मान_Lists& lists, Ast_गाँठो_अभिव्यक्ति* exprp, AstCoverBin* binp,
                              uint32_t index, Ast_गाँठो_अभिव्यक्ति* valuep) {
        अचर std::string bin = cvtToStr(index) + "U";
        यदि (binp->transp()) lists.m_transitions.थप_back(bin);
        लागि (Ast_गाँठो* अचर sourcep : cross_Bin_मानहरू({binp, valuep})) {
            Cross_मान_Range range{sourcep, resolveWidth(sourcep, exprp)};
            यदि (!resolve_मान(sourcep, exprp, सत्य, binp->isWildcard(), range)) {
                // Sampling already resolved every state bin value, so only transitions remain.
                sourcep->v3warn(E_UNSUPPORTED, "Unsupported: non-integral value in a transition "
                                               "bin of a coverpoint with exclusions.");
                जारी;
            }
            यदि (crossRangeEmpty(range)) जारी;
            std::string entry = bin;
            यदि (range.wildcard) {
                V3_सङ्ख्या मान{sourcep, exprp->width()};
                V3_सङ्ख्या mask{sourcep, exprp->width()};
                mask.opBitsNonXZ(range.pattern);
                मान.opBitsOne(range.pattern);
                append_शब्दहरू(entry, मान, exprp);
                append_शब्दहरू(entry, mask, exprp);
            }
            append_शब्दहरू(entry, range.lo, exprp);
            append_शब्दहरू(entry, range.hi, exprp);
            (range.wildcard ? lists.m_patterns : lists.m_ranges).थप_back(entry);
        }
    }

    // Describe a run with one entry, from which the runtime computes the values of its bins.
    स्थिर शून्य collectRunMetadata(मान_Lists& lists, Ast_गाँठो_अभिव्यक्ति* exprp, अचर BinRun& run) {
        V3_सङ्ख्या span{exprp, run.m_stride.width(), 0};
        span.opSub(run.m_stride, V3_सङ्ख्या{exprp, run.m_stride.width(), 1});
        std::string entry = cvtToStr(run.m_declared) + "U, " + cvtToStr(run.m_गन्ती) + "U";
        append_शब्दहरू(entry, run.m_lo, exprp);
        append_शब्दहरू(entry, span, exprp);
        append_शब्दहरू(entry, run.m_hi, exprp);
        lists.m_runs.थप_back(entry);
    }

    // Emit one batched metadata list, bounding the size of each call's temporary list.
    शून्य emit_मान_List(File_पङ्क्ति* fl, AstVar* cpVarp, VCMethod method,
                       अचर std::vector<std::string>& entries) {
        लागि (माप_प्रकार first = 0; first < entries.size(); first += मान_LIST_ENTRIES) {
            अचर माप_प्रकार end = std::min(entries.size(), first + मान_LIST_ENTRIES);
            std::string text = "{" + entries[first];
            लागि (माप_प्रकार i = first + 1; i < end; ++i) text += ", " + entries[i];
            m_constructorp->addStmtsp(
                itemCall(fl, cpVarp, method, {ctext(fl, text + "}")})->makeStmt());
        }
    }

    स्थिर तार्किक checkCrossRef(अचर AstCoverCrossRef* refp, अचर AstCoverCross* crossp) {
        यदि (refp->name() == crossp->name()) फर्काऊ सत्य;
        refp->v3error("Cross selection "
                      << refp->prettyNameQ() << " may only name its enclosing cross "
                      << crossp->prettyNameQ() << " (IEEE 1800-2023 19.6.1.2).");
        फर्काऊ असत्य;
    }

    स्थिर तार्किक checkCrossBinName(अचर AstCoverCrossBin* binp, std::set<std::string>& names) {
        यदि (names.emplace(binp->name()).second) फर्काऊ सत्य;
        binp->v3error("Duplicate cross bin " << binp->prettyNameQ()
                                             << " (IEEE 1800-2023 19.6.1).");
        फर्काऊ असत्य;
    }

    // The span of the implicit automatic bin reported as 'name' ('auto_<i>', see
    // createImplicitAutoBins), found without naming each of its bins
    स्थिर तार्किक implicitAutoBinSpan(अचर CoverpointBins& bins, अचर std::string& name,
                                    BinSpan& span) {
        अचर std::string prefix = "auto_";
        यदि (!VString::startsWith(name, prefix)) फर्काऊ असत्य;
        अचर std::string digits = name.substr(prefix.size());
        अचर अचिन्हित दीर्घ index = std::म_से_अदीर्घ(digits.c_str(), nullptr, 10);
        // Only the reported spelling names the bin, not e.g. 'auto_01' or 'auto_x'
        यदि (index >= bins.implicitAuto.गन्ती || digits != cvtToStr(index)) फर्काऊ असत्य;
        अचर uint32_t offset = static_cast<uint32_t>(index);
        span = BinSpan{bins.implicitAuto.first + offset, 1, bins.implicitAuto.declared + offset};
        फर्काऊ सत्य;
    }

    CrossBinsofTarget
    resolveBinsofTarget(अचर AstCoverBinsof* selectp, अचर AstCoverCross* crossp,
                        अचर std::vector<AstVar*>& cpVars,
                        अचर std::map<std::string, uint32_t>& dimensions) अचर {
        अचर स्वतः dim = dimensions.find(selectp->pointp()->name());
        यदि (dim == dimensions.end()) {
            selectp->v3error("binsof coverpoint "
                             << selectp->pointp()->prettyNameQ() << " is not an item of cross "
                             << crossp->prettyNameQ() << " (IEEE 1800-2023 19.6.1).");
            फर्काऊ {};
        }
        अचर CoverpointBins& bins = m_cpBins.at(cpVars[dim->second]);
        CrossBinsofTarget target{&bins, dim->second, 0, bins.total};
        यदि (!selectp->name().empty()) {
            अचर स्वतः bin = bins.spans.find(selectp->name());
            BinSpan span{0, 0, 0};
            यदि (bin != bins.spans.end()) {
                span = bin->second;
            } अन्यथा यदि (!implicitAutoBinSpan(bins, selectp->name(), span)) {
                selectp->v3error("Cannot find bin " << selectp->prettyNameQ() << " in coverpoint "
                                                    << selectp->pointp()->prettyNameQ()
                                                    << " (IEEE 1800-2023 19.6.1).");
                फर्काऊ {};
            }
            target.m_first = span.first;
            target.m_गन्ती = span.गन्ती;
            target.m_declaredFirst = span.declared;
            target.m_declaredEnd = span.declared + span.गन्ती;
            target.m_sized = span.sized;
        }
        फर्काऊ target;
    }

    // The variables a 'with' filter reads, each by its first reference
    using CrossFilterReads = std::map<अचर AstVar*, AstVarRef*>;

    // Whether a 'with' filter has a count of value tuples, of 'matches' other than '$'
    स्थिर तार्किक counts_मान_Tuples(अचर AstCoverCrossWith* withp) {
        फर्काऊ withp->countp() && !VN_IS(withp->countp(), Unbounded);
    }

    // Whether the runtime knows the values of the cross items a 'with' filter reads, or to count
    // value tuples, of all items, of integral coverpoints; else warns
    स्थिर तार्किक crossFilterSupported(अचर AstCoverCrossWith* withp,
                                     अचर CrossFilterReads& reads) {
        लागि (अचर AstVar* varp = withp->varsp(); varp; varp = VN_AS(varp->nextp(), Var)) {
            यदि (varp->dtypep()->skipRefp()->isIntegralOrPacked()) जारी;
            अचर स्वतः it = reads.find(varp);
            यदि (it != reads.end()) {
                it->second->v3warn(COVERIGN, "Unsupported: cross bin 'with' filter reading the "
                                             "value of non-integral coverpoint "
                                                 << it->second->prettyNameQ());
                फर्काऊ असत्य;
            }
            यदि (counts_मान_Tuples(withp)) {
                withp->countp()->v3warn(COVERIGN, "Unsupported: cross bin 'matches' count of "
                                                  "value tuples of non-integral coverpoint "
                                                      << varp->prettyNameQ());
                फर्काऊ असत्य;
            }
        }
        फर्काऊ सत्य;
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
    शून्य generateCrossFilter(अचर AstCoverCrossWith* withp, अचर CrossFilterReads& reads,
                             अचर AstCoverCrossBin* binp, अचर AstCoverCross* crossp,
                             AstVar* cxp, अचर std::map<string, uint32_t>& dimensions,
                             uint32_t वस्तुहरू) {
        File_पङ्क्ति* अचर fl = withp->fileline();
        अचर string prefix
            = m_sizedNames.get(sanitizeGeneratedName(crossp->name() + "__" + binp->name()));
        अचर तार्किक counting = counts_मान_Tuples(withp);
        // The count, 0 for all
        अचर std::pair<Ast_गाँठो_अभिव्यक्ति*, Ast_गाँठो_अभिव्यक्ति*> गन्ती
            = counting ? new_गन्ती_Args(fl, prefix + "_count", withp->countp())
                       : new_गन्ती_Args(fl, withp->countp() ? 0 : 1);
        AstLoop* अचर loopp = नूतन AstLoop{fl};
        AstCMethodHard* अचर nextp = itemCall(fl, cxp, VCMethod::COVERGROUP_WITH_NEXT);
        nextp->dtypeSetBit();
        loopp->addStmtsp(नूतन AstLoopTest{fl, loopp, nextp});
        string dims;
        लागि (uint32_t dim = 0; counting && dim < वस्तुहरू; ++dim) {
            dims += (dims.empty() ? "" : ", ") + cvtToStr(dim) + "U";
        }
        जबसम्म (AstVar* अचर varp = withp->varsp()) {
            varp->unlinkFrBack();
            यदि (!reads.गन्ती(varp)) {
                VL_DO_DANGLING(थप_Deletep(varp), varp);
                जारी;
            }
            अचर स्वतः dimIt = dimensions.find(varp->name());
            UASSERT_OBJ(dimIt != dimensions.end(), varp, "Cross 'with' candidate not of an item");
            अचर uint32_t dim = dimIt->second;
            यदि (!counting) dims += (dims.empty() ? "" : ", ") + cvtToStr(dim) + "U";
            varp->name(prefix + "_" + sanitizeGeneratedName(varp->name()));
            m_constructorp->addStmtsp(varp);
            यदि (varp->isWide()) {
                loopp->addStmtsp(itemCall(fl, cxp, VCMethod::COVERGROUP_CROSS_WITH_मान_W,
                                          {cnum(fl, dim), नूतन AstVarRef{fl, varp, VAccess::WRITE}})
                                     ->makeStmt());
            } अन्यथा {
                AstCMethodHard* अचर valuep
                    = itemCall(fl, cxp, VCMethod::COVERGROUP_CROSS_WITH_मान, {cnum(fl, dim)});
                valuep->dtypeSetUInt64();
                Ast_गाँठो_DType* अचर dtypep = varp->findLogicDType(
                    varp->width(), varp->width(),
                    varp->isSigned() ? VSigning::SIGNED : VSigning::UNSIGNED);
                loopp->addStmtsp(नूतन AstAssign{fl, नूतन AstVarRef{fl, varp, VAccess::WRITE},
                                               resize_मान(valuep, dtypep)});
            }
        }
        loopp->addStmtsp(itemCall(fl, cxp, VCMethod::COVERGROUP_CROSS_WITH_TEST,
                                  {withp->filterp()->unlinkFrBack()})
                             ->makeStmt());
        अचर तार्किक prot = v3Global.opt.protectIds();
        m_constructorp->addStmtsp(
            itemCall(fl, cxp, VCMethod::COVERGROUP_CROSS_WITH_BEGIN,
                     {ctext(fl, "{" + dims + "}"), गन्ती.first, गन्ती.second,
                      ctext(fl, quoted(VIdProtect::protect_शब्दहरू_If(binp->name(), prot))),
                      ctext(fl, quoted(VIdProtect::protectIf(fl->filename(), prot))),
                      cnum(fl, static_cast<uint32_t>(fl->lineno()))})
                ->makeStmt());
        m_constructorp->addStmtsp(loopp);
        m_constructorp->addStmtsp(
            itemCall(fl, cxp, VCMethod::COVERGROUP_CROSS_WITH_END)->makeStmt());
    }

    तार्किक generateRuntimeSelection(Ast_गाँठो* nodep, अचर AstCoverCrossBin* binp,
                                  AstCoverCross* crossp, AstVar* cxp,
                                  अचर std::vector<AstVar*>& cpVars,
                                  अचर std::map<string, uint32_t>& dimensions) {
        File_पङ्क्ति* अचर fl = nodep->fileline();
        यदि (अचर AstCoverCrossRef* अचर refp = VN_CAST(nodep, CoverCrossRef)) {
            यदि (!checkCrossRef(refp, crossp)) फर्काऊ असत्य;
            m_constructorp->addStmtsp(
                itemCall(fl, cxp, VCMethod::COVERGROUP_SELECT_ALL)->makeStmt());
            फर्काऊ सत्य;
        }
        यदि (अचर AstCoverCrossSelect* अचर opp = VN_CAST(nodep, CoverCrossSelect)) {
            यदि (!generateRuntimeSelection(opp->lhsp(), binp, crossp, cxp, cpVars, dimensions)
                || !generateRuntimeSelection(opp->rhsp(), binp, crossp, cxp, cpVars, dimensions)) {
                फर्काऊ असत्य;
            }
            m_constructorp->addStmtsp(itemCall(fl, cxp,
                                               opp->isOr() ? VCMethod::COVERGROUP_SELECT_OR
                                                           : VCMethod::COVERGROUP_SELECT_AND)
                                          ->makeStmt());
            फर्काऊ सत्य;
        }
        यदि (अचर AstCoverCrossWith* अचर withp = VN_CAST(nodep, CoverCrossWith)) {
            CrossFilterReads reads;
            withp->filterp()->foreach([&](AstVarRef* refp) { reads.emplace(refp->varp(), refp); });
            // The count of 'matches' is a positive integer, or '$' (IEEE 1800-2023 19.6.1.2)
            यदि ((counts_मान_Tuples(withp)
                 && !check_गन्ती(withp->countp(), "Cross bin 'matches' count", "19.6.1.2"))
                || !crossFilterSupported(withp, reads)
                || !generateRuntimeSelection(withp->selectp(), binp, crossp, cxp, cpVars,
                                             dimensions)) {
                फर्काऊ असत्य;
            }
            generateCrossFilter(withp, reads, binp, crossp, cxp, dimensions,
                                static_cast<uint32_t>(cpVars.size()));
            फर्काऊ सत्य;
        }
        AstCoverBinsof* अचर selectp = VN_AS(nodep, CoverBinsof);
        अचर CrossBinsofTarget target = resolveBinsofTarget(selectp, crossp, cpVars, dimensions);
        यदि (!target.m_binsp) फर्काऊ असत्य;
        अचर CoverpointBins& bins = *target.m_binsp;
        यदि (selectp->rangesp() && !bins.exprp->dtypep()->skipRefp()->isIntegralOrPacked()) {
            तार्किक valid = सत्य;
            unsupportedCrossRange(selectp, valid);
            फर्काऊ असत्य;
        }
        Ast_गाँठो_अभिव्यक्ति* firstp;
        Ast_गाँठो_अभिव्यक्ति* endp;
        यदि (target.m_sized < 0) {
            firstp = cnum(fl, target.m_declaredFirst);
            endp = cnum(fl, target.m_declaredEnd);
        } अन्यथा {  // A sized array, whose bins the coverpoint's construction placed
            AstVar* अचर cpVarp = cpVars[target.m_dimension];
            अचर uint32_t sized = static_cast<uint32_t>(target.m_sized);
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
        लागि (Ast_गाँठो* rangep = selectp->rangesp(); rangep; rangep = rangep->nextp()) {
            Cross_मान_Range range{rangep, resolveWidth(rangep, bins.exprp)};
            यदि (!resolve_मान(rangep, bins.exprp, असत्य, असत्य, range)) {
                तार्किक valid = सत्य;
                unsupportedCrossRange(selectp, valid);
                फर्काऊ असत्य;
            }
            यदि (crossRangeEmpty(range)) जारी;
            m_constructorp->addStmtsp(itemCall(fl, cxp,
                                               bins.exprp->isWide()
                                                   ? VCMethod::COVERGROUP_SELECT_RANGE_W
                                                   : VCMethod::COVERGROUP_SELECT_RANGE,
                                               {new_मान_Const(fl, range.lo, bins.exprp),
                                                new_मान_Const(fl, range.hi, bins.exprp)})
                                          ->makeStmt());
        }
        m_constructorp->addStmtsp(
            itemCall(fl, cxp, VCMethod::COVERGROUP_SELECT_DIM_END)->makeStmt());
        फर्काऊ सत्य;
    }

    std::vector<AstCoverCrossBin*>
    generateRuntimeCrossBins(AstCoverCross* crossp, AstVar* cxp,
                             अचर std::vector<AstVar*>& cpVars,
                             अचर std::map<string, uint32_t>& dimensions) {
        std::vector<AstCoverCrossBin*> bins;
        std::set<string> names;
        लागि (Ast_गाँठो* itemp = crossp->binsp(); itemp; itemp = itemp->nextp()) {
            AstCoverCrossBin* अचर binp = VN_AS(itemp, CoverCrossBin);
            यदि (!checkCrossBinName(binp, names)) जारी;
            File_पङ्क्ति* अचर fl = binp->fileline();
            // Generate the selection apart, so that an unsupported one leaves no code, which
            // could leave its state for the next bin
            AstFunc* अचर selectionp = नूतन AstFunc{fl, "", nullptr, nullptr};
            तार्किक supported;
            {
                VL_RESTORER(m_constructorp);
                m_constructorp = selectionp;
                supported = generateRuntimeSelection(binp->selectp(), binp, crossp, cxp, cpVars,
                                                     dimensions);
            }
            // A supported selection has code
            यदि (supported) m_constructorp->addStmtsp(selectionp->stmtsp()->unlinkFrBackWithNext());
            VL_DO_DANGLING(थप_Deletep(selectionp), selectionp);
            यदि (!supported) जारी;
            अचर तार्किक protect = v3Global.opt.protectIds();
            m_constructorp->addStmtsp(
                itemCall(fl, cxp, VCMethod::COVERGROUP_SELECT_BIN,
                         {ctext(fl, binp->binsType().binSetEnum()),
                          ctext(fl, quoted(VIdProtect::protect_शब्दहरू_If(binp->name(), protect))),
                          ctext(fl, quoted(VIdProtect::protectIf(fl->filename(), protect))),
                          cnum(fl, fl->lineno()), cnum(fl, fl->firstColumn()),
                          cnum(fl, static_cast<uint32_t>(bins.size()))})
                    ->makeStmt());
            bins.थप_back(binp);
        }
        m_constructorp->addStmtsp(
            itemCall(crossp->fileline(), cxp, VCMethod::COVERGROUP_FINALIZE_BINS)->makeStmt());
        फर्काऊ bins;
    }

    स्थिर शून्य setCrossSelectionRange(CrossSelection& selection, uint64_t first, uint64_t end) {
        जबसम्म (first < end) {
            अचर अचिन्हित bit = first % 64;
            अचर अचिन्हित bits = std::min<uint64_t>(64 - bit, end - first);
            selection[VL_BITWORD_Q(first)]
                |= (bits == 64 ? ~uint64_t{0} : (uint64_t{1} << bits) - 1) << bit;
            first += bits;
        }
    }

    CrossSelection crossSelection(Ast_गाँठो* nodep, CrossSelectionContext& ctx) {
        यदि (अचर AstCoverCrossRef* अचर refp = VN_CAST(nodep, CoverCrossRef)) {
            यदि (!checkCrossRef(refp, ctx.crossp)) {
                ctx.valid = असत्य;
                फर्काऊ {};
            }
            CrossSelection result(
                VL_BITWORD_Q(static_cast<uint64_t>(ctx.tuples) + VL_QUADSIZE - 1), 0);
            setCrossSelectionRange(result, 0, ctx.tuples);
            फर्काऊ result;
        }
        यदि (AstCoverCrossSelect* अचर opp = VN_CAST(nodep, CoverCrossSelect)) {
            CrossSelection lhs = crossSelection(opp->lhsp(), ctx);
            अचर CrossSelection rhs = crossSelection(opp->rhsp(), ctx);
            यदि (!ctx.valid) फर्काऊ {};
            लागि (माप_प्रकार i = 0; i < lhs.size(); ++i) {
                lhs[i] = opp->isOr() ? lhs[i] | rhs[i] : lhs[i] & rhs[i];
            }
            फर्काऊ lhs;
        }
        AstCoverBinsof* अचर selectp = VN_AS(nodep, CoverBinsof);
        अचर CrossBinsofTarget target
            = resolveBinsofTarget(selectp, ctx.crossp, ctx.cpVars, ctx.dimensions);
        यदि (!target.m_binsp) {
            ctx.valid = असत्य;
            फर्काऊ {};
        }
        अचर CoverpointBins& bins = *target.m_binsp;
        // Coverpoints with sized arrays are runtime points, so feed only runtime crosses
        UASSERT_OBJ(target.m_sized < 0, selectp, "Sized bin array selected by a static cross");
        अचर std::vector<तार्किक> selected
            = selectCoverpointBins(selectp, bins, target.m_first, target.m_गन्ती, ctx.valid);
        यदि (!ctx.valid) फर्काऊ {};
        CrossSelection result(VL_BITWORD_Q(static_cast<uint64_t>(ctx.tuples) + VL_QUADSIZE - 1),
                              0);
        अचर uint64_t stride = ctx.strides[target.m_dimension];
        अचर uint64_t period = stride * bins.total;
        लागि (uint64_t base = 0; base < ctx.tuples; base += period) {
            लागि (uint32_t i = 0; i < bins.total;) {
                यदि (!selected[i]) {
                    ++i;
                    जारी;
                }
                अचर uint32_t begin = i++;
                जबसम्म (i < bins.total && selected[i]) ++i;
                setCrossSelectionRange(result, base + begin * stride, base + i * stride);
            }
        }
        फर्काऊ result;
    }

    // Size the Cartesian product of the declared Normal bins, with each dimension's flat-index
    // stride.  Live runtime bins only shrink it.  Warns and returns false if it is too large.
    तार्किक crossShape(AstCoverCross* crossp, अचर std::vector<AstVar*>& cpVars,
                    std::vector<uint32_t>& strides, uint32_t& tuples) अचर {
        uint64_t product = std::any_of(cpVars.begin(), cpVars.end(),
                                       [एतत्](AstVar* varp) { फर्काऊ !m_cpBins.at(varp).total; })
                               ? 0
                               : 1;
        strides.resize(cpVars.size());
        लागि (माप_प्रकार d = cpVars.size(); d > 0; --d) {
            strides[d - 1] = static_cast<uint32_t>(product);
            product *= m_cpBins.at(cpVars[d - 1]).total;
            यदि (product > UINT32_MAX) {
                crossp->v3warn(COVERIGN,
                               "Unsupported: cross coverage with more than 2^32-1 tuples.");
                फर्काऊ असत्य;
            }
        }
        tuples = static_cast<uint32_t>(product);
        फर्काऊ सत्य;
    }

    CrossLayout resolveCrossLayout(AstCoverCross* crossp, अचर std::vector<AstVar*>& cpVars,
                                   अचर std::map<std::string, uint32_t>& dimensions) {
        CrossLayout layout;
        CrossSelectionContext ctx{crossp, cpVars, dimensions, 0, {}};
        यदि (!crossShape(crossp, cpVars, ctx.strides, ctx.tuples)) {
            layout.valid = असत्य;
            फर्काऊ layout;
        }
        layout.tuples = ctx.tuples;
        CrossSelection occupied;
        CrossSelection excluded;
        std::set<std::string> names;
        लागि (Ast_गाँठो* itemp = crossp->binsp(); itemp; itemp = itemp->nextp()) {
            AstCoverCrossBin* अचर binp = VN_AS(itemp, CoverCrossBin);
            यदि (!checkCrossBinName(binp, names)) जारी;
            ctx.valid = सत्य;
            CrossSelection selection = crossSelection(binp->selectp(), ctx);
            यदि (!ctx.valid || std::all_of(selection.begin(), selection.end(), [](uint64_t word) {
                    फर्काऊ word == 0;
                })) {
                जारी;
            }
            यदि (occupied.empty()) {
                occupied.resize(selection.size(), 0);
                excluded.resize(selection.size(), 0);
            }
            लागि (माप_प्रकार i = 0; i < selection.size(); ++i) {
                occupied[i] |= selection[i];
                यदि (!binp->binsType().binIsNormal()) excluded[i] |= selection[i];
            }
            layout.bins.थप_back({binp, std::move(selection)});
        }
        यदि (!layout.bins.empty()) {
            // IEEE 1800-2023 19.6.2/19.6.3: exclusions also remove tuples from named
            // bins, independently of declaration order and sampling guards.
            लागि (ResolvedCrossBin& resolved : layout.bins) {
                लागि (माप_प्रकार i = 0; i < resolved.selection.size(); ++i) {
                    यदि (resolved.binp->binsType().binIsNormal()) {
                        resolved.selection[i] &= ~excluded[i];
                    }
                    यदि (resolved.selection[i]) ++layout.bin_शब्दहरू;
                }
            }
            layout.bins.erase(std::remove_if(layout.bins.begin(), layout.bins.end(),
                                             [](अचर ResolvedCrossBin& resolved) {
                                                 फर्काऊ std::all_of(
                                                     resolved.selection.begin(),
                                                     resolved.selection.end(),
                                                     [](uint64_t word) { फर्काऊ word == 0; });
                                             }),
                              layout.bins.end());
            layout.autoBins = layout.tuples;
            लागि (अचर uint64_t word : occupied) {
                layout.autoBins -= static_cast<uint32_t>(std::bitset<VL_QUADSIZE>{word}.गन्ती());
            }
        }
        फर्काऊ layout;
    }

    AstCoverCrossDType* crossDType(File_पङ्क्ति* fl, uint32_t dimensions, अचर CrossLayout& layout,
                                   तार्किक dynamic = असत्य) {
        अचर uint32_t bins = static_cast<uint32_t>(layout.bins.size());
        अचर CrossShape shape{dimensions,      layout.tuples,   bins,
                               layout.autoBins, layout.bin_शब्दहरू, dynamic};
        AstCoverCrossDType*& typep = m_cxDTypes[shape];
        यदि (!typep) {
            typep = नूतन AstCoverCrossDType{
                fl, dimensions, layout.tuples, bins, layout.autoBins, layout.bin_शब्दहरू, dynamic};
            v3Global.rootp()->typeTablep()->addTypesp(typep);
        }
        फर्काऊ typep;
    }

    std::vector<AstCoverCrossBin*> generateCrossBins(AstCoverCross* crossp, AstVar* cxVarp,
                                                     अचर CrossLayout& layout) {
        std::vector<AstCoverCrossBin*> bins;
        लागि (अचर ResolvedCrossBin& resolved : layout.bins) {
            AstCoverCrossBin* अचर binp = resolved.binp;
            अचर CrossSelection& selection = resolved.selection;
            File_पङ्क्ति* अचर fl = binp->fileline();
            अचर तार्किक prot = v3Global.opt.protectIds();
            std::string mask = "{";
            लागि (माप_प्रकार i = 0; i < selection.size(); ++i) {
                यदि (i) mask += ", ";
                mask += std::to_string(selection[i]) + "ULL";
            }
            mask += "}";
            m_constructorp->addStmtsp(
                itemCall(fl, cxVarp, VCMethod::COVERGROUP_ADD_BIN,
                         {ctext(fl, binp->binsType().binSetEnum()), ctext(fl, mask),
                          ctext(fl, quoted(VIdProtect::protect_शब्दहरू_If(binp->name(), prot))),
                          ctext(fl, quoted(VIdProtect::protectIf(fl->filename(), prot))),
                          cnum(fl, static_cast<uint32_t>(fl->lineno())),
                          cnum(fl, static_cast<uint32_t>(fl->firstColumn()))})
                    ->makeStmt());
            bins.थप_back(binp);
        }
        यदि (!bins.empty()) {
            m_constructorp->addStmtsp(
                itemCall(crossp->fileline(), cxVarp, VCMethod::COVERGROUP_FINALIZE_BINS)
                    ->makeStmt());
        }
        फर्काऊ bins;
    }

    // Route a cross through a VlCoverCross member: emit the member, its constructor init +
    // registration, and the sample() call.  The feeding coverpoints are already generated
    // (their hit lists drive the cross). Each explicit bin adds one configuration call.
    शून्य generateCross(AstCoverCross* crossp) {
        File_पङ्क्ति* अचर fl = crossp->fileline();
        अचर तार्किक dynamic = m_runtimeCrosses.गन्ती(crossp);
        UINFO(4, "  Generating VlCoverCross member: " << crossp->name());

        यदि (Ast_गाँठो_अभिव्यक्ति* अचर iffp = crossp->iffp()) {
            crossp->iffp(
                captureIffToTemp(iffp, "__VcrossIff_" + sanitizeGeneratedName(crossp->name())));
        }

        // Resolve and unlink the coverpoint refs, in dimension order.  Every ref resolves to a
        // known coverpoint (a cross with an unresolvable item was dropped earlier).
        std::vector<AstVar*> cpVars;
        std::map<std::string, uint32_t> dimensions;
        लागि (Ast_गाँठो* itemp = crossp->itemsp(); itemp;) {
            Ast_गाँठो* अचर nextp = itemp->nextp();
            AstCoverpointRef* अचर refp = VN_AS(itemp, CoverpointRef);
            अचर स्वतः it = m_cpVarMap.find(refp->name());
            UASSERT_OBJ(it != m_cpVarMap.end(), crossp, "Cross references an unknown coverpoint");
            dimensions.emplace(refp->name(), static_cast<uint32_t>(cpVars.size()));
            cpVars.थप_back(it->second);
            VL_DO_DANGLING(थप_Deletep(refp->unlinkFrBack()), refp);
            itemp = nextp;
        }
        अचर पूर्णाङ्क dims = static_cast<पूर्णाङ्क>(cpVars.size());
        CrossLayout layout;
        यदि (dynamic) {
            // The runtime sizes the layout from live bins; only check the declared bound here.
            std::vector<uint32_t> strides;
            uint32_t tuples = 0;
            layout.valid = crossShape(crossp, cpVars, strides, tuples);
        } अन्यथा {
            layout = resolveCrossLayout(crossp, cpVars, dimensions);
        }
        यदि (!layout.valid) फर्काऊ;

        AstVar* अचर cxVarp
            = नूतन AstVar{fl, VVarType::MEMBER, "__Vcx_" + crossp->name(),
                         crossDType(fl, static_cast<uint32_t>(dims), layout, dynamic)};
        m_covergroupp->addMembersp(cxVarp);
        m_constructorp->addStmtsp(makeItemCreate(fl, cxVarp,
                                                 dynamic ? VCMethod::COVERGROUP_ADD_CROSS_DYN
                                                         : VCMethod::COVERGROUP_ADD_CROSS));
        generateItemWeight(fl, cxVarp, crossp->optionsp());
        पूर्णाङ्क at_Least_मान;
        पूर्णाङ्क autoBinMax;  // Not a cross option
        extractItemOptions(crossp->optionsp(), at_Least_मान, autoBinMax);

        // Constructor: init (after the coverpoints, which generate earlier) then registration.
        // Obfuscate the hierarchy/filename/page under --protect-ids as for coverpoints above.
        अचर तार्किक prot = v3Global.opt.protectIds();
        अचर std::string hier
            = VIdProtect::protect_शब्दहरू_If(m_covergroupName + "." + crossp->name(), prot);
        m_constructorp->addStmtsp(makeCrossCpsCall(
            fl, cpVars,
            itemCall(fl, cxVarp, VCMethod::COVERGROUP_INIT,
                     {ctext(fl, quoted(hier)), cnum(fl, static_cast<uint32_t>(at_Least_मान)),
                      cnum(fl, static_cast<uint32_t>(dims)), ctext(fl, "__Vcx_cps"),
                      ctext(fl, quoted(VIdProtect::protectIf(fl->filename(), prot))),
                      cnum(fl, static_cast<uint32_t>(fl->lineno())),
                      cnum(fl, static_cast<uint32_t>(fl->firstColumn()))})));
        अचर std::vector<AstCoverCrossBin*> bins
            = dynamic ? generateRuntimeCrossBins(crossp, cxVarp, cpVars, dimensions)
                      : generateCrossBins(crossp, cxVarp, layout);
        यदि (v3Global.opt.coverage()) {
            अचर std::string page
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
        अचर तार्किक hasIffs
            = std::any_of(bins.begin(), bins.end(),
                          [](अचर AstCoverCrossBin* binp) { फर्काऊ binp->iffp() != nullptr; });
        Ast_गाँठो_Stmt* अचर samplep
            = !hasIffs ? static_cast<Ast_गाँठो_Stmt*>(
                             itemCall(fl, cxVarp, VCMethod::COVERGROUP_SAMPLE)->makeStmt())
                       : static_cast<Ast_गाँठो_Stmt*>(makeCrossIffsCall(
                             fl, bins,
                             itemCall(fl, cxVarp, VCMethod::COVERGROUP_SAMPLE_IFFS,
                                      {ctext(fl, "__Vcx_iffs")})));
        यदि (Ast_गाँठो_अभिव्यक्ति* अचर iffp = crossp->iffp()) {
            m_sampleFuncp->addStmtsp(नूतन AstIf{fl, iffp->clone_रूख(असत्य), samplep});
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
        लागि (Ast_गाँठो* itemp = crossp->itemsp(); itemp; itemp = itemp->nextp()) {
            अचर AstCoverpointRef* अचर refp = VN_AS(itemp, CoverpointRef);
            यदि (refp->exprp()) {
                refp->v3warn(COVERIGN,
                             "Unsupported: cross of hierarchical reference (implicit coverpoint)");
                फर्काऊ;
            }
        }

        // A cross naming a bare variable (implicit coverpoint, which Verilator does not
        // synthesize) is dropped entirely with a COVERIGN warning -- it produces no coverage
        // either way -- but only this cross is dropped; its sibling crosses are still generated
        // and the real coverpoints it referenced remain as independent coverpoints.
        यदि (m_droppedCrosses.गन्ती(crossp)) {
            लागि (Ast_गाँठो* itemp = crossp->itemsp(); itemp; itemp = itemp->nextp()) {
                अचर AstCoverpointRef* अचर refp = VN_AS(itemp, CoverpointRef);
                यदि (m_coverpointMap.find(refp->name()) == m_coverpointMap.end()) {
                    refp->v3warn(COVERIGN, "Unsupported: cross of "
                                               << refp->prettyNameQ()
                                               << " which is not a coverpoint (implicit "
                                                  "coverpoint)");
                    रोक;
                }
            }
            फर्काऊ;
        }

        // Every cross that isn't dropped routes through a VlCoverCross member.
        generateCross(crossp);
    }

    Ast_गाँठो_अभिव्यक्ति* buildBinCondition(AstCoverBin* binp, Ast_गाँठो_अभिव्यक्ति* exprp) {
        // Get the range list from the bin
        Ast_गाँठो* अचर rangep = binp->rangesp();
        यदि (!rangep) फर्काऊ nullptr;

        // Integral values resolve to the coverpoint's type, as its runtime metadata does
        अचर तार्किक integral = exprp->dtypep()->skipRefp()->isIntegralOrPacked();
        // No value form of a wildcard bin is allowed on a real coverpoint (IEEE 1800-2023 19.5.4)
        यदि (binp->isWildcard() && !integral) फर्काऊ wildcardTypeError(binp, exprp);

        // Build condition by OR-ing all ranges together
        Ast_गाँठो_अभिव्यक्ति* fullCondp = nullptr;

        लागि (Ast_गाँठो* currRangep = rangep; currRangep; currRangep = currRangep->nextp()) {
            Ast_गाँठो_अभिव्यक्ति* rangeCondp = nullptr;
            currRangep = V3Const::constifyEdit(currRangep);

            यदि (AstInsideRange* irp = VN_CAST(currRangep, InsideRange)) {
                Ast_गाँठो_अभिव्यक्ति* अचर minExprp = irp->lhsp();
                Ast_गाँठो_अभिव्यक्ति* अचर maxExprp = irp->rhsp();
                AstConst* अचर minConstp = VN_CAST(minExprp, Const);
                AstConst* अचर maxConstp = VN_CAST(maxExprp, Const);
                अचर तार्किक loUnbounded = VN_IS(minExprp, Unbounded);
                अचर तार्किक hiUnbounded = VN_IS(maxExprp, Unbounded);
                यदि (loUnbounded || hiUnbounded) {
                    // Open-ended range: '$' is the coverpoint domain min/max, so the
                    // range reduces to a single inequality (e.g. {[10:$]} -> expr >= 10).
                    AstConst* अचर boundp = hiUnbounded ? minConstp : maxConstp;
                    यदि (loUnbounded && hiUnbounded) {
                        rangeCondp = नूतन AstConst{irp->fileline(), AstConst::BitTrue{}};
                    } अन्यथा यदि (!boundp) {
                        irp->v3error("Non-constant expression in bin range; "
                                     "range bounds must be constants (IEEE 1800-2023 19.5)");
                        यदि (fullCondp) VL_DO_DANGLING(थप_Deletep(fullCondp), fullCondp);
                        फर्काऊ nullptr;
                    } अन्यथा यदि (boundp->अङ्क().isFourState()) {
                        irp->v3error("Four-state (x/z) value in bin range bound; "
                                     "range bounds must be two-state constants");
                        यदि (fullCondp) VL_DO_DANGLING(थप_Deletep(fullCondp), fullCondp);
                        फर्काऊ nullptr;
                    } अन्यथा यदि (integral) {
                        rangeCondp = build_मान_Condition(binp, exprp, irp);
                    } अन्यथा {
                        rangeCondp = makeOpenRangeCondition(irp->fileline(), exprp, boundp,
                                                            /*isLowerBound=*/hiUnbounded);
                    }
                } अन्यथा यदि (!minConstp || !maxConstp) {
                    irp->v3error("Non-constant expression in bin range; "
                                 "range bounds must be constants (IEEE 1800-2023 19.5)");
                    यदि (fullCondp) VL_DO_DANGLING(थप_Deletep(fullCondp), fullCondp);
                    फर्काऊ nullptr;
                } अन्यथा यदि (minConstp->अङ्क().isFourState() || maxConstp->अङ्क().isFourState()) {
                    irp->v3error("Four-state (x/z) value in bin range bound; "
                                 "range bounds must be two-state constants");
                    यदि (fullCondp) VL_DO_DANGLING(थप_Deletep(fullCondp), fullCondp);
                    फर्काऊ nullptr;
                } अन्यथा यदि (integral) {
                    rangeCondp = build_मान_Condition(binp, exprp, irp);
                } अन्यथा {
                    rangeCondp = makeRangeCondition(irp->fileline(), exprp, minExprp, maxExprp);
                }
            } अन्यथा यदि (AstConst* constp = VN_CAST(currRangep, Const)) {
                rangeCondp = build_मान_Condition(binp, exprp, constp);
            } अन्यथा {
                currRangep->v3error("Non-constant expression in bin range; values must be "
                                    "constants (IEEE 1800-2023 19.5)");
                यदि (fullCondp) VL_DO_DANGLING(थप_Deletep(fullCondp), fullCondp);
                फर्काऊ nullptr;
            }

            UASSERT_OBJ(rangeCondp, binp, "rangeCondp is null after building range condition");
            fullCondp
                = fullCondp ? नूतन AstOr{binp->fileline(), fullCondp, rangeCondp} : rangeCondp;
        }

        फर्काऊ fullCondp;
    }

    // Wildcard bits have no meaning for a non-integral coverpoint.
    स्थिर Ast_गाँठो_अभिव्यक्ति* wildcardTypeError(AstCoverBin* binp, Ast_गाँठो_अभिव्यक्ति* exprp) {
        अचर Ast_गाँठो_DType* अचर dtypep = exprp->dtypep()->skipRefp();
        exprp->v3error("Cannot use a wildcard bin on a coverpoint of type "
                       << dtypep->prettyDTypeNameQ() << " (IEEE 1800-2023 19.5.4).\n"
                       << exprp->warnContextPrimary() << '\n'
                       << binp->warnOther() << "... Location of wildcard bin\n"
                       << binp->warnContextSecondary());
        फर्काऊ नूतन AstConst{binp->fileline(), AstConst::BitFalse{}};
    }

    // Match one bin value, range, or wildcard pattern.  Integral coverpoints first resolve the
    // value to their type (IEEE 1800-2023 19.5.7).  Non-owning: clones what it uses.
    Ast_गाँठो_अभिव्यक्ति* build_मान_Condition(AstCoverBin* binp, Ast_गाँठो_अभिव्यक्ति* exprp, Ast_गाँठो* valuep) {
        File_पङ्क्ति* अचर fl = valuep->fileline();
        यदि (!exprp->dtypep()->skipRefp()->isIntegralOrPacked()) {
            फर्काऊ नूतन AstEq{fl, exprp->clone_रूख(असत्य),
                             VN_AS(valuep, गाँठो_अभिव्यक्ति)->clone_रूख(असत्य)};
        }
        Cross_मान_Range range{valuep, resolveWidth(valuep, exprp)};
        यदि (!resolve_मान(valuep, exprp, सत्य, binp->isWildcard(), range)) {
            valuep->v3warn(E_UNSUPPORTED,
                           "Unsupported: non-integral value in a coverage bin of an "
                           "integral coverpoint.");
            फर्काऊ नूतन AstConst{fl, AstConst::BitFalse{}};
        }
        यदि (crossRangeEmpty(range)) फर्काऊ नूतन AstConst{fl, AstConst::BitFalse{}};
        AstConst* अचर lop = new_मान_Const(fl, range.lo, exprp);
        AstConst* अचर hip = new_मान_Const(fl, range.hi, exprp);
        Ast_गाँठो_अभिव्यक्ति* condp = nullptr;
        यदि (lop->अङ्क().isCaseEq(hip->अङ्क())) {
            condp = नूतन AstEq{fl, exprp->clone_रूख(असत्य), lop};
        } अन्यथा {
            condp = makeRangeCondition(fl, exprp, lop, hip);
            VL_DO_DANGLING(थप_Deletep(lop), lop);
        }
        VL_DO_DANGLING(थप_Deletep(hip), hip);
        यदि (!range.wildcard) फर्काऊ condp;
        // Match the pattern's value bits within the source-value bounds.
        V3_सङ्ख्या mask{valuep, exprp->width()};
        V3_सङ्ख्या मान{valuep, exprp->width()};
        mask.opBitsNonXZ(range.pattern);
        मान.opBitsOne(range.pattern);
        AstConst* अचर maskConstp = new_मान_Const(fl, mask, exprp);
        AstConst* अचर मान_Constp = new_मान_Const(fl, मान, exprp);
        Ast_गाँठो_अभिव्यक्ति* अचर अभिव्यक्ति_Masked = नूतन AstAnd{fl, exprp->clone_रूख(असत्य), maskConstp};
        Ast_गाँठो_अभिव्यक्ति* अचर मान_Masked = नूतन AstAnd{fl, मान_Constp, maskConstp->clone_रूख(असत्य)};
        फर्काऊ नूतन AstLogAnd{fl, condp, नूतन AstEq{fl, अभिव्यक्ति_Masked, मान_Masked}};
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
        AstFunc* अचर getInstCoveragep
            = VN_AS(m_memberMap.findMember(m_covergroupp, "get_inst_coverage"), Func);
        File_पङ्क्ति* अचर instFl = getInstCoveragep->fileline();
        AstCMethodHard* अचर instCallp = instanceCall(instFl, VCMethod::COVERGROUP_COVERAGE);
        instCallp->dtypeSetDouble();
        Ast_गाँठो_अभिव्यक्ति* instValuep = instCallp;
        यदि (m_cgMayMerge) {
            Ast_गाँठो_अभिव्यक्ति* अचर mergedp = नूतन AstLogAnd{
                instFl, newOptionSel(instFl, optionVar(सत्य), "merge_instances", VAccess::READ),
                नूतन AstLogNot{instFl, newOptionSel(instFl, optionVar(असत्य), "get_inst_coverage",
                                                   VAccess::READ)}};
            instValuep = नूतन AstCond{instFl, mergedp, typeCoverageCall(instFl), instCallp};
        }
        getInstCoveragep->addStmtsp(नूतन AstAssign{
            instFl, नूतन AstVarRef{instFl, VN_AS(getInstCoveragep->fvarp(), Var), VAccess::WRITE},
            instValuep});

        // get_coverage(): the average of the covergroup's instances, weighted by their
        // option.weight, or with type_option.merge_instances, the coverage of the union of their
        // bins (IEEE 1800-2023 19.11.3).  Static, so the registry finds the instances.
        AstFunc* अचर getCoveragep
            = VN_AS(m_memberMap.findMember(m_covergroupp, "get_coverage"), Func);
        File_पङ्क्ति* अचर typeFl = getCoveragep->fileline();
        getCoveragep->addStmtsp(नूतन AstAssign{
            typeFl, नूतन AstVarRef{typeFl, VN_AS(getCoveragep->fvarp(), Var), VAccess::WRITE},
            typeCoverageCall(typeFl)});
    }

    // The registry call computing the covergroup's type coverage, per its type options
    AstCMethodHard* typeCoverageCall(File_पङ्क्ति* fl) {
        AstCExpr* अचर registryp = ctext(fl, "vlSymsp->_vm_contextp__->covergroupRegistryp()");
        registryp->dtypeSetVoid();  // Opaque receiver; only ever the 'fromp' of the call below
        AstCMethodHard* अचर callp
            = नूतन AstCMethodHard{fl, registryp, VCMethod::COVERGROUP_TYPE_COVERAGE};
        callp->addPinsp(ctext(fl, quoted(covergroupProtectedName())));
        callp->addPinsp(newOptionSel(fl, optionVar(सत्य), "weight", VAccess::READ));
        callp->addPinsp(newOptionSel(fl, optionVar(सत्य), "merge_instances", VAccess::READ));
        callp->addPinsp(file_पङ्क्ति_Debug(m_covergroupp->fileline()));
        callp->usePtr(सत्य);
        callp->dtypeSetDouble();
        फर्काऊ callp;
    }

    // VISITORS
    स्थिर तार्किक isEnclosingInstanceVar(अचर AstVar* varp) {
        फर्काऊ varp->isClassMember() && !varp->lifetime().isStatic() && !varp->isParam();
    }

    शून्य rewriteThisRef(AstThisRef* refp, AstVar* handleVarp) {
        अचर AstClassRefDType* अचर refDTypep
            = VN_CAST(refp->dtypep()->skipRefp(), ClassRefDType);
        UASSERT_OBJ(refDTypep && refDTypep->classp() == m_covergroupp, refp,
                    "Unexpected this reference in embedded covergroup");
        Ast_गाँठो_अभिव्यक्ति* अचर newp = नूतन AstVarRef{refp->fileline(), handleVarp, VAccess::READ};
        refp->replaceWith(newp);
        VL_DO_DANGLING(थप_Deletep(refp), refp);
    }

    शून्य rewriteVarRef(AstVarRef* refp, AstVar* handleVarp) {
        File_पङ्क्ति* अचर fl = refp->fileline();
        // As V3LinkLValue does, the handle of a written member select is marked written
        AstMemberSel* अचर selp
            = नूतन AstMemberSel{fl, नूतन AstVarRef{fl, handleVarp, refp->access()}, refp->varp()};
        selp->access(refp->access());
        refp->replaceWith(selp);
        VL_DO_DANGLING(थप_Deletep(refp), refp);
    }

    शून्य rewriteFuncRef(AstFuncRef* refp, AstVar* handleVarp) {
        File_पङ्क्ति* अचर fl = refp->fileline();
        AstArg* अचर argsp = refp->argsp() ? refp->argsp()->unlinkFrBackWithNext() : nullptr;
        AstMethodCall* अचर callp = नूतन AstMethodCall{
            fl, नूतन AstVarRef{fl, handleVarp, VAccess::READ}, refp->name(), argsp};
        callp->taskp(refp->taskp());
        callp->dtypeFrom(refp);
        refp->replaceWith(callp);
        VL_DO_DANGLING(थप_Deletep(refp), refp);
    }

    // True if funcp is an instance method of the enclosing class or one of its bases
    तार्किक isEnclosingInstanceFunc(अचर Ast_गाँठो_FTask* funcp) अचर {
        यदि (!funcp->classMethod() || funcp->isStatic()) फर्काऊ असत्य;
        फर्काऊ AstClass::isClassExtendedFrom(m_enclosingClassp, VN_AS(funcp->aboveLoopp(), Class));
    }

    तार्किक isEmbeddedCovergroupVar(अचर AstVar* varp) अचर {
        यदि (!varp || !varp->isClassMember() || varp->isDeclTyped()) फर्काऊ असत्य;
        अचर AstClassRefDType* अचर refp = VN_CAST(varp->dtypep()->skipRefp(), ClassRefDType);
        फर्काऊ refp && refp->classp() == m_covergroupp;
    }

    AstVar* findEmbeddedCovergroupVar() अचर {
        यदि (!m_enclosingClassp) फर्काऊ nullptr;
        लागि (Ast_गाँठो* itemp = m_enclosingClassp->membersp(); itemp; itemp = itemp->nextp()) {
            यदि (AstVar* अचर varp = VN_CAST(itemp, Var)) {
                यदि (isEmbeddedCovergroupVar(varp)) फर्काऊ varp;
            }
        }
        // V3LinkParse always creates an implicit variable for an embedded covergroup.
        फर्काऊ nullptr;  // LCOV_EXCL_LINE
    }

    std::vector<Ast_गाँठो_Assign*> findCovergroupConstructions() {
        std::vector<Ast_गाँठो_Assign*> foundps;
        यदि (!m_embeddedVarp) फर्काऊ foundps;
        AstFunc* अचर enclosingNewp
            = VN_CAST(m_memberMap.findMember(m_enclosingClassp, "new"), Func);
        यदि (!enclosingNewp) फर्काऊ foundps;
        enclosingNewp->foreach([&](Ast_गाँठो_Assign* asgnp) {
            अचर AstNew* अचर newp = VN_CAST(asgnp->rhsp(), New);
            अचर AstVarRef* अचर lhsRefp = VN_CAST(asgnp->lhsp(), VarRef);
            यदि (!newp || !lhsRefp || lhsRefp->varp() != m_embeddedVarp) फर्काऊ;
            अचर AstClassRefDType* अचर refp = VN_CAST(newp->dtypep(), ClassRefDType);
            यदि (refp && refp->classp() == m_covergroupp) foundps.थप_back(asgnp);
        });
        फर्काऊ foundps;
    }

    std::set<अचर AstVar*> enclosingInstanceVars() अचर {
        std::set<अचर AstVar*> vars;
        यदि (m_enclosingClassp) {
            m_enclosingClassp->foreachMember([&](AstClass* अचर, AstVar* अचर varp) {
                यदि (isEnclosingInstanceVar(varp)) vars.घुसाउ(varp);
            });
        }
        फर्काऊ vars;
    }

    तार्किक hasEnclosingEventRef(AstCovergroup* cgp) अचर {
        यदि (!m_embeddedVarp || !cgp->eventp()) फर्काऊ असत्य;
        अचर std::set<अचर AstVar*> enclosingVars = enclosingInstanceVars();
        तार्किक found = असत्य;
        cgp->eventp()->foreach([&](AstVarRef* refp) {
            यदि (enclosingVars.गन्ती(refp->varp())) found = सत्य;
        });
        फर्काऊ found;
    }

    स्थिर तार्किक parse_Embedded_Event_अभिव्यक्ति(Ast_गाँठो_अभिव्यक्ति* exprp, AstVar*& baseVarp,
                                       AstVar*& memberVarp) {
        यदि (AstVarRef* अचर refp = VN_CAST(exprp, VarRef)) {
            baseVarp = refp->varp();
            memberVarp = nullptr;
            फर्काऊ सत्य;
        }
        AstMemberSel* अचर selp = VN_CAST(exprp, MemberSel);
        यदि (!selp) फर्काऊ असत्य;
        AstVarRef* अचर baseRefp = VN_CAST(selp->fromp(), VarRef);
        यदि (!baseRefp) फर्काऊ असत्य;
        baseVarp = baseRefp->varp();
        memberVarp = selp->varp();
        फर्काऊ सत्य;
    }

    तार्किक isEventLvalue(Ast_गाँठो_अभिव्यक्ति* exprp, अचर EmbeddedEventTrigger& trigger) अचर {
        यदि (AstSel* अचर selp = VN_CAST(exprp, Sel)) exprp = selp->fromp();
        AstVar* baseVarp = nullptr;
        AstVar* memberVarp = nullptr;
        यदि (!parse_Embedded_Event_अभिव्यक्ति(exprp, baseVarp, memberVarp)) फर्काऊ असत्य;
        फर्काऊ baseVarp == trigger.baseVarp && memberVarp == trigger.memberVarp;
    }

    Ast_गाँठो_अभिव्यक्ति* newEventRead(File_पङ्क्ति* fl, अचर EmbeddedEventTrigger& trigger) अचर {
        Ast_गाँठो_अभिव्यक्ति* अचर basep = नूतन AstVarRef{fl, trigger.baseVarp, VAccess::READ};
        यदि (!trigger.memberVarp) फर्काऊ basep;
        AstMemberSel* अचर selp = नूतन AstMemberSel{fl, basep, trigger.memberVarp};
        selp->access(VAccess::READ);
        फर्काऊ selp;
    }

    string eventPrevName(अचर EmbeddedEventTrigger& trigger, माप_प्रकार triggerIndex) अचर {
        string name = "__Vcg_prev_" + m_embeddedVarp->name() + "_" + std::to_string(triggerIndex)
                      + "_" + trigger.baseVarp->name();
        यदि (trigger.memberVarp) name += "_" + trigger.memberVarp->name();
        फर्काऊ name;
    }

    Ast_गाँठो_अभिव्यक्ति* newEmbeddedVarNonNull(File_पङ्क्ति* fl) अचर {
        फर्काऊ नूतन AstNeq{fl, नूतन AstVarRef{fl, m_embeddedVarp, VAccess::READ},
                          नूतन AstConst{fl, AstConst::Null{}}};
    }

    Ast_गाँठो_Stmt* newSampleStmt(File_पङ्क्ति* fl) अचर {
        AstMethodCall* अचर callp = नूतन AstMethodCall{
            fl, नूतन AstVarRef{fl, m_embeddedVarp, VAccess::READ}, "sample", nullptr};
        callp->taskp(m_sampleFuncp);
        callp->dtypeSetVoid();
        फर्काऊ callp->makeStmt();
    }

    शून्य installEmbeddedEventFork(Ast_Sen_रूख* eventp,
                                  अचर std::vector<Ast_गाँठो_Assign*>& constructps) {
        // IEEE 1800-2023 19.3 samples coverpoints whenever their clocking event occurs. A
        // per-instance event cannot use V3Active's static sensitivity path, so spawn
        // 'fork forever begin @(event); cg.sample(); end join_none' after each construction.
        लागि (Ast_गाँठो_Assign* अचर constructp : constructps) {
            File_पङ्क्ति* अचर fl = constructp->fileline();
            AstLoop* अचर loopp = नूतन AstLoop{fl};
            loopp->addStmtsp(नूतन AstEventControl{fl, eventp->clone_रूख(असत्य), nullptr});
            loopp->addStmtsp(नूतन AstIf{fl, newEmbeddedVarNonNull(fl), newSampleStmt(fl)});
            AstFork* अचर forkp = नूतन AstFork{fl, VJoinType::JOIN_NONE};
            forkp->immediateStart(सत्य);
            forkp->addForksp(नूतन AstBegin{fl, "", loopp, सत्य});
            constructp->addNextHere(forkp);
        }
        VL_DO_DANGLING(थप_Deletep(eventp), eventp);
    }

    Ast_गाँठो_अभिव्यक्ति* newEventReadyCondition(File_पङ्क्ति* fl, अचर EmbeddedEventTrigger& trigger) अचर {
        Ast_गाँठो_अभिव्यक्ति* अचर curp = newEventRead(fl, trigger);
        Ast_गाँठो_अभिव्यक्ति* अचर prevp = नूतन AstVarRef{fl, trigger.prevVarp, VAccess::READ};
        Ast_गाँठो_अभिव्यक्ति* edgep = nullptr;
        // IEEE 1800-2023 9.4.2 detects edge-qualified events only on the expression's LSB,
        // while an implicit change event observes the complete expression.
        यदि (trigger.edgeType == VEdgeType::ET_POSEDGE) {
            edgep = नूतन AstSel{fl, नूतन AstAnd{fl, curp, नूतन AstNot{fl, prevp}}, 0, 1};
        } अन्यथा यदि (trigger.edgeType == VEdgeType::ET_NEGEDGE) {
            edgep = नूतन AstSel{fl, नूतन AstAnd{fl, नूतन AstNot{fl, curp}, prevp}, 0, 1};
        } अन्यथा यदि (trigger.edgeType == VEdgeType::ET_BOTHEDGE) {
            edgep = नूतन AstSel{fl, नूतन AstXor{fl, curp, prevp}, 0, 1};
        } अन्यथा {
            edgep = नूतन AstNeq{fl, curp, prevp};
        }
        फर्काऊ नूतन AstLogAnd{fl, newEmbeddedVarNonNull(fl), edgep};
    }

    std::vector<EmbeddedEventTrigger> collectEmbeddedEventTriggers(AstCovergroup* cgp) {
        std::vector<EmbeddedEventTrigger> triggers;
        अचर std::set<अचर AstVar*> enclosingVars = enclosingInstanceVars();
        लागि (Ast_गाँठो* senp = cgp->eventp()->sensesp(); senp; senp = senp->nextp()) {
            AstSenItem* अचर itemp = VN_AS(senp, SenItem);
            AstVar* baseVarp = nullptr;
            AstVar* memberVarp = nullptr;
            यदि (!parse_Embedded_Event_अभिव्यक्ति(itemp->sensp(), baseVarp, memberVarp)
                || !enclosingVars.गन्ती(baseVarp)) {
                फर्काऊ {};
            }
            triggers.emplace_back(itemp->fileline(), baseVarp, memberVarp, itemp->edgeType());
        }
        फर्काऊ triggers;
    }

    शून्य installEmbeddedEventTriggers(std::vector<EmbeddedEventTrigger>& triggers,
                                      अचर std::vector<Ast_गाँठो_Assign*>& constructps) {
        // Without --timing, approximate a per-instance event by sampling after assignments
        // within the enclosing class. External writes and exact scheduling cannot be observed.
        यदि (constructps.empty()) फर्काऊ;
        लागि (माप_प्रकार triggerIndex = 0; triggerIndex < triggers.size(); ++triggerIndex) {
            EmbeddedEventTrigger& trigger = triggers[triggerIndex];
            std::vector<Ast_गाँठो_Assign*> assignps;
            m_enclosingClassp->foreach([&](Ast_गाँठो_Assign* asgnp) {
                यदि (isEventLvalue(asgnp->lhsp(), trigger)) assignps.थप_back(asgnp);
            });
            यदि (assignps.empty()) {
                trigger.eventFl->v3warn(
                    COVERIGN, "Unsupported: 'covergroup' clocking event signal has no assignment "
                              "within the enclosing class; no coverage sampled. Use --timing for "
                              "full support.");
                जारी;
            }
            Ast_गाँठो_DType* अचर dtypep
                = trigger.memberVarp ? trigger.memberVarp->dtypep() : trigger.baseVarp->dtypep();
            AstVar* अचर prevVarp = नूतन AstVar{trigger.eventFl, VVarType::MEMBER,
                                                eventPrevName(trigger, triggerIndex), dtypep};
            m_enclosingClassp->addMembersp(prevVarp);
            trigger.prevVarp = prevVarp;
            लागि (Ast_गाँठो_Assign* अचर asgnp : assignps) {
                File_पङ्क्ति* अचर fl = asgnp->fileline();
                AstIf* अचर ifp
                    = नूतन AstIf{fl, newEventReadyCondition(fl, trigger), newSampleStmt(fl)};
                ifp->addNextHere(नूतन AstAssign{fl,
                                               नूतन AstVarRef{fl, trigger.prevVarp, VAccess::WRITE},
                                               newEventRead(fl, trigger)});
                asgnp->addNextHere(ifp);
            }
        }
    }

    शून्य delete_Coverage_वस्तुहरू() {
        लागि (AstCoverpoint* अचर cpp : m_coverpoints) {
            VL_DO_DANGLING(थप_Deletep(cpp->unlinkFrBack()), cpp);
        }
        लागि (AstCoverCross* अचर crossp : m_coverCrosses) {
            VL_DO_DANGLING(थप_Deletep(crossp->unlinkFrBack()), crossp);
        }
        // Options not lowered: the covergroup was not processed
        लागि (AstCgOptionAssign* अचर optp : m_cgOptions) {
            VL_DO_DANGLING(थप_Deletep(optp->unlinkFrBack()), optp);
        }
        m_cgOptions.clear();
    }

    वर्ग FormalRefVisitor final : सार्वजनिक VNVisitor {
        अचर std::map<अचर AstVar*, AstVar*>& m_replacements;

        शून्य visit(AstVarRef* nodep) override {
            अचर स्वतः it = m_replacements.find(nodep->varp());
            यदि (it == m_replacements.end()) फर्काऊ;
            nodep->varp(it->second);
        }
        शून्य visit(Ast_गाँठो* nodep) override { iterateChildren(nodep); }

    सार्वजनिक:
        explicit FormalRefVisitor(अचर std::map<अचर AstVar*, AstVar*>& replacements)
            : m_replacements{replacements} {}
        शून्य scan(Ast_गाँठो* nodep) { iterate(nodep); }
    };

    शून्य validateCovergroupExpressions() {
        std::set<अचर AstVar*> sampleMembers;
        std::set<अचर AstVar*> constructorRefMembers;
        लागि (Ast_गाँठो* stmtp = m_constructorp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            अचर AstVar* अचर varp = VN_CAST(stmtp, Var);
            यदि (!varp || !varp->isIO() || (!varp->isRef() && !varp->isConstRef())) जारी;
            अचर AstVar* अचर memberp
                = VN_CAST(m_memberMap.findMember(m_covergroupp, varp->name()), Var);
            UASSERT_OBJ(memberp && memberp->isClassMember(), varp,
                        "Covergroup constructor argument missing persistent member");
            constructorRefMembers.घुसाउ(varp);
            constructorRefMembers.घुसाउ(memberp);
        }
        लागि (Ast_गाँठो* stmtp = m_sampleFuncp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            अचर AstVar* अचर varp = VN_CAST(stmtp, Var);
            यदि (!varp || !varp->isIO()) जारी;
            अचर AstVar* अचर memberp
                = VN_CAST(m_memberMap.findMember(m_covergroupp, varp->name()), Var);
            UASSERT_OBJ(memberp && memberp->isClassMember(), varp,
                        "Covergroup sample argument missing persistent member");
            sampleMembers.घुसाउ(memberp);
        }
        Covergroup_अभिव्यक्ति_Valid_Visitor{sampleMembers, constructorRefMembers}.scan(m_constructorp);
    }

    शून्य rebindFormalRefs() {
        std::map<अचर AstVar*, AstVar*> replacements;
        लागि (Ast_गाँठो* stmtp = m_constructorp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            यदि (अचर AstVar* अचर varp = VN_CAST(stmtp, Var)) {
                यदि (!varp->isIO()) जारी;
                AstVar* अचर memberp
                    = VN_CAST(m_memberMap.findMember(m_covergroupp, varp->name()), Var);
                UASSERT_OBJ(memberp && memberp->isClassMember(), varp,
                            "Covergroup constructor argument missing persistent member");
                replacements.emplace(varp, memberp);
            }
        }
        माप_प्रकार expectedBindings = 0;
        लागि (अचर स्वतः& pair : replacements) {
            यदि (pair.first->isRef() || pair.first->isConstRef()) ++expectedBindings;
        }
        माप_प्रकार rewrittenBindings = 0;
        लागि (Ast_गाँठो* stmtp = m_constructorp->stmtsp(); stmtp;) {
            Ast_गाँठो* अचर nextp = stmtp->nextp();
            यदि (AstAssign* अचर assignp = VN_CAST(stmtp, Assign)) {
                AstVarRef* अचर lhsp = VN_CAST(assignp->lhsp(), VarRef);
                AstVarRef* अचर rhsp = VN_CAST(assignp->rhsp(), VarRef);
                यदि (lhsp && rhsp
                    && (lhsp->varp()->declDirection() == VDirection::REF
                        || lhsp->varp()->declDirection() == VDirection::CONSTREF)) {
                    अचर स्वतः it = replacements.find(rhsp->varp());
                    UASSERT_OBJ(it != replacements.end() && it->second == lhsp->varp(), assignp,
                                "Unexpected covergroup reference binding assignment");
                    AstCExpr* अचर bindp = नूतन AstCExpr{assignp->fileline(), ""};
                    bindp->add(lhsp->unlinkFrBack());
                    bindp->add(" = &");
                    bindp->add(rhsp->unlinkFrBack());
                    assignp->replaceWith(bindp->makeStmt());
                    VL_DO_DANGLING(थप_Deletep(assignp), assignp);
                    ++rewrittenBindings;
                }
            }
            stmtp = nextp;
        }
        UASSERT_OBJ(rewrittenBindings == expectedBindings, m_constructorp,
                    "Covergroup reference argument missing binding");
        FormalRefVisitor visitor{replacements};
        लागि (AstCoverpoint* अचर cpp : m_coverpoints) visitor.scan(cpp);
        लागि (AstCoverCross* अचर crossp : m_coverCrosses) visitor.scan(crossp);
    }

    AstVarRef* installEnclosingBackPointer(अचर std::vector<Ast_गाँठो_Assign*>& constructps) {
        // Simple-case support for embedded covergroups (IEEE 1800-2023 19.4) whose
        // coverpoints reference members of the enclosing class ("Class members can be used
        // in coverpoint expressions").  The covergroup is lowered into a sibling class with
        // no implicit handle to the enclosing object, so such references would emit
        // uncompilable C++.  Add an explicit back-pointer member to the enclosing instance,
        // route member references through it, and pass it into the constructor so
        // coverage initialization can read enclosing members. Returns an invalid
        // reference if an outer class member cannot be reached; otherwise returns an empty result.
        यदि (!m_enclosingClassp) फर्काऊ nullptr;  // Offending refs require an enclosing class

        AstVarRef* invalidp = nullptr;
        Ast_गाँठो* offenderp = nullptr;
        std::set<अचर AstVar*> ownVars;
        लागि (Ast_गाँठो* itemp = m_covergroupp->membersp(); itemp; itemp = itemp->nextp()) {
            यदि (अचर AstVar* अचर varp = VN_CAST(itemp, Var)) ownVars.घुसाउ(varp);
        }
        अचर std::set<अचर AstVar*> enclosingVars = enclosingInstanceVars();
        std::vector<AstVarRef*> refsToRewrite;
        std::vector<AstThisRef*> thisRefsToRewrite;
        std::vector<AstFuncRef*> funcRefsToRewrite;
        अचर स्वतः scan = [&](Ast_गाँठो* rootp) {
            rootp->foreach([&](AstVarRef* refp) {
                यदि (invalidp) फर्काऊ;
                अचर AstVar* अचर varp = refp->varp();
                यदि (!isEnclosingInstanceVar(varp) || ownVars.गन्ती(varp)) फर्काऊ;
                यदि (!enclosingVars.गन्ती(varp)) {
                    invalidp = refp;
                    फर्काऊ;
                }
                refsToRewrite.थप_back(refp);
                यदि (!offenderp) offenderp = refp;
            });
            यदि (invalidp) फर्काऊ;
            rootp->foreach([&](AstThisRef* refp) {
                अचर AstClassRefDType* अचर refDTypep
                    = VN_CAST(refp->dtypep()->skipRefp(), ClassRefDType);
                यदि (refDTypep && refDTypep->classp() == m_covergroupp) {
                    thisRefsToRewrite.थप_back(refp);
                    यदि (!offenderp) offenderp = refp;
                }
            });
            rootp->foreach([&](AstFuncRef* refp) {
                यदि (!isEnclosingInstanceFunc(refp->taskp())) फर्काऊ;
                funcRefsToRewrite.थप_back(refp);
                यदि (!offenderp) offenderp = refp;
            });
        };
        लागि (AstCoverpoint* अचर cpp : m_coverpoints) scan(cpp);
        लागि (AstCoverCross* अचर crossp : m_coverCrosses) scan(crossp);
        लागि (AstCgOptionAssign* अचर optp : m_cgOptions) scan(optp);
        यदि (invalidp || !offenderp) फर्काऊ invalidp;

        UASSERT_OBJ(m_embeddedVarp, m_covergroupp, "Embedded covergroup variable not found");
        // Commit: add the back-pointer member, rewrite the references, initialize the handle.
        File_पङ्क्ति* अचर fl = m_covergroupp->fileline();
        AstClassRefDType* अचर enclDTypep = नूतन AstClassRefDType{fl, m_enclosingClassp, nullptr};
        enclDTypep->rawPointer(सत्य);
        v3Global.rootp()->typeTablep()->addTypesp(enclDTypep);
        AstVar* अचर handleVarp
            = नूतन AstVar{fl, VVarType::MEMBER, "__Vcg_enclosingp", enclDTypep};
        m_covergroupp->addMembersp(handleVarp);
        AstVar* अचर argumentp = नूतन AstVar{fl, VVarType::BLOCKTEMP, "__Vcg_parentp", enclDTypep};
        argumentp->direction(VDirection::आगत);
        argumentp->declDirection(VDirection::आगत);
        argumentp->funcLocal(सत्य);
        argumentp->noReset(सत्य);
        argumentp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
        m_constructorp->addStmtsp(argumentp);
        m_constructorp->prependStmtsp(नूतन AstAssign{fl, memberRef(fl, handleVarp, VAccess::WRITE),
                                                    नूतन AstVarRef{fl, argumentp, VAccess::READ}});

        // Route each enclosing-member reference through the back-pointer: 'm' -> 'h.m'.
        लागि (AstVarRef* अचर refp : refsToRewrite) { rewriteVarRef(refp, handleVarp); }
        लागि (AstThisRef* अचर refp : thisRefsToRewrite) { rewriteThisRef(refp, handleVarp); }
        लागि (AstFuncRef* अचर refp : funcRefsToRewrite) { rewriteFuncRef(refp, handleVarp); }

        // Append a named hidden argument to preserve positional and defaulted user arguments.
        लागि (Ast_गाँठो_Assign* अचर constructp : constructps) {
            File_पङ्क्ति* अचर cfl = constructp->fileline();
            AstCExpr* अचर thisp = नूतन AstCExpr{cfl, "this"};
            thisp->dtypep(enclDTypep);
            VN_AS(constructp->rhsp(), New)->addArgsp(नूतन AstArg{cfl, argumentp->name(), thisp});
        }
        फर्काऊ nullptr;
    }

    शून्य visit(AstClass* nodep) override {
        UINFO(9, "Visiting class: " << nodep->name() << " isCovergroup=" << nodep->isCovergroup());
        यदि (nodep->isCovergroup()) {
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
            Ast_Sen_रूख* embeddedEventForkp = nullptr;

            // Extract and store the clocking event from AstCovergroup node
            // The parser creates this node to preserve the event information
            तार्किक hasUnsupportedEvent = असत्य;
            लागि (Ast_गाँठो* itemp = nodep->membersp(); itemp;) {
                Ast_गाँठो* अचर nextp = itemp->nextp();
                यदि (AstCovergroup* अचर cgp = VN_CAST(itemp, Covergroup)) {
                    // Store the event in the global map for V3Active to retrieve later
                    // V3LinkParse only creates this sentinel AstCovergroup node when a clocking
                    // event exists, so cgp->eventp() is always non-null here.
                    UASSERT_OBJ(cgp->eventp(), cgp,
                                "Sentinel AstCovergroup in class must have non-null eventp");
                    यदि (hasEnclosingEventRef(cgp)) {
                        UASSERT_OBJ(m_embeddedVarp, cgp,
                                    "Embedded covergroup event has no instance variable");
                        यदि (v3Global.opt.timing().isSetTrue()) {
                            embeddedEventForkp = cgp->eventp()->unlinkFrBack();
                        } अन्यथा {
                            embeddedEventTriggers = collectEmbeddedEventTriggers(cgp);
                            यदि (embeddedEventTriggers.empty()) {
                                cgp->v3warn(COVERIGN,
                                            "Unsupported: 'covergroup' clocking event on complex "
                                            "member expression; use --timing for full support.");
                                hasUnsupportedEvent = सत्य;
                            }
                        }
                        VL_DO_DANGLING(थप_Deletep(cgp->unlinkFrBack()), cgp);
                        itemp = nextp;
                        जारी;
                    }
                    // V3Active handles events that do not depend on an enclosing instance.
                    UINFO(4, "Keeping covergroup event node for V3Active: " << nodep->name());
                    itemp = nextp;
                    जारी;
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
            यदि (hasUnsupportedEvent) {
                iterateChildren(nodep);
                validateCovergroupExpressions();
                rebindFormalRefs();
                delete_Coverage_वस्तुहरू();
                यदि (embeddedEventForkp) {
                    VL_DO_DANGLING(थप_Deletep(embeddedEventForkp), embeddedEventForkp);
                }
                फर्काऊ;
            }

            iterateChildren(nodep);
            validateCovergroupExpressions();
            rebindFormalRefs();
            अचर std::vector<Ast_गाँठो_Assign*> constructps = findCovergroupConstructions();

            // Embedded covergroups (IEEE 1800-2023 19.4): coverpoints, iff expressions, and
            // crosses may reference members of the enclosing class. The covergroup is lowered
            // into a sibling class with no implicit handle to the enclosing instance. Install
            // an explicit back-pointer and route the references through it.
            यदि (AstVarRef* अचर invalidp = installEnclosingBackPointer(constructps)) {
                invalidp->v3error("Non-static member "
                                  << invalidp->varp()->prettyNameQ()
                                  << " of an outer class requires an explicit "
                                     "object handle (IEEE 1800-2023 8.23).");
                delete_Coverage_वस्तुहरू();
                यदि (embeddedEventForkp) {
                    VL_DO_DANGLING(थप_Deletep(embeddedEventForkp), embeddedEventForkp);
                }
                फर्काऊ;
            }
            installEmbeddedEventTriggers(embeddedEventTriggers, constructps);
            यदि (embeddedEventForkp) installEmbeddedEventFork(embeddedEventForkp, constructps);
            processCovergroup();
            // Remove lowered coverpoints/crosses from the class - they have been
            // fully translated into C++ code and must not reach downstream passes
            delete_Coverage_वस्तुहरू();
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
        m_coverpoints.थप_back(nodep);
        m_coverpointMap.emplace(nodep->name(), nodep);
        iterateChildren(nodep);
    }

    शून्य visit(AstCoverCross* nodep) override {
        UINFO(9, "Found cross: " << nodep->name());
        m_coverCrosses.थप_back(nodep);
        iterateChildren(nodep);
    }

    // V3Width leaves only the covergroup-level options lowerCovergroupOptions() takes
    शून्य visit(AstCgOptionAssign* nodep) override { m_cgOptions.थप_back(nodep); }

    // A package, interface, or module, so the design unit declaring the classes within
    शून्य visit(Ast_गाँठो_Module* nodep) override {
        VL_RESTORER(m_unitp);
        m_unitp = nodep;
        iterateChildren(nodep);
    }

    शून्य visit(Ast_गाँठो* nodep) override { iterateChildren(nodep); }

सार्वजनिक:
    // CONSTRUCTORS
    FunctionalCoverageVisitor(AstNetlist* nodep,
                              अचर std::set<अचर AstVar*>& mergeableTypeOptions)
        : m_mergeableTypeOptions{mergeableTypeOptions} {
        iterate(nodep);
    }
    ~FunctionalCoverageVisitor() override = स्वभाव;
};

//######################################################################
// Functional coverage class functions

शून्य V3Covergroup::covergroup(AstNetlist* nodep) {
    UINFO(4, __FUNCTION__ << ": ");
    अचर CovergroupAssignValidVisitor validVisitor{nodep};
    यदि (!validVisitor.valid()) V3Error::abortIfErrors();
    {  // Destruct before checking
        FunctionalCoverageVisitor{nodep, validVisitor.mergeableTypeOptions()};
    }
    V3Global::dump_Check_Global_रूख("coveragefunc", 0, dump_रूख_Either_Level() >= 3);
}
