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

#جيڪڏهن_وصف_نه_ٿيل V3OPTION_PARSER_NO_VOPTION_BOOL
#وصف_ڪريو VL_MT_DISABLED_ڪوڊ_UNIT 1

#ضمن "V3Global.h"
#ضمن "V3Options.h"
#جيڪڏهن_پورو
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
    سانچو <en N_گهڻائي, منطقي N_Allow_Partial_ميلاپ = خطأ>
    صنف Action_بنياد VL_NOT_FINAL : عام ActionIfs {
        منطقي m_notForRerun = خطأ;  // This option is not needed for rerun with --dump-inputs
        منطقي m_undocumented = خطأ;  // This option is not documented
    عام:
        منطقي is_قدر_Needed() ثابت override final { أرجع N_گهڻائي == en::قدر_3; }
        منطقي isFOnOffAllowed() ثابت override final { أرجع N_گهڻائي == en::FONOFF; }
        منطقي isNotForRerun() ثابت override final { أرجع m_notForRerun; }
        منطقي isOnOffAllowed() ثابت override final { أرجع N_گهڻائي == en::ONOFF; }
        منطقي is_Partial_ميلاپ_Allowed() ثابت override final { أرجع N_Allow_Partial_ميلاپ; }
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
    سانچو <قسم_نالو T>
    صنف Action_سيٽ;  // "-opt" for bool-ish, "-opt val" for int and string
    سانچو <قسم_نالو N_Bool>
    صنف ActionFOnOff;  // "-fopt" and "-fno-opt" for bool-ish
    سانچو <قسم_نالو N_Bool>
    صنف ActionOnOff;  // "-opt" and "-no-opt" for bool-ish
    صنف ActionCbCall;  // Callback without argument for "-opt"
    صنف ActionCbFOnOff;  // Callback for "-fopt" and "-fno-opt"
    صنف ActionCbOnOff;  // Callback for "-opt" and "-no-opt"
    سانچو <قسم_نالو T>
    صنف ActionCbVal;  // Callback for "-opt val"
    صنف Action_Cb_Partial_ميلاپ;  // Callback "-O3" for "-O"
    صنف Action_Cb_Partial_ميلاپ_Val;  // Callback "-debugi-V3Options 3" for "-debugi-"

    // MEMBERS
    std::map<ثابت string, std::unique_ptr<ActionIfs>> m_options;  // All actions for option
    منطقي m_isFinalized{خطأ};  // Becomes after finalize() is called
    VSpellCheck m_spellCheck;  // Suggests closest option when not found
};

//######################################################################
// Action classes in V3OptionParser::Impl

#وصف_ڪريو V3OPTION_PARSER_DEF_ACT_CLASS(className, type, body, enType) \
    سانچو <> \
    صنف V3OptionParser::Impl::className<type> final : عام Action_بنياد<enType> { \
        type* ثابت m_valp; /* Pointer to an option variable*/ \
\
    عام: \
        واضح className(type* valp) \
            : m_valp(valp) {} \
        فراغ exec(ثابت حرف* optp, ثابت حرف* argp) override { body; } \
    }

V3OPTION_PARSER_DEF_ACT_CLASS(Action_سيٽ, منطقي, *m_valp = سچ, en::NONE);
#جيڪڏهن_وصف_نه_ٿيل V3OPTION_PARSER_NO_VOPTION_BOOL
V3OPTION_PARSER_DEF_ACT_CLASS(Action_سيٽ, VOptionBool, m_valp->سيٽ_True_Or_False(سچ), en::NONE);
#جيڪڏهن_پورو
V3OPTION_PARSER_DEF_ACT_CLASS(Action_سيٽ, صحيح, *m_valp = std::atoi(argp), en::قدر_3);
V3OPTION_PARSER_DEF_ACT_CLASS(Action_سيٽ, string, *m_valp = argp, en::قدر_3);

