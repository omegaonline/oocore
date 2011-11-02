
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


#line 113 "../src/xml.ragel"


#include "../src/Tokenizer.h"


#line 32 "src/xml.cpp"
static const int xml_start = 1;
static const int xml_first_final = 458;
static const int xml_error = 0;

static const int xml_en_ch_or_seq1 = 389;
static const int xml_en_content_1 = 401;
static const int xml_en_main = 1;


#line 118 "../src/xml.ragel"

void Tokenizer::do_init()
{
	
#line 47 "src/xml.cpp"
	{
	 this->m_cs = xml_start;
	 this->m_top = 0;
	}

#line 122 "../src/xml.ragel"
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
		case 29: goto st29;
		case 30: goto st30;
		case 458: goto st458;
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
		case 459: goto st459;
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
		case 460: goto st460;
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
		case 444: goto st444;
		case 445: goto st445;
		case 446: goto st446;
		case 447: goto st447;
		case 448: goto st448;
		case 449: goto st449;
		case 450: goto st450;
		case 451: goto st451;
		case 452: goto st452;
		case 453: goto st453;
		case 454: goto st454;
		case 455: goto st455;
		case 456: goto st456;
		case 457: goto st457;
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
		case 60u: goto st328;
	}
	if ( 9u <= (*p) && (*p) <= 10u )
		goto st2;
	goto st0;
st0:
 this->m_cs = 0;
	goto _out;
tr359:
#line 48 "../src/xml.ragel"
	{token("pi",-1);}
	goto st2;
st2:
	if ( ++p == pe )
		goto _test_eof2;
case 2:
#line 564 "src/xml.cpp"
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
		case 58u: goto tr5;
		case 63u: goto st320;
		case 95u: goto tr5;
	}
	if ( (*p) > 90u ) {
		if ( 97u <= (*p) && (*p) <= 122u )
			goto tr5;
	} else if ( (*p) >= 65u )
		goto tr5;
	goto st0;
st4:
	if ( ++p == pe )
		goto _test_eof4;
