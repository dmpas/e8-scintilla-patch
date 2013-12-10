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

/*! Предельный размер идентификатора в байтах (количество символов *2) */
#define E8_MAX_IDENTIFIER_SIZE_BYTES 1000

static inline bool IsAWordChar(int ch) {

	if (ch >= 0x400 && ch < 0x500)
		return true;

	return ch >= 0x80 ||
	       (isalnum(ch) /*|| ch == '.'*/ || ch == '_');
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

static inline unsigned int code_point(int c1, int c2)
{
	return ((c1 << 6) & 0x7ff) + ((c2) & 0x3f);
}

static inline void decode_point(unsigned cp, int &c1, int &c2)
{
	c1 = ((cp >> 6) & 0x1F) | 0xC0;
	c2 = (cp & 0x3F) | 0x80;
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
				char s[E8_MAX_IDENTIFIER_SIZE_BYTES];
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
			} else if (isoperator(static_cast<char>(sc.ch)) || (sc.ch == '\\') || (sc.ch == '.')) {	// Integer division
				sc.SetState(SCE_E8_OPERATOR);
			}
		}

	}

	if (sc.state == SCE_E8_IDENTIFIER && !IsAWordChar(sc.ch)) {
		char s[E8_MAX_IDENTIFIER_SIZE_BYTES];
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

static void GetRangeLowered(unsigned int start,
		unsigned int end,
		Accessor &styler,
		char *s,
		unsigned int len) {
	unsigned int i = 0;
	while ((i < end - start + 1) && (i < len-1)) {
		s[i] = static_cast<char>(tolower(styler[start + i]));
		i++;
	}
	s[i] = '\0';
}

static void GetForwardRangeLowered(unsigned int start,
		CharacterSet &charSet,
		Accessor &styler,
		char *s,
		unsigned int len) {
	unsigned int i = 0;
	/*
	while ((i < len-1) && charSet.Contains(styler.SafeGetCharAt(start + i))) {
		s[i] = static_cast<char>(tolower(styler.SafeGetCharAt(start + i)));
		i++;
	}*/
	while ((i < len-1)) {
		int cc = styler.SafeGetCharAt(start + i);
		
		if (cc < 0x80) {
			cc = tolower(cc);
		} else {
			int c2 = styler.SafeGetCharAt(start + i + 1);
			unsigned cp = code_point(cc, c2);
			cp = bukvaLower(cp);
			decode_point(cp, cc, c2);
			s[i++] = cc;
			s[i] = c2;
		}
		
		++i;
	}
	s[i] = '\0';

}

enum {
	stateInAsm = 0x1000,
	stateInProperty = 0x2000,
	stateInExport = 0x4000,
	stateFoldInPreprocessor = 0x0100,
	stateFoldInRecord = 0x0200,
	stateFoldInPreprocessorLevelMask = 0x00FF,
	stateFoldMaskAll = 0x0FFF
};


static bool IsStreamCommentStyle(int style) {
	return style == SCE_E8_MULTYLINE_COMMENT;
}

static bool IsStreamDocStyle(int style) {
	return style == SCE_E8_MULTYLINE_DOC;
}

static bool IsCommentLine(int line, Accessor &styler) {
	int pos = styler.LineStart(line);
	int eolPos = styler.LineStart(line + 1) - 1;
	for (int i = pos; i < eolPos; i++) {
		char ch = styler[i];
		char chNext = styler.SafeGetCharAt(i + 1);
		char ch2 = styler.SafeGetCharAt(i + 2);
		int style = styler.StyleAt(i);
		if (ch == '/' && chNext == '/' && ch2 != '*' && ch2 != '/' && ch2 != '!' 
			&& (style == SCE_E8_COMMENT)) {
			return true;
		} else if (!IsASpaceOrTab(ch)) {
			return false;
		}
	}
	return false;
}
static bool IsDocLine(int line, Accessor &styler)
{
	int pos = styler.LineStart(line);
	int eolPos = styler.LineStart(line + 1) - 1;
	for (int i = pos; i < eolPos; i++) {
		char ch = styler[i];
		char chNext = styler.SafeGetCharAt(i + 1);
		char ch2 = styler.SafeGetCharAt(i + 2);
		int style = styler.StyleAt(i);
		if (ch == '/' && chNext == '/' && (ch2 == '*' || ch2 == '/' || ch2 == '!')
			&& (style == SCE_E8_DOC)) {
			return true;
		} else if (!IsASpaceOrTab(ch)) {
			return false;
		}
	}
	return false;
}

static unsigned int SkipWhiteSpace(unsigned int currentPos, unsigned int endPos,
		Accessor &styler, bool includeChars = false) {
	CharacterSet setWord(CharacterSet::setAlphaNum, "_");
	unsigned int j = currentPos + 1;
	char ch = styler.SafeGetCharAt(j);
	while ((j < endPos) && (IsASpaceOrTab(ch) || ch == '\r' || ch == '\n' ||
		IsStreamCommentStyle(styler.StyleAt(j)) || (includeChars && setWord.Contains(ch)))
		|| IsStreamDocStyle(styler.StyleAt(j))
		) {
		j++;
		ch = styler.SafeGetCharAt(j);
	}
	return j;
}

static void ClassifyPascalWordFoldPoint(int &levelCurrent, int &lineFoldStateCurrent,
		int startPos, unsigned int endPos,
		unsigned int lastStart, unsigned int currentPos, Accessor &styler) {
	char s[100];
	GetRangeLowered(lastStart, currentPos, styler, s, sizeof(s));

	if (strcmp(s, "record") == 0) {
		lineFoldStateCurrent |= stateFoldInRecord;
		levelCurrent++;
	} else if (strcmp(s, "begin") == 0 ||
		strcmp(s, "asm") == 0 ||
		strcmp(s, "try") == 0 ||
		(strcmp(s, "case") == 0 && !(lineFoldStateCurrent & stateFoldInRecord))) {
		levelCurrent++;
	} else if (strcmp(s, "class") == 0 || strcmp(s, "object") == 0) {
		// "class" & "object" keywords require special handling...
		bool ignoreKeyword = false;
		unsigned int j = SkipWhiteSpace(currentPos, endPos, styler);
		if (j < endPos) {
			CharacterSet setWordStart(CharacterSet::setAlpha, "_");
			CharacterSet setWord(CharacterSet::setAlphaNum, "_");

			if (styler.SafeGetCharAt(j) == ';') {
				// Handle forward class declarations ("type TMyClass = class;")
				// and object method declarations ("TNotifyEvent = procedure(Sender: TObject) of object;")
				ignoreKeyword = true;
			} else if (strcmp(s, "class") == 0) {
				// "class" keyword has a few more special cases...
				if (styler.SafeGetCharAt(j) == '(') {
					// Handle simplified complete class declarations ("type TMyClass = class(TObject);")
					j = SkipWhiteSpace(j, endPos, styler, true);
					if (j < endPos && styler.SafeGetCharAt(j) == ')') {
						j = SkipWhiteSpace(j, endPos, styler);
						if (j < endPos && styler.SafeGetCharAt(j) == ';') {
							ignoreKeyword = true;
						}
					}
				} else if (setWordStart.Contains(styler.SafeGetCharAt(j))) {
					char s2[11];	// Size of the longest possible keyword + one additional character + null
					GetForwardRangeLowered(j, setWord, styler, s2, sizeof(s2));

					if (strcmp(s2, "procedure") == 0 ||
						strcmp(s2, "function") == 0 ||
						strcmp(s2, "of") == 0 ||
						strcmp(s2, "var") == 0 ||
						strcmp(s2, "property") == 0 ||
						strcmp(s2, "operator") == 0) {
						ignoreKeyword = true;
					}
				}
			}
		}
		if (!ignoreKeyword) {
			levelCurrent++;
		}
	} else if (strcmp(s, "interface") == 0) {
		// "interface" keyword requires special handling...
		bool ignoreKeyword = true;
		int j = lastStart - 1;
		char ch = styler.SafeGetCharAt(j);
		while ((j >= startPos) && (IsASpaceOrTab(ch) || ch == '\r' || ch == '\n' ||
			IsStreamCommentStyle(styler.StyleAt(j)))) {
			j--;
			ch = styler.SafeGetCharAt(j);
		}
		if (j >= startPos && styler.SafeGetCharAt(j) == '=') {
			ignoreKeyword = false;
		}
		if (!ignoreKeyword) {
			unsigned int k = SkipWhiteSpace(currentPos, endPos, styler);
			if (k < endPos && styler.SafeGetCharAt(k) == ';') {
				// Handle forward interface declarations ("type IMyInterface = interface;")
				ignoreKeyword = true;
			}
		}
		if (!ignoreKeyword) {
			levelCurrent++;
		}
	} else if (strcmp(s, "dispinterface") == 0) {
		// "dispinterface" keyword requires special handling...
		bool ignoreKeyword = false;
		unsigned int j = SkipWhiteSpace(currentPos, endPos, styler);
		if (j < endPos && styler.SafeGetCharAt(j) == ';') {
			// Handle forward dispinterface declarations ("type IMyInterface = dispinterface;")
			ignoreKeyword = true;
		}
		if (!ignoreKeyword) {
			levelCurrent++;
		}
	} else if (strcmp(s, "end") == 0) {
		lineFoldStateCurrent &= ~stateFoldInRecord;
		levelCurrent--;
		if (levelCurrent < SC_FOLDLEVELBASE) {
			levelCurrent = SC_FOLDLEVELBASE;
		}
	}
}


static void FoldE8Doc(unsigned int startPos, int length, int initStyle,
						   WordList *[], Accessor &styler)
{

	bool foldComment = styler.GetPropertyInt("fold.comment") != 0;
	bool foldPreprocessor = styler.GetPropertyInt("fold.preprocessor") != 0;
	bool foldCompact = styler.GetPropertyInt("fold.compact", 1) != 0;
	unsigned int endPos = startPos + length;
	int visibleChars = 0;
	int lineCurrent = styler.GetLine(startPos);
	int levelPrev = styler.LevelAt(lineCurrent) & SC_FOLDLEVELNUMBERMASK;
	int levelCurrent = levelPrev;
	int lineFoldStateCurrent = lineCurrent > 0 ? styler.GetLineState(lineCurrent - 1) & stateFoldMaskAll : 0;
	char chNext = styler[startPos];
	int styleNext = styler.StyleAt(startPos);
	int style = initStyle;

	int lastStart = 0;
	
	bool foldDoc = foldComment;
	
	for (unsigned int i = startPos; i < endPos; i++) {
		char ch = chNext;
		chNext = styler.SafeGetCharAt(i + 1);
		int stylePrev = style;
		style = styleNext;
		styleNext = styler.StyleAt(i + 1);
		bool atEOL = (ch == '\r' && chNext != '\n') || (ch == '\n');

		if (foldComment && IsStreamCommentStyle(style)) {
			if (!IsStreamCommentStyle(stylePrev)) {
				levelCurrent++;
			} else if (!IsStreamCommentStyle(styleNext) && !atEOL) {
				// Comments don't end at end of line and the next character may be unstyled.
				levelCurrent--;
			}
		}
		if (foldComment && atEOL && IsCommentLine(lineCurrent, styler))
		{
			if (!IsCommentLine(lineCurrent - 1, styler)
			    && IsCommentLine(lineCurrent + 1, styler))
				levelCurrent++;
			else if (IsCommentLine(lineCurrent - 1, styler)
			         && !IsCommentLine(lineCurrent+1, styler))
				levelCurrent--;
		}
		if (foldDoc && IsStreamDocStyle(style)) {
			if (!IsStreamDocStyle(stylePrev)) {
				levelCurrent++;
			} else if (!IsStreamDocStyle(styleNext) && !atEOL) {
				// Comments don't end at end of line and the next character may be unstyled.
				levelCurrent--;
			}
		}
		if (foldDoc && atEOL && IsDocLine(lineCurrent, styler))
		{
			if (!IsDocLine(lineCurrent - 1, styler)
			    && IsDocLine(lineCurrent + 1, styler))
				levelCurrent++;
			else if (IsDocLine(lineCurrent - 1, styler)
			         && !IsDocLine(lineCurrent+1, styler))
				levelCurrent--;
		}
		if (!IsASpace(ch))
			visibleChars++;

		if (atEOL) {
			int lev = levelPrev;
			/*
			if (visibleChars == 0 && foldCompact)
				lev |= SC_FOLDLEVELWHITEFLAG;
			*/
			if ((levelCurrent > levelPrev) && (visibleChars > 0))
				lev |= SC_FOLDLEVELHEADERFLAG;
			if (lev != styler.LevelAt(lineCurrent)) {
				styler.SetLevel(lineCurrent, lev);
			}
			int newLineState = (styler.GetLineState(lineCurrent) & ~stateFoldMaskAll) | lineFoldStateCurrent;
			styler.SetLineState(lineCurrent, newLineState);
			lineCurrent++;
			levelPrev = levelCurrent;
			visibleChars = 0;
		}
	}

	// If we didn't reach the EOL in previous loop, store line level and whitespace information.
	// The rest will be filled in later...
	int lev = levelPrev;
	/*
	if (visibleChars == 0 && foldCompact)
		lev |= SC_FOLDLEVELWHITEFLAG;
	*/
	styler.SetLevel(lineCurrent, lev);
	
}


static const char * const e8WordListDesc[] = {
	"Keywords",
	"user1",
	"user2",
	"user3",
	0
};

LexerModule lmE8Script(SCLEX_E8Script, ColouriseE8Doc, "e8s", FoldE8Doc, e8WordListDesc);

#undef E8_MAX_IDENTIFIER_SIZE_BYTES