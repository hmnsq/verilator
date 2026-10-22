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

#اگر_تعریف_نشده V3OPTION_PARSER_NO_VOPTION_BOOL
#تعریف_کن VL_MT_DISABLED_کد_UNIT 1

#ضمن "V3Global.h"
#ضمن "V3Options.h"
#پایان_اگر
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
        مقدار_3  // "-opt val"
    };
    // Base class of actual action classes
    قالب <en N_نما, منطقي N_Allow_Partial_تطابق = خطأ>
    صنف Action_پایه VL_NOT_FINAL : عام ActionIfs {
        منطقي m_notForRerun = خطأ;  // This option is not needed for rerun with --dump-inputs
        منطقي m_undocumented = خطأ;  // This option is not documented
    عام:
        منطقي is_مقدار_Needed() ثابت override final { أرجع N_نما == en::مقدار_3; }
        منطقي isFOnOffAllowed() ثابت override final { أرجع N_نما == en::FONOFF; }
        منطقي isNotForRerun() ثابت override final { أرجع m_notForRerun; }
        منطقي isOnOffAllowed() ثابت override final { أرجع N_نما == en::ONOFF; }
        منطقي is_Partial_تطابق_Allowed() ثابت override final { أرجع N_Allow_Partial_تطابق; }
        منطقي isUndocumented() ثابت override { أرجع m_undocumented; }
        ActionIfs& undocumented() override {
            m_undocumented = درست;
            أرجع *هذا;
        }
        ActionIfs& notForRerun() override {
            m_notForRerun = درست;
            أرجع *هذا;
        }
    };

    // Actual action classes
    قالب <نام_نوع T>
    صنف Action_مجموعه;  // "-opt" for bool-ish, "-opt val" for int and string
    قالب <نام_نوع N_Bool>
    صنف ActionFOnOff;  // "-fopt" and "-fno-opt" for bool-ish
    قالب <نام_نوع N_Bool>
    صنف ActionOnOff;  // "-opt" and "-no-opt" for bool-ish
    صنف ActionCbCall;  // Callback without argument for "-opt"
    صنف ActionCbFOnOff;  // Callback for "-fopt" and "-fno-opt"
    صنف ActionCbOnOff;  // Callback for "-opt" and "-no-opt"
    قالب <نام_نوع T>
    صنف ActionCbVal;  // Callback for "-opt val"
    صنف Action_Cb_Partial_تطابق;  // Callback "-O3" for "-O"
    صنف Action_Cb_Partial_تطابق_Val;  // Callback "-debugi-V3Options 3" for "-debugi-"

    // MEMBERS
    std::map<ثابت string, std::unique_ptr<ActionIfs>> m_options;  // All actions for option
    منطقي m_isFinalized{خطأ};  // Becomes after finalize() is called
    VSpellCheck m_spellCheck;  // Suggests closest option when not found
};

//######################################################################
// Action classes in V3OptionParser::Impl

#تعریف_کن V3OPTION_PARSER_DEF_ACT_CLASS(className, type, body, enType) \
    قالب <> \
    صنف V3OptionParser::Impl::className<type> final : عام Action_پایه<enType> { \
        type* ثابت m_valp; /* Pointer to an option variable*/ \
\
    عام: \
        صریح className(type* valp) \
            : m_valp(valp) {} \
        فراغ exec(ثابت حرف* optp, ثابت حرف* argp) override { body; } \
    }

V3OPTION_PARSER_DEF_ACT_CLASS(Action_مجموعه, منطقي, *m_valp = درست, en::NONE);
#اگر_تعریف_نشده V3OPTION_PARSER_NO_VOPTION_BOOL
V3OPTION_PARSER_DEF_ACT_CLASS(Action_مجموعه, VOptionBool, m_valp->مجموعه_True_Or_False(درست), en::NONE);
#پایان_اگر
V3OPTION_PARSER_DEF_ACT_CLASS(Action_مجموعه, صحيح, *m_valp = std::atoi(argp), en::مقدار_3);
V3OPTION_PARSER_DEF_ACT_CLASS(Action_مجموعه, string, *m_valp = argp, en::مقدار_3);

