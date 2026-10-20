# Checked raster oracle

Purpose: preserve every assertion of the expanded raster harness from Rough
framebuffer PRs 1, 2 and 14 while applying mandatory Ithon checking. This is an
Ithon source migration of the existing oracle; the ICK renderer and maintained
pinned-Ike build graph remain the producer and test entrypoint.

Types: subprocess argv are lists of text, PPM payloads are bytes, dimensions
and coordinates are integers. Each pixel becomes an injective hexadecimal
text representation of its three bytes for exact RGB equality. Grey bounds
use explicitly typed integer components. Standard-library filesystem and
process objects are foreign boundaries with explicit local types.

The original fixtures remain: palette samples, fractional coverage, alpha,
taper, adaptive/reversing curves, dotted joints and moves, clipped phase,
solid-pattern reset, 64 tiled triangles, edge ownership, near/far and crossing
depth, and all 18 malformed inputs. Timeout and byte equality remain gates.

The first actual frontend attempt rejected text multiplied by an integer.
The reduced failure is retained in string-repeat.pi. The 500-field malformed
input is now constructed by a typed range and concatenation, preserving the
exact bytes. The complete final module passes the actual mandatory frontend
and then refuses /bin/false at its first actual renderer invocation. That
local observation proves frontend admission and process refusal, not a
positive renderer run. The positive requires current-head ICK/Ike CI.

Local frontend source d6e83969f82512e920fb17b44326cb54f31d015c has the exact
tree of accepted Ithon main 9d530f4ad51f5b0a09c3e74dbe45cac5bb6d52d3. CI
pins that accepted merge directly. Its declared host substrate is CPython;
the mandatory Ithon checker and loader execute before the oracle. This is
not native-tokenizer, Field Mouse, Android device or physical acceptance.
