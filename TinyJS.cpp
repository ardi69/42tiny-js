/*
 * TinyJS
 *
 * A single-file Javascript-alike engine
 *
 * Authored By Gordon Williams <gw@pur3.co.uk>
 *
 * Copyright (C) 2009 Pur3 Ltd
 *

 * 42TinyJS
 *
 * A fork of TinyJS with the goal to makes a more JavaScript/ECMA compliant engine
 *
 * Authored / Changed By Armin Diedering <armin@diedering.de>
 *
 * Copyright (C) 2010-2015 ardisoft
 *
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
 * of the Software, and to permit persons to whom the Software is furnished to do
 * so, subject to the following conditions:

 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.

 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#ifdef _DEBUG
#	ifndef _MSC_VER
#		define DEBUG_MEMORY 1
#	endif
#endif
#include <errno.h>
#include <sstream>
#include <fstream>
#include <iomanip>
#include <iterator>
#include <array>
#include <charconv>
#include <regex>
#include <algorithm>
#include <cmath>
#include <memory>
#include <sys/stat.h>
#include "TinyJS.h"

#ifndef ASSERT
#	define ASSERT(X) assert(X)
#endif


// -----------------------------------------------------------------------------------
//////////////////////////////////////////////////////////////////////////
/// Memory Debug
//////////////////////////////////////////////////////////////////////////

//#define DEBUG_MEMORY 1

#if DEBUG_MEMORY

vector<CScriptVar*> allocatedVars;
vector<CScriptVarLink*> allocatedLinks;

void mark_allocated(CScriptVar *v) {
	allocatedVars.push_back(v);
}

void mark_deallocated(CScriptVar *v) {
	for (size_t i=0;i<allocatedVars.size();i++) {
		if (allocatedVars[i] == v) {
			allocatedVars.erase(allocatedVars.begin()+i);
			break;
		}
	}
}

void mark_allocated(CScriptVarLink *v) {
	allocatedLinks.push_back(v);
}

void mark_deallocated(CScriptVarLink *v) {
	for (size_t i=0;i<allocatedLinks.size();i++) {
		if (allocatedLinks[i] == v) {
			allocatedLinks.erase(allocatedLinks.begin()+i);
			break;
		}
	}
}

void show_allocated() {
	for (size_t i=0;i<allocatedVars.size();i++) {
		printf("ALLOCATED, %d refs\n", allocatedVars[i]->getRefs());
		allocatedVars[i]->trace("  ");
	}
	for (size_t i=0;i<allocatedLinks.size();i++) {
		printf("ALLOCATED LINK %s, allocated[%d] to \n", allocatedLinks[i]->getName().c_str(), allocatedLinks[i]->getVarPtr()->getRefs());
		allocatedLinks[i]->getVarPtr()->trace("  ");
	}
	allocatedVars.clear();
	allocatedLinks.clear();
}
#endif


//////////////////////////////////////////////////////////////////////////
/// Utils
//////////////////////////////////////////////////////////////////////////

inline bool isWhitespace(char ch) {
	return (ch==' ') || (ch=='\t') || (ch=='\n') || (ch=='\r');
}

inline bool isNumeric(char ch) {
	return (ch>='0') && (ch<='9');
}

uint32_t isArrayIndex(std::string_view str) {
	if(str.empty() || (str.length()>1 && str.front() == '0') || str.length() > 10 || (str.length()==10 && str > "4294967294")) return -1; // empty or (more as 1 digit and beginning with '0')
	uint32_t result = 0;
	while(str.length()) {
		if (!isNumeric(str.front())) return -1;
		result = result * 10 + (str.front() - '0');
		str.remove_prefix(1);
	}
	return result;
}
inline bool isHexadecimal(char ch) {
	return ((ch>='0') && (ch<='9')) || ((ch>='a') && (ch<='f')) || ((ch>='A') && (ch<='F'));
}
inline bool isOctal(char ch) {
	return ((ch>='0') && (ch<='7'));
}
inline bool isIDStartChar(char ch) {
	return ((ch>='a') && (ch<='z')) || ((ch>='A') && (ch<='Z')) || ch=='_' || ch=='$';
}
inline bool isIDChar(char ch) {
	return isIDStartChar(ch) || isNumeric(ch);
}

bool isIDString(const char *s) {
	if (!isIDStartChar(*s))
		return false;
	while (*s) {
		if (!isIDChar(*s))
			return false;
		s++;
	}
	return true;
}

void replace(std::string &str, char textFrom, const char *textTo) {
	size_t sLen = strlen(textTo);
	size_t p = str.find(textFrom);
	while (p != std::string::npos) {
		str = str.substr(0, p) + textTo + str.substr(p+1);
		p = str.find(textFrom, p+sLen);
	}
}
	std::string float2string(const double &floatData) {
	std::ostringstream str;
	str.unsetf(std::ios::floatfield);
#if (defined(_MSC_VER) && _MSC_VER >= 1600) || __cplusplus >= 201103L
	str.precision(std::numeric_limits<double>::max_digits10);
#else
	str.precision(numeric_limits<double>::digits10+2);
#endif
	str << floatData;
	return str.str();
}
#define size2int32(size) ((int32_t)(size > INT32_MAX ? INT32_MAX : size))

/// convert the given std::string into a quoted std::string suitable for javascript
std::string getJSString(const std::string &str) {
	char buffer[5] = "\\x00";
	std::string nStr; nStr.reserve(str.length());
	nStr.push_back('\"');
	for (std::string::const_iterator i=str.begin();i!=str.end();i++) {
		const char *replaceWith = 0;
		switch (*i) {
			case '\\': replaceWith = "\\\\"; break;
			case '\n': replaceWith = "\\n"; break;
			case '\r': replaceWith = "\\r"; break;
			case '\a': replaceWith = "\\a"; break;
			case '\b': replaceWith = "\\b"; break;
			case '\f': replaceWith = "\\f"; break;
			case '\t': replaceWith = "\\t"; break;
			case '\v': replaceWith = "\\v"; break;
			case '"': replaceWith = "\\\""; break;
			default: {
					int nCh = ((unsigned char)*i) & 0xff;
					if(nCh<32 || nCh>127) {
						static char hex[] = "0123456789ABCDEF";
						buffer[2] = hex[(nCh>>4)&0x0f];
						buffer[3] = hex[nCh&0x0f];
						replaceWith = buffer;
					};
				}
		}
		if (replaceWith)
			nStr.append(replaceWith);
		else
			nStr.push_back(*i);
	}
	nStr.push_back('\"');
	return nStr;
}

static inline std::string getIDString(const std::string& str) {
	if(isIDString(str.c_str()) && CScriptToken::isReservedWord(str)==LEX_ID)
		return str;
	return getJSString(str);
}


//////////////////////////////////////////////////////////////////////////
/// CScriptException
//////////////////////////////////////////////////////////////////////////

std::string CScriptException::toString() {
	std::ostringstream msg;
	msg << ERROR_NAME[errorType] << ": " << message;
	if(lineNumber >= 0) msg << " at Line:" << lineNumber+1;
	if(column >=0) msg << " Column:" << column+1;
	if(fileName.length()) msg << " in " << fileName;
	return msg.str();
}


//////////////////////////////////////////////////////////////////////////
/// CScriptLex
//////////////////////////////////////////////////////////////////////////

CScriptLex::CScriptLex(const char *Code, const std::string &File, int Line, int Column) : data(Code) {
	currentFile = File;
	pos.currentLineStart = pos.tokenStart = data;
	pos.currentLine = Line;
	reset(pos);
}

void CScriptLex::reset(const POS &toPos) { ///< Reset this lex so we can start again
	dataPos = toPos.tokenStart;
	tk = last_tk = 0;
	tkStr = "";
	pos = toPos;
	lineBreakBeforeToken = false;
	currCh = nextCh = 0;
	getNextCh(); // currCh
	getNextCh(); // nextCh
	while (!getNextToken()); // instead recursion
}

void CScriptLex::check(int expected_tk, int alternate_tk/*=-1*/) {
	if (expected_tk==';' && tk==LEX_EOF) return; // ignore last missing ';'
	if (tk!=expected_tk && tk!=alternate_tk) {
		std::ostringstream errorString;
		if(expected_tk == LEX_EOF)
			errorString << "Got unexpected " << CScriptToken::getTokenStr(tk);
		else
			errorString << "Got '" << CScriptToken::getTokenStr(tk, tkStr.c_str()) << "' expected '" << CScriptToken::getTokenStr(expected_tk) << "'";
		if(alternate_tk!=-1) errorString << " or '" << CScriptToken::getTokenStr(alternate_tk) << "'";
		throw CScriptException(SyntaxError, errorString.str(), currentFile, pos.currentLine, currentColumn());
	}
}
void CScriptLex::match(int expected_tk1, int alternate_tk/*=-1*/) {
	check(expected_tk1, alternate_tk);
	int line = pos.currentLine;
	while (!getNextToken()); // instead recursion
	lineBreakBeforeToken = line != pos.currentLine;
	if(pos.tokenStart-pos.currentLineStart > INT16_MAX) {
		throw CScriptException(Error, "Maximum line length (of 32767 characters) exhausted", currentFile, pos.currentLine, int32_t(pos.tokenStart-pos.currentLineStart));
	}
}

void CScriptLex::getNextCh() {
	if(currCh == '\n') { // Windows or Linux
		pos.currentLine++;
		pos.tokenStart = pos.currentLineStart = dataPos - (nextCh == LEX_EOF ?  0 : 1);
	}
	currCh = nextCh;
	if ( (nextCh = *dataPos) != LEX_EOF ) dataPos++; // stay on EOF
//	if(nextCh == -124) nextCh = '\n'; //
	if(currCh == '\r') { // Windows or Mac
		if(nextCh == '\n')
			getNextCh(); // Windows '\r\n\' --> skip '\r'
		else
			currCh = '\n'; // Mac (only '\r') --> convert '\r' to '\n'
	}
}

static uint16_t not_allowed_tokens_befor_regexp[] = {LEX_ID, LEX_INT, LEX_FLOAT, LEX_STR, LEX_R_TRUE, LEX_R_FALSE, LEX_R_NULL, ']', ')', '.', LEX_PLUSPLUS, LEX_MINUSMINUS, LEX_EOF};
bool CScriptLex::getNextToken()
{
	while (currCh && isWhitespace(currCh)) getNextCh();
	// newline comments
	if (currCh=='/' && nextCh=='/') {
		while (currCh && currCh!='\n') getNextCh();
		getNextCh();
		//getNextToken();
		return false;
	}
	// block comments
	if (currCh=='/' && nextCh=='*') {
		int nested=0;
		do {
			if (currCh=='/' && nextCh=='*') {
				getNextCh(); // skip '/'
				getNextCh(); // skip '*'
				++nested;
			} else if (currCh=='*' && nextCh=='/') {
				getNextCh(); // skip '*'
				getNextCh(); // skip '/'
				--nested;
			} else
				getNextCh();
		} while (currCh && nested);
		//getNextToken();
		return false;
	}
	last_tk = tk;
	tk = LEX_EOF;
	tkStr.clear();
	// record beginning of this token
	pos.tokenStart = dataPos - (nextCh == LEX_EOF ? (currCh == LEX_EOF ? 0 : 1) : 2);
	// tokens

	if (isIDStartChar(currCh)) { //  IDs
		while (currCh >= 'a' && currCh <= 'z') {
			tkStr += currCh;
			getNextCh();
		}
		if (!isIDChar(currCh)) {
			tk = CScriptToken::isReservedWord(tkStr);
#ifdef NO_GENERATORS
			if (tk == LEX_R_YIELD)
				throw CScriptException(Error, "42TinyJS was built without support of generators (yield expression)", currentFile, pos.currentLine, currentColumn());
#endif
		} else {
			while (isIDChar(currCh)) {
				tkStr += currCh;
				getNextCh();
			}
			tk = LEX_ID;
		}

	} else if (isNumeric(currCh) || (currCh=='.' && isNumeric(nextCh))) { // Numbers
		if(currCh=='.') tkStr+='0';
		bool isHex = false, isOct=false;
		if (currCh=='0') {
			tkStr += currCh; getNextCh();
			//if(isOctal(currCh)) isOct = true;
		}
		if (currCh == 'o' || currCh == 'O') {
			isOct = true;
			tkStr += currCh; getNextCh();
		} else if (currCh == 'x' || currCh == 'X') {
			isHex = true;
			tkStr += currCh; getNextCh();
		}
		tk = LEX_INT;
		while (isOctal(currCh) || (!isOct && isNumeric(currCh)) || (isHex && isHexadecimal(currCh))) {
			tkStr += currCh;
			getNextCh();
		}
		if (!isHex && !isOct && currCh=='.') {
			tk = LEX_FLOAT;
			tkStr += '.';
			getNextCh();
			while (isNumeric(currCh)) {
				tkStr += currCh;
				getNextCh();
			}
		}
		// do fancy e-style floating point
		if (!isHex && !isOct && (currCh=='e' || currCh=='E')) {
			tk = LEX_FLOAT;
			tkStr += currCh; getNextCh();
			if (currCh=='-') { tkStr += currCh; getNextCh(); }
			while (isNumeric(currCh)) {
				tkStr += currCh; getNextCh();
			}
		}
	} else if (currCh=='"' || currCh=='\'') {	// strings...
		char endCh = currCh;
		getNextCh();
		while (currCh && currCh!=endCh && currCh!='\n') {
			if (currCh == '\\') {
				getNextCh();
				switch (currCh) {
					case '\n' : break; // ignore newline after '\'
					case 'n': tkStr += '\n'; break;
					case 'r': tkStr += '\r'; break;
					case 'a': tkStr += '\a'; break;
					case 'b': tkStr += '\b'; break;
					case 'f': tkStr += '\f'; break;
					case 't': tkStr += '\t'; break;
					case 'v': tkStr += '\v'; break;
					case 'x': { // hex digits
						getNextCh();
						if(isHexadecimal(currCh)) {
							char buf[3]="\0\0";
							buf[0] = currCh;
							for(int i=0; i<2 && isHexadecimal(nextCh); i++) {
								getNextCh(); buf[i] = currCh;
							}
							tkStr += (char)strtol(buf, 0, 16);
						} else
							throw CScriptException(SyntaxError, "malformed hexadezimal character escape sequence", currentFile, pos.currentLine, currentColumn());
						[[fallthrough]];
					}
					default: {
						if(isOctal(currCh)) {
							char buf[4]="\0\0\0";
							buf[0] = currCh;
							for(int i=1; i<3 && isOctal(nextCh); i++) {
								getNextCh(); buf[i] = currCh;
							}
							tkStr += (char)strtol(buf, 0, 8);
						}
						else tkStr += currCh;
					}
				}
			} else {
				tkStr += currCh;
			}
			getNextCh();
		}
		if(currCh != endCh)
			throw CScriptException(SyntaxError, "unterminated std::string literal", currentFile, pos.currentLine, currentColumn());
		getNextCh();
		tk = LEX_STR;
	} else {
		// single chars
		tk = currCh;
		if (currCh) getNextCh();
		if (tk=='=') {
			if(currCh=='=') { // ==
				tk = LEX_EQUAL;
				getNextCh();
				if (currCh=='=') { // ===
					tk = LEX_TYPEEQUAL;
					getNextCh();
				}
			} else if(currCh =='>') { // =>
				tk = LEX_ARROW;
				getNextCh();
			}
		} else if (tk=='!' && currCh=='=') { // !=
			tk = LEX_NEQUAL;
			getNextCh();
			if (currCh=='=') { // !==
				tk = LEX_NTYPEEQUAL;
				getNextCh();
			}
		} else if (tk=='<') {
			if (currCh=='=') {	// <=
				tk = LEX_LEQUAL;
				getNextCh();
			} else if (currCh=='<') {	// <<
				tk = LEX_LSHIFT;
				getNextCh();
				if (currCh=='=') { // <<=
					tk = LEX_LSHIFTEQUAL;
					getNextCh();
				}
			}
		} else if (tk=='>') {
			if (currCh=='=') {	// >=
				tk = LEX_GEQUAL;
				getNextCh();
			} else if (currCh=='>') {	// >>
				tk = LEX_RSHIFT;
				getNextCh();
				if (currCh=='=') { // >>=
					tk = LEX_RSHIFTEQUAL;
					getNextCh();
				} else if (currCh=='>') { // >>>
					tk = LEX_RSHIFTU;
					getNextCh();
					if (currCh=='=') { // >>>=
						tk = LEX_RSHIFTUEQUAL;
						getNextCh();
					}
				}
			}
		}  else if (tk=='+') {
			if (currCh=='=') {	// +=
				tk = LEX_PLUSEQUAL;
				getNextCh();
			}  else if (currCh=='+') {	// ++
				tk = LEX_PLUSPLUS;
				getNextCh();
			}
		}  else if (tk=='-') {
			if (currCh=='=') {	// -=
				tk = LEX_MINUSEQUAL;
				getNextCh();
			}  else if (currCh=='-') {	// --
				tk = LEX_MINUSMINUS;
				getNextCh();
			}
		} else if (tk=='&') {
			if (currCh=='=') {			// &=
				tk = LEX_ANDEQUAL;
				getNextCh();
			} else if (currCh=='&') {	// &&
				tk = LEX_ANDAND;
				getNextCh();
			}
		} else if (tk=='|') {
			if (currCh=='=') {			// |=
				tk = LEX_OREQUAL;
				getNextCh();
			} else if (currCh=='|') {	// ||
				tk = LEX_OROR;
				getNextCh();
			}
		} else if (tk=='^' && currCh=='=') {
			tk = LEX_XOREQUAL;
			getNextCh();
		} else if (tk=='*' && currCh=='=') {
			tk = LEX_ASTERISKEQUAL;
			getNextCh();
		} else if (tk=='/') {
			// check if it's a RegExp-Literal
			tk = LEX_REGEXP;
			for(uint16_t *p = not_allowed_tokens_befor_regexp; *p; p++) {
				if(*p==last_tk) { tk = '/'; break; }
			}
			if(tk == LEX_REGEXP) {
#ifdef NO_REGEXP
				throw CScriptException(Error, "42TinyJS was built without support of regular expressions", currentFile, pos.currentLine, currentColumn());
#endif
				tkStr = "/";
				while (currCh && currCh!='/' && currCh!='\n') {
					if (currCh == '\\' && nextCh == '/') {
						tkStr.append(1, currCh);
						getNextCh();
					}
					tkStr.append(1, currCh);
					getNextCh();
				}
				if(currCh == '/') {
#ifndef NO_REGEXP
					try { std::regex r(tkStr.substr(1), std::regex_constants::ECMAScript); } catch(std::regex_error e) {
						throw CScriptException(SyntaxError, std::string(e.what())+" - "+CScriptVarRegExp::ErrorStr(e.code()), currentFile, pos.currentLine, currentColumn());
					}
#endif /* NO_REGEXP */
					do {
						tkStr.append(1, currCh);
						getNextCh();
					} while (currCh=='g' || currCh=='i' || currCh=='m' || currCh=='y');
				} else
					throw CScriptException(SyntaxError, "unterminated regular expression literal", currentFile, pos.currentLine, currentColumn());
			} else if(currCh=='=') {
				tk = LEX_SLASHEQUAL;
				getNextCh();
			}
		} else if (tk=='%' && currCh=='=') {
			tk = LEX_PERCENTEQUAL;
			getNextCh();
		} else if (tk == '?' && currCh == '?') {
			tk = LEX_ASKASK;
			getNextCh(); // eat '?'
			if (currCh == '=') {
				tk = LEX_ASKASKEQUAL;
				getNextCh(); // eat '='
			}
		} else if (tk == '*' && currCh == '*') {
			tk = LEX_ASTERISKASTERISK;
			getNextCh(); // eat '*'
			if (currCh == '=') {
				tk = LEX_ASTERISKASTERISKEQUAL;
				getNextCh(); // eat '='
			}
		} else if (tk == '?' && currCh == '.') {
			tk = LEX_OPTIONAL_CHAINING_MEMBER;
			getNextCh(); // eat '?'
			if (currCh == '[') {
				tk = LEX_OPTIONAL_CHAINING_ARRAY;
				getNextCh(); // eat '['
			} else if (currCh == '(') {
				tk = LEX_OPTIONAL_CHANING_FNC;
				getNextCh(); // eat '('
			}
		}
	}
	/* This isn't quite right yet */
	return true;
}


//////////////////////////////////////////////////////////////////////////
// CScriptTokenDataForwards
//////////////////////////////////////////////////////////////////////////


bool CScriptTokenDataForwards::compare_fnc_token_by_name::operator()(const CScriptToken& lhs, const CScriptToken& rhs) const {
	return lhs.Fnc()->name < rhs.Fnc()->name;
}
bool CScriptTokenDataForwards::checkRedefinition(const std::string &Str, bool checkVarsInLetScope) {
	STRING_SET_it it = varNames[LETS].find(Str);
	if(it!=varNames[LETS].end()) return false;
	else if(checkVarsInLetScope) {
		STRING_SET_it it = vars_in_letscope.find(Str);
		if(it!=vars_in_letscope.end()) return false;
	}
	return true;
}

void CScriptTokenDataForwards::addVars( STRING_VECTOR_t &Vars ) {
	varNames[VARS].insert(Vars.begin(), Vars.end());
}
void CScriptTokenDataForwards::addConsts( STRING_VECTOR_t &Vars ) {
	varNames[CONSTS].insert(Vars.begin(), Vars.end());
}
std::string CScriptTokenDataForwards::addVarsInLetscope( STRING_VECTOR_t &Vars )
{
	for(STRING_VECTOR_it it=Vars.begin(); it!=Vars.end(); ++it) {
		if(!checkRedefinition(*it, false)) return *it;
		vars_in_letscope.insert(*it);
	}
	return "";
}

std::string CScriptTokenDataForwards::addLets( STRING_VECTOR_t &Lets )
{
	for(STRING_VECTOR_it it=Lets.begin(); it!=Lets.end(); ++it) {
		if(!checkRedefinition(*it, true)) return *it;
		varNames[LETS].insert(*it);
	}
	return "";
}


//////////////////////////////////////////////////////////////////////////
// CScriptTokenDataLoop
//////////////////////////////////////////////////////////////////////////

std::string CScriptTokenDataLoop::getParsableString(const std::string &IndentString/*=""*/, const std::string &Indent/*=""*/ ) {
	static const char *heads[] = {"for each(", "for(", "for(", "for(", "while(", "do "};
	static const char *ops[] = {" in ", " in ", " of ", "; ", "", ""};
	std::string out = heads[type];
	if(init.size() && type==FOR)	out.append(CScriptToken::getParsableString(init));
	if(type<=WHILE)					out.append(CScriptToken::getParsableString(condition.begin(), condition.end()-(type>=FOR ? 0 : 7)));
	if(type<=FOR)						out.append(ops[type]);
	if(iter.size())					out.append(CScriptToken::getParsableString(iter));
	out.append(")");
	if(body.size()!=1 || body.front().token != LEX_T_ARRAY_COMPREHENSIONS_BODY)
		out.append(" ").append(CScriptToken::getParsableString(body, IndentString, Indent));
	if(type==DO)out.append(" while(").append(CScriptToken::getParsableString(condition)).append(");");
	return out;
}


//////////////////////////////////////////////////////////////////////////
// CScriptTokenDataIf
//////////////////////////////////////////////////////////////////////////

std::string CScriptTokenDataIf::getParsableString(const std::string &IndentString/*=""*/, const std::string &Indent/*=""*/ ) {
	std::string out = "if(";
	std::string nl = Indent.size() ? "\n"+IndentString : " ";
	out.append(CScriptToken::getParsableString(condition)).append(")");
	if(if_body.size()!=1 || if_body.front().token != LEX_T_ARRAY_COMPREHENSIONS_BODY)
		out.append(" ").append(CScriptToken::getParsableString(if_body, IndentString, Indent));
	if(else_body.size()) {
		char back = *out.rbegin();
		if(back != ' ' && back != '\n') out.append(" ");
		out.append("else ").append(CScriptToken::getParsableString(else_body, IndentString, Indent));
	}
	return out;
}


//////////////////////////////////////////////////////////////////////////
// CScriptTokenDataArrayComprehensionsBody
//////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////
// CScriptTokenDataTry
//////////////////////////////////////////////////////////////////////////

std::string CScriptTokenDataTry::getParsableString( const std::string &IndentString/*=""*/, const std::string &Indent/*=""*/ ) {
	std::string out = "try ";
	std::string nl = Indent.size() ? "\n"+IndentString : " ";

	out.append(CScriptToken::getParsableString(tryBlock, IndentString, Indent));
	for(CScriptTokenDataTry::CATCHBLOCKS_it catchBlock = catchBlocks.begin(); catchBlock!=catchBlocks.end(); catchBlock++) {
		out.append(nl).append("catch(").append(catchBlock->indentifiers->getParsableString());
		if(catchBlock->condition.size()>1) {
			out.append(" if ").append(CScriptToken::getParsableString(catchBlock->condition.begin()+1, catchBlock->condition.end()));
		}
		out.append(") ").append(CScriptToken::getParsableString(catchBlock->block, IndentString, Indent));
	}
	if(finallyBlock.size())
		out.append(nl).append("finally ").append(CScriptToken::getParsableString(finallyBlock, IndentString, Indent));
	return out;
}


//////////////////////////////////////////////////////////////////////////
// CScriptTokenDataString
//////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////
// CScriptTokenDataFnc
//////////////////////////////////////////////////////////////////////////


std::string CScriptTokenDataFnc::getArgumentsString( bool forArrowFunction/*=false*/ ) {
	std::ostringstream destination;
	if(!forArrowFunction || arguments.size()!=1)
		destination << "(";
	if(arguments.size()) {
		const char *comma = "";
		for(TOKEN_VECT_it argument=arguments.begin(); argument!=arguments.end(); ++argument, comma=", ") {
			if(argument->token == LEX_ID)
				destination << comma << argument->String();
			else
				destination << comma << argument->DestructuringVar()->getParsableString();
		}
	}
	if(!forArrowFunction || arguments.size()!=1)
		destination << ") ";
	else
		destination << " ";
	return destination.str();
}


//////////////////////////////////////////////////////////////////////////
// CScriptTokenDataDestructuringVar
//////////////////////////////////////////////////////////////////////////

std::string CScriptTokenDataDestructuringVar::getParsableString()
{
	std::string out;
	const char *comma = "";
	std::vector<bool> isObject(1, false);
	for(DESTRUCTURING_VARS_it it=vars.begin(); it!=vars.end(); ++it) {
		if(it->second == "}" || it->second == "]") {
			out.append(it->second);
			isObject.pop_back();
		} else {
			out.append(comma);
			if(it->second == "[" || it->second == "{") {
				comma = "";
				if(isObject.back() && it->first.length())
					out.append(getIDString(it->first)).append(":");
				out.append(it->second);
				isObject.push_back(it->second == "{");
			} else {
				comma = ", ";
				if(it->second.empty())
					continue; // skip empty entries
				if(isObject.back() && it->first!=it->second)
					out.append(getIDString(it->first)).append(":");
				out.append(it->second);
			}
		}
	}
	if(assignment.size())
		out.append("=").append(CScriptToken::getParsableString(assignment));
	return out;
}

void CScriptTokenDataDestructuringVar::getVarNames(STRING_VECTOR_t &Names) {
	for(DESTRUCTURING_VARS_it it = vars.begin(); it != vars.end(); ++it) {
		if(it->second.size() && it->second.find_first_of("{[]}") == std::string::npos)
			Names.push_back(it->second);
	}
}


//////////////////////////////////////////////////////////////////////////
// CScriptTokenDataObjectLiteral
//////////////////////////////////////////////////////////////////////////

std::string CScriptTokenDataObjectLiteral::getParsableString() {
	std::string out = type == OBJECT ? "{ " : "[";
	const char *comma = "";
	for(std::vector<ELEMENT>::iterator it=elements.begin(); it!=elements.end(); ++it) {
		out.append(comma); comma = (type <= ARRAY) ? ", " : " ";
		if(it->value.empty()) continue;
		if(type == OBJECT)
			out.append(getIDString(it->id)).append(" : ");
		out.append(CScriptToken::getParsableString(it->value));
	}
	out.append(type == OBJECT ? " }" : "]");
	return out;
}

void CScriptTokenDataObjectLiteral::setMode(bool Destructuring) {
	structuring = !(destructuring = Destructuring);
	for(std::vector<ELEMENT>::iterator it=elements.begin(); it!=elements.end(); ++it) {
		if(it->value.size() && it->value.front().token == LEX_T_OBJECT_LITERAL) {
			auto &e = it->value.front().Object();
			if(e->destructuring && e->structuring)
				e->setMode(Destructuring);
		}
	}
}

bool CScriptTokenDataObjectLiteral::toDestructuringVar(const std::shared_ptr<CScriptTokenDataDestructuringVar> &DestructuringVar ) {
	if(!destructuring) return false;
//	DestructuringVar.vars.clear();
	auto d = DestructuringVar.get();
	DestructuringVar->vars.push_back(DESTRUCTURING_VAR_t("", type == OBJECT ? "{" : "["));
	for(std::vector<ELEMENT>::iterator it=elements.begin(); it!=elements.end(); ++it) {
		if(it->value.empty()) {
			DestructuringVar->vars.push_back(DESTRUCTURING_VAR_t("",""));
			continue;
		}
		if(it->value.front().token == LEX_T_OBJECT_LITERAL) {
			DESTRUCTURING_VARS_t::size_type pos = DestructuringVar->vars.size();
			if(!it->value.front().Object()->toDestructuringVar(DestructuringVar))
				return false;
			DestructuringVar->vars[pos].first = it->id;
		} else {
			ASSERT(it->value.size()==1 && it->value.front().token==LEX_ID);
			DestructuringVar->vars.push_back(DESTRUCTURING_VAR_t(it->id,it->value.front().String()));
		}
	}
	DestructuringVar->vars.push_back(DESTRUCTURING_VAR_t("",type == OBJECT ? "}" : "]"));
	return true;
}


//////////////////////////////////////////////////////////////////////////
// CScriptToken
//////////////////////////////////////////////////////////////////////////

typedef struct { int id; const char *str; bool need_space; } token2str_t;
static size_t reserved_words_min_len = 2;
static size_t reserved_words_max_len = 10;
static token2str_t reserved_words_begin[] ={
	// reserved words
	{ LEX_R_IF,					"if",						true  },
	{ LEX_R_ELSE,				"else",						true  },
	{ LEX_R_DO,					"do",						true  },
	{ LEX_R_WHILE,				"while",					true  },
	{ LEX_R_FOR,				"for",						true  },
	{ LEX_R_IN,					"in",						true  },
	{ LEX_R_BREAK,				"break",					true  },
	{ LEX_R_CONTINUE,			"continue",					true  },
	{ LEX_R_FUNCTION,			"function",					true  },
	{ LEX_R_RETURN,				"return",					true  },
	{ LEX_R_VAR,				"var",						true  },
	{ LEX_R_LET,				"let",						true  },
	{ LEX_R_CONST,				"const",					true  },
	{ LEX_R_WITH,				"with",						true  },
	{ LEX_R_TRUE,				"true",						true  },
	{ LEX_R_FALSE,				"false",					true  },
	{ LEX_R_NULL,				"null",						true  },
	{ LEX_R_NEW,				"new",						true  },
	{ LEX_R_TRY,				"try",						true  },
	{ LEX_R_CATCH,				"catch",					true  },
	{ LEX_R_FINALLY,			"finally",					true  },
	{ LEX_R_THROW,				"throw",					true  },
	{ LEX_R_TYPEOF,				"typeof",					true  },
	{ LEX_R_VOID,				"void",						true  },
	{ LEX_R_DELETE,				"delete",					true  },
	{ LEX_R_INSTANCEOF,			"instanceof",				true  },
	{ LEX_R_SWITCH,				"switch",					true  },
	{ LEX_R_CASE,				"case",						true  },
	{ LEX_R_DEFAULT,			"default",					true  },
	{ LEX_R_YIELD,				"yield",					true  },
};
#define ARRAY_LENGTH(array) (sizeof(array)/sizeof(array[0]))
#define ARRAY_END(array) (&array[ARRAY_LENGTH(array)])
static token2str_t *reserved_words_end = ARRAY_END(reserved_words_begin);
static token2str_t *str2reserved_begin[ARRAY_LENGTH(reserved_words_begin)];
static token2str_t **str2reserved_end = ARRAY_END(str2reserved_begin);

static token2str_t tokens2str_begin[] = {
	{ LEX_EOF, 									"EOF", 										false },
	{ LEX_ID, 									"ID", 										true  },
	{ LEX_INT, 									"INT", 										true  },
	{ LEX_FLOAT, 								"FLOAT", 									true  },
	{ LEX_STR, 									"STRING", 									true  },
	{ LEX_REGEXP, 								"REGEXP", 									true  },
	{ LEX_ARROW, 								"=>", 										false },
	{ LEX_EQUAL, 								"==", 										false },
	{ LEX_TYPEEQUAL, 							"===", 										false },
	{ LEX_NEQUAL, 								"!=", 										false },
	{ LEX_NTYPEEQUAL, 							"!==", 										false },
	{ LEX_LEQUAL, 								"<=", 										false },
	{ LEX_LSHIFT, 								"<<", 										false },
	{ LEX_LSHIFTEQUAL, 							"<<=", 										false },
	{ LEX_GEQUAL, 								">=", 										false },
	{ LEX_RSHIFT, 								">>", 										false },
	{ LEX_RSHIFTEQUAL, 							">>=", 										false },
	{ LEX_RSHIFTU, 								">>>", 										false },
	{ LEX_RSHIFTUEQUAL, 						">>>=", 									false },
	{ LEX_PLUSEQUAL, 							"+=", 										false },
	{ LEX_MINUSEQUAL, 							"-=", 										false },
	{ LEX_PLUSPLUS, 							"++", 										false },
	{ LEX_MINUSMINUS, 							"--", 										false },
	{ LEX_ANDEQUAL, 							"&=", 										false },
	{ LEX_ANDAND, 								"&&", 										false },
	{ LEX_OREQUAL, 								"|=", 										false },
	{ LEX_OROR, 								"||", 										false },
	{ LEX_XOREQUAL, 							"^=", 										false },
	{ LEX_ASTERISKEQUAL, 						"*=", 										false },
	{ LEX_SLASHEQUAL, 							"/=", 										false },
	{ LEX_PERCENTEQUAL, 						"%=", 										false },
	{ LEX_ASKASK, 								"??", 										false },
	{ LEX_ASKASKEQUAL, 							"??=", 										false },
	{ LEX_ASTERISKASTERISK, 					"**", 										false },
	{ LEX_ASTERISKASTERISKEQUAL, 				"**=", 										false },

	{ LEX_OPTIONAL_CHAINING_MEMBER, 			"?.", 										false },
	{ LEX_OPTIONAL_CHAINING_ARRAY, 				"?.[", 										false },
	{ LEX_OPTIONAL_CHANING_FNC, 				"?.(", 										false },

	// special tokens
	{ LEX_T_OF, 								"of", 										true  },
	{ LEX_T_FUNCTION_OPERATOR, 					"function", 								true  },
	{ LEX_T_GENERATOR, 							"function*", 								true  },
	{ LEX_T_GENERATOR_OPERATOR, 				"function*", 								true  },
	{ LEX_T_GENERATOR_MEMBER, 					"*",		 								true  },
	{ LEX_T_GET, 								"get", 										true  },
	{ LEX_T_SET, 								"set", 										true  },
	{ LEX_T_EXCEPTION_VAR, 						"LEX_T_EXCEPTION_VAR",	 					false },
	{ LEX_T_SKIP, 								"LEX_T_SKIP", 								false },
	{ LEX_T_END_EXPRESSION, 					"LEX_T_END_EXPRESSION",	 					false },
	{ LEX_T_DUMMY_LABEL, 						"LABEL", 									true  },
	{ LEX_T_LABEL, 								"LABEL", 									true  },
	{ LEX_T_LOOP, 								"LEX_T_LOOP", 								true  },
	{ LEX_T_FOR_IN, 							"LEX_FOR_IN", 								true  },
	{ LEX_T_IF, 								"if", 										true  },
	{ LEX_T_FORWARD, 							"LEX_T_FORWARD", 							false },
	{ LEX_T_OBJECT_LITERAL, 					"LEX_T_OBJECT_LITERAL",	 					false },
	{ LEX_T_ARRAY_COMPREHENSIONS_BODY,			"LEX_T_ARRAY_COMPREHENSIONS_BODY",			false },
	{ LEX_T_DESTRUCTURING_VAR, 					"Destructuring Var", 						false },
};
static token2str_t *tokens2str_end = &tokens2str_begin[sizeof(tokens2str_begin)/sizeof(tokens2str_begin[0])];
struct token2str_cmp_t {
	bool operator()(const token2str_t &lhs, const token2str_t &rhs) {
		return lhs.id < rhs.id;
	}
	bool operator()(const token2str_t &lhs, int rhs) {
		return lhs.id < rhs;
	}
	bool operator()(const token2str_t *lhs, const token2str_t *rhs) {
		return strcmp(lhs->str, rhs->str)<0;
	}
	bool operator()(const token2str_t *lhs, const char *rhs) {
		return strcmp(lhs->str, rhs)<0;
	}
};
static bool tokens2str_sort() {
//	printf("tokens2str_sort called\n");
	std::sort(tokens2str_begin, tokens2str_end, token2str_cmp_t());
	std::sort(reserved_words_begin, reserved_words_end, token2str_cmp_t());
	for(unsigned int i=0; i<ARRAY_LENGTH(str2reserved_begin); i++)
		str2reserved_begin[i] = &reserved_words_begin[i];
	std::sort(str2reserved_begin, str2reserved_end, token2str_cmp_t());
	return true;
}
static bool tokens2str_sorted = tokens2str_sort();

CScriptToken::CScriptToken(CScriptLex *l, int Match, int Alternate) : line(l->currentLine()), column(l->currentColumn()), token(l->tk)/*, data(0) needed??? */
{
	if(token == LEX_INT || LEX_TOKEN_DATA_FLOAT(token)) {
		CNumber number(l->tkStr);
		if (number.isInfinity())
			token = LEX_ID, data = CScriptTokenDataString::create("Infinity");
		else if (number.isInt32())
			token = LEX_INT, data = number.toInt32();
		else
			token = LEX_FLOAT, data = number.toDouble();
	} else if(LEX_TOKEN_DATA_STRING(token))
		data = CScriptTokenDataString::create(l->tkStr);
	else if(LEX_TOKEN_DATA_FUNCTION(token))
		data = CScriptTokenDataFnc::create(token);
	else if (LEX_TOKEN_DATA_LOOP(token))
		data = CScriptTokenDataLoop::create();
	else if (LEX_TOKEN_DATA_IF(token))
		data = CScriptTokenDataIf::create();
	else if (LEX_TOKEN_DATA_TRY(token))
		data = CScriptTokenDataTry::create();
	if (Match >= 0)
		l->match(Match, Alternate);
	else
		l->match(l->tk);
#ifdef _DEBUG
	token_str = getTokenStr(token);
#endif
}
CScriptToken::CScriptToken(uint16_t Tk, int32_t IntData) : line(0), column(0), token(Tk)/*, data(0) needed??? */ {
	if (LEX_TOKEN_DATA_SIMPLE(token))
		data = IntData;
	else if (LEX_TOKEN_DATA_FUNCTION(token))
		data = CScriptTokenDataFnc::create(token);
	else if (LEX_TOKEN_DATA_DESTRUCTURING_VAR(token))
		data = CScriptTokenDataDestructuringVar::create();
	else if (LEX_TOKEN_DATA_OBJECT_LITERAL(token))
		data = CScriptTokenDataObjectLiteral::create();
	else if (LEX_TOKEN_DATA_ARRAY_COMPREHENSIONS_BODY(token))
		data = CScriptTokenDataArrayComprehensionsBody::create();
	else if (LEX_TOKEN_DATA_LOOP(token))
		data = CScriptTokenDataLoop::create();
	else if (LEX_TOKEN_DATA_IF(token))
		data = CScriptTokenDataIf::create();
	else if (LEX_TOKEN_DATA_TRY(token))
		data = CScriptTokenDataTry::create();
	else if (LEX_TOKEN_DATA_FORWARDER(token))
		data = CScriptTokenDataForwards::create();
	else
		ASSERT(0);
#ifdef _DEBUG
	token_str = getTokenStr(token);
#endif
}

CScriptToken::CScriptToken(uint16_t Tk, double FloatData) : line(0), column(0), token(Tk)/*, data(0) needed??? */ {
	if (token == LEX_FLOAT)
		data= FloatData;
	else
		ASSERT(0);
#ifdef _DEBUG
	token_str = getTokenStr(token);
#endif
}

CScriptToken::CScriptToken(uint16_t Tk, const std::string &TkStr) : line(0), column(0), token(Tk)/*, data(0) needed??? */ {
	if(LEX_TOKEN_DATA_STRING(token))
		data = CScriptTokenDataString::create(TkStr);
	else if (LEX_TOKEN_DATA_DESTRUCTURING_VAR(token)) {
		auto tmp = CScriptTokenDataDestructuringVar::create();
		tmp->vars.push_back(DESTRUCTURING_VAR_t("", TkStr));
		data = tmp;
	} else
		ASSERT(0);

#ifdef _DEBUG
	token_str = getTokenStr(token);
#endif
}


std::string CScriptToken::getParsableString(TOKEN_VECT &Tokens, const std::string &IndentString, const std::string &Indent) {
	return getParsableString(Tokens.begin(), Tokens.end(), IndentString, Indent);
}
std::string CScriptToken::getParsableString(TOKEN_VECT_it Begin, TOKEN_VECT_it End, const std::string &IndentString, const std::string &Indent) {
	std::ostringstream destination;
	std::string nl = Indent.size() ? "\n" : " ";
	std::string my_indentString = IndentString;
	bool add_nl=false, block_start=false, need_space=false;
	int skip_collon = 0;

	for(TOKEN_VECT_it it=Begin; it != End; ++it) {
		std::string OutString;
		if(add_nl) OutString.append(nl).append(my_indentString);
		bool old_block_start = block_start;
		bool old_need_space = need_space;
		add_nl = block_start = need_space =false;
		if(it->token == LEX_STR)
			OutString.append(getJSString(it->String())), need_space=true;
		else if(LEX_TOKEN_DATA_STRING(it->token))
			OutString.append(it->String()), need_space=true;
		else if(LEX_TOKEN_DATA_FLOAT(it->token))
			OutString.append(CNumber(it->Float()).toString()), need_space=true;
		else if(it->token == LEX_INT)
			OutString.append(CNumber(it->Int()).toString()), need_space=true;
		else if(LEX_TOKEN_DATA_FUNCTION(it->token)) {
			auto &Fnc = it->Fnc();
			bool isArrowFunction = Fnc->isArrowFunction();
			if (!isArrowFunction) {
				if (Fnc->type == LEX_T_GET) OutString.append("get ");
				else if (Fnc->type == LEX_T_SET) OutString.append("set ");
				else if (Fnc->type == LEX_T_GENERATOR_MEMBER) OutString.append("*");
				else {
					OutString.append("function");
					if (Fnc->isGenerator()) OutString.append("*");
					OutString.append(" ");
				}
			}
			if (Fnc->name.size())
				OutString.append(it->Fnc()->name);
			OutString.append(it->Fnc()->getArgumentsString(isArrowFunction));
			if(isArrowFunction)
				OutString.append("=> ");
			OutString.append(getParsableString(it->Fnc()->body, my_indentString, Indent));
		} else if(LEX_TOKEN_DATA_LOOP(it->token)) {
			OutString.append(it->Loop()->getParsableString(my_indentString, Indent));
		} else if(LEX_TOKEN_DATA_IF(it->token)) {
			OutString.append(it->If()->getParsableString(my_indentString, Indent));
		} else if(LEX_TOKEN_DATA_TRY(it->token)) {
			OutString.append(it->Try()->getParsableString(my_indentString, Indent));
		} else if(LEX_TOKEN_DATA_DESTRUCTURING_VAR(it->token)) {
			OutString.append(it->DestructuringVar()->getParsableString());
		} else if(LEX_TOKEN_DATA_OBJECT_LITERAL(it->token)) {
			OutString.append(it->Object()->getParsableString());
		} else if(it->token == '{') {
			OutString.append("{");
			my_indentString.append(Indent);
			add_nl = block_start = true;
		} else if(it->token == '}') {
			my_indentString.resize(my_indentString.size() - std::min(my_indentString.size(),Indent.size()));
			if(old_block_start)
				OutString =  "}";
			else
				OutString = nl + my_indentString + "}";
			add_nl = true;
		} else if(it->token == LEX_T_SKIP) {
			// ignore SKIP-Token
		} else if(it->token == LEX_T_END_EXPRESSION) {
			// ignore SKIP-Token
		} else if(it->token == LEX_T_ARRAY_COMPREHENSIONS_BODY) {
			// ignore Forwarder-Token
		} else if(it->token == LEX_T_FORWARD) {
			// ignore Forwarder-Token
		} else if(it->token == LEX_R_FOR) {
			OutString.append(CScriptToken::getTokenStr(it->token));
			skip_collon=2;
		} else {
			OutString.append(CScriptToken::getTokenStr(it->token, 0, &need_space));
			if(it->token==';') {
				if(skip_collon) { --skip_collon; }
				else add_nl=true;
			}
		}
		if(need_space && old_need_space) destination << " ";
		destination << OutString;
	}
	return destination.str();

}

std::string CScriptToken::getTokenStr( int token, const char *tokenStr/*=0*/, bool *need_space/*=0 */ ) {
	if(!tokens2str_sorted) tokens2str_sorted=tokens2str_sort();
	if(token == LEX_ID && tokenStr) {
		if(need_space) *need_space=true;
		return tokenStr;
	}
	token2str_t *found = std::lower_bound(reserved_words_begin, reserved_words_end, token, token2str_cmp_t());
	if(found != reserved_words_end && found->id==token) {
		if(need_space) *need_space=found->need_space;
		return found->str;
	}
	found = std::lower_bound(tokens2str_begin, tokens2str_end, token, token2str_cmp_t());
	if(found != tokens2str_end && found->id==token) {
		if(need_space) *need_space=found->need_space;
		return found->str;
	}
	if(need_space) *need_space=false;

	if (token>32 && token<128) {
		char buf[2] = " ";
		buf[0] = (char)token;
		return buf;
	}

	std::ostringstream msg;
	msg << "?[" << token << "]";
	return msg.str();
}
const char *CScriptToken::isReservedWord(int Token) {
	if(!tokens2str_sorted) tokens2str_sorted=tokens2str_sort();
	token2str_t *found = std::lower_bound(reserved_words_begin, reserved_words_end, Token, token2str_cmp_t());
	if(found != reserved_words_end && found->id==Token) {
		return found->str;
	}
	return 0;
}
int CScriptToken::isReservedWord(const std::string &Str) {
	size_t len = Str.length();
	if(len >= reserved_words_min_len && len <= reserved_words_max_len) {
		const char *str = Str.c_str();
		if(!tokens2str_sorted) tokens2str_sorted=tokens2str_sort();
		token2str_t **found = std::lower_bound(str2reserved_begin, str2reserved_end, str, token2str_cmp_t());
		if(found != str2reserved_end && strcmp((*found)->str, str)==0) {
			return (*found)->id;
		}
	}
	return LEX_ID;
}


//////////////////////////////////////////////////////////////////////////
/// CScriptTokenizer
//////////////////////////////////////////////////////////////////////////

CScriptTokenizer::CScriptTokenizer() : tk(0), l(0), prevPos(&tokens) {
}
CScriptTokenizer::CScriptTokenizer(CScriptLex &Lexer) : l(0), prevPos(&tokens) {
	tokenizeCode(Lexer);
}

static std::string &read_string_from_istream(std::istream &in, std::string &out)
{
	char buffer[4096];
	while (in.read(buffer, sizeof(buffer)))
		out.append(buffer, sizeof(buffer));
	out.append(buffer, (size_t)in.gcount());
	return out;
}

CScriptTokenizer::CScriptTokenizer(const char *Code, const std::string &File, int Line, int Column) : l(0), prevPos(&tokens) {
	if(Code) {
		CScriptLex lexer(Code, File, Line, Column);
		tokenizeCode(lexer);
	} else {
		std::ifstream in(File.c_str(), std::fstream::in | std::fstream::binary);
		std::string code;
		CScriptLex lexer(read_string_from_istream(in, code).c_str(), File);
		tokenizeCode(lexer);
	}
}

void CScriptTokenizer::tokenizeCode(CScriptLex &Lexer) {
	try {
		l=&Lexer;
		tokens.clear();
		tokenScopeStack.clear();
		ScriptTokenState state;
		pushForwarder(state);
		if(l->tk == '§') { // special-Token at Start means the code begins not at Statement-Level
			l->match('§');
			tokenizeLiteral(state, 0);
		} else do {
			tokenizeStatement(state, 0);
		} while (l->tk!=LEX_EOF);
		pushToken(state.Tokens, LEX_EOF); // add LEX_EOF-Token
		removeEmptyForwarder(state);
//		TOKEN_VECT(tokens).swap(tokens);//	tokens.shrink_to_fit();
		tokens.swap(state.Tokens);
		pushTokenScope(tokens);
		currentFile = l->currentFile;
		tk = getToken().token;
	} catch (...) {
		l=0;
		throw;
	}
}

void CScriptTokenizer::getNextToken() {
	prevPos = tokenScopeStack.back();
	if(getToken().token == LEX_EOF)
		return;
	ScriptTokenPosition &_TokenPos = tokenScopeStack.back();
	_TokenPos.pos++;
	if(_TokenPos.pos == _TokenPos.tokens->end())
		tokenScopeStack.pop_back();
//	ScriptTokenPosition &TokenPos = tokenScopeStack.back();
	tk = getToken().token;
}



void CScriptTokenizer::match(int ExpectedToken, int AlternateToken/*=-1*/) {
	if(check(ExpectedToken, AlternateToken))
		getNextToken();
}
bool CScriptTokenizer::check(int ExpectedToken, int AlternateToken/*=-1*/) {
	int currentToken = getToken().token;
	if (ExpectedToken==';' && (currentToken==LEX_EOF || currentToken=='}')) return false; // ignore last missing ';'
	if (currentToken!=ExpectedToken && currentToken!=AlternateToken) {
		std::ostringstream errorString;
		if(ExpectedToken == LEX_EOF)
			errorString << "Got unexpected " << CScriptToken::getTokenStr(currentToken);
		else {
			errorString << "Got '" << CScriptToken::getTokenStr(currentToken) << "' expected '" << CScriptToken::getTokenStr(ExpectedToken) << "'";
			if(AlternateToken!=-1) errorString << " or '" << CScriptToken::getTokenStr(AlternateToken) << "'";
		}
		throw CScriptException(SyntaxError, errorString.str(), currentFile, currentLine(), currentColumn());
	}
	return true;
}
void CScriptTokenizer::pushTokenScope(TOKEN_VECT &Tokens) {
	tokenScopeStack.push_back(ScriptTokenPosition(&Tokens));
	tk = getToken().token;
}

void CScriptTokenizer::setPos(ScriptTokenPosition &TokenPos) {
	ASSERT( TokenPos.tokens == tokenScopeStack.back().tokens);
	tokenScopeStack.back().pos = TokenPos.pos;
	tk = getToken().token;
}
void CScriptTokenizer::skip(int Tokens) {
	ASSERT(tokenScopeStack.back().tokens->end()-tokenScopeStack.back().pos-Tokens>=0);
	tokenScopeStack.back().pos+=Tokens-1;
	getNextToken();
}

static inline void setTokenSkip(CScriptTokenizer::ScriptTokenState &State) {
	size_t tokenBeginIdx = State.Marks.back();
	State.Marks.pop_back();
	State.Tokens[tokenBeginIdx].Int(size2int32(State.Tokens.size()-tokenBeginIdx));
}

enum {
	TOKENIZE_FLAGS_canLabel			= 1<<0,
	TOKENIZE_FLAGS_canBreak			= 1<<1,
	TOKENIZE_FLAGS_canContinue		= 1<<2,
	TOKENIZE_FLAGS_canReturn		= 1<<3,
	TOKENIZE_FLAGS_canYield			= 1<<4,
	TOKENIZE_FLAGS_asStatement		= 1<<5,
	TOKENIZE_FLAGS_noIn				= 1<<6,	/// expression without 'in' or 'of'
	TOKENIZE_FLAGS_isAccessor		= 1<<7,
	TOKENIZE_FLAGS_isGenerator		= 1<<8,
	TOKENIZE_FLAGS_callForNew		= 1<<9,
	TOKENIZE_FLAGS_noBlockStart		= 1<<10,
	TOKENIZE_FLAGS_nestedObject		= 1<<11,
};
void CScriptTokenizer::tokenizeTry(ScriptTokenState &State, int Flags) {
	l->match(LEX_R_TRY);
	CScriptToken TryToken(LEX_T_TRY);
	auto &TryData = TryToken.Try();
	pushToken(State.Tokens, TryToken);

	TOKEN_VECT mainTokens;
	State.Tokens.swap(mainTokens);

	// try-block
	tokenizeBlock(State, Flags);
	State.Tokens.swap(TryData->tryBlock);

	// catch-blocks
	l->check(LEX_R_CATCH, LEX_R_FINALLY);
	bool unconditionalCatch = false;
	while(l->tk == LEX_R_CATCH) {
		if(unconditionalCatch) throw CScriptException(SyntaxError, "catch after unconditional catch", l->currentFile, l->currentLine(), l->currentColumn());

		// vars & condition
		l->match(LEX_R_CATCH);
		l->match('(');
		TryData->catchBlocks.resize(TryData->catchBlocks.size()+1);
		CScriptTokenDataTry::CatchBlock &catchBlock = TryData->catchBlocks.back();
		pushForwarder(State, true);
		STRING_VECTOR_t vars;
		catchBlock.indentifiers = tokenizeVarIdentifier(&vars).DestructuringVar();
		State.Forwarders.back()->addLets(vars);
		if(l->tk == LEX_R_IF) {
			l->match(LEX_R_IF);
			tokenizeExpression(State, Flags);
		} else
			unconditionalCatch = true;
		State.Tokens.swap(catchBlock.condition);
		l->match(')');

		// catch-block
		tokenizeBlock(State, Flags | TOKENIZE_FLAGS_noBlockStart);
		State.Tokens.swap(catchBlock.block);
		State.Forwarders.pop_back();
	}
	// finally-block
	if(l->tk == LEX_R_FINALLY) {
		l->match(LEX_R_FINALLY);
		tokenizeBlock(State, Flags);
		State.Tokens.swap(TryData->finallyBlock);
	}
	State.Tokens.swap(mainTokens);
}
void CScriptTokenizer::tokenizeSwitch(ScriptTokenState &State, int Flags) {

	State.Marks.push_back(pushToken(State.Tokens)); // push Token & push tokenBeginIdx
	pushToken(State.Tokens, '(');
	tokenizeExpression(State, Flags);
	pushToken(State.Tokens, ')');

	State.Marks.push_back(pushToken(State.Tokens, '{')); // push Token & push blockBeginIdx
	pushForwarder(State);


	std::vector<int>::size_type MarksSize = State.Marks.size();
	Flags |= TOKENIZE_FLAGS_canBreak;
	for(bool hasDefault=false;;) {
		if( l->tk == LEX_R_CASE || l->tk == LEX_R_DEFAULT) {
			if(l->tk == LEX_R_CASE) {
				State.Marks.push_back(pushToken(State.Tokens)); // push Token & push caseBeginIdx
				State.Marks.push_back(pushToken(State.Tokens,CScriptToken(LEX_T_SKIP))); //  skipper to skip case-expression
				tokenizeExpression(State, Flags);
				setTokenSkip(State);
			} else { // default
				State.Marks.push_back(pushToken(State.Tokens)); // push Token & push caseBeginIdx
				if(hasDefault) throw CScriptException(SyntaxError, "more than one switch default", l->currentFile, l->currentLine(), l->currentColumn());
				hasDefault = true;
			}

			State.Marks.push_back(pushToken(State.Tokens, ':'));
			while(l->tk != '}' && l->tk != LEX_R_CASE && l->tk != LEX_R_DEFAULT && l->tk != LEX_EOF )
				tokenizeStatement(State, Flags);
			setTokenSkip(State);
		} else if(l->tk == '}')
			break;
		else
			throw CScriptException(SyntaxError, "invalid switch statement", l->currentFile, l->currentLine(), l->currentColumn());
	}
	while(MarksSize < State.Marks.size()) setTokenSkip(State);
	removeEmptyForwarder(State); // remove Forwarder if empty
	pushToken(State.Tokens, '}');
	setTokenSkip(State); // switch-block
	setTokenSkip(State); // switch-statement
}
void CScriptTokenizer::tokenizeWith(ScriptTokenState &State, int Flags) {
	State.Marks.push_back(pushToken(State.Tokens)); // push Token & push tokenBeginIdx

	pushToken(State.Tokens, '(');
	tokenizeExpression(State, Flags);
	pushToken(State.Tokens, ')');
	tokenizeStatementNoLet(State, Flags);

	setTokenSkip(State);
}

static inline uint32_t GetLoopLabels(CScriptTokenizer::ScriptTokenState &State, const std::shared_ptr<CScriptTokenDataLoop> &LoopData) {
	uint32_t label_count = 0;
	if(State.Tokens.size()>=2) {
		for(TOKEN_VECT::reverse_iterator it = State.Tokens.rbegin(); it!=State.Tokens.rend(); ++it) {
			if(it->token == ':' && (++it)->token == LEX_T_LABEL) {
				++label_count;
				LoopData->labels.push_back(it->String());
				State.LoopLabels.push_back(it->String());
				it->token = LEX_T_DUMMY_LABEL;
			} else
				break;
		}
	}
	return label_count;
}
static inline void PopLoopLabels(uint32_t label_count, STRING_VECTOR_t &LoopLabels) {
	ASSERT(label_count <= LoopLabels.size());
	LoopLabels.resize(LoopLabels.size()-label_count);
}
void CScriptTokenizer::tokenizeWhileAndDo(ScriptTokenState &State, int Flags) {

	bool do_while = l->tk==LEX_R_DO;

	CScriptToken LoopToken(LEX_T_LOOP);
	auto &LoopData = LoopToken.Loop();
	LoopData->type = do_while ? CScriptTokenDataLoop::DO : CScriptTokenDataLoop::WHILE;

	// get loop-labels
	uint32_t label_count = GetLoopLabels(State, LoopData);

	l->match(l->tk); // match while or do

	pushToken(State.Tokens, LoopToken);

	TOKEN_VECT mainTokens;
	State.Tokens.swap(mainTokens);

	if(!do_while) {
		l->match('(');
		tokenizeExpression(State, Flags);
		State.Tokens.swap(LoopData->condition);
		l->match(')');
	}
	tokenizeStatementNoLet(State, Flags | TOKENIZE_FLAGS_canBreak | TOKENIZE_FLAGS_canContinue);
	State.Tokens.swap(LoopData->body);
	if(do_while) {
		l->match(LEX_R_WHILE);
		l->match('(');
		tokenizeExpression(State, Flags);
		State.Tokens.swap(LoopData->condition);
		l->match(')');
		l->match(';');
	}
	State.Tokens.swap(mainTokens);
	PopLoopLabels(label_count, State.LoopLabels);
}

void CScriptTokenizer::tokenizeIf_inArrayComprehensions(ScriptTokenState &State, int Flags, TOKEN_VECT &Assign) {
	CScriptToken IfToken(LEX_T_IF);
	auto &IfData = IfToken.If();

	pushToken(State.Tokens, IfToken);

	l->match(LEX_R_IF);
	l->match('(');
	TOKEN_VECT mainTokens;
	State.Tokens.swap(mainTokens);

	tokenizeExpression(State, Flags);
	l->match(')');
	State.Tokens.push_back(CScriptToken(LEX_T_END_EXPRESSION));
	State.Tokens.swap(IfData->condition);

	if(l->tk == LEX_R_FOR)
		tokenizeFor_inArrayComprehensions(State, Flags, Assign);
	else if(l->tk == LEX_R_IF)
		tokenizeIf_inArrayComprehensions(State, Flags, Assign);
	else {
		if(Assign.empty()) {
			tokenizeAssignment(State, Flags);
			State.Tokens.push_back(CScriptToken(LEX_T_END_EXPRESSION));
			State.Tokens.swap(Assign);
		}
		CScriptToken Token(LEX_T_ARRAY_COMPREHENSIONS_BODY);
		auto &Data = Token.ArrayComprehensionsBody();
		Data->body.insert(Data->body.end(), Assign.begin(), Assign.end());
	}
	State.Tokens.swap(IfData->if_body);
	State.Tokens.swap(mainTokens);
}

void CScriptTokenizer::tokenizeIf(ScriptTokenState &State, int Flags) {

	CScriptToken IfToken(LEX_T_IF);
	auto &IfData = IfToken.If();

	pushToken(State.Tokens, IfToken);

	l->match(LEX_R_IF);
	l->match('(');
	TOKEN_VECT mainTokens;
	State.Tokens.swap(mainTokens);

	tokenizeExpression(State, Flags);
	l->match(')');
	State.Tokens.push_back(CScriptToken(LEX_T_END_EXPRESSION));
	State.Tokens.swap(IfData->condition);
	tokenizeStatementNoLet(State, Flags);
	State.Tokens.swap(IfData->if_body);
	if(l->tk == LEX_R_ELSE) {
		l->match(LEX_R_ELSE);
		tokenizeStatementNoLet(State, Flags);
		State.Tokens.swap(IfData->else_body);
	}
	State.Tokens.swap(mainTokens);
}

void CScriptTokenizer::tokenizeFor_inArrayComprehensions(ScriptTokenState &State, int Flags, TOKEN_VECT &Assign) {

	CScriptToken LoopToken(LEX_T_FOR_IN);
	auto &LoopData = LoopToken.Loop();
	LoopData->type = CScriptTokenDataLoop::FOR_OF;

	LoopToken.line   = l->currentLine();
	LoopToken.column = l->currentColumn();
	l->match(LEX_R_FOR);

	pushToken(State.Tokens, LoopToken);

	l->match('(');
	l->check(LEX_ID);
	TOKEN_VECT mainTokens;
	State.Tokens.swap(mainTokens);

	// tokenize init
	pushForwarder(State, true); // no clean up empty tokenizer
	STRING_VECTOR_t tmp(1, l->tkStr);
	State.Forwarders.back()->addLets(tmp);
	State.Tokens.swap(LoopData->init);

	// tokenize condition
	pushToken(State.Tokens, LEX_ID);
	if(l->tk==LEX_ID && l->tkStr=="of") l->tk = LEX_T_OF; // fake token
	l->match(LEX_T_OF);
	State.Tokens.push_back('=');
	State.Tokens.push_back(LEX_T_EXCEPTION_VAR);
	State.Tokens.push_back('.');
	State.Tokens.push_back(CScriptToken(LEX_ID, "next"));
	State.Tokens.push_back('(');
	State.Tokens.push_back(')');
	State.Tokens.push_back(';');
	State.Tokens.swap(LoopData->condition);

	// tokenize iter
	tokenizeExpression(State, Flags);
	l->match(')');
	State.Tokens.swap(LoopData->iter);

	// tokenize body
	if(l->tk == LEX_R_FOR)
		tokenizeFor_inArrayComprehensions(State, Flags, Assign);
	else if(l->tk == LEX_R_IF)
		tokenizeIf_inArrayComprehensions(State, Flags, Assign);
	else {
		if(Assign.empty()) {
			tokenizeAssignment(State, Flags);
			State.Tokens.push_back(CScriptToken(LEX_T_END_EXPRESSION));
			State.Tokens.swap(Assign);
		}
		CScriptToken Token(LEX_T_ARRAY_COMPREHENSIONS_BODY);
		auto &Data = Token.ArrayComprehensionsBody();
		Data->body.insert(Data->body.end(), Assign.begin(), Assign.end());
		pushToken(State.Tokens, Token);
	}
	State.Forwarders.pop_back();
	State.Tokens.swap(LoopData->body);
	State.Tokens.swap(mainTokens);
}

void CScriptTokenizer::tokenizeFor(ScriptTokenState &State, int Flags) {
	bool for_in=false, for_of=false, for_each_in=false;
	CScriptToken LoopToken(LEX_T_LOOP);
	auto &LoopData = LoopToken.Loop();

	// get loop-labels
	uint32_t label_count = GetLoopLabels(State, LoopData);

	l->match(LEX_R_FOR);
	if((for_in = for_each_in = (l->tk == LEX_ID && l->tkStr == "each")))
		l->match(LEX_ID); // match "each"

	pushToken(State.Tokens, LoopToken);

	l->match('(');
	TOKEN_VECT mainTokens;
	State.Tokens.swap(mainTokens);

	bool haveLetScope = false;

	if(l->tk == LEX_R_VAR || l->tk == LEX_R_LET) {
		if(l->tk == LEX_R_VAR)
			tokenizeVarNoConst(State, Flags | TOKENIZE_FLAGS_noIn);
		else { //if(l->tk == LEX_R_LET)
			haveLetScope = true;
			pushForwarder(State, true); // no clean up empty tokenizer
			tokenizeLet(State, Flags | TOKENIZE_FLAGS_noIn | TOKENIZE_FLAGS_asStatement);
		}
	} else if(l->tk!=';') {
		tokenizeExpression(State, Flags | TOKENIZE_FLAGS_noIn);
	}
	if((for_in=(l->tk==LEX_R_IN || (l->tk==LEX_ID && l->tkStr=="of")))) {
		if(!State.LeftHand)
			throw CScriptException(ReferenceError, "invalid for/in left-hand side", l->currentFile, l->currentLine(), l->currentColumn());
		if(l->tk==LEX_ID && l->tkStr=="of") l->tk = LEX_T_OF; // fake token
		if((for_of = (!for_each_in && l->tk==LEX_T_OF))) {
			l->match(LEX_T_OF);
			LoopData->type = CScriptTokenDataLoop::FOR_OF;
		} else {
			l->match(LEX_R_IN);
			LoopData->type = for_each_in ? CScriptTokenDataLoop::FOR_EACH : CScriptTokenDataLoop::FOR_IN;
		}
		State.Tokens.swap(LoopData->condition);

		if(LoopData->condition.front().token == LEX_T_FORWARD) {
			LoopData->init.push_back(LoopData->condition.front());
			LoopData->condition.erase(LoopData->condition.begin());
		}
		mainTokens.back().token = LEX_T_FOR_IN;
	} else {
		l->check(';'); // no automatic ;-injection
		pushToken(State.Tokens, ';');
		State.Tokens.swap(LoopData->init);
		if(l->tk != ';') tokenizeExpression(State, Flags);
		l->check(';'); // no automatic ;-injection
		l->match(';'); // no automatic ;-injection
		State.Tokens.swap(LoopData->condition);
	}

	if(for_in || l->tk != ')') tokenizeExpression(State, Flags);
	l->match(')');
	State.Tokens.swap(LoopData->iter);
	Flags = (Flags & (TOKENIZE_FLAGS_canReturn | TOKENIZE_FLAGS_canYield)) | TOKENIZE_FLAGS_canBreak | TOKENIZE_FLAGS_canContinue;
	if(haveLetScope) Flags |= TOKENIZE_FLAGS_noBlockStart;
	tokenizeStatementNoLet(State, Flags);
	if(haveLetScope) State.Forwarders.pop_back();

	State.Tokens.swap(LoopData->body);
	State.Tokens.swap(mainTokens);
	if(for_in) {
		LoopData->condition.push_back('=');
		LoopData->condition.push_back(LEX_T_EXCEPTION_VAR);
		LoopData->condition.push_back('.');
		LoopData->condition.push_back(CScriptToken(LEX_ID, "next"));
		LoopData->condition.push_back('(');
		LoopData->condition.push_back(')');
		LoopData->condition.push_back(';');
	}
	PopLoopLabels(label_count, State.LoopLabels);
}

static void tokenizeVarIdentifierDestructuring( CScriptLex *Lexer, DESTRUCTURING_VARS_t &Vars, const std::string &Path, STRING_VECTOR_t *VarNames );
static void tokenizeVarIdentifierDestructuringObject(CScriptLex *Lexer, DESTRUCTURING_VARS_t &Vars, STRING_VECTOR_t *VarNames) {
	Lexer->match('{');
	while(Lexer->tk != '}') {
		CScriptLex::POS prev_pos = Lexer->pos;
		std::string Path = Lexer->tkStr;
		Lexer->match(LEX_ID, LEX_STR);
		if(Lexer->tk == ':') {
			Lexer->match(':');
			tokenizeVarIdentifierDestructuring(Lexer, Vars, Path, VarNames);
		} else {
			Lexer->reset(prev_pos);
			if(VarNames) VarNames->push_back(Lexer->tkStr);
			Vars.push_back(DESTRUCTURING_VAR_t(Lexer->tkStr, Lexer->tkStr));
			Lexer->match(LEX_ID);
		}
		if (Lexer->tk!='}') Lexer->match(',', '}');
	}
	Lexer->match('}');
}
static void tokenizeVarIdentifierDestructuringArray(CScriptLex *Lexer, DESTRUCTURING_VARS_t &Vars, STRING_VECTOR_t *VarNames) {
	int idx = 0;
	Lexer->match('[');
	while(Lexer->tk != ']') {
		if(Lexer->tk == ',')
			Vars.push_back(DESTRUCTURING_VAR_t("", "")); // empty
		else
			tokenizeVarIdentifierDestructuring(Lexer, Vars, std::to_string(idx), VarNames);
		++idx;
		if (Lexer->tk!=']') Lexer->match(',',']');
	}
	Lexer->match(']');
}
static void tokenizeVarIdentifierDestructuring(CScriptLex *Lexer, DESTRUCTURING_VARS_t &Vars, const std::string &Path, STRING_VECTOR_t *VarNames ) {
	if(Lexer->tk == '[') {
		Vars.push_back(DESTRUCTURING_VAR_t(Path, "[")); // marks array begin
		tokenizeVarIdentifierDestructuringArray(Lexer, Vars, VarNames);
		Vars.push_back(DESTRUCTURING_VAR_t("", "]")); // marks array end
	} else if(Lexer->tk == '{') {
		Vars.push_back(DESTRUCTURING_VAR_t(Path, "{")); // marks object begin
		tokenizeVarIdentifierDestructuringObject(Lexer, Vars, VarNames);
		Vars.push_back(DESTRUCTURING_VAR_t("", "}")); // marks object end
	} else {
		if(VarNames) VarNames->push_back(Lexer->tkStr);
		Vars.push_back(DESTRUCTURING_VAR_t(Path, Lexer->tkStr));
		Lexer->match(LEX_ID);
	}
}
CScriptToken CScriptTokenizer::tokenizeVarIdentifier( STRING_VECTOR_t *VarNames/*=0*/, bool *NeedAssignment/*=0*/ ) {
	CScriptToken token(LEX_T_DESTRUCTURING_VAR);
	bool withAssignment = NeedAssignment && *NeedAssignment;
	if(NeedAssignment) *NeedAssignment=(l->tk == '[' || l->tk=='{');
	token.column = l->currentColumn();
	token.line = l->currentLine();
	tokenizeVarIdentifierDestructuring(l, token.DestructuringVar()->vars, "", VarNames);
	if(withAssignment && l->tk=='=') {
		l->match('=');
		ScriptTokenState assignmentState;
		tokenizeCondition(assignmentState, 0);
		token.DestructuringVar()->assignment.swap(assignmentState.Tokens);
	}
	return token;
}
CScriptToken CScriptTokenizer::tokenizeFunctionArgument() {
	bool withAssignment = true;
	return tokenizeVarIdentifier(0, &withAssignment);
}

void CScriptTokenizer::tokenizeArrowFunction(const TOKEN_VECT &Arguments, ScriptTokenState &State, int Flags, bool noLetDef/*=false*/)
{
	l->match(LEX_ARROW);
	CScriptToken FncToken(LEX_T_FUNCTION_ARROW);
	auto &FncData = FncToken.Fnc();
	FncData->arguments = Arguments;
	FncData->file = l->currentFile;
	FncData->line = l->currentLine();

	ScriptTokenState functionState;
	functionState.HaveReturnValue /*= functionState.FunctionIsGenerator*/ = false;
	if(l->tk == '{')
		tokenizeBlock(functionState, TOKENIZE_FLAGS_canReturn); // => { } 
	else {
		tokenizeAssignment(functionState, 0);
		functionState.HaveReturnValue = true;
	}
	functionState.Tokens.swap(FncData->body);
	State.Tokens.push_back(FncToken);
}

void CScriptTokenizer::tokenizeFunction(ScriptTokenState &State, int Flags, bool noLetDef/*=false*/) {
	bool forward = false;
	bool Statement = (Flags & TOKENIZE_FLAGS_asStatement) != 0;
	bool Accessor = (Flags & TOKENIZE_FLAGS_isAccessor) != 0;
	bool Generator = (Flags & TOKENIZE_FLAGS_isGenerator) != 0;
	CScriptLex::POS functionPos = l->pos;

	int tk = l->tk;
	if(Accessor) {
		tk = State.Tokens.back().String()=="get"?LEX_T_GET:LEX_T_SET;
		State.Tokens.pop_back();
	} else if (Generator) {
		tk = LEX_T_GENERATOR_MEMBER;
		State.Tokens.pop_back();
	} else {
//		Generator = l->pos.tokenStart[8] == '*';
		l->match(LEX_R_FUNCTION);
		if (l->tk == '*') {
			Generator = true;
			l->match('*');
			tk = LEX_T_GENERATOR;
//			do {} while (0);
		}
		if(!Statement) tk = tk == LEX_R_FUNCTION ? LEX_T_FUNCTION_OPERATOR : LEX_T_GENERATOR_OPERATOR;
	}
	if(tk == LEX_R_FUNCTION || tk == LEX_T_GENERATOR) // only forward functions
		forward = !noLetDef && State.Forwarders.front() == State.Forwarders.back();

	CScriptToken FncToken(tk);
	auto &FncData = FncToken.Fnc();

	if(l->tk == LEX_ID || Accessor) {
		FncData->name = l->tkStr;
		l->match(LEX_ID, LEX_STR);
	} else if(Statement)
		throw CScriptException(SyntaxError, "Function statement requires a name.", l->currentFile, l->currentLine(), l->currentColumn());
	l->match('(');
	while(l->tk != ')') {
		FncData->arguments.push_back(tokenizeFunctionArgument());
		if (l->tk!=')') l->match(',',')');
	}
	l->match(')');

	FncData->file = l->currentFile;
	FncData->line = l->currentLine();
	
	ScriptTokenState functionState;
//	if(l->tk == '{' || tk==LEX_T_GET || tk==LEX_T_SET)
		tokenizeBlock(functionState, TOKENIZE_FLAGS_canReturn | (Generator ? TOKENIZE_FLAGS_canYield : 0));
//	else {
//		tokenizeExpression(functionState, TOKENIZE_FLAGS_canYield);
//		if(Statement) l->match(';');
//		functionState.HaveReturnValue = true;
//	}
	if(functionState.HaveReturnValue == true && Generator != 0)
		throw CScriptException(TypeError, "generator function returns a value.", l->currentFile, functionPos.currentLine, functionPos.currentColumn());

	functionState.Tokens.swap(FncData->body);
	if(forward) {
		State.Forwarders.front()->functions.insert(FncToken);
		FncToken.token = LEX_T_FUNCTION_PLACEHOLDER;
	}
	State.Tokens.push_back(FncToken);
}

void CScriptTokenizer::tokenizeLet(ScriptTokenState &State, int Flags, bool noLetDef/*=false*/) {
	bool Definition = (Flags & TOKENIZE_FLAGS_asStatement)!=0;
	bool noIN = (Flags & TOKENIZE_FLAGS_noIn)!=0;
	bool Statement = Definition && !noIN;
	bool Expression = !Definition;
	Flags &= ~(TOKENIZE_FLAGS_asStatement);
	if(!Definition) noIN=false, Flags &= ~TOKENIZE_FLAGS_noIn;

	bool foundIN = false;
	bool leftHand = true;
	int currLine = l->currentLine(), currColumn = l->currentColumn();

	State.Marks.push_back(pushToken(State.Tokens)); // push Token & push BeginIdx

	if(l->tk == '(' || !Definition) { // no definition needs statement or expression
		leftHand = false;
		Expression = true;
		pushToken(State.Tokens, '(');
		pushForwarder(State);
	} else if(noLetDef)
		throw CScriptException(SyntaxError, "let declaration not directly within block", l->currentFile, currLine, currColumn);
	STRING_VECTOR_t vars;
	for(;;) {
		bool needAssignment = false;
		State.Tokens.push_back(tokenizeVarIdentifier(&vars, &needAssignment));
		if(noIN && (foundIN=(l->tk==LEX_R_IN || (l->tk==LEX_ID && l->tkStr=="of"))))
			break;
		if(needAssignment || l->tk=='=') {
			leftHand = false;
			pushToken(State.Tokens, '=');
			tokenizeAssignment(State, Flags);
			if(noIN && (foundIN=(l->tk==LEX_R_IN || (l->tk==LEX_ID && l->tkStr=="of"))))
				break;
		}
		if(l->tk==',') {
			leftHand = false;
			pushToken(State.Tokens);
		}
		else
			break;
	}
	if(Expression) {
		std::string redeclared = State.Forwarders.back()->addLets(vars);
		if(redeclared.size())
			throw CScriptException(TypeError, "redeclaration of variable '"+redeclared+"'", l->currentFile, currLine, currColumn);
		if(!foundIN) {
			pushToken(State.Tokens, ')');
			if(Statement) {
				if(l->tk == '{') // no extra BlockStart by expression
					tokenizeBlock(State, Flags|=TOKENIZE_FLAGS_noBlockStart);
				else
					tokenizeStatementNoLet(State, Flags);
			} else
				tokenizeAssignment(State, Flags);
		}
		// never remove Forwarder-token here -- popForwarder(Tokens, BlockStart, Marks);
		State.Forwarders.back()->vars_in_letscope.clear(); // only clear vars_in_letscope
		State.Marks.pop_back();
	} else {
		if(!noIN) pushToken(State.Tokens, ';');

		std::string redeclared;
		if(State.Forwarders.size()<=1) {
			// Currently it is allowed in javascript, to redeclare "let"-declared vars
			// in root- or function-scopes. In this case, "let" handled like "var"
			// To prevent redeclaration in root- or function-scopes define PREVENT_REDECLARATION_IN_FUNCTION_SCOPES
#ifdef PREVENT_REDECLARATION_IN_FUNCTION_SCOPES
			redeclared = State.Forwarders.front()->addLets(vars);
#else
			State.Forwarders.front()->addVars(vars);
#endif
		} else
			redeclared = State.Forwarders.back()->addLets(vars);
		if(redeclared.size())
			throw CScriptException(TypeError, "redeclaration of variable '"+redeclared+"'", l->currentFile, currLine, currColumn);
	}
	setTokenSkip(State);
	if(leftHand) State.LeftHand = true;
}

void CScriptTokenizer::tokenizeVarNoConst( ScriptTokenState &State, int Flags) {
	l->check(LEX_R_VAR);
	tokenizeVarAndConst(State, Flags);
}
void CScriptTokenizer::tokenizeVarAndConst( ScriptTokenState &State, int Flags) {
	bool noIN = (Flags & TOKENIZE_FLAGS_noIn)!=0;
	int currLine = l->currentLine(), currColumn = l->currentColumn();

	bool leftHand = true;
	int tk = l->tk;
	State.Marks.push_back(pushToken(State.Tokens)); // push Token & push BeginIdx

	STRING_VECTOR_t vars;
	for(;;)
	{
		bool needAssignment = false;
		State.Tokens.push_back(tokenizeVarIdentifier(&vars, &needAssignment));
		if(noIN && (l->tk==LEX_R_IN || (l->tk==LEX_ID && l->tkStr=="of")))
			break;
		if(needAssignment || l->tk=='=') {
			leftHand = false;
			pushToken(State.Tokens, '=');
			tokenizeAssignment(State, Flags);
			if(noIN && (l->tk==LEX_R_IN || (l->tk==LEX_ID && l->tkStr=="of")))
				break;
		}
		if(l->tk==',') {
			leftHand = false;
			pushToken(State.Tokens);
		}
		else
			break;
	}
	if(!noIN) pushToken(State.Tokens, ';');

	setTokenSkip(State);

	if(tk==LEX_R_VAR)
		State.Forwarders.front()->addVars(vars);
	else
		State.Forwarders.front()->addConsts(vars);
	std::string redeclared;
	if(State.Forwarders.size()>1) // have let-scope
		redeclared = State.Forwarders.back()->addVarsInLetscope(vars);
#ifdef PREVENT_REDECLARATION_IN_FUNCTION_SCOPES
	else
		redeclared = State.Forwarders.front()->addVarsInLetscope(vars);
#endif
	if(redeclared.size())
		throw CScriptException(TypeError, "redeclaration of variable '"+redeclared+"'", l->currentFile, currLine, currColumn);
	if(leftHand) State.LeftHand = true;
}

void CScriptTokenizer::_tokenizeLiteralObject(ScriptTokenState &State, int Flags) {
	bool forFor = (Flags & TOKENIZE_FLAGS_noIn)!=0;
	bool nestedObject = 	(Flags & TOKENIZE_FLAGS_nestedObject) != 0;
	Flags &= ~(TOKENIZE_FLAGS_noIn | TOKENIZE_FLAGS_nestedObject);
	CScriptToken ObjectToken(LEX_T_OBJECT_LITERAL);
	ObjectToken.line = l->currentLine();
	ObjectToken.column = l->currentColumn();
	auto &Objc = ObjectToken.Object();

	Objc->type = CScriptTokenDataObjectLiteral::OBJECT;
	Objc->destructuring = Objc->structuring = true;

	std::string msg, msgFile;
	int msgLine=0, msgColumn=0;

	l->match('{');
	while (l->tk != '}') {
		CScriptTokenDataObjectLiteral::ELEMENT element;
		bool assign = false;
		if(CScriptToken::isReservedWord(l->tk))
			l->tk = LEX_ID; // fake reserved-word as member.ID
		if(l->tk == LEX_ID) {
			element.id = l->tkStr; 
			CScriptToken Token(l, LEX_ID);
			if((l->tk==LEX_ID || l->tk==LEX_STR ) && (element.id=="get" || element.id == "set")) {
				element.id = l->tkStr;
				element.value.push_back(Token);
				State.Tokens.swap(element.value);
				tokenizeFunction(State, Flags| TOKENIZE_FLAGS_isAccessor);
				State.Tokens.swap(element.value);
				Objc->destructuring = false;
			} else {
				if(Objc->destructuring && (l->tk == ',' || l->tk == '}')) {
					if(!msg.size()) {
						Objc->structuring = false;
						msg.append("Got '").append(CScriptToken::getTokenStr(l->tk, l->tkStr.c_str())).append("' expected ':'");
						msgFile = l->currentFile;
						msgLine = l->currentLine();
						msgColumn = l->currentColumn();
						;
					}
					element.value.push_back(Token);
				} else
					assign = true;
			}
		} else if (l->tk == '*') { // *generator() {}
			CScriptToken Token(l, '*');
			l->check(LEX_ID);
			element.id = l->tkStr;
			element.value.push_back(Token);
			State.Tokens.swap(element.value);
			tokenizeFunction(State, Flags | TOKENIZE_FLAGS_isGenerator);
			State.Tokens.swap(element.value);
			Objc->destructuring = false;
		} else if (l->tk == LEX_INT) {
			element.id = std::to_string((int32_t)strtol(l->tkStr.c_str(),0,0));
			l->match(LEX_INT);
			assign = true;
		} else if(l->tk == LEX_FLOAT) {
			element.id = float2string(strtod(l->tkStr.c_str(),0));
			l->match(LEX_FLOAT);
			assign = true;
		} else if(LEX_TOKEN_DATA_STRING(l->tk) && l->tk != LEX_REGEXP) {
			element.id = l->tkStr;
			l->match(l->tk);
			assign = true;
		} else
			l->match(LEX_ID, LEX_STR);
		if(assign) {
			l->match(':');
			int dFlags = Flags | ((l->tk == '{' || l->tk == '[') ? TOKENIZE_FLAGS_nestedObject : 0);
			State.pushLeftHandState();
			State.Tokens.swap(element.value);
			tokenizeAssignment(State, dFlags);
			State.Tokens.push_back(CScriptToken(LEX_T_END_EXPRESSION));
			State.Tokens.swap(element.value);
			if(Objc->destructuring) Objc->destructuring = State.LeftHand;
			State.popLeftHandeState();
		}

		if(!Objc->destructuring && msg.size())
			throw CScriptException(SyntaxError, msg, msgFile, msgLine, msgColumn);
		Objc->elements.push_back(element);
		if (l->tk != '}') l->match(',', '}');
	}
	l->match('}');
	if(Objc->destructuring && Objc->structuring) {
		if(nestedObject) {
			if(l->tk!=',' && l->tk!='}' && l->tk!=']' /* && l->tk!='=' ??? */)
				Objc->destructuring = false;
		}
		else
			Objc->setMode(l->tk=='=' || l->tk==LEX_ARROW || (forFor && (l->tk==LEX_R_IN ||(l->tk==LEX_ID && l->tkStr=="of"))));
	} else {
		if(!Objc->destructuring && msg.size())
			throw CScriptException(SyntaxError, msg, msgFile, msgLine, msgColumn);
		if(!nestedObject) Objc->setMode(Objc->destructuring);
	}

	if(Objc->destructuring)
		State.LeftHand = true;
	State.Tokens.push_back(ObjectToken);
}
void CScriptTokenizer::_tokenizeLiteralArray(ScriptTokenState &State, int Flags) {
	bool forFor = (Flags & TOKENIZE_FLAGS_noIn)!=0;
	bool nestedObject = 	(Flags & TOKENIZE_FLAGS_nestedObject) != 0;
	Flags &= ~(TOKENIZE_FLAGS_noIn | TOKENIZE_FLAGS_nestedObject);
	CScriptToken ObjectToken(LEX_T_OBJECT_LITERAL);
	ObjectToken.line = l->currentLine();
	ObjectToken.column = l->currentColumn();
	auto &Objc = ObjectToken.Object();

	Objc->type = CScriptTokenDataObjectLiteral::ARRAY;
	Objc->destructuring = Objc->structuring = true;
	int idx = 0;

	l->match('[');
	CScriptTokenDataObjectLiteral::ELEMENT element_arrayComprehensionsBody;
	while (l->tk != ']') {
		CScriptTokenDataObjectLiteral::ELEMENT element;
		element.id = std::to_string(idx++);
		if(idx <= 2 && l->tk == LEX_R_FOR) {
			if(idx==1) {
				Objc->type = CScriptTokenDataObjectLiteral::ARRAY_COMPREHENSIONS;
			} else {
				Objc->type = CScriptTokenDataObjectLiteral::ARRAY_COMPREHENSIONS_OLD;
			}
			Objc->destructuring = false;
			State.Tokens.swap(element.value);
			tokenizeFor_inArrayComprehensions(State, Flags, idx==2 ? Objc->elements[0].value : element_arrayComprehensionsBody.value);
			State.Tokens.swap(element.value);
		} else if(l->tk != ',') {
			int dFlags = Flags | ((l->tk == '{' || l->tk == '[') ? TOKENIZE_FLAGS_nestedObject : 0);
			State.pushLeftHandState();
			State.Tokens.swap(element.value);
			tokenizeAssignment(State, dFlags);
			State.Tokens.push_back(CScriptToken(LEX_T_END_EXPRESSION));
			State.Tokens.swap(element.value);
			if(Objc->destructuring) Objc->destructuring = State.LeftHand;
			State.popLeftHandeState();
		}
		Objc->elements.push_back(element);
		if(Objc->type != CScriptTokenDataObjectLiteral::ARRAY) {
			if(Objc->type == CScriptTokenDataObjectLiteral::ARRAY_COMPREHENSIONS)
				Objc->elements.push_back(element_arrayComprehensionsBody);
			break;
		} else if(idx == 1 && l->tk == LEX_R_FOR)
			continue;
		else if (l->tk != ']') {
			l->match(',', ']');
		}
	}
	l->match(']');
	if(Objc->destructuring && Objc->structuring) {
		if(nestedObject) {
			if(l->tk!=',' && l->tk!=']' && l->tk!='}' /* && l->tk!='=' ??? */)
				Objc->destructuring = false;
		}
		else
			Objc->setMode(l->tk=='=' || l->tk==LEX_ARROW || (forFor && (l->tk==LEX_R_IN || (l->tk==LEX_ID && l->tkStr=="of"))));
	} else
		if(!nestedObject) Objc->setMode(Objc->destructuring);
	if(Objc->destructuring)
		State.LeftHand = true;
	State.Tokens.push_back(ObjectToken);
}

bool CScriptTokenizer::_tokenizeArrayComprehensions(ScriptTokenState &State, int Flags) {
	l->match('[');
	TOKEN_VECT value;
	State.Tokens.swap(value);
	tokenizeCondition(State, Flags);
	if(l->tk != LEX_R_FOR) {
		State.Tokens.swap(value);
		return false;
	}
	tokenizeFor(State, Flags);
	State.Tokens.swap(value);
	return false;
}

void CScriptTokenizer::tokenizeLiteral(ScriptTokenState &State, int Flags) {
	State.LeftHand = false;
	bool canLabel = Flags & TOKENIZE_FLAGS_canLabel; Flags &= ~TOKENIZE_FLAGS_canLabel;
	int ObjectLiteralFlags = Flags;
	Flags &= ~TOKENIZE_FLAGS_noIn;
	switch(l->tk) {
#ifndef NO_GENERATORS
	case LEX_R_YIELD:
		if (Flags & TOKENIZE_FLAGS_canYield) {
			pushToken(State.Tokens);
			if (l->tk != ';' && l->tk != '}' && !l->lineBreakBeforeToken) {
				tokenizeExpression(State, Flags);
			}
			//State.FunctionIsGenerator = true;
			break;
		} else
			l->tk = LEX_ID; // yield is only reserved in generator function bodies
		[[fallthrough]];
#endif
	case LEX_ID:
		{
			std::string label = l->tkStr;
			l->match(LEX_ID);
			if(label != "this" && l->tk == LEX_ARROW) { // Arrow-Function  // label => ...
				TOKEN_VECT arguments;
				CScriptToken token(LEX_T_DESTRUCTURING_VAR);
				token.DestructuringVar()->vars.push_back(DESTRUCTURING_VAR_t("", label));
				token.column = l->currentColumn();
				token.line = l->currentLine();
				arguments.push_back(token);
				tokenizeArrowFunction(arguments, State, Flags); 
			} else {
				pushToken(State.Tokens, CScriptToken(LEX_ID, label));
				if(l->tk==':' && canLabel) {
					if(find(State.Labels.begin(), State.Labels.end(), label) != State.Labels.end())
						throw CScriptException(SyntaxError, "dublicate label '"+label+"'", l->currentFile, l->currentLine(), l->currentColumn()-int32_t(label.size()));
					State.Tokens[State.Tokens.size()-1].token = LEX_T_LABEL; // change LEX_ID to LEX_T_LABEL
					State.Labels.push_back(label);
				} else if(label=="this") {
					if( l->tk == '=' || (l->tk >= LEX_ASSIGNMENTS_BEGIN && l->tk <= LEX_ASSIGNMENTS_END) )
						throw CScriptException(SyntaxError, "invalid assignment left-hand side", l->currentFile, l->currentLine(), l->currentColumn()-int32_t(label.size()));
					if( l->tk==LEX_PLUSPLUS || l->tk==LEX_MINUSMINUS )
						throw CScriptException(SyntaxError, l->tk==LEX_PLUSPLUS?"invalid increment operand":"invalid decrement operand", l->currentFile, l->currentLine(), l->currentColumn()-int32_t(label.size()));
				} else
					State.LeftHand = true;
			}
		}
		break;
	case LEX_INT:
	case LEX_FLOAT:
	case LEX_STR:
	case LEX_REGEXP:
	case LEX_R_TRUE:
	case LEX_R_FALSE:
	case LEX_R_NULL:
		pushToken(State.Tokens);
		break;
	case '{':
	case '[':
		if(l->tk == '{')
			_tokenizeLiteralObject(State, ObjectLiteralFlags);
		else
			_tokenizeLiteralArray(State, ObjectLiteralFlags);
		if(State.LeftHand && l->tk==LEX_ARROW) { // [,] => ... or {} => ...
			TOKEN_VECT arguments;
			CScriptToken token(LEX_T_DESTRUCTURING_VAR);
			State.Tokens.back().Object()->toDestructuringVar(token.DestructuringVar());
//			token.DestructuringVar().vars.push_back(DESTRUCTURING_VAR_t("", label));

			arguments.push_back(token);
			State.Tokens.pop_back();
			tokenizeArrowFunction(arguments, State, Flags);
			State.LeftHand = false;
		}
		break;
	case LEX_R_LET: // let as expression
		tokenizeLet(State, Flags);
		break;
	case LEX_R_FUNCTION:
		tokenizeFunction(State, Flags);
		break;
	case LEX_R_NEW:
		State.Marks.push_back(pushToken(State.Tokens)); // push Token & push BeginIdx
		{
			tokenizeFunctionCall(State, (Flags | TOKENIZE_FLAGS_callForNew) & ~TOKENIZE_FLAGS_noIn);
			State.LeftHand = false;
		}
		setTokenSkip(State);
		break;
	case '(':
		{
			l->match('(');
			CScriptLex::POS prev_pos = l->pos;
			if(l->tk==LEX_ID || l->tk=='[' || l->tk=='{' || l->tk==')') {
				try {
					TOKEN_VECT arguments;
					bool arguments_ok = true;
					while(arguments_ok && l->tk != ')') {
						arguments.push_back(tokenizeFunctionArgument());
						if(l->tk == ',') l->match(',');
						else if (l->tk!=')') arguments_ok = false;
					}
					if((arguments_ok = arguments_ok && l->tk == ')')) l->match(')');
					if(arguments_ok && l->tk == LEX_ARROW) {				// ( ... ) =>
						tokenizeArrowFunction(arguments, State, Flags);
						break;
					}
				} catch(...) {
					/* ignore Error -> try regular (...)-expression */
				}
				l->reset(prev_pos);
			}
			State.Marks.push_back(pushToken(State.Tokens, CScriptToken('('))); // push Token & push BeginIdx
			tokenizeExpression(State, Flags & ~TOKENIZE_FLAGS_noIn);
			State.LeftHand = false;
			pushToken(State.Tokens, ')');
			setTokenSkip(State);
		}
		break;
	default:
		l->check(LEX_EOF);
	}
}
void CScriptTokenizer::tokenizeMember(ScriptTokenState &State, int Flags) {
	while(l->tk == '.' || l->tk == LEX_OPTIONAL_CHAINING_MEMBER || l->tk == '[' || l->tk == LEX_OPTIONAL_CHAINING_ARRAY) {
		if(l->tk == '.' || l->tk == LEX_OPTIONAL_CHAINING_MEMBER) {
			pushToken(State.Tokens);
			if(CScriptToken::isReservedWord(l->tk))
				l->tk = LEX_ID; // fake reserved-word as member.ID
			pushToken(State.Tokens , LEX_ID);
		} else {
			State.Marks.push_back(pushToken(State.Tokens));
			State.pushLeftHandState();
			tokenizeExpression(State, Flags & ~TOKENIZE_FLAGS_noIn);
			State.popLeftHandeState();
			pushToken(State.Tokens, ']');
			setTokenSkip(State);
		}
		State.LeftHand = true;
	}
}
void CScriptTokenizer::tokenizeFunctionCall(ScriptTokenState &State, int Flags) {
	bool for_new = (Flags & TOKENIZE_FLAGS_callForNew)!=0; Flags &= ~TOKENIZE_FLAGS_callForNew;
	tokenizeLiteral(State, Flags);
	tokenizeMember(State, Flags);
	while(l->tk == '(' || l->tk == LEX_OPTIONAL_CHANING_FNC /* '?.(' */) {
		State.LeftHand = false;
		State.Marks.push_back(pushToken(State.Tokens)); // push Token & push BeginIdx
		State.pushLeftHandState();
		while(l->tk!=')') {
			tokenizeAssignment(State, Flags & ~TOKENIZE_FLAGS_noIn);
			if (l->tk!=')') pushToken(State.Tokens, ',', ')');
		}
		State.popLeftHandeState();
		pushToken(State.Tokens);
		setTokenSkip(State);
		if(for_new) break;
		tokenizeMember(State, Flags);
	}
}
template<typename T>
constexpr void bubble_sort(T& arr) {
	for (std::size_t i = 0; i < arr.size(); ++i) {
		for (std::size_t j = 0; j < arr.size() - i - 1; ++j) {
			if (arr[j] > arr[j + 1]) {
				auto x = arr[j]; arr[j] = arr[j + 1]; arr[j + 1] = x;
			}
		}
	}
}

void CScriptTokenizer::tokenizeSubExpression(ScriptTokenState &State, int Flags) {
	constexpr auto Left2Right = [] {
		std::array arr{ // Automatische Typ- & Größenableitung
			    /* Precedence 13 */		LEX_ASTERISKASTERISK,
				/* Precedence 12 */		LEX_ASTERISK, LEX_SLASH, LEX_PERCENT,
				/* Precedence 11 */		LEX_PLUS, LEX_MINUS,
				/* Precedence 10*/		LEX_LSHIFT, LEX_RSHIFT, LEX_RSHIFTU,
				/* Precedence 8 */		LEX_EQUAL, LEX_NEQUAL, LEX_TYPEEQUAL, LEX_NTYPEEQUAL,
				/* Precedence 9 */		LEX_LT, LEX_LEQUAL, LEX_GT, LEX_GEQUAL, LEX_R_IN, LEX_R_INSTANCEOF,
				/* Precedence 7-5 */	LEX_AND, LEX_XOR, LEX_OR,
		};
		[](auto & arr) {
			for (std::size_t i = 0; i < arr.size(); ++i) {
				for (std::size_t j = 0; j < arr.size() - i - 1; ++j) {
					if (arr[j] > arr[j + 1]) {
						auto x = arr[j]; arr[j] = arr[j + 1]; arr[j + 1] = x;
					}
				}
			}
		}(arr);
		//bubble_sort(arr);//		std::sort(arr.begin(), arr.end()); // constexpr std::sort() in C++17+
		return arr;
		}();
	bool noLeftHand = false;
	for(;;) {
		bool right2left_end = false;
		while(!right2left_end) {
			switch (l->tk) {
			case '-':
			case '+':
			case '!':
			case '~':
			case LEX_R_TYPEOF:
			case LEX_R_VOID:
			case LEX_R_DELETE:
			//case LEX_R_AWAIT:
				Flags &= ~TOKENIZE_FLAGS_canLabel;
				noLeftHand = true;
				pushToken(State.Tokens); // Precedence 14
				break;
			case LEX_PLUSPLUS: // pre-increment
			case LEX_MINUSMINUS: { // pre-decrement
					int tk = l->tk;
					Flags &= ~TOKENIZE_FLAGS_canLabel;
					noLeftHand = true;
					pushToken(State.Tokens); // Precedence 14
					if (l->tk == LEX_ID && l->tkStr == "this")
						throw CScriptException(SyntaxError, tk == LEX_PLUSPLUS ? "invalid increment operand" : "invalid decrement operand", l->currentFile, l->currentLine(), l->currentColumn());
					[[fallthrough]];
				}
			default:
				right2left_end = true;
			}
		}
		tokenizeFunctionCall(State, Flags);

		if (!l->lineBreakBeforeToken && (l->tk==LEX_PLUSPLUS || l->tk==LEX_MINUSMINUS)) { // post-in-/de-crement
			noLeftHand = true;;
			pushToken(State.Tokens); // Precedence 15
		}
		if(Flags&TOKENIZE_FLAGS_noIn && l->tk==LEX_R_IN)
			break;
		if (std::binary_search(Left2Right.begin(), Left2Right.end(), l->tk)) {
			noLeftHand = true;
			pushToken(State.Tokens); // Precedence 12-5
		} else
			break;
	}
	if(noLeftHand) State.LeftHand = false;
}

void CScriptTokenizer::tokenizeLogic(ScriptTokenState &State, int Flags, int op /*= LEX_OROR*/, int op_n /*= LEX_ANDAND*/) {
	op_n ? tokenizeLogic(State, Flags, op_n, 0) : tokenizeSubExpression(State, Flags);
	if(l->tk==op || (op==LEX_OROR && l->tk == LEX_ASKASK)) {
		size_t marks_count = State.Marks.size();
		while(l->tk==op || (op == LEX_OROR && l->tk == LEX_ASKASK)) {
			State.Marks.push_back(pushToken(State.Tokens));
			op_n ? tokenizeLogic(State, Flags, op_n, 0) : tokenizeSubExpression(State, Flags);
		}
		while(State.Marks.size()>marks_count) setTokenSkip(State);
		State.LeftHand = false;
	}
}

void CScriptTokenizer::tokenizeCondition(ScriptTokenState &State, int Flags) {
	tokenizeLogic(State, Flags);
	if(l->tk == '?') {
		Flags &= ~(TOKENIZE_FLAGS_noIn | TOKENIZE_FLAGS_canLabel);
		State.Marks.push_back(pushToken(State.Tokens));
		tokenizeAssignment(State, Flags);
		setTokenSkip(State);
		State.Marks.push_back(pushToken(State.Tokens, ':'));
		tokenizeAssignment(State, Flags);
		setTokenSkip(State);
		State.LeftHand = false;
	}
}
void CScriptTokenizer::tokenizeAssignment(ScriptTokenState &State, int Flags) {
	tokenizeCondition(State, Flags);
	if (l->tk=='=' || (l->tk>=LEX_ASSIGNMENTS_BEGIN && l->tk<=LEX_ASSIGNMENTS_END)) {
		if(!State.LeftHand)
			throw CScriptException(ReferenceError, "invalid assignment left-hand side", l->currentFile, l->currentLine(), l->currentColumn());
		pushToken(State.Tokens);
		tokenizeAssignment(State, Flags);
		State.LeftHand = false;
	}
}
void CScriptTokenizer::tokenizeExpression(ScriptTokenState &State, int Flags) {
	tokenizeAssignment(State, Flags);
	while(l->tk == ',') {
		pushToken(State.Tokens);
		tokenizeAssignment(State, Flags);
		State.LeftHand = false;
	}
}
void CScriptTokenizer::tokenizeBlock(ScriptTokenState &State, int Flags) {
	bool addBlockStart = (Flags&TOKENIZE_FLAGS_noBlockStart)==0;
	Flags&=~(TOKENIZE_FLAGS_noBlockStart);
	State.Marks.push_back(pushToken(State.Tokens, '{')); // push Token & push BeginIdx
	if(addBlockStart) pushForwarder(State);

	while(l->tk != '}' && l->tk != LEX_EOF)
		tokenizeStatement(State, Flags);
	pushToken(State.Tokens, '}');

	if(addBlockStart) removeEmptyForwarder(State); // clean-up BlockStarts

	setTokenSkip(State);
}

void CScriptTokenizer::tokenizeStatementNoLet(ScriptTokenState &State, int Flags) {
	if(l->tk == LEX_R_LET)
		tokenizeLet(State, Flags | TOKENIZE_FLAGS_asStatement, true);
	else if(l->tk==LEX_R_FUNCTION)
		tokenizeFunction(State, Flags | TOKENIZE_FLAGS_asStatement, true);
	else
		tokenizeStatement(State, Flags);
}
void CScriptTokenizer::tokenizeStatement(ScriptTokenState &State, int Flags) {
	int tk = l->tk;
	switch (l->tk)
	{
	case '{':				tokenizeBlock(State, Flags); break;
	case ';':				pushToken(State.Tokens); break;
	case LEX_R_CONST:
	case LEX_R_VAR:			tokenizeVarAndConst(State, Flags); break;
	case LEX_R_LET:			tokenizeLet(State, Flags | TOKENIZE_FLAGS_asStatement); break;
	case LEX_R_WITH:		tokenizeWith(State, Flags); break;
	case LEX_R_IF:			tokenizeIf(State, Flags); break;
	case LEX_R_SWITCH:		tokenizeSwitch(State, Flags); break;
	case LEX_R_DO:
	case LEX_R_WHILE:		tokenizeWhileAndDo(State, Flags); break;
	case LEX_R_FOR:			tokenizeFor(State, Flags); break;
	case LEX_R_FUNCTION:	tokenizeFunction(State, Flags | TOKENIZE_FLAGS_asStatement); break;
	case LEX_R_TRY:			tokenizeTry(State, Flags); break;
	case LEX_R_RETURN:
		if ((Flags & TOKENIZE_FLAGS_canReturn) == 0)
			throw CScriptException(SyntaxError, "'return' statement, but not in a function.", l->currentFile, l->currentLine(), l->currentColumn());
		[[fallthrough]];
	case LEX_R_THROW:
		State.Marks.push_back(pushToken(State.Tokens)); // push Token & push BeginIdx
		if (l->tk != ';' && l->tk != '}' && !l->lineBreakBeforeToken) {
			if (tk == LEX_R_RETURN) State.HaveReturnValue = true;
			tokenizeExpression(State, Flags);
		}
		pushToken(State.Tokens, ';'); // push ';'
		setTokenSkip(State);
		break;
	case LEX_R_BREAK:
	case LEX_R_CONTINUE:
		{
			bool isBreak = l->tk == LEX_R_BREAK;
			State.Marks.push_back(pushToken(State.Tokens)); // push Token

			if (l->tk != ';' && !l->lineBreakBeforeToken) {
				l->check(LEX_ID);
				STRING_VECTOR_t& L = isBreak ? State.Labels : State.LoopLabels;
				if (find(L.begin(), L.end(), l->tkStr) == L.end())
					throw CScriptException(SyntaxError, "label '" + l->tkStr + "' not found", l->currentFile, l->currentLine(), l->currentColumn());
				pushToken(State.Tokens); // push 'Label'
			}
			else if ((Flags & (isBreak ? TOKENIZE_FLAGS_canBreak : TOKENIZE_FLAGS_canContinue)) == 0)
				throw CScriptException(SyntaxError,
					isBreak ? "'break' must be inside loop, switch or labeled statement" : "'continue' must be inside loop",
					l->currentFile, l->currentLine(), l->currentColumn());
			pushToken(State.Tokens, ';'); // push ';'
			setTokenSkip(State);
		}
		break;
	case LEX_ID:
		{
			State.Marks.push_back(pushToken(State.Tokens, CScriptToken(LEX_T_SKIP))); // push skip & skiperBeginIdx
			STRING_VECTOR_t::size_type label_count = State.Labels.size();
			tokenizeExpression(State, Flags | TOKENIZE_FLAGS_canLabel);
			if (label_count < State.Labels.size() && l->tk == ':') {
				State.Tokens.erase(State.Tokens.begin() + State.Marks.back()); // remove skip
				State.Marks.pop_back();
				pushToken(State.Tokens); // push ':'
				tokenizeStatement(State, Flags);
				State.Labels.pop_back();
			}
			else {
				pushToken(State.Tokens, ';');
				setTokenSkip(State);
			}
		}
		break;
	case 0:
		break;
	default:
		State.Marks.push_back(pushToken(State.Tokens, CScriptToken(LEX_T_SKIP))); // push skip & skiperBeginIdx
		tokenizeExpression(State, Flags);
		pushToken(State.Tokens, ';');
		setTokenSkip(State);
		break;
	}

}

size_t CScriptTokenizer::pushToken(TOKEN_VECT &Tokens, int Match, int Alternate) {
	if(Match == ';' && l->tk != ';' && (l->lineBreakBeforeToken || l->tk=='}' || l->tk==LEX_EOF))
		Tokens.push_back(CScriptToken(';')); // inject ';'
	else
		Tokens.push_back(CScriptToken(l, Match, Alternate));
	return Tokens.size()-1;
}
size_t CScriptTokenizer::pushToken(TOKEN_VECT &Tokens, const CScriptToken &Token) {
	size_t ret = Tokens.size();
	Tokens.push_back(Token);
	return ret;
}
void CScriptTokenizer::pushForwarder(TOKEN_VECT &Tokens, FORWARDER_VECTOR_t &Forwarders, MARKS_t &Marks) {
	Marks.push_back(Tokens.size());
	CScriptToken token(LEX_T_FORWARD);
	Tokens.push_back(token);
	Forwarders.push_back(token.Forwarder());
}
void CScriptTokenizer::pushForwarder(ScriptTokenState &State, bool noMarks/*=false*/) {
	if(!noMarks) State.Marks.push_back(State.Tokens.size());
	CScriptToken token(LEX_T_FORWARD);
	State.Tokens.push_back(token);
	State.Forwarders.push_back(token.Forwarder());
}
void CScriptTokenizer::removeEmptyForwarder(ScriptTokenState &State)
{
	CScriptTokenDataForwardsPtr &forwarder = State.Forwarders.back();
	forwarder->vars_in_letscope.clear();
	if(forwarder->empty())
		State.Tokens.erase(State.Tokens.begin()+State.Marks.back());
	State.Forwarders.pop_back();
	State.Marks.pop_back();
}

void CScriptTokenizer::removeEmptyForwarder(TOKEN_VECT &Tokens, FORWARDER_VECTOR_t &Forwarders, MARKS_t &Marks) {
	CScriptTokenDataForwardsPtr &forwarder = Forwarders.back();
	forwarder->vars_in_letscope.clear();
	if(forwarder->empty())
		Tokens.erase(Tokens.begin()+Marks.back());
	Forwarders.pop_back();
	Marks.pop_back();
}
void CScriptTokenizer::throwTokenNotExpected() {
	throw CScriptException(SyntaxError, "'"+CScriptToken::getTokenStr(l->tk, l->tkStr.c_str())+"' was not expected", l->currentFile, l->currentLine(), l->currentColumn());
}

//////////////////////////////////////////////////////////////////////////
/// CScriptPropertyName
//////////////////////////////////////////////////////////////////////////

int64_t CScriptPropertyName::name2arrayIdx(const std::string &Name) {
	const char *c_str = Name.c_str();
	std::string::size_type size = Name.size();
	if (size == 0 || !isNumeric(*c_str) || (size > 1 && *c_str == '0') || (size > 9 && *c_str > '4')) return -1; // empty or (more as 1 digit and beginning with '0')
	int64_t result = 0;
	bool overflow = false;
	for (; *c_str; ++c_str) {
		if (!isNumeric(*c_str)) return -1;
		result = (result << 1) + (result << 3) + int64_t(*c_str - '0');
		if(result >= 0x100000000LL) return -1;
	}
	return result;
}

//////////////////////////////////////////////////////////////////////////
/// CScriptVar
//////////////////////////////////////////////////////////////////////////
#if _DEBUG
static uint32_t currentDebugID = 0;
#endif
CScriptVar::CScriptVar(CTinyJS *Context, const CScriptVarPtr &Prototype) {
	extensible = true;
	context = Context;
	memset(temporaryMark, 0, sizeof(temporaryMark));
	if(context->first) {
		next = context->first;
		next->prev = this;
	} else {
		next = 0;
	}
	context->first = this;
	prev = 0;
	refs = 0;
	if (Prototype) {
		prototype = Prototype->ref();
	} else
		prototype = 0;
#if DEBUG_MEMORY
	mark_allocated(this);
#endif
#if _DEBUG
	debugID = currentDebugID++;
#endif
}
CScriptVar::~CScriptVar(void) {
#if DEBUG_MEMORY
	mark_deallocated(this);
#endif
	for(SCRIPTVAR_CHILDS_it it = Childs.begin(); it != Childs.end(); ++it)
		(*it)->setOwner(0);
	removeAllChildren();
	if (prototype) prototype->unref();
	if(prev)
		prev->next = next;
	else
		context->first = next;
	if(next)
		next->prev = prev;
}

CScriptVarPtr CScriptVar::getPrototype() {
	return prototype;
}

void CScriptVar::setPrototype(const CScriptVarPtr &Prototype) {
	if (prototype) { prototype->unref(); }
	if((prototype = Prototype.getVar())) prototype->ref();
}

/// Type

bool CScriptVar::isObject()		{return false;}
bool CScriptVar::isError()			{return false;}
bool CScriptVar::isArray()			{return false;}
bool CScriptVar::isDate()			{return false;}
bool CScriptVar::isRegExp()		{return false;}
bool CScriptVar::isAccessor()		{return false;}
bool CScriptVar::isNull()			{return false;}
bool CScriptVar::isUndefined()	{return false;}
bool CScriptVar::isNullOrUndefined() { return isNull() || isUndefined(); }
bool CScriptVar::isNaN()			{return false;}
bool CScriptVar::isString()		{return false;}
bool CScriptVar::isInt()			{return false;}
bool CScriptVar::isBool()			{return false;}
int CScriptVar::isInfinity()		{ return 0; } ///< +1==POSITIVE_INFINITY, -1==NEGATIVE_INFINITY, 0==is not an InfinityVar
bool CScriptVar::isDouble()		{return false;}
bool CScriptVar::isRealNumber()	{return false;}
bool CScriptVar::isNumber()		{return false;}
bool CScriptVar::isPrimitive()	{return false;}
bool CScriptVar::isFunction()		{return false;}
bool CScriptVar::isNative()		{return false;}
bool CScriptVar::isBounded()		{return false;}
bool CScriptVar::isIterator()		{return false;}
bool CScriptVar::isGenerator()	{return false;}

//////////////////////////////////////////////////////////////////////////
/// Value
//////////////////////////////////////////////////////////////////////////

CScriptVarPrimitivePtr CScriptVar::getRawPrimitive() {
	return CScriptVarPrimitivePtr(); // default NULL-Ptr
}
CScriptVarPrimitivePtr CScriptVar::toPrimitive() {
	return toPrimitive_hintNumber();
}

CScriptVarPrimitivePtr CScriptVar::toPrimitive(CScriptResult &execute) {
	return toPrimitive_hintNumber(execute);
}

CScriptVarPrimitivePtr CScriptVar::toPrimitive_hintString(int32_t radix) {
	CScriptResult execute;
	CScriptVarPrimitivePtr var = toPrimitive_hintString(execute, radix);
	execute.cThrow();
	return var;
}
CScriptVarPrimitivePtr CScriptVar::toPrimitive_hintString(CScriptResult &execute, int32_t radix) {
	if(execute) {
		if(!isPrimitive()) {
			CScriptVarPtr ret = callJS_toString(execute, radix);
			if(execute && !ret->isPrimitive()) {
				ret = callJS_valueOf(execute);
				if(execute && !ret->isPrimitive())
					context->throwError(execute, TypeError, "can't convert to primitive type");
			}
			return ret;
		}
		return this;
	}
	return constScriptVar(Undefined);
}
CScriptVarPrimitivePtr CScriptVar::toPrimitive_hintNumber() {
	CScriptResult execute;
	CScriptVarPrimitivePtr var = toPrimitive_hintNumber(execute);
	execute.cThrow();
	return var;
}
CScriptVarPrimitivePtr CScriptVar::toPrimitive_hintNumber(CScriptResult &execute) {
	if(execute) {
		if(!isPrimitive()) {
			CScriptVarPtr ret = callJS_valueOf(execute);
			if(execute && !ret->isPrimitive()) {
				ret = callJS_toString(execute);
				if(execute && !ret->isPrimitive())
					context->throwError(execute, TypeError, "can't convert to primitive type");
			}
			return ret;
		}
		return this;
	}
	return constScriptVar(Undefined);
}

CScriptVarPtr CScriptVar::callJS_valueOf(CScriptResult &execute) {
	if(execute) {
		CScriptVarPtr FncValueOf = findChildWithPrototypeChain("valueOf").getter(execute);
		if(FncValueOf && FncValueOf != context->objectPrototype_valueOf) { // custom valueOf in JavaScript
			if(FncValueOf->isFunction()) { // no Error if toString not callable
				std::vector<CScriptVarPtr> Params;
				return context->callFunction(execute, FncValueOf, Params, this);
			}
		} else
			return valueOf_CallBack();
	}
	return this;
}
CScriptVarPtr CScriptVar::valueOf_CallBack() {
	return this;
}

CScriptVarPtr CScriptVar::callJS_toString(CScriptResult &execute, int radix/*=0*/) {
	if(execute) {
		CScriptVarPtr FncToString = findChildWithPrototypeChain("toString").getter(execute);
		if(FncToString && FncToString != context->objectPrototype_toString) { // custom valueOf in JavaScript
			if(FncToString->isFunction()) { // no Error if toString not callable
				std::vector<CScriptVarPtr> Params;
				Params.push_back(newScriptVar(radix));
				return context->callFunction(execute, FncToString, Params, this);
			}
		} else
			return toString_CallBack(execute, radix);
	}
	return this;
}
CScriptVarPtr CScriptVar::toString_CallBack(CScriptResult &execute, int radix/*=0*/) {
	return this;
}

CNumber CScriptVar::toNumber() { return toPrimitive_hintNumber()->toNumber_Callback(); };
CNumber CScriptVar::toNumber(CScriptResult &execute) { return toPrimitive_hintNumber(execute)->toNumber_Callback(); };
bool CScriptVar::toBoolean() { return true; }
std::string CScriptVar::toString(int32_t radix) { return toPrimitive_hintString(radix)->toCString(radix); }
std::string CScriptVar::toString(CScriptResult &execute, int32_t radix/*=0*/) { return toPrimitive_hintString(execute, radix)->toCString(radix); }

int CScriptVar::getInt() { return toNumber().toInt32(); }
double CScriptVar::getDouble() { return toNumber().toDouble(); }
bool CScriptVar::getBool() { return toBoolean(); }
std::string CScriptVar::getString() { return toPrimitive_hintString()->toCString(); }

void CScriptVar::setter(CScriptResult &execute, const CScriptVarLinkPtr &link, const CScriptVarPtr &value) {
	link->setVarPtr(value);
}
CScriptVarLinkPtr &CScriptVar::getter(CScriptResult &execute, CScriptVarLinkPtr &link) {
	return link;
}

const std::shared_ptr<CScriptTokenDataFnc> CScriptVar::getFunctionData() const
{ return std::shared_ptr<CScriptTokenDataFnc>(); }

CScriptVarPtr CScriptVar::toIterator(IteratorMode Mode/*=RETURN_ARRAY*/) {
	CScriptResult execute;
	CScriptVarPtr var = toIterator(execute, Mode);
	execute.cThrow();
	return var;
}
CScriptVarPtr CScriptVar::toIterator(CScriptResult &execute, IteratorMode Mode/*=RETURN_ARRAY*/) {
	if(!execute) return constScriptVar(Undefined);
	if(isIterator()) return this;
	CScriptVarFunctionPtr Generator(findChildWithPrototypeChain("__iterator__").getter(execute));
	std::vector<CScriptVarPtr> args;
	if(Generator) return context->callFunction(execute, Generator, args, this);
	return newScriptVarDefaultIterator(context, this, Mode);
}

std::string CScriptVar::getParsableString() {
	uint32_t UniqueID = context->allocUniqueID();
	bool hasRecursion=false;
	std::string ret = getParsableString("", "   ", UniqueID, hasRecursion);
	context->freeUniqueID();
	return ret;
}

std::string CScriptVar::getParsableString(const std::string &indentString, const std::string &indent, uint32_t uniqueID, bool &hasRecursion) {
	getParsableStringRecursionsCheckBegin();
	std::string ret = indentString + toString();
	getParsableStringRecursionsCheckEnd();
	return ret;
}

CScriptVarPtr CScriptVar::getNumericVar() { return newScriptVar(toNumber()); }

////// Flags

void CScriptVar::seal() {
	preventExtensions();
	for(SCRIPTVAR_CHILDS_it it = Childs.begin(); it != Childs.end(); ++it)
		(*it)->setConfigurable(false);
}
bool CScriptVar::isSealed() const {
	if(isExtensible()) return false;
	for(SCRIPTVAR_CHILDS_cit it = Childs.begin(); it != Childs.end(); ++it)
		if((*it)->isConfigurable()) return false;
	return true;
}
void CScriptVar::freeze() {
	preventExtensions();
	for(SCRIPTVAR_CHILDS_it it = Childs.begin(); it != Childs.end(); ++it)
		(*it)->setConfigurable(false), (*it)->setWritable(false);
}
bool CScriptVar::isFrozen() const {
	if(isExtensible()) return false;
	for(SCRIPTVAR_CHILDS_cit it = Childs.begin(); it != Childs.end(); ++it)
		if((*it)->isConfigurable() || (*it)->isWritable()) return false;
	return true;
}

////// Childs

CScriptVarPtr CScriptVar::getOwnPropertyDescriptor(const std::string &Name) {
	CScriptVarLinkPtr child = getOwnProperty(Name);
	if(!child || child->getVarPtr()->isUndefined()) return constScriptVar(Undefined);
	CScriptVarPtr ret = newScriptVar(Object);
	if(child->getVarPtr()->isAccessor()) {
		CScriptVarLinkPtr value = child->getVarPtr()->findChild(TINYJS_ACCESSOR_GET_VAR);
		ret->addChild("get", value ? value->getVarPtr() : constScriptVar(Undefined));
		value = child->getVarPtr()->findChild(TINYJS_ACCESSOR_SET_VAR);
		ret->addChild("set", value ? value->getVarPtr() : constScriptVar(Undefined));
	} else {
		ret->addChild("value", child->getVarPtr()->valueOf_CallBack());
		ret->addChild("writable", constScriptVar(child->isWritable()));
	}
	ret->addChild("enumerable", constScriptVar(child->isEnumerable()));
	ret->addChild("configurable", constScriptVar(child->isConfigurable()));
	return ret;
}
const char *CScriptVar::defineProperty(const std::string &Name, CScriptVarPtr Attributes) {
	CScriptVarPtr attr;
	CScriptVarLinkPtr child = getOwnProperty(Name);

	CScriptVarPtr attr_value			= Attributes->findChild("value");
	CScriptVarPtr attr_writable			= Attributes->findChild("writable");
	CScriptVarPtr attr_get				= Attributes->findChild("get");
	CScriptVarPtr attr_set				= Attributes->findChild("set");
	CScriptVarPtr attr_enumerable		= Attributes->findChild("enumerable");
	CScriptVarPtr attr_configurable		= Attributes->findChild("configurable");
	bool attr_isAccessorDescriptor		= attr_get || attr_set || (!attr_value && child && child->getVarPtr()->isAccessor());
	if(attr_isAccessorDescriptor) {
		if(attr_value || attr_writable) return "property descriptors must not specify a value or be writable when a getter or setter has been specified";
		if(attr_get && (!attr_get->isUndefined() || !attr_get->isFunction())) return "property descriptor's getter field is neither undefined nor a function";
		if(attr_set && (!attr_set->isUndefined() || !attr_set->isFunction())) return "property descriptor's setter field is neither undefined nor a function";
	}
	if(!child) {
		if(!isExtensible()) return "is not extensible";
		if(!attr_isAccessorDescriptor) {
			child = addChild(Name, 0, 0).setter(attr_value?attr_value:constScriptVar(Undefined));
			if(attr_writable) child->setWritable(attr_writable->toBoolean());
		} else {
			child = addChild(Name, 0, attr_set ? SCRIPTVARLINK_WRITABLE : 0).setter(newScriptVarAccessor(context, attr_get, attr_set));
		}
	} else {
		if(!child->isConfigurable()) {
			if(attr_configurable && attr_configurable->toBoolean()) goto cant_redefine;
			if(attr_enumerable && attr_enumerable->toBoolean() != child->isEnumerable()) goto cant_redefine;
			if(child->getVarPtr()->isAccessor()) {
				if(!attr_isAccessorDescriptor) goto cant_redefine;
				if(attr_get && attr_get != child->getVarPtr()->findChild(TINYJS_ACCESSOR_GET_VAR).operator const CScriptVarPtr &()) goto cant_redefine;
				if(attr_set && attr_set != child->getVarPtr()->findChild(TINYJS_ACCESSOR_SET_VAR).operator const CScriptVarPtr &()) goto cant_redefine;
			} else if(attr_isAccessorDescriptor)
				goto cant_redefine;
			else if(!child->isWritable()) {
				if(attr_writable && attr_writable->toBoolean()) goto cant_redefine;
				if(attr_value && !attr_value->mathsOp(child, LEX_EQUAL)->toBoolean()) goto cant_redefine;
			}
		}
		if(!attr_isAccessorDescriptor) {
			if(child->getVarPtr()->isAccessor() && !attr_writable) child->setWritable(false);
			child->setVarPtr(0); child.setter(attr_value?attr_value:constScriptVar(Undefined));
			if(attr_writable) child->setWritable(attr_writable->toBoolean());
		} else {
			if(child->getVarPtr()->isAccessor()) {
				if(!attr_get) attr_get = child->getVarPtr()->findChild(TINYJS_ACCESSOR_GET_VAR);
				if(!attr_set) attr_set = child->getVarPtr()->findChild(TINYJS_ACCESSOR_SET_VAR);
			}
			child->setVarPtr(0); child.setter(newScriptVarAccessor(context, attr_get, attr_set));
			child->setWritable(true);
		}
	}
	if(attr_enumerable) child->setEnumerable(attr_enumerable->toBoolean());
	if(attr_configurable) child->setConfigurable(attr_configurable->toBoolean());
	return 0;
cant_redefine:
	return "can't redefine non-configurable property";
}

CScriptVarLinkPtr CScriptVar::findChild(const std::string &childName) {
	if(Childs.empty()) return 0;
	SCRIPTVAR_CHILDS_it it = lower_bound(Childs.begin(), Childs.end(), childName);
	if(it != Childs.end() && (*it)->getName() == childName)
		return *it;
	return 0;
}

CScriptVarLinkWorkPtr CScriptVar::findChildWithStringChars(const std::string &childName) {
	return getOwnProperty(childName);
/*

	CScriptVarLinkWorkPtr child = findChild(childName);
	if(child) return child;
	CScriptVarStringPtr strVar = getRawPrimitive();
	uint32_t Idx;
	if (strVar && (Idx=isArrayIndex(childName))!=uint32_t(-1)) {
		if (Idx<strVar->getLength()) {
			int Char = strVar->getChar(Idx);
			child(newScriptVar(std::string(1, (char)Char)), childName, SCRIPTVARLINK_ENUMERABLE);
		} else {
			child(constScriptVar(Undefined), childName, SCRIPTVARLINK_ENUMERABLE);
		}
		child.setReferencedOwner(this); // fake referenced Owner
		return child;
	}
	return 0;
*/
}

CScriptVarLinkWorkPtr CScriptVar::getOwnProperty(const std::string &childName) {
	return findChild(childName);
}

CScriptVarLinkPtr CScriptVar::findChildInPrototypeChain(const std::string &childName) {
	unsigned int uniqueID = context->allocUniqueID();
	// Look for links to actual parent classes
	CScriptVarPtr object = this;
	CScriptVarPtr __proto__;
	while( object->getTemporaryMark() != uniqueID && (__proto__ = object->getPrototype()) ) {
		CScriptVarLinkPtr implementation = __proto__->findChild(childName);
		if (implementation) {
			context->freeUniqueID();
			return implementation;
		}
		object->setTemporaryMark(uniqueID); // prevents recursions
		object = __proto__;
	}
	context->freeUniqueID();
	return 0;
}

CScriptVarLinkWorkPtr CScriptVar::findChildWithPrototypeChain(const std::string &childName) {
	CScriptVarLinkWorkPtr child = getOwnProperty(childName);
	if(child) return child;
	child = findChildInPrototypeChain(childName);
	if(child) {
		child(child->getVarPtr(), child->getName(), child->getFlags()); // recreate implementation
		child.setReferencedOwner(this); // fake referenced Owner
	}
	return child;
}
CScriptVarLinkPtr CScriptVar::findChildByPath(const std::string &path) {
	std::string::size_type p = path.find('.');
	CScriptVarLinkPtr child;
	if (p == std::string::npos)
		return findChild(path);
	if( (child = findChild(path.substr(0,p))) )
		return child->getVarPtr()->findChildByPath(path.substr(p+1));
	return 0;
}

CScriptVarLinkPtr CScriptVar::findChildOrCreate(const std::string &childName/*, int varFlags*/) {
	CScriptVarLinkPtr l = findChild(childName);
	if (l) return l;
	return addChild(childName, constScriptVar(Undefined));
	//	return addChild(childName, new CScriptVar(context, TINYJS_BLANK_DATA, varFlags));
}

CScriptVarLinkPtr CScriptVar::findChildOrCreateByPath(const std::string &path) {
	std::string::size_type p = path.find('.');
	if (p == std::string::npos)
		return findChildOrCreate(path);
	std::string childName(path, 0, p);
	CScriptVarLinkPtr l = findChild(childName);
	if (!l) l = addChild(childName, newScriptVar(Object));
	return l->getVarPtr()->findChildOrCreateByPath(path.substr(p+1));
}

void CScriptVar::keys(std::set<std::string> &Keys, bool OnlyEnumerable/*=true*/, uint32_t ID/*=0*/) {
	if(ID) setTemporaryMark(ID);
	for(SCRIPTVAR_CHILDS_it it = Childs.begin(); it != Childs.end(); ++it) {
		if(!OnlyEnumerable || (*it)->isEnumerable())
			Keys.insert((*it)->getName());
	}
	CScriptVarStringPtr isStringObj = this->getRawPrimitive();
	if(isStringObj) {
		size_t length = isStringObj->getLength();
		for(size_t i=0; i<length; ++i)
			Keys.insert(std::to_string(i));
	}
	if( ID && prototype && prototype->getTemporaryMark() != ID )
		prototype->keys(Keys, OnlyEnumerable, ID);
}

/// add & remove
CScriptVarLinkPtr CScriptVar::addChild(const std::string &childName, const CScriptVarPtr &child, int linkFlags /*= SCRIPTVARLINK_DEFAULT*/) {
	CScriptVarLinkPtr link;
	SCRIPTVAR_CHILDS_it it = lower_bound(Childs.begin(), Childs.end(), childName);
	if(it == Childs.end() || (*it)->getName() != childName) {
		link = CScriptVarLinkPtr(child?child:constScriptVar(Undefined), childName, linkFlags);
		link->setOwner(this);

		Childs.insert(it, 1, link);
#ifdef _DEBUG
	} else {
		ASSERT(0); // addChild - the child exists
#endif
	}
	return link;
}
CScriptVarLinkPtr CScriptVar::addChild(uint32_t childName, const CScriptVarPtr &child, int linkFlags /*= SCRIPTVARLINK_DEFAULT*/) {
	return addChild(std::to_string(childName), child, linkFlags);
}

CScriptVarLinkPtr CScriptVar::addChildNoDup(const std::string &childName, const CScriptVarPtr &child, int linkFlags /*= SCRIPTVARLINK_DEFAULT*/) {
	return addChildOrReplace(childName, child, linkFlags);
}
CScriptVarLinkPtr CScriptVar::addChildOrReplace(const std::string &childName, const CScriptVarPtr &child, int linkFlags /*= SCRIPTVARLINK_DEFAULT*/) {
	SCRIPTVAR_CHILDS_it it = lower_bound(Childs.begin(), Childs.end(), childName);
	if(it == Childs.end() || (*it)->getName() != childName) {
		CScriptVarLinkPtr link(child, childName, linkFlags);
		link->setOwner(this);
		Childs.insert(it, 1, link);
		return link;
	} else {
		(*it)->setVarPtr(child);
		return (*it);
	}
}
CScriptVarLinkPtr CScriptVar::addChildOrReplace(uint32_t childName, const CScriptVarPtr &child, int linkFlags /*= SCRIPTVARLINK_DEFAULT*/) {
	return addChildOrReplace(std::to_string(childName), child, linkFlags);
}

bool CScriptVar::removeLink(CScriptVarLinkPtr &link) {
	if (!link) return false;
	SCRIPTVAR_CHILDS_it it = lower_bound(Childs.begin(), Childs.end(), link->getName());
	if(it != Childs.end() && (*it) == link) {
		Childs.erase(it);
#ifdef _DEBUG
	} else {
		ASSERT(0); // removeLink - the link is not atached to this var
#endif
	}
	link.clear();
	return true;
}

bool CScriptVar::removeChild(const std::string &childName) {
	SCRIPTVAR_CHILDS_it it = lower_bound(Childs.begin(), Childs.end(), childName);
	if(it != Childs.end() && (*it)->getName() == childName) {
		Childs.erase(it);
#ifdef _DEBUG
	} else {
		ASSERT(0); // removeLink - the link is not atached to this var
#endif
	}
	return true;
}

void CScriptVar::removeAllChildren() {
	Childs.clear();
}

CScriptVarPtr CScriptVar::getProperty(const std::string &name) {
	return findChildWithPrototypeChain(name).getter();
}

CScriptVarPtr CScriptVar::getProperty(uint32_t idx) {
	return findChildWithPrototypeChain(std::to_string(idx)).getter();
}

void CScriptVar::setProperty(const std::string &name, const CScriptVarPtr &rhs, bool ignoreReadOnly/*=false*/, bool ignoreNotExtensible/*=false*/)
{
	CScriptVarLinkWorkPtr lhs = findChildWithPrototypeChain(name);
	if(lhs->isWritable()) {
		if (!lhs->isOwned()) {
			CScriptVarPtr fakedOwner = lhs.getReferencedOwner();
			if(fakedOwner) {
				if(!fakedOwner->isExtensible()) {
					if(ignoreNotExtensible) return;
					throw newScriptVarError(context, TypeError, (lhs->getName() +" is not extensible").c_str());
				}
				lhs = fakedOwner->addChildOrReplace(lhs->getName(), lhs);
			}
		}
		lhs.setter(rhs);
	} else if(!ignoreReadOnly)
		throw newScriptVarError(context, TypeError, (lhs->getName() +" is read-only").c_str());
}

void CScriptVar::setProperty(uint32_t idx, const CScriptVarPtr &value, bool ignoreReadOnly/*=false*/, bool ignoreNotExtensible/*=false*/) {
	setProperty(std::to_string(idx), value, ignoreReadOnly, ignoreNotExtensible);
}

uint32_t CScriptVar::getLength() {
	CScriptVarLinkWorkPtr link = findChildWithPrototypeChain("length");
	if(link) {
		CNumber number = link.getter()->toNumber();
		if(number.isUInt32()) return number.toUInt32();
	}
	return 0;
}


CScriptVarPtr CScriptVar::getArrayIndex(uint32_t idx) {
	CScriptVarLinkPtr link = findChild(std::to_string(idx));
	if (link) return link;
	else return constScriptVar(Undefined); // undefined
}
void CScriptVar::setArrayIndex(uint32_t idx, const CScriptVarPtr &value) {
	std::string sIdx = std::to_string(idx);
	CScriptVarLinkPtr link = findChild(sIdx);

	if (link) {
		link->setVarPtr(value);
	} else {
		addChild(sIdx, value);
	}
}
uint32_t CScriptVar::getArrayLength() {
	if(isArray()) return getLength();
	return 0;
}

CScriptVarPtr CScriptVar::mathsOp(const CScriptVarPtr &b, int op) {
	CScriptResult execute;
	return context->mathsOp(execute, this, b, op);
}

void CScriptVar::trace(const std::string &name) {
	std::string indentStr;
	uint32_t uniqueID = context->allocUniqueID();
	trace(indentStr, uniqueID, name);
	context->freeUniqueID();
}
void CScriptVar::trace(std::string &indentStr, uint32_t uniqueID, const std::string &name) {
	std::string indent = "  ";
	const char *extra="";
	if(getTemporaryMark() == uniqueID)
		extra = " recursion detected";
	TRACE("%s'%s' = '%s' %s%s\n",
		indentStr.c_str(),
		name.c_str(),
		toString().c_str(),
		getFlagsAsString().c_str(),
		extra);
	if(getTemporaryMark() != uniqueID) {
		setTemporaryMark(uniqueID);
		indentStr+=indent;
		for(SCRIPTVAR_CHILDS_it it = Childs.begin(); it != Childs.end(); ++it) {
			if((*it)->isEnumerable())
				(*it)->getVarPtr()->trace(indentStr, uniqueID, (*it)->getName());
		}
		indentStr = indentStr.substr(0, indentStr.length()-2);
		setTemporaryMark(0);
	}
}

std::string CScriptVar::getFlagsAsString() {
	std::string flagstr = "";
	if (isFunction()) flagstr = flagstr + "FUNCTION ";
	if (isObject()) flagstr = flagstr + "OBJECT ";
	if (isArray()) flagstr = flagstr + "ARRAY ";
	if (isNative()) flagstr = flagstr + "NATIVE ";
	if (isDouble()) flagstr = flagstr + "DOUBLE ";
	if (isInt()) flagstr = flagstr + "INTEGER ";
	if (isBool()) flagstr = flagstr + "BOOLEAN ";
	if (isString()) flagstr = flagstr + "STRING ";
	if (isRegExp()) flagstr = flagstr + "REGEXP ";
	if (isNaN()) flagstr = flagstr + "NaN ";
	if (isInfinity()) flagstr = flagstr + "INFINITY ";
	return flagstr;
}

CScriptVar *CScriptVar::ref() {
	refs++;
	return this;
}
void CScriptVar::unref() {
	refs--;
	if (refs<0)
		do
		{
		} while (0);
	ASSERT(refs>=0); // printf("OMFG, we have unreffed too far!\n");
	if (refs==0)
		delete this;
}

int CScriptVar::getRefs() const {
	return refs;
}

void CScriptVar::cleanUp4Destroy() {
	removeAllChildren();
	if (prototype) {
		prototype->unref();
		prototype = 0;
	}
}

void CScriptVar::setTemporaryMark_recursive(uint32_t ID)
{
	if(getTemporaryMark() != ID) {
		setTemporaryMark(ID);
		if (prototype) prototype->setTemporaryMark_recursive(ID);
		for(SCRIPTVAR_CHILDS_it it = Childs.begin(); it != Childs.end(); ++it) {
			(*it)->getVarPtr()->setTemporaryMark_recursive(ID);
		}
	}
}


//////////////////////////////////////////////////////////////////////////
/// CScriptVarLink
//////////////////////////////////////////////////////////////////////////

CScriptVarLink::CScriptVarLink(const CScriptVarPtr &Var, const std::string &Name /*=TINYJS_TEMP_NAME*/, int Flags /*=SCRIPTVARLINK_DEFAULT*/)
	: name(Name), owner(0), flags(Flags), refs(0) {
#if DEBUG_MEMORY
	mark_allocated(this);
#endif
	var = Var;
}

CScriptVarLink::~CScriptVarLink() {
#if DEBUG_MEMORY
	mark_deallocated(this);
#endif
}

CScriptVarLink *CScriptVarLink::ref() {
	refs++;
	return this;
}
void CScriptVarLink::unref() {
	refs--;
	ASSERT(refs>=0); // printf("OMFG, we have unreffed too far!\n");
	if (refs==0)
		delete this;
}


//////////////////////////////////////////////////////////////////////////
/// CScriptVarLinkPtr
//////////////////////////////////////////////////////////////////////////

CScriptVarLinkPtr & CScriptVarLinkPtr::operator()( const CScriptVarPtr &var, const std::string &name /*= TINYJS_TEMP_NAME*/, int flags /*= SCRIPTVARLINK_DEFAULT*/ ) {
	if(link && link->refs == 1) { // the link is only refered by this
		link->name = name;
		link->owner = 0;
		link->flags = flags;
		link->var = var;
	} else {
		if(link) link->unref();
		link = (new CScriptVarLink(var, name, flags))->ref();
	}
	return *this;
}

bool CScriptVarLinkPtr::operator <(const std::string &rhs) const {
	uint32_t lhs_int = isArrayIndex(link->getName());
	uint32_t rhs_int = isArrayIndex(rhs);
	if(lhs_int==uint32_t(-1)) {				// lhs is not an array-idx
		if(rhs_int==uint32_t(-1))			// and rhs is not an array-idx
			return link->getName() < rhs;	// normal std::string compare
		else
			return true;					// but rhs is an array-idx always true to sort array-idx at the end
	} else if(rhs_int==uint32_t(-1))		// if lhs an array-idx but rhs not always false to sort array-idx at the end
		return false;
	return lhs_int < rhs_int;				// both are array-idx normal int compare 
}



//////////////////////////////////////////////////////////////////////////
/// CScriptVarLinkWorkPtr
//////////////////////////////////////////////////////////////////////////

CScriptVarLinkWorkPtr CScriptVarLinkWorkPtr::getter() {
	if(link && link->getVarPtr()) {
		CScriptResult execute;
		CScriptVarPtr ret = getter(execute);
		execute.cThrow();
		return ret;
	}
	return *this;
}
CScriptVarLinkWorkPtr CScriptVarLinkWorkPtr::getter(CScriptResult &execute) {
	if(execute && link) {
		if (link->getVarPtr() && link->getVarPtr()->isAccessor()) {
			const CScriptVarPtr &var = link->getVarPtr();
			CScriptVarLinkPtr getter = var->findChild(TINYJS_ACCESSOR_GET_VAR);
			if (getter) {
				std::vector<CScriptVarPtr> Params;
				ASSERT(getReferencedOwner());
				return getter->getVarPtr()->getContext()->callFunction(execute, getter->getVarPtr(), Params, getReferencedOwner());
			} else
				return var->constScriptVar(Undefined);
		} else if (referencedOwner)
			return referencedOwner->getter(execute, *this);
	} 
	return *this;
}
CScriptVarLinkWorkPtr CScriptVarLinkWorkPtr::setter( const CScriptVarPtr &Var ) {
	if(link && link->getVarPtr()) {
		CScriptResult execute;
		CScriptVarPtr ret = setter(execute, Var);
		execute.cThrow();
		return ret;
	}
	return *this;
}

CScriptVarLinkWorkPtr CScriptVarLinkWorkPtr::setter( CScriptResult &execute, const CScriptVarPtr &Var ) {
	if(execute && link) {
		if(link->getVarPtr() && link->getVarPtr()->isAccessor()) {
			const CScriptVarPtr &var = link->getVarPtr();
			CScriptVarLinkPtr setter = var->findChild(TINYJS_ACCESSOR_SET_VAR);
			if(setter) {
				std::vector<CScriptVarPtr> Params;
				Params.push_back(Var);
				ASSERT(getReferencedOwner());
				setter->getVarPtr()->getContext()->callFunction(execute, setter->getVarPtr(), Params, getReferencedOwner());
			}
		} else if(referencedOwner)
			referencedOwner->setter(execute, *this, Var);
	}
	return *this;
}


//////////////////////////////////////////////////////////////////////////
/// CScriptVarPrimitive
//////////////////////////////////////////////////////////////////////////

CScriptVarPrimitive::~CScriptVarPrimitive(){}

bool CScriptVarPrimitive::isPrimitive() { return true; }
CScriptVarPrimitivePtr CScriptVarPrimitive::getRawPrimitive() { return this; }
bool CScriptVarPrimitive::toBoolean() { return false; }
CScriptVarPtr CScriptVarPrimitive::toObject() { return this; }
CScriptVarPtr CScriptVarPrimitive::toString_CallBack( CScriptResult &execute, int radix/*=0*/ ) {
	return newScriptVar(toCString(radix));
}


//////////////////////////////////////////////////////////////////////////
// CScriptVarUndefined
//////////////////////////////////////////////////////////////////////////

declare_dummy_t(Undefined);
CScriptVarUndefined::CScriptVarUndefined(CTinyJS *Context) : CScriptVarPrimitive(Context, Context->objectPrototype) { }
CScriptVarUndefined::~CScriptVarUndefined() {}
bool CScriptVarUndefined::isUndefined() { return true; }

CNumber CScriptVarUndefined::toNumber_Callback() { return NaN; }
std::string CScriptVarUndefined::toCString(int radix/*=0*/) { return "undefined"; }
std::string CScriptVarUndefined::getVarType() { return "undefined"; }


//////////////////////////////////////////////////////////////////////////
// CScriptVarNull
//////////////////////////////////////////////////////////////////////////

declare_dummy_t(Null);
CScriptVarNull::CScriptVarNull(CTinyJS *Context) : CScriptVarPrimitive(Context, Context->objectPrototype) { }
CScriptVarNull::~CScriptVarNull() {}
bool CScriptVarNull::isNull() { return true; }

CNumber CScriptVarNull::toNumber_Callback() { return 0; }
std::string CScriptVarNull::toCString(int radix/*=0*/) { return "null"; }
std::string CScriptVarNull::getVarType() { return "null"; }


//////////////////////////////////////////////////////////////////////////
/// CScriptVarString
//////////////////////////////////////////////////////////////////////////

CScriptVarString::CScriptVarString(CTinyJS *Context, const std::string &Data) : CScriptVarPrimitive(Context, Context->stringPrototype), data(Data) {
	addChild("length", newScriptVar(data.size()), SCRIPTVARLINK_CONSTANT);
/*
	CScriptVarLinkPtr acc = addChild("length", newScriptVar(Accessor), 0);
	CScriptVarFunctionPtr getter(::newScriptVar(Context, this, &CScriptVarString::native_Length, 0));
	getter->setFunctionData(new CScriptTokenDataFnc);
	acc->getVarPtr()->addChild(TINYJS_ACCESSOR_GET_VAR, getter, 0);
*/
}
CScriptVarString::~CScriptVarString() {}
bool CScriptVarString::isString() { return true; }

bool CScriptVarString::toBoolean() { return data.length()!=0; }
CNumber CScriptVarString::toNumber_Callback() { return CNumber(data.c_str()); }
std::string CScriptVarString::toCString(int radix/*=0*/) { return data; }

std::string CScriptVarString::getParsableString(const std::string &indentString, const std::string &indent, uint32_t uniqueID, bool &hasRecursion) { return indentString+getJSString(data); }
std::string CScriptVarString::getVarType() { return "std::string"; }

CScriptVarPtr CScriptVarString::toObject() {
	CScriptVarPtr ret = newScriptVar(CScriptVarPrimitivePtr(this), context->stringPrototype);
	ret->addChild("length", newScriptVar(data.size()), SCRIPTVARLINK_CONSTANT);
	return ret;
}

CScriptVarPtr CScriptVarString::toString_CallBack( CScriptResult &execute, int radix/*=0*/ ) {
	return this;
}

CScriptVarLinkWorkPtr CScriptVarString::getOwnProperty(const std::string &childName) {
	CScriptVarLinkWorkPtr child = CScriptVar::getOwnProperty(childName);
	if(child) return child;
	uint32_t Idx = isArrayIndex(childName);
	if (Idx!=uint32_t(-1)) {
		if((std::string::size_type)Idx < data.size())
			child(newScriptVar(data.substr(Idx, 1)), childName, SCRIPTVARLINK_ENUMERABLE);
		else
			child(constScriptVar(Undefined), childName, SCRIPTVARLINK_ENUMERABLE);
		child.setReferencedOwner(this); // fake referenced Owner
		return child;
	}
	return 0;
}

void CScriptVarString::keys(STRING_SET_t &Keys, bool OnlyEnumerable/*=true*/, uint32_t ID/*=0*/) {
	if(ID) setTemporaryMark(ID);
	std::string::size_type length = data.size();
	for(std::string::size_type i=0; i<length; ++i)
		Keys.insert(std::to_string(i));
	CScriptVar::keys(Keys, OnlyEnumerable, ID);
}

uint32_t CScriptVarString::getLength() { return (uint32_t)data.length(); }

int CScriptVarString::getChar(uint32_t Idx) {
	if((std::string::size_type)Idx >= data.size())
		return -1;
	else
		return (unsigned char)data[Idx];
}


//////////////////////////////////////////////////////////////////////////
/// CNumber
//////////////////////////////////////////////////////////////////////////

NegativeZero_t NegativeZero;
declare_dummy_t(NaN);
Infinity InfinityPositive(1);
Infinity InfinityNegative(-1);

CNumber::CNumber(int64_t Value) {
	if(std::numeric_limits<int32_t>::min()<=Value && Value<= std::numeric_limits<int32_t>::max())
		type=tInt32, Int32=int32_t(Value);
	else
		type=tDouble, Double=(double)Value;
}
CNumber::CNumber(uint64_t Value) {
	if(Value<=(uint32_t)std::numeric_limits<int32_t>::max())
		type=tInt32, Int32=int32_t(Value);
	else
		type=tDouble, Double=(double)Value;
}
CNumber::CNumber(double Value) {
	if(Value == 0.0 && signbit(Value))
		type=tnNULL, Int32=0;
	else if(isinf(Value))
		type=tInfinity, Int32=Value > 0 ? 1 : -1;
	else if(isnan(Value))
		type=tNaN, Int32=0;
	else {
		double truncated = trunc(Value);  // Ganzzahliger Anteil
		if (Value == truncated &&
			truncated >= std::numeric_limits<int32_t>::min() &&
			truncated <= std::numeric_limits<int32_t>::max()) {
			type = tInt32;
			Int32 = static_cast<int32_t>(truncated);
		} else {
			type = tDouble;
			Double = Value;
		}
	}
}

// **String-Zuweisung**: Parst die Eingabe und speichert sie
CNumber::CNumber(std::string_view str) : type(tNaN), Int32(0) {
	// 1. Entferne führende Leerzeichen
	size_t left = str.find_first_not_of(" \t\n\r\f\v");
	if (left == std::string_view::npos) { // Falls nur Leerzeichen enthalten sind
		return;
	}
	str.remove_prefix(left);

	std::string_view endptr;

	// 2. Prüfe auf Präfixe (0b, 0o, 0x) für Binär, Oktal und Hexadezimal
	if (str.size() > 1 && str.front() == '0') {
		switch (str[1] | 0x20) { // Konvertiere Großbuchstaben zu Kleinbuchstaben
		case 'b': parseInt(str.substr(2), 2, &endptr); break;
		case 'o': parseInt(str.substr(2), 8, &endptr); break;
		case 'x': parseInt(str.substr(2), 16, &endptr); break;
		default:  parseInt(str, 10, &endptr); break;
		}
	} else {
		// 3. Prüfe, ob die Eingabe ein Gleitkommaformat hat
		if (str.find_first_of(".eE") != std::string_view::npos) {
			parseFloat(str, &endptr);
		} else {
			parseInt(str, 10, &endptr);
		}
	}

	// 4. Falls nach der Zahl noch Zeichen stehen ? NaN setzen
	size_t right = endptr.find_last_not_of(" \t\n\r\f\v");
	if (right != std::string_view::npos) {
		type = tNaN;
		Int32 = 0;
	}
}

// **Parsen einer Ganzzahl mit Basis 2-36**
int32_t CNumber::parseInt(std::string_view str, int32_t radix, std::string_view* endptr) {
	size_t left = str.find_first_not_of(" \t\n\r\f\v");
	if (left == std::string_view::npos) {
		type = tNaN;
		Int32 = 0;
		if (endptr) *endptr = str;
		return 0;
	}
	str.remove_prefix(left);

	bool negative = false;
	if (str.front() == '+' || str.front() == '-') {
		negative = (str.front() == '-');
		str.remove_prefix(1);
	}

	// Falls keine Basis angegeben ist, automatisch bestimmen
	if (radix == 0) {
#ifdef __cpp_lib_starts_ends_with
		if (str.starts_with("0x") || str.starts_with("0X")) {
#else
		if (str.substr(0,2) == "0x" || str.substr(0,2) == "0X") {
#endif
			radix = 16;
			str.remove_prefix(2);
		} else {
			radix = 10;
		}
#ifdef __cpp_lib_starts_ends_with
	} else if (radix == 16 && (str.starts_with("0x") || str.starts_with("0X"))) {
#else
	} else if (radix == 16 && (str.substr(0, 2) == "0x" || str.substr(0, 2) == "0X")) {
#endif
		str.remove_prefix(2);
	}

	if (radix < 2 || radix > 36) { // Ungültige Basis ? NaN
		type = tNaN;
		Int32 = 0;
		if (endptr) *endptr = str;
		return 0;
	}

	int32_t maxSafe = std::numeric_limits<int32_t>::max() / radix;
	int32_t result_i = 0;
	double result_f = 0.0;
	bool useFloat = false;

	while (!str.empty()) {
		char c = str.front();
		int charValue = (c >= '0' && c <= '9') ? c - '0' : (c | 0x20) - 'a' + 10;
		if (charValue < 0 || charValue > radix) break;

		if (!useFloat) {
			if (result_i > maxSafe) {
				result_f = result_i;
				useFloat = true;
			} else {
				result_i = result_i * radix + charValue;
			}
		} else {
			result_f = result_f * radix + charValue;
		}

		str.remove_prefix(1);
	}
	if (useFloat) {
		type = tDouble;
		Double = negative ? -result_f : result_f;
	} else {
		type = tInt32;
		Int32 = negative ? -result_i : result_i;
	}
	return radix;
}

// **Parsen einer Gleitkommazahl**
void CNumber::parseFloat(std::string_view str, std::string_view* endptr/*=0*/) {
	// 1. Entferne führende Leerzeichen
	size_t left = str.find_first_not_of(" \t\n\r\f\v");
	if (left == std::string_view::npos) {
		type = tNaN;
		Int32 = 0;
		if (endptr) *endptr = str;
		return;
	}
	str.remove_prefix(left);

	bool negative = false;
	if (str.front() == '+' || str.front() == '-') {
		negative = (str.front() == '-');
		str.remove_prefix(1);
	}

	// 2. Prüfe auf "Infinity"
#ifdef __cpp_lib_starts_ends_with
	if (str.starts_with("Infinity")) {
#else
	if (str.substr(0, 8) == "Infinity") {
#endif
		* this = negative ? -std::numeric_limits<double>::infinity() : std::numeric_limits<double>::infinity();
		str.remove_prefix(8);
		if (endptr) *endptr = str;
		return;
	}

	// 3. Verwende `from_chars`, um den Float-Wert zu parsen
	double value = 0.0;
	auto [ptr, ec] = std::from_chars(str.data(), str.data() + str.size(), value);

	if (ec != std::errc{}) { // Fehler beim Parsen ? NaN
		type = tNaN;
		Int32 = 0;
		if (endptr) *endptr = str;
		return;
	}

	if (negative) value = -value;
	if (endptr) *endptr = str.substr(ptr - str.data());

	*this = value; // **Nutze den `operator=(double)` direkt**
}

CNumber CNumber::add(const CNumber &Value) const {
	if(type==tNaN || Value.type==tNaN)
		return CNumber(tNaN);
	else if(type==tInfinity || Value.type==tInfinity) {
		if(type!=tInfinity)
			return Value;
		else if(Value.type!=tInfinity || sign()==Value.sign())
			return *this;
		else
			return CNumber(tNaN);
	} else if(type==tnNULL)
		return Value;
	else if(Value.type==tnNULL)
		return *this;
	else if(type==tDouble || Value.type==tDouble)
		return CNumber(toDouble()+Value.toDouble());
	else {
		int32_t range_max = std::numeric_limits<int32_t>::max();
		int32_t range_min = std::numeric_limits<int32_t>::min();
		if(Int32>0) range_max-=Int32;
		else if(Int32<0) range_min-=Int32;
		if(range_min<=Value.Int32 && Value.Int32<=range_max)
			return CNumber(Int32+Value.Int32);
		else
			return CNumber(double(Int32)+double(Value.Int32));
	}
}

CNumber CNumber::operator-() const {
	switch(type) {
	case tInt32:
		if(Int32==0)
			return CNumber(NegativeZero);
		[[fallthrough]];
	case tnNULL:
		return CNumber(-Int32);
	case tDouble:
		return CNumber(-Double);
	case tInfinity:
		return CNumber(tInfinity, -Int32);
	default:
		return CNumber(tNaN);
	}
}

static inline int bits(uint32_t Value) {
	uint32_t b=0, mask=0xFFFF0000UL;
	for(int shift=16; shift>0 && Value!=0; shift>>=1, mask>>=shift) {
		if(Value & mask) {
			b += shift;
			Value>>=shift;
		}
	}
	return b;
}
static inline int bits(int32_t Value) {
	return bits(uint32_t(Value<0?-Value:Value));
}

CNumber CNumber::multi(const CNumber &Value) const {
	if(type==tNaN || Value.type==tNaN)
		return CNumber(tNaN);
	else if(type==tInfinity || Value.type==tInfinity) {
		if(isZero() || Value.isZero())
			return CNumber(tNaN);
		else
			return CNumber(tInfinity, sign()==Value.sign()?1:-1);
	} else if(isZero() || Value.isZero()) {
		if(sign()==Value.sign())
			return CNumber(0);
		else
			return CNumber(NegativeZero);
	} else if(type==tDouble || Value.type==tDouble)
		return CNumber(toDouble()*Value.toDouble());
	else {
		// Int32*Int32
		if(bits(Int32)+bits(Value.Int32) <= 29)
			return CNumber(Int32*Value.Int32);
		else
			return CNumber(double(Int32)*double(Value.Int32));
	}
}


CNumber CNumber::div( const CNumber &Value ) const {
	if(type==tNaN || Value.type==tNaN) return CNumber(tNaN);
	int Sign = sign()*Value.sign();
	if(type==tInfinity) {
		if(Value.type==tInfinity) return CNumber(tNaN);
		else return CNumber(tInfinity, Sign);
	}
	if(Value.type==tInfinity) {
		if(Sign<0) return CNumber(NegativeZero);
		else return CNumber(0);
	} else if(Value.isZero()) {
		if(isZero()) return CNumber(tNaN);
		else return CNumber(tInfinity, Sign);
	} else
		return CNumber(toDouble() / Value.toDouble());
}

CNumber CNumber::pow(const CNumber& Value) const {

	if (*this == 1 && !Value.isFinite()) return NaN;		//	> not IEEE 754 conform pow(1, -Infinity | Infinity | NaN) = 1 / in JS = NaN 
	return ::pow(toDouble(), Value.toDouble());
}

CNumber CNumber::modulo( const CNumber &Value ) const {
	if(type==tNaN || type==tInfinity || Value.type==tNaN || Value.isZero()) return CNumber(tNaN);
	if(Value.type==tInfinity) return CNumber(*this);
	if(isZero()) return CNumber(0);
	if(type==tDouble || Value.type==tDouble || (Int32== std::numeric_limits<int32_t>::min() && Value == -1) /* use double to prevent integer overflow */ ) {
		double n = toDouble(), d = Value.toDouble(), q;
#if __cplusplus >= 201103L || _MSVC_LANG >= 201103L
		std::ignore = modf(n/d, &q);
#else
		modf(n / d, &q);
#endif
		return CNumber(n - (d * q));
	} else
		return CNumber(Int32 % Value.Int32);
}

CNumber CNumber::round() const {
	if(type != tDouble) return CNumber(*this);
	if(Double < 0.0 && Double >= -0.5)
		return CNumber(NegativeZero);
	return CNumber(::floor(Double+0.5));
}

CNumber CNumber::floor() const {
	if(type != tDouble) return CNumber(*this);
	return CNumber(::floor(Double));
}

CNumber CNumber::ceil() const {
	if(type != tDouble) return CNumber(*this);
	return CNumber(::ceil(Double));
}

CNumber CNumber::abs() const {
	if(sign()<0) return -CNumber(*this);
	else return CNumber(*this);
}

CNumber CNumber::shift(const CNumber &Value, bool Right) const {
	int32_t lhs = toInt32();
	uint32_t rhs = Value.toUInt32() & 0x1F;
	return CNumber(Right ? lhs>>rhs : lhs<<rhs);
}

CNumber CNumber::ushift(const CNumber &Value, bool Right) const {
	uint32_t lhs = toUInt32();
	uint32_t rhs = Value.toUInt32() & 0x1F;
	return CNumber(Right ? lhs>>rhs : lhs<<rhs);
}

CNumber CNumber::binary(const CNumber &Value, char Mode) const {
	int32_t lhs = toInt32();
	int32_t rhs = Value.toInt32();

	switch(Mode) {
	case '&':	lhs&=rhs; break;
	case '|':	lhs|=rhs; break;
	case '^':	lhs^=rhs; break;
	}
	return CNumber(lhs);
}


CNumber CNumber::clamp(const CNumber &min, const CNumber &max) const {
	return min > *this ? min : (max < *this ? max : *this);
}

int CNumber::less( const CNumber &Value ) const {
	if(type==tNaN || Value.type==tNaN) return 0;
	else if(type==tInfinity) {
		if(Value.type==tInfinity) return Int32<Value.Int32 ? 1 : -1;
		return -Int32;
	} else if(Value.type==tInfinity)
		return Value.Int32;
	else if(isZero() && Value.isZero()) return -1;
	else if(type==tDouble || Value.type==tDouble) return toDouble() < Value.toDouble() ? 1 : -1;
	return toInt32() < Value.toInt32() ? 1 : -1;
}

bool CNumber::equal( const CNumber &Value ) const {
	if(type==tNaN || Value.type==tNaN) return false;
	else if(type==tInfinity) {
		if(Value.type==tInfinity) return Int32==Value.Int32;
		return false;
	} else if(Value.type==tInfinity)
		return false;
	else if(isZero() && Value.isZero()) return true;
	else if(type==tDouble || Value.type==tDouble) return toDouble() == Value.toDouble();
	return toInt32() == Value.toInt32();
}

bool CNumber::isZero() const
{
	switch(type) {
	case tInt32:
		return Int32==0;
	case tnNULL:
		return true;
	case tDouble:
		return Double==0.0;
	default:
		return false;
	}
}

bool CNumber::isInteger() const
{
	double integral;
	switch(type) {
	case tInt32:
	case tnNULL:
		return true;
	case tDouble:
		return modf(Double, &integral)==0.0;
	default:
		return false;
	}
}

int CNumber::sign() const {
	switch(type) {
	case tInt32:
	case tInfinity:
		return Int32<0?-1:1;
	case tnNULL:
		return -1;
	case tDouble:
		return Double<0.0?-1:1;
	default:
		return 1;
	}
}
char *tiny_ltoa(int32_t val, unsigned radix) {
	char *buf, *buf_end, *p, *firstdig, temp;
	unsigned digval;

	buf = (char*)malloc(64);
	if(!buf) return 0;
	buf_end = buf+64-1; // -1 for '\0'

	p = buf;
	if (val < 0) {
		*p++ = '-';
		val = -val;
	}

	do {
		digval = (unsigned) (val % radix);
		val /= radix;
		*p++ = (char) (digval + (digval > 9 ? ('a'-10) : '0'));
		if(p==buf_end) {
			char *new_buf = (char *)realloc(buf, buf_end-buf+16+1); // for '\0'
			if(!new_buf) { free(buf); return 0; }
			p = new_buf + (buf_end - buf);
			buf_end = p + 16;
			buf = new_buf;
		}
	} while (val > 0);

	// We now have the digit of the number in the buffer, but in reverse
	// order.  Thus we reverse them now.
	*p-- = '\0';
	firstdig = buf;
	if(*firstdig=='-') firstdig++;
	do	{
		temp = *p;
		*p = *firstdig;
		*firstdig = temp;
		p--;
		firstdig++;
	} while (firstdig < p);
	return buf;
}

static char *tiny_dtoa(double val, unsigned radix) {
	char *buf, *buf_end, *p, temp;
	unsigned digval;

	buf = (char*)malloc(64);
	if(!buf) return 0;
	buf_end = buf+64-2; // -1 for '.' , -1 for '\0'

	p = buf;
	if (val < 0.0) {
		*p++ = '-';
		val = -val;
	}

	double val_1 = floor(val);
	double val_2 = val - val_1;


	do {
		double tmp = val_1 / radix;
		val_1 = floor(tmp);
		digval = (unsigned)((tmp - val_1) * radix);

		*p++ = (char) (digval + (digval > 9 ? ('a'-10) : '0'));
		if(p==buf_end) {
			char *new_buf = (char *)realloc(buf, buf_end-buf+16+2); // +2 for '.' + '\0'
			if(!new_buf) { free(buf); return 0; }
			p = new_buf + (buf_end - buf);
			buf_end = p + 16;
			buf = new_buf;
		}
	} while (val_1 > 0.0);

	// We now have the digit of the number in the buffer, but in reverse
	// order.  Thus we reverse them now.
	char *p1 = buf;
	char *p2 = p-1;
	do	{
		temp = *p2;
		*p2-- = *p1;
		*p1++ = temp;
	} while (p1 < p2);

	if(val_2) {
		*p++ = '.';
		do {
			val_2 *= radix;
			digval = (unsigned)(val_2);
			val_2 -= digval;

			*p++ = (char) (digval + (digval > 9 ? ('a'-10) : '0'));
			if(p==buf_end) {
				char *new_buf = (char *)realloc(buf, buf_end-buf+16);
				if(!new_buf) { free(buf); return 0; }
				p = new_buf + (buf_end - buf);
				buf_end = p + 16;
				buf = new_buf;
			}
		} while (val_2 > 0.0);

	}
	*p = '\0';
	return buf;
}
std::string CNumber::toString( uint32_t Radix/*=10*/ ) const {
	char *str;
	if(2 > Radix || Radix > 36)
		Radix = 10; // todo error;
	switch(type) {
	case tInt32:
		if( (str = tiny_ltoa(Int32, Radix)) ) {
			std::string ret(str); free(str);
			return ret;
		}
		break;
	case tnNULL:
		return "0";
	case tDouble:
		if(Radix==10) {
			std::ostringstream str;
			str.unsetf(std::ios::floatfield);
#if (defined(_MSC_VER) && _MSC_VER >= 1600) || __cplusplus >= 201103L
			str.precision(std::numeric_limits<double>::max_digits10);
#else
			str.precision(numeric_limits<double>::digits10+2);
#endif
			str << Double;
			return str.str();
		} else if( (str = tiny_dtoa(Double, Radix)) ) {
			std::string ret(str); free(str);
			return ret;
		}
		break;
	case tInfinity:
		return Int32<0?"-Infinity":"Infinity";
	case tNaN:
		return "NaN";
	}
	return "";
}

double CNumber::toDouble() const
{
	switch(type) {
	case tnNULL:
		return -0.0;
	case tInt32:
		return double(Int32);
	case tDouble:
		return Double;
	case tNaN:
		return std::numeric_limits<double>::quiet_NaN();
	case tInfinity:
		return Int32<0 ? -std::numeric_limits<double>::infinity(): std::numeric_limits<double>::infinity();
	}
	return 0.0;
}

//////////////////////////////////////////////////////////////////////////
/// CScriptVarNumber
//////////////////////////////////////////////////////////////////////////

CScriptVarNumber::CScriptVarNumber(CTinyJS *Context, const CNumber &Data) : CScriptVarPrimitive(Context, Context->numberPrototype), data(Data) {}
CScriptVarNumber::~CScriptVarNumber() {}
bool CScriptVarNumber::isNumber() { return true; }
bool CScriptVarNumber::isInt() { return data.isInt32(); }
bool CScriptVarNumber::isDouble() { return data.isDouble(); }
bool CScriptVarNumber::isRealNumber() { return isInt() || isDouble(); }
bool CScriptVarNumber::isNaN() { return data.isNaN(); }
int CScriptVarNumber::isInfinity() { return data.isInfinity(); }

bool CScriptVarNumber::toBoolean() { return data.toBoolean(); }
CNumber CScriptVarNumber::toNumber_Callback() { return data; }
std::string CScriptVarNumber::toCString(int radix/*=0*/) { return data.toString(radix); }

std::string CScriptVarNumber::getVarType() { return "number"; }

CScriptVarPtr CScriptVarNumber::toObject() { return newScriptVar(CScriptVarPrimitivePtr(this), context->numberPrototype); }
define_newScriptVar_Fnc(Number, CTinyJS *Context, const CNumber &Obj) {
	if(!Obj.isInt32() && !Obj.isDouble()) {
		if(Obj.isNaN()) return Context->constScriptVar(NaN);
		if(Obj.isInfinity()) return Context->constScriptVar(Infinity(Obj.sign()));
		if(Obj.isNegativeZero()) return Context->constScriptVar(NegativeZero);
	}
	return new CScriptVarNumber(Context, Obj);
}


//////////////////////////////////////////////////////////////////////////
// CScriptVarBool
//////////////////////////////////////////////////////////////////////////

CScriptVarBool::CScriptVarBool(CTinyJS *Context, bool Data) : CScriptVarPrimitive(Context, Context->booleanPrototype), data(Data) {}
CScriptVarBool::~CScriptVarBool() {}
bool CScriptVarBool::isBool() { return true; }

bool CScriptVarBool::toBoolean() { return data; }
CNumber CScriptVarBool::toNumber_Callback() { return data?1:0; }
std::string CScriptVarBool::toCString(int radix/*=0*/) { return data ? "true" : "false"; }

std::string CScriptVarBool::getVarType() { return "boolean"; }

CScriptVarPtr CScriptVarBool::toObject() { return newScriptVar(CScriptVarPrimitivePtr(this), context->booleanPrototype); }

//////////////////////////////////////////////////////////////////////////
/// CScriptVarObject
//////////////////////////////////////////////////////////////////////////

declare_dummy_t(Object);
CScriptVarObject::CScriptVarObject(CTinyJS *Context) : CScriptVar(Context, Context->objectPrototype) { }
CScriptVarObject::~CScriptVarObject() {}

void CScriptVarObject::removeAllChildren()
{
	CScriptVar::removeAllChildren();
	value.clear();
}

CScriptVarPrimitivePtr CScriptVarObject::getRawPrimitive() { return value; }
bool CScriptVarObject::isObject() { return true; }

std::string CScriptVarObject::getParsableString(const std::string &indentString, const std::string &indent, uint32_t uniqueID, bool &hasRecursion) {
	getParsableStringRecursionsCheckBegin();
	std::string destination;
	const char *nl = indent.size() ? "\n" : " ";
	const char *comma = "";
	destination.append("{");
	if(Childs.size()) {
		std::string new_indentString = indentString + indent;
		for(SCRIPTVAR_CHILDS_it it = Childs.begin(); it != Childs.end(); ++it) {
			if((*it)->isEnumerable()) {
				destination.append(comma); comma=",";
				destination.append(nl).append(new_indentString);
				std::shared_ptr<CScriptTokenDataFnc> Fnc = (*it)->getVarPtr()->getFunctionData();
				if (Fnc) {
					if (Fnc->type == LEX_T_GET) destination.append("get ");
					else if (Fnc->type == LEX_T_SET) destination.append("set ");
					else if (Fnc->type == LEX_T_GENERATOR_MEMBER) destination.append("*");
				}
				if (!(*it)->getVarPtr()->isAccessor()) {
					destination.append(getIDString((*it)->getName()));
					if (!Fnc || (Fnc->type != LEX_T_GET && Fnc->type != LEX_T_SET && Fnc->type != LEX_T_GENERATOR_MEMBER)) destination.append(" : ");
				}
				destination.append((*it)->getVarPtr()->getParsableString(new_indentString, indent, uniqueID, hasRecursion));
			}
		}
		destination.append(nl).append(indentString);
	}
	destination.append("}");
	getParsableStringRecursionsCheckEnd();
	return destination;
}
std::string CScriptVarObject::getVarType() { return "object"; }
std::string CScriptVarObject::getVarTypeTagName() { return "Object"; }

CScriptVarPtr CScriptVarObject::toObject() { return this; }

CScriptVarPtr CScriptVarObject::valueOf_CallBack() {
	if(value)
		return value->valueOf_CallBack();
	return CScriptVar::valueOf_CallBack();
}
CScriptVarPtr CScriptVarObject::toString_CallBack(CScriptResult &execute, int radix) {
	if(value)
		return value->toString_CallBack(execute, radix);
	return newScriptVar("[object "+getVarTypeTagName()+"]");
};

void CScriptVarObject::setTemporaryMark_recursive( uint32_t ID) {
	CScriptVar::setTemporaryMark_recursive(ID);
	if(value) value->setTemporaryMark_recursive(ID);
}


//////////////////////////////////////////////////////////////////////////
/// CScriptVarObjectTyped (simple Object with Typename
//////////////////////////////////////////////////////////////////////////

declare_dummy_t(StopIteration);
CScriptVarObjectTypeTagged::~CScriptVarObjectTypeTagged() {}
std::string CScriptVarObjectTypeTagged::getVarTypeTagName() { return typeTagName; }


//////////////////////////////////////////////////////////////////////////
/// CScriptVarError
//////////////////////////////////////////////////////////////////////////

const char *ERROR_NAME[] = {"Error", "EvalError", "RangeError", "ReferenceError", "SyntaxError", "TypeError"};

CScriptVarError::CScriptVarError(CTinyJS *Context, ERROR_TYPES type, const char *message, const char *file, int line, int column) : CScriptVarObject(Context, Context->getErrorPrototype(type)) {
	if(message && *message) addChild("message", newScriptVar(message));
	if(file && *file) addChild("fileName", newScriptVar(file));
	if(line>=0) addChild("lineNumber", newScriptVar(line+1));
	if(column>=0) addChild("column", newScriptVar(column+1));
}

CScriptVarError::~CScriptVarError() {}
bool CScriptVarError::isError() { return true; }

CScriptVarPtr CScriptVarError::toString_CallBack(CScriptResult &execute, int radix) {
	CScriptVarLinkPtr link;
	std::string name = ERROR_NAME[Error];
	link = findChildWithPrototypeChain("name"); if(link) name = link->toString(execute);
	std::string message; link = findChildWithPrototypeChain("message"); if(link) message = link->toString(execute);
	std::string fileName; link = findChildWithPrototypeChain("fileName"); if(link) fileName = link->toString(execute);
	int lineNumber=-1; link = findChildWithPrototypeChain("lineNumber"); if(link) lineNumber = link->toNumber().toInt32();
	int column=-1; link = findChildWithPrototypeChain("column"); if(link) column = link->toNumber().toInt32();
	std::ostringstream msg;
	msg << name << ": " << message;
	if(lineNumber >= 0) msg << " at Line:" << lineNumber+1;
	if(column >=0) msg << " Column:" << column+1;
	if(fileName.length()) msg << " in " << fileName;
	return newScriptVar(msg.str());
}

CScriptException CScriptVarError::toCScriptException()
{
	CScriptVarLinkPtr link;
	std::string name = ERROR_NAME[Error];
	link = findChildWithPrototypeChain("name"); if(link) name = link->toString();
	int ErrorCode;
	for(ErrorCode=(sizeof(ERROR_NAME)/sizeof(ERROR_NAME[0]))-1; ErrorCode>0; ErrorCode--) {
		if(name == ERROR_NAME[ErrorCode]) break;
	}
	std::string message; link = findChildWithPrototypeChain("message"); if(link) message = link->toString();
	std::string fileName; link = findChildWithPrototypeChain("fileName"); if(link) fileName = link->toString();
	int lineNumber=-1; link = findChildWithPrototypeChain("lineNumber"); if(link) lineNumber = link->toNumber().toInt32()-1;
	int column=-1; link = findChildWithPrototypeChain("column"); if(link) column = link->toNumber().toInt32()-1;
	return CScriptException((enum ERROR_TYPES)ErrorCode, message, fileName, lineNumber, column);
}


//////////////////////////////////////////////////////////////////////////
// CScriptVarArray
//////////////////////////////////////////////////////////////////////////

declare_dummy_t(Array);
CScriptVarArray::CScriptVarArray(CTinyJS *Context) : CScriptVarObject(Context, Context->arrayPrototype), toStringRecursion(false) {
	CScriptVarLinkPtr acc = addChild("length", newScriptVar(0), SCRIPTVARLINK_WRITABLE);
/*
	CScriptVarLinkPtr acc = addChild("length", newScriptVar(Accessor), SCRIPTVARLINK_WRITABLE);
	CScriptVarFunctionPtr getter(::newScriptVar(Context, this, &CScriptVarArray::native_getLength, 0));
	CScriptVarFunctionPtr setter(::newScriptVar(Context, this, &CScriptVarArray::native_setLength, 0));
	getter->setFunctionData(new CScriptTokenDataFnc);
	setter->setFunctionData(new CScriptTokenDataFnc);
	acc->getVarPtr()->addChild(TINYJS_ACCESSOR_GET_VAR, getter, 0);
	acc->getVarPtr()->addChild(TINYJS_ACCESSOR_SET_VAR, setter, 0);
*/
}

CScriptVarArray::~CScriptVarArray() {}
bool CScriptVarArray::isArray() { return true; }
std::string CScriptVarArray::getParsableString(const std::string &indentString, const std::string &indent, uint32_t uniqueID, bool &hasRecursion) {
	getParsableStringRecursionsCheckBegin();
	std::string destination;
	const char *nl = indent.size() ? "\n" : " ";
	const char *comma = "";
	destination.append("[");
	uint32_t len = getLength();
	if(len) {
		std::string new_indentString = indentString + indent;
		for (uint32_t i=0;i<len;i++) {
			destination.append(comma); comma = ",";
			destination.append(nl).append(new_indentString).append(getArrayElement(i)->getParsableString(new_indentString, indent, uniqueID, hasRecursion));
		}
		destination.append(nl).append(indentString);
	}
	destination.append("]");
	getParsableStringRecursionsCheckEnd();
	return destination;
}
CScriptVarPtr CScriptVarArray::toString_CallBack( CScriptResult &execute, int radix/*=0*/ ) {
	std::ostringstream destination;
	if(toStringRecursion) {
		return newScriptVar("");
	}
	toStringRecursion = true;
	try {
		uint32_t len = getLength();
		for (uint32_t i=0;execute && i<len;i++) {
			CScriptVarLinkWorkPtr element = findChildWithPrototypeChain(std::to_string(i));
			if(element && !(element = element.getter(execute))->getVarPtr()->isUndefined() )
				destination << element->toString(execute);
			if (i<len-1) destination  << ",";
		}
	} catch(...) {
		toStringRecursion = false;
		throw; // rethrow
	}
	toStringRecursion = false;
	return newScriptVar(destination.str());

}

void CScriptVarArray::setter(CScriptResult &execute, const CScriptVarLinkPtr &link, const CScriptVarPtr &value) {
	const std::string &name = link->getName();
	uint32_t newlen;
	if(name == "length") {
		if(!execute) return;
		if((value->toNumber(execute).isUInt32())) {
			newlen = value->toNumber(execute).toUInt32();
			if(newlen < link->getVarPtr()->toNumber()) {
				SCRIPTVAR_CHILDS_it begin = std::lower_bound(Childs.begin(), Childs.end(), std::to_string(newlen));
				//SCRIPTVAR_CHILDS_it end = Childs.end();
#if (__cplusplus >= 201103L || _MSVC_LANG >= 201103L || _MSC_VER >= 1600) // Visual Studio? I do not know exactly! 2010 and above ???
				Childs.erase(begin, Childs.end());
#else
				while(begin != Childs.end()) {
					begin = Childs.erase(begin);
				}
#endif
//
// 				if(execute.useStrict()) {
// 					while(rit != rend)
// 						rit = Childs.erase(rit);
// 				}
// 				Childs.erase(rit, rend);
			}
			link->setVarPtr(newScriptVar(newlen));
		} else {
			context->throwError(execute, RangeError, "invalid array length");
			return;
		}
	} else if((newlen = isArrayIndex(name)+1)) {
		auto length = findChild("length");
		if(length && newlen > length->getVarPtr()->toNumber())
			length->setVarPtr(newScriptVar(newlen));
	}
	CScriptVar::setter(execute, link, value);
}
CScriptVarLinkPtr &CScriptVarArray::getter(CScriptResult &execute, CScriptVarLinkPtr &link) {
	const std::string &name = link->getName();
	if (name == "length") {
		uint32_t len = isArrayIndex(Childs.back()->getName()) + 1;
//		uint32_t val = link->getVarPtr()->toNumber().toUInt32();
		if (len > link->getVarPtr()->toNumber())
			link->setVarPtr(newScriptVar(len));
	}
	return CScriptVar::getter(execute, link);
}


uint32_t CScriptVarArray::getLength() {
	auto length = findChild("length");
	if (length) {
		uint32_t real_len = isArrayIndex(Childs.back()->getName()) + 1;
		uint32_t curr_len = length->getVarPtr()->toNumber().toUInt32();
		if (real_len > curr_len) {
			length->setVarPtr(newScriptVar(real_len));
			return real_len;
		}
		return curr_len;
	}
	return 0;
}

CScriptVarPtr CScriptVarArray::getArrayElement(uint32_t idx) {
	CScriptVarLinkPtr link = findChild(std::to_string(idx));
	if (link) return link.getter();
	else return constScriptVar(Undefined); // undefined
}

void CScriptVarArray::setArrayElement(uint32_t idx, const CScriptVarPtr &value) {
	if(idx < (uint32_t)-1) {
		addChildOrReplace(std::to_string(idx), value);
		auto length = findChild("length");
		if (length && length->getVarPtr()->toNumber().toUInt32() < idx) 
			length->setVarPtr(newScriptVar(idx + 1));
	}
}

/*
void CScriptVarArray::native_getLength(const CFunctionsScopePtr &c, void *data) {
	c->setReturnVar(newScriptVar(getLength()));
}
void CScriptVarArray::native_setLength(const CFunctionsScopePtr &c, void *data) {
	CNumber newLen = c->getArgument(0)->toNumber();
	if(newLen.isUInt32()) {
		uint32_t len = newLen.toUInt32();
		if(len < length) {
			SCRIPTVAR_CHILDS_it it = lower_bound(Childs.begin(), Childs.end(), to_string(len));
			Childs.erase(it, Childs.end());
		}
		length = len;
	} else
		c->throwError(RangeError, "invalid array length");
}
*/


//////////////////////////////////////////////////////////////////////////
/// CScriptVarRegExp
//////////////////////////////////////////////////////////////////////////

#ifndef NO_REGEXP

CScriptVarRegExp::CScriptVarRegExp(CTinyJS *Context, const std::string &Regexp, const std::string &Flags) : CScriptVarObject(Context, Context->regexpPrototype), regexp(Regexp), flags(Flags) {
	addChild("global", ::newScriptVarAccessor<CScriptVarRegExp>(Context, this, &CScriptVarRegExp::native_Global, 0, 0, 0), 0);
	addChild("ignoreCase", ::newScriptVarAccessor<CScriptVarRegExp>(Context, this, &CScriptVarRegExp::native_IgnoreCase, 0, 0, 0), 0);
	addChild("multiline", ::newScriptVarAccessor<CScriptVarRegExp>(Context, this, &CScriptVarRegExp::native_Multiline, 0, 0, 0), 0);
	addChild("sticky", ::newScriptVarAccessor<CScriptVarRegExp>(Context, this, &CScriptVarRegExp::native_Sticky, 0, 0, 0), 0);
	addChild("regexp", ::newScriptVarAccessor<CScriptVarRegExp>(Context, this, &CScriptVarRegExp::native_Source, 0, 0, 0), 0);
	addChild("lastIndex", newScriptVar(0));
}
CScriptVarRegExp::~CScriptVarRegExp() {}
bool CScriptVarRegExp::isRegExp() { return true; }
//int CScriptVarRegExp::getInt() {return strtol(regexp.c_str(),0,0); }
//bool CScriptVarRegExp::getBool() {return regexp.length()!=0;}
//double CScriptVarRegExp::getDouble() {return strtod(regexp.c_str(),0);}
//std::string CScriptVarRegExp::getString() { return "/"+regexp+"/"+flags; }
//std::string CScriptVarRegExp::getParsableString(const std::string &indentString, const std::string &indent, uint32_t uniqueID, bool &hasRecursion) { return getString(); }
CScriptVarPtr CScriptVarRegExp::toString_CallBack(CScriptResult &execute, int radix) {
	return newScriptVar("/"+regexp+"/"+flags);
}
void CScriptVarRegExp::native_Global(const CFunctionsScopePtr &c, void *data) {
	c->setReturnVar(constScriptVar(Global()));
}
void CScriptVarRegExp::native_IgnoreCase(const CFunctionsScopePtr &c, void *data) {
	c->setReturnVar(constScriptVar(IgnoreCase()));
}
void CScriptVarRegExp::native_Multiline(const CFunctionsScopePtr &c, void *data) {
	c->setReturnVar(constScriptVar(Multiline()));
}
void CScriptVarRegExp::native_Sticky(const CFunctionsScopePtr &c, void *data) {
	c->setReturnVar(constScriptVar(Sticky()));
}
void CScriptVarRegExp::native_Source(const CFunctionsScopePtr &c, void *data) {
	c->setReturnVar(newScriptVar(regexp));
}
unsigned int CScriptVarRegExp::LastIndex() {
	CScriptVarPtr lastIndex = findChild("lastIndex");
	if(lastIndex) return lastIndex->toNumber().toInt32();
	return 0;
}

CScriptVarPtr CScriptVarRegExp::exec( const std::string &Input, bool Test /*= false*/ )
{
	std::regex::flag_type flags = std::regex_constants::ECMAScript;
	if(IgnoreCase()) flags |= std::regex_constants::icase;
	bool global = Global(), sticky = Sticky();
	unsigned int lastIndex = LastIndex();
	size_t offset = 0;
	if(global || sticky) {
		if(lastIndex > Input.length()) goto failed;
		offset=lastIndex;
	}
	{
		std::regex_constants::match_flag_type mflag = sticky? std::regex_constants::match_continuous : std::regex_constants::match_default;
		if(offset) mflag |= std::regex_constants::match_prev_avail;
		std::smatch match;
		if(std::regex_search(Input.begin()+offset, Input.end(), match, std::regex(regexp, flags), mflag) ) {
			addChildOrReplace("lastIndex", newScriptVar(offset+match.position()+match.str().length()));
			if(Test) return constScriptVar(true);

			CScriptVarArrayPtr retVar = newScriptVar(Array);
			retVar->addChild("input", newScriptVar(Input));
			retVar->addChild("index", newScriptVar(match.position()));
			for(std::smatch::size_type idx=0; idx<match.size(); idx++)
				retVar->addChild(std::to_string(idx), newScriptVar(match[idx].str()));
			return retVar;
		}
	}
failed:
	if(global || sticky)
		addChildOrReplace("lastIndex", newScriptVar(0));
	if(Test) return constScriptVar(false);
	return constScriptVar(Null);
}

const char * CScriptVarRegExp::ErrorStr( int Error )
{
	switch(Error) {
	case std::regex_constants::error_badbrace: return "the expression contained an invalid count in a { } expression";
	case std::regex_constants::error_badrepeat: return "a repeat expression (one of '*', '?', '+', '{' in most contexts) was not preceded by an expression";
	case std::regex_constants::error_brace: return "the expression contained an unmatched '{' or '}'";
	case std::regex_constants::error_brack: return "the expression contained an unmatched '[' or ']'";
	case std::regex_constants::error_collate: return "the expression contained an invalid collating element name";
	case std::regex_constants::error_complexity: return "an attempted match failed because it was too complex";
	case std::regex_constants::error_ctype: return "the expression contained an invalid character class name";
	case std::regex_constants::error_escape: return "the expression contained an invalid escape sequence";
	case std::regex_constants::error_paren: return "the expression contained an unmatched '(' or ')'";
	case std::regex_constants::error_range: return "the expression contained an invalid character range specifier";
	case std::regex_constants::error_space: return "parsing a regular expression failed because there were not enough resources available";
	case std::regex_constants::error_stack: return "an attempted match failed because there was not enough memory available";
	case std::regex_constants::error_backref: return "the expression contained an invalid back reference";
	default: return "";
	}
}

#endif /* NO_REGEXP */


//////////////////////////////////////////////////////////////////////////
/// CScriptVarDefaultIterator
//////////////////////////////////////////////////////////////////////////

//declare_dummy_t(DefaultIterator);
CScriptVarDefaultIterator::CScriptVarDefaultIterator(CTinyJS *Context, const CScriptVarPtr &Object, IteratorMode Mode)
	: CScriptVarObject(Context, Context->iteratorPrototype), mode(Mode), object(Object) {
	object->keys(keys, true);
	pos = keys.begin();
	addChild("next", ::newScriptVar(context, this, &CScriptVarDefaultIterator::native_next, 0));
}
CScriptVarDefaultIterator::~CScriptVarDefaultIterator() {}
bool CScriptVarDefaultIterator::isIterator()		{return true;}
void CScriptVarDefaultIterator::native_next(const CFunctionsScopePtr &c, void *data) {
	if(pos==keys.end()) throw constScriptVar(StopIteration);
	pos++;
	if(mode==RETURN_ARRAY) {
		CScriptVarArrayPtr arr = newScriptVar(Array);
		arr->setArrayElement(0, newScriptVar(*pos));
		arr->setArrayElement(1, object->getOwnProperty(*pos));
		c->setReturnVar(arr);
	} else if(mode==RETURN_KEY)
		c->setReturnVar(newScriptVar(*pos));
	else
		c->setReturnVar(object->getOwnProperty(*pos));
}


#ifndef NO_GENERATORS
//////////////////////////////////////////////////////////////////////////
/// CScriptVarGenerator
//////////////////////////////////////////////////////////////////////////

//declare_dummy_t(Generator);
#if _MSC_VER == 1600
#pragma warning(push)
#pragma warning(disable: 4355) // possible loss of data
#endif
CScriptVarGenerator::CScriptVarGenerator(CTinyJS *Context, const CScriptVarPtr &FunctionRoot, const CScriptVarFunctionPtr &Function)
	: CScriptVarObject(Context, Context->generatorPrototype), functionRoot(FunctionRoot), function(Function),
	closed(false), yieldVarIsException(false), coroutine(this), 
	callersStackBase(nullptr), callersScopeSize(0) , callersTokenizer(nullptr), callersHaveTry(false) {
//		addChild("next", ::newScriptVar(context, this, &CScriptVarGenerator::native_send, 0, "Generator.next"));
	//	addChild("send", ::newScriptVar(context, this, &CScriptVarGenerator::native_send, (void*)1, "Generator.send"));
		//addChild("close", ::newScriptVar(context, this, &CScriptVarGenerator::native_throw, (void*)0, "Generator.close"));
		//addChild("throw", ::newScriptVar(context, this, &CScriptVarGenerator::native_throw, (void*)1, "Generator.throw"));
}
#if _MSC_VER == 1600
#pragma warning(pop)
#endif
CScriptVarGenerator::~CScriptVarGenerator() {
	if(coroutine.isStarted()) {
		if(coroutine.isRunning()) {
			coroutine.Stop(false);
			coroutine.next();
		}
		coroutine.Stop();
	}
}
bool CScriptVarGenerator::isIterator()		{return true;}
bool CScriptVarGenerator::isGenerator()	{return true;}

std::string CScriptVarGenerator::getVarType() { return "generator"; }
std::string CScriptVarGenerator::getVarTypeTagName() { return "Generator"; }

void CScriptVarGenerator::setTemporaryMark_recursive( uint32_t ID ) {
	CScriptVarObject::setTemporaryMark_recursive(ID);
	functionRoot->setTemporaryMark_recursive(ID);
	function->setTemporaryMark_recursive(ID);
	if(yieldVar) yieldVar->setTemporaryMark_recursive(ID);
	for(std::vector<CScriptVarScopePtr>::iterator it=generatorScopes.begin(); it != generatorScopes.end(); ++it)
		(*it)->setTemporaryMark_recursive(ID);
}
void CScriptVarGenerator::native_send(const CFunctionsScopePtr &c, void *data) {
	// data == 0 ==> next()
	// data != 0 ==> send(...)
	if(closed)
		throw constScriptVar(StopIteration);

	yieldVar = data ? c->getArgument(0) : constScriptVar(Undefined);
	yieldVarIsException = false;

	if(!coroutine.isStarted() && data && !yieldVar->isUndefined())
		c->throwError(TypeError, "attempt to send value to newborn generator");
	if(coroutine.next()) {
		c->setReturnVar(yieldVar);
		return;
	}
	closed = true;
	throw yieldVar;
}
void CScriptVarGenerator::native_throw(const CFunctionsScopePtr &c, void *data) {
	// data == 0 ==> close()
	// data != 0 ==> throw(...)
	if(closed || !coroutine.isStarted()) {
		closed = true;
		if(data)
			throw c->getArgument(0);
		else
			return;
	}
	yieldVar = data ? c->getArgument(0) : CScriptVarPtr();
	yieldVarIsException = true;
	closed = data==0;
	if(coroutine.next()) {
		c->setReturnVar(yieldVar);
		return;
	}
	closed = true;
	/*
	 * from http://developer.mozilla.org/en-US/docs/Web/JavaScript/Guide/Iterators_and_Generators
	 * Generators have a close() method that forces the generator to close itself. The effects of closing a generator are:
	 * - Any finally clauses active in the generator function are run.
	 * - If a finally clause throws any exception other than StopIteration, the exception is propagated to the caller of the close() method.
	 * - The generator terminates.
	 *
	 * but in Firefox is also "StopIteration" propagated to the caller
	 * define GENERATOR_CLOSE_LIKE_IN_FIREFOX to enable this behavior
	*/
//#define GENERATOR_CLOSE_LIKE_IN_FIREFOX
#ifdef GENERATOR_CLOSE_LIKE_IN_FIREFOX
	if(data || yieldVar)
		throw yieldVar;
#else
	if(data || (yieldVar && yieldVar != constScriptVar(StopIteration)))
		throw yieldVar;
#endif
}

int CScriptVarGenerator::Coroutine()
{
	context->generator_start(this);
	return 0;
}

CScriptVarPtr CScriptVarGenerator::yield( CScriptResult &execute, CScriptVar *YieldIn )
{
	yieldVar = YieldIn;
	coroutine.yield();
	if(yieldVarIsException) {
		execute.set(CScriptResult::Throw, yieldVar);
		return constScriptVar(Undefined);
	}
	return yieldVar;
}

#endif /*NO_GENERATORS*/

//////////////////////////////////////////////////////////////////////////
// CScriptVarFunction
//////////////////////////////////////////////////////////////////////////

CScriptVarFunction::CScriptVarFunction(CTinyJS *Context, const std::shared_ptr<CScriptTokenDataFnc> &Data) : CScriptVarObject(Context, Context->functionPrototype), data(0) {
	setFunctionData(Data);
}
bool CScriptVarFunction::isObject() { return true; }
bool CScriptVarFunction::isFunction() { return true; }
bool CScriptVarFunction::isPrimitive()	{ return false; }

//std::string CScriptVarFunction::getString() {return "[ Function ]";}
std::string CScriptVarFunction::getVarType() { return "function"; }
std::string CScriptVarFunction::getParsableString(const std::string &indentString, const std::string &indent, uint32_t uniqueID, bool &hasRecursion) {
	getParsableStringRecursionsCheckBegin();
	std::string destination;
	if (!data->isArrowFunction()) {
		if (data->type == LEX_T_GET) destination.append("get ");
		else if (data->type == LEX_T_SET) destination.append("set ");
		else if (data->type == LEX_T_GENERATOR_MEMBER) destination.append("*");
		else {
			destination.append("function");
			if (data->isGenerator()) destination.append("*");
			destination.append(" ");
		}
		destination.append(data->name);
	}
	// get list of arguments
	destination.append(data->getArgumentsString(data->isArrowFunction()));
	if(data->isArrowFunction())
		destination.append("=> ");

	if(isNative() || isBounded())
		destination.append("{ [native code] }");
	else {
		destination.append(CScriptToken::getParsableString(data->body, indentString, indent));
	}
	getParsableStringRecursionsCheckEnd();
	return destination;
}

CScriptVarPtr CScriptVarFunction::toString_CallBack(CScriptResult &execute, int radix){
//	bool hasRecursion;
	return newScriptVar(((CScriptVar*)this)->getParsableString());
}

void CScriptVarFunction::setTemporaryMark_recursive(uint32_t ID) {
	CScriptVarObject::setTemporaryMark_recursive(ID);
	if (constructor) constructor->setTemporaryMark_recursive(ID);
}

const std::shared_ptr<CScriptTokenDataFnc> CScriptVarFunction::getFunctionData() const { return data; }

void CScriptVarFunction::setFunctionData(const std::shared_ptr<CScriptTokenDataFnc> &Data) {
	if(data) {
		// prevent dead ScriptVars (recursion)
		CScriptVarLinkPtr prototype = findChild(TINYJS_PROTOTYPE_CLASS);
		CScriptVarLinkPtr constructor = prototype->getVarPtr()->findChild(TINYJS_CONSTRUCTOR_VAR);
		prototype->getVarPtr()->removeLink(constructor);

	}
	if(Data) {
		data = Data; 
		addChildOrReplace("length", newScriptVar((int)data->arguments.size()), SCRIPTVARLINK_READONLY);
		addChildOrReplace("name", newScriptVar(data->name), SCRIPTVARLINK_READONLY);
		CScriptVarLinkPtr prototype = addChildOrReplace(TINYJS_PROTOTYPE_CLASS, newScriptVar(Object));
		prototype->getVarPtr()->addChildOrReplace(TINYJS_CONSTRUCTOR_VAR, this, SCRIPTVARLINK_WRITABLE);
	}
}


//////////////////////////////////////////////////////////////////////////
/// CScriptVarFunctionBounded
//////////////////////////////////////////////////////////////////////////

CScriptVarFunctionBounded::CScriptVarFunctionBounded(CScriptVarFunctionPtr BoundedFunction, CScriptVarPtr BoundedThis, const std::vector<CScriptVarPtr> &BoundedArguments)
	: CScriptVarFunction(BoundedFunction->getContext(), CScriptTokenDataFnc::create(LEX_R_FUNCTION)) ,
	boundedFunction(BoundedFunction),
	boundedThis(BoundedThis),
	boundedArguments(BoundedArguments) {
		getFunctionData()->name = BoundedFunction->getFunctionData()->name;
}
CScriptVarFunctionBounded::~CScriptVarFunctionBounded(){}
bool CScriptVarFunctionBounded::isBounded() { return true; }
void CScriptVarFunctionBounded::setTemporaryMark_recursive(uint32_t ID) {
	CScriptVarFunction::setTemporaryMark_recursive(ID);
	boundedThis->setTemporaryMark_recursive(ID);
	for(std::vector<CScriptVarPtr>::iterator it=boundedArguments.begin(); it!=boundedArguments.end(); ++it)
		(*it)->setTemporaryMark_recursive(ID);
}

CScriptVarPtr CScriptVarFunctionBounded::callFunction( CScriptResult &execute, std::vector<CScriptVarPtr> &Arguments, const CScriptVarPtr &This, CScriptVarPtr *newThis/*=0*/ )
{
	std::vector<CScriptVarPtr> newArgs=boundedArguments;
	newArgs.insert(newArgs.end(), Arguments.begin(), Arguments.end());
	return context->callFunction(execute, boundedFunction, newArgs, newThis ? This : boundedThis, newThis);
}


//////////////////////////////////////////////////////////////////////////
/// CScriptVarFunctionNative
//////////////////////////////////////////////////////////////////////////

CScriptVarFunctionNative::CScriptVarFunctionNative(CTinyJS *Context, void *Userdata, const char *Name, const char *Args) : CScriptVarFunction(Context, 0), jsUserData(Userdata) {
	std::shared_ptr<CScriptTokenDataFnc> FncData = CScriptTokenDataFnc::create(LEX_R_FUNCTION);
	if (Name) FncData->name = Name;
	if (Args) {
		CScriptLex lex(Args);
		int end_tk = LEX_EOF;
		if (lex.tk == '(') {
			end_tk = ')';
			lex.match('(');
		}
		while (lex.tk != end_tk) {
			FncData->arguments.push_back(CScriptToken(LEX_T_DESTRUCTURING_VAR, lex.tkStr));
			lex.match(LEX_ID);
			if (lex.tk != end_tk) lex.match(',', end_tk);
		}
		lex.match(end_tk);
		if(end_tk != LEX_EOF) lex.match(LEX_EOF);
	}
	setFunctionData(FncData);
}
CScriptVarFunctionNative::~CScriptVarFunctionNative() {}
bool CScriptVarFunctionNative::isNative() { return true; }


//////////////////////////////////////////////////////////////////////////
/// CScriptVarFunctionNativeCallback
//////////////////////////////////////////////////////////////////////////

CScriptVarFunctionNativeCallback::~CScriptVarFunctionNativeCallback() {}
void CScriptVarFunctionNativeCallback::callFunction(const CFunctionsScopePtr &c) { jsCallback(c, jsUserData); }


//////////////////////////////////////////////////////////////////////////
/// CScriptVarAccessor
//////////////////////////////////////////////////////////////////////////

declare_dummy_t(Accessor);
CScriptVarAccessor::CScriptVarAccessor(CTinyJS *Context) : CScriptVarObject(Context, Context->objectPrototype) { }
CScriptVarAccessor::CScriptVarAccessor(CTinyJS *Context, JSCallback getterFnc, void *getterData, JSCallback setterFnc, void *setterData)
	: CScriptVarObject(Context)
{
	if(getterFnc)
		addChild(TINYJS_ACCESSOR_GET_VAR, ::newScriptVar(Context, getterFnc, getterData), 0);
	if(setterFnc)
		addChild(TINYJS_ACCESSOR_SET_VAR, ::newScriptVar(Context, setterFnc, setterData), 0);
}

CScriptVarAccessor::CScriptVarAccessor( CTinyJS *Context, const CScriptVarFunctionPtr &getter, const CScriptVarFunctionPtr &setter) : CScriptVarObject(Context, Context->objectPrototype) {
	if(getter)
		addChild(TINYJS_ACCESSOR_GET_VAR, getter, 0);
	if(setter)
		addChild(TINYJS_ACCESSOR_SET_VAR, setter, 0);
}

CScriptVarAccessor::~CScriptVarAccessor() {}
bool CScriptVarAccessor::isAccessor() { return true; }
bool CScriptVarAccessor::isPrimitive()	{ return false; }
std::string CScriptVarAccessor::getParsableString(const std::string &indentString, const std::string &indent, uint32_t uniqueID, bool &hasRecursion) {
	getParsableStringRecursionsCheckBegin();
	std::string destination;
	CScriptVarLinkPtr getter = findChild(TINYJS_ACCESSOR_GET_VAR);
	CScriptVarLinkPtr setter = findChild(TINYJS_ACCESSOR_SET_VAR);
	if (getter) destination.append(getter->getVarPtr()->getParsableString(indentString, indent, uniqueID, hasRecursion));
	if (getter && setter) destination.append(", ");
	if (setter) destination.append(setter->getVarPtr()->getParsableString(indentString, indent, uniqueID, hasRecursion));
	getParsableStringRecursionsCheckEnd();
	return destination;
}
std::string CScriptVarAccessor::getVarType() { return "accessor"; }


//////////////////////////////////////////////////////////////////////////
/// CScriptVarScope
//////////////////////////////////////////////////////////////////////////

declare_dummy_t(Scope);
CScriptVarScope::~CScriptVarScope() {}
bool CScriptVarScope::isObject() { return false; }
CScriptVarPtr CScriptVarScope::scopeVar() { return this; }	///< to create var like: var a = ...
CScriptVarPtr CScriptVarScope::scopeLet() { return this; }	///< to create var like: let a = ...
CScriptVarLinkWorkPtr CScriptVarScope::findInScopes(const std::string &childName) {
	return  CScriptVar::findChild(childName);
}
CScriptVarScopePtr CScriptVarScope::getParent() { return CScriptVarScopePtr(); } ///< no Parent


//////////////////////////////////////////////////////////////////////////
/// CScriptVarScopeFnc
//////////////////////////////////////////////////////////////////////////

declare_dummy_t(ScopeFnc);
CScriptVarScopeFnc::~CScriptVarScopeFnc() {}
CScriptVarLinkWorkPtr CScriptVarScopeFnc::findInScopes(const std::string &childName) {
	CScriptVarLinkWorkPtr ret = findChild(childName);
	if( !ret ) {
		if(closure) ret = CScriptVarScopePtr(closure)->findInScopes(childName);
		else ret = context->getRoot()->findChild(childName);
	}
	return ret;
}

void CScriptVarScopeFnc::setReturnVar(const CScriptVarPtr &var) {
	addChildOrReplace(TINYJS_RETURN_VAR, var);
}

CScriptVarPtr CScriptVarScopeFnc::getParameter(const std::string &name) {
	return getArgument(name);
}

CScriptVarPtr CScriptVarScopeFnc::getParameter(int Idx) {
	return getArgument(Idx);
}
CScriptVarPtr CScriptVarScopeFnc::getArgument(const std::string &name) {
	return findChildOrCreate(name);
}
CScriptVarPtr CScriptVarScopeFnc::getArgument(int Idx) {
	CScriptVarLinkPtr arguments = findChildOrCreate(TINYJS_ARGUMENTS_VAR);
	if(arguments) arguments = arguments->getVarPtr()->findChild(std::to_string(Idx));
	return arguments ? arguments->getVarPtr() : constScriptVar(Undefined);
}
int CScriptVarScopeFnc::getParameterLength() {
	return getArgumentsLength();
}
uint32_t CScriptVarScopeFnc::getArgumentsLength()
{
	CScriptVarLinkPtr arguments = findChild(TINYJS_ARGUMENTS_VAR);
	if(arguments) arguments = arguments->getVarPtr()->findChild("length");
	return arguments ? arguments.getter()->toNumber().toUInt32() : 0UL;
}

void CScriptVarScopeFnc::throwError( ERROR_TYPES ErrorType, const std::string &message ) {
	throw newScriptVarError(context, ErrorType, message.c_str());
}
void CScriptVarScopeFnc::throwError( ERROR_TYPES ErrorType, const char *message ) {
	throw newScriptVarError(context, ErrorType, message);
}


void CScriptVarScopeFnc::assign(CScriptVarLinkWorkPtr &lhs, CScriptVarPtr rhs, bool ignoreReadOnly/*=false*/, bool ignoreNotOwned/*=false*/, bool ignoreNotExtensible/*=false*/)
{
	if (!lhs->isOwned() && !lhs.hasReferencedOwner() && lhs->getName().empty()) {
		if(ignoreNotOwned) return;
		throwError(ReferenceError, "invalid assignment left-hand side (at runtime)");
	} else if(lhs->isWritable()) {
		if (!lhs->isOwned()) {
			CScriptVarPtr fakedOwner = lhs.getReferencedOwner();
			if(fakedOwner) {
				if(!fakedOwner->isExtensible()) {
					if(ignoreNotExtensible) return;
					throwError(TypeError, lhs->getName() +" is not extensible");
				}
				lhs = fakedOwner->addChildOrReplace(lhs->getName(), lhs);
			} else
				throwError(ReferenceError, "invalid assignment left-hand side (at runtime)");
		}
		lhs.setter(rhs);
	} else if(!ignoreReadOnly)
		throwError(TypeError, lhs->getName() +" is read-only");
}

void CScriptVarScopeFnc::setProperty(CScriptVarLinkWorkPtr &lhs, const CScriptVarPtr &rhs, bool ignoreReadOnly/*=false*/, bool ignoreNotExtensible/*=false*/, bool ignoreNotOwned/*=false*/)
{
	if (!lhs->isOwned() && !lhs.hasReferencedOwner() && lhs->getName().empty()) {
		if(ignoreNotOwned) return;
		throw newScriptVarError(context, ReferenceError, "invalid assignment left-hand side (at runtime)");
	} else if(lhs->isWritable()) {
		if (!lhs->isOwned()) {
			CScriptVarPtr fakedOwner = lhs.getReferencedOwner();
			if(fakedOwner) {
				if(!fakedOwner->isExtensible()) {
					if(ignoreNotExtensible) return;
					throw newScriptVarError(context, TypeError, (lhs->getName() +" is not extensible").c_str());
				}
				lhs = fakedOwner->addChildOrReplace(lhs->getName(), lhs);
			}
		}
		lhs.setter(rhs);
	} else if(!ignoreReadOnly)
		throw newScriptVarError(context, TypeError, (lhs->getName() +" is read-only").c_str());

}

//////////////////////////////////////////////////////////////////////////
/// CScriptVarScopeLet
//////////////////////////////////////////////////////////////////////////

declare_dummy_t(ScopeLet);
CScriptVarScopeLet::CScriptVarScopeLet(const CScriptVarScopePtr &Parent) // constructor for LetScope
	: CScriptVarScope(Parent->getContext()), parent(addChild(TINYJS_SCOPE_PARENT_VAR, Parent, 0))
	, letExpressionInitMode(false) {}

CScriptVarScopeLet::~CScriptVarScopeLet() {}
CScriptVarPtr CScriptVarScopeLet::scopeVar() {						// to create var like: var a = ...
	return getParent()->scopeVar();
}
CScriptVarScopePtr CScriptVarScopeLet::getParent() { return (CScriptVarPtr)parent; }
CScriptVarLinkWorkPtr CScriptVarScopeLet::findInScopes(const std::string &childName) {
	CScriptVarLinkWorkPtr ret;
	if(letExpressionInitMode) {
		return getParent()->findInScopes(childName);
	} else {
		ret = findChild(childName);
		if( !ret ) ret = getParent()->findInScopes(childName);
	}
	return ret;
}


//////////////////////////////////////////////////////////////////////////
/// CScriptVarScopeWith
//////////////////////////////////////////////////////////////////////////

declare_dummy_t(ScopeWith);
CScriptVarScopeWith::~CScriptVarScopeWith() {}
CScriptVarPtr CScriptVarScopeWith::scopeLet() { 							// to create var like: let a = ...
	return getParent()->scopeLet();
}
CScriptVarLinkWorkPtr CScriptVarScopeWith::findInScopes(const std::string &childName) {
	if(childName == "this") return with;
	CScriptVarLinkWorkPtr ret = with->getVarPtr()->findChild(childName);
	if( !ret ) {
		ret = with->getVarPtr()->findChildInPrototypeChain(childName);
		if(ret) {
			ret(ret->getVarPtr(), ret->getName()); // recreate ret
			ret.setReferencedOwner(with->getVarPtr()); // fake referenced Owner
		}
	}
	if( !ret ) ret = getParent()->findInScopes(childName);
	return ret;
}


//////////////////////////////////////////////////////////////////////////
/// CTinyJS
//////////////////////////////////////////////////////////////////////////

extern "C" void _registerFunctions(CTinyJS *tinyJS);
extern "C" void _registerStringFunctions(CTinyJS *tinyJS);
extern "C" void _registerMathFunctions(CTinyJS *tinyJS);
extern "C" void _registerDateFunctions(CTinyJS *tinyJS);

// replace any_function.prototype
static void replacePrototype(const CScriptVarPtr &var, const CScriptVarPtr &prototype) {
	CScriptVarLinkPtr link;
	link = var->findChild(TINYJS_PROTOTYPE_CLASS);
	// to prevent dead ScriptVars (recursion) remove "constructor" first than replace replace prototype
	CScriptVarLinkPtr constructor = link->getVarPtr()->findChild(TINYJS_CONSTRUCTOR_VAR);
	link->getVarPtr()->removeLink(constructor);
	prototype->addChild(TINYJS_CONSTRUCTOR_VAR, var, SCRIPTVARLINK_WRITABLE);
	var->addChildOrReplace(TINYJS_PROTOTYPE_CLASS, prototype, SCRIPTVARLINK_CONSTANT);
}


CTinyJS::CTinyJS() {
	CScriptVarPtr var, var2;
	CScriptVarLinkPtr link;
	t = 0;
	haveTry = false;
	first = 0;
	uniqueID = 0;
	currentMarkSlot = -1;
	stackBase = 0;


	//////////////////////////////////////////////////////////////////////////
	// Object-Prototype
	// must be created as first object because this prototype is the base of all objects
	objectPrototype = newScriptVar(Object, nullptr);

	// all objects have a prototype. Also the prototype of prototypes
//	objectPrototype->addChild(TINYJS___PROTO___VAR, objectPrototype, 0);

	//////////////////////////////////////////////////////////////////////////
	// Function-Prototype
	// must be created as second object because this is the base of all functions (also constructors)
	functionPrototype = newScriptVar(Object);

	//////////////////////////////////////////////////////////////////////////
	// Scopes
	root = ::newScriptVar(this, Scope);
	scopes.push_back(root);

	//////////////////////////////////////////////////////////////////////////
	// Add built-in classes
	//////////////////////////////////////////////////////////////////////////
	// Object
	var = addNative("function Object(value)", this, &CTinyJS::native_Object, 0, SCRIPTVARLINK_CONSTANT);
	replacePrototype(var, objectPrototype);

	//	objectPrototype->addChild("__proto__", newScriptVar(Accessor, newScriptVar(na))
	objectPrototype->addChild("__proto__", ::newScriptVarAccessor(this, ::newScriptVar<CTinyJS>(this, this, &CTinyJS::native_Object_prototype_getter__proto__, 0), ::newScriptVar<CTinyJS>(this, this, &CTinyJS::native_Object_prototype_setter__proto__, 0)));
	addNative("function Object.getPrototypeOf(obj)", this, &CTinyJS::native_Object_getPrototypeOf);
	addNative("function Object.setPrototypeOf(obj, proto)", this, &CTinyJS::native_Object_setPrototypeOf);
	addNative("function Object.preventExtensions(obj)", this, &CTinyJS::native_Object_setObjectSecure);
	addNative("function Object.isExtensible(obj)", this, &CTinyJS::native_Object_isSecureObject);
	addNative("function Object.seel(obj)", this, &CTinyJS::native_Object_setObjectSecure, (void*)1);
	addNative("function Object.isSealed(obj)", this, &CTinyJS::native_Object_isSecureObject, (void*)1);
	addNative("function Object.freeze(obj)", this, &CTinyJS::native_Object_setObjectSecure, (void*)2);
	addNative("function Object.isFrozen(obj)", this, &CTinyJS::native_Object_isSecureObject, (void*)2);
	addNative("function Object.keys(obj)", this, &CTinyJS::native_Object_keys);
	addNative("function Object.getOwnPropertyNames(obj)", this, &CTinyJS::native_Object_keys, (void*)1);
	addNative("function Object.getOwnPropertyDescriptor(obj,name)", this, &CTinyJS::native_Object_getOwnPropertyDescriptor);
	addNative("function Object.defineProperty(obj,name,attributes)", this, &CTinyJS::native_Object_defineProperty);
	addNative("function Object.defineProperties(obj,properties)", this, &CTinyJS::native_Object_defineProperties);
	addNative("function Object.create(obj,properties)", this, &CTinyJS::native_Object_defineProperties, (void*)1);

	addNative("function Object.prototype.hasOwnProperty(prop)", this, &CTinyJS::native_Object_prototype_hasOwnProperty);
	objectPrototype_valueOf = addNative("function Object.prototype.valueOf()", this, &CTinyJS::native_Object_prototype_valueOf);
	objectPrototype_toString = addNative("function Object.prototype.toString(radix)", this, &CTinyJS::native_Object_prototype_toString);
	pseudo_refered.push_back(&objectPrototype);
	pseudo_refered.push_back(&objectPrototype_valueOf);
	pseudo_refered.push_back(&objectPrototype_toString);

	//////////////////////////////////////////////////////////////////////////
	// Array
	var = addNative("function Array(arrayLength)", this, &CTinyJS::native_Array, 0, SCRIPTVARLINK_CONSTANT);
	arrayPrototype = link = var->findChild(TINYJS_PROTOTYPE_CLASS);
	link->setWritable(false);
	CScriptVarFunctionPtr(var)->setConstructor(::newScriptVar(this, this, &CTinyJS::native_Array, (void*)1, "Array", "(arrayLength)"));

	arrayPrototype->addChild("valueOf", objectPrototype_valueOf, SCRIPTVARLINK_BUILDINDEFAULT);
	arrayPrototype->addChild("toString", objectPrototype_toString, SCRIPTVARLINK_BUILDINDEFAULT);
	pseudo_refered.push_back(&arrayPrototype);

	//////////////////////////////////////////////////////////////////////////
	// String
	var = addNative("function String(thing)", this, &CTinyJS::native_String, 0, SCRIPTVARLINK_CONSTANT);
	stringPrototype = link= var->findChild(TINYJS_PROTOTYPE_CLASS);
	link->setWritable(false);
	CScriptVarFunctionPtr(var)->setConstructor(::newScriptVar(this, this, &CTinyJS::native_String, (void*)1, "String", "(thing)"));
	stringPrototype->addChild("valueOf", objectPrototype_valueOf, SCRIPTVARLINK_BUILDINDEFAULT);
	stringPrototype->addChild("toString", objectPrototype_toString, SCRIPTVARLINK_BUILDINDEFAULT);
	pseudo_refered.push_back(&stringPrototype);

	//////////////////////////////////////////////////////////////////////////
	// RegExp
#ifndef NO_REGEXP
	var = addNative("function RegExp(pattern, flags)", this, &CTinyJS::native_RegExp, 0, SCRIPTVARLINK_CONSTANT);
	regexpPrototype = link = var->findChild(TINYJS_PROTOTYPE_CLASS);
	link->setWritable(false);
	regexpPrototype->addChild("valueOf", objectPrototype_valueOf, SCRIPTVARLINK_BUILDINDEFAULT);
	regexpPrototype->addChild("toString", objectPrototype_toString, SCRIPTVARLINK_BUILDINDEFAULT);
	pseudo_refered.push_back(&regexpPrototype);
#endif /* NO_REGEXP */

	//////////////////////////////////////////////////////////////////////////
	// Number
	var = addNative("function Number(value)", this, &CTinyJS::native_Number, 0, SCRIPTVARLINK_CONSTANT);
	numberPrototype = link =var->findChild(TINYJS_PROTOTYPE_CLASS);
	link->setWritable(false);
	CScriptVarFunctionPtr(var)->setConstructor(::newScriptVar(this, this, &CTinyJS::native_Number, (void*)1, "Number", "(value)"));

	var->addChild("EPSILON", newScriptVarNumber(this, std::numeric_limits<double>::epsilon()), SCRIPTVARLINK_CONSTANT);
	var->addChild("MAX_VALUE", newScriptVarNumber(this, std::numeric_limits<double>::max()), SCRIPTVARLINK_CONSTANT);
	var->addChild("MIN_VALUE", newScriptVarNumber(this, std::numeric_limits<double>::min()), SCRIPTVARLINK_CONSTANT);
	var->addChild("MAX_SAFE_INTEGER", newScriptVarNumber(this, (1LL << std::numeric_limits<double>::digits)-1), SCRIPTVARLINK_CONSTANT);
	var->addChild("MIN_SAFE_INTEGER", newScriptVarNumber(this, 1-(1LL << std::numeric_limits<double>::digits)), SCRIPTVARLINK_CONSTANT);
	var->addChild("NaN", constNaN = newScriptVarNumber(this, NaN), SCRIPTVARLINK_CONSTANT);
	var->addChild("POSITIVE_INFINITY", constInfinityPositive = newScriptVarNumber(this, InfinityPositive), SCRIPTVARLINK_CONSTANT);
	var->addChild("NEGATIVE_INFINITY", constInfinityNegative = newScriptVarNumber(this, InfinityNegative), SCRIPTVARLINK_CONSTANT);

	numberPrototype->addChild("valueOf", objectPrototype_valueOf, SCRIPTVARLINK_BUILDINDEFAULT);
	numberPrototype->addChild("toString", objectPrototype_toString, SCRIPTVARLINK_BUILDINDEFAULT);

	pseudo_refered.push_back(&numberPrototype);
	pseudo_refered.push_back(&constNaN);
	pseudo_refered.push_back(&constInfinityPositive);
	pseudo_refered.push_back(&constInfinityNegative);

	//////////////////////////////////////////////////////////////////////////
	// Boolean
	var = addNative("function Boolean()", this, &CTinyJS::native_Boolean, 0, SCRIPTVARLINK_CONSTANT);
	booleanPrototype = link = var->findChild(TINYJS_PROTOTYPE_CLASS);
	link->setWritable(false);
	CScriptVarFunctionPtr(var)->setConstructor(::newScriptVar(this, this, &CTinyJS::native_Boolean, (void*)1, "Boolean", "(value)"));

	booleanPrototype->addChild("valueOf", objectPrototype_valueOf, SCRIPTVARLINK_BUILDINDEFAULT);
	booleanPrototype->addChild("toString", objectPrototype_toString, SCRIPTVARLINK_BUILDINDEFAULT);
	pseudo_refered.push_back(&booleanPrototype);


	//////////////////////////////////////////////////////////////////////////
	// Iterator
	var = addNative("function Iterator(obj,mode)", this, &CTinyJS::native_Iterator, 0, SCRIPTVARLINK_CONSTANT);
	iteratorPrototype = link = var->findChild(TINYJS_PROTOTYPE_CLASS);
	link->setWritable(false);
	pseudo_refered.push_back(&iteratorPrototype);

	//////////////////////////////////////////////////////////////////////////
	// Generator
//	var = addNative("function Iterator(obj,mode)", this, &CTinyJS::native_Iterator, 0, SCRIPTVARLINK_CONSTANT);
#ifndef NO_GENERATORS
	generatorPrototype = link = newScriptVar(Object);
	link->setWritable(false);
	generatorPrototype->addChild("next", ::newScriptVar(this, this, &CTinyJS::native_Generator_prototype_next, (void*)0, "Generator.next"), SCRIPTVARLINK_BUILDINDEFAULT);
	generatorPrototype->addChild("send", ::newScriptVar(this, this, &CTinyJS::native_Generator_prototype_next, (void*)1, "Generator.send"), SCRIPTVARLINK_BUILDINDEFAULT);
	generatorPrototype->addChild("close", ::newScriptVar(this, this, &CTinyJS::native_Generator_prototype_next, (void*)2, "Generator.close"), SCRIPTVARLINK_BUILDINDEFAULT);
	generatorPrototype->addChild("throw", ::newScriptVar(this, this, &CTinyJS::native_Generator_prototype_next, (void*)3, "Generator.throw"), SCRIPTVARLINK_BUILDINDEFAULT);
	pseudo_refered.push_back(&generatorPrototype);
#endif /*NO_GENERATORS*/

	//////////////////////////////////////////////////////////////////////////
	// Function
	var = addNative("function Function(body)", this, &CTinyJS::native_Function, 0, SCRIPTVARLINK_CONSTANT);
	replacePrototype(var, functionPrototype);

	addNative("function Function.prototype.call(objc)", this, &CTinyJS::native_Function_prototype_call);
	addNative("function Function.prototype.apply(objc, args)", this, &CTinyJS::native_Function_prototype_apply);
	addNative("function Function.prototype.bind(objc, args)", this, &CTinyJS::native_Function_prototype_bind);
	addNative("function Function.prototype.isGenerator()", this, &CTinyJS::native_Function_prototype_isGenerator);

	functionPrototype->addChild("valueOf", objectPrototype_valueOf, SCRIPTVARLINK_BUILDINDEFAULT);
	functionPrototype->addChild("toString", objectPrototype_toString, SCRIPTVARLINK_BUILDINDEFAULT);
	pseudo_refered.push_back(&functionPrototype);


	//////////////////////////////////////////////////////////////////////////
	// Error
	var = addNative("function Error(message, fileName, lineNumber, column)", this, &CTinyJS::native_Error, 0, SCRIPTVARLINK_CONSTANT);
	errorPrototypes[Error] = link = var->findChild(TINYJS_PROTOTYPE_CLASS); link->setWritable(false);
	errorPrototypes[Error]->addChild("message", newScriptVar(""));
	errorPrototypes[Error]->addChild("name", newScriptVar("Error"));
	errorPrototypes[Error]->addChild("fileName", newScriptVar(""));
	errorPrototypes[Error]->addChild("lineNumber", newScriptVar(-1));	// -1 means not viable
	errorPrototypes[Error]->addChild("column", newScriptVar(-1));			// -1 means not viable

	var = addNative("function EvalError(message, fileName, lineNumber, column)", this, &CTinyJS::native_EvalError, 0, SCRIPTVARLINK_CONSTANT);
	errorPrototypes[EvalError] = var->findChild(TINYJS_PROTOTYPE_CLASS); link->setWritable(false);
	CScriptVarFunctionPtr(var)->setPrototype(errorPrototypes[Error]);
	errorPrototypes[EvalError]->addChild("name", newScriptVar("EvalError"));

	var = addNative("function RangeError(message, fileName, lineNumber, column)", this, &CTinyJS::native_RangeError, 0, SCRIPTVARLINK_CONSTANT);
	errorPrototypes[RangeError] = var->findChild(TINYJS_PROTOTYPE_CLASS); link->setWritable(false);
	CScriptVarFunctionPtr(var)->setPrototype(errorPrototypes[Error]);
	errorPrototypes[RangeError]->addChild("name", newScriptVar("RangeError"));

	var = addNative("function ReferenceError(message, fileName, lineNumber, column)", this, &CTinyJS::native_ReferenceError, 0, SCRIPTVARLINK_CONSTANT);
	errorPrototypes[ReferenceError] = var->findChild(TINYJS_PROTOTYPE_CLASS); link->setWritable(false);
	CScriptVarFunctionPtr(var)->setPrototype(errorPrototypes[Error]);
	errorPrototypes[ReferenceError]->addChild("name", newScriptVar("ReferenceError"));

	var = addNative("function SyntaxError(message, fileName, lineNumber, column)", this, &CTinyJS::native_SyntaxError, 0, SCRIPTVARLINK_CONSTANT);
	errorPrototypes[SyntaxError] = var->findChild(TINYJS_PROTOTYPE_CLASS); link->setWritable(false);
	CScriptVarFunctionPtr(var)->setPrototype(errorPrototypes[Error]);
	errorPrototypes[SyntaxError]->addChild("name", newScriptVar("SyntaxError"));

	var = addNative("function TypeError(message, fileName, lineNumber, column)", this, &CTinyJS::native_TypeError, 0, SCRIPTVARLINK_CONSTANT);
	errorPrototypes[TypeError] = var->findChild(TINYJS_PROTOTYPE_CLASS); link->setWritable(false);
	CScriptVarFunctionPtr(var)->setPrototype(errorPrototypes[Error]);
	errorPrototypes[TypeError]->addChild("name", newScriptVar("TypeError"));






	//////////////////////////////////////////////////////////////////////////
	// add global built-in vars & constants
	root->addChild("undefined", constUndefined = newScriptVarUndefined(this), SCRIPTVARLINK_CONSTANT);
	pseudo_refered.push_back(&constUndefined);
	constNull	= newScriptVarNull(this);	pseudo_refered.push_back(&constNull);
	root->addChild("NaN", constNaN, SCRIPTVARLINK_CONSTANT);
	root->addChild("Infinity", constInfinityPositive, SCRIPTVARLINK_CONSTANT);
	root->addChild("StopIteration", constStopIteration=newScriptVar(Object, var=newScriptVar(Object), "StopIteration"), SCRIPTVARLINK_CONSTANT);
	constStopIteration->addChild(TINYJS_PROTOTYPE_CLASS, var, SCRIPTVARLINK_CONSTANT);	pseudo_refered.push_back(&constStopIteration);
	constNegativZero	= newScriptVarNumber(this, NegativeZero);	pseudo_refered.push_back(&constNegativZero);
	constFalse	= newScriptVarBool(this, false);	pseudo_refered.push_back(&constFalse);
	constTrue	= newScriptVarBool(this, true);	pseudo_refered.push_back(&constTrue);

	//////////////////////////////////////////////////////////////////////////
	// add global functions
	addNative("function eval(jsCode)", this, &CTinyJS::native_eval);
	native_require_read = 0;
	addNative("function require(jsFile)", this, &CTinyJS::native_require);
	addNative("function isNaN(objc)", this, &CTinyJS::native_isNAN);
	addNative("function isFinite(objc)", this, &CTinyJS::native_isFinite);
	addNative("function parseInt(std::string, radix)", this, &CTinyJS::native_parseInt);
	addNative("function parseFloat(std::string)", this, &CTinyJS::native_parseFloat);

	root->addChild("JSON", newScriptVar(Object), SCRIPTVARLINK_BUILDINDEFAULT);
	addNative("function JSON.parse(text, reviver)", this, &CTinyJS::native_JSON_parse);

	_registerFunctions(this);
	_registerStringFunctions(this);
	_registerMathFunctions(this);
	_registerDateFunctions(this);
}

CTinyJS::~CTinyJS() {
	ASSERT(!t);
//	objectPrototype->setPrototype(0);
	for (std::vector<CScriptVarPtr*>::iterator it = pseudo_refered.begin(); it != pseudo_refered.end(); ++it) {
		(**it)->cleanUp4Destroy();
		**it = CScriptVarPtr();
	}
	for(int i=Error; i<ERROR_COUNT; i++)
		errorPrototypes[i] = CScriptVarPtr();
	root->cleanUp4Destroy();
	scopes.clear();
	ClearUnreferedVars();
	root = CScriptVarPtr();
#ifdef _DEBUG
	for(CScriptVar *p = first; p; p=p->next)
		printf("%p (%s) %s ID=%u\n", p, typeid(*p).name(), p->getParsableString().c_str(), p->debugID);
#endif
#if DEBUG_MEMORY
	show_allocated();
#endif
}

//////////////////////////////////////////////////////////////////////////
/// throws an Error & Exception
//////////////////////////////////////////////////////////////////////////

void CTinyJS::throwError(CScriptResult &execute, ERROR_TYPES ErrorType, const std::string &message ) {
	if(execute && haveTry) {
		execute.set(CScriptResult::Throw, newScriptVarError(this, ErrorType, message.c_str(), t->currentFile.c_str(), t->currentLine(), t->currentColumn()));
		return;
	}
	throw CScriptException(ErrorType, message, t->currentFile, t->currentLine(), t->currentColumn());
}
void CTinyJS::throwException(ERROR_TYPES ErrorType, const std::string &message ) {
	throw CScriptException(ErrorType, message, t->currentFile, t->currentLine(), t->currentColumn());
}

void CTinyJS::throwError(CScriptResult &execute, ERROR_TYPES ErrorType, const std::string &message, CScriptTokenizer::ScriptTokenPosition &Pos ){
	if(execute && haveTry) {
		execute.set(CScriptResult::Throw, newScriptVarError(this, ErrorType, message.c_str(), t->currentFile.c_str(), Pos.currentLine(), Pos.currentColumn()));
		return;
	}
	throw CScriptException(ErrorType, message, t->currentFile, Pos.currentLine(), Pos.currentColumn());
}
void CTinyJS::throwException(ERROR_TYPES ErrorType, const std::string &message, CScriptTokenizer::ScriptTokenPosition &Pos ){
	throw CScriptException(ErrorType, message, t->currentFile, Pos.currentLine(), Pos.currentColumn());
}

//////////////////////////////////////////////////////////////////////////

void CTinyJS::trace() {
	root->trace();
}

void CTinyJS::execute(CScriptTokenizer &Tokenizer) {
	evaluateComplex(Tokenizer);
}

void CTinyJS::execute(const char *Code, const std::string &File, int Line, int Column) {
	evaluateComplex(Code, File, Line, Column);
}

void CTinyJS::execute(const std::string &Code, const std::string &File, int Line, int Column) {
	evaluateComplex(Code, File, Line, Column);
}

CScriptVarLinkPtr CTinyJS::evaluateComplex(CScriptTokenizer &Tokenizer) {
	t = &Tokenizer;
	CScriptResult execute;
	try {
		do {
			execute_statement(execute);
			while (t->tk==';') t->match(';'); // skip empty statements
		} while (t->tk!=LEX_EOF);
	} catch (...) {
		haveTry = false;
		t=0; // clean up Tokenizer
		throw; //
	}
	t=0;
	ClearUnreferedVars(execute.value);

	uint32_t UniqueID = allocUniqueID();
	setTemporaryID_recursive(UniqueID);
	if(execute.value) execute.value->setTemporaryMark_recursive(UniqueID);
	for(CScriptVar *p = first; p; p=p->next)
	{
		if(p->getTemporaryMark() != UniqueID)
			printf("%s %p\n", p->getVarType().c_str(), p);
	}
	freeUniqueID();

	if (execute.value)
		return CScriptVarLinkPtr(execute.value);
	// return undefined...
	return CScriptVarLinkPtr(constScriptVar(Undefined));
}
CScriptVarLinkPtr CTinyJS::evaluateComplex(const char *Code, const std::string &File, int Line, int Column) {
	CScriptTokenizer Tokenizer(Code, File, Line, Column);
	return evaluateComplex(Tokenizer);
}
CScriptVarLinkPtr CTinyJS::evaluateComplex(const std::string &Code, const std::string &File, int Line, int Column) {
	CScriptTokenizer Tokenizer(Code.c_str(), File, Line, Column);
	return evaluateComplex(Tokenizer);
}

std::string CTinyJS::evaluate(CScriptTokenizer &Tokenizer) {
	return evaluateComplex(Tokenizer)->toString();
}
std::string CTinyJS::evaluate(const char *Code, const std::string &File, int Line, int Column) {
	return evaluateComplex(Code, File, Line, Column)->toString();
}
std::string CTinyJS::evaluate(const std::string &Code, const std::string &File, int Line, int Column) {
	return evaluate(Code.c_str(), File, Line, Column);
}


CScriptVarPtr CTinyJS::addNative_ParseFuncDesc(const std::string &funcDesc, std::string &name, std::string &args) {
	CScriptLex lex(funcDesc.c_str());
	CScriptVarPtr base = root;
	lex.match(LEX_R_FUNCTION);
	name = lex.tkStr;
	lex.match(LEX_ID);
	/* Check for dots, we might want to do something like function String.substring ... */
	while (lex.tk == '.') {
		lex.match('.');
		CScriptVarLinkPtr link = base->findChild(name);
		// if it doesn't exist, make an object class
		if (!link) link = base->addChild(name, base->newScriptVar(Object));
		base = link->getVarPtr();
		name = lex.tkStr;
		lex.match(LEX_ID);
	}
	args = lex.rest();
	lex.match('(');
	return base;
}

CScriptVarFunctionNativePtr CTinyJS::addNative(const std::string &funcDesc, JSCallback ptr, void *userdata, int LinkFlags) {
	std::string name, args;
	CScriptVarPtr ret, base = addNative_ParseFuncDesc(funcDesc, name, args);
	base->addChild(name, ret = ::newScriptVar(this, ptr, userdata, name.c_str(), args.c_str()), LinkFlags);;
	return ret;
}

CScriptVarLinkWorkPtr CTinyJS::parseFunctionDefinition(const CScriptToken &FncToken) {
	auto &Fnc = FncToken.Fnc();
//	std::string fncName = (FncToken.token == LEX_T_FUNCTION_OPERATOR) ? TINYJS_TEMP_NAME : Fnc.name;
	CScriptVarLinkWorkPtr funcVar(newScriptVar(Fnc), Fnc->name);
	if(scope() != root)
		funcVar->getVarPtr()->addChild(TINYJS_FUNCTION_CLOSURE_VAR, scope(), 0);
	return funcVar;
}

CScriptVarLinkWorkPtr CTinyJS::parseFunctionsBodyFromString(const std::string &ArgumentList, const std::string &FncBody) {
	std::string Fnc = "function ("+ArgumentList+"){"+FncBody+"}";
	CScriptTokenizer tokenizer(Fnc.c_str());
	return parseFunctionDefinition(tokenizer.getToken());
}
CScriptVarPtr CTinyJS::callFunction(const CScriptVarFunctionPtr &Function, std::vector<CScriptVarPtr> &Arguments, const CScriptVarPtr &This, CScriptVarPtr *newThis) {
	CScriptResult execute;
	CScriptVarPtr retVar = callFunction(execute, Function, Arguments, This, newThis);
	execute.cThrow();
	return retVar;
}

CScriptVarPtr CTinyJS::callFunction(CScriptResult &execute, const CScriptVarFunctionPtr &Function, std::vector<CScriptVarPtr> &Arguments, const CScriptVarPtr &This, CScriptVarPtr *newThis) {
	ASSERT(Function && Function->isFunction());

	if(Function->isBounded()) return CScriptVarFunctionBoundedPtr(Function)->callFunction(execute, Arguments, This, newThis);

	auto Fnc = Function->getFunctionData();
	CScriptVarScopeFncPtr functionRoot(::newScriptVar(this, ScopeFnc, CScriptVarPtr(Function->findChild(TINYJS_FUNCTION_CLOSURE_VAR))));
	if(Fnc->name.size()) functionRoot->addChild(Fnc->name, Function);
	if(!Fnc->isArrowFunction()) {
		// arrow functions get this from closure
		if(This)
			functionRoot->addChild("this", This);
		else
			functionRoot->addChild("this", root); // if no this given use root
	}
	CScriptVarPtr arguments = functionRoot->addChild(TINYJS_ARGUMENTS_VAR, newScriptVar(Object));

	CScopeControl ScopeControl(this);
	CScriptVarPtr tmpArgsScope = ScopeControl.addLetScope();

	CScriptResult function_execute;
	size_t length_proto = Fnc->arguments.size();
	size_t length_arguments = Arguments.size();
	size_t length = std::max(length_proto, length_arguments);

	STRING_VECTOR_t arg_names;
	for(size_t arguments_idx = 0; arguments_idx<length_proto; ++arguments_idx) {
		ASSERT(Fnc->arguments[arguments_idx].token == LEX_T_DESTRUCTURING_VAR);
		Fnc->arguments[arguments_idx].DestructuringVar()->getVarNames(arg_names);
	}
	for(STRING_VECTOR_it it = arg_names.begin(); it != arg_names.end(); ++it)
		tmpArgsScope->addChildOrReplace(*it, constUndefined);

	for(size_t arguments_idx = 0; execute && arguments_idx<length; ++arguments_idx) {
		std::string arguments_idx_str = std::to_string(arguments_idx);
		CScriptVarLinkWorkPtr value;
		if(arguments_idx < length_arguments)
			value = arguments->addChild(arguments_idx_str, Arguments[arguments_idx]);
		else
			value = constUndefined;

		if(arguments_idx < length_proto) {
			auto &DestructuringVar = Fnc->arguments[arguments_idx].DestructuringVar();
			if(value->getVarPtr()->isUndefined()) {
				if(DestructuringVar->assignment.size()) {
					t->pushTokenScope(DestructuringVar->assignment);
					assign_destructuring_var(execute, DestructuringVar, execute_assignment(execute), tmpArgsScope);
				}
			} else {
				assign_destructuring_var(execute, DestructuringVar, value, tmpArgsScope);
			}
		}
	}
	if(!execute) return constUndefined;
	// copy args from tmpArgsScope to functionRoot
	for(STRING_VECTOR_it it = arg_names.begin(); it != arg_names.end(); ++it) {
		functionRoot->addChildOrReplace(*it, tmpArgsScope->findChild(*it));
	}
	arguments->addChild("length", newScriptVar(length_arguments));

#ifndef NO_GENERATORS
	if(Fnc->isGenerator()) {
		return ::newScriptVarCScriptVarGenerator(this, functionRoot, Function);
	}
#endif /*NO_GENERATORS*/
	// execute function!
	ScopeControl.clear(); 	// remove tmpArgsScope from scope-chain
	// add the function's execute space to the symbol table so we can recurse
	ScopeControl.addFncScope(functionRoot);
	if (Function->isNative()) {
		try {
			CScriptVarFunctionNativePtr(Function)->callFunction(functionRoot);
			CScriptVarLinkPtr ret = functionRoot->findChild(TINYJS_RETURN_VAR);
			function_execute.set(CScriptResult::Return, ret ? CScriptVarPtr(ret) : constUndefined);
		} catch (CScriptVarPtr v) {
			if(haveTry) {
				function_execute.setThrow(v, "native function '"+Fnc->name+"'");
			} else if(v->isError()) {
				CScriptException err = CScriptVarErrorPtr(v)->toCScriptException();
				if(err.fileName.empty()) err.fileName = "native function '"+Fnc->name+"'";
				throw err;
			}
			else
				throw CScriptException(Error, "uncaught exception: '"+v->toString(function_execute)+"' in native function '"+Fnc->name+"'");
		}
	} else {
		/* we just want to execute the block, but something could
			* have messed up and left us with the wrong ScriptLex, so
			* we want to be careful here... */
		std::string oldFile = t->currentFile;
		t->currentFile = Fnc->file;
		t->pushTokenScope(Fnc->body);
		if(Fnc->body.front().token == '{')
			execute_block(function_execute);
		else {
			CScriptVarPtr ret = execute_base(function_execute);
			if(function_execute) function_execute.set(CScriptResult::Return, ret);
		}
		t->currentFile = oldFile;

		// because return will probably have called this, and set execute to false
	}
	if(function_execute.isReturnNormal()) {
		if(newThis) *newThis = functionRoot->findChild("this");
		if(function_execute.isReturn()) {
			CScriptVarPtr ret = function_execute.value;
			return ret;
		}
	} else
		execute = function_execute;
	return constScriptVar(Undefined);
}
#ifndef NO_GENERATORS
void CTinyJS::generator_start(CScriptVarGenerator *Generator)
{
	// push current Generator
	generatorStack.push_back(Generator);

	// safe callers stackBase & set generators one
	Generator->callersStackBase = stackBase;
	stackBase = 0;

	// safe callers ScopeSize
	Generator->callersScopeSize = scopes.size();

	// safe callers Tokenizer & set generators one
	Generator->callersTokenizer = t;
	CScriptTokenizer generatorTokenizer;
	t = &generatorTokenizer;

	// safe callers haveTry
	Generator->callersHaveTry = haveTry;
	haveTry = true;

	// push generator's FunctionRoot
	CScopeControl ScopeControl(this);
	ScopeControl.addFncScope(Generator->getFunctionRoot());

	// call generator-function
	auto Fnc = Generator->getFunction()->getFunctionData();
	CScriptResult function_execute;
	TOKEN_VECT eof(1, CScriptToken());
	t->pushTokenScope(eof);
	t->pushTokenScope(Fnc->body);
	t->currentFile = Fnc->file;
	try {
		if(Fnc->body.front().token == '{')
			execute_block(function_execute);
		else {
			execute_base(function_execute);
		}
		if(function_execute.isThrow())
			Generator->setException(function_execute.value);
		else
			Generator->setException(constStopIteration);
	} catch(CScriptCoroutine::StopIteration_t &) {
		Generator->setException(CScriptVarPtr());
	} catch(...) {
		// pop current Generator
		generatorStack.pop_back();

		// restore callers stackBase
		stackBase = Generator->callersStackBase;

		// restore callers Scope (restored by ScopeControl
		//		scopes.erase(scopes.begin()+Generator->callersScopeSize, scopes.end());

		// restore callers Tokenizer
		t = Generator->callersTokenizer;

		// restore callers haveTry
		haveTry = Generator->callersHaveTry;

		Generator->setException(constStopIteration);
		// re-throw
		throw;
	}
	// pop current Generator
	generatorStack.pop_back();

	// restore callers stackBase
	stackBase = Generator->callersStackBase;

	// restore callers Scope (restored by ScopeControl
	//		scopes.erase(scopes.begin()+Generator->callersScopeSize, scopes.end());

	// restore callers Tokenizer
	t = Generator->callersTokenizer;

	// restore callers haveTry
	haveTry = Generator->callersHaveTry;
}
CScriptVarPtr CTinyJS::generator_yield(CScriptResult &execute, CScriptVar *YieldIn)
{
	if(!execute) return constUndefined;
	CScriptVarGenerator *Generator = generatorStack.back();
	if(Generator->isClosed()) {
		throwError(execute, TypeError, "yield from closing generator function");
		return constUndefined;
	}

	// pop current Generator
	generatorStack.pop_back();

	// safe generators and restore callers stackBase
	void *generatorStckBase = stackBase;
	stackBase = Generator->callersStackBase;

	// safe generators and restore callers scopes
	Generator->generatorScopes.assign(scopes.begin()+Generator->callersScopeSize, scopes.end());
	scopes.erase(scopes.begin()+Generator->callersScopeSize, scopes.end());

	// safe generators and restore callers Tokenizer
	CScriptTokenizer *generatorTokenizer = t;
	t = Generator->callersTokenizer;

	// safe generators and restore callers haveTry
	bool generatorsHaveTry = haveTry;
	haveTry = Generator->callersHaveTry;

	CScriptVarPtr ret;
	try {
		ret = Generator->yield(execute, YieldIn);
	} catch(...) {
		// normaly catch(CScriptCoroutine::CScriptCoroutineFinish_t &)
		// force StopIteration with call CScriptCoroutine::Stop() before CScriptCoroutine::next()
		// but catch(...) is for paranoia

		// push current Generator
		generatorStack.push_back(Generator);

		// safe callers and restore generators stackBase
		Generator->callersStackBase = stackBase;
		stackBase = generatorStckBase;

		// safe callers and restore generator Scopes
		Generator->callersScopeSize = scopes.size();
		scopes.insert(scopes.end(), Generator->generatorScopes.begin(), Generator->generatorScopes.end());
		Generator->generatorScopes.clear();

		// safe callers and restore generator Tokenizer
		Generator->callersTokenizer = t;
		t = generatorTokenizer;

		// safe callers and restore generator haveTry
		Generator->callersHaveTry = haveTry;
		haveTry = generatorsHaveTry;

		// re-throw
		throw;
	}
	// push current Generator
	generatorStack.push_back(Generator);

	// safe callers and restore generators stackBase
	Generator->callersStackBase = stackBase;
	stackBase = generatorStckBase;

	// safe callers and restore generator Scopes
	Generator->callersScopeSize = scopes.size();
	scopes.insert(scopes.end(), Generator->generatorScopes.begin(), Generator->generatorScopes.end());
	Generator->generatorScopes.clear();

	// safe callers and restore generator Tokenizer
	Generator->callersTokenizer = t;
	t = generatorTokenizer;

	// safe callers and restore generator haveTry
	Generator->callersHaveTry = haveTry;
	haveTry = generatorsHaveTry;

	return ret;
}
#endif /*NO_GENERATORS*/



inline CScriptVarPtr CTinyJS::mathsOp(CScriptResult &execute, const CScriptVarPtr &A, const CScriptVarPtr &B, int op) {
	if(!execute) return constUndefined;
	if (op == LEX_TYPEEQUAL || op == LEX_NTYPEEQUAL) {
		// check type first
		if( (A->getVarType() == B->getVarType()) ^ (op == LEX_TYPEEQUAL)) return constFalse;
		// check value second
		return mathsOp(execute, A, B, op == LEX_TYPEEQUAL ? LEX_EQUAL : LEX_NEQUAL);
	}
	if (!A->isPrimitive() && !B->isPrimitive()) { // Objects both
		// check pointers
		switch (op) {
		case LEX_EQUAL:	return constScriptVar(A==B);
		case LEX_NEQUAL:	return constScriptVar(A!=B);
		}
	}

	CScriptVarPtr a = A->toPrimitive(execute);
	CScriptVarPtr b = B->toPrimitive(execute);
	if(!execute) return constUndefined;
	// do maths...
	bool a_isString = a->isString();
	bool b_isString = b->isString();
	// both a String or one a String and op='+'
	if( (a_isString && b_isString) || ((a_isString || b_isString) && op == '+')) {
		std::string da = a->isNull() ? "" : a->toString(execute);
		std::string db = b->isNull() ? "" : b->toString(execute);
		switch (op) {
		case '+': case LEX_PLUSEQUAL:
			try{
				return newScriptVar(da+db);
			} catch(std::exception& e) {
				throwError(execute, Error, e.what());
				return constUndefined;
			}
		case LEX_EQUAL:	return constScriptVar(da==db);
		case LEX_NEQUAL:	return constScriptVar(da!=db);
		case '<':			return constScriptVar(da<db);
		case LEX_LEQUAL:	return constScriptVar(da<=db);
		case '>':			return constScriptVar(da>db);
		case LEX_GEQUAL:	return constScriptVar(da>=db);
		}
	}
	// special for undefined and null --> every true: undefined==undefined, undefined==null, null==undefined and null=null
	else if( (a->isUndefined() || a->isNull()) && (b->isUndefined() || b->isNull()) ) {
		switch (op) {
		case LEX_EQUAL:	return constScriptVar(true);
		case LEX_NEQUAL:
		case LEX_GEQUAL:
		case LEX_LEQUAL:
		case '<':
		case '>':			return constScriptVar(false);
		}
	}
	CNumber da = a->toNumber();
	CNumber db = b->toNumber();
	switch (op) {
	case '+': case LEX_PLUSEQUAL:								return a->newScriptVar(da+db);
	case '-': case LEX_MINUSEQUAL:								return a->newScriptVar(da-db);
	case '*': case LEX_ASTERISKEQUAL:							return a->newScriptVar(da*db);
	case '/': case LEX_SLASHEQUAL:								return a->newScriptVar(da/db);
	case '%': case LEX_PERCENTEQUAL:							return a->newScriptVar(da%db);
	case '&': case LEX_ANDEQUAL:								return a->newScriptVar(da.toInt32()&db.toInt32());
	case '|': case LEX_OREQUAL:									return a->newScriptVar(da.toInt32()|db.toInt32());
	case '^': case LEX_XOREQUAL:								return a->newScriptVar(da.toInt32()^db.toInt32());
	case '~':													return a->newScriptVar(~da);
	case LEX_LSHIFT: case LEX_LSHIFTEQUAL:						return a->newScriptVar(da<<db);
	case LEX_RSHIFT: case LEX_RSHIFTEQUAL:						return a->newScriptVar(da>>db);
	case LEX_RSHIFTU: case LEX_RSHIFTUEQUAL:					return a->newScriptVar(da.ushift(db));
	case LEX_ASTERISKASTERISK: case LEX_ASTERISKASTERISKEQUAL:	return a->newScriptVar(da.pow(db));
	case LEX_EQUAL:												return a->constScriptVar(da==db);
	case LEX_NEQUAL:											return a->constScriptVar(da!=db);
	case '<':													return a->constScriptVar(da<db);
	case LEX_LEQUAL:											return a->constScriptVar(da<=db);
	case '>':													return a->constScriptVar(da>db);
	case LEX_GEQUAL:											return a->constScriptVar(da>=db);
	default: throw CScriptException("This operation not supported on the int datatype");
	}
}

void CTinyJS::assign_destructuring_var(CScriptResult &execute, const std::shared_ptr<CScriptTokenDataDestructuringVar> &Objc, const CScriptVarPtr &Val, const CScriptVarPtr &Scope)
{
	if(!execute) return;
	if(Objc->vars.size() == 1) {
		if(Scope)
			Scope->addChildOrReplace(Objc->vars.front().second, Val);
		else {
			CScriptVarLinkWorkPtr v(findInScopes(Objc->vars.front().second));
			ASSERT(v==true);
			if(v) v->setVarPtr(Val);
		}
	} else {
		DESTRUCTURING_VARS_cit it=Objc->vars.begin();
		std::vector<CScriptVarPtr> Path(1, Val);
		STRING_VECTOR_t PathStr(1,(it++)->second == "{" ? "." : "[");

		if(Val->isNullOrUndefined() && it->second != "}" && it->second != "]") {
			throwError(execute, TypeError, (Val->isNull()?"null has no properties":"undefined has no properties"));
			return;
		}


		for(; it!=Objc->vars.end(); ++it) {
			if(it->second == "}" || it->second == "]") {
				PathStr.pop_back();
				Path.pop_back();
			} else {
				if(it->second.empty()) continue; // skip empty entries
				CScriptVarLinkWorkPtr var = Path.back()->getOwnProperty(it->first);
				if(var) var = var.getter(execute); else var = constUndefined;
				if(!execute) return;
				if(it->second == "{" || it->second == "[") {
					std::string newPathStr = PathStr.back() +  it->first;
					if(*PathStr.back().rbegin() == '[') newPathStr += "]";
					if(var->getVarPtr()->isNullOrUndefined() && (it+1)->second != "}" && (it+1)->second != "]") {
						throwError(execute, TypeError, newPathStr+" is "+(var->getVarPtr()->isNull() ? "null" : "undefined"));
						return;
					}
					PathStr.push_back(newPathStr + (it->second == "{" ? "." : "["));
					Path.push_back(var);
				} else if(Scope)
					Scope->addChildOrReplace(it->second, var);
				else {
					CScriptVarLinkWorkPtr v(findInScopes(it->second));
					ASSERT(v==true);
					if(v) v->setVarPtr(var);
				}
			}
		}
	}
}

inline void CTinyJS::execute_var_init( CScriptResult &execute, bool hideLetScope ) {
	for(;;) {
		t->check(LEX_T_DESTRUCTURING_VAR);
		auto &Objc = t->getToken().DestructuringVar();
		t->match(LEX_T_DESTRUCTURING_VAR);
		if(t->tk == '=') {
			t->match('=');
			if(hideLetScope) CScriptVarScopeLetPtr(scope())->setletExpressionInitMode(true);
			CScriptVarPtr Val = execute_assignment(execute);
			if(hideLetScope) CScriptVarScopeLetPtr(scope())->setletExpressionInitMode(false);
			assign_destructuring_var(execute, Objc, Val, 0);
		}
		if (t->tk == ',')
			t->match(',');
		else
			break;
	}
}
void CTinyJS::execute_destructuring(CScriptResult &execute, const std::shared_ptr<CScriptTokenDataObjectLiteral> &Objc, const CScriptVarPtr &Val, const std::string &Path)
{
	if(Val->isNullOrUndefined() && Objc->elements.size()) {
		throwError(execute, TypeError, (Val->isNull()?"null has no properties":"undefined has no properties"));
		return;
	}
	const char *ObjcOpener = (Objc->type==CScriptTokenDataObjectLiteral::OBJECT ? ".":"[");
	const char *ObjcCloser = (Objc->type==CScriptTokenDataObjectLiteral::OBJECT ? "":"]");
	for(std::vector<CScriptTokenDataObjectLiteral::ELEMENT>::iterator it=Objc->elements.begin(); execute && it!=Objc->elements.end(); ++it) {
		if(it->value.empty()) continue;
		CScriptToken &token = it->value.front();
		CScriptVarPtr rhs = Val->getOwnProperty(it->id).getter(execute);
		if(!rhs) rhs=constUndefined;
		if(token.token == LEX_T_OBJECT_LITERAL && token.Object()->destructuring) {
			auto &Objc = token.Object();
			std::string newPath = Path;
			if(newPath.empty()) newPath += "(intermediate value)";
			newPath += ObjcOpener + it->id + ObjcCloser;
			if(rhs->isNullOrUndefined() && Objc->elements.size())
				throwError(execute, TypeError, newPath + (rhs->isNull() ? " is null" : " is undefined"));
			execute_destructuring(execute, Objc, rhs, newPath);
		} else {
			t->pushTokenScope(it->value);
			CScriptVarLinkWorkPtr lhs = execute_condition(execute);
			t->match(LEX_T_END_EXPRESSION); // eat LEX_T_END_EXPRESSION
			if(lhs->isWritable()) {
				if (!lhs->isOwned()) {
					CScriptVarPtr fakedOwner = lhs.getReferencedOwner();
					if(fakedOwner) {
						if(!fakedOwner->isExtensible())
							continue;
						lhs = fakedOwner->addChildOrReplace(lhs->getName(), lhs);
					} else
						lhs = root->addChildOrReplace(lhs->getName(), lhs);
				}
				lhs.setter(execute, rhs);
			}
		}
	}
}

CScriptVarLinkWorkPtr CTinyJS::execute_literals(CScriptResult &execute) {
	switch(t->tk) {
	case LEX_ID:
		if(execute) {
			CScriptVarLinkWorkPtr a(findInScopes(t->tkStr()));
			if (!a) {
				/* Variable doesn't exist! JavaScript says we should create it
				 * (we won't add it here. This is done in the assignment operator)*/
				if(t->tkStr() == "this")
					a = root; // fake this
				else
					a = CScriptVarLinkPtr(constScriptVar(Undefined), t->tkStr());
			}
/*
			prvention for assignment to this is now done by the tokenizer
			else if(t->tkStr() == "this")
				a(a->getVarPtr()); // prevent assign to this
*/
			t->match(LEX_ID);
			return a;
		}
		t->match(LEX_ID);
		break;
	case LEX_INT:
		{
			CScriptVarPtr a = newScriptVar(t->getToken().Int());
			a->setExtensible(false);
			t->match(LEX_INT);
			return a;
		}
		break;
	case LEX_FLOAT:
		{
			CScriptVarPtr a = newScriptVar(t->getToken().Float());
			t->match(LEX_FLOAT);
			return a;
		}
		break;
	case LEX_STR:
		{
			CScriptVarPtr a = newScriptVar(t->getToken().String());
			t->match(LEX_STR);
			return a;
		}
		break;
#ifndef NO_REGEXP
	case LEX_REGEXP:
		{
			std::string::size_type pos = t->getToken().String().find_last_of('/');
			std::string source = t->getToken().String().substr(1, pos-1);
			std::string flags = t->getToken().String().substr(pos+1);
			CScriptVarPtr a = newScriptVar(source, flags);
			t->match(LEX_REGEXP);
			return a;
		}
		break;
#endif /* NO_REGEXP */
	case LEX_T_OBJECT_LITERAL:
		if(execute) {
			auto &Objc = t->getToken().Object();
			t->match(LEX_T_OBJECT_LITERAL);
			if(Objc->destructuring) {
				t->match('=');
				CScriptVarLinkPtr a = execute_assignment(execute);
				if(execute) execute_destructuring(execute, Objc, a, a->getName());
				return a;
			} else {
				CScriptVarPtr a = Objc->type==CScriptTokenDataObjectLiteral::OBJECT ? newScriptVar(Object) : newScriptVar(Array);
				if(Objc->type >= CScriptTokenDataObjectLiteral::ARRAY_COMPREHENSIONS) {
					CScopeControl ScopeControl(this);
					ScopeControl.addLetScope();// add a LetScope
					scope()->scopeLet()->addChild("__ARRAY_COMPREHENSIONS__", a, SCRIPTVARLINK_VARDEFAULT);
					t->pushTokenScope(Objc->elements[Objc->type - CScriptTokenDataObjectLiteral::ARRAY_COMPREHENSIONS].value);
					execute_statement(execute);
				} else {
					for(std::vector<CScriptTokenDataObjectLiteral::ELEMENT>::iterator it=Objc->elements.begin(); execute && it!=Objc->elements.end(); ++it) {
						if(it->value.empty()) continue;
						CScriptToken &tk = it->value.front();
						if(tk.token==LEX_T_GET || tk.token==LEX_T_SET) {
							auto &Fnc = tk.Fnc();
							if((tk.token == LEX_T_GET && Fnc->arguments.size()==0) || (tk.token == LEX_T_SET && Fnc->arguments.size()==1)) {
								CScriptVarLinkWorkPtr funcVar = parseFunctionDefinition(tk);
								CScriptVarLinkWorkPtr child = a->findChild(Fnc->name);
								if(child && !child->getVarPtr()->isAccessor()) child.clear();
								if(!child) child = a->addChildOrReplace(Fnc->name, newScriptVar(Accessor));
								child->getVarPtr()->addChildOrReplace((tk.token==LEX_T_GET?TINYJS_ACCESSOR_GET_VAR:TINYJS_ACCESSOR_SET_VAR), funcVar->getVarPtr());
							}
						} else if (tk.token == LEX_T_GENERATOR_MEMBER) {
							//auto& Fnc = tk.Fnc();
							CScriptVarLinkWorkPtr funcVar = parseFunctionDefinition(tk);
							a->addChildOrReplace(it->id, funcVar->getVarPtr());
						} else {
							t->pushTokenScope(it->value);
							if (it->id == "__proto__")
								a->setPrototype(execute_assignment(execute));
							else
								a->addChildOrReplace(it->id, 0).setter(execute, execute_assignment(execute));
							t->match(LEX_T_END_EXPRESSION); // eat LEX_T_END_EXPRESSION
						}
					}
				}
				return a;
			}
		} else
			t->match(LEX_T_OBJECT_LITERAL);
		break;
	case LEX_R_LET: // let as expression
		if(execute) {
			CScopeControl ScopeControl(this);
			t->match(LEX_R_LET);
			t->match('(');
			t->check(LEX_T_FORWARD);
			ScopeControl.addLetScope();
			execute_statement(execute); // execute forwarder
			execute_var_init(execute, true);
			t->match(')');
			return execute_assignment(execute);
		} else {
			t->skip(t->getToken().Int());
		}
		break;
	case LEX_T_FUNCTION_OPERATOR:
		if(execute) {
			CScriptVarLinkWorkPtr a = parseFunctionDefinition(t->getToken());
			t->match(LEX_T_FUNCTION_OPERATOR);
			return a;
		}
		t->match(LEX_T_FUNCTION_OPERATOR);
		break;
#ifndef NO_GENERATORS
	case LEX_R_YIELD:
		if (execute) {
			t->match(LEX_R_YIELD);
			CScriptVarPtr result = constUndefined;
			if (t->tk != ';')
				result = execute_base(execute);
			if(execute)
				return generator_yield(execute, result.getVar());
			else
				return constUndefined;
		} else
			t->skip(t->getToken().Int());
		break;
#endif /*NO_GENERATORS*/
	case LEX_R_NEW: // new -> create a new object
		if (execute) {
			t->match(LEX_R_NEW);
			CScriptVarLinkWorkPtr parent = execute_literals(execute);
			CScriptVarLinkWorkPtr objClass = execute_member(parent, execute).getter(execute);
			if (execute) {
				CScriptVarFunctionPtr Constructor = objClass->getVarPtr();
				if(Constructor) {
					CScriptVarLinkPtr prototype = Constructor->findChild(TINYJS_PROTOTYPE_CLASS);
					if(!prototype || !prototype->getVarPtr()->isObject()) {
						prototype = Constructor->addChild(TINYJS_PROTOTYPE_CLASS, newScriptVar(Object), SCRIPTVARLINK_WRITABLE);
					}
					CScriptVarPtr obj(newScriptVar(Object, prototype));
					CScriptVarFunctionPtr __constructor__ = Constructor->getConstructor();
					if(__constructor__)
						Constructor = __constructor__;
					if (stackBase) {
						int dummy = 0;
						if(&dummy < stackBase)
							throwError(execute, Error, "too much recursion");
					}
					std::vector<CScriptVarPtr> arguments;
					if (t->tk == '(') {
						t->match('(');
						while(t->tk!=')') {
							CScriptVarPtr value = execute_assignment(execute).getter(execute);
							if (execute)
								arguments.push_back(value);
							if (t->tk!=')') t->match(',', ')');
						}
						t->match(')');
					}
					if(execute) {
						CScriptVarPtr returnVar = callFunction(execute, Constructor, arguments, obj, &obj);
						if(returnVar->isObject())
							return CScriptVarLinkWorkPtr(returnVar);
						return CScriptVarLinkWorkPtr(obj);
					}
				} else
					throwError(execute, TypeError, objClass->getName() + " is not a constructor");
			} else
				if (t->tk == '(') t->skip(t->getToken().Int());
		} else
			t->skip(t->getToken().Int());
		break;
	case LEX_R_TRUE:
		t->match(LEX_R_TRUE);
		return constScriptVar(true);
	case LEX_R_FALSE:
		t->match(LEX_R_FALSE);
		return constScriptVar(false);
	case LEX_R_NULL:
		t->match(LEX_R_NULL);
		return constScriptVar(Null);
	case '(':
		if(execute) {
			t->match('(');
			CScriptVarLinkWorkPtr a = execute_base(execute).getter(execute);
			t->match(')');
			return a;
		} else
			t->skip(t->getToken().Int());
		break;
	case LEX_T_EXCEPTION_VAR:
		t->match(LEX_T_EXCEPTION_VAR);
		if(execute.value) return execute.value;
		break;
	default:
		t->match(LEX_EOF);
		break;
	}
	return constScriptVar(Undefined);

}
inline CScriptVarLinkWorkPtr CTinyJS::execute_member(CScriptVarLinkWorkPtr &parent, CScriptResult &execute) {
	CScriptVarLinkWorkPtr a;
	parent.swap(a);
	bool chaining_state = true;
	while (t->tk == '.' || t->tk == LEX_OPTIONAL_CHAINING_MEMBER || t->tk == '[' || t->tk == LEX_OPTIONAL_CHAINING_ARRAY) {
		if (execute) {
			a = a.getter(execute); // a is now the "getted" var
			parent.swap(a);
			if (execute && parent->getVarPtr()->isNullOrUndefined()) {
				if (t->tk == LEX_OPTIONAL_CHAINING_MEMBER || t->tk == LEX_OPTIONAL_CHAINING_ARRAY) {
					chaining_state = false;
					execute.set(CScriptResult::noExecute, false);
					a(constScriptVar(Undefined));

				} else
					throwError(execute, ReferenceError, parent->getName() + " is " + parent->toString(execute));
			}
		}
		std::string name;
		if (t->tk == '.' || t->tk == LEX_OPTIONAL_CHAINING_MEMBER) {
			t->match(t->tk);
			name = t->tkStr();
			t->match(LEX_ID);
		} else {
			if (execute) {
				t->match(t->tk);
				name = execute_base(execute)->toString(execute);
				t->match(']');
			} else
				t->skip(t->getToken().Int());
		}
		if (execute) {
			CScriptVarPtr parentVar = parent;
			a = parentVar->findChildWithPrototypeChain(name);
			if (!a) {
				a(constScriptVar(Undefined), name);
				a.setReferencedOwner(parentVar);
			}
		}
	}
	if (chaining_state == false) {
		execute.set(CScriptResult::Normal, false);
	}
	return a;
}

inline CScriptVarLinkWorkPtr CTinyJS::execute_function_call(CScriptResult &execute) {
	CScriptVarLinkWorkPtr parent = execute_literals(execute);
	CScriptVarLinkWorkPtr a = execute_member(parent, execute);
	bool chaining_state = true;
	while (t->tk == '(' || t->tk == LEX_OPTIONAL_CHANING_FNC) {
		if (execute) {
			a = a.getter(execute);
			if (execute && a->getVarPtr()->isNullOrUndefined()) {
				if (t->tk == LEX_OPTIONAL_CHANING_FNC) {
					chaining_state = false;
					execute.set(CScriptResult::noExecute, false);
					a(constScriptVar(Undefined));
				} else
					throwError(execute, ReferenceError, a->getName() + " is " + a->toString(execute));
			}
		}
		if (execute) {
			CScriptVarPtr fnc = a->getVarPtr();
			if (!fnc->isFunction())
				throwError(execute, TypeError, a->getName() + " is not a function");
			if (stackBase) {
				int dummy = 0;
				if(&dummy < stackBase)
					throwError(execute, Error, "too much recursion");
			}

			t->match(t->tk); // path += '(';

			// grab in all parameters
			std::vector<CScriptVarPtr> arguments;
			while(t->tk!=')') {
				CScriptVarLinkWorkPtr value = execute_assignment(execute).getter(execute);
//				path += (*value)->getString();
				if (execute) {
					arguments.push_back(value);
				}
				if (t->tk!=')') { t->match(','); /*path+=',';*/ }
			}
			t->match(')'); //path+=')';
			// setup a return variable
			CScriptVarLinkWorkPtr returnVar;
			if(execute) {
				if (!parent)
					parent = findInScopes("this");
				// if no parent use the root-scope
				CScriptVarPtr This(parent ? parent->getVarPtr() : (CScriptVarPtr )root);
				a = callFunction(execute, fnc, arguments, This);
			}
		} else {
			// function, but not executing - just parse args and be done
			t->match(t->tk);
			while (t->tk != ')') {
				CScriptVarLinkWorkPtr value = execute_base(execute);
				//	if (t->tk!=')') t->match(',');
			}
			t->match(')');
		}
		a = execute_member(parent = a, execute);
	}
	if (chaining_state == false) {
		execute.set(CScriptResult::Normal, false);
	}
	return a;
}
// R->L: Precedence 15 (post-increment/decrement) .++ .--
// R<-L: Precedence 14 (pre-increment/decrement) ++. --.  (unary) ! ~ + - typeof void delete
inline bool CTinyJS::execute_unary_rhs(CScriptResult &execute, CScriptVarLinkWorkPtr& a) {
	t->match(t->tk);
	a = execute_unary(execute).getter(execute);
	if(execute) CheckRightHandVar(execute, a);
	return execute;
};
CScriptVarLinkWorkPtr CTinyJS::execute_unary(CScriptResult &execute) {
	CScriptVarLinkWorkPtr a;
	switch(t->tk) {
	case '-':
		if(execute_unary_rhs(execute, a))
			a(newScriptVar(-a->getVarPtr()->toNumber(execute)));
		break;
	case '+':
		if(execute_unary_rhs(execute, a))
			a = newScriptVar(a->getVarPtr()->toNumber(execute));
		break;
	case '!':
		if(execute_unary_rhs(execute, a))
			a = constScriptVar(!a->getVarPtr()->toBoolean());
		break;
	case '~':
		if(execute_unary_rhs(execute, a))
			a = newScriptVar(~a->getVarPtr()->toNumber(execute).toInt32());
		break;
	case LEX_R_TYPEOF:
		if(execute_unary_rhs(execute, a))
			a = newScriptVar(a->getVarPtr()->getVarType());
		break;
	case LEX_R_VOID:
		if(execute_unary_rhs(execute, a))
			a = constScriptVar(Undefined);
		break;
	case LEX_R_DELETE:
		t->match(LEX_R_DELETE); // delete
		a = execute_unary(execute); // no getter - delete can remove the accessor
		if (execute) {
			// !!! no right-hand-check by delete
			if(a->isOwned() && a->isConfigurable() && a->getName() != "this") {
				a->getOwner()->removeLink(a);	// removes the link from owner
				a = constScriptVar(true);
			}
			else
				a = constScriptVar(false);
		}
		break;
	case LEX_PLUSPLUS:
	case LEX_MINUSMINUS:
		{
			int op = t->tk;
			t->match(op); // pre increment/decrement
			CScriptTokenizer::ScriptTokenPosition ErrorPos = t->getPos();
			a = execute_function_call(execute);
			if (execute) {
				if(a->getName().empty())
					throwError(execute, SyntaxError, std::string("invalid ")+(op==LEX_PLUSPLUS ? "increment" : "decrement")+" operand", ErrorPos);
				else if(!a->isOwned() && !a.hasReferencedOwner() && !a->getName().empty())
					throwError(execute, ReferenceError, a->getName() + " is not defined", ErrorPos);
				CScriptVarPtr res = newScriptVar(a.getter(execute)->getVarPtr()->toNumber(execute).add(op==LEX_PLUSPLUS ? 1 : -1));
				if(a->isWritable()) {
					if(!a->isOwned() && a.hasReferencedOwner() && a.getReferencedOwner()->isExtensible())
						a.getReferencedOwner()->addChildOrReplace(a->getName(), res);
					else
						a.setter(execute, res);
				}
				a = res;
			}
		}
		break;
	default:
		a = execute_function_call(execute);
		break;
	}
	// post increment/decrement
	if (t->tk==LEX_PLUSPLUS || t->tk==LEX_MINUSMINUS) {
		int op = t->tk;
		t->match(op);
		if (execute) {
			if(a->getName().empty())
				throwError(execute, SyntaxError, std::string("invalid ")+(op==LEX_PLUSPLUS ? "increment" : "decrement")+" operand", t->getPrevPos());
			else if(!a->isOwned() && !a.hasReferencedOwner() && !a->getName().empty())
				throwError(execute, ReferenceError, a->getName() + " is not defined", t->getPrevPos());
			CNumber num = a.getter(execute)->getVarPtr()->toNumber(execute);
			CScriptVarPtr res = newScriptVar(num.add(op==LEX_PLUSPLUS ? 1 : -1));
			if(a->isWritable()) {
				if(!a->isOwned() && a.hasReferencedOwner() && a.getReferencedOwner()->isExtensible())
					a.getReferencedOwner()->addChildOrReplace(a->getName(), res);
				else
					a.setter(execute, res);
			}
			a = newScriptVar(num);
		}
	}
	return a;
}

// L<-R: Precedence 13 (exponentiation) **
inline CScriptVarLinkWorkPtr CTinyJS::execute_exponentiation(CScriptResult& execute) {
	CScriptVarLinkWorkPtr a = execute_unary(execute);
	if (t->tk == LEX_ASTERISKASTERISK) {
		CheckRightHandVar(execute, a);
		t->match(t->tk);
		CScriptVarLinkWorkPtr b = execute_exponentiation(execute); // L<-R
		if (execute) {
			CheckRightHandVar(execute, b);
			a = mathsOp(execute, a.getter(execute), b.getter(execute), LEX_ASTERISKASTERISK);
		}
	}
	return a;
}


// L->R: Precedence 12 (term) * / %
inline CScriptVarLinkWorkPtr CTinyJS::execute_term(CScriptResult &execute) {
	CScriptVarLinkWorkPtr a = execute_exponentiation(execute);
	if (t->tk=='*' || t->tk=='/' || t->tk=='%') {
		CheckRightHandVar(execute, a);
		while (t->tk=='*' || t->tk=='/' || t->tk=='%') {
			int op = t->tk;
			t->match(t->tk);
			CScriptVarLinkWorkPtr b = execute_exponentiation(execute); // L->R
			if (execute) {
				CheckRightHandVar(execute, b);
				a = mathsOp(execute, a.getter(execute), b.getter(execute), op);
			}
		}
	}
	return a;
}

// L->R: Precedence 11 (addition/subtraction) + -
inline CScriptVarLinkWorkPtr CTinyJS::execute_expression(CScriptResult &execute) {
	CScriptVarLinkWorkPtr a = execute_term(execute);
	if (t->tk=='+' || t->tk=='-') {
		CheckRightHandVar(execute, a);
		while (t->tk=='+' || t->tk=='-') {
			int op = t->tk;
			t->match(t->tk);
			CScriptVarLinkWorkPtr b = execute_term(execute); // L->R
			if (execute) {
				CheckRightHandVar(execute, b);
				a = mathsOp(execute, a.getter(execute), b.getter(execute), op);
			}
		}
	}
	return a;
}

// L->R: Precedence 10 (bitwise shift) << >> >>>
inline CScriptVarLinkWorkPtr CTinyJS::execute_binary_shift(CScriptResult &execute) {
	CScriptVarLinkWorkPtr a = execute_expression(execute);
	if (t->tk==LEX_LSHIFT || t->tk==LEX_RSHIFT || t->tk==LEX_RSHIFTU) {
		CheckRightHandVar(execute, a);
		while (t->tk>=LEX_SHIFTS_BEGIN && t->tk<=LEX_SHIFTS_END) {
			int op = t->tk;
			t->match(t->tk);

			CScriptVarLinkWorkPtr b = execute_expression(execute); // L->R
			if (execute) {
				CheckRightHandVar(execute, a);
				 // not in-place, so just replace
				 a = mathsOp(execute, a.getter(execute), b.getter(execute), op);
			}
		}
	}
	return a;
}
// L->R: Precedence 9 (relational) < <= > <= in instanceof
// L->R: Precedence 8 (equality) == != === !===
inline CScriptVarLinkWorkPtr CTinyJS::execute_relation(CScriptResult &execute, int set, int set_n) {
	CScriptVarLinkWorkPtr a = set_n ? execute_relation(execute, set_n, 0) : execute_binary_shift(execute);
	if ((set==LEX_EQUAL && t->tk>=LEX_EQUALS_BEGIN && t->tk<=LEX_EQUALS_END)
				||	(set=='<' && (t->tk==LEX_LEQUAL || t->tk==LEX_GEQUAL || t->tk=='<' || t->tk=='>' || t->tk == LEX_R_IN || t->tk == LEX_R_INSTANCEOF))) {
		CheckRightHandVar(execute, a);
		a = a.getter(execute);
		while ((set==LEX_EQUAL && t->tk>=LEX_EQUALS_BEGIN && t->tk<=LEX_EQUALS_END)
					||	(set=='<' && (t->tk==LEX_LEQUAL || t->tk==LEX_GEQUAL || t->tk=='<' || t->tk=='>' || t->tk == LEX_R_IN || t->tk == LEX_R_INSTANCEOF))) {
			int op = t->tk;
			t->match(t->tk);
			CScriptVarLinkWorkPtr b = set_n ? execute_relation(execute, set_n, 0) : execute_binary_shift(execute); // L->R
			if (execute) {
				CheckRightHandVar(execute, b);
				std::string nameOf_b = b->getName();
				b = b.getter(execute);
				if(op == LEX_R_IN) {
					if(!b->getVarPtr()->isObject())
						throwError(execute, TypeError, "invalid 'in' operand "+nameOf_b);
					a(constScriptVar( (bool)b->getVarPtr()->findChildWithPrototypeChain(a->toString(execute))));
				} else if(op == LEX_R_INSTANCEOF) {
					CScriptVarPtr prototype = b->getVarPtr()->getPrototype();
					if(!prototype)
						throwError(execute, TypeError, "invalid 'instanceof' operand "+nameOf_b);
					else {
						unsigned int uniqueID = allocUniqueID();
						CScriptVarPtr object = a->getVarPtr()->getPrototype();
						while( object && object!=prototype && object->getTemporaryMark() != uniqueID) {
							object->setTemporaryMark(uniqueID); // prevents recursions
							object = object->getPrototype();
						}
						freeUniqueID();
						a(constScriptVar(object && object==prototype));
					}
				} else
					a = mathsOp(execute, a, b, op);
			}
		}
	}
	return a;
}

// L->R: Precedence 7 (bitwise-and) &
// L->R: Precedence 6 (bitwise-xor) ^
// L->R: Precedence 5 (bitwise-or) |
inline CScriptVarLinkWorkPtr CTinyJS::execute_binary_logic(CScriptResult &execute, int op, int op_n1, int op_n2) {
	CScriptVarLinkWorkPtr a = op_n1 ? execute_binary_logic(execute, op_n1, op_n2, 0) : execute_relation(execute);
	if (t->tk==op) {
		CheckRightHandVar(execute, a);
		a = a.getter(execute);
		while (t->tk==op) {
			t->match(t->tk);
			CScriptVarLinkWorkPtr b = op_n1 ? execute_binary_logic(execute, op_n1, op_n2, 0) : execute_relation(execute); // L->R
			if (execute) {
				CheckRightHandVar(execute, b);
				a = mathsOp(execute, a, b.getter(execute), op);
			}
		}
	}
	return a;
}
// L->R: Precedence 4 ==> (logical-and) &&
// L->R: Precedence 3 ==> (logical-or) ||  (nullish) ??
inline CScriptVarLinkWorkPtr CTinyJS::execute_logic(CScriptResult &execute, int op /*= LEX_OROR*/, int op_n /*= LEX_ANDAND*/) {
	CScriptVarLinkWorkPtr a = op_n ? execute_logic(execute, op_n, 0) : execute_binary_logic(execute);
	if (t->tk==op || (op == LEX_OROR && t->tk == LEX_ASKASK)) {
		if(execute) {
			CScriptVarLinkWorkPtr b;
			CheckRightHandVar(execute, a);
			a(a.getter(execute)); // rebuild a
			do {
				if(execute && (op==LEX_ANDAND ? a->toBoolean() : (t->tk == LEX_ASKASK ? a->getVarPtr()->isNullOrUndefined() : !a->toBoolean()))) {
					t->match(t->tk);
					b = op_n ? execute_logic(execute, op_n, 0) : execute_binary_logic(execute);
					CheckRightHandVar(execute, b); a(b.getter(execute)); // rebuild a
				} else
					t->skip(t->getToken().Int());
			} while(t->tk==op || (op == LEX_OROR && t->tk == LEX_ASKASK));
		} else
			t->skip(t->getToken().Int());
	}
	return a;
}

// L<-R: Precedence 2 (condition) ?:
inline CScriptVarLinkWorkPtr CTinyJS::execute_condition(CScriptResult &execute) {
	CScriptVarLinkWorkPtr a = execute_logic(execute);
	if (t->tk=='?') {
		CheckRightHandVar(execute, a);
		bool cond = execute && a.getter(execute)->toBoolean();
		if(execute) {
			if(cond) {
				t->match('?');
//				a = execute_condition(execute);
				a = execute_assignment(execute);
				t->check(':');
				t->skip(t->getToken().Int());
			} else {
				CScriptVarLinkWorkPtr b;
				t->skip(t->getToken().Int());
				t->match(':');
				return execute_assignment(execute);
//				return execute_condition(execute);
			}
		} else {
			t->skip(t->getToken().Int());
			t->skip(t->getToken().Int());
		}
	}
	return a;
}

// L<-R: Precedence 2 (assignment) = += -= *= /= %= <<= >>= >>>= &= |= ^=
// now we can return CScriptVarLinkPtr execute_assignment returns always no setters/getters
// force life of the Owner is no more needed
inline CScriptVarLinkPtr CTinyJS::execute_assignment(CScriptResult &execute) {
	return execute_assignment(execute_condition(execute), execute);
}

inline CScriptVarLinkPtr CTinyJS::execute_assignment(CScriptVarLinkWorkPtr lhs, CScriptResult &execute) {
	if (t->tk=='=' || (t->tk>=LEX_ASSIGNMENTS_BEGIN && t->tk<=LEX_ASSIGNMENTS_END)) {
		int op = t->tk;
		CScriptTokenizer::ScriptTokenPosition leftHandPos = t->getPos();
		t->match(t->tk);
		if (execute) {
			if (op != '=' && !lhs->isOwned()) {
				throwError(execute, ReferenceError, lhs->getName() + " is not defined");
			}
			if (op == LEX_ASKASKEQUAL) {
				CScriptVarLinkWorkPtr ret = lhs.getter(execute);
				if (ret->getVarPtr()->isNullOrUndefined())
					op = '='; // lhs == null or undefined execute assignment
				else {
					CScriptResult e(CScriptResult::noExecute);
					execute_assignment(e); // no execute rhs
					return ret; // lhs.getter(execute);
				}
			}
		}
		CScriptVarLinkWorkPtr rhs = execute_assignment(execute).getter(execute); // L<-R
		if (execute) {
			if (!lhs->isOwned() && !lhs.hasReferencedOwner() && lhs->getName().empty()) {
				throw CScriptException(ReferenceError, "invalid assignment left-hand side (at runtime)", t->currentFile, leftHandPos.currentLine(), leftHandPos.currentColumn());
			} else if (op != '=' && !lhs->isOwned()) {
				throwError(execute, ReferenceError, lhs->getName() + " is not defined");
			}
			else if(lhs->isWritable()) {
				if (op=='=') {
					if (!lhs->isOwned()) {
						CScriptVarPtr fakedOwner = lhs.getReferencedOwner();
						if(fakedOwner) {
							if(!fakedOwner->isExtensible())
								return rhs->getVarPtr();
							lhs = fakedOwner->addChildOrReplace(lhs->getName(), lhs);
						} else
							lhs = root->addChildOrReplace(lhs->getName(), lhs);
					}
					lhs.setter(execute, rhs);
					return rhs->getVarPtr();
				} else {
					CScriptVarPtr result;
					//static int assignments[] = {'+', '-', '*', '/', '%', LEX_LSHIFT, LEX_RSHIFT, LEX_RSHIFTU, '&', '|', '^'};
					result = mathsOp(execute, lhs.getter(execute), rhs, op /*assignments[op - LEX_PLUSEQUAL]*/);
					lhs.setter(execute, result);
					return result;
				}
			} else {
				// lhs is not writable we ignore lhs & use rhs
				return rhs->getVarPtr();
			}
		}
	}
	else
		CheckRightHandVar(execute, lhs);
	return lhs.getter(execute);
}
// L->R: Precedence 1 (comma) ,
inline CScriptVarLinkPtr CTinyJS::execute_base(CScriptResult &execute) {
	CScriptVarLinkPtr a;
	for(;;)
	{
		a = execute_assignment(execute); // L->R
		if (t->tk == ',') {
			t->match(',');
		} else
			break;
	}
	return a;
}
inline void CTinyJS::execute_block(CScriptResult &execute) {
	if(execute) {
		t->match('{');
		CScopeControl ScopeControl(this);
		if(t->tk==LEX_T_FORWARD) // add a LetScope only if needed
			ScopeControl.addLetScope();
		while (t->tk && t->tk!='}')
			execute_statement(execute);
		t->match('}');
		// scopes.pop_back();
	}
	else
		t->skip(t->getToken().Int());
}
void CTinyJS::execute_statement(CScriptResult &execute) {
	switch(t->tk) {
	case '{':		/* A block of code */
		execute_block(execute);
		break;
	case ';':		/* Empty statement - to allow things like ;;; */
		t->match(';');
		break;
	case LEX_T_FORWARD:
		{
			CScriptVarPtr in_scope = scope()->scopeLet();
			STRING_SET_t *varNames = t->getToken().Forwarder()->varNames;
			for(int i=0; i<CScriptTokenDataForwards::END; ++i) {
				for(STRING_SET_it it=varNames[i].begin(); it!=varNames[i].end(); ++it) {
					CScriptVarLinkPtr a = in_scope->findChild(*it);
					if(!a) in_scope->addChild(*it, constScriptVar(Undefined), i==CScriptTokenDataForwards::CONSTS ? SCRIPTVARLINK_CONSTDEFAULT : SCRIPTVARLINK_VARDEFAULT);
				}
				in_scope = scope()->scopeVar();
			}
			CScriptTokenDataForwards::FNC_SET_t &functions = t->getToken().Forwarder()->functions;
			for(CScriptTokenDataForwards::FNC_SET_it it=functions.begin(); it!=functions.end(); ++it) {
				CScriptVarLinkWorkPtr funcVar = parseFunctionDefinition(*it);
				in_scope->addChildOrReplace(funcVar->getName(), funcVar, SCRIPTVARLINK_VARDEFAULT);
			}
			t->match(LEX_T_FORWARD);
		}
		break;
	case LEX_R_VAR:
	case LEX_R_LET:
	case LEX_R_CONST:
		if(execute)
		{
			CScopeControl ScopeControl(this);
			bool isLet = t->tk==LEX_R_LET, let_ext=false;
			t->match(t->tk);
			if(isLet && t->tk=='(') {
				let_ext = true;
				t->match('(');
				t->check(LEX_T_FORWARD);
				ScopeControl.addLetScope();
				execute_statement(execute); // forwarder
			}
			execute_var_init(execute, let_ext);
			if(let_ext) {
				t->match(')');
				execute_statement(execute);
			} else
				t->match(';');
		} else
			t->skip(t->getToken().Int());
		break;
	case LEX_T_ARRAY_COMPREHENSIONS_BODY:
		{
			auto &body = t->getToken().ArrayComprehensionsBody();
			t->match(LEX_T_ARRAY_COMPREHENSIONS_BODY);
			if(!execute) break;
			CScriptVarArrayPtr a(findInScopes("__ARRAY_COMPREHENSIONS__"));
			t->pushTokenScope(body->body);
			CScriptVarPtr b = execute_assignment(execute);
			t->match(LEX_T_END_EXPRESSION);
			if(execute) a->setArrayElement(a->getLength(), b);
			break;
		}
		break;
	case LEX_R_WITH:
		if(execute) {
			t->match(LEX_R_WITH);
			t->match('(');
			CScriptVarLinkPtr var = execute_base(execute);
			t->match(')');
			CScopeControl ScopeControl(this);
			ScopeControl.addWithScope(var);
			execute_statement(execute);
		} else
			t->skip(t->getToken().Int());
		break;
	case LEX_T_IF:
		{
			auto &IfData = t->getToken().If();
			t->match(LEX_T_IF);
			if(!execute) break;

			t->pushTokenScope(IfData->condition);
			bool cond = execute_base(execute)->toBoolean();
			t->match(LEX_T_END_EXPRESSION); // eat LEX_T_END_EXPRESSION
			if(execute) {
				if(cond) {
					t->pushTokenScope(IfData->if_body);
					execute_statement(execute);
				} else if(IfData->else_body.size()) {
					t->pushTokenScope(IfData->else_body);
					execute_statement(execute);
				}
			}
			break;
		}
	case LEX_T_FOR_IN:
		{
			auto &LoopData = t->getToken().Loop();
			t->match(LEX_T_FOR_IN);
			if(!execute) break;

			CScopeControl ScopeControl(this);
			if(LoopData->init.size()) {
				t->pushTokenScope(LoopData->init);
				ScopeControl.addLetScope();
				execute_statement(execute); // forwarder
			}
			if(!execute) break;

			t->pushTokenScope(LoopData->iter);
			CScriptVarPtr for_in_var = execute_base(execute);

			if(!execute) break;

			CScriptVarPtr Iterator(for_in_var->toIterator(execute, LoopData->type!=CScriptTokenDataLoop::FOR_IN ? RETURN_VALUE : RETURN_KEY));
			CScriptVarFunctionPtr Iterator_next(Iterator->findChildWithPrototypeChain("next").getter(execute));
			if(execute && !Iterator_next) throwError(execute, TypeError, "'" + for_in_var->toString(execute) + "' is not iterable", t->getPrevPos());
			if(!execute) break;
			CScriptResult tmp_execute;
			for(;;) {
				bool old_haveTry = haveTry;
				haveTry = true;
				tmp_execute.set(CScriptResult::Normal, Iterator);
				t->pushTokenScope(LoopData->condition);
				execute_statement(tmp_execute);
				haveTry = old_haveTry;
				if(tmp_execute.isThrow()){
					if(tmp_execute.value != constStopIteration) {
						if(!haveTry)
							throw CScriptException("uncaught exception: '"+tmp_execute.value->toString(CScriptResult())+"'", t->currentFile, t->currentLine(), t->currentColumn());
						else
							execute = tmp_execute;
					}
					break;
				}
				t->pushTokenScope(LoopData->body);
				execute_statement(execute);
				if(!execute) {
					bool Continue = false;
					if(execute.isBreakContinue()
						&&
						(execute.target.empty() || find(LoopData->labels.begin(), LoopData->labels.end(), execute.target) != LoopData->labels.end())) {
							Continue = execute.isContinue();
							execute.set(CScriptResult::Normal, false);
					}
					if(!Continue) break;
				}
			}
		}
		break;
	case LEX_T_LOOP:
		{
			auto &LoopData = t->getToken().Loop();
			t->match(LEX_T_LOOP);
			if(!execute) break;

			CScopeControl ScopeControl(this);
			if(LoopData->type == CScriptTokenDataLoop::FOR) {
				CScriptResult tmp_execute;
				t->pushTokenScope(LoopData->init);
				if(t->tk == LEX_T_FORWARD) {
					ScopeControl.addLetScope();
					execute_statement(tmp_execute); // forwarder
				}
				if(t->tk==LEX_R_VAR || t->tk==LEX_R_LET)
					execute_statement(tmp_execute); // initialisation
				else if (t->tk != ';')
					execute_base(tmp_execute); // initialisation
				if(!execute.updateIsNotNormal(tmp_execute)) break;
			}

			bool loopCond = true;	// Empty Condition -->always true
			if(LoopData->type != CScriptTokenDataLoop::DO && LoopData->condition.size()) {
				t->pushTokenScope(LoopData->condition);
				loopCond = execute_base(execute)->toBoolean();
				if(!execute) break;
			}
			while (loopCond && execute) {
				t->pushTokenScope(LoopData->body);
				execute_statement(execute);
				if(!execute) {
					bool Continue = false;
					if(execute.isBreakContinue()
						&&
						(execute.target.empty() || find(LoopData->labels.begin(), LoopData->labels.end(), execute.target) != LoopData->labels.end())) {
							Continue = execute.isContinue();
							execute.set(CScriptResult::Normal, false);
					}
					if(!Continue) break;
				}
				if(LoopData->type == CScriptTokenDataLoop::FOR && execute && LoopData->iter.size()) {
					t->pushTokenScope(LoopData->iter);
					execute_base(execute);
				}
				if(execute && LoopData->condition.size()) {
					t->pushTokenScope(LoopData->condition);
					loopCond = execute_base(execute)->toBoolean();
				}
			}
		}
		break;
	case LEX_R_BREAK:
	case LEX_R_CONTINUE:
		if (execute)
		{
			CScriptResult::TYPE type = t->tk==LEX_R_BREAK ? CScriptResult::Break : CScriptResult::Continue;
			std::string label;
			t->match(t->tk);
			if(t->tk == LEX_ID) {
				label = t->tkStr();
				t->match(LEX_ID);
			}
			t->match(';');
			execute.set(type, label);
		} else
			t->skip(t->getToken().Int());
		break;
	case LEX_R_RETURN:
		if (execute) {
			t->match(LEX_R_RETURN);
			CScriptVarPtr result = constUndefined;
			if (t->tk != ';')
				result = execute_base(execute);
			t->match(';');
			execute.set(CScriptResult::Return, result);
		} else
			t->skip(t->getToken().Int());
		break;
	case LEX_R_FUNCTION:
		if(execute) {
			CScriptVarLinkWorkPtr funcVar = parseFunctionDefinition(t->getToken());
			scope()->scopeVar()->addChildOrReplace(funcVar->getName(), funcVar, SCRIPTVARLINK_VARDEFAULT);
		}
		[[fallthrough]];
	case LEX_T_FUNCTION_PLACEHOLDER:
		t->match(t->tk);
		break;
	case LEX_T_TRY:
		if(execute) {
			auto &TryData = t->getToken().Try();

			bool old_haveTry = haveTry;
			haveTry = true;

			// execute try-block
			t->pushTokenScope(TryData->tryBlock);
			execute_block(execute);

			bool isThrow = execute.isThrow();

			if(isThrow && execute.value) {
				// execute catch-blocks only if value set (spezial case Generator.close() -> only finally-blocks are executed)
				for(CScriptTokenDataTry::CATCHBLOCKS_it catchBlock = TryData->catchBlocks.begin(); catchBlock!=TryData->catchBlocks.end(); catchBlock++) {
					CScriptResult catch_execute;
					CScopeControl ScopeControl(this);
					ScopeControl.addLetScope();
					t->pushTokenScope(catchBlock->condition); // condition;
					execute_statement(catch_execute); // forwarder
					assign_destructuring_var(catch_execute, catchBlock->indentifiers, execute.value, 0);
					bool condition = true;
					if(catchBlock->condition.size()>1)
						condition = execute_base(catch_execute)->toBoolean();
					if(!catch_execute) {
						execute = catch_execute;
						break;
					} else if(condition) {
						t->pushTokenScope(catchBlock->block); // condition;
						execute_block(catch_execute);
						execute = catch_execute;
						break;
					}
				}
			}
			if(TryData->finallyBlock.size()) {
				CScriptResult finally_execute; // alway execute finally-block
				t->pushTokenScope(TryData->finallyBlock); // finally;
				execute_block(finally_execute);
				execute.updateIsNotNormal(finally_execute);
			}
			// restore haveTry
			haveTry = old_haveTry;
			if(execute.isThrow() && !haveTry) { // (exception in catch or finally or no catch-clause found) and no parent try-block
				if(execute.value->isError())
					throw CScriptVarErrorPtr(execute.value)->toCScriptException();
				throw CScriptException("uncaught exception: '"+execute.value->toString()+"'", execute.throw_at_file, execute.throw_at_line, execute.throw_at_column);
			}

		}
		t->match(LEX_T_TRY);
		break;
	case LEX_R_THROW:
		if(execute) {
			CScriptTokenizer::ScriptTokenPosition tokenPos = t->getPos();
			//		int tokenStart = t->getToken().pos;
			t->match(LEX_R_THROW);
			CScriptVarPtr a = execute_base(execute);
			if(execute) {
				if(haveTry)
					execute.setThrow(a, t->currentFile, tokenPos.currentLine(), tokenPos.currentColumn());
				else
					throw CScriptException("uncaught exception: '"+a->toString(execute)+"'", t->currentFile, tokenPos.currentLine(), tokenPos.currentColumn());
			}
		} else
			t->skip(t->getToken().Int());
		break;
	case LEX_R_SWITCH:
		if(execute) {
			t->match(LEX_R_SWITCH);
			t->match('(');
			CScriptVarPtr SwitchValue = execute_base(execute);
			t->match(')');
			if(execute) {
				t->match('{');
				CScopeControl ScopeControl(this);
				if(t->tk == LEX_T_FORWARD) {
					ScopeControl.addLetScope(); // add let-scope only if needed
					execute_statement(execute); // execute forwarder
				}
				CScriptTokenizer::ScriptTokenPosition defaultStart = t->getPos();
				bool hasDefault = false, found = false;
				while (t->tk) {
					switch(t->tk) {
					case LEX_R_CASE:
						if(!execute)
							t->skip(t->getToken().Int());				// skip up to'}'
						else if(found) {	// execute && found
							t->match(LEX_R_CASE);
							t->skip(t->getToken().Int());				// skip up to ':'
							t->match(':');									// skip ':' and execute all after ':'
						} else {	// execute && !found
							t->match(LEX_R_CASE);
							t->match(LEX_T_SKIP);						// skip 'L_T_SKIP'
							CScriptVarLinkPtr CaseValue = execute_base(execute);
							CaseValue = mathsOp(execute, CaseValue, SwitchValue, LEX_TYPEEQUAL);
							if(execute) {
								found = CaseValue->toBoolean();
								if(found) t->match(':'); 				// skip ':' and execute all after ':'
								else t->skip(t->getToken().Int());	// skip up to next 'case'/'default' or '}'
							} else
								t->skip(t->getToken().Int());			// skip up to next 'case'/'default' or '}'
						}
						break;
					case LEX_R_DEFAULT:
						if(!execute)
							t->skip(t->getToken().Int());				// skip up to'}' NOTE: no extra 'L_T_SKIP' for skipping tp ':'
						else {
							t->match(LEX_R_DEFAULT);
							if(found)
								t->match(':'); 							// skip ':' and execute all after ':'
							else {
								hasDefault = true;						// in fist pass: skip default-area
								defaultStart = t->getPos(); 			// remember pos of default
								t->skip(t->getToken().Int());			// skip up to next 'case' or '}'
							}
						}
						break;
					case '}':
						if(execute && !found && hasDefault) {		// if not found & have default -> execute default
							found = true;
							t->setPos(defaultStart);
							t->match(':');
						} else
							goto end_while; // goto isn't fine but C supports no "break lable;"
						break;
					default:
						ASSERT(found);
						execute_statement(execute);
						break;
					}
				}
end_while:
				t->match('}');
				if(execute.isBreak() && execute.target.empty()) {
					execute.set(CScriptResult::Normal);
				}
			} else
				t->skip(t->getToken().Int());
		} else
			t->skip(t->getToken().Int());
		break;
	case LEX_T_DUMMY_LABEL:
		t->match(LEX_T_DUMMY_LABEL);
		t->match(':');
		break;
	case LEX_T_LABEL:
		{
			STRING_VECTOR_t Labels;
			while(t->tk == LEX_T_LABEL) {
				Labels.push_back(t->tkStr());
				t->match(LEX_T_LABEL);
				t->match(':');
			}
			if(execute) {
				execute_statement(execute);
				if(execute.isBreak() && find(Labels.begin(), Labels.end(), execute.target) != Labels.end()) { // break this label
					execute.set(CScriptResult::Normal, false);
				}
			}
			else
				execute_statement(execute);
		}
		break;
	case LEX_EOF:
		t->match(LEX_EOF);
		break;
	default:
		if(t->tk!=LEX_T_SKIP || execute) {
			if(t->tk==LEX_T_SKIP) t->match(LEX_T_SKIP);
			/* Execute a simple statement that only contains basic arithmetic... */
			CScriptVarPtr ret = execute_base(execute);
			if(execute) execute.set(CScriptResult::Normal, CScriptVarPtr(ret));
			t->match(';');
		} else
			t->skip(t->getToken().Int());
		break;
	}
}


/// Finds a child, looking recursively up the scopes
CScriptVarLinkPtr CTinyJS::findInScopes(const std::string &childName) {
	return scope()->findInScopes(childName);
}

//////////////////////////////////////////////////////////////////////////
/// Object
//////////////////////////////////////////////////////////////////////////

void CTinyJS::native_Object(const CFunctionsScopePtr &c, void *data) {
	c->setReturnVar(c->getArgument(0)->toObject());
}
void CTinyJS::native_Object_getPrototypeOf(const CFunctionsScopePtr &c, void *data) {
	if (c->getArgumentsLength() >= 1) {
		CScriptVarPtr obj = c->getArgument(0);
		if (obj->isObject()) {
			c->setReturnVar(obj->getPrototype());
			return;
		}
	}
	c->throwError(TypeError, "argument is not an object");
}
void CTinyJS::native_Object_setPrototypeOf(const CFunctionsScopePtr &c, void *data) {
	CScriptVarPtr obj = c->getArgument("obj");
	if (obj->isUndefined()) c->throwError(TypeError, "can't convert undefined to object");
	else if (obj->isNull()) c->throwError(TypeError, "can't convert null to object");
	CScriptVarPtr proto = c->getArgument("proto"); // proto
	if (!obj->isObject() && !obj->isNull()) c->throwError(TypeError, "expected an object or null");
	if (obj->isObject()) obj->setPrototype(proto);
	c->setReturnVar(obj);
}

void CTinyJS::native_Object_prototype_getter__proto__(const CFunctionsScopePtr &c, void *data) {
	CScriptVarPtr obj = c->getArgument("this");
	if (obj->isObject()) {
		c->setReturnVar(obj->getPrototype());
		return;
	}
}

void CTinyJS::native_Object_prototype_setter__proto__(const CFunctionsScopePtr &c, void *data) {
	CScriptVarPtr obj = c->getArgument("this");
	CScriptVarPtr proto = c->getArgument("proto");
	if (obj->isUndefined()) c->throwError(TypeError, "can't convert undefined to object");
	else if (obj->isNull()) c->throwError(TypeError, "can't convert null to object");
	if (!proto->isObject() || !proto->isNull()) return;
	obj->setPrototype(proto);
}

void CTinyJS::native_Object_setObjectSecure(const CFunctionsScopePtr &c, void *data) {
	CScriptVarPtr obj = c->getArgument(0);
	if(!obj->isObject()) c->throwError(TypeError, "argument is not an object");
	if(data==(void*)2)
		obj->freeze();
	else if(data==(void*)1)
		obj->seal();
	else
		obj->preventExtensions();
	c->setReturnVar(obj);
}

void CTinyJS::native_Object_isSecureObject(const CFunctionsScopePtr &c, void *data) {
	CScriptVarPtr obj = c->getArgument(0);
	if(!obj->isObject()) c->throwError(TypeError, "argument is not an object");
	bool ret;
	if(data==(void*)2)
		ret = obj->isFrozen();
	else if(data==(void*)1)
		ret = obj->isSealed();
	else
		ret = obj->isExtensible();
	c->setReturnVar(constScriptVar(ret));
}

void CTinyJS::native_Object_keys(const CFunctionsScopePtr &c, void *data) {
	CScriptVarPtr obj = c->getArgument(0);
	if(!obj->isObject()) c->throwError(TypeError, "argument is not an object");
	CScriptVarPtr returnVar = c->newScriptVar(Array);
	c->setReturnVar(returnVar);

	STRING_SET_t keys;
	obj->keys(keys, data==0);

	uint32_t idx=0;
	for(STRING_SET_it it=keys.begin(); it!=keys.end(); ++it, ++idx)
		returnVar->addChildOrReplace(std::to_string(idx), newScriptVar(*it));
}

void CTinyJS::native_Object_getOwnPropertyDescriptor(const CFunctionsScopePtr &c, void *data) {
	CScriptVarPtr obj = c->getArgument(0);
	if(!obj->isObject()) c->throwError(TypeError, "argument is not an object");
	c->setReturnVar(obj->getOwnPropertyDescriptor(c->getArgument(1)->toString()));
}

void CTinyJS::native_Object_defineProperty(const CFunctionsScopePtr &c, void *data) {
	CScriptVarPtr obj = c->getArgument(0);
	if(!obj->isObject()) c->throwError(TypeError, "argument is not an object");
	std::string name = c->getArgument(1)->toString();
	CScriptVarPtr attributes = c->getArgument(2);
	if(!attributes->isObject()) c->throwError(TypeError, "attributes is not an object");
	const char *err = obj->defineProperty(name, attributes);
	if(err) c->throwError(TypeError, err);
	c->setReturnVar(obj);
}

void CTinyJS::native_Object_defineProperties(const CFunctionsScopePtr &c, void *data) {
	bool ObjectCreate = data!=0;
	CScriptVarPtr obj = c->getArgument(0);
	if(ObjectCreate) {
		if(!obj->isObject() && !obj->isNull()) c->throwError(TypeError, "argument is not an object or null");
		obj = newScriptVar(Object, obj);
	} else
		if(!obj->isObject()) c->throwError(TypeError, "argument is not an object");
	c->setReturnVar(obj);
	if(c->getArgumentsLength()<2) {
		if(ObjectCreate) return;
		c->throwError(TypeError, "Object.defineProperties requires 2 arguments");
	}

	CScriptVarPtr properties = c->getArgument(1);

	STRING_SET_t names;
	properties->keys(names, true);

	for(STRING_SET_it it=names.begin(); it!=names.end(); ++it) {
		CScriptVarPtr attributes = properties->getOwnProperty(*it).getter();
		if(!attributes->isObject()) c->throwError(TypeError, "descriptor for "+*it+" is not an object");
		const char *err = obj->defineProperty(*it, attributes);
		if(err) c->throwError(TypeError, err);
	}
}


//////////////////////////////////////////////////////////////////////////
/// Object.prototype
//////////////////////////////////////////////////////////////////////////

void CTinyJS::native_Object_prototype_hasOwnProperty(const CFunctionsScopePtr &c, void *data) {
	CScriptVarPtr This = c->getArgument("this");
	std::string PropStr = c->getArgument("prop")->toString();
	CScriptVarLinkPtr Prop = This->findChild(PropStr);
	bool res = Prop && !Prop->getVarPtr()->isUndefined();
	if(!res) {
		CScriptVarStringPtr This_asString = This->getRawPrimitive();
		if(This_asString) {
			uint32_t Idx = isArrayIndex(PropStr);
			res = Idx!=uint32_t(-1) && Idx<This_asString->getLength();
		}
	}
	c->setReturnVar(c->constScriptVar(res));
}
void CTinyJS::native_Object_prototype_valueOf(const CFunctionsScopePtr &c, void *data) {
	c->setReturnVar(c->getArgument("this")->valueOf_CallBack());
}
void CTinyJS::native_Object_prototype_toString(const CFunctionsScopePtr &c, void *data) {
	CScriptResult execute;
	int radix = 10;
	if(c->getArgumentsLength()>=1) radix = c->getArgument("radix")->toNumber().toInt32();
	c->setReturnVar(c->getArgument("this")->toString_CallBack(execute, radix));
	if(!execute) {
		execute.cThrow();
	}
}

//////////////////////////////////////////////////////////////////////////
/// Array
//////////////////////////////////////////////////////////////////////////

void CTinyJS::native_Array(const CFunctionsScopePtr &c, void *data) {
	CScriptVarPtr returnVar = c->newScriptVar(Array);
	c->setReturnVar(returnVar);
	uint32_t args = c->getArgumentsLength();
	CScriptVarPtr Argument_0_Var = c->getArgument(0);
	if(data!=0 && args == 1 && Argument_0_Var->isNumber()) {
		CNumber Argument_0 = Argument_0_Var->toNumber();
		if(Argument_0.isUInt32())
			c->setProperty(returnVar, "length", newScriptVar(Argument_0.toUInt32()));
		else
			c->throwError(RangeError, "invalid array length");
	} else for(uint32_t i=0; i<args; i++)
		c->setProperty(returnVar, i, c->getArgument(i));
}

//////////////////////////////////////////////////////////////////////////
/// String
//////////////////////////////////////////////////////////////////////////

void CTinyJS::native_String(const CFunctionsScopePtr &c, void *data) {
	CScriptVarPtr arg;
	if(c->getArgumentsLength()==0)
		arg = newScriptVar("");
	else
		arg = newScriptVar(c->getArgument(0)->toString());
	if(data)
		c->setReturnVar(arg->toObject());
	else
		c->setReturnVar(arg);
}


//////////////////////////////////////////////////////////////////////////
/// RegExp
//////////////////////////////////////////////////////////////////////////
#ifndef NO_REGEXP

void CTinyJS::native_RegExp(const CFunctionsScopePtr &c, void *data) {
	int arglen = c->getArgumentsLength();
	std::string RegExp, Flags;
	if(arglen>=1) {
		RegExp = c->getArgument(0)->toString();
		try { std::regex r(RegExp, std::regex_constants::ECMAScript); } catch(std::regex_error e) {
			c->throwError(SyntaxError, std::string(e.what())+" - "+CScriptVarRegExp::ErrorStr(e.code()));
		}
		if(arglen>=2) {
			Flags = c->getArgument(1)->toString();
			std::string::size_type pos = Flags.find_first_not_of("gimy");
			if(pos != std::string::npos) {
				c->throwError(SyntaxError, std::string("invalid regular expression flag ")+Flags[pos]);
			}
		}
	}
	c->setReturnVar(newScriptVar(RegExp, Flags));
}
#endif /* NO_REGEXP */

//////////////////////////////////////////////////////////////////////////
/// Number
//////////////////////////////////////////////////////////////////////////

void CTinyJS::native_Number(const CFunctionsScopePtr &c, void *data) {
	CScriptVarPtr arg;
	if(c->getArgumentsLength()==0)
		arg = newScriptVar(0);
	else
		arg = newScriptVar(c->getArgument(0)->toNumber());
	if(data)
		c->setReturnVar(arg->toObject());
	else
		c->setReturnVar(arg);
}


//////////////////////////////////////////////////////////////////////////
/// Boolean
//////////////////////////////////////////////////////////////////////////

void CTinyJS::native_Boolean(const CFunctionsScopePtr &c, void *data) {
	CScriptVarPtr arg;
	if(c->getArgumentsLength()==0)
		arg = constScriptVar(false);
	else
		arg = constScriptVar(c->getArgument(0)->toBoolean());
	if(data)
		c->setReturnVar(arg->toObject());
	else
		c->setReturnVar(arg);
}

//////////////////////////////////////////////////////////////////////////
/// Iterator
//////////////////////////////////////////////////////////////////////////

void CTinyJS::native_Iterator(const CFunctionsScopePtr &c, void *data) {
	if(c->getArgumentsLength()<1) c->throwError(TypeError, "missing argument 0 when calling function Iterator");
	c->setReturnVar(c->getArgument(0)->toIterator(c->getArgument(1)->toBoolean() ? RETURN_KEY : RETURN_ARRAY));
}

//////////////////////////////////////////////////////////////////////////
/// Generator
//////////////////////////////////////////////////////////////////////////

#ifndef NO_GENERATORS
void CTinyJS::native_Generator_prototype_next(const CFunctionsScopePtr &c, void *data) {
	CScriptVarGeneratorPtr Generator(c->getArgument("this"));
	if(!Generator) {
		static const char *fnc[] = {"next","send","close","throw"};
		c->throwError(TypeError, std::string(fnc[(ptrdiff_t)data])+" method called on incompatible Object");
	}
	if((ptrdiff_t)data >=2)
		Generator->native_throw(c, (void*)(((ptrdiff_t)data)-2));
	else
		Generator->native_send(c, data);
}
#endif /*NO_GENERATORS*/


//////////////////////////////////////////////////////////////////////////
/// Function
//////////////////////////////////////////////////////////////////////////

void CTinyJS::native_Function(const CFunctionsScopePtr &c, void *data) {
	int length = c->getArgumentsLength();
	std::string params, body;
	if(length>=1)
		body = c->getArgument(length-1)->toString();
	if(length>=2) {
		params = c->getArgument(0)->toString();
		for(int i=1; i<length-1; i++)
		{
			params.append(",");
			params.append(c->getArgument(i)->toString());
		}
	}
	c->setReturnVar(parseFunctionsBodyFromString(params,body));
}

void CTinyJS::native_Function_prototype_call(const CFunctionsScopePtr &c, void *data) {
	int length = c->getArgumentsLength();
	CScriptVarPtr Fnc = c->getArgument("this");
	if(!Fnc->isFunction()) c->throwError(TypeError, "Function.prototype.call called on incompatible Object");
	CScriptVarPtr This = c->getArgument(0);
	std::vector<CScriptVarPtr> Args;
	for(int i=1; i<length; i++)
		Args.push_back(c->getArgument(i));
	c->setReturnVar(callFunction(Fnc, Args, This));
}
void CTinyJS::native_Function_prototype_apply(const CFunctionsScopePtr &c, void *data) {
	int length=0;
	CScriptVarPtr Fnc = c->getArgument("this");
	if(!Fnc->isFunction()) c->throwError(TypeError, "Function.prototype.apply called on incompatible Object");
	// Argument_0
	CScriptVarPtr This = c->getArgument(0)->toObject();
	if(This->isNull() || This->isUndefined()) This=root;
	// Argument_1
	CScriptVarPtr Array = c->getArgument(1);
	if(!Array->isNull() && !Array->isUndefined()) {
		CScriptVarLinkWorkPtr Length = Array->findChild("length");
		if(!Length) c->throwError(TypeError, "second argument to Function.prototype.apply must be an array or an array like object");
		length = Length.getter()->toNumber().toInt32();
	}
	std::vector<CScriptVarPtr> Args;
	for(int i=0; i<length; i++) {
		CScriptVarLinkPtr value = Array->findChild(std::to_string(i));
		if(value) Args.push_back(value);
		else Args.push_back(constScriptVar(Undefined));
	}
	c->setReturnVar(callFunction(Fnc, Args, This));
}
void CTinyJS::native_Function_prototype_bind(const CFunctionsScopePtr &c, void *data) {
	int length = c->getArgumentsLength();
	CScriptVarPtr Fnc = c->getArgument("this");
	if(!Fnc->isFunction()) c->throwError(TypeError, "Function.prototype.bind called on incompatible Object");
	CScriptVarPtr This = c->getArgument(0);
	if(This->isUndefined() || This->isNull()) This = root;
	std::vector<CScriptVarPtr> Args;
	for(int i=1; i<length; i++) Args.push_back(c->getArgument(i));
	c->setReturnVar(newScriptVarFunctionBounded(Fnc, This, Args));
}
void CTinyJS::native_Function_prototype_isGenerator(const CFunctionsScopePtr &c, void *data) {
	c->setReturnVar(constScriptVar(c->getArgument("this")->isGenerator()));
}


//////////////////////////////////////////////////////////////////////////
/// Error
//////////////////////////////////////////////////////////////////////////

static CScriptVarPtr _newError(CTinyJS *context, ERROR_TYPES type, const CFunctionsScopePtr &c) {
	int i = c->getArgumentsLength();
	std::string message, fileName;
	int line=-1, column=-1;
	if(i>0) message	= c->getArgument(0)->toString();
	if(i>1) fileName	= c->getArgument(1)->toString();
	if(i>2) line		= c->getArgument(2)->toNumber().toInt32();
	if(i>3) column		= c->getArgument(3)->toNumber().toInt32();
	return ::newScriptVarError(context, type, message.c_str(), fileName.c_str(), line, column);
}
void CTinyJS::native_Error(const CFunctionsScopePtr &c, void *data) { c->setReturnVar(_newError(this, Error,c)); }
void CTinyJS::native_EvalError(const CFunctionsScopePtr &c, void *data) { c->setReturnVar(_newError(this, EvalError,c)); }
void CTinyJS::native_RangeError(const CFunctionsScopePtr &c, void *data) { c->setReturnVar(_newError(this, RangeError,c)); }
void CTinyJS::native_ReferenceError(const CFunctionsScopePtr &c, void *data){ c->setReturnVar(_newError(this, ReferenceError,c)); }
void CTinyJS::native_SyntaxError(const CFunctionsScopePtr &c, void *data){ c->setReturnVar(_newError(this, SyntaxError,c)); }
void CTinyJS::native_TypeError(const CFunctionsScopePtr &c, void *data){ c->setReturnVar(_newError(this, TypeError,c)); }

//////////////////////////////////////////////////////////////////////////
/// global functions
//////////////////////////////////////////////////////////////////////////
void CTinyJS::native_eval(const CFunctionsScopePtr &c, void *data) {
	std::string Code = c->getArgument("jsCode")->toString();
	CScriptVarScopePtr scEvalScope = scopes.back(); // save scope
	scopes.pop_back(); // go back to the callers scope
	CScriptResult execute;
	CScriptTokenizer *oldTokenizer = t; t=0;
	try {
		CScriptTokenizer Tokenizer(Code.c_str(), "eval");
		t = &Tokenizer;
		do {
			execute_statement(execute);
			while (t->tk==';') t->match(';'); // skip empty statements
		} while (t->tk!=LEX_EOF);
	} catch (CScriptException &e) { // script exceptions
		t = oldTokenizer; // restore tokenizer
		scopes.push_back(scEvalScope); // restore Scopes;
		if(haveTry) { // an Error in eval is always catchable
			CScriptVarPtr E = newScriptVarError(this, e.errorType, e.message.c_str(), e.fileName.c_str(), e.lineNumber, e.column);
			throw E;
		} else
			throw; // rethrow
	} catch (...) { // all other exceptions
		t = oldTokenizer; // restore tokenizer
		scopes.push_back(scEvalScope); // restore Scopes;
		throw; // re-throw
	}
	t = oldTokenizer; // restore tokenizer
	scopes.push_back(scEvalScope); // restore Scopes;
	if(execute.value)
		c->setReturnVar(execute.value);
}

static int _native_require_read(const std::string &Fname, std::string &Data) {
	std::ifstream in(Fname.c_str(), std::ios::in | std::ios::binary);
	if (in) {
		in.seekg(0, std::ios::end);
		Data.resize((std::string::size_type)in.tellg());
		in.seekg(0, std::ios::beg);
		in.read(&Data[0], Data.size());
		in.close();
		return(0);
	}
	return errno;
}

void CTinyJS::native_require(const CFunctionsScopePtr &c, void *data) {
	std::string File = c->getArgument("jsFile")->toString();
	std::string Code;
	int ErrorNo;
	if(!native_require_read)
		native_require_read = _native_require_read; // use builtin if no callback

	if((ErrorNo = native_require_read(File, Code))) {
		std::ostringstream msg;
		msg << "can't read \"" << File << "\" (Error=" << ErrorNo << ")";
		c->throwError(Error, msg.str());
	}
	c->addChildOrReplace("jsCode", c->newScriptVar(Code));
	native_eval(c, data);
}

void CTinyJS::native_isNAN(const CFunctionsScopePtr &c, void *data) {
	c->setReturnVar(constScriptVar(c->getArgument("objc")->toNumber().isNaN()));
}

void CTinyJS::native_isFinite(const CFunctionsScopePtr &c, void *data) {
	c->setReturnVar(constScriptVar(c->getArgument("objc")->toNumber().isFinite()));
}

void CTinyJS::native_parseInt(const CFunctionsScopePtr &c, void *) {
	CNumber result;
	result.parseInt(c->getArgument("std::string")->toString(), c->getArgument("radix")->toNumber().toInt32());
	c->setReturnVar(c->newScriptVar(result));
}

void CTinyJS::native_parseFloat(const CFunctionsScopePtr &c, void *) {
	CNumber result;
	result.parseFloat(c->getArgument("std::string")->toString());
	c->setReturnVar(c->newScriptVar(result));
}



void CTinyJS::native_JSON_parse(const CFunctionsScopePtr &c, void *data) {
	std::string Code = "§" + c->getArgument("text")->toString();
	// "§" is a spezal-token - it's for the tokenizer and means the code begins not in Statement-level
	CScriptVarLinkWorkPtr returnVar;
	CScriptTokenizer *oldTokenizer = t; t=0;
	try {
		CScriptTokenizer Tokenizer(Code.c_str(), "JSON.parse", 0, -1);
		t = &Tokenizer;
		CScriptResult execute;
		returnVar = execute_literals(execute);
		t->match(LEX_EOF);
	} catch (CScriptException &) {
		t = oldTokenizer;
		throw; // rethrow
	}
	t = oldTokenizer;

	if(returnVar)
		c->setReturnVar(returnVar);
}

void CTinyJS::setTemporaryID_recursive(uint32_t ID) {
	for(std::vector<CScriptVarPtr*>::iterator it = pseudo_refered.begin(); it!=pseudo_refered.end(); ++it)
		if(**it) (**it)->setTemporaryMark_recursive(ID);
	for(int i=Error; i<ERROR_COUNT; i++)
		if(errorPrototypes[i]) errorPrototypes[i]->setTemporaryMark_recursive(ID);
	root->setTemporaryMark_recursive(ID);
}

void CTinyJS::ClearUnreferedVars(const CScriptVarPtr &extra/*=CScriptVarPtr()*/) {
	uint32_t UniqueID = allocUniqueID();
	setTemporaryID_recursive(UniqueID);
	if(extra) extra->setTemporaryMark_recursive(UniqueID);
	CScriptVar *p = first;

	while (p)
	{
		if(p->getTemporaryMark() != UniqueID)
		{
			CScriptVarPtr var = p;
			var->cleanUp4Destroy();
			p = var->next;
		}
		else
			p = p->next;
	}
	freeUniqueID();
}

