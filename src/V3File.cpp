// -*- mode: C++; c-file-style: "cc-mode" -*-
//*************************************************************************
// DESCRIPTION: Verilator: File stream wrapper that understands indentation
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

#ضمن "V3PchAstMT.h"

#ضمن "V3File.h"

#ضمن "V3Os.h"
#ضمن "V3String.h"

#ضمن <cerrno>
#ضمن <cstdarg>
#ضمن <fcntl.h>
#ضمن <iomanip>
#ضمن <map>
#ضمن <memory>

#ضمن <sys/stat.h>
#ضمن <sys/types.h>

// clang-format off
#إذا defined(__unix__) || defined(__unix) || (defined(__APPLE__) && defined(__MACH__)) || defined(VL_CPPCHECK)
# define INFILTER_PIPE  // Allow pipe filtering.  Needs fork()
#پایان_اگر

#اگر_تعریف_شده HAVE_STAT_NSEC  // i.e. Linux 2.6, from configure
# define VL_STAT_CTIME_NSEC(stat) ((stat).st_ctim.tv_nsec)  // Nanoseconds
# define VL_STAT_MTIME_NSEC(stat) ((stat).st_mtim.tv_nsec)  // Nanoseconds
#وإلا
# define VL_STAT_CTIME_NSEC(stat) (0)
# define VL_STAT_MTIME_NSEC(stat) (0)
#پایان_اگر

#اگر_تعریف_شده INFILTER_PIPE
# include <sys/wait.h>
#پایان_اگر

#إذا defined(_WIN32) || defined(__MINGW32__)
# include <io.h>  // open, read, write, close
#پایان_اگر
// clang-format on

VL_DEFINE_DEBUG_FUNCTIONS;

// If change this code, run a test with the below size set very small
// #define INFILTER_IPC_BUFSIZ 16
constexpr صحيح INFILTER_IPC_BUFSIZ = (64 * 1024);  // For debug, try this as a small number
constexpr صحيح INFILTER_CACHE_MAX = (64 * 1024);  // Maximum bytes to cache if same file read twice

constexpr off_t FILE_HASH_اندازه_MAX = 1 * 1024 * 1024;  // Maxium size file to hash

//######################################################################
// V3File Internal state

صنف V3FileDependImp final {
    // TYPES
    صنف DependFile final {
        // A single file
        ثابت منطقي m_هدف;  // True if write, else read
        منطقي m_exists = درست;  // File exists
        ثابت string m_filename;  // Filename
        بنية stat m_stat;  // Stat information
        VHashSha512 m_hash;  // SHA hash of file contents
    عام:
        DependFile(ثابت string& filename, منطقي هدف)
            : m_هدف{هدف}
            , m_filename{filename} {
            m_stat.st_ctime = 0;
            m_stat.st_mtime = 0;
        }
        ~DependFile() = افتراضي;
        ثابت string& filename() ثابت { أرجع m_filename; }
        منطقي هدف() ثابت { أرجع m_هدف; }
        منطقي exists() ثابت { أرجع m_exists; }
        off_t اندازه() ثابت { أرجع m_stat.st_اندازه; }
        ino_t ino() ثابت { أرجع m_stat.st_ino; }
        زمان_t cstime() ثابت { أرجع m_stat.st_ctime; }  // Seconds
        زمان_t cnstime() ثابت { أرجع VL_STAT_CTIME_NSEC(m_stat); }  // Nanoseconds
        زمان_t mstime() ثابت { أرجع m_stat.st_mtime; }  // Seconds
        زمان_t mnstime() ثابت { أرجع VL_STAT_MTIME_NSEC(m_stat); }  // Nanoseconds
        string hash_Digest_نماد() {
            ساكن VHashSha512 s_خالی_Hash;
            أرجع m_hash.digestSymbol24() != s_خالی_Hash.digestSymbol24()
                       ? m_hash.digestSymbol24()
                       : "unhashed";
        }
        فراغ بارگیری_Stats() {
            إذا (!m_stat.st_mtime) {
                ثابت string fn = filename();
                ثابت صحيح err = stat(fn.c_str(), &m_stat);
                إذا (err != 0) {
                    std::memset(&m_stat, 0, حجم(m_stat));
                    m_stat.st_mtime = 1;
                    m_exists = خطأ;
                    // Not an error... This can occur due to `line directives in the .vpp files
                    UINFO(1, "-Info: File not statable: " << filename());
                } وإلا {
                    // For speed, don't hash large files (e.g. verilator_bin)
                    إذا (!هدف() && اندازه() <= FILE_HASH_اندازه_MAX) m_hash.درج_File(filename());
                }
            }
        }
        منطقي عملگر<(ثابت DependFile& rhs) ثابت { أرجع filename() < rhs.filename(); }
    };

    // MEMBERS
    V3Mutex m_mutex;  // Protects members
    std::مجموعه<string> m_filename_مجموعه VL_GUARDED_BY(m_mutex);  // Files generated (elim duplicates)
    std::مجموعه<DependFile> m_filenameList;  // Files sourced/generated

    ساكن string stripQuotes(ثابت string& درون) {
        string pretty = درون;
        string::اندازه_type pos;
        طالما ((pos = pretty.پیدا_کن('\"')) != string::npos) pretty.replace(pos, 1, "_");
        طالما ((pos = pretty.پیدا_کن('\n')) != string::npos) pretty.replace(pos, 1, "_");
        أرجع pretty;
    }

عام:
    // ACCESSOR METHODS
    فراغ جمع_کن_Src_Depend(ثابت string& filename) VL_MT_امن_EXCLUDES(m_mutex) {
        ثابت V3_قفل_Guard قفل_2{m_mutex};
        ثابت خودکار it_یافت_Pair = m_filename_مجموعه.درج(filename);
        إذا (it_یافت_Pair.دومی) {
            DependFile df{filename, خطأ};
            df.بارگیری_Stats();  // Get size now, in case changes during the run
            m_filenameList.درج(df);
        }
    }
    فراغ جمع_کن_Tgt_Depend(ثابت string& filename) VL_MT_امن_EXCLUDES(m_mutex) {
        ثابت V3_قفل_Guard قفل_2{m_mutex};
        ثابت خودکار it_یافت_Pair = m_filename_مجموعه.درج(filename);
        إذا (it_یافت_Pair.دومی) m_filenameList.درج(DependFile{filename, درست});
    }
    فراغ writeDepend(ثابت string& filename);
    std::وکتور<string> getAllDeps() ثابت;
    فراغ writeTimes(ثابت string& filename, ثابت string& cmdline_درون);
    منطقي checkTimes(ثابت string& filename, ثابت string& cmdline_درون);
};

