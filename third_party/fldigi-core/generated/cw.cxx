// ----------------------------------------------------------------------------
// cw.cxx  --  morse code modem
//
// Copyright (C) 2006-2010
//		Dave Freese, W1HKJ
//		   (C) Mauri Niininen, AG1LE
//
// This file is part of fldigi.  Adapted from code contained in gmfsk source code
// distribution.
//  gmfsk Copyright (C) 2001, 2002, 2003
//  Tomi Manninen (oh2bns@sral.fi)
//  Copyright (C) 2004
//  Lawrence Glaister (ve7it@shaw.ca)
//
// Fldigi is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// Fldigi is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with fldigi.  If not, see <http://www.gnu.org/licenses/>.
// ----------------------------------------------------------------------------

#define CW_DEBUG 0

#include <config.h>

#include <cstring>
#include <string>
#include <stdio.h>
#include <iostream>
#include <fstream>
#include <cstdlib>

#include "timeops.h"
#ifdef __MINGW32__
#  include "compat.h"
#endif

#if !HAVE_CLOCK_GETTIME
#  ifdef __APPLE__
#    include <mach/mach_time.h>
#  endif
#endif

#include "digiscope.h"
#include "waterfall.h"
#include "fl_digi.h"
#include "fftfilt.h"
#include "serial.h"
#include "ptt.h"
#include "main.h"

#include "cw.h"
#include "misc.h"
#include "configuration.h"
#include "confdialog.h"
#include "status.h"
#include "debug.h"
#include "FTextRXTX.h"
#include "modem.h"

#include "qrunner.h"

#include "winkeyer.h"
#include "nanoIO.h"
#include "KYkeying.h"
#include "ICOMkeying.h"
#include "YAESUkeying.h"

#include "audio_alert.h"

#include "gpio_common.h"

#define FIR_DECIMATE    10 //16

void start_cwio_thread();
void stop_cwio_thread();

static double nano_d2d = 0;
static int nano_wpm = 0;
static bool first_time = true;
static double cw_freq = 1500;
static int FIR_FILTER_LEN = 512;

#if USE_LIBGPIOD

void start_gpio_thread();
void stop_gpio_thread();
static gpio_num_t cw_gpio_num = GPIO_COMMON_UNKNOWN;

#endif

const cw::SOM_TABLE cw::som_table[] = {
	/* Prosigns */
	{"-...-",	{1.0,  0.33,  0.33,  0.33, 1.0,   0, 0} },
	{".-.-",	{ 0.33, 1.0,  0.33, 1.0,   0,   0, 0} },
	{".-...",	{ 0.33, 1.0,  0.33,  0.33,  0.33,   0, 0} },
	{".-.-.",	{ 0.33, 1.0,  0.33, 1.0,  0.33,   0, 0} },
	{"...-.-",	{ 0.33,  0.33,  0.33, 1.0,  0.33, 1.0, 0} },
	{"-.--.",	{1.0,  0.33, 1.0, 1.0,  0.33,   0, 0} },
	{"..-.-",	{ 0.33,  0.33, 1.0,  0.33, 1.0,   0, 0} },
	{"....--",	{ 0.33,  0.33,  0.33,  0.33, 1.0, 1.0, 0} },
	{"...-.",	{ 0.33,  0.33,  0.33, 1.0,  0.33,   0, 0} },
	/* ASCII 7bit letters */
	{".-",		{ 0.33, 1.0,   0,   0,   0,   0, 0}	},
	{"-...",	{1.0,  0.33,  0.33,  0.33,   0,   0, 0}	},
	{"-.-.",	{1.0,  0.33, 1.0,  0.33,   0,   0, 0}	},
	{"-..",		{1.0,  0.33,  0.33,   0,   0,   0, 0} 	},
	{".",		{ 0.33,   0,   0,   0,   0,   0, 0}	},
	{"..-.",	{ 0.33,  0.33, 1.0,  0.33,   0,   0, 0}	},
	{"--.",		{1.0, 1.0,  0.33,   0,   0,   0, 0}	},
	{"....",	{ 0.33,  0.33,  0.33,  0.33,   0,   0, 0}	},
	{"..",		{ 0.33,  0.33,   0,   0,   0,   0, 0}	},
	{".---",	{ 0.33, 1.0, 1.0, 1.0,   0,   0, 0}	},
	{"-.-",		{1.0,  0.33, 1.0,   0,   0,   0, 0}	},
	{".-..",	{ 0.33, 1.0,  0.33,  0.33,   0,   0, 0}	},
	{"--",		{1.0, 1.0,   0,   0,   0,   0, 0}	},
	{"-.",		{1.0,  0.33,   0,   0,   0,   0, 0}	},
	{"---",		{1.0, 1.0, 1.0,   0,   0,   0, 0}	},
	{".--.",	{ 0.33, 1.0, 1.0,  0.33,   0,   0, 0}	},
	{"--.-",	{1.0, 1.0,  0.33, 1.0,   0,   0, 0}	},
	{".-.",		{ 0.33, 1.0,  0.33,   0,   0,   0, 0}	},
	{"...",		{ 0.33,  0.33,  0.33,   0,   0,   0, 0}	},
	{"-",		{1.0,   0,   0,   0,   0,   0, 0}	},
	{"..-",		{ 0.33,  0.33, 1.0,   0,   0,   0, 0}	},
	{"...-",	{ 0.33,  0.33,  0.33, 1.0,   0,   0, 0}	},
	{".--",		{ 0.33, 1.0, 1.0,   0,   0,   0, 0}	},
	{"-..-",	{1.0,  0.33,  0.33, 1.0,   0,   0, 0}	},
	{"-.--",	{1.0,  0.33, 1.0, 1.0,   0,   0, 0}	},
	{"--..",	{1.0, 1.0,  0.33,  0.33,   0,   0, 0}	},
	/* Numerals */
	{"-----",	{1.0, 1.0, 1.0, 1.0, 1.0,   0, 0}	},
	{".----",	{ 0.33, 1.0, 1.0, 1.0, 1.0,   0, 0}	},
	{"..---",	{ 0.33,  0.33, 1.0, 1.0, 1.0,   0, 0}	},
	{"...--",	{ 0.33,  0.33,  0.33, 1.0, 1.0,   0, 0}	},
	{"....-",	{ 0.33,  0.33,  0.33,  0.33, 1.0,   0, 0}	},
	{".....",	{ 0.33,  0.33,  0.33,  0.33,  0.33,   0, 0}	},
	{"-....",	{1.0,  0.33,  0.33,  0.33,  0.33,   0, 0}	},
	{"--...",	{1.0, 1.0,  0.33,  0.33,  0.33,   0, 0}	},
	{"---..",	{1.0, 1.0, 1.0,  0.33,  0.33,   0, 0}	},
	{"----.",	{1.0, 1.0, 1.0, 1.0,  0.33,   0, 0}	},
	/* Punctuation */
	{".-..-.",	{ 0.33, 1.0,  0.33,  0.33, 1.0,  0.33, 0}	},
	{".----.",	{ 0.33, 1.0, 1.0, 1.0, 1.0,  0.33, 0}	},
	{"...-..-",	{ 0.33,  0.33,  0.33, 1.0,  0.33,  0.33, 1.0}	},
	{"-.---.",	{1.0,  0.33, 1.0, 1.0,  0.33,   0, 0}	},
	{"-.--.-",	{1.0,  0.33, 1.0, 1.0,  0.33, 1.0, 0}	},
	{"--..--",	{1.0, 1.0,  0.33,  0.33, 1.0, 1.0, 0}	},
	{"-....-",	{1.0,  0.33,  0.33,  0.33,  0.33, 1.0, 0}	},
	{".-.-.-",	{ 0.33, 1.0,  0.33, 1.0,  0.33, 1.0, 0}	},
	{"-..-.",	{1.0,  0.33,  0.33, 1.0,  0.33,   0, 0}	},
	{"---...",	{1.0, 1.0, 1.0,  0.33,  0.33,  0.33, 0}	},
	{"-.-.-.",	{1.0,  0.33, 1.0,  0.33, 1.0,  0.33, 0}	},
	{"..--..",	{ 0.33,  0.33, 1.0, 1.0,  0.33,  0.33, 0}	},
	{"..--.-",	{ 0.33,  0.33, 1.0, 1.0,  0.33, 1.0, 0}	},
	{".--.-.",	{ 0.33, 1.0, 1.0,  0.33, 1.0,  0.33, 0}	},
	{"-.-.--",	{1.0,  0.33, 1.0,  0.33, 1.0, 1.0, 0}	},

	{".-.-",	{0.33, 1.0, 0.33, 1.0, 0, 0 , 0}  },	// A umlaut, A aelig
	{".--.-",	{0.33, 1.0, 1.0, 0.33, 1.0, 0, 0 }  },	// A ring
	{"-.-..",	{1.0, 0.33, 1.0, 0.33, 0.33, 0, 0} },	// C cedilla
	{".-..-",	{0.33, 1.0, 0.33, 0.33, 1.0, 0, 0} },	// E grave
	{"..-..",	{0.33, 0.33, 1.0, 0.33, 0.33, 0, 0} },	// E acute
	{"---.",	{1.0, 1.0, 1.0, 0.33, 0, 0, 0} },		// O acute, O umlat, O slash
	{"--.--",	{1.0, 1.0, 0.33, 1.0, 1.0, 0, 0} },		// N tilde
	{"..--",	{0.33, 0.33, 1.0, 1.0, 0, 0, 0} },		// U umlaut, U circ

	{"", {0.0}}
};