V3OPTION_PARSER_DEF_ACT_CLASS(ActionFOnOff, منطقي, *m_valp = !has_اڳياڙي_FNo(optp), en::FONOFF);
V3OPTION_PARSER_DEF_ACT_CLASS(ActionOnOff, منطقي, *m_valp = !has_اڳياڙي_No(optp), en::ONOFF);
#جيڪڏهن_وصف_نه_ٿيل V3OPTION_PARSER_NO_VOPTION_BOOL
V3OPTION_PARSER_DEF_ACT_CLASS(ActionOnOff, VOptionBool, m_valp->سيٽ_True_Or_False(!has_اڳياڙي_No(optp)),
                              en::ONOFF);
#جيڪڏهن_پورو
#وصف_رد V3OPTION_PARSER_DEF_ACT_CLASS

#وصف_ڪريو V3OPTION_PARSER_DEF_ACT_CB_CLASS(className, funcType, body, ...) \
    صنف V3OptionParser::Impl::className final : عام Action_بنياد<__VA_ARGS__> { \
        std::function<funcType> m_cb; /* Callback function */ \
\
    عام: \
        استعمال CbType = std::function<funcType>; \
        واضح className(CbType cb) \
            : m_cb(std::move(cb)) {} \
        فراغ exec(ثابت حرف* optp, ثابت حرف* argp) override { body; } \
    }

V3OPTION_PARSER_DEF_ACT_CB_CLASS(ActionCbCall, فراغ(فراغ), m_cb(), en::NONE);
V3OPTION_PARSER_DEF_ACT_CB_CLASS(ActionCbFOnOff, فراغ(منطقي), m_cb(!has_اڳياڙي_FNo(optp)),
                                 en::FONOFF);
V3OPTION_PARSER_DEF_ACT_CB_CLASS(ActionCbOnOff, فراغ(منطقي), m_cb(!has_اڳياڙي_No(optp)), en::ONOFF);
V3OPTION_PARSER_DEF_ACT_CB_CLASS(Action_Cb_Partial_ميلاپ, فراغ(ثابت حرف*), m_cb(optp), en::NONE,
                                 سچ);
V3OPTION_PARSER_DEF_ACT_CB_CLASS(Action_Cb_Partial_ميلاپ_Val, فراغ(ثابت حرف*, ثابت حرف*),
                                 m_cb(optp, argp), en::قدر_3, سچ);

سانچو <>
صنف V3OptionParser::Impl::ActionCbVal<صحيح> final : عام Action_بنياد<en::قدر_3> {
    std::function<فراغ(صحيح)> m_cb;  // Callback function

عام:
    استعمال CbType = std::function<فراغ(صحيح)>;
    واضح ActionCbVal(CbType cb)
        : m_cb{std::move(cb)} {}
    فراغ exec(ثابت حرف* optp, ثابت حرف* argp) override { m_cb(std::atoi(argp)); }
};

سانچو <>
صنف V3OptionParser::Impl::ActionCbVal<ثابت حرف*> final : عام Action_بنياد<en::قدر_3> {
    std::function<فراغ(ثابت حرف*)> m_cb;

عام:
    استعمال CbType = std::function<فراغ(ثابت حرف*)>;
    واضح ActionCbVal(CbType cb)
        : m_cb{std::move(cb)} {}
    فراغ exec(ثابت حرف* optp, ثابت حرف* argp) override { m_cb(argp); }
};

#وصف_رد V3OPTION_PARSER_DEF_ACT_CB_CLASS

//######################################################################
// Member functions of V3OptionParser

V3OptionParser::ActionIfs* V3OptionParser::ڳوليو(ثابت حرف* optp) {
    ثابت خودڪار it = m_pimpl->m_options.ڳوليو(optp);
    إذا (it != m_pimpl->m_options.پڇاڙي()) أرجع it->ٻيو.get();  // Exact match
    لكل (خودڪار&& act : m_pimpl->m_options) {
        إذا (act.ٻيو->isFOnOffAllowed()) {  // Find starts with "-fno"
            إذا (ثابت حرف* ثابت nop
                = VString::startsWith(optp, "-fno-") ? (optp + std::لڙي_ڊيگهه("-fno-")) : خالي_اشارو) {
                إذا (act.پهريون.substr(std::لڙي_ڊيگهه("-f"), std::string::npos)
                    == nop) {  // [-f]opt = [-fno-]opt
                    أرجع act.ٻيو.get();
                }
            }
        }
        إذا (act.ٻيو->isOnOffAllowed()) {  // Find starts with "-no"
            إذا (ثابت حرف* ثابت nop
                = VString::startsWith(optp, "-no") ? (optp + std::لڙي_ڊيگهه("-no")) : خالي_اشارو) {
                إذا (act.پهريون == nop || act.پهريون == ("-"s + nop)) أرجع act.ٻيو.get();
            }
        } وإلا إذا (act.ٻيو->is_Partial_ميلاپ_Allowed()) {
            إذا (VString::startsWith(optp, act.پهريون)) أرجع act.ٻيو.get();
        }
    }
    أرجع خالي_اشارو;
}

