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

#شامل "V3PchAstMT.h"

#شامل "V3File.h"

#شامل "V3Os.h"
#شامل "V3String.h"

#شامل <cerrno>
#شامل <cstdarg>
#شامل <fcntl.h>
#شامل <iomanip>
#شامل <map>
#شامل <memory>

#شامل <sys/stat.h>
#شامل <sys/types.h>

// clang-format off
#اگر defined(__unix__) || defined(__unix) || (defined(__APPLE__) && defined(__MACH__)) || defined(VL_CPPCHECK)
# define INFILTER_PIPE  // Allow pipe filtering.  Needs fork()
#اگر_ختم

#اگر_تعریف_شدہ HAVE_STAT_NSEC  // i.e. Linux 2.6, from configure
# define VL_STAT_CTIME_NSEC(stat) ((stat).st_ctim.tv_nsec)  // Nanoseconds
# define VL_STAT_MTIME_NSEC(stat) ((stat).st_mtim.tv_nsec)  // Nanoseconds
#ورنہ
# define VL_STAT_CTIME_NSEC(stat) (0)
# define VL_STAT_MTIME_NSEC(stat) (0)
#اگر_ختم

#اگر_تعریف_شدہ INFILTER_PIPE
# include <sys/wait.h>
#اگر_ختم

#اگر defined(_WIN32) || defined(__MINGW32__)
# include <io.h>  // open, read, write, close
#اگر_ختم
// clang-format on

VL_DEFINE_DEBUG_FUNCTIONS;

// If change this code, run a test with the below size set very small
// #define INFILTER_IPC_BUFSIZ 16
constexpr صحیح INFILTER_IPC_BUFSIZ = (64 * 1024);  // For debug, try this as a small number
constexpr صحیح INFILTER_CACHE_MAX = (64 * 1024);  // Maximum bytes to cache if same file read twice

constexpr off_t FILE_HASH_سائز_MAX = 1 * 1024 * 1024;  // Maxium size file to hash

//######################################################################
// V3File Internal state

جماعت V3FileDependImp final {
    // TYPES
    جماعت DependFile final {
        // A single file
        ثابت منطقی m_ہدف;  // True if write, else read
        منطقی m_exists = سچ;  // File exists
        ثابت string m_filename;  // Filename
        ساخت stat m_stat;  // Stat information
        VHashSha512 m_hash;  // SHA hash of file contents
    عوامی:
        DependFile(ثابت string& filename, منطقی ہدف)
            : m_ہدف{ہدف}
            , m_filename{filename} {
            m_stat.st_ctime = 0;
            m_stat.st_mtime = 0;
        }
        ~DependFile() = طےشدہ;
        ثابت string& filename() ثابت { واپس m_filename; }
        منطقی ہدف() ثابت { واپس m_ہدف; }
        منطقی exists() ثابت { واپس m_exists; }
        off_t سائز() ثابت { واپس m_stat.st_سائز; }
        ino_t ino() ثابت { واپس m_stat.st_ino; }
        وقت_t cstime() ثابت { واپس m_stat.st_ctime; }  // Seconds
        وقت_t cnstime() ثابت { واپس VL_STAT_CTIME_NSEC(m_stat); }  // Nanoseconds
        وقت_t mstime() ثابت { واپس m_stat.st_mtime; }  // Seconds
        وقت_t mnstime() ثابت { واپس VL_STAT_MTIME_NSEC(m_stat); }  // Nanoseconds
        string hash_Digest_علامت() {
            ساکن VHashSha512 s_خالی_خانے_Hash;
            واپس m_hash.digestSymbol24() != s_خالی_خانے_Hash.digestSymbol24()
                       ? m_hash.digestSymbol24()
                       : "unhashed";
        }
        خالی بھرو_Stats() {
            اگر (!m_stat.st_mtime) {
                ثابت string fn = filename();
                ثابت صحیح err = stat(fn.c_str(), &m_stat);
                اگر (err != 0) {
                    std::memset(&m_stat, 0, حجم(m_stat));
                    m_stat.st_mtime = 1;
                    m_exists = جھوٹ;
                    // Not an error... This can occur due to `line directives in the .vpp files
                    UINFO(1, "-Info: File not statable: " << filename());
                } ورنہ {
                    // For speed, don't hash large files (e.g. verilator_bin)
                    اگر (!ہدف() && سائز() <= FILE_HASH_سائز_MAX) m_hash.داخل_کرو_File(filename());
                }
            }
        }
        منطقی عامل<(ثابت DependFile& rhs) ثابت { واپس filename() < rhs.filename(); }
    };

    // MEMBERS
    V3Mutex m_mutex;  // Protects members
    std::سیٹ<string> m_filename_سیٹ VL_GUARDED_BY(m_mutex);  // Files generated (elim duplicates)
    std::سیٹ<DependFile> m_filenameList;  // Files sourced/generated

    ساکن string stripQuotes(ثابت string& اندر) {
        string pretty = اندر;
        string::سائز_type pos;
        جب ((pos = pretty.ڈھونڈو('\"')) != string::npos) pretty.replace(pos, 1, "_");
        جب ((pos = pretty.ڈھونڈو('\n')) != string::npos) pretty.replace(pos, 1, "_");
        واپس pretty;
    }

عوامی:
    // ACCESSOR METHODS
    خالی جوڑو_Src_Depend(ثابت string& filename) VL_MT_محفوظ_EXCLUDES(m_mutex) {
        ثابت V3_تالا_Guard تالا_2{m_mutex};
        ثابت خودکار it_ملا_Pair = m_filename_سیٹ.داخل_کرو(filename);
        اگر (it_ملا_Pair.دوسرا) {
            DependFile df{filename, جھوٹ};
            df.بھرو_Stats();  // Get size now, in case changes during the run
            m_filenameList.داخل_کرو(df);
        }
    }
    خالی جوڑو_Tgt_Depend(ثابت string& filename) VL_MT_محفوظ_EXCLUDES(m_mutex) {
        ثابت V3_تالا_Guard تالا_2{m_mutex};
        ثابت خودکار it_ملا_Pair = m_filename_سیٹ.داخل_کرو(filename);
        اگر (it_ملا_Pair.دوسرا) m_filenameList.داخل_کرو(DependFile{filename, سچ});
    }
    خالی writeDepend(ثابت string& filename);
    std::سمتیہ<string> getAllDeps() ثابت;
    خالی writeTimes(ثابت string& filename, ثابت string& cmdline_اندر);
    منطقی checkTimes(ثابت string& filename, ثابت string& cmdline_اندر);
};