V3OPTION_PARSER_DEF_ACT_CLASS(ActionFOnOff, منطقي, *m_valp = !has_پیشوند_FNo(optp), en::FONOFF);
V3OPTION_PARSER_DEF_ACT_CLASS(ActionOnOff, منطقي, *m_valp = !has_پیشوند_No(optp), en::ONOFF);
#اگر_تعریف_نشده V3OPTION_PARSER_NO_VOPTION_BOOL
V3OPTION_PARSER_DEF_ACT_CLASS(ActionOnOff, VOptionBool, m_valp->مجموعه_True_Or_False(!has_پیشوند_No(optp)),
                              en::ONOFF);
#پایان_اگر
#لغو_تعریف V3OPTION_PARSER_DEF_ACT_CLASS

#تعریف_کن V3OPTION_PARSER_DEF_ACT_CB_CLASS(className, funcType, body, ...) \
    صنف V3OptionParser::Impl::className final : عام Action_پایه<__VA_ARGS__> { \
        std::function<funcType> m_cb; /* Callback function */ \
\
    عام: \
        استفاده CbType = std::function<funcType>; \
        صریح className(CbType cb) \
            : m_cb(std::move(cb)) {} \
        فراغ exec(ثابت حرف* optp, ثابت حرف* argp) override { body; } \
    }

V3OPTION_PARSER_DEF_ACT_CB_CLASS(ActionCbCall, فراغ(فراغ), m_cb(), en::NONE);
V3OPTION_PARSER_DEF_ACT_CB_CLASS(ActionCbFOnOff, فراغ(منطقي), m_cb(!has_پیشوند_FNo(optp)),
                                 en::FONOFF);
V3OPTION_PARSER_DEF_ACT_CB_CLASS(ActionCbOnOff, فراغ(منطقي), m_cb(!has_پیشوند_No(optp)), en::ONOFF);
V3OPTION_PARSER_DEF_ACT_CB_CLASS(Action_Cb_Partial_تطابق, فراغ(ثابت حرف*), m_cb(optp), en::NONE,
                                 درست);
V3OPTION_PARSER_DEF_ACT_CB_CLASS(Action_Cb_Partial_تطابق_Val, فراغ(ثابت حرف*, ثابت حرف*),
                                 m_cb(optp, argp), en::مقدار_3, درست);

قالب <>
صنف V3OptionParser::Impl::ActionCbVal<صحيح> final : عام Action_پایه<en::مقدار_3> {
    std::function<فراغ(صحيح)> m_cb;  // Callback function

عام:
    استفاده CbType = std::function<فراغ(صحيح)>;
    صریح ActionCbVal(CbType cb)
        : m_cb{std::move(cb)} {}
    فراغ exec(ثابت حرف* optp, ثابت حرف* argp) override { m_cb(std::atoi(argp)); }
};

قالب <>
صنف V3OptionParser::Impl::ActionCbVal<ثابت حرف*> final : عام Action_پایه<en::مقدار_3> {
    std::function<فراغ(ثابت حرف*)> m_cb;

عام:
    استفاده CbType = std::function<فراغ(ثابت حرف*)>;
    صریح ActionCbVal(CbType cb)
        : m_cb{std::move(cb)} {}
    فراغ exec(ثابت حرف* optp, ثابت حرف* argp) override { m_cb(argp); }
};

#لغو_تعریف V3OPTION_PARSER_DEF_ACT_CB_CLASS

//######################################################################
// Member functions of V3OptionParser

