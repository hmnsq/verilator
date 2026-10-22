// -*- mode: C++; c-file-style: "cc-mode" -*-
//*************************************************************************
// DESCRIPTION: Verilator: Command line option parser
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

#నిర్వచితం_కాకపోతే V3OPTION_PARSER_NO_VOPTION_BOOL
#నిర్వచించు VL_MT_DISABLED_సంకేతం_UNIT 1

#అన్తర్భావ "V3Global.h"
#అన్తర్భావ "V3Options.h"
#అయితే_ముగింపు
#అన్తర్భావ "V3Error.h"
#అన్తర్భావ "V3OptionParser.h"
#అన్తర్భావ "V3String.h"

//######################################################################
// V3OptionParser::Impl
నిర్మాణం V3OptionParser::Impl final {
    // TYPES

    // Setting for isOnOffAllowed() and isPartialMatchAllowed()
    గణన తరగతి en : uint8_t {
        NONE,  // "-opt"
        FONOFF,  // "-fopt" and "-fno-opt"
        ONOFF,  // "-opt" and "-no-opt"
        విలువ_3  // "-opt val"
    };
    // Base class of actual action classes
    మూస <en N_బహుళకం, తార్కిక N_Allow_Partial_సరిపోలిక = అబద్ధం>
    తరగతి Action_పునాది VL_NOT_FINAL : బహిరంగ ActionIfs {
        తార్కిక m_notForRerun = అబద్ధం;  // This option is not needed for rerun with --dump-inputs
        తార్కిక m_undocumented = అబద్ధం;  // This option is not documented
    బహిరంగ:
        తార్కిక is_విలువ_Needed() స్థిరరాశి override final { తిరిగి N_బహుళకం == en::విలువ_3; }
        తార్కిక isFOnOffAllowed() స్థిరరాశి override final { తిరిగి N_బహుళకం == en::FONOFF; }
        తార్కిక isNotForRerun() స్థిరరాశి override final { తిరిగి m_notForRerun; }
        తార్కిక isOnOffAllowed() స్థిరరాశి override final { తిరిగి N_బహుళకం == en::ONOFF; }
        తార్కిక is_Partial_సరిపోలిక_Allowed() స్థిరరాశి override final { తిరిగి N_Allow_Partial_సరిపోలిక; }
        తార్కిక isUndocumented() స్థిరరాశి override { తిరిగి m_undocumented; }
        ActionIfs& undocumented() override {
            m_undocumented = నిజం;
            తిరిగి *ఇది;
        }
        ActionIfs& notForRerun() override {
            m_notForRerun = నిజం;
            తిరిగి *ఇది;
        }
    };

    // Actual action classes
    మూస <రకం_పేరు T>
    తరగతి Action_సమితి;  // "-opt" for bool-ish, "-opt val" for int and string
    మూస <రకం_పేరు N_Bool>
    తరగతి ActionFOnOff;  // "-fopt" and "-fno-opt" for bool-ish
    మూస <రకం_పేరు N_Bool>
    తరగతి ActionOnOff;  // "-opt" and "-no-opt" for bool-ish
    తరగతి ActionCbCall;  // Callback without argument for "-opt"
    తరగతి ActionCbFOnOff;  // Callback for "-fopt" and "-fno-opt"
    తరగతి ActionCbOnOff;  // Callback for "-opt" and "-no-opt"
    మూస <రకం_పేరు T>
    తరగతి ActionCbVal;  // Callback for "-opt val"
    తరగతి Action_Cb_Partial_సరిపోలిక;  // Callback "-O3" for "-O"
    తరగతి Action_Cb_Partial_సరిపోలిక_Val;  // Callback "-debugi-V3Options 3" for "-debugi-"

    // MEMBERS
    std::map<స్థిరరాశి string, std::unique_ptr<ActionIfs>> m_options;  // All actions for option
    తార్కిక m_isFinalized{అబద్ధం};  // Becomes after finalize() is called
    VSpellCheck m_spellCheck;  // Suggests closest option when not found
};