V3FileDependImp dependImp;  // Depend implementation class

//######################################################################
// V3FileDependImp

خالی V3FileDependImp::writeDepend(ثابت string& filename) {
    ثابت std::unique_ptr<std::ofstream> ofp{V3File::new_ofstream(filename)};
    اگر (ofp->fail()) v3fatal("Can't write file: " << filename);

    برائے (ثابت DependFile& i : m_filenameList) {
        اگر (i.ہدف()) *ofp << i.filename() << " ";
    }
    *ofp << " : ";
    *ofp << v3Global.opt.buildDepBin();
    *ofp << " ";

    برائے (ثابت DependFile& i : m_filenameList) {
        اگر (!i.ہدف()) *ofp << i.filename() << " ";
    }

    *ofp << '\n';

    اگر (v3Global.opt.makePhony()) {
        *ofp << '\n';
        برائے (ثابت DependFile& i : m_filenameList) {
            اگر (!i.ہدف()) *ofp << i.filename() << ":\n";
        }
    }
}

std::سمتیہ<string> V3FileDependImp::getAllDeps() ثابت {
    std::سمتیہ<string> r;
    برائے (ثابت خودکار& itr : m_filenameList) {
        اگر (!itr.ہدف() && itr.exists()) r.ڈالو_عقب(itr.filename());
    }
    واپس r;
}

خالی V3FileDependImp::writeTimes(ثابت string& filename, ثابت string& cmdline_اندر) {
    ثابت std::unique_ptr<std::ofstream> ofp{V3File::new_ofstream(filename)};
    اگر (ofp->fail()) v3fatal("Can't write file: " << filename);

    ثابت string cmdline = stripQuotes(cmdline_اندر);
    *ofp << "# DESCR"
         << "IPTION: Verilator output: Timestamp data for --skip-identical.  Delete at will.\n";
    *ofp << "C \"" << cmdline << "\"\n";

    برائے (خودکار& it : m_filenameList) {
        DependFile* ثابت depfilep = const_cast<DependFile*>(&it);
        // Read stats of files we create after we're done making them
        // (except for this file, of course)
        V3Os::filesystemFlush(depfilep->filename());
        depfilep->بھرو_Stats();
        off_t show_سائز = depfilep->سائز();
        ino_t showIno = depfilep->ino();
        اگر (depfilep->filename() == filename) {
            show_سائز = 0;
            showIno = 0;  // We're writing it, so need to ignore it
        }

        *ofp << (depfilep->ہدف() ? "T" : "S") << " ";
        *ofp << " " << std::setw(8) << show_سائز;
        *ofp << " " << std::setw(8) << showIno;
        *ofp << " " << std::setw(11) << depfilep->cstime();
        *ofp << " " << std::setw(11) << depfilep->cnstime();
        *ofp << " " << std::setw(11) << depfilep->mstime();
        *ofp << " " << std::setw(11) << depfilep->mnstime();
        *ofp << " \"" << depfilep->hash_Digest_علامت() << "\"";
        *ofp << " \"" << depfilep->filename() << "\"";
        *ofp << '\n';
    }
}

منطقی V3FileDependImp::checkTimes(ثابت string& filename, ثابت string& cmdline_اندر) {
    ثابت std::unique_ptr<std::ifstream> ifp{V3File::new_ifstream_nodepend(filename)};
    اگر (ifp->fail()) {
        UINFO(2, "   --check-times failed: no input " << filename);
        واپس جھوٹ;
    }
    {
        ثابت string ignore = V3Os::getline(*ifp);
        اگر (ignore.خالی_خانے()) { /*مستعمل*/
        }
    }
    {
        حرف chkDir;
        *ifp >> chkDir;
        حرف quote;
        *ifp >> quote;
        ثابت string chkCmdline = V3Os::getline(*ifp, '"');
        ثابت string cmdline = stripQuotes(cmdline_اندر);
        اگر (cmdline != chkCmdline) {
            UINFO(2, "   --check-times failed: different command line");
            واپس جھوٹ;
        }
    }

    ثابت منطقی skipHashing = !V3Os::getenvStr("VERILATOR_DEBUG_SKIP_HASH", "").خالی_خانے();

    جب (!ifp->eof()) {
        حرف chkDir;
        *ifp >> chkDir;
        off_t chk_سائز;
        *ifp >> chk_سائز;
        ino_t chkIno;
        *ifp >> chkIno;
        اگر (ifp->eof()) توڑو;  // Needed to read final whitespace before found eof
        وقت_t chkCstime;
        *ifp >> chkCstime;
        وقت_t chkCnstime;
        *ifp >> chkCnstime;
        وقت_t chkMstime;
        *ifp >> chkMstime;
        وقت_t chkMnstime;
        *ifp >> chkMnstime;
        حرف quote;
        *ifp >> quote;
        ثابت string chkHash = V3Os::getline(*ifp, '"');
        *ifp >> quote;
        ثابت string chkFilename = V3Os::getline(*ifp, '"');

        V3Os::filesystemFlush(chkFilename);
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-member-init)
        ساخت stat curStat;
        ثابت صحیح err = stat(chkFilename.c_str(), &curStat);
        اگر (err != 0) {
            UINFO(2, "   --check-times failed: missing " << chkFilename);
            واپس جھوٹ;
        }

        // Skip the .dat file itself, as we were writing it at the time was being made
        اگر (filename == chkFilename) جاری;

        UINFO(9, " got d=" << chkDir << " s=" << chk_سائز << " ct=" << chkCstime << "."
                           << chkCnstime << " mt=" << chkMstime << "." << chkMnstime
                           << " h=" << chkHash << " fn= " << chkFilename);
        UINFO(9, " nowSt  s=" << curStat.st_سائز << " mt=" << curStat.st_mtime
                              << " ct=" << curStat.st_ctime);

        // We'd like this rule:
        // if (!(curStat.st_size == chkSize
        //      && curStat.st_mtime == chkMstime) {
        // However NFS messes us up, as there might be some data outstanding when
        // we determined the original size.  For safety, we know the creation time
        // must be within a few second window... call it 20 sec.
        اگر (!(curStat.st_سائز >= chk_سائز && curStat.st_ino == chkIno
              && curStat.st_ctime == chkCstime && VL_STAT_CTIME_NSEC(curStat) == chkCnstime
              && curStat.st_mtime <= (chkMstime + 20)
              // Not comparing chkMnstime
              )) {
            UINFO(2, "   --check-times: out-of-date "
                         << chkFilename << "; " << curStat.st_سائز << "=?" << chk_سائز << " "
                         << curStat.st_ctime << "." << VL_STAT_CTIME_NSEC(curStat) << "=?"
                         << chkCstime << "." << chkCnstime << " " << curStat.st_mtime << "."
                         << VL_STAT_MTIME_NSEC(curStat) << "=?" << chkMstime << "." << chkMnstime);
            اگر (skipHashing) واپس جھوٹ;
            // We didn't hash target output files, nor large files,
            // as unlikely to find a match and can be large
            اگر (chkHash == "unhashed") واپس جھوٹ;

            VHashSha512 curHash;
            curHash.داخل_کرو_File(chkFilename);
            اگر (curHash.digestSymbol24() != chkHash) {
                UINFO(2, "   --check-times: hash differs " << chkFilename << "; "
                                                           << curHash.digestSymbol24() << "=?"
                                                           << chkHash);
                واپس جھوٹ;
            }
        }
    }
    واپس سچ;
}