int cw::normalize(float *v, int n, int twodots)
{
	if( n == 0 ) return 0 ;

	float max = v[0];
	float min = v[0];
	int j;

	/* find max and min values */
	for (j=1; j<n; j++) {
		float vj = v[j];
		if (vj > max)	max = vj;
		else if (vj < min)	min = vj;
	}
	/* all values 0 - no need to normalize or decode */
	if (max == 0.0) return 0;

	/* scale values between  [0,1] -- if Max longer than 2 dots it was "dah" and should be 1.0, otherwise it was "dit" and should be 0.33 */
	float ratio = (max > twodots) ? 1.0 : 0.33 ;
	ratio /= max ;
	for (j=0; j<n; j++) v[j] *= ratio;
	return (1);
}


std::string cw::find_winner (float *inbuf, int twodots)
{
	float diffsf = 999999999999.0;

	if ( normalize (inbuf, WGT_SIZE, twodots) == 0) return " ";

	int winner = -1;
	for ( int n = 0; som_table[n].rpr.length(); n++) {
		 /* Compute the distance between codebook and input entry */
		float difference = 0.0;
	   	for (int i = 0; i < WGT_SIZE; i++) {
			float diff = (inbuf[i] - som_table[n].wgt[i]);
					difference += diff * diff;
					if (difference > diffsf) break;
	  		}

	 /* If distance is smaller than previous distances */
			if (difference < diffsf) {
	  			winner = n;
	  			diffsf = difference;
			}
	}

	std::string sc;
	if (!som_table[winner].rpr.empty()) {
		sc = morse->rx_lookup(som_table[winner].rpr);
		if (sc.empty()) 
			sc = (progdefaults.CW_noise == '*' ? "*" :
				  progdefaults.CW_noise == '_' ? "_" :
				  progdefaults.CW_noise == ' ' ? " " : "");
	} else
		sc = (progdefaults.CW_noise == '*' ? "*" :
			  progdefaults.CW_noise == '_' ? "_" :
			  progdefaults.CW_noise == ' ' ? " " : "");
	return sc;
}

void cw::tx_init()
{
	phaseacc = 0;
	lastsym = 0;
	qskphase = 0;
	if (progdefaults.pretone) pretone();

	symbols = 0;
	acc_symbols = 0;
	ovhd_symbols = 0;

	maxval = 0.0;
}

void cw::rx_init()
{
    webEnvelopePower = 0;
    webEnvelopeCount = 0;
    webSpaceSent = true;
    webLastElement = 0;
	cw_receive_state = RS_IDLE;
	smpl_ctr = 0;
	cw_rr_current = 0;
	cw_ptr = 0;
	agc_peak = 0.0;
	set_scope_mode(Digiscope::SCOPE);

	update_Status();
	usedefaultWPM = false;
	scope_clear = true;

	viewcw.restart();
}