//######################################################################
// Action classes in V3OptionParser::Impl

#నిర్వచించు V3OPTION_PARSER_DEF_ACT_CLASS(className, type, body, enType) \
    మూస <> \
    తరగతి V3OptionParser::Impl::className<type> final : బహిరంగ Action_పునాది<enType> { \
        type* స్థిరరాశి m_valp; /* Pointer to an option variable*/ \
\
    బహిరంగ: \
        స్పష్టమైన className(type* valp) \
            : m_valp(valp) {} \
        శూన్య exec(స్థిరరాశి అక్షరం* optp, స్థిరరాశి అక్షరం* argp) override { body; } \
    }

V3OPTION_PARSER_DEF_ACT_CLASS(Action_సమితి, తార్కిక, *m_valp = నిజం, en::NONE);
#నిర్వచితం_కాకపోతే V3OPTION_PARSER_NO_VOPTION_BOOL
V3OPTION_PARSER_DEF_ACT_CLASS(Action_సమితి, VOptionBool, m_valp->సమితి_True_Or_False(నిజం), en::NONE);
#అయితే_ముగింపు
V3OPTION_PARSER_DEF_ACT_CLASS(Action_సమితి, పూర్ణాంకం, *m_valp = std::atoi(argp), en::విలువ_3);
V3OPTION_PARSER_DEF_ACT_CLASS(Action_సమితి, string, *m_valp = argp, en::విలువ_3);

V3OPTION_PARSER_DEF_ACT_CLASS(ActionFOnOff, తార్కిక, *m_valp = !has_ఉపసర్గ_FNo(optp), en::FONOFF);
V3OPTION_PARSER_DEF_ACT_CLASS(ActionOnOff, తార్కిక, *m_valp = !has_ఉపసర్గ_No(optp), en::ONOFF);
#నిర్వచితం_కాకపోతే V3OPTION_PARSER_NO_VOPTION_BOOL
V3OPTION_PARSER_DEF_ACT_CLASS(ActionOnOff, VOptionBool, m_valp->సమితి_True_Or_False(!has_ఉపసర్గ_No(optp)),
                              en::ONOFF);
#అయితే_ముగింపు
#నిర్వచనం_రద్దు V3OPTION_PARSER_DEF_ACT_CLASS

#నిర్వచించు V3OPTION_PARSER_DEF_ACT_CB_CLASS(className, funcType, body, ...) \
    తరగతి V3OptionParser::Impl::className final : బహిరంగ Action_పునాది<__VA_ARGS__> { \
        std::function<funcType> m_cb; /* Callback function */ \
\
    బహిరంగ: \
        ఉపయోగించి CbType = std::function<funcType>; \
        స్పష్టమైన className(CbType cb) \
            : m_cb(std::move(cb)) {} \
        శూన్య exec(స్థిరరాశి అక్షరం* optp, స్థిరరాశి అక్షరం* argp) override { body; } \
    }

V3OPTION_PARSER_DEF_ACT_CB_CLASS(ActionCbCall, శూన్య(శూన్య), m_cb(), en::NONE);
V3OPTION_PARSER_DEF_ACT_CB_CLASS(ActionCbFOnOff, శూన్య(తార్కిక), m_cb(!has_ఉపసర్గ_FNo(optp)),
                                 en::FONOFF);
V3OPTION_PARSER_DEF_ACT_CB_CLASS(ActionCbOnOff, శూన్య(తార్కిక), m_cb(!has_ఉపసర్గ_No(optp)), en::ONOFF);
V3OPTION_PARSER_DEF_ACT_CB_CLASS(Action_Cb_Partial_సరిపోలిక, శూన్య(స్థిరరాశి అక్షరం*), m_cb(optp), en::NONE,
                                 నిజం);
V3OPTION_PARSER_DEF_ACT_CB_CLASS(Action_Cb_Partial_సరిపోలిక_Val, శూన్య(స్థిరరాశి అక్షరం*, స్థిరరాశి అక్షరం*),
                                 m_cb(optp, argp), en::విలువ_3, నిజం);