V3FileDependImp dependImp;  // Depend implementation class

//######################################################################
// V3FileDependImp

فراغ V3FileDependImp::writeDepend(ثابت string& filename) {
    ثابت std::unique_ptr<std::ofstream> ofp{V3File::new_ofstream(filename)};
    إذا (ofp->fail()) v3fatal("Can't write file: " << filename);

    لكل (ثابت DependFile& i : m_filenameList) {
        إذا (i.هدف()) *ofp << i.filename() << " ";
    }
    *ofp << " : ";
    *ofp << v3Global.opt.buildDepBin();
    *ofp << " ";

    لكل (ثابت DependFile& i : m_filenameList) {
        إذا (!i.هدف()) *ofp << i.filename() << " ";
    }

    *ofp << '\n';

    إذا (v3Global.opt.makePhony()) {
        *ofp << '\n';
        لكل (ثابت DependFile& i : m_filenameList) {
            إذا (!i.هدف()) *ofp << i.filename() << ":\n";
        }
    }
}

std::وکتور<string> V3FileDependImp::getAllDeps() ثابت {
    std::وکتور<string> r;
    لكل (ثابت خودکار& itr : m_filenameList) {
        إذا (!itr.هدف() && itr.exists()) r.افزودن_پشت(itr.filename());
    }
    أرجع r;
}

فراغ V3FileDependImp::writeTimes(ثابت string& filename, ثابت string& cmdline_درون) {
    ثابت std::unique_ptr<std::ofstream> ofp{V3File::new_ofstream(filename)};
    إذا (ofp->fail()) v3fatal("Can't write file: " << filename);

    ثابت string cmdline = stripQuotes(cmdline_درون);
    *ofp << "# DESCR"
         << "IPTION: Verilator output: Timestamp data for --skip-identical.  Delete at will.\n";
    *ofp << "C \"" << cmdline << "\"\n";

    لكل (خودکار& it : m_filenameList) {
        DependFile* ثابت depfilep = const_cast<DependFile*>(&it);
        // Read stats of files we create after we're done making them
        // (except for this file, of course)
        V3Os::filesystemFlush(depfilep->filename());
        depfilep->بارگیری_Stats();
        off_t show_اندازه = depfilep->اندازه();
        ino_t showIno = depfilep->ino();
        إذا (depfilep->filename() == filename) {
            show_اندازه = 0;
            showIno = 0;  // We're writing it, so need to ignore it
        }

        *ofp << (depfilep->هدف() ? "T" : "S") << " ";
        *ofp << " " << std::setw(8) << show_اندازه;
        *ofp << " " << std::setw(8) << showIno;
        *ofp << " " << std::setw(11) << depfilep->cstime();
        *ofp << " " << std::setw(11) << depfilep->cnstime();
        *ofp << " " << std::setw(11) << depfilep->mstime();
        *ofp << " " << std::setw(11) << depfilep->mnstime();
        *ofp << " \"" << depfilep->hash_Digest_نماد() << "\"";
        *ofp << " \"" << depfilep->filename() << "\"";
        *ofp << '\n';
    }
}

منطقي V3FileDependImp::checkTimes(ثابت string& filename, ثابت string& cmdline_درون) {
    ثابت std::unique_ptr<std::ifstream> ifp{V3File::new_ifstream_nodepend(filename)};
    إذا (ifp->fail()) {
        UINFO(2, "   --check-times failed: no input " << filename);
        أرجع خطأ;
    }
    {
        ثابت string ignore = V3Os::getline(*ifp);
        إذا (ignore.خالی()) { /*استفاده_شده*/
        }
    }
    {
        حرف chkDir;
        *ifp >> chkDir;
        حرف quote;
        *ifp >> quote;
        ثابت string chkCmdline = V3Os::getline(*ifp, '"');
        ثابت string cmdline = stripQuotes(cmdline_درون);
        إذا (cmdline != chkCmdline) {
            UINFO(2, "   --check-times failed: different command line");
            أرجع خطأ;
        }
    }

    ثابت منطقي skipHashing = !V3Os::getenvStr("VERILATOR_DEBUG_SKIP_HASH", "").خالی();

    طالما (!ifp->eof()) {
        حرف chkDir;
        *ifp >> chkDir;
        off_t chk_اندازه;
        *ifp >> chk_اندازه;
        ino_t chkIno;
        *ifp >> chkIno;
        إذا (ifp->eof()) اكسر;  // Needed to read final whitespace before found eof
        زمان_t chkCstime;
        *ifp >> chkCstime;
        زمان_t chkCnstime;
        *ifp >> chkCnstime;
        زمان_t chkMstime;
        *ifp >> chkMstime;
        زمان_t chkMnstime;
        *ifp >> chkMnstime;
        حرف quote;
        *ifp >> quote;
        ثابت string chkHash = V3Os::getline(*ifp, '"');
        *ifp >> quote;
        ثابت string chkFilename = V3Os::getline(*ifp, '"');

        V3Os::filesystemFlush(chkFilename);
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-member-init)
        بنية stat curStat;
        ثابت صحيح err = stat(chkFilename.c_str(), &curStat);
        إذا (err != 0) {
            UINFO(2, "   --check-times failed: missing " << chkFilename);
            أرجع خطأ;
        }

        // Skip the .dat file itself, as we were writing it at the time was being made
        إذا (filename == chkFilename) استمر;

        UINFO(9, " got d=" << chkDir << " s=" << chk_اندازه << " ct=" << chkCstime << "."
                           << chkCnstime << " mt=" << chkMstime << "." << chkMnstime
                           << " h=" << chkHash << " fn= " << chkFilename);
        UINFO(9, " nowSt  s=" << curStat.st_اندازه << " mt=" << curStat.st_mtime
                              << " ct=" << curStat.st_ctime);

        // We'd like this rule:
        // if (!(curStat.st_size == chkSize
        //      && curStat.st_mtime == chkMstime) {
        // However NFS messes us up, as there might be some data outstanding when
        // we determined the original size.  For safety, we know the creation time
        // must be within a few second window... call it 20 sec.
        إذا (!(curStat.st_اندازه >= chk_اندازه && curStat.st_ino == chkIno
              && curStat.st_ctime == chkCstime && VL_STAT_CTIME_NSEC(curStat) == chkCnstime
              && curStat.st_mtime <= (chkMstime + 20)
              // Not comparing chkMnstime
              )) {
            UINFO(2, "   --check-times: out-of-date "
                         << chkFilename << "; " << curStat.st_اندازه << "=?" << chk_اندازه << " "
                         << curStat.st_ctime << "." << VL_STAT_CTIME_NSEC(curStat) << "=?"
                         << chkCstime << "." << chkCnstime << " " << curStat.st_mtime << "."
                         << VL_STAT_MTIME_NSEC(curStat) << "=?" << chkMstime << "." << chkMnstime);
            إذا (skipHashing) أرجع خطأ;
            // We didn't hash target output files, nor large files,
            // as unlikely to find a match and can be large
            إذا (chkHash == "unhashed") أرجع خطأ;

            VHashSha512 curHash;
            curHash.درج_File(chkFilename);
            إذا (curHash.digestSymbol24() != chkHash) {
                UINFO(2, "   --check-times: hash differs " << chkFilename << "; "
                                                           << curHash.digestSymbol24() << "=?"
                                                           << chkHash);
                أرجع خطأ;
            }
        }
    }
    أرجع درست;
}