case 4:
	switch( (*p) ) {
		case 45u: goto st5;
		case 68u: goto st10;
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
	switch( (*p) ) {
		case 45u: goto st9;
		case 62u: goto st2;
	}
	goto st0;
st9:
	if ( ++p == pe )
		goto _test_eof9;
case 9:
	if ( (*p) == 62u )
		goto st2;
	goto st0;
st10:
	if ( ++p == pe )
		goto _test_eof10;
case 10:
	if ( (*p) == 79u )
		goto st11;
	goto st0;
st11:
	if ( ++p == pe )
		goto _test_eof11;
case 11:
	if ( (*p) == 67u )
		goto st12;
	goto st0;
st12:
	if ( ++p == pe )
		goto _test_eof12;
case 12:
	if ( (*p) == 84u )
		goto st13;
	goto st0;
st13:
	if ( ++p == pe )
		goto _test_eof13;
case 13:
	if ( (*p) == 89u )
		goto st14;
	goto st0;
st14:
	if ( ++p == pe )
		goto _test_eof14;
case 14:
	if ( (*p) == 80u )
		goto st15;
	goto st0;
st15:
	if ( ++p == pe )
		goto _test_eof15;
case 15:
	if ( (*p) == 69u )
		goto st16;
	goto st0;
st16:
	if ( ++p == pe )
		goto _test_eof16;
case 16:
	switch( (*p) ) {
		case 13u: goto st17;
		case 32u: goto st17;
	}
	if ( 9u <= (*p) && (*p) <= 10u )
		goto st17;
	goto st0;
st17:
	if ( ++p == pe )
		goto _test_eof17;
case 17:
	switch( (*p) ) {
		case 13u: goto st17;
		case 32u: goto st17;
		case 58u: goto tr20;
		case 95u: goto tr20;
	}
	if ( (*p) < 65u ) {
		if ( 9u <= (*p) && (*p) <= 10u )
			goto st17;
	} else if ( (*p) > 90u ) {
		if ( 97u <= (*p) && (*p) <= 122u )
			goto tr20;
	} else
		goto tr20;
	goto st0;
tr20:
#line 30 "../src/xml.ragel"
	{ m_output.push(m_char); }
	goto st18;
st18:
	if ( ++p == pe )
		goto _test_eof18;
case 18:
#line 715 "src/xml.cpp"
	switch( (*p) ) {
		case 13u: goto st19;
		case 32u: goto st19;
		case 62u: goto st20;
		case 91u: goto st84;
		case 95u: goto tr20;
	}
	if ( (*p) < 48u ) {
		if ( (*p) > 10u ) {
			if ( 45u <= (*p) && (*p) <= 46u )
				goto tr20;
		} else if ( (*p) >= 9u )
			goto st19;
	} else if ( (*p) > 58u ) {
		if ( (*p) > 90u ) {
			if ( 97u <= (*p) && (*p) <= 122u )
				goto tr20;
		} else if ( (*p) >= 65u )
			goto tr20;
	} else
		goto tr20;
	goto st0;
st19:
	if ( ++p == pe )
		goto _test_eof19;
case 19:
	switch( (*p) ) {
		case 13u: goto st19;
		case 32u: goto st19;
		case 62u: goto st20;
		case 80u: goto st72;
		case 83u: goto st315;
		case 91u: goto st84;
	}
	if ( 9u <= (*p) && (*p) <= 10u )
		goto st19;
	goto st0;
tr90:
#line 48 "../src/xml.ragel"
	{token("pi",-1);}
	goto st20;
st20:
	if ( ++p == pe )
		goto _test_eof20;
case 20:
#line 761 "src/xml.cpp"
	switch( (*p) ) {
		case 13u: goto st20;
		case 32u: goto st20;
		case 60u: goto st21;
	}
	if ( 9u <= (*p) && (*p) <= 10u )
		goto st20;
	goto st0;
st21:
	if ( ++p == pe )
		goto _test_eof21;
case 21:
	switch( (*p) ) {
		case 33u: goto st22;
		case 58u: goto tr5;
		case 63u: goto st64;
		case 95u: goto tr5;
	}
	if ( (*p) > 90u ) {
		if ( 97u <= (*p) && (*p) <= 122u )
			goto tr5;
	} else if ( (*p) >= 65u )
		goto tr5;
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
	goto st0;
st24:
	if ( ++p == pe )
		goto _test_eof24;
case 24:
	if ( (*p) == 45u )
		goto st25;
	goto st24;
st25:
	if ( ++p == pe )
		goto _test_eof25;
case 25:
	if ( (*p) == 45u )
		goto st26;
	goto st24;
st26:
	if ( ++p == pe )
		goto _test_eof26;
case 26:
	switch( (*p) ) {
		case 45u: goto st27;
		case 62u: goto st20;
	}
	goto st0;
st27:
	if ( ++p == pe )
		goto _test_eof27;
case 27:
	if ( (*p) == 62u )
		goto st20;
	goto st0;
tr5:
#line 30 "../src/xml.ragel"
	{ m_output.push(m_char); }
	goto st28;
st28:
	if ( ++p == pe )
		goto _test_eof28;
case 28:
#line 838 "src/xml.cpp"
	switch( (*p) ) {
		case 13u: goto tr34;
		case 32u: goto tr34;
		case 47u: goto tr35;
		case 62u: goto tr36;
		case 95u: goto tr5;
	}
	if ( (*p) < 45u ) {
		if ( 9u <= (*p) && (*p) <= 10u )
			goto tr34;
	} else if ( (*p) > 58u ) {
		if ( (*p) > 90u ) {
			if ( 97u <= (*p) && (*p) <= 122u )
				goto tr5;
		} else if ( (*p) >= 65u )
			goto tr5;
	} else
		goto tr5;
	goto st0;
tr34:
#line 101 "../src/xml.ragel"
	{ token("element_name"); }
	goto st29;
st29:
	if ( ++p == pe )
		goto _test_eof29;
case 29:
#line 866 "src/xml.cpp"
	switch( (*p) ) {
		case 13u: goto st29;
		case 32u: goto st29;
		case 47u: goto st30;
		case 58u: goto tr39;
		case 62u: goto tr40;
		case 95u: goto tr39;
	}
	if ( (*p) < 65u ) {
		if ( 9u <= (*p) && (*p) <= 10u )
			goto st29;
	} else if ( (*p) > 90u ) {
		if ( 97u <= (*p) && (*p) <= 122u )
			goto tr39;
	} else
		goto tr39;
	goto st0;
tr35:
#line 101 "../src/xml.ragel"
	{ token("element_name"); }
	goto st30;
st30:
	if ( ++p == pe )
		goto _test_eof30;
case 30:
#line 892 "src/xml.cpp"
	if ( (*p) == 62u )
		goto tr41;
	goto st0;
tr36:
#line 101 "../src/xml.ragel"
	{ token("element_name"); }
#line 100 "../src/xml.ragel"
	{ { pre_push(); { this->m_stack[ this->m_top++] = 458; goto st401;}} }
	goto st458;
tr40:
#line 100 "../src/xml.ragel"
	{ { pre_push(); { this->m_stack[ this->m_top++] = 458; goto st401;}} }
	goto st458;
tr41:
#line 102 "../src/xml.ragel"
	{token("empty element");}
	goto st458;
tr55:
#line 48 "../src/xml.ragel"
	{token("pi",-1);}
	goto st458;
st458:
	if ( ++p == pe )
		goto _test_eof458;
case 458:
#line 918 "src/xml.cpp"
	switch( (*p) ) {
		case 13u: goto st458;
		case 32u: goto st458;
		case 60u: goto st31;
	}
	if ( 9u <= (*p) && (*p) <= 10u )
		goto st458;
	goto st0;
st31:
	if ( ++p == pe )
		goto _test_eof31;
case 31:
	switch( (*p) ) {
		case 33u: goto st32;
		case 63u: goto st38;
	}
	goto st0;
st32:
	if ( ++p == pe )
		goto _test_eof32;
case 32:
	if ( (*p) == 45u )
		goto st33;
	goto st0;
st33:
	if ( ++p == pe )
		goto _test_eof33;
case 33:
	if ( (*p) == 45u )
		goto st34;
	goto st0;
st34:
	if ( ++p == pe )
		goto _test_eof34;
case 34:
	if ( (*p) == 45u )
		goto st35;
	goto st34;
st35:
	if ( ++p == pe )
		goto _test_eof35;
case 35:
	if ( (*p) == 45u )
		goto st36;
	goto st34;
st36:
	if ( ++p == pe )
		goto _test_eof36;
case 36:
	switch( (*p) ) {
		case 45u: goto st37;
		case 62u: goto st458;
	}
	goto st0;
st37:
	if ( ++p == pe )
		goto _test_eof37;
case 37:
	if ( (*p) == 62u )
		goto st458;
	goto st0;
st38:
	if ( ++p == pe )
		goto _test_eof38;
case 38:
	switch( (*p) ) {
		case 58u: goto tr50;
		case 88u: goto tr51;
		case 95u: goto tr50;
		case 120u: goto tr51;
	}
	if ( (*p) > 90u ) {
		if ( 97u <= (*p) && (*p) <= 122u )
			goto tr50;
	} else if ( (*p) >= 65u )
		goto tr50;
	goto st0;
tr50:
#line 30 "../src/xml.ragel"
	{ m_output.push(m_char); }
	goto st39;
st39:
	if ( ++p == pe )
		goto _test_eof39;
case 39:
#line 1004 "src/xml.cpp"
	switch( (*p) ) {
		case 13u: goto tr52;
		case 32u: goto tr52;
		case 63u: goto st42;
		case 95u: goto tr50;
	}
	if ( (*p) < 48u ) {
		if ( (*p) > 10u ) {
			if ( 45u <= (*p) && (*p) <= 46u )
				goto tr50;
		} else if ( (*p) >= 9u )
			goto tr52;
	} else if ( (*p) > 58u ) {
		if ( (*p) > 90u ) {
			if ( 97u <= (*p) && (*p) <= 122u )
				goto tr50;
		} else if ( (*p) >= 65u )
			goto tr50;
	} else
		goto tr50;
	goto st0;
tr52:
#line 30 "../src/xml.ragel"
	{ m_output.push(m_char); }
	goto st40;
st40:
	if ( ++p == pe )
		goto _test_eof40;
case 40:
#line 1034 "src/xml.cpp"
	if ( (*p) == 63u )
		goto tr54;
	goto tr52;
tr54:
#line 30 "../src/xml.ragel"
	{ m_output.push(m_char); }
	goto st41;
st41:
	if ( ++p == pe )
		goto _test_eof41;
case 41:
#line 1046 "src/xml.cpp"
	switch( (*p) ) {
		case 62u: goto tr55;
		case 63u: goto tr54;
	}
	goto tr52;
st42:
	if ( ++p == pe )
		goto _test_eof42;
case 42:
	if ( (*p) == 62u )
		goto tr55;
	goto st0;
tr51:
#line 30 "../src/xml.ragel"
	{ m_output.push(m_char); }
	goto st43;
st43:
	if ( ++p == pe )
		goto _test_eof43;
case 43:
#line 1067 "src/xml.cpp"
	switch( (*p) ) {
		case 13u: goto tr52;
		case 32u: goto tr52;
		case 63u: goto st42;
		case 77u: goto tr56;
		case 95u: goto tr50;
		case 109u: goto tr56;
	}
	if ( (*p) < 48u ) {
		if ( (*p) > 10u ) {
			if ( 45u <= (*p) && (*p) <= 46u )
				goto tr50;
		} else if ( (*p) >= 9u )
			goto tr52;
	} else if ( (*p) > 58u ) {
		if ( (*p) > 90u ) {
			if ( 97u <= (*p) && (*p) <= 122u )
				goto tr50;
		} else if ( (*p) >= 65u )
			goto tr50;
	} else
		goto tr50;
	goto st0;
tr56:
#line 30 "../src/xml.ragel"
	{ m_output.push(m_char); }
	goto st44;
st44:
	if ( ++p == pe )
		goto _test_eof44;
case 44:
#line 1099 "src/xml.cpp"
	switch( (*p) ) {
		case 13u: goto tr52;
		case 32u: goto tr52;
		case 63u: goto st42;
		case 76u: goto tr57;
		case 95u: goto tr50;
		case 108u: goto tr57;
	}
	if ( (*p) < 48u ) {
		if ( (*p) > 10u ) {
			if ( 45u <= (*p) && (*p) <= 46u )
				goto tr50;
		} else if ( (*p) >= 9u )
			goto tr52;
	} else if ( (*p) > 58u ) {
		if ( (*p) > 90u ) {
			if ( 97u <= (*p) && (*p) <= 122u )
				goto tr50;
		} else if ( (*p) >= 65u )
			goto tr50;
	} else
		goto tr50;
	goto st0;
tr57:
#line 30 "../src/xml.ragel"
	{ m_output.push(m_char); }
	goto st45;
st45:
	if ( ++p == pe )
		goto _test_eof45;
case 45:
#line 1131 "src/xml.cpp"
	if ( (*p) == 95u )
		goto tr50;
	if ( (*p) < 48u ) {
		if ( 45u <= (*p) && (*p) <= 46u )
			goto tr50;
	} else if ( (*p) > 58u ) {
		if ( (*p) > 90u ) {
			if ( 97u <= (*p) && (*p) <= 122u )
				goto tr50;
		} else if ( (*p) >= 65u )
			goto tr50;
	} else
		goto tr50;
	goto st0;
tr39:
#line 30 "../src/xml.ragel"
	{ m_output.push(m_char); }
	goto st46;
st46:
	if ( ++p == pe )
		goto _test_eof46;
case 46:
#line 1154 "src/xml.cpp"
	switch( (*p) ) {
		case 13u: goto tr58;
		case 32u: goto tr58;
		case 61u: goto tr59;
		case 95u: goto tr39;
	}
	if ( (*p) < 48u ) {
		if ( (*p) > 10u ) {
			if ( 45u <= (*p) && (*p) <= 46u )
				goto tr39;
		} else if ( (*p) >= 9u )
			goto tr58;
	} else if ( (*p) > 58u ) {
		if ( (*p) > 90u ) {
			if ( 97u <= (*p) && (*p) <= 122u )
				goto tr39;
		} else if ( (*p) >= 65u )
			goto tr39;
	} else
		goto tr39;
	goto st0;
tr58:
#line 80 "../src/xml.ragel"
	{token("attr_name");}
	goto st47;
st47:
	if ( ++p == pe )
		goto _test_eof47;
case 47:
#line 1184 "src/xml.cpp"
	switch( (*p) ) {
		case 13u: goto st47;
		case 32u: goto st47;
		case 61u: goto st48;
	}
	if ( 9u <= (*p) && (*p) <= 10u )
		goto st47;
	goto st0;
tr59:
#line 80 "../src/xml.ragel"
	{token("attr_name");}
	goto st48;
st48:
	if ( ++p == pe )
		goto _test_eof48;
case 48:
#line 1201 "src/xml.cpp"
	switch( (*p) ) {
		case 13u: goto st48;
		case 32u: goto st48;
		case 34u: goto st49;
		case 39u: goto st57;
	}
	if ( 9u <= (*p) && (*p) <= 10u )
		goto st48;
	goto st0;
tr64:
#line 30 "../src/xml.ragel"
	{ m_output.push(m_char); }
	goto st49;
tr71:
#line 63 "../src/xml.ragel"
	{subst_char();}
	goto st49;
tr73:
#line 63 "../src/xml.ragel"
	{subst_hex();}
	goto st49;
tr74:
#line 62 "../src/xml.ragel"
	{subst_entity();}
	goto st49;
st49:
	if ( ++p == pe )
		goto _test_eof49;
case 49:
#line 1231 "src/xml.cpp"
	switch( (*p) ) {
		case 34u: goto tr65;
		case 38u: goto st51;
		case 60u: goto st0;
	}
	goto tr64;
tr65:
#line 65 "../src/xml.ragel"
	{token("attr_value");}
	goto st50;
st50:
	if ( ++p == pe )
		goto _test_eof50;
case 50:
#line 1246 "src/xml.cpp"
	switch( (*p) ) {
		case 13u: goto st29;
		case 32u: goto st29;
		case 47u: goto st30;
		case 62u: goto tr40;
	}
	if ( 9u <= (*p) && (*p) <= 10u )
		goto st29;
	goto st0;
st51:
	if ( ++p == pe )
		goto _test_eof51;
case 51:
	switch( (*p) ) {
		case 35u: goto st52;
		case 58u: goto tr68;
		case 95u: goto tr68;
	}
	if ( (*p) > 90u ) {
		if ( 97u <= (*p) && (*p) <= 122u )
			goto tr68;
	} else if ( (*p) >= 65u )
		goto tr68;
	goto st0;
st52:
	if ( ++p == pe )
		goto _test_eof52;
case 52:
	if ( (*p) == 120u )
		goto st54;
	if ( 48u <= (*p) && (*p) <= 57u )
		goto tr69;
	goto st0;
tr69:
#line 31 "../src/xml.ragel"
	{ m_entity.push(m_char); }
	goto st53;
st53:
	if ( ++p == pe )
		goto _test_eof53;
case 53:
#line 1288 "src/xml.cpp"
	if ( (*p) == 59u )
		goto tr71;
	if ( 48u <= (*p) && (*p) <= 57u )
		goto tr69;
	goto st0;
st54:
	if ( ++p == pe )
		goto _test_eof54;
case 54:
	if ( (*p) < 65u ) {
		if ( 48u <= (*p) && (*p) <= 57u )
			goto tr72;
	} else if ( (*p) > 70u ) {
		if ( 97u <= (*p) && (*p) <= 102u )
			goto tr72;
	} else
		goto tr72;
	goto st0;
tr72:
#line 31 "../src/xml.ragel"
	{ m_entity.push(m_char); }
	goto st55;
st55:
	if ( ++p == pe )
		goto _test_eof55;
case 55:
#line 1315 "src/xml.cpp"
	if ( (*p) == 59u )
		goto tr73;
	if ( (*p) < 65u ) {
		if ( 48u <= (*p) && (*p) <= 57u )
			goto tr72;
	} else if ( (*p) > 70u ) {
		if ( 97u <= (*p) && (*p) <= 102u )
			goto tr72;
	} else
		goto tr72;
	goto st0;
tr68:
#line 31 "../src/xml.ragel"
	{ m_entity.push(m_char); }
	goto st56;
st56:
	if ( ++p == pe )
		goto _test_eof56;
case 56:
#line 1335 "src/xml.cpp"
	switch( (*p) ) {
		case 59u: goto tr74;
		case 95u: goto tr68;
	}
	if ( (*p) < 48u ) {
		if ( 45u <= (*p) && (*p) <= 46u )
			goto tr68;
	} else if ( (*p) > 58u ) {
		if ( (*p) > 90u ) {
			if ( 97u <= (*p) && (*p) <= 122u )
				goto tr68;
		} else if ( (*p) >= 65u )
			goto tr68;
	} else
		goto tr68;
	goto st0;
tr75:
#line 30 "../src/xml.ragel"
	{ m_output.push(m_char); }
	goto st57;
tr81:
#line 63 "../src/xml.ragel"
	{subst_char();}
	goto st57;
tr83:
#line 63 "../src/xml.ragel"
	{subst_hex();}
	goto st57;
tr84:
#line 62 "../src/xml.ragel"
	{subst_entity();}
	goto st57;
st57:
	if ( ++p == pe )
		goto _test_eof57;
case 57:
#line 1372 "src/xml.cpp"
	switch( (*p) ) {
		case 38u: goto st58;
		case 39u: goto tr65;
		case 60u: goto st0;
	}
	goto tr75;
st58:
	if ( ++p == pe )
		goto _test_eof58;
case 58:
	switch( (*p) ) {
		case 35u: goto st59;
		case 58u: goto tr78;
		case 95u: goto tr78;
	}
	if ( (*p) > 90u ) {
		if ( 97u <= (*p) && (*p) <= 122u )
			goto tr78;
	} else if ( (*p) >= 65u )
		goto tr78;
	goto st0;
st59:
	if ( ++p == pe )
		goto _test_eof59;
case 59:
	if ( (*p) == 120u )
		goto st61;
	if ( 48u <= (*p) && (*p) <= 57u )
		goto tr79;
	goto st0;
tr79:
#line 31 "../src/xml.ragel"
	{ m_entity.push(m_char); }
	goto st60;
st60:
	if ( ++p == pe )
		goto _test_eof60;
case 60:
#line 1411 "src/xml.cpp"
	if ( (*p) == 59u )
		goto tr81;
	if ( 48u <= (*p) && (*p) <= 57u )
		goto tr79;
	goto st0;
st61:
	if ( ++p == pe )
		goto _test_eof61;
case 61:
	if ( (*p) < 65u ) {
		if ( 48u <= (*p) && (*p) <= 57u )
			goto tr82;
	} else if ( (*p) > 70u ) {
		if ( 97u <= (*p) && (*p) <= 102u )
			goto tr82;
	} else
		goto tr82;
	goto st0;
tr82:
#line 31 "../src/xml.ragel"
	{ m_entity.push(m_char); }
	goto st62;
st62:
	if ( ++p == pe )
		goto _test_eof62;
case 62:
#line 1438 "src/xml.cpp"
	if ( (*p) == 59u )
		goto tr83;
	if ( (*p) < 65u ) {
		if ( 48u <= (*p) && (*p) <= 57u )
			goto tr82;
	} else if ( (*p) > 70u ) {
		if ( 97u <= (*p) && (*p) <= 102u )
			goto tr82;
	} else
		goto tr82;
	goto st0;
tr78:
#line 31 "../src/xml.ragel"
	{ m_entity.push(m_char); }
	goto st63;
st63:
	if ( ++p == pe )
		goto _test_eof63;
case 63:
#line 1458 "src/xml.cpp"
	switch( (*p) ) {
		case 59u: goto tr84;
		case 95u: goto tr78;
	}
	if ( (*p) < 48u ) {
		if ( 45u <= (*p) && (*p) <= 46u )
			goto tr78;
	} else if ( (*p) > 58u ) {
		if ( (*p) > 90u ) {
			if ( 97u <= (*p) && (*p) <= 122u )
				goto tr78;
		} else if ( (*p) >= 65u )
			goto tr78;
	} else
		goto tr78;
	goto st0;
st64:
	if ( ++p == pe )
		goto _test_eof64;
case 64:
	switch( (*p) ) {
		case 58u: goto tr85;
		case 88u: goto tr86;
		case 95u: goto tr85;
		case 120u: goto tr86;
	}
	if ( (*p) > 90u ) {
		if ( 97u <= (*p) && (*p) <= 122u )
			goto tr85;
	} else if ( (*p) >= 65u )
		goto tr85;
	goto st0;
tr85:
#line 30 "../src/xml.ragel"
	{ m_output.push(m_char); }
	goto st65;
st65:
	if ( ++p == pe )
		goto _test_eof65;
case 65:
#line 1499 "src/xml.cpp"
	switch( (*p) ) {
		case 13u: goto tr87;
		case 32u: goto tr87;
		case 63u: goto st68;
		case 95u: goto tr85;
	}
	if ( (*p) < 48u ) {
		if ( (*p) > 10u ) {
			if ( 45u <= (*p) && (*p) <= 46u )
				goto tr85;
		} else if ( (*p) >= 9u )
			goto tr87;
	} else if ( (*p) > 58u ) {
		if ( (*p) > 90u ) {
			if ( 97u <= (*p) && (*p) <= 122u )
				goto tr85;
		} else if ( (*p) >= 65u )
			goto tr85;
	} else
		goto tr85;
	goto st0;
tr87:
#line 30 "../src/xml.ragel"
	{ m_output.push(m_char); }
	goto st66;
st66:
	if ( ++p == pe )
		goto _test_eof66;
case 66:
#line 1529 "src/xml.cpp"
	if ( (*p) == 63u )
		goto tr89;
	goto tr87;
tr89:
#line 30 "../src/xml.ragel"
	{ m_output.push(m_char); }
	goto st67;
st67:
	if ( ++p == pe )
		goto _test_eof67;
case 67:
#line 1541 "src/xml.cpp"
	switch( (*p) ) {
		case 62u: goto tr90;
		case 63u: goto tr89;
	}
	goto tr87;
st68:
	if ( ++p == pe )
		goto _test_eof68;
case 68:
	if ( (*p) == 62u )
		goto tr90;
	goto st0;
tr86:
#line 30 "../src/xml.ragel"
	{ m_output.push(m_char); }
	goto st69;
st69:
	if ( ++p == pe )
		goto _test_eof69;
case 69:
#line 1562 "src/xml.cpp"
	switch( (*p) ) {
		case 13u: goto tr87;
		case 32u: goto tr87;
		case 63u: goto st68;
		case 77u: goto tr91;
		case 95u: goto tr85;
		case 109u: goto tr91;
	}
	if ( (*p) < 48u ) {
		if ( (*p) > 10u ) {
			if ( 45u <= (*p) && (*p) <= 46u )
				goto tr85;
		} else if ( (*p) >= 9u )
			goto tr87;
	} else if ( (*p) > 58u ) {
		if ( (*p) > 90u ) {
			if ( 97u <= (*p) && (*p) <= 122u )
				goto tr85;
		} else if ( (*p) >= 65u )
			goto tr85;
	} else
		goto tr85;
	goto st0;
tr91:
#line 30 "../src/xml.ragel"
	{ m_output.push(m_char); }
	goto st70;
st70:
	if ( ++p == pe )
		goto _test_eof70;
case 70:
#line 1594 "src/xml.cpp"
	switch( (*p) ) {
		case 13u: goto tr87;
		case 32u: goto tr87;
		case 63u: goto st68;
		case 76u: goto tr92;
		case 95u: goto tr85;
		case 108u: goto tr92;
	}
	if ( (*p) < 48u ) {
		if ( (*p) > 10u ) {
			if ( 45u <= (*p) && (*p) <= 46u )
				goto tr85;
		} else if ( (*p) >= 9u )
			goto tr87;
	} else if ( (*p) > 58u ) {
		if ( (*p) > 90u ) {
			if ( 97u <= (*p) && (*p) <= 122u )
				goto tr85;
		} else if ( (*p) >= 65u )
			goto tr85;
	} else
		goto tr85;
	goto st0;
tr92:
#line 30 "../src/xml.ragel"
	{ m_output.push(m_char); }
	goto st71;
st71:
	if ( ++p == pe )
		goto _test_eof71;
case 71:
#line 1626 "src/xml.cpp"
	if ( (*p) == 95u )
		goto tr85;
	if ( (*p) < 48u ) {
		if ( 45u <= (*p) && (*p) <= 46u )
			goto tr85;
	} else if ( (*p) > 58u ) {
		if ( (*p) > 90u ) {
			if ( 97u <= (*p) && (*p) <= 122u )
				goto tr85;
		} else if ( (*p) >= 65u )
			goto tr85;
	} else
		goto tr85;
	goto st0;
st72:
	if ( ++p == pe )
		goto _test_eof72;
case 72:
	if ( (*p) == 85u )
		goto st73;
	goto st0;
st73:
	if ( ++p == pe )
		goto _test_eof73;
case 73:
	if ( (*p) == 66u )
		goto st74;
	goto st0;
st74:
	if ( ++p == pe )
		goto _test_eof74;
case 74:
	if ( (*p) == 76u )
		goto st75;
	goto st0;
st75:
	if ( ++p == pe )
		goto _test_eof75;
case 75:
	if ( (*p) == 73u )
		goto st76;
	goto st0;
st76:
	if ( ++p == pe )
		goto _test_eof76;
case 76:
	if ( (*p) == 67u )
		goto st77;
	goto st0;
st77:
	if ( ++p == pe )
		goto _test_eof77;
case 77:
	switch( (*p) ) {
		case 13u: goto st78;
		case 32u: goto st78;
	}
	if ( 9u <= (*p) && (*p) <= 10u )
		goto st78;
	goto st0;
st78:
	if ( ++p == pe )
		goto _test_eof78;
case 78:
	switch( (*p) ) {
		case 13u: goto st78;
		case 32u: goto st78;
		case 34u: goto st79;
		case 39u: goto st314;
	}
	if ( 9u <= (*p) && (*p) <= 10u )
		goto st78;
	goto st0;
st79:
	if ( ++p == pe )
		goto _test_eof79;
case 79:
	switch( (*p) ) {
		case 10u: goto st79;
		case 13u: goto st79;
		case 34u: goto st80;
		case 61u: goto st79;
		case 95u: goto st79;
	}
	if ( (*p) < 39u ) {
		if ( 32u <= (*p) && (*p) <= 37u )
			goto st79;
	} else if ( (*p) > 59u ) {
		if ( (*p) > 90u ) {
			if ( 97u <= (*p) && (*p) <= 122u )
				goto st79;
		} else if ( (*p) >= 63u )
			goto st79;
	} else
		goto st79;
	goto st0;
st80:
	if ( ++p == pe )
		goto _test_eof80;
case 80:
	switch( (*p) ) {
		case 13u: goto st81;
		case 32u: goto st81;
	}
	if ( 9u <= (*p) && (*p) <= 10u )
		goto st81;
	goto st0;
st81:
	if ( ++p == pe )
		goto _test_eof81;
case 81:
	switch( (*p) ) {
		case 13u: goto st81;
		case 32u: goto st81;
		case 34u: goto st82;
		case 39u: goto st313;
	}
	if ( 9u <= (*p) && (*p) <= 10u )
		goto st81;
	goto st0;
st82:
	if ( ++p == pe )
		goto _test_eof82;
case 82:
	if ( (*p) == 34u )
		goto st83;
	goto st82;
st83:
	if ( ++p == pe )
		goto _test_eof83;
case 83:
	switch( (*p) ) {
		case 13u: goto st83;
		case 32u: goto st83;
		case 62u: goto st20;
		case 91u: goto st84;
	}
	if ( 9u <= (*p) && (*p) <= 10u )
		goto st83;
	goto st0;
tr347:
#line 48 "../src/xml.ragel"
	{token("pi",-1);}
	goto st84;
st84:
	if ( ++p == pe )
		goto _test_eof84;
case 84:
#line 1775 "src/xml.cpp"
	switch( (*p) ) {
		case 13u: goto st84;
		case 32u: goto st84;
		case 37u: goto st85;
		case 60u: goto st87;
		case 93u: goto st312;
	}
	if ( 9u <= (*p) && (*p) <= 10u )
		goto st84;
	goto st0;
st85:
	if ( ++p == pe )
		goto _test_eof85;
case 85:
	switch( (*p) ) {
		case 58u: goto tr109;
		case 95u: goto tr109;
	}
	if ( (*p) > 90u ) {
		if ( 97u <= (*p) && (*p) <= 122u )
			goto tr109;
	} else if ( (*p) >= 65u )
		goto tr109;
	goto st0;
tr109:
#line 30 "../src/xml.ragel"
	{ m_output.push(m_char); }
	goto st86;
st86:
	if ( ++p == pe )
		goto _test_eof86;
case 86:
#line 1808 "src/xml.cpp"
	switch( (*p) ) {
		case 59u: goto st84;
		case 95u: goto tr109;
	}
	if ( (*p) < 48u ) {
		if ( 45u <= (*p) && (*p) <= 46u )
			goto tr109;
	} else if ( (*p) > 58u ) {
		if ( (*p) > 90u ) {
			if ( 97u <= (*p) && (*p) <= 122u )
				goto tr109;
		} else if ( (*p) >= 65u )
			goto tr109;
	} else
		goto tr109;
	goto st0;
st87:
	if ( ++p == pe )
		goto _test_eof87;
case 87:
	switch( (*p) ) {
		case 33u: goto st88;
		case 63u: goto st304;
	}
	goto st0;
st88:
	if ( ++p == pe )
		goto _test_eof88;
case 88:
	switch( (*p) ) {
		case 45u: goto st89;
		case 65u: goto st94;
		case 69u: goto st177;
		case 78u: goto st282;
	}
	goto st0;
st89:
	if ( ++p == pe )
		goto _test_eof89;
case 89:
	if ( (*p) == 45u )
		goto st90;
	goto st0;
st90:
	if ( ++p == pe )
		goto _test_eof90;
case 90:
	if ( (*p) == 45u )
		goto st91;
	goto st90;
st91:
	if ( ++p == pe )
		goto _test_eof91;
case 91:
	if ( (*p) == 45u )
		goto st92;
	goto st90;
st92:
	if ( ++p == pe )
		goto _test_eof92;
case 92:
	switch( (*p) ) {
		case 45u: goto st93;
		case 62u: goto st84;
	}
	goto st0;
st93:
	if ( ++p == pe )
		goto _test_eof93;
case 93:
	if ( (*p) == 62u )
		goto st84;
	goto st0;
st94:
	if ( ++p == pe )
		goto _test_eof94;
case 94:
	if ( (*p) == 84u )
		goto st95;
	goto st0;
st95:
	if ( ++p == pe )
		goto _test_eof95;
case 95:
	if ( (*p) == 84u )
		goto st96;
	goto st0;
st96:
	if ( ++p == pe )
		goto _test_eof96;
case 96:
	if ( (*p) == 76u )
		goto st97;
	goto st0;
st97:
	if ( ++p == pe )
		goto _test_eof97;
case 97:
	if ( (*p) == 73u )
		goto st98;
	goto st0;
st98:
	if ( ++p == pe )
		goto _test_eof98;
case 98:
	if ( (*p) == 83u )
		goto st99;
	goto st0;
st99:
	if ( ++p == pe )
		goto _test_eof99;
case 99:
	if ( (*p) == 84u )
		goto st100;
	goto st0;
st100:
	if ( ++p == pe )
		goto _test_eof100;
case 100:
	switch( (*p) ) {
		case 13u: goto st101;
		case 32u: goto st101;
	}
	if ( 9u <= (*p) && (*p) <= 10u )
		goto st101;
	goto st0;
st101:
	if ( ++p == pe )
		goto _test_eof101;
case 101:
	switch( (*p) ) {
		case 13u: goto st101;
		case 32u: goto st101;
		case 58u: goto tr127;
		case 95u: goto tr127;
	}
	if ( (*p) < 65u ) {
		if ( 9u <= (*p) && (*p) <= 10u )
			goto st101;
	} else if ( (*p) > 90u ) {
		if ( 97u <= (*p) && (*p) <= 122u )
			goto tr127;
	} else
		goto tr127;
	goto st0;
tr127:
#line 30 "../src/xml.ragel"
	{ m_output.push(m_char); }
	goto st102;
st102:
	if ( ++p == pe )
		goto _test_eof102;
case 102:
#line 1962 "src/xml.cpp"
	switch( (*p) ) {
		case 13u: goto st103;
		case 32u: goto st103;
		case 62u: goto st84;
		case 95u: goto tr127;
	}
	if ( (*p) < 48u ) {
		if ( (*p) > 10u ) {
			if ( 45u <= (*p) && (*p) <= 46u )
				goto tr127;
		} else if ( (*p) >= 9u )
			goto st103;
	} else if ( (*p) > 58u ) {
		if ( (*p) > 90u ) {
			if ( 97u <= (*p) && (*p) <= 122u )
				goto tr127;
		} else if ( (*p) >= 65u )
			goto tr127;
	} else
		goto tr127;
	goto st0;
st103:
	if ( ++p == pe )
		goto _test_eof103;
case 103:
	switch( (*p) ) {
		case 13u: goto st103;
		case 32u: goto st103;
		case 58u: goto tr129;
		case 62u: goto st84;
		case 95u: goto tr129;
	}
	if ( (*p) < 65u ) {
		if ( 9u <= (*p) && (*p) <= 10u )
			goto st103;
	} else if ( (*p) > 90u ) {
		if ( 97u <= (*p) && (*p) <= 122u )
			goto tr129;
	} else
		goto tr129;
	goto st0;
tr129:
#line 30 "../src/xml.ragel"
	{ m_output.push(m_char); }
	goto st104;
st104:
	if ( ++p == pe )
		goto _test_eof104;
case 104:
#line 2012 "src/xml.cpp"
	switch( (*p) ) {
		case 13u: goto st105;
		case 32u: goto st105;
		case 95u: goto tr129;
	}
	if ( (*p) < 48u ) {
		if ( (*p) > 10u ) {
			if ( 45u <= (*p) && (*p) <= 46u )
				goto tr129;
		} else if ( (*p) >= 9u )
			goto st105;
	} else if ( (*p) > 58u ) {
		if ( (*p) > 90u ) {
			if ( 97u <= (*p) && (*p) <= 122u )
				goto tr129;
		} else if ( (*p) >= 65u )
			goto tr129;
	} else
		goto tr129;
	goto st0;
st105:
	if ( ++p == pe )
		goto _test_eof105;
case 105:
	switch( (*p) ) {
		case 13u: goto st105;
		case 32u: goto st105;
		case 40u: goto st106;
		case 67u: goto st144;
		case 69u: goto st148;
		case 73u: goto st155;
		case 78u: goto st160;
	}
	if ( 9u <= (*p) && (*p) <= 10u )
		goto st105;
	goto st0;
st106:
	if ( ++p == pe )
		goto _test_eof106;
case 106:
	switch( (*p) ) {
		case 13u: goto st106;
		case 32u: goto st106;
		case 95u: goto st107;
	}
	if ( (*p) < 48u ) {
		if ( (*p) > 10u ) {
			if ( 45u <= (*p) && (*p) <= 46u )
				goto st107;
		} else if ( (*p) >= 9u )
			goto st106;
	} else if ( (*p) > 58u ) {
		if ( (*p) > 90u ) {
			if ( 97u <= (*p) && (*p) <= 122u )
				goto st107;
		} else if ( (*p) >= 65u )
			goto st107;
	} else
		goto st107;
	goto st0;
st107:
	if ( ++p == pe )
		goto _test_eof107;
case 107:
	switch( (*p) ) {
		case 13u: goto st108;
		case 32u: goto st108;
		case 41u: goto st109;
		case 95u: goto st107;
		case 124u: goto st106;
	}
	if ( (*p) < 48u ) {
		if ( (*p) > 10u ) {
			if ( 45u <= (*p) && (*p) <= 46u )
				goto st107;
		} else if ( (*p) >= 9u )
			goto st108;
	} else if ( (*p) > 58u ) {
		if ( (*p) > 90u ) {
			if ( 97u <= (*p) && (*p) <= 122u )
				goto st107;
		} else if ( (*p) >= 65u )
			goto st107;
	} else
		goto st107;
	goto st0;
st108:
	if ( ++p == pe )
		goto _test_eof108;
case 108:
	switch( (*p) ) {
		case 13u: goto st108;
		case 32u: goto st108;
		case 41u: goto st109;
		case 124u: goto st106;
	}
	if ( 9u <= (*p) && (*p) <= 10u )
		goto st108;
	goto st0;
st109:
	if ( ++p == pe )
		goto _test_eof109;
case 109:
	switch( (*p) ) {
		case 13u: goto st110;
		case 32u: goto st110;
	}
	if ( 9u <= (*p) && (*p) <= 10u )
		goto st110;
	goto st0;
st110:
	if ( ++p == pe )
		goto _test_eof110;
case 110:
	switch( (*p) ) {
		case 13u: goto st110;
		case 32u: goto st110;
		case 34u: goto st111;
		case 35u: goto st119;
		case 39u: goto st126;
	}
	if ( 9u <= (*p) && (*p) <= 10u )
		goto st110;
	goto st0;
tr143:
#line 30 "../src/xml.ragel"
	{ m_output.push(m_char); }
	goto st111;
tr150:
#line 63 "../src/xml.ragel"
	{subst_char();}
	goto st111;
tr152:
#line 63 "../src/xml.ragel"
	{subst_hex();}
	goto st111;
tr153:
#line 62 "../src/xml.ragel"
	{subst_entity();}
	goto st111;
st111:
	if ( ++p == pe )
		goto _test_eof111;
case 111:
#line 2157 "src/xml.cpp"
	switch( (*p) ) {
		case 34u: goto tr144;
		case 38u: goto st113;
		case 60u: goto st0;
	}
	goto tr143;
tr144:
#line 65 "../src/xml.ragel"
	{token("attr_value");}
	goto st112;
st112:
	if ( ++p == pe )
		goto _test_eof112;
case 112:
#line 2172 "src/xml.cpp"
	switch( (*p) ) {
		case 13u: goto st103;
		case 32u: goto st103;
		case 62u: goto st84;
	}
	if ( 9u <= (*p) && (*p) <= 10u )
		goto st103;
	goto st0;
st113:
	if ( ++p == pe )
		goto _test_eof113;
case 113:
	switch( (*p) ) {
		case 35u: goto st114;
		case 58u: goto tr147;
		case 95u: goto tr147;
	}
	if ( (*p) > 90u ) {
		if ( 97u <= (*p) && (*p) <= 122u )
			goto tr147;
	} else if ( (*p) >= 65u )
		goto tr147;
	goto st0;
st114:
	if ( ++p == pe )
		goto _test_eof114;
case 114:
	if ( (*p) == 120u )
		goto st116;
	if ( 48u <= (*p) && (*p) <= 57u )
		goto tr148;
	goto st0;
tr148:
#line 31 "../src/xml.ragel"
	{ m_entity.push(m_char); }
	goto st115;
st115:
	if ( ++p == pe )
		goto _test_eof115;
case 115:
#line 2213 "src/xml.cpp"
	if ( (*p) == 59u )
		goto tr150;
	if ( 48u <= (*p) && (*p) <= 57u )
		goto tr148;
	goto st0;
st116:
	if ( ++p == pe )
		goto _test_eof116;
case 116:
	if ( (*p) < 65u ) {
		if ( 48u <= (*p) && (*p) <= 57u )
			goto tr151;
	} else if ( (*p) > 70u ) {
		if ( 97u <= (*p) && (*p) <= 102u )
			goto tr151;
	} else
		goto tr151;
	goto st0;
tr151:
#line 31 "../src/xml.ragel"
	{ m_entity.push(m_char); }
	goto st117;
st117:
	if ( ++p == pe )
		goto _test_eof117;
case 117:
#line 2240 "src/xml.cpp"
	if ( (*p) == 59u )
		goto tr152;
	if ( (*p) < 65u ) {
		if ( 48u <= (*p) && (*p) <= 57u )
			goto tr151;
	} else if ( (*p) > 70u ) {
		if ( 97u <= (*p) && (*p) <= 102u )
			goto tr151;
	} else
		goto tr151;
	goto st0;
tr147:
#line 31 "../src/xml.ragel"
	{ m_entity.push(m_char); }
	goto st118;
st118:
	if ( ++p == pe )
		goto _test_eof118;
case 118:
#line 2260 "src/xml.cpp"
	switch( (*p) ) {
		case 59u: goto tr153;
		case 95u: goto tr147;
	}
	if ( (*p) < 48u ) {
		if ( 45u <= (*p) && (*p) <= 46u )
			goto tr147;
	} else if ( (*p) > 58u ) {
		if ( (*p) > 90u ) {
			if ( 97u <= (*p) && (*p) <= 122u )
				goto tr147;
		} else if ( (*p) >= 65u )
			goto tr147;
	} else
		goto tr147;
	goto st0;
st119:
	if ( ++p == pe )
		goto _test_eof119;
case 119:
	switch( (*p) ) {
		case 70u: goto st120;
		case 73u: goto st133;
		case 82u: goto st139;
	}
	goto st0;
st120:
	if ( ++p == pe )
		goto _test_eof120;
case 120:
	if ( (*p) == 73u )
		goto st121;
	goto st0;
st121:
	if ( ++p == pe )
		goto _test_eof121;
case 121:
	if ( (*p) == 88u )
		goto st122;
	goto st0;
st122:
	if ( ++p == pe )
		goto _test_eof122;
case 122:
	if ( (*p) == 69u )
		goto st123;
	goto st0;
st123:
	if ( ++p == pe )
		goto _test_eof123;
case 123:
	if ( (*p) == 68u )
		goto st124;
	goto st0;
st124:
	if ( ++p == pe )
		goto _test_eof124;
case 124:
	switch( (*p) ) {
		case 13u: goto st125;
		case 32u: goto st125;
	}
	if ( 9u <= (*p) && (*p) <= 10u )
		goto st125;
	goto st0;
st125:
	if ( ++p == pe )
		goto _test_eof125;
case 125:
	switch( (*p) ) {
		case 13u: goto st125;
		case 32u: goto st125;
		case 34u: goto st111;
		case 39u: goto st126;
	}
	if ( 9u <= (*p) && (*p) <= 10u )
		goto st125;
	goto st0;
tr162:
#line 30 "../src/xml.ragel"
	{ m_output.push(m_char); }
	goto st126;
tr168:
#line 63 "../src/xml.ragel"
	{subst_char();}
	goto st126;
tr170:
#line 63 "../src/xml.ragel"
	{subst_hex();}
	goto st126;
tr171:
#line 62 "../src/xml.ragel"
	{subst_entity();}
	goto st126;
st126:
	if ( ++p == pe )
		goto _test_eof126;
case 126:
#line 2359 "src/xml.cpp"
	switch( (*p) ) {
		case 38u: goto st127;
		case 39u: goto tr144;
		case 60u: goto st0;
	}
	goto tr162;
st127:
	if ( ++p == pe )
		goto _test_eof127;
case 127:
	switch( (*p) ) {
		case 35u: goto st128;
		case 58u: goto tr165;
		case 95u: goto tr165;
	}
	if ( (*p) > 90u ) {
		if ( 97u <= (*p) && (*p) <= 122u )
			goto tr165;
	} else if ( (*p) >= 65u )
		goto tr165;
	goto st0;
st128:
	if ( ++p == pe )
		goto _test_eof128;
case 128:
	if ( (*p) == 120u )
		goto st130;
	if ( 48u <= (*p) && (*p) <= 57u )
		goto tr166;
	goto st0;
tr166:
#line 31 "../src/xml.ragel"
	{ m_entity.push(m_char); }
	goto st129;
st129:
	if ( ++p == pe )
		goto _test_eof129;
case 129:
#line 2398 "src/xml.cpp"
	if ( (*p) == 59u )
		goto tr168;
	if ( 48u <= (*p) && (*p) <= 57u )
		goto tr166;
	goto st0;
st130:
	if ( ++p == pe )
		goto _test_eof130;
case 130:
	if ( (*p) < 65u ) {
		if ( 48u <= (*p) && (*p) <= 57u )
			goto tr169;
	} else if ( (*p) > 70u ) {
		if ( 97u <= (*p) && (*p) <= 102u )
			goto tr169;
	} else
		goto tr169;
	goto st0;
tr169:
#line 31 "../src/xml.ragel"
	{ m_entity.push(m_char); }
	goto st131;
st131:
	if ( ++p == pe )
		goto _test_eof131;
case 131:
#line 2425 "src/xml.cpp"
	if ( (*p) == 59u )
		goto tr170;
	if ( (*p) < 65u ) {
		if ( 48u <= (*p) && (*p) <= 57u )
			goto tr169;
	} else if ( (*p) > 70u ) {
		if ( 97u <= (*p) && (*p) <= 102u )
			goto tr169;
	} else
		goto tr169;
	goto st0;
tr165:
#line 31 "../src/xml.ragel"
	{ m_entity.push(m_char); }
	goto st132;
st132:
	if ( ++p == pe )
		goto _test_eof132;
case 132:
#line 2445 "src/xml.cpp"
	switch( (*p) ) {
		case 59u: goto tr171;
		case 95u: goto tr165;
	}
	if ( (*p) < 48u ) {
		if ( 45u <= (*p) && (*p) <= 46u )
			goto tr165;
	} else if ( (*p) > 58u ) {
		if ( (*p) > 90u ) {
			if ( 97u <= (*p) && (*p) <= 122u )
				goto tr165;
		} else if ( (*p) >= 65u )
			goto tr165;
	} else
		goto tr165;
	goto st0;
st133:
	if ( ++p == pe )
		goto _test_eof133;
case 133:
	if ( (*p) == 77u )
		goto st134;
	goto st0;
st134:
	if ( ++p == pe )
		goto _test_eof134;
case 134:
	if ( (*p) == 80u )
		goto st135;
	goto st0;
st135:
	if ( ++p == pe )
		goto _test_eof135;
case 135:
	if ( (*p) == 76u )
		goto st136;
	goto st0;
st136:
	if ( ++p == pe )
		goto _test_eof136;
case 136:
	if ( (*p) == 73u )
		goto st137;
	goto st0;
st137:
	if ( ++p == pe )
		goto _test_eof137;
case 137:
	if ( (*p) == 69u )
		goto st138;
	goto st0;
st138:
	if ( ++p == pe )
		goto _test_eof138;
case 138:
	if ( (*p) == 68u )
		goto st112;
	goto st0;
st139:
	if ( ++p == pe )
		goto _test_eof139;
case 139:
	if ( (*p) == 69u )
		goto st140;
	goto st0;
st140:
	if ( ++p == pe )
		goto _test_eof140;
case 140:
	if ( (*p) == 81u )
		goto st141;
	goto st0;
st141:
	if ( ++p == pe )
		goto _test_eof141;
case 141:
	if ( (*p) == 85u )
		goto st142;
	goto st0;
st142:
	if ( ++p == pe )
		goto _test_eof142;
case 142:
	if ( (*p) == 73u )
		goto st143;
	goto st0;
st143:
	if ( ++p == pe )
		goto _test_eof143;
case 143:
	if ( (*p) == 82u )
		goto st137;
	goto st0;
st144:
	if ( ++p == pe )
		goto _test_eof144;
case 144:
	if ( (*p) == 68u )
		goto st145;
	goto st0;
st145:
	if ( ++p == pe )
		goto _test_eof145;
case 145:
	if ( (*p) == 65u )
		goto st146;
	goto st0;
st146:
	if ( ++p == pe )
		goto _test_eof146;
case 146:
	if ( (*p) == 84u )
		goto st147;
	goto st0;
st147:
	if ( ++p == pe )
		goto _test_eof147;
case 147:
	if ( (*p) == 65u )
		goto st109;
	goto st0;
st148:
	if ( ++p == pe )
		goto _test_eof148;
case 148:
	if ( (*p) == 78u )
		goto st149;
	goto st0;
st149:
	if ( ++p == pe )
		goto _test_eof149;
case 149:
	if ( (*p) == 84u )
		goto st150;
	goto st0;
st150:
	if ( ++p == pe )
		goto _test_eof150;
case 150:
	if ( (*p) == 73u )
		goto st151;
	goto st0;
st151:
	if ( ++p == pe )
		goto _test_eof151;
case 151:
	if ( (*p) == 84u )
		goto st152;
	goto st0;
st152:
	if ( ++p == pe )
		goto _test_eof152;
case 152:
	switch( (*p) ) {
		case 73u: goto st153;
		case 89u: goto st109;
	}
	goto st0;
st153:
	if ( ++p == pe )
		goto _test_eof153;
case 153:
	if ( (*p) == 69u )
		goto st154;
	goto st0;
st154:
	if ( ++p == pe )
		goto _test_eof154;
case 154:
	if ( (*p) == 83u )
		goto st109;
	goto st0;
st155:
	if ( ++p == pe )
		goto _test_eof155;
case 155:
	if ( (*p) == 68u )
		goto st156;
	goto st0;
st156:
	if ( ++p == pe )
		goto _test_eof156;
case 156:
	switch( (*p) ) {
		case 13u: goto st110;
		case 32u: goto st110;
		case 82u: goto st157;
	}
	if ( 9u <= (*p) && (*p) <= 10u )
		goto st110;
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
	if ( (*p) == 70u )
		goto st159;
	goto st0;
st159:
	if ( ++p == pe )
		goto _test_eof159;
case 159:
	switch( (*p) ) {
		case 13u: goto st110;
		case 32u: goto st110;
		case 83u: goto st109;
	}
	if ( 9u <= (*p) && (*p) <= 10u )
		goto st110;
	goto st0;
st160:
	if ( ++p == pe )
		goto _test_eof160;
case 160:
	switch( (*p) ) {
		case 77u: goto st161;
		case 79u: goto st166;
	}
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
	if ( (*p) == 79u )
		goto st163;
	goto st0;
st163:
	if ( ++p == pe )
		goto _test_eof163;
case 163:
	if ( (*p) == 75u )
		goto st164;
	goto st0;
st164:
	if ( ++p == pe )
		goto _test_eof164;
case 164:
	if ( (*p) == 69u )
		goto st165;
	goto st0;
st165:
	if ( ++p == pe )
		goto _test_eof165;
case 165:
	if ( (*p) == 78u )
		goto st159;
	goto st0;
st166:
	if ( ++p == pe )
		goto _test_eof166;
case 166:
	if ( (*p) == 84u )
		goto st167;
	goto st0;
st167:
	if ( ++p == pe )
		goto _test_eof167;
case 167:
	if ( (*p) == 65u )
		goto st168;
	goto st0;
st168:
	if ( ++p == pe )
		goto _test_eof168;
case 168:
	if ( (*p) == 84u )
		goto st169;
	goto st0;
st169:
	if ( ++p == pe )
		goto _test_eof169;
case 169:
	if ( (*p) == 73u )
		goto st170;
	goto st0;
st170:
	if ( ++p == pe )
		goto _test_eof170;
case 170:
	if ( (*p) == 79u )
		goto st171;
	goto st0;
st171:
	if ( ++p == pe )
		goto _test_eof171;
case 171:
	if ( (*p) == 78u )
		goto st172;
	goto st0;
st172:
	if ( ++p == pe )
		goto _test_eof172;
case 172:
	switch( (*p) ) {
		case 13u: goto st173;
		case 32u: goto st173;
	}
	if ( 9u <= (*p) && (*p) <= 10u )
		goto st173;
	goto st0;
st173:
	if ( ++p == pe )
		goto _test_eof173;
case 173:
	switch( (*p) ) {
		case 13u: goto st173;
		case 32u: goto st173;
		case 40u: goto st174;
	}
	if ( 9u <= (*p) && (*p) <= 10u )
		goto st173;
	goto st0;
st174:
	if ( ++p == pe )
		goto _test_eof174;
case 174:
	switch( (*p) ) {
		case 13u: goto st174;
		case 32u: goto st174;
		case 58u: goto tr209;
		case 95u: goto tr209;
	}
	if ( (*p) < 65u ) {
		if ( 9u <= (*p) && (*p) <= 10u )
			goto st174;
	} else if ( (*p) > 90u ) {
		if ( 97u <= (*p) && (*p) <= 122u )
			goto tr209;
	} else
		goto tr209;
	goto st0;
tr209:
#line 30 "../src/xml.ragel"
	{ m_output.push(m_char); }
	goto st175;
st175:
	if ( ++p == pe )
		goto _test_eof175;
case 175:
#line 2799 "src/xml.cpp"
	switch( (*p) ) {
		case 13u: goto st176;
		case 32u: goto st176;
		case 41u: goto st109;
		case 95u: goto tr209;
		case 124u: goto st174;
	}
	if ( (*p) < 48u ) {
		if ( (*p) > 10u ) {
			if ( 45u <= (*p) && (*p) <= 46u )
				goto tr209;
		} else if ( (*p) >= 9u )
			goto st176;
	} else if ( (*p) > 58u ) {
		if ( (*p) > 90u ) {
			if ( 97u <= (*p) && (*p) <= 122u )
				goto tr209;
		} else if ( (*p) >= 65u )
			goto tr209;
	} else
		goto tr209;
	goto st0;
st176:
	if ( ++p == pe )
		goto _test_eof176;
case 176:
	switch( (*p) ) {
		case 13u: goto st176;
		case 32u: goto st176;
		case 41u: goto st109;
		case 124u: goto st174;
	}
	if ( 9u <= (*p) && (*p) <= 10u )
		goto st176;
	goto st0;
st177:
	if ( ++p == pe )
		goto _test_eof177;
case 177:
	switch( (*p) ) {
		case 76u: goto st178;
		case 78u: goto st207;
	}
	goto st0;
st178:
	if ( ++p == pe )
		goto _test_eof178;
case 178:
	if ( (*p) == 69u )
		goto st179;
	goto st0;
st179:
	if ( ++p == pe )
		goto _test_eof179;
case 179:
	if ( (*p) == 77u )
		goto st180;
	goto st0;
st180:
	if ( ++p == pe )
		goto _test_eof180;
case 180:
	if ( (*p) == 69u )
		goto st181;
	goto st0;
st181:
	if ( ++p == pe )
		goto _test_eof181;
case 181:
	if ( (*p) == 78u )
		goto st182;
	goto st0;
st182:
	if ( ++p == pe )
		goto _test_eof182;
case 182:
	if ( (*p) == 84u )
		goto st183;
	goto st0;
st183:
	if ( ++p == pe )
		goto _test_eof183;
case 183:
	switch( (*p) ) {
		case 13u: goto st184;
		case 32u: goto st184;
	}
	if ( 9u <= (*p) && (*p) <= 10u )
		goto st184;
	goto st0;
st184:
	if ( ++p == pe )
		goto _test_eof184;
case 184:
	switch( (*p) ) {
		case 13u: goto st184;
		case 32u: goto st184;
		case 58u: goto tr219;
		case 95u: goto tr219;
	}
	if ( (*p) < 65u ) {
		if ( 9u <= (*p) && (*p) <= 10u )
			goto st184;
	} else if ( (*p) > 90u ) {
		if ( 97u <= (*p) && (*p) <= 122u )
			goto tr219;
	} else
		goto tr219;
	goto st0;
tr219:
#line 30 "../src/xml.ragel"
	{ m_output.push(m_char); }
	goto st185;
st185:
	if ( ++p == pe )
		goto _test_eof185;
case 185:
#line 2917 "src/xml.cpp"
	switch( (*p) ) {
		case 13u: goto st186;
		case 32u: goto st186;
		case 95u: goto tr219;
	}
	if ( (*p) < 48u ) {
		if ( (*p) > 10u ) {
			if ( 45u <= (*p) && (*p) <= 46u )
				goto tr219;
		} else if ( (*p) >= 9u )
			goto st186;
	} else if ( (*p) > 58u ) {
		if ( (*p) > 90u ) {
			if ( 97u <= (*p) && (*p) <= 122u )
				goto tr219;
		} else if ( (*p) >= 65u )
			goto tr219;
	} else
		goto tr219;
	goto st0;
st186:
	if ( ++p == pe )
		goto _test_eof186;
case 186:
	switch( (*p) ) {
		case 13u: goto st186;
		case 32u: goto st186;
		case 40u: goto tr221;
		case 65u: goto st202;
		case 69u: goto st204;
	}
	if ( 9u <= (*p) && (*p) <= 10u )
		goto st186;
	goto st0;
tr221:
#line 87 "../src/xml.ragel"
	{ { pre_push(); { this->m_stack[ this->m_top++] = 187; goto st389;}} }
	goto st187;
st187:
	if ( ++p == pe )
		goto _test_eof187;
case 187:
#line 2960 "src/xml.cpp"
	switch( (*p) ) {
		case 13u: goto st188;
		case 32u: goto st188;
		case 35u: goto st189;
		case 62u: goto st84;
		case 63u: goto st197;
	}
	if ( (*p) > 10u ) {
		if ( 42u <= (*p) && (*p) <= 43u )
			goto st197;
	} else if ( (*p) >= 9u )
		goto st188;
	goto st0;
st188:
	if ( ++p == pe )
		goto _test_eof188;
case 188:
	switch( (*p) ) {
		case 13u: goto st188;
		case 32u: goto st188;
		case 35u: goto st189;
		case 62u: goto st84;
	}
	if ( 9u <= (*p) && (*p) <= 10u )
		goto st188;
	goto st0;
st189:
	if ( ++p == pe )
		goto _test_eof189;
case 189:
	if ( (*p) == 80u )
		goto st190;
	goto st0;
st190:
	if ( ++p == pe )
		goto _test_eof190;
case 190:
	if ( (*p) == 67u )
		goto st191;
	goto st0;
st191:
	if ( ++p == pe )
		goto _test_eof191;
case 191:
	if ( (*p) == 68u )
		goto st192;
	goto st0;
st192:
	if ( ++p == pe )
		goto _test_eof192;
case 192:
	if ( (*p) == 65u )
		goto st193;
	goto st0;
st193:
	if ( ++p == pe )
		goto _test_eof193;
case 193:
	if ( (*p) == 84u )
		goto st194;
	goto st0;
st194:
	if ( ++p == pe )
		goto _test_eof194;
case 194:
	if ( (*p) == 65u )
		goto st195;
	goto st0;
st195:
	if ( ++p == pe )
		goto _test_eof195;
case 195:
	switch( (*p) ) {
		case 13u: goto st195;
		case 32u: goto st195;
		case 41u: goto st196;
		case 124u: goto st198;
	}
	if ( 9u <= (*p) && (*p) <= 10u )
		goto st195;
	goto st0;
st196:
	if ( ++p == pe )
		goto _test_eof196;
case 196:
	switch( (*p) ) {
		case 13u: goto st197;
		case 32u: goto st197;
		case 42u: goto st197;
		case 62u: goto st84;
	}
	if ( 9u <= (*p) && (*p) <= 10u )
		goto st197;
	goto st0;
st197:
	if ( ++p == pe )
		goto _test_eof197;
case 197:
	switch( (*p) ) {
		case 13u: goto st197;
		case 32u: goto st197;
		case 62u: goto st84;
	}
	if ( 9u <= (*p) && (*p) <= 10u )
		goto st197;
	goto st0;
st198:
	if ( ++p == pe )
		goto _test_eof198;
case 198:
	switch( (*p) ) {
		case 13u: goto st198;
		case 32u: goto st198;
		case 58u: goto tr235;
		case 95u: goto tr235;
	}
	if ( (*p) < 65u ) {
		if ( 9u <= (*p) && (*p) <= 10u )
			goto st198;
	} else if ( (*p) > 90u ) {
		if ( 97u <= (*p) && (*p) <= 122u )
			goto tr235;
	} else
		goto tr235;
	goto st0;
tr235:
#line 30 "../src/xml.ragel"
	{ m_output.push(m_char); }
	goto st199;
st199:
	if ( ++p == pe )
		goto _test_eof199;
case 199:
#line 3094 "src/xml.cpp"
	switch( (*p) ) {
		case 13u: goto st200;
		case 32u: goto st200;
		case 41u: goto st201;
		case 95u: goto tr235;
		case 124u: goto st198;
	}
	if ( (*p) < 48u ) {
		if ( (*p) > 10u ) {
			if ( 45u <= (*p) && (*p) <= 46u )
				goto tr235;
		} else if ( (*p) >= 9u )
			goto st200;
	} else if ( (*p) > 58u ) {
		if ( (*p) > 90u ) {
			if ( 97u <= (*p) && (*p) <= 122u )
				goto tr235;
		} else if ( (*p) >= 65u )
			goto tr235;
	} else
		goto tr235;
	goto st0;
st200:
	if ( ++p == pe )
		goto _test_eof200;
case 200:
	switch( (*p) ) {
		case 13u: goto st200;
		case 32u: goto st200;
		case 41u: goto st201;
		case 124u: goto st198;
	}
	if ( 9u <= (*p) && (*p) <= 10u )
		goto st200;
	goto st0;
st201:
	if ( ++p == pe )
		goto _test_eof201;
case 201:
	if ( (*p) == 42u )
		goto st197;
	goto st0;
st202:
	if ( ++p == pe )
		goto _test_eof202;
case 202:
	if ( (*p) == 78u )
		goto st203;
	goto st0;
st203:
	if ( ++p == pe )
		goto _test_eof203;
case 203:
	if ( (*p) == 89u )
		goto st197;
	goto st0;
st204:
	if ( ++p == pe )
		goto _test_eof204;
case 204:
	if ( (*p) == 77u )
		goto st205;
	goto st0;
st205:
	if ( ++p == pe )
		goto _test_eof205;
case 205:
	if ( (*p) == 80u )
		goto st206;
	goto st0;
st206:
	if ( ++p == pe )
		goto _test_eof206;
case 206:
	if ( (*p) == 84u )
		goto st203;
	goto st0;
st207:
	if ( ++p == pe )
		goto _test_eof207;
case 207:
	if ( (*p) == 84u )
		goto st208;
	goto st0;
st208:
	if ( ++p == pe )
		goto _test_eof208;
case 208:
	if ( (*p) == 73u )
		goto st209;
	goto st0;
st209:
	if ( ++p == pe )
		goto _test_eof209;
case 209:
	if ( (*p) == 84u )
		goto st210;
	goto st0;
st210:
	if ( ++p == pe )
		goto _test_eof210;
case 210:
	if ( (*p) == 89u )
		goto st211;
	goto st0;
st211:
	if ( ++p == pe )
		goto _test_eof211;
case 211:
	switch( (*p) ) {
		case 13u: goto st212;
		case 32u: goto st212;
	}
	if ( 9u <= (*p) && (*p) <= 10u )
		goto st212;
	goto st0;
st212:
	if ( ++p == pe )
		goto _test_eof212;
case 212:
	switch( (*p) ) {
		case 13u: goto st212;
		case 32u: goto st212;
		case 37u: goto st213;
		case 58u: goto tr247;
		case 95u: goto tr247;
	}
	if ( (*p) < 65u ) {
		if ( 9u <= (*p) && (*p) <= 10u )
			goto st212;
	} else if ( (*p) > 90u ) {
		if ( 97u <= (*p) && (*p) <= 122u )
			goto tr247;
	} else
		goto tr247;
	goto st0;
st213:
	if ( ++p == pe )
		goto _test_eof213;
case 213:
	switch( (*p) ) {
		case 13u: goto st214;
		case 32u: goto st214;
	}
	if ( 9u <= (*p) && (*p) <= 10u )
		goto st214;
	goto st0;
st214:
	if ( ++p == pe )
		goto _test_eof214;
case 214:
	switch( (*p) ) {
		case 13u: goto st214;
		case 32u: goto st214;
		case 58u: goto tr249;
		case 95u: goto tr249;
	}
	if ( (*p) < 65u ) {
		if ( 9u <= (*p) && (*p) <= 10u )
			goto st214;
	} else if ( (*p) > 90u ) {
		if ( 97u <= (*p) && (*p) <= 122u )
			goto tr249;
	} else
		goto tr249;
	goto st0;
tr249:
#line 30 "../src/xml.ragel"
	{ m_output.push(m_char); }
	goto st215;
st215:
	if ( ++p == pe )
		goto _test_eof215;
case 215:
#line 3269 "src/xml.cpp"
	switch( (*p) ) {
		case 13u: goto st216;
		case 32u: goto st216;
		case 95u: goto tr249;
	}
	if ( (*p) < 48u ) {
		if ( (*p) > 10u ) {
			if ( 45u <= (*p) && (*p) <= 46u )
				goto tr249;
		} else if ( (*p) >= 9u )
			goto st216;
	} else if ( (*p) > 58u ) {
		if ( (*p) > 90u ) {
			if ( 97u <= (*p) && (*p) <= 122u )
				goto tr249;
		} else if ( (*p) >= 65u )
			goto tr249;
	} else
		goto tr249;
	goto st0;
st216:
	if ( ++p == pe )
		goto _test_eof216;
case 216:
	switch( (*p) ) {
		case 13u: goto st216;
		case 32u: goto st216;
		case 34u: goto st217;
		case 39u: goto st226;
		case 80u: goto st235;
		case 83u: goto st248;
	}
	if ( 9u <= (*p) && (*p) <= 10u )
		goto st216;
	goto st0;
tr262:
#line 63 "../src/xml.ragel"
	{subst_char();}
	goto st217;
tr264:
#line 63 "../src/xml.ragel"
	{subst_hex();}
	goto st217;
tr265:
#line 62 "../src/xml.ragel"
	{subst_entity();}
	goto st217;
st217:
	if ( ++p == pe )
		goto _test_eof217;
case 217:
#line 3321 "src/xml.cpp"
	switch( (*p) ) {
		case 34u: goto st197;
		case 37u: goto st218;
		case 38u: goto st220;
	}
	goto st217;
st218:
	if ( ++p == pe )
		goto _test_eof218;
case 218:
	switch( (*p) ) {
		case 58u: goto tr257;
		case 95u: goto tr257;
	}
	if ( (*p) > 90u ) {
		if ( 97u <= (*p) && (*p) <= 122u )
			goto tr257;
	} else if ( (*p) >= 65u )
		goto tr257;
	goto st0;
tr257:
#line 30 "../src/xml.ragel"
	{ m_output.push(m_char); }
	goto st219;
st219:
	if ( ++p == pe )
		goto _test_eof219;
case 219:
#line 3350 "src/xml.cpp"
	switch( (*p) ) {
		case 59u: goto st217;
		case 95u: goto tr257;
	}
	if ( (*p) < 48u ) {
		if ( 45u <= (*p) && (*p) <= 46u )
			goto tr257;
	} else if ( (*p) > 58u ) {
		if ( (*p) > 90u ) {
			if ( 97u <= (*p) && (*p) <= 122u )
				goto tr257;
		} else if ( (*p) >= 65u )
			goto tr257;
	} else
		goto tr257;
	goto st0;
st220:
	if ( ++p == pe )
		goto _test_eof220;
case 220:
	switch( (*p) ) {
		case 35u: goto st221;
		case 58u: goto tr259;
		case 95u: goto tr259;
	}
	if ( (*p) > 90u ) {
		if ( 97u <= (*p) && (*p) <= 122u )
			goto tr259;
	} else if ( (*p) >= 65u )
		goto tr259;
	goto st0;
st221:
	if ( ++p == pe )
		goto _test_eof221;
case 221:
	if ( (*p) == 120u )
		goto st223;
	if ( 48u <= (*p) && (*p) <= 57u )
		goto tr260;
	goto st0;
tr260:
#line 31 "../src/xml.ragel"
	{ m_entity.push(m_char); }
	goto st222;
st222:
	if ( ++p == pe )
		goto _test_eof222;
case 222:
#line 3399 "src/xml.cpp"
	if ( (*p) == 59u )
		goto tr262;
	if ( 48u <= (*p) && (*p) <= 57u )
		goto tr260;
	goto st0;
st223:
	if ( ++p == pe )
		goto _test_eof223;
case 223:
	if ( (*p) < 65u ) {
		if ( 48u <= (*p) && (*p) <= 57u )
			goto tr263;
	} else if ( (*p) > 70u ) {
		if ( 97u <= (*p) && (*p) <= 102u )
			goto tr263;
	} else
		goto tr263;
	goto st0;
tr263:
#line 31 "../src/xml.ragel"
	{ m_entity.push(m_char); }
	goto st224;
st224:
	if ( ++p == pe )
		goto _test_eof224;
case 224:
#line 3426 "src/xml.cpp"
	if ( (*p) == 59u )
		goto tr264;
	if ( (*p) < 65u ) {
		if ( 48u <= (*p) && (*p) <= 57u )
			goto tr263;
	} else if ( (*p) > 70u ) {
		if ( 97u <= (*p) && (*p) <= 102u )
			goto tr263;
	} else
		goto tr263;
	goto st0;
tr259:
#line 31 "../src/xml.ragel"
	{ m_entity.push(m_char); }
	goto st225;
st225:
	if ( ++p == pe )
		goto _test_eof225;
case 225:
#line 3446 "src/xml.cpp"
	switch( (*p) ) {
		case 59u: goto tr265;
		case 95u: goto tr259;
	}
	if ( (*p) < 48u ) {
		if ( 45u <= (*p) && (*p) <= 46u )
			goto tr259;
	} else if ( (*p) > 58u ) {
		if ( (*p) > 90u ) {
			if ( 97u <= (*p) && (*p) <= 122u )
				goto tr259;
		} else if ( (*p) >= 65u )
			goto tr259;
	} else
		goto tr259;
	goto st0;
tr273:
#line 63 "../src/xml.ragel"
	{subst_char();}
	goto st226;
tr275:
#line 63 "../src/xml.ragel"
	{subst_hex();}
	goto st226;
tr276:
#line 62 "../src/xml.ragel"
	{subst_entity();}
	goto st226;
st226:
	if ( ++p == pe )
		goto _test_eof226;
case 226:
#line 3479 "src/xml.cpp"
	switch( (*p) ) {
		case 37u: goto st227;
		case 38u: goto st229;
		case 39u: goto st197;
	}
	goto st226;
st227:
	if ( ++p == pe )
		goto _test_eof227;
case 227:
	switch( (*p) ) {
		case 58u: goto tr268;
		case 95u: goto tr268;
	}
	if ( (*p) > 90u ) {
		if ( 97u <= (*p) && (*p) <= 122u )
			goto tr268;
	} else if ( (*p) >= 65u )
		goto tr268;
	goto st0;
tr268:
#line 30 "../src/xml.ragel"
	{ m_output.push(m_char); }
	goto st228;
st228:
	if ( ++p == pe )
		goto _test_eof228;
case 228:
#line 3508 "src/xml.cpp"
	switch( (*p) ) {
		case 59u: goto st226;
		case 95u: goto tr268;
	}
	if ( (*p) < 48u ) {
		if ( 45u <= (*p) && (*p) <= 46u )
			goto tr268;
	} else if ( (*p) > 58u ) {
		if ( (*p) > 90u ) {
			if ( 97u <= (*p) && (*p) <= 122u )
				goto tr268;
		} else if ( (*p) >= 65u )
			goto tr268;
	} else
		goto tr268;
	goto st0;
st229:
	if ( ++p == pe )
		goto _test_eof229;
case 229:
	switch( (*p) ) {
		case 35u: goto st230;
		case 58u: goto tr270;
		case 95u: goto tr270;
	}
	if ( (*p) > 90u ) {
		if ( 97u <= (*p) && (*p) <= 122u )
			goto tr270;
	} else if ( (*p) >= 65u )
		goto tr270;
	goto st0;
st230:
	if ( ++p == pe )
		goto _test_eof230;
case 230:
	if ( (*p) == 120u )
		goto st232;
	if ( 48u <= (*p) && (*p) <= 57u )
		goto tr271;
	goto st0;
tr271:
#line 31 "../src/xml.ragel"
	{ m_entity.push(m_char); }
	goto st231;
st231:
	if ( ++p == pe )
		goto _test_eof231;
case 231:
#line 3557 "src/xml.cpp"
	if ( (*p) == 59u )
		goto tr273;
	if ( 48u <= (*p) && (*p) <= 57u )
		goto tr271;
	goto st0;
st232:
	if ( ++p == pe )
		goto _test_eof232;
case 232:
	if ( (*p) < 65u ) {
		if ( 48u <= (*p) && (*p) <= 57u )
			goto tr274;
	} else if ( (*p) > 70u ) {
		if ( 97u <= (*p) && (*p) <= 102u )
			goto tr274;
	} else
		goto tr274;
	goto st0;
tr274:
#line 31 "../src/xml.ragel"
	{ m_entity.push(m_char); }
	goto st233;
st233:
	if ( ++p == pe )
		goto _test_eof233;
case 233:
#line 3584 "src/xml.cpp"
	if ( (*p) == 59u )
		goto tr275;
	if ( (*p) < 65u ) {
		if ( 48u <= (*p) && (*p) <= 57u )
			goto tr274;
	} else if ( (*p) > 70u ) {
		if ( 97u <= (*p) && (*p) <= 102u )
			goto tr274;
	} else
		goto tr274;
	goto st0;
tr270:
#line 31 "../src/xml.ragel"
	{ m_entity.push(m_char); }
	goto st234;
st234:
	if ( ++p == pe )
		goto _test_eof234;
case 234:
#line 3604 "src/xml.cpp"
	switch( (*p) ) {
		case 59u: goto tr276;
		case 95u: goto tr270;
	}
	if ( (*p) < 48u ) {
		if ( 45u <= (*p) && (*p) <= 46u )
			goto tr270;
	} else if ( (*p) > 58u ) {
		if ( (*p) > 90u ) {
			if ( 97u <= (*p) && (*p) <= 122u )
				goto tr270;
		} else if ( (*p) >= 65u )
			goto tr270;
	} else
		goto tr270;
	goto st0;
st235:
	if ( ++p == pe )
		goto _test_eof235;
case 235:
	if ( (*p) == 85u )
		goto st236;
	goto st0;
st236:
	if ( ++p == pe )
		goto _test_eof236;
case 236:
	if ( (*p) == 66u )
		goto st237;
	goto st0;
st237:
	if ( ++p == pe )
		goto _test_eof237;
case 237:
	if ( (*p) == 76u )
		goto st238;
	goto st0;
st238:
	if ( ++p == pe )
		goto _test_eof238;
case 238:
	if ( (*p) == 73u )
		goto st239;
	goto st0;
st239:
	if ( ++p == pe )
		goto _test_eof239;
case 239:
	if ( (*p) == 67u )
		goto st240;
	goto st0;
st240:
	if ( ++p == pe )
		goto _test_eof240;
case 240:
	switch( (*p) ) {
		case 13u: goto st241;
		case 32u: goto st241;
	}
	if ( 9u <= (*p) && (*p) <= 10u )
		goto st241;
	goto st0;
st241:
	if ( ++p == pe )
		goto _test_eof241;
case 241:
	switch( (*p) ) {
		case 13u: goto st241;
		case 32u: goto st241;
		case 34u: goto st242;
		case 39u: goto st247;
	}
	if ( 9u <= (*p) && (*p) <= 10u )
		goto st241;
	goto st0;
st242:
	if ( ++p == pe )
		goto _test_eof242;
case 242:
	switch( (*p) ) {
		case 10u: goto st242;
		case 13u: goto st242;
		case 34u: goto st243;
		case 61u: goto st242;
		case 95u: goto st242;
	}
	if ( (*p) < 39u ) {
		if ( 32u <= (*p) && (*p) <= 37u )
			goto st242;
	} else if ( (*p) > 59u ) {
		if ( (*p) > 90u ) {
			if ( 97u <= (*p) && (*p) <= 122u )
				goto st242;
		} else if ( (*p) >= 63u )
			goto st242;
	} else
		goto st242;
	goto st0;
st243:
	if ( ++p == pe )
		goto _test_eof243;
case 243:
	switch( (*p) ) {
		case 13u: goto st244;
		case 32u: goto st244;
	}
	if ( 9u <= (*p) && (*p) <= 10u )
		goto st244;
	goto st0;
st244:
	if ( ++p == pe )
		goto _test_eof244;
case 244:
	switch( (*p) ) {
		case 13u: goto st244;
		case 32u: goto st244;
		case 34u: goto st245;
		case 39u: goto st246;
	}
	if ( 9u <= (*p) && (*p) <= 10u )
		goto st244;
	goto st0;
st245:
	if ( ++p == pe )
		goto _test_eof245;
case 245:
	if ( (*p) == 34u )
		goto st197;
	goto st245;
st246:
	if ( ++p == pe )
		goto _test_eof246;
case 246:
	if ( (*p) == 39u )
		goto st197;
	goto st246;
st247:
	if ( ++p == pe )
		goto _test_eof247;
case 247:
	switch( (*p) ) {
		case 10u: goto st247;
		case 13u: goto st247;
		case 39u: goto st243;
		case 61u: goto st247;
		case 95u: goto st247;
	}
	if ( (*p) < 40u ) {
		if ( (*p) > 33u ) {
			if ( 35u <= (*p) && (*p) <= 37u )
				goto st247;
		} else if ( (*p) >= 32u )
			goto st247;
	} else if ( (*p) > 59u ) {
		if ( (*p) > 90u ) {
			if ( 97u <= (*p) && (*p) <= 122u )
				goto st247;
		} else if ( (*p) >= 63u )
			goto st247;
	} else
		goto st247;
	goto st0;
st248:
	if ( ++p == pe )
		goto _test_eof248;
case 248:
	if ( (*p) == 89u )
		goto st249;
	goto st0;
st249:
	if ( ++p == pe )
		goto _test_eof249;
case 249:
	if ( (*p) == 83u )
		goto st250;
	goto st0;
st250:
	if ( ++p == pe )
		goto _test_eof250;
case 250:
	if ( (*p) == 84u )
		goto st251;
	goto st0;
st251:
	if ( ++p == pe )
		goto _test_eof251;
case 251:
	if ( (*p) == 69u )
		goto st252;
	goto st0;
st252:
	if ( ++p == pe )
		goto _test_eof252;
case 252:
	if ( (*p) == 77u )
		goto st243;
	goto st0;
tr247:
#line 30 "../src/xml.ragel"
	{ m_output.push(m_char); }
	goto st253;
st253:
	if ( ++p == pe )
		goto _test_eof253;
case 253:
#line 3810 "src/xml.cpp"
	switch( (*p) ) {
		case 13u: goto st254;
		case 32u: goto st254;
		case 95u: goto tr247;
	}
	if ( (*p) < 48u ) {
		if ( (*p) > 10u ) {
			if ( 45u <= (*p) && (*p) <= 46u )
				goto tr247;
		} else if ( (*p) >= 9u )
			goto st254;
	} else if ( (*p) > 58u ) {
		if ( (*p) > 90u ) {
			if ( 97u <= (*p) && (*p) <= 122u )
				goto tr247;
		} else if ( (*p) >= 65u )
			goto tr247;
	} else
		goto tr247;
	goto st0;
st254:
	if ( ++p == pe )
		goto _test_eof254;
case 254:
	switch( (*p) ) {
		case 13u: goto st254;
		case 32u: goto st254;
		case 34u: goto st217;
		case 39u: goto st226;
		case 80u: goto st255;
		case 83u: goto st277;
	}
	if ( 9u <= (*p) && (*p) <= 10u )
		goto st254;
	goto st0;
st255:
	if ( ++p == pe )
		goto _test_eof255;
case 255:
	if ( (*p) == 85u )
		goto st256;
	goto st0;
st256:
	if ( ++p == pe )
		goto _test_eof256;
case 256:
	if ( (*p) == 66u )
		goto st257;
	goto st0;
st257:
	if ( ++p == pe )
		goto _test_eof257;
case 257:
	if ( (*p) == 76u )
		goto st258;
	goto st0;
st258:
	if ( ++p == pe )
		goto _test_eof258;
case 258:
	if ( (*p) == 73u )
		goto st259;
	goto st0;
st259:
	if ( ++p == pe )
		goto _test_eof259;
case 259:
	if ( (*p) == 67u )
		goto st260;
	goto st0;
st260:
	if ( ++p == pe )
		goto _test_eof260;
case 260:
	switch( (*p) ) {
		case 13u: goto st261;
		case 32u: goto st261;
	}
	if ( 9u <= (*p) && (*p) <= 10u )
		goto st261;
	goto st0;
st261:
	if ( ++p == pe )
		goto _test_eof261;
case 261:
	switch( (*p) ) {
		case 13u: goto st261;
		case 32u: goto st261;
		case 34u: goto st262;
		case 39u: goto st276;
	}
	if ( 9u <= (*p) && (*p) <= 10u )
		goto st261;
	goto st0;
st262:
	if ( ++p == pe )
		goto _test_eof262;
case 262:
	switch( (*p) ) {
		case 10u: goto st262;
		case 13u: goto st262;
		case 34u: goto st263;
		case 61u: goto st262;
		case 95u: goto st262;
	}
	if ( (*p) < 39u ) {
		if ( 32u <= (*p) && (*p) <= 37u )
			goto st262;
	} else if ( (*p) > 59u ) {
		if ( (*p) > 90u ) {
			if ( 97u <= (*p) && (*p) <= 122u )
				goto st262;
		} else if ( (*p) >= 63u )
			goto st262;
	} else
		goto st262;
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
		case 34u: goto st265;
		case 39u: goto st275;
	}
	if ( 9u <= (*p) && (*p) <= 10u )
		goto st264;
	goto st0;
