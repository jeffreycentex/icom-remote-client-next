// ----------------------------------------------------------------------------
// ifkp.cxx  --  ifkp modem
//
// Copyright (C) 2015
//		Dave Freese, W1HKJ
//
// This file is part of fldigi.
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

#include <config.h>

#include <stdlib.h>
#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <iomanip>
#include <cmath>
#include <libgen.h>

#include <FL/filename.H>
#include "progress.h"
#include "ifkp.h"
#include "complex.h"
#include "fl_digi.h"
#include "ascii.h"
#include "misc.h"
#include "fileselect.h"
#include "threads.h"
#include "debug.h"

#include "configuration.h"
#include "qrunner.h"
#include "fl_digi.h"
#include "status.h"
#include "main.h"
#include "icons.h"

#include "confdialog.h"

#include "test_signal.h"

int ifkp_varicode[][2] = {
	{ 0, 0}, { 0, 0}, { 0, 0}, { 0, 0}, { 0, 0}, { 0, 0}, { 0, 0}, { 0, 0},
	{27,31}, { 0, 0}, {28,30}, { 0, 0}, { 0, 0}, { 0, 0}, { 0, 0}, { 0, 0},
	{ 0, 0}, { 0, 0}, { 0, 0}, { 0, 0}, { 0, 0}, { 0, 0}, { 0, 0}, { 0, 0},
	{ 0, 0}, { 0, 0}, { 0, 0}, { 0, 0}, { 0, 0}, { 0, 0}, { 0, 0}, { 0, 0},
	{28, 0}, {11,30}, {12,30}, {13,30}, {14,30}, {15,30}, {16,30}, {17,30},//' '!-'
	{18,30}, {19,30}, {20,30}, {21,30}, {27,29}, {22,30}, {27, 0}, {23,30},
	{10,30}, { 1,30}, { 2,30}, { 3,30}, { 4,30}, { 5,30}, { 6,30}, { 7,30},// 0 - 7
	{ 8,30}, { 9,30}, {24,30}, {25,30}, {26,30}, { 0,31}, {27,30}, {28,29},// 8, 9
	{ 0,29}, { 1,29}, { 2,29}, { 3,29}, { 4,29}, { 5,29}, { 6,29}, { 7,29},
	{ 8,29}, { 9,29}, {10,29}, {11,29}, {12,29}, {13,29}, {14,29}, {15,29},
	{16,29}, {17,29}, {18,29}, {19,29}, {20,29}, {21,29}, {22,29}, {23,29}, // ... :
	{24,29}, {25,29}, {26,29}, { 1,31}, { 2,31}, { 3,31}, { 4,31}, { 5,31},
	{ 9,31}, { 1, 0}, { 2, 0}, { 3, 0}, { 4, 0}, { 5, 0}, { 6, 0}, { 7, 0},// @ - g
	{ 8, 0}, { 9, 0}, {10, 0}, {11, 0}, {12, 0}, {13, 0}, {14, 0}, {15, 0},// h - o
	{16, 0}, {17, 0}, {18, 0}, {19, 0}, {20, 0}, {21, 0}, {22, 0}, {23, 0},// p - w
	{24, 0}, {25, 0}, {26, 0}, { 6,31}, { 7,31}, { 8,31}, { 0,30}, {28,31},// x - 127
	{ 0, 0}, { 0, 0}, { 0, 0}, { 0, 0}, { 0, 0}, { 0, 0}, { 0, 0}, { 0, 0},//135
	{ 0, 0}, { 0, 0}, { 0, 0}, { 0, 0}, { 0, 0}, { 0, 0}, { 0, 0}, { 0, 0},//143
	{ 0, 0}, { 0, 0}, { 0, 0}, { 0, 0}, { 0, 0}, { 0, 0}, { 0, 0}, { 0, 0},//151
	{ 0, 0}, { 0, 0}, { 0, 0}, { 0, 0}, { 0, 0}, { 0, 0}, { 0, 0}, { 0, 0},//159
	{ 0, 0}, { 0, 0}, { 0, 0}, {14,31}, { 0, 0}, { 0, 0}, { 0, 0}, { 0, 0},//167
	{ 0, 0}, { 0, 0}, { 0, 0}, { 0, 0}, { 0, 0}, { 0, 0}, { 0, 0}, { 0, 0},//175
	{12,31}, {10,31}, { 0, 0}, { 0, 0}, { 0, 0}, { 0, 0}, { 0, 0}, { 0, 0},//183
	{ 0, 0}, { 0, 0}, { 0, 0}, { 0, 0}, { 0, 0}, { 0, 0}, { 0, 0}, { 0, 0},//191
	{ 0, 0}, { 0, 0}, { 0, 0}, { 0, 0}, { 0, 0}, { 0, 0}, { 0, 0}, { 0, 0},//199
	{ 0, 0}, { 0, 0}, { 0, 0}, { 0, 0}, { 0, 0}, { 0, 0}, { 0, 0}, { 0, 0},//207
	{ 0, 0}, { 0, 0}, { 0, 0}, { 0, 0}, { 0, 0}, { 0, 0}, { 0, 0}, {13,31},//215
	{ 0, 0}, { 0, 0}, { 0, 0}, { 0, 0}, { 0, 0}, { 0, 0}, { 0, 0}, { 0, 0},//223
	{ 0, 0}, { 0, 0}, { 0, 0}, { 0, 0}, { 0, 0}, { 0, 0}, { 0, 0}, { 0, 0},//231
	{ 0, 0}, { 0, 0}, { 0, 0}, { 0, 0}, { 0, 0}, { 0, 0}, { 0, 0}, { 0, 0},//239
	{ 0, 0}, { 0, 0}, { 0, 0}, { 0, 0}, { 0, 0}, { 0, 0}, { 0, 0}, {11,31},//247
	{12,31}, { 0, 0}, { 0, 0}, { 0, 0}, { 0, 0}, { 0, 0}, { 0, 0}, { 0, 0} //255
};

