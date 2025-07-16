#pragma once

//Unicode list of blocks related to Arabic: https://en.wikipedia.org/wiki/Unicode_block
//Plane         Block Range          Block Name                                 Codepoints       Assigned Characters              Scripts
//----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
//[0 BMP]     U+0600..U+06FF           Arabic                                    256                    256                   Arabic (238 characters), Common (6 characters), Inherited (12 characters)
//[0 BMP]     U+0750..U+077F      Arabic Supplement                              48                     48                    Arabic
//[0 BMP]     U+0870..U+089F      Arabic Extended B                              41                     41                    Arabic
//[0 BMP]     U+08A0..U+08FF      Arabic Extended A                              96                     96                    Arabic
//[0 BMP]     U+FB50..U+FDFF      Arabic Presentation Forms A                    688                    631                   Arabic (629 characters), Common (2 characters)
//[0 BMP]     U+FE70..U+FEFF      Arabic Presentation Forms B                    144                    141                   Arabic (140 characters), Common (1 character)
//[1 SMP]     U+10E60..U+10E7F    Rumi Numeral SYmbols                           32                     31                    Arabic
//[1 SMP]     U+10EC0..U+10EFF    Arabic Extended C                              64                     3                     Arabic
//[1 SMP]     U+1EE00..U+1EEFF    Arabic Mathematical Alphabetic Symbols         256                    143                   Arabic
//================================================================================================================================================================================================================


//Arabic Range
#define ARABIC_RANGE_START 0x0621
#define ARABIC_RANGE_END 0x064A

//Lam & Aleph have some cases, let's keep their codepoints accessible in easy way
#define ARABIC_UNICODE_LETTER_LAM 0x0644
#define ARABIC_UNICODE_LETTER_ALEPH 0x0627
#define ARABIC_UNICODE_LETTER_LAM_ALEPH 0xFEFC
#define ARABIC_UNICODE_LETTER_LAM_ALEPH_ISOLATED 0xFEFB

//we will need these defines to access the elements of the [4] array below in a tidy & recognized way
#define ARABIC_UNICODE_LETTER_FORM_ISOLATED 0		//aalt - Access All Alternates
#define ARABIC_UNICODE_LETTER_FORM_END 1		    //fina - Terminal Forms
#define ARABIC_UNICODE_LETTER_FORM_BEGIN 2			//init - Initial Forms
#define ARABIC_UNICODE_LETTER_FORM_MIDDLE 3		    //medi - Medial Forms

typedef i32 per_character_major_forms[4];       //array of 4 forms base don the 4 majors forms (isolate, ending, initial & middle)
typedef per_character_major_forms all_forms[2]; //first array is the 4 forms, second array is the Ligatures

