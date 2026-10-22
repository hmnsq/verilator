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

#جے_بنا_تعریف V3OPTION_PARSER_NO_VOPTION_BOOL
#تعریف_کرو VL_MT_DISABLED_کوڈ_UNIT 1

#ضمن "V3Global.h"
#ضمن "V3Options.h"
#جے_انت
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
        قدر_3  // "-opt val"
    };
    // Base class of actual action classes
    سانچا <en N_منوال, منطقي N_Allow_Partial_میل = خطأ>
    صنف Action_ادھار VL_NOT_FINAL : عام ActionIfs {
        منطقي m_notForRerun = خطأ;  // This option is not needed for rerun with --dump-inputs
        منطقي m_undocumented = خطأ;  // This option is not documented
    عام:
        منطقي is_قدر_Needed() ثابت override final { أرجع N_منوال == en::قدر_3; }
        منطقي isFOnOffAllowed() ثابت override final { أرجع N_منوال == en::FONOFF; }
        منطقي isNotForRerun() ثابت override final { أرجع m_notForRerun; }
        منطقي isOnOffAllowed() ثابت override final { أرجع N_منوال == en::ONOFF; }
        منطقي is_Partial_میل_Allowed() ثابت override final { أرجع N_Allow_Partial_میل; }
        منطقي isUndocumented() ثابت override { أرجع m_undocumented; }
        ActionIfs& undocumented() override {
            m_undocumented = سچ;
            أرجع *هذا;
        }
        ActionIfs& notForRerun() override {
            m_notForRerun = سچ;
            أرجع *هذا;
        }
    };

    // Actual action classes
    سانچا <قسم_ناں T>
    صنف Action_سیٹ;  // "-opt" for bool-ish, "-opt val" for int and string
    سانچا <قسم_ناں N_Bool>
    صنف ActionFOnOff;  // "-fopt" and "-fno-opt" for bool-ish
    سانچا <قسم_ناں N_Bool>
    صنف ActionOnOff;  // "-opt" and "-no-opt" for bool-ish
    صنف ActionCbCall;  // Callback without argument for "-opt"
    صنف ActionCbFOnOff;  // Callback for "-fopt" and "-fno-opt"
    صنف ActionCbOnOff;  // Callback for "-opt" and "-no-opt"
    سانچا <قسم_ناں T>
    صنف ActionCbVal;  // Callback for "-opt val"
    صنف Action_Cb_Partial_میل;  // Callback "-O3" for "-O"
    صنف Action_Cb_Partial_میل_Val;  // Callback "-debugi-V3Options 3" for "-debugi-"

    // MEMBERS
    std::map<ثابت string, std::unique_ptr<ActionIfs>> m_options;  // All actions for option
    منطقي m_isFinalized{خطأ};  // Becomes after finalize() is called
    VSpellCheck m_spellCheck;  // Suggests closest option when not found
};

//######################################################################
// Action classes in V3OptionParser::Impl

#تعریف_کرو V3OPTION_PARSER_DEF_ACT_CLASS(className, type, body, enType) \
    سانچا <> \
    صنف V3OptionParser::Impl::className<type> final : عام Action_ادھار<enType> { \
        type* ثابت m_valp; /* Pointer to an option variable*/ \
\
    عام: \
        واضح className(type* valp) \
            : m_valp(valp) {} \
        فراغ exec(ثابت حرف* optp, ثابت حرف* argp) override { body; } \
    }

V3OPTION_PARSER_DEF_ACT_CLASS(Action_سیٹ, منطقي, *m_valp = سچ, en::NONE);
#جے_بنا_تعریف V3OPTION_PARSER_NO_VOPTION_BOOL
V3OPTION_PARSER_DEF_ACT_CLASS(Action_سیٹ, VOptionBool, m_valp->سیٹ_True_Or_False(سچ), en::NONE);
#جے_انت
V3OPTION_PARSER_DEF_ACT_CLASS(Action_سیٹ, صحيح, *m_valp = std::atoi(argp), en::قدر_3);
V3OPTION_PARSER_DEF_ACT_CLASS(Action_سیٹ, string, *m_valp = argp, en::قدر_3);

