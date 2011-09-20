
#line 1 "../src/xml.ragel"
///////////////////////////////////////////////////////////////////////////////////
//
// Copyright (C) 2011 Rick Taylor
//
// This file is part of OOXML, the Omega Online XML library.
//
// OOXML is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// OOXML is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with OOXML.  If not, see <http://www.gnu.org/licenses/>.
//
///////////////////////////////////////////////////////////////////////////////////


#line 120 "../src/xml.ragel"


#include "../src/Tokenizer.h"


#line 32 "src/xml.cpp"
static const int xml_start = 1;
static const int xml_first_final = 444;
static const int xml_error = 0;

static const int xml_en_ch_or_seq1 = 378;
static const int xml_en_content_1 = 390;
static const int xml_en_main = 1;


#line 125 "../src/xml.ragel"

void Tokenizer::do_init()
{
	
#line 47 "src/xml.cpp"
	{
	 this->m_cs = xml_start;
	 this->m_top = 0;
	}

#line 129 "../src/xml.ragel"
}

bool Tokenizer::do_exec()
{
	static const unsigned char pe = '\0';
	Tokenizer& p = *this;
	
	//while (p != pe && m_cs != %%{ write error; }%% )
	{
		
#line 64 "src/xml.cpp"
	{
	if ( p == pe )
		goto _test_eof;
	goto _resume;

_again:
	switch (  this->m_cs ) {
		case 1: goto st1;
		case 0: goto st0;
		case 2: goto st2;
		case 3: goto st3;
		case 4: goto st4;
		case 5: goto st5;
		case 6: goto st6;
		case 7: goto st7;
		case 8: goto st8;
		case 9: goto st9;
		case 10: goto st10;
		case 11: goto st11;
		case 12: goto st12;
		case 13: goto st13;
		case 14: goto st14;
		case 15: goto st15;
		case 16: goto st16;
		case 17: goto st17;
		case 18: goto st18;
		case 19: goto st19;
		case 20: goto st20;
		case 21: goto st21;
		case 22: goto st22;
		case 23: goto st23;
		case 24: goto st24;
		case 25: goto st25;
		case 26: goto st26;
		case 27: goto st27;
		case 28: goto st28;
		case 444: goto st444;
		case 29: goto st29;
		case 30: goto st30;
		case 31: goto st31;
		case 32: goto st32;
		case 33: goto st33;
		case 34: goto st34;
		case 35: goto st35;
		case 36: goto st36;
		case 37: goto st37;
		case 38: goto st38;
		case 39: goto st39;
		case 40: goto st40;
		case 41: goto st41;
		case 42: goto st42;
		case 43: goto st43;
		case 44: goto st44;
		case 45: goto st45;
		case 46: goto st46;
		case 47: goto st47;
		case 48: goto st48;
		case 49: goto st49;
		case 50: goto st50;
		case 51: goto st51;
		case 52: goto st52;
		case 53: goto st53;
		case 54: goto st54;
		case 55: goto st55;
		case 56: goto st56;
		case 57: goto st57;
		case 58: goto st58;
		case 59: goto st59;
		case 60: goto st60;
		case 61: goto st61;
		case 62: goto st62;
		case 63: goto st63;
		case 64: goto st64;
		case 65: goto st65;
		case 66: goto st66;
		case 67: goto st67;
		case 68: goto st68;
		case 69: goto st69;
		case 70: goto st70;
		case 71: goto st71;
		case 72: goto st72;
		case 73: goto st73;
		case 74: goto st74;
		case 75: goto st75;
		case 76: goto st76;
		case 77: goto st77;
		case 78: goto st78;
		case 79: goto st79;
		case 80: goto st80;
		case 81: goto st81;
		case 82: goto st82;
		case 83: goto st83;
		case 84: goto st84;
		case 85: goto st85;
		case 86: goto st86;
		case 87: goto st87;
		case 88: goto st88;
		case 89: goto st89;
		case 90: goto st90;
		case 91: goto st91;
		case 92: goto st92;
		case 93: goto st93;
		case 94: goto st94;
		case 95: goto st95;
		case 96: goto st96;
		case 97: goto st97;
		case 98: goto st98;
		case 99: goto st99;
		case 100: goto st100;
		case 101: goto st101;
		case 102: goto st102;
		case 103: goto st103;
		case 104: goto st104;
		case 105: goto st105;
		case 106: goto st106;
		case 107: goto st107;
		case 108: goto st108;
		case 109: goto st109;
		case 110: goto st110;
		case 111: goto st111;
		case 112: goto st112;
		case 113: goto st113;
		case 114: goto st114;
		case 115: goto st115;
		case 116: goto st116;
		case 117: goto st117;
		case 118: goto st118;
		case 119: goto st119;
		case 120: goto st120;
		case 121: goto st121;
		case 122: goto st122;
		case 123: goto st123;
		case 124: goto st124;
		case 125: goto st125;
		case 126: goto st126;
		case 127: goto st127;
		case 128: goto st128;
		case 129: goto st129;
		case 130: goto st130;
		case 131: goto st131;
		case 132: goto st132;
		case 133: goto st133;
		case 134: goto st134;
		case 135: goto st135;
		case 136: goto st136;
		case 137: goto st137;
		case 138: goto st138;
		case 139: goto st139;
		case 140: goto st140;
		case 141: goto st141;
		case 142: goto st142;
		case 143: goto st143;
		case 144: goto st144;
		case 145: goto st145;
		case 146: goto st146;
		case 147: goto st147;
		case 148: goto st148;
		case 149: goto st149;
		case 150: goto st150;
		case 151: goto st151;
		case 152: goto st152;
		case 153: goto st153;
		case 154: goto st154;
		case 155: goto st155;
		case 156: goto st156;
		case 157: goto st157;
		case 158: goto st158;
		case 159: goto st159;
		case 160: goto st160;
		case 161: goto st161;
		case 162: goto st162;
		case 163: goto st163;
		case 164: goto st164;
		case 165: goto st165;
		case 166: goto st166;
		case 167: goto st167;
		case 168: goto st168;
		case 169: goto st169;
		case 170: goto st170;
		case 171: goto st171;
		case 172: goto st172;
		case 173: goto st173;
		case 174: goto st174;
		case 175: goto st175;
		case 176: goto st176;
		case 177: goto st177;
		case 178: goto st178;
		case 179: goto st179;
		case 180: goto st180;
		case 181: goto st181;
		case 182: goto st182;
		case 183: goto st183;
		case 184: goto st184;
		case 185: goto st185;
		case 186: goto st186;
		case 187: goto st187;
		case 188: goto st188;
		case 189: goto st189;
		case 190: goto st190;
		case 191: goto st191;
		case 192: goto st192;
		case 193: goto st193;
		case 194: goto st194;
		case 195: goto st195;
		case 196: goto st196;
		case 197: goto st197;
		case 198: goto st198;
		case 199: goto st199;
		case 200: goto st200;
		case 201: goto st201;
		case 202: goto st202;
		case 203: goto st203;
		case 204: goto st204;
		case 205: goto st205;
		case 206: goto st206;
		case 207: goto st207;
		case 208: goto st208;
		case 209: goto st209;
		case 210: goto st210;
		case 211: goto st211;
		case 212: goto st212;
		case 213: goto st213;
		case 214: goto st214;
		case 215: goto st215;
		case 216: goto st216;
		case 217: goto st217;
		case 218: goto st218;
		case 219: goto st219;
		case 220: goto st220;
		case 221: goto st221;
		case 222: goto st222;
		case 223: goto st223;
		case 224: goto st224;
		case 225: goto st225;
		case 226: goto st226;
		case 227: goto st227;
		case 228: goto st228;
		case 229: goto st229;
		case 230: goto st230;
		case 231: goto st231;
		case 232: goto st232;
		case 233: goto st233;
		case 234: goto st234;
		case 235: goto st235;
		case 236: goto st236;
		case 237: goto st237;
		case 238: goto st238;
		case 239: goto st239;
		case 240: goto st240;
		case 241: goto st241;
		case 242: goto st242;
		case 243: goto st243;
		case 244: goto st244;
		case 245: goto st245;
		case 246: goto st246;
		case 247: goto st247;
		case 248: goto st248;
		case 249: goto st249;
		case 250: goto st250;
		case 251: goto st251;
		case 252: goto st252;
		case 253: goto st253;
		case 254: goto st254;
		case 255: goto st255;
		case 256: goto st256;
		case 257: goto st257;
		case 258: goto st258;
		case 259: goto st259;
		case 260: goto st260;
		case 261: goto st261;
		case 262: goto st262;
		case 263: goto st263;
		case 264: goto st264;
		case 265: goto st265;
		case 266: goto st266;
		case 267: goto st267;
		case 268: goto st268;
		case 269: goto st269;
		case 270: goto st270;
		case 271: goto st271;
		case 272: goto st272;
		case 273: goto st273;
		case 274: goto st274;
		case 275: goto st275;
		case 276: goto st276;
		case 277: goto st277;
		case 278: goto st278;
		case 279: goto st279;
		case 280: goto st280;
		case 281: goto st281;
		case 282: goto st282;
		case 283: goto st283;
		case 284: goto st284;
		case 285: goto st285;
		case 286: goto st286;
		case 287: goto st287;
		case 288: goto st288;
		case 289: goto st289;
		case 290: goto st290;
		case 291: goto st291;
		case 292: goto st292;
		case 293: goto st293;
		case 294: goto st294;
		case 295: goto st295;
		case 296: goto st296;
		case 297: goto st297;
		case 298: goto st298;
		case 299: goto st299;
		case 300: goto st300;
		case 301: goto st301;
		case 302: goto st302;
		case 303: goto st303;
		case 304: goto st304;
		case 305: goto st305;
		case 306: goto st306;
		case 307: goto st307;
		case 308: goto st308;
		case 309: goto st309;
		case 310: goto st310;
		case 311: goto st311;
		case 312: goto st312;
		case 313: goto st313;
		case 314: goto st314;
		case 315: goto st315;
		case 316: goto st316;
		case 317: goto st317;
		case 318: goto st318;
		case 319: goto st319;
		case 320: goto st320;
		case 321: goto st321;
		case 322: goto st322;
		case 323: goto st323;
		case 324: goto st324;
		case 325: goto st325;
		case 326: goto st326;
		case 327: goto st327;
		case 328: goto st328;
		case 329: goto st329;
		case 330: goto st330;
		case 331: goto st331;
		case 332: goto st332;
		case 333: goto st333;
		case 334: goto st334;
		case 335: goto st335;
		case 336: goto st336;
		case 337: goto st337;
		case 338: goto st338;
		case 339: goto st339;
		case 340: goto st340;
		case 341: goto st341;
		case 342: goto st342;
		case 343: goto st343;
		case 344: goto st344;
		case 345: goto st345;
		case 346: goto st346;
		case 347: goto st347;
		case 348: goto st348;
		case 349: goto st349;
		case 350: goto st350;
		case 351: goto st351;
		case 352: goto st352;
		case 353: goto st353;
		case 354: goto st354;
		case 355: goto st355;
		case 356: goto st356;
		case 357: goto st357;
		case 358: goto st358;
		case 359: goto st359;
		case 360: goto st360;
		case 361: goto st361;
		case 362: goto st362;
		case 363: goto st363;
		case 364: goto st364;
		case 365: goto st365;
		case 366: goto st366;
		case 367: goto st367;
		case 368: goto st368;
		case 369: goto st369;
		case 370: goto st370;
		case 371: goto st371;
		case 372: goto st372;
		case 373: goto st373;
		case 374: goto st374;
		case 375: goto st375;
		case 376: goto st376;
		case 377: goto st377;
		case 378: goto st378;
		case 379: goto st379;
		case 380: goto st380;
		case 445: goto st445;
		case 381: goto st381;
		case 382: goto st382;
		case 383: goto st383;
		case 384: goto st384;
		case 385: goto st385;
		case 386: goto st386;
		case 387: goto st387;
		case 388: goto st388;
		case 389: goto st389;
		case 390: goto st390;
		case 391: goto st391;
		case 392: goto st392;
		case 393: goto st393;
		case 394: goto st394;
		case 395: goto st395;
		case 396: goto st396;
		case 397: goto st397;
		case 398: goto st398;
		case 399: goto st399;
		case 400: goto st400;
		case 401: goto st401;
		case 402: goto st402;
		case 403: goto st403;
		case 404: goto st404;
		case 405: goto st405;
		case 406: goto st406;
		case 407: goto st407;
		case 408: goto st408;
		case 409: goto st409;
		case 410: goto st410;
		case 411: goto st411;
		case 412: goto st412;
		case 413: goto st413;
		case 414: goto st414;
		case 446: goto st446;
		case 415: goto st415;
		case 416: goto st416;
		case 417: goto st417;
		case 418: goto st418;
		case 419: goto st419;
		case 420: goto st420;
		case 421: goto st421;
		case 422: goto st422;
		case 423: goto st423;
		case 424: goto st424;
		case 425: goto st425;
		case 426: goto st426;
		case 427: goto st427;
		case 428: goto st428;
		case 429: goto st429;
		case 430: goto st430;
		case 431: goto st431;
		case 432: goto st432;
		case 433: goto st433;
		case 434: goto st434;
		case 435: goto st435;
		case 436: goto st436;
		case 437: goto st437;
		case 438: goto st438;
		case 439: goto st439;
		case 440: goto st440;
		case 441: goto st441;
		case 442: goto st442;
		case 443: goto st443;
	default: break;
	}

	if ( ++p == pe )
		goto _test_eof;
_resume:
	switch (  this->m_cs )
	{
st1:
	if ( ++p == pe )
		goto _test_eof1;
case 1:
	switch( (*p) ) {
		case 13u: goto st2;
		case 32u: goto st2;
		case 60u: goto st317;
	}
	if ( 9u <= (*p) && (*p) <= 10u )
		goto st2;
	goto st0;
st0:
 this->m_cs = 0;
	goto _out;
st2:
	if ( ++p == pe )
		goto _test_eof2;
case 2:
	switch( (*p) ) {
		case 13u: goto st2;
		case 32u: goto st2;
		case 60u: goto st3;
	}
	if ( 9u <= (*p) && (*p) <= 10u )
		goto st2;
	goto st0;
st3:
	if ( ++p == pe )
		goto _test_eof3;
case 3:
	switch( (*p) ) {
		case 33u: goto st4;
		case 58u: goto st26;
		case 63u: goto st310;
		case 95u: goto st26;
	}
	if ( (*p) > 90u ) {
		if ( 97u <= (*p) && (*p) <= 122u )
			goto st26;
	} else if ( (*p) >= 65u )
		goto st26;
	goto st0;
st4:
	if ( ++p == pe )
		goto _test_eof4;
case 4:
	switch( (*p) ) {
		case 45u: goto st5;
		case 68u: goto st9;
	}
	goto st0;
st5:
	if ( ++p == pe )
		goto _test_eof5;
case 5:
	if ( (*p) == 45u )
		goto st6;
	goto st0;
st6:
	if ( ++p == pe )
		goto _test_eof6;
case 6:
	if ( (*p) == 45u )
		goto st7;
	goto st6;
st7:
	if ( ++p == pe )
		goto _test_eof7;
case 7:
	if ( (*p) == 45u )
		goto st8;
	goto st6;
st8:
	if ( ++p == pe )
		goto _test_eof8;
case 8:
	if ( (*p) == 62u )
		goto st2;
	goto st0;
st9:
	if ( ++p == pe )
		goto _test_eof9;
case 9:
	if ( (*p) == 79u )
		goto st10;
	goto st0;
st10:
	if ( ++p == pe )
		goto _test_eof10;
case 10:
	if ( (*p) == 67u )
		goto st11;
	goto st0;
st11:
	if ( ++p == pe )
		goto _test_eof11;
case 11:
	if ( (*p) == 84u )
		goto st12;
	goto st0;
st12:
	if ( ++p == pe )
		goto _test_eof12;
case 12:
	if ( (*p) == 89u )
		goto st13;
	goto st0;
st13:
	if ( ++p == pe )
		goto _test_eof13;
case 13:
	if ( (*p) == 80u )
		goto st14;
	goto st0;
st14:
	if ( ++p == pe )
		goto _test_eof14;
case 14:
	if ( (*p) == 69u )
		goto st15;
	goto st0;
st15:
	if ( ++p == pe )
		goto _test_eof15;
case 15:
	switch( (*p) ) {
		case 13u: goto st16;
		case 32u: goto st16;
	}
	if ( 9u <= (*p) && (*p) <= 10u )
		goto st16;
	goto st0;
st16:
	if ( ++p == pe )
		goto _test_eof16;
case 16:
	switch( (*p) ) {
		case 13u: goto st16;
		case 32u: goto st16;
		case 58u: goto st17;
		case 95u: goto st17;
	}
	if ( (*p) < 65u ) {
		if ( 9u <= (*p) && (*p) <= 10u )
			goto st16;
	} else if ( (*p) > 90u ) {
		if ( 97u <= (*p) && (*p) <= 122u )
			goto st17;
	} else
		goto st17;
	goto st0;
st17:
	if ( ++p == pe )
		goto _test_eof17;
case 17:
	switch( (*p) ) {
		case 13u: goto st18;
		case 32u: goto st18;
		case 62u: goto st19;
		case 91u: goto st78;
		case 95u: goto st17;
	}
	if ( (*p) < 48u ) {
		if ( (*p) > 10u ) {
			if ( 45u <= (*p) && (*p) <= 46u )
				goto st17;
		} else if ( (*p) >= 9u )
			goto st18;
	} else if ( (*p) > 58u ) {
		if ( (*p) > 90u ) {
			if ( 97u <= (*p) && (*p) <= 122u )
				goto st17;
		} else if ( (*p) >= 65u )
			goto st17;
	} else
		goto st17;
	goto st0;
st18:
	if ( ++p == pe )
		goto _test_eof18;
case 18:
	switch( (*p) ) {
		case 13u: goto st18;
		case 32u: goto st18;
		case 62u: goto st19;
		case 80u: goto st66;
		case 83u: goto st305;
		case 91u: goto st78;
	}
	if ( 9u <= (*p) && (*p) <= 10u )
		goto st18;
	goto st0;
st19:
	if ( ++p == pe )
		goto _test_eof19;
case 19:
	switch( (*p) ) {
		case 13u: goto st19;
		case 32u: goto st19;
		case 60u: goto st20;
	}
	if ( 9u <= (*p) && (*p) <= 10u )
		goto st19;
	goto st0;
st20:
	if ( ++p == pe )
		goto _test_eof20;
case 20:
	switch( (*p) ) {
		case 33u: goto st21;
		case 58u: goto st26;
		case 63u: goto st59;
		case 95u: goto st26;
	}
	if ( (*p) > 90u ) {
		if ( 97u <= (*p) && (*p) <= 122u )
			goto st26;
	} else if ( (*p) >= 65u )
		goto st26;
	goto st0;
st21:
	if ( ++p == pe )
		goto _test_eof21;
case 21:
	if ( (*p) == 45u )
		goto st22;
	goto st0;
st22:
	if ( ++p == pe )
		goto _test_eof22;
case 22:
	if ( (*p) == 45u )
		goto st23;
	goto st0;
st23:
	if ( ++p == pe )
		goto _test_eof23;
case 23:
	if ( (*p) == 45u )
		goto st24;
	goto st23;
st24:
	if ( ++p == pe )
		goto _test_eof24;
case 24:
	if ( (*p) == 45u )
		goto st25;
	goto st23;
st25:
	if ( ++p == pe )
		goto _test_eof25;
case 25:
	if ( (*p) == 62u )
		goto st19;
	goto st0;
st26:
	if ( ++p == pe )
		goto _test_eof26;
case 26:
	switch( (*p) ) {
		case 13u: goto st27;
		case 32u: goto st27;
		case 47u: goto st28;
		case 62u: goto tr34;
		case 95u: goto st26;
	}
	if ( (*p) < 45u ) {
		if ( 9u <= (*p) && (*p) <= 10u )
			goto st27;
	} else if ( (*p) > 58u ) {
		if ( (*p) > 90u ) {
			if ( 97u <= (*p) && (*p) <= 122u )
				goto st26;
		} else if ( (*p) >= 65u )
			goto st26;
	} else
		goto st26;
	goto st0;
st27:
	if ( ++p == pe )
		goto _test_eof27;
case 27:
	switch( (*p) ) {
		case 13u: goto st27;
		case 32u: goto st27;
		case 47u: goto st28;
		case 58u: goto st41;
		case 62u: goto tr34;
		case 95u: goto st41;
	}
	if ( (*p) < 65u ) {
		if ( 9u <= (*p) && (*p) <= 10u )
			goto st27;
	} else if ( (*p) > 90u ) {
		if ( 97u <= (*p) && (*p) <= 122u )
			goto st41;
	} else
		goto st41;
	goto st0;
st28:
	if ( ++p == pe )
		goto _test_eof28;
case 28:
	if ( (*p) == 62u )
		goto st444;
	goto st0;
tr34:
#line 108 "../src/xml.ragel"
	{ { pre_push(); { this->m_stack[ this->m_top++] = 444; goto st390;}} }
	goto st444;
st444:
	if ( ++p == pe )
		goto _test_eof444;
case 444:
#line 842 "src/xml.cpp"
	switch( (*p) ) {
		case 13u: goto st444;
		case 32u: goto st444;
		case 60u: goto st29;
	}
	if ( 9u <= (*p) && (*p) <= 10u )
		goto st444;
	goto st0;
st29:
	if ( ++p == pe )
		goto _test_eof29;
case 29:
	switch( (*p) ) {
		case 33u: goto st30;
		case 63u: goto st34;
	}
	goto st0;
st30:
	if ( ++p == pe )
		goto _test_eof30;
case 30:
	if ( (*p) == 45u )
		goto st31;
	goto st0;
st31:
	if ( ++p == pe )
		goto _test_eof31;
case 31:
	if ( (*p) == 45u )
		goto st32;
	goto st0;
st32:
	if ( ++p == pe )
		goto _test_eof32;
case 32:
	if ( (*p) == 45u )
		goto st33;
	goto st32;
st33:
	if ( ++p == pe )
		goto _test_eof33;
case 33:
	if ( (*p) == 45u )
		goto st28;
	goto st32;
st34:
	if ( ++p == pe )
		goto _test_eof34;
case 34:
	switch( (*p) ) {
		case 58u: goto st35;
		case 88u: goto st38;
		case 95u: goto st35;
		case 120u: goto st38;
	}
	if ( (*p) > 90u ) {
		if ( 97u <= (*p) && (*p) <= 122u )
			goto st35;
	} else if ( (*p) >= 65u )
		goto st35;
	goto st0;
st35:
	if ( ++p == pe )
		goto _test_eof35;
case 35:
	switch( (*p) ) {
		case 13u: goto st36;
		case 32u: goto st36;
		case 63u: goto st28;
		case 95u: goto st35;
	}
	if ( (*p) < 48u ) {
		if ( (*p) > 10u ) {
			if ( 45u <= (*p) && (*p) <= 46u )
				goto st35;
		} else if ( (*p) >= 9u )
			goto st36;
	} else if ( (*p) > 58u ) {
		if ( (*p) > 90u ) {
			if ( 97u <= (*p) && (*p) <= 122u )
				goto st35;
		} else if ( (*p) >= 65u )
			goto st35;
	} else
		goto st35;
	goto st0;
st36:
	if ( ++p == pe )
		goto _test_eof36;
case 36:
	if ( (*p) == 63u )
		goto st37;
	goto st36;
st37:
	if ( ++p == pe )
		goto _test_eof37;
case 37:
	switch( (*p) ) {
		case 62u: goto st444;
		case 63u: goto st37;
	}
	goto st36;
st38:
	if ( ++p == pe )
		goto _test_eof38;
case 38:
	switch( (*p) ) {
		case 13u: goto st36;
		case 32u: goto st36;
		case 63u: goto st28;
		case 77u: goto st39;
		case 95u: goto st35;
		case 109u: goto st39;
	}
	if ( (*p) < 48u ) {
		if ( (*p) > 10u ) {
			if ( 45u <= (*p) && (*p) <= 46u )
				goto st35;
		} else if ( (*p) >= 9u )
			goto st36;
	} else if ( (*p) > 58u ) {
		if ( (*p) > 90u ) {
			if ( 97u <= (*p) && (*p) <= 122u )
				goto st35;
		} else if ( (*p) >= 65u )
			goto st35;
	} else
		goto st35;
	goto st0;
st39:
	if ( ++p == pe )
		goto _test_eof39;
case 39:
	switch( (*p) ) {
		case 13u: goto st36;
		case 32u: goto st36;
		case 63u: goto st28;
		case 76u: goto st40;
		case 95u: goto st35;
		case 108u: goto st40;
	}
	if ( (*p) < 48u ) {
		if ( (*p) > 10u ) {
			if ( 45u <= (*p) && (*p) <= 46u )
				goto st35;
		} else if ( (*p) >= 9u )
			goto st36;
	} else if ( (*p) > 58u ) {
		if ( (*p) > 90u ) {
			if ( 97u <= (*p) && (*p) <= 122u )
				goto st35;
		} else if ( (*p) >= 65u )
			goto st35;
	} else
		goto st35;
	goto st0;
st40:
	if ( ++p == pe )
		goto _test_eof40;
case 40:
	if ( (*p) == 95u )
		goto st35;
	if ( (*p) < 48u ) {
		if ( 45u <= (*p) && (*p) <= 46u )
			goto st35;
	} else if ( (*p) > 58u ) {
		if ( (*p) > 90u ) {
			if ( 97u <= (*p) && (*p) <= 122u )
				goto st35;
		} else if ( (*p) >= 65u )
			goto st35;
	} else
		goto st35;
	goto st0;
st41:
	if ( ++p == pe )
		goto _test_eof41;
case 41:
	switch( (*p) ) {
		case 13u: goto st42;
		case 32u: goto st42;
		case 61u: goto st43;
		case 95u: goto st41;
	}
	if ( (*p) < 48u ) {
		if ( (*p) > 10u ) {
			if ( 45u <= (*p) && (*p) <= 46u )
				goto st41;
		} else if ( (*p) >= 9u )
			goto st42;
	} else if ( (*p) > 58u ) {
		if ( (*p) > 90u ) {
			if ( 97u <= (*p) && (*p) <= 122u )
				goto st41;
		} else if ( (*p) >= 65u )
			goto st41;
	} else
		goto st41;
	goto st0;
st42:
	if ( ++p == pe )
		goto _test_eof42;
case 42:
	switch( (*p) ) {
		case 13u: goto st42;
		case 32u: goto st42;
		case 61u: goto st43;
	}
	if ( 9u <= (*p) && (*p) <= 10u )
		goto st42;
	goto st0;
st43:
	if ( ++p == pe )
		goto _test_eof43;
case 43:
	switch( (*p) ) {
		case 13u: goto st43;
		case 32u: goto st43;
		case 34u: goto tr50;
		case 39u: goto tr51;
	}
	if ( 9u <= (*p) && (*p) <= 10u )
		goto st43;
	goto st0;
tr50:
#line 30 "../src/xml.ragel"
	{ m_output.push('['); ++m_rec; }
	goto st44;
tr59:
#line 31 "../src/xml.ragel"
	{ pop(); }
	goto st44;
st44:
	if ( ++p == pe )
		goto _test_eof44;
case 44:
#line 1079 "src/xml.cpp"
	switch( (*p) ) {
		case 34u: goto tr53;
		case 38u: goto tr54;
		case 60u: goto st0;
	}
	goto st44;
tr53:
#line 31 "../src/xml.ragel"
	{ pop(); }
	goto st45;
st45:
	if ( ++p == pe )
		goto _test_eof45;
case 45:
#line 1094 "src/xml.cpp"
	switch( (*p) ) {
		case 13u: goto st27;
		case 32u: goto st27;
		case 47u: goto st28;
		case 62u: goto tr34;
	}
	if ( 9u <= (*p) && (*p) <= 10u )
		goto st27;
	goto st0;
tr54:
#line 30 "../src/xml.ragel"
	{ m_output.push('['); ++m_rec; }
	goto st46;
st46:
	if ( ++p == pe )
		goto _test_eof46;
case 46:
#line 1112 "src/xml.cpp"
	switch( (*p) ) {
		case 35u: goto st47;
		case 58u: goto st51;
		case 95u: goto st51;
	}
	if ( (*p) > 90u ) {
		if ( 97u <= (*p) && (*p) <= 122u )
			goto st51;
	} else if ( (*p) >= 65u )
		goto st51;
	goto st0;
st47:
	if ( ++p == pe )
		goto _test_eof47;
case 47:
	if ( (*p) == 120u )
		goto st49;
	if ( 48u <= (*p) && (*p) <= 57u )
		goto st48;
	goto st0;
st48:
	if ( ++p == pe )
		goto _test_eof48;
case 48:
	if ( (*p) == 59u )
		goto tr59;
	if ( 48u <= (*p) && (*p) <= 57u )
		goto st48;
	goto st0;
st49:
	if ( ++p == pe )
		goto _test_eof49;
case 49:
	if ( (*p) < 65u ) {
		if ( 48u <= (*p) && (*p) <= 57u )
			goto st50;
	} else if ( (*p) > 70u ) {
		if ( 97u <= (*p) && (*p) <= 102u )
			goto st50;
	} else
		goto st50;
	goto st0;
st50:
	if ( ++p == pe )
		goto _test_eof50;
case 50:
	if ( (*p) == 59u )
		goto tr59;
	if ( (*p) < 65u ) {
		if ( 48u <= (*p) && (*p) <= 57u )
			goto st50;
	} else if ( (*p) > 70u ) {
		if ( 97u <= (*p) && (*p) <= 102u )
			goto st50;
	} else
		goto st50;
	goto st0;
st51:
	if ( ++p == pe )
		goto _test_eof51;
case 51:
	switch( (*p) ) {
		case 59u: goto st44;
		case 95u: goto st51;
	}
	if ( (*p) < 48u ) {
		if ( 45u <= (*p) && (*p) <= 46u )
			goto st51;
	} else if ( (*p) > 58u ) {
		if ( (*p) > 90u ) {
			if ( 97u <= (*p) && (*p) <= 122u )
				goto st51;
		} else if ( (*p) >= 65u )
			goto st51;
	} else
		goto st51;
	goto st0;
tr51:
#line 30 "../src/xml.ragel"
	{ m_output.push('['); ++m_rec; }
	goto st52;
tr67:
#line 31 "../src/xml.ragel"
	{ pop(); }
	goto st52;
st52:
	if ( ++p == pe )
		goto _test_eof52;
case 52:
#line 1202 "src/xml.cpp"
	switch( (*p) ) {
		case 38u: goto tr62;
		case 39u: goto tr53;
		case 60u: goto st0;
	}
	goto st52;
tr62:
#line 30 "../src/xml.ragel"
	{ m_output.push('['); ++m_rec; }
	goto st53;
st53:
	if ( ++p == pe )
		goto _test_eof53;
case 53:
#line 1217 "src/xml.cpp"
	switch( (*p) ) {
		case 35u: goto st54;
		case 58u: goto st58;
		case 95u: goto st58;
	}
	if ( (*p) > 90u ) {
		if ( 97u <= (*p) && (*p) <= 122u )
			goto st58;
	} else if ( (*p) >= 65u )
		goto st58;
	goto st0;
st54:
	if ( ++p == pe )
		goto _test_eof54;
case 54:
	if ( (*p) == 120u )
		goto st56;
	if ( 48u <= (*p) && (*p) <= 57u )
		goto st55;
	goto st0;
st55:
	if ( ++p == pe )
		goto _test_eof55;
case 55:
	if ( (*p) == 59u )
		goto tr67;
	if ( 48u <= (*p) && (*p) <= 57u )
		goto st55;
	goto st0;
st56:
	if ( ++p == pe )
		goto _test_eof56;
case 56:
	if ( (*p) < 65u ) {
		if ( 48u <= (*p) && (*p) <= 57u )
			goto st57;
	} else if ( (*p) > 70u ) {
		if ( 97u <= (*p) && (*p) <= 102u )
			goto st57;
	} else
		goto st57;
	goto st0;
st57:
	if ( ++p == pe )
		goto _test_eof57;
case 57:
	if ( (*p) == 59u )
		goto tr67;
	if ( (*p) < 65u ) {
		if ( 48u <= (*p) && (*p) <= 57u )
			goto st57;
	} else if ( (*p) > 70u ) {
		if ( 97u <= (*p) && (*p) <= 102u )
			goto st57;
	} else
		goto st57;
	goto st0;
st58:
	if ( ++p == pe )
		goto _test_eof58;
case 58:
	switch( (*p) ) {
		case 59u: goto st52;
		case 95u: goto st58;
	}
	if ( (*p) < 48u ) {
		if ( 45u <= (*p) && (*p) <= 46u )
			goto st58;
	} else if ( (*p) > 58u ) {
		if ( (*p) > 90u ) {
			if ( 97u <= (*p) && (*p) <= 122u )
				goto st58;
		} else if ( (*p) >= 65u )
			goto st58;
	} else
		goto st58;
	goto st0;
st59:
	if ( ++p == pe )
		goto _test_eof59;
case 59:
	switch( (*p) ) {
		case 58u: goto st60;
		case 88u: goto st63;
		case 95u: goto st60;
		case 120u: goto st63;
	}
	if ( (*p) > 90u ) {
		if ( 97u <= (*p) && (*p) <= 122u )
			goto st60;
	} else if ( (*p) >= 65u )
		goto st60;
	goto st0;
st60:
	if ( ++p == pe )
		goto _test_eof60;
case 60:
	switch( (*p) ) {
		case 13u: goto st61;
		case 32u: goto st61;
		case 63u: goto st25;
		case 95u: goto st60;
	}
	if ( (*p) < 48u ) {
		if ( (*p) > 10u ) {
			if ( 45u <= (*p) && (*p) <= 46u )
				goto st60;
		} else if ( (*p) >= 9u )
			goto st61;
	} else if ( (*p) > 58u ) {
		if ( (*p) > 90u ) {
			if ( 97u <= (*p) && (*p) <= 122u )
				goto st60;
		} else if ( (*p) >= 65u )
			goto st60;
	} else
		goto st60;
	goto st0;
st61:
	if ( ++p == pe )
		goto _test_eof61;
case 61:
	if ( (*p) == 63u )
		goto st62;
	goto st61;
st62:
	if ( ++p == pe )
		goto _test_eof62;
case 62:
	switch( (*p) ) {
		case 62u: goto st19;
		case 63u: goto st62;
	}
	goto st61;
st63:
	if ( ++p == pe )
		goto _test_eof63;
case 63:
	switch( (*p) ) {
		case 13u: goto st61;
		case 32u: goto st61;
		case 63u: goto st25;
		case 77u: goto st64;
		case 95u: goto st60;
		case 109u: goto st64;
	}
	if ( (*p) < 48u ) {
		if ( (*p) > 10u ) {
			if ( 45u <= (*p) && (*p) <= 46u )
				goto st60;
		} else if ( (*p) >= 9u )
			goto st61;
	} else if ( (*p) > 58u ) {
		if ( (*p) > 90u ) {
			if ( 97u <= (*p) && (*p) <= 122u )
				goto st60;
		} else if ( (*p) >= 65u )
			goto st60;
	} else
		goto st60;
	goto st0;
st64:
	if ( ++p == pe )
		goto _test_eof64;
case 64:
	switch( (*p) ) {
		case 13u: goto st61;
		case 32u: goto st61;
		case 63u: goto st25;
		case 76u: goto st65;
		case 95u: goto st60;
		case 108u: goto st65;
	}
	if ( (*p) < 48u ) {
		if ( (*p) > 10u ) {
			if ( 45u <= (*p) && (*p) <= 46u )
				goto st60;
		} else if ( (*p) >= 9u )
			goto st61;
	} else if ( (*p) > 58u ) {
		if ( (*p) > 90u ) {
			if ( 97u <= (*p) && (*p) <= 122u )
				goto st60;
		} else if ( (*p) >= 65u )
			goto st60;
	} else
		goto st60;
	goto st0;
st65:
	if ( ++p == pe )
		goto _test_eof65;
case 65:
	if ( (*p) == 95u )
		goto st60;
	if ( (*p) < 48u ) {
		if ( 45u <= (*p) && (*p) <= 46u )
			goto st60;
	} else if ( (*p) > 58u ) {
		if ( (*p) > 90u ) {
			if ( 97u <= (*p) && (*p) <= 122u )
				goto st60;
		} else if ( (*p) >= 65u )
			goto st60;
	} else
		goto st60;
	goto st0;
st66:
	if ( ++p == pe )
		goto _test_eof66;
case 66:
	if ( (*p) == 85u )
		goto st67;
	goto st0;
st67:
	if ( ++p == pe )
		goto _test_eof67;
case 67:
	if ( (*p) == 66u )
		goto st68;
	goto st0;
st68:
	if ( ++p == pe )
		goto _test_eof68;
case 68:
	if ( (*p) == 76u )
		goto st69;
	goto st0;
st69:
	if ( ++p == pe )
		goto _test_eof69;
case 69:
	if ( (*p) == 73u )
		goto st70;
	goto st0;
st70:
	if ( ++p == pe )
		goto _test_eof70;
case 70:
	if ( (*p) == 67u )
		goto st71;
	goto st0;
st71:
	if ( ++p == pe )
		goto _test_eof71;
case 71:
	switch( (*p) ) {
		case 13u: goto st72;
		case 32u: goto st72;
	}
	if ( 9u <= (*p) && (*p) <= 10u )
		goto st72;
	goto st0;
st72:
	if ( ++p == pe )
		goto _test_eof72;
case 72:
	switch( (*p) ) {
		case 13u: goto st72;
		case 32u: goto st72;
		case 34u: goto st73;
		case 39u: goto st304;
	}
	if ( 9u <= (*p) && (*p) <= 10u )
		goto st72;
	goto st0;
st73:
	if ( ++p == pe )
		goto _test_eof73;
case 73:
	switch( (*p) ) {
		case 10u: goto st73;
		case 13u: goto st73;
		case 34u: goto st74;
		case 61u: goto st73;
		case 95u: goto st73;
	}
	if ( (*p) < 39u ) {
		if ( 32u <= (*p) && (*p) <= 37u )
			goto st73;
	} else if ( (*p) > 59u ) {
		if ( (*p) > 90u ) {
			if ( 97u <= (*p) && (*p) <= 122u )
				goto st73;
		} else if ( (*p) >= 63u )
			goto st73;
	} else
		goto st73;
	goto st0;
st74:
	if ( ++p == pe )
		goto _test_eof74;
case 74:
	switch( (*p) ) {
		case 13u: goto st75;
		case 32u: goto st75;
	}
	if ( 9u <= (*p) && (*p) <= 10u )
		goto st75;
	goto st0;
st75:
	if ( ++p == pe )
		goto _test_eof75;
case 75:
	switch( (*p) ) {
		case 13u: goto st75;
		case 32u: goto st75;
		case 34u: goto st76;
		case 39u: goto st303;
	}
	if ( 9u <= (*p) && (*p) <= 10u )
		goto st75;
	goto st0;
st76:
	if ( ++p == pe )
		goto _test_eof76;
case 76:
	if ( (*p) == 34u )
		goto st77;
	goto st76;
st77:
	if ( ++p == pe )
		goto _test_eof77;
case 77:
	switch( (*p) ) {
		case 13u: goto st77;
		case 32u: goto st77;
		case 62u: goto st19;
		case 91u: goto st78;
	}
	if ( 9u <= (*p) && (*p) <= 10u )
		goto st77;
	goto st0;
st78:
	if ( ++p == pe )
		goto _test_eof78;
case 78:
	switch( (*p) ) {
		case 13u: goto st78;
		case 32u: goto st78;
		case 37u: goto st79;
		case 60u: goto st81;
		case 93u: goto st302;
	}
	if ( 9u <= (*p) && (*p) <= 10u )
		goto st78;
	goto st0;
st79:
	if ( ++p == pe )
		goto _test_eof79;
case 79:
	switch( (*p) ) {
		case 58u: goto st80;
		case 95u: goto st80;
	}
	if ( (*p) > 90u ) {
		if ( 97u <= (*p) && (*p) <= 122u )
			goto st80;
	} else if ( (*p) >= 65u )
		goto st80;
	goto st0;
st80:
	if ( ++p == pe )
		goto _test_eof80;
case 80:
	switch( (*p) ) {
		case 59u: goto st78;
		case 95u: goto st80;
	}
	if ( (*p) < 48u ) {
		if ( 45u <= (*p) && (*p) <= 46u )
			goto st80;
	} else if ( (*p) > 58u ) {
		if ( (*p) > 90u ) {
			if ( 97u <= (*p) && (*p) <= 122u )
				goto st80;
		} else if ( (*p) >= 65u )
			goto st80;
	} else
		goto st80;
	goto st0;
st81:
	if ( ++p == pe )
		goto _test_eof81;
case 81:
	switch( (*p) ) {
		case 33u: goto st82;
		case 63u: goto st295;
	}
	goto st0;
st82:
	if ( ++p == pe )
		goto _test_eof82;
case 82:
	switch( (*p) ) {
		case 45u: goto st83;
		case 65u: goto st87;
		case 69u: goto st170;
		case 78u: goto st273;
	}
	goto st0;
st83:
	if ( ++p == pe )
		goto _test_eof83;
case 83:
	if ( (*p) == 45u )
		goto st84;
	goto st0;
st84:
	if ( ++p == pe )
		goto _test_eof84;
case 84:
	if ( (*p) == 45u )
		goto st85;
	goto st84;
st85:
	if ( ++p == pe )
		goto _test_eof85;
case 85:
	if ( (*p) == 45u )
		goto st86;
	goto st84;
st86:
	if ( ++p == pe )
		goto _test_eof86;
case 86:
	if ( (*p) == 62u )
		goto st78;
	goto st0;
st87:
	if ( ++p == pe )
		goto _test_eof87;
case 87:
	if ( (*p) == 84u )
		goto st88;
	goto st0;
st88:
	if ( ++p == pe )
		goto _test_eof88;
case 88:
	if ( (*p) == 84u )
		goto st89;
	goto st0;
st89:
	if ( ++p == pe )
		goto _test_eof89;
case 89:
	if ( (*p) == 76u )
		goto st90;
	goto st0;
st90:
	if ( ++p == pe )
		goto _test_eof90;
case 90:
	if ( (*p) == 73u )
		goto st91;
	goto st0;
st91:
	if ( ++p == pe )
		goto _test_eof91;
case 91:
	if ( (*p) == 83u )
		goto st92;
	goto st0;
st92:
	if ( ++p == pe )
		goto _test_eof92;
case 92:
	if ( (*p) == 84u )
		goto st93;
	goto st0;
st93:
	if ( ++p == pe )
		goto _test_eof93;
case 93:
	switch( (*p) ) {
		case 13u: goto st94;
		case 32u: goto st94;
	}
	if ( 9u <= (*p) && (*p) <= 10u )
		goto st94;
	goto st0;
st94:
	if ( ++p == pe )
		goto _test_eof94;
case 94:
	switch( (*p) ) {
		case 13u: goto st94;
		case 32u: goto st94;
		case 58u: goto st95;
		case 95u: goto st95;
	}
	if ( (*p) < 65u ) {
		if ( 9u <= (*p) && (*p) <= 10u )
			goto st94;
	} else if ( (*p) > 90u ) {
		if ( 97u <= (*p) && (*p) <= 122u )
			goto st95;
	} else
		goto st95;
	goto st0;
st95:
	if ( ++p == pe )
		goto _test_eof95;
case 95:
	switch( (*p) ) {
		case 13u: goto st96;
		case 32u: goto st96;
		case 62u: goto st78;
		case 95u: goto st95;
	}
	if ( (*p) < 48u ) {
		if ( (*p) > 10u ) {
			if ( 45u <= (*p) && (*p) <= 46u )
				goto st95;
		} else if ( (*p) >= 9u )
			goto st96;
	} else if ( (*p) > 58u ) {
		if ( (*p) > 90u ) {
			if ( 97u <= (*p) && (*p) <= 122u )
				goto st95;
		} else if ( (*p) >= 65u )
			goto st95;
	} else
		goto st95;
	goto st0;
st96:
	if ( ++p == pe )
		goto _test_eof96;
case 96:
	switch( (*p) ) {
		case 13u: goto st96;
		case 32u: goto st96;
		case 58u: goto st97;
		case 62u: goto st78;
		case 95u: goto st97;
	}
	if ( (*p) < 65u ) {
		if ( 9u <= (*p) && (*p) <= 10u )
			goto st96;
	} else if ( (*p) > 90u ) {
		if ( 97u <= (*p) && (*p) <= 122u )
			goto st97;
	} else
		goto st97;
	goto st0;
st97:
	if ( ++p == pe )
		goto _test_eof97;
case 97:
	switch( (*p) ) {
		case 13u: goto st98;
		case 32u: goto st98;
		case 95u: goto st97;
	}
	if ( (*p) < 48u ) {
		if ( (*p) > 10u ) {
			if ( 45u <= (*p) && (*p) <= 46u )
				goto st97;
		} else if ( (*p) >= 9u )
			goto st98;
	} else if ( (*p) > 58u ) {
		if ( (*p) > 90u ) {
			if ( 97u <= (*p) && (*p) <= 122u )
				goto st97;
		} else if ( (*p) >= 65u )
			goto st97;
	} else
		goto st97;
	goto st0;
st98:
	if ( ++p == pe )
		goto _test_eof98;
case 98:
	switch( (*p) ) {
		case 13u: goto st98;
		case 32u: goto st98;
		case 40u: goto st99;
		case 67u: goto st137;
		case 69u: goto st141;
		case 73u: goto st148;
		case 78u: goto st153;
	}
	if ( 9u <= (*p) && (*p) <= 10u )
		goto st98;
	goto st0;
st99:
	if ( ++p == pe )
		goto _test_eof99;
case 99:
	switch( (*p) ) {
		case 13u: goto st99;
		case 32u: goto st99;
		case 95u: goto st100;
	}
	if ( (*p) < 48u ) {
		if ( (*p) > 10u ) {
			if ( 45u <= (*p) && (*p) <= 46u )
				goto st100;
		} else if ( (*p) >= 9u )
			goto st99;
	} else if ( (*p) > 58u ) {
		if ( (*p) > 90u ) {
			if ( 97u <= (*p) && (*p) <= 122u )
				goto st100;
		} else if ( (*p) >= 65u )
			goto st100;
	} else
		goto st100;
	goto st0;
st100:
	if ( ++p == pe )
		goto _test_eof100;
case 100:
	switch( (*p) ) {
		case 13u: goto st101;
		case 32u: goto st101;
		case 41u: goto st102;
		case 95u: goto st100;
		case 124u: goto st99;
	}
	if ( (*p) < 48u ) {
		if ( (*p) > 10u ) {
			if ( 45u <= (*p) && (*p) <= 46u )
				goto st100;
		} else if ( (*p) >= 9u )
			goto st101;
	} else if ( (*p) > 58u ) {
		if ( (*p) > 90u ) {
			if ( 97u <= (*p) && (*p) <= 122u )
				goto st100;
		} else if ( (*p) >= 65u )
			goto st100;
	} else
		goto st100;
	goto st0;
st101:
	if ( ++p == pe )
		goto _test_eof101;
case 101:
	switch( (*p) ) {
		case 13u: goto st101;
		case 32u: goto st101;
		case 41u: goto st102;
		case 124u: goto st99;
	}
	if ( 9u <= (*p) && (*p) <= 10u )
		goto st101;
	goto st0;
st102:
	if ( ++p == pe )
		goto _test_eof102;
case 102:
	switch( (*p) ) {
		case 13u: goto st103;
		case 32u: goto st103;
	}
	if ( 9u <= (*p) && (*p) <= 10u )
		goto st103;
	goto st0;
st103:
	if ( ++p == pe )
		goto _test_eof103;
case 103:
	switch( (*p) ) {
		case 13u: goto st103;
		case 32u: goto st103;
		case 34u: goto st104;
		case 35u: goto st112;
		case 39u: goto st119;
	}
	if ( 9u <= (*p) && (*p) <= 10u )
		goto st103;
	goto st0;
tr130:
#line 31 "../src/xml.ragel"
	{ pop(); }
	goto st104;
st104:
	if ( ++p == pe )
		goto _test_eof104;
case 104:
#line 1899 "src/xml.cpp"
	switch( (*p) ) {
		case 34u: goto st105;
		case 38u: goto tr125;
		case 60u: goto st0;
	}
	goto st104;
st105:
	if ( ++p == pe )
		goto _test_eof105;
case 105:
	switch( (*p) ) {
		case 13u: goto st96;
		case 32u: goto st96;
		case 62u: goto st78;
	}
	if ( 9u <= (*p) && (*p) <= 10u )
		goto st96;
	goto st0;
tr125:
#line 30 "../src/xml.ragel"
	{ m_output.push('['); ++m_rec; }
	goto st106;
st106:
	if ( ++p == pe )
		goto _test_eof106;
case 106:
#line 1926 "src/xml.cpp"
	switch( (*p) ) {
		case 35u: goto st107;
		case 58u: goto st111;
		case 95u: goto st111;
	}
	if ( (*p) > 90u ) {
		if ( 97u <= (*p) && (*p) <= 122u )
			goto st111;
	} else if ( (*p) >= 65u )
		goto st111;
	goto st0;
st107:
	if ( ++p == pe )
		goto _test_eof107;
case 107:
	if ( (*p) == 120u )
		goto st109;
	if ( 48u <= (*p) && (*p) <= 57u )
		goto st108;
	goto st0;
st108:
	if ( ++p == pe )
		goto _test_eof108;
case 108:
	if ( (*p) == 59u )
		goto tr130;
	if ( 48u <= (*p) && (*p) <= 57u )
		goto st108;
	goto st0;
st109:
	if ( ++p == pe )
		goto _test_eof109;
case 109:
	if ( (*p) < 65u ) {
		if ( 48u <= (*p) && (*p) <= 57u )
			goto st110;
	} else if ( (*p) > 70u ) {
		if ( 97u <= (*p) && (*p) <= 102u )
			goto st110;
	} else
		goto st110;
	goto st0;
st110:
	if ( ++p == pe )
		goto _test_eof110;
case 110:
	if ( (*p) == 59u )
		goto tr130;
	if ( (*p) < 65u ) {
		if ( 48u <= (*p) && (*p) <= 57u )
			goto st110;
	} else if ( (*p) > 70u ) {
		if ( 97u <= (*p) && (*p) <= 102u )
			goto st110;
	} else
		goto st110;
	goto st0;
st111:
	if ( ++p == pe )
		goto _test_eof111;
case 111:
	switch( (*p) ) {
		case 59u: goto st104;
		case 95u: goto st111;
	}
	if ( (*p) < 48u ) {
		if ( 45u <= (*p) && (*p) <= 46u )
			goto st111;
	} else if ( (*p) > 58u ) {
		if ( (*p) > 90u ) {
			if ( 97u <= (*p) && (*p) <= 122u )
				goto st111;
		} else if ( (*p) >= 65u )
			goto st111;
	} else
		goto st111;
	goto st0;
st112:
	if ( ++p == pe )
		goto _test_eof112;
case 112:
	switch( (*p) ) {
		case 70u: goto st113;
		case 73u: goto st126;
		case 82u: goto st132;
	}
	goto st0;
st113:
	if ( ++p == pe )
		goto _test_eof113;
case 113:
	if ( (*p) == 73u )
		goto st114;
	goto st0;
st114:
	if ( ++p == pe )
		goto _test_eof114;
case 114:
	if ( (*p) == 88u )
		goto st115;
	goto st0;
st115:
	if ( ++p == pe )
		goto _test_eof115;
case 115:
	if ( (*p) == 69u )
		goto st116;
	goto st0;
st116:
	if ( ++p == pe )
		goto _test_eof116;
case 116:
	if ( (*p) == 68u )
		goto st117;
	goto st0;
st117:
	if ( ++p == pe )
		goto _test_eof117;
case 117:
	switch( (*p) ) {
		case 13u: goto st118;
		case 32u: goto st118;
	}
	if ( 9u <= (*p) && (*p) <= 10u )
		goto st118;
	goto st0;
st118:
	if ( ++p == pe )
		goto _test_eof118;
case 118:
	switch( (*p) ) {
		case 13u: goto st118;
		case 32u: goto st118;
		case 34u: goto st104;
		case 39u: goto st119;
	}
	if ( 9u <= (*p) && (*p) <= 10u )
		goto st118;
	goto st0;
tr145:
#line 31 "../src/xml.ragel"
	{ pop(); }
	goto st119;
st119:
	if ( ++p == pe )
		goto _test_eof119;
case 119:
#line 2074 "src/xml.cpp"
	switch( (*p) ) {
		case 38u: goto tr140;
		case 39u: goto st105;
		case 60u: goto st0;
	}
	goto st119;
tr140:
#line 30 "../src/xml.ragel"
	{ m_output.push('['); ++m_rec; }
	goto st120;
st120:
	if ( ++p == pe )
		goto _test_eof120;
case 120:
#line 2089 "src/xml.cpp"
	switch( (*p) ) {
		case 35u: goto st121;
		case 58u: goto st125;
		case 95u: goto st125;
	}
	if ( (*p) > 90u ) {
		if ( 97u <= (*p) && (*p) <= 122u )
			goto st125;
	} else if ( (*p) >= 65u )
		goto st125;
	goto st0;
st121:
	if ( ++p == pe )
		goto _test_eof121;
case 121:
	if ( (*p) == 120u )
		goto st123;
	if ( 48u <= (*p) && (*p) <= 57u )
		goto st122;
	goto st0;
st122:
	if ( ++p == pe )
		goto _test_eof122;
case 122:
	if ( (*p) == 59u )
		goto tr145;
	if ( 48u <= (*p) && (*p) <= 57u )
		goto st122;
	goto st0;
st123:
	if ( ++p == pe )
		goto _test_eof123;
case 123:
	if ( (*p) < 65u ) {
		if ( 48u <= (*p) && (*p) <= 57u )
			goto st124;
	} else if ( (*p) > 70u ) {
		if ( 97u <= (*p) && (*p) <= 102u )
			goto st124;
	} else
		goto st124;
	goto st0;
st124:
	if ( ++p == pe )
		goto _test_eof124;
case 124:
	if ( (*p) == 59u )
		goto tr145;
	if ( (*p) < 65u ) {
		if ( 48u <= (*p) && (*p) <= 57u )
			goto st124;
	} else if ( (*p) > 70u ) {
		if ( 97u <= (*p) && (*p) <= 102u )
			goto st124;
	} else
		goto st124;
	goto st0;
st125:
	if ( ++p == pe )
		goto _test_eof125;
case 125:
	switch( (*p) ) {
		case 59u: goto st119;
		case 95u: goto st125;
	}
	if ( (*p) < 48u ) {
		if ( 45u <= (*p) && (*p) <= 46u )
			goto st125;
	} else if ( (*p) > 58u ) {
		if ( (*p) > 90u ) {
			if ( 97u <= (*p) && (*p) <= 122u )
				goto st125;
		} else if ( (*p) >= 65u )
			goto st125;
	} else
		goto st125;
	goto st0;
st126:
	if ( ++p == pe )
		goto _test_eof126;
case 126:
	if ( (*p) == 77u )
		goto st127;
	goto st0;
st127:
	if ( ++p == pe )
		goto _test_eof127;
case 127:
	if ( (*p) == 80u )
		goto st128;
	goto st0;
st128:
	if ( ++p == pe )
		goto _test_eof128;
case 128:
	if ( (*p) == 76u )
		goto st129;
	goto st0;
st129:
	if ( ++p == pe )
		goto _test_eof129;
case 129:
	if ( (*p) == 73u )
		goto st130;
	goto st0;
st130:
	if ( ++p == pe )
		goto _test_eof130;
case 130:
	if ( (*p) == 69u )
		goto st131;
	goto st0;
st131:
	if ( ++p == pe )
		goto _test_eof131;
case 131:
	if ( (*p) == 68u )
		goto st105;
	goto st0;
st132:
	if ( ++p == pe )
		goto _test_eof132;
case 132:
	if ( (*p) == 69u )
		goto st133;
	goto st0;
st133:
	if ( ++p == pe )
		goto _test_eof133;
case 133:
	if ( (*p) == 81u )
		goto st134;
	goto st0;
st134:
	if ( ++p == pe )
		goto _test_eof134;
case 134:
	if ( (*p) == 85u )
		goto st135;
	goto st0;
st135:
	if ( ++p == pe )
		goto _test_eof135;
case 135:
	if ( (*p) == 73u )
		goto st136;
	goto st0;
st136:
	if ( ++p == pe )
		goto _test_eof136;
case 136:
	if ( (*p) == 82u )
		goto st130;
	goto st0;
st137:
	if ( ++p == pe )
		goto _test_eof137;
case 137:
	if ( (*p) == 68u )
		goto st138;
	goto st0;
st138:
	if ( ++p == pe )
		goto _test_eof138;
case 138:
	if ( (*p) == 65u )
		goto st139;
	goto st0;
st139:
	if ( ++p == pe )
		goto _test_eof139;
case 139:
	if ( (*p) == 84u )
		goto st140;
	goto st0;
st140:
	if ( ++p == pe )
		goto _test_eof140;
case 140:
	if ( (*p) == 65u )
		goto st102;
	goto st0;
st141:
	if ( ++p == pe )
		goto _test_eof141;
case 141:
	if ( (*p) == 78u )
		goto st142;
	goto st0;
st142:
	if ( ++p == pe )
		goto _test_eof142;
case 142:
	if ( (*p) == 84u )
		goto st143;
	goto st0;
st143:
	if ( ++p == pe )
		goto _test_eof143;
case 143:
	if ( (*p) == 73u )
		goto st144;
	goto st0;
st144:
	if ( ++p == pe )
		goto _test_eof144;
case 144:
	if ( (*p) == 84u )
		goto st145;
	goto st0;
st145:
	if ( ++p == pe )
		goto _test_eof145;
case 145:
	switch( (*p) ) {
		case 73u: goto st146;
		case 89u: goto st102;
	}
	goto st0;
st146:
	if ( ++p == pe )
		goto _test_eof146;
case 146:
	if ( (*p) == 69u )
		goto st147;
	goto st0;
st147:
	if ( ++p == pe )
		goto _test_eof147;
case 147:
	if ( (*p) == 83u )
		goto st102;
	goto st0;
st148:
	if ( ++p == pe )
		goto _test_eof148;
case 148:
	if ( (*p) == 68u )
		goto st149;
	goto st0;
st149:
	if ( ++p == pe )
		goto _test_eof149;
case 149:
	switch( (*p) ) {
		case 13u: goto st103;
		case 32u: goto st103;
		case 82u: goto st150;
	}
	if ( 9u <= (*p) && (*p) <= 10u )
		goto st103;
	goto st0;
st150:
	if ( ++p == pe )
		goto _test_eof150;
case 150:
	if ( (*p) == 69u )
		goto st151;
	goto st0;
st151:
	if ( ++p == pe )
		goto _test_eof151;
case 151:
	if ( (*p) == 70u )
		goto st152;
	goto st0;
st152:
	if ( ++p == pe )
		goto _test_eof152;
case 152:
	switch( (*p) ) {
		case 13u: goto st103;
		case 32u: goto st103;
		case 83u: goto st102;
	}
	if ( 9u <= (*p) && (*p) <= 10u )
		goto st103;
	goto st0;
st153:
	if ( ++p == pe )
		goto _test_eof153;
case 153:
	switch( (*p) ) {
		case 77u: goto st154;
		case 79u: goto st159;
	}
	goto st0;
st154:
	if ( ++p == pe )
		goto _test_eof154;
case 154:
	if ( (*p) == 84u )
		goto st155;
	goto st0;
st155:
	if ( ++p == pe )
		goto _test_eof155;
case 155:
	if ( (*p) == 79u )
		goto st156;
	goto st0;
st156:
	if ( ++p == pe )
		goto _test_eof156;
case 156:
	if ( (*p) == 75u )
		goto st157;
	goto st0;
st157:
	if ( ++p == pe )
		goto _test_eof157;
case 157:
	if ( (*p) == 69u )
		goto st158;
	goto st0;
st158:
	if ( ++p == pe )
		goto _test_eof158;
case 158:
	if ( (*p) == 78u )
		goto st152;
	goto st0;
st159:
	if ( ++p == pe )
		goto _test_eof159;
case 159:
	if ( (*p) == 84u )
		goto st160;
	goto st0;
st160:
	if ( ++p == pe )
		goto _test_eof160;
case 160:
	if ( (*p) == 65u )
		goto st161;
	goto st0;
st161:
	if ( ++p == pe )
		goto _test_eof161;
case 161:
	if ( (*p) == 84u )
		goto st162;
	goto st0;
st162:
	if ( ++p == pe )
		goto _test_eof162;
case 162:
	if ( (*p) == 73u )
		goto st163;
	goto st0;
st163:
	if ( ++p == pe )
		goto _test_eof163;
case 163:
	if ( (*p) == 79u )
		goto st164;
	goto st0;
st164:
	if ( ++p == pe )
		goto _test_eof164;
case 164:
	if ( (*p) == 78u )
		goto st165;
	goto st0;
st165:
	if ( ++p == pe )
		goto _test_eof165;
case 165:
	switch( (*p) ) {
		case 13u: goto st166;
		case 32u: goto st166;
	}
	if ( 9u <= (*p) && (*p) <= 10u )
		goto st166;
	goto st0;
st166:
	if ( ++p == pe )
		goto _test_eof166;
case 166:
	switch( (*p) ) {
		case 13u: goto st166;
		case 32u: goto st166;
		case 40u: goto st167;
	}
	if ( 9u <= (*p) && (*p) <= 10u )
		goto st166;
	goto st0;
st167:
	if ( ++p == pe )
		goto _test_eof167;
case 167:
	switch( (*p) ) {
		case 13u: goto st167;
		case 32u: goto st167;
		case 58u: goto st168;
		case 95u: goto st168;
	}
	if ( (*p) < 65u ) {
		if ( 9u <= (*p) && (*p) <= 10u )
			goto st167;
	} else if ( (*p) > 90u ) {
		if ( 97u <= (*p) && (*p) <= 122u )
			goto st168;
	} else
		goto st168;
	goto st0;
st168:
	if ( ++p == pe )
		goto _test_eof168;
case 168:
	switch( (*p) ) {
		case 13u: goto st169;
		case 32u: goto st169;
		case 41u: goto st102;
		case 95u: goto st168;
		case 124u: goto st167;
	}
	if ( (*p) < 48u ) {
		if ( (*p) > 10u ) {
			if ( 45u <= (*p) && (*p) <= 46u )
				goto st168;
		} else if ( (*p) >= 9u )
			goto st169;
	} else if ( (*p) > 58u ) {
		if ( (*p) > 90u ) {
			if ( 97u <= (*p) && (*p) <= 122u )
				goto st168;
		} else if ( (*p) >= 65u )
			goto st168;
	} else
		goto st168;
	goto st0;
st169:
	if ( ++p == pe )
		goto _test_eof169;
case 169:
	switch( (*p) ) {
		case 13u: goto st169;
		case 32u: goto st169;
		case 41u: goto st102;
		case 124u: goto st167;
	}
	if ( 9u <= (*p) && (*p) <= 10u )
		goto st169;
	goto st0;
st170:
	if ( ++p == pe )
		goto _test_eof170;
case 170:
	switch( (*p) ) {
		case 76u: goto st171;
		case 78u: goto st200;
	}
	goto st0;
st171:
	if ( ++p == pe )
		goto _test_eof171;
case 171:
	if ( (*p) == 69u )
		goto st172;
	goto st0;
st172:
	if ( ++p == pe )
		goto _test_eof172;
case 172:
	if ( (*p) == 77u )
		goto st173;
	goto st0;
st173:
	if ( ++p == pe )
		goto _test_eof173;
case 173:
	if ( (*p) == 69u )
		goto st174;
	goto st0;
st174:
	if ( ++p == pe )
		goto _test_eof174;
case 174:
	if ( (*p) == 78u )
		goto st175;
	goto st0;
st175:
	if ( ++p == pe )
		goto _test_eof175;
case 175:
	if ( (*p) == 84u )
		goto st176;
	goto st0;
st176:
	if ( ++p == pe )
		goto _test_eof176;
case 176:
	switch( (*p) ) {
		case 13u: goto st177;
		case 32u: goto st177;
	}
	if ( 9u <= (*p) && (*p) <= 10u )
		goto st177;
	goto st0;
st177:
	if ( ++p == pe )
		goto _test_eof177;
case 177:
	switch( (*p) ) {
		case 13u: goto st177;
		case 32u: goto st177;
		case 58u: goto st178;
		case 95u: goto st178;
	}
	if ( (*p) < 65u ) {
		if ( 9u <= (*p) && (*p) <= 10u )
			goto st177;
	} else if ( (*p) > 90u ) {
		if ( 97u <= (*p) && (*p) <= 122u )
			goto st178;
	} else
		goto st178;
	goto st0;
st178:
	if ( ++p == pe )
		goto _test_eof178;
case 178:
	switch( (*p) ) {
		case 13u: goto st179;
		case 32u: goto st179;
		case 95u: goto st178;
	}
	if ( (*p) < 48u ) {
		if ( (*p) > 10u ) {
			if ( 45u <= (*p) && (*p) <= 46u )
				goto st178;
		} else if ( (*p) >= 9u )
			goto st179;
	} else if ( (*p) > 58u ) {
		if ( (*p) > 90u ) {
			if ( 97u <= (*p) && (*p) <= 122u )
				goto st178;
		} else if ( (*p) >= 65u )
			goto st178;
	} else
		goto st178;
	goto st0;
st179:
	if ( ++p == pe )
		goto _test_eof179;
case 179:
	switch( (*p) ) {
		case 13u: goto st179;
		case 32u: goto st179;
		case 40u: goto tr195;
		case 65u: goto st195;
		case 69u: goto st197;
	}
	if ( 9u <= (*p) && (*p) <= 10u )
		goto st179;
	goto st0;
tr195:
#line 91 "../src/xml.ragel"
	{ { pre_push(); { this->m_stack[ this->m_top++] = 180; goto st378;}} }
	goto st180;
st180:
	if ( ++p == pe )
		goto _test_eof180;
case 180:
#line 2655 "src/xml.cpp"
	switch( (*p) ) {
		case 13u: goto st181;
		case 32u: goto st181;
		case 35u: goto st182;
		case 62u: goto st78;
		case 63u: goto st190;
	}
	if ( (*p) > 10u ) {
		if ( 42u <= (*p) && (*p) <= 43u )
			goto st190;
	} else if ( (*p) >= 9u )
		goto st181;
	goto st0;
st181:
	if ( ++p == pe )
		goto _test_eof181;
case 181:
	switch( (*p) ) {
		case 13u: goto st181;
		case 32u: goto st181;
		case 35u: goto st182;
		case 62u: goto st78;
	}
	if ( 9u <= (*p) && (*p) <= 10u )
		goto st181;
	goto st0;
st182:
	if ( ++p == pe )
		goto _test_eof182;
case 182:
	if ( (*p) == 80u )
		goto st183;
	goto st0;
st183:
	if ( ++p == pe )
		goto _test_eof183;
case 183:
	if ( (*p) == 67u )
		goto st184;
	goto st0;
st184:
	if ( ++p == pe )
		goto _test_eof184;
case 184:
	if ( (*p) == 68u )
		goto st185;
	goto st0;
st185:
	if ( ++p == pe )
		goto _test_eof185;
case 185:
	if ( (*p) == 65u )
		goto st186;
	goto st0;
st186:
	if ( ++p == pe )
		goto _test_eof186;
case 186:
	if ( (*p) == 84u )
		goto st187;
	goto st0;
st187:
	if ( ++p == pe )
		goto _test_eof187;
case 187:
	if ( (*p) == 65u )
		goto st188;
	goto st0;
st188:
	if ( ++p == pe )
		goto _test_eof188;
case 188:
	switch( (*p) ) {
		case 13u: goto st188;
		case 32u: goto st188;
		case 41u: goto st189;
		case 124u: goto st191;
	}
	if ( 9u <= (*p) && (*p) <= 10u )
		goto st188;
	goto st0;
st189:
	if ( ++p == pe )
		goto _test_eof189;
case 189:
	switch( (*p) ) {
		case 13u: goto st190;
		case 32u: goto st190;
		case 42u: goto st190;
		case 62u: goto st78;
	}
	if ( 9u <= (*p) && (*p) <= 10u )
		goto st190;
	goto st0;
st190:
	if ( ++p == pe )
		goto _test_eof190;
case 190:
	switch( (*p) ) {
		case 13u: goto st190;
		case 32u: goto st190;
		case 62u: goto st78;
	}
	if ( 9u <= (*p) && (*p) <= 10u )
		goto st190;
	goto st0;
st191:
	if ( ++p == pe )
		goto _test_eof191;
case 191:
	switch( (*p) ) {
		case 13u: goto st191;
		case 32u: goto st191;
		case 58u: goto st192;
		case 95u: goto st192;
	}
	if ( (*p) < 65u ) {
		if ( 9u <= (*p) && (*p) <= 10u )
			goto st191;
	} else if ( (*p) > 90u ) {
		if ( 97u <= (*p) && (*p) <= 122u )
			goto st192;
	} else
		goto st192;
	goto st0;
st192:
	if ( ++p == pe )
		goto _test_eof192;
case 192:
	switch( (*p) ) {
		case 13u: goto st193;
		case 32u: goto st193;
		case 41u: goto st194;
		case 95u: goto st192;
		case 124u: goto st191;
	}
	if ( (*p) < 48u ) {
		if ( (*p) > 10u ) {
			if ( 45u <= (*p) && (*p) <= 46u )
				goto st192;
		} else if ( (*p) >= 9u )
			goto st193;
	} else if ( (*p) > 58u ) {
		if ( (*p) > 90u ) {
			if ( 97u <= (*p) && (*p) <= 122u )
				goto st192;
		} else if ( (*p) >= 65u )
			goto st192;
	} else
		goto st192;
	goto st0;
st193:
	if ( ++p == pe )
		goto _test_eof193;
case 193:
	switch( (*p) ) {
		case 13u: goto st193;
		case 32u: goto st193;
		case 41u: goto st194;
		case 124u: goto st191;
	}
	if ( 9u <= (*p) && (*p) <= 10u )
		goto st193;
	goto st0;
st194:
	if ( ++p == pe )
		goto _test_eof194;
case 194:
	if ( (*p) == 42u )
		goto st190;
	goto st0;
st195:
	if ( ++p == pe )
		goto _test_eof195;
case 195:
	if ( (*p) == 78u )
		goto st196;
	goto st0;
st196:
	if ( ++p == pe )
		goto _test_eof196;
case 196:
	if ( (*p) == 89u )
		goto st190;
	goto st0;
st197:
	if ( ++p == pe )
		goto _test_eof197;
case 197:
	if ( (*p) == 77u )
		goto st198;
	goto st0;
st198:
	if ( ++p == pe )
		goto _test_eof198;
case 198:
	if ( (*p) == 80u )
		goto st199;
	goto st0;
st199:
	if ( ++p == pe )
		goto _test_eof199;
case 199:
	if ( (*p) == 84u )
		goto st196;
	goto st0;
st200:
	if ( ++p == pe )
		goto _test_eof200;
case 200:
	if ( (*p) == 84u )
		goto st201;
	goto st0;
st201:
	if ( ++p == pe )
		goto _test_eof201;
case 201:
	if ( (*p) == 73u )
		goto st202;
	goto st0;
st202:
	if ( ++p == pe )
		goto _test_eof202;
case 202:
	if ( (*p) == 84u )
		goto st203;
	goto st0;
st203:
	if ( ++p == pe )
		goto _test_eof203;
case 203:
	if ( (*p) == 89u )
		goto st204;
	goto st0;
st204:
	if ( ++p == pe )
		goto _test_eof204;
case 204:
	switch( (*p) ) {
		case 13u: goto st205;
		case 32u: goto st205;
	}
	if ( 9u <= (*p) && (*p) <= 10u )
		goto st205;
	goto st0;
st205:
	if ( ++p == pe )
		goto _test_eof205;
case 205:
	switch( (*p) ) {
		case 13u: goto st205;
		case 32u: goto st205;
		case 37u: goto st206;
		case 58u: goto st244;
		case 95u: goto st244;
	}
	if ( (*p) < 65u ) {
		if ( 9u <= (*p) && (*p) <= 10u )
			goto st205;
	} else if ( (*p) > 90u ) {
		if ( 97u <= (*p) && (*p) <= 122u )
			goto st244;
	} else
		goto st244;
	goto st0;
st206:
	if ( ++p == pe )
		goto _test_eof206;
case 206:
	switch( (*p) ) {
		case 13u: goto st207;
		case 32u: goto st207;
	}
	if ( 9u <= (*p) && (*p) <= 10u )
		goto st207;
	goto st0;
st207:
	if ( ++p == pe )
		goto _test_eof207;
case 207:
	switch( (*p) ) {
		case 13u: goto st207;
		case 32u: goto st207;
		case 58u: goto st208;
		case 95u: goto st208;
	}
	if ( (*p) < 65u ) {
		if ( 9u <= (*p) && (*p) <= 10u )
			goto st207;
	} else if ( (*p) > 90u ) {
		if ( 97u <= (*p) && (*p) <= 122u )
			goto st208;
	} else
		goto st208;
	goto st0;
st208:
	if ( ++p == pe )
		goto _test_eof208;
case 208:
	switch( (*p) ) {
		case 13u: goto st209;
		case 32u: goto st209;
		case 95u: goto st208;
	}
	if ( (*p) < 48u ) {
		if ( (*p) > 10u ) {
			if ( 45u <= (*p) && (*p) <= 46u )
				goto st208;
		} else if ( (*p) >= 9u )
			goto st209;
	} else if ( (*p) > 58u ) {
		if ( (*p) > 90u ) {
			if ( 97u <= (*p) && (*p) <= 122u )
				goto st208;
		} else if ( (*p) >= 65u )
			goto st208;
	} else
		goto st208;
	goto st0;
st209:
	if ( ++p == pe )
		goto _test_eof209;
case 209:
	switch( (*p) ) {
		case 13u: goto st209;
		case 32u: goto st209;
		case 34u: goto st210;
		case 39u: goto st218;
		case 80u: goto st226;
		case 83u: goto st239;
	}
	if ( 9u <= (*p) && (*p) <= 10u )
		goto st209;
	goto st0;
tr235:
#line 31 "../src/xml.ragel"
	{ pop(); }
	goto st210;
st210:
	if ( ++p == pe )
		goto _test_eof210;
case 210:
#line 2998 "src/xml.cpp"
	switch( (*p) ) {
		case 34u: goto st190;
		case 37u: goto st211;
		case 38u: goto tr230;
	}
	goto st210;
st211:
	if ( ++p == pe )
		goto _test_eof211;
case 211:
	switch( (*p) ) {
		case 58u: goto st212;
		case 95u: goto st212;
	}
	if ( (*p) > 90u ) {
		if ( 97u <= (*p) && (*p) <= 122u )
			goto st212;
	} else if ( (*p) >= 65u )
		goto st212;
	goto st0;
st212:
	if ( ++p == pe )
		goto _test_eof212;
case 212:
	switch( (*p) ) {
		case 59u: goto st210;
		case 95u: goto st212;
	}
	if ( (*p) < 48u ) {
		if ( 45u <= (*p) && (*p) <= 46u )
			goto st212;
	} else if ( (*p) > 58u ) {
		if ( (*p) > 90u ) {
			if ( 97u <= (*p) && (*p) <= 122u )
				goto st212;
		} else if ( (*p) >= 65u )
			goto st212;
	} else
		goto st212;
	goto st0;
tr230:
#line 30 "../src/xml.ragel"
	{ m_output.push('['); ++m_rec; }
	goto st213;
st213:
	if ( ++p == pe )
		goto _test_eof213;
case 213:
#line 3047 "src/xml.cpp"
	switch( (*p) ) {
		case 35u: goto st214;
		case 58u: goto st212;
		case 95u: goto st212;
	}
	if ( (*p) > 90u ) {
		if ( 97u <= (*p) && (*p) <= 122u )
			goto st212;
	} else if ( (*p) >= 65u )
		goto st212;
	goto st0;
st214:
	if ( ++p == pe )
		goto _test_eof214;
case 214:
	if ( (*p) == 120u )
		goto st216;
	if ( 48u <= (*p) && (*p) <= 57u )
		goto st215;
	goto st0;
st215:
	if ( ++p == pe )
		goto _test_eof215;
case 215:
	if ( (*p) == 59u )
		goto tr235;
	if ( 48u <= (*p) && (*p) <= 57u )
		goto st215;
	goto st0;
st216:
	if ( ++p == pe )
		goto _test_eof216;
case 216:
	if ( (*p) < 65u ) {
		if ( 48u <= (*p) && (*p) <= 57u )
			goto st217;
	} else if ( (*p) > 70u ) {
		if ( 97u <= (*p) && (*p) <= 102u )
			goto st217;
	} else
		goto st217;
	goto st0;
st217:
	if ( ++p == pe )
		goto _test_eof217;
case 217:
	if ( (*p) == 59u )
		goto tr235;
	if ( (*p) < 65u ) {
		if ( 48u <= (*p) && (*p) <= 57u )
			goto st217;
	} else if ( (*p) > 70u ) {
		if ( 97u <= (*p) && (*p) <= 102u )
			goto st217;
	} else
		goto st217;
	goto st0;
tr243:
#line 31 "../src/xml.ragel"
	{ pop(); }
	goto st218;
st218:
	if ( ++p == pe )
		goto _test_eof218;
case 218:
#line 3113 "src/xml.cpp"
	switch( (*p) ) {
		case 37u: goto st219;
		case 38u: goto tr238;
		case 39u: goto st190;
	}
	goto st218;
st219:
	if ( ++p == pe )
		goto _test_eof219;
case 219:
	switch( (*p) ) {
		case 58u: goto st220;
		case 95u: goto st220;
	}
	if ( (*p) > 90u ) {
		if ( 97u <= (*p) && (*p) <= 122u )
			goto st220;
	} else if ( (*p) >= 65u )
		goto st220;
	goto st0;
st220:
	if ( ++p == pe )
		goto _test_eof220;
case 220:
	switch( (*p) ) {
		case 59u: goto st218;
		case 95u: goto st220;
	}
	if ( (*p) < 48u ) {
		if ( 45u <= (*p) && (*p) <= 46u )
			goto st220;
	} else if ( (*p) > 58u ) {
		if ( (*p) > 90u ) {
			if ( 97u <= (*p) && (*p) <= 122u )
				goto st220;
		} else if ( (*p) >= 65u )
			goto st220;
	} else
		goto st220;
	goto st0;
tr238:
#line 30 "../src/xml.ragel"
	{ m_output.push('['); ++m_rec; }
	goto st221;
st221:
	if ( ++p == pe )
		goto _test_eof221;
case 221:
#line 3162 "src/xml.cpp"
	switch( (*p) ) {
		case 35u: goto st222;
		case 58u: goto st220;
		case 95u: goto st220;
	}
	if ( (*p) > 90u ) {
		if ( 97u <= (*p) && (*p) <= 122u )
			goto st220;
	} else if ( (*p) >= 65u )
		goto st220;
	goto st0;
st222:
	if ( ++p == pe )
		goto _test_eof222;
case 222:
	if ( (*p) == 120u )
		goto st224;
	if ( 48u <= (*p) && (*p) <= 57u )
		goto st223;
	goto st0;
st223:
	if ( ++p == pe )
		goto _test_eof223;
case 223:
	if ( (*p) == 59u )
		goto tr243;
	if ( 48u <= (*p) && (*p) <= 57u )
		goto st223;
	goto st0;
st224:
	if ( ++p == pe )
		goto _test_eof224;
case 224:
	if ( (*p) < 65u ) {
		if ( 48u <= (*p) && (*p) <= 57u )
			goto st225;
	} else if ( (*p) > 70u ) {
		if ( 97u <= (*p) && (*p) <= 102u )
			goto st225;
	} else
		goto st225;
	goto st0;
st225:
	if ( ++p == pe )
		goto _test_eof225;
case 225:
	if ( (*p) == 59u )
		goto tr243;
	if ( (*p) < 65u ) {
		if ( 48u <= (*p) && (*p) <= 57u )
			goto st225;
	} else if ( (*p) > 70u ) {
		if ( 97u <= (*p) && (*p) <= 102u )
			goto st225;
	} else
		goto st225;
	goto st0;
st226:
	if ( ++p == pe )
		goto _test_eof226;
case 226:
	if ( (*p) == 85u )
		goto st227;
	goto st0;
st227:
	if ( ++p == pe )
		goto _test_eof227;
case 227:
	if ( (*p) == 66u )
		goto st228;
	goto st0;
st228:
	if ( ++p == pe )
		goto _test_eof228;
case 228:
	if ( (*p) == 76u )
		goto st229;
	goto st0;
st229:
	if ( ++p == pe )
		goto _test_eof229;
case 229:
	if ( (*p) == 73u )
		goto st230;
	goto st0;
st230:
	if ( ++p == pe )
		goto _test_eof230;
case 230:
	if ( (*p) == 67u )
		goto st231;
	goto st0;
st231:
	if ( ++p == pe )
		goto _test_eof231;
case 231:
	switch( (*p) ) {
		case 13u: goto st232;
		case 32u: goto st232;
	}
	if ( 9u <= (*p) && (*p) <= 10u )
		goto st232;
	goto st0;
st232:
	if ( ++p == pe )
		goto _test_eof232;
case 232:
	switch( (*p) ) {
		case 13u: goto st232;
		case 32u: goto st232;
		case 34u: goto st233;
		case 39u: goto st238;
	}
	if ( 9u <= (*p) && (*p) <= 10u )
		goto st232;
	goto st0;
st233:
	if ( ++p == pe )
		goto _test_eof233;
case 233:
	switch( (*p) ) {
		case 10u: goto st233;
		case 13u: goto st233;
		case 34u: goto st234;
		case 61u: goto st233;
		case 95u: goto st233;
	}
	if ( (*p) < 39u ) {
		if ( 32u <= (*p) && (*p) <= 37u )
			goto st233;
	} else if ( (*p) > 59u ) {
		if ( (*p) > 90u ) {
			if ( 97u <= (*p) && (*p) <= 122u )
				goto st233;
		} else if ( (*p) >= 63u )
			goto st233;
	} else
		goto st233;
	goto st0;
st234:
	if ( ++p == pe )
		goto _test_eof234;
case 234:
	switch( (*p) ) {
		case 13u: goto st235;
		case 32u: goto st235;
	}
	if ( 9u <= (*p) && (*p) <= 10u )
		goto st235;
	goto st0;
st235:
	if ( ++p == pe )
		goto _test_eof235;
case 235:
	switch( (*p) ) {
		case 13u: goto st235;
		case 32u: goto st235;
		case 34u: goto st236;
		case 39u: goto st237;
	}
	if ( 9u <= (*p) && (*p) <= 10u )
		goto st235;
	goto st0;
st236:
	if ( ++p == pe )
		goto _test_eof236;
case 236:
	if ( (*p) == 34u )
		goto st190;
	goto st236;
st237:
	if ( ++p == pe )
		goto _test_eof237;
case 237:
	if ( (*p) == 39u )
		goto st190;
	goto st237;
st238:
	if ( ++p == pe )
		goto _test_eof238;
case 238:
	switch( (*p) ) {
		case 10u: goto st238;
		case 13u: goto st238;
		case 39u: goto st234;
		case 61u: goto st238;
		case 95u: goto st238;
	}
	if ( (*p) < 40u ) {
		if ( (*p) > 33u ) {
			if ( 35u <= (*p) && (*p) <= 37u )
				goto st238;
		} else if ( (*p) >= 32u )
			goto st238;
	} else if ( (*p) > 59u ) {
		if ( (*p) > 90u ) {
			if ( 97u <= (*p) && (*p) <= 122u )
				goto st238;
		} else if ( (*p) >= 63u )
			goto st238;
	} else
		goto st238;
	goto st0;
st239:
	if ( ++p == pe )
		goto _test_eof239;
case 239:
	if ( (*p) == 89u )
		goto st240;
	goto st0;
st240:
	if ( ++p == pe )
		goto _test_eof240;
case 240:
	if ( (*p) == 83u )
		goto st241;
	goto st0;
st241:
	if ( ++p == pe )
		goto _test_eof241;
case 241:
	if ( (*p) == 84u )
		goto st242;
	goto st0;
st242:
	if ( ++p == pe )
		goto _test_eof242;
case 242:
	if ( (*p) == 69u )
		goto st243;
	goto st0;
st243:
	if ( ++p == pe )
		goto _test_eof243;
case 243:
	if ( (*p) == 77u )
		goto st234;
	goto st0;
st244:
	if ( ++p == pe )
		goto _test_eof244;
case 244:
	switch( (*p) ) {
		case 13u: goto st245;
		case 32u: goto st245;
		case 95u: goto st244;
	}
	if ( (*p) < 48u ) {
		if ( (*p) > 10u ) {
			if ( 45u <= (*p) && (*p) <= 46u )
				goto st244;
		} else if ( (*p) >= 9u )
			goto st245;
	} else if ( (*p) > 58u ) {
		if ( (*p) > 90u ) {
			if ( 97u <= (*p) && (*p) <= 122u )
				goto st244;
		} else if ( (*p) >= 65u )
			goto st244;
	} else
		goto st244;
	goto st0;
st245:
	if ( ++p == pe )
		goto _test_eof245;
case 245:
	switch( (*p) ) {
		case 13u: goto st245;
		case 32u: goto st245;
		case 34u: goto st210;
		case 39u: goto st218;
		case 80u: goto st246;
		case 83u: goto st268;
	}
	if ( 9u <= (*p) && (*p) <= 10u )
		goto st245;
	goto st0;
st246:
	if ( ++p == pe )
		goto _test_eof246;
case 246:
	if ( (*p) == 85u )
		goto st247;
	goto st0;
st247:
	if ( ++p == pe )
		goto _test_eof247;
case 247:
	if ( (*p) == 66u )
		goto st248;
	goto st0;
st248:
	if ( ++p == pe )
		goto _test_eof248;
case 248:
	if ( (*p) == 76u )
		goto st249;
	goto st0;
st249:
	if ( ++p == pe )
		goto _test_eof249;
case 249:
	if ( (*p) == 73u )
		goto st250;
	goto st0;
st250:
	if ( ++p == pe )
		goto _test_eof250;
case 250:
	if ( (*p) == 67u )
		goto st251;
	goto st0;
st251:
	if ( ++p == pe )
		goto _test_eof251;
case 251:
	switch( (*p) ) {
		case 13u: goto st252;
		case 32u: goto st252;
	}
	if ( 9u <= (*p) && (*p) <= 10u )
		goto st252;
	goto st0;
st252:
	if ( ++p == pe )
		goto _test_eof252;
case 252:
	switch( (*p) ) {
		case 13u: goto st252;
		case 32u: goto st252;
		case 34u: goto st253;
		case 39u: goto st267;
	}
	if ( 9u <= (*p) && (*p) <= 10u )
		goto st252;
	goto st0;
st253:
	if ( ++p == pe )
		goto _test_eof253;
case 253:
	switch( (*p) ) {
		case 10u: goto st253;
		case 13u: goto st253;
		case 34u: goto st254;
		case 61u: goto st253;
		case 95u: goto st253;
	}
	if ( (*p) < 39u ) {
		if ( 32u <= (*p) && (*p) <= 37u )
			goto st253;
	} else if ( (*p) > 59u ) {
		if ( (*p) > 90u ) {
			if ( 97u <= (*p) && (*p) <= 122u )
				goto st253;
		} else if ( (*p) >= 63u )
			goto st253;
	} else
		goto st253;
	goto st0;
st254:
	if ( ++p == pe )
		goto _test_eof254;
case 254:
	switch( (*p) ) {
		case 13u: goto st255;
		case 32u: goto st255;
	}
	if ( 9u <= (*p) && (*p) <= 10u )
		goto st255;
	goto st0;
st255:
	if ( ++p == pe )
		goto _test_eof255;
case 255:
	switch( (*p) ) {
		case 13u: goto st255;
		case 32u: goto st255;
		case 34u: goto st256;
		case 39u: goto st266;
	}
	if ( 9u <= (*p) && (*p) <= 10u )
		goto st255;
	goto st0;
st256:
	if ( ++p == pe )
		goto _test_eof256;
case 256:
	if ( (*p) == 34u )
		goto st257;
	goto st256;
st257:
	if ( ++p == pe )
		goto _test_eof257;
case 257:
	switch( (*p) ) {
		case 13u: goto st258;
		case 32u: goto st258;
		case 62u: goto st78;
	}
	if ( 9u <= (*p) && (*p) <= 10u )
		goto st258;
	goto st0;
st258:
	if ( ++p == pe )
		goto _test_eof258;
case 258:
	switch( (*p) ) {
		case 13u: goto st258;
		case 32u: goto st258;
		case 62u: goto st78;
		case 78u: goto st259;
	}
	if ( 9u <= (*p) && (*p) <= 10u )
		goto st258;
	goto st0;
st259:
	if ( ++p == pe )
		goto _test_eof259;
case 259:
	if ( (*p) == 68u )
		goto st260;
	goto st0;
st260:
	if ( ++p == pe )
		goto _test_eof260;
case 260:
	if ( (*p) == 65u )
		goto st261;
	goto st0;
st261:
	if ( ++p == pe )
		goto _test_eof261;
case 261:
	if ( (*p) == 84u )
		goto st262;
	goto st0;
st262:
	if ( ++p == pe )
		goto _test_eof262;
case 262:
	if ( (*p) == 65u )
		goto st263;
	goto st0;
st263:
	if ( ++p == pe )
		goto _test_eof263;
case 263:
	switch( (*p) ) {
		case 13u: goto st264;
		case 32u: goto st264;
	}
	if ( 9u <= (*p) && (*p) <= 10u )
		goto st264;
	goto st0;
st264:
	if ( ++p == pe )
		goto _test_eof264;
case 264:
	switch( (*p) ) {
		case 13u: goto st264;
		case 32u: goto st264;
		case 58u: goto st265;
		case 95u: goto st265;
	}
	if ( (*p) < 65u ) {
		if ( 9u <= (*p) && (*p) <= 10u )
			goto st264;
	} else if ( (*p) > 90u ) {
		if ( 97u <= (*p) && (*p) <= 122u )
			goto st265;
	} else
		goto st265;
	goto st0;
st265:
	if ( ++p == pe )
		goto _test_eof265;
case 265:
	switch( (*p) ) {
		case 13u: goto st190;
		case 32u: goto st190;
		case 62u: goto st78;
		case 95u: goto st265;
	}
	if ( (*p) < 48u ) {
		if ( (*p) > 10u ) {
			if ( 45u <= (*p) && (*p) <= 46u )
				goto st265;
		} else if ( (*p) >= 9u )
			goto st190;
	} else if ( (*p) > 58u ) {
		if ( (*p) > 90u ) {
			if ( 97u <= (*p) && (*p) <= 122u )
				goto st265;
		} else if ( (*p) >= 65u )
			goto st265;
	} else
		goto st265;
	goto st0;
st266:
	if ( ++p == pe )
		goto _test_eof266;
case 266:
	if ( (*p) == 39u )
		goto st257;
	goto st266;
st267:
	if ( ++p == pe )
		goto _test_eof267;
case 267:
	switch( (*p) ) {
		case 10u: goto st267;
		case 13u: goto st267;
		case 39u: goto st254;
		case 61u: goto st267;
		case 95u: goto st267;
	}
	if ( (*p) < 40u ) {
		if ( (*p) > 33u ) {
			if ( 35u <= (*p) && (*p) <= 37u )
				goto st267;
		} else if ( (*p) >= 32u )
			goto st267;
	} else if ( (*p) > 59u ) {
		if ( (*p) > 90u ) {
			if ( 97u <= (*p) && (*p) <= 122u )
				goto st267;
		} else if ( (*p) >= 63u )
			goto st267;
	} else
		goto st267;
	goto st0;
st268:
	if ( ++p == pe )
		goto _test_eof268;
case 268:
	if ( (*p) == 89u )
		goto st269;
	goto st0;
st269:
	if ( ++p == pe )
		goto _test_eof269;
case 269:
	if ( (*p) == 83u )
		goto st270;
	goto st0;
st270:
	if ( ++p == pe )
		goto _test_eof270;
case 270:
	if ( (*p) == 84u )
		goto st271;
	goto st0;
st271:
	if ( ++p == pe )
		goto _test_eof271;
case 271:
	if ( (*p) == 69u )
		goto st272;
	goto st0;
st272:
	if ( ++p == pe )
		goto _test_eof272;
case 272:
	if ( (*p) == 77u )
		goto st254;
	goto st0;
st273:
	if ( ++p == pe )
		goto _test_eof273;
case 273:
	if ( (*p) == 79u )
		goto st274;
	goto st0;
st274:
	if ( ++p == pe )
		goto _test_eof274;
case 274:
	if ( (*p) == 84u )
		goto st275;
	goto st0;
st275:
	if ( ++p == pe )
		goto _test_eof275;
case 275:
	if ( (*p) == 65u )
		goto st276;
	goto st0;
st276:
	if ( ++p == pe )
		goto _test_eof276;
case 276:
	if ( (*p) == 84u )
		goto st277;
	goto st0;
st277:
	if ( ++p == pe )
		goto _test_eof277;
case 277:
	if ( (*p) == 73u )
		goto st278;
	goto st0;
st278:
	if ( ++p == pe )
		goto _test_eof278;
case 278:
	if ( (*p) == 79u )
		goto st279;
	goto st0;
st279:
	if ( ++p == pe )
		goto _test_eof279;
case 279:
	if ( (*p) == 78u )
		goto st280;
	goto st0;
st280:
	if ( ++p == pe )
		goto _test_eof280;
case 280:
	switch( (*p) ) {
		case 13u: goto st281;
		case 32u: goto st281;
	}
	if ( 9u <= (*p) && (*p) <= 10u )
		goto st281;
	goto st0;
st281:
	if ( ++p == pe )
		goto _test_eof281;
case 281:
	switch( (*p) ) {
		case 13u: goto st281;
		case 32u: goto st281;
		case 58u: goto st282;
		case 95u: goto st282;
	}
	if ( (*p) < 65u ) {
		if ( 9u <= (*p) && (*p) <= 10u )
			goto st281;
	} else if ( (*p) > 90u ) {
		if ( 97u <= (*p) && (*p) <= 122u )
			goto st282;
	} else
		goto st282;
	goto st0;
st282:
	if ( ++p == pe )
		goto _test_eof282;
case 282:
	switch( (*p) ) {
		case 13u: goto st283;
		case 32u: goto st283;
		case 95u: goto st282;
	}
	if ( (*p) < 48u ) {
		if ( (*p) > 10u ) {
			if ( 45u <= (*p) && (*p) <= 46u )
				goto st282;
		} else if ( (*p) >= 9u )
			goto st283;
	} else if ( (*p) > 58u ) {
		if ( (*p) > 90u ) {
			if ( 97u <= (*p) && (*p) <= 122u )
				goto st282;
		} else if ( (*p) >= 65u )
			goto st282;
	} else
		goto st282;
	goto st0;
st283:
	if ( ++p == pe )
		goto _test_eof283;
case 283:
	switch( (*p) ) {
		case 13u: goto st283;
		case 32u: goto st283;
		case 80u: goto st284;
		case 83u: goto st239;
	}
	if ( 9u <= (*p) && (*p) <= 10u )
		goto st283;
	goto st0;
st284:
	if ( ++p == pe )
		goto _test_eof284;
case 284:
	if ( (*p) == 85u )
		goto st285;
	goto st0;
st285:
	if ( ++p == pe )
		goto _test_eof285;
case 285:
	if ( (*p) == 66u )
		goto st286;
	goto st0;
st286:
	if ( ++p == pe )
		goto _test_eof286;
case 286:
	if ( (*p) == 76u )
		goto st287;
	goto st0;
st287:
	if ( ++p == pe )
		goto _test_eof287;
case 287:
	if ( (*p) == 73u )
		goto st288;
	goto st0;
st288:
	if ( ++p == pe )
		goto _test_eof288;
case 288:
	if ( (*p) == 67u )
		goto st289;
	goto st0;
st289:
	if ( ++p == pe )
		goto _test_eof289;
case 289:
	switch( (*p) ) {
		case 13u: goto st290;
		case 32u: goto st290;
	}
	if ( 9u <= (*p) && (*p) <= 10u )
		goto st290;
	goto st0;
st290:
	if ( ++p == pe )
		goto _test_eof290;
case 290:
	switch( (*p) ) {
		case 13u: goto st290;
		case 32u: goto st290;
		case 34u: goto st291;
		case 39u: goto st294;
	}
	if ( 9u <= (*p) && (*p) <= 10u )
		goto st290;
	goto st0;
st291:
	if ( ++p == pe )
		goto _test_eof291;
case 291:
	switch( (*p) ) {
		case 10u: goto st291;
		case 13u: goto st291;
		case 34u: goto st292;
		case 61u: goto st291;
		case 95u: goto st291;
	}
	if ( (*p) < 39u ) {
		if ( 32u <= (*p) && (*p) <= 37u )
			goto st291;
	} else if ( (*p) > 59u ) {
		if ( (*p) > 90u ) {
			if ( 97u <= (*p) && (*p) <= 122u )
				goto st291;
		} else if ( (*p) >= 63u )
			goto st291;
	} else
		goto st291;
	goto st0;
st292:
	if ( ++p == pe )
		goto _test_eof292;
case 292:
	switch( (*p) ) {
		case 13u: goto st293;
		case 32u: goto st293;
		case 62u: goto st78;
	}
	if ( 9u <= (*p) && (*p) <= 10u )
		goto st293;
	goto st0;
st293:
	if ( ++p == pe )
		goto _test_eof293;
case 293:
	switch( (*p) ) {
		case 13u: goto st293;
		case 32u: goto st293;
		case 34u: goto st236;
		case 39u: goto st237;
		case 62u: goto st78;
	}
	if ( 9u <= (*p) && (*p) <= 10u )
		goto st293;
	goto st0;
st294:
	if ( ++p == pe )
		goto _test_eof294;
case 294:
	switch( (*p) ) {
		case 10u: goto st294;
		case 13u: goto st294;
		case 39u: goto st292;
		case 61u: goto st294;
		case 95u: goto st294;
	}
	if ( (*p) < 40u ) {
		if ( (*p) > 33u ) {
			if ( 35u <= (*p) && (*p) <= 37u )
				goto st294;
		} else if ( (*p) >= 32u )
			goto st294;
	} else if ( (*p) > 59u ) {
		if ( (*p) > 90u ) {
			if ( 97u <= (*p) && (*p) <= 122u )
				goto st294;
		} else if ( (*p) >= 63u )
			goto st294;
	} else
		goto st294;
	goto st0;
st295:
	if ( ++p == pe )
		goto _test_eof295;
case 295:
	switch( (*p) ) {
		case 58u: goto st296;
		case 88u: goto st299;
		case 95u: goto st296;
		case 120u: goto st299;
	}
	if ( (*p) > 90u ) {
		if ( 97u <= (*p) && (*p) <= 122u )
			goto st296;
	} else if ( (*p) >= 65u )
		goto st296;
	goto st0;
st296:
	if ( ++p == pe )
		goto _test_eof296;
case 296:
	switch( (*p) ) {
		case 13u: goto st297;
		case 32u: goto st297;
		case 63u: goto st86;
		case 95u: goto st296;
	}
	if ( (*p) < 48u ) {
		if ( (*p) > 10u ) {
			if ( 45u <= (*p) && (*p) <= 46u )
				goto st296;
		} else if ( (*p) >= 9u )
			goto st297;
	} else if ( (*p) > 58u ) {
		if ( (*p) > 90u ) {
			if ( 97u <= (*p) && (*p) <= 122u )
				goto st296;
		} else if ( (*p) >= 65u )
			goto st296;
	} else
		goto st296;
	goto st0;
st297:
	if ( ++p == pe )
		goto _test_eof297;
case 297:
	if ( (*p) == 63u )
		goto st298;
	goto st297;
st298:
	if ( ++p == pe )
		goto _test_eof298;
case 298:
	switch( (*p) ) {
		case 62u: goto st78;
		case 63u: goto st298;
	}
	goto st297;
st299:
	if ( ++p == pe )
		goto _test_eof299;
case 299:
	switch( (*p) ) {
		case 13u: goto st297;
		case 32u: goto st297;
		case 63u: goto st86;
		case 77u: goto st300;
		case 95u: goto st296;
		case 109u: goto st300;
	}
	if ( (*p) < 48u ) {
		if ( (*p) > 10u ) {
			if ( 45u <= (*p) && (*p) <= 46u )
				goto st296;
		} else if ( (*p) >= 9u )
			goto st297;
	} else if ( (*p) > 58u ) {
		if ( (*p) > 90u ) {
			if ( 97u <= (*p) && (*p) <= 122u )
				goto st296;
		} else if ( (*p) >= 65u )
			goto st296;
	} else
		goto st296;
	goto st0;
st300:
	if ( ++p == pe )
		goto _test_eof300;
case 300:
	switch( (*p) ) {
		case 13u: goto st297;
		case 32u: goto st297;
		case 63u: goto st86;
		case 76u: goto st301;
		case 95u: goto st296;
		case 108u: goto st301;
	}
	if ( (*p) < 48u ) {
		if ( (*p) > 10u ) {
			if ( 45u <= (*p) && (*p) <= 46u )
				goto st296;
		} else if ( (*p) >= 9u )
			goto st297;
	} else if ( (*p) > 58u ) {
		if ( (*p) > 90u ) {
			if ( 97u <= (*p) && (*p) <= 122u )
				goto st296;
		} else if ( (*p) >= 65u )
			goto st296;
	} else
		goto st296;
	goto st0;
st301:
	if ( ++p == pe )
		goto _test_eof301;
case 301:
	if ( (*p) == 95u )
		goto st296;
	if ( (*p) < 48u ) {
		if ( 45u <= (*p) && (*p) <= 46u )
			goto st296;
	} else if ( (*p) > 58u ) {
		if ( (*p) > 90u ) {
			if ( 97u <= (*p) && (*p) <= 122u )
				goto st296;
		} else if ( (*p) >= 65u )
			goto st296;
	} else
		goto st296;
	goto st0;
st302:
	if ( ++p == pe )
		goto _test_eof302;
case 302:
	switch( (*p) ) {
		case 13u: goto st302;
		case 32u: goto st302;
		case 62u: goto st19;
	}
	if ( 9u <= (*p) && (*p) <= 10u )
		goto st302;
	goto st0;
st303:
	if ( ++p == pe )
		goto _test_eof303;
case 303:
	if ( (*p) == 39u )
		goto st77;
	goto st303;
st304:
	if ( ++p == pe )
		goto _test_eof304;
case 304:
	switch( (*p) ) {
		case 10u: goto st304;
		case 13u: goto st304;
		case 39u: goto st74;
		case 61u: goto st304;
		case 95u: goto st304;
	}
	if ( (*p) < 40u ) {
		if ( (*p) > 33u ) {
			if ( 35u <= (*p) && (*p) <= 37u )
				goto st304;
		} else if ( (*p) >= 32u )
			goto st304;
	} else if ( (*p) > 59u ) {
		if ( (*p) > 90u ) {
			if ( 97u <= (*p) && (*p) <= 122u )
				goto st304;
		} else if ( (*p) >= 63u )
			goto st304;
	} else
		goto st304;
	goto st0;
st305:
	if ( ++p == pe )
		goto _test_eof305;
case 305:
	if ( (*p) == 89u )
		goto st306;
	goto st0;
st306:
	if ( ++p == pe )
		goto _test_eof306;
case 306:
	if ( (*p) == 83u )
		goto st307;
	goto st0;
st307:
	if ( ++p == pe )
		goto _test_eof307;
case 307:
	if ( (*p) == 84u )
		goto st308;
	goto st0;
st308:
	if ( ++p == pe )
		goto _test_eof308;
case 308:
	if ( (*p) == 69u )
		goto st309;
	goto st0;
st309:
	if ( ++p == pe )
		goto _test_eof309;
case 309:
	if ( (*p) == 77u )
		goto st74;
	goto st0;
st310:
	if ( ++p == pe )
		goto _test_eof310;
case 310:
	switch( (*p) ) {
		case 58u: goto st311;
		case 88u: goto st314;
		case 95u: goto st311;
		case 120u: goto st314;
	}
	if ( (*p) > 90u ) {
		if ( 97u <= (*p) && (*p) <= 122u )
			goto st311;
	} else if ( (*p) >= 65u )
		goto st311;
	goto st0;
st311:
	if ( ++p == pe )
		goto _test_eof311;
case 311:
	switch( (*p) ) {
		case 13u: goto st312;
		case 32u: goto st312;
		case 63u: goto st8;
		case 95u: goto st311;
	}
	if ( (*p) < 48u ) {
		if ( (*p) > 10u ) {
			if ( 45u <= (*p) && (*p) <= 46u )
				goto st311;
		} else if ( (*p) >= 9u )
			goto st312;
	} else if ( (*p) > 58u ) {
		if ( (*p) > 90u ) {
			if ( 97u <= (*p) && (*p) <= 122u )
				goto st311;
		} else if ( (*p) >= 65u )
			goto st311;
	} else
		goto st311;
	goto st0;
st312:
	if ( ++p == pe )
		goto _test_eof312;
case 312:
	if ( (*p) == 63u )
		goto st313;
	goto st312;
st313:
	if ( ++p == pe )
		goto _test_eof313;
case 313:
	switch( (*p) ) {
		case 62u: goto st2;
		case 63u: goto st313;
	}
	goto st312;
st314:
	if ( ++p == pe )
		goto _test_eof314;
case 314:
	switch( (*p) ) {
		case 13u: goto st312;
		case 32u: goto st312;
		case 63u: goto st8;
		case 77u: goto st315;
		case 95u: goto st311;
		case 109u: goto st315;
	}
	if ( (*p) < 48u ) {
		if ( (*p) > 10u ) {
			if ( 45u <= (*p) && (*p) <= 46u )
				goto st311;
		} else if ( (*p) >= 9u )
			goto st312;
	} else if ( (*p) > 58u ) {
		if ( (*p) > 90u ) {
			if ( 97u <= (*p) && (*p) <= 122u )
				goto st311;
		} else if ( (*p) >= 65u )
			goto st311;
	} else
		goto st311;
	goto st0;
st315:
	if ( ++p == pe )
		goto _test_eof315;
case 315:
	switch( (*p) ) {
		case 13u: goto st312;
		case 32u: goto st312;
		case 63u: goto st8;
		case 76u: goto st316;
		case 95u: goto st311;
		case 108u: goto st316;
	}
	if ( (*p) < 48u ) {
		if ( (*p) > 10u ) {
			if ( 45u <= (*p) && (*p) <= 46u )
				goto st311;
		} else if ( (*p) >= 9u )
			goto st312;
	} else if ( (*p) > 58u ) {
		if ( (*p) > 90u ) {
			if ( 97u <= (*p) && (*p) <= 122u )
				goto st311;
		} else if ( (*p) >= 65u )
			goto st311;
	} else
		goto st311;
	goto st0;
st316:
	if ( ++p == pe )
		goto _test_eof316;
case 316:
	if ( (*p) == 95u )
		goto st311;
	if ( (*p) < 48u ) {
		if ( 45u <= (*p) && (*p) <= 46u )
			goto st311;
	} else if ( (*p) > 58u ) {
		if ( (*p) > 90u ) {
			if ( 97u <= (*p) && (*p) <= 122u )
				goto st311;
		} else if ( (*p) >= 65u )
			goto st311;
	} else
		goto st311;
	goto st0;
st317:
	if ( ++p == pe )
		goto _test_eof317;
case 317:
	switch( (*p) ) {
		case 33u: goto st4;
		case 58u: goto st26;
		case 63u: goto st318;
		case 95u: goto st26;
	}
	if ( (*p) > 90u ) {
		if ( 97u <= (*p) && (*p) <= 122u )
			goto st26;
	} else if ( (*p) >= 65u )
		goto st26;
	goto st0;
st318:
	if ( ++p == pe )
		goto _test_eof318;
case 318:
	switch( (*p) ) {
		case 58u: goto st311;
		case 88u: goto st314;
		case 95u: goto st311;
		case 120u: goto st319;
	}
	if ( (*p) > 90u ) {
		if ( 97u <= (*p) && (*p) <= 122u )
			goto st311;
	} else if ( (*p) >= 65u )
		goto st311;
	goto st0;
st319:
	if ( ++p == pe )
		goto _test_eof319;
case 319:
	switch( (*p) ) {
		case 13u: goto st312;
		case 32u: goto st312;
		case 63u: goto st8;
		case 77u: goto st315;
		case 95u: goto st311;
		case 109u: goto st320;
	}
	if ( (*p) < 48u ) {
		if ( (*p) > 10u ) {
			if ( 45u <= (*p) && (*p) <= 46u )
				goto st311;
		} else if ( (*p) >= 9u )
			goto st312;
	} else if ( (*p) > 58u ) {
		if ( (*p) > 90u ) {
			if ( 97u <= (*p) && (*p) <= 122u )
				goto st311;
		} else if ( (*p) >= 65u )
			goto st311;
	} else
		goto st311;
	goto st0;
st320:
	if ( ++p == pe )
		goto _test_eof320;
case 320:
	switch( (*p) ) {
		case 13u: goto st312;
		case 32u: goto st312;
		case 63u: goto st8;
		case 76u: goto st316;
		case 95u: goto st311;
		case 108u: goto st321;
	}
	if ( (*p) < 48u ) {
		if ( (*p) > 10u ) {
			if ( 45u <= (*p) && (*p) <= 46u )
				goto st311;
		} else if ( (*p) >= 9u )
			goto st312;
	} else if ( (*p) > 58u ) {
		if ( (*p) > 90u ) {
			if ( 97u <= (*p) && (*p) <= 122u )
				goto st311;
		} else if ( (*p) >= 65u )
			goto st311;
	} else
		goto st311;
	goto st0;
st321:
	if ( ++p == pe )
		goto _test_eof321;
case 321:
	switch( (*p) ) {
		case 13u: goto st322;
		case 32u: goto st322;
		case 95u: goto st311;
	}
	if ( (*p) < 48u ) {
		if ( (*p) > 10u ) {
			if ( 45u <= (*p) && (*p) <= 46u )
				goto st311;
		} else if ( (*p) >= 9u )
			goto st322;
	} else if ( (*p) > 58u ) {
		if ( (*p) > 90u ) {
			if ( 97u <= (*p) && (*p) <= 122u )
				goto st311;
		} else if ( (*p) >= 65u )
			goto st311;
	} else
		goto st311;
	goto st0;
st322:
	if ( ++p == pe )
		goto _test_eof322;
case 322:
	switch( (*p) ) {
		case 13u: goto st322;
		case 32u: goto st322;
		case 118u: goto st323;
	}
	if ( 9u <= (*p) && (*p) <= 10u )
		goto st322;
	goto st0;
st323:
	if ( ++p == pe )
		goto _test_eof323;
case 323:
	if ( (*p) == 101u )
		goto st324;
	goto st0;
st324:
	if ( ++p == pe )
		goto _test_eof324;
case 324:
	if ( (*p) == 114u )
		goto st325;
	goto st0;
st325:
	if ( ++p == pe )
		goto _test_eof325;
case 325:
	if ( (*p) == 115u )
		goto st326;
	goto st0;
st326:
	if ( ++p == pe )
		goto _test_eof326;
case 326:
	if ( (*p) == 105u )
		goto st327;
	goto st0;
st327:
	if ( ++p == pe )
		goto _test_eof327;
case 327:
	if ( (*p) == 111u )
		goto st328;
	goto st0;
st328:
	if ( ++p == pe )
		goto _test_eof328;
case 328:
	if ( (*p) == 110u )
		goto st329;
	goto st0;
st329:
	if ( ++p == pe )
		goto _test_eof329;
case 329:
	switch( (*p) ) {
		case 13u: goto st329;
		case 32u: goto st329;
		case 61u: goto st330;
	}
	if ( 9u <= (*p) && (*p) <= 10u )
		goto st329;
	goto st0;
st330:
	if ( ++p == pe )
		goto _test_eof330;
case 330:
	switch( (*p) ) {
		case 13u: goto st330;
		case 32u: goto st330;
		case 34u: goto st331;
		case 39u: goto st374;
	}
	if ( 9u <= (*p) && (*p) <= 10u )
		goto st330;
	goto st0;
st331:
	if ( ++p == pe )
		goto _test_eof331;
case 331:
	if ( (*p) == 49u )
		goto st332;
	goto st0;
st332:
	if ( ++p == pe )
		goto _test_eof332;
case 332:
	if ( (*p) == 46u )
		goto st333;
	goto st0;
st333:
	if ( ++p == pe )
		goto _test_eof333;
case 333:
	if ( (*p) == 48u )
		goto st334;
	goto st0;
st334:
	if ( ++p == pe )
		goto _test_eof334;
case 334:
	if ( (*p) == 34u )
		goto st335;
	goto st0;
st335:
	if ( ++p == pe )
		goto _test_eof335;
case 335:
	switch( (*p) ) {
		case 13u: goto st336;
		case 32u: goto st336;
		case 63u: goto st8;
	}
	if ( 9u <= (*p) && (*p) <= 10u )
		goto st336;
	goto st0;
st336:
	if ( ++p == pe )
		goto _test_eof336;
case 336:
	switch( (*p) ) {
		case 13u: goto st336;
		case 32u: goto st336;
		case 63u: goto st8;
		case 101u: goto st337;
		case 115u: goto st350;
	}
	if ( 9u <= (*p) && (*p) <= 10u )
		goto st336;
	goto st0;
st337:
	if ( ++p == pe )
		goto _test_eof337;
case 337:
	if ( (*p) == 110u )
		goto st338;
	goto st0;
st338:
	if ( ++p == pe )
		goto _test_eof338;
case 338:
	if ( (*p) == 99u )
		goto st339;
	goto st0;
st339:
	if ( ++p == pe )
		goto _test_eof339;
case 339:
	if ( (*p) == 111u )
		goto st340;
	goto st0;
st340:
	if ( ++p == pe )
		goto _test_eof340;
case 340:
	if ( (*p) == 100u )
		goto st341;
	goto st0;
st341:
	if ( ++p == pe )
		goto _test_eof341;
case 341:
	if ( (*p) == 105u )
		goto st342;
	goto st0;
st342:
	if ( ++p == pe )
		goto _test_eof342;
case 342:
	if ( (*p) == 110u )
		goto st343;
	goto st0;
st343:
	if ( ++p == pe )
		goto _test_eof343;
case 343:
	if ( (*p) == 103u )
		goto st344;
	goto st0;
st344:
	if ( ++p == pe )
		goto _test_eof344;
case 344:
	switch( (*p) ) {
		case 13u: goto st344;
		case 32u: goto st344;
		case 61u: goto st345;
	}
	if ( 9u <= (*p) && (*p) <= 10u )
		goto st344;
	goto st0;
st345:
	if ( ++p == pe )
		goto _test_eof345;
case 345:
	switch( (*p) ) {
		case 13u: goto st345;
		case 32u: goto st345;
		case 34u: goto tr356;
		case 39u: goto tr357;
	}
	if ( 9u <= (*p) && (*p) <= 10u )
		goto st345;
	goto st0;
tr356:
#line 30 "../src/xml.ragel"
	{ m_output.push('['); ++m_rec; }
	goto st346;
st346:
	if ( ++p == pe )
		goto _test_eof346;
case 346:
#line 4642 "src/xml.cpp"
	if ( (*p) > 90u ) {
		if ( 97u <= (*p) && (*p) <= 122u )
			goto tr358;
	} else if ( (*p) >= 65u )
		goto tr358;
	goto st0;
tr358:
#line 31 "../src/xml.ragel"
	{ pop(); }
	goto st347;
st347:
	if ( ++p == pe )
		goto _test_eof347;
case 347:
#line 4657 "src/xml.cpp"
	switch( (*p) ) {
		case 34u: goto st348;
		case 95u: goto tr358;
	}
	if ( (*p) < 48u ) {
		if ( 45u <= (*p) && (*p) <= 46u )
			goto tr358;
	} else if ( (*p) > 57u ) {
		if ( (*p) > 90u ) {
			if ( 97u <= (*p) && (*p) <= 122u )
				goto tr358;
		} else if ( (*p) >= 65u )
			goto tr358;
	} else
		goto tr358;
	goto st0;
st348:
	if ( ++p == pe )
		goto _test_eof348;
case 348:
	switch( (*p) ) {
		case 13u: goto st349;
		case 32u: goto st349;
		case 63u: goto st8;
	}
	if ( 9u <= (*p) && (*p) <= 10u )
		goto st349;
	goto st0;
st349:
	if ( ++p == pe )
		goto _test_eof349;
case 349:
	switch( (*p) ) {
		case 13u: goto st349;
		case 32u: goto st349;
		case 63u: goto st8;
		case 115u: goto st350;
	}
	if ( 9u <= (*p) && (*p) <= 10u )
		goto st349;
	goto st0;
st350:
	if ( ++p == pe )
		goto _test_eof350;
case 350:
	if ( (*p) == 116u )
		goto st351;
	goto st0;
st351:
	if ( ++p == pe )
		goto _test_eof351;
case 351:
	if ( (*p) == 97u )
		goto st352;
	goto st0;
st352:
	if ( ++p == pe )
		goto _test_eof352;
case 352:
	if ( (*p) == 110u )
		goto st353;
	goto st0;
st353:
	if ( ++p == pe )
		goto _test_eof353;
case 353:
	if ( (*p) == 100u )
		goto st354;
	goto st0;
st354:
	if ( ++p == pe )
		goto _test_eof354;
case 354:
	if ( (*p) == 97u )
		goto st355;
	goto st0;
st355:
	if ( ++p == pe )
		goto _test_eof355;
case 355:
	if ( (*p) == 108u )
		goto st356;
	goto st0;
st356:
	if ( ++p == pe )
		goto _test_eof356;
case 356:
	if ( (*p) == 111u )
		goto st357;
	goto st0;
st357:
	if ( ++p == pe )
		goto _test_eof357;
case 357:
	if ( (*p) == 110u )
		goto st358;
	goto st0;
st358:
	if ( ++p == pe )
		goto _test_eof358;
case 358:
	if ( (*p) == 101u )
		goto st359;
	goto st0;
st359:
	if ( ++p == pe )
		goto _test_eof359;
case 359:
	switch( (*p) ) {
		case 13u: goto st359;
		case 32u: goto st359;
		case 61u: goto st360;
	}
	if ( 9u <= (*p) && (*p) <= 10u )
		goto st359;
	goto st0;
st360:
	if ( ++p == pe )
		goto _test_eof360;
case 360:
	switch( (*p) ) {
		case 13u: goto st360;
		case 32u: goto st360;
		case 34u: goto st361;
		case 39u: goto st367;
	}
	if ( 9u <= (*p) && (*p) <= 10u )
		goto st360;
	goto st0;
st361:
	if ( ++p == pe )
		goto _test_eof361;
case 361:
	switch( (*p) ) {
		case 110u: goto st362;
		case 121u: goto st365;
	}
	goto st0;
st362:
	if ( ++p == pe )
		goto _test_eof362;
case 362:
	if ( (*p) == 111u )
		goto st363;
	goto st0;
st363:
	if ( ++p == pe )
		goto _test_eof363;
case 363:
	if ( (*p) == 34u )
		goto st364;
	goto st0;
st364:
	if ( ++p == pe )
		goto _test_eof364;
case 364:
	switch( (*p) ) {
		case 13u: goto st364;
		case 32u: goto st364;
		case 63u: goto st8;
	}
	if ( 9u <= (*p) && (*p) <= 10u )
		goto st364;
	goto st0;
st365:
	if ( ++p == pe )
		goto _test_eof365;
case 365:
	if ( (*p) == 101u )
		goto st366;
	goto st0;
st366:
	if ( ++p == pe )
		goto _test_eof366;
case 366:
	if ( (*p) == 115u )
		goto st363;
	goto st0;
st367:
	if ( ++p == pe )
		goto _test_eof367;
case 367:
	switch( (*p) ) {
		case 110u: goto st368;
		case 121u: goto st370;
	}
	goto st0;
st368:
	if ( ++p == pe )
		goto _test_eof368;
case 368:
	if ( (*p) == 111u )
		goto st369;
	goto st0;
st369:
	if ( ++p == pe )
		goto _test_eof369;
case 369:
	if ( (*p) == 39u )
		goto st364;
	goto st0;
st370:
	if ( ++p == pe )
		goto _test_eof370;
case 370:
	if ( (*p) == 101u )
		goto st371;
	goto st0;
st371:
	if ( ++p == pe )
		goto _test_eof371;
case 371:
	if ( (*p) == 115u )
		goto st369;
	goto st0;
tr357:
#line 30 "../src/xml.ragel"
	{ m_output.push('['); ++m_rec; }
	goto st372;
st372:
	if ( ++p == pe )
		goto _test_eof372;
case 372:
#line 4881 "src/xml.cpp"
	if ( (*p) > 90u ) {
		if ( 97u <= (*p) && (*p) <= 122u )
			goto tr382;
	} else if ( (*p) >= 65u )
		goto tr382;
	goto st0;
tr382:
#line 31 "../src/xml.ragel"
	{ pop(); }
	goto st373;
st373:
	if ( ++p == pe )
		goto _test_eof373;
case 373:
#line 4896 "src/xml.cpp"
	switch( (*p) ) {
		case 39u: goto st348;
		case 95u: goto tr382;
	}
	if ( (*p) < 48u ) {
		if ( 45u <= (*p) && (*p) <= 46u )
			goto tr382;
	} else if ( (*p) > 57u ) {
		if ( (*p) > 90u ) {
			if ( 97u <= (*p) && (*p) <= 122u )
				goto tr382;
		} else if ( (*p) >= 65u )
			goto tr382;
	} else
		goto tr382;
	goto st0;
st374:
	if ( ++p == pe )
		goto _test_eof374;
case 374:
	if ( (*p) == 49u )
		goto st375;
	goto st0;
st375:
	if ( ++p == pe )
		goto _test_eof375;
case 375:
	if ( (*p) == 46u )
		goto st376;
	goto st0;
st376:
	if ( ++p == pe )
		goto _test_eof376;
case 376:
	if ( (*p) == 48u )
		goto st377;
	goto st0;
st377:
	if ( ++p == pe )
		goto _test_eof377;
case 377:
	if ( (*p) == 39u )
		goto st335;
	goto st0;
st378:
	if ( ++p == pe )
		goto _test_eof378;
case 378:
	switch( (*p) ) {
		case 13u: goto st378;
		case 32u: goto st378;
		case 40u: goto tr387;
		case 58u: goto st389;
		case 95u: goto st389;
	}
	if ( (*p) < 65u ) {
		if ( 9u <= (*p) && (*p) <= 10u )
			goto st378;
	} else if ( (*p) > 90u ) {
		if ( 97u <= (*p) && (*p) <= 122u )
			goto st389;
	} else
		goto st389;
	goto st0;
tr387:
#line 91 "../src/xml.ragel"
	{ { pre_push(); { this->m_stack[ this->m_top++] = 379; goto st378;}} }
	goto st379;
st379:
	if ( ++p == pe )
		goto _test_eof379;
case 379:
#line 4969 "src/xml.cpp"
	switch( (*p) ) {
		case 13u: goto st380;
		case 32u: goto st380;
		case 41u: goto tr390;
		case 44u: goto st381;
		case 63u: goto st380;
		case 124u: goto st385;
	}
	if ( (*p) > 10u ) {
		if ( 42u <= (*p) && (*p) <= 43u )
			goto st380;
	} else if ( (*p) >= 9u )
		goto st380;
	goto st0;
st380:
	if ( ++p == pe )
		goto _test_eof380;
case 380:
	switch( (*p) ) {
		case 13u: goto st380;
		case 32u: goto st380;
		case 41u: goto tr390;
		case 44u: goto st381;
		case 124u: goto st385;
	}
	if ( 9u <= (*p) && (*p) <= 10u )
		goto st380;
	goto st0;
tr390:
#line 29 "../src/xml.ragel"
	{ { this->m_cs =  this->m_stack[-- this->m_top];goto _again;} }
	goto st445;
st445:
	if ( ++p == pe )
		goto _test_eof445;
case 445:
#line 5006 "src/xml.cpp"
	goto st0;
st381:
	if ( ++p == pe )
		goto _test_eof381;
case 381:
	switch( (*p) ) {
		case 13u: goto st381;
		case 32u: goto st381;
		case 40u: goto tr393;
		case 58u: goto st384;
		case 95u: goto st384;
	}
	if ( (*p) < 65u ) {
		if ( 9u <= (*p) && (*p) <= 10u )
			goto st381;
	} else if ( (*p) > 90u ) {
		if ( 97u <= (*p) && (*p) <= 122u )
			goto st384;
	} else
		goto st384;
	goto st0;
tr393:
#line 91 "../src/xml.ragel"
	{ { pre_push(); { this->m_stack[ this->m_top++] = 382; goto st378;}} }
	goto st382;
st382:
	if ( ++p == pe )
		goto _test_eof382;
case 382:
#line 5036 "src/xml.cpp"
	switch( (*p) ) {
		case 13u: goto st383;
		case 32u: goto st383;
		case 41u: goto tr390;
		case 44u: goto st381;
		case 63u: goto st383;
	}
	if ( (*p) > 10u ) {
		if ( 42u <= (*p) && (*p) <= 43u )
			goto st383;
	} else if ( (*p) >= 9u )
		goto st383;
	goto st0;
st383:
	if ( ++p == pe )
		goto _test_eof383;
case 383:
	switch( (*p) ) {
		case 13u: goto st383;
		case 32u: goto st383;
		case 41u: goto tr390;
		case 44u: goto st381;
	}
	if ( 9u <= (*p) && (*p) <= 10u )
		goto st383;
	goto st0;
st384:
	if ( ++p == pe )
		goto _test_eof384;
case 384:
	switch( (*p) ) {
		case 13u: goto st383;
		case 32u: goto st383;
		case 41u: goto tr390;
		case 44u: goto st381;
		case 63u: goto st383;
		case 95u: goto st384;
	}
	if ( (*p) < 45u ) {
		if ( (*p) > 10u ) {
			if ( 42u <= (*p) && (*p) <= 43u )
				goto st383;
		} else if ( (*p) >= 9u )
			goto st383;
	} else if ( (*p) > 46u ) {
		if ( (*p) < 65u ) {
			if ( 48u <= (*p) && (*p) <= 58u )
				goto st384;
		} else if ( (*p) > 90u ) {
			if ( 97u <= (*p) && (*p) <= 122u )
				goto st384;
		} else
			goto st384;
	} else
		goto st384;
	goto st0;
st385:
	if ( ++p == pe )
		goto _test_eof385;
case 385:
	switch( (*p) ) {
		case 13u: goto st385;
		case 32u: goto st385;
		case 40u: goto tr396;
		case 58u: goto st388;
		case 95u: goto st388;
	}
	if ( (*p) < 65u ) {
		if ( 9u <= (*p) && (*p) <= 10u )
			goto st385;
	} else if ( (*p) > 90u ) {
		if ( 97u <= (*p) && (*p) <= 122u )
			goto st388;
	} else
		goto st388;
	goto st0;
tr396:
#line 91 "../src/xml.ragel"
	{ { pre_push(); { this->m_stack[ this->m_top++] = 386; goto st378;}} }
	goto st386;
st386:
	if ( ++p == pe )
		goto _test_eof386;
case 386:
#line 5121 "src/xml.cpp"
	switch( (*p) ) {
		case 13u: goto st387;
		case 32u: goto st387;
		case 41u: goto tr390;
		case 63u: goto st387;
		case 124u: goto st385;
	}
	if ( (*p) > 10u ) {
		if ( 42u <= (*p) && (*p) <= 43u )
			goto st387;
	} else if ( (*p) >= 9u )
		goto st387;
	goto st0;
st387:
	if ( ++p == pe )
		goto _test_eof387;
case 387:
	switch( (*p) ) {
		case 13u: goto st387;
		case 32u: goto st387;
		case 41u: goto tr390;
		case 124u: goto st385;
	}
	if ( 9u <= (*p) && (*p) <= 10u )
		goto st387;
	goto st0;
st388:
	if ( ++p == pe )
		goto _test_eof388;
case 388:
	switch( (*p) ) {
		case 13u: goto st387;
		case 32u: goto st387;
		case 41u: goto tr390;
		case 63u: goto st387;
		case 95u: goto st388;
		case 124u: goto st385;
	}
	if ( (*p) < 45u ) {
		if ( (*p) > 10u ) {
			if ( 42u <= (*p) && (*p) <= 43u )
				goto st387;
		} else if ( (*p) >= 9u )
			goto st387;
	} else if ( (*p) > 46u ) {
		if ( (*p) < 65u ) {
			if ( 48u <= (*p) && (*p) <= 58u )
				goto st388;
		} else if ( (*p) > 90u ) {
			if ( 97u <= (*p) && (*p) <= 122u )
				goto st388;
		} else
			goto st388;
	} else
		goto st388;
	goto st0;
st389:
	if ( ++p == pe )
		goto _test_eof389;
case 389:
	switch( (*p) ) {
		case 13u: goto st380;
		case 32u: goto st380;
		case 41u: goto tr390;
		case 44u: goto st381;
		case 63u: goto st380;
		case 95u: goto st389;
		case 124u: goto st385;
	}
	if ( (*p) < 45u ) {
		if ( (*p) > 10u ) {
			if ( 42u <= (*p) && (*p) <= 43u )
				goto st380;
		} else if ( (*p) >= 9u )
			goto st380;
	} else if ( (*p) > 46u ) {
		if ( (*p) < 65u ) {
			if ( 48u <= (*p) && (*p) <= 58u )
				goto st389;
		} else if ( (*p) > 90u ) {
			if ( 97u <= (*p) && (*p) <= 122u )
				goto st389;
		} else
			goto st389;
	} else
		goto st389;
	goto st0;
tr430:
#line 108 "../src/xml.ragel"
	{ { pre_push(); { this->m_stack[ this->m_top++] = 390; goto st390;}} }
	goto st390;
tr407:
#line 31 "../src/xml.ragel"
	{ pop(); }
	goto st390;
st390:
	if ( ++p == pe )
		goto _test_eof390;
case 390:
#line 5221 "src/xml.cpp"
	switch( (*p) ) {
		case 38u: goto tr400;
		case 60u: goto st397;
		case 93u: goto st442;
	}
	goto st390;
tr400:
#line 30 "../src/xml.ragel"
	{ m_output.push('['); ++m_rec; }
	goto st391;
st391:
	if ( ++p == pe )
		goto _test_eof391;
case 391:
#line 5236 "src/xml.cpp"
	switch( (*p) ) {
		case 35u: goto st392;
		case 58u: goto st396;
		case 95u: goto st396;
	}
	if ( (*p) > 90u ) {
		if ( 97u <= (*p) && (*p) <= 122u )
			goto st396;
	} else if ( (*p) >= 65u )
		goto st396;
	goto st0;
st392:
	if ( ++p == pe )
		goto _test_eof392;
case 392:
	if ( (*p) == 120u )
		goto st394;
	if ( 48u <= (*p) && (*p) <= 57u )
		goto st393;
	goto st0;
st393:
	if ( ++p == pe )
		goto _test_eof393;
case 393:
	if ( (*p) == 59u )
		goto tr407;
	if ( 48u <= (*p) && (*p) <= 57u )
		goto st393;
	goto st0;
st394:
	if ( ++p == pe )
		goto _test_eof394;
case 394:
	if ( (*p) < 65u ) {
		if ( 48u <= (*p) && (*p) <= 57u )
			goto st395;
	} else if ( (*p) > 70u ) {
		if ( 97u <= (*p) && (*p) <= 102u )
			goto st395;
	} else
		goto st395;
	goto st0;
st395:
	if ( ++p == pe )
		goto _test_eof395;
case 395:
	if ( (*p) == 59u )
		goto tr407;
	if ( (*p) < 65u ) {
		if ( 48u <= (*p) && (*p) <= 57u )
			goto st395;
	} else if ( (*p) > 70u ) {
		if ( 97u <= (*p) && (*p) <= 102u )
			goto st395;
	} else
		goto st395;
	goto st0;
st396:
	if ( ++p == pe )
		goto _test_eof396;
case 396:
	switch( (*p) ) {
		case 59u: goto st390;
		case 95u: goto st396;
	}
	if ( (*p) < 48u ) {
		if ( 45u <= (*p) && (*p) <= 46u )
			goto st396;
	} else if ( (*p) > 58u ) {
		if ( (*p) > 90u ) {
			if ( 97u <= (*p) && (*p) <= 122u )
				goto st396;
		} else if ( (*p) >= 65u )
			goto st396;
	} else
		goto st396;
	goto st0;
st397:
	if ( ++p == pe )
		goto _test_eof397;
case 397:
	switch( (*p) ) {
		case 33u: goto st398;
		case 47u: goto st412;
		case 58u: goto st415;
		case 63u: goto st435;
		case 95u: goto st415;
	}
	if ( (*p) > 90u ) {
		if ( 97u <= (*p) && (*p) <= 122u )
			goto st415;
	} else if ( (*p) >= 65u )
		goto st415;
	goto st0;
st398:
	if ( ++p == pe )
		goto _test_eof398;
case 398:
	switch( (*p) ) {
		case 45u: goto st399;
		case 91u: goto st403;
	}
	goto st0;
st399:
	if ( ++p == pe )
		goto _test_eof399;
case 399:
	if ( (*p) == 45u )
		goto st400;
	goto st0;
st400:
	if ( ++p == pe )
		goto _test_eof400;
case 400:
	if ( (*p) == 45u )
		goto st401;
	goto st400;
st401:
	if ( ++p == pe )
		goto _test_eof401;
case 401:
	if ( (*p) == 45u )
		goto st402;
	goto st400;
st402:
	if ( ++p == pe )
		goto _test_eof402;
case 402:
	if ( (*p) == 62u )
		goto st390;
	goto st0;
st403:
	if ( ++p == pe )
		goto _test_eof403;
case 403:
	if ( (*p) == 67u )
		goto st404;
	goto st0;
st404:
	if ( ++p == pe )
		goto _test_eof404;
case 404:
	if ( (*p) == 68u )
		goto st405;
	goto st0;
st405:
	if ( ++p == pe )
		goto _test_eof405;
case 405:
	if ( (*p) == 65u )
		goto st406;
	goto st0;
st406:
	if ( ++p == pe )
		goto _test_eof406;
case 406:
	if ( (*p) == 84u )
		goto st407;
	goto st0;
st407:
	if ( ++p == pe )
		goto _test_eof407;
case 407:
	if ( (*p) == 65u )
		goto st408;
	goto st0;
st408:
	if ( ++p == pe )
		goto _test_eof408;
case 408:
	if ( (*p) == 91u )
		goto st409;
	goto st0;
st409:
	if ( ++p == pe )
		goto _test_eof409;
case 409:
	if ( (*p) == 93u )
		goto st410;
	goto st409;
st410:
	if ( ++p == pe )
		goto _test_eof410;
case 410:
	if ( (*p) == 93u )
		goto st411;
	goto st409;
st411:
	if ( ++p == pe )
		goto _test_eof411;
case 411:
	switch( (*p) ) {
		case 62u: goto st390;
		case 93u: goto st411;
	}
	goto st409;
st412:
	if ( ++p == pe )
		goto _test_eof412;
case 412:
	switch( (*p) ) {
		case 58u: goto st413;
		case 95u: goto st413;
	}
	if ( (*p) > 90u ) {
		if ( 97u <= (*p) && (*p) <= 122u )
			goto st413;
	} else if ( (*p) >= 65u )
		goto st413;
	goto st0;
st413:
	if ( ++p == pe )
		goto _test_eof413;
case 413:
	switch( (*p) ) {
		case 13u: goto st414;
		case 32u: goto st414;
		case 62u: goto tr428;
		case 95u: goto st413;
	}
	if ( (*p) < 48u ) {
		if ( (*p) > 10u ) {
			if ( 45u <= (*p) && (*p) <= 46u )
				goto st413;
		} else if ( (*p) >= 9u )
			goto st414;
	} else if ( (*p) > 58u ) {
		if ( (*p) > 90u ) {
			if ( 97u <= (*p) && (*p) <= 122u )
				goto st413;
		} else if ( (*p) >= 65u )
			goto st413;
	} else
		goto st413;
	goto st0;
st414:
	if ( ++p == pe )
		goto _test_eof414;
case 414:
	switch( (*p) ) {
		case 13u: goto st414;
		case 32u: goto st414;
		case 62u: goto tr428;
	}
	if ( 9u <= (*p) && (*p) <= 10u )
		goto st414;
	goto st0;
tr428:
#line 29 "../src/xml.ragel"
	{ { this->m_cs =  this->m_stack[-- this->m_top];goto _again;} }
	goto st446;
st446:
	if ( ++p == pe )
		goto _test_eof446;
case 446:
#line 5492 "src/xml.cpp"
	goto st0;
st415:
	if ( ++p == pe )
		goto _test_eof415;
case 415:
	switch( (*p) ) {
		case 13u: goto st416;
		case 32u: goto st416;
		case 47u: goto st402;
		case 62u: goto tr430;
		case 95u: goto st415;
	}
	if ( (*p) < 45u ) {
		if ( 9u <= (*p) && (*p) <= 10u )
			goto st416;
	} else if ( (*p) > 58u ) {
		if ( (*p) > 90u ) {
			if ( 97u <= (*p) && (*p) <= 122u )
				goto st415;
		} else if ( (*p) >= 65u )
			goto st415;
	} else
		goto st415;
	goto st0;
st416:
	if ( ++p == pe )
		goto _test_eof416;
case 416:
	switch( (*p) ) {
		case 13u: goto st416;
		case 32u: goto st416;
		case 47u: goto st402;
		case 58u: goto st417;
		case 62u: goto tr430;
		case 95u: goto st417;
	}
	if ( (*p) < 65u ) {
		if ( 9u <= (*p) && (*p) <= 10u )
			goto st416;
	} else if ( (*p) > 90u ) {
		if ( 97u <= (*p) && (*p) <= 122u )
			goto st417;
	} else
		goto st417;
	goto st0;
st417:
	if ( ++p == pe )
		goto _test_eof417;
case 417:
	switch( (*p) ) {
		case 13u: goto st418;
		case 32u: goto st418;
		case 61u: goto st419;
		case 95u: goto st417;
	}
	if ( (*p) < 48u ) {
		if ( (*p) > 10u ) {
			if ( 45u <= (*p) && (*p) <= 46u )
				goto st417;
		} else if ( (*p) >= 9u )
			goto st418;
	} else if ( (*p) > 58u ) {
		if ( (*p) > 90u ) {
			if ( 97u <= (*p) && (*p) <= 122u )
				goto st417;
		} else if ( (*p) >= 65u )
			goto st417;
	} else
		goto st417;
	goto st0;
st418:
	if ( ++p == pe )
		goto _test_eof418;
case 418:
	switch( (*p) ) {
		case 13u: goto st418;
		case 32u: goto st418;
		case 61u: goto st419;
	}
	if ( 9u <= (*p) && (*p) <= 10u )
		goto st418;
	goto st0;
st419:
	if ( ++p == pe )
		goto _test_eof419;
case 419:
	switch( (*p) ) {
		case 13u: goto st419;
		case 32u: goto st419;
		case 34u: goto tr434;
		case 39u: goto tr435;
	}
	if ( 9u <= (*p) && (*p) <= 10u )
		goto st419;
	goto st0;
tr434:
#line 30 "../src/xml.ragel"
	{ m_output.push('['); ++m_rec; }
	goto st420;
tr443:
#line 31 "../src/xml.ragel"
	{ pop(); }
	goto st420;
st420:
	if ( ++p == pe )
		goto _test_eof420;
case 420:
#line 5600 "src/xml.cpp"
	switch( (*p) ) {
		case 34u: goto tr437;
		case 38u: goto tr438;
		case 60u: goto st0;
	}
	goto st420;
tr437:
#line 31 "../src/xml.ragel"
	{ pop(); }
	goto st421;
st421:
	if ( ++p == pe )
		goto _test_eof421;
case 421:
#line 5615 "src/xml.cpp"
	switch( (*p) ) {
		case 13u: goto st416;
		case 32u: goto st416;
		case 47u: goto st402;
		case 62u: goto tr430;
	}
	if ( 9u <= (*p) && (*p) <= 10u )
		goto st416;
	goto st0;
tr438:
#line 30 "../src/xml.ragel"
	{ m_output.push('['); ++m_rec; }
	goto st422;
st422:
	if ( ++p == pe )
		goto _test_eof422;
case 422:
#line 5633 "src/xml.cpp"
	switch( (*p) ) {
		case 35u: goto st423;
		case 58u: goto st427;
		case 95u: goto st427;
	}
	if ( (*p) > 90u ) {
		if ( 97u <= (*p) && (*p) <= 122u )
			goto st427;
	} else if ( (*p) >= 65u )
		goto st427;
	goto st0;
st423:
	if ( ++p == pe )
		goto _test_eof423;
case 423:
	if ( (*p) == 120u )
		goto st425;
	if ( 48u <= (*p) && (*p) <= 57u )
		goto st424;
	goto st0;
st424:
	if ( ++p == pe )
		goto _test_eof424;
case 424:
	if ( (*p) == 59u )
		goto tr443;
	if ( 48u <= (*p) && (*p) <= 57u )
		goto st424;
	goto st0;
st425:
	if ( ++p == pe )
		goto _test_eof425;
case 425:
	if ( (*p) < 65u ) {
		if ( 48u <= (*p) && (*p) <= 57u )
			goto st426;
	} else if ( (*p) > 70u ) {
		if ( 97u <= (*p) && (*p) <= 102u )
			goto st426;
	} else
		goto st426;
	goto st0;
st426:
	if ( ++p == pe )
		goto _test_eof426;
case 426:
	if ( (*p) == 59u )
		goto tr443;
	if ( (*p) < 65u ) {
		if ( 48u <= (*p) && (*p) <= 57u )
			goto st426;
	} else if ( (*p) > 70u ) {
		if ( 97u <= (*p) && (*p) <= 102u )
			goto st426;
	} else
		goto st426;
	goto st0;
st427:
	if ( ++p == pe )
		goto _test_eof427;
case 427:
	switch( (*p) ) {
		case 59u: goto st420;
		case 95u: goto st427;
	}
	if ( (*p) < 48u ) {
		if ( 45u <= (*p) && (*p) <= 46u )
			goto st427;
	} else if ( (*p) > 58u ) {
		if ( (*p) > 90u ) {
			if ( 97u <= (*p) && (*p) <= 122u )
				goto st427;
		} else if ( (*p) >= 65u )
			goto st427;
	} else
		goto st427;
	goto st0;
tr435:
#line 30 "../src/xml.ragel"
	{ m_output.push('['); ++m_rec; }
	goto st428;
tr451:
#line 31 "../src/xml.ragel"
	{ pop(); }
	goto st428;
st428:
	if ( ++p == pe )
		goto _test_eof428;
case 428:
#line 5723 "src/xml.cpp"
	switch( (*p) ) {
		case 38u: goto tr446;
		case 39u: goto tr437;
		case 60u: goto st0;
	}
	goto st428;
tr446:
#line 30 "../src/xml.ragel"
	{ m_output.push('['); ++m_rec; }
	goto st429;
st429:
	if ( ++p == pe )
		goto _test_eof429;
case 429:
#line 5738 "src/xml.cpp"
	switch( (*p) ) {
		case 35u: goto st430;
		case 58u: goto st434;
		case 95u: goto st434;
	}
	if ( (*p) > 90u ) {
		if ( 97u <= (*p) && (*p) <= 122u )
			goto st434;
	} else if ( (*p) >= 65u )
		goto st434;
	goto st0;
st430:
	if ( ++p == pe )
		goto _test_eof430;
case 430:
	if ( (*p) == 120u )
		goto st432;
	if ( 48u <= (*p) && (*p) <= 57u )
		goto st431;
	goto st0;
st431:
	if ( ++p == pe )
		goto _test_eof431;
case 431:
	if ( (*p) == 59u )
		goto tr451;
	if ( 48u <= (*p) && (*p) <= 57u )
		goto st431;
	goto st0;
st432:
	if ( ++p == pe )
		goto _test_eof432;
case 432:
	if ( (*p) < 65u ) {
		if ( 48u <= (*p) && (*p) <= 57u )
			goto st433;
	} else if ( (*p) > 70u ) {
		if ( 97u <= (*p) && (*p) <= 102u )
			goto st433;
	} else
		goto st433;
	goto st0;
st433:
	if ( ++p == pe )
		goto _test_eof433;
case 433:
	if ( (*p) == 59u )
		goto tr451;
	if ( (*p) < 65u ) {
		if ( 48u <= (*p) && (*p) <= 57u )
			goto st433;
	} else if ( (*p) > 70u ) {
		if ( 97u <= (*p) && (*p) <= 102u )
			goto st433;
	} else
		goto st433;
	goto st0;
st434:
	if ( ++p == pe )
		goto _test_eof434;
case 434:
	switch( (*p) ) {
		case 59u: goto st428;
		case 95u: goto st434;
	}
	if ( (*p) < 48u ) {
		if ( 45u <= (*p) && (*p) <= 46u )
			goto st434;
	} else if ( (*p) > 58u ) {
		if ( (*p) > 90u ) {
			if ( 97u <= (*p) && (*p) <= 122u )
				goto st434;
		} else if ( (*p) >= 65u )
			goto st434;
	} else
		goto st434;
	goto st0;
st435:
	if ( ++p == pe )
		goto _test_eof435;
case 435:
	switch( (*p) ) {
		case 58u: goto st436;
		case 88u: goto st439;
		case 95u: goto st436;
		case 120u: goto st439;
	}
	if ( (*p) > 90u ) {
		if ( 97u <= (*p) && (*p) <= 122u )
			goto st436;
	} else if ( (*p) >= 65u )
		goto st436;
	goto st0;
st436:
	if ( ++p == pe )
		goto _test_eof436;
case 436:
	switch( (*p) ) {
		case 13u: goto st437;
		case 32u: goto st437;
		case 63u: goto st402;
		case 95u: goto st436;
	}
	if ( (*p) < 48u ) {
		if ( (*p) > 10u ) {
			if ( 45u <= (*p) && (*p) <= 46u )
				goto st436;
		} else if ( (*p) >= 9u )
			goto st437;
	} else if ( (*p) > 58u ) {
		if ( (*p) > 90u ) {
			if ( 97u <= (*p) && (*p) <= 122u )
				goto st436;
		} else if ( (*p) >= 65u )
			goto st436;
	} else
		goto st436;
	goto st0;
st437:
	if ( ++p == pe )
		goto _test_eof437;
case 437:
	if ( (*p) == 63u )
		goto st438;
	goto st437;
st438:
	if ( ++p == pe )
		goto _test_eof438;
case 438:
	switch( (*p) ) {
		case 62u: goto st390;
		case 63u: goto st438;
	}
	goto st437;
st439:
	if ( ++p == pe )
		goto _test_eof439;
case 439:
	switch( (*p) ) {
		case 13u: goto st437;
		case 32u: goto st437;
		case 63u: goto st402;
		case 77u: goto st440;
		case 95u: goto st436;
		case 109u: goto st440;
	}
	if ( (*p) < 48u ) {
		if ( (*p) > 10u ) {
			if ( 45u <= (*p) && (*p) <= 46u )
				goto st436;
		} else if ( (*p) >= 9u )
			goto st437;
	} else if ( (*p) > 58u ) {
		if ( (*p) > 90u ) {
			if ( 97u <= (*p) && (*p) <= 122u )
				goto st436;
		} else if ( (*p) >= 65u )
			goto st436;
	} else
		goto st436;
	goto st0;
st440:
	if ( ++p == pe )
		goto _test_eof440;
case 440:
	switch( (*p) ) {
		case 13u: goto st437;
		case 32u: goto st437;
		case 63u: goto st402;
		case 76u: goto st441;
		case 95u: goto st436;
		case 108u: goto st441;
	}
	if ( (*p) < 48u ) {
		if ( (*p) > 10u ) {
			if ( 45u <= (*p) && (*p) <= 46u )
				goto st436;
		} else if ( (*p) >= 9u )
			goto st437;
	} else if ( (*p) > 58u ) {
		if ( (*p) > 90u ) {
			if ( 97u <= (*p) && (*p) <= 122u )
				goto st436;
		} else if ( (*p) >= 65u )
			goto st436;
	} else
		goto st436;
	goto st0;
st441:
	if ( ++p == pe )
		goto _test_eof441;
case 441:
	if ( (*p) == 95u )
		goto st436;
	if ( (*p) < 48u ) {
		if ( 45u <= (*p) && (*p) <= 46u )
			goto st436;
	} else if ( (*p) > 58u ) {
		if ( (*p) > 90u ) {
			if ( 97u <= (*p) && (*p) <= 122u )
				goto st436;
		} else if ( (*p) >= 65u )
			goto st436;
	} else
		goto st436;
	goto st0;
st442:
	if ( ++p == pe )
		goto _test_eof442;
case 442:
	switch( (*p) ) {
		case 38u: goto tr400;
		case 60u: goto st397;
		case 93u: goto st443;
	}
	goto st390;
st443:
	if ( ++p == pe )
		goto _test_eof443;
case 443:
	switch( (*p) ) {
		case 38u: goto tr400;
		case 60u: goto st397;
		case 62u: goto st0;
		case 93u: goto st443;
	}
	goto st390;
	}
	_test_eof1:  this->m_cs = 1; goto _test_eof; 
	_test_eof2:  this->m_cs = 2; goto _test_eof; 
	_test_eof3:  this->m_cs = 3; goto _test_eof; 
	_test_eof4:  this->m_cs = 4; goto _test_eof; 
	_test_eof5:  this->m_cs = 5; goto _test_eof; 
	_test_eof6:  this->m_cs = 6; goto _test_eof; 
	_test_eof7:  this->m_cs = 7; goto _test_eof; 
	_test_eof8:  this->m_cs = 8; goto _test_eof; 
	_test_eof9:  this->m_cs = 9; goto _test_eof; 
	_test_eof10:  this->m_cs = 10; goto _test_eof; 
	_test_eof11:  this->m_cs = 11; goto _test_eof; 
	_test_eof12:  this->m_cs = 12; goto _test_eof; 
	_test_eof13:  this->m_cs = 13; goto _test_eof; 
	_test_eof14:  this->m_cs = 14; goto _test_eof; 
	_test_eof15:  this->m_cs = 15; goto _test_eof; 
	_test_eof16:  this->m_cs = 16; goto _test_eof; 
	_test_eof17:  this->m_cs = 17; goto _test_eof; 
	_test_eof18:  this->m_cs = 18; goto _test_eof; 
	_test_eof19:  this->m_cs = 19; goto _test_eof; 
	_test_eof20:  this->m_cs = 20; goto _test_eof; 
	_test_eof21:  this->m_cs = 21; goto _test_eof; 
	_test_eof22:  this->m_cs = 22; goto _test_eof; 
	_test_eof23:  this->m_cs = 23; goto _test_eof; 
	_test_eof24:  this->m_cs = 24; goto _test_eof; 
	_test_eof25:  this->m_cs = 25; goto _test_eof; 
	_test_eof26:  this->m_cs = 26; goto _test_eof; 
	_test_eof27:  this->m_cs = 27; goto _test_eof; 
	_test_eof28:  this->m_cs = 28; goto _test_eof; 
	_test_eof444:  this->m_cs = 444; goto _test_eof; 
	_test_eof29:  this->m_cs = 29; goto _test_eof; 
	_test_eof30:  this->m_cs = 30; goto _test_eof; 
	_test_eof31:  this->m_cs = 31; goto _test_eof; 
	_test_eof32:  this->m_cs = 32; goto _test_eof; 
	_test_eof33:  this->m_cs = 33; goto _test_eof; 
	_test_eof34:  this->m_cs = 34; goto _test_eof; 
	_test_eof35:  this->m_cs = 35; goto _test_eof; 
	_test_eof36:  this->m_cs = 36; goto _test_eof; 
	_test_eof37:  this->m_cs = 37; goto _test_eof; 
	_test_eof38:  this->m_cs = 38; goto _test_eof; 
	_test_eof39:  this->m_cs = 39; goto _test_eof; 
	_test_eof40:  this->m_cs = 40; goto _test_eof; 
	_test_eof41:  this->m_cs = 41; goto _test_eof; 
	_test_eof42:  this->m_cs = 42; goto _test_eof; 
	_test_eof43:  this->m_cs = 43; goto _test_eof; 
	_test_eof44:  this->m_cs = 44; goto _test_eof; 
	_test_eof45:  this->m_cs = 45; goto _test_eof; 
	_test_eof46:  this->m_cs = 46; goto _test_eof; 
	_test_eof47:  this->m_cs = 47; goto _test_eof; 
	_test_eof48:  this->m_cs = 48; goto _test_eof; 
	_test_eof49:  this->m_cs = 49; goto _test_eof; 
	_test_eof50:  this->m_cs = 50; goto _test_eof; 
	_test_eof51:  this->m_cs = 51; goto _test_eof; 
	_test_eof52:  this->m_cs = 52; goto _test_eof; 
	_test_eof53:  this->m_cs = 53; goto _test_eof; 
	_test_eof54:  this->m_cs = 54; goto _test_eof; 
	_test_eof55:  this->m_cs = 55; goto _test_eof; 
	_test_eof56:  this->m_cs = 56; goto _test_eof; 
	_test_eof57:  this->m_cs = 57; goto _test_eof; 
	_test_eof58:  this->m_cs = 58; goto _test_eof; 
	_test_eof59:  this->m_cs = 59; goto _test_eof; 
	_test_eof60:  this->m_cs = 60; goto _test_eof; 
	_test_eof61:  this->m_cs = 61; goto _test_eof; 
	_test_eof62:  this->m_cs = 62; goto _test_eof; 
	_test_eof63:  this->m_cs = 63; goto _test_eof; 
	_test_eof64:  this->m_cs = 64; goto _test_eof; 
	_test_eof65:  this->m_cs = 65; goto _test_eof; 
	_test_eof66:  this->m_cs = 66; goto _test_eof; 
	_test_eof67:  this->m_cs = 67; goto _test_eof; 
	_test_eof68:  this->m_cs = 68; goto _test_eof; 
	_test_eof69:  this->m_cs = 69; goto _test_eof; 
	_test_eof70:  this->m_cs = 70; goto _test_eof; 
	_test_eof71:  this->m_cs = 71; goto _test_eof; 
	_test_eof72:  this->m_cs = 72; goto _test_eof; 
	_test_eof73:  this->m_cs = 73; goto _test_eof; 
	_test_eof74:  this->m_cs = 74; goto _test_eof; 
	_test_eof75:  this->m_cs = 75; goto _test_eof; 
	_test_eof76:  this->m_cs = 76; goto _test_eof; 
	_test_eof77:  this->m_cs = 77; goto _test_eof; 
	_test_eof78:  this->m_cs = 78; goto _test_eof; 
	_test_eof79:  this->m_cs = 79; goto _test_eof; 
	_test_eof80:  this->m_cs = 80; goto _test_eof; 
	_test_eof81:  this->m_cs = 81; goto _test_eof; 
	_test_eof82:  this->m_cs = 82; goto _test_eof; 
	_test_eof83:  this->m_cs = 83; goto _test_eof; 
	_test_eof84:  this->m_cs = 84; goto _test_eof; 
	_test_eof85:  this->m_cs = 85; goto _test_eof; 
	_test_eof86:  this->m_cs = 86; goto _test_eof; 
	_test_eof87:  this->m_cs = 87; goto _test_eof; 
	_test_eof88:  this->m_cs = 88; goto _test_eof; 
	_test_eof89:  this->m_cs = 89; goto _test_eof; 
	_test_eof90:  this->m_cs = 90; goto _test_eof; 
	_test_eof91:  this->m_cs = 91; goto _test_eof; 
	_test_eof92:  this->m_cs = 92; goto _test_eof; 
	_test_eof93:  this->m_cs = 93; goto _test_eof; 
	_test_eof94:  this->m_cs = 94; goto _test_eof; 
	_test_eof95:  this->m_cs = 95; goto _test_eof; 
	_test_eof96:  this->m_cs = 96; goto _test_eof; 
	_test_eof97:  this->m_cs = 97; goto _test_eof; 
	_test_eof98:  this->m_cs = 98; goto _test_eof; 
	_test_eof99:  this->m_cs = 99; goto _test_eof; 
	_test_eof100:  this->m_cs = 100; goto _test_eof; 
	_test_eof101:  this->m_cs = 101; goto _test_eof; 
	_test_eof102:  this->m_cs = 102; goto _test_eof; 
	_test_eof103:  this->m_cs = 103; goto _test_eof; 
	_test_eof104:  this->m_cs = 104; goto _test_eof; 
	_test_eof105:  this->m_cs = 105; goto _test_eof; 
	_test_eof106:  this->m_cs = 106; goto _test_eof; 
	_test_eof107:  this->m_cs = 107; goto _test_eof; 
	_test_eof108:  this->m_cs = 108; goto _test_eof; 
	_test_eof109:  this->m_cs = 109; goto _test_eof; 
	_test_eof110:  this->m_cs = 110; goto _test_eof; 
	_test_eof111:  this->m_cs = 111; goto _test_eof; 
	_test_eof112:  this->m_cs = 112; goto _test_eof; 
	_test_eof113:  this->m_cs = 113; goto _test_eof; 
	_test_eof114:  this->m_cs = 114; goto _test_eof; 
	_test_eof115:  this->m_cs = 115; goto _test_eof; 
	_test_eof116:  this->m_cs = 116; goto _test_eof; 
	_test_eof117:  this->m_cs = 117; goto _test_eof; 
	_test_eof118:  this->m_cs = 118; goto _test_eof; 
	_test_eof119:  this->m_cs = 119; goto _test_eof; 
	_test_eof120:  this->m_cs = 120; goto _test_eof; 
	_test_eof121:  this->m_cs = 121; goto _test_eof; 
	_test_eof122:  this->m_cs = 122; goto _test_eof; 
	_test_eof123:  this->m_cs = 123; goto _test_eof; 
	_test_eof124:  this->m_cs = 124; goto _test_eof; 
	_test_eof125:  this->m_cs = 125; goto _test_eof; 
	_test_eof126:  this->m_cs = 126; goto _test_eof; 
	_test_eof127:  this->m_cs = 127; goto _test_eof; 
	_test_eof128:  this->m_cs = 128; goto _test_eof; 
	_test_eof129:  this->m_cs = 129; goto _test_eof; 
	_test_eof130:  this->m_cs = 130; goto _test_eof; 
	_test_eof131:  this->m_cs = 131; goto _test_eof; 
	_test_eof132:  this->m_cs = 132; goto _test_eof; 
	_test_eof133:  this->m_cs = 133; goto _test_eof; 
	_test_eof134:  this->m_cs = 134; goto _test_eof; 
	_test_eof135:  this->m_cs = 135; goto _test_eof; 
	_test_eof136:  this->m_cs = 136; goto _test_eof; 
	_test_eof137:  this->m_cs = 137; goto _test_eof; 
	_test_eof138:  this->m_cs = 138; goto _test_eof; 
	_test_eof139:  this->m_cs = 139; goto _test_eof; 
	_test_eof140:  this->m_cs = 140; goto _test_eof; 
	_test_eof141:  this->m_cs = 141; goto _test_eof; 
	_test_eof142:  this->m_cs = 142; goto _test_eof; 
	_test_eof143:  this->m_cs = 143; goto _test_eof; 
	_test_eof144:  this->m_cs = 144; goto _test_eof; 
	_test_eof145:  this->m_cs = 145; goto _test_eof; 
	_test_eof146:  this->m_cs = 146; goto _test_eof; 
	_test_eof147:  this->m_cs = 147; goto _test_eof; 
	_test_eof148:  this->m_cs = 148; goto _test_eof; 
	_test_eof149:  this->m_cs = 149; goto _test_eof; 
	_test_eof150:  this->m_cs = 150; goto _test_eof; 
	_test_eof151:  this->m_cs = 151; goto _test_eof; 
	_test_eof152:  this->m_cs = 152; goto _test_eof; 
	_test_eof153:  this->m_cs = 153; goto _test_eof; 
	_test_eof154:  this->m_cs = 154; goto _test_eof; 
	_test_eof155:  this->m_cs = 155; goto _test_eof; 
	_test_eof156:  this->m_cs = 156; goto _test_eof; 
	_test_eof157:  this->m_cs = 157; goto _test_eof; 
	_test_eof158:  this->m_cs = 158; goto _test_eof; 
	_test_eof159:  this->m_cs = 159; goto _test_eof; 
	_test_eof160:  this->m_cs = 160; goto _test_eof; 
	_test_eof161:  this->m_cs = 161; goto _test_eof; 
	_test_eof162:  this->m_cs = 162; goto _test_eof; 
	_test_eof163:  this->m_cs = 163; goto _test_eof; 
	_test_eof164:  this->m_cs = 164; goto _test_eof; 
	_test_eof165:  this->m_cs = 165; goto _test_eof; 
	_test_eof166:  this->m_cs = 166; goto _test_eof; 
	_test_eof167:  this->m_cs = 167; goto _test_eof; 
	_test_eof168:  this->m_cs = 168; goto _test_eof; 
	_test_eof169:  this->m_cs = 169; goto _test_eof; 
	_test_eof170:  this->m_cs = 170; goto _test_eof; 
	_test_eof171:  this->m_cs = 171; goto _test_eof; 
	_test_eof172:  this->m_cs = 172; goto _test_eof; 
	_test_eof173:  this->m_cs = 173; goto _test_eof; 
	_test_eof174:  this->m_cs = 174; goto _test_eof; 
	_test_eof175:  this->m_cs = 175; goto _test_eof; 
	_test_eof176:  this->m_cs = 176; goto _test_eof; 
	_test_eof177:  this->m_cs = 177; goto _test_eof; 
	_test_eof178:  this->m_cs = 178; goto _test_eof; 
	_test_eof179:  this->m_cs = 179; goto _test_eof; 
	_test_eof180:  this->m_cs = 180; goto _test_eof; 
	_test_eof181:  this->m_cs = 181; goto _test_eof; 
	_test_eof182:  this->m_cs = 182; goto _test_eof; 
	_test_eof183:  this->m_cs = 183; goto _test_eof; 
	_test_eof184:  this->m_cs = 184; goto _test_eof; 
	_test_eof185:  this->m_cs = 185; goto _test_eof; 
	_test_eof186:  this->m_cs = 186; goto _test_eof; 
	_test_eof187:  this->m_cs = 187; goto _test_eof; 
	_test_eof188:  this->m_cs = 188; goto _test_eof; 
	_test_eof189:  this->m_cs = 189; goto _test_eof; 
	_test_eof190:  this->m_cs = 190; goto _test_eof; 
	_test_eof191:  this->m_cs = 191; goto _test_eof; 
	_test_eof192:  this->m_cs = 192; goto _test_eof; 
	_test_eof193:  this->m_cs = 193; goto _test_eof; 
	_test_eof194:  this->m_cs = 194; goto _test_eof; 
	_test_eof195:  this->m_cs = 195; goto _test_eof; 
	_test_eof196:  this->m_cs = 196; goto _test_eof; 
	_test_eof197:  this->m_cs = 197; goto _test_eof; 
	_test_eof198:  this->m_cs = 198; goto _test_eof; 
	_test_eof199:  this->m_cs = 199; goto _test_eof; 
	_test_eof200:  this->m_cs = 200; goto _test_eof; 
	_test_eof201:  this->m_cs = 201; goto _test_eof; 
	_test_eof202:  this->m_cs = 202; goto _test_eof; 
	_test_eof203:  this->m_cs = 203; goto _test_eof; 
	_test_eof204:  this->m_cs = 204; goto _test_eof; 
	_test_eof205:  this->m_cs = 205; goto _test_eof; 
	_test_eof206:  this->m_cs = 206; goto _test_eof; 
	_test_eof207:  this->m_cs = 207; goto _test_eof; 
	_test_eof208:  this->m_cs = 208; goto _test_eof; 
	_test_eof209:  this->m_cs = 209; goto _test_eof; 
	_test_eof210:  this->m_cs = 210; goto _test_eof; 
	_test_eof211:  this->m_cs = 211; goto _test_eof; 
	_test_eof212:  this->m_cs = 212; goto _test_eof; 
	_test_eof213:  this->m_cs = 213; goto _test_eof; 
	_test_eof214:  this->m_cs = 214; goto _test_eof; 
	_test_eof215:  this->m_cs = 215; goto _test_eof; 
	_test_eof216:  this->m_cs = 216; goto _test_eof; 
	_test_eof217:  this->m_cs = 217; goto _test_eof; 
	_test_eof218:  this->m_cs = 218; goto _test_eof; 
	_test_eof219:  this->m_cs = 219; goto _test_eof; 
	_test_eof220:  this->m_cs = 220; goto _test_eof; 
	_test_eof221:  this->m_cs = 221; goto _test_eof; 
	_test_eof222:  this->m_cs = 222; goto _test_eof; 
	_test_eof223:  this->m_cs = 223; goto _test_eof; 
	_test_eof224:  this->m_cs = 224; goto _test_eof; 
	_test_eof225:  this->m_cs = 225; goto _test_eof; 
	_test_eof226:  this->m_cs = 226; goto _test_eof; 
	_test_eof227:  this->m_cs = 227; goto _test_eof; 
	_test_eof228:  this->m_cs = 228; goto _test_eof; 
	_test_eof229:  this->m_cs = 229; goto _test_eof; 
	_test_eof230:  this->m_cs = 230; goto _test_eof; 
	_test_eof231:  this->m_cs = 231; goto _test_eof; 
	_test_eof232:  this->m_cs = 232; goto _test_eof; 
	_test_eof233:  this->m_cs = 233; goto _test_eof; 
	_test_eof234:  this->m_cs = 234; goto _test_eof; 
	_test_eof235:  this->m_cs = 235; goto _test_eof; 
	_test_eof236:  this->m_cs = 236; goto _test_eof; 
	_test_eof237:  this->m_cs = 237; goto _test_eof; 
	_test_eof238:  this->m_cs = 238; goto _test_eof; 
	_test_eof239:  this->m_cs = 239; goto _test_eof; 
	_test_eof240:  this->m_cs = 240; goto _test_eof; 
	_test_eof241:  this->m_cs = 241; goto _test_eof; 
	_test_eof242:  this->m_cs = 242; goto _test_eof; 
	_test_eof243:  this->m_cs = 243; goto _test_eof; 
	_test_eof244:  this->m_cs = 244; goto _test_eof; 
	_test_eof245:  this->m_cs = 245; goto _test_eof; 
	_test_eof246:  this->m_cs = 246; goto _test_eof; 
	_test_eof247:  this->m_cs = 247; goto _test_eof; 
	_test_eof248:  this->m_cs = 248; goto _test_eof; 
	_test_eof249:  this->m_cs = 249; goto _test_eof; 
	_test_eof250:  this->m_cs = 250; goto _test_eof; 
	_test_eof251:  this->m_cs = 251; goto _test_eof; 
	_test_eof252:  this->m_cs = 252; goto _test_eof; 
	_test_eof253:  this->m_cs = 253; goto _test_eof; 
	_test_eof254:  this->m_cs = 254; goto _test_eof; 
	_test_eof255:  this->m_cs = 255; goto _test_eof; 
	_test_eof256:  this->m_cs = 256; goto _test_eof; 
	_test_eof257:  this->m_cs = 257; goto _test_eof; 
	_test_eof258:  this->m_cs = 258; goto _test_eof; 
	_test_eof259:  this->m_cs = 259; goto _test_eof; 
	_test_eof260:  this->m_cs = 260; goto _test_eof; 
	_test_eof261:  this->m_cs = 261; goto _test_eof; 
	_test_eof262:  this->m_cs = 262; goto _test_eof; 
	_test_eof263:  this->m_cs = 263; goto _test_eof; 
	_test_eof264:  this->m_cs = 264; goto _test_eof; 
	_test_eof265:  this->m_cs = 265; goto _test_eof; 
	_test_eof266:  this->m_cs = 266; goto _test_eof; 
	_test_eof267:  this->m_cs = 267; goto _test_eof; 
	_test_eof268:  this->m_cs = 268; goto _test_eof; 
	_test_eof269:  this->m_cs = 269; goto _test_eof; 
	_test_eof270:  this->m_cs = 270; goto _test_eof; 
	_test_eof271:  this->m_cs = 271; goto _test_eof; 
	_test_eof272:  this->m_cs = 272; goto _test_eof; 
	_test_eof273:  this->m_cs = 273; goto _test_eof; 
	_test_eof274:  this->m_cs = 274; goto _test_eof; 
	_test_eof275:  this->m_cs = 275; goto _test_eof; 
	_test_eof276:  this->m_cs = 276; goto _test_eof; 
	_test_eof277:  this->m_cs = 277; goto _test_eof; 
	_test_eof278:  this->m_cs = 278; goto _test_eof; 
	_test_eof279:  this->m_cs = 279; goto _test_eof; 
	_test_eof280:  this->m_cs = 280; goto _test_eof; 
	_test_eof281:  this->m_cs = 281; goto _test_eof; 
	_test_eof282:  this->m_cs = 282; goto _test_eof; 
	_test_eof283:  this->m_cs = 283; goto _test_eof; 
	_test_eof284:  this->m_cs = 284; goto _test_eof; 
	_test_eof285:  this->m_cs = 285; goto _test_eof; 
	_test_eof286:  this->m_cs = 286; goto _test_eof; 
	_test_eof287:  this->m_cs = 287; goto _test_eof; 
	_test_eof288:  this->m_cs = 288; goto _test_eof; 
	_test_eof289:  this->m_cs = 289; goto _test_eof; 
	_test_eof290:  this->m_cs = 290; goto _test_eof; 
	_test_eof291:  this->m_cs = 291; goto _test_eof; 
	_test_eof292:  this->m_cs = 292; goto _test_eof; 
	_test_eof293:  this->m_cs = 293; goto _test_eof; 
	_test_eof294:  this->m_cs = 294; goto _test_eof; 
	_test_eof295:  this->m_cs = 295; goto _test_eof; 
	_test_eof296:  this->m_cs = 296; goto _test_eof; 
	_test_eof297:  this->m_cs = 297; goto _test_eof; 
	_test_eof298:  this->m_cs = 298; goto _test_eof; 
	_test_eof299:  this->m_cs = 299; goto _test_eof; 
	_test_eof300:  this->m_cs = 300; goto _test_eof; 
	_test_eof301:  this->m_cs = 301; goto _test_eof; 
	_test_eof302:  this->m_cs = 302; goto _test_eof; 
	_test_eof303:  this->m_cs = 303; goto _test_eof; 
	_test_eof304:  this->m_cs = 304; goto _test_eof; 
	_test_eof305:  this->m_cs = 305; goto _test_eof; 
	_test_eof306:  this->m_cs = 306; goto _test_eof; 
	_test_eof307:  this->m_cs = 307; goto _test_eof; 
	_test_eof308:  this->m_cs = 308; goto _test_eof; 
	_test_eof309:  this->m_cs = 309; goto _test_eof; 
	_test_eof310:  this->m_cs = 310; goto _test_eof; 
	_test_eof311:  this->m_cs = 311; goto _test_eof; 
	_test_eof312:  this->m_cs = 312; goto _test_eof; 
	_test_eof313:  this->m_cs = 313; goto _test_eof; 
	_test_eof314:  this->m_cs = 314; goto _test_eof; 
	_test_eof315:  this->m_cs = 315; goto _test_eof; 
	_test_eof316:  this->m_cs = 316; goto _test_eof; 
	_test_eof317:  this->m_cs = 317; goto _test_eof; 
	_test_eof318:  this->m_cs = 318; goto _test_eof; 
	_test_eof319:  this->m_cs = 319; goto _test_eof; 
	_test_eof320:  this->m_cs = 320; goto _test_eof; 
	_test_eof321:  this->m_cs = 321; goto _test_eof; 
	_test_eof322:  this->m_cs = 322; goto _test_eof; 
	_test_eof323:  this->m_cs = 323; goto _test_eof; 
	_test_eof324:  this->m_cs = 324; goto _test_eof; 
	_test_eof325:  this->m_cs = 325; goto _test_eof; 
	_test_eof326:  this->m_cs = 326; goto _test_eof; 
	_test_eof327:  this->m_cs = 327; goto _test_eof; 
	_test_eof328:  this->m_cs = 328; goto _test_eof; 
	_test_eof329:  this->m_cs = 329; goto _test_eof; 
	_test_eof330:  this->m_cs = 330; goto _test_eof; 
	_test_eof331:  this->m_cs = 331; goto _test_eof; 
	_test_eof332:  this->m_cs = 332; goto _test_eof; 
	_test_eof333:  this->m_cs = 333; goto _test_eof; 
	_test_eof334:  this->m_cs = 334; goto _test_eof; 
	_test_eof335:  this->m_cs = 335; goto _test_eof; 
	_test_eof336:  this->m_cs = 336; goto _test_eof; 
	_test_eof337:  this->m_cs = 337; goto _test_eof; 
	_test_eof338:  this->m_cs = 338; goto _test_eof; 
	_test_eof339:  this->m_cs = 339; goto _test_eof; 
	_test_eof340:  this->m_cs = 340; goto _test_eof; 
	_test_eof341:  this->m_cs = 341; goto _test_eof; 
	_test_eof342:  this->m_cs = 342; goto _test_eof; 
	_test_eof343:  this->m_cs = 343; goto _test_eof; 
	_test_eof344:  this->m_cs = 344; goto _test_eof; 
	_test_eof345:  this->m_cs = 345; goto _test_eof; 
	_test_eof346:  this->m_cs = 346; goto _test_eof; 
	_test_eof347:  this->m_cs = 347; goto _test_eof; 
	_test_eof348:  this->m_cs = 348; goto _test_eof; 
	_test_eof349:  this->m_cs = 349; goto _test_eof; 
	_test_eof350:  this->m_cs = 350; goto _test_eof; 
	_test_eof351:  this->m_cs = 351; goto _test_eof; 
	_test_eof352:  this->m_cs = 352; goto _test_eof; 
	_test_eof353:  this->m_cs = 353; goto _test_eof; 
	_test_eof354:  this->m_cs = 354; goto _test_eof; 
	_test_eof355:  this->m_cs = 355; goto _test_eof; 
	_test_eof356:  this->m_cs = 356; goto _test_eof; 
	_test_eof357:  this->m_cs = 357; goto _test_eof; 
	_test_eof358:  this->m_cs = 358; goto _test_eof; 
	_test_eof359:  this->m_cs = 359; goto _test_eof; 
	_test_eof360:  this->m_cs = 360; goto _test_eof; 
	_test_eof361:  this->m_cs = 361; goto _test_eof; 
	_test_eof362:  this->m_cs = 362; goto _test_eof; 
	_test_eof363:  this->m_cs = 363; goto _test_eof; 
	_test_eof364:  this->m_cs = 364; goto _test_eof; 
	_test_eof365:  this->m_cs = 365; goto _test_eof; 
	_test_eof366:  this->m_cs = 366; goto _test_eof; 
	_test_eof367:  this->m_cs = 367; goto _test_eof; 
	_test_eof368:  this->m_cs = 368; goto _test_eof; 
	_test_eof369:  this->m_cs = 369; goto _test_eof; 
	_test_eof370:  this->m_cs = 370; goto _test_eof; 
	_test_eof371:  this->m_cs = 371; goto _test_eof; 
	_test_eof372:  this->m_cs = 372; goto _test_eof; 
	_test_eof373:  this->m_cs = 373; goto _test_eof; 
	_test_eof374:  this->m_cs = 374; goto _test_eof; 
	_test_eof375:  this->m_cs = 375; goto _test_eof; 
	_test_eof376:  this->m_cs = 376; goto _test_eof; 
	_test_eof377:  this->m_cs = 377; goto _test_eof; 
	_test_eof378:  this->m_cs = 378; goto _test_eof; 
	_test_eof379:  this->m_cs = 379; goto _test_eof; 
	_test_eof380:  this->m_cs = 380; goto _test_eof; 
	_test_eof445:  this->m_cs = 445; goto _test_eof; 
	_test_eof381:  this->m_cs = 381; goto _test_eof; 
	_test_eof382:  this->m_cs = 382; goto _test_eof; 
	_test_eof383:  this->m_cs = 383; goto _test_eof; 
	_test_eof384:  this->m_cs = 384; goto _test_eof; 
	_test_eof385:  this->m_cs = 385; goto _test_eof; 
	_test_eof386:  this->m_cs = 386; goto _test_eof; 
	_test_eof387:  this->m_cs = 387; goto _test_eof; 
	_test_eof388:  this->m_cs = 388; goto _test_eof; 
	_test_eof389:  this->m_cs = 389; goto _test_eof; 
	_test_eof390:  this->m_cs = 390; goto _test_eof; 
	_test_eof391:  this->m_cs = 391; goto _test_eof; 
	_test_eof392:  this->m_cs = 392; goto _test_eof; 
	_test_eof393:  this->m_cs = 393; goto _test_eof; 
	_test_eof394:  this->m_cs = 394; goto _test_eof; 
	_test_eof395:  this->m_cs = 395; goto _test_eof; 
	_test_eof396:  this->m_cs = 396; goto _test_eof; 
	_test_eof397:  this->m_cs = 397; goto _test_eof; 
	_test_eof398:  this->m_cs = 398; goto _test_eof; 
	_test_eof399:  this->m_cs = 399; goto _test_eof; 
	_test_eof400:  this->m_cs = 400; goto _test_eof; 
	_test_eof401:  this->m_cs = 401; goto _test_eof; 
	_test_eof402:  this->m_cs = 402; goto _test_eof; 
	_test_eof403:  this->m_cs = 403; goto _test_eof; 
	_test_eof404:  this->m_cs = 404; goto _test_eof; 
	_test_eof405:  this->m_cs = 405; goto _test_eof; 
	_test_eof406:  this->m_cs = 406; goto _test_eof; 
	_test_eof407:  this->m_cs = 407; goto _test_eof; 
	_test_eof408:  this->m_cs = 408; goto _test_eof; 
	_test_eof409:  this->m_cs = 409; goto _test_eof; 
	_test_eof410:  this->m_cs = 410; goto _test_eof; 
	_test_eof411:  this->m_cs = 411; goto _test_eof; 
	_test_eof412:  this->m_cs = 412; goto _test_eof; 
	_test_eof413:  this->m_cs = 413; goto _test_eof; 
	_test_eof414:  this->m_cs = 414; goto _test_eof; 
	_test_eof446:  this->m_cs = 446; goto _test_eof; 
	_test_eof415:  this->m_cs = 415; goto _test_eof; 
	_test_eof416:  this->m_cs = 416; goto _test_eof; 
	_test_eof417:  this->m_cs = 417; goto _test_eof; 
	_test_eof418:  this->m_cs = 418; goto _test_eof; 
	_test_eof419:  this->m_cs = 419; goto _test_eof; 
	_test_eof420:  this->m_cs = 420; goto _test_eof; 
	_test_eof421:  this->m_cs = 421; goto _test_eof; 
	_test_eof422:  this->m_cs = 422; goto _test_eof; 
	_test_eof423:  this->m_cs = 423; goto _test_eof; 
	_test_eof424:  this->m_cs = 424; goto _test_eof; 
	_test_eof425:  this->m_cs = 425; goto _test_eof; 
	_test_eof426:  this->m_cs = 426; goto _test_eof; 
	_test_eof427:  this->m_cs = 427; goto _test_eof; 
	_test_eof428:  this->m_cs = 428; goto _test_eof; 
	_test_eof429:  this->m_cs = 429; goto _test_eof; 
	_test_eof430:  this->m_cs = 430; goto _test_eof; 
	_test_eof431:  this->m_cs = 431; goto _test_eof; 
	_test_eof432:  this->m_cs = 432; goto _test_eof; 
	_test_eof433:  this->m_cs = 433; goto _test_eof; 
	_test_eof434:  this->m_cs = 434; goto _test_eof; 
	_test_eof435:  this->m_cs = 435; goto _test_eof; 
	_test_eof436:  this->m_cs = 436; goto _test_eof; 
	_test_eof437:  this->m_cs = 437; goto _test_eof; 
	_test_eof438:  this->m_cs = 438; goto _test_eof; 
	_test_eof439:  this->m_cs = 439; goto _test_eof; 
	_test_eof440:  this->m_cs = 440; goto _test_eof; 
	_test_eof441:  this->m_cs = 441; goto _test_eof; 
	_test_eof442:  this->m_cs = 442; goto _test_eof; 
	_test_eof443:  this->m_cs = 443; goto _test_eof; 

	_test_eof: {}
	_out: {}
	}

#line 139 "../src/xml.ragel"
	}
	
	return (m_cs != 
#line 6422 "src/xml.cpp"
0
#line 141 "../src/xml.ragel"
);
}
