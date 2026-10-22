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

#ਜੇ_ਅਪਰਿਭਾਸ਼ਿਤ V3OPTION_PARSER_NO_VOPTION_BOOL
#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ VL_MT_DISABLED_ਕੋਡ_UNIT 1

#ਅਨ੍ਤਰ੍ਭਾਵ "V3Global.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3Options.h"
#ਜੇ_ਅੰਤ
#ਅਨ੍ਤਰ੍ਭਾਵ "V3Error.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3OptionParser.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3String.h"

//######################################################################
// V3OptionParser::Impl
ਬਣਤਰ V3OptionParser::Impl final {
    // TYPES

    // Setting for isOnOffAllowed() and isPartialMatchAllowed()
    ਗਿਣਤੀ ਵਰਗ en : uint8_t {
        NONE,  // "-opt"
        FONOFF,  // "-fopt" and "-fno-opt"
        ONOFF,  // "-opt" and "-no-opt"
        ਮੁੱਲ_3  // "-opt val"
    };
    // Base class of actual action classes
    ਸਾਂਚਾ <en N_ਬਹੁਲਕ, ਤਾਰਕਿਕ N_Allow_Partial_ਮੇਲ = ਝੂਠ>
    ਵਰਗ Action_ਅਧਾਰ VL_NOT_FINAL : ਜਨਤਕ ActionIfs {
        ਤਾਰਕਿਕ m_notForRerun = ਝੂਠ;  // This option is not needed for rerun with --dump-inputs
        ਤਾਰਕਿਕ m_undocumented = ਝੂਠ;  // This option is not documented
    ਜਨਤਕ:
        ਤਾਰਕਿਕ is_ਮੁੱਲ_Needed() ਅਚਲ override final { ਵਾਪਸ N_ਬਹੁਲਕ == en::ਮੁੱਲ_3; }
        ਤਾਰਕਿਕ isFOnOffAllowed() ਅਚਲ override final { ਵਾਪਸ N_ਬਹੁਲਕ == en::FONOFF; }
        ਤਾਰਕਿਕ isNotForRerun() ਅਚਲ override final { ਵਾਪਸ m_notForRerun; }
        ਤਾਰਕਿਕ isOnOffAllowed() ਅਚਲ override final { ਵਾਪਸ N_ਬਹੁਲਕ == en::ONOFF; }
        ਤਾਰਕਿਕ is_Partial_ਮੇਲ_Allowed() ਅਚਲ override final { ਵਾਪਸ N_Allow_Partial_ਮੇਲ; }
        ਤਾਰਕਿਕ isUndocumented() ਅਚਲ override { ਵਾਪਸ m_undocumented; }
        ActionIfs& undocumented() override {
            m_undocumented = ਸੱਚ;
            ਵਾਪਸ *ਇਹ;
        }
        ActionIfs& notForRerun() override {
            m_notForRerun = ਸੱਚ;
            ਵਾਪਸ *ਇਹ;
        }
    };

    // Actual action classes
    ਸਾਂਚਾ <ਕਿਸਮ_ਨਾਂ T>
    ਵਰਗ Action_ਸੈੱਟ;  // "-opt" for bool-ish, "-opt val" for int and string
    ਸਾਂਚਾ <ਕਿਸਮ_ਨਾਂ N_Bool>
    ਵਰਗ ActionFOnOff;  // "-fopt" and "-fno-opt" for bool-ish
    ਸਾਂਚਾ <ਕਿਸਮ_ਨਾਂ N_Bool>
    ਵਰਗ ActionOnOff;  // "-opt" and "-no-opt" for bool-ish
    ਵਰਗ ActionCbCall;  // Callback without argument for "-opt"
    ਵਰਗ ActionCbFOnOff;  // Callback for "-fopt" and "-fno-opt"
    ਵਰਗ ActionCbOnOff;  // Callback for "-opt" and "-no-opt"
    ਸਾਂਚਾ <ਕਿਸਮ_ਨਾਂ T>
    ਵਰਗ ActionCbVal;  // Callback for "-opt val"
    ਵਰਗ Action_Cb_Partial_ਮੇਲ;  // Callback "-O3" for "-O"
    ਵਰਗ Action_Cb_Partial_ਮੇਲ_Val;  // Callback "-debugi-V3Options 3" for "-debugi-"

    // MEMBERS
    std::map<ਅਚਲ string, std::unique_ptr<ActionIfs>> m_options;  // All actions for option
    ਤਾਰਕਿਕ m_isFinalized{ਝੂਠ};  // Becomes after finalize() is called
    VSpellCheck m_spellCheck;  // Suggests closest option when not found
};