void cw::init()
{
	bool wfrev = wf->Reverse();
	bool wfsb = wf->USB();
	reverse = wfrev ^ !wfsb;

	trackingfilter->reset();
	two_dots = (long int)trackingfilter->run(2 * cw_send_dot_length);
	put_cwRcvWPM(cw_send_speed);

	memset(outbuf, 0, OUTBUFSIZE*sizeof(*outbuf));
	memset(qskbuf, 0, OUTBUFSIZE*sizeof(*qskbuf));

	morse->init();
	use_paren = progdefaults.CW_use_paren;
	prosigns = progdefaults.CW_prosigns;

	rx_init();

	stopflag = false;
	maxval = 0;

	if (use_nanoIO) set_nanoCW();

	reset_rx_filter();

}

cw::~cw() {
	if (cw_filter) delete cw_filter;
	if (bitfilter) delete bitfilter;
	if (trackingfilter) delete trackingfilter;
	stop_cwio_thread();
#if USE_LIBGPIOD
	stop_gpio_thread();
#endif
}

static int debug_count = 0;
static int filnbr = -1;

cw::cw() : modem()
{
    first_time = true;
	cap |= CAP_BW;

	mode = MODE_CW;
	freqlock = false;
	usedefaultWPM = false;

	risetime = progdefaults.CWrisetime;
	QSKshape = progdefaults.QSKshape;

	cw_ptr = 0;
	clrcount = CLRCOUNT;

	samplerate = CW_SAMPLERATE;
	fragmentsize = CWMaxSymLen;

	wpm = cw_speed  = progdefaults.CWspeed;
	bandwidth = progdefaults.CWbandwidth;

	cw_send_speed = cw_speed;
	cw_receive_speed = cw_speed;
	two_dots = 2 * KWPM / cw_speed;
	cw_noise_spike_threshold = two_dots / 4;
	cw_send_dot_length = KWPM / cw_send_speed;
	cw_send_dash_length = 3 * cw_send_dot_length;
	symbollen = (int)round(samplerate * 1.2 / progdefaults.CWspeed);  // transmit char rate
	fsymlen = (int)round(samplerate * 1.2 / progdefaults.CWfarnsworth); // transmit word rate

	rx_rep_buf.clear();

// block of variables that get updated each time speed changes
	pipesize = (22 * samplerate * 12) / (progdefaults.CWspeed * 160);
	if (pipesize < 0) pipesize = 512;
	if (pipesize > MAX_PIPE_SIZE) pipesize = MAX_PIPE_SIZE;

	cwTrack = true;
	phaseacc = 0.0;
	FFTphase = 0.0;
	FFTvalue = 0.0;
	pipeptr = 0;
	clrcount = 0;

	upper_threshold = progdefaults.CWupper;
	lower_threshold = progdefaults.CWlower;
	for (int i = 0; i < MAX_PIPE_SIZE; clearpipe[i++] = 0.0);

	agc_peak = 0.0;
	in_replay = 0;

	use_matched_filter = progdefaults.CWmfilt;

	if (progdefaults.StartAtSweetSpot) frequency = progdefaults.CWsweetspot;

	bandwidth = progdefaults.CWbandwidth;
	if (use_matched_filter)
		bandwidth = 2 * progdefaults.CWspeed;

	switch (progdefaults.CW_fillen) {
		case 0: FIR_FILTER_LEN = 128; break;
		case 1: FIR_FILTER_LEN = 256; break;
		case 2: default: FIR_FILTER_LEN = 512; break;
		case 3: FIR_FILTER_LEN = 1024; break;
	}
	filnbr = progdefaults.CW_fillen;

	cw_filter = new C_FIR_filter();
	cw_filter->init_bandpass(FIR_FILTER_LEN, 1, 1.0*(frequency - bandwidth/2)/samplerate, 1.0*(frequency + bandwidth/2)/samplerate);


	int bfv = (symbollen / FIR_DECIMATE ) / 3;
	if (bfv < 1) bfv = 1;

	bitfilter = new Cmovavg(bfv);

	trackingfilter = new Cmovavg(TRACKING_FILTER_SIZE);

	create_edges();

	nano_wpm = progdefaults.CWspeed;
	nano_d2d = progdefaults.CWdash2dot;

	sync_parameters();

	REQ(static_cast<void (waterfall::*)(int)>(&waterfall::Bandwidth), wf, (int)bandwidth);
	REQ(static_cast<int (Fl_Counter2::*)(double)>(&Fl_Counter2::value), cntCWbandwidth, (int)bandwidth);
	REQ(static_cast<int (Fl_Counter2::*)(double)>(&Fl_Counter2::value), cntcwsbandwidth, (int)bandwidth);


	update_Status();

	synchscope = 50;
	noise_floor = 1.0;
	sig_avg = 0.5;

	cal_wpm = 20;

	start_cwio_thread();

#if USE_LIBGPIOD
	cw_gpio_num = GPIO_COMMON_UNKNOWN;
#endif

}

