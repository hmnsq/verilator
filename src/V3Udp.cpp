// -*- mode: C++; c-file-style: "cc-mode" -*-
//*************************************************************************
// DESCRIPTION: Verilator: Implementation of User defined primitives
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
// V3Udp's Transformations:
//
// For every table line create an always block containing if statements
// with condition depending on a combination of the input fields:
//
// 0 1 0 on a, b, c turns into !a&b&~c
//
//*************************************************************************

#ضمن "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ضمن "V3Udp.h"

#ضمن "V3Error.h"

#ضمن <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

صنف UdpVisitor final : عام VNVisitor {
    منطقي m_inInitial = ܕܓܠܐ;  // Is inside of an initial block
    AstVar* m_oFieldVarp = nullptr;  // Output filed var of table line
    std::vector<AstVar*> m_inputVars;  // All the input vars in the AstPrimitive
    std::vector<AstVar*> m_outputVars;  // All the output vars in the AstPrimitive
    AstPrimitive* m_primp = nullptr;  // The current primitive
    منطقي m_isFirstOutput = ܕܓܠܐ;  // Whether the first IO port is output
    AstVarRef* m_outputInitVerfp = nullptr;  // Initial output value for sequential UDP
    AstAlways* m_alwaysBlockp = nullptr;  // Main Always block in UDP transform

    فراغ visit(AstInitial* nodep) override {
        VL_RESTORER(m_inInitial);
        ܐܢ (m_primp) m_inInitial = ܫܪܪܐ;
        iterateChildren(nodep);
    }
    فراغ visit(AstAssign* nodep) override {
        ܐܢ (m_inInitial) { m_outputInitVerfp = VN_CAST(nodep->lhsp(), VarRef); }
        iterateChildren(nodep);
    }
    فراغ visit(AstPrimitive* nodep) override {
        UASSERT_OBJ(!m_primp, nodep, "Primitives cannot nest");
        VL_RESTORER(m_primp);
        VL_RESTORER(m_outputInitVerfp);
        VL_RESTORER(m_isFirstOutput);
        VL_RESTORER_CLEAR(m_inputVars);
        VL_RESTORER_CLEAR(m_outputVars);
        m_outputInitVerfp = nullptr;
        m_primp = nodep;
        m_isFirstOutput = ܕܓܠܐ;
        iterateChildren(nodep);
    }
    فراغ visit(AstVar* nodep) override {
        // Push the input and output vars for primitive.
        ܐܢ (m_primp) {
            ܐܢ (nodep->isIO()) {
                ܐܢ (nodep->isInput()) {
                    m_inputVars.push_back(nodep);
                } ܐܚܪܢܐ {
                    m_outputVars.push_back(nodep);
                }
                ܐܢ ((m_inputVars.size() == 0) && (m_outputVars.size() == 1)) {
                    m_isFirstOutput = ܫܪܪܐ;
                }
            }
        }
        iterateChildren(nodep);
    }
    فراغ visit(AstUdpTable* nodep) override {
        FileLine* ثابت fl = nodep->fileline();
        ܐܢ (m_outputVars.size() != 1) {
            m_outputVars.back()->v3error(
                m_outputVars.size()
                << " output ports for UDP table, there must be one output port");
        }
        ܐܢ (!m_isFirstOutput && m_outputVars.size()) {
            m_inputVars[0]->v3error("First UDP port must be the output port");
        }
        m_oFieldVarp = m_outputVars[0];

        m_alwaysBlockp = جديد AstAlways{fl, VAlwaysKwd::ALWAYS, nullptr, nullptr};
        fl->warnOff(V3ErrorCode::LATCH, ܫܪܪܐ);
        iterateChildren(nodep);

        nodep->replaceWith(m_alwaysBlockp);
        VL_DO_DANGLING(pushDeletep(nodep), nodep);
    }
    فراغ visit(AstUdpTableLine* nodep) override {
        FileLine* ثابت fl = nodep->fileline();
        ܐܢ (!nodep->udpIsCombo() && !m_oFieldVarp->isBitLogic()) {
            m_oFieldVarp->v3error("For sequential UDP, the output must be of 'reg' data type");
        }
        ܐܢ (nodep->udpIsCombo() && m_oFieldVarp->isBitLogic()) {
            m_oFieldVarp->v3error(
                "For combinational UDP, the output must not be a 'reg' data type");
        }
        AstNode* iNodep = nodep->iFieldsp();
        AstNode* oNodep = nodep->oFieldsp();
        AstSenTree* edgetrigp = nullptr;

        AstLogAnd* logandp = جديد AstLogAnd{fl, جديد AstConst{fl, AstConst::BitTrue{}},
                                           جديد AstConst{fl, AstConst::BitTrue{}}};

        لكل (AstVar* ثابت varp : m_inputVars) {
            ܐܢ (!iNodep) اكسر;
            ܐܢ (AstUdpTableLineVal* linevalp = VN_CAST(iNodep, UdpTableLineVal)) {
                string valName = linevalp->name();
                ܐܢ (isEdgeTrig(valName)) {
                    ܐܢ (nodep->udpIsCombo()) {
                        linevalp->v3error(
                            "There should not be a edge trigger for combinational UDP table line");
                    }
                    ܐܢ (edgetrigp) {
                        linevalp->v3error("There can be only one edge trigger signal");
                        VL_DO_DANGLING(pushDeletep(edgetrigp), edgetrigp);
                    }
                    edgetrigp = جديد AstSenTree{
                        fl, جديد AstSenItem{fl, VEdgeType::ET_BOTHEDGE,
                                           جديد AstVarRef{fl, varp, VAccess::READ}}};
                }
                ܐܢ (valName == "0" || valName == "f") {
                    logandp = جديد AstLogAnd{
                        fl, logandp, جديد AstLogNot{fl, جديد AstVarRef{fl, varp, VAccess::READ}}};
                } ܐܚܪܢܐ ܐܢ (valName == "1" || valName == "r") {
                    logandp = جديد AstLogAnd{fl, logandp, جديد AstVarRef{fl, varp, VAccess::READ}};
                } ܐܚܪܢܐ ܐܢ (valName == "x" || valName == "X") {
                    // No x inputs supported yet, so this whole table line
                    // can never match. Drop the whole thing.
                    ܐܢ (edgetrigp) VL_DO_DANGLING(pushDeletep(edgetrigp), edgetrigp);
                    ܐܢ (logandp) VL_DO_DANGLING(pushDeletep(logandp), logandp);
                    ܦܢܐ;
                }
            }
            iNodep = iNodep->nextp();
        }

        uint32_t inputvars = 0;
        لكل (ثابت AstNode* icountp = nodep->iFieldsp(); icountp; icountp = icountp->nextp())
            ++inputvars;
        ܐܢ (inputvars != m_inputVars.size()) {
            nodep->v3error("Incorrect number of input values, expected " << m_inputVars.size()
                                                                         << ", got " << inputvars);
        }

        string ثابت oValName = nodep->udpIsCombo() ? oNodep->name() : oNodep->nextp()->name();
        ܐܢ (oValName == "-") {
            ܐܢ (edgetrigp) VL_DO_DANGLING(pushDeletep(edgetrigp), edgetrigp);
            ܐܢ (logandp) VL_DO_DANGLING(pushDeletep(logandp), logandp);
            ܦܢܐ;
        }

        ܐܢ (!nodep->udpIsCombo()) {
            ܐܢ (oNodep->name() == "0") {
                logandp = جديد AstLogAnd{
                    fl, logandp,
                    جديد AstLogNot{fl, جديد AstVarRef{fl, m_oFieldVarp, VAccess::READ}}};
            } ܐܚܪܢܐ ܐܢ (oNodep->name() == "1") {
                logandp
                    = جديد AstLogAnd{fl, logandp, جديد AstVarRef{fl, m_oFieldVarp, VAccess::READ}};
            } ܐܚܪܢܐ ܐܢ (oNodep->name() == "x" || oNodep->name() == "X") {
                // No x inputs supported yet, so this whole table line
                // can never match. Drop the whole thing.
                ܐܢ (edgetrigp) VL_DO_DANGLING(pushDeletep(edgetrigp), edgetrigp);
                ܐܢ (logandp) VL_DO_DANGLING(pushDeletep(logandp), logandp);
                ܦܢܐ;
            }
        }

        fl->warnOff(V3ErrorCode::LATCH, ܫܪܪܐ);
        // Edge-triggered lines are clocked logic, so update like a flop:
        // other clocked logic reading the output sees the pre-edge value, regardless of ordering
        AstNodeStmt* ثابت assignp
            = newOutputAssign(fl, getOutputNum(nodep, oValName), edgetrigp != nullptr);
        AstIf* ثابت ifp = جديد AstIf{fl, logandp, assignp};
        ܐܢ (nodep->udpIsCombo()) {
            ܐܢ (!isCombOutputSig(oValName)) {
                oNodep->v3error("Illegal value for combinational UDP line output");
            }
            m_alwaysBlockp->addStmtsp(ifp);
            ܐܢ (edgetrigp) VL_DO_DANGLING(pushDeletep(edgetrigp), edgetrigp);
            ܦܢܐ;
        }
        ܐܢ (!isSequentOutputSig(oValName)) {
            oNodep->nextp()->v3error("Illegal value for sequential UDP line output");
        }
        m_alwaysBlockp->addNext(جديد AstAlways{fl, VAlwaysKwd::ALWAYS, edgetrigp, ifp});
    }
    فراغ visit(AstNode* nodep) override { iterateChildren(nodep); }
    فراغ visit(AstLogAnd* nodep) override { iterateChildren(nodep); }
    فراغ visit(AstLogNot* nodep) override { iterateChildren(nodep); }
    // For logic processing.
    ساكن منطقي isEdgeTrig(std::string& valName) {
        ܐܢ (valName == "x" || valName == "X") ܦܢܐ ܕܓܠܐ;
        ܐܢ (valName == "*") ܦܢܐ ܫܪܪܐ;
        ܐܢ (valName == "01" || valName == "p" || valName == "P" || valName == "r"
            || valName == "R") {
            valName = "r";
            ܦܢܐ ܫܪܪܐ;
        }
        ܐܢ (valName == "10" || valName == "n" || valName == "N" || valName == "f"
            || valName == "F") {
            valName = "f";
            ܦܢܐ ܫܪܪܐ;
        }
        ܐܢ (valName.size() == 2) {
            ܐܢ (valName[0] == 'x' || valName[0] == 'X' || valName[1] == 'x' || valName[1] == 'X')
                valName = "x";
            ܐܚܪܢܐ ܐܢ (valName[0] == '1' || valName[1] == '0')
                valName = "f";
            ܐܚܪܢܐ ܐܢ (valName[0] == '0' || valName[1] == '1')
                valName = "r";
            ܦܢܐ ܫܪܪܐ;
        }
        ܐܢ (valName[0] != '0' && valName[0] != '1') { valName = "?"; }
        ܦܢܐ ܕܓܠܐ;
    }
    ساكن منطقي isCombOutputSig(ثابت std::string& valName) {
        ܦܢܐ (valName == "0" || valName == "1" || valName == "x" || valName == "X");
    }
    ساكن منطقي isSequentOutputSig(ثابت std::string& valName) {
        ܦܢܐ (valName == "0" || valName == "1" || valName == "x" || valName == "X"
                || valName == "-");
    }
    AstNodeStmt* newOutputAssign(FileLine* fl, ثابت V3Number& outNum, منطقي isDelayed) ثابت {
        AstVarRef* ثابت lhsp = جديد AstVarRef{fl, m_oFieldVarp, VAccess::WRITE};
        AstConst* ثابت rhsp = جديد AstConst{fl, outNum};
        ܐܢ (isDelayed) ܦܢܐ جديد AstAssignDly{fl, lhsp, rhsp};
        ܦܢܐ جديد AstAssign{fl, lhsp, rhsp};
    }
    ساكن V3Number getOutputNum(AstNode* nodep, ثابت std::string& fieldNames) {
        V3Number outputNum{nodep, 1};
        ܐܢ (fieldNames == "0") {
            outputNum.setBit(0, 0);
        } ܐܚܪܢܐ ܐܢ (fieldNames == "1") {
            outputNum.setBit(0, 1);
        } ܐܚܪܢܐ {
            outputNum.setBit(0, 'x');
        }
        ܦܢܐ outputNum;
    }

عام:
    // CONSTRUCTORS
    explicit UdpVisitor(AstNetlist* nodep) { iterate(nodep); }
    ~UdpVisitor() override = افتراضي;
};

فراغ V3Udp::udpResolve(AstNetlist* rootp) {
    UINFO(4, __FUNCTION__ << ": ");
    { ثابت UdpVisitor visitor{rootp}; }  // Destruct before checking
    V3Global::dumpCheckGlobalTree("udp", 0, dumpTreeEitherLevel() >= 3);
}
