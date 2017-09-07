/* FACT - XACT Reimplementation for FNA
 * Copyright 2009-2017 Ethan Lee and the MonoGame Team
 *
 * Released under the Microsoft Public License.
 * See LICENSE for details.
 */

#include "FACT_internal.h"

uint32_t FACTCreateAudioEngine(
	uint32_t dwCreationFlags,
	FACTAudioEngine **ppEngine
) {
	*ppEngine = (FACTAudioEngine*) FACT_malloc(sizeof(FACTAudioEngine));
	if (*ppEngine == NULL)
	{
		return -1; /* TODO: E_OUTOFMEMORY */
	}
	FACT_zero(*ppEngine, sizeof(FACTAudioEngine));
	return 0;
}

uint32_t FACTAudioEngine_GetRendererCount(
	FACTAudioEngine *pEngine,
	uint16_t *pnRendererCount
) {
	/* TODO */
	return 0;
}

uint32_t FACTAudioEngine_GetRendererDetails(
	FACTAudioEngine *pEngine,
	uint16_t nRendererIndex,
	FACTRendererDetails *pRendererDetails
) {
	/* TODO */
	return 0;
}

uint32_t FACTAudioEngine_GetFinalMixFormat(
	FACTAudioEngine *pEngine,
	FACTWaveFormatExtensible *pFinalMixFormat
) {
	/* TODO */
	return 0;
}

uint32_t FACTAudioEngine_Initialize(
	FACTAudioEngine *pEngine,
	const FACTRuntimeParameters *pParams
) {
	uint32_t	categoryOffset,
			variableOffset,
			blob1Offset,
			categoryNameIndexOffset,
			blob2Offset,
			variableNameIndexOffset,
			categoryNameOffset,
			variableNameOffset,
			rpcOffset,
			dspPresetOffset,
			dspParameterOffset;
	uint16_t blob1Count, blob2Count;
	size_t memsize;
	uint16_t i, j;

	uint8_t *ptr = (uint8_t*) pParams->pGlobalSettingsBuffer;
	uint8_t *start = ptr;

	if (	read_u32(&ptr) != 0x46534758 /* 'XGSF' */ ||
		read_u16(&ptr) != FACT_CONTENT_VERSION ||
		read_u16(&ptr) != 42 /* Tool version */	)
	{
		return -1; /* TODO: NOT XACT FILE */
	}

	ptr += 2; /* Unknown value */

	/* Last modified, unused */
	ptr += 8;

	/* XACT Version */
	if (read_u8(&ptr) != 3)
	{
		return -1; /* TODO: VERSION TOO OLD */
	}

	/* Object counts */
	pEngine->categoryCount = read_u16(&ptr);
	pEngine->variableCount = read_u16(&ptr);
	blob1Count = read_u16(&ptr);
	blob2Count = read_u16(&ptr);
	pEngine->rpcCount = read_u16(&ptr);
	pEngine->dspPresetCount = read_u16(&ptr);
	pEngine->dspParameterCount = read_u16(&ptr);

	/* Object offsets */
	categoryOffset = read_u32(&ptr);
	variableOffset = read_u32(&ptr);
	blob1Offset = read_u32(&ptr);
	categoryNameIndexOffset = read_u32(&ptr);
	blob2Offset = read_u32(&ptr);
	variableNameIndexOffset = read_u32(&ptr);
	categoryNameOffset = read_u32(&ptr);
	variableNameOffset = read_u32(&ptr);
	rpcOffset = read_u32(&ptr);
	dspPresetOffset = read_u32(&ptr);
	dspParameterOffset = read_u32(&ptr);

	/* Category data */
	assert((ptr - start) == categoryOffset);
	memsize = sizeof(FACTAudioCategory) * pEngine->categoryCount;
	pEngine->categories = (FACTAudioCategory*) FACT_malloc(memsize);
	FACT_memcpy(pEngine->categories, ptr, memsize);
	ptr += memsize;

	/* Variable data */
	assert((ptr - start) == variableOffset);
	memsize = sizeof(FACTVariable) * pEngine->variableCount;
	pEngine->variables = (FACTVariable*) FACT_malloc(memsize);
	FACT_memcpy(pEngine->variables, ptr, memsize);
	ptr += memsize;

	/* Global variable storage. Some unused data for non-global vars */
	memsize = sizeof(float) * pEngine->variableCount;
	pEngine->globalVariableValues = (float*) FACT_malloc(memsize);
	for (i = 0; i < pEngine->variableCount; i += 1)
	{
		pEngine->globalVariableValues[i] = pEngine->variables[i].initialValue;
	}

	/* RPC data */
	assert((ptr - start) == rpcOffset);
	pEngine->rpcs = (FACTRPC*) FACT_malloc(
		sizeof(FACTRPC) *
		pEngine->rpcCount
	);
	pEngine->rpcCodes = (size_t*) FACT_malloc(
		sizeof(size_t) *
		pEngine->rpcCount
	);
	for (i = 0; i < pEngine->rpcCount; i += 1)
	{
		pEngine->rpcCodes[i] = ptr - start;
		pEngine->rpcs[i].variable = read_u16(&ptr);
		pEngine->rpcs[i].pointCount = read_u8(&ptr);
		pEngine->rpcs[i].parameter = read_u16(&ptr);
		pEngine->rpcs[i].points = (FACTRPCPoint*) FACT_malloc(
			sizeof(FACTRPCPoint) *
			pEngine->rpcs[i].pointCount
		);
		for (j = 0; j < pEngine->rpcs[i].pointCount; j += 1)
		{
			pEngine->rpcs[i].points[j].x = read_f32(&ptr);
			pEngine->rpcs[i].points[j].y = read_f32(&ptr);
			pEngine->rpcs[i].points[j].type = read_u8(&ptr);
		}
	}

	/* DSP Preset data */
	assert((ptr - start) == dspPresetOffset);
	pEngine->dspPresets = (FACTDSPPreset*) FACT_malloc(
		sizeof(FACTDSPPreset) *
		pEngine->dspPresetCount
	);
	pEngine->dspPresetCodes = (size_t*) FACT_malloc(
		sizeof(size_t) *
		pEngine->dspPresetCount
	);
	for (i = 0; i < pEngine->dspPresetCount; i += 1)
	{
		pEngine->dspPresetCodes[i] = ptr - start;
		pEngine->dspPresets[i].accessibility = read_u8(&ptr);
		pEngine->dspPresets[i].parameterCount = read_u32(&ptr);
		pEngine->dspPresets[i].parameters = (FACTDSPParameter*) FACT_malloc(
			sizeof(FACTDSPParameter) *
			pEngine->dspPresets[i].parameterCount
		); /* This will be filled in just a moment... */
	}

	/* DSP Parameter data */
	assert((ptr - start) == dspParameterOffset);
	for (i = 0; i < pEngine->dspPresetCount; i += 1)
	{
		memsize = (
			sizeof(FACTDSPParameter) *
			pEngine->dspPresets[i].parameterCount
		);
		FACT_memcpy(
			pEngine->dspPresets[i].parameters,
			ptr,
			memsize
		);
		ptr += memsize;
	}

	/* Blob #1, no idea what this is... */
	assert((ptr - start) == blob1Offset);
	ptr += blob1Count * 2;

	/* Category Name Index data */
	assert((ptr - start) == categoryNameIndexOffset);
	ptr += pEngine->categoryCount * 6; /* FIXME: index as assert value? */

	/* Category Name data */
	assert((ptr - start) == categoryNameOffset);
	pEngine->categoryNames = (char**) FACT_malloc(
		sizeof(char*) *
		pEngine->categoryCount
	);
	for (i = 0; i < pEngine->categoryCount; i += 1)
	{
		memsize = FACT_strlen((char*) ptr) + 1; /* Dastardly! */
		pEngine->categoryNames[i] = (char*) FACT_malloc(memsize);
		FACT_memcpy(pEngine->categoryNames[i], ptr, memsize);
		ptr += memsize;
	}

	/* Blob #2, no idea what this is... */
	assert((ptr - start) == blob2Offset);
	ptr += blob2Count * 2;

	/* Variable Name Index data */
	assert((ptr - start) == variableNameIndexOffset);
	ptr += pEngine->variableCount * 6; /* FIXME: index as assert value? */

	/* Variable Name data */
	assert((ptr - start) == variableNameOffset);
	pEngine->variableNames = (char**) FACT_malloc(
		sizeof(char*) *
		pEngine->variableCount
	);
	for (i = 0; i < pEngine->variableCount; i += 1)
	{
		memsize = FACT_strlen((char*) ptr) + 1; /* Dastardly! */
		pEngine->variableNames[i] = (char*) FACT_malloc(memsize);
		FACT_memcpy(pEngine->variableNames[i], ptr, memsize);
		ptr += memsize;
	}

	/* Finally. */
	assert((ptr - start) == pParams->globalSettingsBufferSize);
	return 0;
}

