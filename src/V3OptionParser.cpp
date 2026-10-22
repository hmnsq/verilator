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

#ವ್ಯಾಖ್ಯಾನಿತವಾಗದಿದ್ದರೆ V3OPTION_PARSER_NO_VOPTION_BOOL
#ವ್ಯಾಖ್ಯಾನಿಸು VL_MT_DISABLED_ಸಂಕೇತ_UNIT 1

#ಅನ್ತರ್ಭಾವ "V3Global.h"
#ಅನ್ತರ್ಭಾವ "V3Options.h"
#ಆದರೆ_ಅಂತ್ಯ
#ಅನ್ತರ್ಭಾವ "V3Error.h"
#ಅನ್ತರ್ಭಾವ "V3OptionParser.h"
#ಅನ್ತರ್ಭಾವ "V3String.h"

//######################################################################
// V3OptionParser::Impl
ರಚನೆ V3OptionParser::Impl final {
    // TYPES

    // Setting for isOnOffAllowed() and isPartialMatchAllowed()
    ಎಣಿಕೆ ವರ್ಗ en : uint8_t {
        NONE,  // "-opt"
        FONOFF,  // "-fopt" and "-fno-opt"
        ONOFF,  // "-opt" and "-no-opt"
        ಮೌಲ್ಯ_3  // "-opt val"
    };
    // Base class of actual action classes
    ಮಾದರಿ <en N_ಬಹುಲಕ, ತಾರ್ಕಿಕ N_Allow_Partial_ಹೊಂದಿಕೆ = ಸುಳ್ಳು>
    ವರ್ಗ Action_ತಳ VL_NOT_FINAL : ಸಾರ್ವಜನಿಕ ActionIfs {
        ತಾರ್ಕಿಕ m_notForRerun = ಸುಳ್ಳು;  // This option is not needed for rerun with --dump-inputs
        ತಾರ್ಕಿಕ m_undocumented = ಸುಳ್ಳು;  // This option is not documented
    ಸಾರ್ವಜನಿಕ:
        ತಾರ್ಕಿಕ is_ಮೌಲ್ಯ_Needed() ಸ್ಥಿರಾಂಕ override final { ಹಿಂತಿರುಗಿ N_ಬಹುಲಕ == en::ಮೌಲ್ಯ_3; }
        ತಾರ್ಕಿಕ isFOnOffAllowed() ಸ್ಥಿರಾಂಕ override final { ಹಿಂತಿರುಗಿ N_ಬಹುಲಕ == en::FONOFF; }
        ತಾರ್ಕಿಕ isNotForRerun() ಸ್ಥಿರಾಂಕ override final { ಹಿಂತಿರುಗಿ m_notForRerun; }
        ತಾರ್ಕಿಕ isOnOffAllowed() ಸ್ಥಿರಾಂಕ override final { ಹಿಂತಿರುಗಿ N_ಬಹುಲಕ == en::ONOFF; }
        ತಾರ್ಕಿಕ is_Partial_ಹೊಂದಿಕೆ_Allowed() ಸ್ಥಿರಾಂಕ override final { ಹಿಂತಿರುಗಿ N_Allow_Partial_ಹೊಂದಿಕೆ; }
        ತಾರ್ಕಿಕ isUndocumented() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ m_undocumented; }
        ActionIfs& undocumented() override {
            m_undocumented = ನಿಜ;
            ಹಿಂತಿರುಗಿ *ಇದು;
        }
        ActionIfs& notForRerun() override {
            m_notForRerun = ನಿಜ;
            ಹಿಂತಿರುಗಿ *ಇದು;
        }
    };

    // Actual action classes
    ಮಾದರಿ <ಬಗೆ_ಹೆಸರು T>
    ವರ್ಗ Action_ಗಣ;  // "-opt" for bool-ish, "-opt val" for int and string
    ಮಾದರಿ <ಬಗೆ_ಹೆಸರು N_Bool>
    ವರ್ಗ ActionFOnOff;  // "-fopt" and "-fno-opt" for bool-ish
    ಮಾದರಿ <ಬಗೆ_ಹೆಸರು N_Bool>
    ವರ್ಗ ActionOnOff;  // "-opt" and "-no-opt" for bool-ish
    ವರ್ಗ ActionCbCall;  // Callback without argument for "-opt"
    ವರ್ಗ ActionCbFOnOff;  // Callback for "-fopt" and "-fno-opt"
    ವರ್ಗ ActionCbOnOff;  // Callback for "-opt" and "-no-opt"
    ಮಾದರಿ <ಬಗೆ_ಹೆಸರು T>
    ವರ್ಗ ActionCbVal;  // Callback for "-opt val"
    ವರ್ಗ Action_Cb_Partial_ಹೊಂದಿಕೆ;  // Callback "-O3" for "-O"
    ವರ್ಗ Action_Cb_Partial_ಹೊಂದಿಕೆ_Val;  // Callback "-debugi-V3Options 3" for "-debugi-"

    // MEMBERS
    std::map<ಸ್ಥಿರಾಂಕ string, std::unique_ptr<ActionIfs>> m_options;  // All actions for option
    ತಾರ್ಕಿಕ m_isFinalized{ಸುಳ್ಳು};  // Becomes after finalize() is called
    VSpellCheck m_spellCheck;  // Suggests closest option when not found
};

