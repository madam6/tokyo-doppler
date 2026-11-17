#include "DopplerEffectMetasoundNode.h"

#include "MetasoundNodeRegistrationMacro.h"
#include "MetasoundParamHelper.h"
#include "MetasoundVertex.h"


namespace Metasound
{
    const FNodeClassMetadata& DopplerCalculator::GetNodeInfo()
    {
        auto CreateNodeClassMetadata = []() -> FNodeClassMetadata
            {
                FVertexInterface NodeInterface = DeclareVertexInterface();

                FNodeClassMetadata Metadata
                {
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
        float speedOfSound = *SpeedOfSoundValue;
        float observerVelocity = *ObserverVelocityValue;
        float sourceVelocity = *SourceVelocityValue;

        // Prevent division by zero if the source travels at the speed of sound
        if (abs(speedOfSound - sourceVelocity) <= 0.0001f)
        {
            *PitchMultiplierOutput = 1.0f;
            return;
        }

        // The Standard Doppler Formula
        // f0 = ((v +- vo) / (v +- vs)) * fs
        // f0 = observer frequency
        // v = speed of sound = speedOfSound
        // vo = observer velocity = observerVelocity
        // vs = source velocity = sourceVelocity
        // fs = emitted frequency
        float PitchScale = (speedOfSound + observerVelocity) / (speedOfSound - sourceVelocity);

        // Make sure that pitch is negative to avoid any bugs
        *PitchMultiplierOutput = std::max(0.0f, PitchScale);
    }

    METASOUND_REGISTER_NODE(FDopplerCalculatorNode)
}

#undef LOCTEXT_NAMESPACE