void cw::reset_rx_filter()
{
	if (first_time ||
		(cw_freq != wf->Carrier()) ||
		(use_matched_filter != progdefaults.CWmfilt) ||
		filnbr != progdefaults.CW_fillen ||
		(progdefaults.CWmfilt && (cw_speed != progdefaults.CWspeed)) ||
		(bandwidth != progdefaults.CWbandwidth && !use_matched_filter)) {

		double sf = 0;

		cw_speed = progdefaults.CWspeed;

		if (first_time) {
			if (progdefaults.StartAtSweetSpot) sf = progdefaults.CWsweetspot;
			else if (progStatus.carrier != 0) sf = progStatus.carrier;
			else sf = wf->Carrier();
		} else
			sf = wf->Carrier();

		set_freq(cw_freq = sf);

		use_matched_filter = progdefaults.CWmfilt;

		bandwidth = progdefaults.CWbandwidth;
		if (use_matched_filter)
			bandwidth = 2 * progdefaults.CWspeed;

		filnbr = progdefaults.CW_fillen;
		switch (progdefaults.CW_fillen) {
			case 0: FIR_FILTER_LEN = 128; break;
			case 1: FIR_FILTER_LEN = 256; break;
			case 2: default: FIR_FILTER_LEN = 512; break;
			case 3: FIR_FILTER_LEN = 1024; break;
		}

		cw_filter->init_bandpass(FIR_FILTER_LEN, 1, 1.0*(frequency - bandwidth/2)/samplerate, 1.0*(frequency + bandwidth/2)/samplerate);

		FFTphase = 0;

		REQ(static_cast<void (waterfall::*)(int)>(&waterfall::Bandwidth), wf, (int)bandwidth);
		REQ(static_cast<int (Fl_Counter2::*)(double)>(&Fl_Counter2::value), cntCWbandwidth, (int)bandwidth);
		REQ(static_cast<int (Fl_Counter2::*)(double)>(&Fl_Counter2::value), cntcwsbandwidth, (int)bandwidth);

		pipesize = (22 * samplerate * 12) / (progdefaults.CWspeed * 160);
		if (pipesize < 0) pipesize = 512;
		if (pipesize > MAX_PIPE_SIZE) pipesize = MAX_PIPE_SIZE;

		two_dots = 2 * KWPM / cw_speed;
		cw_noise_spike_threshold = two_dots / 4;
		cw_send_dot_length = KWPM / cw_send_speed;
		cw_send_dash_length = 3 * cw_send_dot_length;

		symbollen = (int)round(samplerate * 1.2 / progdefaults.CWspeed);
		fsymlen = (int)round(samplerate * 1.2 / progdefaults.CWfarnsworth);

		int bfv = (symbollen / FIR_DECIMATE ) / 3;
		if (bfv < 1) bfv = 1;

		bitfilter->setLength(bfv);

if (CW_DEBUG)
{
	printf("\
Statitistics: %d\n\
   dot length:        %ld\n\
   dash length:       %ld\n\
   noise threshold:   %ld\n\
   symbollen:         %d\n\
   fsymlen:           %d\n\
   bit filter length: %d\n\
   frequency:         %f\n\
   bandwidth:         %f\n\
   matched:           %d\n\
   FIR filter length: %d\n",
		++debug_count,
		cw_send_dot_length,
		cw_send_dash_length,
		cw_noise_spike_threshold,
		symbollen,
		fsymlen,
		bfv,
		frequency,
		bandwidth,
		use_matched_filter,
		FIR_FILTER_LEN
	);
}

		phaseacc = 0.0;
		FFTphase = 0.0;
		FFTvalue = 0.0;
		pipeptr = 0;
		clrcount = 0;
		smpl_ctr = 0;

		rx_rep_buf.clear();

		siglevel = 0;

	}
	first_time = false;
}

// sync_parameters()
// Synchronize the dot, dash, end of element, end of character, and end
// of word timings and ranges to new values of Morse speed, or receive tolerance.

void cw::sync_transmit_parameters()
{
//	wpm = usedefaultWPM ? progdefaults.defCWspeed : progdefaults.CWspeed;
	fwpm = progdefaults.CWfarnsworth;

	cw_send_dot_length = KWPM / progdefaults.CWspeed;
	cw_send_dash_length = 3 * cw_send_dot_length;

	nusymbollen = (int)round(samplerate * 1.2 / progdefaults.CWspeed);
	nufsymlen = (int)round(samplerate * 1.2 / fwpm);

	if (symbollen != nusymbollen ||
		nufsymlen != fsymlen ||
		risetime  != progdefaults.CWrisetime ||
		QSKshape  != progdefaults.QSKshape) {
		risetime = progdefaults.CWrisetime;
		QSKshape = progdefaults.QSKshape;
		symbollen = nusymbollen;
		fsymlen = nufsymlen;
		create_edges();
	}
}

void cw::sync_parameters()
{
	sync_transmit_parameters();

	if (use_nanoIO) {
		if (nano_wpm != progdefaults.CWspeed) {
			nano_wpm = progdefaults.CWspeed;
			set_nanoWPM(progdefaults.CWspeed);
		}
		if (nano_d2d != progdefaults.CWdash2dot) {
			nano_d2d = progdefaults.CWdash2dot;
			set_nano_dash2dot(progdefaults.CWdash2dot);
		}
	}

// check if user changed the tracking or the cw default speed
	if ((cwTrack != progdefaults.CWtrack) ||
		(cw_send_speed != progdefaults.CWspeed)) {
		trackingfilter->reset();
		two_dots = 2 * cw_send_dot_length;
		put_cwRcvWPM(cw_send_speed);
	}
	cwTrack = progdefaults.CWtrack;
	cw_send_speed = progdefaults.CWspeed;

// Receive parameters:
	lowerwpm = cw_send_speed - progdefaults.CWrange;
	upperwpm = cw_send_speed + progdefaults.CWrange;
	if (lowerwpm < progdefaults.CWlowerlimit)
		lowerwpm = progdefaults.CWlowerlimit;
	if (upperwpm > progdefaults.CWupperlimit)
		upperwpm = progdefaults.CWupperlimit;
	cw_lower_limit = 2 * KWPM / upperwpm;
	cw_upper_limit = 2 * KWPM / lowerwpm;

	if (cwTrack)
		cw_receive_speed = KWPM / (two_dots / 2);
	else {
		cw_receive_speed = cw_send_speed;
		two_dots = 2 * cw_send_dot_length;
	}

	if (cw_receive_speed > 0)
		cw_receive_dot_length = KWPM / cw_receive_speed;
	else
		cw_receive_dot_length = KWPM / 5;

	cw_receive_dash_length = 3 * cw_receive_dot_length;

	cw_noise_spike_threshold = cw_receive_dot_length / 2;

}


//=======================================================================
// cw_update_tracking()
//=======================================================================

inline void cw::update_tracking(int dur_1, int dur_2)
{
static int min_dot = KWPM / 200;
static int max_dash = 3 * KWPM / 5;
	if ((dur_1 > dur_2) && (dur_1 > 4 * dur_2)) return;
	if ((dur_2 > dur_1) && (dur_2 > 4 * dur_1)) return;
	if (dur_1 < min_dot || dur_2 < min_dot) return;
	if (dur_2 > max_dash || dur_2 > max_dash) return;

	two_dots = trackingfilter->run((dur_1 + dur_2) / 2);

	sync_parameters();
}

void cw::update_Status()
{
	put_MODEstatus("CW %s Rx %d", usedefaultWPM ? "*" : " ", cw_receive_speed);
}

//=======================================================================
//update_syncscope()
//Routine called to update the display on the sync scope display.
//For CW this is an o scope pattern that shows the cw data streacwTrackm.
//=======================================================================
//

void cw::update_syncscope()
{
	if (pipesize < 0 || pipesize > MAX_PIPE_SIZE)
		return;

	for (int i = 0; i < pipesize; i++)
		scopedata[i] = 0.96*pipe[i]+0.02;

	set_scope_xaxis_1(siglevel);

	set_scope(scopedata, pipesize, true);
	scopedata.next(); // change buffers

	clrcount = CLRCOUNT;
	put_cwRcvWPM(cw_receive_speed);
	update_Status();
}

