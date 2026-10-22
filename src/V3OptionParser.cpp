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

#که_نه_تعریف_شوی V3OPTION_PARSER_NO_VOPTION_BOOL
#تعریف_کړه VL_MT_DISABLED_کوډ_UNIT 1

#ضمن "V3Global.h"
#ضمن "V3Options.h"
#د_که_پای
#ضمن "V3Error.h"
#ضمن "V3OptionParser.h"
#ضمن "V3String.h"

//######################################################################
// V3OptionParser::Impl
بنية V3OptionParser::Impl final {
    // TYPES

    // Setting for isOnOffAllowed() and isPartialMatchAllowed()
    تعداد صنف en : uint8_t {
        NONE,  // "-opt"
        FONOFF,  // "-fopt" and "-fno-opt"
        ONOFF,  // "-opt" and "-no-opt"
        ارزښت_3  // "-opt val"
    };
    // Base class of actual action classes
    کينډۍ <en N_ډېرپېښ, منطقي N_Allow_Partial_سمون = خطأ>
    صنف Action_بنسټ VL_NOT_FINAL : عام ActionIfs {
        منطقي m_notForRerun = خطأ;  // This option is not needed for rerun with --dump-inputs
        منطقي m_undocumented = خطأ;  // This option is not documented
    عام:
        منطقي is_ارزښت_Needed() ثابت override final { أرجع N_ډېرپېښ == en::ارزښت_3; }
        منطقي isFOnOffAllowed() ثابت override final { أرجع N_ډېرپېښ == en::FONOFF; }
        منطقي isNotForRerun() ثابت override final { أرجع m_notForRerun; }
        منطقي isOnOffAllowed() ثابت override final { أرجع N_ډېرپېښ == en::ONOFF; }
        منطقي is_Partial_سمون_Allowed() ثابت override final { أرجع N_Allow_Partial_سمون; }
        منطقي isUndocumented() ثابت override { أرجع m_undocumented; }
        ActionIfs& undocumented() override {
            m_undocumented = ریښتیا;
            أرجع *هذا;
        }
        ActionIfs& notForRerun() override {
            m_notForRerun = ریښتیا;
            أرجع *هذا;
        }
    };

    // Actual action classes
    کينډۍ <ډول_نوم T>
    صنف Action_سېټ;  // "-opt" for bool-ish, "-opt val" for int and string
    کينډۍ <ډول_نوم N_Bool>
    صنف ActionFOnOff;  // "-fopt" and "-fno-opt" for bool-ish
    کينډۍ <ډول_نوم N_Bool>
    صنف ActionOnOff;  // "-opt" and "-no-opt" for bool-ish
    صنف ActionCbCall;  // Callback without argument for "-opt"
    صنف ActionCbFOnOff;  // Callback for "-fopt" and "-fno-opt"
    صنف ActionCbOnOff;  // Callback for "-opt" and "-no-opt"
    کينډۍ <ډول_نوم T>
    صنف ActionCbVal;  // Callback for "-opt val"
    صنف Action_Cb_Partial_سمون;  // Callback "-O3" for "-O"
    صنف Action_Cb_Partial_سمون_Val;  // Callback "-debugi-V3Options 3" for "-debugi-"

    // MEMBERS
    std::map<ثابت string, std::unique_ptr<ActionIfs>> m_options;  // All actions for option
    منطقي m_isFinalized{خطأ};  // Becomes after finalize() is called
    VSpellCheck m_spellCheck;  // Suggests closest option when not found
};

//######################################################################
// Action classes in V3OptionParser::Impl

#تعریف_کړه V3OPTION_PARSER_DEF_ACT_CLASS(className, type, body, enType) \
    کينډۍ <> \
    صنف V3OptionParser::Impl::className<type> final : عام Action_بنسټ<enType> { \
        type* ثابت m_valp; /* Pointer to an option variable*/ \
\
    عام: \
        څرګند className(type* valp) \
            : m_valp(valp) {} \
        فراغ exec(ثابت حرف* optp, ثابت حرف* argp) override { body; } \
    }

