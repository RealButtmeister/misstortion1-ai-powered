/*
	==============================================================================

		This file was auto-generated!

		It contains the basic framework code for a JUCE plugin processor.

	==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

#include "Utils.h"

#include <cmath>


//==============================================================================
MisstortionAudioProcessor::MisstortionAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
	: AudioProcessor(BusesProperties()
#if ! JucePlugin_IsMidiEffect
#if ! JucePlugin_IsSynth
		.withInput("Input", AudioChannelSet::stereo(), true)
#endif
		.withOutput("Output", AudioChannelSet::stereo(), true)
#endif
	)
#endif
{
	addParameter(m_paramMix = new AudioParameterFloat("mix", "Mix", NormalisableRange<float>(0.0f, 100.0f), 50.0f));
	addParameter(m_paramGainIn = new AudioParameterFloat("gainin", "Gain in", NormalisableRange<float>(-50.0f, 50.0f, 0.0f, 0.5f, true), 0.0f));
	addParameter(m_paramGainOut = new AudioParameterFloat("gainout", "Gain out", NormalisableRange<float>(-50.0f, 50.0f, 0.0f, 0.5f, true), 0.0f));

	addParameter(m_paramDriveHard = new AudioParameterFloat("drive", "Drive", NormalisableRange<float>(0.0f, 50.0f, 0.0f, 0.5f), 0.0f));
	addParameter(m_paramDriveSoft = new AudioParameterFloat("drive2", "Drive 2", NormalisableRange<float>(0.0f, 50.0f, 0.0f, 0.5f), 0.0f));
	addParameter(m_paramToneHP = new AudioParameterInt("tone", "Tone", 0, 20000, 20));
	addParameter(m_paramToneLP = new AudioParameterInt("tonepost", "Tone Post", 1, 20000, 20000));
	addParameter(m_paramSymmetry = new AudioParameterFloat("symmetry", "Symmetry", NormalisableRange<float>(0.0f, 100.0f), 50.0f));
	addParameter(m_paramFilterMode = new AudioParameterInt("filtermode", "Filter Mode", 0, 2, 0));

	m_genreText = "hard style";
}

MisstortionAudioProcessor::~MisstortionAudioProcessor()
{
}

//==============================================================================
const String MisstortionAudioProcessor::getName() const
{
	return JucePlugin_Name;
}

bool MisstortionAudioProcessor::acceptsMidi() const
{
#if JucePlugin_WantsMidiInput
	return true;
#else
	return false;
#endif
}

bool MisstortionAudioProcessor::producesMidi() const
{
#if JucePlugin_ProducesMidiOutput
	return true;
#else
	return false;
#endif
}

bool MisstortionAudioProcessor::isMidiEffect() const
{
#if JucePlugin_IsMidiEffect
	return true;
#else
	return false;
#endif
}

double MisstortionAudioProcessor::getTailLengthSeconds() const
{
	return 0.0;
}

int MisstortionAudioProcessor::getNumPrograms()
{
	return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
							// so this should be at least 1, even if you're not really implementing programs.
}

int MisstortionAudioProcessor::getCurrentProgram()
{
	return 0;
}

void MisstortionAudioProcessor::setCurrentProgram(int index)
{
}

const String MisstortionAudioProcessor::getProgramName(int index)
{
	return {};
}

void MisstortionAudioProcessor::changeProgramName(int index, const String& newName)
{
}

//==============================================================================
void MisstortionAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
	dsp::ProcessSpec dspSpec;
	dspSpec.sampleRate = sampleRate;
	dspSpec.maximumBlockSize = samplesPerBlock;
	dspSpec.numChannels = getTotalNumOutputChannels();

	m_filterHP.prepare(dspSpec);
	m_filterLP.prepare(dspSpec);
}

void MisstortionAudioProcessor::releaseResources()
{
	// When playback stops, you can use this as an opportunity to free up any
	// spare memory, etc.
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool MisstortionAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
#if JucePlugin_IsMidiEffect
	ignoreUnused(layouts);
	return true;
#else
	// This is the place where you check if the layout is supported.
	// In this template code we only support mono or stereo.
	if (layouts.getMainOutputChannelSet() != AudioChannelSet::mono()
		&& layouts.getMainOutputChannelSet() != AudioChannelSet::stereo())
		return false;

	// This checks if the input layout matches the output layout
#if ! JucePlugin_IsSynth
	if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
		return false;
#endif

	return true;
#endif
}
#endif

void MisstortionAudioProcessor::processBlock(AudioSampleBuffer& buffer, MidiBuffer& midiMessages)
{
	ScopedNoDenormals noDenormals;
	const int totalNumInputChannels = getTotalNumInputChannels();
	const int totalNumOutputChannels = getTotalNumOutputChannels();
	const int sourceChannels = Min(2, totalNumInputChannels);

	for (int i = totalNumInputChannels; i < totalNumOutputChannels; i++) {
		buffer.clear(i, 0, buffer.getNumSamples());
	}

	double sampleRate = getSampleRate();
	int numSamples = buffer.getNumSamples();

	if (sourceChannels > 0 && numSamples > 0) {
		float peak = 0.0f;
		double sumSquares = 0.0;
		double sumAbs = 0.0;
		double sumDelta = 0.0;
		int countedSamples = 0;

		for (int channel = 0; channel < sourceChannels; ++channel) {
			const float* channelData = buffer.getReadPointer(channel);
			float previous = channelData[0];

			for (int i = 0; i < numSamples; i++) {
				const float sample = channelData[i];
				const float absSample = std::abs(sample);

				peak = jmax(peak, absSample);
				sumSquares += sample * sample;
				sumAbs += absSample;

				if (i > 0) {
					sumDelta += std::abs(sample - previous);
				}

				previous = sample;
				countedSamples++;
			}
		}

		if (countedSamples > 0) {
			const float rms = std::sqrt((float)(sumSquares / countedSamples));
			const float activity = jlimit(0.0f, 1.0f, (float)(sumAbs / countedSamples) * 4.0f);
			const float brightness = jlimit(0.0f, 1.0f, (float)(sumDelta / countedSamples) * 12.0f);
			const float smoothing = 0.08f;

			m_signalRms.store(Lerp(m_signalRms.load(), rms, smoothing));
			m_signalPeak.store(Lerp(m_signalPeak.load(), peak, smoothing));
			m_signalActivity.store(Lerp(m_signalActivity.load(), activity, smoothing));
			m_signalBrightness.store(Lerp(m_signalBrightness.load(), brightness, smoothing));
		}
	}

	float mix = (*m_paramMix / 100.0f);
	float gainIn = Decibels::decibelsToGain((float)*m_paramGainIn, -50.0f);
	float gainOut = Decibels::decibelsToGain((float)*m_paramGainOut, -50.0f);

	float driveHard = Decibels::decibelsToGain((float)*m_paramDriveHard);
	float driveSoftDb = (float)*m_paramDriveSoft;
	float driveSoft = Decibels::decibelsToGain(driveSoftDb);
	int toneHP = *m_paramToneHP;
	int toneLP = *m_paramToneLP;
	float symmetry = (*m_paramSymmetry / 100.0f);

	int filterMode = *m_paramFilterMode;
	if (filterMode == 0) {
		// Legacy (1.2 stock, very steep)
		double qo = pow(2.0, 6.0);
		double q = sqrt(qo) / (qo - 1);

		if (toneHP > 0) {
			m_filtersHPLegacy[0].setCoefficients(IIRCoefficients::makeHighPass(sampleRate, (double)toneHP, q));
			m_filtersHPLegacy[1].setCoefficients(IIRCoefficients::makeHighPass(sampleRate, (double)toneHP, q));
		}

		m_filtersLPLegacy[0].setCoefficients(IIRCoefficients::makeLowPass(sampleRate, (double)toneLP, q));
		m_filtersLPLegacy[1].setCoefficients(IIRCoefficients::makeLowPass(sampleRate, (double)toneLP, q));
	}

	// Make a temporary buffer for the final mix
	AudioSampleBuffer processBuffer;
	processBuffer.makeCopyOf(buffer);

	dsp::AudioBlock<float> dspBlock(processBuffer);
	dsp::ProcessContextReplacing<float> dspContext(dspBlock);

	// Apply input gain
	processBuffer.applyGain(gainIn);

	// Apply tone filter (6db/oct or 12db/oct high pass filter)
	if (filterMode != 0 && toneHP > 0) {
		// filterMode as order means 1st order = 6db/oct, 2nd order = 12db/oct
		auto coeff = dsp::FilterDesign<float>::designIIRHighpassHighOrderButterworthMethod((float)toneHP, sampleRate, filterMode);
		m_filterHP.state->coefficients = coeff[0]->coefficients;
		m_filterHP.process(dspContext);
	}

	//TODO: Turn this into a DSP processor?
	for (int channel = 0; channel < Min(2, totalNumInputChannels); ++channel) {
		float* channelData = processBuffer.getWritePointer(channel);

		for (int i = 0; i < numSamples; i++) {
			float &sample = channelData[i];

			// Legacy tone filter (from 1.2, weird Q value on a 2nd order filter)
			if (filterMode == 0 && toneHP > 0) {
				sample = m_filtersHPLegacy[channel].processSingleSampleRaw(sample);
			}

			// Hard clip distortion
			if (driveHard > 1.0f) {
				sample = Clamp(-1.0f, 1.0f, sample * driveHard);
			}

			// Hyperbolic soft clip distortion
			if (driveSoft > 1.0f) {
				sample = atanf(sample * driveSoft);
				sample = Clamp(-1.0f + symmetry, symmetry, sample);
			}

			// Apply symmetry
			if (sample < 0.0f) {
				sample *= (1.0f - symmetry);
			} else {
				sample *= symmetry;
			}

			// Legacy clip filter (from 1.2, weird Q value on a 2nd order filter)
			if (filterMode == 0) {
				sample = m_filtersLPLegacy[channel].processSingleSampleRaw(sample);
			}
		}
	}

	// Apply clip filter (6db/oct or 12db/oct low pass filter)
	if (filterMode != 0) {
		// filterMode as order means 1st order = 6db/oct, 2nd order = 12db/oct
		auto coeff = dsp::FilterDesign<float>::designIIRLowpassHighOrderButterworthMethod((float)toneLP, sampleRate, filterMode);
		m_filterLP.state->coefficients = coeff[0]->coefficients;
		m_filterLP.process(dspContext);
	}

	// Apply mix from the buffer
	for (int channel = 0; channel < Min(2, totalNumInputChannels); ++channel) {
		float* channelData = buffer.getWritePointer(channel);
		const float* processedData = processBuffer.getReadPointer(channel);

		for (int i = 0; i < numSamples; i++) {
			channelData[i] = Lerp(channelData[i], processedData[i], mix);
		}
	}

	// And finally, apply output gain
	buffer.applyGain(gainOut);
}

void MisstortionAudioProcessor::applyGenreSettings(const String& genre)
{
	m_genreText = genre.trim().isNotEmpty() ? genre.trim() : "hard style";
	const String cleanGenre = m_genreText.toLowerCase();

	float intensity = jlimit(0.0f, 1.0f, (m_signalRms.load() * 3.0f) + (m_signalPeak.load() * 0.7f) + (m_signalActivity.load() * 0.35f));
	float brightness = jlimit(0.0f, 1.0f, m_signalBrightness.load());

	float mix = 58.0f + intensity * 24.0f;
	float gainIn = -4.0f + intensity * 8.0f;
	float gainOut = -5.0f - intensity * 5.0f;
	float driveHard = 10.0f + intensity * 18.0f;
	float driveSoft = 14.0f + intensity * 16.0f;
	int toneHP = 24 + (int)(brightness * 80.0f);
	int toneLP = 9800 + (int)(brightness * 5200.0f);
	float symmetry = 48.0f + intensity * 8.0f;
	int filterMode = 2;

	if (cleanGenre.contains("hardstyle") || cleanGenre.contains("hard style")) {
		mix = 66.0f + intensity * 24.0f;
		gainIn = -2.0f + intensity * 9.0f;
		gainOut = -7.0f - intensity * 6.0f;
		driveHard = 18.0f + intensity * 20.0f;
		driveSoft = 21.0f + intensity * 18.0f;
		toneHP = 35 + (int)(brightness * 115.0f);
		toneLP = 8400 + (int)(brightness * 4600.0f);
		symmetry = 52.0f + intensity * 10.0f;
		filterMode = 2;
	} else if (cleanGenre.contains("techno")) {
		mix = 58.0f + intensity * 18.0f;
		driveHard = 12.0f + intensity * 18.0f;
		driveSoft = 18.0f + intensity * 14.0f;
		toneHP = 55 + (int)(brightness * 140.0f);
		toneLP = 7800 + (int)(brightness * 4200.0f);
		symmetry = 50.0f;
	} else if (cleanGenre.contains("trap") || cleanGenre.contains("hip hop")) {
		mix = 48.0f + intensity * 18.0f;
		gainOut = -4.0f - intensity * 4.0f;
		driveHard = 7.0f + intensity * 14.0f;
		driveSoft = 16.0f + intensity * 18.0f;
		toneHP = 20 + (int)(brightness * 60.0f);
		toneLP = 10500 + (int)(brightness * 5200.0f);
		symmetry = 46.0f + intensity * 6.0f;
		filterMode = 1;
	}

	*m_paramMix = jlimit(0.0f, 100.0f, mix);
	*m_paramGainIn = jlimit(-50.0f, 50.0f, gainIn);
	*m_paramGainOut = jlimit(-50.0f, 50.0f, gainOut);
	*m_paramDriveHard = jlimit(0.0f, 50.0f, driveHard);
	*m_paramDriveSoft = jlimit(0.0f, 50.0f, driveSoft);
	*m_paramToneHP = jlimit(0, 20000, toneHP);
	*m_paramToneLP = jlimit(1, 20000, toneLP);
	*m_paramSymmetry = jlimit(0.0f, 100.0f, symmetry);
	*m_paramFilterMode = jlimit(0, 2, filterMode);
}

//==============================================================================
bool MisstortionAudioProcessor::hasEditor() const
{
	return true; // (change this to false if you choose to not supply an editor)
}

AudioProcessorEditor* MisstortionAudioProcessor::createEditor()
{
	return new MisstortionAudioProcessorEditor(*this);
}

//==============================================================================
void MisstortionAudioProcessor::getStateInformation(MemoryBlock& destData)
{
	auto xml = std::make_unique<XmlElement>("root");

	XmlElement* xmlVersion = new XmlElement("version");
	xmlVersion->addTextElement(JucePlugin_VersionString);
	xml->addChildElement(xmlVersion);

	XmlElement* xmlSettings = new XmlElement("settings");
	xmlSettings->setAttribute("mix", *m_paramMix);
	xmlSettings->setAttribute("gainin", *m_paramGainIn);
	xmlSettings->setAttribute("gainout", *m_paramGainOut);

	xmlSettings->setAttribute("drive", *m_paramDriveHard);
	xmlSettings->setAttribute("drive2", *m_paramDriveSoft);
	xmlSettings->setAttribute("tone", *m_paramToneHP);
	xmlSettings->setAttribute("tonepost", *m_paramToneLP);
	xmlSettings->setAttribute("symmetry", *m_paramSymmetry);
	xmlSettings->setAttribute("filtermode", *m_paramFilterMode);
	xmlSettings->setAttribute("genre", m_genreText);
	xml->addChildElement(xmlSettings);

	copyXmlToBinary(*xml, destData);
}

void MisstortionAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
	std::unique_ptr<XmlElement> xml = getXmlFromBinary(data, sizeInBytes);

	XmlElement* xmlSettings = xml->getChildByName("settings");
	if (xmlSettings != nullptr) {
		*m_paramMix = (float)xmlSettings->getDoubleAttribute("mix");
		*m_paramGainIn = (float)xmlSettings->getDoubleAttribute("gainin");
		*m_paramGainOut = (float)xmlSettings->getDoubleAttribute("gainout");

		// From previous version, if it's set
		double oldGainOut = xmlSettings->getDoubleAttribute("gain", -1.0f);
		if (oldGainOut >= 0.0f) {
			*m_paramGainIn = 0.0f;
			*m_paramGainOut = (float)oldGainOut;
		}

		*m_paramDriveHard = (float)xmlSettings->getDoubleAttribute("drive");
		*m_paramDriveSoft = (float)xmlSettings->getDoubleAttribute("drive2");
		*m_paramToneHP = xmlSettings->getIntAttribute("tone");
		*m_paramToneLP = xmlSettings->getIntAttribute("tonepost");
		*m_paramSymmetry = (float)xmlSettings->getDoubleAttribute("symmetry");
		*m_paramFilterMode = (int)xmlSettings->getIntAttribute("filtermode");
		m_genreText = xmlSettings->getStringAttribute("genre", "hard style");
	}

#if defined(_DEBUG)
	Logger::writeToLog(String::formatted("  Mix: %f", (float)*m_paramMix));
	Logger::writeToLog(String::formatted("  Gain In: %f", (float)*m_paramGainIn));
	Logger::writeToLog(String::formatted("  Gain Out: %f", (float)*m_paramGainOut));
	Logger::writeToLog(String::formatted("  Drive: %f", (float)*m_paramDriveHard));
	Logger::writeToLog(String::formatted("  Drive2: %f", (float)*m_paramDriveSoft));
	Logger::writeToLog(String::formatted("  Tone: %d", (int)*m_paramToneHP));
	Logger::writeToLog(String::formatted("  TonePost: %d", (int)*m_paramToneLP));
	Logger::writeToLog(String::formatted("  Symmetry: %f", (float)*m_paramSymmetry));
	Logger::writeToLog(String::formatted("  Filter Mode: %d", (int)*m_paramFilterMode));
#endif
}

//==============================================================================
// This creates new instances of the plugin..
AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
	return new MisstortionAudioProcessor();
}