uint32_t FACTAudioEngine_Shutdown(FACTAudioEngine *pEngine)
{
	int i;

	/* Category data */
	for (i = 0; i < pEngine->categoryCount; i += 1)
	{
		FACT_free(pEngine->categoryNames[i]);
	}
	FACT_free(pEngine->categoryNames);
	FACT_free(pEngine->categories);

	/* Variable data */
	for (i = 0; i < pEngine->variableCount; i += 1)
	{
		FACT_free(pEngine->variableNames[i]);
	}
	FACT_free(pEngine->variableNames);
	FACT_free(pEngine->variables);

	/* RPC data */
	for (i = 0; i < pEngine->rpcCount; i += 1)
	{
		FACT_free(pEngine->rpcs[i].points);
	}
	FACT_free(pEngine->rpcs);
	FACT_free(pEngine->rpcCodes);

	/* DSP data */
	for (i = 0; i < pEngine->dspPresetCount; i += 1)
	{
		FACT_free(pEngine->dspPresets[i].parameters);
	}
	FACT_free(pEngine->dspPresets);
	FACT_free(pEngine->dspPresetCodes);

	/* Finally. */
	FACT_free(pEngine);
	return 0;
}

uint32_t FACTAudioEngine_DoWork(FACTAudioEngine *pEngine)
{
	/* TODO */
	return 0;
}