/*
 * The same in a format more suitable for decoding.
 * The index is the varicode symbol, being 1-2 nibbles. The value is the
 * ASCII character.
 * NB. Do NOT use CR as a line feed, as you will get an extra space and the 
 * new line will be indented. Use LF = ascii 10 instead.
 */
// decode [ prev * 32 + curr] ; 0 < prev < 29; curr == 29, 30, 31
int ifkp_varidecode[] = {
//  0   1   2   3   4   5   6   7   8   9   10  11  12  13  14  15  16  17  18  19  20  21  22  23  24  25  26  27  28  29  30  31
    00, 97, 98, 99,100,101,102,103,104,105,106,107,108,109,110,111,112,113,114,115,116,117,118,119,120,121,122, 46, 32, 64,126, 61, //  0 - 31
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 65, 49, 91, // 32 - 63
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 66, 50, 92, // 2
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 67, 51, 93, // 3
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 68, 52, 94, // 4
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 69, 53, 95, // 5
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 70, 54,123, // 6
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 71, 55,124, // 7
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 72, 56,125, // 8
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 73, 57, 96, // 9
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 74, 48,177, // 10
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 75, 33,247, // 11
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 76, 34,176, // 12
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 77, 35,215, // 13
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 78, 36,163, // 14
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 79, 37, -1, // 15
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 80, 38, -1, // 16
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 81, 39, -1, // 17
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 82, 40, -1, // 18
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 83, 41, -1, // 19
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 84, 42, -1, // 20
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 85, 43, -1, // 21
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 86, 45, -1, // 22
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 87, 47, -1, // 23
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 88, 58, -1, // 24
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 89, 59, -1, // 25
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 90, 60, -1, // 26
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 44, 62,  8, // 27
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 63, 10,127, // 28
};