//######################################################################
// V3File

خالی V3File::جوڑو_Src_Depend(ثابت string& filename) VL_MT_محفوظ { dependImp.جوڑو_Src_Depend(filename); }
خالی V3File::جوڑو_Tgt_Depend(ثابت string& filename) VL_MT_محفوظ { dependImp.جوڑو_Tgt_Depend(filename); }
خالی V3File::writeDepend(ثابت string& filename) { dependImp.writeDepend(filename); }
std::سمتیہ<string> V3File::getAllDeps() { واپس dependImp.getAllDeps(); }
خالی V3File::writeTimes(ثابت string& filename, ثابت string& cmdline_اندر) {
    dependImp.writeTimes(filename, cmdline_اندر);
}
منطقی V3File::checkTimes(ثابت string& filename, ثابت string& cmdline_اندر) {
    واپس dependImp.checkTimes(filename, cmdline_اندر);
}
خالی V3File::createMakeDirFor(ثابت string& filename) {
    اگر (filename != VL_DEV_NULL
        // If doesn't start with makeDir then some output file user requested
        && filename.substr(0, v3Global.opt.makeDir().لمبائی_2() + 1)
               == v3Global.opt.makeDir() + "/") {
        createMakeDir();
    }
}
خالی V3File::createMakeDir() {
    ساکن منطقی s_created = جھوٹ;
    اگر (!s_created) {
        s_created = سچ;
        V3Os::createDir(v3Global.opt.makeDir());
        اگر (v3Global.opt.hier_چوٹی()) V3Os::createDir(v3Global.opt.hier_چوٹی_ڈیٹا_Dir());
    }
}

//######################################################################
// VInFilterImp