سانچو <قسم_نالو T_Act, قسم_نالو T_Arg>
V3OptionParser::ActionIfs& V3OptionParser::جوڙيو(ثابت std::string& opt, T_Arg arg) {
    UASSERT(!m_pimpl->m_isFinalized, "Cannot add after finalize() is called");
    std::unique_ptr<T_Act> act{جديد T_Act{std::move(arg)}};
    UASSERT(opt.سائيز() >= 2, opt << " is too short");
    UASSERT(opt[0] == '-' || opt[0] == '+', opt << " does not start with either '-' or '+'");
    UASSERT(!(opt[0] == '-' && opt[1] == '-'), "Option must have single '-', but " << opt);
    ثابت خودڪار inserted_نتيجو = m_pimpl->m_options.emplace(opt, std::move(act));
    UASSERT(inserted_نتيجو.ٻيو, opt << " is already registered");
    أرجع *inserted_نتيجو.پهريون->ٻيو;
}

منطقي V3OptionParser::has_اڳياڙي_FNo(ثابت حرف* strp) {
    UASSERT(strp[0] == '-', strp << " does not start with '-'");
    إذا (strp[1] == '-') ++strp;
    أرجع VString::startsWith(strp, "-fno");
}

منطقي V3OptionParser::has_اڳياڙي_No(ثابت حرف* strp) {
    UASSERT(strp[0] == '-', strp << " does not start with '-'");
    إذا (strp[1] == '-') ++strp;
    أرجع VString::startsWith(strp, "-no");
}

std::pair<صحيح, منطقي> V3OptionParser::parse(صحيح idx, صحيح argc, حرف* argv[]) {
    UASSERT(m_pimpl->m_isFinalized, "finalize() must be called before parse()");
    ثابت حرف* optp = argv[idx];
    إذا (optp[0] == '-' && optp[1] == '-') ++optp;
    ActionIfs* ثابت actp = ڳوليو(optp);
    إذا (!actp) أرجع {0, خطأ};
    إذا (!actp->is_قدر_Needed()) {
        actp->exec(optp, خالي_اشارو);
        أرجع {1, !actp->isNotForRerun()};
    }
    إذا (idx + 1 < argc) {
        actp->exec(optp, argv[idx + 1]);
        أرجع {2, !actp->isNotForRerun()};
    }
    أرجع {0, خطأ};
}

string V3OptionParser::getSuggestion(ثابت حرف* str) ثابت {
    أرجع m_pimpl->m_spellCheck.بهترين_Candidate_نياپو(str);
}

فراغ V3OptionParser::جوڙيو_Suggestion_Candidate(ثابت string& s) {
    m_pimpl->m_spellCheck.ڌڪو_Candidate(s);
}

فراغ V3OptionParser::finalize() {
    UASSERT(!m_pimpl->m_isFinalized, "finalize() must not be called twice");
    لكل (خودڪار&& opt : m_pimpl->m_options) {
        إذا (opt.ٻيو->isUndocumented()) استمر;
        m_pimpl->m_spellCheck.ڌڪو_Candidate(opt.پهريون);
        إذا (opt.ٻيو->isFOnOffAllowed()) {
            m_pimpl->m_spellCheck.ڌڪو_Candidate(
                "-fno-" + opt.پهريون.substr(std::لڙي_ڊيگهه("-f"), std::string::npos));
        }
        إذا (opt.ٻيو->isOnOffAllowed()) m_pimpl->m_spellCheck.ڌڪو_Candidate("-no" + opt.پهريون);
    }
    m_pimpl->m_isFinalized = سچ;
}