void INTERNAL_FACTParseClip(uint8_t **ptr, FACTClip *clip)
{
	uint32_t evtInfo;
	uint8_t minWeight, maxWeight;
	int i, j;

	clip->volume = read_u8(ptr);

	/* Clip offset, unused */
	*ptr += 4;

	clip->filter = read_u8(ptr);
	if (clip->filter & 0x01)
	{
		clip->filter = (clip->filter >> 1) & 0x02;
	}
	else
	{
		/* Huh...? */
		clip->filter = 0xFF;
	}

	clip->qfactor = read_u8(ptr);
	clip->frequency = read_u16(ptr);

	clip->eventCount = read_u8(ptr);
	clip->events = (FACTEvent*) FACT_malloc(
		sizeof(FACTEvent) *
		clip->eventCount
	);
	for (i = 0; i < clip->eventCount; i += 1)
	{
		evtInfo = read_u32(ptr);
		clip->events[i].randomOffset = read_u16(ptr);

		clip->events[i].type = evtInfo & 0x001F;
		clip->events[i].timestamp = (evtInfo >> 5) & 0xFFFF;

		assert(read_u8(ptr) == 0xFF); /* Separator? */

		#define CLIPTYPE(t) (clip->events[i].type == t)
		if (CLIPTYPE(FACTEVENT_STOP))
		{
			clip->events[i].stop.flags = read_u8(ptr);
		}
		else if (CLIPTYPE(FACTEVENT_PLAYWAVE))
		{
			/* Basic Wave */
			clip->events[i].wave.isComplex = 0;
			clip->events[i].wave.flags = read_u8(ptr);
			clip->events[i].wave.simple.track = read_u16(ptr);
			clip->events[i].wave.simple.wavebank = read_u16(ptr);
			clip->events[i].loopCount = read_u8(ptr);
			clip->events[i].wave.position = read_u16(ptr);
			clip->events[i].wave.angle = read_u16(ptr);

			/* No Variation */
			clip->events[i].wave.variationFlags = 0;
		}
		else if (CLIPTYPE(FACTEVENT_PLAYWAVETRACKVARIATION))
		{
			/* Complex Wave */
			clip->events[i].wave.isComplex = 1;
			clip->events[i].wave.flags = read_u8(ptr);
			clip->events[i].loopCount = read_u8(ptr);
			clip->events[i].wave.position = read_u16(ptr);
			clip->events[i].wave.angle = read_u16(ptr);

			/* Track Variation */
			clip->events[i].wave.complex.trackCount = read_u16(ptr);
			clip->events[i].wave.complex.variation = read_u16(ptr);
			*ptr += 4; /* Unknown values */
			clip->events[i].wave.complex.tracks = (uint16_t*) FACT_malloc(
				sizeof(uint16_t) *
				clip->events[i].wave.complex.trackCount
			);
			clip->events[i].wave.complex.wavebanks = (uint8_t*) FACT_malloc(
				sizeof(uint8_t) *
				clip->events[i].wave.complex.trackCount
			);
			clip->events[i].wave.complex.weights = (uint8_t*) FACT_malloc(
				sizeof(uint8_t) *
				clip->events[i].wave.complex.trackCount
			);
			for (j = 0; j < clip->events[i].wave.complex.trackCount; j += 1)
			{
				clip->events[i].wave.complex.tracks[j] = read_u16(ptr);
				clip->events[i].wave.complex.wavebanks[j] = read_u8(ptr);
				minWeight = read_u8(ptr);
				maxWeight = read_u8(ptr);
				clip->events[i].wave.complex.weights[j] = (
					maxWeight - minWeight
				);
			}
		}
		else if (CLIPTYPE(FACTEVENT_PLAYWAVEEFFECTVARIATION))
		{
			/* Basic Wave */
			clip->events[i].wave.isComplex = 0;
			clip->events[i].wave.flags = read_u8(ptr);
			clip->events[i].wave.simple.track = read_u16(ptr);
			clip->events[i].wave.simple.wavebank = read_u16(ptr);
			clip->events[i].loopCount = read_u8(ptr);
			clip->events[i].wave.position = read_u16(ptr);
			clip->events[i].wave.angle = read_u16(ptr);

			/* Effect Variation */
			clip->events[i].wave.minPitch = read_s16(ptr);
			clip->events[i].wave.maxPitch = read_s16(ptr);
			clip->events[i].wave.minVolume = read_u8(ptr);
			clip->events[i].wave.maxVolume = read_u8(ptr);
			clip->events[i].wave.minFrequency = read_f32(ptr);
			clip->events[i].wave.maxFrequency = read_f32(ptr);
			clip->events[i].wave.minQFactor = read_f32(ptr);
			clip->events[i].wave.maxQFactor = read_f32(ptr);
			clip->events[i].wave.variationFlags = read_u16(ptr);
		}
		else if (CLIPTYPE(FACTEVENT_PLAYWAVETRACKEFFECTVARIATION))
		{
			/* Complex Wave */
			clip->events[i].wave.isComplex = 1;
			clip->events[i].wave.flags = read_u8(ptr);
			clip->events[i].loopCount = read_u8(ptr);
			clip->events[i].wave.position = read_u16(ptr);
			clip->events[i].wave.angle = read_u16(ptr);

			/* Effect Variation */
			clip->events[i].wave.minPitch = read_s16(ptr);
			clip->events[i].wave.maxPitch = read_s16(ptr);
			clip->events[i].wave.minVolume = read_u8(ptr);
			clip->events[i].wave.maxVolume = read_u8(ptr);
			clip->events[i].wave.minFrequency = read_f32(ptr);
			clip->events[i].wave.maxFrequency = read_f32(ptr);
			clip->events[i].wave.minQFactor = read_f32(ptr);
			clip->events[i].wave.maxQFactor = read_f32(ptr);
			clip->events[i].wave.variationFlags = read_u16(ptr);

			/* Track Variation */
			clip->events[i].wave.complex.trackCount = read_u16(ptr);
			clip->events[i].wave.complex.variation = read_u16(ptr);
			*ptr += 4; /* Unknown values */
			clip->events[i].wave.complex.tracks = (uint16_t*) FACT_malloc(
				sizeof(uint16_t) *
				clip->events[i].wave.complex.trackCount
			);
			clip->events[i].wave.complex.wavebanks = (uint8_t*) FACT_malloc(
				sizeof(uint8_t) *
				clip->events[i].wave.complex.trackCount
			);
			clip->events[i].wave.complex.weights = (uint8_t*) FACT_malloc(
				sizeof(uint8_t) *
				clip->events[i].wave.complex.trackCount
			);
			for (j = 0; j < clip->events[i].wave.complex.trackCount; j += 1)
			{
				clip->events[i].wave.complex.tracks[j] = read_u16(ptr);
				clip->events[i].wave.complex.wavebanks[j] = read_u8(ptr);
				minWeight = read_u8(ptr);
				maxWeight = read_u8(ptr);
				clip->events[i].wave.complex.weights[j] = (
					maxWeight - minWeight
				);
			}
		}
		else if (	CLIPTYPE(FACTEVENT_PITCH) ||
				CLIPTYPE(FACTEVENT_VOLUME) ||
				CLIPTYPE(FACTEVENT_PITCHREPEATING) ||
				CLIPTYPE(FACTEVENT_VOLUMEREPEATING)	)
		{
			clip->events[i].value.settings = read_u8(ptr);
			if (clip->events[i].value.settings & 1) /* Ramp */
			{
				clip->events[i].loopCount = 0;
				clip->events[i].value.ramp.initialValue = read_f32(ptr);
				clip->events[i].value.ramp.initialSlope = read_f32(ptr);
				clip->events[i].value.ramp.slopeDelta = read_f32(ptr);
				clip->events[i].value.ramp.duration = read_u16(ptr);
			}
			else /* Equation */
			{
				clip->events[i].value.equation.flags = read_u8(ptr);

				/* SetValue, SetRandomValue, anything else? */
				assert(clip->events[i].value.equation.flags & 0x0C);

				clip->events[i].value.equation.value1 = read_f32(ptr);
				clip->events[i].value.equation.value2 = read_f32(ptr);

				*ptr += 5; /* Unknown values */

				if (	CLIPTYPE(FACTEVENT_PITCHREPEATING) ||
					CLIPTYPE(FACTEVENT_VOLUMEREPEATING)	)
				{
					clip->events[i].loopCount = read_u16(ptr);
					clip->events[i].frequency = read_u16(ptr);
				}
				else
				{
					clip->events[i].loopCount = 0;
				}
			}
		}
		else if (CLIPTYPE(FACTEVENT_MARKER))
		{
			clip->events[i].marker.marker = read_u32(ptr);
			clip->events[i].loopCount = read_u16(ptr);
			clip->events[i].frequency = read_u16(ptr);
			clip->events[i].marker.repeating = 0;
		}
		else if (CLIPTYPE(FACTEVENT_MARKERREPEATING))
		{
			clip->events[i].marker.marker = read_u32(ptr);
			clip->events[i].loopCount = read_u16(ptr);
			clip->events[i].frequency = read_u16(ptr);
			clip->events[i].marker.repeating = 1;
		}
		else
		{
			assert(0 && "Unknown event type!");
		}
		#undef CLIPTYPE
	}
}

