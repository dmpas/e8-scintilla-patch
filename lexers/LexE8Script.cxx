// Scintilla source code edit control
/** @file LexE8Script.cxx
 ** Lexer for E8::Sript
 **/

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <assert.h>
#include <ctype.h>

#include "ILexer.h"
#include "Scintilla.h"
#include "SciLexer.h"

#include "WordList.h"
#include "LexAccessor.h"
#include "Accessor.h"
#include "StyleContext.h"
#include "CharacterSet.h"
#include "LexerModule.h"

#ifdef SCI_NAMESPACE
using namespace Scintilla;
#endif


static bool IsE8Comment(Accessor &styler, int pos, int len) {
	return len > 0 && styler[pos] == '\'';
}

static inline bool IsTypeCharacter(int ch) {
	return ch == '%' || ch == '&' || ch == '@' || ch == '!' || ch == '#' || ch == '$';
}

static inline bool IsAWordChar(int ch) {

	if (ch >= 0x400 && ch < 0x500)
		return true;

	return ch >= 0x80 ||
	       (isalnum(ch) || ch == '.' || ch == '_');
}

static inline bool IsAWordStart(int ch) {

	if (ch >= 0x400 && ch < 0x500)
		return true;

	return ch >= 0x80 ||
	       (isalpha(ch) || ch == '_');
}

static inline bool IsANumberChar(int ch) {
	return (ch < 0x80) &&
	        (isdigit(ch) || toupper(ch) == 'E' ||
             ch == '.' || ch == '-' || ch == '+');
}


static int bukvaLower(int ch)
{
	if (ch >= 0x0410 && ch < 0x0430)
		return ch + 0x20;
	return ch;
}


static void utf_lowercase(char *s)
{
	unsigned char *c = (unsigned char *)s;
	while (*c) {
		if (*c < 0x80) {
			*c = tolower(*c);
		} else {
			unsigned cp = *c;
            ++c;
            cp = ((cp << 6) & 0x7ff) + ((*c) & 0x3f);
			--c;
			cp = bukvaLower(cp);

			*c = ((cp >> 6) & 0x1F) | 0xC0;
            ++c;

            *c = (cp & 0x3F) | 0x80;
		}
		++c;
	}
}

static void ColouriseE8Doc(unsigned int startPos, int length, int initStyle,
                           WordList *keywordlists[], Accessor &styler) {

	WordList &keywords = *keywordlists[0];
	WordList &keywords2 = *keywordlists[1];
	WordList &keywords3 = *keywordlists[2];
	WordList &keywords4 = *keywordlists[3];

	styler.StartAt(startPos);

	// Do not leak onto next line
	if ( initStyle == SCE_E8_STRINGEOL 
		|| initStyle == SCE_E8_COMMENT 
		|| initStyle == SCE_E8_DOC
		|| initStyle == SCE_E8_PREPROCESSOR 
		|| initStyle == SCE_E8_DATE) {
		initStyle = SCE_E8_DEFAULT;
	}

	StyleContext sc(startPos, length, initStyle, styler);

	for (; sc.More(); sc.Forward()) {
		if (sc.state == SCE_E8_OPERATOR) {
			sc.SetState(SCE_E8_DEFAULT);
		} else if (sc.state == SCE_E8_IDENTIFIER) {
			if (!IsAWordChar(sc.ch)) {
				char s[1000];
				sc.GetCurrent(s, sizeof(s));
				utf_lowercase(s);

				if (keywords.InList(s)) {
					sc.ChangeState(SCE_E8_KEYWORD);
				} else if (keywords2.InList(s)) {
					sc.ChangeState(SCE_E8_KEYWORD2);
				} else if (keywords3.InList(s)) {
					sc.ChangeState(SCE_E8_KEYWORD3);
				} else if (keywords4.InList(s)) {
					sc.ChangeState(SCE_E8_KEYWORD4);
				}	// Else, it is really an identifier...

				sc.SetState(SCE_E8_DEFAULT);
			}
		} else if (sc.state == SCE_E8_NUMBER) {
			if (!IsANumberChar(sc.ch) && !(tolower(sc.ch) >= 'a' && tolower(sc.ch) <= 'f')) {
				sc.SetState(SCE_E8_DEFAULT);
			}
		} else if (sc.state == SCE_E8_STRING) {

			if (sc.ch == '\"') {
				if (sc.chNext == '\"' || sc.chNext == '|') {
					sc.Forward();
				} else {
					sc.ForwardSetState(SCE_E8_DEFAULT);
				}
			} else if (sc.atLineEnd) {
				sc.ChangeState(SCE_E8_STRINGEOL);
				sc.ForwardSetState(SCE_E8_DEFAULT);
			}
		} else if (sc.state == SCE_E8_COMMENT || sc.state == SCE_E8_DOC) {
			if (sc.atLineEnd) {
				sc.ForwardSetState(SCE_E8_DEFAULT);
			}
		} else if (sc.state == SCE_E8_MULTYLINE_COMMENT || sc.state == SCE_E8_MULTYLINE_DOC) {
			if (sc.ch == '*' && sc.chNext == '/') {
				sc.Forward();

				sc.ForwardSetState(SCE_E8_DEFAULT);
			}
		} else if (sc.state == SCE_E8_PREPROCESSOR) {
			if (sc.atLineEnd) {
				sc.ForwardSetState(SCE_E8_DEFAULT);
			}
		} else if (sc.state == SCE_E8_DATE) {
			if (sc.atLineEnd) {
				sc.ForwardSetState(SCE_E8_DEFAULT);
			} else if (sc.ch == '\'') {
				sc.ForwardSetState(SCE_E8_DEFAULT);
			}
		}

		if (sc.state == SCE_E8_DEFAULT) {
			if (sc.ch == '/' && sc.chNext == '/') {
				
				sc.SetState(SCE_E8_COMMENT);
				sc.Forward();
				
				if (sc.chNext == '!' || sc.chNext == '*' || sc.chNext == '/') {
					sc.ChangeState(SCE_E8_DOC);
					sc.SetState(SCE_E8_DOC);
				}
					
			} else if (sc.ch == '/' && sc.chNext == '*') {
				
				sc.SetState(SCE_E8_MULTYLINE_COMMENT);
				sc.Forward();
				
				if (sc.chNext == '!' || sc.chNext == '*') {
					sc.ChangeState(SCE_E8_MULTYLINE_DOC);
					sc.SetState(SCE_E8_MULTYLINE_DOC);
				}
					
			} else if (sc.ch == '\"') {
				sc.SetState(SCE_E8_STRING);
			} else if (sc.ch == '|') {
				sc.SetState(SCE_E8_STRING);
			} else if (sc.ch == '\'') {
				sc.SetState(SCE_E8_DATE);
			} else if (sc.ch == '#') {
				sc.SetState(SCE_E8_PREPROCESSOR);
			} else if (sc.ch == '&') {
				sc.SetState(SCE_E8_PREPROCESSOR);
			} else if (IsADigit(sc.ch) || (sc.ch == '.' && IsADigit(sc.chNext))) {
				sc.SetState(SCE_E8_NUMBER);
			} else if (IsAWordStart(sc.ch)) {
				sc.SetState(SCE_E8_IDENTIFIER);
			} else if (isoperator(static_cast<char>(sc.ch)) || (sc.ch == '\\')) {	// Integer division
				sc.SetState(SCE_E8_OPERATOR);
			}
		}

	}

	if (sc.state == SCE_E8_IDENTIFIER && !IsAWordChar(sc.ch)) {
		char s[1000];
		sc.GetCurrent(s, sizeof(s));
		utf_lowercase(s);
		if (keywords.InList(s)) {
			sc.ChangeState(SCE_E8_KEYWORD);
		} else if (keywords2.InList(s)) {
			sc.ChangeState(SCE_E8_KEYWORD2);
		} else if (keywords3.InList(s)) {
			sc.ChangeState(SCE_E8_KEYWORD3);
		} else if (keywords4.InList(s)) {
			sc.ChangeState(SCE_E8_KEYWORD4);
		}	// Else, it is really an identifier...
		sc.SetState(SCE_E8_DEFAULT);
	}

	sc.Complete();
}