మూస <>
తరగతి V3OptionParser::Impl::ActionCbVal<పూర్ణాంకం> final : బహిరంగ Action_పునాది<en::విలువ_3> {
    std::function<శూన్య(పూర్ణాంకం)> m_cb;  // Callback function

బహిరంగ:
    ఉపయోగించి CbType = std::function<శూన్య(పూర్ణాంకం)>;
    స్పష్టమైన ActionCbVal(CbType cb)
        : m_cb{std::move(cb)} {}
    శూన్య exec(స్థిరరాశి అక్షరం* optp, స్థిరరాశి అక్షరం* argp) override { m_cb(std::atoi(argp)); }
};

మూస <>
తరగతి V3OptionParser::Impl::ActionCbVal<స్థిరరాశి అక్షరం*> final : బహిరంగ Action_పునాది<en::విలువ_3> {
    std::function<శూన్య(స్థిరరాశి అక్షరం*)> m_cb;

బహిరంగ:
    ఉపయోగించి CbType = std::function<శూన్య(స్థిరరాశి అక్షరం*)>;
    స్పష్టమైన ActionCbVal(CbType cb)
        : m_cb{std::move(cb)} {}
    శూన్య exec(స్థిరరాశి అక్షరం* optp, స్థిరరాశి అక్షరం* argp) override { m_cb(argp); }
};

#నిర్వచనం_రద్దు V3OPTION_PARSER_DEF_ACT_CB_CLASS

//######################################################################
// Member functions of V3OptionParser

V3OptionParser::ActionIfs* V3OptionParser::కనుగొను(స్థిరరాశి అక్షరం* optp) {
    స్థిరరాశి స్వయంచాలక it = m_pimpl->m_options.కనుగొను(optp);
    ఒకవేళ (it != m_pimpl->m_options.చివర()) తిరిగి it->రెండవ.get();  // Exact match
    కొరకు (స్వయంచాలక&& act : m_pimpl->m_options) {
        ఒకవేళ (act.రెండవ->isFOnOffAllowed()) {  // Find starts with "-fno"
            ఒకవేళ (స్థిరరాశి అక్షరం* స్థిరరాశి nop
                = VString::startsWith(optp, "-fno-") ? (optp + std::అక్షరశ్రేణి_పొడవు("-fno-")) : శూన్యసూచిక) {
                ఒకవేళ (act.మొదటి.substr(std::అక్షరశ్రేణి_పొడవు("-f"), std::string::npos)
                    == nop) {  // [-f]opt = [-fno-]opt
                    తిరిగి act.రెండవ.get();
                }
            }
        }
        ఒకవేళ (act.రెండవ->isOnOffAllowed()) {  // Find starts with "-no"
            ఒకవేళ (స్థిరరాశి అక్షరం* స్థిరరాశి nop
                = VString::startsWith(optp, "-no") ? (optp + std::అక్షరశ్రేణి_పొడవు("-no")) : శూన్యసూచిక) {
                ఒకవేళ (act.మొదటి == nop || act.మొదటి == ("-"s + nop)) తిరిగి act.రెండవ.get();
            }
        } లేకపోతే ఒకవేళ (act.రెండవ->is_Partial_సరిపోలిక_Allowed()) {
            ఒకవేళ (VString::startsWith(optp, act.మొదటి)) తిరిగి act.రెండవ.get();
        }
    }
    తిరిగి శూన్యసూచిక;
}

మూస <రకం_పేరు T_Act, రకం_పేరు T_Arg>
V3OptionParser::ActionIfs& V3OptionParser::కూడు(స్థిరరాశి std::string& opt, T_Arg arg) {
    UASSERT(!m_pimpl->m_isFinalized, "Cannot add after finalize() is called");
    std::unique_ptr<T_Act> act{కొత్త T_Act{std::move(arg)}};
    UASSERT(opt.కొలత() >= 2, opt << " is too short");
    UASSERT(opt[0] == '-' || opt[0] == '+', opt << " does not start with either '-' or '+'");
    UASSERT(!(opt[0] == '-' && opt[1] == '-'), "Option must have single '-', but " << opt);
    స్థిరరాశి స్వయంచాలక inserted_ఫలితం = m_pimpl->m_options.emplace(opt, std::move(act));
    UASSERT(inserted_ఫలితం.రెండవ, opt << " is already registered");
    తిరిగి *inserted_ఫలితం.మొదటి->రెండవ;
}