/*
0: Isolated form
1: End form
2: Begin form
3: Middle form
*/
static all_forms G_ARABIC_PRESENTATION_FORMS[] =
{
//forms                                 Ligatures
//{Isolated, Ending, Initial, Middle} , {Isolated, Ending, Initial, Middle}
	{ {0xFE80, 0xFE80,      0,      0}, {-1, -1, 0, 0} },			            //[0] ء
	{ {0xFE81, 0xFE82,      0,      0}, {-1, -1, 0xFEF5, 0xFEF6} },	            //[1] آ
	{ {0xFE83, 0xFE84,      0,      0}, {-1, -1, 0xFEF7, 0xFEF8} },	            //[2] أ
	{ {0xFE85, 0xFE86,      0,      0}, {-1, -1, 0, 0} },			            //[3] ؤ
	{ {0xFE87, 0xFE88,      0,      0}, {-1, -1, 0xFEF9, 0xFEFA} },	            //[4] ﺇ
	{ {0xFE89, 0xFE8A, 0xFE8B, 0xFE8C}, {-1, -1, 0, 0} },			            //[5] ﺉ
	{ {0xFE8D, 0xFE8E,      0,      0}, {-1, -1, 0xFEFB, 0xFEFC} },	            //[6] ﺍ
	{ {0xFE8F, 0xFE90, 0xFE91, 0xFE92}, {-1, -1, 0, 0} },			            //[7] ب
	{ {0xFE93, 0xFE94,      0,      0}, {-1, -1, 0, 0} },			            //[8] ﺓ
	{ {0xFE95, 0xFE96, 0xFE97, 0xFE98}, {-1, -1, 0, 0} },			            //[9] ت
	{ {0xFE99, 0xFE9A, 0xFE9B, 0xFE9C}, {-1, -1, 0, 0} },			            //[10] ث
	{ {0xFE9D, 0xFE9E, 0xFE9F, 0xFEA0}, {-1, -1, 0, 0} },			            //[11] ج
	{ {0xFEA1, 0xFEA2, 0xFEA3, 0xFEA4}, {-1, -1, 0, 0} },			            //[12] ح
	{ {0xFEA5, 0xFEA6, 0xFEA7, 0xFEA8}, {-1, -1, 0, 0} },			            //[13] خ
	{ {0xFEA9, 0xFEAA,      0,      0}, {-1, -1, 0, 0} },			            //[14] د
	{ {0xFEAB, 0xFEAC,      0,      0}, {-1, -1, 0, 0} },			            //[15] ذ
	{ {0xFEAD, 0xFEAE,      0,      0}, {-1, -1, 0, 0} },			            //[16] ر
	{ {0xFEAF, 0xFEB0,      0,      0}, {-1, -1, 0, 0} },			            //[17] ز
	{ {0xFEB1, 0xFEB2, 0xFEB3, 0xFEB4}, {-1, -1, 0, 0} },			            //[18] س
	{ {0xFEB5, 0xFEB6, 0xFEB7, 0xFEB8}, {-1, -1, 0, 0} },			            //[19] ش
	{ {0xFEB9, 0xFEBA, 0xFEBB, 0xFEBC}, {-1, -1, 0, 0} },			            //[20] ص
	{ {0xFEBD, 0xFEBE, 0xFEBF, 0xFEC0}, {-1, -1, 0, 0} },			            //[21] ض
	{ {0xFEC1, 0xFEC2, 0xFEC3, 0xFEC4}, {-1, -1, 0, 0} },			            //[22] ط
	{ {0xFEC5, 0xFEC6, 0xFEC7, 0xFEC8}, {-1, -1, 0, 0} },			            //[23] ظ
	{ {0xFEC9, 0xFECA, 0xFECB, 0xFECC}, {-1, -1, 0, 0} },			            //[24] ع
	{ {0xFECD, 0xFECE, 0xFECF, 0xFED0}, {-1, -1, 0, 0} },			            //[25] غ
	{ {     0,      0,      0,      0}, {-1, -1, 0, 0} },			            //[26]
	{ {     0,      0,      0,      0}, {-1, -1, 0, 0} },			            //[27]
	{ {     0,      0,      0,      0}, {-1, -1, 0, 0} },			            //[28]
	{ {     0,      0,      0,      0}, {-1, -1, 0, 0} },			            //[29]
	{ {     0,      0,      0,      0}, {-1, -1, 0, 0} },			            //[30]
	{ { 0x640,  0x640,  0x640,  0x640}, {-1, -1, 0, 0} },			            //[31] ـ
	{ {0xFED1, 0xFED2, 0xFED3, 0xFED4}, {-1, -1, 0, 0} },			            //[32] ف
	{ {0xFED5, 0xFED6, 0xFED7, 0xFED8}, {-1, -1, 0, 0} },			            //[33] ق
	{ {0xFED9, 0xFEDA, 0xFEDB, 0xFEDC}, {-1, -1, 0, 0} },			            //[34] ك
	{ {0xFEDD, 0xFEDE, 0xFEDF, 0xFEE0}, {-1, -1, 0, 0} },			            //[35] ل
	{ {0xFEE1, 0xFEE2, 0xFEE3, 0xFEE4}, {-1, -1, 0, 0} },			            //[36] م
	{ {0xFEE5, 0xFEE6, 0xFEE7, 0xFEE8}, {-1, -1, 0, 0} },			            //[37] ن
	{ {0xFEE9, 0xFEEA, 0xFEEB, 0xFEEC}, {-1, -1, 0, 0} },			            //[38] ه
	{ {0xFEED, 0xFEEE,      0,      0}, {-1, -1, 0, 0} },			            //[39] و
	{ {0xFEEF, 0xFEF0,      0,      0}, {-1, -1, 0, 0} },			            //[40] ﻯ
	{ {0xFEF1, 0xFEF2, 0xFEF3, 0xFEF4}, {-1, -1, 0, 0} },			            //[41] ﻱ
};