//######################################################################
// Action classes in V3OptionParser::Impl

#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ V3OPTION_PARSER_DEF_ACT_CLASS(className, type, body, enType) \
    ਸਾਂਚਾ <> \
    ਵਰਗ V3OptionParser::Impl::className<type> final : ਜਨਤਕ Action_ਅਧਾਰ<enType> { \
        type* ਅਚਲ m_valp; /* Pointer to an option variable*/ \
\
    ਜਨਤਕ: \
        ਸਪਸ਼ਟ className(type* valp) \
            : m_valp(valp) {} \
        ਖਾਲੀ exec(ਅਚਲ ਅੱਖਰ* optp, ਅਚਲ ਅੱਖਰ* argp) override { body; } \
    }

V3OPTION_PARSER_DEF_ACT_CLASS(Action_ਸੈੱਟ, ਤਾਰਕਿਕ, *m_valp = ਸੱਚ, en::NONE);
#ਜੇ_ਅਪਰਿਭਾਸ਼ਿਤ V3OPTION_PARSER_NO_VOPTION_BOOL
V3OPTION_PARSER_DEF_ACT_CLASS(Action_ਸੈੱਟ, VOptionBool, m_valp->ਸੈੱਟ_True_Or_False(ਸੱਚ), en::NONE);
#ਜੇ_ਅੰਤ
V3OPTION_PARSER_DEF_ACT_CLASS(Action_ਸੈੱਟ, ਪੂਰਨ, *m_valp = std::atoi(argp), en::ਮੁੱਲ_3);
V3OPTION_PARSER_DEF_ACT_CLASS(Action_ਸੈੱਟ, string, *m_valp = argp, en::ਮੁੱਲ_3);

V3OPTION_PARSER_DEF_ACT_CLASS(ActionFOnOff, ਤਾਰਕਿਕ, *m_valp = !has_ਅਗੇਤਰ_FNo(optp), en::FONOFF);
V3OPTION_PARSER_DEF_ACT_CLASS(ActionOnOff, ਤਾਰਕਿਕ, *m_valp = !has_ਅਗੇਤਰ_No(optp), en::ONOFF);
#ਜੇ_ਅਪਰਿਭਾਸ਼ਿਤ V3OPTION_PARSER_NO_VOPTION_BOOL
V3OPTION_PARSER_DEF_ACT_CLASS(ActionOnOff, VOptionBool, m_valp->ਸੈੱਟ_True_Or_False(!has_ਅਗੇਤਰ_No(optp)),
                              en::ONOFF);
#ਜੇ_ਅੰਤ
#ਪਰਿਭਾਸ਼ਾ_ਰੱਦ V3OPTION_PARSER_DEF_ACT_CLASS

#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ V3OPTION_PARSER_DEF_ACT_CB_CLASS(className, funcType, body, ...) \
    ਵਰਗ V3OptionParser::Impl::className final : ਜਨਤਕ Action_ਅਧਾਰ<__VA_ARGS__> { \
        std::function<funcType> m_cb; /* Callback function */ \
\
    ਜਨਤਕ: \
        ਵਰਤੋਂ CbType = std::function<funcType>; \
        ਸਪਸ਼ਟ className(CbType cb) \
            : m_cb(std::move(cb)) {} \
        ਖਾਲੀ exec(ਅਚਲ ਅੱਖਰ* optp, ਅਚਲ ਅੱਖਰ* argp) override { body; } \
    }