st265:
	if ( ++p == pe )
		goto _test_eof265;
case 265:
	if ( (*p) == 34u )
		goto st266;
	goto st265;
st266:
	if ( ++p == pe )
		goto _test_eof266;
case 266:
	switch( (*p) ) {
		case 13u: goto st267;
		case 32u: goto st267;
		case 62u: goto st84;
	}
	if ( 9u <= (*p) && (*p) <= 10u )
		goto st267;
	goto st0;
st267:
	if ( ++p == pe )
		goto _test_eof267;
case 267:
	switch( (*p) ) {
		case 13u: goto st267;
		case 32u: goto st267;
		case 62u: goto st84;
		case 78u: goto st268;
	}
	if ( 9u <= (*p) && (*p) <= 10u )
		goto st267;
	goto st0;
st268:
	if ( ++p == pe )
		goto _test_eof268;
case 268:
	if ( (*p) == 68u )
		goto st269;
	goto st0;
st269:
	if ( ++p == pe )
		goto _test_eof269;
case 269:
	if ( (*p) == 65u )
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
	if ( (*p) == 65u )
		goto st272;
	goto st0;
st272:
	if ( ++p == pe )
		goto _test_eof272;
case 272:
	switch( (*p) ) {
		case 13u: goto st273;
		case 32u: goto st273;
	}
	if ( 9u <= (*p) && (*p) <= 10u )
		goto st273;
	goto st0;