V3OPTION_PARSER_DEF_ACT_CLASS(ActionFOnOff, منطقي, *m_valp = !has_اگیتر_FNo(optp), en::FONOFF);
V3OPTION_PARSER_DEF_ACT_CLASS(ActionOnOff, منطقي, *m_valp = !has_اگیتر_No(optp), en::ONOFF);
#جے_بنا_تعریف V3OPTION_PARSER_NO_VOPTION_BOOL
V3OPTION_PARSER_DEF_ACT_CLASS(ActionOnOff, VOptionBool, m_valp->سیٹ_True_Or_False(!has_اگیتر_No(optp)),
                              en::ONOFF);
#جے_انت
#تعریف_رد V3OPTION_PARSER_DEF_ACT_CLASS

#تعریف_کرو V3OPTION_PARSER_DEF_ACT_CB_CLASS(className, funcType, body, ...) \
    صنف V3OptionParser::Impl::className final : عام Action_ادھار<__VA_ARGS__> { \
        std::function<funcType> m_cb; /* Callback function */ \
\
    عام: \
        ورتوں CbType = std::function<funcType>; \
        واضح className(CbType cb) \
            : m_cb(std::move(cb)) {} \
        فراغ exec(ثابت حرف* optp, ثابت حرف* argp) override { body; } \
    }

V3OPTION_PARSER_DEF_ACT_CB_CLASS(ActionCbCall, فراغ(فراغ), m_cb(), en::NONE);
V3OPTION_PARSER_DEF_ACT_CB_CLASS(ActionCbFOnOff, فراغ(منطقي), m_cb(!has_اگیتر_FNo(optp)),
                                 en::FONOFF);
V3OPTION_PARSER_DEF_ACT_CB_CLASS(ActionCbOnOff, فراغ(منطقي), m_cb(!has_اگیتر_No(optp)), en::ONOFF);
V3OPTION_PARSER_DEF_ACT_CB_CLASS(Action_Cb_Partial_میل, فراغ(ثابت حرف*), m_cb(optp), en::NONE,
                                 سچ);
V3OPTION_PARSER_DEF_ACT_CB_CLASS(Action_Cb_Partial_میل_Val, فراغ(ثابت حرف*, ثابت حرف*),
                                 m_cb(optp, argp), en::قدر_3, سچ);

سانچا <>
صنف V3OptionParser::Impl::ActionCbVal<صحيح> final : عام Action_ادھار<en::قدر_3> {
    std::function<فراغ(صحيح)> m_cb;  // Callback function

عام:
    ورتوں CbType = std::function<فراغ(صحيح)>;
    واضح ActionCbVal(CbType cb)
        : m_cb{std::move(cb)} {}
    فراغ exec(ثابت حرف* optp, ثابت حرف* argp) override { m_cb(std::atoi(argp)); }
};

سانچا <>
صنف V3OptionParser::Impl::ActionCbVal<ثابت حرف*> final : عام Action_ادھار<en::قدر_3> {
    std::function<فراغ(ثابت حرف*)> m_cb;

عام:
    ورتوں CbType = std::function<فراغ(ثابت حرف*)>;
    واضح ActionCbVal(CbType cb)
        : m_cb{std::move(cb)} {}
    فراغ exec(ثابت حرف* optp, ثابت حرف* argp) override { m_cb(argp); }
};

#تعریف_رد V3OPTION_PARSER_DEF_ACT_CB_CLASS

//######################################################################
// Member functions of V3OptionParser