V3OPTION_PARSER_DEF_ACT_CB_CLASS(ActionCbCall, ਖਾਲੀ(ਖਾਲੀ), m_cb(), en::NONE);
V3OPTION_PARSER_DEF_ACT_CB_CLASS(ActionCbFOnOff, ਖਾਲੀ(ਤਾਰਕਿਕ), m_cb(!has_ਅਗੇਤਰ_FNo(optp)),
                                 en::FONOFF);
V3OPTION_PARSER_DEF_ACT_CB_CLASS(ActionCbOnOff, ਖਾਲੀ(ਤਾਰਕਿਕ), m_cb(!has_ਅਗੇਤਰ_No(optp)), en::ONOFF);
V3OPTION_PARSER_DEF_ACT_CB_CLASS(Action_Cb_Partial_ਮੇਲ, ਖਾਲੀ(ਅਚਲ ਅੱਖਰ*), m_cb(optp), en::NONE,
                                 ਸੱਚ);
V3OPTION_PARSER_DEF_ACT_CB_CLASS(Action_Cb_Partial_ਮੇਲ_Val, ਖਾਲੀ(ਅਚਲ ਅੱਖਰ*, ਅਚਲ ਅੱਖਰ*),
                                 m_cb(optp, argp), en::ਮੁੱਲ_3, ਸੱਚ);

ਸਾਂਚਾ <>
ਵਰਗ V3OptionParser::Impl::ActionCbVal<ਪੂਰਨ> final : ਜਨਤਕ Action_ਅਧਾਰ<en::ਮੁੱਲ_3> {
    std::function<ਖਾਲੀ(ਪੂਰਨ)> m_cb;  // Callback function

ਜਨਤਕ:
    ਵਰਤੋਂ CbType = std::function<ਖਾਲੀ(ਪੂਰਨ)>;
    ਸਪਸ਼ਟ ActionCbVal(CbType cb)
        : m_cb{std::move(cb)} {}
    ਖਾਲੀ exec(ਅਚਲ ਅੱਖਰ* optp, ਅਚਲ ਅੱਖਰ* argp) override { m_cb(std::atoi(argp)); }
};

ਸਾਂਚਾ <>
ਵਰਗ V3OptionParser::Impl::ActionCbVal<ਅਚਲ ਅੱਖਰ*> final : ਜਨਤਕ Action_ਅਧਾਰ<en::ਮੁੱਲ_3> {
    std::function<ਖਾਲੀ(ਅਚਲ ਅੱਖਰ*)> m_cb;

ਜਨਤਕ:
    ਵਰਤੋਂ CbType = std::function<ਖਾਲੀ(ਅਚਲ ਅੱਖਰ*)>;
    ਸਪਸ਼ਟ ActionCbVal(CbType cb)
        : m_cb{std::move(cb)} {}
    ਖਾਲੀ exec(ਅਚਲ ਅੱਖਰ* optp, ਅਚਲ ਅੱਖਰ* argp) override { m_cb(argp); }
};

#ਪਰਿਭਾਸ਼ਾ_ਰੱਦ V3OPTION_PARSER_DEF_ACT_CB_CLASS

//######################################################################
// Member functions of V3OptionParser