uint32_t FACTAudioEngine_CreateSoundBank(
	FACTAudioEngine *pEngine,
	const void *pvBuffer,
	uint32_t dwSize,
	uint32_t dwFlags,
	uint32_t dwAllocAttributes,
	FACTSoundBank **ppSoundBank
) {
	FACTSoundBank *sb;
	uint16_t	cueSimpleCount,
			cueComplexCount;
	uint32_t	cueSimpleOffset,
			cueComplexOffset,
			cueNameOffset,
			variationOffset,
			wavebankNameOffset,
			cueHashOffset,
			cueNameIndexOffset,
			soundOffset;
	size_t memsize;
	uint16_t i, j, cur;
	uint8_t *ptrBookmark;

	uint8_t *ptr = (uint8_t*) pvBuffer;
	uint8_t *start = ptr;

	if (	read_u32(&ptr) != 0x4B424453 /* 'SDBK' */ ||
		read_u16(&ptr) != FACT_CONTENT_VERSION ||
		read_u16(&ptr) != 43 /* Tool version */	)
	{
		return -1; /* TODO: NOT XACT FILE */
	}

	/* CRC, unused */
	ptr += 2;

	/* Last modified, unused */
	ptr += 8;

	/* Windows == 1, Xbox == 0 (I think?) */
	if (read_u32(&ptr) != 1)
	{
		return -1; /* TODO: WRONG PLATFORM */
	}

	sb = (FACTSoundBank*) FACT_malloc(sizeof(FACTSoundBank));

	cueSimpleCount = read_u16(&ptr);
	cueComplexCount = read_u16(&ptr);

	ptr += 2; /* Unknown value */

	sb->cueCount = read_u16(&ptr);
	sb->wavebankCount = read_u8(&ptr);
	sb->soundCount = read_u16(&ptr);

	/* Cue name length, unused */
	ptr += 2;

	ptr += 2; /* Unknown value */

	cueSimpleOffset = read_u32(&ptr);
	cueComplexOffset = read_u32(&ptr);
	cueNameOffset = read_u32(&ptr);

	ptr += 4; /* Unknown value */

	variationOffset = read_u32(&ptr);

	ptr += 4; /* Unknown value */

	wavebankNameOffset = read_u32(&ptr);
	cueHashOffset = read_u32(&ptr);
	cueNameIndexOffset = read_u32(&ptr);
	soundOffset = read_u32(&ptr);

	/* SoundBank Name */
	memsize = FACT_strlen((char*) ptr) + 1; /* Dastardly! */
	sb->name = (char*) FACT_malloc(memsize);
	FACT_memcpy(sb->name, ptr, memsize);
	ptr += 64;

	/* WaveBank Name data */
	assert((ptr - start) == wavebankNameOffset);
	sb->wavebankNames = (char**) FACT_malloc(
		sizeof(char*) *
		sb->wavebankCount
	);
	for (i = 0; i < sb->wavebankCount; i += 1)
	{
		memsize = FACT_strlen((char*) ptr) + 1;
		sb->wavebankNames[i] = (char*) FACT_malloc(memsize);
		FACT_memcpy(sb->wavebankNames[i], ptr, memsize);
		ptr += 64;
	}

	/* Sound data */
	assert((ptr - start) == soundOffset);
	sb->sounds = (FACTSound*) FACT_malloc(
		sizeof(FACTSound) *
		sb->soundCount
	);
	sb->soundCodes = (size_t*) FACT_malloc(
		sizeof(size_t) *
		sb->soundCount
	);
	for (i = 0; i < sb->soundCount; i += 1)
	{
		sb->soundCodes[i] = ptr - start;
		sb->sounds[i].flags = read_u8(&ptr);
		sb->sounds[i].category = read_u16(&ptr);
		sb->sounds[i].volume = read_u8(&ptr);
		sb->sounds[i].pitch = read_s16(&ptr);

		ptr += 1; /* Unknown value */

		/* Length of sound entry, unused */
		ptr += 2;

		/* Simple/Complex Clip data */
		if (sb->sounds[i].flags & 0x01)
		{
			sb->sounds[i].clipCount = read_u8(&ptr);
			sb->sounds[i].clips = (FACTClip*) FACT_malloc(
				sizeof(FACTClip) *
				sb->sounds[i].clipCount
			);
		}
		else
		{
			sb->sounds[i].clipCount = 1;
			sb->sounds[i].clips = (FACTClip*) FACT_malloc(
				sizeof(FACTClip)
			);
			sb->sounds[i].clips[0].volume = FACT_VOLUME_0;
			sb->sounds[i].clips[0].filter = 0xFF;
			sb->sounds[i].clips[0].eventCount = 1;
			sb->sounds[i].clips[0].events = (FACTEvent*) FACT_malloc(
				sizeof(FACTEvent)
			);
			sb->sounds[i].clips[0].events[0].type = FACTEVENT_PLAYWAVE;
			sb->sounds[i].clips[0].events[0].timestamp = 0;
			sb->sounds[i].clips[0].events[0].randomOffset = 0;
			sb->sounds[i].clips[0].events[0].loopCount = 0;
			sb->sounds[i].clips[0].events[0].wave.flags = 0;
			sb->sounds[i].clips[0].events[0].wave.position = 0; /* FIXME */
			sb->sounds[i].clips[0].events[0].wave.angle = 0; /* FIXME */
			sb->sounds[i].clips[0].events[0].wave.isComplex = 0;
			sb->sounds[i].clips[0].events[0].wave.simple.track = read_u16(&ptr);
			sb->sounds[i].clips[0].events[0].wave.simple.wavebank = read_u8(&ptr);
		}

		/* RPC Code data */
		cur = 0;
		if (sb->sounds[i].flags & 0x0E)
		{
			const uint16_t rpcDataLength = read_u16(&ptr);
			ptrBookmark = ptr - 2;
			while ((ptr - ptrBookmark) < rpcDataLength)
			{
				const uint8_t codes = read_u8(&ptr);
				sb->sounds[i].rpcCodeCount += codes;
				ptr += 4 * codes;
			}
			sb->sounds[i].rpcCodes = (size_t*) FACT_malloc(
				sizeof(size_t) *
				sb->sounds[i].rpcCodeCount
			);
			ptr = ptrBookmark;
			while ((ptr - ptrBookmark) < rpcDataLength)
			{
				const uint8_t codes = read_u8(&ptr);
				for (j = 0; j < codes; j += 1, cur += 1)
				{
					sb->sounds[i].rpcCodes[j + cur] = read_u32(&ptr);
				}
			}
		}
		else
		{
			sb->sounds[i].rpcCodeCount = 0;
			sb->sounds[i].rpcCodes = NULL;
		}

		/* DSP Preset Code data */
		if (sb->sounds[i].flags & 0x10)
		{
			/* DSP presets length, unused */
			ptr += 2;

			sb->sounds[i].dspCodeCount = read_u8(&ptr);
			sb->sounds[i].dspCodes = (size_t*) FACT_malloc(
				sizeof(size_t) *
				sb->sounds[i].dspCodeCount
			);
			for (j = 0; j < sb->sounds[i].dspCodeCount; j += 1)
			{
				sb->sounds[i].dspCodes[i] = read_u32(&ptr);
			}
		}
		else
		{
			sb->sounds[i].dspCodeCount = 0;
			sb->sounds[i].dspCodes = NULL;
		}

		/* Clip data */
		if (sb->sounds[i].flags & 0x01)
		{
			for (j = 0; j < sb->sounds[i].clipCount; j += 1)
			{
				INTERNAL_FACTParseClip(
					&ptr,
					&sb->sounds[i].clips[j]
				);
			}
		}
	}

	/* All Cue data */
	sb->variationCount = 0;
	sb->cues = (FACTCueData*) FACT_malloc(
		sizeof(FACTCueData) *
		sb->cueCount
	);
	cur = 0;

	/* Simple Cue data */
	assert(cueSimpleCount == 0 || (ptr - start) == cueSimpleOffset);
	for (i = 0; i < cueSimpleCount; i += 1, cur += 1)
	{
		sb->cues[cur].flags = read_u8(&ptr);
		sb->cues[cur].sbCode = read_u32(&ptr);
		sb->cues[cur].transitionOffset = read_u32(&ptr);
		sb->cues[cur].instanceLimit = 0xFF;
		sb->cues[cur].fadeIn = 0;
		sb->cues[cur].fadeOut = 0;
		sb->cues[cur].maxInstanceBehavior = 0;
		sb->cues[cur].instanceCount = 0;
	}

	/* Complex Cue data */
	assert(cueComplexCount == 0 || (ptr - start) == cueComplexOffset);
	for (i = 0; i < cueComplexCount; i += 1, cur += 1)
	{
		sb->cues[cur].flags = read_u8(&ptr);
		sb->cues[cur].sbCode = read_u32(&ptr);
		sb->cues[cur].transitionOffset = read_u32(&ptr);
		sb->cues[cur].instanceLimit = read_u8(&ptr);
		sb->cues[cur].fadeIn = read_u16(&ptr);
		sb->cues[cur].fadeOut = read_u16(&ptr);
		sb->cues[cur].maxInstanceBehavior = read_u8(&ptr);
		sb->cues[cur].instanceCount = 0;

		if (!(sb->cues[cur].flags & 0x04))
		{
			/* FIXME: Is this the only way to get this...? */
			sb->variationCount += 1;
		}
	}

	/* Variation data */
	if (sb->variationCount > 0)
	{
		assert((ptr - start) == variationOffset);
		sb->variations = (FACTVariationTable*) FACT_malloc(
			sizeof(FACTVariationTable) *
			sb->variationCount
		);
		sb->variationCodes = (size_t*) FACT_malloc(
			sizeof(size_t) *
			sb->variationCount
		);
	}
	for (i = 0; i < sb->variationCount; i += 1)
	{
		sb->variationCodes[i] = ptr - start;
		sb->variations[i].entryCount = read_u16(&ptr);
		sb->variations[i].flags = (read_u16(&ptr) >> 3) & 0x07;
		ptr += 2; /* Unknown value */
		sb->variations[i].variable = read_u16(&ptr);

		sb->variations[i].entries = (FACTVariation*) FACT_malloc(
			sizeof(FACTVariation) *
			sb->variations[i].entryCount
		);

		if (sb->variations[i].flags == 0)
		{
			for (j = 0; i < sb->variations[i].entryCount; j += 1)
			{
				sb->variations[i].entries[j].simple.track = read_u16(&ptr);
				sb->variations[i].entries[j].simple.wavebank = read_u8(&ptr);
				sb->variations[i].entries[j].minWeight = read_u8(&ptr) / 255.0f;
				sb->variations[i].entries[j].maxWeight = read_u8(&ptr) / 255.0f;
			}
		}
		else if (sb->variations[i].flags == 1)
		{
			for (j = 0; i < sb->variations[i].entryCount; j += 1)
			{
				sb->variations[i].entries[j].soundCode = read_u32(&ptr);
				sb->variations[i].entries[j].minWeight = read_u8(&ptr) / 255.0f;
				sb->variations[i].entries[j].maxWeight = read_u8(&ptr) / 255.0f;
			}
		}
		if (sb->variations[i].flags == 3)
		{
			for (j = 0; i < sb->variations[i].entryCount; j += 1)
			{
				sb->variations[i].entries[j].soundCode = read_u32(&ptr);
				sb->variations[i].entries[j].minWeight = read_f32(&ptr);
				sb->variations[i].entries[j].maxWeight = read_f32(&ptr);
			}
		}
		if (sb->variations[i].flags == 4)
		{
			for (j = 0; i < sb->variations[i].entryCount; j += 1)
			{
				sb->variations[i].entries[j].simple.track = read_u16(&ptr);
				sb->variations[i].entries[j].simple.wavebank = read_u8(&ptr);
				sb->variations[i].entries[j].minWeight = 0.0f;
				sb->variations[i].entries[j].maxWeight = 1.0f;
			}
		}
		else
		{
			assert(0 && "Unknown variation type!");
		}
	}

	/* Cue Hash data? No idea what this is... */
	assert((ptr - start) == cueHashOffset);
	ptr += 2 * sb->cueCount;

	/* Cue Name Index data */
	assert((ptr - start) == cueNameIndexOffset);
	ptr += 6 * sb->cueCount; /* FIXME: index as assert value? */

	/* Cue Name data */
	assert((ptr - start) == cueNameOffset);
	sb->cueNames = (char**) FACT_malloc(
		sizeof(char*) *
		sb->cueCount
	);
	for (int i = 0; i < sb->cueCount; i += 1)
	{
		memsize = FACT_strlen((char*) ptr) + 1;
		sb->cueNames[i] = (char*) FACT_malloc(memsize);
		FACT_memcpy(sb->cueNames[i], ptr, memsize);
		ptr += memsize;
	}

	/* Finally. */
	assert((ptr - start) == dwSize);
	*ppSoundBank = sb;
	return 0;
}