//######################################################################
// Action classes in V3OptionParser::Impl

#ವ್ಯಾಖ್ಯಾನಿಸು V3OPTION_PARSER_DEF_ACT_CLASS(className, type, body, enType) \
    ಮಾದರಿ <> \
    ವರ್ಗ V3OptionParser::Impl::className<type> final : ಸಾರ್ವಜನಿಕ Action_ತಳ<enType> { \
        type* ಸ್ಥಿರಾಂಕ m_valp; /* Pointer to an option variable*/ \
\
    ಸಾರ್ವಜನಿಕ: \
        ಸ್ಪಷ್ಟ className(type* valp) \
            : m_valp(valp) {} \
        ಶೂನ್ಯ exec(ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* optp, ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* argp) override { body; } \
    }

V3OPTION_PARSER_DEF_ACT_CLASS(Action_ಗಣ, ತಾರ್ಕಿಕ, *m_valp = ನಿಜ, en::NONE);
#ವ್ಯಾಖ್ಯಾನಿತವಾಗದಿದ್ದರೆ V3OPTION_PARSER_NO_VOPTION_BOOL
V3OPTION_PARSER_DEF_ACT_CLASS(Action_ಗಣ, VOptionBool, m_valp->ಗಣ_True_Or_False(ನಿಜ), en::NONE);
#ಆದರೆ_ಅಂತ್ಯ
V3OPTION_PARSER_DEF_ACT_CLASS(Action_ಗಣ, ಪೂರ್ಣಾಂಕ, *m_valp = std::atoi(argp), en::ಮೌಲ್ಯ_3);
V3OPTION_PARSER_DEF_ACT_CLASS(Action_ಗಣ, string, *m_valp = argp, en::ಮೌಲ್ಯ_3);

V3OPTION_PARSER_DEF_ACT_CLASS(ActionFOnOff, ತಾರ್ಕಿಕ, *m_valp = !has_ಪೂರ್ವಪ್ರತ್ಯಯ_FNo(optp), en::FONOFF);
V3OPTION_PARSER_DEF_ACT_CLASS(ActionOnOff, ತಾರ್ಕಿಕ, *m_valp = !has_ಪೂರ್ವಪ್ರತ್ಯಯ_No(optp), en::ONOFF);
#ವ್ಯಾಖ್ಯಾನಿತವಾಗದಿದ್ದರೆ V3OPTION_PARSER_NO_VOPTION_BOOL
V3OPTION_PARSER_DEF_ACT_CLASS(ActionOnOff, VOptionBool, m_valp->ಗಣ_True_Or_False(!has_ಪೂರ್ವಪ್ರತ್ಯಯ_No(optp)),
                              en::ONOFF);
#ಆದರೆ_ಅಂತ್ಯ
#ವ್ಯಾಖ್ಯಾನ_ರದ್ದು V3OPTION_PARSER_DEF_ACT_CLASS