V3OptionParser::ActionIfs* V3OptionParser::ਲੱਭੋ(ਅਚਲ ਅੱਖਰ* optp) {
    ਅਚਲ ਸਵੈਚਾਲਿਤ it = m_pimpl->m_options.ਲੱਭੋ(optp);
    ਜੇ (it != m_pimpl->m_options.ਸਿਰਾ()) ਵਾਪਸ it->ਦੂਜਾ.get();  // Exact match
    ਲਈ (ਸਵੈਚਾਲਿਤ&& act : m_pimpl->m_options) {
        ਜੇ (act.ਦੂਜਾ->isFOnOffAllowed()) {  // Find starts with "-fno"
            ਜੇ (ਅਚਲ ਅੱਖਰ* ਅਚਲ nop
                = VString::startsWith(optp, "-fno-") ? (optp + std::ਲੜੀ_ਲੰਬਾਈ("-fno-")) : ਖਾਲੀ_ਸੂਚਕ) {
                ਜੇ (act.ਪਹਿਲਾ.substr(std::ਲੜੀ_ਲੰਬਾਈ("-f"), std::string::npos)
                    == nop) {  // [-f]opt = [-fno-]opt
                    ਵਾਪਸ act.ਦੂਜਾ.get();
                }
            }
        }
        ਜੇ (act.ਦੂਜਾ->isOnOffAllowed()) {  // Find starts with "-no"
            ਜੇ (ਅਚਲ ਅੱਖਰ* ਅਚਲ nop
                = VString::startsWith(optp, "-no") ? (optp + std::ਲੜੀ_ਲੰਬਾਈ("-no")) : ਖਾਲੀ_ਸੂਚਕ) {
                ਜੇ (act.ਪਹਿਲਾ == nop || act.ਪਹਿਲਾ == ("-"s + nop)) ਵਾਪਸ act.ਦੂਜਾ.get();
            }
        } ਨਹੀਂਤਾਂ ਜੇ (act.ਦੂਜਾ->is_Partial_ਮੇਲ_Allowed()) {
            ਜੇ (VString::startsWith(optp, act.ਪਹਿਲਾ)) ਵਾਪਸ act.ਦੂਜਾ.get();
        }
    }
    ਵਾਪਸ ਖਾਲੀ_ਸੂਚਕ;
}

ਸਾਂਚਾ <ਕਿਸਮ_ਨਾਂ T_Act, ਕਿਸਮ_ਨਾਂ T_Arg>
V3OptionParser::ActionIfs& V3OptionParser::ਜੋੜੋ(ਅਚਲ std::string& opt, T_Arg arg) {
    UASSERT(!m_pimpl->m_isFinalized, "Cannot add after finalize() is called");
    std::unique_ptr<T_Act> act{ਨਵਾਂ T_Act{std::move(arg)}};
    UASSERT(opt.ਮਾਪ() >= 2, opt << " is too short");
    UASSERT(opt[0] == '-' || opt[0] == '+', opt << " does not start with either '-' or '+'");
    UASSERT(!(opt[0] == '-' && opt[1] == '-'), "Option must have single '-', but " << opt);
    ਅਚਲ ਸਵੈਚਾਲਿਤ inserted_ਨਤੀਜਾ = m_pimpl->m_options.emplace(opt, std::move(act));
    UASSERT(inserted_ਨਤੀਜਾ.ਦੂਜਾ, opt << " is already registered");
    ਵਾਪਸ *inserted_ਨਤੀਜਾ.ਪਹਿਲਾ->ਦੂਜਾ;
}

ਤਾਰਕਿਕ V3OptionParser::has_ਅਗੇਤਰ_FNo(ਅਚਲ ਅੱਖਰ* strp) {
    UASSERT(strp[0] == '-', strp << " does not start with '-'");
    ਜੇ (strp[1] == '-') ++strp;
    ਵਾਪਸ VString::startsWith(strp, "-fno");
}

ਤਾਰਕਿਕ V3OptionParser::has_ਅਗੇਤਰ_No(ਅਚਲ ਅੱਖਰ* strp) {
    UASSERT(strp[0] == '-', strp << " does not start with '-'");
    ਜੇ (strp[1] == '-') ++strp;
    ਵਾਪਸ VString::startsWith(strp, "-no");
}

