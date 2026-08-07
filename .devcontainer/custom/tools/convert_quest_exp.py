import math
import re
from pathlib import Path

ROOT = Path("/src/.devcontainer/repo/quests")      # Change to your quest directory

# The optional `q` group captures a surrounding quote, if any, so `quest::exp("400000")` is caught
# as well as `quest::exp(400000)` -- Perl stringifies those, so they are live calls. The quote is
# carried in `pre`/`post` and the backreference forces the closing quote to match the opening one,
# so the original quoting style is preserved exactly.
patterns = [
    re.compile(r'(?P<pre>(?:quest)?::exp\s*\(\s*(?P<q>["\']?))(?P<num>\d+)(?P<post>(?P=q)\s*\))'),
    re.compile(r'(?P<pre>(?:\w+\.)?AddEXP\s*\(\s*(?P<q>["\']?))(?P<num>\d+)(?P<post>(?P=q)\s*\))'),
]

modified_files = 0
modified_calls = 0


def convert(xp):
    return math.ceil(xp ** (1 / 3)) * 20


for path in ROOT.rglob("*"):
    if path.suffix.lower() not in (".pl", ".lua"):
        continue
    text = path.read_text(encoding="latin1")
    original = text

    for pattern in patterns:

        def repl(match):
            global modified_calls

            old = int(match.group("num"))
            new = convert(old)

            modified_calls += 1
            print(f"{path}: {old} -> {new}")

            return f"{match.group('pre')}{new}{match.group('post')}"

        text = pattern.sub(repl, text)

    if text != original:
        # latin1 on the way out to match the read. latin1 is byte-transparent for 0-255, so every
        # byte round-trips exactly and only the numbers change. Writing utf-8 here re-encoded every
        # byte above 0x7F: it mangled Windows-1252 apostrophes and double-encoded the files that
        # were already utf-8 (global_player.lua among them).
        path.write_text(text, encoding="latin1")
        modified_files += 1

print(f"\nModified {modified_calls} EXP calls in {modified_files} files.")
