# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

@AGENTS.md

## Claude-specific

Distil planning documents (see *Planning documents* in `AGENTS.md`) with a Fable subagent (the
`Agent` tool with `model: "fable"`), since that step loses information for good if done badly. If
Fable is not available on the account, use the most capable model that is (`model: "opus"`), not
whatever smaller model the session happens to be running. Either way it must be a fresh subagent,
since a fork always inherits the session's model, so give it a self-contained prompt: the plan,
where each kind of content goes, and the rule that every statement must be verified against the
code. Review its output before committing it.