std::pair<ਪੂਰਨ, ਤਾਰਕਿਕ> V3OptionParser::parse(ਪੂਰਨ idx, ਪੂਰਨ argc, ਅੱਖਰ* argv[]) {
    UASSERT(m_pimpl->m_isFinalized, "finalize() must be called before parse()");
    ਅਚਲ ਅੱਖਰ* optp = argv[idx];
    ਜੇ (optp[0] == '-' && optp[1] == '-') ++optp;
    ActionIfs* ਅਚਲ actp = ਲੱਭੋ(optp);
    ਜੇ (!actp) ਵਾਪਸ {0, ਝੂਠ};
    ਜੇ (!actp->is_ਮੁੱਲ_Needed()) {
        actp->exec(optp, ਖਾਲੀ_ਸੂਚਕ);
        ਵਾਪਸ {1, !actp->isNotForRerun()};
    }
    ਜੇ (idx + 1 < argc) {
        actp->exec(optp, argv[idx + 1]);
        ਵਾਪਸ {2, !actp->isNotForRerun()};
    }
    ਵਾਪਸ {0, ਝੂਠ};
}

string V3OptionParser::getSuggestion(ਅਚਲ ਅੱਖਰ* str) ਅਚਲ {
    ਵਾਪਸ m_pimpl->m_spellCheck.ਉੱਤਮ_Candidate_ਸੁਨੇਹਾ(str);
}

ਖਾਲੀ V3OptionParser::ਜੋੜੋ_Suggestion_Candidate(ਅਚਲ string& s) {
    m_pimpl->m_spellCheck.ਧੱਕੋ_Candidate(s);
}

ਖਾਲੀ V3OptionParser::finalize() {
    UASSERT(!m_pimpl->m_isFinalized, "finalize() must not be called twice");
    ਲਈ (ਸਵੈਚਾਲਿਤ&& opt : m_pimpl->m_options) {
        ਜੇ (opt.ਦੂਜਾ->isUndocumented()) ਜਾਰੀ;
        m_pimpl->m_spellCheck.ਧੱਕੋ_Candidate(opt.ਪਹਿਲਾ);
        ਜੇ (opt.ਦੂਜਾ->isFOnOffAllowed()) {
            m_pimpl->m_spellCheck.ਧੱਕੋ_Candidate(
                "-fno-" + opt.ਪਹਿਲਾ.substr(std::ਲੜੀ_ਲੰਬਾਈ("-f"), std::string::npos));
        }
        ਜੇ (opt.ਦੂਜਾ->isOnOffAllowed()) m_pimpl->m_spellCheck.ਧੱਕੋ_Candidate("-no" + opt.ਪਹਿਲਾ);
    }
    m_pimpl->m_isFinalized = ਸੱਚ;
}

ਖਾਲੀ V3OptionParser::dumpOptions() {
    ਲਈ (ਅਚਲ ਸਵੈਚਾਲਿਤ& opt : m_pimpl->m_spellCheck.candidates()) {
        std::ਬਾਹਰ << "OPTION: \"" + opt + "\"\n";
    }
}

V3OptionParser::V3OptionParser()
    : m_pimpl{ਨਵਾਂ Impl{}} {}

V3OptionParser::~V3OptionParser() = ਮੂਲ;

//######################################################################
// Member functions of V3OptionParser::AppendHelper

#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ V3OPTION_PARSER_DEF_OP(act_ਕਿਸਮ, argType, actType) \
    V3OptionParser::ActionIfs& V3OptionParser::AppendHelper::ਸੰਚਾਲਕ()( \
        ਅਚਲ ਅੱਖਰ* optp, act_ਕਿਸਮ, argType arg) ਅਚਲ { \
        ਵਾਪਸ m_parser.ਜੋੜੋ<Impl::actType>(optp, arg); \
    }
