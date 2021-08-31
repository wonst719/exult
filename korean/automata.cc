// Copyright 2010 Won Star

#include <memory.h>
#include <string.h>

#include "automata.h"

// 한글 입력용 자체 5바이트 체계 -> UCS-2

// 초성: ㄱ ㄲ ㄴ ㄷ ㄸ ㄹ ㅁ ㅂ ㅃ ㅅ ㅆ ㅇ ㅈ ㅉ ㅊ ㅋ ㅌ ㅍ ㅎ

// 중성: ㅏ ㅐ ㅑ ㅒ ㅓ ㅔ ㅕ ㅖ ㅗ ㅘ ㅙ ㅚ ㅛ ㅜ ㅝ ㅞ ㅟ ㅠ ㅡ ㅢ ㅣ
//  - 단모음: ㅏ ㅐ ㅑ ㅒ ㅓ ㅔ ㅕ ㅖ ㅗ ㅛ ㅜ ㅠ ㅡ ㅣ
//  - 복모음: ㅘ ㅙ ㅚ ㅝ ㅞ ㅟ ㅢ

// 종성:    ㄱ ㄲ ㄳ ㄴ ㄵ ㄶ ㄷ ㄹ ㄺ ㄻ ㄼ ㄽ ㄾ ㄿ ㅀ ㅁ ㅂ ㅄ ㅅ ㅆ ㅇ ㅈ ㅊ ㅋ ㅌ ㅍ ㅎ
//  - 단자음: ㄱ ㄲ ㄴ ㄷ ㄹ ㅁ ㅂ ㅅ ㅆ ㅇ ㅈ ㅊ ㅋ ㅌ ㅍ ㅎ
//  - 쌍자음: ㄳ ㄵ ㄶ ㄺ ㄻ ㄼ ㄽ ㄾ ㄿ ㅀ ㅄ

// 상태
// C: 초성
// V: 중성
// VV: 복모음
// B: 종성
// BB: 복자음
// X: 이외의 문자

/*
	S0	-C> S1
		-V> S2

	S1	-V> S2
		-X> S6

	S2	-VV> S3
		-B> S4
		-X> S6

	S3	-B> S4
		-X> S6

	S4	-BB> S5
		-X> S6
		-V> S7

	S5	-X> S6
		-V> S7

	S6	-> S0 / 출력

	S7	-> S2 / 출력
*/

// 모음, 자음
static const char* consonants = " rRseEfaqQtTdwWczxvg";
static const char* vowels = " koiOjpuPh   yn   bm l";

// 끝자리에 올 수 있는 것
static const char* finals = " rR s  ef       aq tTdwczxvg";

// 끝소리용
static const Jamo finalTransition[11][3] = {
	{1, 19, 2},		// ㄱㅅ
	{3, 21, 1},		// ㄴㅈ
	{3, 27, 2},		// ㄴㅎ
	{8, 1, 1},		// ㄹㄱ
	{8, 16, 2},		// ㄹㅁ
	{8, 17, 3},		// ㄹㅂ
	{8, 19, 4},		// ㄹㅅ
	{8, 25, 5},		// ㄹㅌ
	{8, 26, 6},		// ㄹㅍ
	{8, 27, 7},		// ㄹㅎ
	{17, 19, 1},	// ㅂㅅ
};

static const Jamo vowelTransition[7][3] = {
	{9, 1, 1},		// ㅗㅏ
	{9, 2, 2},		// ㅗㅐ
	{9, 21, 3},		// ㅗㅣ
	{14, 5, 1},		// ㅜㅓ
	{14, 6, 2},		// ㅜㅔ
	{14, 21, 3},	// ㅜㅣ
	{19, 21, 1},	// ㅡㅣ
};

void CompositingChar::Clear()
{
	initial = 0;
	vowel = 0;
	vowel2 = 0;
	final = 0;
	final2 = 0;
}

bool CompositingChar::IsEmpty()
{
	return initial == 0 && vowel == 0 && vowel2 == 0 && final == 0 && final2 == 0;
}

//////////////////////////////////////////////////////////////////////////

Automata::Automata()
{
	currentState = 0;

	InitConsonantTable();
	InitVowelTable();
	InitFinalTable();
}

Automata::~Automata()
{
}