جماعت VInFilterImp final {
    استعمال StrList = VInFilter::StrList;

    std::map<ثابت std::string, std::string> m_contentsMap;  // Cache of file contents
    منطقی m_readEof = جھوٹ;  // Received EOF on read
#اگر_تعریف_شدہ INFILTER_PIPE
    pid_t m_pid = 0;  // fork() process id
#ورنہ
    صحیح m_pid = 0;  // fork() process id - always zero as disabled
#اگر_ختم
    منطقی m_pidExited = جھوٹ;  //  waitpid() reaped the filter process
    صحیح m_pid_حالت = 0;  // Exit status of the filter process
    صحیح m_writeFd = 0;  // File descriptor TO filter
    صحیح m_readFd = 0;  // File descriptor FROM filter

نجی:
    // METHODS

    منطقی readContents(ثابت string& filename, StrList& outl) {
        اگر (m_pid) {
            واپس readContentsFilter(filename, outl);
        } ورنہ {
            واپس readContentsFile(filename, outl);
        }
    }
    منطقی readContentsFile(ثابت string& filename, StrList& outl) {
        ثابت صحیح fd = open(filename.c_str(), O_RDONLY);
        اگر (fd < 0) واپس جھوٹ;
        m_readEof = جھوٹ;
        read_بلاکیں(fd, -1, outl);
        close(fd);
        واپس سچ;
    }
    // cppcheck-suppress constParameterReference
    منطقی readContentsFilter(ثابت string& filename, StrList& outl) {
        (خالی)filename;  // Prevent unused variable warning
        (خالی)outl;  // Prevent unused variable warning
#اگر_تعریف_شدہ INFILTER_PIPE
        writeFilter("read \"" + filename + "\"\n");
        ثابت string سطر = read_Filter_سطر();
        اگر (سطر.ڈھونڈو("Content-Length") != string::npos) {
            صحیح len = 0;
            لڑی_سے_پڑھو(سطر.c_str(), "Content-Length: %d\n", &len);
            read_بلاکیں(m_readFd, len, outl);
            واپس سچ;
        } ورنہ {
            اگر (سطر != "") v3error("--pipe-filter protocol error, unexpected: " << سطر);
            واپس جھوٹ;
        }
#ورنہ
        v3fatalSrc("--pipe-filter not implemented on this platform");
        واپس جھوٹ;
#اگر_ختم
    }

    // cppcheck-suppress functionConst
    خالی checkFilter(منطقی hang) {
#اگر_تعریف_شدہ INFILTER_PIPE
        اگر (!m_pidExited && waitpid(m_pid, &m_pid_حالت, hang ? 0 : WNOHANG)) {
            UINFO(1, "--pipe-filter: Exited, status "
                         << m_pid_حالت << " exit=" << WEXITSTATUS(m_pid_حالت) << " err"
                         << std::strerror(errno));
            m_readEof = سچ;
            m_pidExited = سچ;
        }
#اگر_ختم
    }

    خالی read_بلاکیں(صحیح fd, صحیح سائز, StrList& outl) {
        حرف buf[INFILTER_IPC_BUFSIZ];
        ssize_t sizegot = 0;
        جب (!m_readEof && (سائز < 0 || سائز > sizegot)) {
            ssize_t todo = INFILTER_IPC_BUFSIZ;
            اگر (سائز > 0 && سائز < todo) todo = سائز;
            errno = 0;
            ثابت ssize_t got = اقرأ(fd, buf, todo);
            // UINFO(9, "RD GOT g " << got << " e " << errno << " " << strerror(errno));
            // usleep(50*1000);
            اگر (got > 0) {
                outl.emplace_عقب(string(buf, got));
                sizegot += got;
            } ورنہ اگر (errno == EINTR || errno == EAGAIN
#اگر_تعریف_شدہ EWOULDBLOCK
                       || errno == EWOULDBLOCK
#اگر_ختم
            ) {
                checkFilter(جھوٹ);
                V3Os::u_sleep(1000);
                جاری;
            } ورنہ {
                m_readEof = سچ;
                توڑو;
            }
        }
    }
    // cppverilator-suppress unusedFunction unusedPrivateFunction
    string read_Filter_سطر() {
        // Slow, but we don't need it much
        UINFO(9, "readFilterLine");
        string سطر;
        جب (!m_readEof) {
            StrList outl;
            read_بلاکیں(m_readFd, 1, outl);
            ثابت string onechar = listString(outl);
            سطر += onechar;
            اگر (onechar == "\n") {
                اگر (سطر == "\n") {
                    سطر = "";
                    جاری;
                } ورنہ {
                    توڑو;
                }
            }
        }
        UINFO(6, "filter-line-in: " << سطر);
        واپس سطر;
    }
    // cppverilator-suppress unusedFunction unusedPrivateFunction
    خالی writeFilter(ثابت string& برآمد) {
        اگر (debug() >= 6) {
            UINFO(6, "filter-out: " << برآمد);
            اگر (برآمد[برآمد.لمبائی_2() - 1] != '\n') خارج << '\n';
        }
        اگر (!m_pid) {
            v3error("--pipe-filter: write to closed file\n");
            m_readEof = سچ;
            stop();
        }
        غیرعلامتی انحراف = 0;
        جب (!m_readEof && برآمد.لمبائی_2() > انحراف) {
            errno = 0;
            ثابت صحیح got = اكتب(m_writeFd, (برآمد.c_str()) + انحراف, برآمد.لمبائی_2() - انحراف);
            // UINFO(9, "WR GOT g " << got << " e " << errno << " " << strerror(errno));
            // usleep(50*1000);
            اگر (got > 0) {
                انحراف += got;
            } ورنہ اگر (errno == EINTR || errno == EAGAIN
#اگر_تعریف_شدہ EWOULDBLOCK
                       || errno == EWOULDBLOCK
#اگر_ختم
            ) {
                checkFilter(جھوٹ);
                V3Os::u_sleep(1000);
                جاری;
            } ورنہ {
                توڑو;
            }
        }
    }

    // Start the filter
    خالی آغاز(ثابت string& command) {
        اگر (command == "") {
            m_pid = 0;  // Disabled
        } ورنہ {
            آغاز_Filter(command);
        }
    }
    خالی آغاز_Filter(ثابت string& command) {
        (خالی)command;  // Prevent Unused variable warning
#اگر_تعریف_شدہ INFILTER_PIPE
        صحیح fd_stdin[2];  // Can't use std::array
        صحیح fd_stdout[2];  // Can't use std::array
        constexpr صحیح P_RD = 0;
        constexpr صحیح P_WR = 1;

        اگر (VL_UNCOVERABLE(أنبوب(fd_stdin) != 0 || أنبوب(fd_stdout) != 0))
            v3fatal("--pipe-filter: Can't pipe: " << std::strerror(errno));  // LCOV_EXCL_LINE
        اگر (fd_stdin[P_RD] <= 2 || fd_stdin[P_WR] <= 2 || fd_stdout[P_RD] <= 2
            || fd_stdout[P_WR] <= 2) {
            // We'd have to rearrange all of the FD usages in this case.
            // Too unlikely; verilator isn't a daemon.
            v3fatal("--pipe-filter: stdin/stdout closed before pipe opened\n");
        }

        UINFO(1, "--pipe-filter: /bin/sh -c " << command);

        ثابت pid_t pid = fork();
        اگر (VL_UNCOVERABLE(pid < 0))
            v3fatal("--pipe-filter: fork failed: " << std::strerror(errno));  // LCOV_EXCL_LINE
        اگر (pid == 0) {  //بچہ
            UINFO(6, "In child");
            close(fd_stdin[P_WR]);
            dup2(fd_stdin[P_RD], 0);
            close(fd_stdout[P_RD]);
            dup2(fd_stdout[P_WR], 1);
            // And stderr comes from parent

            execl("/bin/sh", "sh", "-c", command.c_str(), static_cast<حرف*>(خالی_اشاریہ));
            // Don't use v3fatal, we don't share the common structures any more
            fprintf(stderr, "--pipe-filter: exec failed: %s\n", std::strerror(errno));
            _exit(1);
        } ورنہ {  //والد
            UINFO(6, "In parent, child pid " << pid << " stdin " << fd_stdin[P_WR] << "->"
                                             << fd_stdin[P_RD] << " stdout " << fd_stdout[P_WR]
                                             << "->" << fd_stdout[P_RD]);
            m_pid = pid;
            m_pidExited = جھوٹ;
            m_pid_حالت = 0;
            m_writeFd = fd_stdin[P_WR];
            m_readFd = fd_stdout[P_RD];
            m_readEof = جھوٹ;

            close(fd_stdin[P_RD]);
            close(fd_stdout[P_WR]);

            صحیح flags = fcntl(m_readFd, F_GETFL, 0);
            fcntl(m_readFd, F_SETFL, flags | O_NONBLOCK);

            flags = fcntl(m_writeFd, F_GETFL, 0);
            fcntl(m_writeFd, F_SETFL, flags | O_NONBLOCK);
        }
        UINFO(6, "startFilter complete");
#ورنہ
        v3fatalSrc("--pipe-filter not implemented on this platform");
#اگر_ختم
    }

    خالی stop() {
        اگر (m_pid) stopFilter();
    }
    خالی stopFilter() {
        UINFO(6, "Stopping filter process");
#اگر_تعریف_شدہ INFILTER_PIPE
        close(m_writeFd);
        checkFilter(سچ);
        اگر (!WIFEXITED(m_pid_حالت) || WEXITSTATUS(m_pid_حالت) != 0) {
            v3fatal("--pipe-filter returned bad status");
        }
        m_pid = 0;
        close(m_readFd);
        UINFO(6, "Closed");
#ورنہ
        v3fatalSrc("--pipe-filter not implemented on this platform");
#اگر_ختم
    }

محفوظ:
    دوست جماعت VInFilter;
    // Read file contents and return it
    منطقی readWholefile(ثابت string& filename, StrList& outl) {
        ثابت خودکار it = m_contentsMap.ڈھونڈو(filename);
        اگر (it != m_contentsMap.انتہا()) {
            outl.ڈالو_عقب(it->دوسرا);
            واپس سچ;
        }
        اگر (!readContents(filename, outl)) واپس جھوٹ;
        اگر (list_سائز(outl) < INFILTER_CACHE_MAX) {
            // Cache small files (only to save space)
            // It's quite common to `include "timescale" thousands of times
            // This isn't so important if it's just an open(), but filtering can be slow
            m_contentsMap.emplace(filename, listString(outl));
        }
        واپس سچ;
    }
    ساکن سائز_t list_سائز(ثابت StrList& sl) {
        سائز_t نتیجہ = 0;
        برائے (ثابت string& i : sl) نتیجہ += i.لمبائی_2();
        واپس نتیجہ;
    }
    ساکن string listString(ثابت StrList& sl) {
        string نتیجہ;
        برائے (ثابت string& i : sl) نتیجہ += i;
        واپس نتیجہ;
    }
    // CONSTRUCTORS
    صریح VInFilterImp(ثابت string& command) { آغاز(command); }
    ~VInFilterImp() { stop(); }
};