void cw::clear_syncscope()
{
	set_scope_xaxis_1(siglevel);

	set_scope(clearpipe, pipesize, false);
	clrcount = CLRCOUNT;
}

cmplx cw::mixer(cmplx in)
{
	cmplx z (cos(phaseacc), sin(phaseacc));
	z = z * in;

	phaseacc += TWOPI * frequency / samplerate;
	if (phaseacc > TWOPI) phaseacc -= TWOPI;

	return z;
}

/*
bool header = false;
bool keydown = false;
int  data_num = 0;
*/

void cw::decode_stream(double value)
{
	std::string sc;
	std::string somc;
	int attack = 0;
	int decay = 0;

	sc.clear();

	switch (progdefaults.cwrx_attack) {
		case 0: attack = 400; break;//100; break;
		case 1: default: attack = 200; break;//50; break;
		case 2: attack = 100;//25;
	}
	switch (progdefaults.cwrx_decay) {
		case 0: decay = 2000; break;//1000; break;
		case 1: default : decay = 1000; break;//500; break;
		case 2: decay = 500;//250;
	}

	sig_avg = decayavg(sig_avg, 0.5 * value, (value > sig_avg ? attack : decay ));

	if (value < sig_avg) {
		if (value < noise_floor)
			noise_floor = decayavg(noise_floor, value, attack);
		else 
			noise_floor = decayavg(noise_floor, value, decay);
	}
	if (value > sig_avg)  {
		if (value > agc_peak)
			agc_peak = decayavg(agc_peak, value, attack);
		else
			agc_peak = decayavg(agc_peak, value, decay); 
	}

	float norm_noise;
	float norm_sig;
	float norm_value;

	if (agc_peak) {
		norm_value = value / agc_peak;
		norm_noise = noise_floor / agc_peak;
		norm_sig = sig_avg / agc_peak;
	} 	else {
		norm_value = noise_floor;
		norm_noise = noise_floor;
		norm_sig = noise_floor;
	}
	siglevel = norm_sig;
	progdefaults.CWupper = 1.05 * norm_sig;
	progdefaults.CWlower = 0.95 * norm_sig;

	metric = 0.8 * metric;
	if ((noise_floor > 1e-4) && (noise_floor < sig_avg))
		metric += 0.2 * clamp(2.5 * (20*log10(norm_sig / norm_noise)) , 0, 100);

	pipe[pipeptr] = norm_value;
	if (++pipeptr == pipesize) pipeptr = 0;

	if (!progStatus.sqlonoff || metric > progStatus.sldrSquelchValue ) {
// Power detection using hysterisis detector
// upward trend means tone starting
		if ((norm_value > progdefaults.CWupper) && (cw_receive_state != RS_IN_TONE)) {
			handle_event(CW_KEYDOWN_EVENT, sc);
//			keydown = true;
		}
// downward trend means tone stopping
		else if ((norm_value < progdefaults.CWlower) && (cw_receive_state == RS_IN_TONE)) {
			handle_event(CW_KEYUP_EVENT, sc);
//			keydown = false;
		}
	}

/*
FILE *data;
if (!header) {
	data = fopen("data.txt", "w");
	fprintf(data,"N, Noise, Norm, Signal, Key\n");
	header = true;
	data_num = 0;
	fclose(data);
}
if (data_num >= 1024) {
	data = fopen("data.txt", "a");
	fprintf(data, "%d,%f,%f,%f,%d\n", data_num - 1024, norm_noise, norm_sig, norm_value, keydown);
	fclose(data);
}
++data_num;
*/

//	if (handle_event(CW_QUERY_EVENT, sc) == SC_VALID) {
	handle_event(CW_QUERY_EVENT, sc);
	if (!sc.empty()) {
		update_syncscope();
		synchscope = 100;
		if (progdefaults.CWuseSOMdecoding) {
			somc = find_winner(cw_buffer, two_dots);
			if (!somc.empty())
				for (size_t n = 0; n < somc.length(); n++)
					put_rx_char(
						somc[n],
						somc[0] == '<' ? FTextBase::CTRL : FTextBase::RECV);
			cw_ptr = 0;
			memset(cw_buffer, 0, sizeof(cw_buffer));
		} else {
			for (size_t n = 0; n < sc.length(); n++)
				put_rx_char(
					sc[n],
					sc[0] == '<' ? FTextBase::CTRL : FTextBase::RECV);
		}
	} else {
		if (--synchscope == 0) {
			synchscope = 25;
			update_syncscope();
		}
	}

}

void cw::rx_FFTprocess(const double *buf, int len)
{
	if (len <= 0) return;

	int n = 0;
	double fil_in = 0, fil_out = 0, bit_value = 0;

	for (int i = 0; i < len; i++) {
		fil_in = buf[i];
		cw_filter->Irun(fil_in, fil_out);
        webEnvelopePower += fil_out * fil_out;
        n = ++webEnvelopeCount == FIR_DECIMATE;
        if (n) {
            fil_out = sqrt(webEnvelopePower / FIR_DECIMATE);
            webEnvelopePower = 0;
            webEnvelopeCount = 0;
        }
		smpl_ctr++;
		if (n) {
			bit_value = bitfilter->run(abs(fil_out));
			decode_stream(bit_value);
		}
	}
}

static bool cwprocessing = false;

int cw::rx_process(const double *buf, int len)
{
	if (use_paren != progdefaults.CW_use_paren ||
		prosigns != progdefaults.CW_prosigns) {
		use_paren = progdefaults.CW_use_paren;
		prosigns = progdefaults.CW_prosigns;
		morse->init();
	}

	if (first_time) return 0; // wait for initialization to complete

	if (cwprocessing)
		return 0;

	cwprocessing = true;

	reset_rx_filter();

	rx_FFTprocess(buf, len);

	if (!clrcount--) clear_syncscope();

	display_metric(metric);

	if ( (dlgViewer->visible() || progStatus.show_channels ) 
		&& !bHighSpeed && !bHistory )
		viewcw.rx_process(buf, len);

	cwprocessing = false;

	return 0;
}

// ----------------------------------------------------------------------

// Compare two timestamps, and return the difference between them in usecs.

inline int cw::usec_diff(unsigned int earlier, unsigned int later)
{
	return (earlier >= later) ? 0 : (later - earlier);
}