void Automata::InitConsonantTable()
{
	memset(consonantToIdx, 0, sizeof(char) * 128);

	for (size_t i = 1; i < strlen(consonants); i++)
	{
		consonantToIdx[consonants[i]] = i;
	}
}

void Automata::InitVowelTable()
{
	memset(vowelToIdx, 0, sizeof(char) * 128);

	for (size_t i = 1; i < strlen(vowels); i++)
	{
		vowelToIdx[vowels[i]] = i;
	}
}

void Automata::InitFinalTable()
{
	memset(finalToIdx, 0, sizeof(char) * 128);

	for (size_t i = 1; i < strlen(finals); i++)
	{
		finalToIdx[finals[i]] = i;
	}
}

bool Automata::IsInitialConsonant(char ch)
{
	if (ch == ' ')
		return false;

	return strchr(consonants, ch) != 0;
}

bool Automata::IsVowel(char ch)
{
	if (ch == ' ')
		return false;

	return strchr(vowels, ch) != 0;
}

bool Automata::IsFinal(char ch)
{
	if (ch == ' ')
		return false;

	return strchr(finals, ch) != 0;
}

bool Automata::IsNonKorean(char ch)
{
	return !IsInitialConsonant(ch) && !IsVowel(ch);
}

int Automata::CompositeVowel(char vowel, char vowel2)
{
	for (size_t i = 0; i < 7; i++)
	{
		if (vowelTransition[i][0] == vowel && vowelTransition[i][1] == vowel2)
			return vowel + vowelTransition[i][2];
	}

	return vowel;
}

int Automata::CompositeFinal(char final, char final2)
{
	for (size_t i = 0; i < 11; i++)
	{
		if (finalTransition[i][0] == final && finalTransition[i][1] == final2)
			return final + finalTransition[i][2];
	}

	return final;
}

bool Automata::IsAcceptableVowel2(char vowel, char vowel2)
{
	for (size_t i = 0; i < 7; i++)
	{
		if (vowelTransition[i][0] == vowel && vowelTransition[i][1] == vowel2)
			return true;
	}

	return false;
}

bool Automata::IsAcceptableFinal2(char final, char final2)
{
	for (size_t i = 0; i < 11; i++)
	{
		if (finalTransition[i][0] == final && finalTransition[i][1] == final2)
			return true;
	}

	return false;
}

char Automata::FinalToConsonant(char final)
{
	if (final == ' ')
		return false;

	if (finals[final] == ' ')
	{
		return -1;
	}
	else
	{
		if (consonantToIdx[finals[final]] == ' ')
		{
			return -1;
		}
		else
		{
			return consonantToIdx[finals[final]];
		}
	}
}

wchar_t Automata::ToUnicodeChar(const CompositingChar& compositingChar)
{
	// (((초성 * 21) + 중성) * 28) + 종성 + 0xAC00;

	// 초성만
	if (compositingChar.initial && !compositingChar.vowel)
	{
		return 0x1100 + compositingChar.initial - 1;
	}

	// 중성만
	if (!compositingChar.initial && compositingChar.vowel)
	{
		return 0x1161 + CompositeVowel(compositingChar.vowel, compositingChar.vowel2) - 1;
	}

	int initial = compositingChar.initial - 1;
	int vowel = CompositeVowel(compositingChar.vowel, compositingChar.vowel2) - 1;
	int final = CompositeFinal(compositingChar.final, compositingChar.final2);

	wchar_t ch = 0xac00;
	ch += (((initial * 21) + vowel) * 28) + final;

	return ch;
}