//######################################################################
// VInFilter
// Just dispatch to the implementation

VInFilter::VInFilter(ثابت string& command) { m_impp = نیا VInFilterImp{command}; }
VInFilter::~VInFilter() {
    اگر (m_impp) VL_DO_CLEAR(حذف m_impp, m_impp = خالی_اشاریہ);
}

منطقی VInFilter::readWholefile(ثابت string& filename, VInFilter::StrList& outl) {
    UASSERT(m_impp, "readWholefile on invalid filter");
    واپس m_impp->readWholefile(filename, outl);
}

//######################################################################
// V3OutFormatter: A class for printing code with automatic indentation.

V3_برآمد_Formatter::V3_برآمد_Formatter(V3_برآمد_Formatter::Language lang)
    : m_lang{lang} {
    m_بلاک_Indent = v3Global.opt.decoration() ? 4 : 1;
    m_comma_چوڑائی = v3Global.opt.decoration() ? 50 : 150;
}

//----------------------------------------------------------------------

ساکن constexpr صحیح MAXSPACE = 80;  // After this indent, stop indenting more

// Table of spaces, so we don't have to alloc/free them all the time
ساکن ثابت std::array<std::string, MAXSPACE + 1> s_indentSpaces = []() {
    std::array<std::string, MAXSPACE + 1> جدول;
    برائے (صحیح i = 0; i <= MAXSPACE; ++i) جدول[i] = std::string(static_cast<سائز_t>(i), ' ');
    واپس جدول;
}();

ثابت std::string& V3_برآمد_Formatter::indentSpaces(صحیح عدد) {
    // Indent the specified number of spaces.
    واپس s_indentSpaces[std::max(0, std::min(عدد, MAXSPACE))];
}

منطقی V3_برآمد_Formatter::token_مطابقت(ثابت حرف* cp, ثابت حرف* cmp) {
    جب (*cmp && *cmp == *cp) {
        ++cp;
        ++cmp;
    }
    اگر (*cmp) واپس جھوٹ;
    اگر (*cp && !std::خلا_ہے(*cp)) واپس جھوٹ;
    واپس سچ;
}

منطقی V3_برآمد_Formatter::token_آغاز(ثابت حرف* cp) {
    واپس (token_مطابقت(cp, "begin") || token_مطابقت(cp, "case") || token_مطابقت(cp, "casex")
            || token_مطابقت(cp, "casez") || token_مطابقت(cp, "class") || token_مطابقت(cp, "function")
            || token_مطابقت(cp, "interface") || token_مطابقت(cp, "module") || token_مطابقت(cp, "package")
            || token_مطابقت(cp, "task"));
}

منطقی V3_برآمد_Formatter::token_انتہا(ثابت حرف* cp) {
    واپس (token_مطابقت(cp, "end") || token_مطابقت(cp, "endcase") || token_مطابقت(cp, "endclass")
            || token_مطابقت(cp, "endfunction") || token_مطابقت(cp, "endinterface")
            || token_مطابقت(cp, "endmodule") || token_مطابقت(cp, "endpackage")
            || token_مطابقت(cp, "endtask"));
}

منطقی V3_برآمد_Formatter::token_Not_آغاز(ثابت حرف* cp) {
    واپس (token_مطابقت(cp, "export") || token_مطابقت(cp, "import"));
}

