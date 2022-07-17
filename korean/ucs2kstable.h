#pragma once

unsigned short KSToUnicode(unsigned short codepoint);
unsigned short UnicodeToKS(unsigned short codepoint);
void DecomposeUnicode(unsigned short codepoint, int& outFirst, int& outVowel, int& outFinal);