st273:
	if ( ++p == pe )
		goto _test_eof273;
case 273:
	switch( (*p) ) {
		case 13u: goto st273;
		case 32u: goto st273;
		case 58u: goto tr316;
		case 95u: goto tr316;
	}
	if ( (*p) < 65u ) {
		if ( 9u <= (*p) && (*p) <= 10u )
			goto st273;
	} else if ( (*p) > 90u ) {
		if ( 97u <= (*p) && (*p) <= 122u )
			goto tr316;
	} else
		goto tr316;
	goto st0;
tr316:
#line 30 "../src/xml.ragel"
	{ m_output.push(m_char); }
	goto st274;
st274:
	if ( ++p == pe )
		goto _test_eof274;
case 274:
#line 4050 "src/xml.cpp"
	switch( (*p) ) {
		case 13u: goto st197;
		case 32u: goto st197;
		case 62u: goto st84;
		case 95u: goto tr316;
	}
	if ( (*p) < 48u ) {
		if ( (*p) > 10u ) {
			if ( 45u <= (*p) && (*p) <= 46u )
				goto tr316;
		} else if ( (*p) >= 9u )
			goto st197;
	} else if ( (*p) > 58u ) {
		if ( (*p) > 90u ) {
			if ( 97u <= (*p) && (*p) <= 122u )
				goto tr316;
		} else if ( (*p) >= 65u )
			goto tr316;
	} else
		goto tr316;
	goto st0;
