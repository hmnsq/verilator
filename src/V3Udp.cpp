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

#అన్తర్భావ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#అన్తర్భావ "V3Udp.h"

#అన్తర్భావ "V3Error.h"

#అన్తర్భావ <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

తరగతి UdpVisitor final : బహిరంగ VNVisitor {
    తార్కిక m_లోపల_Initial = అబద్ధం;  // Is inside of an initial block
    AstVar* m_o_క్షేత్రం_Varp = శూన్యసూచిక;  // Output filed var of table line
    std::సదిశ<AstVar*> m_ఆదానం_Vars;  // All the input vars in the AstPrimitive
    std::సదిశ<AstVar*> m_outputVars;  // All the output vars in the AstPrimitive
    AstPrimitive* m_primp = శూన్యసూచిక;  // The current primitive
    తార్కిక m_is_మొదటి_Output = అబద్ధం;  // Whether the first IO port is output
    AstVarRef* m_outputInitVerfp = శూన్యసూచిక;  // Initial output value for sequential UDP
    AstAlways* m_alwaysBlockp = శూన్యసూచిక;  // Main Always block in UDP transform

    శూన్య సందర్శించు(AstInitial* nodep) override {
        VL_RESTORER(m_లోపల_Initial);
        ఒకవేళ (m_primp) m_లోపల_Initial = నిజం;
        iterateChildren(nodep);
    }
    శూన్య సందర్శించు(Ast_కేటాయించు* nodep) override {
        ఒకవేళ (m_లోపల_Initial) { m_outputInitVerfp = VN_CAST(nodep->lhsp(), VarRef); }
        iterateChildren(nodep);
    }
    శూన్య సందర్శించు(AstPrimitive* nodep) override {
        UASSERT_OBJ(!m_primp, nodep, "Primitives cannot nest");
        VL_RESTORER(m_primp);
        VL_RESTORER(m_outputInitVerfp);
        VL_RESTORER(m_is_మొదటి_Output);
        VL_RESTORER_CLEAR(m_ఆదానం_Vars);
        VL_RESTORER_CLEAR(m_outputVars);
        m_outputInitVerfp = శూన్యసూచిక;
        m_primp = nodep;
        m_is_మొదటి_Output = అబద్ధం;
        iterateChildren(nodep);
    }
    శూన్య సందర్శించు(AstVar* nodep) override {
        // Push the input and output vars for primitive.
        ఒకవేళ (m_primp) {
            ఒకవేళ (nodep->isIO()) {
                ఒకవేళ (nodep->is_ఆదానం()) {
                    m_ఆదానం_Vars.నెట్టు_వెనుక(nodep);
                } లేకపోతే {
                    m_outputVars.నెట్టు_వెనుక(nodep);
                }
                ఒకవేళ ((m_ఆదానం_Vars.కొలత() == 0) && (m_outputVars.కొలత() == 1)) {
                    m_is_మొదటి_Output = నిజం;
                }
            }
        }
        iterateChildren(nodep);
    }
    శూన్య సందర్శించు(Ast_Udp_పట్టిక* nodep) override {
        File_పంక్తి* స్థిరరాశి fl = nodep->fileline();
        ఒకవేళ (m_outputVars.కొలత() != 1) {
            m_outputVars.వెనుక()->v3error(
                m_outputVars.కొలత()
                << " output ports for UDP table, there must be one output port");
        }
        ఒకవేళ (!m_is_మొదటి_Output && m_outputVars.కొలత()) {
            m_ఆదానం_Vars[0]->v3error("First UDP port must be the output port");
        }
        m_o_క్షేత్రం_Varp = m_outputVars[0];

        m_alwaysBlockp = కొత్త AstAlways{fl, VAlwaysKwd::ALWAYS, శూన్యసూచిక, శూన్యసూచిక};
        fl->warnOff(V3_Error_సంకేతం::LATCH, నిజం);
        iterateChildren(nodep);

        nodep->replaceWith(m_alwaysBlockp);
        VL_DO_DANGLING(నెట్టు_Deletep(nodep), nodep);
    }
    శూన్య సందర్శించు(Ast_Udp_పట్టిక_పంక్తి* nodep) override {
        File_పంక్తి* స్థిరరాశి fl = nodep->fileline();
        ఒకవేళ (!nodep->udpIsCombo() && !m_o_క్షేత్రం_Varp->is_బిట్టు_Logic()) {
            m_o_క్షేత్రం_Varp->v3error("For sequential UDP, the output must be of 'reg' data type");
        }
        ఒకవేళ (nodep->udpIsCombo() && m_o_క్షేత్రం_Varp->is_బిట్టు_Logic()) {
            m_o_క్షేత్రం_Varp->v3error(
                "For combinational UDP, the output must not be a 'reg' data type");
        }
        Ast_కణుపు* iNodep = nodep->iFieldsp();
        Ast_కణుపు* oNodep = nodep->oFieldsp();
        Ast_Sen_వృక్షం* edgetrigp = శూన్యసూచిక;

        AstLogAnd* logandp = కొత్త AstLogAnd{fl, కొత్త AstConst{fl, AstConst::బిట్టు_True{}},
                                           కొత్త AstConst{fl, AstConst::బిట్టు_True{}}};

        కొరకు (AstVar* స్థిరరాశి varp : m_ఆదానం_Vars) {
            ఒకవేళ (!iNodep) ఆపు;
            ఒకవేళ (Ast_Udp_పట్టిక_పంక్తి_Val* linevalp = VN_CAST(iNodep, Udp_పట్టిక_పంక్తి_Val)) {
                string valName = linevalp->name();
                ఒకవేళ (is_అంచు_Trig(valName)) {
                    ఒకవేళ (nodep->udpIsCombo()) {
                        linevalp->v3error(
                            "There should not be a edge trigger for combinational UDP table line");
                    }
                    ఒకవేళ (edgetrigp) {
                        linevalp->v3error("There can be only one edge trigger signal");
                        VL_DO_DANGLING(నెట్టు_Deletep(edgetrigp), edgetrigp);
                    }
                    edgetrigp = కొత్త Ast_Sen_వృక్షం{
                        fl, కొత్త Ast_Sen_అంశం{fl, VEdgeType::ET_BOTHEDGE,
                                           కొత్త AstVarRef{fl, varp, VAccess::READ}}};
                }
                ఒకవేళ (valName == "0" || valName == "f") {
                    logandp = కొత్త AstLogAnd{
                        fl, logandp, కొత్త AstLogNot{fl, కొత్త AstVarRef{fl, varp, VAccess::READ}}};
                } లేకపోతే ఒకవేళ (valName == "1" || valName == "r") {
                    logandp = కొత్త AstLogAnd{fl, logandp, కొత్త AstVarRef{fl, varp, VAccess::READ}};
                } లేకపోతే ఒకవేళ (valName == "x" || valName == "X") {
                    // No x inputs supported yet, so this whole table line
                    // can never match. Drop the whole thing.
                    ఒకవేళ (edgetrigp) VL_DO_DANGLING(నెట్టు_Deletep(edgetrigp), edgetrigp);
                    ఒకవేళ (logandp) VL_DO_DANGLING(నెట్టు_Deletep(logandp), logandp);
                    తిరిగి;
                }
            }
            iNodep = iNodep->nextp();
        }

        uint32_t inputvars = 0;
        కొరకు (స్థిరరాశి Ast_కణుపు* icountp = nodep->iFieldsp(); icountp; icountp = icountp->nextp())
            ++inputvars;
        ఒకవేళ (inputvars != m_ఆదానం_Vars.కొలత()) {
            nodep->v3error("Incorrect number of input values, expected " << m_ఆదానం_Vars.కొలత()
                                                                         << ", got " << inputvars);
        }

        string స్థిరరాశి oValName = nodep->udpIsCombo() ? oNodep->name() : oNodep->nextp()->name();
        ఒకవేళ (oValName == "-") {
            ఒకవేళ (edgetrigp) VL_DO_DANGLING(నెట్టు_Deletep(edgetrigp), edgetrigp);
            ఒకవేళ (logandp) VL_DO_DANGLING(నెట్టు_Deletep(logandp), logandp);
            తిరిగి;
        }

        ఒకవేళ (!nodep->udpIsCombo()) {
            ఒకవేళ (oNodep->name() == "0") {
                logandp = కొత్త AstLogAnd{
                    fl, logandp,
                    కొత్త AstLogNot{fl, కొత్త AstVarRef{fl, m_o_క్షేత్రం_Varp, VAccess::READ}}};
            } లేకపోతే ఒకవేళ (oNodep->name() == "1") {
                logandp
                    = కొత్త AstLogAnd{fl, logandp, కొత్త AstVarRef{fl, m_o_క్షేత్రం_Varp, VAccess::READ}};
            } లేకపోతే ఒకవేళ (oNodep->name() == "x" || oNodep->name() == "X") {
                // No x inputs supported yet, so this whole table line
                // can never match. Drop the whole thing.
                ఒకవేళ (edgetrigp) VL_DO_DANGLING(నెట్టు_Deletep(edgetrigp), edgetrigp);
                ఒకవేళ (logandp) VL_DO_DANGLING(నెట్టు_Deletep(logandp), logandp);
                తిరిగి;
            }
        }

        fl->warnOff(V3_Error_సంకేతం::LATCH, నిజం);
        // Edge-triggered lines are clocked logic, so update like a flop:
        // other clocked logic reading the output sees the pre-edge value, regardless of ordering
        Ast_కణుపు_Stmt* స్థిరరాశి assignp
            = new_Output_కేటాయించు(fl, get_Output_అంకె(nodep, oValName), edgetrigp != శూన్యసూచిక);
        AstIf* స్థిరరాశి ifp = కొత్త AstIf{fl, logandp, assignp};
        ఒకవేళ (nodep->udpIsCombo()) {
            ఒకవేళ (!isCombOutputSig(oValName)) {
                oNodep->v3error("Illegal value for combinational UDP line output");
            }
            m_alwaysBlockp->కూడు_Stmtsp(ifp);
            ఒకవేళ (edgetrigp) VL_DO_DANGLING(నెట్టు_Deletep(edgetrigp), edgetrigp);
            తిరిగి;
        }
        ఒకవేళ (!isSequentOutputSig(oValName)) {
            oNodep->nextp()->v3error("Illegal value for sequential UDP line output");
        }
        m_alwaysBlockp->కూడు_తదుపరి(కొత్త AstAlways{fl, VAlwaysKwd::ALWAYS, edgetrigp, ifp});
    }
    శూన్య సందర్శించు(Ast_కణుపు* nodep) override { iterateChildren(nodep); }
    శూన్య సందర్శించు(AstLogAnd* nodep) override { iterateChildren(nodep); }
    శూన్య సందర్శించు(AstLogNot* nodep) override { iterateChildren(nodep); }
    // For logic processing.
    స్థిర తార్కిక is_అంచు_Trig(std::string& valName) {
        ఒకవేళ (valName == "x" || valName == "X") తిరిగి అబద్ధం;
        ఒకవేళ (valName == "*") తిరిగి నిజం;
        ఒకవేళ (valName == "01" || valName == "p" || valName == "P" || valName == "r"
            || valName == "R") {
            valName = "r";
            తిరిగి నిజం;
        }
        ఒకవేళ (valName == "10" || valName == "n" || valName == "N" || valName == "f"
            || valName == "F") {
            valName = "f";
            తిరిగి నిజం;
        }
        ఒకవేళ (valName.కొలత() == 2) {
            ఒకవేళ (valName[0] == 'x' || valName[0] == 'X' || valName[1] == 'x' || valName[1] == 'X')
                valName = "x";
            లేకపోతే ఒకవేళ (valName[0] == '1' || valName[1] == '0')
                valName = "f";
            లేకపోతే ఒకవేళ (valName[0] == '0' || valName[1] == '1')
                valName = "r";
            తిరిగి నిజం;
        }
        ఒకవేళ (valName[0] != '0' && valName[0] != '1') { valName = "?"; }
        తిరిగి అబద్ధం;
    }
    స్థిర తార్కిక isCombOutputSig(స్థిరరాశి std::string& valName) {
        తిరిగి (valName == "0" || valName == "1" || valName == "x" || valName == "X");
    }
    స్థిర తార్కిక isSequentOutputSig(స్థిరరాశి std::string& valName) {
        తిరిగి (valName == "0" || valName == "1" || valName == "x" || valName == "X"
                || valName == "-");
    }
    Ast_కణుపు_Stmt* new_Output_కేటాయించు(File_పంక్తి* fl, స్థిరరాశి V3_సంఖ్య& బహిర్గతం_అంకె, తార్కిక isDelayed) స్థిరరాశి {
        AstVarRef* స్థిరరాశి lhsp = కొత్త AstVarRef{fl, m_o_క్షేత్రం_Varp, VAccess::WRITE};
        AstConst* స్థిరరాశి rhsp = కొత్త AstConst{fl, బహిర్గతం_అంకె};
        ఒకవేళ (isDelayed) తిరిగి కొత్త Ast_కేటాయించు_Dly{fl, lhsp, rhsp};
        తిరిగి కొత్త Ast_కేటాయించు{fl, lhsp, rhsp};
    }
    స్థిర V3_సంఖ్య get_Output_అంకె(Ast_కణుపు* nodep, స్థిరరాశి std::string& క్షేత్రం_పేర్లు) {
        V3_సంఖ్య output_అంకె{nodep, 1};
        ఒకవేళ (క్షేత్రం_పేర్లు == "0") {
            output_అంకె.సమితి_బిట్టు(0, 0);
        } లేకపోతే ఒకవేళ (క్షేత్రం_పేర్లు == "1") {
            output_అంకె.సమితి_బిట్టు(0, 1);
        } లేకపోతే {
            output_అంకె.సమితి_బిట్టు(0, 'x');
        }
        తిరిగి output_అంకె;
    }

బహిరంగ:
    // CONSTRUCTORS
    స్పష్టమైన UdpVisitor(AstNetlist* nodep) { iterate(nodep); }
    ~UdpVisitor() override = అప్రమేయ;
};

శూన్య V3Udp::udpResolve(AstNetlist* rootp) {
    UINFO(4, __FUNCTION__ << ": ");
    { స్థిరరాశి UdpVisitor visitor{rootp}; }  // Destruct before checking
    V3Global::dump_Check_Global_వృక్షం("udp", 0, dump_వృక్షం_Either_Level() >= 3);
}
