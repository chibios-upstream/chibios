# ChibiOS Pull-Request Review Procedure

This is the review procedure for ChibiOS pull requests, carried out by the
`chibios-sheriff` assistant (first layer) and the human maintainers (final say).
It also serves as the review-guidance file for any automated reviewer.

## Role & boundaries
- The automated review is a **first layer and advisory**. It never merges; a human
  maintainer (the project lead or the relevant port owner) makes the merge decision.
- An automated approval is **advisory** and does not substitute for human review.
- **Fork PRs:** *compile* the change, but do **not run** the resulting binary on the
  host (the POSIX simulator would execute contributor code). Run only in a sandbox if
  execution is genuinely needed.

## 0. Intake
- Capture: repo, PR number, author, **base branch**, size (files, +/-), description, linked issue.
- **Targeting rule (upstream-first):** ALL changes land on `main` first. The `stable-*`
  branches only receive **backports of commits already merged on `main`**, selected by the
  maintainer (typically bugfixes/regressions). PRs targeting `stable-*` directly are
  redirected to `main`; the stable backport is decided after the `main` merge.
- Scope sanity: reasonable size, on-topic, not bundling unrelated changes.

## 1. Fetch
- `gh pr checkout <n>` into a worktree/scratch; `git submodule update --init --recursive`
  if `ext` or `tools/ftl` are touched.

## 2. Staged review (stop at the first stage with findings)
Reviews are run in **stages of increasing depth**, and each pass posts **only the
findings of the stage it stopped at**. A stage that yields no problems is passed
through silently — the pass **proceeds to the next stage** in the same session; in
the best case a single pass runs A through D and ends in an advisory-LGTM. If a
stage produces blocking or should-fix findings, **stop there**: post them, let the
author fix, then the next pass resumes from that stage and goes deeper. This avoids
overwhelming the PR with one giant wall-of-text review, and avoids spending
deep-review effort on code that is about to change anyway.

Intake (§0) findings have no stage of their own: they are reported together with the
Stage A pass.

**Re-review passes:** start from the previously posted findings — explicitly verify
each open item against the new head (resolved / still open / fix ineffective) before
looking at anything new. A fix round is judged by whether it closes the loop, not by
whether it changed code; carried findings are restated, not silently dropped.

- **Stage A — build (objective):** the POSIX simulator (host gcc) plus the most
  relevant ARM target(s) for the touched port(s). Failure → **request changes**,
  stop here.
- **Stage B — structural scan (cheap judgment):** layering, conventions, scope,
  headers/docs — the "easy" dimensions of §3. Findings → post, stop here.
- **Stage C — deep review (expensive judgment):** correctness/concurrency,
  registers vs. datasheet, portability, resource/safety — the remaining dimensions
  of §3. Only reached when A and B are clean.
- **Stage D — style (final polish):** `tools/style/stylecheck.py` on the changed
  `.c`/`.h` files, checked **last**, once the content is settled — otherwise every
  fix round would re-trigger it. On `main` PRs the CI style job re-runs on every
  push anyway, so the author sees findings continuously; the sheriff only verifies
  it is clean before an advisory-LGTM.

Nits found early may be carried along to later passes instead of forcing a stop on
their own; only blocking/should-fix findings gate a stage. State in the summary which
stage the pass reached, so the author knows a clean comment is not yet a full review.

The **first review posted on a PR** must include the **frozen disclaimer** (see the
review output template) explaining that the review is articulated in four stages —
so an early stop is not mistaken for the full review. Use it verbatim; do not
regenerate or rephrase it.

## 3. Judgment review, by dimension
Stage B dimensions (cheap, check first):
- **Architectural layering:** the change sits in the correct layer. No vendor/port-isms
  leaking upward (e.g. STM32 register names, family-specific assumptions or behavior
  exposed in the high-level HAL, RT or OSAL); portable code stays portable,
  family-specific code stays in the LLD/port layer; abstractions are not bent to fit
  one silicon family.
- **HAL/port conventions:** driver model, LLD structure, OSAL usage, naming,
  `.chcfg`/`.mk` consistency.
- **Headers/docs/tests:** license headers present/correct; doxygen comments updated;
  test suite updated or added when behavior changes.

Stage C dimensions (deep):
- **Correctness & concurrency (RTOS):** logic, races, ISR-safety, no blocking in ISR,
  critical-section/locking discipline, thread priorities.
- **Hardware/registers:** bitfields, masks, channel counts vs. the datasheet.
- **Portability:** does not break other ports/configs; `#if` conditionals correct; no
  accidental coupling.
- **API/ABI stability:** on `stable-*`, no public API breaks; backward compatibility.
- **Resource/safety:** no prohibited dynamic allocation, stack/footprint, deterministic
  timing, MISRA-oriented hygiene.