//empty entires [26 : 30] is to compensate while traversing the array, otherwise the order of letters is messed up.

static inline b8 is_character_in_arabic_codepoint_range(u32 codepoint)
{
	return codepoint >= ARABIC_RANGE_START && codepoint <= ARABIC_RANGE_END;
}

//Check if a given codepoint is a Lam-Aleph type.
//This happens when the given codepoint is Lam and the next codepoint is an Arabic character and the initial ligatures for the next character is valid value (in the ligatures array)
static inline b8 is_lam_alef(u32 codepoint, u32 next)
{
	return codepoint == ARABIC_UNICODE_LETTER_LAM && is_character_in_arabic_codepoint_range(next) && G_ARABIC_PRESENTATION_FORMS[next - ARABIC_RANGE_START][1][ARABIC_UNICODE_LETTER_FORM_BEGIN] != 0;
}

//Check if there is a Lam before the given codepoint
//This happens when the previous codepoint is Lam and the current codepoint is an Arabic codepoint and the initial ligatures for the current character is valid value (in the ligatures array)
static inline b8 is_alef_before_lam(u32 prev, u32 codepoint)
{
	return prev == ARABIC_UNICODE_LETTER_LAM && is_character_in_arabic_codepoint_range(codepoint) && G_ARABIC_PRESENTATION_FORMS[codepoint - ARABIC_RANGE_START][1][ARABIC_UNICODE_LETTER_FORM_BEGIN] != 0;
}

//Check for linking of the the given codepoint
//This happens when the given codepoitn is an Arabic codepoint and the middle form for the current codepoint isolated form is valid value (in the 4 forms array)
static inline b8 is_linking_type(u32 codepoint)
{
	return is_character_in_arabic_codepoint_range(codepoint) && G_ARABIC_PRESENTATION_FORMS[codepoint - ARABIC_RANGE_START][0][ARABIC_UNICODE_LETTER_FORM_MIDDLE] != 0;
}

//Check if the given codepoint is for a Diacritics or sign,
//Unfortunatly the ranges are scattered for the different signs
/* static inline b8 is_diacritics_or_sign(u32 codepoint)
{
	return
		(codepoint >= ARABIC_TASHKEEL_A_START && codepoint <= ARABIC_TASHKEEL_A_END) ||
		(codepoint >= ARABIC_TASHKEEL_B_START && codepoint <= ARABIC_TASHKEEL_B_END) ||
		(codepoint >= ARABIC_TASHKEEL_C_START && codepoint <= ARABIC_TASHKEEL_C_END);
} */
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wbitwise-instead-of-logical"
#pragma clang diagnostic ignored "-Wunused-function"
static u32 get_presentation_form_for_char(u32 prev, u32 next, u32 codepoint)
{
	//early out if not an arabic letter
	if (!is_character_in_arabic_codepoint_range(codepoint))
	{
		return codepoint;
	}
  
  //bools to check for Aleph & Lam mixing speical cases (compound or ligatures)
	b8 _is_lam_aleph = is_lam_alef(codepoint, next);
	b8 _is_alef_before_lam = is_alef_before_lam(prev, codepoint);

	b8 _is_aleph_or_lam_case = _is_lam_aleph || _is_alef_before_lam;

  //get the form from the array:
  //first get the index of the from from the 4 forms (linkage type & being aleph/lam case is defining that)
  //then get the character reference (which one from 0 to 41 from the array entires)
  //naviage through the array to return the proper form (basic for or ligatures)
	u32 _idx = (( (_is_aleph_or_lam_case | is_character_in_arabic_codepoint_range(next)) & is_linking_type(codepoint)) << 1) | is_linking_type(prev);
	u32 _ref = next * _is_lam_aleph + codepoint * (!_is_lam_aleph) - ARABIC_RANGE_START;
	return G_ARABIC_PRESENTATION_FORMS[_ref][_is_aleph_or_lam_case][_idx];
}
#pragma clang diagnostic pop