/* The unxwb project, written by Luigi Auriemma, was released in 2006 under the
 * GNU General Public License, version 2.0:
 *
 * http://www.gnu.org/licenses/gpl-2.0.html
 *
 * While the unxwb project was released under the GPL, Luigi has given express
 * permission to the MonoGame project to use code from unxwb under the MonoGame
 * project license. See LICENSE for details.
 *
 * The unxwb website can be found here:
 *
 * http://aluigi.altervista.org/papers.htm#xbox
 */
uint32_t FACT_ParseWaveBank(
	FACTAudioEngine *pEngine,
	FACTIOStream *io,
	uint16_t isStreaming,
	FACTWaveBank **ppWaveBank
) {
	FACTWaveBank *wb;
	size_t memsize;
	uint32_t i;
	struct
	{
		uint32_t wbnd;
		uint32_t contentversion;
		uint32_t toolversion;
	} header;
	struct
	{
		uint32_t infoOffset;
		uint32_t infoLength;
		uint32_t entryTableOffset;
		uint32_t entryTableLength;
		uint32_t unknownOffset1;
		uint32_t unknownLength1;
		uint32_t unknownOffset2;
		uint32_t unknownLength2;
		uint32_t playRegionOffset;
		uint32_t playRegionLength;
	} wbtable;
	struct
	{
		uint16_t streaming;
		uint16_t flags;
		uint32_t entryCount;
		char name[64];
		uint32_t metadataElementSize;
		uint32_t nameElementSize;
		uint32_t alignment;
		uint32_t compactFormat;
		uint64_t unknown;
	} wbinfo;
	uint32_t compactEntry;

	io->read(io->data, &header, sizeof(header), 1);
	if (	header.wbnd != 0x444E4257 ||
		header.contentversion != FACT_CONTENT_VERSION ||
		header.toolversion != 44	)
	{
		return -1; /* TODO: NOT XACT FILE */
	}

	wb = (FACTWaveBank*) FACT_malloc(sizeof(FACTWaveBank));

	/* Offset Table */
	io->read(io->data, &wbtable, sizeof(wbtable), 1);

	/* TODO: What are these...? */
	assert(wbtable.unknownOffset1 == wbtable.playRegionOffset);
	assert(wbtable.unknownLength1 == 0);
	assert(wbtable.unknownOffset2 == 0);
	assert(wbtable.unknownLength2 == 0);

	/* WaveBank Info */
	assert(io->seek(io->data, 0, 1) == wbtable.infoOffset);
	io->read(io->data, &wbinfo, sizeof(wbinfo), 1);
	assert(wbinfo.streaming == isStreaming);
	wb->streaming = wbinfo.streaming;
	wb->entryCount = wbinfo.entryCount;
	memsize = FACT_strlen(wbinfo.name) + 1;
	wb->name = (char*) FACT_malloc(memsize);
	FACT_memcpy(wb->name, wbinfo.name, memsize);
	memsize = sizeof(FACTWaveBankEntry) * wbinfo.entryCount;
	wb->entries = (FACTWaveBankEntry*) FACT_malloc(memsize);
	FACT_zero(wb->entries, memsize);
	memsize = sizeof(uint32_t) * wbinfo.entryCount;
	wb->entryRefs = (uint32_t*) FACT_malloc(memsize);
	FACT_zero(wb->entryRefs, memsize);

	/* WaveBank Entries */
	assert(io->seek(io->data, 0, 1) == wbtable.entryTableOffset);
	if (wbinfo.flags & 0x0002)
	{
		for (i = 0; i < wbinfo.entryCount - 1; i += 1)
		{
			io->read(
				io->data,
				&compactEntry,
				sizeof(compactEntry),
				1
			);
			wb->entries[i].PlayRegion.dwOffset = (
				(compactEntry & ((1 << 21) - 1)) *
				wbinfo.alignment
			);
			wb->entries[i].PlayRegion.dwLength = (
				(compactEntry >> 21) & ((1 << 11) - 1)
			);

			/* TODO: Deviation table */
			io->seek(
				io->data,
				wbinfo.metadataElementSize,
				1
			);
			wb->entries[i].PlayRegion.dwLength = (
				(compactEntry & ((1 << 21) - 1)) *
				wbinfo.alignment
			) - wb->entries[i].PlayRegion.dwOffset;

			wb->entries[i].PlayRegion.dwOffset +=
				wbtable.playRegionOffset;
		}

		io->read(
			io->data,
			&compactEntry,
			sizeof(compactEntry),
			1
		);
		wb->entries[i].PlayRegion.dwOffset = (
			(compactEntry & ((1 << 21) - 1)) *
			wbinfo.alignment
		);

		/* TODO: Deviation table */
		io->seek(
			io->data,
			wbinfo.metadataElementSize,
			1
		);
		wb->entries[i].PlayRegion.dwLength = (
			wbtable.playRegionLength -
			wb->entries[i].PlayRegion.dwOffset
		);

		wb->entries[i].PlayRegion.dwOffset +=
			wbtable.playRegionOffset;
	}
	else
	{
		for (i = 0; i < wbinfo.entryCount; i += 1)
		{
			io->read(
				io->data,
				&wb->entries[i],
				wbinfo.metadataElementSize,
				1
			);
			wb->entries[i].PlayRegion.dwOffset +=
				wbtable.playRegionOffset;
		}

		/* FIXME: This is a bit hacky. */
		if (wbinfo.metadataElementSize < 24)
		{
			for (i = 0; i < wbinfo.entryCount; i += 1)
			{
				wb->entries[i].PlayRegion.dwLength =
					wbtable.playRegionLength;
			}
		}
	}

	/* Finally. */
	*ppWaveBank = wb;
	return 0;
}