## 4. Findings & verdict
- Severity: **blocking** (correctness/safety/API break, or build/style failure),
  **should-fix** (conventions), **nit** (cosmetic).
- Verdict: **request changes** if any blocking item; **comment** if only should-fix/nit;
  **advisory-LGTM** only when stages C and D completed clean (a pass stopped at A or B
  can never be an advisory-LGTM).
- For small, clear issues, offer to draft/push a fix to the PR branch.
- **Trivial ping-pong escalation:** when a PR keeps cycling on trivial items only
  (should-fix/nit findings carried across two or more fix rounds while the
  substantive work is done), the reviewer proactively offers to push the remaining
  fixes to the PR branch instead of requesting another round. On the author's
  confirmation, push the fixes (clearly attributed commits), comment exactly what
  was pushed, and re-verify. Always ask first — it is the contributor's branch.

## 5. Post & handoff
- Post inline comments on the relevant lines plus a summary block with severity-tagged
  findings.
- **Keep it digestible:** post only the current stage's findings (see §2); prefer a
  handful of inline comments over long prose; one summary block per pass.
- **Stay scoped to the PR:** do not discuss unrelated or base-branch problems in PR
  comments — handle those through separate channels. If CI is red for a reason not
  caused by the PR, say only that it is a known project-side issue being addressed;
  no details about the unrelated problem.
- **Question authors only on their own changes:** findings are raised against what
  the PR adds or modifies. Pre-existing anomalies in surrounding code discovered
  during the review are NOT the author's burden — record them for the maintainer
  (issue, knowledge base, separate fix) and leave the author out of it. The same
  principle the CI applies to style (changed lines only) applies to judgment
  findings.
- **Changelog tagging is fixed project-side:** every change's `readme.txt` entry
  references its pull request — `(github PR #N)` — and backports add
  `(backported to <release>)`. Contributors are not expected to get this tagging
  right: the reviewer checks that the entry itself exists and describes the change
  (that part IS the author's, Stage B); tagging/format errors (missing PR number,
  `FIX:`/`NEW:` prefix, wrapping, annotations) are fixed by the reviewer with a
  small commit pushed directly to the PR branch and a one-line comment — never
  raised as findings, never a fix round on their own.
- **Make findings reproducible:** when reporting style findings, include the local
  reproduction hint so the author can self-check before pushing:
  `python3 tools/style/stylecheck.py <file>` (findings are printed as `style:`/
  `error:` lines; the tool always exits 0 — read the output, not the exit code).
  Note: the local tool reports the WHOLE file; CI enforces only findings on lines
  changed by the PR (pre-existing findings in legacy files are printed for
  context but do not fail the gate).
- Summarize to the maintainer; **the human decides the merge.**

## 6. Backports (stable-*)
Backports are **manual by design** — the stable branches have diverged from `main`
(different port layouts, no XHAL, different test trees), so cherry-picks need adaptation
and judgment; there is no automatic mechanism.
- **Selection:** the maintainer picks which merged `main` commits get backported and to
  which `stable-*` branches.
- **Execution:** cherry-pick onto the target branch, adapt to that branch's layout, then
  run the mechanical gates **locally** (stylecheck on touched `.c/.h` + a representative
  build for the touched port) — the stable branches have no CI.
- The backport commit message references the original `main` commit hash.
- **Changelog is the visible record:** the change's entry in `readme.txt` on `main` gains a
  `(backported to <release>)` annotation — one per stable release that received it, e.g.
  `(backported to 21.11.6).` An entry without annotations has not been backported anywhere.
- Landing: via PR (review rules apply) or direct maintainer push.

## Review output template
Post the review as one summary block (plus inline comments on specific lines where useful),
using this shape:

```
🤖 chibios-sheriff — first-layer review (advisory)   ← automated reviews MUST open with this AI-disclosure line

Scope: <what changed, target branch — one line>
Stage reached: A (build) | B (structural) | C (deep) | D (style)   ← per §2; below D means "not a full review yet"

<first review on the PR only — include the frozen disclaimer below, verbatim>:

> **Note:** this is a staged review — **A** build → **B** structural → **C** deep →
> **D** style. Each pass stops at the first stage with findings; after a fix, the
> next pass resumes from there and goes deeper. An early short review is therefore
> not the full review yet.

Blocking:    <correctness/safety/API-break/build or style failures — or omit if none>
Should-fix:  <convention/quality items — or omit>
Notes:       <non-blocking nits/observations — or omit>

Mechanical gates: <style + build result, or what wasn't run this pass>

Verdict: request changes | comment | advisory-LGTM  (+ any condition)
```

Human reviewers use the same structure, minus the `🤖` disclosure line (that line is required
for automated reviewers, which can't carry GitHub's native `[bot]` badge).

## Cadence
- Pull-based: a maintainer brings a PR to a review session.