తార్కిక V3OptionParser::has_ఉపసర్గ_FNo(స్థిరరాశి అక్షరం* strp) {
    UASSERT(strp[0] == '-', strp << " does not start with '-'");
    ఒకవేళ (strp[1] == '-') ++strp;
    తిరిగి VString::startsWith(strp, "-fno");
}

తార్కిక V3OptionParser::has_ఉపసర్గ_No(స్థిరరాశి అక్షరం* strp) {
    UASSERT(strp[0] == '-', strp << " does not start with '-'");
    ఒకవేళ (strp[1] == '-') ++strp;
    తిరిగి VString::startsWith(strp, "-no");
}

std::pair<పూర్ణాంకం, తార్కిక> V3OptionParser::parse(పూర్ణాంకం idx, పూర్ణాంకం argc, అక్షరం* argv[]) {
    UASSERT(m_pimpl->m_isFinalized, "finalize() must be called before parse()");
    స్థిరరాశి అక్షరం* optp = argv[idx];
    ఒకవేళ (optp[0] == '-' && optp[1] == '-') ++optp;
    ActionIfs* స్థిరరాశి actp = కనుగొను(optp);
    ఒకవేళ (!actp) తిరిగి {0, అబద్ధం};
    ఒకవేళ (!actp->is_విలువ_Needed()) {
        actp->exec(optp, శూన్యసూచిక);
        తిరిగి {1, !actp->isNotForRerun()};
    }
    ఒకవేళ (idx + 1 < argc) {
        actp->exec(optp, argv[idx + 1]);
        తిరిగి {2, !actp->isNotForRerun()};
    }
    తిరిగి {0, అబద్ధం};
}

string V3OptionParser::getSuggestion(స్థిరరాశి అక్షరం* str) స్థిరరాశి {
    తిరిగి m_pimpl->m_spellCheck.ఉత్తమ_Candidate_సందేశం(str);
}

శూన్య V3OptionParser::కూడు_Suggestion_Candidate(స్థిరరాశి string& s) {
    m_pimpl->m_spellCheck.నెట్టు_Candidate(s);
}

శూన్య V3OptionParser::finalize() {
    UASSERT(!m_pimpl->m_isFinalized, "finalize() must not be called twice");
    కొరకు (స్వయంచాలక&& opt : m_pimpl->m_options) {
        ఒకవేళ (opt.రెండవ->isUndocumented()) కొనసాగు;
        m_pimpl->m_spellCheck.నెట్టు_Candidate(opt.మొదటి);
        ఒకవేళ (opt.రెండవ->isFOnOffAllowed()) {
            m_pimpl->m_spellCheck.నెట్టు_Candidate(
                "-fno-" + opt.మొదటి.substr(std::అక్షరశ్రేణి_పొడవు("-f"), std::string::npos));
        }
        ఒకవేళ (opt.రెండవ->isOnOffAllowed()) m_pimpl->m_spellCheck.నెట్టు_Candidate("-no" + opt.మొదటి);
    }
    m_pimpl->m_isFinalized = నిజం;
}

శూన్య V3OptionParser::dumpOptions() {
    కొరకు (స్థిరరాశి స్వయంచాలక& opt : m_pimpl->m_spellCheck.candidates()) {
        std::బయటకు << "OPTION: \"" + opt + "\"\n";
    }
}

V3OptionParser::V3OptionParser()
    : m_pimpl{కొత్త Impl{}} {}

V3OptionParser::~V3OptionParser() = అప్రమేయ;

//######################################################################
// Member functions of V3OptionParser::AppendHelper

