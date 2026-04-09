---
name: optimize-claude
description: Audit and optimize Claude Code configuration, skills, and CLAUDE.md for maximum AI performance. Use to tune Claude's effectiveness on this repo.
allowed-tools: Bash Read Grep Glob Edit Write
argument-hint: "[audit|fix|report]"
---

# Optimize Claude Configuration

Audit and optimize Claude Code setup for the MyAthan firmware repo.

## Model Selection Guide

Choose the right Claude model for each task type:

| Task Type | Model | Reasoning |
|-----------|-------|-----------|
| Prayer calculation algorithms, high-latitude edge cases | **Opus** | Complex math and astronomical reasoning |
| Architecture changes, cross-repo schema updates | **Opus** | Cascading effects across firmware and core |
| Feature implementation, driver code, new modules | **Sonnet** | Standard coding with good speed/quality balance |
| Bug fixes, config changes, single-file edits | **Sonnet** | Well-scoped changes with clear patterns |
| Build/flash/test commands, simple config tweaks | **Haiku** | Fast CLI execution with minimal reasoning |
| Code review with ESP32 safety checklist | **Sonnet** | Checklist-driven analysis within bounded scope |

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
   - [ ] Model selection guidance section
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

## Token Efficiency Tips

- CLAUDE.md should be under 100 lines — dense, structured, no prose
- Use tables and lists over paragraphs
- Include file paths so Claude doesn't need to search
- Reference specific types/functions by name
- Keep cross-repo sync notes brief but actionable
