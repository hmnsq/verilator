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

#اگر_غیر_تعریف_شدہ V3OPTION_PARSER_NO_VOPTION_BOOL
#تعریف_کرو VL_MT_DISABLED_رمز_UNIT 1

#شامل "V3Global.h"
#شامل "V3Options.h"
#اگر_ختم
#شامل "V3Error.h"
#شامل "V3OptionParser.h"
#شامل "V3String.h"

//######################################################################
// V3OptionParser::Impl
ساخت V3OptionParser::Impl final {
    // TYPES

    // Setting for isOnOffAllowed() and isPartialMatchAllowed()
    شمار جماعت en : uint8_t {
        NONE,  // "-opt"
        FONOFF,  // "-fopt" and "-fno-opt"
        ONOFF,  // "-opt" and "-no-opt"
        قدر_3  // "-opt val"
    };
    // Base class of actual action classes
    سانچہ <en N_منوال, منطقی N_Allow_Partial_مطابقت = جھوٹ>
    جماعت Action_بنیاد VL_NOT_FINAL : عوامی ActionIfs {
        منطقی m_notForRerun = جھوٹ;  // This option is not needed for rerun with --dump-inputs
        منطقی m_undocumented = جھوٹ;  // This option is not documented
    عوامی:
        منطقی is_قدر_Needed() ثابت override final { واپس N_منوال == en::قدر_3; }
        منطقی isFOnOffAllowed() ثابت override final { واپس N_منوال == en::FONOFF; }
        منطقی isNotForRerun() ثابت override final { واپس m_notForRerun; }
        منطقی isOnOffAllowed() ثابت override final { واپس N_منوال == en::ONOFF; }
        منطقی is_Partial_مطابقت_Allowed() ثابت override final { واپس N_Allow_Partial_مطابقت; }
        منطقی isUndocumented() ثابت override { واپس m_undocumented; }
        ActionIfs& undocumented() override {
            m_undocumented = سچ;
            واپس *یہ;
        }
        ActionIfs& notForRerun() override {
            m_notForRerun = سچ;
            واپس *یہ;
        }
    };

    // Actual action classes
    سانچہ <قسم_نام T>
    جماعت Action_سیٹ;  // "-opt" for bool-ish, "-opt val" for int and string
    سانچہ <قسم_نام N_Bool>
    جماعت ActionFOnOff;  // "-fopt" and "-fno-opt" for bool-ish
    سانچہ <قسم_نام N_Bool>
    جماعت ActionOnOff;  // "-opt" and "-no-opt" for bool-ish
    جماعت ActionCbCall;  // Callback without argument for "-opt"
    جماعت ActionCbFOnOff;  // Callback for "-fopt" and "-fno-opt"
    جماعت ActionCbOnOff;  // Callback for "-opt" and "-no-opt"
    سانچہ <قسم_نام T>
    جماعت ActionCbVal;  // Callback for "-opt val"
    جماعت Action_Cb_Partial_مطابقت;  // Callback "-O3" for "-O"
    جماعت Action_Cb_Partial_مطابقت_Val;  // Callback "-debugi-V3Options 3" for "-debugi-"

    // MEMBERS
    std::map<ثابت string, std::unique_ptr<ActionIfs>> m_options;  // All actions for option
    منطقی m_isFinalized{جھوٹ};  // Becomes after finalize() is called
    VSpellCheck m_spellCheck;  // Suggests closest option when not found
};

//######################################################################
// Action classes in V3OptionParser::Impl

#تعریف_کرو V3OPTION_PARSER_DEF_ACT_CLASS(className, type, body, enType) \
    سانچہ <> \
    جماعت V3OptionParser::Impl::className<type> final : عوامی Action_بنیاد<enType> { \
        type* ثابت m_valp; /* Pointer to an option variable*/ \
\
    عوامی: \
        صریح className(type* valp) \
            : m_valp(valp) {} \
        خالی exec(ثابت حرف* optp, ثابت حرف* argp) override { body; } \
    }