V3OptionParser::ActionIfs* V3OptionParser::لبھو(ثابت حرف* optp) {
    ثابت خودکار it = m_pimpl->m_options.لبھو(optp);
    إذا (it != m_pimpl->m_options.سرا()) أرجع it->دوجا.get();  // Exact match
    لكل (خودکار&& act : m_pimpl->m_options) {
        إذا (act.دوجا->isFOnOffAllowed()) {  // Find starts with "-fno"
            إذا (ثابت حرف* ثابت nop
                = VString::startsWith(optp, "-fno-") ? (optp + std::لڑی_لمبائی("-fno-")) : خالی_سوچک) {
                إذا (act.پہلا.substr(std::لڑی_لمبائی("-f"), std::string::npos)
                    == nop) {  // [-f]opt = [-fno-]opt
                    أرجع act.دوجا.get();
                }
            }
        }
        إذا (act.دوجا->isOnOffAllowed()) {  // Find starts with "-no"
            إذا (ثابت حرف* ثابت nop
                = VString::startsWith(optp, "-no") ? (optp + std::لڑی_لمبائی("-no")) : خالی_سوچک) {
                إذا (act.پہلا == nop || act.پہلا == ("-"s + nop)) أرجع act.دوجا.get();
            }
        } وإلا إذا (act.دوجا->is_Partial_میل_Allowed()) {
            إذا (VString::startsWith(optp, act.پہلا)) أرجع act.دوجا.get();
        }
    }
    أرجع خالی_سوچک;
}

سانچا <قسم_ناں T_Act, قسم_ناں T_Arg>
V3OptionParser::ActionIfs& V3OptionParser::جوڑو(ثابت std::string& opt, T_Arg arg) {
    UASSERT(!m_pimpl->m_isFinalized, "Cannot add after finalize() is called");
    std::unique_ptr<T_Act> act{جديد T_Act{std::move(arg)}};
    UASSERT(opt.ناپ() >= 2, opt << " is too short");
    UASSERT(opt[0] == '-' || opt[0] == '+', opt << " does not start with either '-' or '+'");
    UASSERT(!(opt[0] == '-' && opt[1] == '-'), "Option must have single '-', but " << opt);
    ثابت خودکار inserted_نتیجہ = m_pimpl->m_options.emplace(opt, std::move(act));
    UASSERT(inserted_نتیجہ.دوجا, opt << " is already registered");
    أرجع *inserted_نتیجہ.پہلا->دوجا;
}

منطقي V3OptionParser::has_اگیتر_FNo(ثابت حرف* strp) {
    UASSERT(strp[0] == '-', strp << " does not start with '-'");
    إذا (strp[1] == '-') ++strp;
    أرجع VString::startsWith(strp, "-fno");
}

منطقي V3OptionParser::has_اگیتر_No(ثابت حرف* strp) {
    UASSERT(strp[0] == '-', strp << " does not start with '-'");
    إذا (strp[1] == '-') ++strp;
    أرجع VString::startsWith(strp, "-no");
}

std::pair<صحيح, منطقي> V3OptionParser::parse(صحيح idx, صحيح argc, حرف* argv[]) {
    UASSERT(m_pimpl->m_isFinalized, "finalize() must be called before parse()");
    ثابت حرف* optp = argv[idx];
    إذا (optp[0] == '-' && optp[1] == '-') ++optp;
    ActionIfs* ثابت actp = لبھو(optp);
    إذا (!actp) أرجع {0, خطأ};
    إذا (!actp->is_قدر_Needed()) {
        actp->exec(optp, خالی_سوچک);
        أرجع {1, !actp->isNotForRerun()};
    }
    إذا (idx + 1 < argc) {
        actp->exec(optp, argv[idx + 1]);
        أرجع {2, !actp->isNotForRerun()};
    }
    أرجع {0, خطأ};
}

string V3OptionParser::getSuggestion(ثابت حرف* str) ثابت {
    أرجع m_pimpl->m_spellCheck.بہترین_Candidate_پیغام(str);
}

فراغ V3OptionParser::جوڑو_Suggestion_Candidate(ثابت string& s) {
    m_pimpl->m_spellCheck.ڈالو_Candidate(s);
}

فراغ V3OptionParser::finalize() {
    UASSERT(!m_pimpl->m_isFinalized, "finalize() must not be called twice");
    لكل (خودکار&& opt : m_pimpl->m_options) {
        إذا (opt.دوجا->isUndocumented()) استمر;
        m_pimpl->m_spellCheck.ڈالو_Candidate(opt.پہلا);
        إذا (opt.دوجا->isFOnOffAllowed()) {
            m_pimpl->m_spellCheck.ڈالو_Candidate(
                "-fno-" + opt.پہلا.substr(std::لڑی_لمبائی("-f"), std::string::npos));
        }
        إذا (opt.دوجا->isOnOffAllowed()) m_pimpl->m_spellCheck.ڈالو_Candidate("-no" + opt.پہلا);
    }
    m_pimpl->m_isFinalized = سچ;
}