st275:
	if ( ++p == pe )
		goto _test_eof275;
case 275:
	if ( (*p) == 39u )
		goto st266;
	goto st275;
st276:
	if ( ++p == pe )
		goto _test_eof276;
case 276:
	switch( (*p) ) {
		case 10u: goto st276;
		case 13u: goto st276;
		case 39u: goto st263;
		case 61u: goto st276;
		case 95u: goto st276;
	}
	if ( (*p) < 40u ) {
		if ( (*p) > 33u ) {
			if ( 35u <= (*p) && (*p) <= 37u )
				goto st276;
		} else if ( (*p) >= 32u )
			goto st276;
	} else if ( (*p) > 59u ) {
		if ( (*p) > 90u ) {
			if ( 97u <= (*p) && (*p) <= 122u )
				goto st276;
		} else if ( (*p) >= 63u )
			goto st276;
	} else
		goto st276;
	goto st0;
st277:
	if ( ++p == pe )
		goto _test_eof277;
case 277:
	if ( (*p) == 89u )
		goto st278;
	goto st0;
st278:
	if ( ++p == pe )
		goto _test_eof278;
case 278:
	if ( (*p) == 83u )
		goto st279;
	goto st0;
st279:
	if ( ++p == pe )
		goto _test_eof279;
case 279:
	if ( (*p) == 84u )
		goto st280;
	goto st0;
st280:
	if ( ++p == pe )
		goto _test_eof280;
case 280:
	if ( (*p) == 69u )
		goto st281;
	goto st0;
st281:
	if ( ++p == pe )
		goto _test_eof281;
case 281:
	if ( (*p) == 77u )
		goto st263;
	goto st0;
st282:
	if ( ++p == pe )
		goto _test_eof282;
case 282:
	if ( (*p) == 79u )
		goto st283;
	goto st0;
st283:
	if ( ++p == pe )
		goto _test_eof283;
case 283:
	if ( (*p) == 84u )
		goto st284;
	goto st0;
st284:
	if ( ++p == pe )
		goto _test_eof284;
case 284:
	if ( (*p) == 65u )
		goto st285;
	goto st0;
st285:
	if ( ++p == pe )
		goto _test_eof285;
case 285:
	if ( (*p) == 84u )
		goto st286;
	goto st0;
st286:
	if ( ++p == pe )
		goto _test_eof286;
case 286:
	if ( (*p) == 73u )
		goto st287;
	goto st0;
st287:
	if ( ++p == pe )
		goto _test_eof287;
case 287:
	if ( (*p) == 79u )
		goto st288;
	goto st0;
st288:
	if ( ++p == pe )
		goto _test_eof288;
case 288:
	if ( (*p) == 78u )
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
		case 58u: goto tr329;
		case 95u: goto tr329;
	}
	if ( (*p) < 65u ) {
		if ( 9u <= (*p) && (*p) <= 10u )
			goto st290;
	} else if ( (*p) > 90u ) {
		if ( 97u <= (*p) && (*p) <= 122u )
			goto tr329;
	} else
		goto tr329;
	goto st0;
tr329:
#line 30 "../src/xml.ragel"
	{ m_output.push(m_char); }
	goto st291;
st291:
	if ( ++p == pe )
		goto _test_eof291;
case 291:
#line 4227 "src/xml.cpp"
	switch( (*p) ) {
		case 13u: goto st292;
		case 32u: goto st292;
		case 95u: goto tr329;
	}
	if ( (*p) < 48u ) {
		if ( (*p) > 10u ) {
			if ( 45u <= (*p) && (*p) <= 46u )
				goto tr329;
		} else if ( (*p) >= 9u )
			goto st292;
	} else if ( (*p) > 58u ) {
		if ( (*p) > 90u ) {
			if ( 97u <= (*p) && (*p) <= 122u )
				goto tr329;
		} else if ( (*p) >= 65u )
			goto tr329;
	} else
		goto tr329;
	goto st0;
st292:
	if ( ++p == pe )
		goto _test_eof292;
case 292:
	switch( (*p) ) {
		case 13u: goto st292;
		case 32u: goto st292;
		case 80u: goto st293;
		case 83u: goto st248;
	}
	if ( 9u <= (*p) && (*p) <= 10u )
		goto st292;
	goto st0;
st293:
	if ( ++p == pe )
		goto _test_eof293;
case 293:
	if ( (*p) == 85u )
		goto st294;
	goto st0;
st294:
	if ( ++p == pe )
		goto _test_eof294;
case 294:
	if ( (*p) == 66u )
		goto st295;
	goto st0;
st295:
	if ( ++p == pe )
		goto _test_eof295;
case 295:
	if ( (*p) == 76u )
		goto st296;
	goto st0;
st296:
	if ( ++p == pe )
		goto _test_eof296;
case 296:
	if ( (*p) == 73u )
		goto st297;
	goto st0;
st297:
	if ( ++p == pe )
		goto _test_eof297;
case 297:
	if ( (*p) == 67u )
		goto st298;
	goto st0;
st298:
	if ( ++p == pe )
		goto _test_eof298;
case 298:
	switch( (*p) ) {
		case 13u: goto st299;
		case 32u: goto st299;
	}
	if ( 9u <= (*p) && (*p) <= 10u )
		goto st299;
	goto st0;
st299:
	if ( ++p == pe )
		goto _test_eof299;
case 299:
	switch( (*p) ) {
		case 13u: goto st299;
		case 32u: goto st299;
		case 34u: goto st300;
		case 39u: goto st303;
	}
	if ( 9u <= (*p) && (*p) <= 10u )
		goto st299;
	goto st0;
st300:
	if ( ++p == pe )
		goto _test_eof300;
case 300:
	switch( (*p) ) {
		case 10u: goto st300;
		case 13u: goto st300;
		case 34u: goto st301;
		case 61u: goto st300;
		case 95u: goto st300;
	}
	if ( (*p) < 39u ) {
		if ( 32u <= (*p) && (*p) <= 37u )
			goto st300;
	} else if ( (*p) > 59u ) {
		if ( (*p) > 90u ) {
			if ( 97u <= (*p) && (*p) <= 122u )
				goto st300;
		} else if ( (*p) >= 63u )
			goto st300;
	} else
		goto st300;
	goto st0;
st301:
	if ( ++p == pe )
		goto _test_eof301;
case 301:
	switch( (*p) ) {
		case 13u: goto st302;
		case 32u: goto st302;
		case 62u: goto st84;
	}
	if ( 9u <= (*p) && (*p) <= 10u )
		goto st302;
	goto st0;
st302:
	if ( ++p == pe )
		goto _test_eof302;
case 302:
	switch( (*p) ) {
		case 13u: goto st302;
		case 32u: goto st302;
		case 34u: goto st245;
		case 39u: goto st246;
		case 62u: goto st84;
	}
	if ( 9u <= (*p) && (*p) <= 10u )
		goto st302;
	goto st0;
st303:
	if ( ++p == pe )
		goto _test_eof303;
case 303:
	switch( (*p) ) {
		case 10u: goto st303;
		case 13u: goto st303;
		case 39u: goto st301;
		case 61u: goto st303;
		case 95u: goto st303;
	}
	if ( (*p) < 40u ) {
		if ( (*p) > 33u ) {
			if ( 35u <= (*p) && (*p) <= 37u )
				goto st303;
		} else if ( (*p) >= 32u )
			goto st303;
	} else if ( (*p) > 59u ) {
		if ( (*p) > 90u ) {
			if ( 97u <= (*p) && (*p) <= 122u )
				goto st303;
		} else if ( (*p) >= 63u )
			goto st303;
	} else
		goto st303;
	goto st0;
st304:
	if ( ++p == pe )
		goto _test_eof304;
case 304:
	switch( (*p) ) {
		case 58u: goto tr342;
		case 88u: goto tr343;
		case 95u: goto tr342;
		case 120u: goto tr343;
	}
	if ( (*p) > 90u ) {
		if ( 97u <= (*p) && (*p) <= 122u )
			goto tr342;
	} else if ( (*p) >= 65u )
		goto tr342;
	goto st0;
tr342:
#line 30 "../src/xml.ragel"
	{ m_output.push(m_char); }
	goto st305;
st305:
	if ( ++p == pe )
		goto _test_eof305;
case 305:
#line 4419 "src/xml.cpp"
	switch( (*p) ) {
		case 13u: goto tr344;
		case 32u: goto tr344;
		case 63u: goto st308;
		case 95u: goto tr342;
	}
	if ( (*p) < 48u ) {
		if ( (*p) > 10u ) {
			if ( 45u <= (*p) && (*p) <= 46u )
				goto tr342;
		} else if ( (*p) >= 9u )
			goto tr344;
	} else if ( (*p) > 58u ) {
		if ( (*p) > 90u ) {
			if ( 97u <= (*p) && (*p) <= 122u )
				goto tr342;
		} else if ( (*p) >= 65u )
			goto tr342;
	} else
		goto tr342;
	goto st0;
tr344:
#line 30 "../src/xml.ragel"
	{ m_output.push(m_char); }
	goto st306;
st306:
	if ( ++p == pe )
		goto _test_eof306;
case 306:
#line 4449 "src/xml.cpp"
	if ( (*p) == 63u )
		goto tr346;
	goto tr344;
tr346:
#line 30 "../src/xml.ragel"
	{ m_output.push(m_char); }
	goto st307;
st307:
	if ( ++p == pe )
		goto _test_eof307;
case 307:
#line 4461 "src/xml.cpp"
	switch( (*p) ) {
		case 62u: goto tr347;
		case 63u: goto tr346;
	}
	goto tr344;
st308:
	if ( ++p == pe )
		goto _test_eof308;
case 308:
	if ( (*p) == 62u )
		goto tr347;
	goto st0;
tr343:
#line 30 "../src/xml.ragel"
	{ m_output.push(m_char); }
	goto st309;
