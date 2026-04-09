---
name: optimize-claude
description: Audit and optimize Claude Code configuration, skills, and CLAUDE.md for maximum AI performance. Use to tune Claude's effectiveness on this repo.
allowed-tools: Bash Read Grep Glob Edit Write
argument-hint: "[audit|fix|report]"
---

# Optimize Claude Configuration

Audit and optimize Claude Code setup for the MyAthan firmware repo.

## Model Selection Engine

### Available Models
- **Opus 4.6** (`claude-opus-4-6`) — Best reasoning. ~$15/M in, $75/M out. Use for hard problems.
- **Sonnet 4.6** (`claude-sonnet-4-6`) — Best value. ~$3/M in, $15/M out. Default choice.
- **Haiku 4.5** (`claude-haiku-4-5-20251001`) — Fastest/cheapest. ~$0.25/M in, $1.25/M out. Use for simple tasks.

### Decision Matrix — Always pick the cheapest option that gets the job done

| Task Type | Model | Version | Effort | Thinking | 1M Context | Cost |
|-----------|-------|---------|--------|----------|------------|------|
| Prayer math, high-latitude algorithms | Opus | 4.6 | max | ON | no | $$$$ |
| Architecture, cross-repo schema cascades | Opus | 4.6 | high | ON | no | $$$ |
| Multi-module debugging (scheduler+audio+prayer) | Opus | 4.6 | high | ON | no | $$$ |
| Feature implementation, new driver/module | Sonnet | 4.6 | high | OFF | no | $$ |
| Bug fixes, config changes | Sonnet | 4.6 | med | OFF | no | $$ |
| Code review (ESP32 safety checklist) | Sonnet | 4.6 | high | OFF | no | $$ |
| Single-file edits, header updates | Sonnet | 4.6 | med | OFF | no | $$ |
| Build/flash/test commands | Haiku | 4.5 | low | OFF | no | $ |
| Simple config.json tweaks | Haiku | 4.5 | low | OFF | no | $ |
| Git operations, file lookups | Haiku | 4.5 | low | OFF | no | $ |

### Configuration Rules

**Version:** Always 4.6 for Opus/Sonnet (strictly better). Haiku = 4.5 (only version).

**Thinking mode:**
- **ON** when: prayer math (trig, angle-of-sun), high-latitude edge cases, multi-module debugging, config migration design, security analysis
- **OFF** when: everything else. Embedded code follows clear patterns; thinking adds cost without benefit.
- Rule of thumb: if you can describe the change in one sentence, thinking is OFF.

**1M context:**
- Almost **never** for this repo — firmware is ~20 source files, well under standard context.
- Only if reading ARCHITECTURE.md (36KB) + multiple source files + tests simultaneously.
- Never for build/flash/test, single-file edits, or standard feature work.

**Effort levels:**
- `max` — Prayer calculation only. Wrong math = wrong prayer times (high stakes).
- `high` — Multi-file changes, code review (ESP32 safety matters), new modules.
- `med` — Bug fixes, config changes, single-file work. **This is the default.**
- `low` — CLI commands (build/flash/test), lookups, git operations.

**Cost escalation rule:** Start at the cheapest tier. Only escalate if:
1. The task failed or produced poor results at the current tier
2. The task inherently requires deeper reasoning (see matrix above)
3. Never pre-escalate "just to be safe" — that wastes budget

## Steps

Based on argument:

### `audit` (default)
1. Read `CLAUDE.md` and check for:
   - [ ] Project overview and tech stack documented
   - [ ] Architecture section with file paths
   - [ ] Hardware constraints documented (RAM, flash, GPIO)
   - [ ] Key modules listed with locations
   - [ ] Development/build commands documented
   - [ ] Cross-repo references (core repo link)
   - [ ] Model selection decision matrix (model + version + effort + thinking + 1M)
   - [ ] Cost tier guidance with escalation rules
   - [ ] Common development patterns
   - [ ] Testing strategy documented
2. Scan all skills in `.claude/skills/*/SKILL.md`:
   - [ ] Each skill has a clear, trigger-friendly description
   - [ ] `allowed-tools` matches the skill's actual needs
   - [ ] `disable-model-invocation` is only set for pure CLI-runner skills
   - [ ] Error recovery instructions included for skills that can fail
   - [ ] Skills that analyze code have `Grep` and `Glob` tools
   - [ ] Skills that fix code have `Edit` tool
3. Check `.claude/settings.json`:
   - [ ] Read-only tools auto-allowed (Read, Glob, Grep)
   - [ ] No overly permissive settings
4. Check `.github/workflows/claude.yml`:
   - [ ] Claude Code Action configured
   - [ ] Proper permissions (contents: read, pull-requests: write, issues: write)
5. Calculate optimization score (checklist items passed / total)
6. Report findings with specific recommendations

### `fix`
1. Run the `audit` checklist above
2. Automatically fix issues found:
   - Add missing tools to skill `allowed-tools`
   - Remove unnecessary `disable-model-invocation` flags
   - Add missing sections to CLAUDE.md
   - Create settings.json if missing
3. Report what was fixed

### `report`
1. Run `audit` and output a concise summary:
   - Overall score: X/Y checks passing
   - Top 3 highest-impact improvements
   - Model selection recommendations for recent git activity

## Skill Optimization Checklist

When reviewing any skill, verify:
- **Description**: Should clearly state WHEN to use it (trigger words help Claude match user intent)
- **Tools**: Minimum set needed. Read-only analysis needs `Read Grep Glob`. Code changes need `Edit`. File creation needs `Write`.
- **Model invocation**: Disable ONLY for pure CLI runners (build, flash, dev server). Enable for anything requiring analysis, diagnosis, or decision-making.
- **Error recovery**: Skills that run commands should include "On Failure" instructions
- **Context**: Skills should reference project-specific paths, constraints, and patterns

## ESP32-C3 Constraints (Quick Reference)
- 400KB total RAM, ~300KB usable
- 4MB flash: 2x1MB app partitions (OTA) + 600KB LittleFS
- BLE 5.0 only (no Classic BT, no A2DP audio streaming)
- GPIO6/7 reserved for DFPlayer UART1, GPIO8 for LED, GPIO9 for button
- No SoftwareSerial — hardware UART only
- OTA binary must be <=1.5MB

## Token Efficiency & Caching Tips

- CLAUDE.md should be under 120 lines — dense, structured, no prose
- Use tables and lists over paragraphs (30-50% fewer tokens than prose)
- Include file paths so Claude doesn't need to search (saves Grep/Glob tool calls)
- Reference specific types/functions by name
- Keep cross-repo sync notes brief but actionable
- Put stable content at top of CLAUDE.md, volatile content at bottom (prompt caching works top-down)
- Avoid rewriting CLAUDE.md frequently — each edit invalidates the prompt cache

## Caching & Compression Audit

When auditing, also check:
- [ ] CLAUDE.md has stable content (tech stack, architecture) at the top
- [ ] CLAUDE.md has volatile content (branch names, current sprint) at the bottom
- [ ] CLAUDE.md line count is under 120 (longer = more cached tokens but also more cost)
- [ ] Skills with `disable-model-invocation: true` are truly CLI-only (saves reasoning tokens)
- [ ] settings.json auto-allows read-only tools (saves permission round-trip tokens)
- [ ] No duplicate information between CLAUDE.md and ARCHITECTURE.md/README.md (wasted tokens)
- [ ] Cost & performance optimization section exists in CLAUDE.md
- [ ] Subagent delegation patterns documented (use cheap models for search, expensive for reasoning)