//######################################################################
// V3File

فراغ V3File::جمع_کن_Src_Depend(ثابت string& filename) VL_MT_امن { dependImp.جمع_کن_Src_Depend(filename); }
فراغ V3File::جمع_کن_Tgt_Depend(ثابت string& filename) VL_MT_امن { dependImp.جمع_کن_Tgt_Depend(filename); }
فراغ V3File::writeDepend(ثابت string& filename) { dependImp.writeDepend(filename); }
std::وکتور<string> V3File::getAllDeps() { أرجع dependImp.getAllDeps(); }
فراغ V3File::writeTimes(ثابت string& filename, ثابت string& cmdline_درون) {
    dependImp.writeTimes(filename, cmdline_درون);
}
منطقي V3File::checkTimes(ثابت string& filename, ثابت string& cmdline_درون) {
    أرجع dependImp.checkTimes(filename, cmdline_درون);
}
فراغ V3File::createMakeDirFor(ثابت string& filename) {
    إذا (filename != VL_DEV_NULL
        // If doesn't start with makeDir then some output file user requested
        && filename.substr(0, v3Global.opt.makeDir().درازی() + 1)
               == v3Global.opt.makeDir() + "/") {
        createMakeDir();
    }
}
فراغ V3File::createMakeDir() {
    ساكن منطقي s_created = خطأ;
    إذا (!s_created) {
        s_created = درست;
        V3Os::createDir(v3Global.opt.makeDir());
        إذا (v3Global.opt.hier_بالاترین()) V3Os::createDir(v3Global.opt.hier_بالاترین_داده_Dir());
    }
}

//######################################################################
// VInFilterImp