V3OPTION_PARSER_DEF_ACT_CLASS(Action_سېټ, منطقي, *m_valp = ریښتیا, en::NONE);
#که_نه_تعریف_شوی V3OPTION_PARSER_NO_VOPTION_BOOL
V3OPTION_PARSER_DEF_ACT_CLASS(Action_سېټ, VOptionBool, m_valp->سېټ_True_Or_False(ریښتیا), en::NONE);
#د_که_پای
V3OPTION_PARSER_DEF_ACT_CLASS(Action_سېټ, صحيح, *m_valp = std::atoi(argp), en::ارزښت_3);
V3OPTION_PARSER_DEF_ACT_CLASS(Action_سېټ, string, *m_valp = argp, en::ارزښت_3);

V3OPTION_PARSER_DEF_ACT_CLASS(ActionFOnOff, منطقي, *m_valp = !has_مختاړی_FNo(optp), en::FONOFF);
V3OPTION_PARSER_DEF_ACT_CLASS(ActionOnOff, منطقي, *m_valp = !has_مختاړی_No(optp), en::ONOFF);
#که_نه_تعریف_شوی V3OPTION_PARSER_NO_VOPTION_BOOL
V3OPTION_PARSER_DEF_ACT_CLASS(ActionOnOff, VOptionBool, m_valp->سېټ_True_Or_False(!has_مختاړی_No(optp)),
                              en::ONOFF);
#د_که_پای
#تعریف_لغوه V3OPTION_PARSER_DEF_ACT_CLASS

#تعریف_کړه V3OPTION_PARSER_DEF_ACT_CB_CLASS(className, funcType, body, ...) \
    صنف V3OptionParser::Impl::className final : عام Action_بنسټ<__VA_ARGS__> { \
        std::function<funcType> m_cb; /* Callback function */ \
\
    عام: \
        کارول CbType = std::function<funcType>; \
        څرګند className(CbType cb) \
            : m_cb(std::move(cb)) {} \
        فراغ exec(ثابت حرف* optp, ثابت حرف* argp) override { body; } \
    }

V3OPTION_PARSER_DEF_ACT_CB_CLASS(ActionCbCall, فراغ(فراغ), m_cb(), en::NONE);
V3OPTION_PARSER_DEF_ACT_CB_CLASS(ActionCbFOnOff, فراغ(منطقي), m_cb(!has_مختاړی_FNo(optp)),
                                 en::FONOFF);
V3OPTION_PARSER_DEF_ACT_CB_CLASS(ActionCbOnOff, فراغ(منطقي), m_cb(!has_مختاړی_No(optp)), en::ONOFF);
V3OPTION_PARSER_DEF_ACT_CB_CLASS(Action_Cb_Partial_سمون, فراغ(ثابت حرف*), m_cb(optp), en::NONE,
                                 ریښتیا);
V3OPTION_PARSER_DEF_ACT_CB_CLASS(Action_Cb_Partial_سمون_Val, فراغ(ثابت حرف*, ثابت حرف*),
                                 m_cb(optp, argp), en::ارزښت_3, ریښتیا);

کينډۍ <>
صنف V3OptionParser::Impl::ActionCbVal<صحيح> final : عام Action_بنسټ<en::ارزښت_3> {
    std::function<فراغ(صحيح)> m_cb;  // Callback function

عام:
    کارول CbType = std::function<فراغ(صحيح)>;
    څرګند ActionCbVal(CbType cb)
        : m_cb{std::move(cb)} {}
    فراغ exec(ثابت حرف* optp, ثابت حرف* argp) override { m_cb(std::atoi(argp)); }
};

کينډۍ <>
صنف V3OptionParser::Impl::ActionCbVal<ثابت حرف*> final : عام Action_بنسټ<en::ارزښت_3> {
    std::function<فراغ(ثابت حرف*)> m_cb;

عام:
    کارول CbType = std::function<فراغ(ثابت حرف*)>;
    څرګند ActionCbVal(CbType cb)
        : m_cb{std::move(cb)} {}
    فراغ exec(ثابت حرف* optp, ثابت حرف* argp) override { m_cb(argp); }
};

#تعریف_لغوه V3OPTION_PARSER_DEF_ACT_CB_CLASS

//######################################################################
// Member functions of V3OptionParser