const char *ifkp_ascii[256] = {
	"<NUL>", "<SOH>", "<STX>", "<ETX>", "<EOT>", "<ENQ>", "<ACK>", "<BEL>",
	"<BS>",  "<TAB>", "<LF>",  "<VT>", "<FF>",  "<CR>",  "<SO>",  "<SI>",
	"<DLE>", "<DC1>", "<DC2>", "<DC3>", "<DC4>", "<NAK>", "<SYN>", "<ETB>",
	"<CAN>", "<EM>",  "<SUB>", "<ESC>", "<FS>",  "<GS>",  "<RS>",  "<US>",
	" ",     "!",     "\"",    "#",     "$",     "%",     "&",     "\'",
	"(",     ")",     "*",     "+",     ",",     "-",     ".",     "/",
	"0",     "1",     "2",     "3",     "4",     "5",     "6",     "7",
	"8",     "9",     ":",     ";",     "<",     "=",     ">",     "?",
	"@",     "A",     "B",     "C",     "D",     "E",     "F",     "G",
	"H",     "I",     "J",     "K",     "L",     "M",     "N",     "O",
	"P",     "Q",     "R",     "S",     "T",     "U",     "V",     "W",
	"X",     "Y",     "Z",     "[",     "\\",    "]",     "^",     "_",
	"`",     "a",     "b",     "c",     "d",     "e",     "f",     "g",
	"h",     "i",     "j",     "k",     "l",     "m",     "n",     "o",
	"p",     "q",     "r",     "s",     "t",     "u",     "v",     "w",
	"x",     "y",     "z",     "{",     "|",     "}",     "~",     "<DEL>",
	"<128>", "<129>", "<130>", "<131>", "<132>", "<133>", "<134>", "<135>",
	"<136>", "<137>", "<138>", "<139>", "<140>", "<141>", "<142>", "<143>",
	"<144>", "<145>", "<146>", "<147>", "<148>", "<149>", "<150>", "<151>",
	"<152>", "<153>", "<154>", "<155>", "<156>", "<157>", "<158>", "<159>",
	"<160>", "<161>", "<162>", "£",     "<164>", "<165>", "<166>", "<167>",
	"<168>", "<169>", "<170>", "<171>", "<172>", "<173>", "<174>", "<175>",
	"°",     "±",     "<178>", "<179>", "<180>", "<181>", "<182>", "<183>",
	"<184>", "<185>", "<186>", "<187>", "<188>", "<189>", "<190>", "<191>",
	"<192>", "<193>", "<194>", "<195>", "<196>", "<197>", "<198>", "<199>",
	"<200>", "<201>", "<202>", "<203>", "<204>", "<205>", "<206>", "<207>",
	"<208>", "<209>", "<210>", "<211>", "<212>", "<213>", "<214>", "×",
	"<216>", "<217>", "<218>", "<219>", "<220>", "<221>", "<222>", "<223>",
	"<224>", "<225>", "<226>", "<227>", "<228>", "<229>", "<230>", "<231>",
	"<232>", "<233>", "<234>", "<235>", "<236>", "<237>", "<238>", "<239>",
	"<240>", "<241>", "<242>", "<243>", "<244>", "<245>", "<246>", "÷",
	"<248>", "<249>", "<250>", "<251>", "<252>", "<253>", "<254>", "<255>" 
};


#define IFKP_SR 16000

#include "web_picture.h"

static fre_t call("([[:alnum:]]?[[:alpha:]/]+[[:digit:]]+[[:alnum:]/]+)", REG_EXTENDED);
static std::string teststr = "";
static std::string allowed = " 0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ/";
static char sz[21];

static bool enable_audit_log = false;
static bool enable_heard_log = false;

int ifkp::IMAGEspp = IMAGESPP;

static std::string valid_callsign(char ch)
{
	if (allowed.find(ch) == std::string::npos) ch = ' ';
	teststr += tolower(ch);
	if (teststr.length() > 20) teststr.erase(0,1);
	// wait for ' de '
	size_t p1;
	if ((p1 = teststr.find(" de ")) != std::string::npos) { // test for callsign
		p1 += 4;
		if (p1 >= teststr.length()) return "";
		while (p1 < teststr.length() && teststr[p1] == ' ') p1++;
		if (p1 == teststr.length()) return "";
		size_t p2 = teststr.rfind(' ');
		if ((p2 > p1) && (p2 - p1 < 21)) { // found a word, test for callsign
			memset(sz, 0, 21);
			strcpy(sz, teststr.substr(p1, p2-p1).c_str());
			teststr.erase(0, p2);
			if (call.match(sz)) {
				return sz;
			}
			return "";
		}
	}
	return "";
}

// nibbles table used for fast conversion from tone difference to symbol

void ifkp::init_nibbles()
{
	int nibble = 0;
	for (int i = 0; i < 199; i++) {
		nibble = floor(0.5 + (i - 99.0)/IFKP_SPACING);
		// allow for wrap-around (33 tones for 32 tone differences)
		if (nibble < 0) nibble += 33;
		if (nibble > 32) nibble -= 33;
		// adjust for +1 symbol at the transmitter
		nibble--;
		nibbles[i] = nibble;
	}
}

ifkp::ifkp(trx_mode md) : modem()
{
	samplerate = IFKP_SR;
	symlen = IFKP_SYMLEN;

	cap |= CAP_IMG;

	if (progdefaults.StartAtSweetSpot) {
		frequency = progdefaults.PSKsweetspot;
	} else
		frequency = wf->Carrier();
	REQ(put_freq, frequency);

	mode = md;
	fft = new g_fft<double>(IFKP_FFTSIZE);
	snfilt = new Cmovavg(32);
	movavg_size = 4;
	for (int i = 0; i < IFKP_NUMBINS; i++) binfilt[i] = new Cmovavg(movavg_size);
	txphase = 0;
	basetone = 197;

	float lo = frequency - 0.75 * bandwidth;
	float hi = frequency + 0.75 * bandwidth;

	rxfilter = new C_FIR_filter();
	rxfilter->init_bandpass(129, 1, lo/samplerate, hi/samplerate);

	picfilter = new C_FIR_filter();
	picfilter->init_lowpass(129, 1, 2.0 * bandwidth / samplerate);

	phase = 0;
	phidiff = 2.0 * M_PI * frequency / samplerate;

	IMAGEspp = IMAGESPP;
	pixfilter = new Cmovavg(IMAGEspp / 2);
	ampfilter = new Cmovavg(IMAGEspp);
	syncfilter = new Cmovavg(IMAGEspp / 2);

	bkptr = 0;
	peak_counter = 0;
	peak = last_peak = 0;
	max = 0;
	curr_nibble = prev_nibble = 0;
	s2n = 0;

	memset(rx_stream, 0, sizeof(rx_stream));
	rx_text.clear();

	for (int i = 0; i < IFKP_BLOCK_SIZE; i++)
		a_blackman[i] = blackman(1.0 * i / IFKP_BLOCK_SIZE);

	state = TEXT;

	init_nibbles();

	TX_IMAGE = TX_AVATAR = false;

	restart();

	toggle_logs();

	activate_ifkp_image_item(true);

}

