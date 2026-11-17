#include "DopplerEffectMetasoundNode.h"

#include "MetasoundNodeRegistrationMacro.h"
#include "MetasoundParamHelper.h"
#include "MetasoundVertex.h"

#define LOCTEXT_NAMESPACE "DopplerCalculator"

namespace Metasound
{
    const FNodeClassMetadata& DopplerCalculator::GetNodeInfo()
    {
        auto CreateNodeClassMetadata = []() -> FNodeClassMetadata
            {
                FVertexInterface NodeInterface = DeclareVertexInterface();

                FNodeClassMetadata Metadata
                {
                        // NOTE: the class name is what saved MetaSound graphs use to find this node.
                        // Changing it orphans the node in every existing asset.
                        FNodeClassName { "CityUniversity", "Doppler Calculator", "Audio" },
                        1, 0,
                        METASOUND_LOCTEXT("DopplerNodeDisplayName", "Doppler Calculator"),
                        METASOUND_LOCTEXT("DopplerNodeDesc", "Calculates pitch multiplier for the Doppler effect"),
                        PluginAuthor,
                        PluginNodeMissingPrompt,
                        NodeInterface,
                        { }, { },
                        FNodeDisplayStyle{}
                };

                return Metadata;
            };

        static const FNodeClassMetadata Metadata = CreateNodeClassMetadata();
        return Metadata;
    }

    const FVertexInterface& DopplerCalculator::DeclareVertexInterface()
    {
        using namespace DopplerNode;

        static const FVertexInterface Interface(
            FInputVertexInterface(
                TInputDataVertex<float>(METASOUND_GET_PARAM_NAME_AND_METADATA(SpeedOfSoundName), 34300.0f),
                TInputDataVertex<float>(METASOUND_GET_PARAM_NAME_AND_METADATA(ObserverVelocityName), 0.0f),
                TInputDataVertex<float>(METASOUND_GET_PARAM_NAME_AND_METADATA(SourceVelName), 0.0f)
            ),
            FOutputVertexInterface(
                TOutputDataVertex<float>(METASOUND_GET_PARAM_NAME_AND_METADATA(PitchMultiplierName))
            )
        );

        return Interface;
    }

    TUniquePtr<IOperator> DopplerCalculator::CreateOperator(const FBuildOperatorParams& InParams, FBuildResults& OutResults)
    {
        using namespace DopplerNode;

        const FInputVertexInterfaceData& InputData = InParams.InputData;

        TDataReadReference<float> SpeedOfSound = InputData.GetOrCreateDefaultDataReadReference<float>(METASOUND_GET_PARAM_NAME(SpeedOfSoundName), InParams.OperatorSettings);
        TDataReadReference<float> ObserverVel = InputData.GetOrCreateDefaultDataReadReference<float>(METASOUND_GET_PARAM_NAME(ObserverVelocityName), InParams.OperatorSettings);
        TDataReadReference<float> SourceVel = InputData.GetOrCreateDefaultDataReadReference<float>(METASOUND_GET_PARAM_NAME(SourceVelName), InParams.OperatorSettings);

        return MakeUnique<DopplerCalculator>(InParams.OperatorSettings, SpeedOfSound, ObserverVel, SourceVel);
    }

    DopplerCalculator::DopplerCalculator(const FOperatorSettings& InSettings, const FFloatReadRef& SpeedOfSound, const FFloatReadRef& ObserverVelocity, const FFloatReadRef& SourceVelocity)
        : SpeedOfSoundValue(SpeedOfSound)
        , ObserverVelocityValue(ObserverVelocity)
        , SourceVelocityValue(SourceVelocity)
        // Initialize the output to a neutral pitch of 1.0
        , PitchMultiplierOutput(FFloatWriteRef::CreateNew(1.f))
    {
    }

    void DopplerCalculator::BindInputs(FInputVertexInterfaceData& InOutVertexData)
    {
        using namespace DopplerNode;

        InOutVertexData.BindReadVertex(METASOUND_GET_PARAM_NAME(SpeedOfSoundName), SpeedOfSoundValue);
        InOutVertexData.BindReadVertex(METASOUND_GET_PARAM_NAME(ObserverVelocityName), ObserverVelocityValue);
        InOutVertexData.BindReadVertex(METASOUND_GET_PARAM_NAME(SourceVelName), SourceVelocityValue);
    }

    void DopplerCalculator::BindOutputs(FOutputVertexInterfaceData& InOutVertexData)
    {
        using namespace DopplerNode;

        InOutVertexData.BindReadVertex(METASOUND_GET_PARAM_NAME(PitchMultiplierName), PitchMultiplierOutput);
    }

    void DopplerCalculator::Execute()
    {
        // Get the current values from the input pins
        const float speedOfSound = *SpeedOfSoundValue;
        const float observerVelocity = *ObserverVelocityValue;
        const float sourceVelocity = *SourceVelocityValue;

        // Prevent division by zero if the source travels at the speed of sound
        if (FMath::Abs(speedOfSound - sourceVelocity) <= 0.0001f)
        {
            *PitchMultiplierOutput = 1.0f;
            return;
        }

        // Classic Doppler equation for a moving source and a moving observer:
        //   f_o = f_s * (v + v_o) / (v - v_s)
        // f_o = frequency heard by the observer
        // f_s = frequency emitted by the source
        // v   = speed of sound              = speedOfSound
        // v_o = observer velocity (towards) = observerVelocity
        // v_s = source velocity (towards)   = sourceVelocity
        // We only need the ratio f_o / f_s, which is the pitch multiplier.
        const float PitchScale = (speedOfSound + observerVelocity) / (speedOfSound - sourceVelocity);

        // Past Mach 1 the denominator flips sign and the ratio goes negative, which a pitch shifter cannot use.
        // Clamp to zero instead.
        *PitchMultiplierOutput = FMath::Max(0.0f, PitchScale);
    }

    METASOUND_REGISTER_NODE(FDopplerCalculatorNode)
}

#undef LOCTEXT_NAMESPACE
