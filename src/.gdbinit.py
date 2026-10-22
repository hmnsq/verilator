# pylint: disable=line-too-long,invalid-name,multiple-statements,missing-function-docstring,missing-class-docstring,missing-module-docstring,no-else-return,too-few-public-methods,unused-argument
# DESCRIPTION: Verilator: GDB startup file with useful defines
# SPDX-FileCopyrightText: 2023 Wilson Snyder
# SPDX-License-Identifier: LGPL-3.0-only OR Artistic-2.0
######################################################################

واردکن tempfile
واردکن gdb  # pylint: disable=import-error


def _vltgdb_get_dump(گره):
    gdb.execute(f'set $_gdb_dump_json_str = AstNode::dumpTreeJsonGdb({node})')
    dump = gdb.execute('printf "%s", $_gdb_dump_json_str', to_string=درست)
    gdb.execute('call (void)free($_gdb_dump_json_str)')
    بازگشت dump


def _vltgdb_tmpfile():
    بازگشت tempfile.NamedTemporaryFile(mode="wt")  # write, text mode


def _vltgdb_fwrite(file, s):
    """Write to file and flush buffer before passing the file to astsee"""
    file.اكتب(s)
    file.flush()


رده AstseeCmd(gdb.Command):
    """Verilator: Pretty print or diff nodes using `astsee`. A node can be:
    * an pointer identifier,
    * an address literal,
    * a GDB value (like `$1`) that stores a dump previously done by the `jstash` command.
    Apart from not taking input from a file, it works exactly like `verilator_astsee`:
    * passing one node gives you a pretty print,
    * passing two nodes gives you a diff,
    * for more options see `astsee` readme/help.
    For examples see internals.rst
    """

    def __init__(self):
        ابر().__init__("jtree", gdb.COMMAND_USER, gdb.COMPLETE_EXPRESSION)

    def _null_check(self, old, جدید):
        err = ""
        اگر old == "<nullptr>\n":
            err += "old == <nullptr>\n"
        اگر جدید == "<nullptr>\n":
            err += "new == <nullptr>"
        اگر err:
            بالاببر gdb.GdbError(err.strip("\n"))

    def invoke(self, arg_str, from_tty):
        از astsee واردکن verilator_cli بهعنوان astsee  # pylint: disable=import-error,import-outside-toplevel
        # we import it here so "no module X" error would occur on typing `jtree` rather than on every gdb start

        # We hack `astsee_verilator`'s arg parser to find arguments with nodes
        # After finding them, we replace them with proper files
        astsee_args = astsee.parser.parse_args(gdb.string_to_argv(arg_str))
        with _vltgdb_tmpfile() بهعنوان oldfile, _vltgdb_tmpfile() بهعنوان newfile, _vltgdb_tmpfile(
        ) بهعنوان metafile:
            اگر astsee_args.file:
                _vltgdb_fwrite(oldfile, _vltgdb_get_dump(astsee_args.file))
                astsee_args.file = oldfile.name
            اگر astsee_args.newfile:
                _vltgdb_fwrite(newfile, _vltgdb_get_dump(astsee_args.newfile))
                astsee_args.newfile = newfile.name
            اگر astsee_args.meta is None:
                # pass
                gdb.execute(f'call AstNode::dumpJsonMetaFileGdb("{metafile.name}")')
                astsee_args.meta = metafile.name
            تلاش:
                astsee.اصلی(astsee_args)
            استثنا SystemExit بهعنوان e:  # astsee prints nice errmsgs on exit(), so rethrow it as GdbError to suppress cryptic python trace
                بالاببر gdb.GdbError("astsee exited with non-zero code") از e


AstseeCmd()