صنف VInFilterImp final {
    استفاده StrList = VInFilter::StrList;

    std::map<ثابت std::string, std::string> m_contentsMap;  // Cache of file contents
    منطقي m_readEof = خطأ;  // Received EOF on read
#اگر_تعریف_شده INFILTER_PIPE
    pid_t m_pid = 0;  // fork() process id
#وإلا
    صحيح m_pid = 0;  // fork() process id - always zero as disabled
#پایان_اگر
    منطقي m_pidExited = خطأ;  //  waitpid() reaped the filter process
    صحيح m_pid_وضعیت = 0;  // Exit status of the filter process
    صحيح m_writeFd = 0;  // File descriptor TO filter
    صحيح m_readFd = 0;  // File descriptor FROM filter

خاص:
    // METHODS

    منطقي readContents(ثابت string& filename, StrList& outl) {
        إذا (m_pid) {
            أرجع readContentsFilter(filename, outl);
        } وإلا {
            أرجع readContentsFile(filename, outl);
        }
    }
    منطقي readContentsFile(ثابت string& filename, StrList& outl) {
        ثابت صحيح fd = open(filename.c_str(), O_RDONLY);
        إذا (fd < 0) أرجع خطأ;
        m_readEof = خطأ;
        read_بلوکها(fd, -1, outl);
        close(fd);
        أرجع درست;
    }
    // cppcheck-suppress constParameterReference
    منطقي readContentsFilter(ثابت string& filename, StrList& outl) {
        (فراغ)filename;  // Prevent unused variable warning
        (فراغ)outl;  // Prevent unused variable warning
#اگر_تعریف_شده INFILTER_PIPE
        writeFilter("read \"" + filename + "\"\n");
        ثابت string سطر = read_Filter_سطر();
        إذا (سطر.پیدا_کن("Content-Length") != string::npos) {
            صحيح len = 0;
            از_رشته_بخوان(سطر.c_str(), "Content-Length: %d\n", &len);
            read_بلوکها(m_readFd, len, outl);
            أرجع درست;
        } وإلا {
            إذا (سطر != "") v3error("--pipe-filter protocol error, unexpected: " << سطر);
            أرجع خطأ;
        }
#وإلا
        v3fatalSrc("--pipe-filter not implemented on this platform");
        أرجع خطأ;
#پایان_اگر
    }

    // cppcheck-suppress functionConst
    فراغ checkFilter(منطقي hang) {
#اگر_تعریف_شده INFILTER_PIPE
        إذا (!m_pidExited && waitpid(m_pid, &m_pid_وضعیت, hang ? 0 : WNOHANG)) {
            UINFO(1, "--pipe-filter: Exited, status "
                         << m_pid_وضعیت << " exit=" << WEXITSTATUS(m_pid_وضعیت) << " err"
                         << std::strerror(errno));
            m_readEof = درست;
            m_pidExited = درست;
        }
#پایان_اگر
    }

    فراغ read_بلوکها(صحيح fd, صحيح اندازه, StrList& outl) {
        حرف buf[INFILTER_IPC_BUFSIZ];
        ssize_t sizegot = 0;
        طالما (!m_readEof && (اندازه < 0 || اندازه > sizegot)) {
            ssize_t todo = INFILTER_IPC_BUFSIZ;
            إذا (اندازه > 0 && اندازه < todo) todo = اندازه;
            errno = 0;
            ثابت ssize_t got = اقرأ(fd, buf, todo);
            // UINFO(9, "RD GOT g " << got << " e " << errno << " " << strerror(errno));
            // usleep(50*1000);
            إذا (got > 0) {
                outl.emplace_پشت(string(buf, got));
                sizegot += got;
            } وإلا إذا (errno == EINTR || errno == EAGAIN
#اگر_تعریف_شده EWOULDBLOCK
                       || errno == EWOULDBLOCK
#پایان_اگر
            ) {
                checkFilter(خطأ);
                V3Os::u_sleep(1000);
                استمر;
            } وإلا {
                m_readEof = درست;
                اكسر;
            }
        }
    }
    // cppverilator-suppress unusedFunction unusedPrivateFunction
    string read_Filter_سطر() {
        // Slow, but we don't need it much
        UINFO(9, "readFilterLine");
        string سطر;
        طالما (!m_readEof) {
            StrList outl;
            read_بلوکها(m_readFd, 1, outl);
            ثابت string onechar = listString(outl);
            سطر += onechar;
            إذا (onechar == "\n") {
                إذا (سطر == "\n") {
                    سطر = "";
                    استمر;
                } وإلا {
                    اكسر;
                }
            }
        }
        UINFO(6, "filter-line-in: " << سطر);
        أرجع سطر;
    }
    // cppverilator-suppress unusedFunction unusedPrivateFunction
    فراغ writeFilter(ثابت string& بیرون) {
        إذا (debug() >= 6) {
            UINFO(6, "filter-out: " << بیرون);
            إذا (بیرون[بیرون.درازی() - 1] != '\n') خروجی << '\n';
        }
        إذا (!m_pid) {
            v3error("--pipe-filter: write to closed file\n");
            m_readEof = درست;
            stop();
        }
        بیعلامت جابه_جایی = 0;
        طالما (!m_readEof && بیرون.درازی() > جابه_جایی) {
            errno = 0;
            ثابت صحيح got = اكتب(m_writeFd, (بیرون.c_str()) + جابه_جایی, بیرون.درازی() - جابه_جایی);
            // UINFO(9, "WR GOT g " << got << " e " << errno << " " << strerror(errno));
            // usleep(50*1000);
            إذا (got > 0) {
                جابه_جایی += got;
            } وإلا إذا (errno == EINTR || errno == EAGAIN
#اگر_تعریف_شده EWOULDBLOCK
                       || errno == EWOULDBLOCK
#پایان_اگر
            ) {
                checkFilter(خطأ);
                V3Os::u_sleep(1000);
                استمر;
            } وإلا {
                اكسر;
            }
        }
    }

    // Start the filter
    فراغ شروع(ثابت string& command) {
        إذا (command == "") {
            m_pid = 0;  // Disabled
        } وإلا {
            شروع_Filter(command);
        }
    }
    فراغ شروع_Filter(ثابت string& command) {
        (فراغ)command;  // Prevent Unused variable warning
#اگر_تعریف_شده INFILTER_PIPE
        صحيح fd_stdin[2];  // Can't use std::array
        صحيح fd_stdout[2];  // Can't use std::array
        constexpr صحيح P_RD = 0;
        constexpr صحيح P_WR = 1;

        إذا (VL_UNCOVERABLE(أنبوب(fd_stdin) != 0 || أنبوب(fd_stdout) != 0))
            v3fatal("--pipe-filter: Can't pipe: " << std::strerror(errno));  // LCOV_EXCL_LINE
        إذا (fd_stdin[P_RD] <= 2 || fd_stdin[P_WR] <= 2 || fd_stdout[P_RD] <= 2
            || fd_stdout[P_WR] <= 2) {
            // We'd have to rearrange all of the FD usages in this case.
            // Too unlikely; verilator isn't a daemon.
            v3fatal("--pipe-filter: stdin/stdout closed before pipe opened\n");
        }

        UINFO(1, "--pipe-filter: /bin/sh -c " << command);

        ثابت pid_t pid = fork();
        إذا (VL_UNCOVERABLE(pid < 0))
            v3fatal("--pipe-filter: fork failed: " << std::strerror(errno));  // LCOV_EXCL_LINE
        إذا (pid == 0) {  //فرزند
            UINFO(6, "In child");
            close(fd_stdin[P_WR]);
            dup2(fd_stdin[P_RD], 0);
            close(fd_stdout[P_RD]);
            dup2(fd_stdout[P_WR], 1);
            // And stderr comes from parent

            execl("/bin/sh", "sh", "-c", command.c_str(), static_cast<حرف*>(اشاره_تهی));
            // Don't use v3fatal, we don't share the common structures any more
            fprintf(stderr, "--pipe-filter: exec failed: %s\n", std::strerror(errno));
            _exit(1);
        } وإلا {  //والد
            UINFO(6, "In parent, child pid " << pid << " stdin " << fd_stdin[P_WR] << "->"
                                             << fd_stdin[P_RD] << " stdout " << fd_stdout[P_WR]
                                             << "->" << fd_stdout[P_RD]);
            m_pid = pid;
            m_pidExited = خطأ;
            m_pid_وضعیت = 0;
            m_writeFd = fd_stdin[P_WR];
            m_readFd = fd_stdout[P_RD];
            m_readEof = خطأ;

            close(fd_stdin[P_RD]);
            close(fd_stdout[P_WR]);

            صحيح flags = fcntl(m_readFd, F_GETFL, 0);
            fcntl(m_readFd, F_SETFL, flags | O_NONBLOCK);

            flags = fcntl(m_writeFd, F_GETFL, 0);
            fcntl(m_writeFd, F_SETFL, flags | O_NONBLOCK);
        }
        UINFO(6, "startFilter complete");
#وإلا
        v3fatalSrc("--pipe-filter not implemented on this platform");
#پایان_اگر
    }

    فراغ stop() {
        إذا (m_pid) stopFilter();
    }
    فراغ stopFilter() {
        UINFO(6, "Stopping filter process");
#اگر_تعریف_شده INFILTER_PIPE
        close(m_writeFd);
        checkFilter(درست);
        إذا (!WIFEXITED(m_pid_وضعیت) || WEXITSTATUS(m_pid_وضعیت) != 0) {
            v3fatal("--pipe-filter returned bad status");
        }
        m_pid = 0;
        close(m_readFd);
        UINFO(6, "Closed");
#وإلا
        v3fatalSrc("--pipe-filter not implemented on this platform");
#پایان_اگر
    }

محمي:
    دوست صنف VInFilter;
    // Read file contents and return it
    منطقي readWholefile(ثابت string& filename, StrList& outl) {
        ثابت خودکار it = m_contentsMap.پیدا_کن(filename);
        إذا (it != m_contentsMap.انتها()) {
            outl.افزودن_پشت(it->دومی);
            أرجع درست;
        }
        إذا (!readContents(filename, outl)) أرجع خطأ;
        إذا (list_اندازه(outl) < INFILTER_CACHE_MAX) {
            // Cache small files (only to save space)
            // It's quite common to `include "timescale" thousands of times
            // This isn't so important if it's just an open(), but filtering can be slow
            m_contentsMap.emplace(filename, listString(outl));
        }
        أرجع درست;
    }
    ساكن اندازه_t list_اندازه(ثابت StrList& sl) {
        اندازه_t نتیجه = 0;
        لكل (ثابت string& i : sl) نتیجه += i.درازی();
        أرجع نتیجه;
    }
    ساكن string listString(ثابت StrList& sl) {
        string نتیجه;
        لكل (ثابت string& i : sl) نتیجه += i;
        أرجع نتیجه;
    }
    // CONSTRUCTORS
    صریح VInFilterImp(ثابت string& command) { شروع(command); }
    ~VInFilterImp() { stop(); }
};