uint32_t FACTAudioEngine_CreateInMemoryWaveBank(
	FACTAudioEngine *pEngine,
	const void *pvBuffer,
	uint32_t dwSize,
	uint32_t dwFlags,
	uint32_t dwAllocAttributes,
	FACTWaveBank **ppWaveBank
) {
	return FACT_ParseWaveBank(
		pEngine,
		FACT_memopen((void*) pvBuffer, dwSize),
		0,
		ppWaveBank
	);
}

uint32_t FACTAudioEngine_CreateStreamingWaveBank(
	FACTAudioEngine *pEngine,
	const FACTStreamingParameters *pParms,
	FACTWaveBank **ppWaveBank
) {
	FACTIOStream *io = (FACTIOStream*) pParms->file;
	io->seek(io, pParms->offset, 0);
	return FACT_ParseWaveBank(
		pEngine,
		io,
		1,
		ppWaveBank
	);
}

uint32_t FACTAudioEngine_PrepareWave(
	FACTAudioEngine *pEngine,
	uint32_t dwFlags,
	const char *szWavePath,
	uint32_t wStreamingPacketSize,
	uint32_t dwAlignment,
	uint32_t dwPlayOffset,
	uint8_t nLoopCount,
	FACTWave **ppWave
) {
	/* TODO */
	return 0;
}