st309:
	if ( ++p == pe )
		goto _test_eof309;
case 309:
#line 4482 "src/xml.cpp"
	switch( (*p) ) {
		case 13u: goto tr344;
		case 32u: goto tr344;
		case 63u: goto st308;
		case 77u: goto tr348;
		case 95u: goto tr342;
		case 109u: goto tr348;
	}
	if ( (*p) < 48u ) {
		if ( (*p) > 10u ) {
			if ( 45u <= (*p) && (*p) <= 46u )
				goto tr342;
		} else if ( (*p) >= 9u )
			goto tr344;
	} else if ( (*p) > 58u ) {
		if ( (*p) > 90u ) {
			if ( 97u <= (*p) && (*p) <= 122u )
				goto tr342;
		} else if ( (*p) >= 65u )
			goto tr342;
	} else
		goto tr342;
	goto st0;
tr348:
#line 30 "../src/xml.ragel"
	{ m_output.push(m_char); }
	goto st310;
st310:
	if ( ++p == pe )
		goto _test_eof310;
case 310:
#line 4514 "src/xml.cpp"
	switch( (*p) ) {
		case 13u: goto tr344;
		case 32u: goto tr344;
		case 63u: goto st308;
		case 76u: goto tr349;
		case 95u: goto tr342;
		case 108u: goto tr349;
	}
	if ( (*p) < 48u ) {
		if ( (*p) > 10u ) {
			if ( 45u <= (*p) && (*p) <= 46u )
				goto tr342;
		} else if ( (*p) >= 9u )
			goto tr344;
	} else if ( (*p) > 58u ) {
		if ( (*p) > 90u ) {
			if ( 97u <= (*p) && (*p) <= 122u )
				goto tr342;
		} else if ( (*p) >= 65u )
			goto tr342;
	} else
		goto tr342;
	goto st0;
tr349:
#line 30 "../src/xml.ragel"
	{ m_output.push(m_char); }
	goto st311;
st311:
	if ( ++p == pe )
		goto _test_eof311;
case 311:
#line 4546 "src/xml.cpp"
	if ( (*p) == 95u )
		goto tr342;
	if ( (*p) < 48u ) {
		if ( 45u <= (*p) && (*p) <= 46u )
			goto tr342;
	} else if ( (*p) > 58u ) {
		if ( (*p) > 90u ) {
			if ( 97u <= (*p) && (*p) <= 122u )
				goto tr342;
		} else if ( (*p) >= 65u )
			goto tr342;
	} else
		goto tr342;
	goto st0;
st312:
	if ( ++p == pe )
		goto _test_eof312;
case 312:
	switch( (*p) ) {
		case 13u: goto st312;
		case 32u: goto st312;
		case 62u: goto st20;
	}
	if ( 9u <= (*p) && (*p) <= 10u )
		goto st312;
	goto st0;
st313:
	if ( ++p == pe )
		goto _test_eof313;
case 313:
	if ( (*p) == 39u )
		goto st83;
	goto st313;
st314:
	if ( ++p == pe )
		goto _test_eof314;
case 314:
	switch( (*p) ) {
		case 10u: goto st314;
		case 13u: goto st314;
		case 39u: goto st80;
		case 61u: goto st314;
		case 95u: goto st314;
	}
	if ( (*p) < 40u ) {
		if ( (*p) > 33u ) {
			if ( 35u <= (*p) && (*p) <= 37u )
				goto st314;
		} else if ( (*p) >= 32u )
			goto st314;
	} else if ( (*p) > 59u ) {
		if ( (*p) > 90u ) {
			if ( 97u <= (*p) && (*p) <= 122u )
				goto st314;
		} else if ( (*p) >= 63u )
			goto st314;
	} else
		goto st314;
	goto st0;
st315:
	if ( ++p == pe )
		goto _test_eof315;
case 315:
	if ( (*p) == 89u )
		goto st316;
	goto st0;
st316:
	if ( ++p == pe )
		goto _test_eof316;
case 316:
	if ( (*p) == 83u )
		goto st317;
	goto st0;
st317:
	if ( ++p == pe )
		goto _test_eof317;
case 317:
	if ( (*p) == 84u )
		goto st318;
	goto st0;
st318:
	if ( ++p == pe )
		goto _test_eof318;
case 318:
	if ( (*p) == 69u )
		goto st319;
	goto st0;
st319:
	if ( ++p == pe )
		goto _test_eof319;
case 319:
	if ( (*p) == 77u )
		goto st80;
	goto st0;
st320:
	if ( ++p == pe )
		goto _test_eof320;
case 320:
	switch( (*p) ) {
		case 58u: goto tr354;
		case 88u: goto tr355;
		case 95u: goto tr354;
		case 120u: goto tr355;
	}
	if ( (*p) > 90u ) {
		if ( 97u <= (*p) && (*p) <= 122u )
			goto tr354;
	} else if ( (*p) >= 65u )
		goto tr354;
	goto st0;
tr354:
#line 30 "../src/xml.ragel"
	{ m_output.push(m_char); }
	goto st321;
st321:
	if ( ++p == pe )
		goto _test_eof321;
case 321:
#line 4665 "src/xml.cpp"
	switch( (*p) ) {
		case 13u: goto tr356;
		case 32u: goto tr356;
		case 63u: goto st324;
		case 95u: goto tr354;
	}
	if ( (*p) < 48u ) {
		if ( (*p) > 10u ) {
			if ( 45u <= (*p) && (*p) <= 46u )
				goto tr354;
		} else if ( (*p) >= 9u )
			goto tr356;
	} else if ( (*p) > 58u ) {
		if ( (*p) > 90u ) {
			if ( 97u <= (*p) && (*p) <= 122u )
				goto tr354;
		} else if ( (*p) >= 65u )
			goto tr354;
	} else
		goto tr354;
	goto st0;
tr356:
#line 30 "../src/xml.ragel"
	{ m_output.push(m_char); }
	goto st322;
st322:
	if ( ++p == pe )
		goto _test_eof322;
case 322:
#line 4695 "src/xml.cpp"
	if ( (*p) == 63u )
		goto tr358;
	goto tr356;
tr358:
#line 30 "../src/xml.ragel"
	{ m_output.push(m_char); }
	goto st323;
st323:
	if ( ++p == pe )
		goto _test_eof323;
case 323:
#line 4707 "src/xml.cpp"
	switch( (*p) ) {
		case 62u: goto tr359;
		case 63u: goto tr358;
	}
	goto tr356;
st324:
	if ( ++p == pe )
		goto _test_eof324;
case 324:
	if ( (*p) == 62u )
		goto tr359;
	goto st0;
tr355:
#line 30 "../src/xml.ragel"
	{ m_output.push(m_char); }
	goto st325;
st325:
	if ( ++p == pe )
		goto _test_eof325;
case 325:
#line 4728 "src/xml.cpp"
	switch( (*p) ) {
		case 13u: goto tr356;
		case 32u: goto tr356;
		case 63u: goto st324;
		case 77u: goto tr360;
		case 95u: goto tr354;
		case 109u: goto tr360;
	}
	if ( (*p) < 48u ) {
		if ( (*p) > 10u ) {
			if ( 45u <= (*p) && (*p) <= 46u )
				goto tr354;
		} else if ( (*p) >= 9u )
			goto tr356;
	} else if ( (*p) > 58u ) {
		if ( (*p) > 90u ) {
			if ( 97u <= (*p) && (*p) <= 122u )
				goto tr354;
		} else if ( (*p) >= 65u )
			goto tr354;
	} else
		goto tr354;
	goto st0;
tr360:
#line 30 "../src/xml.ragel"
	{ m_output.push(m_char); }
	goto st326;
st326:
	if ( ++p == pe )
		goto _test_eof326;
case 326:
#line 4760 "src/xml.cpp"
	switch( (*p) ) {
		case 13u: goto tr356;
		case 32u: goto tr356;
		case 63u: goto st324;
		case 76u: goto tr361;
		case 95u: goto tr354;
		case 108u: goto tr361;
	}
	if ( (*p) < 48u ) {
		if ( (*p) > 10u ) {
			if ( 45u <= (*p) && (*p) <= 46u )
				goto tr354;
		} else if ( (*p) >= 9u )
			goto tr356;
	} else if ( (*p) > 58u ) {
		if ( (*p) > 90u ) {
			if ( 97u <= (*p) && (*p) <= 122u )
				goto tr354;
		} else if ( (*p) >= 65u )
			goto tr354;
	} else
		goto tr354;
	goto st0;
tr361:
#line 30 "../src/xml.ragel"
	{ m_output.push(m_char); }
	goto st327;
st327:
	if ( ++p == pe )
		goto _test_eof327;
case 327:
#line 4792 "src/xml.cpp"
	if ( (*p) == 95u )
		goto tr354;
	if ( (*p) < 48u ) {
		if ( 45u <= (*p) && (*p) <= 46u )
			goto tr354;
	} else if ( (*p) > 58u ) {
		if ( (*p) > 90u ) {
			if ( 97u <= (*p) && (*p) <= 122u )
				goto tr354;
		} else if ( (*p) >= 65u )
			goto tr354;
	} else
		goto tr354;
	goto st0;
st328:
	if ( ++p == pe )
		goto _test_eof328;
case 328:
	switch( (*p) ) {
		case 33u: goto st4;
		case 58u: goto tr5;
		case 63u: goto st329;
		case 95u: goto tr5;
	}
	if ( (*p) > 90u ) {
		if ( 97u <= (*p) && (*p) <= 122u )
			goto tr5;
	} else if ( (*p) >= 65u )
		goto tr5;
	goto st0;
st329:
	if ( ++p == pe )
		goto _test_eof329;
case 329:
	switch( (*p) ) {
		case 58u: goto tr354;
		case 88u: goto tr355;
		case 95u: goto tr354;
		case 120u: goto tr363;
	}
	if ( (*p) > 90u ) {
		if ( 97u <= (*p) && (*p) <= 122u )
			goto tr354;
	} else if ( (*p) >= 65u )
		goto tr354;
	goto st0;
tr363:
#line 30 "../src/xml.ragel"
	{ m_output.push(m_char); }
	goto st330;
st330:
	if ( ++p == pe )
		goto _test_eof330;
case 330:
#line 4847 "src/xml.cpp"
	switch( (*p) ) {
		case 13u: goto tr356;
		case 32u: goto tr356;
		case 63u: goto st324;
		case 77u: goto tr360;
		case 95u: goto tr354;
		case 109u: goto tr364;
	}
	if ( (*p) < 48u ) {
		if ( (*p) > 10u ) {
			if ( 45u <= (*p) && (*p) <= 46u )
				goto tr354;
		} else if ( (*p) >= 9u )
			goto tr356;
	} else if ( (*p) > 58u ) {
		if ( (*p) > 90u ) {
			if ( 97u <= (*p) && (*p) <= 122u )
				goto tr354;
		} else if ( (*p) >= 65u )
			goto tr354;
	} else
		goto tr354;
	goto st0;
tr364:
#line 30 "../src/xml.ragel"
	{ m_output.push(m_char); }
	goto st331;
st331:
	if ( ++p == pe )
		goto _test_eof331;
case 331:
#line 4879 "src/xml.cpp"
	switch( (*p) ) {
		case 13u: goto tr356;
		case 32u: goto tr356;
		case 63u: goto st324;
		case 76u: goto tr361;
		case 95u: goto tr354;
		case 108u: goto tr365;
	}
	if ( (*p) < 48u ) {
		if ( (*p) > 10u ) {
			if ( 45u <= (*p) && (*p) <= 46u )
				goto tr354;
		} else if ( (*p) >= 9u )
			goto tr356;
	} else if ( (*p) > 58u ) {
		if ( (*p) > 90u ) {
			if ( 97u <= (*p) && (*p) <= 122u )
				goto tr354;
		} else if ( (*p) >= 65u )
			goto tr354;
	} else
		goto tr354;
	goto st0;
tr365:
#line 30 "../src/xml.ragel"
	{ m_output.push(m_char); }
	goto st332;
st332:
	if ( ++p == pe )
		goto _test_eof332;
case 332:
#line 4911 "src/xml.cpp"
	switch( (*p) ) {
		case 13u: goto st333;
		case 32u: goto st333;
		case 95u: goto tr354;
	}
	if ( (*p) < 48u ) {
		if ( (*p) > 10u ) {
			if ( 45u <= (*p) && (*p) <= 46u )
				goto tr354;
		} else if ( (*p) >= 9u )
			goto st333;
	} else if ( (*p) > 58u ) {
		if ( (*p) > 90u ) {
			if ( 97u <= (*p) && (*p) <= 122u )
				goto tr354;
		} else if ( (*p) >= 65u )
			goto tr354;
	} else
		goto tr354;
	goto st0;
st333:
	if ( ++p == pe )
		goto _test_eof333;
case 333:
	switch( (*p) ) {
		case 13u: goto st333;
		case 32u: goto st333;
		case 118u: goto st334;
	}
	if ( 9u <= (*p) && (*p) <= 10u )
		goto st333;
	goto st0;
st334:
	if ( ++p == pe )
		goto _test_eof334;
case 334:
	if ( (*p) == 101u )
		goto st335;
	goto st0;
st335:
	if ( ++p == pe )
		goto _test_eof335;
case 335:
	if ( (*p) == 114u )
		goto st336;
	goto st0;
st336:
	if ( ++p == pe )
		goto _test_eof336;
case 336:
	if ( (*p) == 115u )
		goto st337;
	goto st0;
st337:
	if ( ++p == pe )
		goto _test_eof337;
case 337:
	if ( (*p) == 105u )
		goto st338;
	goto st0;
st338:
	if ( ++p == pe )
		goto _test_eof338;
case 338:
	if ( (*p) == 111u )
		goto st339;
	goto st0;
st339:
	if ( ++p == pe )
		goto _test_eof339;
case 339:
	if ( (*p) == 110u )
		goto st340;
	goto st0;
st340:
	if ( ++p == pe )
		goto _test_eof340;
case 340:
	switch( (*p) ) {
		case 13u: goto st340;
		case 32u: goto st340;
		case 61u: goto st341;
	}
	if ( 9u <= (*p) && (*p) <= 10u )
		goto st340;
	goto st0;
st341:
	if ( ++p == pe )
		goto _test_eof341;
case 341:
	switch( (*p) ) {
		case 13u: goto st341;
		case 32u: goto st341;
		case 34u: goto st342;
		case 39u: goto st385;
	}
	if ( 9u <= (*p) && (*p) <= 10u )
		goto st341;
	goto st0;
st342:
	if ( ++p == pe )
		goto _test_eof342;
case 342:
	if ( (*p) == 49u )
		goto st343;
	goto st0;
st343:
	if ( ++p == pe )
		goto _test_eof343;
case 343:
	if ( (*p) == 46u )
		goto st344;
	goto st0;
st344:
	if ( ++p == pe )
		goto _test_eof344;
case 344:
	if ( (*p) == 48u )
		goto st345;
	goto st0;
st345:
	if ( ++p == pe )
		goto _test_eof345;
case 345:
	if ( (*p) == 34u )
		goto st346;
	goto st0;
st346:
	if ( ++p == pe )
		goto _test_eof346;
case 346:
	switch( (*p) ) {
		case 13u: goto st347;
		case 32u: goto st347;
		case 63u: goto st9;
	}
	if ( 9u <= (*p) && (*p) <= 10u )
		goto st347;
	goto st0;
st347:
	if ( ++p == pe )
		goto _test_eof347;
case 347:
	switch( (*p) ) {
		case 13u: goto st347;
		case 32u: goto st347;
		case 63u: goto st9;
		case 101u: goto st348;
		case 115u: goto st361;
	}
	if ( 9u <= (*p) && (*p) <= 10u )
		goto st347;
	goto st0;
st348:
	if ( ++p == pe )
		goto _test_eof348;
case 348:
	if ( (*p) == 110u )
		goto st349;
	goto st0;
st349:
	if ( ++p == pe )
		goto _test_eof349;
case 349:
	if ( (*p) == 99u )
		goto st350;
	goto st0;
st350:
	if ( ++p == pe )
		goto _test_eof350;
case 350:
	if ( (*p) == 111u )
		goto st351;
	goto st0;
st351:
	if ( ++p == pe )
		goto _test_eof351;
case 351:
	if ( (*p) == 100u )
		goto st352;
	goto st0;
st352:
	if ( ++p == pe )
		goto _test_eof352;
case 352:
	if ( (*p) == 105u )
		goto st353;
	goto st0;
st353:
	if ( ++p == pe )
		goto _test_eof353;
case 353:
	if ( (*p) == 110u )
		goto st354;
	goto st0;
st354:
	if ( ++p == pe )
		goto _test_eof354;
case 354:
	if ( (*p) == 103u )
		goto st355;
	goto st0;
st355:
	if ( ++p == pe )
		goto _test_eof355;
case 355:
	switch( (*p) ) {
		case 13u: goto st355;
		case 32u: goto st355;
		case 61u: goto st356;
	}
	if ( 9u <= (*p) && (*p) <= 10u )
		goto st355;
	goto st0;
st356:
	if ( ++p == pe )
		goto _test_eof356;
case 356:
	switch( (*p) ) {
		case 13u: goto st356;
		case 32u: goto st356;
		case 34u: goto st357;
		case 39u: goto st383;
	}
	if ( 9u <= (*p) && (*p) <= 10u )
		goto st356;
	goto st0;
st357:
	if ( ++p == pe )
		goto _test_eof357;
case 357:
	if ( (*p) > 90u ) {
		if ( 97u <= (*p) && (*p) <= 122u )
			goto st358;
	} else if ( (*p) >= 65u )
		goto st358;
	goto st0;
st358:
	if ( ++p == pe )
		goto _test_eof358;
case 358:
	switch( (*p) ) {
		case 34u: goto st359;
		case 95u: goto st358;
	}
	if ( (*p) < 48u ) {
		if ( 45u <= (*p) && (*p) <= 46u )
			goto st358;
	} else if ( (*p) > 57u ) {
		if ( (*p) > 90u ) {
			if ( 97u <= (*p) && (*p) <= 122u )
				goto st358;
		} else if ( (*p) >= 65u )
			goto st358;
	} else
		goto st358;
	goto st0;
st359:
	if ( ++p == pe )
		goto _test_eof359;
case 359:
	switch( (*p) ) {
		case 13u: goto st360;
		case 32u: goto st360;
		case 63u: goto st9;
	}
	if ( 9u <= (*p) && (*p) <= 10u )
		goto st360;
	goto st0;
st360:
	if ( ++p == pe )
		goto _test_eof360;
case 360:
	switch( (*p) ) {
		case 13u: goto st360;
		case 32u: goto st360;
		case 63u: goto st9;
		case 115u: goto st361;
	}
	if ( 9u <= (*p) && (*p) <= 10u )
		goto st360;
	goto st0;
st361:
	if ( ++p == pe )
		goto _test_eof361;
case 361:
	if ( (*p) == 116u )
		goto st362;
	goto st0;
st362:
	if ( ++p == pe )
		goto _test_eof362;
case 362:
	if ( (*p) == 97u )
		goto st363;
	goto st0;
st363:
	if ( ++p == pe )
		goto _test_eof363;
case 363:
	if ( (*p) == 110u )
		goto st364;
	goto st0;
st364:
	if ( ++p == pe )
		goto _test_eof364;
case 364:
	if ( (*p) == 100u )
		goto st365;
	goto st0;
st365:
	if ( ++p == pe )
		goto _test_eof365;
case 365:
	if ( (*p) == 97u )
		goto st366;
	goto st0;
st366:
	if ( ++p == pe )
		goto _test_eof366;
case 366:
	if ( (*p) == 108u )
		goto st367;
	goto st0;