ifkp::~ifkp()
{
	delete fft;
	delete snfilt;
	delete rxfilter;
	delete picfilter;
	for (int i = 0; i < IFKP_NUMBINS; i++)
		delete binfilt[i];
	ifkp_deleteTxViewer();
	ifkp_deleteRxViewer();
	heard_log.close();
	audit_log.close();

	activate_ifkp_image_item(false);

}

void  ifkp::tx_init()
{
	tone = prevtone = 0;
	txphase = 0;
	send_bot = true;
	mycall = progdefaults.myCall;
	if (progdefaults.ifkp_lowercase)
		for (size_t n = 0; n < mycall.length(); n++) mycall[n] = tolower(mycall[n]);
	videoText();
}

void  ifkp::rx_init()
{
	bkptr = 0;
	peak_counter = 0;
	peak = last_peak = 0;
	max = 0;
	curr_nibble = prev_nibble = 0;
	s2n = 0;

	memset(rx_stream, 0, sizeof(rx_stream));

	prevz = cmplx(0,0);
	image_counter = 0;
	state = TEXT;

	rx_text.clear();
	for (int i = 0; i < IFKP_NUMBINS; i++) {
		tones[i] = 0.0;
		binfilt[i]->reset();
	}
	pixel = 0;
	pic_str = "     ";
}

void ifkp::rx_reset()
{
	prevz = cmplx(0,0);
	image_counter = 0;
	pixel = 0;
	pic_str = "     ";
	snfilt->reset();
	state = TEXT;
}

void ifkp::init()
{
	peak_hits = 4;

	mycall = progdefaults.myCall;
	if (progdefaults.ifkp_lowercase)
		for (size_t n = 0; n < mycall.length(); n++) mycall[n] = tolower(mycall[n]);

	movavg_size = 3;

	for (int i = 0; i < IFKP_NUMBINS; i++) binfilt[i]->setLength(movavg_size);

	rx_init();
}

void ifkp::set_freq(double f)
{
	if (progdefaults.ifkp_freqlock)
		frequency = 1500;
	else
		frequency = f;

	if (frequency < 100 + 0.5 * bandwidth) frequency = 100 + 0.5 * bandwidth;
	if (frequency > 3900 - 0.5 * bandwidth) frequency = 3900 - 0.5 * bandwidth;

	tx_frequency = frequency;

	REQ(put_freq, frequency);

	set_bandwidth(33 * IFKP_SPACING * samplerate / symlen);

	basetone = ceil((frequency - bandwidth / 2.0) * symlen / samplerate);

	float lo = frequency - 0.75 * bandwidth;
	float hi = frequency + 0.75 * bandwidth;

	rxfilter->init_bandpass(129, 1, lo/samplerate, hi/samplerate);
	picfilter->init_lowpass(129, 1, 2.0 * bandwidth / samplerate);

	phase = 0;
	phidiff = 2.0 * M_PI * frequency / samplerate;

	std::ostringstream it;
	it << "\nSamplerate..... " << samplerate;
	it << "\nBandwidth...... " << bandwidth;
	it << "\nlower cutoff... " << lo;
	it << "\nupper cutoff... " << hi;
	it << "\ncenter ........ " << frequency;
	it << "\nSymbol length.. " << symlen    << "\nBlock size..... " << IFKP_SHIFT_SIZE;
	it << "\nMinimum Hits... " << peak_hits << "\nBasetone....... " << basetone << "\n";

	LOG_VERBOSE("%s", it.str().c_str());
}

void ifkp::show_mode()
{
	if (progdefaults.ifkp_baud == 0)
		put_MODEstatus("IFKP 0.5");
	else if (progdefaults.ifkp_baud == 1)
		put_MODEstatus("IFKP 1.0");
	else
		put_MODEstatus("IFKP 2.0");
	return;
}

