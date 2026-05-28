// Post-build tool: adds units:unit annotations to a JUCE-generated dsp.ttl.
//
// JUCE's LV2 TTL writer does not emit units:unit. This tool is compiled
// per-effect (same SURGE_FX_IS_<NAME> selection as the plugin), introspects
// the effect's parameter metadata via sst-effects, and rewrites the TTL
// in place so MODEP/hosts display correct units (Hz, ms, %, dB).
//
// Usage: patch-lv2-units-<effect> <path/to/dsp.ttl>

#include <fstream>
#include <iostream>
#include <map>
#include <memory>
#include <sstream>
#include <string>

#include <sst/effects/ConcreteConfig.h>

#if defined(SURGE_FX_IS_FLANGER)
#include <sst/effects/Flanger.h>
using SurgeFXType = sst::effects::flanger::Flanger<sst::effects::core::ConcreteConfig>;

#elif defined(SURGE_FX_IS_PHASER)
#include <sst/effects/Phaser.h>
using SurgeFXType = sst::effects::phaser::Phaser<sst::effects::core::ConcreteConfig>;

#elif defined(SURGE_FX_IS_REVERB1)
#include <sst/effects/Reverb1.h>
using SurgeFXType = sst::effects::reverb1::Reverb1<sst::effects::core::ConcreteConfig>;

#elif defined(SURGE_FX_IS_DELAY)
#include <sst/effects/Delay.h>
using SurgeFXType = sst::effects::delay::Delay<sst::effects::core::ConcreteConfig>;

#elif defined(SURGE_FX_IS_ROTARY)
#include <sst/effects/RotarySpeaker.h>
using SurgeFXType = sst::effects::rotaryspeaker::RotarySpeaker<sst::effects::core::ConcreteConfig>;

#else
#error "No effect selected. Define SURGE_FX_IS_<NAME>=1"
#endif

static const char *unitToLV2(const std::string &unit)
{
    if (unit == "%")
        return "units:pc";
    if (unit == "Hz")
        return "units:hz";
    if (unit == "ms")
        return "units:ms";
    if (unit == "dB")
        return "units:db";
    return nullptr;
}

int main(int argc, char **argv)
{
    if (argc < 2)
    {
        std::cerr << "Usage: " << argv[0] << " <dsp.ttl path>\n";
        return 1;
    }

    using Config = sst::effects::core::ConcreteConfig;
    Config::GS gs(44100.0);
    Config::ES es;
    auto effect = std::make_unique<SurgeFXType>(&gs, &es, nullptr);
    effect->initialize();

    std::map<std::string, std::string> unitsByName;
    for (int i = 0; i < SurgeFXType::numParams; ++i)
    {
        auto pmd = effect->paramAt(i);
        if (auto *u = unitToLV2(pmd.unit))
            unitsByName[pmd.name] = u;
    }

    if (unitsByName.empty())
        return 0;

    std::ifstream in(argv[1]);
    if (!in)
    {
        std::cerr << "Cannot open " << argv[1] << "\n";
        return 1;
    }
    std::stringstream buf;
    buf << in.rdbuf();
    std::string content = buf.str();
    in.close();

    // Walk parameter blocks (each starts with "plug:" and ends with " .\n").
    // For blocks matching a known parameter name, append units:unit before the
    // terminating " .\n".
    std::string result;
    result.reserve(content.size() + 256);

    size_t pos = 0;
    while (pos < content.size())
    {
        size_t blockStart = content.find("plug:", pos);
        if (blockStart == std::string::npos)
        {
            result.append(content, pos, std::string::npos);
            break;
        }

        result.append(content, pos, blockStart - pos);

        size_t blockEnd = content.find(" .\n", blockStart);
        if (blockEnd == std::string::npos)
        {
            result.append(content, blockStart, std::string::npos);
            break;
        }
        size_t terminatorEnd = blockEnd + 3;
        std::string block(content, blockStart, terminatorEnd - blockStart);

        if (block.find("a lv2:Parameter ;") != std::string::npos)
        {
            const std::string labelTag = "rdfs:label \"";
            size_t labelStart = block.find(labelTag);
            if (labelStart != std::string::npos)
            {
                labelStart += labelTag.size();
                size_t labelEnd = block.find('"', labelStart);
                if (labelEnd != std::string::npos)
                {
                    std::string name(block, labelStart, labelEnd - labelStart);
                    auto it = unitsByName.find(name);
                    if (it != unitsByName.end())
                    {
                        // Replace trailing " .\n" with " ;\n\tunits:unit XXX .\n"
                        std::string suffix = " ;\n\tunits:unit " + it->second + " .\n";
                        block.replace(block.size() - 3, 3, suffix);
                    }
                }
            }
        }

        result.append(block);
        pos = terminatorEnd;
    }

    std::ofstream out(argv[1]);
    out << result;
    return 0;
}