صحیح V3_برآمد_Formatter::انتہا_سطحیں(ثابت حرف* strg) {
    صحیح سطحیں = m_indentLevel;
    {
        ثابت حرف* cp = strg;
        جب (std::خلا_ہے(*cp)) ++cp;
        انتخاب (*cp) {
        حالت '\n':  // Newlines.. No need for whitespace before it
            واپس 0;
        حالت '#':  // Preproc directive
            اگر (m_lang == LA_C) واپس 0;
        }
        {
            // label/public/private:  Deindent by 2 spaces
            ثابت حرف* mp = cp;
            برائے (; std::حرف_یا_ہندسہ(*mp); ++mp) {}
            اگر (mp[0] == ':' && mp[1] != ':') واپس (سطحیں - m_بلاک_Indent / 2);
        }
    }
    // We want "} else {" to be one level to the left of normal
    برائے (ثابت حرف* cp = strg; *cp; ++cp) {
        انتخاب (*cp) {
        حالت '}':
        حالت ')': سطحیں -= m_بلاک_Indent; توڑو;
        حالت '<':
            اگر (m_lang == LA_XML) {
                اگر (cp[1] == '/') سطحیں -= m_بلاک_Indent;
            }
            توڑو;
        حالت 'e':
            اگر (m_lang == LA_VERILOG && token_انتہا(cp)) سطحیں -= m_بلاک_Indent;
            توڑو;
        حالت '\t':
        حالت ' ': توڑو;  // Continue
        طےشدہ: واپس سطحیں;  // Letter
        }
    }
    واپس سطحیں;
}

خالی V3_برآمد_Formatter::putns(ثابت Ast_گرہ* nodep, ثابت حرف* strg) {
    اگر (!v3Global.opt.decoration()) {
        putsOutput(strg);
        واپس;
    }

    ثابت منطقی put_گرہ_Decoration =  //
        nodep  //
        && v3Global.opt.decorationNodes()  //
        && !v3Global.opt.protectIds()  //
        && (m_ماخذ_آخری_Filenameno != nodep->fileline()->filenameno()  //
            || m_ماخذ_آخری_Lineno != nodep->fileline()->پہلا_Lineno())  //
        && File_سطر::built_اندر_Filename() != nodep->fileline()->filename();

    اگر (m_prependIndent && ((strg[0] && strg[0] != '\n') || put_گرہ_Decoration)) {
        putsNoTracking(indentSpaces(انتہا_سطحیں(strg)));
        m_prependIndent = جھوٹ;
    }

    اگر (put_گرہ_Decoration) {
        ثابت File_سطر* ثابت flp = nodep->fileline();
        m_ماخذ_آخری_Lineno = flp->پہلا_Lineno();
        m_ماخذ_آخری_Filenameno = flp->filenameno();
        ثابت std::string lineno = std::مقصد_string(flp->lineno());
        putsNoTracking("/*" + flp->filename() + ":" + lineno + " " + cvt_مقصد_Hex(nodep) + "*/");
    }

    منطقی notstart = جھوٹ;
    منطقی wordstart = سچ;
    منطقی equalsForBracket = جھوٹ;  // Looking for "= {"
    برائے (ثابت حرف* cp = strg; *cp; ++cp) {
        putcNoTracking(*cp);
        اگر (std::حرف_ہے(*cp)) {
            اگر (wordstart && m_lang == LA_VERILOG && token_Not_آغاز(cp)) notstart = سچ;
            اگر (wordstart && m_lang == LA_VERILOG && !notstart && token_آغاز(cp)) indentInc();
            اگر (wordstart && m_lang == LA_VERILOG && token_انتہا(cp)) indentDec();
        }
        ثابت منطقی was_اندر_Backslash = m_اندر_Backslash;
        m_اندر_Backslash = جھوٹ;
        انتخاب (*cp) {
        حالت '\n':
            ++m_lineno;
            wordstart = سچ;
            اگر (cp[1] == '\0') {
                // Add the indent later, may be a indentInc/indentDec
                // called between now and then
                m_prependIndent = سچ;
            } ورنہ {
                m_prependIndent = جھوٹ;
                putsNoTracking(indentSpaces(انتہا_سطحیں(cp + 1)));
            }
            توڑو;
        حالت ' ': wordstart = سچ; توڑو;
        حالت '\t': wordstart = سچ; توڑو;
        حالت '"':
            wordstart = جھوٹ;
            اگر (!was_اندر_Backslash) m_اندر_String_Literal = !m_اندر_String_Literal;
            توڑو;
        حالت '\\':
            wordstart = جھوٹ;
            اگر (!was_اندر_Backslash) m_اندر_Backslash = سچ;
            توڑو;
        حالت '/':
            اگر (m_lang == LA_C || m_lang == LA_VERILOG) {
                اگر (cp > strg && cp[-1] == '/' && !m_اندر_String_Literal) {
                    // Output ignoring contents to EOL
                    ++cp;
                    جب (*cp && cp[1] && cp[1] != '\n') putcNoTracking(*cp++);
                    اگر (*cp) putcNoTracking(*cp);
                }
            }
            توڑو;
        حالت '{':
            اگر (!m_اندر_String_Literal) {
                اگر (m_lang == LA_C && (equalsForBracket || m_bracketLevel)) {
                    // Break up large code inside "= { ..."
                    m_parenVec.ڈالو(m_indentLevel
                                    * m_بلاک_Indent);  // Line up continuation with block+1
                    ++m_bracketLevel;
                }
                indentInc();
            }
            توڑو;
        حالت '}':
            اگر (!m_اندر_String_Literal) {
                اگر (m_bracketLevel > 0) {
                    m_parenVec.نکالو();
                    --m_bracketLevel;
                }
                indentDec();
            }
            توڑو;
        حالت '(':
            اگر (!m_اندر_String_Literal) {
                indentInc();
                // Line up continuation with open paren, plus one indent
                m_parenVec.ڈالو(m_عمود);
            }
            توڑو;
        حالت ')':
            اگر (!m_اندر_String_Literal) {
                اگر (!m_parenVec.خالی_خانے()) m_parenVec.نکالو();
                indentDec();
            }
            توڑو;
        حالت '<':
            اگر (m_lang == LA_XML) {
                اگر (cp[1] == '/') {
                    // Zero as the > will result in net decrease by one
                } ورنہ اگر (cp[1] == '!' || cp[1] == '?') {
                    indentInc();  // net same indent
                } ورنہ {
                    indentInc();  // net increase by one
                    indentInc();
                }
            }
            توڑو;
        حالت '>':
            اگر (m_lang == LA_XML) {
                indentDec();
                اگر (cp > strg && cp[-1] == '/') indentDec();  // < ..... /> stays same level
            }
            توڑو;
        طےشدہ: wordstart = جھوٹ; توڑو;
        }

        انتخاب (*cp) {
        حالت '=': equalsForBracket = سچ; توڑو;
        حالت ' ': توڑو;
        طےشدہ: equalsForBracket = جھوٹ; توڑو;
        }
    }
}