void ifkp::toggle_logs()
{
	if (enable_audit_log != progdefaults.ifkp_enable_audit_log){
		enable_audit_log = progdefaults.ifkp_enable_audit_log;
		if (audit_log.is_open()) audit_log.close();
	}
	if (enable_heard_log != progdefaults.ifkp_enable_heard_log) {
		enable_heard_log = progdefaults.ifkp_enable_heard_log;
		if (heard_log.is_open()) heard_log.close();
	}

	if (enable_heard_log) {
		heard_log_fname = progdefaults.ifkp_heard_log;
		std::string sheard = TempDir;
		sheard.append(heard_log_fname);
		heard_log.open(sheard.c_str(), std::ios::app);

		heard_log << "==================================================\n";
		heard_log << "Heard log: " << zdate() << ", " << ztime() << "\n";
		heard_log << "==================================================\n";
		heard_log.flush();
	}

	if (enable_audit_log) {
		audit_log_fname = progdefaults.ifkp_audit_log;
		std::string saudit = TempDir;
		saudit.append(audit_log_fname);
		audit_log.close();
		audit_log.open(saudit.c_str(), std::ios::app);

		audit_log << "==================================================\n";
		audit_log << "Audit log: " << zdate() << ", " << ztime() << "\n";
		audit_log << "==================================================\n";
		audit_log.flush();
	}

}

void ifkp::restart()
{
	set_freq(wf->Carrier());

	peak_hits = 4;

	mycall = progdefaults.myCall;
	if (progdefaults.ifkp_lowercase)
		for (size_t n = 0; n < mycall.length(); n++) mycall[n] = tolower(mycall[n]);

	movavg_size = progdefaults.ifkp_baud == 2 ? 3 : 4;

	for (int i = 0; i < IFKP_NUMBINS; i++) binfilt[i]->setLength(movavg_size);

	show_mode();

}

// valid printable character

bool ifkp::valid_char(int ch)
{
	if ( ! (ch ==  10 || ch == 163 || ch == 176 ||
		ch == 177 || ch == 215 || ch == 247 ||
		(ch > 31 && ch < 128)))
		return false;
	return true;
}

//=====================================================================
// receive processing
//=====================================================================

void ifkp::parse_pic(int ch)
{
	b_ava = false;
	image_mode = 0;

	pic_str.erase(0,1);
	pic_str += ch;

	if (pic_str.find("pic%") == 0) {
		switch (pic_str[4]) {
			case 'A':	picW = 59; picH = 74; b_ava = true; break;
			case 'T':	picW = 59; picH = 74; break;
			case 't':	picW = 59; picH = 74; image_mode = 1; break;
			case 'S':	picW = 160; picH = 120; break;
			case 's':	picW = 160; picH = 120; image_mode = 1; break;
			case 'L':	picW = 320; picH = 240; break;
			case 'l':	picW = 320; picH = 240; image_mode = 1; break;
			case 'V':	picW = 640; picH = 480; break;
			case 'v':	picW = 640; picH = 480; image_mode = 1; break;
			case 'F':	picW = 640; picH = 480; image_mode = 1; break;
			case 'P':	picW = 240; picH = 300; break;
			case 'p':	picW = 240; picH = 300; image_mode = 1; break;
			case 'M':	picW = 120; picH = 150; break;
			case 'm':	picW = 120; picH = 150; image_mode = 1; break;
			default: 
				syncfilter->reset();
				pixfilter->reset();
				ampfilter->reset();
				return;
		}
	} else
		return;

	if (!b_ava)
		ifkp_showRxViewer( pic_str[4]);
	else
		ifkp_clear_avatar();

	image_counter = -symlen / 2;
	col = row = rgb = 0;
	syncfilter->reset();
	pixfilter->reset();
//	ampfilter->reset();
	state = IMAGE_START;
}

