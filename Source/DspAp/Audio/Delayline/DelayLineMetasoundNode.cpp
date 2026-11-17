#include "DelayLineMetasoundNode.h"

#include "MetasoundNodeRegistrationMacro.h"
#include "MetasoundParamHelper.h"
#include "MetasoundVertex.h"

#define LOCTEXT_NAMESPACE "DelayLine"

namespace Metasound
{

	namespace DelayLineNode
	{
		METASOUND_PARAM(DelayValueName, "Delay value", "Delay applied to input signal in seconds.");
		METASOUND_PARAM(FeedbackValueName, "Feedback value", "Amount of feedback applied to the delay signal in range 0 to 1.");
		METASOUND_PARAM(AudioInputName, "Input", "The Input signal");
		METASOUND_PARAM(AudioOutputName, "Output", "The output signal with Delay being applied filtering applied");
	}

	const FNodeClassMetadata& DelayLine::GetNodeInfo()
	{
		auto CreateNodeClassMetadata = []() -> FNodeClassMetadata
			{
				FVertexInterface NodeInterface = DeclareVertexInterface();

				FNodeClassMetadata Metadata
				{
						FNodeClassName { "CityUniversity", "Delay line", "Audio" },
						1, // Major Version
						0, // Minor Version
						METASOUND_LOCTEXT("DelayLineNodeDisplayName", "Delay line"),
						METASOUND_LOCTEXT("DelayLineNodeDesc", "My implementation of delay line using circular buffer"),
						PluginAuthor,
						PluginNodeMissingPrompt,
						NodeInterface,
						{ },
						{ },
						FNodeDisplayStyle{}
				};

				return Metadata;
			};

		static const FNodeClassMetadata Metadata = CreateNodeClassMetadata();
		return Metadata;
	}

	const FVertexInterface& DelayLine::DeclareVertexInterface()
	{
		using namespace DelayLineNode;

		static const FVertexInterface Interface(
			FInputVertexInterface(
				TInputDataVertex<float>
				(METASOUND_GET_PARAM_NAME_AND_METADATA(DelayValueName)),
				TInputDataVertex<float>
				(METASOUND_GET_PARAM_NAME_AND_METADATA(FeedbackValueName)),
				TInputDataVertex<FAudioBuffer>
				(METASOUND_GET_PARAM_NAME_AND_METADATA(AudioInputName))
			),
			FOutputVertexInterface(
				TOutputDataVertex<FAudioBuffer>
				(METASOUND_GET_PARAM_NAME_AND_METADATA(AudioOutputName))
			)
		);

		return Interface;
	}

	TUniquePtr<IOperator> DelayLine::CreateOperator(const FBuildOperatorParams& InParams, FBuildResults& OutResults)
	{
		using namespace DelayLineNode;

		const FInputVertexInterfaceData& InputData = InParams.InputData;

		TDataReadReference<float> DelayValue = InputData.GetOrCreateDefaultDataReadReference<float>(METASOUND_GET_PARAM_NAME(DelayValueName), InParams.OperatorSettings);
		TDataReadReference<float> FeedbackValue = InputData.GetOrCreateDefaultDataReadReference<float>(METASOUND_GET_PARAM_NAME(FeedbackValueName), InParams.OperatorSettings);
		FAudioBufferReadRef InputAudio = InputData.GetOrCreateDefaultDataReadReference<FAudioBuffer>(METASOUND_GET_PARAM_NAME(AudioInputName), InParams.OperatorSettings);

		return MakeUnique<DelayLine>(InParams.OperatorSettings, DelayValue, FeedbackValue, InputAudio);
	}

	DelayLine::DelayLine(const FOperatorSettings& InSettings, const FFloatReadRef& Delay, const FFloatReadRef& Feedback, const FAudioBufferReadRef& Input)
		: DelayValue(Delay)
		, FeedbackValue(Feedback)
		, AudioInput(Input)
		, AudioOutput(FAudioBufferWriteRef::CreateNew(InSettings))
		, m_SampleRate(InSettings.GetSampleRate())
		, m_CircularBuffer(InSettings.GetSampleRate(), 2.0f)
	{
	}

	void DelayLine::BindInputs(FInputVertexInterfaceData& InOutVertexData)
	{
		using namespace DelayLineNode;

		InOutVertexData.BindReadVertex(METASOUND_GET_PARAM_NAME(DelayValueName), DelayValue);
		InOutVertexData.BindReadVertex(METASOUND_GET_PARAM_NAME(FeedbackValueName), FeedbackValue);
		InOutVertexData.BindReadVertex(METASOUND_GET_PARAM_NAME(AudioInputName), AudioInput);
	}

	void DelayLine::BindOutputs(FOutputVertexInterfaceData& InOutVertexData)
	{
		using namespace DelayLineNode;

		InOutVertexData.BindReadVertex(METASOUND_GET_PARAM_NAME(AudioOutputName), AudioOutput);
	}

	void DelayLine::Execute()
	{
		// Retrieve raw pointers to the underlying arrays of input and output audio
		const float* InputBuffer = AudioInput->GetData();
		float* OutputBuffer = AudioOutput->GetData();

		// Iterate through every single audio sample in the current block
		for (int i = 0; i < AudioInput->Num(); i++)
		{
			// Get the incoming sample (the dry signal)
			float dryInput = InputBuffer[i];

			// Retrieve the historical sample from the circular buffer (the wet signal)
			float delayedInput = m_CircularBuffer.Read(*DelayValue, m_SampleRate);

			// Output the combination of the dry signal and the echo to the listener
			OutputBuffer[i] = dryInput + delayedInput;

			// Attenuate the delayed signal using the feedback parameter to create decaying echoes
			delayedInput *= *FeedbackValue;

			// Store the dry input plus the newly attenuated feedback back into the buffer so it can be read again in the future
			m_CircularBuffer.Add(dryInput + delayedInput);
		}
	}

	METASOUND_REGISTER_NODE(FDelayLineNode)
}

#undef LOCTEXT_NAMESPACE