#ವ್ಯಾಖ್ಯಾನಿಸು V3OPTION_PARSER_DEF_ACT_CB_CLASS(className, funcType, body, ...) \
    ವರ್ಗ V3OptionParser::Impl::className final : ಸಾರ್ವಜನಿಕ Action_ತಳ<__VA_ARGS__> { \
        std::function<funcType> m_cb; /* Callback function */ \
\
    ಸಾರ್ವಜನಿಕ: \
        ಬಳಸಿ CbType = std::function<funcType>; \
        ಸ್ಪಷ್ಟ className(CbType cb) \
            : m_cb(std::move(cb)) {} \
        ಶೂನ್ಯ exec(ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* optp, ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* argp) override { body; } \
    }

V3OPTION_PARSER_DEF_ACT_CB_CLASS(ActionCbCall, ಶೂನ್ಯ(ಶೂನ್ಯ), m_cb(), en::NONE);
V3OPTION_PARSER_DEF_ACT_CB_CLASS(ActionCbFOnOff, ಶೂನ್ಯ(ತಾರ್ಕಿಕ), m_cb(!has_ಪೂರ್ವಪ್ರತ್ಯಯ_FNo(optp)),
                                 en::FONOFF);
V3OPTION_PARSER_DEF_ACT_CB_CLASS(ActionCbOnOff, ಶೂನ್ಯ(ತಾರ್ಕಿಕ), m_cb(!has_ಪೂರ್ವಪ್ರತ್ಯಯ_No(optp)), en::ONOFF);
V3OPTION_PARSER_DEF_ACT_CB_CLASS(Action_Cb_Partial_ಹೊಂದಿಕೆ, ಶೂನ್ಯ(ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ*), m_cb(optp), en::NONE,
                                 ನಿಜ);
V3OPTION_PARSER_DEF_ACT_CB_CLASS(Action_Cb_Partial_ಹೊಂದಿಕೆ_Val, ಶೂನ್ಯ(ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ*, ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ*),
                                 m_cb(optp, argp), en::ಮೌಲ್ಯ_3, ನಿಜ);

ಮಾದರಿ <>
ವರ್ಗ V3OptionParser::Impl::ActionCbVal<ಪೂರ್ಣಾಂಕ> final : ಸಾರ್ವಜನಿಕ Action_ತಳ<en::ಮೌಲ್ಯ_3> {
    std::function<ಶೂನ್ಯ(ಪೂರ್ಣಾಂಕ)> m_cb;  // Callback function

ಸಾರ್ವಜನಿಕ:
    ಬಳಸಿ CbType = std::function<ಶೂನ್ಯ(ಪೂರ್ಣಾಂಕ)>;
    ಸ್ಪಷ್ಟ ActionCbVal(CbType cb)
        : m_cb{std::move(cb)} {}
    ಶೂನ್ಯ exec(ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* optp, ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* argp) override { m_cb(std::atoi(argp)); }
};

ಮಾದರಿ <>
ವರ್ಗ V3OptionParser::Impl::ActionCbVal<ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ*> final : ಸಾರ್ವಜನಿಕ Action_ತಳ<en::ಮೌಲ್ಯ_3> {
    std::function<ಶೂನ್ಯ(ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ*)> m_cb;

ಸಾರ್ವಜನಿಕ:
    ಬಳಸಿ CbType = std::function<ಶೂನ್ಯ(ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ*)>;
    ಸ್ಪಷ್ಟ ActionCbVal(CbType cb)
        : m_cb{std::move(cb)} {}
    ಶೂನ್ಯ exec(ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* optp, ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* argp) override { m_cb(argp); }
};

#ವ್ಯಾಖ್ಯಾನ_ರದ್ದು V3OPTION_PARSER_DEF_ACT_CB_CLASS

//######################################################################
// Member functions of V3OptionParser