st367:
	if ( ++p == pe )
		goto _test_eof367;
case 367:
	if ( (*p) == 111u )
		goto st368;
	goto st0;
st368:
	if ( ++p == pe )
		goto _test_eof368;
case 368:
	if ( (*p) == 110u )
		goto st369;
	goto st0;
st369:
	if ( ++p == pe )
		goto _test_eof369;
case 369:
	if ( (*p) == 101u )
		goto st370;
	goto st0;
st370:
	if ( ++p == pe )
		goto _test_eof370;
case 370:
	switch( (*p) ) {
		case 13u: goto st370;
		case 32u: goto st370;
		case 61u: goto st371;
	}
	if ( 9u <= (*p) && (*p) <= 10u )
		goto st370;
	goto st0;
st371:
	if ( ++p == pe )
		goto _test_eof371;
case 371:
	switch( (*p) ) {
		case 13u: goto st371;
		case 32u: goto st371;
		case 34u: goto st372;
		case 39u: goto st378;
	}
	if ( 9u <= (*p) && (*p) <= 10u )
		goto st371;
	goto st0;
st372:
	if ( ++p == pe )
		goto _test_eof372;
case 372:
	switch( (*p) ) {
		case 110u: goto st373;
		case 121u: goto st376;
	}
	goto st0;
st373:
	if ( ++p == pe )
		goto _test_eof373;
case 373:
	if ( (*p) == 111u )
		goto st374;
	goto st0;
st374:
	if ( ++p == pe )
		goto _test_eof374;
case 374:
	if ( (*p) == 34u )
		goto st375;
	goto st0;
st375:
	if ( ++p == pe )
		goto _test_eof375;
case 375:
	switch( (*p) ) {
		case 13u: goto st375;
		case 32u: goto st375;
		case 63u: goto st9;
	}
	if ( 9u <= (*p) && (*p) <= 10u )
		goto st375;
	goto st0;
st376:
	if ( ++p == pe )
		goto _test_eof376;
case 376:
	if ( (*p) == 101u )
		goto st377;
	goto st0;
st377:
	if ( ++p == pe )
		goto _test_eof377;
case 377:
	if ( (*p) == 115u )
		goto st374;
	goto st0;
st378:
	if ( ++p == pe )
		goto _test_eof378;
case 378:
	switch( (*p) ) {
		case 110u: goto st379;
		case 121u: goto st381;
	}
	goto st0;
st379:
	if ( ++p == pe )
		goto _test_eof379;
case 379:
	if ( (*p) == 111u )
		goto st380;
	goto st0;
st380:
	if ( ++p == pe )
		goto _test_eof380;
case 380:
	if ( (*p) == 39u )
		goto st375;
	goto st0;
st381:
	if ( ++p == pe )
		goto _test_eof381;
case 381:
	if ( (*p) == 101u )
		goto st382;
	goto st0;
st382:
	if ( ++p == pe )
		goto _test_eof382;
case 382:
	if ( (*p) == 115u )
		goto st380;
	goto st0;
st383:
	if ( ++p == pe )
		goto _test_eof383;
case 383:
	if ( (*p) > 90u ) {
		if ( 97u <= (*p) && (*p) <= 122u )
			goto st384;
	} else if ( (*p) >= 65u )
		goto st384;
	goto st0;
st384:
	if ( ++p == pe )
		goto _test_eof384;
case 384:
	switch( (*p) ) {
		case 39u: goto st359;
		case 95u: goto st384;
	}
	if ( (*p) < 48u ) {
		if ( 45u <= (*p) && (*p) <= 46u )
			goto st384;
	} else if ( (*p) > 57u ) {
		if ( (*p) > 90u ) {
			if ( 97u <= (*p) && (*p) <= 122u )
				goto st384;
		} else if ( (*p) >= 65u )
			goto st384;
	} else
		goto st384;
	goto st0;
st385:
	if ( ++p == pe )
		goto _test_eof385;
case 385:
	if ( (*p) == 49u )
		goto st386;
	goto st0;
st386:
	if ( ++p == pe )
		goto _test_eof386;
case 386:
	if ( (*p) == 46u )
		goto st387;
	goto st0;
st387:
	if ( ++p == pe )
		goto _test_eof387;
case 387:
	if ( (*p) == 48u )
		goto st388;
	goto st0;
st388:
	if ( ++p == pe )
		goto _test_eof388;
case 388:
	if ( (*p) == 39u )
		goto st346;
	goto st0;
st389:
	if ( ++p == pe )
		goto _test_eof389;
case 389:
	switch( (*p) ) {
		case 13u: goto st389;
		case 32u: goto st389;
		case 40u: goto tr423;
		case 58u: goto tr424;
		case 95u: goto tr424;
	}
	if ( (*p) < 65u ) {
		if ( 9u <= (*p) && (*p) <= 10u )
			goto st389;
	} else if ( (*p) > 90u ) {
		if ( 97u <= (*p) && (*p) <= 122u )
			goto tr424;
	} else
		goto tr424;
	goto st0;
tr423:
#line 87 "../src/xml.ragel"
	{ { pre_push(); { this->m_stack[ this->m_top++] = 390; goto st389;}} }
	goto st390;
st390:
	if ( ++p == pe )
		goto _test_eof390;
case 390:
#line 5454 "src/xml.cpp"
	switch( (*p) ) {
		case 13u: goto st391;
		case 32u: goto st391;
		case 41u: goto tr426;
		case 44u: goto st392;
		case 63u: goto st391;
		case 124u: goto st396;
	}
	if ( (*p) > 10u ) {
		if ( 42u <= (*p) && (*p) <= 43u )
			goto st391;
	} else if ( (*p) >= 9u )
		goto st391;
	goto st0;
st391:
	if ( ++p == pe )
		goto _test_eof391;
case 391:
	switch( (*p) ) {
		case 13u: goto st391;
		case 32u: goto st391;
		case 41u: goto tr426;
		case 44u: goto st392;
		case 124u: goto st396;
	}
	if ( 9u <= (*p) && (*p) <= 10u )
		goto st391;
	goto st0;
tr426:
#line 29 "../src/xml.ragel"
	{ { this->m_cs =  this->m_stack[-- this->m_top];goto _again;} }
	goto st459;
st459:
	if ( ++p == pe )
		goto _test_eof459;
case 459:
#line 5491 "src/xml.cpp"
	goto st0;
st392:
	if ( ++p == pe )
		goto _test_eof392;
case 392:
	switch( (*p) ) {
		case 13u: goto st392;
		case 32u: goto st392;
		case 40u: goto tr429;
		case 58u: goto tr430;
		case 95u: goto tr430;
	}
	if ( (*p) < 65u ) {
		if ( 9u <= (*p) && (*p) <= 10u )
			goto st392;
	} else if ( (*p) > 90u ) {
		if ( 97u <= (*p) && (*p) <= 122u )
			goto tr430;
	} else
		goto tr430;
	goto st0;
tr429:
#line 87 "../src/xml.ragel"
	{ { pre_push(); { this->m_stack[ this->m_top++] = 393; goto st389;}} }
	goto st393;
st393:
	if ( ++p == pe )
		goto _test_eof393;
case 393:
#line 5521 "src/xml.cpp"
	switch( (*p) ) {
		case 13u: goto st394;
		case 32u: goto st394;
		case 41u: goto tr426;
		case 44u: goto st392;
		case 63u: goto st394;
	}
	if ( (*p) > 10u ) {
		if ( 42u <= (*p) && (*p) <= 43u )
			goto st394;
	} else if ( (*p) >= 9u )
		goto st394;
	goto st0;
st394:
	if ( ++p == pe )
		goto _test_eof394;
case 394:
	switch( (*p) ) {
		case 13u: goto st394;
		case 32u: goto st394;
		case 41u: goto tr426;
		case 44u: goto st392;
	}
	if ( 9u <= (*p) && (*p) <= 10u )
		goto st394;
	goto st0;
tr430:
#line 30 "../src/xml.ragel"
	{ m_output.push(m_char); }
	goto st395;
st395:
	if ( ++p == pe )
		goto _test_eof395;
case 395:
#line 5556 "src/xml.cpp"
	switch( (*p) ) {
		case 13u: goto st394;
		case 32u: goto st394;
		case 41u: goto tr426;
		case 44u: goto st392;
		case 63u: goto st394;
		case 95u: goto tr430;
	}
	if ( (*p) < 45u ) {
		if ( (*p) > 10u ) {
			if ( 42u <= (*p) && (*p) <= 43u )
				goto st394;
		} else if ( (*p) >= 9u )
			goto st394;
	} else if ( (*p) > 46u ) {
		if ( (*p) < 65u ) {
			if ( 48u <= (*p) && (*p) <= 58u )
				goto tr430;
		} else if ( (*p) > 90u ) {
			if ( 97u <= (*p) && (*p) <= 122u )
				goto tr430;
		} else
			goto tr430;
	} else
		goto tr430;
	goto st0;
st396:
	if ( ++p == pe )
		goto _test_eof396;
case 396:
	switch( (*p) ) {
		case 13u: goto st396;
		case 32u: goto st396;
		case 40u: goto tr432;
		case 58u: goto tr433;
		case 95u: goto tr433;
	}
	if ( (*p) < 65u ) {
		if ( 9u <= (*p) && (*p) <= 10u )
			goto st396;
	} else if ( (*p) > 90u ) {
		if ( 97u <= (*p) && (*p) <= 122u )
			goto tr433;
	} else
		goto tr433;
	goto st0;
tr432:
#line 87 "../src/xml.ragel"
	{ { pre_push(); { this->m_stack[ this->m_top++] = 397; goto st389;}} }
	goto st397;
st397:
	if ( ++p == pe )
		goto _test_eof397;
case 397:
#line 5611 "src/xml.cpp"
	switch( (*p) ) {
		case 13u: goto st398;
		case 32u: goto st398;
		case 41u: goto tr426;
		case 63u: goto st398;
		case 124u: goto st396;
	}
	if ( (*p) > 10u ) {
		if ( 42u <= (*p) && (*p) <= 43u )
			goto st398;
	} else if ( (*p) >= 9u )
		goto st398;
	goto st0;
st398:
	if ( ++p == pe )
		goto _test_eof398;
case 398:
	switch( (*p) ) {
		case 13u: goto st398;
		case 32u: goto st398;
		case 41u: goto tr426;
		case 124u: goto st396;
	}
	if ( 9u <= (*p) && (*p) <= 10u )
		goto st398;
	goto st0;
tr433:
#line 30 "../src/xml.ragel"
	{ m_output.push(m_char); }
	goto st399;
st399:
	if ( ++p == pe )
		goto _test_eof399;
case 399:
#line 5646 "src/xml.cpp"
	switch( (*p) ) {
		case 13u: goto st398;
		case 32u: goto st398;
		case 41u: goto tr426;
		case 63u: goto st398;
		case 95u: goto tr433;
		case 124u: goto st396;
	}
	if ( (*p) < 45u ) {
		if ( (*p) > 10u ) {
			if ( 42u <= (*p) && (*p) <= 43u )
				goto st398;
		} else if ( (*p) >= 9u )
			goto st398;
	} else if ( (*p) > 46u ) {
		if ( (*p) < 65u ) {
			if ( 48u <= (*p) && (*p) <= 58u )
				goto tr433;
		} else if ( (*p) > 90u ) {
			if ( 97u <= (*p) && (*p) <= 122u )
				goto tr433;
		} else
			goto tr433;
	} else
		goto tr433;
	goto st0;
tr424:
#line 30 "../src/xml.ragel"
	{ m_output.push(m_char); }
	goto st400;
st400:
	if ( ++p == pe )
		goto _test_eof400;
case 400:
#line 5681 "src/xml.cpp"
	switch( (*p) ) {
		case 13u: goto st391;
		case 32u: goto st391;
		case 41u: goto tr426;
		case 44u: goto st392;
		case 63u: goto st391;
		case 95u: goto tr424;
		case 124u: goto st396;
	}
	if ( (*p) < 45u ) {
		if ( (*p) > 10u ) {
			if ( 42u <= (*p) && (*p) <= 43u )
				goto st391;
		} else if ( (*p) >= 9u )
			goto st391;
	} else if ( (*p) > 46u ) {
		if ( (*p) < 65u ) {
			if ( 48u <= (*p) && (*p) <= 58u )
				goto tr424;
		} else if ( (*p) > 90u ) {
			if ( 97u <= (*p) && (*p) <= 122u )
				goto tr424;
		} else
			goto tr424;
	} else
		goto tr424;
	goto st0;
tr435:
#line 30 "../src/xml.ragel"
	{ m_output.push(m_char); }
	goto st401;
tr473:
#line 101 "../src/xml.ragel"
	{ token("element_name"); }
#line 100 "../src/xml.ragel"
	{ { pre_push(); { this->m_stack[ this->m_top++] = 401; goto st401;}} }
	goto st401;
tr477:
#line 100 "../src/xml.ragel"
	{ { pre_push(); { this->m_stack[ this->m_top++] = 401; goto st401;}} }
	goto st401;
tr478:
#line 102 "../src/xml.ragel"
	{token("empty element");}
	goto st401;
tr511:
#line 48 "../src/xml.ragel"
	{token("pi",-1);}
	goto st401;
tr443:
#line 63 "../src/xml.ragel"
	{subst_char();}
	goto st401;
tr445:
#line 63 "../src/xml.ragel"
	{subst_hex();}
	goto st401;
tr446:
#line 62 "../src/xml.ragel"
	{subst_entity();}
	goto st401;
st401:
	if ( ++p == pe )
		goto _test_eof401;
case 401:
#line 5747 "src/xml.cpp"
	switch( (*p) ) {
		case 38u: goto tr436;
		case 60u: goto tr437;
		case 93u: goto tr438;
	}
	goto tr435;
tr436:
#line 81 "../src/xml.ragel"
	{token("text");}
	goto st402;
st402:
	if ( ++p == pe )
		goto _test_eof402;
case 402:
#line 5762 "src/xml.cpp"
	switch( (*p) ) {
		case 35u: goto st403;
		case 58u: goto tr440;
		case 95u: goto tr440;
	}
	if ( (*p) > 90u ) {
		if ( 97u <= (*p) && (*p) <= 122u )
			goto tr440;
	} else if ( (*p) >= 65u )
		goto tr440;
	goto st0;
st403:
	if ( ++p == pe )
		goto _test_eof403;
case 403:
	if ( (*p) == 120u )
		goto st405;
	if ( 48u <= (*p) && (*p) <= 57u )
		goto tr441;
	goto st0;
tr441:
#line 31 "../src/xml.ragel"
	{ m_entity.push(m_char); }
	goto st404;
st404:
	if ( ++p == pe )
		goto _test_eof404;
case 404:
#line 5791 "src/xml.cpp"
	if ( (*p) == 59u )
		goto tr443;
	if ( 48u <= (*p) && (*p) <= 57u )
		goto tr441;
	goto st0;
st405:
	if ( ++p == pe )
		goto _test_eof405;
case 405:
	if ( (*p) < 65u ) {
		if ( 48u <= (*p) && (*p) <= 57u )
			goto tr444;
	} else if ( (*p) > 70u ) {
		if ( 97u <= (*p) && (*p) <= 102u )
			goto tr444;
	} else
		goto tr444;
	goto st0;
tr444:
#line 31 "../src/xml.ragel"
	{ m_entity.push(m_char); }
	goto st406;
st406:
	if ( ++p == pe )
		goto _test_eof406;
case 406:
#line 5818 "src/xml.cpp"
	if ( (*p) == 59u )
		goto tr445;
	if ( (*p) < 65u ) {
		if ( 48u <= (*p) && (*p) <= 57u )
			goto tr444;
	} else if ( (*p) > 70u ) {
		if ( 97u <= (*p) && (*p) <= 102u )
			goto tr444;
	} else
		goto tr444;
	goto st0;
tr440:
#line 31 "../src/xml.ragel"
	{ m_entity.push(m_char); }
	goto st407;
st407:
	if ( ++p == pe )
		goto _test_eof407;
case 407:
#line 5838 "src/xml.cpp"
	switch( (*p) ) {
		case 59u: goto tr446;
		case 95u: goto tr440;
	}
	if ( (*p) < 48u ) {
		if ( 45u <= (*p) && (*p) <= 46u )
			goto tr440;
	} else if ( (*p) > 58u ) {
		if ( (*p) > 90u ) {
			if ( 97u <= (*p) && (*p) <= 122u )
				goto tr440;
		} else if ( (*p) >= 65u )
			goto tr440;
	} else
		goto tr440;
	goto st0;
tr437:
#line 81 "../src/xml.ragel"
	{token("text");}
	goto st408;
st408:
	if ( ++p == pe )
		goto _test_eof408;
case 408:
#line 5863 "src/xml.cpp"
	switch( (*p) ) {
		case 33u: goto st409;
		case 47u: goto st424;
		case 58u: goto tr449;
		case 63u: goto st448;
		case 95u: goto tr449;
	}
	if ( (*p) > 90u ) {
		if ( 97u <= (*p) && (*p) <= 122u )
			goto tr449;
	} else if ( (*p) >= 65u )
		goto tr449;
	goto st0;
st409:
	if ( ++p == pe )
		goto _test_eof409;
case 409:
	switch( (*p) ) {
		case 45u: goto st410;
		case 91u: goto st415;
	}
	goto st0;
st410:
	if ( ++p == pe )
		goto _test_eof410;
case 410:
	if ( (*p) == 45u )
		goto st411;
	goto st0;
st411:
	if ( ++p == pe )
		goto _test_eof411;
case 411:
	if ( (*p) == 45u )
		goto st412;
	goto st411;
st412:
	if ( ++p == pe )
		goto _test_eof412;
case 412:
	if ( (*p) == 45u )
		goto st413;
	goto st411;
st413:
	if ( ++p == pe )
		goto _test_eof413;
case 413:
	switch( (*p) ) {
		case 45u: goto st414;
		case 62u: goto st401;
	}
	goto st0;
st414:
	if ( ++p == pe )
		goto _test_eof414;
case 414:
	if ( (*p) == 62u )
		goto st401;
	goto st0;
st415:
	if ( ++p == pe )
		goto _test_eof415;
case 415:
	if ( (*p) == 67u )
		goto st416;
	goto st0;
st416:
	if ( ++p == pe )
		goto _test_eof416;
case 416:
	if ( (*p) == 68u )
		goto st417;
	goto st0;
st417:
	if ( ++p == pe )
		goto _test_eof417;
case 417:
	if ( (*p) == 65u )
		goto st418;
	goto st0;
st418:
	if ( ++p == pe )
		goto _test_eof418;
case 418:
	if ( (*p) == 84u )
		goto st419;
	goto st0;
st419:
	if ( ++p == pe )
		goto _test_eof419;
case 419:
	if ( (*p) == 65u )
		goto st420;
	goto st0;
st420:
	if ( ++p == pe )
		goto _test_eof420;
case 420:
	if ( (*p) == 91u )
		goto st421;
	goto st0;
st421:
	if ( ++p == pe )
		goto _test_eof421;
case 421:
	if ( (*p) == 93u )
		goto st422;
	goto st421;
st422:
	if ( ++p == pe )
		goto _test_eof422;
case 422:
	if ( (*p) == 93u )
		goto st423;
	goto st421;
st423:
	if ( ++p == pe )
		goto _test_eof423;
case 423:
	switch( (*p) ) {
		case 62u: goto st401;
		case 93u: goto st423;
	}
	goto st421;
st424:
	if ( ++p == pe )
		goto _test_eof424;
case 424:
	switch( (*p) ) {
		case 58u: goto tr466;
		case 95u: goto tr466;
	}
	if ( (*p) > 90u ) {
		if ( 97u <= (*p) && (*p) <= 122u )
			goto tr466;
	} else if ( (*p) >= 65u )
		goto tr466;
	goto st0;