V3OPTION_PARSER_DEF_ACT_CLASS(Action_سیٹ, منطقی, *m_valp = سچ, en::NONE);
#اگر_غیر_تعریف_شدہ V3OPTION_PARSER_NO_VOPTION_BOOL
V3OPTION_PARSER_DEF_ACT_CLASS(Action_سیٹ, VOptionBool, m_valp->سیٹ_True_Or_False(سچ), en::NONE);
#اگر_ختم
V3OPTION_PARSER_DEF_ACT_CLASS(Action_سیٹ, صحیح, *m_valp = std::atoi(argp), en::قدر_3);
V3OPTION_PARSER_DEF_ACT_CLASS(Action_سیٹ, string, *m_valp = argp, en::قدر_3);

V3OPTION_PARSER_DEF_ACT_CLASS(ActionFOnOff, منطقی, *m_valp = !has_سابقہ_FNo(optp), en::FONOFF);
V3OPTION_PARSER_DEF_ACT_CLASS(ActionOnOff, منطقی, *m_valp = !has_سابقہ_No(optp), en::ONOFF);
#اگر_غیر_تعریف_شدہ V3OPTION_PARSER_NO_VOPTION_BOOL
V3OPTION_PARSER_DEF_ACT_CLASS(ActionOnOff, VOptionBool, m_valp->سیٹ_True_Or_False(!has_سابقہ_No(optp)),
                              en::ONOFF);
#اگر_ختم
#تعریف_منسوخ V3OPTION_PARSER_DEF_ACT_CLASS

#تعریف_کرو V3OPTION_PARSER_DEF_ACT_CB_CLASS(className, funcType, body, ...) \
    جماعت V3OptionParser::Impl::className final : عوامی Action_بنیاد<__VA_ARGS__> { \
        std::function<funcType> m_cb; /* Callback function */ \
\
    عوامی: \
        استعمال CbType = std::function<funcType>; \
        صریح className(CbType cb) \
            : m_cb(std::move(cb)) {} \
        خالی exec(ثابت حرف* optp, ثابت حرف* argp) override { body; } \
    }

V3OPTION_PARSER_DEF_ACT_CB_CLASS(ActionCbCall, خالی(خالی), m_cb(), en::NONE);
V3OPTION_PARSER_DEF_ACT_CB_CLASS(ActionCbFOnOff, خالی(منطقی), m_cb(!has_سابقہ_FNo(optp)),
                                 en::FONOFF);
V3OPTION_PARSER_DEF_ACT_CB_CLASS(ActionCbOnOff, خالی(منطقی), m_cb(!has_سابقہ_No(optp)), en::ONOFF);
V3OPTION_PARSER_DEF_ACT_CB_CLASS(Action_Cb_Partial_مطابقت, خالی(ثابت حرف*), m_cb(optp), en::NONE,
                                 سچ);
V3OPTION_PARSER_DEF_ACT_CB_CLASS(Action_Cb_Partial_مطابقت_Val, خالی(ثابت حرف*, ثابت حرف*),
                                 m_cb(optp, argp), en::قدر_3, سچ);

سانچہ <>
جماعت V3OptionParser::Impl::ActionCbVal<صحیح> final : عوامی Action_بنیاد<en::قدر_3> {
    std::function<خالی(صحیح)> m_cb;  // Callback function

عوامی:
    استعمال CbType = std::function<خالی(صحیح)>;
    صریح ActionCbVal(CbType cb)
        : m_cb{std::move(cb)} {}
    خالی exec(ثابت حرف* optp, ثابت حرف* argp) override { m_cb(std::atoi(argp)); }
};

سانچہ <>
جماعت V3OptionParser::Impl::ActionCbVal<ثابت حرف*> final : عوامی Action_بنیاد<en::قدر_3> {
    std::function<خالی(ثابت حرف*)> m_cb;

عوامی:
    استعمال CbType = std::function<خالی(ثابت حرف*)>;
    صریح ActionCbVal(CbType cb)
        : m_cb{std::move(cb)} {}
    خالی exec(ثابت حرف* optp, ثابت حرف* argp) override { m_cb(argp); }
};

#تعریف_منسوخ V3OPTION_PARSER_DEF_ACT_CB_CLASS

//######################################################################
// Member functions of V3OptionParser