void ifkp::process_symbol(int sym)
{
	int nibble = 0;
	int curr_ch = -1	;

	symbol = sym;

	nibble = symbol - prev_symbol;
	if (nibble < -99 || nibble > 99) {
		prev_symbol = symbol;
		return;
	}
	nibble = nibbles[nibble + 99];

	if (nibble >= 0) { // process nibble
		curr_nibble = nibble;

// single-nibble characters
		if ((prev_nibble < 29) & (curr_nibble < 29)) {
			curr_ch = ifkp_varidecode[prev_nibble];

// double-nibble characters
		} else if ( (prev_nibble < 29) &&
					 (curr_nibble > 28) &&
					 (curr_nibble < 32)) {
			curr_ch = ifkp_varidecode[prev_nibble * 32 + curr_nibble];
		}
		if (curr_ch > 0) {
			if (ch_sqlch_open || metric >= progStatus.sldrSquelchValue) {
//			if (metric >= progStatus.sldrSquelchValue) {
				put_rx_char(curr_ch, FTextBase::RECV);
				if (enable_audit_log) {
					audit_log << ifkp_ascii[curr_ch];
					if (curr_ch == '\n') audit_log << '\n';
				}
				parse_pic(curr_ch);
				if (state != IMAGE_START) {
				station_calling = valid_callsign(curr_ch);
				if (!station_calling.empty()) {
					snprintf(szestimate, sizeof(szestimate), "%.0f db", s2n );
					REQ(add_to_heard_list, station_calling.c_str(), szestimate);
					if (enable_heard_log) {
						std::string sheard = zdate();
						sheard.append(":").append(ztime());
						sheard.append(",").append(station_calling);
						sheard.append(",").append(szestimate).append("\n");
						heard_log << sheard;
						heard_log.flush();
					}
				}
			}
			}
		}
		prev_nibble = curr_nibble;
	}

	prev_symbol = symbol;
}

int no_signal = 0;

void ifkp::process_tones()
{
	max = 0;
	peak = 0;

	max = 0;
	peak = IFKP_NUMBINS / 2;

	int firstbin = frequency * IFKP_SYMLEN / samplerate - IFKP_NUMBINS / 2;
	double sigval = 0;
	double min = 3.0e8;

	for (int i = 0; i < IFKP_NUMBINS; ++i) {
		val = norm(fft_data[i + firstbin]);
// search for minimum and maximum signalx
		tones[i] = binfilt[i]->run(val);
		if (tones[i] > max) {
			max = tones[i];
			peak = i;
		}
		if (tones[i] < min) {
			min = tones[i];
		}
	}

	if (peak == prev_peak) {
		peak_counter++;
	} else {
		peak_counter = 0;
	}

	if ((peak_counter >= peak_hits) &&
		(peak != last_peak)) {
		sigval = (tones[peak-1] + tones[peak] + tones[peak+1]);
		if (min == 0) min = max / 1000;
		if (min > 0 && max > 0) {
			s2n = 20 * log10( snfilt->run(sigval/min));
			s2n -= 64;
			s2n *= 40;
			s2n /= 75;
			s2n -= 20;
		} else
			s2n = -25;

//scale to -25 to +45 db range
// -25 -> 0 linear
// 0 - > 45 compressed by 2

		if (s2n <= 0) metric = 2 * (25 + s2n);
		if (s2n > 0) metric = 50 * ( 1 + s2n / 45);
			metric = clamp(metric, 0, 100);

		display_metric(metric);

		if (metric >= progStatus.sldrSquelchValue ||
			progStatus.sqlonoff == false) { //) {
			process_symbol(peak);
		}
		peak_counter = 0;
		last_peak = peak;
		no_signal = 0;
	} else if (++no_signal > 128) { // reset the filter and zero the s/n display
		snfilt->reset();
		display_metric(0);
	}

	prev_peak = peak;
}

void ifkp::recvpic(double smpl)
{
	phidiff = 2.0 * M_PI * frequency / samplerate;
	phase -= phidiff;
	if (phase < 0) phase += 2.0 * M_PI;

	cmplx z = smpl * cmplx( cos(phase), sin(phase ) );
	picfilter->run( z, currz);
	pixel = (samplerate / TWOPI) * pixfilter->run(arg(conj(prevz) * currz));
	sync = (samplerate / TWOPI) * syncfilter->run(arg(conj(prevz) * currz));

	prevz = currz;

	image_counter++;

	if (image_counter < 0) return;

	if (state == IMAGE_START) {
		if (sync < -0.59 * bandwidth) {
			state = IMAGE_SYNC;
				}
		return;
	}
	if (state == IMAGE_SYNC) {
		if (sync > -0.51 * bandwidth) {
			state = IMAGE;
		}
		return;
	}

	if ((image_counter % IMAGEspp) == 0) {
		byte = pixel * 256.0 / bandwidth + 128;
		byte = (int)CLAMP( byte, 0.0, 255.0);

		if (image_mode == 1) { // bw transmission
			pixelnbr = 3 * (col + row * picW);
			if (b_ava) {
				ifkp_update_avatar( byte, pixelnbr);
				ifkp_update_avatar( byte, pixelnbr + 1);
				ifkp_update_avatar( byte, pixelnbr + 2);
			} else {
				ifkp_updateRxPic( byte, pixelnbr);
				ifkp_updateRxPic( byte, pixelnbr + 1);
				ifkp_updateRxPic( byte, pixelnbr + 2);
			}
			if (++ col == picW) {
				col = 0;
				row++;
				if (row >= picH) {
					rx_reset();
					REQ(ifkp_enableshift);
				}
			}
		} else { // color transmission
			pixelnbr = rgb + 3 * (col + row * picW);
			if (b_ava)
				ifkp_update_avatar( byte, pixelnbr);
			else
				ifkp_updateRxPic( byte, pixelnbr);
			if (++col == picW) {
				col = 0;
				if (++rgb == 3) {
					rgb = 0;
					++row;
				}
			}
			if (row >= picH) {
				rx_reset();
				REQ(ifkp_enableshift);
			}
		}
	}
}

