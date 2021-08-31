#pragma once

// Copyright 2010 Won Star

#include <queue>

typedef unsigned char Jamo;

struct CompositingChar
{
	Jamo initial;
	Jamo vowel;
	Jamo vowel2;
	Jamo final;
	Jamo final2;

	CompositingChar()
	{
		Clear();
	}

	void Clear();

	bool IsEmpty();
};

class Automata
{
	char consonantToIdx[128];
	char vowelToIdx[128];
	char finalToIdx[128];

	int currentState;

	CompositingChar compositingChar;

	std::queue< wchar_t > completedChars;

private:
	static bool IsInitialConsonant(char ch);
	static bool IsVowel(char ch);
	static bool IsFinal(char ch);
	static bool IsNonKorean(char ch);

	static int CompositeVowel(char vowel, char vowel2);
	static int CompositeFinal(char final, char final2);

	static bool IsAcceptableVowel2(char vowel, char vowel2);
	static bool IsAcceptableFinal2(char final, char final2);

	static wchar_t ToUnicodeChar(const CompositingChar& compositingChar);

	char FinalToConsonant(char final);

	void InitConsonantTable();
	void InitVowelTable();
	void InitFinalTable();

public:
	Automata();
	~Automata();

	// 스테이트 롤백
	void RollbackState();

	// 아스키로 입력
	void OnInput(char ch);
	void ProcessInput(char ch);

	void CompleteChar();
	void CancelAllInputs();

	bool IsCompositing();

	wchar_t GetCompositingChar();
	std::queue< wchar_t >& GetCompletedChars();
};