V3OptionParser::ActionIfs* V3OptionParser::ڈھونڈو(ثابت حرف* optp) {
    ثابت خودکار it = m_pimpl->m_options.ڈھونڈو(optp);
    اگر (it != m_pimpl->m_options.انتہا()) واپس it->دوسرا.get();  // Exact match
    برائے (خودکار&& act : m_pimpl->m_options) {
        اگر (act.دوسرا->isFOnOffAllowed()) {  // Find starts with "-fno"
            اگر (ثابت حرف* ثابت nop
                = VString::startsWith(optp, "-fno-") ? (optp + std::لڑی_لمبائی("-fno-")) : خالی_اشاریہ) {
                اگر (act.پہلا.substr(std::لڑی_لمبائی("-f"), std::string::npos)
                    == nop) {  // [-f]opt = [-fno-]opt
                    واپس act.دوسرا.get();
                }
            }
        }
        اگر (act.دوسرا->isOnOffAllowed()) {  // Find starts with "-no"
            اگر (ثابت حرف* ثابت nop
                = VString::startsWith(optp, "-no") ? (optp + std::لڑی_لمبائی("-no")) : خالی_اشاریہ) {
                اگر (act.پہلا == nop || act.پہلا == ("-"s + nop)) واپس act.دوسرا.get();
            }
        } ورنہ اگر (act.دوسرا->is_Partial_مطابقت_Allowed()) {
            اگر (VString::startsWith(optp, act.پہلا)) واپس act.دوسرا.get();
        }
    }
    واپس خالی_اشاریہ;
}

سانچہ <قسم_نام T_Act, قسم_نام T_Arg>
V3OptionParser::ActionIfs& V3OptionParser::جوڑو(ثابت std::string& opt, T_Arg arg) {
    UASSERT(!m_pimpl->m_isFinalized, "Cannot add after finalize() is called");
    std::unique_ptr<T_Act> act{نیا T_Act{std::move(arg)}};
    UASSERT(opt.سائز() >= 2, opt << " is too short");
    UASSERT(opt[0] == '-' || opt[0] == '+', opt << " does not start with either '-' or '+'");
    UASSERT(!(opt[0] == '-' && opt[1] == '-'), "Option must have single '-', but " << opt);
    ثابت خودکار inserted_نتیجہ = m_pimpl->m_options.emplace(opt, std::move(act));
    UASSERT(inserted_نتیجہ.دوسرا, opt << " is already registered");
    واپس *inserted_نتیجہ.پہلا->دوسرا;
}

منطقی V3OptionParser::has_سابقہ_FNo(ثابت حرف* strp) {
    UASSERT(strp[0] == '-', strp << " does not start with '-'");
    اگر (strp[1] == '-') ++strp;
    واپس VString::startsWith(strp, "-fno");
}

منطقی V3OptionParser::has_سابقہ_No(ثابت حرف* strp) {
    UASSERT(strp[0] == '-', strp << " does not start with '-'");
    اگر (strp[1] == '-') ++strp;
    واپس VString::startsWith(strp, "-no");
}

std::pair<صحیح, منطقی> V3OptionParser::parse(صحیح idx, صحیح argc, حرف* argv[]) {
    UASSERT(m_pimpl->m_isFinalized, "finalize() must be called before parse()");
    ثابت حرف* optp = argv[idx];
    اگر (optp[0] == '-' && optp[1] == '-') ++optp;
    ActionIfs* ثابت actp = ڈھونڈو(optp);
    اگر (!actp) واپس {0, جھوٹ};
    اگر (!actp->is_قدر_Needed()) {
        actp->exec(optp, خالی_اشاریہ);
        واپس {1, !actp->isNotForRerun()};
    }
    اگر (idx + 1 < argc) {
        actp->exec(optp, argv[idx + 1]);
        واپس {2, !actp->isNotForRerun()};
    }
    واپس {0, جھوٹ};
}

string V3OptionParser::getSuggestion(ثابت حرف* str) ثابت {
    واپس m_pimpl->m_spellCheck.بہترین_Candidate_پیغام(str);
}

خالی V3OptionParser::جوڑو_Suggestion_Candidate(ثابت string& s) {
    m_pimpl->m_spellCheck.ڈالو_Candidate(s);
}

خالی V3OptionParser::finalize() {
    UASSERT(!m_pimpl->m_isFinalized, "finalize() must not be called twice");
    برائے (خودکار&& opt : m_pimpl->m_options) {
        اگر (opt.دوسرا->isUndocumented()) جاری;
        m_pimpl->m_spellCheck.ڈالو_Candidate(opt.پہلا);
        اگر (opt.دوسرا->isFOnOffAllowed()) {
            m_pimpl->m_spellCheck.ڈالو_Candidate(
                "-fno-" + opt.پہلا.substr(std::لڑی_لمبائی("-f"), std::string::npos));
        }
        اگر (opt.دوسرا->isOnOffAllowed()) m_pimpl->m_spellCheck.ڈالو_Candidate("-no" + opt.پہلا);
    }
    m_pimpl->m_isFinalized = سچ;
}