//######################################################################
// VInFilter
// Just dispatch to the implementation

VInFilter::VInFilter(ثابت string& command) { m_impp = جديد VInFilterImp{command}; }
VInFilter::~VInFilter() {
    إذا (m_impp) VL_DO_CLEAR(حذف m_impp, m_impp = اشاره_تهی);
}

منطقي VInFilter::readWholefile(ثابت string& filename, VInFilter::StrList& outl) {
    UASSERT(m_impp, "readWholefile on invalid filter");
    أرجع m_impp->readWholefile(filename, outl);
}

//######################################################################
// V3OutFormatter: A class for printing code with automatic indentation.

V3_بیرون_Formatter::V3_بیرون_Formatter(V3_بیرون_Formatter::Language lang)
    : m_lang{lang} {
    m_بلوک_Indent = v3Global.opt.decoration() ? 4 : 1;
    m_comma_عرض = v3Global.opt.decoration() ? 50 : 150;
}

//----------------------------------------------------------------------

ساكن constexpr صحيح MAXSPACE = 80;  // After this indent, stop indenting more

// Table of spaces, so we don't have to alloc/free them all the time
ساكن ثابت std::array<std::string, MAXSPACE + 1> s_indentSpaces = []() {
    std::array<std::string, MAXSPACE + 1> جدول;
    لكل (صحيح i = 0; i <= MAXSPACE; ++i) جدول[i] = std::string(static_cast<اندازه_t>(i), ' ');
    أرجع جدول;
}();

ثابت std::string& V3_بیرون_Formatter::indentSpaces(صحيح رقم) {
    // Indent the specified number of spaces.
    أرجع s_indentSpaces[std::max(0, std::min(رقم, MAXSPACE))];
}

منطقي V3_بیرون_Formatter::token_تطابق(ثابت حرف* cp, ثابت حرف* cmp) {
    طالما (*cmp && *cmp == *cp) {
        ++cp;
        ++cmp;
    }
    إذا (*cmp) أرجع خطأ;
    إذا (*cp && !std::آیا_فاصله(*cp)) أرجع خطأ;
    أرجع درست;
}

منطقي V3_بیرون_Formatter::token_شروع(ثابت حرف* cp) {
    أرجع (token_تطابق(cp, "begin") || token_تطابق(cp, "case") || token_تطابق(cp, "casex")
            || token_تطابق(cp, "casez") || token_تطابق(cp, "class") || token_تطابق(cp, "function")
            || token_تطابق(cp, "interface") || token_تطابق(cp, "module") || token_تطابق(cp, "package")
            || token_تطابق(cp, "task"));
}

منطقي V3_بیرون_Formatter::token_انتها(ثابت حرف* cp) {
    أرجع (token_تطابق(cp, "end") || token_تطابق(cp, "endcase") || token_تطابق(cp, "endclass")
            || token_تطابق(cp, "endfunction") || token_تطابق(cp, "endinterface")
            || token_تطابق(cp, "endmodule") || token_تطابق(cp, "endpackage")
            || token_تطابق(cp, "endtask"));
}

منطقي V3_بیرون_Formatter::token_Not_شروع(ثابت حرف* cp) {
    أرجع (token_تطابق(cp, "export") || token_تطابق(cp, "import"));
}