فراغ V3OptionParser::dumpOptions() {
    لكل (ثابت خودڪار& opt : m_pimpl->m_spellCheck.candidates()) {
        std::ٻاهر << "OPTION: \"" + opt + "\"\n";
    }
}

V3OptionParser::V3OptionParser()
    : m_pimpl{جديد Impl{}} {}

V3OptionParser::~V3OptionParser() = افتراضي;

//######################################################################
// Member functions of V3OptionParser::AppendHelper

#وصف_ڪريو V3OPTION_PARSER_DEF_OP(act_قسم, argType, actType) \
    V3OptionParser::ActionIfs& V3OptionParser::AppendHelper::عامل()( \
        ثابت حرف* optp, act_قسم, argType arg) ثابت { \
        أرجع m_parser.جوڙيو<Impl::actType>(optp, arg); \
    }
V3OPTION_PARSER_DEF_OP(سيٽ_2, منطقي*, Action_سيٽ<منطقي>)
#جيڪڏهن_وصف_نه_ٿيل V3OPTION_PARSER_NO_VOPTION_BOOL
V3OPTION_PARSER_DEF_OP(سيٽ_2, VOptionBool*, Action_سيٽ<VOptionBool>)
#جيڪڏهن_پورو
V3OPTION_PARSER_DEF_OP(سيٽ_2, صحيح*, Action_سيٽ<صحيح>)
V3OPTION_PARSER_DEF_OP(سيٽ_2, string*, Action_سيٽ<string>)
V3OPTION_PARSER_DEF_OP(FOnOff, منطقي*, ActionFOnOff<منطقي>)
V3OPTION_PARSER_DEF_OP(OnOff, منطقي*, ActionOnOff<منطقي>)
#جيڪڏهن_وصف_نه_ٿيل V3OPTION_PARSER_NO_VOPTION_BOOL
V3OPTION_PARSER_DEF_OP(OnOff, VOptionBool*, ActionOnOff<VOptionBool>)
#جيڪڏهن_پورو
V3OPTION_PARSER_DEF_OP(CbCall, Impl::ActionCbCall::CbType, ActionCbCall)
V3OPTION_PARSER_DEF_OP(CbFOnOff, Impl::ActionCbFOnOff::CbType, ActionCbFOnOff)
V3OPTION_PARSER_DEF_OP(CbOnOff, Impl::ActionCbOnOff::CbType, ActionCbOnOff)
V3OPTION_PARSER_DEF_OP(CbVal, Impl::ActionCbVal<صحيح>::CbType, ActionCbVal<صحيح>)
V3OPTION_PARSER_DEF_OP(CbVal, Impl::ActionCbVal<ثابت حرف*>::CbType, ActionCbVal<ثابت حرف*>)
#وصف_رد V3OPTION_PARSER_DEF_OP

V3OptionParser::ActionIfs&
V3OptionParser::AppendHelper::عامل()(ثابت حرف* optp, Cb_Partial_ميلاپ,
                                         Impl::Action_Cb_Partial_ميلاپ::CbType cb) ثابت {
    ثابت سائيز_t اڳياڙي_Len = std::لڙي_ڊيگهه(optp);
    ثابت خودڪار wrap = [اڳياڙي_Len, cb](ثابت حرف* optp) { cb(optp + اڳياڙي_Len); };
    أرجع m_parser.جوڙيو<Impl::Action_Cb_Partial_ميلاپ>(optp, std::move(wrap));
}

V3OptionParser::ActionIfs&
V3OptionParser::AppendHelper::عامل()(ثابت حرف* optp, Cb_Partial_ميلاپ_Val,
                                         Impl::Action_Cb_Partial_ميلاپ_Val::CbType cb) ثابت {
    ثابت سائيز_t اڳياڙي_Len = std::لڙي_ڊيگهه(optp);
    ثابت خودڪار wrap
        = [اڳياڙي_Len, cb](ثابت حرف* optp, ثابت حرف* argp) { cb(optp + اڳياڙي_Len, argp); };
    أرجع m_parser.جوڙيو<Impl::Action_Cb_Partial_ميلاپ_Val>(optp, std::move(wrap));
}