فراغ V3OptionParser::dumpOptions() {
    لكل (ثابت خودکار& opt : m_pimpl->m_spellCheck.candidates()) {
        std::باہر << "OPTION: \"" + opt + "\"\n";
    }
}

V3OptionParser::V3OptionParser()
    : m_pimpl{جديد Impl{}} {}

V3OptionParser::~V3OptionParser() = افتراضي;

//######################################################################
// Member functions of V3OptionParser::AppendHelper

#تعریف_کرو V3OPTION_PARSER_DEF_OP(act_قسم, argType, actType) \
    V3OptionParser::ActionIfs& V3OptionParser::AppendHelper::عامل()( \
        ثابت حرف* optp, act_قسم, argType arg) ثابت { \
        أرجع m_parser.جوڑو<Impl::actType>(optp, arg); \
    }
V3OPTION_PARSER_DEF_OP(سیٹ_2, منطقي*, Action_سیٹ<منطقي>)
#جے_بنا_تعریف V3OPTION_PARSER_NO_VOPTION_BOOL
V3OPTION_PARSER_DEF_OP(سیٹ_2, VOptionBool*, Action_سیٹ<VOptionBool>)
#جے_انت
V3OPTION_PARSER_DEF_OP(سیٹ_2, صحيح*, Action_سیٹ<صحيح>)
V3OPTION_PARSER_DEF_OP(سیٹ_2, string*, Action_سیٹ<string>)
V3OPTION_PARSER_DEF_OP(FOnOff, منطقي*, ActionFOnOff<منطقي>)
V3OPTION_PARSER_DEF_OP(OnOff, منطقي*, ActionOnOff<منطقي>)
#جے_بنا_تعریف V3OPTION_PARSER_NO_VOPTION_BOOL
V3OPTION_PARSER_DEF_OP(OnOff, VOptionBool*, ActionOnOff<VOptionBool>)
#جے_انت
V3OPTION_PARSER_DEF_OP(CbCall, Impl::ActionCbCall::CbType, ActionCbCall)
V3OPTION_PARSER_DEF_OP(CbFOnOff, Impl::ActionCbFOnOff::CbType, ActionCbFOnOff)
V3OPTION_PARSER_DEF_OP(CbOnOff, Impl::ActionCbOnOff::CbType, ActionCbOnOff)
V3OPTION_PARSER_DEF_OP(CbVal, Impl::ActionCbVal<صحيح>::CbType, ActionCbVal<صحيح>)
V3OPTION_PARSER_DEF_OP(CbVal, Impl::ActionCbVal<ثابت حرف*>::CbType, ActionCbVal<ثابت حرف*>)
#تعریف_رد V3OPTION_PARSER_DEF_OP

V3OptionParser::ActionIfs&
V3OptionParser::AppendHelper::عامل()(ثابت حرف* optp, Cb_Partial_میل,
                                         Impl::Action_Cb_Partial_میل::CbType cb) ثابت {
    ثابت ناپ_t اگیتر_Len = std::لڑی_لمبائی(optp);
    ثابت خودکار wrap = [اگیتر_Len, cb](ثابت حرف* optp) { cb(optp + اگیتر_Len); };
    أرجع m_parser.جوڑو<Impl::Action_Cb_Partial_میل>(optp, std::move(wrap));
}

V3OptionParser::ActionIfs&
V3OptionParser::AppendHelper::عامل()(ثابت حرف* optp, Cb_Partial_میل_Val,
                                         Impl::Action_Cb_Partial_میل_Val::CbType cb) ثابت {
    ثابت ناپ_t اگیتر_Len = std::لڑی_لمبائی(optp);
    ثابت خودکار wrap
        = [اگیتر_Len, cb](ثابت حرف* optp, ثابت حرف* argp) { cb(optp + اگیتر_Len, argp); };
    أرجع m_parser.جوڑو<Impl::Action_Cb_Partial_میل_Val>(optp, std::move(wrap));
}