صحيح V3_بیرون_Formatter::انتها_سطوح(ثابت حرف* strg) {
    صحيح سطوح = m_indentLevel;
    {
        ثابت حرف* cp = strg;
        طالما (std::آیا_فاصله(*cp)) ++cp;
        بدل (*cp) {
        حالة '\n':  // Newlines.. No need for whitespace before it
            أرجع 0;
        حالة '#':  // Preproc directive
            إذا (m_lang == LA_C) أرجع 0;
        }
        {
            // label/public/private:  Deindent by 2 spaces
            ثابت حرف* mp = cp;
            لكل (; std::آیا_حرف_یا_رقم(*mp); ++mp) {}
            إذا (mp[0] == ':' && mp[1] != ':') أرجع (سطوح - m_بلوک_Indent / 2);
        }
    }
    // We want "} else {" to be one level to the left of normal
    لكل (ثابت حرف* cp = strg; *cp; ++cp) {
        بدل (*cp) {
        حالة '}':
        حالة ')': سطوح -= m_بلوک_Indent; اكسر;
        حالة '<':
            إذا (m_lang == LA_XML) {
                إذا (cp[1] == '/') سطوح -= m_بلوک_Indent;
            }
            اكسر;
        حالة 'e':
            إذا (m_lang == LA_VERILOG && token_انتها(cp)) سطوح -= m_بلوک_Indent;
            اكسر;
        حالة '\t':
        حالة ' ': اكسر;  // Continue
        افتراضي: أرجع سطوح;  // Letter
        }
    }
    أرجع سطوح;
}

فراغ V3_بیرون_Formatter::putns(ثابت Ast_گره* nodep, ثابت حرف* strg) {
    إذا (!v3Global.opt.decoration()) {
        putsOutput(strg);
        أرجع;
    }

    ثابت منطقي put_گره_Decoration =  //
        nodep  //
        && v3Global.opt.decorationNodes()  //
        && !v3Global.opt.protectIds()  //
        && (m_منبع_آخرین_Filenameno != nodep->fileline()->filenameno()  //
            || m_منبع_آخرین_Lineno != nodep->fileline()->اولی_Lineno())  //
        && File_سطر::built_درون_Filename() != nodep->fileline()->filename();

    إذا (m_prependIndent && ((strg[0] && strg[0] != '\n') || put_گره_Decoration)) {
        putsNoTracking(indentSpaces(انتها_سطوح(strg)));
        m_prependIndent = خطأ;
    }

    إذا (put_گره_Decoration) {
        ثابت File_سطر* ثابت flp = nodep->fileline();
        m_منبع_آخرین_Lineno = flp->اولی_Lineno();
        m_منبع_آخرین_Filenameno = flp->filenameno();
        ثابت std::string lineno = std::مقصد_string(flp->lineno());
        putsNoTracking("/*" + flp->filename() + ":" + lineno + " " + cvt_مقصد_Hex(nodep) + "*/");
    }

    منطقي notstart = خطأ;
    منطقي wordstart = درست;
    منطقي equalsForBracket = خطأ;  // Looking for "= {"
    لكل (ثابت حرف* cp = strg; *cp; ++cp) {
        putcNoTracking(*cp);
        إذا (std::آیا_حرف(*cp)) {
            إذا (wordstart && m_lang == LA_VERILOG && token_Not_شروع(cp)) notstart = درست;
            إذا (wordstart && m_lang == LA_VERILOG && !notstart && token_شروع(cp)) indentInc();
            إذا (wordstart && m_lang == LA_VERILOG && token_انتها(cp)) indentDec();
        }
        ثابت منطقي was_درون_Backslash = m_درون_Backslash;
        m_درون_Backslash = خطأ;
        بدل (*cp) {
        حالة '\n':
            ++m_lineno;
            wordstart = درست;
            إذا (cp[1] == '\0') {
                // Add the indent later, may be a indentInc/indentDec
                // called between now and then
                m_prependIndent = درست;
            } وإلا {
                m_prependIndent = خطأ;
                putsNoTracking(indentSpaces(انتها_سطوح(cp + 1)));
            }
            اكسر;
        حالة ' ': wordstart = درست; اكسر;
        حالة '\t': wordstart = درست; اكسر;
        حالة '"':
            wordstart = خطأ;
            إذا (!was_درون_Backslash) m_درون_String_Literal = !m_درون_String_Literal;
            اكسر;
        حالة '\\':
            wordstart = خطأ;
            إذا (!was_درون_Backslash) m_درون_Backslash = درست;
            اكسر;
        حالة '/':
            إذا (m_lang == LA_C || m_lang == LA_VERILOG) {
                إذا (cp > strg && cp[-1] == '/' && !m_درون_String_Literal) {
                    // Output ignoring contents to EOL
                    ++cp;
                    طالما (*cp && cp[1] && cp[1] != '\n') putcNoTracking(*cp++);
                    إذا (*cp) putcNoTracking(*cp);
                }
            }
            اكسر;
        حالة '{':
            إذا (!m_درون_String_Literal) {
                إذا (m_lang == LA_C && (equalsForBracket || m_bracketLevel)) {
                    // Break up large code inside "= { ..."
                    m_parenVec.افزودن(m_indentLevel
                                    * m_بلوک_Indent);  // Line up continuation with block+1
                    ++m_bracketLevel;
                }
                indentInc();
            }
            اكسر;
        حالة '}':
            إذا (!m_درون_String_Literal) {
                إذا (m_bracketLevel > 0) {
                    m_parenVec.برداشت();
                    --m_bracketLevel;
                }
                indentDec();
            }
            اكسر;
        حالة '(':
            إذا (!m_درون_String_Literal) {
                indentInc();
                // Line up continuation with open paren, plus one indent
                m_parenVec.افزودن(m_ستونی);
            }
            اكسر;
        حالة ')':
            إذا (!m_درون_String_Literal) {
                إذا (!m_parenVec.خالی()) m_parenVec.برداشت();
                indentDec();
            }
            اكسر;
        حالة '<':
            إذا (m_lang == LA_XML) {
                إذا (cp[1] == '/') {
                    // Zero as the > will result in net decrease by one
                } وإلا إذا (cp[1] == '!' || cp[1] == '?') {
                    indentInc();  // net same indent
                } وإلا {
                    indentInc();  // net increase by one
                    indentInc();
                }
            }
            اكسر;
        حالة '>':
            إذا (m_lang == LA_XML) {
                indentDec();
                إذا (cp > strg && cp[-1] == '/') indentDec();  // < ..... /> stays same level
            }
            اكسر;
        افتراضي: wordstart = خطأ; اكسر;
        }

        بدل (*cp) {
        حالة '=': equalsForBracket = درست; اكسر;
        حالة ' ': اكسر;
        افتراضي: equalsForBracket = خطأ; اكسر;
        }
    }
}