V3OptionParser::ActionIfs* V3OptionParser::پیدا_کړه(ثابت حرف* optp) {
    ثابت خپلکاری it = m_pimpl->m_options.پیدا_کړه(optp);
    إذا (it != m_pimpl->m_options.پای_ټکی()) أرجع it->دوهم.get();  // Exact match
    لكل (خپلکاری&& act : m_pimpl->m_options) {
        إذا (act.دوهم->isFOnOffAllowed()) {  // Find starts with "-fno"
            إذا (ثابت حرف* ثابت nop
                = VString::startsWith(optp, "-fno-") ? (optp + std::مزي_اوږدوالی("-fno-")) : تش_اشاره) {
                إذا (act.لومړی.substr(std::مزي_اوږدوالی("-f"), std::string::npos)
                    == nop) {  // [-f]opt = [-fno-]opt
                    أرجع act.دوهم.get();
                }
            }
        }
        إذا (act.دوهم->isOnOffAllowed()) {  // Find starts with "-no"
            إذا (ثابت حرف* ثابت nop
                = VString::startsWith(optp, "-no") ? (optp + std::مزي_اوږدوالی("-no")) : تش_اشاره) {
                إذا (act.لومړی == nop || act.لومړی == ("-"s + nop)) أرجع act.دوهم.get();
            }
        } وإلا إذا (act.دوهم->is_Partial_سمون_Allowed()) {
            إذا (VString::startsWith(optp, act.لومړی)) أرجع act.دوهم.get();
        }
    }
    أرجع تش_اشاره;
}

کينډۍ <ډول_نوم T_Act, ډول_نوم T_Arg>
V3OptionParser::ActionIfs& V3OptionParser::جمع_کړه(ثابت std::string& opt, T_Arg arg) {
    UASSERT(!m_pimpl->m_isFinalized, "Cannot add after finalize() is called");
    std::unique_ptr<T_Act> act{جديد T_Act{std::move(arg)}};
    UASSERT(opt.اندازه() >= 2, opt << " is too short");
    UASSERT(opt[0] == '-' || opt[0] == '+', opt << " does not start with either '-' or '+'");
    UASSERT(!(opt[0] == '-' && opt[1] == '-'), "Option must have single '-', but " << opt);
    ثابت خپلکاری inserted_پایله = m_pimpl->m_options.emplace(opt, std::move(act));
    UASSERT(inserted_پایله.دوهم, opt << " is already registered");
    أرجع *inserted_پایله.لومړی->دوهم;
}

منطقي V3OptionParser::has_مختاړی_FNo(ثابت حرف* strp) {
    UASSERT(strp[0] == '-', strp << " does not start with '-'");
    إذا (strp[1] == '-') ++strp;
    أرجع VString::startsWith(strp, "-fno");
}

منطقي V3OptionParser::has_مختاړی_No(ثابت حرف* strp) {
    UASSERT(strp[0] == '-', strp << " does not start with '-'");
    إذا (strp[1] == '-') ++strp;
    أرجع VString::startsWith(strp, "-no");
}

std::pair<صحيح, منطقي> V3OptionParser::parse(صحيح idx, صحيح argc, حرف* argv[]) {
    UASSERT(m_pimpl->m_isFinalized, "finalize() must be called before parse()");
    ثابت حرف* optp = argv[idx];
    إذا (optp[0] == '-' && optp[1] == '-') ++optp;
    ActionIfs* ثابت actp = پیدا_کړه(optp);
    إذا (!actp) أرجع {0, خطأ};
    إذا (!actp->is_ارزښت_Needed()) {
        actp->exec(optp, تش_اشاره);
        أرجع {1, !actp->isNotForRerun()};
    }
    إذا (idx + 1 < argc) {
        actp->exec(optp, argv[idx + 1]);
        أرجع {2, !actp->isNotForRerun()};
    }
    أرجع {0, خطأ};
}

string V3OptionParser::getSuggestion(ثابت حرف* str) ثابت {
    أرجع m_pimpl->m_spellCheck.غوره_Candidate_پیغام(str);
}

فراغ V3OptionParser::جمع_کړه_Suggestion_Candidate(ثابت string& s) {
    m_pimpl->m_spellCheck.ټېل_Candidate(s);
}

فراغ V3OptionParser::finalize() {
    UASSERT(!m_pimpl->m_isFinalized, "finalize() must not be called twice");
    لكل (خپلکاری&& opt : m_pimpl->m_options) {
        إذا (opt.دوهم->isUndocumented()) استمر;
        m_pimpl->m_spellCheck.ټېل_Candidate(opt.لومړی);
        إذا (opt.دوهم->isFOnOffAllowed()) {
            m_pimpl->m_spellCheck.ټېل_Candidate(
                "-fno-" + opt.لومړی.substr(std::مزي_اوږدوالی("-f"), std::string::npos));
        }
        إذا (opt.دوهم->isOnOffAllowed()) m_pimpl->m_spellCheck.ټېل_Candidate("-no" + opt.لومړی);
    }
    m_pimpl->m_isFinalized = ریښتیا;
}