uint32_t FACTAudioEngine_PrepareInMemoryWave(
	FACTAudioEngine *pEngine,
	uint32_t dwFlags,
	FACTWaveBankEntry entry,
	uint32_t *pdwSeekTable, /* Optional! */
	uint8_t *pbWaveData,
	uint32_t dwPlayOffset,
	uint8_t nLoopCount,
	FACTWave **ppWave
) {
	/* TODO */
	return 0;
}

uint32_t FACTAudioEngine_PrepareStreamingWave(
	FACTAudioEngine *pEngine,
	uint32_t dwFlags,
	FACTWaveBankEntry entry,
	FACTStreamingParameters streamingParams,
	uint32_t dwAlignment,
	uint32_t *pdwSeekTable, /* Optional! */
	uint8_t *pbWaveData,
	uint32_t dwPlayOffset,
	uint8_t nLoopCount,
	FACTWave **ppWave
) {
	/* TODO */
	return 0;
}

uint32_t FACTAudioEngine_RegisterNotification(
	FACTAudioEngine *pEngine,
	const FACTNotificationDescription *pNotificationDescription
) {
	/* TODO */
	return 0;
}

uint32_t FACTAudioEngine_UnRegisterNotification(
	FACTAudioEngine *pEngine,
	const FACTNotificationDescription *pNotificationDescription
) {
	/* TODO */
	return 0;
}