//=======================================================================
// handle_event()
//	high level cw decoder... gets called with keyup, keydown, reset and
//	query commands.
//   Keyup/down influences decoding logic.
//	Reset starts everything out fresh.
//	The query command returns SC_VALID and the character that has
//	been decoded (may be '*',' ' or [a-z,0-9] or a few others)
//	returns decoded chaacer string, or an empty string if decode
//	is not valid.
//=======================================================================

void cw::handle_event(int cw_event, std::string &sc)
{
	bool& space_sent = webSpaceSent;	// for word space logic
	int& last_element = webLastElement;	// length of last dot/dash
	int element_usec;		// Time difference in usecs

	sc.clear();

	switch (cw_event) {
// ---------
		case CW_RESET_EVENT:
			sync_parameters();
			cw_receive_state = RS_IDLE;
			cw_rr_current = 0;			// reset decoding pointer
			cw_ptr = 0;
			memset(cw_buffer, 0, sizeof(cw_buffer));
			smpl_ctr = 0;					// reset audio sample counter
			rx_rep_buf.clear();
if (CW_DEBUG) printf("RESET   ");
			return;

// ---------
		case CW_KEYDOWN_EVENT:
// A receive tone start can only happen while we
// are idle, or in the middle of a character.
			if (cw_receive_state == RS_IN_TONE)
				return;
// first tone in idle state reset audio sample counter
			if (cw_receive_state == RS_IDLE) {
				smpl_ctr = 0;
				rx_rep_buf.clear();
				cw_rr_current = 0;
				cw_ptr = 0;
			}
// save the timestamp
			cw_rr_start_timestamp = smpl_ctr;
// Set state to indicate we are inside a tone.
			old_cw_receive_state = cw_receive_state;
			cw_receive_state = RS_IN_TONE;
			return;

// ---------
		case CW_KEYUP_EVENT:
// The receive state is expected to be inside a tone.
			if (cw_receive_state != RS_IN_TONE)
				return;
// Save the current timestamp
			cw_rr_end_timestamp = smpl_ctr;
			element_usec = usec_diff(cw_rr_start_timestamp, cw_rr_end_timestamp);

// make sure our timing values are up to date
			sync_parameters();
// If the tone length is shorter than any noise cancelling
// threshold that has been set, then ignore this tone.
			if (element_usec
				&& (cw_noise_spike_threshold > 0)
				&& (element_usec < cw_noise_spike_threshold)) {
				cw_receive_state = RS_IDLE;
//if (CW_DEBUG) printf("NOISE(%d): %f / %f\n", cw_receive_state, 1.0*element_usec, 1.0*cw_noise_spike_threshold);
				return;
			}
// Set up to track speed on dot-dash or dash-dot pairs for this test to work, we need a dot dash pair or a
// dash dot pair to validate timing from and force the speed tracking in the right direction. This method
// is fundamentally different than the method in the unix cw project. Great ideas come from staring at the
// screen long enough!. Its kind of simple really ... when you have no idea how fast or slow the cw is...
// the only way to get a threshold is by having both code elements and setting the threshold between them
// knowing that one is supposed to be 3 times longer than the other. with straight key code... this gets
// quite variable, but with most faster cw sent with electronic keyers, this is one relationship that is
// quite reliable. Lawrence Glaister (ve7it@shaw.ca)
			if (last_element > 0) {
// check for dot dash sequence (current should be 3 x last)
				if ((element_usec > 2 * last_element) &&
					(element_usec < 4 * last_element)) {
					update_tracking(last_element, element_usec);
				}
// check for dash dot sequence (last should be 3 x current)
				if ((last_element > 2 * element_usec) &&
					(last_element < 4 * element_usec)) {
					update_tracking(element_usec, last_element);
				}
			}
			last_element = element_usec;
// ok... do we have a dit or a dah?
// a dot is anything shorter than 2 dot times
			if (element_usec <= two_dots) {
				rx_rep_buf += CW_DOT_REPRESENTATION;
//if (CW_DEBUG) printf("dot length: %d\n", last_element);
				cw_buffer[cw_ptr++] = (float)last_element;
			} else {
// a dash is anything longer than 2 dot times
//if (CW_DEBUG) printf("dash length: %d\n", last_element);
				rx_rep_buf += CW_DASH_REPRESENTATION;
				cw_buffer[cw_ptr++] = (float)last_element;
			}
// We just added a representation to the receive buffer.
// If it's full, then reset everything as it probably noise
			if (rx_rep_buf.length() > MAX_MORSE_ELEMENTS) {
				cw_receive_state = RS_IDLE;
				cw_rr_current = 0;	// reset decoding pointer
				cw_ptr = 0;
				smpl_ctr = 0;		// reset audio sample counter
				return;
			} else {
// zero terminate representation
//			rx_rep_buf.clear();
				cw_buffer[cw_ptr] = 0.0;
			}

// All is well.  Move to the more normal after-tone state.
			cw_receive_state = RS_AFTER_TONE;
			return;

// ---------
		case CW_QUERY_EVENT:
// this should be called quite often (faster than inter-character gap) It looks after timing
// key up intervals and determining when a character, a word space, or an error char '*' should be returned.
// SC_VALID is returned when there is a printable character. Nothing to do if we are in a tone
			if (cw_receive_state == RS_IN_TONE)
				return;
// compute length of silence so far
			sync_parameters();
			element_usec = usec_diff(cw_rr_end_timestamp, smpl_ctr);
// SHORT time since keyup... nothing to do yet
			if (element_usec < (2 * cw_receive_dot_length))
				return;
// MEDIUM time since keyup... check for character space
// one shot through this code via receive state logic
// FARNSWOTH MOD HERE -->
			if (element_usec >= (2 * cw_receive_dot_length) &&
				element_usec <= (4 * cw_receive_dot_length) &&
				cw_receive_state == RS_AFTER_TONE) {
// Look up the representation
//if (CW_DEBUG) printf("Decode buffer: %s [", rx_rep_buf.c_str());
				sc = morse->rx_lookup(rx_rep_buf);
				if (sc.empty()) {
// invalid decode... let user see error
					sc = (progdefaults.CW_noise == '*' ? "*" :
					progdefaults.CW_noise == '_' ? "_" :
					progdefaults.CW_noise == ' ' ? " " : "");
				}
				rx_rep_buf.clear();
				cw_receive_state = RS_IDLE;
				cw_rr_current = 0;	// reset decoding pointer
				space_sent = false;
				cw_ptr = 0;
//if (CW_DEBUG) printf("%s]\n", sc.c_str());
				return;;
			}
// LONG time since keyup... check for a word space
// FARNSWOTH MOD HERE -->
			if ((element_usec > (4 * cw_receive_dot_length)) && !space_sent) {
				sc = " ";
				space_sent = true;
//if (CW_DEBUG) printf("<SP>\n");
				return;;
			}
			return;

	}

	return;
}