فراغ V3OptionParser::dumpOptions() {
    لكل (ثابت خپلکاری& opt : m_pimpl->m_spellCheck.candidates()) {
        std::وتنه << "OPTION: \"" + opt + "\"\n";
    }
}

V3OptionParser::V3OptionParser()
    : m_pimpl{جديد Impl{}} {}

V3OptionParser::~V3OptionParser() = افتراضي;

//######################################################################
// Member functions of V3OptionParser::AppendHelper

#تعریف_کړه V3OPTION_PARSER_DEF_OP(act_ډول, argType, actType) \
    V3OptionParser::ActionIfs& V3OptionParser::AppendHelper::چلوونکی()( \
        ثابت حرف* optp, act_ډول, argType arg) ثابت { \
        أرجع m_parser.جمع_کړه<Impl::actType>(optp, arg); \
    }
V3OPTION_PARSER_DEF_OP(سېټ_2, منطقي*, Action_سېټ<منطقي>)
#که_نه_تعریف_شوی V3OPTION_PARSER_NO_VOPTION_BOOL
V3OPTION_PARSER_DEF_OP(سېټ_2, VOptionBool*, Action_سېټ<VOptionBool>)
#د_که_پای
V3OPTION_PARSER_DEF_OP(سېټ_2, صحيح*, Action_سېټ<صحيح>)
V3OPTION_PARSER_DEF_OP(سېټ_2, string*, Action_سېټ<string>)
V3OPTION_PARSER_DEF_OP(FOnOff, منطقي*, ActionFOnOff<منطقي>)
V3OPTION_PARSER_DEF_OP(OnOff, منطقي*, ActionOnOff<منطقي>)
#که_نه_تعریف_شوی V3OPTION_PARSER_NO_VOPTION_BOOL
V3OPTION_PARSER_DEF_OP(OnOff, VOptionBool*, ActionOnOff<VOptionBool>)
#د_که_پای
V3OPTION_PARSER_DEF_OP(CbCall, Impl::ActionCbCall::CbType, ActionCbCall)
V3OPTION_PARSER_DEF_OP(CbFOnOff, Impl::ActionCbFOnOff::CbType, ActionCbFOnOff)
V3OPTION_PARSER_DEF_OP(CbOnOff, Impl::ActionCbOnOff::CbType, ActionCbOnOff)
V3OPTION_PARSER_DEF_OP(CbVal, Impl::ActionCbVal<صحيح>::CbType, ActionCbVal<صحيح>)
V3OPTION_PARSER_DEF_OP(CbVal, Impl::ActionCbVal<ثابت حرف*>::CbType, ActionCbVal<ثابت حرف*>)
#تعریف_لغوه V3OPTION_PARSER_DEF_OP

V3OptionParser::ActionIfs&
V3OptionParser::AppendHelper::چلوونکی()(ثابت حرف* optp, Cb_Partial_سمون,
                                         Impl::Action_Cb_Partial_سمون::CbType cb) ثابت {
    ثابت اندازه_t مختاړی_Len = std::مزي_اوږدوالی(optp);
    ثابت خپلکاری wrap = [مختاړی_Len, cb](ثابت حرف* optp) { cb(optp + مختاړی_Len); };
    أرجع m_parser.جمع_کړه<Impl::Action_Cb_Partial_سمون>(optp, std::move(wrap));
}

V3OptionParser::ActionIfs&
V3OptionParser::AppendHelper::چلوونکی()(ثابت حرف* optp, Cb_Partial_سمون_Val,
                                         Impl::Action_Cb_Partial_سمون_Val::CbType cb) ثابت {
    ثابت اندازه_t مختاړی_Len = std::مزي_اوږدوالی(optp);
    ثابت خپلکاری wrap
        = [مختاړی_Len, cb](ثابت حرف* optp, ثابت حرف* argp) { cb(optp + مختاړی_Len, argp); };
    أرجع m_parser.جمع_کړه<Impl::Action_Cb_Partial_سمون_Val>(optp, std::move(wrap));
}
