#pragma once

#include "CoreMinimal.h"
#include "MetasoundExecutableOperator.h"
#include "MetasoundFacade.h"
#include "MetasoundNodeInterface.h"
#include "MetasoundPrimitives.h"
#include "MetasoundDataReference.h"
#include "MetasoundParamHelper.h"

// Pin names live in the header so the automation tests can look up the output by name.
// The namespace is scoped to this block so it does not leak into files that include this header.
#define LOCTEXT_NAMESPACE "DopplerCalculator"

namespace Metasound
{
    namespace DopplerNode
    {
        METASOUND_PARAM(SpeedOfSoundName, "Speed of Sound", "Speed of sound in cm/s (Default 34300).");
        METASOUND_PARAM(ObserverVelocityName, "Observer Velocity", "Speed of the listener moving towards the source (cm/s).");
        METASOUND_PARAM(SourceVelName, "Source Velocity", "Speed of the source moving towards the listener (cm/s).");
        METASOUND_PARAM(PitchMultiplierName, "Pitch Multiplier", "The calculated Doppler pitch shift factor.");
    }
}

#undef LOCTEXT_NAMESPACE

namespace Metasound
{
    // Control-rate node: turns radial velocities into a frequency ratio.
    // Velocities are signed: positive means "moving towards the other party".
    class DopplerCalculator : public TExecutableOperator<DopplerCalculator>
    {
    public:

        static const FNodeClassMetadata& GetNodeInfo();
        static const FVertexInterface& DeclareVertexInterface();

        static TUniquePtr<IOperator> CreateOperator(const FBuildOperatorParams& InParams, FBuildResults& OutResults);

        // Constructor
        DopplerCalculator(const FOperatorSettings& InSettings,
            const FFloatReadRef& SpeedOfSound,
            const FFloatReadRef& ObserverVelocity,
            const FFloatReadRef& SourceVelocity);

        virtual void BindInputs(FInputVertexInterfaceData& InOutVertexData) override;
        virtual void BindOutputs(FOutputVertexInterfaceData& InOutVertexData) override;

        void Execute();

    private:

        // The speed of sound in cm/s.
        FFloatReadRef SpeedOfSoundValue;

        // How fast the listener is moving TOWARDS the source (cm/s)
        FFloatReadRef ObserverVelocityValue;

        // How fast the source is moving TOWARDS the listener (cm/s)
        FFloatReadRef SourceVelocityValue;


        // The resulting pitch multiplier (1.0 is normal pitch)
        FFloatWriteRef PitchMultiplierOutput;
    };

    using FDopplerCalculatorNode = TNodeFacade<DopplerCalculator>;

}
