/*

  Nullsoft WASABI Source File License

  Copyright 1999-2003 Nullsoft, Inc.

    This software is provided 'as-is', without any express or implied
    warranty.  In no event will the authors be held liable for any damages
    arising from the use of this software.

    Permission is granted to anyone to use this software for any purpose,
    including commercial applications, and to alter it and redistribute it
    freely, subject to the following restrictions:

    1. The origin of this software must not be misrepresented; you must not
       claim that you wrote the original software. If you use this software
       in a product, an acknowledgment in the product documentation would be
       appreciated but is not required.
    2. Altered source versions must be plainly marked as such, and must not be
       misrepresented as being the original software.
    3. This notice may not be removed or altered from any source distribution.

*/
//
//   makewav.cpp - by Don Cross <dcross@intersrv.com>
//
//   Get latest version of this file at:
//   http://www.intersrv.com/~dcross/makewav
//
//   Get latest version of C++ code for class WaveFile at:
//   http://www.intersrv.com/~dcross/wavio.html
//   

#include <stdio.h>
#include <stdlib.h>
#include <fstream.h>

#include "riff.h"
#include "gain_analysis.h"

#define MAX_CHUNK 10000

const int MAX_CHANNELS = 4;   // make larger if needed

void showVersion() {
	cerr << "WavGain version 0.1" << endl;
}

void showUsage(const char *progname) {
	showVersion();
	cerr << endl << "Usage: " << progname << " [options] <filename> [<filename 2> ...]" << endl;
	cerr << "  --use \"" << progname << " /?\" for a full list of options" << endl << endl;
	cerr << "each line of output is \"<filename> [TAB] <radio gain> [TAB] <peak (ratio)>\"" << endl;
	cerr << "last line is \"Album [TAB] <album gain> [TAB] <peak (ratio)>\"" << endl;
}

void showFullUsage(const char *progname) {
	showVersion();
	cerr << endl << "Usage: " << progname << " [options] <filename> [<filename 2> ...]" << endl << endl;
	cerr << "each line of output is \"<filename> [TAB] <radio gain> [TAB] <peak (ratio)>\"" << endl;
	cerr << "last line is \"Album [TAB] <album gain> [TAB] <peak (ratio)>\"" << endl << endl;
	cerr << "Options: " << endl;
	cerr << "\t/v - show program version number" << endl;
	cerr << "\t/? - show options" << endl;
	cerr << "\t...ummm... that's about all the options for now." << endl;
}

int doFile(const char *inFilename, unsigned long *lastFreq, long *maxSample) {
	long maxFileSample = 0;
	long totsamp = 0;

	WaveFile inWave;
	DDCRET rc = inWave.OpenForRead( inFilename );

	switch (rc) {
	case DDC_SUCCESS:           // The operation succeded
		break;

	case DDC_INVALID_FILE:       // File format does not match
		cerr << inFilename << " is not a valid WAV file" << endl;
		return 0;

	default:           // The operation failed for unspecified reasons
		cerr << "Error opening WAV file " << inFilename << endl;
		return 0;

	}

	int bit8;

	switch (inWave.BitsPerSample()) {
		case 8:
			bit8 = 1;
			break;

		case 16:
			bit8 = 0;
			break;

		default:
			cerr << "Sorry, only 8-bit or 16-bit WAV files" << endl;
			return 0;
	}

	if (inWave.SamplingRate() != *lastFreq) {
		*lastFreq = inWave.SamplingRate();
		ResetSampleFrequency(*lastFreq);
	}

	if (inWave.NumChannels() == 2) {
		Float_t lsamps[MAX_CHUNK], rsamps[MAX_CHUNK];

		int done = 0;
		int cursamp = 0;
		INT16 lsamp, rsamp;

		rc = inWave.ReadStereoSample(&lsamp,&rsamp);

		while (!done) {

			while ((cursamp < MAX_CHUNK) && (rc == DDC_SUCCESS)) {

				if (bit8) {
					lsamp = ((lsamp - 128) << 8) - 1;
					rsamp = ((rsamp - 128) << 8) - 1;
				}

				if (lsamp > maxFileSample)
					maxFileSample = lsamp;
				else if (-lsamp > maxFileSample)
					maxFileSample = -lsamp;

				if (rsamp > maxFileSample)
					maxFileSample = rsamp;
				else if (-rsamp > maxFileSample)
					maxFileSample = -rsamp;

				lsamps[cursamp] = (Float_t)(lsamp); // << multiplier);
				rsamps[cursamp++] = (Float_t)(rsamp); // << multiplier);

				rc = inWave.ReadStereoSample(&lsamp,&rsamp);
			}

			if (cursamp > 0) {
				totsamp += cursamp;
				AnalyzeSamples(lsamps,rsamps,cursamp,2);
				cursamp = 0;
			}

			done = (rc != DDC_SUCCESS);
		}
	}
	else if (inWave.NumChannels() == 1) {
		Float_t samps[MAX_CHUNK];

		int done = 0;
		int cursamp = 0;
		INT16 samp;

		rc = inWave.ReadMonoSample(&samp);

		while (!done) {

			while ((cursamp < MAX_CHUNK) && (rc == DDC_SUCCESS)) {
				
				if (bit8) {
					samp = ((samp - 128) << 8) - 1;
				}

				if (samp > maxFileSample)
					maxFileSample = samp;
				else if (-samp > maxFileSample)
					maxFileSample = -samp;

				samps[cursamp++] = (Float_t)(samp); // << multiplier);

				rc = inWave.ReadMonoSample(&samp);
			}

			if (cursamp > 0) {
				totsamp += cursamp;
				AnalyzeSamples(samps,NULL,cursamp,1);
				cursamp = 0;
			}

			done = (rc != DDC_SUCCESS);
		}
	}
	else {
		cerr << "Can't handle more than 2 channels." << endl;
		return 0;
	}

	if (maxFileSample > *maxSample)
		*maxSample = maxFileSample;

	inWave.Close();

	if (totsamp > 0) {
		cout << inFilename << "\t" << GetTitleGain() << "\t" << ((double)(maxFileSample) / 32767.0) << endl;
		return 1;
	}

	return 0;
}

int main ( int argc, const char *argv[] )
{
	
	if (argc < 2) {
		showUsage(argv[0]);
		return 1;
	}

	unsigned long lastFreq = 44100;
	int fileStart = 1;
	int okFilecount = 0;
	long maxSample = 0;
	int i;

	InitGainAnalysis(lastFreq);

	for (i = 1; i < argc; i++) {
		if (argv[i][0] == '/') {
			fileStart++;
			switch(argv[i][1]) {
				case 'h':
				case 'H':
				case '?':
					showFullUsage(argv[0]);
					return 1;

				case 'v':
				case 'V':
					showVersion();
					return 1;

				default:
					cerr << "Unrecognized option: /" << argv[i][1] << endl;
			}
		}
	}

	for (i = fileStart; i < argc; i++) {
		okFilecount += doFile(argv[i], &lastFreq, &maxSample);
	}

	if (okFilecount)
		cout << "Album" << "\t" << GetAlbumGain() << "\t" << ((double)(maxSample) / 32767.0) << endl;

    return 0;
}