فراغ V3_بیرون_Formatter::put_Break_عبارت() {
    إذا (!m_parenVec.خالی()) putBreak();
}

// Add a line break if too wide
فراغ V3_بیرون_Formatter::putBreak() {
    إذا (!v3Global.opt.decoration()) أرجع;
    إذا (!m_nobreak) {
        // char s[1000]; sprintf(s, "{%d,%d}", m_column, m_parenVec.top()); putsNoTracking(s);
        إذا (exceeded_عرض()) {
            putcNoTracking('\n');
            إذا (!m_parenVec.خالی()) putsNoTracking(indentSpaces(m_parenVec.بالاترین()));
        }
    }
}

فراغ V3_بیرون_Formatter::putsQuoted(ثابت string& strg) {
    // Quote \ and " for use inside C programs
    // Don't use to quote a filename for #include - #include doesn't \ escape.
    ثابت string quoted = quoteNameControls(strg);
    putcNoTracking('"');
    لكل (ثابت حرف c : quoted) putcNoTracking(c);
    putcNoTracking('"');
    إذا (strg.پیدا_کن('\0') != std::string::npos) putcNoTracking('s');  // C++14 std::string
}
فراغ V3_بیرون_Formatter::putsNoTracking(ثابت string& strg) {
    إذا (!v3Global.opt.decoration()) {
        putsOutput(strg.c_str());
        أرجع;
    }
    // Don't track {}'s, probably because it's a $display format string
    لكل (ثابت حرف c : strg) putcNoTracking(c);
}

فراغ V3_بیرون_Formatter::putcNoTracking(حرف chr) {
    إذا (!v3Global.opt.decoration()) {
        putcOutput(chr);
        أرجع;
    }
    بدل (chr) {
    حالة '\n':
        ++m_lineno;
        m_ستونی = 0;
        m_nobreak = درست;
        اكسر;
    حالة '\t': m_ستونی = ((m_ستونی + 9) / 8) * 8; اكسر;
    حالة ' ':
    حالة '(':
    حالة '|':
    حالة '&': ++m_ستونی; اكسر;
    افتراضي:
        ++m_ستونی;
        m_nobreak = خطأ;
        اكسر;
    }
    putcOutput(chr);
}

string V3_بیرون_Formatter::quoteNameControls(ثابت string& namein,
                                         V3_بیرون_Formatter::Language lang) VL_خالص {
    // Encode control chars into output-appropriate escapes
    // Reverse is VString::unquoteSVString
    string بیرون;
    إذا (lang == LA_XML) {
        // Encode chars into XML string
        لكل (ثابت حرف c : namein) {
            إذا (c == '"') {
                بیرون += "&quot;"s;
            } وإلا إذا (c == '\'') {
                بیرون += "&apos;"s;
            } وإلا إذا (c == '<') {
                بیرون += "&lt;"s;
            } وإلا إذا (c == '>') {
                بیرون += "&gt;"s;
            } وإلا إذا (c == '&') {
                بیرون += "&amp;"s;
            } وإلا إذا (std::isprint(c)) {
                بیرون += c;
            } وإلا {
                بیرون += "&#"s + cvt_مقصد_Str(static_cast<بیعلامت صحيح>(c & 0xff)) + ";";
            }
        }
    } وإلا {
        // Encode control chars into C style escapes
        لكل (ثابت حرف c : namein) {
            إذا (c == '\\' || c == '"') {
                بیرون += "\\"s + c;
            } وإلا إذا (c == '\n') {
                بیرون += "\\n";
            } وإلا إذا (c == '\r') {
                بیرون += "\\r";
            } وإلا إذا (c == '\t') {
                بیرون += "\\t";
            } وإلا إذا (std::isprint(c)) {
                بیرون += c;
            } وإلا {
                // This will also cover \a etc
                ثابت string octal
                    = "\\"s + cvt_مقصد_Str((c >> 6) & 3) + cvt_مقصد_Str((c >> 3) & 7) + cvt_مقصد_Str(c & 7);
                بیرون += octal;
            }
        }
    }
    أرجع بیرون;
}

//----------------------------------------------------------------------
// Simple wrappers

فراغ V3_بیرون_Formatter::چاپ_کن(ثابت حرف* fmt...) {
    constexpr اندازه_t bufsize = 5000;
    حرف sbuff[bufsize];
    va_list ap;
    va_شروع(ap, fmt);
    VL_VSNPRINTF(sbuff, bufsize, fmt, ap);
    va_انتها(ap);
    هذا->puts(sbuff);
}

//######################################################################
// V3OutFormatter: A class for printing to a file, with automatic indentation of C++ code.

V3_بیرون_File::V3_بیرون_File(ثابت string& filename, V3_بیرون_Formatter::Language lang)
    : V3_بیرون_Formatter{lang}
    , m_filename{filename}
    , m_bufferp{جديد std::array<حرف, WRITE_میانگیر_اندازه_بایتها>{}} {
    إذا ((m_fp = V3File::new_fopen_w(filename)) == اشاره_تهی) {
        v3fatal("Can't write file: " << filename);
    }
}

V3_بیرون_File::~V3_بیرون_File() {
    write_بلوک();

    إذا (m_fp) fclose(m_fp);
    m_fp = اشاره_تهی;
}

فراغ V3_بیرون_File::putsForceIncs() {
    ثابت VStringList& forceIncs = v3Global.opt.forceIncs();
    لكل (ثابت string& i : forceIncs) puts("#include \"" + i + "\"\n");
}

فراغ V3_بیرون_CFile::putsGuard() {
    UASSERT(!m_guard, "Already called putsGuard in emit file");
    m_guard = درست;
    string var = VString::upcase("VERILATED_"s + V3Os::filenameNonDir(filename()) + "_");
    لكل (حرف& c : var) {
        إذا (!std::آیا_حرف_یا_رقم(c)) c = '_';
    }
    puts("\n#ifndef " + var + "\n");
    puts("#define " + var + "  // guard\n");
}