//===========================================================================
// cw transmit routines
// Define the amplitude envelop for key down events (32 samples long)
// this is 1/2 cycle of a raised cosine
//===========================================================================

double keyshape[CWKNUM];
double QSKkeyshape[CWKNUM];

void cw::create_edges()
{
	for (int i = 0; i < CWKNUM; i++) keyshape[i] = 1.0;

	switch (QSKshape) {
		case 1: // blackman
			knum = (int)(risetime * CW_SAMPLERATE / 1000);
			if (knum >= symbollen) knum = symbollen;
			for (int i = 0; i < knum; i++)
				keyshape[i] = (0.42 - 0.50 * cos(M_PI * i/ knum) + 0.08 * cos(2 * M_PI * i / knum));
			break;
		case 0: // hanning
		default:
			knum = (int)(risetime * CW_SAMPLERATE / 1000);
			if (knum >= symbollen) knum = symbollen;
			for (int i = 0; i < knum; i++)
				keyshape[i] = 0.5 * (1.0 - cos (M_PI * i / knum));
	}

	for (int i = 0; i < CWKNUM; i++) QSKkeyshape[i] = 1.0;

	switch (QSKshape) {
		case 1: // blackman
			qnum = (int)(progdefaults.QSKrisetime * CW_SAMPLERATE / 1000);
			if (qnum >= symbollen) qnum = symbollen;
			for (int i = 0; i < qnum; i++)
				QSKkeyshape[i] = (0.42 - 0.50 * cos(M_PI * i/ qnum) + 0.08 * cos(2 * M_PI * i / qnum));
			break;
		case 0: // hanning
		default:
			qnum = (int)(progdefaults.QSKrisetime * CW_SAMPLERATE / 1000);
			if (qnum >= symbollen) qnum = symbollen;
			for (int i = 0; i < qnum; i++)
				QSKkeyshape[i] = 0.5 * (1.0 - cos (M_PI * i / qnum));
	}
}

inline double cw::nco(double freq)
{
	phaseacc += 2.0 * M_PI * freq / samplerate;
	if (phaseacc > TWOPI) phaseacc -= TWOPI;
	return sin(phaseacc);
}

inline double cw::qsknco()
{
	double amp;
	amp = sin(qskphase);
	qskphase += TWOPI * progdefaults.QSKfrequency / samplerate;
	if (qskphase > TWOPI) qskphase -= TWOPI;
	return amp;
}

//=====================================================================
// send_symbol()
// Sends a part of a morse character (one dot duration) of either
// sound at the correct freq or silence. Rise and fall time is controlled
// with a raised cosine shape.
//
// Left channel contains the shaped A2 CW waveform
// Right channel contains a square wave signal that is used
// to trigger a qsk switch.  Right channel has pre and post timings for
// proper switching of the qsk switch before and after the A2 element.
// If the Pre + Post timing exceeds the interelement spacing then the
// Pre and / or Post is only applied at the beginning and end of the
// character.
//=======================================================================

bool first_char = true;

enum {START, FIRST, MID, LAST, SPACE};

void cw::send_symbol(int bit, int len, int state)
{
	double qsk_amp = progdefaults.QSK ? progdefaults.QSKamp : 0.0;
	double xmt_freq = get_txfreq_woffset();

	if (CW_KEYLINE_isopen || 
		progdefaults.CW_KEYLINE_on_cat_port ||
		progdefaults.CW_KEYLINE_on_ptt_port)
		xmt_freq = progdefaults.CWsweetspot;

	sync_transmit_parameters();
	acc_symbols += len;

	memset(outbuf, 0, OUTBUFSIZE*sizeof(*outbuf));
	memset(qskbuf, 0, OUTBUFSIZE*sizeof(*qskbuf));

	if (bit == 1) { // keydown
		for (int n = 0; n < len; n++) {
			outbuf[n] = nco(xmt_freq);
			if (n < knum) outbuf[n] *= keyshape[n];
			if (len - n < knum) outbuf[n] *= keyshape[len - n];
			qskbuf[n] = qsk_amp * qsknco();
		}
	} else { // keyup
		for (int n = 0; n < len; n++) {
			outbuf[n] = 0;
			if (progdefaults.QSK) {
				qskbuf[n] = 0;
				if (state == START || state == FIRST) {
					qskbuf[n] = 0;
					if (n > len - kpre) {
						qskbuf[n] = qsk_amp * qsknco();
						if (n < len - kpre + qnum)
							qskbuf[n] *= QSKkeyshape[n - (len - kpre)];
					}
				} else if (state == MID) {
					qskbuf[n] = qsk_amp * qsknco();
					if (len > kpre + kpost) {
						if (n < kpost)
							qskbuf[n] *= QSKkeyshape[kpost - n];
						else if (n > len - kpre)
							qskbuf[n] *= QSKkeyshape[n - (len - kpre)];
						else qskbuf[n] = 0;
					}
				} else if (state == LAST) {
					qskbuf[n] = qsk_amp * qsknco();
					if (n > kpost - qnum)
						qskbuf[n] *= QSKkeyshape[kpost - n];
					if (n >= kpost) qskbuf[n] = 0;
				} else { // state == SPACE
					qskbuf[n] = 0;
				}
			}
		}
	}

	if (progdefaults.QSK)
		ModulateStereo(outbuf, qskbuf, len);
	else
		ModulateXmtr(outbuf, len);

}