void Automata::ProcessInput(char ch)
{
	switch (currentState)
	{
	case 0:
		{
			// 	S0	-C> S1
			// 		-V> S2
			if (IsInitialConsonant(ch))
			{
				compositingChar.initial = consonantToIdx[ch];
				currentState = 1;
			}
			else if (IsVowel(ch))
			{
				compositingChar.vowel = vowelToIdx[ch];
				currentState = 2;
			}
			else
			{
				if (compositingChar.IsEmpty())
				{
					//
				}
				else
				{
					completedChars.push(ToUnicodeChar(compositingChar));

					memset(&compositingChar, 0, sizeof(compositingChar));
				}

				completedChars.push(ch);
			}
		}
		break;
	case 1:
		// 	S1	-V> S2
		// 		-X> S6
		if (IsVowel(ch))
		{
			compositingChar.vowel = vowelToIdx[ch];
			currentState = 2;
		}
		else
		{
			currentState = 6;
			ProcessInput(ch);
		}
		break;
	case 2:
		// 	S2	-VV> S3
		// 		-B> S4
		// 		-X> S6

		if (IsAcceptableVowel2(compositingChar.vowel, vowelToIdx[ch]))
		{
			compositingChar.vowel2 = vowelToIdx[ch];
			currentState = 3;
		}
		else if (IsFinal(ch))
		{
			compositingChar.final = finalToIdx[ch];
			currentState = 4;
		}
		else
		{
			currentState = 6;
			ProcessInput(ch);
		}
		break;
	case 3:
		// 	S3	-B> S4
		// 		-X> S6
		if (IsFinal(ch))
		{
			compositingChar.final = finalToIdx[ch];
			currentState = 4;
		}
		else
		{
			currentState = 6;
			ProcessInput(ch);
		}
		break;
	case 4:
		// 	S4	-BB> S5
		// 		-X> S6
		// 		-V> S7

		if (IsFinal(ch) && IsAcceptableFinal2(compositingChar.final, finalToIdx[ch]))
		{
			compositingChar.final2 = finalToIdx[ch];
			currentState = 5;
		}
		else if (IsVowel(ch))
		{
			currentState = 7;
			ProcessInput(ch);
		}
		else
		{
			currentState = 6;
			ProcessInput(ch);
		}
		break;
	case 5:
		// 	S5	-X> S6
		// 		-V> S7
		if (IsVowel(ch))
		{
			currentState = 7;
			ProcessInput(ch);
		}
		else
		{
			currentState = 6;
			ProcessInput(ch);
		}
		break;
	case 6:
		// 	S6	-> S0 / 출력
		{
			completedChars.push(ToUnicodeChar(compositingChar));
			memset(&compositingChar, 0, sizeof(compositingChar));

		}
		currentState = 0;
		ProcessInput(ch);
		break;
	case 7:		// V 넘김
		{
			int nextInitial = 0;
			if (compositingChar.final2)
			{
				nextInitial = FinalToConsonant(compositingChar.final2);
				compositingChar.final2 = 0;
			}
			else if (compositingChar.final)
			{
				nextInitial = FinalToConsonant(compositingChar.final);
				compositingChar.final = 0;
			}

			// 	S7	-> S2 / 출력
			{
				completedChars.push(ToUnicodeChar(compositingChar));
				memset(&compositingChar, 0, sizeof(compositingChar));
				if (nextInitial < 0)
				{
					printf("err\n");
				}
				compositingChar.initial = nextInitial;
				compositingChar.vowel = vowelToIdx[ch];
			}

			currentState = 2;
		}
		break;
	}
}

void Automata::RollbackState()
{
	if (compositingChar.final2)
	{
		compositingChar.final2 = 0;
		currentState = 4;
	}
	else if (compositingChar.final)
	{
		compositingChar.final = 0;
		if (compositingChar.vowel2)
		{
			currentState = 3;
		}
		else
		{
			currentState = 2;
		}
	}
	else if (compositingChar.vowel2)
	{
		compositingChar.vowel2 = 0;
		currentState = 2;
	}
	else if (compositingChar.vowel)
	{
		compositingChar.vowel = 0;
		if (compositingChar.initial)
		{
			currentState = 1;
		}
		else
		{
			currentState = 0;
		}
	}
	else if (compositingChar.initial)
	{
		compositingChar.initial = 0;
		currentState = 0;
	}
	else
	{
		currentState = 0;
	}
}

void Automata::OnInput(char ch)
{

}

bool Automata::IsCompositing()
{
	return !compositingChar.IsEmpty();
}

void Automata::CompleteChar()
{
	if (!compositingChar.IsEmpty())
	{
		completedChars.push(ToUnicodeChar(compositingChar));
	}

	memset(&compositingChar, 0, sizeof(compositingChar));
	currentState = 0;
}

void Automata::CancelAllInputs()
{
	while (!completedChars.empty())
	{
		completedChars.pop();
	}

	memset(&compositingChar, 0, sizeof(compositingChar));
	currentState = 0;
}

wchar_t Automata::GetCompositingChar()
{
	if (compositingChar.IsEmpty())
		return 0;

	return ToUnicodeChar(compositingChar);
}

std::queue< wchar_t >& Automata::GetCompletedChars()
{
	return completedChars;
}