خالی V3_برآمد_Formatter::put_Break_اظہاریہ() {
    اگر (!m_parenVec.خالی_خانے()) putBreak();
}

// Add a line break if too wide
خالی V3_برآمد_Formatter::putBreak() {
    اگر (!v3Global.opt.decoration()) واپس;
    اگر (!m_nobreak) {
        // char s[1000]; sprintf(s, "{%d,%d}", m_column, m_parenVec.top()); putsNoTracking(s);
        اگر (exceeded_چوڑائی()) {
            putcNoTracking('\n');
            اگر (!m_parenVec.خالی_خانے()) putsNoTracking(indentSpaces(m_parenVec.چوٹی()));
        }
    }
}

خالی V3_برآمد_Formatter::putsQuoted(ثابت string& strg) {
    // Quote \ and " for use inside C programs
    // Don't use to quote a filename for #include - #include doesn't \ escape.
    ثابت string quoted = quoteNameControls(strg);
    putcNoTracking('"');
    برائے (ثابت حرف c : quoted) putcNoTracking(c);
    putcNoTracking('"');
    اگر (strg.ڈھونڈو('\0') != std::string::npos) putcNoTracking('s');  // C++14 std::string
}
خالی V3_برآمد_Formatter::putsNoTracking(ثابت string& strg) {
    اگر (!v3Global.opt.decoration()) {
        putsOutput(strg.c_str());
        واپس;
    }
    // Don't track {}'s, probably because it's a $display format string
    برائے (ثابت حرف c : strg) putcNoTracking(c);
}

خالی V3_برآمد_Formatter::putcNoTracking(حرف chr) {
    اگر (!v3Global.opt.decoration()) {
        putcOutput(chr);
        واپس;
    }
    انتخاب (chr) {
    حالت '\n':
        ++m_lineno;
        m_عمود = 0;
        m_nobreak = سچ;
        توڑو;
    حالت '\t': m_عمود = ((m_عمود + 9) / 8) * 8; توڑو;
    حالت ' ':
    حالت '(':
    حالت '|':
    حالت '&': ++m_عمود; توڑو;
    طےشدہ:
        ++m_عمود;
        m_nobreak = جھوٹ;
        توڑو;
    }
    putcOutput(chr);
}

string V3_برآمد_Formatter::quoteNameControls(ثابت string& namein,
                                         V3_برآمد_Formatter::Language lang) VL_خالص {
    // Encode control chars into output-appropriate escapes
    // Reverse is VString::unquoteSVString
    string برآمد;
    اگر (lang == LA_XML) {
        // Encode chars into XML string
        برائے (ثابت حرف c : namein) {
            اگر (c == '"') {
                برآمد += "&quot;"s;
            } ورنہ اگر (c == '\'') {
                برآمد += "&apos;"s;
            } ورنہ اگر (c == '<') {
                برآمد += "&lt;"s;
            } ورنہ اگر (c == '>') {
                برآمد += "&gt;"s;
            } ورنہ اگر (c == '&') {
                برآمد += "&amp;"s;
            } ورنہ اگر (std::isprint(c)) {
                برآمد += c;
            } ورنہ {
                برآمد += "&#"s + cvt_مقصد_Str(static_cast<غیرعلامتی صحیح>(c & 0xff)) + ";";
            }
        }
    } ورنہ {
        // Encode control chars into C style escapes
        برائے (ثابت حرف c : namein) {
            اگر (c == '\\' || c == '"') {
                برآمد += "\\"s + c;
            } ورنہ اگر (c == '\n') {
                برآمد += "\\n";
            } ورنہ اگر (c == '\r') {
                برآمد += "\\r";
            } ورنہ اگر (c == '\t') {
                برآمد += "\\t";
            } ورنہ اگر (std::isprint(c)) {
                برآمد += c;
            } ورنہ {
                // This will also cover \a etc
                ثابت string octal
                    = "\\"s + cvt_مقصد_Str((c >> 6) & 3) + cvt_مقصد_Str((c >> 3) & 7) + cvt_مقصد_Str(c & 7);
                برآمد += octal;
            }
        }
    }
    واپس برآمد;
}

//----------------------------------------------------------------------
// Simple wrappers

خالی V3_برآمد_Formatter::لکھو(ثابت حرف* fmt...) {
    constexpr سائز_t bufsize = 5000;
    حرف sbuff[bufsize];
    va_list ap;
    va_آغاز(ap, fmt);
    VL_VSNPRINTF(sbuff, bufsize, fmt, ap);
    va_انتہا(ap);
    یہ->puts(sbuff);
}

//######################################################################
// V3OutFormatter: A class for printing to a file, with automatic indentation of C++ code.

V3_برآمد_File::V3_برآمد_File(ثابت string& filename, V3_برآمد_Formatter::Language lang)
    : V3_برآمد_Formatter{lang}
    , m_filename{filename}
    , m_bufferp{نیا std::array<حرف, WRITE_بفر_سائز_بائٹیں>{}} {
    اگر ((m_fp = V3File::new_fopen_w(filename)) == خالی_اشاریہ) {
        v3fatal("Can't write file: " << filename);
    }
}

V3_برآمد_File::~V3_برآمد_File() {
    write_بلاک();

    اگر (m_fp) fclose(m_fp);
    m_fp = خالی_اشاریہ;
}

خالی V3_برآمد_File::putsForceIncs() {
    ثابت VStringList& forceIncs = v3Global.opt.forceIncs();
    برائے (ثابت string& i : forceIncs) puts("#include \"" + i + "\"\n");
}

خالی V3_برآمد_CFile::putsGuard() {
    UASSERT(!m_guard, "Already called putsGuard in emit file");
    m_guard = سچ;
    string var = VString::upcase("VERILATED_"s + V3Os::filenameNonDir(filename()) + "_");
    برائے (حرف& c : var) {
        اگر (!std::حرف_یا_ہندسہ(c)) c = '_';
    }
    puts("\n#ifndef " + var + "\n");
    puts("#define " + var + "  // guard\n");
}

//######################################################################
// V3OutStream

