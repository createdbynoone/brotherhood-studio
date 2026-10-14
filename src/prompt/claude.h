#pragma once
#include <QString>
#include <QStringList>

// Prompt generation runs through the Claude Code CLI in headless mode so it bills against the
// subscription, not per-token API usage. Two stages, two models: Sonnet writes the brief from the
// reference + product, Opus turns brief + images into the final image prompt. The transcript is
// streamed (stream-json) so we can fail loudly if a reference image was never actually Read.
// Blocking: call from worker threads.
namespace ClaudeCli {

bool isInstalled();
QString generateBrief(const QStringList &refs, const QStringList &products, const QString &specs);

struct PromptResult { QString prompt; QString memoryId; };
PromptResult generatePrompt(const QStringList &refs, const QStringList &products, const QString &description, const QString &specs);

}