//######################################################################
// V3OutStream

V3_بیرون_جریان::V3_بیرون_جریان(std::ostream& ostream, V3_بیرون_Formatter::Language lang)
    : V3_بیرون_Formatter{lang}
    , m_ostream{ostream} {}

//######################################################################
// VIdProtect

صنف VIdProtectImp final {
    // MEMBERS
    V3Mutex m_mutex;  // Protects members
    std::map<ثابت std::string, std::string> m_nameMap;  // Map of old name into new name
    std::unordered_مجموعه<std::string> m_new_Id_مجموعه VL_GUARDED_BY(m_mutex);  // Which new names exist
محمي:
    // CONSTRUCTORS
    دوست صنف VIdProtect;
    ساكن VIdProtectImp& singleton() VL_MT_امن {
        ساكن VIdProtectImp s;
        أرجع s;
    }

عام:
    VIdProtectImp() {
        passthru("this");
        passthru("TOP");
        passthru("vlInst");
        passthru("vlSelf");
        passthru("vlSymsp");
    }
    ~VIdProtectImp() = افتراضي;
    // METHODS
    string passthru(ثابت string& old) VL_MT_امن_EXCLUDES(m_mutex) {
        إذا (!v3Global.opt.protectIds()) أرجع old;
        ثابت V3_قفل_Guard قفل_2{m_mutex};
        ثابت خودکار it = m_nameMap.پیدا_کن(old);
        إذا (it != m_nameMap.انتها()) {
            // No way to go back and correct the older crypt name
            UASSERT(old == it->دومی,
                    "Pass-thru request for '" + old + "' after already --protect-ids of it.");
        } وإلا {
            m_nameMap.emplace(old, old);
            m_new_Id_مجموعه.درج(old);
        }
        أرجع old;
    }
    string protectIf(ثابت string& old, منطقي doIt) VL_MT_امن_EXCLUDES(m_mutex) {
        إذا (!v3Global.opt.protectIds() || old.خالی() || !doIt) أرجع old;
        ثابت V3_قفل_Guard قفل_2{m_mutex};
        ثابت خودکار pair = m_nameMap.emplace(old, "");
        إذا (pair.دومی) {
            string بیرون;
            إذا (v3Global.opt.debugProtect()) {
                // This lets us see the symbol being protected to debug cases
                // where e.g. the definition is protect() but reference is
                // missing a protect()
                بیرون = "PS" + old;
            } وإلا {
                VHashSha512 digest{v3Global.opt.protect_کلید_Defaulted()};
                digest.درج(old);
                // Add "PS" prefix (Protect Symbols) as cannot start symbol with number
                بیرون = "PS" + digest.digestSymbol24();
                // See if we can shrink the digest symbol to something smaller
                لكل (اندازه_t len = 6; len < بیرون.اندازه() - 3; len += 3) {
                    ثابت string tryout = بیرون.substr(0, len);
                    إذا (m_new_Id_مجموعه.درج(tryout).دومی) {
                        بیرون = tryout;
                        اكسر;
                    }
                }
            }
            pair.اولی->دومی = بیرون;
        }
        أرجع pair.اولی->دومی;
    }
    string protect_واژگان_If(ثابت string& old, منطقي doIt) VL_MT_امن {
        // Split at " " (for traces), "." (for scopes), "->", "(", "&", ")" (for self pointers)
        إذا (!(doIt && v3Global.opt.protectIds())) أرجع old;
        string بیرون;
        string::اندازه_type شروع = 0;
        // space, ., ->
        طالما (درست) {
            // When C++11, use find_if and lambda
            string::اندازه_type pos = string::npos;
            string separator;
            trySep(old, شروع, " ", pos /*ref*/, separator /*ref*/);
            trySep(old, شروع, ".", pos /*ref*/, separator /*ref*/);
            trySep(old, شروع, "->", pos /*ref*/, separator /*ref*/);
            trySep(old, شروع, "(", pos /*ref*/, separator /*ref*/);
            trySep(old, شروع, "&", pos /*ref*/, separator /*ref*/);
            trySep(old, شروع, ")", pos /*ref*/, separator /*ref*/);
            إذا (pos == string::npos) اكسر;
            بیرون += protectIf(old.substr(شروع, pos - شروع), درست) + separator;
            شروع = pos + separator.درازی();
        }
        بیرون += protectIf(old.substr(شروع), درست);
        أرجع بیرون;
    }
    فراغ writeMapFile(ثابت string& filename) ثابت {
        V3_بیرون_Xml_File از{filename};
        از.puts_سرآیند();
        از.puts("<!-- DESCR"
                "IPTION: Verilator output: XML representation of netlist -->\n");
        از.puts("<verilator_id_map>\n");
        {
            لكل (ثابت خودکار& itr : m_nameMap) {
                از.puts("<map from=\"" + itr.دومی + "\" to=\"" + itr.اولی + "\"/>\n");
            }
        }
        از.puts("</verilator_id_map>\n");
    }

خاص:
    فراغ trySep(ثابت string& old, string::اندازه_type شروع, ثابت string& trySep,
                string::اندازه_type& posr, string& separatorr) VL_خالص {
        ثابت string::اندازه_type trypos = old.پیدا_کن(trySep, شروع);
        إذا (trypos != string::npos) {
            إذا (posr == string::npos || (posr > trypos)) {
                posr = trypos;
                separatorr = trySep;
            }
        }
    }
};

string VIdProtect::ifNoProtect(ثابت string& درون) VL_MT_امن {
    أرجع v3Global.opt.protectIds() ? "" : درون;
}
string VIdProtect::protectIf(ثابت string& old, منطقي doIt) VL_MT_امن {
    أرجع VIdProtectImp::singleton().protectIf(old, doIt);
}
string VIdProtect::protect_واژگان_If(ثابت string& old, منطقي doIt) VL_MT_امن {
    أرجع VIdProtectImp::singleton().protect_واژگان_If(old, doIt);
}
فراغ VIdProtect::writeMapFile(ثابت string& filename) {
    VIdProtectImp::singleton().writeMapFile(filename);
}