V3OPTION_PARSER_DEF_OP(ਸੈੱਟ_2, ਤਾਰਕਿਕ*, Action_ਸੈੱਟ<ਤਾਰਕਿਕ>)
#ਜੇ_ਅਪਰਿਭਾਸ਼ਿਤ V3OPTION_PARSER_NO_VOPTION_BOOL
V3OPTION_PARSER_DEF_OP(ਸੈੱਟ_2, VOptionBool*, Action_ਸੈੱਟ<VOptionBool>)
#ਜੇ_ਅੰਤ
V3OPTION_PARSER_DEF_OP(ਸੈੱਟ_2, ਪੂਰਨ*, Action_ਸੈੱਟ<ਪੂਰਨ>)
V3OPTION_PARSER_DEF_OP(ਸੈੱਟ_2, string*, Action_ਸੈੱਟ<string>)
V3OPTION_PARSER_DEF_OP(FOnOff, ਤਾਰਕਿਕ*, ActionFOnOff<ਤਾਰਕਿਕ>)
V3OPTION_PARSER_DEF_OP(OnOff, ਤਾਰਕਿਕ*, ActionOnOff<ਤਾਰਕਿਕ>)
#ਜੇ_ਅਪਰਿਭਾਸ਼ਿਤ V3OPTION_PARSER_NO_VOPTION_BOOL
V3OPTION_PARSER_DEF_OP(OnOff, VOptionBool*, ActionOnOff<VOptionBool>)
#ਜੇ_ਅੰਤ
V3OPTION_PARSER_DEF_OP(CbCall, Impl::ActionCbCall::CbType, ActionCbCall)
V3OPTION_PARSER_DEF_OP(CbFOnOff, Impl::ActionCbFOnOff::CbType, ActionCbFOnOff)
V3OPTION_PARSER_DEF_OP(CbOnOff, Impl::ActionCbOnOff::CbType, ActionCbOnOff)
V3OPTION_PARSER_DEF_OP(CbVal, Impl::ActionCbVal<ਪੂਰਨ>::CbType, ActionCbVal<ਪੂਰਨ>)
V3OPTION_PARSER_DEF_OP(CbVal, Impl::ActionCbVal<ਅਚਲ ਅੱਖਰ*>::CbType, ActionCbVal<ਅਚਲ ਅੱਖਰ*>)
#ਪਰਿਭਾਸ਼ਾ_ਰੱਦ V3OPTION_PARSER_DEF_OP

V3OptionParser::ActionIfs&
V3OptionParser::AppendHelper::ਸੰਚਾਲਕ()(ਅਚਲ ਅੱਖਰ* optp, Cb_Partial_ਮੇਲ,
                                         Impl::Action_Cb_Partial_ਮੇਲ::CbType cb) ਅਚਲ {
    ਅਚਲ ਮਾਪ_t ਅਗੇਤਰ_Len = std::ਲੜੀ_ਲੰਬਾਈ(optp);
    ਅਚਲ ਸਵੈਚਾਲਿਤ wrap = [ਅਗੇਤਰ_Len, cb](ਅਚਲ ਅੱਖਰ* optp) { cb(optp + ਅਗੇਤਰ_Len); };
    ਵਾਪਸ m_parser.ਜੋੜੋ<Impl::Action_Cb_Partial_ਮੇਲ>(optp, std::move(wrap));
}

V3OptionParser::ActionIfs&
V3OptionParser::AppendHelper::ਸੰਚਾਲਕ()(ਅਚਲ ਅੱਖਰ* optp, Cb_Partial_ਮੇਲ_Val,
                                         Impl::Action_Cb_Partial_ਮੇਲ_Val::CbType cb) ਅਚਲ {
    ਅਚਲ ਮਾਪ_t ਅਗੇਤਰ_Len = std::ਲੜੀ_ਲੰਬਾਈ(optp);
    ਅਚਲ ਸਵੈਚਾਲਿਤ wrap
        = [ਅਗੇਤਰ_Len, cb](ਅਚਲ ਅੱਖਰ* optp, ਅਚਲ ਅੱਖਰ* argp) { cb(optp + ਅਗੇਤਰ_Len, argp); };
    ਵਾਪਸ m_parser.ਜੋੜੋ<Impl::Action_Cb_Partial_ਮੇਲ_Val>(optp, std::move(wrap));
}
