#pragma once
#include <QList>
#include <QPair>
#include <QString>
#include <QStringList>

#include "higgsfield.h"

// The image engines on Higgsfield, shared by the Prompts and Ecommerce panes.
struct Provider {
    QString shortLabel, label, jobType;
    QStringList ratios, resolutions;                // the short list Marketing Prompts offers (first = default)
    int maxRefs;
    QStringList qualities;                          // empty: the model has no quality setting
    QString defaultQuality;
    QList<QPair<QString, QString>> fixed;           // always-sent params (a model variant, ...)
    QStringList allRatios;                          // everything the model accepts (Ecommerce matches the source photo)
};

// Seedream 5.0 Pro has no 4:5 in its aspect_ratio enum (3:4 is its closest portrait).
// First ratio is the provider default. `higgsfield model get <job type>` lists what each model accepts.
inline const QList<Provider> &providers() {
    static const QList<Provider> p = {
        {"Seedream", "Seedream 5.0 Pro", "seedream_v5_pro", {"3:4", "9:16"}, {"1k", "2k"}, 10, {}, {}, {},
         {"1:1", "4:3", "3:4", "16:9", "9:16", "3:2", "2:3", "21:9"}},
        {"NB Pro", "Nano Banana Pro", "nano_banana_pro", {"4:5", "9:16"}, {"1k", "2k", "4k"}, 14, {}, {}, {},
         {"1:1", "3:2", "2:3", "4:3", "3:4", "4:5", "5:4", "9:16", "16:9", "21:9"}},
        // GPT Image 2.5 in its Sunburst variant (the other one is Flare)
        {"GPT Sunburst", "GPT Image 2.5 Sunburst", "gpt_image_2_5", {"4:5", "9:16", "1:1"}, {"1k", "2k", "4k"}, 10,
         {"low", "medium", "high", "xhigh", "max"}, "high", {{"variant", "sunburst"}},
         {"1:1", "3:2", "2:3", "4:3", "3:4", "4:5", "5:4", "9:16", "16:9", "9:8", "8:9", "21:9"}},
    };
    return p;
}

// Params that decide the price (the prompt and references do not), in the order `generate create` takes them
inline Higgsfield::Params priceParams(const Provider &p, const QString &ratio, const QString &res, const QString &quality,
                                      bool anyRatio = false) {
    const QStringList &ratios = anyRatio ? p.allRatios : p.ratios;
    Higgsfield::Params params = {{"aspect_ratio", ratios.contains(ratio) ? ratio : ratios.first()},
                                 {"resolution", p.resolutions.contains(res) ? res : p.resolutions.last()},
                                 {"prompt", QStringLiteral("cost estimate")}};
    if (!p.qualities.isEmpty()) params.append(qMakePair(QStringLiteral("quality"), QVariant(p.qualities.contains(quality) ? quality : p.defaultQuality)));
    for (const auto &f : p.fixed) params.append(qMakePair(f.first, QVariant(f.second)));
    return params;
}