V3OptionParser::ActionIfs* V3OptionParser::پیدا_کن(ثابت حرف* optp) {
    ثابت خودکار it = m_pimpl->m_options.پیدا_کن(optp);
    إذا (it != m_pimpl->m_options.انتها()) أرجع it->دومی.get();  // Exact match
    لكل (خودکار&& act : m_pimpl->m_options) {
        إذا (act.دومی->isFOnOffAllowed()) {  // Find starts with "-fno"
            إذا (ثابت حرف* ثابت nop
                = VString::startsWith(optp, "-fno-") ? (optp + std::طول_رشته("-fno-")) : اشاره_تهی) {
                إذا (act.اولی.substr(std::طول_رشته("-f"), std::string::npos)
                    == nop) {  // [-f]opt = [-fno-]opt
                    أرجع act.دومی.get();
                }
            }
        }
        إذا (act.دومی->isOnOffAllowed()) {  // Find starts with "-no"
            إذا (ثابت حرف* ثابت nop
                = VString::startsWith(optp, "-no") ? (optp + std::طول_رشته("-no")) : اشاره_تهی) {
                إذا (act.اولی == nop || act.اولی == ("-"s + nop)) أرجع act.دومی.get();
            }
        } وإلا إذا (act.دومی->is_Partial_تطابق_Allowed()) {
            إذا (VString::startsWith(optp, act.اولی)) أرجع act.دومی.get();
        }
    }
    أرجع اشاره_تهی;
}

قالب <نام_نوع T_Act, نام_نوع T_Arg>
V3OptionParser::ActionIfs& V3OptionParser::جمع_کن(ثابت std::string& opt, T_Arg arg) {
    UASSERT(!m_pimpl->m_isFinalized, "Cannot add after finalize() is called");
    std::unique_ptr<T_Act> act{جديد T_Act{std::move(arg)}};
    UASSERT(opt.اندازه() >= 2, opt << " is too short");
    UASSERT(opt[0] == '-' || opt[0] == '+', opt << " does not start with either '-' or '+'");
    UASSERT(!(opt[0] == '-' && opt[1] == '-'), "Option must have single '-', but " << opt);
    ثابت خودکار inserted_نتیجه = m_pimpl->m_options.emplace(opt, std::move(act));
    UASSERT(inserted_نتیجه.دومی, opt << " is already registered");
    أرجع *inserted_نتیجه.اولی->دومی;
}

منطقي V3OptionParser::has_پیشوند_FNo(ثابت حرف* strp) {
    UASSERT(strp[0] == '-', strp << " does not start with '-'");
    إذا (strp[1] == '-') ++strp;
    أرجع VString::startsWith(strp, "-fno");
}

منطقي V3OptionParser::has_پیشوند_No(ثابت حرف* strp) {
    UASSERT(strp[0] == '-', strp << " does not start with '-'");
    إذا (strp[1] == '-') ++strp;
    أرجع VString::startsWith(strp, "-no");
}

std::pair<صحيح, منطقي> V3OptionParser::parse(صحيح idx, صحيح argc, حرف* argv[]) {
    UASSERT(m_pimpl->m_isFinalized, "finalize() must be called before parse()");
    ثابت حرف* optp = argv[idx];
    إذا (optp[0] == '-' && optp[1] == '-') ++optp;
    ActionIfs* ثابت actp = پیدا_کن(optp);
    إذا (!actp) أرجع {0, خطأ};
    إذا (!actp->is_مقدار_Needed()) {
        actp->exec(optp, اشاره_تهی);
        أرجع {1, !actp->isNotForRerun()};
    }
    إذا (idx + 1 < argc) {
        actp->exec(optp, argv[idx + 1]);
        أرجع {2, !actp->isNotForRerun()};
    }
    أرجع {0, خطأ};
}

string V3OptionParser::getSuggestion(ثابت حرف* str) ثابت {
    أرجع m_pimpl->m_spellCheck.بهترین_Candidate_پیام(str);
}

فراغ V3OptionParser::جمع_کن_Suggestion_Candidate(ثابت string& s) {
    m_pimpl->m_spellCheck.افزودن_Candidate(s);
}

فراغ V3OptionParser::finalize() {
    UASSERT(!m_pimpl->m_isFinalized, "finalize() must not be called twice");
    لكل (خودکار&& opt : m_pimpl->m_options) {
        إذا (opt.دومی->isUndocumented()) استمر;
        m_pimpl->m_spellCheck.افزودن_Candidate(opt.اولی);
        إذا (opt.دومی->isFOnOffAllowed()) {
            m_pimpl->m_spellCheck.افزودن_Candidate(
                "-fno-" + opt.اولی.substr(std::طول_رشته("-f"), std::string::npos));
        }
        إذا (opt.دومی->isOnOffAllowed()) m_pimpl->m_spellCheck.افزودن_Candidate("-no" + opt.اولی);
    }
    m_pimpl->m_isFinalized = درست;
}