int ifkp::rx_process(const double *buf, int len)
{
	double val;
	cmplx zin, z;

	if (enable_audit_log != progdefaults.ifkp_enable_audit_log ||
		enable_heard_log != progdefaults.ifkp_enable_heard_log)
		toggle_logs();

	if (bkptr < 0) bkptr = 0;
	if (bkptr >= IFKP_SHIFT_SIZE) bkptr = 0;

	if (progStatus.ifkp_rx_abort) {
		state = TEXT;
		progStatus.ifkp_rx_abort = false;
		REQ(ifkp_clear_rximage);
	}

	while (len) {
		if (state == TEXT) {
			rxfilter->Irun(*buf, val);
			rx_stream[IFKP_BLOCK_SIZE + bkptr] = val;
			bkptr++;

			if (bkptr == IFKP_SHIFT_SIZE) {
				bkptr = 0;
				memmove(rx_stream,								// to
						&rx_stream[IFKP_SHIFT_SIZE],			// from
						IFKP_BLOCK_SIZE*sizeof(*rx_stream));	// # bytes

				for (int n = 0; n < 2*IFKP_FFTSIZE; n++)
					fft_data[n] = cmplx(0,0);

				for (int i = 0; i < IFKP_BLOCK_SIZE; i++) {
					double d = rx_stream[i] * a_blackman[i];
					fft_data[i] = cmplx(d,d);
				}
				fft->ComplexFFT(fft_data);
				process_tones();
			}
		} else
			recvpic(*buf);

		len--;
		buf++;
	}
	return 0;
}

//=====================================================================
// transmit processing
//=====================================================================

void ifkp::transmit(double *buf, int len)
{
//	if (xmtfilt && progdefaults.ifkp_xmtfilter)
//		for (int i = 0; i < len; i++) xmtfilt->Irun(buf[i], buf[i]);
	ModulateXmtr(buf, len);
}

void ifkp::send_tone(int tone)
{
	double phaseincr;
	double frequency;

	frequency = (basetone + tone * IFKP_SPACING) * samplerate / symlen - progdefaults.TxOffset;
	if (test_signal_window && test_signal_window->visible() && btnOffsetOn->value())
		frequency += ctrl_freq_offset->value();

	phaseincr = 2.0 * M_PI * frequency / samplerate;
	prevtone = tone;

	int send_symlen = symlen * (
			progdefaults.ifkp_baud == 2 ? 0.5 :
			progdefaults.ifkp_baud == 0 ? 2.0 : 1.0);

	for (int i = 0; i < send_symlen; i++) {
		outbuf[i] = cos(txphase);
		txphase -= phaseincr;
		if (txphase < 0) txphase += TWOPI;
	}
	transmit(outbuf, send_symlen);
}

void ifkp::send_symbol(int sym)
{
	tone = (prevtone + sym + IFKP_OFFSET) % 33;
	send_tone(tone);
}

void ifkp::send_idle()
{
	send_symbol(0);
}

void ifkp::send_char(int ch)
{
	if (ch <= 0) return send_idle();

	int sym1 = ifkp_varicode[ch][0];
	int sym2 = ifkp_varicode[ch][1];

	send_symbol(sym1);
	if (sym2 > 28)
		send_symbol(sym2);
	put_echo_char(ch);
}

void ifkp::send_avatar()
{
	int W = 59, H = 74;  // grey scale transfer (FAX)
	float freq, phaseincr;
	float radians = 2.0 * M_PI / samplerate;

	freq = frequency - 0.6 * bandwidth;
	#define PHASE_CORR  (3 * symlen / 2)
	phaseincr = radians * freq;
	for (int n = 0; n < PHASE_CORR; n++) {
		outbuf[n] = cos(txphase);
		txphase -= phaseincr;
		if (txphase < 0) txphase += TWOPI;
	}
	transmit(outbuf, PHASE_CORR);

	for (int row = 0; row < H; row++) {
		for (int color = 0; color < 3; color++) {
			memset(outbuf, 0, IMAGEspp * sizeof(*outbuf));
			for (int col = 0; col < W; col++) {
				if (stopflag) return;
				tx_pixelnbr = col + row * W;
				tx_pixel = ifkp_get_avatar_pixel(tx_pixelnbr, color);
				freq = frequency + (tx_pixel - 128) * bandwidth / 256.0;
				phaseincr = radians * freq;
				for (int n = 0; n < IMAGEspp; n++) {
					outbuf[n] = cos(txphase);
					txphase -= phaseincr;
					if (txphase < 0) txphase += TWOPI;
				}
				transmit(outbuf, IMAGEspp);
			}
			Fl::awake();
		}
	}
}