static void FoldE8Doc(unsigned int startPos, int length, int,
						   WordList *[], Accessor &styler) {
	int endPos = startPos + length;

	// Backtrack to previous line in case need to fix its fold status
	int lineCurrent = styler.GetLine(startPos);
	if (startPos > 0) {
		if (lineCurrent > 0) {
			lineCurrent--;
			startPos = styler.LineStart(lineCurrent);
		}
	}
	int spaceFlags = 0;
	int indentCurrent = styler.IndentAmount(lineCurrent, &spaceFlags, IsE8Comment);
	char chNext = styler[startPos];
	for (int i = startPos; i < endPos; i++) {
		char ch = chNext;
		chNext = styler.SafeGetCharAt(i + 1);

		if ((ch == '\r' && chNext != '\n') || (ch == '\n') || (i == endPos)) {
			int lev = indentCurrent;
			int indentNext = styler.IndentAmount(lineCurrent + 1, &spaceFlags, IsE8Comment);
			if (!(indentCurrent & SC_FOLDLEVELWHITEFLAG)) {
				// Only non whitespace lines can be headers
				if ((indentCurrent & SC_FOLDLEVELNUMBERMASK) < (indentNext & SC_FOLDLEVELNUMBERMASK)) {
					lev |= SC_FOLDLEVELHEADERFLAG;
				} else if (indentNext & SC_FOLDLEVELWHITEFLAG) {
					// Line after is blank so check the next - maybe should continue further?
					int spaceFlags2 = 0;
					int indentNext2 = styler.IndentAmount(lineCurrent + 2, &spaceFlags2, IsE8Comment);
					if ((indentCurrent & SC_FOLDLEVELNUMBERMASK) < (indentNext2 & SC_FOLDLEVELNUMBERMASK)) {
						lev |= SC_FOLDLEVELHEADERFLAG;
					}
				}
			}
			indentCurrent = indentNext;
			styler.SetLevel(lineCurrent, lev);
			lineCurrent++;
		}
	}
}


static const char * const e8WordListDesc[] = {
	"Keywords",
	"user1",
	"user2",
	"user3",
	0
};

LexerModule lmE8Script(SCLEX_E8Script, ColouriseE8Doc, "e8s", FoldE8Doc, e8WordListDesc);
