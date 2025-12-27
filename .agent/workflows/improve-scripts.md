---
description: Iteratively improve static analysis scripts
---

# Improve Static Analysis Scripts

## 1. Run Analysis & Review Output
// turbo
```bash
cd /home/jimmy/SNAKE && ./scripts/analyze_all.sh
```

Review outputs in `scripts/out/`:
- Check for empty files (indicates detection gaps)
- Note low-signal outputs (too terse, missing context)

## 2. Identify Target Script
Pick lowest-rated script from `script_review.md` or one with empty/weak output.

Priority order for improvement:
1. `errors.py` - Critical safety checks
2. `memory_map.py` - Needs legend, pairing
3. `invariants.py` - Expand detection
4. `macros.py` - Detect risky patterns

## 3. Analyze Script Logic
View the target script:
```bash
cat scripts/<target>.py
```

Identify:
- What patterns it searches for (regex, AST parsing)
- What it misses (false negatives)
- What would add value (new heuristics)

## 4. Propose & Implement Changes
Common improvements:
- **errors.py**: Multi-statement lookahead for NULL checks, detect ignored return values
- **memory_map.py**: Add legend header, pair alloc/free by variable name
- **invariants.py**: Detect enum assignments, state machine patterns
- **macros.py**: Flag multi-eval macros, hidden control flow

Edit the script, keeping output token-minimized.

## 5. Test Changes
// turbo
```bash
cd /home/jimmy/SNAKE && python3 scripts/<target>.py
```

Compare old vs new output. Verify:
- No false positives introduced
- New findings are actionable
- Output remains concise

## 6. Update Review Document
If improvement is significant, update rating in `script_review.md`.

## 7. Repeat
Return to step 1 for next script until all scripts rate ≥7/10.
