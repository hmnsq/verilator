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

#include "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#include "V3Udp.h"

#include "V3Error.h"

#include <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

class UdpVisitor final : public VNVisitor {
    bool m_inInitial = false;  // Is inside of an initial block
    AstVar* m_oFieldVarp = nullptr;  // Output filed var of table line
    std::vector<AstVar*> m_inputVars;  // All the input vars in the AstPrimitive
    std::vector<AstVar*> m_outputVars;  // All the output vars in the AstPrimitive
    AstPrimitive* m_primp = nullptr;  // The current primitive
    bool m_isFirstOutput = false;  // Whether the first IO port is output
    AstVarRef* m_outputInitVerfp = nullptr;  // Initial output value for sequential UDP
    AstAlways* m_alwaysBlockp = nullptr;  // Main Always block in UDP transform

    void visit(AstInitial* nodep) override {
        VL_RESTORER(m_inInitial);
        ᰀᰧ (m_primp) m_inInitial = true;
        iterateChildren(nodep);
    }
    void visit(AstAssign* nodep) override {
        ᰀᰧ (m_inInitial) { m_outputInitVerfp = VN_CAST(nodep->lhsp(), VarRef); }
        iterateChildren(nodep);
    }
    void visit(AstPrimitive* nodep) override {
        UASSERT_OBJ(!m_primp, nodep, "Primitives cannot nest");
        VL_RESTORER(m_primp);
        VL_RESTORER(m_outputInitVerfp);
        VL_RESTORER(m_isFirstOutput);
        VL_RESTORER_CLEAR(m_inputVars);
        VL_RESTORER_CLEAR(m_outputVars);
        m_outputInitVerfp = nullptr;
        m_primp = nodep;
        m_isFirstOutput = false;
        iterateChildren(nodep);
    }
    void visit(AstVar* nodep) override {
        // Push the input and output vars for primitive.
        ᰀᰧ (m_primp) {
            ᰀᰧ (nodep->isIO()) {
                ᰀᰧ (nodep->isInput()) {
                    m_inputVars.push_back(nodep);
                } ᰀᰧᰀ {
                    m_outputVars.push_back(nodep);
                }
                ᰀᰧ ((m_inputVars.size() == 0) && (m_outputVars.size() == 1)) {
                    m_isFirstOutput = true;
                }
            }
        }
        iterateChildren(nodep);
    }
    void visit(AstUdpTable* nodep) override {
        FileLine* const fl = nodep->fileline();
        ᰀᰧ (m_outputVars.size() != 1) {
            m_outputVars.back()->v3error(
                m_outputVars.size()
                << " output ports for UDP table, there must be one output port");
        }
        ᰀᰧ (!m_isFirstOutput && m_outputVars.size()) {
            m_inputVars[0]->v3error("First UDP port must be the output port");
        }
        m_oFieldVarp = m_outputVars[0];

        m_alwaysBlockp = new AstAlways{fl, VAlwaysKwd::ALWAYS, nullptr, nullptr};
        fl->warnOff(V3ErrorCode::LATCH, true);
        iterateChildren(nodep);

        nodep->replaceWith(m_alwaysBlockp);
        VL_DO_DANGLING(pushDeletep(nodep), nodep);
    }
    void visit(AstUdpTableLine* nodep) override {
        FileLine* const fl = nodep->fileline();
        ᰀᰧ (!nodep->udpIsCombo() && !m_oFieldVarp->isBitLogic()) {
            m_oFieldVarp->v3error("For sequential UDP, the output must be of 'reg' data type");
        }
        ᰀᰧ (nodep->udpIsCombo() && m_oFieldVarp->isBitLogic()) {
            m_oFieldVarp->v3error(
                "For combinational UDP, the output must not be a 'reg' data type");
        }
        AstNode* iNodep = nodep->iFieldsp();
        AstNode* oNodep = nodep->oFieldsp();
        AstSenTree* edgetrigp = nullptr;

        AstLogAnd* logandp = new AstLogAnd{fl, new AstConst{fl, AstConst::BitTrue{}},
                                           new AstConst{fl, AstConst::BitTrue{}}};

        for (AstVar* const varp : m_inputVars) {
            ᰀᰧ (!iNodep) bha_nga;
            ᰀᰧ (AstUdpTableLineVal* linevalp = VN_CAST(iNodep, UdpTableLineVal)) {
                string valName = linevalp->name();
                ᰀᰧ (isEdgeTrig(valName)) {
                    ᰀᰧ (nodep->udpIsCombo()) {
                        linevalp->v3error(
                            "There should not be a edge trigger for combinational UDP table line");
                    }
                    ᰀᰧ (edgetrigp) {
                        linevalp->v3error("There can be only one edge trigger signal");
                        VL_DO_DANGLING(pushDeletep(edgetrigp), edgetrigp);
                    }
                    edgetrigp = new AstSenTree{
                        fl, new AstSenItem{fl, VEdgeType::ET_BOTHEDGE,
                                           new AstVarRef{fl, varp, VAccess::READ}}};
                }
                ᰀᰧ (valName == "0" || valName == "f") {
                    logandp = new AstLogAnd{
                        fl, logandp, new AstLogNot{fl, new AstVarRef{fl, varp, VAccess::READ}}};
                } ᰀᰧᰀ ᰀᰧ (valName == "1" || valName == "r") {
                    logandp = new AstLogAnd{fl, logandp, new AstVarRef{fl, varp, VAccess::READ}};
                } ᰀᰧᰀ ᰀᰧ (valName == "x" || valName == "X") {
                    // No x inputs supported yet, so this whole table line
                    // can never match. Drop the whole thing.
                    ᰀᰧ (edgetrigp) VL_DO_DANGLING(pushDeletep(edgetrigp), edgetrigp);
                    ᰀᰧ (logandp) VL_DO_DANGLING(pushDeletep(logandp), logandp);
                    ᰀᰧᰌ;
                }
            }
            iNodep = iNodep->nextp();
        }

        uint32_t inputvars = 0;
        for (const AstNode* icountp = nodep->iFieldsp(); icountp; icountp = icountp->nextp())
            ++inputvars;
        ᰀᰧ (inputvars != m_inputVars.size()) {
            nodep->v3error("Incorrect number of input values, expected " << m_inputVars.size()
                                                                         << ", got " << inputvars);
        }

        string const oValName = nodep->udpIsCombo() ? oNodep->name() : oNodep->nextp()->name();
        ᰀᰧ (oValName == "-") {
            ᰀᰧ (edgetrigp) VL_DO_DANGLING(pushDeletep(edgetrigp), edgetrigp);
            ᰀᰧ (logandp) VL_DO_DANGLING(pushDeletep(logandp), logandp);
            ᰀᰧᰌ;
        }

        ᰀᰧ (!nodep->udpIsCombo()) {
            ᰀᰧ (oNodep->name() == "0") {
                logandp = new AstLogAnd{
                    fl, logandp,
                    new AstLogNot{fl, new AstVarRef{fl, m_oFieldVarp, VAccess::READ}}};
            } ᰀᰧᰀ ᰀᰧ (oNodep->name() == "1") {
                logandp
                    = new AstLogAnd{fl, logandp, new AstVarRef{fl, m_oFieldVarp, VAccess::READ}};
            } ᰀᰧᰀ ᰀᰧ (oNodep->name() == "x" || oNodep->name() == "X") {
                // No x inputs supported yet, so this whole table line
                // can never match. Drop the whole thing.
                ᰀᰧ (edgetrigp) VL_DO_DANGLING(pushDeletep(edgetrigp), edgetrigp);
                ᰀᰧ (logandp) VL_DO_DANGLING(pushDeletep(logandp), logandp);
                ᰀᰧᰌ;
            }
        }

        fl->warnOff(V3ErrorCode::LATCH, true);
        // Edge-triggered lines are clocked logic, so update like a flop:
        // other clocked logic reading the output sees the pre-edge value, regardless of ordering
        AstNodeStmt* const assignp
            = newOutputAssign(fl, getOutputNum(nodep, oValName), edgetrigp != nullptr);
        AstIf* const ifp = new AstIf{fl, logandp, assignp};
        ᰀᰧ (nodep->udpIsCombo()) {
            ᰀᰧ (!isCombOutputSig(oValName)) {
                oNodep->v3error("Illegal value for combinational UDP line output");
            }
            m_alwaysBlockp->addStmtsp(ifp);
            ᰀᰧ (edgetrigp) VL_DO_DANGLING(pushDeletep(edgetrigp), edgetrigp);
            ᰀᰧᰌ;
        }
        ᰀᰧ (!isSequentOutputSig(oValName)) {
            oNodep->nextp()->v3error("Illegal value for sequential UDP line output");
        }
        m_alwaysBlockp->addNext(new AstAlways{fl, VAlwaysKwd::ALWAYS, edgetrigp, ifp});
    }
    void visit(AstNode* nodep) override { iterateChildren(nodep); }
    void visit(AstLogAnd* nodep) override { iterateChildren(nodep); }
    void visit(AstLogNot* nodep) override { iterateChildren(nodep); }
    // For logic processing.
    static bool isEdgeTrig(std::string& valName) {
        ᰀᰧ (valName == "x" || valName == "X") ᰀᰧᰌ false;
        ᰀᰧ (valName == "*") ᰀᰧᰌ true;
        ᰀᰧ (valName == "01" || valName == "p" || valName == "P" || valName == "r"
            || valName == "R") {
            valName = "r";
            ᰀᰧᰌ true;
        }
        ᰀᰧ (valName == "10" || valName == "n" || valName == "N" || valName == "f"
            || valName == "F") {
            valName = "f";
            ᰀᰧᰌ true;
        }
        ᰀᰧ (valName.size() == 2) {
            ᰀᰧ (valName[0] == 'x' || valName[0] == 'X' || valName[1] == 'x' || valName[1] == 'X')
                valName = "x";
            ᰀᰧᰀ ᰀᰧ (valName[0] == '1' || valName[1] == '0')
                valName = "f";
            ᰀᰧᰀ ᰀᰧ (valName[0] == '0' || valName[1] == '1')
                valName = "r";
            ᰀᰧᰌ true;
        }
        ᰀᰧ (valName[0] != '0' && valName[0] != '1') { valName = "?"; }
        ᰀᰧᰌ false;
    }
    static bool isCombOutputSig(const std::string& valName) {
        ᰀᰧᰌ (valName == "0" || valName == "1" || valName == "x" || valName == "X");
    }
    static bool isSequentOutputSig(const std::string& valName) {
        ᰀᰧᰌ (valName == "0" || valName == "1" || valName == "x" || valName == "X"
                || valName == "-");
    }
    AstNodeStmt* newOutputAssign(FileLine* fl, const V3Number& outNum, bool isDelayed) const {
        AstVarRef* const lhsp = new AstVarRef{fl, m_oFieldVarp, VAccess::WRITE};
        AstConst* const rhsp = new AstConst{fl, outNum};
        ᰀᰧ (isDelayed) ᰀᰧᰌ new AstAssignDly{fl, lhsp, rhsp};
        ᰀᰧᰌ new AstAssign{fl, lhsp, rhsp};
    }
    static V3Number getOutputNum(AstNode* nodep, const std::string& fieldNames) {
        V3Number outputNum{nodep, 1};
        ᰀᰧ (fieldNames == "0") {
            outputNum.setBit(0, 0);
        } ᰀᰧᰀ ᰀᰧ (fieldNames == "1") {
            outputNum.setBit(0, 1);
        } ᰀᰧᰀ {
            outputNum.setBit(0, 'x');
        }
        ᰀᰧᰌ outputNum;
    }

public:
    // CONSTRUCTORS
    explicit UdpVisitor(AstNetlist* nodep) { iterate(nodep); }
    ~UdpVisitor() override = default;
};

void V3Udp::udpResolve(AstNetlist* rootp) {
    UINFO(4, __FUNCTION__ << ": ");
    { const UdpVisitor visitor{rootp}; }  // Destruct before checking
    V3Global::dumpCheckGlobalTree("udp", 0, dumpTreeEitherLevel() >= 3);
}