uint16_t FACTAudioEngine_GetCategory(
	FACTAudioEngine *pEngine,
	const char *szFriendlyName
) {
	uint16_t i;
	for (i = 0; i < pEngine->categoryCount; i += 1)
	{
		if (FACT_strcmp(szFriendlyName, pEngine->categoryNames[i]) == 0)
		{
			return i;
		}
	}

	assert(0 && "Category name not found!");
	return 0;
}

uint32_t FACTAudioEngine_Stop(
	FACTAudioEngine *pEngine,
	uint16_t nCategory,
	uint32_t dwFlags
) {
	/* TODO */
	return 0;
}

uint32_t FACTAudioEngine_SetVolume(
	FACTAudioEngine *pEngine,
	uint16_t nCategory,
	uint32_t dwFlags
) {
	/* TODO */
	return 0;
}

uint32_t FACTAudioEngine_Pause(
	FACTAudioEngine *pEngine,
	uint16_t nCategory,
	int32_t fPause
) {
	/* TODO */
	return 0;
}

uint16_t FACTAudioEngine_GetGlobalVariableIndex(
	FACTAudioEngine *pEngine,
	const char *szFriendlyName
) {
	uint16_t i;
	for (i = 0; i < pEngine->variableCount; i += 1)
	{
		if (	FACT_strcmp(szFriendlyName, pEngine->variableNames[i]) == 0 &&
			pEngine->variables[i].accessibility & 0x04	)
		{
			return i;
		}
	}
	assert(0 && "Variable name not found!");
	return 0;
}

uint32_t FACTAudioEngine_SetGlobalVariable(
	FACTAudioEngine *pEngine,
	uint16_t nIndex,
	float nValue
) {
	FACTVariable *var = &pEngine->variables[nIndex];
	assert(var->accessibility & 0x01);
	assert(var->accessibility & 0x04);
	assert(!(var->accessibility & 0x02));
	pEngine->globalVariableValues[nIndex] = FACT_clamp(
		nValue,
		var->minValue,
		var->maxValue
	);
	return 0;
}

uint32_t FACTAudioEngine_GetGlobalVariable(
	FACTAudioEngine *pEngine,
	uint16_t nIndex,
	float *pnValue
) {
	FACTVariable *var = &pEngine->variables[nIndex];
	assert(var->accessibility & 0x01);
	assert(var->accessibility & 0x04);
	*pnValue = pEngine->globalVariableValues[nIndex];
	return 0;
}