فراغ V3OptionParser::dumpOptions() {
    لكل (ثابت خودکار& opt : m_pimpl->m_spellCheck.candidates()) {
        std::خروجی << "OPTION: \"" + opt + "\"\n";
    }
}

V3OptionParser::V3OptionParser()
    : m_pimpl{جديد Impl{}} {}

V3OptionParser::~V3OptionParser() = افتراضي;

//######################################################################
// Member functions of V3OptionParser::AppendHelper

#تعریف_کن V3OPTION_PARSER_DEF_OP(act_نوع, argType, actType) \
    V3OptionParser::ActionIfs& V3OptionParser::AppendHelper::عملگر()( \
        ثابت حرف* optp, act_نوع, argType arg) ثابت { \
        أرجع m_parser.جمع_کن<Impl::actType>(optp, arg); \
    }
V3OPTION_PARSER_DEF_OP(مجموعه_2, منطقي*, Action_مجموعه<منطقي>)
#اگر_تعریف_نشده V3OPTION_PARSER_NO_VOPTION_BOOL
V3OPTION_PARSER_DEF_OP(مجموعه_2, VOptionBool*, Action_مجموعه<VOptionBool>)
#پایان_اگر
V3OPTION_PARSER_DEF_OP(مجموعه_2, صحيح*, Action_مجموعه<صحيح>)
V3OPTION_PARSER_DEF_OP(مجموعه_2, string*, Action_مجموعه<string>)
V3OPTION_PARSER_DEF_OP(FOnOff, منطقي*, ActionFOnOff<منطقي>)
V3OPTION_PARSER_DEF_OP(OnOff, منطقي*, ActionOnOff<منطقي>)
#اگر_تعریف_نشده V3OPTION_PARSER_NO_VOPTION_BOOL
V3OPTION_PARSER_DEF_OP(OnOff, VOptionBool*, ActionOnOff<VOptionBool>)
#پایان_اگر
V3OPTION_PARSER_DEF_OP(CbCall, Impl::ActionCbCall::CbType, ActionCbCall)
V3OPTION_PARSER_DEF_OP(CbFOnOff, Impl::ActionCbFOnOff::CbType, ActionCbFOnOff)
V3OPTION_PARSER_DEF_OP(CbOnOff, Impl::ActionCbOnOff::CbType, ActionCbOnOff)
V3OPTION_PARSER_DEF_OP(CbVal, Impl::ActionCbVal<صحيح>::CbType, ActionCbVal<صحيح>)
V3OPTION_PARSER_DEF_OP(CbVal, Impl::ActionCbVal<ثابت حرف*>::CbType, ActionCbVal<ثابت حرف*>)
#لغو_تعریف V3OPTION_PARSER_DEF_OP

V3OptionParser::ActionIfs&
V3OptionParser::AppendHelper::عملگر()(ثابت حرف* optp, Cb_Partial_تطابق,
                                         Impl::Action_Cb_Partial_تطابق::CbType cb) ثابت {
    ثابت اندازه_t پیشوند_Len = std::طول_رشته(optp);
    ثابت خودکار wrap = [پیشوند_Len, cb](ثابت حرف* optp) { cb(optp + پیشوند_Len); };
    أرجع m_parser.جمع_کن<Impl::Action_Cb_Partial_تطابق>(optp, std::move(wrap));
}

V3OptionParser::ActionIfs&
V3OptionParser::AppendHelper::عملگر()(ثابت حرف* optp, Cb_Partial_تطابق_Val,
                                         Impl::Action_Cb_Partial_تطابق_Val::CbType cb) ثابت {
    ثابت اندازه_t پیشوند_Len = std::طول_رشته(optp);
    ثابت خودکار wrap
        = [پیشوند_Len, cb](ثابت حرف* optp, ثابت حرف* argp) { cb(optp + پیشوند_Len, argp); };
    أرجع m_parser.جمع_کن<Impl::Action_Cb_Partial_تطابق_Val>(optp, std::move(wrap));
}