static bool send_color = true;

void ifkp::send_image()
{
	int W = 640, H = 480;  // grey scale transfer (FAX)
	float freq, phaseincr;
	float radians = 2.0 * M_PI / samplerate;

	if (!ifkppicTxWin || !ifkppicTxWin->visible()) {
		return;
	}

	switch (selifkppicSize->value()) {
		case 0 : W = 59; H = 74; break;
		case 1 : W = 120; H = 150; break;
		case 2 : W = 240; H = 300; break;
		case 3 : W = 160; H = 120; break;
		case 4 : W = 320; H = 240; break;
		case 5 : W = 640; H = 480; break;
	}


if (web_image_position == 0) {
	REQ(ifkp_clear_tximage);

	stop_deadman();

	for (size_t n = 0; n < imageheader.length(); n++)
		send_char(imageheader[n]);
	send_char(0); // needed to flush the header at the Rx decoder

	freq = get_txfreq_woffset() - 0.6 * bandwidth; // black-black
	phaseincr = radians * freq;

	for (int j = 0; j < 7; j++) {
		for (int i = 0; i < symlen/2; i++) {
			outbuf[i] = cos(txphase);
			txphase -= phaseincr;
			if (txphase < 0) txphase += TWOPI;
		}
		transmit(outbuf, symlen/2);
	}

	
}

    const bool gray = !send_color;
    const int channels = gray ? 1 : 3;
    const int total = W * H * channels;
    const int limit = std::min(total, web_image_position + 128);
    for (; web_image_position < limit; ++web_image_position) {
        int row = web_image_position / (W * channels);
        int col = web_image_position % W;
        int color = gray ? 0 : (web_image_position / W) % 3;
        tx_pixelnbr = col + row * W;
					tx_pixel = gray ?
            0.3 * ifkppic_TxGetPixel(tx_pixelnbr, 0) +
            0.6 * ifkppic_TxGetPixel(tx_pixelnbr, 1) +
            0.1 * ifkppic_TxGetPixel(tx_pixelnbr, 2) :
            ifkppic_TxGetPixel(tx_pixelnbr, color);
					REQ(ifkp_updateTxPic, tx_pixel, tx_pixelnbr*3 + color);
					freq = get_txfreq_woffset() + (tx_pixel - 128) * bandwidth / 256.0;
					phaseincr = radians * freq;
					for (int n = 0; n < IMAGEspp; n++) {
						outbuf[n] = cos(txphase);
						txphase -= phaseincr;
						if (txphase < 0) txphase += TWOPI;
					}
					transmit(outbuf, IMAGEspp);
    }
    web_image_done = web_image_position == total;
    if (web_image_done) start_deadman();
}

std::string img_str;

void ifkp::ifkp_send_image(std::string image_str, bool gray) {
	send_color = !gray;
	imageheader = image_str;
	TX_IMAGE = true;
	start_tx();
}

void ifkp::ifkp_send_avatar() {
	img_str = "\npic%A";
	TX_AVATAR = true;
	start_tx();
}


void ifkp::m_ifkp_send_avatar() {}
int ifkp::tx_process()
{
	modem::tx_process();

	if (send_bot) {
		send_bot = false;
		send_char(0);
		send_char(0);
	}

	int c = get_tx_char();

//	if (c == GET_TX_CHAR_ETX || enable_image) {
	if (TX_IMAGE || TX_AVATAR) {
		if (img_str.length()) {
			for (size_t n = 0; n < img_str.length(); n++)
				send_char(img_str[n]);
		}
		img_str.clear();

		if (TX_IMAGE) {
			stopflag = false;
			send_image();
			if (!web_image_done) return 0;
			ifkppicTxWin->hide();
		}
		if (TX_AVATAR) {
			stopflag = false;
			send_avatar();
		}

		send_char(0);

		TX_IMAGE = false;
		TX_AVATAR = false;
		ifkppicTxWin->hide();

		return 0;

	}
	if ( stopflag || c == GET_TX_CHAR_ETX) { // aborts transmission
		send_char(0);
//		TX_IMAGE = false;
//		TX_AVATAR = false;
		stopflag = false;
		return -1;
	}
	send_char(c);
	return 0;
}


std::string ifkp::imageheader;