V3OptionParser::ActionIfs* V3OptionParser::ಕಂಡುಹಿಡಿ(ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* optp) {
    ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ it = m_pimpl->m_options.ಕಂಡುಹಿಡಿ(optp);
    ಆದರೆ (it != m_pimpl->m_options.ಕೊನೆ()) ಹಿಂತಿರುಗಿ it->ಎರಡನೇ.get();  // Exact match
    ಗಾಗಿ (ಸ್ವಯಂಚಾಲಿತ&& act : m_pimpl->m_options) {
        ಆದರೆ (act.ಎರಡನೇ->isFOnOffAllowed()) {  // Find starts with "-fno"
            ಆದರೆ (ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* ಸ್ಥಿರಾಂಕ nop
                = VString::startsWith(optp, "-fno-") ? (optp + std::ಅಕ್ಷರಸರಣಿ_ಉದ್ದ("-fno-")) : ಶೂನ್ಯಸೂಚಕ) {
                ಆದರೆ (act.ಮೊದಲ.substr(std::ಅಕ್ಷರಸರಣಿ_ಉದ್ದ("-f"), std::string::npos)
                    == nop) {  // [-f]opt = [-fno-]opt
                    ಹಿಂತಿರುಗಿ act.ಎರಡನೇ.get();
                }
            }
        }
        ಆದರೆ (act.ಎರಡನೇ->isOnOffAllowed()) {  // Find starts with "-no"
            ಆದರೆ (ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* ಸ್ಥಿರಾಂಕ nop
                = VString::startsWith(optp, "-no") ? (optp + std::ಅಕ್ಷರಸರಣಿ_ಉದ್ದ("-no")) : ಶೂನ್ಯಸೂಚಕ) {
                ಆದರೆ (act.ಮೊದಲ == nop || act.ಮೊದಲ == ("-"s + nop)) ಹಿಂತಿರುಗಿ act.ಎರಡನೇ.get();
            }
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (act.ಎರಡನೇ->is_Partial_ಹೊಂದಿಕೆ_Allowed()) {
            ಆದರೆ (VString::startsWith(optp, act.ಮೊದಲ)) ಹಿಂತಿರುಗಿ act.ಎರಡನೇ.get();
        }
    }
    ಹಿಂತಿರುಗಿ ಶೂನ್ಯಸೂಚಕ;
}

ಮಾದರಿ <ಬಗೆ_ಹೆಸರು T_Act, ಬಗೆ_ಹೆಸರು T_Arg>
V3OptionParser::ActionIfs& V3OptionParser::ಕೂಡಿಸು(ಸ್ಥಿರಾಂಕ std::string& opt, T_Arg arg) {
    UASSERT(!m_pimpl->m_isFinalized, "Cannot add after finalize() is called");
    std::unique_ptr<T_Act> act{ಹೊಸ T_Act{std::move(arg)}};
    UASSERT(opt.ಗಾತ್ರ_2() >= 2, opt << " is too short");
    UASSERT(opt[0] == '-' || opt[0] == '+', opt << " does not start with either '-' or '+'");
    UASSERT(!(opt[0] == '-' && opt[1] == '-'), "Option must have single '-', but " << opt);
    ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ inserted_ಫಲಿತಾಂಶ = m_pimpl->m_options.emplace(opt, std::move(act));
    UASSERT(inserted_ಫಲಿತಾಂಶ.ಎರಡನೇ, opt << " is already registered");
    ಹಿಂತಿರುಗಿ *inserted_ಫಲಿತಾಂಶ.ಮೊದಲ->ಎರಡನೇ;
}

ತಾರ್ಕಿಕ V3OptionParser::has_ಪೂರ್ವಪ್ರತ್ಯಯ_FNo(ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* strp) {
    UASSERT(strp[0] == '-', strp << " does not start with '-'");
    ಆದರೆ (strp[1] == '-') ++strp;
    ಹಿಂತಿರುಗಿ VString::startsWith(strp, "-fno");
}

ತಾರ್ಕಿಕ V3OptionParser::has_ಪೂರ್ವಪ್ರತ್ಯಯ_No(ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* strp) {
    UASSERT(strp[0] == '-', strp << " does not start with '-'");
    ಆದರೆ (strp[1] == '-') ++strp;
    ಹಿಂತಿರುಗಿ VString::startsWith(strp, "-no");
}

std::pair<ಪೂರ್ಣಾಂಕ, ತಾರ್ಕಿಕ> V3OptionParser::parse(ಪೂರ್ಣಾಂಕ idx, ಪೂರ್ಣಾಂಕ argc, ಅಕ್ಷರ* argv[]) {
    UASSERT(m_pimpl->m_isFinalized, "finalize() must be called before parse()");
    ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* optp = argv[idx];
    ಆದರೆ (optp[0] == '-' && optp[1] == '-') ++optp;
    ActionIfs* ಸ್ಥಿರಾಂಕ actp = ಕಂಡುಹಿಡಿ(optp);
    ಆದರೆ (!actp) ಹಿಂತಿರುಗಿ {0, ಸುಳ್ಳು};
    ಆದರೆ (!actp->is_ಮೌಲ್ಯ_Needed()) {
        actp->exec(optp, ಶೂನ್ಯಸೂಚಕ);
        ಹಿಂತಿರುಗಿ {1, !actp->isNotForRerun()};
    }
    ಆದರೆ (idx + 1 < argc) {
        actp->exec(optp, argv[idx + 1]);
        ಹಿಂತಿರುಗಿ {2, !actp->isNotForRerun()};
    }
    ಹಿಂತಿರುಗಿ {0, ಸುಳ್ಳು};
}

string V3OptionParser::getSuggestion(ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* str) ಸ್ಥಿರಾಂಕ {
    ಹಿಂತಿರುಗಿ m_pimpl->m_spellCheck.ಉತ್ತಮ_Candidate_ಸಂದೇಶ(str);
}

ಶೂನ್ಯ V3OptionParser::ಕೂಡಿಸು_Suggestion_Candidate(ಸ್ಥಿರಾಂಕ string& s) {
    m_pimpl->m_spellCheck.ತಳ್ಳು_Candidate(s);
}

ಶೂನ್ಯ V3OptionParser::finalize() {
    UASSERT(!m_pimpl->m_isFinalized, "finalize() must not be called twice");
    ಗಾಗಿ (ಸ್ವಯಂಚಾಲಿತ&& opt : m_pimpl->m_options) {
        ಆದರೆ (opt.ಎರಡನೇ->isUndocumented()) ಮುಂದುವರಿ;
        m_pimpl->m_spellCheck.ತಳ್ಳು_Candidate(opt.ಮೊದಲ);
        ಆದರೆ (opt.ಎರಡನೇ->isFOnOffAllowed()) {
            m_pimpl->m_spellCheck.ತಳ್ಳು_Candidate(
                "-fno-" + opt.ಮೊದಲ.substr(std::ಅಕ್ಷರಸರಣಿ_ಉದ್ದ("-f"), std::string::npos));
        }
        ಆದರೆ (opt.ಎರಡನೇ->isOnOffAllowed()) m_pimpl->m_spellCheck.ತಳ್ಳು_Candidate("-no" + opt.ಮೊದಲ);
    }
    m_pimpl->m_isFinalized = ನಿಜ;
}

ಶೂನ್ಯ V3OptionParser::dumpOptions() {
    ಗಾಗಿ (ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ& opt : m_pimpl->m_spellCheck.candidates()) {
        std::ಹೊರಗೆ << "OPTION: \"" + opt + "\"\n";
    }
}

V3OptionParser::V3OptionParser()
    : m_pimpl{ಹೊಸ Impl{}} {}

V3OptionParser::~V3OptionParser() = ಪೂರ್ವನಿಯೋಜಿತ;

//######################################################################
// Member functions of V3OptionParser::AppendHelper

#ವ್ಯಾಖ್ಯಾನಿಸು V3OPTION_PARSER_DEF_OP(act_ಬಗೆ, argType, actType) \
    V3OptionParser::ActionIfs& V3OptionParser::AppendHelper::ಕ್ರಿಯಾಕಾರಕ()( \
        ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* optp, act_ಬಗೆ, argType arg) ಸ್ಥಿರಾಂಕ { \
        ಹಿಂತಿರುಗಿ m_parser.ಕೂಡಿಸು<Impl::actType>(optp, arg); \
    }
V3OPTION_PARSER_DEF_OP(ಗಣ_2, ತಾರ್ಕಿಕ*, Action_ಗಣ<ತಾರ್ಕಿಕ>)
#ವ್ಯಾಖ್ಯಾನಿತವಾಗದಿದ್ದರೆ V3OPTION_PARSER_NO_VOPTION_BOOL
V3OPTION_PARSER_DEF_OP(ಗಣ_2, VOptionBool*, Action_ಗಣ<VOptionBool>)
#ಆದರೆ_ಅಂತ್ಯ
V3OPTION_PARSER_DEF_OP(ಗಣ_2, ಪೂರ್ಣಾಂಕ*, Action_ಗಣ<ಪೂರ್ಣಾಂಕ>)
V3OPTION_PARSER_DEF_OP(ಗಣ_2, string*, Action_ಗಣ<string>)
V3OPTION_PARSER_DEF_OP(FOnOff, ತಾರ್ಕಿಕ*, ActionFOnOff<ತಾರ್ಕಿಕ>)
V3OPTION_PARSER_DEF_OP(OnOff, ತಾರ್ಕಿಕ*, ActionOnOff<ತಾರ್ಕಿಕ>)
#ವ್ಯಾಖ್ಯಾನಿತವಾಗದಿದ್ದರೆ V3OPTION_PARSER_NO_VOPTION_BOOL
V3OPTION_PARSER_DEF_OP(OnOff, VOptionBool*, ActionOnOff<VOptionBool>)
#ಆದರೆ_ಅಂತ್ಯ
V3OPTION_PARSER_DEF_OP(CbCall, Impl::ActionCbCall::CbType, ActionCbCall)
V3OPTION_PARSER_DEF_OP(CbFOnOff, Impl::ActionCbFOnOff::CbType, ActionCbFOnOff)
V3OPTION_PARSER_DEF_OP(CbOnOff, Impl::ActionCbOnOff::CbType, ActionCbOnOff)
V3OPTION_PARSER_DEF_OP(CbVal, Impl::ActionCbVal<ಪೂರ್ಣಾಂಕ>::CbType, ActionCbVal<ಪೂರ್ಣಾಂಕ>)
V3OPTION_PARSER_DEF_OP(CbVal, Impl::ActionCbVal<ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ*>::CbType, ActionCbVal<ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ*>)
#ವ್ಯಾಖ್ಯಾನ_ರದ್ದು V3OPTION_PARSER_DEF_OP

V3OptionParser::ActionIfs&
V3OptionParser::AppendHelper::ಕ್ರಿಯಾಕಾರಕ()(ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* optp, Cb_Partial_ಹೊಂದಿಕೆ,
                                         Impl::Action_Cb_Partial_ಹೊಂದಿಕೆ::CbType cb) ಸ್ಥಿರಾಂಕ {
    ಸ್ಥಿರಾಂಕ ಗಾತ್ರ_t ಪೂರ್ವಪ್ರತ್ಯಯ_Len = std::ಅಕ್ಷರಸರಣಿ_ಉದ್ದ(optp);
    ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ wrap = [ಪೂರ್ವಪ್ರತ್ಯಯ_Len, cb](ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* optp) { cb(optp + ಪೂರ್ವಪ್ರತ್ಯಯ_Len); };
    ಹಿಂತಿರುಗಿ m_parser.ಕೂಡಿಸು<Impl::Action_Cb_Partial_ಹೊಂದಿಕೆ>(optp, std::move(wrap));
}

V3OptionParser::ActionIfs&
V3OptionParser::AppendHelper::ಕ್ರಿಯಾಕಾರಕ()(ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* optp, Cb_Partial_ಹೊಂದಿಕೆ_Val,
                                         Impl::Action_Cb_Partial_ಹೊಂದಿಕೆ_Val::CbType cb) ಸ್ಥಿರಾಂಕ {
    ಸ್ಥಿರಾಂಕ ಗಾತ್ರ_t ಪೂರ್ವಪ್ರತ್ಯಯ_Len = std::ಅಕ್ಷರಸರಣಿ_ಉದ್ದ(optp);
    ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ wrap
        = [ಪೂರ್ವಪ್ರತ್ಯಯ_Len, cb](ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* optp, ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* argp) { cb(optp + ಪೂರ್ವಪ್ರತ್ಯಯ_Len, argp); };
    ಹಿಂತಿರುಗಿ m_parser.ಕೂಡಿಸು<Impl::Action_Cb_Partial_ಹೊಂದಿಕೆ_Val>(optp, std::move(wrap));
}