V3_برآمد_دھارا::V3_برآمد_دھارا(std::ostream& ostream, V3_برآمد_Formatter::Language lang)
    : V3_برآمد_Formatter{lang}
    , m_ostream{ostream} {}

//######################################################################
// VIdProtect

جماعت VIdProtectImp final {
    // MEMBERS
    V3Mutex m_mutex;  // Protects members
    std::map<ثابت std::string, std::string> m_nameMap;  // Map of old name into new name
    std::unordered_سیٹ<std::string> m_new_Id_سیٹ VL_GUARDED_BY(m_mutex);  // Which new names exist
محفوظ:
    // CONSTRUCTORS
    دوست جماعت VIdProtect;
    ساکن VIdProtectImp& singleton() VL_MT_محفوظ {
        ساکن VIdProtectImp s;
        واپس s;
    }

عوامی:
    VIdProtectImp() {
        passthru("this");
        passthru("TOP");
        passthru("vlInst");
        passthru("vlSelf");
        passthru("vlSymsp");
    }
    ~VIdProtectImp() = طےشدہ;
    // METHODS
    string passthru(ثابت string& old) VL_MT_محفوظ_EXCLUDES(m_mutex) {
        اگر (!v3Global.opt.protectIds()) واپس old;
        ثابت V3_تالا_Guard تالا_2{m_mutex};
        ثابت خودکار it = m_nameMap.ڈھونڈو(old);
        اگر (it != m_nameMap.انتہا()) {
            // No way to go back and correct the older crypt name
            UASSERT(old == it->دوسرا,
                    "Pass-thru request for '" + old + "' after already --protect-ids of it.");
        } ورنہ {
            m_nameMap.emplace(old, old);
            m_new_Id_سیٹ.داخل_کرو(old);
        }
        واپس old;
    }
    string protectIf(ثابت string& old, منطقی doIt) VL_MT_محفوظ_EXCLUDES(m_mutex) {
        اگر (!v3Global.opt.protectIds() || old.خالی_خانے() || !doIt) واپس old;
        ثابت V3_تالا_Guard تالا_2{m_mutex};
        ثابت خودکار pair = m_nameMap.emplace(old, "");
        اگر (pair.دوسرا) {
            string برآمد;
            اگر (v3Global.opt.debugProtect()) {
                // This lets us see the symbol being protected to debug cases
                // where e.g. the definition is protect() but reference is
                // missing a protect()
                برآمد = "PS" + old;
            } ورنہ {
                VHashSha512 digest{v3Global.opt.protect_کلید_Defaulted()};
                digest.داخل_کرو(old);
                // Add "PS" prefix (Protect Symbols) as cannot start symbol with number
                برآمد = "PS" + digest.digestSymbol24();
                // See if we can shrink the digest symbol to something smaller
                برائے (سائز_t len = 6; len < برآمد.سائز() - 3; len += 3) {
                    ثابت string tryout = برآمد.substr(0, len);
                    اگر (m_new_Id_سیٹ.داخل_کرو(tryout).دوسرا) {
                        برآمد = tryout;
                        توڑو;
                    }
                }
            }
            pair.پہلا->دوسرا = برآمد;
        }
        واپس pair.پہلا->دوسرا;
    }
    string protect_الفاظ_If(ثابت string& old, منطقی doIt) VL_MT_محفوظ {
        // Split at " " (for traces), "." (for scopes), "->", "(", "&", ")" (for self pointers)
        اگر (!(doIt && v3Global.opt.protectIds())) واپس old;
        string برآمد;
        string::سائز_type آغاز = 0;
        // space, ., ->
        جب (سچ) {
            // When C++11, use find_if and lambda
            string::سائز_type pos = string::npos;
            string separator;
            trySep(old, آغاز, " ", pos /*ref*/, separator /*ref*/);
            trySep(old, آغاز, ".", pos /*ref*/, separator /*ref*/);
            trySep(old, آغاز, "->", pos /*ref*/, separator /*ref*/);
            trySep(old, آغاز, "(", pos /*ref*/, separator /*ref*/);
            trySep(old, آغاز, "&", pos /*ref*/, separator /*ref*/);
            trySep(old, آغاز, ")", pos /*ref*/, separator /*ref*/);
            اگر (pos == string::npos) توڑو;
            برآمد += protectIf(old.substr(آغاز, pos - آغاز), سچ) + separator;
            آغاز = pos + separator.لمبائی_2();
        }
        برآمد += protectIf(old.substr(آغاز), سچ);
        واپس برآمد;
    }
    خالی writeMapFile(ثابت string& filename) ثابت {
        V3_برآمد_Xml_File کا{filename};
        کا.puts_سرخی();
        کا.puts("<!-- DESCR"
                "IPTION: Verilator output: XML representation of netlist -->\n");
        کا.puts("<verilator_id_map>\n");
        {
            برائے (ثابت خودکار& itr : m_nameMap) {
                کا.puts("<map from=\"" + itr.دوسرا + "\" to=\"" + itr.پہلا + "\"/>\n");
            }
        }
        کا.puts("</verilator_id_map>\n");
    }

نجی:
    خالی trySep(ثابت string& old, string::سائز_type آغاز, ثابت string& trySep,
                string::سائز_type& posr, string& separatorr) VL_خالص {
        ثابت string::سائز_type trypos = old.ڈھونڈو(trySep, آغاز);
        اگر (trypos != string::npos) {
            اگر (posr == string::npos || (posr > trypos)) {
                posr = trypos;
                separatorr = trySep;
            }
        }
    }
};

string VIdProtect::ifNoProtect(ثابت string& اندر) VL_MT_محفوظ {
    واپس v3Global.opt.protectIds() ? "" : اندر;
}
string VIdProtect::protectIf(ثابت string& old, منطقی doIt) VL_MT_محفوظ {
    واپس VIdProtectImp::singleton().protectIf(old, doIt);
}
string VIdProtect::protect_الفاظ_If(ثابت string& old, منطقی doIt) VL_MT_محفوظ {
    واپس VIdProtectImp::singleton().protect_الفاظ_If(old, doIt);
}
خالی VIdProtect::writeMapFile(ثابت string& filename) {
    VIdProtectImp::singleton().writeMapFile(filename);
}