خالی V3OptionParser::dumpOptions() {
    برائے (ثابت خودکار& opt : m_pimpl->m_spellCheck.candidates()) {
        std::خارج << "OPTION: \"" + opt + "\"\n";
    }
}

V3OptionParser::V3OptionParser()
    : m_pimpl{نیا Impl{}} {}

V3OptionParser::~V3OptionParser() = طےشدہ;

//######################################################################
// Member functions of V3OptionParser::AppendHelper

#تعریف_کرو V3OPTION_PARSER_DEF_OP(act_قسم, argType, actType) \
    V3OptionParser::ActionIfs& V3OptionParser::AppendHelper::عامل()( \
        ثابت حرف* optp, act_قسم, argType arg) ثابت { \
        واپس m_parser.جوڑو<Impl::actType>(optp, arg); \
    }
V3OPTION_PARSER_DEF_OP(سیٹ_2, منطقی*, Action_سیٹ<منطقی>)
#اگر_غیر_تعریف_شدہ V3OPTION_PARSER_NO_VOPTION_BOOL
V3OPTION_PARSER_DEF_OP(سیٹ_2, VOptionBool*, Action_سیٹ<VOptionBool>)
#اگر_ختم
V3OPTION_PARSER_DEF_OP(سیٹ_2, صحیح*, Action_سیٹ<صحیح>)
V3OPTION_PARSER_DEF_OP(سیٹ_2, string*, Action_سیٹ<string>)
V3OPTION_PARSER_DEF_OP(FOnOff, منطقی*, ActionFOnOff<منطقی>)
V3OPTION_PARSER_DEF_OP(OnOff, منطقی*, ActionOnOff<منطقی>)
#اگر_غیر_تعریف_شدہ V3OPTION_PARSER_NO_VOPTION_BOOL
V3OPTION_PARSER_DEF_OP(OnOff, VOptionBool*, ActionOnOff<VOptionBool>)
#اگر_ختم
V3OPTION_PARSER_DEF_OP(CbCall, Impl::ActionCbCall::CbType, ActionCbCall)
V3OPTION_PARSER_DEF_OP(CbFOnOff, Impl::ActionCbFOnOff::CbType, ActionCbFOnOff)
V3OPTION_PARSER_DEF_OP(CbOnOff, Impl::ActionCbOnOff::CbType, ActionCbOnOff)
V3OPTION_PARSER_DEF_OP(CbVal, Impl::ActionCbVal<صحیح>::CbType, ActionCbVal<صحیح>)
V3OPTION_PARSER_DEF_OP(CbVal, Impl::ActionCbVal<ثابت حرف*>::CbType, ActionCbVal<ثابت حرف*>)
#تعریف_منسوخ V3OPTION_PARSER_DEF_OP

V3OptionParser::ActionIfs&
V3OptionParser::AppendHelper::عامل()(ثابت حرف* optp, Cb_Partial_مطابقت,
                                         Impl::Action_Cb_Partial_مطابقت::CbType cb) ثابت {
    ثابت سائز_t سابقہ_Len = std::لڑی_لمبائی(optp);
    ثابت خودکار wrap = [سابقہ_Len, cb](ثابت حرف* optp) { cb(optp + سابقہ_Len); };
    واپس m_parser.جوڑو<Impl::Action_Cb_Partial_مطابقت>(optp, std::move(wrap));
}

V3OptionParser::ActionIfs&
V3OptionParser::AppendHelper::عامل()(ثابت حرف* optp, Cb_Partial_مطابقت_Val,
                                         Impl::Action_Cb_Partial_مطابقت_Val::CbType cb) ثابت {
    ثابت سائز_t سابقہ_Len = std::لڑی_لمبائی(optp);
    ثابت خودکار wrap
        = [سابقہ_Len, cb](ثابت حرف* optp, ثابت حرف* argp) { cb(optp + سابقہ_Len, argp); };
    واپس m_parser.جوڑو<Impl::Action_Cb_Partial_مطابقت_Val>(optp, std::move(wrap));
}