//=====================================================================
// send_ch()
// sends a morse character and the space afterwards
//=======================================================================
void cw::send_ch(int ch)
{
	std::string code;

	float kfactor = CW_SAMPLERATE / 1000.0;
	float tc = 1200.0 / progdefaults.CWspeed;
	float ta = 0.0;
	float tch = 3 * tc, twd = 4 * tc;
	int   stdwpm = progdefaults.CWspeed;

	if (progdefaults.CWusefarnsworth && (stdwpm > progdefaults.CWfarnsworth)) {
		ta = 60000.0 / progdefaults.CWfarnsworth - 37200.0 / progdefaults.CWspeed;
		tch = 3 * ta / 19;
		twd = 4 * ta / 19;
		stdwpm = progdefaults.CWfarnsworth;
	}
	if (progdefaults.CWusewordsworth && (stdwpm > progdefaults.CWwordsworth)) {
		twd += (50.0 * 1200.0 / progdefaults.CWwordsworth - 50.0 * 1200.0 / stdwpm);
	}

	tc *= kfactor;
	tch *= kfactor;
	twd *= kfactor;

	sync_parameters();

	if (progdefaults.CWpre < progdefaults.QSKrisetime)
		kpre = progdefaults.QSKrisetime * kfactor;
	else
		kpre = progdefaults.CWpre * kfactor;

	if (progdefaults.CWpost < progdefaults.QSKrisetime)
		kpost = progdefaults.QSKrisetime * kfactor;
	else
		kpost = progdefaults.CWpost * kfactor;

	if ((ch == ' ') || (ch == '\n')) {
		while (twd > 0) {
			send_symbol(0, (twd < 4096 ? twd : 4096), SPACE);
			twd -= 4096;
		}
		put_echo_char(progdefaults.rx_lowercase ? tolower(ch) : ch);
		return;
	}

	code = morse->tx_lookup(ch);

	if (!code.length()) {
		return;
	}

	float w = (progdefaults.CWdash2dot + 1) / (progdefaults.CWdash2dot -1);
	float weight = progdefaults.CWweight;

	int elements = code.length();

	if (kpre)
		send_symbol(
			0,
			(first_char ? kpre :
				(kpre < 3 * tc - kpost) ? kpre : 
					3 * tc ),
			(first_char ? START : FIRST));

	for (int n = 0; n < elements; n++) {
		send_symbol(1, 
					(code[n] == '-' ? (w + 1) : (w - 1)) * (1 + weight) * symbollen,
					MID);
		send_symbol(0,
					((n < elements - 1) ? tc :
						(kpost + kpre < 3 * tc) ? tch - kpre:
							tch) * (1 - weight),
					(n < elements - 1 ? MID : LAST) );
	}

	if (ch != -1) {
		std::string prtstr = morse->tx_print();
		for (size_t n = 0; n < prtstr.length(); n++)
			put_echo_char(
				prtstr[n],
				prtstr[0] == '<' ? FTextBase::CTRL : FTextBase::XMIT);
	}
}

//=====================================================================
// cw_txprocess()
// Read characters from screen and send them out the sound card.
// This is called repeatedly from a thread during tx.
//=======================================================================
int cw::tx_process()
{
	int c = get_tx_char();
	if (c == GET_TX_CHAR_NODATA) {
		if (stopflag) {
			stopflag = false;
			put_echo_char('\n');
			first_char = true;
			return -1;
		}
		Fl::awake();
		MilliSleep(50);
		return 0;
	}

	if (progdefaults.use_FLRIGkeying) {
		if (c == GET_TX_CHAR_ETX || stopflag) {
			stopflag = false;
			put_echo_char('\n');
			return -1;
		}
		flrig_cwio_send(c);
		put_echo_char(c);
		return 0;
	}

	if (progStatus.WK_online) {
		if (c == GET_TX_CHAR_ETX || stopflag) {
			stopflag = false;
			put_echo_char('\n');
			return -1;
		}
		if (WK_send_char(c)) {
			put_echo_char('\n');
			return -1; // WinKeyer problem
		}
		return 0;
	}

	if (use_nanoIO) {
		if (c == GET_TX_CHAR_ETX || stopflag) {
			stopflag = false;
			put_echo_char('\n');
			return -1;
		}
		nano_send_char(c);
		put_echo_char(c);
		return 0;
	}

	if (progdefaults.use_ELCTkeying || progdefaults.use_KNWDkeying) {
		if (c == GET_TX_CHAR_ETX || stopflag) {
			stopflag = false;
			put_echo_char('\n');
			return -1;
		}
		KYkeyer_send_char(c);
		put_echo_char(c);
		return 0;
	}

	if (progdefaults.use_ICOMkeying) {
		if (c == GET_TX_CHAR_ETX || stopflag) {
			stopflag = false;
			put_echo_char('\n');
			return -1;
		}
		ICOMkeyer_send_char(c);
		put_echo_char(c);
		return 0;
	}

	if (progdefaults.use_YAESUkeying) {
		if (c == GET_TX_CHAR_ETX || stopflag) {
			stopflag = false;
			put_echo_char('\n');
			return -1;
		}
		FTkeyer_send_char(c);
		put_echo_char(c);
		return 0;
	}

	if (c == GET_TX_CHAR_ETX || stopflag) {
		stopflag = false;
		put_echo_char('\n');
		first_char = true;
		return -1;
	}

	acc_symbols = 0;

	if (CW_KEYLINE_isopen ||
		progdefaults.CW_KEYLINE_on_cat_port ||
		progdefaults.CW_KEYLINE_on_ptt_port)
		send_CW(c);

#if USE_LIBGPIOD
	if (progdefaults.gpio_cw_line != -1)
		send_gpio_CW(c);
#endif
	send_ch(c);

	first_char = false;

	char_samples = acc_symbols;

	return 0;
}

void cw::incWPM()
{

	if (usedefaultWPM) return;
	if (progdefaults.CWspeed < progdefaults.CWupperlimit) {
		progdefaults.CWspeed++;
		sync_parameters();
		set_CWwpm();
		update_Status();
	}
}

void cw::decWPM()
{

	if (usedefaultWPM) return;
	if (progdefaults.CWspeed > progdefaults.CWlowerlimit) {
		progdefaults.CWspeed--;
		set_CWwpm();
		sync_parameters();
		update_Status();
	}
}

void cw::toggleWPM()
{
	usedefaultWPM = !usedefaultWPM;
	if (usedefaultWPM) {
		wpm = progdefaults.CWspeed;
		progdefaults.CWspeed = progdefaults.defCWspeed;
	} else {
		progdefaults.CWspeed = wpm;
	}
	sync_parameters();
	update_Status();
}

// ---------------------------------------------------------------------
// CW output on DTR/RTS signal lines
//----------------------------------------------------------------------


void cw::send_CW(int) {}