#నిర్వచించు V3OPTION_PARSER_DEF_OP(act_రకం, argType, actType) \
    V3OptionParser::ActionIfs& V3OptionParser::AppendHelper::క్రియాకారకం()( \
        స్థిరరాశి అక్షరం* optp, act_రకం, argType arg) స్థిరరాశి { \
        తిరిగి m_parser.కూడు<Impl::actType>(optp, arg); \
    }
V3OPTION_PARSER_DEF_OP(సమితి_2, తార్కిక*, Action_సమితి<తార్కిక>)
#నిర్వచితం_కాకపోతే V3OPTION_PARSER_NO_VOPTION_BOOL
V3OPTION_PARSER_DEF_OP(సమితి_2, VOptionBool*, Action_సమితి<VOptionBool>)
#అయితే_ముగింపు
V3OPTION_PARSER_DEF_OP(సమితి_2, పూర్ణాంకం*, Action_సమితి<పూర్ణాంకం>)
V3OPTION_PARSER_DEF_OP(సమితి_2, string*, Action_సమితి<string>)
V3OPTION_PARSER_DEF_OP(FOnOff, తార్కిక*, ActionFOnOff<తార్కిక>)
V3OPTION_PARSER_DEF_OP(OnOff, తార్కిక*, ActionOnOff<తార్కిక>)
#నిర్వచితం_కాకపోతే V3OPTION_PARSER_NO_VOPTION_BOOL
V3OPTION_PARSER_DEF_OP(OnOff, VOptionBool*, ActionOnOff<VOptionBool>)
#అయితే_ముగింపు
V3OPTION_PARSER_DEF_OP(CbCall, Impl::ActionCbCall::CbType, ActionCbCall)
V3OPTION_PARSER_DEF_OP(CbFOnOff, Impl::ActionCbFOnOff::CbType, ActionCbFOnOff)
V3OPTION_PARSER_DEF_OP(CbOnOff, Impl::ActionCbOnOff::CbType, ActionCbOnOff)
V3OPTION_PARSER_DEF_OP(CbVal, Impl::ActionCbVal<పూర్ణాంకం>::CbType, ActionCbVal<పూర్ణాంకం>)
V3OPTION_PARSER_DEF_OP(CbVal, Impl::ActionCbVal<స్థిరరాశి అక్షరం*>::CbType, ActionCbVal<స్థిరరాశి అక్షరం*>)
#నిర్వచనం_రద్దు V3OPTION_PARSER_DEF_OP

V3OptionParser::ActionIfs&
V3OptionParser::AppendHelper::క్రియాకారకం()(స్థిరరాశి అక్షరం* optp, Cb_Partial_సరిపోలిక,
                                         Impl::Action_Cb_Partial_సరిపోలిక::CbType cb) స్థిరరాశి {
    స్థిరరాశి కొలత_t ఉపసర్గ_Len = std::అక్షరశ్రేణి_పొడవు(optp);
    స్థిరరాశి స్వయంచాలక wrap = [ఉపసర్గ_Len, cb](స్థిరరాశి అక్షరం* optp) { cb(optp + ఉపసర్గ_Len); };
    తిరిగి m_parser.కూడు<Impl::Action_Cb_Partial_సరిపోలిక>(optp, std::move(wrap));
}

V3OptionParser::ActionIfs&
V3OptionParser::AppendHelper::క్రియాకారకం()(స్థిరరాశి అక్షరం* optp, Cb_Partial_సరిపోలిక_Val,
                                         Impl::Action_Cb_Partial_సరిపోలిక_Val::CbType cb) స్థిరరాశి {
    స్థిరరాశి కొలత_t ఉపసర్గ_Len = std::అక్షరశ్రేణి_పొడవు(optp);
    స్థిరరాశి స్వయంచాలక wrap
        = [ఉపసర్గ_Len, cb](స్థిరరాశి అక్షరం* optp, స్థిరరాశి అక్షరం* argp) { cb(optp + ఉపసర్గ_Len, argp); };
    తిరిగి m_parser.కూడు<Impl::Action_Cb_Partial_సరిపోలిక_Val>(optp, std::move(wrap));
}
