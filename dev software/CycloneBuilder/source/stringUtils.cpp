#include "stringUtils.h"

string toLower(string S) {
	char C;
	for (int I = 0; I < S.length(); I++) {
		C = S[I];
		if (C <= 'Z' && C >= 'A') {
			C = C - ('Z' - 'z');
		}
		S[I] = C;
	}
	return S;
}