tr466:
#line 30 "../src/xml.ragel"
	{ m_output.push(m_char); }
	goto st425;
st425:
	if ( ++p == pe )
		goto _test_eof425;
case 425:
#line 6010 "src/xml.cpp"
	switch( (*p) ) {
		case 13u: goto tr467;
		case 32u: goto tr467;
		case 62u: goto tr468;
		case 95u: goto tr466;
	}
	if ( (*p) < 48u ) {
		if ( (*p) > 10u ) {
			if ( 45u <= (*p) && (*p) <= 46u )
				goto tr466;
		} else if ( (*p) >= 9u )
			goto tr467;
	} else if ( (*p) > 58u ) {
		if ( (*p) > 90u ) {
			if ( 97u <= (*p) && (*p) <= 122u )
				goto tr466;
		} else if ( (*p) >= 65u )
			goto tr466;
	} else
		goto tr466;
	goto st0;
tr467:
#line 104 "../src/xml.ragel"
	{token("end element");}
	goto st426;
st426:
	if ( ++p == pe )
		goto _test_eof426;
case 426:
#line 6040 "src/xml.cpp"
	switch( (*p) ) {
		case 13u: goto st426;
		case 32u: goto st426;
		case 62u: goto tr470;
	}
	if ( 9u <= (*p) && (*p) <= 10u )
		goto st426;
	goto st0;
tr470:
#line 29 "../src/xml.ragel"
	{ { this->m_cs =  this->m_stack[-- this->m_top];goto _again;} }
	goto st460;
tr468:
#line 104 "../src/xml.ragel"
	{token("end element");}
#line 29 "../src/xml.ragel"
	{ { this->m_cs =  this->m_stack[-- this->m_top];goto _again;} }
	goto st460;
st460:
	if ( ++p == pe )
		goto _test_eof460;
case 460:
#line 6063 "src/xml.cpp"
	goto st0;
tr449:
#line 30 "../src/xml.ragel"
	{ m_output.push(m_char); }
	goto st427;
st427:
	if ( ++p == pe )
		goto _test_eof427;
case 427:
#line 6073 "src/xml.cpp"
	switch( (*p) ) {
		case 13u: goto tr471;
		case 32u: goto tr471;
		case 47u: goto tr472;
		case 62u: goto tr473;
		case 95u: goto tr449;
	}
	if ( (*p) < 45u ) {
		if ( 9u <= (*p) && (*p) <= 10u )
			goto tr471;
	} else if ( (*p) > 58u ) {
		if ( (*p) > 90u ) {
			if ( 97u <= (*p) && (*p) <= 122u )
				goto tr449;
		} else if ( (*p) >= 65u )
			goto tr449;
	} else
		goto tr449;
	goto st0;
tr471:
#line 101 "../src/xml.ragel"
	{ token("element_name"); }
	goto st428;
st428:
	if ( ++p == pe )
		goto _test_eof428;
case 428:
#line 6101 "src/xml.cpp"
	switch( (*p) ) {
		case 13u: goto st428;
		case 32u: goto st428;
		case 47u: goto st429;
		case 58u: goto tr476;
		case 62u: goto tr477;
		case 95u: goto tr476;
	}
	if ( (*p) < 65u ) {
		if ( 9u <= (*p) && (*p) <= 10u )
			goto st428;
	} else if ( (*p) > 90u ) {
		if ( 97u <= (*p) && (*p) <= 122u )
			goto tr476;
	} else
		goto tr476;
	goto st0;
tr472:
#line 101 "../src/xml.ragel"
	{ token("element_name"); }
	goto st429;
st429:
	if ( ++p == pe )
		goto _test_eof429;
case 429:
#line 6127 "src/xml.cpp"
	if ( (*p) == 62u )
		goto tr478;
	goto st0;
tr476:
#line 30 "../src/xml.ragel"
	{ m_output.push(m_char); }
	goto st430;
st430:
	if ( ++p == pe )
		goto _test_eof430;
case 430:
#line 6139 "src/xml.cpp"
	switch( (*p) ) {
		case 13u: goto tr479;
		case 32u: goto tr479;
		case 61u: goto tr480;
		case 95u: goto tr476;
	}
	if ( (*p) < 48u ) {
		if ( (*p) > 10u ) {
			if ( 45u <= (*p) && (*p) <= 46u )
				goto tr476;
		} else if ( (*p) >= 9u )
			goto tr479;
	} else if ( (*p) > 58u ) {
		if ( (*p) > 90u ) {
			if ( 97u <= (*p) && (*p) <= 122u )
				goto tr476;
		} else if ( (*p) >= 65u )
			goto tr476;
	} else
		goto tr476;
	goto st0;
tr479:
#line 80 "../src/xml.ragel"
	{token("attr_name");}
	goto st431;
st431:
	if ( ++p == pe )
		goto _test_eof431;
case 431:
#line 6169 "src/xml.cpp"
	switch( (*p) ) {
		case 13u: goto st431;
		case 32u: goto st431;
		case 61u: goto st432;
	}
	if ( 9u <= (*p) && (*p) <= 10u )
		goto st431;
	goto st0;
tr480:
#line 80 "../src/xml.ragel"
	{token("attr_name");}
	goto st432;
st432:
	if ( ++p == pe )
		goto _test_eof432;
case 432:
#line 6186 "src/xml.cpp"
	switch( (*p) ) {
		case 13u: goto st432;
		case 32u: goto st432;
		case 34u: goto st433;
		case 39u: goto st441;
	}
	if ( 9u <= (*p) && (*p) <= 10u )
		goto st432;
	goto st0;
tr485:
#line 30 "../src/xml.ragel"
	{ m_output.push(m_char); }
	goto st433;
tr492:
#line 63 "../src/xml.ragel"
	{subst_char();}
	goto st433;
tr494:
#line 63 "../src/xml.ragel"
	{subst_hex();}
	goto st433;
tr495:
#line 62 "../src/xml.ragel"
	{subst_entity();}
	goto st433;
st433:
	if ( ++p == pe )
		goto _test_eof433;
case 433:
#line 6216 "src/xml.cpp"
	switch( (*p) ) {
		case 34u: goto tr486;
		case 38u: goto st435;
		case 60u: goto st0;
	}
	goto tr485;
tr486:
#line 65 "../src/xml.ragel"
	{token("attr_value");}
	goto st434;
st434:
	if ( ++p == pe )
		goto _test_eof434;
case 434:
#line 6231 "src/xml.cpp"
	switch( (*p) ) {
		case 13u: goto st428;
		case 32u: goto st428;
		case 47u: goto st429;
		case 62u: goto tr477;
	}
	if ( 9u <= (*p) && (*p) <= 10u )
		goto st428;
	goto st0;
st435:
	if ( ++p == pe )
		goto _test_eof435;
case 435:
	switch( (*p) ) {
		case 35u: goto st436;
		case 58u: goto tr489;
		case 95u: goto tr489;
	}
	if ( (*p) > 90u ) {
		if ( 97u <= (*p) && (*p) <= 122u )
			goto tr489;
	} else if ( (*p) >= 65u )
		goto tr489;
	goto st0;
st436:
	if ( ++p == pe )
		goto _test_eof436;
case 436:
	if ( (*p) == 120u )
		goto st438;
	if ( 48u <= (*p) && (*p) <= 57u )
		goto tr490;
	goto st0;
tr490:
#line 31 "../src/xml.ragel"
	{ m_entity.push(m_char); }
	goto st437;
st437:
	if ( ++p == pe )
		goto _test_eof437;
case 437:
#line 6273 "src/xml.cpp"
	if ( (*p) == 59u )
		goto tr492;
	if ( 48u <= (*p) && (*p) <= 57u )
		goto tr490;
	goto st0;
st438:
	if ( ++p == pe )
		goto _test_eof438;
case 438:
	if ( (*p) < 65u ) {
		if ( 48u <= (*p) && (*p) <= 57u )
			goto tr493;
	} else if ( (*p) > 70u ) {
		if ( 97u <= (*p) && (*p) <= 102u )
			goto tr493;
	} else
		goto tr493;
	goto st0;
tr493:
#line 31 "../src/xml.ragel"
	{ m_entity.push(m_char); }
	goto st439;
st439:
	if ( ++p == pe )
		goto _test_eof439;
case 439:
#line 6300 "src/xml.cpp"
	if ( (*p) == 59u )
		goto tr494;
	if ( (*p) < 65u ) {
		if ( 48u <= (*p) && (*p) <= 57u )
			goto tr493;
	} else if ( (*p) > 70u ) {
		if ( 97u <= (*p) && (*p) <= 102u )
			goto tr493;
	} else
		goto tr493;
	goto st0;
tr489:
#line 31 "../src/xml.ragel"
	{ m_entity.push(m_char); }
	goto st440;
st440:
	if ( ++p == pe )
		goto _test_eof440;
case 440:
#line 6320 "src/xml.cpp"
	switch( (*p) ) {
		case 59u: goto tr495;
		case 95u: goto tr489;
	}
	if ( (*p) < 48u ) {
		if ( 45u <= (*p) && (*p) <= 46u )
			goto tr489;
	} else if ( (*p) > 58u ) {
		if ( (*p) > 90u ) {
			if ( 97u <= (*p) && (*p) <= 122u )
				goto tr489;
		} else if ( (*p) >= 65u )
			goto tr489;
	} else
		goto tr489;
	goto st0;
tr496:
#line 30 "../src/xml.ragel"
	{ m_output.push(m_char); }
	goto st441;
tr502:
#line 63 "../src/xml.ragel"
	{subst_char();}
	goto st441;
tr504:
#line 63 "../src/xml.ragel"
	{subst_hex();}
	goto st441;
tr505:
#line 62 "../src/xml.ragel"
	{subst_entity();}
	goto st441;
st441:
	if ( ++p == pe )
		goto _test_eof441;
case 441:
#line 6357 "src/xml.cpp"
	switch( (*p) ) {
		case 38u: goto st442;
		case 39u: goto tr486;
		case 60u: goto st0;
	}
	goto tr496;
st442:
	if ( ++p == pe )
		goto _test_eof442;
case 442:
	switch( (*p) ) {
		case 35u: goto st443;
		case 58u: goto tr499;
		case 95u: goto tr499;
	}
	if ( (*p) > 90u ) {
		if ( 97u <= (*p) && (*p) <= 122u )
			goto tr499;
	} else if ( (*p) >= 65u )
		goto tr499;
	goto st0;
st443:
	if ( ++p == pe )
		goto _test_eof443;
case 443:
	if ( (*p) == 120u )
		goto st445;
	if ( 48u <= (*p) && (*p) <= 57u )
		goto tr500;
	goto st0;
tr500:
#line 31 "../src/xml.ragel"
	{ m_entity.push(m_char); }
	goto st444;
st444:
	if ( ++p == pe )
		goto _test_eof444;
case 444:
#line 6396 "src/xml.cpp"
	if ( (*p) == 59u )
		goto tr502;
	if ( 48u <= (*p) && (*p) <= 57u )
		goto tr500;
	goto st0;
st445:
	if ( ++p == pe )
		goto _test_eof445;
case 445:
	if ( (*p) < 65u ) {
		if ( 48u <= (*p) && (*p) <= 57u )
			goto tr503;
	} else if ( (*p) > 70u ) {
		if ( 97u <= (*p) && (*p) <= 102u )
			goto tr503;
	} else
		goto tr503;
	goto st0;
tr503:
#line 31 "../src/xml.ragel"
	{ m_entity.push(m_char); }
	goto st446;
st446:
	if ( ++p == pe )
		goto _test_eof446;
case 446:
#line 6423 "src/xml.cpp"
	if ( (*p) == 59u )
		goto tr504;
	if ( (*p) < 65u ) {
		if ( 48u <= (*p) && (*p) <= 57u )
			goto tr503;
	} else if ( (*p) > 70u ) {
		if ( 97u <= (*p) && (*p) <= 102u )
			goto tr503;
	} else
		goto tr503;
	goto st0;
tr499:
#line 31 "../src/xml.ragel"
	{ m_entity.push(m_char); }
	goto st447;
st447:
	if ( ++p == pe )
		goto _test_eof447;
case 447:
#line 6443 "src/xml.cpp"
	switch( (*p) ) {
		case 59u: goto tr505;
		case 95u: goto tr499;
	}
	if ( (*p) < 48u ) {
		if ( 45u <= (*p) && (*p) <= 46u )
			goto tr499;
	} else if ( (*p) > 58u ) {
		if ( (*p) > 90u ) {
			if ( 97u <= (*p) && (*p) <= 122u )
				goto tr499;
		} else if ( (*p) >= 65u )
			goto tr499;
	} else
		goto tr499;
	goto st0;
st448:
	if ( ++p == pe )
		goto _test_eof448;
case 448:
	switch( (*p) ) {
		case 58u: goto tr506;
		case 88u: goto tr507;
		case 95u: goto tr506;
		case 120u: goto tr507;
	}
	if ( (*p) > 90u ) {
		if ( 97u <= (*p) && (*p) <= 122u )
			goto tr506;
	} else if ( (*p) >= 65u )
		goto tr506;
	goto st0;
tr506:
#line 30 "../src/xml.ragel"
	{ m_output.push(m_char); }
	goto st449;
st449:
	if ( ++p == pe )
		goto _test_eof449;
case 449:
#line 6484 "src/xml.cpp"
	switch( (*p) ) {
		case 13u: goto tr508;
		case 32u: goto tr508;
		case 63u: goto st452;
		case 95u: goto tr506;
	}
	if ( (*p) < 48u ) {
		if ( (*p) > 10u ) {
			if ( 45u <= (*p) && (*p) <= 46u )
				goto tr506;
		} else if ( (*p) >= 9u )
			goto tr508;
	} else if ( (*p) > 58u ) {
		if ( (*p) > 90u ) {
			if ( 97u <= (*p) && (*p) <= 122u )
				goto tr506;
		} else if ( (*p) >= 65u )
			goto tr506;
	} else
		goto tr506;
	goto st0;
tr508:
#line 30 "../src/xml.ragel"
	{ m_output.push(m_char); }
	goto st450;
st450:
	if ( ++p == pe )
		goto _test_eof450;
case 450:
#line 6514 "src/xml.cpp"
	if ( (*p) == 63u )
		goto tr510;
	goto tr508;
tr510:
#line 30 "../src/xml.ragel"
	{ m_output.push(m_char); }
	goto st451;
st451:
	if ( ++p == pe )
		goto _test_eof451;
case 451:
#line 6526 "src/xml.cpp"
	switch( (*p) ) {
		case 62u: goto tr511;
		case 63u: goto tr510;
	}
	goto tr508;
st452:
	if ( ++p == pe )
		goto _test_eof452;
case 452:
	if ( (*p) == 62u )
		goto tr511;
	goto st0;
tr507:
#line 30 "../src/xml.ragel"
	{ m_output.push(m_char); }
	goto st453;
st453:
	if ( ++p == pe )
		goto _test_eof453;
case 453:
#line 6547 "src/xml.cpp"
	switch( (*p) ) {
		case 13u: goto tr508;
		case 32u: goto tr508;
		case 63u: goto st452;
		case 77u: goto tr512;
		case 95u: goto tr506;
		case 109u: goto tr512;
	}
	if ( (*p) < 48u ) {
		if ( (*p) > 10u ) {
			if ( 45u <= (*p) && (*p) <= 46u )
				goto tr506;
		} else if ( (*p) >= 9u )
			goto tr508;
	} else if ( (*p) > 58u ) {
		if ( (*p) > 90u ) {
			if ( 97u <= (*p) && (*p) <= 122u )
				goto tr506;
		} else if ( (*p) >= 65u )
			goto tr506;
	} else
		goto tr506;
	goto st0;
tr512:
#line 30 "../src/xml.ragel"
	{ m_output.push(m_char); }
	goto st454;
st454:
	if ( ++p == pe )
		goto _test_eof454;
case 454:
#line 6579 "src/xml.cpp"
	switch( (*p) ) {
		case 13u: goto tr508;
		case 32u: goto tr508;
		case 63u: goto st452;
		case 76u: goto tr513;
		case 95u: goto tr506;
		case 108u: goto tr513;
	}
	if ( (*p) < 48u ) {
		if ( (*p) > 10u ) {
			if ( 45u <= (*p) && (*p) <= 46u )
				goto tr506;
		} else if ( (*p) >= 9u )
			goto tr508;
	} else if ( (*p) > 58u ) {
		if ( (*p) > 90u ) {
			if ( 97u <= (*p) && (*p) <= 122u )
				goto tr506;
		} else if ( (*p) >= 65u )
			goto tr506;
	} else
		goto tr506;
	goto st0;
tr513:
#line 30 "../src/xml.ragel"
	{ m_output.push(m_char); }
	goto st455;
st455:
	if ( ++p == pe )
		goto _test_eof455;
case 455:
#line 6611 "src/xml.cpp"
	if ( (*p) == 95u )
		goto tr506;
	if ( (*p) < 48u ) {
		if ( 45u <= (*p) && (*p) <= 46u )
			goto tr506;
	} else if ( (*p) > 58u ) {
		if ( (*p) > 90u ) {
			if ( 97u <= (*p) && (*p) <= 122u )
				goto tr506;
		} else if ( (*p) >= 65u )
			goto tr506;
	} else
		goto tr506;
	goto st0;
tr438:
#line 30 "../src/xml.ragel"
	{ m_output.push(m_char); }
	goto st456;
st456:
	if ( ++p == pe )
		goto _test_eof456;
case 456:
#line 6634 "src/xml.cpp"
	switch( (*p) ) {
		case 38u: goto tr436;
		case 60u: goto tr437;
		case 93u: goto tr514;
	}
	goto tr435;
tr514:
#line 30 "../src/xml.ragel"
	{ m_output.push(m_char); }
	goto st457;
st457:
	if ( ++p == pe )
		goto _test_eof457;
case 457:
#line 6649 "src/xml.cpp"
	switch( (*p) ) {
		case 38u: goto tr436;
		case 60u: goto tr437;
		case 62u: goto st0;
		case 93u: goto tr514;
	}
	goto tr435;
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
	_test_eof29:  this->m_cs = 29; goto _test_eof; 
	_test_eof30:  this->m_cs = 30; goto _test_eof; 
	_test_eof458:  this->m_cs = 458; goto _test_eof; 
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
	_test_eof459:  this->m_cs = 459; goto _test_eof; 
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
	_test_eof460:  this->m_cs = 460; goto _test_eof; 
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
	_test_eof444:  this->m_cs = 444; goto _test_eof; 
	_test_eof445:  this->m_cs = 445; goto _test_eof; 
	_test_eof446:  this->m_cs = 446; goto _test_eof; 
	_test_eof447:  this->m_cs = 447; goto _test_eof; 
	_test_eof448:  this->m_cs = 448; goto _test_eof; 
	_test_eof449:  this->m_cs = 449; goto _test_eof; 
	_test_eof450:  this->m_cs = 450; goto _test_eof; 
	_test_eof451:  this->m_cs = 451; goto _test_eof; 
	_test_eof452:  this->m_cs = 452; goto _test_eof; 
	_test_eof453:  this->m_cs = 453; goto _test_eof; 
	_test_eof454:  this->m_cs = 454; goto _test_eof; 
	_test_eof455:  this->m_cs = 455; goto _test_eof; 
	_test_eof456:  this->m_cs = 456; goto _test_eof; 
	_test_eof457:  this->m_cs = 457; goto _test_eof; 

	_test_eof: {}
	_out: {}
	}

#line 132 "../src/xml.ragel"
	}
	
	return (m_cs != 
#line 7127 "src/xml.cpp"
0
#line 134 "../src/xml.ragel"
);
}
